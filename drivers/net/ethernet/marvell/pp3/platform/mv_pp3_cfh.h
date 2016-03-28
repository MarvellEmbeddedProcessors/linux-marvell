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

#ifndef __mv_pp3_cfh_h__
#define __mv_pp3_cfh_h__

#define MV_PP3_CFH_COMMON_WORDS			(8)	/* CFH common size in words */
#define MV_PP3_CFH_HDR_SIZE			(32)	/* CFH header size in bytes */
#define MV_PP3_CFH_ALIGN_HDR_SIZE		(64)	/* CFH header size aligned to 64 bytes to prevent dummy CFHs */
#define MV_PP3_CFH_MDATA_SIZE			(32)	/* CFH meta data size in bytes */
#define MV_PP3_CFH_PKT_SIZE			(MV_PP3_CFH_HDR_SIZE + MV_PP3_CFH_MDATA_SIZE)
#define MV_PP3_CFH_PAYLOAD_MAX_SIZE		(MV_PP3_CFH_MAX_SIZE - MV_PP3_CFH_PKT_SIZE)
#define MV_PP3_CFH_MAX_SIZE			(128)	/* CFH max size in bytes */
#define MV_PP3_CFH_DG_SIZE			(16)	/* Datagram size in bytes */
#define MV_PP3_CFH_PKT_DG_SIZE			(MV_PP3_CFH_PKT_SIZE/MV_PP3_CFH_DG_SIZE)
#define MV_PP3_CFH_DG_MAX_NUM			(MV_PP3_CFH_MAX_SIZE / MV_PP3_CFH_DG_SIZE)

struct mv_cfh_common {
	u32 plen_order;
	u32 ctrl;
	u32 tag1;
	u32 tag2;
	u32 phys_l;
	u32 vm_bp;
	u32 marker_l;
	u32 l3_l4_info;
};

enum mv_pp3_cfh_mode {
	HMAC_CFH = 0,
	EMAC_CFH,
	CMAC_CFH,
	RADIO_CFH
};

enum mv_pp3_cfh_pp_mode_tx {
	PP_TX_PACKET = 0,
	PP_TX_PACKET_NSS,
	PP_TX_RESERVED,
	PP_TX_MESSAGE
};

enum mv_pp3_cfh_pp_mode_rx {
	PP_RX_PACKET = 0,
	PP_RX_REASEM_PACKET,	/* Reassembly packet */
	PP_RX_INV_PACKET,	/* Invalid packet */
	PP_RX_MESSAGE
};


enum mv_pp3_cfh_l4_info_rx {
	L4_RX_UNKNOWN = 0,
	L4_RX_TCP,
	L4_RX_TCP_CS_ERR,
	L4_RX_UDP,
	L4_RX_UDP_LITE,
	L4_RX_UDP_CS_ERR,
	L4_RX_IGMP,
	L4_RX_OTHER
};

enum mv_pp3_cfh_l4_info_tx {
	L4_TX_TCP = 0,
	L4_TX_UDP
};

enum mv_pp3_cfh_vlan_info {
	VLAN_UNTAGGED = 0,
	VLAN_SINGLE,
	VLAN_DOUBLE,
	VLAN_RESERVED
};

enum mv_pp3_cfh_l2_info {
	L2_UCAST = 0,
	L2_MCAST,
	L2_IP_MCAST,
	L2_BCAST
};

enum mv_pp3_cfh_l3_info_rx {
	L3_RX_UNKNOWN = 0,
	L3_RX_IP4,
	L3_RX_IP4_FRAG,
	L3_RX_IP4_OPT,
	L3_RX_IP4_ERR,
	L3_RX_IP6,
	L3_RX_IP6_EXT,
	L3_RX_ARP
};

enum mv_pp3_cfh_l3_info_tx {
	L3_TX_IP4 = 0,
	L3_TX_IP6
};
enum mv_pp3_cfh_reorder {
	REORD_BYPASS = 0,
	REORD_NEW,
	REOED_FIN,
	REORD_RENEW
};

enum mv_pp3_l4_csum {
	L4_CSUM = 0,
	L4_CSUM_FRAG,
	L4_CSUM_NOT
};

/*------------------------------------------------------*/
/*	CFH - common fileds for packet and message	*/
/*------------------------------------------------------*/

/* word 0 */
#define MV_CFH_PKT_LEN_OFFS		(0)
#define MV_CFH_PKT_LEN_MASK		(0xFFFF)
#define MV_CFH_PKT_LEN_SET(v)		(((v) & MV_CFH_PKT_LEN_MASK) << MV_CFH_PKT_LEN_OFFS)
#define MV_CFH_PKT_LEN_GET(v)		(((v) >> MV_CFH_PKT_LEN_OFFS) & MV_CFH_PKT_LEN_MASK)


#define MV_CFH_REORDER_OFFS		(28)
#define MV_CFH_REORDER_MASK		(0x3)
#define MV_CFH_REORDER_SET(v)		(((v) & MV_CFH_REORDER_MASK) << MV_CFH_REORDER_OFFS)
#define MV_CFH_REORDER_GET(v)		(((v) >> MV_CFH_REORDER_OFFS) & MV_CFH_REORDER_MASK)

#define MV_CFH_DEQ_MODE_BIT_OFFS	(30)	/* 0-packet 1-message */

#define MV_CFH_DEQ_MODE_BIT_SET		(0x1 << MV_CFH_DEQ_MODE_BIT_OFFS)
#define MV_CFH_DEQ_MODE_BIT_GET(v)	(((v) >> MV_CFH_DEQ_MODE_BIT_OFFS) & 0x1)

#define MV_CFH_LAST_BIT_OFFS		(31)
#define MV_CFH_LAST_BIT_SET		(0x1 << MV_CFH_LAST_BIT_OFFS)

/* word 1 */

#define MV_CFH_WR_OFFS			(0)	/* packet recived - Payload byte offset in buffer */
#define MV_CFH_WR_MASK			(0xF)
#define MV_CFH_WR_GET(v)		(((v) >> MV_CFH_WR_OFFS) & MV_CFH_WR_MASK)
#define MV_CFH_WR_RES			(32)   /* 32 bytes resolution for normal (non reassembly) packets */
#define MV_CFH_WR_REASEM_RES		(2)    /* 2 bytes resolution for reassembly packets */

#define MV_CFH_RD_OFFS			(0)	/* packet transmit - Payload byte offset in buffer */
#define MV_CFH_RD_MASK			(0x3FF)
#define MV_CFH_RD_SET(v)		(((v) & MV_CFH_RD_MASK) << MV_CFH_RD_OFFS)

#define MV_CFH_QC_BIT_OFFS		(13)
#define MV_CFH_QC_BIT_SET		(0x1 << MV_CFH_QC_BIT_OFFS)
#define MV_CFH_QC_BIT_GET(v)		(((v) >> MV_CFH_QC_OFFS) & 0x1)

#define MV_CFH_SWQ_OFFS			(4)
#define MV_CFH_SWQ_MASK			(0xFF)
#define MV_CFH_SWQ_GET(v)		(((v) >> MV_CFH_SWQ_OFFS) & MV_CFH_SWQ_MASK)



#define MV_CFH_LEN_OFFS			(16)
#define MV_CFH_LEN_MASK			(0xFF)
#define MV_CFH_LEN_SET(v)		(((v) & MV_CFH_LEN_MASK) << MV_CFH_LEN_OFFS)
#define MV_CFH_LEN_GET(v)		(((v) >> MV_CFH_LEN_OFFS) & MV_CFH_LEN_MASK)

#define MV_CFH_MODE_OFFS		(24)
#define MV_CFH_MODE_MASK		(0x3)
#define MV_CFH_MODE_SET(v)		(((v) & MV_CFH_MODE_MASK) << MV_CFH_MODE_OFFS)


#define MV_CFH_MDATA_BIT_OFFS		(26)
#define MV_CFH_MDATA_BIT_SET		(0x1 << MV_CFH_MDATA_BIT_OFFS)
#define MV_CFH_MDATA_BIT_GET(v)		(((v) >> MV_CFH_MDATA_BIT_OFFS) & 0x1)

#define MV_CFH_PP_MODE_OFFS		(30)
#define MV_CFH_PP_MODE_MASK		(0x3)
#define MV_CFH_PP_MODE_SET(v)		(((v) & MV_CFH_PP_MODE_MASK) << MV_CFH_PP_MODE_OFFS)
#define MV_CFH_PP_MODE_GET(v)		(((v) >> MV_CFH_PP_MODE_OFFS) & MV_CFH_PP_MODE_MASK)

/* CFH word 2 */
#define MV_CFH_INIT_CS_OFSS		(0)
#define MV_CFH_INIT_CS_MASK		(0xFFFF)

#define MV_CFH_HWQ_OFFS			(16)	/* packet transmit - QM Q */
#define MV_CFH_HWQ_MASK			(0xFFF)
#define MV_CFH_HWQ_SET(v)		(((v) & MV_CFH_HWQ_MASK) << MV_CFH_HWQ_OFFS)

#define MV_CFH_ADD_CRC_BIT_OFFS		(28)
#define MV_CFH_ADD_CRC_BIT_SET		(1 << MV_CFH_ADD_CRC_BIT_OFFS)

#define MV_CFH_L2_PAD_BIT_OFFS		(31)
#define MV_CFH_L2_PAD_BIT_SET		(1 << MV_CFH_L2_PAD_BIT_OFFS)

#define MV_CFH_CHAN_ID_OFFS		(16)	/* message transmit - set channel id */
#define MV_CFH_CHAN_ID_MASK		(0xFF)
#define MV_CFH_CHAN_ID_SET(v)		(((v) & MV_CFH_CHAN_ID_MASK) << MV_CFH_CHAN_ID_OFFS)

#define MV_CFH_PTP_QS_OFFS		(26)
#define MV_CFH_PTP_QS_MASK		(1)
#define MV_CFH_PTP_QS_SET(v)		(((v) & MV_CFH_PTP_QS_MASK) << MV_CFH_PTP_QS_OFFS)
#define MV_CFH_PTP_QS_GET(v)		(((v) >> MV_CFH_PTP_QS_OFFS) & 0x1)

#define MV_CFH_PTP_TSE_OFFS		(27)
#define MV_CFH_PTP_TSE_SET		(1 << MV_CFH_PTP_TSE_OFFS)
#define MV_CFH_PTP_TSE_GET(v)		(((v) >> MV_CFH_PTP_TSE_OFFS) & 0x1)

/* CFH word 3 */
#define MV_CFH_PTP_TS_OFF_OFFS		(0)
#define MV_CFH_PTP_TS_OFF_MASK		(0xFF)
#define MV_CFH_PTP_TS_OFF_SET(v)	(((v) & MV_CFH_PTP_TS_OFF_MASK) << MV_CFH_PTP_TS_OFF_OFFS)
#define MV_CFH_PTP_TS_OFF_GET(v)	(((v) >> MV_CFH_PTP_TS_OFF_OFFS) & MV_CFH_PTP_TS_OFF_MASK)

#define MV_CFH_PTP_CS_OFF_OFFS		(8)
#define MV_CFH_PTP_CS_OFF_MASK		(0xFF)
#define MV_CFH_PTP_CS_OFF_SET(v)	(((v) & MV_CFH_PTP_CS_OFF_MASK) << MV_CFH_PTP_CS_OFF_OFFS)
#define MV_CFH_PTP_CS_OFF_GET(v)	(((v) >> MV_CFH_PTP_CS_OFF_OFFS) & MV_CFH_PTP_CS_OFF_MASK)

#define MV_CFH_PTP_CUE_OFFS		(16)
#define MV_CFH_PTP_CUE_MASK		(1)
#define MV_CFH_PTP_CUE_SET(v)		(((v) & MV_CFH_PTP_CUE_MASK) << MV_CFH_PTP_CUE_OFFS)
#define MV_CFH_PTP_CUE_GET(v)		(((v) >> MV_CFH_PTP_CUE_OFFS) & MV_CFH_PTP_CUE_MASK)

#define MV_CFH_PTP_PACT_OFFS		(17)	/* PTP action */
#define MV_CFH_PTP_PACT_MASK		(0xF)
#define MV_CFH_PTP_PACT_SET(v)		(((v) & MV_CFH_PTP_PACT_MASK) << MV_CFH_PTP_PACT_OFFS)
#define MV_CFH_PTP_PACT_GET(v)		(((v) >> MV_CFH_PTP_PACT_OFFS) & MV_CFH_PTP_PACT_MASK)

#define MV_CFH_PTP_WC_OFFS		(24)
#define MV_CFH_PTP_WC_MASK		(1)
#define MV_CFH_PTP_WC_SET(v)		(((v) & MV_CFH_PTP_WC_MASK) << MV_CFH_PTP_WC_OFFS)
#define MV_CFH_PTP_WC_GET(v)		(((v) >> MV_CFH_PTP_WC_OFFS) & MV_CFH_PTP_WC_MASK)

#define MV_CFH_PTP_SEC_OFFS		(30)
#define MV_CFH_PTP_SEC_MASK		(3)
#define MV_CFH_PTP_SEC_SET(v)		(((v) & MV_CFH_PTP_SEC_MASK) << MV_CFH_PTP_SEC_OFFS)
#define MV_CFH_PTP_SEC_GET(v)		(((v) >> MV_CFH_PTP_SEC_OFFS) & MV_CFH_PTP_SEC_MASK)

/* word 4 */
#define MV_CFH_PHYS_L_OFFS		(0)
#define MV_CFH_PHYS_L_MASK		(0xFFFFFFFF)

/* word 5 */
#define MV_CFH_PHYS_H_OFFS		(0x0)
#define MV_CFH_PHYS_H_MASK		(0xFF)
#define MV_CFH_PHYS_H_SET(v)		(((v) & MV_CFH_PHYS_H_MASK) << MV_CFH_PHYS_H_OFFS)
#define MV_CFH_PHYS_H_GET(v)		(((v) >> MV_CFH_PHYS_H_OFFS) & MV_CFH_PHYS_H_MASK)

#define MV_CFH_VMID_OFFS		(16)
#define MV_CFH_VMID_MASK		(0xFF)
#define MV_CFH_VMID_SET(v)		(((v) & MV_CFH_VMID_MASK) << MV_CFH_VMID_OFFS)

#define MV_CFH_BPID_OFFS		(24)
#define MV_CFH_BPID_MASK		(0xFF)
#define MV_CFH_BPID_SET(v)		(((v) & MV_CFH_BPID_MASK) << MV_CFH_BPID_OFFS)
#define MV_CFH_BPID_GET(v)		(((v) >> MV_CFH_BPID_OFFS) & MV_CFH_BPID_MASK)

/* word 6 */
#define MV_CFH_VIRT_L_OFFS		(0)
#define MV_CFH_VIRT_L_MASK		(0xFF)


/* CFH word 7 - received packet parsing*/

#define MV_CFH_VIRT_H_OFFS		(0x0)
#define MV_CFH_VIRT_H_MASK		(0xFF)
#define MV_CFH_VIRT_H_SET(v)		(((v) & MV_CFH_VIRT_H_MASK) << MV_CFH_VIRT_H_OFFS)
#define MV_CFH_VIRT_H_GET(v)		(((v) >> MV_CFH_VIRT_H_OFFS) & MV_CFH_VIRT_H_MASK)

#define MV_CFH_L3_OFFS			(8)
#define MV_CFH_L3_OFFS_MASK		(0x7f)
#define MV_CFH_L3_OFFS_SET(v)		(((v) & MV_CFH_L3_OFFS_MASK) << MV_CFH_L3_OFFS)
#define MV_CFH_L3_OFFS_GET(v)		(((v) >> MV_CFH_L3_OFFS) & MV_CFH_L3_OFFS_MASK)

#define MV_CFH_MACME_BIT_OFFS		(15) /* packet recived */
#define MV_CFH_MACME_BIT_GET(v)		(((v) >> MV_CFH_MACME_BIT_OFFS) & 0x1)

#define MV_CFH_IP_CSUM_BIT_OFFS		(15) /* packet transmit cs calc */
#define MV_CFH_IP_CSUM_DISABLE		(1 << MV_CFH_IP_CSUM_BIT_OFFS)

#define MV_CFH_IPHDR_LEN_OFFS		(16)
#define MV_CFH_IPHDR_LEN_MASK		(0x1F)
#define MV_CFH_IPHDR_LEN_SET(v)		(((v) & MV_CFH_IPHDR_LEN_MASK) << MV_CFH_IPHDR_LEN_OFFS)
#define MV_CFH_IPHDR_LEN_GET(v)		(((v) >> MV_CFH_IPHDR_LEN_OFFS) & MV_CFH_IPHDR_LEN_MASK)

#define MV_CFH_L4_INFO_RX_OFFS		(21)
#define MV_CFH_L4_INFO_RX_MASK		(0x7)
#define MV_CFH_L4_INFO_RX_GET(v)	(((v) >> MV_CFH_L4_INFO_RX_OFFS) & MV_CFH_L4_INFO_RX_MASK)

#define MV_CFH_VLAN_INFO_OFFS		(24)
#define MV_CFH_VLAN_INFO_MASK		(0x3)
#define MV_CFH_VLAN_INFO_GET(v)		(((v) >> MV_CFH_VLAN_INFO_OFFS) & MV_CFH_VLAN_INFO_MASK)

#define MV_CFH_L4_CSUM_OFFS		(26) /* packet transmit cs calc */
#define MV_CFH_L4_CSUM_MASK		(0x3)
#define MV_CFH_L4_CSUM_SET(v)		(((v) & MV_CFH_L4_CSUM_MASK) << MV_CFH_L4_CSUM_OFFS)

#define MV_CFH_MGMT_BIT_OFFS		(26)
#define MV_CFH_MGMT_BIT_GET(v)		(((v) >> MV_CFH_MGMT_BIT_OFFS) & 0x1)

#define MV_CFH_L2_INFO_OFFS		(27)
#define MV_CFH_L2_INFO_MASK		(0x3)
#define MV_CFH_L2_INFO_GET(v)		(((v) >> MV_CFH_L2_INFO_OFFS) & MV_CFH_L2_INFO_MASK)

#define MV_CFH_L3_INFO_RX_OFFS		(29)
#define MV_CFH_L3_INFO_RX_MASK		(0x7)
#define MV_CFH_L3_INFO_RX_GET(v)	(((v) >> MV_CFH_L3_INFO_RX_OFFS) & MV_CFH_L3_INFO_RX_MASK)

#define MV_CFH_L4_INFO_TX_OFFS		(28)
#define MV_CFH_L4_INFO_TX_MASK		(0x3)
#define MV_CFH_L4_INFO_TX_SET(v)	(((v) & MV_CFH_L4_INFO_TX_MASK) << MV_CFH_L4_INFO_TX_OFFS)

#define MV_CFH_L3_INFO_TX_OFFS		(30)
#define MV_CFH_L3_INFO_TX_MASK		(0x3)
#define MV_CFH_L3_INFO_TX_SET(v)	(((v) & MV_CFH_L3_INFO_TX_MASK) << MV_CFH_L3_INFO_TX_OFFS)

#endif /* __mv_pp3_cfh_h__ */




