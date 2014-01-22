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
#include <linux/mv_neta.h>
#include <net/ip.h>

#include "mvCommon.h"
#include "mvOs.h"
#include "mv802_3.h"
#include "mvStack.h"

#include "gbe/mvNeta.h"
#include "bm/mvBmRegs.h"
#include "bm/mvBm.h"

#define MV_ETH_MAX_NETDEV_NUM	24

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

#ifdef CONFIG_MV_ETH_PNC
extern unsigned int mv_eth_pnc_ctrl_en;
int mv_eth_ctrl_pnc(int en);
#endif /* CONFIG_MV_ETH_PNC */

extern int mv_ctrl_txdone;

/****************************************************************************
 * Rx buffer size: MTU + 2(Marvell Header) + 4(VLAN) + 14(MAC hdr) + 4(CRC) *
 ****************************************************************************/
#define RX_PKT_SIZE(mtu) \
		MV_ALIGN_UP((mtu) + 2 + 4 + ETH_HLEN + 4, CPU_D_CACHE_LINE_SIZE)

#define RX_BUF_SIZE(pkt_size)   ((pkt_size) + NET_SKB_PAD)


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
#ifdef CONFIG_MV_ETH_TXDONE_ISR
#define MV_ETH_TXDONE_INTR_MASK       (((1 << CONFIG_MV_ETH_TXQ) - 1) << NETA_CAUSE_TXQ_SENT_DESC_OFFS)
#else
#define MV_ETH_TXDONE_INTR_MASK       0
#endif

#define MV_ETH_MISC_SUM_INTR_MASK     (NETA_CAUSE_TX_ERR_SUM_MASK | NETA_CAUSE_MISC_SUM_MASK)
#define MV_ETH_RX_INTR_MASK           (((1 << CONFIG_MV_ETH_RXQ) - 1) << NETA_CAUSE_RXQ_OCCUP_DESC_OFFS)
#define NETA_RX_FL_DESC_MASK          (NETA_RX_F_DESC_MASK|NETA_RX_L_DESC_MASK)

/* NAPI CPU defualt group */
#define CPU_GROUP_DEF 0

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


#define mv_eth_lock(txq_ctrl, flags)			     \
{							     \
	if (txq_ctrl->flags & MV_ETH_F_TX_SHARED)	     \
		MV_ETH_LOCK(&txq_ctrl->queue_lock, flags)    \
	else                                                 \
		MV_ETH_LIGHT_LOCK(flags)		     \
}

#define mv_eth_unlock(txq_ctrl, flags)                        \
{							      \
	if (txq_ctrl->flags & MV_ETH_F_TX_SHARED)	      \
		MV_ETH_UNLOCK(&txq_ctrl->queue_lock, flags)   \
	else                                                  \
		MV_ETH_LIGHT_UNLOCK(flags)		      \
}

#if defined(CONFIG_CPU_SHEEVA_PJ4B_V7) || defined(CONFIG_CPU_SHEEVA_PJ4B_V6)
#  define mv_neta_wmb()
#else
#  define mv_neta_wmb() wmb()
#endif

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
	u32 txq_txdone;
#endif /* CONFIG_MV_ETH_STAT_DBG */
};

struct port_stats {

#ifdef CONFIG_MV_ETH_STAT_ERR
	u32 rx_error;
	u32 tx_timeout;
	u32 netif_stop;
	u32 ext_stack_empty;
	u32 ext_stack_full;
	u32 netif_wake;
	u32 state_err;
#endif /* CONFIG_MV_ETH_STAT_ERR */

#ifdef CONFIG_MV_ETH_STAT_INF
	u32 irq[CONFIG_NR_CPUS];
	u32 irq_err[CONFIG_NR_CPUS];
	u32 poll[CONFIG_NR_CPUS];
	u32 poll_exit[CONFIG_NR_CPUS];
	u32 tx_done_timer_event[CONFIG_NR_CPUS];
	u32 tx_done_timer_add[CONFIG_NR_CPUS];
	u32 tx_fragment;
	u32 tx_done;
	u32 cleanup_timer;
	u32 link;
	u32 netdev_stop;

#ifdef CONFIG_MV_ETH_RX_SPECIAL
	u32 rx_special;
#endif /* CONFIG_MV_ETH_RX_SPECIAL */

#ifdef CONFIG_MV_ETH_TX_SPECIAL
	u32	tx_special;
#endif /* CONFIG_MV_ETH_TX_SPECIAL */

#endif /* CONFIG_MV_ETH_STAT_INF */

#ifdef CONFIG_MV_ETH_STAT_DBG
	u32 rxq[CONFIG_MV_ETH_RXQ];
	u32 rx_tagged;
	u32 rxq_fill[CONFIG_MV_ETH_RXQ];
	u32 rx_netif;
	u32 rx_nfp;
	u32 rx_nfp_drop;
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
	u32 tx_tso_bytes;
	u32 ext_stack_put;
	u32 ext_stack_get;
#endif /* CONFIG_MV_ETH_STAT_DBG */
};

/* Used for define type of data saved in shadow: SKB or eth_pbuf or nothing */
#define MV_ETH_SHADOW_SKB		0x1
#define MV_ETH_SHADOW_EXT		0x2

/* Masks used for pp->flags */
#define MV_ETH_F_STARTED_BIT        0
#define MV_ETH_F_MH_BIT             1
#define MV_ETH_F_NO_PAD_BIT         2
#define MV_ETH_F_DBG_RX_BIT         3
#define MV_ETH_F_DBG_TX_BIT         4
#define MV_ETH_F_EXT_SWITCH_BIT	    5	/* port is connected to the Switch without the Gateway driver */
#define MV_ETH_F_CONNECT_LINUX_BIT  6	/* port is connected to Linux netdevice */
#define MV_ETH_F_LINK_UP_BIT        7
#define MV_ETH_F_DBG_DUMP_BIT       8
#define MV_ETH_F_DBG_ISR_BIT        9
#define MV_ETH_F_DBG_POLL_BIT       10
#define MV_ETH_F_NFP_EN_BIT         11
#define MV_ETH_F_SUSPEND_BIT        12
#define MV_ETH_F_STARTED_OLD_BIT    13 /*STARTED_BIT value before suspend */
#define MV_ETH_F_FORCE_LINK_BIT     14
#define MV_ETH_F_IFCAP_NETMAP_BIT   15

#define MV_ETH_F_STARTED           (1 << MV_ETH_F_STARTED_BIT)
#define MV_ETH_F_SWITCH            (1 << MV_ETH_F_SWITCH_BIT)
#define MV_ETH_F_MH                (1 << MV_ETH_F_MH_BIT)
#define MV_ETH_F_NO_PAD            (1 << MV_ETH_F_NO_PAD_BIT)
#define MV_ETH_F_DBG_RX            (1 << MV_ETH_F_DBG_RX_BIT)
#define MV_ETH_F_DBG_TX            (1 << MV_ETH_F_DBG_TX_BIT)
#define MV_ETH_F_EXT_SWITCH        (1 << MV_ETH_F_EXT_SWITCH_BIT)
#define MV_ETH_F_CONNECT_LINUX     (1 << MV_ETH_F_CONNECT_LINUX_BIT)
#define MV_ETH_F_LINK_UP           (1 << MV_ETH_F_LINK_UP_BIT)
#define MV_ETH_F_DBG_DUMP          (1 << MV_ETH_F_DBG_DUMP_BIT)
#define MV_ETH_F_DBG_ISR           (1 << MV_ETH_F_DBG_ISR_BIT)
#define MV_ETH_F_DBG_POLL          (1 << MV_ETH_F_DBG_POLL_BIT)
#define MV_ETH_F_NFP_EN            (1 << MV_ETH_F_NFP_EN_BIT)
#define MV_ETH_F_SUSPEND           (1 << MV_ETH_F_SUSPEND_BIT)
#define MV_ETH_F_STARTED_OLD       (1 << MV_ETH_F_STARTED_OLD_BIT)
#define MV_ETH_F_FORCE_LINK        (1 << MV_ETH_F_FORCE_LINK_BIT)
#define MV_ETH_F_IFCAP_NETMAP      (1 << MV_ETH_F_IFCAP_NETMAP_BIT)

/* Masks used for cpu_ctrl->flags */
#define MV_ETH_F_TX_DONE_TIMER_BIT  0
#define MV_ETH_F_CLEANUP_TIMER_BIT  1

#define MV_ETH_F_TX_DONE_TIMER		(1 << MV_ETH_F_TX_DONE_TIMER_BIT)	/* 0x01 */
#define MV_ETH_F_CLEANUP_TIMER		(1 << MV_ETH_F_CLEANUP_TIMER_BIT)	/* 0x02 */

/* Masks used for tx_queue->flags */
#define MV_ETH_F_TX_SHARED_BIT  0

#define MV_ETH_F_TX_SHARED		(1 << MV_ETH_F_TX_SHARED_BIT)	/* 0x01 */



/* One of three TXQ states */
#define MV_ETH_TXQ_FREE         0
#define MV_ETH_TXQ_CPU          1
#define MV_ETH_TXQ_HWF          2

#define MV_ETH_TXQ_INVALID		0xFF

struct mv_eth_tx_spec {
	u32		hw_cmd;	/* tx_desc offset = 0xC */
	u16		flags;
	u8		txp;
	u8		txq;
#ifdef CONFIG_MV_ETH_TX_SPECIAL
	void		(*tx_func) (u8 *data, int size, struct mv_eth_tx_spec *tx_spec);
#endif
};

struct tx_queue {
	MV_NETA_TXQ_CTRL   *q;
	u8                  cpu_owner[CONFIG_NR_CPUS]; /* counter */
	u8                  hwf_rxp;
	u8                  txp;
	u8                  txq;
	int                 txq_size;
	int                 txq_count;
	int                 bm_only;
	u32                 *shadow_txq; /* can be MV_ETH_PKT* or struct skbuf* */
	int                 shadow_txq_put_i;
	int                 shadow_txq_get_i;
	struct txq_stats    stats;
	spinlock_t          queue_lock;
	MV_U32              txq_done_pkts_coal;
	unsigned long       flags;
	int		    nfpCounter;
};

struct rx_queue {
	MV_NETA_RXQ_CTRL    *q;
	int                 rxq_size;
	int                 missed;
	MV_U32	            rxq_pkts_coal;
	MV_U32	            rxq_time_coal;
};

struct dist_stats {
	u32     *rx_dist;
	int     rx_dist_size;
	u32     *tx_done_dist;
	int     tx_done_dist_size;
	u32     *tx_tso_dist;
	int     tx_tso_dist_size;
};

struct cpu_ctrl {
	MV_U8  			cpuTxqMask;
	MV_U8			cpuRxqMask;
	MV_U8			cpuTxqOwner;
	MV_U8  			txq_tos_map[256];
	MV_U32			causeRxTx;
	struct eth_port		*pp;
	struct napi_struct	*napi;
	int			napiCpuGroup;
	int             	txq;
	int                     cpu;
	struct timer_list   	tx_done_timer;
	struct timer_list   	cleanup_timer;
	unsigned long       	flags;

};

struct eth_port {
	int                 port;
	bool                tagged; /* NONE/MH/DSA/EDSA/VLAN */
	struct mv_neta_pdata *plat_data;
	MV_NETA_PORT_CTRL   *port_ctrl;
	struct rx_queue     *rxq_ctrl;
	struct tx_queue     *txq_ctrl;
	int                 txp_num;
	struct net_device   *dev;
	rwlock_t            rwlock;
	struct bm_pool      *pool_long;
	int                 pool_long_num;
#ifdef CONFIG_MV_ETH_BM_CPU
	struct bm_pool      *pool_short;
	int                 pool_short_num;
#endif /* CONFIG_MV_ETH_BM_CPU */
	struct napi_struct  *napiGroup[CONFIG_MV_ETH_NAPI_GROUPS];
	unsigned long       flags;	/* MH, TIMER, etc. */
	u32                 hw_cmd;	/* offset 0xc in TX descriptor */
	int                 txp;
	u16                 tx_mh;	/* 2B MH */
	struct port_stats   stats;
	struct dist_stats   dist_stats;
	int                 weight;
	MV_STACK            *extArrStack;
	int                 extBufSize;
	spinlock_t          extLock;
	/* Ethtool parameters */
	__u16               speed_cfg;
	__u8                duplex_cfg;
	__u8                autoneg_cfg;
	__u16		        advertise_cfg;
	__u32               rx_time_coal_cfg;
	__u32               rx_pkts_coal_cfg;
	__u32               tx_pkts_coal_cfg;
	__u32               rx_time_low_coal_cfg;
	__u32               rx_time_high_coal_cfg;
	__u32               rx_pkts_low_coal_cfg;
	__u32               rx_pkts_high_coal_cfg;
	__u32               pkt_rate_low_cfg;
	__u32               pkt_rate_high_cfg;
	__u32               rate_current; /* unknown (0), low (1), normal (2), high (3) */
	__u32               rate_sample_cfg;
	__u32               rx_adaptive_coal_cfg;
	/* Rate calculate */
	unsigned long	    rx_rate_pkts;
	unsigned long	    rx_timestamp;
#ifdef CONFIG_MV_ETH_RX_SPECIAL
	void    (*rx_special_proc)(int port, int rxq, struct net_device *dev,
					struct sk_buff *skb, struct neta_rx_desc *rx_desc);
#endif /* CONFIG_MV_ETH_RX_SPECIAL */
#ifdef CONFIG_MV_ETH_TX_SPECIAL
	int     (*tx_special_check)(int port, struct net_device *dev, struct sk_buff *skb,
					struct mv_eth_tx_spec *tx_spec_out);
#endif /* CONFIG_MV_ETH_TX_SPECIAL */

	MV_U32              cpu_mask;
	MV_U32              rx_indir_table[256];
	struct cpu_ctrl	    *cpu_config[CONFIG_NR_CPUS];
	MV_U32              sgmii_serdes;
	int	                wol_mode;
};

struct eth_netdev {
	u16     tx_vlan_mh;		/* 2B MH */
	u16     vlan_grp_id;		/* vlan group ID */
	u16     port_map;		/* switch port map */
	u16     link_map;		/* switch port link map */
	u16     cpu_port;		/* switch CPU port */
	u16     group;
};

struct eth_dev_priv {
	struct eth_port     *port_p;
	struct eth_netdev   *netdev_p;
};

#define MV_ETH_PRIV(dev)        ((struct eth_port *)(netdev_priv(dev)))
#define MV_DEV_STAT(dev)        (&((dev)->stats))

/* define which Switch ports are relevant */
#define SWITCH_CONNECTED_PORTS_MASK	0x7F

#define MV_SWITCH_ID_0			0
#define MV_ETH_PORT_0			0
#define MV_ETH_PORT_1			1

struct pool_stats {
#ifdef CONFIG_MV_ETH_STAT_ERR
	u32 skb_alloc_oom;
	u32 stack_empty;
	u32 stack_full;
#endif /* CONFIG_MV_ETH_STAT_ERR */

#ifdef CONFIG_MV_ETH_STAT_DBG
	u32 bm_put;
	u32 stack_put;
	u32 stack_get;
	u32 skb_alloc_ok;
	u32 skb_recycled_ok;
	u32 skb_recycled_err;
	u32 skb_hw_cookie_err;
#endif /* CONFIG_MV_ETH_STAT_DBG */
};

struct bm_pool {
	int         pool;
	int         capacity;
	int         buf_num;
	int         pkt_size;
	MV_ULONG    physAddr;
	u32         *bm_pool;
	MV_STACK    *stack;
	spinlock_t  lock;
	u32         port_map;
	int         missed;		/* FIXME: move to stats */
	struct pool_stats  stats;
};

#ifdef CONFIG_MV_ETH_BM_CPU
#define MV_ETH_BM_POOLS	        MV_BM_POOLS
#define mv_eth_pool_bm(p)       (p->bm_pool)
#define mv_eth_txq_bm(q)        (q->bm_only)
#else
#define MV_ETH_BM_POOLS		CONFIG_MV_ETH_PORTS_NUM
#define mv_eth_pool_bm(p)       0
#define mv_eth_txq_bm(q)        0
#endif /* CONFIG_MV_ETH_BM_CPU */

#ifdef CONFIG_MV_ETH_BM
MV_STATUS mv_eth_bm_config_get(void);
int mv_eth_bm_config_pkt_size_get(int pool);
int mv_eth_bm_config_pkt_size_set(int pool, int pkt_size);
int mv_eth_bm_config_short_pool_get(int port);
int mv_eth_bm_config_short_buf_num_get(int port);
int mv_eth_bm_config_long_pool_get(int port);
int mv_eth_bm_config_long_buf_num_get(int port);
void mv_eth_bm_config_print(void);
#endif /* CONFIG_MV_ETH_BM */

void mv_eth_stack_print(int port, MV_BOOL isPrintElements);
extern struct bm_pool mv_eth_pool[MV_ETH_BM_POOLS];
extern struct eth_port **mv_eth_ports;

static inline void mv_eth_interrupts_unmask(void *arg)
{
	struct eth_port *pp = arg;

	/* unmask interrupts */
	if (!test_bit(MV_ETH_F_FORCE_LINK_BIT, &(pp->flags)))
		MV_REG_WRITE(NETA_INTR_MISC_MASK_REG(pp->port), NETA_CAUSE_LINK_CHANGE_MASK);

	MV_REG_WRITE(NETA_INTR_NEW_MASK_REG(pp->port),
		(MV_ETH_MISC_SUM_INTR_MASK |
		MV_ETH_TXDONE_INTR_MASK |
		MV_ETH_RX_INTR_MASK));
}

static inline void mv_eth_interrupts_mask(void *arg)
{
	struct eth_port *pp = arg;

	/* clear all ethernet port interrupts */
	MV_REG_WRITE(NETA_INTR_MISC_CAUSE_REG(pp->port), 0);
	MV_REG_WRITE(NETA_INTR_OLD_CAUSE_REG(pp->port), 0);

	/* mask all ethernet port interrupts */
	MV_REG_WRITE(NETA_INTR_NEW_MASK_REG(pp->port), 0);
	MV_REG_WRITE(NETA_INTR_OLD_MASK_REG(pp->port), 0);
	MV_REG_WRITE(NETA_INTR_MISC_MASK_REG(pp->port), 0);
}


static inline void mv_eth_txq_update_shared(struct tx_queue *txq_ctrl, struct eth_port *pp)
{
	int numOfRefCpu, cpu;
	struct cpu_ctrl	*cpuCtrl;

	numOfRefCpu = 0;

	for_each_possible_cpu(cpu) {
		cpuCtrl = pp->cpu_config[cpu];

		if (txq_ctrl->cpu_owner[cpu] == 0)
			cpuCtrl->cpuTxqOwner &= ~(1 << txq_ctrl->txq);
		else {
			numOfRefCpu++;
			cpuCtrl->cpuTxqOwner |= (1 << txq_ctrl->txq);
		}
	}

	if ((txq_ctrl->nfpCounter != 0) || (numOfRefCpu > 1))
		txq_ctrl->flags |=  MV_ETH_F_TX_SHARED;
	else
		txq_ctrl->flags &= ~MV_ETH_F_TX_SHARED;
}

static inline int mv_eth_ctrl_is_tx_enabled(struct eth_port *pp)
{
	if (!pp)
		return -ENODEV;

	if (pp->flags & MV_ETH_F_CONNECT_LINUX)
		return 1;

	return 0;
}

static inline struct neta_tx_desc *mv_eth_tx_desc_get(struct tx_queue *txq_ctrl, int num)
{
	/* Is enough TX descriptors to send packet */
	if ((txq_ctrl->txq_count + num) >= txq_ctrl->txq_size) {
		/*
		printk(KERN_ERR "eth_tx: txq_ctrl->txq=%d - no_resource: txq_count=%d, txq_size=%d, num=%d\n",
			txq_ctrl->txq, txq_ctrl->txq_count, txq_ctrl->txq_size, num);
		*/
		STAT_ERR(txq_ctrl->stats.txq_err++);
		return NULL;
	}
	return mvNetaTxqNextDescGet(txq_ctrl->q);
}

static inline void mv_eth_tx_desc_flush(struct neta_tx_desc *tx_desc)
{
#if defined(MV_CPU_BE)
	mvNetaTxqDescSwap(tx_desc);
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

static inline void mv_eth_add_cleanup_timer(struct cpu_ctrl *cpuCtrl)
{
	if (test_and_set_bit(MV_ETH_F_CLEANUP_TIMER_BIT, &(cpuCtrl->flags)) == 0) {
		cpuCtrl->cleanup_timer.expires = jiffies + ((HZ * CONFIG_MV_ETH_CLEANUP_TIMER_PERIOD) / 1000); /* ms */
		add_timer_on(&cpuCtrl->cleanup_timer, smp_processor_id());
	}
}

static inline void mv_eth_add_tx_done_timer(struct cpu_ctrl *cpuCtrl)
{
	if (test_and_set_bit(MV_ETH_F_TX_DONE_TIMER_BIT, &(cpuCtrl->flags)) == 0) {

		cpuCtrl->tx_done_timer.expires = jiffies + ((HZ * CONFIG_MV_ETH_TX_DONE_TIMER_PERIOD) / 1000); /* ms */
		STAT_INFO(cpuCtrl->pp->stats.tx_done_timer_add[smp_processor_id()]++);
		add_timer_on(&cpuCtrl->tx_done_timer, smp_processor_id());
	}
}

static inline void mv_eth_shadow_inc_get(struct tx_queue *txq)
{
	txq->shadow_txq_get_i++;
	if (txq->shadow_txq_get_i == txq->txq_size)
		txq->shadow_txq_get_i = 0;
}

static inline void mv_eth_shadow_inc_put(struct tx_queue *txq)
{
	txq->shadow_txq_put_i++;
	if (txq->shadow_txq_put_i == txq->txq_size)
		txq->shadow_txq_put_i = 0;
}

static inline void mv_eth_shadow_dec_put(struct tx_queue *txq)
{
	if (txq->shadow_txq_put_i == 0)
		txq->shadow_txq_put_i = txq->txq_size - 1;
	else
		txq->shadow_txq_put_i--;
}

/* Free pkt + skb pair */
static inline void mv_eth_pkt_free(struct eth_pbuf *pkt)
{
	struct sk_buff *skb = (struct sk_buff *)pkt->osInfo;

#ifdef CONFIG_NET_SKB_RECYCLE
	skb->skb_recycle = NULL;
	skb->hw_cookie = 0;
#endif /* CONFIG_NET_SKB_RECYCLE */

	dev_kfree_skb_any(skb);
	mvOsFree(pkt);
}

static inline int mv_eth_pool_put(struct bm_pool *pool, struct eth_pbuf *pkt)
{
	unsigned long flags = 0;

	MV_ETH_LOCK(&pool->lock, flags);
	if (mvStackIsFull(pool->stack)) {
		STAT_ERR(pool->stats.stack_full++);
		MV_ETH_UNLOCK(&pool->lock, flags);

		/* free pkt+skb */
		mv_eth_pkt_free(pkt);
		return 1;
	}
	mvStackPush(pool->stack, (MV_U32) pkt);
	STAT_DBG(pool->stats.stack_put++);
	MV_ETH_UNLOCK(&pool->lock, flags);
	return 0;
}


/* Pass pkt to BM Pool or RXQ ring */
static inline void mv_eth_rxq_refill(struct eth_port *pp, int rxq,
				     struct eth_pbuf *pkt, struct bm_pool *pool, struct neta_rx_desc *rx_desc)
{
	if (mv_eth_pool_bm(pool)) {
		/* Refill BM pool */
		STAT_DBG(pool->stats.bm_put++);
		mvBmPoolPut(pkt->pool, (MV_ULONG) pkt->physAddr);
		mvOsCacheLineInv(pp->dev->dev.parent, rx_desc);
	} else {
		/* Refill Rx descriptor */
		STAT_DBG(pp->stats.rxq_fill[rxq]++);
		mvNetaRxDescFill(rx_desc, pkt->physAddr, (MV_U32)pkt);
		mvOsCacheLineFlush(pp->dev->dev.parent, rx_desc);
	}
}

/******************************************************
 * Function prototypes --                             *
 ******************************************************/
int         mv_eth_stop(struct net_device *dev);
int         mv_eth_start(struct net_device *dev);
int         mv_eth_change_mtu(struct net_device *dev, int mtu);
int         mv_eth_check_mtu_internals(struct net_device *dev, int mtu);
int         mv_eth_check_mtu_valid(struct net_device *dev, int mtu);

int         mv_eth_set_mac_addr(struct net_device *dev, void *mac);
void        mv_eth_set_multicast_list(struct net_device *dev);
int         mv_eth_open(struct net_device *dev);
int         mv_eth_port_suspend(int port);
int         mv_eth_port_resume(int port);
int         mv_eth_resume_clock(int port);
int         mv_eth_suspend_clock(int port);
int         mv_eth_suspend_internals(struct eth_port *pp);
int         mv_eth_resume_internals(struct eth_port *pp, int mtu);
int         mv_eth_restore_registers(struct eth_port *pp, int mtu);

void        mv_eth_win_init(int port);
int         mv_eth_resume_network_interfaces(struct eth_port *pp);
int         mv_eth_wol_mode_set(int port, int mode);

int	    mv_eth_cpu_txq_mask_set(int port, int cpu, int txqMask);

irqreturn_t mv_eth_isr(int irq, void *dev_id);
int         mv_eth_start_internals(struct eth_port *pp, int mtu);
int         mv_eth_stop_internals(struct eth_port *pp);
int         mv_eth_change_mtu_internals(struct net_device *netdev, int mtu);

int         mv_eth_rx_reset(int port);
int         mv_eth_txp_reset(int port, int txp);
int         mv_eth_txq_clean(int port, int txp, int txq);

MV_STATUS   mv_eth_rx_pkts_coal_set(int port, int rxq, MV_U32 value);
MV_STATUS   mv_eth_rx_time_coal_set(int port, int rxq, MV_U32 value);
MV_STATUS   mv_eth_tx_done_pkts_coal_set(int port, int txp, int txq, MV_U32 value);

struct eth_port     *mv_eth_port_by_id(unsigned int port);
struct net_device   *mv_eth_netdev_by_id(unsigned int idx);
bool                 mv_eth_netdev_find(unsigned int if_index);

void        mv_eth_mac_show(int port);
void        mv_eth_tos_map_show(int port);
int         mv_eth_rxq_tos_map_set(int port, int rxq, unsigned char tos);
int         mv_eth_txq_tos_map_set(int port, int txq, int cpu, unsigned int tos);
int         mv_eth_napi_set_cpu_affinity(int port, int group, int affinity);
int         mv_eth_napi_set_rxq_affinity(int port, int group, int rxq);
void        mv_eth_napi_group_show(int port);

int         mv_eth_rxq_vlan_prio_set(int port, int rxq, unsigned char prio);
void        mv_eth_vlan_prio_show(int port);

void        mv_eth_netdev_print(struct net_device *netdev);
void        mv_eth_status_print(void);
void        mv_eth_port_status_print(unsigned int port);
void        mv_eth_port_stats_print(unsigned int port);
void        mv_eth_pool_status_print(int pool);

void        mv_eth_set_noqueue(struct net_device *dev, int enable);

void        mv_eth_ctrl_hwf(int en);
int         mv_eth_ctrl_recycle(int en);
void        mv_eth_ctrl_txdone(int num);
int         mv_eth_ctrl_tx_mh(int port, u16 mh);
int         mv_eth_ctrl_tx_cmd(int port, u32 cmd);
int         mv_eth_ctrl_txq_cpu_def(int port, int txp, int txq, int cpu);
int         mv_eth_ctrl_txq_mode_get(int port, int txp, int txq, int *rx_port);
int         mv_eth_ctrl_txq_cpu_own(int port, int txp, int txq, int add, int cpu);
int         mv_eth_ctrl_txq_hwf_own(int port, int txp, int txq, int rxp);
int         mv_eth_ctrl_flag(int port, u32 flag, u32 val);
int         mv_eth_ctrl_txq_size_set(int port, int txp, int txq, int value);
int         mv_eth_ctrl_rxq_size_set(int port, int rxq, int value);
int         mv_eth_ctrl_port_buf_num_set(int port, int long_num, int short_num);
int         mv_eth_ctrl_pool_size_set(int pool, int pkt_size);
int         mv_eth_ctrl_set_poll_rx_weight(int port, u32 weight);
int         mv_eth_shared_set(int port, int txp, int txq, int value);
void        mv_eth_tx_desc_print(struct neta_tx_desc *desc);
void        mv_eth_pkt_print(struct eth_pbuf *pkt);
void        mv_eth_rx_desc_print(struct neta_rx_desc *desc);
void        mv_eth_skb_print(struct sk_buff *skb);
void        mv_eth_link_status_print(int port);

#ifdef CONFIG_MV_PON
typedef MV_BOOL(*PONLINKSTATUSPOLLFUNC)(void);		  /* prototype for PON link status polling function */
typedef void   (*PONLINKSTATUSNOTIFYFUNC)(MV_BOOL state); /* prototype for PON link status notification function */

MV_BOOL mv_pon_link_status(void);
void mv_pon_link_state_register(PONLINKSTATUSPOLLFUNC poll_func, PONLINKSTATUSNOTIFYFUNC *notify_func);
void mv_pon_ctrl_omci_type(MV_U16 type);
void mv_pon_ctrl_omci_rx_gh(int en);
void mv_pon_omci_print(void);

#endif /* CONFIG_MV_PON */

#ifdef CONFIG_MV_ETH_TX_SPECIAL
void        mv_eth_tx_special_check_func(int port, int (*func)(int port, struct net_device *dev,
				  struct sk_buff *skb, struct mv_eth_tx_spec *tx_spec_out));
#endif /* CONFIG_MV_ETH_TX_SPECIAL */

#ifdef CONFIG_MV_ETH_RX_SPECIAL
void        mv_eth_rx_special_proc_func(int port, void (*func)(int port, int rxq, struct net_device *dev,
							struct sk_buff *skb, struct neta_rx_desc *rx_desc));
#endif /* CONFIG_MV_ETH_RX_SPECIAL */

int  mv_eth_poll(struct napi_struct *napi, int budget);
void mv_eth_link_event(struct eth_port *pp, int print);

int mv_eth_rx_policy(u32 cause);
int mv_eth_refill(struct eth_port *pp, int rxq,
				struct eth_pbuf *pkt, struct bm_pool *pool, struct neta_rx_desc *rx_desc);
u32 mv_eth_txq_done(struct eth_port *pp, struct tx_queue *txq_ctrl);
u32 mv_eth_tx_done_gbe(struct eth_port *pp, u32 cause_tx_done, int *tx_todo);
u32 mv_eth_tx_done_pon(struct eth_port *pp, int *tx_todo);

#ifdef CONFIG_MV_ETH_RX_DESC_PREFETCH
struct neta_rx_desc *mv_eth_rx_prefetch(struct eth_port *pp,
						MV_NETA_RXQ_CTRL *rx_ctrl, int rx_done, int rx_todo);
#endif /* CONFIG_MV_ETH_RX_DESC_PREFETCH */

#ifdef CONFIG_MV_ETH_BM
void	*mv_eth_bm_pool_create(int pool, int capacity, MV_ULONG *physAddr);
#endif /* CONFIG_MV_ETH_BM */

#if defined(CONFIG_MV_ETH_HWF) && !defined(CONFIG_MV_ETH_BM_CPU)
MV_STATUS mv_eth_hwf_bm_create(int port, int mtuPktSize);
void      mv_hwf_bm_dump(void);
#endif /* CONFIG_MV_ETH_HWF && !CONFIG_MV_ETH_BM_CPU */

#ifdef CONFIG_MV_ETH_L2FW
int         mv_l2fw_init(void);
#endif

#ifdef CONFIG_MV_ETH_NFP
int         mv_eth_nfp_ctrl(struct net_device *dev, int en);
int         mv_eth_nfp_ext_ctrl(struct net_device *dev, int en);
int         mv_eth_nfp_ext_add(struct net_device *dev, int port);
int         mv_eth_nfp_ext_del(struct net_device *dev);
MV_STATUS   mv_eth_nfp(struct eth_port *pp, int rxq, struct neta_rx_desc *rx_desc,
					struct eth_pbuf *pkt, struct bm_pool *pool);
#endif /* CONFIG_MV_ETH_NFP */

#endif /* __mv_netdev_h__ */
