/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Driver extension for Marvell User-space SDK.
 *
 * Copyright (C) 2024 Marvell
 *
 * Wilson Ding <dingwei@marvell.com>
 */

#ifndef _MVPP2_MUSDK_H_
#define _MVPP2_MUSDK_H_

#include <linux/platform_device.h>
#include "mvpp2.h"

#define IS_MUSDK_PORT(port)	((port)->flags & MVPP22_F_IF_MUSDK)

typedef int (*mvpp2_musdk_port_cb_t)(struct mvpp2_port *port);

size_t mvpp2_musdk_port_priv_size(void);
void mvpp2_musdk_port_init(struct mvpp2_port *port);
int mvpp2_musdk_port_enable(struct mvpp2_port *port, mvpp2_musdk_port_cb_t cb);
int mvpp2_musdk_port_disable(struct mvpp2_port *port, mvpp2_musdk_port_cb_t cb);

int mvpp2_musdk_uio_init(struct platform_device *pdev, struct mvpp2 *priv);
void mvpp2_musdk_uio_exit(struct mvpp2 *priv);

#endif /* _MVPP2_MUSDK_H_ */

