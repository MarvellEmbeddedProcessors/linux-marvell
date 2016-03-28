/*******************************************************************************
Copyright (C) Marvell International Ltd. and its affiliates

This software file (the "File") is owned and distributed by Marvell
International Ltd. and/or its affiliates ("Marvell") under the following
alternative licensing terms.  Once you have made an election to distribute the
File under one of the following license alternatives, please (i) delete this
introductory statement regarding license alternatives, (ii) delete the two
license alternatives that you have not elected to use and (iii) preserve the
Marvell copyright notice above.


********************************************************************************
Marvell GPL License Option

If you received this File from Marvell, you may opt to use, redistribute and/or
modify this File in accordance with the terms and conditions of the General
Public License Version 2, June 1991 (the "GPL License"), a copy of which is
available along with the File in the license.txt file or by writing to the Free
Software Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 or
on the worldwide web at http://www.gnu.org/licenses/gpl.txt.

THE FILE IS DISTRIBUTED AS-IS, WITHOUT WARRANTY OF ANY KIND, AND THE IMPLIED
WARRANTIES OF MERCHANTABILITY OR FITNESS FOR A PARTICULAR PURPOSE ARE EXPRESSLY
DISCLAIMED.  The GPL License provides additional details about this warranty
disclaimer.
*******************************************************************************/
#include "common/mv_sw_if.h"
#include "gop/mv_gop_if.h"
#include <gop/mac/mv_gmac_if.h>
#include <gop/mac/mv_gmac_regs.h>

/* print value of unit registers */
void mv_gmac_regs_dump(int port)
{
	int ind;
	char reg_name[32];

	mv_gop_reg_print("PORT_MAC_CTRL0", MV_GMAC_PORT_CTRL0_REG(port));
	mv_gop_reg_print("PORT_MAC_CTRL1", MV_GMAC_PORT_CTRL1_REG(port));
	mv_gop_reg_print("PORT_MAC_CTRL2", MV_GMAC_PORT_CTRL2_REG(port));
	mv_gop_reg_print("PORT_AUTO_NEG_CFG", MV_GMAC_PORT_AUTO_NEG_CFG_REG(port));
	mv_gop_reg_print("PORT_STATUS0", MV_GMAC_PORT_STATUS0_REG(port));
	mv_gop_reg_print("PORT_SERIAL_PARAM_CFG", MV_GMAC_PORT_SERIAL_PARAM_CFG_REG(port));
	mv_gop_reg_print("PORT_FIFO_CFG_0", MV_GMAC_PORT_FIFO_CFG_0_REG(port));
	mv_gop_reg_print("PORT_FIFO_CFG_1", MV_GMAC_PORT_FIFO_CFG_1_REG(port));
	mv_gop_reg_print("PORT_SERDES_CFG0", MV_GMAC_PORT_SERDES_CFG0_REG(port));
	mv_gop_reg_print("PORT_SERDES_CFG1", MV_GMAC_PORT_SERDES_CFG1_REG(port));
	mv_gop_reg_print("PORT_SERDES_CFG2", MV_GMAC_PORT_SERDES_CFG2_REG(port));
	mv_gop_reg_print("PORT_SERDES_CFG3", MV_GMAC_PORT_SERDES_CFG3_REG(port));
	mv_gop_reg_print("PORT_PRBS_STATUS", MV_GMAC_PORT_PRBS_STATUS_REG(port));
	mv_gop_reg_print("PORT_PRBS_ERR_CNTR", MV_GMAC_PORT_PRBS_ERR_CNTR_REG(port));
	mv_gop_reg_print("PORT_STATUS1", MV_GMAC_PORT_STATUS1_REG(port));
	mv_gop_reg_print("PORT_MIB_CNTRS_CTRL", MV_GMAC_PORT_MIB_CNTRS_CTRL_REG(port));
	mv_gop_reg_print("PORT_MAC_CTRL3", MV_GMAC_PORT_CTRL3_REG(port));
	mv_gop_reg_print("QSGMII", MV_GMAC_QSGMII_REG(port));
	mv_gop_reg_print("QSGMII_STATUS", MV_GMAC_QSGMII_STATUS_REG(port));
	mv_gop_reg_print("QSGMII_PRBS_CNTR", MV_GMAC_QSGMII_PRBS_CNTR_REG(port));
	for (ind = 0; ind < 8; ind++) {
		sprintf(reg_name, "CCFC_PORT_SPEED_TIMER%d", ind);
		mv_gop_reg_print(reg_name, MV_GMAC_CCFC_PORT_SPEED_TIMER_REG(port, ind));
	}
	for (ind = 0; ind < 4; ind++) {
		sprintf(reg_name, "FC_DSA_TAG%d", ind);
		mv_gop_reg_print(reg_name, MV_GMAC_FC_DSA_TAG_REG(port, ind));
	}
	mv_gop_reg_print("LINK_LEVEL_FLOW_CTRL_WIN_REG_0", MV_GMAC_LINK_LEVEL_FLOW_CTRL_WINDOW_REG_0(port));
	mv_gop_reg_print("LINK_LEVEL_FLOW_CTRL_WIN_REG_1", MV_GMAC_LINK_LEVEL_FLOW_CTRL_WINDOW_REG_1(port));
	mv_gop_reg_print("PORT_MAC_CTRL4", MV_GMAC_PORT_CTRL4_REG(port));
	mv_gop_reg_print("PORT_SERIAL_PARAM_1_CFG", MV_GMAC_PORT_SERIAL_PARAM_1_CFG_REG(port));
	mv_gop_reg_print("LPI_CTRL_0", MV_GMAC_LPI_CTRL_0_REG(port));
	mv_gop_reg_print("LPI_CTRL_1", MV_GMAC_LPI_CTRL_1_REG(port));
	mv_gop_reg_print("LPI_CTRL_2", MV_GMAC_LPI_CTRL_2_REG(port));
	mv_gop_reg_print("LPI_STATUS", MV_GMAC_LPI_STATUS_REG(port));
	mv_gop_reg_print("LPI_CNTR", MV_GMAC_LPI_CNTR_REG(port));
	mv_gop_reg_print("PULSE_1_MS_LOW", MV_GMAC_PULSE_1_MS_LOW_REG(port));
	mv_gop_reg_print("PULSE_1_MS_HIGH", MV_GMAC_PULSE_1_MS_HIGH_REG(port));
	mv_gop_reg_print("PORT_INT_MASK", MV_GMAC_INTERRUPT_MASK_REG(port));
	mv_gop_reg_print("INT_SUM_MASK", MV_GMAC_INTERRUPT_SUM_MASK_REG(port));
}

/* Set the MAC to reset or exit from reset */
int mv_gmac_reset(int mac_num, enum mv_reset reset)
{
	u32 reg_addr;
	u32 val;

	reg_addr = MV_GMAC_PORT_CTRL2_REG(mac_num);

	/* read - modify - write */
	val = mv_gop_reg_read(reg_addr);
	if (reset == RESET)
		val |= MV_GMAC_PORT_CTRL2_PORTMACRESET_MASK;
	else
		val &= ~MV_GMAC_PORT_CTRL2_PORTMACRESET_MASK;
	mv_gop_reg_write(reg_addr, val);

	return 0;
}

static void mv_gmac_rgmii_cfg(int mac_num)
{
	u32 val, thresh, an;

	/* configure minimal level of the Tx FIFO before the lower part starts to read a packet */
	thresh = MV_RGMII_TX_FIFO_MIN_TH;
	val = mv_gop_reg_read(MV_GMAC_PORT_FIFO_CFG_1_REG(mac_num));
	MV_U32_SET_FIELD(val, MV_GMAC_PORT_FIFO_CFG_1_TX_FIFO_MIN_TH_MASK,
		(thresh << MV_GMAC_PORT_FIFO_CFG_1_TX_FIFO_MIN_TH_OFFS));
	mv_gop_reg_write(MV_GMAC_PORT_FIFO_CFG_1_REG(mac_num), val);

	/* Disable bypass of sync module */
	val = mv_gop_reg_read(MV_GMAC_PORT_CTRL4_REG(mac_num));
	val |= MV_GMAC_PORT_CTRL4_SYNC_BYPASS_MASK;
	/* configure DP clock select according to mode */
	val &= ~MV_GMAC_PORT_CTRL4_DP_CLK_SEL_MASK;
	val |= MV_GMAC_PORT_CTRL4_QSGMII_BYPASS_ACTIVE_MASK;
	val |= MV_GMAC_PORT_CTRL4_EXT_PIN_GMII_SEL_MASK;
	mv_gop_reg_write(MV_GMAC_PORT_CTRL4_REG(mac_num), val);

	val = mv_gop_reg_read(MV_GMAC_PORT_CTRL2_REG(mac_num));
	val &= ~MV_GMAC_PORT_CTRL2_DIS_PADING_OFFS;
	mv_gop_reg_write(MV_GMAC_PORT_CTRL2_REG(mac_num), val);

	val = mv_gop_reg_read(MV_GMAC_PORT_CTRL0_REG(mac_num));
	/* configure GIG MAC to SGMII mode */
	val &= ~MV_GMAC_PORT_CTRL0_PORTTYPE_MASK;
	mv_gop_reg_write(MV_GMAC_PORT_CTRL0_REG(mac_num), val);

	/* configure AN 0xb8e8 */
	an = MV_GMAC_PORT_AUTO_NEG_CFG_AN_BYPASS_EN_MASK |
		MV_GMAC_PORT_AUTO_NEG_CFG_EN_AN_SPEED_MASK   |
		MV_GMAC_PORT_AUTO_NEG_CFG_EN_FC_AN_MASK      |
		MV_GMAC_PORT_AUTO_NEG_CFG_EN_FDX_AN_MASK     |
		MV_GMAC_PORT_AUTO_NEG_CFG_CHOOSE_SAMPLE_TX_CONFIG_MASK;
	mv_gop_reg_write(MV_GMAC_PORT_AUTO_NEG_CFG_REG(mac_num), an);
}

static void mv_gmac_qsgmii_cfg(int mac_num)
{
	u32 val, thresh, an;

	/* configure minimal level of the Tx FIFO before the lower part starts to read a packet */
	thresh = MV_SGMII_TX_FIFO_MIN_TH;
	val = mv_gop_reg_read(MV_GMAC_PORT_FIFO_CFG_1_REG(mac_num));
	MV_U32_SET_FIELD(val, MV_GMAC_PORT_FIFO_CFG_1_TX_FIFO_MIN_TH_MASK,
		(thresh << MV_GMAC_PORT_FIFO_CFG_1_TX_FIFO_MIN_TH_OFFS));
	mv_gop_reg_write(MV_GMAC_PORT_FIFO_CFG_1_REG(mac_num), val);

	/* Disable bypass of sync module */
	val = mv_gop_reg_read(MV_GMAC_PORT_CTRL4_REG(mac_num));
	val |= MV_GMAC_PORT_CTRL4_SYNC_BYPASS_MASK;
	/* configure DP clock select according to mode */
	val &= ~MV_GMAC_PORT_CTRL4_DP_CLK_SEL_MASK;
	val &= ~MV_GMAC_PORT_CTRL4_EXT_PIN_GMII_SEL_MASK;
	/* configure QSGMII bypass according to mode */
	val &= ~MV_GMAC_PORT_CTRL4_QSGMII_BYPASS_ACTIVE_MASK;
	mv_gop_reg_write(MV_GMAC_PORT_CTRL4_REG(mac_num), val);

	val = mv_gop_reg_read(MV_GMAC_PORT_CTRL2_REG(mac_num));
	val &= ~MV_GMAC_PORT_CTRL2_DIS_PADING_OFFS;
	mv_gop_reg_write(MV_GMAC_PORT_CTRL2_REG(mac_num), val);

	val = mv_gop_reg_read(MV_GMAC_PORT_CTRL0_REG(mac_num));
	/* configure GIG MAC to SGMII mode */
	val &= ~MV_GMAC_PORT_CTRL0_PORTTYPE_MASK;
	mv_gop_reg_write(MV_GMAC_PORT_CTRL0_REG(mac_num), val);

	/* configure AN 0xB8EC */
	an = MV_GMAC_PORT_AUTO_NEG_CFG_EN_PCS_AN_MASK |
		MV_GMAC_PORT_AUTO_NEG_CFG_AN_BYPASS_EN_MASK |
		MV_GMAC_PORT_AUTO_NEG_CFG_EN_AN_SPEED_MASK  |
		MV_GMAC_PORT_AUTO_NEG_CFG_EN_FC_AN_MASK     |
		MV_GMAC_PORT_AUTO_NEG_CFG_EN_FDX_AN_MASK    |
		MV_GMAC_PORT_AUTO_NEG_CFG_CHOOSE_SAMPLE_TX_CONFIG_MASK;
	mv_gop_reg_write(MV_GMAC_PORT_AUTO_NEG_CFG_REG(mac_num), an);
}

static void mv_gmac_sgmii_cfg(int mac_num)
{
	u32 val, thresh, an;

	/* configure minimal level of the Tx FIFO before the lower part starts to read a packet */
	thresh = MV_SGMII_TX_FIFO_MIN_TH;
	val = mv_gop_reg_read(MV_GMAC_PORT_FIFO_CFG_1_REG(mac_num));
	MV_U32_SET_FIELD(val, MV_GMAC_PORT_FIFO_CFG_1_TX_FIFO_MIN_TH_MASK,
		(thresh << MV_GMAC_PORT_FIFO_CFG_1_TX_FIFO_MIN_TH_OFFS));
	mv_gop_reg_write(MV_GMAC_PORT_FIFO_CFG_1_REG(mac_num), val);

	/* Disable bypass of sync module */
	val = mv_gop_reg_read(MV_GMAC_PORT_CTRL4_REG(mac_num));
	val |= MV_GMAC_PORT_CTRL4_SYNC_BYPASS_MASK;
	/* configure DP clock select according to mode */
	val &= ~MV_GMAC_PORT_CTRL4_DP_CLK_SEL_MASK;
	/* configure QSGMII bypass according to mode */
	val |= MV_GMAC_PORT_CTRL4_QSGMII_BYPASS_ACTIVE_MASK;
	mv_gop_reg_write(MV_GMAC_PORT_CTRL4_REG(mac_num), val);

	val = mv_gop_reg_read(MV_GMAC_PORT_CTRL2_REG(mac_num));
	val |= MV_GMAC_PORT_CTRL2_DIS_PADING_OFFS;
	mv_gop_reg_write(MV_GMAC_PORT_CTRL2_REG(mac_num), val);

	val = mv_gop_reg_read(MV_GMAC_PORT_CTRL0_REG(mac_num));
	/* configure GIG MAC to SGMII mode */
	val &= ~MV_GMAC_PORT_CTRL0_PORTTYPE_MASK;
	mv_gop_reg_write(MV_GMAC_PORT_CTRL0_REG(mac_num), val);

	/* configure AN */
	an = MV_GMAC_PORT_AUTO_NEG_CFG_EN_PCS_AN_MASK |
		MV_GMAC_PORT_AUTO_NEG_CFG_AN_BYPASS_EN_MASK |
		MV_GMAC_PORT_AUTO_NEG_CFG_EN_AN_SPEED_MASK  |
		MV_GMAC_PORT_AUTO_NEG_CFG_EN_FC_AN_MASK     |
		MV_GMAC_PORT_AUTO_NEG_CFG_EN_FDX_AN_MASK    |
		MV_GMAC_PORT_AUTO_NEG_CFG_CHOOSE_SAMPLE_TX_CONFIG_MASK;
	mv_gop_reg_write(MV_GMAC_PORT_AUTO_NEG_CFG_REG(mac_num), an);
}

static void mv_gmac_sgmii2_5_cfg(int mac_num)
{
	u32 val, thresh, an;

	/* configure minimal level of the Tx FIFO before the lower part starts to read a packet */
	thresh = MV_SGMII2_5_TX_FIFO_MIN_TH;
	val = mv_gop_reg_read(MV_GMAC_PORT_FIFO_CFG_1_REG(mac_num));
	MV_U32_SET_FIELD(val, MV_GMAC_PORT_FIFO_CFG_1_TX_FIFO_MIN_TH_MASK,
		(thresh << MV_GMAC_PORT_FIFO_CFG_1_TX_FIFO_MIN_TH_OFFS));
	mv_gop_reg_write(MV_GMAC_PORT_FIFO_CFG_1_REG(mac_num), val);

	/* Disable bypass of sync module */
	val = mv_gop_reg_read(MV_GMAC_PORT_CTRL4_REG(mac_num));
	val |= MV_GMAC_PORT_CTRL4_SYNC_BYPASS_MASK;
	/* configure DP clock select according to mode */
	val |= MV_GMAC_PORT_CTRL4_DP_CLK_SEL_MASK;
	/* configure QSGMII bypass according to mode */
	val |= MV_GMAC_PORT_CTRL4_QSGMII_BYPASS_ACTIVE_MASK;
	mv_gop_reg_write(MV_GMAC_PORT_CTRL4_REG(mac_num), val);

	val = mv_gop_reg_read(MV_GMAC_PORT_CTRL2_REG(mac_num));
	val |= MV_GMAC_PORT_CTRL2_DIS_PADING_OFFS;
	mv_gop_reg_write(MV_GMAC_PORT_CTRL2_REG(mac_num), val);

	val = mv_gop_reg_read(MV_GMAC_PORT_CTRL0_REG(mac_num));
	/* configure GIG MAC to 1000Base-X mode connected to a fiber transceiver */
	val |= MV_GMAC_PORT_CTRL0_PORTTYPE_MASK;
	mv_gop_reg_write(MV_GMAC_PORT_CTRL0_REG(mac_num), val);

	/* configure AN 0x9268 */
	an = MV_GMAC_PORT_AUTO_NEG_CFG_AN_BYPASS_EN_MASK |
		MV_GMAC_PORT_AUTO_NEG_CFG_SET_MII_SPEED_MASK  |
		MV_GMAC_PORT_AUTO_NEG_CFG_SET_GMII_SPEED_MASK     |
		MV_GMAC_PORT_AUTO_NEG_CFG_ADV_PAUSE_MASK    |
		MV_GMAC_PORT_AUTO_NEG_CFG_SET_FULL_DX_MASK  |
		MV_GMAC_PORT_AUTO_NEG_CFG_CHOOSE_SAMPLE_TX_CONFIG_MASK;
	mv_gop_reg_write(MV_GMAC_PORT_AUTO_NEG_CFG_REG(mac_num), an);
}

/* Set the internal mux's to the required MAC in the GOP */
int mv_gmac_mode_cfg(int mac_num, int mode)
{
	u32 reg_addr;
	u32 val;

	/* Set TX FIFO thresholds */
	switch (mode) {
	case MV_PORT_SGMII2_5:
		mv_gmac_sgmii2_5_cfg(mac_num);
	break;
	case MV_PORT_SGMII:
		mv_gmac_sgmii_cfg(mac_num);
	break;
	case MV_PORT_RGMII:
		mv_gmac_rgmii_cfg(mac_num);
	break;
	case MV_PORT_QSGMII:
		mv_gmac_qsgmii_cfg(mac_num);
	break;
	default:
		return -1;
	}

	/* Jumbo frame support - 0x1400*2= 0x2800 bytes */
	val = mv_gop_reg_read(MV_GMAC_PORT_CTRL0_REG(mac_num));
	MV_U32_SET_FIELD(val, MV_GMAC_PORT_CTRL0_FRAMESIZELIMIT_MASK,
		(0x1400 << MV_GMAC_PORT_CTRL0_FRAMESIZELIMIT_OFFS));
	mv_gop_reg_write(MV_GMAC_PORT_CTRL0_REG(mac_num), val);

	/* PeriodicXonEn disable */
	reg_addr = MV_GMAC_PORT_CTRL1_REG(mac_num);
	val = mv_gop_reg_read(reg_addr);
	val &= ~MV_GMAC_PORT_CTRL1_EN_PERIODIC_FC_XON_MASK;
	mv_gop_reg_write(reg_addr, val);

	/* mask all ports interrupts */
	mv_gmac_port_link_event_mask(mac_num);

	/* unmask link change interrupt */
	val = mv_gop_reg_read(MV_GMAC_INTERRUPT_MASK_REG(mac_num));
	val |= MV_GMAC_INTERRUPT_CAUSE_LINK_CHANGE_MASK;
	val |= 1; /* unmask summary bit */
	mv_gop_reg_write(MV_GMAC_INTERRUPT_MASK_REG(mac_num), val);

	return 0;
}

/* Configure MAC loopback */
int mv_gmac_loopback_cfg(int mac_num, enum mv_lb_type type)
{
	u32 reg_addr;
	u32 val;

	reg_addr = MV_GMAC_PORT_CTRL1_REG(mac_num);
	val = mv_gop_reg_read(reg_addr);
	switch (type) {
	case MV_DISABLE_LB:
		val &= ~MV_GMAC_PORT_CTRL1_GMII_LOOPBACK_MASK;
		break;
	case MV_TX_2_RX_LB:
		val |= MV_GMAC_PORT_CTRL1_GMII_LOOPBACK_MASK;
		break;
	case MV_RX_2_TX_LB:
	default:
		return -1;
	}
	mv_gop_reg_write(reg_addr, val);

	return 0;
}

/* Get MAC link status */
bool mv_gmac_link_status_get(int mac_num)
{
	u32 reg_addr;
	u32 val;

	reg_addr = MV_GMAC_PORT_STATUS0_REG(mac_num);

	val = mv_gop_reg_read(reg_addr);
	return (val & 1) ? true : false;
}

/* Enable port and MIB counters */
void mv_gmac_port_enable(int mac_num)
{
	u32 reg_val;

	reg_val = mv_gop_reg_read(MV_GMAC_PORT_CTRL0_REG(mac_num));
	reg_val |= MV_GMAC_PORT_CTRL0_PORTEN_MASK;
	reg_val |= MV_GMAC_PORT_CTRL0_COUNT_EN_MASK;

	mv_gop_reg_write(MV_GMAC_PORT_CTRL0_REG(mac_num), reg_val);
}

/* Disable port */
void mv_gmac_port_disable(int mac_num)
{
	u32 reg_val;

	/* mask all ports interrupts */
	mv_gmac_port_link_event_mask(mac_num);

	reg_val = mv_gop_reg_read(MV_GMAC_PORT_CTRL0_REG(mac_num));
	reg_val &= ~MV_GMAC_PORT_CTRL0_PORTEN_MASK;

	mv_gop_reg_write(MV_GMAC_PORT_CTRL0_REG(mac_num), reg_val);
}

void mv_gmac_port_periodic_xon_set(int mac_num, int enable)
{
	u32 reg_val;

	reg_val =  mv_gop_reg_read(MV_GMAC_PORT_CTRL1_REG(mac_num));

	if (enable)
		reg_val |= MV_GMAC_PORT_CTRL1_EN_PERIODIC_FC_XON_MASK;
	else
		reg_val &= ~MV_GMAC_PORT_CTRL1_EN_PERIODIC_FC_XON_MASK;

	mv_gop_reg_write(MV_GMAC_PORT_CTRL1_REG(mac_num), reg_val);
}

int mv_gmac_link_status(int mac_num, struct mv_port_link_status *pstatus)
{
	u32 reg_val;

	reg_val = mv_gop_reg_read(MV_GMAC_PORT_STATUS0_REG(mac_num));

	if (reg_val & MV_GMAC_PORT_STATUS0_GMIISPEED_MASK)
		pstatus->speed = MV_PORT_SPEED_1000;
	else if (reg_val & MV_GMAC_PORT_STATUS0_MIISPEED_MASK)
		pstatus->speed = MV_PORT_SPEED_100;
	else
		pstatus->speed = MV_PORT_SPEED_10;

	if (reg_val & MV_GMAC_PORT_STATUS0_LINKUP_MASK)
		pstatus->linkup = 1 /*TRUE*/;
	else
		pstatus->linkup = 0 /*FALSE*/;

	if (reg_val & MV_GMAC_PORT_STATUS0_FULLDX_MASK)
		pstatus->duplex = MV_PORT_DUPLEX_FULL;
	else
		pstatus->duplex = MV_PORT_DUPLEX_HALF;

	if (reg_val & MV_GMAC_PORT_STATUS0_PORTTXPAUSE_MASK)
		pstatus->tx_fc = MV_PORT_FC_ACTIVE;
	else if (reg_val & MV_GMAC_PORT_STATUS0_TXFCEN_MASK)
		pstatus->tx_fc = MV_PORT_FC_ENABLE;
	else
		pstatus->tx_fc = MV_PORT_FC_DISABLE;

	if (reg_val & MV_GMAC_PORT_STATUS0_PORTRXPAUSE_MASK)
		pstatus->rx_fc = MV_PORT_FC_ACTIVE;
	else if (reg_val & MV_GMAC_PORT_STATUS0_RXFCEN_MASK)
		pstatus->rx_fc = MV_PORT_FC_ENABLE;
	else
		pstatus->rx_fc = MV_PORT_FC_DISABLE;

	return 0;
}

/* Change maximum receive size of the port */
int mv_gmac_max_rx_size_set(int mac_num, int max_rx_size)
{
	u32	reg_val;

	reg_val =  mv_gop_reg_read(MV_GMAC_PORT_CTRL0_REG(mac_num));
	reg_val &= ~MV_GMAC_PORT_CTRL0_FRAMESIZELIMIT_MASK;
	reg_val |= (((max_rx_size - MV_MH_SIZE) / 2) << MV_GMAC_PORT_CTRL0_FRAMESIZELIMIT_OFFS);
	mv_gop_reg_write(MV_GMAC_PORT_CTRL0_REG(mac_num), reg_val);

	return 0;
}

/* Sets "Force Link Pass" and "Do Not Force Link Fail" bits.
*  This function should only be called when the port is disabled.
* INPUT:
*	int  port		- port number
*	bool force_link_pass	- Force Link Pass
*	bool force_link_fail - Force Link Failure
*		0, 0 - normal state: detect link via PHY and connector
*		1, 1 - prohibited state.
*/
int mv_gmac_force_link_mode_set(int mac_num, bool force_link_up, bool force_link_down)
{
	u32 reg_val;

	/* Can't force link pass and link fail at the same time */
	if ((force_link_up) && (force_link_down))
		return -EINVAL;

	reg_val = mv_gop_reg_read(MV_GMAC_PORT_AUTO_NEG_CFG_REG(mac_num));

	if (force_link_up)
		reg_val |= MV_GMAC_PORT_AUTO_NEG_CFG_FORCE_LINK_UP_MASK;
	else
		reg_val &= ~MV_GMAC_PORT_AUTO_NEG_CFG_FORCE_LINK_UP_MASK;

	if (force_link_down)
		reg_val |= MV_GMAC_PORT_AUTO_NEG_CFG_FORCE_LINK_DOWN_MASK;
	else
		reg_val &= ~MV_GMAC_PORT_AUTO_NEG_CFG_FORCE_LINK_DOWN_MASK;

	mv_gop_reg_write(MV_GMAC_PORT_AUTO_NEG_CFG_REG(mac_num), reg_val);

	return 0;
}

/* Sets port speed to Auto Negotiation / 1000 / 100 / 10 Mbps.
*  Sets port duplex to Auto Negotiation / Full / Half Duplex.
*/
int mv_gmac_speed_duplex_set(int mac_num, enum mv_port_speed speed, enum mv_port_duplex duplex)
{
	u32 reg_val;

	/* Check validity */
	if ((speed == MV_PORT_SPEED_1000) && (duplex == MV_PORT_DUPLEX_HALF))
		return -EINVAL;

	reg_val = mv_gop_reg_read(MV_GMAC_PORT_AUTO_NEG_CFG_REG(mac_num));

	switch (speed) {
	case MV_PORT_SPEED_AN:
		reg_val |= MV_GMAC_PORT_AUTO_NEG_CFG_EN_AN_SPEED_MASK;
		/* the other bits don't matter in this case */
		break;
	case MV_PORT_SPEED_1000:
		reg_val &= ~MV_GMAC_PORT_AUTO_NEG_CFG_EN_AN_SPEED_MASK;
		reg_val |= MV_GMAC_PORT_AUTO_NEG_CFG_SET_GMII_SPEED_MASK;
		/* the 100/10 bit doesn't matter in this case */
		break;
	case MV_PORT_SPEED_100:
		reg_val &= ~MV_GMAC_PORT_AUTO_NEG_CFG_EN_AN_SPEED_MASK;
		reg_val &= ~MV_GMAC_PORT_AUTO_NEG_CFG_SET_GMII_SPEED_MASK;
		reg_val |= MV_GMAC_PORT_AUTO_NEG_CFG_SET_MII_SPEED_MASK;
		break;
	case MV_PORT_SPEED_10:
		reg_val &= ~MV_GMAC_PORT_AUTO_NEG_CFG_EN_AN_SPEED_MASK;
		reg_val &= ~MV_GMAC_PORT_AUTO_NEG_CFG_SET_GMII_SPEED_MASK;
		reg_val &= ~MV_GMAC_PORT_AUTO_NEG_CFG_SET_MII_SPEED_MASK;
		break;
	default:
		pr_info("GMAC: Unexpected Speed value %d\n", speed);
		return -EINVAL;
	}

	switch (duplex) {
	case MV_PORT_DUPLEX_AN:
		reg_val  |= MV_GMAC_PORT_AUTO_NEG_CFG_EN_FDX_AN_MASK;
		/* the other bits don't matter in this case */
		break;
	case MV_PORT_DUPLEX_HALF:
		reg_val &= ~MV_GMAC_PORT_AUTO_NEG_CFG_EN_FDX_AN_MASK;
		reg_val &= ~MV_GMAC_PORT_AUTO_NEG_CFG_SET_FULL_DX_MASK;
		break;
	case MV_PORT_DUPLEX_FULL:
		reg_val &= ~MV_GMAC_PORT_AUTO_NEG_CFG_EN_FDX_AN_MASK;
		reg_val |= MV_GMAC_PORT_AUTO_NEG_CFG_SET_FULL_DX_MASK;
		break;
	default:
		pr_err("GMAC: Unexpected Duplex value %d\n", duplex);
		return -EINVAL;
	}

	mv_gop_reg_write(MV_GMAC_PORT_AUTO_NEG_CFG_REG(mac_num), reg_val);
	return 0;
}

/* Gets port speed and duplex */
int mv_gmac_speed_duplex_get(int mac_num, enum mv_port_speed *speed, enum mv_port_duplex *duplex)
{
	u32 reg_val;

	/* Check validity */
	if (!speed || !duplex)
		return -EINVAL;

	reg_val = mv_gop_reg_read(MV_GMAC_PORT_AUTO_NEG_CFG_REG(mac_num));

	if (reg_val & MV_GMAC_PORT_AUTO_NEG_CFG_EN_AN_SPEED_MASK)
		*speed = MV_PORT_SPEED_AN;
	else if (reg_val & MV_GMAC_PORT_AUTO_NEG_CFG_SET_GMII_SPEED_MASK)
		*speed = MV_PORT_SPEED_1000;
	else if (reg_val & MV_GMAC_PORT_AUTO_NEG_CFG_SET_MII_SPEED_MASK)
		*speed = MV_PORT_SPEED_100;
	else
		*speed = MV_PORT_SPEED_10;

	if (reg_val & MV_GMAC_PORT_AUTO_NEG_CFG_EN_FDX_AN_MASK)
		*duplex = MV_PORT_DUPLEX_AN;
	else if (reg_val & MV_GMAC_PORT_AUTO_NEG_CFG_SET_FULL_DX_MASK)
		*duplex = MV_PORT_DUPLEX_FULL;
	else
		*duplex = MV_PORT_DUPLEX_HALF;

	return 0;
}

/* Configure the port's Flow Control properties */
int mv_gmac_fc_set(int mac_num, enum mv_port_fc fc)
{
	u32 reg_val;
	u32 fc_en;

	reg_val = mv_gop_reg_read(MV_GMAC_PORT_AUTO_NEG_CFG_REG(mac_num));

	switch (fc) {
	case MV_PORT_FC_AN_NO:
		reg_val &= ~MV_GMAC_PORT_AUTO_NEG_CFG_EN_FC_AN_MASK;
		reg_val &= ~MV_GMAC_PORT_AUTO_NEG_CFG_ADV_PAUSE_MASK;
		reg_val &= ~MV_GMAC_PORT_AUTO_NEG_CFG_ADV_ASM_PAUSE_MASK;
		break;

	case MV_PORT_FC_AN_SYM:
		reg_val |= MV_GMAC_PORT_AUTO_NEG_CFG_EN_FC_AN_MASK;
		reg_val |= MV_GMAC_PORT_AUTO_NEG_CFG_ADV_PAUSE_MASK;
		reg_val &= ~MV_GMAC_PORT_AUTO_NEG_CFG_ADV_ASM_PAUSE_MASK;
		break;

	case MV_PORT_FC_AN_ASYM:
		reg_val |= MV_GMAC_PORT_AUTO_NEG_CFG_EN_FC_AN_MASK;
		reg_val |= MV_GMAC_PORT_AUTO_NEG_CFG_ADV_PAUSE_MASK;
		reg_val |= MV_GMAC_PORT_AUTO_NEG_CFG_ADV_ASM_PAUSE_MASK;
		break;

	case MV_PORT_FC_DISABLE:
		reg_val &= ~MV_GMAC_PORT_AUTO_NEG_CFG_EN_FC_AN_MASK;
		reg_val &= ~MV_GMAC_PORT_AUTO_NEG_CFG_ADV_ASM_PAUSE_MASK;
		fc_en = mv_gop_reg_read(MV_GMAC_PORT_CTRL4_REG(mac_num));
		fc_en &= ~MV_GMAC_PORT_CTRL4_FC_EN_RX_MASK;
		fc_en &= ~MV_GMAC_PORT_CTRL4_FC_EN_TX_MASK;
		mv_gop_reg_write(MV_GMAC_PORT_CTRL4_REG(mac_num), fc_en);
		break;

	case MV_PORT_FC_ENABLE:
		reg_val &= ~MV_GMAC_PORT_AUTO_NEG_CFG_EN_FC_AN_MASK;
		fc_en = mv_gop_reg_read(MV_GMAC_PORT_CTRL4_REG(mac_num));
		fc_en |= MV_GMAC_PORT_CTRL4_FC_EN_RX_MASK;
		fc_en |= MV_GMAC_PORT_CTRL4_FC_EN_TX_MASK;
		mv_gop_reg_write(MV_GMAC_PORT_CTRL4_REG(mac_num), fc_en);
		break;

	default:
		pr_err("GMAC: Unexpected FlowControl value %d\n", fc);
		return -EINVAL;
	}

	mv_gop_reg_write(MV_GMAC_PORT_AUTO_NEG_CFG_REG(mac_num), reg_val);
	return 0;
}

/* Get Flow Control configuration of the port */
void mv_gmac_fc_get(int mac_num, enum mv_port_fc *fc)
{
	u32 reg_val;

	reg_val = mv_gop_reg_read(MV_GMAC_PORT_AUTO_NEG_CFG_REG(mac_num));

	if (reg_val & MV_GMAC_PORT_AUTO_NEG_CFG_EN_FC_AN_MASK) {
		/* Auto negotiation is enabled */
		if (reg_val & MV_GMAC_PORT_AUTO_NEG_CFG_ADV_PAUSE_MASK) {
			if (reg_val & MV_GMAC_PORT_AUTO_NEG_CFG_ADV_ASM_PAUSE_MASK)
				*fc = MV_PORT_FC_AN_ASYM;
			else
				*fc = MV_PORT_FC_AN_SYM;
		} else
			*fc = MV_PORT_FC_AN_NO;
	} else {
		/* Auto negotiation is disabled */
		reg_val = mv_gop_reg_read(MV_GMAC_PORT_CTRL4_REG(mac_num));
		if ((reg_val & MV_GMAC_PORT_CTRL4_FC_EN_RX_MASK) &&
			(reg_val & MV_GMAC_PORT_CTRL4_FC_EN_TX_MASK))
			*fc = MV_PORT_FC_ENABLE;
		else
			*fc = MV_PORT_FC_DISABLE;
	}
}

int mv_gmac_port_link_speed_fc(int mac_num, enum mv_port_speed speed, int force_link_up)
{
	if (force_link_up) {
		if (mv_gmac_speed_duplex_set(mac_num, speed, MV_PORT_DUPLEX_FULL)) {
			pr_err("mv_gmac_speed_duplex_set failed\n");
			return -EPERM;
		}
		if (mv_gmac_fc_set(mac_num, MV_PORT_FC_ENABLE)) {
			pr_err("mv_gmac_fc_set failed\n");
			return -EPERM;
		}
		if (mv_gmac_force_link_mode_set(mac_num, 1, 0)) {
			pr_err("mv_gmac_force_link_mode_set failed\n");
			return -EPERM;
		}
	} else {
		if (mv_gmac_force_link_mode_set(mac_num, 0, 0)) {
			pr_err("mv_gmac_force_link_mode_set failed\n");
			return -EPERM;
		}
		if (mv_gmac_speed_duplex_set(mac_num, MV_PORT_SPEED_AN, MV_PORT_DUPLEX_AN)) {
			pr_err("mv_gmac_speed_duplex_set failed\n");
			return -EPERM;
		}
		if (mv_gmac_fc_set(mac_num, MV_PORT_FC_AN_SYM)) {
			pr_err("mv_gmac_fc_set failed\n");
			return -EPERM;
		}
	}

	return 0;
}

void mv_gmac_port_link_event_mask(int mac_num)
{
	u32 reg_val;

	reg_val = mv_gop_reg_read(MV_GMAC_INTERRUPT_SUM_MASK_REG(mac_num));
	reg_val &= ~MV_GMAC_INTERRUPT_SUM_CAUSE_LINK_CHANGE_MASK;
	mv_gop_reg_write(MV_GMAC_INTERRUPT_SUM_MASK_REG(mac_num), reg_val);
}

void mv_gmac_port_link_event_unmask(int mac_num)
{
	u32 reg_val;

	reg_val = mv_gop_reg_read(MV_GMAC_INTERRUPT_SUM_MASK_REG(mac_num));
	reg_val |= MV_GMAC_INTERRUPT_SUM_CAUSE_LINK_CHANGE_MASK;
	reg_val |= 1; /* unmask summary bit */
	mv_gop_reg_write(MV_GMAC_INTERRUPT_SUM_MASK_REG(mac_num), reg_val);
}

void mv_gmac_port_link_event_clear(int mac_num)
{
	u32 reg_val;

	reg_val = mv_gop_reg_read(MV_GMAC_INTERRUPT_CAUSE_REG(mac_num));
}

int mv_gmac_port_autoneg_restart(int mac_num)
{
	u32 reg_val;

	reg_val = mv_gop_reg_read(MV_GMAC_PORT_AUTO_NEG_CFG_REG(mac_num));
	/* enable AN and restart it */
	reg_val |= MV_GMAC_PORT_AUTO_NEG_CFG_EN_PCS_AN_MASK;
	reg_val |= MV_GMAC_PORT_AUTO_NEG_CFG_INBAND_RESTARTAN_MASK;
	mv_gop_reg_write(MV_GMAC_PORT_AUTO_NEG_CFG_REG(mac_num), reg_val);
	return 0;
}
