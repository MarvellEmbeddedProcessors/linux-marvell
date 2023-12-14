// SPDX-License-Identifier: GPL-2.0
/* Marvell RVU Ethernet driver
 *
 * Copyright (C) 2025 Marvell.
 *
 */

#include <linux/interrupt.h>
#include <linux/pci.h>
#include <net/tso.h>
#include <linux/bitfield.h>

#include "otx2_reg.h"
#include "otx2_common.h"
#include "otx2_struct.h"
#include "hw/otx2_cmn.h"
#include "cn10k.h"

static struct xarray oxt2_cmn_init_fops_arr;

struct otx2_cmn_fops *otx2_cmn_fops_arr_lookup(int pci_dev_id)
{
	unsigned long pos = pci_dev_id;
	struct otx2_cmn_fops *ops;

	ops = xa_find(&oxt2_cmn_init_fops_arr, &pos, ULONG_MAX, XA_PRESENT);

	return ops;
}

void otx2_cmn_fops_arr_add(int pci_dev_id, struct otx2_cmn_fops *ops)
{
	xa_store(&oxt2_cmn_init_fops_arr, pci_dev_id, ops, GFP_KERNEL);
}

void otx2_cmn_fops_arr_del(int pci_dev_id)
{
	xa_erase(&oxt2_cmn_init_fops_arr, pci_dev_id);
}

static int otx2_alloc_pool_buf(struct otx2_nic *pfvf, struct otx2_pool *pool,
			       dma_addr_t *dma)
{
	unsigned int offset = 0;
	struct page *page;
	size_t sz;

	sz = SKB_DATA_ALIGN(pool->rbsize);
	sz = ALIGN(sz, OTX2_ALIGN);

	page = page_pool_alloc_frag(pool->page_pool, &offset, sz, GFP_ATOMIC);
	if (unlikely(!page))
		return -ENOMEM;

	*dma = page_pool_get_dma_addr(page) + offset;
	return 0;
}

static int __otx2_alloc_rbuf(struct otx2_nic *pfvf, struct otx2_pool *pool,
			     dma_addr_t *dma)
{
	u8 *buf;

	if (pool->page_pool)
		return otx2_alloc_pool_buf(pfvf, pool, dma);

	buf = napi_alloc_frag_align(pool->rbsize, OTX2_ALIGN);
	if (unlikely(!buf))
		return -ENOMEM;

	*dma = dma_map_single_attrs(pfvf->dev, buf, pool->rbsize,
				    DMA_FROM_DEVICE, DMA_ATTR_SKIP_CPU_SYNC);
	if (unlikely(dma_mapping_error(pfvf->dev, *dma))) {
		page_frag_free(buf);
		return -ENOMEM;
	}

	return 0;
}

static int dup_alloc_rbuf(struct otx2_nic *pfvf, struct otx2_pool *pool,
			  dma_addr_t *dma)
{
	int ret;

	local_bh_disable();
	ret = __otx2_alloc_rbuf(pfvf, pool, dma);
	local_bh_enable();
	return ret;
}

int dup_alloc_buffer(struct otx2_nic *pfvf, struct otx2_cq_queue *cq,
		     dma_addr_t *dma)
{
	if (unlikely(__otx2_alloc_rbuf(pfvf, cq->rbpool, dma)))
		return -ENOMEM;
	return 0;
}

int dup_txschq_config(struct otx2_nic *pfvf, int lvl, int prio,
		      bool txschq_for_pfc)
{
	u16 (*schq_list)[MAX_TXSCHQ_PER_FUNC];
	struct otx2_hw *hw = &pfvf->hw;
	struct nix_txschq_config *req;
	u16 schq, parent;
	u64 dwrr_val;

	dwrr_val = mtu_to_dwrr_weight(pfvf, pfvf->tx_max_pktlen);

	req = otx2_mbox_alloc_msg_nix_txschq_cfg(&pfvf->mbox);
	if (!req)
		return -ENOMEM;

	req->lvl = lvl;
	req->num_regs = 1;

	schq_list = hw->txschq_list;
#ifdef CONFIG_DCB
	if (txschq_for_pfc)
		schq_list = pfvf->pfc_schq_list;
#endif

	schq = schq_list[lvl][prio];
	/* Set topology e.t.c configuration */
	if (lvl == NIX_TXSCH_LVL_SMQ) {
		req->reg[0] = NIX_AF_SMQX_CFG(schq);
		req->regval[0] = ((u64)pfvf->tx_max_pktlen << 8) | OTX2_MIN_MTU;
		req->regval[0] |= (0x20ULL << 51) | (0x80ULL << 39) |
				  (0x2ULL << 36);
		/* Set link type for DWRR MTU selection on CN10K silicons */
		if (!is_dev_otx2(pfvf->pdev))
			req->regval[0] |= FIELD_PREP(GENMASK_ULL(58, 57),
						(u64)hw->smq_link_type);
		req->num_regs++;
		/* MDQ config */
		parent = schq_list[NIX_TXSCH_LVL_TL4][prio];
		req->reg[1] = NIX_AF_MDQX_PARENT(schq);
		req->regval[1] = parent << 16;
		req->num_regs++;
		/* Set DWRR quantum */
		req->reg[2] = NIX_AF_MDQX_SCHEDULE(schq);
		req->regval[2] =  dwrr_val;
	} else if (lvl == NIX_TXSCH_LVL_TL4) {
		int sdp_channel = hw->tx_chan_base + prio;

		/* For SDP, TL4 is the last level used, so we always just
		 * want 1 queue configured after that.
		 */
		if (is_otx2_sdpvf(pfvf->pdev))
			prio = 0;
		parent = schq_list[NIX_TXSCH_LVL_TL3][prio];
		req->reg[0] = NIX_AF_TL4X_PARENT(schq);
		req->regval[0] = (u64)parent << 16;
		req->num_regs++;
		req->reg[1] = NIX_AF_TL4X_SCHEDULE(schq);
		req->regval[1] = dwrr_val;
		if (is_otx2_sdpvf(pfvf->pdev)) {
			req->num_regs++;
			req->reg[2] = NIX_AF_TL4X_SDP_LINK_CFG(schq);
			req->regval[2] = BIT_ULL(12) | BIT_ULL(13) | (sdp_channel & 0xff);
		}
	} else if (lvl == NIX_TXSCH_LVL_TL3) {
		parent = schq_list[NIX_TXSCH_LVL_TL2][prio];
		req->reg[0] = NIX_AF_TL3X_PARENT(schq);
		req->regval[0] = (u64)parent << 16;
		req->num_regs++;
		req->reg[1] = NIX_AF_TL3X_SCHEDULE(schq);
		req->regval[1] = dwrr_val;
		if (lvl == hw->txschq_link_cfg_lvl && !is_otx2_sdpvf(pfvf->pdev)) {
			req->num_regs++;
			req->reg[2] = NIX_AF_TL3_TL2X_LINKX_CFG(schq, hw->tx_link);
			/* Enable this queue and backpressure
			 * and set relative channel
			 */
			req->regval[2] = BIT_ULL(13) | BIT_ULL(12) | prio;
		}
	} else if (lvl == NIX_TXSCH_LVL_TL2) {
		parent = schq_list[NIX_TXSCH_LVL_TL1][prio];
		req->reg[0] = NIX_AF_TL2X_PARENT(schq);
		req->regval[0] = (u64)parent << 16;

		req->num_regs++;
		req->reg[1] = NIX_AF_TL2X_SCHEDULE(schq);
		req->regval[1] = (u64)hw->txschq_aggr_lvl_rr_prio << 24 | dwrr_val;

		if (lvl == hw->txschq_link_cfg_lvl && !is_otx2_sdpvf(pfvf->pdev)) {
			req->num_regs++;
			req->reg[2] = NIX_AF_TL3_TL2X_LINKX_CFG(schq, hw->tx_link);
			/* Enable this queue and backpressure
			 * and set relative channel
			 */
			req->regval[2] = BIT_ULL(13) | BIT_ULL(12) | prio;
		}
	} else if (lvl == NIX_TXSCH_LVL_TL1) {
		/* Default config for TL1.
		 * For VF this is always ignored.
		 */

		/* On CN10K, if RR_WEIGHT is greater than 16384, HW will
		 * clip it to 16384, so configuring a 24bit max value
		 * will work on both OTx2 and CN10K.
		 */
		req->reg[0] = NIX_AF_TL1X_SCHEDULE(schq);
		req->regval[0] = TXSCH_TL1_DFLT_RR_QTM;

		req->num_regs++;
		req->reg[1] = NIX_AF_TL1X_TOPOLOGY(schq);
		req->regval[1] = hw->txschq_aggr_lvl_rr_prio << 1;

		req->num_regs++;
		req->reg[2] = NIX_AF_TL1X_CIR(schq);
		req->regval[2] = 0;
	}

	return otx2_sync_mbox_msg(&pfvf->mbox);
}

int dup_smq_flush(struct otx2_nic *pfvf, int smq)
{
	struct nix_txschq_config *req;
	int rc;

	mutex_lock(&pfvf->mbox.lock);

	req = otx2_mbox_alloc_msg_nix_txschq_cfg(&pfvf->mbox);
	if (!req) {
		mutex_unlock(&pfvf->mbox.lock);
		return -ENOMEM;
	}

	req->lvl = NIX_TXSCH_LVL_SMQ;
	req->reg[0] = NIX_AF_SMQX_CFG(smq);
	req->regval[0] |= BIT_ULL(49);
	req->num_regs++;

	rc = otx2_sync_mbox_msg(&pfvf->mbox);
	mutex_unlock(&pfvf->mbox.lock);
	return rc;
}

int dup_txsch_alloc(struct otx2_nic *pfvf)
{
	int chan_cnt = pfvf->hw.tx_chan_cnt;
	struct nix_txsch_alloc_req *req;
	struct nix_txsch_alloc_rsp *rsp;
	int lvl, schq, rc;

	/* Get memory to put this msg */
	req = otx2_mbox_alloc_msg_nix_txsch_alloc(&pfvf->mbox);
	if (!req)
		return -ENOMEM;

	/* Request one schq per level */
	for (lvl = 0; lvl < NIX_TXSCH_LVL_CNT; lvl++)
		req->schq[lvl] = 1;

	if (is_otx2_sdpvf(pfvf->pdev) && chan_cnt > 1) {
		/* For SDP, backpressure is asserted at TL4,
		 * so single scheduler queue at higher levels suffice.
		 */
		req->schq[NIX_TXSCH_LVL_SMQ] = chan_cnt;
		req->schq[NIX_TXSCH_LVL_TL4] = chan_cnt;
	}

	rc = otx2_sync_mbox_msg(&pfvf->mbox);
	if (rc)
		return rc;

	rsp = (struct nix_txsch_alloc_rsp *)
	      otx2_mbox_get_rsp(&pfvf->mbox.mbox, 0, &req->hdr);
	if (IS_ERR(rsp))
		return PTR_ERR(rsp);

	/* Setup transmit scheduler list */
	for (lvl = 0; lvl < NIX_TXSCH_LVL_CNT; lvl++) {
		pfvf->hw.txschq_cnt[lvl] = rsp->schq[lvl];
		for (schq = 0; schq < rsp->schq[lvl]; schq++)
			pfvf->hw.txschq_list[lvl][schq] =
				rsp->schq_list[lvl][schq];
	}

	pfvf->hw.txschq_link_cfg_lvl = rsp->link_cfg_lvl;
	pfvf->hw.txschq_aggr_lvl_rr_prio = rsp->aggr_lvl_rr_prio;

	return 0;
}

void dup_txschq_stop(struct otx2_nic *pfvf)
{
	struct otx2_cmn_fops *ops;
	int lvl, schq, idx;

	ops = otx2_cmn_fops_arr_lookup(pfvf->pdev->device);
	if (!ops) {
		netdev_err(pfvf->netdev, "Could not locate ops structure\n");
		return;
	}

	/* free non QOS TLx nodes */
	for (lvl = 0; lvl < NIX_TXSCH_LVL_CNT; lvl++) {
		for (idx = 0; idx < pfvf->hw.txschq_cnt[lvl]; idx++) {
			ops->tx_schq_free_one(pfvf, lvl,
					      pfvf->hw.txschq_list[lvl][idx]);
		}
	}

	/* Clear the txschq list */
	for (lvl = 0; lvl < NIX_TXSCH_LVL_CNT; lvl++) {
		for (schq = 0; schq < MAX_TXSCHQ_PER_FUNC; schq++)
			pfvf->hw.txschq_list[lvl][schq] = 0;
	}
}

void dup_sqb_flush(struct otx2_nic *pfvf)
{
	int qidx, sqe_tail, sqe_head;
	struct otx2_snd_queue *sq;
	u64 incr, *ptr, val;

	ptr = (u64 *)otx2_get_regaddr(pfvf, NIX_LF_SQ_OP_STATUS);
	for (qidx = 0; qidx < otx2_get_total_tx_queues(pfvf); qidx++) {
		sq = &pfvf->qset.sq[qidx];
		if (!sq->sqb_ptrs)
			continue;

		incr = (u64)qidx << 32;
		val = otx2_atomic64_add(incr, ptr);
		sqe_head = (val >> 20) & 0x3F;
		sqe_tail = (val >> 28) & 0x3F;
		if (sqe_head != sqe_tail)
			usleep_range(50, 60);
	}
}

/* RED and drop levels of CQ on packet reception.
 * For CQ level is measure of emptiness ( 0x0 = full, 255 = empty).
 */
#define RQ_PASS_LVL_CQ(skid, qsize)	((((skid) + 16) * 256) / (qsize))
#define RQ_DROP_LVL_CQ(skid, qsize)	(((skid) * 256) / (qsize))

/* RED and drop levels of AURA for packet reception.
 * For AURA level is measure of fullness (0x0 = empty, 255 = full).
 * Eg: For RQ length 1K, for pass/drop level 204/230.
 * RED accepts pkts if free pointers > 102 & <= 205.
 * Drops pkts if free pointers < 102.
 */
#define RQ_BP_LVL_AURA   (255 - ((85 * 256) / 100)) /* BP when 85% is full */
#define RQ_PASS_LVL_AURA (255 - ((95 * 256) / 100)) /* RED when 95% is full */
#define RQ_DROP_LVL_AURA (255 - ((99 * 256) / 100)) /* Drop when 99% is full */

int dup_rq_init(struct otx2_nic *pfvf, u16 qidx, u16 lpb_aura)
{
	struct otx2_qset *qset = &pfvf->qset;
	struct nix_aq_enq_req *aq;

	/* Get memory to put this msg */
	aq = otx2_mbox_alloc_msg_nix_aq_enq(&pfvf->mbox);
	if (!aq)
		return -ENOMEM;

	aq->rq.cq = qidx;
	aq->rq.ena = 1;
	aq->rq.pb_caching = 1;
	aq->rq.lpb_aura = lpb_aura; /* Use large packet buffer aura */
	aq->rq.lpb_sizem1 = (DMA_BUFFER_LEN(pfvf->rbsize) / 8) - 1;
	aq->rq.xqe_imm_size = 0; /* Copying of packet to CQE not needed */
	aq->rq.flow_tagw = 32; /* Copy full 32bit flow_tag to CQE header */
	aq->rq.qint_idx = 0;
	aq->rq.lpb_drop_ena = 1; /* Enable RED dropping for AURA */
	aq->rq.xqe_drop_ena = 1; /* Enable RED dropping for CQ/SSO */
	aq->rq.xqe_pass = RQ_PASS_LVL_CQ(pfvf->hw.rq_skid, qset->rqe_cnt);
	aq->rq.xqe_drop = RQ_DROP_LVL_CQ(pfvf->hw.rq_skid, qset->rqe_cnt);
	aq->rq.lpb_aura_pass = RQ_PASS_LVL_AURA;
	aq->rq.lpb_aura_drop = RQ_DROP_LVL_AURA;

	/* Fill AQ info */
	aq->qidx = qidx;
	aq->ctype = NIX_AQ_CTYPE_RQ;
	aq->op = NIX_AQ_INSTOP_INIT;

	return otx2_sync_mbox_msg(&pfvf->mbox);
}

int dup_sq_aq_init(void *dev, u16 qidx, u8 chan_offset, u16 sqb_aura)
{
	struct otx2_nic *pfvf = dev;
	struct otx2_snd_queue *sq;
	struct nix_aq_enq_req *aq;

	sq = &pfvf->qset.sq[qidx];
	sq->lmt_addr = (__force u64 *)(pfvf->reg_base + LMT_LF_LMTLINEX(qidx));
	/* Get memory to put this msg */
	aq = otx2_mbox_alloc_msg_nix_aq_enq(&pfvf->mbox);
	if (!aq)
		return -ENOMEM;

	aq->sq.cq = pfvf->hw.rx_queues + qidx;
	aq->sq.max_sqe_size = NIX_MAXSQESZ_W16; /* 128 byte */
	aq->sq.cq_ena = 1;
	aq->sq.ena = 1;
	aq->sq.smq = otx2_get_smq_idx(pfvf, qidx);
	aq->sq.smq_rr_quantum = mtu_to_dwrr_weight(pfvf, pfvf->tx_max_pktlen);
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

static void otx2_pool_refill_task(struct work_struct *work)
{
	struct otx2_cq_queue *cq;
	struct refill_work *wrk;
	struct otx2_nic *pfvf;
	int qidx;

	wrk = container_of(work, struct refill_work, pool_refill_work.work);
	pfvf = wrk->pf;
	qidx = wrk - pfvf->refill_wrk;
	cq = &pfvf->qset.cq[qidx];

	cq->refill_task_sched = false;

	local_bh_disable();
	napi_schedule(wrk->napi);
	local_bh_enable();
}

int dup_config_nix_queues(struct otx2_nic *pfvf)
{
	struct otx2_cmn_fops *ops;
	int qidx, err;

	ops = otx2_cmn_fops_arr_lookup(pfvf->pdev->device);
	if (!ops) {
		netdev_err(pfvf->netdev, "Could not locate ops structure\n");
		return -EINVAL;
	}

	/* Initialize RX queues */
	for (qidx = 0; qidx < pfvf->hw.rx_queues; qidx++) {
		u16 lpb_aura = otx2_get_pool_idx(pfvf, AURA_NIX_RQ, qidx);

		err = ops->rq_init(pfvf, qidx, lpb_aura);
		if (err)
			return err;
	}

	/* Initialize TX queues */
	for (qidx = 0; qidx < pfvf->hw.non_qos_queues; qidx++) {
		u16 sqb_aura = otx2_get_pool_idx(pfvf, AURA_NIX_SQ, qidx);

		err = ops->sq_init(pfvf, qidx, sqb_aura);
		if (err)
			return err;
	}

	/* Initialize completion queues */
	for (qidx = 0; qidx < pfvf->qset.cq_cnt; qidx++) {
		err = ops->cq_init(pfvf, qidx, 0);
		if (err)
			return err;
	}

	pfvf->cq_op_addr = (__force u64 *)otx2_get_regaddr(pfvf,
							   NIX_LF_CQ_OP_STATUS);

	/* Initialize work queue for receive buffer refill */
	pfvf->refill_wrk = devm_kcalloc(pfvf->dev, pfvf->qset.cq_cnt,
					sizeof(struct refill_work), GFP_KERNEL);
	if (!pfvf->refill_wrk)
		return -ENOMEM;

	for (qidx = 0; qidx < pfvf->qset.cq_cnt; qidx++) {
		pfvf->refill_wrk[qidx].pf = pfvf;
		INIT_DELAYED_WORK(&pfvf->refill_wrk[qidx].pool_refill_work,
				  otx2_pool_refill_task);
	}
	return 0;
}

int dup_config_nix(struct otx2_nic *pfvf)
{
	struct nix_lf_alloc_req  *nixlf;
	struct nix_lf_alloc_rsp *rsp;
	int err;

	pfvf->qset.xqe_size = pfvf->hw.xqe_size;

	/* Get memory to put this msg */
	nixlf = otx2_mbox_alloc_msg_nix_lf_alloc(&pfvf->mbox);
	if (!nixlf)
		return -ENOMEM;

	/* Set RQ/SQ/CQ counts */
	nixlf->rq_cnt = pfvf->hw.rx_queues;
	nixlf->sq_cnt = otx2_get_total_tx_queues(pfvf);
	nixlf->cq_cnt = pfvf->qset.cq_cnt;
	nixlf->rss_sz = MAX_RSS_INDIR_TBL_SIZE;
	nixlf->rss_grps = MAX_RSS_GROUPS;
	nixlf->xqe_sz = pfvf->hw.xqe_size == 128 ? NIX_XQESZ_W16 : NIX_XQESZ_W64;
	/* We don't know absolute NPA LF idx attached.
	 * AF will replace 'RVU_DEFAULT_PF_FUNC' with
	 * NPA LF attached to this RVU PF/VF.
	 */
	nixlf->npa_func = RVU_DEFAULT_PF_FUNC;
	/* Disable alignment pad, enable L2 length check,
	 * enable L4 TCP/UDP checksum verification.
	 */
	nixlf->rx_cfg = BIT_ULL(33) | BIT_ULL(35) | BIT_ULL(37);

	err = otx2_sync_mbox_msg(&pfvf->mbox);
	if (err)
		return err;

	rsp = (struct nix_lf_alloc_rsp *)otx2_mbox_get_rsp(&pfvf->mbox.mbox, 0,
							   &nixlf->hdr);
	if (IS_ERR(rsp))
		return PTR_ERR(rsp);

	if (rsp->qints < 1)
		return -ENXIO;

	return rsp->hdr.rc;
}

void dup_sq_free_sqbs(struct otx2_nic *pfvf)
{
	struct otx2_qset *qset = &pfvf->qset;
	struct otx2_hw *hw = &pfvf->hw;
	struct otx2_snd_queue *sq;
	int sqb, qidx;
	u64 iova, pa;

	for (qidx = 0; qidx < otx2_get_total_tx_queues(pfvf); qidx++) {
		sq = &qset->sq[qidx];
		if (!sq->sqb_ptrs)
			continue;
		for (sqb = 0; sqb < sq->sqb_count; sqb++) {
			if (!sq->sqb_ptrs[sqb])
				continue;
			iova = sq->sqb_ptrs[sqb];
			pa = otx2_iova_to_phys(pfvf->iommu_domain, iova);
			dma_unmap_page_attrs(pfvf->dev, iova, hw->sqb_size,
					     DMA_FROM_DEVICE,
					     DMA_ATTR_SKIP_CPU_SYNC);
			if (page_ref_count(virt_to_head_page(phys_to_virt(pa))))
				page_frag_free(phys_to_virt(pa));
		}
		sq->sqb_count = 0;
	}
}

void dup_free_bufs(struct otx2_nic *pfvf, struct otx2_pool *pool,
		   u64 iova, int size)
{
	struct page *page;
	u64 pa;

	pa = otx2_iova_to_phys(pfvf->iommu_domain, iova);
	page = virt_to_head_page(phys_to_virt(pa));

	if (pool->page_pool) {
		page_pool_put_full_page(pool->page_pool, page, true);
	} else {
		dma_unmap_page_attrs(pfvf->dev, iova, size,
				     DMA_FROM_DEVICE,
				     DMA_ATTR_SKIP_CPU_SYNC);

		if (page_ref_count(page))
			page_frag_free(page);

		put_page(page);
	}
}

void dup_otx2_free_aura_ptr(struct otx2_nic *pfvf, int type)
{
	int pool_id, pool_start = 0, pool_end = 0, size = 0;
	struct otx2_pool *pool;
	u64 iova;

	if (type == AURA_NIX_SQ) {
		pool_start = otx2_get_pool_idx(pfvf, type, 0);
		pool_end =  pool_start + pfvf->hw.sqpool_cnt;
		size = pfvf->hw.sqb_size;
	}
	if (type == AURA_NIX_RQ) {
		pool_start = otx2_get_pool_idx(pfvf, type, 0);
		pool_end = pfvf->hw.rqpool_cnt;
		size = pfvf->rbsize;
	}

	/* Free SQB and RQB pointers from the aura pool */
	for (pool_id = pool_start; pool_id < pool_end; pool_id++) {
		iova = otx2_aura_allocptr(pfvf, pool_id);
		pool = &pfvf->qset.pool[pool_id];
		while (iova) {
			if (type == AURA_NIX_RQ)
				iova -= OTX2_HEAD_ROOM;

			dup_free_bufs(pfvf, pool, iova, size);

			iova = otx2_aura_allocptr(pfvf, pool_id);
		}
	}
}

void dup_aura_pool_free(struct otx2_nic *pfvf)
{
	struct otx2_pool *pool;
	int pool_id;

	if (!pfvf->qset.pool)
		return;

	for (pool_id = 0; pool_id < pfvf->hw.pool_cnt; pool_id++) {
		pool = &pfvf->qset.pool[pool_id];
		qmem_free(pfvf->dev, pool->stack);
		qmem_free(pfvf->dev, pool->fc_addr);
		page_pool_destroy(pool->page_pool);
		pool->page_pool = NULL;
	}
	devm_kfree(pfvf->dev, pfvf->qset.pool);
	pfvf->qset.pool = NULL;
}

static int dup_aura_init(struct otx2_nic *pfvf, int aura_id,
			 int pool_id, int numptrs)
{
	struct npa_aq_enq_req *aq;
	struct otx2_pool *pool;
	int err;

	pool = &pfvf->qset.pool[pool_id];

	/* Allocate memory for HW to update Aura count.
	 * Alloc one cache line, so that it fits all FC_STYPE modes.
	 */
	if (!pool->fc_addr) {
		err = qmem_alloc(pfvf->dev, &pool->fc_addr, 1, OTX2_ALIGN);
		if (err)
			return err;
	}

	/* Initialize this aura's context via AF */
	aq = otx2_mbox_alloc_msg_npa_aq_enq(&pfvf->mbox);
	if (!aq) {
		/* Shared mbox memory buffer is full, flush it and retry */
		err = otx2_sync_mbox_msg(&pfvf->mbox);
		if (err)
			return err;
		aq = otx2_mbox_alloc_msg_npa_aq_enq(&pfvf->mbox);
		if (!aq)
			return -ENOMEM;
	}

	aq->aura_id = aura_id;
	/* Will be filled by AF with correct pool context address */
	aq->aura.pool_addr = pool_id;
	aq->aura.pool_caching = 1;
	aq->aura.shift = ilog2(numptrs) - 8;
	aq->aura.count = numptrs;
	aq->aura.limit = numptrs;
	aq->aura.avg_level = 255;
	aq->aura.ena = 1;
	aq->aura.fc_ena = 1;
	aq->aura.fc_addr = pool->fc_addr->iova;
	aq->aura.fc_hyst_bits = 0; /* Store count on all updates */

	/* Enable backpressure for RQ aura */
	if (aura_id < pfvf->hw.rqpool_cnt && !is_otx2_lbkvf(pfvf->pdev)) {
		aq->aura.bp_ena = 0;
		/* If NIX1 LF is attached then specify NIX1_RX.
		 *
		 * Below NPA_AURA_S[BP_ENA] is set according to the
		 * NPA_BPINTF_E enumeration given as:
		 * 0x0 + a*0x1 where 'a' is 0 for NIX0_RX and 1 for NIX1_RX so
		 * NIX0_RX is 0x0 + 0*0x1 = 0
		 * NIX1_RX is 0x0 + 1*0x1 = 1
		 * But in HRM it is given that
		 * "NPA_AURA_S[BP_ENA](w1[33:32]) - Enable aura backpressure to
		 * NIX-RX based on [BP] level. One bit per NIX-RX; index
		 * enumerated by NPA_BPINTF_E."
		 */
		if (pfvf->nix_blkaddr == BLKADDR_NIX1)
			aq->aura.bp_ena = 1;
#ifdef CONFIG_DCB
		aq->aura.nix0_bpid = pfvf->bpid[pfvf->queue_to_pfc_map[aura_id]];
#else
		aq->aura.nix0_bpid = pfvf->bpid[0];
#endif

		/* Set backpressure level for RQ's Aura */
		aq->aura.bp = RQ_BP_LVL_AURA;
	}

	/* Fill AQ info */
	aq->ctype = NPA_AQ_CTYPE_AURA;
	aq->op = NPA_AQ_INSTOP_INIT;

	return 0;
}

static int dup_pool_init(struct otx2_nic *pfvf, u16 pool_id,
			 int stack_pages, int numptrs, int buf_size, int type)
{
	struct page_pool_params pp_params = { 0 };
	struct npa_aq_enq_req *aq;
	struct otx2_pool *pool;
	int err, sz;

	pool = &pfvf->qset.pool[pool_id];
	/* Alloc memory for stack which is used to store buffer pointers */
	err = qmem_alloc(pfvf->dev, &pool->stack,
			 stack_pages, pfvf->hw.stack_pg_bytes);
	if (err)
		return err;

	pool->rbsize = buf_size;

	/* Initialize this pool's context via AF */
	aq = otx2_mbox_alloc_msg_npa_aq_enq(&pfvf->mbox);
	if (!aq) {
		/* Shared mbox memory buffer is full, flush it and retry */
		err = otx2_sync_mbox_msg(&pfvf->mbox);
		if (err) {
			qmem_free(pfvf->dev, pool->stack);
			return err;
		}
		aq = otx2_mbox_alloc_msg_npa_aq_enq(&pfvf->mbox);
		if (!aq) {
			qmem_free(pfvf->dev, pool->stack);
			return -ENOMEM;
		}
	}

	aq->aura_id = pool_id;
	aq->pool.stack_base = pool->stack->iova;
	aq->pool.stack_caching = 1;
	aq->pool.ena = 1;
	aq->pool.buf_size = buf_size / 128;
	aq->pool.stack_max_pages = stack_pages;
	aq->pool.shift = ilog2(numptrs) - 8;
	aq->pool.ptr_start = 0;
	aq->pool.ptr_end = ~0ULL;

	/* Fill AQ info */
	aq->ctype = NPA_AQ_CTYPE_POOL;
	aq->op = NPA_AQ_INSTOP_INIT;

	if (type != AURA_NIX_RQ) {
		pool->page_pool = NULL;
		return 0;
	}

	sz = ALIGN(ALIGN(SKB_DATA_ALIGN(buf_size), OTX2_ALIGN), PAGE_SIZE);
	pp_params.order = (sz / PAGE_SIZE) - 1;
	pp_params.flags = PP_FLAG_PAGE_FRAG | PP_FLAG_DMA_MAP;
	pp_params.pool_size = min(OTX2_PAGE_POOL_SZ, numptrs);
	pp_params.nid = NUMA_NO_NODE;
	pp_params.dev = pfvf->dev;
	pp_params.dma_dir = DMA_FROM_DEVICE;
	pool->page_pool = page_pool_create(&pp_params);
	if (IS_ERR(pool->page_pool)) {
		netdev_err(pfvf->netdev, "Creation of page pool failed\n");
		return PTR_ERR(pool->page_pool);
	}

	return 0;
}

int dup_sq_aura_pool_init(struct otx2_nic *pfvf)
{
	int qidx, pool_id, stack_pages, num_sqbs;
	struct otx2_qset *qset = &pfvf->qset;
	struct otx2_hw *hw = &pfvf->hw;
	struct otx2_snd_queue *sq;
	struct otx2_pool *pool;
	dma_addr_t bufptr;
	int err, ptr;

	/* Calculate number of SQBs needed.
	 *
	 * For a 128byte SQE, and 4K size SQB, 31 SQEs will fit in one SQB.
	 * Last SQE is used for pointing to next SQB.
	 */
	num_sqbs = (hw->sqb_size / 128) - 1;
	num_sqbs = (qset->sqe_cnt + num_sqbs) / num_sqbs;

	/* Get no of stack pages needed */
	stack_pages =
		(num_sqbs + hw->stack_pg_ptrs - 1) / hw->stack_pg_ptrs;

	for (qidx = 0; qidx < hw->non_qos_queues; qidx++) {
		pool_id = otx2_get_pool_idx(pfvf, AURA_NIX_SQ, qidx);
		/* Initialize aura context */
		err = dup_aura_init(pfvf, pool_id, pool_id, num_sqbs);
		if (err)
			goto fail;

		/* Initialize pool context */
		err = dup_pool_init(pfvf, pool_id, stack_pages,
				    num_sqbs, hw->sqb_size, AURA_NIX_SQ);
		if (err)
			goto fail;
	}

	/* Flush accumulated messages */
	err = otx2_sync_mbox_msg(&pfvf->mbox);
	if (err)
		goto fail;

	/* Allocate pointers and free them to aura/pool */
	for (qidx = 0; qidx < hw->non_qos_queues; qidx++) {
		pool_id = otx2_get_pool_idx(pfvf, AURA_NIX_SQ, qidx);
		pool = &pfvf->qset.pool[pool_id];

		sq = &qset->sq[qidx];
		sq->sqb_count = 0;
		sq->sqb_ptrs = kcalloc(num_sqbs, sizeof(*sq->sqb_ptrs), GFP_KERNEL);
		if (!sq->sqb_ptrs) {
			err = -ENOMEM;
			goto err_mem;
		}

		for (ptr = 0; ptr < num_sqbs; ptr++) {
			err = dup_alloc_rbuf(pfvf, pool, &bufptr);
			if (err)
				goto err_mem;
			pfvf->hw_ops->aura_freeptr(pfvf, pool_id, bufptr);
			sq->sqb_ptrs[sq->sqb_count++] = (u64)bufptr;
		}
	}

err_mem:
	return err ? -ENOMEM : 0;

fail:
	otx2_mbox_reset(&pfvf->mbox.mbox, 0);
	dup_aura_pool_free(pfvf);
	return err;
}

int dup_rq_aura_pool_init(struct otx2_nic *pfvf)
{
	struct otx2_hw *hw = &pfvf->hw;
	int stack_pages, pool_id, rq;
	struct otx2_pool *pool;
	int err, ptr, num_ptrs;
	dma_addr_t bufptr;

	num_ptrs = pfvf->qset.rqe_cnt;

	stack_pages =
		(num_ptrs + hw->stack_pg_ptrs - 1) / hw->stack_pg_ptrs;

	for (rq = 0; rq < hw->rx_queues; rq++) {
		pool_id = otx2_get_pool_idx(pfvf, AURA_NIX_RQ, rq);
		/* Initialize aura context */
		err = dup_aura_init(pfvf, pool_id, pool_id, num_ptrs);
		if (err)
			goto fail;
	}
	for (pool_id = 0; pool_id < hw->rqpool_cnt; pool_id++) {
		err = dup_pool_init(pfvf, pool_id, stack_pages,
				    num_ptrs, pfvf->rbsize, AURA_NIX_RQ);
		if (err)
			goto fail;
	}

	/* Flush accumulated messages */
	err = otx2_sync_mbox_msg(&pfvf->mbox);
	if (err)
		goto fail;

	/* Allocate pointers and free them to aura/pool */
	for (pool_id = 0; pool_id < hw->rqpool_cnt; pool_id++) {
		pool = &pfvf->qset.pool[pool_id];
		for (ptr = 0; ptr < num_ptrs; ptr++) {
			err = dup_alloc_rbuf(pfvf, pool, &bufptr);
			if (err)
				return -ENOMEM;
			pfvf->hw_ops->aura_freeptr(pfvf, pool_id,
						   bufptr + OTX2_HEAD_ROOM);
		}
	}
	return 0;
fail:
	otx2_mbox_reset(&pfvf->mbox.mbox, 0);
	dup_aura_pool_free(pfvf);
	return err;
}

int dup_config_npa(struct otx2_nic *pfvf)
{
	struct otx2_qset *qset = &pfvf->qset;
	struct npa_lf_alloc_req  *npalf;
	struct otx2_hw *hw = &pfvf->hw;
	int aura_cnt;

	/* Pool - Stack of free buffer pointers
	 * Aura - Alloc/frees pointers from/to pool for NIX DMA.
	 */

	if (!hw->pool_cnt)
		return -EINVAL;

	qset->pool = devm_kcalloc(pfvf->dev, hw->pool_cnt,
				  sizeof(struct otx2_pool), GFP_KERNEL);
	if (!qset->pool)
		return -ENOMEM;

	/* Get memory to put this msg */
	npalf = otx2_mbox_alloc_msg_npa_lf_alloc(&pfvf->mbox);
	if (!npalf)
		return -ENOMEM;

	/* Set aura and pool counts */
	npalf->nr_pools = hw->pool_cnt;
	aura_cnt = ilog2(roundup_pow_of_two(hw->pool_cnt));
	npalf->aura_sz = (aura_cnt >= ilog2(128)) ? (aura_cnt - 6) : 1;

	return otx2_sync_mbox_msg(&pfvf->mbox);
}

int dup_detach_resources(struct mbox *mbox)
{
	struct rsrc_detach *detach;

	mutex_lock(&mbox->lock);
	detach = otx2_mbox_alloc_msg_detach_resources(mbox);
	if (!detach) {
		mutex_unlock(&mbox->lock);
		return -ENOMEM;
	}

	/* detach all */
	detach->partial = false;

	/* Send detach request to AF */
	otx2_sync_mbox_msg(mbox);
	mutex_unlock(&mbox->lock);
	return 0;
}

int dup_attach_npa_nix(struct otx2_nic *pfvf)
{
	struct rsrc_attach *attach;
	struct msg_req *msix;
	int err;

	mutex_lock(&pfvf->mbox.lock);
	/* Get memory to put this msg */
	attach = otx2_mbox_alloc_msg_attach_resources(&pfvf->mbox);
	if (!attach) {
		mutex_unlock(&pfvf->mbox.lock);
		return -ENOMEM;
	}

	attach->npalf = true;
	attach->nixlf = true;

	/* Send attach request to AF */
	err = otx2_sync_mbox_msg(&pfvf->mbox);
	if (err) {
		mutex_unlock(&pfvf->mbox.lock);
		return err;
	}

	pfvf->nix_blkaddr = BLKADDR_NIX0;

	/* If the platform has two NIX blocks then LF may be
	 * allocated from NIX1.
	 */
	if (otx2_read64(pfvf, RVU_PF_BLOCK_ADDRX_DISC(BLKADDR_NIX1)) & 0x1FFULL)
		pfvf->nix_blkaddr = BLKADDR_NIX1;

	/* Get NPA and NIX MSIX vector offsets */
	msix = otx2_mbox_alloc_msg_msix_offset(&pfvf->mbox);
	if (!msix) {
		mutex_unlock(&pfvf->mbox.lock);
		return -ENOMEM;
	}

	err = otx2_sync_mbox_msg(&pfvf->mbox);
	if (err) {
		mutex_unlock(&pfvf->mbox.lock);
		return err;
	}
	mutex_unlock(&pfvf->mbox.lock);

	if (pfvf->hw.npa_msixoff == MSIX_VECTOR_INVALID ||
	    pfvf->hw.nix_msixoff == MSIX_VECTOR_INVALID) {
		dev_err(pfvf->dev,
			"RVUPF: Invalid MSIX vector offset for NPA/NIX\n");
		return -EINVAL;
	}

	return 0;
}

void dup_ctx_disable(struct mbox *mbox, int type, bool npa)
{
	struct hwctx_disable_req *req;

	mutex_lock(&mbox->lock);
	/* Request AQ to disable this context */
	if (npa)
		req = otx2_mbox_alloc_msg_npa_hwctx_disable(mbox);
	else
		req = otx2_mbox_alloc_msg_nix_hwctx_disable(mbox);

	if (!req) {
		mutex_unlock(&mbox->lock);
		return;
	}

	req->ctype = type;

	if (otx2_sync_mbox_msg(mbox))
		dev_err(mbox->pfvf->dev, "%s failed to disable context\n",
			__func__);

	mutex_unlock(&mbox->lock);
}

int dup_nix_config_bp(struct otx2_nic *pfvf, bool enable)
{
	struct nix_bp_cfg_req *req;

	if (enable)
		req = otx2_mbox_alloc_msg_nix_bp_enable(&pfvf->mbox);
	else
		req = otx2_mbox_alloc_msg_nix_bp_disable(&pfvf->mbox);

	if (!req)
		return -ENOMEM;

	req->chan_base = 0;
#ifdef CONFIG_DCB
	req->chan_cnt = pfvf->pfc_en ? IEEE_8021QAZ_MAX_TCS : pfvf->hw.rx_chan_cnt;
	req->bpid_per_chan = pfvf->pfc_en ? 1 : 0;
#else
	req->chan_cnt =  pfvf->hw.rx_chan_cnt;
	req->bpid_per_chan = 0;
#endif

	return otx2_sync_mbox_msg(&pfvf->mbox);
}

/* Mbox message handlers */
void dup_mbox_handler_cgx_stats(struct otx2_nic *pfvf,
				struct cgx_stats_rsp *rsp)
{
	int id;

	for (id = 0; id < CGX_RX_STATS_COUNT; id++)
		pfvf->hw.cgx_rx_stats[id] = rsp->rx_stats[id];
	for (id = 0; id < CGX_TX_STATS_COUNT; id++)
		pfvf->hw.cgx_tx_stats[id] = rsp->tx_stats[id];
}

void dup_mbox_handler_cgx_fec_stats(struct otx2_nic *pfvf,
				    struct cgx_fec_stats_rsp *rsp)
{
	pfvf->hw.cgx_fec_corr_blks += rsp->fec_corr_blks;
	pfvf->hw.cgx_fec_uncorr_blks += rsp->fec_uncorr_blks;
}

void dup_mbox_handler_npa_lf_alloc(struct otx2_nic *pfvf,
				   struct npa_lf_alloc_rsp *rsp)
{
	pfvf->hw.stack_pg_ptrs = rsp->stack_pg_ptrs;
	pfvf->hw.stack_pg_bytes = rsp->stack_pg_bytes;
}

void dup_mbox_handler_nix_lf_alloc(struct otx2_nic *pfvf,
				   struct nix_lf_alloc_rsp *rsp)
{
	pfvf->hw.sqb_size = rsp->sqb_size;
	pfvf->hw.rx_chan_base = rsp->rx_chan_base;
	pfvf->hw.tx_chan_base = rsp->tx_chan_base;
	pfvf->hw.rx_chan_cnt = rsp->rx_chan_cnt;
	pfvf->hw.tx_chan_cnt = rsp->tx_chan_cnt;
	pfvf->hw.lso_tsov4_idx = rsp->lso_tsov4_idx;
	pfvf->hw.lso_tsov6_idx = rsp->lso_tsov6_idx;
	pfvf->hw.cgx_links = rsp->cgx_links;
	pfvf->hw.lbk_links = rsp->lbk_links;
	pfvf->hw.tx_link = rsp->tx_link;
}

void dup_mbox_handler_nix_bp_enable(struct otx2_nic *pfvf,
				    struct nix_bp_cfg_rsp *rsp)
{
	int chan, chan_id;

	for (chan = 0; chan < rsp->chan_cnt; chan++) {
		chan_id = ((rsp->chan_bpid[chan] >> 10) & 0x7F);
		pfvf->bpid[chan_id] = rsp->chan_bpid[chan] & 0x3FF;
	}
}

void dup_set_cints_affinity(struct otx2_nic *pfvf)
{
	struct otx2_hw *hw = &pfvf->hw;
	int vec, cpu, irq, cint;

	vec = hw->nix_msixoff + NIX_LF_CINT_VEC_START;
	cpu = cpumask_first(cpu_online_mask);

	/* CQ interrupts */
	for (cint = 0; cint < pfvf->hw.cint_cnt; cint++, vec++) {
		if (!alloc_cpumask_var(&hw->affinity_mask[vec], GFP_KERNEL))
			return;

		cpumask_set_cpu(cpu, hw->affinity_mask[vec]);

		irq = pci_irq_vector(pfvf->pdev, vec);
		irq_set_affinity_hint(irq, hw->affinity_mask[vec]);

		cpu = cpumask_next(cpu, cpu_online_mask);
		if (unlikely(cpu >= nr_cpu_ids))
			cpu = 0;
	}
}
