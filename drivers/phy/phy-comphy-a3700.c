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

static int mvebu_a3700_comphy_sata_power_on(struct mvebu_comphy_priv *priv,
					    struct mvebu_comphy *comphy)
{
	dev_dbg(priv->dev, "%s: Enter\n", __func__);

	mvebu_a3700_comphy_set_phy_selector(priv, comphy);

	dev_err(priv->dev, "SATA mode is not implemented\n");

	dev_dbg(priv->dev, "%s: Exit\n", __func__);

	return -ENOTSUPP;
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

