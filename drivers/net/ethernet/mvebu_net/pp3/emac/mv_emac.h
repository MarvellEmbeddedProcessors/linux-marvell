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

#ifndef __mvEmac_h__
#define __mvEmac_h__

#include <linux/netdevice.h>

/*--------------------------------------------------------------*/
/*--------------------- EMAC globals ---------------------------*/
/*--------------------------------------------------------------*/

/* temporary defenition */
#define MV_PP3_EMAC_MAX		4

struct mv_pp3_emac_ctrl {
	u32 base;
	u32 flags;
};

/* mv_pp3_emac_ctrl flags */


#define MV_PP3_EMAC_F_DEBUG_BIT		0
#define MV_PP3_EMAC_F_ATTACH_BIT	1

#define MV_PP3_EMAC_F_DEBUG		(1 << MV_PP3_EMAC_F_DEBUG_BIT)
#define MV_PP3_EMAC_F_ATTACH		(1 << MV_PP3_EMAC_F_ATTACH_BIT)

void mv_pp3_emac_unit_base(int index, u32 base);

u32  mv_pp3_emac_reg_read(int port, u32 reg);

void mv_pp3_emac_reg_write(int port, u32 reg, u32 data);

void mv_pp3_emac_init(int port);

void mv_pp3_emac_qm_mapping(int port, int qm_port, int qm_q);

void mv_pp3_emac_mh_en(int port, int en);

void mv_pp3_emac_ts(int port, int from);

void mv_pp3_emac_loopback(int port, int lb);

void mv_pp3_emac_regs(int port);

void mv_pp3_emac_debug(int port, int en);

void mv_pp3_emac_rx_enable(int port, int en);

void mv_pp3_emac_rx_cfh_lock_id(int port, int lock_id);

void mv_pp3_emac_rx_cfh_deq_mode(int port, int mode);

void mv_pp3_emac_rx_cfh_reorder_mode(int port, int mode);

int mv_pp3_emac_rx_desc_rsvd(int port, int bytes);

void mv_pp3_emac_rx_mh(int port, short mh);

void mv_pp3_emac_tx_min_pkt_len(int port, int bytes);

void mv_pp3_emac_fw_data(int port);
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

#endif /* __mvEmac_h__ */
