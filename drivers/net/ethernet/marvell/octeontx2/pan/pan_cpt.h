/* SPDX-License-Identifier: GPL-2.0 */
#ifndef __PAN_CPT_H__
#define __PAN_CPT_H__

/* Default engine groups */
#define CN10K_CPT_DFLT_ENG_GRP_SE	0ULL
#define CN10K_CPT_DFLT_ENG_GRP_SE_IE	1ULL
#define CN10K_CPT_DFLT_ENG_GRP_AE	2ULL
#define CN10K_CPT_MAX_KTLS_CTX		8

int pan_cpt_alloc(struct otx2_nic *pfvf, u16 qidx);
int pan_cpt_attach(struct otx2_nic *pfvf);
void pan_cpt_inst_flush(struct otx2_nic *pf,
			struct cpt_inst_s *inst,
			u64 size, u64 io_addr);
int pan_cpt_wait_for_respose(struct cpt_res_s *res);
int pan_cpt_tx_lf_alloc(struct otx2_nic *pf);
int pan_cpt_tx_iq_alloc(struct otx2_nic *pf,
			struct cn10k_cpt_inst_queue *iq);
void pan_cpt_tx_iq_free(struct otx2_nic *pf,
			struct cn10k_cpt_inst_queue *iq);

struct pan_cpt_tls_ctx_s {
	u64 rsvd_15_0	: 16; /* W0 */
	u64 rsvd_47_16	: 32;
	u64 push_size	: 7;
	u64 rsvd_55	: 1;
	u64 hdr_size	: 2;
	u64 aop_valid	: 1;
	u64 rsvd_59	: 1;
	u64 ctx_size	: 4;
	u64 rsvd_63_0	: 64; /* W1 */
#define CPT_KTLS_CTX_VER_1_2		0x1ULL
	u64 version	: 4; /* W2 */
#define CPT_KTLS_CTX_AES_KEYLEN_128	0x1ULL
#define CPT_KTLS_CTX_AES_KEYLEN_256	0x3ULL
	u64 aes_key_len	: 2;
#define CPT_KTLS_CTX_CIPHER_3DES	0x1ULL
#define CPT_KTLS_CTX_CIPHER_AESCBC	0x3ULL
#define CPT_KTLS_CTX_CIPHER_AESGCM	0x7ULL
	u64 cipher	: 4;
#define CPT_KTLS_CTX_MAC_SHA1		0x2ULL
#define CPT_KTLS_CTX_MAC_SHA256		0x4ULL
	u64 mac_select	: 4;
	u64 iv_in_cptr	: 1;
	u64 rsvd_63_15	: 49;
	u64 rsvd_w3	: 64; /* W3 */
	u8 key_w0[8]; /* W4 */
	u8 key_w1[8]; /* W5 */
	u64 key_w2; /* W6 */
	u64 key_w3; /* W7 */
	u8 salt[8];
	u8 iv[8]; /* W8, W9 */
	u64 opad_w0	: 64; /* W10 */
	u64 opad_w1	: 64; /* W11 */
	u64 opad_w2	: 64; /* W12 */
	u64 opad_w3	: 64; /* W13 */
	u64 opad_w4	: 64; /* W14 */
	u64 opad_w5	: 64; /* W15 */
	u64 opad_w6	: 64; /* W16 */
	u64 opad_w7	: 64; /* W17 */
	u64 ipad_w0	: 64; /* W18 */
	u64 ipad_w1	: 64; /* W19 */
	u64 ipad_w2	: 64; /* W20 */
	u64 ipad_w3	: 64; /* W21 */
	u64 ipad_w4	: 64; /* W22 */
	u64 ipad_w5	: 64; /* W23 */
	u64 ipad_w6	: 64; /* W24 */
	u64 ipad_w7	: 64; /* W25 */
	u64 rsvd_w26	: 64; /* W26 */
	u64 seq_no; /* W27 */
};

#endif /* __PAN_CPT_H__ */
