/*
* ***************************************************************************
* Copyright (C) 2015 Marvell International Ltd.
* ***************************************************************************
* This program is free software: you can redistribute it and/or modify it
* under the terms of the GNU General Public License as published by the Free
* Software Foundation, either version 2 of the License, or any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program.  If not, see <http://www.gnu.org/licenses/>.
* ***************************************************************************
*/

#ifndef __mv_pp3_vport_h__
#define __mv_pp3_vport_h__

#include "common/mv_sw_if.h"
#include "mv_pp3_pool.h"

struct pp3_vq;

/* number of virtual ports are stored by driver                         */
/* include internal cpu vports, EMAC vports and external vports         */
/* EMAC vports and external vports are stored according to vport number */
/* internal cpu ports are stored start from (MV_NSS_EXT_PORT_MAX + 1)  */
#define MV_PP3_COMMON_VPORTS_NUM	(MV_PP3_INTERNAL_CPU_PORT_NUM + MV_NSS_EXT_PORT_MAX + 1)
#define MV_PP3_INTERNAL_CPU_PORT_BASE	(MV_NSS_EXT_PORT_MAX + 1)

/************************/
/*   vport structures   */
/************************/

/* Masks used for pp3_emac flags */
#define MV_PP3_EMAC_F_INIT_BIT		0

/* EMAC port structure */
struct pp3_emac {
	int			emac_num;               /* EMAC number */
	struct tasklet_struct	lc_tasklet;             /* Link change tasklet */
	int			lc_irq_num;             /* Link change IRQ number */
	char                    lc_irq_name[15];	/* Link change IRQ name */
	enum mv_port_mode	port_mode;              /* Port mode: RXAUI, SGMII, etc. */
	bool			force_link;             /* Force link or don't force link */
	int			phy_addr;               /* PHY address. -1 if EMAC is not connected to PHY */
	int			mtu;			/* maximum transmission unit */
	unsigned char		l2_options;		/* L2 filtering options */
	unsigned long		flags;
};

struct pp3_cpu_vp_stats {
	unsigned int irq;
	unsigned int irq_err;
	unsigned int rx_err;
	unsigned int rx_csum_l4_err;
	unsigned int rx_csum_l3_err;
	unsigned int napi_sched;
	unsigned int napi_enter;
	unsigned int napi_complete;
	unsigned int rx_netif;
	unsigned int rx_netif_drop;
	unsigned int rx_pkt_calc;
	unsigned int rx_split_pkt;
	unsigned int rx_cfh_pkt;
	unsigned int rx_buf_pkt;
	unsigned int rx_bytes;
	unsigned int rx_drop_calc;
	unsigned int rx_cfh_dummy_calc;
	unsigned int rx_cfh_reassembly_calc;
	unsigned int rx_cfh_invalid_calc;
	unsigned int rx_csum_hw;
	unsigned int rx_csum_sw;
	unsigned int rx_gro;
	unsigned int rx_gro_bytes;
	unsigned int rx_no_pool;
	unsigned int tx_pkt_calc;
	unsigned int tx_bytes;
	unsigned int tx_cfh_pkt;
	unsigned int tx_drop_calc;
	unsigned int tx_no_resource_calc;
	unsigned int tx_csum_hw;
	unsigned int tx_csum_sw;
	unsigned int tx_sg_bytes;
	unsigned int tx_sg_pkts;
	unsigned int tx_sg_err;
	unsigned int tx_sg_frags;
	unsigned int tx_tso_skb;
	unsigned int tx_tso_pkts;
#ifdef CONFIG_MV_PP3_TSO
	unsigned int tx_tso_bytes;
	unsigned int tx_tso_frags;
	unsigned int tx_tso_err;
#endif
	unsigned int txdone;
};

/* CPU port structure */
struct pp3_cpu_port {
	int			cpu_num;                /* CPU number */
	int			irq_num;                /* RX IRQ number */
	int			txdone_todo;		/* number of trasmited buffers that not released yet */
	char			irq_name[15];		/* RX IRQ name */
	struct pp3_cpu		*cpu_ctrl;              /* Pointer to physical CPU control structure */
	struct pp3_cpu_shared	*cpu_shared;		/* Pointer to shared CPU structure (per poort)*/
	struct pp3_cpu_vp_stats	stats;                  /* Pointer to CPU virtual port statistics */
	struct mv_pp3_timer	txdone_timer;		/* TX done - free buffers form linux pool */
	struct napi_struct	napi;			/* NAPI structure */
	int			napi_q_num;		/* number of queues processed by napi */
	int			napi_proc_qs[3][MV_PP3_VQ_NUM];
	int			napi_master_array;	/* list of queues actually processed by napi */
	int			napi_next_array;	/* list of queues arranged for next process loop */
};

/* General Virtual port structure */
struct pp3_vport {
	int			vport;                  /* virtual port number */
	bool			state;			/* UP (1) or DOWN (0) */
	void			*root;			/* pointer to root device */
	int			cos;			/* class of service */
	int			dest_vp;		/* Default egress virtual port number */
	enum mv_nss_port_type	type;                   /* EMAC or CPU */
	int			tx_vqs_num;             /* Number of egress virtual queues */
	struct pp3_vq		*tx_vqs[MV_PP3_VQ_NUM]; /* Array of egress virtual queues */
	int			rx_vqs_num;             /* Number of ingress virtual queues */
	struct pp3_vq		*rx_vqs[MV_PP3_VQ_NUM]; /* Array of ingress virtual queues */
	int			tx_cos_to_vq[MV_PP3_PRIO_NUM]; /* Mapping of CoS value to egress VQ */
	int			rx_cos_to_vq[MV_PP3_PRIO_NUM]; /* Mapping of CoS value to egress VQ */
	union {
		struct pp3_cpu_port  cpu;
		struct pp3_emac      emac;
	} port;
};

/************************/
/*     Global variables */
/************************/
extern struct pp3_vport    **pp3_vports;

/************************/
/*     vport APIs       */
/************************/

int mv_pp3_vports_global_init(struct mv_pp3 *priv, int vports_num);
struct pp3_vport *mv_pp3_vport_alloc(int vp, enum mv_nss_port_type vp_type, int rxqs_num, int txqs_num);
int mv_pp3_cpu_vport_sw_init(struct pp3_vport *vport, struct cpumask *rx_cpus, int cpu);
int mv_pp3_emac_vport_sw_init(struct pp3_vport *vport, int emac, struct mv_mac_data *mac_data);
int mv_pp3_cpu_vport_hw_init(struct pp3_vport *vport);
int mv_pp3_emac_vport_hw_init(struct pp3_vport *vport);
void mv_pp3_vport_delete(struct pp3_vport *vport);
void mv_pp3_cpu_vport_cnt_dump(struct pp3_vport **cpu_vp, int cpu_vp_num);
void mv_pp3_vports_dump(void);
int  mv_pp3_cpu_vport_stats_dump(int vport);
int  mv_pp3_cpu_vport_stats_clear(int vport);
void mv_pp3_cpu_vport_vqs_stats_dump(int vport);
void mv_pp3_vport_fw_stats_dump(int vport);
int mv_pp3_vport_fw_stats_clear(int vport);
int pp3_cpu_vport_fw_set(struct pp3_vport *vport);
int pp3_emac_vport_fw_set(struct pp3_vport *vport, unsigned char *mac);

int mv_pp3_cpu_vport_rx_pkt_coal_set(struct pp3_vport *cpu_vp, int pkts_num);
int mv_pp3_cpu_vport_rx_time_prof_set(struct pp3_vport *cpu_vp, int prof);
int mv_pp3_cpu_vport_rx_time_coal_set(struct pp3_vport *cpu_vp, int usec);
int mv_pp3_egress_vport_shaper_set(struct pp3_vport *emac_vp, struct mv_nss_meter *meter);

int mv_pp3_vport_sysfs_init(struct kobject *pp3_kobj);
int mv_pp3_vport_sysfs_exit(struct kobject *pp3_kobj);

#endif /* __mv_pp3_vport_h__ */
