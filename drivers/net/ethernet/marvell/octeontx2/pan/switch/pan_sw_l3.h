/* SPDX-License-Identifier: GPL-2.0 */
/* Marvell PAN driver
 *
 * Copyright (C) 2025 Marvell.
 *
 */

#ifndef PAN_SW_L3_H_
#define PAN_SW_L3_H_

struct pan_sw_l3_offl_node {
	unsigned int port_id;
	struct otx2_nic *pf;
	unsigned long jiffies;
	u16 mcam_idx;
	u16 match_id;
	u64 tuple_installed :1;
	struct pan_tuple tuple;
	struct fib_entry *entry;
};

void pan_sw_l3_deinit(void);
int pan_sw_l3_init(void);

struct net_device *
pan_sw_l3_route_lookup(u32 dst);
int pan_sw_l3_ev_enq(struct otx2_nic *otx2_nic, int cnt, struct fib_entry *fe);

#endif //PAN_SWITCH_H_
