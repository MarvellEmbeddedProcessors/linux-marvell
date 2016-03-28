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

#ifndef __mv_netdev_structs_h__
#define __mv_netdev_structs_h__

#include <linux/interrupt.h>
#include <net/gnss/mv_nss_defs.h>


#include "common/mv_sw_if.h"
#include "platform/mv_pp3.h"

struct pp3_dev_rss {
	int			rss_profile;
	int			def_cpu;
	enum mv_hash_type	l2_hash;
	enum mv_hash_type	l3_hash;
	enum mv_hash_type	l4_hash;
	int			hash_weights[CONFIG_NR_CPUS];
};

struct pp3_dev_qos {
	int			qos_profile;
	enum mv_qos_mode	qos_mode;
	int			rx_prio_to_vq[MV_PP3_PRIO_NUM];
	int			rx_dscp_to_vq[MV_DSCP_NUM];
	int			tx_prio_to_vq[MV_PP3_PRIO_NUM];
	int			tx_dscp_to_vq[MV_DSCP_NUM];
};
/*---------------------------------------------------------------------------*/

/* Masks used for pp3_dev_priv flags */
#define MV_PP3_F_DBG_RX_BIT		0
#define MV_PP3_F_DBG_TX_BIT		1
#define MV_PP3_F_DBG_ISR_BIT		2
#define MV_PP3_F_DBG_POLL_BIT		3
#define MV_PP3_F_INIT_BIT		4
#define MV_PP3_F_FP_BIT			5
#define MV_PP3_F_DBG_SG_BIT		6
#define MV_PP3_F_SHARED_POOLS_BIT	7
#define MV_PP3_F_IF_UP_BIT		8
#define MV_PP3_F_IF_LINK_UP_BIT		9
#define MV_PP3_F_MAC_CONNECT_BIT	10

#define MV_PP3_F_DBG_RX			(1 << MV_PP3_F_DBG_RX_BIT)
#define MV_PP3_F_DBG_TX			(1 << MV_PP3_F_DBG_TX_BIT)
#define MV_PP3_F_DBG_ISR		(1 << MV_PP3_F_DBG_ISR_BIT)
#define MV_PP3_F_DBG_POLL		(1 << MV_PP3_F_DBG_POLL_BIT)
#define MV_PP3_F_INIT			(1 << MV_PP3_F_INIT_BIT)
#define MV_PP3_F_FP			(1 << MV_PP3_F_FP_BIT)
#define MV_PP3_F_DBG_SG			(1 << MV_PP3_F_DBG_SG_BIT)
#define MV_PP3_F_IF_UP			(1 << MV_PP3_F_IF_UP_BIT)
#define MV_PP3_F_IF_LINK_UP		(1 << MV_PP3_F_IF_LINK_UP_BIT)
#define MV_PP3_F_MAC_CONNECT		(1 << MV_PP3_F_MAC_CONNECT_BIT)

#define MV_PP3_PRIV(dev)		((struct pp3_dev_priv *)(netdev_priv(dev)))
#define MV_PP3_VPORT_DEV(vport)		((struct net_device *)(vport->root))
#define MV_PP3_VPORT_DEV_PRIV(vport)	(MV_PP3_PRIV(MV_PP3_VPORT_DEV(vport)))

#define DEV_PRIV_STATS(dev_priv, cpu)	per_cpu_ptr((dev_priv)->dev_stats, (cpu))
/* counters below used to update net_device statistics and should not zeroed */
struct pp3_netdev_stats {
	unsigned int rx_pkt_dev;
	unsigned int tx_pkt_dev;
	unsigned int rx_bytes_dev;
	unsigned int tx_bytes_dev;
	unsigned int rx_drop_dev;
	unsigned int tx_drop_dev;
	unsigned int rx_err_dev;
};

struct pp3_ptp_desc; /* private PTP descriptor */

/* PP3 driver private information attached to network device */
struct pp3_dev_priv {
	int			id;           /* ID taken from FDT or equal to external virtual port id */
	struct mv_mac_data	mac_data;     /* EMAC data get from FDT file */
	struct pp3_vport	*cpu_vp[CONFIG_NR_CPUS]; /* CPU virtual ports (per CPU) */
	struct pp3_vport	*vport;       /* EMAC/External virtual port (single) */
	struct pp3_cpu_shared   *cpu_shared;  /* Pointer to shared CPUs structure (per port)*/
	struct net_device	*dev;         /* pointer to network device */
	struct list_head	mac_list;     /* Shadow list of MAC addresses */
	struct pp3_ptp_desc	*ptp_desc; /* private PTP descriptor */
	int			mac_list_size;/* number of MAC addreses in list */
	unsigned long		flags;        /* PP3 driver flags used for net_device */
	int			rxqs_per_cpu; /* Number of RXQs per interface per CPU */
	int			txqs_per_cpu; /* Number of TXQs per interface per CPU */
	int			rxq_capacity; /* RXQ maximum capacity [pkts] - allocation size */
	int			txq_capacity; /* TXQ maximum capacity [pkts] - allocation size */
	struct cpumask		rx_cpus;      /* CPUs which can RX on this network interface */
	int			rx_pkt_coal;  /* RX coalescing [pkts] for this device */
	int			rx_time_coal; /* RX time coalescing [usec] for this device */
	int			rx_time_prof; /* RX time coalescing profile this device connected to */
	int			tx_done_pkt_coal; /* TX Done coalescing [pkts] for this device */
	int			tx_done_time_coal; /* TX Done coalescing [usec] for this device */
	struct pp3_netdev_stats __percpu *dev_stats;
};
/*---------------------------------------------------------------------------*/
#endif /* __mv_netdev_structs_h__ */

