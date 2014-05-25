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

#include "mvPp2PrsHw.h"


/*-------------------------------------------------------------------------------*/
/*	Static functions declaretion for internal use 				*/
/*-------------------------------------------------------------------------------*/

static int mvPp2PrsSwSramRiDump(MV_PP2_PRS_ENTRY *pe);
static int mvPp2PrsSwSramAiDump(MV_PP2_PRS_ENTRY *pe);
/*-------------------------------------------------------------------------------*/

PRS_SHADOW_ENTRY  mvPrsShadowTbl[MV_PP2_PRS_TCAM_SIZE];
int  mvPrsFlowIdTbl[MV_PP2_PRS_FLOW_ID_SIZE];
/******************************************************************************
 * Common utilities
 ******************************************************************************/
int mvPp2PrsShadowIsValid(int index)
{
	return mvPrsShadowTbl[index].valid;
}

int mvPp2PrsShadowLu(int index)
{
	return mvPrsShadowTbl[index].lu;
}

int mvPp2PrsShadowUdf(int index)
{
	return mvPrsShadowTbl[index].udf;
}
unsigned int mvPp2PrsShadowRi(int index)
{
	return mvPrsShadowTbl[index].ri;
}

unsigned int mvPp2PrsShadowRiMask(int index)
{
	return mvPrsShadowTbl[index].riMask;
}

void mvPp2PrsShadowUdfSet(int index, int udf)
{
	mvPrsShadowTbl[index].udf = udf;
}

void mvPp2PrsShadowSet(int index, int lu, char *text)
{

	strncpy((char *)mvPrsShadowTbl[index].text, text, PRS_TEXT_SIZE);
	mvPrsShadowTbl[index].text[PRS_TEXT_SIZE - 1] = 0;
	mvPrsShadowTbl[index].valid = MV_TRUE;
	mvPrsShadowTbl[index].lu = lu;
}
void mvPp2PrsShadowLuSet(int index, int lu)
{
	mvPrsShadowTbl[index].lu = lu;
}

void mvPp2PrsShadowRiSet(int index, unsigned int ri, unsigned int riMask)
{
	mvPrsShadowTbl[index].riMask = riMask;
	mvPrsShadowTbl[index].ri = ri;
}

void mvPp2PrsShadowFinSet(int index, MV_BOOL finish)
{
	mvPrsShadowTbl[index].finish = finish;
}

MV_BOOL mvPp2PrsShadowFin(int index)
{
	return mvPrsShadowTbl[index].finish;
}

void mvPp2PrsShadowClear(int index)
{
	mvPrsShadowTbl[index].valid = MV_FALSE;
	mvPrsShadowTbl[index].text[0] = 0;
}

void mvPp2PrsShadowClearAll(void)
{
	int index;

	for (index = 0; index < MV_PP2_PRS_TCAM_SIZE; index++) {
		mvPrsShadowTbl[index].valid = MV_FALSE;
		mvPrsShadowTbl[index].text[0] = 0;
	}

}

int mvPrsFlowIdGet(int flowId)
{
	return mvPrsFlowIdTbl[flowId];
}

void mvPrsFlowIdSet(int flowId)
{
	mvPrsFlowIdTbl[flowId] = MV_TRUE;
}

void mvPrsFlowIdClear(int flowId)
{
	mvPrsFlowIdTbl[flowId] = MV_FALSE;
}

void mvPrsFlowIdClearAll(void)
{
	int index;

	for (index = 0; index < MV_PP2_PRS_FLOW_ID_SIZE; index++)
		mvPrsFlowIdTbl[index] = MV_FALSE;
}

int mvPrsFlowIdDump(void)
{
	int index;

	for (index = 0; index < MV_PP2_PRS_FLOW_ID_SIZE; index++) {
		if (mvPrsFlowIdGet(index) == MV_TRUE)
			mvOsPrintf("Flow ID[%d]: In_USE\n", index);
	}

	return MV_OK;
}

static int mvPp2PrsFirstFreeGet(int from, int to)
{
	int tid;

	for (tid = from; tid <= to; tid++) {
		if (!mvPrsShadowTbl[tid].valid)
			break;
	}
	return tid;
}

static int mvPp2PrsLastFreeGet(int from, int to)
{
	int tid;

	/* Go through the all entires from last to fires */

	for (tid = from; tid >= to; tid--) {
		if (!mvPrsShadowTbl[tid].valid)
			break;
	}
	return tid;
}
/*
* mvPp2PrsTcamFirstFree - seek for free tcam entry
* Return first free TCAM index, seeking from start to end
* If start < end - seek up-->bottom
* If start > end - seek bottom-->up
*/
int mvPp2PrsTcamFirstFree(int start, int end)
{
	int tid;

	if (start < end)
		tid = mvPp2PrsFirstFreeGet(start, end);
	else
		tid =  mvPp2PrsLastFreeGet(start, end);

	if ((tid < MV_PP2_PRS_TCAM_SIZE) && (tid > -1))
		return tid;
	else
		return MV_PRS_OUT_OF_RAGE;
}
/*
 * mvPrsSwAlloc - allocate new prs entry
 * @id: tcam lookup id
 */
MV_PP2_PRS_ENTRY  *mvPp2PrsSwAlloc(unsigned int lu)
{
	MV_PP2_PRS_ENTRY *pe = mvOsMalloc(sizeof(MV_PP2_PRS_ENTRY));

	WARN_OOM(pe == NULL);
	mvPp2PrsSwClear(pe);
	mvPp2PrsSwTcamLuSet(pe, lu);

	return pe;
}

/*-------------------------------------------------------------------------------*/
/*
 * mvPp2PrsSwFree mvPrsSwAlloc - free prs entry
 * @id: tcam lookup id
 */

void mvPp2PrsSwFree(MV_PP2_PRS_ENTRY *pe)
{
	mvOsFree(pe);
}
/*-------------------------------------------------------------------------------*/

int mvPp2PrsHwPortInit(int port, int lu_first, int lu_max, int offs)
{
	int status = MV_OK;

	status = mvPrsHwLkpFirstSet(port, lu_first);
	if (status < 0)
		return status;

	status = mvPrsHwLkpMaxSet(port, lu_max);
	if (status < 0)
		return status;

	status = mvPrsHwLkpFirstOffsSet(port, offs);
	if (status < 0)
		return status;

	return MV_OK;
}


int mvPp2PrsHwRegsDump()
{
	int i;
	char reg_name[100];

	mvPp2PrintReg(MV_PP2_PRS_INIT_LOOKUP_REG, "MV_PP2_PRS_INIT_LOOKUP_REG");
	mvPp2PrintReg(MV_PP2_PRS_INIT_OFFS_0_REG, "MV_PP2_PRS_INIT_OFFS_0_REG");
	mvPp2PrintReg(MV_PP2_PRS_INIT_OFFS_1_REG, "MV_PP2_PRS_INIT_OFFS_1_REG");
	mvPp2PrintReg(MV_PP2_PRS_MAX_LOOP_0_REG, "MV_PP2_PRS_MAX_LOOP_0_REG");
	mvPp2PrintReg(MV_PP2_PRS_MAX_LOOP_1_REG, "MV_PP2_PRS_MAX_LOOP_1_REG");

	mvPp2PrintReg(MV_PP2_PRS_INTR_CAUSE_REG, "MV_PP2_PRS_INTR_CAUSE_REG");
	mvPp2PrintReg(MV_PP2_PRS_INTR_MASK_REG, "MV_PP2_PRS_INTR_MASK_REG");
	mvPp2PrintReg(MV_PP2_PRS_TCAM_IDX_REG, "MV_PP2_PRS_TCAM_IDX_REG");

	for (i = 0; i < MV_PP2_PRC_TCAM_WORDS; i++) {
		mvOsSPrintf(reg_name, "MV_PP2_PRS_TCAM_DATA_%d_REG", i);
		mvPp2PrintReg(MV_PP2_PRS_TCAM_DATA_REG(i), reg_name);
	}
	mvPp2PrintReg(MV_PP2_PRS_SRAM_IDX_REG, "MV_PP2_PRS_SRAM_IDX_REG");

	for (i = 0; i < MV_PP2_PRC_SRAM_WORDS; i++) {
		mvOsSPrintf(reg_name, "MV_PP2_PRS_SRAM_DATA_%d_REG", i);
		mvPp2PrintReg(MV_PP2_PRS_SRAM_DATA_REG(i), reg_name);
	}

	mvPp2PrintReg(MV_PP2_PRS_EXP_REG, "MV_PP2_PRS_EXP_REG");
	mvPp2PrintReg(MV_PP2_PRS_TCAM_CTRL_REG, "MV_PP2_PRS_TCAM_CTRL_REG");

	return MV_OK;
}


int mvPrsHwLkpFirstSet(int port, int lu_first)
{
	unsigned int regVal;

	POS_RANGE_VALIDATE(lu_first, MV_PP2_PRS_PORT_LU_MAX);
	regVal = mvPp2RdReg(MV_PP2_PRS_INIT_LOOKUP_REG);
	regVal &= ~MV_PP2_PRS_PORT_LU_MASK(port);
	regVal |=  MV_PP2_PRS_PORT_LU_VAL(port, lu_first);
	mvPp2WrReg(MV_PP2_PRS_INIT_LOOKUP_REG, regVal);

	return MV_OK;
}

int mvPrsHwLkpMaxSet(int port, int lu_max)
{
	unsigned int regVal;

	RANGE_VALIDATE(lu_max, MV_PP2_PRS_MAX_LOOP_MIN, MV_PP2_PRS_PORT_LU_MAX);

	regVal = mvPp2RdReg(MV_PP2_PRS_MAX_LOOP_REG(port));
	regVal &= ~MV_PP2_PRS_MAX_LOOP_MASK(port);
	regVal |= MV_PP2_PRS_MAX_LOOP_VAL(port, lu_max);
	mvPp2WrReg(MV_PP2_PRS_MAX_LOOP_REG(port), regVal);

	return MV_OK;
}

int mvPrsHwLkpFirstOffsSet(int port, int off)
{
	unsigned int regVal;
	/* todo if port > 7 return error */

	POS_RANGE_VALIDATE(off, MV_PP2_PRS_INIT_OFF_MAX);

	regVal = mvPp2RdReg(MV_PP2_PRS_INIT_OFFS_REG(port));
	regVal &= ~MV_PP2_PRS_INIT_OFF_MASK(port);
	regVal |= MV_PP2_PRS_INIT_OFF_VAL(port, off);
	mvPp2WrReg(MV_PP2_PRS_INIT_OFFS_REG(port), regVal);

	return MV_OK;
}

/*
	user responsibility to check valid bit after the call to this function
	this function will return OK even if the entry is invalid
*/
int mvPp2PrsHwRead(MV_PP2_PRS_ENTRY *pe)
{
	int index, tid;

	PTR_VALIDATE(pe);
	POS_RANGE_VALIDATE(pe->index, MV_PP2_PRS_TCAM_SIZE - 1);

	tid = pe->index;
/*
	mvOsPrintf("start read parser entry %d \n",tid);
*/
	/* write index */
	mvPp2WrReg(MV_PP2_PRS_TCAM_IDX_REG, tid);
	pe->tcam.word[TCAM_INV_WORD] = mvPp2RdReg(MV_PP2_PRS_TCAM_DATA5_REG);

	if ((pe->tcam.word[TCAM_INV_WORD] & TCAM_INV_MASK) != TCAM_VALID)
		/* Invalid entry */
		return MV_OK;

	for (index = 0; index < MV_PP2_PRC_TCAM_WORDS; index++)
		pe->tcam.word[index] = mvPp2RdReg(MV_PP2_PRS_TCAM_DATA_REG(index));

	/* write index */
	mvPp2WrReg(MV_PP2_PRS_SRAM_IDX_REG, tid);

	for (index = 0; index < MV_PP2_PRC_SRAM_WORDS; index++)
		pe->sram.word[index] = mvPp2RdReg(MV_PP2_PRS_SRAM_DATA_REG(index));
/*
	mvOsPrintf("end read parser entry %d \n",tid);
*/
	return MV_OK;
}

/*
write entry SRAM + TCAM,
*/
int mvPp2PrsHwWrite(MV_PP2_PRS_ENTRY *pe)
{
	int index, tid;

	PTR_VALIDATE(pe);
/*
	mvOsPrintf("Write parser entry %d - start\n",pe->index);
*/
	POS_RANGE_VALIDATE(pe->index, MV_PP2_PRS_TCAM_SIZE - 1);
	tid = pe->index;

	/*clear invalid bit*/
	pe->tcam.word[TCAM_INV_WORD] &= ~TCAM_INV_MASK;

	/* write index */
	mvPp2WrReg(MV_PP2_PRS_TCAM_IDX_REG, tid);

	for (index = 0; index < MV_PP2_PRC_TCAM_WORDS; index++)
		mvPp2WrReg(MV_PP2_PRS_TCAM_DATA_REG(index), pe->tcam.word[index]);

	/* write index */
	mvPp2WrReg(MV_PP2_PRS_SRAM_IDX_REG, tid);

	for (index = 0; index < MV_PP2_PRC_SRAM_WORDS; index++)
		mvPp2WrReg(MV_PP2_PRS_SRAM_DATA_REG(index), pe->sram.word[index]);
/*
	mvOsPrintf("Write parser entry %d - end\n",pe->index);
*/
	return MV_OK;
}

/* Read tcam hit counter*/
/* PPv2.1 MASS 3.20 new feature */
/* return tcam entry (tid) hits counter or error if tid is out of range */
static int mvPp2V1PrsHwTcamCntDump(int tid, unsigned int *cnt)
{
	unsigned int regVal;

	POS_RANGE_VALIDATE(tid, MV_PP2_PRS_TCAM_SIZE - 1);

	/* write index */
	mvPp2WrReg(MV_PP2_PRS_TCAM_HIT_IDX_REG, tid);

	regVal = mvPp2RdReg(MV_PP2_PRS_TCAM_HIT_CNT_REG);
	regVal &= MV_PP2_PRS_TCAM_HIT_CNT_MASK;

	if (cnt)
		*cnt = regVal;
	else
		mvOsPrintf("HIT COUNTER: %d\n", regVal);

	return MV_OK;
}
/* mvPp2PrsHwHitsDump - dump all non zeroed hit counters and the associated TCAM entries */
/* PPv2.1 MASS 3.20 new feature */
int mvPp2V1PrsHwHitsDump()
{
	int index;
	unsigned int cnt;
	MV_PP2_PRS_ENTRY pe;

	for (index = 0; index < MV_PP2_PRS_TCAM_SIZE; index++) {
		pe.index = index;
		mvPp2PrsHwRead(&pe);
		if ((pe.tcam.word[TCAM_INV_WORD] & TCAM_INV_MASK) == TCAM_VALID) {
			mvPp2V1PrsHwTcamCntDump(index, &cnt);

			if (cnt == 0)
				continue;

			mvOsPrintf("%s\n", mvPrsShadowTbl[index].text);
			mvPp2PrsSwDump(&pe);
			mvOsPrintf("       HITS: %d\n", cnt);
			mvOsPrintf("-------------------------------------------------------------------------\n");
		}
	}

	return MV_OK;
}

/* delete hw entry (set as invalid) */
int mvPp2PrsHwInv(int tid)
{
	POS_RANGE_VALIDATE(tid, MV_PP2_PRS_TCAM_SIZE - 1);

	/* write index */
	mvPp2WrReg(MV_PP2_PRS_TCAM_IDX_REG, tid);
	/* write invalid */
	mvPp2WrReg(MV_PP2_PRS_TCAM_DATA_REG(TCAM_INV_WORD), TCAM_INV_MASK);

	return MV_OK;
}

/* delete all hw entry (set all as invalid) */
int mvPp2PrsHwInvAll()
{
	int index;

	for (index = 0; index < MV_PP2_PRS_TCAM_SIZE; index++)
		mvPp2PrsHwInv(index);

	return MV_OK;
}

int mvPp2PrsHwClearAll()
{
	int index, i;

	for (index = 0; index < MV_PP2_PRS_TCAM_SIZE; index++) {
		mvPp2WrReg(MV_PP2_PRS_TCAM_IDX_REG, index);

		for (i = 0; i < MV_PP2_PRC_TCAM_WORDS; i++)
			mvPp2WrReg(MV_PP2_PRS_TCAM_DATA_REG(i), 0);

		mvPp2WrReg(MV_PP2_PRS_SRAM_IDX_REG, index);

		for (i = 0; i < MV_PP2_PRC_SRAM_WORDS; i++)
			mvPp2WrReg(MV_PP2_PRS_SRAM_DATA_REG(i), 0);
	}

	return MV_OK;
}

int mvPp2PrsHwDump()
{
	int index;
	MV_PP2_PRS_ENTRY pe;

	for (index = 0; index < MV_PP2_PRS_TCAM_SIZE; index++) {
		pe.index = index;
		mvPp2PrsHwRead(&pe);
		if ((pe.tcam.word[TCAM_INV_WORD] & TCAM_INV_MASK) == TCAM_VALID) {
			mvOsPrintf("%s\n", mvPrsShadowTbl[index].text);
			mvPp2PrsSwDump(&pe);
#ifdef MV_ETH_PPV2_1
			mvPp2V1PrsHwTcamCntDump(index);
#endif
			mvOsPrintf("-------------------------------------------------------------------------\n");
		}
	}

	return MV_OK;
}

int mvPp2PrsSwDump(MV_PP2_PRS_ENTRY *pe)
{
	MV_U32	op, type, lu, done, flowid;
	int	shift, offset, i;

	PTR_VALIDATE(pe);

	/* hw entry id */
	mvOsPrintf("[%4d] ", pe->index);

	i = MV_PP2_PRC_TCAM_WORDS - 1;
	mvOsPrintf("%1.1x ", pe->tcam.word[i--] & 0xF);

	while (i >= 0)
		mvOsPrintf("%4.4x ", (pe->tcam.word[i--]) & 0xFFFF);

	mvOsPrintf("| ");

	mvOsPrintf(PRS_SRAM_FMT, PRS_SRAM_VAL(pe->sram.word));

	mvOsPrintf("\n       ");

	i = MV_PP2_PRC_TCAM_WORDS - 1;
	mvOsPrintf("%1.1x ", (pe->tcam.word[i--] >> 16) & 0xF);

	while (i >= 0)
		mvOsPrintf("%4.4x ", ((pe->tcam.word[i--]) >> 16)  & 0xFFFF);

	mvOsPrintf("| ");

	mvPp2PrsSwSramShiftGet(pe, &shift);
	mvOsPrintf("SH=%d ", shift);

	mvPp2PrsSwSramOffsetGet(pe, &type, &offset, &op);
	if (offset != 0)
		mvOsPrintf("UDFT=%u UDFO=%d ", type, offset);

	mvOsPrintf("op=%u ", op);

	mvPp2PrsSwSramNextLuGet(pe, &lu);
	mvOsPrintf("LU=%u ", lu);

	mvPp2PrsSwSramLuDoneGet(pe, &done);
	mvOsPrintf("%s ", done ? "DONE" : "N_DONE");

	/*flow id generation bit*/
	mvPp2PrsSwSramFlowidGenGet(pe, &flowid);
	mvOsPrintf("%s ", flowid ? "FIDG" : "N_FIDG");

	(pe->tcam.word[TCAM_INV_WORD] & TCAM_INV_MASK) ? mvOsPrintf(" [inv]") : 0;

	if (mvPp2PrsSwSramRiDump(pe))
		return MV_ERROR;

	if (mvPp2PrsSwSramAiDump(pe))
		return MV_ERROR;

	mvOsPrintf("\n");

	return MV_OK;

}

void mvPp2PrsSwClear(MV_PP2_PRS_ENTRY *pe)
{
	memset(pe, 0, sizeof(MV_PP2_PRS_ENTRY));
}

/*
	enable - Tcam Ebable/Disable
*/

int mvPp2PrsSwTcam(int enable)
{
	POS_RANGE_VALIDATE(enable, 1);

	mvPp2WrReg(MV_PP2_PRS_TCAM_CTRL_REG, enable);

	return MV_OK;
}
/*
	byte - data to tcam entry
	enable - data to tcam enable endtry
*/
int mvPp2PrsSwTcamByteSet(MV_PP2_PRS_ENTRY *pe, unsigned int offs, unsigned char byte, unsigned char enable)
{
	PTR_VALIDATE(pe);
	POS_RANGE_VALIDATE(offs, TCAM_DATA_MAX);

	pe->tcam.byte[TCAM_DATA_BYTE(offs)] = byte;
	pe->tcam.byte[TCAM_DATA_MASK(offs)] = enable;

	return MV_OK;
}

/*  get byte from entry structure MV_PP2_PRS_ENTRY (sw)
	byte - data of tcam entry
	enable - data of tcam enable entry
*/
int mvPp2PrsSwTcamByteGet(MV_PP2_PRS_ENTRY *pe, unsigned int offs, unsigned char *byte, unsigned char *enable)
{

	PTR_VALIDATE(pe);
	PTR_VALIDATE(byte);
	PTR_VALIDATE(enable);

	POS_RANGE_VALIDATE(offs, TCAM_DATA_MAX);

	*byte = pe->tcam.byte[TCAM_DATA_BYTE(offs)];
	*enable = pe->tcam.byte[TCAM_DATA_MASK(offs)];

	return MV_OK;
}

int mvPp2PrsSwTcamWordSet(MV_PP2_PRS_ENTRY *pe, unsigned int offs, unsigned int word, unsigned int mask)
{
	int index, offset;
	unsigned char byte, byteMask;

	PTR_VALIDATE(pe);
	POS_RANGE_VALIDATE(offs, TCAM_DATA_WORD_MAX);

#if defined(MV_CPU_BE)
	word = MV_BYTE_SWAP_32BIT(word);
	mask = MV_BYTE_SWAP_32BIT(mask);
#endif
	for (index = 0; index < DWORD_BYTES_LEN; index++) {

		offset = (offs * DWORD_BYTES_LEN) + index;
		byte = ((unsigned char *) &word)[index];
		byteMask = ((unsigned char *) &mask)[index];

		mvPp2PrsSwTcamByteSet(pe, offset, byte, byteMask);
	}

	return MV_OK;
}

int mvPp2PrsSwTcamWordGet(MV_PP2_PRS_ENTRY *pe, unsigned int offs, unsigned int *word, unsigned int *enable)
{
	int index, offset;
	unsigned char byte, mask;

	PTR_VALIDATE(pe);
	PTR_VALIDATE(word);
	PTR_VALIDATE(enable);

	POS_RANGE_VALIDATE(offs, TCAM_DATA_WORD_MAX);

	for (index = 0; index < DWORD_BYTES_LEN; index++) {
		offset = (offs * DWORD_BYTES_LEN) + index;
		mvPp2PrsSwTcamByteGet(pe, offset,  &byte, &mask);
		((unsigned char *) word)[index] = byte;
		((unsigned char *) enable)[index] = mask;
	}

	return MV_OK;
}

/* compare in sw.
	return EQUALS if tcam_data[off]&tcam_mask[off] = byte

	user should call hw_read before.!!!!!
*/
int mvPp2PrsSwTcamByteCmp(MV_PP2_PRS_ENTRY *pe, unsigned int offs, unsigned char byte)
{
	unsigned char tcamByte, tcamMask;

	PTR_VALIDATE(pe);

	if (mvPp2PrsSwTcamByteGet(pe, offs, &tcamByte, &tcamMask) != MV_OK)
		return MV_PRS_ERR;

	if ((tcamByte & tcamMask) == (byte & tcamMask))
		return EQUALS;

	return NOT_EQUALS;
}

/* compare in sw.
	enable data according to corresponding enable bits in  MV_PP2_PRS_ENTRY
	user should call hw_read before.!!!!!
	byte - data of tcam entry
	enable - data of tcam enable endtry
	return 0 if equals ..else return 1
	return MV_PRS_ERR if falied !
*/

int mvPp2PrsSwTcamBytesCmp(MV_PP2_PRS_ENTRY *pe, unsigned int offs, unsigned int size, unsigned char *bytes)
{
	int status, index;

	PTR_VALIDATE(pe);

	POS_RANGE_VALIDATE(offs + size, TCAM_DATA_SIZE);

	for (index = 0; index < size; index++) {
		status = mvPp2PrsSwTcamByteCmp(pe, offs + index, bytes[index]);
		if (status != EQUALS)
			return status;
	}
	return EQUALS;
}

int mvPp2PrsSwTcamBytesIgnorMaskCmp(MV_PP2_PRS_ENTRY *pe, unsigned int offs, unsigned int size, unsigned char *bytes)
{
	int		index;
	unsigned char 	tcamByte, tcamMask;

	PTR_VALIDATE(pe);
	POS_RANGE_VALIDATE(offs + size, TCAM_DATA_SIZE);

	for (index = 0; index < size; index++) {
		mvPp2PrsSwTcamByteGet(pe, offs + index, &tcamByte, &tcamMask);

		if (tcamByte != bytes[index])
			return NOT_EQUALS;
	}
	return EQUALS;
}



int mvPp2PrsSwTcamAiUpdate(MV_PP2_PRS_ENTRY *pe, unsigned int bits, unsigned int enable)
{
	int i;

	PTR_VALIDATE(pe);
	POS_RANGE_VALIDATE(bits, AI_MASK);
	POS_RANGE_VALIDATE(enable, AI_MASK);

	for (i = 0; i < AI_BITS; i++)
		if (enable & (1 << i)) {
			if (bits & (1 << i))
				pe->tcam.byte[TCAM_AI_BYTE] |= (1 << i);
			else
				pe->tcam.byte[TCAM_AI_BYTE] &= ~(1 << i);
		}

	pe->tcam.byte[TCAM_MASK_OFFS(TCAM_AI_BYTE)] |= enable;
	return MV_OK;
}

int mvPp2PrsSwTcamAiGet(MV_PP2_PRS_ENTRY *pe, unsigned int *bits, unsigned int *enable)
{
	PTR_VALIDATE(pe);
	PTR_VALIDATE(bits);
	PTR_VALIDATE(enable);

	*bits = pe->tcam.byte[TCAM_AI_BYTE];
	*enable = pe->tcam.byte[TCAM_MASK_OFFS(TCAM_AI_BYTE)];

	return MV_OK;
}

int mvPp2PrsSwTcamPortSet(MV_PP2_PRS_ENTRY *pe, unsigned int port, int add)
{
	PTR_VALIDATE(pe);
	POS_RANGE_VALIDATE(port, 7);/*TODO define max port val*/
	POS_RANGE_VALIDATE(add, 1);

	if (add == 1)
		pe->tcam.byte[TCAM_MASK_OFFS(TCAM_PORT_BYTE)] &= ~(1 << port);
	else
		pe->tcam.byte[TCAM_MASK_OFFS(TCAM_PORT_BYTE)] |= (1 << port);

	return MV_OK;
}

int mvPp2PrsSwTcamPortGet(MV_PP2_PRS_ENTRY *pe, unsigned int port, MV_BOOL *status)
{
	PTR_VALIDATE(pe);
	POS_RANGE_VALIDATE(port, 7);/*TODO define max port val*/

	if (~(pe->tcam.byte[TCAM_MASK_OFFS(TCAM_PORT_BYTE)]) & (1 << port))
		*status = MV_TRUE;
	else
		*status = MV_FALSE;

	return MV_OK;
}

int mvPp2PrsSwTcamPortMapSet(MV_PP2_PRS_ENTRY *pe, unsigned int ports)
{
	PTR_VALIDATE(pe);
	POS_RANGE_VALIDATE(ports, PORT_MASK);

	pe->tcam.byte[TCAM_PORT_BYTE] = 0;
	pe->tcam.byte[TCAM_MASK_OFFS(TCAM_PORT_BYTE)] &= (unsigned char)(~PORT_MASK);
	pe->tcam.byte[TCAM_MASK_OFFS(TCAM_PORT_BYTE)] |= ((~ports) & PORT_MASK);

	return MV_OK;
}
int mvPp2PrsSwTcamPortMapGet(MV_PP2_PRS_ENTRY *pe, unsigned int *ports)
{
	PTR_VALIDATE(pe);
	PTR_VALIDATE(ports);

	*ports = (~pe->tcam.byte[TCAM_MASK_OFFS(TCAM_PORT_BYTE)]) & PORT_MASK;

	return MV_OK;
}


int mvPp2PrsSwTcamLuSet(MV_PP2_PRS_ENTRY *pe, unsigned int lu)
{

	PTR_VALIDATE(pe);
	POS_RANGE_VALIDATE(lu, LU_MASK);

	pe->tcam.byte[TCAM_LU_BYTE] = lu;
	pe->tcam.byte[TCAM_MASK_OFFS(TCAM_LU_BYTE)] = LU_MASK;

	return MV_OK;
}

int mvPp2PrsSwTcamLuGet(MV_PP2_PRS_ENTRY *pe, unsigned int *lu, unsigned int *enable)
{
	PTR_VALIDATE(pe);
	PTR_VALIDATE(lu);
	PTR_VALIDATE(enable);

	*lu = (pe->tcam.byte[TCAM_LU_BYTE]) & LU_MASK;
	*enable = (pe->tcam.byte[TCAM_MASK_OFFS(TCAM_LU_BYTE)]) & LU_MASK;

	return MV_OK;
}

int mvPp2PrsSwSramRiSetBit(MV_PP2_PRS_ENTRY *pe, unsigned int bit)
{
	PTR_VALIDATE(pe);

	POS_RANGE_VALIDATE(bit, (SRAM_RI_BITS - 1));

	pe->sram.word[SRAM_RI_WORD] |= (1 << bit);
	pe->sram.word[SRAM_RI_CTRL_WORD] |= (1 << bit);


	return MV_OK;
}

int mvPp2PrsSwSramRiClearBit(MV_PP2_PRS_ENTRY *pe, unsigned int bit)
{
	PTR_VALIDATE(pe);

	POS_RANGE_VALIDATE(bit, (SRAM_RI_BITS-1));

	pe->sram.word[SRAM_RI_OFFS] &= ~(1 << bit);
	pe->sram.word[SRAM_RI_CTRL_WORD] |= (1 << bit);

	return MV_OK;
}


/* set RI and RI_UPDATE */
int mvPp2PrsSwSramRiUpdate(MV_PP2_PRS_ENTRY *pe, unsigned int bits, unsigned int enable)
{
/* ALTERNATIVE WAY:
   find the bist that set in defRiMask and clear in riMask
   maskDiff = defRiMask & (defRiMask ^ riMask);
   update 1's: ri |= (defRi & maskDiff);
   update 0's: ri &= ~(maskDiff & (~defRi));
   update mask: riMask |= defRiMask;
*/

	unsigned int i;

	PTR_VALIDATE(pe);


	for (i = 0; i < SRAM_RI_BITS; i++) {
		if (enable & (1 << i)) {
			if (bits & (1 << i))
				mvPp2PrsSwSramRiSetBit(pe, i);
			else
				mvPp2PrsSwSramRiClearBit(pe, i);
		}
	}
	return MV_OK;
}

/* return RI and RI_UPDATE */
int mvPp2PrsSwSramRiGet(MV_PP2_PRS_ENTRY *pe, unsigned int *bits, unsigned int *enable)
{
	PTR_VALIDATE(pe);
	PTR_VALIDATE(bits);
	PTR_VALIDATE(enable);

	*bits = pe->sram.word[SRAM_RI_OFFS/32];
	*enable = pe->sram.word[SRAM_RI_CTRL_OFFS/32];
	return MV_OK;
}

int mvPp2PrsSwSramRiSet(MV_PP2_PRS_ENTRY *pe, unsigned int bits, unsigned int enable)
{
	PTR_VALIDATE(pe);

	pe->sram.word[SRAM_RI_OFFS/32] = bits;
	pe->sram.word[SRAM_RI_CTRL_OFFS/32] = enable;
	return MV_OK;
}

static int mvPp2PrsSwSramRiDump(MV_PP2_PRS_ENTRY *pe)
{
	unsigned int data, mask;
	int i, off = 0, bitsOffs = 0;
	char bits[100];

	PTR_VALIDATE(pe);

	mvPp2PrsSwSramRiGet(pe, &data, &mask);
	if (mask == 0)
		return off;

	mvOsPrintf("\n       ");

	mvOsPrintf("S_RI=");
	for (i = (SRAM_RI_BITS-1); i > -1 ; i--)
		if (mask & (1 << i)) {
			mvOsPrintf("%d", ((data & (1 << i)) != 0));
			bitsOffs += mvOsSPrintf(bits + bitsOffs, "%d:", i);
		} else
			mvOsPrintf("x");

	bits[bitsOffs] = '\0';
	mvOsPrintf(" %s", bits);

	return MV_OK;
}

int mvPp2PrsSwSramAiSetBit(MV_PP2_PRS_ENTRY *pe, unsigned char bit)
{
	PTR_VALIDATE(pe);
	POS_RANGE_VALIDATE(bit, (SRAM_AI_CTRL_BITS - 1));

	pe->sram.byte[SRAM_BIT_TO_BYTE(SRAM_AI_OFFS + bit)] |= (1  << ((SRAM_AI_OFFS + bit) % 8));
	pe->sram.byte[SRAM_BIT_TO_BYTE(SRAM_AI_CTRL_OFFS + bit)] |= (1  << ((SRAM_AI_CTRL_OFFS + bit) % 8));

	return MV_OK;
}

int mvPp2PrsSwSramAiClearBit(MV_PP2_PRS_ENTRY *pe, unsigned char bit)
{
	PTR_VALIDATE(pe);
	POS_RANGE_VALIDATE(bit, (SRAM_AI_CTRL_BITS - 1));

	pe->sram.byte[SRAM_BIT_TO_BYTE(SRAM_AI_OFFS + bit)] &= ~(1  << ((SRAM_AI_OFFS + bit) % 8));
	pe->sram.byte[SRAM_BIT_TO_BYTE(SRAM_AI_CTRL_OFFS + bit)] |= (1  << ((SRAM_AI_CTRL_OFFS + bit) % 8));

	return MV_OK;
}


int mvPp2PrsSwSramAiUpdate(MV_PP2_PRS_ENTRY *pe, unsigned int bits, unsigned int mask)
{
	unsigned int i;

	PTR_VALIDATE(pe);

	POS_RANGE_VALIDATE(bits, AI_MASK);
	POS_RANGE_VALIDATE(mask, AI_MASK);

	for (i = 0; i < SRAM_AI_CTRL_BITS; i++)
		if (mask & (1 << i)) {
			if (bits & (1 << i))
				mvPp2PrsSwSramAiSetBit(pe, i);
			else
				mvPp2PrsSwSramAiClearBit(pe, i);
		}
	return MV_OK;
}


/* return AI and AI_UPDATE */
int mvPp2PrsSwSramAiGet(MV_PP2_PRS_ENTRY *pe, unsigned int *bits, unsigned int *enable)
{

	PTR_VALIDATE(pe);
	PTR_VALIDATE(bits);
	PTR_VALIDATE(enable);

	*bits = (pe->sram.byte[SRAM_BIT_TO_BYTE(SRAM_AI_OFFS)] >> (SRAM_AI_OFFS % 8)) |
		(pe->sram.byte[SRAM_BIT_TO_BYTE(SRAM_AI_OFFS+SRAM_AI_CTRL_BITS)] << (8 - (SRAM_AI_OFFS % 8)));

	*enable = (pe->sram.byte[SRAM_BIT_TO_BYTE(SRAM_AI_CTRL_OFFS)] >> (SRAM_AI_CTRL_OFFS % 8)) |
			(pe->sram.byte[SRAM_BIT_TO_BYTE(SRAM_AI_CTRL_OFFS+SRAM_AI_CTRL_BITS)] <<
				(8 - (SRAM_AI_CTRL_OFFS % 8)));

	*bits &= SRAM_AI_MASK;
	*enable &= SRAM_AI_MASK;

	return MV_OK;
}

static int mvPp2PrsSwSramAiDump(MV_PP2_PRS_ENTRY *pe)
{
	int i, bitsOffs = 0;
	unsigned int data, mask;
	char bits[30];

	PTR_VALIDATE(pe);

	mvPp2PrsSwSramAiGet(pe, &data, &mask);

	if (mask == 0)
		return MV_OK;

	mvOsPrintf("\n       ");

	mvOsPrintf("S_AI=");
	for (i = (SRAM_AI_CTRL_BITS-1); i > -1 ; i--)
		if (mask & (1 << i)) {
			mvOsPrintf("%d", ((data & (1 << i)) != 0));
			bitsOffs += mvOsSPrintf(bits + bitsOffs, "%d:", i);
		} else
			mvOsPrintf("x");
	bits[bitsOffs] = '\0';
	mvOsPrintf(" %s", bits);

	return MV_OK;
}

int mvPp2PrsSwSramNextLuSet(MV_PP2_PRS_ENTRY *pe, unsigned int lu)
{
	PTR_VALIDATE(pe);

	POS_RANGE_VALIDATE(lu, SRAM_NEXT_LU_MASK);

	pe->sram.byte[SRAM_BIT_TO_BYTE(SRAM_NEXT_LU_OFFS)] &= ~(SRAM_NEXT_LU_MASK << (SRAM_NEXT_LU_OFFS % 8));
	pe->sram.byte[SRAM_BIT_TO_BYTE(SRAM_NEXT_LU_OFFS)] |= (lu << (SRAM_NEXT_LU_OFFS % 8));
	return MV_OK;
}

int mvPp2PrsSwSramNextLuGet(MV_PP2_PRS_ENTRY *pe, unsigned int *lu)
{
	PTR_VALIDATE(pe);
	PTR_VALIDATE(lu);

	*lu = pe->sram.byte[SRAM_BIT_TO_BYTE(SRAM_NEXT_LU_OFFS)];
	*lu = ((*lu) >> SRAM_NEXT_LU_OFFS % 8);
	*lu &= SRAM_NEXT_LU_MASK;
	return MV_OK;
}

/* shift to (current offset + shift) */
int mvPp2PrsSwSramShiftSet(MV_PP2_PRS_ENTRY *pe, int shift, unsigned int op)
{
	PTR_VALIDATE(pe);
	RANGE_VALIDATE(shift, 0 - SRAM_SHIFT_MASK, SRAM_SHIFT_MASK);
	POS_RANGE_VALIDATE(op, SRAM_OP_SEL_SHIFT_MASK);

	/* Set sign */
	if (shift < 0) {
		pe->sram.byte[SRAM_BIT_TO_BYTE(SRAM_SHIFT_SIGN_BIT)] |= (1 << (SRAM_SHIFT_SIGN_BIT % 8));
		shift = 0 - shift;
	} else
		pe->sram.byte[SRAM_BIT_TO_BYTE(SRAM_SHIFT_SIGN_BIT)] &= ~(1 << (SRAM_SHIFT_SIGN_BIT % 8));

	/* Set offset */
	pe->sram.byte[SRAM_BIT_TO_BYTE(SRAM_SHIFT_OFFS)] = (unsigned char)shift;

	/* Reset and Set operation */
	pe->sram.byte[SRAM_BIT_TO_BYTE(SRAM_OP_SEL_SHIFT_OFFS)] &=
		~(SRAM_OP_SEL_SHIFT_MASK << (SRAM_OP_SEL_SHIFT_OFFS % 8));

	pe->sram.byte[SRAM_BIT_TO_BYTE(SRAM_OP_SEL_SHIFT_OFFS)] |= (op << (SRAM_OP_SEL_SHIFT_OFFS % 8));

	/* Set base offset as current */
	pe->sram.byte[SRAM_BIT_TO_BYTE(SRAM_OP_SEL_BASE_OFFS)] &= ~(1 << (SRAM_OP_SEL_BASE_OFFS % 8));

	return MV_OK;
}

int mvPp2PrsSwSramShiftGet(MV_PP2_PRS_ENTRY *pe, int *shift)
{
	int sign;

	PTR_VALIDATE(pe);
	PTR_VALIDATE(shift);

	sign = pe->sram.byte[SRAM_BIT_TO_BYTE(SRAM_SHIFT_SIGN_BIT)] & (1 << (SRAM_SHIFT_SIGN_BIT % 8));
	*shift = ((int)(pe->sram.byte[SRAM_BIT_TO_BYTE(SRAM_SHIFT_OFFS)])) & SRAM_SHIFT_MASK;

	if (sign == 1)
		*shift *= -1;

	return MV_OK;
}
/* shift to (InitOffs + shift) */
int mvPp2PrsSwSramShiftAbsUpdate(MV_PP2_PRS_ENTRY *pe, int shift, unsigned int op)
{
	mvPp2PrsSwSramShiftSet(pe, shift, op);

	/* Set base offset as initial */
	pe->sram.byte[SRAM_BIT_TO_BYTE(SRAM_OP_SEL_BASE_OFFS)] |= (1 << (SRAM_OP_SEL_BASE_OFFS % 8));

	return MV_OK;
}


int mvPp2PrsSwSramOffsetSet(MV_PP2_PRS_ENTRY *pe, unsigned int type, int offset, unsigned int op)
{
	PTR_VALIDATE(pe);

	RANGE_VALIDATE(offset, 0 - SRAM_OFFSET_MASK, SRAM_OFFSET_MASK);
	POS_RANGE_VALIDATE(type, SRAM_OFFSET_TYPE_MASK);
	POS_RANGE_VALIDATE(op, SRAM_OP_SEL_OFFSET_MASK);

	/* Set offset sign */
	if (offset < 0) {
		offset = 0 - offset;
		/* set sram offset sign bit */
		pe->sram.byte[SRAM_BIT_TO_BYTE(SRAM_OFFSET_SIGN_BIT)] |= (1 << (SRAM_OFFSET_SIGN_BIT % 8));
	} else
		pe->sram.byte[SRAM_BIT_TO_BYTE(SRAM_OFFSET_SIGN_BIT)] &= ~(1 << (SRAM_OFFSET_SIGN_BIT % 8));

	/* set offset value */
	pe->sram.byte[SRAM_BIT_TO_BYTE(SRAM_OFFSET_OFFS)] &= ~(SRAM_OFFSET_MASK << (SRAM_OFFSET_OFFS % 8));
	pe->sram.byte[SRAM_BIT_TO_BYTE(SRAM_OFFSET_OFFS)] |= (offset << (SRAM_OFFSET_OFFS % 8));
	pe->sram.byte[SRAM_BIT_TO_BYTE(SRAM_OFFSET_OFFS + SRAM_OFFSET_BITS)] &=
		~(SRAM_OFFSET_MASK >> (8 - (SRAM_OFFSET_OFFS % 8)));

	pe->sram.byte[SRAM_BIT_TO_BYTE(SRAM_OFFSET_OFFS + SRAM_OFFSET_BITS)] |=
		(offset >> (8 - (SRAM_OFFSET_OFFS % 8)));

	/* set offset type */
	pe->sram.byte[SRAM_BIT_TO_BYTE(SRAM_OFFSET_TYPE_OFFS)] &=
		~(SRAM_OFFSET_TYPE_MASK << (SRAM_OFFSET_TYPE_OFFS % 8));

	pe->sram.byte[SRAM_BIT_TO_BYTE(SRAM_OFFSET_TYPE_OFFS)] |= (type << (SRAM_OFFSET_TYPE_OFFS % 8));

	/* Set offset operation */
	pe->sram.byte[SRAM_BIT_TO_BYTE(SRAM_OP_SEL_OFFSET_OFFS)] &=
		~(SRAM_OP_SEL_OFFSET_MASK << (SRAM_OP_SEL_OFFSET_OFFS % 8));

	pe->sram.byte[SRAM_BIT_TO_BYTE(SRAM_OP_SEL_OFFSET_OFFS)] |= (op << (SRAM_OP_SEL_OFFSET_OFFS % 8));

	pe->sram.byte[SRAM_BIT_TO_BYTE(SRAM_OP_SEL_OFFSET_OFFS + SRAM_OP_SEL_OFFSET_BITS)] &=
			 ~(SRAM_OP_SEL_OFFSET_MASK >> (8 - (SRAM_OP_SEL_OFFSET_OFFS % 8)));

	pe->sram.byte[SRAM_BIT_TO_BYTE(SRAM_OP_SEL_OFFSET_OFFS + SRAM_OP_SEL_OFFSET_BITS)] |=
			  (op >> (8 - (SRAM_OP_SEL_OFFSET_OFFS % 8)));

	/* Set base offset as current */
	pe->sram.byte[SRAM_BIT_TO_BYTE(SRAM_OP_SEL_BASE_OFFS)] &= ~(1 << (SRAM_OP_SEL_BASE_OFFS % 8));

	return MV_OK;
}

int mvPp2PrsSwSramOffsetGet(MV_PP2_PRS_ENTRY *pe, unsigned int *type, int *offset, unsigned int *op)
{
	int sign;

	PTR_VALIDATE(pe);
	PTR_VALIDATE(offset);
	PTR_VALIDATE(type);

	*type = pe->sram.byte[SRAM_BIT_TO_BYTE(SRAM_OFFSET_TYPE_OFFS)] >> (SRAM_OFFSET_TYPE_OFFS % 8);
	*type &= SRAM_OFFSET_TYPE_MASK;


	*offset = (pe->sram.byte[SRAM_BIT_TO_BYTE(SRAM_OFFSET_OFFS)] >> (SRAM_OFFSET_OFFS % 8)) & 0x7f;
	*offset |= (pe->sram.byte[SRAM_BIT_TO_BYTE(SRAM_OFFSET_OFFS + SRAM_OFFSET_BITS)] <<
			(8 - (SRAM_OFFSET_OFFS % 8))) & 0x80;

	*op = (pe->sram.byte[SRAM_BIT_TO_BYTE(SRAM_OP_SEL_OFFS)] >> (SRAM_OP_SEL_OFFS % 8)) & 0x7;
	*op |= (pe->sram.byte[SRAM_BIT_TO_BYTE(SRAM_OP_SEL_OFFS + SRAM_OP_SEL_BITS)] <<
			(8 - (SRAM_OP_SEL_OFFS % 8))) & 0x18;

	/* if signed bit is tes */
	sign = pe->sram.byte[SRAM_BIT_TO_BYTE(SRAM_OFFSET_SIGN_BIT)] & (1 << (SRAM_OFFSET_SIGN_BIT % 8));
	if (sign != 0)
		*offset = 1-(*offset);

	return MV_OK;
}

int mvPp2PrsSramBitSet(MV_PP2_PRS_ENTRY *pe, int bitNum)
{

	PTR_VALIDATE(pe);

	pe->sram.byte[SRAM_BIT_TO_BYTE(bitNum)] |= (1 << (bitNum % 8));
	return MV_OK;
}

int mvPp2PrsSramBitClear(MV_PP2_PRS_ENTRY *pe, int bitNum)
{
	PTR_VALIDATE(pe);

	pe->sram.byte[SRAM_BIT_TO_BYTE(bitNum)] &= ~(1 << (bitNum % 8));
	return MV_OK;
}

int mvPp2PrsSramBitGet(MV_PP2_PRS_ENTRY *pe, int bitNum, unsigned int *bit)
{
	PTR_VALIDATE(pe);

	*bit = pe->sram.byte[SRAM_BIT_TO_BYTE(bitNum)]  & (1 << (bitNum % 8));
	*bit = (*bit) >> (bitNum % 8);
	return MV_OK;
}

int mvPp2PrsSwSramLuDoneSet(MV_PP2_PRS_ENTRY *pe)
{
	return mvPp2PrsSramBitSet(pe, SRAM_LU_DONE_BIT);
}

int mvPp2PrsSwSramLuDoneClear(MV_PP2_PRS_ENTRY *pe)
{
	return mvPp2PrsSramBitClear(pe, SRAM_LU_DONE_BIT);
}

int mvPp2PrsSwSramLuDoneGet(MV_PP2_PRS_ENTRY *pe, unsigned int *bit)
{
	return mvPp2PrsSramBitGet(pe, SRAM_LU_DONE_BIT, bit);
}

int mvPp2PrsSwSramFlowidGenSet(MV_PP2_PRS_ENTRY *pe)
{
	return mvPp2PrsSramBitSet(pe, SRAM_LU_GEN_BIT);
}

int mvPp2PrsSwSramFlowidGenClear(MV_PP2_PRS_ENTRY *pe)
{
	return mvPp2PrsSramBitClear(pe, SRAM_LU_GEN_BIT);
}

int mvPp2PrsSwSramFlowidGenGet(MV_PP2_PRS_ENTRY *pe, unsigned int *bit)
{
	return mvPp2PrsSramBitGet(pe, SRAM_LU_GEN_BIT, bit);

}

