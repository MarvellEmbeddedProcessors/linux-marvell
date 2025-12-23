// SPDX-License-Identifier: GPL-2.0
/* Marvell RVU Admin Function driver
 *
 * Copyright (C) 2024 Marvell.
 *
 */

#include <linux/module.h>
#include <linux/pci.h>

#include "struct.h"
#include "../rvu.h"

static struct workqueue_struct *re_flt_wq;
struct cpt_re_flt_work_data {
	struct work_struct cpt_re_wd_work;
	u32 tile_start_eng;
	struct rvu *rvu;
};

#define NUM_RE_ENGINES_PER_TILE 8
/* Each RE tile is in multiple of 8 engines */
#define GET_RE_ENGINE_TILE_ID(_re) ((_re) / NUM_RE_ENGINES_PER_TILE)

#define ERR_CODE_MASK  GENMASK_ULL(31, 24)
#define ERR_EID_MASK   GENMASK_ULL(7, 0)
#define WD_TIMEOUT_ERR_CODE 0x4
/* No. of AE 16 bits for FLT_INTR_VEC_2 */
#define AE_ENG_NR_BITS_FLT_INTR_VEC_2 16
/* Compute bit index inside FLTX_INT_VEC3 (vector 3) for an absolute RE
 * engine index. Subtract MAX AE bits to no.of bits already represented in
 * earlier vector(i.e FLTX_INT_VEC2).
 */
#define GET_FLT_INTR_VEC3_REG_RE_IDX(_reg) ({ \
	FIELD_GET(MAX_AE, reg) - \
	min_t(u64, FIELD_GET(MAX_AE, reg), AE_ENG_NR_BITS_FLT_INTR_VEC_2); \
})
static int re_start_eng;
static int re_max_engines;
/* RE tile start vector 3 register index */
static int re_tile_start_vec3_reg_idx;
static inline bool cpt_re_check_eng_wd_timeout(int eng, struct rvu *rvu)
{
	u64 reg;

	reg = rvu_read64(rvu, BLKADDR_CPT0, CPT_AF_EXE_ERR_INFO);

	return ((FIELD_GET(ERR_CODE_MASK, reg) == WD_TIMEOUT_ERR_CODE) &&
		(FIELD_GET(ERR_EID_MASK, reg) == (eng & ERR_EID_MASK)));
}

static inline bool cpt_is_re_eng_valid(int eng)
{
	if (eng < re_start_eng ||
	    eng >= (re_start_eng + re_max_engines))
		return false;

	return true;
}

static inline int cpt_re_get_tile_start_eng(int eng)
{
	u32 tile_id;

	tile_id = GET_RE_ENGINE_TILE_ID(eng - re_start_eng);
	return re_start_eng + tile_id * NUM_RE_ENGINES_PER_TILE;
}

static void cpt_re_eng_intr_set(u32 reg_eng_idx, struct rvu *rvu, u64 reg)
{
	u64 regval;

	regval = rvu_read64(rvu, BLKADDR_CPT0, reg);
	regval |= (1 << reg_eng_idx);
	rvu_write64(rvu, BLKADDR_CPT0, reg, regval);
}

static int cpt_re_eng_intr_disable(u32 re_tile_start_eng_id, struct rvu *rvu)
{
	u32 tile_start_idx;
	u64 reg;
	u32 eng;

	tile_start_idx = re_tile_start_eng_id - re_start_eng;
	reg = rvu_read64(rvu, BLKADDR_CPT0, CPT_AF_FLTX_INT_ENA_W1S(3));
	/* check if intr of re tile starting engine already masked */
	if ((reg & BIT_ULL(tile_start_idx + re_tile_start_vec3_reg_idx)) == 0)
		return -EALREADY;

	for (eng = tile_start_idx;
	     eng < tile_start_idx + NUM_RE_ENGINES_PER_TILE; eng++)
		cpt_re_eng_intr_set(eng + re_tile_start_vec3_reg_idx, rvu,
				    CPT_AF_FLTX_INT_ENA_W1C(3));
	return 0;
}

static void cpt_re_eng_intr_enable(u32 re_tile_start_eng_id, struct rvu *rvu)
{
	u32 tile_start_idx;
	u32 eng;

	tile_start_idx = re_tile_start_eng_id - re_start_eng;
	for (eng = tile_start_idx;
	     eng < tile_start_idx + NUM_RE_ENGINES_PER_TILE; eng++)
		cpt_re_eng_intr_set(eng + re_tile_start_vec3_reg_idx, rvu,
				    CPT_AF_FLTX_INT_ENA_W1S(3));
}

void cpt_cn20k_rxc_time_cfg(struct rvu *rvu, int blkaddr,
			    struct cpt_rxc_time_cfg_req *req,
			    struct cpt_rxc_time_cfg_req *save)
{
	u16 qid = req->queue_id;
	u64 dfrg_reg;

	if (save) {
		/* Save older config */
		dfrg_reg = rvu_read64(rvu, blkaddr, CPT_AF_RXC_QUEX_DFRG(qid));
		save->zombie_thres = FIELD_GET(RXC_ZOMBIE_THRES, dfrg_reg);
		save->zombie_limit = FIELD_GET(RXC_ZOMBIE_LIMIT, dfrg_reg);
		save->active_thres = FIELD_GET(RXC_ACTIVE_THRES, dfrg_reg);
		save->active_limit = FIELD_GET(RXC_ACTIVE_LIMIT, dfrg_reg);

		save->step = rvu_read64(rvu, blkaddr, CPT_AF_RXC_TIME_CFG);

		save->cpt_af_rxc_que_cfg = rvu_read64(rvu, blkaddr,
						      CPT_AF_RXC_QUEX_CFG(qid));
	}

	dfrg_reg = FIELD_PREP(RXC_ZOMBIE_THRES, req->zombie_thres);
	dfrg_reg |= FIELD_PREP(RXC_ZOMBIE_LIMIT, req->zombie_limit);
	dfrg_reg |= FIELD_PREP(RXC_ACTIVE_THRES, req->active_thres);
	dfrg_reg |= FIELD_PREP(RXC_ACTIVE_LIMIT, req->active_limit);

	rvu_write64(rvu, blkaddr, CPT_AF_RXC_TIME_CFG, req->step);
	rvu_write64(rvu, blkaddr, CPT_AF_RXC_QUEX_DFRG(qid), dfrg_reg);
	rvu_write64(rvu, blkaddr, CPT_AF_RXC_QUEX_CFG(qid),
		    req->cpt_af_rxc_que_cfg);
}

static void cpt_cn20k_rxc_flush(struct rvu *rvu, u16 pcifunc, int blkaddr)
{
	struct cpt_rxc_time_cfg_req req, prev;
	struct rvu_cpt *cpt = &rvu->cpt;
	int timeout = 2000, queue_idx;
	u64 reg;

	/* Set time limit to minimum values, so that rxc entries will be
	 * flushed out quickly.
	 */
	req.step = 1;
	req.zombie_thres = 1;
	req.zombie_limit = 1;
	req.active_thres = 1;
	req.active_limit = 1;

	for_each_set_bit(queue_idx, cpt->cpt_rx_queue_bitmap,
			 CPT_AF_MAX_RXC_QUEUES) {
		/* Skip queues that don't match the given pcifunc, unless
		 * it's queue 0.
		 */
		if (cpt->cptpfvf_map[queue_idx] != pcifunc && queue_idx)
			continue;

		req.queue_id = queue_idx;
		prev.queue_id = queue_idx;

		cpt_cn20k_rxc_time_cfg(rvu, blkaddr, &req, &prev);

		do {
			reg = rvu_read64(rvu, blkaddr,
					 CPT_AF_RXC_QUEX_ACTIVE_STS(queue_idx));
			udelay(1);
			if (FIELD_GET(RXC_ACTIVE_COUNT, reg))
				timeout--;
			else
				break;
		} while (timeout);

		if (timeout == 0)
			dev_warn(rvu->dev,
				 "Poll for RXC active count hits hard loop counter\n");

		timeout = 2000;
		do {
			reg = rvu_read64(rvu, blkaddr,
					 CPT_AF_RXC_QUEX_ZOMBIE_STS(queue_idx));
			udelay(1);
			if (FIELD_GET(RXC_ZOMBIE_COUNT, reg))
				timeout--;
			else
				break;
		} while (timeout);

		if (timeout == 0)
			dev_warn(rvu->dev,
				 "Poll for RXC zombie count hits hard loop counter\n");

		/* Restore config */
		cpt_cn20k_rxc_time_cfg(rvu, blkaddr, &prev, NULL);
	}
}

int cpt_cn20k_ctx_flush(struct rvu *rvu, int blkaddr, u16 pcifunc)
{
	/* Flush RXC */
	cpt_cn20k_rxc_flush(rvu, pcifunc, blkaddr);

	 /* Invalidate all context entries for the given 'pcifunc' */
	rvu_write64(rvu, blkaddr, CPT_AF_CTX_PFF_INVAL,
		    FIELD_PREP(CPT_CTX_INVAL_PFFUNC, pcifunc));

	return 0;
}

void cpt_cn20k_rxc_teardown(struct rvu *rvu, u16 pcifunc, int blkaddr)
{
	struct rvu_cpt *cpt = &rvu->cpt;
	int queue_idx = 1;

	/* Flush RXC and context */
	cpt_cn20k_ctx_flush(rvu, blkaddr, pcifunc);

	for_each_set_bit_from(queue_idx, cpt->cpt_rx_queue_bitmap,
			      CPT_AF_MAX_RXC_QUEUES) {
		/* Skip queues that don't match the given pcifunc */
		if (cpt->cptpfvf_map[queue_idx] != pcifunc)
			continue;

		/* Free queue except global queue and remove pf-func mapping */
		__clear_bit(queue_idx, cpt->cpt_rx_queue_bitmap);
		cpt->cptpfvf_map[queue_idx] = 0;
	}
}

static int cpt_rx_inline_queue_cfg(struct rvu *rvu, int blkaddr, u8 cptlf,
				   struct cpt_rx_inline_qcfg_req *req)
{
	u8 pri_mask = otx2_cpt_que_pri_mask(rvu);
	u16 sso_pf_func = req->sso_pf_func;
	u8 nix_queue = req->rx_queue_id;
	u64 val;

	val = rvu_read64(rvu, blkaddr, CPT_AF_LFX_CTL(cptlf));
	if (req->enable && (val & BIT_ULL(16))) {
		/* IPSec inline outbound path is already enabled for a given
		 * CPT LF, HRM states that inline inbound & outbound paths
		 * must not be enabled at the same time for a given CPT LF
		 */
		return CPT_AF_ERR_INLINE_IPSEC_INB_ENA;
	}

	/* Check if requested 'CPTLF <=> SSOLF' mapping is valid */
	if (sso_pf_func && !is_pffunc_map_valid(rvu, sso_pf_func, BLKTYPE_SSO))
		return CPT_AF_ERR_SSO_PF_FUNC_INVALID;

	/* Check if requested 'CPTLF <=> NIXLF' mapping is valid */
	if (req->nix_pf_func) {
		if (!is_pffunc_map_valid(rvu, req->nix_pf_func,
					 BLKTYPE_NIX))
			return CPT_AF_ERR_NIX_PF_FUNC_INVALID;
	}

	/* Disable CPT LF for IPsec inline inbound operations */
	val &= ~BIT_ULL(9);

	if (req->enable) {
		if (req->eng_grpmsk == 0x0)
			return CPT_AF_ERR_GRP_INVALID;

		if (req->queue_pri > pri_mask)
			return CPT_AF_ERR_PRI_INVALID;

		if (req->ctx_ilen > CPT_AF_MAX_CTX_ILEN)
			return CPT_AF_ERR_CTX_ILEN_INVALID;

		/* Enable CPT LF for IPsec inline inbound operations */
		val |= BIT_ULL(9);

		if (req->pf_func_ctx)
			val |= BIT_ULL(20);
		else
			val &= ~BIT_ULL(20);

		val &= ~CPT_AF_ENG_GRPMASK;
		val |= FIELD_PREP(CPT_AF_ENG_GRPMASK, req->eng_grpmsk);
		val &= ~pri_mask;
		val |= FIELD_PREP(CPT_AF_QUEUE_PRI, req->queue_pri);
		val &= ~CPT_AF_INFLIGHT_LIMIT;
		val |= FIELD_PREP(CPT_AF_INFLIGHT_LIMIT, req->inflight_limit);
		if (req->ctx_ilen) {
			val &= ~CPT_AF_CTX_ILEN;
			val |= FIELD_PREP(CPT_AF_CTX_ILEN, req->ctx_ilen);
		} else {
			val &= ~CPT_AF_CTX_ILEN;
			val |= FIELD_PREP(CPT_AF_CTX_ILEN, CPT_CTX_ILEN);
		}
		val &= ~CPT_AF_RXC_QUEUE;
		val |= FIELD_PREP(CPT_AF_RXC_QUEUE, nix_queue);
		val &= ~CPT_AF_NIX_QUEUE;
		val |= FIELD_PREP(CPT_AF_NIX_QUEUE, nix_queue);
	}

	rvu_write64(rvu, blkaddr, CPT_AF_LFX_CTL(cptlf), val);

	val = rvu_read64(rvu, blkaddr, CPT_AF_LFX_CTL2(cptlf));
	if (sso_pf_func) {
		/* Set SSO, and NIX_PF_FUNC */
		val &= ~(CPT_AF_SSO_PF_FUNC | CPT_AF_NIX_PF_FUNC);
		val |= FIELD_PREP(CPT_AF_SSO_PF_FUNC, req->sso_pf_func);
		val |= FIELD_PREP(CPT_AF_NIX_PF_FUNC, req->nix_pf_func);
	}
	if (req->pf_func_ctx) {
		val &= ~CPT_AF_CTX_PF_FUNC;
		val |= FIELD_PREP(CPT_AF_CTX_PF_FUNC, req->ctx_pf_func);
	}
	rvu_write64(rvu, blkaddr, CPT_AF_LFX_CTL2(cptlf), val);

	val = req->pdb_ena ? BIT_ULL(0) : 0;
	val |= req->cq_remap ? BIT_ULL(1) : 0;
	rvu_write64(rvu, blkaddr, CPT_AF_CN20K_NIXRXX_CFG(nix_queue), val);

	return 0;
}

int rvu_mbox_handler_cpt_rx_inl_queue_cfg(struct rvu *rvu,
					  struct cpt_rx_inline_qcfg_req *req,
					  struct msg_rsp *rsp)
{
	struct rvu_cpt *cpt = &rvu->cpt;
	u16 pcifunc = req->hdr.pcifunc;
	struct rvu_block *block;
	int cptlf, blkaddr;
	u16 actual_slot;

	if (!is_cn20k(rvu->pdev)) {
		dev_err(rvu->dev, "Mbox support is only for cn20k\n");
		return -EOPNOTSUPP;
	}

	if (req->rx_queue_id >= CPT_AF_MAX_RXC_QUEUES)
		return CPT_AF_ERR_RXC_QUEUE_INVALID;

	if (cpt->cptpfvf_map[req->rx_queue_id] != req->hdr.pcifunc)
		return CPT_AF_ERR_QUEUE_PCIFUNC_MAP_INVALID;

	blkaddr = rvu_get_blkaddr_from_slot(rvu, BLKTYPE_CPT, pcifunc,
					    req->slot, &actual_slot);
	if (blkaddr < 0)
		return CPT_AF_ERR_LF_INVALID;

	block = &rvu->hw->block[blkaddr];

	cptlf = rvu_get_lf(rvu, block, pcifunc, actual_slot);
	if (cptlf < 0)
		return CPT_AF_ERR_LF_INVALID;

	return cpt_rx_inline_queue_cfg(rvu, blkaddr, cptlf, req);
}

int rvu_mbox_handler_cpt_rx_inline_qalloc(struct rvu *rvu,
					  struct msg_req *req,
					  struct cpt_rx_inline_qalloc_rsp *rsp)
{
	struct rvu_cpt *cpt = &rvu->cpt;

	int index = find_first_zero_bit(cpt->cpt_rx_queue_bitmap,
					CPT_AF_MAX_RXC_QUEUES);
	if (index >= CPT_AF_MAX_RXC_QUEUES)
		return CPT_AF_ERR_RXC_QUEUE_INVALID;

	/* Flag the queue ID as allocated */
	set_bit(index, cpt->cpt_rx_queue_bitmap);

	cpt->cptpfvf_map[index] = req->hdr.pcifunc;
	rsp->rx_queue_id = index;

	return 0;
}

static void cpt_rx_qid_init(struct rvu *rvu)
{
	struct rvu_cpt *cpt = &rvu->cpt;

	bitmap_zero(cpt->cpt_rx_queue_bitmap, CPT_AF_MAX_RXC_QUEUES);

	/* Queue 0 is reserved for Global LF, that is allocated via CPT
	 * PF, and can be used by RVU netdev.
	 */
	set_bit(0, cpt->cpt_rx_queue_bitmap);
}

static void cpt_cn20k_cfg_ucc_cq_err_codes(struct rvu *rvu, int blkaddr)
{
	static const u8 ucc_error_codes[] = {
		/* Success/Warning Codes */
		0xF0,  /* UCODE_IPSEC_SUCCESS_SOFT_LIFETIME_EXPIRED */
		0xF2,  /* UCODE_IPSEC_SUCCESS_SOFT_LIFETIME2_EXPIRED */
		/* SA-Related Errors */
		0xB0,  /* ERR_UCODE_IPSEC_SA_INVAL */
		0xB1,  /* ERR_UCODE_IPSEC_SA_EXPIRED */
		0xB2,  /* ERR_UCODE_IPSEC_SA_OVERFLO */
		0xB3,  /* ERR_UCODE_IPSEC_SA_ESP_BADALGO */
		0xB4,  /* ERR_UCODE_IPSEC_SA_AH_BADALGO */
		0xB5,  /* ERR_UCODE_IPSEC_SA_BADCTX */
		0xB6,  /* ERR_UCODE_IPSEC_SA_CTX_FLAG_MISMATCH */
		/* Atomic Operation Error */
		0xB7,  /* ERR_UCODE_IPSEC_AOP_ERR */
		/* Packet Errors */
		0xB8,  /* ERR_UCODE_IPSEC_PKT_IP */
		0xB9,  /* ERR_UCODE_IPSEC_PKT_IP6_BADEXT */
		0xBA,  /* ERR_UCODE_IPSEC_PKT_IP6_HBH */
		0xBB,  /* ERR_UCODE_IPSEC_PKT_IP6_BIGEXT */
		/* Additional Errors */
		0xC0,  /* ERR_UCODE_IPSEC_PKT_BADICV */
		0xC4,  /* ERR_UCODE_IPSEC_BAD_DLEN */
		0xC5,  /* ERR_UCODE_SA_ESP_BADKEYS5 */
		0xC6,  /* ERR_UCODE_SA_AH_BADKEYS */
		0xC7   /* ERR_UCODE_BADIP */
	};
	u64 reg_val = CPT_AF_UCC_CTL_CQ_ENA_MASK;
	int i;

	for (i = 0; i < ARRAY_SIZE(ucc_error_codes); i++)
		rvu_write64(rvu, blkaddr,
			    CPT_AF_UCCX_CTL(ucc_error_codes[i]), reg_val);
}

void rvu_cn20k_cpt_init(struct rvu *rvu)
{
	u64 reg_val;

	/* Enable CPT parse and Frag info header byte swapping. This
	 * must be consistent with NIX_AF_CFG.CPT_MP_ENA_SWAP, So that
	 * nix knows that these headers are byte swapped.
	 */

	reg_val = rvu_read64(rvu, BLKADDR_CPT0, CPT_AF_RXC_CFG2);
	reg_val |= BIT_ULL(8);
	rvu_write64(rvu, BLKADDR_CPT0, CPT_AF_RXC_CFG2, reg_val);

	/* Set the default value for metadata result offset to 1 */
	reg_val = rvu_read64(rvu, BLKADDR_CPT0, CPT_AF_CTL);
	reg_val |= FIELD_PREP(CPT_AF_CTL_RES_META_OFFSET, 1);
	rvu_write64(rvu, BLKADDR_CPT0, CPT_AF_CTL, reg_val);

	/* Configure UCC error codes for inline IPsec CQ error path */
	cpt_cn20k_cfg_ucc_cq_err_codes(rvu, BLKADDR_CPT0);

	cpt_rx_qid_init(rvu);
}

void cpt_cn20k_re_flt_destroy(void)
{
	if (!re_flt_wq)
		return;
	flush_workqueue(re_flt_wq);
	destroy_workqueue(re_flt_wq);
	re_flt_wq = NULL;
}

int cpt_cn20k_re_flt_init(struct rvu *rvu)
{
	u64 reg;

	reg = rvu_read64(rvu, BLKADDR_CPT0, CPT_AF_CONSTANTS1);
	re_max_engines = FIELD_GET(MAX_RE, reg);
	if (!re_max_engines)
		return 0;

	re_start_eng = FIELD_GET(MAX_SE, reg) + FIELD_GET(MAX_IE, reg) +
		       FIELD_GET(MAX_AE, reg);
	re_tile_start_vec3_reg_idx = GET_FLT_INTR_VEC3_REG_RE_IDX(reg);
	re_flt_wq = alloc_workqueue("cpt_re_flt_wq",
				    WQ_UNBOUND | WQ_MEM_RECLAIM, 0);
	if (!re_flt_wq) {
		dev_err(rvu->dev,
			"CPT_RE: Reset FLT workqueue creation failed\n");
		return -ENOMEM;
	}
	return 0;
}

static inline bool cpt_re_check_flt_wd_timeout(int eng, struct rvu *rvu)
{
	if (!re_max_engines || !cpt_is_re_eng_valid(eng))
		return false;
	return cpt_re_check_eng_wd_timeout(eng, rvu);
}

static void cpt_re_flt_work_handler(struct work_struct *work)
{
	u8 eng_grp_mask[NUM_RE_ENGINES_PER_TILE] = {0};
	struct cpt_re_flt_work_data *work_data;
	struct rvu *rvu;
	int start_eng;
	int end_eng;
	int eng;
	u64 reg;

	work_data = container_of(work, struct cpt_re_flt_work_data,
				 cpt_re_wd_work);
	start_eng = work_data->tile_start_eng;
	end_eng = start_eng + NUM_RE_ENGINES_PER_TILE;
	rvu = work_data->rvu;
	/* Clear group masks for all the engines in the tile */
	for (eng = start_eng; eng < end_eng; eng++) {
		reg = rvu_read64(rvu, BLKADDR_CPT0, CPT_AF_EXEX_CTL2(eng));
		eng_grp_mask[eng - start_eng] = reg & 0xFF;
		rvu_write64(rvu, BLKADDR_CPT0, CPT_AF_EXEX_CTL2(eng), 0x0);
	}
	/* Wait for all the engines in the tile to be idle */
	for (eng = start_eng; eng < end_eng; eng++) {
		u32 timeout = 5000;

		do {
			reg = rvu_read64(rvu, BLKADDR_CPT0,
					 CPT_AF_EXEX_STS(eng));
			if ((reg & CPT_AF_EXE_STS_MASK) == 0)
				break;
			udelay(1);
			if (--timeout == 0) {
				dev_warn(rvu->dev,
					 "CPT FLT RE engine %d failed to go idle\n",
					 eng);
				break;
			}
		} while (1);
	}
	/* Disable and enable all the engines in the tile */
	for (eng = start_eng; eng < end_eng; eng++) {
		reg = rvu_read64(rvu, BLKADDR_CPT0, CPT_AF_EXEX_CTL(eng));
		rvu_write64(rvu, BLKADDR_CPT0,
			    CPT_AF_EXEX_CTL(eng), reg & ~1ULL);
		rvu_write64(rvu, BLKADDR_CPT0,
			    CPT_AF_EXEX_CTL(eng), reg | 1ULL);
	}
	/* Put all the engines in the tile group mask back */
	for (eng = start_eng; eng < end_eng; eng++) {
		rvu_write64(rvu, BLKADDR_CPT0, CPT_AF_EXEX_CTL2(eng),
			    eng_grp_mask[eng - start_eng]);
	}
	/* enable RE engines interrupts again */
	cpt_re_eng_intr_enable(start_eng, rvu);
	kfree(work_data);
}

bool cpt_cn20k_re_flt_handler(int eng, struct rvu *rvu)
{
	struct cpt_re_flt_work_data *work_data;
	u32 tile_start_eng;

	if (!cpt_re_check_flt_wd_timeout(eng, rvu))
		return false;

	tile_start_eng = cpt_re_get_tile_start_eng(eng);
	if (cpt_re_eng_intr_disable(tile_start_eng, rvu) < 0)
		return true;

	work_data = kmalloc(sizeof(*work_data), GFP_ATOMIC);
	if (!work_data) {
		cpt_re_eng_intr_enable(tile_start_eng, rvu);
		return true;
	}

	work_data->rvu = rvu;
	work_data->tile_start_eng = tile_start_eng;
	INIT_WORK(&work_data->cpt_re_wd_work, cpt_re_flt_work_handler);
	queue_work(re_flt_wq, &work_data->cpt_re_wd_work);
	return true;
}
