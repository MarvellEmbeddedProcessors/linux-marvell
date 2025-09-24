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
#include <linux/debugfs.h>

#include "pan_cmn.h"
#include "../nic/switch/sw_nb.h"

static struct otx2_nic *otx2_nic;

static LIST_HEAD(offl_l3_lh);
static DEFINE_SPINLOCK(offl_l3_lock);

static DEFINE_HASHTABLE(fib_h_tbl, 8);
static HLIST_HEAD(fib_root_lh);
static struct pan_sw_l3_offl_tnode *root;

static struct hlist_head fib_hnodes[33];

struct pan_sw_l3_offl_tnode  {
	struct pan_sw_l3_offl_node *node;
	struct pan_sw_l3_offl_tnode *l, *r, *p;
	struct hlist_node hnode;
	struct hlist_node lh;
	struct hlist_node hnode2; // For dst_len
};

static unsigned long valid_route;
static int cnt_routes;

static int pan_sw_l3_fl_tbl_del_one_entry(struct pan_sw_l3_offl_node *node)
{
	int err;

	if (!node)
		return 0;

	if (!node->tuple_installed)
		return 0;

	pan_tuple_hash_set(&node->tuple, node->match_id);
	err = pan_fl_tbl_offl_del(&node->tuple);
	if (err) {
		pr_err("%s:%d Failed to del tbl flow match_id %d\n",
		       __func__, __LINE__, node->match_id);
		return err;
	}
	node->tuple_installed = 0;

	return 0;
}

static void pan_sw_l3_walk_n_free_empty_tnode(struct pan_sw_l3_offl_tnode *walk)
{
	if (!walk)
		return;

	pan_sw_l3_walk_n_free_empty_tnode(walk->l);
	pan_sw_l3_walk_n_free_empty_tnode(walk->r);
	if (walk == root)
		root = NULL;

	kfree(walk);
}

static int
pan_sw_l3_route_del(struct fib_entry *entry, int *mcam_idx, int *match_id, bool *me_deleted);

static void
pan_sw_l3_fl_tbl_del_n_free_all(struct pan_sw_l3_offl_tnode *walk, bool *me_deleted)
{
	struct pan_sw_l3_offl_tnode *pos, *r;
	struct pan_sw_l3_offl_node *node;
	int mcam_idx, match_id;

	if (!walk)
		return;

	r = walk->r;

	*me_deleted = false;
	pan_sw_l3_fl_tbl_del_n_free_all(walk->l, me_deleted);

	if (*me_deleted && r)
		r->p = NULL;

	*me_deleted = false;
	pan_sw_l3_fl_tbl_del_n_free_all(r, me_deleted);

	if (*me_deleted)
		return;

	if (!walk->node)
		return;

	if (walk != root) {
		node = walk->node;
		if (!node)
			return;

		pan_sw_l3_fl_tbl_del_one_entry(node);
		pan_sw_l3_route_del(node->entry, &mcam_idx, &match_id, me_deleted);
		return;
	}

	hlist_for_each_entry(pos, &fib_root_lh, lh) {
		node = pos->node;
		if (!node)
			continue;

		pan_sw_l3_fl_tbl_del_one_entry(node);
		pan_sw_l3_route_del(node->entry, &mcam_idx, &match_id, me_deleted);
	}
}

static void
pan_sw_l3_fl_tbl_del_all(struct pan_sw_l3_offl_tnode *walk)
{
	struct pan_sw_l3_offl_tnode *pos;

	if (!walk)
		return;

	pan_sw_l3_fl_tbl_del_all(walk->l);
	pan_sw_l3_fl_tbl_del_all(walk->r);

	if (!walk->node)
		return;

	if (walk != root) {
		pan_sw_l3_fl_tbl_del_one_entry(walk->node);
		return;
	}

	hlist_for_each_entry(pos, &fib_root_lh, lh)
		pan_sw_l3_fl_tbl_del_one_entry(pos->node);
}

static int pan_sw_l3_hw_npc_del_flows(int *marr, int *cnt)
{
	struct npc_flow_del_n_free_req *req;
	int err;

	if (!*cnt)
		return 0;

	mutex_lock(&otx2_nic->mbox.lock);
	req = otx2_mbox_alloc_msg_npc_flow_del_n_free(&otx2_nic->mbox);
	if (!req) {
		mutex_unlock(&otx2_nic->mbox.lock);
		return -ENOMEM;
	}

	req->cnt = *cnt;

	for (int i = 0; i < *cnt; i++)
		req->entry[i] = marr[i];

	/* Send message to AF */
	err = otx2_sync_mbox_msg(&otx2_nic->mbox);
	mutex_unlock(&otx2_nic->mbox.lock);

	*cnt = 0;

	return 0;
}

static int *pan_sw_l3_hw_alloc_mcam(u16 cnt)
{
	struct npc_mcam_alloc_entry_req *req;
	struct npc_mcam_alloc_entry_rsp *rsp;
	struct otx2_flow_config *flow_cfg;
	int *arr, i;
	int entry;

	flow_cfg = otx2_nic->flow_cfg;

	/* Alloc mcam entry */
	mutex_lock(&otx2_nic->mbox.lock);

	req = otx2_mbox_alloc_msg_npc_mcam_alloc_entry(&otx2_nic->mbox);
	if (!req) {
		pr_err("%s:%d Error happened while mcam alloc req\n",
		       __func__, __LINE__);
		goto fail_alloc_entry;
	}

	req->contig = true;
	req->count = cnt;

	if (otx2_sync_mbox_msg(&otx2_nic->mbox)) {
		pr_err("%s:%d Error to sync mbox\n", __func__, __LINE__);
		goto fail_mbox_sync;
	}

	rsp = (struct npc_mcam_alloc_entry_rsp *)otx2_mbox_get_rsp
		(&otx2_nic->mbox.mbox, 0, &req->hdr);

	flow_cfg->flow_ent[flow_cfg->max_flows++] = rsp->entry_list[0];
	arr = kcalloc(cnt, sizeof(int), GFP_KERNEL);

	entry = rsp->entry;
	for (i = 0; i < cnt; i++)
		arr[i] = entry++;

	mutex_unlock(&otx2_nic->mbox.lock);

	return arr;

fail_mbox_sync:
fail_alloc_entry:
	mutex_unlock(&otx2_nic->mbox.lock);
	return NULL;
}

static int pan_sw_l3_flow_tbl_entry_add(struct pan_sw_l3_offl_node *node)
{
	struct pan_fl_tbl_opaque opq = { 0 };
	struct pan_rvu_gbl_t *pan_rvu_gbl;
	struct pan_fl_tbl_res res = { 0 };
	struct net_device *netdev;
	struct pan_tuple *tuple;
	struct fib_entry *entry;
	struct netdev_hw_addr *ha;
	u16 pcifunc;
	int err;

	if (node->tuple_installed)
		return 0;

	tuple = &node->tuple;
	tuple->flags = PAN_TUPLE_FLAG_L3_PROTO_V4;

	pan_rvu_gbl = pan_rvu_get_gbl();
	pcifunc = pan_sw_get_pcifunc(node->port_id);
	if (pcifunc == -1) {
		pr_err("%s:%d pcifunc is -1 for port_id=%#x\n",
		       __func__, __LINE__, node->port_id);
		return -EFAULT;
	}

	entry = node->entry;

	if (entry->vlan_valid)
		opq.vlan_tag = entry->vlan_tag;

	netdev = xa_load(&pan_rvu_gbl->pfunc2dev, pcifunc);
	if (netdev && !entry->host) {
		res.opq = &opq;
		for_each_dev_addr(netdev, ha) {
			/* TODO: what if there are More than one mac address */
			ether_addr_copy(opq.eg_smac, ha->addr);
			break;
		}
	}

	if (entry->host) {
		res.act = PAN_FL_TBL_ACT_EXP;
	} else if (entry->bridge) {
		res.act = entry->vlan_valid ? PAN_FL_TBL_ACT_L3_BR_VLAN_FWD :
			PAN_FL_TBL_ACT_L3_BR_FWD;
	} else {
		res.act = entry->vlan_valid ? PAN_FL_TBL_ACT_L3_VLAN_FWD :
			PAN_FL_TBL_ACT_L3_FWD;
	}

	res.pcifuncoff = pan_rvu_pcifunc2_sq_off(pcifunc);

	res.dir = IP_CT_DIR_ORIGINAL;

	pan_tuple_hash_set(tuple, node->match_id);

	if (entry->mac_valid)
		ether_addr_copy(tuple->dmac, entry->mac);

	if (entry->gw_valid)
		tuple->dst_ip4.s_addr = htonl(entry->gw);
	else
		tuple->dst_ip4.s_addr = htonl(entry->dst);

	pr_debug("%s:%d Adding to PAN table mac=%pM pcifunc=%#x pcifuncoff=%u\n",
		 __func__, __LINE__,
		 entry->mac, pcifunc, res.pcifuncoff);

	/* MAC addr copied won't affect hash */
	err = pan_fl_tbl_offl_add(tuple, &res);
	if (err) {
		pr_err("%s:%d Failed to add tbl flow\n", __func__, __LINE__);
		return err;
	}

	node->tuple_installed = 1;

	return 0;
}

static int pan_sw_l3_hw_install_flow(struct pan_sw_l3_offl_node *node)
{
	u8 mac_mask[ETH_ALEN] = { 0xff, 0xff, 0xff, 0xff, 0xff, 0xff};
	struct npc_install_flow_req *req;
	struct flow_msg *pkt, *pmask;
	struct fib_entry *entry;
	int bits, err;
	u32 mask;

	if (node->mcam_idx == -1)
		return 0;

	mutex_lock(&otx2_nic->mbox.lock);

	req = otx2_mbox_alloc_msg_npc_install_flow(&otx2_nic->mbox);
	if (!req) {
		mutex_unlock(&otx2_nic->mbox.lock);
		return -ENOMEM;
	}

	pkt = &req->packet;
	pmask = &req->mask;

	entry = node->entry;

	if (entry->mac_valid) {
		ether_addr_copy(pkt->dmac, entry->mac);
		ether_addr_copy(pmask->dmac, mac_mask);
		req->features |= BIT_ULL(NPC_DMAC);
	}

	if (entry->gw_valid) {
		pkt->ip4dst = htonl(entry->gw);
		pmask->ip4dst = 0xffffffff;
		req->features |= BIT_ULL(NPC_DIP_IPV4);
	} else {
		pkt->ip4dst = htonl(entry->dst);
		bits = entry->dst_len;
		mask = ((1ULL << bits) - 1) << (31 - bits + 1);
		pmask->ip4dst = htonl(mask);
		req->features |= BIT_ULL(NPC_DIP_IPV4);
	}

	req->entry = node->mcam_idx;
	req->intf = NIX_INTF_RX;
	req->set_cntr = 1;
	req->op = NIX_RX_ACTIONOP_RSS;
	req->match_id = node->match_id;
	req->channel = 0;
	req->chan_mask = 0;
	req->set_chanmask = 1;

	err = otx2_sync_mbox_msg(&otx2_nic->mbox);
	if (err) {
		pr_err("%s:%d Error happened while installing the flow\n",
		       __func__, __LINE__);
		goto fail_flow;
	}

fail_flow:
	mutex_unlock(&otx2_nic->mbox.lock);

	return err;
}

static void pan_sw_l3_fib_dump(struct fib_entry *entry)
{
	pr_debug("%s:%d cmd=%s gw_valid=%d mac_valid=%d dst=%#x len=%d gw=%#x mac=%pM nud_state=%#x\n",
		 __func__, __LINE__,
		 sw_nb_get_cmd2str(entry->cmd),
		 entry->gw_valid, entry->mac_valid, entry->dst, entry->dst_len,
		 entry->gw, entry->mac, entry->nud_state);
}

static void pan_sw_l3_node_dump(struct pan_sw_l3_offl_node *node)
{
	struct fib_entry *entry = node->entry;

	pr_debug("%s:%d port_id=%#x mcam_idx=%d match_id=%d\n",
		 __func__, __LINE__, node->port_id, node->mcam_idx, node->match_id);
	pr_debug("%s:%d cmd=%s gw_valid=%d mac_valid=%d dst=%#x len=%d gw=%#x mac=%pM nud_state=%#x\n",
		 __func__, __LINE__,
		 sw_nb_get_cmd2str(entry->cmd),
		 entry->gw_valid, entry->mac_valid, entry->dst, entry->dst_len,
		 entry->gw, entry->mac, entry->nud_state);
}

static struct workqueue_struct *pan_sw_l3_fib_wq;
static void pan_sw_l3_fib_work_handler(struct work_struct *work);
static DECLARE_DELAYED_WORK(pan_sw_l3_fib_work, pan_sw_l3_fib_work_handler);

struct pan_sw_l3_ev {
	struct list_head list;
	struct otx2_nic *otx2_nic;
	int cnt;
	struct fib_entry fe[];
};

static LIST_HEAD(pan_sw_l3_ev_lh);
static int pan_sw_l3_ev_process(bool *);

static int *marr;
static int lcnt;
static void pan_sw_l3_fib_work_handler(struct work_struct *work)
{
	struct pan_sw_l3_offl_tnode *tnode;
	struct pan_sw_l3_offl_node *node, nnode;
	struct fib_entry entry;
	int num_routes;
	bool reshuffle;
	int smidx;
	int bitnr;
	int emidx;
	int err;

	if (pan_sw_l3_ev_process(&reshuffle))
		return;

	if (!reshuffle)
		return;

	spin_lock(&offl_l3_lock);
	pan_sw_l3_fl_tbl_del_all(root);
	num_routes = cnt_routes;
	spin_unlock(&offl_l3_lock);

	pan_sw_l3_hw_npc_del_flows(marr, &lcnt);
	kfree(marr);
	marr = NULL;

	if (!num_routes)
		return;

	lcnt = num_routes;
	marr = pan_sw_l3_hw_alloc_mcam(num_routes);

	smidx = 0;
	emidx = num_routes - 1;

	spin_lock(&offl_l3_lock);
	for_each_set_bit(bitnr, &valid_route, 33) {
		int bucket = bitnr;

		if (hlist_empty(&fib_hnodes[bucket])) {
			pr_err("%s:%d bucket %d is empty\n",
			       __func__, __LINE__, bucket);
			continue;
		}

		hlist_for_each_entry(tnode, &fib_hnodes[bucket], hnode2) {
			node = tnode->node;
			if (!node) {
				pr_debug("%s:%d No node in tnode\n",
					 __func__, __LINE__);
				continue;
			}

			node->mcam_idx = marr[emidx];

			emidx--;

			pan_sw_l3_node_dump(node);

			nnode = *node;
			entry = *node->entry;
			nnode.entry = &entry;

			node->tuple_installed = 1;
			spin_unlock(&offl_l3_lock);

			pan_sw_l3_flow_tbl_entry_add(&nnode);
			err = pan_sw_l3_hw_install_flow(&nnode);
			if (err)
				pr_err("%s:%d Error to push NPC rule\n",
				       __func__, __LINE__);

			spin_lock(&offl_l3_lock);
		}
	}
	spin_unlock(&offl_l3_lock);

	spin_lock(&offl_l3_lock);
	if (!list_empty(&pan_sw_l3_ev_lh))
		queue_delayed_work(pan_sw_l3_fib_wq, &pan_sw_l3_fib_work,
				   msecs_to_jiffies(100));
	spin_unlock(&offl_l3_lock);

}

static void
pan_sw_l3_fib_h_tbl_add_entry(struct pan_sw_l3_offl_tnode *tnode)
{
	struct hlist_node *p, *n, *s = NULL;
	struct fib_entry *fe;
	unsigned int hash;

	fe = tnode->node->entry;
	hash = fe->gw_valid ? fe->gw : fe->dst;

	hash_add(fib_h_tbl, &tnode->hnode, hash);

	if (hlist_empty(&fib_hnodes[fe->dst_len])) {
		hlist_add_head(&tnode->hnode2, &fib_hnodes[fe->dst_len]);
	} else {
		hlist_for_each_safe(p, n, &fib_hnodes[fe->dst_len])
			s = p;

		hlist_add_behind(&tnode->hnode2, s);
	}

	set_bit(fe->dst_len, &valid_route);
	cnt_routes++;
}

static void
pan_sw_l3_fib_h_tbl_del_entry(struct pan_sw_l3_offl_tnode *tnode)
{
	struct fib_entry *fe;
	int dst_len;

	hash_del(&tnode->hnode);
	hlist_del_init(&tnode->hnode2);

	if (tnode->node) {
		fe = tnode->node->entry;

		dst_len = tnode->node->entry->dst_len;
		if (hlist_empty(&fib_hnodes[dst_len]))
			clear_bit(dst_len, &valid_route);
		cnt_routes--;
	}
}

static struct pan_sw_l3_offl_tnode *
pan_sw_l3_fib_h_tbl_lookup(struct fib_entry *entry)
{
	unsigned int hash = entry->gw_valid ? entry->gw : entry->dst;
	struct pan_sw_l3_offl_tnode *tentry;
	struct fib_entry *fe;

	hash_for_each_possible(fib_h_tbl, tentry, hnode, hash) {
		if (!tentry->node) {
			pr_err("%s:%d Found a tnode without node while searching for dst=%#x\n",
			       __func__, __LINE__, hash);
			continue;
		}

		fe = tentry->node->entry;
		if (entry->gw_valid && fe->gw_valid) {
			if (fe->gw == entry->gw)
				return tentry;

			continue;
		}

		if (fe->dst != entry->dst)
			continue;

		if (fe->dst_len != entry->dst_len)
			continue;

		return tentry;
	}
	return NULL;
}

static int
pan_sw_l3_neigh_update(struct fib_entry *entry)
{
	struct pan_sw_l3_offl_tnode *tnode;
	struct fib_entry *fe;
	bool nud_valid;

	/* Check if it is a host ? */
	entry->dst_len = 32;
	tnode = pan_sw_l3_fib_h_tbl_lookup(entry);
	if (!tnode) {
		/* Check if it a gw */
		entry->gw_valid = 1;
		entry->gw = entry->dst;
		tnode = pan_sw_l3_fib_h_tbl_lookup(entry);
	}

	if (!tnode) {
		pr_debug("%s:%d Failed to find tnode for dst=%#x entry->mac=%pM\n",
			 __func__, __LINE__,
			 entry->dst, entry->mac);
		return -ESRCH;
	}

	fe = tnode->node->entry;
	nud_valid = fe->nud_state & NUD_VALID;

	if (nud_valid) {
		if (ether_addr_equal(fe->mac, entry->mac))
			return 0;

		pr_debug("%s:%d Changing mac to %pM from %pM for DST=%#x gw=%#x\n",
			 __func__, __LINE__,
			 fe->mac, entry->mac, entry->dst, entry->gw);
		ether_addr_copy(fe->mac, entry->mac);
		fe->mac_valid = 1;
		return 0;
	}

	if (fe->nud_state == NUD_FAILED) {
		pr_debug("%s:%d Resetting mac to 0 from %pM for DST=%#x gw=%#x\n",
			 __func__, __LINE__,
			 fe->mac, entry->dst, entry->gw);
		fe->mac_valid = 0;
		eth_zero_addr(fe->mac);
	}

	return 0;
}

static struct pan_sw_l3_offl_tnode *
pan_sw_l3_tnode_alloc(struct otx2_nic *pf,
		      u32 switch_id,
		      struct fib_entry *entry)
{
	struct pan_sw_l3_offl_tnode *tnode;
	struct pan_sw_l3_offl_node *node;
	struct pan_rvu_gbl_t *pan_rvu_gbl;

	pan_rvu_gbl = pan_rvu_get_gbl();

	tnode = kcalloc(1, sizeof(*tnode), GFP_KERNEL);
	if (!tnode)
		return NULL;

	tnode->node = kcalloc(1, sizeof(*tnode->node), GFP_KERNEL);
	INIT_HLIST_NODE(&tnode->lh);
	INIT_HLIST_NODE(&tnode->hnode);
	INIT_HLIST_NODE(&tnode->hnode2);
	node = tnode->node;

	node->port_id = entry->port_id;
	node->jiffies = jiffies;
	node->pf = pf;
	node->match_id = pan_alloc_matchid(&pan_rvu_gbl->rsrc);
	node->mcam_idx = -1;

	node->entry = kcalloc(1, sizeof(*entry), GFP_KERNEL);
	if (!node->entry)
		return NULL;

	*node->entry = *entry;
	return tnode;
}

static int
pan_sw_l3_route_add(struct pan_sw_l3_offl_tnode *tnode)
{
	struct pan_sw_l3_offl_tnode *tn = NULL, *walk;
	struct pan_sw_l3_offl_node *node = tnode->node;
	struct fib_entry *entry = node->entry;
	u32 sbit;
	u32 cnt;
	u32 dst;

	pan_sw_l3_fib_dump(entry);

	if (entry->gw_valid) {
		pan_sw_l3_fib_h_tbl_add_entry(tnode);

		if (!root) {
			root = tnode;
			hlist_add_head(&tnode->lh, &fib_root_lh);

			return 0;
		}

		if (!root->node) {
			tnode->l = root->l;
			tnode->r = root->r;

			pan_sw_l3_fib_h_tbl_del_entry(root);
			hlist_del_init(&root->lh);
			kfree(root);

			pr_debug("%s:%d First node exist, but a new gw=%#x got populated there\n",
				 __func__, __LINE__, entry->gw);

			root = tnode;
			hlist_add_head(&tnode->lh, &fib_root_lh);
			return 0;
		}

		/* Request to add one more gw; new one would be the valid */
		hlist_add_head(&tnode->lh, &fib_root_lh);
		tnode->l = root->l;
		tnode->r = root->r;
		root->l = NULL;
		root->r = NULL;
		root = tnode;

		pr_debug("%s:%d new gw=%#x got Added\n",
			 __func__, __LINE__, entry->gw);

		return 0;
	}

	if (!root)
		root = kcalloc(1, sizeof(*root), GFP_ATOMIC);
	kfree(tnode);

	walk = root;
	cnt = entry->dst_len;
	dst = entry->dst;
	sbit = 31;

	while (cnt) {
		u32 mask = 1 << sbit;

		if (!tn)
			tn = kcalloc(1, sizeof(*tn), GFP_ATOMIC);

		if (dst & mask) {
			if (!walk->r) {
				walk->r = tn;
				tn->p = walk;
				tn = NULL;
			}
			walk = walk->r;
		} else {
			if (!walk->l) {
				walk->l = tn;
				tn->p = walk;
				tn = NULL;
			}
			walk = walk->l;
		}
		sbit--;
		cnt--;
	}

	walk->node = node;
	pan_sw_l3_fib_h_tbl_add_entry(walk);

	pr_debug("%s:%d new route dst=%#x dst_len=%d got Added, match_id=%d\n",
		 __func__, __LINE__, entry->dst, entry->dst_len, node->match_id);

	return 0;
}

static int
pan_sw_l3_route_del(struct fib_entry *entry, int *mcam_idx, int *match_id, bool *me_deleted)
{
	struct pan_sw_l3_offl_tnode *tn, *walk, *p, *next;
	struct pan_rvu_gbl_t *pan_rvu_gbl;
	u32 sbit, cnt, dst;

	pan_rvu_gbl = pan_rvu_get_gbl();

	pr_debug("%s:%d route DEL request for  dst=%#x dst_len=%d got Added\n",
		 __func__, __LINE__, entry->dst, entry->dst_len);

	*me_deleted = false;

	if (entry->gw_valid) {
		tn = pan_sw_l3_fib_h_tbl_lookup(entry);
		if (!tn) {
			pr_err("%s:%d Could not find entry->gw=%#x in tree\n",
			       __func__, __LINE__, entry->gw);
			return -ESRCH;
		}

		pan_sw_l3_fib_h_tbl_del_entry(tn);
		hlist_del_init(&tn->lh);

		*match_id = tn->node->match_id;
		*mcam_idx = tn->node->mcam_idx;

		kfree(tn->node->entry);
		pan_free_matchid(&pan_rvu_gbl->rsrc, tn->node->match_id);

		kfree(tn->node);
		tn->node = NULL;

		if (hlist_empty(&fib_root_lh)) {
			if (!tn->l && !tn->r) {
				root = NULL;

				kfree(tn);
				return 0;
			}

			hlist_add_head(&tn->lh, &fib_root_lh);
			return 0;
		}

		if (tn != root) {
			kfree(tn);
			return 0;
		}

		next = hlist_entry(fib_root_lh.first,
				   struct pan_sw_l3_offl_tnode,
				   lh);
		hlist_del_init(&root->lh);
		next->l = root->l;
		next->r = root->r;
		kfree(root);
		root = next;
		return 0;
	}

	if (!root) {
		pr_err("%s:%d Root is not configured yet\n", __func__, __LINE__);
		return -EFAULT;
	}

	tn = pan_sw_l3_fib_h_tbl_lookup(entry);
	if (!tn) {
		pr_err("%s:%d Could not find entry->gw=%#x in tree\n",
		       __func__, __LINE__, entry->gw);
		return -ESRCH;
	}

	pan_sw_l3_fib_h_tbl_del_entry(tn);
	hlist_del_init(&tn->lh);

	sbit = 31;
	cnt = entry->dst_len;
	dst = entry->dst;
	walk = root;
	while (cnt) {
		u32 mask = 1 << sbit;

		if (dst & mask)
			walk = walk->r;
		else
			walk = walk->l;

		sbit--;
		cnt--;
	}

	if (!walk) {
		pr_err("%s:%d Could not find tnode with dst=%#x dst_len=%d\n",
		       __func__, __LINE__,
		       entry->dst, entry->dst_len);

		return -ESRCH;
	}

	if (!walk->node) {
		pr_err("%s:%d Could not find node with dst=%#x dst_len=%d\n",
		       __func__, __LINE__,
		       entry->dst, entry->dst_len);
		return -ESRCH;
	}

	*match_id = walk->node->match_id;
	*mcam_idx = walk->node->mcam_idx;

	entry = walk->node->entry;
	pr_debug("%s:%d Deleting dst=%#x dst_len=%d\n",
		 __func__, __LINE__,
		 entry->dst, entry->dst_len);

	kfree(entry);
	pan_free_matchid(&pan_rvu_gbl->rsrc, walk->node->match_id);
	kfree(walk->node);
	walk->node = NULL;

	if (!walk->node && !walk->l && !walk->r) {
		p = walk->p;
		if (p) {
			if (p->l == walk)
				p->l = NULL;
			else
				p->r = NULL;
		}
		walk->p = NULL;
		if (me_deleted)
			*me_deleted = true;
		kfree(walk);
	}

	return 0;
}

/* Use only in BH context */
struct net_device *
pan_sw_l3_route_lookup(u32 dst)
{
	struct pan_sw_l3_offl_tnode *walk, *cur = NULL;
	struct pan_rvu_gbl_t *pan_rvu_gbl;
	struct fib_entry *entry;
	struct net_device *dev;
	u32 sbit = 31;
	u16 pcifunc;
	u32 mask;
	int bit;

	dst = ntohl(dst);

	pan_rvu_gbl = pan_rvu_get_gbl();

	rcu_read_lock();

	walk = root;
	while (walk && sbit) {
		if (walk->node)
			cur = walk;

		mask = 1UL << sbit;
		bit = dst & mask;

		if (bit)
			walk = walk->r;
		else
			walk = walk->l;

		sbit--;
	}

	if (!cur) {
		pr_err("Failed to find route %#x\n", dst);
		rcu_read_unlock();
		return NULL;
	}

	if (cur) {
		entry = cur->node->entry;
		pcifunc = entry->port_id;
		dev = xa_load(&pan_rvu_gbl->pfunc2dev, pcifunc);
	}

	rcu_read_unlock();

	pr_debug("%s:%d Found route for %#x , dst=%#x dst_len=%d gw=%#x\n",
		 __func__, __LINE__,
		 dst, entry->dst, entry->dst_len, entry->gw);

	return dev;
}

static int pan_sw_l3_remove_one_fl_tb_entry(int mcam_idx, int match_id)
{
	struct pan_tuple tuple = { 0 };
	int err;

	pan_tuple_hash_set(&tuple, match_id);
	err = pan_fl_tbl_offl_del(&tuple);
	if (err) {
		pr_debug("%s:%d Failed to del tbl flow mcam=%d match_id=%d\n",
			 __func__, __LINE__, mcam_idx, match_id);
		return err;
	}

	return 0;
}

int pan_sw_l3_ev_enq(struct otx2_nic *otx2_nic, int cnt, struct fib_entry *fe)
{
	struct pan_sw_l3_ev *ev;
	int sz = sizeof(*ev) + cnt * sizeof(*fe);

	ev = kcalloc(1, sz, GFP_KERNEL);
	if (!ev)
		return -ENOMEM;

	INIT_LIST_HEAD(&ev->list);

	for (int i = 0; i < cnt; i++)
		ev->fe[i] = *(fe + i);

	ev->otx2_nic = otx2_nic;
	ev->cnt = cnt;
	spin_lock(&offl_l3_lock);
	list_add_tail(&ev->list, &pan_sw_l3_ev_lh);
	spin_unlock(&offl_l3_lock);

	queue_delayed_work(pan_sw_l3_fib_wq, &pan_sw_l3_fib_work,
			   msecs_to_jiffies(100));
	return 0;
}

static int
pan_sw_l3_process(struct otx2_nic *pf, u32 switch_id,
		  u16 cnt, struct fib_entry *entry,
		  bool *reshuffle)
{
	struct pan_sw_l3_offl_tnode *tnode, *tmp;
	struct pan_rvu_gbl_t *pan_rvu_gbl;
	int mcam_idx, match_id;
	bool me_deleted;
	int err;
	int i;

	*reshuffle = false;

	for (i = 0; i < cnt; i++, entry++) {
		switch (entry->cmd) {
		case OTX2_DEV_UP:
		case OTX2_FIB_ENTRY_ADD:
		case OTX2_FIB_ENTRY_REPLACE:
			tnode =	pan_sw_l3_tnode_alloc(pf, switch_id, entry);
			if (!tnode) {
				pr_err("%s:%d tnode creation failed for dst=%#x dst_len=%d gw=%#x\n",
				       __func__, __LINE__,
				       entry->dst, entry->dst_len, entry->gw);
				continue;
			}

			spin_lock(&offl_l3_lock);
			tmp = pan_sw_l3_fib_h_tbl_lookup(entry);
			if (tmp) {
				pr_debug("%s:%d dst=%#x dst_len=%d gw=%#x already exist\n",
					 __func__, __LINE__,
					 entry->dst, entry->dst_len, entry->gw);

				spin_unlock(&offl_l3_lock);
				kfree(tnode->node->entry);

				pan_rvu_gbl = pan_rvu_get_gbl();
				pan_free_matchid(&pan_rvu_gbl->rsrc, tnode->node->match_id);
				kfree(tnode->node);
				kfree(tnode);
				continue;
			}

			pan_sw_l3_route_add(tnode);

			spin_unlock(&offl_l3_lock);
			pr_debug("%s:%d dst=%#x dst_len=%d gw=%#x got added\n",
				 __func__, __LINE__,
				 entry->dst, entry->dst_len, entry->gw);
			*reshuffle = true;
			break;

		case OTX2_DEV_DOWN:
		case OTX2_FIB_ENTRY_DEL:
			spin_lock(&offl_l3_lock);
			tmp = pan_sw_l3_fib_h_tbl_lookup(entry);
			spin_unlock(&offl_l3_lock);
			if (!tmp) {
				pr_debug("%s:%d dst=%#x dst_len=%d gw=%#x does not exist\n",
					 __func__, __LINE__,
					 entry->dst, entry->dst_len, entry->gw);
				continue;
			}

			spin_lock(&offl_l3_lock);
			err = pan_sw_l3_route_del(entry, &mcam_idx, &match_id, &me_deleted);
			spin_unlock(&offl_l3_lock);

			if (!err) {
				err = pan_sw_l3_remove_one_fl_tb_entry(mcam_idx, match_id);
				if (err)
					pr_debug("%s:%d err in removing mcam=%d match_id=%d\n",
						 __func__, __LINE__, mcam_idx, match_id);
			}

			pr_debug("%s:%d dst=%#x dst_len=%d gw=%#x got deleted err=%d\n",
				 __func__, __LINE__,
				 entry->dst, entry->dst_len, entry->gw, err);
			*reshuffle = true;
			break;

		case OTX2_NEIGH_UPDATE:
			spin_lock(&offl_l3_lock);
			err = pan_sw_l3_neigh_update(entry);
			spin_unlock(&offl_l3_lock);
			break;
		}
	}

	return 0;
}

static int pan_sw_l3_ev_process(bool *reshuffle)
{
	struct pan_sw_l3_ev *ev;
	struct list_head local_lh;

	INIT_LIST_HEAD(&local_lh);

	spin_lock(&offl_l3_lock);
	list_splice_init(&pan_sw_l3_ev_lh, &local_lh);
	spin_unlock(&offl_l3_lock);

	if (list_empty(&local_lh))
		return -EFAULT;

	while (!list_empty(&local_lh)) {
		ev = list_first_entry_or_null(&local_lh, struct pan_sw_l3_ev,
					      list);
		list_del_init(&ev->list);
		pan_sw_l3_process(ev->otx2_nic, 0x12345, ev->cnt,
				  ev->fe, reshuffle);
		kfree(ev);
	}
	return 0;
}

static void pan_sw_l3_fib_entry_dump(struct pan_sw_l3_offl_node *node,
				     struct seq_file *m)
{
	struct fib_entry *entry = node->entry;

	if (entry->gw_valid) {
		seq_printf(m, "0.0.0.0\t\t%d\t%pI4h\t%pM\t%#x\t%d\t\t%d\n",
			   entry->dst_len, &entry->gw, entry->mac, node->port_id,
			   node->match_id, node->mcam_idx);

		return;
	}

	seq_printf(m, "%pI4h\t%d\t0.0.0.0\t\t%pM\t%#x\t%d\t\t%d\n",
		   &entry->dst, entry->dst_len, entry->mac, node->port_id,
		   node->match_id, node->mcam_idx);
}

static void
pan_sw_l3_offl_tnode_traverse(struct pan_sw_l3_offl_tnode *walk, struct seq_file *m)
{
	struct pan_sw_l3_offl_tnode *pos;

	if (!walk)
		return;

	pan_sw_l3_offl_tnode_traverse(walk->l, m);
	pan_sw_l3_offl_tnode_traverse(walk->r, m);

	if (!walk->node)
		return;

	if (walk != root) {
		pan_sw_l3_fib_entry_dump(walk->node, m);
		return;
	}

	hlist_for_each_entry(pos, &fib_root_lh, lh)
		pan_sw_l3_fib_entry_dump(pos->node, m);
}

static int pan_sw_l3_show(struct seq_file *m, void *v)
{
	seq_puts(m, "\n++++ routes +++\n\n");
	seq_puts(m, "Dest\t\tMask\tGW\t\tMAC\t\t\tPcifunc\tmatch_id\tmcam_idx\n");

	spin_lock(&offl_l3_lock);
	pan_sw_l3_offl_tnode_traverse(root, m);
	spin_unlock(&offl_l3_lock);

	return 0;
}
DEFINE_SHOW_ATTRIBUTE(pan_sw_l3);

static int pan_sw_l3_debugfs_add(void)
{
	struct dentry *pdir;
	struct dentry *file;

	pdir = pan_dbgfs_dir();
	if (!pdir) {
		pr_err("Could not create pan directory\n");
		return -ESRCH;
	}

	file = debugfs_create_file("route", 0400, pdir, NULL,
				   &pan_sw_l3_fops);
	if (!file) {
		pr_err("Could not create switch debugfs entry\n");
		return -EFAULT;
	}

	return 0;
}

static void pan_sw_l3_debugfs_remove(void)
{
	pan_dbgfs_rm_file("route");
}

int pan_sw_l3_init(void)
{
	int i;

	pan_sw_l3_fib_wq = alloc_workqueue("pan_sw_l3_fib_wq", 0, 0);

	otx2_nic = pan_rvu_get_pan_nic();

	for (i = 0; i < 33; i++)
		INIT_HLIST_HEAD(&fib_hnodes[i]);

	pan_sw_l3_debugfs_add();
	return 0;
}

void pan_sw_l3_deinit(void)
{
	bool me_deleted = false;

	pan_sw_l3_debugfs_remove();
	cancel_delayed_work_sync(&pan_sw_l3_fib_work);
	spin_lock(&offl_l3_lock);
	pan_sw_l3_fl_tbl_del_n_free_all(root, &me_deleted);
	pan_sw_l3_walk_n_free_empty_tnode(root);
	spin_unlock(&offl_l3_lock);
	pan_sw_l3_hw_npc_del_flows(marr, &lcnt);
}
