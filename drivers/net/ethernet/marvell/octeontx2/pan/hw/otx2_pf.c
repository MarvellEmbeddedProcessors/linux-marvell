// SPDX-License-Identifier: GPL-2.0
/* Marvell RVU Physical Function ethernet driver
 *
 * Copyright (C) 2025 Marvell.
 *
 */

#include <linux/module.h>
#include <linux/interrupt.h>
#include <linux/pci.h>
#include <linux/etherdevice.h>
#include <linux/of.h>
#include <linux/if_vlan.h>
#include <linux/iommu.h>
#include <net/ip.h>
#include <linux/bpf.h>
#include <linux/bpf_trace.h>
#include <linux/bitfield.h>

#include "otx2_reg.h"
#include "otx2_common.h"
#include "../../nic/otx2_txrx.h"
#include "otx2_struct.h"
#include "otx2_ptp.h"
#include "cn10k.h"
#include "qos.h"
#include <rvu_trace.h>
#include "hw/otx2_cmn.h"

int otx2_mbox_up_handler_af2swdev_notify(struct otx2_nic *pf,
					 struct af2swdev_notify_req *req,
					 struct msg_rsp *rsp);

static void otx2_forward_msg_pfvf(struct otx2_mbox_dev *mdev,
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

static int otx2_forward_vf_mbox_msgs(struct otx2_nic *pf,
				     struct otx2_mbox *src_mbox,
				     int dir, int vf, int num_msgs)
{
	struct otx2_mbox_dev *src_mdev, *dst_mdev;
	struct mbox_hdr *mbox_hdr;
	struct mbox_hdr *req_hdr;
	struct mbox *dst_mbox;
	int dst_size, err;

	if (dir == MBOX_DIR_PFAF) {
		/* Set VF's mailbox memory as PF's bounce buffer memory, so
		 * that explicit copying of VF's msgs to PF=>AF mbox region
		 * and AF=>PF responses to VF's mbox region can be avoided.
		 */
		src_mdev = &src_mbox->dev[vf];
		mbox_hdr = src_mbox->hwbase +
				src_mbox->rx_start + (vf * MBOX_SIZE);

		dst_mbox = &pf->mbox;
		dst_size = dst_mbox->mbox.tx_size -
				ALIGN(sizeof(*mbox_hdr), MBOX_MSG_ALIGN);
		/* Check if msgs fit into destination area and has valid size */
		if (mbox_hdr->msg_size > dst_size || !mbox_hdr->msg_size)
			return -EINVAL;

		dst_mdev = &dst_mbox->mbox.dev[0];

		mutex_lock(&pf->mbox.lock);
		dst_mdev->mbase = src_mdev->mbase;
		dst_mdev->msg_size = mbox_hdr->msg_size;
		dst_mdev->num_msgs = num_msgs;
		err = otx2_sync_mbox_msg(dst_mbox);
		/* Error code -EIO indicate there is a communication failure
		 * to the AF. Rest of the error codes indicate that AF processed
		 * VF messages and set the error codes in response messages
		 * (if any) so simply forward responses to VF.
		 */
		if (err == -EIO) {
			dev_warn(pf->dev,
				 "AF not responding to VF%d messages\n", vf);
			/* restore PF mbase and exit */
			dst_mdev->mbase = pf->mbox.bbuf_base;
			mutex_unlock(&pf->mbox.lock);
			return err;
		}
		/* At this point, all the VF messages sent to AF are acked
		 * with proper responses and responses are copied to VF
		 * mailbox hence raise interrupt to VF.
		 */
		req_hdr = (struct mbox_hdr *)(dst_mdev->mbase +
					      dst_mbox->mbox.rx_start);
		req_hdr->num_msgs = num_msgs;

		otx2_forward_msg_pfvf(dst_mdev, &pf->mbox_pfvf[0].mbox,
				      pf->mbox.bbuf_base, vf);
		mutex_unlock(&pf->mbox.lock);
	} else if (dir == MBOX_DIR_PFVF_UP) {
		src_mdev = &src_mbox->dev[0];
		mbox_hdr = src_mbox->hwbase + src_mbox->rx_start;
		req_hdr = (struct mbox_hdr *)(src_mdev->mbase +
					      src_mbox->rx_start);
		req_hdr->num_msgs = num_msgs;

		dst_mbox = &pf->mbox_pfvf[0];
		dst_size = dst_mbox->mbox_up.tx_size -
				ALIGN(sizeof(*mbox_hdr), MBOX_MSG_ALIGN);
		/* Check if msgs fit into destination area */
		if (mbox_hdr->msg_size > dst_size)
			return -EINVAL;

		dst_mdev = &dst_mbox->mbox_up.dev[vf];
		dst_mdev->mbase = src_mdev->mbase;
		dst_mdev->msg_size = mbox_hdr->msg_size;
		dst_mdev->num_msgs = mbox_hdr->num_msgs;
		err = otx2_sync_mbox_up_msg(dst_mbox, vf);
		if (err) {
			dev_warn(pf->dev,
				 "VF%d is not responding to mailbox\n", vf);
			return err;
		}
	} else if (dir == MBOX_DIR_VFPF_UP) {
		req_hdr = (struct mbox_hdr *)(src_mbox->dev[0].mbase +
					      src_mbox->rx_start);
		req_hdr->num_msgs = num_msgs;
		otx2_forward_msg_pfvf(&pf->mbox_pfvf->mbox_up.dev[vf],
				      &pf->mbox.mbox_up,
				      pf->mbox_pfvf[vf].bbuf_base,
				      0);
	}

	return 0;
}

static void otx2_process_pfaf_mbox_msg(struct otx2_nic *pf,
				       struct mbox_msghdr *msg)
{
	int devid;

	if (msg->id >= MBOX_MSG_MAX) {
		dev_err(pf->dev,
			"Mbox msg with unknown ID 0x%x\n", msg->id);
		return;
	}

	if (msg->sig != OTX2_MBOX_RSP_SIG) {
		dev_err(pf->dev,
			"Mbox msg with wrong signature %x, ID 0x%x\n",
			 msg->sig, msg->id);
		return;
	}

	/* message response heading VF */
	devid = msg->pcifunc & RVU_PFVF_FUNC_MASK;
	if (devid) {
		struct otx2_vf_config *config = &pf->vf_configs[devid - 1];
		struct delayed_work *dwork;

		switch (msg->id) {
		case MBOX_MSG_NIX_LF_START_RX:
			config->intf_down = false;
			dwork = &config->link_event_work;
			schedule_delayed_work(dwork, msecs_to_jiffies(100));
			break;
		case MBOX_MSG_NIX_LF_STOP_RX:
			config->intf_down = true;
			break;
		}

		return;
	}

	switch (msg->id) {
	case MBOX_MSG_READY:
		pf->pcifunc = msg->pcifunc;
		break;
	case MBOX_MSG_MSIX_OFFSET:
		mbox_handler_msix_offset(pf, (struct msix_offset_rsp *)msg);
		break;
	case MBOX_MSG_NPA_LF_ALLOC:
		dup_mbox_handler_npa_lf_alloc(pf, (struct npa_lf_alloc_rsp *)msg);
		break;
	case MBOX_MSG_NIX_LF_ALLOC:
		dup_mbox_handler_nix_lf_alloc(pf, (struct nix_lf_alloc_rsp *)msg);
		break;
	case MBOX_MSG_NIX_BP_ENABLE:
		dup_mbox_handler_nix_bp_enable(pf, (struct nix_bp_cfg_rsp *)msg);
		break;
	case MBOX_MSG_CGX_STATS:
		dup_mbox_handler_cgx_stats(pf, (struct cgx_stats_rsp *)msg);
		break;
	case MBOX_MSG_CGX_FEC_STATS:
		dup_mbox_handler_cgx_fec_stats(pf, (struct cgx_fec_stats_rsp *)msg);
		break;
	default:
		if (msg->rc)
			dev_err(pf->dev,
				"Mbox msg response has err %d, ID 0x%x\n",
				msg->rc, msg->id);
		break;
	}
}

static void otx2_pfaf_mbox_handler(struct work_struct *work)
{
	struct otx2_mbox_dev *mdev;
	struct mbox_hdr *rsp_hdr;
	struct mbox_msghdr *msg;
	struct otx2_mbox *mbox;
	struct mbox *af_mbox;
	struct otx2_nic *pf;
	int offset, id;
	u16 num_msgs;

	af_mbox = container_of(work, struct mbox, mbox_wrk);
	mbox = &af_mbox->mbox;
	mdev = &mbox->dev[0];
	rsp_hdr = (struct mbox_hdr *)(mdev->mbase + mbox->rx_start);
	num_msgs = rsp_hdr->num_msgs;

	offset = mbox->rx_start + ALIGN(sizeof(*rsp_hdr), MBOX_MSG_ALIGN);
	pf = af_mbox->pfvf;

	trace_otx2_msg_status(pf->pdev, "PF-AF down queue handler(response)",
			      num_msgs);

	for (id = 0; id < num_msgs; id++) {
		msg = (struct mbox_msghdr *)(mdev->mbase + offset);
		otx2_process_pfaf_mbox_msg(pf, msg);
		offset = mbox->rx_start + msg->next_msgoff;
		if (mdev->msgs_acked == (num_msgs - 1))
			__otx2_mbox_reset(mbox, 0);
		mdev->msgs_acked++;
	}
}

static int otx2_process_mbox_msg_up(struct otx2_nic *pf,
				    struct mbox_msghdr *req)
{
	/* Check if valid, if not reply with a invalid msg */
	if (req->sig != OTX2_MBOX_REQ_SIG) {
		otx2_reply_invalid_msg(&pf->mbox.mbox_up, 0, 0, req->id);
		return -ENODEV;
	}

	switch (req->id) {
#define M(_name, _id, _fn_name, _req_type, _rsp_type)			\
	case _id: {							\
		struct _rsp_type *rsp;					\
		int err;						\
									\
		rsp = (struct _rsp_type *)otx2_mbox_alloc_msg(		\
			&pf->mbox.mbox_up, 0,				\
			sizeof(struct _rsp_type));			\
		if (!rsp)						\
			return -ENOMEM;					\
									\
		rsp->hdr.id = _id;					\
		rsp->hdr.sig = OTX2_MBOX_RSP_SIG;			\
		rsp->hdr.pcifunc = 0;					\
		rsp->hdr.rc = 0;					\
									\
		err = otx2_mbox_up_handler_ ## _fn_name(		\
			pf, (struct _req_type *)req, rsp);		\
		return err;						\
	}
MBOX_UP_CGX_MESSAGES
MBOX_UP_AF2SWDEV_MESSAGES
#undef M
		break;
	default:
		otx2_reply_invalid_msg(&pf->mbox.mbox_up, 0, 0, req->id);
		return -ENODEV;
	}
	return 0;
}

static void otx2_pfaf_mbox_up_handler(struct work_struct *work)
{
	struct mbox *af_mbox = container_of(work, struct mbox, mbox_up_wrk);
	struct otx2_mbox *mbox = &af_mbox->mbox_up;
	struct otx2_mbox_dev *mdev = &mbox->dev[0];
	struct otx2_nic *pf = af_mbox->pfvf;
	int offset, id, devid = 0;
	struct mbox_hdr *rsp_hdr;
	struct mbox_msghdr *msg;
	u16 num_msgs;

	rsp_hdr = (struct mbox_hdr *)(mdev->mbase + mbox->rx_start);
	num_msgs = rsp_hdr->num_msgs;

	offset = mbox->rx_start + ALIGN(sizeof(*rsp_hdr), MBOX_MSG_ALIGN);

	trace_otx2_msg_status(pf->pdev, "PF-AF up queue handler(notification)",
			      num_msgs);

	for (id = 0; id < num_msgs; id++) {
		msg = (struct mbox_msghdr *)(mdev->mbase + offset);

		devid = msg->pcifunc & RVU_PFVF_FUNC_MASK;
		/* Skip processing VF's messages */
		if (!devid)
			otx2_process_mbox_msg_up(pf, msg);
		offset = mbox->rx_start + msg->next_msgoff;
	}
	/* Forward to VF iff VFs are really present */
	if (devid && pci_num_vf(pf->pdev)) {
		otx2_forward_vf_mbox_msgs(pf, &pf->mbox.mbox_up,
					  MBOX_DIR_PFVF_UP, devid - 1,
					  num_msgs);
		return;
	}

	otx2_mbox_msg_send(mbox, 0);
}

void dup_disable_mbox_intr(struct otx2_nic *pf)
{
	int vector = pci_irq_vector(pf->pdev, RVU_PF_INT_VEC_AFPF_MBOX);

	/* Disable AF => PF mailbox IRQ */
	otx2_write64(pf, RVU_PF_INT_ENA_W1C, BIT_ULL(0));
	free_irq(vector, pf);
}

int dup_register_mbox_intr(struct otx2_nic *pf, bool probe_af)
{
	struct otx2_hw *hw = &pf->hw;
	struct msg_req *req;
	char *irq_name;
	int err;

	/* Register mailbox interrupt handler */
	irq_name = &hw->irq_name[RVU_PF_INT_VEC_AFPF_MBOX * NAME_SIZE];
	snprintf(irq_name, NAME_SIZE, "RVUPFAF Mbox");
	err = request_irq(pci_irq_vector(pf->pdev, RVU_PF_INT_VEC_AFPF_MBOX),
			  otx2_pfaf_mbox_intr_handler, 0, irq_name, pf);
	if (err) {
		dev_err(pf->dev,
			"RVUPF: IRQ registration failed for PFAF mbox irq\n");
		return err;
	}

	/* Enable mailbox interrupt for msgs coming from AF.
	 * First clear to avoid spurious interrupts, if any.
	 */
	otx2_write64(pf, RVU_PF_INT, BIT_ULL(0));
	otx2_write64(pf, RVU_PF_INT_ENA_W1S, BIT_ULL(0));

	if (!probe_af)
		return 0;

	/* Check mailbox communication with AF */
	req = otx2_mbox_alloc_msg_ready(&pf->mbox);
	if (!req) {
		dup_disable_mbox_intr(pf);
		return -ENOMEM;
	}
	err = otx2_sync_mbox_msg(&pf->mbox);
	if (err) {
		dev_warn(pf->dev,
			 "AF not responding to mailbox, deferring probe\n");
		dup_disable_mbox_intr(pf);
		return -EPROBE_DEFER;
	}

	return 0;
}

void dup_pfaf_mbox_destroy(struct otx2_nic *pf)
{
	struct mbox *mbox = &pf->mbox;

	if (pf->mbox_wq) {
		destroy_workqueue(pf->mbox_wq);
		pf->mbox_wq = NULL;
	}

	if (mbox->mbox.hwbase)
		iounmap((void __iomem *)mbox->mbox.hwbase);

	otx2_mbox_destroy(&mbox->mbox);
	otx2_mbox_destroy(&mbox->mbox_up);
}

int dup_pfaf_mbox_init(struct otx2_nic *pf)
{
	struct mbox *mbox = &pf->mbox;
	void __iomem *hwbase;
	int err;

	mbox->pfvf = pf;
	pf->mbox_wq = alloc_ordered_workqueue("otx2_pfaf_mailbox",
					      WQ_HIGHPRI | WQ_MEM_RECLAIM);
	if (!pf->mbox_wq)
		return -ENOMEM;

	/* Mailbox is a reserved memory (in RAM) region shared between
	 * admin function (i.e AF) and this PF, shouldn't be mapped as
	 * device memory to allow unaligned accesses.
	 */
	hwbase = ioremap_wc(pci_resource_start(pf->pdev, PCI_MBOX_BAR_NUM),
			    MBOX_SIZE);
	if (!hwbase) {
		dev_err(pf->dev, "Unable to map PFAF mailbox region\n");
		err = -ENOMEM;
		goto exit;
	}

	err = otx2_mbox_init(&mbox->mbox, hwbase, pf->pdev, pf->reg_base,
			     MBOX_DIR_PFAF, 1);
	if (err)
		goto exit;

	err = otx2_mbox_init(&mbox->mbox_up, hwbase, pf->pdev, pf->reg_base,
			     MBOX_DIR_PFAF_UP, 1);
	if (err)
		goto exit;

	err = otx2_mbox_bbuf_init(mbox, pf->pdev);
	if (err)
		goto exit;

	INIT_WORK(&mbox->mbox_wrk, otx2_pfaf_mbox_handler);
	INIT_WORK(&mbox->mbox_up_wrk, otx2_pfaf_mbox_up_handler);
	mutex_init(&mbox->lock);

	return 0;
exit:
	dup_pfaf_mbox_destroy(pf);
	return err;
}

static void otx2_free_cq_res(struct otx2_nic *pf)
{
	struct otx2_qset *qset = &pf->qset;
	struct otx2_cq_queue *cq;
	int qidx;

	/* Disable CQs */
	dup_ctx_disable(&pf->mbox, NIX_AQ_CTYPE_CQ, false);
	for (qidx = 0; qidx < qset->cq_cnt; qidx++) {
		cq = &qset->cq[qidx];
		qmem_free(pf->dev, cq->cqe);
	}
}

static void otx2_free_sq_res(struct otx2_nic *pf)
{
	struct otx2_qset *qset = &pf->qset;
	struct otx2_snd_queue *sq;
	int qidx;

	/* Disable SQs */
	dup_ctx_disable(&pf->mbox, NIX_AQ_CTYPE_SQ, false);
	/* Free SQB pointers */
	dup_sq_free_sqbs(pf);
	for (qidx = 0; qidx < otx2_get_total_tx_queues(pf); qidx++) {
		sq = &qset->sq[qidx];
		/* Skip freeing Qos queues if they are not initialized */
		if (!sq->sqe)
			continue;
		qmem_free(pf->dev, sq->sqe);
		qmem_free(pf->dev, sq->tso_hdrs);
		kfree(sq->sg);
		kfree(sq->sqb_ptrs);
	}
}

static int otx2_get_rbuf_size(struct otx2_nic *pf, int mtu)
{
	int frame_size;
	int total_size;
	int rbuf_size;

	if (pf->hw.rbuf_len)
		return ALIGN(pf->hw.rbuf_len, OTX2_ALIGN) + OTX2_HEAD_ROOM;

	/* The data transferred by NIX to memory consists of actual packet
	 * plus additional data which has timestamp and/or EDSA/HIGIG2
	 * headers if interface is configured in corresponding modes.
	 * NIX transfers entire data using 6 segments/buffers and writes
	 * a CQE_RX descriptor with those segment addresses. First segment
	 * has additional data prepended to packet. Also software omits a
	 * headroom of 128 bytes in each segment. Hence the total size of
	 * memory needed to receive a packet with 'mtu' is:
	 * frame size =  mtu + additional data;
	 * memory = frame_size + headroom * 6;
	 * each receive buffer size = memory / 6;
	 */
	frame_size = mtu + OTX2_ETH_HLEN + OTX2_HW_TIMESTAMP_LEN +
		     pf->addl_mtu + pf->xtra_hdr;
	total_size = frame_size + OTX2_HEAD_ROOM * 6;
	rbuf_size = total_size / 6;

	return ALIGN(rbuf_size, 2048);
}

int dup_init_hw_resources(struct otx2_nic *pf)
{
	struct nix_lf_free_req *free_req;
	struct mbox *mbox = &pf->mbox;
	struct otx2_cmn_fops *ops;
	struct otx2_hw *hw = &pf->hw;
	struct msg_req *req;
	int err = 0;

	ops = otx2_cmn_fops_arr_lookup(pf->pdev->device);
	if (!ops) {
		netdev_err(pf->netdev, "Could not locate ops structure\n");
		return -EINVAL;
	}

	/* Set required NPA LF's pool counts
	 * Auras and Pools are used in a 1:1 mapping,
	 * so, aura count = pool count.
	 */
	hw->rqpool_cnt = hw->rx_queues;
	hw->sqpool_cnt = otx2_get_total_tx_queues(pf);
	hw->pool_cnt = hw->rqpool_cnt + hw->sqpool_cnt;

	/* Maximum hardware supported transmit length */
	pf->tx_max_pktlen = pf->netdev->max_mtu + OTX2_ETH_HLEN;

	pf->rbsize = otx2_get_rbuf_size(pf, pf->netdev->mtu);

	mutex_lock(&mbox->lock);
	/* NPA init */
	err = dup_config_npa(pf);
	if (err) {
		dev_err(pf->dev, "Failed to configure NPA\n");
		goto exit;
	}
	/* NIX init */
	err = dup_config_nix(pf);
	if (err) {
		dev_err(pf->dev, "Failed to configure NIX\n");
		goto err_free_npa_lf;
	}

	/* Enable backpressure for CGX mapped PF/VFs */
	if (!is_otx2_lbkvf(pf->pdev))
		dup_nix_config_bp(pf, true);

	/* Init Auras and pools used by NIX RQ, for free buffer ptrs */
	err = dup_rq_aura_pool_init(pf);
	if (err) {
		dev_err(pf->dev, "Failed configure RQ Aura\n");
		goto err_free_nix_lf;
	}
	/* Init Auras and pools used by NIX SQ, for queueing SQEs */
	err = dup_sq_aura_pool_init(pf);
	if (err) {
		dev_err(pf->dev, "Failed to configure SQ Aura\n");
		goto err_free_rq_ptrs;
	}

	err = ops->tx_schq_init(pf);
	if (err) {
		dev_err(pf->dev, "Failed to config NIX queues\n");
		goto err_free_sq_ptrs;
	}

	err = dup_config_nix_queues(pf);
	if (err) {
		dev_err(pf->dev, "Failed to config NIX queues\n");
		goto err_free_txsch;
	}

	mutex_unlock(&mbox->lock);
	return err;

err_free_txsch:
	dup_txschq_stop(pf);
	otx2_free_sq_res(pf);
	otx2_free_cq_res(pf);
	dup_ctx_disable(mbox, NIX_AQ_CTYPE_RQ, false);
err_free_sq_ptrs:
	dup_sq_free_sqbs(pf);

err_free_rq_ptrs:
	dup_otx2_free_aura_ptr(pf, AURA_NIX_RQ);
	dup_ctx_disable(mbox, NPA_AQ_CTYPE_POOL, true);
	dup_ctx_disable(mbox, NPA_AQ_CTYPE_AURA, true);
	dup_aura_pool_free(pf);
err_free_nix_lf:
	free_req = otx2_mbox_alloc_msg_nix_lf_free(mbox);
	if (free_req) {
		free_req->flags = NIX_LF_DISABLE_FLOWS;
		if (otx2_sync_mbox_msg(mbox))
			dev_err(pf->dev, "%s failed to free nixlf\n", __func__);
	}
err_free_npa_lf:
	/* Reset NPA LF */
	req = otx2_mbox_alloc_msg_npa_lf_free(mbox);
	if (req) {
		if (otx2_sync_mbox_msg(mbox))
			dev_err(pf->dev, "%s failed to free npalf\n", __func__);
	}
exit:
	mutex_unlock(&mbox->lock);
	return err;
}

void dup_free_hw_resources(struct otx2_nic *pf)
{
	struct otx2_qset *qset = &pf->qset;
	struct nix_lf_free_req *free_req;
	struct mbox *mbox = &pf->mbox;
	struct otx2_cmn_fops *ops;
	struct otx2_cq_queue *cq;
	struct otx2_pool *pool;
	struct msg_req *req;
	int pool_id;
	int qidx;

	/* Ensure all SQE are processed */
	dup_sqb_flush(pf);

	/* Stop transmission */
	dup_txschq_stop(pf);

#ifdef CONFIG_DCB
	if (pf->pfc_en)
		otx2_pfc_txschq_stop(pf);
#endif

	dup_clean_qos_queues(pf);

	mutex_lock(&mbox->lock);
	/* Disable backpressure */
	if (!(pf->pcifunc & RVU_PFVF_FUNC_MASK))
		dup_nix_config_bp(pf, false);
	mutex_unlock(&mbox->lock);

	/* Disable RQs */
	dup_ctx_disable(mbox, NIX_AQ_CTYPE_RQ, false);

	ops = otx2_cmn_fops_arr_lookup(pf->pdev->device);
	BUG_ON(!ops);

	/*Dequeue all CQEs */
	for (qidx = 0; qidx < qset->cq_cnt; qidx++) {
		cq = &qset->cq[qidx];
		if (cq->cq_type == CQ_RX)
			ops->rx_cq_clean(pf, cq, qidx);
#if IS_ENABLED(CONFIG_OCTEONTX_PAN_TX_COMPLETION)
		else
			ops->tx_cq_clean(pf, cq, qidx);
#endif
	}

	dup_free_pending_sqe(pf);

	otx2_free_sq_res(pf);

	/* Free RQ buffer pointers*/
	dup_otx2_free_aura_ptr(pf, AURA_NIX_RQ);

	for (qidx = 0; qidx < pf->hw.rx_queues; qidx++) {
		pool_id = otx2_get_pool_idx(pf, AURA_NIX_RQ, qidx);
		pool = &pf->qset.pool[pool_id];
		page_pool_destroy(pool->page_pool);
		pool->page_pool = NULL;
	}

	otx2_free_cq_res(pf);

	/* Free all ingress bandwidth profiles allocated */
	dup_cn10k_free_all_ipolicers(pf);

	mutex_lock(&mbox->lock);
	/* Reset NIX LF */
	free_req = otx2_mbox_alloc_msg_nix_lf_free(mbox);
	if (free_req) {
		free_req->flags = NIX_LF_DISABLE_FLOWS;
		if (!(pf->flags & OTX2_FLAG_PF_SHUTDOWN))
			free_req->flags |= NIX_LF_DONT_FREE_TX_VTAG;
		if (otx2_sync_mbox_msg(mbox))
			dev_err(pf->dev, "%s failed to free nixlf\n", __func__);
	}
	mutex_unlock(&mbox->lock);

	/* Disable NPA Pool and Aura hw context */
	dup_ctx_disable(mbox, NPA_AQ_CTYPE_POOL, true);
	dup_ctx_disable(mbox, NPA_AQ_CTYPE_AURA, true);
	dup_aura_pool_free(pf);

	mutex_lock(&mbox->lock);
	/* Reset NPA LF */
	req = otx2_mbox_alloc_msg_npa_lf_free(mbox);
	if (req) {
		if (otx2_sync_mbox_msg(mbox))
			dev_err(pf->dev, "%s failed to free npalf\n", __func__);
	}
	mutex_unlock(&mbox->lock);
}

void dup_free_queue_mem(struct otx2_qset *qset)
{
	kfree(qset->sq);
	qset->sq = NULL;
	kfree(qset->cq);
	qset->cq = NULL;
	kfree(qset->rq);
	qset->rq = NULL;
	kfree(qset->napi);
}

int dup_check_pf_usable(struct otx2_nic *nic)
{
	u64 rev;

	rev = otx2_read64(nic, RVU_PF_BLOCK_ADDRX_DISC(BLKADDR_RVUM));
	rev = (rev >> 12) & 0xFF;
	/* Check if AF has setup revision for RVUM block,
	 * otherwise this driver probe should be deferred
	 * until AF driver comes up.
	 */
	if (!rev) {
		dev_warn(nic->dev,
			 "AF is not initialized, deferring probe\n");
		return -EPROBE_DEFER;
	}
	return 0;
}

int dup_realloc_msix_vectors(struct otx2_nic *pf)
{
	struct otx2_hw *hw = &pf->hw;
	int num_vec, err;

	/* NPA interrupts are inot registered, so alloc only
	 * up to NIX vector offset.
	 */
	num_vec = hw->nix_msixoff;
	num_vec += NIX_LF_CINT_VEC_START + hw->max_queues;

	dup_disable_mbox_intr(pf);
	pci_free_irq_vectors(hw->pdev);
	err = pci_alloc_irq_vectors(hw->pdev, num_vec, num_vec, PCI_IRQ_MSIX);
	if (err < 0) {
		dev_err(pf->dev, "%s: Failed to realloc %d IRQ vectors\n",
			__func__, num_vec);
		return err;
	}

	return dup_register_mbox_intr(pf, false);
}
