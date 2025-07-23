/* SPDX-License-Identifier: GPL-2.0 */
#ifndef __PAN_MBOX_H__
#define __PAN_MBOX_H__

/* KTLS mbox between PAN and Host */
#define PAN_KTLS_ADD_CONN	0x7F80
#define PAN_KTLS_DEL_CONN	0x7F81

int pan_mbox_host_init(struct otx2_nic *pf);
void pan_mbox_host_destroy(void);
int pan_mbox_register_host_intr(struct otx2_nic *pf, bool probe_af);
void pan_mbox_disable_host_intr(struct otx2_nic *pf);

#endif /* __PAN_MBOX_H__ */
