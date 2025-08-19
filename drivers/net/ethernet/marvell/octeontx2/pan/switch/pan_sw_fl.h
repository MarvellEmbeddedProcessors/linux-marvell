/* SPDX-License-Identifier: GPL-2.0 */
/* Marvell PAN driver
 *
 * Copyright (C) 2025 Marvell.
 *
 */

#ifndef PAN_SW_FL_H_
#define PAN_SW_FL_H_
void pan_sw_fl_deinit(void);
int pan_sw_fl_init(void);

int pan_sw_fl_ev_enq(struct otx2_nic *pf, u16 switch_id, u32 port_id,
		     struct fl_tuple *ftuple, u64 flags, unsigned long cookie);

#endif //PAN_SW_FL_H
