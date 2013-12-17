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

#include "mvCommon.h"  /* Should be included before mvSysHwConfig */
#include "mvTypes.h"
#include "mvDebug.h"
#include "mvOs.h"

#include "common/mvPp2Common.h"
#include "gbe/mvPp2Gbe.h"
#include "mvPp2PlcrHw.h"


void        mvPp2PlcrHwRegs(void)
{
	int    i;
	MV_U32 regVal;

	mvOsPrintf("\n[PLCR registers: %d policers]\n", MV_PP2_PLCR_NUM);

#ifdef CONFIG_MV_ETH_PP2_1
	mvPp2PrintReg(MV_PP2_PLCR_MODE_REG,	"MV_PP2_PLCR_MODE_REG");
#else
	mvPp2PrintReg(MV_PP2_PLCR_ENABLE_REG,	"MV_PP2_PLCR_ENABLE_REG");
#endif
	mvPp2PrintReg(MV_PP2_PLCR_BASE_PERIOD_REG,	"MV_PP2_PLCR_BASE_PERIOD_REG");
	mvPp2PrintReg(MV_PP2_PLCR_MIN_PKT_LEN_REG,	"MV_PP2_PLCR_MIN_PKT_LEN_REG");
	mvPp2PrintReg(MV_PP2_PLCR_EDROP_EN_REG,		"MV_PP2_PLCR_EDROP_EN_REG");

	for (i = 0; i < MV_PP2_PLCR_NUM; i++) {
		mvOsPrintf("\n[Policer %d registers]\n", i);

		mvPp2WrReg(MV_PP2_PLCR_TABLE_INDEX_REG, i);
		mvPp2PrintReg(MV_PP2_PLCR_COMMIT_TOKENS_REG, "MV_PP2_PLCR_COMMIT_TOKENS_REG");
		mvPp2PrintReg(MV_PP2_PLCR_EXCESS_TOKENS_REG, "MV_PP2_PLCR_EXCESS_TOKENS_REG");
		mvPp2PrintReg(MV_PP2_PLCR_BUCKET_SIZE_REG,   "MV_PP2_PLCR_BUCKET_SIZE_REG");
		mvPp2PrintReg(MV_PP2_PLCR_TOKEN_CFG_REG,     "MV_PP2_PLCR_TOKEN_CFG_REG");
	}

	mvOsPrintf("\nEarly Drop Thresholds for SW and HW forwarding\n");
#ifdef CONFIG_MV_ETH_PP2_1
	for (i = 0; i < MV_PP2_V1_PLCR_EDROP_THRESH_NUM; i++) {
		mvPp2PrintReg2(MV_PP2_V1_PLCR_EDROP_CPU_TR_REG(i),   "MV_PP2_V1_PLCR_EDROP_CPU_TR_REG", i);
		mvPp2PrintReg2(MV_PP2_V1_PLCR_EDROP_HWF_TR_REG(i),   "MV_PP2_V1_PLCR_EDROP_HWF_TR_REG", i);
	}
#else

	for (i = 0; i < MV_PP2_V0_PLCR_EDROP_THRESH_NUM; i++) {
		mvPp2PrintReg2(MV_PP2_V0_PLCR_EDROP_CPU_TR_REG(i),   "MV_PP2_V0_PLCR_EDROP_CPU_TR_REG", i);
		mvPp2PrintReg2(MV_PP2_V0_PLCR_EDROP_HWF_TR_REG(i),   "MV_PP2_V0_PLCR_EDROP_HWF_TR_REG", i);
	}
#endif
	mvOsPrintf("\nPer RXQ: Non zero early drop thresholds\n");
	for (i = 0; i < MV_ETH_RXQ_TOTAL_NUM; i++) {
		mvPp2WrReg(MV_PP2_PLCR_EDROP_RXQ_REG, i);
		regVal = mvPp2RdReg(MV_PP2_PLCR_EDROP_RXQ_TR_REG);
		if (regVal != 0)
			mvOsPrintf("  %-32s: 0x%x = 0x%08x\n", "MV_PP2_PLCR_EDROP_RXQ_TR_REG", MV_PP2_PLCR_EDROP_RXQ_TR_REG, regVal);
	}
	mvOsPrintf("\nPer TXQ: Non zero Early Drop Thresholds\n");
	for (i = 0; i < MV_PP2_TXQ_TOTAL_NUM; i++) {
		mvPp2WrReg(MV_PP2_PLCR_EDROP_TXQ_REG, i);
		regVal = mvPp2RdReg(MV_PP2_PLCR_EDROP_TXQ_TR_REG);
		if (regVal != 0)
			mvOsPrintf("  %-32s: 0x%x = 0x%08x\n", "MV_PP2_PLCR_EDROP_TXQ_TR_REG", MV_PP2_PLCR_EDROP_TXQ_TR_REG, regVal);
	}
}

static void        mvPp2PlcrHwDumpTitle(void)
{
	MV_U32 regVal;

	regVal = mvPp2RdReg(MV_PP2_PLCR_BASE_PERIOD_REG);
	mvOsPrintf("PLCR status: %d policers, period=%d (%s), ",
				MV_PP2_PLCR_NUM, regVal & MV_PP2_PLCR_BASE_PERIOD_ALL_MASK,
				regVal & MV_PP2_PLCR_ADD_TOKENS_EN_MASK ? "En" : "Dis");

	regVal = mvPp2RdReg(MV_PP2_PLCR_EDROP_EN_REG);
	mvOsPrintf("edrop=%s, ", regVal & MV_PP2_PLCR_EDROP_EN_MASK ? "En" : "Dis");

	regVal = mvPp2RdReg(MV_PP2_PLCR_MIN_PKT_LEN_REG);
	mvOsPrintf("min_pkt=%d bytes\n", (regVal & MV_PP2_PLCR_MIN_PKT_LEN_ALL_MASK) >> MV_PP2_PLCR_MIN_PKT_LEN_OFFS);

	mvOsPrintf("PLCR: enable period  unit   type  tokens  color  c_size  e_size  c_tokens  e_tokens\n");
}

static void        mvPp2PlcrHwDump(int plcr)
{
	int units, type, tokens, color, enable;
	MV_U32 regVal;

	mvPp2WrReg(MV_PP2_PLCR_TABLE_INDEX_REG, plcr);
	mvOsPrintf("%3d:  ", plcr);

#ifndef CONFIG_MV_ETH_PP2_1
	enable = mvPp2RdReg(MV_PP2_PLCR_ENABLE_REG);
	mvOsPrintf("%4s", MV_BIT_CHECK(enable, plcr) ? "Yes" : "No");
#endif

	regVal = mvPp2RdReg(MV_PP2_PLCR_TOKEN_CFG_REG);
	units = regVal & MV_PP2_PLCR_TOKEN_UNIT_MASK;
	color = regVal & MV_PP2_PLCR_COLOR_MODE_MASK;
	type = (regVal & MV_PP2_PLCR_TOKEN_TYPE_ALL_MASK) >> MV_PP2_PLCR_TOKEN_TYPE_OFFS;
	tokens =  (regVal & MV_PP2_PLCR_TOKEN_VALUE_ALL_MASK) >> MV_PP2_PLCR_TOKEN_VALUE_OFFS;
#ifdef CONFIG_MV_ETH_PP2_1
	enable = regVal & MV_PP2_PLCR_ENABLE_MASK;
	mvOsPrintf("%4s", enable ? "Yes" : "No");
#endif
	mvOsPrintf("   %-5s  %2d   %5d", units ? "pkts" : "bytes", type, tokens);
	mvOsPrintf("  %-5s", color ? "aware" : "blind");

	regVal = mvPp2RdReg(MV_PP2_PLCR_BASE_PERIOD_REG);
	mvOsPrintf("  %6d", regVal & MV_PP2_PLCR_BASE_PERIOD_ALL_MASK);

	regVal = mvPp2RdReg(MV_PP2_PLCR_BUCKET_SIZE_REG);
	mvOsPrintf("    %04x    %04x",
			(regVal & MV_PP2_PLCR_COMMIT_SIZE_ALL_MASK) >> MV_PP2_PLCR_COMMIT_SIZE_OFFS,
			(regVal & MV_PP2_PLCR_EXCESS_SIZE_ALL_MASK) >> MV_PP2_PLCR_EXCESS_SIZE_OFFS);

	regVal = mvPp2RdReg(MV_PP2_PLCR_COMMIT_TOKENS_REG);
	mvOsPrintf("    %08x", regVal);

	regVal = mvPp2RdReg(MV_PP2_PLCR_EXCESS_TOKENS_REG);
	mvOsPrintf("  %08x", regVal);

	mvOsPrintf("\n");
}

void        mvPp2PlcrHwDumpAll(void)
{
	int i;

	mvPp2PlcrHwDumpTitle();
	for (i = 0; i < MV_PP2_PLCR_NUM; i++)
		mvPp2PlcrHwDump(i);
}

void        mvPp2PlcrHwDumpSingle(int plcr)
{
	mvPp2PlcrHwDumpTitle();
	mvPp2PlcrHwDump(plcr);
}

MV_STATUS   mvPp2PlcrHwBaseRateGenEnable(int enable)
{
	MV_U32 regVal;

	regVal = mvPp2RdReg(MV_PP2_PLCR_BASE_PERIOD_REG);
	if (enable)
		regVal |= MV_PP2_PLCR_ADD_TOKENS_EN_MASK;
	else
		regVal &= ~MV_PP2_PLCR_ADD_TOKENS_EN_MASK;

	mvPp2WrReg(MV_PP2_PLCR_BASE_PERIOD_REG, regVal);

	return MV_OK;
}

MV_STATUS   mvPp2PlcrHwBasePeriodSet(int period)
{
	MV_U32 regVal;

	regVal = mvPp2RdReg(MV_PP2_PLCR_BASE_PERIOD_REG);
	regVal &= ~MV_PP2_PLCR_BASE_PERIOD_ALL_MASK;
	regVal |= MV_PP2_PLCR_BASE_PERIOD_MASK(period);
	mvPp2WrReg(MV_PP2_PLCR_BASE_PERIOD_REG, regVal);

	return MV_OK;
}

MV_STATUS   mvPp2PlcrHwMode(int mode)
{
	mvPp2WrReg(MV_PP2_PLCR_MODE_REG, mode);
	return MV_OK;
}

MV_STATUS   mvPp2PlcrHwEnable(int plcr, int enable)
{
	MV_U32 regVal;

	mvPp2WrReg(MV_PP2_PLCR_TABLE_INDEX_REG, plcr);

	regVal = mvPp2RdReg(MV_PP2_PLCR_TOKEN_CFG_REG);
	if (enable)
		regVal |= MV_PP2_PLCR_ENABLE_MASK;
	else
		regVal &= ~MV_PP2_PLCR_ENABLE_MASK;

	mvPp2WrReg(MV_PP2_PLCR_TOKEN_CFG_REG, regVal);

	return MV_OK;
}

MV_STATUS   mvPp2PlcrHwMinPktLen(int bytes)
{
	MV_U32 regVal;

	regVal = mvPp2RdReg(MV_PP2_PLCR_MIN_PKT_LEN_REG);
	regVal &= ~MV_PP2_PLCR_MIN_PKT_LEN_ALL_MASK;
	regVal |= MV_PP2_PLCR_MIN_PKT_LEN_MASK(bytes);
	mvPp2WrReg(MV_PP2_PLCR_MIN_PKT_LEN_REG, regVal);

	return MV_OK;
}

MV_STATUS   mvPp2PlcrHwEarlyDropSet(int enable)
{
	MV_U32 regVal;

	regVal = mvPp2RdReg(MV_PP2_PLCR_EDROP_EN_REG);
	if (enable)
		regVal |= MV_PP2_PLCR_EDROP_EN_MASK;
	else
		regVal &= ~MV_PP2_PLCR_EDROP_EN_MASK;

	mvPp2WrReg(MV_PP2_PLCR_EDROP_EN_REG, regVal);

	return MV_OK;
}

MV_STATUS   mvPp2PlcrHwTokenConfig(int plcr, int unit, int type)
{
	MV_U32 regVal;

	mvPp2WrReg(MV_PP2_PLCR_TABLE_INDEX_REG, plcr);
	regVal = mvPp2RdReg(MV_PP2_PLCR_TOKEN_CFG_REG);
	if (unit)
		regVal |= MV_PP2_PLCR_TOKEN_UNIT_MASK;
	else
		regVal &= ~MV_PP2_PLCR_TOKEN_UNIT_MASK;

	regVal &= ~MV_PP2_PLCR_TOKEN_TYPE_ALL_MASK;
	regVal |= MV_PP2_PLCR_TOKEN_TYPE_MASK(type);

	mvPp2WrReg(MV_PP2_PLCR_TOKEN_CFG_REG, regVal);

	return MV_OK;
}

MV_STATUS   mvPp2PlcrHwTokenValue(int plcr, int value)
{
	MV_U32 regVal;

	mvPp2WrReg(MV_PP2_PLCR_TABLE_INDEX_REG, plcr);
	regVal = mvPp2RdReg(MV_PP2_PLCR_TOKEN_CFG_REG);

	regVal &= ~MV_PP2_PLCR_TOKEN_VALUE_ALL_MASK;
	regVal |= MV_PP2_PLCR_TOKEN_VALUE_MASK(value);
	mvPp2WrReg(MV_PP2_PLCR_TOKEN_CFG_REG, regVal);

	return MV_OK;
}

MV_STATUS   mvPp2PlcrHwColorModeSet(int plcr, int enable)
{
	MV_U32 regVal;

	mvPp2WrReg(MV_PP2_PLCR_TABLE_INDEX_REG, plcr);
	regVal = mvPp2RdReg(MV_PP2_PLCR_TOKEN_CFG_REG);
	if (enable)
		regVal |= MV_PP2_PLCR_COLOR_MODE_MASK;
	else
		regVal &= ~MV_PP2_PLCR_COLOR_MODE_MASK;

	mvPp2WrReg(MV_PP2_PLCR_TOKEN_CFG_REG, regVal);

	return MV_OK;
}


MV_STATUS   mvPp2PlcrHwBucketSizeSet(int plcr, int commit, int excess)
{
	MV_U32 regVal;

	mvPp2WrReg(MV_PP2_PLCR_TABLE_INDEX_REG, plcr);
	regVal = MV_PP2_PLCR_EXCESS_SIZE_MASK(excess) | MV_PP2_PLCR_COMMIT_SIZE_MASK(commit);
	mvPp2WrReg(MV_PP2_PLCR_BUCKET_SIZE_REG, regVal);

	return MV_OK;
}
/*ppv2.1 policer early drop threshold mechanism changed*/
MV_STATUS   mvPp2V0PlcrHwCpuThreshSet(int idx, int threshold)
{
	MV_U32 regVal;

	regVal = mvPp2RdReg(MV_PP2_V0_PLCR_EDROP_CPU_TR_REG(idx));
	regVal &= ~MV_PP2_V0_PLCR_EDROP_TR_ALL_MASK(idx);
	regVal |= MV_PP2_V0_PLCR_EDROP_TR_MASK(idx, threshold);
	mvPp2WrReg(MV_PP2_V0_PLCR_EDROP_CPU_TR_REG(idx), regVal);

	return MV_OK;
}
/*ppv2.1 policer early drop threshold mechanism changed*/
MV_STATUS   mvPp2V1PlcrHwCpuThreshSet(int idx, int threshold)
{
	mvPp2WrReg(MV_PP2_V1_PLCR_EDROP_CPU_TR_REG(idx), threshold);

	return MV_OK;
}

/*ppv2.1 policer early drop threshold mechanism changed*/
MV_STATUS   mvPp2V0PlcrHwHwfThreshSet(int idx, int threshold)
{
	MV_U32 regVal;

	regVal = mvPp2RdReg(MV_PP2_V0_PLCR_EDROP_HWF_TR_REG(idx));
	regVal &= ~MV_PP2_V0_PLCR_EDROP_TR_ALL_MASK(idx);
	regVal |= MV_PP2_V0_PLCR_EDROP_TR_MASK(idx, threshold);
	mvPp2WrReg(MV_PP2_V0_PLCR_EDROP_HWF_TR_REG(idx), regVal);

	return MV_OK;
}

/*ppv2.1 policer early drop threshold mechanism changed*/
MV_STATUS   mvPp2V1PlcrHwHwfThreshSet(int idx, int threshold)
{
	mvPp2WrReg(MV_PP2_V1_PLCR_EDROP_HWF_TR_REG(idx), threshold);

	return MV_OK;
}

MV_STATUS   mvPp2PlcrHwRxqThreshSet(int rxq, int idx)
{
	mvPp2WrReg(MV_PP2_PLCR_EDROP_RXQ_REG, rxq);
	mvPp2WrReg(MV_PP2_PLCR_EDROP_RXQ_TR_REG, idx);

	return MV_OK;
}

MV_STATUS   mvPp2PlcrHwTxqThreshSet(int txq, int idx)
{
	mvPp2WrReg(MV_PP2_PLCR_EDROP_TXQ_REG, txq);
	mvPp2WrReg(MV_PP2_PLCR_EDROP_TXQ_TR_REG, idx);

	return MV_OK;
}

void mvPp2V1PlcrTbCntDump(int plcr)
{
	mvPp2PrintReg2(MV_PP2_V1_PLCR_PKT_GREEN_REG(plcr), "MV_PP2_V1_PLCR_PKT_GREEN_REG", plcr);
	mvPp2PrintReg2(MV_PP2_V1_PLCR_PKT_YELLOW_REG(plcr), "MV_PP2_V1_PLCR_PKT_YELLOW_REG", plcr);
	mvPp2PrintReg2(MV_PP2_V1_PLCR_PKT_RED_REG(plcr), "MV_PP2_V1_PLCR_PKT_RED_REG", plcr);

}


