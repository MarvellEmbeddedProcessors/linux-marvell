/******************************************************************************
*Copyright (C) Marvell International Ltd. and its affiliates
*
*This software file (the "File") is owned and distributed by Marvell
*International Ltd. and/or its affiliates ("Marvell") under the following
*alternative licensing terms.  Once you have made an election to distribute the
*File under one of the following license alternatives, please (i) delete this
*introductory statement regarding license alternatives, (ii) delete the two
*license alternatives that you have not elected to use and (iii) preserve the
*Marvell copyright notice above.
*
******************************************************************************
*Marvell GPL License Option
*
*If you received this File from Marvell, you may opt to use, redistribute and/or
*modify this File in accordance with the terms and conditions of the General
*Public License Version 2, June 1991 (the "GPL License"), a copy of which is
*available along with the File in the license.txt file or on the worldwide web
*at http://www.gnu.org/licenses/gpl.txt.
*
*THE FILE IS DISTRIBUTED AS-IS, WITHOUT WARRANTY OF ANY KIND, AND THE IMPLIED
*WARRANTIES OF MERCHANTABILITY OR FITNESS FOR A PARTICULAR PURPOSE ARE EXPRESSLY
*DISCLAIMED.  The GPL License provides additional details about this warranty
*disclaimer.
******************************************************************************/

#include <linux/kernel.h>
#include <linux/version.h>
#include <linux/netdevice.h>
#include <linux/etherdevice.h>
#include <linux/platform_device.h>
#include <linux/skbuff.h>
#include <linux/module.h>
#include <linux/inetdevice.h>
#include <linux/interrupt.h>
#include <linux/mbus.h>
#include <asm/setup.h>
#include <linux/list.h>
#include <linux/firmware.h>
#include <linux/io.h>

#include "mv_net_complex_a39x.h"

static u32 mv_net_complex_vbase_addr;
static u32 mv_net_complex_misc_vbase_addr;
static u32 mv_net_complex_phy_vbase_addr;
static u32 mv_net_reg_virt_base;
static u32 mv_net_dfx_vbase_addr;

static void mv_net_assert_load_config(void)
{
	u32 reg;

	reg = readl((void *)MV_NETCOMP_SYSTEM_SOFT_RESET);
	reg &= ~NETC_NSS_SRAM_LOAD_CONF_MASK;
	reg &= ~NETC_NSS_PPC_LOAD_CONF_MASK;
	reg &= ~NETC_NSS_MACS_LOAD_CONF_MASK;
	reg &= ~NETC_NSS_QM1_LOAD_CONF_MASK;
	writel(reg, (void *)MV_NETCOMP_SYSTEM_SOFT_RESET);

}

static void mv_net_de_assert_load_config(void)
{
	u32 reg;

	reg = readl((void *)MV_NETCOMP_SYSTEM_SOFT_RESET);
	reg |= NETC_NSS_SRAM_LOAD_CONF_MASK;
	reg |= NETC_NSS_PPC_LOAD_CONF_MASK;
	reg |= NETC_NSS_MACS_LOAD_CONF_MASK;
	reg |= NETC_NSS_QM1_LOAD_CONF_MASK;
	writel(reg, (void *)MV_NETCOMP_SYSTEM_SOFT_RESET);
}

static void mv_net_assert_soft_reset(void)
{
	u32 reg;

	reg = readl((void *)MV_NETCOMP_SYSTEM_SOFT_RESET);
	reg &= ~NETC_NSS_SRAM_SOFT_RESET_MASK;
	reg &= ~NETC_NSS_PPC_SOFT_RESET_MASK;
	reg &= ~NETC_NSS_MACS_SOFT_RESET_MASK;
	reg &= ~NETC_NSS_QM1_SOFT_RESET_MASK;
	reg &= ~NETC_NSS_QM2_SOFT_RESET_MASK;

	writel(reg, (void *)MV_NETCOMP_SYSTEM_SOFT_RESET);
}

static void mv_net_de_assert_soft_reset(void)
{
	u32 reg;

	reg = readl((void *)MV_NETCOMP_SYSTEM_SOFT_RESET);
	reg |= NETC_NSS_SRAM_SOFT_RESET_MASK;
	reg |= NETC_NSS_PPC_SOFT_RESET_MASK;
	reg |= NETC_NSS_MACS_SOFT_RESET_MASK;
	reg |= NETC_NSS_QM1_SOFT_RESET_MASK;
	reg |= NETC_NSS_QM2_SOFT_RESET_MASK;

	writel(reg, (void *)MV_NETCOMP_SYSTEM_SOFT_RESET);
}

static void mv_net_pm_clock_down(void)
{
	u32 reg;

	reg = readl((void *)MV_NETCOMP_CLOCK_GATING);

	reg &= ~NETC_CLOCK_GATING_SRAM_X2_MASK;
	reg &= ~NETC_CLOCK_GATING_SRAM_MASK;
	reg &= ~NETC_CLOCK_GATING_PPC_CMAC_MASK;
	reg &= ~NETC_CLOCK_GATING_PPC_PP_MASK;
	reg &= ~NETC_CLOCK_GATING_PPC_NSS_MASK;
	reg &= ~NETC_CLOCK_GATING_CMAC_MASK;
	reg &= ~NETC_CLOCK_GATING_NSS_MASK;
	reg &= ~NETC_CLOCK_GATING_QM2_MASK;
	reg &= ~NETC_CLOCK_GATING_QM1_X2_MASK;
	reg &= ~NETC_CLOCK_GATING_QM1_MASK;

	writel(reg, (void *)MV_NETCOMP_CLOCK_GATING);
}

static void mv_net_pm_clock_up(void)
{
	u32 reg;

	reg = readl((void *)MV_NETCOMP_CLOCK_GATING);

	reg |= NETC_CLOCK_GATING_SRAM_X2_MASK;
	reg |= NETC_CLOCK_GATING_SRAM_MASK;
	reg |= NETC_CLOCK_GATING_PPC_CMAC_MASK;
	reg |= NETC_CLOCK_GATING_PPC_PP_MASK;
	reg |= NETC_CLOCK_GATING_PPC_NSS_MASK;
	reg |= NETC_CLOCK_GATING_CMAC_MASK;
	reg |= NETC_CLOCK_GATING_NSS_MASK;
	reg |= NETC_CLOCK_GATING_QM2_MASK;
	reg |= NETC_CLOCK_GATING_QM1_X2_MASK;
	reg |= NETC_CLOCK_GATING_QM1_MASK;

	writel(reg, (void *)MV_NETCOMP_CLOCK_GATING);
}

static void mv_net_restore_regs_defaults(void)
{
	/* WA for A390 - when NSS wake up from reset
	* registers default values are wrong
	*/

	mv_net_pm_clock_down();
	mv_net_assert_load_config();
	mv_net_pm_clock_up();
	mv_net_pm_clock_down();
	mv_net_de_assert_load_config();
	mv_net_pm_clock_up();
}

void mv_net_complex_nss_select(u32 val)
{
	u32 reg;

	reg = readl((void *)MV_NETCOMP_FUNCTION_ENABLE_CTRL_1);
	reg &= ~NETC_PACKET_PROCESS_MASK;

	val <<= NETC_PACKET_PROCESS_OFFSET;
	val &= NETC_PACKET_PROCESS_MASK;

	reg |= val;

	writel(reg, (void *)MV_NETCOMP_FUNCTION_ENABLE_CTRL_1);
}

static void mv_net_complex_active_port(u32 port, u32 val)
{
	u32 reg;

	reg = readl((void *)MV_NETCOMP_PORTS_CONTROL_1);
	reg &= ~NETC_PORTS_ACTIVE_MASK(port);

	val <<= NETC_PORTS_ACTIVE_OFFSET(port);
	val &= NETC_PORTS_ACTIVE_MASK(port);

	reg |= val;

	writel(reg, (void *)MV_NETCOMP_PORTS_CONTROL_1);
}

static void mv_net_complex_xaui_enable(u32 port, u32 val)
{
	u32 reg;

	reg = readl((void *)MV_NETCOMP_CONTROL_0);
	reg &= ~NETC_CTRL_ENA_XAUI_MASK;

	val <<= NETC_CTRL_ENA_XAUI_OFFSET;
	val &= NETC_CTRL_ENA_XAUI_MASK;

	reg |= val;

	writel(reg, (void *)MV_NETCOMP_CONTROL_0);
}

static void mv_net_complex_rxaui_enable(u32 port, u32 val)
{
	u32 reg;

	reg = readl((void *)MV_NETCOMP_CONTROL_0);
	reg &= ~NETC_CTRL_ENA_RXAUI_MASK;

	val <<= NETC_CTRL_ENA_RXAUI_OFFSET;
	val &= NETC_CTRL_ENA_RXAUI_MASK;

	reg |= val;

	writel(reg, (void *)MV_NETCOMP_CONTROL_0);
}

static void mv_net_complex_gop_reset(u32 val)
{
	u32 reg;

	reg = readl((void *)MV_NETCOMP_SYSTEM_SOFT_RESET);
	reg &= ~NETC_GOP_SOFT_RESET_MASK;

	val <<= NETC_GOP_SOFT_RESET_OFFSET;
	val &= NETC_GOP_SOFT_RESET_MASK;

	reg |= val;

	writel(reg, (void *)MV_NETCOMP_SYSTEM_SOFT_RESET);
}

static void mv_net_complex_gop_clock_logic_set(u32 val)
{
	u32 reg;

	reg = readl((void *)MV_NETCOMP_PORTS_CONTROL_0);
	reg &= ~NETC_CLK_DIV_PHASE_MASK;

	val <<= NETC_CLK_DIV_PHASE_OFFSET;
	val &= NETC_CLK_DIV_PHASE_MASK;

	reg |= val;

	writel(reg, (void *)MV_NETCOMP_PORTS_CONTROL_0);
}

static void mv_net_complex_port_rf_reset(u32 port, u32 val)
{
	u32 reg;

	reg = readl((void *)MV_NETCOMP_PORTS_CONTROL_1);
	reg &= ~NETC_PORT_GIG_RF_RESET_MASK(port);

	val <<= NETC_PORT_GIG_RF_RESET_OFFSET(port);
	val &= NETC_PORT_GIG_RF_RESET_MASK(port);

	reg |= val;

	writel(reg, (void *)MV_NETCOMP_PORTS_CONTROL_1);
}

static void mv_net_complex_gbe_mode_select(u32 val)
{
	u32 reg;

	reg = readl((void *)MV_NETCOMP_CONTROL_0);
	reg &= ~NETC_GBE_PORT1_MODE_MASK;

	val <<= NETC_GBE_PORT1_MODE_OFFSET;
	val &= NETC_GBE_PORT1_MODE_MASK;

	reg |= val;

	writel(reg, (void *)MV_NETCOMP_CONTROL_0);
}

static void mv_net_complex_bus_width_select(u32 val)
{
	u32 reg;

	reg = readl((void *)MV_NETCOMP_PORTS_CONTROL_0);
	reg &= ~NETC_BUS_WIDTH_SELECT_MASK;

	val <<= NETC_BUS_WIDTH_SELECT_OFFSET;
	val &= NETC_BUS_WIDTH_SELECT_MASK;

	reg |= val;

	writel(reg, (void *)MV_NETCOMP_PORTS_CONTROL_0);
}

static void mv_net_complex_sample_stages_timing(u32 val)
{
	u32 reg;

	reg = readl((void *)MV_NETCOMP_PORTS_CONTROL_0);
	reg &= ~NETC_GIG_RX_DATA_SAMPLE_MASK;

	val <<= NETC_GIG_RX_DATA_SAMPLE_OFFSET;
	val &= NETC_GIG_RX_DATA_SAMPLE_MASK;

	reg |= val;

	writel(reg, (void *)MV_NETCOMP_PORTS_CONTROL_0);
}

static void mv_net_complex_com_phy_selector_config(u32 net_complex)
{
	u32 selector = readl((void *)COMMON_PHYS_SELECTORS_REG);

	/* Change the value of the selector from the legacy mode to NSS mode */
	if (net_complex & MV_NETCOMP_GE_MAC0_2_SGMII_L0) {
		selector &= ~COMMON_PHYS_SELECTOR_LANE_MASK(0);
		selector |= 0x4 << COMMON_PHYS_SELECTOR_LANE_OFFSET(0);
	}
	if (net_complex &  MV_NETCOMP_GE_MAC0_2_SGMII_L1) {
		selector &= ~COMMON_PHYS_SELECTOR_LANE_MASK(1);
		selector |= 0x8 << COMMON_PHYS_SELECTOR_LANE_OFFSET(1);
	}
	if (net_complex &  MV_NETCOMP_GE_MAC1_2_SGMII_L1) {
		selector &= ~COMMON_PHYS_SELECTOR_LANE_MASK(1);
		selector |= 0x9 << COMMON_PHYS_SELECTOR_LANE_OFFSET(1);
	}
	if (net_complex &  MV_NETCOMP_GE_MAC1_2_SGMII_L2) {
		selector &= ~COMMON_PHYS_SELECTOR_LANE_MASK(2);
		selector |= 0x5 << COMMON_PHYS_SELECTOR_LANE_OFFSET(2);
	}
	if (net_complex &  MV_NETCOMP_GE_MAC2_2_SGMII_L3) {
		selector &= ~COMMON_PHYS_SELECTOR_LANE_MASK(3);
		selector |= 0x7 << COMMON_PHYS_SELECTOR_LANE_OFFSET(3);
	}
	if (net_complex &  MV_NETCOMP_GE_MAC3_2_SGMII_L4) {
		selector &= ~COMMON_PHYS_SELECTOR_LANE_MASK(4);
		selector |= 0x8 << COMMON_PHYS_SELECTOR_LANE_OFFSET(4);
	}
	if (net_complex &  MV_NETCOMP_GE_MAC2_2_SGMII_L5) {
		selector &= ~COMMON_PHYS_SELECTOR_LANE_MASK(5);
		selector |= 0x6 << COMMON_PHYS_SELECTOR_LANE_OFFSET(5);
	}
	if (net_complex &  MV_NETCOMP_GE_MAC3_2_SGMII_L6) {
		selector &= ~COMMON_PHYS_SELECTOR_LANE_MASK(6);
		selector |= 0x2 << COMMON_PHYS_SELECTOR_LANE_OFFSET(6);
	}
	if (net_complex &  MV_NETCOMP_GE_MAC0_2_SGMII_L6) {
		selector &= ~COMMON_PHYS_SELECTOR_LANE_MASK(6);
		selector |= 0xC << COMMON_PHYS_SELECTOR_LANE_OFFSET(6);
	}

	writel(selector, (void *)COMMON_PHYS_SELECTORS_REG);
}

static void mv_net_complex_qsgmii_ctrl_config(void)
{
	u32 reg;

	/* Reset the QSGMII controller */
	reg = readl((void *)MV_NETCOMP_QSGMII_CTRL_1);
	reg &= ~NETC_QSGMII_CTRL_RSTN_MASK;
	writel(reg, (void *)MV_NETCOMP_QSGMII_CTRL_1);

	/* Set the QSGMII controller to work with NSS */
	reg |= NETC_QSGMII_CTRL_VERSION_MASK;
	reg |= NETC_QSGMII_CTRL_V3ACTIVE_MASK;
	reg &= ~NETC_QSGMII_CTRL_ACTIVE_MASK;
	writel(reg, (void *)MV_NETCOMP_QSGMII_CTRL_1);

	/* De-assert the QSGMII controller */
	reg |= NETC_QSGMII_CTRL_RSTN_MASK;
	writel(reg, (void *)MV_NETCOMP_QSGMII_CTRL_1);
}

static void mv_net_complex_mac_to_rgmii(u32 port, enum mv_net_complex_phase phase)
{
	switch (phase) {
	case MV_NETC_FIRST_PHASE:
		/* Set Bus Width to HB mode = 1 */
		mv_net_complex_bus_width_select(1);
		/* Select RGMII mode */
		mv_net_complex_gbe_mode_select(MV_NETC_GBE1_RGMII);
		break;
	case MV_NETC_SECOND_PHASE:
		/* De-assert the relevant port HB reset */
		mv_net_complex_port_rf_reset(port, 1);
		break;
	}
}

static void mv_net_complex_mac_to_qsgmii(u32 port, enum mv_net_complex_phase phase)
{
	switch (phase) {
	case MV_NETC_FIRST_PHASE:
		/* Set Bus Width to FB mode = 0 */
		mv_net_complex_bus_width_select(0);
		/* Select SGMII mode */
		mv_net_complex_gbe_mode_select(MV_NETC_GBE1_SGMII);
		/* Configure the sample stages */
		mv_net_complex_sample_stages_timing(0);
		/* config QSGMII */
		mv_net_complex_qsgmii_ctrl_config();
		break;
	case MV_NETC_SECOND_PHASE:
		/* De-assert the relevant port HB reset */
		mv_net_complex_port_rf_reset(port, 1);
		break;
	}
}

static void mv_net_complex_mac_to_sgmii(u32 port,
					enum mv_net_complex_phase phase,
					u32 net_complex)
{
	switch (phase) {
	case MV_NETC_FIRST_PHASE:
		/* Set Bus Width to HB mode = 1 */
		mv_net_complex_bus_width_select(1);
		/* Select SGMII mode */
		if (port == 1)
			mv_net_complex_gbe_mode_select(MV_NETC_GBE1_SGMII);
		/* Configure the sample stages */
		mv_net_complex_sample_stages_timing(0);
		/* Configure the ComPhy Selector */
		mv_net_complex_com_phy_selector_config(net_complex);
		break;
	case MV_NETC_SECOND_PHASE:
		/* De-assert the relevant port HB reset */
		mv_net_complex_port_rf_reset(port, 1);
		break;
	}
}

static void mv_net_complex_mac_to_rxaui(u32 port, enum mv_net_complex_phase phase)
{
	switch (phase) {
	case MV_NETC_FIRST_PHASE:
		/* RXAUI Serdes/s Clock alignment */
		mv_net_complex_rxaui_enable(port, 1);
		break;
	case MV_NETC_SECOND_PHASE:
		/* De-assert the relevant port HB reset */
		mv_net_complex_port_rf_reset(port, 1);
		break;
	}
}

static void mv_net_complex_mac_to_xaui(u32 port, enum mv_net_complex_phase phase)
{
	switch (phase) {
	case MV_NETC_FIRST_PHASE:
		/* RXAUI Serdes/s Clock alignment */
		mv_net_complex_xaui_enable(port, 1);
		break;
	case MV_NETC_SECOND_PHASE:
		/* De-assert the relevant port HB reset */
		mv_net_complex_port_rf_reset(port, 1);
		break;
	}
}

/* build net complex init mask according to board configuration */
static int mv_net_mask_init(u32 selector)
{
	u32 net_mask = 0;
	int lane, xaui_lanes;

	xaui_lanes = 0;
	/* check lane 0 */
	lane = 0;
	switch (COMMON_PHYS_SELECTOR_LANE_MASK(lane) & selector) {
	case COMMON_PHYS_SELECTOR_LANE_0_GBE_PORT0:
	case COMMON_PHYS_SELECTOR_LANE_0_GBE_V3_PORT0:
		net_mask |= MV_NETCOMP_GE_MAC0_2_SGMII_L0;
		break;
	}

	/* check lane 1 */
	lane = 1;
	switch (COMMON_PHYS_SELECTOR_LANE_MASK(lane) & selector) {
	case COMMON_PHYS_SELECTOR_LANE_1_GBE_PORT0:
	case COMMON_PHYS_SELECTOR_LANE_1_GBE_V3_PORT0:
		net_mask |= MV_NETCOMP_GE_MAC0_2_SGMII_L1;
		break;
	case COMMON_PHYS_SELECTOR_LANE_1_GBE_PORT1:
	case COMMON_PHYS_SELECTOR_LANE_1_GBE_V3_PORT1:
		net_mask |= MV_NETCOMP_GE_MAC1_2_SGMII_L1;
		break;
	case COMMON_PHYS_SELECTOR_LANE_1_QSGMII:
		net_mask |= MV_NETCOMP_GE_MAC1_2_QSGMII;
		break;
	default:
		net_mask |= MV_NETCOMP_GE_MAC1_2_RGMII1;
		break;
	}

	/* check lane 2 */
	lane = 2;
	switch (COMMON_PHYS_SELECTOR_LANE_MASK(lane) & selector) {
	case COMMON_PHYS_SELECTOR_LANE_2_GBE_PORT1:
	case COMMON_PHYS_SELECTOR_LANE_2_GBE_V3_PORT1:
		net_mask |= MV_NETCOMP_GE_MAC1_2_SGMII_L2;
		break;
	}

	/* check lane 3 */
	lane = 3;
	switch (COMMON_PHYS_SELECTOR_LANE_MASK(lane) & selector) {
	case COMMON_PHYS_SELECTOR_LANE_3_GBE_PORT2:
	case COMMON_PHYS_SELECTOR_LANE_3_GBE_V3_PORT2:
		net_mask |= MV_NETCOMP_GE_MAC2_2_SGMII_L3;
		break;
	case COMMON_PHYS_SELECTOR_LANE_3_XAUI_PORT3:
		xaui_lanes++;
		break;
	}

	/* check lane 4 */
	lane = 4;
	switch (COMMON_PHYS_SELECTOR_LANE_MASK(lane) & selector) {
	case COMMON_PHYS_SELECTOR_LANE_4_GBE_PORT1:
	case COMMON_PHYS_SELECTOR_LANE_4_GBE_V3_PORT3:
		net_mask |= MV_NETCOMP_GE_MAC3_2_SGMII_L4;
		break;
	case COMMON_PHYS_SELECTOR_LANE_4_XAUI_PORT2:
		xaui_lanes++;
		break;
	}

	/* check lane 5 */
	lane = 5;
	switch (COMMON_PHYS_SELECTOR_LANE_MASK(lane) & selector) {
	case COMMON_PHYS_SELECTOR_LANE_5_GBE_PORT2:
	case COMMON_PHYS_SELECTOR_LANE_5_GBE_V3_PORT2:
		net_mask |= MV_NETCOMP_GE_MAC2_2_SGMII_L5;
		break;
	case COMMON_PHYS_SELECTOR_LANE_5_XAUI_PORT1:
		xaui_lanes++;
		break;
	}

	/* check lane 6 */
	lane = 6;
	switch (COMMON_PHYS_SELECTOR_LANE_MASK(lane) & selector) {
	case COMMON_PHYS_SELECTOR_LANE_6_GBE_V3_PORT3:
		net_mask |= MV_NETCOMP_GE_MAC3_2_SGMII_L6;
		break;
	case COMMON_PHYS_SELECTOR_LANE_6_GBE_V3_PORT0:
		net_mask |= MV_NETCOMP_GE_MAC0_2_SGMII_L6;
		break;
	case COMMON_PHYS_SELECTOR_LANE_6_XAUI_PORT0:
		xaui_lanes++;
		break;
	}

	if (xaui_lanes > 0) {
		if (xaui_lanes == 4)
			net_mask |= MV_NETCOMP_GE_MAC0_2_XAUI;
		else if (xaui_lanes == 2)
			net_mask |= MV_NETCOMP_GE_MAC0_2_RXAUI;
		else
			return 0;
	}
	return net_mask;
}

/* lane_mode - new mode of SERDES 6 */
int mv_net_complex_dynamic_init(enum mv_net_complex_topology lane_mode)
{
	u32 i;
	u32 selector = readl((void *)COMMON_PHYS_SELECTORS_REG);

	u32 net_complex;

	/* reset current SERDES 6 configuration */
	selector &= ~COMMON_PHYS_SELECTOR_LANE_MASK(6);
	if ((selector & COMMON_PHYS_SELECTOR_LANE_MASK(5))
		== COMMON_PHYS_SELECTOR_LANE_5_XAUI_PORT1)
		/* reset current SERDES 5 configuration */
		selector &= ~COMMON_PHYS_SELECTOR_LANE_MASK(5);

	switch (lane_mode) {
	case MV_NETCOMP_GE_MAC0_2_RXAUI:
		/* reset current SERDES 5 configuration */
		selector &= ~COMMON_PHYS_SELECTOR_LANE_MASK(5);
		/* configure RXAUI on SERDES 5 and 6 */
		selector |= (COMMON_PHYS_SELECTOR_LANE_5_XAUI_PORT1 |
			     COMMON_PHYS_SELECTOR_LANE_6_XAUI_PORT0);
		break;
	case MV_NETCOMP_GE_MAC0_2_SGMII_L6:
		/* configure SGMII on SERDES 6 connected to MAC0 */
		selector |= COMMON_PHYS_SELECTOR_LANE_6_GBE_V3_PORT0;
		break;
	case MV_NETCOMP_GE_MAC3_2_SGMII_L6:
		/* configure SGMII on SERDES 6 connected to MAC3 */
		selector |= COMMON_PHYS_SELECTOR_LANE_6_GBE_V3_PORT3;
		break;
	default:
		return -1;
	}

	net_complex = mv_net_mask_init(selector);

	/* Active the GOP 4 ports */
	for (i = 0; i < 4; i++)
		mv_net_complex_active_port(i, 1);

	if ((net_complex & MV_NETCOMP_GE_MAC0_2_RXAUI) == 1)
		mv_net_complex_rxaui_enable(0, 1);
	else
		mv_net_complex_rxaui_enable(0, 0);

	/* Set Bus Width to HB mode = 1 */
	mv_net_complex_bus_width_select(1);
	/* Select SGMII mode */
	mv_net_complex_gbe_mode_select(MV_NETC_GBE1_SGMII);
	/* Configure the sample stages */
	mv_net_complex_sample_stages_timing(0);

	/* Configure the ComPhy Selector */
	mv_net_complex_com_phy_selector_config(net_complex);

	/* un reset */
	for (i = 0; i < 4; i++)
		mv_net_complex_port_rf_reset(i, 1);

	/* Enable the GOP internal clock logic */
	mv_net_complex_gop_clock_logic_set(1);
	/* De-assert GOP unit reset */
	mv_net_complex_gop_reset(1);

	return 0;
}

int mv_net_complex_init(u32 net_comp_config, enum mv_net_complex_phase phase)
{
	u32 reg;
	u32 c = net_comp_config, i;

	if (phase == MV_NETC_FIRST_PHASE) {
		/* fix the base address for transactions from the AXI to MBUS */
		reg = (readl((void *)MV_NETCOMP_AMB_ACCESS_CTRL_0)
		       & (~NETC_AMB_ACCESS_CTRL_MASK));
		reg |= (mv_net_reg_virt_base & NETC_AMB_ACCESS_CTRL_MASK);
		writel(reg, (void *)MV_NETCOMP_AMB_ACCESS_CTRL_0);
		/* Reset the GOP unit */
		mv_net_complex_gop_reset(0);
		/* Active the GOP 4 ports */
		for (i = 0; i < 4; i++)
			mv_net_complex_active_port(i, 1);
	}

	if (c & MV_NETCOMP_GE_MAC0_2_RXAUI)
		mv_net_complex_mac_to_rxaui(0, phase);

	if (c & MV_NETCOMP_GE_MAC0_2_XAUI)
		mv_net_complex_mac_to_xaui(0, phase);

	if (c & (MV_NETCOMP_GE_MAC0_2_SGMII_L0 | MV_NETCOMP_GE_MAC0_2_SGMII_L1 |
		MV_NETCOMP_GE_MAC0_2_SGMII_L6))
		mv_net_complex_mac_to_sgmii(0, phase, c);

	if (c & MV_NETCOMP_GE_MAC0_2_QSGMII)
		mv_net_complex_mac_to_qsgmii(0, phase);

	if (c & (MV_NETCOMP_GE_MAC1_2_SGMII_L1 | MV_NETCOMP_GE_MAC1_2_SGMII_L2 |
			MV_NETCOMP_GE_MAC1_2_SGMII_L4))
		mv_net_complex_mac_to_sgmii(1, phase, c);

	if (c & MV_NETCOMP_GE_MAC1_2_QSGMII)
		mv_net_complex_mac_to_qsgmii(1, phase);

	if (c & MV_NETCOMP_GE_MAC1_2_RGMII1)
		mv_net_complex_mac_to_rgmii(1, phase);

	if (c & (MV_NETCOMP_GE_MAC2_2_SGMII_L3 | MV_NETCOMP_GE_MAC2_2_SGMII_L5))
		mv_net_complex_mac_to_sgmii(2, phase, c);

	if (c & MV_NETCOMP_GE_MAC2_2_QSGMII)
		mv_net_complex_mac_to_qsgmii(2, phase);

	if (c & (MV_NETCOMP_GE_MAC3_2_SGMII_L4 | MV_NETCOMP_GE_MAC3_2_SGMII_L6))
		mv_net_complex_mac_to_sgmii(3, phase, c);

	if (c & MV_NETCOMP_GE_MAC3_2_QSGMII)
		mv_net_complex_mac_to_qsgmii(3, phase);

	if (phase == MV_NETC_FIRST_PHASE)
		/* Enable the NSS (PPv3) instead of the NetA (PPv1) */
		mv_net_complex_nss_select(1);

	if (phase == MV_NETC_SECOND_PHASE) {
		/* Enable the GOP internal clock logic */
		mv_net_complex_gop_clock_logic_set(1);
		/* De-assert GOP unit reset */
		mv_net_complex_gop_reset(1);
	}

	return 0;
}

static int mv_net_complex_plat_data_get(struct platform_device *pdev)
{
	struct resource *misc, *phy, *base, *regs, *dfx;

	misc = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	if (!misc) {
		pr_err("Can not find SoC control registers base address, aborting\n");
		return -1;
	}
	phy = platform_get_resource(pdev, IORESOURCE_MEM, 1);
	if (!phy) {
		pr_err("Can not find PHY registers base address, aborting\n");
		return -1;
	}
	base = platform_get_resource(pdev, IORESOURCE_MEM, 2);
	if (!base) {
		pr_err("Can not find NSS complex registers base address, aborting\n");
		return -1;
	}

	dfx = platform_get_resource(pdev, IORESOURCE_MEM, 3);
	if (!base) {
		pr_err("Can not find NSS complex registers base address, aborting\n");
		return -1;
	}

	/* map Misc registers space */
	mv_net_complex_misc_vbase_addr =
		(u32)devm_ioremap(&pdev->dev, misc->start, resource_size(misc));
	if (!mv_net_complex_misc_vbase_addr) {
		pr_err("Cannot map netcomplex misc registers, aborting\n");
		return -1;
	}
	pr_info("PP3 netcomplex misc registers base: PHYS = 0x%x, VIRT = 0x%0x, size = %d Bytes\n",
		misc->start, mv_net_complex_misc_vbase_addr,
		resource_size(misc));

	/* map PHY registers space */
	mv_net_complex_phy_vbase_addr =
		(u32)devm_ioremap(&pdev->dev, phy->start, resource_size(phy));
	if (!mv_net_complex_phy_vbase_addr) {
		pr_err("Cannot map netcomplex phy registers, aborting\n");
		return -1;
	}
	pr_info("PP3 netcomplex PHY registers base: PHYS = 0x%x, VIRT = 0x%0x, size = %d Bytes\n",
		phy->start, mv_net_complex_phy_vbase_addr, resource_size(phy));

	/* map NSS complex registers space */
	mv_net_complex_vbase_addr =
		(u32)devm_ioremap(&pdev->dev, base->start, resource_size(base));
	if (!mv_net_complex_vbase_addr) {
		pr_err("Cannot map netcomplex base registers, aborting\n");
		return -1;
	}
	pr_info("PP3 netcomplex registers base: PHYS = 0x%x, VIRT = 0x%0x, size = %d Bytes\n",
		base->start, mv_net_complex_vbase_addr, resource_size(base));

	/* map DFX server registers space */
	mv_net_dfx_vbase_addr =
		(u32)devm_ioremap(&pdev->dev, dfx->start, resource_size(dfx));
	if (!mv_net_dfx_vbase_addr) {
		pr_err("Cannot map DFX server registers, aborting\n");
		return -1;
	}
	pr_info("DFX server registers base: PHYS = 0x%x, VIRT = 0x%0x, size = %d Bytes\n",
		dfx->start, mv_net_dfx_vbase_addr, resource_size(dfx));

	/* map registers physical addr */
	regs = platform_get_resource(pdev, IORESOURCE_MEM, 4);
	if (!regs) {
		pr_err("Can not find NSS complex registers base address, aborting\n");
		return -1;
	}
	mv_net_reg_virt_base = (u32)ioremap(regs->start, resource_size(regs));
	if (!mv_net_reg_virt_base) {
		pr_err("Cannot map base registers, aborting\n");
		return -1;
	}
	pr_info("PP3 netcomplex bus region: PHYS = 0x%x, VIRT = 0x%0x, size = %d Bytes\n",
		regs->start, mv_net_reg_virt_base, resource_size(regs));

	return 0;
}

void mv_nss_sw_reset(void)
{
	int reg;

	/* De-activate table start init in the DFX server
	* bit[2] in Server Reset Control register
	*/

	reg = (readl((void *)MV_DFX_SERVER_RESET_REG_OFFSET)
	       | MV_DFX_SERVER_RESET_TABLE_START_INIT_MASK);
	writel(reg, (void *)MV_DFX_SERVER_RESET_REG_OFFSET);

	/* Pull down the SW resets of all NSS macros: reg = 0x7e55ff */
	mv_net_assert_soft_reset();

	/* Pull down the load_config of all NSS macros: reg = 0x7c01ff */
	mv_net_assert_load_config();

	/* Deassert load_config of all NSS macros:  reg = 0x7e55ff */
	mv_net_de_assert_load_config();

	/* Deassert SW reset of all NSS macros:  reg = 0x7fffff */
	mv_net_de_assert_soft_reset();

	/* Activate table start init in the DFX server */
	reg = (readl((void *)MV_DFX_SERVER_RESET_REG_OFFSET)
	       & (~MV_DFX_SERVER_RESET_TABLE_START_INIT_MASK));
	writel(reg, (void *)MV_DFX_SERVER_RESET_REG_OFFSET);

	mv_net_restore_regs_defaults();
}

static int mv_net_complex_probe(struct platform_device *pdev)
{
	int ret;
	u32 net_complex = 0;

	ret = mv_net_complex_plat_data_get(pdev);
	if (ret) {
		pr_err("net complex data get fail\n");
		return -1;
	}

	/* build net complex init mask according to board configuration */
	net_complex = mv_net_mask_init(readl((void *)COMMON_PHYS_SELECTORS_REG));
	if (net_complex == 0) {
		pr_info("\nSystem run with out net_complex\n");
	} else {
		pr_info("\nSystem run with net_complex = 0x%x\n", net_complex);
		mv_net_complex_init(net_complex, 0);
		mv_net_complex_init(net_complex, 1);
	}

	/* WA for A390 - when NSS wake up from reset
	* registers default values are wrong
	*/
	mv_net_restore_regs_defaults();

	return 0;
}

static int mv_net_complex_remove(struct platform_device *pdev)
{
	/* free all shared resources */
	return 0;
}

static const struct of_device_id net_complex_match[] = {
	{ .compatible = "marvell,armada-390-nss_complex" },
	{ }
};
MODULE_DEVICE_TABLE(of, net_complex_match);

static struct platform_driver mv_net_complex_driver = {
	.probe		= mv_net_complex_probe,
	.remove		= mv_net_complex_remove,
	.driver = {
		.name	= MV_NET_COMPLEX_NAME,
		.owner	= THIS_MODULE,
		.of_match_table = net_complex_match,
	},
};

module_platform_driver(mv_net_complex_driver);

