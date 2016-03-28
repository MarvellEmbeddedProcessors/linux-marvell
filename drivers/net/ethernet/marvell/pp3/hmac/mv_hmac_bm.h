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

#ifndef __mv_hmac_bm_h__
#define __mv_hmac_bm_h__

#include "hmac/mv_hmac_regs.h"
#include "hmac/mv_hmac.h"

#define MV_PP3_BM_PE_SIZE  (16) /* bytes in 2-PE format */
#define MV_PP3_BM_PE_DG    (1)  /* PE size = 1 datagram */

#define MV_PP3_HMAC_BM_BUSY_TIMEOUT	(10000)

/* max number of buffers that can be requested by one register access */
#define MV_PP3_REQ_BUF_NUM_MAX		(8)

struct mv_pp3_hmac_bm_cfh {
	u32 buffer_addr_low;
	u8  buffer_addr_high;
	u8  reserved0;
	u8  vm_id;
	u8  bp_id;
	u32 marker_low;
	u8  marker_high;
	u8  reserved1;
	u16 reserved2;
};

/* configure queue parameters used by BM queue       */
static inline int mv_pp3_hmac_bm_queue_init(int frame, int queue, int q_size)
{
	int size;
	void *rxq_ctrl, *txq_ctrl;

	size = MV_PP3_BM_PE_DG * q_size;
	rxq_ctrl = mv_pp3_hmac_rxq_init(frame, queue, size);
	txq_ctrl = mv_pp3_hmac_txq_init(frame, queue, size, MV_PP3_BM_PE_DG);
	if ((rxq_ctrl == NULL) || (txq_ctrl == NULL))
		return -1;

	mv_pp3_hmac_queue_bm_mode_cfg(frame, queue);
	mv_pp3_hmac_rxq_enable(frame, queue);
	mv_pp3_hmac_txq_enable(frame, queue);

	return 0;
}

static inline int mv_pp3_hmac_bm_busy(int frame, int queue)
{
	int iter = 0;
	u32 reg_val = 0;

	while (iter++ < MV_PP3_HMAC_BM_BUSY_TIMEOUT) {
		reg_val = mv_pp3_hmac_frame_reg_read(frame, MV_HMAC_RX_Q_STATUS_REG(queue));
		/* check BM Allocate Busy bit */
		if (!(reg_val & MV_HMAC_RX_Q_STATUS_BM_ALLOCATE_BUSY_MASK))
			return 0;
	}
	pr_info("%s: BM busy (%d:%d) for %d times, value is 0x%x", __func__, frame, queue, iter, reg_val);
	return -1;
}

/* send to BM pool (bp_id) request for (buff_num) buffers */
static inline int mv_pp3_hmac_bm_buff_request(int frame, int queue, int bp_id, int buff_num)
{
	u32 data;
	int req_num;

	/* on one register access 8 buffers can be requested (zero based counter) */
	req_num = buff_num * MV_PP3_BM_PE_DG; /* number of buffer per datagram */
	if (req_num) {
		data = ((req_num - 1) << MV_HMAC_SEND_Q_NUM_BPID_BM_ALLOC_COUNT_OFFS) + bp_id;
		mv_pp3_hmac_frame_reg_write(frame, MV_HMAC_SEND_Q_NUM_BPID_REG(queue), data);
	}
	return 0;
}

/* process BM pool (bp_id) responce for (buff_num) buffers
 * return pool ID, physical and virtual address of buffer  */
static inline int mv_pp3_hmac_bm_buff_get(int frame, int queue, int *bp_id, u32 *ph_addr, u32 *vr_addr)
{
	struct mv_pp3_hmac_bm_cfh *bm_cfh;
	struct mv_pp3_hmac_queue_ctrl *rxq_ctrl = mv_hmac_rxq_handle[frame][queue];

	bm_cfh = (struct mv_pp3_hmac_bm_cfh *)(rxq_ctrl->next_proc);
	/* move queue current pointer to next CFH (each CFH 32 bytes) */
	if ((rxq_ctrl->next_proc + MV_PP3_BM_PE_SIZE) >= rxq_ctrl->end)
		rxq_ctrl->next_proc = rxq_ctrl->first;
	else
		rxq_ctrl->next_proc += MV_PP3_BM_PE_SIZE;

	*bp_id = bm_cfh->bp_id;
	*ph_addr = bm_cfh->buffer_addr_low;
	*vr_addr = bm_cfh->marker_low;

	return 0;
}

/* fill request for BM buffer release.
 * return ERROR, if no space for message */
static inline int mv_pp3_hmac_bm_buff_put(int frame, int queue, int bp_id, u32 ph_addr, u32 vr_addr)
{
	struct mv_pp3_hmac_bm_cfh *bm_cfh;
	struct mv_pp3_hmac_queue_ctrl *txq_ctrl = mv_hmac_txq_handle[frame][queue];

	if ((txq_ctrl->cfh_size + txq_ctrl->occ_dg) > txq_ctrl->size)
		return -1;

	/* get pointer to PE and write parameters */
	bm_cfh = (struct mv_pp3_hmac_bm_cfh *)(txq_ctrl->next_proc);
	txq_ctrl->next_proc += MV_PP3_BM_PE_SIZE;
	txq_ctrl->occ_dg += MV_PP3_BM_PE_DG;

	/* do WA, if needed */
	if (txq_ctrl->next_proc == txq_ctrl->end)
		txq_ctrl->next_proc = txq_ctrl->first;

	bm_cfh->buffer_addr_low = ph_addr;
	bm_cfh->marker_low = vr_addr;
	bm_cfh->bp_id = bp_id;

	return 0;
}

#endif /* __mv_hmac_bm_h__ */
