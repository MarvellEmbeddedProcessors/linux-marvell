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

#define COMPHY_FW_PCIE_FORMAT(pcie_width, mode)	\
			      (((pcie_width) << COMPHY_PCI_WIDTH_OFFSET) | mode)

/* PIPE selector configures for PCIe, USB 3.0 Host, and USB 3.0 Device mode */
void mvebu_cp110_comphy_set_pipe_selector(struct mvebu_comphy_priv *priv,
					  struct mvebu_comphy *comphy)
{
	u32 reg;
	u32 shift = COMMON_SELECTOR_COMPHYN_FIELD_WIDTH * comphy->index;
	int mode = COMPHY_GET_MODE(priv->lanes[comphy->index].mode);
	u32 mask = COMMON_SELECTOR_COMPHY_MASK << shift;
	u32 pipe_sel = 0x0;

	reg = readl(priv->comphy_regs + COMMON_SELECTOR_PIPE_REG_OFFSET);
	reg &= ~mask;

	switch (mode) {
	case (COMPHY_PCIE_MODE):
		/* For lanes support PCIE, selector value are all same */
		pipe_sel = COMMON_SELECTOR_PIPE_COMPHY_PCIE;
		break;

	case (COMPHY_USB3H_MODE):
		/* Only lane 1-4 support USB host, selector value is same */
		if (comphy->index == COMPHY_LANE0 ||
		    comphy->index == COMPHY_LANE5)
			dev_err(priv->dev, "COMPHY[%d] mode[%d] is invalid\n",
				comphy->index, mode);
		else
			pipe_sel = COMMON_SELECTOR_PIPE_COMPHY_USBH;
		break;

	case (COMPHY_USB3D_MODE):
		/* Lane 1 and 4 support USB device, selector value is same */
		if (comphy->index == COMPHY_LANE1 ||
		    comphy->index == COMPHY_LANE4)
			pipe_sel = COMMON_SELECTOR_PIPE_COMPHY_USBD;
		else
			dev_err(priv->dev, "COMPHY[%d] mode[%d] is invalid\n",
				comphy->index, mode);
		break;

	default:
		dev_err(priv->dev, "COMPHY[%d] mode[%d] is invalid\n",
			comphy->index, mode);
		break;
	}

	writel(reg | (pipe_sel << shift),
	       priv->comphy_regs + COMMON_SELECTOR_PIPE_REG_OFFSET);
}

static int mvebu_cp110_comphy_usb3_power_on(struct mvebu_comphy_priv *priv,
					    struct mvebu_comphy *comphy)
{
	void __iomem *hpipe_addr, *comphy_addr, *addr;
	u32 mask, data;
	int ret = 0;

	dev_dbg(priv->dev, "%s: Enter\n", __func__);

	/* Configure PIPE selector for USB3 */
	mvebu_cp110_comphy_set_pipe_selector(priv, comphy);

	hpipe_addr = HPIPE_ADDR(priv->comphy_pipe_regs, comphy->index);
	comphy_addr = COMPHY_ADDR(priv->comphy_regs, comphy->index);

	dev_dbg(priv->dev, "stage: RFU configurations - hard reset comphy\n");
	/* RFU configurations - hard reset comphy */
	mask = COMMON_PHY_CFG1_PWR_UP_MASK;
	data = 0x1 << COMMON_PHY_CFG1_PWR_UP_OFFSET;
	mask |= COMMON_PHY_CFG1_PIPE_SELECT_MASK;
	data |= 0x1 << COMMON_PHY_CFG1_PIPE_SELECT_OFFSET;
	mask |= COMMON_PHY_CFG1_PWR_ON_RESET_MASK;
	data |= 0x0 << COMMON_PHY_CFG1_PWR_ON_RESET_OFFSET;
	mask |= COMMON_PHY_CFG1_CORE_RSTN_MASK;
	data |= 0x0 << COMMON_PHY_CFG1_CORE_RSTN_OFFSET;
	mask |= COMMON_PHY_PHY_MODE_MASK;
	data |= 0x1 << COMMON_PHY_PHY_MODE_OFFSET;
	reg_set(comphy_addr + COMMON_PHY_CFG1_REG, data, mask);

	/* release from hard reset */
	mask = COMMON_PHY_CFG1_PWR_ON_RESET_MASK;
	data = 0x1 << COMMON_PHY_CFG1_PWR_ON_RESET_OFFSET;
	mask |= COMMON_PHY_CFG1_CORE_RSTN_MASK;
	data |= 0x1 << COMMON_PHY_CFG1_CORE_RSTN_OFFSET;
	reg_set(comphy_addr + COMMON_PHY_CFG1_REG, data, mask);

	/* Wait 1ms - until band gap and ref clock ready */
	mdelay(1);

	/* Start comphy Configuration */
	dev_dbg(priv->dev, "stage: Comphy configuration\n");
	/* Set PIPE soft reset */
	mask = HPIPE_RST_CLK_CTRL_PIPE_RST_MASK;
	data = 0x1 << HPIPE_RST_CLK_CTRL_PIPE_RST_OFFSET;
	/* Set PHY datapath width mode for V0 */
	mask |= HPIPE_RST_CLK_CTRL_FIXED_PCLK_MASK;
	data |= 0x0 << HPIPE_RST_CLK_CTRL_FIXED_PCLK_OFFSET;
	/* Set Data bus width USB mode for V0 */
	mask |= HPIPE_RST_CLK_CTRL_PIPE_WIDTH_MASK;
	data |= 0x0 << HPIPE_RST_CLK_CTRL_PIPE_WIDTH_OFFSET;
	/* Set CORE_CLK output frequency for 250Mhz */
	mask |= HPIPE_RST_CLK_CTRL_CORE_FREQ_SEL_MASK;
	data |= 0x0 << HPIPE_RST_CLK_CTRL_CORE_FREQ_SEL_OFFSET;
	reg_set(hpipe_addr + HPIPE_RST_CLK_CTRL_REG, data, mask);
	/* Set PLL ready delay for 0x2 */
	reg_set(hpipe_addr + HPIPE_CLK_SRC_LO_REG,
		0x2 << HPIPE_CLK_SRC_LO_PLL_RDY_DL_OFFSET,
		HPIPE_CLK_SRC_LO_PLL_RDY_DL_MASK);
	/* Set reference clock to come from group 1 - 25Mhz */
	reg_set(hpipe_addr + HPIPE_MISC_REG,
		0x0 << HPIPE_MISC_REFCLK_SEL_OFFSET,
		HPIPE_MISC_REFCLK_SEL_MASK);
	/* Set reference frequcency select - 0x2 */
	mask = HPIPE_PWR_PLL_REF_FREQ_MASK;
	data = 0x2 << HPIPE_PWR_PLL_REF_FREQ_OFFSET;
	/* Set PHY mode to USB - 0x5 */
	mask |= HPIPE_PWR_PLL_PHY_MODE_MASK;
	data |= 0x5 << HPIPE_PWR_PLL_PHY_MODE_OFFSET;
	reg_set(hpipe_addr + HPIPE_PWR_PLL_REG, data, mask);
	/* Set the amount of time spent in the LoZ state - set for 0x7 */
	reg_set(hpipe_addr + HPIPE_GLOBAL_PM_CTRL,
		0x7 << HPIPE_GLOBAL_PM_RXDLOZ_WAIT_OFFSET,
		HPIPE_GLOBAL_PM_RXDLOZ_WAIT_MASK);
	/* Set max PHY generation setting - 5Gbps */
	reg_set(hpipe_addr + HPIPE_INTERFACE_REG,
		0x1 << HPIPE_INTERFACE_GEN_MAX_OFFSET,
		HPIPE_INTERFACE_GEN_MAX_MASK);
	/* Set select data width 20Bit (SEL_BITS[2:0]) */
	reg_set(hpipe_addr + HPIPE_LOOPBACK_REG,
		0x1 << HPIPE_LOOPBACK_SEL_OFFSET,
		HPIPE_LOOPBACK_SEL_MASK);
	/* select de-emphasize 3.5db */
	reg_set(hpipe_addr + HPIPE_LANE_CONFIG0_REG,
		0x1 << HPIPE_LANE_CONFIG0_TXDEEMPH0_OFFSET,
		HPIPE_LANE_CONFIG0_TXDEEMPH0_MASK);
	/* override tx margining from the MAC */
	reg_set(hpipe_addr + HPIPE_TST_MODE_CTRL_REG,
		0x1 << HPIPE_TST_MODE_CTRL_MODE_MARGIN_OFFSET,
		HPIPE_TST_MODE_CTRL_MODE_MARGIN_MASK);

	/* Start analog parameters from ETP(HW) */
	dev_dbg(priv->dev, "stage: Analog parameters from ETP(HW)\n");
	/* Set Pin DFE_PAT_DIS -> Bit[1]: PIN_DFE_PAT_DIS = 0x0 */
	mask = HPIPE_LANE_CFG4_DFE_CTRL_MASK;
	data = 0x1 << HPIPE_LANE_CFG4_DFE_CTRL_OFFSET;
	/* Set Override PHY DFE control pins for 0x1 */
	mask |= HPIPE_LANE_CFG4_DFE_OVER_MASK;
	data |= 0x1 << HPIPE_LANE_CFG4_DFE_OVER_OFFSET;
	/* Set Spread Spectrum Clock Enable fot 0x1 */
	mask |= HPIPE_LANE_CFG4_SSC_CTRL_MASK;
	data |= 0x1 << HPIPE_LANE_CFG4_SSC_CTRL_OFFSET;
	reg_set(hpipe_addr + HPIPE_LANE_CFG4_REG, data, mask);
	/* Confifure SSC amplitude */
	mask = HPIPE_G2_TX_SSC_AMP_MASK;
	data = 0x1f << HPIPE_G2_TX_SSC_AMP_OFFSET;
	reg_set(hpipe_addr + HPIPE_G2_SET_2_REG, data, mask);
	/* End of analog parameters */

	dev_dbg(priv->dev, "stage: Comphy power up\n");
	/* Release from PIPE soft reset */
	reg_set(hpipe_addr + HPIPE_RST_CLK_CTRL_REG,
		0x0 << HPIPE_RST_CLK_CTRL_PIPE_RST_OFFSET,
		HPIPE_RST_CLK_CTRL_PIPE_RST_MASK);

	/* wait 15ms - for comphy calibration done */
	dev_dbg(priv->dev, "stage: Check PLL\n");
	/* Read lane status */
	addr = hpipe_addr + HPIPE_LANE_STATUS1_REG;
	data = HPIPE_LANE_STATUS1_PCLK_EN_MASK;
	mask = data;
	data = polling_with_timeout(addr, data, mask, 15000, REG_32BIT);
	if (data != 0) {
		dev_dbg(priv->dev, "Read from reg = %p - value = 0x%x\n",
			hpipe_addr + HPIPE_LANE_STATUS1_REG, data);
		dev_err(priv->dev, "HPIPE_LANE_STATUS1_PCLK_EN_MASK is 0\n");
		ret = -ETIMEDOUT;
	}

	dev_dbg(priv->dev, "%s: Exit\n", __func__);

	return ret;
}

/* This function performs RX training for one Feed Forward Equalization (FFE)
 * value.
 * The RX traiing result is stored in 'Saved DFE values Register' (SAV_F0D).
 *
 * Return '0' on success, error code in  a case of failure.
 */
static int mvebu_cp110_comphy_test_single_ffe(struct mvebu_comphy_priv *priv,
					      struct mvebu_comphy *comphy,
					      u32 ffe, u32 *result)
{
	u32 mask, data, timeout;
	void __iomem *hpipe_addr = HPIPE_ADDR(priv->comphy_pipe_regs, comphy->index);
	void __iomem *sd_ip_addr = SD_ADDR(priv->comphy_pipe_regs, comphy->index);

	/* Configure PRBS counters */
	mask = HPIPE_PHY_TEST_PATTERN_SEL_MASK;
	data = 0xe << HPIPE_PHY_TEST_PATTERN_SEL_OFFSET;
	reg_set(hpipe_addr + HPIPE_PHY_TEST_CONTROL_REG, data, mask);

	mask = HPIPE_PHY_TEST_DATA_MASK;
	data = 0x64 << HPIPE_PHY_TEST_DATA_OFFSET;
	reg_set(hpipe_addr + HPIPE_PHY_TEST_DATA_REG, data, mask);

	mask = HPIPE_PHY_TEST_EN_MASK;
	data = 0x1 << HPIPE_PHY_TEST_EN_OFFSET;
	reg_set(hpipe_addr + HPIPE_PHY_TEST_CONTROL_REG, data, mask);

	mdelay(50);

	/* Set the FFE value */
	mask = HPIPE_G1_SETTINGS_3_G1_FFE_RES_SEL_MASK;
	data = ffe << HPIPE_G1_SETTINGS_3_G1_FFE_RES_SEL_OFFSET;
	reg_set(hpipe_addr + HPIPE_G1_SETTINGS_3_REG, data, mask);

	/* Start RX training */
	mask = SD_EXTERNAL_STATUS_START_RX_TRAINING_MASK;
	data = 1 << SD_EXTERNAL_STATUS_START_RX_TRAINING_OFFSET;
	reg_set(sd_ip_addr + SD_EXTERNAL_STATUS_REG, data, mask);

	/* Check the result of RX training */
	timeout = RX_TRAINING_TIMEOUT;
	while (timeout) {
		data = readl(sd_ip_addr + SD_EXTERNAL_STATAUS1_REG);
		if (data & SD_EXTERNAL_STATAUS1_REG_RX_TRAIN_COMP_MASK)
			break;
		mdelay(1);
		timeout--;
	}

	if (timeout == 0)
		return -ETIMEDOUT;

	if (data & SD_EXTERNAL_STATAUS1_REG_RX_TRAIN_FAILED_MASK)
		return -EINVAL;

	/* Stop RX training */
	mask = SD_EXTERNAL_STATUS_START_RX_TRAINING_MASK;
	data = 0 << SD_EXTERNAL_STATUS_START_RX_TRAINING_OFFSET;
	reg_set(sd_ip_addr + SD_EXTERNAL_STATUS_REG, data, mask);

	/* Read the result */
	data = readl(hpipe_addr + HPIPE_SAVED_DFE_VALUES_REG);
	data &= HPIPE_SAVED_DFE_VALUES_SAV_F0D_MASK;
	data >>= HPIPE_SAVED_DFE_VALUES_SAV_F0D_OFFSET;
	*result = data;

	mask = HPIPE_PHY_TEST_RESET_MASK;
	data = 0x1 << HPIPE_PHY_TEST_RESET_OFFSET;
	mask |= HPIPE_PHY_TEST_EN_MASK;
	data |= 0x0 << HPIPE_PHY_TEST_EN_OFFSET;
	reg_set(hpipe_addr + HPIPE_PHY_TEST_CONTROL_REG, data, mask);

	mask = HPIPE_PHY_TEST_RESET_MASK;
	data = 0x0 << HPIPE_PHY_TEST_RESET_OFFSET;
	reg_set(hpipe_addr + HPIPE_PHY_TEST_CONTROL_REG, data, mask);

	return 0;
}

/* This function runs complete RX training sequence:
 *	- Run RX training for all possible Feed Forward Equalization values
 *	- Choose the FFE which gives the best result.
 *	- Run RX training again with the best result.
 *
 * Return '0' on success, error code in  a case of failure.
 */
static int mvebu_cp110_comphy_xfi_rx_training(struct mvebu_comphy_priv *priv,
					      struct mvebu_comphy *comphy)
{
	u32 mask, data, max_rx_train = 0, max_rx_train_index = 0;
	void __iomem *hpipe_addr = HPIPE_ADDR(priv->comphy_pipe_regs, comphy->index);
	u32 rx_train_result;
	int ret, i;

	dev_dbg(priv->dev, "%s: Enter\n", __func__);

	/* Configure SQ threshold and CDR lock */
	mask = HPIPE_SQUELCH_THRESH_IN_MASK;
	data = 0xc << HPIPE_SQUELCH_THRESH_IN_OFFSET;
	reg_set(hpipe_addr + HPIPE_SQUELCH_FFE_SETTING_REG, data, mask);

	mask = HPIPE_SQ_DEGLITCH_WIDTH_P_MASK;
	data = 0xf << HPIPE_SQ_DEGLITCH_WIDTH_P_OFFSET;
	mask |= HPIPE_SQ_DEGLITCH_WIDTH_N_MASK;
	data |= 0xf << HPIPE_SQ_DEGLITCH_WIDTH_N_OFFSET;
	mask |= HPIPE_SQ_DEGLITCH_EN_MASK;
	data |= 0x1 << HPIPE_SQ_DEGLITCH_EN_OFFSET;
	reg_set(hpipe_addr + HPIPE_SQ_GLITCH_FILTER_CTRL, data, mask);

	mask = HPIPE_CDR_LOCK_DET_EN_MASK;
	data = 0x1 << HPIPE_CDR_LOCK_DET_EN_OFFSET;
	reg_set(hpipe_addr + HPIPE_LOOPBACK_REG, data, mask);

	udelay(100);

	/* Determine if we have a cable attached to this comphy, if not,
	 * we can't perform RX training.
	 */
	data = readl(hpipe_addr + HPIPE_SQUELCH_FFE_SETTING_REG);
	if (data & HPIPE_SQUELCH_DETECTED_MASK) {
		dev_err(priv->dev, "Squelsh is not detected, can't perform RX training\n");
		return -EINVAL;
	}

	data = readl(hpipe_addr + HPIPE_LOOPBACK_REG);
	if (!(data & HPIPE_CDR_LOCK_MASK)) {
		dev_err(priv->dev, "CDR is not locked, can't perform RX training\n");
		return -EINVAL;
	}

	/* Do preparations for RX training */
	mask = HPIPE_DFE_RES_FORCE_MASK;
	data = 0x0 << HPIPE_DFE_RES_FORCE_OFFSET;
	reg_set(hpipe_addr + HPIPE_DFE_REG0, data, mask);

	mask = HPIPE_G1_SETTINGS_3_G1_FFE_CAP_SEL_MASK;
	data = 0xf << HPIPE_G1_SETTINGS_3_G1_FFE_CAP_SEL_OFFSET;
	mask |= HPIPE_G1_SETTINGS_3_G1_FFE_SETTING_FORCE_MASK;
	data |= 1 << HPIPE_G1_SETTINGS_3_G1_FFE_SETTING_FORCE_OFFSET;
	reg_set(hpipe_addr + HPIPE_G1_SETTINGS_3_REG, data, mask);

	/* Perform RX training for all possible FFE (Feed Forward
	 * Equalization, possible values are 0-7).
	 * We update the best value reached and the FFE which gave this value.
	 */
	for (i = 0; i < MAX_NUM_OF_FFE; i++) {
		rx_train_result = 0;
		ret = mvebu_cp110_comphy_test_single_ffe(priv, comphy, i,
							 &rx_train_result);

		if ((!ret) && (rx_train_result > max_rx_train)) {
			max_rx_train = rx_train_result;
			max_rx_train_index = i;
		}
	}

	/* If we were able to determine which FFE gives the best value,
	 * now we need to set it and run RX training again (only for this
	 * FFE).
	 */
	if (max_rx_train) {
		ret = mvebu_cp110_comphy_test_single_ffe(priv, comphy,
							 max_rx_train_index,
							 &rx_train_result);
	} else {
		dev_err(priv->dev, "RX Training failed for comphy%d\n", comphy->index);
		ret = -EINVAL;
	}

	dev_dbg(priv->dev, "%s: Exit\n", __func__);

	return ret;
}

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
		err = comphy_smc(MV_SIP_COMPHY_POWER_ON, priv->cp_phys,
				 comphy->index, comphy->mode);
		dev_dbg(priv->dev, "%s: smc returned %d\n", __func__, err);
		break;

	case (COMPHY_USB3H_MODE):
	case (COMPHY_USB3D_MODE):
		err = mvebu_cp110_comphy_usb3_power_on(priv, comphy);
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
		switch (COMPHY_GET_MODE(priv->lanes[comphy->index].mode)) {
		case (COMPHY_XFI_MODE):
		case (COMPHY_SFI_MODE):
			ret = mvebu_cp110_comphy_xfi_rx_training(priv, comphy);
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

