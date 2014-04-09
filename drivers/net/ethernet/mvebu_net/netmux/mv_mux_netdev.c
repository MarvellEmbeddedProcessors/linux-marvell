/*******************************************************************************
Copyright (C) Marvell International Ltd. and its affiliates

This software file (the "File") is owned and distributed by Marvell
International Ltd. and/or its affiliates ("Marvell") under the following
alternative licensing terms.  Once you have made an election to distribute the
File under one of the following license alternatives, please (i) delete this
introductory statement regarding license alternatives, (ii) delete the two
license alternatives that you have not elected to use and (iii) preserve the
Marvell copyright notice above.

********************************************************************************
Marvell GPL License Option

If you received this File from Marvell, you may opt to use, redistribute and/or
modify this File in accordance with the terms and conditions of the General
Public License Version 2, June 1991 (the "GPL License"), a copy of which is
available along with the File in the license.txt file or by writing to the Free
Software Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 or
on the worldwide web at http://www.gnu.org/licenses/gpl.txt.

THE FILE IS DISTRIBUTED AS-IS, WITHOUT WARRANTY OF ANY KIND, AND THE IMPLIED
WARRANTIES OF MERCHANTABILITY OR FITNESS FOR A PARTICULAR PURPOSE ARE EXPRESSLY
DISCLAIMED.  The GPL License provides additional details about this warranty
disclaimer.
*******************************************************************************/

#include "mvCommon.h"
#include <linux/platform_device.h>
#include <linux/module.h>

#ifdef CONFIG_ARCH_MVEBU
#include "mvNetConfig.h"
#else
#include "ctrlEnv/mvCtrlEnvSpec.h"
#endif

#include "mvDebug.h"
#include "mv_switch.h"

#include "mv_mux_netdev.h"
#include "mv_mux_tool.h"

static struct notifier_block mux_notifier_block __read_mostly;
static const struct net_device_ops mv_mux_netdev_ops;
static struct  mv_mux_switch_port  mux_switch_shadow;
struct  mv_mux_eth_port mux_eth_shadow[MV_ETH_MAX_PORTS];

/* switch functions that called from mux */
static const struct  mv_mux_switch_ops *switch_ops;

/* count mux devices number */
static int mux_init_cnt;

/* ppv2/neta functions that called from mux */
static struct  mv_mux_eth_ops	*eth_ops;

/* mux functions that called from switch */
static const struct  mv_switch_mux_ops mux_ops;

static inline struct net_device *mv_mux_rx_netdev_get(int port, struct sk_buff *skb);
static inline int mv_mux_rx_tag_remove(struct net_device *dev, struct sk_buff *skb);
static inline int mv_mux_tx_skb_tag_add(struct net_device *dev, struct sk_buff *skb);
static int mv_mux_netdev_delete_all(int port);

/*-----------------------------------------------------------------------------------------*/
/*----------------------------     MANAGER      -------------------------------------------*/
/*-----------------------------------------------------------------------------------------*/
static int mv_mux_mgr_create(char *name, int gbe_port, int group, MV_MUX_TAG *tag)
{
	struct net_device *mux_dev;
	unsigned char broadcast[MV_MAC_ADDR_SIZE] = {0xff, 0xff, 0xff, 0xff, 0xff, 0xff};
	char *unicast;

	mux_dev = mv_mux_netdev_alloc(name, group, tag);
	if (mux_dev == NULL) {
		printk(KERN_ERR "%s: mv_mux_netdev_alloc falied\n", __func__);
		return MV_FAIL;
	}
	mv_mux_netdev_add(gbe_port, mux_dev);

	/* update switch group's cookie for mux ops */
	if (switch_ops && switch_ops->group_cookie_set)
		switch_ops->group_cookie_set(group, mux_dev);

	/* update switch's DB with mux's MAC addresses (bcast, ucast) */
	unicast = mv_mux_get_mac(mux_dev);

	if (switch_ops && switch_ops->mac_addr_set) {
		switch_ops->mac_addr_set(group, unicast, 1);
		switch_ops->mac_addr_set(group, broadcast, 1);
	}

	return 0;
}
/*-----------------------------------------------------------------------------------------*/

static int mv_mux_mgr_init(MV_SWITCH_PRESET_TYPE preset, int vid, MV_TAG_TYPE tag_mode, int gbe_port)
{
	char name[7] = {0, 0, 0, 0, 0, 0, 0};
	MV_MUX_TAG tag;
	unsigned int g;

	for (g = 0; g < MV_SWITCH_DB_NUM; g++) {
		/* get tag data according to switch */
		if (switch_ops && switch_ops->tag_get)
			if (switch_ops->tag_get(g, tag_mode, preset, vid, &tag)) {
				/* group g enabled */
				sprintf(name, "mux%d", g);
				/* create new mux device */
				mv_mux_mgr_create(name, gbe_port, g, &tag);
			}

	}

	return 0;
}
/*-----------------------------------------------------------------------------------------*/

static int mv_mux_mgr_probe(int gbe_port)
{
	MV_TAG_TYPE tag_mode = mux_switch_shadow.tag_type;
	MV_SWITCH_PRESET_TYPE preset = mux_switch_shadow.preset;
	int vid = mux_switch_shadow.vid;

	/* config switch according to preset mode */
	if (switch_ops && switch_ops->preset_init)
		switch_ops->preset_init(tag_mode, preset, vid);

	/* update netdev port with tag type */
	mv_mux_tag_type_set(gbe_port, tag_mode);

	/* config mux interfaces according to preset mode */
	mv_mux_mgr_init(preset, vid, tag_mode, gbe_port);

	if (tag_mode != MV_TAG_TYPE_NONE)
		dev_set_promiscuity(mux_eth_shadow[gbe_port].root, 1);

	if (switch_ops && switch_ops->interrupt_unmask)
		switch_ops->interrupt_unmask();

	printk(KERN_ERR "port #%d establish switch connection\n\n", gbe_port);

	return 0;
}

/*-----------------------------------------------------------------------------------------*/
/*----------------------------    MUX DRIVER    -------------------------------------------*/
/*-----------------------------------------------------------------------------------------*/

/*-----------------------------------------------------------------------------------------*/
int mv_mux_switch_ops_set(const struct mv_mux_switch_ops *switch_ops_ptr)
{
	switch_ops = switch_ops_ptr;

	return 0;
}

static inline bool mv_mux_internal_switch(int port)
{
	/* note: in external switch - attach return false */
	return ((mux_switch_shadow.attach) && (mux_switch_shadow.gbe_port == port));
}

void mv_mux_shadow_print(int gbe_port)
{
	struct mv_mux_eth_port shadow;
	static const char * const tags[] = {"None", "mh", "dsa", "edas", "vlan"};

	if (mux_eth_shadow[gbe_port].root == NULL)
		printk(KERN_ERR "gbe port %d is not attached.\n", gbe_port);

	shadow = mux_eth_shadow[gbe_port];
		printk(KERN_ERR "\n");
		printk(KERN_ERR "port #%d: tag type=%s, switch_dev=0x%p, root_dev = 0x%p, flags=0x%x\n",
			gbe_port, tags[shadow.tag_type],
			shadow.switch_dev, shadow.root, (unsigned int)shadow.flags);

	mv_mux_netdev_print_all(gbe_port);
}

/*-----------------------------------------------------------------------------------------*/
void mv_mux_switch_attach(int gbe_port, int preset, int vid, int tag, int switch_port)
{
	/* allready attach */
	if (mux_switch_shadow.attach)
		return;

	mux_switch_shadow.tag_type = tag;
	mux_switch_shadow.preset = preset;
	mux_switch_shadow.vid = vid;
	mux_switch_shadow.switch_port = switch_port;
	mux_switch_shadow.gbe_port = gbe_port;
	mux_switch_shadow.attach = MV_TRUE;
	/* Update MTU when activating master interface */
	mux_switch_shadow.mtu = -1;

#ifdef CONFIG_MV_INCLUDE_SWITCH
	mv_switch_mux_ops_set(&mux_ops);
#endif

	if (mux_eth_shadow[gbe_port].root)
		/* gbe port already attached */
		mv_mux_mgr_probe(gbe_port);
}


void mv_mux_eth_attach(int port, struct net_device *root, struct mv_mux_eth_ops *ops)
{
	/* allready attach */
	if (mux_eth_shadow[port].root)
		return;

	/* update root device in shadow */
	mux_eth_shadow[port].root = root;

	/* update ops structure */
	eth_ops = ops;

	if (mux_switch_shadow.attach && (mux_switch_shadow.gbe_port == port))
		/* switch already attached */
		mv_mux_mgr_probe(port);
}
EXPORT_SYMBOL(mv_mux_eth_attach);

void mv_mux_eth_detach(int port)
{
	/* allready deattach */
	if (mux_eth_shadow[port].root == NULL)
		return;

	/* delete all attached mux devices */
	mv_mux_netdev_delete_all(port);

	/* clear port data */
	memset(&mux_eth_shadow[port], 0, sizeof(struct mv_mux_eth_port));
}
EXPORT_SYMBOL(mv_mux_eth_detach);
/*-----------------------------------------------------------------------------------------*/

int mv_mux_netdev_find(unsigned int dev_idx)
{
	int port;
	struct net_device *root;

	for (port = 0; port < MV_ETH_MAX_PORTS; port++) {
		root = mux_eth_shadow[port].root;

		if (root && (root->ifindex == dev_idx))
			return port;
	}
	return -1;
}
/*-----------------------------------------------------------------------------------------*/
int mv_mux_update_link(void *cookie, int link_up)
{
	struct net_device *mux_dev = (struct net_device *)cookie;

	(link_up) ? netif_carrier_on(mux_dev) : netif_carrier_off(mux_dev);

	return 0;
}

/*-----------------------------------------------------------------------------------------*/
static inline int mv_mux_get_tag_size(MV_TAG_TYPE type)
{
	static const int size_arr[] = {0, MV_ETH_MH_SIZE,
					MV_ETH_DSA_SIZE,
					MV_TAG_TYPE_EDSA,
					MV_TAG_TYPE_VLAN};
	return size_arr[type];
}
/*-----------------------------------------------------------------------------------------*/

int mv_mux_rx(struct sk_buff *skb, int port, struct napi_struct *napi)

{
	struct net_device *mux_dev;
	int    len;

	mux_dev = mv_mux_rx_netdev_get(port, skb);

	if (mux_dev == NULL)
		goto out;

	/* mux device is down */
	if (!(mux_dev->flags & IFF_UP))
		goto out1;

	/* remove tag*/
	len = mv_mux_rx_tag_remove(mux_dev, skb);
	mux_dev->stats.rx_packets++;
	mux_dev->stats.rx_bytes += skb->len - len;


#ifdef CONFIG_MV_ETH_DEBUG_CODE
	if (mux_eth_shadow[port].flags & MV_MUX_F_DBG_RX) {
		struct mux_netdev *pmux_priv = MV_MUX_PRIV(mux_dev);
		pr_err("\n%s - %s: port=%d, cpu=%d\n",
			mux_dev->name, __func__, pmux_priv->port, smp_processor_id());
		/* mv_eth_skb_print(skb); */
		mvDebugMemDump(skb->data, 64, 1);
	}
#endif /* CONFIG_MV_ETH_DEBUG_CODE */

/*
#ifdef ETH_SKB_DEBUG
		mv_eth_skb_check(skb);
#endif
*/
	skb->protocol = eth_type_trans(skb, mux_dev);

	if (mux_dev->features & NETIF_F_GRO) {
		/*
		TODO update mux priv gro counters
		STAT_DBG(pp->stats.rx_gro++);
		STAT_DBG(pp->stats.rx_gro_bytes += skb->len);
	`	*/
		return napi_gro_receive(napi, skb);
	}


	return netif_receive_skb(skb);

out1:
	mux_dev->stats.rx_dropped++;
out:
	kfree_skb(skb);
	return 0;
}
EXPORT_SYMBOL(mv_mux_rx);

/*-----------------------------------------------------------------------------------------*/

static int mv_mux_xmit(struct sk_buff *skb, struct net_device *dev)
{
	struct mux_netdev *pmux_priv = MV_MUX_PRIV(dev);

	if (!(netif_running(dev))) {
		printk(KERN_ERR "!netif_running() in %s\n", __func__);
		goto out;
	}

	if (mv_mux_tx_skb_tag_add(dev, skb)) {
		printk(KERN_ERR "%s: mv_mux_tx_skb_tag_add failed.\n", __func__);
		goto out;
	}

#ifdef CONFIG_MV_ETH_DEBUG_CODE
	if (mux_eth_shadow[pmux_priv->port].flags & MV_MUX_F_DBG_TX) {
		pr_err("\n%s - %s_%lu: port=%d, cpu=%d, in_intr=0x%lx\n",
			dev->name, __func__, dev->stats.tx_packets, pmux_priv->port,
			smp_processor_id(), in_interrupt());
		/* mv_eth_skb_print(skb); */
		mvDebugMemDump(skb->data, 64, 1);
	}
#endif /* CONFIG_MV_ETH_DEBUG_CODE */

	dev->stats.tx_packets++;
	dev->stats.tx_bytes += skb->len;

	/* assign the packet to the hw interface */
	skb->dev = mux_eth_shadow[pmux_priv->port].root;

	/* mark skb as tagged skb */
	MV_MUX_SKB_TAG_SET(skb);

	/*tell Linux to pass it to its device */
	return dev_queue_xmit(skb);

out:
	dev->stats.tx_dropped++;
	dev_kfree_skb_any(skb);
	return NETDEV_TX_OK;
}

/*-----------------------------------------------------------------------------------------*/
/* Return mux device mac address							   */
/*-----------------------------------------------------------------------------------------*/
char *mv_mux_get_mac(struct net_device *mux_dev)
{

	if (!mux_dev) {
		printk(KERN_ERR "%s: mux net device is NULL.\n", __func__);
		return NULL;
	}

	return mux_dev->dev_addr;
}
/*-----------------------------------------------------------------------------------------*/

static void mv_mux_set_rx_mode(struct net_device *dev)
{
/*
	printk(KERN_ERR "Invalid operation %s is virtual interface.\n", dev->name);
*/
}

/*-----------------------------------------------------------------------------------------*/

void mv_mux_change_rx_flags(struct net_device *mux_dev, int flags)
{
	struct mux_netdev *pmux_priv = MV_MUX_PRIV(mux_dev);

	if (mv_mux_internal_switch(pmux_priv->port))
		if (switch_ops && switch_ops->promisc_set)
			switch_ops->promisc_set(pmux_priv->idx, (mux_dev->flags & IFF_PROMISC) ? 1 : 0);
}

/*-----------------------------------------------------------------------------------------*/
static int mv_mux_ioctl(struct net_device *dev, struct ifreq *ifr, int cmd)
{
	/*TODO compleate implementation*/
	printk(KERN_ERR "Not supported yet.\n");
	return 0;
}

/*-----------------------------------------------------------------------------------------*/

static void mv_mux_switch_mtu_update(int mtu)
{
	int pkt_size, tag_size = 0;
	MV_TAG_TYPE tag_type =  mux_switch_shadow.tag_type;

	if (tag_type == MV_TAG_TYPE_MH)
		tag_size = MV_ETH_MH_SIZE;
	else if (tag_type == MV_TAG_TYPE_DSA)
		tag_size = MV_ETH_DSA_SIZE;

	pkt_size = mtu + tag_size + MV_ETH_ALEN + MV_ETH_VLAN_SIZE + MV_ETH_CRC_SIZE;

	if (switch_ops && switch_ops->jumbo_mode_set)
		switch_ops->jumbo_mode_set(pkt_size);
}

/*-----------------------------------------------------------------------------------------*/

int mv_mux_close(struct net_device *dev)
{
	struct mux_netdev *pmux_priv = MV_MUX_PRIV(dev);
	struct net_device *root = mux_eth_shadow[pmux_priv->port].root;

	netif_stacked_transfer_operstate(root, dev);
	netif_tx_stop_all_queues(dev);

	if (mv_mux_internal_switch(pmux_priv->port))
		if (switch_ops && switch_ops->group_disable)
			switch_ops->group_disable(pmux_priv->idx);

	printk(KERN_NOTICE "%s: stopped\n", dev->name);

	return MV_OK;
}

/*-----------------------------------------------------------------------------------------*/
int mv_mux_open(struct net_device *dev)
{
	struct mux_netdev *pmux_priv = MV_MUX_PRIV(dev);
	struct net_device *root = mux_eth_shadow[pmux_priv->port].root;

	if (!root) {
		printk(KERN_ERR "%s:Invalid operation, set master before up.\n", __func__);
		return MV_ERROR;
	}

	/* if master is close */
	if (!(root->flags & IFF_UP)) {
		printk(KERN_ERR "%s:Invalid operation, port %d is down.\n", __func__, pmux_priv->port);
		return MV_ERROR;
	}

	netif_stacked_transfer_operstate(root, dev);
	netif_tx_wake_all_queues(dev);

	if (dev->mtu > root->mtu)
		dev->mtu = root->mtu;

	if (mv_mux_internal_switch(pmux_priv->port))
		if (switch_ops && switch_ops->group_enable)
			switch_ops->group_enable(pmux_priv->idx);

	printk(KERN_NOTICE "%s: started\n", dev->name);

	return MV_OK;

}

/*-----------------------------------------------------------------------------------------*/

static int mv_mux_set_mac(struct net_device *mux_dev, void *addr)
{
	struct mux_netdev *pmux_priv = MV_MUX_PRIV(mux_dev);

	u8 *mac = &(((u8 *)addr)[2]);  /* skip on first 2B (ether HW addr type) */

	mv_mux_close(mux_dev);

	/*TODO: update parser/PNC - mac filtering*/

	if (mv_mux_internal_switch(pmux_priv->port))
		if (switch_ops && switch_ops->mac_addr_set) {

			/* delete old mac */
			if (switch_ops->mac_addr_set(pmux_priv->idx, mux_dev->dev_addr, 0))
				return MV_ERROR;

			/* set new mac */
			if (switch_ops->mac_addr_set(pmux_priv->idx, mac, 1))
				return MV_ERROR;
		}

	memcpy(mux_dev->dev_addr, mac, ETH_ALEN);

	mv_mux_open(mux_dev);

	return 0;
}

/*-----------------------------------------------------------------------------------------*/

int mv_mux_mtu_change(struct net_device *mux_dev, int mtu)
{
	struct mux_netdev *pmux_priv = MV_MUX_PRIV(mux_dev);
	struct net_device *root = mux_eth_shadow[pmux_priv->port].root;

	if (root->mtu < mtu) {
		printk(KERN_ERR "Invalid mtu value.\n");
		return MV_ERROR;
	}

	mux_dev->mtu = mtu;
	return MV_OK;
}

/*-----------------------------------------------------------------------------------------*/
/* Create new mux device, if device is allready exist just change tag value                */
/* mv_mux_netdev_add should called after mv_mux_netdev_alloc                               */
/*-----------------------------------------------------------------------------------------*/
struct net_device *mv_mux_netdev_alloc(char *name, int idx, MV_MUX_TAG *tag_cfg)
{
	struct net_device *mux_dev;
	struct mux_netdev *pmux_priv;

	if (name == NULL) {
		printk(KERN_ERR "%s: mux net device name is missig.\n", __func__);
		return NULL;
	}

	mux_dev = dev_get_by_name(&init_net, name);


	if (!mux_dev) {
		/* new net device */
		mux_dev = alloc_netdev(sizeof(struct mux_netdev), name, ether_setup);
		if (!mux_dev) {
			printk(KERN_ERR "%s: out of memory, net device allocation failed.\n", __func__);
			return NULL;
		}
		/* allocation succeed */
		mux_dev->irq = NO_IRQ;
		/* must set netdev_ops before registration */
		mux_dev->netdev_ops = &mv_mux_netdev_ops;

		if (register_netdev(mux_dev)) {
			printk(KERN_ERR "%s: failed to register %s\n", __func__, mux_dev->name);
			free_netdev(mux_dev);
			return NULL;
		}

	} else
		dev_put(mux_dev);

	pmux_priv = MV_MUX_PRIV(mux_dev);

	if (tag_cfg == NULL) {
		memset(pmux_priv, 0, sizeof(struct mux_netdev));
		pmux_priv->port = -1;
		pmux_priv->next = NULL;
	} else{
		/* next, pp not changed*/
		pmux_priv->tx_tag = tag_cfg->tx_tag;
		pmux_priv->rx_tag_ptrn = tag_cfg->rx_tag_ptrn;
		pmux_priv->rx_tag_mask = tag_cfg->rx_tag_mask;
	}
	pmux_priv->idx = idx;
	return mux_dev;
}

/*-----------------------------------------------------------------------------------------*/
/* Init mux device features								   */
/*-----------------------------------------------------------------------------------------*/
static inline void mv_mux_init_features(struct net_device *mux_dev)
{
	struct mux_netdev *pmux_priv = MV_MUX_PRIV(mux_dev);
	struct net_device *root = mux_eth_shadow[pmux_priv->port].root;

	mux_dev->features = root->features;
	mux_dev->hw_features = root->hw_features & ~NETIF_F_RXCSUM;
	mux_dev->wanted_features = root->wanted_features;
	mux_dev->vlan_features = root->vlan_features;
}
/*-----------------------------------------------------------------------------------------*/
/* mv_mux_transfer_features								   */
/* update features when root features are changed					   */
/*-----------------------------------------------------------------------------------------*/
static void mv_mux_transfer_features(struct net_device *root, struct net_device *mux_dev)
{
	mux_dev->features &= ~NETIF_F_RXCSUM;
	mux_dev->features |=  (root->features & NETIF_F_RXCSUM);

	mux_dev->features &= ~NETIF_F_IP_CSUM;
	mux_dev->features |=  (root->features & NETIF_F_IP_CSUM);

	mux_dev->hw_features &= ~NETIF_F_IP_CSUM;
	mux_dev->hw_features |=  (root->features & NETIF_F_IP_CSUM);

	mux_dev->features &= ~NETIF_F_TSO;
	mux_dev->features |=  (root->features & NETIF_F_TSO);

	mux_dev->hw_features &= ~NETIF_F_TSO;
	mux_dev->hw_features |=  (root->features & NETIF_F_TSO);

	mux_dev->features &= ~NETIF_F_SG;
	mux_dev->features |=  (root->features & NETIF_F_SG);

	mux_dev->hw_features &= ~NETIF_F_SG;
	mux_dev->hw_features |=  (root->features & NETIF_F_SG);
}
/*----------------------------------------------------------------------------------------*/
/* Function attache mux device to root device,						  */
/* Set mux mac address and features according to root device				  */
/*----------------------------------------------------------------------------------------*/
static struct net_device *mv_mux_netdev_init(int port, struct net_device *mux_dev)
{
	struct mux_netdev *pmux_priv;
	struct net_device *root = mux_eth_shadow[port].root;
	int tag_type = mux_eth_shadow[port].tag_type;

	if (root == NULL)
		return NULL;
/*
	if (pp && !(pp->flags & MV_ETH_F_CONNECT_LINUX)) {
		printk(KERN_ERR "%s: root device is not connect to linux.\n", __func__);
		return NULL;
	}
*/
	if (!mux_dev) {
		printk(KERN_ERR "%s: mux net device is NULL.\n", __func__);
		return NULL;
	}

	/* set skb header size , avoid from skb reallocation*/
	mux_dev->hard_header_len = root->hard_header_len +
					mv_mux_get_tag_size(tag_type);

	/* Copy MAC address and MTU from root netdevice */
	mux_dev->mtu = root->mtu;
	pmux_priv = MV_MUX_PRIV(mux_dev);
	pmux_priv->port = port;
	memcpy(mux_dev->dev_addr, root->dev_addr, MV_MAC_ADDR_SIZE);

	/* TODO: handle features */
	mv_mux_init_features(mux_dev);

	SET_ETHTOOL_OPS(mux_dev, &mv_mux_tool_ops);

	return mux_dev;
}
/*-----------------------------------------------------------------------------------------*/
struct net_device *mv_mux_switch_ptr_get(int port)
{
	return mux_eth_shadow[port].switch_dev;
}
/*-----------------------------------------------------------------------------------------*/
int mv_mux_tag_type_get(int port)
{
	return mux_eth_shadow[port].tag_type;
}

/*-----------------------------------------------------------------------------------------*/

struct net_device *mv_mux_netdev_add(int port, struct net_device *mux_dev)
{
	struct net_device *dev_temp;
	struct net_device *switch_dev;

	struct mux_netdev *pdev;

	if (mux_eth_shadow[port].root == NULL)
		return NULL;

	mux_dev = mv_mux_netdev_init(port, mux_dev);

	if (mux_dev == NULL)
		return NULL;

	switch_dev = mux_eth_shadow[port].switch_dev;

	if (switch_dev == NULL) {
		/* First tag netdev */
		mux_eth_shadow[port].switch_dev = mux_dev;
	} else {
		pdev = MV_MUX_PRIV(switch_dev);
		while (pdev->next != NULL) {
			dev_temp = pdev->next;
			pdev = MV_MUX_PRIV(dev_temp);
		}
		pdev->next = mux_dev;
	}

	if (!mux_init_cnt)
		if (register_netdevice_notifier(&mux_notifier_block) < 0)
			unregister_netdevice_notifier(&mux_notifier_block);

	mux_init_cnt++;

	return mux_dev;
}

/*-----------------------------------------------------------------------------------------*/
int mv_mux_tag_type_set(int port, int type)
{
	unsigned int flgs;
	struct net_device *root;


	if ((type < MV_TAG_TYPE_NONE) || (type >= MV_TAG_TYPE_LAST)) {
		printk(KERN_INFO "%s: Invalid tag type %d\n", __func__, type);
		return MV_ERROR;
	}
	root = mux_eth_shadow[port].root;
	/* port not initialized */
	if (root == NULL)
		return MV_ERROR;

	/* No change in tag type */
	if (mux_eth_shadow[port].tag_type == type)
		return MV_OK;

	flgs = root->flags;

	if (flgs & IFF_UP) {
		printk(KERN_ERR "%s: root device (%s) must stopped before.\n", __func__, root->name);
		return MV_ERROR;
	}

	/* delete all attached virtual interfaces */
	if (mv_mux_netdev_delete_all(port))
		return MV_ERROR;

	mux_eth_shadow[port].tag_type = type;

	if (eth_ops && eth_ops->set_tag_type)
		eth_ops->set_tag_type(port, mux_eth_shadow[port].tag_type);

	return MV_OK;
}

/*-----------------------------------------------------------------------------------------*/
/* Delete mux device                                                                       */
/*	remove device from port linked list						   */
/*	free mux device                                                                    */
/*-----------------------------------------------------------------------------------------*/

int mv_mux_netdev_delete(struct net_device *mux_dev)
{
	struct net_device *pdev_curr, *pdev_prev = NULL;
	struct mux_netdev *pdev_tmp_curr, *pdev_tmp_prev, *pdev;
	struct net_device *root;
	int flgs, port;

	if (mux_dev == NULL) {
		printk(KERN_ERR "%s: mux net device is NULL.\n", __func__);
		return MV_ERROR;
	}
	pdev = MV_MUX_PRIV(mux_dev);
	port = pdev->port;

	root = mux_eth_shadow[pdev->port].root;

	/*not attached to gbe port*/
	if (root == NULL) {
		synchronize_net();
		unregister_netdev(mux_dev);
		free_netdev(mux_dev);
		/*
		we don't need to decrease here mux_init_cnt
		mux_init_cnt incease only in mv_mux_netdev_add
		when mux attached to gbe port
		*/
		return MV_OK;
	}

	flgs = mux_dev->flags;
	if (flgs & IFF_UP) {
		printk(KERN_ERR "%s: root device (%s) must stopped before.\n", __func__, root->name);
		return MV_ERROR;
	}

	pdev_curr = mux_eth_shadow[port].switch_dev;

	while (pdev_curr != NULL) {

		pdev_tmp_curr = MV_MUX_PRIV(pdev_curr);

		if (pdev_curr == mux_dev) {
			if (pdev_curr == mux_eth_shadow[port].switch_dev) {
				/* first element*/
				mux_eth_shadow[port].switch_dev = pdev_tmp_curr->next;
			} else {
				pdev_tmp_prev = MV_MUX_PRIV(pdev_prev);
				pdev_tmp_prev->next = pdev_tmp_curr->next;
			}
			/* delet current */
			synchronize_net();
			unregister_netdev(mux_dev);
			printk(KERN_ERR "%s has been removed.\n", mux_dev->name);
			free_netdev(mux_dev);

			mux_init_cnt--;

			if (!mux_init_cnt)
				unregister_netdevice_notifier(&mux_notifier_block);

			return MV_OK;

		} else {
			pdev_prev = pdev_curr;
			pdev_curr = pdev_tmp_curr->next;
		}
	}
	/* mux_dev not found */
	return MV_ERROR;
}

/*-----------------------------------------------------------------------------------------*/
static int mv_mux_netdev_delete_all(int port)
{
	/* delete all attached mux devices */
	struct net_device *mux_dev;

	if (mux_eth_shadow[port].root == NULL)
		return MV_ERROR;

	/* delete all attached mux devices */
	mux_dev = mux_eth_shadow[port].switch_dev;
	while (mux_dev) {
		if (mv_mux_netdev_delete(mux_dev))
			return MV_ERROR;

		mux_dev = mux_eth_shadow[port].switch_dev;
	}

	return MV_OK;
}

/*-----------------------------------------------------------------------------------------*/
static int mux_device_event(struct notifier_block *unused, unsigned long event, void *ptr)
{
	struct net_device *mux_dev, *dev = ptr;
	struct mux_netdev *pdev_priv;
	int tag_type;
	int port = 0;
	int flgs;

	/*recognize if marvell event */
	port = mv_mux_netdev_find(dev->ifindex);

	if (port == -1)
		goto out;

	tag_type = mux_eth_shadow[port].tag_type;

	/* exit - if transparent mode */
	if (mv_mux_internal_switch(port) && (tag_type == MV_TAG_TYPE_NONE))
		goto out;

	switch (event) {

	case NETDEV_CHANGE:
		mux_dev = mux_eth_shadow[port].switch_dev;
		while (mux_dev != NULL) {
			pdev_priv = MV_MUX_PRIV(mux_dev);
			if (mv_mux_internal_switch(port)) {
				/* In case of internal switch, link is determined by switch */
				if (switch_ops && switch_ops->link_status_get) {
					int link_up = switch_ops->link_status_get(pdev_priv->idx);
					mv_mux_update_link(mux_dev, link_up);
				}
			} else {
				/* In case of external switch, propagate real device link state to mux devices */
				/* change state*/
				netif_stacked_transfer_operstate(dev, mux_dev);
			}
			mux_dev = pdev_priv->next;
		}
		break;

	case NETDEV_CHANGEADDR:
		/* Propagate real device mac adress to mux devices */
		mux_dev = mux_eth_shadow[port].switch_dev;

		while (mux_dev != NULL) {
			pdev_priv = MV_MUX_PRIV(mux_dev);
			/* May be called without an actual change */
			if (!compare_ether_addr(mux_dev->dev_addr, dev->dev_addr)) {
				mux_dev = pdev_priv->next;
				continue;
			}
			memcpy(mux_dev->dev_addr, dev->dev_addr, ETH_ALEN);
			mux_dev = pdev_priv->next;
		}
		break;

	case NETDEV_CHANGEMTU:
		mux_dev = mux_eth_shadow[port].switch_dev;

		while (mux_dev != NULL) {
			pdev_priv = MV_MUX_PRIV(mux_dev);
			dev_set_mtu(mux_dev, dev->mtu);
			mux_dev = pdev_priv->next;
		}

		if (mv_mux_internal_switch(port)) {
			mux_switch_shadow.mtu = dev->mtu;
			mv_mux_switch_mtu_update(dev->mtu);
		}

		break;

	case NETDEV_DOWN:
		/* Master down - Put all mux devices for this dev in the down state too.  */
		mux_dev = mux_eth_shadow[port].switch_dev;

		while (mux_dev != NULL) {
			pdev_priv = MV_MUX_PRIV(mux_dev);
			flgs = mux_dev->flags;
			if (!(flgs & IFF_UP)) {
				mux_dev = pdev_priv->next;
				continue;
			}
			/* dev_change_flags call to mv_mux_close*/
			dev_change_flags(mux_dev, flgs & ~IFF_UP);
			mux_dev = pdev_priv->next;
		}
		break;

	case NETDEV_UP:
		/* Check for MTU updates */
		if (mv_mux_internal_switch(port) &&
			((mux_switch_shadow.mtu == -1) || (mux_switch_shadow.mtu > dev->mtu))) {
				mux_switch_shadow.mtu = dev->mtu;
				mv_mux_switch_mtu_update(dev->mtu);
		}

		break;

	case NETDEV_FEAT_CHANGE:
		/* Master features changed - Propagate device features to underlying device */
		mux_dev = mux_eth_shadow[port].switch_dev;
		while (mux_dev != NULL) {
			pdev_priv = MV_MUX_PRIV(mux_dev);
			mv_mux_transfer_features(mux_eth_shadow[port].root, mux_dev);
			mux_dev = pdev_priv->next;
		}
		break;
	} /*switch*/
out:
	return NOTIFY_DONE;
}
/*-----------------------------------------------------------------------------------------*/
static struct notifier_block mux_notifier_block __read_mostly = {
	.notifier_call = mux_device_event,
};
/*-----------------------------------------------------------------------------------------*/

bool mv_mux_netdev_link_status(struct net_device *dev)
{
	return netif_carrier_ok(dev) ? true : false;
}

/*-----------------------------------------------------------------------------------------*/

void mv_mux_vlan_set(MV_MUX_TAG *mux_cfg, unsigned int vid)
{

	mux_cfg->tx_tag.vlan = MV_32BIT_BE((MV_VLAN_TYPE << 16) | vid);
	mux_cfg->rx_tag_ptrn.vlan = MV_32BIT_BE((MV_VLAN_TYPE << 16) | vid);

	/*mask priority*/
	mux_cfg->rx_tag_mask.vlan = MV_32BIT_BE(0xFFFF0FFF);

	mux_cfg->tag_type = MV_TAG_TYPE_VLAN;
}

/*-----------------------------------------------------------------------------------------*/
void mv_mux_cfg_get(struct net_device *mux_dev, MV_MUX_TAG *mux_cfg)
{
	if (mux_dev) {
		struct mux_netdev *pmux_priv;
		pmux_priv = MV_MUX_PRIV(mux_dev);
		mux_cfg->tx_tag = pmux_priv->tx_tag;
		mux_cfg->rx_tag_ptrn = pmux_priv->rx_tag_ptrn;
		mux_cfg->rx_tag_mask = pmux_priv->rx_tag_mask;
	} else
		memset(mux_cfg, 0, sizeof(MV_MUX_TAG));
}
/*-----------------------------------------------------------------------------------------*/

static inline struct net_device *mv_mux_mh_netdev_get(int port, MV_TAG *tag)
{
	struct net_device *dev = mux_eth_shadow[port].switch_dev;
	struct mux_netdev *pdev;

	while (dev != NULL) {
		pdev = MV_MUX_PRIV(dev);
		if ((tag->mh & pdev->rx_tag_mask.mh) == pdev->rx_tag_ptrn.mh)
			return dev;

		dev = pdev->next;
	}
	printk(KERN_ERR "%s: MH=0x%04x match no interfaces\n", __func__, tag->mh);
	return NULL;
}

/*-----------------------------------------------------------------------------------------*/

static inline struct net_device *mv_mux_vlan_netdev_get(int port, MV_TAG *tag)
{
	struct net_device *dev = mux_eth_shadow[port].switch_dev;
	struct mux_netdev *pdev;

	while (dev != NULL) {
		pdev = MV_MUX_PRIV(dev);
#ifdef CONFIG_MV_ETH_DEBUG_CODE
		if (mux_eth_shadow[port].flags & MV_MUX_F_DBG_RX)
			printk(KERN_ERR "pkt tag = 0x%x, rx_tag_ptrn = 0x%x, rx_tag_mask = 0x%x\n",
				 tag->vlan, pdev->rx_tag_ptrn.vlan, pdev->rx_tag_mask.vlan);
#endif
		if ((tag->vlan & pdev->rx_tag_mask.vlan) ==
			(pdev->rx_tag_ptrn.vlan & pdev->rx_tag_mask.vlan))
			return dev;

		dev = pdev->next;
	}
#ifdef CONFIG_MV_ETH_DEBUG_CODE
	printk(KERN_ERR "%s:Error TAG=0x%08x match no interfaces\n", __func__, tag->vlan);
#endif

	return NULL;
}

/*-----------------------------------------------------------------------------------------*/

static inline struct net_device *mv_mux_dsa_netdev_get(int port, MV_TAG *tag)
{
	/*
	   MV_TAG.vlan and MV_TAG.dsa size are equal.
	   MV_TAG type is union.
	   We can use in the same functins.
	*/

	return mv_mux_vlan_netdev_get(port, tag);
}

/*-----------------------------------------------------------------------------------------*/

static inline struct net_device *mv_mux_edsa_netdev_get(int port, MV_TAG *tag)
{
	struct net_device *dev = mux_eth_shadow[port].switch_dev;
	struct mux_netdev *pdev;

	while (dev != NULL) {
		pdev = MV_MUX_PRIV(dev);
#ifdef CONFIG_MV_ETH_DEBUG_CODE
		if (mux_eth_shadow[port].flags & MV_MUX_F_DBG_RX)
			printk(KERN_ERR "pkt tag = 0x%x %x, rx_tag_ptrn = 0x%x %x, rx_tag_mask = 0x%x %x\n",
				 tag->edsa[0], tag->edsa[1], pdev->rx_tag_ptrn.edsa[0], pdev->rx_tag_ptrn.edsa[1],
				 pdev->rx_tag_mask.edsa[0], pdev->rx_tag_mask.edsa[1]);
#endif
		/* compare tags */
		if (((tag->edsa[0] & pdev->rx_tag_mask.edsa[0]) ==
			(pdev->rx_tag_ptrn.edsa[0] & pdev->rx_tag_mask.edsa[0])) &&
			((tag->edsa[1] & pdev->rx_tag_mask.edsa[1]) ==
			(pdev->rx_tag_ptrn.edsa[1] & pdev->rx_tag_mask.edsa[1])))
				return dev;

		dev = pdev->next;
	}
#ifdef CONFIG_MV_ETH_DEBUG_CODE
	printk(KERN_ERR "%s:Error TAG=0x%08x match no interfaces\n", __func__, tag->vlan);
#endif

	return NULL;
}

/*-----------------------------------------------------------------------------------------*/


static inline struct net_device *mv_mux_rx_netdev_get(int port, struct sk_buff *skb)
{
	struct net_device *dev;
	MV_TAG tag;
	MV_U8 *data = skb->data;
	int tag_type = mux_eth_shadow[port].tag_type;


	/* skb->data point to MH */
	switch (tag_type) {

	case MV_TAG_TYPE_MH:
		tag.mh = *(MV_U16 *)data;
		dev = mv_mux_mh_netdev_get(port, &tag);
		break;

	case MV_TAG_TYPE_VLAN:
		tag.vlan = *(MV_U32 *)(data + MV_ETH_MH_SIZE + (2 * MV_MAC_ADDR_SIZE));
		dev = mv_mux_vlan_netdev_get(port, &tag);
		break;

	case MV_TAG_TYPE_DSA:
		tag.dsa = *(MV_U32 *)(data + MV_ETH_MH_SIZE + (2 * MV_MAC_ADDR_SIZE));
		dev = mv_mux_dsa_netdev_get(port, &tag);
		break;

	case MV_TAG_TYPE_EDSA:
		tag.edsa[0] = *(MV_U32 *)(data + MV_ETH_MH_SIZE + (2 * MV_MAC_ADDR_SIZE));
		tag.edsa[1] = *(MV_U32 *)(data + MV_ETH_MH_SIZE + (2 * MV_MAC_ADDR_SIZE) + 4);
		dev = mv_mux_edsa_netdev_get(port, &tag);
		break;

	default:
		printk(KERN_ERR "%s: unexpected port mode = %d\n", __func__, tag_type);
		return NULL;
	}

	return dev;
}

/*-----------------------------------------------------------------------------------------*/
static inline int mv_mux_mh_skb_remove(struct sk_buff *skb)
{
	__skb_pull(skb, MV_ETH_MH_SIZE);
	return MV_ETH_MH_SIZE;
}

/*-----------------------------------------------------------------------------------------*/

static inline int mv_mux_vlan_skb_remove(struct sk_buff *skb)
{
	/* memmove use temporrary array, no overlap problem*/
	memmove(skb->data + MV_VLAN_HLEN, skb->data, (2 * MV_MAC_ADDR_SIZE) + MV_ETH_MH_SIZE);

	__skb_pull(skb, MV_VLAN_HLEN);

	return MV_ETH_VLAN_SIZE;
}
/*-----------------------------------------------------------------------------------------*/

static inline int mv_mux_dsa_skb_remove(struct sk_buff *skb)
{
	/* memmove use temporrary array, no overlap problem*/
	memmove(skb->data + MV_ETH_DSA_SIZE, skb->data, (2 * MV_MAC_ADDR_SIZE) + MV_ETH_MH_SIZE);

	__skb_pull(skb, MV_ETH_DSA_SIZE);

	return MV_ETH_DSA_SIZE;
}
/*-----------------------------------------------------------------------------------------*/

static inline int mv_mux_edsa_skb_remove(struct sk_buff *skb)
{
	/* memmove use temporrary array, no overlap problem*/
	memmove(skb->data + MV_ETH_EDSA_SIZE, skb->data, (2 * MV_MAC_ADDR_SIZE) + MV_ETH_MH_SIZE);

	__skb_pull(skb, MV_ETH_EDSA_SIZE);

	return MV_ETH_EDSA_SIZE;
}

/*-----------------------------------------------------------------------------------------*/

static inline int mv_mux_rx_tag_remove(struct net_device *dev, struct sk_buff *skb)
{
	int shift = 0;
	struct mux_netdev *pdev = MV_MUX_PRIV(dev);
	int tag_type = mux_eth_shadow[pdev->port].tag_type;

	if (pdev->leave_tag == true)
		return 0;

	switch (tag_type) {

	case MV_TAG_TYPE_MH:
		break;

	case MV_TAG_TYPE_VLAN:
		shift = mv_mux_vlan_skb_remove(skb);
		break;

	case MV_TAG_TYPE_DSA:
		shift = mv_mux_dsa_skb_remove(skb);
		break;

	case MV_TAG_TYPE_EDSA:
		shift = mv_mux_edsa_skb_remove(skb);
		break;

	default:
		printk(KERN_ERR "%s: unexpected port mode = %d\n", __func__, tag_type);
		return -1;
	}
	/* MH exist in packet anycase - Skip it */
	shift += mv_mux_mh_skb_remove(skb);

	return shift;
}


/*-----------------------------------------------------------------------------------------*/

static inline int mv_eth_skb_mh_add(struct sk_buff *skb, u16 mh)
{

	/* sanity: Check that there is place for MH in the buffer */
	if (skb_headroom(skb) < MV_ETH_MH_SIZE) {
		printk(KERN_ERR "%s: skb (%p) doesn't have place for MH, head=%p, data=%p\n",
		       __func__, skb, skb->head, skb->data);
		return 1;
	}

	/* Prepare place for MH header */
	__skb_push(skb, MV_ETH_MH_SIZE);

	*((u16 *) skb->data) = mh;

	return MV_OK;
}

static inline int mv_mux_tx_skb_mh_add(struct net_device *dev, struct sk_buff *skb)
{
	struct mux_netdev *pdev = MV_MUX_PRIV(dev);

	return mv_eth_skb_mh_add(skb, pdev->tx_tag.mh);
}

/*-----------------------------------------------------------------------------------------*/

static inline int mv_mux_skb_vlan_add(struct sk_buff *skb, unsigned int vlan)
{
	unsigned char *pvlan;
/*
	TODO: add stat counter to mux_pp
		mean that there is not enough bytes in header room
		to push vlan, skb_cow will realloc skb

	if (skb_headroom(skb) < MV_VLAN_HLEN) {
		mux_skb_tx_realloc++;
	}
*/
	if (skb_cow(skb, MV_VLAN_HLEN)) {
		printk(KERN_ERR "%s: skb (%p) headroom < VLAN_HDR, skb_head=%p, skb_data=%p\n",
		       __func__, skb, skb->head, skb->data);
		return 1;
	}

	__skb_push(skb, MV_VLAN_HLEN);

	memmove(skb->data, skb->data + MV_VLAN_HLEN, 2 * MV_MAC_ADDR_SIZE);

	pvlan = skb->data + (2 * MV_MAC_ADDR_SIZE);
	*(MV_U32 *)pvlan = vlan;

	return MV_OK;
}

static inline int mv_mux_tx_skb_vlan_add(struct net_device *dev, struct sk_buff *skb)
{
	struct mux_netdev *pdev = MV_MUX_PRIV(dev);

	return mv_mux_skb_vlan_add(skb, pdev->tx_tag.vlan);
}


/*-----------------------------------------------------------------------------------------*/

static inline int mv_mux_tx_skb_dsa_add(struct net_device *dev, struct sk_buff *skb)
{
	/* both DSA and VLAN are 4 bytes tags, placed in the same offset in the packet */
	return mv_mux_tx_skb_vlan_add(dev, skb);
}

/*-----------------------------------------------------------------------------------------*/

static inline int mv_mux_skb_edsa_add(struct sk_buff *skb, unsigned int edsaL, unsigned int edsaH)
{
	unsigned char *pedsa;

	if (skb_cow(skb, MV_ETH_EDSA_SIZE)) {
		printk(KERN_ERR "%s: skb (%p) headroom < VLAN_HDR, skb_head=%p, skb_data=%p\n",
		       __func__, skb, skb->head, skb->data);
		return 1;
	}

	__skb_push(skb, MV_ETH_EDSA_SIZE);

	memmove(skb->data, skb->data + MV_ETH_EDSA_SIZE, 2 * MV_MAC_ADDR_SIZE);

	pedsa = skb->data + (2 * MV_MAC_ADDR_SIZE);
	*(MV_U32 *)pedsa = edsaL;
	*((MV_U32 *)pedsa + 1) = edsaH;

	return MV_OK;
}

static inline int mv_mux_tx_skb_edsa_add(struct net_device *dev, struct sk_buff *skb)
{
	struct mux_netdev *pdev = MV_MUX_PRIV(dev);

	return mv_mux_skb_edsa_add(skb, pdev->tx_tag.edsa[0], pdev->tx_tag.edsa[1]);
}

/*-----------------------------------------------------------------------------------------*/

static inline int mv_mux_tx_skb_tag_add(struct net_device *dev, struct sk_buff *skb)
{
	struct mux_netdev *pdev = MV_MUX_PRIV(dev);
	int tag_type = mux_eth_shadow[pdev->port].tag_type;
	int err = 0;

	switch (tag_type) {

	case MV_TAG_TYPE_MH:
		err = mv_mux_tx_skb_mh_add(dev, skb);
		break;
	case MV_TAG_TYPE_VLAN:
		err = mv_mux_tx_skb_vlan_add(dev, skb);
		break;
	case MV_TAG_TYPE_DSA:
		err = mv_mux_tx_skb_dsa_add(dev, skb);
		break;
	case MV_TAG_TYPE_EDSA:
		err = mv_mux_tx_skb_edsa_add(dev, skb);
		break;
	default:
		printk(KERN_ERR "%s: unexpected port mode = %d\n", __func__, tag_type);
		err = 1;
	}
	return err;
}

/*--------------------------------------------------------------------------------------*/
/* Print mux device data								*/
/*--------------------------------------------------------------------------------------*/

void mv_mux_netdev_print(struct net_device *mux_dev)
{

	struct mux_netdev *pdev;
	int tag_type;


	if (!mux_dev) {
		printk(KERN_ERR "%s:device in NULL.\n", __func__);
		return;
	}

	if (mv_mux_netdev_find(mux_dev->ifindex) != -1) {
		printk(KERN_ERR "%s: %s is not mux device.\n", __func__, mux_dev->name);
		return;
	}

	pdev = MV_MUX_PRIV(mux_dev);

	if (!pdev || (pdev->port == -1)) {
		printk(KERN_ERR "%s: device must be conncted to physical port\n", __func__);
		return;
	}
	tag_type = mux_eth_shadow[pdev->port].tag_type;
	switch (tag_type) {

	case MV_TAG_TYPE_VLAN:
		printk(KERN_ERR "%s: port=%d, pdev=%p, tx_vlan=0x%08x, rx_vlan=0x%08x, rx_mask=0x%08x\n",
			mux_dev->name, pdev->port, pdev, pdev->tx_tag.vlan,
			pdev->rx_tag_ptrn.vlan, pdev->rx_tag_mask.vlan);
		break;

	case MV_TAG_TYPE_DSA:
		printk(KERN_ERR "%s: port=%d, pdev=%p: tx_dsa=0x%08x, rx_dsa=0x%08x, rx_mask=0x%08x\n",
			mux_dev->name, pdev->port, pdev, pdev->tx_tag.dsa,
			pdev->rx_tag_ptrn.dsa, pdev->rx_tag_mask.dsa);
		break;

	case MV_TAG_TYPE_MH:
		printk(KERN_ERR "%s: port=%d, pdev=%p: tx_mh=0x%04x, rx_mh=0x%04x, rx_mask=0x%04x\n",
			mux_dev->name, pdev->port, pdev, pdev->tx_tag.mh, pdev->rx_tag_ptrn.mh, pdev->rx_tag_mask.mh);
		break;

	case MV_TAG_TYPE_EDSA:
		printk(KERN_ERR "%s: port=%d, pdev=%p: tx_edsa=0x%08x %08x, rx_edsa=0x%08x %08x, rx_mask=0x%08x %08x\n",
			mux_dev->name, pdev->port, pdev, pdev->tx_tag.edsa[1], pdev->tx_tag.edsa[0],
			pdev->rx_tag_ptrn.edsa[1], pdev->rx_tag_ptrn.edsa[0],
			pdev->rx_tag_mask.edsa[1], pdev->rx_tag_mask.edsa[0]);
		break;

	default:
		printk(KERN_ERR "%s: Error, Unknown tag type\n", __func__);
	}
}
EXPORT_SYMBOL(mv_mux_netdev_print);

/*--------------------------------------------------------------------------------------*/
/* Print all port's mux devices data							*/
/*--------------------------------------------------------------------------------------*/
void mv_mux_netdev_print_all(int port)
{
	struct net_device *dev;
	struct mux_netdev *dev_priv;

	dev = mux_eth_shadow[port].root;

	if (!dev)
		return;

	dev = mux_eth_shadow[port].switch_dev;

	while (dev != NULL) {
		mv_mux_netdev_print(dev);
		dev_priv = MV_MUX_PRIV(dev);
		dev = dev_priv->next;
		printk(KERN_CONT "\n");
	}
}
EXPORT_SYMBOL(mv_mux_netdev_print_all);
/*-----------------------------------------------------------------------------------------*/
int mv_mux_ctrl_dbg_flag(int port, u32 flag, u32 val)
{
#ifdef CONFIG_MV_ETH_DEBUG_CODE
	struct net_device *root = mux_eth_shadow[port].root;
	u32 bit_flag = (fls(flag) - 1);

	if (!root)
		return -ENODEV;

	if (val)
		set_bit(bit_flag, (unsigned long *)&(mux_eth_shadow[port].flags));
	else
		clear_bit(bit_flag, (unsigned long *)&(mux_eth_shadow[port].flags));
#endif /* CONFIG_MV_ETH_DEBUG_CODE */

	return 0;
}
/*-----------------------------------------------------------------------------------------*/
static const struct net_device_ops mv_mux_netdev_ops = {
	.ndo_open		= mv_mux_open,
	.ndo_stop		= mv_mux_close,
	.ndo_start_xmit		= mv_mux_xmit,
	.ndo_set_mac_address	= mv_mux_set_mac,
	.ndo_do_ioctl		= mv_mux_ioctl,
	.ndo_set_rx_mode	= mv_mux_set_rx_mode,
	.ndo_change_rx_flags	= mv_mux_change_rx_flags,
	.ndo_change_mtu		= mv_mux_mtu_change,
};
/*-----------------------------------------------------------------------------------------*/
static const struct mv_switch_mux_ops mux_ops =  {
	.update_link = mv_mux_update_link,
};
