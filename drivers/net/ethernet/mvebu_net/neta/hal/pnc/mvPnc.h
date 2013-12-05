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
Marvell Commercial License Option

If you received this File from Marvell and you have entered into a commercial
license agreement (a "Commercial License") with Marvell, the File is licensed
to you under the terms of the applicable Commercial License.

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
********************************************************************************
Marvell BSD License Option

If you received this File from Marvell, you may opt to use, redistribute and/or
modify this File under the following licensing terms.
Redistribution and use in source and binary forms, with or without modification,
are permitted provided that the following conditions are met:

    *   Redistributions of source code must retain the above copyright notice,
	this list of conditions and the following disclaimer.

    *   Redistributions in binary form must reproduce the above copyright
	notice, this list of conditions and the following disclaimer in the
	documentation and/or other materials provided with the distribution.

    *   Neither the name of Marvell nor the names of its contributors may be
	used to endorse or promote products derived from this software without
	specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

*******************************************************************************/

#ifndef __MV_PNC_H__
#define __MV_PNC_H__


#include "mvTcam.h"

/*
 * Errors
 */
#define ERR_ON_OOR(cond) if (cond) { mvOsPrintf("%s: out of range\n", __func__);  return PNC_ERR_OOR; }
#define WARN_ON_OOR(cond) if (cond) { mvOsPrintf("%s: out of range\n", __func__);  return; }
#define WARN_ON_OOM(cond) if (cond) { mvOsPrintf("%s: out of memory\n", __func__); return NULL; }

 /*
 * Errors assigment
 */
#define PNC_ERR_OOR			1			/* out of range error */
#define PNC_ERR_INV			1			/* invalid parameter */

 /* Result info bits assigment */
#define RI_DROP	            (BIT0)		/* drop */

#define RI_L4_OFFS     	    1
#define RI_L4_MASK     	    (3 << RI_L4_OFFS)
#define RI_L4_TCP           (0 << RI_L4_OFFS)
#define RI_L4_UDP           (1 << RI_L4_OFFS)
#define RI_L4_UN            (2 << RI_L4_OFFS)

#define RI_L3_OFFS     	    3
#define RI_L3_MASK     	    (7 << RI_L3_OFFS)
#define RI_L3_UN            (0 << RI_L3_OFFS)
#define RI_L3_IP6           (1 << RI_L3_OFFS)
#define RI_L3_IP4_FRAG      (2 << RI_L3_OFFS)
#define RI_L3_IP4           (3 << RI_L3_OFFS)
#define RI_L3_IP4_FRAG_F    (6 << RI_L3_OFFS)

#define RI_MCAST_OFFS       6
#define RI_MCAST_MASK       (3 << RI_MCAST_OFFS)
#define RI_MCAST_SPEC       (0 << RI_MCAST_OFFS)
#define RI_MCAST_PNC_SPEC   (1 << RI_MCAST_OFFS)
#define RI_MCAST_OTHER      (2 << RI_MCAST_OFFS)
#define RI_MCAST_PNC_OTHER  (3 << RI_MCAST_OFFS)
#define RI_MCAST_PNC_ONLY   (4 << RI_MCAST_OFFS)

#define RI_DA_MC            (BIT10)	/* multicast */
#define RI_DA_BC            (BIT11)	/* broadcast */
#define RI_DA_ME            (BIT12)	/* unicast */
#define RI_PPPOE            (BIT13)	/* PPPoE */
/* BITS 14-17 are reserved for setting MH field in HWF packets (TXz_MH_reg_1-15) */
#define RI_L3_FLOW          (BIT18)     /* L3 flow is found */
#define RI_VLAN             (BIT19)     /* VLAN */
#define RI_RX_SPECIAL       (BIT20)     /* Packet for special RX processing */

 /* Additional info bits assigment */
#define AI_DONE_BIT         0
#define AI_DONE_MASK        (1 << AI_DONE_BIT)

/* PnC result info */
#define NETA_PNC_VLAN       (RI_VLAN  >> 9)
#define NETA_PNC_DA_MC      (RI_DA_MC >> 9)
#define NETA_PNC_DA_BC      (RI_DA_BC >> 9)
#define NETA_PNC_DA_UC      (RI_DA_ME >> 9)
#define NETA_PNC_PPPOE      (RI_PPPOE >> 9)
#define NETA_PNC_RX_SPECIAL (RI_RX_SPECIAL >> 9)

/*---------------------------------------------------------------------------*/

MV_STATUS   mvPncInit(MV_U8 *pncVirtBase);

#ifdef CONFIG_MV_ETH_PNC
/*
 * TCAM topology definition.
 * The TCAM is divided into sections per protocol encapsulation.
 * Usually each section is designed to be to a lookup.
 * Change sizes of sections according to the target product.
 */
enum {
	/* MAC Lookup including Marvell/PON header */
	TE_MH,         		/* Match marvell header */
	TE_MAC_BC,	        /* broadcast */
	TE_MAC_FLOW_CTRL,   /* Flow Control PAUSE frames */
	TE_MAC_MC_ALL,      /* first multicast entry (always reserved for all MCASTs) */
	TE_MAC_MC_L = TE_MAC_MC_ALL + CONFIG_MV_ETH_PNC_MCAST_NUM,    /* last multicast entry */
	TE_MAC_ME,	        /* mac to me per port */
	TE_MAC_ME_END = TE_MAC_ME + CONFIG_MV_ETH_PORTS_NUM - 1,
	TE_MAC_EOF,

    /* VLAN Lookup */
#if (CONFIG_MV_ETH_PNC_VLAN_PRIO > 0)
	TE_VLAN_PRIO,
	TE_VLAN_PRIO_END = TE_VLAN_PRIO + CONFIG_MV_ETH_PNC_VLAN_PRIO,
#endif /* (CONFIG_MV_ETH_PNC_VLAN_PRIO > 0) */
	TE_VLAN_EOF,

    /* Ethertype Lookup */
	TE_ETYPE_ARP,
	TE_ETYPE_IP4,
	TE_ETYPE_IP6,
	TE_PPPOE_IP4,
	TE_PPPOE_IP6,
	TE_ETYPE,	/* custom ethertype */
	TE_ETYPE_EOF = TE_ETYPE + CONFIG_MV_ETH_PNC_ETYPE,

	/* IP4 Lookup */
#if (CONFIG_MV_ETH_PNC_DSCP_PRIO > 0)
	TE_IP4_DSCP,
	TE_IP4_DSCP_END = TE_IP4_DSCP + CONFIG_MV_ETH_PNC_DSCP_PRIO - 1,
#endif /* CONFIG_MV_ETH_PNC_DSCP_PRIO > 0 */

	TE_IP4_TCP,
	TE_IP4_TCP_FRAG,
	TE_IP4_UDP,
	TE_IP4_UDP_FRAG,
	TE_IP4_IGMP,
	TE_IP4_ESP,
	TE_IP4_EOF,

	/* IP6 Lookup */
	TE_IP6_TCP,
	TE_IP6_UDP,
	TE_IP6_EOF,

#ifdef CONFIG_MV_ETH_PNC_L3_FLOW
	/* Session Lookup for IPv4 and IPv6 */
	TE_FLOW_L3,
	TE_FLOW_L3_END = TE_FLOW_L3 + CONFIG_MV_ETH_PNC_L3_FLOW_LINES - 1,
	TE_FLOW_IP4_EOF,
	TE_FLOW_IP6_A_EOF,
	TE_FLOW_IP6_B_EOF,
#else
	TE_L4_EOF,
#endif /* CONFIG_MV_ETH_PNC_L3_FLOW */

#ifdef CONFIG_MV_ETH_PNC_WOL
	TE_WOL,
	TE_WOL_EOF = CONFIG_MV_PNC_TCAM_LINES - 1,
#endif /* CONFIG_MV_ETH_PNC_WOL */
};


enum {
	TCAM_LU_MAC,
	TCAM_LU_L2,
	TCAM_LU_IP4,
	TCAM_LU_IP6,
#ifdef CONFIG_MV_ETH_PNC_L3_FLOW
	TCAM_LU_FLOW_IP4,
	TCAM_LU_FLOW_IP6_A,
	TCAM_LU_FLOW_IP6_B,
#else
	TCAM_LU_L4,
#endif /* CONFIG_MV_ETH_PNC_L3_FLOW */

#ifdef CONFIG_MV_ETH_PNC_WOL
	TCAM_LU_WOL,
#endif /* CONFIG_MV_ETH_PNC_WOL */
};

/*
 * Pre-defined FlowId assigment
 */
#define FLOWID_EOF_LU_MAC 			0xFFF0
#define FLOWID_EOF_LU_L2			0xFFF2
#define FLOWID_EOF_LU_IP4			0xFFF4
#define FLOWID_EOF_LU_IP6			0xFFF6
#define FLOWID_EOF_LU_L4			0xFFF7
#define FLOWID_EOF_LU_FLOW_IP4		0xFFF8
#define FLOWID_EOF_LU_FLOW_IP6_A	0xFFFA
#define FLOWID_EOF_LU_FLOW_IP6_B	0xFFFB

#define FLOWID_CTRL_FULL_MASK       FLOW_CTRL_MASK
#define FLOWID_CTRL_LOW_HALF_MASK   FLOW_CTRL_HALF_MASK
#define FLOWID_CTRL_HIGH_HALF_MASK  (FLOW_CTRL_MASK & ~FLOW_CTRL_HALF_MASK)

/************ FlowID field detalization for HWF support ***********************/

/* TXP for HWF: 0 - no HWF, 1 - Giga0, 2 - Giga1, 3..10 PON - TCONTs 0..7 */
#define PNC_FLOWID_HWF_TXP_OFFS    24
#define PNC_FLOWID_HWF_TXP_MASK    (0xF << PNC_FLOWID_HWF_TXP_OFFS)

#define PNC_FLOWID_HWF_GEM_OFFS    12
#define PNC_FLOWID_HWF_GEM_MASK    (0xFFF << PNC_FLOWID_HWF_GEM_OFFS)

#define PNC_FLOWID_HWF_MOD_OFFS    0
#define PNC_FLOWID_HWF_MOD_MASK    (0x3FF << PNC_FLOWID_HWF_MOD_OFFS)

#define PNC_FLOWID_IS_HWF(flowid)	(((flowid) & PNC_FLOWID_HWF_TXP_MASK) != 0)
/*---------------------------------------------------------------------------*/

/*
 * Export API
 */

int pnc_default_init(void);

int pnc_te_del(unsigned int tid);

struct tcam_entry *pnc_tcam_entry_get(int tid);

int          pnc_eth_port_map(int eth_port);
int          pnc_port_map(int pnc_port);

unsigned int pnc_port_mask_update(unsigned int mask, int eth_port, int add);
unsigned int pnc_port_mask(int eth_port);
int          pnc_tcam_port_update(int tid, int eth_port, int add);


/* Set number of Rx queues */
void pnc_rxq_max(int rxq_max);

/* Assign Rx queue to a protocol */
int pnc_rxq_proto(unsigned int proto, unsigned int rxq);

/* Get availible range on section */
int pnc_rule_range(int sec, int *first, int *last);

/* Get section for specific rule */
int pnc_rule_sec(int tid);

/* Delete rule */
int pnc_rule_del(int tid);

/* Set MAC address of a port, or NULL for promiscuous */
int pnc_mac_me(unsigned int port, unsigned char *mac, int rxq);

/* Set Multicast MAC address to be accepted on the port */
int pnc_mcast_me(unsigned int port, unsigned char *mac);

/* Set VLAN priority entry */
int pnc_vlan_prio_set(int port, int prio, int rxq);
void pnc_vlan_prio_show(int port);

/* match arp */
void pnc_etype_arp(int rxq);

/* TCP/IP header parsing for fragmentation and L4 offset.  */
void pnc_ip4_tcp(int rxq);

/* IPv4/UDP header parsing for fragmentation and L4 offset. */
void pnc_ip4_udp(int rxq);

/* Enable / Disable accept ALL Multicast */
int pnc_mcast_all(unsigned int port, int en);

void    pnc_mac_show(void);

#ifdef CONFIG_MV_ETH_PNC_L3_FLOW
int pnc_ip4_2tuple_rxq(unsigned int eth_port, unsigned int sip, unsigned int dip, int rxq);
int pnc_ip4_5tuple_rxq(unsigned int eth_port, unsigned int sip, unsigned int dip, unsigned int ports,
						unsigned int proto, int rxq);
int pnc_rxq_map_dump(void);
#endif /* CONFIG_MV_ETH_PNC_L3_FLOW */

/* Add TOS priority rule */
int     pnc_ip4_dscp(int port, unsigned char dscp, unsigned char mask, int rxq);
void    pnc_ipv4_dscp_show(int port);


/* 2 tuple match */
int pnc_ipv4_2_tuples_add(unsigned int tid, unsigned int flow_hi,
			      unsigned int sip, unsigned int dip, unsigned int rxq);

int pnc_ipv6_2_tuples_add(unsigned int tid1, unsigned int tid2, unsigned int flow_id,
					      MV_U8 unique, MV_U8 *sip, MV_U8 *dip, unsigned int rxq);

/* 5 tuple match */
int pnc_ipv4_5_tuples_add(unsigned int tid, unsigned int flow_hi,
				unsigned int sip, unsigned int dip,
				unsigned int proto, unsigned int ports, unsigned int rxq);

#ifdef CONFIG_MV_ETH_PNC_WOL
void mv_pnc_wol_init(void);
int  mv_pnc_wol_rule_set(int port, char *data, char *mask, int size);
int  mv_pnc_wol_rule_del(int idx);
int  mv_pnc_wol_rule_del_all(int port);

int  mv_pnc_wol_pkt_match(int port, char *data, int size, int *ruleId);
void mv_pnc_wol_sleep(int port);
void mv_pnc_wol_wakeup(int port);
int  mv_pnc_wol_rule_dump(int idx);
void mv_pnc_wol_dump(void);
#endif /* CONFIG_MV_ETH_PNC_WOL */

#ifdef MV_ETH_PNC_AGING
MV_U32  mvPncAgingCntrRead(int tid);
void    mvPncAgingCntrWrite(int tid, MV_U32 w32);
void    mvPncAgingDump(int all);
void    mvPncAgingReset(void);
void    mvPncAgingScannerDump(void);
void    mvPncAgingCntrClear(int tid);
void    mvPncAgingCntrGroupSet(int tid, int gr);
void    mvPncAgingCounterClear(int tid, int gr);
#endif /* MV_ETH_PNC_AGING */

#ifdef MV_ETH_PNC_LB
void    mvPncLbDump(void);
int     mvPncLbRxqSet(int hash, int rxq);
int		mvPncLbModeIp4(int mode);
int		mvPncLbModeIp6(int mode);
int		mvPncLbModeL4(int mode);
#endif /* MV_ETH_PNC_LB */

#endif /* CONFIG_MV_ETH_PNC */

#endif /*__MV_PNC_H__ */
