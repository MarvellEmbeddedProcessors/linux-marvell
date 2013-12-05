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
#include "mvPp2PmeHw.h"


/* #define PME_DBG mvOsPrintf */
#define PME_DBG(X...)

static char *mvPp2PmeCmdName(enum MV_PP2_PME_CMD_E cmd)
{
	switch (cmd) {
	case MV_PP2_PME_CMD_NONE:		return "NO_MOD";
	case MV_PP2_PME_CMD_ADD_2B:		return "ADD_2B";
	case MV_PP2_PME_CMD_CFG_VLAN:	return "CFG_VLAN";
	case MV_PP2_PME_CMD_ADD_VLAN:	return "ADD_VLAN";
	case MV_PP2_PME_CMD_CFG_DSA_1:	return "CFG_DSA_1";
	case MV_PP2_PME_CMD_CFG_DSA_2:	return "CFG_DSA_2";
	case MV_PP2_PME_CMD_ADD_DSA:	return "ADD_DSA";
	case MV_PP2_PME_CMD_DEL_BYTES:	return "DEL_BYTES";
	case MV_PP2_PME_CMD_REPLACE_2B: return "REPLACE_2B";
	case MV_PP2_PME_CMD_REPLACE_LSB: return "REPLACE_LSB";
	case MV_PP2_PME_CMD_REPLACE_MSB: return "REPLACE_MSB";
	case MV_PP2_PME_CMD_REPLACE_VLAN: return "REPLACE_VLAN";
	case MV_PP2_PME_CMD_DEC_LSB:	return "DEC_LSB";
	case MV_PP2_PME_CMD_DEC_MSB:	return "DEC_MSB";
	case MV_PP2_PME_CMD_ADD_CALC_LEN: return "ADD_CALC_LEN";
	case MV_PP2_PME_CMD_REPLACE_LEN: return "REPLACE_LEN";
	case MV_PP2_PME_CMD_IPV4_CSUM:	return "IPV4_CSUM";
	case MV_PP2_PME_CMD_L4_CSUM:	return "L4_CSUM";
	case MV_PP2_PME_CMD_SKIP:		return "SKIP";
	case MV_PP2_PME_CMD_JUMP:		return "JUMP";
	case MV_PP2_PME_CMD_JUMP_SKIP:	return "JUMP_SKIP";
	case MV_PP2_PME_CMD_JUMP_SUB:	return "JUMP_SUB";
	case MV_PP2_PME_CMD_PPPOE:		return "PPPOE";
	case MV_PP2_PME_CMD_STORE:		return "STORE";
	case MV_PP2_PME_CMD_ADD_IP4_CSUM: return "ADD_L4";
	case MV_PP2_PME_CMD_PPPOE_2:	return "PPPOE_2";
	case MV_PP2_PME_CMD_REPLACE_MID: return "REPLACE_MID";
	case MV_PP2_PME_CMD_ADD_MULT:	return "ADD_MULT";
	case MV_PP2_PME_CMD_REPLACE_MULT: return "REPLACE_MULT";
	case MV_PP2_PME_CMD_REPLACE_REM_2B: return "REPLACE_REM_2B"; /* For PPv2.1 - A0 only, MAS 3.3 */
	case MV_PP2_PME_CMD_ADD_IP6_HDR: return "ADD_IP6_HDR";       /* For PPv2.1 - A0 only, MAS 3.15 */
	case MV_PP2_PME_CMD_DROP_PKT:	return "DROP";
	default:
		return "UNKNOWN";
	}
	return NULL;
};

static 	int mvPp2PmeDataTblSize(int tbl)
{
	int max;

	switch (tbl) {
	case 0:
		max = MV_PP2_PME_DATA1_SIZE;
		break;
	case 1:
		max = MV_PP2_PME_DATA2_SIZE;
		break;
	default:
		max = 0;
		mvOsPrintf("%s: tbl %d is out of range [0..1]\n", __func__, tbl);
	}
	return max;
}

static inline MV_U32	mvPp2PmeDataTblRegAddr(int tbl)
{
	MV_U32 regAddr;

	switch (tbl) {
	case 0:
		regAddr = MV_PP2_PME_TBL_DATA1_REG;
		break;
	case 1:
		regAddr = MV_PP2_PME_TBL_DATA2_REG;
		break;
	default:
		regAddr = 0;
		mvOsPrintf("%s: tbl %d is out of range [0..1]\n", __func__, tbl);
	}
	return regAddr;
}

/*******************************************************************************
* mvPp2PmeHwRegs - Print PME hardware registers
*
*******************************************************************************/
void        mvPp2PmeHwRegs(void)
{
	int    i;
	MV_U32 regVal;

	mvOsPrintf("\n[PME registers]\n");

	mvPp2PrintReg(MV_PP2_PME_TBL_IDX_REG, "MV_PP2_PME_TBL_IDX_REG");
	mvPp2PrintReg(MV_PP2_PME_TCONT_THRESH_REG, "MV_PP2_PME_TCONT_THRESH_REG");
	mvPp2PrintReg(MV_PP2_PME_MTU_REG, "MV_PP2_PME_MTU_REG");

	for (i = 0; i < MV_PP2_PME_MAX_VLAN_ETH_TYPES; i++)
		mvPp2PrintReg2(MV_PP2_PME_VLAN_ETH_TYPE_REG(i), "MV_PP2_PME_VLAN_ETH_TYPE_REG", i);

	mvPp2PrintReg(MV_PP2_PME_DEF_VLAN_CFG_REG, "MV_PP2_PME_DEF_VLAN_CFG_REG");
	for (i = 0; i < MV_PP2_PME_MAX_DSA_ETH_TYPES; i++)
		mvPp2PrintReg2(MV_PP2_PME_DEF_DSA_CFG_REG(i), "MV_PP2_PME_DEF_DSA_CFG_REG", i);

	mvPp2PrintReg(MV_PP2_PME_DEF_DSA_SRC_DEV_REG, "MV_PP2_PME_DEF_DSA_SRC_DEV_REG");
	mvPp2PrintReg(MV_PP2_PME_TTL_ZERO_FRWD_REG, "MV_PP2_PME_TTL_ZERO_FRWD_REG");
	mvPp2PrintReg(MV_PP2_PME_PPPOE_ETYPE_REG, "MV_PP2_PME_PPPOE_ETYPE_REG");
	mvPp2PrintReg(MV_PP2_PME_PPPOE_DATA_REG, "MV_PP2_PME_PPPOE_DATA_REG");
	mvPp2PrintReg(MV_PP2_PME_PPPOE_LEN_REG, "MV_PP2_PME_PPPOE_LEN_REG");
	mvPp2PrintReg(MV_PP2_PME_PPPOE_PROTO_REG, "MV_PP2_PME_PPPOE_PROTO_REG");
	mvPp2PrintReg(MV_PP2_PME_CONFIG_REG, "MV_PP2_PME_CONFIG_REG");
	mvPp2PrintReg(MV_PP2_PME_STATUS_1_REG, "MV_PP2_PME_STATUS_1_REG");

	mvOsPrintf("\nMV_PP2_PME_STATUS_2_REG[txp] registers that are not zero\n");
	for (i = 0; i < MV_PP2_TOTAL_TXP_NUM; i++) {
		regVal = mvPp2RdReg(MV_PP2_PME_STATUS_2_REG(i));
		if (regVal != 0)
			mvOsPrintf("%-32s[%2d]: 0x%x = 0x%08x\n",
				"MV_PP2_PME_STATUS_2_REG", i, MV_PP2_PME_STATUS_2_REG(i), regVal);
	}

	mvOsPrintf("\nMV_PP2_PME_STATUS_3_REG[txp] registers that are not zero\n");
	for (i = 0; i < MV_PP2_TOTAL_TXP_NUM; i++) {
		regVal = mvPp2RdReg(MV_PP2_PME_STATUS_3_REG(i));
		if (regVal != 0)
			mvOsPrintf("%-32s[%2d]: 0x%x = 0x%08x\n",
				"MV_PP2_PME_STATUS_3_REG", i, MV_PP2_PME_STATUS_3_REG(i), regVal);
	}
}

/*******************************************************************************
* mvPp2PmeHwWrite - Write PME entry to the hardware
*
* INPUT:
*       int			idx	- PME entry index to write to
*       MV_PP2_PME_ENTRY	*pEntry - PME software entry to be written
*
* RETURN:   MV_STATUS
*               MV_OK - Success, Others - Failure
*******************************************************************************/
MV_STATUS   mvPp2PmeHwWrite(int idx, MV_PP2_PME_ENTRY *pEntry)
{
	if ((idx < 0) || (idx >= MV_PP2_PME_INSTR_SIZE)) {
		mvOsPrintf("%s: entry %d is out of range [0..%d]\n", __func__, idx, MV_PP2_PME_INSTR_SIZE);
		return MV_OUT_OF_RANGE;
	}
	pEntry->index = idx;
	mvPp2WrReg(MV_PP2_PME_TBL_IDX_REG, idx);
	mvPp2WrReg(MV_PP2_PME_TBL_INSTR_REG, pEntry->word);

	return MV_OK;
}

/*******************************************************************************
* mvPp2PmeHwRead - Read PME entry from the hardware
*
* INPUT:
*       int	idx	- PME entry index to write to
* OUTPUT:
*       MV_PP2_PME_ENTRY	*pEntry - PME software entry to be read into
*
* RETURN:   MV_STATUS
*               MV_OK - Success, Others - Failure
*******************************************************************************/
MV_STATUS mvPp2PmeHwRead(int idx, MV_PP2_PME_ENTRY *pEntry)
{
	if ((idx < 0) || (idx >= MV_PP2_PME_INSTR_SIZE)) {
		mvOsPrintf("%s: entry %d is out of range [0..%d]\n", __func__, idx, MV_PP2_PME_INSTR_SIZE);
		return MV_OUT_OF_RANGE;
	}

	pEntry->index = idx;
	mvPp2WrReg(MV_PP2_PME_TBL_IDX_REG, idx);
	pEntry->word = mvPp2RdReg(MV_PP2_PME_TBL_INSTR_REG);

	return MV_OK;
}

/*******************************************************************************
* mvPp2PmeHwInv - Invalidate single PME entry in the hardware
*
* INPUT:
*       int	idx	- PME entry index to be invaliadted
*
* RETURN:   MV_STATUS
*               MV_OK - Success, Others - Failure
*******************************************************************************/
MV_STATUS   mvPp2PmeHwInv(int idx)
{
	MV_PP2_PME_ENTRY	entry;

	if ((idx < 0) || (idx >= MV_PP2_PME_INSTR_SIZE)) {
		mvOsPrintf("%s: entry %d is out of range [0..%d]\n", __func__, idx, MV_PP2_PME_INSTR_SIZE);
		return MV_OUT_OF_RANGE;
	}
	mvPp2PmeSwClear(&entry);

	return mvPp2PmeHwWrite(idx, &entry);
}

/*******************************************************************************
* mvPp2PmeHwInvAll - Invalidate all PME entries in the hardware
*
* INPUT:
*
* RETURN:   MV_STATUS
*               MV_OK - Success, Others - Failure
*******************************************************************************/
MV_STATUS   mvPp2PmeHwInvAll(void)
{
	int	idx;

	for (idx = 0; idx < MV_PP2_PME_INSTR_SIZE; idx++)
		mvPp2PmeHwInv(idx);

	return MV_OK;
}

/*******************************************************************************
* mvPp2PmeHwInit - Init TX Packet Modification driver
*
* INPUT:
*
* RETURN:   MV_STATUS
*               MV_OK - Success, Others - Failure
*******************************************************************************/
MV_STATUS   mvPp2PmeHwInit(void)
{
	mvPp2PmeHwInvAll();
	mvPp2PmeHwDataTblClear(0);
	mvPp2PmeHwDataTblClear(1);

	return MV_OK;
}

/*******************************************************************************
* mvPp2PmeSwDump - Print PME software entry
*
* INPUT:
*       MV_PP2_PME_ENTRY*    pEntry - PME software entry to be printed
*
* RETURN:   void
*******************************************************************************/
MV_STATUS	mvPp2PmeSwDump(MV_PP2_PME_ENTRY *pEntry)
{
	mvOsPrintf("%04x %04x: ",
		MV_PP2_PME_CTRL_GET(pEntry), MV_PP2_PME_DATA_GET(pEntry));

	mvOsPrintf("%s ", mvPp2PmeCmdName(MV_PP2_PME_CMD_GET(pEntry)));

	if (pEntry->word & MV_PP2_PME_IP4_CSUM_MASK)
		mvOsPrintf(", IPv4 csum");

	if (pEntry->word & MV_PP2_PME_L4_CSUM_MASK)
		mvOsPrintf(", L4 csum");

	if (pEntry->word & MV_PP2_PME_LAST_MASK)
		mvOsPrintf(", Last");

	mvOsPrintf("\n");

	return MV_OK;
}

/*******************************************************************************
* mvPp2PmeHwDump - Dump PME hardware entries
*
* INPUT:
*       int	mode   -
*
* RETURN:   void
*******************************************************************************/
MV_STATUS	mvPp2PmeHwDump(int mode)
{
	int					idx, count = 0;
	MV_PP2_PME_ENTRY 	entry;
	MV_STATUS			status;

	mvOsPrintf("PME instraction table: #%d entries\n", MV_PP2_PME_INSTR_SIZE);
	for (idx = 0; idx < MV_PP2_PME_INSTR_SIZE; idx++) {
		status = mvPp2PmeHwRead(idx, &entry);
		if (status != MV_OK) {
			mvOsPrintf("%s failed: idx=%d, status=%d\n",
					__func__, idx, status);
			return status;
		}
		if (mode == 0) {
			if (!MV_PP2_PME_IS_VALID(&entry))
				continue;
		}

		count++;
		mvOsPrintf("[%4d]: ", idx);
		mvPp2PmeSwDump(&entry);
	}

	if (!count)
		mvOsPrintf("Table is Empty\n");

	return MV_OK;
}

/*******************************************************************************
* mvPp2PmeEntryPrint - Print PME entry
*
* INPUT:
*       MV_PP2_PME_ENTRY	*pEntry - PME entry to be printed
*
* RETURN:   MV_STATUS
*******************************************************************************/
MV_STATUS   mvPp2PmeSwClear(MV_PP2_PME_ENTRY *pEntry)
{
	pEntry->word = 0;

	return MV_OK;
}

/*******************************************************************************
* mvPp2PmeSwWordSet - Set 4 bytes data to software PME entry
*
* INPUT:
*       MV_PP2_PME_ENTRY*    pEntry - PME entry to be set
*		MV_U32               word   - 4 bytes of data to be set
*
* RETURN:   MV_STATUS
*******************************************************************************/
MV_STATUS   mvPp2PmeSwWordSet(MV_PP2_PME_ENTRY *pEntry, MV_U32 word)
{
	pEntry->word = word;

	return MV_OK;
}

/*******************************************************************************
* mvPp2PmeSwCmdSet - Set modification command to software PME instruction entry
*
* INPUT:
*       MV_PP2_PME_ENTRY*     pEntry - PME entry to be set
*		enum MV_PP2_PME_CMD_E cmd    - modification command to be set
*
* RETURN:   MV_STATUS
*******************************************************************************/
MV_STATUS   mvPp2PmeSwCmdSet(MV_PP2_PME_ENTRY *pEntry, enum MV_PP2_PME_CMD_E cmd)
{
	pEntry->word &= ~MV_PP2_PME_CMD_ALL_MASK;
	pEntry->word |= MV_PP2_PME_CMD_MASK(cmd);

	return MV_OK;
}

/*******************************************************************************
* mvPp2PmeSwCmdTypeSet - Set modification command type to software PME instruction entry
*
* INPUT:
*       MV_PP2_PME_ENTRY*     pEntry - PME entry to be set
*		int                   type    - modification command type to be set
*
* RETURN:   MV_STATUS
*******************************************************************************/
MV_STATUS   mvPp2PmeSwCmdTypeSet(MV_PP2_PME_ENTRY *pEntry, int type)
{
	pEntry->word &= ~MV_PP2_PME_CMD_TYPE_ALL_MASK;
	pEntry->word |= MV_PP2_PME_CMD_TYPE_MASK(type);

	return MV_OK;
}

MV_STATUS   mvPp2PmeSwCmdLastSet(MV_PP2_PME_ENTRY *pEntry, int last)
{
	if (last)
		pEntry->word |= MV_PP2_PME_LAST_MASK;
	else
		pEntry->word &= ~MV_PP2_PME_LAST_MASK;

	return MV_OK;
}

MV_STATUS   mvPp2PmeSwCmdFlagsSet(MV_PP2_PME_ENTRY *pEntry, int last, int ipv4, int l4)
{
	if (last)
		pEntry->word |= MV_PP2_PME_LAST_MASK;
	else
		pEntry->word &= ~MV_PP2_PME_LAST_MASK;

	if (ipv4)
		pEntry->word |= MV_PP2_PME_IP4_CSUM_MASK;
	else
		pEntry->word &= ~MV_PP2_PME_IP4_CSUM_MASK;

	if (l4)
		pEntry->word |= MV_PP2_PME_L4_CSUM_MASK;
	else
		pEntry->word &= ~MV_PP2_PME_L4_CSUM_MASK;

	return MV_OK;
}

MV_STATUS   mvPp2PmeSwCmdDataSet(MV_PP2_PME_ENTRY *pEntry, MV_U16 data)
{
	MV_PP2_PME_DATA_SET(pEntry, data);
	return MV_OK;
}

/* Functions to access PME data1 and data2 tables */
MV_STATUS   mvPp2PmeHwDataTblWrite(int tbl, int idx, MV_U16 data)
{
	MV_U32  regVal;

	if ((tbl < 0) || (tbl > 1)) {
		mvOsPrintf("%s: data table %d is out of range [0..1]\n", __func__, tbl);
		return MV_BAD_PARAM;
	}
	if ((idx < 0) || (idx >= mvPp2PmeDataTblSize(tbl))) {
		mvOsPrintf("%s: entry index #%d is out of range [0..%d] for data table #%d\n",
					__func__, idx, tbl, mvPp2PmeDataTblSize(tbl));
		return MV_FALSE;
	}

	mvPp2WrReg(MV_PP2_PME_TBL_IDX_REG, idx / 2);

	regVal = mvPp2RdReg(mvPp2PmeDataTblRegAddr(tbl));
	regVal &= ~MV_PP2_PME_TBL_DATA_MASK(idx % 2);
	regVal |= (data << MV_PP2_PME_TBL_DATA_OFFS(idx % 2));

	mvPp2WrReg(MV_PP2_PME_TBL_IDX_REG, idx / 2);
	mvPp2WrReg(mvPp2PmeDataTblRegAddr(tbl), regVal);

	return MV_OK;
}

MV_STATUS   mvPp2PmeHwDataTblRead(int tbl, int idx, MV_U16 *data)
{
	MV_U32  regVal;

	if ((tbl < 0) || (tbl > 1)) {
		mvOsPrintf("%s: data table %d is out of range [0..1]\n", __func__, tbl);
		return MV_BAD_PARAM;
	}
	if ((idx < 0) || (idx >= mvPp2PmeDataTblSize(tbl))) {
		mvOsPrintf("%s: entry index #%d is out of range [0..%d] for data table #%d\n",
					__func__, idx, tbl, mvPp2PmeDataTblSize(tbl));
		return MV_FALSE;
	}

	mvPp2WrReg(MV_PP2_PME_TBL_IDX_REG, idx / 2);

	regVal = mvPp2RdReg(mvPp2PmeDataTblRegAddr(tbl));

	if (data)
		*data = (regVal & MV_PP2_PME_TBL_DATA_MASK(idx % 2)) >> MV_PP2_PME_TBL_DATA_OFFS(idx % 2);

	return MV_OK;
}

MV_STATUS   mvPp2PmeHwDataTblDump(int tbl)
{
	int idx, max, count = 0;
	MV_U16 data;

	if ((tbl < 0) || (tbl > 1)) {
		mvOsPrintf("%s: data table %d is out of range [0..1]\n", __func__, tbl);
		return MV_BAD_PARAM;
	}
	max = mvPp2PmeDataTblSize(tbl);

	mvOsPrintf("PME Data%d table: #%d entries\n", tbl + 1, max);
	for (idx = 0; idx < max; idx++) {
		mvPp2PmeHwDataTblRead(tbl, idx, &data);
		if (data != 0) {
			mvOsPrintf("[%4d]: 0x%04x\n", idx, data);
			count++;
		}
	}
	if (count == 0)
		mvOsPrintf("Table is Empty\n");

	return MV_OK;
}

MV_STATUS   mvPp2PmeHwDataTblClear(int tbl)
{
	int max, idx;

	if ((tbl < 0) || (tbl > 1)) {
		mvOsPrintf("%s: data table %d is out of range [0..1]\n", __func__, tbl);
		return MV_BAD_PARAM;
	}

	max = mvPp2PmeDataTblSize(tbl);
	for (idx = 0; idx < max; idx++)
		mvPp2PmeHwDataTblWrite(tbl, idx, 0);

	return MV_OK;
}

/* Functions to set other PME register fields */
MV_STATUS   mvPp2PmeVlanEtherTypeSet(int idx, MV_U16 ethertype)
{
	MV_U32 regVal = (MV_U32)ethertype;

	if ((idx < 0) || (idx > MV_PP2_PME_MAX_VLAN_ETH_TYPES)) {
		mvOsPrintf("%s: idx %d is out of range [0..%d]\n", __func__, idx, MV_PP2_PME_MAX_VLAN_ETH_TYPES);
		return MV_BAD_PARAM;
	}
	mvPp2WrReg(MV_PP2_PME_VLAN_ETH_TYPE_REG(idx), regVal);

	return MV_OK;
}

MV_STATUS   mvPp2PmeVlanDefaultSet(MV_U16 ethertype)
{
	mvPp2WrReg(MV_PP2_PME_DEF_VLAN_CFG_REG, (MV_U32)ethertype);
	return MV_OK;
}

MV_STATUS   mvPp2PmeDsaDefaultSet(int idx, MV_U16 ethertype)
{
	MV_U32 regVal = (MV_U32)ethertype;

	if ((idx < 0) || (idx > MV_PP2_PME_MAX_DSA_ETH_TYPES)) {
		mvOsPrintf("%s: idx %d is out of range [0..%d]\n", __func__, idx, MV_PP2_PME_MAX_DSA_ETH_TYPES);
		return MV_BAD_PARAM;
	}
	mvPp2WrReg(MV_PP2_PME_DEF_DSA_CFG_REG(idx), regVal);
	return MV_OK;
}

MV_STATUS   mvPp2PmeDsaSrcDevSet(MV_U8 src)
{
	MV_U32 regVal = 0;

	regVal &= ~MV_PP2_PME_DSA_SRC_DEV_ALL_MASK;
	regVal |= MV_PP2_PME_DSA_SRC_DEV_MASK(src);
	mvPp2WrReg(MV_PP2_PME_DEF_DSA_SRC_DEV_REG, regVal);
	return MV_OK;
}

MV_STATUS   mvPp2PmeTtlZeroSet(int forward)
{
	MV_U32 regVal = 0;

	regVal |= MV_PP2_PME_TTL_ZERO_FRWD_MASK;
	mvPp2WrReg(MV_PP2_PME_TTL_ZERO_FRWD_REG, regVal);
	return MV_OK;
}

MV_STATUS   mvPp2PmePppoeEtypeSet(MV_U16 ethertype)
{
	mvPp2WrReg(MV_PP2_PME_PPPOE_ETYPE_REG, (MV_U32)ethertype);
	return MV_OK;
}

MV_STATUS   mvPp2PmePppoeLengthSet(MV_U16 length)
{
	mvPp2WrReg(MV_PP2_PME_PPPOE_LEN_REG, (MV_U32)length);
	return MV_OK;
}

MV_STATUS   mvPp2PmePppoeConfig(MV_U8 version, MV_U8 type, MV_U8 code)
{
	MV_U32 regVal = 0;

	regVal |= MV_PP2_PME_PPPOE_VER_MASK(version);
	regVal |= MV_PP2_PME_PPPOE_TYPE_MASK(type);
	regVal |= MV_PP2_PME_PPPOE_CODE_MASK(code);

	mvPp2WrReg(MV_PP2_PME_PPPOE_DATA_REG, regVal);

	return MV_OK;
}

MV_STATUS   mvPp2PmePppoeProtoSet(int idx, MV_U16 protocol)
{
	MV_U32 regVal = 0;

	if ((idx < 0) || (idx > 1)) {
		mvOsPrintf("%s: idx %d is out of range [0..1]\n", __func__, idx);
		return MV_BAD_PARAM;
	}
	regVal = mvPp2RdReg(MV_PP2_PME_PPPOE_PROTO_REG);

	regVal &= ~MV_PP2_PME_PPPOE_PROTO_ALL_MASK(idx);
	regVal |= MV_PP2_PME_PPPOE_PROTO_MASK(idx, protocol);
	mvPp2WrReg(MV_PP2_PME_PPPOE_PROTO_REG, regVal);

	return MV_OK;
}

MV_STATUS   mvPp2PmeMaxConfig(int maxsize, int maxinstr, int errdrop)
{
	MV_U32 regVal = 0;

	regVal |= MV_PP2_PME_MAX_INSTR_NUM_MASK(maxinstr);
	regVal |= MV_PP2_PME_MAX_HDR_SIZE_MASK(maxsize);
	if (errdrop)
		regVal |= MV_PP2_PME_DROP_ON_ERR_MASK;

	mvPp2WrReg(MV_PP2_PME_CONFIG_REG, regVal);

	return MV_OK;
}
