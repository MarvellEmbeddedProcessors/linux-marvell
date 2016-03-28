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

#ifndef __mv_emac_h__
#define __mv_emac_h__

#include "common/mv_sw_if.h"

/*--------------------------------------------------------------*/
/*--------------------- EMAC globals ---------------------------*/
/*--------------------------------------------------------------*/

struct mv_pp3_emac_ctrl {
	void __iomem *base;
	u32 flags;
	/* shadow of HW ROC counters */
	u32 enq_drop_cnt;
	u32 enq_xoff_cnt;
};

/* mv_pp3_emac_ctrl flags */


#define MV_PP3_EMAC_F_DEBUG_BIT		0
#define MV_PP3_EMAC_F_ATTACH_BIT	1
#define MV_PP3_EMAC_F_ROC_CNT_EN_BIT	2

#define MV_PP3_EMAC_F_DEBUG		(1 << MV_PP3_EMAC_F_DEBUG_BIT)
#define MV_PP3_EMAC_F_ATTACH		(1 << MV_PP3_EMAC_F_ATTACH_BIT)
#define MV_PP3_EMAC_F_ROC_CNT_EN	(1 << MV_PP3_EMAC_F_ROC_CNT_EN_BIT)


void mv_pp3_emac_unit_base(int index, void __iomem *base);
int mv_pp3_emac_global_init(int emacs_num);
u32  mv_pp3_emac_reg_read(int port, u32 reg);
void mv_pp3_emac_reg_write(int port, u32 reg, u32 data);
void mv_pp3_emac_init(int port, int qmp, int qmq);
void mv_pp3_emac_qm_mapping(int port, int qmp, int qmq);
void mv_pp3_emac_mh_en(int port, int en);
void mv_pp3_emac_ts(int port, int from);
void mv_pp3_emac_loopback(int port, int lb);
void mv_pp3_emac_regs(int port);
u32 mv_pp3_emac_drop_packets_cntr_get(int port);
void mv_pp3_emac_counters_show(int port);
void mv_pp3_emac_counters_clear(int port);
void mv_pp3_emac_status(int port);
void mv_pp3_emac_debug(int port, int en);
void mv_pp3_emac_rx_enable(int port, int en);
void mv_pp3_emac_rx_cfh_lock_id(int port, int lock_id);
void mv_pp3_emac_rx_cfh_deq_mode(int port, int mode);
void mv_pp3_emac_rx_cfh_reorder_mode(int port, int mode);
int mv_pp3_emac_rx_desc_rsvd(int port, int bytes);
void mv_pp3_emac_rx_mh(int port, short mh);
void mv_pp3_emac_tx_min_pkt_len(int port, int bytes);
void mv_pp3_emac_fw_data(int port);
void mv_pp3_emac_sleep_state(int port, bool en);
void mv_pp3_emac_deq_undrn_threshold(int port, int thr);

/*--------------------------------------------------------------*/
/*------------------------- PFC --------------------------------*/
/*--------------------------------------------------------------*/

#define MV_EMAC_PFC_PRIO_MAX	8

void mv_pp3_emac_pfc_tbl_addr(int port, int prio, u32 val);
void mv_pp3_emac_pfc_tbl_pause(int port, int prio, u32 val);
void mv_pp3_emac_pfc_tbl_resume(int port, int prio, u32 val);
void mv_pp3_emac_pfc_regs(int port);

/*--------------------------------------------------------------*/
/*-------------------------- WOL -------------------------------*/
/*--------------------------------------------------------------*/

void mv_pp3_emac_wol_regs(int port);
/* TODO */

/*--------------------------------------------------------------*/
/*------------------------- SYSFS ------------------------------*/
/*--------------------------------------------------------------*/

int mv_pp3_emac_sysfs_exit(struct kobject *pp3_kobj);
int mv_pp3_emac_sysfs_init(struct kobject *pp3_kobj);

#endif /* __mv_emac_h__ */
