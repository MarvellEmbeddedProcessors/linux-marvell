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

#ifndef __MV_PRS_H__
#define __MV_PRS_H__

/*
#define PP2_PRS_DEBUG
*/
/*
 * TCAM topology definition.
 * The TCAM is divided into sections per protocol encapsulation.
 * Usually each section is designed to be to a lookup.
 * Change sizes of sections according to the target product.
 */

/* VLAN */
#define SINGLE_VLAN_AI		0
#define DBL_VLAN_AI_BIT		7
#define DBL_VLAN_SHADOW_SIZE	0x64	/* max number of double vlan*/

/* DSA/EDSA type */
#define TAGGED			1
#define UNTAGGED		0
#define EDSA			1
#define DSA			0

#define	DSA_ETHER_TYPE		0xDADA/*TODO set to default DSA ether type*/

#define MV_PP2_PRS_INVALID_FLOW_ID	(0xFF) /* Invalid Flow ID */

/* MAC entries , shadow udf */
enum prs_udf {
	PRS_UDF_MAC_DEF,
	PRS_UDF_MAC_RANGE,
	PRS_UDF_L2_DEF,
	PRS_UDF_L2_DEF_COPY,
	PRS_UDF_L2_USER,
};

/* LOOKUP ID */
enum prs_lookup {
	PRS_LU_MH,
	PRS_LU_MAC,
	PRS_LU_DSA,
	PRS_LU_VLAN,
	PRS_LU_L2,
	PRS_LU_PPPOE,
	PRS_LU_IP4,
	PRS_LU_IP6,
	PRS_LU_FLOWS,
	PRS_LU_LAST,
};

/* L3 cast enum */
enum prs_l3_cast {
	L3_UNIT_CAST,
	L3_MULTI_CAST,
	L3_BROAD_CAST
};

/* PP2 GMAC enum */
enum prs_gmacs_enum {
	INVALID_GMAC = -1,
	ENUM_GMAC_0,
	ENUM_GMAC_1,
	ENUM_GMAC_LPK,
	ENUM_PMAC = 7,
	MAX_GMAC = ENUM_PMAC,
	MAX_NUM_GMACS
};

/* Multicast MAC kinds */
enum prs_mac_mc {
	IP4_MAC_MC,
	IP6_MAC_MC,
	MAX_MAC_MC
};

/* IPV6 extension header length supported
*  Calculate: Length = 8B + len * 4 */
enum prs_ip6_ext_ah_len {
	IP6_AH_LEN_16B = 2,
	IP6_AH_LEN_20B = 3,
	IP6_AH_LEN_24B = 4,
	IP6_AH_LEN_28B = 5,
	IP6_AH_LEN_32B = 6,
	IP6_AH_LEN_36B = 7,
	IP6_AH_LEN_MAX
};

/* Tcam entries ID */
#define PE_DROP_ALL				0
/* The TCAM rule for RX Special packets based on Marvell header is allocated dynamically */
#define PE_FIRST_FREE_TID				1

#define PE_LAST_FREE_TID	(MV_PP2_PRS_TCAM_SIZE - 31)
#define PE_IP6_EXT_PROTO_UN	(MV_PP2_PRS_TCAM_SIZE - 30)
#define PE_MAC_MC_IP6		(MV_PP2_PRS_TCAM_SIZE - 29) /* multicast for IPv6 */
#define PE_IP6_ADDR_UN		(MV_PP2_PRS_TCAM_SIZE - 28)
#define PE_IP4_ADDR_UN		(MV_PP2_PRS_TCAM_SIZE - 27)
#define PE_LAST_DEFAULT_FLOW	(MV_PP2_PRS_TCAM_SIZE - 26)
#define PE_FIRST_DEFAULT_FLOW	(MV_PP2_PRS_TCAM_SIZE - 19)
/*#define PE_ETYPE_DSA		(MV_PP2_PRS_TCAM_SIZE - 19)*/
#define PE_EDSA_TAGGED		(MV_PP2_PRS_TCAM_SIZE - 18)
#define PE_EDSA_UNTAGGED	(MV_PP2_PRS_TCAM_SIZE - 17)
#define PE_DSA_TAGGED		(MV_PP2_PRS_TCAM_SIZE - 16)
#define PE_DSA_UNTAGGED		(MV_PP2_PRS_TCAM_SIZE - 15)

#define PE_ETYPE_EDSA_TAGGED	(MV_PP2_PRS_TCAM_SIZE - 14)
#define PE_ETYPE_EDSA_UNTAGGED	(MV_PP2_PRS_TCAM_SIZE - 13)
#define PE_ETYPE_DSA_TAGGED	(MV_PP2_PRS_TCAM_SIZE - 12)
#define PE_ETYPE_DSA_UNTAGGED	(MV_PP2_PRS_TCAM_SIZE - 11)

#define PE_MH_DEFAULT		(MV_PP2_PRS_TCAM_SIZE - 10) /* Marvell header default rule */
#define PE_DSA_DEFAULT		(MV_PP2_PRS_TCAM_SIZE - 9)
#define PE_IP6_PROTO_UN		(MV_PP2_PRS_TCAM_SIZE - 8)
#define PE_IP4_PROTO_UN		(MV_PP2_PRS_TCAM_SIZE - 7)
#define PE_ETH_TYPE_UN		(MV_PP2_PRS_TCAM_SIZE - 6)
#define PE_VLAN_DBL		(MV_PP2_PRS_TCAM_SIZE - 5) /* accept double vlan*/
#define PE_VLAN_NONE		(MV_PP2_PRS_TCAM_SIZE - 4) /* vlan default*/
#define PE_MAC_MC_ALL		(MV_PP2_PRS_TCAM_SIZE - 3) /* all multicast mode */
#define PE_MAC_PROMISCOUS	(MV_PP2_PRS_TCAM_SIZE - 2) /* promiscous mode */
#define PE_MAC_NON_PROMISCOUS	(MV_PP2_PRS_TCAM_SIZE - 1) /* non-promiscous mode */

/*
 * Pre-defined FlowId assigment
*/

#define FLOWID_DEF(_port_)	(_port_)
#define FLOWID_MASK	 	0x3F

/*
 * AI bits assigment
*/
#define IPV4_DIP_AI_BIT		0
#define IPV6_NO_EXT_AI_BIT	0
#define IPV6_EXT_AI_BIT		1
#define IPV6_EXT_AH_AI_BIT	2
#define IPV6_EXT_AH_LEN_AI_BIT	3
#define IPV6_EXT_AH_L4_AI_BIT	4

/*
 * IPv6 extension header related
*/
#define IPV6_EXT_EXCLUDE_BYTES	8	/* IP6 excluding bytes in extension header, 8 bytes */
#define IPV6_EXT_AH_UNIT_BYTES	4	/* The AH length units, 4 bytes */

/*
 * Export API
 */
int mvPrsDefFlow(int port);
int mvPrsDefaultInit(void);
int mvPrsMacDaAccept(int port, unsigned char *da, int add);
int mvPrsMacDaRangeSet(unsigned portBmp, MV_U8 *da, MV_U8 *mask, unsigned int ri, unsigned int riMask, MV_BOOL finish);
int mvPrsMacDaRangeDel(unsigned portBmp, MV_U8 *da, MV_U8 *mask);
int mvPrsMacDropAllSet(int port, int add);
int mvPrsMhSet(unsigned int portMap, unsigned short mh, unsigned short mh_mask, unsigned int ri, unsigned int riMask, MV_BOOL finish);
int mvPrsMhDel(unsigned int portMap, unsigned short mh, unsigned short mh_mask);
int mvPrsMcastDelAll(int port);
int mvPrsMhRxSpecialSet(int port, unsigned short mh, int add);
int mvPrsMacPromiscousSet(int port, int add);
int mvPrsMacAllMultiSet(int port, int add);
int mvPrsDebugBasicInit(void);
int mvPrsFlowIdGen(int tid, int flowId, unsigned int res, unsigned int resMask, int portBmp);
int mvPrsFlowIdDel(int tid);
int mvPrsFlowIdFirstFreeGet(void);
int mvPrsFlowIdLastFreeGet(void);
int mvPrsFlowIdRelease(int flowId);
int mvPp2PrsTagModeSet(int port, int type);
int mvPp2PrsEtypeDsaModeSet(int port, int extand);
int mvPp2PrsEtypeDsaSet(unsigned int eType);
int mvPrsEthTypeSet(int portMap, unsigned short ethertype, unsigned int ri, unsigned int riMask, MV_BOOL finish);
int mvPrsEthTypeDel(int portMap, unsigned short eth_type);
int mvPrsVlanExist(unsigned short tpid, int ai);
int mvPrsDoubleVlanExist(unsigned short tpid1, unsigned short tpid2);
int mvPp2PrsTripleVlan(unsigned short tpid1, unsigned short tpid2, unsigned short tpid3, unsigned int portBmp, int add);
int mvPp2PrsDoubleVlan(unsigned short tpid1, unsigned short tpid2, unsigned int portBmp, int add);
int mvPp2PrsSingleVlan(unsigned short tpid, unsigned int portBmp, int add);
int mvPp2PrsVlanAllDel(void);
char *mvPrsVlanInfoStr(unsigned int vlan_info);
char *mvPrsL2InfoStr(unsigned int l2_info);
int mvPrsIp6NhSet(void);
/*
int mvPrsMacDaDrop(int port, unsigned char *da, int add);
*/
#endif /*__MV_PRS_H__ */
