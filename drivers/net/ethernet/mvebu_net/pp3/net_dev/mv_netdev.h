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

/* TODO remove next lines */
#define MV_PP3_BM_POOLS 20
#define MV_PP3_EMACS	5
#define MV_PP3_FRAMES	4
#define MV_ETH_MH_SIZE  2

#define MV_PP3_EMAC_BASE(_emac_) (0x000CA000 + (0x1000 * (_emac_)))
#define MV_PP3_POOL_INIT_TIMEOUT_MSEC	1000
#define BM_DRAM_POOL_CAPACITY		1024
#define BM_GPM_POOL_CAPACITY		1024
#define MV_PP3_LINUX_POOL_SIZE		1024
#define MV_PP3_LONG_POOL_SIZE		1024
#define MV_PP3_SHORT_POOL_SIZE		1024
#define MV_PP3_LRO_POOL_SIZE		1024
#define MV_PP3_GPM_POOL_0		0
#define MV_PP3_GPM_POOL_1		1
#define MV_PP3_DRAM_POOL_0		2
#define MV_PP3_DRAM_POOL_1		3
#define MV_PP3_FRM_TIME_COAL_NUM	2
#define MV_PP3_FRM_TIME_COAL_0		64
#define MV_PP3_RXQ_TIME_COAL_DEF_PROF	0
#define MV_PP3_TXQ_TIME_COAL_DEF_PROF	0
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

#endif /* __mv_netdev_h__ */

