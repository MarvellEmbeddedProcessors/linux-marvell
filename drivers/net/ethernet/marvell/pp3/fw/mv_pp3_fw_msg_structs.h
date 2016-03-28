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
* ***************************************************************************/
#ifndef __mv_pp3_fw_msg_structs_h__
#define __mv_pp3_fw_msg_structs_h__

#include "common/mv_sw_if.h"
#include "platform/mv_pp3_defs.h"
#include "platform/mv_pp3_fw_opcodes.h"

/* Create communication channel between Host and Firmware - MV_FW_MSG_CHAN_SET - 4 bytes */
struct mv_pp3_fw_msg_chan_cfg {
	unsigned char	chan_id;
	unsigned char	hmac_sw_rxq;
	unsigned short	hmac_hw_rxq;
};

#define MV_PP3_VERSION_NAME_SIZE	4

/* Get version message. Version number is represented in x.y.z */
/* MV_FW_VERSION_GET - 8 bytes */
struct mv_pp3_version {
	char            name[MV_PP3_VERSION_NAME_SIZE];
	unsigned char	major_x;
	unsigned char	minor_y;
	unsigned char	local_z;
	unsigned char	debug_d;
};

/* Set/Get EMAC virtual port - MV_FW_EMAC_VPORT_SET/MV_FW_EMAC_VPORT_GET - 16 bytes */
struct mv_pp3_fw_emac_vport {
	unsigned short	vport;          /* virtual port ID of the EMAC */
	unsigned short	mtu;		/* MTU for virtual port - used for MSS and MRU */
	unsigned short  vport_dst;      /* default desctination virtual port */
	unsigned char   state;          /* 0 - disable, 1 - enable */
	unsigned char   cos;            /* default CoS value */
	unsigned char	l2_options;	/* bitmask of options from "enum mv_gnss_l2_option" */
	unsigned char   reserved;
	unsigned char	mac_addr[MV_MAC_ADDR_SIZE];    /* MAC addresses for the port */
};

/* Set/Get CPU virtual port - MV_FW_CPU_VPORT_SET/MV_FW_CPU_VPORT_GET - 8 bytes */
struct mv_pp3_fw_cpu_vport {
	unsigned short	vport;          /* CPU virtual port [10..11] */
	unsigned short  vport_dst;      /* default desctination virtual port */
	unsigned char   state;          /* 0 - disable, 1 - enable */
	unsigned char   cos;            /* default CoS value */
	unsigned char   reserved[2];
};

/* Set/Get internal CPU port - MV_FW_INTERNAL_CPU_PORT_SET/MV_FW_INTERNAL_CPU_PORT_GET - 8 bytes */
struct mv_pp3_fw_internal_cpu_port {
	unsigned short	int_cpu_port;   /* internal CPU port in range [0 .. 31] */
	unsigned char	rx_pkt_mode;	/* value from "enum mv_pp3_pkt_mode" */
	signed char	bm_long_pool;   /* long pool must be valid */
	signed char	bm_short_pool;	/* -1 - short pool is not exist */
	signed char	bm_lro_pool;	/* -1 - LRO pool is not exist */
	unsigned char   reserved[2];
};

/* Set internal virtual port to be used between EMAC/WLAN virtual port and CPU virtual port */
/* MV_FW_CPU_VPORT_MAP - 8 bytes */
struct mv_pp3_fw_cpu_vport_map {
	unsigned short vport;		/* virtual port ID of EMAC/WLAN virtual port */
	unsigned short cpu_vport;	/* CPU virtual port [10, 11] */
	unsigned short int_cpu_port;	/* Internal CPU port to be used [0..31] */
	unsigned short reserved;
};

/* Message to set rx_pkt_mode for virtual port */
/* MV_FW_INTERNAL_CPU_PORT_RX_PKT_MODE_SET - 4 bytes */
struct mv_pp3_fw_vport_rx_pkt_mode {
	unsigned short	int_cpu_port;   /* Internal CPU port i[0..31] */
	unsigned char	rx_pkt_mode;	/* one from mv_pp3_pkt_mode enumerations: */
					/* 0 - minimal CFH, the whole packet is in DRAM buffer */
					/* 1 - maximal CFH, upto 96 bytes of packet in CFH, rest in DRAM buffer */
	unsigned char	reserved;
};

/* Message to map ingress/egress VQ to SWQ/HWQ pair */
/* MV_FW_VQ_MAP_SET / MV_FW_VQ_MAP_GET - 8 bytes */
/* For EMAC virtual port valid vq types are 0 - emac_to_ppc, 1 - ppc_to_emac */
/* For internal CPU port valid vq types are 2 - hmac_to_ppc, 3 - ppc_to_hmac */
struct mv_pp3_fw_vq_map {
	unsigned short  vport;          /* EMAC virtual port / internal CPU port */
	unsigned char   type;           /* for emac virtual port: 0 - emac_to_ppc (not for FW), 1 - ppc_to_emac, */
					/* for internal cpu port: 2 - hmac_to_ppc (not for FW), 3 - ppc_to_hmac */
	unsigned char   vq;             /* virtual queue number: [0..15] */
	unsigned short  hwq;            /* HWQ number */
	unsigned char   swq;            /* SWQ number */
	unsigned char   reserved;
};

/* Map ingress/egress CoS value to VQ per vport */
/* MV_FW_COS_TO_VQ_SET  - 8 bytes */
struct mv_pp3_fw_cos_to_vq {
	unsigned short  vport;          /* EMAC virtual port / internal CPU port */
	unsigned char   type;           /* for emac virtual port: 0 - emac_to_ppc (not for FW), 1 - ppc_to_emac */
					/* for internal cpu port: 2 - hmac_to_ppc (not for FW), 3 - ppc_to_hmac */
	unsigned char   cos;            /* CoS value */
	unsigned char   vq;             /* virtual queue number: [0..15] */
	unsigned char   reserved[3];
};

/* Message to set metering parameters for VQ per vport */
/* MV_FW_VQ_POLICER_SET / MV_FW_VQ_POLICER_GET - 24 bytes */
struct mv_pp3_fw_vq_policer {
	unsigned short  vport;          /* EMAC virtual port / internal CPU port */
	unsigned char   type;           /* for emac virtual port: 0 - emac_to_ppc, 1 - ppc_to_emac */
					/* for internal cpu port: 2 - hmac_to_ppc, 3 - ppc_to_hmac */
	unsigned char   vq;             /* virtual queue number 0 ... 15 */
	unsigned char   enable;         /* 0 - disable, 1 - enable */
	unsigned char   reserved[3];
	unsigned int    cir;            /* committed information rate, Kbps. */
	unsigned int    eir;            /* excess information rate, Kbps. */
	unsigned int    cbs;            /* committed burst size, KB. */
	unsigned int    ebs;            /* excess burst size, KB. */
};

/* BM pool configuration - MV_FW_BM_POOL_SET - 8 bytes */
struct mv_pp3_fw_bm_pool {
	unsigned char   bm_pool_num;
	unsigned char   buf_headroom; /* 32 bytes resolution */
	unsigned short  buf_size;
	unsigned char   pe_size;      /* 0 - one pool element, 1 - two pool elements */
	char            reserved[3];
};

/* Set MAC address to EMAC/WLAN vport structure. */
/* MV_FW_VPORT_MAC_SET - 8 bytes */
struct mv_pp3_fw_vport_mac {
	unsigned short vport;
	unsigned char  mac[MV_MAC_ADDR_SIZE];
};

#define MV_PP3_MAC_ADDR_NUM	6

/* Set/Get list of MAC addresses for EMAC virtual port. */
/* MV_FW_VPORT_MAC_LIST_SET / MV_FW_VPORT_MAC_LIST_GET  */
/* MV_PP3_MAC_ADDR_NUM = 4 -> message size = 28 bytes */
/* MV_PP3_MAC_ADDR_NUM = 6 -> message size = 40 bytes */
/* MV_PP3_MAC_ADDR_NUM = 8 -> message size = 52 bytes */
/* MV_PP3_MAC_ADDR_NUM = 14 -> message size = 88 bytes */
struct mv_pp3_fw_vport_mac_list {
	unsigned short vport;
	unsigned char  mac_addr_list_size;  /* number of valid MAC addresses (MCAST and UCAST) in the mac_addr_list */
	unsigned char  reserved;
	unsigned char  mac_addr_list[MV_PP3_MAC_ADDR_NUM][MV_MAC_ADDR_SIZE]; /* array of MAC addresses for the port */
};

/* Enable / Disable virtual port: MV_FW_VPORT_STATE_SET */
struct mv_pp3_fw_vport_state {
	unsigned short vport;
	unsigned char  state;          /* 0 - disable, 1 - enable */
	unsigned char  reserved;
};

/* Toggle (on/off) l2 option. MV_FW_VPORT_L2_OPTION_SET - 4 bytes */
struct mv_pp3_fw_l2_option {
	unsigned short vport;
	unsigned char  option;   /* one from "enum mv_gnss_l2_option" */
	unsigned char  state;    /* 0 - off, 1 - on */
};

/* Set MTU for virtual port: MV_FW_VPORT_MTU_SET */
struct mv_pp3_fw_vport_mtu {
	unsigned short vport;
	unsigned short mtu; /* MTU for virtual port - used for MSS and MRU */
};

/* Set default destination virtual port: MV_FW_VPORT_DEF_DEST_SET */
struct mv_pp3_fw_vport_def_dest {
	unsigned short vport;
	unsigned short def_dst_vport; /* Default destination virtual port for packets recieved on [vport] */
};

/* Virtual port statistics (6 counters * 8 bytes) */
struct mv_pp3_fw_vport_stat {
	unsigned int rx_packets_high;	/* Number of packets received high */
	unsigned int rx_packets_low;	/* Number of packets received low */
	unsigned int rx_errors_high;	/* Number of errors received high */
	unsigned int rx_errors_low;	/* Number of errors received low */
	unsigned int tx_packets_high;	/* Number of packets transmited high */
	unsigned int tx_packets_low;	/* Number of packets transmitted */
	unsigned int tx_errors_high;	/* Number of errors transmitted high */
	unsigned int tx_errors_low;	/* Number of errors transmitted low */
	unsigned int rx_bytes_high;	/* Number of bytes received high */
	unsigned int rx_bytes_low;	/* Number of bytes received low */
	unsigned int tx_bytes_high;	/* Number of bytes transmitted high */
	unsigned int tx_bytes_low;	/* Number of bytes transmitted low */
};

/* get HWQ statistics: MV_FW_HWQ_STATS_GET */
struct mv_pp3_fw_hwq_stats_get {
	unsigned short hwq;		/* hwq number */
	unsigned char  clear;		/* clear after read */
	unsigned char  reserved;
};

/* HWQ statistics */
struct mv_pp3_fw_hwq_stat {
	unsigned short hwq;		/* HWQ number */
	unsigned short reserved;
	unsigned int hwq_pkt_high;	/* Number of packets enqueue to specific HWQ success */
	unsigned int hwq_pkt_low;	/* Number of packets enqueue to specific HWQ success */
	unsigned int hwq_oct_high;	/* Number of octets enqueue to specific HWQ success */
	unsigned int hwq_oct_low;	/* Number of octets enqueue to specific HWQ success */
	unsigned int hwq_pkt_drop_high;	/* Number of times enqueue to specific HWQ failed (full hwq) */
	unsigned int hwq_pkt_drop_low;	/* Number of times enqueue to specific HWQ failed (full hwq) */
};

/* SWQ statistics */
struct mv_pp3_fw_swq_stat {
	unsigned int swq_enq_cntr_high;	/* Number of times enqueue to specific SWQ success */
	unsigned int swq_enq_cntr_low;	/* Number of times enqueue to specific SWQ success */
	unsigned int swq_enq_err_cntr;	/* Number of times enqueue to specific SWQ failed */
};

/* BM pool statistics */
struct mv_pp3_fw_bm_pool_stat {
	unsigned int bm_alloc_cntr;	/* Number of times Firmware allocate buffer from specific BM pool */
	unsigned int bm_free_cntr;	/* Number of times Firmware free buffer to specific BM pool */
};

/* Message channel statistics */
struct mv_pp3_fw_msg_chan_stat {
	unsigned int msg_request_cntr;	/* Number of requests received by Firmware on specific channel */
	unsigned int msg_request_err;	/* Number of requests that Firmware failed to execute */
	unsigned int msg_reply_cntr;	/* Number of replies sent by Firmware on specific channel */
	unsigned int msg_event_cntr;	/* Number of events sent by Firmware on specific channel */
};


#define MV_PP3_PPN_MEM_BUFS	(16)
/* MV_FW_MEM_REQ_GET request message */
struct mv_pp3_fw_mem_get {
	unsigned int size;		/* requested memory size in kbytes */
};

/* MV_FW_MEM_REQ_SET response message */
struct mv_pp3_fw_mem_set {
	unsigned short buf_num;		/* number of allocated buffers */
	unsigned short size;		/* single buffer memory size in kbytes */
	unsigned int buffer[MV_PP3_PPN_MEM_BUFS];	/* pointers to allocated buffers */
};


/* MV_FW_RESET_STATISTICS */
enum mv_fw_counter_types {

	MV_PP3_FW_ALL_STAT = 0,
	MV_PP3_FW_VPORT_STAT,
	MV_PP3_FW_HWQ_STAT,
	MV_PP3_FW_SWQ_STAT,
	MV_PP3_FW_BM_POOL_STAT,
	MV_PP3_FW_CHANNEL_STAT,
};

struct mv_pp3_fw_reset_stat {
	unsigned char type;		/* counter type to reset */
	unsigned char reserved;
	unsigned short index;		/* counter index according to type */
};

/* MV_FW_LINK_CHANGE_NOTE message */
/* for speed:
	MV_PORT_SPEED_UNKNOWN	= 0,
	MV_PORT_SPEED_10	= 1,
	MV_PORT_SPEED_100	= 2,
	MV_PORT_SPEED_1000	= 3,
	MV_PORT_SPEED_2000	= 4,
	MV_PORT_SPEED_10000	= 5

   for duplex:
	MV_PORT_DUPLEX_UNKNOWN	= 0,
	MV_PORT_DUPLEX_HALF	= 1,
	MV_PORT_DUPLEX_FULL	= 2
*/
struct mv_pp3_fw_link_change_note {
	unsigned char emac_num;
	unsigned char link_status;	/* 0 - link down, 1 - link up */
	unsigned char speed;
	unsigned char duplex;
};

#endif /* __mv_pp3_fw_msg_structs_h__ */
