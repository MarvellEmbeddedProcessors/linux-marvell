/*
 * Copyright (C) 2013 Marvell
 * Author: Gregory CLEMENT <gregory.clement@free-electrons.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * version 2 as published by the Free Software Foundation.
 */

#include <linux/clk.h>
#include <linux/io.h>
#include <linux/mbus.h>
#include <linux/of.h>
#include <linux/platform_device.h>

#include "xhci.h"

#define USB3_MAX_WINDOWS	4
#define USB3_WIN_CTRL(w)	(0x0 + ((w) * 8))
#define USB3_WIN_BASE(w)	(0x4 + ((w) * 8))

struct xhci_mvebu_priv {
	void __iomem *base;
	struct clk *clk;
};

static void mv_usb3_conf_mbus_windows(void __iomem *base,
				      const struct mbus_dram_target_info *dram)
{
	int win;

	/* Clear all existing windows */
	for (win = 0; win < USB3_MAX_WINDOWS; win++) {
		writel(0, base + USB3_WIN_CTRL(win));
		writel(0, base + USB3_WIN_BASE(win));
	}

	/* Program each DRAM CS in a seperate window */
	for (win = 0; win < dram->num_cs; win++) {
		const struct mbus_dram_window *cs = dram->cs + win;

		writel(((cs->size - 1) & 0xffff0000) | (cs->mbus_attr << 8) |
		       (dram->mbus_dram_target_id << 4) | 1,
		       base + USB3_WIN_CTRL(win));

		writel((cs->base & 0xffff0000), base + USB3_WIN_BASE(win));
	}
}

int xhci_mvebu_probe(struct platform_device *pdev)
{
	struct resource	*res;
	struct xhci_mvebu_priv *priv;
	void __iomem	*base;
	const struct mbus_dram_target_info *dram;
	int ret;
	struct clk *clk;

	priv = devm_kzalloc(&pdev->dev, sizeof(struct xhci_mvebu_priv),
			    GFP_KERNEL);
	if (!priv)
		return -ENOMEM;

	res = platform_get_resource(pdev, IORESOURCE_MEM, 1);
	if (!res)
		return -ENODEV;

	base = devm_ioremap_resource(&pdev->dev, res);
	if (!base)
		return -ENOMEM;

	clk = devm_clk_get(&pdev->dev, NULL);
	if (IS_ERR(clk)) {
		return PTR_ERR(clk);
	}

	ret = clk_prepare_enable(clk);
	if (ret < 0) {
		return ret;
	}

	dram = mv_mbus_dram_info();
	mv_usb3_conf_mbus_windows(base, dram);

	priv->base = base;
	priv->clk = clk;

	ret = common_xhci_plat_probe(pdev, priv);
	if (ret < 0) {
		clk_disable_unprepare(clk);
		return ret;
	}

	return ret;
}

int xhci_mvebu_remove(struct platform_device *pdev)
{
	struct usb_hcd *hcd = platform_get_drvdata(pdev);
	struct xhci_hcd	*xhci = hcd_to_xhci(hcd);
	struct xhci_mvebu_priv *priv = (struct xhci_mvebu_priv *)xhci->priv;
	struct clk *clk = priv->clk;

	common_xhci_plat_remove(pdev);
	clk_disable_unprepare(clk);

	return 0;
}

void xhci_mvebu_resume(struct device *dev)
{
	const struct mbus_dram_target_info *dram;
	struct usb_hcd *hcd = dev_get_drvdata(dev);
	struct xhci_hcd	*xhci = hcd_to_xhci(hcd);
	struct xhci_mvebu_priv *priv = (struct xhci_mvebu_priv *)xhci->priv;
	void __iomem *base = priv->base;

	dram = mv_mbus_dram_info();
	mv_usb3_conf_mbus_windows(base, dram);
}
