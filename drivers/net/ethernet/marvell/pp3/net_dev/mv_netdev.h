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
#include <linux/if_vlan.h>
#include <linux/mv_pp3.h>
#include <net/ip.h>

#include "common/mv_sw_if.h"
#include "platform/mv_pp3.h"
#include "platform/mv_pp3_config.h"
#include "mv_netdev_structs.h"
#include "mv_dev_dbg.h"
#include "hmac/mv_hmac.h"
#include "common/mv_stack.h"
#include "vport/mv_pp3_vport.h"

#define PP3_INTERNAL_DEBUG

#ifdef PP3_INTERNAL_DEBUG
enum mv_dbg_action {
	MV_DBG_ACTION_WARNING = 0,
	MV_DBG_ACTION_STOP,
	MV_DBG_ACTION_PANIC,
	MV_DBG_ACTION_MAX
};

void mv_pp3_internal_debug_init(void);
int mv_pp3_ctrl_internal_debug_set(int action);
const char *mv_pp3_get_internal_debug_str(void);
#endif

#define TOS_TO_DSCP(tos)	((tos) >> 2)

/* Default DSCP to Priority mapping: 0-7 -> 0, 8-15 -> 1, 16-23 -> 2, 56 - 63 -> 7 */
#define DSCP_TO_PRIO(dscp)	(((dscp) >> 3) & 0x7)

#define MV_PP3_PROC_RXQ_INDEX_GET(cpu, ind) (cpu.napi_proc_qs[cpu.napi_master_array][ind])

/*---------------------------------------------------------------------------*/
/*				Function prototypes                          */
/*---------------------------------------------------------------------------*/
int mv_pp3_netdev_global_init(struct mv_pp3 *priv);
int mv_pp3_netdev_close_all(void);
int mv_pp3_netdev_close(struct net_device *dev);
void mv_pp3_netdev_show(struct net_device *dev);
int mv_pp3_dev_num_get(void);
struct pp3_dev_priv *mv_pp3_dev_priv_get(int i);
struct net_device *mv_pp3_vport_dev_get(int vport);
struct net_device *mv_pp3_netdev_init(const char *name, int rx_vqs, int tx_vqs);
struct net_device *mv_pp3_netdev_nic_init(struct device_node *np);
void mv_pp3_netdev_delete(struct net_device *dev);
int mv_pp3_netdev_set_emac_params(struct net_device *dev, struct device_node *np);
int mv_pp3_rx_dg_to_pkts(struct pp3_dev_priv *dev_priv, int dg);
int mv_pp3_rx_pkts_to_dg(struct pp3_dev_priv *dev_priv, int pkts);
const char *mv_pp3_pool_name_get(struct pp3_pool *ppool);
int mv_pp3_rx_hwq_alloc_mode_set(enum mv_hwq_alloc_mode mode);
int mv_pp3_dev_rxqs_set(struct net_device *dev, int rxqs);
int mv_pp3_dev_txqs_set(struct net_device *dev, int txqs);
int mv_pp3_dev_rxvq_num_get(struct net_device *dev, int cpu);
void mv_pp3_dev_init_show(struct net_device *dev);
int mv_pp3_txdone_pkt_coal_set(struct net_device *dev, int pkts_num);
int mv_pp3_txdone_pkt_coal_get(struct net_device *dev, int *pkts_num);
int mv_pp3_txdone_time_coal_set(struct net_device *dev, unsigned int usec);
int mv_pp3_txdone_time_coal_get(struct net_device *dev, unsigned int *usec);
int mv_pp3_recycle_set(struct net_device *dev, int enable);
int mv_pp3_rx_pkt_coal_set(struct net_device *dev, int pkts_num);
int mv_pp3_rx_pkt_coal_get(struct net_device *dev, int *pkts_num);
void mv_pp3_rx_time_coal_profile_set(struct net_device *dev, int profile);
void mv_pp3_rx_time_coal_set(struct net_device *dev, int usec);
int mv_pp3_rx_time_coal_get(struct net_device *dev, int *usec);
int mv_pp3_cpu_affinity_set(struct net_device *dev, int cpu);
int mv_pp3_rx_pkt_mode_set(struct net_device *dev, enum mv_pp3_pkt_mode mode);
int mv_pp3_poll(struct napi_struct *napi, int budget);
struct net_device_stats *mv_pp3_get_stats(struct net_device *dev);
int mv_pp3_dev_open(struct net_device *dev);
int mv_pp3_dev_stop(struct net_device *dev);
int mv_pp3_ingress_vqs_priv_init(struct pp3_dev_priv *dev_priv, enum mv_hwq_alloc_mode rxq_mode);
int mv_pp3_egress_vqs_priv_init(struct pp3_dev_priv *dev_priv);
int mv_pp3_ingress_vqs_delete(struct pp3_dev_priv *dev_priv);
int mv_pp3_egress_vqs_delete(struct pp3_dev_priv *dev_priv);
int mv_pp3_dev_rx_cpus_set(struct net_device *dev, int cpus_mask);
int mv_pp3_dev_cpu_inuse(struct net_device *dev, int cpu);
void mv_pp3_dev_rx_pause(struct net_device *dev, int cos);
void mv_pp3_dev_rx_resume(struct net_device *dev, int cos);
bool mv_pp3_dev_is_valid(struct net_device *dev);
struct pp3_dev_priv *mv_pp3_dev_priv_exist_get(struct net_device *dev);
struct pp3_dev_priv *mv_pp3_dev_priv_ready_get(struct net_device *dev);

#ifdef CONFIG_MV_PP3_SKB_RECYCLE
int mv_pp3_ctrl_nic_skb_recycle(int en);
bool mv_pp3_is_nic_skb_recycle(void);
#endif

#define MV_PP3_RXREFILL_TIMER_USEC_PERIOD (20000)
#define MV_PP3_TXDONE_TIMER_USEC_PERIOD (1000)
#define MV_PP3_BUF_REQUEST_SIZE	(8)

#if 0
#ifdef CONFIG_MV_PP3_SKB_RECYCLE

/* Pool mask, the low 6 bits of CB used to record BPID */
#define MV_PP3_SKB_RECYCLE_POOL_MASK                        (0x3F)
/* GP pool from 8 to 35 */
#define MV_PP3_SKB_RECYCLE_POOL_START                       (8)
#define MV_PP3_SKB_RECYCLE_POOL_END                         (35)
/* SKB recycle magic, indicate the skb can be recycled, here it is the address of skb */
#define MV_PP3_SKB_RECYCLE_MAGIC(skb)                       (((unsigned int)skb) & (~MV_PP3_SKB_RECYCLE_POOL_MASK))
/* Cb to store magic and bpid, the last 4 bytes of cb is to use */
#define MV_PP3_SKB_RECYCLE_CB(skb)                          (*((unsigned int *)(&(skb->cb[sizeof(skb->cb) - 4]))))

/*---------------------------------------------------------------------------*/
/*				Inline functions                             */
/*---------------------------------------------------------------------------*/
/* mv_pp3_skb_recycle_magic_bpid_set, set magic and rx pool id */
static inline void mv_pp3_skb_recycle_magic_bpid_set(struct sk_buff *skb, int bpid)
{
	MV_PP3_SKB_RECYCLE_CB(skb) = MV_PP3_SKB_RECYCLE_MAGIC(skb) | bpid;
}

/* mv_pp3_skb_recycle_bpid_get, get rx pool id */
static inline int mv_pp3_skb_recycle_bpid_get(struct sk_buff *skb)
{
	int bpid;
	unsigned int magic_bpid = MV_PP3_SKB_RECYCLE_CB(skb);

	/* Check skb recycle magic */
	if (MV_PP3_SKB_RECYCLE_MAGIC(skb) != (magic_bpid & ~MV_PP3_SKB_RECYCLE_POOL_MASK))
		return -1;

	bpid = magic_bpid & MV_PP3_SKB_RECYCLE_POOL_MASK;
	/* Check bpid range for A390 */
	if ((bpid < MV_PP3_SKB_RECYCLE_POOL_START) || (bpid > MV_PP3_SKB_RECYCLE_POOL_END))
		return -1;

	return bpid;
}

static inline int mv_pp3_stack_put(struct pp3_cpu *cpu_ctrl, struct sk_buff *skb)
{
	if (mv_stack_is_full(cpu_ctrl->stack)) {
		STAT_ERR(cpu_ctrl->stack_full++);
		return -1;
	}
	mv_stack_push(cpu_ctrl->stack, (unsigned int)skb);
	STAT_DBG(cpu_ctrl->stack_put++);

	return 0;
}

static inline struct sk_buff *mv_pp3_stack_get(struct pp3_cpu *cpu_ctrl)
{
	struct sk_buff *skb = NULL;

	if (mv_stack_index(cpu_ctrl->stack) > 0) {
		STAT_DBG(cpu_ctrl->stack_get++);
		skb = (struct sk_buff *)mv_stack_pop(cpu_ctrl->stack);
	} else
		STAT_ERR(cpu_ctrl->stack_empty++);

	return skb;
}

#endif /* CONFIG_MV_PP3_SKB_RECYCLE */
#endif /* if 0 */

/*  Function prepare CFH QC field for TX L3 checksum calculation offload */
static inline u32 mv_pp3_cfh_tx_l3_csum_offload(bool enable, int l3_offs, int l3_proto, int ip_hdr_len, int *valid)
{
	u32 command;
	*valid = 1;

	command = MV_CFH_L3_OFFS_SET(l3_offs);
	command |= MV_CFH_IPHDR_LEN_SET(ip_hdr_len);

	if (!enable) {
		command |= MV_CFH_IP_CSUM_DISABLE;
		*valid = 0;

	} else if (l3_proto != htons(ETH_P_IP)) {
		/* enable L3 IP6 CS, IP4 supported by default*/
		command |= MV_CFH_L3_INFO_TX_SET(L3_TX_IP6);
		command |= MV_CFH_IP_CSUM_DISABLE;
		*valid = 0;
	}

	return command;
}

/*  Function prepare CFH QC field for TX L4 checksum calculation offload */
static inline u32 mv_pp3_cfh_tx_l4_csum_offload(bool enable, int l4_proto, int *valid)
{
	u32 command = 0;
	*valid = 1;

	if (!enable) {
		command |= MV_CFH_L4_CSUM_SET(L4_CSUM_NOT);
		*valid = 0;

	} else if (l4_proto == IPPROTO_TCP)
		/*L4_TX_TCP by defult*/
		command |= MV_CFH_L4_CSUM_SET(L4_CSUM);

	else if (l4_proto == IPPROTO_UDP)
		command |= (MV_CFH_L4_INFO_TX_SET(L4_TX_UDP) | MV_CFH_L4_CSUM_SET(L4_CSUM));

	else {
		command |= MV_CFH_L4_CSUM_SET(L4_CSUM_NOT);
		*valid = 0;
	}

	return command;
}
/* Function verify CHECKSUM validity and update statistics counters */
static inline bool mv_pp3_rx_csum(struct pp3_vport *cpu_vp, struct mv_cfh_common *cfh, struct sk_buff *skb)
{
	struct net_device *dev = (struct net_device *)cpu_vp->root;

	if (dev->features & NETIF_F_RXCSUM) {
		enum mv_pp3_cfh_l3_info_rx l3_info = MV_CFH_L3_INFO_RX_GET(cfh->l3_l4_info);

		if ((l3_info == L3_RX_IP4) || (l3_info == L3_RX_IP6)) {
			enum mv_pp3_cfh_l4_info_rx l4_info = MV_CFH_L4_INFO_RX_GET(cfh->l3_l4_info);

			if ((l4_info == L4_RX_TCP) || (l4_info == L4_RX_UDP)) {
				STAT_DBG(cpu_vp->port.cpu.stats.rx_csum_hw++);
				return true;

			} else if ((l4_info == L4_RX_TCP_CS_ERR) || (l4_info == L4_RX_UDP_CS_ERR)) {
				/* l4 error found */
				STAT_ERR(cpu_vp->port.cpu.stats.rx_csum_l4_err++;)
			}

		} else if (l3_info == L3_RX_IP4_ERR) {
			/* l3 error found */
			STAT_ERR(cpu_vp->port.cpu.stats.rx_csum_l3_err++;)
		}
	}
	STAT_DBG(cpu_vp->port.cpu.stats.rx_csum_sw++);
	return false;
}

#endif /* __mv_netdev_h__ */

