// SPDX-License-Identifier: GPL-2.0
/* Marvell RVU Ethernet driver
 *
 * Copyright (C) 2025 Marvell.
 *
 */

#include "cn10k.h"
#include "otx2_reg.h"
#include "otx2_struct.h"
#include "hw/otx2_cmn.h"

static struct dev_hw_ops	otx2_hw_ops = {
	.sq_aq_init = dup_sq_aq_init,
	.sqe_flush = dup_sqe_flush,
	.aura_freeptr = otx2_aura_freeptr,
	.refill_pool_ptrs = dup_refill_pool_ptrs,
};

static int dup_cn10k_sq_aq_init(void *dev, u16 qidx, u8 chan_offset, u16 sqb_aura);
static struct dev_hw_ops cn10k_hw_ops = {
	.sq_aq_init = dup_cn10k_sq_aq_init,
	.sqe_flush = dup_cn10k_sqe_flush,
	.aura_freeptr = cn10k_aura_freeptr,
	.refill_pool_ptrs = dup_cn10k_refill_pool_ptrs,
};

int dup_cn10k_lmtst_init(struct otx2_nic *pfvf)
{
	struct lmtst_tbl_setup_req *req;
	struct otx2_lmt_info *lmt_info;
	int err, cpu;

	if (!test_bit(CN10K_LMTST, &pfvf->hw.cap_flag)) {
		pfvf->hw_ops = &otx2_hw_ops;
		return 0;
	}

	pfvf->hw_ops = &cn10k_hw_ops;
	/* Total LMTLINES = num_online_cpus() * 32 (For Burst flush).*/
	pfvf->tot_lmt_lines = (num_online_cpus() * LMT_BURST_SIZE);
	pfvf->hw.lmt_info = alloc_percpu(struct otx2_lmt_info);

	mutex_lock(&pfvf->mbox.lock);
	req = otx2_mbox_alloc_msg_lmtst_tbl_setup(&pfvf->mbox);
	if (!req) {
		mutex_unlock(&pfvf->mbox.lock);
		return -ENOMEM;
	}

	req->use_local_lmt_region = true;

	err = qmem_alloc(pfvf->dev, &pfvf->dync_lmt, pfvf->tot_lmt_lines,
			 LMT_LINE_SIZE);
	if (err) {
		mutex_unlock(&pfvf->mbox.lock);
		return err;
	}
	pfvf->hw.lmt_base = (u64 *)pfvf->dync_lmt->base;
	req->lmt_iova = (u64)pfvf->dync_lmt->iova;

	err = otx2_sync_mbox_msg(&pfvf->mbox);
	mutex_unlock(&pfvf->mbox.lock);

	for_each_possible_cpu(cpu) {
		lmt_info = per_cpu_ptr(pfvf->hw.lmt_info, cpu);
		lmt_info->lmt_addr = ((u64)pfvf->hw.lmt_base +
				      (cpu * LMT_BURST_SIZE * LMT_LINE_SIZE));
		lmt_info->lmt_id = cpu * LMT_BURST_SIZE;
	}

	return 0;
}

static int dup_cn10k_sq_aq_init(void *dev, u16 qidx, u8 chan_offset, u16 sqb_aura)
{
	struct nix_cn10k_aq_enq_req *aq;
	struct otx2_nic *pfvf = dev;

	/* Get memory to put this msg */
	aq = otx2_mbox_alloc_msg_nix_cn10k_aq_enq(&pfvf->mbox);
	if (!aq)
		return -ENOMEM;

	aq->sq.cq = pfvf->hw.rx_queues + qidx;
	aq->sq.max_sqe_size = NIX_MAXSQESZ_W16; /* 128 byte */
	aq->sq.cq_ena = 1;
	aq->sq.ena = 1;
	aq->sq.smq = otx2_get_smq_idx(pfvf, qidx);
	aq->sq.smq_rr_weight = mtu_to_dwrr_weight(pfvf, pfvf->tx_max_pktlen);
	aq->sq.default_chan = pfvf->hw.tx_chan_base + chan_offset;
	aq->sq.sqe_stype = NIX_STYPE_STF; /* Cache SQB */
	aq->sq.sqb_aura = sqb_aura;
	aq->sq.sq_int_ena = NIX_SQINT_BITS;
	aq->sq.qint_idx = 0;
	/* Due pipelining impact minimum 2000 unused SQ CQE's
	 * need to maintain to avoid CQ overflow.
	 */
	aq->sq.cq_limit = ((SEND_CQ_SKID * 256) / (pfvf->qset.sqe_cnt));

	/* Fill AQ info */
	aq->qidx = qidx;
	aq->ctype = NIX_AQ_CTYPE_SQ;
	aq->op = NIX_AQ_INSTOP_INIT;

	return otx2_sync_mbox_msg(&pfvf->mbox);
}

#define NPA_MAX_BURST 16
int dup_cn10k_refill_pool_ptrs(void *dev, struct otx2_cq_queue *cq)
{
	struct otx2_nic *pfvf = dev;
	int cnt = cq->pool_ptrs;
	u64 ptrs[NPA_MAX_BURST];
	dma_addr_t bufptr;
	int num_ptrs = 1;

	/* Refill pool with new buffers */
	while (cq->pool_ptrs) {
		if (dup_alloc_buffer(pfvf, cq, &bufptr)) {
			if (num_ptrs--)
				__cn10k_aura_freeptr(pfvf, cq->cq_idx, ptrs,
						     num_ptrs);
			break;
		}
		cq->pool_ptrs--;
		ptrs[num_ptrs] = (u64)bufptr + OTX2_HEAD_ROOM;
		num_ptrs++;
		if (num_ptrs == NPA_MAX_BURST || cq->pool_ptrs == 0) {
			__cn10k_aura_freeptr(pfvf, cq->cq_idx, ptrs,
					     num_ptrs);
			num_ptrs = 1;
		}
	}
	return cnt - cq->pool_ptrs;
}

void dup_cn10k_sqe_flush(void *dev, struct otx2_snd_queue *sq, int size, int qidx)
{
	struct otx2_lmt_info *lmt_info;
	struct otx2_nic *pfvf = dev;
	u64 val = 0, tar_addr = 0;

	lmt_info = per_cpu_ptr(pfvf->hw.lmt_info, smp_processor_id());
	/* FIXME: val[0:10] LMT_ID.
	 * [12:15] no of LMTST - 1 in the burst.
	 * [19:63] data size of each LMTST in the burst except first.
	 */
	val = (lmt_info->lmt_id & 0x7FF);
	/* Target address for LMTST flush tells HW how many 128bit
	 * words are present.
	 * tar_addr[6:4] size of first LMTST - 1 in units of 128b.
	 */
	tar_addr |= sq->io_addr | (((size / 16) - 1) & 0x7) << 4;
	dma_wmb();
	memcpy((u64 *)lmt_info->lmt_addr, sq->sqe_base, size);
	cn10k_lmt_flush(val, tar_addr);

	sq->head++;
	sq->head &= (sq->sqe_cnt - 1);
}

int dup_cn10k_free_all_ipolicers(struct otx2_nic *pfvf)
{
	struct nix_bandprof_free_req *req;
	int rc;

	if (is_dev_otx2(pfvf->pdev))
		return 0;

	mutex_lock(&pfvf->mbox.lock);

	req = otx2_mbox_alloc_msg_nix_bandprof_free(&pfvf->mbox);
	if (!req) {
		rc =  -ENOMEM;
		goto out;
	}

	/* Free all bandwidth profiles allocated */
	req->free_all = true;

	rc = otx2_sync_mbox_msg(&pfvf->mbox);
out:
	mutex_unlock(&pfvf->mbox.lock);
	return rc;
}

int dup_cn10k_alloc_leaf_profile(struct otx2_nic *pfvf, u16 *leaf)
{
	struct nix_bandprof_alloc_req *req;
	struct nix_bandprof_alloc_rsp *rsp;
	int rc;

	req = otx2_mbox_alloc_msg_nix_bandprof_alloc(&pfvf->mbox);
	if (!req)
		return  -ENOMEM;

	req->prof_count[BAND_PROF_LEAF_LAYER] = 1;

	rc = otx2_sync_mbox_msg(&pfvf->mbox);
	if (rc)
		goto out;

	rsp = (struct  nix_bandprof_alloc_rsp *)
	       otx2_mbox_get_rsp(&pfvf->mbox.mbox, 0, &req->hdr);
	if (!rsp->prof_count[BAND_PROF_LEAF_LAYER]) {
		rc = -EIO;
		goto out;
	}

	*leaf = rsp->prof_idx[BAND_PROF_LEAF_LAYER][0];
out:
	if (rc) {
		dev_warn(pfvf->dev,
			 "Failed to allocate ingress bandwidth policer\n");
	}

	return rc;
}

#define POLICER_TIMESTAMP	  1  /* 1 second */
#define MAX_RATE_EXP		  22 /* Valid rate exponent range: 0 - 22 */
