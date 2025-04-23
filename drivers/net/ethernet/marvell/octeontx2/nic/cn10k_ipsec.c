// SPDX-License-Identifier: GPL-2.0
/* Marvell IPSEC offload driver
 *
 * Copyright (C) 2024 Marvell.
 */

#include <net/xfrm.h>
#include <linux/netdevice.h>
#include <linux/bitfield.h>
#include <crypto/aead.h>
#include <crypto/gcm.h>

#include "otx2_common.h"
#include "otx2_struct.h"
#include "cn10k_ipsec.h"

static bool is_dev_support_ipsec_offload(struct pci_dev *pdev)
{
	return is_dev_cn10ka_b0(pdev) || is_dev_cn10kb(pdev);
}

static bool cn10k_cpt_device_set_inuse(struct otx2_nic *pf)
{
	enum cn10k_cpt_hw_state_e state;

	while (true) {
		state = atomic_cmpxchg(&pf->ipsec.cpt_state,
				       CN10K_CPT_HW_AVAILABLE,
				       CN10K_CPT_HW_IN_USE);
		if (state == CN10K_CPT_HW_AVAILABLE)
			return true;
		if (state == CN10K_CPT_HW_UNAVAILABLE)
			return false;

		mdelay(1);
	}
}

static void cn10k_cpt_device_set_available(struct otx2_nic *pf)
{
	atomic_set(&pf->ipsec.cpt_state, CN10K_CPT_HW_AVAILABLE);
}

static void  cn10k_cpt_device_set_unavailable(struct otx2_nic *pf)
{
	atomic_set(&pf->ipsec.cpt_state, CN10K_CPT_HW_UNAVAILABLE);
}

static int cn10k_outb_cptlf_attach(struct otx2_nic *pf)
{
	struct rsrc_attach *attach;
	int ret = -ENOMEM;

	mutex_lock(&pf->mbox.lock);
	/* Get memory to put this msg */
	attach = otx2_mbox_alloc_msg_attach_resources(&pf->mbox);
	if (!attach)
		goto unlock;

	attach->cptlfs = true;
	attach->modify = true;

	/* Send attach request to AF */
	ret = otx2_sync_mbox_msg(&pf->mbox);

unlock:
	mutex_unlock(&pf->mbox.lock);
	return ret;
}

static int cn10k_outb_cptlf_detach(struct otx2_nic *pf)
{
	struct rsrc_detach *detach;
	int ret = -ENOMEM;

	mutex_lock(&pf->mbox.lock);
	detach = otx2_mbox_alloc_msg_detach_resources(&pf->mbox);
	if (!detach)
		goto unlock;

	detach->partial = true;
	detach->cptlfs = true;

	/* Send detach request to AF */
	ret = otx2_sync_mbox_msg(&pf->mbox);

unlock:
	mutex_unlock(&pf->mbox.lock);
	return ret;
}

static int cn10k_outb_cptlf_alloc(struct otx2_nic *pf)
{
	struct cpt_lf_alloc_req_msg *req;
	int ret = -ENOMEM;

	mutex_lock(&pf->mbox.lock);
	req = otx2_mbox_alloc_msg_cpt_lf_alloc(&pf->mbox);
	if (!req)
		goto unlock;

	/* PF function */
	req->nix_pf_func = pf->pcifunc;
	/* Enable SE-IE Engine Group */
	req->eng_grpmsk = 1 << CN10K_DEF_CPT_IPSEC_EGRP;

	ret = otx2_sync_mbox_msg(&pf->mbox);

unlock:
	mutex_unlock(&pf->mbox.lock);
	return ret;
}

static void cn10k_outb_cptlf_free(struct otx2_nic *pf)
{
	mutex_lock(&pf->mbox.lock);
	otx2_mbox_alloc_msg_cpt_lf_free(&pf->mbox);
	otx2_sync_mbox_msg(&pf->mbox);
	mutex_unlock(&pf->mbox.lock);
}

static int cn10k_outb_cptlf_config(struct otx2_nic *pf)
{
	struct cpt_inline_ipsec_cfg_msg *req;
	int ret = -ENOMEM;

	mutex_lock(&pf->mbox.lock);
	req = otx2_mbox_alloc_msg_cpt_inline_ipsec_cfg(&pf->mbox);
	if (!req)
		goto unlock;

	req->dir = CPT_INLINE_OUTBOUND;
	req->enable = 1;
	req->nix_pf_func = pf->pcifunc;
	ret = otx2_sync_mbox_msg(&pf->mbox);
unlock:
	mutex_unlock(&pf->mbox.lock);
	return ret;
}

static void cn10k_outb_cptlf_iq_enable(struct otx2_nic *pf)
{
	u64 reg_val;

	/* Set Execution Enable of instruction queue */
	reg_val = otx2_read64(pf, CN10K_CPT_LF_INPROG);
	reg_val |= BIT_ULL(16);
	otx2_write64(pf, CN10K_CPT_LF_INPROG, reg_val);

	/* Set iqueue's enqueuing */
	reg_val = otx2_read64(pf, CN10K_CPT_LF_CTL);
	reg_val |= BIT_ULL(0);
	otx2_write64(pf, CN10K_CPT_LF_CTL, reg_val);
}

static void cn10k_outb_cptlf_iq_disable(struct otx2_nic *pf)
{
	u32 inflight, grb_cnt, gwb_cnt;
	u32 nq_ptr, dq_ptr;
	int timeout = 20;
	u64 reg_val;
	int cnt;

	/* Disable instructions enqueuing */
	otx2_write64(pf, CN10K_CPT_LF_CTL, 0ull);

	/* Wait for instruction queue to become empty.
	 * CPT_LF_INPROG.INFLIGHT count is zero
	 */
	do {
		reg_val = otx2_read64(pf, CN10K_CPT_LF_INPROG);
		inflight = FIELD_GET(CPT_LF_INPROG_INFLIGHT, reg_val);
		if (!inflight)
			break;

		usleep_range(10000, 20000);
		if (timeout-- < 0) {
			netdev_err(pf->netdev, "Timeout to cleanup CPT IQ\n");
			break;
		}
	} while (1);

	/* Disable executions in the LF's queue,
	 * the queue should be empty at this point
	 */
	reg_val &= ~BIT_ULL(16);
	otx2_write64(pf, CN10K_CPT_LF_INPROG, reg_val);

	/* Wait for instruction queue to become empty */
	cnt = 0;
	do {
		reg_val = otx2_read64(pf, CN10K_CPT_LF_INPROG);
		if (reg_val & BIT_ULL(31))
			cnt = 0;
		else
			cnt++;
		reg_val = otx2_read64(pf, CN10K_CPT_LF_Q_GRP_PTR);
		nq_ptr = FIELD_GET(CPT_LF_Q_GRP_PTR_DQ_PTR, reg_val);
		dq_ptr = FIELD_GET(CPT_LF_Q_GRP_PTR_DQ_PTR, reg_val);
	} while ((cnt < 10) && (nq_ptr != dq_ptr));

	cnt = 0;
	do {
		reg_val = otx2_read64(pf, CN10K_CPT_LF_INPROG);
		inflight = FIELD_GET(CPT_LF_INPROG_INFLIGHT, reg_val);
		grb_cnt = FIELD_GET(CPT_LF_INPROG_GRB_CNT, reg_val);
		gwb_cnt = FIELD_GET(CPT_LF_INPROG_GWB_CNT, reg_val);
		if (inflight == 0 && gwb_cnt < 40 &&
		    (grb_cnt == 0 || grb_cnt == 40))
			cnt++;
		else
			cnt = 0;
	} while (cnt < 10);
}

/* Allocate memory for CPT outbound Instruction queue.
 * Instruction queue memory format is:
 *      -----------------------------
 *     | Instruction Group memory    |
 *     |  (CPT_LF_Q_SIZE[SIZE_DIV40] |
 *     |   x 16 Bytes)               |
 *     |                             |
 *      ----------------------------- <-- CPT_LF_Q_BASE[ADDR]
 *     | Flow Control (128 Bytes)    |
 *     |                             |
 *      -----------------------------
 *     |  Instruction Memory         |
 *     |  (CPT_LF_Q_SIZE[SIZE_DIV40] |
 *     |   × 40 × 64 bytes)          |
 *     |                             |
 *      -----------------------------
 */
static int cn10k_outb_cptlf_iq_alloc(struct otx2_nic *pf)
{
	struct cn10k_cpt_inst_queue *iq = &pf->ipsec.iq;

	iq->size = CN10K_CPT_INST_QLEN_BYTES + CN10K_CPT_Q_FC_LEN +
		    CN10K_CPT_INST_GRP_QLEN_BYTES + OTX2_ALIGN;

	iq->real_vaddr = dma_alloc_coherent(pf->dev, iq->size,
					    &iq->real_dma_addr, GFP_KERNEL);
	if (!iq->real_vaddr)
		return -ENOMEM;

	/* iq->vaddr/dma_addr points to Flow Control location */
	iq->vaddr = iq->real_vaddr + CN10K_CPT_INST_GRP_QLEN_BYTES;
	iq->dma_addr = iq->real_dma_addr + CN10K_CPT_INST_GRP_QLEN_BYTES;

	/* Align pointers */
	iq->vaddr = PTR_ALIGN(iq->vaddr, OTX2_ALIGN);
	iq->dma_addr = PTR_ALIGN(iq->dma_addr, OTX2_ALIGN);
	return 0;
}

static void cn10k_outb_cptlf_iq_free(struct otx2_nic *pf)
{
	struct cn10k_cpt_inst_queue *iq = &pf->ipsec.iq;

	if (iq->real_vaddr)
		dma_free_coherent(pf->dev, iq->size, iq->real_vaddr,
				  iq->real_dma_addr);

	iq->real_vaddr = NULL;
	iq->vaddr = NULL;
}

static int cn10k_outb_cptlf_iq_init(struct otx2_nic *pf)
{
	u64 reg_val;
	int ret;

	/* Allocate Memory for CPT IQ */
	ret = cn10k_outb_cptlf_iq_alloc(pf);
	if (ret)
		return ret;

	/* Disable IQ */
	cn10k_outb_cptlf_iq_disable(pf);

	/* Set IQ base address */
	otx2_write64(pf, CN10K_CPT_LF_Q_BASE, pf->ipsec.iq.dma_addr);

	/* Set IQ size */
	reg_val = FIELD_PREP(CPT_LF_Q_SIZE_DIV40, CN10K_CPT_SIZE_DIV40 +
			     CN10K_CPT_EXTRA_SIZE_DIV40);
	otx2_write64(pf, CN10K_CPT_LF_Q_SIZE, reg_val);

	return 0;
}

static int cn10k_outb_cptlf_init(struct otx2_nic *pf)
{
	int ret;

	/* Initialize CPTLF Instruction Queue (IQ) */
	ret = cn10k_outb_cptlf_iq_init(pf);
	if (ret)
		return ret;

	/* Configure CPTLF for outbound ipsec offload */
	ret = cn10k_outb_cptlf_config(pf);
	if (ret)
		goto iq_clean;

	/* Enable CPTLF IQ */
	cn10k_outb_cptlf_iq_enable(pf);
	return 0;
iq_clean:
	cn10k_outb_cptlf_iq_free(pf);
	return ret;
}

static int cn10k_outb_cpt_init(struct net_device *netdev)
{
	struct otx2_nic *pf = netdev_priv(netdev);
	int ret;

	/* Attach a CPT LF for outbound ipsec offload */
	ret = cn10k_outb_cptlf_attach(pf);
	if (ret)
		return ret;

	/* Allocate a CPT LF for outbound ipsec offload */
	ret = cn10k_outb_cptlf_alloc(pf);
	if (ret)
		goto detach;

	/* Initialize the CPTLF for outbound ipsec offload */
	ret = cn10k_outb_cptlf_init(pf);
	if (ret)
		goto lf_free;

	pf->ipsec.io_addr = (__force u64)otx2_get_regaddr(pf,
						CN10K_CPT_LF_NQX(0));

	return 0;

lf_free:
	cn10k_outb_cptlf_free(pf);
detach:
	cn10k_outb_cptlf_detach(pf);
	return ret;
}

struct nix_wqe_rx_s *cn10k_ipsec_process_cpt_metapkt(struct otx2_nic *pfvf,
						     struct sk_buff *skb,
						     dma_addr_t seg_addr)
{
	struct nix_wqe_rx_s *wqe = NULL;
	struct cpt_parse_hdr_s *cptp;
	struct xfrm_offload *xo;
	struct xfrm_state *xs;
	struct sec_path *sp;
	dma_addr_t wqe_iova;
	u32 sa_index;
	u64 *sa_ptr;

	/* CPT_PARSE_HDR_S is present in the beginning of the buffer */
	cptp = phys_to_virt(otx2_iova_to_phys(pfvf->iommu_domain, seg_addr));

	/* Convert the wqe_ptr from CPT_PARSE_HDR_S to a CPU usable pointer */
	wqe_iova = FIELD_GET(CPT_PARSE_HDR_W1_WQE_PTR, cptp->w1);
	wqe = phys_to_virt(otx2_iova_to_phys(pfvf->iommu_domain,
					     be64_to_cpu((__force __be64)wqe_iova)));

	/* Get the XFRM state pointer stored in SA context */
	sa_index = FIELD_GET(CPT_PARSE_HDR_W0_COOKIE, cptp->w0);
	sa_ptr = pfvf->ipsec.inb_sa->base + 1024 +
		 (be32_to_cpu((__force __be32)sa_index) * pfvf->ipsec.sa_tbl_entry_sz);
	xs = (struct xfrm_state *)*sa_ptr;

	/* Set XFRM offload status and flags for successful decryption */
	sp = secpath_set(skb);
	if (!sp) {
		netdev_err(pfvf->netdev, "Failed to secpath_set\n");
		wqe = NULL;
		goto err_out;
	}

	rcu_read_lock();
	xfrm_state_hold(xs);
	rcu_read_unlock();

	sp->xvec[sp->len++] = xs;
	sp->olen++;

	xo = xfrm_offload(skb);
	xo->flags = CRYPTO_DONE;
	xo->status = CRYPTO_SUCCESS;

err_out:
	return wqe;
}

static int cn10k_inb_alloc_mcam_entry(struct otx2_nic *pfvf,
				      struct cn10k_inb_sw_ctx_info *inb_ctx_info)
{
	struct otx2_flow_config *flow_cfg = pfvf->flow_cfg;
	struct npc_mcam_alloc_entry_req *mcam_req;
	struct npc_mcam_alloc_entry_rsp *mcam_rsp;
	int err = 0;

	if (!pfvf->flow_cfg || !flow_cfg->flow_ent)
		return -ENODEV;

	mutex_lock(&pfvf->mbox.lock);

	/* Request an MCAM entry to install UCAST_IPSEC rule */
	mcam_req = otx2_mbox_alloc_msg_npc_mcam_alloc_entry(&pfvf->mbox);
	if (!mcam_req) {
		err = -ENOMEM;
		goto out;
	}

	mcam_req->contig = false;
	mcam_req->count = 1;
	mcam_req->ref_entry = flow_cfg->flow_ent[0];
	mcam_req->ref_prio = NPC_MCAM_HIGHER_PRIO;

	if (otx2_sync_mbox_msg(&pfvf->mbox)) {
		err = -ENODEV;
		goto out;
	}

	mcam_rsp = (struct npc_mcam_alloc_entry_rsp *)otx2_mbox_get_rsp(&pfvf->mbox.mbox,
									0, &mcam_req->hdr);

	/* Store NPC MCAM entry for bookkeeping */
	inb_ctx_info->npc_mcam_entry = mcam_rsp->entry_list[0];

out:
	mutex_unlock(&pfvf->mbox.lock);
	return err;
}

static int cn10k_inb_install_flow(struct otx2_nic *pfvf,
				  struct cn10k_inb_sw_ctx_info *inb_ctx_info)
{
	struct npc_install_flow_req *req;
	int err;

	/* Allocate an MCAM entry if not previously allocated */
	if (!inb_ctx_info->npc_mcam_entry) {
		err = cn10k_inb_alloc_mcam_entry(pfvf, inb_ctx_info);
		if (err) {
			netdev_err(pfvf->netdev,
				   "Failed to allocate MCAM entry for Inbound IPsec flow\n");
			goto out;
		}
	}

	mutex_lock(&pfvf->mbox.lock);

	req = otx2_mbox_alloc_msg_npc_install_flow(&pfvf->mbox);
	if (!req) {
		err = -ENOMEM;
		goto out;
	}

	req->entry = inb_ctx_info->npc_mcam_entry;
	req->features |= BIT(NPC_IPPROTO_ESP) | BIT(NPC_IPSEC_SPI);
	req->intf = NIX_INTF_RX;
	req->index = pfvf->ipsec.inb_ipsec_rq;
	req->match_id = 0xfeed;
	req->channel = pfvf->hw.rx_chan_base;
	req->op = NIX_RX_ACTIONOP_UCAST_IPSEC;
	req->set_cntr = 1;
	req->packet.spi = inb_ctx_info->spi;
	req->mask.spi = cpu_to_be32(0xffffffff);

	/* Send message to AF */
	err = otx2_sync_mbox_msg(&pfvf->mbox);
out:
	mutex_unlock(&pfvf->mbox.lock);
	return err;
}

static int cn10k_inb_delete_flow(struct otx2_nic *pfvf,
				 struct cn10k_inb_sw_ctx_info *inb_ctx_info)
{
	struct npc_delete_flow_req *req;
	int err = 0;

	mutex_lock(&pfvf->mbox.lock);

	req = otx2_mbox_alloc_msg_npc_delete_flow(&pfvf->mbox);
	if (!req) {
		err = -ENOMEM;
		goto out;
	}

	req->entry = inb_ctx_info->npc_mcam_entry;

	/* Send message to AF */
	err = otx2_sync_mbox_msg(&pfvf->mbox);
out:
	mutex_unlock(&pfvf->mbox.lock);
	return err;
}

static int cn10k_inb_ena_dis_flow(struct otx2_nic *pfvf,
				  struct cn10k_inb_sw_ctx_info *inb_ctx_info,
				  bool disable)
{
	struct npc_mcam_ena_dis_entry_req *req;
	int err = 0;

	mutex_lock(&pfvf->mbox.lock);

	if (disable)
		req = otx2_mbox_alloc_msg_npc_mcam_dis_entry(&pfvf->mbox);
	else
		req = otx2_mbox_alloc_msg_npc_mcam_ena_entry(&pfvf->mbox);
	if (!req) {
		err = -ENOMEM;
		goto out;
	}

	req->entry = inb_ctx_info->npc_mcam_entry;

	err = otx2_sync_mbox_msg(&pfvf->mbox);
out:
	mutex_unlock(&pfvf->mbox.lock);
	return err;
}

void cn10k_ipsec_inb_disable_flows(struct otx2_nic *pfvf)
{
	struct cn10k_inb_sw_ctx_info *inb_ctx_info;

	list_for_each_entry(inb_ctx_info, &pfvf->ipsec.inb_sw_ctx_list, list) {
		if (cn10k_inb_ena_dis_flow(pfvf, inb_ctx_info, true)) {
			netdev_err(pfvf->netdev,
				   "Failed to disable UCAST_IPSEC entry %d\n",
				   inb_ctx_info->npc_mcam_entry);
			continue;
		}
		inb_ctx_info->delete_npc_and_match_entry = false;
	}
}

static int cn10k_inb_install_spi_to_sa_match_entry(struct otx2_nic *pfvf,
						   struct xfrm_state *x,
						   struct cn10k_inb_sw_ctx_info *inb_ctx_info)
{
	struct nix_spi_to_sa_add_req *req;
	struct nix_spi_to_sa_add_rsp *rsp;
	int err;

	mutex_lock(&pfvf->mbox.lock);
	req = otx2_mbox_alloc_msg_nix_spi_to_sa_add(&pfvf->mbox);
	if (!req) {
		mutex_unlock(&pfvf->mbox.lock);
		return -ENOMEM;
	}

	req->sa_index = inb_ctx_info->sa_index;
	req->spi_index = be32_to_cpu(x->id.spi);
	req->match_id = 0xfeed;
	req->valid = 1;

	/* Send message to AF */
	err = otx2_sync_mbox_msg(&pfvf->mbox);

	rsp = (struct nix_spi_to_sa_add_rsp *)otx2_mbox_get_rsp(&pfvf->mbox.mbox, 0, &req->hdr);
	inb_ctx_info->hash_index = rsp->hash_index;
	inb_ctx_info->way = rsp->way;

	mutex_unlock(&pfvf->mbox.lock);
	return err;
}

static int cn10k_inb_delete_spi_to_sa_match_entry(struct otx2_nic *pfvf,
						  struct cn10k_inb_sw_ctx_info *inb_ctx_info)
{
	struct nix_spi_to_sa_delete_req *req;
	int err;

	mutex_lock(&pfvf->mbox.lock);
	req = otx2_mbox_alloc_msg_nix_spi_to_sa_delete(&pfvf->mbox);
	if (!req) {
		mutex_unlock(&pfvf->mbox.lock);
		return -ENOMEM;
	}

	req->hash_index = inb_ctx_info->hash_index;
	req->way = inb_ctx_info->way;

	err = otx2_sync_mbox_msg(&pfvf->mbox);
	mutex_unlock(&pfvf->mbox.lock);
	return err;
}

static int cn10k_inb_nix_inline_lf_cfg(struct otx2_nic *pfvf)
{
	struct nix_inline_ipsec_lf_cfg *req;
	int ret = 0;

	mutex_lock(&pfvf->mbox.lock);
	req = otx2_mbox_alloc_msg_nix_inline_ipsec_lf_cfg(&pfvf->mbox);
	if (!req) {
		ret = -ENOMEM;
		goto error;
	}

	req->sa_base_addr = pfvf->ipsec.inb_sa->iova;
	req->ipsec_cfg0.tag_const = 0;
	req->ipsec_cfg0.tt = 0;
	req->ipsec_cfg0.lenm1_max = 11872; /* (Max packet size - 128 (first skip)) */
	req->ipsec_cfg0.sa_pow2_size = 0xb; /* 2048 */
	req->ipsec_cfg1.sa_idx_max = CN10K_IPSEC_INB_MAX_SA - 1;
	req->ipsec_cfg1.sa_idx_w = 0x7;
	req->enable = 1;

	ret = otx2_sync_mbox_msg(&pfvf->mbox);
error:
	mutex_unlock(&pfvf->mbox.lock);
	return ret;
}

static int cn10k_inb_nix_inline_lf_rq_cfg(struct otx2_nic *pfvf)
{
	struct nix_rq_cpt_field_mask_cfg_req *req;
	int ret = 0, i;

	mutex_lock(&pfvf->mbox.lock);
	req = otx2_mbox_alloc_msg_nix_lf_inline_rq_cfg(&pfvf->mbox);
	if (!req) {
		ret = -ENOMEM;
		goto error;
	}

	for (i = 0; i < RQ_CTX_MASK_MAX; i++)
		req->rq_ctx_word_mask[i] = 0xffffffffffffffff;

	req->rq_set.len_ol3_dis = 1;
	req->rq_set.len_ol4_dis = 1;
	req->rq_set.len_il3_dis = 1;
	req->rq_set.len_il4_dis = 1;
	req->rq_set.csum_ol4_dis = 1;
	req->rq_set.csum_il4_dis = 1;
	req->rq_set.lenerr_dis = 1;
	req->rq_set.port_ol4_dis = 1;
	req->rq_set.port_il4_dis = 1;
	req->rq_set.lpb_drop_ena = 0;
	req->rq_set.spb_drop_ena = 0;
	req->rq_set.xqe_drop_ena = 0;
	req->rq_set.spb_ena = 1;
	req->rq_set.ena = 1;

	req->rq_mask.len_ol3_dis = 0;
	req->rq_mask.len_ol4_dis = 0;
	req->rq_mask.len_il3_dis = 0;
	req->rq_mask.len_il4_dis = 0;
	req->rq_mask.csum_ol4_dis = 0;
	req->rq_mask.csum_il4_dis = 0;
	req->rq_mask.lenerr_dis = 0;
	req->rq_mask.port_ol4_dis = 0;
	req->rq_mask.port_il4_dis = 0;
	req->rq_mask.lpb_drop_ena = 0;
	req->rq_mask.spb_drop_ena = 0;
	req->rq_mask.xqe_drop_ena = 0;
	req->rq_mask.spb_ena = 0;
	req->rq_mask.ena = 0;

	/* Setup SPB fields for second pass */
	req->ipsec_cfg1_inline_replay.rq_mask_enable = 1;

	ret = otx2_sync_mbox_msg(&pfvf->mbox);
error:
	mutex_unlock(&pfvf->mbox.lock);
	return ret;
}

static int cn10k_inb_nix_inline_ipsec_cfg(struct otx2_nic *pfvf)
{
	struct cpt_rx_inline_lf_cfg_msg *req;
	int ret = 0;

	mutex_lock(&pfvf->mbox.lock);
	req = otx2_mbox_alloc_msg_cpt_rx_inline_lf_cfg(&pfvf->mbox);
	if (!req) {
		ret = -ENOMEM;
		goto error;
	}

	req->sso_pf_func = 0;
	req->opcode = CN10K_IPSEC_MAJOR_OP_INB_IPSEC | (1 << 6);
	req->param1 = 7; /* bit 0:ip_csum_dis 1:tcp_csum_dis 2:esp_trailer_dis */
	req->param2 = 0;
	req->bpid = pfvf->ipsec.bpid;
	req->credit = (pfvf->qset.rqe_cnt * 3) / 4;
	req->credit_th = pfvf->qset.rqe_cnt / 10;
	req->ctx_ilen_valid = 1;
	req->ctx_ilen = 5;

	ret = otx2_sync_mbox_msg(&pfvf->mbox);
error:
	mutex_unlock(&pfvf->mbox.lock);
	return ret;
}

static int cn10k_ipsec_ingress_rq_init(struct otx2_nic *pfvf, u16 qidx, u16 lpb_aura)
{
	struct nix_cn10k_aq_enq_req *aq;

	/* Get memory to put this msg */
	aq = otx2_mbox_alloc_msg_nix_cn10k_aq_enq(&pfvf->mbox);
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
	aq->rq.lpb_aura_pass = RQ_PASS_LVL_AURA;
	aq->rq.lpb_aura_drop = RQ_DROP_LVL_AURA;
	aq->rq.ipsech_ena = 1;		/* IPsec HW fast path enable */
	aq->rq.ipsecd_drop_ena = 1;	/* IPsec dynamic drop enable */
	aq->rq.ena_wqwd = 1;		/* Store NIX header in packet buffer */
	aq->rq.first_skip = 16;		/* Store packet after skipping 16x8
					 * bytes to accommodate NIX header.
					 */

	/* Fill AQ info */
	aq->qidx = qidx;
	aq->ctype = NIX_AQ_CTYPE_RQ;
	aq->op = NIX_AQ_INSTOP_INIT;

	return otx2_sync_mbox_msg(&pfvf->mbox);
}

/* Enable backpressure for the specified aura since
 * it cannot be enabled during aura initialization.
 */
static int cn10k_ipsec_enable_aura_backpressure(struct otx2_nic *pfvf,
						int aura_id, int bpid)
{
	struct npa_aq_enq_req *npa_aq;

	npa_aq = otx2_mbox_alloc_msg_npa_aq_enq(&pfvf->mbox);
	if (!npa_aq)
		return -ENOMEM;

	npa_aq->aura.bp_ena = 1;
	npa_aq->aura_mask.bp_ena = 1;
	npa_aq->aura.nix0_bpid = bpid;
	npa_aq->aura_mask.nix0_bpid = GENMASK(8, 0);
	npa_aq->aura.bp = (255 - ((50 * 256) / 100));
	npa_aq->aura_mask.bp = GENMASK(7, 0);

	/* Fill NPA AQ info */
	npa_aq->aura_id = aura_id;
	npa_aq->ctype = NPA_AQ_CTYPE_AURA;
	npa_aq->op = NPA_AQ_INSTOP_WRITE;

	return otx2_sync_mbox_msg(&pfvf->mbox);
}

static int cn10k_ipsec_aura_and_pool_init(struct otx2_nic *pfvf, int pool_id,
					  int bpid)
{
	struct otx2_hw *hw = &pfvf->hw;
	struct otx2_pool *pool = NULL;
	int num_ptrs, stack_pages;
	dma_addr_t bufptr;
	int err, ptr;

	num_ptrs = pfvf->qset.rqe_cnt;
	stack_pages = (num_ptrs + hw->stack_pg_ptrs - 1) / hw->stack_pg_ptrs;
	pool = &pfvf->qset.pool[pool_id];

	/* Initialize aura context */
	err = otx2_aura_init(pfvf, pool_id, pool_id, num_ptrs);
	if (err)
		goto fail;

	/* Initialize pool */
	err = otx2_pool_init(pfvf, pool_id, stack_pages, num_ptrs, pfvf->rbsize,
			     AURA_NIX_RQ);
	if (err)
		goto fail;

	/* Flush accumulated messages */
	err = otx2_sync_mbox_msg(&pfvf->mbox);
	if (err)
		goto fail;

	/* Allocate pointers and free them to aura/pool */
	for (ptr = 0; ptr < num_ptrs; ptr++) {
		err = otx2_alloc_rbuf(pfvf, pool, &bufptr, pool_id, ptr);
		if (err) {
			err = -ENOMEM;
			goto free_auras;
		}
		pfvf->hw_ops->aura_freeptr(pfvf, pool_id, bufptr + OTX2_HEAD_ROOM);
	}

	/* Enable backpressure for the aura */
	err = cn10k_ipsec_enable_aura_backpressure(pfvf, pool_id, bpid);
	if (err)
		goto free_auras;

	return err;

free_auras:
	cn10k_ipsec_free_aura_ptrs(pfvf);
fail:
	otx2_mbox_reset(&pfvf->mbox.mbox, 0);
	return err;
}

static int cn10k_ipsec_setup_nix_rx_hw_resources(struct otx2_nic *pfvf)
{
	int rbsize, err, pool;

	mutex_lock(&pfvf->mbox.lock);

	/* Initialize Pool for first pass */
	err = cn10k_ipsec_aura_and_pool_init(pfvf, pfvf->ipsec.inb_ipsec_pool,
					     pfvf->ipsec.bpid);
	if (err)
		return err;

	/* Initialize first pass RQ and map buffers from pool_id */
	err = cn10k_ipsec_ingress_rq_init(pfvf, pfvf->ipsec.inb_ipsec_rq,
					  pfvf->ipsec.inb_ipsec_pool);
	if (err)
		goto free_auras;

	/* Initialize SPB pool for second pass */
	rbsize = pfvf->rbsize;
	pfvf->rbsize = 512;

	for (pool = pfvf->ipsec.inb_ipsec_spb_pool;
	     pool < pfvf->hw.rx_queues + pfvf->ipsec.inb_ipsec_spb_pool; pool++) {
		err = cn10k_ipsec_aura_and_pool_init(pfvf, pool,
						     pfvf->ipsec.spb_bpid);
		if (err)
			goto free_auras;
	}
	pfvf->rbsize = rbsize;

	mutex_unlock(&pfvf->mbox.lock);
	return 0;

free_auras:
	cn10k_ipsec_free_aura_ptrs(pfvf);
	mutex_unlock(&pfvf->mbox.lock);
	otx2_mbox_reset(&pfvf->mbox.mbox, 0);
	return err;
}

static void cn10k_ipsec_npa_refill_inb_ipsecq(struct work_struct *work)
{
	struct cn10k_ipsec *ipsec = container_of(work, struct cn10k_ipsec,
						 refill_npa_inline_ipsecq);
	struct otx2_nic *pfvf = container_of(ipsec, struct otx2_nic, ipsec);
	struct otx2_pool *pool = NULL;
	int err, pool_id, idx;
	void __iomem *ptr;
	dma_addr_t bufptr;
	u64 val, count;

	val = otx2_read64(pfvf, NPA_LF_QINTX_INT(0));
	if (!(val & 1))
		return;

	ptr = otx2_get_regaddr(pfvf, NPA_LF_AURA_OP_INT);
	val = otx2_atomic64_add(((u64)pfvf->ipsec.inb_ipsec_pool << 44), ptr);

	/* Refill buffers only on a threshold interrupt */
	if (!(val & NPA_LF_AURA_OP_INT_THRESH_INT))
		return;

	local_bh_disable();

	/* Allocate and refill to the IPsec pool */
	pool_id = pfvf->ipsec.inb_ipsec_pool;
	pool = &pfvf->qset.pool[pool_id];

	/* Calculate the number of spb buffers received */
	for (count = 0, idx = 0; idx < pfvf->hw.rx_queues; idx++)
		count += atomic_xchg(&pfvf->ipsec.inb_spb_count[idx], 0);

	for (idx = 0; idx < count; idx++) {
		err = otx2_alloc_rbuf(pfvf, pool, &bufptr, pool_id, idx);
		if (err) {
			netdev_err(pfvf->netdev,
				   "Insufficient memory for IPsec pool buffers\n");
			break;
		}
		pfvf->hw_ops->aura_freeptr(pfvf, pool_id, bufptr + OTX2_HEAD_ROOM);
	}

	/* Clear/ACK Interrupt */
	val = FIELD_PREP(NPA_LF_AURA_OP_INT_AURA, pfvf->ipsec.inb_ipsec_pool);
	val |= NPA_LF_AURA_OP_INT_THRESH_INT;
	otx2_write64(pfvf, NPA_LF_AURA_OP_INT, val);

	local_bh_enable();
}

static irqreturn_t cn10k_ipsec_npa_inb_ipsecq_intr_handler(int irq, void *data)
{
	struct otx2_nic *pf = data;

	schedule_work(&pf->ipsec.refill_npa_inline_ipsecq);

	return IRQ_HANDLED;
}

static int cn10k_inb_cpt_init(struct net_device *netdev)
{
	struct otx2_nic *pfvf = netdev_priv(netdev);
	struct cn10k_inb_sw_ctx_info *inb_ctx_info;
	int ret = 0, spb_cnt;
	char *irq_name;
	int  vec;
	u64 val;

	/* Set sa_tbl_entry_sz to 2048 since we are programming NIX RX
	 * to calculate SA index as SPI * 2048. The first 1024 bytes
	 * are used for SA context and  the next half for bookkeeping data.
	 */
	pfvf->ipsec.sa_tbl_entry_sz = 2048;
	ret = qmem_alloc(pfvf->dev, &pfvf->ipsec.inb_sa, CN10K_IPSEC_INB_MAX_SA,
			 pfvf->ipsec.sa_tbl_entry_sz);
	if (ret)
		return ret;

	memset(pfvf->ipsec.inb_sa->base, 0,
	       pfvf->ipsec.sa_tbl_entry_sz * CN10K_IPSEC_INB_MAX_SA);

	/* Allocate SPB buffer count array to track the number of inbound SPB
	 * buffers received per RX queue. This count would later be used to
	 * refill the first pass IPsec pool.
	 */
	pfvf->ipsec.inb_spb_count = devm_kmalloc_array(pfvf->dev,
						       pfvf->hw.rx_queues,
						       sizeof(atomic_t),
						       GFP_KERNEL);
	if (!pfvf->ipsec.inb_spb_count) {
		netdev_err(netdev, "Failed to allocate inbound SPB buffer count array\n");
		return -ENOMEM;
	}

	for (spb_cnt = 0; spb_cnt < pfvf->hw.rx_queues; spb_cnt++)
		atomic_set(&pfvf->ipsec.inb_spb_count[spb_cnt], 0);

	/* Setup NIX RX HW resources for inline inbound IPSec */
	ret = cn10k_ipsec_setup_nix_rx_hw_resources(pfvf);
	if (ret) {
		netdev_err(netdev, "Failed to setup NIX HW resources for IPsec\n");
		return ret;
	}

	/* Work entry for refilling the NPA queue for ingress inline IPSec */
	INIT_WORK(&pfvf->ipsec.refill_npa_inline_ipsecq,
		  cn10k_ipsec_npa_refill_inb_ipsecq);

	/* Register NPA interrupt */
	vec = pfvf->hw.npa_msixoff;
	irq_name = &pfvf->hw.irq_name[vec * NAME_SIZE];
	snprintf(irq_name, NAME_SIZE, "%s-npa-qint", pfvf->netdev->name);

	ret = request_irq(pci_irq_vector(pfvf->pdev, vec),
			  cn10k_ipsec_npa_inb_ipsecq_intr_handler, 0,
			  irq_name, pfvf);
	if (ret) {
		dev_err(pfvf->dev,
			"RVUPF%d: IRQ registration failed for NPA QINT\n",
			rvu_get_pf(pfvf->pdev, pfvf->pcifunc));
		return ret;
	}

	/* Set threshold values */
	val = FIELD_PREP(NPA_LF_AURA_OP_THRESH_AURA, pfvf->ipsec.inb_ipsec_pool);
	val |= NPA_LF_AURA_OP_THRESH_THRESH_UP;
	val |= FIELD_PREP(NPA_LF_AURA_OP_THRESH_THRESHOLD,
			  ((pfvf->qset.rqe_cnt / 2) - 1));
	otx2_write64(pfvf, NPA_LF_AURA_OP_THRESH, val);

	/* Enable NPA threshold interrupt */
	val = FIELD_PREP(NPA_LF_AURA_OP_INT_AURA, pfvf->ipsec.inb_ipsec_pool);
	val |= NPA_LF_AURA_OP_INT_SETOP;
	val |= NPA_LF_AURA_OP_INT_THRESH_ENA;
	otx2_write64(pfvf, NPA_LF_AURA_OP_INT, val);

	/* Enable interrupt */
	otx2_write64(pfvf, NPA_LF_QINTX_ENA_W1S(0), BIT_ULL(0));

	/* Enable inbound inline IPSec in NIX LF */
	ret = cn10k_inb_nix_inline_lf_cfg(pfvf);
	if (ret) {
		netdev_err(netdev, "Error configuring NIX for Inline IPSec\n");
		goto out;
	}

	/* IPsec specific RQ settings in NIX LF */
	ret = cn10k_inb_nix_inline_lf_rq_cfg(pfvf);
	if (ret) {
		netdev_err(netdev, "Error configuring NIX for Inline IPSec\n");
		goto out;
	}

	/* One-time configuration to enable CPT LF for inline inbound IPSec */
	ret = cn10k_inb_nix_inline_ipsec_cfg(pfvf);
	if (ret && ret != -EEXIST)
		netdev_err(netdev, "CPT LF configuration error\n");
	else
		ret = 0;

	/* If the driver has any offloaded inbound SA context(s), re-install the
	 * associated SPI-to-SA match and NPC rules. This is generally executed
	 * when the RQs are changed at runtime.
	 */
	list_for_each_entry(inb_ctx_info, &pfvf->ipsec.inb_sw_ctx_list, list) {
		cn10k_inb_ena_dis_flow(pfvf, inb_ctx_info, false);
		cn10k_inb_install_flow(pfvf, inb_ctx_info);
		cn10k_inb_install_spi_to_sa_match_entry(pfvf,
							inb_ctx_info->x_state,
							inb_ctx_info);
	}

out:
	return ret;
}

static int cn10k_outb_cpt_clean(struct otx2_nic *pf)
{
	int ret;

	/* Disable CPTLF Instruction Queue (IQ) */
	cn10k_outb_cptlf_iq_disable(pf);

	/* Set IQ base address and size to 0 */
	otx2_write64(pf, CN10K_CPT_LF_Q_BASE, 0);
	otx2_write64(pf, CN10K_CPT_LF_Q_SIZE, 0);

	/* Free CPTLF IQ */
	cn10k_outb_cptlf_iq_free(pf);

	/* Free and detach CPT LF */
	cn10k_outb_cptlf_free(pf);
	ret = cn10k_outb_cptlf_detach(pf);
	if (ret)
		netdev_err(pf->netdev, "Failed to detach CPT LF\n");

	return ret;
}

static u32 cn10k_inb_alloc_sa(struct otx2_nic *pf, struct xfrm_state *x)
{
	u32 sa_index = 0;

	sa_index = find_first_zero_bit(pf->ipsec.inb_sa_table, CN10K_IPSEC_INB_MAX_SA);
	if (sa_index >= CN10K_IPSEC_INB_MAX_SA)
		return sa_index;

	set_bit(sa_index, pf->ipsec.inb_sa_table);

	return sa_index;
}

static void cn10k_cpt_inst_flush(struct otx2_nic *pf, struct cpt_inst_s *inst,
				 u64 size)
{
	struct otx2_lmt_info *lmt_info;
	u64 val = 0, tar_addr = 0;

	lmt_info = per_cpu_ptr(pf->hw.lmt_info, smp_processor_id());
	/* FIXME: val[0:10] LMT_ID.
	 * [12:15] no of LMTST - 1 in the burst.
	 * [19:63] data size of each LMTST in the burst except first.
	 */
	val = (lmt_info->lmt_id & 0x7FF);
	/* Target address for LMTST flush tells HW how many 128bit
	 * words are present.
	 * tar_addr[6:4] size of first LMTST - 1 in units of 128b.
	 */
	tar_addr |= pf->ipsec.io_addr | (((size / 16) - 1) & 0x7) << 4;
	dma_wmb();
	memcpy((u64 *)lmt_info->lmt_addr, inst, size);
	cn10k_lmt_flush(val, tar_addr);
}

static int cn10k_wait_for_cpt_respose(struct otx2_nic *pf,
				      struct cpt_res_s *res)
{
	unsigned long timeout = jiffies + msecs_to_jiffies(100);
	u64 *completion_ptr = (u64 *)res;

	do {
		if (time_after(jiffies, timeout)) {
			netdev_err(pf->netdev, "CPT response timeout\n");
			return -EBUSY;
		}
	} while ((READ_ONCE(*completion_ptr) & CN10K_CPT_COMP_E_MASK) ==
		 CN10K_CPT_COMP_E_NOTDONE);

	if (!(res->compcode == CN10K_CPT_COMP_E_GOOD ||
	      res->compcode == CN10K_CPT_COMP_E_WARN) || res->uc_compcode) {
		netdev_err(pf->netdev, "compcode=%x doneint=%x\n",
			   res->compcode, res->doneint);
		netdev_err(pf->netdev, "uc_compcode=%x uc_info=%llx esn=%llx\n",
			   res->uc_compcode, (u64)res->uc_info, res->esn);
	}
	return 0;
}

static int cn10k_outb_write_sa(struct otx2_nic *pf, struct qmem *sa_info)
{
	dma_addr_t res_iova, dptr_iova, sa_iova;
	struct cn10k_tx_sa_s *sa_dptr;
	struct cpt_inst_s inst = {};
	struct cpt_res_s *res;
	u32 sa_size, off;
	u64 *sptr, *dptr;
	u64 reg_val;
	int ret;

	sa_iova = sa_info->iova;
	if (!sa_iova)
		return -EINVAL;

	res = dma_alloc_coherent(pf->dev, sizeof(struct cpt_res_s),
				 &res_iova, GFP_ATOMIC);
	if (!res)
		return -ENOMEM;

	sa_size = sizeof(struct cn10k_tx_sa_s);
	sa_dptr = dma_alloc_coherent(pf->dev, sa_size, &dptr_iova, GFP_ATOMIC);
	if (!sa_dptr) {
		dma_free_coherent(pf->dev, sizeof(struct cpt_res_s), res,
				  res_iova);
		return -ENOMEM;
	}

	sptr = (__force u64 *)sa_info->base;
	dptr =  (__force u64 *)sa_dptr;
	for (off = 0; off < (sa_size / 8); off++)
		*(dptr + off) = (__force u64)cpu_to_be64(*(sptr + off));

	res->compcode = CN10K_CPT_COMP_E_NOTDONE;
	inst.res_addr = res_iova;
	inst.dptr = (u64)dptr_iova;
	inst.param2 = sa_size >> 3;
	inst.dlen = sa_size;
	inst.opcode_major = CN10K_IPSEC_MAJOR_OP_WRITE_SA;
	inst.opcode_minor = CN10K_IPSEC_MINOR_OP_WRITE_SA;
	inst.cptr = sa_iova;
	inst.ctx_val = 1;
	inst.egrp = CN10K_DEF_CPT_IPSEC_EGRP;

	/* Check if CPT-LF available */
	if (!cn10k_cpt_device_set_inuse(pf)) {
		ret = -ENODEV;
		goto free_mem;
	}

	cn10k_cpt_inst_flush(pf, &inst, sizeof(struct cpt_inst_s));
	dma_wmb();
	ret = cn10k_wait_for_cpt_respose(pf, res);
	if (ret)
		goto set_available;

	/* Trigger CTX flush to write dirty data back to DRAM */
	reg_val = FIELD_PREP(CPT_LF_CTX_FLUSH_CPTR, sa_iova >> 7);
	otx2_write64(pf, CN10K_CPT_LF_CTX_FLUSH, reg_val);

set_available:
	cn10k_cpt_device_set_available(pf);
free_mem:
	dma_free_coherent(pf->dev, sa_size, sa_dptr, dptr_iova);
	dma_free_coherent(pf->dev, sizeof(struct cpt_res_s), res, res_iova);
	return ret;
}

static int cn10k_inb_write_sa(struct otx2_nic *pf,
			      struct xfrm_state *x,
			      struct cn10k_inb_sw_ctx_info *inb_ctx_info)
{
	dma_addr_t res_iova, dptr_iova, sa_iova;
	struct cn10k_rx_sa_s *sa_dptr, *sa_cptr;
	struct cpt_inst_s inst;
	struct cpt_res_s *res;
	u32 sa_size, off;
	u64 reg_val;
	int ret;

	res = dma_alloc_coherent(pf->dev, sizeof(struct cpt_res_s),
				 &res_iova, GFP_ATOMIC);
	if (!res)
		return -ENOMEM;

	sa_cptr = inb_ctx_info->sa_entry;
	sa_iova = inb_ctx_info->sa_iova;
	sa_size = sizeof(struct cn10k_rx_sa_s);

	sa_dptr = dma_alloc_coherent(pf->dev, sa_size, &dptr_iova, GFP_ATOMIC);
	if (!sa_dptr) {
		dma_free_coherent(pf->dev, sizeof(struct cpt_res_s), res,
				  res_iova);
		return -ENOMEM;
	}

	for (off = 0; off < (sa_size / 8); off++)
		*((u64 *)sa_dptr + off) = (__force u64)cpu_to_be64(*((u64 *)sa_cptr + off));

	memset(&inst, 0, sizeof(struct cpt_inst_s));

	res->compcode = 0;
	inst.res_addr = res_iova;
	inst.dptr = (u64)dptr_iova;
	inst.param2 = sa_size >> 3;
	inst.dlen = sa_size;
	inst.opcode_major = CN10K_IPSEC_MAJOR_OP_WRITE_SA;
	inst.opcode_minor = CN10K_IPSEC_MINOR_OP_WRITE_SA;
	inst.cptr = sa_iova;
	inst.ctx_val = 1;
	inst.egrp = CN10K_DEF_CPT_IPSEC_EGRP;

	/* Reuse Outbound CPT LF to install Ingress SAs as well because
	 * the driver does not own the ingress CPT LF.
	 */
	pf->ipsec.io_addr = (__force u64)otx2_get_regaddr(pf, CN10K_CPT_LF_NQX(0));
	cn10k_cpt_inst_flush(pf, &inst, sizeof(struct cpt_inst_s));
	dma_wmb();

	ret = cn10k_wait_for_cpt_respose(pf, res);
	if (ret)
		goto out;

	/* Trigger CTX flush to write dirty data back to DRAM */
	reg_val = FIELD_PREP(GENMASK_ULL(45, 0), sa_iova >> 7);
	otx2_write64(pf, CN10K_CPT_LF_CTX_FLUSH, reg_val);

out:
	dma_free_coherent(pf->dev, sa_size, sa_dptr, dptr_iova);
	dma_free_coherent(pf->dev, sizeof(struct cpt_res_s), res, res_iova);
	return ret;
}

static void cn10k_xfrm_inb_prepare_sa(struct otx2_nic *pf, struct xfrm_state *x,
				      struct cn10k_inb_sw_ctx_info *inb_ctx_info)
{
	struct cn10k_rx_sa_s *sa_entry = inb_ctx_info->sa_entry;
	int key_len = (x->aead->alg_key_len + 7) / 8;
	u32 sa_size = sizeof(struct cn10k_rx_sa_s);
	u8 *key = x->aead->alg_key;
	u64 *tmp_key;
	u32 *tmp_salt;
	int idx;

	memset(sa_entry, 0, sizeof(struct cn10k_rx_sa_s));

	/* Disable ESN for now */
	sa_entry->esn_en = 0;

	/* HW context offset is word-31 */
	sa_entry->hw_ctx_off = 31;
	sa_entry->pkind = NPC_RX_CPT_HDR_PKIND;
	sa_entry->eth_ovrwr = 1;
	sa_entry->pkt_output = 1;
	sa_entry->pkt_format = 1;
	sa_entry->orig_pkt_free = 0;
	/* context push size is up to word 31 */
	sa_entry->ctx_push_size = 31 + 1;
	/* context size, 128 Byte aligned up */
	sa_entry->ctx_size = (sa_size / OTX2_ALIGN)  & 0xF;

	sa_entry->cookie = inb_ctx_info->sa_index;

	/* 1 word (??) prepanded to context header size */
	sa_entry->ctx_hdr_size = 1;
	/* Mark SA entry valid */
	sa_entry->aop_valid = 1;

	sa_entry->sa_dir = 0;			/* Inbound */
	sa_entry->ipsec_protocol = 1;		/* ESP */
	/* Default to Transport Mode */
	if (x->props.mode == XFRM_MODE_TUNNEL)
		sa_entry->ipsec_mode = 1;	/* Tunnel Mode */

	sa_entry->et_ovrwr_ddr_en = 1;
	sa_entry->enc_type = 5;			/* AES-GCM only */
	sa_entry->aes_key_len = 1;		/* AES key length 128 */
	sa_entry->l2_l3_hdr_on_error = 1;
	sa_entry->spi = (__force u32)be32_to_cpu(x->id.spi);

	/* Last 4 bytes are salt */
	key_len -= 4;
	memcpy(sa_entry->cipher_key, key, key_len);
	tmp_key = (u64 *)sa_entry->cipher_key;

	for (idx = 0; idx < key_len / 8; idx++)
		tmp_key[idx] = (__force u64)cpu_to_be64(tmp_key[idx]);

	memcpy(&sa_entry->iv_gcm_salt, key + key_len, 4);
	tmp_salt = (u32 *)&sa_entry->iv_gcm_salt;
	*tmp_salt = (__force u32)cpu_to_be32(*tmp_salt);

	/* Write SA context data to memory before enabling */
	wmb();

	/* Enable SA */
	sa_entry->sa_valid = 1;
}

static int cn10k_ipsec_get_hw_ctx_offset(void)
{
	/* Offset on Hardware-context offset in word */
	return (offsetof(struct cn10k_tx_sa_s, hw_ctx) / sizeof(u64)) & 0x7F;
}

static int cn10k_ipsec_get_ctx_push_size(void)
{
	/* Context push size is round up and in multiple of 8 Byte */
	return (roundup(offsetof(struct cn10k_tx_sa_s, hw_ctx), 8) / 8) & 0x7F;
}

static int cn10k_ipsec_get_aes_key_len(int key_len)
{
	/* key_len is aes key length in bytes */
	switch (key_len) {
	case 16:
		return CN10K_IPSEC_SA_AES_KEY_LEN_128;
	case 24:
		return CN10K_IPSEC_SA_AES_KEY_LEN_192;
	default:
		return CN10K_IPSEC_SA_AES_KEY_LEN_256;
	}
}

static void cn10k_outb_prepare_sa(struct xfrm_state *x,
				  struct cn10k_tx_sa_s *sa_entry)
{
	int key_len = (x->aead->alg_key_len + 7) / 8;
	struct net_device *netdev = x->xso.dev;
	u8 *key = x->aead->alg_key;
	struct otx2_nic *pf;
	u32 *tmp_salt;
	u64 *tmp_key;
	int idx;

	memset(sa_entry, 0, sizeof(struct cn10k_tx_sa_s));

	/* context size, 128 Byte aligned up */
	pf = netdev_priv(netdev);
	sa_entry->ctx_size = (pf->ipsec.sa_size / OTX2_ALIGN)  & 0xF;
	sa_entry->hw_ctx_off = cn10k_ipsec_get_hw_ctx_offset();
	sa_entry->ctx_push_size = cn10k_ipsec_get_ctx_push_size();

	/* Ucode to skip two words of CPT_CTX_HW_S */
	sa_entry->ctx_hdr_size = 1;

	/* Allow Atomic operation (AOP) */
	sa_entry->aop_valid = 1;

	/* Outbound, ESP TRANSPORT/TUNNEL Mode, AES-GCM with */
	sa_entry->sa_dir = CN10K_IPSEC_SA_DIR_OUTB;
	sa_entry->ipsec_protocol = CN10K_IPSEC_SA_IPSEC_PROTO_ESP;
	sa_entry->enc_type = CN10K_IPSEC_SA_ENCAP_TYPE_AES_GCM;
	sa_entry->iv_src = CN10K_IPSEC_SA_IV_SRC_PACKET;
	if (x->props.mode == XFRM_MODE_TUNNEL)
		sa_entry->ipsec_mode = CN10K_IPSEC_SA_IPSEC_MODE_TUNNEL;
	else
		sa_entry->ipsec_mode = CN10K_IPSEC_SA_IPSEC_MODE_TRANSPORT;

	/* Last 4 bytes are salt */
	key_len -= 4;
	sa_entry->aes_key_len = cn10k_ipsec_get_aes_key_len(key_len);
	memcpy(sa_entry->cipher_key, key, key_len);
	tmp_key = (u64 *)sa_entry->cipher_key;

	for (idx = 0; idx < key_len / 8; idx++)
		tmp_key[idx] = (__force u64)cpu_to_be64(tmp_key[idx]);

	memcpy(&sa_entry->iv_gcm_salt, key + key_len, 4);
	tmp_salt = (u32 *)&sa_entry->iv_gcm_salt;
	*tmp_salt = (__force u32)cpu_to_be32(*tmp_salt);

	/* Write SA context data to memory before enabling */
	wmb();

	/* Enable SA */
	sa_entry->sa_valid = 1;
}

static int cn10k_ipsec_validate_state(struct xfrm_state *x,
				      struct netlink_ext_ack *extack)
{
	if (x->props.aalgo != SADB_AALG_NONE) {
		NL_SET_ERR_MSG_MOD(extack,
				   "Cannot offload authenticated xfrm states");
		return -EINVAL;
	}
	if (x->props.ealgo != SADB_X_EALG_AES_GCM_ICV16) {
		NL_SET_ERR_MSG_MOD(extack,
				   "Only AES-GCM-ICV16 xfrm state may be offloaded");
		return -EINVAL;
	}
	if (x->props.calgo != SADB_X_CALG_NONE) {
		NL_SET_ERR_MSG_MOD(extack,
				   "Cannot offload compressed xfrm states");
		return -EINVAL;
	}
	if (x->props.flags & XFRM_STATE_ESN) {
		NL_SET_ERR_MSG_MOD(extack, "Cannot offload ESN xfrm states");
		return -EINVAL;
	}
	if (x->props.family != AF_INET && x->props.family != AF_INET6) {
		NL_SET_ERR_MSG_MOD(extack,
				   "Only IPv4/v6 xfrm states may be offloaded");
		return -EINVAL;
	}
	if (x->props.mode != XFRM_MODE_TRANSPORT &&
	    x->props.mode != XFRM_MODE_TUNNEL) {
		NL_SET_ERR_MSG_MOD(extack,
				   "Only tunnel/transport xfrm states may be offloaded");
		return -EINVAL;
	}
	if (x->id.proto != IPPROTO_ESP) {
		NL_SET_ERR_MSG_MOD(extack,
				   "Only ESP xfrm state may be offloaded");
		return -EINVAL;
	}
	if (!x->aead) {
		NL_SET_ERR_MSG_MOD(extack,
				   "Cannot offload xfrm states without aead");
		return -EINVAL;
	}

	if (x->aead->alg_icv_len != 128) {
		NL_SET_ERR_MSG_MOD(extack,
				   "Cannot offload xfrm states with AEAD ICV length other than 128bit");
		return -EINVAL;
	}
	if (x->aead->alg_key_len != 128 + 32 &&
	    x->aead->alg_key_len != 192 + 32 &&
	    x->aead->alg_key_len != 256 + 32) {
		NL_SET_ERR_MSG_MOD(extack,
				   "Cannot offload xfrm states with AEAD key length other than 128/192/256bit");
		return -EINVAL;
	}
	if (x->tfcpad) {
		NL_SET_ERR_MSG_MOD(extack,
				   "Cannot offload xfrm states with tfc padding");
		return -EINVAL;
	}
	if (!x->geniv) {
		NL_SET_ERR_MSG_MOD(extack,
				   "Cannot offload xfrm states without geniv");
		return -EINVAL;
	}
	if (strcmp(x->geniv, "seqiv")) {
		NL_SET_ERR_MSG_MOD(extack,
				   "Cannot offload xfrm states with geniv other than seqiv");
		return -EINVAL;
	}
	return 0;
}

static int cn10k_ipsec_inb_add_state(struct xfrm_state *x,
				     struct netlink_ext_ack *extack)
{
	struct cn10k_inb_sw_ctx_info *inb_ctx_info = NULL, *inb_ctx;
	struct net_device *dev = x->xso.dev;
	bool enable_rule = false;
	struct otx2_nic *pf;
	u64 *sa_offset_ptr;
	u32 sa_index = 0;
	int err = 0;

	pf = netdev_priv(dev);

	/* If XFRM policy was added before state, then the inb_ctx_info instance
	 * would be allocated there.
	 */
	list_for_each_entry(inb_ctx, &pf->ipsec.inb_sw_ctx_list, list) {
		if (inb_ctx->reqid == x->props.reqid) {
			inb_ctx_info = inb_ctx;
			enable_rule = true;
			break;
		}
	}

	if (!inb_ctx_info) {
		/* Allocate a structure to track SA related info in driver */
		inb_ctx_info = devm_kzalloc(pf->dev, sizeof(*inb_ctx_info), GFP_KERNEL);
		if (!inb_ctx_info)
			return -ENOMEM;

		/* Stash pointer in the xfrm offload handle */
		x->xso.offload_handle = (unsigned long)inb_ctx_info;
	}

	sa_index = cn10k_inb_alloc_sa(pf, x);
	if (sa_index >= CN10K_IPSEC_INB_MAX_SA) {
		netdev_err(dev, "Failed to find free entry in SA Table\n");
		err = -ENOMEM;
		goto err_out;
	}

	/* Fill in information for bookkeeping */
	inb_ctx_info->sa_index = sa_index;
	inb_ctx_info->spi = x->id.spi;
	inb_ctx_info->reqid = x->props.reqid;
	inb_ctx_info->sa_entry = pf->ipsec.inb_sa->base +
				 (sa_index * pf->ipsec.sa_tbl_entry_sz);
	inb_ctx_info->sa_iova = pf->ipsec.inb_sa->iova +
				(sa_index * pf->ipsec.sa_tbl_entry_sz);
	inb_ctx_info->x_state = x;

	/* Store XFRM state pointer in SA context at an offset of 1KB.
	 * It will be later used in the rcv_pkt_handler to associate
	 * an skb with XFRM state.
	 */
	sa_offset_ptr = pf->ipsec.inb_sa->base +
		 (sa_index * pf->ipsec.sa_tbl_entry_sz) + 1024;
	*sa_offset_ptr = (u64)x;

	err = cn10k_inb_install_spi_to_sa_match_entry(pf, x, inb_ctx_info);
	if (err) {
		netdev_err(dev, "Failed to install Inbound IPSec exact match entry\n");
		goto err_out;
	}

	/* Fill the Inbound SA context structure */
	cn10k_xfrm_inb_prepare_sa(pf, x, inb_ctx_info);

	err = cn10k_inb_write_sa(pf, x, inb_ctx_info);
	if (err)
		netdev_err(dev, "Error writing inbound SA\n");

	/* Enable NPC rule if policy was already installed */
	if (enable_rule) {
		err = cn10k_inb_ena_dis_flow(pf, inb_ctx_info, false);
		if (err)
			netdev_err(dev, "Failed to enable rule\n");
	} else {
		/* All set, add ctx_info to the list */
		list_add_tail(&inb_ctx_info->list, &pf->ipsec.inb_sw_ctx_list);
	}

	cn10k_cpt_device_set_available(pf);
	return err;

err_out:
	x->xso.offload_handle = 0;
	devm_kfree(pf->dev, inb_ctx_info);
	return err;
}

static int cn10k_ipsec_outb_add_state(struct xfrm_state *x,
				      struct netlink_ext_ack *extack)
{
	struct net_device *dev = x->xso.dev;
	struct cn10k_tx_sa_s *sa_entry;
	struct qmem *sa_info;
	struct otx2_nic *pf;
	int err;

	pf = netdev_priv(dev);

	err = qmem_alloc(pf->dev, &sa_info, pf->ipsec.sa_size, OTX2_ALIGN);
	if (err)
		return err;

	sa_entry = (struct cn10k_tx_sa_s *)sa_info->base;
	cn10k_outb_prepare_sa(x, sa_entry);

	err = cn10k_outb_write_sa(pf, sa_info);
	if (err) {
		NL_SET_ERR_MSG_MOD(extack, "Error writing outbound SA");
		qmem_free(pf->dev, sa_info);
		return err;
	}

	x->xso.offload_handle = (unsigned long)sa_info;
	/* Enable static branch when first SA setup */
	if (!pf->ipsec.outb_sa_count)
		static_branch_enable(&cn10k_ipsec_sa_enabled);
	pf->ipsec.outb_sa_count++;
	return 0;
}

static int cn10k_ipsec_add_state(struct xfrm_state *x,
				 struct netlink_ext_ack *extack)
{
	int err;

	err = cn10k_ipsec_validate_state(x, extack);
	if (err)
		return err;

	if (x->xso.dir == XFRM_DEV_OFFLOAD_IN)
		return cn10k_ipsec_inb_add_state(x, extack);
	else
		return cn10k_ipsec_outb_add_state(x, extack);

	return err;
}

static void cn10k_ipsec_inb_del_state(struct net_device *dev,
				      struct otx2_nic *pf, struct xfrm_state *x)
{
	struct cn10k_inb_sw_ctx_info *inb_ctx_info;
	struct cn10k_rx_sa_s *sa_entry;
	int err = 0;

	/* 1. Find SPI to SA entry */
	inb_ctx_info = (struct cn10k_inb_sw_ctx_info *)x->xso.offload_handle;

	if (inb_ctx_info->spi != x->id.spi) {
		netdev_err(dev, "SPI Mismatch (ctx) 0x%x != 0x%x (xfrm)\n",
			   inb_ctx_info->spi, be32_to_cpu(x->id.spi));
		return;
	}

	/* 2. Delete SA in CPT HW */
	sa_entry = inb_ctx_info->sa_entry;
	memset(sa_entry, 0, sizeof(struct cn10k_rx_sa_s));

	sa_entry->ctx_push_size = 31 + 1;
	sa_entry->ctx_size = (sizeof(struct cn10k_rx_sa_s) / OTX2_ALIGN) & 0xF;
	sa_entry->aop_valid = 1;

	if (cn10k_cpt_device_set_inuse(pf)) {
		err = cn10k_inb_write_sa(pf, x, inb_ctx_info);
		if (err)
			netdev_err(dev, "Error (%d) deleting INB SA\n", err);
		cn10k_cpt_device_set_available(pf);
	}

	x->xso.offload_handle = 0;
}

static void cn10k_ipsec_del_state(struct xfrm_state *x)
{
	struct net_device *dev = x->xso.dev;
	struct cn10k_tx_sa_s *sa_entry;
	struct qmem *sa_info;
	struct otx2_nic *pf;
	int err;

	pf = netdev_priv(dev);

	if (x->xso.dir == XFRM_DEV_OFFLOAD_IN)
		return cn10k_ipsec_inb_del_state(dev, pf, x);

	sa_info = (struct qmem *)x->xso.offload_handle;
	sa_entry = (struct cn10k_tx_sa_s *)sa_info->base;
	memset(sa_entry, 0, sizeof(struct cn10k_tx_sa_s));
	/* Disable SA in CPT h/w */
	sa_entry->ctx_push_size = cn10k_ipsec_get_ctx_push_size();
	sa_entry->ctx_size = (pf->ipsec.sa_size / OTX2_ALIGN)  & 0xF;
	sa_entry->aop_valid = 1;

	err = cn10k_outb_write_sa(pf, sa_info);
	if (err)
		netdev_err(dev, "Error (%d) deleting SA\n", err);

	x->xso.offload_handle = 0;
	qmem_free(pf->dev, sa_info);

	/* If no more SA's then update netdev feature for potential change
	 * in NETIF_F_HW_ESP.
	 */
	pf->ipsec.outb_sa_count--;
	queue_work(pf->ipsec.sa_workq, &pf->ipsec.sa_work);
}

static int cn10k_ipsec_policy_add(struct xfrm_policy *x,
				  struct netlink_ext_ack *extack)
{
	struct cn10k_inb_sw_ctx_info *inb_ctx_info = NULL, *inb_ctx;
	struct net_device *netdev = x->xdo.dev;
	bool disable_rule = true;
	struct otx2_nic *pf;
	int ret = 0;

	if (x->xdo.dir != XFRM_DEV_OFFLOAD_IN) {
		netdev_err(netdev, "ERR: Can only offload Inbound policies\n");
		ret = -EINVAL;
	}

	if (x->xdo.type != XFRM_DEV_OFFLOAD_PACKET) {
		netdev_err(netdev, "ERR: Only Packet mode supported\n");
		ret = -EINVAL;
	}

	pf = netdev_priv(netdev);

	/* If XFRM state was added before policy, then the inb_ctx_info instance
	 * would be allocated there.
	 */
	list_for_each_entry(inb_ctx, &pf->ipsec.inb_sw_ctx_list, list) {
		if (inb_ctx->reqid == x->xfrm_vec[0].reqid) {
			inb_ctx_info = inb_ctx;
			disable_rule = false;
			break;
		}
	}

	if (!inb_ctx_info) {
		/* Allocate a structure to track SA related info in driver */
		inb_ctx_info = devm_kzalloc(pf->dev, sizeof(*inb_ctx_info), GFP_KERNEL);
		if (!inb_ctx_info)
			return -ENOMEM;

		inb_ctx_info->reqid = x->xfrm_vec[0].reqid;
	}

	ret = cn10k_inb_alloc_mcam_entry(pf, inb_ctx_info);
	if (ret) {
		netdev_err(netdev, "Failed to allocate MCAM entry for Inbound IPSec flow\n");
		goto err_out;
	}

	ret = cn10k_inb_install_flow(pf, inb_ctx_info);
	if (ret) {
		netdev_err(netdev, "Failed to install Inbound IPSec flow\n");
		goto err_out;
	}

	/* Leave rule in a disabled state until xfrm_state add is completed */
	if (disable_rule) {
		ret = cn10k_inb_ena_dis_flow(pf, inb_ctx_info, true);
		if (ret)
			netdev_err(netdev, "Failed to disable rule\n");

		/* All set, add ctx_info to the list */
		list_add_tail(&inb_ctx_info->list, &pf->ipsec.inb_sw_ctx_list);
	}

	/* Stash pointer in the xfrm offload handle */
	x->xdo.offload_handle = (unsigned long)inb_ctx_info;

err_out:
	return ret;
}

static void cn10k_ipsec_policy_delete(struct xfrm_policy *x)
{
	struct cn10k_inb_sw_ctx_info *inb_ctx_info;
	struct net_device *netdev = x->xdo.dev;
	struct otx2_nic *pf;

	if (!x->xdo.offload_handle)
		return;

	pf = netdev_priv(netdev);
	inb_ctx_info = (struct cn10k_inb_sw_ctx_info *)x->xdo.offload_handle;

	/* Schedule a workqueue to free NPC rule and SPI-to-SA match table
	 * entry because they are freed via a mailbox call which can sleep
	 * and the delete policy routine from XFRM stack is called in an
	 * atomic context.
	 */
	inb_ctx_info->delete_npc_and_match_entry = true;
	queue_work(pf->ipsec.sa_workq, &pf->ipsec.sa_work);
}

static void cn10k_ipsec_policy_free(struct xfrm_policy *x)
{
	return;
}

static const struct xfrmdev_ops cn10k_ipsec_xfrmdev_ops = {
	.xdo_dev_state_add	= cn10k_ipsec_add_state,
	.xdo_dev_state_delete	= cn10k_ipsec_del_state,
	.xdo_dev_policy_add	= cn10k_ipsec_policy_add,
	.xdo_dev_policy_delete	= cn10k_ipsec_policy_delete,
	.xdo_dev_policy_free	= cn10k_ipsec_policy_free,
};

static void cn10k_ipsec_sa_wq_handler(struct work_struct *work)
{
	struct cn10k_ipsec *ipsec = container_of(work, struct cn10k_ipsec,
						 sa_work);
	struct otx2_nic *pf = container_of(ipsec, struct otx2_nic, ipsec);
	struct cn10k_inb_sw_ctx_info *inb_ctx_info, *tmp;
	int err;

	list_for_each_entry_safe(inb_ctx_info, tmp, &pf->ipsec.inb_sw_ctx_list,
				 list) {
		if (!inb_ctx_info->delete_npc_and_match_entry)
			continue;

		/* Delete all the associated NPC rules associated */
		err = cn10k_inb_delete_flow(pf, inb_ctx_info);
		if (err)
			netdev_err(pf->netdev,
				   "Failed to free UCAST_IPSEC entry %d\n",
				   inb_ctx_info->npc_mcam_entry);

		/* Remove SPI_TO_SA exact match entry */
		err = cn10k_inb_delete_spi_to_sa_match_entry(pf, inb_ctx_info);
		if (err)
			netdev_err(pf->netdev,
				   "Failed to delete spi_to_sa_match_entry\n");

		inb_ctx_info->delete_npc_and_match_entry = false;

		/* Finally clear the entry from the SA Table and free inb_ctx_info */
		clear_bit(inb_ctx_info->sa_index, pf->ipsec.inb_sa_table);
		list_del(&inb_ctx_info->list);
		devm_kfree(pf->dev, inb_ctx_info);
	}

	/* Disable static branch when no more SA(s) are enabled */
	if (list_empty(&pf->ipsec.inb_sw_ctx_list) && !pf->ipsec.outb_sa_count) {
		static_branch_disable(&cn10k_ipsec_sa_enabled);
		rtnl_lock();
		netdev_update_features(pf->netdev);
		rtnl_unlock();
	}
}

static void cn10k_ipsec_cleanup_send_queues(struct otx2_nic *pfvf)
{
	struct otx2_qset *qset = &pfvf->qset;
	struct otx2_snd_queue *sq;
	int qidx;

	for (qidx = 0; qidx < pfvf->hw.non_qos_queues; qidx++) {
		sq = &qset->sq[qidx];
		qmem_free(pfvf->dev, sq->sqe_ring);
		qmem_free(pfvf->dev, sq->cpt_resp);
		sq->sqe_ring = NULL;
		sq->cpt_resp = NULL;
	}
}

static int cn10k_ipsec_init_send_queues(struct otx2_nic *pfvf)
{
	struct otx2_qset *qset = &pfvf->qset;
	struct otx2_snd_queue *sq;
	int qidx, err;

	for (qidx = 0; qidx < pfvf->hw.non_qos_queues; qidx++) {
		/* Allocate memory for NIX SQE (which includes NIX SG) and
		 * CPT SG. SG of NIX and CPT are same in size. Allocate memory
		 * for CPT SG same as NIX SQE for base address alignment.
		 * Layout of a NIX SQE and CPT SG entry:
		 *      -----------------------------
		 *     |     CPT Scatter Gather      |
		 *     |       (SQE SIZE)            |
		 *     |                             |
		 *      -----------------------------
		 *     |       NIX SQE               |
		 *     |       (SQE SIZE)            |
		 *     |                             |
		 *      -----------------------------
		 */
		sq = &qset->sq[qidx];

		err = qmem_alloc(pfvf->dev, &sq->sqe_ring, qset->sqe_cnt,
				 sq->sqe_size * 2);
		if (err)
			goto free_mem;

		err = qmem_alloc(pfvf->dev, &sq->cpt_resp, qset->sqe_cnt, 64);
		if (err)
			goto free_mem;
	}

	return 0;

free_mem:
	cn10k_ipsec_cleanup_send_queues(pfvf);
	return err;
}

int cn10k_ipsec_send_queues_init(struct otx2_nic *pfvf)
{
	if (otx2_rep_dev(pfvf->pdev))
		return 0;

	if (pfvf->netdev->features & NETIF_F_HW_ESP)
		return cn10k_ipsec_init_send_queues(pfvf);

	return 0;
}

void cn10k_ipsec_send_queues_cleanup(struct otx2_nic *pfvf)
{
	if (otx2_rep_dev(pfvf->pdev))
		return;

	if (pfvf->netdev->features & NETIF_F_HW_ESP)
		cn10k_ipsec_cleanup_send_queues(pfvf);
}

void cn10k_ipsec_free_aura_ptrs(struct otx2_nic *pfvf)
{
	struct otx2_pool *pool;
	u64 iova, val;
	int pool_id;

	/* Disable threshold interrupt */
	val = FIELD_PREP(NPA_LF_AURA_OP_INT_AURA, pfvf->ipsec.inb_ipsec_pool);
	val |= NPA_LF_AURA_OP_INT_THRESH_ENA;
	otx2_write64(pfvf, NPA_LF_AURA_OP_INT, val);

	/* Free all first and second pass pool buffers */
	for (pool_id = pfvf->ipsec.inb_ipsec_spb_pool;
	     pool_id <= pfvf->ipsec.inb_ipsec_pool; pool_id++) {
		pool = &pfvf->qset.pool[pool_id];
		do {
			iova = otx2_aura_allocptr(pfvf, pool_id);
			if (!iova)
				break;
			otx2_free_bufs(pfvf, pool, iova - OTX2_HEAD_ROOM,
				       pfvf->rbsize);
		} while (1);
	}
}

static int cn10k_ipsec_free_cpt_bpid(struct otx2_nic *pfvf)
{
	struct nix_bpids *req;
	int rc;

	req = otx2_mbox_alloc_msg_nix_free_bpids(&pfvf->mbox);
	if (!req)
		return -ENOMEM;

	req->bpid_cnt = 2;
	req->bpids[0] = pfvf->ipsec.bpid;
	req->bpids[1] = pfvf->ipsec.spb_bpid;

	rc = otx2_sync_mbox_msg(&pfvf->mbox);
	if (rc)
		return rc;

	/* Clear the bpids */
	pfvf->ipsec.bpid = 0;
	pfvf->ipsec.spb_bpid = 0;
	return 0;
}

static void cn10k_ipsec_free_hw_resources(struct otx2_nic *pfvf)
{
	int vec;

	if (!cn10k_cpt_device_set_inuse(pfvf)) {
		netdev_err(pfvf->netdev, "CPT LF device unavailable\n");
		return;
	}

	cn10k_outb_cpt_clean(pfvf);

	/* Free the per spb pool buffer counters */
	devm_kfree(pfvf->dev, pfvf->ipsec.inb_spb_count);

	/* Free the inbound SA table */
	qmem_free(pfvf->dev, pfvf->ipsec.inb_sa);

	cn10k_ipsec_free_aura_ptrs(pfvf);

	vec = pci_irq_vector(pfvf->pdev, pfvf->hw.npa_msixoff);
	free_irq(vec, pfvf);

	if (pfvf->ipsec.bpid && pfvf->ipsec.spb_bpid)
		cn10k_ipsec_free_cpt_bpid(pfvf);
}

static int cn10k_ipsec_configure_cpt_bpid(struct otx2_nic *pfvf)
{
	struct nix_rx_chan_cfg *chan_cfg, *chan_cfg_rsp;
	struct nix_alloc_bpid_req *req;
	int chan, chan_cnt = 1;
	struct nix_bpids *rsp;
	u64 rx_chan_cfg;
	int rc;

	req = otx2_mbox_alloc_msg_nix_alloc_bpids(&pfvf->mbox);
	if (!req)
		return -ENOMEM;

	/* Request 2 BPIDs:
	 * One for 1st pass LPB pool and another for 2nd pass SPB pool
	 */
	req->bpid_cnt = 2;
	req->type = NIX_INTF_TYPE_CPT;

	rc = otx2_sync_mbox_msg(&pfvf->mbox);
	if (rc)
		return rc;

	rsp = (struct nix_bpids *)otx2_mbox_get_rsp(&pfvf->mbox.mbox, 0, &req->hdr);
	if (IS_ERR(rsp))
		return PTR_ERR(rsp);

	/* Store the bpid for configuring it in the future */
	pfvf->ipsec.bpid = rsp->bpids[0];
	pfvf->ipsec.spb_bpid = rsp->bpids[1];

	/* Get the default RX channel configuration */
	chan_cfg = otx2_mbox_alloc_msg_nix_rx_chan_cfg(&pfvf->mbox);
	if (!chan_cfg)
		return -ENOMEM;

	chan_cfg->read = true;
	rc = otx2_sync_mbox_msg(&pfvf->mbox);
	if (rc)
		return rc;

	/* Get the response */
	chan_cfg_rsp = (struct nix_rx_chan_cfg *)
			otx2_mbox_get_rsp(&pfvf->mbox.mbox, 0, &chan_cfg->hdr);

	rx_chan_cfg = chan_cfg_rsp->val;
	/* Find a free backpressure ID slot to configure */
	if (!FIELD_GET(NIX_AF_RX_CHANX_CFG_BP1_ENA, rx_chan_cfg)) {
		rx_chan_cfg |= FIELD_PREP(NIX_AF_RX_CHANX_CFG_BP1_ENA, 1) |
			       FIELD_PREP(NIX_AF_RX_CHANX_CFG_BP1, pfvf->ipsec.bpid);
	} else if (!FIELD_GET(NIX_AF_RX_CHANX_CFG_BP2_ENA, rx_chan_cfg)) {
		rx_chan_cfg |= FIELD_PREP(NIX_AF_RX_CHANX_CFG_BP2_ENA, 1) |
			       FIELD_PREP(NIX_AF_RX_CHANX_CFG_BP2, pfvf->ipsec.bpid);
	} else if (!FIELD_GET(NIX_AF_RX_CHANX_CFG_BP3_ENA, rx_chan_cfg)) {
		rx_chan_cfg |= FIELD_PREP(NIX_AF_RX_CHANX_CFG_BP3_ENA, 1) |
			       FIELD_PREP(NIX_AF_RX_CHANX_CFG_BP3, pfvf->ipsec.bpid);
	} else {
		netdev_err(pfvf->netdev, "No BPID available in RX channel\n");
		return -ENONET;
	}

	/* Update the RX_CHAN_CFG to listen to backpressure due to IPsec traffic */
	chan_cfg = otx2_mbox_alloc_msg_nix_rx_chan_cfg(&pfvf->mbox);
	if (!chan_cfg)
		return -ENOMEM;

	/* Configure BPID for PF RX channel */
	chan_cfg->val = rx_chan_cfg;
	rc = otx2_sync_mbox_msg(&pfvf->mbox);
	if (rc)
		return rc;

	/* Enable backpressure in CPT Link's RX Channel(s) */
#ifdef CONFIG_DCB
	chan_cnt = IEEE_8021QAZ_MAX_TCS;
#endif
	for (chan = 0; chan < chan_cnt; chan++) {
		chan_cfg = otx2_mbox_alloc_msg_nix_rx_chan_cfg(&pfvf->mbox);
		if (!chan_cfg)
			return -ENOMEM;

		/* CPT Link can be backpressured due to buffers reaching the
		 * threshold in SPB pool (pfvf->ipsec.spb_bpid) or due to CQ
		 * (pfvf->bpid[chan]) entries crossing the configured threshold
		 */
		chan_cfg->chan = chan;
		chan_cfg->type = NIX_INTF_TYPE_CPT;
		chan_cfg->val = FIELD_PREP(NIX_AF_RX_CHANX_CFG_BP0_ENA, 1) |
				FIELD_PREP(NIX_AF_RX_CHANX_CFG_BP0, pfvf->bpid[chan]) |
				FIELD_PREP(NIX_AF_RX_CHANX_CFG_BP1_ENA, 1) |
				FIELD_PREP(NIX_AF_RX_CHANX_CFG_BP1, pfvf->ipsec.spb_bpid);

		rc = otx2_sync_mbox_msg(&pfvf->mbox);
		if (rc)
			netdev_err(pfvf->netdev,
				   "Failed to enable backpressure on CPT channel %d\n",
				   chan);
	}

	return 0;
}

int cn10k_ipsec_ethtool_init(struct net_device *netdev, bool enable)
{
	struct otx2_nic *pf = netdev_priv(netdev);
	int err;

	/* IPsec offload supported on cn10k */
	if (!is_dev_support_ipsec_offload(pf->pdev))
		return -EOPNOTSUPP;

	/* Initialize CPT for outbound and inbound IPsec offload */
	if (enable) {
		if (netif_running(netdev)) {
			err = cn10k_ipsec_init_send_queues(pf);
			if (err)
				return err;
		}

		err = cn10k_outb_cpt_init(netdev);
		if (err)
			return err;

		/* Configure NIX <-> CPT backpressure */
		err = cn10k_ipsec_configure_cpt_bpid(pf);
		if (err)
			goto out;

		err = cn10k_inb_cpt_init(netdev);
		if (err)
			goto out;

		/* Set ipsec offload enabled for this device */
		pf->flags |= OTX2_FLAG_IPSEC_OFFLOAD_ENABLED;
		cn10k_cpt_device_set_available(pf);
		return err;
	}

	/* Don't cleanup CPT if any inbound or outbound SA is installed */
	if (!list_empty(&pf->ipsec.inb_sw_ctx_list) && !pf->ipsec.outb_sa_count) {
		netdev_err(pf->netdev, "SA installed on this device\n");
		return -EBUSY;
	}

out:
	/* Cleanup IPsec SQs and CPT */
	if (netif_running(netdev))
		cn10k_ipsec_cleanup_send_queues(pf);

	cn10k_ipsec_free_hw_resources(pf);
	cn10k_cpt_device_set_unavailable(pf);
	return err;
}

int cn10k_ipsec_init(struct net_device *netdev)
{
	struct otx2_nic *pf = netdev_priv(netdev);
	u32 sa_size;

	if (!is_dev_support_ipsec_offload(pf->pdev))
		return 0;

	/* Each SA entry size is 128 Byte round up in size */
	sa_size = sizeof(struct cn10k_tx_sa_s) % OTX2_ALIGN ?
			 (sizeof(struct cn10k_tx_sa_s) / OTX2_ALIGN + 1) *
			 OTX2_ALIGN : sizeof(struct cn10k_tx_sa_s);
	pf->ipsec.sa_size = sa_size;

	/* List to track all ingress SAs */
	INIT_LIST_HEAD(&pf->ipsec.inb_sw_ctx_list);

	INIT_WORK(&pf->ipsec.sa_work, cn10k_ipsec_sa_wq_handler);
	pf->ipsec.sa_workq = alloc_workqueue("cn10k_ipsec_sa_workq", 0, 0);
	if (!pf->ipsec.sa_workq) {
		netdev_err(pf->netdev, "SA alloc workqueue failed\n");
		return -ENOMEM;
	}

	/* Set xfrm device ops */
	netdev->xfrmdev_ops = &cn10k_ipsec_xfrmdev_ops;
	netdev->hw_features |= NETIF_F_HW_ESP;
	netdev->hw_enc_features |= NETIF_F_HW_ESP;

	cn10k_cpt_device_set_unavailable(pf);
	return 0;
}
EXPORT_SYMBOL(cn10k_ipsec_init);

void cn10k_ipsec_clean(struct otx2_nic *pf)
{
	if (!is_dev_support_ipsec_offload(pf->pdev))
		return;

	if (!(pf->flags & OTX2_FLAG_IPSEC_OFFLOAD_ENABLED))
		return;

	if (pf->ipsec.sa_workq) {
		destroy_workqueue(pf->ipsec.sa_workq);
		pf->ipsec.sa_workq = NULL;
	}

	/* Set ipsec offload disabled for this device */
	pf->flags &= ~OTX2_FLAG_IPSEC_OFFLOAD_ENABLED;

	cn10k_ipsec_free_hw_resources(pf);
}
EXPORT_SYMBOL(cn10k_ipsec_clean);

static u16 cn10k_ipsec_get_ip_data_len(struct xfrm_state *x,
				       struct sk_buff *skb)
{
	struct ipv6hdr *ipv6h;
	struct iphdr *iph;
	u8 *src;

	src = (u8 *)skb->data + ETH_HLEN;

	if (x->props.family == AF_INET) {
		iph = (struct iphdr *)src;
		return ntohs(iph->tot_len);
	}

	ipv6h = (struct ipv6hdr *)src;
	return ntohs(ipv6h->payload_len) + sizeof(struct ipv6hdr);
}

/* Prepare CPT and NIX SQE scatter/gather subdescriptor structure.
 * SG of NIX and CPT are same in size.
 * Layout of a NIX SQE and CPT SG entry:
 *      -----------------------------
 *     |     CPT Scatter Gather      |
 *     |       (SQE SIZE)            |
 *     |                             |
 *      -----------------------------
 *     |       NIX SQE               |
 *     |       (SQE SIZE)            |
 *     |                             |
 *      -----------------------------
 */
bool otx2_sqe_add_sg_ipsec(struct otx2_nic *pfvf, struct otx2_snd_queue *sq,
			   struct sk_buff *skb, int num_segs, int *offset)
{
	struct cpt_sg_s *cpt_sg = NULL;
	struct nix_sqe_sg_s *sg = NULL;
	u64 dma_addr, *iova = NULL;
	u64 *cpt_iova = NULL;
	u16 *sg_lens = NULL;
	int seg, len;

	sq->sg[sq->head].num_segs = 0;
	cpt_sg = (struct cpt_sg_s *)(sq->sqe_base - sq->sqe_size);

	for (seg = 0; seg < num_segs; seg++) {
		if ((seg % MAX_SEGS_PER_SG) == 0) {
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

			cpt_sg += (seg / MAX_SEGS_PER_SG) * 4;
			cpt_iova = (void *)cpt_sg + sizeof(*cpt_sg);
		}
		dma_addr = otx2_dma_map_skb_frag(pfvf, skb, seg, &len);
		if (dma_mapping_error(pfvf->dev, dma_addr))
			return false;

		sg_lens[seg % MAX_SEGS_PER_SG] = len;
		sg->segs++;
		*iova++ = dma_addr;
		*cpt_iova++ = dma_addr;

		/* Save DMA mapping info for later unmapping */
		sq->sg[sq->head].dma_addr[seg] = dma_addr;
		sq->sg[sq->head].size[seg] = len;
		sq->sg[sq->head].num_segs++;

		*cpt_sg = *(struct cpt_sg_s *)sg;
		cpt_sg->rsvd_63_50 = 0;
	}

	sq->sg[sq->head].skb = (u64)skb;
	return true;
}

static u16 cn10k_ipsec_get_param1(u8 iv_offset)
{
	u16 param1_val;

	/* Set Crypto mode, disable L3/L4 checksum */
	param1_val = CN10K_IPSEC_INST_PARAM1_DIS_L4_CSUM |
		      CN10K_IPSEC_INST_PARAM1_DIS_L3_CSUM;
	param1_val |= (u16)iv_offset << CN10K_IPSEC_INST_PARAM1_IV_OFFSET_SHIFT;
	return param1_val;
}

bool cn10k_ipsec_transmit(struct otx2_nic *pf, struct netdev_queue *txq,
			  struct otx2_snd_queue *sq, struct sk_buff *skb,
			  int num_segs, int size)
{
	struct cpt_inst_s inst;
	struct cpt_res_s *res;
	struct xfrm_state *x;
	struct qmem *sa_info;
	dma_addr_t dptr_iova;
	struct sec_path *sp;
	u8 encap_offset;
	u8 auth_offset;
	u8 gthr_size;
	u8 iv_offset;
	u16 dlen;

	/* Check for IPSEC offload enabled */
	if (!(pf->flags & OTX2_FLAG_IPSEC_OFFLOAD_ENABLED))
		goto drop;

	sp = skb_sec_path(skb);
	if (unlikely(!sp->len))
		goto drop;

	x = xfrm_input_state(skb);
	if (unlikely(!x))
		goto drop;

	if (x->props.mode != XFRM_MODE_TRANSPORT &&
	    x->props.mode != XFRM_MODE_TUNNEL)
		goto drop;

	dlen = cn10k_ipsec_get_ip_data_len(x, skb);
	if (dlen == 0 && netif_msg_tx_err(pf)) {
		netdev_err(pf->netdev, "Invalid IP header, ip-length zero\n");
		goto drop;
	}

	/* Check for valid SA context */
	sa_info = (struct qmem *)x->xso.offload_handle;
	if (!sa_info)
		goto drop;

	memset(&inst, 0, sizeof(struct cpt_inst_s));

	/* Get authentication offset */
	if (x->props.family == AF_INET)
		auth_offset = sizeof(struct iphdr);
	else
		auth_offset = sizeof(struct ipv6hdr);

	/* IV offset is after ESP header */
	iv_offset = auth_offset + sizeof(struct ip_esp_hdr);
	/* Encap will start after IV */
	encap_offset = iv_offset + GCM_RFC4106_IV_SIZE;

	/* CPT Instruction word-1 */
	res = (struct cpt_res_s *)(sq->cpt_resp->base + (64 * sq->head));
	res->compcode = 0;
	inst.res_addr = sq->cpt_resp->iova + (64 * sq->head);

	/* CPT Instruction word-2 */
	inst.rvu_pf_func = pf->pcifunc;

	/* CPT Instruction word-3:
	 * Set QORD to force CPT_RES_S write completion
	 */
	inst.qord = 1;

	/* CPT Instruction word-4 */
	/* inst.dlen should not include ICV length */
	inst.dlen = dlen + ETH_HLEN - (x->aead->alg_icv_len / 8);
	inst.opcode_major = CN10K_IPSEC_MAJOR_OP_OUTB_IPSEC;
	inst.param1 = cn10k_ipsec_get_param1(iv_offset);

	inst.param2 = encap_offset <<
		       CN10K_IPSEC_INST_PARAM2_ENC_DATA_OFFSET_SHIFT;
	inst.param2 |= (u16)auth_offset <<
			CN10K_IPSEC_INST_PARAM2_AUTH_DATA_OFFSET_SHIFT;

	/* CPT Instruction word-5 */
	gthr_size = num_segs / MAX_SEGS_PER_SG;
	gthr_size = (num_segs % MAX_SEGS_PER_SG) ? gthr_size + 1 : gthr_size;

	gthr_size &= 0xF;
	dptr_iova = (sq->sqe_ring->iova + (sq->head * (sq->sqe_size * 2)));
	inst.dptr = dptr_iova | ((u64)gthr_size << 60);

	/* CPT Instruction word-6 */
	inst.rptr = inst.dptr;

	/* CPT Instruction word-7 */
	inst.cptr = sa_info->iova;
	inst.ctx_val = 1;
	inst.egrp = CN10K_DEF_CPT_IPSEC_EGRP;

	/* CPT Instruction word-0 */
	inst.nixtxl = (size / 16) - 1;
	inst.dat_offset = ETH_HLEN;
	inst.nixtx_offset = sq->sqe_size;

	netdev_tx_sent_queue(txq, skb->len);

	/* Finally Flush the CPT instruction */
	sq->head++;
	sq->head &= (sq->sqe_cnt - 1);
	cn10k_cpt_inst_flush(pf, &inst, sizeof(struct cpt_inst_s));
	return true;
drop:
	dev_kfree_skb_any(skb);
	return false;
}
