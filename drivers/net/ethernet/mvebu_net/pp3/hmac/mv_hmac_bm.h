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

#ifndef __mvHmacBm_h__
#define __mvHmacBm_h__

#include "hmac/mv_hmac_regs.h"

#define MV_PP3_BM_PE_SIZE  (16) /* bytes in 2-PE format */
#define MV_PP3_BM_PE_DG    (1)  /* PE size = 1 datagram */

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
static int mv_pp3_hmac_bm_queue_init(int frame, int queue, int q_size)
{
	int size;
	void *rxq_ctrl, *txq_ctrl;

	size = MV_PP3_BM_PE_DG; /* number of descriptors * 1 datagrams (per PE) */
	rxq_ctrl = mv_pp3_hmac_rxq_init(frame, queue, size);
	txq_ctrl = mv_pp3_hmac_txq_init(frame, queue, size, MV_PP3_BM_PE_DG);
	if ((rxq_ctrl == NULL) || (txq_ctrl == NULL))
		return -1;

	mv_pp3_hmac_queue_bm_mode_cfg(frame, queue);

	return 0;
}

/* send to BM pool (bp_id) request for (buff_num) buffers */
static void mv_pp3_hmac_bm_buff_request(int frame, int queue, int bp_id, int buff_num)
{
	u32 data;

	/* 2 datagrams per buffer */
	data = ((buff_num * MV_PP3_BM_PE_DG) << MV_HMAC_SEND_Q_NUM_BPID_BM_ALLOC_COUNT_OFFS) + bp_id;
	mv_pp3_hmac_frame_reg_write(frame, MV_HMAC_SEND_Q_NUM_BPID_REG(queue), data);
}

/* process BM pool (bp_id) responce for (buff_num) buffers
 * return pointer to buffer and move to next CFH           */
static struct mv_pp3_hmac_bm_cfh *mv_pp3_hmac_bm_buff_get(struct mv_pp3_hmac_queue_ctrl *rxq_ctrl)
{
	struct mv_pp3_hmac_bm_cfh *bm_cfh;

	bm_cfh = (struct mv_pp3_hmac_bm_cfh *)(rxq_ctrl->next_proc);
	/* move queue current pointer to next CFH (each CFH 32 bytes) */
	rxq_ctrl->next_proc += MV_PP3_BM_PE_SIZE;

	return bm_cfh;
}

/* fill request for BM buffer release.
 * return ERROR, if no space for message */
static int mv_pp3_hmac_bm_buff_free(int bp_id, u32 buff_addr, int frame, int queue)
{
	struct mv_pp3_hmac_bm_cfh *bm_cfh;

	/* get pointer to PE and write parameters */
	bm_cfh = (struct mv_pp3_hmac_bm_cfh *)mv_pp3_hmac_const_txq_next_cfh(frame, queue);
	if (bm_cfh == NULL)
		return 1;

	bm_cfh->buffer_addr_low = buff_addr;
	bm_cfh->bp_id = bp_id;

	return 0;
}

#endif /* __mvHmacBm_h__ */
