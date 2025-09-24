/* SPDX-License-Identifier: GPL-2.0 */
/* Marvell PAN driver
 *
 * Copyright (C) 2025 Marvell.
 *
 */
#ifndef PAN_STATS_H_
#define PAN_STATS_H_

enum pan_stats_fld {
	PAN_STATS_FLD_IN_NON_SG_PKTS,
	PAN_STATS_FLD_IN_SG_PKTS,
	PAN_STATS_FLD_OUT_NON_SG_PKTS,
	PAN_STATS_FLD_OUT_SG_PKTS,
	PAN_STATS_FLD_DROP_PKTS,
	PAN_STATS_FLD_INTR,
	PAN_STATS_FLD_TX_DESC,
	PAN_STATS_FLD_SQE_THRESH,
	PAN_STATS_FLD_RX_CQ_PKTS,
	PAN_STATS_FLD_TX_CQ_PKTS,
	PAN_STATS_FLD_INVAL_SQ,
	PAN_STATS_FLD_EXP_PKTS,
	PAN_STATS_FLD_NAT_PKTS,
	PAN_STATS_FLD_NAPT_PKTS,
	PAN_STATS_FLD_ROUTE_PKTS,
	PAN_STATS_FLD_BR_PKTS,
	PAN_STATS_FLD_IN_VLAN_ROUTE_PKTS,
	PAN_STATS_FLD_OUT_VLAN_ROUTE_PKTS,
	PAN_STATS_FLD_MAX,
};

struct pan_stats {
	u64 fld[PAN_STATS_FLD_MAX];
};

enum pan_stats_exp_fld {
	PAN_STAT_EXP_FLD_NO_SIP_NEIGH,
	PAN_STAT_EXP_FLD_NO_DIP_NEIGH,
	PAN_STAT_EXP_FLD_NO_DEV,
	PAN_STAT_EXP_FLD_NO_IN_L2,
	PAN_STAT_EXP_FLD_NO_OUT_L2,
	PAN_STAT_EXP_FLD_INVAL_ACT,
	PAN_STATS_EXP_FLD_MAX,
};

struct pan_stats_exp {
	atomic_t fld[PAN_STATS_EXP_FLD_MAX];
};

void pan_stats_inc(enum pan_stats_fld fld);
void pan_stats_add(enum pan_stats_fld fld, u32 cnt);
u64 pan_stats_get(enum pan_stats_fld fld, int cpu);
int pan_stats_init(void);
void pan_stats_deinit(void);
void pan_stats_exp_inc(enum pan_stats_exp_fld fld);

#endif // End of PAN_STATS_H_
