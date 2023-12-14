/* SPDX-License-Identifier: GPL-2.0 */
/* Marvell PAN driver
 *
 * Copyright (C) 2025 Marvell.
 *
 */
int pan_test_init(void);
void pan_test_deinit(void);

int pan_test_rx_sock(struct socket *sock, struct sockaddr_in *addr,
		     unsigned char *buf, int len);
