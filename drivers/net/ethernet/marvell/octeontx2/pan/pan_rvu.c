// SPDX-License-Identifier: GPL-2.0
/* Marvell RVU PAN driver
 *
 * Copyright (C) 2025 Marvell.
 *
 */
#include <linux/etherdevice.h>
#include <linux/module.h>
#include <linux/pci.h>
#include <linux/kernel.h>     /* Needed for KERN_INFO */
#include <linux/device.h>
#include <linux/msi.h>
#include <net/busy_poll.h>
#include <net/tso.h>

#include "pan_cmn.h"

#define CQE_ADDR(CQ, idx) ((CQ)->cqe_base + ((CQ)->cqe_size * (idx)))
#define RQ_DROP_LVL_CQ(skid, qsize)	(((skid) * 256) / (qsize))
#define RQ_PASS_LVL_CQ(skid, qsize)	((((skid) + 16) * 256) / (qsize))

static const struct pci_device_id pan_rvu_id_table[] = {
	{ PCI_DEVICE(PCI_VENDOR_ID_CAVIUM, PCI_DEVID_PAN_RVU) },
	{ }
};

MODULE_DEVICE_TABLE(pci, pan_rvu_id_table);

#define DRV_NAME        "pan_rvu"

static struct pan_rvu_gbl_t pan_rvu_gbl;

static void pan_rvu_gbl_init(void)
{
	int i;

	for (i = 0; i < 256 + 32; i++)
		pan_rvu_gbl.sqoff2pcifunc[i] = -1;
}

struct pan_rvu_gbl_t *pan_rvu_get_gbl(void)
{
	return &pan_rvu_gbl;
}

int pan_rvu_pcifunc2_sq_off(u16 pcifunc)
{
	unsigned long pos;
	struct xarray *xa;
	void *entry;

	pos = pcifunc;

	xa = &pan_rvu_gbl.pcifunc2sqoff;
	entry = xa_find(xa, &pos, ULONG_MAX, XA_PRESENT);
	if (!entry) {
		pr_err("Could not find the mapped offset %#x\n", pcifunc);
		return -ESRCH;
	}

	return xa_to_value(entry);
}

static int pan_rvu_pcifunc2sq_off_map_create(struct otx2_nic *otx2_nic)
{
	struct pan_rvu_cq_info *cq_info;
	struct pan_rvu_sq_info *sq_info;
	struct pan_rvu_dev_priv *priv;
	struct net_device *netdev;
	struct xarray *xa;
	int j;

	xa_init(&pan_rvu_gbl.pcifunc2sqoff);

	netdev = otx2_nic->netdev;
	priv = netdev_priv(netdev);

	cq_info = priv->cq_info;
	sq_info = cq_info->sq_info;
	if (!sq_info)
		return -ESRCH;

	if (!cq_info->sq2cqidxs)
		return -ESRCH;

	xa = &pan_rvu_gbl.pcifunc2sqoff;

	/* Searching in the first sq_info will yield offset */
	for (j = 0; j < cq_info->sq_cnt; j++, sq_info++) {
		/* TODO: free the stored valued */
		xa_store(xa, sq_info->pcifunc, xa_mk_value(j), GFP_KERNEL);
		pan_rvu_gbl.sqoff2pcifunc[j] = sq_info->pcifunc;
	}

	return 0;
}

struct otx2_nic *pan_rvu_get_otx2_nic(struct net_device *dev)
{
	struct ethtool_ops const *ops;
	struct ethtool_drvinfo info;

	ops = dev->ethtool_ops;
	if (!ops)
		return NULL;

	if (!ops->get_drvinfo)
		return NULL;

	ops->get_drvinfo(dev, &info);

	if (strncmp(info.driver, "rvu-nicvf", sizeof(info.driver)) &&
	    strncmp(info.driver, "rvu-nicpf", sizeof(info.driver)))
		return NULL;

	return netdev_priv(dev);
}

struct net_device *
__pan_rvu_get_kernel_netdev_by_pcifunc(u16 pcifunc)
{
	struct net_device *dev = NULL;
	struct otx2_nic *otx2_nic;
	struct net *net;

	for_each_net(net) {
		for_each_netdev(net, dev) {
			otx2_nic = pan_rvu_get_otx2_nic(dev);
			if (!otx2_nic)
				continue;

			if (otx2_nic->pcifunc != pcifunc)
				continue;

			goto done;
		}
	}

done:
	return dev;
}

struct net_device *pan_rvu_get_kernel_netdev_by_pcifunc(u16 pcifunc)
{
	struct net_device *dev = NULL;

	rtnl_lock();
	dev = __pan_rvu_get_kernel_netdev_by_pcifunc(pcifunc);
	rtnl_unlock();
	return dev;
}

static int pan_rvu_find_nsgs_n_len(struct nix_cqe_rx_s *cqe, int *num_sgs, int *len)
{
	struct nix_rx_sg_s *rx_sg_s;
	int num_rx_desc;
	int sz;

	sz = (cqe->parse.desc_sizem1) ?
		((cqe->parse.desc_sizem1 + 1) * 16) :
		sizeof(struct nix_rx_sg_s);

	num_rx_desc = sz / sizeof(struct nix_rx_sg_s);
	rx_sg_s = &cqe->sg;

	/* Non SG case */
	if (likely(num_rx_desc == 1) && likely(rx_sg_s->segs == 1)) {
		*num_sgs = 1;
		*len = rx_sg_s->seg_size;
		return 0;
	}

	return -EFAULT;
}

static int pan_rvu_get_sq_chan(struct otx2_nic *otx2_nic, int sq_idx, u16 *chan,
			       bool *is_sdp)
{
	struct pan_rvu_cq_info *cq_info;
	struct pan_rvu_sq_info *sq_info;
	struct pan_rvu_dev_priv *priv;
	struct net_device *netdev;
	int i, j, idx;

	netdev = otx2_nic->netdev;
	priv = netdev_priv(netdev);

	idx = sq_idx / pan_rvu_gbl.sqs_per_core;
	cq_info = &priv->cq_info[idx];

	for (i = 0; i < otx2_nic->hw.cint_cnt; i++, cq_info++) {
		sq_info = cq_info->sq_info;
		if (!sq_info)
			continue;

		for (j = 0; j < cq_info->sq_cnt; j++, sq_info++) {
			if (sq_info->sqidx != sq_idx)
				continue;

			*chan = sq_info->tx_chan;
			*is_sdp = sq_info->is_sdp;
			return 0;
		}
	}

	return -ESRCH;
}

static void pan_rvu_pull_from_frag0(struct sk_buff *skb, int grow)
{
	struct skb_shared_info *pinfo = skb_shinfo(skb);
	const skb_frag_t *frag0 = &pinfo->frags[0];

	BUG_ON(skb->end - skb->tail < grow);

	memcpy(skb_tail_pointer(skb), skb_frag_address(frag0), grow);

	skb->data_len -= grow;
	skb->tail += grow;

	skb_frag_off_add(&pinfo->frags[0], grow);
	skb_frag_size_sub(&pinfo->frags[0], grow);

	if (unlikely(!skb_frag_size(&pinfo->frags[0]))) {
		skb_frag_unref(skb, 0);
		memmove(pinfo->frags, pinfo->frags + 1,
			--pinfo->nr_frags * sizeof(pinfo->frags[0]));
	}
}

static int pan_rvu_sg_iterate(struct nix_cqe_rx_s *cqe, pan_rvu_sg_cb cb, void *arg)
{
	struct nix_rx_sg_s *rx_sg_s;
	int num_rx_desc;
	u64 *sg_addr;
	int i, k, sz;
	int tot = 0;
	u16 *sg_sz;

	sz = (cqe->parse.desc_sizem1) ?
		((cqe->parse.desc_sizem1 + 1) * 16) :
		sizeof(struct nix_rx_sg_s);
	num_rx_desc = sz / sizeof(struct nix_rx_sg_s);
	rx_sg_s = &cqe->sg;

	/* Non SG case */
	if (likely(num_rx_desc == 1) && likely(rx_sg_s->segs == 1))
		return cb(rx_sg_s->seg_addr, rx_sg_s->seg_size,
			  true, 0, arg);

	pr_debug("Multiple SG case\n");
	tot = 0;
	rx_sg_s = &cqe->sg;
	for (i = 0; i < num_rx_desc; i++, rx_sg_s++)
		tot += rx_sg_s->segs;

	rx_sg_s = &cqe->sg;
	for (i = 0; i < num_rx_desc; i++, rx_sg_s++) {
		sg_addr = &rx_sg_s->seg_addr;
		sg_sz = (u16 *)rx_sg_s;
		for (k = 0; k < rx_sg_s->segs; k++, sg_addr++, sg_sz++) {
			tot--;
			if (cb(*sg_addr, *sg_sz, !!tot, k, arg))
				return -1;
		}
	}

	return 0;
}

static int pan_rvu_fill_in_tuple(struct otx2_nic *pfvf, struct nix_cqe_rx_s *cqe,
				 struct pan_tuple *tuple, struct pan_tuple_hdr *hdr,
				 int *num_sgs, int *len)
{
	struct nix_rx_sg_s *rx_sg_s;
	int num_rx_desc;
	u64 *sg_addr;
	int i, k, sz;
	u16 *sg_sz;
	u8 *va;

	sz = (cqe->parse.desc_sizem1) ?
		((cqe->parse.desc_sizem1 + 1) * 16) :
		sizeof(struct nix_rx_sg_s);

	num_rx_desc = sz / sizeof(struct nix_rx_sg_s);
	rx_sg_s = &cqe->sg;

	/* Non SG case */
	if (likely(num_rx_desc == 1) && (rx_sg_s->segs == 1)) {
		if (rx_sg_s->seg_size  < sizeof(struct ethhdr)) {
			pr_err("buffer size(%u) is less than eth hdr\n", rx_sg_s->seg_size);
			return -ENOBUFS;
		}

		va = (u8 *)phys_to_virt(otx2_iova_to_phys(pfvf->iommu_domain,
							  rx_sg_s->seg_addr));

		pan_parse_buf(va, tuple, hdr);

		*num_sgs = 1;
		*len = rx_sg_s->seg_size;

		return 0;
	}

	/* TODO: handle pan_parse_buf() for SG case */
	*num_sgs = 0;
	*len = 0;

	rx_sg_s = &cqe->sg;
	for (i = 0; i < num_rx_desc; i++, rx_sg_s++) {
		*num_sgs += rx_sg_s->segs;
		sg_addr = &rx_sg_s->seg_addr;
		sg_sz = (u16 *)rx_sg_s;
		for (k = 0; k < rx_sg_s->segs; k++, sg_addr++, sg_sz++) {
			*len += *sg_sz;

			/* TODO: parse data */
		}
	}

	return 0;
}

#define MAX_SEGS_PER_SG	3

struct pan_rvu_sg_data {
	struct otx2_snd_queue *sq;
	int *offset;
	int num_sgs;
	int len;
	struct otx2_nic *pfvf;
	struct otx2_nic *rxpfvf;
	int cq_idx;
};

enum {
	SG_LIST_FLAG_LAST_FRAG = BIT_ULL(0),
};

#define MAX_SEGS_PER_SG	3
static int pan_rvu_rx2tx_sg_map_iter(u64 rx_sg_addr, u16 rx_sg_sz, bool is_last, int seg, void *data)
{
	struct pan_rvu_sg_data *arg = (struct pan_rvu_sg_data *)data;
	struct nix_sqe_sg_s *sg = NULL;
	struct otx2_snd_queue *sq = arg->sq;
	int num_segs = arg->num_sgs;
	u64 *iova = NULL;
	u16 *sg_lens = NULL;
	int *offset = arg->offset;

	sq->sg[sq->head].num_segs = 0;

	if ((seg & (MAX_SEGS_PER_SG - 1)) == 0) {
		sg = (struct nix_sqe_sg_s *)(sq->sqe_base + *offset);
		sg->ld_type = NIX_SEND_LDTYPE_LDD;
		sg->subdc = NIX_SUBDC_SG;
		sg->segs = 0;
		sg_lens = (void *)sg;
		iova = (void *)sg + sizeof(*sg);
		/* Next subdc always starts at a 16byte boundary.
		 * So if sg->segs is whether 2 or 3, offset += 16bytes.
		 */
		if ((num_segs - seg) >= (MAX_SEGS_PER_SG - 1))
			*offset += sizeof(*sg) + (3 * sizeof(u64));
		else
			*offset += sizeof(*sg) + sizeof(u64);
	}

	sg_lens[seg] = rx_sg_sz;
	sg->segs++;
	*iova++ = rx_sg_addr;

	/* Save DMA mapping info for later unmapping */
	sq->sg[sq->head].dma_addr[seg] = rx_sg_addr;
	sq->sg[sq->head].size[seg] = rx_sg_sz;
	sq->sg[sq->head].num_segs++;

	/* TODO: intoduce a field to skip skb freeing */
	sq->sg[sq->head].skb = 0;
#if IS_ENABLED(CONFIG_OCTEONTX_PAN_TX_COMPLETION)
	sq->sg[sq->head].cq_idx = arg->cq_idx;
#endif

	if (likely(is_last)) {
#if IS_ENABLED(CONFIG_OCTEONTX_PAN_TX_COMPLETION)
		sq->sg[sq->head].flags = SG_LIST_FLAG_LAST_FRAG;
#endif
		sq->sg[sq->head].len = arg->len;
	}

	return true;
}

/* Add SQE scatter/gather subdescriptor structure */
static bool pan_rvu_rx2tx_sg_map(struct otx2_nic *rxpfvf, struct otx2_snd_queue *sq,
				 struct nix_sqe_hdr_s *sqe_hdr,
				 struct nix_cqe_rx_s *cqe, int qidx, int num_sgs,
				 int *offset, int len)
{
	struct pan_rvu_sg_data data = {
		.sq = sq,
		.offset = offset,
		.num_sgs = num_sgs,
		.rxpfvf = rxpfvf,
		.len = len,
		.cq_idx = qidx,
	};

	/* TODO: handle error case */
	pan_rvu_sg_iterate(cqe, pan_rvu_rx2tx_sg_map_iter, &data);

	return true;
}

static void pan_rvu_free_rx_bufs(struct otx2_nic *rxpfvf,
				 struct otx2_cq_queue *cq,
				 struct nix_cqe_rx_s *cqe)
{
	struct nix_rx_sg_s *rx_sg_s;
	int num_rx_desc;
	u64 *sg_addr;
	int i, k, sz;
	u16 *sg_sz;

	sz = (cqe->parse.desc_sizem1) ?
		((cqe->parse.desc_sizem1 + 1) * 16) :
		sizeof(struct nix_rx_sg_s);
	num_rx_desc = sz / sizeof(struct nix_rx_sg_s);

	rx_sg_s = &cqe->sg;
	if (likely(num_rx_desc == 1) && likely(rx_sg_s->segs == 1)) {
		sg_addr = &rx_sg_s->seg_addr;
		rxpfvf->hw_ops->aura_freeptr(rxpfvf, cq->cq_idx,
					     *sg_addr & ~0x07ULL);
		return;
	}

	for (i = 0; i < num_rx_desc; i++, rx_sg_s++) {
		sg_addr = &rx_sg_s->seg_addr;
		sg_sz = (u16 *)rx_sg_s;
		for (k = 0; k < rx_sg_s->segs; k++, sg_addr++, sg_sz++)
			rxpfvf->hw_ops->aura_freeptr(rxpfvf, cq->cq_idx,
						     *sg_addr & ~0x07ULL);
	}
}

static bool pan_rvu_buf_xmit(struct pan_fl_tbl_res *res,
			     struct otx2_cq_queue *cq,
			     struct nix_cqe_rx_s *cqe,
			     int num_sgs, int len,
			     struct pan_tuple_hdr *hdr,
			     struct otx2_nic *rxpfvf,
			     u16 off)
{
	struct nix_sqe_hdr_s *sqe_hdr;
	int offset, free_desc_or_sqe;
	struct nix_sqe_ext_s *ext;
	struct otx2_snd_queue *sq;
	int sq_idx;

	/* TODO: fix Dont allow to cross for now */
	if (cq->cq_idx >= rxpfvf->hw.cint_cnt) {
		pr_err("sq_idx=%u is > max (%u)\n", sq_idx, rxpfvf->hw.cint_cnt);
		return -EINVAL;
	}

	sq_idx = cq->cq_idx * pan_rvu_gbl.sqs_per_core + off;

	if (unlikely(sq_idx > pan_rvu_gbl.sqs_usable)) {
		pr_err("sq_idx is too big (%u) max=%u\n", sq_idx, pan_rvu_gbl.sqs_usable);
		pan_rvu_free_rx_bufs(rxpfvf, cq, cqe);
		return -ESRCH;
	}

	sq = &rxpfvf->qset.sq[sq_idx];
	pan_stats_inc(num_sgs > 1 ? PAN_STATS_FLD_IN_SG_PKTS :
		      PAN_STATS_FLD_IN_NON_SG_PKTS);

#if IS_ENABLED(CONFIG_OCTEONTX_PAN_TX_COMPLETION)
	/* Check if there is enough room between producer
	 * and consumer index.
	 */
	free_desc_or_sqe = (sq->cons_head - sq->head - 1 + sq->sqe_cnt) & (sq->sqe_cnt - 1);
#else
	free_desc_or_sqe = (sq->num_sqbs - *sq->aura_fc_addr) * sq->sqe_per_sqb;
#endif

	if (free_desc_or_sqe < sq->sqe_thresh) {
		/* TODO: Fix it by enqueuing to a queue ?
		 */
		pan_rvu_free_rx_bufs(rxpfvf, cq, cqe);
		pan_stats_inc(PAN_STATS_FLD_SQE_THRESH);
		return false;
	}

	if (free_desc_or_sqe < num_sgs) {
		/* TODO: Fix it by enqueuing to a queue ?
		 */
		pan_rvu_free_rx_bufs(rxpfvf, cq, cqe);
		pan_stats_inc(PAN_STATS_FLD_TX_DESC);
		return false;
	}

	/* TODO: check how to do TSO mainly in pkt modified case */

	/* Set SQE's SEND_HDR.
	 * Do not clear the first 64bit as it contains constant info.
	 */
	memset(sq->sqe_base + 8, 0, sq->sqe_size - 8);
	sqe_hdr = (struct nix_sqe_hdr_s *)(sq->sqe_base);

	/* Check if SQE was framed before, if yes then no need to
	 * set these constants again and again.
	 */
	if (unlikely(!sqe_hdr->total)) {
#if IS_ENABLED(CONFIG_OCTEONTX_PAN_TX_COMPLETION)
		sqe_hdr->df = 1;
		sqe_hdr->aura = sq->aura_id;
		/* Post a CQE Tx after pkt transmission */
		sqe_hdr->pnc = 1;
#else
		/* Free hardware buffer to aura */
		sqe_hdr->df = 0;
		sqe_hdr->aura = cqe->parse.pb_aura;
#endif
		/* sq_idx is calculated above */
		sqe_hdr->sq = sq_idx;
	}

	sqe_hdr->total = len;

	/* Set SQE identifier which will be used later for freeing SKB */
	sqe_hdr->sqe_id = sq->head;

	/* TODO: change CHECKSUM_NONE to proper value */

	/* Packet is parsed and modified. Recalculate Checksum */
	if (unlikely(hdr->flags & PAN_TUPLE_FLAG_L4_PROTO_TCP)) {
		sqe_hdr->ol3ptr = hdr->l3hdr - hdr->l2hdr;
		sqe_hdr->ol4ptr = hdr->l4hdr - hdr->l2hdr;
		sqe_hdr->ol3type = NIX_SENDL3TYPE_IP4_CKSUM;
		sqe_hdr->ol4type = NIX_SENDL4TYPE_TCP_CKSUM;
	}

	/* TODO: Add extended header if needed */
	offset = sizeof(*sqe_hdr) +  sizeof(*ext);
	ext = (struct nix_sqe_ext_s *)(sq->sqe_base + sizeof(*sqe_hdr));
	ext->subdc = NIX_SUBDC_EXT;

	/* Add SG subdesc with data frags */
	if (!pan_rvu_rx2tx_sg_map(rxpfvf, sq, sqe_hdr, cqe, cq->cq_idx,
				  num_sgs, &offset, len)) {
		/* TODO: handle unmap incase of error */
		smp_mb();

		/* TODO: free the rx buffers or enqueue to backlog queue */
		/* TODO: add debug stats */
		return false;
	}

	sqe_hdr->sizem1 = (offset / 16) - 1;
	/* Flush SQE to HW */
	rxpfvf->hw_ops->sqe_flush(rxpfvf, sq, offset, sq_idx);

	pan_stats_inc(num_sgs > 1 ? PAN_STATS_FLD_OUT_SG_PKTS :
		      PAN_STATS_FLD_OUT_NON_SG_PKTS);

	return true;
}

static int
pan_rvu_inject_buf2stack(struct otx2_nic *pfvf,
			 struct pan_rvu_cq_info *cq_info,
			 struct otx2_cq_queue *cq,
			 struct nix_cqe_rx_s *cqe,
			 struct pan_fl_tbl_res *res,
			 enum pan_fl_tbl_act act)
{
	struct nix_rx_parse_s *parse = &cqe->parse;
	struct pan_sw_l2_offl_node *node;
	struct nix_rx_sg_s *sg = &cqe->sg;
	struct skb_shared_info *pinfo;
	struct pan_tuple tuple = {0};
	struct pan_fl_tbl_res *pres;
	struct sk_buff *skb = NULL;
	struct net_device *netdev;
	const struct ethhdr *eth;
	const skb_frag_t *frag0;
	void *end, *start;
	u64 *seg_addr;
	u16 *seg_size;
	int seg;

	if (unlikely(parse->errlev || parse->errcode)) {
		if (dup_check_rcv_errors(pfvf, cqe, cq->cq_idx))
			return -EFAULT;
	}

	skb = alloc_skb(MAX_HEADER, GFP_ATOMIC);
	if (unlikely(!skb))
		return -ENOMEM;

	pan_stats_inc(PAN_STATS_FLD_EXP_PKTS);

	start = (void *)sg;
	end = start + ((cqe->parse.desc_sizem1 + 1) * 16);
	while (start < end) {
		sg = (struct nix_rx_sg_s *)start;
		seg_addr = &sg->seg_addr;
		seg_size = (void *)sg;
		for (seg = 0; seg < sg->segs; seg++, seg_addr++) {
			if (otx2_skb_add_frag(pfvf, skb, *seg_addr,
					      seg_size[seg], parse, cq->cq_idx))
				cq->pool_ptrs++;
		}
		start += sizeof(*sg);
	}
	dup_set_rxhash(pfvf, cqe, skb);

	skb_record_rx_queue(skb, cq->cq_idx);

	dup_set_taginfo(parse, skb);
	skb_mark_for_recycle(skb);

	if (pfvf->netdev->features & NETIF_F_RXCSUM)
		skb->ip_summed = CHECKSUM_UNNECESSARY;

	pinfo = skb_shinfo(skb);
	frag0 = &pinfo->frags[0];

	/* We dont have a NAPI (for pan device)to receive the packet; so won't call
	 * dev_gro_receive(). So create ethhdr in linear skb part
	 */
	pan_rvu_pull_from_frag0(skb, sizeof(*eth));
	skb_reset_mac_header(skb);

	eth = (const struct ethhdr *)skb->data;

	/* TODO: Improve on ~7 mask */
	netdev = xa_load(&pan_rvu_gbl.chan2dev, parse->chan & ~0x7);
	if (netdev) {
		skb->protocol = eth_type_trans(skb, netdev);
		skb->dev = netdev;
	} else {
		skb->dev = pfvf->netdev;
	}

	if (act & PAN_FL_TBL_ACT_EXP)
		return netif_receive_skb(skb);

	if (act & PAN_FL_TBL_ACT_L2_FWD) {
		node = __pan_sw_l2_mac_tbl_lookup(eth->h_source);
		if (node) {
			pan_tuple_hash_set(&tuple, node->match_id);
			tuple.flags |= PAN_TUPLE_FLAG_L3_PROTO_V4;
			if (!__pan_fl_tbl_offl_lookup_n_res(&tuple, &pres)) {
				pres->dir = FLOW_OFFLOAD_DIR_REPLY;
				res->dir = FLOW_OFFLOAD_DIR_REPLY;

				res->pair = pres;
				pres->pair = res;
			}
			/* TODO: handle err */
			return netif_receive_skb(skb);
		}

		/* TODO: handle err */
		if (netdev)
			return netif_receive_skb(skb);

		WARN_ON_ONCE(1);
	}

	skb->dev = pfvf->netdev;
	skb_dump(KERN_ERR, skb, true);
	dev_kfree_skb_any(skb);

	return -1;
}

static int
pan_rvu_rewrite_l2_hdr(struct otx2_nic *pfvf,
		       struct pan_rvu_cq_info *cq_info,
		       struct otx2_cq_queue *cq,
		       struct nix_cqe_rx_s *cqe,
		       struct pan_fl_tbl_res *res,
		       u16 *xmit_pcifunc_off)
{
	struct nix_rx_parse_s *parse = &cqe->parse;
	struct pan_sw_l2_offl_node *l2_node;
	struct nix_rx_sg_s *sg = &cqe->sg;
	struct net_device *dev, *in_dev;
	struct pan_tuple tuple = {};
	struct pan_fl_tbl_res *pres;
	struct neighbour *neigh;
	struct iphdr *iphdr;
	struct ethhdr *eth;
	bool br_routing;
	u64 *seg_addr;
	u16 *seg_size;
	u16 pcifunc;
	void *start;
	void *va;

	if (unlikely(parse->errlev || parse->errcode)) {
		if (dup_check_rcv_errors(pfvf, cqe, cq->cq_idx))
			return -EFAULT;
	}

	start = (void *)sg;
	sg = (struct nix_rx_sg_s *)start;
	seg_addr = &sg->seg_addr;
	seg_size = (void *)sg;

	pcifunc = pan_rvu_gbl.sqoff2pcifunc[res->pcifuncoff];
	dev = xa_load(&pan_rvu_gbl.pfunc2dev, pcifunc);
	if (likely(!dev))
		return -ENOENT;

	br_routing = !!(res->act & PAN_FL_TBL_ACT_L3_BR_FWD);
	if (unlikely(br_routing))
		dev = netdev_master_upper_dev_get_rcu(dev);

	va = phys_to_virt(otx2_iova_to_phys(pfvf->iommu_domain, *seg_addr));

	eth = (struct ethhdr *)va;
	iphdr = (struct iphdr *)(va + ETH_HLEN);

	in_dev = xa_load(&pan_rvu_gbl.chan2dev, parse->chan & ~0x7);
	if (in_dev) {
		if (unlikely(netif_is_bridge_port(in_dev))) {
			l2_node = __pan_sw_l2_mac_tbl_lookup(eth->h_source);
			if (!l2_node)
				return -ENOENT;

			tuple.flags |= PAN_TUPLE_FLAG_L3_PROTO_V4;
			pan_tuple_hash_set(&tuple, l2_node->match_id);
			__pan_fl_tbl_offl_lookup_n_res(&tuple, &pres);
		}
	}

	neigh = __ipv4_neigh_lookup_noref(dev, (__force u32)iphdr->daddr);
	if (unlikely(!neigh))
		return -ENOENT;

	if (unlikely(br_routing)) {
		l2_node = __pan_sw_l2_mac_tbl_lookup(neigh->ha);
		if (!l2_node)
			return -ENOENT;
	}

	ether_addr_copy(eth->h_source, res->opq->eg_mac);
	ether_addr_copy(eth->h_dest, neigh->ha);

	if (likely(!br_routing))
		return 0;

	pcifunc = l2_node->port_id;
	*xmit_pcifunc_off = pan_rvu_pcifunc2_sq_off(pcifunc);
	return 0;
}

static void pan_rvu_process_buf(struct otx2_nic *pfvf,
				struct pan_rvu_cq_info *cq_info,
				struct otx2_cq_queue *cq,
				struct nix_cqe_rx_s *cqe)
{
	struct nix_rx_parse_s *parse = &cqe->parse;
	struct pan_tuple_hdr hdr = { 0 };
	struct pan_tuple tuple = { 0 };
	struct pan_fl_tbl_res *res;
	u16 xmit_pcifunc_off;
	int len, ret;
	int num_sgs;

	if (unlikely(parse->errlev || parse->errcode)) {
		if (dup_check_rcv_errors(pfvf, cqe, cq->cq_idx))
			return;
	}

	if (unlikely(!parse->match_id)) {
		ret = pan_rvu_fill_in_tuple(pfvf, cqe, &tuple, &hdr, &num_sgs, &len);
		if (ret) {
			pr_err("Error happened while filling tuple\n");
			/* TODO: how to free the buffer ? */
			return;
		}

		if (__pan_fl_tbl_lookup_n_res(&tuple, &res)) {
			/* Send packet to stack thru pan device */
#if IS_ENABLED(CONFIG_OCTEONTX_PAN_POLLING_MODE)
			local_bh_disable();
#endif
			pan_rvu_inject_buf2stack(pfvf, cq_info, cq, cqe, res, res->act);

#if IS_ENABLED(CONFIG_OCTEONTX_PAN_POLLING_MODE)
			local_bh_enable();
#endif
			return;
		}
	} else {
		pan_tuple_hash_set(&tuple, parse->match_id);
		tuple.flags |= PAN_TUPLE_FLAG_L3_PROTO_V4;

		if (__pan_fl_tbl_offl_lookup_n_res(&tuple, &res)) {
			/* Send packet to stack thru pan device */
			pan_rvu_inject_buf2stack(pfvf, cq_info, cq, cqe, res, res->act);
			return;
		}

		ret = pan_rvu_find_nsgs_n_len(cqe, &num_sgs, &len);
		if (ret) {
			pr_err("Failed to find nums_sgs and len\n");
			pan_rvu_inject_buf2stack(pfvf, cq_info, cq, cqe, res, res->act);
			return;
		}
	}

	if (unlikely(res->pcifuncoff == -1)) {
		pr_debug("Tuple matched; but no pcifunc configured\n");
		pan_rvu_inject_buf2stack(pfvf, cq_info, cq, cqe, res, res->act);
		return;
	}

	/* Last byte of match id is connection id */
	switch (res->act) {
	case PAN_FL_TBL_ACT_EXP:
		pan_rvu_inject_buf2stack(pfvf, cq_info, cq, cqe, res, PAN_FL_TBL_ACT_EXP);
		return;

	case PAN_FL_TBL_ACT_L3_FWD:
		ret = pan_rvu_rewrite_l2_hdr(pfvf, cq_info, cq, cqe, res, NULL);

		/* Incase of error reinject the packet back to stack */
		if (ret) {
			pan_rvu_inject_buf2stack(pfvf, cq_info, cq, cqe, res,
						 PAN_FL_TBL_ACT_EXP);
			return;
		}

		xmit_pcifunc_off = res->pcifuncoff;
		break;

	case PAN_FL_TBL_ACT_L3_BR_FWD:
		ret = pan_rvu_rewrite_l2_hdr(pfvf, cq_info, cq, cqe, res, &xmit_pcifunc_off);

		/* Incase of error reinject the packet back to stack */
		if (ret) {
			pan_rvu_inject_buf2stack(pfvf, cq_info, cq, cqe, res,
						 PAN_FL_TBL_ACT_EXP);
			return;
		}
		break;

	case PAN_FL_TBL_ACT_L2_FWD:

		/* Say  x86-A (mac: X) ---------> eth0 DUT(pan) eth1---------> x86-B(MAC: Y)
		 *				  [ eth0 and eth1 are
		 *				    bridged(br0) ]
		 * Here first packet from A to B machine will cause MAC:X to be learned on eth0
		 * (fbd learning on src mac). This will push an DMAC offload rule to NPC, where
		 * DMAC = X. All packets from X86-B to X86-A will hit this rule. So linux slow
		 * path bridge will never see the packet, hence no fdb in reverse path. So only
		 * direction will be offloaded.
		 * Avoid this situation by reinjecting the packet back to stack and feciliate
		 * fdb learning by bridge.
		 */
		if (res->dir == FLOW_OFFLOAD_DIR_ORIGINAL) {
			pan_rvu_inject_buf2stack(pfvf, cq_info, cq, cqe, res, res->act);
			return;
		}

		xmit_pcifunc_off = res->pcifuncoff;
		break;

	default:
		xmit_pcifunc_off = res->pcifuncoff;
		break;
	}

#if IS_ENABLED(CONFIG_OCTEONTX_PAN_TX_COMPLETION)
	/* For refilling the buffers */
	cq->pool_ptrs += num_sgs;
#endif

	ret = pan_rvu_buf_xmit(res, cq, cqe, num_sgs, len, &hdr,
			       pfvf, xmit_pcifunc_off);
	if (!ret)
		return;
}

static int pan_rvu_rx_cq_reap(struct otx2_nic *pfvf,
			      struct pan_rvu_cq_info *cq_info,
			      struct otx2_cq_queue *cq, int budget)
{
	struct nix_cqe_rx_s *cqe;
	int processed_cqe = 0;

	if (cq->pend_cqe >= budget)
		goto process_cqe;

	if (dup_nix_cq_op_status(pfvf, cq) || !cq->pend_cqe)
		return 0;

process_cqe:
	while (likely(processed_cqe < budget) && cq->pend_cqe) {
		cqe = (struct nix_cqe_rx_s *)CQE_ADDR(cq, cq->cq_head);
		if (cqe->hdr.cqe_type == NIX_XQE_TYPE_INVALID ||
		    !cqe->sg.seg_addr) {
			if (!processed_cqe)
				return 0;
			break;
		}
		cq->cq_head++;
		cq->cq_head &= (cq->cqe_cnt - 1);

		pan_rvu_process_buf(pfvf, cq_info, cq, cqe);

		cqe->hdr.cqe_type = NIX_XQE_TYPE_INVALID;
		cqe->sg.seg_addr = 0x00;
		processed_cqe++;
		cq->pend_cqe--;
	}

	/* Free CQEs to HW */
	otx2_write64(pfvf, NIX_LF_CQ_OP_DOOR,
		     ((u64)cq->cq_idx << 32) | processed_cqe);

	return processed_cqe;
}

#if !IS_ENABLED(CONFIG_OCTEONTX_PAN_POLLING_MODE)
#if IS_ENABLED(CONFIG_OCTEONTX_PAN_TX_COMPLETION)
static void pan_rvu_snd_pkt_handler(struct otx2_nic *pfvf,
				    struct otx2_cq_queue *cq,
				    struct otx2_snd_queue *sq,
				    struct nix_cqe_tx_s *cqe,
				    int budget, int *tx_pkts, int *tx_bytes)
{
	struct nix_send_comp_s *snd_comp = &cqe->comp;
	struct sk_buff *skb = NULL;
	struct otx2_pool *pool;
	struct sg_list *sg;
	struct page *page;
	u16 pool_id;
	u64 iova;
	u64 pa;

	if (unlikely(snd_comp->status) && netif_msg_tx_err(pfvf))
		net_err_ratelimited("%s: TX%d: Error in send CQ status:%x\n",
				    pfvf->netdev->name, cq->cint_idx,
				    snd_comp->status);

	/* Barrier, so that update to sq by other cpus is visible */
	smp_mb();
	sg = &sq->sg[snd_comp->sqe_id];
	skb = (struct sk_buff *)sg->skb;
	BUG_ON(skb);

	/* TODO: make sure SG len is total length in case of SG */
	*tx_bytes += sg->len;
	(*tx_pkts)++;

	if (!(sg->flags & SG_LIST_FLAG_LAST_FRAG))
		BUG_ON(1);

	/* TODO: Try removing this check */
	BUG_ON(sg->cq_idx > num_online_cpus());

	/* TODO: cq_idx will be 1:1 mapping to RQ. Is this assumption
	 * will be wrong anywhere ?
	 */
	pool_id = otx2_get_pool_idx(pfvf, AURA_NIX_RQ, sg->cq_idx);
	pool = &pfvf->qset.pool[pool_id];

	/* TODO: Fix if packet getting modified or in case of
	 * SG
	 */
	iova = sg->dma_addr[0] - OTX2_HEAD_ROOM;
	pa = otx2_iova_to_phys(pfvf->iommu_domain, iova);
	page = virt_to_head_page(phys_to_virt(pa));
	page_pool_put_full_page(pool->page_pool, page, true);
}

static int pan_rvu_tx_cq_reap(struct otx2_nic *pfvf,
			      struct otx2_cq_queue *cq,
			      int sq_idx, int budget)
{
	int tx_pkts = 0, tx_bytes = 0;
	struct otx2_snd_queue *sq;
	struct nix_cqe_tx_s *cqe;
	int processed_cqe = 0;

	if (cq->pend_cqe >= budget)
		goto process_cqe;

	if (dup_nix_cq_op_status(pfvf, cq) || !cq->pend_cqe)
		return 0;

process_cqe:
	sq = &pfvf->qset.sq[sq_idx];

	while (likely(processed_cqe < budget) && cq->pend_cqe) {
		cqe = (struct nix_cqe_tx_s *)dup_get_next_cqe(cq);
		if (unlikely(!cqe)) {
			if (!processed_cqe)
				return 0;
			break;
		}
		pan_rvu_snd_pkt_handler(pfvf, cq, sq, cqe, budget,
					&tx_pkts, &tx_bytes);

		cqe->hdr.cqe_type = NIX_XQE_TYPE_INVALID;
		processed_cqe++;
		cq->pend_cqe--;

		sq->cons_head++;
		sq->cons_head &= (sq->sqe_cnt - 1);
	}

	/* Free CQEs to HW */
	otx2_write64(pfvf, NIX_LF_CQ_OP_DOOR,
		     ((u64)cq->cq_idx << 32) | processed_cqe);

	pr_debug("TX reap packets(%u) seesn on sq=%u\n", tx_pkts, sq_idx);

	return tx_pkts;
}

#endif
#endif

static bool pan_rvu_cq_reap(struct pan_rvu_cq_info *cq_info, int budget)
{
	int workdone = 0, rq_cq_idx;
	struct otx2_cq_queue *rx_cq;
	struct otx2_qset *qset;
	struct otx2_nic *pfvf;
#if !IS_ENABLED(CONFIG_OCTEONTX_PAN_POLLING_MODE)
#if IS_ENABLED(CONFIG_OCTEONTX_PAN_TX_COMPLETION)
	struct otx2_hw *hw;
	struct otx2_cq_queue *tx_cq;
	int sq_idx, sq_cq_idx;
	int tx_workdone = 0;
	int i;
#endif
#endif

	pfvf = (struct otx2_nic *)cq_info->dev;
	qset = &pfvf->qset;

	rq_cq_idx = cq_info->rq2cqidx;
	if (unlikely(rq_cq_idx == CINT_INVALID_CQ))
		return false;

	rx_cq = &qset->cq[rq_cq_idx];
	BUG_ON(rx_cq->cq_type != CQ_RX);

	workdone = pan_rvu_rx_cq_reap(pfvf, cq_info, rx_cq, budget);
	pan_stats_add(PAN_STATS_FLD_RX_CQ_PKTS, (u32)workdone);

	if (rx_cq->pool_ptrs)
		pfvf->hw_ops->refill_pool_ptrs(pfvf, rx_cq);

#if !IS_ENABLED(CONFIG_OCTEONTX_PAN_POLLING_MODE)
#if IS_ENABLED(CONFIG_OCTEONTX_PAN_TX_COMPLETION)
	/* TODO: avoid iterating */
	hw = &pfvf->hw;
	for (i = 0; i < cq_info->sq_cnt; i++) {
		sq_cq_idx = cq_info->sq2cqidxs[i];
		sq_idx = sq_cq_idx - hw->rx_queues;
		tx_cq = &qset->cq[sq_cq_idx];
		tx_workdone += pan_rvu_tx_cq_reap(pfvf, tx_cq, sq_idx, budget);
	}
	pan_stats_add(PAN_STATS_FLD_TX_CQ_PKTS, (u32)tx_workdone);

	/* TODO: do we need to add tx work to total work done ? */
//	workdone += tx_workdone;
#endif

	/* Clear the IRQ */
	otx2_write64(pfvf, NIX_LF_CINTX_INT(cq_info->cint_idx), BIT_ULL(0));

	if (workdone < budget) {
		/* Re-enable interrupts */
		otx2_write64(pfvf, NIX_LF_CINTX_ENA_W1S(cq_info->cint_idx),
			     BIT_ULL(0));

		return false;
	}
#endif
	return true;
}

static irqreturn_t pan_cq_intr_fn(int irq, void *cq_irq)
{
	struct pan_rvu_cq_info *cq_info = (struct pan_rvu_cq_info *)cq_irq;

	cq_info->bh_cnt++;

	while (pan_rvu_cq_reap(cq_info, NAPI_POLL_WEIGHT))
		;

	return IRQ_HANDLED;
}

#if !IS_ENABLED(CONFIG_OCTEONTX_PAN_POLLING_MODE)

static irqreturn_t pan_cq_intr_handler(int irq, void *cq_irq)
{
	struct pan_rvu_cq_info *cq_info = (struct pan_rvu_cq_info *)cq_irq;
	struct otx2_nic *pf = (struct otx2_nic *)cq_info->dev;
	int qidx = cq_info->cint_idx;

	/* Disable interrupts.
	 *
	 * Completion interrupts behave in a level-triggered interrupt
	 * fashion, and hence have to be cleared only after it is serviced.
	 */
	otx2_write64(pf, NIX_LF_CINTX_ENA_W1C(qidx), BIT_ULL(0));
	pan_stats_inc(PAN_STATS_FLD_INTR);

	return IRQ_WAKE_THREAD;
}
#endif

static void pan_rvu_free_cints(struct otx2_nic *pfvf, int n)
{
	struct pan_rvu_dev_priv *pan_priv;
	struct otx2_hw *hw = &pfvf->hw;
	int irq, qidx;

	pan_priv = netdev_priv(pfvf->netdev);

	for (qidx = 0, irq = hw->nix_msixoff + NIX_LF_CINT_VEC_START;
	     qidx < n;
	     qidx++, irq++) {
		int vector = pci_irq_vector(pfvf->pdev, irq);

		irq_set_affinity_hint(vector, NULL);
		free_cpumask_var(hw->affinity_mask[irq]);
		free_irq(vector, &pan_priv->cq_info[qidx]);
	}
}

#if IS_ENABLED(CONFIG_OCTEONTX_PAN_POLLING_MODE)

struct pan_rvu_rx_thread_s {
	struct task_struct *iothread;
	struct pan_rvu_cq_info *cq_info;
	spinlock_t p_work_lock; /* Spin lock for threads */
};

static DEFINE_PER_CPU(struct pan_rvu_rx_thread_s, pan_rvu_rx_thread_per_cpu);

static int pan_rvu_rx_process_thread(void *arg)
{
	struct pan_rvu_rx_thread_s *p = arg;

	while (1)
		pan_cq_intr_fn(0, p->cq_info);
	return 0;
}

static int pan_rvu_rx_thread_start(unsigned int cpu, struct pan_rvu_cq_info *cq_info)
{
	struct pan_rvu_rx_thread_s *p = per_cpu_ptr(&pan_rvu_rx_thread_per_cpu, cpu);
	struct task_struct *thread;

	p->cq_info = cq_info;

	thread = kthread_create_on_node(pan_rvu_rx_process_thread, (void *)p,
					cpu_to_node(cpu),
					"rx_thread/%d", cpu);
	if (IS_ERR(thread))
		return PTR_ERR(thread);

	kthread_bind(thread, cpu);
	p->iothread = thread;
	wake_up_process(thread);
	return 0;
}

static int pan_rvu_rx_thread_stop(unsigned int cpu)
{
	struct pan_rvu_rx_thread_s *p = per_cpu_ptr(&pan_rvu_rx_thread_per_cpu, cpu);
	struct task_struct *thread;
	unsigned long flags;

	spin_lock_irqsave(&p->p_work_lock, flags);
	thread = p->iothread;
	p->iothread = NULL;

	spin_unlock_irqrestore(&p->p_work_lock, flags);
	if (thread)
		kthread_stop(thread);
	return 0;
}
#endif

static int pan_rvu_irq_init(struct net_device *netdev)
{
	struct pan_rvu_dev_priv *pan_priv;
	struct otx2_nic *otx2_nic;
#if IS_ENABLED(CONFIG_OCTEONTX_PAN_POLLING_MODE)
	int err = 0;
	int cpu;
#else
	int err = 0, qidx, vec;
	struct otx2_hw *hw;
	char *irq_name;
#endif

	pan_priv = netdev_priv(netdev);
	otx2_nic = pan_priv->otx2_nic;

#if IS_ENABLED(CONFIG_OCTEONTX_PAN_POLLING_MODE)
	for_each_online_cpu(cpu) {
		pan_rvu_rx_thread_start(cpu, &pan_priv->cq_info[cpu]);
	}
#endif

#if !IS_ENABLED(CONFIG_OCTEONTX_PAN_POLLING_MODE)
	hw = &otx2_nic->hw;
	/* Register CQ IRQ handlers */
	vec = hw->nix_msixoff + NIX_LF_CINT_VEC_START;
	for (qidx = 0; qidx < hw->cint_cnt; qidx++) {
		irq_name = &hw->irq_name[vec * NAME_SIZE];

		snprintf(irq_name, NAME_SIZE, "pan%d", qidx);

		err = request_threaded_irq(pci_irq_vector(otx2_nic->pdev, vec),
					   pan_cq_intr_handler, pan_cq_intr_fn,
					   0, irq_name,
					   &pan_priv->cq_info[qidx]);
		if (err) {
			dev_err(otx2_nic->dev,
				"RVU PAN IRQ registration failed for CQ%d\n", qidx);
			goto err_free_cints;
		}
		vec++;

		/* Enable CQ IRQ */
		otx2_write64(otx2_nic, NIX_LF_CINTX_INT(qidx), BIT_ULL(0));
		otx2_write64(otx2_nic, NIX_LF_CINTX_ENA_W1S(qidx), BIT_ULL(0));
	}
#endif
	otx2_nic->flags &= ~OTX2_FLAG_INTF_DOWN;

#if !IS_ENABLED(CONFIG_OCTEONTX_PAN_POLLING_MODE)
	/* Spread CINT */
	dup_set_cints_affinity(otx2_nic);
	return 0;

err_free_cints:
	pan_rvu_free_cints(otx2_nic, hw->cint_cnt);
	vec = pci_irq_vector(otx2_nic->pdev,
			     hw->nix_msixoff + NIX_LF_QINT_VEC_START);
	otx2_write64(otx2_nic, NIX_LF_QINTX_ENA_W1C(0), BIT_ULL(0));
	synchronize_irq(vec);
	free_irq(vec, otx2_nic);
#endif
	return err;
}

int pan_rvu_get_iface_info(struct iface_info *info, int *cnt, bool add_pan)
{
	struct iface_get_info_rsp *rsp;
	struct pan_rvu_dev_priv *priv;
	struct iface_info pan_info;
	struct otx2_nic *otx2_nic;
	struct pan_rvu_gbl_t *gbl;
	struct net_device *dev;
	struct msg_req *req;
	struct otx2_hw *hw;
	int ret;

	dev = dev_get_by_name(&init_net, PAN_DEV_NAME);
	if (!dev) {
		pr_err("Could not find PAN device\n");
		return -EFAULT;
	}
	dev_put(dev);
	priv = netdev_priv(dev);
	otx2_nic = priv->otx2_nic;
	hw = &otx2_nic->hw;

	mutex_lock(&otx2_nic->mbox.lock);

	req = otx2_mbox_alloc_msg_iface_get_info(&otx2_nic->mbox);

	ret = otx2_sync_mbox_msg(&otx2_nic->mbox);
	if (ret)
		goto done;

	rsp = (struct iface_get_info_rsp *)otx2_mbox_get_rsp
		(&otx2_nic->mbox.mbox, 0, &req->hdr);

	*cnt = rsp->cnt;

	if (info)
		memcpy(info, rsp->info, (*cnt) * sizeof(*info));

	/* TODO: Remove this hack ? */
	if (add_pan) {
		if (info) {
			memset(&pan_info, 0, sizeof(pan_info));
			info += *cnt;

			gbl = pan_rvu_get_gbl();
			pan_info.is_vf = 0;
			pan_info.pcifunc = priv->pcifunc;
			pan_info.rx_chan_base = hw->rx_chan_base;
			pan_info.tx_chan_base = hw->tx_chan_base;
			pan_info.rx_chan_cnt = hw->rx_chan_cnt;
			pan_info.tx_chan_cnt = hw->tx_chan_cnt;
			pan_info.sq_cnt = gbl->sqs_usable;

			/* TODO: Fix by getting the values */
			pan_info.cq_cnt = num_online_cpus();
			pan_info.rq_cnt = num_online_cpus();
			pan_info.tx_link = 14;

			memcpy(info, &pan_info, sizeof(*info));
		}

		(*cnt)++;
	}

done:
	mutex_unlock(&otx2_nic->mbox.lock);
	return ret;
}

/*TODO: fix passing rx queues and using it for num of CINT */
static int pan_rvu_cq_info_init(struct net_device *netdev)
{
	int err = 0, cnt, i, cidx;
	struct pan_rvu_dev_priv *pan_priv;
	struct pan_rvu_cq_info *cq_info;
	struct otx2_nic *otx2_nic;
	struct iface_info *ifinfo;
	struct iface_info *iter;
	struct otx2_hw *hw;
	int sqidx;

	pan_priv = netdev_priv(netdev);
	otx2_nic = pan_priv->otx2_nic;
	hw = &otx2_nic->hw;

	BUG_ON(hw->rx_queues > num_online_cpus());

	ifinfo = kcalloc(256 + 32, sizeof(*ifinfo), GFP_KERNEL);
	if (!ifinfo)
		return -ENOMEM;

	err = pan_rvu_get_iface_info(ifinfo, &cnt, false);
	if (err) {
		pr_err("Error happened while getting info\n");
		return err;
	}

	/* Register desc reap handlers */
	sqidx = 0;
	for (cidx = 0; cidx < hw->cint_cnt; cidx++) {
		cq_info = &pan_priv->cq_info[cidx];
		cq_info->cint_idx = cidx;
		cq_info->dev = (void *)pan_priv->otx2_nic;

		/* RX CQ info fill */
		cq_info->rq2cqidx = cidx;

		/* TX CQ info fill */
		cq_info->sq_cnt = cnt;

		/* TODO: free the memory while unloading the module */
		cq_info->sq2cqidxs = kcalloc(cnt, sizeof(u16), GFP_KERNEL);
		if (!cq_info->sq2cqidxs) {
			kfree(ifinfo);
			return -ENOMEM;
		}

		/* TODO: free the memory while unloading the module */
		cq_info->sq_info = kcalloc(cnt, sizeof(cq_info->sq_info),
					   GFP_KERNEL);
		if (!cq_info->sq_info) {
			pr_err("Sq info allocation failed\n");
			kfree(ifinfo);
			kfree(cq_info->sq2cqidxs);
			return -ENOMEM;
		}

		iter = ifinfo;
		for (i = 0; i < cnt; i++, iter++) {
			cq_info->sq2cqidxs[i] = hw->rx_queues + sqidx;
			cq_info->sq_info[i].pcifunc = iter->pcifunc;
			cq_info->sq_info[i].tx_chan = iter->tx_chan_base;
			cq_info->sq_info[i].sqidx = sqidx;
			cq_info->sq_info[i].is_sdp = iter->is_sdp;
			sqidx++;
			pan_rvu_gbl.sdp_cnt += !!iter->is_sdp;
		}
	}

	pan_rvu_gbl.sqs_usable = sqidx;
	kfree(ifinfo);
	return 0;
}

int pan_rvu_alloc_mcam_entry(void)
{
	struct npc_mcam_alloc_entry_req *req;
	struct npc_mcam_alloc_entry_rsp *rsp;
	struct otx2_flow_config *flow_cfg;
	struct otx2_nic *pan;

	pan = pci_get_drvdata(pci_get_device(PCI_VENDOR_ID_CAVIUM,
					     PCI_DEVID_PAN_RVU, NULL));
	if (!pan)
		return -ENODEV;
	flow_cfg = pan->flow_cfg;

	mutex_lock(&pan->mbox.lock);
	req = otx2_mbox_alloc_msg_npc_mcam_alloc_entry(&pan->mbox);
	if (!req)
		goto exit;

	req->contig = false;
	req->count = 1;

	/* Send message to AF */
	if (otx2_sync_mbox_msg(&pan->mbox))
		goto exit;
	rsp = (struct npc_mcam_alloc_entry_rsp *)otx2_mbox_get_rsp
		(&pan->mbox.mbox, 0, &req->hdr);
	flow_cfg->flow_ent[flow_cfg->max_flows++] = rsp->entry_list[0];

	mutex_unlock(&pan->mbox.lock);
	return 0;

exit:
	mutex_unlock(&pan->mbox.lock);
	return -ENOSPC;
}

static void otx2_free_mcam_entries(struct otx2_nic *pan)
{
	struct otx2_flow_config *flow_cfg = pan->flow_cfg;
	struct npc_mcam_free_entry_req *req;
	int ent, err;

	if (!flow_cfg->max_flows)
		return;

	mutex_lock(&pan->mbox.lock);
	for (ent = 0; ent < flow_cfg->max_flows; ent++) {
		req = otx2_mbox_alloc_msg_npc_mcam_free_entry(&pan->mbox);
		if (!req)
			break;

		req->entry = flow_cfg->flow_ent[ent];

		/* Send message to AF to free MCAM entries */
		err = otx2_sync_mbox_msg(&pan->mbox);
		if (err)
			break;
	}
	mutex_unlock(&pan->mbox.lock);
}

int pan_alloc_matchid(struct matchid_bmap *rsrc)
{
	int id;

	if (!rsrc->bmap)
		return -EINVAL;

	id = find_first_zero_bit(rsrc->bmap, rsrc->max);
	if (id >= rsrc->max)
		return -ENOSPC;

	__set_bit(id, rsrc->bmap);

	return id;
}

void pan_free_matchid(struct matchid_bmap *rsrc, int id)
{
	if (!rsrc->bmap)
		return;
	__clear_bit(id, rsrc->bmap);
}

int pan_rvu_install_flow(struct pan_tuple *tuple)
{
	u8 pan_mac_mask[ETH_ALEN] = { 0xff, 0xff, 0xff, 0xff, 0xff, 0xff};
	struct npc_install_flow_req *req;
	struct otx2_flow_config *flow_cfg;
	u32 pan_ipv4_mask = (BIT(32) - 1);
	u16 pan_port_mask = (BIT(16) - 1);
	struct flow_msg *pkt, *pmask;
	struct otx2_nic *pan;
	int index, err;

	pan = pci_get_drvdata(pci_get_device(PCI_VENDOR_ID_CAVIUM,
					     PCI_DEVID_PAN_RVU, NULL));
	if (!pan)
		return -ENODEV;

	flow_cfg = pan->flow_cfg;
	index = flow_cfg->flow_ent[flow_cfg->max_flows - 1]; //point to latest mcam index
	mutex_lock(&pan->mbox.lock);

	req = otx2_mbox_alloc_msg_npc_install_flow(&pan->mbox);
	if (!req) {
		mutex_unlock(&pan->mbox.lock);
		return -ENOMEM;
	}

	pkt = &req->packet;
	pmask = &req->mask;

	if (!is_zero_ether_addr(tuple->dmac)) {
		ether_addr_copy(pkt->dmac, tuple->dmac);
		ether_addr_copy(pkt->dmac, pan_mac_mask);
		req->features |= BIT_ULL(NPC_DMAC);
	}

	if (tuple->src_ip4.s_addr) {
		memcpy(&pkt->ip4src, &tuple->src_ip4.s_addr,
		       sizeof(pkt->ip4src));
		memcpy(&pmask->ip4src, &pan_ipv4_mask,
		       sizeof(pmask->ip4src));
		req->features |= BIT_ULL(NPC_SIP_IPV4);
	}

	if (tuple->dst_ip4.s_addr) {
		memcpy(&pkt->ip4dst, &tuple->dst_ip4.s_addr,
		       sizeof(pkt->ip4dst));
		memcpy(&pmask->ip4dst, &pan_ipv4_mask,
		       sizeof(pmask->ip4dst));
		req->features |= BIT_ULL(NPC_DIP_IPV4);
	}

	if (tuple->l4proto) {
		memcpy(&pkt->sport, &tuple->sport,
		       sizeof(pkt->sport));
		memcpy(&pmask->sport, &pan_port_mask,
		       sizeof(pmask->sport));
		memcpy(&pkt->dport, &tuple->dport,
		       sizeof(pkt->sport));
		memcpy(&pmask->dport, &pan_port_mask,
		       sizeof(pmask->sport));

		switch (tuple->l4proto) {
		case IPPROTO_TCP:
			req->features |= BIT_ULL(NPC_IPPROTO_TCP);
			if (tuple->sport)
				req->features |= BIT_ULL(NPC_SPORT_TCP);
			if (tuple->dport)
				req->features |= BIT_ULL(NPC_DPORT_TCP);
			break;
		case IPPROTO_UDP:
			req->features |= BIT_ULL(NPC_IPPROTO_UDP);
			if (tuple->sport)
				req->features |= BIT_ULL(NPC_SPORT_UDP);
			if (tuple->dport)
				req->features |= BIT_ULL(NPC_DPORT_UDP);
			break;
		}
	}

	req->entry = index;
	req->intf = NIX_INTF_RX;
	req->set_cntr = 1;
	req->op = NIX_RX_ACTIONOP_UCAST;
	req->match_id = tuple->hash;
	req->channel = 0x100;
	req->index = 1;
	err = otx2_sync_mbox_msg(&pan->mbox);
	mutex_unlock(&pan->mbox.lock);
	return err;
}

static int pan_rvu_open(struct net_device *netdev)
{
	struct pan_rvu_dev_priv *pan_priv;
	struct otx2_nic *otx2_nic;
	struct pan_rvu_gbl_t *gbl;
	struct iface_info *info;
	struct net_device *dev;
	int err, cnt, i;

	info = kcalloc(256 + 32, sizeof(*info), GFP_KERNEL);
	if (!info)
		return -ENOMEM;

	netif_carrier_on(netdev);

	err = pan_rvu_cq_info_init(netdev);
	if (err) {
		netdev_err(netdev, "Failed to init cq info\n");
		return err;
	}

	pan_priv = netdev_priv(netdev);
	otx2_nic = pan_priv->otx2_nic;

	err = pan_rvu_get_iface_info(info, &cnt, false);
	if (err) {
		netdev_err(netdev, "Error happened while getting info\n");
		return err;
	}

	for (i = 0; i < cnt; i++, info++) {
		dev = __pan_rvu_get_kernel_netdev_by_pcifunc(info->pcifunc);
		if (!dev)
			continue;
		xa_store(&pan_rvu_gbl.chan2dev,
			 info->rx_chan_base, dev, GFP_KERNEL);

		xa_store(&pan_rvu_gbl.pfunc2dev,
			 info->pcifunc, dev, GFP_KERNEL);
	}
	kfree(info);

	/* Store it for faster access in data path */
	pan_rvu_gbl.sqs_per_core = cnt;

	err = dup_init_hw_resources(otx2_nic);
	if (err) {
		netdev_err(netdev, "Failed init hw resources\n");
		return err;
	}

	/* Initialize RSS */
	err = otx2_rss_init(otx2_nic);
	if (err) {
		dev_err(otx2_nic->dev, "Failed to config RSS\n");
		goto err_free_hw_rsrc;
	}

	err = pan_rvu_irq_init(netdev);
	if (err) {
		netdev_err(netdev, "Failed to init irqs\n");
		goto err_free_hw_rsrc;
	}

	pan_rvu_gbl_init();

	err = pan_rvu_pcifunc2sq_off_map_create(otx2_nic);
	if (err) {
		netdev_err(netdev, "Failed to create pcifunc2sqoff map\n");
		/* TODO: fixup goto in all above cases to free mem */
		goto err_free_hw_rsrc;
	}

	gbl = pan_rvu_get_gbl();

	/* Set links for PAN */
	err = pan_tl_set_links(!!gbl->sdp_cnt);
	if (err) {
		pr_err("Error in setting links\n");
		goto err_free_hw_rsrc;
	}

	return 0;

err_free_hw_rsrc:
	dup_free_hw_resources(otx2_nic);
	return err;
}

static int pan_rvu_close(struct net_device *netdev)
{
	struct pan_rvu_dev_priv *pan_priv;
	struct otx2_nic *priv;
	struct otx2_hw *hw;
	int vec;
#if IS_ENABLED(CONFIG_OCTEONTX_PAN_POLLING_MODE)
	int cpu;
#endif

	pan_priv = netdev_priv(netdev);
	priv = pan_priv->otx2_nic;
	hw = &priv->hw;

	netif_carrier_off(netdev);

	otx2_write64(priv, NIX_LF_QINTX_ENA_W1C(0), BIT_ULL(0));
	vec = pci_irq_vector(priv->pdev,
			     hw->nix_msixoff + NIX_LF_QINT_VEC_START);
	synchronize_irq(vec);

#if IS_ENABLED(CONFIG_OCTEONTX_PAN_POLLING_MODE)
	for_each_online_cpu(cpu) {
		pan_rvu_rx_thread_stop(cpu);
	}
#endif

	pan_rvu_free_cints(priv, hw->cint_cnt);
	dup_free_hw_resources(priv);
	return 0;
}

static netdev_tx_t pan_rvu_xmit(struct sk_buff *skb, struct net_device *dev)
{
	skb_tx_timestamp(skb);
	dev_kfree_skb(skb);
	return NETDEV_TX_OK;
}

static const struct net_device_ops pan_netdev_ops = {
	.ndo_open		= pan_rvu_open,
	.ndo_stop		= pan_rvu_close,
	.ndo_start_xmit		= pan_rvu_xmit,
};

static int pan_rvu_alloc_queue_mem(struct otx2_nic *pf)
{
	struct otx2_qset *qset = &pf->qset;
	int err = -ENOMEM;

	/* RQ and SQs are mapped to different CQs,
	 * so find out max CQ IRQs (i.e CINTs) needed.
	 */
	pf->hw.non_qos_queues = pf->hw.tx_queues + pf->hw.xdp_queues;
	pf->hw.cint_cnt = num_online_cpus();
	pf->qset.cq_cnt = pf->hw.rx_queues + otx2_get_total_tx_queues(pf);

	/* CQ size of RQ */
	qset->rqe_cnt = qset->rqe_cnt ? qset->rqe_cnt : Q_COUNT(Q_SIZE_256);
	/* CQ size of SQ */
	qset->sqe_cnt = qset->sqe_cnt ? qset->sqe_cnt : Q_COUNT(Q_SIZE_4K);

	qset->cq = kcalloc(pf->qset.cq_cnt,
			   sizeof(struct otx2_cq_queue), GFP_KERNEL);
	if (!qset->cq)
		goto err_free_mem;

	qset->sq = kcalloc(otx2_get_total_tx_queues(pf) + pf->hw.tc_tx_queues,
			   sizeof(struct otx2_snd_queue), GFP_KERNEL);
	if (!qset->sq)
		goto err_free_mem;

	qset->rq = kcalloc(pf->hw.rx_queues,
			   sizeof(struct otx2_rcv_queue), GFP_KERNEL);
	if (!qset->rq)
		goto err_free_mem;

	return 0;

err_free_mem:
	dup_free_queue_mem(qset);
	return err;
}

static int pan_rvu_init_npc(struct otx2_nic *pan)
{
	struct otx2_flow_config *flow_cfg;
	struct matchid_bmap *rsrc;

	pan->flow_cfg = devm_kzalloc(pan->dev,
				     sizeof(struct otx2_flow_config),
				     GFP_KERNEL);
	if (!pan->flow_cfg)
		return -ENOMEM;

	flow_cfg = pan->flow_cfg;

	flow_cfg->flow_ent = devm_kmalloc_array(pan->dev, 3,
						sizeof(u16), GFP_KERNEL);
	if (!flow_cfg->flow_ent)
		return -ENOMEM;

	flow_cfg->max_flows = 0;

	/* init NPC matchid bmap */
	rsrc = &pan_rvu_gbl.rsrc;
	rsrc->max  = BIT(16) - 1;
	rsrc->bmap = kcalloc(BITS_TO_LONGS(rsrc->max),
			     sizeof(long), GFP_KERNEL);
	if (!rsrc->bmap)
		return -ENOMEM;

	/* Allocate match_id from 1 */
	__set_bit(0, rsrc->bmap);

	dup_rxtx_enable(pan, true);
	return 0;
}

static void pan_rvu_deinit_npc(struct otx2_nic *pan)
{
	struct otx2_flow_config *flow_cfg = pan->flow_cfg;

	otx2_free_mcam_entries(pan);
	devm_kfree(pan->dev, flow_cfg->flow_ent);
	flow_cfg->flow_ent = NULL;
	flow_cfg->max_flows = 0;
	devm_kfree(pan->dev, flow_cfg);

	dup_rxtx_enable(pan, false);
}

static int pan_switch_up_event_notify(struct otx2_nic *pan)
{
	struct swdev2af_notify_req *req;
	int ret = 0;

	mutex_lock(&pan->mbox.lock);
	req = otx2_mbox_alloc_msg_swdev2af_notify(&pan->mbox);
	if (!req) {
		ret = -ENOMEM;
		goto done;
	}

	req->msg_type = SWDEV2AF_MSG_TYPE_FW_STATUS;
	req->fw_up = true;
	req->pcifunc = pan->pcifunc;

	/* Send message to AF to free MCAM entries */
	ret = otx2_sync_mbox_msg(&pan->mbox);
done:
	mutex_unlock(&pan->mbox.lock);
	return ret;
}

static int pan_rvu_probe(struct pci_dev *pdev, const struct pci_device_id *id)
{
	struct device *dev = &pdev->dev;
	struct pan_rvu_dev_priv *pan_priv;
	struct otx2_nic *otx2_nic;
	struct net_device *netdev;
	struct otx2_hw *hw;
	int num_vec, err;
	int qcount;

	/* Not supported on 96xx or 98xx platforms */
	if (pdev->subsystem_device == PCI_SUBSYS_DEVID_98XX ||
	    pdev->subsystem_device == PCI_SUBSYS_DEVID_96XX) {
		dev_err(dev,
			"PAN is not supported in this platform (%#x)\n",
			pdev->subsystem_device);
		return -EOPNOTSUPP;
	}

	err = pcim_enable_device(pdev);
	if (err) {
		dev_err(dev, "Failed to enable PCI device\n");
		return err;
	}

	err = pci_request_regions(pdev, "bars");
	if (err) {
		dev_err(dev, "PCI request regions failed 0x%x\n", err);
		return err;
	}

	err = dma_set_mask_and_coherent(dev, DMA_BIT_MASK(48));
	if (err) {
		dev_err(dev, "DMA mask config failed, abort\n");
		goto err_release_regions;
	}

	pci_set_master(pdev);

	otx2_nic = devm_kzalloc(dev, sizeof(*otx2_nic), GFP_KERNEL);
	if (!otx2_nic) {
		err = -ENOMEM;
		dev_err(dev, "Allocation of otx2_nic failed\n");
		goto err_release_regions;
	}

	pci_set_drvdata(pdev, otx2_nic);
	otx2_nic->pdev = pdev;
	otx2_nic->dev = dev;

	hw = &otx2_nic->hw;
	hw->pdev = pdev;

	/* Map CSRs */
	otx2_nic->reg_base = pcim_iomap(pdev, PCI_CFG_REG_BAR_NUM, 0);
	if (!otx2_nic->reg_base) {
		dev_err(dev, "Unable to map physical function CSRs, aborting\n");
		err = -ENOMEM;
		goto err_free_otx2_nic;
	}

	err = dup_check_pf_usable(otx2_nic);
	if (err) {
		dev_err(dev, "Revision id is not configured yet, retry\n");
		goto err_free_otx2_nic;
	}

	num_vec = pci_msix_vec_count(pdev);
	hw->irq_name = devm_kmalloc_array(dev, num_vec, NAME_SIZE, GFP_KERNEL);
	if (!hw->irq_name) {
		dev_err(dev, "allocation of irq_name failed\n");
		err = -ENOMEM;
		goto err_free_otx2_nic;
	}

	hw->affinity_mask = devm_kcalloc(dev, num_vec, sizeof(cpumask_var_t), GFP_KERNEL);
	if (!hw->affinity_mask) {
		dev_err(dev, "allocation of irq_mask failed\n");
		err = -ENOMEM;
		goto err_free_irq_name;
	}

	err = pci_alloc_irq_vectors(pdev, RVU_PF_INT_VEC_CNT,
				    RVU_PF_INT_VEC_CNT, PCI_IRQ_MSIX);
	if (err < 0) {
		dev_err(dev, "%s: Failed to alloc %d IRQ vectors\n",
			__func__, num_vec);
		goto err_free_affinity_mask;
	}

	otx2_setup_dev_hw_settings(otx2_nic);

	qcount = min_t(int, num_online_cpus(), OTX2_MAX_CQ_CNT);
	hw->rx_queues = qcount;

	/* TODO: Lets us assume max 32 queues (32 interface active)
	 * We can't query af to get maximum SQs per core before
	 * mbox is initialized
	 */
	hw->tx_queues = 32 * num_online_cpus();
	pan_rvu_gbl.sqs_total = hw->tx_queues;
	hw->max_queues = OTX2_MAX_CQ_CNT;

	err = dup_pfaf_mbox_init(otx2_nic);
	if (err) {
		dev_err(dev, "PF AF mbox init failed\n");
		goto err_free_irq_vectors;
	}

	/* Register mailbox interrupt */
	err = dup_register_mbox_intr(otx2_nic, true);
	if (err) {
		dev_err(dev, "Register mbox intr failed\n");
		goto err_mbox_destroy;
	}

	err = dup_attach_npa_nix(otx2_nic);
	if (err) {
		dev_err(dev, "Failed to attach npa nix\n");
		goto err_disable_mbox_intr;
	}

	err = dup_realloc_msix_vectors(otx2_nic);
	if (err) {
		dev_err(dev, "failed to realloc msix vectors\n");
		goto err_detach_rsrc;
	}

	err = dup_cn10k_lmtst_init(otx2_nic);
	if (err) {
		dev_err(dev, "Failed to init lmtst\n");
		goto err_detach_rsrc;
	}

	otx2_nic->iommu_domain = iommu_get_domain_for_dev(dev);
	if (otx2_nic->iommu_domain)
		otx2_nic->iommu_domain_type =
			((struct iommu_domain *)otx2_nic->iommu_domain)->type;

	hw->rbuf_len = OTX2_DEFAULT_RBUF_LEN;
	otx2_nic->rbsize = ALIGN(OTX2_DEFAULT_RBUF_LEN, OTX2_ALIGN) + OTX2_HEAD_ROOM;

	hw->xqe_size = 128;

	err = pan_rvu_alloc_queue_mem(otx2_nic);
	if (err) {
		dev_err(dev, "Failed to alloc queues\n");
		goto err_detach_rsrc;
	}

	/* Dummy netdev */
	netdev = alloc_etherdev_mqs(sizeof(*pan_priv), qcount, qcount);
	if (!netdev) {
		dev_err(dev, "Could not allocate a netdev\n");
		err = -ENOMEM;
		goto err_dealloc_queues;
	}

	netdev->netdev_ops = &pan_netdev_ops;
	snprintf(netdev->name, sizeof(netdev->name), "%s", PAN_DEV_NAME);
	netdev->flags = IFF_NOARP;

	if (register_netdev(netdev))
		goto err_free_netdev;

	pan_priv = netdev_priv(netdev);
	pan_priv->otx2_nic = otx2_nic;
	pan_priv->pcifunc = otx2_nic->pcifunc;
	otx2_nic->netdev = netdev;

	otx2_nic->af_xdp_zc_qidx = bitmap_zalloc(qcount, GFP_KERNEL);
	if (!otx2_nic->af_xdp_zc_qidx) {
		dev_err(dev, "Error to alloc xdp zc qidx\n");
		goto err_free_netdev;
	}

	err = pan_rvu_init_npc(otx2_nic);
	if (err) {
		dev_err(dev, "Failed to configure NPC\n");
		goto err_free_netdev;
	}

	err = pan_switch_up_event_notify(otx2_nic);
	if (err) {
		dev_err(dev, "Failed to send swdev up notification to AF\n");
		goto err_free_netdev;
	}

	dev_info(dev, "Pan probe called successfully, pci_func of PAN=0x%x\n", otx2_nic->pcifunc);

	return 0;

err_free_netdev:
	free_netdev(netdev);

err_dealloc_queues:
	dup_free_queue_mem(&otx2_nic->qset);

err_detach_rsrc:
	if (otx2_nic->hw.lmt_info)
		free_percpu(otx2_nic->hw.lmt_info);
	if (test_bit(CN10K_LMTST, &otx2_nic->hw.cap_flag))
		qmem_free(otx2_nic->dev, otx2_nic->dync_lmt);
	dup_detach_resources(&otx2_nic->mbox);
err_disable_mbox_intr:
	dup_disable_mbox_intr(otx2_nic);

err_mbox_destroy:
	dup_pfaf_mbox_destroy(otx2_nic);

err_free_irq_vectors:
	pci_free_irq_vectors(pdev);

err_free_affinity_mask:
	devm_kfree(dev, hw->affinity_mask);

err_free_irq_name:
	devm_kfree(dev, hw->irq_name);

err_free_otx2_nic:
	devm_kfree(dev, otx2_nic);

err_release_regions:
	pci_set_drvdata(pdev, NULL);
	pci_release_regions(pdev);
	return err;
}

static void pan_rvu_remove(struct pci_dev *pdev)
{
	struct otx2_nic *priv = pci_get_drvdata(pdev);
	struct device *dev = &pdev->dev;

	pan_rvu_deinit_npc(priv);
	unregister_netdev(priv->netdev);
	free_netdev(priv->netdev);
	priv->netdev = NULL;
	dup_free_queue_mem(&priv->qset);

	if (priv->hw.lmt_info)
		free_percpu(priv->hw.lmt_info);
	if (test_bit(CN10K_LMTST, &priv->hw.cap_flag))
		qmem_free(priv->dev, priv->dync_lmt);

	dup_detach_resources(&priv->mbox);
	dup_disable_mbox_intr(priv);
	dup_pfaf_mbox_destroy(priv);
	pci_free_irq_vectors(priv->pdev);
	devm_kfree(dev, priv->hw.irq_name);
	devm_kfree(dev, priv->hw.affinity_mask);
	devm_kfree(dev, priv);
	pci_set_drvdata(pdev, NULL);
	pci_release_regions(pdev);
}

static struct pci_driver pan_rvu_driver = {
	.name = DRV_NAME,
	.id_table = pan_rvu_id_table,
	.probe = pan_rvu_probe,
	.remove = pan_rvu_remove,
	.shutdown = pan_rvu_remove,
};

static int pan_rvu_cq_init(struct otx2_nic *pfvf, u16 qidx, u16 __maybe_unused unused)
{
	struct otx2_qset *qset = &pfvf->qset;
	int err, pool_id, non_xdp_queues;
	struct nix_aq_enq_req *aq;
	struct otx2_cq_queue *cq;

	cq = &qset->cq[qidx];
	cq->cq_idx = qidx;
	non_xdp_queues = pfvf->hw.rx_queues + pfvf->hw.tx_queues;
	if (qidx < pfvf->hw.rx_queues) {
		cq->cq_type = CQ_RX;
		cq->cint_idx = qidx;
		cq->cqe_cnt = qset->rqe_cnt;
		if (pfvf->xdp_prog)
			xdp_rxq_info_reg(&cq->xdp_rxq, pfvf->netdev, qidx, 0);
	} else if (qidx < non_xdp_queues) {
		cq->cq_type = CQ_TX;
		cq->cint_idx = (qidx - pfvf->hw.rx_queues) / pan_rvu_gbl.sqs_per_core;
		cq->cqe_cnt = qset->sqe_cnt;
	} else {
		if (pfvf->hw.xdp_queues &&
		    qidx < non_xdp_queues + pfvf->hw.xdp_queues) {
			cq->cq_type = CQ_XDP;
			cq->cint_idx = qidx - non_xdp_queues;
			cq->cqe_cnt = qset->sqe_cnt;
		} else {
			cq->cq_type = CQ_QOS;
			cq->cint_idx = qidx - non_xdp_queues -
				       pfvf->hw.xdp_queues;
			cq->cqe_cnt = qset->sqe_cnt;
		}
	}
	cq->cqe_size = pfvf->qset.xqe_size;

	/* Allocate memory for CQEs */
	err = qmem_alloc(pfvf->dev, &cq->cqe, cq->cqe_cnt, cq->cqe_size);
	if (err)
		return err;

	/* Save CQE CPU base for faster reference */
	cq->cqe_base = cq->cqe->base;
	/* In case where all RQs auras point to single pool,
	 * all CQs receive buffer pool also point to same pool.
	 */
	pool_id = ((cq->cq_type == CQ_RX) &&
		   (pfvf->hw.rqpool_cnt != pfvf->hw.rx_queues)) ? 0 : qidx;
	cq->rbpool = &qset->pool[pool_id];
	cq->refill_task_sched = false;
	cq->pend_cqe = 0;

	/* Get memory to put this msg */
	aq = otx2_mbox_alloc_msg_nix_aq_enq(&pfvf->mbox);
	if (!aq)
		return -ENOMEM;

	aq->cq.ena = 1;
	aq->cq.qsize = Q_SIZE(cq->cqe_cnt, 4);
	aq->cq.caching = 1;
	aq->cq.base = cq->cqe->iova;
	aq->cq.cint_idx = cq->cint_idx;
	aq->cq.cq_err_int_ena = NIX_CQERRINT_BITS;
	aq->cq.qint_idx = 0;
	aq->cq.avg_level = 255;

	if (qidx < pfvf->hw.rx_queues) {
		aq->cq.drop = RQ_DROP_LVL_CQ(pfvf->hw.rq_skid, cq->cqe_cnt);
		aq->cq.drop_ena = 1;

		if (!is_otx2_lbkvf(pfvf->pdev)) {
			/* Enable receive CQ backpressure */
			aq->cq.bp_ena = 1;
#ifdef CONFIG_DCB
			if (pfvf->queue_to_pfc_map)
				aq->cq.bpid = pfvf->bpid[pfvf->queue_to_pfc_map[qidx]];
#else
			aq->cq.bpid = pfvf->bpid[0];
#endif

			/* Set backpressure level is same as cq pass level */
			aq->cq.bp = RQ_PASS_LVL_CQ(pfvf->hw.rq_skid, qset->rqe_cnt);
		}
	}

	/* Fill AQ info */
	aq->qidx = qidx;
	aq->ctype = NIX_AQ_CTYPE_CQ;
	aq->op = NIX_AQ_INSTOP_INIT;

	return otx2_sync_mbox_msg(&pfvf->mbox);
}

static int pan_rvu_sq_aq_init(struct otx2_nic *pfvf, u16 qidx,
			      u16 chan, u16 sqb_aura, int smq)
{
	struct nix_cn10k_aq_enq_req *aq;

	/* Get memory to put this msg */
	aq = otx2_mbox_alloc_msg_nix_cn10k_aq_enq(&pfvf->mbox);
	if (!aq)
		return -ENOMEM;

	aq->sq.cq = pfvf->hw.rx_queues + qidx;
	aq->sq.max_sqe_size = NIX_MAXSQESZ_W16; /* 128 byte */
	aq->sq.cq_ena = 1;
	aq->sq.ena = 1;
	aq->sq.smq = smq;
	aq->sq.smq_rr_weight = mtu_to_dwrr_weight(pfvf, pfvf->tx_max_pktlen);
	aq->sq.default_chan = chan;
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

static int pan_rvu_sq_init(struct otx2_nic *pfvf, u16 qidx, u16 sqb_aura)
{
	struct otx2_qset *qset = &pfvf->qset;
	struct otx2_snd_queue *sq;
	struct otx2_pool *pool;
	u16 tx_chan;
	bool is_sdp;
	int err;
	u16 smq;

	tx_chan = pfvf->hw.tx_chan_base;
	if (qidx < pan_rvu_gbl.sqs_usable) {
		err = pan_rvu_get_sq_chan(pfvf, qidx, &tx_chan, &is_sdp);
		if (err) {
			pr_err("Failed to get tx_chan for sqidx=%u\n", qidx);
			/*TODO: should we return error */
			return 0;
		}
	}

	pool = &pfvf->qset.pool[sqb_aura];
	sq = &qset->sq[qidx];
	sq->sqe_size = NIX_SQESZ_W16 ? 64 : 128;
	sq->sqe_cnt = qset->sqe_cnt;

	err = qmem_alloc(pfvf->dev, &sq->sqe, 1, sq->sqe_size);
	if (err)
		return err;

	if (qidx < pfvf->hw.tx_queues) {
		err = qmem_alloc(pfvf->dev, &sq->tso_hdrs, qset->sqe_cnt,
				 TSO_HEADER_SIZE);
		if (err)
			return err;
	}

	sq->sqe_base = sq->sqe->base;
	sq->sg = kcalloc(qset->sqe_cnt, sizeof(struct sg_list), GFP_ATOMIC);
	if (!sq->sg)
		return -ENOMEM;

	if (pfvf->ptp && qidx < pfvf->hw.tx_queues) {
		err = qmem_alloc(pfvf->dev, &sq->timestamps, qset->sqe_cnt,
				 sizeof(*sq->timestamps));
		if (err)
			return err;
	}

	sq->head = 0;
	sq->cons_head = 0;
	sq->sqe_per_sqb = (pfvf->hw.sqb_size / sq->sqe_size) - 1;
	sq->num_sqbs = (qset->sqe_cnt + sq->sqe_per_sqb) / sq->sqe_per_sqb;
	/* Set SQE threshold to 10% of total SQEs */
	sq->sqe_thresh = ((sq->num_sqbs * sq->sqe_per_sqb) * 10) / 100;
	sq->aura_id = sqb_aura;
	sq->aura_fc_addr = pool->fc_addr->base;
	sq->io_addr = (__force u64)otx2_get_regaddr(pfvf, NIX_LF_OP_SENDX(0));

	sq->stats.bytes = 0;
	sq->stats.pkts = 0;

#if 0
	/* TODO: remove this store and restore hack */
	tmp = pfvf->hw.tx_chan_base;
	pfvf->hw.tx_chan_base = tx_chan;
//	chan_offset = qidx % pfvf->hw.tx_chan_cnt;
	err = pfvf->hw_ops->sq_aq_init(pfvf, qidx, 0, sqb_aura);
	if (err) {
		pr_err("Failed AQ operation for sqidx = %u\n", qidx);
		return err;
	}
	pfvf->hw.tx_chan_base = tmp;
#endif

	smq = pfvf->hw.txschq_list[NIX_TXSCH_LVL_SMQ][!!is_sdp];
	err =  pan_rvu_sq_aq_init(pfvf, qidx, tx_chan, sqb_aura, smq);
	return 0;
}

#if IS_ENABLED(CONFIG_OCTEONTX_PAN_TX_COMPLETION)

static void pan_rvu_cleanup_tx_cqes(struct otx2_nic *pfvf, struct otx2_cq_queue *cq,
				    int __maybe_unused idx)
{
	int tx_pkts = 0, tx_bytes = 0;
	struct sk_buff *skb = NULL;
	struct otx2_snd_queue *sq;
	struct nix_cqe_tx_s *cqe;
	struct netdev_queue *txq;
	struct otx2_pool *pool;
	int processed_cqe = 0;
	struct sg_list *sg;
	struct page *page;
	u16 pool_id;
	int qidx;
	u64 iova;
	u64 pa;

	qidx = cq->cq_idx - pfvf->hw.rx_queues;
	sq = &pfvf->qset.sq[qidx];

	if (dup_nix_cq_op_status(pfvf, cq) || !cq->pend_cqe)
		return;

	while (cq->pend_cqe) {
		cqe = (struct nix_cqe_tx_s *)dup_get_next_cqe(cq);
		processed_cqe++;
		cq->pend_cqe--;

		if (!cqe)
			continue;
		sg = &sq->sg[cqe->comp.sqe_id];
		skb = (struct sk_buff *)sg->skb;
		BUG_ON(skb);

		BUG_ON(!(sg->flags & SG_LIST_FLAG_LAST_FRAG));

		BUG_ON(sg->cq_idx > num_online_cpus());

		/* TODO: cq_idx will be 1:1 mapping to RQ. Is this assumption
		 * will be wrong anywhere ?
		 */
		pool_id = otx2_get_pool_idx(pfvf, AURA_NIX_RQ, sg->cq_idx);
		pool = &pfvf->qset.pool[pool_id];

		/* TODO: Fix if packet getting modified or in case of
		 * SG
		 */
		iova = sg->dma_addr[0] - OTX2_HEAD_ROOM;
		pa = otx2_iova_to_phys(pfvf->iommu_domain, iova);
		page = virt_to_head_page(phys_to_virt(pa));
		page_pool_put_full_page(pool->page_pool, page, true);
	}

	if (likely(tx_pkts)) {
		if (qidx >= pfvf->hw.tx_queues)
			qidx -= pfvf->hw.xdp_queues;
		txq = netdev_get_tx_queue(pfvf->netdev, qidx);
		netdev_tx_completed_queue(txq, tx_pkts, tx_bytes);
	}
	/* Free CQEs to HW */
	otx2_write64(pfvf, NIX_LF_CQ_OP_DOOR,
		     ((u64)cq->cq_idx << 32) | processed_cqe);
}

#endif

static struct otx2_cmn_fops pan_cmn_fops = {
	.sq_init = pan_rvu_sq_init,
	.rq_init = dup_rq_init,
	.cq_init = pan_rvu_cq_init,
	.rx_cq_clean = dup_cleanup_rx_cqes,
#if IS_ENABLED(CONFIG_OCTEONTX_PAN_TX_COMPLETION)
	.tx_cq_clean = pan_rvu_cleanup_tx_cqes,
#endif
	.tx_schq_init = pan_tl_txschq_rsrcs,
	.tx_schq_free_one = pan_tl_txschq_free_one,
};

int pan_rvu_init(void)
{
#if IS_ENABLED(CONFIG_OCTEONTX_PAN_POLLING_MODE)
	struct pan_rvu_rx_thread_s *p;
	int cpu;

	for_each_possible_cpu(cpu) {
		p = &per_cpu(pan_rvu_rx_thread_per_cpu, cpu);
		spin_lock_init(&p->p_work_lock);
		p->iothread = NULL;
	}

#endif
	otx2_cmn_fops_arr_add(PCI_DEVID_PAN_RVU, &pan_cmn_fops);
	xa_init(&pan_rvu_gbl.chan2dev);
	xa_init(&pan_rvu_gbl.pfunc2dev);
	return pci_register_driver(&pan_rvu_driver);
}

void pan_rvu_deinit(void)
{
	xa_destroy(&pan_rvu_gbl.pcifunc2sqoff);
	xa_destroy(&pan_rvu_gbl.pfunc2dev);
	xa_destroy(&pan_rvu_gbl.chan2dev);
	pci_unregister_driver(&pan_rvu_driver);
	otx2_cmn_fops_arr_del(PCI_DEVID_PAN_RVU);
}
