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
#include "hmac/mv_hmac.h"

#ifdef PP3_DEBUG
#define PP3_HMAC_DEBUG pr_info("\n%s::", __func__)
#else
#define PP3_HMAC_DEBUG
#endif

/* bitmap to store queues state (allocated/free) per frame */
static u32 mv_pp3_hmac_queue_act[MV_PP3_HMAC_MAX_FRAME] = {0};
/* */
struct pp3_unit_info pp3_hmac_gl;
struct pp3_unit_info pp3_hmac_fr;

/* Array of pointers to HMAC queue control structure */
struct mv_pp3_hmac_queue_ctrl *mv_hmac_rxq_handle[MV_PP3_HMAC_MAX_FRAME][MV_PP3_QUEUES_PER_FRAME];
struct mv_pp3_hmac_queue_ctrl *mv_hmac_txq_handle[MV_PP3_HMAC_MAX_FRAME][MV_PP3_QUEUES_PER_FRAME];


/* local functions declaration */
static int mv_pp3_hmac_queue_create(struct mv_pp3_hmac_queue_ctrl *q_ctrl);

/* general functions */
/* HMAC unit init */
void mv_pp3_hmac_init(void)
{
	int base = mv_hw_silicon_base_addr_get();

	mv_pp3_hmac_gl_unit_base(base + MV_PP3_HMAC_GL_UNIT_OFFSET);
	mv_pp3_hmac_frame_unit_base(base + MV_PP3_HMAC_FR_UNIT_OFFSET, MV_PP3_HMAC_FR_INST_OFFSET);
}

/* store unit base address = silicon base address + unit offset */
void mv_pp3_hmac_gl_unit_base(unsigned int unit_offset)
{
	pp3_hmac_gl.base_addr = unit_offset;
	pp3_hmac_gl.ins_offs = 0;
}

/* store unit base address = silicon base address + unit offset */
/* store unit instance offset                                   */
void mv_pp3_hmac_frame_unit_base(unsigned int unit_offset, unsigned int ins_offset)
{
	pp3_hmac_fr.base_addr = unit_offset;
	pp3_hmac_fr.ins_offs = ins_offset;
}

/* configure queue to be used like BM queue */
void mv_pp3_hmac_queue_bm_mode_cfg(int frame, int queue)
{
	u32 reg_data;

	PP3_HMAC_DEBUG;
	reg_data = mv_pp3_hmac_frame_reg_read(frame, MV_HMAC_SEND_Q_CTRL_REG(queue));
	U32_SET_FIELD(reg_data, MV_HMAC_SEND_Q_CTRL_Q_MODE_MASK, 1 << MV_HMAC_SEND_Q_CTRL_Q_MODE_OFFS);
	U32_SET_FIELD(reg_data, MV_HMAC_SEND_Q_CTRL_BM_PE_FORMAT_MASK, 1 << MV_HMAC_SEND_Q_CTRL_BM_PE_FORMAT_OFFS);

	mv_pp3_hmac_frame_reg_write(frame, MV_HMAC_SEND_Q_CTRL_REG(queue), reg_data);
}

/* configure queue parameters used by QM queue
 * qm_num - is a number of QM queue                   */
void mv_pp3_hmac_queue_qm_mode_cfg(int frame, int queue, int qm_num)
{
	u32 reg_data;

	PP3_HMAC_DEBUG;
	/* configure queue to be QM queue */
	reg_data = mv_pp3_hmac_frame_reg_read(frame, MV_HMAC_SEND_Q_CTRL_REG(queue));
	U32_SET_FIELD(reg_data, MV_HMAC_SEND_Q_CTRL_Q_MODE_MASK, 0);
	mv_pp3_hmac_frame_reg_write(frame, MV_HMAC_SEND_Q_CTRL_REG(queue), reg_data);
	/* map QM queue number */
	mv_pp3_hmac_frame_reg_write(frame, MV_HMAC_SEND_Q_NUM_BPID_REG(queue), qm_num);
}


/************************ RX queue functions **************************************************/
/* Allocate memory and init RX queue HW facility
 * size is a queue size in datagrams (16 bytes each)
 * Returns - pointer to HMAC RX queue structure */
void *mv_pp3_hmac_rxq_init(int frame, int queue, int size)
{
	struct mv_pp3_hmac_queue_ctrl *qctrl;
	u32 reg_data;

	PP3_HMAC_DEBUG;
	/* check if already created */
	if ((mv_pp3_hmac_queue_act[frame] >> queue) & 1)
		return NULL;

	/* allocate hmac queue control stucture */
	qctrl = kmalloc(sizeof(struct mv_pp3_hmac_queue_ctrl), GFP_KERNEL);
	if (qctrl == NULL)
		return NULL;

	qctrl->size = size;
	mv_pp3_hmac_queue_create(qctrl);
	/* Write pointer to allocated memory */
	mv_pp3_hmac_frame_reg_write(frame, MV_PP3_HMAC_RQ_ADDR_LOW(queue), (u32)qctrl->first);
	/* Store queue size in rq_size table, number of 16B units */
	mv_pp3_hmac_frame_reg_write(frame, MV_PP3_HMAC_RQ_SIZE(queue), (u32)qctrl->size);
	/* Configure Receive Threshold TBD */
	/* Enable queue, hmac_%m_rec_q_%n_control set to 1 */
	reg_data = mv_pp3_hmac_frame_reg_read(frame, MV_HMAC_REC_Q_CTRL_REG(queue));
	U32_SET_FIELD(reg_data, MV_HMAC_REC_Q_CTRL_RCV_Q_EN_MASK, 1 << MV_HMAC_REC_Q_CTRL_RCV_Q_EN_OFFS);
	mv_pp3_hmac_frame_reg_write(frame, MV_HMAC_REC_Q_CTRL_REG(queue), reg_data);

	/* mark queue as created */
	mv_hmac_rxq_handle[frame][queue] = qctrl;
	mv_pp3_hmac_queue_act[frame] |= (1 << queue);

	return qctrl;
}

void mv_pp3_hmac_rxq_flush(int frame, int queue)
{
	u32 data;

	/* flush queue */
	data = mv_pp3_hmac_frame_reg_read(frame, MV_HMAC_REC_Q_CTRL_REG(queue));
	data &= ~(MV_HMAC_REC_Q_CTRL_RCV_Q_FLUSH_MASK);
	mv_pp3_hmac_frame_reg_write(frame, MV_HMAC_REC_Q_CTRL_REG(queue), data);
}

void mv_pp3_hmac_rxq_enable(int frame, int queue)
{
	u32 reg_data;

	/* Enable queue */
	reg_data = mv_pp3_hmac_frame_reg_read(frame, MV_HMAC_REC_Q_CTRL_REG(queue));
	U32_SET_FIELD(reg_data, MV_HMAC_REC_Q_CTRL_RCV_Q_EN_MASK, 1 << MV_HMAC_REC_Q_CTRL_RCV_Q_EN_OFFS);
	mv_pp3_hmac_frame_reg_write(frame, MV_HMAC_REC_Q_CTRL_REG(queue), reg_data);
}

void mv_pp3_hmac_rxq_disable(int frame, int queue)
{
	u32 reg_data;

	/* Disable queue */
	reg_data = mv_pp3_hmac_frame_reg_read(frame, MV_HMAC_REC_Q_CTRL_REG(queue));
	U32_SET_FIELD(reg_data, MV_HMAC_REC_Q_CTRL_RCV_Q_EN_MASK, 0);
	mv_pp3_hmac_frame_reg_write(frame, MV_HMAC_REC_Q_CTRL_REG(queue), reg_data);
}

/* Connect one of queue RX events to SPI interrupt group
Inputs:
	event - HMAC Rx event
	 * 0 - QM queue - Timeout or new items added to the queue
	 *     BM queue - allocate completed
	 * 1 - QM queue only - Receive queue almost full
	group - SPI group for event (0 - 7)
*/
void mv_pp3_hmac_rxq_event_cfg(int frame, int queue, int event, int group)
{
	u32 reg_data;

	PP3_HMAC_DEBUG;
	/* Configure event group */
	reg_data = mv_pp3_hmac_frame_reg_read(frame, MV_PP3_HMAC_RQ_EVENT_GROUP(queue));
	if (event == 0) {
		/* set group for event 0 */
		U32_SET_FIELD(reg_data, MV_PP3_HMAC_RQ_EVENT0_GROUP_MASK, group << MV_PP3_HMAC_RQ_EVENT0_GROUP_OFFS);
		/* enable event */
		U32_SET_FIELD(reg_data, MV_PP3_HMAC_RQ_EVENT0_EN_MASK, 1 << MV_PP3_HMAC_RQ_EVENT0_EN_OFFS);
	} else if (event == 1) {
		U32_SET_FIELD(reg_data, MV_PP3_HMAC_RQ_EVENT1_GROUP_MASK, group << MV_PP3_HMAC_RQ_EVENT1_GROUP_OFFS);
		/* enable event */
		U32_SET_FIELD(reg_data, MV_PP3_HMAC_RQ_EVENT1_EN_MASK, 1 << MV_PP3_HMAC_RQ_EVENT1_EN_OFFS);
	}
	mv_pp3_hmac_frame_reg_write(frame, MV_PP3_HMAC_RQ_EVENT_GROUP(queue), reg_data);
}


/************************ TX queue functions **************************************************/
/* Allocate memory and init TX queue HW facility
 * size - queue size in datagrams (16 bytes each)
 * cfh_size - if not 0, define queue with constant CFH size (number of datagrams in CFH)
 * Returns - pointer to HMAC TX queue structure */
void *mv_pp3_hmac_txq_init(int frame, int queue, int size, int cfh_size)
{
	struct mv_pp3_hmac_queue_ctrl *qctrl;
	u32 reg_data;

	PP3_HMAC_DEBUG;
	/* allocate hmac queue control stucture */
	qctrl = kmalloc(sizeof(struct mv_pp3_hmac_queue_ctrl), GFP_KERNEL);
	if (qctrl == NULL)
		return NULL;

	qctrl->size = size;
	mv_pp3_hmac_queue_create(qctrl);
	qctrl->cfh_size = cfh_size;

	/* Write pointer to allocated memory */
	mv_pp3_hmac_frame_reg_write(frame, MV_PP3_HMAC_SQ_ADDR_LOW(queue), (u32)qctrl->first);
	/* Store queue size in rq_size table, number of 16B units */
	mv_pp3_hmac_frame_reg_write(frame, MV_PP3_HMAC_SQ_SIZE(queue), (u32)qctrl->size);

	/* Configure Transmit Threshold TBD */
	/* Enable queue */
	reg_data = mv_pp3_hmac_frame_reg_read(frame, MV_HMAC_SEND_Q_CTRL_REG(queue));
	U32_SET_FIELD(reg_data, MV_HMAC_SEND_Q_CTRL_SEND_Q_EN_MASK, 1 << MV_HMAC_SEND_Q_CTRL_SEND_Q_EN_OFFS);
	mv_pp3_hmac_frame_reg_write(frame, MV_HMAC_SEND_Q_CTRL_REG(queue), reg_data);

	mv_hmac_txq_handle[frame][queue] = qctrl;

	return qctrl;
}

void mv_pp3_hmac_txq_enable(int frame, int queue)
{
	u32 reg_data;

	/* Enable queue */
	reg_data = mv_pp3_hmac_frame_reg_read(frame, MV_HMAC_SEND_Q_CTRL_REG(queue));
	U32_SET_FIELD(reg_data, MV_HMAC_SEND_Q_CTRL_SEND_Q_EN_MASK, 1 << MV_HMAC_SEND_Q_CTRL_SEND_Q_EN_OFFS);
	mv_pp3_hmac_frame_reg_write(frame, MV_HMAC_SEND_Q_CTRL_REG(queue), reg_data);
}

void mv_pp3_hmac_txq_disable(int frame, int queue)
{
	u32 reg_data;

	/* Disable queue */
	reg_data = mv_pp3_hmac_frame_reg_read(frame, MV_HMAC_SEND_Q_CTRL_REG(queue));
	U32_SET_FIELD(reg_data, MV_HMAC_SEND_Q_CTRL_SEND_Q_EN_MASK, 0);
	mv_pp3_hmac_frame_reg_write(frame, MV_HMAC_SEND_Q_CTRL_REG(queue), reg_data);
}

/* Local functions */
/* allocate descriptors */
static u8 *mv_pp3_queue_mem_alloc(int size)
{
	u8 *p_virt;

	p_virt = kmalloc(size, GFP_ATOMIC);
	/*if (pVirt)
		mvOsMemset(pVirt, 0, descSize);*/

	return p_virt;
}

static int mv_pp3_hmac_queue_create(struct mv_pp3_hmac_queue_ctrl *q_ctrl)
{
	int size;

	/* CFH buffer must be aligned to 256B */
	size = q_ctrl->size * MV_PP3_HMAC_DG_SIZE + MV_PP3_HMAC_Q_ALIGN; /* in bytes */
	/* Allocate memory for queue */
	q_ctrl->buf_ptr = (u32) mv_pp3_queue_mem_alloc(size);
	q_ctrl->first = (u8 *)MV_ALIGN_UP(q_ctrl->buf_ptr, MV_PP3_HMAC_Q_ALIGN);

	if (q_ctrl->first == NULL) {
		pr_err("%s: Can't allocate %d bytes for HMAC queue.\n", __func__, size);
		return 1;
	}

	q_ctrl->next_proc = q_ctrl->first;
	q_ctrl->occ_dg = 0;
	q_ctrl->dummy_dg = 0;
	q_ctrl->cfh_size = 0;
	q_ctrl->end = q_ctrl->first + size;

	return 0;
}

/* Connect queue TX event to SPI interrupt group (BM queue only)
Inputs:
	group - SPI group for event (0 - 7)
*/
void mv_pp3_hmac_txq_event_cfg(int frame, int queue, int group)
{
	u32 reg_data;

	PP3_HMAC_DEBUG;
	/* Configure event group */
	reg_data = mv_pp3_hmac_frame_reg_read(frame, MV_PP3_HMAC_SQ_EVENT_GROUP(queue));
	/* set group for event 0 */
	U32_SET_FIELD(reg_data, MV_PP3_HMAC_SQ_EVENT_GROUP_MASK, group << MV_PP3_HMAC_SQ_EVENT_GROUP_OFFS);
	/* enable event */
	U32_SET_FIELD(reg_data, MV_PP3_HMAC_SQ_EVENT_EN_MASK, 1 << MV_PP3_HMAC_SQ_EVENT_EN_OFFS);

	mv_pp3_hmac_frame_reg_write(frame, MV_PP3_HMAC_SQ_EVENT_GROUP(queue), reg_data);
}

/************************ Print HMAC Frame unit register **************************************/
static void mv_pp3_hmac_fr_reg_print(int frame, char *reg_name, u32 reg)
{
	pr_info("  %-32s: 0x%x = 0x%08x\n", reg_name, reg, mv_pp3_hmac_frame_reg_read(frame, reg));
}

static void mv_pp3_hmac_global_reg_print(char *reg_name, u32 reg)
{
	pr_info("  %-32s: 0x%x = 0x%08x\n", reg_name, reg, mv_pp3_hmac_gl_reg_read(reg));
}

/* dump hmac queue registers */
void mv_pp3_hmac_rxq_regs_dump(int frame, int queue)
{
	pr_info("\n-------------- HMAC RX (frame = %d, queue = %d) regs (0x%x)-----------\n",
		frame, queue, pp3_hmac_fr.base_addr + pp3_hmac_fr.ins_offs * frame);
	mv_pp3_hmac_fr_reg_print(frame, "QUEUE_CTRL", MV_HMAC_REC_Q_CTRL_REG(queue));
	mv_pp3_hmac_fr_reg_print(frame, "QUEUE_STATUS", MV_HMAC_REC_Q_STATUS_REG(queue));
	mv_pp3_hmac_fr_reg_print(frame, "TIMEOUT", MV_HMAC_REC_Q_TIMEOUT_REG(queue));
	mv_pp3_hmac_fr_reg_print(frame, "ADDR_LOW", MV_PP3_HMAC_RQ_ADDR_LOW(queue));
	mv_pp3_hmac_fr_reg_print(frame, "QUEUE_SIZE", MV_PP3_HMAC_RQ_SIZE(queue));
	mv_pp3_hmac_fr_reg_print(frame, "OCC_STATUS", MV_PP3_HMAC_RQ_OCC_STATUS(queue));
	mv_pp3_hmac_fr_reg_print(frame, "AXI_ATTR", MV_PP3_HMAC_RQ_AXI_ATTR(queue));
	mv_pp3_hmac_fr_reg_print(frame, "EVENT_GROUP", MV_PP3_HMAC_RQ_EVENT_GROUP(queue));
	mv_pp3_hmac_fr_reg_print(frame, "INT_THRESH", MV_PP3_HMAC_RQ_INT_THRESH(queue));
}

/* dump hmac queue registers */
void mv_pp3_hmac_txq_regs_dump(int frame, int queue)
{
	pr_info("\n-------------- HMAC TX (frame = %d, queue = %d) regs (0x%x)-----------\n",
		frame, queue, pp3_hmac_fr.base_addr + pp3_hmac_fr.ins_offs * frame);
	mv_pp3_hmac_fr_reg_print(frame, "QUEUE_CTRL", MV_HMAC_SEND_Q_CTRL_REG(queue));
	mv_pp3_hmac_fr_reg_print(frame, "QM_NUM", MV_HMAC_SEND_Q_NUM_BPID_REG(queue));
	mv_pp3_hmac_fr_reg_print(frame, "QUEUE_STATUS", MV_HMAC_SEND_Q_STATUS_REG(queue));
	mv_pp3_hmac_fr_reg_print(frame, "TIMEOUT", MV_HMAC_SEND_Q_TIMEOUT_REG(queue));
	mv_pp3_hmac_fr_reg_print(frame, "ADDR_LOW", MV_PP3_HMAC_SQ_ADDR_LOW(queue));
	mv_pp3_hmac_fr_reg_print(frame, "QUEUE_SIZE", MV_PP3_HMAC_SQ_SIZE(queue));
	mv_pp3_hmac_fr_reg_print(frame, "OCC_STATUS", MV_PP3_HMAC_SQ_OCC_STATUS(queue));
	mv_pp3_hmac_fr_reg_print(frame, "AXI_ATTR", MV_PP3_HMAC_SQ_AXI_ATTR(queue));
	mv_pp3_hmac_fr_reg_print(frame, "EVENT_GROUP", MV_PP3_HMAC_SQ_EVENT_GROUP(queue));
}

void mv_pp3_hmac_frame_regs_dump(int frame)
{
	pr_info("\n-------------- HMAC Frame %d regs -----------\n", frame);
	mv_pp3_hmac_global_reg_print("VMID", MV_HMAC_VMID_FRAME_REG(frame));
	mv_pp3_hmac_global_reg_print("Event Address Low", MV_HMAC_EVENT_ADDR_LOW_REG(frame));
	mv_pp3_hmac_global_reg_print("Event Address High", MV_HMAC_EVENT_ADDR_HIGH_REG(frame));
	mv_pp3_hmac_global_reg_print("DRAM Upd Timeout", MV_HMAC_DRAM_UPDATE_TIME_OUT_REG(frame));
	mv_pp3_hmac_global_reg_print("DRAM Upd Threshold", MV_HMAC_DRAM_UPDATE_THRESHOLD_REG(frame));
	mv_pp3_hmac_global_reg_print("AXI Protection Secure", MV_HMAC_AXI_PROT_SECURE_REG(frame));
}

void mv_pp3_hmac_global_regs_dump(void)
{
	pr_info("\n-------------- HMAC Golbal regs (0x%x) -----------\n", pp3_hmac_gl.base_addr);
	mv_pp3_hmac_global_reg_print("ECO", MV_HMAC_ECO_REG);
	mv_pp3_hmac_global_reg_print("BW Control", MV_HMAC_BW_CTRL_REG);
	mv_pp3_hmac_global_reg_print("Receive QM Port", MV_HMAC_REC_QM_PORT_NUMBER_REG);
	mv_pp3_hmac_global_reg_print("AXI Interrupt Cause", MV_HMAC_AXI_INT_CAUSE);
	mv_pp3_hmac_global_reg_print("AXI Interrupt Mask", MV_HMAC_AXI_INT_MASK);
	mv_pp3_hmac_global_reg_print("AXI Interrupt Syndrome", MV_HMAC_AXI_INT_SYNDROME);
	mv_pp3_hmac_global_reg_print("MISC Interrupt Cause", MV_HMAC_MISC_INT_CAUSE);
	mv_pp3_hmac_global_reg_print("MISC Interrupt Mask", MV_HMAC_MISC_INT_MASK);
	mv_pp3_hmac_global_reg_print("MISC Interrupt Syndrome", MV_HMAC_MISC_INT_SYNDROME);
	mv_pp3_hmac_global_reg_print("HMAC Busy", MV_HMAC_BUSY);
}

static void mv_pp3_hmac_queue_show(struct mv_pp3_hmac_queue_ctrl *rxq)
{
	pr_info("\n-------------- HMAC RX queue (0x%p) -----------", rxq);
	pr_info("\n *first = 0x%p", rxq->first);
	pr_info("\n *next = 0x%p", rxq->next_proc);
	pr_info("\n *end = 0x%p", rxq->end);
	pr_info("\n occ_dg = %d", rxq->occ_dg);
	pr_info("\n dummy_dg = %d", rxq->dummy_dg);
	pr_info("\n size = %d", rxq->size);
	pr_info("\n cfh_size = %d", rxq->cfh_size);
	pr_info("\n");

	return;
}


void mv_pp3_hmac_rx_queue_show(int frame, int queue)
{
	return mv_pp3_hmac_queue_show(mv_hmac_rxq_handle[frame][queue]);
}

void mv_pp3_hmac_tx_queue_show(int frame, int queue)
{
	return mv_pp3_hmac_queue_show(mv_hmac_txq_handle[frame][queue]);
}
