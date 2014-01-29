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

/* according to num of emac units */
#define MAX_ETH_DEVICES	4

#define MV_PP3_PRIV(dev)	((struct pp3_dev_priv *)(netdev_priv(dev)))

struct pp3_dev_priv {
	int			index;
	struct mv_pp3_port_data *plat_data;
	struct pp3_group	*group_ctrl[CONFIG_NR_CPUS];
	struct net_device	*dev;
	unsigned long		flags;
	unsigned char		emac_map;
};

/* Masks used for pp3_dev_priv flags */
#define MV_ETH_F_STARTED_BIT		0
#define MV_ETH_F_LINK_UP_BIT		1
#define MV_ETH_F_CONNECT_LINUX_BIT	2
#define MV_ETH_F_DBG_RX_BIT		3
#define MV_ETH_F_DBG_TX_BIT		4
#define MV_ETH_F_DBG_DUMP_BIT		5
#define MV_ETH_F_DBG_ISR_BIT		6
#define MV_ETH_F_DBG_POLL_BIT		7
#define MV_ETH_F_DBG_BUFF_HDR_BIT	8


#define MV_ETH_F_STARTED                (1 << MV_ETH_F_STARTED_BIT)
#define MV_ETH_F_LINK_UP                (1 << MV_ETH_F_LINK_UP_BIT)
#define MV_ETH_F_CONNECT_LINUX          (1 << MV_ETH_F_CONNECT_LINUX_BIT)
#define MV_ETH_F_DBG_RX			(1 << MV_ETH_F_DBG_RX_BIT)
#define MV_ETH_F_DBG_TX			(1 << MV_ETH_F_DBG_TX_BIT)
#define MV_ETH_F_DBG_DUMP		(1 << MV_ETH_F_DBG_DUMP_BIT)
#define MV_ETH_F_DBG_ISR		(1 << MV_ETH_F_DBG_ISR_BIT)
#define MV_ETH_F_DBG_POLL		(1 << MV_ETH_F_DBG_POLL_BIT)
#define MV_ETH_F_DBG_BUFF_HDR		(1 << MV_ETH_F_DBG_BUFF_HDR_BIT)



struct pp3_group_stats {
	unsigned int irq;
	unsigned int irq_err;
	unsigned int rx_err;
	unsigned int rx_netif;
	unsigned int rx_drop;
};

struct pp3_group {
	int	rxqs_num;
	int	txqs_num;
	/* Q: use pp3_queue ? */
	struct	pp3_rxq		**rxqs;
	struct	pp3_txq		**txqs;
	struct	pp3_cpu		*cpu_ctrl;
	struct	napi_struct	*napi;
	struct	pp3_bm_pool	*long_pool;
	struct	pp3_bm_pool	*short_pool;
	struct	pp3_bm_pool	*lro_pool;
	struct	pp3_group_stats	stats;
};

struct pp3_cpu {
	int	cpu;
	int	frames_num;
/*
	not sure that pp3_frame is necessary
	meanwhile not defined
	struct	pp3_frame	**frame_ctrl;
*/
	struct	pp3_dev_priv	*dev_priv[MAX_ETH_DEVICES];
	struct	pp3_bm_pool	*tx_linux_pool;
	struct	pp3_queue	*bm_msg_queue;
	struct	pp3_queue	*fw_msg_queue;
};

struct pp3_xq_stats {
	u32 success;
	u32 err;
};


enum  pp3_q_type {
	PP3_Q_TYPE_BM = 0,
	PP3_Q_TYPE_QM = 1
};

struct pp3_rxq {
	int	frame_num;
	int	logic_q;
	int	phys_q;
	enum	pp3_q_type		type;
	struct	mv_pp3_queue_ctrl	*queue;
	struct	pp3_dev_priv		*dev_priv;
	int	pkt_coal;
	int	time_coal;
	struct	pp3_xq_stats		stats;
	/*
	not sure yet about this
	struct	pp3_frame	*frame_ctrl;
	*/
};

struct pp3_txq {
	int	frame_num;
	int	logic_q;
	int	phys_q;
	enum	pp3_q_type		type;
	struct	mv_pp3_queue_ctrl	*queue;
	struct	pp3_dev_priv		*dev_priv;
	struct	pp3_xq_stats		stats;
	/*
	not sure yet about this
	struct	pp3_frame	*frame_ctrl;
	*/
};

struct pp3_queue {
	int frame;
	struct pp3_rxq rxq;
	struct pp3_txq txq;
};
/* TODO define bm_pool */

#endif /* __mv_netdev_structs_h__ */
