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

/* Clear PHY selector - avoid collision with prior u-boot configuration */
static void mvebu_cp110_comphy_clr_phy_selector(struct mvebu_comphy_priv *priv,
						struct mvebu_comphy *comphy)
{
	u32 reg, mask, field;
	u32 comphy_offset = COMMON_SELECTOR_COMPHYN_FIELD_WIDTH * comphy->index;

	mask = COMMON_SELECTOR_COMPHY_MASK << comphy_offset;
	reg = readl(priv->comphy_regs + COMMON_SELECTOR_PHY_REG_OFFSET);
	field = reg & mask;

	/* Clear comphy selector - if it was set by u-boot.
	 * (might be that this comphy was configured as PCIe/USB,
	 * in such case, no need to clear comphy selector because PCIe/USB
	 * are controlled by hpipe selector.
	 */
	if (field) {
		reg &= ~mask;
		writel(reg, priv->comphy_regs + COMMON_SELECTOR_PHY_REG_OFFSET);
	}
}

/* PHY selector configures SATA and Network modes */
static void mvebu_cp110_comphy_set_phy_selector(struct mvebu_comphy_priv *priv,
						struct mvebu_comphy *comphy)
{
	u32 reg, mask;
	u32 comphy_offset = COMMON_SELECTOR_COMPHYN_FIELD_WIDTH * comphy->index;
	int mode;

	/* Comphy mode (compound of the IO mode and id) is stored during
	 * the execution of mvebu_comphy_of_xlate.
	 * Here, only the IO mode is required to distinguish between SATA and
	 * network modes.
	 */
	mode = COMPHY_GET_MODE(priv->lanes[comphy->index].mode);

	mask = COMMON_SELECTOR_COMPHY_MASK << comphy_offset;
	reg = readl(priv->comphy_regs + COMMON_SELECTOR_PHY_REG_OFFSET);
	reg &= ~mask;

	/* SATA port 0/1 require the same configuration */
	if (mode == COMPHY_SATA_MODE) {
		/* SATA selector values is always 4 */
		reg |= COMMON_SELECTOR_COMPHYN_SATA << comphy_offset;
	} else {
		switch (comphy->index) {
		case(0):
		case(1):
		case(2):
			/* For comphy 0,1, and 2:
			 *	Network selector value is always 1.
			 */
			reg |= COMMON_SELECTOR_COMPHY0_1_2_NETWORK << comphy_offset;
			break;
		case(3):
			/* For comphy 3:
			 * 0x1 = RXAUI_Lane1
			 * 0x2 = SGMII/HS-SGMII Port1
			 */
			if (mode == COMPHY_RXAUI_MODE)
				reg |= COMMON_SELECTOR_COMPHY3_RXAUI << comphy_offset;
			else
				reg |= COMMON_SELECTOR_COMPHY3_SGMII << comphy_offset;
			break;
		case(4):
			 /* For comphy 4:
			  * 0x1 = SGMII/HS-SGMII Port1
			  * 0x2 = SGMII/HS-SGMII Port0: XFI/SFI, RXAUI_Lane0
			  *
			  * We want to check if SGMII1/HS_SGMII1 is the requested mode in order to
			  * determine which value should be set (all other modes use the same value)
			  * so we need to strip the mode, and check the ID because we might handle
			  * SGMII0/HS_SGMII0 too.
			  */
			if ((mode == COMPHY_SGMII_MODE || mode == COMPHY_HS_SGMII_MODE) &&
			    COMPHY_GET_ID(priv->lanes[comphy->index].mode) == 1)
				reg |= COMMON_SELECTOR_COMPHY4_SGMII1 << comphy_offset;
			else
				reg |= COMMON_SELECTOR_COMPHY4_ALL_OTHERS << comphy_offset;
			break;
		case(5):
			/* For comphy 5:
			 * 0x1 = SGMII/HS-SGMII Port2
			 * 0x2 = RXAUI Lane1
			 */
			if (mode == COMPHY_RXAUI_MODE)
				reg |= COMMON_SELECTOR_COMPHY5_RXAUI << comphy_offset;
			else
				reg |= COMMON_SELECTOR_COMPHY5_SGMII << comphy_offset;
			break;
		}
	}

	writel(reg, priv->comphy_regs + COMMON_SELECTOR_PHY_REG_OFFSET);

}

/* Clear PIPE selector - avoid collision with prior u-boot configuration */
void mvebu_cp110_comphy_clr_pipe_selector(struct mvebu_comphy_priv *priv,
					  struct mvebu_comphy *comphy)
{
	u32 reg, mask, field;
	u32 comphy_offset = COMMON_SELECTOR_COMPHYN_FIELD_WIDTH * comphy->index;

	mask = COMMON_SELECTOR_COMPHY_MASK << comphy_offset;
	reg = readl(priv->comphy_regs + COMMON_SELECTOR_PIPE_REG_OFFSET);
	field = reg & mask;

	if (field) {
		reg &= ~mask;
		writel(reg,
		       priv->comphy_regs + COMMON_SELECTOR_PIPE_REG_OFFSET);
	}
}

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

static int mvebu_cp110_comphy_sata_power_on(struct mvebu_comphy_priv *priv,
					    struct mvebu_comphy *comphy)
{
	void __iomem *hpipe_addr, *sd_ip_addr, *comphy_addr;
	u32 mask, data;
	int ret = 0;

	dev_dbg(priv->dev, "%s: Enter\n", __func__);

	/* configure phy selector for SATA */
	mvebu_cp110_comphy_set_phy_selector(priv, comphy);

	hpipe_addr = HPIPE_ADDR(priv->comphy_pipe_regs, comphy->index);
	sd_ip_addr = SD_ADDR(priv->comphy_pipe_regs, comphy->index);
	comphy_addr = COMPHY_ADDR(priv->comphy_regs, comphy->index);

	dev_dbg(priv->dev, "stage: RFU configurations - hard reset comphy\n");
	/* RFU configurations - hard reset comphy */
	mask = COMMON_PHY_CFG1_PWR_UP_MASK;
	data = 0x1 << COMMON_PHY_CFG1_PWR_UP_OFFSET;
	mask |= COMMON_PHY_CFG1_PIPE_SELECT_MASK;
	data |= 0x0 << COMMON_PHY_CFG1_PIPE_SELECT_OFFSET;
	mask |= COMMON_PHY_CFG1_PWR_ON_RESET_MASK;
	data |= 0x0 << COMMON_PHY_CFG1_PWR_ON_RESET_OFFSET;
	mask |= COMMON_PHY_CFG1_CORE_RSTN_MASK;
	data |= 0x0 << COMMON_PHY_CFG1_CORE_RSTN_OFFSET;
	reg_set(comphy_addr + COMMON_PHY_CFG1_REG, data, mask);

	/* Set select data  width 40Bit - SATA mode only */
	reg_set(comphy_addr + COMMON_PHY_CFG6_REG,
		0x1 << COMMON_PHY_CFG6_IF_40_SEL_OFFSET, COMMON_PHY_CFG6_IF_40_SEL_MASK);

	/* release from hard reset in SD external */
	mask = SD_EXTERNAL_CONFIG1_RESET_IN_MASK;
	data = 0x1 << SD_EXTERNAL_CONFIG1_RESET_IN_OFFSET;
	mask |= SD_EXTERNAL_CONFIG1_RESET_CORE_MASK;
	data |= 0x1 << SD_EXTERNAL_CONFIG1_RESET_CORE_OFFSET;
	reg_set(sd_ip_addr + SD_EXTERNAL_CONFIG1_REG, data, mask);

	/* Wait 1ms - until band gap and ref clock ready */
	mdelay(1);

	dev_dbg(priv->dev, "stage: Comphy configuration\n");
	/* Start comphy Configuration */
	/* Set reference clock to comes from group 1 - choose 25Mhz */
	reg_set(hpipe_addr + HPIPE_MISC_REG,
		0x0 << HPIPE_MISC_REFCLK_SEL_OFFSET, HPIPE_MISC_REFCLK_SEL_MASK);
	/* Reference frequency select set 1 (for SATA = 25Mhz) */
	mask = HPIPE_PWR_PLL_REF_FREQ_MASK;
	data = 0x1 << HPIPE_PWR_PLL_REF_FREQ_OFFSET;
	/* PHY mode select (set SATA = 0x0 */
	mask |= HPIPE_PWR_PLL_PHY_MODE_MASK;
	data |= 0x0 << HPIPE_PWR_PLL_PHY_MODE_OFFSET;
	reg_set(hpipe_addr + HPIPE_PWR_PLL_REG, data, mask);
	/* Set max PHY generation setting - 6Gbps */
	reg_set(hpipe_addr + HPIPE_INTERFACE_REG,
		0x2 << HPIPE_INTERFACE_GEN_MAX_OFFSET, HPIPE_INTERFACE_GEN_MAX_MASK);
	/* Set select data  width 40Bit (SEL_BITS[2:0]) */
	reg_set(hpipe_addr + HPIPE_LOOPBACK_REG,
		0x2 << HPIPE_LOOPBACK_SEL_OFFSET, HPIPE_LOOPBACK_SEL_MASK);

	dev_dbg(priv->dev, "stage: Analog parameters from ETP(HW)\n");
	/* G1 settings */
	mask = HPIPE_G1_SET_1_G1_RX_SELMUPI_MASK;
	data = 0x0 << HPIPE_G1_SET_1_G1_RX_SELMUPI_OFFSET;
	mask |= HPIPE_G1_SET_1_G1_RX_SELMUPP_MASK;
	data |= 0x1 << HPIPE_G1_SET_1_G1_RX_SELMUPP_OFFSET;
	mask |= HPIPE_G1_SET_1_G1_RX_SELMUFI_MASK;
	data |= 0x0 << HPIPE_G1_SET_1_G1_RX_SELMUFI_OFFSET;
	mask |= HPIPE_G1_SET_1_G1_RX_SELMUFF_MASK;
	data |= 0x3 << HPIPE_G1_SET_1_G1_RX_SELMUFF_OFFSET;
	mask |= HPIPE_G1_SET_1_G1_RX_DIGCK_DIV_MASK;
	data |= 0x1 << HPIPE_G1_SET_1_G1_RX_DIGCK_DIV_OFFSET;
	reg_set(hpipe_addr + HPIPE_G1_SET_1_REG, data, mask);

	mask = HPIPE_G1_SETTINGS_3_G1_FFE_CAP_SEL_MASK;
	data = 0xf << HPIPE_G1_SETTINGS_3_G1_FFE_CAP_SEL_OFFSET;
	mask |= HPIPE_G1_SETTINGS_3_G1_FFE_RES_SEL_MASK;
	data |= 0x2 << HPIPE_G1_SETTINGS_3_G1_FFE_RES_SEL_OFFSET;
	mask |= HPIPE_G1_SETTINGS_3_G1_FFE_SETTING_FORCE_MASK;
	data |= 0x1 << HPIPE_G1_SETTINGS_3_G1_FFE_SETTING_FORCE_OFFSET;
	mask |= HPIPE_G1_SETTINGS_3_G1_FFE_DEG_RES_LEVEL_MASK;
	data |= 0x1 << HPIPE_G1_SETTINGS_3_G1_FFE_DEG_RES_LEVEL_OFFSET;
	mask |= HPIPE_G1_SETTINGS_3_G1_FFE_LOAD_RES_LEVEL_MASK;
	data |= 0x1 << HPIPE_G1_SETTINGS_3_G1_FFE_LOAD_RES_LEVEL_OFFSET;
	reg_set(hpipe_addr + HPIPE_G1_SETTINGS_3_REG, data, mask);

	/* G2 settings */
	mask = HPIPE_G2_SET_1_G2_RX_SELMUPI_MASK;
	data = 0x0 << HPIPE_G2_SET_1_G2_RX_SELMUPI_OFFSET;
	mask |= HPIPE_G2_SET_1_G2_RX_SELMUPP_MASK;
	data |= 0x1 << HPIPE_G2_SET_1_G2_RX_SELMUPP_OFFSET;
	mask |= HPIPE_G2_SET_1_G2_RX_SELMUFI_MASK;
	data |= 0x0 << HPIPE_G2_SET_1_G2_RX_SELMUFI_OFFSET;
	mask |= HPIPE_G2_SET_1_G2_RX_SELMUFF_MASK;
	data |= 0x3 << HPIPE_G2_SET_1_G2_RX_SELMUFF_OFFSET;
	mask |= HPIPE_G2_SET_1_G2_RX_DIGCK_DIV_MASK;
	data |= 0x1 << HPIPE_G2_SET_1_G2_RX_DIGCK_DIV_OFFSET;
	reg_set(hpipe_addr + HPIPE_G2_SET_1_REG, data, mask);

	/* G3 settings */
	mask = HPIPE_G3_SET_1_G3_RX_SELMUPI_MASK;
	data = 0x2 << HPIPE_G3_SET_1_G3_RX_SELMUPI_OFFSET;
	mask |= HPIPE_G3_SET_1_G3_RX_SELMUPF_MASK;
	data |= 0x2 << HPIPE_G3_SET_1_G3_RX_SELMUPF_OFFSET;
	mask |= HPIPE_G3_SET_1_G3_RX_SELMUFI_MASK;
	data |= 0x3 << HPIPE_G3_SET_1_G3_RX_SELMUFI_OFFSET;
	mask |= HPIPE_G3_SET_1_G3_RX_SELMUFF_MASK;
	data |= 0x3 << HPIPE_G3_SET_1_G3_RX_SELMUFF_OFFSET;
	mask |= HPIPE_G3_SET_1_G3_RX_DFE_EN_MASK;
	data |= 0x1 << HPIPE_G3_SET_1_G3_RX_DFE_EN_OFFSET;
	mask |= HPIPE_G3_SET_1_G3_RX_DIGCK_DIV_MASK;
	data |= 0x2 << HPIPE_G3_SET_1_G3_RX_DIGCK_DIV_OFFSET;
	mask |= HPIPE_G3_SET_1_G3_SAMPLER_INPAIRX2_EN_MASK;
	data |= 0x0 << HPIPE_G3_SET_1_G3_SAMPLER_INPAIRX2_EN_OFFSET;
	reg_set(hpipe_addr + HPIPE_G3_SET_1_REG, data, mask);

	/* DTL Control */
	mask = HPIPE_PWR_CTR_DTL_SQ_DET_EN_MASK;
	data = 0x1 << HPIPE_PWR_CTR_DTL_SQ_DET_EN_OFFSET;
	mask |= HPIPE_PWR_CTR_DTL_SQ_PLOOP_EN_MASK;
	data |= 0x1 << HPIPE_PWR_CTR_DTL_SQ_PLOOP_EN_OFFSET;
	mask |= HPIPE_PWR_CTR_DTL_FLOOP_EN_MASK;
	data |= 0x1 << HPIPE_PWR_CTR_DTL_FLOOP_EN_OFFSET;
	mask |= HPIPE_PWR_CTR_DTL_CLAMPING_SEL_MASK;
	data |= 0x1 << HPIPE_PWR_CTR_DTL_CLAMPING_SEL_OFFSET;
	mask |= HPIPE_PWR_CTR_DTL_INTPCLK_DIV_FORCE_MASK;
	data |= 0x1 << HPIPE_PWR_CTR_DTL_INTPCLK_DIV_FORCE_OFFSET;
	mask |= HPIPE_PWR_CTR_DTL_CLK_MODE_MASK;
	data |= 0x1 << HPIPE_PWR_CTR_DTL_CLK_MODE_OFFSET;
	mask |= HPIPE_PWR_CTR_DTL_CLK_MODE_FORCE_MASK;
	data |= 0x1 << HPIPE_PWR_CTR_DTL_CLK_MODE_FORCE_OFFSET;
	reg_set(hpipe_addr + HPIPE_PWR_CTR_DTL_REG, data, mask);

	/* Trigger sampler enable pulse */
	mask = HPIPE_SMAPLER_MASK;
	data = 0x1 << HPIPE_SMAPLER_OFFSET;
	reg_set(hpipe_addr + HPIPE_SAMPLER_N_PROC_CALIB_CTRL_REG, data, mask);
	mask = HPIPE_SMAPLER_MASK;
	data = 0x0 << HPIPE_SMAPLER_OFFSET;
	reg_set(hpipe_addr + HPIPE_SAMPLER_N_PROC_CALIB_CTRL_REG, data, mask);

	/* VDD Calibration Control 3 */
	mask = HPIPE_EXT_SELLV_RXSAMPL_MASK;
	data = 0x10 << HPIPE_EXT_SELLV_RXSAMPL_OFFSET;
	reg_set(hpipe_addr + HPIPE_VDD_CAL_CTRL_REG, data, mask);

	/* DFE Resolution Control */
	mask = HPIPE_DFE_RES_FORCE_MASK;
	data = 0x1 << HPIPE_DFE_RES_FORCE_OFFSET;
	reg_set(hpipe_addr + HPIPE_DFE_REG0, data, mask);

	/* DFE F3-F5 Coefficient Control */
	mask = HPIPE_DFE_F3_F5_DFE_EN_MASK;
	data = 0x0 << HPIPE_DFE_F3_F5_DFE_EN_OFFSET;
	mask |= HPIPE_DFE_F3_F5_DFE_CTRL_MASK;
	data = 0x0 << HPIPE_DFE_F3_F5_DFE_CTRL_OFFSET;
	reg_set(hpipe_addr + HPIPE_DFE_F3_F5_REG, data, mask);

	/* G3 Setting 3 */
	mask = HPIPE_G3_FFE_CAP_SEL_MASK;
	data = 0xf << HPIPE_G3_FFE_CAP_SEL_OFFSET;
	mask |= HPIPE_G3_FFE_RES_SEL_MASK;
	data |= 0x4 << HPIPE_G3_FFE_RES_SEL_OFFSET;
	mask |= HPIPE_G3_FFE_SETTING_FORCE_MASK;
	data |= 0x1 << HPIPE_G3_FFE_SETTING_FORCE_OFFSET;
	mask |= HPIPE_G3_FFE_DEG_RES_LEVEL_MASK;
	data |= 0x1 << HPIPE_G3_FFE_DEG_RES_LEVEL_OFFSET;
	mask |= HPIPE_G3_FFE_LOAD_RES_LEVEL_MASK;
	data |= 0x3 << HPIPE_G3_FFE_LOAD_RES_LEVEL_OFFSET;
	reg_set(hpipe_addr + HPIPE_G3_SETTING_3_REG, data, mask);

	/* G3 Setting 4 */
	mask = HPIPE_G3_DFE_RES_MASK;
	data = 0x1 << HPIPE_G3_DFE_RES_OFFSET;
	reg_set(hpipe_addr + HPIPE_G3_SETTING_4_REG, data, mask);

	/* Offset Phase Control */
	mask = HPIPE_OS_PH_OFFSET_MASK;
	data = 0x61 << HPIPE_OS_PH_OFFSET_OFFSET;
	mask |= HPIPE_OS_PH_OFFSET_FORCE_MASK;
	data |= 0x1 << HPIPE_OS_PH_OFFSET_FORCE_OFFSET;
	mask |= HPIPE_OS_PH_VALID_MASK;
	data |= 0x0 << HPIPE_OS_PH_VALID_OFFSET;
	reg_set(hpipe_addr + HPIPE_PHASE_CONTROL_REG, data, mask);
	mask = HPIPE_OS_PH_VALID_MASK;
	data = 0x1 << HPIPE_OS_PH_VALID_OFFSET;
	reg_set(hpipe_addr + HPIPE_PHASE_CONTROL_REG, data, mask);
	mask = HPIPE_OS_PH_VALID_MASK;
	data = 0x0 << HPIPE_OS_PH_VALID_OFFSET;
	reg_set(hpipe_addr + HPIPE_PHASE_CONTROL_REG, data, mask);

	/* Set G1 TX amplitude and TX post emphasis value */
	mask = HPIPE_G1_SET_0_G1_TX_AMP_MASK;
	data = 0x8 << HPIPE_G1_SET_0_G1_TX_AMP_OFFSET;
	mask |= HPIPE_G1_SET_0_G1_TX_AMP_ADJ_MASK;
	data |= 0x1 << HPIPE_G1_SET_0_G1_TX_AMP_ADJ_OFFSET;
	mask |= HPIPE_G1_SET_0_G1_TX_EMPH1_MASK;
	data |= 0x1 << HPIPE_G1_SET_0_G1_TX_EMPH1_OFFSET;
	mask |= HPIPE_G1_SET_0_G1_TX_EMPH1_EN_MASK;
	data |= 0x1 << HPIPE_G1_SET_0_G1_TX_EMPH1_EN_OFFSET;
	reg_set(hpipe_addr + HPIPE_G1_SET_0_REG, data, mask);

	/* Set G2 TX amplitude and TX post emphasis value */
	mask = HPIPE_G2_SET_0_G2_TX_AMP_MASK;
	data = 0xa << HPIPE_G2_SET_0_G2_TX_AMP_OFFSET;
	mask |= HPIPE_G2_SET_0_G2_TX_AMP_ADJ_MASK;
	data |= 0x1 << HPIPE_G2_SET_0_G2_TX_AMP_ADJ_OFFSET;
	mask |= HPIPE_G2_SET_0_G2_TX_EMPH1_MASK;
	data |= 0x2 << HPIPE_G2_SET_0_G2_TX_EMPH1_OFFSET;
	mask |= HPIPE_G2_SET_0_G2_TX_EMPH1_EN_MASK;
	data |= 0x1 << HPIPE_G2_SET_0_G2_TX_EMPH1_EN_OFFSET;
	reg_set(hpipe_addr + HPIPE_G2_SET_0_REG, data, mask);

	/* Set G3 TX amplitude and TX post emphasis value */
	mask = HPIPE_G3_SET_0_G3_TX_AMP_MASK;
	data = 0x1e << HPIPE_G3_SET_0_G3_TX_AMP_OFFSET;
	mask |= HPIPE_G3_SET_0_G3_TX_AMP_ADJ_MASK;
	data |= 0x1 << HPIPE_G3_SET_0_G3_TX_AMP_ADJ_OFFSET;
	mask |= HPIPE_G3_SET_0_G3_TX_EMPH1_MASK;
	data |= 0xe << HPIPE_G3_SET_0_G3_TX_EMPH1_OFFSET;
	mask |= HPIPE_G3_SET_0_G3_TX_EMPH1_EN_MASK;
	data |= 0x1 << HPIPE_G3_SET_0_G3_TX_EMPH1_EN_OFFSET;
	mask |= HPIPE_G3_SET_0_G3_TX_SLEW_RATE_SEL_MASK;
	data |= 0x4 << HPIPE_G3_SET_0_G3_TX_SLEW_RATE_SEL_OFFSET;
	mask |= HPIPE_G3_SET_0_G3_TX_SLEW_CTRL_EN_MASK;
	data |= 0x0 << HPIPE_G3_SET_0_G3_TX_SLEW_CTRL_EN_OFFSET;
	reg_set(hpipe_addr + HPIPE_G3_SET_0_REG, data, mask);

	/* SERDES External Configuration 2 register */
	mask = SD_EXTERNAL_CONFIG2_SSC_ENABLE_MASK;
	data = 0x1 << SD_EXTERNAL_CONFIG2_SSC_ENABLE_OFFSET;
	reg_set(sd_ip_addr + SD_EXTERNAL_CONFIG2_REG, data, mask);

	/* DFE reset sequence */
	reg_set(hpipe_addr + HPIPE_PWR_CTR_REG,
		0x1 << HPIPE_PWR_CTR_RST_DFE_OFFSET, HPIPE_PWR_CTR_RST_DFE_MASK);
	reg_set(hpipe_addr + HPIPE_PWR_CTR_REG,
		0x0 << HPIPE_PWR_CTR_RST_DFE_OFFSET, HPIPE_PWR_CTR_RST_DFE_MASK);
	/* SW reset for interrupt logic */
	reg_set(hpipe_addr + HPIPE_PWR_CTR_REG,
		0x1 << HPIPE_PWR_CTR_SFT_RST_OFFSET, HPIPE_PWR_CTR_SFT_RST_MASK);
	reg_set(hpipe_addr + HPIPE_PWR_CTR_REG,
		0x0 << HPIPE_PWR_CTR_SFT_RST_OFFSET, HPIPE_PWR_CTR_SFT_RST_MASK);

	dev_dbg(priv->dev, "stage: Comphy power up\n");

	return ret;
}

static int mvebu_cp110_comphy_sgmii_power_on(struct mvebu_comphy_priv *priv,
					     struct mvebu_comphy *comphy)
{
	void __iomem *hpipe_addr, *sd_ip_addr, *comphy_addr, *addr;
	u32 mask, data;
	int ret = 0;
	int mode = COMPHY_GET_MODE(priv->lanes[comphy->index].mode);

	dev_dbg(priv->dev, "%s: Enter\n", __func__);

	hpipe_addr = HPIPE_ADDR(priv->comphy_pipe_regs, comphy->index);
	sd_ip_addr = SD_ADDR(priv->comphy_pipe_regs, comphy->index);
	comphy_addr = COMPHY_ADDR(priv->comphy_regs, comphy->index);

	/* configure phy selector for SGMII */
	mvebu_cp110_comphy_set_phy_selector(priv, comphy);

	/* Confiugre the lane */
	dev_dbg(priv->dev, "stage: RFU configurations - hard reset comphy\n");
	/* RFU configurations - hard reset comphy */
	mask = COMMON_PHY_CFG1_PWR_UP_MASK;
	data = 0x1 << COMMON_PHY_CFG1_PWR_UP_OFFSET;
	mask |= COMMON_PHY_CFG1_PIPE_SELECT_MASK;
	data |= 0x0 << COMMON_PHY_CFG1_PIPE_SELECT_OFFSET;
	reg_set(comphy_addr + COMMON_PHY_CFG1_REG, data, mask);

	/* Select Baud Rate of Comphy And PD_PLL/Tx/Rx */
	mask = SD_EXTERNAL_CONFIG0_SD_PU_PLL_MASK;
	data = 0x0 << SD_EXTERNAL_CONFIG0_SD_PU_PLL_OFFSET;
	mask |= SD_EXTERNAL_CONFIG0_SD_PHY_GEN_RX_MASK;
	mask |= SD_EXTERNAL_CONFIG0_SD_PHY_GEN_TX_MASK;

	if (mode == COMPHY_SGMII_MODE) {
		/* SGMII 1G, SerDes speed 1.25G */
		data |= 0x6 << SD_EXTERNAL_CONFIG0_SD_PHY_GEN_RX_OFFSET;
		data |= 0x6 << SD_EXTERNAL_CONFIG0_SD_PHY_GEN_TX_OFFSET;
	} else if (mode == COMPHY_HS_SGMII_MODE) {
		/* HS SGMII (2.5G), SerDes speed 3.125G */
		data |= 0x8 << SD_EXTERNAL_CONFIG0_SD_PHY_GEN_RX_OFFSET;
		data |= 0x8 << SD_EXTERNAL_CONFIG0_SD_PHY_GEN_TX_OFFSET;
	} else {
		/* Other rates are not supported */
		dev_err(priv->dev, "unsupported SGMII speed on comphy%d\n",
			comphy->index);
		return -EINVAL;
	}

	mask |= SD_EXTERNAL_CONFIG0_SD_PU_RX_MASK;
	data |= 0 << SD_EXTERNAL_CONFIG0_SD_PU_RX_OFFSET;
	mask |= SD_EXTERNAL_CONFIG0_SD_PU_TX_MASK;
	data |= 0 << SD_EXTERNAL_CONFIG0_SD_PU_TX_OFFSET;
	mask |= SD_EXTERNAL_CONFIG0_HALF_BUS_MODE_MASK;
	data |= 1 << SD_EXTERNAL_CONFIG0_HALF_BUS_MODE_OFFSET;
	reg_set(sd_ip_addr + SD_EXTERNAL_CONFIG0_REG, data, mask);

	/* Set hard reset */
	mask = SD_EXTERNAL_CONFIG1_RESET_IN_MASK;
	data = 0x0 << SD_EXTERNAL_CONFIG1_RESET_IN_OFFSET;
	mask |= SD_EXTERNAL_CONFIG1_RESET_CORE_MASK;
	data |= 0x0 << SD_EXTERNAL_CONFIG1_RESET_CORE_OFFSET;
	mask |= SD_EXTERNAL_CONFIG1_RF_RESET_IN_MASK;
	data |= 0x0 << SD_EXTERNAL_CONFIG1_RF_RESET_IN_OFFSET;
	reg_set(sd_ip_addr + SD_EXTERNAL_CONFIG1_REG, data, mask);

	/* Release hard reset */
	mask = SD_EXTERNAL_CONFIG1_RESET_IN_MASK;
	data = 0x1 << SD_EXTERNAL_CONFIG1_RESET_IN_OFFSET;
	mask |= SD_EXTERNAL_CONFIG1_RESET_CORE_MASK;
	data |= 0x1 << SD_EXTERNAL_CONFIG1_RESET_CORE_OFFSET;
	reg_set(sd_ip_addr + SD_EXTERNAL_CONFIG1_REG, data, mask);

	/* Wait 1ms - until band gap and ref clock ready */
	mdelay(1);

	/* Make sure that 40 data bits is disabled
	 * This bit is not cleared by reset
	 */
	mask = COMMON_PHY_CFG6_IF_40_SEL_MASK;
	data = 0 << COMMON_PHY_CFG6_IF_40_SEL_OFFSET;
	reg_set(comphy_addr + COMMON_PHY_CFG6_REG, data, mask);

	/* Start comphy Configuration */
	dev_dbg(priv->dev, "stage: Comphy configuration\n");
	/* set reference clock */
	mask = HPIPE_MISC_REFCLK_SEL_MASK;
	data = 0x0 << HPIPE_MISC_REFCLK_SEL_OFFSET;
	reg_set(hpipe_addr + HPIPE_MISC_REG, data, mask);
	/* Power and PLL Control */
	mask = HPIPE_PWR_PLL_REF_FREQ_MASK;
	data = 0x1 << HPIPE_PWR_PLL_REF_FREQ_OFFSET;
	mask |= HPIPE_PWR_PLL_PHY_MODE_MASK;
	data |= 0x4 << HPIPE_PWR_PLL_PHY_MODE_OFFSET;
	reg_set(hpipe_addr + HPIPE_PWR_PLL_REG, data, mask);
	/* Loopback register */
	mask = HPIPE_LOOPBACK_SEL_MASK;
	data = 0x1 << HPIPE_LOOPBACK_SEL_OFFSET;
	reg_set(hpipe_addr + HPIPE_LOOPBACK_REG, data, mask);
	/* rx control 1 */
	mask = HPIPE_RX_CONTROL_1_RXCLK2X_SEL_MASK;
	data = 0x1 << HPIPE_RX_CONTROL_1_RXCLK2X_SEL_OFFSET;
	mask |= HPIPE_RX_CONTROL_1_CLK8T_EN_MASK;
	data |= 0x0 << HPIPE_RX_CONTROL_1_CLK8T_EN_OFFSET;
	reg_set(hpipe_addr + HPIPE_RX_CONTROL_1_REG, data, mask);
	/* DTL Control */
	mask = HPIPE_PWR_CTR_DTL_FLOOP_EN_MASK;
	data = 0x0 << HPIPE_PWR_CTR_DTL_FLOOP_EN_OFFSET;
	reg_set(hpipe_addr + HPIPE_PWR_CTR_DTL_REG, data, mask);

	/* Set analog parameters from ETP(HW) - for now use the default datas */
	dev_dbg(priv->dev, "stage: Analog parameters from ETP(HW)\n");

	reg_set(hpipe_addr + HPIPE_G1_SET_0_REG,
		0x1 << HPIPE_G1_SET_0_G1_TX_EMPH1_OFFSET, HPIPE_G1_SET_0_G1_TX_EMPH1_MASK);

	dev_dbg(priv->dev, "stage: RFU configurations- Power Up PLL,Tx,Rx\n");
	/* SERDES External Configuration */
	mask = SD_EXTERNAL_CONFIG0_SD_PU_PLL_MASK;
	data = 0x1 << SD_EXTERNAL_CONFIG0_SD_PU_PLL_OFFSET;
	mask |= SD_EXTERNAL_CONFIG0_SD_PU_RX_MASK;
	data |= 0x1 << SD_EXTERNAL_CONFIG0_SD_PU_RX_OFFSET;
	mask |= SD_EXTERNAL_CONFIG0_SD_PU_TX_MASK;
	data |= 0x1 << SD_EXTERNAL_CONFIG0_SD_PU_TX_OFFSET;
	reg_set(sd_ip_addr + SD_EXTERNAL_CONFIG0_REG, data, mask);

	/* check PLL rx & tx ready */
	addr = sd_ip_addr + SD_EXTERNAL_STATUS0_REG;
	data = SD_EXTERNAL_STATUS0_PLL_RX_MASK | SD_EXTERNAL_STATUS0_PLL_TX_MASK;
	mask = data;
	data = polling_with_timeout(addr, data, mask, 15000, REG_32BIT);
	if (data != 0) {
		if (data & SD_EXTERNAL_STATUS0_PLL_RX_MASK)
			dev_err(priv->dev, "RX PLL is not locked\n");
		if (data & SD_EXTERNAL_STATUS0_PLL_TX_MASK)
			dev_err(priv->dev, "TX PLL is not locked\n");

		ret = -ETIMEDOUT;
	}

	/* RX init */
	mask = SD_EXTERNAL_CONFIG1_RX_INIT_MASK;
	data = 0x1 << SD_EXTERNAL_CONFIG1_RX_INIT_OFFSET;
	reg_set(sd_ip_addr + SD_EXTERNAL_CONFIG1_REG, data, mask);

	/* check that RX init done */
	addr = sd_ip_addr + SD_EXTERNAL_STATUS0_REG;
	data = SD_EXTERNAL_STATUS0_RX_INIT_MASK;
	mask = data;
	data = polling_with_timeout(addr, data, mask, 100, REG_32BIT);
	if (data != 0) {
		dev_err(priv->dev, "RX init failed\n");
		ret = -ETIMEDOUT;
	}

	dev_dbg(priv->dev, "stage: RF Reset\n");
	/* RF Reset */
	mask =	SD_EXTERNAL_CONFIG1_RX_INIT_MASK;
	data = 0x0 << SD_EXTERNAL_CONFIG1_RX_INIT_OFFSET;
	mask |= SD_EXTERNAL_CONFIG1_RF_RESET_IN_MASK;
	data |= 0x1 << SD_EXTERNAL_CONFIG1_RF_RESET_IN_OFFSET;
	reg_set(sd_ip_addr + SD_EXTERNAL_CONFIG1_REG, data, mask);

	return ret;
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

static int mvebu_cp110_comphy_pcie_power_on(struct mvebu_comphy_priv *priv,
					    struct mvebu_comphy *comphy)
{
	int ret = 0;
	u32 mask, data, pcie_width;
	unsigned int lane = comphy->index;
	void __iomem *addr;
	bool is_end_point;
	u32 clk_dir;
	struct sar_val sar;
	struct device_node *sar_node;
	struct platform_device *sar_pdev;
	void __iomem *hpipe_addr = HPIPE_ADDR(priv->comphy_pipe_regs, lane);
	void __iomem *comphy_addr = COMPHY_ADDR(priv->comphy_regs, lane);
	bool clk_src = COMPHY_GET_CLK_SRC(priv->lanes[comphy->index].mode);
	struct platform_device *pdev = container_of(priv->dev,
						    struct platform_device,
						    dev);
	struct device_node *dn = pdev->dev.of_node;

	dev_dbg(priv->dev, "%s: Enter\n", __func__);

	/* Configure PIPE selector for PCIE */
	mvebu_cp110_comphy_set_pipe_selector(priv, comphy);

	/*
	 * Read SAR (Sample-At-Reset) configuration for the PCIe clock
	 * direction.
	 *
	 * SerDes Lane 4/5 got the PCIe ref-clock #1,
	 * and SerDes Lane 0 got PCIe ref-clock #0
	 */
	sar_node = of_parse_phandle(dn, "sar-data", 0);
	if (!sar_node) {
		dev_err(&pdev->dev, "Failed to get SAR data\n");
		return -EINVAL;
	}
	sar_pdev = of_find_device_by_node(sar_node);
	if (!sar_pdev) {
		dev_err(&pdev->dev, "Failed to get SAR device\n");
		return -EINVAL;
	}
	if (comphy->index == COMPHY_LANE4 || comphy->index == COMPHY_LANE5)
		mv_sar_value_get(&sar_pdev->dev, SAR_CP_PCIE1_CLK, &sar);
	else
		mv_sar_value_get(&sar_pdev->dev, SAR_CP_PCIE0_CLK, &sar);

	clk_dir = sar.clk_direction;

	is_end_point = priv->lanes[comphy->index].misc.pcie_is_ep;
	pcie_width = priv->lanes[lane].misc.pcie_width;

	dev_dbg(priv->dev, "On lane %d\n", lane);
	dev_dbg(priv->dev, "PCIe clock direction = %x\n", clk_dir);
	dev_dbg(priv->dev, "PCIe RC    = %d\n", !is_end_point);
	dev_dbg(priv->dev, "PCIe Width = %d\n", pcie_width);

	/* enable PCIe X4 and X2 */
	if (lane == COMPHY_LANE0) {
		if (pcie_width == 4) {
			data = 0x1 << COMMON_PHY_SD_CTRL1_PCIE_X4_EN_OFFSET;
			mask = COMMON_PHY_SD_CTRL1_PCIE_X4_EN_MASK;
			reg_set(priv->comphy_regs + COMMON_PHY_SD_CTRL1,
				data, mask);
		} else if (pcie_width == 2) {
			data = 0x1 << COMMON_PHY_SD_CTRL1_PCIE_X2_EN_OFFSET;
			mask = COMMON_PHY_SD_CTRL1_PCIE_X2_EN_MASK;
			reg_set(priv->comphy_regs + COMMON_PHY_SD_CTRL1,
				data, mask);
		}
	}

	/* If PCIe clock is output and clock source from SerDes lane 5,
	 * need to configure the clock-source MUX.
	 * By default, the clock source is from lane 4
	 */
	if (clk_dir && clk_src && (lane == COMPHY_LANE5) &&
	    of_device_is_compatible(dn, "marvell,cp110-comphy")) {
		void __iomem *cp110_dfx_reg;
		const __be32 *reg;
		phys_addr_t paddr;
		u32 size;
		int len;

		/* Get DFX register property */
		reg = of_get_property(dn, "dfx-reg", &len);
		if (!reg) {
			dev_err(priv->dev,
				"No DFX register found\n");
			return -EINVAL;
		}

		/* Translate the offset to physical address */
		paddr = of_translate_address(dn, reg);
		if (paddr == OF_BAD_ADDR) {
			dev_err(priv->dev,
				"of_translate_address failed for DFX\n");
			return -EINVAL;
		}

		/* Get register space size */
		size = be32_to_cpup(&reg[1]);
		cp110_dfx_reg = ioremap(paddr, size);
		if (!cp110_dfx_reg) {
			dev_err(priv->dev,
				"ioremap failed for DFX register space\n");
			return -EINVAL;
		}

		data = DFX_DEV_GEN_PCIE_CLK_SRC_MUX <<
						DFX_DEV_GEN_PCIE_CLK_SRC_OFFSET;
		mask = DFX_DEV_GEN_PCIE_CLK_SRC_MASK;
		reg_set(cp110_dfx_reg, data, mask);

		/* Release resources */
		iounmap(cp110_dfx_reg);
	}

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
	data |= 0x0 << COMMON_PHY_PHY_MODE_OFFSET;
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
	data |= 0x1 << HPIPE_RST_CLK_CTRL_FIXED_PCLK_OFFSET;
	/* Set Data bus width USB mode for V0 */
	mask |= HPIPE_RST_CLK_CTRL_PIPE_WIDTH_MASK;
	data |= 0x0 << HPIPE_RST_CLK_CTRL_PIPE_WIDTH_OFFSET;
	/* Set CORE_CLK output frequency for 250Mhz */
	mask |= HPIPE_RST_CLK_CTRL_CORE_FREQ_SEL_MASK;
	data |= 0x0 << HPIPE_RST_CLK_CTRL_CORE_FREQ_SEL_OFFSET;
	reg_set(hpipe_addr + HPIPE_RST_CLK_CTRL_REG, data, mask);
	/* Set PLL ready delay for 0x2 */
	data = 0x2 << HPIPE_CLK_SRC_LO_PLL_RDY_DL_OFFSET;
	mask = HPIPE_CLK_SRC_LO_PLL_RDY_DL_MASK;
	if (pcie_width != 1) {
		data |= 0x1 << HPIPE_CLK_SRC_LO_BUNDLE_PERIOD_SEL_OFFSET;
		mask |= HPIPE_CLK_SRC_LO_BUNDLE_PERIOD_SEL_MASK;
		data |= 0x1 << HPIPE_CLK_SRC_LO_BUNDLE_PERIOD_SCALE_OFFSET;
		mask |= HPIPE_CLK_SRC_LO_BUNDLE_PERIOD_SCALE_MASK;
	}
	reg_set(hpipe_addr + HPIPE_CLK_SRC_LO_REG, data, mask);

	/* Set PIPE mode interface to PCIe3 - 0x1  & set lane order */
	data = 0x1 << HPIPE_CLK_SRC_HI_MODE_PIPE_OFFSET;
	mask = HPIPE_CLK_SRC_HI_MODE_PIPE_MASK;
	if (pcie_width != 1) {
		mask |= HPIPE_CLK_SRC_HI_LANE_STRT_MASK;
		mask |= HPIPE_CLK_SRC_HI_LANE_MASTER_MASK;
		mask |= HPIPE_CLK_SRC_HI_LANE_BREAK_MASK;
		if (lane == 0) {
			data |= 0x1 << HPIPE_CLK_SRC_HI_LANE_STRT_OFFSET;
			data |= 0x1 << HPIPE_CLK_SRC_HI_LANE_MASTER_OFFSET;
		} else if (lane == (pcie_width - 1)) {
			data |= 0x1 << HPIPE_CLK_SRC_HI_LANE_BREAK_OFFSET;
		}
	}
	reg_set(hpipe_addr + HPIPE_CLK_SRC_HI_REG, data, mask);
	/* Config update polarity equalization */
	data = 0x1 << HPIPE_CFG_UPDATE_POLARITY_OFFSET;
	mask = HPIPE_CFG_UPDATE_POLARITY_MASK;
	reg_set(hpipe_addr + HPIPE_LANE_EQ_CFG1_REG, data, mask);
	/* Set PIPE version 4 to mode enable */
	data = 0x1 << HPIPE_DFE_CTRL_28_PIPE4_OFFSET;
	mask = HPIPE_DFE_CTRL_28_PIPE4_MASK;
	reg_set(hpipe_addr + HPIPE_DFE_CTRL_28_REG, data, mask);
	/* TODO: check if pcie clock is output/input - for bringup use input*/
	/* Enable PIN clock 100M_125M */
	mask = 0;
	data = 0;
	/* Only if clock is output, configure the clock-source mux */
	if (clk_dir) {
		mask |= HPIPE_MISC_CLK100M_125M_MASK;
		data |= 0x1 << HPIPE_MISC_CLK100M_125M_OFFSET;
	}
	/* Set PIN_TXDCLK_2X Clock Frequency Selection for outputs 500MHz clock */
	mask |= HPIPE_MISC_TXDCLK_2X_MASK;
	data |= 0x0 << HPIPE_MISC_TXDCLK_2X_OFFSET;
	/* Enable 500MHz Clock */
	mask |= HPIPE_MISC_CLK500_EN_MASK;
	data |= 0x1 << HPIPE_MISC_CLK500_EN_OFFSET;
	if (clk_dir) { /* output */
		/* Set reference clock comes from group 1 */
		mask |= HPIPE_MISC_REFCLK_SEL_MASK;
		data |= 0x0 << HPIPE_MISC_REFCLK_SEL_OFFSET;
	} else {
		/* Set reference clock comes from group 2 */
		mask |= HPIPE_MISC_REFCLK_SEL_MASK;
		data |= 0x1 << HPIPE_MISC_REFCLK_SEL_OFFSET;
	}
	mask |= HPIPE_MISC_ICP_FORCE_MASK;
	data |= 0x1 << HPIPE_MISC_ICP_FORCE_OFFSET;
	reg_set(hpipe_addr + HPIPE_MISC_REG, data, mask);
	if (clk_dir) { /* output */
		/* Set reference frequcency select - 0x2 for 25MHz*/
		mask = HPIPE_PWR_PLL_REF_FREQ_MASK;
		data = 0x2 << HPIPE_PWR_PLL_REF_FREQ_OFFSET;
	} else {
		/* Set reference frequcency select - 0x0 for 100MHz*/
		mask = HPIPE_PWR_PLL_REF_FREQ_MASK;
		data = 0x0 << HPIPE_PWR_PLL_REF_FREQ_OFFSET;
	}
	/* Set PHY mode to PCIe */
	mask |= HPIPE_PWR_PLL_PHY_MODE_MASK;
	data |= 0x3 << HPIPE_PWR_PLL_PHY_MODE_OFFSET;
	reg_set(hpipe_addr + HPIPE_PWR_PLL_REG, data, mask);

	/* ref clock alignment */
	if (pcie_width != 1) {
		mask = HPIPE_LANE_ALIGN_OFF_MASK;
		data = 0x0 << HPIPE_LANE_ALIGN_OFF_OFFSET;
		reg_set(hpipe_addr + HPIPE_LANE_ALIGN_REG, data, mask);
	}

	/* Set the amount of time spent in the LoZ state - set for 0x7 only if
	 * the PCIe clock is output
	 */
	if (clk_dir)
		reg_set(hpipe_addr + HPIPE_GLOBAL_PM_CTRL,
			0x7 << HPIPE_GLOBAL_PM_RXDLOZ_WAIT_OFFSET,
			HPIPE_GLOBAL_PM_RXDLOZ_WAIT_MASK);

	/* Set Maximal PHY Generation Setting(8Gbps) */
	mask = HPIPE_INTERFACE_GEN_MAX_MASK;
	data = 0x2 << HPIPE_INTERFACE_GEN_MAX_OFFSET;
	/* Bypass frame detection and sync detection for RX DATA */
	mask |= HPIPE_INTERFACE_DET_BYPASS_MASK;
	data |= 0x1 << HPIPE_INTERFACE_DET_BYPASS_OFFSET;
	/* Set Link Train Mode (Tx training control pins are used) */
	mask |= HPIPE_INTERFACE_LINK_TRAIN_MASK;
	data |= 0x1 << HPIPE_INTERFACE_LINK_TRAIN_OFFSET;
	reg_set(hpipe_addr + HPIPE_INTERFACE_REG, data, mask);

	/* Set Idle_sync enable */
	mask = HPIPE_PCIE_IDLE_SYNC_MASK;
	data = 0x1 << HPIPE_PCIE_IDLE_SYNC_OFFSET;
	/* Select bits for PCIE Gen3(32bit) */
	mask |= HPIPE_PCIE_SEL_BITS_MASK;
	data |= 0x2 << HPIPE_PCIE_SEL_BITS_OFFSET;
	reg_set(hpipe_addr + HPIPE_PCIE_REG0, data, mask);

	/* Enable Tx_adapt_g1 */
	mask = HPIPE_TX_TRAIN_CTRL_G1_MASK;
	data = 0x1 << HPIPE_TX_TRAIN_CTRL_G1_OFFSET;
	/* Enable Tx_adapt_gn1 */
	mask |= HPIPE_TX_TRAIN_CTRL_GN1_MASK;
	data |= 0x1 << HPIPE_TX_TRAIN_CTRL_GN1_OFFSET;
	/* Disable Tx_adapt_g0 */
	mask |= HPIPE_TX_TRAIN_CTRL_G0_MASK;
	data |= 0x0 << HPIPE_TX_TRAIN_CTRL_G0_OFFSET;
	reg_set(hpipe_addr + HPIPE_TX_TRAIN_CTRL_REG, data, mask);

	/* Set reg_tx_train_chk_init */
	mask = HPIPE_TX_TRAIN_CHK_INIT_MASK;
	data = 0x0 << HPIPE_TX_TRAIN_CHK_INIT_OFFSET;
	/* Enable TX_COE_FM_PIN_PCIE3_EN */
	mask |= HPIPE_TX_TRAIN_COE_FM_PIN_PCIE3_MASK;
	data |= 0x1 << HPIPE_TX_TRAIN_COE_FM_PIN_PCIE3_OFFSET;
	reg_set(hpipe_addr + HPIPE_TX_TRAIN_REG, data, mask);

	dev_dbg(priv->dev, "stage: TRx training parameters\n");
	/* Set Preset sweep configurations */
	mask = HPIPE_TX_TX_STATUS_CHECK_MODE_MASK;
	data = 0x1 << HPIPE_TX_STATUS_CHECK_MODE_OFFSET;

	mask |= HPIPE_TX_NUM_OF_PRESET_MASK;
	data |= 0x7 << HPIPE_TX_NUM_OF_PRESET_OFFSET;

	mask |= HPIPE_TX_SWEEP_PRESET_EN_MASK;
	data |= 0x1 << HPIPE_TX_SWEEP_PRESET_EN_OFFSET;
	reg_set(hpipe_addr + HPIPE_TX_TRAIN_CTRL_11_REG, data, mask);

	/* Tx train start configuration */
	mask = HPIPE_TX_TRAIN_START_SQ_EN_MASK;
	data = 0x1 << HPIPE_TX_TRAIN_START_SQ_EN_OFFSET;

	mask |= HPIPE_TX_TRAIN_START_FRM_DET_EN_MASK;
	data |= 0x0 << HPIPE_TX_TRAIN_START_FRM_DET_EN_OFFSET;

	mask |= HPIPE_TX_TRAIN_START_FRM_LOCK_EN_MASK;
	data |= 0x0 << HPIPE_TX_TRAIN_START_FRM_LOCK_EN_OFFSET;

	mask |= HPIPE_TX_TRAIN_WAIT_TIME_EN_MASK;
	data |= 0x1 << HPIPE_TX_TRAIN_WAIT_TIME_EN_OFFSET;
	reg_set(hpipe_addr + HPIPE_TX_TRAIN_CTRL_5_REG, data, mask);

	/* Enable Tx train P2P */
	mask = HPIPE_TX_TRAIN_P2P_HOLD_MASK;
	data = 0x1 << HPIPE_TX_TRAIN_P2P_HOLD_OFFSET;
	reg_set(hpipe_addr + HPIPE_TX_TRAIN_CTRL_0_REG, data, mask);

	/* Configure Tx train timeout */
	mask = HPIPE_TRX_TRAIN_TIMER_MASK;
	data = 0x17 << HPIPE_TRX_TRAIN_TIMER_OFFSET;
	reg_set(hpipe_addr + HPIPE_TX_TRAIN_CTRL_4_REG, data, mask);

	/* Disable G0/G1/GN1 adaptation */
	mask = HPIPE_TX_TRAIN_CTRL_G1_MASK | HPIPE_TX_TRAIN_CTRL_GN1_MASK
		| HPIPE_TX_TRAIN_CTRL_G0_OFFSET;
	data = 0;
	reg_set(hpipe_addr + HPIPE_TX_TRAIN_CTRL_REG, data, mask);

	/* Disable DTL frequency loop */
	mask = HPIPE_PWR_CTR_DTL_FLOOP_EN_MASK;
	data = 0x0 << HPIPE_PWR_CTR_DTL_FLOOP_EN_OFFSET;
	reg_set(hpipe_addr + HPIPE_PWR_CTR_DTL_REG, data, mask);

	/* Configure G3 DFE */
	mask = HPIPE_G3_DFE_RES_MASK;
	data = 0x3 << HPIPE_G3_DFE_RES_OFFSET;
	reg_set(hpipe_addr + HPIPE_G3_SETTING_4_REG, data, mask);

	/* Use TX/RX training result for DFE */
	mask = HPIPE_DFE_RES_FORCE_MASK;
	data = 0x0 << HPIPE_DFE_RES_FORCE_OFFSET;
	reg_set(hpipe_addr + HPIPE_DFE_REG0,  data, mask);

	/* Configure initial and final coefficient value for receiver */
	mask = HPIPE_G3_SET_1_G3_RX_SELMUPI_MASK;
	data = 0x1 << HPIPE_G3_SET_1_G3_RX_SELMUPI_OFFSET;

	mask |= HPIPE_G3_SET_1_G3_RX_SELMUPF_MASK;
	data |= 0x1 << HPIPE_G3_SET_1_G3_RX_SELMUPF_OFFSET;

	mask |= HPIPE_G3_SET_1_G3_SAMPLER_INPAIRX2_EN_MASK;
	data |= 0x0 << HPIPE_G3_SET_1_G3_SAMPLER_INPAIRX2_EN_OFFSET;
	reg_set(hpipe_addr + HPIPE_G3_SET_1_REG,  data, mask);

	/* Trigger sampler enable pulse */
	mask = HPIPE_SMAPLER_MASK;
	data = 0x1 << HPIPE_SMAPLER_OFFSET;
	reg_set(hpipe_addr + HPIPE_SAMPLER_N_PROC_CALIB_CTRL_REG, data, mask);
	udelay(5);
	reg_set(hpipe_addr + HPIPE_SAMPLER_N_PROC_CALIB_CTRL_REG, 0, mask);

	/* FFE resistor tuning for different bandwidth  */
	mask = HPIPE_G3_FFE_DEG_RES_LEVEL_MASK;
	data = 0x1 << HPIPE_G3_FFE_DEG_RES_LEVEL_OFFSET;
	mask |= HPIPE_G3_FFE_LOAD_RES_LEVEL_MASK;
	data |= 0x3 << HPIPE_G3_FFE_LOAD_RES_LEVEL_OFFSET;
	reg_set(hpipe_addr + HPIPE_G3_SETTING_3_REG, data, mask);

	/* Pattern lock lost timeout disable */
	mask = HPIPE_PATTERN_LOCK_LOST_TIMEOUT_EN_MASK;
	data = 0x0 << HPIPE_PATTERN_LOCK_LOST_TIMEOUT_EN_OFFSET;
	reg_set(hpipe_addr + HPIPE_FRAME_DETECT_CTRL_3_REG, data, mask);

	/* Configure DFE adaptations */
	mask = HPIPE_CDR_MAX_DFE_ADAPT_1_MASK;
	data = 0x1 << HPIPE_CDR_MAX_DFE_ADAPT_1_OFFSET;
	mask |= HPIPE_CDR_MAX_DFE_ADAPT_0_MASK;
	data |= 0x0 << HPIPE_CDR_MAX_DFE_ADAPT_0_OFFSET;
	mask |= HPIPE_CDR_RX_MAX_DFE_ADAPT_1_MASK;
	data |= 0x0 << HPIPE_CDR_RX_MAX_DFE_ADAPT_1_OFFSET;
	reg_set(hpipe_addr + HPIPE_CDR_CONTROL_REG, data, mask);
	mask = HPIPE_DFE_TX_MAX_DFE_ADAPT_MASK;
	data = 0x0 << HPIPE_DFE_TX_MAX_DFE_ADAPT_OFFSET;
	reg_set(hpipe_addr + HPIPE_DFE_CONTROL_REG, data, mask);

	/* Genration 2 setting 1*/
	mask = HPIPE_G2_SET_1_G2_RX_SELMUPI_MASK;
	data = 0x0 << HPIPE_G2_SET_1_G2_RX_SELMUPI_OFFSET;
	mask |= HPIPE_G2_SET_1_G2_RX_SELMUPP_MASK;
	data |= 0x1 << HPIPE_G2_SET_1_G2_RX_SELMUPP_OFFSET;
	mask |= HPIPE_G2_SET_1_G2_RX_SELMUFI_MASK;
	data |= 0x0 << HPIPE_G2_SET_1_G2_RX_SELMUFI_OFFSET;
	reg_set(hpipe_addr + HPIPE_G2_SET_1_REG, data, mask);

	/* DFE enable */
	mask = HPIPE_G2_DFE_RES_MASK;
	data = 0x3 << HPIPE_G2_DFE_RES_OFFSET;
	reg_set(hpipe_addr + HPIPE_G2_SETTINGS_4_REG, data, mask);

	/* Configure DFE Resolution */
	mask = HPIPE_LANE_CFG4_DFE_EN_SEL_MASK;
	data = 0x1 << HPIPE_LANE_CFG4_DFE_EN_SEL_OFFSET;
	reg_set(hpipe_addr + HPIPE_LANE_CFG4_REG, data, mask);

	/* VDD calibration control */
	mask = HPIPE_EXT_SELLV_RXSAMPL_MASK;
	data = 0x16 << HPIPE_EXT_SELLV_RXSAMPL_OFFSET;
	reg_set(hpipe_addr + HPIPE_VDD_CAL_CTRL_REG, data, mask);

	/* Set PLL Charge-pump Current Control */
	mask = HPIPE_G3_SETTING_5_G3_ICP_MASK;
	data = 0x4 << HPIPE_G3_SETTING_5_G3_ICP_OFFSET;
	reg_set(hpipe_addr + HPIPE_G3_SETTING_5_REG, data, mask);

	/* Set lane rqualization remote setting */
	mask = HPIPE_LANE_CFG_FOM_DIRN_OVERRIDE_MASK;
	data = 0x1 << HPIPE_LANE_CFG_FOM_DIRN_OVERRIDE_OFFSET;
	mask |= HPIPE_LANE_CFG_FOM_ONLY_MODE_MASK;
	data |= 0x1 << HPIPE_LANE_CFG_FOM_ONLY_MODE_OFFFSET;
	mask |= HPIPE_LANE_CFG_FOM_PRESET_VECTOR_MASK;
	data |= 0x2 << HPIPE_LANE_CFG_FOM_PRESET_VECTOR_OFFSET;
	reg_set(hpipe_addr + HPIPE_LANE_EQ_REMOTE_SETTING_REG, data, mask);

	if (!is_end_point) {
		/* Set phy in root complex mode */
		mask = HPIPE_CFG_PHY_RC_EP_MASK;
		data = 0x1 << HPIPE_CFG_PHY_RC_EP_OFFSET;
		reg_set(hpipe_addr + HPIPE_LANE_EQU_CONFIG_0_REG, data, mask);
	}

	dev_dbg(priv->dev, "stage: Comphy power up\n");

	/* For PCIe X4 or X2:
	 * release from reset only after finish to configure all lanes
	 */
	if ((pcie_width == 1) || (lane == (pcie_width - 1))) {
		u32 i, start_lane, end_lane;

		if (pcie_width != 1) {
			/* allows writing to all lanes in one write */
			data = 0x0;
			mask = COMMON_PHY_SD_CTRL1_COMPHY_0_4_PORT_MASK;
			reg_set(priv->comphy_regs + COMMON_PHY_SD_CTRL1,
				data,
				mask);
			start_lane = 0;
			end_lane = pcie_width;

			/* Release from PIPE soft reset
			 * For PCIe by4 or by2:
			 * release from soft reset all lanes - can't use
			 *read modify write
			 */
			reg_set(HPIPE_ADDR(priv->comphy_pipe_regs, 0) +
				HPIPE_RST_CLK_CTRL_REG, 0x24, 0xffffffff);
		} else {
			start_lane = lane;
			end_lane = lane + 1;

			/* Release from PIPE soft reset
			 * for PCIe by4 or by2:
			 * release from soft reset all lanes
			 */
			reg_set(hpipe_addr + HPIPE_RST_CLK_CTRL_REG,
				0x0 << HPIPE_RST_CLK_CTRL_PIPE_RST_OFFSET,
				HPIPE_RST_CLK_CTRL_PIPE_RST_MASK);
		}

		if (pcie_width != 1) {
			/* disable writing to all lanes with one write */
			data = (COMPHY_LANE0 <<
				COMMON_PHY_SD_CTRL1_COMPHY_0_PORT_OFFSET) |
				(COMPHY_LANE1 <<
				COMMON_PHY_SD_CTRL1_COMPHY_1_PORT_OFFSET) |
				(COMPHY_LANE2 <<
				COMMON_PHY_SD_CTRL1_COMPHY_2_PORT_OFFSET) |
				(COMPHY_LANE3 <<
				COMMON_PHY_SD_CTRL1_COMPHY_3_PORT_OFFSET);
			mask = COMMON_PHY_SD_CTRL1_COMPHY_0_4_PORT_MASK;
			reg_set(priv->comphy_regs + COMMON_PHY_SD_CTRL1,
				data, mask);
		}

		dev_dbg(priv->dev, "stage: Check PLL\n");
		/* Read lane status */
		for (i = start_lane; i < end_lane; i++) {
			addr = HPIPE_ADDR(priv->comphy_pipe_regs, i) +
				HPIPE_LANE_STATUS1_REG;
			data = HPIPE_LANE_STATUS1_PCLK_EN_MASK;
			mask = data;
			ret = polling_with_timeout(addr, data, mask,
						   PLL_LOCK_TIMEOUT,
						   REG_32BIT);
			if (ret)
				dev_err(priv->dev, "Failed to lock PCIE PLL\n");
		}

	}

	dev_dbg(priv->dev, "%s: Exit\n", __func__);

	return ret;
}

static int mvebu_cp110_comphy_rxaui_power_on(struct mvebu_comphy_priv *priv,
					     struct mvebu_comphy *comphy)
{
	dev_dbg(priv->dev, "%s: Enter\n", __func__);

	dev_err(priv->dev, "RXAUI mode is not implemented\n");

	/* configure phy selector for RXAUI */
	mvebu_cp110_comphy_set_phy_selector(priv, comphy);

	dev_dbg(priv->dev, "%s: Exit\n", __func__);

	return -ENOTSUPP;
}

static int mvebu_cp110_comphy_xfi_power_on(struct mvebu_comphy_priv *priv,
					   struct mvebu_comphy *comphy)
{
	void __iomem *hpipe_addr, *sd_ip_addr, *comphy_addr, *addr;
	u32 mask, data, speed = COMPHY_GET_SPEED(priv->lanes[comphy->index].mode);
	int ret = 0;

	dev_dbg(priv->dev, "%s: Enter\n", __func__);

	if ((speed != COMPHY_SPEED_5_15625G) &&
	     (speed != COMPHY_SPEED_10_3125G) &&
	     (speed != COMPHY_SPEED_DEFAULT)) {
		dev_err(priv->dev, "comphy:%d: unsupported sfi/xfi speed\n",
			comphy->index);
		return -EINVAL;
	}

	hpipe_addr = HPIPE_ADDR(priv->comphy_pipe_regs, comphy->index);
	sd_ip_addr = SD_ADDR(priv->comphy_pipe_regs, comphy->index);
	comphy_addr = COMPHY_ADDR(priv->comphy_regs, comphy->index);

	/* configure phy selector for XFI/SFI */
	mvebu_cp110_comphy_set_phy_selector(priv, comphy);

	dev_dbg(priv->dev, "stage: RFU configurations - hard reset comphy\n");
	/* RFU configurations - hard reset comphy */
	mask = COMMON_PHY_CFG1_PWR_UP_MASK;
	data = 0x1 << COMMON_PHY_CFG1_PWR_UP_OFFSET;
	mask |= COMMON_PHY_CFG1_PIPE_SELECT_MASK;
	data |= 0x0 << COMMON_PHY_CFG1_PIPE_SELECT_OFFSET;
	reg_set(comphy_addr + COMMON_PHY_CFG1_REG, data, mask);

	/* Make sure that 40 data bits is disabled
	 * This bit is not cleared by reset
	 */
	mask = COMMON_PHY_CFG6_IF_40_SEL_MASK;
	data = 0 << COMMON_PHY_CFG6_IF_40_SEL_OFFSET;
	reg_set(comphy_addr + COMMON_PHY_CFG6_REG, data, mask);

	/* Select Baud Rate of Comphy And PD_PLL/Tx/Rx */
	mask = SD_EXTERNAL_CONFIG0_SD_PU_PLL_MASK;
	data = 0x0 << SD_EXTERNAL_CONFIG0_SD_PU_PLL_OFFSET;
	mask |= SD_EXTERNAL_CONFIG0_SD_PHY_GEN_RX_MASK;
	data |= 0xE << SD_EXTERNAL_CONFIG0_SD_PHY_GEN_RX_OFFSET;
	mask |= SD_EXTERNAL_CONFIG0_SD_PHY_GEN_TX_MASK;
	data |= 0xE << SD_EXTERNAL_CONFIG0_SD_PHY_GEN_TX_OFFSET;
	mask |= SD_EXTERNAL_CONFIG0_SD_PU_RX_MASK;
	data |= 0 << SD_EXTERNAL_CONFIG0_SD_PU_RX_OFFSET;
	mask |= SD_EXTERNAL_CONFIG0_SD_PU_TX_MASK;
	data |= 0 << SD_EXTERNAL_CONFIG0_SD_PU_TX_OFFSET;
	mask |= SD_EXTERNAL_CONFIG0_HALF_BUS_MODE_MASK;
	data |= 0 << SD_EXTERNAL_CONFIG0_HALF_BUS_MODE_OFFSET;
	reg_set(sd_ip_addr + SD_EXTERNAL_CONFIG0_REG, data, mask);

	/* release from hard reset */
	mask = SD_EXTERNAL_CONFIG1_RESET_IN_MASK;
	data = 0x0 << SD_EXTERNAL_CONFIG1_RESET_IN_OFFSET;
	mask |= SD_EXTERNAL_CONFIG1_RESET_CORE_MASK;
	data |= 0x0 << SD_EXTERNAL_CONFIG1_RESET_CORE_OFFSET;
	mask |= SD_EXTERNAL_CONFIG1_RF_RESET_IN_MASK;
	data |= 0x0 << SD_EXTERNAL_CONFIG1_RF_RESET_IN_OFFSET;
	reg_set(sd_ip_addr + SD_EXTERNAL_CONFIG1_REG, data, mask);

	mask = SD_EXTERNAL_CONFIG1_RESET_IN_MASK;
	data = 0x1 << SD_EXTERNAL_CONFIG1_RESET_IN_OFFSET;
	mask |= SD_EXTERNAL_CONFIG1_RESET_CORE_MASK;
	data |= 0x1 << SD_EXTERNAL_CONFIG1_RESET_CORE_OFFSET;
	reg_set(sd_ip_addr + SD_EXTERNAL_CONFIG1_REG, data, mask);


	/* Wait 1ms - until band gap and ref clock ready */
	mdelay(1);

	/* Start comphy Configuration */
	dev_dbg(priv->dev, "stage: Comphy configuration\n");
	/* set reference clock */
	mask = HPIPE_MISC_ICP_FORCE_MASK;
	data = (speed == COMPHY_SPEED_5_15625G) ?
		(0x0 << HPIPE_MISC_ICP_FORCE_OFFSET) :
		(0x1 << HPIPE_MISC_ICP_FORCE_OFFSET);
	mask |= HPIPE_MISC_REFCLK_SEL_MASK;
	data |= 0x0 << HPIPE_MISC_REFCLK_SEL_OFFSET;
	reg_set(hpipe_addr + HPIPE_MISC_REG, data, mask);
	/* Power and PLL Control */
	mask = HPIPE_PWR_PLL_REF_FREQ_MASK;
	data = 0x1 << HPIPE_PWR_PLL_REF_FREQ_OFFSET;
	mask |= HPIPE_PWR_PLL_PHY_MODE_MASK;
	data |= 0x4 << HPIPE_PWR_PLL_PHY_MODE_OFFSET;
	reg_set(hpipe_addr + HPIPE_PWR_PLL_REG, data, mask);
	/* Loopback register */
	mask = HPIPE_LOOPBACK_SEL_MASK;
	data = 0x1 << HPIPE_LOOPBACK_SEL_OFFSET;
	reg_set(hpipe_addr + HPIPE_LOOPBACK_REG, data, mask);
	/* rx control 1 */
	mask = HPIPE_RX_CONTROL_1_RXCLK2X_SEL_MASK;
	data = 0x1 << HPIPE_RX_CONTROL_1_RXCLK2X_SEL_OFFSET;
	mask |= HPIPE_RX_CONTROL_1_CLK8T_EN_MASK;
	data |= 0x1 << HPIPE_RX_CONTROL_1_CLK8T_EN_OFFSET;
	reg_set(hpipe_addr + HPIPE_RX_CONTROL_1_REG, data, mask);
	/* DTL Control */
	mask = HPIPE_PWR_CTR_DTL_FLOOP_EN_MASK;
	data = 0x1 << HPIPE_PWR_CTR_DTL_FLOOP_EN_OFFSET;
	reg_set(hpipe_addr + HPIPE_PWR_CTR_DTL_REG, data, mask);

	/* Transmitter/Receiver Speed Divider Force */
	if (speed == COMPHY_SPEED_5_15625G) {
		mask = HPIPE_SPD_DIV_FORCE_RX_SPD_DIV_MASK;
		data = 1 << HPIPE_SPD_DIV_FORCE_RX_SPD_DIV_OFFSET;
		mask |= HPIPE_SPD_DIV_FORCE_RX_SPD_DIV_FORCE_MASK;
		data |= 1 << HPIPE_SPD_DIV_FORCE_RX_SPD_DIV_FORCE_OFFSET;
		mask |= HPIPE_SPD_DIV_FORCE_TX_SPD_DIV_MASK;
		data |= 1 << HPIPE_SPD_DIV_FORCE_TX_SPD_DIV_OFFSET;
		mask |= HPIPE_SPD_DIV_FORCE_TX_SPD_DIV_FORCE_MASK;
		data |= 1 << HPIPE_SPD_DIV_FORCE_TX_SPD_DIV_FORCE_OFFSET;
	} else {
		mask = HPIPE_TXDIGCK_DIV_FORCE_MASK;
		data = 0x1 << HPIPE_TXDIGCK_DIV_FORCE_OFFSET;
	}
	reg_set(hpipe_addr + HPIPE_SPD_DIV_FORCE_REG, data, mask);

	/* Set analog parameters from ETP(HW) */
	dev_dbg(priv->dev, "stage: Analog parameters from ETP(HW)\n");
	/* SERDES External Configuration 2 */
	mask = SD_EXTERNAL_CONFIG2_PIN_DFE_EN_MASK;
	data = 0x1 << SD_EXTERNAL_CONFIG2_PIN_DFE_EN_OFFSET;
	reg_set(sd_ip_addr + SD_EXTERNAL_CONFIG2_REG, data, mask);
	/* 0x7-DFE Resolution control */
	mask = HPIPE_DFE_RES_FORCE_MASK;
	data = 0x1 << HPIPE_DFE_RES_FORCE_OFFSET;
	reg_set(hpipe_addr + HPIPE_DFE_REG0, data, mask);
	/* 0xd-G1_Setting_0 */
	if (speed == COMPHY_SPEED_5_15625G) {
		mask = HPIPE_G1_SET_0_G1_TX_EMPH1_MASK;
		data = 0x6 << HPIPE_G1_SET_0_G1_TX_EMPH1_OFFSET;
	} else {
		mask = HPIPE_G1_SET_0_G1_TX_AMP_MASK;
		data = 0x1c << HPIPE_G1_SET_0_G1_TX_AMP_OFFSET;
		mask |= HPIPE_G1_SET_0_G1_TX_EMPH1_MASK;
		data |= 0xe << HPIPE_G1_SET_0_G1_TX_EMPH1_OFFSET;
	}
	reg_set(hpipe_addr + HPIPE_G1_SET_0_REG, data, mask);
	/* Genration 1 setting 2 (G1_Setting_2) */
	mask = HPIPE_G1_SET_2_G1_TX_EMPH0_MASK;
	data = 0x0 << HPIPE_G1_SET_2_G1_TX_EMPH0_OFFSET;
	mask |= HPIPE_G1_SET_2_G1_TX_EMPH0_EN_MASK;
	data |= 0x1 << HPIPE_G1_SET_2_G1_TX_EMPH0_EN_OFFSET;
	reg_set(hpipe_addr + HPIPE_G1_SET_2_REG, data, mask);
	/* Transmitter Slew Rate Control register (tx_reg1) */
	mask = HPIPE_TX_REG1_TX_EMPH_RES_MASK;
	data = 0x3 << HPIPE_TX_REG1_TX_EMPH_RES_OFFSET;
	mask |= HPIPE_TX_REG1_SLC_EN_MASK;
	data |= 0x3f << HPIPE_TX_REG1_SLC_EN_OFFSET;
	reg_set(hpipe_addr + HPIPE_TX_REG1_REG, data, mask);
	/* Impedance Calibration Control register (cal_reg1) */
	mask = HPIPE_CAL_REG_1_EXT_TXIMP_MASK;
	data = 0xe << HPIPE_CAL_REG_1_EXT_TXIMP_OFFSET;
	mask |= HPIPE_CAL_REG_1_EXT_TXIMP_EN_MASK;
	data |= 0x1 << HPIPE_CAL_REG_1_EXT_TXIMP_EN_OFFSET;
	reg_set(hpipe_addr + HPIPE_CAL_REG1_REG, data, mask);
	/* Generation 1 Setting 5 (g1_setting_5) */
	mask = HPIPE_G1_SETTING_5_G1_ICP_MASK;
	data = 0 << HPIPE_CAL_REG_1_EXT_TXIMP_OFFSET;
	reg_set(hpipe_addr + HPIPE_G1_SETTING_5_REG, data, mask);

	/* 0xE-G1_Setting_1 */
	mask = HPIPE_G1_SET_1_G1_RX_DFE_EN_MASK;
	data = 0x1 << HPIPE_G1_SET_1_G1_RX_DFE_EN_OFFSET;
	if (speed == COMPHY_SPEED_5_15625G) {
		mask |= HPIPE_G1_SET_1_G1_RX_SELMUPI_MASK;
		data |= 0x1 << HPIPE_G1_SET_1_G1_RX_SELMUPI_OFFSET;
		mask |= HPIPE_G1_SET_1_G1_RX_SELMUPP_MASK;
		data |= 0x1 << HPIPE_G1_SET_1_G1_RX_SELMUPP_OFFSET;
	} else {
		mask |= HPIPE_G1_SET_1_G1_RX_SELMUPI_MASK;
		data |= 0x2 << HPIPE_G1_SET_1_G1_RX_SELMUPI_OFFSET;
		mask |= HPIPE_G1_SET_1_G1_RX_SELMUPP_MASK;
		data |= 0x2 << HPIPE_G1_SET_1_G1_RX_SELMUPP_OFFSET;
		mask |= HPIPE_G1_SET_1_G1_RX_SELMUFI_MASK;
		data |= 0x0 << HPIPE_G1_SET_1_G1_RX_SELMUFI_OFFSET;
		mask |= HPIPE_G1_SET_1_G1_RX_SELMUFF_MASK;
		data |= 0x1 << HPIPE_G1_SET_1_G1_RX_SELMUFF_OFFSET;
		mask |= HPIPE_G1_SET_1_G1_RX_DIGCK_DIV_MASK;
		data |= 0x3 << HPIPE_G1_SET_1_G1_RX_DIGCK_DIV_OFFSET;
	}
	reg_set(hpipe_addr + HPIPE_G1_SET_1_REG, data, mask);

	/* 0xA-DFE_Reg3 */
	mask = HPIPE_DFE_F3_F5_DFE_EN_MASK;
	data = 0x0 << HPIPE_DFE_F3_F5_DFE_EN_OFFSET;
	mask |= HPIPE_DFE_F3_F5_DFE_CTRL_MASK;
	data |= 0x0 << HPIPE_DFE_F3_F5_DFE_CTRL_OFFSET;
	reg_set(hpipe_addr + HPIPE_DFE_F3_F5_REG, data, mask);

	/* 0x111-G1_Setting_4 */
	mask = HPIPE_G1_SETTINGS_4_G1_DFE_RES_MASK;
	data = 0x1 << HPIPE_G1_SETTINGS_4_G1_DFE_RES_OFFSET;
	reg_set(hpipe_addr + HPIPE_G1_SETTINGS_4_REG, data, mask);
	/* Genration 1 setting 3 (G1_Setting_3) */
	mask = HPIPE_G1_SETTINGS_3_G1_FBCK_SEL_MASK;
	data = 0x1 << HPIPE_G1_SETTINGS_3_G1_FBCK_SEL_OFFSET;
	if (speed == COMPHY_SPEED_5_15625G) {
		/* Force FFE (Feed Forward Equalization) to 5G */
		mask |= HPIPE_G1_SETTINGS_3_G1_FFE_CAP_SEL_MASK;
		data |= 0xf << HPIPE_G1_SETTINGS_3_G1_FFE_CAP_SEL_OFFSET;
		mask |= HPIPE_G1_SETTINGS_3_G1_FFE_RES_SEL_MASK;
		data |= 0x4 << HPIPE_G1_SETTINGS_3_G1_FFE_RES_SEL_OFFSET;
		mask |= HPIPE_G1_SETTINGS_3_G1_FFE_SETTING_FORCE_MASK;
		data |= 0x1 << HPIPE_G1_SETTINGS_3_G1_FFE_SETTING_FORCE_OFFSET;
	}
	reg_set(hpipe_addr + HPIPE_G1_SETTINGS_3_REG, data, mask);

	/* Connfigure RX training timer */
	mask = HPIPE_RX_TRAIN_TIMER_MASK;
	data = 0x13 << HPIPE_RX_TRAIN_TIMER_OFFSET;
	reg_set(hpipe_addr + HPIPE_TX_TRAIN_CTRL_5_REG, data, mask);

	/* Enable TX train peak to peak hold */
	mask = HPIPE_TX_TRAIN_P2P_HOLD_MASK;
	data = 0x1 << HPIPE_TX_TRAIN_P2P_HOLD_OFFSET;
	reg_set(hpipe_addr + HPIPE_TX_TRAIN_CTRL_0_REG, data, mask);

	/* Configure TX preset index */
	mask = HPIPE_TX_PRESET_INDEX_MASK;
	data = 0x2 << HPIPE_TX_PRESET_INDEX_OFFSET;
	reg_set(hpipe_addr + HPIPE_TX_PRESET_INDEX_REG, data, mask);

	/* Disable pattern lock lost timeout */
	mask = HPIPE_PATTERN_LOCK_LOST_TIMEOUT_EN_MASK;
	data = 0x0 << HPIPE_PATTERN_LOCK_LOST_TIMEOUT_EN_OFFSET;
	reg_set(hpipe_addr + HPIPE_FRAME_DETECT_CTRL_3_REG, data, mask);

	/* Configure TX training pattern and TX training 16bit auto */
	mask = HPIPE_TX_TRAIN_16BIT_AUTO_EN_MASK;
	data = 0x1 << HPIPE_TX_TRAIN_16BIT_AUTO_EN_OFFSET;
	mask |= HPIPE_TX_TRAIN_PAT_SEL_MASK;
	data |= 0x1 << HPIPE_TX_TRAIN_PAT_SEL_OFFSET;
	reg_set(hpipe_addr + HPIPE_TX_TRAIN_REG, data, mask);

	/* Configure Training patten number */
	mask = HPIPE_TRAIN_PAT_NUM_MASK;
	data = 0x88 << HPIPE_TRAIN_PAT_NUM_OFFSET;
	reg_set(hpipe_addr + HPIPE_FRAME_DETECT_CTRL_0_REG, data, mask);

	/* Configure differencial manchester encoter to ethernet mode */
	mask = HPIPE_DME_ETHERNET_MODE_MASK;
	data = 0x1 << HPIPE_DME_ETHERNET_MODE_OFFSET;
	reg_set(hpipe_addr + HPIPE_DME_REG, data, mask);

	/* Configure VDD Continuous Calibration */
	mask = HPIPE_CAL_VDD_CONT_MODE_MASK;
	data = 0x1 << HPIPE_CAL_VDD_CONT_MODE_OFFSET;
	reg_set(hpipe_addr + HPIPE_VDD_CAL_0_REG, data, mask);

	/* Trigger sampler enable pulse (by toggleing the bit) */
	mask = HPIPE_RX_SAMPLER_OS_GAIN_MASK;
	data = 0x3 << HPIPE_RX_SAMPLER_OS_GAIN_OFFSET;
	mask |= HPIPE_SMAPLER_MASK;
	data |= 0x1 << HPIPE_SMAPLER_OFFSET;
	reg_set(hpipe_addr + HPIPE_SAMPLER_N_PROC_CALIB_CTRL_REG, data, mask);
	mask = HPIPE_SMAPLER_MASK;
	data = 0x0 << HPIPE_SMAPLER_OFFSET;
	reg_set(hpipe_addr + HPIPE_SAMPLER_N_PROC_CALIB_CTRL_REG, data, mask);

	/* Set External RX Regulator Control */
	mask = HPIPE_EXT_SELLV_RXSAMPL_MASK;
	data = 0x1A << HPIPE_EXT_SELLV_RXSAMPL_OFFSET;
	reg_set(hpipe_addr + HPIPE_VDD_CAL_CTRL_REG, data, mask);

	dev_dbg(priv->dev, "stage: RFU configurations- Power Up PLL,Tx,Rx\n");
	/* SERDES External Configuration */
	mask = SD_EXTERNAL_CONFIG0_SD_PU_PLL_MASK;
	data = 0x1 << SD_EXTERNAL_CONFIG0_SD_PU_PLL_OFFSET;
	mask |= SD_EXTERNAL_CONFIG0_SD_PU_RX_MASK;
	data |= 0x1 << SD_EXTERNAL_CONFIG0_SD_PU_RX_OFFSET;
	mask |= SD_EXTERNAL_CONFIG0_SD_PU_TX_MASK;
	data |= 0x1 << SD_EXTERNAL_CONFIG0_SD_PU_TX_OFFSET;
	reg_set(sd_ip_addr + SD_EXTERNAL_CONFIG0_REG, data, mask);

	/* check PLL rx & tx ready */
	addr = sd_ip_addr + SD_EXTERNAL_STATUS0_REG;
	data = SD_EXTERNAL_STATUS0_PLL_RX_MASK | SD_EXTERNAL_STATUS0_PLL_TX_MASK;
	mask = data;
	data = polling_with_timeout(addr, data, mask, 15000, REG_32BIT);
	if (data != 0) {
		if (data & SD_EXTERNAL_STATUS0_PLL_RX_MASK)
			dev_err(priv->dev, "RX PLL is not locked\n");
		if (data & SD_EXTERNAL_STATUS0_PLL_TX_MASK)
			dev_err(priv->dev, "TX PLL is not locked\n");

		ret = -ETIMEDOUT;
	}

	/* RX init */
	mask = SD_EXTERNAL_CONFIG1_RX_INIT_MASK;
	data = 0x1 << SD_EXTERNAL_CONFIG1_RX_INIT_OFFSET;
	reg_set(sd_ip_addr + SD_EXTERNAL_CONFIG1_REG, data, mask);

	/* check that RX init done */
	addr = sd_ip_addr + SD_EXTERNAL_STATUS0_REG;
	data = SD_EXTERNAL_STATUS0_RX_INIT_MASK;
	mask = data;
	data = polling_with_timeout(addr, data, mask, 100, REG_32BIT);
	if (data != 0) {
		dev_err(priv->dev, "RX init failed\n");
		ret = -ETIMEDOUT;
	}

	dev_dbg(priv->dev, "stage: RF Reset\n");
	/* RF Reset */
	mask =  SD_EXTERNAL_CONFIG1_RX_INIT_MASK;
	data = 0x0 << SD_EXTERNAL_CONFIG1_RX_INIT_OFFSET;
	mask |= SD_EXTERNAL_CONFIG1_RF_RESET_IN_MASK;
	data |= 0x1 << SD_EXTERNAL_CONFIG1_RF_RESET_IN_OFFSET;
	reg_set(sd_ip_addr + SD_EXTERNAL_CONFIG1_REG, data, mask);

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

static int mvebu_cp110_comphy_power_on(struct phy *phy)
{
	struct mvebu_comphy *comphy = phy_get_drvdata(phy);
	struct mvebu_comphy_priv *priv = to_mvebu_comphy_priv(comphy);
	int mode = COMPHY_GET_MODE(priv->lanes[comphy->index].mode);
	int err = 0;

	dev_dbg(priv->dev, "%s: Enter\n", __func__);

	spin_lock(&priv->lock);

	switch (mode) {
	case(COMPHY_SATA_MODE):
		err = mvebu_cp110_comphy_sata_power_on(priv, comphy);
		break;

	case(COMPHY_SGMII_MODE):
	case(COMPHY_HS_SGMII_MODE):
		err = mvebu_cp110_comphy_sgmii_power_on(priv, comphy);
		break;

	case (COMPHY_USB3H_MODE):
	case (COMPHY_USB3D_MODE):
		err = mvebu_cp110_comphy_usb3_power_on(priv, comphy);
		break;

	case (COMPHY_PCIE_MODE):
		err = mvebu_cp110_comphy_pcie_power_on(priv, comphy);
		break;

	case (COMPHY_RXAUI_MODE):
		err = mvebu_cp110_comphy_rxaui_power_on(priv, comphy);
		break;
	/* From comphy perspective, XFI and SFI are the same */
	case (COMPHY_XFI_MODE):
	case (COMPHY_SFI_MODE):
		err = mvebu_cp110_comphy_xfi_power_on(priv, comphy);
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
	int mode = COMPHY_GET_MODE(priv->lanes[comphy->index].mode);
	void __iomem *sd_ip_addr;
	u32 mask, data;

	dev_dbg(priv->dev, "%s: Enter\n", __func__);

	if (comphy->skip_pcie_power_off) {
		dev_dbg(priv->dev, "skip_pcie_power_off indicated, so avoid powering off lane\n");
		goto exit;
	}

	spin_lock(&priv->lock);

	switch (mode) {
	case(COMPHY_SGMII_MODE):
	case(COMPHY_HS_SGMII_MODE):
	case(COMPHY_RXAUI_MODE):
	case(COMPHY_XFI_MODE):
	case(COMPHY_SFI_MODE):
		sd_ip_addr = SD_ADDR(priv->comphy_pipe_regs, comphy->index);

		/* Hard reset the comphy */
		mask = SD_EXTERNAL_CONFIG1_RESET_IN_MASK;
		data = 0x0 << SD_EXTERNAL_CONFIG1_RESET_IN_OFFSET;
		mask |= SD_EXTERNAL_CONFIG1_RESET_CORE_MASK;
		data |= 0x0 << SD_EXTERNAL_CONFIG1_RESET_CORE_OFFSET;
		mask |= SD_EXTERNAL_CONFIG1_RF_RESET_IN_MASK;
		data |= 0x0 << SD_EXTERNAL_CONFIG1_RF_RESET_IN_OFFSET;
		reg_set(sd_ip_addr + SD_EXTERNAL_CONFIG1_REG, data, mask);
		break;
	default:
		dev_dbg(priv->dev, "comphy%d: power down is not implemented for 0x%x mode\n",
			comphy->index, mode);
		break;
	}

	/* Clear comphy PHY and PIPE selector, can't rely on u-boot */
	mvebu_cp110_comphy_clr_phy_selector(priv, comphy);
	mvebu_cp110_comphy_clr_pipe_selector(priv, comphy);

	spin_unlock(&priv->lock);

exit:
	dev_dbg(priv->dev, "%s: Exit\n", __func__);
	return 0;
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
		priv->lanes[comphy->index].misc.pcie_width = pcie_width;
		break;
	/* The following command is to indicate the PCIe works in endpoint mode
	 * which is from PCIe host driver.
	 */
	case COMPHY_COMMAND_PCIE_IS_EP:
		priv->lanes[comphy->index].misc.pcie_is_ep = true;
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
	void __iomem *sd_ip_addr, *addr;
	u32 mask, data;
	int ret = 0;

	sd_ip_addr = SD_ADDR(priv->comphy_pipe_regs, comphy->index);

	addr = sd_ip_addr + SD_EXTERNAL_STATUS0_REG;
	data = SD_EXTERNAL_STATUS0_PLL_TX_MASK & SD_EXTERNAL_STATUS0_PLL_RX_MASK;
	mask = data;
	data = polling_with_timeout(addr, data, mask, PLL_LOCK_TIMEOUT, REG_32BIT);
	if (data != 0) {
		if (data & SD_EXTERNAL_STATUS0_PLL_RX_MASK)
			dev_err(priv->dev, "RX PLL is not locked\n");
		if (data & SD_EXTERNAL_STATUS0_PLL_TX_MASK)
			dev_err(priv->dev, "TX PLL is not locked\n");

		ret = -ETIMEDOUT;
	}

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
		{COMPHY_UNUSED, COMPHY_SGMII0, COMPHY_HS_SGMII0, COMPHY_SGMII1,
		 COMPHY_HS_SGMII1, COMPHY_RXAUI0, COMPHY_XFI, COMPHY_SFI,
		 COMPHY_USB3H1, COMPHY_USB3D0, COMPHY_PCIE1},
		/* Lane 5 */
		{COMPHY_UNUSED, COMPHY_RXAUI1, COMPHY_SGMII2, COMPHY_HS_SGMII2,
		 COMPHY_SATA1, COMPHY_PCIE2},
	},
	.comphy_ops = &cp110_comphy_ops,
};

