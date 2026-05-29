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

/* TLS record framing constants (RFC 5246 / RFC 5288 AES-GCM) */
#define TLS_RECORD_HDR_LEN	5
#define TLS_GCM_EXPLICIT_IV_LEN	8
#define TLS_GCM_AUTH_TAG_LEN	16
#define TLS_GCM_OVERHEAD	(TLS_RECORD_HDR_LEN + TLS_GCM_EXPLICIT_IV_LEN + \
				 TLS_GCM_AUTH_TAG_LEN)

/*
 * io_buf size: one full TLS plaintext record (up to TLS_MAX_PAYLOAD_SIZE) plus
 * TLS header, explicit IV and auth-tag overhead.
 */
#define PAN_KTLS_IOBUF_SIZE	((size_t)(TLS_MAX_PAYLOAD_SIZE + TLS_GCM_OVERHEAD))

/*
 * Upper bound on TCP segments that can carry a single TLS record.
 * Max TLS payload 16 KiB / min MSS ~1460 B ≈ 12; use 64 as a safe margin.
 */
#define PAN_KTLS_MAX_SEG_PKTS	64

struct otx2_nic;
struct mbox_msghdr;
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

/*
 * Per-packet metadata saved while buffering segments of a multi-segment
 * TLS record.  The nix_cqe_rx_s is copied verbatim so that the saved
 * segment addresses remain valid after the live CQE slot is recycled.
 */
struct pan_ktls_pkt_ref {
	struct nix_cqe_rx_s  cqe;		/* full copy of RX CQE */
	struct pan_tuple_hdr hdr;		/* L2/L3/L4 header pointers */
	u16		     data_off[3];	/* per-segment data offset array */
	u16		     xmit_pcifunc_off;	/* TX SQ selection offset */
	int		     num_sgs;		/* RX scatter-gather count */
	int		     len;		/* total frame length */
	u8		    *va;		/* packet buffer virtual address */
	u16		     tcp_payload_off;	/* offset to TCP payload in frame */
	u16		     tcp_payload_len;	/* TCP payload byte count */
};

enum pan_ktls_seg_state {
	PAN_KTLS_SEG_IDLE = 0,
	PAN_KTLS_SEG_COLLECTING,
};

/*
 * Per-connection state for buffering a TLS record that spans multiple TCP
 * segments.  One instance per conn_id, zero-initialised on connection setup.
 */
struct pan_ktls_seg_ctx {
	enum pan_ktls_seg_state  state;
	u32			 tls_record_plen;  /* expected plaintext length */
	u32			 collected_len;	   /* plaintext bytes in io_buf */
	int			 npkts;		   /* saved packet count */
	struct qmem		*io_buf;	   /* DMA plaintext/ciphertext buf */
	struct otx2_cq_queue	*cq;		   /* RX CQ common to all segments */
	struct pan_ktls_pkt_ref	 pkts[PAN_KTLS_MAX_SEG_PKTS];
};

struct pan_ktls_dev {
	struct pan_ktls_npc_info info[PAN_KTLS_MAX_CONN];
	struct pan_ktls_mbox_ctx ktls_ctx[PAN_KTLS_MAX_CONN];
	struct workqueue_struct *work;
	struct pan_ktls_seg_ctx  seg[PAN_KTLS_MAX_CONN];
};

int pan_ktls_init(void);
void pan_ktls_process_mbox(struct otx2_nic *pf, struct mbox_msghdr *msg);
int pan_ktls_hw_init(struct otx2_nic *oct);
void pan_ktls_hw_exit(void);
int pan_tls_encrypt(struct otx2_nic *pf, struct pan_fl_tbl_res *res,
		    u8 conn_id, struct otx2_cq_queue *cq,
		    struct nix_cqe_rx_s *cqe, struct pan_tuple *tuple,
		    struct pan_tuple_hdr *hdr, int *len,
		    u16 xmit_pcifunc_off, u16 *data_off);
#endif
#endif /* __PAN_H__ */
