/*
 * Marvell UTMI PHY driver
 *
 * Copyright (C) 2017 Marvell
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#include <linux/delay.h>
#include <linux/io.h>
#include <linux/module.h>
#include <linux/of_device.h>
#include <linux/phy/phy.h>
#include <linux/platform_device.h>
#include <dt-bindings/phy/phy-utmi-mvebu.h>

#define PLL_LOCK_TIMEOUT		1000

/* SoC Armada 37xx UTMI register macro definetions */
#define USB2_PHY_PLL_CTRL_REG0		(0x0)
#define PLL_REF_DIV_OFF			(0)
#define PLL_REF_DIV_MASK		(0x7F << PLL_REF_DIV_OFF)
#define PLL_REF_DIV_5			(0x5)
#define PLL_FB_DIV_OFF			(16)
#define PLL_FB_DIV_MASK			(0x1FF << PLL_FB_DIV_OFF)
#define PLL_FB_DIV_96			(96)
#define PLL_SEL_LPFR_OFF		(28)
#define PLL_SEL_LPFR_MASK		(0x3 << PLL_SEL_LPFR_OFF)
#define PLL_READY			BIT(31)
/* Both UTMI PHY and UTMI OTG PHY */
#define USB2_PHY_CAL_CTRL_ADDR		(0x8)
#define PHY_PLLCAL_DONE			BIT(31)
#define PHY_IMPCAL_DONE			BIT(23)
#define USB2_RX_CHAN_CTRL1_ADDR		(0x18)
#define USB2PHY_SQCAL_DONE		BIT(31)
#define USB2_PHY_OTG_CTRL_ADDR		(0x34)
#define PHY_PU_OTG			BIT(4)
#define USB2_PHY_CHRGR_DET_ADDR		(0x38)
#define PHY_CDP_EN			BIT(2)
#define PHY_DCP_EN			BIT(3)
#define PHY_PD_EN			BIT(4)
#define PHY_PU_CHRG_DTC			BIT(5)
#define PHY_CDP_DM_AUTO			BIT(7)
#define PHY_ENSWITCH_DP			BIT(12)
#define PHY_ENSWITCH_DM			BIT(13)
#define USB2_PHY2_CTRL_ADDR		(0x804)
#define USB2_PHY2_SUSPM			BIT(7)
#define USB2_PHY2_PU			BIT(0)
#define USB2_OTG_PHY_CTRL_ADDR		(0x820)
#define USB2_OTGPHY2_SUSPM		BIT(14)
#define USB2_OTGPHY2_PU			BIT(0)
#define USB2_DP_PULLDN_DEV_MODE		BIT(5)
#define USB2_DM_PULLDN_DEV_MODE		BIT(6)

#define USB2_PHY_CTRL_ADDR(usb32) (usb32 == 0 ? USB2_PHY2_CTRL_ADDR :\
						USB2_OTG_PHY_CTRL_ADDR)
#define RB_USB2PHY_SUSPM(usb32) (usb32 == 0 ? USB2_PHY2_SUSPM :\
						USB2_OTGPHY2_SUSPM)
#define RB_USB2PHY_PU(usb32) (usb32 == 0 ? USB2_PHY2_PU : USB2_OTGPHY2_PU)

struct mvebu_utmi_phy {
	struct phy *phy;
	void __iomem *regs;
	u32 connect_to;
	u32 index;
};

/**************************************************************************
  * poll_reg
  *
  * return: 0 on success, or timeout
 **************************************************************************/
static inline u32 poll_reg(void __iomem *addr, u32 val, u32 mask, u32 timeout)
{
	u32 rval = 0xdead;

	for (; timeout > 0; timeout--) {
		rval = readl(addr);
		if ((rval & mask) == val)
			return 0;

		mdelay(10);
	}

	return -ETIME;
}

static inline void reg_set(void __iomem *addr, u32 data, u32 mask)
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
	bool usb32;
	struct mvebu_utmi_phy *utmi_phy = phy_get_drvdata(phy);
	struct device *dev = &phy->dev;

	dev_dbg(dev, "%s: Enter\n", __func__);

	if (!utmi_phy)
		return -ENODEV;

	/* On Armada 37xx there are 2 kinds of USB controller, one is USB2
	 * controller, which only supports USB2 protocol, and another is USB32
	 * controller, which supports both USB2 and USB3. To support USB2, both
	 * USB controllers need connect to corresponding UTMI PHY. So a flag of
	 * usb32 is needed to differentiate the USB controller that the UTMI
	 * connect to, and run corresponding initialization.
	 */
	if (utmi_phy->connect_to == UTMI_PHY_TO_USB3_HOST0)
		usb32 = true;
	else
		usb32 = false;

	/* 0. Setup PLL. 40MHz clock uses defaults.It is 25MHz now.
	 *    See "PLL Settings for Typical REFCLK" table
	 */
	data = (PLL_REF_DIV_5 << PLL_REF_DIV_OFF);
	data |= (PLL_FB_DIV_96 << PLL_FB_DIV_OFF);
	mask = PLL_REF_DIV_MASK | PLL_FB_DIV_MASK | PLL_SEL_LPFR_MASK;
	reg_set(USB2_PHY_PLL_CTRL_REG0 + utmi_phy->regs, data, mask);

	/* 1. PHY pull up and disable USB2 suspend */
	reg_set(USB2_PHY_CTRL_ADDR(usb32) + utmi_phy->regs,
		RB_USB2PHY_SUSPM(usb32) | RB_USB2PHY_PU(usb32), 0);

	if (usb32) {
		/* 2. Power up OTG module */
		reg_set(USB2_PHY_OTG_CTRL_ADDR + utmi_phy->regs, PHY_PU_OTG, 0);

		/* 3. Configure PHY charger detection */
		reg_set(USB2_PHY_CHRGR_DET_ADDR + utmi_phy->regs, 0,
			PHY_CDP_EN | PHY_DCP_EN | PHY_PD_EN | PHY_CDP_DM_AUTO |
			PHY_ENSWITCH_DP | PHY_ENSWITCH_DM | PHY_PU_CHRG_DTC);

		/* 4. Clear USB2 Host and Device OTG phy DP/DM pull-down,
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
		dev_err(dev, "Failed to end USB2 PLL calibration\n");
		return ret;
	}

	/* Assert impedance calibration done */
	ret = poll_reg(USB2_PHY_CAL_CTRL_ADDR + utmi_phy->regs,
		       PHY_IMPCAL_DONE,
		       PHY_IMPCAL_DONE,
		       PLL_LOCK_TIMEOUT);
	if (ret) {
		dev_err(dev, "Failed to end USB2 impedance calibration\n");
		return ret;
	}

	/* Assert squetch calibration done */
	ret = poll_reg(USB2_RX_CHAN_CTRL1_ADDR + utmi_phy->regs,
		       USB2PHY_SQCAL_DONE,
		       USB2PHY_SQCAL_DONE,
		       PLL_LOCK_TIMEOUT);
	if (ret) {
		dev_err(dev, "Failed to end USB2 unknown calibration\n");
		return ret;
	}

	/* Assert PLL is ready */
	ret = poll_reg(USB2_PHY_PLL_CTRL_REG0 + utmi_phy->regs,
		       PLL_READY,
		       PLL_READY,
		       PLL_LOCK_TIMEOUT);

	if (ret) {
		dev_err(dev, "Failed to lock USB2 PLL\n");
		return ret;
	}

	dev_dbg(dev, "%s: Exit\n", __func__);

	return ret;
}

static int mvebu_a3700_utmi_phy_power_off(struct phy *phy)
{
	bool usb32;
	struct mvebu_utmi_phy *utmi_phy = phy_get_drvdata(phy);

	dev_dbg(&phy->dev, "%s: Enter\n", __func__);

	if (!utmi_phy)
		return -ENODEV;

	if (utmi_phy->connect_to == UTMI_PHY_TO_USB3_HOST0)
		usb32 = true;
	else
		usb32 = false;

	/* 1. PHY pull down and enable USB2 suspend */
	reg_set(USB2_PHY_CTRL_ADDR(usb32) + utmi_phy->regs,
		0, RB_USB2PHY_SUSPM(usb32) | RB_USB2PHY_PU(usb32));

	/* 2. Power down OTG module */
	if (usb32)
		reg_set(USB2_PHY_OTG_CTRL_ADDR + utmi_phy->regs, 0, PHY_PU_OTG);

	dev_dbg(&phy->dev, "%s: Exit\n", __func__);

	return 0;
}

const struct phy_ops armada3700_utmi_phy_ops = {
	.power_on = mvebu_a3700_utmi_phy_power_on,
	.power_off = mvebu_a3700_utmi_phy_power_off,
	.owner = THIS_MODULE,
};

static const struct of_device_id mvebu_utmi_of_match[] = {
	{
		.compatible = "marvell,armada-3700-utmi-phy",
		.data = &armada3700_utmi_phy_ops
	},
	{ },
};
MODULE_DEVICE_TABLE(of, mvebu_utmi_of_match);

static int mvebu_utmi_phy_probe(struct platform_device *pdev)
{
	int ret;
	struct device *dev = &pdev->dev;
	struct phy *phy;
	struct resource *res;
	struct phy_provider *phy_provider;
	struct mvebu_utmi_phy *utmi_phy;
	const struct of_device_id *match;
	struct phy_ops *mvebu_utmi_phy_ops;

	match = of_match_device(mvebu_utmi_of_match, &pdev->dev);
	mvebu_utmi_phy_ops = (struct phy_ops *)match->data;

	utmi_phy = devm_kzalloc(dev, sizeof(*utmi_phy), GFP_KERNEL);
	if (!utmi_phy)
		return  -ENOMEM;

	/* Get UTMI register base */
	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	if (!res) {
		dev_err(&pdev->dev, "no UTMI PHY memory resource\n");
		return -ENODEV;
	}

	utmi_phy->regs = devm_ioremap_resource(&pdev->dev, res);
	if (IS_ERR(utmi_phy->regs))
		return PTR_ERR(utmi_phy->regs);

	/* Get property of utmi-port, used check the UTMI connect to where */
	ret = of_property_read_u32(pdev->dev.of_node, "utmi-port",
				   &utmi_phy->connect_to);
	if (ret) {
		dev_err(&pdev->dev, "no property of utmi-port\n");
		return ret;
	}

	/* Get the UTMI PHY index in case there are more than one UTMI */
	ret = of_property_read_u32(pdev->dev.of_node, "utmi-index",
				   &utmi_phy->index);
	if (ret)
		utmi_phy->index = 0;

	phy = devm_phy_create(dev, NULL, mvebu_utmi_phy_ops);
	if (IS_ERR(phy)) {
		dev_err(dev, "failed to create PHY\n");
		return PTR_ERR(phy);
	}

	utmi_phy->phy = phy;
	dev_set_drvdata(dev, utmi_phy);
	phy_set_drvdata(phy, utmi_phy);

	/* Power off the PHY as default */
	mvebu_utmi_phy_ops->power_off(phy);

	phy_provider = devm_of_phy_provider_register(&pdev->dev,
						     of_phy_simple_xlate);

	return PTR_ERR_OR_ZERO(phy_provider);
}

static struct platform_driver mvebu_utmi_driver = {
	.probe	= mvebu_utmi_phy_probe,
	.driver	= {
		.name		= "phy-mvebu-utmi",
		.owner		= THIS_MODULE,
		.of_match_table	= mvebu_utmi_of_match,
	 },
};
module_platform_driver(mvebu_utmi_driver);

MODULE_AUTHOR("Evan Wang <xswang@marvell.com>");
MODULE_DESCRIPTION("Marvell EBU UTMI PHY driver");
MODULE_LICENSE("GPL");

