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
#ifndef __mv_netdev_h__
#define __mv_netdev_h__

#include <linux/netdevice.h>
#include <linux/etherdevice.h>
#include <linux/skbuff.h>
#include <linux/mv_pp2.h>
#include <net/ip.h>

#include "mvCommon.h"
#include "mvOs.h"
#include "mv802_3.h"
#include "mvStack.h"

#include "gbe/mvPp2Gbe.h"
#include "bm/mvBmRegs.h"
#include "bm/mvBm.h"

#ifndef CONFIG_MV_PON_TCONTS
# define CONFIG_MV_PON_TCONTS 16
#endif

/******************************************************
 * driver statistics control --                       *
 ******************************************************/
#ifdef CONFIG_MV_ETH_STAT_ERR
#define STAT_ERR(c) c
#else
#define STAT_ERR(c)
#endif

#ifdef CONFIG_MV_ETH_STAT_INF
#define STAT_INFO(c) c
#else
#define STAT_INFO(c)
#endif

#ifdef CONFIG_MV_ETH_STAT_DBG
#define STAT_DBG(c) c
#else
#define STAT_DBG(c)
#endif

#ifdef CONFIG_MV_ETH_STAT_DIST
#define STAT_DIST(c) c
#else
#define STAT_DIST(c)
#endif

extern int mv_ctrl_txdone;

/****************************************************************************
 * Rx buffer size: MTU + 2(Marvell Header) + 4(VLAN) + 14(MAC hdr) + 4(CRC) *
 ****************************************************************************/
#define MV_ETH_SKB_SHINFO_SIZE		SKB_DATA_ALIGN(sizeof(struct skb_shared_info))

#define RX_PKT_SIZE(mtu) \
		MV_ALIGN_UP((mtu) + 2 + 4 + ETH_HLEN + 4, CPU_D_CACHE_LINE_SIZE)

#define RX_BUF_SIZE(pkt_size)		((pkt_size) + NET_SKB_PAD)
#define RX_TOTAL_SIZE(buf_size)		((buf_size) + MV_ETH_SKB_SHINFO_SIZE)
#define RX_MAX_PKT_SIZE(total_size)	((total_size) - NET_SKB_PAD - MV_ETH_SKB_SHINFO_SIZE)

#define RX_HWF_PKT_OFFS			32
#define RX_HWF_BUF_SIZE(pkt_size)	((pkt_size) + RX_HWF_PKT_OFFS)
#define RX_HWF_TOTAL_SIZE(buf_size)	(buf_size)
#define RX_HWF_MAX_PKT_SIZE(total_size)	((total_size) - RX_HWF_PKT_OFFS)

#define RX_TRUE_SIZE(total_size)	roundup_pow_of_two(total_size)

#ifdef CONFIG_NET_SKB_RECYCLE
extern int mv_ctrl_recycle;

#define mv_eth_is_recycle()     (mv_ctrl_recycle)
int mv_eth_skb_recycle(struct sk_buff *skb);
#else
#define mv_eth_is_recycle()     0
#endif /* CONFIG_NET_SKB_RECYCLE */



/******************************************************
 * interrupt control --                               *
 ******************************************************/
#define MV_ETH_TRYLOCK(lock, flags)                           \
	(in_interrupt() ? spin_trylock((lock)) :              \
		spin_trylock_irqsave((lock), (flags)))

#define MV_ETH_LOCK(lock, flags)                              \
{                                                             \
	if (in_interrupt())                                   \
		spin_lock((lock));                            \
	else                                                  \
		spin_lock_irqsave((lock), (flags));           \
}

#define MV_ETH_UNLOCK(lock, flags)                            \
{                                                             \
	if (in_interrupt())                                   \
		spin_unlock((lock));                          \
	else                                                  \
		spin_unlock_irqrestore((lock), (flags));      \
}

#define MV_ETH_LIGHT_LOCK(flags)                              \
	if (!in_interrupt())                                  \
		local_irq_save(flags);

#define MV_ETH_LIGHT_UNLOCK(flags)	                      \
	if (!in_interrupt())                                  \
		local_irq_restore(flags);

/******************************************************
 * rx / tx queues --                                  *
 ******************************************************/
/*
 * Debug statistics
 */

struct txq_stats {
#ifdef CONFIG_MV_ETH_STAT_ERR
	u32 txq_err;
#endif /* CONFIG_MV_ETH_STAT_ERR */
#ifdef CONFIG_MV_ETH_STAT_DBG
	u32 txq_tx;
	u32 txq_txreq; /*request reserved tx descriptors*/
	u32 txq_txdone;
#endif /* CONFIG_MV_ETH_STAT_DBG */
};

struct port_stats {

#ifdef CONFIG_MV_ETH_STAT_ERR
	u32 rx_error;
	u32 tx_timeout;
	u32 ext_stack_empty;
	u32 ext_stack_full;
	u32 state_err;
#endif /* CONFIG_MV_ETH_STAT_ERR */

#ifdef CONFIG_MV_ETH_STAT_INF
	u32 irq[CONFIG_NR_CPUS];
	u32 irq_err[CONFIG_NR_CPUS];
	u32 poll[CONFIG_NR_CPUS];
	u32 poll_exit[CONFIG_NR_CPUS];
	u32 tx_done_timer_event[CONFIG_NR_CPUS];
	u32 tx_done_timer_add[CONFIG_NR_CPUS];
	u32 tx_done;
	u32 link;
	u32 netdev_stop;
	u32 rx_buf_hdr;

#ifdef CONFIG_MV_ETH_RX_SPECIAL
	u32 rx_special;
#endif /* CONFIG_MV_ETH_RX_SPECIAL */

#ifdef CONFIG_MV_ETH_TX_SPECIAL
	u32 tx_special;
#endif /* CONFIG_MV_ETH_TX_SPECIAL */

#endif /* CONFIG_MV_ETH_STAT_INF */

#ifdef CONFIG_MV_ETH_STAT_DBG
	u32 rxq[CONFIG_MV_ETH_RXQ];
	u32 rx_tagged;
	u32 rx_netif;
	u32 rx_gro;
	u32 rx_gro_bytes;
	u32 rx_drop_sw;
	u32 rx_csum_hw;
	u32 rx_csum_sw;
	u32 tx_csum_hw;
	u32 tx_csum_sw;
	u32 tx_skb_free;
	u32 tx_sg;
	u32 tx_tso;
	u32 tx_tso_no_resource;
	u32 tx_tso_bytes;
	u32 ext_stack_put;
	u32 ext_stack_get;
#endif /* CONFIG_MV_ETH_STAT_DBG */
};

#define MV_ETH_TX_DESC_ALIGN		0x1f

/* Used for define type of data saved in shadow: SKB or extended buffer or nothing */
#define MV_ETH_SHADOW_SKB		0x1
#define MV_ETH_SHADOW_EXT		0x2

/* Masks used for pp->flags */
#define MV_ETH_F_STARTED_BIT            0
#define MV_ETH_F_RX_DESC_PREFETCH_BIT   1
#define MV_ETH_F_RX_PKT_PREFETCH_BIT    2
#define MV_ETH_F_CONNECT_LINUX_BIT      5 /* port is connected to Linux netdevice */
#define MV_ETH_F_LINK_UP_BIT            6
#define MV_ETH_F_IFCAP_NETMAP_BIT       15

#define MV_ETH_F_STARTED                (1 << MV_ETH_F_STARTED_BIT)
#define MV_ETH_F_RX_DESC_PREFETCH       (1 << MV_ETH_F_RX_DESC_PREFETCH_BIT)
#define MV_ETH_F_RX_PKT_PREFETCH        (1 << MV_ETH_F_RX_PKT_PREFETCH_BIT)
#define MV_ETH_F_CONNECT_LINUX          (1 << MV_ETH_F_CONNECT_LINUX_BIT)
#define MV_ETH_F_LINK_UP                (1 << MV_ETH_F_LINK_UP_BIT)
#define MV_ETH_F_IFCAP_NETMAP           (1 << MV_ETH_F_IFCAP_NETMAP_BIT)

#ifdef CONFIG_MV_ETH_DEBUG_CODE
/* Masks used for pp->dbg_flags */
#define MV_ETH_F_DBG_RX_BIT         0
#define MV_ETH_F_DBG_TX_BIT         1
#define MV_ETH_F_DBG_DUMP_BIT       2
#define MV_ETH_F_DBG_ISR_BIT        3
#define MV_ETH_F_DBG_POLL_BIT       4
#define MV_ETH_F_DBG_BUFF_HDR_BIT   5

#define MV_ETH_F_DBG_RX            (1 << MV_ETH_F_DBG_RX_BIT)
#define MV_ETH_F_DBG_TX            (1 << MV_ETH_F_DBG_TX_BIT)
#define MV_ETH_F_DBG_DUMP          (1 << MV_ETH_F_DBG_DUMP_BIT)
#define MV_ETH_F_DBG_ISR           (1 << MV_ETH_F_DBG_ISR_BIT)
#define MV_ETH_F_DBG_POLL          (1 << MV_ETH_F_DBG_POLL_BIT)
#define MV_ETH_F_DBG_BUFF_HDR      (1 << MV_ETH_F_DBG_BUFF_HDR_BIT)
#endif /* CONFIG_MV_ETH_DEBUG_CODE */

/* Masks used for cpu_ctrl->flags */
#define MV_ETH_F_TX_DONE_TIMER_BIT  0

#define MV_ETH_F_TX_DONE_TIMER		(1 << MV_ETH_F_TX_DONE_TIMER_BIT)	/* 0x01 */


#define MV_ETH_TXQ_INVALID	0xFF

#define TOS_TO_DSCP(tos)	((tos >> 2) & 0x3F)

/* Used in PPv2.1 */
#define MV_ETH_CPU_DESC_CHUNK	20

/* Masks used for tx_spec->flags */
#define MV_ETH_TX_F_NO_PAD	0x0001
#define MV_ETH_TX_F_MH		0x0002
#define MV_ETH_TX_F_HW_CMD	0x0004

struct mv_eth_tx_spec {
	unsigned long	flags;
	u32		hw_cmd[3];     /* tx_desc offset = 0x10, 0x14, 0x18 */
	u16		tx_mh;
	u8		txp;
	u8		txq;
#ifdef CONFIG_MV_ETH_TX_SPECIAL
	void		(*tx_func) (u8 *data, int size, struct mv_eth_tx_spec *tx_spec);
#endif
};

struct txq_cpu_ctrl {
	int			txq_size;
	int			txq_count;
	int			reserved_num; /* PPv2.1 (MAS 3.16)- number of reserved descriptors for this CPU */
	u32			*shadow_txq; /* can be MV_ETH_PKT* or struct skbuf* */
	int			shadow_txq_put_i;
	int			shadow_txq_get_i;
	struct txq_stats	stats;
};

struct tx_queue {
	MV_PP2_PHYS_TXQ_CTRL	*q;
	u8			txp;
	u8			txq;
	int			txq_size;
	int			hwf_size;
	int			swf_size;
	int			rsvd_chunk;
	struct txq_cpu_ctrl	txq_cpu[CONFIG_NR_CPUS];
	spinlock_t		queue_lock;
	MV_U32			txq_done_pkts_coal;
	unsigned long		flags;
};

struct aggr_tx_queue {
	MV_PP2_AGGR_TXQ_CTRL	*q;
	int			txq_size;
	int			txq_count;
	struct txq_stats	stats;
};

struct rx_queue {
	MV_PP2_PHYS_RXQ_CTRL	*q;
	int			rxq_size;
	MV_U32			rxq_pkts_coal;
	MV_U32			rxq_time_coal;
};

struct dist_stats {
	u32	*rx_dist;
	int	rx_dist_size;
	u32	*tx_done_dist;
	int	tx_done_dist_size;
	u32	*tx_tso_dist;
	int	tx_tso_dist_size;
};

struct napi_group_ctrl {
	int			id;
	MV_U8			cpu_mask;
	MV_U16			rxq_mask;
	MV_U32			cause_rx_tx;
	struct napi_struct	*napi;
};

struct cpu_ctrl {
	struct eth_port		*pp;
	struct napi_group_ctrl	*napi_group;
	int			txq;
	int			cpu;
	struct timer_list	tx_done_timer;
	unsigned long		flags;
};

struct eth_port {
	int			port;
	struct mv_pp2_pdata	*plat_data;
	bool			tagged; /* NONE/MH/DSA/EDSA/VLAN */
	MV_PP2_PORT_CTRL	*port_ctrl;
	struct rx_queue		*rxq_ctrl;
	struct tx_queue		*txq_ctrl;
	int			txp_num;
	int			first_rxq;
	int			rxq_num;
	struct net_device	*dev;
	rwlock_t		rwlock;
	struct bm_pool		*pool_long;
	struct bm_pool		*pool_short;
	struct bm_pool		*hwf_pool_long;
	struct bm_pool		*hwf_pool_short;
	struct napi_group_ctrl	*napi_group[MV_ETH_MAX_RXQ];
	unsigned long		flags; /* MH, TIMER, etc. */
	u8			dbg_flags;
	struct mv_eth_tx_spec	tx_spec;
	struct port_stats	stats;
	struct dist_stats	dist_stats;
	int			weight;
	MV_STACK		*extArrStack;
	int			extBufSize;
	spinlock_t		extLock;
	MV_U8			txq_dscp_map[64];
	/* Ethtool parameters */
	__u16			speed_cfg;
	__u8			duplex_cfg;
	__u8			autoneg_cfg;
	__u16			advertise_cfg;
	__u32			rx_time_coal_cfg;
	__u32			rx_pkts_coal_cfg;
	__u32			tx_pkts_coal_cfg;
	__u32			rx_time_low_coal_cfg;
	__u32			rx_time_high_coal_cfg;
	__u32			rx_pkts_low_coal_cfg;
	__u32			rx_pkts_high_coal_cfg;
	__u32			pkt_rate_low_cfg;
	__u32			pkt_rate_high_cfg;
	__u32			rate_current; /* unknown (0), low (1), normal (2), high (3) */
	__u32			rate_sample_cfg;
	__u32			rx_adaptive_coal_cfg;
	/* Rate calculate */
	unsigned long		rx_rate_pkts;
	unsigned long		rx_timestamp;
#ifdef CONFIG_MV_ETH_RX_SPECIAL
	void			(*rx_special_proc)(int port, int rxq, struct net_device *dev,
						struct sk_buff *skb, struct pp2_rx_desc *rx_desc);
#endif /* CONFIG_MV_ETH_RX_SPECIAL */
#ifdef CONFIG_MV_ETH_TX_SPECIAL
	int			(*tx_special_check)(int port, struct net_device *dev, struct sk_buff *skb,
						struct mv_eth_tx_spec *tx_spec_out);
#endif /* CONFIG_MV_ETH_TX_SPECIAL */
	MV_U32			cpuMask;
	MV_U32			rx_indir_table[256];
	struct cpu_ctrl		*cpu_config[CONFIG_NR_CPUS];
	MV_U32			sgmii_serdes;
	int			pm_mode;
};

enum eth_pm_mode {
	MV_ETH_PM_WOL = 0,
	MV_ETH_PM_CLOCK,
	MV_ETH_PM_DISABLE,
	MV_ETH_PM_LAST
};

#define MV_ETH_PRIV(dev)	((struct eth_port *)(netdev_priv(dev)))
#define MV_DEV_STAT(dev)	(&((dev)->stats))

/* BM specific defines */
struct pool_stats {
#ifdef CONFIG_MV_ETH_STAT_ERR
	u32 skb_alloc_oom;
	u32 stack_empty;
	u32 stack_full;
#endif /* CONFIG_MV_ETH_STAT_ERR */

#ifdef CONFIG_MV_ETH_STAT_DBG
	u32 no_recycle;
	u32 bm_put;
	u32 stack_put;
	u32 stack_get;
	u32 skb_alloc_ok;
	u32 skb_recycled_ok;
	u32 skb_recycled_err;
	u32 bm_cookie_err;
#endif /* CONFIG_MV_ETH_STAT_DBG */
};

/* BM pool assignment */
#ifdef CONFIG_MV_ETH_BM_PER_PORT_MODE
/* #port   SWF long   SWF short   HWF long   HWF short *
 *   0         0          1           0           1    *
 *   1         2          3           2           3    *
 *   2         4          5           4           5    *
 *   3         6          7           6           7    */
#define MV_ETH_BM_SWF_LONG_POOL(port)		(port << 1)
#define MV_ETH_BM_SWF_SHORT_POOL(port)		((port << 1) + 1)
#define MV_ETH_BM_HWF_LONG_POOL(port)		(MV_ETH_BM_SWF_LONG_POOL(port))
#define MV_ETH_BM_HWF_SHORT_POOL(port)		(MV_ETH_BM_SWF_SHORT_POOL(port))
#else /* CONFIG_MV_ETH_BM_SWF_HWF_MODE */
/* #port   SWF long   SWF short   HWF long   HWF short *
 *   0         0          3           4           7    *
 *   1         1          3           5           7    *
 *   2         2          3           6           7    *
 *   3         2          3           6           7    */
#define MV_ETH_BM_SWF_LONG_POOL(port)		((port > 2) ? 2 : port)
#define MV_ETH_BM_SWF_SHORT_POOL(port)		(3)
#define MV_ETH_BM_HWF_LONG_POOL(port)		((port > 2) ? 6 : (port + 4))
#define MV_ETH_BM_HWF_SHORT_POOL(port)		(7)
#endif

#define MV_ETH_BM_POOLS		MV_BM_POOLS
#define mv_eth_pool_bm(p)	(p->bm_pool)

enum mv_eth_bm_type {
	MV_ETH_BM_FREE,		/* BM pool is not being used by any port		   */
	MV_ETH_BM_SWF_LONG,	/* BM pool is being used by SWF as long pool		   */
	MV_ETH_BM_SWF_SHORT,	/* BM pool is being used by SWF as short pool		   */
	MV_ETH_BM_HWF_LONG,	/* BM pool is being used by HWF as long pool		   */
	MV_ETH_BM_HWF_SHORT,	/* BM pool is being used by HWF as short pool		   */
	MV_ETH_BM_MIXED_LONG,	/* BM pool is being used by both HWF and SWF as long pool  */
	MV_ETH_BM_MIXED_SHORT	/* BM pool is being used by both HWF and SWF as short pool */
};

/* Macros for using mv_eth_bm_type */
#define MV_ETH_BM_POOL_IS_HWF(type)	((type == MV_ETH_BM_HWF_LONG) || (type == MV_ETH_BM_HWF_SHORT))
#define MV_ETH_BM_POOL_IS_SWF(type)	((type == MV_ETH_BM_SWF_LONG) || (type == MV_ETH_BM_SWF_SHORT))
#define MV_ETH_BM_POOL_IS_MIXED(type)	((type == MV_ETH_BM_MIXED_LONG) || (type == MV_ETH_BM_MIXED_SHORT))
#define MV_ETH_BM_POOL_IS_SHORT(type)	((type == MV_ETH_BM_SWF_SHORT) || (type == MV_ETH_BM_HWF_SHORT)\
									|| (type == MV_ETH_BM_MIXED_SHORT))
#define MV_ETH_BM_POOL_IS_LONG(type)	((type == MV_ETH_BM_SWF_LONG) || (type == MV_ETH_BM_HWF_LONG)\
									|| (type == MV_ETH_BM_MIXED_LONG))

/* BM short pool packet size						*/
/* These values assure that for both HWF and SWF,			*/
/* the total number of bytes allocated for each buffer will be 512	*/
#define MV_ETH_BM_SHORT_HWF_PKT_SIZE	RX_HWF_MAX_PKT_SIZE(512)
#define MV_ETH_BM_SHORT_PKT_SIZE	RX_MAX_PKT_SIZE(512)

struct bm_pool {
	int			pool;
	enum mv_eth_bm_type	type;
	int			capacity;
	int			buf_num;
	int			pkt_size;
	u32			*bm_pool;
	MV_ULONG		physAddr;
	spinlock_t		lock;
	u32			port_map;
	atomic_t		in_use;
	int			in_use_thresh;
	struct			pool_stats stats;
};

/* BM cookie (32 bits) definition */
/* bits[0-7]   - Flags  */
/*      bit0 - bm_cookie is invalid for SKB recycle */
#define MV_ETH_BM_COOKIE_F_INVALID		0
/*      bit7 - buffer is guaranteed */
#define MV_ETH_BM_COOKIE_F_GRNTD_OFF		7
/* bits[8-15]  - PoolId */
#define MV_ETH_BM_COOKIE_POOL_OFFS		8
/* bits[16-23] - Qset   */
#define MV_ETH_BM_COOKIE_QSET_OFFS		16
/* bits[24-31] - Cpu    */
#define MV_ETH_BM_COOKIE_CPU_OFFS		24

static inline int mv_eth_bm_cookie_grntd_get(__u32 cookie)
{
	return (cookie >> MV_ETH_BM_COOKIE_F_GRNTD_OFF) & 0x1;
}

static inline int mv_eth_bm_cookie_qset_get(__u32 cookie)
{
	return (cookie >> 16) & 0xFF;
}

static inline int mv_eth_bm_cookie_pool_get(__u32 cookie)
{
	return (cookie >> 8) & 0xFF;
}

static inline __u32 mv_eth_bm_cookie_pool_set(__u32 cookie, int pool)
{
	__u32 bm;

	bm = cookie & ~(0xFF << MV_ETH_BM_COOKIE_POOL_OFFS);
	bm |= ((pool & 0xFF) << MV_ETH_BM_COOKIE_POOL_OFFS);

	return bm;
}
static inline int mv_eth_bm_cookie_cpu_get(__u32 cookie)
{
	return (cookie >> MV_ETH_BM_COOKIE_CPU_OFFS) & 0xFF;
}

/* Build bm cookie from rx_desc */
/* Cookie includes information needed to return buffer to bm pool: poolid, qset, etc */
static inline __u32 mv_eth_bm_cookie_build(struct pp2_rx_desc *rx_desc)
{
	int pool = mvPp2RxBmPoolId(rx_desc);
	int cpu = smp_processor_id();
	int qset = (rx_desc->bmQset & PP2_RX_BUFF_QSET_NUM_MASK) >> PP2_RX_BUFF_QSET_NUM_OFFS;
	int grntd = ((rx_desc->bmQset & PP2_RX_BUFF_TYPE_MASK) >> PP2_RX_BUFF_TYPE_OFFS);

	return ((pool & 0xFF) << MV_ETH_BM_COOKIE_POOL_OFFS) |
		((cpu & 0xFF) << MV_ETH_BM_COOKIE_CPU_OFFS) |
		((qset & 0xFF) << MV_ETH_BM_COOKIE_QSET_OFFS) |
		((grntd & 0x1) << MV_ETH_BM_COOKIE_F_GRNTD_OFF);

}

static inline int mv_eth_bm_in_use_read(struct bm_pool *bm)
{
	return atomic_read(&bm->in_use);
}

extern struct bm_pool mv_eth_pool[MV_ETH_BM_POOLS];
extern struct eth_port **mv_eth_ports;

static inline void mv_eth_rx_csum(struct eth_port *pp, struct pp2_rx_desc *rx_desc, struct sk_buff *skb)
{
	if (pp->dev->features & NETIF_F_RXCSUM) {
		if ((PP2_RX_L3_IS_IP4(rx_desc->status) && !PP2_RX_IP4_HDR_ERR(rx_desc->status)) ||
			(PP2_RX_L3_IS_IP6(rx_desc->status))) {
			if ((PP2_RX_L4_IS_UDP(rx_desc->status) || PP2_RX_L4_IS_TCP(rx_desc->status)) &&
				(PP2_RX_L4_CHK_OK(rx_desc->status))) {
				skb->csum = 0;
				skb->ip_summed = CHECKSUM_UNNECESSARY;
				STAT_DBG(pp->stats.rx_csum_hw++);
				return;
			}
		}
	}

	skb->ip_summed = CHECKSUM_NONE;
	STAT_DBG(pp->stats.rx_csum_sw++);
}

static inline void mv_eth_interrupts_unmask(struct eth_port *pp)
{
	int cpu = smp_processor_id();
	struct napi_group_ctrl *napi_group;

	napi_group = pp->cpu_config[cpu]->napi_group;

	if (napi_group == NULL)
		return;


	/* unmask interrupts - for RX unmask only RXQs that are in the same napi group */
#ifdef CONFIG_MV_ETH_TXDONE_ISR
	mvPp2GbeIsrRxTxUnmask(pp->port, napi_group->rxq_mask, 1 /* unmask TxDone interrupts */);
#else
	mvPp2GbeIsrRxTxUnmask(pp->port, napi_group->rxq_mask, 0 /* mask TxDone interrupts */);
#endif /* CONFIG_MV_ETH_TXDONE_ISR */
}

static inline void mv_eth_interrupts_mask(struct eth_port *pp)
{
	mvPp2GbeIsrRxTxMask(pp->port);
}

static inline int mv_eth_ctrl_is_tx_enabled(struct eth_port *pp)
{
	if (!pp)
		return -ENODEV;

	if (pp->flags & MV_ETH_F_CONNECT_LINUX)
		return 1;

	return 0;
}

/*
	Check if there are enough descriptors in physical TXQ.

	return: 1 - not enough descriptors,  0 - enough descriptors
*/
static inline int mv_eth_phys_desc_num_check(struct txq_cpu_ctrl *txq_ctrl, int num)
{

	if ((txq_ctrl->txq_count + num) > txq_ctrl->txq_size) {
		/*
		printk(KERN_ERR "eth_tx: txq_ctrl->txq=%d - no_resource: txq_count=%d, txq_size=%d, num=%d\n",
			txq_ctrl->txq, txq_ctrl->txq_count, txq_ctrl->txq_size, num);
		*/
		STAT_ERR(txq_ctrl->stats.txq_err++);
		return 1;
	}
	return 0;
}

/*
	Check if there are enough reserved descriptors for SWF
	If not enough, then try to reqest chunk of reserved descriptors and check again.

	return: 1 - not enough descriptors,  0 - enough descriptors
*/
static inline int mv_eth_reserved_desc_num_proc(struct eth_port *pp, int txp, int txq, int num)
{
	struct tx_queue *txq_ctrl = &pp->txq_ctrl[txp * CONFIG_MV_ETH_TXQ + txq];
	struct txq_cpu_ctrl *txq_cpu_p;
	struct txq_cpu_ctrl *txq_cpu_ptr =  &txq_ctrl->txq_cpu[smp_processor_id()];


	if (txq_cpu_ptr->reserved_num < num) {
		int req, new_reserved, cpu, txq_count = 0;

		/* new chunk is necessary */

		for_each_possible_cpu(cpu) {
			/* compute tolat txq used descriptors */
			txq_cpu_p = &txq_ctrl->txq_cpu[cpu];
			txq_count += txq_cpu_p->txq_count;
			txq_count += txq_cpu_p->reserved_num;
		}

		req = MV_MAX(txq_ctrl->rsvd_chunk, num - txq_cpu_ptr->reserved_num);
		txq_count += req;

		if (txq_count  > txq_ctrl->swf_size) {
			STAT_ERR(txq_cpu_ptr->stats.txq_err++);
			return 1;
		}


		new_reserved = mvPp2TxqAllocReservedDesc(pp->port, txp, txq, req);
		txq_cpu_ptr->reserved_num += new_reserved;

		if (txq_cpu_ptr->reserved_num < num)
			/* out of resources - drop packet*/
			return 1;

		STAT_DBG(txq_cpu_ptr->stats.txq_txreq++);
	}

	return 0;
}
/*
	Check if there are enough descriptors in aggregated TXQ.
	If not enough, then try to update number of occupied aggr descriptors and check again.

	return: 1 - not enough descriptors,  0 - enough descriptors
*/
static inline int mv_eth_aggr_desc_num_check(struct aggr_tx_queue *aggr_txq_ctrl, int num)
{
	/* Is enough aggregated TX descriptors to send packet */
	if ((aggr_txq_ctrl->txq_count + num) > aggr_txq_ctrl->txq_size) {
		/* update number of available aggregated TX descriptors */
		aggr_txq_ctrl->txq_count = mvPp2AggrTxqPendDescNumGet(smp_processor_id());
	}
	/* Is enough aggregated descriptors */
	if ((aggr_txq_ctrl->txq_count + num) > aggr_txq_ctrl->txq_size) {
		/*
		printk(KERN_ERR "eth_tx: txq_ctrl->txq=%d - no_resource: txq_count=%d, txq_size=%d, num=%d\n",
			txq_ctrl->txq, txq_ctrl->txq_count, txq_ctrl->txq_size, num);
		*/
		STAT_ERR(aggr_txq_ctrl->stats.txq_err++);
		return 1;
	}

	return 0;
}

static inline void mv_eth_tx_desc_flush(struct pp2_tx_desc *tx_desc)
{
#if defined(MV_CPU_BE)
	/*mvNetaTxqDescSwap(tx_desc);*/
#endif /* MV_CPU_BE */

	mvOsCacheLineFlush(NULL, tx_desc);
}

static inline void *mv_eth_extra_pool_get(struct eth_port *pp)
{
	void *ext_buf;

	spin_lock(&pp->extLock);
	if (mvStackIndex(pp->extArrStack) == 0) {
		STAT_ERR(pp->stats.ext_stack_empty++);
		ext_buf = mvOsMalloc(CONFIG_MV_ETH_EXTRA_BUF_SIZE);
	} else {
		STAT_DBG(pp->stats.ext_stack_get++);
		ext_buf = (void *)mvStackPop(pp->extArrStack);
	}
	spin_unlock(&pp->extLock);

	return ext_buf;
}

static inline int mv_eth_extra_pool_put(struct eth_port *pp, void *ext_buf)
{
	spin_lock(&pp->extLock);
	if (mvStackIsFull(pp->extArrStack)) {
		STAT_ERR(pp->stats.ext_stack_full++);
		spin_unlock(&pp->extLock);
		mvOsFree(ext_buf);
		return 1;
	}
	mvStackPush(pp->extArrStack, (MV_U32)ext_buf);
	STAT_DBG(pp->stats.ext_stack_put++);
	spin_unlock(&pp->extLock);
	return 0;
}

static inline void mv_eth_add_tx_done_timer(struct cpu_ctrl *cpuCtrl)
{
	if (test_and_set_bit(MV_ETH_F_TX_DONE_TIMER_BIT, &(cpuCtrl->flags)) == 0) {

		cpuCtrl->tx_done_timer.expires = jiffies + ((HZ * CONFIG_MV_ETH_TX_DONE_TIMER_PERIOD) / 1000); /* ms */
		STAT_INFO(cpuCtrl->pp->stats.tx_done_timer_add[smp_processor_id()]++);
		add_timer_on(&cpuCtrl->tx_done_timer, smp_processor_id());
	}
}

static inline void mv_eth_shadow_inc_get(struct txq_cpu_ctrl *txq_cpu)
{
	txq_cpu->shadow_txq_get_i++;
	if (txq_cpu->shadow_txq_get_i == txq_cpu->txq_size)
		txq_cpu->shadow_txq_get_i = 0;
}

static inline void mv_eth_shadow_inc_put(struct txq_cpu_ctrl *txq_cpu)
{
	txq_cpu->shadow_txq_put_i++;
	if (txq_cpu->shadow_txq_put_i == txq_cpu->txq_size)
		txq_cpu->shadow_txq_put_i = 0;
}

static inline void mv_eth_shadow_dec_put(struct txq_cpu_ctrl *txq_cpu)
{
	if (txq_cpu->shadow_txq_put_i == 0)
		txq_cpu->shadow_txq_put_i = txq_cpu->txq_size - 1;
	else
		txq_cpu->shadow_txq_put_i--;
}

static inline u32 mv_eth_shadow_get_pop(struct txq_cpu_ctrl *txq_cpu)
{
	u32 res = txq_cpu->shadow_txq[txq_cpu->shadow_txq_get_i];

	txq_cpu->shadow_txq_get_i++;
	if (txq_cpu->shadow_txq_get_i == txq_cpu->txq_size)
		txq_cpu->shadow_txq_get_i = 0;
	return res;
}

static inline void mv_eth_shadow_push(struct txq_cpu_ctrl *txq_cpu, int val)
{
	txq_cpu->shadow_txq[txq_cpu->shadow_txq_put_i] = val;
	txq_cpu->shadow_txq_put_i++;
	if (txq_cpu->shadow_txq_put_i == txq_cpu->txq_size)
		txq_cpu->shadow_txq_put_i = 0;
}

/* Free skb pair */
static inline void mv_eth_skb_free(struct sk_buff *skb)
{
#ifdef CONFIG_NET_SKB_RECYCLE
	skb->skb_recycle = NULL;
	skb->hw_cookie = 0;
#endif /* CONFIG_NET_SKB_RECYCLE */

	dev_kfree_skb_any(skb);
}

/* PPv2.1 new API - pass packet to Qset */
/*
static inline void mv_eth_pool_qset_put(int pool, MV_ULONG phys_addr, MV_ULONG cookie, struct pp2_rx_desc *rx_desc)
{
	int qset, is_grntd;

	qset = (rx_desc->bmQset & PP2_RX_BUFF_QSET_NUM_MASK) >> PP2_RX_BUFF_QSET_NUM_OFFS;
	is_grntd = (rx_desc->bmQset & PP2_RX_BUFF_TYPE_MASK) >> PP2_RX_BUFF_TYPE_OFFS;

	mvBmPoolQsetPut(pool, (MV_ULONG) phys_addr, (MV_ULONG) cookie, qset, is_grntd);
}
*/

/* Pass pkt to BM Pool or RXQ ring */
static inline void mv_eth_pool_refill(struct bm_pool *ppool, __u32 bm,
				MV_ULONG phys_addr, MV_ULONG cookie)
{
	int pool = mv_eth_bm_cookie_pool_get(bm);
	unsigned long flags = 0;
	int grntd, qset;

	/* Refill BM pool */
	STAT_DBG(ppool->stats.bm_put++);
	MV_ETH_LIGHT_LOCK(flags);
#ifdef CONFIG_MV_ETH_PP2_1
	grntd =  mv_eth_bm_cookie_grntd_get(bm);
	qset = mv_eth_bm_cookie_qset_get(bm);
	mvBmPoolQsetPut(pool, (MV_ULONG) phys_addr, (MV_ULONG) cookie, qset, grntd);
#else
	mvBmPoolPut(pool, phys_addr, cookie);
#endif
	MV_ETH_LIGHT_UNLOCK(flags);
}

static inline MV_U32 mv_eth_pool_get(int pool)
{
	MV_U32 bufCookie;
	unsigned long flags = 0;

	MV_ETH_LIGHT_LOCK(flags);
	bufCookie = mvBmPoolGet(pool, NULL);
	MV_ETH_LIGHT_UNLOCK(flags);
	return bufCookie;
}


/******************************************************
 * Function prototypes --                             *
 ******************************************************/
int         mv_eth_start(struct net_device *dev);
int         mv_eth_stop(struct net_device *dev);
int         mv_eth_change_mtu(struct net_device *dev, int mtu);
int         mv_eth_check_mtu_internals(struct net_device *dev, int mtu);
int         mv_eth_check_mtu_valid(struct net_device *dev, int mtu);

int         mv_eth_set_mac_addr(struct net_device *dev, void *mac);
void        mv_eth_change_rx_flags(struct net_device *dev, int flags);
int         mv_eth_open(struct net_device *dev);
int         mv_eth_port_suspend(int port);
int         mv_eth_port_resume(int port);
int         mv_eth_resume_clock(int port);
int         mv_eth_suspend_clock(int port);
int         mv_eth_suspend_internals(struct eth_port *pp);
int         mv_eth_resume_internals(struct eth_port *pp, int mtu);
int         mv_eth_restore_registers(struct eth_port *pp, int mtu);

void	    mv_eth_port_promisc_set(int port);

void        mv_eth_win_init(void);
int         mv_eth_resume_network_interfaces(struct eth_port *pp);
int         mv_eth_pm_mode_set(int port, int mode);

irqreturn_t mv_eth_isr(int irq, void *dev_id);
irqreturn_t mv_eth_link_isr(int irq, void *dev_id);
int         mv_eth_start_internals(struct eth_port *pp, int mtu);
int         mv_eth_stop_internals(struct eth_port *pp);
int         mv_eth_change_mtu_internals(struct net_device *netdev, int mtu);

int         mv_eth_rx_reset(int port);
int         mv_eth_txq_clean(int port, int txp, int txq);
int         mv_eth_txp_clean(int port, int txp);
int         mv_eth_all_ports_cleanup(void);
int         mv_eth_all_ports_probe(void);

MV_STATUS   mv_eth_rx_ptks_coal_set(int port, int rxq, MV_U32 value);
MV_STATUS   mv_eth_rx_time_coal_set(int port, int rxq, MV_U32 value);
MV_STATUS   mv_eth_tx_done_ptks_coal_set(int port, int txp, int txq, MV_U32 value);

struct eth_port     *mv_eth_port_by_id(unsigned int port);
bool                 mv_eth_netdev_find(unsigned int if_index);

void        mv_eth_mac_show(int port);
void        mv_eth_dscp_map_show(int port);
int         mv_eth_rxq_dscp_map_set(int port, int rxq, unsigned char dscp);
int         mv_eth_txq_dscp_map_set(int port, int txq, unsigned char dscp);

int         mv_eth_rxq_vlan_prio_set(int port, int rxq, unsigned char prio);
void        mv_eth_vlan_prio_show(int port);

void        mv_eth_netdev_print(struct net_device *netdev);
void        mv_eth_status_print(void);
void        mv_eth_port_status_print(unsigned int port);
void        mv_eth_port_stats_print(unsigned int port);
void 	    mv_eth_pool_status_print(int pool);

void        mv_eth_set_noqueue(struct net_device *dev, int enable);
void	    mv_eth_ctrl_pnc(int en);
void        mv_eth_ctrl_hwf(int en);
int         mv_eth_ctrl_recycle(int en);
void        mv_eth_ctrl_txdone(int num);
int         mv_eth_ctrl_tx_mh(int port, u16 mh);

int         mv_eth_ctrl_tx_cmd_dsa(int port, u16 dsa);
int         mv_eth_ctrl_tx_cmd_color(int port, u16 color);
int         mv_eth_ctrl_tx_cmd_gem_id(int port, u16 gem_id);
int         mv_eth_ctrl_tx_cmd_pon_fec(int port, u16 pon_fec);
int         mv_eth_ctrl_tx_cmd_gem_oem(int port, u16 gem_oem);
int         mv_eth_ctrl_tx_cmd_mod(int port, u16 mod);
int         mv_eth_ctrl_tx_cmd_pme_dptr(int port, u16 pme_dptr);
int         mv_eth_ctrl_tx_cmd_pme_prog(int port, u16 pme_prog);

int         mv_eth_ctrl_txq_cpu_def(int port, int txp, int txq, int cpu);
int         mv_eth_ctrl_flag(int port, u32 flag, u32 val);
int         mv_eth_ctrl_tx_flag(int port, u32 flag, u32 val);
int	    mv_eth_ctrl_dbg_flag(int port, u32 flag, u32 val);
int	    mv_eth_ctrl_txq_size_set(int port, int txp, int txq, int txq_size);
int         mv_eth_ctrl_txq_limits_set(int port, int txp, int txq, int hwf_size, int swf_size);
int         mv_eth_ctrl_txq_chunk_set(int port, int txp, int txq, int chunk_size);
int         mv_eth_ctrl_rxq_size_set(int port, int rxq, int value);
int	    mv_eth_ctrl_pool_buf_num_set(int pool, int buf_num);
int         mv_eth_ctrl_pool_detach(int port, struct bm_pool *ppool);
int         mv_eth_ctrl_pool_size_set(int pool, int pkt_size);
int	    mv_eth_ctrl_long_pool_set(int port, int pool);
int	    mv_eth_ctrl_short_pool_set(int port, int pool);
int	    mv_eth_ctrl_hwf_long_pool_set(int port, int pool);
int	    mv_eth_ctrl_hwf_short_pool_set(int port, int pool);
int     mv_eth_ctrl_set_poll_rx_weight(int port, u32 weight);
int     mv_eth_ctrl_pool_port_map_get(int pool);
void        mv_eth_tx_desc_print(struct pp2_tx_desc *desc);
void        mv_eth_pkt_print(struct eth_pbuf *pkt);
void        mv_eth_rx_desc_print(struct pp2_rx_desc *desc);
void        mv_eth_skb_print(struct sk_buff *skb);
void        mv_eth_link_status_print(int port);
void        mv_eth_buff_hdr_rx_dump(struct eth_port *pp, struct pp2_rx_desc *rx_desc);
void        mv_eth_buff_hdr_rx(struct eth_port *pp, struct pp2_rx_desc *rx_desc);

/* External MAC support (i.e. PON) */
/* callback functions to be called by netdev (implemented in external MAC module) */
struct mv_eth_ext_mac_ops {
	MV_BOOL		(*link_status_get)(int port_id);
	MV_STATUS	(*max_pkt_size_set)(int port_id, MV_U32 maxEth);
	MV_STATUS	(*mac_addr_set)(int port_id, void *addr);
	MV_STATUS	(*port_enable)(int port_id);
	MV_STATUS	(*port_disable)(int port_id);
	MV_STATUS	(*mib_counters_show)(int port_id);
};

/* callback functions to be called by external MAC module (implemented in netdev) */
struct mv_netdev_notify_ops {
	void		(*link_notify)(int port_id, MV_BOOL state);
};

/* Called by external MAC module */
void mv_eth_ext_mac_ops_register(int port_id,
		struct mv_eth_ext_mac_ops **extern_mac_ops, struct mv_netdev_notify_ops **netdev_ops);

#ifdef CONFIG_MV_INCLUDE_PON
MV_BOOL mv_pon_link_status(MV_ETH_PORT_STATUS *link);
MV_STATUS mv_pon_mtu_config(MV_U32 maxEth);
MV_STATUS mv_pon_set_mac_addr(void *addr);
MV_STATUS mv_pon_enable(void);
MV_STATUS mv_pon_disable(void);
#endif

#ifdef CONFIG_MV_ETH_TX_SPECIAL
void        mv_eth_tx_special_check_func(int port, int (*func)(int port, struct net_device *dev,
				  struct sk_buff *skb, struct mv_eth_tx_spec *tx_spec_out));
#endif /* CONFIG_MV_ETH_TX_SPECIAL */

#ifdef CONFIG_MV_ETH_RX_SPECIAL
void        mv_eth_rx_special_proc_func(int port, void (*func)(int port, int rxq, struct net_device *dev,
							struct sk_buff *skb, struct pp2_rx_desc *rx_desc));
#endif /* CONFIG_MV_ETH_RX_SPECIAL */

int  mv_eth_poll(struct napi_struct *napi, int budget);
void mv_eth_link_event(struct eth_port *pp, int print);

int mv_eth_rx_policy(u32 cause);
int mv_eth_refill(struct bm_pool *ppool, __u32 bm, int is_recycle);
u32 mv_eth_txq_done(struct eth_port *pp, struct tx_queue *txq_ctrl);
u32 mv_eth_tx_done_gbe(struct eth_port *pp, u32 cause_tx_done, int *tx_todo);
u32 mv_eth_tx_done_pon(struct eth_port *pp, int *tx_todo);


/*****************************************
 *            NAPI Group API             *
 *****************************************/
int  mv_eth_port_napi_group_create(int port, int group);
int  mv_eth_port_napi_group_delete(int port, int group);
int  mv_eth_napi_set_cpu_affinity(int port, int group, int cpu_mask);
int  mv_eth_napi_set_rxq_affinity(int port, int group, int rxq_mask);
void mv_eth_napi_groups_print(int port);

struct pp2_rx_desc *mv_eth_rx_prefetch(struct eth_port *pp,
					MV_PP2_PHYS_RXQ_CTRL *rx_ctrl, int rx_done, int rx_todo);

void		*mv_eth_bm_pool_create(int pool, int capacity, MV_ULONG *physAddr);

#if defined(CONFIG_MV_ETH_HWF) && !defined(CONFIG_MV_ETH_BM_CPU)
MV_STATUS mv_eth_hwf_bm_create(int port, int mtuPktSize);
void      mv_hwf_bm_dump(void);
#endif /* CONFIG_MV_ETH_HWF && !CONFIG_MV_ETH_BM_CPU */

#endif /* __mv_netdev_h__ */

