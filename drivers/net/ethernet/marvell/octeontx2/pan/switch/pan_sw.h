/* SPDX-License-Identifier: GPL-2.0 */
/* Marvell PAN driver
 *
 * Copyright (C) 2025 Marvell.
 *
 */

#ifndef PAN_SW_H_
#define PAN_SW_H_

void pan_sw_deinit(void);
int pan_sw_init(void);

int otx2_mbox_up_handler_af2swdev_notify(struct otx2_nic *pf,
					 struct af2swdev_notify_req *req,
					 struct msg_rsp *rsp);

u16 pan_sw_get_pcifunc(unsigned int port_id);

#endif //PAN_SWITCH_H_
