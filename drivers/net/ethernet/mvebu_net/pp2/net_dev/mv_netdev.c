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
#include <linux/kernel.h>
#include <linux/version.h>
#include <linux/netdevice.h>
#include <linux/etherdevice.h>
#include <linux/platform_device.h>
#include <linux/skbuff.h>
#include <linux/module.h>
#include <linux/mbus.h>
#include <linux/inetdevice.h>
#include <linux/interrupt.h>
#include <linux/mv_pp2.h>
#include <asm/setup.h>
#include <net/ip.h>
#include <net/ipv6.h>

#include "mvOs.h"
#include "mvDebug.h"
#include "mvEthPhy.h"

#include "gbe/mvPp2Gbe.h"
#include "prs/mvPp2Prs.h"
#include "prs/mvPp2PrsHw.h"
#include "cls/mvPp2Classifier.h"
#include "dpi/mvPp2DpiHw.h"

#include "mv_mux_netdev.h"
#include "mv_netdev.h"
#include "mv_eth_tool.h"
#include "mv_eth_sysfs.h"

#define MV_ETH_MAX_NAPI_GROUPS	MV_ETH_MAX_RXQ

#define MV_ETH_TX_PENDING_TIMEOUT_MSEC     1000

#ifdef CONFIG_MV_ETH_SWF_HWF_CORRUPTION_WA
static unsigned int mv_pp2_swf_hwf_wa_en;
void mv_pp2_cache_inv_wa_ctrl(int en)
{
	mv_pp2_swf_hwf_wa_en = en;
}
void mv_eth_iocc_l1_l2_cache_inv(unsigned char *v_start, int size)
{
	if (mv_pp2_swf_hwf_wa_en)
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 34)
		___dma_single_dev_to_cpu(v_start, size, DMA_FROM_DEVICE);
#else
		dma_cache_maint(v_start, size, DMA_FROM_DEVICE);
#endif
}
#endif /* CONFIG_MV_ETH_SWF_HWF_CORRUPTION_WA */

static struct mv_mux_eth_ops mux_eth_ops;

static struct  platform_device *pp2_sysfs;

/*
platform_device used in mv_eth_all_ports_probe only for debug
*/

struct platform_device *plats[MV_ETH_MAX_PORTS];

/* Temporary implementation for SWF to HWF transition */
static void *sync_head;
static u32   sync_rx_desc;

static inline int mv_eth_tx_policy(struct eth_port *pp, struct sk_buff *skb);

#ifdef CONFIG_NET_SKB_RECYCLE
int mv_ctrl_recycle = CONFIG_NET_SKB_RECYCLE_DEF;
EXPORT_SYMBOL(mv_ctrl_recycle);

int mv_eth_ctrl_recycle(int en)
{
	mv_ctrl_recycle = en;
	return 0;
}
#else
int mv_eth_ctrl_recycle(int en)
{
	printk(KERN_ERR "SKB recycle is not supported\n");
	return 1;
}
#endif /* CONFIG_NET_SKB_RECYCLE */

struct bm_pool mv_eth_pool[MV_ETH_BM_POOLS];
struct eth_port **mv_eth_ports;
struct aggr_tx_queue *aggr_txqs;
EXPORT_SYMBOL(aggr_txqs);

int mv_ctrl_txdone = CONFIG_MV_ETH_TXDONE_COAL_PKTS;
EXPORT_SYMBOL(mv_ctrl_txdone);

unsigned int mv_eth_pnc_ctrl_en = 1;

/*
 * Static declarations
 */
static int mv_eth_ports_num = 0;

static int mv_eth_initialized = 0;

static struct tasklet_struct link_tasklet;

/*
 * Local functions
 */
static void mv_eth_txq_delete(struct eth_port *pp, struct tx_queue *txq_ctrl);
static void mv_eth_tx_timeout(struct net_device *dev);
static int  mv_eth_tx(struct sk_buff *skb, struct net_device *dev);
static void mv_eth_tx_frag_process(struct eth_port *pp, struct sk_buff *skb, struct aggr_tx_queue *aggr_txq_ctrl,
		struct tx_queue *txq_ctrl, struct mv_eth_tx_spec *tx_spec);

static void mv_eth_config_show(void);
static int  mv_eth_priv_init(struct eth_port *pp, int port);
static void mv_eth_priv_cleanup(struct eth_port *pp);
static int  mv_eth_config_get(struct platform_device *pdev, u8 *mac);
static int  mv_eth_hal_init(struct eth_port *pp);
struct net_device *mv_eth_netdev_init(int mtu, u8 *mac, struct platform_device *pdev);
static int mv_eth_netdev_connect(struct eth_port *pp);
static void mv_eth_netdev_init_features(struct net_device *dev);
#if LINUX_VERSION_CODE < KERNEL_VERSION(3, 4, 25)
static u32 mv_eth_netdev_fix_features(struct net_device *dev, u32 features);
#else
static netdev_features_t mv_eth_netdev_fix_features(struct net_device *dev, netdev_features_t features);
#endif

static struct sk_buff *mv_eth_skb_alloc(struct bm_pool *pool, phys_addr_t *phys_addr, gfp_t gfp_mask);
static MV_STATUS mv_eth_pool_create(int pool, int capacity);
static int mv_eth_pool_add(int pool, int buf_num);
static int mv_eth_pool_free(int pool, int num);
static int mv_eth_pool_destroy(int pool);
static struct bm_pool *mv_eth_pool_use(int pool, enum mv_eth_bm_type type, int pkt_size);

#ifdef CONFIG_MV_ETH_TSO
int mv_eth_tx_tso(struct sk_buff *skb, struct net_device *dev, struct mv_eth_tx_spec *tx_spec,
			struct tx_queue *txq_ctrl, struct aggr_tx_queue *aggr_txq_ctrl);
#endif

#if defined(CONFIG_NETMAP) || defined(CONFIG_NETMAP_MODULE)
#include <mv_pp2_netmap.h>
#endif


void mv_eth_ctrl_pnc(int en)
{
	mv_eth_pnc_ctrl_en = en;
}

/*****************************************
 *          Adaptive coalescing          *
 *****************************************/
static void mv_eth_adaptive_rx_update(struct eth_port *pp)
{
	unsigned long period = jiffies - pp->rx_timestamp;

	if (period >= (pp->rate_sample_cfg * HZ)) {
		int i;
		unsigned long rate = pp->rx_rate_pkts * HZ / period;

		if (rate < pp->pkt_rate_low_cfg) {
			if (pp->rate_current != 1) {
				pp->rate_current = 1;
				for (i = 0; i < CONFIG_MV_ETH_RXQ; i++) {
					mv_eth_rx_time_coal_set(pp->port, i, pp->rx_time_low_coal_cfg);
					mv_eth_rx_ptks_coal_set(pp->port, i, pp->rx_pkts_low_coal_cfg);
				}
			}
		} else if (rate > pp->pkt_rate_high_cfg) {
			if (pp->rate_current != 3) {
				pp->rate_current = 3;
				for (i = 0; i < CONFIG_MV_ETH_RXQ; i++) {
					mv_eth_rx_time_coal_set(pp->port, i, pp->rx_time_high_coal_cfg);
					mv_eth_rx_ptks_coal_set(pp->port, i, pp->rx_pkts_high_coal_cfg);
				}
			}
		} else {
			if (pp->rate_current != 2) {
				pp->rate_current = 2;
				for (i = 0; i < CONFIG_MV_ETH_RXQ; i++) {
					mv_eth_rx_time_coal_set(pp->port, i, pp->rx_time_coal_cfg);
					mv_eth_rx_ptks_coal_set(pp->port, i, pp->rx_pkts_coal_cfg);
				}
			}
		}

		pp->rx_rate_pkts = 0;
		pp->rx_timestamp = jiffies;
	}
}

/*****************************************
 *            MUX function                *
 *****************************************/
static int mv_eth_tag_type_set(int port, int type)
{
	struct eth_port *pp = mv_eth_port_by_id(port);

	if ((type == MV_TAG_TYPE_MH) || (type == MV_TAG_TYPE_DSA) || (type == MV_TAG_TYPE_EDSA))
		mvPp2MhSet(port, type);

	pp->tagged = (type == MV_TAG_TYPE_NONE) ? MV_FALSE : MV_TRUE;

	return 0;
}
/*****************************************
 *            NAPI Group API             *
 *****************************************/
/* Add/update a new empty napi_group */
int mv_eth_port_napi_group_create(int port, int group)
{
	struct eth_port *pp = mv_eth_port_by_id(port);
	struct napi_group_ctrl *napi_group;

	if ((group < 0) || (group >= MV_ETH_MAX_NAPI_GROUPS)) {
		printk(KERN_ERR "%s: invalid napi group number - %d\n", __func__, group);
		return 1;
	}

	napi_group = pp->napi_group[group];
	if (napi_group) {
		printk(KERN_ERR "%s: group already exist - %d\n", __func__, group);
		return 1;
	}

	napi_group = mvOsMalloc(sizeof(struct napi_group_ctrl));
	if (!napi_group)
		return 1;

	napi_group->napi = kmalloc(sizeof(struct napi_struct), GFP_KERNEL);
	if (!napi_group->napi) {
		mvOsFree(napi_group);
		return 1;
	}

	memset(napi_group->napi, 0, sizeof(struct napi_struct));
	netif_napi_add(pp->dev, napi_group->napi, mv_eth_poll, pp->weight);
	pp->napi_group[group] = napi_group;
	napi_group->id = group;

	return 0;
}

/* Delete napi_group */
int mv_eth_port_napi_group_delete(int port, int group)
{
	struct eth_port *pp = mv_eth_port_by_id(port);
	struct napi_group_ctrl *napi_group;

	if ((group < 0) || (group >= MV_ETH_MAX_NAPI_GROUPS)) {
		printk(KERN_ERR "%s: invalid napi group number - %d\n", __func__, group);
		return 1;
	}

	napi_group = pp->napi_group[group];
	if (!napi_group)
		return 1;

	if ((napi_group->cpu_mask != 0) || (napi_group->rxq_mask != 0)) {
		printk(KERN_ERR "%s: group %d still has cpus/rxqs - cpus=0x%02x  rxqs=0x%04x\n", __func__,
			group, napi_group->cpu_mask, napi_group->rxq_mask);
		return 1;
	}

	netif_napi_del(napi_group->napi);
	mvOsFree(napi_group->napi);
	mvOsFree(napi_group);
	pp->napi_group[group] = NULL;

	return 0;
}

int mv_eth_napi_set_cpu_affinity(int port, int group, int cpu_mask)
{
	struct eth_port *pp = mv_eth_port_by_id(port);
	struct napi_group_ctrl *napi_group;
	int i, cpu;

	if (pp == NULL) {
		printk(KERN_ERR "%s: pp is null \n", __func__);
		return MV_FAIL;
	}
	if ((group < 0) || (group >= MV_ETH_MAX_NAPI_GROUPS)) {
		printk(KERN_ERR "%s: group number is higher than %d\n", __func__, MV_ETH_MAX_NAPI_GROUPS - 1);
		return -1;
	}
	if (pp->flags & MV_ETH_F_STARTED) {
		printk(KERN_ERR "%s: port %d must be stopped\n", __func__, port);
		return -EINVAL;
	}

	/* check that cpu_mask doesn't have cpu that belong to other group */
	for (i = 0; i < MV_ETH_MAX_NAPI_GROUPS; i++) {
		napi_group = pp->napi_group[i];
		if ((!napi_group) || (i == group))
			continue;

		if (napi_group->cpu_mask & cpu_mask) {
			printk(KERN_ERR "%s: cpus mask contains cpu that is already in other group(%d) - %d\n",
				__func__, i, cpu_mask);
			return MV_FAIL;
		}
	}

	napi_group = pp->napi_group[group];
	if (napi_group == NULL) {
		printk(KERN_ERR "%s: napi group #%d doesn't exist\n", __func__, group);
		return MV_FAIL;
	}

	/* update group's CPU mask - remove old CPUs using this group */
	for_each_possible_cpu(cpu)
		if ((1 << cpu) & napi_group->cpu_mask)
			pp->cpu_config[cpu]->napi_group = NULL;

	napi_group->cpu_mask = cpu_mask;
	napi_group->cause_rx_tx = 0;

	for_each_possible_cpu(cpu)
		if ((1 << cpu) & cpu_mask)
			pp->cpu_config[cpu]->napi_group = napi_group;

	return 0;
}

int mv_eth_napi_set_rxq_affinity(int port, int group, int rxq_mask)
{
	struct eth_port *pp = mv_eth_port_by_id(port);
	struct napi_group_ctrl *napi_group;
	int i;

	if (pp == NULL) {
		printk(KERN_ERR "%s: pp is null \n", __func__);
		return MV_FAIL;
	}
	if ((group < 0) || (group >= MV_ETH_MAX_NAPI_GROUPS)) {
		printk(KERN_ERR "%s: group number is higher than %d\n", __func__, MV_ETH_MAX_NAPI_GROUPS - 1);
		return -1;
	}
	if (pp->flags & MV_ETH_F_STARTED) {
		printk(KERN_ERR "%s: port %d must be stopped\n", __func__, port);
		return -EINVAL;
	}

	/* check that rxq_mask doesn't have rxq that belong to other group */
	for (i = 0; i < MV_ETH_MAX_NAPI_GROUPS; i++) {
		napi_group = pp->napi_group[i];
		if ((!napi_group) || (i == group))
			continue;

		if (napi_group->rxq_mask & rxq_mask) {
			printk(KERN_ERR "%s: rxqs/cpus mask contains rxq that is already in other group(%d) - %d\n",
				__func__, i, rxq_mask);
			return MV_FAIL;
		}
	}

	napi_group = pp->napi_group[group];
	if (napi_group == NULL) {
		printk(KERN_ERR "%s: napi group #%d doesn't exist\n", __func__, group);
		return MV_FAIL;
	}

	napi_group->rxq_mask = rxq_mask;
	napi_group->cause_rx_tx = 0;

	return 0;
}

/**********************************************************/

struct eth_port *mv_eth_port_by_id(unsigned int port)
{
	if (mv_eth_ports && (port < mv_eth_ports_num))
		return mv_eth_ports[port];

	return NULL;
}

/* return the first port in port_mask that is up, or -1 if all ports are down */
static int mv_eth_port_up_get(unsigned int port_mask)
{
	int port;
	struct eth_port *pp;

	for (port = 0; port < mv_eth_ports_num; port++) {
		if (!((1 << port) & port_mask))
			continue;

		pp = mv_eth_port_by_id(port);
		if (pp == NULL)
			continue;

		if (pp->flags & MV_ETH_F_STARTED)
			return port;
	}

	return -1;
}

static inline int mv_eth_skb_mh_add(struct sk_buff *skb, u16 mh)
{
       /* sanity: Check that there is place for MH in the buffer */
       if (skb_headroom(skb) < MV_ETH_MH_SIZE) {
		printk(KERN_ERR "%s: skb (%p) doesn't have place for MH, head=%p, data=%p\n",
		__func__, skb, skb->head, skb->data);
		return 1;
	}

	/* Prepare place for MH header */
	skb->len += MV_ETH_MH_SIZE;
	skb->data -= MV_ETH_MH_SIZE;
	*((u16 *) skb->data) = mh;

	return 0;
}

static inline int mv_eth_mh_skb_skip(struct sk_buff *skb)
{
	__skb_pull(skb, MV_ETH_MH_SIZE);
	return MV_ETH_MH_SIZE;
}

void mv_eth_ctrl_txdone(int num)
{
	mv_ctrl_txdone = num;
}

int mv_eth_ctrl_tx_flag(int port, u32 flag, u32 val)
{
	struct eth_port *pp = mv_eth_port_by_id(port);
	u32 bit_flag = (fls(flag) - 1);

	if (!pp)
		return -ENODEV;

	if (val)
		set_bit(bit_flag, &(pp->tx_spec.flags));
	else
		clear_bit(bit_flag, &(pp->tx_spec.flags));

	return 0;
}

int mv_eth_ctrl_flag(int port, u32 flag, u32 val)
{
	struct eth_port *pp = mv_eth_port_by_id(port);
	u32 bit_flag = (fls(flag) - 1);

	if (!pp)
		return -ENODEV;

	if (val)
		set_bit(bit_flag, &(pp->flags));
	else
		clear_bit(bit_flag, &(pp->flags));

	return 0;
}

int mv_eth_ctrl_dbg_flag(int port, u32 flag, u32 val)
{
#ifdef CONFIG_MV_ETH_DEBUG_CODE
	struct eth_port *pp = mv_eth_port_by_id(port);
	u32 bit_flag = (fls(flag) - 1);

	if (!pp)
		return -ENODEV;

	if (val)
		pp->dbg_flags |= (1 << bit_flag);
	else
		pp->dbg_flags &= ~(1 << bit_flag);
#endif /* CONFIG_MV_ETH_DEBUG_CODE */

	return 0;
}

/* mv_eth_ctrl_pool_port_map_get					*
 *     - Return ports map use this BM pool			*/
int mv_eth_ctrl_pool_port_map_get(int pool)
{
	struct bm_pool *ppool;

	if ((pool < 0) || (pool >= MV_ETH_BM_POOLS)) {
		pr_err("%s: Invalid pool number (%d)\n", __func__, pool);
		return -1;
	}

	ppool = &mv_eth_pool[pool];
	if (ppool == NULL) {
		pr_err("%s: BM pool %d is not initialized\n", __func__, pool);
		return -1;
	}
	return ppool->port_map;
}

/* mv_eth_ctrl_pool_buf_num_set					*
 *     - Set number of buffers for BM pool			*
 *     - Add or remove buffers to this pool accordingly		*/
int mv_eth_ctrl_pool_buf_num_set(int pool, int buf_num)
{
	unsigned long flags = 0;
	struct bm_pool *ppool;

	if ((pool < 0) || (pool >= MV_ETH_BM_POOLS)) {
		pr_err("%s: Invalid pool number (%d)\n", __func__, pool);
		return -1;
	}

	ppool = &mv_eth_pool[pool];
	if (ppool == NULL) {
		pr_err("%s: BM pool %d is not initialized\n", __func__, pool);
		return -1;
	}

	MV_ETH_LOCK(&ppool->lock, flags);
	if (ppool->buf_num > buf_num)
		mv_eth_pool_free(pool, ppool->buf_num - buf_num);
	else
		mv_eth_pool_add(pool, buf_num - ppool->buf_num);
	MV_ETH_UNLOCK(&ppool->lock, flags);

	return 0;
}

/* mv_eth_ctrl_pool_size_set				*
 *     - Set buffer size for BM pool			*
 *     - All ports using this pool must be stopped	*
 *     - Re-allocate all buffers			*/
int mv_eth_ctrl_pool_size_set(int pool, int total_size)
{
	unsigned long flags = 0;
	struct eth_port *pp;
	struct bm_pool *ppool = &mv_eth_pool[pool];
	int port, pkt_size, buf_size, pkts_num;

	port = mv_eth_port_up_get(ppool->port_map);
	if (port != -1) {
		pr_err("%s: Can't change pool %d buffer size, while port %d is up\n",
			__func__, pool, port);
		return -EINVAL;
	}

	if (MV_ETH_BM_POOL_IS_HWF(ppool->type)) {
		pkt_size = RX_HWF_MAX_PKT_SIZE(total_size);
		buf_size = RX_HWF_BUF_SIZE(pkt_size);
	} else {
		pkt_size = RX_MAX_PKT_SIZE(total_size);
		buf_size = RX_BUF_SIZE(pkt_size);
	}


	for (port = 0; port < mv_eth_ports_num; port++) {
		if (!((1 << port) & ppool->port_map))
			continue;

		pp = mv_eth_port_by_id(port);
		if (pp == NULL)
			continue;

		/* If this pool is used as long pool, then it is expected that MTU will be smaller than buffer size */
		if (MV_ETH_BM_POOL_IS_LONG(ppool->type) && (RX_PKT_SIZE(pp->dev->mtu) > pkt_size))
			pr_warn("%s: port %d MTU (%d) is larger than requested packet size (%d) [total size = %d]\n",
				__func__, port, RX_PKT_SIZE(pp->dev->mtu), pkt_size, total_size);
	}

	MV_ETH_LOCK(&ppool->lock, flags);
	pkts_num = ppool->buf_num;
	mv_eth_pool_free(pool, pkts_num);
	ppool->pkt_size = pkt_size;
	mv_eth_pool_add(pool, pkts_num);

	mvBmPoolBufSizeSet(pool, buf_size);
	MV_ETH_UNLOCK(&ppool->lock, flags);

	pr_info("%s: BM pool %d:\n", __func__, pool);
	pr_info("       packet size = %d, buffer size = %d, total bytes per buffer = %d, true buffer size = %d\n",
		pkt_size, buf_size, total_size, (int)RX_TRUE_SIZE(total_size));

	return 0;
}

/* detach port from old pool */
int mv_eth_ctrl_pool_detach(int port, struct bm_pool *pool)
{
	unsigned long flags = 0;
	/*TODO remove struct bm_pool *pool;*/
	struct eth_port *pp = mv_eth_port_by_id(port);

	if (pp == NULL) {
		pr_err("%s: port %d does not exist\n" , __func__, port);
		return -EINVAL;
	}

	if (pool == NULL) {
		pr_err("%s: pool is null\n" , __func__);
		return -EINVAL;
	}

	if (pp->flags & MV_ETH_F_STARTED) {
		pr_err("%s: port %d must be stopped before\n", __func__, port);
		return -EINVAL;
	}


	pool->port_map &= ~(1 << port);

	if (!pool->port_map) {
		MV_ETH_LOCK(&pool->lock, flags);
		mv_eth_pool_free(pool->pool, pool->buf_num);

		pool->type = MV_ETH_BM_FREE;
		pool->pkt_size = 0;

		mvPp2BmPoolBufSizeSet(pool->pool, 0);
		MV_ETH_UNLOCK(&pool->lock, flags);
	}

	return MV_OK;
}

#ifdef CONFIG_MV_ETH_PP2_1
static int mv_eth_hwf_long_pool_attach(int port, int pool)
{
	int txp, txq;
	struct eth_port *pp = mv_eth_port_by_id(port);

	if (pp == NULL) {
		pr_err("%s: port %d does not exist\n" , __func__, port);
		return -EINVAL;
	}

	for (txp = 0; txp < pp->txp_num; txp++)
		for (txq = 0; txq < CONFIG_MV_ETH_TXQ; txq++)
			mvPp2TxqBmLongPoolSet(port, txp, txq, pool);

	return MV_OK;
}

static int mv_eth_hwf_short_pool_attach(int port, int pool)
{
	int txp, txq;
	struct eth_port *pp = mv_eth_port_by_id(port);

	if (pp == NULL) {
		pr_err("%s: port %d does not exist\n" , __func__, port);
		return -EINVAL;
	}

	for (txp = 0; txp < pp->txp_num; txp++)
		for (txq = 0; txq < CONFIG_MV_ETH_TXQ; txq++)
			mvPp2TxqBmShortPoolSet(port, txp, txq, pool);

	return MV_OK;
}

#else

/* Init classifer MTU */
/* the same MTU for all Ports/Queues */
static int mv_eth_tx_mtu_set(int port, int mtu)
{
	int txp;

	struct eth_port *pp = mv_eth_port_by_id(port);

	if (pp == NULL) {
		pr_err("%s: port %d does not exist\n" , __func__, port);
		return -EINVAL;
	}

	for (txp = 0; txp < pp->txp_num; txp++)
		mvPp2V0ClsHwMtuSet(MV_PPV2_PORT_PHYS(port), txp, RX_PKT_SIZE(mtu));

	return MV_OK;

}

#endif /* CONFIG_MV_ETH_PP2_1 */

int mv_eth_ctrl_long_pool_set(int port, int pool)
{
	unsigned long flags = 0;
	struct eth_port *pp = mv_eth_port_by_id(port);
	struct bm_pool *old_pool;
	int rxq, pkt_size = RX_PKT_SIZE(pp->dev->mtu);

	if (pp == NULL) {
		pr_err("%s: port %d does not exist\n" , __func__, port);
		return -EINVAL;
	}

	if (pp->flags & MV_ETH_F_STARTED) {
		pr_err("%s: port %d must be stopped before\n", __func__, port);
		return -EINVAL;
	}

	old_pool = pp->pool_long;
	if (old_pool) {
		if (old_pool->pool == pool)
			return 0;

		if (pp->hwf_pool_long != pp->pool_long)
			if (mv_eth_ctrl_pool_detach(port, old_pool))
				return -EINVAL;
	}

	pp->pool_long = mv_eth_pool_use(pool, MV_ETH_BM_SWF_LONG, pkt_size);
	if (!pp->pool_long)
		return -EINVAL;
	MV_ETH_LOCK(&pp->pool_long->lock, flags);
	pp->pool_long->port_map |= (1 << port);
	MV_ETH_UNLOCK(&pp->pool_long->lock, flags);

	for (rxq = 0; rxq < pp->rxq_num; rxq++)
		mvPp2RxqBmLongPoolSet(port, rxq, pp->pool_long->pool);

	return 0;
}

int mv_eth_ctrl_short_pool_set(int port, int pool)
{
	unsigned long flags = 0;
	struct eth_port *pp = mv_eth_port_by_id(port);
	struct bm_pool *old_pool;
	int rxq;

	if (pp == NULL) {
		pr_err("%s: port %d does not exist\n" , __func__, port);
		return -EINVAL;
	}

	if (pp->flags & MV_ETH_F_STARTED) {
		pr_err("%s: port %d must be stopped before\n", __func__, port);
		return -EINVAL;
	}

	old_pool = pp->pool_short;
	if (old_pool) {
		if (old_pool->pool == pool)
			return 0;

		if (pp->hwf_pool_short != pp->pool_short)
			if (mv_eth_ctrl_pool_detach(port, old_pool))
				return -EINVAL;
	}

	pp->pool_short = mv_eth_pool_use(pool, MV_ETH_BM_SWF_SHORT, MV_ETH_BM_SHORT_PKT_SIZE);
	if (!pp->pool_short)
		return -EINVAL;
	MV_ETH_LOCK(&pp->pool_short->lock, flags);
	pp->pool_short->port_map |= (1 << port);
	MV_ETH_UNLOCK(&pp->pool_short->lock, flags);

	for (rxq = 0; rxq < pp->rxq_num; rxq++)
		mvPp2RxqBmShortPoolSet(port, rxq, pp->pool_short->pool);

	return 0;
}

int mv_eth_ctrl_hwf_long_pool_set(int port, int pool)
{
	unsigned long flags = 0;
	struct eth_port *pp = mv_eth_port_by_id(port);
	struct bm_pool *old_pool;
	int pkt_size = RX_PKT_SIZE(pp->dev->mtu);

	if (pp == NULL) {
		pr_err("%s: port %d does not exist\n" , __func__, port);
		return -EINVAL;
	}

	if (pp->flags & MV_ETH_F_STARTED) {
		pr_err("%s: port %d must be stopped before\n", __func__, port);
		return -EINVAL;
	}

	old_pool = pp->hwf_pool_long;
	if (old_pool) {
		if (old_pool->pool == pool)
			return 0;

		if (pp->hwf_pool_long != pp->pool_long)
			if (mv_eth_ctrl_pool_detach(port, old_pool))
				return -EINVAL;
	}

	pp->hwf_pool_long = mv_eth_pool_use(pool, MV_ETH_BM_HWF_LONG, pkt_size);
	if (!pp->hwf_pool_long)
		return -EINVAL;
	MV_ETH_LOCK(&pp->hwf_pool_long->lock, flags);
	pp->hwf_pool_long->port_map |= (1 << port);
	MV_ETH_UNLOCK(&pp->hwf_pool_long->lock, flags);

#ifdef CONFIG_MV_ETH_PP2_1
	mv_eth_hwf_long_pool_attach(pp->port, pp->hwf_pool_long->pool);
#else
	mvPp2PortHwfBmPoolSet(pp->port, pp->hwf_pool_short->pool, pp->hwf_pool_long->pool);
#endif

	return 0;
}

int mv_eth_ctrl_hwf_short_pool_set(int port, int pool)
{
	unsigned long flags = 0;
	struct eth_port *pp = mv_eth_port_by_id(port);
	struct bm_pool *old_pool;

	if (pp == NULL) {
		pr_err("%s: port %d does not exist\n" , __func__, port);
		return -EINVAL;
	}

	if (pp->flags & MV_ETH_F_STARTED) {
		pr_err("%s: port %d must be stopped before\n", __func__, port);
		return -EINVAL;
	}

	old_pool = pp->hwf_pool_short;
	if (old_pool) {
		if (old_pool->pool == pool)
			return 0;

		if (pp->hwf_pool_short != pp->pool_short)
			if (mv_eth_ctrl_pool_detach(port, old_pool))
				return -EINVAL;
	}
	pp->hwf_pool_short = mv_eth_pool_use(pool, MV_ETH_BM_HWF_SHORT, MV_ETH_BM_SHORT_HWF_PKT_SIZE);
	if (!pp->hwf_pool_short)
		return -EINVAL;
	MV_ETH_LOCK(&pp->hwf_pool_short->lock, flags);
	pp->hwf_pool_short->port_map |= (1 << port);
	MV_ETH_UNLOCK(&pp->hwf_pool_short->lock, flags);

#ifdef CONFIG_MV_ETH_PP2_1
	mv_eth_hwf_short_pool_attach(pp->port, pp->hwf_pool_short->pool);
#else
	mvPp2PortHwfBmPoolSet(pp->port, pp->hwf_pool_short->pool, pp->hwf_pool_long->pool);
#endif

	return 0;
}

int mv_eth_ctrl_set_poll_rx_weight(int port, u32 weight)
{
	struct eth_port *pp = mv_eth_port_by_id(port);
	int i;

	if (pp == NULL) {
		printk(KERN_INFO "port doens not exist (%d) in %s\n" , port, __func__);
		return -EINVAL;
	}

	if (pp->flags & MV_ETH_F_STARTED) {
		printk(KERN_ERR "Port %d must be stopped before\n", port);
		return -EINVAL;
	}

	if (weight > 255)
		weight = 255;
	pp->weight = weight;

	for (i = 0; i < MV_ETH_MAX_NAPI_GROUPS; i++) {
		if (!pp->napi_group[i])
			continue;
		pp->napi_group[i]->napi->weight = pp->weight;
	}

	return 0;
}

int mv_eth_ctrl_rxq_size_set(int port, int rxq, int value)
{
	struct eth_port *pp;
	struct rx_queue	*rxq_ctrl;

	if (mvPp2PortCheck(port))
		return -EINVAL;

	if (mvPp2MaxCheck(rxq, CONFIG_MV_ETH_RXQ, "rxq"))
		return -EINVAL;

	if ((value <= 0) || (value > 0x3FFF) || (value % 16)) {
		pr_err("Invalid RXQ size %d\n", value);
		return -EINVAL;
	}

	pp = mv_eth_port_by_id(port);
	if (pp == NULL) {
		pr_err("Port %d does not exist\n", port);
		return -EINVAL;
	}

	if (pp->flags & MV_ETH_F_STARTED) {
		pr_err("Port %d must be stopped before\n", port);
		return -EINVAL;
	}

	if (value % 16 != 0) {
		printk(KERN_ERR "invalid rxq size\n");
		return -EINVAL;
	}

	rxq_ctrl = &pp->rxq_ctrl[rxq];
	if ((rxq_ctrl->q) && (rxq_ctrl->rxq_size != value)) {
		/* Reset is required when RXQ ring size is changed */
		mvPp2RxqReset(pp->port, rxq);
		mvPp2RxqDelete(pp->port, rxq);
		rxq_ctrl->q = NULL;
	}
	pp->rxq_ctrl[rxq].rxq_size = value;

	/* New RXQ will be created during mv_eth_start_internals */
	return 0;
}


/* set <txp/txq> SWF request chunk size */
int mv_eth_ctrl_txq_chunk_set(int port, int txp, int txq, int chunk_size)
{
	struct tx_queue *txq_ctrl;
	struct eth_port *pp = mv_eth_port_by_id(port);


	if (pp == NULL) {
		printk(KERN_INFO "port does not exist (%d) in %s\n" , port, __func__);
		return -EINVAL;
	}

	txq_ctrl = &pp->txq_ctrl[txp * CONFIG_MV_ETH_TXQ + txq];

	if (!txq_ctrl) {
		printk(KERN_INFO "queue does not exist (%d) in %s\n" , port, __func__);
		return -EINVAL;
	}

	txq_ctrl->rsvd_chunk = chunk_size;

	return MV_OK;
}

/* swf_size is in use only in ppv2.1, ignored in ppv2.0 */
int mv_eth_ctrl_txq_limits_set(int port, int txp, int txq, int hwf_size, int swf_size)
{
	int txq_size;
	struct tx_queue *txq_ctrl;
	struct eth_port *pp;

	if (mvPp2TxpCheck(port, txp))
		return -EINVAL;

	if (mvPp2MaxCheck(txq, CONFIG_MV_ETH_TXQ, "txq"))
		return -EINVAL;

	pp = mv_eth_port_by_id(port);
	if (pp == NULL) {
		printk(KERN_INFO "port does not exist (%d) in %s\n" , port, __func__);
		return -EINVAL;
	}

	txq_ctrl = &pp->txq_ctrl[txp * CONFIG_MV_ETH_TXQ + txq];

	if (!txq_ctrl) {
		printk(KERN_INFO "queue is null %s\n", __func__);
		return -EINVAL;
	}

	txq_size = txq_ctrl->txq_size;

#ifdef CONFIG_MV_ETH_PP2_1
	if (hwf_size < swf_size) {
		printk(KERN_ERR "Invalid size params, swf size must be less than hwf size\n");
		return -EINVAL;
	}
#endif

	if (txq_size < hwf_size) {
		printk(KERN_ERR "invalid hwf size, must be less or equal to txq size (%d)\n", txq_size);
		return -EINVAL;
	}

	if (hwf_size % 16 != 0) {
		printk(KERN_ERR "invalid hwf size, must be aligned to 16\n");
		return -EINVAL;
	}

	txq_ctrl->hwf_size = hwf_size;

#ifdef CONFIG_MV_ETH_PP2_1
	txq_ctrl->swf_size = swf_size;
#endif
	mvPp2TxqHwfSizeSet(port, txp, txq, hwf_size);

	return 0;
}

int mv_eth_ctrl_txq_size_set(int port, int txp, int txq, int txq_size)
{
	int cpu, cpu_size;
	struct tx_queue *txq_ctrl;
	struct txq_cpu_ctrl *txq_cpu_ptr;
	struct eth_port *pp;

	if (mvPp2TxpCheck(port, txp))
		return -EINVAL;

	if (mvPp2MaxCheck(txq, CONFIG_MV_ETH_TXQ, "txq"))
		return -EINVAL;

	if ((txq_size <= 0) || (txq_size > 0x3FFF) || (txq_size % 16)) {
		pr_err("Invalid TXQ size %d\n", txq_size);
		return -EINVAL;
	}

	pp = mv_eth_port_by_id(port);
	if (pp == NULL) {
		pr_err("Port %d does not exist\n", port);
		return -EINVAL;
	}

	if (pp->flags & MV_ETH_F_STARTED) {
		pr_err("Port %d must be stopped before\n", port);
		return -EINVAL;
	}

	txq_ctrl = &pp->txq_ctrl[txp * CONFIG_MV_ETH_TXQ + txq];

	if (!txq_ctrl) {
		pr_err("TXQ is not exist\n");
		return -EINVAL;
	}

	if ((txq_ctrl->q) && (txq_size < txq_ctrl->hwf_size)) {
		pr_err("Invalid TXQ size %d, must be greater than HWF size (%d)\n",
			txq_size, txq_ctrl->hwf_size);
		return -EINVAL;
	}

	if ((txq_ctrl->q) && (txq_ctrl->txq_size != txq_size)) {
		/* Clean and Reset of txq is required when TXQ ring size is changed */
		mv_eth_txq_clean(port, txp, txq);

		/* TBD: If needed to send dummy packets to reset number of descriptors reserved by all CPUs */

		mvPp2TxqReset(port, txp, txq);
		mv_eth_txq_delete(pp, txq_ctrl);
	}

	txq_ctrl->txq_size = txq_size;

	/* right now, all cpus have same size */
	/* relevant only for ppv2.1 */
	cpu_size = (txq_ctrl->txq_size - txq_ctrl->hwf_size) / CONFIG_NR_CPUS;

#ifdef CONFIG_MV_ETH_PP2_1
	for_each_possible_cpu(cpu) {
		txq_cpu_ptr = &txq_ctrl->txq_cpu[cpu];
		txq_cpu_ptr->txq_size = txq_size;
	}
#else
	for_each_possible_cpu(cpu) {
		txq_cpu_ptr = &txq_ctrl->txq_cpu[cpu];
		txq_cpu_ptr->txq_size = cpu_size;
	}
#endif
	/* New TXQ will be created during mv_eth_start_internals */
	return 0;
}


/* Set TXQ for CPU originated packets */
int mv_eth_ctrl_txq_cpu_def(int port, int txp, int txq, int cpu)
{
	struct eth_port *pp = mv_eth_port_by_id(port);

	if ((cpu >= CONFIG_NR_CPUS) || (cpu < 0)) {
		printk(KERN_ERR "cpu #%d is out of range: from 0 to %d\n",
			cpu, CONFIG_NR_CPUS - 1);
		return -EINVAL;
	}

	if (txq >= CONFIG_MV_ETH_TXQ) {
		pr_err("txq #%d is out of range: from 0 to %d\n", txq, CONFIG_MV_ETH_TXQ - 1);
		return -EINVAL;
	}

	if (mvPp2TxpCheck(port, txp))
		return -EINVAL;

	if ((pp == NULL) || (pp->txq_ctrl == NULL))
		return -ENODEV;

	pp->tx_spec.txp = txp;
	pp->cpu_config[cpu]->txq = txq;

	return 0;
}

int mv_eth_ctrl_tx_mh(int port, u16 mh)
{
	struct eth_port *pp = mv_eth_port_by_id(port);

	if (!pp)
		return -ENODEV;

	pp->tx_spec.tx_mh = mh;

	return 0;
}

int mv_eth_ctrl_tx_cmd_dsa(int port, u16 dsa_tag)
{
	struct eth_port *pp = mv_eth_port_by_id(port);

	if (!pp)
		return -ENODEV;

	pp->tx_spec.hw_cmd[0] &= ~PP2_TX_DSA_ALL_MASK;
	pp->tx_spec.hw_cmd[0] |= PP2_TX_DSA_MASK(dsa_tag);

	return 0;
}

int mv_eth_ctrl_tx_cmd_color(int port, u16 color)
{
	struct eth_port *pp = mv_eth_port_by_id(port);

	if (!pp)
		return -ENODEV;

	pp->tx_spec.hw_cmd[0] &= ~PP2_TX_COLOR_ALL_MASK;
	pp->tx_spec.hw_cmd[0] |= PP2_TX_COLOR_MASK(color);

	return 0;
}

int mv_eth_ctrl_tx_cmd_gem_id(int port, u16 gem_port_id)
{
	struct eth_port *pp = mv_eth_port_by_id(port);

	if (!pp)
		return -ENODEV;

	pp->tx_spec.hw_cmd[0] &= ~PP2_TX_GEMPID_ALL_MASK;
	pp->tx_spec.hw_cmd[0] |= PP2_TX_GEMPID_MASK(gem_port_id);

	return 0;
}

int mv_eth_ctrl_tx_cmd_pon_fec(int port, u16 pon_fec)
{
	struct eth_port *pp = mv_eth_port_by_id(port);

	if (!pp)
		return -ENODEV;

	if (pon_fec == 0)
		pp->tx_spec.hw_cmd[2] &= ~PP2_TX_PON_FEC_MASK;
	else
		pp->tx_spec.hw_cmd[2] |= PP2_TX_PON_FEC_MASK;

	return 0;
}

int mv_eth_ctrl_tx_cmd_gem_oem(int port, u16 gem_oem)
{
	struct eth_port *pp = mv_eth_port_by_id(port);

	if (!pp)
		return -ENODEV;

	if (gem_oem == 0)
		pp->tx_spec.hw_cmd[2] &= ~PP2_TX_GEM_OEM_MASK;
	else
		pp->tx_spec.hw_cmd[2] |= PP2_TX_GEM_OEM_MASK;

	return 0;
}

int mv_eth_ctrl_tx_cmd_mod(int port, u16 mod)
{
	struct eth_port *pp = mv_eth_port_by_id(port);
	u32 mask = (PP2_TX_MOD_DSCP_MASK | PP2_TX_MOD_PRIO_MASK | PP2_TX_MOD_DSCP_EN_MASK
			| PP2_TX_MOD_PRIO_EN_MASK | PP2_TX_MOD_GEMPID_EN_MASK);

	if (!pp)
		return -ENODEV;

	/* This command update all fields in the TX descriptor offset 0x14 */
	/* MOD_DSCP - 6 bits, MOD_PRIO - 3bits, MOD_DSCP_EN - 1b, MOD_PRIO_EN - 1b, MOD_GEMPID_EN - 1b */
	pp->tx_spec.hw_cmd[1] &= ~mask;
	pp->tx_spec.hw_cmd[1] |= ((mod << PP2_TX_MOD_DSCP_OFFS) & mask);

	return 0;
}

int mv_eth_ctrl_tx_cmd_pme_dptr(int port, u16 pme_dptr)
{
	struct eth_port *pp = mv_eth_port_by_id(port);

	if (!pp)
		return -ENODEV;

	pp->tx_spec.hw_cmd[2] &= ~PP2_TX_PME_DPTR_ALL_MASK;
	pp->tx_spec.hw_cmd[2] |= PP2_TX_PME_DPTR_MASK(pme_dptr);

	return 0;
}

int mv_eth_ctrl_tx_cmd_pme_prog(int port, u16 pme_prog)
{
	struct eth_port *pp = mv_eth_port_by_id(port);

	if (!pp)
		return -ENODEV;

	pp->tx_spec.hw_cmd[2] &= ~PP2_TX_PME_IPTR_ALL_MASK;
	pp->tx_spec.hw_cmd[2] |= PP2_TX_PME_IPTR_MASK(pme_prog);

	return 0;
}


#ifdef CONFIG_MV_ETH_TX_SPECIAL
/* Register special transmit check function */
void mv_eth_tx_special_check_func(int port,
					int (*func)(int port, struct net_device *dev, struct sk_buff *skb,
								struct mv_eth_tx_spec *tx_spec_out))
{
	struct eth_port *pp = mv_eth_port_by_id(port);

	if (pp)
		pp->tx_special_check = func;
}
#endif /* CONFIG_MV_ETH_TX_SPECIAL */

#ifdef CONFIG_MV_ETH_RX_SPECIAL
/* Register special transmit check function */
void mv_eth_rx_special_proc_func(int port, void (*func)(int port, int rxq, struct net_device *dev,
							struct sk_buff *skb, struct pp2_rx_desc *rx_desc))
{
	struct eth_port *pp = mv_eth_port_by_id(port);

	if (pp)
		pp->rx_special_proc = func;
}
#endif /* CONFIG_MV_ETH_RX_SPECIAL */

static inline u16 mv_eth_select_txq(struct net_device *dev, struct sk_buff *skb)
{
	struct eth_port *pp = MV_ETH_PRIV(dev);
	return mv_eth_tx_policy(pp, skb);
}

static const struct net_device_ops mv_eth_netdev_ops = {
	.ndo_open = mv_eth_open,
	.ndo_stop = mv_eth_stop,
	.ndo_start_xmit = mv_eth_tx,
	.ndo_set_rx_mode = mv_eth_rx_set_rx_mode,
	.ndo_set_mac_address = mv_eth_set_mac_addr,
	.ndo_change_mtu = mv_eth_change_mtu,
	.ndo_tx_timeout = mv_eth_tx_timeout,
	.ndo_select_queue = mv_eth_select_txq,
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 39)
	.ndo_fix_features = mv_eth_netdev_fix_features,
#endif
};


void mv_eth_link_status_print(int port)
{
	MV_ETH_PORT_STATUS link;

#ifdef CONFIG_MV_PON
	if (MV_PON_PORT(port))
		mv_pon_link_status(&link);
	else
#endif /* CONFIG_MV_PON */
		mvGmacLinkStatus(port, &link);

	if (link.linkup) {
		printk(KERN_CONT "link up");
		printk(KERN_CONT ", %s duplex", (link.duplex == MV_ETH_DUPLEX_FULL) ? "full" : "half");
		printk(KERN_CONT ", speed ");

		printk(KERN_CONT "%s\n", mvGmacSpeedStrGet(link.speed));
	} else
		printk(KERN_CONT "link down\n");

}

static void mv_eth_rx_error(struct eth_port *pp, struct pp2_rx_desc *rx_desc)
{
	STAT_ERR(pp->stats.rx_error++);

	if (pp->dev)
		pp->dev->stats.rx_errors++;

#ifdef CONFIG_MV_ETH_DEBUG_CODE
	if ((pp->dbg_flags & MV_ETH_F_DBG_RX) == 0)
		return;

	if (!printk_ratelimit())
		return;

	switch (rx_desc->status & PP2_RX_ERR_CODE_MASK) {
	case PP2_RX_ERR_CRC:
		printk(KERN_ERR "giga #%d: bad rx status %08x (crc error), size=%d\n",
				pp->port, rx_desc->status, rx_desc->dataSize);
		break;
	case PP2_RX_ERR_OVERRUN:
		printk(KERN_ERR "giga #%d: bad rx status %08x (overrun error), size=%d\n",
				pp->port, rx_desc->status, rx_desc->dataSize);
		break;
	/*case NETA_RX_ERR_LEN:*/
	case PP2_RX_ERR_RESOURCE:
		printk(KERN_ERR "giga #%d: bad rx status %08x (resource error), size=%d\n",
				pp->port, rx_desc->status, rx_desc->dataSize);
		break;
	}
	mv_eth_rx_desc_print(rx_desc);
#endif /* CONFIG_MV_ETH_DEBUG_CODE */
}

void mv_eth_skb_print(struct sk_buff *skb)
{
	printk(KERN_ERR "skb=%p: head=%p, data=%p, tail=%p, end=%p\n", skb, skb->head, skb->data, skb->tail, skb->end);
	printk(KERN_ERR "\t mac=%p, network=%p, transport=%p\n",
			skb->mac_header, skb->network_header, skb->transport_header);
	printk(KERN_ERR "\t truesize=%d, len=%d, data_len=%d, mac_len=%d\n",
		skb->truesize, skb->len, skb->data_len, skb->mac_len);
	printk(KERN_ERR "\t users=%d, dataref=%d, nr_frags=%d, gso_size=%d, gso_segs=%d\n",
	       atomic_read(&skb->users), atomic_read(&skb_shinfo(skb)->dataref),
	       skb_shinfo(skb)->nr_frags, skb_shinfo(skb)->gso_size, skb_shinfo(skb)->gso_segs);
	printk(KERN_ERR "\t proto=%d, ip_summed=%d, priority=%d\n", ntohs(skb->protocol), skb->ip_summed, skb->priority);
#ifdef CONFIG_NET_SKB_RECYCLE
	printk(KERN_ERR "\t skb_recycle=%p, hw_cookie=0x%x\n", skb->skb_recycle, skb->hw_cookie);
#endif /* CONFIG_NET_SKB_RECYCLE */
}

void mv_eth_rx_desc_print(struct pp2_rx_desc *desc)
{
	int i;
	u32 *words = (u32 *) desc;

	printk(KERN_ERR "RX desc - %p: ", desc);
	for (i = 0; i < 8; i++)
		printk(KERN_CONT "%8.8x ", *words++);
	printk(KERN_CONT "\n");

	printk(KERN_CONT "pkt_size=%d, L3_offs=%d, IP_hlen=%d, ",
	       desc->dataSize,
	       (desc->status & PP2_RX_L3_OFFSET_MASK) >> PP2_RX_L3_OFFSET_OFFS,
	       (desc->status & PP2_RX_IP_HLEN_MASK) >> PP2_RX_IP_HLEN_OFFS);

	printk(KERN_CONT "L2=%s, ",
		mvPrsL2InfoStr((desc->parserInfo & PP2_RX_L2_CAST_MASK) >> PP2_RX_L2_CAST_OFFS));

	printk(KERN_CONT "VLAN=");
	printk(KERN_CONT "%s, ",
		mvPrsVlanInfoStr((desc->parserInfo & PP2_RX_VLAN_INFO_MASK) >> PP2_RX_VLAN_INFO_OFFS));

	printk(KERN_CONT "L3=");
	if (PP2_RX_L3_IS_IP4(desc->status))
		printk(KERN_CONT "IPv4 (hdr=%s), ", PP2_RX_IP4_HDR_ERR(desc->status) ? "bad" : "ok");
	else if (PP2_RX_L3_IS_IP4_OPT(desc->status))
		printk(KERN_CONT "IPv4 Options (hdr=%s), ", PP2_RX_IP4_HDR_ERR(desc->status) ? "bad" : "ok");
	else if (PP2_RX_L3_IS_IP4_OTHER(desc->status))
		printk(KERN_CONT "IPv4 Other (hdr=%s), ", PP2_RX_IP4_HDR_ERR(desc->status) ? "bad" : "ok");
	else if (PP2_RX_L3_IS_IP6(desc->status))
		printk(KERN_CONT "IPv6, ");
	else if (PP2_RX_L3_IS_IP6_EXT(desc->status))
		printk(KERN_CONT "IPv6 Ext, ");
	else
		printk(KERN_CONT "Unknown, ");

	if (desc->status & PP2_RX_IP_FRAG_MASK)
		printk(KERN_CONT "Frag, ");

	printk(KERN_CONT "L4=");
	if (PP2_RX_L4_IS_TCP(desc->status))
		printk(KERN_CONT "TCP (csum=%s)", (desc->status & PP2_RX_L4_CHK_OK_MASK) ? "Ok" : "Bad");
	else if (PP2_RX_L4_IS_UDP(desc->status))
		printk(KERN_CONT "UDP (csum=%s)", (desc->status & PP2_RX_L4_CHK_OK_MASK) ? "Ok" : "Bad");
	else
		printk(KERN_CONT "Unknown");

	printk(KERN_CONT "\n");

	printk(KERN_INFO "Lookup_ID=0x%x, cpu_code=0x%x\n",
		(desc->parserInfo & PP2_RX_LKP_ID_MASK) >> PP2_RX_LKP_ID_OFFS,
		(desc->parserInfo & PP2_RX_CPU_CODE_MASK) >> PP2_RX_CPU_CODE_OFFS);
}
EXPORT_SYMBOL(mv_eth_rx_desc_print);

void mv_eth_tx_desc_print(struct pp2_tx_desc *desc)
{
	int i;
	u32 *words = (u32 *) desc;

	printk(KERN_ERR "TX desc - %p: ", desc);
	for (i = 0; i < 8; i++)
		printk(KERN_CONT "%8.8x ", *words++);
	printk(KERN_CONT "\n");
}
EXPORT_SYMBOL(mv_eth_tx_desc_print);

void mv_eth_pkt_print(struct eth_pbuf *pkt)
{
	printk(KERN_ERR "pkt: len=%d off=%d pool=%d "
	       "skb=%p pa=%lx buf=%p\n",
	       pkt->bytes, pkt->offset, pkt->pool,
	       pkt->osInfo, pkt->physAddr, pkt->pBuf);

	mvDebugMemDump(pkt->pBuf + pkt->offset, 64, 1);
	mvOsCacheInvalidate(NULL, pkt->pBuf + pkt->offset, 64);
}
EXPORT_SYMBOL(mv_eth_pkt_print);

static inline int mv_eth_tx_done_policy(u32 cause)
{
	return fls(cause) - 1;
}

inline int mv_eth_rx_policy(u32 cause)
{
	return fls(cause) - 1;
}

static inline int mv_eth_txq_dscp_map_get(struct eth_port *pp, MV_U8 dscp)
{
	MV_U8 q = pp->txq_dscp_map[dscp];

	if (q == MV_ETH_TXQ_INVALID)
		return pp->cpu_config[smp_processor_id()]->txq;

	return q;
}

static inline int mv_eth_tx_policy(struct eth_port *pp, struct sk_buff *skb)
{
	int txq = pp->cpu_config[smp_processor_id()]->txq;

	if (skb->protocol == htons(ETH_P_IP)) {
		struct iphdr *iph = ip_hdr(skb);

		txq = mv_eth_txq_dscp_map_get(pp, TOS_TO_DSCP(iph->tos));
	}
	return txq;
}

#ifdef CONFIG_NET_SKB_RECYCLE
int mv_eth_skb_recycle(struct sk_buff *skb)
{
	int pool, cpu;
	__u32 bm = skb->hw_cookie;
	phys_addr_t phys_addr;
	struct bm_pool *ppool;
	bool is_recyclable;

	skb->hw_cookie = 0;
	skb->skb_recycle = NULL;

	cpu = mv_eth_bm_cookie_cpu_get(bm);

	pool = mv_eth_bm_cookie_pool_get(bm);
	if (mvPp2MaxCheck(pool, MV_ETH_BM_POOLS, "bm_pool"))
		return 1;

	ppool = &mv_eth_pool[pool];

	/*
	WA for Linux network stack issue that prevent skb recycle.
	If dev_kfree_skb_any called from interrupt context or interrupts disabled context
	skb->users will be zero when skb_recycle callback function is called.
	In such case skb_recycle_check function returns error because skb->users != 1.
	*/
	if (atomic_read(&skb->users) == 0)
		atomic_set(&skb->users, 1);

	if (bm & MV_ETH_BM_COOKIE_F_INVALID) {
		/* hw_cookie is not valid for recycle */
		STAT_DBG(ppool->stats.bm_cookie_err++);
		is_recyclable = false;
	} else if (!skb_recycle_check(skb, ppool->pkt_size)) {
		STAT_DBG(ppool->stats.skb_recycled_err++);
		is_recyclable = false;
	} else
		is_recyclable = true;

	if (is_recyclable) {
#ifdef CONFIG_MV_ETH_DEBUG_CODE
		/* Sanity check */
		if (SKB_TRUESIZE(skb->end - skb->head) != skb->truesize) {
			pr_err("%s: skb=%p, Wrong SKB_TRUESIZE(end - head)=%d\n",
				__func__, skb, SKB_TRUESIZE(skb->end - skb->head));
			mv_eth_skb_print(skb);
		}
#endif /* CONFIG_MV_ETH_DEBUG_CODE */

		STAT_DBG(ppool->stats.skb_recycled_ok++);

		phys_addr = dma_map_single(NULL, skb->head, RX_BUF_SIZE(ppool->pkt_size), DMA_FROM_DEVICE);
		/*phys_addr = virt_to_phys(skb->head);*/
#ifdef CONFIG_MV_ETH_SWF_HWF_CORRUPTION_WA
		/* Invalidate only part of the buffer used by CPU */
		if ((ppool->type == MV_ETH_BM_MIXED_LONG) || (ppool->type == MV_ETH_BM_MIXED_SHORT))
			mv_eth_iocc_l1_l2_cache_inv(skb->head, skb->len + skb_headroom(skb));
#endif /* CONFIG_MV_ETH_SWF_HWF_CORRUPTION_WA */
	} else {
/*
		pr_err("%s: Failed - skb=%p, pool=%d, bm_cookie=0x%x\n",
			__func__, skb, MV_ETH_BM_COOKIE_POOL(bm), bm.word);

		mv_eth_skb_print(skb);
*/
		skb = mv_eth_skb_alloc(ppool, &phys_addr,  GFP_ATOMIC);
		if (!skb) {
			pr_err("Linux processing - Can't refill\n");
			return 1;
		}
	}
	mv_eth_pool_refill(ppool, bm, phys_addr, (unsigned long) skb);
	atomic_dec(&ppool->in_use);

#ifdef CONFIG_MV_ETH_DEBUG_CODE
/*
	if (cpu != smp_processor_id()) {
		pr_warning("%s on CPU=%d other than RX=%d\n", __func__,
			smp_processor_id(), cpu);
	}
*/
#endif /* CONFIG_MV_ETH_DEBUG_CODE */

	return !is_recyclable;
}
EXPORT_SYMBOL(mv_eth_skb_recycle);

#endif /* CONFIG_NET_SKB_RECYCLE */

static struct sk_buff *mv_eth_skb_alloc(struct bm_pool *pool, phys_addr_t *phys_addr, gfp_t gfp_mask)
{
	struct sk_buff *skb;
	phys_addr_t pa;

	skb = __dev_alloc_skb(pool->pkt_size, gfp_mask);
	if (!skb) {
		STAT_ERR(pool->stats.skb_alloc_oom++);
		return NULL;
	}
	/* pa = virt_to_phys(skb->head); */
	if (phys_addr) {
		pa = dma_map_single(NULL, skb->head, RX_BUF_SIZE(pool->pkt_size), DMA_FROM_DEVICE);
		*phys_addr = pa;

#ifdef CONFIG_MV_ETH_SWF_HWF_CORRUPTION_WA
		if ((pool->type == MV_ETH_BM_MIXED_LONG) || (pool->type == MV_ETH_BM_MIXED_SHORT))
			mv_eth_iocc_l1_l2_cache_inv(skb->head, RX_BUF_SIZE(pool->pkt_size));
#endif
	}

	STAT_DBG(pool->stats.skb_alloc_ok++);

	return skb;
}

static unsigned char *mv_eth_hwf_buff_alloc(struct bm_pool *pool, phys_addr_t *phys_addr)
{
	unsigned char *buff;
	int size = RX_HWF_BUF_SIZE(pool->pkt_size);

	buff = mvOsMalloc(size);
	if (!buff)
		return NULL;

	if (phys_addr != NULL)
		*phys_addr = mvOsCacheInvalidate(NULL, buff, size);

	return buff;
}

static inline void mv_eth_txq_buf_free(struct eth_port *pp, u32 shadow)
{
	if (!shadow)
		return;

	if (shadow & MV_ETH_SHADOW_SKB) {
		shadow &= ~MV_ETH_SHADOW_SKB;
		dev_kfree_skb_any((struct sk_buff *)shadow);
		STAT_DBG(pp->stats.tx_skb_free++);
	} else if (shadow & MV_ETH_SHADOW_EXT) {
		shadow &= ~MV_ETH_SHADOW_EXT;
		mv_eth_extra_pool_put(pp, (void *)shadow);
	} else {
		/* TBD - return buffer back to BM */
		printk(KERN_ERR "%s: unexpected buffer - not skb and not ext\n", __func__);
	}
}


static inline void mv_eth_txq_bufs_free(struct eth_port *pp, struct txq_cpu_ctrl *txq_cpu, int num)
{
	u32 shadow;
	int i;

	/* Free buffers that was not freed automatically by BM */
	for (i = 0; i < num; i++) {
		shadow = mv_eth_shadow_get_pop(txq_cpu);
		mv_eth_txq_buf_free(pp, shadow);
	}
}

inline u32 mv_eth_txq_done(struct eth_port *pp, struct tx_queue *txq_ctrl)
{
	int tx_done;
	struct txq_cpu_ctrl *txq_cpu_ptr = &txq_ctrl->txq_cpu[smp_processor_id()];

	/* get number of transmitted TX descriptors by this CPU */
	tx_done = mvPp2TxqSentDescProc(pp->port, txq_ctrl->txp, txq_ctrl->txq);
	if (!tx_done)
		return tx_done;
/*
	printk(KERN_ERR "tx_done: txq_count=%d, port=%d, txp=%d, txq=%d, tx_done=%d\n",
			txq_ctrl->txq_count, pp->port, txq_ctrl->txp, txq_ctrl->txq, tx_done);
*/
	/* packet sent by outer tx function */
	if (txq_cpu_ptr->txq_count < tx_done)
		return tx_done;

	mv_eth_txq_bufs_free(pp, txq_cpu_ptr, tx_done);

	txq_cpu_ptr->txq_count -= tx_done;
	STAT_DBG(txq_cpu_ptr->stats.txq_txdone += tx_done);

	return tx_done;
}
EXPORT_SYMBOL(mv_eth_txq_done);

/* Reuse skb if possible, allocate new skb and move to BM pool */
inline int mv_eth_refill(struct bm_pool *ppool, __u32 bm, int is_recycle)
{
	struct sk_buff *skb;
	phys_addr_t phys_addr;

	if (is_recycle && (mv_eth_bm_in_use_read(ppool) < ppool->in_use_thresh))
		return 0;

	/* No recycle or too many buffers are in use - alloc new skb */
	skb = mv_eth_skb_alloc(ppool, &phys_addr, GFP_ATOMIC);
	if (!skb) {
		pr_err("Linux processing - Can't refill\n");
		return 1;
	}
	STAT_DBG(ppool->stats.no_recycle++);

	mv_eth_pool_refill(ppool, bm, phys_addr, (unsigned long) skb);
	atomic_dec(&ppool->in_use);

	return 0;
}
EXPORT_SYMBOL(mv_eth_refill);

static inline MV_U32 mv_eth_skb_tx_csum(struct eth_port *pp, struct sk_buff *skb)
{
	if (skb->ip_summed == CHECKSUM_PARTIAL) {
		int   ip_hdr_len = 0;
		MV_U8 l4_proto;

		if (skb->protocol == htons(ETH_P_IP)) {
			struct iphdr *ip4h = ip_hdr(skb);

			/* Calculate IPv4 checksum and L4 checksum */
			ip_hdr_len = ip4h->ihl;
			l4_proto = ip4h->protocol;
		} else if (skb->protocol == htons(ETH_P_IPV6)) {
			/* If not IPv4 - must be ETH_P_IPV6 - Calculate only L4 checksum */
			struct ipv6hdr *ip6h = ipv6_hdr(skb);

			/* Read l4_protocol from one of IPv6 extra headers ?????? */
			if (skb_network_header_len(skb) > 0)
				ip_hdr_len = (skb_network_header_len(skb) >> 2);
			l4_proto = ip6h->nexthdr;
		} else {
			STAT_DBG(pp->stats.tx_csum_sw++);
			return PP2_TX_L4_CSUM_NOT;
		}
		STAT_DBG(pp->stats.tx_csum_hw++);

		return mvPp2TxqDescCsum(skb_network_offset(skb), skb->protocol, ip_hdr_len, l4_proto);
	}

	STAT_DBG(pp->stats.tx_csum_sw++);
	return PP2_TX_L4_CSUM_NOT | PP2_TX_IP_CSUM_DISABLE_MASK;
}

inline struct pp2_rx_desc *mv_eth_rx_prefetch(struct eth_port *pp, MV_PP2_PHYS_RXQ_CTRL *rx_ctrl,
									  int rx_done, int rx_todo)
{
	struct pp2_rx_desc	*rx_desc, *next_desc;

	rx_desc = mvPp2RxqNextDescGet(rx_ctrl);
	if (rx_done == 0) {
		/* First descriptor in the NAPI loop */
		mvOsCacheLineInv(NULL, rx_desc);
		prefetch(rx_desc);
	}
	if ((rx_done + 1) == rx_todo) {
		/* Last descriptor in the NAPI loop - prefetch are not needed */
		return rx_desc;
	}
	/* Prefetch next descriptor */
	next_desc = mvPp2RxqDescGet(rx_ctrl);
	mvOsCacheLineInv(NULL, next_desc);
	prefetch(next_desc);

	return rx_desc;
}

void mv_eth_buff_hdr_rx(struct eth_port *pp, struct pp2_rx_desc *rx_desc)
{
	u32 rx_status = rx_desc->status;
	int mc_id, pool_id;
	PP2_BUFF_HDR *buff_hdr;
	MV_U32 buff_phys_addr, buff_virt_addr, buff_phys_addr_next, buff_virt_addr_next;
	int count = 0;

#ifdef CONFIG_MV_ETH_PP2_1
	int qset, is_grntd;

	qset = (rx_desc->bmQset & PP2_RX_BUFF_QSET_NUM_MASK) >> PP2_RX_BUFF_QSET_NUM_OFFS;
	is_grntd = (rx_desc->bmQset & PP2_RX_BUFF_TYPE_MASK) >> PP2_RX_BUFF_TYPE_OFFS;
#endif

	pool_id = (rx_status & PP2_RX_BM_POOL_ALL_MASK) >> PP2_RX_BM_POOL_ID_OFFS;
	buff_phys_addr = rx_desc->bufPhysAddr;
	buff_virt_addr = rx_desc->bufCookie;

	do {
		buff_hdr = (PP2_BUFF_HDR *)(((struct sk_buff *)buff_virt_addr)->head);
		mc_id = PP2_BUFF_HDR_INFO_MC_ID(buff_hdr->info);

#ifdef CONFIG_MV_ETH_DEBUG_CODE
		if (pp->dbg_flags & MV_ETH_F_DBG_BUFF_HDR) {
			printk(KERN_ERR "buff header #%d:\n", count);
			mvDebugMemDump(buff_hdr, 32, 1);

			printk(KERN_ERR "byte count = %d   MC ID = %d   last = %d\n",
				buff_hdr->byteCount, mc_id,
				PP2_BUFF_HDR_INFO_IS_LAST(buff_hdr->info));
		}
#endif

		count++;
		buff_phys_addr_next = buff_hdr->nextBuffPhysAddr;
		buff_virt_addr_next = buff_hdr->nextBuffVirtAddr;

		/* release buffer */
#ifdef CONFIG_MV_ETH_PP2_1
		mvBmPoolQsetMcPut(pool_id, buff_phys_addr, buff_virt_addr, qset, is_grntd, mc_id, 0);

		/* Qset number and buffer type of next buffer */
		qset = (buff_hdr->bmQset & PP2_BUFF_HDR_BM_QSET_NUM_MASK) >> PP2_BUFF_HDR_BM_QSET_NUM_OFFS;
		is_grntd = (buff_hdr->bmQset & PP2_BUFF_HDR_BM_QSET_TYPE_MASK) >> PP2_BUFF_HDR_BM_QSET_TYPE_OFFS;
#else
		mvBmPoolMcPut(pool_id, buff_phys_addr, buff_virt_addr, mc_id, 0);
#endif
		buff_phys_addr = buff_phys_addr_next;
		buff_virt_addr = buff_virt_addr_next;

		STAT_DBG((&mv_eth_pool[pool_id])->stats.bm_put++);

	} while (!PP2_BUFF_HDR_INFO_IS_LAST(buff_hdr->info));

	mvOsCacheLineInv(NULL, rx_desc);
	STAT_INFO(pp->stats.rx_buf_hdr++);
}
EXPORT_SYMBOL(mv_eth_buff_hdr_rx);


void mv_eth_buff_hdr_rx_dump(struct eth_port *pp, struct pp2_rx_desc *rx_desc)
{
	int mc_id;
	PP2_BUFF_HDR *buff_hdr;
	MV_U32 buff_phys_addr, buff_virt_addr;

	int count = 0;

	buff_phys_addr = rx_desc->bufPhysAddr;
	buff_virt_addr = rx_desc->bufCookie;
		printk(KERN_ERR "------------------------\n");
	do {
		printk(KERN_ERR "buff_virt_addr = %x\n", buff_virt_addr);
		buff_hdr = (PP2_BUFF_HDR *)(((struct eth_pbuf *)buff_virt_addr)->pBuf);

		printk(KERN_ERR "buff_hdr = %p\n", buff_hdr);
		mc_id = PP2_BUFF_HDR_INFO_MC_ID(buff_hdr->info);

		printk(KERN_ERR "buff header #%d:\n", ++count);
		mvDebugMemDump(buff_hdr, buff_hdr->byteCount, 1);

		printk(KERN_ERR "byte count = %d   MC ID = %d   last = %d\n",
			buff_hdr->byteCount, mc_id,
			PP2_BUFF_HDR_INFO_IS_LAST(buff_hdr->info));

		buff_phys_addr = buff_hdr->nextBuffPhysAddr;
		buff_virt_addr  = buff_hdr->nextBuffVirtAddr;

	} while (!PP2_BUFF_HDR_INFO_IS_LAST(buff_hdr->info));

}

static inline int mv_eth_rx(struct eth_port *pp, int rx_todo, int rxq, struct napi_struct *napi)
{
	struct net_device *dev = pp->dev;
	MV_PP2_PHYS_RXQ_CTRL *rx_ctrl = pp->rxq_ctrl[rxq].q;
	int rx_done, pool;
	struct pp2_rx_desc *rx_desc;
	u32 rx_status;
	int rx_bytes;
	struct sk_buff *skb;
	__u32 bm;
	struct bm_pool *ppool;
#ifdef CONFIG_NETMAP
	if (pp->flags & MV_ETH_F_IFCAP_NETMAP) {
		int netmap_done;
		if (netmap_rx_irq(pp->dev, 0, &netmap_done))
			return 1; /* seems to be ignored */
	}
#endif /* CONFIG_NETMAP */
	/* Get number of received packets */
	rx_done = mvPp2RxqBusyDescNumGet(pp->port, rxq);
	mvOsCacheIoSync(NULL);

	if ((rx_todo > rx_done) || (rx_todo < 0))
		rx_todo = rx_done;

	if (rx_todo == 0)
		return 0;

	rx_done = 0;

	/* Fairness NAPI loop */
	while (rx_done < rx_todo) {

		if (pp->flags & MV_ETH_F_RX_DESC_PREFETCH)
			rx_desc = mv_eth_rx_prefetch(pp, rx_ctrl, rx_done, rx_todo);
		else {
			rx_desc = mvPp2RxqNextDescGet(rx_ctrl);
			mvOsCacheLineInv(NULL, rx_desc);
			prefetch(rx_desc);
		}
		rx_done++;

#if defined(MV_CPU_BE)
		mvPPv2RxqDescSwap(rx_desc);
#endif /* MV_CPU_BE */

#ifdef CONFIG_MV_ETH_DEBUG_CODE
		if (pp->dbg_flags & MV_ETH_F_DBG_RX) {
			printk(KERN_ERR "\n%s: port=%d, cpu=%d\n", __func__, pp->port, smp_processor_id());
			mv_eth_rx_desc_print(rx_desc);
		}
#endif /* CONFIG_MV_ETH_DEBUG_CODE */

		rx_status = rx_desc->status;
		bm = mv_eth_bm_cookie_build(rx_desc);
		pool = mv_eth_bm_cookie_pool_get(bm);
		ppool = &mv_eth_pool[pool];

		/* check if buffer header is used */
		if ((rx_status & (PP2_RX_HWF_SYNC_MASK | PP2_RX_BUF_HDR_MASK)) == PP2_RX_BUF_HDR_MASK) {
			mv_eth_buff_hdr_rx(pp, rx_desc);
			continue;
		}

		if (rx_status & PP2_RX_ES_MASK) {
			mv_eth_rx_error(pp, rx_desc);

			mv_eth_pool_refill(ppool, bm, rx_desc->bufPhysAddr, rx_desc->bufCookie);
			mvOsCacheLineInv(NULL, rx_desc);
			continue;
		}
		skb = (struct sk_buff *)rx_desc->bufCookie;

		if (rx_status & PP2_RX_HWF_SYNC_MASK) {
			/* Remember sync bit for TX */
			pr_info("\n%s: port=%d, rxq=%d, cpu=%d, skb=%p - Sync packet received\n",
			__func__, pp->port, rxq, smp_processor_id(), skb);
			mv_eth_rx_desc_print(rx_desc);
			sync_head = skb->head;
			sync_rx_desc = rx_status;
		}

		/*dma_unmap_single(NULL, rx_desc->bufPhysAddr, RX_BUF_SIZE(ppool->pkt_size), DMA_FROM_DEVICE);*/

		/* Prefetch two cache lines from beginning of packet */
		if (pp->flags & MV_ETH_F_RX_PKT_PREFETCH) {
			prefetch(skb->data);
			prefetch(skb->data + CPU_D_CACHE_LINE_SIZE);
		}

		atomic_inc(&ppool->in_use);
		STAT_DBG(pp->stats.rxq[rxq]++);
		dev->stats.rx_packets++;

		rx_bytes = rx_desc->dataSize;
		dev->stats.rx_bytes += rx_bytes;

#ifdef CONFIG_MV_ETH_DEBUG_CODE
		if (pp->dbg_flags & MV_ETH_F_DBG_RX) {
			printk(KERN_ERR "skb=%p, buf=%p, ksize=%d\n", skb, skb->head, ksize(skb->head));
			mvDebugMemDump(skb->head + NET_SKB_PAD, 64, 1);
		}
#endif /* CONFIG_MV_ETH_DEBUG_CODE */

		/* Linux processing */
		__skb_put(skb, rx_bytes);

#if defined(CONFIG_MV_ETH_RX_SPECIAL)
		/* Special RX processing */
		if (mvPp2IsRxSpecial(rx_desc->parserInfo)) {
			if (pp->rx_special_proc) {
				pp->rx_special_proc(pp->port, rxq, dev, skb, rx_desc);
				STAT_INFO(pp->stats.rx_special++);

				/* Refill processing */
				mv_eth_refill(ppool, bm, 0);
				mvOsCacheLineInv(NULL, rx_desc);
				continue;
			}
		}
#endif /* CONFIG_MV_ETH_RX_SPECIAL */

#ifdef CONFIG_NET_SKB_RECYCLE
		if (mv_eth_is_recycle()) {
			skb->skb_recycle = mv_eth_skb_recycle;
			skb->hw_cookie = bm;
		}
#endif /* CONFIG_NET_SKB_RECYCLE */

		mv_eth_rx_csum(pp, rx_desc, skb);

		if (pp->tagged) {
			mv_mux_rx(skb, pp->port, napi);
			STAT_DBG(pp->stats.rx_tagged++);
			skb = NULL;
		} else {
			dev->stats.rx_bytes -= mv_eth_mh_skb_skip(skb);
			skb->protocol = eth_type_trans(skb, dev);
		}

		if (skb && (dev->features & NETIF_F_GRO)) {
			STAT_DBG(pp->stats.rx_gro++);
			STAT_DBG(pp->stats.rx_gro_bytes += skb->len);

			rx_status = napi_gro_receive(napi, skb);
			skb = NULL;
		}

		if (skb) {
			STAT_DBG(pp->stats.rx_netif++);
			rx_status = netif_receive_skb(skb);
			STAT_DBG((rx_status == 0) ? pp->stats.rx_drop_sw += 0 : pp->stats.rx_drop_sw++);
		}

		/* Refill processing: */
		mv_eth_refill(ppool, bm, mv_eth_is_recycle());
		mvOsCacheLineInv(NULL, rx_desc);
	}

	/* Update RxQ management counters */
	wmb();
	mvPp2RxqDescNumUpdate(pp->port, rxq, rx_done, rx_done);

	return rx_done;
}

static int mv_eth_tx(struct sk_buff *skb, struct net_device *dev)
{
	struct eth_port *pp = MV_ETH_PRIV(dev);
	int frags = 0, cpu = smp_processor_id();
	u32 tx_cmd, bufPhysAddr;
	struct mv_eth_tx_spec tx_spec, *tx_spec_ptr = NULL;
	struct tx_queue *txq_ctrl = NULL;
	struct txq_cpu_ctrl *txq_cpu_ptr = NULL;
	struct aggr_tx_queue *aggr_txq_ctrl = NULL;
	struct pp2_tx_desc *tx_desc;
	unsigned long flags = 0;

	if (!test_bit(MV_ETH_F_STARTED_BIT, &(pp->flags))) {
		STAT_INFO(pp->stats.netdev_stop++);
#ifdef CONFIG_MV_ETH_DEBUG_CODE
		if (pp->dbg_flags & MV_ETH_F_DBG_TX)
			printk(KERN_ERR "%s: STARTED_BIT = 0, packet is dropped.\n", __func__);
#endif /* CONFIG_MV_ETH_DEBUG_CODE */
		goto out;
	}

	if (!(netif_running(dev))) {
		printk(KERN_ERR "!netif_running() in %s\n", __func__);
		goto out;
	}

#if defined(CONFIG_MV_ETH_TX_SPECIAL)
	if (pp->tx_special_check) {

		if (pp->tx_special_check(pp->port, dev, skb, &tx_spec)) {
			STAT_INFO(pp->stats.tx_special++);
			if (tx_spec.tx_func) {
				tx_spec.tx_func(skb->data, skb->len, &tx_spec);
				goto out;
			} else {
				/* Check validity of tx_spec txp/txq must be CPU owned */
				tx_spec_ptr = &tx_spec;
			}
		}
	}
#endif /* CONFIG_MV_ETH_TX_SPECIAL */

	/* In case this port is tagged, check if SKB is tagged - i.e. SKB's source is MUX interface */
	if (pp->tagged && (!MV_MUX_SKB_IS_TAGGED(skb))) {
#ifdef CONFIG_MV_ETH_DEBUG_CODE
		if (pp->dbg_flags & MV_ETH_F_DBG_TX)
			pr_err("%s: port %d is tagged, skb not from MUX interface - packet is dropped.\n",
				__func__, pp->port);
#endif /* CONFIG_MV_ETH_DEBUG_CODE */

		goto out;
	}

	/* Get TXQ (without BM) to send packet generated by Linux */
	if (tx_spec_ptr == NULL) {
		tx_spec_ptr = &pp->tx_spec;
		tx_spec_ptr->txq = mv_eth_select_txq(dev, skb);
	}

	aggr_txq_ctrl = &aggr_txqs[smp_processor_id()];
	txq_ctrl = &pp->txq_ctrl[tx_spec_ptr->txp * CONFIG_MV_ETH_TXQ + tx_spec_ptr->txq];
	if (txq_ctrl == NULL) {
		printk(KERN_ERR "%s: invalidate txp/txq (%d/%d)\n",
			__func__, tx_spec_ptr->txp, tx_spec_ptr->txq);
		goto out;
	}
	txq_cpu_ptr = &txq_ctrl->txq_cpu[cpu];

	MV_ETH_LIGHT_LOCK(flags);

#ifdef CONFIG_MV_ETH_TSO
	/* GSO/TSO */
	if (skb_is_gso(skb)) {
		frags = mv_eth_tx_tso(skb, dev, tx_spec_ptr, txq_ctrl, aggr_txq_ctrl);
		goto out;
	}
#endif /* CONFIG_MV_ETH_TSO */

	frags = skb_shinfo(skb)->nr_frags + 1;

	if (tx_spec_ptr->flags & MV_ETH_TX_F_MH) {
		if (mv_eth_skb_mh_add(skb, tx_spec_ptr->tx_mh)) {
			frags = 0;
			goto out;
		}
	}

	/* is enough descriptors? */
#ifdef CONFIG_MV_ETH_PP2_1
	if (mv_eth_reserved_desc_num_proc(pp, tx_spec_ptr->txp, tx_spec_ptr->txq, frags) ||
		mv_eth_aggr_desc_num_check(aggr_txq_ctrl, frags)) {
#else
	if (mv_eth_phys_desc_num_check(txq_cpu_ptr, frags) ||
		mv_eth_aggr_desc_num_check(aggr_txq_ctrl, frags)) {

#endif

		frags = 0;
		goto out;
	}

	tx_desc = mvPp2AggrTxqNextDescGet(aggr_txq_ctrl->q);

	tx_desc->physTxq = MV_PPV2_TXQ_PHYS(pp->port, tx_spec_ptr->txp, tx_spec_ptr->txq);

	/* Don't use BM for Linux packets: NETA_TX_BM_ENABLE_MASK = 0 */
	/* NETA_TX_PKT_OFFSET_MASK = 0 - for all descriptors */
	tx_cmd = mv_eth_skb_tx_csum(pp, skb);

	if (tx_spec_ptr->flags & MV_ETH_TX_F_HW_CMD) {
		tx_desc->hwCmd[0] = tx_spec_ptr->hw_cmd[0];
		tx_desc->hwCmd[1] = tx_spec_ptr->hw_cmd[1];
		tx_desc->hwCmd[2] = tx_spec_ptr->hw_cmd[2];
	}

	if (skb->head == sync_head)
		tx_cmd |= PP2_TX_HWF_SYNC_MASK;

	/* FIXME: beware of nonlinear --BK */
	tx_desc->dataSize = skb_headlen(skb);
	bufPhysAddr = mvOsCacheFlush(NULL, skb->data, tx_desc->dataSize);
	tx_desc->pktOffset = bufPhysAddr & MV_ETH_TX_DESC_ALIGN;
	tx_desc->bufPhysAddr = bufPhysAddr & (~MV_ETH_TX_DESC_ALIGN);

	if (frags == 1) {
		/*
		 * First and Last descriptor
		 */
		if (tx_spec_ptr->flags & MV_ETH_TX_F_NO_PAD)
			tx_cmd |= PP2_TX_F_DESC_MASK | PP2_TX_L_DESC_MASK | PP2_TX_PADDING_DISABLE_MASK;
		else
			tx_cmd |= PP2_TX_F_DESC_MASK | PP2_TX_L_DESC_MASK;

		tx_desc->command = tx_cmd;
		mv_eth_tx_desc_flush(tx_desc);

		mv_eth_shadow_push(txq_cpu_ptr, ((MV_ULONG) skb | MV_ETH_SHADOW_SKB));
	} else {
		/* First but not Last */
		tx_cmd |= PP2_TX_F_DESC_MASK | PP2_TX_PADDING_DISABLE_MASK;

		mv_eth_shadow_push(txq_cpu_ptr, 0);

		tx_desc->command = tx_cmd;
		mv_eth_tx_desc_flush(tx_desc);

		/* Continue with other skb fragments */
		mv_eth_tx_frag_process(pp, skb, aggr_txq_ctrl, txq_ctrl, tx_spec_ptr);
		STAT_DBG(pp->stats.tx_sg++);
	}

#ifdef CONFIG_MV_ETH_PP2_1
	/* PPv2.1 - MAS 3.16, decrease number of reserved descriptors */
	txq_cpu_ptr->reserved_num -= frags;
#endif

	txq_cpu_ptr->txq_count += frags;
	aggr_txq_ctrl->txq_count += frags;

	if (tx_cmd & PP2_TX_HWF_SYNC_MASK) {
		pr_info("%s: port=%d, txp=%d, txq=%d, cpu=%d, skb=%p, rx_desc=0x%08x - Sync packet transmitted\n",
			__func__, pp->port, tx_spec_ptr->txp, tx_spec_ptr->txq, smp_processor_id(),
			skb, sync_rx_desc);
		mv_eth_tx_desc_print(tx_desc);
		mvDebugMemDump(skb->data, 64, 1);
		sync_head = NULL;
		sync_rx_desc = 0;
	}
#ifdef CONFIG_MV_ETH_DEBUG_CODE
	if (pp->dbg_flags & MV_ETH_F_DBG_TX) {
		printk(KERN_ERR "\n");
		printk(KERN_ERR "%s - eth_tx_%lu: cpu=%d, in_intr=0x%lx, port=%d, txp=%d, txq=%d\n",
			dev->name, dev->stats.tx_packets, smp_processor_id(), in_interrupt(),
			pp->port, tx_spec_ptr->txp, tx_spec_ptr->txq);
		printk(KERN_ERR "\t skb=%p, head=%p, data=%p, size=%d\n", skb, skb->head, skb->data, skb->len);
		pr_info("\t sync_head=%p, sync_rx_desc=0x%08x\n", sync_head, sync_rx_desc);
		mv_eth_tx_desc_print(tx_desc);
		/*mv_eth_skb_print(skb);*/
		mvDebugMemDump(skb->data, 64, 1);
	}
#endif /* CONFIG_MV_ETH_DEBUG_CODE */
	/* Enable transmit */
	wmb();
	mvPp2AggrTxqPendDescAdd(frags);

	STAT_DBG(aggr_txq_ctrl->stats.txq_tx += frags);
	STAT_DBG(txq_cpu_ptr->stats.txq_tx += frags);

out:
	if (frags > 0) {
		dev->stats.tx_packets++;
		dev->stats.tx_bytes += skb->len;
	} else {
		dev->stats.tx_dropped++;
		dev_kfree_skb_any(skb);
	}

#ifndef CONFIG_MV_ETH_TXDONE_ISR
	if (txq_ctrl) {
		if (txq_cpu_ptr->txq_count >= mv_ctrl_txdone) {
#ifdef CONFIG_MV_ETH_STAT_DIST
			u32 tx_done = mv_eth_txq_done(pp, txq_ctrl);

			if (tx_done < pp->dist_stats.tx_done_dist_size)
				pp->dist_stats.tx_done_dist[tx_done]++;
#else
			mv_eth_txq_done(pp, txq_ctrl);
#endif /* CONFIG_MV_ETH_STAT_DIST */
		}
		/* If after calling mv_eth_txq_done, txq_ctrl->txq_count equals frags, we need to set the timer */
		if ((txq_cpu_ptr->txq_count == frags) && (frags > 0))
			mv_eth_add_tx_done_timer(pp->cpu_config[smp_processor_id()]);
	}
#endif /* CONFIG_MV_ETH_TXDONE_ISR */

	if (txq_ctrl)
		MV_ETH_LIGHT_UNLOCK(flags);

	return NETDEV_TX_OK;
}

#ifdef CONFIG_MV_ETH_TSO
/* Validate TSO */
static inline int mv_eth_tso_validate(struct sk_buff *skb, struct net_device *dev)
{
	if (!(dev->features & NETIF_F_TSO)) {
		pr_err("error: (skb_is_gso(skb) returns true but features is not NETIF_F_TSO\n");
		return 1;
	}
	if (skb_shinfo(skb)->frag_list != NULL) {
		pr_err("***** ERROR: frag_list is not null\n");
		return 1;
	}
	if (skb_shinfo(skb)->gso_segs == 1) {
		pr_err("***** ERROR: only one TSO segment\n");
		return 1;
	}
	if (skb->len <= skb_shinfo(skb)->gso_size) {
		pr_err("***** ERROR: total_len (%d) less than gso_size (%d)\n", skb->len, skb_shinfo(skb)->gso_size);
		return 1;
	}
	if ((htons(ETH_P_IP) != skb->protocol) || (ip_hdr(skb)->protocol != IPPROTO_TCP) || (tcp_hdr(skb) == NULL)) {
		pr_err("***** ERROR: Protocol is not TCP over IP\n");
		return 1;
	}

	return 0;
}

static inline int mv_eth_tso_build_hdr_desc(struct pp2_tx_desc *tx_desc, struct eth_port *priv, struct sk_buff *skb,
					     struct txq_cpu_ctrl *txq_ctrl, u16 *mh, int hdr_len, int size,
					     MV_U32 tcp_seq, MV_U16 ip_id, int left_len)
{
	struct iphdr *iph;
	struct tcphdr *tcph;
	MV_U8 *data, *mac;
	MV_U32 bufPhysAddr;
	int mac_hdr_len = skb_network_offset(skb);

	data = mv_eth_extra_pool_get(priv);
	if (!data) {
		pr_err("Can't allocate extra buffer for TSO\n");
		return 0;
	}
	mv_eth_shadow_push(txq_ctrl, ((MV_ULONG)data | MV_ETH_SHADOW_EXT));

	/* Reserve 2 bytes for IP header alignment */
	mac = data + MV_ETH_MH_SIZE;
	iph = (struct iphdr *)(mac + mac_hdr_len);

	memcpy(mac, skb->data, hdr_len);

	if (iph) {
		iph->id = htons(ip_id);
		iph->tot_len = htons(size + hdr_len - mac_hdr_len);
	}

	tcph = (struct tcphdr *)(mac + skb_transport_offset(skb));
	tcph->seq = htonl(tcp_seq);

	if (left_len) {
		/* Clear all special flags for not last packet */
		tcph->psh = 0;
		tcph->fin = 0;
		tcph->rst = 0;
	}

	if (mh) {
		/* Start tarnsmit from MH - add 2 bytes to size */
		*((MV_U16 *)data) = *mh;
		/* increment ip_offset field in TX descriptor by 2 bytes */
		mac_hdr_len += MV_ETH_MH_SIZE;
		hdr_len += MV_ETH_MH_SIZE;
	} else {
		/* Start transmit from MAC */
		data = mac;
	}

	tx_desc->dataSize = hdr_len;
	tx_desc->command = mvPp2TxqDescCsum(mac_hdr_len, skb->protocol, ((u8 *)tcph - (u8 *)iph) >> 2, IPPROTO_TCP);
	tx_desc->command |= PP2_TX_F_DESC_MASK;

	bufPhysAddr = mvOsCacheFlush(NULL, data, tx_desc->dataSize);
	tx_desc->pktOffset = bufPhysAddr & MV_ETH_TX_DESC_ALIGN;
	tx_desc->bufPhysAddr = bufPhysAddr & (~MV_ETH_TX_DESC_ALIGN);

	mv_eth_tx_desc_flush(tx_desc);

	return hdr_len;
}

static inline int mv_eth_tso_build_data_desc(struct pp2_tx_desc *tx_desc, struct sk_buff *skb,
					     struct txq_cpu_ctrl *txq_ctrl, char *frag_ptr,
					     int frag_size, int data_left, int total_left)
{
	MV_U32 bufPhysAddr;
	int size, val = 0;

	size = MV_MIN(frag_size, data_left);

	tx_desc->dataSize = size;
	bufPhysAddr = mvOsCacheFlush(NULL, frag_ptr, size);
	tx_desc->pktOffset = bufPhysAddr & MV_ETH_TX_DESC_ALIGN;
	tx_desc->bufPhysAddr = bufPhysAddr & (~MV_ETH_TX_DESC_ALIGN);


	tx_desc->command = 0;

	if (size == data_left) {
		/* last descriptor in the TCP packet */
		tx_desc->command = PP2_TX_L_DESC_MASK;

		if (total_left == 0) {
			/* last descriptor in SKB */
			val = ((MV_ULONG) skb | MV_ETH_SHADOW_SKB);
		}
	}
	mv_eth_shadow_push(txq_ctrl, val);
	mv_eth_tx_desc_flush(tx_desc);

	return size;
}

/***********************************************************
 * mv_eth_tx_tso --                                        *
 *   send a packet.                                        *
 ***********************************************************/
int mv_eth_tx_tso(struct sk_buff *skb, struct net_device *dev, struct mv_eth_tx_spec *tx_spec,
			struct tx_queue *txq_ctrl, struct aggr_tx_queue *aggr_txq_ctrl)
{
	int ptxq, frag = 0;
	int total_len, hdr_len, size, frag_size, data_left;
	int total_desc_num, seg_desc_num, total_bytes = 0;
	char *frag_ptr;
	struct pp2_tx_desc *tx_desc;
	struct txq_cpu_ctrl *txq_cpu_ptr = NULL;
	MV_U16 ip_id, *mh = NULL;
	MV_U32 tcp_seq = 0;
	skb_frag_t *skb_frag_ptr;
	const struct tcphdr *th = tcp_hdr(skb);
	struct eth_port *priv = MV_ETH_PRIV(dev);

	STAT_DBG(priv->stats.tx_tso++);

	if (mv_eth_tso_validate(skb, dev))
		return 0;

	txq_cpu_ptr = &txq_ctrl->txq_cpu[smp_processor_id()];

	total_len = skb->len;
	hdr_len = (skb_transport_offset(skb) + tcp_hdrlen(skb));

	total_len -= hdr_len;
	ip_id = ntohs(ip_hdr(skb)->id);
	tcp_seq = ntohl(th->seq);

	frag_size = skb_headlen(skb);
	frag_ptr = skb->data;

	if (frag_size < hdr_len) {
		pr_err("***** ERROR: frag_size=%d, hdr_len=%d\n", frag_size, hdr_len);
		return 0;
	}

	/* Skip header - we'll add header in another buffer (from extra pool) */
	frag_size -= hdr_len;
	frag_ptr += hdr_len;

	/* A special case where the first skb's frag contains only the packet's header */
	if (frag_size == 0) {
		skb_frag_ptr = &skb_shinfo(skb)->frags[frag];

		/* Move to next segment */
		frag_size = skb_frag_ptr->size;
#if LINUX_VERSION_CODE > KERNEL_VERSION(3, 1, 10)
		frag_ptr = page_address(skb_frag_ptr->page.p) + skb_frag_ptr->page_offset;
#else
		frag_ptr = page_address(skb_frag_ptr->page) + skb_frag_ptr->page_offset;
#endif
		frag++;
	}
	total_desc_num = 0;
	ptxq = MV_PPV2_TXQ_PHYS(priv->port, tx_spec->txp, tx_spec->txq);

	/* Each iteration - create new TCP segment */
	while (total_len > 0) {
		data_left = MV_MIN(skb_shinfo(skb)->gso_size, total_len);

		/* Calculate maximum number of descriptors needed for the current TCP segment			  *
		 * At worst case we'll transmit all remaining frags including skb->data (one descriptor per frag) *
		 * We also need one descriptor for packet header						  */
		seg_desc_num = skb_shinfo(skb)->nr_frags - frag + 2;

		if (mv_eth_aggr_desc_num_check(aggr_txq_ctrl, seg_desc_num)) {
			STAT_DBG(priv->stats.tx_tso_no_resource++);
			return 0;
		}

		/* Check if there are enough descriptors in physical TXQ */
#ifdef CONFIG_MV_ETH_PP2_1
		if (mv_eth_reserved_desc_num_proc(priv, tx_spec->txp, tx_spec->txq, seg_desc_num)) {
#else
		if (mv_eth_phys_desc_num_check(txq_cpu_ptr, seg_desc_num)) {
#endif
			STAT_DBG(priv->stats.tx_tso_no_resource++);
			return 0;
		}

		seg_desc_num = 0;

		tx_desc = mvPp2AggrTxqNextDescGet(aggr_txq_ctrl->q);
		tx_desc->physTxq = ptxq;

		seg_desc_num++;
		total_desc_num++;
		total_len -= data_left;

		aggr_txq_ctrl->txq_count++;
		txq_cpu_ptr->txq_count++;

		if (tx_spec->flags & MV_ETH_TX_F_MH)
			mh = &tx_spec->tx_mh;

		/* prepare packet headers: MAC + IP + TCP */
		size = mv_eth_tso_build_hdr_desc(tx_desc, priv, skb, txq_cpu_ptr, mh,
					hdr_len, data_left, tcp_seq, ip_id, total_len);
		if (size == 0) {
			aggr_txq_ctrl->txq_count--;
			txq_cpu_ptr->txq_count--;
			mv_eth_shadow_dec_put(txq_cpu_ptr);
			mvPp2AggrTxqPrevDescGet(aggr_txq_ctrl->q);

			STAT_DBG(priv->stats.tx_tso_no_resource++);
			return 0;
		}
		total_bytes += size;

		/* Update packet's IP ID */
		ip_id++;

		while (data_left > 0) {
			tx_desc = mvPp2AggrTxqNextDescGet(aggr_txq_ctrl->q);
			tx_desc->physTxq = ptxq;

			seg_desc_num++;
			total_desc_num++;
			aggr_txq_ctrl->txq_count++;
			txq_cpu_ptr->txq_count++;

			size = mv_eth_tso_build_data_desc(tx_desc, skb, txq_cpu_ptr,
							  frag_ptr, frag_size, data_left, total_len);
			total_bytes += size;
			data_left -= size;

			/* Update TCP sequence number */
			tcp_seq += size;

			/* Update frag size, and offset */
			frag_size -= size;
			frag_ptr += size;

			if ((frag_size == 0) && (frag < skb_shinfo(skb)->nr_frags)) {
				skb_frag_ptr = &skb_shinfo(skb)->frags[frag];

				/* Move to next segment */
				frag_size = skb_frag_ptr->size;
#if LINUX_VERSION_CODE > KERNEL_VERSION(3, 1, 10)
				frag_ptr = page_address(skb_frag_ptr->page.p) + skb_frag_ptr->page_offset;
#else
				frag_ptr = page_address(skb_frag_ptr->page) + skb_frag_ptr->page_offset;
#endif
				frag++;
			}
		}

#ifdef CONFIG_MV_ETH_PP2_1
		/* PPv2.1 - MAS 3.16, decrease number of reserved descriptors */
		txq_cpu_ptr->reserved_num -= seg_desc_num;
#endif

		/* TCP segment is ready - transmit it */
		wmb();
		mvPp2AggrTxqPendDescAdd(seg_desc_num);

		STAT_DBG(aggr_txq_ctrl->stats.txq_tx += seg_desc_num);
		STAT_DBG(txq_cpu_ptr->stats.txq_tx += seg_desc_num);
	}

	STAT_DBG(priv->stats.tx_tso_bytes += total_bytes);

	return total_desc_num;
}

#endif /* CONFIG_MV_ETH_TSO */

/* Push packets received by the RXQ to BM pool */
static void mv_eth_rxq_drop_pkts(struct eth_port *pp, int rxq)
{
	struct pp2_rx_desc   *rx_desc;
	int	                 rx_done, i;
	MV_PP2_PHYS_RXQ_CTRL *rx_ctrl = pp->rxq_ctrl[rxq].q;

	if (rx_ctrl == NULL)
		return;

	rx_done = mvPp2RxqBusyDescNumGet(pp->port, rxq);
	mvOsCacheIoSync(NULL);

	for (i = 0; i < rx_done; i++) {
		__u32 bm;
		int pool;
		struct bm_pool *ppool;

		rx_desc = mvPp2RxqNextDescGet(rx_ctrl);

#if defined(MV_CPU_BE)
		mvPPv2RxqDescSwap(rx_desc);
#endif /* MV_CPU_BE */

		bm = mv_eth_bm_cookie_build(rx_desc);
		pool = mv_eth_bm_cookie_pool_get(bm);
		ppool = &mv_eth_pool[pool];

		mv_eth_pool_refill(ppool, bm, rx_desc->bufPhysAddr, rx_desc->bufCookie);
		mvOsCacheLineInv(NULL, rx_desc);
	}
	if (rx_done) {
		mvOsCacheIoSync(NULL);
		mvPp2RxqDescNumUpdate(pp->port, rxq, rx_done, rx_done);
	}
}

static int mv_eth_txq_done_force(struct eth_port *pp, struct tx_queue *txq_ctrl)
{
	int cpu, tx_done = 0;
	struct txq_cpu_ctrl *txq_cpu_ptr;

	for_each_possible_cpu(cpu) {
		txq_cpu_ptr = &txq_ctrl->txq_cpu[cpu];
		tx_done = txq_cpu_ptr->txq_count;
		mv_eth_txq_bufs_free(pp, &txq_ctrl->txq_cpu[cpu], tx_done);
		STAT_DBG(txq_cpu_ptr->stats.txq_txdone += tx_done);

		/* reset txq */
		txq_cpu_ptr->txq_count = 0;
		txq_cpu_ptr->shadow_txq_put_i = 0;
		txq_cpu_ptr->shadow_txq_get_i = 0;
	}
	return tx_done;
}

inline u32 mv_eth_tx_done_pon(struct eth_port *pp, int *tx_todo)
{
	int txp, txq;
	struct tx_queue *txq_ctrl;
	struct txq_cpu_ctrl *txq_cpu_ptr;
	u32 tx_done = 0;

	*tx_todo = 0;

	STAT_INFO(pp->stats.tx_done++);

	/* simply go over all TX ports and TX queues */
	txp = pp->txp_num;
	while (txp--) {
		txq = CONFIG_MV_ETH_TXQ;

		while (txq--) {
			txq_ctrl = &pp->txq_ctrl[txp * CONFIG_MV_ETH_TXQ + txq];
			txq_cpu_ptr = &txq_ctrl->txq_cpu[smp_processor_id()];
			if ((txq_ctrl) && (txq_cpu_ptr->txq_count)) {
				tx_done += mv_eth_txq_done(pp, txq_ctrl);
				*tx_todo += txq_cpu_ptr->txq_count;
			}
		}
	}

	STAT_DIST((tx_done < pp->dist_stats.tx_done_dist_size) ? pp->dist_stats.tx_done_dist[tx_done]++ : 0);

	return tx_done;
}


inline u32 mv_eth_tx_done_gbe(struct eth_port *pp, u32 cause_tx_done, int *tx_todo)
{
	int txq;
	struct tx_queue *txq_ctrl;
	struct txq_cpu_ctrl *txq_cpu_ptr;
	u32 tx_done = 0;

	*tx_todo = 0;

	STAT_INFO(pp->stats.tx_done++);

	while (cause_tx_done != 0) {

		/* For GbE ports we get TX Buffers Threshold Cross per queue in bits [7:0] */
		txq = mv_eth_tx_done_policy(cause_tx_done);

		if (txq == -1)
			break;

		txq_ctrl = &pp->txq_ctrl[txq];
		txq_cpu_ptr = &txq_ctrl->txq_cpu[smp_processor_id()];

		if (txq_ctrl == NULL) {
			printk(KERN_ERR "%s: txq_ctrl = NULL, txq=%d\n", __func__, txq);
			return -EINVAL;
		}

		if ((txq_ctrl) && (txq_cpu_ptr->txq_count)) {
			tx_done += mv_eth_txq_done(pp, txq_ctrl);
			*tx_todo += txq_cpu_ptr->txq_count;
		}

		cause_tx_done &= ~(1 << txq);
	}

	STAT_DIST((tx_done < pp->dist_stats.tx_done_dist_size) ? pp->dist_stats.tx_done_dist[tx_done]++ : 0);

	return tx_done;
}


static void mv_eth_tx_frag_process(struct eth_port *pp, struct sk_buff *skb, struct aggr_tx_queue *aggr_txq_ctrl,
					struct tx_queue *txq_ctrl, struct mv_eth_tx_spec *tx_spec)
{
	int i, cpu = smp_processor_id();
	struct pp2_tx_desc *tx_desc;
	MV_U32 bufPhysAddr;

	for (i = 0; i < skb_shinfo(skb)->nr_frags; i++) {
		skb_frag_t *frag = &skb_shinfo(skb)->frags[i];

		tx_desc = mvPp2AggrTxqNextDescGet(aggr_txq_ctrl->q);
		tx_desc->physTxq = MV_PPV2_TXQ_PHYS(pp->port, tx_spec->txp, tx_spec->txq);

		/* NETA_TX_BM_ENABLE_MASK = 0 */
		/* NETA_TX_PKT_OFFSET_MASK = 0 */
		tx_desc->dataSize = frag->size;

#if LINUX_VERSION_CODE > KERNEL_VERSION(3, 1, 10)
		bufPhysAddr = mvOsCacheFlush(NULL, page_address(frag->page.p) + frag->page_offset,
						      tx_desc->dataSize);
#else
		bufPhysAddr = mvOsCacheFlush(NULL, page_address(frag->page) + frag->page_offset,
						      tx_desc->dataSize);
#endif

		tx_desc->pktOffset = bufPhysAddr & MV_ETH_TX_DESC_ALIGN;
		tx_desc->bufPhysAddr = bufPhysAddr & (~MV_ETH_TX_DESC_ALIGN);

		if (i == (skb_shinfo(skb)->nr_frags - 1)) {
			/* Last descriptor */
			if (tx_spec->flags & MV_ETH_TX_F_NO_PAD)
				tx_desc->command = (PP2_TX_L_DESC_MASK | PP2_TX_PADDING_DISABLE_MASK);
			else
				tx_desc->command = PP2_TX_L_DESC_MASK;

			mv_eth_shadow_push(&txq_ctrl->txq_cpu[cpu], ((MV_ULONG) skb | MV_ETH_SHADOW_SKB));
		} else {
			/* Descriptor in the middle: Not First, Not Last */
			tx_desc->command = 0;

			mv_eth_shadow_push(&txq_ctrl->txq_cpu[cpu], 0);
		}

		mv_eth_tx_desc_flush(tx_desc);
	}
}


/* Free "num" buffers from the pool */
static int mv_eth_pool_free(int pool, int num)
{
	int i = 0, buf_size, total_size;
	u32 pa;
	struct bm_pool *ppool = &mv_eth_pool[pool];
	bool free_all = false;

	if (num >= ppool->buf_num) {
		/* Free all buffers from the pool */
		free_all = true;
		num = ppool->buf_num;
	}

	if (MV_ETH_BM_POOL_IS_HWF(ppool->type)) {
		buf_size = RX_HWF_BUF_SIZE(ppool->pkt_size);
		total_size = RX_HWF_TOTAL_SIZE(buf_size);
	} else {
		buf_size = RX_BUF_SIZE(ppool->pkt_size);
		total_size = RX_TOTAL_SIZE(buf_size);
	}

	while (i < num) {
		MV_U32 *va;
		va = (MV_U32 *)mvBmPoolGet(pool, &pa);
		if (va == 0)
			break;

		/*pr_info("%4d: phys_addr=0x%x, virt_addr=%p\n", i, pa, va);*/

		if (!MV_ETH_BM_POOL_IS_HWF(ppool->type)) {
			mv_eth_skb_free((struct sk_buff *)va);
		} else { /* HWF pool */
			mvOsFree((char *)va);
		}
		i++;
	}
	pr_info("bm pool #%d: pkt_size=%4d, buf_size=%4d, total buf_size=%4d - %d of %d buffers free\n",
			pool, ppool->pkt_size, buf_size, total_size, i, num);

	ppool->buf_num -= num;

	/* Update BM driver with number of buffers removed from pool */
	mvBmPoolBufNumUpdate(pool, num, 0);

	return i;
}


static int mv_eth_pool_destroy(int pool)
{
	int num, status = 0;
	struct bm_pool *ppool = &mv_eth_pool[pool];

	num = mv_eth_pool_free(pool, ppool->buf_num);
	if (num != ppool->buf_num) {
		printk(KERN_ERR "Warning: could not free all buffers in pool %d while destroying pool\n", pool);
		return MV_ERROR;
	}

	mvBmPoolControl(pool, MV_STOP);

	/* Note: we don't free the bm_pool here ! */
	if (ppool->bm_pool)
		mvOsIoUncachedFree(NULL, sizeof(MV_U32) * ppool->capacity, ppool->physAddr, ppool->bm_pool, 0);

	memset(ppool, 0, sizeof(struct bm_pool));

	return status;
}


static int mv_eth_pool_add(int pool, int buf_num)
{
	struct bm_pool *bm_pool;
	struct sk_buff *skb;
	unsigned char *hwf_buff;
	int i, buf_size, total_size;
	__u32 bm = 0;
	phys_addr_t phys_addr;

	if (mvPp2MaxCheck(pool, MV_ETH_BM_POOLS, "bm_pool"))
		return 0;

	bm_pool = &mv_eth_pool[pool];

	if (MV_ETH_BM_POOL_IS_HWF(bm_pool->type)) {
		buf_size = RX_HWF_BUF_SIZE(bm_pool->pkt_size);
		total_size = RX_HWF_TOTAL_SIZE(buf_size);
	} else {
		buf_size = RX_BUF_SIZE(bm_pool->pkt_size);
		total_size = RX_TOTAL_SIZE(buf_size);
	}

	/* Check buffer size */
	if (bm_pool->pkt_size == 0) {
		printk(KERN_ERR "%s: invalid pool #%d state: pkt_size=%d, buf_size=%d, buf_num=%d\n",
		       __func__, pool, bm_pool->pkt_size, RX_BUF_SIZE(bm_pool->pkt_size), bm_pool->buf_num);
		return 0;
	}

	/* Insure buf_num is smaller than capacity */
	if ((buf_num < 0) || ((buf_num + bm_pool->buf_num) > (bm_pool->capacity))) {
		printk(KERN_ERR "%s: can't add %d buffers into bm_pool=%d: capacity=%d, buf_num=%d\n",
		       __func__, buf_num, pool, bm_pool->capacity, bm_pool->buf_num);
		return 0;
	}

	bm = mv_eth_bm_cookie_pool_set(bm, pool);
	for (i = 0; i < buf_num; i++) {
		if (!MV_ETH_BM_POOL_IS_HWF(bm_pool->type)) {
			/* Allocate skb for pool used for SWF */
			skb = mv_eth_skb_alloc(bm_pool, &phys_addr, GFP_KERNEL);
			if (!skb)
				break;

			mv_eth_pool_refill(bm_pool, bm, phys_addr, (unsigned long) skb);
		} else {
			/* Allocate pkt + buffer for pool used for HWF */
			hwf_buff = mv_eth_hwf_buff_alloc(bm_pool, &phys_addr);
			if (!hwf_buff)
				break;

			memset(hwf_buff, 0, buf_size);
			mv_eth_pool_refill(bm_pool, bm, phys_addr, (MV_ULONG) hwf_buff);
		}
	}

	bm_pool->buf_num += i;
	bm_pool->in_use_thresh = bm_pool->buf_num / 4;

	/* Update BM driver with number of buffers added to pool */
	mvBmPoolBufNumUpdate(pool, i, 1);

	pr_info("%s %s pool #%d: pkt_size=%4d, buf_size=%4d, total_size=%4d - %d of %d buffers added\n",
		MV_ETH_BM_POOL_IS_HWF(bm_pool->type) ? "HWF" : "SWF",
		MV_ETH_BM_POOL_IS_SHORT(bm_pool->type) ? "short" : " long",
		pool, bm_pool->pkt_size, buf_size, total_size, i, buf_num);

	return i;
}

void	*mv_eth_bm_pool_create(int pool, int capacity, MV_ULONG *pPhysAddr)
{
	MV_ULONG physAddr;
	void *pVirt;
	MV_STATUS status;
	int size = 2 * sizeof(MV_U32) * capacity;

	pVirt = mvOsIoUncachedMalloc(NULL, size, &physAddr, NULL);
	if (pVirt == NULL) {
		mvOsPrintf("%s: Can't allocate %d bytes for pool #%d\n",
				__func__, size, pool);
		return NULL;
	}

	/* Pool address must be MV_BM_POOL_PTR_ALIGN bytes aligned */
	if (MV_IS_NOT_ALIGN((unsigned)pVirt, MV_BM_POOL_PTR_ALIGN)) {
		mvOsPrintf("memory allocated for BM pool #%d is not %d bytes aligned\n",
					pool, MV_BM_POOL_PTR_ALIGN);
		mvOsIoCachedFree(NULL, size, physAddr, pVirt, 0);
		return NULL;
	}
	status = mvBmPoolInit(pool, pVirt, physAddr, capacity);
	if (status != MV_OK) {
		mvOsPrintf("%s: Can't init #%d BM pool. status=%d\n", __func__, pool, status);
		mvOsIoCachedFree(NULL, size, physAddr, pVirt, 0);
		return NULL;
	}

	mvBmPoolControl(pool, MV_START);

	if (pPhysAddr != NULL)
		*pPhysAddr = physAddr;

	return pVirt;
}

static MV_STATUS mv_eth_pool_create(int pool, int capacity)
{
	struct bm_pool *bm_pool;
	MV_ULONG    physAddr;

	if ((pool < 0) || (pool >= MV_ETH_BM_POOLS)) {
		printk(KERN_ERR "%s: pool=%d is out of range\n", __func__, pool);
		return MV_BAD_VALUE;
	}

	bm_pool = &mv_eth_pool[pool];
	memset(bm_pool, 0, sizeof(struct bm_pool));

	bm_pool->bm_pool = mv_eth_bm_pool_create(pool, capacity, &physAddr);
	if (bm_pool->bm_pool == NULL)
		return MV_FAIL;

	bm_pool->pool = pool;
	bm_pool->type = MV_ETH_BM_FREE;
	bm_pool->capacity = capacity;
	bm_pool->pkt_size = 0;
	bm_pool->buf_num = 0;
	atomic_set(&bm_pool->in_use, 0);

	spin_lock_init(&bm_pool->lock);

	return MV_OK;
}

/* mv_eth_pool_use:							*
 *	- notify the driver that BM pool is being used as specific type	*
 *	- Allocate / Free buffers if necessary				*
 *	- Returns the used pool pointer in case of success		*
 *	- Parameters:							*
 *		- pool: BM pool that is being used			*
 *		- type: type of usage (SWF/HWF/MIXED long/short)	*
 *		- pkt_size: number of bytes per packet			*/
static struct bm_pool *mv_eth_pool_use(int pool, enum mv_eth_bm_type type, int pkt_size)
{
	unsigned long flags = 0;
	struct bm_pool *new_pool;
	int num;

	new_pool = &mv_eth_pool[pool];

	if ((MV_ETH_BM_POOL_IS_SHORT(new_pool->type) && MV_ETH_BM_POOL_IS_LONG(type))
		|| (MV_ETH_BM_POOL_IS_SHORT(type) && MV_ETH_BM_POOL_IS_LONG(new_pool->type))) {
		pr_err("%s FAILED: BM pool can't be used as short and long at the same time\n", __func__);
		return NULL;
	}

	MV_ETH_LOCK(&new_pool->lock, flags);

	if (new_pool->type == MV_ETH_BM_FREE)
		new_pool->type = type;
	else if (MV_ETH_BM_POOL_IS_SWF(new_pool->type) && MV_ETH_BM_POOL_IS_HWF(type))
		new_pool->type = MV_ETH_BM_POOL_IS_LONG(type) ? MV_ETH_BM_MIXED_LONG : MV_ETH_BM_MIXED_SHORT;

	/* Check if buffer allocation is needed, there are 3 cases:			*
	 *	1. BM pool was used only by HWF, and will be used by SWF as well	*
	 *	2. BM pool is used as long pool, but packet size doesn't match MTU	*
	 *	3. BM pool hasn't being used yet					*/
	if ((MV_ETH_BM_POOL_IS_HWF(new_pool->type) && MV_ETH_BM_POOL_IS_SWF(type))
		|| (MV_ETH_BM_POOL_IS_LONG(type) && (pkt_size > new_pool->pkt_size))
		|| (new_pool->pkt_size == 0)) {
		int port, pkts_num;

		/* If there are ports using this pool, they must be stopped before allocation */
		port = mv_eth_port_up_get(new_pool->port_map);
		if (port != -1) {
			pr_err("%s: port %d use pool %d and must be stopped before buffer re-allocation\n",
				__func__, port, new_pool->pool);
			MV_ETH_UNLOCK(&new_pool->lock, flags);
			return NULL;
		}

		/* if pool is empty, then set default buffers number		*
		 * if pool is not empty, then we must free all the buffers	*/
		pkts_num = new_pool->buf_num;
		if (pkts_num == 0)
			pkts_num = (MV_ETH_BM_POOL_IS_LONG(type)) ?
				CONFIG_MV_ETH_BM_LONG_BUF_NUM : CONFIG_MV_ETH_BM_SHORT_BUF_NUM;
		else
			mv_eth_pool_free(new_pool->pool, pkts_num);

		/* Check if pool has moved to SWF and HWF shared mode */
		if ((MV_ETH_BM_POOL_IS_HWF(new_pool->type) && !MV_ETH_BM_POOL_IS_HWF(type))
			|| (MV_ETH_BM_POOL_IS_HWF(type) && !MV_ETH_BM_POOL_IS_HWF(new_pool->type)))
			new_pool->type = MV_ETH_BM_POOL_IS_LONG(type) ? MV_ETH_BM_MIXED_LONG : MV_ETH_BM_MIXED_SHORT;

		/* Update packet size (in case of MTU larger than current ot new pool) */
		if ((new_pool->pkt_size == 0)
			|| (MV_ETH_BM_POOL_IS_LONG(type) && (pkt_size > new_pool->pkt_size)))
			new_pool->pkt_size = pkt_size;

		/* Allocate buffers for this pool */
		num = mv_eth_pool_add(new_pool->pool, pkts_num);
		if (num != pkts_num) {
			pr_err("%s FAILED: pool=%d, pkt_size=%d, only %d of %d allocated\n",
				__func__, new_pool->pool, new_pool->pkt_size, num, pkts_num);
			MV_ETH_UNLOCK(&new_pool->lock, flags);
			return NULL;
		}

	}

	if (MV_ETH_BM_POOL_IS_HWF(new_pool->type))
		mvPp2BmPoolBufSizeSet(new_pool->pool, RX_HWF_BUF_SIZE(new_pool->pkt_size));
	else
		mvPp2BmPoolBufSizeSet(new_pool->pool, RX_BUF_SIZE(new_pool->pkt_size));

	MV_ETH_UNLOCK(&new_pool->lock, flags);

	return new_pool;
}

/* Interrupt handling */
irqreturn_t mv_eth_isr(int irq, void *dev_id)
{
	struct eth_port *pp = (struct eth_port *)dev_id;
	int cpu = smp_processor_id();
	struct napi_group_ctrl *napi_group = pp->cpu_config[cpu]->napi_group;
	struct napi_struct *napi = napi_group->napi;

#ifdef CONFIG_MV_ETH_DEBUG_CODE
	if (pp->dbg_flags & MV_ETH_F_DBG_ISR) {
		pr_info("%s: port=%d, cpu=%d, mask=0x%x, cause=0x%x\n",
			__func__, pp->port, cpu,
			mvPp2RdReg(MV_PP2_ISR_RX_TX_MASK_REG(MV_PPV2_PORT_PHYS(pp->port))),
			mvPp2GbeIsrCauseRxTxGet(pp->port));
	}
#endif /* CONFIG_MV_ETH_DEBUG_CODE */

	STAT_INFO(pp->stats.irq[cpu]++);

	/* Mask all interrupts for cpus in this group */
	mvPp2GbeCpuInterruptsDisable(pp->port, napi_group->cpu_mask);

	/* Verify that the device not already on the polling list */
	if (napi_schedule_prep(napi)) {
		/* schedule the work (rx+txdone+link) out of interrupt contxet */
		__napi_schedule(napi);
	} else {
		STAT_INFO(pp->stats.irq_err[cpu]++);
#ifdef CONFIG_MV_ETH_DEBUG_CODE
		pr_warning("%s: IRQ=%d, port=%d, cpu=%d, cpu_mask=0x%x - NAPI already scheduled\n",
			__func__, irq, pp->port, cpu, napi_group->cpu_mask);
#endif /* CONFIG_MV_ETH_DEBUG_CODE */
	}

	return IRQ_HANDLED;
}

irqreturn_t mv_eth_link_isr(int irq, void *dev_id)
{
	mvGmacIsrSummaryMask();

	tasklet_schedule(&link_tasklet);

	return IRQ_HANDLED;
}

void mv_eth_link_tasklet(unsigned long data)
{
	int port;
	MV_U32 regVal, regVal1;
	struct eth_port *pp;

	regVal = mvGmacIsrSummaryCauseGet();

	/* check only relevant interrupts - ports0 and 1 */
	regVal &= (ETH_ISR_SUM_PORT0_MASK | ETH_ISR_SUM_PORT1_MASK);

	for (port = 0; port < mv_eth_ports_num; port++) {
		/* check if interrupt was caused by this port */
		if (!(ETH_ISR_SUM_PORT_MASK(port) & regVal))
			continue;

		regVal1 = mvGmacPortIsrCauseGet(port);

		/* check for link change interrupt */
		if (!(regVal1 & ETH_PORT_LINK_CHANGE_MASK)) {
			mvGmacPortIsrUnmask(port);
			continue;
		}

		pp = mv_eth_port_by_id(port);
		if (pp)
			mv_eth_link_event(pp, 1);
	}

	mvGmacIsrSummaryUnmask();
}

static bool mv_eth_link_status(struct eth_port *pp)
{
#ifdef CONFIG_MV_INCLUDE_PON
	if (MV_PON_PORT(pp->port))
		return mv_pon_link_status(NULL);
	else
#endif /* CONFIG_MV_PON */
		return mvGmacPortIsLinkUp(pp->port);
}

void mv_eth_link_event(struct eth_port *pp, int print)
{
	struct net_device *dev = pp->dev;
	bool              link_is_up = false;

	STAT_INFO(pp->stats.link++);

	/* Check Link status on ethernet port */
	link_is_up = mv_eth_link_status(pp);

	if (link_is_up) {
		/* Link Up event */
		mvPp2PortEgressEnable(pp->port, MV_TRUE);
		set_bit(MV_ETH_F_LINK_UP_BIT, &(pp->flags));

		if (mv_eth_ctrl_is_tx_enabled(pp)) {
			if (dev) {
				netif_carrier_on(dev);
				netif_tx_wake_all_queues(dev);
			}
		}
		mvPp2PortIngressEnable(pp->port, MV_TRUE);
	} else {
		/* Link Down event */
		mvPp2PortIngressEnable(pp->port, MV_FALSE);
		if (dev) {
			netif_carrier_off(dev);
			netif_tx_stop_all_queues(dev);
		}
		mvPp2PortEgressEnable(pp->port, MV_FALSE);
		clear_bit(MV_ETH_F_LINK_UP_BIT, &(pp->flags));
	}

	if (print) {
		if (dev)
			printk(KERN_ERR "%s: ", dev->name);
		else
			printk(KERN_ERR "%s: ", "none");

		mv_eth_link_status_print(pp->port);
	}
}

/***********************************************************************************************/
int mv_eth_poll(struct napi_struct *napi, int budget)
{
	int rx_done = 0;
	struct napi_group_ctrl *napi_group;
	MV_U32 causeRxTx;
	struct eth_port *pp = MV_ETH_PRIV(napi->dev);

#ifdef CONFIG_MV_ETH_DEBUG_CODE
	if (pp->dbg_flags & MV_ETH_F_DBG_POLL) {
		printk(KERN_ERR "%s ENTER: port=%d, cpu=%d, mask=0x%x, cause=0x%x\n",
			__func__, pp->port, smp_processor_id(),
			mvPp2RdReg(MV_PP2_ISR_RX_TX_MASK_REG(MV_PPV2_PORT_PHYS(pp->port))),
			mvPp2GbeIsrCauseRxTxGet(pp->port));
	}
#endif /* CONFIG_MV_ETH_DEBUG_CODE */

	if (!test_bit(MV_ETH_F_STARTED_BIT, &(pp->flags))) {
		STAT_INFO(pp->stats.netdev_stop++);

#ifdef CONFIG_MV_ETH_DEBUG_CODE
		if (pp->dbg_flags & MV_ETH_F_DBG_RX)
			printk(KERN_ERR "%s: STARTED_BIT = 0, poll completed.\n", __func__);
#endif /* CONFIG_MV_ETH_DEBUG_CODE */

		napi_complete(napi);
		STAT_INFO(pp->stats.poll_exit[smp_processor_id()]++);
		return rx_done;
	}

	STAT_INFO(pp->stats.poll[smp_processor_id()]++);

	/* Read cause register */
	causeRxTx = mvPp2GbeIsrCauseRxTxGet(pp->port);

	if (causeRxTx & MV_PP2_CAUSE_MISC_SUM_MASK) {
		if (causeRxTx & MV_PP2_CAUSE_FCS_ERR_MASK)
			printk(KERN_ERR "%s: FCS error\n", __func__);

		if (causeRxTx & MV_PP2_CAUSE_RX_FIFO_OVERRUN_MASK)
			printk(KERN_ERR "%s: RX fifo overrun error\n", __func__);

		if (causeRxTx & MV_PP2_CAUSE_TX_FIFO_UNDERRUN_MASK)
			printk(KERN_ERR "%s: TX fifo underrun error\n", __func__);

		if (causeRxTx & MV_PP2_CAUSE_MISC_SUM_MASK) {
			printk(KERN_ERR "%s: misc event\n", __func__);
			mvPp2WrReg(MV_PP2_ISR_MISC_CAUSE_REG, 0);
		}

		causeRxTx &= ~MV_PP2_CAUSE_MISC_SUM_MASK;
		mvPp2WrReg(MV_PP2_ISR_RX_TX_CAUSE_REG(MV_PPV2_PORT_PHYS(pp->port)), causeRxTx);
	}
	napi_group = pp->cpu_config[smp_processor_id()]->napi_group;
	causeRxTx |= napi_group->cause_rx_tx;

#ifdef CONFIG_MV_ETH_TXDONE_ISR
	if (mvPp2GbeIsrCauseTxDoneIsSet(pp->port, causeRxTx)) {
		int tx_todo = 0, cause_tx_done;
		/* TX_DONE process */
		cause_tx_done = mvPp2GbeIsrCauseTxDoneOffset(pp->port, causeRxTx);
		if (MV_PON_PORT(pp->port))
			mv_eth_tx_done_pon(pp, &tx_todo);
		else
			mv_eth_tx_done_gbe(pp, cause_tx_done, &tx_todo);
	}
#endif /* CONFIG_MV_ETH_TXDONE_ISR */
	if (MV_PON_PORT(pp->port))
		causeRxTx &= ~MV_PP2_PON_CAUSE_TXP_OCCUP_DESC_ALL_MASK;
	else
		causeRxTx &= ~MV_PP2_CAUSE_TXQ_OCCUP_DESC_ALL_MASK;

	while ((causeRxTx != 0) && (budget > 0)) {
		int count, rx_queue;

		rx_queue = mv_eth_rx_policy(causeRxTx);
		if (rx_queue == -1)
			break;

		count = mv_eth_rx(pp, budget, rx_queue, napi);
		rx_done += count;
		budget -= count;
		if (budget > 0)
			causeRxTx &= ~((1 << rx_queue) << MV_PP2_CAUSE_RXQ_OCCUP_DESC_OFFS);
	}

	/* Maintain RX packets rate if adaptive RX coalescing is enabled */
	if (pp->rx_adaptive_coal_cfg)
		pp->rx_rate_pkts += rx_done;

	STAT_DIST((rx_done < pp->dist_stats.rx_dist_size) ? pp->dist_stats.rx_dist[rx_done]++ : 0);

#ifdef CONFIG_MV_ETH_DEBUG_CODE
	if (pp->dbg_flags & MV_ETH_F_DBG_POLL) {
		printk(KERN_ERR "%s  EXIT: port=%d, cpu=%d, budget=%d, rx_done=%d\n",
			__func__, pp->port, smp_processor_id(), budget, rx_done);
	}
#endif /* CONFIG_MV_ETH_DEBUG_CODE */

	if (budget > 0) {
		causeRxTx = 0;

		napi_complete(napi);

		STAT_INFO(pp->stats.poll_exit[smp_processor_id()]++);

		/* adapt RX coalescing according to packets rate */
		if (pp->rx_adaptive_coal_cfg)
			mv_eth_adaptive_rx_update(pp);

		/* Enable interrupts for all cpus belong to this group */
		if (!(pp->flags & MV_ETH_F_IFCAP_NETMAP)) {
			wmb();
			mvPp2GbeCpuInterruptsEnable(pp->port, napi_group->cpu_mask);
		}
	}
	napi_group->cause_rx_tx = causeRxTx;

	return rx_done;
}

void mv_eth_port_filtering_cleanup(int port)
{
	static bool is_first = true;

	/* clean TCAM and SRAM only one, no need to do this per port. */
	if (is_first) {
		mvPp2PrsHwClearAll();
		mvPp2PrsHwInvAll();
		is_first = false;
	}
}


static MV_STATUS mv_eth_bm_pools_init(void)
{
	int i, j;
	MV_STATUS status;

	/* Create all pools with maximum capacity */
	for (i = 0; i < MV_ETH_BM_POOLS; i++) {
		status = mv_eth_pool_create(i, MV_BM_POOL_CAP_MAX);
		if (status != MV_OK) {
			printk(KERN_ERR "%s: can't create bm_pool=%d - capacity=%d\n", __func__, i, MV_BM_POOL_CAP_MAX);
			for (j = 0; j < i; j++)
				mv_eth_pool_destroy(j);
			return status;
		}

		mv_eth_pool[i].pkt_size = 0;
		mv_eth_pool[i].type = MV_ETH_BM_FREE;

		mvPp2BmPoolBufSizeSet(i, 0);
	}
	return 0;
}

int mv_eth_swf_bm_pool_init(struct eth_port *pp, int mtu)
{
	unsigned long flags = 0;
	int rxq, pkt_size = RX_PKT_SIZE(mtu);

	if (pp->pool_long == NULL) {
		pp->pool_long = mv_eth_pool_use(MV_ETH_BM_SWF_LONG_POOL(pp->port),
							MV_ETH_BM_SWF_LONG, pkt_size);
		if (pp->pool_long == NULL)
			return -1;

		MV_ETH_LOCK(&pp->pool_long->lock, flags);
		pp->pool_long->port_map |= (1 << pp->port);
		MV_ETH_UNLOCK(&pp->pool_long->lock, flags);

		for (rxq = 0; rxq < pp->rxq_num; rxq++)
			mvPp2RxqBmLongPoolSet(pp->port, rxq, pp->pool_long->pool);
	}

	if (pp->pool_short == NULL) {
		pp->pool_short = mv_eth_pool_use(MV_ETH_BM_SWF_SHORT_POOL(pp->port),
							MV_ETH_BM_SWF_SHORT, MV_ETH_BM_SHORT_PKT_SIZE);
		if (pp->pool_short == NULL)
			return -1;

		MV_ETH_LOCK(&pp->pool_short->lock, flags);
		pp->pool_short->port_map |= (1 << pp->port);
		MV_ETH_UNLOCK(&pp->pool_short->lock, flags);

		for (rxq = 0; rxq < pp->rxq_num; rxq++)
			mvPp2RxqBmShortPoolSet(pp->port, rxq, pp->pool_short->pool);
	}

	return 0;
}

#ifdef CONFIG_MV_ETH_HWF
int mv_eth_hwf_bm_pool_init(struct eth_port *pp, int mtu)
{
	unsigned long flags = 0;
	int pkt_size = RX_PKT_SIZE(mtu);

	if (pp->hwf_pool_long == NULL) {
		pp->hwf_pool_long = mv_eth_pool_use(MV_ETH_BM_HWF_LONG_POOL(pp->port),
							MV_ETH_BM_HWF_LONG, pkt_size);
		if (pp->hwf_pool_long == NULL)
			return -1;

		MV_ETH_LOCK(&pp->hwf_pool_long->lock, flags);
		pp->hwf_pool_long->port_map |= (1 << pp->port);
		MV_ETH_UNLOCK(&pp->hwf_pool_long->lock, flags);

#ifdef CONFIG_MV_ETH_PP2_1
		mv_eth_hwf_long_pool_attach(pp->port, pp->hwf_pool_long->pool);
#endif
	}

	if (pp->hwf_pool_short == NULL) {
		pp->hwf_pool_short = mv_eth_pool_use(MV_ETH_BM_HWF_SHORT_POOL(pp->port),
							MV_ETH_BM_HWF_SHORT, MV_ETH_BM_SHORT_HWF_PKT_SIZE);
		if (pp->hwf_pool_short == NULL)
			return -1;

		MV_ETH_LOCK(&pp->hwf_pool_short->lock, flags);
		pp->hwf_pool_short->port_map |= (1 << pp->port);
		MV_ETH_UNLOCK(&pp->hwf_pool_short->lock, flags);

#ifdef CONFIG_MV_ETH_PP2_1
		mv_eth_hwf_short_pool_attach(pp->port, pp->hwf_pool_short->pool);
#endif
	}

#ifndef CONFIG_MV_ETH_PP2_1
	mvPp2PortHwfBmPoolSet(pp->port, pp->hwf_pool_short->pool, pp->hwf_pool_long->pool);
#endif

	return 0;
}
#endif /* CONFIG_MV_ETH_HWF */

static int mv_eth_port_link_speed_fc(int port, MV_ETH_PORT_SPEED port_speed, int en_force)
{
	if (en_force) {
		if (mvGmacSpeedDuplexSet(port, port_speed, MV_ETH_DUPLEX_FULL)) {
			printk(KERN_ERR "SpeedDuplexSet failed\n");
			return -EIO;
		}
		if (mvGmacFlowCtrlSet(port, MV_ETH_FC_ENABLE)) {
			printk(KERN_ERR "FlowCtrlSet failed\n");
			return -EIO;
		}
		if (mvGmacForceLinkModeSet(port, 1, 0)) {
			printk(KERN_ERR "ForceLinkModeSet failed\n");
			return -EIO;
		}
	} else {
		if (mvGmacForceLinkModeSet(port, 0, 0)) {
			printk(KERN_ERR "ForceLinkModeSet failed\n");
			return -EIO;
		}
		if (mvGmacSpeedDuplexSet(port, MV_ETH_SPEED_AN, MV_ETH_DUPLEX_AN)) {
			printk(KERN_ERR "SpeedDuplexSet failed\n");
			return -EIO;
		}
		if (mvGmacFlowCtrlSet(port, MV_ETH_FC_AN_SYM)) {
			printk(KERN_ERR "FlowCtrlSet failed\n");
			return -EIO;
		}
	}
	return 0;
}

static int mv_eth_load_network_interfaces(struct platform_device *pdev)
{
	u32 port;
	struct eth_port *pp;
	struct net_device *dev;
	int mtu, err, phys_port, speed, force_link = 0;
	struct mv_pp2_pdata *plat_data = (struct mv_pp2_pdata *)pdev->dev.platform_data;
	u8 mac[MV_MAC_ADDR_SIZE];

	port = pdev->id;
	phys_port = MV_PPV2_PORT_PHYS(port);
	pr_info("  o Loading network interface(s) for port #%d: cpu_mask=0x%x, mtu=%d\n",
			port, plat_data->cpu_mask, plat_data->mtu);

	mtu = mv_eth_config_get(pdev, mac);

	dev = mv_eth_netdev_init(mtu, mac, pdev);

	if (dev == NULL) {
		pr_err("\to %s: can't create netdevice\n", __func__);
		return -EIO;
	}

	pp = (struct eth_port *)netdev_priv(dev);
	pp->plat_data = plat_data;

	mv_eth_ports[port] = pp;

	err = mv_eth_priv_init(pp, port);
	if (err) {
		mv_eth_priv_cleanup(pp);
		return err;
	}

	if (plat_data->flags & MV_PP2_PDATA_F_LINUX_CONNECT) {
		pr_info("\to Port %d is connected to Linux netdevice\n", port);
		set_bit(MV_ETH_F_CONNECT_LINUX_BIT, &(pp->flags));
	} else {
		pr_info("\to Port %d is disconnected from Linux netdevice\n", pp->port);
		clear_bit(MV_ETH_F_CONNECT_LINUX_BIT, &(pp->flags));
	}


	pp->cpuMask = plat_data->cpu_mask;

	switch (plat_data->speed) {
	case SPEED_10:
		speed = MV_ETH_SPEED_10;
		force_link = 1;
		break;
	case SPEED_100:
		speed = MV_ETH_SPEED_100;
		force_link = 1;
		break;
	case SPEED_1000:
		speed = MV_ETH_SPEED_1000;
		force_link = 1;
		break;
	case 0:
		speed = MV_ETH_SPEED_AN;
		force_link = 0;
		break;
	default:
		pr_err("\to gbe #%d: unknown speed = %d\n", pp->port, plat_data->speed);
		return -EIO;
	}

	/* set port's speed, duplex, fc */
	if (!MV_PON_PORT(pp->port)) {
		/* force link, speed and duplex if necessary based on board information */
		err = mv_eth_port_link_speed_fc(pp->port, speed, force_link);
		if (err) {
			mv_eth_priv_cleanup(pp);
			return err;
		}
	}

	pr_info("\to %s p=%d: phy=%d,  mtu=%d, mac="MV_MACQUAD_FMT", speed=%s %s\n",
		MV_PON_PORT(port) ? "pon" : "giga", port, plat_data->phy_addr, mtu,
		MV_MACQUAD(mac), mvGmacSpeedStrGet(speed), force_link ? "(force)" : "");

	if (mv_eth_hal_init(pp)) {
		pr_err("\to %s: can't init eth hal\n", __func__);
		mv_eth_priv_cleanup(pp);
		return -EIO;
	}

	if (mv_eth_netdev_connect(pp) < 0) {
		pr_err("\to %s: can't connect to linux\n", __func__);
		mv_eth_priv_cleanup(pp);
		return -EIO;
	}

	/* Default NAPI initialization */
	/* Create one group for this port, that contains all RXQs and all CPUs - every cpu can process all RXQs */
	if (pp->flags & MV_ETH_F_CONNECT_LINUX) {
		if (mv_eth_port_napi_group_create(pp->port, 0))
			return -EIO;
		if (mv_eth_napi_set_cpu_affinity(pp->port, 0, (1 << CONFIG_NR_CPUS) - 1) ||
				mv_eth_napi_set_rxq_affinity(pp->port, 0, (1 << MV_ETH_MAX_RXQ) - 1))
			return -EIO;
	}

	if (mv_eth_pnc_ctrl_en) {
#ifndef CONFIG_MV_ETH_PP2_1
		mv_eth_tx_mtu_set(port, mtu);
#endif /* CONFIG_MV_ETH_PP2_1 */

#ifndef CONFIG_MV_ETH_PP2_1
		mvPp2ClsHwOversizeRxqSet(MV_PPV2_PORT_PHYS(pp->port), pp->first_rxq);
#else
		mvPp2ClsHwOversizeRxqLowSet(MV_PPV2_PORT_PHYS(pp->port),
			(pp->first_rxq) & MV_PP2_CLS_OVERSIZE_RXQ_LOW_MASK);
		mvPp2ClsHwRxQueueHighSet(MV_PPV2_PORT_PHYS(pp->port),
			1,
			(pp->first_rxq) >> MV_PP2_CLS_OVERSIZE_RXQ_LOW_BITS);
#endif

		/* classifier port default config */
		mvPp2ClsHwPortDefConfig(phys_port, 0, FLOWID_DEF(phys_port), pp->first_rxq);
	}

#ifdef CONFIG_NETMAP
	mv_pp2_netmap_attach(pp);
#endif /* CONFIG_NETMAP */

	/* Call mv_eth_open specifically for ports not connected to Linux netdevice */
	if (!(pp->flags & MV_ETH_F_CONNECT_LINUX))
		mv_eth_open(pp->dev);

	mux_eth_ops.set_tag_type = mv_eth_tag_type_set;
	mv_mux_eth_attach(pp->port, pp->dev, &mux_eth_ops);

	pr_info("\n");

	return 0;
}



int mv_eth_resume_network_interfaces(struct eth_port *pp)
{
/* TBD */
	return 0;
}

/***********************************************************
 * mv_eth_port_resume                                      *
 ***********************************************************/

int mv_eth_port_resume(int port)
{
/* TBD */
	return 0;
}

void    mv_eth_hal_shared_init(struct mv_pp2_pdata *plat_data)
{
	MV_PP2_HAL_DATA halData;

	memset(&halData, 0, sizeof(halData));

	halData.maxPort = plat_data->max_port;
	halData.tClk = plat_data->tclk;
	halData.maxCPUs = nr_cpu_ids;
	halData.iocc = arch_is_coherent();
	halData.ctrlModel = plat_data->ctrl_model;
	halData.ctrlRev = plat_data->ctrl_rev;
	halData.aggrTxqSize = CONFIG_MV_ETH_AGGR_TXQ_SIZE;

#ifdef CONFIG_MV_INCLUDE_PON
	halData.maxTcont = CONFIG_MV_PON_TCONTS;
#endif

	mvPp2HalInit(&halData);

	return;
}


/***********************************************************
 * mv_eth_win_init --                                      *
 *   Win initilization                                     *
 ***********************************************************/
void mv_eth_win_init(void)
{
	const struct mbus_dram_target_info *dram;
	int i;
	u32 enable;

	/* First disable all address decode windows */
	enable = 0;
	mvPp2WrReg(ETH_BASE_ADDR_ENABLE_REG, enable);

	dram = mv_mbus_dram_info();
	if (!dram) {
		pr_err("%s: No DRAM information\n", __func__);
		return;
	}
	for (i = 0; i < dram->num_cs; i++) {
		const struct mbus_dram_window *cs = dram->cs + i;
		u32 baseReg, base = cs->base;
		u32 sizeReg, size = cs->size;
		u32 alignment;
		u8 attr = cs->mbus_attr;
		u8 target = dram->mbus_dram_target_id;

		/* check if address is aligned to the size */
		if (MV_IS_NOT_ALIGN(base, size)) {
			pr_err("%s: Error setting window for cs #%d.\n"
			   "Address 0x%08x is not aligned to size 0x%x.\n",
			   __func__, i, base, size);
			return;
		}

		if (!MV_IS_POWER_OF_2(size)) {
			pr_err("%s: Error setting window for cs #%d.\n"
				"Window size %u is not a power to 2.\n",
				__func__, i, size);
			return;
		}

#ifdef CONFIG_MV_SUPPORT_L2_DEPOSIT
		/* Setting DRAM windows attribute to :
			0x3 - Shared transaction + L2 write allocate (L2 Deposit) */
		attr &= ~(0x30);
		attr |= 0x30;
#endif

		baseReg = (base & ETH_WIN_BASE_MASK);
		sizeReg = mvPp2RdReg(ETH_WIN_SIZE_REG(i));

		/* set size */
		alignment = 1 << ETH_WIN_SIZE_OFFS;
		sizeReg &= ~ETH_WIN_SIZE_MASK;
		sizeReg |= (((size / alignment) - 1) << ETH_WIN_SIZE_OFFS);

		/* set attributes */
		baseReg &= ~ETH_WIN_ATTR_MASK;
		baseReg |= attr << ETH_WIN_ATTR_OFFS;

		/* set target ID */
		baseReg &= ~ETH_WIN_TARGET_MASK;
		baseReg |= target << ETH_WIN_TARGET_OFFS;

		mvPp2WrReg(ETH_WIN_BASE_REG(i), baseReg);
		mvPp2WrReg(ETH_WIN_SIZE_REG(i), sizeReg);

		enable |= (1 << i);
	}
	/* Enable window */
	mvPp2WrReg(ETH_BASE_ADDR_ENABLE_REG, enable);
}


/***********************************************************
 * mv_eth_port_suspend                                     *
 *   main driver initialization. loading the interfaces.   *
 ***********************************************************/
int mv_eth_port_suspend(int port)
{
/* TBD */
	return 0;
}

/***********************************************************
 * mv_eth_pm_mode_set --                                   *
 *   set pm_mode. (power menegment mod)			   *
 ***********************************************************/
int	mv_eth_pm_mode_set(int port, int mode)
{
/* TBD */
	return 0;
}

static void mv_eth_sysfs_exit(void)
{
	struct device *pd;

	pd = bus_find_device_by_name(&platform_bus_type, NULL, "pp2");
	if (!pd) {
		printk(KERN_ERR"%s: cannot find pp2 device\n", __func__);
		return;
	}
#ifdef CONFIG_MV_ETH_L2FW
	mv_pp2_l2fw_sysfs_exit(&pd->kobj);
#endif

#ifdef CONFIG_MV_ETH_PP2_1
	mv_pp2_dpi_sysfs_exit(&pd->kobj);
#endif

	mv_pp2_wol_sysfs_exit(&pd->kobj);
	mv_pp2_pme_sysfs_exit(&pd->kobj);
	mv_pp2_plcr_sysfs_exit(&pd->kobj);
	mv_pp2_mc_sysfs_exit(&pd->kobj);
	mv_pp2_cls4_sysfs_exit(&pd->kobj);
	mv_pp2_cls3_sysfs_exit(&pd->kobj);
	mv_pp2_cls2_sysfs_exit(&pd->kobj);
	mv_pp2_cls_sysfs_exit(&pd->kobj);
	mv_pp2_prs_high_sysfs_exit(&pd->kobj);
	mv_pp2_gbe_sysfs_exit(&pd->kobj);
	/* can't delete, we call to init/clean function from this sysfs */
	/* TODO: open this line when we delete clean/init sysfs commands*/
	/*mv_pp2_dbg_sysfs_exit(&pd->kobj);*/
	platform_device_unregister(pp2_sysfs);
}

static int mv_eth_sysfs_init(void)
{
	struct device *pd;

	pd = bus_find_device_by_name(&platform_bus_type, NULL, "pp2");
	if (!pd) {
		pp2_sysfs = platform_device_register_simple("pp2", -1, NULL, 0);
		pd = bus_find_device_by_name(&platform_bus_type, NULL, "pp2");
	}

	if (!pd) {
		printk(KERN_ERR"%s: cannot find pp2 device\n", __func__);
		return -1;
	}

	mv_pp2_gbe_sysfs_init(&pd->kobj);
	mv_pp2_prs_high_sysfs_init(&pd->kobj);
	mv_pp2_cls_sysfs_init(&pd->kobj);
	mv_pp2_cls2_sysfs_init(&pd->kobj);
	mv_pp2_cls3_sysfs_init(&pd->kobj);
	mv_pp2_cls4_sysfs_init(&pd->kobj);
	mv_pp2_mc_sysfs_init(&pd->kobj);
	mv_pp2_plcr_sysfs_init(&pd->kobj);
	mv_pp2_pme_sysfs_init(&pd->kobj);
	mv_pp2_dbg_sysfs_init(&pd->kobj);
	mv_pp2_wol_sysfs_init(&pd->kobj);

#ifdef CONFIG_MV_ETH_PP2_1
	mv_pp2_dpi_sysfs_init(&pd->kobj);
#endif

#ifdef CONFIG_MV_ETH_L2FW
	mv_pp2_l2fw_sysfs_init(&pd->kobj);
#endif


	return 0;
}
static int	mv_eth_shared_probe(struct mv_pp2_pdata *plat_data)
{
	int size, cpu;

	mv_eth_sysfs_init();

	/* init MAC Unit */
	mv_eth_win_init();

	/* init MAC Unit */
	mv_eth_hal_shared_init(plat_data);

	mv_eth_config_show();

	size = mv_eth_ports_num * sizeof(struct eth_port *);
	mv_eth_ports = mvOsMalloc(size);
	if (!mv_eth_ports)
		goto oom;

	memset(mv_eth_ports, 0, size);

	/* Allocate aggregated TXQs control */
	size = CONFIG_NR_CPUS * sizeof(struct aggr_tx_queue);
	aggr_txqs = mvOsMalloc(size);
	if (!aggr_txqs)
		goto oom;

	memset(aggr_txqs, 0, size);
	for_each_possible_cpu(cpu) {
		aggr_txqs[cpu].txq_size = CONFIG_MV_ETH_AGGR_TXQ_SIZE;
		aggr_txqs[cpu].q = mvPp2AggrTxqInit(cpu, CONFIG_MV_ETH_AGGR_TXQ_SIZE);
		if (!aggr_txqs[cpu].q)
			goto oom;
	}

#ifdef CONFIG_MV_ETH_HWF
	/* Create temporary TXQ for switching between HWF and SWF */
	if (mvPp2TxqTempInit(CONFIG_MV_ETH_TEMP_TXQ_SIZE, CONFIG_MV_ETH_TEMP_TXQ_HWF_SIZE) != MV_OK)
		goto oom;
#endif

	if (mv_eth_bm_pools_init())
		goto oom;

	/* Parser default initialization */
	if (mv_eth_pnc_ctrl_en) {
		if (mvPrsDefaultInit())
			printk(KERN_ERR "%s: Warning PARSER default init failed\n", __func__);

		if (mvPp2ClassifierDefInit())
			printk(KERN_ERR "%s: Warning Classifier defauld init failed\n", __func__);
	}

#ifdef CONFIG_MV_ETH_PP2_1
	mvPp2DpiInit();
#endif

	/* Initialize tasklet for handle link events */
	tasklet_init(&link_tasklet, mv_eth_link_tasklet, 0);

	/* request IRQ for link interrupts from GOP */
	if (request_irq(IRQ_GLOBAL_GOP, mv_eth_link_isr, (IRQF_DISABLED), "mv_eth_link", NULL))
		printk(KERN_ERR "%s: Could not request IRQ for GOP interrupts\n", __func__);

	mvGmacIsrSummaryUnmask();

	mv_eth_initialized = 1;
	return 0;

oom:
	if (mv_eth_ports)
		mvOsFree(mv_eth_ports);

	if (aggr_txqs)
		mvOsFree(aggr_txqs);

	printk(KERN_ERR "%s: out of memory\n", __func__);
	return -ENOMEM;
}

static void mv_eth_shared_cleanup(void)
{
	int pool, cpu;

	/*
	There is no memory allocation in prser & classifier
	cleanup functions are not necessary
	*/

	for (pool = 0; pool < MV_ETH_BM_POOLS; pool++)
		mv_eth_pool_destroy(pool);

#ifdef CONFIG_MV_ETH_HWF
	/* Delete temporary TXQ (switching between HWF and SWF)*/
	mvPp2TxqTempDelete();
#endif
	/* cleanup aggregated tx queues */
	for_each_possible_cpu(cpu)
		mvPp2AggrTxqDelete(cpu);

	mvOsFree(aggr_txqs);

	mvOsFree(mv_eth_ports);

	/* Hal init by mv_eth_hal_shared_init*/
	mvPp2HalDestroy();

	/*mv_eth_win_cleanup();*/

	mv_eth_sysfs_exit();

	mv_eth_initialized = 0;
}

/***********************************************************
 * mv_eth_probe --                                         *
 *   main driver initialization. loading the interfaces.   *
 ***********************************************************/
static int mv_eth_probe(struct platform_device *pdev)
{
	struct mv_pp2_pdata *plat_data = (struct mv_pp2_pdata *)pdev->dev.platform_data;
	int phyAddr, is_sgmii, is_rgmii, port = pdev->id;

	if (!mv_eth_initialized) {

		mv_eth_ports_num = plat_data->max_port;

		if (mv_eth_shared_probe(plat_data))
			return -ENODEV;
	}

	if (!MV_PON_PORT(port)) {
		/* First: Disable Gmac */
		mvGmacPortDisable(port);

		/* Set the board information regarding PHY address */
		phyAddr = plat_data->phy_addr;
		if (phyAddr != -1) {
			mvGmacPhyAddrSet(port, phyAddr);
			mvEthPhyReset(phyAddr, 1000);
		}

		is_sgmii = (plat_data->flags & MV_PP2_PDATA_F_SGMII) ? 1 : 0;
		is_rgmii = (plat_data->flags & MV_PP2_PDATA_F_RGMII) ? 1 : 0;

		if (plat_data->flags & MV_PP2_PDATA_F_LB)
			mvGmacPortLbSet(port, (plat_data->speed == SPEED_1000), is_sgmii);

		mvGmacPortPowerUp(port, is_sgmii, is_rgmii);

		mvGmacPortSumIsrUnmask(port);
	}

	if (mv_eth_load_network_interfaces(pdev))
		return -ENODEV;

	/* used in mv_eth_all_ports_probe */
	plats[port] = pdev;

	return 0;
}


static int mv_eth_config_get(struct platform_device *pdev, MV_U8 *mac_addr)
{
	struct mv_pp2_pdata *plat_data = (struct mv_pp2_pdata *)pdev->dev.platform_data;

	if (mac_addr)
		memcpy(mac_addr, plat_data->mac_addr, MV_MAC_ADDR_SIZE);

	return plat_data->mtu;
}

/***********************************************************
 * mv_eth_tx_timeout --                                    *
 *   nothing to be done (?)                                *
 ***********************************************************/
static void mv_eth_tx_timeout(struct net_device *dev)
{
#ifdef CONFIG_MV_ETH_STAT_ERR
	struct eth_port *pp = MV_ETH_PRIV(dev);

	pp->stats.tx_timeout++;
#endif /* #ifdef CONFIG_MV_ETH_STAT_ERR */

	printk(KERN_INFO "%s: tx timeout\n", dev->name);
}

/***************************************************************
 * mv_eth_netdev_init -- Allocate and initialize net_device    *
 *                   structure                                 *
 ***************************************************************/
struct net_device *mv_eth_netdev_init(int mtu, u8 *mac, struct platform_device *pdev)
{
	struct net_device *dev;
	struct eth_port *dev_priv;
	struct resource *res;

	dev = alloc_etherdev_mq(sizeof(struct eth_port), CONFIG_MV_ETH_TXQ);
	if (!dev)
		return NULL;

	dev_priv = (struct eth_port *)netdev_priv(dev);
	if (!dev_priv)
		return NULL;

	memset(dev_priv, 0, sizeof(struct eth_port));

	dev_priv->dev = dev;

	res = platform_get_resource(pdev, IORESOURCE_IRQ, 0);
	BUG_ON(!res);
	dev->irq = res->start;

	dev->mtu = mtu;
	memcpy(dev->dev_addr, mac, MV_MAC_ADDR_SIZE);
	dev->tx_queue_len = CONFIG_MV_ETH_TXQ_DESC;
	dev->watchdog_timeo = 5 * HZ;

	if (MV_PON_PORT(dev_priv->port))
		dev->hard_header_len += MV_ETH_MH_SIZE;

	dev->netdev_ops = &mv_eth_netdev_ops;

	SET_ETHTOOL_OPS(dev, &mv_eth_tool_ops);

	SET_NETDEV_DEV(dev, &pdev->dev);

	return dev;

}

/***************************************************************
 * mv_eth_netdev_connect -- Connect device to linux            *
***************************************************************/
static int mv_eth_netdev_connect(struct eth_port *pp)
{
	struct net_device *dev;
	struct cpu_ctrl	*cpuCtrl;
	int cpu;

	if (!pp) {
		pr_err("\to failed to register, uninitialized port\n");
		return -ENODEV;
	}

	dev = pp->dev;

	for_each_possible_cpu(cpu) {
		cpuCtrl = pp->cpu_config[cpu];
		cpuCtrl->napi_group = NULL;
	}

	if (pp->flags & MV_ETH_F_CONNECT_LINUX) {
		mv_eth_netdev_init_features(pp->dev);
		if (register_netdev(dev)) {
			pr_err("\to failed to register %s\n", dev->name);
			free_netdev(dev);
			return -ENODEV;
		} else
			pr_info("\to %s, ifindex = %d, GbE port = %d", dev->name, dev->ifindex, pp->port);
	}

	return MV_OK;
}

bool mv_eth_netdev_find(unsigned int dev_idx)
{
	int port;

	for (port = 0; port < mv_eth_ports_num; port++) {
		struct eth_port *pp = mv_eth_port_by_id(port);

		if (pp && pp->dev && (pp->dev->ifindex == dev_idx))
			return true;
	}
	return false;
}
EXPORT_SYMBOL(mv_eth_netdev_find);


int mv_eth_hal_init(struct eth_port *pp)
{
	int rxq, txp, txq, size, cpu;
	struct tx_queue *txq_ctrl;
	struct txq_cpu_ctrl *txq_cpu_ptr;
	struct rx_queue *rxq_ctrl;

	/* Init port */
	pp->port_ctrl = mvPp2PortInit(pp->port, pp->first_rxq, pp->rxq_num, NULL);
	if (!pp->port_ctrl) {
		printk(KERN_ERR "%s: failed to load port=%d\n", __func__, pp->port);
		return -ENODEV;
	}

	size = pp->txp_num * CONFIG_MV_ETH_TXQ * sizeof(struct tx_queue);
	pp->txq_ctrl = mvOsMalloc(size);
	if (!pp->txq_ctrl)
		goto oom;

	memset(pp->txq_ctrl, 0, size);

	/* Create TX descriptor rings */
	for (txp = 0; txp < pp->txp_num; txp++) {
		for (txq = 0; txq < CONFIG_MV_ETH_TXQ; txq++) {
			txq_ctrl = &pp->txq_ctrl[txp * CONFIG_MV_ETH_TXQ + txq];

			txq_ctrl->q = NULL;
			txq_ctrl->txp = txp;
			txq_ctrl->txq = txq;
			txq_ctrl->txq_size = CONFIG_MV_ETH_TXQ_DESC;
			txq_ctrl->hwf_size = CONFIG_MV_ETH_TXQ_HWF_DESC;

#ifdef CONFIG_MV_ETH_PP2_1
			txq_ctrl->rsvd_chunk = MV_ETH_CPU_DESC_CHUNK;
			txq_ctrl->hwf_size =
				txq_ctrl->txq_size - (nr_cpu_ids * txq_ctrl->rsvd_chunk);
			txq_ctrl->swf_size =
				txq_ctrl->hwf_size - (nr_cpu_ids * txq_ctrl->rsvd_chunk);
#endif
			for_each_possible_cpu(cpu) {
				txq_cpu_ptr = &txq_ctrl->txq_cpu[cpu];
				txq_cpu_ptr->shadow_txq = NULL;
#ifdef CONFIG_MV_ETH_PP2_1
				/* In default, all CPUs and TXQs have same chunk size */
				txq_cpu_ptr->txq_size = CONFIG_MV_ETH_TXQ_DESC;
				txq_cpu_ptr->reserved_num = 0;
#else
				txq_cpu_ptr->txq_size =
					(CONFIG_MV_ETH_TXQ_DESC - CONFIG_MV_ETH_TXQ_HWF_DESC) / nr_cpu_ids;
#endif
				txq_cpu_ptr->txq_count = 0;
				txq_cpu_ptr->shadow_txq_put_i = 0;
				txq_cpu_ptr->shadow_txq_get_i = 0;
			}

			txq_ctrl->txq_done_pkts_coal = mv_ctrl_txdone;
		}
	}

	pp->rxq_ctrl = mvOsMalloc(pp->rxq_num * sizeof(struct rx_queue));
	if (!pp->rxq_ctrl)
		goto oom;

	memset(pp->rxq_ctrl, 0, pp->rxq_num * sizeof(struct rx_queue));

	/* Create Rx descriptor rings */
	for (rxq = 0; rxq < pp->rxq_num; rxq++) {
		rxq_ctrl = &pp->rxq_ctrl[rxq];
		rxq_ctrl->rxq_size = CONFIG_MV_ETH_RXQ_DESC;
		rxq_ctrl->rxq_pkts_coal = CONFIG_MV_ETH_RX_COAL_PKTS;
		rxq_ctrl->rxq_time_coal = CONFIG_MV_ETH_RX_COAL_USEC;
	}

	if (pp->tx_spec.flags & MV_ETH_TX_F_MH)
		mvPp2MhSet(pp->port, MV_TAG_TYPE_MH);

	/* Configure defaults */
	pp->autoneg_cfg = AUTONEG_ENABLE;
	pp->speed_cfg = SPEED_1000;
	pp->duplex_cfg = DUPLEX_FULL;
	pp->advertise_cfg = 0x2f;
	pp->rx_time_coal_cfg = CONFIG_MV_ETH_RX_COAL_USEC;
	pp->rx_pkts_coal_cfg = CONFIG_MV_ETH_RX_COAL_PKTS;
	pp->tx_pkts_coal_cfg = mv_ctrl_txdone;
	pp->rx_time_low_coal_cfg = CONFIG_MV_ETH_RX_COAL_USEC >> 2;
	pp->rx_time_high_coal_cfg = CONFIG_MV_ETH_RX_COAL_USEC << 2;
	pp->rx_pkts_low_coal_cfg = CONFIG_MV_ETH_RX_COAL_PKTS;
	pp->rx_pkts_high_coal_cfg = CONFIG_MV_ETH_RX_COAL_PKTS;
	pp->pkt_rate_low_cfg = 1000;
	pp->pkt_rate_high_cfg = 50000;
	pp->rate_sample_cfg = 5;
	pp->rate_current = 0; /* Unknown */

	return 0;
oom:
	printk(KERN_ERR "%s: port=%d: out of memory\n", __func__, pp->port);
	return -ENODEV;
}

/* Show network driver configuration */
void mv_eth_config_show(void)
{
#ifdef CONFIG_MV_ETH_PP2_1
	pr_info("  o	PPv2.1 Giga driver\n");
#else
	pr_info("  o	PPv2.0 Giga driver\n");
#endif

	printk(KERN_ERR "  o %d Giga ports supported\n", mv_eth_ports_num);

#ifdef CONFIG_MV_INCLUDE_PON
	printk(KERN_ERR "  o xPON port is #%d: - %d of %d TCONTs supported\n",
		MV_PON_LOGIC_PORT_GET(), CONFIG_MV_PON_TCONTS, MV_ETH_MAX_TCONT);
#endif

#ifdef CONFIG_NET_SKB_RECYCLE
	printk(KERN_ERR "  o SKB recycle supported (%s)\n", mv_ctrl_recycle ? "Enabled" : "Disabled");
#endif

	printk(KERN_ERR "  o BM supported for CPU: %d BM pools\n", MV_ETH_BM_POOLS);

#ifdef CONFIG_MV_ETH_HWF
	printk(KERN_ERR "  o HWF supported\n");
#endif

#ifdef CONFIG_MV_ETH_PMT
	printk(KERN_ERR "  o PME supported\n");
#endif

	printk(KERN_ERR "  o RX Queue support: %d Queues * %d Descriptors\n", CONFIG_MV_ETH_RXQ, CONFIG_MV_ETH_RXQ_DESC);

	printk(KERN_ERR "  o TX Queue support: %d Queues * %d Descriptors\n", CONFIG_MV_ETH_TXQ, CONFIG_MV_ETH_TXQ_DESC);

#if defined(CONFIG_MV_ETH_TSO)
	printk(KERN_ERR "  o GSO supported\n");
#endif /* CONFIG_MV_ETH_TSO */

#if defined(CONFIG_MV_ETH_RX_CSUM_OFFLOAD)
	printk(KERN_ERR "  o Receive checksum offload supported\n");
#endif
#if defined(CONFIG_MV_ETH_TX_CSUM_OFFLOAD)
	printk(KERN_ERR "  o Transmit checksum offload supported\n");
#endif

#ifdef CONFIG_MV_ETH_STAT_ERR
	printk(KERN_ERR "  o Driver ERROR statistics enabled\n");
#endif

#ifdef CONFIG_MV_ETH_STAT_INF
	printk(KERN_ERR "  o Driver INFO statistics enabled\n");
#endif

#ifdef CONFIG_MV_ETH_STAT_DBG
	printk(KERN_ERR "  o Driver DEBUG statistics enabled\n");
#endif

#ifdef ETH_DEBUG
	printk(KERN_ERR "  o Driver debug messages enabled\n");
#endif

#if defined(CONFIG_MV_INCLUDE_SWITCH)
	printk(KERN_ERR "  o Switch support enabled\n");
#endif /* CONFIG_MV_INCLUDE_SWITCH */

	printk(KERN_ERR "\n");
}

/* Set network device features on initialization. Take into account default compile time configuration. */
void mv_eth_netdev_init_features(struct net_device *dev)
{
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 39)
	dev->features = NETIF_F_RXCSUM | NETIF_F_IP_CSUM | NETIF_F_SG | NETIF_F_LLTX;
#else
	dev->features = NETIF_F_IP_CSUM | NETIF_F_SG | NETIF_F_LLTX;
#endif
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 39)
	dev->hw_features = NETIF_F_GRO | NETIF_F_RXCSUM | NETIF_F_IP_CSUM | NETIF_F_SG;
#endif

#ifdef CONFIG_MV_ETH_TSO
	dev->features |= NETIF_F_TSO;
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 39)
	dev->hw_features |= NETIF_F_TSO;
#endif
#endif
}

#if LINUX_VERSION_CODE < KERNEL_VERSION(3, 4, 25)
static u32 mv_eth_netdev_fix_features(struct net_device *dev, u32 features)
{
	if (dev->mtu > MV_ETH_TX_CSUM_MAX_SIZE)
		if (features & (NETIF_F_IP_CSUM | NETIF_F_TSO)) {
			features &= ~(NETIF_F_IP_CSUM | NETIF_F_TSO);
			printk(KERN_ERR "%s: NETIF_F_IP_CSUM and NETIF_F_TSO not supported for mtu larger %d bytes\n",
				dev->name, MV_ETH_TX_CSUM_MAX_SIZE);
		}

	return features;
}
#else
static netdev_features_t mv_eth_netdev_fix_features(struct net_device *dev, netdev_features_t features)
{
	if (dev->mtu > MV_ETH_TX_CSUM_MAX_SIZE)
		if (features & (NETIF_F_IP_CSUM | NETIF_F_TSO)) {
			features &= ~(NETIF_F_IP_CSUM | NETIF_F_TSO);
			printk(KERN_ERR "%s: NETIF_F_IP_CSUM and NETIF_F_TSO not supported for mtu larger %d bytes\n",
				dev->name, MV_ETH_TX_CSUM_MAX_SIZE);
		}

	return features;
}
#endif

static int mv_eth_rxq_fill(struct eth_port *pp, int rxq, int num)
{
	mvPp2RxqNonOccupDescAdd(pp->port, rxq, num);
	return num;
}

static int mv_eth_txq_create(struct eth_port *pp, struct tx_queue *txq_ctrl)
{
	int cpu;
	struct txq_cpu_ctrl *txq_cpu_ptr;

	txq_ctrl->q = mvPp2TxqInit(pp->port, txq_ctrl->txp, txq_ctrl->txq, txq_ctrl->txq_size, txq_ctrl->hwf_size);
	if (txq_ctrl->q == NULL) {
#ifdef CONFIG_MV_ETH_PP2_1
		printk(KERN_ERR "%s: can't create TxQ - port=%d, txp=%d, txq=%d, desc=%d, hwf desc=%d swf desc = %d\n",
			__func__, pp->port, txq_ctrl->txp, txq_ctrl->txp,
			txq_ctrl->txq_size, txq_ctrl->hwf_size, txq_ctrl->swf_size);
#else
		printk(KERN_ERR "%s: can't create TxQ - port=%d, txp=%d, txq=%d, desc=%d, hwf desc=%d\n",
		       __func__, pp->port, txq_ctrl->txp, txq_ctrl->txp, txq_ctrl->txq_size, txq_ctrl->hwf_size);
#endif
		return -ENODEV;
	}

	for_each_possible_cpu(cpu) {
		txq_cpu_ptr = &txq_ctrl->txq_cpu[cpu];
		txq_cpu_ptr->shadow_txq = mvOsMalloc(txq_cpu_ptr->txq_size * sizeof(MV_U32));
		if (txq_cpu_ptr->shadow_txq == NULL)
			goto no_mem;
		/* reset txq */
		txq_cpu_ptr->txq_count = 0;
		txq_cpu_ptr->shadow_txq_put_i = 0;
		txq_cpu_ptr->shadow_txq_get_i = 0;
	}
	return 0;

no_mem:
	mv_eth_txq_delete(pp, txq_ctrl);
	return -ENOMEM;
}


static void mv_eth_txq_delete(struct eth_port *pp, struct tx_queue *txq_ctrl)
{
	int cpu;
	struct txq_cpu_ctrl *txq_cpu_ptr;

	for_each_possible_cpu(cpu) {
		txq_cpu_ptr = &txq_ctrl->txq_cpu[cpu];
		if (txq_cpu_ptr->shadow_txq) {
			mvOsFree(txq_cpu_ptr->shadow_txq);
			txq_cpu_ptr->shadow_txq = NULL;
		}
	}

	if (txq_ctrl->q) {
		mvPp2TxqDelete(pp->port, txq_ctrl->txp, txq_ctrl->txq);
		txq_ctrl->q = NULL;
	}
}

int mv_eth_txq_clean(int port, int txp, int txq)
{
	struct eth_port *pp;
	struct tx_queue *txq_ctrl;
	int msec, pending, tx_done;

	if (mvPp2TxpCheck(port, txp))
		return -EINVAL;

	pp = mv_eth_port_by_id(port);
	if ((pp == NULL) || (pp->txq_ctrl == NULL))
		return -ENODEV;

	if (mvPp2MaxCheck(txq, CONFIG_MV_ETH_TXQ, "txq"))
		return -EINVAL;

	txq_ctrl = &pp->txq_ctrl[txp * CONFIG_MV_ETH_TXQ + txq];
	if (txq_ctrl->q) {
		/* Enable TXQ drain */
		mvPp2TxqDrainSet(port, txp, txq, MV_TRUE);

		/* Wait for all packets to be transmitted */
		msec = 0;
		do {
			if (msec >= MV_ETH_TX_PENDING_TIMEOUT_MSEC) {
				pr_err("port=%d, txp=%d txq=%d: timeout for transmit pending descriptors\n",
					port, txp, txq);
				break;
			}
			mdelay(1);
			msec++;

			pending = mvPp2TxqPendDescNumGet(port, txp, txq);
		} while (pending);

		/* Disable TXQ Drain */
		mvPp2TxqDrainSet(port, txp, txq, MV_FALSE);

		/* release all transmitted packets */

		tx_done = mv_eth_txq_done(pp, txq_ctrl);
		if (tx_done > 0)
			mvOsPrintf(KERN_INFO "%s: port=%d, txp=%d txq=%d: Free %d transmitted descriptors\n",
				__func__, port, txp, txq, tx_done);

		/* release all untransmitted packets */
		tx_done = mv_eth_txq_done_force(pp, txq_ctrl);
		if (tx_done > 0)
			mvOsPrintf(KERN_INFO "%s: port=%d, txp=%d txq=%d: Free %d untransmitted descriptors\n",
				__func__, port, txp, txq, tx_done);
	}
	return 0;
}

/* Free all packets pending transmit from all TXQs and reset TX port */
int mv_eth_txp_clean(int port, int txp)
{
	struct eth_port *pp;
	int txq;

	if (mvPp2TxpCheck(port, txp))
		return -EINVAL;

	pp = mv_eth_port_by_id(port);
	if ((pp == NULL) || (pp->txq_ctrl == NULL))
		return -ENODEV;

	if (pp->flags & MV_ETH_F_STARTED) {
		printk(KERN_ERR "Port %d must be stopped before\n", port);
		return -EINVAL;
	}

	/* Flush TX FIFO */
	mvPp2TxPortFifoFlush(port, MV_TRUE);

	/* free the skb's in the hal tx ring */
	for (txq = 0; txq < CONFIG_MV_ETH_TXQ; txq++)
		mv_eth_txq_clean(port, txp, txq);

	mvPp2TxPortFifoFlush(port, MV_FALSE);

	return 0;
}

/* Free received packets from all RXQs and reset RX of the port */
int mv_eth_rx_reset(int port)
{
	struct eth_port *pp = mv_eth_port_by_id(port);

	if (pp->flags & MV_ETH_F_STARTED) {
		printk(KERN_ERR "Port %d must be stopped before\n", port);
		return -EINVAL;
	}

	mvPp2RxReset(port);
	return 0;
}

/***********************************************************
 * coal set functions		                           *
 ***********************************************************/
MV_STATUS mv_eth_rx_ptks_coal_set(int port, int rxq, MV_U32 value)
{
	MV_STATUS status = mvPp2RxqPktsCoalSet(port, rxq, value);
	struct eth_port *pp = mv_eth_port_by_id(port);
	if (status == MV_OK)
		pp->rxq_ctrl[rxq].rxq_pkts_coal = value;
	return status;
}

MV_STATUS mv_eth_rx_time_coal_set(int port, int rxq, MV_U32 value)
{

	MV_STATUS status = mvPp2RxqTimeCoalSet(port, rxq, value);
	struct eth_port *pp = mv_eth_port_by_id(port);
	if (status == MV_OK)
		pp->rxq_ctrl[rxq].rxq_time_coal = value;
	return status;

	return  MV_OK;
}

MV_STATUS mv_eth_tx_done_ptks_coal_set(int port, int txp, int txq, MV_U32 value)
{
	MV_STATUS status = mvPp2TxDonePktsCoalSet(port, txp, txq, value);
	struct eth_port *pp = mv_eth_port_by_id(port);
	if (status == MV_OK)
		pp->txq_ctrl[txp * CONFIG_MV_ETH_TXQ + txq].txq_done_pkts_coal = value;
	return status;
}

/***********************************************************
* mv_eth_start_internals --                               *
*   fill rx buffers. start rx/tx activity. set coalesing. *
*   clear and unmask interrupt bits                       *
*   -   RX and TX init
*   -   HW port enable
*   -   HW enable port tx
*   -   Enable NAPI
*   -   Enable interrupts (RXQ still close .. interrupts will not received)
*   -   SW start tx (wake_up _all_queues)
*   -   HW start rx
***********************************************************/
int mv_eth_start_internals(struct eth_port *pp, int mtu)
{
	int rxq, txp, txq, err = 0;

	if (test_bit(MV_ETH_F_STARTED_BIT, &(pp->flags))) {
		STAT_ERR(pp->stats.state_err++);
		printk(KERN_ERR "%s: port %d, wrong state: STARTED_BIT = 1\n", __func__, pp->port);
		err = -EINVAL;
		goto out;
	}

	if (!MV_PON_PORT(pp->port))
		mvGmacMaxRxSizeSet(pp->port, RX_PKT_SIZE(mtu));
#ifdef CONFIG_MV_INCLUDE_PON
	else
		mv_pon_mtu_config(RX_PKT_SIZE(mtu));
#endif

	err = mv_eth_swf_bm_pool_init(pp, mtu);
	if (err)
		goto out;
#ifdef CONFIG_MV_ETH_HWF
	err = mv_eth_hwf_bm_pool_init(pp, mtu);
	if (err)
		goto out;
#endif /* CONFIG_MV_ETH_HWF */


	for (rxq = 0; rxq < pp->rxq_num; rxq++) {
		if (pp->rxq_ctrl[rxq].q == NULL) {
			/* allocate descriptors and initialize RXQ */
			pp->rxq_ctrl[rxq].q = mvPp2RxqInit(pp->port, rxq, pp->rxq_ctrl[rxq].rxq_size);
			if (!pp->rxq_ctrl[rxq].q) {
				printk(KERN_ERR "%s: can't create RxQ port=%d, rxq=%d, desc=%d\n",
				       __func__, pp->port, rxq, pp->rxq_ctrl[rxq].rxq_size);
				err = -ENODEV;
				goto out;
			}
			/* Set Offset  - at this point logical RXQs are already mappedto physical RXQs */
			mvPp2RxqOffsetSet(pp->port, rxq, NET_SKB_PAD);
		}

		/* Set coalescing pkts and time */
		mv_eth_rx_ptks_coal_set(pp->port, rxq, pp->rxq_ctrl[rxq].rxq_pkts_coal);
		mv_eth_rx_time_coal_set(pp->port, rxq, pp->rxq_ctrl[rxq].rxq_time_coal);

		if (!(pp->flags & MV_ETH_F_IFCAP_NETMAP)) {
			if (mvPp2RxqFreeDescNumGet(pp->port, rxq) == 0)
				mv_eth_rxq_fill(pp, rxq, pp->rxq_ctrl[rxq].rxq_size);
		} else {
			/*printk(KERN_ERR "%s :: run with netmap enable", __func__);*/
			mvPp2RxqNonOccupDescAdd(pp->port, rxq, pp->rxq_ctrl[rxq].rxq_size);
#ifdef CONFIG_NETMAP
			if (pp2_netmap_rxq_init_buffers(pp, rxq))
				return MV_ERROR;
#endif
		}
	}

	for (txp = 0; txp < pp->txp_num; txp++) {
		for (txq = 0; txq < CONFIG_MV_ETH_TXQ; txq++) {
			struct tx_queue *txq_ctrl = &pp->txq_ctrl[txp * CONFIG_MV_ETH_TXQ + txq];

			if ((txq_ctrl->q == NULL) && (txq_ctrl->txq_size > 0)) {
				err = mv_eth_txq_create(pp, txq_ctrl);
				if (err)
					goto out;
				spin_lock_init(&txq_ctrl->queue_lock);
			}
#ifdef CONFIG_MV_ETH_TXDONE_ISR
			mv_eth_tx_done_ptks_coal_set(pp->port, txp, txq, txq_ctrl->txq_done_pkts_coal);
#endif /* CONFIG_MV_ETH_TXDONE_ISR */
#ifdef CONFIG_NETMAP
		if (pp->flags & MV_ETH_F_IFCAP_NETMAP) {
			if (pp2_netmap_txq_init_buffers(pp, txp, txq))
				return MV_ERROR;
		}
#endif /* CONFIG_NETMAP */
		}
		mvPp2TxpMaxTxSizeSet(pp->port, txp, RX_PKT_SIZE(mtu));
	}
	/* TODO: set speed, duplex, fc with ethtool parameres (speed_cfg, etc..) */

	set_bit(MV_ETH_F_STARTED_BIT, &(pp->flags));
 out:
	return err;
}



int mv_eth_resume_internals(struct eth_port *pp, int mtu)
{
/* TBD */
	return 0;

}


int mv_eth_restore_registers(struct eth_port *pp, int mtu)
{
/* TBD */
	return 0;
}


/***********************************************************
 * mv_eth_suspend_internals --                                *
 *   stop port rx/tx activity. free skb's from rx/tx rings.*
 ***********************************************************/
int mv_eth_suspend_internals(struct eth_port *pp)
{
/* TBD */
	return 0;
}


/***********************************************************
* mv_eth_stop_internals --                                *
*   -   HW stop rx
*   -   SW stop tx (tx_stop_all_queues)
*   -   Disable interrupts
*   -   Disable NAPI
*   -   HW  disable port tx
*   -   HW disable port
*   -   RX and TX cleanups
***********************************************************/
int mv_eth_stop_internals(struct eth_port *pp)
{
	int queue, txp;

	if (!test_and_clear_bit(MV_ETH_F_STARTED_BIT, &(pp->flags))) {
		STAT_ERR(pp->stats.state_err++);
		printk(KERN_ERR "%s: port %d, wrong state: STARTED_BIT = 0.\n", __func__, pp->port);
		goto error;
	}

	mdelay(10);

	/* Transmit and free all packets */
	for (txp = 0; txp < pp->txp_num; txp++)
		mv_eth_txp_clean(pp->port, txp);


	/* free the skb's in the hal rx ring */
	for (queue = 0; queue < pp->rxq_num; queue++)
		mv_eth_rxq_drop_pkts(pp, queue);

	return 0;

error:
	printk(KERN_ERR "GbE port %d: stop internals failed\n", pp->port);
	return -1;
}

/* return positive if MTU is valid */
int mv_eth_check_mtu_valid(struct net_device *dev, int mtu)
{
	if (mtu < 68) {
		printk(KERN_INFO "MTU must be at least 68, change mtu failed\n");
		return -EINVAL;
	}
	if (mtu > 9676 /* 9700 - 20 and rounding to 8 */) {
		printk(KERN_ERR "%s: Illegal MTU value %d, ", dev->name, mtu);
		mtu = 9676;
		printk(KERN_CONT " rounding MTU to: %d \n", mtu);
	}

	if (MV_IS_NOT_ALIGN(RX_PKT_SIZE(mtu), 8)) {
		printk(KERN_ERR "%s: Illegal MTU value %d, ", dev->name, mtu);
		mtu = MV_ALIGN_UP(RX_PKT_SIZE(mtu), 8);
		printk(KERN_CONT " rounding MTU to: %d \n", mtu);
	}
	return mtu;
}

/* Check if MTU can be changed */
int mv_eth_check_mtu_internals(struct net_device *dev, int mtu)
{
	struct eth_port *pp = MV_ETH_PRIV(dev);
	struct bm_pool *port_pool;

	if (!pp)
		return -EPERM;

	port_pool = pp->pool_long;

	if (!port_pool)
		return 0;

	/* long pool is not shared with other ports */
	if ((port_pool) && (port_pool->port_map == (1 << pp->port)))
		return 0;

	return 1;
}

/***********************************************************
 * mv_eth_change_mtu_internals --                          *
 *   stop port activity. release skb from rings. set new   *
 *   mtu in device and hw. restart port activity and       *
 *   and fill rx-buiffers with size according to new mtu.  *
 ***********************************************************/
int mv_eth_change_mtu_internals(struct net_device *dev, int mtu)
{
	struct bm_pool *port_pool;
	struct eth_port *pp = MV_ETH_PRIV(dev);
	int pkt_size = RX_PKT_SIZE(mtu), pkts_num;
	unsigned long flags = 0;

	if (test_bit(MV_ETH_F_STARTED_BIT, &(pp->flags))) {
		STAT_ERR(pp->stats.state_err++);
#ifdef CONFIG_MV_ETH_DEBUG_CODE
		if (pp->dbg_flags & MV_ETH_F_DBG_RX)
			printk(KERN_ERR "%s: port %d, STARTED_BIT = 0, Invalid value.\n", __func__, pp->port);
#endif
		return -1;
	}

	if (mtu == dev->mtu)
		goto mtu_out;

	port_pool = pp->pool_long;

	if (port_pool) {
		MV_ETH_LOCK(&port_pool->lock, flags);
		pkts_num = port_pool->buf_num;
		/* for now, swf long pool must not be shared with other ports */
		if (port_pool->port_map == (1 << pp->port)) {
			/* refill pool with updated buffer size */
			mv_eth_pool_free(port_pool->pool, pkts_num);
			port_pool->pkt_size = pkt_size;
			mv_eth_pool_add(port_pool->pool, pkts_num);
		} else {
			printk(KERN_ERR "%s: port %d, SWF long pool is shared with other ports.\n", __func__, pp->port);
			MV_ETH_UNLOCK(&port_pool->lock, flags);
			return -1;
		}
		mvPp2BmPoolBufSizeSet(port_pool->pool, RX_BUF_SIZE(port_pool->pkt_size));
		MV_ETH_UNLOCK(&port_pool->lock, flags);
	}

#ifdef CONFIG_MV_ETH_HWF
	port_pool = pp->hwf_pool_long;

	if (port_pool && (pp->hwf_pool_long != pp->pool_long)) {
		MV_ETH_LOCK(&port_pool->lock, flags);
		pkts_num = port_pool->buf_num;
		/* for now, hwf long pool must not be shared with other ports */
		if (port_pool->port_map == (1 << pp->port)) {
			/* refill pool with updated buffer size */
			mv_eth_pool_free(port_pool->pool, pkts_num);
			port_pool->pkt_size = pkt_size;
			mv_eth_pool_add(port_pool->pool, pkts_num);
		} else {
			printk(KERN_ERR "%s: port %d, HWF long pool is shared with other ports.\n", __func__, pp->port);
			MV_ETH_UNLOCK(&port_pool->lock, flags);
			return -1;
		}
		mvPp2BmPoolBufSizeSet(port_pool->pool, RX_HWF_BUF_SIZE(port_pool->pkt_size));
		MV_ETH_UNLOCK(&port_pool->lock, flags);
	}
#endif /* CONFIG_MV_ETH_HWF */

	if (!MV_PON_PORT(pp->port))
		mvGmacMaxRxSizeSet(pp->port, pkt_size);
#ifdef CONFIG_MV_INCLUDE_PON
	else
		mv_pon_mtu_config(pkt_size);
#endif

#ifndef CONFIG_MV_ETH_PP2_1
	mv_eth_tx_mtu_set(pp->port, pkt_size);
#endif

mtu_out:
	dev->mtu = mtu;
#if LINUX_VERSION_CODE >= KERNEL_VERSION(3, 0, 0)
	netdev_update_features(dev);
#else
	netdev_features_change(dev);
#endif
	return 0;
}

/***********************************************************
 * mv_eth_tx_done_timer_callback --			   *
 *   N msec periodic callback for tx_done                  *
 ***********************************************************/
static void mv_eth_tx_done_timer_callback(unsigned long data)
{
	struct cpu_ctrl *cpuCtrl = (struct cpu_ctrl *)data;
	struct eth_port *pp = cpuCtrl->pp;
	int tx_done = 0, tx_todo = 0;
	unsigned int txq_mask;

	STAT_INFO(pp->stats.tx_done_timer_event[smp_processor_id()]++);

	clear_bit(MV_ETH_F_TX_DONE_TIMER_BIT, &(cpuCtrl->flags));

	if (!test_bit(MV_ETH_F_STARTED_BIT, &(pp->flags))) {
		STAT_INFO(pp->stats.netdev_stop++);
#ifdef CONFIG_MV_ETH_DEBUG_CODE
		if (pp->dbg_flags & MV_ETH_F_DBG_TX)
			printk(KERN_ERR "%s: port #%d is stopped, STARTED_BIT = 0, exit timer.\n", __func__, pp->port);
#endif /* CONFIG_MV_ETH_DEBUG_CODE */

		return;
	}

	if (MV_PON_PORT(pp->port))
		tx_done = mv_eth_tx_done_pon(pp, &tx_todo);
	else {
		/* check all possible queues, as there is no indication from interrupt */
		txq_mask = (1 << CONFIG_MV_ETH_TXQ) - 1;
		tx_done = mv_eth_tx_done_gbe(pp, txq_mask, &tx_todo);
	}

	if (cpuCtrl->cpu != smp_processor_id()) {
		pr_warning("%s: Called on other CPU - %d != %d\n", __func__, cpuCtrl->cpu, smp_processor_id());
		cpuCtrl = pp->cpu_config[smp_processor_id()];
	}
	if (tx_todo > 0)
		mv_eth_add_tx_done_timer(cpuCtrl);
}

void mv_eth_mac_show(int port)
{
	struct eth_port *pp = mv_eth_port_by_id(port);

	if (pp == NULL) {
		printk(KERN_ERR "%s: port %d entry is null \n", __func__, port);
		return;
	}

	/* TODO - example in NETA */
}

/********************************************/
/*		DSCP API		    */
/********************************************/
void mv_eth_dscp_map_show(int port)
{
	int dscp, txq;
	struct eth_port *pp = mv_eth_port_by_id(port);

	if (pp == NULL) {
		printk(KERN_ERR "%s: port %d entry is null \n", __func__, port);
		return;
	}

	/* TODO - call pnc_ipv4_dscp_show() example in NETA */

	printk(KERN_ERR "\n");
	printk(KERN_ERR " DSCP <=> TXQ map for port #%d\n\n", port);
	for (dscp = 0; dscp < sizeof(pp->txq_dscp_map); dscp++) {
		txq = pp->txq_dscp_map[dscp];
		if (txq != MV_ETH_TXQ_INVALID)
			printk(KERN_ERR "0x%02x <=> %d\n", dscp, txq);
	}
}

int mv_eth_rxq_dscp_map_set(int port, int rxq, unsigned char dscp)
{
	/* TBD */
	printk(KERN_ERR "Not supported\n");

	return MV_FAIL;
}

/* Set TXQ for special DSCP value. txq=-1 - use default TXQ for this port */
int mv_eth_txq_dscp_map_set(int port, int txq, unsigned char dscp)
{
	MV_U8 old_txq;
	struct eth_port *pp = mv_eth_port_by_id(port);

	if (mvPp2PortCheck(port))
		return -EINVAL;

	if ((pp == NULL) || (pp->txq_ctrl == NULL))
		return -ENODEV;

	if ((dscp < 0) || (dscp >= 64))
		return -EINVAL;

	old_txq = pp->txq_dscp_map[dscp];

	/* The same txq - do nothing */
	if (old_txq == (MV_U8) txq)
		return 0;

	if (txq == -1) {
		pp->txq_dscp_map[dscp] = MV_ETH_TXQ_INVALID;
		return 0;
	}

	if ((txq < 0) || (txq >= CONFIG_MV_ETH_TXQ))
		return -EINVAL;

	pp->txq_dscp_map[dscp] = (MV_U8) txq;

	return 0;
}

/********************************************/

void mv_eth_vlan_prio_show(int port)
{
	struct eth_port *pp = mv_eth_port_by_id(port);

	if (pp == NULL) {
		printk(KERN_ERR "%s: port %d entry is null \n", __func__, port);
		return;
	}

	/* TODO - example in NETA */
}

int mv_eth_rxq_vlan_prio_set(int port, int rxq, unsigned char prio)
{
	int status = -1;
	/*
	TODO - example in NETA
	status = pnc_vlan_prio_set(port, prio, rxq);
	*/

	if (status == 0)
		printk(KERN_ERR "Succeeded\n");
	else if (status == -1)
		printk(KERN_ERR "Not supported\n");
	else
		printk(KERN_ERR "Failed\n");

	return status;
}


static int mv_eth_priv_init(struct eth_port *pp, int port)
{
	static int first_rxq = 0;
	static int first_rx_q[MV_ETH_MAX_PORTS];
	int cpu, i;
	struct cpu_ctrl	*cpuCtrl;
	u8	*ext_buf;

	/* Default field per cpu initialization */
	for (i = 0; i < CONFIG_NR_CPUS; i++) {
		pp->cpu_config[i] = kmalloc(sizeof(struct cpu_ctrl), GFP_KERNEL);
		memset(pp->cpu_config[i], 0, sizeof(struct cpu_ctrl));
	}
	/* init only once */
	if (first_rxq == 0)
		for (i = 0; i < MV_ETH_MAX_PORTS; i++)
			first_rx_q[i] = -1;

	pp->port = port;
	pp->rxq_num = CONFIG_MV_ETH_RXQ;
	pp->txp_num = 1;
	pp->tx_spec.flags = 0;
	pp->tx_spec.txp = 0;

	if (first_rx_q[port] == -1) {
		first_rx_q[port] = first_rxq;
		first_rxq += pp->rxq_num;
	}

	pp->first_rxq = first_rx_q[port];

	for_each_possible_cpu(cpu) {
		cpuCtrl = pp->cpu_config[cpu];
		cpuCtrl->txq = CONFIG_MV_ETH_TXQ_DEF;
		cpuCtrl->pp = pp;
		cpuCtrl->cpu = cpu;
	}

	pp->flags = 0;

#ifdef CONFIG_MV_ETH_RX_DESC_PREFETCH
	pp->flags |= MV_ETH_F_RX_DESC_PREFETCH;
#endif

#ifdef CONFIG_MV_ETH_RX_PKT_PREFETCH
	pp->flags |= MV_ETH_F_RX_PKT_PREFETCH;
#endif

	for (i = 0; i < 64; i++)
		pp->txq_dscp_map[i] = MV_ETH_TXQ_INVALID;
#ifdef CONFIG_MV_ETH_TX_SPECIAL
	pp->tx_special_check = NULL;
#endif /* CONFIG_MV_ETH_TX_SPECIAL */

#ifdef CONFIG_MV_INCLUDE_PON
	if (MV_PON_PORT(port)) {
		pp->tx_spec.flags |= MV_ETH_TX_F_MH;
		pp->txp_num = CONFIG_MV_PON_TCONTS;
		pp->tx_spec.txp = CONFIG_MV_PON_TXP_DEF;
		for_each_possible_cpu(i)
			pp->cpu_config[i]->txq = CONFIG_MV_PON_TXQ_DEF;
	}
#endif

	for_each_possible_cpu(cpu) {
		cpuCtrl = pp->cpu_config[cpu];
		memset(&cpuCtrl->tx_done_timer, 0, sizeof(struct timer_list));
		cpuCtrl->tx_done_timer.function = mv_eth_tx_done_timer_callback;
		cpuCtrl->tx_done_timer.data = (unsigned long)cpuCtrl;
		init_timer(&cpuCtrl->tx_done_timer);
		clear_bit(MV_ETH_F_TX_DONE_TIMER_BIT, &(cpuCtrl->flags));
	}

	pp->weight = CONFIG_MV_ETH_RX_POLL_WEIGHT;

	/* Init pool of external buffers for TSO, fragmentation, etc */
	spin_lock_init(&pp->extLock);
	pp->extBufSize = CONFIG_MV_ETH_EXTRA_BUF_SIZE;
	pp->extArrStack = mvStackCreate(CONFIG_MV_ETH_EXTRA_BUF_NUM);
	if (pp->extArrStack == NULL) {
		pr_err("\to %s: Error: failed create  extArrStack for port #%d\n", __func__, port);
		return -ENOMEM;
	}

	for (i = 0; i < CONFIG_MV_ETH_EXTRA_BUF_NUM; i++) {
		ext_buf = mvOsMalloc(CONFIG_MV_ETH_EXTRA_BUF_SIZE);
		if (ext_buf == NULL) {
			pr_warn("\to %s Warning: %d of %d extra buffers allocated\n",
				__func__, i, CONFIG_MV_ETH_EXTRA_BUF_NUM);
			break;
		}
		mvStackPush(pp->extArrStack, (MV_U32)ext_buf);
	}

#ifdef CONFIG_MV_ETH_STAT_DIST
	pp->dist_stats.rx_dist = mvOsMalloc(sizeof(u32) * (pp->rxq_num * CONFIG_MV_ETH_RXQ_DESC + 1));
	if (pp->dist_stats.rx_dist != NULL) {
		pp->dist_stats.rx_dist_size = pp->rxq_num * CONFIG_MV_ETH_RXQ_DESC + 1;
		memset(pp->dist_stats.rx_dist, 0, sizeof(u32) * pp->dist_stats.rx_dist_size);
	} else
		pr_err("\to ethPort #%d: Can't allocate %d bytes for rx_dist\n",
		       pp->port, sizeof(u32) * (pp->rxq_num * CONFIG_MV_ETH_RXQ_DESC + 1));

	pp->dist_stats.tx_done_dist =
	    mvOsMalloc(sizeof(u32) * (pp->txp_num * CONFIG_MV_ETH_TXQ * CONFIG_MV_ETH_TXQ_DESC + 1));
	if (pp->dist_stats.tx_done_dist != NULL) {
		pp->dist_stats.tx_done_dist_size = pp->txp_num * CONFIG_MV_ETH_TXQ * CONFIG_MV_ETH_TXQ_DESC + 1;
		memset(pp->dist_stats.tx_done_dist, 0, sizeof(u32) * pp->dist_stats.tx_done_dist_size);
	} else
		pr_err("\to ethPort #%d: Can't allocate %d bytes for tx_done_dist\n",
		       pp->port, sizeof(u32) * (pp->txp_num * CONFIG_MV_ETH_TXQ * CONFIG_MV_ETH_TXQ_DESC + 1));
#endif /* CONFIG_MV_ETH_STAT_DIST */

	return 0;
}

/*
free the memory that allocate by
mv_eth_netdev_init
mv_eth_priv_init
mv_eth_hal_init
*/
static void mv_eth_priv_cleanup(struct eth_port *pp)
{
	int i, port;

	if (!pp)
		return;
	port = pp->port;

	mvOsFree(pp->rxq_ctrl);
	pp->rxq_ctrl = NULL;


	mvOsFree(pp->txq_ctrl);
	pp->txq_ctrl = NULL;

	mvPp2PortDestroy(pp->port);

	for (i = 0; i < CONFIG_NR_CPUS; i++)
		kfree(pp->cpu_config[i]);

	/* delete pool of external buffers for TSO, fragmentation, etc */
	if (mvStackDelete(pp->extArrStack))
		printk(KERN_ERR "Error: failed delete extArrStack for port #%d\n", port);

#ifdef CONFIG_MV_ETH_STAT_DIST
	mvOsFree(pp->dist_stats.rx_dist);
	mvOsFree(pp->dist_stats.tx_done_dist);
	mvOsFree(pp->dist_stats.tx_tso_dist);
#endif /* CONFIG_MV_ETH_STAT_DIST */

	/* allocate by mv_eth_netdev_init */
	/* free dev and pp*/
	synchronize_net();
	unregister_netdev(pp->dev);
	free_netdev(pp->dev);
	mv_eth_ports[port] = NULL;
}



/***********************************************************************************
 ***  print RX bm_pool status
 ***********************************************************************************/
void mv_eth_napi_groups_print(int port)
{
	int i;
	struct eth_port *pp = mv_eth_port_by_id(port);

	printk(KERN_CONT "NAPI groups:   cpu_mask   rxq_mask   napi_state\n");
	for (i = 0; i < MV_ETH_MAX_NAPI_GROUPS; i++) {
		if (!pp->napi_group[i])
			continue;
		printk(KERN_ERR "          %d:      0x%02x     0x%04x             %d\n",
			i, pp->napi_group[i]->cpu_mask, pp->napi_group[i]->rxq_mask,
			test_bit(NAPI_STATE_SCHED, &pp->napi_group[i]->napi->state));
	}

	printk(KERN_CONT "\n");
}

/***********************************************************************************
 ***  print RX bm_pool status
 ***********************************************************************************/
void mv_eth_pool_status_print(int pool)
{
	const char *type;
	struct bm_pool *bm_pool = &mv_eth_pool[pool];
	int buf_size, total_size, true_size;

	if (MV_ETH_BM_POOL_IS_HWF(bm_pool->type)) {
		buf_size = RX_HWF_BUF_SIZE(bm_pool->pkt_size);
		total_size = RX_HWF_TOTAL_SIZE(buf_size);
	} else {
		buf_size = RX_BUF_SIZE(bm_pool->pkt_size);
		total_size = RX_TOTAL_SIZE(buf_size);
	}
	true_size = RX_TRUE_SIZE(total_size);

	switch (bm_pool->type) {
	case MV_ETH_BM_FREE:
		type = "MV_ETH_BM_FREE";
		break;
	case MV_ETH_BM_SWF_LONG:
		type = "MV_ETH_BM_SWF_LONG";
		break;
	case MV_ETH_BM_SWF_SHORT:
		type = "MV_ETH_BM_SWF_SHORT";
		break;
	case MV_ETH_BM_HWF_LONG:
		type = "MV_ETH_BM_HWF_LONG";
		break;
	case MV_ETH_BM_HWF_SHORT:
		type = "MV_ETH_BM_HWF_SHORT";
		break;
	case MV_ETH_BM_MIXED_LONG:
		type = "MV_ETH_BM_MIXED_LONG";
		break;
	case MV_ETH_BM_MIXED_SHORT:
		type = "MV_ETH_BM_MIXED_SHORT";
		break;
	default:
		type = "Unknown";
	}

	pr_info("\nBM Pool #%d: pool type = %s, buffers num = %d\n", pool, type, bm_pool->buf_num);
	pr_info("     packet size = %d, buffer size = %d, total size = %d, true size = %d\n",
		bm_pool->pkt_size, buf_size, total_size, true_size);
	pr_info("     capacity=%d, buf_num=%d, port_map=0x%x, in_use=%u, in_use_thresh=%u\n",
		bm_pool->capacity, bm_pool->buf_num, bm_pool->port_map,
		mv_eth_bm_in_use_read(bm_pool), bm_pool->in_use_thresh);

#ifdef CONFIG_MV_ETH_STAT_ERR
	pr_cont("     skb_alloc_oom=%u", bm_pool->stats.skb_alloc_oom);
#endif /* #ifdef CONFIG_MV_ETH_STAT_ERR */

#ifdef CONFIG_MV_ETH_STAT_DBG
	pr_cont(", skb_alloc_ok=%u, bm_put=%u\n",
	       bm_pool->stats.skb_alloc_ok, bm_pool->stats.bm_put);

	pr_info("     no_recycle=%u, skb_recycled_ok=%u, skb_recycled_err=%u, bm_cookie_err=%u\n",
		bm_pool->stats.no_recycle, bm_pool->stats.skb_recycled_ok,
		bm_pool->stats.skb_recycled_err, bm_pool->stats.bm_cookie_err);
#endif /* CONFIG_MV_ETH_STAT_DBG */

	memset(&bm_pool->stats, 0, sizeof(bm_pool->stats));
}


/***********************************************************************************
 ***  print ext pool status
 ***********************************************************************************/
void mv_eth_ext_pool_print(struct eth_port *pp)
{
	printk(KERN_ERR "\nExt Pool Stack: bufSize = %u bytes\n", pp->extBufSize);
	mvStackStatus(pp->extArrStack, 0);
}

/***********************************************************************************
 ***  print net device status
 ***********************************************************************************/
void mv_eth_netdev_print(struct net_device *dev)
{
	printk(KERN_ERR "%s net_device status:\n\n", dev->name);
	printk(KERN_ERR "ifIdx=%d, mtu=%u, pkt_size=%d, buf_size=%d, MAC=" MV_MACQUAD_FMT "\n",
		dev->ifindex, dev->mtu, RX_PKT_SIZE(dev->mtu),
		(int)RX_BUF_SIZE(RX_PKT_SIZE(dev->mtu)), MV_MACQUAD(dev->dev_addr));

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 39)
	printk(KERN_ERR "features=0x%x, hw_features=0x%x, wanted_features=0x%x, vlan_features=0x%x\n",
			(unsigned int)(dev->features), (unsigned int)(dev->hw_features),
			(unsigned int)(dev->wanted_features), (unsigned int)(dev->vlan_features));
#else
	printk(KERN_ERR "features=0x%x, vlan_features=0x%x\n",
		 (unsigned int)(dev->features), (unsigned int)(dev->vlan_features));
#endif

	pr_info("flags=0x%x, gflags=0x%x: running=%d, oper_up=%d\n",
		(unsigned int)(dev->flags), (unsigned int)(dev->flags), netif_running(dev), netif_oper_up(dev));
	pr_info("uc_promisc=%d, promiscuity=%d, allmulti=%d\n", dev->uc_promisc, dev->promiscuity, dev->allmulti);

	if (mv_eth_netdev_find(dev->ifindex)) {
		struct eth_port *pp = MV_ETH_PRIV(dev);
		if (pp->tagged)
			mv_mux_netdev_print_all(pp->port);
	} else {
		/* Check if this is mux netdevice */
		if (mv_mux_netdev_find(dev->ifindex) != -1)
			mv_mux_netdev_print(dev);
	}
}

void mv_eth_status_print(void)
{
	printk(KERN_ERR "totals: ports=%d\n", mv_eth_ports_num);

#ifdef CONFIG_NET_SKB_RECYCLE
	pr_info("SKB recycle                  : %s\n", mv_ctrl_recycle ? "Enabled" : "Disabled");
#endif /* CONFIG_NET_SKB_RECYCLE */

#ifdef CONFIG_MV_ETH_SWF_HWF_CORRUPTION_WA
	pr_info("HWF + SWF data corruption WA : %s\n", mv_pp2_swf_hwf_wa_en ? "Enabled" : "Disabled");
#endif /* CONFIG_MV_ETH_SWF_HWF_CORRUPTION_WA */
}

/***********************************************************************************
 ***  print Ethernet port status
 ***********************************************************************************/
void mv_eth_port_status_print(unsigned int port)
{
	int txp, q;

	struct eth_port *pp = mv_eth_port_by_id(port);
	struct tx_queue *txq_ctrl;
	struct cpu_ctrl	*cpuCtrl;

	if (!pp)
		return;

	printk(KERN_ERR "\n");
	printk(KERN_ERR "port=%d, flags=0x%lx, rx_weight=%d\n", port, pp->flags, pp->weight);

	pr_info("RX next descriptor prefetch  : %s\n",
			pp->flags & MV_ETH_F_RX_DESC_PREFETCH ? "Enabled" : "Disabled");

	pr_info("RX packet header prefetch    : %s\n\n",
			pp->flags & MV_ETH_F_RX_PKT_PREFETCH ? "Enabled" : "Disabled");

	if (pp->flags & MV_ETH_F_CONNECT_LINUX)
		printk(KERN_ERR "%s: ", pp->dev->name);
	else
		printk(KERN_ERR "port %d: ", port);

	mv_eth_link_status_print(port);

	pr_cont("\n");
	pr_info("rxq_coal(pkts)[ q]         = ");
	for (q = 0; q < pp->rxq_num; q++)
		printk(KERN_CONT "%4d ", mvPp2RxqPktsCoalGet(port, q));

	pr_cont("\n");
	pr_info("rxq_coal(usec)[ q]         = ");
	for (q = 0; q < pp->rxq_num; q++)
		pr_cont("%4d ", mvPp2RxqTimeCoalGet(port, q));

	pr_cont("\n");
	pr_info("rxq_desc(num)[ q]          = ");
	for (q = 0; q < pp->rxq_num; q++)
		pr_info("%4d ", pp->rxq_ctrl[q].rxq_size);

	pr_cont("\n");
	for (txp = 0; txp < pp->txp_num; txp++) {
		pr_info("txq_coal(pkts)[%2d.q]       = ", txp);
		for (q = 0; q < CONFIG_MV_ETH_TXQ; q++)
			pr_cont("%4d ", mvPp2TxDonePktsCoalGet(port, txp, q));
		pr_cont("\n");

		pr_info("txq_desc(num) [%2d.q]       = ", txp);
		for (q = 0; q < CONFIG_MV_ETH_TXQ; q++) {
			txq_ctrl = &pp->txq_ctrl[txp * CONFIG_MV_ETH_TXQ + q];
			pr_cont("%4d ", txq_ctrl->txq_size);
		}
		pr_cont("\n");

		pr_info("txq_hwf_desc(num) [%2d.q]   = ", txp);
		for (q = 0; q < CONFIG_MV_ETH_TXQ; q++) {
			txq_ctrl = &pp->txq_ctrl[txp * CONFIG_MV_ETH_TXQ + q];
			pr_cont("%4d ", txq_ctrl->hwf_size);
		}
		pr_cont("\n");

		pr_info("txq_swf_desc(num) [%2d.q]   = ", txp);
		for (q = 0; q < CONFIG_MV_ETH_TXQ; q++) {
			txq_ctrl = &pp->txq_ctrl[txp * CONFIG_MV_ETH_TXQ + q];
			pr_cont("%4d ", txq_ctrl->txq_cpu[0].txq_size);
		}
		pr_info("txq_rsvd_chunk(num) [%2d.q] = ", txp);
		for (q = 0; q < CONFIG_MV_ETH_TXQ; q++) {
			txq_ctrl = &pp->txq_ctrl[txp * CONFIG_MV_ETH_TXQ + q];
			pr_cont("%4d ", txq_ctrl->rsvd_chunk);
		}
		pr_cont("\n");
	}
	pr_info("\n");

#ifdef CONFIG_MV_ETH_TXDONE_ISR
	printk(KERN_ERR "Do tx_done in NAPI context triggered by ISR\n");
	for (txp = 0; txp < pp->txp_num; txp++) {
		printk(KERN_ERR "txcoal(pkts)[%2d.q] = ", txp);
		for (q = 0; q < CONFIG_MV_ETH_TXQ; q++)
			printk(KERN_CONT "%3d ", mvPp2TxDonePktsCoalGet(port, txp, q));
		printk(KERN_CONT "\n");
	}
	printk(KERN_ERR "\n");
#else
	printk(KERN_ERR "Do tx_done in TX or Timer context: tx_done_threshold=%d\n", mv_ctrl_txdone);
#endif /* CONFIG_MV_ETH_TXDONE_ISR */

	printk(KERN_ERR "txp=%d, zero_pad=%s, mh_en=%s (0x%04x), hw_cmd: 0x%08x 0x%08x 0x%08x\n",
		pp->tx_spec.txp, (pp->tx_spec.flags & MV_ETH_TX_F_NO_PAD) ? "Disabled" : "Enabled",
		(pp->tx_spec.flags & MV_ETH_TX_F_MH) ? "Enabled" : "Disabled",
		MV_16BIT_BE(pp->tx_spec.tx_mh), pp->tx_spec.hw_cmd[0],
		pp->tx_spec.hw_cmd[1], pp->tx_spec.hw_cmd[2]);

	printk(KERN_CONT "\n");
	printk(KERN_CONT "CPU:   txq_def   napi_group   group_id\n");
	{
		int cpu;
		for_each_possible_cpu(cpu) {
			cpuCtrl = pp->cpu_config[cpu];
			if (MV_BIT_CHECK(pp->cpuMask, cpu))
				printk(KERN_ERR "  %d:   %3d        %p    %3d\n",
					cpu, cpuCtrl->txq, cpuCtrl->napi_group,
					(cpuCtrl->napi_group != NULL) ? cpuCtrl->napi_group->id : -1);
		}
	}

	printk(KERN_CONT "\n");

	mv_eth_napi_groups_print(port);

	/* Print status of all mux_dev for this port */
	if (pp->tagged) {
		printk(KERN_CONT "TAGGED PORT\n\n");
		mv_mux_netdev_print_all(port);
	} else
		printk(KERN_CONT "UNTAGGED PORT\n");
}


/***********************************************************************************
 ***  print port statistics
 ***********************************************************************************/

void mv_eth_port_stats_print(unsigned int port)
{
	struct eth_port *pp = mv_eth_port_by_id(port);
	struct port_stats *stat = NULL;
	struct tx_queue *txq_ctrl;
	struct txq_cpu_ctrl *txq_cpu_ptr;
	int txp, queue, cpu = smp_processor_id();

	pr_info("\n====================================================\n");
	pr_info("ethPort_%d: Statistics (running on cpu#%d)", port, cpu);
	pr_info("----------------------------------------------------\n\n");

	if (pp == NULL) {
		printk(KERN_ERR "eth_stats_print: wrong port number %d\n", port);
		return;
	}
	stat = &(pp->stats);

#ifdef CONFIG_MV_ETH_STAT_ERR
	pr_info("Errors:\n");
	pr_info("rx_error..................%10u\n", stat->rx_error);
	pr_info("tx_timeout................%10u\n", stat->tx_timeout);
	pr_info("ext_stack_empty...........%10u\n", stat->ext_stack_empty);
	pr_info("ext_stack_full............%10u\n", stat->ext_stack_full);
	pr_info("state_err.................%10u\n", stat->state_err);
#endif /* CONFIG_MV_ETH_STAT_ERR */

#ifdef CONFIG_MV_ETH_STAT_INF
	pr_info("\nEvents:\n");

	pr_info("irq[cpu]            = ");
	for_each_possible_cpu(cpu)
		printk(KERN_CONT "%8d ", stat->irq[cpu]);

	pr_info("irq_none[cpu]       = ");
	for_each_possible_cpu(cpu)
		printk(KERN_CONT "%8d ", stat->irq_err[cpu]);

	pr_info("poll[cpu]           = ");
	for_each_possible_cpu(cpu)
		printk(KERN_CONT "%8d ", stat->poll[cpu]);

	pr_info("poll_exit[cpu]      = ");
	for_each_possible_cpu(cpu)
		printk(KERN_CONT "%8d ", stat->poll_exit[cpu]);

	pr_info("tx_timer_event[cpu] = ");
	for_each_possible_cpu(cpu)
		printk(KERN_CONT "%8d ", stat->tx_done_timer_event[cpu]);

	pr_info("tx_timer_add[cpu]   = ");
	for_each_possible_cpu(cpu)
		printk(KERN_CONT "%8d ", stat->tx_done_timer_add[cpu]);

	pr_info("\n");
	pr_info("tx_done_event.............%10u\n", stat->tx_done);
	pr_info("link......................%10u\n", stat->link);
	pr_info("netdev_stop...............%10u\n", stat->netdev_stop);
	pr_info("rx_buf_hdr................%10u\n", stat->rx_buf_hdr);

#ifdef CONFIG_MV_ETH_RX_SPECIAL
	pr_info("rx_special................%10u\n", stat->rx_special);
#endif /* CONFIG_MV_ETH_RX_SPECIAL */

#ifdef CONFIG_MV_ETH_TX_SPECIAL
	pr_info("tx_special................%10u\n", stat->tx_special);
#endif /* CONFIG_MV_ETH_TX_SPECIAL */
#endif /* CONFIG_MV_ETH_STAT_INF */

#ifdef CONFIG_MV_ETH_STAT_DBG
	{
		__u32 total_rx_ok = 0;

		pr_info("\nDebug statistics:\n");
		printk(KERN_ERR "\n");
		printk(KERN_ERR "rx_gro....................%10u\n", stat->rx_gro);
		printk(KERN_ERR "rx_gro_bytes .............%10u\n", stat->rx_gro_bytes);

		printk(KERN_ERR "tx_tso....................%10u\n", stat->tx_tso);
		printk(KERN_ERR "tx_tso_bytes .............%10u\n", stat->tx_tso_bytes);
		printk(KERN_ERR "tx_tso_no_resource........%10u\n", stat->tx_tso_no_resource);

		printk(KERN_ERR "rx_netif..................%10u\n", stat->rx_netif);
		printk(KERN_ERR "rx_drop_sw................%10u\n", stat->rx_drop_sw);
		printk(KERN_ERR "rx_csum_hw................%10u\n", stat->rx_csum_hw);
		printk(KERN_ERR "rx_csum_sw................%10u\n", stat->rx_csum_sw);


		printk(KERN_ERR "tx_skb_free...............%10u\n", stat->tx_skb_free);
		printk(KERN_ERR "tx_sg.....................%10u\n", stat->tx_sg);
		printk(KERN_ERR "tx_csum_hw................%10u\n", stat->tx_csum_hw);
		printk(KERN_ERR "tx_csum_sw................%10u\n", stat->tx_csum_sw);

		printk(KERN_ERR "ext_stack_get.............%10u\n", stat->ext_stack_get);
		printk(KERN_ERR "ext_stack_put ............%10u\n", stat->ext_stack_put);

		printk(KERN_ERR "\n");

		printk(KERN_ERR "RXQ:       rx_ok\n\n");
		for (queue = 0; queue < pp->rxq_num; queue++) {
			u32 rxq_ok = 0;

			rxq_ok = stat->rxq[queue];

			pr_info("%3d:  %10u\n",	queue, rxq_ok);
			total_rx_ok += rxq_ok;
		}
		printk(KERN_ERR "SUM:  %10u\n", total_rx_ok);
	}
#endif /* CONFIG_MV_ETH_STAT_DBG */

	pr_info("\nAggregated TXQs statistics\n");
	pr_info("CPU:  count        send       no_resource\n\n");
	for_each_possible_cpu(cpu) {
		struct aggr_tx_queue *aggr_txq_ctrl = &aggr_txqs[cpu];
		u32 txq_tx = 0, txq_err = 0;

#ifdef CONFIG_MV_ETH_STAT_DBG
		txq_tx = aggr_txq_ctrl->stats.txq_tx;
#endif /* CONFIG_MV_ETH_STAT_DBG */
#ifdef CONFIG_MV_ETH_STAT_ERR
		txq_err = aggr_txq_ctrl->stats.txq_err;
#endif /* CONFIG_MV_ETH_STAT_ERR */

		pr_info(" %d:    %3d   %10u    %10u\n",
		       cpu, aggr_txq_ctrl->txq_count, txq_tx, txq_err);

		memset(&aggr_txq_ctrl->stats, 0, sizeof(aggr_txq_ctrl->stats));
	}

	pr_info("\n");
	pr_info("TXP-TXQ:  count  res_num      send          done     no_resource      res_req      res_total\n\n");

	for (txp = 0; txp < pp->txp_num; txp++) {
		for (queue = 0; queue < CONFIG_MV_ETH_TXQ; queue++)
			for_each_possible_cpu(cpu) {
				u32 txq_tx = 0, txq_done = 0, txq_reserved_req = 0, txq_reserved_total = 0, txq_err = 0;

				txq_ctrl = &pp->txq_ctrl[txp * CONFIG_MV_ETH_TXQ + queue];
				txq_cpu_ptr = &txq_ctrl->txq_cpu[cpu];
#ifdef CONFIG_MV_ETH_STAT_DBG
				txq_tx = txq_cpu_ptr->stats.txq_tx;
				txq_done = txq_cpu_ptr->stats.txq_txdone;
				txq_reserved_req = txq_cpu_ptr->stats.txq_reserved_req;
				txq_reserved_total = txq_cpu_ptr->stats.txq_reserved_total;

#endif /* CONFIG_MV_ETH_STAT_DBG */
#ifdef CONFIG_MV_ETH_STAT_ERR
				txq_err = txq_cpu_ptr->stats.txq_err;
#endif /* CONFIG_MV_ETH_STAT_ERR */

				pr_info("%d-%d-cpu#%d: %3d    %3d   %10u    %10u    %10u    %10u    %10u\n",
				       txp, queue, cpu, txq_cpu_ptr->txq_count, txq_cpu_ptr->reserved_num,
				       txq_tx, txq_done, txq_err, txq_reserved_req, txq_reserved_total);

				memset(&txq_cpu_ptr->stats, 0, sizeof(txq_cpu_ptr->stats));
			}
	}
	memset(stat, 0, sizeof(struct port_stats));

	mv_eth_ext_pool_print(pp);

	/* RX pool statistics */
	if (pp->pool_short)
		mv_eth_pool_status_print(pp->pool_short->pool);

	if (pp->pool_long)
		mv_eth_pool_status_print(pp->pool_long->pool);

#ifdef CONFIG_MV_ETH_STAT_DIST
	{
		int i;
		struct dist_stats *dist_stats = &(pp->dist_stats);

		if (dist_stats->rx_dist) {
			printk(KERN_ERR "\n      Linux Path RX distribution\n");
			for (i = 0; i < dist_stats->rx_dist_size; i++) {
				if (dist_stats->rx_dist[i] != 0) {
					printk(KERN_ERR "%3d RxPkts - %u times\n", i, dist_stats->rx_dist[i]);
					dist_stats->rx_dist[i] = 0;
				}
			}
		}

		if (dist_stats->tx_done_dist) {
			printk(KERN_ERR "\n      tx-done distribution\n");
			for (i = 0; i < dist_stats->tx_done_dist_size; i++) {
				if (dist_stats->tx_done_dist[i] != 0) {
					printk(KERN_ERR "%3d TxDoneDesc - %u times\n", i, dist_stats->tx_done_dist[i]);
					dist_stats->tx_done_dist[i] = 0;
				}
			}
		}
#ifdef CONFIG_MV_ETH_TSO
		if (dist_stats->tx_tso_dist) {
			printk(KERN_ERR "\n      TSO stats\n");
			for (i = 0; i < dist_stats->tx_tso_dist_size; i++) {
				if (dist_stats->tx_tso_dist[i] != 0) {
					printk(KERN_ERR "%3d KBytes - %u times\n", i, dist_stats->tx_tso_dist[i]);
					dist_stats->tx_tso_dist[i] = 0;
				}
			}
		}
#endif /* CONFIG_MV_ETH_TSO */
	}
#endif /* CONFIG_MV_ETH_STAT_DIST */
}
/* mv_eth_tx_cleanup - reset and delete all tx queues */
static void mv_eth_tx_cleanup(struct eth_port *pp)
{
	int txp, txq;
	struct tx_queue *txq_ctrl;

	if (!pp)
		return;

	/* Reset Tx ports */
	for (txp = 0; txp < pp->txp_num; txp++) {
		if (mv_eth_txp_clean(pp->port, txp))
			printk(KERN_ERR "Warning: Port %d Tx port %d reset failed\n", pp->port, txp);
	}

	/* Delete Tx queues */
	for (txp = 0; txp < pp->txp_num; txp++) {
		for (txq = 0; txq < CONFIG_MV_ETH_TXQ; txq++) {
			txq_ctrl = &pp->txq_ctrl[txp * CONFIG_MV_ETH_TXQ + txq];
			if (txq_ctrl->q)
				mv_eth_txq_delete(pp, txq_ctrl);
		}
	}
}

/* mv_eth_rx_cleanup - reset and delete all rx queues */
static void mv_eth_rx_cleanup(struct eth_port *pp)
{
	int rxq, prxq;
	struct rx_queue *rxq_ctrl;

	if (!pp)
		return;

	/* Reset RX ports */
	if (mv_eth_rx_reset(pp->port))
		printk(KERN_ERR "%s Warning: Rx port %d reset failed\n", __func__, pp->port);

	/* Delete Rx queues */
	/* TODO - delete rxq only if port was in up at least once */
	for (rxq = 0; rxq < pp->rxq_num; rxq++) {
		rxq_ctrl = &pp->rxq_ctrl[rxq];

		/* port start called before*/
		if (rxq_ctrl->q)
			mvPp2RxqDelete(pp->port, rxq);

		prxq = mvPp2LogicRxqToPhysRxq(pp->port, rxq);
		mvPp2PhysRxqMapDel(prxq);
		rxq_ctrl->q = NULL;
	}

}


/* mv_eth_pool_cleanup - delete all ports buffers from pool */
static void mv_eth_pool_cleanup(int port, struct bm_pool *ppool)
{
	if (!ppool)
		return;

	ppool->port_map &= ~(1 << port);

	if (ppool->port_map == 0) {
		mv_eth_pool_free(ppool->pool, ppool->buf_num);
		ppool->type = MV_ETH_BM_FREE;
	}
}

static void mv_eth_napi_cleanup(struct eth_port *pp)
{
	int i;
	struct napi_group_ctrl *napi_group;

	if (!pp)
		return;

	if (!(pp->flags & MV_ETH_F_CONNECT_LINUX))
		return;

	for (i = 0; i < MV_ETH_MAX_NAPI_GROUPS; i++) {
		napi_group = pp->napi_group[i];
		if (napi_group) {
			netif_napi_del(napi_group->napi);
			mvOsFree(napi_group->napi);
			mvOsFree(napi_group);
			pp->napi_group[i] = NULL;
		}
	}
}

static int mv_eth_port_cleanup(int port)
{
	struct eth_port *pp;
	pp = mv_eth_port_by_id(port);

	if (pp == NULL) {
		printk(KERN_ERR "port %d already clean\n", port);
		return 0;
	}

	if (pp->flags & MV_ETH_F_STARTED) {
		printk(KERN_ERR "%s: port %d is started, cannot cleanup\n", __func__, port);
		return -1;
	}

	mv_eth_tx_cleanup(pp);
	mv_eth_rx_cleanup(pp);

	/*pools cleanup*/
	mv_eth_pool_cleanup(port, pp->pool_long);
	mv_eth_pool_cleanup(port, pp->pool_short);
	mv_eth_pool_cleanup(port, pp->hwf_pool_long);
	mv_eth_pool_cleanup(port, pp->hwf_pool_short);

	/* Clear Marvell Header related modes - will be set again if needed on re-init */
	mvPp2MhSet(port, MV_TAG_TYPE_NONE);

	/* Clear any forced link, speed and duplex */
	mv_eth_port_link_speed_fc(port, MV_ETH_SPEED_AN, 0);

	mv_eth_napi_cleanup(pp);

	if (pp->tagged)
		mv_mux_eth_detach(pp->port);

	mv_eth_priv_cleanup(pp);

	printk(KERN_ERR "port %d cleanup done\n", port);

	return 0;
}

int mv_eth_all_ports_cleanup(void)
{
	int port, status;

	for (port = 0; port < mv_eth_ports_num; port++) {
		status = mv_eth_port_cleanup(port);
		if (status != 0) {
			printk(KERN_ERR "%s :port %d, cleanup failed, stopping all ports cleanup\n", __func__, port);
			return status;
		}
	}

	if (mv_eth_initialized)
		mv_eth_shared_cleanup();

	return MV_OK;

}

int mv_eth_all_ports_probe(void)
{
	int port = 0;

	for (port = 0; port < mv_eth_ports_num; port++)
		if (mv_eth_probe(plats[port]))
			return 1;
	return 0;
}

#ifdef CONFIG_MV_INCLUDE_PON
/* Used by PON module */
struct mv_netdev_notify_ops mv_netdev_callbacks;

/* Used by netdev driver */
struct mv_eth_ext_mac_ops *mv_pon_callbacks;

void pon_link_status_notify(int port_id, MV_BOOL link_state)
{
	struct eth_port *pon_port = mv_eth_port_by_id(MV_PON_LOGIC_PORT_GET());
	mv_eth_link_event(pon_port, 1);
}

/* called by PON module */
void mv_eth_ext_mac_ops_register(int port_id,
		struct mv_eth_ext_mac_ops **ext_mac_ops, struct mv_netdev_notify_ops **netdev_ops)
{
	if (*netdev_ops == NULL) {
		pr_err("%s: netdev_ops is uninitialized\n", __func__);
		return;
	}

	mv_netdev_callbacks.link_notify = pon_link_status_notify;
	*netdev_ops = &mv_netdev_callbacks;

	if (*ext_mac_ops == NULL) {
		pr_err("%s: netdev_ops is uninitialized\n", __func__);
		return;
	}

	mv_pon_callbacks = *ext_mac_ops;
}

MV_BOOL mv_pon_link_status(MV_ETH_PORT_STATUS *link)
{
	MV_BOOL linkup = MV_TRUE;

	if (mv_pon_callbacks && mv_pon_callbacks->link_status_get)
		linkup = mv_pon_callbacks->link_status_get(MV_PON_LOGIC_PORT_GET());

	if (link) {
		link->linkup = linkup;
		link->speed = MV_ETH_SPEED_1000;
		link->duplex = MV_ETH_DUPLEX_FULL;
	}

	return linkup;
}

MV_STATUS mv_pon_mtu_config(MV_U32 maxEth)
{
	if (mv_pon_callbacks && mv_pon_callbacks->max_pkt_size_set) {
		if (mv_pon_callbacks->max_pkt_size_set(MV_PON_LOGIC_PORT_GET(), maxEth) != MV_OK) {
			printk(KERN_ERR "pon max_pkt_size_set failed\n");
			return MV_FAIL;
		}
	} else {
		printk(KERN_ERR "pon max_pkt_size_set is uninitialized\n");
		return MV_FAIL;
	}

	return MV_OK;
}

MV_STATUS mv_pon_set_mac_addr(void *addr)
{
	if (mv_pon_callbacks && mv_pon_callbacks->mac_addr_set) {
		if (mv_pon_callbacks->mac_addr_set(MV_PON_LOGIC_PORT_GET(), addr) != MV_OK) {
			printk(KERN_ERR "pon mac_addr_set failed\n");
			return MV_FAIL;
		}
	} else {
		printk(KERN_ERR "pon mac_addr_set is uninitialized\n");
		return MV_FAIL;
	}

	return MV_OK;
}

MV_STATUS mv_pon_enable(void)
{
	if (mv_pon_callbacks && mv_pon_callbacks->port_enable) {
		if (mv_pon_callbacks->port_enable(MV_PON_LOGIC_PORT_GET()) != MV_OK) {
			printk(KERN_ERR "pon port_enable failed\n");
			return MV_FAIL;
		}
	} else
		printk(KERN_ERR "Warning: pon port_enable is uninitialized\n");

	if (mv_pon_link_status(NULL) == MV_TRUE)
		return mvPp2PortEgressEnable(MV_PON_LOGIC_PORT_GET(), MV_TRUE);

	return MV_NOT_READY;
}

MV_STATUS mv_pon_disable(void)
{
	mvPp2PortEgressEnable(MV_PON_LOGIC_PORT_GET(), MV_FALSE);
	if (mv_pon_callbacks && mv_pon_callbacks->port_disable) {
		if (mv_pon_callbacks->port_disable(MV_PON_LOGIC_PORT_GET()) != MV_OK) {
			printk(KERN_ERR "pon port_disable failed\n");
			return MV_FAIL;
		}
	} else
		printk(KERN_ERR "Warning: pon port_disable is uninitialized\n");	return MV_OK;

	return MV_OK;
}
#endif /* CONFIG_MV_INCLUDE_PON */

/* Support for platform driver */

#ifdef CONFIG_CPU_IDLE


int mv_eth_suspend_clock(int port)
{
/* TBD */
	return 0;
}


int mv_eth_suspend(struct platform_device *pdev, pm_message_t state)
{
/* TBD */
	return 0;
}


int mv_eth_resume_clock(int port)
{
/* TBD */
	return 0;
}


int mv_eth_resume(struct platform_device *pdev)
{
/* TBD */
	return 0;

}

#endif	/* CONFIG_CPU_IDLE */

static int mv_eth_remove(struct platform_device *pdev)
{
#ifdef CONFIG_NETMAP
	int port = pdev->id;
	struct eth_port *pp = mv_eth_port_by_id(port);
#endif
	printk(KERN_INFO "Removing Marvell Ethernet Driver\n");
	mv_eth_sysfs_exit();
#ifdef CONFIG_NETMAP
	if (pp->flags & MV_ETH_F_IFCAP_NETMAP)
		netmap_detach(pp->dev);
#endif /* CONFIG_NETMAP */
	return 0;
}

static void mv_eth_shutdown(struct platform_device *pdev)
{
	printk(KERN_INFO "Shutting Down Marvell Ethernet Driver\n");
}

static struct platform_driver mv_eth_driver = {
	.probe = mv_eth_probe,
	.remove = mv_eth_remove,
	.shutdown = mv_eth_shutdown,
#ifdef CONFIG_CPU_IDLE
	.suspend = mv_eth_suspend,
	.resume = mv_eth_resume,
#endif /* CONFIG_CPU_IDLE */
	.driver = {
		.name = MV_PP2_PORT_NAME,
	},
};

static int __init mv_eth_init_module(void)
{
	return platform_driver_register(&mv_eth_driver);
}
module_init(mv_eth_init_module);

static void __exit mv_eth_cleanup_module(void)
{
	platform_driver_unregister(&mv_eth_driver);
}
module_exit(mv_eth_cleanup_module);


MODULE_DESCRIPTION("Marvell Ethernet Driver - www.marvell.com");
MODULE_AUTHOR("Dmitri Epshtein <dima@marvell.com>");
MODULE_LICENSE("GPL");

