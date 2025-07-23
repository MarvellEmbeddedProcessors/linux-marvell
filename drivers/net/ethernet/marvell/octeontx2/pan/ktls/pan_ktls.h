/* SPDX-License-Identifier: GPL-2.0 */
#ifndef __PAN_KTLS_H__
#define __PAN_KTLS_H__

#if IS_ENABLED(CONFIG_OCTEONTX_PAN_KTLS_TX)

#include <linux/types.h>
#include <linux/tls.h>
#include <net/tls.h>
#include "../pan_tuple.h"
#include "../../nic/cn10k_ipsec.h"
#include "../pan_cpt.h"

#define CN10K_KTLS_MAJOR_OP	0x16ULL
#define CN10K_CPT_MAX_KTLS_CTX	8

#define PAN_KTLS_DEC_MATCHID 0xff00
#define PAN_KTLS_ENC_MATCHID 0xfe00

#define PAN_KTLS_MAX_CONN	8

struct otx2_nic;
struct mbox_msghdr;
struct nix_cqe_rx_s;
struct otx2_cq_queue;
struct pan_fl_tbl_res;

struct pan_ktls_ctx {
	u8 conn_id;
	unsigned char key[TLS_CIPHER_AES_GCM_128_KEY_SIZE];
	unsigned char salt[TLS_CIPHER_AES_GCM_128_SALT_SIZE];
	unsigned char iv[TLS_CIPHER_AES_GCM_128_IV_SIZE];
	unsigned char rec_no[TLS_CIPHER_AES_GCM_128_REC_SEQ_SIZE];
};

struct pan_ktls_add_conn_req {
	struct mbox_msghdr hdr;
	u8 conn_id;
	u8 l4_proto;
	u32 src_ip;
	u32 dst_ip;
	u16 src_port;
	u16 dst_port;
	u32 counter;
	u32 tcp_seq;
	unsigned char key[16];
	unsigned char salt[4];
	unsigned char iv[8];
	unsigned char rec_no[8];
};

struct pan_ktls_del_conn_req {
	struct mbox_msghdr hdr;
	u8 conn_id;
};

struct pan_ktls_conn_req {
	u16 msg_id;
	struct pan_ktls_add_conn_req *add_req;
	struct pan_ktls_del_conn_req *del_req;
};

struct pan_ktls_npc_info {
	u64 ctx_handle;
	u16 mcam_entry;
};

/* ktls context */
struct pan_ktls_mbox_ctx {
	__u8 rsvd_front[64];
	__u8 conn_id;
	__u8 l4_proto;
	__u32 src_ip;
	__u32 dst_ip;
	__u16 src_port;
	__u16 dst_port;
	__u32 counter;
	__u32 tcp_seq;
	unsigned char key[TLS_CIPHER_AES_GCM_128_KEY_SIZE];
	unsigned char salt[TLS_CIPHER_AES_GCM_128_SALT_SIZE];
	unsigned char iv[TLS_CIPHER_AES_GCM_128_IV_SIZE];
	unsigned char rec_no[TLS_CIPHER_AES_GCM_128_REC_SEQ_SIZE];
	__u8 rsvd_back[64];
};

struct pan_ktls_dev {
	struct pan_ktls_npc_info info[PAN_KTLS_MAX_CONN];
	struct pan_ktls_mbox_ctx ktls_ctx[PAN_KTLS_MAX_CONN];
	struct workqueue_struct *work;
};

int pan_ktls_init(void);
void pan_ktls_process_mbox(struct otx2_nic *pf, struct mbox_msghdr *msg);
int pan_ktls_hw_init(struct otx2_nic *oct);
void pan_ktls_hw_exit(void);
int pan_tls_encrypt(struct otx2_nic *pf, struct pan_fl_tbl_res *res,
		    u8 conn_id, struct otx2_cq_queue *cq, struct nix_cqe_rx_s *cqe,
		    struct pan_tuple *tuple, struct pan_tuple_hdr *hdr, int *len);
#endif
#endif /* __PAN_H__ */
