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
#include <linux/if_bridge.h>

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
		struct {
			u8 mac[ETH_ALEN];
			bool dp_added;
		};
		struct {
			struct fl_tuple tuple;
			unsigned long cookie;
		};
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
			seq_printf(m, "mac=%pM", ev->mac);
			if (ev->dp_added)
				seq_puts(m, " (Added by DP)");
			seq_puts(m, "\n");
			continue;
		}

		if (ev->cmd == FL_ADD || ev->cmd == FL_DEL) {
			t = &ev->tuple;

			if (t->proto == IPPROTO_TCP)
				str_prot = "TCP";
			else if (t->proto == IPPROTO_UDP)
				str_prot = "UDP";
			else if (t->proto == IPPROTO_ICMP)
				str_prot = "icmp";

			seq_printf(m, "%s cookie=%lu (%pM, %pI4:%u) to (%pM, %pI4:%u) %s %s\n",
				   ev->cmd == FL_ADD ? "ADD" : "DEL",
				   ev->cookie,
				   t->smac, &t->ip4src, t->sport,
				   t->dmac, &t->ip4dst, t->dport,
				   (t->eth_type == htons(ETH_P_IPV6)) ? "IPv6" : "IPv4",
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

	switch (cmd) {
	case FDB_ADD:
	case FDB_DEL:
		ev = kcalloc(1, sizeof(*ev), GFP_KERNEL);
		ev->cmd = cmd;
		ev->jiffies = jiffies;
		ether_addr_copy(ev->mac, req->mac);
		ev->dp_added = !!(req->flags & DP_ADD);

		mutex_lock(&ev_lk);
		idx = __pan_sw_event_get_slot();
		ev_arr[idx] = ev;
		mutex_unlock(&ev_lk);
		break;

	case FL_ADD:
	case FL_DEL:
		ev = kcalloc(1, sizeof(*ev), GFP_KERNEL);
		ev->cmd = cmd;
		ev->jiffies = jiffies;
		ev->tuple = req->tuple;
		ev->cookie = req->cookie;

		mutex_lock(&ev_lk);
		idx = __pan_sw_event_get_slot();
		ev_arr[idx] = ev;
		mutex_unlock(&ev_lk);
		break;

	case FIB_CMD:
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
		break;
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

static DEFINE_SPINLOCK(pan_sw_fdb_lk);
static LIST_HEAD(pan_sw_fdb_lh);

struct pan_sw_fdb {
	struct list_head list;
	struct otx2_nic *pf;
	struct net_device *dev;
	u8 mac[6];
	u64 flags;
};

static void
pan_sw_dp_add_fdb(struct work_struct *unused)
{
	struct af2swdev_notify_req req = { };
	struct net_device *port;
	struct otx2_nic *nic;
	struct msg_rsp rsp = { };
	LIST_HEAD(llh);
	struct pan_sw_fdb *entry;

	spin_lock(&pan_sw_fdb_lk);
	list_splice_init(&pan_sw_fdb_lh, &llh);
	spin_unlock(&pan_sw_fdb_lk);

	if (list_empty(&llh))
		return;

	while ((entry = list_first_entry_or_null(&llh,
						 struct pan_sw_fdb, list))) {
		list_del_init(&entry->list);

		if (pan_sw_l2_mac_tbl_lookup(entry->mac)) {
			dev_put(entry->dev);
			kfree(entry);
			continue;
		}

		rtnl_lock();
		port = br_fdb_find_port(entry->dev, entry->mac, 0);
		if (!port) {
			dev_put(entry->dev);
			rtnl_unlock();
			kfree(entry);
			continue;
		}

		nic = netdev_priv(port);
		req.port_id = nic->pcifunc;
		req.flags = entry->flags;
		ether_addr_copy(req.mac, entry->mac);
		rtnl_unlock();

		otx2_mbox_up_handler_af2swdev_notify(entry->pf, &req, &rsp);
		dev_put(entry->dev);
		kfree(entry);
	}
}

static DECLARE_WORK(pan_sw_dp_work, pan_sw_dp_add_fdb);

// Hack
int pan_sw_inject_fdb_add_event(struct otx2_nic *pf, struct net_device *br_dev, u8 *mac)
{
	struct pan_sw_fdb *fdb_info;

	if (!is_valid_ether_addr(mac))
		return 0;

	if (pan_sw_l2_mac_tbl_lookup(mac))
		return 0;

	fdb_info = kcalloc(1, sizeof(*fdb_info), GFP_ATOMIC);
	if (!fdb_info)
		return -ENOMEM;

	dev_hold(br_dev);
	fdb_info->dev = br_dev;
	fdb_info->flags = FDB_ADD | DP_ADD;
	fdb_info->pf = pf;
	ether_addr_copy(fdb_info->mac, mac);
	INIT_LIST_HEAD(&fdb_info->list);

	spin_lock(&pan_sw_fdb_lk);
	list_add_tail(&fdb_info->list, &pan_sw_fdb_lh);
	spin_unlock(&pan_sw_fdb_lk);

	schedule_work(&pan_sw_dp_work);
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
