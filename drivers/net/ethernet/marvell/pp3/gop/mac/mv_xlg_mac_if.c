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
#include "gop/mac/mv_xlg_mac_if.h"
#include "gop/mac/mv_xlg_mac_regs.h"


/* print value of unit registers */
void mv_xlg_mac_regs_dump(int port)
{
	int timer;
	char reg_name[16];

	mv_gop_reg_print("PORT_MAC_CTRL0", MV_XLG_PORT_MAC_CTRL0_REG(port));
	mv_gop_reg_print("PORT_MAC_CTRL1", MV_XLG_PORT_MAC_CTRL1_REG(port));
	mv_gop_reg_print("PORT_MAC_CTRL2", MV_XLG_PORT_MAC_CTRL2_REG(port));
	mv_gop_reg_print("PORT_STATUS", MV_XLG_MAC_PORT_STATUS_REG(port));
	mv_gop_reg_print("PORT_FIFOS_THRS_CFG", MV_XLG_PORT_FIFOS_THRS_CFG_REG(port));
	mv_gop_reg_print("PORT_MAC_CTRL3", MV_XLG_PORT_MAC_CTRL3_REG(port));
	mv_gop_reg_print("PORT_PER_PRIO_FLOW_CTRL_STATUS", MV_XLG_PORT_PER_PRIO_FLOW_CTRL_STATUS_REG(port));
	mv_gop_reg_print("DEBUG_BUS_STATUS", MV_XLG_DEBUG_BUS_STATUS_REG(port));
	mv_gop_reg_print("PORT_METAL_FIX", MV_XLG_PORT_METAL_FIX_REG(port));
	mv_gop_reg_print("XG_MIB_CNTRS_CTRL", MV_XLG_MIB_CNTRS_CTRL_REG(port));
	for (timer = 0; timer < 8; timer++) {
		sprintf(reg_name, "CNCCFC_TIMER%d", timer);
		mv_gop_reg_print(reg_name, MV_XLG_CNCCFC_TIMERI_REG(port, timer));
	}
	mv_gop_reg_print("PPFC_CTRL", MV_XLG_MAC_PPFC_CTRL_REG(port));
	mv_gop_reg_print("FC_DSA_TAG_0", MV_XLG_MAC_FC_DSA_TAG_0_REG(port));
	mv_gop_reg_print("FC_DSA_TAG_1", MV_XLG_MAC_FC_DSA_TAG_1_REG(port));
	mv_gop_reg_print("FC_DSA_TAG_2", MV_XLG_MAC_FC_DSA_TAG_2_REG(port));
	mv_gop_reg_print("FC_DSA_TAG_3", MV_XLG_MAC_FC_DSA_TAG_3_REG(port));
	mv_gop_reg_print("DIC_BUDGET_COMPENSATION", MV_XLG_MAC_DIC_BUDGET_COMPENSATION_REG(port));
	mv_gop_reg_print("PORT_MAC_CTRL4", MV_XLG_PORT_MAC_CTRL4_REG(port));
	mv_gop_reg_print("PORT_MAC_CTRL5", MV_XLG_PORT_MAC_CTRL5_REG(port));
	mv_gop_reg_print("EXT_CTRL", MV_XLG_MAC_EXT_CTRL_REG(port));
	mv_gop_reg_print("MACRO_CTRL", MV_XLG_MAC_MACRO_CTRL_REG(port));
	mv_gop_reg_print("MACRO_CTRL", MV_XLG_MAC_MACRO_CTRL_REG(port));
	mv_gop_reg_print("PORT_INT_MASK", MV_XLG_INTERRUPT_MASK_REG(port));
	mv_gop_reg_print("EXTERNAL_INT_MASK", MV_XLG_EXTERNAL_INTERRUPT_MASK_REG(port));
}

/* Set the MAC to reset or exit from reset */
int mv_xlg_mac_reset(int mac_num, enum mv_reset reset)
{
	u32 reg_addr;
	u32 val;

	reg_addr = MV_XLG_PORT_MAC_CTRL0_REG(mac_num);

	/* read - modify - write */
	val = mv_gop_reg_read(reg_addr);
	if (reset == RESET)
		val &= ~MV_XLG_MAC_CTRL0_MACRESETN_MASK;
	else
		val |= MV_XLG_MAC_CTRL0_MACRESETN_MASK;
	mv_gop_reg_write(reg_addr, val);

	return 0;
}

/* Set the internal mux's to the required MAC in the GOP */
int mv_xlg_mac_mode_cfg(int mac_num, int num_of_act_lanes)
{
	u32 reg_addr;
	u32 val;

	/* Set TX FIFO thresholds */
	reg_addr = MV_XLG_PORT_FIFOS_THRS_CFG_REG(mac_num);
	val = mv_gop_reg_read(reg_addr);
	MV_U32_SET_FIELD(val, MV_XLG_MAC_PORT_FIFOS_THRS_CFG_TXRDTHR_MASK,
		(6 << MV_XLG_MAC_PORT_FIFOS_THRS_CFG_TXRDTHR_OFFS));
	mv_gop_reg_write(reg_addr, val);

	/* configure 10G MAC mode */
	reg_addr = MV_XLG_PORT_MAC_CTRL3_REG(mac_num);
	val = mv_gop_reg_read(reg_addr);
	MV_U32_SET_FIELD(val, MV_XLG_MAC_CTRL3_MACMODESELECT_MASK,
		(1 << MV_XLG_MAC_CTRL3_MACMODESELECT_OFFS));
	mv_gop_reg_write(reg_addr, val);

	reg_addr = MV_XLG_PORT_MAC_CTRL4_REG(mac_num);

	/* read - modify - write */
	val = mv_gop_reg_read(reg_addr);
	MV_U32_SET_FIELD(val, 0x1F10, 0x310);
	mv_gop_reg_write(reg_addr, val);

	/* Jumbo frame support - 0x1400*2= 0x2800 bytes */
	val = mv_gop_reg_read(MV_XLG_PORT_MAC_CTRL1_REG(mac_num));
	MV_U32_SET_FIELD(val, 0x1FFF, 0x1400);
	mv_gop_reg_write(MV_XLG_PORT_MAC_CTRL1_REG(mac_num), val);

	/* mask all port interrupts */
	mv_xlg_port_link_event_mask(mac_num);

	/* unmask link change interrupt */
	val = mv_gop_reg_read(MV_XLG_INTERRUPT_MASK_REG(mac_num));
	val |= MV_XLG_INTERRUPT_LINK_CHANGE_MASK;
	val |= 1; /* unmask summary bit */
	mv_gop_reg_write(MV_XLG_INTERRUPT_MASK_REG(mac_num), val);


	return 0;
}

/* Configure MAC loopback */
int mv_xlg_mac_loopback_cfg(int mac_num, enum mv_lb_type type)
{
	u32 reg_addr;
	u32 val;

	reg_addr = MV_XLG_PORT_MAC_CTRL1_REG(mac_num);
	val = mv_gop_reg_read(reg_addr);
	switch (type) {
	case MV_DISABLE_LB:
		val &= ~MV_XLG_MAC_CTRL1_MACLOOPBACKEN_MASK;
		val &= ~MV_XLG_MAC_CTRL1_XGMIILOOPBACKEN_MASK;
		break;
	case MV_RX_2_TX_LB:
		val &= ~MV_XLG_MAC_CTRL1_MACLOOPBACKEN_MASK;
		val |= MV_XLG_MAC_CTRL1_XGMIILOOPBACKEN_MASK;
		break;
	case MV_TX_2_RX_LB:
		val |= MV_XLG_MAC_CTRL1_MACLOOPBACKEN_MASK;
		val |= MV_XLG_MAC_CTRL1_XGMIILOOPBACKEN_MASK;
		break;
	default:
		return -1;
	}
	mv_gop_reg_write(reg_addr, val);
	return 0;
}

/* Get MAC link status */
bool mv_xlg_mac_link_status_get(int mac_num)
{

	if (mv_gop_reg_read(MV_XLG_MAC_PORT_STATUS_REG(mac_num)) & 1)
		return true;

	return false;
}


/* Enable port and MIB counters update */
void mv_xlg_mac_port_enable(int mac_num)
{
	u32 reg_val;

	reg_val = mv_gop_reg_read(MV_XLG_PORT_MAC_CTRL0_REG(mac_num));
	reg_val |= MV_XLG_MAC_CTRL0_PORTEN_MASK;
	reg_val &= ~MV_XLG_MAC_CTRL0_MIBCNTDIS_MASK;

	mv_gop_reg_write(MV_XLG_PORT_MAC_CTRL0_REG(mac_num), reg_val);
}

/* Disable port */
void mv_xlg_mac_port_disable(int mac_num)
{
	u32 reg_val;

	/* mask all port interrupts */
	mv_xlg_port_link_event_mask(mac_num);

	reg_val = mv_gop_reg_read(MV_XLG_PORT_MAC_CTRL0_REG(mac_num));
	reg_val &= ~MV_XLG_MAC_CTRL0_PORTEN_MASK;

	mv_gop_reg_write(MV_XLG_PORT_MAC_CTRL0_REG(mac_num), reg_val);
}

void mv_xlg_mac_port_periodic_xon_set(int mac_num, int enable)
{
	u32 reg_val;

	reg_val =  mv_gop_reg_read(MV_XLG_PORT_MAC_CTRL0_REG(mac_num));

	if (enable)
		reg_val |= MV_XLG_MAC_CTRL0_PERIODICXONEN_MASK;
	else
		reg_val &= ~MV_XLG_MAC_CTRL0_PERIODICXONEN_MASK;

	mv_gop_reg_write(MV_XLG_PORT_MAC_CTRL0_REG(mac_num), reg_val);
}

int mv_xlg_mac_link_status(int mac_num, struct mv_port_link_status *pstatus)
{
	u32 reg_val;
	u32 mac_mode;
	u32 fc_en;

	reg_val = mv_gop_reg_read(MV_XLG_PORT_MAC_CTRL3_REG(mac_num));
	mac_mode = (reg_val & MV_XLG_MAC_CTRL3_MACMODESELECT_MASK) >> MV_XLG_MAC_CTRL3_MACMODESELECT_OFFS;

	/* speed  and duplex */
	switch (mac_mode) {
	case 0:
		pstatus->speed = MV_PORT_SPEED_1000;
		pstatus->duplex = MV_PORT_DUPLEX_AN;
		break;
	case 1:
		pstatus->speed = MV_PORT_SPEED_10000;
		pstatus->duplex = MV_PORT_DUPLEX_FULL;
		break;
	default:
		return -1;
	}

	/* link status */
	reg_val = mv_gop_reg_read(MV_XLG_MAC_PORT_STATUS_REG(mac_num));
	if (reg_val & MV_XLG_MAC_PORT_STATUS_LINKSTATUS_MASK)
		pstatus->linkup = 1 /*TRUE*/;
	else
		pstatus->linkup = 0 /*FALSE*/;

	/* flow control status */
	fc_en = mv_gop_reg_read(MV_XLG_PORT_MAC_CTRL0_REG(mac_num));
	if (reg_val & MV_XLG_MAC_PORT_STATUS_PORTTXPAUSE_MASK)
		pstatus->tx_fc = MV_PORT_FC_ACTIVE;
	else if (fc_en & MV_XLG_MAC_CTRL0_TXFCEN_MASK)
		pstatus->tx_fc = MV_PORT_FC_ENABLE;
	else
		pstatus->tx_fc = MV_PORT_FC_DISABLE;

	if (reg_val & MV_XLG_MAC_PORT_STATUS_PORTRXPAUSE_MASK)
		pstatus->rx_fc = MV_PORT_FC_ACTIVE;
	else if (fc_en & MV_XLG_MAC_CTRL0_RXFCEN_MASK)
		pstatus->rx_fc = MV_PORT_FC_ENABLE;
	else
		pstatus->rx_fc = MV_PORT_FC_DISABLE;

	return 0;
}

/* Change maximum receive size of the port */
int mv_xlg_mac_max_rx_size_set(int mac_num, int max_rx_size)
{
	u32	reg_val;

	reg_val =  mv_gop_reg_read(MV_XLG_PORT_MAC_CTRL1_REG(mac_num));
	reg_val &= ~MV_XLG_MAC_CTRL1_FRAMESIZELIMIT_MASK;
	reg_val |= (((max_rx_size - MV_MH_SIZE) / 2) << MV_XLG_MAC_CTRL1_FRAMESIZELIMIT_OFFS);
	mv_gop_reg_write(MV_XLG_PORT_MAC_CTRL1_REG(mac_num), reg_val);

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
int mv_xlg_mac_force_link_mode_set(int mac_num, bool force_link_up, bool force_link_down)
{
	u32 reg_val;

	/* Can't force link pass and link fail at the same time */
	if ((force_link_up) && (force_link_down))
		return -EINVAL;

	reg_val = mv_gop_reg_read(MV_XLG_PORT_MAC_CTRL0_REG(mac_num));

	if (force_link_up)
		reg_val |= MV_XLG_MAC_CTRL0_FORCELINKPASS_MASK;
	else
		reg_val &= ~MV_XLG_MAC_CTRL0_FORCELINKPASS_MASK;

	if (force_link_down)
		reg_val |= MV_XLG_MAC_CTRL0_FORCELINKDOWN_MASK;
	else
		reg_val &= ~MV_XLG_MAC_CTRL0_FORCELINKDOWN_MASK;

	mv_gop_reg_write(MV_XLG_PORT_MAC_CTRL0_REG(mac_num), reg_val);

	return 0;
}

/* Sets port speed to Auto Negotiation / 1000 / 100 / 10 Mbps.
*  Sets port duplex to Auto Negotiation / Full / Half Duplex.
*/
int mv_xlg_mac_speed_duplex_set(int mac_num, enum mv_port_speed speed, enum mv_port_duplex duplex)
{
	/* not supported */
	return -1;
}

/* Gets port speed and duplex */
int mv_xlg_mac_speed_duplex_get(int mac_num, enum mv_port_speed *speed, enum mv_port_duplex *duplex)
{
	/* not supported */
	return -1;
}

/* Configure the port's Flow Control properties */
int mv_xlg_mac_fc_set(int mac_num, enum mv_port_fc fc)
{
	u32 reg_val;

	reg_val = mv_gop_reg_read(MV_XLG_PORT_MAC_CTRL0_REG(mac_num));

	switch (fc) {
	case MV_PORT_FC_DISABLE:
		reg_val &= ~MV_XLG_MAC_CTRL0_RXFCEN_MASK;
		reg_val &= ~MV_XLG_MAC_CTRL0_TXFCEN_MASK;
		break;

	case MV_PORT_FC_ENABLE:
		reg_val |= MV_XLG_MAC_CTRL0_RXFCEN_MASK;
		reg_val |= MV_XLG_MAC_CTRL0_TXFCEN_MASK;
		break;

	case MV_PORT_FC_AN_NO:
	case MV_PORT_FC_AN_SYM:
	case MV_PORT_FC_AN_ASYM:
	default:
		pr_err("XLG MAC: Unexpected FlowControl value %d\n", fc);
		return -EINVAL;
	}

	mv_gop_reg_write(MV_XLG_PORT_MAC_CTRL0_REG(mac_num), reg_val);
	return 0;
}

/* Get Flow Control configuration of the port */
void mv_xlg_mac_fc_get(int mac_num, enum mv_port_fc *fc)
{
	u32 reg_val;

	/* No auto negotiation for flow control */
	reg_val = mv_gop_reg_read(MV_XLG_PORT_MAC_CTRL0_REG(mac_num));

	if ((reg_val & MV_XLG_MAC_CTRL0_RXFCEN_MASK) && (reg_val & MV_XLG_MAC_CTRL0_TXFCEN_MASK))
		*fc = MV_PORT_FC_ENABLE;
	else
		*fc = MV_PORT_FC_DISABLE;
}

int mv_xlg_mac_port_link_speed_fc(int mac_num, enum mv_port_speed speed, int force_link_up)
{
	if (force_link_up) {
		if (mv_xlg_mac_fc_set(mac_num, MV_PORT_FC_ENABLE)) {
			pr_err("mv_xlg_mac_fc_set failed\n");
			return -EPERM;
		}
		if (mv_xlg_mac_force_link_mode_set(mac_num, 1, 0)) {
			pr_err("mv_xlg_mac_force_link_mode_set failed\n");
			return -EPERM;
		}
	} else {
		if (mv_xlg_mac_force_link_mode_set(mac_num, 0, 0)) {
			pr_err("mv_xlg_mac_force_link_mode_set failed\n");
			return -EPERM;
		}
		if (mv_xlg_mac_fc_set(mac_num, MV_PORT_FC_AN_SYM)) {
			pr_err("mv_xlg_mac_fc_set failed\n");
			return -EPERM;
		}
	}

	return 0;
}

void mv_xlg_port_link_event_mask(int mac_num)
{
	u32 reg_val;

	reg_val = mv_gop_reg_read(MV_XLG_EXTERNAL_INTERRUPT_MASK_REG(mac_num));
	reg_val &= ~(1 << 1);
	mv_gop_reg_write(MV_XLG_EXTERNAL_INTERRUPT_MASK_REG(mac_num), reg_val);
}

void mv_xlg_port_external_event_unmask(int mac_num, int bit_2_open)
{
	u32 reg_val;

	reg_val = mv_gop_reg_read(MV_XLG_EXTERNAL_INTERRUPT_MASK_REG(mac_num));
	reg_val |= (1 << bit_2_open);
	reg_val |= 1; /* unmask summary bit */
	mv_gop_reg_write(MV_XLG_EXTERNAL_INTERRUPT_MASK_REG(mac_num), reg_val);
}

void mv_xlg_port_link_event_clear(int mac_num)
{
	u32 reg_val;

	reg_val = mv_gop_reg_read(MV_XLG_INTERRUPT_CAUSE_REG(mac_num));
}

void mv_xlg_2_gig_mac_cfg(int mac_num)
{
	u32 reg_val;

	/* relevant only for MAC0 (XLG0 and GMAC0) */
	if (mac_num > 0)
		return;

	/* configure 1Gig MAC mode */
	reg_val = mv_gop_reg_read(MV_XLG_PORT_MAC_CTRL3_REG(mac_num));
	MV_U32_SET_FIELD(reg_val, MV_XLG_MAC_CTRL3_MACMODESELECT_MASK,
		(0 << MV_XLG_MAC_CTRL3_MACMODESELECT_OFFS));
	mv_gop_reg_write(MV_XLG_PORT_MAC_CTRL3_REG(mac_num), reg_val);
}
