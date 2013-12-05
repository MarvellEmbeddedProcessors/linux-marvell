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

#ifndef __mvPp2PlcrHw_h__
#define __mvPp2PlcrHw_h__

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */


#ifdef CONFIG_MV_ETH_PP2_1
#define MV_PP2_PLCR_NUM		48
#else
#define MV_PP2_PLCR_NUM		16
#endif

/*********************************** RX Policer Registers *******************/
/* exist only in ppv2.0 */
#define MV_PP2_PLCR_ENABLE_REG			(MV_PP2_REG_BASE + 0x1300)

#define MV_PP2_PLCR_EN_OFFS			0
#define MV_PP2_PLCR_EN_ALL_MASK			(((1 << MV_PP2_PLCR_NUM) - 1) << MV_PP2_PLCR_EN_OFFS)
#define MV_PP2_PLCR_EN_MASK(plcr)		((1 << (plcr)) << MV_PP2_PLCR_EN_OFFS)
/*---------------------------------------------------------------------------------------------*/

#define MV_PP2_PLCR_BASE_PERIOD_REG		(MV_PP2_REG_BASE + 0x1304)

#define MV_PP2_PLCR_BASE_PERIOD_OFFS		0
#define MV_PP2_PLCR_BASE_PERIOD_BITS		16
#define MV_PP2_PLCR_BASE_PERIOD_ALL_MASK	\
		(((1 << MV_PP2_PLCR_BASE_PERIOD_BITS) - 1) << MV_PP2_PLCR_BASE_PERIOD_OFFS)
#define MV_PP2_PLCR_BASE_PERIOD_MASK(p)		\
		(((p) << MV_PP2_PLCR_BASE_PERIOD_OFFS) & MV_PP2_PLCR_BASE_PERIOD_ALL_MASK)

#define MV_PP2_PLCR_ADD_TOKENS_EN_BIT		16
#define MV_PP2_PLCR_ADD_TOKENS_EN_MASK		(1 << MV_PP2_PLCR_ADD_TOKENS_EN_BIT)
/*---------------------------------------------------------------------------------------------*/
#define MV_PP2_PLCR_MODE_REG			(MV_PP2_REG_BASE + 0x1308)
#define MV_PP2_PLCR_MODE_BITS			(3)
#define MV_PP2_PLCR_MODE_MASK			(((1 << MV_PP2_PLCR_MODE_BITS) - 1) << 0)

/*---------------------------------------------------------------------------------------------*/
/* exist only in ppv2.1*/
#define MV_PP2_PLCR_TABLE_INDEX_REG		(MV_PP2_REG_BASE + 0x130c)
#define MV_PP2_PLCR_COMMIT_TOKENS_REG		(MV_PP2_REG_BASE + 0x1310)
#define MV_PP2_PLCR_EXCESS_TOKENS_REG		(MV_PP2_REG_BASE + 0x1314)
/*---------------------------------------------------------------------------------------------*/

#define MV_PP2_PLCR_BUCKET_SIZE_REG		(MV_PP2_REG_BASE + 0x1318)

#define MV_PP2_PLCR_COMMIT_SIZE_OFFS		0
#define MV_PP2_PLCR_COMMIT_SIZE_BITS		16
#define MV_PP2_PLCR_COMMIT_SIZE_ALL_MASK	\
		(((1 << MV_PP2_PLCR_COMMIT_SIZE_BITS) - 1) << MV_PP2_PLCR_COMMIT_SIZE_OFFS)
#define MV_PP2_PLCR_COMMIT_SIZE_MASK(size)	\
		(((size) << MV_PP2_PLCR_COMMIT_SIZE_OFFS) & MV_PP2_PLCR_COMMIT_SIZE_ALL_MASK)

#define MV_PP2_PLCR_EXCESS_SIZE_OFFS		16
#define MV_PP2_PLCR_EXCESS_SIZE_BITS		16
#define MV_PP2_PLCR_EXCESS_SIZE_ALL_MASK	\
		(((1 << MV_PP2_PLCR_EXCESS_SIZE_BITS) - 1) << MV_PP2_PLCR_EXCESS_SIZE_OFFS)
#define MV_PP2_PLCR_EXCESS_SIZE_MASK(size)	\
		(((size) << MV_PP2_PLCR_EXCESS_SIZE_OFFS) & MV_PP2_PLCR_EXCESS_SIZE_ALL_MASK)
/*---------------------------------------------------------------------------------------------*/

#define MV_PP2_PLCR_TOKEN_CFG_REG		(MV_PP2_REG_BASE + 0x131c)

#define MV_PP2_PLCR_TOKEN_VALUE_OFFS		0
#define MV_PP2_PLCR_TOKEN_VALUE_BITS		10
#define MV_PP2_PLCR_TOKEN_VALUE_ALL_MASK	\
		(((1 << MV_PP2_PLCR_TOKEN_VALUE_BITS) - 1) << MV_PP2_PLCR_TOKEN_VALUE_OFFS)
#define MV_PP2_PLCR_TOKEN_VALUE_MASK(val)	\
		(((val) << MV_PP2_PLCR_TOKEN_VALUE_OFFS) & MV_PP2_PLCR_TOKEN_VALUE_ALL_MASK)

#define MV_PP2_PLCR_TOKEN_TYPE_OFFS		12
#define MV_PP2_PLCR_TOKEN_TYPE_BITS		3
#define MV_PP2_PLCR_TOKEN_TYPE_ALL_MASK		\
		(((1 << MV_PP2_PLCR_TOKEN_TYPE_BITS) - 1) << MV_PP2_PLCR_TOKEN_TYPE_OFFS)
#define MV_PP2_PLCR_TOKEN_TYPE_MASK(type)	\
		(((type) << MV_PP2_PLCR_TOKEN_TYPE_OFFS) & MV_PP2_PLCR_TOKEN_TYPE_ALL_MASK)

#define MV_PP2_PLCR_TOKEN_UNIT_BIT		31
#define MV_PP2_PLCR_TOKEN_UNIT_MASK		(1 << MV_PP2_PLCR_TOKEN_UNIT_BIT)
#define MV_PP2_PLCR_TOKEN_UNIT_BYTES		(0 << MV_PP2_PLCR_TOKEN_UNIT_BIT)
#define MV_PP2_PLCR_TOKEN_UNIT_PKTS		(1 << MV_PP2_PLCR_TOKEN_UNIT_BIT)

#define MV_PP2_PLCR_COLOR_MODE_BIT		30
#define MV_PP2_PLCR_COLOR_MODE_MASK		(1 << MV_PP2_PLCR_COLOR_MODE_BIT)
#define MV_PP2_PLCR_COLOR_MODE_BLIND		(0 << MV_PP2_PLCR_COLOR_MODE_BIT)
#define MV_PP2_PLCR_COLOR_MODE_AWARE		(1 << MV_PP2_PLCR_COLOR_MODE_BIT)

#define MV_PP2_PLCR_ENABLE_BIT			29
#define MV_PP2_PLCR_ENABLE_MASK			(1 << MV_PP2_PLCR_ENABLE_BIT)
/*---------------------------------------------------------------------------------------------*/

#define MV_PP2_PLCR_MIN_PKT_LEN_REG		(MV_PP2_REG_BASE + 0x1320)

#define MV_PP2_PLCR_MIN_PKT_LEN_OFFS		0
#define MV_PP2_PLCR_MIN_PKT_LEN_BITS		8
#define MV_PP2_PLCR_MIN_PKT_LEN_ALL_MASK	\
		(((1 << MV_PP2_PLCR_MIN_PKT_LEN_BITS) - 1) << MV_PP2_PLCR_MIN_PKT_LEN_OFFS)
#define MV_PP2_PLCR_MIN_PKT_LEN_MASK(len)	\
		(((len) << MV_PP2_PLCR_MIN_PKT_LEN_OFFS) & MV_PP2_PLCR_MIN_PKT_LEN_ALL_MASK)
/*---------------------------------------------------------------------------------------------*/

#define MV_PP2_PLCR_EDROP_EN_REG		(MV_PP2_REG_BASE + 0x1330)

#define MV_PP2_PLCR_EDROP_EN_BIT		0
#define MV_PP2_PLCR_EDROP_EN_MASK		(1 << MV_PP2_PLCR_EDROP_EN_BIT)
/*---------------------------------------------------------------------------------------------*/
/*ppv2.1 policer early drop threshold mechanism changed*/
#define MV_PP2_V0_PLCR_EDROP_THRESH_NUM		4

#define MV_PP2_V0_PLCR_EDROP_TR_OFFS(i)		((i % 2) ? 16 : 0)
#define MV_PP2_V0_PLCR_EDROP_TR_BITS		14
#define MV_PP2_V0_PLCR_EDROP_TR_ALL_MASK(i)	\
		(((1 << MV_PP2_V0_PLCR_EDROP_TR_BITS) - 1) << MV_PP2_V0_PLCR_EDROP_TR_OFFS(i))
#define MV_PP2_V0_PLCR_EDROP_TR_MASK(i, tr)	\
		(((tr) << MV_PP2_V0_PLCR_EDROP_TR_OFFS(i)) & MV_PP2_V0_PLCR_EDROP_TR_ALL_MASK(i))

#define MV_PP2_V0_PLCR_EDROP_CPU_TR_REG(i)	(MV_PP2_REG_BASE + 0x1340 + (((i) / 2) << 2))
#define MV_PP2_V0_PLCR_EDROP_HWF_TR_REG(i)	(MV_PP2_REG_BASE + 0x1350 + (((i) / 2) << 2))
/*---------------------------------------------------------------------------------------------*/
/*ppv2.1 policer early drop threshold new mechanism*/
#define MV_PP2_V1_PLCR_EDROP_THRESH_NUM		16

#define MV_PP2_V1_PLCR_EDROP_TR_OFFS		0
#define MV_PP2_V1_PLCR_EDROP_TR_BITS		14

#define MV_PP2_V1_PLCR_EDROP_TR_MASK(i)		\
		(((1 << MV_PP2_V1_PLCR_EDROP_TR_BITS) - 1) << MV_PP2_V1_PLCR_EDROP_TR_OFFS)

#define MV_PP2_V1_PLCR_EDROP_CPU_TR_REG(i)	(MV_PP2_REG_BASE + 0x1380 + ((i) * 4))
#define MV_PP2_V1_PLCR_EDROP_HWF_TR_REG(i)	(MV_PP2_REG_BASE + 0x13c0 + ((i) * 4))

/*---------------------------------------------------------------------------------------------*/

#define MV_PP2_PLCR_EDROP_RXQ_REG		(MV_PP2_REG_BASE + 0x1348)
#define MV_PP2_PLCR_EDROP_RXQ_TR_REG		(MV_PP2_REG_BASE + 0x134c)
/*---------------------------------------------------------------------------------------------*/

#define MV_PP2_PLCR_EDROP_TXQ_REG		(MV_PP2_REG_BASE + 0x1358)
#define MV_PP2_PLCR_EDROP_TXQ_TR_REG		(MV_PP2_REG_BASE + 0x135c)
/*---------------------------------------------------------------------------------------------*/
#define MV_PP2_V1_PLCR_PKT_GREEN_REG(pol)	(MV_PP2_REG_BASE + 0x7400 + 4 * (pol))
#define MV_PP2_V1_PLCR_PKT_YELLOW_REG(pol)	(MV_PP2_REG_BASE + 0x7500 + 4 * (pol))
#define MV_PP2_V1_PLCR_PKT_RED_REG(pol)		(MV_PP2_REG_BASE + 0x7600 + 4 * (pol))
/*---------------------------------------------------------------------------------------------*/

/* Policer APIs */
void        mvPp2PlcrHwRegs(void);
void        mvPp2PlcrHwDumpAll(void);
void        mvPp2PlcrHwDumpSingle(int plcr);
void        mvPp2V1PlcrTbCntDump(int plcr);
MV_STATUS   mvPp2PlcrHwBasePeriodSet(int period);
MV_STATUS   mvPp2PlcrHwBaseRateGenEnable(int enable);
MV_STATUS   mvPp2PlcrHwEnable(int plcr, int enable);
MV_STATUS   mvPp2PlcrHwMode(int mode);
MV_STATUS   mvPp2PlcrHwMinPktLen(int bytes);
MV_STATUS   mvPp2PlcrHwEarlyDropSet(int enable);
MV_STATUS   mvPp2PlcrHwTokenConfig(int plcr, int unit, int type);
MV_STATUS   mvPp2PlcrHwTokenValue(int plcr, int value);
MV_STATUS   mvPp2PlcrHwColorModeSet(int plcr, int enable);
MV_STATUS   mvPp2PlcrHwBucketSizeSet(int plcr, int commit, int excess);

/*ppv2.1 policer early drop threshold mechanism changed*/
MV_STATUS   mvPp2V0PlcrHwCpuThreshSet(int idx, int threshold);
MV_STATUS   mvPp2V0PlcrHwHwfThreshSet(int idx, int threshold);
MV_STATUS   mvPp2V1PlcrHwCpuThreshSet(int idx, int threshold);
MV_STATUS   mvPp2V1PlcrHwHwfThreshSet(int idx, int threshold);
MV_STATUS   mvPp2PlcrHwRxqThreshSet(int rxq, int idx);
MV_STATUS   mvPp2PlcrHwTxqThreshSet(int txq, int idx);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __mvPp2PlcrHw_h__ */
