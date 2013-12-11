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
Marvell Commercial License Option

If you received this File from Marvell and you have entered into a commercial
license agreement (a "Commercial License") with Marvell, the File is licensed
to you under the terms of the applicable Commercial License.

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
********************************************************************************
Marvell BSD License Option

If you received this File from Marvell, you may opt to use, redistribute and/or
modify this File under the following licensing terms.
Redistribution and use in source and binary forms, with or without modification,
are permitted provided that the following conditions are met:

    *   Redistributions of source code must retain the above copyright notice,
	this list of conditions and the following disclaimer.

    *   Redistributions in binary form must reproduce the above copyright
	notice, this list of conditions and the following disclaimer in the
	documentation and/or other materials provided with the distribution.

    *   Neither the name of Marvell nor the names of its contributors may be
	used to endorse or promote products derived from this software without
	specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

*******************************************************************************/

/* includes */
#include <linux/kernel.h>
#include <linux/io.h>
#include <linux/slab.h>
#include "common/mv_hw_if.h"
#include "emac/mv_emac.h"
#include "emac/mv_emac_regs.h"

static struct mv_pp3_emac_ctrl pp3_emac[MV_PP3_EMAC_MAX];

/*--------------------------------------------------------------*/
/*--------------------- EMAC globals ---------------------------*/
/*--------------------------------------------------------------*/


u32 mv_pp3_emac_reg_read(int port, u32 reg)
{
	u32 reg_data;

	mv_pp3_hw_read(reg + pp3_emac[port].base, 1, &reg_data);

	if (pp3_emac[port].flags & MV_PP3_EMAC_F_DEBUG)
		pr_info("read     : 0x%x = 0x%08x\n", reg, reg_data);

	return reg_data;
}

void mv_pp3_emac_reg_write(int port, u32 reg, u32 data)
{
	mv_pp3_hw_write(reg + pp3_emac[port].base, 1, &data);

	if (pp3_emac[port].flags & MV_PP3_EMAC_F_DEBUG) {
		u32 reg_data;
		pr_info("write    : 0x%x = 0x%08x\n", reg, data);
		mv_pp3_hw_read(reg + pp3_emac[port].base, 1, &reg_data);
		pr_info("read back: 0x%x = 0x%08x\n", reg, reg_data);
	}
}

static void mv_pp3_emac_reg_print(int port, char *reg_name, u32 reg)
{
	pr_info("  %-32s: 0x%x = 0x%08x\n", reg_name, reg, mv_pp3_emac_reg_read(port, reg));
}

void mv_pp3_emac_init(int port, u32 base)
{
	/* attach to QM */
	/* TODO config the correct values of qm_q and qm_port */
	mv_pp3_emac_qm_mapping(port, port, port);

	/* enable MH */
	mv_pp3_emac_mh_en(port, 1);

	pp3_emac[port].base = base;
	pp3_emac[port].flags |= MV_PP3_EMAC_F_ATTACH;
}

/* enable debug flag */
void mv_pp3_emac_debug(int port, int en)
{
	if (en)
		pp3_emac[port].flags |= MV_PP3_EMAC_F_DEBUG_BIT;
	else
		pp3_emac[port].flags &= ~MV_PP3_EMAC_F_DEBUG_BIT;
}

/* set QM Enq queu and Deq port */
void mv_pp3_emac_qm_mapping(int port, int qm_port, int qm_q)
{
	u32 data;

	data = (qm_q << MV_EMAC_AXI_CFG_AXI4_ENQ_QUEUE_NUM_OFFS) |
		(qm_port << MV_EMAC_AXI_CFG_AXI4_DEQ_PORT_NUM_OFFS);

	mv_pp3_emac_reg_write(port, MV_EMAC_AXI_CFG_REG, data);
}

/* add mh in rx and strip in tx - enable/disable */
void mv_pp3_emac_mh_en(int port, int en)
{
	u32 data_rx, data_tx;

	data_rx = mv_pp3_emac_reg_read(port, MV_EMAC_ENQ_CR_REG);
	data_tx = mv_pp3_emac_reg_read(port, MV_EMAC_DEQ_CR_REG);

	if (en) {
		data_rx |= MV_EMAC_ENQ_CR_ENQ_CR_ADD_MH_MASK;
		data_tx |= MV_EMAC_DEQ_CR_DEQ_STRIP_MH_MASK;
	} else {
		data_rx &= ~MV_EMAC_ENQ_CR_ENQ_CR_ADD_MH_MASK;
		data_tx &= ~MV_EMAC_DEQ_CR_DEQ_STRIP_MH_MASK;
	}

	mv_pp3_emac_reg_write(port, MV_EMAC_ENQ_CR_REG, data_rx);
	mv_pp3_emac_reg_write(port, MV_EMAC_DEQ_CR_REG, data_tx);
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
		data_rx |= MV_EMAC_ENQ_CR_ENQ_CR_TIMESTAMP_FROM_DESCRIPTOR_MASK;
		data_tx |= MV_EMAC_DEQ_CR_DEQ_TX_SOP_DESC_INGRS_TIME_STMP_FROM_CFH_MASK;
	} else {
		data_rx &= ~MV_EMAC_ENQ_CR_ENQ_CR_TIMESTAMP_FROM_DESCRIPTOR_MASK;
		data_tx &= ~MV_EMAC_DEQ_CR_DEQ_TX_SOP_DESC_INGRS_TIME_STMP_FROM_CFH_MASK;
	}

	mv_pp3_emac_reg_write(port, MV_EMAC_ENQ_CR_REG, data_rx);
	mv_pp3_emac_reg_write(port, MV_EMAC_DEQ_CR_REG, data_tx);
}

/* dump emac registers */
void mv_pp3_emac_regs(int port)
{
	mv_pp3_emac_reg_print(port, "CR", MV_EMAC_CR_REG);
	mv_pp3_emac_reg_print(port, "ENQ", MV_EMAC_ENQ_CR_REG);
	mv_pp3_emac_reg_print(port, "ENQ_CFH_MH", MV_EMAC_ENQ_CFH_MH_REG);
	mv_pp3_emac_reg_print(port, "ENQ_DESC_W0", MV_EMAC_ENQ_DESC_W0_REG);
	mv_pp3_emac_reg_print(port, "ENQ_DESC_W1", MV_EMAC_ENQ_DESC_W1_REG);
	mv_pp3_emac_reg_print(port, "AXI_CFG", MV_EMAC_AXI_CFG_REG);
	mv_pp3_emac_reg_print(port, "ENQ_DRP_PKT_CNT", MV_EMAC_ENQ_DRP_PKT_CNT_REG);
	mv_pp3_emac_reg_print(port, "DEQ_CR", MV_EMAC_DEQ_CR_REG);
	mv_pp3_emac_reg_print(port, "MIN_PKT_LEN", MV_EMAC_MIN_PKT_LEN_REG);
	mv_pp3_emac_reg_print(port, "DEQ_RTC_STRM_VAL", MV_EMAC_DEQ_RTC_STRM_VAL_REG);
	mv_pp3_emac_reg_print(port, "DEQ_RTC_PORT_VAL", MV_EMAC_DEQ_RTC_PORT_VAL_REG);

	pr_info("-------------- debug regs -----------\n");

	mv_pp3_emac_reg_print(port, "DBG_SM_STATUS", MV_EMAC_DBG_SM_STATUS_REG);
	mv_pp3_emac_reg_print(port, "DBG_FIFO_FILL_LVL1", MV_EMAC_DBG_FIFO_FILL_LVL1_REG);
	mv_pp3_emac_reg_print(port, "DBG_FIFO_FILL_LVL2", MV_EMAC_DBG_FIFO_FILL_LVL2_REG);
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

	data &= ~MV_EMAC_ENQ_CR_ENQ_CR_CFH_OFFSET_MASK;
	data |= ((bytes/8) << MV_EMAC_ENQ_CR_ENQ_CR_CFH_OFFSET_OFFS);

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

	data &= ~MV_EMAC_MIN_PKT_LEN_DEQ_MIN_PKT_LEN_MASK;
	data |= (bytes << MV_EMAC_MIN_PKT_LEN_DEQ_MIN_PKT_LEN_OFFS);

	mv_pp3_emac_reg_write(port, MV_EMAC_MIN_PKT_LEN_REG, data);
}

/*--------------------------------------------------------------*/
/*------------------------- PFC --------------------------------*/
/*--------------------------------------------------------------*/

/* dump emac pfc registers */
void mv_pp3_emac_pfc_regs(int port)
{
	int i;
	/*char buf[64];*/

	mv_pp3_emac_reg_print(port, "ENQ_XOFF_CNT", MV_EMAC_ENQ_XOFF_CNT_REG);

	pr_info("/n");

	for (i = 0; i < MV_EMAC_PFC_PRIO_MAX; i++) {
		/* TODO: open next line */
		/* pr_info(buf, "%s[%d]", "PFC_TBL_ADDR", i); */
		mv_pp3_emac_reg_print(port, "PFC_TBL_ADDR", MV_EMAC_PFC_TBL_ADDR_REG(i));
	}

	pr_info("/n");

	for (i = 0; i < MV_EMAC_PFC_PRIO_MAX; i++) {
		/* TODO: open next line */
		/* mvOsSPrintf(buf, "%s[%d]", "PFC_TBL_PAUSE", i);*/
		mv_pp3_emac_reg_print(port, "PFC_TBL_PAUSE", MV_EMAC_PFC_TBL_PAUSE_VAL_REG(i));
	}

	pr_info("/n");

	for (i = 0; i < MV_EMAC_PFC_PRIO_MAX; i++) {
		/* TODO: open next line */
		/* mvOsSPrintf(buf, "%s[%d]", "PFC_TBL_RESUME", i);*/
		mv_pp3_emac_reg_print(port, "PFC_TBL_RESUME", MV_EMAC_PFC_TBL_RESUME_VAL_REG(i));
	}
}

/*
set pfc table address
	prio: priority 0-7
	val: address
*/
void mv_pp3_emac_pfc_tbl_addr(int port, int prio, u32 val)
{
	/* TODO: add parameters validation */
	mv_pp3_emac_reg_write(port, MV_EMAC_PFC_TBL_ADDR_REG(prio), val);
}
/*
set pfc table address
	prio: priority 0-7
	val: pause value
*/
void mv_pp3_emac_pfc_tbl_pause(int port, int prio, u32 val)
{
	mv_pp3_emac_reg_write(port, MV_EMAC_PFC_TBL_PAUSE_VAL_REG(prio), val);
}
/*
set pfc table address
	prio: priority 0-7
	val: resume value
*/
void mv_pp3_emac_pfc_tbl_resume(int port, int prio, u32 val)
{
	mv_pp3_emac_reg_write(port, MV_EMAC_PFC_TBL_RESUME_VAL_REG(prio), val);
}



/*--------------------------------------------------------------*/
/*------------------------ EMAC WOL ----------------------------*/
/*--------------------------------------------------------------*/

void mv_pp3_emac_wol_regs(int port)
{
	return;
}

/* TODO */
