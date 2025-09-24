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

static LIST_HEAD(pan_sw_fl_lh);
static DEFINE_MUTEX(pan_sw_fl_lock);

static struct otx2_nic *otx2_nic;

struct pan_sw_fl {
	struct list_head		list;
	unsigned long			cookie;
	u16				mcam_idx[2];
	struct fl_tuple			ftuple;
	u32				port_id;
	u16				match_id[2];
	u64				features;
	struct rcu_head			rcu;
	bool				uni_di;
};

static struct pan_sw_fl *
__pan_sw_fl_entry_by_cookie(unsigned long cookie)
{
	struct pan_sw_fl *entry;

	list_for_each_entry(entry, &pan_sw_fl_lh, list)
		if (entry->cookie == cookie)
			return entry;

	return NULL;
}

static LIST_HEAD(pan_sw_fl_ev_lh);
struct pan_sw_fl_ev {
	struct list_head		list;
	unsigned long			cookie;
	struct fl_tuple			ftuple;
	u64				flags;
	u32				port_id;
};

static bool fl_wq_started;

static void pan_sw_fl_dwork(struct work_struct *dwork);
static DECLARE_DELAYED_WORK(pan_sw_fl_dwq, pan_sw_fl_dwork);

int
pan_sw_fl_ev_enq(struct otx2_nic *pf, u16 switch_id, u32 port_id,
		 struct fl_tuple *ftuple, u64 flags,
		 unsigned long cookie)
{
	struct pan_sw_fl_ev *fl_ev_node;

	fl_ev_node = kcalloc(1, sizeof(*fl_ev_node), GFP_KERNEL);
	if (!fl_ev_node)
		return -ENOMEM;

	fl_ev_node->flags = flags;
	fl_ev_node->port_id = port_id;
	fl_ev_node->ftuple = *ftuple;
	fl_ev_node->cookie = cookie;
	INIT_LIST_HEAD(&fl_ev_node->list);

	mutex_lock(&pan_sw_fl_lock);
	list_add_tail(&fl_ev_node->list, &pan_sw_fl_ev_lh);
	mutex_unlock(&pan_sw_fl_lock);

	if (!fl_wq_started) {
		schedule_delayed_work(&pan_sw_fl_dwq, msecs_to_jiffies(100));
		fl_wq_started = true;
	}

	return 0;
}

struct pan_sw_fl_stats_node {
	struct list_head list;
	unsigned long cookie;
	u16 mcam_idx[2];
	bool disabled;
	bool uni_di;
};

static LIST_HEAD(pan_sw_fl_stats_lh);
static DEFINE_MUTEX(pan_sw_fl_stats_lock);

static int
pan_sw_fl_stats_add_node(unsigned long cookie, u16 mcam_idx[2], bool uni_di)
{
	struct pan_sw_fl_stats_node *snode;

	snode = kcalloc(1, sizeof(*snode), GFP_KERNEL);
	if (!snode)
		return -ENOMEM;

	snode->cookie = cookie;
	snode->mcam_idx[0] = mcam_idx[0];
	snode->mcam_idx[1] = mcam_idx[1];
	snode->uni_di = uni_di;
	INIT_LIST_HEAD(&snode->list);

	mutex_lock(&pan_sw_fl_stats_lock);
	list_add_tail(&snode->list, &pan_sw_fl_stats_lh);
	mutex_unlock(&pan_sw_fl_stats_lock);

	return 0;
}

static int
pan_sw_fl_stats_node_mark_for_del(unsigned long cookie)
{
	struct pan_sw_fl_stats_node *snode;

	mutex_lock(&pan_sw_fl_stats_lock);
	list_for_each_entry(snode, &pan_sw_fl_stats_lh, list) {
		if (snode->cookie != cookie)
			continue;

		snode->disabled = true;
		mutex_unlock(&pan_sw_fl_stats_lock);
		return 0;
	}
	mutex_unlock(&pan_sw_fl_stats_lock);

	return -ESRCH;
}

static int pan_sw_fl_hw_del_n_free(u16 *mcam_idx, int cnt)
{
	struct npc_flow_del_n_free_req *req;
	int err;

	mutex_lock(&otx2_nic->mbox.lock);
	req = otx2_mbox_alloc_msg_npc_flow_del_n_free(&otx2_nic->mbox);
	if (!req) {
		mutex_unlock(&otx2_nic->mbox.lock);
		return -ENOMEM;
	}

	req->cnt = cnt;

	for (int i = 0; i < cnt; i++)
		req->entry[i] = mcam_idx[i];

	/* Send message to AF */
	err = otx2_sync_mbox_msg(&otx2_nic->mbox);
	mutex_unlock(&otx2_nic->mbox.lock);

	return err;
}

static int pan_sw_fl_del(unsigned long cookie)
{
	struct pan_tuple tuple = { 0 };
	struct pan_rvu_gbl_t *gbl;
	struct pan_sw_fl *fl;
	bool uni_di;
	int err;

	mutex_lock(&pan_sw_fl_lock);
	fl = __pan_sw_fl_entry_by_cookie(cookie);
	if (!fl) {
		mutex_unlock(&pan_sw_fl_lock);
		return -EINVAL;
	}
	list_del_init(&fl->list);
	mutex_unlock(&pan_sw_fl_lock);

	uni_di = fl->uni_di;

	pan_sw_fl_hw_del_n_free(fl->mcam_idx, uni_di ? 1 : 2);

	pan_tuple_hash_set(&tuple, fl->match_id[0]);
	err = pan_fl_tbl_offl_del(&tuple);
	if (err) {
		pr_err("%s:%d Failed to del tbl flow match_id=%d\n",
		       __func__, __LINE__, fl->match_id[0]);
		return err;
	}

	gbl = pan_rvu_get_gbl();
	if (fl->match_id[0])
		pan_free_matchid(&gbl->rsrc, fl->match_id[0]);

	if (uni_di)
		goto done;

	pan_tuple_hash_set(&tuple, fl->match_id[1]);
	err = pan_fl_tbl_offl_del(&tuple);
	if (err) {
		pr_err("%s:%d Failed to del tbl flow match_id=%d\n",
		       __func__, __LINE__, fl->match_id[1]);
		return err;
	}

	if (fl->match_id[1])
		pan_free_matchid(&gbl->rsrc, fl->match_id[1]);

done:
	pan_sw_fl_stats_node_mark_for_del(cookie);
	kfree_rcu(fl, rcu);
	return 0;
}

static int pan_sw_fl_hw_alloc_mcam_entry(u16 *mcam_idx, int cnt)
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
	req->count = cnt;

	if (otx2_sync_mbox_msg(&otx2_nic->mbox))
		goto fail_mbox_sync;

	rsp = (struct npc_mcam_alloc_entry_rsp *)otx2_mbox_get_rsp
		(&otx2_nic->mbox.mbox, 0, &req->hdr);

	flow_cfg->flow_ent[flow_cfg->max_flows++] = rsp->entry_list[0];

	for (int i = 0; i < cnt; i++)
		mcam_idx[i] = rsp->entry_list[i];

	mutex_unlock(&otx2_nic->mbox.lock);

	return 0;

fail_mbox_sync:
fail_alloc_entry:
	mutex_unlock(&otx2_nic->mbox.lock);
	return -EFAULT;
}

static int pan_sw_fl_hw_flow_install(u16 mcam_idx, struct fl_tuple *ftuple, u16 match_id,
				     struct pan_tuple *tuple)
{
	u8 pan_mac_mask[ETH_ALEN] = { 0xff, 0xff, 0xff, 0xff, 0xff, 0xff};
	struct npc_install_flow_req *req;
	struct flow_msg *pkt, *pmask;
	struct pan_rvu_gbl_t *gbl;
	u64 features;
	u64 npc_rx_features;
	int rc;

	gbl = pan_rvu_get_gbl();
	npc_rx_features = gbl->npc_rx_features;

	features = ftuple->features;

	if (!(features & (BIT_ULL(NPC_DMAC) | BIT_ULL(NPC_SMAC) |
			  BIT_ULL(NPC_DIP_IPV4) |
			  BIT_ULL(NPC_SIP_IPV4) |
			  BIT_ULL(NPC_ETYPE) |
			  BIT_ULL(NPC_IPPROTO_TCP) |
			  BIT_ULL(NPC_IPPROTO_UDP)))) {
		return 0;
	}

	mutex_lock(&otx2_nic->mbox.lock);
	req = otx2_mbox_alloc_msg_npc_install_flow(&otx2_nic->mbox);
	if (!req) {
		mutex_unlock(&otx2_nic->mbox.lock);
		return -ENOMEM;
	}

	pkt = &req->packet;
	pmask = &req->mask;

	/* nf tables does not support L2 offload
	 * So no need for mac addresses
	 */
	if (features & BIT_ULL(NPC_DMAC) & npc_rx_features) {
		ether_addr_copy(pkt->dmac, tuple->dmac);
		ether_addr_copy(pmask->dmac, pan_mac_mask);
		req->features |= BIT_ULL(NPC_DMAC);
	}

	if (features & BIT_ULL(NPC_SMAC) & npc_rx_features) {
		ether_addr_copy(pkt->smac, tuple->smac);
		ether_addr_copy(pmask->smac, pan_mac_mask);
		req->features |= BIT_ULL(NPC_SMAC);
	}

	if (features & BIT_ULL(NPC_DIP_IPV4) & npc_rx_features) {
		pkt->ip4dst = tuple->dst_ip4.s_addr;
		pmask->ip4dst = ftuple->m_ip4dst;
		req->features |= BIT_ULL(NPC_DIP_IPV4);
	}

	if (features & BIT_ULL(NPC_SIP_IPV4) & npc_rx_features) {
		pkt->ip4src = tuple->src_ip4.s_addr;
		pmask->ip4src = ftuple->m_ip4src;
		req->features |= BIT_ULL(NPC_SIP_IPV4);
	}

	if (features & BIT_ULL(NPC_ETYPE) & npc_rx_features) {
		pkt->etype = tuple->l3proto;
		pmask->etype = ftuple->m_eth_type;
		req->features |= BIT_ULL(NPC_ETYPE);
	}

	if (features & BIT_ULL(NPC_IPPROTO_TCP) & npc_rx_features) {
		req->features |= BIT_ULL(NPC_SPORT_TCP) |
			BIT_ULL(NPC_DPORT_TCP);
		pkt->sport = tuple->sport;
		pmask->sport = ftuple->m_sport;

		pkt->dport = tuple->dport;
		pmask->dport = ftuple->m_dport;
	}

	if (features & BIT_ULL(NPC_IPPROTO_UDP) & npc_rx_features) {
		req->features |= BIT_ULL(NPC_SPORT_UDP) |
			BIT_ULL(NPC_DPORT_UDP);
		pkt->sport = tuple->sport;
		pmask->sport = ftuple->m_sport;

		pkt->dport = tuple->dport;
		pmask->dport = ftuple->m_dport;
	}

	req->entry = mcam_idx;
	req->intf = NIX_INTF_RX;
	req->set_cntr = 1;
	req->op = NIX_RX_ACTIONOP_RSS;
	req->match_id = match_id;
	req->channel = 0;
	req->chan_mask = 0;
	req->set_chanmask = 1;

	/* Send message to AF */
	rc = otx2_sync_mbox_msg(&otx2_nic->mbox);
	if (rc) {
		mutex_unlock(&otx2_nic->mbox.lock);
		return -ENOMEM;
	}

	mutex_unlock(&otx2_nic->mbox.lock);
	return 0;
}

static void
pan_sw_fl_get_reply_ftuple(struct fl_tuple *tuple)
{
	u8 eth_addr[16];
	u32 ip_addr;
	u16 port;
	u16 pf;
	u8 bit;

	ether_addr_copy(eth_addr, tuple->dmac);
	ether_addr_copy(tuple->dmac, tuple->smac);
	ether_addr_copy(tuple->smac, eth_addr);

	ip_addr = tuple->ip4src;
	tuple->ip4src = tuple->ip4dst;
	tuple->ip4dst = ip_addr;

	port = tuple->sport;
	tuple->sport = tuple->dport;
	tuple->dport = port;

	pf = tuple->in_pf;
	tuple->in_pf = tuple->xmit_pf;
	tuple->xmit_pf = pf;

	bit = tuple->is_xdev_br;
	tuple->is_xdev_br = tuple->is_indev_br;
	tuple->is_indev_br = bit;
}

struct pan_sw_fl_mangle_info {
	u16 dmac[3];
	u16 smac[3];
	u32 sip;
	u32 dip;
	u16 sport;
	u16 dport;
};

static void
pan_sw_fl_mangle_parse(struct fl_tuple *ftuple, u64 *act,
		       struct pan_sw_fl_mangle_info *minfo)
{
	u16 m_smac[3] = { 0 };
	u16 m_dmac[3] = { 0 };

	u16 smac[3] = { 0 };
	u16 dmac[3] = { 0 };

	u32 m_sip = 0;
	u32 m_dip = 0;
	u32 sip = 0;
	u32 dip = 0;

	u16 m_sport = 0;
	u16 sport = 0;
	u16 m_dport = 0;
	u16 dport = 0;

	u32 val;
	u32 mask;
	u8 offset;

	*act = PAN_FL_TBL_ACT_L2_FWD;

	for (int i = 0; i < ftuple->mangle_cnt; i++) {
		if ((ftuple->mangle_map[FLOW_ACT_MANGLE_HDR_TYPE_ETH] & BIT(i))) {
			val = ftuple->mangle[i].val;
			mask = ftuple->mangle[i].mask;
			offset = ftuple->mangle[i].offset;

			*act = PAN_FL_TBL_ACT_L3_FWD;
			if (ftuple->is_xdev_br)
				*act = PAN_FL_TBL_ACT_L3_BR_FWD;

#define PAN_SW_FL_OFFSET_ETH_DMAC_31_0            0x0
#define PAN_SW_FL_OFFSET_ETH_DMAC_47_32_SMAC_15_0 0x4
#define PAN_SW_FL_OFFSET_ETH_SMAC_47_16           0x8
			switch (offset) {
			case PAN_SW_FL_OFFSET_ETH_DMAC_31_0:
				dmac[0] = FIELD_GET(GENMASK(15, 0), val);
				m_dmac[0] = FIELD_GET(GENMASK(15, 0), mask);

				dmac[1] = FIELD_GET(GENMASK(31, 16), val);
				m_dmac[1] = FIELD_GET(GENMASK(31, 16), mask);
				break;

			case PAN_SW_FL_OFFSET_ETH_DMAC_47_32_SMAC_15_0:
				dmac[2] = FIELD_GET(GENMASK(15, 0), val);
				m_dmac[2] = FIELD_GET(GENMASK(15, 0), mask);

				smac[0] = FIELD_GET(GENMASK(31, 16), val);
				m_smac[0] = FIELD_GET(GENMASK(31, 16), mask);
				break;
			case PAN_SW_FL_OFFSET_ETH_SMAC_47_16:
				smac[1] = FIELD_GET(GENMASK(15, 0), val);
				m_smac[1] = FIELD_GET(GENMASK(15, 0), mask);

				smac[2] = FIELD_GET(GENMASK(31, 16), val);
				m_smac[2] = FIELD_GET(GENMASK(31, 16), mask);
				break;
			}
			continue;
		}

		if ((ftuple->mangle_map[FLOW_ACT_MANGLE_HDR_TYPE_IP4] & BIT(i))) {
			val = ftuple->mangle[i].val;
			mask = ftuple->mangle[i].mask;
			offset = ftuple->mangle[i].offset;

			switch (offset) {
			case offsetof(struct iphdr, saddr):
				sip = val;
				m_sip = mask;

				*act = PAN_FL_TBL_ACT_L3_SNAT;
				if (ftuple->is_xdev_br)
					*act = PAN_FL_TBL_ACT_L3_BR_SNAT;

				break;

			case offsetof(struct iphdr, daddr):
				dip = val;
				m_dip = mask;

				*act = PAN_FL_TBL_ACT_L3_DNAT;
				if (ftuple->is_xdev_br)
					*act = PAN_FL_TBL_ACT_L3_BR_DNAT;
				break;
			}
			continue;
		}

		if ((ftuple->mangle_map[FLOW_ACT_MANGLE_HDR_TYPE_TCP] & BIT(i)) ||
		    (ftuple->mangle_map[FLOW_ACT_MANGLE_HDR_TYPE_UDP] & BIT(i))) {
			val = ftuple->mangle[i].val;
			mask = ftuple->mangle[i].mask;
			offset = ftuple->mangle[i].offset;

			switch (offset) {
			case offsetof(struct udphdr, dest):
				dport = val;
				m_dport = mask;

				*act = PAN_FL_TBL_ACT_L3_DNAPT;
				if (ftuple->is_xdev_br)
					*act = PAN_FL_TBL_ACT_L3_BR_DNAPT;

				break;

			case offsetof(struct udphdr, source):
				sport = val;
				m_sport = mask;

				*act = PAN_FL_TBL_ACT_L3_SNAPT;
				if (ftuple->is_xdev_br)
					*act = PAN_FL_TBL_ACT_L3_BR_SNAPT;
				break;
			}
			continue;
		}
	}

	ether_addr_copy((u8 *)minfo->dmac, (u8 *)dmac);
	ether_addr_copy((u8 *)minfo->smac, (u8 *)smac);
	minfo->sip = sip;
	minfo->dip = dip;
	minfo->sport = sport;
	minfo->dport = dport;
}

static int pan_sw_fl_tbl_entry_add(struct fl_tuple *ftuple, u16 match_id, u8 dir,
				   struct pan_sw_fl_mangle_info *minfo, u64 act,
				   struct pan_tuple *tuple)
{
	struct pan_rvu_gbl_t *pan_rvu_gbl;
	struct netdev_hw_addr *ha;
	struct pan_fl_tbl_res res = { 0 };
	struct pan_fl_tbl_opaque opq = { 0 };
	struct net_device *netdev;
	u64 features;
	int err;

	tuple->flags = PAN_TUPLE_FLAG_L3_PROTO_V4;

	pan_rvu_gbl = pan_rvu_get_gbl();
	netdev = xa_load(&pan_rvu_gbl->pfunc2dev,  ftuple->xmit_pf);
	if (netdev) {
		for_each_dev_addr(netdev, ha) {
			/* TODO: what if there are More than one mac address */
			ether_addr_copy(opq.eg_smac, ha->addr);
			ether_addr_copy(tuple->smac, ha->addr);
			break;
		}
	}

	res.act = act;
	res.uni_di = ftuple->uni_di;

	res.pcifuncoff = pan_rvu_pcifunc2_sq_off(ftuple->xmit_pf);
	res.opq = &opq;

	res.dir = dir;
	pan_tuple_hash_set(tuple, match_id);

	features = ftuple->features;

	if (features & BIT_ULL(NPC_DMAC))
		ether_addr_copy(tuple->dmac, ftuple->dmac);

	if (!is_zero_ether_addr(ftuple->smac)) {
		ether_addr_copy(tuple->smac, ftuple->smac);
		ether_addr_copy(opq.eg_smac, ftuple->smac);
	}

	if (features & BIT_ULL(NPC_SIP_IPV4))
		tuple->src_ip4.s_addr = ftuple->ip4src;

	if (features & BIT_ULL(NPC_DIP_IPV4))
		tuple->dst_ip4.s_addr = ftuple->ip4dst;

	if (features & BIT_ULL(NPC_ETYPE))
		tuple->l3proto = ftuple->eth_type;

	if (features & BIT_ULL(NPC_IPPROTO_TCP)) {
		tuple->sport = ftuple->sport;
		tuple->dport = ftuple->dport;
		tuple->l4proto = IPPROTO_TCP;
	}

	if (features & BIT_ULL(NPC_IPPROTO_UDP)) {
		tuple->sport = ftuple->sport;
		tuple->dport = ftuple->dport;
		tuple->l4proto = IPPROTO_UDP;
	}

	opq.eg_dmac_can_set = 1;

	if (dir == IP_CT_DIR_ORIGINAL) {
		if (!is_zero_ether_addr((u8 *)minfo->dmac) && (features & BIT_ULL(NPC_DMAC))) {
			ether_addr_copy(opq.eg_dmac, (u8 *)minfo->dmac);
			opq.eg_dmac_is_set = 1;
		}

		if (!is_zero_ether_addr((u8 *)minfo->smac))
			ether_addr_copy(opq.eg_smac, (u8 *)minfo->dmac);

		if (minfo->sip && (features & BIT_ULL(NPC_SIP_IPV4))) {
			switch (act) {
			case PAN_FL_TBL_ACT_L3_SNAT:
			case PAN_FL_TBL_ACT_L3_SNAPT:
				opq.eg_sip = minfo->sip;
				break;
			}
		}

		if (minfo->dip && (features & BIT_ULL(NPC_DIP_IPV4))) {
			switch (act) {
			case PAN_FL_TBL_ACT_L3_DNAT:
				break;
			}
		}

		if (minfo->dport &&
		    (features & (BIT_ULL(NPC_IPPROTO_TCP) | BIT_ULL(NPC_IPPROTO_UDP)))) {
			opq.eg_dport = minfo->dport;
		}

		if (minfo->sport &&
		    (features & (BIT_ULL(NPC_IPPROTO_TCP) | BIT_ULL(NPC_IPPROTO_UDP)))) {
			switch (act) {
			case PAN_FL_TBL_ACT_L3_SNAPT:
				opq.eg_sport = minfo->sport;
				break;
			}
		}
	} else {
		if (is_zero_ether_addr((u8 *)minfo->dmac) && (features & BIT_ULL(NPC_DMAC)))
			ether_addr_copy(opq.eg_dmac, (u8 *)minfo->dmac);

		if (is_zero_ether_addr((u8 *)minfo->smac))
			ether_addr_copy(opq.eg_smac, (u8 *)minfo->smac);

		if (minfo->sip && (features & BIT_ULL(NPC_SIP_IPV4))) {
			switch (act) {
			case PAN_FL_TBL_ACT_L3_SNAT:
			case PAN_FL_TBL_ACT_L3_SNAPT:
				opq.eg_dip = tuple->dst_ip4.s_addr;
				tuple->dst_ip4.s_addr = minfo->sip;
				break;
			}
		}

		if (minfo->dip && (features & BIT_ULL(NPC_DIP_IPV4))) {
			switch (act) {
			case PAN_FL_TBL_ACT_L3_DNAT:
				break;
			}
		}

		if (minfo->dport &&
		    (features & (BIT_ULL(NPC_IPPROTO_TCP) | BIT_ULL(NPC_IPPROTO_UDP)))) {
			opq.eg_dport = minfo->dport;
		}

		if (minfo->sport &&
		    (features & (BIT_ULL(NPC_IPPROTO_TCP) | BIT_ULL(NPC_IPPROTO_UDP)))) {
			switch (act) {
			case PAN_FL_TBL_ACT_L3_SNAPT:
				opq.eg_dport = tuple->dport;
				tuple->dport = minfo->sport;
				break;
			}
		}

	}

	/* MAC addr copied won't affect hash */
	err = pan_fl_tbl_offl_add(tuple, &res);
	if (err) {
		pr_debug("%s:%d Failed to add tbl flow\n", __func__, __LINE__);
		return err;
	}

	return 0;
}

static int pan_sw_fl_add(unsigned long cookie, struct fl_tuple *ftuple)
{
	struct pan_rvu_gbl_t *pan_rvu_gbl;
	struct fl_tuple reply_ftuple = { 0 };
	struct pan_sw_fl *nfl, *ofl;
	struct pan_sw_fl_mangle_info minfo = { 0 };
	struct pan_tuple tuple = { 0 };
	u64 act = 0;
	int match_id[2];
	u16  mcam_idx[2];
	bool uni_di;
	int rc;

	/* allocate memory for the new flow and it's node */
	nfl = kzalloc(sizeof(*nfl), GFP_KERNEL);
	if (!nfl)
		return -ENOMEM;

	mutex_lock(&pan_sw_fl_lock);
	ofl = __pan_sw_fl_entry_by_cookie(cookie);
	if (ofl) {
		mutex_unlock(&pan_sw_fl_lock);
		return 0;
	}
	uni_di = ftuple->uni_di;
	mutex_unlock(&pan_sw_fl_lock);

	pan_rvu_gbl = pan_rvu_get_gbl();

	pan_sw_fl_mangle_parse(ftuple, &act, &minfo);

	match_id[0] = pan_alloc_matchid(&pan_rvu_gbl->rsrc);
	rc = pan_sw_fl_tbl_entry_add(ftuple, match_id[0], IP_CT_DIR_ORIGINAL, &minfo, act, &tuple);
	if (rc) {
		pr_err("%s:%d Error to install flow tuple\n", __func__, __LINE__);
		goto free;
	}

	rc = pan_sw_fl_hw_alloc_mcam_entry(mcam_idx, uni_di ? 1 : 2);
	if (rc) {
		pr_err("%s:%d Error to alloc mcam idxs\n", __func__, __LINE__);
		goto free;
	}

	rc = pan_sw_fl_hw_flow_install(mcam_idx[0], ftuple, match_id[0], &tuple);
	if (rc) {
		pr_err("%s:%d Error to install original dir flow", __func__, __LINE__);
		goto free;
	}

	INIT_LIST_HEAD(&nfl->list);
	nfl->cookie = cookie;

	nfl->mcam_idx[0] = mcam_idx[0];
	nfl->match_id[0] = match_id[0];

	if (uni_di) {
		nfl->uni_di = uni_di;
		goto done;
	}

	match_id[1] = pan_alloc_matchid(&pan_rvu_gbl->rsrc);
	reply_ftuple = *ftuple;

	pan_sw_fl_get_reply_ftuple(&reply_ftuple);

	memset(&tuple, 0, sizeof(tuple));
	rc = pan_sw_fl_tbl_entry_add(&reply_ftuple, match_id[1], IP_CT_DIR_REPLY,
				     &minfo, act, &tuple);
	if (rc) {
		pr_err("%s:%d Error install flow tuple\n", __func__, __LINE__);
		goto free;
	}

	rc = pan_sw_fl_hw_flow_install(mcam_idx[1], &reply_ftuple, match_id[1], &tuple);
	if (rc) {
		pr_err("%s:%d Error to install reply dir flow", __func__, __LINE__);
		goto free;
	}

	nfl->mcam_idx[1] = mcam_idx[1];
	nfl->match_id[1] = match_id[1];

done:
	rc = pan_sw_fl_stats_add_node(cookie, mcam_idx, uni_di);
	if (rc) {
		pr_err("%s:%d Error to install stats node", __func__, __LINE__);
		goto free;
	}

	mutex_lock(&pan_sw_fl_lock);
	list_add_tail(&nfl->list, &pan_sw_fl_lh);
	mutex_unlock(&pan_sw_fl_lock);

	return 0;
free:
	pan_free_matchid(&pan_rvu_gbl->rsrc, match_id[0]);
	if (!uni_di)
		pan_free_matchid(&pan_rvu_gbl->rsrc, match_id[1]);
	pan_sw_fl_del(cookie);
	kfree_rcu(nfl, rcu);
	return rc;
}

static void pan_sw_fl_dwork(struct work_struct *dwork)
{
	struct pan_sw_fl_ev *ev_node;
	struct pan_sw_fl_stats_node *snode;
	struct swdev2af_notify_req *req;
	unsigned long cookie[64];
	unsigned long fcookie = 0;
	u16 mcam_idx[64][2];
	bool disabled[64];
	bool uni_di[64];
	LIST_HEAD(local_lh);
	int iter = 64;
	int ret, cnt = 0;

	INIT_LIST_HEAD(&local_lh);
	mutex_lock(&pan_sw_fl_lock);
	list_splice_init(&pan_sw_fl_ev_lh, &local_lh);
	mutex_unlock(&pan_sw_fl_lock);

	while (!list_empty(&local_lh)) {
		ev_node = list_first_entry_or_null(&local_lh, struct pan_sw_fl_ev, list);
		list_del_init(&ev_node->list);

		if (ev_node->flags & FL_ADD)
			pan_sw_fl_add(ev_node->cookie, &ev_node->ftuple);
		else if (ev_node->flags & FL_DEL)
			pan_sw_fl_del(ev_node->cookie);
		kfree(ev_node);
	}

	mutex_lock(&pan_sw_fl_stats_lock);
	if (list_empty(&pan_sw_fl_stats_lh)) {
		mutex_unlock(&pan_sw_fl_stats_lock);
		goto done;
	}

	while (iter--) {
		snode = list_first_entry_or_null(&pan_sw_fl_stats_lh,
						 struct pan_sw_fl_stats_node, list);
		if (!snode)
			break;

		if (fcookie) {
			if (snode->cookie == fcookie)
				break;
		} else {
			if (!snode->disabled)
				fcookie = snode->cookie;
		}

		list_del_init(&snode->list);
		cookie[cnt] = snode->cookie;
		mcam_idx[cnt][0] = snode->mcam_idx[0];
		mcam_idx[cnt][1] = snode->mcam_idx[1];
		disabled[cnt] = snode->disabled;
		uni_di[cnt] = snode->uni_di;
		cnt++;

		if (snode->disabled)
			kfree(snode);
		else
			list_add_tail(&snode->list, &pan_sw_fl_stats_lh);
	}
	mutex_unlock(&pan_sw_fl_stats_lock);

	mutex_lock(&otx2_nic->mbox.lock);
	req = otx2_mbox_alloc_msg_swdev2af_notify(&otx2_nic->mbox);
	if (!req) {
		mutex_unlock(&otx2_nic->mbox.lock);
		pr_err("Error to alloc memory for request\n");
		goto done;
	}
	req->msg_type = SWDEV2AF_MSG_TYPE_REFRESH_FL;
	req->cnt = cnt;

	for (int i = 0; i < cnt; i++) {
		req->fl[i].cookie = cookie[i];
		req->fl[i].mcam_idx[0] = mcam_idx[i][0];
		req->fl[i].mcam_idx[1] = mcam_idx[i][1];
		req->fl[i].uni_di = uni_di[i];
		req->fl[i].dis = disabled[i];
	}

	ret = otx2_sync_mbox_msg(&otx2_nic->mbox);
	if (ret)
		pr_err("Error to send fl stats sync\n");
	mutex_unlock(&otx2_nic->mbox.lock);

done:
	schedule_delayed_work(&pan_sw_fl_dwq, msecs_to_jiffies(1000));
}

int pan_sw_fl_init(void)
{
	otx2_nic = pan_rvu_get_pan_nic();
	return 0;
}

void pan_sw_fl_deinit(void)
{
	flush_delayed_work(&pan_sw_fl_dwq);
	cancel_delayed_work_sync(&pan_sw_fl_dwq);
}
