/*#include "mvCopyright.h"*/

#ifndef __mvFwIf_h__
#define __mvFwIf_h__

#define MV_CFH_MSG_OFFS (32)  /* bytes */
#define MV_CFH_MSG_DG_OFFS (MV_CFH_MSG_OFFS/MV_PP3_HMAC_DG_SIZE)  /* datagramm */

/* CFH modes */
enum mv_pp3_cfh_mode {
	CMAC_CFH = 0,
	HMAC_CFH,
	EMAC_CFH,
	RADIO_CFH
};

#define MV_PP3_MCG_PP_MODE_OFFS		0
#define MV_PP3_MSG_PP_MODE_GET(cfh)	((cfh->cfh_format >> MV_PP3_MCG_PP_MODE_OFFS) & 3)
#define MV_PP3_MCG_ACK_OFFS		2
#define MV_PP3_MSG_ACK_GET(cfh)		((cfh->cfh_format >> MV_PP3_MCG_ACK_OFFS) & 1)
#define MV_PP3_MCG_CFH_MODE_OFFS	7
#define MV_PP3_MSG_CFH_MODE_GET(cfh)	((cfh->cfh_format >> MV_PP3_MCG_CFH_MODE_OFFS) & 3)
#define MV_PP3_MCG_CHAN_ID_OFFS		16

/* CFH processing modes */
enum mv_pp3_cfh_pp_mode {
	PP_PACKET_0 = 0,
	RESERVED_1,
	RESERVED_2,
	PP_MESSAGE
};

/* Host to FW message types */
enum mv_pp3_h2f_msg_type {
	H2F_NO_ACK_REQ = 0,
	H2F_ACK_REPLY_REQ
};

/* FW to Host message type */
enum mv_pp3_f2h_msg_type {
	F2H_PPN_EVENT = 0,
	F2H_ACK_REPLY
};

/* CFH message data structure */
struct host_fw_cfh_msg {
	unsigned short msg_opcode;	/* one from MV_CFH_MSG_TYPE enum */
	unsigned short msg_size;	/* size of the message (including header and data) in bytes */
	unsigned short msg_id;		/* message serial number */
	unsigned char  rc;		/* acknowledge or reply */
	unsigned char  type;		/* message type bits [7:6],  */
	unsigned char  message[1];
};

/* buffer extention header structure */
struct host_fw_header_ext {
	unsigned short in_buf_size;
	unsigned short out_buf_size;
	unsigned int   in_buf;
	unsigned int   out_buf;
};

/* CFH message opcode */
enum mv_pp3_fw_nic_msg_opcode {

	MV_IDLE_MSG = 0,
	MV_FW_MSG_CHAN_CFG,
	MV_FW_EMAC_VPORT_ADD,
	MV_FW_HMAC_SW_RXQ_SET,
	MV_FW_BM_POOL_SET,
	MV_FW_RSS_PROFILE_SET,
	MV_FW_QOS_PROFILE_SET,
	MV_FW_UCAST_MAC_SET,
	MV_FW_MCAST_MAC_ADD,
	MV_FW_MCAST_MAC_DEL,
	MV_FW_L2_FILTER_SET
};


enum mv_pp3_fw_l2_fltr_mode {
	L2_FLTR_DROP_ALL = 0,
	L2_FLTR_NON_PROMISC,
	L2_FLTR_ALL_MC,
	L2_FLTR_PROMISC
};

/* Create communication channel between Host and Firmware - MV_FW_MSG_CHAN_CFG - 8 bytes */
struct mv_pp3_msg_chan_cfg {
	unsigned char	chan_id;
	unsigned char	hmac_sw_rxq;
	unsigned short	hmac_hw_rxq;
	unsigned char	bm_pool_id;
	unsigned short	pool_buf_size;
	unsigned char	buf_headroom;
};

/* Create virtual port for all EMACs - MV_FW_EMAC_VPORT_ADD - 26 bytes */
struct mv_pp3_fw_vp_create {
	unsigned char		emac_vport_id;
	unsigned char		rss_profile_id;   /* 0 - RSS disable, 1-4 - RSS profle ID */
	unsigned char		qos_profile_id;   /* 0 - QOS disable, 1-4 - QOS profle ID */
	unsigned char		hmac_sw_rxq_base; /* HMAC_SW_RXQ base when no RSS applied */
	unsigned short		hmac_hw_rxq_base;
	unsigned short		emac_hw_rxq;
	unsigned char		ucast_mac[6];
	unsigned char		l2_filter_mode;   /* one from enum mv_pp3_fw_l2_fltr_mode */
	unsigned char		reserved;
	unsigned short		tpid_first[2];
	unsigned short		tpid_second[2];
	unsigned short		mtu;
};

/* HMAC out queue configuration message definition - MV_FW_HMAC_SW_RXQ_SET - 4 bytes */
struct mv_pp3_fw_hmac_rxq_cfg {
	unsigned char	hmac_sw_rxq;
	char		bm_pool_short;  /* -1 - short pool is not exist */
	unsigned char	bm_long_pool;
	char		bm_lro_pool;    /* -1 - LRO pool is not exist, LRO can't be enabled */
};

/* BM pool configuration - MV_FW_BM_POOL_SET - 4 bytes */
struct mv_pp3_fw_bmp_cfg {
	unsigned char bm_pool_num;
	unsigned char buf_headroom;
	unsigned short buf_size;
};

/* RSS profile configuration - MV_FW_RSS_PROFILE_SET - 18 bytes */
enum mv_pp3_fw_rss_hash_types {
	MV_PP3_RSS_HASH_TYPE_DISABLED = 0,
	MV_PP3_RSS_HASH_5T_2T_SADA,
	MV_PP3_RSS_HASH_SADA_ALL
};

struct mv_pp3_fw_rss_cfg {
	unsigned char rss_profile;
	unsigned char hash_type;		/* one from mv_pp3_fw_rss_hash_types */
	unsigned char hmac_sw_rxq_base[16];	/* HMAC RXQ base for each HASH value */
};

/* QoS profile configuration - MV_FW_QOS_PROFILE_SET - 90 bytes */
enum mv_pp3_fw_qos_modes {
	MV_PP3_QOS_MODE_DISABLED = 0,
	MV_PP3_QOS_DSCP_VPRIO,
	MV_PP3_QOS_VPRIO_DSCP,
	MV_PP3_QOS_USER_DEFINED
};

struct mv_pp3_fw_qos_cfg {
	unsigned char qos_profile;
	unsigned char qos_mode;			/* one from mv_pp3_fw_qos_modes */
	unsigned char dscp_prio_map[64];	/* priority for each one of 64 DSCP values */
	unsigned char vprio_prio_map[8];	/* priority for each one of 8 802.3p values */
	unsigned char prio_hwq_update[8];	/* value to be added to HMAC_HW_RXQ base per packet prio */
	unsigned char prio_swq_update[8];	/* value to be added to HMAC_SW_RXQ base per packet prio */
};


#endif /* __mvFwIf_h__ */
