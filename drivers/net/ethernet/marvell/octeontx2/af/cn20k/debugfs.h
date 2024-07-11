/* SPDX-License-Identifier: GPL-2.0 */
/* Marvell OcteonTx2 CGX driver
 *
 * Copyright (C) 2024 Marvell.
 *
 */

#ifndef DEBUFS_H
#define DEBUFS_H

int npc_cn20k_debugfs_init(struct rvu *rvu);
void npc_cn20k_debugfs_deinit(struct rvu *rvu);
#endif
