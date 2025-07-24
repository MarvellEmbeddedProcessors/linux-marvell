// SPDX-License-Identifier: GPL-2.0
/* Marvell RVU PAN driver
 *
 * Copyright (C) 2025 Marvell.
 *
 */

#include <linux/types.h>
#include <linux/string.h>
#include <linux/errno.h>
#include <linux/stddef.h>
#include <linux/seq_file.h>
#include <linux/debugfs.h>
#include <linux/slab.h>
#include <linux/list.h>
#include <linux/netdevice.h>
#include <net/switchdev.h>
#include <linux/hashtable.h>

#include "pan_cmn.h"

#define PAN_SWITCH_PORT_ID(domain, bus, devfn)  \
	(FIELD_PREP(GENMASK_ULL(31, 16), domain) | \
	FIELD_PREP(GENMASK_ULL(15, 8), bus) | \
	FIELD_PREP(GENMASK_ULL(7, 0), devfn))

u16 pan_sw_get_pcifunc(unsigned int port_id)
{
	return FIELD_GET(GENMASK_ULL(15, 0), port_id);
}

int otx2_mbox_up_handler_af2swdev_notify(struct otx2_nic *pf,
					 struct af2swdev_notify_req *req,
					 struct msg_rsp *rsp)
{
	if (req->flags & FDB_ADD)
		pan_sw_l2_offl(pf, 0x1234, req->port_id, req->mac);
	else if (req->flags & FDB_DEL)
		pan_sw_l2_de_offl(pf, 0x1234, req->port_id, req->mac);

	return 0;
}

static void pan_sw_debugfs_remove(void)
{
	struct dentry *parent, *pdir;

	parent = debugfs_lookup("cn10k", NULL);
	if (!parent) {
		pr_err("Could not find dir cn10ka in debugfs\n");
		return;
	}

	pdir = debugfs_lookup("pan", parent);
	if (!pdir)
		return;

	debugfs_remove_recursive(pdir);
}

int pan_sw_init(void)
{
	pan_sw_l2_init();
	return 0;
}

void pan_sw_deinit(void)
{
	pan_sw_l2_deinit();
	pan_sw_debugfs_remove();
}
