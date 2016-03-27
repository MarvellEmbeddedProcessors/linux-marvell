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
/*  mv_nss.h */

#ifndef __MV_NSS_DEFS_H__
#define __MV_NSS_DEFS_H__

/* Minimal Ethernet virtual port ID */
#define MV_NSS_ETH_PORT_MIN		(0)
/* Maximum number of Ethernet virtual ports */
#define MV_NSS_ETH_PORT_NUM		(8)
#define MV_NSS_ETH_PORT_MAX		(MV_NSS_ETH_PORT_MIN + MV_NSS_ETH_PORT_NUM - 1)

/* CPU virtual ports - one per CPU */
#define MV_NSS_CPU_PORT_MIN		(10)
#define MV_NSS_CPU_PORT_NUM		(4)
#define MV_NSS_CPU_PORT_MAX		(MV_NSS_CPU_PORT_MIN + MV_NSS_CPU_PORT_NUM - 1)

/* Virtual port IDs - external virtual port (WLAN) */
#define MV_NSS_EXT_PORT_MIN		(16)
#define MV_NSS_EXT_PORT_NUM		(16)
#define MV_NSS_EXT_PORT_MAX		(MV_NSS_EXT_PORT_MIN + MV_NSS_EXT_PORT_NUM - 1)

/* Minimal application defined virtual port ID */
#define MV_NSS_PORT_APP_MIN		(32)
/* Maximum number of application virtual ports */
#define MV_NSS_PORT_APP_NUM		(256 - MV_NSS_PORT_APP_MIN)
#define MV_NSS_PORT_APP_MAX		(MV_NSS_PORT_APP_MIN + MV_NSS_PORT_APP_NUM - 1)

/* Drop virtual port. Packet will be dropped */
#define MV_NSS_PORT_DROP		(0xFFFE)

/* Invalid or no virtual port indication */
#define MV_NSS_PORT_NONE		(0xFFFF)

enum mv_nss_l2_option {
	MV_NSS_L2_UCAST_PROMISC = 0,
	MV_NSS_L2_MCAST_PROMISC = 1,
	MV_NSS_L2_BCAST_ADM = 2,
	MV_NSS_L2_MCAST_ADM = 3,
	MV_NSS_L2_IP_MCAST_LOCAL_ADM = 4,
	MV_NSS_L2_IP_MCAST_NON_LOCAL_ADM = 5,
	MV_NSS_L2_VLAN_UNKNOWN_ADM = 6,
	MV_NSS_L2_OPTION_LAST = 8
};

#define MV_NSS_NON_PROMISC_MODE (1 << MV_NSS_L2_BCAST_ADM)
#define MV_NSS_ALL_MCAST_MODE   (MV_NSS_NON_PROMISC_MODE | (1 << MV_NSS_L2_MCAST_PROMISC))
#define MV_NSS_PROMISC_MODE     (MV_NSS_ALL_MCAST_MODE | (1 << MV_NSS_L2_UCAST_PROMISC))


struct mv_nss_meter {
	bool         enable;
	unsigned int cir;
	unsigned int eir;
	unsigned int cbs;
	unsigned int ebs;
};

struct mv_nss_drop {
	bool           enable;
	unsigned short td;             /* tail drop threshold */
	unsigned short red;            /* random early drop threshold */
};

struct mv_nss_sched {
	unsigned short priority;       /* priority */
	unsigned short weight;         /* weight */
	bool           wrr_enable;
};


struct mv_nss_vq_stats {
	uint64_t pkts;    /* Number of processed packets */
	uint64_t octets;  /* Number of processed octets */
	uint64_t errors;  /* Number of processing errors */
	uint64_t drops;   /* Number of dropped packets */
};

struct mv_nss_vq_advance_stats {
	unsigned int	pkts_fill_lvl;     /* Current queue fill level, packets */
	unsigned int	pkts_fill_lvl_max; /* Maximum queue fill level, packets */
	unsigned int	pkts_fill_lvl_avg; /* Average queue fill level, packets */
	unsigned int	pkts_rate;         /* Average packet arrival rate, pkts/sec */
	unsigned int	bytes_fill_lvl;	   /* Current queue fill level, octets */
	unsigned int	bytes_fill_lvl_max;  /* Maximum queue fill level, octets */
	unsigned int	bytes_fill_lvl_avg;  /* Average queue fill level, octets */
	unsigned int	bytes_rate;          /* Average octets arrival rate, octets/sec */
	unsigned int	time_elapsed;	   /* Time elapsed from the last query, msec */
};

#endif /* __MV_NSS_H__ */
