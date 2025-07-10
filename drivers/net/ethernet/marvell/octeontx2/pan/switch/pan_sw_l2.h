/* SPDX-License-Identifier: GPL-2.0 */
/* Marvell PAN driver
 *
 * Copyright (C) 2025 Marvell.
 *
 */

#ifndef PAN_SW_L2_H_
#define PAN_SW_L2_H_

enum l2_offl_state {
	SWITCH_L2_OFFL_STATE_INVALID,
	SWITCH_L2_OFFL_STATE_INIT,
	SWITCH_L2_OFFL_STATE_DONE,
	SWITCH_L2_OFFL_STATE_FAIL,
	SWITCH_L2_OFFL_STATE_DEINIT,
	SWITCH_L2_OFFL_STATE_MAX,
};

struct pan_sw_l2_offl_node {
	struct list_head list;
	unsigned int port_id;
	u8 mac[ETH_ALEN];
	enum l2_offl_state state;
	u16 mcam_idx;
	u16 match_id;
	unsigned long jiffies;
	struct pan_tuple tuple;
	unsigned long long hits;
	struct otx2_nic *pf;
	struct hlist_node hnode;
};

void pan_sw_l2_deinit(void);
int pan_sw_l2_init(void);

int pan_sw_l2_de_offl(struct otx2_nic *pf, u32 switch_id,
		      unsigned int port_id, u8 *mac);

int pan_sw_l2_offl(struct otx2_nic *pf, u32 switch_id,
		   unsigned int port_id, u8 *mac);

struct pan_sw_l2_offl_node *pan_sw_l2_mac_tbl_lookup(const u8 *mac);
struct pan_sw_l2_offl_node *__pan_sw_l2_mac_tbl_lookup(const u8 *mac);
#endif //PAN_SWITCH_H_
