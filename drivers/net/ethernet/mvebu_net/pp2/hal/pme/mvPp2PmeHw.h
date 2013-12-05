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


#ifndef __mvPp2PmeHw_h__
#define __mvPp2PmeHw_h__

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/* FIXME: move to ctrlSpec.h */
#define MV_PP2_PME_INSTR_SIZE	2600
#define MV_PP2_PME_DATA1_SIZE   (46 * 1024 / 2) /* 46KBytes = 23K data of 2 bytes */
#define MV_PP2_PME_DATA2_SIZE   (4 * 1024 / 2) /* 4KBytes = 2K data of 2 bytes */

/*************** TX Packet Modification Registers *******************/
#define MV_PP2_PME_TBL_IDX_REG				(MV_PP2_REG_BASE + 0x8400)
#define MV_PP2_PME_TBL_INSTR_REG			(MV_PP2_REG_BASE + 0x8480)
#define MV_PP2_PME_TBL_DATA1_REG			(MV_PP2_REG_BASE + 0x8500)
#define MV_PP2_PME_TBL_DATA2_REG			(MV_PP2_REG_BASE + 0x8580)
#define MV_PP2_PME_TBL_STATUS_REG			(MV_PP2_REG_BASE + 0x8600)
#define MV_PP2_PME_TCONT_THRESH_REG			(MV_PP2_REG_BASE + 0x8604)
#define MV_PP2_PME_MTU_REG					(MV_PP2_REG_BASE + 0x8608)

#define MV_PP2_PME_MAX_VLAN_ETH_TYPES		4
#define MV_PP2_PME_VLAN_ETH_TYPE_REG(i)		(MV_PP2_REG_BASE + 0x8610 + ((i) << 2))
/*---------------------------------------------------------------------------*/

#define MV_PP2_PME_DEF_VLAN_CFG_REG			(MV_PP2_REG_BASE + 0x8620)
/*---------------------------------------------------------------------------*/

#define MV_PP2_PME_MAX_DSA_ETH_TYPES		2
#define MV_PP2_PME_DEF_DSA_CFG_REG(i)		(MV_PP2_REG_BASE + 0x8624 + ((i) << 2))
/*---------------------------------------------------------------------------*/

#define MV_PP2_PME_DEF_DSA_SRC_DEV_REG		(MV_PP2_REG_BASE + 0x8630)
#define MV_PP2_PME_DSA_SRC_DEV_OFFS			1
#define MV_PP2_PME_DSA_SRC_DEV_BITS			4
#define MV_PP2_PME_DSA_SRC_DEV_ALL_MASK		(((1 << MV_PP2_PME_DSA_SRC_DEV_BITS) - 1) << MV_PP2_PME_DSA_SRC_DEV_OFFS)
#define MV_PP2_PME_DSA_SRC_DEV_MASK(dev)	((dev) << MV_PP2_PME_DSA_SRC_DEV_OFFS)
/*---------------------------------------------------------------------------*/

#define MV_PP2_PME_TTL_ZERO_FRWD_REG		(MV_PP2_REG_BASE + 0x8640)
#define MV_PP2_PME_TTL_ZERO_FRWD_BIT		0
#define MV_PP2_PME_TTL_ZERO_FRWD_MASK		(1 << MV_PP2_PME_TTL_ZERO_FRWD_BIT);
/*---------------------------------------------------------------------------*/

#define MV_PP2_PME_PPPOE_ETYPE_REG			(MV_PP2_REG_BASE + 0x8650)

#define MV_PP2_PME_PPPOE_DATA_REG			(MV_PP2_REG_BASE + 0x8654)

#define MV_PP2_PME_PPPOE_CODE_OFFS			0
#define MV_PP2_PME_PPPOE_CODE_BITS			8
#define MV_PP2_PME_PPPOE_CODE_ALL_MASK		(((1 << MV_PP2_PME_PPPOE_CODE_BITS) - 1) << MV_PP2_PME_PPPOE_CODE_OFFS)
#define MV_PP2_PME_PPPOE_CODE_MASK(code)	(((code) << MV_PP2_PME_PPPOE_CODE_OFFS) & MV_PP2_PME_PPPOE_CODE_ALL_MASK)

#define MV_PP2_PME_PPPOE_TYPE_OFFS			8
#define MV_PP2_PME_PPPOE_TYPE_BITS			4
#define MV_PP2_PME_PPPOE_TYPE_ALL_MASK		(((1 << MV_PP2_PME_PPPOE_TYPE_BITS) - 1) << MV_PP2_PME_PPPOE_TYPE_OFFS)
#define MV_PP2_PME_PPPOE_TYPE_MASK(type)	(((type) << MV_PP2_PME_PPPOE_TYPE_OFFS) & MV_PP2_PME_PPPOE_TYPE_ALL_MASK)

#define MV_PP2_PME_PPPOE_VER_OFFS			12
#define MV_PP2_PME_PPPOE_VER_BITS			4
#define MV_PP2_PME_PPPOE_VER_ALL_MASK		(((1 << MV_PP2_PME_PPPOE_VER_BITS) - 1) << MV_PP2_PME_PPPOE_VER_OFFS)
#define MV_PP2_PME_PPPOE_VER_MASK(ver)		(((ver) << MV_PP2_PME_PPPOE_VER_OFFS) & MV_PP2_PME_PPPOE_VER_ALL_MASK)
/*---------------------------------------------------------------------------*/

#define MV_PP2_PME_PPPOE_LEN_REG			(MV_PP2_REG_BASE + 0x8658)
/*---------------------------------------------------------------------------*/

#define MV_PP2_PME_PPPOE_PROTO_REG			(MV_PP2_REG_BASE + 0x865c)

#define MV_PP2_PME_PPPOE_PROTO_OFFS(i)		((i == 0) ? 0 : 16)
#define MV_PP2_PME_PPPOE_PROTO_BITS			16
#define MV_PP2_PME_PPPOE_PROTO_ALL_MASK(i)	(((1 << MV_PP2_PME_PPPOE_PROTO_BITS) - 1) << MV_PP2_PME_PPPOE_PROTO_OFFS(i))
#define MV_PP2_PME_PPPOE_PROTO_MASK(i, p)	(((p) << MV_PP2_PME_PPPOE_PROTO_OFFS(i)) & MV_PP2_PME_PPPOE_PROTO_ALL_MASK(i))
/*---------------------------------------------------------------------------*/

#define MV_PP2_PME_CONFIG_REG				(MV_PP2_REG_BASE + 0x8660)

#define MV_PP2_PME_MAX_HDR_SIZE_OFFS		0
#define MV_PP2_PME_MAX_HDR_SIZE_BITS		8
#define MV_PP2_PME_MAX_HDR_SIZE_ALL_MASK	(((1 << MV_PP2_PME_MAX_HDR_SIZE_BITS) - 1) << MV_PP2_PME_MAX_HDR_SIZE_OFFS)
#define MV_PP2_PME_MAX_HDR_SIZE_MASK(size)	(((size) << MV_PP2_PME_MAX_HDR_SIZE_OFFS) & MV_PP2_PME_MAX_HDR_SIZE_ALL_MASK)

#define MV_PP2_PME_MAX_INSTR_NUM_OFFS		16
#define MV_PP2_PME_MAX_INSTR_NUM_BITS		8
#define MV_PP2_PME_MAX_INSTR_NUM_ALL_MASK	(((1 << MV_PP2_PME_MAX_INSTR_NUM_BITS) - 1) << MV_PP2_PME_MAX_INSTR_NUM_OFFS)
#define MV_PP2_PME_MAX_INSTR_NUM_MASK(num)	(((num) << MV_PP2_PME_MAX_INSTR_NUM_OFFS) & MV_PP2_PME_MAX_INSTR_NUM_ALL_MASK)

#define MV_PP2_PME_DROP_ON_ERR_BIT			24
#define MV_PP2_PME_DROP_ON_ERR_MASK			(1 << MV_PP2_PME_DROP_ON_ERR_BIT)
/*---------------------------------------------------------------------------*/

#define MV_PP2_PME_STATUS_1_REG				(MV_PP2_REG_BASE + 0x8664)
#define MV_PP2_PME_STATUS_2_REG(txp)		(MV_PP2_REG_BASE + 0x8700 + 4 * (txp))
#define MV_PP2_PME_STATUS_3_REG(txp)		(MV_PP2_REG_BASE + 0x8780 + 4 * (txp))

/* PME instructions table (MV_PP2_PME_TBL_INSTR_REG) fields definition */
#define MV_PP2_PME_DATA_OFFS				0
#define MV_PP2_PME_DATA_BITS				16
#define MV_PP2_PME_DATA_MASK				(((1 << MV_PP2_PME_DATA_BITS) - 1) << MV_PP2_PME_DATA_OFFS)

#define MV_PP2_PME_CTRL_OFFS				16
#define MV_PP2_PME_CTRL_BITS				16
#define MV_PP2_PME_CTRL_MASK				(((1 << MV_PP2_PME_CTRL_BITS) - 1) << MV_PP2_PME_CTRL_OFFS)

#define MV_PP2_PME_CMD_OFFS					16
#define MV_PP2_PME_CMD_BITS					5
#define MV_PP2_PME_CMD_ALL_MASK				(((1 << MV_PP2_PME_CMD_BITS) - 1) << MV_PP2_PME_CMD_OFFS)
#define MV_PP2_PME_CMD_MASK(cmd)			((cmd) << MV_PP2_PME_CMD_OFFS)

#define MV_PP2_PME_IP4_CSUM_BIT				21
#define MV_PP2_PME_IP4_CSUM_MASK			(1 << MV_PP2_PME_IP4_CSUM_BIT)

#define MV_PP2_PME_L4_CSUM_BIT				22
#define MV_PP2_PME_L4_CSUM_MASK				(1 << MV_PP2_PME_L4_CSUM_BIT)

#define MV_PP2_PME_LAST_BIT					23
#define MV_PP2_PME_LAST_MASK				(1 << MV_PP2_PME_LAST_BIT)

#define MV_PP2_PME_CMD_TYPE_OFFS			24
#define MV_PP2_PME_CMD_TYPE_BITS			3
#define MV_PP2_PME_CMD_TYPE_ALL_MASK		(((1 << MV_PP2_PME_CMD_TYPE_BITS) - 1) << MV_PP2_PME_CMD_TYPE_OFFS)
#define MV_PP2_PME_CMD_TYPE_MASK(type)		((type) << MV_PP2_PME_CMD_TYPE_OFFS)

enum MV_PP2_PME_CMD_E {
	MV_PP2_PME_CMD_NONE        = 0,
	MV_PP2_PME_CMD_ADD_2B,
	MV_PP2_PME_CMD_CFG_VLAN,
	MV_PP2_PME_CMD_ADD_VLAN,
	MV_PP2_PME_CMD_CFG_DSA_1,
	MV_PP2_PME_CMD_CFG_DSA_2,
	MV_PP2_PME_CMD_ADD_DSA,
	MV_PP2_PME_CMD_DEL_BYTES,
	MV_PP2_PME_CMD_REPLACE_2B,
	MV_PP2_PME_CMD_REPLACE_LSB,
	MV_PP2_PME_CMD_REPLACE_MSB,
	MV_PP2_PME_CMD_REPLACE_VLAN,
	MV_PP2_PME_CMD_DEC_LSB,
	MV_PP2_PME_CMD_DEC_MSB,
	MV_PP2_PME_CMD_ADD_CALC_LEN,
	MV_PP2_PME_CMD_REPLACE_LEN,
	MV_PP2_PME_CMD_IPV4_CSUM,
	MV_PP2_PME_CMD_L4_CSUM,
	MV_PP2_PME_CMD_SKIP,
	MV_PP2_PME_CMD_JUMP,
	MV_PP2_PME_CMD_JUMP_SKIP,
	MV_PP2_PME_CMD_JUMP_SUB,
	MV_PP2_PME_CMD_PPPOE,
	MV_PP2_PME_CMD_STORE,
	MV_PP2_PME_CMD_ADD_IP4_CSUM,
	MV_PP2_PME_CMD_PPPOE_2,
	MV_PP2_PME_CMD_REPLACE_MID,
	MV_PP2_PME_CMD_ADD_MULT,
	MV_PP2_PME_CMD_REPLACE_MULT,
	MV_PP2_PME_CMD_REPLACE_REM_2B, /* 0x1d - added on PPv2.1 (A0), MAS 3.3 */
	MV_PP2_PME_CMD_ADD_IP6_HDR,    /* 0x1e - added on PPv2.1 (A0), MAS 3.15 */
	MV_PP2_PME_CMD_DROP_PKT = 0x1f,
	MV_PP2_TMP_CMD_LAST
};

/* PME data1 and data2 fields MV_PP2_PME_TBL_DATA1_REG and MV_PP2_PME_TBL_DATA2_REG */
#define MV_PP2_PME_TBL_DATA_BITS		16
#define MV_PP2_PME_TBL_DATA_OFFS(idx)	((idx == 0) ? MV_PP2_PME_TBL_DATA_BITS : 0)
#define MV_PP2_PME_TBL_DATA_MASK(idx)	(((1 << MV_PP2_PME_TBL_DATA_BITS) - 1) << MV_PP2_PME_TBL_DATA_OFFS(idx))

/* Macros for internal usage */
#define MV_PP2_PME_IS_VALID(pme)        \
		((((pme)->word & MV_PP2_PME_CMD_ALL_MASK) >> MV_PP2_PME_CMD_OFFS) != MV_PP2_PME_CMD_NONE)

#define MV_PP2_PME_INVALID_SET(pme)        \
		((pme)->word = MV_PP2_PME_CMD_MASK(MV_NETA_CMD_NONE) | MV_PP2_PME_LAST_MASK);

#define MV_PP2_PME_CTRL_GET(pme)           \
		(MV_U16)(((pme)->word & MV_PP2_PME_CTRL_MASK) >> MV_PP2_PME_CTRL_OFFS)

#define MV_PP2_PME_CMD_GET(pme)           \
		(((pme)->word & MV_PP2_PME_CMD_ALL_MASK) >> MV_PP2_PME_CMD_OFFS)

#define MV_PP2_PME_DATA_GET(pme)           \
		(MV_U16)(((pme)->word & MV_PP2_PME_DATA_MASK) >> MV_PP2_PME_DATA_OFFS)

#define MV_PP2_PME_CMD_SET(pme, cmd)                       \
		(pme)->word &= ~MV_PP2_PME_CMD_ALL_MASK;       \
		(pme)->word |= MV_PP2_PME_CMD_MASK(cmd);

#define MV_PP2_PME_DATA_SET(pme, data)                         \
		(pme)->word &= ~MV_PP2_PME_DATA_MASK;              \
		(pme)->word |= ((data) << MV_PP2_PME_DATA_OFFS);

/* TX packet modification table entry */
typedef struct mv_pp2_pme {
	int     index;
	MV_U32	word;

} MV_PP2_PME_ENTRY;
/*------------------------------------------------------------*/


/* TX packet modification APIs */
void        mvPp2PmeHwRegs(void);
void        mvPp2PmeHwCntrs(void);
MV_STATUS   mvPp2PmeHwDump(int mode);
MV_STATUS   mvPp2PmeHwInvAll(void);

MV_STATUS   mvPp2PmeHwInv(int idx);
MV_STATUS   mvPp2PmeHwWrite(int idx, MV_PP2_PME_ENTRY *pEntry);
MV_STATUS   mvPp2PmeHwRead(int idx, MV_PP2_PME_ENTRY *pEntry);

MV_STATUS   mvPp2PmeSwDump(MV_PP2_PME_ENTRY *pEntry);
MV_STATUS   mvPp2PmeSwClear(MV_PP2_PME_ENTRY *pEntry);
MV_STATUS   mvPp2PmeSwWordSet(MV_PP2_PME_ENTRY *pEntry, MV_U32 word);
MV_STATUS   mvPp2PmeSwCmdSet(MV_PP2_PME_ENTRY *pEntry, enum MV_PP2_PME_CMD_E cmd);
MV_STATUS   mvPp2PmeSwCmdTypeSet(MV_PP2_PME_ENTRY *pEntry, int type);
MV_STATUS   mvPp2PmeSwCmdFlagsSet(MV_PP2_PME_ENTRY *pEntry, int last, int ipv4, int l4);
MV_STATUS   mvPp2PmeSwCmdLastSet(MV_PP2_PME_ENTRY *pEntry, int last);
MV_STATUS   mvPp2PmeSwCmdDataSet(MV_PP2_PME_ENTRY *pEntry, MV_U16 data);

MV_STATUS   mvPp2PmeHwDataTblDump(int tbl);
MV_STATUS   mvPp2PmeHwDataTblClear(int tbl);
MV_STATUS   mvPp2PmeHwDataTblWrite(int tbl, int idx, MV_U16 data);
MV_STATUS   mvPp2PmeHwDataTblRead(int tbl, int idx, MV_U16 *data);

MV_STATUS   mvPp2PmeVlanEtherTypeSet(int idx, MV_U16 ethertype);
MV_STATUS   mvPp2PmeVlanDefaultSet(MV_U16 ethertype);
MV_STATUS   mvPp2PmeDsaDefaultSet(int idx, MV_U16 ethertype);
MV_STATUS   mvPp2PmeDsaSrcDevSet(MV_U8 src);
MV_STATUS   mvPp2PmeTtlZeroSet(int forward);
MV_STATUS   mvPp2PmePppoeConfig(MV_U8 version, MV_U8 type, MV_U8 code);
MV_STATUS   mvPp2PmePppoeProtoSet(int idx, MV_U16 protocol);
MV_STATUS   mvPp2PmePppoeEtypeSet(MV_U16 ethertype);
MV_STATUS   mvPp2PmePppoeLengthSet(MV_U16 length);
MV_STATUS   mvPp2PmeMaxConfig(int maxsize, int maxinstr, int errdrop);


#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __mvPp2PmeHw_h__ */
