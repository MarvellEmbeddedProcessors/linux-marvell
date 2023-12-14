/* SPDX-License-Identifier: GPL-2.0 */
/* Marvell PAN driver
 *
 * Copyright (C) 2025 Marvell.
 *
 */
#ifndef PAN_RVU_H_
#define PAN_RVU_H_

#define PCI_DEVID_PAN_RVU	0xA0E2
#define PAN_DEV_NAME		"pan"

/* Subsystem Device ID */
#define PCI_SUBSYS_DEVID_98XX                  0xB100
#define PCI_SUBSYS_DEVID_96XX                  0xB200

#define MAX_TX_IFACE 16

struct pan_rvu_sq_info {
	u16 sqidx;	/* SQ index */
	u16 pcifunc;
	u16 tx_chan;
	u64 is_sdp :1;
};

struct matchid_bmap {
	unsigned long *bmap;
	u32  max;
};

struct pan_rvu_gbl_t {
	u16 sqs_total;
	u16 sqs_usable;
	u16 sqs_per_core;
	struct xarray pcifunc2sqoff;
	u16 sdp_cnt;
	u16 sqoff2pcifunc[256 + 32];
	struct matchid_bmap rsrc;
	struct xarray chan2pfunc;
};

struct pan_rvu_cq_info {
	void			*dev;
	u64			sq_bitmask;
	u16			*sq2cqidxs;	/* SQ to CQ map */
	u16			rq2cqidx;	/* RQ to CQ map */
	u8			cint_idx;	/* num_online_cpus() */
	u8			sq_cnt;	/* number of entries in sq_idxs */
	u64			bh_cnt; /* Bottom half execution count */
	struct pan_rvu_sq_info  *sq_info;
};

struct pan_rvu_dev_priv {
	u16 pcifunc;
	struct otx2_nic *otx2_nic;
	struct pan_rvu_cq_info cq_info[NR_CPUS];
};

struct pan_rvu_gbl_t *pan_rvu_get_gbl(void);

void pan_rvu_deinit(void);
int pan_rvu_init(void);
int pan_rvu_get_sq_offset(struct otx2_nic *otx2_nic, u16 pcifunc);
typedef int (*pan_rvu_sg_cb)(u64 addr, u16 size, bool is_last, int idx, void *data);
struct net_device *pan_rvu_get_kernel_netdev_by_pcifunc(u16 pcifunc);
struct iface_info;
int pan_rvu_get_iface_info(struct iface_info *info, int *cnt, bool add_pan);
struct otx2_nic *pan_rvu_get_otx2_nic(struct net_device *dev);
int pan_rvu_pcifunc2_sq_off(u16 pcifunc);

// NPC related functions
int pan_rvu_alloc_mcam_entry(void);
int pan_rvu_install_flow(struct pan_tuple *tuple);
int pan_alloc_matchid(struct matchid_bmap *rsrc);
void pan_free_matchid(struct matchid_bmap *rsrc, int id);

#endif // PAN_RVU_H_
