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
#ifndef __mv_hmac_h__
#define __mv_hmac_h__

#include "platform/mv_pp3.h"
#include "platform/mv_pp3_cfh.h"
#include "platform/a390_gic_odmi_if.h"
#include "qm/mv_qm.h"
#include "hmac/mv_hmac_regs.h"

#define MV_PP3_HMAC_Q_ALIGN			(256)
#define MV_PP3_HMAC_CFH_DUMMY			(0x8000)
#define MV_PP3_HMAC_PHYS_SWQ_NUM(queue, frame)	((queue) + MV_PP3_HFRM_Q_NUM * (frame))
#define MV_PP3_HFRM_TIME_COAL			(64) /* Default time coalescing */

extern struct mv_unit_info pp3_hmac_gl;
extern struct mv_unit_info pp3_hmac_fr;
extern struct mv_pp3_hmac_queue_ctrl *mv_hmac_rxq_handle[MV_PP3_HFRM_NUM][MV_PP3_HFRM_Q_NUM];
extern struct mv_pp3_hmac_queue_ctrl *mv_hmac_txq_handle[MV_PP3_HFRM_NUM][MV_PP3_HFRM_Q_NUM];

struct mv_pp3_hmac_queue_ctrl {
	u8 *first;	/* pointer to first (virtual) byte in queue */
	u8 *next_proc;	/* pointer to next CFH to procces in queue */
	u8 *end;	/* pointer to first byte not belong to queue */
	int occ_dg;	/* number of occupated datagrams in queue */
	int dummy_dg;	/* number of dummy datagrams added by last wraparound */
	int size;	/* number of 16 bytes units (datagram) in queue */
	int cfh_size;	/* for queue with constant CFH size is number of datargarms in CFH, (or 0) */
	int capacity;	/* max number of occupated datagrams allowed for queue */
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

/* debug print flags definition */
#define MV_PP3_HMAC_READ_DEBUG_BIT	0
#define MV_PP3_HMAC_WRITE_DEBUG_BIT	1
#define MV_PP3_HMAC_TX_WA_DEBUG_BIT	2

#define MV_PP3_HMAC_READ_DEBUG		(1 << MV_PP3_HMAC_READ_DEBUG_BIT)
#define MV_PP3_HMAC_WRITE_DEBUG		(1 << MV_PP3_HMAC_WRITE_DEBUG_BIT)
#define MV_PP3_HMAC_TX_WA_DEBUG		(1 << MV_PP3_HMAC_TX_WA_DEBUG_BIT)

extern int mv_pp3_hmac_debug_flags;

/*****************************************
 *     Reigister acccess functions       *
 *****************************************/
static inline u32 mv_pp3_hmac_gl_reg_read(u32 reg)
{
	u32 reg_data;

	mv_pp3_hw_read(reg + pp3_hmac_gl.base_addr, 1, &reg_data);

	/* debug print */
	if (mv_pp3_hmac_debug_flags & MV_PP3_HMAC_READ_DEBUG)
		pr_info("read     : %8p = 0x%08x\n", reg + pp3_hmac_gl.base_addr, reg_data);

	return reg_data;
}

static inline u32 mv_pp3_hmac_frame_reg_read(int frame_id, u32 reg)
{
	void __iomem *reg_addr;
	u32 reg_data;

	if (mv_pp3_max_check(frame_id, MV_PP3_HFRM_NUM, "HMAC frame"))
		return 0;

	reg_addr = pp3_hmac_fr.base_addr + pp3_hmac_fr.ins_offs * frame_id + reg;
	reg_data = mv_pp3_hw_reg_read(reg_addr);

	/* debug print */
	if (mv_pp3_hmac_debug_flags & MV_PP3_HMAC_READ_DEBUG)
		pr_info("read     : %8p = 0x%08x\n", reg_addr, reg_data);

	return reg_data;
}

static inline void mv_pp3_hmac_gl_reg_write(u32 reg, u32 data)
{
	mv_pp3_hw_reg_write(reg + pp3_hmac_gl.base_addr, data);
	/* debug print */
	if (mv_pp3_hmac_debug_flags & MV_PP3_HMAC_WRITE_DEBUG)
		pr_info("write    : %8p = 0x%08x\n", reg + pp3_hmac_gl.base_addr, data);
}

static inline void mv_pp3_hmac_frame_reg_write(int frame_id, u32 reg, u32 data)
{
	void __iomem *reg_addr;

	if (mv_pp3_max_check(frame_id, MV_PP3_HFRM_NUM, "HMAC frame"))
		return;

	reg_addr = pp3_hmac_fr.base_addr + pp3_hmac_fr.ins_offs * frame_id + reg;
	mv_pp3_hw_reg_write(reg_addr, data);
	/* debug print */
	if (mv_pp3_hmac_debug_flags & MV_PP3_HMAC_WRITE_DEBUG)
		pr_info("write    : %8p = 0x%08x\n", reg_addr, data);
}

/*****************************************
 *        HMAC unit init functions       *
 *****************************************/
void mv_pp3_hmac_init(struct mv_pp3 *priv);
/* Init HMAC global unit base address
 * unit_offset = silicon base address + unit offset  */
void mv_pp3_hmac_gl_unit_base(void __iomem *unit_base);
/* Init HMAC Frame first unit base address
 * unit_offset = silicon base address + unit offset
 * frame_offset - is an next frame unit offset       */
void mv_pp3_hmac_frame_unit_base(void __iomem *unit_base, u32 frame_offset);

/*****************************************
 *        Frame init functions           *
 *****************************************/
void mv_pp3_hmac_frame_cfg(u32 frame_id, u8 vm_id);

/*****************************************
 *           RX queue functions          *
 *****************************************/
/* Allocate memory and init RX queue HW facility
 * size is a queue size in datagrams (16 bytes each) */
void *mv_pp3_hmac_rxq_init(int frame, int queue, int size);
void mv_pp3_hmac_rxq_delete(int frame, int queue);
void mv_pp3_hmac_rxq_flush(int frame, int queue);
void mv_pp3_hmac_rxq_enable(int frame, int queue);
void mv_pp3_hmac_rxq_disable(int frame, int queue);
void mv_pp3_hmac_rxq_event_cfg(int frame, int queue, int event, int group);
void mv_pp3_hmac_rxq_event_disable(int frame, int queue);
void mv_pp3_hmac_rxq_event_enable(int frame, int queue);
void mv_pp3_hmac_rxq_bp_node_set(int frame, int queue, enum mv_qm_node_type node_type, int node_id);
int mv_pp3_hmac_rxq_bp_thresh_set(int frame, int queue, int thresh_dg);
void mv_pp3_hmac_rxq_time_coal_profile_set(int frame, int queue, int profile);
void mv_pp3_hmac_frame_time_coal_set(int frame, int profile, int usec);
void mv_pp3_hmac_rxq_coal_get(int frame, int queue, int *profile, int *dq_num);
void mv_pp3_hmac_frame_time_coal_get(int frame, int profile, int *usec);

void mv_pp3_hmac_rxq_pause(int frame, int queue);
void mv_pp3_hmac_rxq_resume(int frame, int queue);

/* Return number of received datagrams */
static inline int mv_pp3_hmac_rxq_occ_get(int frame, int queue)
{
	return mv_pp3_hmac_frame_reg_read(frame, MV_PP3_HMAC_RQ_OCC_STATUS(queue)) & MV_PP3_HMAC_OCC_COUNTER_MASK;
}

/* Write a number of processed datagram (16 bytes each) */
static inline void mv_pp3_hmac_rxq_occ_set(int frame, int queue, int size)
{
	mv_pp3_hmac_frame_reg_write(frame, MV_PP3_HMAC_RQ_OCC_STATUS(queue), size);
}

/* Returns pointer to next recieved CFH buffer and it */
/* size - number of datagram                          */
static inline u8 *mv_pp3_hmac_rxq_next_cfh(int frame, int queue, int *size)
{
	struct cfh_common_format *cfh;
	struct mv_pp3_hmac_queue_ctrl *qctrl = mv_hmac_rxq_handle[frame][queue];
	unsigned int cfh_size; /* real CFH size aligned to 16 bytes in bytes */

	/* Read 16 bytes of CFH pointed by "next_proc" field and calculate size of CFH in bytes */
	cfh = (struct cfh_common_format *)qctrl->next_proc;
	cfh_size = MV_ALIGN_UP(cfh->cfh_length, MV_PP3_CFH_DG_SIZE);

	if (cfh_size == 0) {
		pr_err("%s: error CFH 0x%p with wrong size %d (%d) on frame %d, queue %d\n",
			__func__, cfh, cfh_size, cfh->cfh_length, frame, queue);
		*size = 0;
		return NULL;
	}

	/* Move "next_proc" pointer to next CFH ("next_proc" + size) */
	qctrl->next_proc += cfh_size;
	*size = cfh_size / MV_PP3_CFH_DG_SIZE;
	/* check if there is an end of queue */
	if (qctrl->next_proc >= qctrl->end) {
		if (qctrl->next_proc == qctrl->end)
			qctrl->next_proc = qctrl->first;
		else {
			memcpy(qctrl->end, qctrl->first, qctrl->next_proc - qctrl->end);
			qctrl->next_proc = qctrl->first + (qctrl->next_proc - qctrl->end);
		}
	}

	/* if get empty CFH with "W" bit set, return NULL */
	if (cfh->qm_cntrl & MV_PP3_HMAC_CFH_DUMMY) {
		return NULL;
	}

	/* return real CFH pointer */
	return (u8 *)cfh;
}

/* set next CFH pointer = current - size * 16 */
static inline u8 *mv_pp3_hmac_rxq_cfh_free(int frame, int queue, int size)
{
	struct mv_pp3_hmac_queue_ctrl *qctrl = mv_hmac_rxq_handle[frame][queue];
	int dg = (qctrl->next_proc - qctrl->first) / MV_PP3_CFH_DG_SIZE;

	if (dg < size) {
		size -= dg;
		qctrl->next_proc = qctrl->end;
	}

	qctrl->next_proc -= (size * MV_PP3_CFH_DG_SIZE);

	return qctrl->next_proc;
}

/* configure rxq packets coalesing profile */
static inline void mv_pp3_hmac_rxq_pkt_coal_set(int frame, int queue, int dg_num)
{
	mv_pp3_hmac_frame_reg_write(frame, MV_PP3_HMAC_RQ_INT_THRESH(queue), dg_num);
}

/*****************************************
 *           TX queue functions          *
 *****************************************/
/* Allocate memory and init TX queue HW facility:
 * size is a queue size in datagrams (16 bytes each) */
void *mv_pp3_hmac_txq_init(int frame, int queue, int size, int cfh_size);
void mv_pp3_hmac_txq_delete(int frame, int queue);
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

	if ((qctrl->capacity - qctrl->occ_dg) >= dg_num)
		return 0;

	qctrl->occ_dg = mv_pp3_hmac_frame_reg_read(frame, MV_PP3_HMAC_SQ_OCC_STATUS(queue)) &
					MV_PP3_HMAC_OCC_COUNTER_MASK;
	return ((qctrl->capacity - qctrl->occ_dg) >= dg_num) ? 0 : 1;
}

/* Return number of free space in the end of queue (in datagrams) */
static inline int mv_pp3_hmac_txq_free_room(struct mv_pp3_hmac_queue_ctrl *qctrl)
{
	return (qctrl->end - qctrl->next_proc) / MV_PP3_CFH_DG_SIZE;
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

	/* check queue capacity */
	if ((qctrl->occ_dg + qctrl->cfh_size) > qctrl->capacity) {
		/* update from HW number of occupited DG */
		qctrl->occ_dg = mv_pp3_hmac_frame_reg_read(frame, MV_PP3_HMAC_SQ_OCC_STATUS(queue)) &
						MV_PP3_HMAC_OCC_COUNTER_MASK;
		if ((qctrl->occ_dg + qctrl->cfh_size) > qctrl->capacity)
			return NULL;
	}

	cfh_ptr = qctrl->next_proc;
	qctrl->next_proc += (qctrl->cfh_size * MV_PP3_CFH_DG_SIZE);
	qctrl->occ_dg += qctrl->cfh_size;

	if (qctrl->next_proc == qctrl->end)
		qctrl->next_proc = qctrl->first;

	return cfh_ptr;
}

static void mv_pp3_dummy_message_print(char *str, char *cfh_ptr, int size)
{
	u32  *tmp;
	int i;

	pr_info("\n%s on cpu %d with data length %d 0x%p:", str, smp_processor_id(), size, cfh_ptr);

	tmp = (u32 *)cfh_ptr;
	pr_info("Message header: ");
	for (i = 0; i < 4; i++)
		pr_cont("%08x ", tmp[i]);
	pr_info("\n");
}

/* Return last allocated unused CFHs.
 * size is CFH size in datagrams (16 bytes each)     */
static inline u8 *mv_pp3_hmac_txq_cfh_free(int frame, int queue, int size)
{
	struct mv_pp3_hmac_queue_ctrl *qctrl = mv_hmac_txq_handle[frame][queue];

	/* check queue occupation */
	if (qctrl->occ_dg < size)
		return NULL;

	qctrl->occ_dg -= size;

	if (qctrl->next_proc == qctrl->first) {
		qctrl->next_proc = qctrl->end;
		if (qctrl->dummy_dg) {
			qctrl->next_proc -= (qctrl->dummy_dg * MV_PP3_CFH_DG_SIZE);
			qctrl->occ_dg -= qctrl->dummy_dg;
			qctrl->dummy_dg = 0;
		}
	}

	qctrl->next_proc -= (size * MV_PP3_CFH_DG_SIZE);

	return qctrl->next_proc;
}

/* Return pointer to first free one CFH (run queue wraparound, if needed) :
 * size is CFH size in datagrams (16 bytes each)     */
static inline u8 *mv_pp3_hmac_txq_next_cfh(int frame, int queue, int size)
{
	u8 *cfh_ptr;
	int end_free_dg;	/* number of free datagram in the queue end */
	struct mv_pp3_hmac_queue_ctrl *qctrl = mv_hmac_txq_handle[frame][queue];

	/* check queue capacity */
	if ((qctrl->occ_dg + size) > qctrl->capacity) {
		/* update from HW number of occupited DG */
		qctrl->occ_dg = mv_pp3_hmac_frame_reg_read(frame, MV_PP3_HMAC_SQ_OCC_STATUS(queue)) &
						MV_PP3_HMAC_OCC_COUNTER_MASK;

		if ((qctrl->occ_dg + size) > qctrl->capacity)
			return NULL;
	}

	/* calculate number of unused datagram in the queue end */
	end_free_dg = (qctrl->end - qctrl->next_proc) / MV_PP3_CFH_DG_SIZE;
	if (end_free_dg >= size) {
		cfh_ptr = qctrl->next_proc;
		qctrl->next_proc += (size * MV_PP3_CFH_DG_SIZE);
		qctrl->occ_dg += size;
		return cfh_ptr;
	}

	/* There is not enough space in the queue end. */
	/* return pointer to fisrt CFH and move next pointer to second CFH in queue */
	if (end_free_dg > 0) {
		/* do wraparound with dummy CFH sent */
		struct cfh_common_format *cfh = (struct cfh_common_format *)qctrl->next_proc;
		u32 dummy_dg;

		/* check queue capacity include dummy WA CFH */
		if ((qctrl->occ_dg + size + end_free_dg) > qctrl->capacity) {
			/* update from HW number of occupited DG */
			qctrl->occ_dg = mv_pp3_hmac_frame_reg_read(frame, MV_PP3_HMAC_SQ_OCC_STATUS(queue)) &
						MV_PP3_HMAC_OCC_COUNTER_MASK;

			if ((qctrl->occ_dg + size + end_free_dg) > qctrl->capacity)
				return NULL;
		}

		memset(cfh, 0, (end_free_dg * MV_PP3_CFH_DG_SIZE));

		qctrl->dummy_dg = end_free_dg;
		qctrl->occ_dg += end_free_dg;
		do {
			dummy_dg = MV_MIN(end_free_dg, MV_PP3_CFH_DG_MAX_NUM);

			cfh->cfh_length = dummy_dg * MV_PP3_CFH_DG_SIZE;
			cfh->qm_cntrl = MV_PP3_HMAC_CFH_DUMMY; /* set bit 'W' */

			if (mv_pp3_hmac_debug_flags & MV_PP3_HMAC_TX_WA_DEBUG)
				mv_pp3_dummy_message_print("Sent Dummy", qctrl->next_proc, cfh->cfh_length);

			qctrl->next_proc += cfh->cfh_length;
			cfh = (struct cfh_common_format *)qctrl->next_proc;
			end_free_dg -= dummy_dg;

		} while (end_free_dg);
	}
	qctrl->next_proc = qctrl->first + (size * MV_PP3_CFH_DG_SIZE);
	qctrl->occ_dg += size;

	return qctrl->first;
}
static inline u32 mv_pp3_hmac_txq_dummy_dg_get(int frame, int queue)
{
	struct mv_pp3_hmac_queue_ctrl *qctrl = mv_hmac_txq_handle[frame][queue];
	u32 size = qctrl->dummy_dg;

	qctrl->dummy_dg = 0;
	return size;
}
/* size - is number of datagrams to transmit         */
static inline void mv_pp3_hmac_txq_send(int frame, int queue, int size)
{
	struct mv_pp3_hmac_queue_ctrl *qctrl = mv_hmac_txq_handle[frame][queue];

	size += qctrl->dummy_dg;

	mv_pp3_hmac_frame_reg_write(frame, MV_PP3_HMAC_SQ_OCC_STATUS(queue), size);

	qctrl->dummy_dg = 0;
}

/* Update sw counter of TX queue occupited datagrams */
/* Must be called through TX Done processing */
static inline void mv_pp3_hmac_txq_occ_upd(int frame, int queue, int dg_num)
{
	struct mv_pp3_hmac_queue_ctrl *qctrl = mv_hmac_txq_handle[frame][queue];

	(dg_num < qctrl->occ_dg) ? qctrl->occ_dg -= dg_num : 0;
}

/* Configure queue capacity */
static inline int mv_pp3_hmac_txq_capacity_cfg(int frame, int queue, int cap)
{
	struct mv_pp3_hmac_queue_ctrl *qctrl;

	if (mv_pp3_max_check(frame, MV_PP3_HFRM_NUM, "HMAC frame"))
		return -1;
	if (mv_pp3_max_check(queue, MV_PP3_HFRM_Q_NUM, "HMAC queue"))
		return -1;

	qctrl = mv_hmac_txq_handle[frame][queue];

	if ((cap > qctrl->size) || (cap < MV_PP3_CFH_DG_MAX_NUM)) {
		pr_err("%s: HMAC TXQ size #%d [dg] is out of range: [%d..%d]\n", __func__,
			cap, MV_PP3_CFH_DG_MAX_NUM, qctrl->size);
		return -1;
	}

	qctrl->capacity = cap;
	return 0;
}

/* Unmask all events on group in frame */
static inline void mv_pp3_hmac_group_event_unmask(int frame, int event_group)
{
	u32 reg_val;

	reg_val = (1 << event_group);
	mv_pp3_hmac_gl_reg_write(MV_HMAC_EVENT_MASK_REG(frame), reg_val);
}

/* Mask all events on group in frame */
static inline void mv_pp3_hmac_group_event_mask(int frame, int event_group)
{
	u32 reg_val;

	reg_val = (1 << (event_group + MV_HMAC_EVENT_MASK_GROUP_DIS_MASK_OFFS));
	mv_pp3_hmac_gl_reg_write(MV_HMAC_EVENT_MASK_REG(frame), reg_val);
}

/* configure queue parameters used by BM queue       */
void mv_pp3_hmac_queue_bm_mode_cfg(int frame, int queue);
/* configure queue parameters used by QM queue
 * q_num - is a number of QM queue                   */
void mv_pp3_hmac_queue_qm_mode_cfg(int frame, int queue, int q_num);

/* dump hmac queue registers */
void mv_pp3_hmac_rxq_regs_dump(int frame, int queue);
void mv_pp3_hmac_txq_regs_dump(int frame, int queue);
void mv_pp3_hmac_frame_regs_dump(int frame);
void mv_pp3_hmac_global_regs_dump(void);
/* queue show functions */
void mv_pp3_hmac_rx_queue_show(int frame, int queue);
void mv_pp3_hmac_tx_queue_show(int frame, int queue);
/* queue dump functions */
void mv_pp3_hmac_rx_queue_dump(int frame, int queue, bool mode);
void mv_pp3_hmac_tx_queue_dump(int frame, int queue, bool mode);
/* debug functions */
void mv_pp3_hmac_debug_cfg(int flags);
/* sysfs functions */
int mv_pp3_hmac_sysfs_init(struct kobject *kobj);
int mv_pp3_hmac_sysfs_exit(struct kobject *kobj);

#endif /* __mv_hmac_h__ */
