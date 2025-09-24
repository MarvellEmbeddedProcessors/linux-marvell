/* SPDX-License-Identifier: GPL-2.0 */
/* Marvell PAN driver
 *
 * Copyright (C) 2025 Marvell.
 *
 */
#ifndef PAN_FL_TBL_H_
#define PAN_FL_TBL_H_

#include "pan_cmn.h"

enum pan_fl_tbl_type {
	PAN_FL_TBL_TYPE_IPV4,
	PAN_FL_TBL_TYPE_IPV6,
	PAN_FL_TBL_TYPE_MAX,
};

typedef int (*tbl_node_cb)(struct pan_tuple *tuple, struct sk_buff *skb,
			   void *arg);

struct pan_fl_tbl_opaque {
	/* all other fields should be inside this */
	union {
		struct pan_ktls_ctx ctx;
	};

	u64 eg_dmac_is_set : 1;
	u64 eg_dmac_can_set : 1;
	u64 eg_dip_is_set : 1;
	u8 eg_smac[ETH_ALEN];
	u8 eg_dmac[ETH_ALEN];
	u16 vlan_tag;
	u32 eg_sip;
	u32 eg_dip;
	u16 eg_sport;
	u16 eg_dport;
	struct rcu_head rcu;
};

enum pan_fl_tbl_act {
	PAN_FL_TBL_ACT_TLS_ENC = BIT_ULL(0),
	PAN_FL_TBL_ACT_TLS_DEC = BIT_ULL(1),
	PAN_FL_TBL_ACT_L2_FWD = BIT_ULL(2),
	PAN_FL_TBL_ACT_L3_FWD = BIT_ULL(3),
	PAN_FL_TBL_ACT_L3_BR_FWD = BIT_ULL(4),
	PAN_FL_TBL_ACT_EXP = BIT_ULL(5),
	PAN_FL_TBL_ACT_L3_SNAT = BIT_ULL(6),
	PAN_FL_TBL_ACT_L3_DNAT = BIT_ULL(7),
	PAN_FL_TBL_ACT_L3_BR_SNAT = BIT_ULL(8),
	PAN_FL_TBL_ACT_L3_BR_DNAT = BIT_ULL(9),
	PAN_FL_TBL_ACT_L3_VLAN_FWD = BIT_ULL(10),
	PAN_FL_TBL_ACT_L3_BR_VLAN_FWD = BIT_ULL(11),
	PAN_FL_TBL_ACT_L3_SNAPT = BIT_ULL(12),
	PAN_FL_TBL_ACT_L3_DNAPT = BIT_ULL(13),
	PAN_FL_TBL_ACT_L3_BR_SNAPT = BIT_ULL(14),
	PAN_FL_TBL_ACT_L3_BR_DNAPT = BIT_ULL(15),
	PAN_FL_TBL_ACT_MAX,
};

struct pan_fl_tbl_res {
	enum pan_fl_tbl_act act;
	u16 pcifuncoff;
	u64 dir : 1;
	u64 uni_di : 1;
	struct pan_fl_tbl_res *pair;
	struct pan_fl_tbl_opaque *opq;
	/* Don't add any fields to this structure.
	 * This is to keep table entry size optimal.
	 * Add new fields to pan_fl_tbl_opaque structure.
	 */
};

struct pan_fl_tbl_node {
	struct rhash_head rh_node;
	struct pan_tuple tuple;
	struct rcu_head rcu;
	u64 __percpu *hits;
	tbl_node_cb ig_cb, eg_cb;
	void *arg;
	struct pan_fl_tbl_res res;
};

struct pan_fl_tbl_rdx_node {
	unsigned long index;
	struct rhashtable ht;
	struct rhashtable_params rht_params;
	u64 flag;
	enum pan_fl_tbl_type type;
	spinlock_t lock;	/* Lock for add/del from table */
};

enum pan_fl_tbl_type pan_fl_tbl_find_type(struct pan_tuple *tuple);
struct pan_fl_tbl_rdx_node *
pan_fl_tbl_rdx_node_get(enum pan_fl_tbl_type type);

int pan_fl_tbl_lookup(struct pan_tuple *tuple);
void pan_fl_tbl_deinit(void);
int pan_fl_tbl_init(void);
int pan_fl_tbl_del(struct pan_tuple *tuple);
int pan_fl_tbl_offl_del(struct pan_tuple *tuple);
int pan_fl_tbl_del_by_handle(u64 handle);

int pan_fl_tbl_add(struct pan_tuple *tuple, struct pan_fl_tbl_res *res, u64 *handle);
int pan_fl_tbl_offl_add(struct pan_tuple *tuple, struct pan_fl_tbl_res *res);
int pan_fl_tbl_register_eg_cb(struct pan_tuple *tuple, tbl_node_cb cb,
			      void *arg);
int pan_fl_tbl_register_ig_cb(struct pan_tuple *tuple, tbl_node_cb cb,
			      void *arg);

int pan_fl_tbl_lookup_and_cb(struct pan_tuple *tuple, struct sk_buff *skb);

int __pan_fl_tbl_lookup_n_res(struct pan_tuple *tuple, struct pan_fl_tbl_res **res);
int __pan_fl_tbl_offl_lookup_n_res(struct pan_tuple *tuple, struct pan_fl_tbl_res **res);
int __pan_fl_tbl_offl_get_hit_cnt(struct pan_tuple *tuple, u64 *hits);

#endif // PAN_FL_TBL_H_
