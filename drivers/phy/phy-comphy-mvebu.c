/*
 * Marvell comphy driver
 *
 * Copyright (C) 2016 Marvell
 *
 * Igal Liberman <igall@marvell.com>
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
#include "phy-comphy-cp110.h"
#include "phy-comphy-a3700.h"

/* mvebu_comphy_set_mode: shared by all SoCs */
int mvebu_comphy_set_mode(struct phy *phy, enum phy_mode mode)
{
	struct mvebu_comphy *comphy = phy_get_drvdata(phy);
	struct mvebu_comphy_priv *priv = to_mvebu_comphy_priv(comphy);
	int i;

	dev_dbg(priv->dev, "%s: Enter\n", __func__);

	for (i = 0; i < MVEBU_COMPHY_FUNC_MAX; i++) {
		/* We need to find a match between requested mode and
		 * the SoC configuration which is stored in
		 * mvebu_comphy_soc_info.
		 */
		int functions = priv->soc_info->functions[comphy->index][i];

		if (COMPHY_GET_MODE(functions) == COMPHY_GET_MODE((int)mode) &&
		    COMPHY_GET_ID(functions) == COMPHY_GET_ID((int)mode))
			break;
	}

	if (i == MVEBU_COMPHY_FUNC_MAX) {
		dev_err(priv->dev, "can't set mode 0x%x for COMPHY%d\n",
			mode, comphy->index);
		return -EINVAL;
	}

	priv->lanes[comphy->index].mode = (int)mode;

	dev_dbg(priv->dev, "%s: Exit\n", __func__);

	return 0;
}

/* mvebu_comphy_get_mode: shared by all SoCs */
enum phy_mode mvebu_comphy_get_mode(struct phy *phy)
{
	struct mvebu_comphy *comphy = phy_get_drvdata(phy);
	struct mvebu_comphy_priv *priv = to_mvebu_comphy_priv(comphy);
	int mode;

	dev_dbg(priv->dev, "%s: Enter\n", __func__);

	mode = priv->lanes[comphy->index].mode;

	dev_dbg(priv->dev, "%s: Exit\n", __func__);

	return (enum phy_mode)mode;
}

static const struct of_device_id mvebu_comphy_of_match[] = {
#ifdef CONFIG_PHY_MVEBU_COMPHY_CP110
	{ .compatible = "marvell,cp110-comphy", .data = &cp110_comphy },
#endif /* CONFIG_PHY_MVEBU_COMPHY_CP110 */
#ifdef CONFIG_PHY_MVEBU_COMPHY_A3700
	{ .compatible = "marvell,armada-3700-comphy", .data = &a3700_comphy },
#endif /* CONFIG_PHY_MVEBU_COMPHY_A3700 */
	{ },
};
MODULE_DEVICE_TABLE(of, mvebu_comphy_of_match);

/**
 * mvebu_comphy_of_xlate
 *
 * @dev - pointer to the device structure
 * @args - pointer to the lane information (id and mode from the device-tree).
 *
 * This callback is registered during probe and called by the generic phy
 * infrastructure when phy consumer calls 'devm_of_phy_get'.
 * This function has 2 purposes:
 *	- Check if the requested configuration is valid.
 *	- Update comphy internal structure with the configuration for the
 *	  specific lane (by default, all lanes set to 'COMPHY_UNUSED'.
 *
 * Return: pointer to the associated phy (on success), error code otherwise
 */
static struct phy *mvebu_comphy_of_xlate(struct device *dev,
					 struct of_phandle_args *args)
{
	struct mvebu_comphy_priv *priv = dev_get_drvdata(dev);
	int lane = args->args[0];
	int mode = args->args[1];
	int i;

	dev_dbg(priv->dev, "%s: Enter\n", __func__);

	if (lane >= (int)priv->soc_info->num_of_lanes) {
		dev_err(dev, "Wrong lane number %d for PHY, max is %d\n",
			lane, priv->soc_info->num_of_lanes);
		return ERR_PTR(-ENODEV);
	}

	for (i = 0; i < MVEBU_COMPHY_FUNC_MAX; i++) {
		/* We need to find a match between requested mode and
		 * the SoC configuration which is stored in
		 * mvebu_comphy_soc_info.
		 */
		int functions = priv->soc_info->functions[lane][i];

		if (COMPHY_GET_MODE(functions) == COMPHY_GET_MODE(mode) &&
		    COMPHY_GET_ID(functions) == COMPHY_GET_ID(mode))
			break;
	}

	if (i == MVEBU_COMPHY_FUNC_MAX) {
		dev_err(dev, "Wrong mode 0x%x for COMPHY\n", mode);
		return ERR_PTR(-ENODEV);
	}

	priv->lanes[lane].mode = mode;

	dev_dbg(priv->dev, "%s: Exit\n", __func__);

	return priv->lanes[lane].phy;
}

static int mvebu_comphy_probe(struct platform_device *pdev)
{
	const struct of_device_id *match;
	struct mvebu_comphy_priv *priv;
	struct resource *res;
	struct phy_provider *phy_provider;
	const struct mvebu_comphy_soc_info *soc_info;
	bool skip_pcie_power_off;
	int i;

	dev_dbg(priv->dev, "%s: Enter\n", __func__);

	match = of_match_device(mvebu_comphy_of_match, &pdev->dev);
	soc_info = match->data;
	priv = devm_kzalloc(&pdev->dev, sizeof(*priv), GFP_KERNEL);
	if (!priv)
		return -ENOMEM;

	res = platform_get_resource_byname(pdev, IORESOURCE_MEM, "comphy");
	priv->comphy_regs = devm_ioremap_resource(&pdev->dev, res);
	if (IS_ERR(priv->comphy_regs))
		return PTR_ERR(priv->comphy_regs);

	res = platform_get_resource_byname(pdev, IORESOURCE_MEM, "serdes");
	priv->comphy_pipe_regs = devm_ioremap_resource(&pdev->dev, res);
	if (IS_ERR(priv->comphy_pipe_regs))
		return PTR_ERR(priv->comphy_pipe_regs);

	/* check if skip_pcie_power_off flag exist */
	skip_pcie_power_off = of_property_read_bool(pdev->dev.of_node,
						    "skip_pcie_power_off");

	priv->soc_info = soc_info;
	priv->dev = &pdev->dev;
	spin_lock_init(&priv->lock);

	for (i = 0; i < soc_info->num_of_lanes; i++) {
		struct phy *phy;
		u32 reg_data;

		phy = devm_phy_create(&pdev->dev, NULL, soc_info->comphy_ops);
		if (IS_ERR(phy)) {
			dev_err(&pdev->dev, "failed to create PHY\n");
			return PTR_ERR(phy);
		}

		/* In this stage we have no information regarding comphy
		 * configuration so we set all comphys to UNUSED.
		 * Later, when all interfaces are probed, each interface
		 * is responsible of calling to power_on call back for
		 * comphy configuration.
		 */
		priv->lanes[i].phy = phy;
		priv->lanes[i].index = i;
		priv->lanes[i].mode = COMPHY_UNUSED;

		/* In case PCIe is in End point mode,
		 * link must be always kept alive vs the remote host,
		 * so we can't power off the SerDes as part of the
		 * initialization flow, thus end point PCIe ports must set this
		 * boolean to true in device tree, to avoid being powered off.
		 */
		if (skip_pcie_power_off) {
			/* If a port is set to PCIe and indicated in device
			 * tree to skip PCIe power off, than we assume it's
			 * an End point PCIe port that can't be powered off
			 */
			reg_data = readl(priv->comphy_regs +
					 COMMON_SELECTOR_PIPE_REG_OFFSET);
			reg_data = reg_data >>
				   (COMMON_SELECTOR_COMPHYN_FIELD_WIDTH * i);
			reg_data &= COMMON_SELECTOR_COMPHY_MASK;
			if (reg_data == COMMON_SELECTOR_PIPE_COMPHY_PCIE)
				priv->lanes[i].skip_pcie_power_off = true;
		}

		phy_set_drvdata(phy, &priv->lanes[i]);

		soc_info->comphy_ops->power_off(phy);
	}

	platform_set_drvdata(pdev, priv);

	phy_provider = devm_of_phy_provider_register(&pdev->dev,
						     mvebu_comphy_of_xlate);

	dev_dbg(priv->dev, "%s: Exit\n", __func__);

	return PTR_ERR_OR_ZERO(phy_provider);
}

static struct platform_driver mvebu_comphy_driver = {
	.probe	= mvebu_comphy_probe,
	.driver	= {
		.name		= "phy-mvebu-comphy",
		.owner		= THIS_MODULE,
		.of_match_table	= mvebu_comphy_of_match,
	 },
};
module_platform_driver(mvebu_comphy_driver);

MODULE_AUTHOR("Igal Liberman <igall@marvell.com>");
MODULE_DESCRIPTION("Marvell EBU COMPHY driver");
MODULE_LICENSE("GPL");

