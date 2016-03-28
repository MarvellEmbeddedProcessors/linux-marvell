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
#include "platform/mv_pp3.h"
#include "hmac/mv_hmac.h"

#ifdef PP3_DEBUG
#define PP3_HMAC_DEBUG pr_info("\n%s::", __func__)
#else
#define PP3_HMAC_DEBUG
#endif

int mv_pp3_hmac_debug_flags;
int mv_pp3_hmac_hz_tclk;

struct mv_unit_info pp3_hmac_gl;
struct mv_unit_info pp3_hmac_fr;

/* Array of pointers to HMAC queue control structure */
struct mv_pp3_hmac_queue_ctrl *mv_hmac_rxq_handle[MV_PP3_HFRM_NUM][MV_PP3_HFRM_Q_NUM];
struct mv_pp3_hmac_queue_ctrl *mv_hmac_txq_handle[MV_PP3_HFRM_NUM][MV_PP3_HFRM_Q_NUM];

/* local functions declaration */
static int mv_pp3_hmac_queue_create(struct mv_pp3_hmac_queue_ctrl *q_ctrl);

/* platform pointer for dma operation */
static struct mv_pp3 *pp3_priv;

/* general functions */
/* HMAC unit init */
void mv_pp3_hmac_init(struct mv_pp3 *priv)
{
	void __iomem *base = mv_pp3_nss_regs_vaddr_get();

	mv_pp3_hmac_hz_tclk = mv_pp3_silicon_tclk_get();
	mv_pp3_hmac_gl_unit_base(base);
	mv_pp3_hmac_frame_unit_base(base, MV_PP3_HMAC_FR_INST_OFFSET);
	pp3_priv = priv;
}

/* store unit base address = silicon base address + unit offset */
void mv_pp3_hmac_gl_unit_base(void __iomem *unit_base)
{
	pp3_hmac_gl.base_addr = unit_base;
	pp3_hmac_gl.ins_offs = 0;
}

/* store unit base address = silicon base address + unit offset */
/* store unit instance offset                                   */
void mv_pp3_hmac_frame_unit_base(void __iomem *unit_base, unsigned int ins_offset)
{
	pp3_hmac_fr.base_addr = unit_base;
	pp3_hmac_fr.ins_offs = ins_offset;
}

/* configure rxq packets coalesing profile */
void mv_pp3_hmac_rxq_time_coal_profile_set(int frame, int queue, int profile)
{
	u32 reg_data;

	reg_data = mv_pp3_hmac_frame_reg_read(frame, MV_HMAC_RX_Q_CTRL_REG(queue));

	if (profile)
		reg_data |= MV_HMAC_RX_Q_CTRL_RCV_Q_TIMER_SEL_MASK;
	else
		reg_data &= ~MV_HMAC_RX_Q_CTRL_RCV_Q_TIMER_SEL_MASK;

	mv_pp3_hmac_frame_reg_write(frame, MV_HMAC_RX_Q_CTRL_REG(queue), reg_data);
}

/* configure frame time coalesing profile
   inputs:
	frame - frame id
	profile - time profile num (0 or 1)
	usec - time in micro seconds, if usec is zero time timer is disabled
*/
void mv_pp3_hmac_frame_time_coal_set(int frame, int profile, int usec)
{
	u32 reg_data;
	int t_clk_usec;

	/* Register contains interrupt time in units of 256 core clock cycles */

	t_clk_usec = mv_pp3_hmac_hz_tclk / 1000000;
	t_clk_usec = MV_ALIGN_UP(t_clk_usec * usec, 256);

	if ((t_clk_usec != 0) && (t_clk_usec < 256))
		t_clk_usec = 256;

	t_clk_usec = t_clk_usec / 256;

	reg_data = mv_pp3_hmac_gl_reg_read(MV_HMAC_RX_Q_TIMEOUT_REG(frame));
	if (profile) {
		reg_data &= ~MV_HMAC_RX_Q_TIMEOUT_RQ_TIMEOUT_1_MASK;
		reg_data |= ((t_clk_usec) << MV_HMAC_RX_Q_TIMEOUT_RQ_TIMEOUT_1_OFFS);
	} else {
		reg_data &= ~MV_HMAC_RX_Q_TIMEOUT_RQ_TIMEOUT_0_MASK;
		reg_data |= ((t_clk_usec) << MV_HMAC_RX_Q_TIMEOUT_RQ_TIMEOUT_0_OFFS);
	}
	mv_pp3_hmac_gl_reg_write(MV_HMAC_RX_Q_TIMEOUT_REG(frame), reg_data);
}

/* configure frame parameters */
void mv_pp3_hmac_frame_cfg(u32 frame, u8 vm_id)
{
	u32 reg_data;

	PP3_HMAC_DEBUG;
	reg_data = mv_pp3_hmac_gl_reg_read(MV_HMAC_VMID_FRAME_REG(frame));
	MV_U32_SET_FIELD(reg_data, MV_HMAC_VMID_FRAME_CONTEXT_ID_MASK,
		(vm_id & MV_HMAC_VMID_FRAME_CONTEXT_ID_MASK) << MV_HMAC_VMID_FRAME_CONTEXT_ID_OFFS);
	MV_U32_SET_FIELD(reg_data, MV_HMAC_VMID_FRAME_AXI_PROT_PRIVILEGE_MASK,
			((vm_id >> 6) & 1) << MV_HMAC_VMID_FRAME_AXI_PROT_PRIVILEGE_OFFS);
	mv_pp3_hmac_gl_reg_write(MV_HMAC_VMID_FRAME_REG(frame), reg_data);

	reg_data = mv_pp3_hmac_gl_reg_read(MV_HMAC_AXI_PROT_SECURE_REG(frame));
	MV_U32_SET_FIELD(reg_data, MV_HMAC_AXI_PROT_SECURE_AXI_PROT_SECURE_MASK,
	((vm_id >> 7) & MV_HMAC_AXI_PROT_SECURE_AXI_PROT_SECURE_MASK) << MV_HMAC_AXI_PROT_SECURE_AXI_PROT_SECURE_OFFS);
	mv_pp3_hmac_gl_reg_write(MV_HMAC_AXI_PROT_SECURE_REG(frame), reg_data);
#ifdef CONFIG_MV_PP3_FPGA
	mv_pp3_hmac_gl_reg_write(MV_HMAC_EVENT_ADDR_LOW_REG(frame), 0);
	/* disable all frame events */
	mv_pp3_hmac_gl_reg_write(MV_HMAC_EVENT_MASK_REG(frame), MV_HMAC_EVENT_MASK_GROUP_DIS_MASK);
#else
	mv_pp3_hmac_gl_reg_write(MV_HMAC_EVENT_ADDR_LOW_REG(frame),
		mv_pp3_internal_regs_paddr_get(pp3_priv) + MV_A390_GIC_INTERRUPT_REG(frame));
#endif
	mv_pp3_hmac_gl_reg_write(MV_HMAC_EVENT_ADDR_HIGH_REG(frame), 0);
}


/* configure queue to be used like BM queue */
void mv_pp3_hmac_queue_bm_mode_cfg(int frame, int queue)
{
	u32 reg_data;

	PP3_HMAC_DEBUG;
	reg_data = mv_pp3_hmac_frame_reg_read(frame, MV_HMAC_SEND_Q_CTRL_REG(queue));
	MV_U32_SET_FIELD(reg_data, MV_HMAC_SEND_Q_CTRL_Q_MODE_MASK, 1 << MV_HMAC_SEND_Q_CTRL_Q_MODE_OFFS);
	MV_U32_SET_FIELD(reg_data, MV_HMAC_SEND_Q_CTRL_BM_PE_FORMAT_MASK, 1 << MV_HMAC_SEND_Q_CTRL_BM_PE_FORMAT_OFFS);

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
	MV_U32_SET_FIELD(reg_data, MV_HMAC_SEND_Q_CTRL_Q_MODE_MASK, 0);
	mv_pp3_hmac_frame_reg_write(frame, MV_HMAC_SEND_Q_CTRL_REG(queue), reg_data);
	/* map QM queue number */
	mv_pp3_hmac_frame_reg_write(frame, MV_HMAC_SEND_Q_NUM_BPID_REG(queue), qm_num);
}

/* Allocate queue memory and queue control structure
 * size is a queue size in datagrams (16 bytes each) will align to 8 datagramms
 * Returns - pointer to HMAC queue structure */
static void *mv_pp3_hmac_queue_alloc(int size)
{
	struct mv_pp3_hmac_queue_ctrl *qctrl;

	if (size > MV_PP3_HMAC_Q_SIZE_MASK) {
		pr_err("%s: cannot create HMAC RX queue, size %d too big", __func__, size);
		return NULL;
	}

	if (MV_IS_NOT_ALIGN(size, MV_PP3_CFH_DG_MAX_NUM)) {
		pr_err("%s: Illegal queue size %d.", __func__, size);
		return NULL;
	}

	/* 140 DG - HMAC internal FIFO size */
	/* 32 DG - BP Hysteresis recomented value */
	if (size <= (140 + 32)) {
		pr_err("%s: cannot create HMAC RX queue, size %d too small", __func__, size);
		return NULL;
	}

	/* allocate hmac queue control stucture */
	qctrl = kmalloc(sizeof(struct mv_pp3_hmac_queue_ctrl), GFP_KERNEL);
	if (qctrl == NULL) {
		pr_err("%s: cannot create HMAC RX queue, no memory", __func__);
		return NULL;
	}

	qctrl->size = size;
	if (mv_pp3_hmac_queue_create(qctrl) != 0) {
		kfree(qctrl);
		return NULL;
	}

	/* default capacity is a queue size */
	qctrl->capacity = size;

	return qctrl;
}

/************************ RX queue functions **************************************************/
/* Allocate memory and init RX queue HW facility
 * size is a queue size in datagrams (16 bytes each) will align to 8 datagramms
 * Returns - pointer to HMAC RX queue structure */
void *mv_pp3_hmac_rxq_init(int frame, int queue, int size)
{
	struct mv_pp3_hmac_queue_ctrl *qctrl;
	u32 reg_data;
	u32 phys_addr;

	PP3_HMAC_DEBUG;

	if (mv_hmac_rxq_handle[frame][queue] == NULL) {
		qctrl = mv_pp3_hmac_queue_alloc(size);
		if (qctrl == NULL)
			return qctrl;
		mv_hmac_rxq_handle[frame][queue] = qctrl;
	} else
		qctrl = mv_hmac_rxq_handle[frame][queue];

	/* Write pointer to allocated memory */
	phys_addr = mv_pp3_os_dma_map_single(mv_pp3_dev_get(pp3_priv), qctrl->first, size, DMA_FROM_DEVICE);
	mv_pp3_hmac_frame_reg_write(frame, MV_PP3_HMAC_RQ_ADDR_LOW(queue), phys_addr);
	/* Store queue size in rq_size table, number of 16B units */
	mv_pp3_hmac_frame_reg_write(frame, MV_PP3_HMAC_RQ_SIZE(queue), (u32)qctrl->size);

	/* disable all queue events */
	reg_data = MV_PP3_HMAC_RQ_EVENT0_DIS_MASK | MV_PP3_HMAC_RQ_EVENT1_DIS_MASK;
	mv_pp3_hmac_frame_reg_write(frame, MV_PP3_HMAC_RQ_EVENT_GROUP(queue), reg_data);

	return qctrl;
}

int mv_pp3_hmac_rxq_bp_thresh_set(int frame, int queue, int thresh_dg)
{
	u32 reg_data;
	struct mv_pp3_hmac_queue_ctrl *qctrl = mv_hmac_rxq_handle[frame][queue];

	if (qctrl == NULL) {
		pr_err("%s: HMAC rxq: frame=%d, rxq=%d doesn't exist\n", __func__,
			frame, queue);
		return -1;
	}

	if (thresh_dg > qctrl->size) {
		pr_err("%s: XOFF threshold #%d [dg] is too large. Maximum is #%d [dg]\n", __func__,
			thresh_dg, qctrl->size);
		return -1;
	}
	/* maximum XOFF threshold must be less than RQ allocated size by 140 dg */
	if (thresh_dg > (qctrl->size - 140))
		thresh_dg = qctrl->size - 140;

	reg_data = (thresh_dg << MV_PP3_HMAC_RQ_BP_XOFF_THRESH_OFFS);

	/* XON threshold must be less than XOFF threshold (32 dg) */
	reg_data |= ((thresh_dg - 32) << MV_PP3_HMAC_RQ_BP_XON_THRESH_OFFS);
	mv_pp3_hmac_frame_reg_write(frame, MV_PP3_HMAC_RQ_BACK_PRES0(queue), reg_data);

	return 0;
}

/* Configure Receive queue threshold.
* node_type	- BP levels: Q=0, A=1, B=2, C=3, P=4
* node_id	- number of node type
* BP threshold set to maximum possible value accordingly with allocated RQ size
*/
void mv_pp3_hmac_rxq_bp_node_set(int frame, int queue, enum mv_qm_node_type node_type, int node_id)
{
	u32 reg_data;

	reg_data = (node_type << 12); /* BP levels: Q=0, A=1, B=2, C=3, P=4 */
	reg_data |= node_id;          /* BP node number */

	mv_pp3_hmac_frame_reg_write(frame, MV_PP3_HMAC_RQ_BACK_PRES1(queue), reg_data);
}

void mv_pp3_hmac_rxq_flush(int frame, int queue)
{
	u32 reg_data;

	/* Enable queue flush*/
	reg_data = mv_pp3_hmac_frame_reg_read(frame, MV_HMAC_RX_Q_CTRL_REG(queue));
	reg_data |= MV_HMAC_RX_Q_CTRL_RCV_Q_FLUSH_MASK;
	mv_pp3_hmac_frame_reg_write(frame, MV_HMAC_RX_Q_CTRL_REG(queue), reg_data);

	/* clear the reflected pointers and HMAC internal registers */
	mv_pp3_hmac_frame_reg_write(frame, MV_PP3_HMAC_RQ_OCC_STATUS(queue), 0);

	/* Disable queue flush*/
	reg_data = mv_pp3_hmac_frame_reg_read(frame, MV_HMAC_RX_Q_CTRL_REG(queue));
	reg_data &= ~MV_HMAC_RX_Q_CTRL_RCV_Q_FLUSH_MASK;
	mv_pp3_hmac_frame_reg_write(frame, MV_HMAC_RX_Q_CTRL_REG(queue), reg_data);
}

void mv_pp3_hmac_rxq_enable(int frame, int queue)
{
	u32 reg_data;

	/* Enable queue */
	reg_data = mv_pp3_hmac_frame_reg_read(frame, MV_HMAC_RX_Q_CTRL_REG(queue));
	MV_U32_SET_FIELD(reg_data, MV_HMAC_RX_Q_CTRL_RCV_Q_EN_MASK, 1 << MV_HMAC_RX_Q_CTRL_RCV_Q_EN_OFFS);
	mv_pp3_hmac_frame_reg_write(frame, MV_HMAC_RX_Q_CTRL_REG(queue), reg_data);
}

void mv_pp3_hmac_rxq_disable(int frame, int queue)
{
	u32 reg_data;

	/* Disable queue */
	reg_data = mv_pp3_hmac_frame_reg_read(frame, MV_HMAC_RX_Q_CTRL_REG(queue));
	MV_U32_SET_FIELD(reg_data, MV_HMAC_RX_Q_CTRL_RCV_Q_EN_MASK, 0);
	mv_pp3_hmac_frame_reg_write(frame, MV_HMAC_RX_Q_CTRL_REG(queue), reg_data);
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
		MV_U32_SET_FIELD(reg_data, MV_PP3_HMAC_RQ_EVENT0_GROUP_MASK, group << MV_PP3_HMAC_RQ_EVENT0_GROUP_OFFS);
		/* enable event,  0 is active */
		reg_data &= ~MV_PP3_HMAC_RQ_EVENT0_DIS_MASK;
	} else if (event == 1) {
		MV_U32_SET_FIELD(reg_data, MV_PP3_HMAC_RQ_EVENT1_GROUP_MASK, group << MV_PP3_HMAC_RQ_EVENT1_GROUP_OFFS);
		/* enable event */
		reg_data &= ~MV_PP3_HMAC_RQ_EVENT1_DIS_MASK;
	}
	mv_pp3_hmac_frame_reg_write(frame, MV_PP3_HMAC_RQ_EVENT_GROUP(queue), reg_data);
}

/* Disable RX events on queue
	QM queue - Timeout or new items added to the queue
	BM queue - allocate completed
*/
void mv_pp3_hmac_rxq_event_disable(int frame, int queue)
{
	u32 reg_data;

	PP3_HMAC_DEBUG;

	reg_data = mv_pp3_hmac_frame_reg_read(frame, MV_PP3_HMAC_RQ_EVENT_GROUP(queue));
	reg_data |= MV_PP3_HMAC_RQ_EVENT0_DIS_MASK;

	mv_pp3_hmac_frame_reg_write(frame, MV_PP3_HMAC_RQ_EVENT_GROUP(queue), reg_data);
}

/* Enable RX events on queue
	QM queue - Timeout or new items added to the queue
	BM queue - allocate completed
*/
void mv_pp3_hmac_rxq_event_enable(int frame, int queue)
{
	u32 reg_data;

	PP3_HMAC_DEBUG;

	reg_data = mv_pp3_hmac_frame_reg_read(frame, MV_PP3_HMAC_RQ_EVENT_GROUP(queue));
	reg_data &= ~MV_PP3_HMAC_RQ_EVENT0_DIS_MASK;

	mv_pp3_hmac_frame_reg_write(frame, MV_PP3_HMAC_RQ_EVENT_GROUP(queue), reg_data);
}

/* Pause RX events on queue */
void mv_pp3_hmac_rxq_pause(int frame, int queue)
{
	u32 reg_data;

	PP3_HMAC_DEBUG;

	reg_data = mv_pp3_hmac_frame_reg_read(frame, MV_PP3_HMAC_RQ_EVENT_GROUP(queue));
	reg_data |= MV_PP3_HMAC_RQ_EVENT0_DIS_MASK;

	mv_pp3_hmac_frame_reg_write(frame, MV_PP3_HMAC_RQ_EVENT_GROUP(queue), reg_data);
}

/* Resume RX events on queue */
void mv_pp3_hmac_rxq_resume(int frame, int queue)
{
	u32 reg_data;

	PP3_HMAC_DEBUG;

	reg_data = mv_pp3_hmac_frame_reg_read(frame, MV_PP3_HMAC_RQ_EVENT_GROUP(queue));
	reg_data &= ~MV_PP3_HMAC_RQ_EVENT0_DIS_MASK;

	mv_pp3_hmac_frame_reg_write(frame, MV_PP3_HMAC_RQ_EVENT_GROUP(queue), reg_data);
	/* clear the reflected pointers and HMAC internal registers */
	mv_pp3_hmac_frame_reg_write(frame, MV_PP3_HMAC_RQ_OCC_STATUS(queue), 0);
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
	u32 phys_addr;

	PP3_HMAC_DEBUG;

	if (mv_hmac_txq_handle[frame][queue] == NULL) {
		qctrl = mv_pp3_hmac_queue_alloc(size);
		if (qctrl == NULL)
			return qctrl;
		mv_hmac_txq_handle[frame][queue] = qctrl;
	} else
		qctrl = mv_hmac_txq_handle[frame][queue];

	qctrl->cfh_size = cfh_size;

	/* Write pointer to allocated memory */
	phys_addr = mv_pp3_os_dma_map_single(mv_pp3_dev_get(pp3_priv), qctrl->first, size, DMA_TO_DEVICE);
	mv_pp3_hmac_frame_reg_write(frame, MV_PP3_HMAC_SQ_ADDR_LOW(queue), phys_addr);
	/* Store queue size in rq_size table, number of 16B units */
	mv_pp3_hmac_frame_reg_write(frame, MV_PP3_HMAC_SQ_SIZE(queue), (u32)qctrl->size);

	/* disable all queue events */
	reg_data = MV_PP3_HMAC_SQ_EVENT_DIS_MASK;
	mv_pp3_hmac_frame_reg_write(frame, MV_PP3_HMAC_SQ_EVENT_GROUP(queue), reg_data);

	return qctrl;
}

void mv_pp3_hmac_txq_flush(int frame, int queue)
{
	u32 reg_data;

	/* Enable queue flush */
	reg_data = mv_pp3_hmac_frame_reg_read(frame, MV_HMAC_SEND_Q_CTRL_REG(queue));
	reg_data |= MV_HMAC_SEND_Q_CTRL_SEND_Q_FLUSH_MASK;
	mv_pp3_hmac_frame_reg_write(frame, MV_HMAC_SEND_Q_CTRL_REG(queue), reg_data);

	/* clear reflected pointers and HMAC internal registers */
	mv_pp3_hmac_frame_reg_write(frame, MV_PP3_HMAC_SQ_OCC_STATUS(queue), 0);

	/* Disable queue flush */
	reg_data = mv_pp3_hmac_frame_reg_read(frame, MV_HMAC_SEND_Q_CTRL_REG(queue));
	reg_data &= ~MV_HMAC_SEND_Q_CTRL_SEND_Q_FLUSH_MASK;
	mv_pp3_hmac_frame_reg_write(frame, MV_HMAC_SEND_Q_CTRL_REG(queue), reg_data);
}

void mv_pp3_hmac_txq_enable(int frame, int queue)
{
	u32 reg_data;

	/* Enable queue */
	reg_data = mv_pp3_hmac_frame_reg_read(frame, MV_HMAC_SEND_Q_CTRL_REG(queue));
	MV_U32_SET_FIELD(reg_data, MV_HMAC_SEND_Q_CTRL_SEND_Q_EN_MASK, 1 << MV_HMAC_SEND_Q_CTRL_SEND_Q_EN_OFFS);
	mv_pp3_hmac_frame_reg_write(frame, MV_HMAC_SEND_Q_CTRL_REG(queue), reg_data);
}

void mv_pp3_hmac_txq_disable(int frame, int queue)
{
	u32 reg_data;

	/* Disable queue */
	reg_data = mv_pp3_hmac_frame_reg_read(frame, MV_HMAC_SEND_Q_CTRL_REG(queue));
	MV_U32_SET_FIELD(reg_data, MV_HMAC_SEND_Q_CTRL_SEND_Q_EN_MASK, 0);
	mv_pp3_hmac_frame_reg_write(frame, MV_HMAC_SEND_Q_CTRL_REG(queue), reg_data);
}

/* Local functions */
static int mv_pp3_hmac_queue_create(struct mv_pp3_hmac_queue_ctrl *q_ctrl)
{
	int size;
	int tail_room = MV_PP3_HMAC_Q_ALIGN;

	/* CFH buffer must be aligned to 256B */
	size = q_ctrl->size * MV_PP3_CFH_DG_SIZE; /* in bytes */
	/* Allocate memory for queue. Add tail buffer to handle split CFH. */
	q_ctrl->first = kzalloc(size + tail_room, GFP_ATOMIC);
	if (q_ctrl->first) {
		if (MV_IS_NOT_ALIGN((u32)(q_ctrl->first), MV_PP3_HMAC_Q_ALIGN)) {
			pr_err("%s: Allocate not aligned pointer 0x%p (%d bytes)\n", __func__, q_ctrl->first, size);
			q_ctrl->first = 0;
			return -1;
		}
	}
	if (q_ctrl->first == NULL) {
		pr_err("%s: Can't allocate %d bytes for HMAC queue.\n", __func__, size);
		return -1;
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
	MV_U32_SET_FIELD(reg_data, MV_PP3_HMAC_SQ_EVENT_GROUP_MASK, group << MV_PP3_HMAC_SQ_EVENT_GROUP_OFFS);
	/* enable event */
	MV_U32_SET_FIELD(reg_data, MV_PP3_HMAC_SQ_EVENT_DIS_MASK, 1 << MV_PP3_HMAC_SQ_EVENT_DIS_OFFS);

	mv_pp3_hmac_frame_reg_write(frame, MV_PP3_HMAC_SQ_EVENT_GROUP(queue), reg_data);
}

/************************ Print HMAC Frame unit register **************************************/
static void mv_pp3_hmac_fr_reg_print(int frame, char *reg_name, u32 reg)
{
	pr_info("  %-32s: 0x%04x = 0x%08x\n", reg_name, reg, mv_pp3_hmac_frame_reg_read(frame, reg));
}

static void mv_pp3_hmac_global_reg_print(char *reg_name, u32 reg)
{
	pr_info("  %-32s: 0x%04x = 0x%08x\n", reg_name, reg, mv_pp3_hmac_gl_reg_read(reg));
}

/* dump hmac queue registers */
void mv_pp3_hmac_rxq_regs_dump(int frame, int queue)
{
	if (mv_pp3_max_check(frame, MV_PP3_HFRM_NUM, "HMAC frame"))
		return;
	if (mv_pp3_max_check(queue, MV_PP3_HFRM_Q_NUM, "HMAC queue"))
		return;

	pr_info("\n-------------- HMAC RX (frame = %d, queue = %d) regs (%p)-----------\n",
		frame, queue, pp3_hmac_fr.base_addr + pp3_hmac_fr.ins_offs * frame);
	mv_pp3_hmac_fr_reg_print(frame, "QUEUE CONTROL", MV_HMAC_RX_Q_CTRL_REG(queue));
	mv_pp3_hmac_fr_reg_print(frame, "QUEUE STATUS", MV_HMAC_RX_Q_STATUS_REG(queue));
	mv_pp3_hmac_fr_reg_print(frame, "QUEUE ADDRESS LOW", MV_PP3_HMAC_RQ_ADDR_LOW(queue));
	mv_pp3_hmac_fr_reg_print(frame, "QUEUE SIZE", MV_PP3_HMAC_RQ_SIZE(queue));
	mv_pp3_hmac_fr_reg_print(frame, "OCCUPIED STATUS", MV_PP3_HMAC_RQ_OCC_STATUS(queue));
	mv_pp3_hmac_fr_reg_print(frame, "AXI ATTRIBUTES", MV_PP3_HMAC_RQ_AXI_ATTR(queue));
	mv_pp3_hmac_fr_reg_print(frame, "EVENT GROUP", MV_PP3_HMAC_RQ_EVENT_GROUP(queue));
	mv_pp3_hmac_fr_reg_print(frame, "INTERRUPT THRESHOLD", MV_PP3_HMAC_RQ_INT_THRESH(queue));
	mv_pp3_hmac_fr_reg_print(frame, "BACK PRESSURE 0", MV_PP3_HMAC_RQ_BACK_PRES0(queue));
	mv_pp3_hmac_fr_reg_print(frame, "BACK PRESSURE 1", MV_PP3_HMAC_RQ_BACK_PRES1(queue));
}

/* dump hmac queue registers */
void mv_pp3_hmac_txq_regs_dump(int frame, int queue)
{
	if (mv_pp3_max_check(frame, MV_PP3_HFRM_NUM, "HMAC frame"))
		return;
	if (mv_pp3_max_check(queue, MV_PP3_HFRM_Q_NUM, "HMAC queue"))
		return;

	pr_info("\n-------------- HMAC TX (frame = %d, queue = %d) regs (%p)-----------\n",
		frame, queue, pp3_hmac_fr.base_addr + pp3_hmac_fr.ins_offs * frame);
	mv_pp3_hmac_fr_reg_print(frame, "QUEUE CONTROL", MV_HMAC_SEND_Q_CTRL_REG(queue));
	mv_pp3_hmac_fr_reg_print(frame, "QUEUE NUMBER BPID", MV_HMAC_SEND_Q_NUM_BPID_REG(queue));
	mv_pp3_hmac_fr_reg_print(frame, "QUEUE STATUS", MV_HMAC_SEND_Q_STATUS_REG(queue));
	mv_pp3_hmac_fr_reg_print(frame, "QUEUE ADDRESS LOW", MV_PP3_HMAC_SQ_ADDR_LOW(queue));
	mv_pp3_hmac_fr_reg_print(frame, "QUEUE SIZE", MV_PP3_HMAC_SQ_SIZE(queue));
	mv_pp3_hmac_fr_reg_print(frame, "OCCUPIED STATUS", MV_PP3_HMAC_SQ_OCC_STATUS(queue));
	mv_pp3_hmac_fr_reg_print(frame, "AXI ATTRIBUTES", MV_PP3_HMAC_SQ_AXI_ATTR(queue));
	mv_pp3_hmac_fr_reg_print(frame, "EVENT GROUP", MV_PP3_HMAC_SQ_EVENT_GROUP(queue));
}

void mv_pp3_hmac_frame_regs_dump(int frame)
{
	if (mv_pp3_max_check(frame, MV_PP3_HFRM_NUM, "HMAC frame"))
		return;

	pr_info("\n-------------- HMAC Frame %d regs -----------\n", frame);
	mv_pp3_hmac_global_reg_print("VMID", MV_HMAC_VMID_FRAME_REG(frame));
	mv_pp3_hmac_global_reg_print("EVENT ADDRESS LOW", MV_HMAC_EVENT_ADDR_LOW_REG(frame));
	mv_pp3_hmac_global_reg_print("EVENT ADDRESS HIGH", MV_HMAC_EVENT_ADDR_HIGH_REG(frame));
	mv_pp3_hmac_global_reg_print("EVENT MASK", MV_HMAC_EVENT_MASK_REG(frame));
	mv_pp3_hmac_global_reg_print("AXI PROTECTION SECURE", MV_HMAC_AXI_PROT_SECURE_REG(frame));
	mv_pp3_hmac_global_reg_print("RX TIMEOUT", MV_HMAC_RX_Q_TIMEOUT_REG(frame));
	mv_pp3_hmac_global_reg_print("TX TIMEOUT", MV_HMAC_SEND_Q_TIMEOUT_REG(frame));
}

void mv_pp3_hmac_global_regs_dump(void)
{
	pr_info("\n-------------- HMAC Golbal regs (%p) -----------\n", pp3_hmac_gl.base_addr);
	mv_pp3_hmac_global_reg_print("ECO", MV_HMAC_ECO_REG);
	mv_pp3_hmac_global_reg_print("RECEIVE QM Port", MV_HMAC_RX_QM_PORT_NUMBER_REG);
	mv_pp3_hmac_global_reg_print("AXI INTERRUPT CAUSE", MV_HMAC_AXI_INT_CAUSE);
	mv_pp3_hmac_global_reg_print("AXI INTERRUPT MASK", MV_HMAC_AXI_INT_MASK);
	mv_pp3_hmac_global_reg_print("AXI INTERRUPT SYNDROME", MV_HMAC_AXI_INT_SYNDROME);
	mv_pp3_hmac_global_reg_print("MISC INTERRUPT CAUSE", MV_HMAC_MISC_INT_CAUSE);
	mv_pp3_hmac_global_reg_print("MISC INTERRUPT MASK", MV_HMAC_MISC_INT_MASK);
	mv_pp3_hmac_global_reg_print("MISC INTERRUPT SYNDROME", MV_HMAC_MISC_INT_SYNDROME);
	mv_pp3_hmac_global_reg_print("HMAC BUSY", MV_HMAC_BUSY_REG);
}

static void mv_pp3_hmac_rxq_queue_show(int frame, int queue)
{
	struct mv_pp3_hmac_queue_ctrl *rxq = mv_hmac_rxq_handle[frame][queue];
	u32 reg_data;

	reg_data = mv_pp3_hmac_frame_reg_read(frame, MV_PP3_HMAC_RQ_SIZE(queue));
	pr_info("Size in datargams          : %-4d\t\t(%d)\n", MV_PP3_HMAC_Q_SIZE_MASK & reg_data, rxq->size);
	reg_data = mv_pp3_hmac_frame_reg_read(frame, MV_PP3_HMAC_RQ_ADDR_LOW(queue));
	pr_info("First CFH                  : 0x%08x\t\t(0x%p)\n", reg_data, rxq->first);
	reg_data = mv_pp3_hmac_frame_reg_read(frame, MV_PP3_HMAC_RQ_OCC_STATUS(queue));
	pr_info("Next to process            : 0x%04x\t\t(0x%p)\n",
		((reg_data >> 16) & MV_PP3_HMAC_OCC_COUNTER_MASK) * 16, rxq->next_proc);
	pr_info("Occupated datagrams        : %-4d\n", reg_data & MV_PP3_HMAC_OCC_COUNTER_MASK);

	return;
}

static void mv_pp3_hmac_txq_queue_show(int frame, int queue)
{
	struct mv_pp3_hmac_queue_ctrl *txq = mv_hmac_txq_handle[frame][queue];
	u32 reg_data;

	reg_data = mv_pp3_hmac_frame_reg_read(frame, MV_PP3_HMAC_SQ_SIZE(queue));
	pr_info("Capacity in datagrams      : %-4d\n", txq->capacity);
	pr_info("Size in datargams          : %-4d\t\t(%d)\n", MV_PP3_HMAC_Q_SIZE_MASK & reg_data, txq->size);
	reg_data = mv_pp3_hmac_frame_reg_read(frame, MV_PP3_HMAC_SQ_ADDR_LOW(queue));
	pr_info("First CFH                  : 0x%08x\t\t(0x%p)\n", reg_data, txq->first);
	reg_data = mv_pp3_hmac_frame_reg_read(frame, MV_PP3_HMAC_SQ_OCC_STATUS(queue));
	pr_info("Next to process            : 0x%04x\t\t(0x%p)\n",
			((reg_data >> 16) & MV_PP3_HMAC_OCC_COUNTER_MASK) * 16,	txq->next_proc);
	pr_info("Occupied datagrams         : %-4d\t\t(%d)\n", reg_data & MV_PP3_HMAC_OCC_COUNTER_MASK, txq->occ_dg);
	if (txq->cfh_size)
		/* relevant for txq with constant CFH size */
		pr_info("CFH size                   : %d\n", txq->cfh_size);

	return;
}

static void mv_pp3_hmac_queue_dump(struct mv_pp3_hmac_queue_ctrl *qctrl, bool print_all)
{
	struct cfh_common_format *cfh;
	u32 *tmp_cfh;
	u8  *cfh_curr;
	unsigned int cfh_size; /* real CFH size aligned to 16 bytes in bytes */
	int i, j;

	j = 1;
	cfh = (struct cfh_common_format *)qctrl->first;
	cfh_curr = (u8 *)qctrl->first;
	do {
		cfh_size = cfh->cfh_length;
		tmp_cfh = (u32 *)cfh_curr;
		pr_info("%3d. cfh_ptr=0x%p: cfh_length=%3d, pkt_length=%5d, qm_cntrl=0x%04x, cfh_format=0x%02x, bp_id=%2d\n",
			j++, cfh, cfh_size, cfh->pkt_length, cfh->qm_cntrl, cfh->cfh_format, cfh->bp_id);
		pr_info("     Common CFH   : ");
		for (i = 0; i < 32/4; i++)
			pr_cont("0x%08x ", tmp_cfh[i]);

		if (print_all && (cfh_size > 32)) {
			u8 *tmp;
			pr_info("     Data         : ");
			tmp = (u8 *)&tmp_cfh[8];
			for (i = 0; i < cfh_size - 32; i++) {
				if ((i != 0) && ((i%32) == 0))
					pr_cont("\n                    ");
				pr_cont("%02x ", tmp[i]);
			}
			pr_info("\n");
		}
		cfh_size = MV_ALIGN_UP(cfh_size, 16);
		cfh_curr += cfh_size;
		cfh = (struct cfh_common_format *)cfh_curr;
	} while ((cfh_size > 0) && ((u8 *)cfh < qctrl->end));
	pr_info("\n");
}

void mv_pp3_hmac_rxq_bp_show(int frame, int queue)
{
	u32 reg_data;
	char level;

	reg_data = mv_pp3_hmac_frame_reg_read(frame, MV_PP3_HMAC_RQ_BACK_PRES1(queue));

	switch ((reg_data >> 12) & 7) {
	case 0:
		level = 'Q';
		break;
	case 1:
		level = 'A';
		break;
	case 2:
		level = 'B';
		break;
	case 3:
		level = 'C';
		break;
	case 4:
		level = 'P';
		break;
	default:
		level = 'N';
		break;
	}
	pr_info("Back pressure node         : %c %d\n", level, reg_data & 0xFFF);

	reg_data = mv_pp3_hmac_frame_reg_read(frame, MV_PP3_HMAC_RQ_BACK_PRES0(queue));
	pr_info("Back pressure ON (dg)      : %d\n", (reg_data >> 16) & 0xFFFF);
	pr_info("Back pressure OFF (dg)     : %d\n", reg_data & 0xFFFF);
}

void mv_pp3_hmac_rxq_coal_get(int frame, int queue, int *profile, int *dg_num)
{
	u32 reg_data;

	if (profile) {
		reg_data = mv_pp3_hmac_frame_reg_read(frame, MV_HMAC_RX_Q_CTRL_REG(queue));
		*profile = (reg_data >> MV_HMAC_RX_Q_CTRL_RCV_Q_TIMER_SEL_OFFS) & 1;
	}

	if (dg_num)
		*dg_num = mv_pp3_hmac_frame_reg_read(frame, MV_PP3_HMAC_RQ_INT_THRESH(queue));
}

void mv_pp3_hmac_frame_time_coal_get(int frame, int profile, int *usec)
{
	int tmp;
	u32 reg_data;

	reg_data = mv_pp3_hmac_gl_reg_read(MV_HMAC_RX_Q_TIMEOUT_REG(frame));
	tmp = (profile) ? (reg_data >> MV_HMAC_RX_Q_TIMEOUT_RQ_TIMEOUT_1_OFFS) & 0x7FF :
		(reg_data >> MV_HMAC_RX_Q_TIMEOUT_RQ_TIMEOUT_0_OFFS) & 0x7FF;

	/* Register contains interrupt time in units of 256 core clock cycles */
	*usec = tmp * 256 / (mv_pp3_hmac_hz_tclk / 1000000);
}

void mv_pp3_hmac_rxq_coal_show(int frame, int queue)
{
	int profile, time, dg;

	mv_pp3_hmac_rxq_coal_get(frame, queue, &profile, &dg);
	mv_pp3_hmac_frame_time_coal_get(frame, profile, &time);
	pr_info("Interrupt coal (datagrams) : %d\n", dg);
	pr_info("Interrupt time coal (usec) : %d\t\t(profile %d)\n", time, profile);
}

static void mv_pp3_hmac_rxq_status_show(int frame, int queue)
{
	u32 reg_data;

	reg_data = mv_pp3_hmac_frame_reg_read(frame, MV_HMAC_RX_Q_CTRL_REG(queue));
	pr_info("Queue status               : ");
	if (reg_data & MV_HMAC_RX_Q_CTRL_RCV_Q_EN_MASK)
		pr_cont("enabled");
	else
		pr_cont("disabled");
	reg_data = mv_pp3_hmac_frame_reg_read(frame, MV_HMAC_RX_Q_STATUS_REG(queue));
	if (reg_data & MV_HMAC_RX_Q_STATUS_RQ_BUSY_MASK)
		pr_cont(", busy");

	pr_info("\n");
}

static void mv_pp3_hmac_txq_status_show(int frame, int queue)
{
	u32 reg_data, status;

	reg_data = mv_pp3_hmac_frame_reg_read(frame, MV_HMAC_SEND_Q_CTRL_REG(queue));
	pr_info("Queue status               : ");
	if (reg_data & MV_HMAC_SEND_Q_CTRL_SEND_Q_EN_MASK)
		pr_cont("enabled");
	else
		pr_cont("disabled");
	status = mv_pp3_hmac_frame_reg_read(frame, MV_HMAC_SEND_Q_STATUS_REG(queue));
	if (status & MV_HMAC_SEND_Q_STATUS_SQ_BUSY_MASK)
		pr_cont(", busy");
	if (status & MV_HMAC_SEND_Q_STATUS_SQ_QM_BP_OFF_OFFS)
		pr_cont(", BP OFF");

	if (reg_data & MV_HMAC_SEND_Q_CTRL_Q_MODE_MASK) {
		pr_info("BM queue                   : ");
		if (reg_data & MV_HMAC_SEND_Q_CTRL_BM_PE_FORMAT_MASK)
			pr_cont("double PE");
		else
			pr_cont("single PE");
	}
	pr_info("\n");
}


void mv_pp3_hmac_rx_queue_show(int frame, int queue)
{
	pr_info("\n-------------- HMAC RX frame: #%d, queue: #%d --------------\n", frame, queue);
	if (mv_pp3_max_check(frame, MV_PP3_HFRM_NUM, "HMAC frame"))
		return;
	if (mv_pp3_max_check(queue, MV_PP3_HFRM_Q_NUM, "HMAC queue"))
		return;

	mv_pp3_hmac_rxq_status_show(frame, queue);

	if (!mv_hmac_rxq_handle[frame][queue])
		return;

	mv_pp3_hmac_rxq_queue_show(frame, queue);
	mv_pp3_hmac_rxq_bp_show(frame, queue);
	mv_pp3_hmac_rxq_coal_show(frame, queue);
}

void mv_pp3_hmac_tx_queue_show(int frame, int queue)
{
	pr_info("\n-------------- HMAC TX frame: #%d, queue: #%d -------------\n", frame, queue);
	if (mv_pp3_max_check(frame, MV_PP3_HFRM_NUM, "HMAC frame"))
		return;
	if (mv_pp3_max_check(queue, MV_PP3_HFRM_Q_NUM, "HMAC queue"))
		return;

	mv_pp3_hmac_txq_status_show(frame, queue);

	if (!mv_hmac_txq_handle[frame][queue]) {
		pr_info("\n");
		return;
	}

	mv_pp3_hmac_txq_queue_show(frame, queue);
	pr_info("\n");
}

/* queue dump functions */
void mv_pp3_hmac_rx_queue_dump(int frame, int queue, bool mode)
{
	if (mv_pp3_max_check(frame, MV_PP3_HFRM_NUM, "HMAC frame"))
		return;
	if (mv_pp3_max_check(queue, MV_PP3_HFRM_Q_NUM, "HMAC queue"))
		return;

	if (!mv_hmac_rxq_handle[frame][queue])
		return;

	pr_info("\n-------------- HMAC RX frame: #%d, queue: #%d -----------", frame, queue);
	mv_pp3_hmac_queue_dump(mv_hmac_rxq_handle[frame][queue], mode);
}

void mv_pp3_hmac_tx_queue_dump(int frame, int queue, bool mode)
{
	if (mv_pp3_max_check(frame, MV_PP3_HFRM_NUM, "HMAC frame"))
		return;
	if (mv_pp3_max_check(queue, MV_PP3_HFRM_Q_NUM, "HMAC queue"))
		return;

	if (!mv_hmac_txq_handle[frame][queue])
		return;

	pr_info("\n-------------- HMAC TX frame: #%d, queue: #%d -----------", frame, queue);
	mv_pp3_hmac_queue_dump(mv_hmac_txq_handle[frame][queue], mode);
}

/* debug functions */
void mv_pp3_hmac_debug_cfg(int flags)
{
	mv_pp3_hmac_debug_flags = flags;
}

void mv_pp3_hmac_txq_delete(int frame, int queue)
{
	if (!mv_hmac_txq_handle[frame][queue])
		return;

	kfree(mv_hmac_txq_handle[frame][queue]->first);
	kfree(mv_hmac_txq_handle[frame][queue]);

	mv_hmac_txq_handle[frame][queue] = NULL;
}

void mv_pp3_hmac_rxq_delete(int frame, int queue)
{
	if (!mv_hmac_rxq_handle[frame][queue])
		return;

	kfree(mv_hmac_rxq_handle[frame][queue]->first);
	kfree(mv_hmac_rxq_handle[frame][queue]);

	mv_hmac_rxq_handle[frame][queue] = NULL;
}

