// SPDX-License-Identifier: GPL-2.0
/* Marvell RVU Ethernet driver
 *
 * Copyright (C) 2025 Marvell.
 *
 */

#include <linux/etherdevice.h>
#include <net/ip.h>
#include <net/tso.h>
#include <linux/bpf.h>
#include <linux/bpf_trace.h>
#include <net/ip6_checksum.h>

#include "otx2_reg.h"
#include "otx2_common.h"
#include "otx2_struct.h"
#include "../../nic/otx2_txrx.h"
#include "otx2_ptp.h"
#include "cn10k.h"
#include "hw/otx2_cmn.h"

#define CQE_ADDR(CQ, idx) ((CQ)->cqe_base + ((CQ)->cqe_size * (idx)))
#define PTP_PORT	        0x13F
/* PTPv2 header Original Timestamp starts at byte offset 34 and
 * contains 6 byte seconds field and 4 byte nano seconds field.
 */
#define PTP_SYNC_SEC_OFFSET	34

int dup_nix_cq_op_status(struct otx2_nic *pfvf,
			 struct otx2_cq_queue *cq)
{
	u64 incr = (u64)(cq->cq_idx) << 32;
	u64 status;

	status = otx2_atomic64_fetch_add(incr, pfvf->cq_op_addr);

	if (unlikely(status & BIT_ULL(CQ_OP_STAT_OP_ERR) ||
		     status & BIT_ULL(CQ_OP_STAT_CQ_ERR))) {
		dev_err(pfvf->dev, "CQ stopped due to error");
		return -EINVAL;
	}

	cq->cq_tail = status & 0xFFFFF;
	cq->cq_head = (status >> 20) & 0xFFFFF;
	if (cq->cq_tail < cq->cq_head)
		cq->pend_cqe = (cq->cqe_cnt - cq->cq_head) +
				cq->cq_tail;
	else
		cq->pend_cqe = cq->cq_tail - cq->cq_head;

	return 0;
}

struct nix_cqe_hdr_s *dup_get_next_cqe(struct otx2_cq_queue *cq)
{
	struct nix_cqe_hdr_s *cqe_hdr;

	cqe_hdr = (struct nix_cqe_hdr_s *)CQE_ADDR(cq, cq->cq_head);
	if (cqe_hdr->cqe_type == NIX_XQE_TYPE_INVALID)
		return NULL;

	cq->cq_head++;
	cq->cq_head &= (cq->cqe_cnt - 1);

	return cqe_hdr;
}

static void dup_dma_unmap_skb_frags(struct otx2_nic *pfvf, struct sg_list *sg)
{
	int seg;

	for (seg = 0; seg < sg->num_segs; seg++) {
		otx2_dma_unmap_page(pfvf, sg->dma_addr[seg],
				    sg->size[seg], DMA_TO_DEVICE);
	}
	sg->num_segs = 0;
}

void dup_set_taginfo(struct nix_rx_parse_s *parse,
		     struct sk_buff *skb)
{
	/* Check if VLAN is present, captured and stripped from packet */
	if (parse->vtag0_valid && parse->vtag0_gone) {
		skb_frag_t *frag0 = &skb_shinfo(skb)->frags[0];

		/* Is the tag captured STAG or CTAG ? */
		if (((struct ethhdr *)skb_frag_address(frag0))->h_proto ==
		    htons(ETH_P_8021Q))
			__vlan_hwaccel_put_tag(skb, htons(ETH_P_8021AD),
					       parse->vtag0_tci);
		else
			__vlan_hwaccel_put_tag(skb, htons(ETH_P_8021Q),
					       parse->vtag0_tci);
	}
}

bool otx2_skb_add_frag(struct otx2_nic *pfvf, struct sk_buff *skb,
		       u64 iova, int len, struct nix_rx_parse_s *parse,
		       int qidx)
{
	struct page *page;
	int off = 0;
	void *va;

	va = phys_to_virt(otx2_iova_to_phys(pfvf->iommu_domain, iova));

	if (likely(!skb_shinfo(skb)->nr_frags)) {
		/* Check if data starts at some nonzero offset
		 * from the start of the buffer.  For now the
		 * only possible offset is 8 bytes in the case
		 * where packet is prepended by a timestamp.
		 */
		if (parse->laptr) {
//			otx2_set_rxtstamp(pfvf, skb, va);
			off = OTX2_HW_TIMESTAMP_LEN;
		}
		off += pfvf->xtra_hdr;
	}

	page = virt_to_page(va);
	if (likely(skb_shinfo(skb)->nr_frags < MAX_SKB_FRAGS)) {
		skb_add_rx_frag(skb, skb_shinfo(skb)->nr_frags, page,
				va - page_address(page) + off,
				len - off, pfvf->rbsize);
		return true;
	}

	/* If more than MAX_SKB_FRAGS fragments are received then
	 * give back those buffer pointers to hardware for reuse.
	 */
	pfvf->hw_ops->aura_freeptr(pfvf, qidx, iova & ~0x07ULL);

	return false;
}

void dup_set_rxhash(struct otx2_nic *pfvf,
		    struct nix_cqe_rx_s *cqe, struct sk_buff *skb)
{
	enum pkt_hash_types hash_type = PKT_HASH_TYPE_NONE;
	struct otx2_rss_info *rss;
	u32 hash = 0;

	if (!(pfvf->netdev->features & NETIF_F_RXHASH))
		return;

	rss = &pfvf->hw.rss_info;
	if (rss->flowkey_cfg) {
		if (rss->flowkey_cfg &
		    ~(NIX_FLOW_KEY_TYPE_IPV4 | NIX_FLOW_KEY_TYPE_IPV6))
			hash_type = PKT_HASH_TYPE_L4;
		else
			hash_type = PKT_HASH_TYPE_L3;
		hash = cqe->hdr.flow_tag;
	}
	skb_set_hash(skb, hash, hash_type);
}

static void otx2_free_rcv_seg(struct otx2_nic *pfvf, struct nix_cqe_rx_s *cqe,
			      int qidx)
{
	struct nix_rx_sg_s *sg = &cqe->sg;
	void *end, *start;
	u64 *seg_addr;
	int seg;

	start = (void *)sg;
	end = start + ((cqe->parse.desc_sizem1 + 1) * 16);
	while (start < end) {
		sg = (struct nix_rx_sg_s *)start;
		seg_addr = &sg->seg_addr;
		for (seg = 0; seg < sg->segs; seg++, seg_addr++) {
			if (unlikely(!seg_addr))
				return;
			pfvf->hw_ops->aura_freeptr(pfvf, qidx,
						   *seg_addr & ~0x07ULL);
		}
		start += sizeof(*sg);
	}
}

bool dup_check_rcv_errors(struct otx2_nic *pfvf,
			  struct nix_cqe_rx_s *cqe, int qidx)
{
	struct otx2_drv_stats *stats = &pfvf->hw.drv_stats;
	struct nix_rx_parse_s *parse = &cqe->parse;

	if (netif_msg_rx_err(pfvf))
		netdev_err(pfvf->netdev,
			   "RQ%d: Error pkt with errlev:0x%x errcode:0x%x\n",
			   qidx, parse->errlev, parse->errcode);

	if (parse->errlev == NPC_ERRLVL_RE) {
		switch (parse->errcode) {
		case ERRCODE_FCS:
		case ERRCODE_FCS_RCV:
			atomic_inc(&stats->rx_fcs_errs);
			break;
		case ERRCODE_UNDERSIZE:
			atomic_inc(&stats->rx_undersize_errs);
			break;
		case ERRCODE_OVERSIZE:
			atomic_inc(&stats->rx_oversize_errs);
			break;
		case ERRCODE_OL2_LEN_MISMATCH:
			atomic_inc(&stats->rx_len_errs);
			break;
		default:
			atomic_inc(&stats->rx_other_errs);
			break;
		}
	} else if (parse->errlev == NPC_ERRLVL_NIX) {
		switch (parse->errcode) {
		case ERRCODE_OL3_LEN:
		case ERRCODE_OL4_LEN:
		case ERRCODE_IL3_LEN:
		case ERRCODE_IL4_LEN:
			atomic_inc(&stats->rx_len_errs);
			break;
		case ERRCODE_OL4_CSUM:
		case ERRCODE_IL4_CSUM:
			atomic_inc(&stats->rx_csum_errs);
			break;
		default:
			atomic_inc(&stats->rx_other_errs);
			break;
		}
	} else {
		atomic_inc(&stats->rx_other_errs);
		/* For now ignore all the NPC parser errors and
		 * pass the packets to stack.
		 */
		return false;
	}

	/* If RXALL is enabled pass on packets to stack. */
	if (pfvf->netdev->features & NETIF_F_RXALL)
		return false;

	/* Free buffer back to pool */
	if (cqe->sg.segs)
		otx2_free_rcv_seg(pfvf, cqe, qidx);
	return true;
}

int dup_refill_pool_ptrs(void *dev, struct otx2_cq_queue *cq)
{
	struct otx2_nic *pfvf = dev;
	int cnt = cq->pool_ptrs;
	dma_addr_t bufptr;

	while (cq->pool_ptrs) {
		if (dup_alloc_buffer(pfvf, cq, &bufptr))
			break;
		otx2_aura_freeptr(pfvf, cq->cq_idx, bufptr + OTX2_HEAD_ROOM);
		cq->pool_ptrs--;
	}

	return cnt - cq->pool_ptrs;
}

void dup_sqe_flush(void *dev, struct otx2_snd_queue *sq,
		   int size, int qidx)
{
	u64 status;

	/* Packet data stores should finish before SQE is flushed to HW */
	dma_wmb();

	do {
		memcpy(sq->lmt_addr, sq->sqe_base, size);
		status = otx2_lmt_flush(sq->io_addr);
	} while (status == 0);

	sq->head++;
	sq->head &= (sq->sqe_cnt - 1);
}

void dup_cleanup_rx_cqes(struct otx2_nic *pfvf, struct otx2_cq_queue *cq, int qidx)
{
	struct nix_cqe_rx_s *cqe;
	struct otx2_pool *pool;
	int processed_cqe = 0;
	u16 pool_id;
	u64 iova;

	if (pfvf->xdp_prog)
		xdp_rxq_info_unreg(&cq->xdp_rxq);

	if (dup_nix_cq_op_status(pfvf, cq) || !cq->pend_cqe)
		return;

	pool_id = otx2_get_pool_idx(pfvf, AURA_NIX_RQ, qidx);
	pool = &pfvf->qset.pool[pool_id];

	while (cq->pend_cqe) {
		cqe = (struct nix_cqe_rx_s *)dup_get_next_cqe(cq);
		processed_cqe++;
		cq->pend_cqe--;

		if (!cqe)
			continue;
		if (cqe->sg.segs > 1) {
			otx2_free_rcv_seg(pfvf, cqe, cq->cq_idx);
			continue;
		}
		iova = cqe->sg.seg_addr - OTX2_HEAD_ROOM;

		dup_free_bufs(pfvf, pool, iova, pfvf->rbsize);
	}

	/* Free CQEs to HW */
	otx2_write64(pfvf, NIX_LF_CQ_OP_DOOR,
		     ((u64)cq->cq_idx << 32) | processed_cqe);
}

void dup_free_pending_sqe(struct otx2_nic *pfvf)
{
	int tx_pkts = 0, tx_bytes = 0;
	struct sk_buff *skb = NULL;
	struct otx2_snd_queue *sq;
	struct netdev_queue *txq;
	struct sg_list *sg;
	int sq_idx, sqe;

	for (sq_idx = 0; sq_idx < pfvf->hw.tx_queues; sq_idx++) {
		sq = &pfvf->qset.sq[sq_idx];
		for (sqe = 0; sqe < sq->sqe_cnt; sqe++) {
			sg = &sq->sg[sqe];
			skb = (struct sk_buff *)sg->skb;
			if (skb) {
				tx_bytes += skb->len;
				tx_pkts++;
				dup_dma_unmap_skb_frags(pfvf, sg);
				dev_kfree_skb_any(skb);
				sg->skb = (u64)NULL;
			}
		}

		if (!tx_pkts)
			continue;
		txq = netdev_get_tx_queue(pfvf->netdev, sq_idx);
		netdev_tx_completed_queue(txq, tx_pkts, tx_bytes);
		tx_pkts = 0;
		tx_bytes = 0;
	}
}

int dup_rxtx_enable(struct otx2_nic *pfvf, bool enable)
{
	struct msg_req *msg;
	int err;

	mutex_lock(&pfvf->mbox.lock);
	if (enable)
		msg = otx2_mbox_alloc_msg_nix_lf_start_rx(&pfvf->mbox);
	else
		msg = otx2_mbox_alloc_msg_nix_lf_stop_rx(&pfvf->mbox);

	if (!msg) {
		mutex_unlock(&pfvf->mbox.lock);
		return -ENOMEM;
	}

	err = otx2_sync_mbox_msg(&pfvf->mbox);
	mutex_unlock(&pfvf->mbox.lock);
	return err;
}
