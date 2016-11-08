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
#include <dt-bindings/phy/phy-mvebu-comphy.h>

#include "phy-mvebu-comphy.h"

#define MVEBU_COMPHY_MAX_CNT	6
#define MVEBU_COMPHY_FUNC_MAX	11

#define to_mvebu_comphy_priv(lane) \
	container_of((lane), struct mvebu_comphy_priv, lanes[(lane)->index])

struct mvebu_comphy_priv {
	struct device *dev;
	void __iomem *comphy_regs;
	void __iomem *comphy_pipe_regs;
	spinlock_t lock;
	const struct mvebu_comphy_soc_info *sinfo;
	struct mvebu_comphy {
		struct phy *phy;
		int mode;
		int index;
	} lanes[MVEBU_COMPHY_MAX_CNT];
};

struct mvebu_comphy_soc_info {
	int num_of_lanes;
	int functions[MVEBU_COMPHY_MAX_CNT][MVEBU_COMPHY_FUNC_MAX];
};

/* For CP-110 there are 2 Selector registers "PHY Selectors"
 * and "PIPE Selectors".
 *	- PIPE selector include USB and PCIe options.
 *	- PHY selector include the Ethernet and SATA options.
 *	  every Ethernet option has different options,
 *	  for example: serdes lane2 had option Eth_port_0 that include
 *	  (SGMII0, XAUI0, RXAUI0, XFI, SFI)
 */
static const struct mvebu_comphy_soc_info cp110_comphy = {
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
};

static void __maybe_unused reg_set(void __iomem *addr, u32 data, u32 mask)
{
	u32 reg_data;

	reg_data = readl(addr);
	reg_data &= ~mask;
	reg_data |= data;
	writel(reg_data, addr);
}

u32 __maybe_unused polling_with_timeout(void __iomem *addr, u32 val, u32 mask,
					unsigned long usec_timout)
{
	u32 data;

	do {
		udelay(1);
		data = readl(addr) & mask;
	} while (data != val  && --usec_timout > 0);

	if (usec_timout == 0)
		return data;

	return 0;
}

/* PHY selector configures SATA and Network modes */
static void mvebu_comphy_set_phy_selector(struct mvebu_comphy_priv *priv,
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
			  * 0x1 = SGMII/HS-SGMII Port2
			  * 0x2 = SGMII/HS-SGMII Port1: XFI/SFI, RXAUI_Lane0
			  */
			if (priv->lanes[comphy->index].mode == COMPHY_SGMII2 ||
			    priv->lanes[comphy->index].mode == COMPHY_HS_SGMII2)
				reg |= COMMON_SELECTOR_COMPHY4_SGMII2 << comphy_offset;
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

static int mvebu_comphy_sata_power_on(struct mvebu_comphy_priv *priv,
				      struct mvebu_comphy *comphy)
{
	dev_dbg(priv->dev, "%s: Enter\n", __func__);

	dev_err(priv->dev, "SATA mode is not implemented\n");

	/* configure phy selector for SATA */
	mvebu_comphy_set_phy_selector(priv, comphy);

	dev_dbg(priv->dev, "%s: Exit\n", __func__);

	return -ENOTSUPP;
}

static int mvebu_comphy_sgmii_power_on(struct mvebu_comphy_priv *priv,
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
	mvebu_comphy_set_phy_selector(priv, comphy);

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
	data = polling_with_timeout(addr, data, mask, 15000);
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
	data = polling_with_timeout(addr, data, mask, 100);
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

static int mvebu_comphy_usb3_power_on(struct mvebu_comphy_priv *priv,
				      struct mvebu_comphy *comphy)
{
	dev_dbg(priv->dev, "%s: Enter\n", __func__);

	dev_err(priv->dev, "USB mode is not implemented\n");

	dev_dbg(priv->dev, "%s: Exit\n", __func__);

	return -ENOTSUPP;
}

static int mvebu_comphy_pcie_power_on(struct mvebu_comphy_priv *priv,
				      struct mvebu_comphy *comphy)
{
	dev_dbg(priv->dev, "%s: Enter\n", __func__);

	dev_err(priv->dev, "PCIE mode is not implemented\n");

	dev_dbg(priv->dev, "%s: Exit\n", __func__);

	return -ENOTSUPP;
}

static int mvebu_comphy_rxaui_power_on(struct mvebu_comphy_priv *priv,
				       struct mvebu_comphy *comphy)
{
	dev_dbg(priv->dev, "%s: Enter\n", __func__);

	dev_err(priv->dev, "RXAUI mode is not implemented\n");

	/* configure phy selector for RXAUI */
	mvebu_comphy_set_phy_selector(priv, comphy);

	dev_dbg(priv->dev, "%s: Exit\n", __func__);

	return -ENOTSUPP;
}

static int mvebu_comphy_xfi_power_on(struct mvebu_comphy_priv *priv,
				     struct mvebu_comphy *comphy)
{
	void __iomem *hpipe_addr, *sd_ip_addr, *comphy_addr, *addr;
	u32 mask, data;
	int ret = 0;

	dev_dbg(priv->dev, "%s: Enter\n", __func__);

	hpipe_addr = HPIPE_ADDR(priv->comphy_pipe_regs, comphy->index);
	sd_ip_addr = SD_ADDR(priv->comphy_pipe_regs, comphy->index);
	comphy_addr = COMPHY_ADDR(priv->comphy_regs, comphy->index);

	/* configure phy selector for XFI/SFI */
	mvebu_comphy_set_phy_selector(priv, comphy);

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
	data = 0x1 << HPIPE_MISC_ICP_FORCE_OFFSET;
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
	mask = HPIPE_G1_SET_0_G1_TX_AMP_MASK;
	data = 0x1c << HPIPE_G1_SET_0_G1_TX_AMP_OFFSET;
	mask |= HPIPE_G1_SET_0_G1_TX_EMPH1_MASK;
	data |= 0xe << HPIPE_G1_SET_0_G1_TX_EMPH1_OFFSET;
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
	mask = HPIPE_G1_SET_1_G1_RX_SELMUPI_MASK;
	data = 0x1 << HPIPE_G1_SET_1_G1_RX_SELMUPI_OFFSET;
	mask |= HPIPE_G1_SET_1_G1_RX_SELMUPP_MASK;
	data |= 0x1 << HPIPE_G1_SET_1_G1_RX_SELMUPP_OFFSET;
	mask |= HPIPE_G1_SET_1_G1_RX_DFE_EN_MASK;
	data |= 0x1 << HPIPE_G1_SET_1_G1_RX_DFE_EN_OFFSET;
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
	reg_set(hpipe_addr + HPIPE_G1_SETTINGS_3_REG, data, mask);

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
	data = polling_with_timeout(addr, data, mask, 15000);
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
	data = polling_with_timeout(addr, data, mask, 100);
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

static int mvebu_comphy_power_on(struct phy *phy)
{
	struct mvebu_comphy *comphy = phy_get_drvdata(phy);
	struct mvebu_comphy_priv *priv = to_mvebu_comphy_priv(comphy);
	int mode = COMPHY_GET_MODE(priv->lanes[comphy->index].mode);
	int err = 0;

	dev_dbg(priv->dev, "%s: Enter\n", __func__);

	spin_lock(&priv->lock);

	switch (mode) {
	case(COMPHY_SATA_MODE):
		err = mvebu_comphy_sata_power_on(priv, comphy);
		break;

	case(COMPHY_SGMII_MODE):
	case(COMPHY_HS_SGMII_MODE):
		err = mvebu_comphy_sgmii_power_on(priv, comphy);
		break;

	case (COMPHY_USB3H_MODE):
	case (COMPHY_USB3D_MODE):
		err = mvebu_comphy_usb3_power_on(priv, comphy);
		break;

	case (COMPHY_PCIE_MODE):
		err = mvebu_comphy_pcie_power_on(priv, comphy);
		break;

	case (COMPHY_RXAUI_MODE):
		err = mvebu_comphy_rxaui_power_on(priv, comphy);
		break;
	/* From comphy perspective, XFI and SFI are the same */
	case (COMPHY_XFI_MODE):
	case (COMPHY_SFI_MODE):
		err = mvebu_comphy_xfi_power_on(priv, comphy);
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

static int mvebu_comphy_power_off(struct phy *phy)
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

static struct phy_ops mvebu_comphy_ops = {
	.power_on	= mvebu_comphy_power_on,
	.power_off	= mvebu_comphy_power_off,
	.owner		= THIS_MODULE,
};

static const struct of_device_id mvebu_comphy_of_match[] = {
	{ .compatible = "marvell,cp110-comphy", .data = &cp110_comphy },
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

	if (lane >= (int)priv->sinfo->num_of_lanes) {
		dev_err(dev, "Wrong lane number %d for PHY, max is %d\n",
			lane, priv->sinfo->num_of_lanes);
		return ERR_PTR(-ENODEV);
	}

	for (i = 0; i < MVEBU_COMPHY_FUNC_MAX; i++)
		if (priv->sinfo->functions[lane][i] == mode)
			break;

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
	const struct mvebu_comphy_soc_info *sinfo;
	int i;

	dev_dbg(priv->dev, "%s: Enter\n", __func__);

	match = of_match_device(mvebu_comphy_of_match, &pdev->dev);
	sinfo = match->data;
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

	priv->sinfo = sinfo;
	priv->dev = &pdev->dev;
	spin_lock_init(&priv->lock);

	for (i = 0; i < sinfo->num_of_lanes; i++) {
		struct phy *phy;

		phy = devm_phy_create(&pdev->dev, NULL, &mvebu_comphy_ops);
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
		phy_set_drvdata(phy, &priv->lanes[i]);

		mvebu_comphy_power_off(phy);
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

