/*
 * Copyright (C) 2016 Marvell
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * Armada 3700 comes with an USB2 host and device controller and an
 * USB32 controller. Each of them has a UTMI PHY for USB2 protocol.
 */

#include <linux/delay.h>
#include <linux/of_device.h>
#include <linux/init.h>
#include <linux/io.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/of_address.h>
#include <linux/phy/phy.h>
#include <linux/platform_device.h>
#include <dt-bindings/phy/phy-armada3700-utmi.h>
#include "phy-armada3700-utmi.h"


struct mvebu_a3700_utmi_phy {
	struct phy *phy;
	void __iomem *regs;
	bool usb32;
};

/**************************************************************************
  * poll_reg
  *
  * return: 0 on success, 1 on timeout
 **************************************************************************/
static u32 poll_reg(void __iomem *addr, u32 val, u32 mask, u32 timeout)
{
	u32 rval = 0xdead;

	for (; timeout > 0; timeout--) {
		rval = readl(addr);
		if ((rval & mask) == val)
			return 0;

		mdelay(10);
	}

	return 1;
}

static void reg_set(void __iomem *addr, u32 data, u32 mask)
{
	u32 reg_data;

	reg_data = readl(addr);
	reg_data &= ~mask;
	reg_data |= data;
	writel(reg_data, addr);
}

static int mvebu_a3700_utmi_phy_power_on(struct phy *phy)
{
	int ret = 0;
	u32 data, mask;
	struct mvebu_a3700_utmi_phy *utmi_phy = phy_get_drvdata(phy);

	dev_dbg(&phy->dev, "%s: Enter\n", __func__);

	if (!utmi_phy)
		return -ENODEV;

	/*
	 * 0. Setup PLL. 40MHz clock uses defaults.It is 25MHz now.
	 *    See "PLL Settings for Typical REFCLK" table
	 */
	data = (PLL_REF_DIV_5 << PLL_REF_DIV_OFF) | (PLL_FB_DIV_96 << PLL_FB_DIV_OFF);
	mask = PLL_REF_DIV_MASK | PLL_FB_DIV_MASK | PLL_SEL_LPFR_MASK;
	reg_set(USB2_PHY_PLL_CTRL_REG0 + utmi_phy->regs, data, mask);

	/*
	 * 1. PHY pull up and disable USB2 suspend
	 */
	reg_set(USB2_PHY_CTRL_ADDR(utmi_phy->usb32) + utmi_phy->regs,
		RB_USB2PHY_SUSPM(utmi_phy->usb32) | RB_USB2PHY_PU(utmi_phy->usb32), 0);

	if (utmi_phy->usb32) {
		/*
		 * 2. Power up OTG module
		 */
		reg_set(USB2_PHY_OTG_CTRL_ADDR + utmi_phy->regs, PHY_PU_OTG, 0);

		/*
		 * 3. Configure PHY charger detection
		 */
		reg_set(USB2_PHY_CHRGR_DET_ADDR + utmi_phy->regs, 0,
			PHY_CDP_EN | PHY_DCP_EN | PHY_PD_EN | PHY_CDP_DM_AUTO |
			PHY_ENSWITCH_DP | PHY_ENSWITCH_DM | PHY_PU_CHRG_DTC);

		/*
		 * 4. Clear USB2 Host and Device OTG phy DP/DM pull-down,
		 *    needed in evice mode
		 */
		reg_set(USB2_OTG_PHY_CTRL_ADDR + utmi_phy->regs, 0,
			USB2_DP_PULLDN_DEV_MODE | USB2_DM_PULLDN_DEV_MODE);
	}

	/* Assert PLL calibration done */
	ret = poll_reg(USB2_PHY_CAL_CTRL_ADDR + utmi_phy->regs,
		       PHY_PLLCAL_DONE,
		       PHY_PLLCAL_DONE,
		       PLL_LOCK_TIMEOUT);
	if (ret) {
		dev_err(&phy->dev, "Failed to end USB2 PLL calibration\n");
		return ret;
	}

	/* Assert impedance calibration done */
	ret = poll_reg(USB2_PHY_CAL_CTRL_ADDR + utmi_phy->regs,
		       PHY_IMPCAL_DONE,
		       PHY_IMPCAL_DONE,
		       PLL_LOCK_TIMEOUT);
	if (ret) {
		dev_err(&phy->dev, "Failed to end USB2 impedance calibration\n");
		return ret;
	}

	/* Assert squetch calibration done */
	ret = poll_reg(USB2_RX_CHAN_CTRL1_ADDR + utmi_phy->regs,
		       USB2PHY_SQCAL_DONE,
		       USB2PHY_SQCAL_DONE,
		       PLL_LOCK_TIMEOUT);
	if (ret) {
		dev_err(&phy->dev, "Failed to end USB2 unknown calibration\n");
		return ret;
	}

	/* Assert PLL is ready */
	ret = poll_reg(USB2_PHY_PLL_CTRL_REG0 + utmi_phy->regs,
		       PLL_READY,
		       PLL_READY,
		       PLL_LOCK_TIMEOUT);

	if (ret) {
		dev_err(&phy->dev, "Failed to lock USB2 PLL\n");
		return ret;
	}

	dev_dbg(&phy->dev, "%s: Exit\n", __func__);

	return ret;
}

static int mvebu_a3700_utmi_phy_power_off(struct phy *phy)
{
	struct mvebu_a3700_utmi_phy *utmi_phy = phy_get_drvdata(phy);

	dev_dbg(&phy->dev, "%s: Enter\n", __func__);

	if (!utmi_phy)
		return -ENODEV;

	/*
	 * 1. PHY pull down and enable USB2 suspend
	 */
	reg_set(USB2_PHY_CTRL_ADDR(utmi_phy->usb32) + utmi_phy->regs,
		0, RB_USB2PHY_SUSPM(utmi_phy->usb32) | RB_USB2PHY_PU(utmi_phy->usb32));

	/*
	 * 2. Power down OTG module
	 */
	if (utmi_phy->usb32)
		reg_set(USB2_PHY_OTG_CTRL_ADDR + utmi_phy->regs, 0, PHY_PU_OTG);

	dev_dbg(&phy->dev, "%s: Exit\n", __func__);

	return 0;
}

static const struct phy_ops armada3700_utmi_phy_ops = {
	.power_on = mvebu_a3700_utmi_phy_power_on,
	.power_off = mvebu_a3700_utmi_phy_power_off,
	.owner = THIS_MODULE,
};

static int mvebu_armada3700_utmi_phy_probe(struct platform_device *pdev)
{
	int ret;
	u32 utmi_port = 0;
	struct device *dev = &pdev->dev;
	struct phy *phy;
	struct resource *res;
	struct phy_provider *phy_provider;
	struct mvebu_a3700_utmi_phy *utmi_phy;

	utmi_phy = devm_kzalloc(dev, sizeof(*utmi_phy), GFP_KERNEL);
	if (!utmi_phy)
		return  -ENOMEM;

	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	if (!res) {
		dev_err(&pdev->dev, "no UTMI PHY memory resource\n");
		ret = -ENODEV;
		goto free_utmi_phy;
	}

	utmi_phy->regs = devm_ioremap_resource(&pdev->dev, res);
	if (IS_ERR(utmi_phy->regs)) {
		ret = PTR_ERR(utmi_phy->regs);
		goto free_utmi_phy;
	}

	ret = of_property_read_u32(pdev->dev.of_node, "utmi-port", &utmi_port);
	if (ret) {
		dev_err(&pdev->dev, "no property of utmi-port\n");
		goto unmap_phy;
	}

	if (utmi_port == UTMI_PHY_TO_USB3_HOST0)
		utmi_phy->usb32 = true;
	else
		utmi_phy->usb32 = false;

	phy = devm_phy_create(dev, NULL, &armada3700_utmi_phy_ops);
	if (IS_ERR(phy)) {
		dev_err(dev, "failed to create PHY\n");
		ret = PTR_ERR(phy);
		goto unmap_phy;
	}

	utmi_phy->phy = phy;

	dev_set_drvdata(dev, utmi_phy);
	phy_set_drvdata(phy, utmi_phy);

	phy_provider = devm_of_phy_provider_register(&pdev->dev,
						     of_phy_simple_xlate);

	ret = PTR_ERR_OR_ZERO(phy_provider);
	if (ret)
		goto unmap_phy;

	return ret;

unmap_phy:
	iounmap(utmi_phy->regs);

free_utmi_phy:
	devm_kfree(dev, utmi_phy);

	return ret;
}

static const struct of_device_id of_usb_utmi_table[] = {
	{ .compatible = "marvell,armada-3700-utmi-phy", },
	{ /* end of list */ },
};
MODULE_DEVICE_TABLE(of, of_usb_utmi_table);

static struct platform_driver armada3700_utmi_phy_driver = {
	.probe	= mvebu_armada3700_utmi_phy_probe,
	.driver = {
		.of_match_table	= of_usb_utmi_table,
		.name  = "armada3700_utmi_phy_driver",
	}
};
module_platform_driver(armada3700_utmi_phy_driver);

MODULE_DESCRIPTION("Armada 3700 UTMI PHY driver");
MODULE_AUTHOR("Evan Wang <xswang@marvell.com>");
MODULE_LICENSE("GPL");

