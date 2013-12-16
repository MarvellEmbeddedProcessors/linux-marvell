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
#include <linux/mv_pp3.h>
#include <net/ip.h>

/*
#include "mvCommon.h"
#include "mvOs.h"
#include "mv802_3.h"
#include "mvStack.h"*/
/*
#include "gbe/mvPp2Gbe.h"
#include "bm/mvBmRegs.h"
#include "bm/mvBm.h"
*/

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

struct tx_queue {
	struct mv_pp3_queue_ctrl	queue_ctrl;
	int			port;
	u8			txp;
	u8			txq;
	int			txq_size;
	int			hwf_size;
/*	struct txq_cpu_ctrl	txq_cpu[CONFIG_NR_CPUS];
	spinlock_t		queue_lock;
	MV_U32			txq_done_pkts_coal;
	unsigned long		flags;*/
};

struct rx_queue {
	struct mv_pp3_queue_ctrl	queue_ctrl;
	int					port;
	int					logic_queue;
	int					frame_num;
	int					queue_num;
	int                 rxq_size;
	unsigned int        rxq_pkts_coal;
	unsigned int        rxq_time_coal;
};

struct eth_port {
	int                      port;
	struct mv_pp3_port_data *plat_data;
	bool                     tagged; /* NONE/MH/DSA/EDSA/VLAN */
	/*MV_PP3_PORT_CTRL    *port_ctrl;*/
	struct rx_queue          *rxq_ctrl; /* array of logical queues */
	int                       rxq_num;
	struct tx_queue          *txq_ctrl;
	/*int                       txp_num;*/
	struct net_device      *dev;
};

struct napi_group_ctrl {
	int			id;
	u8			cpu_mask;
	u16			rxq_mask;
	u32			cause_rx_tx;
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

#define MV_ETH_PRIV(dev)	((struct eth_port *)(netdev_priv(dev)))

#endif /* __mv_netdev_h__ */
