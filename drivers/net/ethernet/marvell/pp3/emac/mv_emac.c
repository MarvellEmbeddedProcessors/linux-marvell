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
#include "common/mv_hw_if.h"
#include "emac/mv_emac.h"
#include "mv_emac_regs.h"

static struct mv_pp3_emac_ctrl *pp3_emac;

/*--------------------------------------------------------------*/
/*--------------------- EMAC globals ---------------------------*/
/*--------------------------------------------------------------*/


u32 mv_pp3_emac_reg_read(int port, u32 reg)
{
	u32 reg_data;

	reg_data = mv_pp3_hw_reg_read(reg + pp3_emac[port].base);

	if (pp3_emac[port].flags & MV_PP3_EMAC_F_DEBUG)
		pr_info("read     : 0x%04x = 0x%08x\n", reg, reg_data);

	return reg_data;
}

void mv_pp3_emac_reg_write(int port, u32 reg, u32 data)
{
	mv_pp3_hw_reg_write(reg + pp3_emac[port].base, data);

	if (pp3_emac[port].flags & MV_PP3_EMAC_F_DEBUG) {
		u32 reg_data;
		pr_info("write    : 0x%04x = 0x%08x\n", reg, data);
		reg_data = mv_pp3_hw_reg_read(reg + pp3_emac[port].base);
		pr_info("read back: 0x%04x = 0x%08x\n", reg, reg_data);
	}
}

static void mv_pp3_emac_reg_print(int port, char *reg_name, u32 reg)
{
	pr_info("  %-32s: 0x%04x = 0x%08x\n", reg_name, reg, mv_pp3_emac_reg_read(port, reg));
}

int mv_pp3_emac_global_init(int emacs_num)
{
	if (pp3_emac != NULL) {
		pr_warn("EMAC component is already initialized\n");
		return -1;
	}
	pp3_emac = kzalloc(sizeof(struct mv_pp3_emac_ctrl) * emacs_num, GFP_KERNEL);
	if (pp3_emac == NULL) {
		pr_err("%s: Memory allocation of %d bytes failed\n", __func__,
				sizeof(struct mv_pp3_emac_ctrl) * emacs_num);
		return -ENOMEM;
	}
	return 0;
}

void mv_pp3_emac_unit_base(int port, void __iomem *base)
{
	pp3_emac[port].base = base;
	pp3_emac[port].flags |= MV_PP3_EMAC_F_ATTACH;
}

void mv_pp3_emac_init(int port, int qmp, int qmq)
{
	mv_pp3_emac_qm_mapping(port, qmp, qmq);
	mv_pp3_emac_fw_data(port);
	mv_pp3_emac_deq_undrn_threshold(port, MV_EMAC_DEQ_CR_FIFO_UNDRN_PROT_TH_DEF);

	/* use hw defaults */
	/*mv_pp3_emac_rx_cfh_reorder_mode(int port, int lock_id);*/
	/*mv_pp3_emac_rx_cfh_deq_mode(port, mode)*/
	/*mv_pp3_emac_mh_en(port, 1);*/
	/*mv_pp3_emac_rx_desc_rsvd(port, 2);*/
	/* mv_pp3_emac_tx_min_pkt_len(int port, 60)*/
	/*mv_pp3_emac_rx_enable(port,1);*/
	/*mv_pp3_emac_loopback(port, 0)*/
}

/* enable debug flag */
void mv_pp3_emac_debug(int port, int en)
{
	if (en)
		pp3_emac[port].flags |= MV_PP3_EMAC_F_DEBUG;
	else
		pp3_emac[port].flags &= ~MV_PP3_EMAC_F_DEBUG;


}

/* set logical port and CFH mode, used by FW */
void mv_pp3_emac_fw_data(int port)
{
	u32 data;

	data = mv_pp3_emac_reg_read(port, MV_EMAC_ENQ_DESC_W1_REG);

	data &= ~(MV_EMAC_ENQ_DESC_W1_FW_CFH_MODE_MASK | MV_EMAC_ENQ_DESC_W1_FW_LOGIC_PORT_MASK);

	data |= (port << MV_EMAC_ENQ_DESC_W1_FW_LOGIC_PORT_OFFS) |
		(1 << MV_EMAC_ENQ_DESC_W1_FW_CFH_MODE_OFFS);

	mv_pp3_emac_reg_write(port, MV_EMAC_ENQ_DESC_W1_REG, data);
}

/* set QM Enq queu and Deq port */
void mv_pp3_emac_qm_mapping(int port, int qmp, int qmq)
{
	u32 data;

	data = (qmq << MV_EMAC_AXI_CFG_AXI4_ENQ_QUEUE_NUM_OFFS) |
		(qmp << MV_EMAC_AXI_CFG_AXI4_DEQ_PORT_NUM_OFFS);

	mv_pp3_emac_reg_write(port, MV_EMAC_AXI_CFG_REG, data);
}

/* add mh in rx and strip in tx - enable/disable */
void mv_pp3_emac_mh_en(int port, int en)
{
	u32 data_rx, data_tx;

	data_rx = mv_pp3_emac_reg_read(port, MV_EMAC_ENQ_CR_REG);
	data_tx = mv_pp3_emac_reg_read(port, MV_EMAC_DEQ_CR_REG);

	if (en) {
		data_rx |= MV_EMAC_ENQ_CR_ADD_MH_MASK;
		data_tx |= MV_EMAC_DEQ_CR_STRIP_MH_MASK;
	} else {
		data_rx &= ~MV_EMAC_ENQ_CR_ADD_MH_MASK;
		data_tx &= ~MV_EMAC_DEQ_CR_STRIP_MH_MASK;
	}

	mv_pp3_emac_reg_write(port, MV_EMAC_ENQ_CR_REG, data_rx);
	mv_pp3_emac_reg_write(port, MV_EMAC_DEQ_CR_REG, data_tx);
}

/* Set dequeue packet FIFO under run protection threshold in units of 16 bytes*/
void mv_pp3_emac_deq_undrn_threshold(int port, int thr)
{
	u32 reg_val;

	reg_val = mv_pp3_emac_reg_read(port, MV_EMAC_DEQ_CR_REG);

	reg_val &= ~MV_EMAC_DEQ_CR_FIFO_UNDRN_PROT_TH_MASK;
	reg_val |= (thr << MV_EMAC_DEQ_CR_FIFO_UNDRN_PROT_TH_OFFS);

	mv_pp3_emac_reg_write(port, MV_EMAC_DEQ_CR_REG, reg_val);
}
/*
set source of timestamp
	from = 0: from RTC
	from = 1: from descriptor
*/
void mv_pp3_emac_ts(int port, int from)
{
	u32 data_rx, data_tx;

	data_rx = mv_pp3_emac_reg_read(port, MV_EMAC_ENQ_CR_REG);
	data_tx = mv_pp3_emac_reg_read(port, MV_EMAC_DEQ_CR_REG);

	if (from) {
		data_rx |= MV_EMAC_ENQ_CR_TIMESTAMP_FROM_DESCRIPTOR_MASK;
		data_tx |= MV_EMAC_DEQ_CR_TX_SOP_DESC_INGRS_TIME_STMP_FROM_CFH_MASK;
	} else {
		data_rx &= ~MV_EMAC_ENQ_CR_TIMESTAMP_FROM_DESCRIPTOR_MASK;
		data_tx &= ~MV_EMAC_DEQ_CR_TX_SOP_DESC_INGRS_TIME_STMP_FROM_CFH_MASK;
	}

	mv_pp3_emac_reg_write(port, MV_EMAC_ENQ_CR_REG, data_rx);
	mv_pp3_emac_reg_write(port, MV_EMAC_DEQ_CR_REG, data_tx);
}

/* dump emac registers */
void mv_pp3_emac_regs(int port)
{
	mv_pp3_emac_reg_print(port, "CR", MV_EMAC_CR_REG);
	mv_pp3_emac_reg_print(port, "INT_MASK", MV_EMAC_INT_MASK_REG);
	mv_pp3_emac_reg_print(port, "STATUS", MV_EMAC_STATUS_REG);
	mv_pp3_emac_reg_print(port, "ENQ", MV_EMAC_ENQ_CR_REG);
	mv_pp3_emac_reg_print(port, "ENQ_CFH_MH", MV_EMAC_ENQ_CFH_MH_REG);
	mv_pp3_emac_reg_print(port, "ENQ_DESC_W0", MV_EMAC_ENQ_DESC_W0_REG);
	mv_pp3_emac_reg_print(port, "ENQ_DESC_W1", MV_EMAC_ENQ_DESC_W1_REG);
	mv_pp3_emac_reg_print(port, "AXI_CFG", MV_EMAC_AXI_CFG_REG);
	mv_pp3_emac_reg_print(port, "DEQ_CR", MV_EMAC_DEQ_CR_REG);
	mv_pp3_emac_reg_print(port, "MIN_PKT_LEN", MV_EMAC_MIN_PKT_LEN_REG);
	mv_pp3_emac_reg_print(port, "DEQ_RTC_STRM_VAL", MV_EMAC_DEQ_RTC_STRM_VAL_REG);
	mv_pp3_emac_reg_print(port, "DEQ_RTC_PORT_VAL", MV_EMAC_DEQ_RTC_PORT_VAL_REG);

	pr_info("------------------------ debug regs ---------------------\n");

	mv_pp3_emac_reg_print(port, "DBG_SM_STATUS", MV_EMAC_DBG_SM_STATUS_REG);
	mv_pp3_emac_reg_print(port, "DBG_FIFO_FILL_LVL1", MV_EMAC_DBG_FIFO_FILL_LVL1_REG);
	mv_pp3_emac_reg_print(port, "DBG_FIFO_FILL_LVL2", MV_EMAC_DBG_FIFO_FILL_LVL2_REG);
}


/* dump emac status */
void mv_pp3_emac_status(int port)
{
	u32 reg_val;

	pr_info("settings for emac%d:\n", port);

	/*-------------------------------------------------------------------*/

	reg_val = mv_pp3_emac_reg_read(port, MV_EMAC_CR_REG);
	pr_info("HW loopback: %s\n", (reg_val & MV_EMAC_CR_LOOPBACK_EN_MASK) ? "on" : "off");
	pr_info("RX enq: %s\n", (reg_val & MV_EMAC_CR_ENQ_EN_MASK) ? "on" : "off");

	/*-------------------------------------------------------------------*/

	reg_val = mv_pp3_emac_reg_read(port, MV_EMAC_ENQ_CR_REG);
	pr_info("RX add MH: %s\n", (reg_val & MV_EMAC_ENQ_CR_ADD_MH_MASK) ? "on" : "off");
	pr_info("RX timestamp source: %s\n",
			(reg_val & MV_EMAC_ENQ_CR_TIMESTAMP_FROM_DESCRIPTOR_MASK) ? "descriptor" : "RTC");
	pr_info("RX CFH wr offs: %d\n",
			((MV_EMAC_ENQ_CR_CFH_OFFSET_MASK & reg_val) >>
			MV_EMAC_ENQ_CR_CFH_OFFSET_OFFS) * 8);

	/*-------------------------------------------------------------------*/

	reg_val = mv_pp3_emac_reg_read(port, MV_EMAC_ENQ_DESC_W1_REG);
	pr_info("RX CFH wr mode: %d (0-HMAC, 1-EMAC, 2-CMAC, 3-RADIO)\n",
			(MV_EMAC_ENQ_DESC_W1_FW_CFH_MODE_MASK & reg_val) >> MV_EMAC_ENQ_DESC_W1_FW_CFH_MODE_OFFS);
	pr_info("RX CFH wr port: %d\n",
			(MV_EMAC_ENQ_DESC_W1_FW_LOGIC_PORT_MASK & reg_val) >> MV_EMAC_ENQ_DESC_W1_FW_LOGIC_PORT_OFFS);

	/*-------------------------------------------------------------------*/

	reg_val = mv_pp3_emac_reg_read(port, MV_EMAC_ENQ_CFH_MH_REG);
	pr_info("RX MH val: %d\n", reg_val &
			(MV_EMAC_ENQ_CFH_MH_EMAC_ENQ_CFH_MH_MASK & reg_val) >> MV_EMAC_ENQ_CFH_MH_EMAC_ENQ_CFH_MH_OFFS);

	/*-------------------------------------------------------------------*/

	reg_val = mv_pp3_emac_reg_read(port, MV_EMAC_ENQ_DESC_W0_REG);
	pr_info("RX lock id: %d\n", reg_val &
			(MV_EMAC_ENQ_DESC_W0_LOCKID_MASK & reg_val) >> MV_EMAC_ENQ_DESC_W0_LOCKID_OFFS);
	pr_info("RX reorder mode: %d\n", reg_val &
			(MV_EMAC_ENQ_DESC_W0_REORDER_MODE_MASK & reg_val) >> MV_EMAC_ENQ_DESC_W0_REORDER_MODE_OFFS);
	pr_info("TX deq mode: %d\n", reg_val &
			(MV_EMAC_ENQ_DESC_W0_DEQ_MODE_MASK & reg_val) >> MV_EMAC_ENQ_DESC_W0_DEQ_MODE_OFFS);

	/*-------------------------------------------------------------------*/

	reg_val = mv_pp3_emac_reg_read(port, MV_EMAC_DEQ_CR_REG);
	pr_info("TX strm CFH size: %d\n",
			((MV_EMAC_DEQ_CR_STRM_DESC_SIZE_MASK & reg_val) >>
			MV_EMAC_DEQ_CR_STRM_DESC_SIZE_OFFS) * 16);
	pr_info("TX srip MH: %s\n",
			(reg_val & MV_EMAC_DEQ_CR_STRIP_MH_MASK) ? "on" : "off");
	pr_info("TX timestamp source: %s\n",
			(reg_val & MV_EMAC_DEQ_CR_TX_SOP_DESC_INGRS_TIME_STMP_FROM_CFH_MASK) ?
			"descriptor" : "RTC");

	/*-------------------------------------------------------------------*/

	reg_val = mv_pp3_emac_reg_read(port, MV_EMAC_MIN_PKT_LEN_REG);
	pr_info("TX min packet length: %d\n",
			(MV_EMAC_MIN_PKT_LEN_DEQ_MASK & reg_val) >>
			MV_EMAC_MIN_PKT_LEN_DEQ_OFFS);
}

/* enable/disable loopback forom TX to RX */
void mv_pp3_emac_loopback(int port, int lb)
{
	u32 data;

	data = mv_pp3_emac_reg_read(port, MV_EMAC_CR_REG);

	if (lb)
		data |= MV_EMAC_CR_LOOPBACK_EN_MASK;
	else
		data &= ~MV_EMAC_CR_LOOPBACK_EN_MASK;

	mv_pp3_emac_reg_write(port, MV_EMAC_CR_REG, data);
}

/* enable/disable emac rx */
void mv_pp3_emac_rx_enable(int port, int en)
{
	u32 data;

	data = mv_pp3_emac_reg_read(port, MV_EMAC_CR_REG);

	if (en)
		data |= MV_EMAC_CR_ENQ_EN_MASK;
	else
		data &= ~MV_EMAC_CR_ENQ_EN_MASK;

	mv_pp3_emac_reg_write(port, MV_EMAC_CR_REG, data);

	/*pr_info("\nEMAC #%d - %s\n", port, en ? "enabled" : "disabled");*/

	/*TODO do we need to wait to interrupt bit*/
}

/*
set offset to CFH after 16B descriptor
	bytes: 0-64 in multiples of 8 (0, 8, 16, etc.)
*/
int mv_pp3_emac_rx_desc_rsvd(int port, int bytes)
{
	u32 data;

	/* TODO: do we need bytes validation here ? */
	if (bytes % 8)
		return 1;

	data = mv_pp3_emac_reg_read(port, MV_EMAC_ENQ_CR_REG);

	data &= ~MV_EMAC_ENQ_CR_CFH_OFFSET_MASK;
	data |= ((bytes/8) << MV_EMAC_ENQ_CR_CFH_OFFSET_OFFS);

	mv_pp3_emac_reg_write(port, MV_EMAC_ENQ_CR_REG, data);

	return 0;
}

/* set mh value, relevant only if mh is enable */
void mv_pp3_emac_rx_mh(int port, short mh)
{
	u32 data;

	data = mv_pp3_emac_reg_read(port, MV_EMAC_ENQ_CFH_MH_REG);

	data &= ~MV_EMAC_ENQ_CFH_MH_EMAC_ENQ_CFH_MH_MASK;
	data |= (mh << MV_EMAC_ENQ_CFH_MH_EMAC_ENQ_CFH_MH_OFFS);

	mv_pp3_emac_reg_write(port, MV_EMAC_ENQ_CFH_MH_REG, data);
}

/*
set the minimum packet length that will sent to GOP
	bytes: length in bytes, valid range [16, 64]
*/
void mv_pp3_emac_tx_min_pkt_len(int port, int bytes)
{
	u32 data;

	/* TODO: do we need bytes validation here ? */

	data = mv_pp3_emac_reg_read(port, MV_EMAC_MIN_PKT_LEN_REG);

	data &= ~MV_EMAC_MIN_PKT_LEN_DEQ_MASK;
	data |= (bytes << MV_EMAC_MIN_PKT_LEN_DEQ_OFFS);

	mv_pp3_emac_reg_write(port, MV_EMAC_MIN_PKT_LEN_REG, data);
}
/*
set CFH lockId
*/
void mv_pp3_emac_rx_cfh_lock_id(int port, int lock_id)
{
	u32 data;

	data = mv_pp3_emac_reg_read(port, MV_EMAC_ENQ_DESC_W0_REG);
	data &= ~MV_EMAC_ENQ_DESC_W0_LOCKID_MASK;
	data |= (lock_id << MV_EMAC_ENQ_DESC_W0_LOCKID_OFFS);

	mv_pp3_emac_reg_write(port, MV_EMAC_ENQ_DESC_W0_REG, data);
}

/*
set CFH reorder mode
*/
void mv_pp3_emac_rx_cfh_reorder_mode(int port, int mode)
{
	u32 data;

	data = mv_pp3_emac_reg_read(port, MV_EMAC_ENQ_DESC_W0_REG);
	data &= ~MV_EMAC_ENQ_DESC_W0_REORDER_MODE_MASK;
	data |= (mode << MV_EMAC_ENQ_DESC_W0_REORDER_MODE_OFFS);

	mv_pp3_emac_reg_write(port, MV_EMAC_ENQ_DESC_W0_REG, data);
}

/*
set CFH deq mode
*/
void mv_pp3_emac_rx_cfh_deq_mode(int port, int mode)
{
	u32 data;

	data = mv_pp3_emac_reg_read(port, MV_EMAC_ENQ_DESC_W0_REG);
	data &= ~MV_EMAC_ENQ_DESC_W0_DEQ_MODE_MASK;
	data |= (mode << MV_EMAC_ENQ_DESC_W0_DEQ_MODE_OFFS);

	mv_pp3_emac_reg_write(port, MV_EMAC_ENQ_DESC_W0_REG, data);
}

u32 mv_pp3_emac_drop_packets_cntr_get(int port)
{
	u32 reg_val;

	reg_val = mv_pp3_emac_reg_read(port, MV_EMAC_ENQ_DRP_PKT_CNT_REG);
	if (pp3_emac[port].flags & MV_PP3_EMAC_F_ROC_CNT_EN)
		return reg_val;

	pp3_emac[port].enq_drop_cnt += reg_val;
	return pp3_emac[port].enq_drop_cnt;
}

void mv_pp3_emac_counters_clear(int port)
{
	u32 reg_val;

	reg_val = mv_pp3_emac_reg_read(port, MV_EMAC_ENQ_DRP_PKT_CNT_REG);
	reg_val = mv_pp3_emac_reg_read(port, MV_EMAC_ENQ_XOFF_CNT_REG);

	pp3_emac[port].enq_drop_cnt = 0;
	pp3_emac[port].enq_xoff_cnt = 0;
}

void mv_pp3_emac_counters_show(int port)
{
	u32 drop, xoff;

	pr_info("\n-------------- EMAC %d counters -----------", port);

	drop = mv_pp3_emac_reg_read(port, MV_EMAC_ENQ_DRP_PKT_CNT_REG);
	xoff = mv_pp3_emac_reg_read(port, MV_EMAC_ENQ_XOFF_CNT_REG);
	if (!(pp3_emac[port].flags & MV_PP3_EMAC_F_ROC_CNT_EN)) {
		pp3_emac[port].enq_drop_cnt += drop;
		pp3_emac[port].enq_xoff_cnt += xoff;
		drop = pp3_emac[port].enq_drop_cnt;
		xoff = pp3_emac[port].enq_xoff_cnt;
	}
	pr_info("  %-32s: %-8u\n", "ENQ_DRP_PKT_CNT", drop);
	pr_info("  %-32s: %-8u\n", "ENQ_XOFF_CNT", xoff);
}


void mv_pp3_emac_pfc_regs(int port)
{
	/* TODO: print PFC registers */
	return;
}

void mv_pp3_emac_sleep_state(int port, bool en)
{
	u32 reg_val;

	reg_val = mv_pp3_emac_reg_read(port, MV_EMAC_CR_REG);
	if (en)
		reg_val |= MV_EMAC_CR_SLEEP_MASK;
	else
		reg_val &= (~MV_EMAC_CR_SLEEP_MASK);

	mv_pp3_emac_reg_write(port, MV_EMAC_CR_REG, reg_val);

	return;
}
