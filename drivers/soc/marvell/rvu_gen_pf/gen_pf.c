// SPDX-License-Identifier: GPL-2.0
/* Marvell Octeon RVU Generic Physical Function driver
 *
 * Copyright (C) 2024 Marvell.
 *
 */
#include <linux/delay.h>
#include <linux/interrupt.h>
#include <linux/irq.h>
#include <linux/list.h>
#include <linux/module.h>
#include <linux/pci.h>
#include <linux/sysfs.h>

#include "gen_pf.h"
#include <rvu_trace.h>
#include <rvu.h>
#include <rvu_eb_sdp.h>

 /* PCI BAR nos */
#define PCI_CFG_REG_BAR_NUM		2
#define PCI_MBOX_BAR_NUM		4

#define DRV_NAME    "rvu_generic_pf"

/* Supported devices */
static const struct pci_device_id rvu_gen_pf_id_table[] = {
	{ PCI_DEVICE(PCI_VENDOR_ID_CAVIUM, PCI_DEVID_OCTEONTX2_GEN_PF) },
	{ }  /* end of table */
};
MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("Marvell Octeon RVU Generic PF Driver");
MODULE_DEVICE_TABLE(pci, rvu_gen_pf_id_table);

static void gen_pf_vf_task(struct work_struct *work);

inline int rvu_get_pf(u16 pcifunc)
{
	return (pcifunc >> RVU_PFVF_PF_SHIFT) & RVU_PFVF_PF_MASK;
}

static int rvu_gen_pf_check_pf_usable(struct gen_pf_dev *pfdev)
{
	u64 rev;

	if (is_cn20k(pfdev->pdev)) {
		rev = readq(pfdev->reg_base + RVU_PF_DISC);
		rev = FIELD_GET(BIT_ULL(BLKADDR_RVUM), rev);
	} else {
		rev = readq(pfdev->reg_base + RVU_PF_BLOCK_ADDRX_DISC(BLKADDR_RVUM));
		rev = FIELD_GET(GENMASK(19, 12), rev);
	}
	/* Check if AF has setup revision for RVUM block,
	 * otherwise this driver probe should be deferred
	 * until AF driver comes up.
	 */
	if (!rev) {
		dev_warn(pfdev->dev,
			 "AF is not initialized, deferring probe\n");
		return -EPROBE_DEFER;
	}
	return 0;
}

static void rvu_gen_pf_forward_msg_pfvf(struct otx2_mbox_dev *mdev,
					struct otx2_mbox *pfvf_mbox, void *bbuf_base,
					int devid)
{
	struct otx2_mbox_dev *src_mdev = mdev;
	int offset;

	/* Msgs are already copied, trigger VF's mbox irq */
	smp_wmb();

	otx2_mbox_wait_for_zero(pfvf_mbox, devid);
	offset = pfvf_mbox->trigger | (devid << pfvf_mbox->tr_shift);
	writeq(MBOX_DOWN_MSG, (void __iomem *)pfvf_mbox->reg_base + offset);

	/* Restore VF's mbox bounce buffer region address */
	src_mdev->mbase = bbuf_base;
}

static int rvu_gen_pf_forward_vf_mbox_msgs(struct gen_pf_dev *pfdev,
					   struct otx2_mbox *src_mbox,
					   int dir, int vf, int num_msgs)
{
	struct otx2_mbox_dev *src_mdev, *dst_mdev;
	struct mbox_hdr *mbox_hdr;
	struct mbox_hdr *req_hdr;
	struct mbox *dst_mbox;
	int dst_size, err;

	if (dir == MBOX_DIR_PFAF) {
		/*
		 * Set VF's mailbox memory as PF's bounce buffer memory, so
		 * that explicit copying of VF's msgs to PF=>AF mbox region
		 * and AF=>PF responses to VF's mbox region can be avoided.
		 */
		src_mdev = &src_mbox->dev[vf];
		mbox_hdr = src_mbox->hwbase +
				src_mbox->rx_start + (vf * MBOX_SIZE);
		dst_mbox = &pfdev->mbox;
		dst_size = dst_mbox->mbox.tx_size -
				ALIGN(sizeof(*mbox_hdr), MBOX_MSG_ALIGN);
		/* Check if msgs fit into destination area and has valid size */
		if (mbox_hdr->msg_size > dst_size || !mbox_hdr->msg_size)
			return -EINVAL;

		dst_mdev = &dst_mbox->mbox.dev[0];

		mutex_lock(&pfdev->mbox.lock);
		dst_mdev->mbase = src_mdev->mbase;
		dst_mdev->msg_size = mbox_hdr->msg_size;
		dst_mdev->num_msgs = num_msgs;
		err = rvu_gen_pf_sync_mbox_msg(dst_mbox);
		/*
		 * Error code -EIO indicate there is a communication failure
		 * to the AF. Rest of the error codes indicate that AF processed
		 * VF messages and set the error codes in response messages
		 * (if any) so simply forward responses to VF.
		 */
		if (err == -EIO) {
			dev_warn(pfdev->dev,
				 "AF not responding to VF%d messages\n", vf);
			/* restore PF mbase and exit */
			dst_mdev->mbase = pfdev->mbox.bbuf_base;
			mutex_unlock(&pfdev->mbox.lock);
			return err;
		}
		/*
		 * At this point, all the VF messages sent to AF are acked
		 * with proper responses and responses are copied to VF
		 * mailbox hence raise interrupt to VF.
		 */
		req_hdr = (struct mbox_hdr *)(dst_mdev->mbase +
					      dst_mbox->mbox.rx_start);
		req_hdr->num_msgs = num_msgs;

		rvu_gen_pf_forward_msg_pfvf(dst_mdev, &pfdev->mbox_pfvf[0].mbox,
					    pfdev->mbox.bbuf_base, vf);
		mutex_unlock(&pfdev->mbox.lock);
	} else if (dir == MBOX_DIR_PFVF_UP) {
		src_mdev = &src_mbox->dev[0];
		mbox_hdr = src_mbox->hwbase + src_mbox->rx_start;
		req_hdr = (struct mbox_hdr *)(src_mdev->mbase +
					      src_mbox->rx_start);
		req_hdr->num_msgs = num_msgs;

		dst_mbox = &pfdev->mbox_pfvf[0];
		dst_size = dst_mbox->mbox_up.tx_size -
				ALIGN(sizeof(*mbox_hdr), MBOX_MSG_ALIGN);
		/* Check if msgs fit into destination area */
		if (mbox_hdr->msg_size > dst_size)
			return -EINVAL;
		dst_mdev = &dst_mbox->mbox_up.dev[vf];
		dst_mdev->mbase = src_mdev->mbase;
		dst_mdev->msg_size = mbox_hdr->msg_size;
		dst_mdev->num_msgs = mbox_hdr->num_msgs;
		err = rvu_gen_pf_sync_mbox_up_msg(dst_mbox, vf);
		if (err) {
			dev_warn(pfdev->dev,
				 "VF%d is not responding to mailbox\n", vf);
			return err;
		}
	} else if (dir == MBOX_DIR_VFPF_UP) {
		req_hdr = (struct mbox_hdr *)(src_mbox->dev[0].mbase +
					      src_mbox->rx_start);
		req_hdr->num_msgs = num_msgs;
		rvu_gen_pf_forward_msg_pfvf(&pfdev->mbox_pfvf->mbox_up.dev[vf],
					    &pfdev->mbox.mbox_up,
					    pfdev->mbox_pfvf[vf].bbuf_base,
					    0);
	}

	return 0;
}

static irqreturn_t rvu_gen_pf_pfaf_mbox_intr_handler(int irq, void *pf_irq)
{
	struct gen_pf_dev *pfdev = (struct gen_pf_dev *)pf_irq;
	struct mbox *mw = &pfdev->mbox;
	struct otx2_mbox_dev *mdev;
	struct otx2_mbox *mbox;
	struct mbox_hdr *hdr;
	u64 mbox_data;

	/* Clear the IRQ */
	writeq(BIT_ULL(0), pfdev->reg_base + RVU_PF_INT);

	mbox_data = readq(pfdev->reg_base + RVU_PF_PFAF_MBOX0);

	if (mbox_data & MBOX_UP_MSG) {
		mbox_data &= ~MBOX_UP_MSG;
		writeq(mbox_data, pfdev->reg_base + RVU_PF_PFAF_MBOX0);

		mbox = &mw->mbox_up;
		mdev = &mbox->dev[0];
		otx2_sync_mbox_bbuf(mbox, 0);

		hdr = (struct mbox_hdr *)(mdev->mbase + mbox->rx_start);
		if (hdr->num_msgs)
			queue_work(pfdev->mbox_wq, &mw->mbox_up_wrk);

		trace_otx2_msg_interrupt(pfdev->pdev, "UP message from AF to PF",
					 BIT_ULL(0));
	}

	if (mbox_data & MBOX_DOWN_MSG) {
		mbox_data &= ~MBOX_DOWN_MSG;
		writeq(mbox_data, pfdev->reg_base + RVU_PF_PFAF_MBOX0);

		mbox = &mw->mbox;
		mdev = &mbox->dev[0];
		otx2_sync_mbox_bbuf(mbox, 0);

		hdr = (struct mbox_hdr *)(mdev->mbase + mbox->rx_start);
		if (hdr->num_msgs)
			queue_work(pfdev->mbox_wq, &mw->mbox_wrk);

		trace_otx2_msg_interrupt(pfdev->pdev, "DOWN reply from AF to PF",
					 BIT_ULL(0));
	}
	return IRQ_HANDLED;
}

/* CN20K mbox AF <==> PF irq handler */
static irqreturn_t rvu_gen_pf_cn20k_pfaf_mbox_intr_handler(int irq, void *pf_irq)
{
	struct gen_pf_dev *pfdev = (struct gen_pf_dev *)pf_irq;
	struct mbox *mw = &pfdev->mbox;
	struct otx2_mbox_dev *mdev;
	struct otx2_mbox *mbox;
	struct mbox_hdr *hdr;
	u64 intr;

	/* Read the interrupt bits */
	intr = readq(pfdev->reg_base + RVU_PF_INT);
	intr &= 0x3;

	/* Clear and ack the interrupt */
	writeq(intr, pfdev->reg_base + RVU_PF_INT);

	if (intr & BIT_ULL(0)) {
		mbox = &mw->mbox_up;
		mdev = &mbox->dev[0];
		otx2_sync_mbox_bbuf(mbox, 0);

		hdr = (struct mbox_hdr *)(mdev->mbase + mbox->rx_start);
		if (hdr->num_msgs)
			queue_work(pfdev->mbox_wq, &mw->mbox_up_wrk);
	}

	if (intr & BIT_ULL(1)) {
		mbox = &mw->mbox;
		mdev = &mbox->dev[0];
		otx2_sync_mbox_bbuf(mbox, 0);

		hdr = (struct mbox_hdr *)(mdev->mbase + mbox->rx_start);
		if (hdr->num_msgs)
			queue_work(pfdev->mbox_wq, &mw->mbox_wrk);
	}
	return IRQ_HANDLED;
}

static void rvu_gen_pf_disable_mbox_intr(struct gen_pf_dev *pfdev)
{
	int vector;

	/* Disable AF => PF mailbox IRQ */
	if (!is_cn20k(pfdev->pdev)) {
		vector = pci_irq_vector(pfdev->pdev, RVU_PF_INT_VEC_AFPF_MBOX);
		writeq(BIT_ULL(0), pfdev->reg_base + RVU_PF_INT_ENA_W1C);
	} else {
		vector = pci_irq_vector(pfdev->pdev,
					RVU_MBOX_PF_INT_VEC_AFPF_MBOX);
		writeq(BIT_ULL(0) | BIT_ULL(1), pfdev->reg_base + RVU_PF_INT_ENA_W1C);
	}
	free_irq(vector, pfdev);
}

static int rvu_gen_pf_register_mbox_intr(struct gen_pf_dev *pfdev)
{
	struct msg_req *req;
	char *irq_name;
	int err;

	/* Register mailbox interrupt handler */
	if (!is_cn20k(pfdev->pdev)) {
		irq_name = &pfdev->irq_name[RVU_PF_INT_VEC_AFPF_MBOX * NAME_SIZE];
		snprintf(irq_name, NAME_SIZE, "GENPF%d AFPF Mbox",
			 rvu_get_pf(pfdev->pcifunc));
		err = request_irq(pci_irq_vector(pfdev->pdev, RVU_PF_INT_VEC_AFPF_MBOX),
				  rvu_gen_pf_pfaf_mbox_intr_handler, 0, irq_name, pfdev);
	} else {
		irq_name = &pfdev->irq_name[RVU_MBOX_PF_INT_VEC_AFPF_MBOX *
						NAME_SIZE];
		snprintf(irq_name, NAME_SIZE, "GENPF%d AFPF Mbox",
			 rvu_get_pf(pfdev->pcifunc));
		err = request_irq(pci_irq_vector(pfdev->pdev, RVU_MBOX_PF_INT_VEC_AFPF_MBOX),
				  rvu_gen_pf_cn20k_pfaf_mbox_intr_handler, 0, irq_name, pfdev);
	}
	if (err) {
		dev_err(pfdev->dev,
			"GenPF: IRQ registration failed for PFAF mbox irq\n");
		return err;
	}

	/* Enable mailbox interrupt for msgs coming from AF.
	 * First clear to avoid spurious interrupts, if any.
	 */
	if (!is_cn20k(pfdev->pdev)) {
		writeq(BIT_ULL(0), pfdev->reg_base + RVU_PF_INT);
		writeq(BIT_ULL(0), pfdev->reg_base + RVU_PF_INT_ENA_W1S);
	} else {
		writeq(BIT_ULL(0) | BIT_ULL(1), pfdev->reg_base + RVU_PF_INT);
		writeq(BIT_ULL(0) | BIT_ULL(1), pfdev->reg_base + RVU_PF_INT_ENA_W1S);
	}

	/* Check mailbox communication with AF */
	req = gen_pf_mbox_alloc_msg_ready(&pfdev->mbox);
	if (!req) {
		rvu_gen_pf_disable_mbox_intr(pfdev);
		return -ENOMEM;
	}
	err = rvu_gen_pf_sync_mbox_msg(&pfdev->mbox);
	if (err) {
		dev_warn(pfdev->dev,
			 "AF not responding to mailbox, deferring probe\n");
		rvu_gen_pf_disable_mbox_intr(pfdev);
		return -EPROBE_DEFER;
	}
	return 0;
}

static void rvu_gen_pf_pfaf_mbox_destroy(struct gen_pf_dev *pfdev)
{
	struct mbox *mbox = &pfdev->mbox;

	if (pfdev->mbox_wq) {
		destroy_workqueue(pfdev->mbox_wq);
		pfdev->mbox_wq = NULL;
	}

	if (mbox->mbox.hwbase)
		iounmap((void __iomem *)mbox->mbox.hwbase);

	otx2_mbox_destroy(&mbox->mbox);
	otx2_mbox_destroy(&mbox->mbox_up);
}

static void rvu_gen_pf_process_pfaf_mbox_msg(struct gen_pf_dev *pfdev,
					     struct mbox_msghdr *msg)
{
	if (msg->id >= MBOX_MSG_MAX) {
		dev_err(pfdev->dev,
			"Mbox msg with unknown ID 0x%x\n", msg->id);
		return;
	}

	if (msg->sig != OTX2_MBOX_RSP_SIG) {
		dev_err(pfdev->dev,
			"Mbox msg with wrong signature %x, ID 0x%x\n",
			 msg->sig, msg->id);
		return;
	}

	switch (msg->id) {
	case MBOX_MSG_READY:
		pfdev->pcifunc = msg->pcifunc;
		break;
	default:
		if (msg->rc)
			dev_err(pfdev->dev,
				"Mbox msg response has err %d, ID 0x%x\n",
				msg->rc, msg->id);
		break;
	}
}

static void gen_pf_vf_task(struct work_struct *work)
{
	struct gen_pf_vf_config *config;
	struct mbox_msghdr *msghdr;
	struct delayed_work *dwork;
	struct gen_pf_dev *pf;
	int vf_idx;

	config = container_of(work, struct gen_pf_vf_config, vf_work.work);
	vf_idx = config - config->pf->vf_configs;
	pf = config->pf;

	mutex_lock(&pf->mbox.lock);

	dwork = &config->vf_work;

	if (!otx2_mbox_wait_for_zero(&pf->mbox_pfvf[0].mbox_up, vf_idx)) {
		schedule_delayed_work(dwork, msecs_to_jiffies(100));
		mutex_unlock(&pf->mbox.lock);
		return;
	}

	memcpy(msghdr, config->cfg_buff, sizeof(*msghdr));

	switch (msghdr->id) {
#define M(_name, _id, _fn_name, _req_type, _rsp_type)			\
	case _id: {							\
		struct _req_type *req;					\
									\
		msghdr = otx2_mbox_alloc_msg_rsp(&pf->mbox_pfvf[0].mbox_up, \
						 vf_idx, sizeof(*req),	\
						 sizeof(struct msg_rsp)); \
		if (!msghdr) {						\
			dev_err(pf->dev,				\
				"Failed to create VF%d creation event\n", \
				vf_idx);				\
			mutex_unlock(&pf->mbox.lock);			\
			return;						\
		}							\
									\
		WARN_ON(sizeof(*req) > sizeof(config->cfg_buff));	\
		memcpy(msghdr, config->cfg_buff, sizeof(*req));		\
									\
		req = (struct _req_type *)msghdr;			\
		req->hdr.id = _id;					\
		req->hdr.sig = OTX2_MBOX_REQ_SIG;			\
		req->hdr.pcifunc = pf->pcifunc;				\
	}								\
		break;
MBOX_EBLOCK_UP_SDP_MESSAGES
#undef M
	default:
		dev_err(pf->dev, "Invalid VF UP message ID %d\n", msghdr->id);
		mutex_unlock(&pf->mbox.lock);
		return;
	}

	otx2_mbox_wait_for_zero(&pf->mbox_pfvf[0].mbox_up, vf_idx);

	rvu_gen_pf_sync_mbox_up_msg(&pf->mbox_pfvf[0], vf_idx);

	mutex_unlock(&pf->mbox.lock);
}

static int rvu_gen_pf_process_mbox_msg_up(struct gen_pf_dev *pf,
					  struct mbox_msghdr *msg)
{
	struct gen_pf_vf_config *config;
	struct delayed_work *dwork;
	int vf;

	/* Check if valid, if not reply with a invalid msg */
	if (msg->sig != OTX2_MBOX_REQ_SIG) {
		otx2_reply_invalid_msg(&pf->mbox.mbox_up, 0, 0, msg->id);
		return -ENODEV;
	}

	switch (msg->id) {
#define M(_name, _id, _fn_name, _req_type, _rsp_type)			\
	case _id: {							\
		struct _req_type *req;					\
		struct _rsp_type *rsp;					\
									\
		rsp = (struct _rsp_type *)otx2_mbox_alloc_msg(		\
			&pf->mbox.mbox_up, 0,				\
			sizeof(struct _rsp_type));			\
		if (!rsp)						\
			return -ENOMEM;					\
									\
		rsp->hdr.id = _id;					\
		rsp->hdr.sig = OTX2_MBOX_RSP_SIG;			\
		rsp->hdr.pcifunc = pf->pcifunc;				\
		rsp->hdr.rc = 0;					\
									\
		for_each_set_bit(vf, &req->vf_bmap1,			\
				 sizeof(unsigned long) * BITS_PER_BYTE) { \
			config = &pf->vf_configs[vf];			\
			dwork = &config->vf_work;			\
									\
			WARN_ON(sizeof(req) > sizeof(config->cfg_buff)); \
			memcpy(config->cfg_buff, req, sizeof(*req));	\
									\
			schedule_delayed_work(dwork, msecs_to_jiffies(100)); \
		}							\
									\
		for_each_set_bit(vf, &req->vf_bmap2,			\
				 sizeof(unsigned long) * BITS_PER_BYTE) { \
			config = &pf->vf_configs[vf + 64];		\
			dwork = &config->vf_work;			\
									\
			WARN_ON(sizeof(*req) > sizeof(config->cfg_buff)); \
			memcpy(config->cfg_buff, req, sizeof(*req));	\
									\
			schedule_delayed_work(dwork, msecs_to_jiffies(100)); \
		}							\
									\
		return 0;						\
	}
MBOX_EBLOCK_UP_SDP_MESSAGES
#undef M
		break;
	default:
		otx2_reply_invalid_msg(&pf->mbox.mbox_up, 0, 0, msg->id);
		return -ENODEV;
	}

	return 0;
}

static void rvu_gen_pf_pfaf_mbox_up_handler(struct work_struct *work)
{
	struct mbox *af_mbox = container_of(work, struct mbox, mbox_up_wrk);
	struct otx2_mbox *mbox = &af_mbox->mbox_up;
	struct otx2_mbox_dev *mdev = &mbox->dev[0];
	struct gen_pf_dev *pfdev = af_mbox->pfvf;
	int offset, id, devid = 0;
	struct mbox_hdr *rsp_hdr;
	struct mbox_msghdr *msg;
	u16 num_msgs;

	rsp_hdr = (struct mbox_hdr *)(mdev->mbase + mbox->rx_start);
	num_msgs = rsp_hdr->num_msgs;

	offset = mbox->rx_start + ALIGN(sizeof(*rsp_hdr), MBOX_MSG_ALIGN);

	for (id = 0; id < num_msgs; id++) {
		msg = (struct mbox_msghdr *)(mdev->mbase + offset);

		devid = msg->pcifunc & RVU_PFVF_FUNC_MASK;
		/* Skip processing VF's messages */
		if (!devid)
			rvu_gen_pf_process_mbox_msg_up(pfdev, msg);
		offset = mbox->rx_start + msg->next_msgoff;
	}
	/* Forward to VF iff VFs are really present */
	if (devid && pci_num_vf(pfdev->pdev)) {
		rvu_gen_pf_forward_vf_mbox_msgs(pfdev, &pfdev->mbox.mbox_up,
						MBOX_DIR_PFVF_UP, devid - 1,
						num_msgs);
		return;
	}

	otx2_mbox_msg_send(mbox, 0);
}

static void rvu_gen_pf_pfaf_mbox_handler(struct work_struct *work)
{
	struct otx2_mbox_dev *mdev;
	struct gen_pf_dev *pfdev;
	struct mbox_hdr *rsp_hdr;
	struct mbox_msghdr *msg;
	struct otx2_mbox *mbox;
	struct mbox *af_mbox;
	int offset, id;
	u16 num_msgs;

	af_mbox = container_of(work, struct mbox, mbox_wrk);
	mbox = &af_mbox->mbox;
	mdev = &mbox->dev[0];
	rsp_hdr = (struct mbox_hdr *)(mdev->mbase + mbox->rx_start);
	num_msgs = rsp_hdr->num_msgs;

	offset = mbox->rx_start + ALIGN(sizeof(*rsp_hdr), MBOX_MSG_ALIGN);
	pfdev = af_mbox->pfvf;

	for (id = 0; id < num_msgs; id++) {
		msg = (struct mbox_msghdr *)(mdev->mbase + offset);
		rvu_gen_pf_process_pfaf_mbox_msg(pfdev, msg);
		offset = mbox->rx_start + msg->next_msgoff;
		if (mdev->msgs_acked == (num_msgs - 1))
			__otx2_mbox_reset(mbox, 0);
		mdev->msgs_acked++;
	}
}

static int rvu_gen_pf_pfaf_mbox_init(struct gen_pf_dev *pfdev)
{
	struct mbox *mbox = &pfdev->mbox;
	void __iomem *hwbase;
	int err;

	mbox->pfvf = pfdev;
	pfdev->mbox_wq = alloc_ordered_workqueue("otx2_pfaf_mailbox",
						 WQ_HIGHPRI | WQ_MEM_RECLAIM);

	if (!pfdev->mbox_wq)
		return -ENOMEM;

	if (is_cn20k(pfdev->pdev)) {
		hwbase = pfdev->reg_base + GEN_PF_CN20K_PFAF_MBOX_BASE +
			 ((u64)BLKADDR_MBOX << OTX2_GEN_PF_FUNC_BLKADDR_SHIFT);
	} else {
		hwbase = ioremap_wc(pci_resource_start(pfdev->pdev, PCI_MBOX_BAR_NUM),
				    MBOX_SIZE);
		if (!hwbase) {
			dev_err(pfdev->dev, "Unable to map BAR4\n");
			err = -ENOMEM;
			goto exit;
		}
	}

	/* Mailbox is a reserved memory (in RAM) region shared between
	 * admin function (i.e AF) and this PF, shouldn't be mapped as
	 * device memory to allow unaligned accesses.
	 */

	err = otx2_mbox_init(&mbox->mbox, hwbase, pfdev->pdev, pfdev->reg_base,
			     MBOX_DIR_PFAF, 1);
	if (err)
		goto exit;

	err = otx2_mbox_init(&mbox->mbox_up, hwbase, pfdev->pdev, pfdev->reg_base,
			     MBOX_DIR_PFAF_UP, 1);

	if (err)
		goto exit;

	err = otx2_mbox_bbuf_init(mbox, pfdev->pdev);
	if (err)
		goto exit;

	INIT_WORK(&mbox->mbox_wrk, rvu_gen_pf_pfaf_mbox_handler);
	INIT_WORK(&mbox->mbox_up_wrk, rvu_gen_pf_pfaf_mbox_up_handler);
	mutex_init(&mbox->lock);

	return 0;
exit:
	rvu_gen_pf_pfaf_mbox_destroy(pfdev);
	return err;
}

static void rvu_gen_pf_pfvf_mbox_handler(struct work_struct *work)
{
	struct mbox_msghdr *msg = NULL;
	int offset, vf_idx, id, err;
	struct otx2_mbox_dev *mdev;
	struct gen_pf_dev *pfdev;
	struct mbox_hdr *req_hdr;
	struct otx2_mbox *mbox;
	struct mbox *vf_mbox;

	vf_mbox = container_of(work, struct mbox, mbox_wrk);
	pfdev = vf_mbox->pfvf;
	vf_idx = vf_mbox - pfdev->mbox_pfvf;

	mbox = &pfdev->mbox_pfvf[0].mbox;
	mdev = &mbox->dev[vf_idx];
	req_hdr = (struct mbox_hdr *)(mdev->mbase + mbox->rx_start);

	offset = ALIGN(sizeof(*req_hdr), MBOX_MSG_ALIGN);

	for (id = 0; id < vf_mbox->num_msgs; id++) {
		msg = (struct mbox_msghdr *)(mdev->mbase + mbox->rx_start +
					     offset);

		if (msg->sig != OTX2_MBOX_REQ_SIG)
			goto inval_msg;

		/* Set VF's number in each of the msg */
		msg->pcifunc &= ~RVU_PFVF_FUNC_MASK;
		msg->pcifunc |= (vf_idx + 1) & RVU_PFVF_FUNC_MASK;
		offset = msg->next_msgoff;
	}
	err = rvu_gen_pf_forward_vf_mbox_msgs(pfdev, mbox, MBOX_DIR_PFAF, vf_idx,
					      vf_mbox->num_msgs);
	if (err)
		goto inval_msg;
	return;

inval_msg:
	if (!msg)
		return;

	otx2_reply_invalid_msg(mbox, vf_idx, 0, msg->id);
	otx2_mbox_msg_send(mbox, vf_idx);
}

static void *rvu_gen_pf_cn20k_pfvf_mbox_alloc(struct gen_pf_dev *pfdev, int numvfs)
{
	struct qmem *mbox_addr;
	int err;

	err = qmem_alloc(&pfdev->pdev->dev, &mbox_addr, numvfs, MBOX_SIZE);
	if (err) {
		dev_err(pfdev->dev, "qmem alloc fail\n");
		return ERR_PTR(-ENOMEM);
	}

	writeq((u64)mbox_addr->iova, pfdev->reg_base + RVU_PF_VF_MBOX_ADDR);
	pfdev->pfvf_mbox_addr = mbox_addr;

	return mbox_addr->base;
}

static int rvu_gen_pf_pfvf_mbox_init(struct gen_pf_dev *pfdev, int numvfs)
{
	void __iomem *hwbase;
	struct mbox *mbox;
	int err, vf;
	u64 base;

	if (!numvfs)
		return -EINVAL;

	pfdev->mbox_pfvf = devm_kcalloc(&pfdev->pdev->dev, numvfs,
					sizeof(struct mbox), GFP_KERNEL);

	if (!pfdev->mbox_pfvf)
		return -ENOMEM;

	pfdev->mbox_pfvf_wq = alloc_workqueue("otx2_pfvf_mailbox",
					      WQ_UNBOUND | WQ_HIGHPRI |
					      WQ_MEM_RECLAIM, 0);
	if (!pfdev->mbox_pfvf_wq)
		return -ENOMEM;

	/* For CN20K, PF allocates mbox memory in DRAM and writes PF/VF
	 * regions/offsets in RVU_PF_VF_MBOX_ADDR, the RVU_PFX_FUNC_PFAF_MBOX
	 * gives the aliased address to access PF/VF mailbox regions.
	 */

	if (is_cn20k(pfdev->pdev)) {
		hwbase = rvu_gen_pf_cn20k_pfvf_mbox_alloc(pfdev, numvfs);
	} else {
	/* PF <-> VF mailbox region follows after
	 * PF <-> AF mailbox region.
	 */
		if (test_bit(CN10K_MBOX, &pfdev->hw.cap_flag))
			base = pci_resource_start(pfdev->pdev, PCI_MBOX_BAR_NUM) +
						  MBOX_SIZE;
		else
			base = readq((void __iomem *)((u64)pfdev->reg_base +
					      RVU_PF_VF_BAR4_ADDR));

		hwbase = ioremap_wc(base, MBOX_SIZE * pfdev->total_vfs);
		if (!hwbase) {
			err = -ENOMEM;
			goto free_wq;
		}
	}

	mbox = &pfdev->mbox_pfvf[0];
	err = otx2_mbox_init(&mbox->mbox, hwbase, pfdev->pdev, pfdev->reg_base,
			     MBOX_DIR_PFVF, numvfs);
	if (err)
		goto free_iomem;

	err = otx2_mbox_init(&mbox->mbox_up, hwbase, pfdev->pdev, pfdev->reg_base,
			     MBOX_DIR_PFVF_UP, numvfs);
	if (err)
		goto free_iomem;

	for (vf = 0; vf < numvfs; vf++) {
		mbox->pfvf = pfdev;
		INIT_WORK(&mbox->mbox_wrk, rvu_gen_pf_pfvf_mbox_handler);
		mbox++;
	}

	return 0;

free_iomem:
	if (hwbase && !(is_cn20k(pfdev->pdev)))
		iounmap(hwbase);
free_wq:
	destroy_workqueue(pfdev->mbox_pfvf_wq);
	return err;
}

static void rvu_gen_pf_pfvf_mbox_destroy(struct gen_pf_dev *pfdev)
{
	struct mbox *mbox = &pfdev->mbox_pfvf[0];

	if (!mbox)
		return;

	if (pfdev->mbox_pfvf_wq) {
		destroy_workqueue(pfdev->mbox_pfvf_wq);
		pfdev->mbox_pfvf_wq = NULL;
	}

	if (mbox->mbox.hwbase && is_cn20k(pfdev->pdev))
		iounmap((void __iomem *)mbox->mbox.hwbase);
	else
		qmem_free(&pfdev->pdev->dev, pfdev->pfvf_mbox_addr);

	otx2_mbox_destroy(&mbox->mbox);
}

static void rvu_gen_pf_enable_pfvf_mbox_intr(struct gen_pf_dev *pfdev, int numvfs)
{
	/* Clear PF <=> VF mailbox IRQ */
	writeq(~0ull, pfdev->reg_base + RVU_PF_VFPF_MBOX_INTX(0));
	writeq(~0ull, pfdev->reg_base + RVU_PF_VFPF_MBOX_INTX(1));

	/* Enable PF <=> VF mailbox IRQ */
	writeq(INTR_MASK(numvfs), pfdev->reg_base + RVU_PF_VFPF_MBOX_INT_ENA_W1SX(0));
	if (numvfs > 64) {
		numvfs -= 64;
		writeq(INTR_MASK(numvfs), pfdev->reg_base + RVU_PF_VFPF_MBOX_INT_ENA_W1SX(1));
	}
}

void rvu_gen_pf_cn20k_disable_pfvf_mbox_intr(struct gen_pf_dev *pfdev, int numvfs)
{
	int vector = 0, intr_vec; // vec = 0;

	/* Disable PF <=> VF mailbox IRQ */
	writeq(~0ull, pfdev->reg_base + RVU_MBOX_PF_VFPF_INT_ENA_W1CX(0));
	writeq(~0ull, pfdev->reg_base + RVU_MBOX_PF_VFPF_INT_ENA_W1CX(1));
	writeq(~0ull, pfdev->reg_base + RVU_MBOX_PF_VFPF1_INT_ENA_W1CX(0));
	writeq(~0ull, pfdev->reg_base + RVU_MBOX_PF_VFPF1_INT_ENA_W1CX(1));

	writeq(~0ull, pfdev->reg_base + RVU_MBOX_PF_VFPF_INTX(0));
	writeq(~0ull, pfdev->reg_base + RVU_MBOX_PF_VFPF1_INTX(0));

	if (numvfs > 64) {
		writeq(~0ull, pfdev->reg_base + RVU_MBOX_PF_VFPF_INTX(1));
		writeq(~0ull, pfdev->reg_base + RVU_MBOX_PF_VFPF1_INTX(1));
	}

	for (intr_vec = RVU_MBOX_PF_INT_VEC_VFPF_MBOX0; intr_vec <=
			RVU_MBOX_PF_INT_VEC_VFPF1_MBOX1; intr_vec++) {
		free_irq(pci_irq_vector(pfdev->pdev, intr_vec),
			 &pfdev->irq_data[vector]);
		vector++;
	}
}

static void rvu_gen_pf_disable_pfvf_mbox_intr(struct gen_pf_dev *pfdev, int numvfs)
{
	int vector;

	if (is_cn20k(pfdev->pdev))
		return rvu_gen_pf_cn20k_disable_pfvf_mbox_intr(pfdev, numvfs);

	/* Disable PF <=> VF mailbox IRQ */
	writeq(~0ull, pfdev->reg_base + RVU_PF_VFPF_MBOX_INT_ENA_W1CX(0));
	writeq(~0ull, pfdev->reg_base + RVU_PF_VFPF_MBOX_INT_ENA_W1CX(1));

	writeq(~0ull, pfdev->reg_base + RVU_PF_VFPF_MBOX_INTX(0));
	vector = pci_irq_vector(pfdev->pdev, RVU_PF_INT_VEC_VFPF_MBOX0);
	free_irq(vector, pfdev);

	if (numvfs > 64) {
		writeq(~0ull, pfdev->reg_base + RVU_PF_VFPF_MBOX_INTX(1));
		vector = pci_irq_vector(pfdev->pdev, RVU_PF_INT_VEC_VFPF_MBOX1);
		free_irq(vector, pfdev);
	}
}

static void rvu_gen_pf_queue_vf_work(struct mbox *mw, struct workqueue_struct *mbox_wq,
				     int first, int mdevs, u64 intr)
{
	struct otx2_mbox_dev *mdev;
	struct otx2_mbox *mbox;
	struct mbox_hdr *hdr;
	int i;

	for (i = first; i < mdevs; i++) {
		/* start from 0 */
		if (!(intr & BIT_ULL(i - first)))
			continue;

		mbox = &mw->mbox;
		mdev = &mbox->dev[i];
		hdr = mdev->mbase + mbox->rx_start;
		/*
		 * The hdr->num_msgs is set to zero immediately in the interrupt
		 * handler to ensure that it holds a correct value next time
		 * when the interrupt handler is called. pf->mw[i].num_msgs
		 * holds the data for use in otx2_pfvf_mbox_handler and
		 * pf->mw[i].up_num_msgs holds the data for use in
		 * otx2_pfvf_mbox_up_handler.
		 */
		if (hdr->num_msgs) {
			mw[i].num_msgs = hdr->num_msgs;
			hdr->num_msgs = 0;
			queue_work(mbox_wq, &mw[i].mbox_wrk);
		}

		mbox = &mw->mbox_up;
		mdev = &mbox->dev[i];
		hdr = mdev->mbase + mbox->rx_start;
		if (hdr->num_msgs) {
			mw[i].up_num_msgs = hdr->num_msgs;
			hdr->num_msgs = 0;
			queue_work(mbox_wq, &mw[i].mbox_up_wrk);
		}
	}
}

static void rvu_gen_pf_flr_wq_destroy(struct gen_pf_dev *pfdev)
{
	if (!pfdev->flr_wq)
		return;
	destroy_workqueue(pfdev->flr_wq);
	pfdev->flr_wq = NULL;
	devm_kfree(pfdev->dev, pfdev->flr_wrk);
}

void rvu_gen_pf_cn20k_enable_pfvf_mbox_intr(struct gen_pf_dev *pfdev, int numvfs)
{
	/* Clear PF <=> VF mailbox IRQ */
	writeq(~0ull, pfdev->reg_base + RVU_MBOX_PF_VFPF_INTX(0));
	writeq(~0ull, pfdev->reg_base + RVU_MBOX_PF_VFPF_INTX(1));
	writeq(~0ull, pfdev->reg_base + RVU_MBOX_PF_VFPF1_INTX(0));
	writeq(~0ull, pfdev->reg_base + RVU_MBOX_PF_VFPF1_INTX(1));

	/* Enable PF <=> VF mailbox IRQ */
	writeq(INTR_MASK(numvfs), pfdev->reg_base + RVU_MBOX_PF_VFPF_INT_ENA_W1SX(0));
	writeq(INTR_MASK(numvfs), pfdev->reg_base + RVU_MBOX_PF_VFPF1_INT_ENA_W1SX(0));
	if (numvfs > 64) {
		numvfs -= 64;
		writeq(INTR_MASK(numvfs), pfdev->reg_base + RVU_MBOX_PF_VFPF_INT_ENA_W1SX(1));
		writeq(INTR_MASK(numvfs), pfdev->reg_base + RVU_MBOX_PF_VFPF1_INT_ENA_W1SX(1));
	}
}

irqreturn_t rvu_gen_pf_cn20k_pfvf_mbox_intr(int irq, void *pf_irq)
{
	struct gen_pf_irq_data *irq_data = (struct gen_pf_irq_data *)(pf_irq);
	struct gen_pf_dev *pfdev = irq_data->pf;
	struct mbox *mbox;
	u64 intr;

	/* Sync with the mbox memory region */
	rmb();

	intr = readq(pfdev->reg_base + irq_data->intr_status);
	writeq(intr, pfdev->reg_base + irq_data->intr_status);
	mbox = pfdev->mbox_pfvf;

	if (intr)
		trace_otx2_msg_interrupt(pfdev->pdev, "VF(s) to PF", intr);

	irq_data->pf_queue_work_hdlr(mbox, pfdev->mbox_pfvf_wq, irq_data->start,
				     irq_data->mdevs, intr);

	return IRQ_HANDLED;
}

static irqreturn_t rvu_gen_pf_pfvf_mbox_intr_handler(int irq, void *pf_irq)
{
	struct gen_pf_dev *pfdev = (struct gen_pf_dev *)(pf_irq);
	int vfs = pfdev->total_vfs;
	struct mbox *mbox;
	u64 intr;

	mbox = pfdev->mbox_pfvf;
	/* Handle VF interrupts */
	if (vfs > 64) {
		intr = readq(pfdev->reg_base + RVU_PF_VFPF_MBOX_INTX(1));
		writeq(intr, pfdev->reg_base + RVU_PF_VFPF_MBOX_INTX(1));
		rvu_gen_pf_queue_vf_work(mbox, pfdev->mbox_pfvf_wq, 64, vfs, intr);
		if (intr)
			trace_otx2_msg_interrupt(mbox->mbox.pdev, "VF(s) to PF", intr);
		vfs = 64;
	}

	intr = readq(pfdev->reg_base + RVU_PF_VFPF_MBOX_INTX(0));
	writeq(intr, pfdev->reg_base + RVU_PF_VFPF_MBOX_INTX(0));

	rvu_gen_pf_queue_vf_work(mbox, pfdev->mbox_pfvf_wq, 0, vfs, intr);

	if (intr)
		trace_otx2_msg_interrupt(mbox->mbox.pdev, "VF(s) to PF", intr);

	return IRQ_HANDLED;
}

int rvu_gen_pf_cn20k_register_pfvf_mbox_intr(struct gen_pf_dev *pfdev,
					     int num_vfs)
{
	struct gen_pf_irq_data *irq_data = &pfdev->irq_data[0];
	int intr_vec, ret, vec = 0;
	char *irq_name;

	if (!irq_data)
		return -ENOMEM;

	for (intr_vec = RVU_MBOX_PF_INT_VEC_VFPF_MBOX0; intr_vec <=
			RVU_MBOX_PF_INT_VEC_VFPF1_MBOX1; intr_vec++, vec++) {
		switch (intr_vec) {
		case RVU_MBOX_PF_INT_VEC_VFPF_MBOX0:
			/* VF(0..63) Request messages(MBOX0) to PF */
			irq_data[vec].intr_status = RVU_MBOX_PF_VFPF_INTX(0);
			irq_data[vec].start = 0;
			irq_data[vec].mdevs = 64;
			break;
		case RVU_MBOX_PF_INT_VEC_VFPF_MBOX1:
			/* VF(64..127) Request messages(MBOX0) to PF */
			irq_data[vec].intr_status = RVU_MBOX_PF_VFPF_INTX(1);
			irq_data[vec].start = 64;
			irq_data[vec].mdevs = 128;
			break;
		case RVU_MBOX_PF_INT_VEC_VFPF1_MBOX0:
			/* PF ACK messages(MBOX1) to VF(0..63) */
			irq_data[vec].intr_status = RVU_MBOX_PF_VFPF1_INTX(0);
			irq_data[vec].start = 0;
			irq_data[vec].mdevs = 64;
			break;
		case RVU_MBOX_PF_INT_VEC_VFPF1_MBOX1:
			/* PF ACK messages(MBOX1) to VF(64..127) */
			irq_data[vec].intr_status = RVU_MBOX_PF_VFPF1_INTX(1);
			irq_data[vec].start = 64;
			irq_data[vec].mdevs = 128;
			break;
		}
		irq_data[vec].pf_queue_work_hdlr = rvu_gen_pf_queue_vf_work;
		irq_data[vec].vec_num = intr_vec;
		irq_data[vec].pf = pfdev;

		irq_name = &irq_data[vec].irq_name[0];
		snprintf(irq_name, GEN_PF_CN20K_PFVF_MBOX_IRQ_NAME,
			 "GENPF PFVF%d Mbox%d", (vec % 2), (vec / 2));
		ret = request_irq(pci_irq_vector(pfdev->pdev, intr_vec),
				  rvu_gen_pf_cn20k_pfvf_mbox_intr, 0,
				  irq_name, &irq_data[vec]);
		if (ret) {
			dev_err(&pfdev->pdev->dev,
				"IRQ registration fail for CPT PFVF mbox %d\n",
				vec);
			return ret;
		}
	}

	rvu_gen_pf_cn20k_enable_pfvf_mbox_intr(pfdev, num_vfs);
	return 0;
}

static int rvu_gen_pf_register_pfvf_mbox_intr(struct gen_pf_dev *pfdev, int numvfs)
{
	char *irq_name;
	int err;

	if (is_cn20k(pfdev->pdev))
		return rvu_gen_pf_cn20k_register_pfvf_mbox_intr(pfdev, numvfs);

	/* Register MBOX0 interrupt handler */
	irq_name = &pfdev->irq_name[RVU_PF_INT_VEC_VFPF_MBOX0 * NAME_SIZE];
	if (pfdev->pcifunc)
		snprintf(irq_name, NAME_SIZE,
			 "GENPF %d_VF Mbox0", rvu_get_pf(pfdev->pcifunc));
	else
		snprintf(irq_name, NAME_SIZE, "GENPF_PF_VF Mbox0");
	if (is_cn20k(pfdev->pdev)) {
		err = rvu_gen_pf_cn20k_register_pfvf_mbox_intr(pfdev, numvfs);
	} else {
		err = request_irq(pci_irq_vector(pfdev->pdev, RVU_PF_INT_VEC_VFPF_MBOX0),
				  rvu_gen_pf_pfvf_mbox_intr_handler,
				  0, irq_name, pfdev);
	}
	if (err) {
		dev_err(pfdev->dev,
			"RVUPF: IRQ registration failed for PFVF mbox0 irq\n");
		return err;
	}

	if (numvfs > 64) {
		/* Register MBOX1 interrupt handler */
		irq_name = &pfdev->irq_name[RVU_PF_INT_VEC_VFPF_MBOX1 * NAME_SIZE];
		if (pfdev->pcifunc)
			snprintf(irq_name, NAME_SIZE,
				 "Generic RVUPF%d_VF Mbox1", rvu_get_pf(pfdev->pcifunc));
		else
			snprintf(irq_name, NAME_SIZE, "Generic RVUPF_VF Mbox1");
		err = request_irq(pci_irq_vector(pfdev->pdev,
						 RVU_PF_INT_VEC_VFPF_MBOX1),
						 rvu_gen_pf_pfvf_mbox_intr_handler,
						 0, irq_name, pfdev);
		if (err) {
			dev_err(pfdev->dev,
				"RVUPF: IRQ registration failed for PFVF mbox1 irq\n");
			return err;
		}
	}

	rvu_gen_pf_enable_pfvf_mbox_intr(pfdev, numvfs);

	return 0;
}

static void rvu_gen_pf_flr_handler(struct work_struct *work)
{
	struct flr_work *flrwork = container_of(work, struct flr_work, work);
	struct gen_pf_dev *pfdev = flrwork->pfdev;
	struct mbox *mbox = &pfdev->mbox;
	struct msg_req *req;
	int vf, reg = 0;

	vf = flrwork - pfdev->flr_wrk;

	mutex_lock(&mbox->lock);
	req = gen_pf_mbox_alloc_msg_vf_flr(mbox);
	if (!req) {
		mutex_unlock(&mbox->lock);
		return;
	}
	req->hdr.pcifunc &= ~RVU_PFVF_FUNC_MASK;
	req->hdr.pcifunc |= (vf + 1) & RVU_PFVF_FUNC_MASK;

	if (!rvu_gen_pf_sync_mbox_msg(&pfdev->mbox)) {
		if (vf >= 64) {
			reg = 1;
			vf = vf - 64;
		}
		/* clear transcation pending bit */
		writeq(BIT_ULL(vf), pfdev->reg_base + RVU_PF_VFTRPENDX(reg));
		writeq(BIT_ULL(vf), pfdev->reg_base + RVU_PF_VFFLR_INT_ENA_W1SX(reg));
	}

	mutex_unlock(&mbox->lock);
}

static irqreturn_t rvu_gen_pf_me_intr_handler(int irq, void *pf_irq)
{
	struct gen_pf_dev *pfdev = (struct gen_pf_dev *)pf_irq;
	int vf, reg, num_reg = 1;
	u64 intr;

	if (pfdev->total_vfs > 64)
		num_reg = 2;

	for (reg = 0; reg < num_reg; reg++) {
		intr = readq(pfdev->reg_base + RVU_PF_VFME_INTX(reg));
		if (!intr)
			continue;
		for (vf = 0; vf < 64; vf++) {
			if (!(intr & BIT_ULL(vf)))
				continue;
			/* clear trpend bit */
			writeq(BIT_ULL(vf), pfdev->reg_base + RVU_PF_VFTRPENDX(reg));
			/* clear interrupt */
			writeq(BIT_ULL(vf), pfdev->reg_base + RVU_PF_VFME_INTX(reg));
		}
	}
	return IRQ_HANDLED;
}

static irqreturn_t rvu_gen_pf_flr_intr_handler(int irq, void *pf_irq)
{
	struct gen_pf_dev *pfdev = (struct gen_pf_dev *)pf_irq;
	int reg, dev, vf, start_vf, num_reg = 1;
	u64 intr;

	if (pfdev->total_vfs > 64)
		num_reg = 2;

	for (reg = 0; reg < num_reg; reg++) {
		intr = readq(pfdev->reg_base + RVU_PF_VFFLR_INTX(reg));
		if (!intr)
			continue;
		start_vf = 64 * reg;
		for (vf = 0; vf < 64; vf++) {
			if (!(intr & BIT_ULL(vf)))
				continue;
			dev = vf + start_vf;
			queue_work(pfdev->flr_wq, &pfdev->flr_wrk[dev].work);
			/* Clear interrupt */
			writeq(BIT_ULL(vf), pfdev->reg_base + RVU_PF_VFFLR_INTX(reg));
			/* Disable the interrupt */
			writeq(BIT_ULL(vf), pfdev->reg_base + RVU_PF_VFFLR_INT_ENA_W1CX(reg));
		}
	}
	return IRQ_HANDLED;
}

static int rvu_gen_pf_register_flr_me_intr(struct gen_pf_dev *pfdev, int numvfs)
{
	char *irq_name;
	int ret;

	/* Register ME interrupt handler*/
	irq_name = &pfdev->irq_name[RVU_PF_INT_VEC_VFME0 * NAME_SIZE];
	snprintf(irq_name, NAME_SIZE, "Generic RVUPF%d_ME0", rvu_get_pf(pfdev->pcifunc));
	ret = request_irq(pci_irq_vector(pfdev->pdev, RVU_PF_INT_VEC_VFME0),
			  rvu_gen_pf_me_intr_handler, 0, irq_name, pfdev);

	if (ret) {
		dev_err(pfdev->dev,
			"Generic RVUPF: IRQ registration failed for ME0\n");
	}

	/* Register FLR interrupt handler */
	irq_name = &pfdev->irq_name[RVU_PF_INT_VEC_VFFLR0 * NAME_SIZE];
	snprintf(irq_name, NAME_SIZE, "Generic RVUPF%d_FLR0", rvu_get_pf(pfdev->pcifunc));
	ret = request_irq(pci_irq_vector(pfdev->pdev, RVU_PF_INT_VEC_VFFLR0),
			  rvu_gen_pf_flr_intr_handler, 0, irq_name, pfdev);
	if (ret) {
		dev_err(pfdev->dev,
			"Generic RVUPF: IRQ registration failed for FLR0\n");
		return ret;
	}

	if (numvfs > 64) {
		irq_name = &pfdev->irq_name[RVU_PF_INT_VEC_VFME1 * NAME_SIZE];
		snprintf(irq_name, NAME_SIZE, "Generic RVUPF%d_ME1",
			 rvu_get_pf(pfdev->pcifunc));
		ret = request_irq(pci_irq_vector
				  (pfdev->pdev, RVU_PF_INT_VEC_VFME1),
				  rvu_gen_pf_me_intr_handler, 0, irq_name, pfdev);
		if (ret) {
			dev_err(pfdev->dev,
				"Generic RVUPF: IRQ registration failed for ME1\n");
		}
		irq_name = &pfdev->irq_name[RVU_PF_INT_VEC_VFFLR1 * NAME_SIZE];
		snprintf(irq_name, NAME_SIZE, "Generic RVUPF%d_FLR1",
			 rvu_get_pf(pfdev->pcifunc));
		ret = request_irq(pci_irq_vector
				(pfdev->pdev, RVU_PF_INT_VEC_VFFLR1),
				rvu_gen_pf_flr_intr_handler, 0, irq_name, pfdev);
		if (ret) {
			dev_err(pfdev->dev,
				"Generic RVUPF: IRQ registration failed for FLR1\n");
			return ret;
		}
	}

	/* Enable ME interrupt for all VFs*/
	writeq(INTR_MASK(numvfs), pfdev->reg_base + RVU_PF_VFME_INTX(0));
	writeq(INTR_MASK(numvfs), pfdev->reg_base + RVU_PF_VFME_INT_ENA_W1SX(0));

	/* Enable FLR interrupt for all VFs*/
	writeq(INTR_MASK(numvfs), pfdev->reg_base + RVU_PF_VFFLR_INTX(0));
	writeq(INTR_MASK(numvfs), pfdev->reg_base + RVU_PF_VFFLR_INT_ENA_W1SX(0));

	if (numvfs > 64) {
		numvfs -= 64;

		writeq(INTR_MASK(numvfs), pfdev->reg_base + RVU_PF_VFME_INTX(1));
		writeq(INTR_MASK(numvfs), pfdev->reg_base + RVU_PF_VFME_INT_ENA_W1SX(1));

		writeq(INTR_MASK(numvfs), pfdev->reg_base + RVU_PF_VFFLR_INTX(1));
		writeq(INTR_MASK(numvfs), pfdev->reg_base + RVU_PF_VFFLR_INT_ENA_W1SX(1));
	}
	return 0;
}

static void rvu_gen_pf_disable_flr_me_intr(struct gen_pf_dev *pfdev)
{
	int irq, vfs = pfdev->total_vfs;

	/* Disable VFs ME interrupts */
	writeq(INTR_MASK(vfs), pfdev->reg_base + RVU_PF_VFME_INT_ENA_W1CX(0));
	irq = pci_irq_vector(pfdev->pdev, RVU_PF_INT_VEC_VFME0);
	free_irq(irq, pfdev);

	/* Disable VFs FLR interrupts */
	writeq(INTR_MASK(vfs), pfdev->reg_base + RVU_PF_VFFLR_INT_ENA_W1CX(0));
	irq = pci_irq_vector(pfdev->pdev, RVU_PF_INT_VEC_VFFLR0);
	free_irq(irq, pfdev);

	if (vfs <= 64)
		return;

	writeq(INTR_MASK(vfs - 64), pfdev->reg_base + RVU_PF_VFME_INT_ENA_W1CX(1));
	irq = pci_irq_vector(pfdev->pdev, RVU_PF_INT_VEC_VFME1);
	free_irq(irq, pfdev);

	writeq(INTR_MASK(vfs - 64), pfdev->reg_base + RVU_PF_VFFLR_INT_ENA_W1CX(1));
	irq = pci_irq_vector(pfdev->pdev, RVU_PF_INT_VEC_VFFLR1);
	free_irq(irq, pfdev);
}

static int rvu_gen_pf_flr_init(struct gen_pf_dev *pfdev, int num_vfs)
{
	int vf;

	pfdev->flr_wq = alloc_ordered_workqueue("otx2_pf_flr_wq", WQ_HIGHPRI);
	if (!pfdev->flr_wq)
		return -ENOMEM;

	pfdev->flr_wrk = devm_kcalloc(pfdev->dev, num_vfs,
				      sizeof(struct flr_work), GFP_KERNEL);
	if (!pfdev->flr_wrk) {
		destroy_workqueue(pfdev->flr_wq);
		return -ENOMEM;
	}

	for (vf = 0; vf < num_vfs; vf++) {
		pfdev->flr_wrk[vf].pfdev = pfdev;
		INIT_WORK(&pfdev->flr_wrk[vf].work, rvu_gen_pf_flr_handler);
	}

	return 0;
}

static int rvu_gen_pf_vfcfg_init(struct gen_pf_dev *pf)
{
	int i;

	pf->vf_configs = devm_kcalloc(pf->dev, pf->total_vfs,
				      sizeof(struct gen_pf_vf_config),
				      GFP_KERNEL);
	if (!pf->vf_configs)
		return -ENOMEM;

	for (i = 0; i < pf->total_vfs; i++) {
		pf->vf_configs[i].pf = pf;
		INIT_DELAYED_WORK(&pf->vf_configs[i].vf_work,
				  gen_pf_vf_task);
	}

	return 0;
}

static void rvu_gen_pf_vfcfg_cleanup(struct gen_pf_dev *pf)
{
	int i;

	if (!pf->vf_configs)
		return;

	for (i = 0; i < pf->total_vfs; i++)
		cancel_delayed_work_sync(&pf->vf_configs[i].vf_work);
}

static int rvu_gen_pf_sriov_enable(struct pci_dev *pdev, int numvfs)
{
	struct gen_pf_dev *pfdev = pci_get_drvdata(pdev);
	int ret;

	/* Init PF <=> VF mailbox stuff */
	ret = rvu_gen_pf_pfvf_mbox_init(pfdev, numvfs);
	if (ret)
		return ret;

	ret = rvu_gen_pf_register_pfvf_mbox_intr(pfdev, numvfs);
	if (ret)
		goto free_mbox;

	ret = rvu_gen_pf_flr_init(pfdev, numvfs);
	if (ret)
		goto free_intr;

	ret = rvu_gen_pf_register_flr_me_intr(pfdev, numvfs);
	if (ret)
		goto free_flr;

	ret = pci_enable_sriov(pdev, numvfs);
	if (ret)
		goto free_flr_intr;

	return numvfs;
free_flr_intr:
	rvu_gen_pf_disable_flr_me_intr(pfdev);
free_flr:
	rvu_gen_pf_flr_wq_destroy(pfdev);
free_intr:
	rvu_gen_pf_disable_pfvf_mbox_intr(pfdev, numvfs);
free_mbox:
	rvu_gen_pf_pfvf_mbox_destroy(pfdev);
	return ret;
}

static int rvu_gen_pf_sriov_disable(struct pci_dev *pdev)
{
	struct gen_pf_dev *pfdev = pci_get_drvdata(pdev);
	int numvfs = pci_num_vf(pdev);

	if (!numvfs)
		return 0;

	pci_disable_sriov(pdev);

	rvu_gen_pf_disable_flr_me_intr(pfdev);
	rvu_gen_pf_flr_wq_destroy(pfdev);
	rvu_gen_pf_disable_pfvf_mbox_intr(pfdev, numvfs);
	rvu_gen_pf_pfvf_mbox_destroy(pfdev);

	return 0;
}

static int rvu_gen_pf_sriov_configure(struct pci_dev *pdev, int numvfs)
{
	if (numvfs == 0)
		return rvu_gen_pf_sriov_disable(pdev);

	return rvu_gen_pf_sriov_enable(pdev, numvfs);
}

static void rvu_gen_pf_remove(struct pci_dev *pdev)
{
	struct gen_pf_dev *pfdev = pci_get_drvdata(pdev);

	rvu_gen_pf_vfcfg_cleanup(pfdev);
	rvu_gen_pf_sriov_disable(pfdev->pdev);
	pci_set_drvdata(pdev, NULL);

	pci_release_regions(pdev);
}

static int rvu_gen_pf_sdp_init(struct gen_pf_dev *pf)
{
	/* Firmware sets the total VFs such that it includes max VFs of a PF
	 * and one extra VF since VF0 of PF serve IO for EPFs on host.
	 */
	return rvu_gen_pf_sriov_enable(pf->pdev,
				       pci_sriov_get_totalvfs(pf->pdev));
}

static int rvu_gen_pf_probe(struct pci_dev *pdev, const struct pci_device_id *id)
{
	struct device *dev = &pdev->dev;
	struct gen_pf_dev *pfdev;
	int num_vec;
	int err;

	err = pcim_enable_device(pdev);
	if (err) {
		dev_err(dev, "Failed to enable PCI device\n");
		return err;
	}

	err = pci_request_regions(pdev, DRV_NAME);
	if (err) {
		dev_err(dev, "PCI request regions failed %d\n", err);
		goto err_map_failed;
	}

	err = dma_set_mask_and_coherent(dev, DMA_BIT_MASK(48));
	if (err) {
		dev_err(dev, "DMA mask config failed, abort\n");
		goto err_release_regions;
	}

	pci_set_master(pdev);

	pfdev = devm_kzalloc(dev, sizeof(struct gen_pf_dev), GFP_KERNEL);
	if (!pfdev) {
		err = -ENOMEM;
		goto err_release_regions;
	}

	pci_set_drvdata(pdev, pfdev);
	pfdev->pdev = pdev;
	pfdev->dev = dev;
	pfdev->total_vfs = pci_sriov_get_totalvfs(pdev);
	num_vec = pci_msix_vec_count(pdev);
	pfdev->irq_name = devm_kmalloc_array(&pfdev->pdev->dev, num_vec, NAME_SIZE,
					     GFP_KERNEL);

	/* Map CSRs */
	pfdev->reg_base = pcim_iomap(pdev, PCI_CFG_REG_BAR_NUM, 0);
	if (!pfdev->reg_base) {
		dev_err(dev, "Unable to map physical function CSRs, aborting\n");
		err = -ENOMEM;
		goto err_release_regions;
	}

	err = rvu_gen_pf_check_pf_usable(pfdev);
	if (err)
		goto err_pcim_iounmap;

	err = pci_alloc_irq_vectors(pfdev->pdev, num_vec, num_vec, PCI_IRQ_MSIX);
	if (err < 0) {
		dev_err(dev, "%s: Failed to alloc %d IRQ vectors\n",
			__func__, num_vec);
		goto err_pcim_iounmap;
	}

	/* Init PF <=> AF mailbox stuff */
	err = rvu_gen_pf_pfaf_mbox_init(pfdev);
	if (err)
		goto err_free_irq_vectors;

	/* Register mailbox interrupt */
	err = rvu_gen_pf_register_mbox_intr(pfdev);
	if (err)
		goto err_mbox_destroy;

	err = rvu_gen_pf_vfcfg_init(pfdev);
	if (err)
		goto err_mbox_destroy;

	rvu_gen_pf_sdp_init(pfdev);

	return 0;

err_mbox_destroy:
	rvu_gen_pf_pfaf_mbox_destroy(pfdev);
err_free_irq_vectors:
	pci_free_irq_vectors(pfdev->pdev);
err_pcim_iounmap:
	pcim_iounmap(pdev, pfdev->reg_base);
err_release_regions:
	pci_release_regions(pdev);
	pci_set_drvdata(pdev, NULL);
err_map_failed:
	pci_disable_device(pdev);
	return err;
}

static struct pci_driver rvu_gen_driver = {
	.name = DRV_NAME,
	.id_table = rvu_gen_pf_id_table,
	.probe = rvu_gen_pf_probe,
	.remove = rvu_gen_pf_remove,
	.sriov_configure = rvu_gen_pf_sriov_configure,
};

static int __init rvu_gen_pf_init_module(void)
{
	return pci_register_driver(&rvu_gen_driver);
}

static void __exit rvu_gen_pf_cleanup_module(void)
{
	pci_unregister_driver(&rvu_gen_driver);
}

module_init(rvu_gen_pf_init_module);
module_exit(rvu_gen_pf_cleanup_module);
