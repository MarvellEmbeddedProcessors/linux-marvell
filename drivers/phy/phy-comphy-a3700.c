/*
 * Marvell comphy driver
 *
 * Copyright (C) 2016 Marvell
 *
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
#include <dt-bindings/phy/phy-comphy-mvebu.h>

#include "phy-comphy-mvebu.h"
#include "phy-comphy-a3700.h"


/* PHY selector configures with corresponding modes */
static void mvebu_a3700_comphy_set_phy_selector(struct mvebu_comphy_priv *priv,
						struct mvebu_comphy *comphy)
{
	u32 reg;
	int mode = COMPHY_GET_MODE(priv->lanes[comphy->index].mode);


	reg = readl(priv->comphy_regs + COMPHY_SELECTOR_PHY_REG_OFFSET);
	switch (mode) {
	case (COMPHY_SATA_MODE):
		/* SATA must be in Lane2 */
		if (comphy->index == COMPHY_LANE2)
			reg &= ~COMPHY_SELECTOR_USB3_PHY_SEL_BIT;
		else
			dev_err(priv->dev, "COMPHY[%d] mode[%d] is invalid\n", comphy->index, mode);
		break;

	case (COMPHY_SGMII_MODE):
	case (COMPHY_HS_SGMII_MODE):
		if (comphy->index == COMPHY_LANE1)
			reg &= ~COMPHY_SELECTOR_USB3_GBE1_SEL_BIT;
		else if (comphy->index == COMPHY_LANE0)
			reg &= ~COMPHY_SELECTOR_PCIE_GBE0_SEL_BIT;
		else
			dev_err(priv->dev, "COMPHY[%d] mode[%d] is invalid\n", comphy->index, mode);
		break;

	case (COMPHY_USB3H_MODE):
	case (COMPHY_USB3D_MODE):
	case (COMPHY_USB3_MODE):
		if (comphy->index == COMPHY_LANE2)
			reg |= COMPHY_SELECTOR_USB3_PHY_SEL_BIT;
		else if (comphy->index == COMPHY_LANE1)
			reg |= COMPHY_SELECTOR_USB3_GBE1_SEL_BIT;
		else
			dev_err(priv->dev, "COMPHY[%d] mode[%d] is invalid\n", comphy->index, mode);
		break;

	case (COMPHY_PCIE_MODE):
		/* PCIE must be in Lane0 */
		if (comphy->index == COMPHY_LANE0)
			reg |= COMPHY_SELECTOR_PCIE_GBE0_SEL_BIT;
		else
			dev_err(priv->dev, "COMPHY[%d] mode[%d] is invalid\n", comphy->index, mode);
		break;

	default:
		dev_err(priv->dev, "COMPHY[%d] mode[%d] is invalid\n", comphy->index, mode);
		break;
	}

	writel(reg, priv->comphy_regs + COMPHY_SELECTOR_PHY_REG_OFFSET);
}

/***************************************************************************************************
  * mvebu_comphy_reg_set_indirect
  * It is only used for SATA and USB3 on comphy lane2.
  * return: void
 ***************************************************************************************************/
static void mvebu_comphy_reg_set_indirect(void __iomem *addr, u32 reg_offset, u16 data, u16 mask, int mode)
{
	/*
	 * When Lane 2 PHY is for USB3, access the PHY registers
	 * through indirect Address and Data registers INDIR_ACC_PHY_ADDR (RD00E0178h [31:0]) and
	 * INDIR_ACC_PHY_DATA (RD00E017Ch [31:0]) within the SATA Host Controller registers, Lane 2
	 * base register offset is 0x200
	 */
	if (mode == COMPHY_UNUSED)
		return;

	if (mode == COMPHY_SATA_MODE)
		writel(reg_offset, addr + COMPHY_LANE2_INDIR_ADDR_OFFSET);
	else
		writel(reg_offset + USB3PHY_LANE2_REG_BASE_OFFSET, addr + COMPHY_LANE2_INDIR_ADDR_OFFSET);

	reg_set(addr + COMPHY_LANE2_INDIR_DATA_OFFSET, data, mask);
}

static int mvebu_a3700_comphy_sata_power_on(struct mvebu_comphy_priv *priv,
					    struct mvebu_comphy *comphy)
{
	int ret = 0;
	u32 reg_offset, data = 0;
	void __iomem *comphy_indir_regs;
	struct resource *res;
	struct platform_device *pdev = container_of(priv->dev, struct platform_device, dev);
	int mode = COMPHY_GET_MODE(priv->lanes[comphy->index].mode);
	int invert = COMPHY_GET_POLARITY_INVERT(priv->lanes[comphy->index].mode);

	dev_dbg(priv->dev, "%s: Enter\n", __func__);

	/* Get the indirect access register resource and map */
	res = platform_get_resource_byname(pdev, IORESOURCE_MEM, "indirect");
	if (res) {
		comphy_indir_regs = devm_ioremap_resource(&pdev->dev, res);
		if (IS_ERR(comphy_indir_regs))
			return PTR_ERR(comphy_indir_regs);
	} else {
		dev_err(priv->dev, "no inirect register resource\n");
		return -ENOTSUPP;
	}

	/* Configure phy selector for SATA */
	mvebu_a3700_comphy_set_phy_selector(priv, comphy);

	/*
	 * 0. Check the Polarity invert bits
	 */
	if (invert & COMPHY_POLARITY_TXD_INVERT)
		data |= TXD_INVERT_BIT;
	if (invert & COMPHY_POLARITY_RXD_INVERT)
		data |= RXD_INVERT_BIT;

	reg_offset = COMPHY_SYNC_PATTERN_REG + SATAPHY_LANE2_REG_BASE_OFFSET;
	mvebu_comphy_reg_set_indirect(comphy_indir_regs,
				      reg_offset,
				      data,
				      TXD_INVERT_BIT | RXD_INVERT_BIT,
				      mode);

	/*
	 * 1. Select 40-bit data width width
	 */
	reg_offset = COMPHY_LOOPBACK_REG0 + SATAPHY_LANE2_REG_BASE_OFFSET;
	mvebu_comphy_reg_set_indirect(comphy_indir_regs,
				      reg_offset,
				      (DATA_WIDTH_40BIT << SEL_DATA_WIDTH_OFFSET),
				      SEL_DATA_WIDTH_MASK,
				      mode);

	/*
	 * 2. Select reference clock(25M) and PHY mode (SATA)
	 */
	reg_offset = COMPHY_POWER_PLL_CTRL + SATAPHY_LANE2_REG_BASE_OFFSET;
	mvebu_comphy_reg_set_indirect(comphy_indir_regs,
				      reg_offset,
				      ((REF_CLOCK_SPEED_25M << REF_FREF_SEL_OFFSET) |
				       (PHY_MODE_SATA << PHY_MODE_OFFSET)),
				      REF_FREF_SEL_MASK | PHY_MODE_MASK,
				      mode);

	/*
	 * 3. Use maximum PLL rate (no power save)
	 */
	reg_offset = COMPHY_KVCO_CAL_CTRL + SATAPHY_LANE2_REG_BASE_OFFSET;
	mvebu_comphy_reg_set_indirect(comphy_indir_regs,
				      reg_offset,
				      USE_MAX_PLL_RATE_BIT,
				      USE_MAX_PLL_RATE_BIT,
				      mode);

	/*
	 * 4. Reset reserved bit
	 */
	mvebu_comphy_reg_set_indirect(comphy_indir_regs,
				      COMPHY_RESERVED_REG,
				      0,
				      PHYCTRL_FRM_PIN_BIT,
				      mode);

	/*
	 * 5. Set vendor-specific configuration (It is done in sata driver)
	 */

	/* Wait for > 55 us to allow PLL be enabled */
	udelay(PLL_SET_DELAY_US);

	/* Polling status */
	writel(COMPHY_LOOPBACK_REG0 + SATAPHY_LANE2_REG_BASE_OFFSET,
	       comphy_indir_regs + COMPHY_LANE2_INDIR_ADDR_OFFSET);
	ret = polling_with_timeout(comphy_indir_regs + COMPHY_LANE2_INDIR_DATA_OFFSET,
				   PLL_READY_TX_BIT,
				   PLL_READY_TX_BIT,
				   A3700_COMPHY_PLL_LOCK_TIMEOUT,
				   REG_32BIT);

	/* Unmap resource */
	devm_iounmap(&pdev->dev, comphy_indir_regs);
	devm_release_mem_region(&pdev->dev, res->start, resource_size(res));

	dev_dbg(priv->dev, "%s: Exit\n", __func__);

	return ret;
}

static int mvebu_a3700_comphy_sgmii_power_on(struct mvebu_comphy_priv *priv,
					     struct mvebu_comphy *comphy)
{
	dev_dbg(priv->dev, "%s: Enter\n", __func__);

	dev_err(priv->dev, "SGMII mode is not implemented\n");

	dev_dbg(priv->dev, "%s: Exit\n", __func__);

	return -ENOTSUPP;
}

static int mvebu_a3700_comphy_usb3_power_on(struct mvebu_comphy_priv *priv,
					    struct mvebu_comphy *comphy)
{
	dev_dbg(priv->dev, "%s: Enter\n", __func__);

	dev_err(priv->dev, "USB mode is not implemented\n");

	dev_dbg(priv->dev, "%s: Exit\n", __func__);

	return -ENOTSUPP;
}

static int mvebu_a3700_comphy_pcie_power_on(struct mvebu_comphy_priv *priv,
					    struct mvebu_comphy *comphy)
{
	dev_dbg(priv->dev, "%s: Enter\n", __func__);

	dev_err(priv->dev, "PCIE mode is not implemented\n");

	dev_dbg(priv->dev, "%s: Exit\n", __func__);

	return -ENOTSUPP;
}

static int mvebu_a3700_comphy_power_on(struct phy *phy)
{
	struct mvebu_comphy *comphy = phy_get_drvdata(phy);
	struct mvebu_comphy_priv *priv = to_mvebu_comphy_priv(comphy);
	int mode = COMPHY_GET_MODE(priv->lanes[comphy->index].mode);
	int err = 0;

	dev_dbg(priv->dev, "%s: Enter\n", __func__);

	spin_lock(&priv->lock);

	switch (mode) {
	case(COMPHY_SATA_MODE):
		err = mvebu_a3700_comphy_sata_power_on(priv, comphy);
		break;

	case(COMPHY_SGMII_MODE):
	case(COMPHY_HS_SGMII_MODE):
		err = mvebu_a3700_comphy_sgmii_power_on(priv, comphy);
		break;

	case (COMPHY_USB3_MODE):
		err = mvebu_a3700_comphy_usb3_power_on(priv, comphy);
		break;

	case (COMPHY_PCIE_MODE):
		err = mvebu_a3700_comphy_pcie_power_on(priv, comphy);
		break;

	default:
		dev_err(priv->dev, "comphy%d: unsupported comphy mode\n",
			comphy->index);
		err = -EINVAL;
		break;
	}

	spin_unlock(&priv->lock);

	dev_dbg(priv->dev, "%s: Exit\n", __func__);

	return err;
}

static int mvebu_a3700_comphy_power_off(struct phy *phy)
{
	struct mvebu_comphy *comphy = phy_get_drvdata(phy);
	struct mvebu_comphy_priv *priv = to_mvebu_comphy_priv(comphy);

	dev_dbg(priv->dev, "%s: Enter\n", __func__);

	spin_lock(&priv->lock);

	dev_dbg(priv->dev, "power off is not implemented\n");

	spin_unlock(&priv->lock);

	dev_dbg(priv->dev, "%s: Exit\n", __func__);

	return 0;
}

static struct phy_ops a3700_comphy_ops = {
	.power_on	= mvebu_a3700_comphy_power_on,
	.power_off	= mvebu_a3700_comphy_power_off,
	.set_mode	= mvebu_comphy_set_mode,
	.get_mode	= mvebu_comphy_get_mode,
	.owner		= THIS_MODULE,
};

const struct mvebu_comphy_soc_info a3700_comphy = {
	.num_of_lanes = 3,
	.functions = {
		/* Lane 0 */
		{COMPHY_UNUSED, COMPHY_PCIE0, COMPHY_SGMII0},
		/* Lane 1 */
		{COMPHY_UNUSED, COMPHY_SGMII1, COMPHY_HS_SGMII1, COMPHY_USB3},
		/* Lane 2 */
		{COMPHY_UNUSED, COMPHY_SATA0, COMPHY_USB3},
	},
	.comphy_ops = &a3700_comphy_ops,
};

