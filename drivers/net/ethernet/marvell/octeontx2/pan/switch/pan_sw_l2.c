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

static LIST_HEAD(l2_offl_lh);
static DEFINE_SPINLOCK(l2_offl_lock);

static struct otx2_nic *otx2_nic;

static DEFINE_HASHTABLE(mac_h_tbl, 8);

static inline unsigned int pan_sw_l2_mac_hash(const u8 *mac)
{
	return *(mac + 2);
}

static void
__pan_sw_l2_mac_table_add_entry(struct pan_sw_l2_offl_node *node)
{
	unsigned int hash = pan_sw_l2_mac_hash(node->mac);

	hash_add(mac_h_tbl, &node->hnode, hash);
}

static void
__pan_sw_l2_mac_table_del_entry(struct pan_sw_l2_offl_node *node)
{
	hash_del(&node->hnode);
}

struct pan_sw_l2_offl_node *pan_sw_l2_mac_tbl_lookup(const u8 *mac)
{
	struct pan_sw_l2_offl_node *entry = NULL;
	unsigned int hash = pan_sw_l2_mac_hash(mac);

	spin_lock(&l2_offl_lock);
	hash_for_each_possible(mac_h_tbl, entry, hnode, hash)
		if (ether_addr_equal(entry->mac, mac))
			break;
	spin_unlock(&l2_offl_lock);
	return entry;
}

static int pan_sw_l2_hw_remove_dmac_flow(u16 mcam_idx)
{
	struct npc_delete_flow_req *req;
	int err;

	mutex_lock(&otx2_nic->mbox.lock);
	req = otx2_mbox_alloc_msg_npc_delete_flow(&otx2_nic->mbox);
	if (!req) {
		mutex_unlock(&otx2_nic->mbox.lock);
		return -ENOMEM;
	}

	req->entry = mcam_idx;

	/* Send message to AF */
	err = otx2_sync_mbox_msg(&otx2_nic->mbox);
	mutex_unlock(&otx2_nic->mbox.lock);

	return err;
}

static int pan_sw_l2_hw_install_dmac_flow(u16 mcam_idx, u8 *mac_addr,
					  u16 cntr, u16 match_id)
{
	u8 mac_mask[ETH_ALEN] = { 0xff, 0xff, 0xff, 0xff, 0xff, 0xff};
	struct npc_install_flow_req *req;
	struct flow_msg *pkt, *pmask;
	int err;

	mutex_lock(&otx2_nic->mbox.lock);

	req = otx2_mbox_alloc_msg_npc_install_flow(&otx2_nic->mbox);
	if (!req) {
		mutex_unlock(&otx2_nic->mbox.lock);
		return -ENOMEM;
	}

	pkt = &req->packet;
	pmask = &req->mask;

	ether_addr_copy(pkt->dmac, mac_addr);
	ether_addr_copy(pmask->dmac, mac_mask);

	req->features |= BIT_ULL(NPC_DMAC);
	req->entry = mcam_idx;
	req->intf = NIX_INTF_RX;
	req->set_cntr = 1;
	req->cntr_val = cntr;
	req->op = NIX_RX_ACTIONOP_RSS;
	req->match_id = match_id;
	req->channel = 0;
	req->chan_mask = 0;
	req->set_chanmask = 1;

	err = otx2_sync_mbox_msg(&otx2_nic->mbox);
	if (err) {
		pr_err("Error happened while installing the flow\n");
		goto fail_flow;
	}

fail_flow:
	mutex_unlock(&otx2_nic->mbox.lock);
	return err;
}

static int pan_sw_l2_hw_alloc_dmac_counter(u16 *cntr)
{
	struct npc_mcam_alloc_counter_req *cntr_req;
	struct npc_mcam_alloc_counter_rsp *cntr_rsp;

	mutex_lock(&otx2_nic->mbox.lock);

	cntr_req = otx2_mbox_alloc_msg_npc_mcam_alloc_counter(&otx2_nic->mbox);
	if (!cntr_req) {
		mutex_unlock(&otx2_nic->mbox.lock);
		pr_err("Allocation req for cntr failed\n");
		return -EFAULT;
	}

	cntr_req->contig = true;
	cntr_req->count = 1;

	if (otx2_sync_mbox_msg(&otx2_nic->mbox))
		goto fail_mbox_sync;

	cntr_rsp = (struct npc_mcam_alloc_counter_rsp *)
			otx2_mbox_get_rsp(&otx2_nic->mbox.mbox, 0, &cntr_req->hdr);

	*cntr = cntr_rsp->cntr;
	mutex_unlock(&otx2_nic->mbox.lock);

	return 0;

fail_mbox_sync:
	mutex_unlock(&otx2_nic->mbox.lock);
	return -EFAULT;
}

static int pan_sw_l2_hw_alloc_dmac_flow(u16 *mcam_idx)
{
	struct npc_mcam_alloc_entry_req *req;
	struct npc_mcam_alloc_entry_rsp *rsp;
	struct otx2_flow_config *flow_cfg;

	flow_cfg = otx2_nic->flow_cfg;

	/* Alloc mcam entry */
	mutex_lock(&otx2_nic->mbox.lock);

	req = otx2_mbox_alloc_msg_npc_mcam_alloc_entry(&otx2_nic->mbox);
	if (!req) {
		pr_err("Error happened while mcam alloc req\n");
		goto fail_alloc_entry;
	}

	req->contig = false;
	req->count = 1;

	if (otx2_sync_mbox_msg(&otx2_nic->mbox))
		goto fail_mbox_sync;

	rsp = (struct npc_mcam_alloc_entry_rsp *)otx2_mbox_get_rsp
		(&otx2_nic->mbox.mbox, 0, &req->hdr);

	flow_cfg->flow_ent[flow_cfg->max_flows++] = rsp->entry_list[0];

	*mcam_idx = rsp->entry_list[0];

	mutex_unlock(&otx2_nic->mbox.lock);

	return 0;

fail_mbox_sync:
fail_alloc_entry:
	mutex_unlock(&otx2_nic->mbox.lock);
	return -EFAULT;
}

static void pan_sw_l2_dwork(struct work_struct *dwork);
static DECLARE_DELAYED_WORK(pan_sw_l2_dwq, pan_sw_l2_dwork);

static int pan_sw_l2_del_flow_tbl(struct pan_sw_l2_offl_node *node)
{
	struct pan_tuple tuple = { 0 };
	int err;

	pan_tuple_hash_set(&tuple, node->match_id);
	err = pan_fl_tbl_offl_del(&tuple);
	if (err) {
		pr_err("Failed to del tbl flow\n");
		return err;
	}

	return 0;
}

static int pan_sw_l2_add_flow_tbl(struct pan_sw_l2_offl_node *node)
{
	struct pan_rvu_gbl_t *pan_rvu_gbl;
	struct pan_fl_tbl_res res = { 0 };
	struct pan_tuple *tuple;
	u16 npc_matchid;
	u16 pcifunc;
	int err;

	tuple = &node->tuple;
	tuple->flags = PAN_TUPLE_FLAG_L3_PROTO_V4;

	pan_rvu_gbl = pan_rvu_get_gbl();
	npc_matchid = pan_alloc_matchid(&pan_rvu_gbl->rsrc);
	pcifunc = pan_sw_get_pcifunc(node->port_id);
	if (pcifunc == -1) {
		pr_err("pcifunc is -1 for port_id=%#x\n", node->port_id);
		return -EFAULT;
	}

	res.act = PAN_FL_TBL_ACT_L2_FWD;
	res.pcifuncoff = pan_rvu_pcifunc2_sq_off(pcifunc);
	res.dir = FLOW_OFFLOAD_DIR_ORIGINAL;

	pan_tuple_hash_set(tuple, npc_matchid);
	tuple->hash = npc_matchid;
	ether_addr_copy(tuple->dmac, node->mac);

	node->match_id = npc_matchid;

	pr_debug("Adding to PAN table mac=%pM pcifunc=%#x pcifuncoff=%u\n",
		 node->mac, pcifunc, res.pcifuncoff);

	/* MAC addr copied won't affect hash */
	err = pan_fl_tbl_add(tuple, &res, NULL);
	if (err) {
		pr_err("Failed to add tbl flow\n");
		return err;
	}

	return 0;
}

static int pan_sw_l2_offl_hw(struct pan_sw_l2_offl_node *node,
			     u16 *mcam_idx, u16 *cntr)
{
	/* TODO: error handling */
	int ret;

	ret = pan_sw_l2_hw_alloc_dmac_counter(cntr);
	if (ret) {
		pr_err("Error to alloc counter for mac=%pM\n", node->mac);
		return -EFAULT;
	}

	ret = pan_sw_l2_hw_alloc_dmac_flow(mcam_idx);
	if (ret) {
		pr_err("Error to alloc flow for mac=%pM\n", node->mac);
		return -EFAULT;
	}

	ret = pan_sw_l2_hw_install_dmac_flow(*mcam_idx, node->mac, *cntr, node->match_id);
	if (ret) {
		pr_err("Fail to install flow for mac=%pM\n", node->mac);
		node->state = SWITCH_L2_OFFL_STATE_FAIL;
		return -EFAULT;
	}

	pr_debug("Installed mcam=%d for mac=%pM\n", *mcam_idx, node->mac);
	return 0;
}

static void pan_sw_l2_dwork(struct work_struct *dwork)
{
	struct pan_sw_l2_offl_node *node;
	struct swdev2af_notify_req *req;
	struct otx2_nic *pan;
	unsigned long timeout;
	unsigned long long hits;
	u16 cntr;
	int ret;
	u16 mcam_idx;
	int iter = 3;

	while (iter--) {
		spin_lock(&l2_offl_lock);

		if (list_empty(&l2_offl_lh)) {
			spin_unlock(&l2_offl_lock);
			break;
		}

		node = list_first_entry(&l2_offl_lh,
					struct pan_sw_l2_offl_node,
					list);
		list_del_init(&node->list);
		spin_unlock(&l2_offl_lock);

		switch (node->state) {
		case SWITCH_L2_OFFL_STATE_DEINIT:
			ret = pan_sw_l2_hw_remove_dmac_flow(node->mcam_idx);
			if (ret) {
				pr_err("Deleting mac=%pM mcam_idx=%d failed\n",
				       node->mac, node->mcam_idx);
				kfree(node);
				continue;
			}

			ret =  pan_sw_l2_del_flow_tbl(node);
			if (ret) {
				pr_err("Could not del pan flow  mac=%pM mcam_idx=%u\n",
				       node->mac, node->mcam_idx);
				kfree(node);
				continue;
			}

			pr_debug("Deleted node %pM mcam_idx=%u state=%d\n",
				 node->mac, node->mcam_idx, node->state);
			kfree(node);
			continue;

		case SWITCH_L2_OFFL_STATE_INIT:
			ret = pan_sw_l2_add_flow_tbl(node);
			if (ret) {
				node->state = SWITCH_L2_OFFL_STATE_FAIL;
				break;
			}

			ret = pan_sw_l2_offl_hw(node, &mcam_idx, &cntr);
			if (ret) {
				node->state = SWITCH_L2_OFFL_STATE_FAIL;
				break;
			}

			node->state = SWITCH_L2_OFFL_STATE_DONE;
			node->mcam_idx = mcam_idx;

			pr_debug("Offloaded successfully mac=%pM mcam_idx=%u\n",
				 node->mac, node->mcam_idx);
			break;

		case SWITCH_L2_OFFL_STATE_DONE:

			/* Let update fdb once in 100 seconds. Usually fdb timeout is 300s
			 * TODO: what if user changes ageing timeout to lower value
			 */
			timeout = node->jiffies + 50 * HZ;
			if (time_before(jiffies, timeout))
				break;

			node->jiffies = jiffies;

			/* TODO: race with hits ? */
			ret =  __pan_fl_tbl_offl_get_hit_cnt(&node->tuple, &hits);
			if (ret)
				break;

			if (node->hits == hits)
				break;

			node->hits = hits;

			pr_debug("UPdating mac=%pM port_id=%#x\n", node->mac, node->port_id);

			pan = node->pf;

			mutex_lock(&pan->mbox.lock);
			req = otx2_mbox_alloc_msg_swdev2af_notify(&pan->mbox);
			if (!req) {
				ret = -ENOMEM;
				goto done;
			}

			req->msg_type = SWDEV2AF_MSG_TYPE_REFRESH_FDB;
			req->pcifunc = pan_sw_get_pcifunc(node->port_id);
			ether_addr_copy(req->mac, node->mac);

			/* Send message to AF to free MCAM entries */
			ret = otx2_sync_mbox_msg(&pan->mbox);
done:
			mutex_unlock(&pan->mbox.lock);
			break;

		default:
			pr_debug("Unknown state %d for mac=%pM\n", node->state, node->mac);
			break;
		}

		spin_lock(&l2_offl_lock);
		list_add_tail(&node->list, &l2_offl_lh);
		spin_unlock(&l2_offl_lock);
	}
	schedule_delayed_work(&pan_sw_l2_dwq, msecs_to_jiffies(10000));
}

int pan_sw_l2_de_offl(struct otx2_nic *pf, u32 switch_id,
		      unsigned int port_id, u8 *mac)
{
	struct pan_sw_l2_offl_node *entry, *tmp;
	int found = false;

	spin_lock(&l2_offl_lock);
	list_for_each_entry_safe(entry, tmp, &l2_offl_lh, list) {
		if (!ether_addr_equal(mac, entry->mac))
			continue;

		entry->state = SWITCH_L2_OFFL_STATE_DEINIT;
		__pan_sw_l2_mac_table_del_entry(entry);
		found = true;
	}
	spin_unlock(&l2_offl_lock);

	if (found)
		return 0;

	spin_lock(&l2_offl_lock);
	list_for_each_entry_safe(entry, tmp, &l2_offl_lh, list) {
		if (!ether_addr_equal(mac, entry->mac))
			continue;

		entry->state = SWITCH_L2_OFFL_STATE_DEINIT;
		__pan_sw_l2_mac_table_del_entry(entry);
		found = true;
	}
	spin_unlock(&l2_offl_lock);

	if (found)
		return 0;

	return -ESRCH;
}

int pan_sw_l2_offl(struct otx2_nic *pf, u32 switch_id,
		   unsigned int port_id, u8 *mac)
{
	struct pan_sw_l2_offl_node *node;

	node = kcalloc(1, sizeof(*node), GFP_KERNEL);
	if (!node)
		return -ENOMEM;

	ether_addr_copy(node->mac, mac);
	node->state = SWITCH_L2_OFFL_STATE_INIT;
	node->port_id = port_id;
	node->jiffies = jiffies;
	node->pf = pf;

	pr_debug("Received to add %pM\n", mac);

	spin_lock(&l2_offl_lock);
	list_add_tail(&node->list, &l2_offl_lh);
	__pan_sw_l2_mac_table_add_entry(node);
	spin_unlock(&l2_offl_lock);

	return 0;
}

static int pan_sw_l2_show(struct seq_file *m, void *v)
{
	struct pan_sw_l2_offl_node *entry, *tmp;

	seq_puts(m, "\n++++ pan switch +++\n\n");

	spin_lock(&l2_offl_lock);

	list_for_each_entry_safe(entry, tmp, &l2_offl_lh, list) {
		seq_printf(m, "%pM : state=%u mcam=%u match_idx=%u\n",
			   entry->mac, entry->state, entry->mcam_idx,
			   entry->match_id);
	}

	spin_unlock(&l2_offl_lock);
	return 0;
}

DEFINE_SHOW_ATTRIBUTE(pan_sw_l2);

static void pan_sw_l2_debugfs_remove(void)
{
}

static int pan_sw_l2_debugfs_add(void)
{
	struct dentry *parent, *pdir;
	struct dentry *file;

	parent = debugfs_lookup("cn10k", NULL);
	if (!parent)
		parent = debugfs_lookup("octeontx2", NULL);

	if (!parent) {
		pr_err("Could not find dir cn10ka or octeontx2 in debugfs\n");
		return -ESRCH;
	}

	pdir = debugfs_lookup("pan", parent);
	if (!pdir)
		pdir = debugfs_create_dir("pan", parent);

	if (!pdir) {
		pr_err("Could not create pan directory\n");
		return -ESRCH;
	}

	file = debugfs_create_file("switch", 0400, pdir, NULL,
				   &pan_sw_l2_fops);
	if (!file) {
		pr_err("Could not create switch debugfs entry\n");
		return -EFAULT;
	}

	return 0;
}

int pan_sw_l2_init(void)
{
	struct pan_rvu_dev_priv *pan_priv;
	struct net_device *dev;

	dev = dev_get_by_name(&init_net, PAN_DEV_NAME);
	if (!dev) {
		pr_err("Could not find PAN device\n");
		return -EFAULT;
	}
	dev_put(dev);

	pan_priv = netdev_priv(dev);
	otx2_nic = pan_priv->otx2_nic;

	hash_init(mac_h_tbl);

	pan_sw_l2_debugfs_add();
	schedule_delayed_work(&pan_sw_l2_dwq, msecs_to_jiffies(10000));
	return 0;
}

void pan_sw_l2_deinit(void)
{
	pan_sw_l2_debugfs_remove();
	flush_delayed_work(&pan_sw_l2_dwq);
	cancel_delayed_work_sync(&pan_sw_l2_dwq);
}
