/* SPDX-License-Identifier: GPL-2.0 */
/* Marvell RVU PAN driver
 *
 * Copyright (C) 2025 Marvell.
 *
 */
#ifndef PAN_TL_H_
#define PAN_TL_H_

int pan_tl_set_links(bool set_sdp);
void pan_tl_txschq_free_one(struct otx2_nic *pfvf, u16 lvl, u16 schq);
int pan_tl_txschq_rsrcs(struct otx2_nic *pf);
int pan_tl_init(void);
void pan_tl_deinit(void);

#endif // End of PAN_TL_H_
