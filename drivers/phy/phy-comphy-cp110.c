/*
 * Marvell cp110 comphy driver
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

#include <linux/arm-smccc.h>
#include <linux/delay.h>
#include <linux/io.h>
#include <linux/module.h>
#include <linux/of_device.h>
#include <linux/phy/phy.h>
#include <linux/platform_device.h>
#include <dt-bindings/phy/phy-comphy-mvebu.h>
#include <linux/mvebu-sample-at-reset.h>
#include <linux/of_address.h>
#include <linux/pci.h>

#include "phy-comphy-mvebu.h"
#include "phy-comphy-cp110.h"

#define MV_SIP_COMPHY_POWER_ON	0x82000001
#define MV_SIP_COMPHY_POWER_OFF	0x82000002
#define MV_SIP_COMPHY_PLL_LOCK	0x82000003
#define MV_SIP_COMPHY_XFI_TRAIN	0x82000004

#define COMPHY_FW_PCIE_FORMAT(pcie_width, mode)	\
			      (((pcie_width) << COMPHY_PCI_WIDTH_OFFSET) | mode)

static unsigned long comphy_smc(unsigned long function_id,
	 void __iomem *comphy_phys_addr, unsigned long lane, unsigned long mode)
{
	struct arm_smccc_res res;

	arm_smccc_smc(function_id, (unsigned long)comphy_phys_addr,
		      lane, mode, 0, 0, 0, 0, &res);
	return res.a0;
}

static int mvebu_cp110_comphy_power_on(struct phy *phy)
{
	struct mvebu_comphy *comphy = phy_get_drvdata(phy);
	struct mvebu_comphy_priv *priv = to_mvebu_comphy_priv(comphy);
	int mode = COMPHY_GET_MODE(comphy->mode);
	int err = 0;

	dev_dbg(priv->dev, "%s: Enter\n", __func__);

	spin_lock(&priv->lock);

	switch (mode) {
	case (COMPHY_SATA_MODE):
	case (COMPHY_SGMII_MODE):
	case (COMPHY_HS_SGMII_MODE):
	case (COMPHY_XFI_MODE):
	case (COMPHY_SFI_MODE):
	case (COMPHY_RXAUI_MODE):
	case (COMPHY_USB3H_MODE):
	case (COMPHY_USB3D_MODE):
		err = comphy_smc(MV_SIP_COMPHY_POWER_ON, priv->cp_phys,
				 comphy->index, comphy->mode);
		dev_dbg(priv->dev, "%s: smc returned %d\n", __func__, err);
		break;

	case (COMPHY_PCIE_MODE):
		err = comphy_smc(MV_SIP_COMPHY_POWER_ON, priv->cp_phys,
				 comphy->index,
				 COMPHY_FW_PCIE_FORMAT(comphy->pcie_width,
						       comphy->mode));
		dev_dbg(priv->dev, "%s: smc returned %d\n", __func__, err);
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

static int mvebu_cp110_comphy_power_off(struct phy *phy)
{
	struct mvebu_comphy *comphy = phy_get_drvdata(phy);
	struct mvebu_comphy_priv *priv = to_mvebu_comphy_priv(comphy);
	int err;

	dev_dbg(priv->dev, "%s: Enter\n", __func__);

	if (comphy->skip_pcie_power_off) {
		dev_dbg(priv->dev, "skip_pcie_power_off indicated, so avoid powering off lane\n");
		goto exit;
	}

	spin_lock(&priv->lock);

	err = comphy_smc(MV_SIP_COMPHY_POWER_OFF, priv->cp_phys, comphy->index,
					       priv->lanes[comphy->index].mode);
	spin_unlock(&priv->lock);

exit:
	dev_dbg(priv->dev, "%s: Exit\n", __func__);
	return err;
}

/*
 * This function allows to reset the digital synchronizers between
 * the MAC and the PHY, it is required when the MAC changes its state.
 */
static int mvebu_cp110_comphy_digital_reset(struct mvebu_comphy *comphy,
					     struct mvebu_comphy_priv *priv,
					     u32 command)
{
	int mode = COMPHY_GET_MODE(priv->lanes[comphy->index].mode);
	void __iomem *sd_ip_addr;
	u32 mask, data;

	sd_ip_addr = SD_ADDR(priv->comphy_pipe_regs, comphy->index);

	switch (mode) {
	case (COMPHY_SGMII_MODE):
	case (COMPHY_HS_SGMII_MODE):
	case (COMPHY_XFI_MODE):
	case (COMPHY_SFI_MODE):
	case (COMPHY_RXAUI_MODE):
		mask = SD_EXTERNAL_CONFIG1_RF_RESET_IN_MASK;
		data = ((command == COMPHY_COMMAND_DIGITAL_PWR_OFF) ? 0x0 : 0x1) <<
			SD_EXTERNAL_CONFIG1_RF_RESET_IN_OFFSET;
		reg_set(sd_ip_addr + SD_EXTERNAL_CONFIG1_REG, data, mask);
		break;
	default:
		dev_err(priv->dev, "comphy%d: COMPHY_COMMAND_DIGITAL_PWR_ON/OFF is not supported\n",
			comphy->index);
			return -EINVAL;
	}

	return 0;
}

static int mvebu_cp110_comphy_send_command(struct phy *phy, u32 command)
{
	struct mvebu_comphy *comphy = phy_get_drvdata(phy);
	struct mvebu_comphy_priv *priv = to_mvebu_comphy_priv(comphy);
	int ret = 0, pcie_width;

	switch (command) {
	case(COMPHY_COMMAND_DIGITAL_PWR_OFF):
	case(COMPHY_COMMAND_DIGITAL_PWR_ON):
		ret = mvebu_cp110_comphy_digital_reset(comphy, priv, command);
		break;
	/* The following commands are for PCIe width, currently the A8K supports
	 * width of X1, X2 and X4. The command is from PCIe host driver before
	 * comphy is initialized.
	 */
	case (COMPHY_COMMAND_PCIE_WIDTH_1):
		pcie_width = PCIE_LNK_X1;
	case (COMPHY_COMMAND_PCIE_WIDTH_2):
		if (command == COMPHY_COMMAND_PCIE_WIDTH_2)
			pcie_width = PCIE_LNK_X2;
	case (COMPHY_COMMAND_PCIE_WIDTH_4):
		if (command == COMPHY_COMMAND_PCIE_WIDTH_4)
			pcie_width = PCIE_LNK_X4;
	case (COMPHY_COMMAND_PCIE_WIDTH_UNSUPPORT):
		if (command == COMPHY_COMMAND_PCIE_WIDTH_UNSUPPORT)
			pcie_width = PCIE_LNK_WIDTH_UNKNOWN;

		if ((comphy->index >= COMPHY_LANE4 &&
		     pcie_width > PCIE_LNK_X1) ||
		    (pcie_width == PCIE_LNK_WIDTH_UNKNOWN))
			return -EIO;
		priv->lanes[comphy->index].pcie_width = pcie_width;
		break;
	case(COMPHY_COMMAND_SFI_RX_TRAINING):
		switch (COMPHY_GET_MODE(comphy->mode)) {
		case (COMPHY_XFI_MODE):
		case (COMPHY_SFI_MODE):
			ret = comphy_smc(MV_SIP_COMPHY_XFI_TRAIN, priv->cp_phys,
					 comphy->index, comphy->mode);
			if (ret < 0)
				dev_err(priv->dev, "%s: smc returned %d\n",
					__func__, ret);
		break;
			break;
		default:
			dev_err(priv->dev, "%s: RX training (command 0x%x) is supported only for SFI/XFI mode!\n",
				__func__, command);
			ret = -EINVAL;
		}
		break;
	default:
		dev_err(priv->dev, "%s: unsupported command (0x%x)\n",
			__func__, command);
		ret = -EINVAL;
	}

	return ret;
}

static int mvebu_cp110_comphy_is_pll_locked(struct phy *phy)
{

	struct mvebu_comphy *comphy = phy_get_drvdata(phy);
	struct mvebu_comphy_priv *priv = to_mvebu_comphy_priv(comphy);
	int ret;

	ret = comphy_smc(MV_SIP_COMPHY_PLL_LOCK, priv->cp_phys, comphy->index,
					       priv->lanes[comphy->index].mode);

	/* Firmware uses different error definiton for timeout - map it to the
	 * Linux one
	 */
	if (ret < 0)
		ret = -ETIMEDOUT;

	return ret;
}

static struct phy_ops cp110_comphy_ops = {
	.power_on	= mvebu_cp110_comphy_power_on,
	.power_off	= mvebu_cp110_comphy_power_off,
	.set_mode	= mvebu_comphy_set_mode,
	.get_mode	= mvebu_comphy_get_mode,
	.send_command	= mvebu_cp110_comphy_send_command,
	.is_pll_locked  = mvebu_cp110_comphy_is_pll_locked,
	.owner		= THIS_MODULE,
};

/* For CP-110 there are 2 Selector registers "PHY Selectors"
 * and "PIPE Selectors".
 *	- PIPE selector include USB and PCIe options.
 *	- PHY selector include the Ethernet and SATA options.
 *	  every Ethernet option has different options,
 *	  for example: serdes lane2 had option Eth_port_0 that include
 *	  (SGMII0, XAUI0, RXAUI0, XFI, SFI)
 */
const struct mvebu_comphy_soc_info cp110_comphy = {
	.num_of_lanes = 6,
	.functions = {
		/* Lane 0 */
		{COMPHY_UNUSED, COMPHY_SGMII1, COMPHY_HS_SGMII1,
		 COMPHY_SATA1, COMPHY_PCIE0},
		/* Lane 1 */
		{COMPHY_UNUSED, COMPHY_SGMII2, COMPHY_HS_SGMII2,
		 COMPHY_SATA0, COMPHY_USB3H0, COMPHY_USB3D0, COMPHY_PCIE0},
		/* Lane 2 */
		{COMPHY_UNUSED, COMPHY_SGMII0, COMPHY_HS_SGMII0, COMPHY_RXAUI0,
		 COMPHY_XFI, COMPHY_SFI, COMPHY_SATA0, COMPHY_USB3H0,
		 COMPHY_PCIE0},
		/* Lane 3 */
		{COMPHY_UNUSED, COMPHY_SGMII1, COMPHY_HS_SGMII1, COMPHY_RXAUI1,
		 COMPHY_SATA1, COMPHY_USB3H1, COMPHY_PCIE0},
		/* Lane 4 */
		{COMPHY_UNUSED, COMPHY_SGMII0, COMPHY_HS_SGMII0, COMPHY_SGMII2,
		 COMPHY_HS_SGMII2, COMPHY_RXAUI0, COMPHY_XFI, COMPHY_SFI,
		 COMPHY_USB3H1, COMPHY_USB3D0, COMPHY_PCIE1},
		/* Lane 5 */
		{COMPHY_UNUSED, COMPHY_RXAUI1, COMPHY_SGMII2, COMPHY_HS_SGMII2,
		 COMPHY_SATA1, COMPHY_PCIE2},
	},
	.comphy_ops = &cp110_comphy_ops,
};

