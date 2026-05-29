// SPDX-License-Identifier: GPL-2.0
#include <linux/bitfield.h>
#include <linux/etherdevice.h>
#include <linux/mutex.h>
#include <linux/pci.h>
#include <linux/errno.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/mm.h>
#include <linux/slab.h>
#include <linux/types.h>
#include <linux/uaccess.h>
#include <net/tls.h>

#include "pan_cmn.h"

#define OTX2_FLAG_KTLS_TX_ENABLED	BIT_ULL(21)

struct otx2_nic *pan_nic;
struct pan_ktls_dev pan_ktls;

struct ktls_cpt_ctx {
	u64 io_addr;
	struct cn10k_cpt_inst_queue iq;
	struct qmem *ctx;
	u16 nr_connections;
};

struct ktls_cpt_ctx kcpt_ctx;

static void pan_ktls_tx_lf_free(struct otx2_nic *pf)
{
	mutex_lock(&pf->mbox.lock);
	otx2_mbox_alloc_msg_cpt_lf_free(&pf->mbox);
	otx2_sync_mbox_msg(&pf->mbox);
	mutex_unlock(&pf->mbox.lock);
}

static void pan_ktls_tx_iq_enable(struct otx2_nic *pf)
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

static inline void pan_ktls_tx_iq_disable(struct otx2_nic *pf)
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
			dev_err(pf->dev, "Error CPT LF is still busy\n");
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

static int pan_ktls_tx_iq_init(struct otx2_nic *pf)
{
	u64 reg_val;
	int ret;

	/* Allocate Memory for CPT IQ */
	ret = pan_cpt_tx_iq_alloc(pf, &kcpt_ctx.iq);
	if (ret)
		return ret;

	/* Disable IQ */
	pan_ktls_tx_iq_disable(pf);

	/* Set IQ base address */
	otx2_write64(pf, CN10K_CPT_LF_Q_BASE, kcpt_ctx.iq.dma_addr);

	/* Set IQ size */
	reg_val = FIELD_PREP(CPT_LF_Q_SIZE_DIV40, CN10K_CPT_SIZE_DIV40 +
			     CN10K_CPT_EXTRA_SIZE_DIV40);
	otx2_write64(pf, CN10K_CPT_LF_Q_SIZE, reg_val);

	return 0;
}

static int pan_ktls_tx_hw_init(struct otx2_nic *pf)
{
	int ret;

	ret = pan_ktls_tx_iq_init(pf);
	if (ret)
		return ret;

	pan_ktls_tx_iq_enable(pf);

	return 0;
}

static int pan_ktls_tx_init(struct otx2_nic *pf)
{
	int ret;

	/* TODO: For now use BLKADDR_CPT0 */
	ret = pan_cpt_tx_lf_alloc(pf);
	if (ret)
		return ret;

	ret = qmem_alloc(pf->dev, &kcpt_ctx.ctx, CN10K_CPT_MAX_KTLS_CTX,
			 ALIGN(sizeof(struct pan_cpt_tls_ctx_s), 128));
	if (ret)
		return -ENOMEM;

	ret = pan_ktls_tx_hw_init(pf);
	if (ret) {
		pan_ktls_tx_lf_free(pf);
		return ret;
	}

	kcpt_ctx.io_addr = (__force u64)otx2_get_regaddr(pf, CN10K_CPT_LF_NQX(0));

	/* Mark Inline ktls enabled for this device now */
	pf->flags |= OTX2_FLAG_KTLS_TX_ENABLED;

	return 0;
}

static void pan_ktls_tx_clean(struct otx2_nic *pf)
{
	/* Mark Inline ktls disabled for this device */
	pf->flags &= ~OTX2_FLAG_KTLS_TX_ENABLED;

	/* Disable IQ */
	pan_ktls_tx_iq_disable(pf);

	/* Set IQ base address  and size to 0 */
	otx2_write64(pf, CN10K_CPT_LF_Q_BASE, 0);
	otx2_write64(pf, CN10K_CPT_LF_Q_SIZE, 0);

	pan_cpt_tx_iq_free(pf, &kcpt_ctx.iq);
}

static struct pan_cpt_tls_ctx_s *cn10k_ktls_new_conn(struct otx2_nic *pf,
						     u8 conn_id)
{
	struct pan_cpt_tls_ctx_s *ktls_ctx;
	struct qmem *ctx = kcpt_ctx.ctx;

	if (conn_id >= CN10K_CPT_MAX_KTLS_CTX)
		return NULL;

	ktls_ctx = ctx->base + conn_id * ALIGN(sizeof(struct pan_cpt_tls_ctx_s), 128);
	return ktls_ctx;
}

static int pan_ktls_submit(struct otx2_nic *pf, struct otx2_rcv_queue *rq,
			   u8 *packet, unsigned int len,
			   struct pan_tuple_hdr *hdr, u8 conn_id)
{
	struct tcphdr *th = (struct tcphdr *)(hdr->l4hdr);
	struct qmem *ctx = kcpt_ctx.ctx;
	struct cpt_inst_s inst;
	struct cpt_res_s *res;
	dma_addr_t cptr_iova;
	dma_addr_t dptr_iova;
	u32 tcp_payload_len;
	int ret;

	if (!th) {
		pr_err("%s: tcphdr is corrupted/not present, nothing to do\n", __func__);
		return 0;
	}

	tcp_payload_len = len - ((hdr->l4hdr - hdr->l2hdr) + __tcp_hdrlen(th));
	/* Pure ACK - do nothing */
	if (!tcp_payload_len)
		return 0;

	cptr_iova = ctx->iova +
		    conn_id * (ALIGN(sizeof(struct pan_cpt_tls_ctx_s), 128));
	/* For a TLS packet, plain text size = TCP payload - TLS header size -
	 *				       IV - Auth Tag
	 */
	tcp_payload_len = tcp_payload_len - 5 - 8 - 16;

	memset(&inst, 0, sizeof(struct cpt_inst_s));

	/* word-1 */
	/* TODO: single cpt response pointer is sufficient? */
	res = (struct cpt_res_s *)(rq->cpt_resp->base);
	res->compcode = 0;
	inst.res_addr = rq->cpt_resp->iova;

	/* word-3: Set QORD to force CPT_RES_S write completion */
	inst.qord = 1;

	/* word-4 */
	inst.dlen = tcp_payload_len;
	inst.opcode_major = CN10K_KTLS_MAJOR_OP;
	inst.param2 = 0x17; /* Application data */
	inst.param1 = tcp_payload_len;

	dptr_iova = otx2_dma_map_page(pf, virt_to_page(packet),
				      offset_in_page(packet),
				      len, DMA_BIDIRECTIONAL);

	dptr_iova += (hdr->l4hdr - hdr->l2hdr) + __tcp_hdrlen(th) + 5 + 8;

	inst.dptr = dptr_iova;

	/* word-6 */
	inst.rptr = inst.dptr - (8 + 5);

	/* word-7 */
	inst.cptr = cptr_iova;
	inst.ctx_val = 1;
	inst.egrp = CN10K_CPT_DFLT_ENG_GRP_SE;

	/* Finally Flush the CPT instruction */
	pan_cpt_inst_flush(pf, &inst, sizeof(struct cpt_inst_s), kcpt_ctx.io_addr);
	dmb(sy);
	ret = pan_cpt_wait_for_respose(res);
	if (ret)
		return -EINVAL;

	return 0;
}

static int pan_tls_add_conn(struct otx2_nic *pf, struct pan_fl_tbl_res *res)
{
	struct pan_ktls_ctx *ctx = &res->opq->ctx;
	struct pan_ktls_seg_ctx *seg;
	struct pan_cpt_tls_ctx_s *cptr;
	u8 conn = ctx->conn_id;
	int i, j = 0;
	int ret;

	cptr = cn10k_ktls_new_conn(pf, conn);
	if (!cptr)
		return -ENOSPC;

	seg = &pan_ktls.seg[conn];

	/* Allocate coherent DMA buffer used to assemble a full TLS record when
	 * it spans multiple TCP segments.  Sized for the worst-case TLS record.
	 */
	if (!seg->io_buf) {
		ret = qmem_alloc(pf->dev, &seg->io_buf, 1, PAN_KTLS_IOBUF_SIZE);
		if (ret) {
			dev_err(pf->dev, "KTLS: io_buf alloc failed for conn %u\n",
				conn);
			return ret;
		}
	}
	seg->state = PAN_KTLS_SEG_IDLE;
	seg->npkts = 0;
	seg->collected_len = 0;

	memset(cptr, 0, sizeof(*cptr));

	cptr->push_size = 28;
	cptr->hdr_size = 1;
	cptr->aop_valid = 1;
	cptr->ctx_size = 1;
	cptr->version = CPT_KTLS_CTX_VER_1_2;
	cptr->aes_key_len = CPT_KTLS_CTX_AES_KEYLEN_128;
	cptr->cipher = CPT_KTLS_CTX_CIPHER_AESGCM;
	cptr->mac_select = CPT_KTLS_CTX_MAC_SHA256;

	print_hex_dump(KERN_INFO, "IV: ", DUMP_PREFIX_ADDRESS, 16,
		       1, ctx->iv, TLS_CIPHER_AES_GCM_128_IV_SIZE,  false);

	print_hex_dump(KERN_INFO, "SALT: ", DUMP_PREFIX_ADDRESS, 16,
		       1, ctx->salt, TLS_CIPHER_AES_GCM_128_SALT_SIZE,  false);

	/* salt: 4 byte
	 * iv: 8 byte
	 * cptr->salt is combination of 4 byte slat and 4 byte iv.
	 * cptr->iv contains the other 4 byte of iv.
	 * All keys(salt+iv+keys) are stored in big-endian format.
	 */
	for (i = 0; i < TLS_CIPHER_AES_GCM_128_SALT_SIZE; i++, j++)
		cptr->salt[8 - 1 - j] = ctx->salt[i];
	for (i = 0; i < 4; i++, j++)
		cptr->salt[8 - 1 - j] = ctx->iv[i];
	j = 0;
	for (; i < 8; i++, j++)
		cptr->iv[8 - 1 - j] = ctx->iv[i];

	for (i = 0; i < 8; i++)
		cptr->key_w0[8 - 1 - i] = ctx->key[i];
	for (i = 0; i < 8; i++)
		cptr->key_w1[8 - 1 - i] = ctx->key[8 + i];

	memcpy((unsigned char *)&cptr->seq_no, ctx->rec_no,
	       sizeof(ctx->rec_no));

	return 0;
}

static int pan_ktls_conn_add(struct pan_ktls_add_conn_req *req)
{
	struct pan_tuple tuple1 = { .flags = PAN_TUPLE_FLAG_L3_PROTO_V4, };
	struct pan_tuple tuple = { .flags = PAN_TUPLE_FLAG_L3_PROTO_V4, };
	struct pan_ktls_npc_info *info;
	struct pan_ktls_mbox_ctx *ctx;
	struct pan_fl_tbl_res *res, res_obj = { 0 };
	struct pan_fl_tbl_opaque opq_obj = { 0 };
	int err;

	if (req->conn_id >= CN10K_CPT_MAX_KTLS_CTX)
		return -EINVAL;

	res = &res_obj;
	res->opq = &opq_obj;

	/* Required for MCAM entry */
	tuple.src_ip4.s_addr = req->src_ip;
	tuple.dst_ip4.s_addr = req->dst_ip;
	tuple.l3proto = htons(ETH_P_IP);
	tuple.sport   = req->src_port;
	tuple.dport   = req->dst_port;
	tuple.l4proto = req->l4_proto;
	pan_tuple_hash_set(&tuple, PAN_KTLS_ENC_MATCHID | req->conn_id);

	/* Populate res for flow table */
	memcpy(res->opq->ctx.key, req->key,
	       TLS_CIPHER_AES_GCM_128_KEY_SIZE);
	memcpy(res->opq->ctx.iv, req->iv,
	       TLS_CIPHER_AES_GCM_128_IV_SIZE);
	memcpy(res->opq->ctx.salt, req->salt,
	       TLS_CIPHER_AES_GCM_128_SALT_SIZE);
	memcpy(res->opq->ctx.rec_no, req->rec_no,
	       TLS_CIPHER_AES_GCM_128_REC_SEQ_SIZE);
	res->opq->ctx.conn_id = req->conn_id;

	res->act = PAN_FL_TBL_ACT_TLS_ENC;
	pan_tuple_hash_set(&tuple1, PAN_KTLS_ENC_MATCHID | req->conn_id);

	info = &pan_ktls.info[req->conn_id];
	ctx = &pan_ktls.ktls_ctx[req->conn_id];
	err = pan_fl_tbl_add(&tuple1, res, &info->ctx_handle);
	if (err)
		pr_info("Failed to install flow\n");

	/*get NPC MCAM entry to install flow */
	if (pan_rvu_alloc_mcam_entry(pan_nic, &info->mcam_entry))
		pr_info("Failed to allocate Mcam index\n");
	pan_rvu_install_flow(pan_nic, &tuple);
	pan_tls_add_conn(pan_nic, res);

	ctx->conn_id = req->conn_id;
	ctx->l4_proto = req->l4_proto;
	ctx->src_ip = req->src_ip;
	ctx->dst_ip = req->dst_ip;
	ctx->src_port = req->src_port;
	ctx->dst_port = req->dst_port;
	ctx->counter = req->counter;
	ctx->tcp_seq = req->tcp_seq;
	memcpy(ctx->key, req->key,
	       TLS_CIPHER_AES_GCM_128_KEY_SIZE);
	memcpy(ctx->iv, req->iv,
	       TLS_CIPHER_AES_GCM_128_IV_SIZE);
	memcpy(ctx->salt, req->salt,
	       TLS_CIPHER_AES_GCM_128_SALT_SIZE);
	memcpy(ctx->rec_no, req->rec_no,
	       TLS_CIPHER_AES_GCM_128_REC_SEQ_SIZE);
	pr_info("PAN offload rule added for conn_id:%d\n", req->conn_id);
	return 0;
}

static void pan_ktls_seg_reset(struct pan_ktls_seg_ctx *seg, u8 conn_id)
{
	struct pan_ktls_pkt_ref *ref;
	int i;

	/* Drop any packets buffered mid-record to avoid buffer leaks */
	if (seg->state == PAN_KTLS_SEG_COLLECTING && seg->npkts > 0) {
		pr_warn("KTLS: conn %u deleted mid-record; dropping %d buffered pkts\n",
			conn_id, seg->npkts);
		for (i = 0; i < seg->npkts; i++) {
			ref = &seg->pkts[i];
			pan_nic->hw_ops->aura_freeptr(pan_nic,
						      seg->cq->cq_idx,
						      ref->cqe.sg.seg_addr &
						      ~0x07ULL);
		}
	}

	if (seg->io_buf && pan_nic) {
		qmem_free(pan_nic->dev, seg->io_buf);
		seg->io_buf = NULL;
	}

	memset(seg, 0, sizeof(*seg));
}

static int pan_ktls_conn_del(struct pan_ktls_del_conn_req *req)
{
	struct pan_ktls_mbox_ctx *ctx;
	struct pan_ktls_npc_info *info;

	if (req->conn_id >= CN10K_CPT_MAX_KTLS_CTX)
		return -EINVAL;

	info = &pan_ktls.info[req->conn_id];
	ctx = &pan_ktls.ktls_ctx[req->conn_id];
	if (!info) {
		pr_err("No PAN/NPC info found for conn_id:%d\n", req->conn_id);
		return 0;
	}

	pan_rvu_delete_flow(info->mcam_entry);
	pan_rvu_free_mcam_entry(info->mcam_entry);
	if (info->ctx_handle)
		pan_fl_tbl_del_by_handle(info->ctx_handle);

	pan_ktls_seg_reset(&pan_ktls.seg[req->conn_id], req->conn_id);
	memset(ctx, 0, sizeof(*ctx));
	memset(info, 0, sizeof(*info));
	pr_info("PAN offload rule deleted for conn_id:%d\n", req->conn_id);
	return 0;
}

static void pan_ktls_conn_del_all(void)
{
	struct pan_ktls_npc_info *info;
	struct pan_ktls_mbox_ctx *ctx;
	int i;

	for (i = 0; i < PAN_KTLS_MAX_CONN; i++) {
		info = &pan_ktls.info[i];
		ctx = &pan_ktls.ktls_ctx[i];
		if (!info) {
			pr_debug("No PAN/NPC info found for conn_id:%d\n", i);
			continue;
		}

		pan_rvu_delete_flow(info->mcam_entry);
		pan_rvu_free_mcam_entry(info->mcam_entry);
		if (info->ctx_handle)
			pan_fl_tbl_del_by_handle(info->ctx_handle);

		pan_ktls_seg_reset(&pan_ktls.seg[i], i);
		memset(ctx, 0, sizeof(*ctx));
		memset(info, 0, sizeof(*info));
		pr_info("PAN offload rule deleted for conn_id:%d\n", i);
	}
}

static void pan_ktls_deinit(void)
{
	if (!pan_nic)
		return; /* Nothing to do */

	pan_ktls_conn_del_all();
	pan_ktls_tx_clean(pan_nic);
}

void pan_ktls_process_mbox(struct otx2_nic *pf, struct mbox_msghdr *msg)
{
	switch (msg->id) {
	case PAN_KTLS_ADD_CONN:
		pan_ktls_conn_add((struct pan_ktls_add_conn_req *)msg);
		break;
	case PAN_KTLS_DEL_CONN:
		pan_ktls_conn_del((struct pan_ktls_del_conn_req *)msg);
		break;
	default:
		dev_err(pf->dev, "message id:%d is not supported in PAN\n", msg->id);
		return;
	}
}

/*
 * Encrypt the full TLS record assembled in seg->io_buf using the NONSEG
 * CPT opcode.  The io_buf must be laid out as:
 *   [0 .. 4]                  TLS record header (5 B, used as AAD)
 *   [5 .. 12]                 explicit IV (8 B)
 *   [13 .. 13+plen-1]         plaintext (plen B)
 *   [13+plen .. 13+plen+15]   auth-tag space (16 B, zeroed, filled by CPT)
 *
 * On success the same region holds the encrypted output in-place.
 */
static int pan_ktls_encrypt_iobuf(struct otx2_nic *pf,
				  struct otx2_rcv_queue *rq,
				  struct pan_ktls_seg_ctx *seg, u8 conn_id)
{
	struct qmem *ctx = kcpt_ctx.ctx;
	dma_addr_t cptr_iova;
	struct cpt_inst_s inst;
	struct cpt_res_s *res;

	cptr_iova = ctx->iova +
		    conn_id * ALIGN(sizeof(struct pan_cpt_tls_ctx_s), 128);

	memset(&inst, 0, sizeof(struct cpt_inst_s));

	res = (struct cpt_res_s *)(rq->cpt_resp->base);
	res->compcode = 0;
	inst.res_addr = rq->cpt_resp->iova;

	inst.qord = 1;

	inst.dlen      = seg->tls_record_plen;
	inst.opcode_major = CN10K_KTLS_MAJOR_OP;
	inst.param2    = 0x17; /* application data */
	inst.param1    = seg->tls_record_plen;

	/* dptr → start of plaintext; rptr = dptr - 13 → start of TLS header */
	inst.dptr = seg->io_buf->iova +
		    TLS_RECORD_HDR_LEN + TLS_GCM_EXPLICIT_IV_LEN;
	inst.rptr = seg->io_buf->iova; /* = inst.dptr - (TLS_RECORD_HDR_LEN +
					*                TLS_GCM_EXPLICIT_IV_LEN) */

	inst.cptr    = cptr_iova;
	inst.ctx_val = 1;
	inst.egrp    = CN10K_CPT_DFLT_ENG_GRP_SE;

	pan_cpt_inst_flush(pf, &inst, sizeof(struct cpt_inst_s),
			   kcpt_ctx.io_addr);
	dmb(sy);
	return pan_cpt_wait_for_respose(res);
}

/*
 * After pan_ktls_encrypt_iobuf() has written ciphertext into seg->io_buf,
 * distribute the result back to the original per-segment packet buffers.
 *
 * io_buf layout after encryption:
 *   offset 0..4:           TLS header       (unchanged, skip)
 *   offset 5..12:          explicit IV      (8 B, CPT may update)
 *   offset 13..13+N-1:     ciphertext       (N = tls_record_plen bytes)
 *   offset 13+N..13+N+15:  auth tag         (16 B)
 *
 * For pkt[0]: copy io_buf[5 ..] → tcp_payload[5 ..] (IV + ciphertext start)
 * For pkt[i>0]: copy io_buf[..] → tcp_payload[0 ..] (ciphertext continuation)
 * The last packet's tcp_payload ends with the 16-byte auth tag.
 */
static void pan_ktls_copyback(struct pan_ktls_seg_ctx *seg)
{
	u32 io_offset = TLS_RECORD_HDR_LEN; /* skip TLS header in io_buf */
	struct pan_ktls_pkt_ref *ref;
	u8 *pkt_tls_start;
	u16 copy_len;
	int i;

	for (i = 0; i < seg->npkts; i++) {
		ref = &seg->pkts[i];
		pkt_tls_start = ref->va + ref->tcp_payload_off;

		if (i == 0) {
			/* Skip the TLS header already in the packet; copy IV +
			 * first portion of ciphertext.
			 */
			copy_len = ref->tcp_payload_len - TLS_RECORD_HDR_LEN;
			memcpy(pkt_tls_start + TLS_RECORD_HDR_LEN,
			       (u8 *)seg->io_buf->base + io_offset,
			       copy_len);
		} else {
			/* Continuation: copy ciphertext (+ auth tag for last) */
			copy_len = ref->tcp_payload_len;
			memcpy(pkt_tls_start,
			       (u8 *)seg->io_buf->base + io_offset,
			       copy_len);
		}
		io_offset += copy_len;
	}
}

int pan_tls_encrypt(struct otx2_nic *pf, struct pan_fl_tbl_res *res,
		    u8 conn_id, struct otx2_cq_queue *cq,
		    struct nix_cqe_rx_s *cqe, struct pan_tuple *tuple,
		    struct pan_tuple_hdr *hdr, int *len,
		    u16 xmit_pcifunc_off, u16 *data_off)
{
	struct pan_ktls_seg_ctx *seg = &pan_ktls.seg[conn_id];
	struct nix_rx_parse_s *parse = &cqe->parse;
	struct nix_rx_sg_s *rx_sg_s = &cqe->sg;
	bool current_pkt_saved = false;
	struct pan_ktls_pkt_ref *ref;
	struct otx2_rcv_queue *rq;
	u32 pkt_plain, remaining;
	u16 tls_len_field;
	u16 tcp_payload_off;
	u16 tcp_payload_len;
	int num_rx_desc, sz;
	struct tcphdr *th;
	u8 *tls_start;
	int num_sgs;
	bool is_last;
	int ret, i;
	u8 *va;

	rq = &pf->qset.rq[cq->cq_idx];
	if (!rq)
		return -EOPNOTSUPP;

	sz = (parse->desc_sizem1) ?
		((parse->desc_sizem1 + 1) * 16) :
		sizeof(struct nix_rx_sg_s);
	num_rx_desc = sz / sizeof(struct nix_rx_sg_s);

	/* Only single-buffer non-SG packets are supported */
	if (likely(num_rx_desc == 1) && rx_sg_s->segs == 1) {
		if (rx_sg_s->seg_size < sizeof(struct ethhdr)) {
			dev_err(pf->dev, "buffer size(%u) is less than eth hdr\n",
				rx_sg_s->seg_size);
			return -ENOBUFS;
		}
		va = (u8 *)phys_to_virt(otx2_iova_to_phys(pf->iommu_domain,
							   rx_sg_s->seg_addr));
		*len  = rx_sg_s->seg_size;
		num_sgs = 1;
	} else {
		return -EOPNOTSUPP;
	}

	pan_parse_buf(va, tuple, hdr);
	hdr->flags |= tuple->flags;

	th = (struct tcphdr *)(hdr->l4hdr);
	if (!th) {
		dev_err(pf->dev, "%s: TCP header not found\n", __func__);
		return -EINVAL;
	}

	tcp_payload_off = (hdr->l4hdr - hdr->l2hdr) + __tcp_hdrlen(th);
	tcp_payload_len = *len - tcp_payload_off;

	/* Pure ACK: no TLS payload, let caller forward as-is */
	if (!tcp_payload_len)
		return 0;

	tls_start = va + tcp_payload_off;

	switch (seg->state) {
	case PAN_KTLS_SEG_IDLE:
		if (tcp_payload_len < TLS_RECORD_HDR_LEN) {
			dev_err(pf->dev,
				"%s: TCP payload (%u B) shorter than TLS hdr\n",
				__func__, tcp_payload_len);
			return -EINVAL;
		}

		/* TLS record-length field = explicit-IV + ciphertext + auth-tag */
		tls_len_field = (tls_start[3] << 8) | tls_start[4];
		if (tls_len_field < TLS_GCM_EXPLICIT_IV_LEN + TLS_GCM_AUTH_TAG_LEN) {
			dev_err(pf->dev,
				"%s: invalid TLS record length %u\n",
				__func__, tls_len_field);
			return -EINVAL;
		}

		seg->tls_record_plen = tls_len_field -
				       TLS_GCM_EXPLICIT_IV_LEN -
				       TLS_GCM_AUTH_TAG_LEN;

		/* NONSEG: the full TLS record fits in this single packet */
		if (tcp_payload_len >= seg->tls_record_plen + TLS_GCM_OVERHEAD)
			return pan_ktls_submit(pf, rq, va, *len, hdr, conn_id);

		/* COLLECTING: first segment of a multi-segment TLS record */
		seg->collected_len = 0;
		seg->npkts         = 0;
		seg->cq            = cq;
		seg->state         = PAN_KTLS_SEG_COLLECTING;

		/* Copy TLS header + explicit IV into io_buf header area */
		memcpy(seg->io_buf->base, tls_start,
		       TLS_RECORD_HDR_LEN + TLS_GCM_EXPLICIT_IV_LEN);

		/* Kernel guarantees the first segment contains at least the
		 * TLS header and explicit IV (13 bytes), so no underflow here.
		 */
		pkt_plain = tcp_payload_len -
			    TLS_RECORD_HDR_LEN - TLS_GCM_EXPLICIT_IV_LEN;
		memcpy((u8 *)seg->io_buf->base +
		       TLS_RECORD_HDR_LEN + TLS_GCM_EXPLICIT_IV_LEN,
		       tls_start + TLS_RECORD_HDR_LEN + TLS_GCM_EXPLICIT_IV_LEN,
		       pkt_plain);
		seg->collected_len = pkt_plain;
		printk("%s: PAN_KTLS_SEG_IDLE pkt_plain:%d\n", __func__, pkt_plain);
		break; /* → save current pkt ref and return -EAGAIN */

	case PAN_KTLS_SEG_COLLECTING:
		remaining = seg->tls_record_plen - seg->collected_len;

		/* Last segment when remaining plaintext + auth-tag exactly fit */
		is_last = (tcp_payload_len >= remaining + TLS_GCM_AUTH_TAG_LEN);
		pkt_plain = is_last ? remaining : tcp_payload_len;

		memcpy((u8 *)seg->io_buf->base +
		       TLS_RECORD_HDR_LEN + TLS_GCM_EXPLICIT_IV_LEN +
		       seg->collected_len,
		       tls_start, pkt_plain);
		seg->collected_len += pkt_plain;

		if (!is_last)
			break; /* → save current pkt ref and return -EAGAIN */

		/* FULL: all segments collected.  Save current (last) pkt ref,
		 * encrypt the assembled record, copy ciphertext back to every
		 * original buffer, then transmit.
		 */
		if (seg->npkts >= PAN_KTLS_MAX_SEG_PKTS) {
			dev_err(pf->dev,
				"%s: too many segments for conn %u\n",
				__func__, conn_id);
			goto seg_error;
		}

		ref = &seg->pkts[seg->npkts];
		memcpy(&ref->cqe, cqe, sizeof(ref->cqe));
		ref->hdr              = *hdr;
		memcpy(ref->data_off, data_off, sizeof(ref->data_off));
		ref->xmit_pcifunc_off = xmit_pcifunc_off;
		ref->num_sgs          = num_sgs;
		ref->len              = *len;
		ref->va               = va;
		ref->tcp_payload_off  = tcp_payload_off;
		ref->tcp_payload_len  = tcp_payload_len;
		seg->npkts++;
		current_pkt_saved = true;

		ret = pan_ktls_encrypt_iobuf(pf, rq, seg, conn_id);
		if (ret) {
			dev_err(pf->dev,
				"%s: CPT encryption failed (%d)\n",
				__func__, ret);
			goto seg_error;
		}

		/* Write ciphertext + auth-tag back to original packet buffers */
		pan_ktls_copyback(seg);

		/* Transmit all buffered packets except the last one; the outer
		 * caller (pan_rvu_process_buf) transmits the last packet via the
		 * live CQE it already holds.
		 */
		for (i = 0; i < seg->npkts - 1; i++) {
			ref = &seg->pkts[i];
			pan_rvu_ktls_buf_xmit(res, seg->cq, &ref->cqe,
					      ref->num_sgs, ref->len,
					      &ref->hdr, pf,
					      ref->xmit_pcifunc_off,
					      ref->data_off);
		}

		seg->state         = PAN_KTLS_SEG_IDLE;
		seg->npkts         = 0;
		seg->collected_len = 0;
		printk("%s: PAN_KTLS_SEG_COLLECTING pkt_plain:%d\n", __func__, pkt_plain);
		return 0; /* caller transmits the last (current) packet */
	}

	/* --- common path: buffer the current packet and wait for more --- */
	if (seg->npkts >= PAN_KTLS_MAX_SEG_PKTS) {
		dev_err(pf->dev,
			"%s: too many segments for conn %u\n",
			__func__, conn_id);
		goto seg_error;
	}

	ref = &seg->pkts[seg->npkts];
	memcpy(&ref->cqe, cqe, sizeof(ref->cqe));
	ref->hdr              = *hdr;
	memcpy(ref->data_off, data_off, sizeof(ref->data_off));
	ref->xmit_pcifunc_off = xmit_pcifunc_off;
	ref->num_sgs          = num_sgs;
	ref->len              = *len;
	ref->va               = va;
	ref->tcp_payload_off  = tcp_payload_off;
	ref->tcp_payload_len  = tcp_payload_len;
	seg->npkts++;
	return -EAGAIN;

seg_error:
	/*
	 * Drop all BUFFERED packets by returning their buffers to the NPA pool.
	 * If the current packet was already added to pkts[] (current_pkt_saved),
	 * it occupies pkts[npkts-1]; skip it so the caller's live CQE still
	 * points to a valid buffer and can safely re-inject it to the stack.
	 * If it was not yet saved the loop covers all of pkts[0..npkts-1] and
	 * the current packet is left for the caller to handle.
	 */
	{
		int free_count = current_pkt_saved ? seg->npkts - 1 : seg->npkts;

		for (i = 0; i < free_count; i++) {
			ref = &seg->pkts[i];
			pf->hw_ops->aura_freeptr(pf, cq->cq_idx,
						 ref->cqe.sg.seg_addr &
						 ~0x07ULL);
		}
	}
	seg->state         = PAN_KTLS_SEG_IDLE;
	seg->npkts         = 0;
	seg->collected_len = 0;
	return -EINVAL;
}

void pan_ktls_hw_exit(void)
{
	pan_ktls_deinit();
}

int pan_ktls_init(void)
{
	pan_nic = pan_rvu_get_pan_nic();
	return 0;
}

int pan_ktls_hw_init(struct otx2_nic *pf)
{
	struct net_device *netdev = pf->netdev;
	int ret = 0;

	if (is_dev_otx2(pf->pdev))
		return 0;

	ret = pan_mbox_host_init(pf);
	if (ret) {
		dev_err(pf->dev, "PAN Host mbox init failed\n");
		return ret;
	}

	/* Request AF to attach CPT LF for KTLS */
	ret = pan_cpt_attach(pf);
	if (ret) {
		dev_err(pf->dev, "Failed to attach CPT LF for KTLS offload\n");
		return ret;
	}

	ret = pan_mbox_register_host_intr(pf, true);
	if (ret) {
		dev_err(pf->dev, "Register mbox intr failed\n");
		return ret;
	}

	pan_ktls_tx_init(pf);

	netdev->hw_features |= NETIF_F_HW_TLS_TX;
	netdev->features |= NETIF_F_HW_TLS_TX;

	return ret;
}
