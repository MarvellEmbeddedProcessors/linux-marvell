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

#ifndef __mvHmac_h__
#define __mvHmac_h__

#include "hmac/mv_hmac_regs.h"

#define MV_PP3_HMAC_MAX_FRAME			(16)
#define MV_PP3_QUEUES_PER_FRAME			(16)

#define MV_PP3_HMAC_DG_SIZE				(16)	/* bytes */
#define MV_PP3_CFH_MIN_SIZE				(32)
#define MV_PP3_CFH_MAX_SIZE				(128)
#define MV_PP3_CFH_DG_NUM				(MV_PP3_CFH_MIN_SIZE / MV_PP3_HMAC_DG_SIZE)
#define MV_PP3_HMAC_Q_ALIGN				(256)

#define MV_PP3_HMAC_CFH_DUMMY			(0x8000)

extern struct pp3_unit_info pp3_hmac_gl;
extern struct pp3_unit_info pp3_hmac_fr;
extern struct mv_pp3_hmac_queue_ctrl *mv_hmac_rxq_handle[MV_PP3_HMAC_MAX_FRAME][MV_PP3_QUEUES_PER_FRAME];
extern struct mv_pp3_hmac_queue_ctrl *mv_hmac_txq_handle[MV_PP3_HMAC_MAX_FRAME][MV_PP3_QUEUES_PER_FRAME];

struct mv_pp3_hmac_queue_ctrl {
	u8 *first;		/* pointer to first byte in queue */
	u8 *next_proc;	/* pointer to next CFH to procces in queue */
	u8 *end;		/* pointer to first byte not belong to queue */
	int occ_dg;		/* number of occupated datagram in queue */
	int dummy_dg;	/* number of dummy datagrams added by last wraparound */
	int size;		/* number of 16 bytes units (datagram) in queue */
	int cfh_size;	/* for queue with constant CFH size is number of datargarms in CFH, (or 0) */
};

/* CFH structure */
struct cfh_common_format {

	unsigned short pkt_length;
	unsigned short seq_id_qe_cntrl;
	unsigned short qm_cntrl;
	unsigned char  cfh_length;	/* bytes */
	unsigned char  cfh_format;
	unsigned int   tag1;
	unsigned int   tag2;
	unsigned int   addr_low;
	unsigned short addr_high;
	unsigned char  vm_id;
	unsigned char  bp_id;
	unsigned int   marker_low;
	unsigned char  marker_high;
	unsigned char  reserved0;
	unsigned short qc_cntrl;
};

/*****************************************
 *     Reigister acccess functions       *
 *****************************************/
static inline u32 mv_pp3_hmac_gl_reg_read(u32 reg)
{
	u32 reg_data;

	mv_pp3_hw_read(reg + pp3_hmac_gl.base_addr, 1, &reg_data);
	/* add debug print */

	return reg_data;
}

static inline u32 mv_pp3_hmac_frame_reg_read(int frameId, u32 reg)
{
	u32 reg_addr;

	reg_addr = pp3_hmac_fr.base_addr + pp3_hmac_fr.ins_offs * frameId + reg;
	/* add debug print */

	return mv_pp3_hw_reg_read(reg_addr);
}

static inline void mv_pp3_hmac_gl_reg_write(u32 reg, u32 data)
{
	mv_pp3_hw_reg_write(reg + pp3_hmac_gl.base_addr, data);
}

static inline void mv_pp3_hmac_frame_reg_write(int frame_id, u32 reg, u32 data)
{
	u32 reg_addr = pp3_hmac_fr.base_addr + pp3_hmac_fr.ins_offs * frame_id + reg;

	mv_pp3_hw_reg_write(reg_addr, data);
}

/*****************************************
 *        HMAC unit init functions       *
 *****************************************/
/* Init HMAC global unit base address
 * unit_offset = silicon base address + unit offset  */
void mv_pp3_hmac_gl_unit_base(u32 unit_offset);
/* Init HMAC Frame first unit base address
 * unit_offset = silicon base address + unit offset
 * frame_offset - is an next frame unit offset       */
void mv_pp3_hmac_frame_unit_base(u32 unit_offset, u32 frame_offset);

/*****************************************
 *           RX queue functions          *
 *****************************************/
/* Allocate memory and init RX queue HW facility
 * size is a queue size in datagrams (16 bytes each) */
void *mv_pp3_hmac_rxq_init(int frame, int queue, int size);
void mv_pp3_hmac_rxq_flush(int frame, int queue);
void mv_pp3_hmac_rxq_enable(int frame, int queue);
void mv_pp3_hmac_rxq_disable(int frame, int queue);
void mv_pp3_hmac_rxq_event_cfg(int frame, int queue, int event, int group);

/* Return number of received datagrams */
static inline int mv_pp3_hmac_rxq_occ_get(int frame, int queue)
{
	return mv_pp3_hmac_frame_reg_read(frame, MV_PP3_HMAC_RQ_OCC_STATUS(queue)) & MV_PP3_HMAC_OCC_COUNTER_MASK;
}

/* Write a number of processed datagram (16 bytes each) */
static inline void mv_pp3_hmac_rxq_occ_set(int frame, int queue, int size)
{
	mv_pp3_hmac_frame_reg_write(frame, MV_HMAC_REC_Q_OCC_STATUS_UPDATE_REG(queue), size);
}

/* Returns pointer to next CFH buffer and it size */
/* size - number of datagram                      */
static inline u8 *mv_pp3_hmac_rxq_next_cfh(int frame, int queue, int *size)
{
	struct cfh_common_format *cfh;
	struct mv_pp3_hmac_queue_ctrl *qctrl = mv_hmac_rxq_handle[frame][queue];

	/* Read 16 bytes of CFH pointed by "next_proc" field and calculate size of CFH in bytes */
	cfh = (struct cfh_common_format *)qctrl->next_proc;

	/* if get NULL CFH with "W" bit set, do wraparound */
	if (cfh->qm_cntrl & MV_PP3_HMAC_CFH_DUMMY) {
		qctrl->next_proc = qctrl->first;
		/* return first CFH in queue */
		cfh = (struct cfh_common_format *)qctrl->next_proc;
		*size = cfh->cfh_length / MV_PP3_HMAC_DG_SIZE;
		return NULL;
	}

	/* Move "next_proc" pointer to next CFH ("next_proc" + size) */
	qctrl->next_proc += cfh->cfh_length;

	*size = cfh->cfh_length / MV_PP3_HMAC_DG_SIZE;
	return (u8 *)cfh;
}

/*****************************************
 *           TX queue functions          *
 *****************************************/
/* Allocate memory and init TX queue HW facility:
 * size is a queue size in datagrams (16 bytes each) */
void *mv_pp3_hmac_txq_init(int frame, int queue, int size, int cfh_size);
void mv_pp3_hmac_txq_flush(int frame, int queue);
void mv_pp3_hmac_txq_enable(int frame, int queue);
void mv_pp3_hmac_txq_disable(int frame, int queue);
void mv_pp3_hmac_txq_event_cfg(int frame, int queue, int group);

/* Check for space in the queue.
 * Return 0 for positive answer, or 1 for negative.
 * dg_num - number of datagrams we are looking for */
static inline int mv_pp3_hmac_txq_check_for_space(int frame, int queue, int dg_num)
{
	struct mv_pp3_hmac_queue_ctrl *qctrl = mv_hmac_txq_handle[frame][queue];

	if ((qctrl->size - qctrl->occ_dg) >= dg_num)
		return 0;

	qctrl->occ_dg = mv_pp3_hmac_frame_reg_read(frame, MV_PP3_HMAC_SQ_OCC_STATUS(queue)) &
					MV_PP3_HMAC_OCC_COUNTER_MASK;
	return ((qctrl->size - qctrl->occ_dg) >= dg_num) ? 0 : 1;
}

/* Return number of free space in the end of queue (in datagrams) */
static inline int mv_pp3_hmac_txq_free_room(struct mv_pp3_hmac_queue_ctrl *qctrl)
{
	return (qctrl->end - qctrl->next_proc) / MV_PP3_HMAC_DG_SIZE;
}

/* Return number of currently occupated datagrams in queue */
static inline int mv_pp3_hmac_txq_occ_get(int frame, int queue)
{
	struct mv_pp3_hmac_queue_ctrl *qctrl = mv_hmac_txq_handle[frame][queue];

	qctrl->occ_dg = mv_pp3_hmac_frame_reg_read(frame, MV_PP3_HMAC_SQ_OCC_STATUS(queue)) &
					MV_PP3_HMAC_OCC_COUNTER_MASK;
	return qctrl->occ_dg;
}

/* Return pointer to first free one CFH from queue with constant CFH size
 * (do queue wraparound, if needed) */
static inline u8 *mv_pp3_hmac_const_txq_next_cfh(int frame, int queue)
{
	u8 *cfh_ptr;
	struct mv_pp3_hmac_queue_ctrl *qctrl = mv_hmac_txq_handle[frame][queue];

	if ((qctrl->cfh_size + qctrl->occ_dg) > qctrl->size)
		return NULL;

	cfh_ptr = qctrl->next_proc;
	qctrl->next_proc += (qctrl->cfh_size * MV_PP3_HMAC_DG_SIZE);
	qctrl->occ_dg += qctrl->cfh_size;

	if (qctrl->next_proc == qctrl->end)
		qctrl->next_proc = qctrl->first;

	return cfh_ptr;
}

/* Return pointer to first free one CFH (run queue wraparound, if needed) :
 * size is CFH size in datagrams (16 bytes each)     */
static inline u8 *mv_pp3_hmac_txq_next_cfh(int frame, int queue, int size)
{
	u8 *cfh_ptr;
	int end_free_dg;	/* number of free datagram in the queue end */
	int start_free_dg;	/* number of free datagram in the queue start */
	struct mv_pp3_hmac_queue_ctrl *qctrl = mv_hmac_txq_handle[frame][queue];

	/* calculate number of unused datagram in the queue end */
	end_free_dg = (qctrl->end - qctrl->next_proc) / MV_PP3_HMAC_DG_SIZE;
	if ((end_free_dg >= size) && (end_free_dg < (MV_PP3_CFH_MAX_SIZE / MV_PP3_HMAC_DG_SIZE))) {
		cfh_ptr = qctrl->next_proc;
		qctrl->next_proc += (size * MV_PP3_HMAC_DG_SIZE);
		qctrl->occ_dg += size;

		return cfh_ptr;
	}

	/* There is not enough space in the queue end. */
	/* Check and if possible to queue wraparound */
	/* calculate number of unused datagrams in the queue start */
	start_free_dg = (qctrl->size - qctrl->occ_dg) - end_free_dg;
	if (start_free_dg < size)
		/* not enough space in queue (cannot run wraparound) */
		return NULL;

	/* do FIFO wraparound */
	/* return pointer to fisrt CFH and move next pointer to second CFH in queue */
	if (qctrl->end != qctrl->next_proc) {
		/* do wraparound with dummy CFH sent */
		struct cfh_common_format *cfh = (struct cfh_common_format *)qctrl->next_proc;

		cfh->pkt_length = end_free_dg * MV_PP3_HMAC_DG_SIZE;
		cfh->qm_cntrl = MV_PP3_HMAC_CFH_DUMMY; /* set bit 'W' */
		qctrl->dummy_dg = end_free_dg;
	}
	qctrl->next_proc = qctrl->first + (size * MV_PP3_HMAC_DG_SIZE);
	qctrl->occ_dg += size;

	return qctrl->first;
}

/* size - is number of datagrams to transmit         */
static inline void mv_pp3_hmac_txq_send(int frame, int queue, int size)
{
	struct mv_pp3_hmac_queue_ctrl *qctrl = mv_hmac_txq_handle[frame][queue];

	size += qctrl->dummy_dg;
	mv_pp3_hmac_frame_reg_write(frame, MV_HMAC_SEND_Q_OCC_STATUS_UPDATE_REG(queue), size);
	qctrl->dummy_dg = 0;
}

/* configure queue parameters used by BM queue       */
void mv_pp3_hmac_queue_bm_mode_cfg(int frame, int queue);
/* configure queue parameters used by QM queue
 * q_num - is a number of QM queue                   */
void mv_pp3_hmac_queue_qm_mode_cfg(int frame, int queue, int q_num);


#endif /* __mvHmac_h__ */
