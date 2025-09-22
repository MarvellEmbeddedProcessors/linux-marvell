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
#include "../nic/switch/sw_nb.h"

#define PAN_SWITCH_PORT_ID(domain, bus, devfn)  \
	(FIELD_PREP(GENMASK_ULL(31, 16), domain) | \
	FIELD_PREP(GENMASK_ULL(15, 8), bus) | \
	FIELD_PREP(GENMASK_ULL(7, 0), devfn))

static DEFINE_MUTEX(ev_lk);

static char *pan_sw_event_cmd2str[] = {
	[FDB_ADD] = "FDB ADD",
	[FDB_DEL] = "FDB DEL",
	[FL_ADD] = "FL ADD",
	[FL_DEL] = "FL DEL",
	[FIB_CMD] = "FIB CMD",
};

struct pan_sw_event {
	int cmd;
	unsigned long jiffies;

	union {
		struct fib_entry fe;
		u8 mac[16];
		struct fl_tuple tuple;
	};
};

static struct pan_sw_event *ev_arr[32];
static u8 ev_cnt;

static void pan_sw_event_slots_free(void)
{
	int i;

	mutex_lock(&ev_lk);
	for (i = 0; i < 32; i++) {
		kfree(ev_arr[i]);
		ev_arr[i] = NULL;
	}
	mutex_unlock(&ev_lk);
}

static int __pan_sw_event_get_slot(void)
{
	int idx;

	idx = ev_cnt % 32;
	ev_cnt++;

	kfree(ev_arr[idx]);
	ev_arr[idx] = 0;

	return idx;
}

static int pan_sw_debugfs_show(struct seq_file *m, void *v)
{
	struct pan_sw_event *ev;
	struct fib_entry *fe;
	struct fl_tuple *t;
	int idx;
	char *str_prot = NULL;

	mutex_lock(&ev_lk);
	idx = ev_cnt;

	for (int i = 0; i < 32; i++, idx++) {
		idx = idx % 32;
		if (!ev_arr[idx])
			continue;

		ev = ev_arr[idx];
		if (ev->cmd == FDB_ADD || ev->cmd == FDB_DEL) {
			seq_printf(m, "%lu %s\t", ev->jiffies,
				   pan_sw_event_cmd2str[ev->cmd]);
			seq_printf(m, "mac=%pM\n", ev->mac);
			continue;
		}

		if (ev->cmd == FL_ADD || ev->cmd == FL_DEL) {
			t = &ev->tuple;

			if (t->proto == 6)
				str_prot = "TCP";
			else if (t->proto == 17)
				str_prot = "UDP";

			seq_printf(m, "%pI4(%u) to %pI4(%u)  %s %s\n",
				   &t->ip4src, t->sport, &t->ip4dst, t->dport,
				   (t->eth_type == htonl(0x86dd)) ? "IPv6" : "IPv4",
				   str_prot);
			continue;
		}

		fe = &ev->fe;
		seq_printf(m, "%lu %s\t", ev->jiffies, sw_nb_get_cmd2str(fe->cmd));
		seq_printf(m, "dst=%pI4h len=%u vlan_id=%#x\n", fe->gw_valid ? &fe->gw : &fe->dst,
			   fe->dst_len, fe->vlan_tag);
	}
	mutex_unlock(&ev_lk);
	return 0;
}
DEFINE_SHOW_ATTRIBUTE(pan_sw_debugfs);

static void pan_sw_event_log(struct af2swdev_notify_req *req)
{
	struct pan_sw_event *ev;
	struct fib_entry *fe;
	int cmd;
	int idx;

	if (req->flags & FDB_ADD)
		cmd = FDB_ADD;
	else if (req->flags & FDB_DEL)
		cmd = FDB_DEL;
	else if (req->flags & FIB_CMD)
		cmd = FIB_CMD;
	else if (req->flags & FL_ADD)
		cmd = FL_ADD;
	else if (req->flags & FL_DEL)
		cmd = FL_DEL;

	if (cmd == FDB_ADD || cmd == FDB_DEL) {
		ev = kcalloc(1, sizeof(*ev), GFP_KERNEL);
		ev->cmd = cmd;
		ev->jiffies = jiffies;
		ether_addr_copy(ev->mac, req->mac);

		mutex_lock(&ev_lk);
		idx = __pan_sw_event_get_slot();
		ev_arr[idx] = ev;
		mutex_unlock(&ev_lk);

		return;
	}

	if (cmd == FL_ADD || cmd == FL_DEL) {
		ev = kcalloc(1, sizeof(*ev), GFP_KERNEL);
		ev->cmd = cmd;
		ev->jiffies = jiffies;
		ev->tuple = req->tuple;

		mutex_lock(&ev_lk);
		idx = __pan_sw_event_get_slot();
		ev_arr[idx] = ev;
		mutex_unlock(&ev_lk);

		return;
	}

	fe = req->entry;
	for (int i = 0; i < req->cnt; i++, fe++) {
		if (fe->cmd == OTX2_NEIGH_UPDATE)
			continue;

		ev = kcalloc(1, sizeof(*ev), GFP_KERNEL);
		ev->cmd = cmd;
		ev->fe = *fe;
		ev->jiffies = jiffies;

		mutex_lock(&ev_lk);
		idx = __pan_sw_event_get_slot();
		ev_arr[idx] = ev;
		mutex_unlock(&ev_lk);
	}
}

u16 pan_sw_get_pcifunc(unsigned int port_id)
{
	return FIELD_GET(GENMASK_ULL(15, 0), port_id);
}

static bool sw_mode = true;
int otx2_mbox_up_handler_af2swdev_notify(struct otx2_nic *pf,
					 struct af2swdev_notify_req *req,
					 struct msg_rsp *rsp)
{
	int err;

	pan_sw_event_log(req);

	if (req->flags & (FDB_ADD | FDB_DEL)) {
		err = pan_sw_l2_ev_enq(pf, 0x1234, req->port_id,
				       req->mac, req->flags);
		goto done;
	}

	if (!sw_mode) {
		if (req->flags & (FL_ADD | FL_DEL)) {
			err = pan_sw_fl_ev_enq(pf, 0x1234, req->port_id,
					       &req->tuple, req->flags,
					       req->cookie);
			goto done;
		}
		return 0;
	}

	if (req->flags & FIB_CMD) {
		err = pan_sw_l3_ev_enq(pf, req->cnt, req->entry);
		goto done;
	}

done:

	if (err)
		pr_debug("%s:%d Error happened while pushing rule to PAN\n",
			 __func__, __LINE__);
	return 0;
}

static int pan_sw_simple_llu_get(void *data, u64 *val)
{
	*val = !!sw_mode;
	return 0;
}

static void pan_sw_l3_fib_disable(void)
{
	pan_sw_l3_deinit();
}

static int pan_sw_simple_llu_set(void *data, u64 val)
{
	if (val != 0) {
		pr_err("Only disabling (0) is allowed\n");
		return -EINVAL;
	}

	pan_sw_l3_fib_disable();
	sw_mode = false;
	return 0;
}

DEFINE_SIMPLE_ATTRIBUTE(pan_sw_simple_llu_fops, pan_sw_simple_llu_get,
			pan_sw_simple_llu_set, "%llu\n");

static void pan_sw_debugfs_create(void)
{
	struct dentry *parent, *pdir, *file;

	parent = debugfs_lookup("cn10k", NULL);
	if (!parent) {
		pr_err("Could not find dir cn10ka in debugfs\n");
		return;
	}

	pdir = debugfs_lookup("pan", parent);
	if (!pdir)
		return;

	file = debugfs_create_file("sw_events", 0400, pdir, NULL,
				   &pan_sw_debugfs_fops);

	file = debugfs_create_file("sw_mode", 0600, pdir, "l3_rtable_dis",
				   &pan_sw_simple_llu_fops);

}

static void pan_sw_debugfs_remove(void)
{
	pan_dbgfs_rm_file("sw_events");
	pan_dbgfs_rm_file("sw_mode");
}

int pan_sw_init(void)
{
	pan_sw_l2_init();
	pan_sw_l3_init();
	pan_sw_fl_init();
	pan_sw_debugfs_create();

	return 0;
}

void pan_sw_deinit(void)
{
	pan_sw_fl_deinit();
	if (sw_mode)
		pan_sw_l3_deinit();

	pan_sw_l2_deinit();
	pan_sw_debugfs_remove();
	pan_sw_event_slots_free();
}
