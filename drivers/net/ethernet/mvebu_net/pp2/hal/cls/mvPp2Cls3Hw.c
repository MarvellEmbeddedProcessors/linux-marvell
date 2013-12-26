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
#include "mvPp2Cls3Hw.h"

CLS3_SHADOW_HASH_ENTRY mvCls3ShadowTbl[MV_PP2_CLS_C3_HASH_TBL_SIZE];
int mvCls3ShadowExtTbl[MV_PP2_CLS_C3_EXT_TBL_SIZE];
static int mvPp2ClsC3SwActDump(MV_PP2_CLS_C3_ENTRY *c3);

static int SwInitCntSet;

/******************************************************************************
 * 			Common utilities

******************************************************************************/
static void mvPp2ClsC3ShadowSet(int hekSize, int index, int ext_index)
{
	mvCls3ShadowTbl[index].size = hekSize;

	if (hekSize > MV_PP2_CLS_C3_HEK_BYTES) {
		mvCls3ShadowTbl[index].ext_ptr = ext_index;
		mvCls3ShadowExtTbl[ext_index] = IN_USE;
	} else
		mvCls3ShadowTbl[index].ext_ptr = NOT_IN_USE;
}

/*-----------------------------------------------------------------------------*/
void mvPp2ClsC3ShadowInit(void)
{
	/* clear hash shadow and extension shadow */
	int index;

	for (index = 0; index < MV_PP2_CLS_C3_HASH_TBL_SIZE; index++) {
		mvCls3ShadowTbl[index].size = 0;
		mvCls3ShadowTbl[index].ext_ptr = NOT_IN_USE;
	}

	for (index = 0; index < MV_PP2_CLS_C3_EXT_TBL_SIZE; index++)
		mvCls3ShadowExtTbl[index] = NOT_IN_USE;
}

/*-----------------------------------------------------------------------------*/
int mvPp2ClsC3ShadowFreeGet(void)
{
	int index;

	/* Go through the all entires from first to last */
	for (index = 0; index < MV_PP2_CLS_C3_HASH_TBL_SIZE; index++) {
		if (!mvCls3ShadowTbl[index].size)
			break;
	}
	return index;
}
/*-----------------------------------------------------------------------------*/
int mvPp2ClsC3ShadowExtFreeGet(void)
{
	int index;

	/* Go through the all entires from first to last */
	for (index = 0; index < MV_PP2_CLS_C3_EXT_TBL_SIZE; index++) {
		if (mvCls3ShadowExtTbl[index] == NOT_IN_USE)
			break;
	}
	return index;
}
/*-----------------------------------------------------------------------------*/
void mvPp2C3ShadowClear(int index)
{
	int ext_ptr;

	mvCls3ShadowTbl[index].size = 0;
	ext_ptr = mvCls3ShadowTbl[index].ext_ptr;

	if (ext_ptr != NOT_IN_USE)
		mvCls3ShadowExtTbl[ext_ptr] = NOT_IN_USE;

	mvCls3ShadowTbl[index].ext_ptr = NOT_IN_USE;
}
/*-------------------------------------------------------------------------------
retun 1 scan procedure completed
-------------------------------------------------------------------------------*/
static int mvPp2ClsC3ScanIsComplete(void)
{
	unsigned int regVal;

	regVal = mvPp2RdReg(MV_PP2_CLS3_STATE_REG);
	regVal &= MV_PP2_CLS3_STATE_SC_DONE_MASK;
	regVal >>= MV_PP2_CLS3_STATE_SC_DONE;

	return regVal;
}
/*-------------------------------------------------------------------------------
return 1 if that the last CPU access (Query,Add or Delete) was completed
-------------------------------------------------------------------------------*/
static int mvPp2ClsC3CpuIsDone(void)
{
	unsigned int regVal;

	regVal = mvPp2RdReg(MV_PP2_CLS3_STATE_REG);
	regVal &= MV_PP2_CLS3_STATE_CPU_DONE_MASK;
	regVal >>= MV_PP2_CLS3_STATE_CPU_DONE;
	return regVal;
}

/*-------------------------------------------------------------------------------
0x0  "ScanCompleted"  scan completed and the scan results are ready in hardware
0x1  "HitCountersClear"  The engine is clearing the Hit Counters
0x2  "ScanWait"  The engine waits for the scan delay timer
0x3  "ScanInProgress"  The scan process is in progress
-------------------------------------------------------------------------------*/
static int mvPp2ClsC3ScanStateGet(int *state)
{
	unsigned int regVal;

	PTR_VALIDATE(state);

	regVal = mvPp2RdReg(MV_PP2_CLS3_STATE_REG);
	regVal &= MV_PP2_CLS3_STATE_SC_STATE_MASK;
	regVal >>= MV_PP2_CLS3_STATE_SC_STATE;
	*state = regVal;

	return MV_OK;
}
/*-------------------------------------------------------------------------------
return 1 if counters clearing is completed
--------------------------------------------------------------------------------*/
static int mvPp2ClsC3HitCntrClearDone(void)
{
	unsigned int regVal;

	regVal = mvPp2RdReg(MV_PP2_CLS3_STATE_REG);
	regVal &= MV_PP2_CLS3_STATE_CLEAR_CTR_DONE_MASK;
	regVal >>= MV_PP2_CLS3_STATE_CLEAR_CTR_DONE;
	return regVal;
}
/*-------------------------------------------------------------------------------*/

void mvPp2ClsC3SwClear(MV_PP2_CLS_C3_ENTRY *c3)
{
	memset(c3, 0, sizeof(MV_PP2_CLS_C3_ENTRY));
}
/*-------------------------------------------------------------------------------*/

int mvPp2ClsC3Init()
{
	int rc;

	mvPp2ClsC3ShadowInit();
	rc = mvPp2ClsC3HitCntrsClearAll();
	return rc;
}

/*-------------------------------------------------------------------------------
  bank first 2 entries are reserved for miss actions
--------------------------------------------------------------------------------*/
static int mvPp2ClsC3IsReservedIndex(int index)
{
#ifdef CONFIG_MV_ETH_PP2_1
	return MV_FALSE;
#endif
	if ((index % MV_PP2_CLS_C3_BANK_SIZE) > 1)
		/* not reserved */
		return MV_FALSE;

	return MV_TRUE;
}

/*-------------------------------------------------------------------------------
Add entry to hash table
ext_index used only if hek size < 12
-------------------------------------------------------------------------------*/
int mvPp2ClsC3HwAdd(MV_PP2_CLS_C3_ENTRY *c3, int index, int ext_index)
{
	int regStartInd, hekSize, iter = 0;
	unsigned int regVal = 0;


	PTR_VALIDATE(c3);
	POS_RANGE_VALIDATE(index, MV_PP2_CLS3_HASH_OP_TBL_ADDR_MAX);

	c3->index = index;

	/* write key control */
	mvPp2WrReg(MV_PP2_CLS3_KEY_CTRL_REG, c3->key.key_ctrl);

	hekSize = ((c3->key.key_ctrl & KEY_CTRL_HEK_SIZE_MASK) >> KEY_CTRL_HEK_SIZE);

	if (hekSize > MV_PP2_CLS_C3_HEK_BYTES) {
		/* Extension */
		POS_RANGE_VALIDATE(ext_index, MV_PP2_CLS3_HASH_OP_EXT_TBL_ADDR_MAX)
		c3->ext_index = ext_index;
		regVal |= (ext_index << MV_PP2_CLS3_HASH_OP_EXT_TBL_ADDR);

		/* write 9 hek refisters */
		regStartInd = 0;
	} else
		/* write 3 hek refisters */
		regStartInd = 6;

	for (; regStartInd < MV_PP2_CLS_C3_EXT_HEK_WORDS; regStartInd++)
		mvPp2WrReg(MV_PP2_CLS3_KEY_HEK_REG(regStartInd), c3->key.hek.words[regStartInd]);


	regVal |= (index << MV_PP2_CLS3_HASH_OP_TBL_ADDR);
	regVal &= ~MV_PP2_CLS3_MISS_PTR_MASK; /*set miss bit to 0, ppv2.1 mas 3.16*/
	regVal |= (1 << MV_PP2_CLS3_HASH_OP_ADD);

	/* set hit counter init value */
#ifdef CONFIG_MV_ETH_PP2_1
	mvPp2WrReg(MV_PP2_CLS3_INIT_HIT_CNT_REG, SwInitCntSet << MV_PP2_CLS3_INIT_HIT_CNT_OFFS);
#else
	regVal |= (SwInitCntSet << MV_PP2_CLS3_HASH_OP_INIT_CTR_VAL);
#endif
	/*trigger ADD operation*/
	mvPp2WrReg(MV_PP2_CLS3_HASH_OP_REG, regVal);

	/* wait to cpu access done bit */
	while (!mvPp2ClsC3CpuIsDone())
		if (++iter >= RETRIES_EXCEEDED) {
			mvOsPrintf("%s:Error - retries exceeded.\n", __func__);
			return MV_CLS3_RETRIES_EXCEEDED;
		}

	/* write action table registers */
	mvPp2WrReg(MV_PP2_CLS3_ACT_REG, c3->sram.regs.actions);
	mvPp2WrReg(MV_PP2_CLS3_ACT_QOS_ATTR_REG, c3->sram.regs.qos_attr);
	mvPp2WrReg(MV_PP2_CLS3_ACT_HWF_ATTR_REG, c3->sram.regs.hwf_attr);
	mvPp2WrReg(MV_PP2_CLS3_ACT_DUP_ATTR_REG, c3->sram.regs.dup_attr);
#ifdef CONFIG_MV_ETH_PP2_1
	mvPp2WrReg(MV_PP2_CLS3_ACT_SEQ_L_ATTR_REG, c3->sram.regs.seq_l_attr);
	mvPp2WrReg(MV_PP2_CLS3_ACT_SEQ_H_ATTR_REG, c3->sram.regs.seq_h_attr);
#endif
	/* set entry as valid, extesion pointer in use only if size > 12*/
	mvPp2ClsC3ShadowSet(hekSize, index, ext_index);


	return MV_OK;
}

/*-------------------------------------------------------------------------------
Add entry to miss hash table
ppv2.1 mas 3.16 relevant only for ppv2.1
-------------------------------------------------------------------------------*/
int mvPp2ClsC3HwMissAdd(MV_PP2_CLS_C3_ENTRY *c3, int lkp_type)
{
	unsigned int regVal = 0;

	PTR_VALIDATE(c3);
	POS_RANGE_VALIDATE(lkp_type, MV_PP2_CLS_C3_MISS_TBL_SIZE - 1);

	c3->index = lkp_type;

	regVal |= (lkp_type << MV_PP2_CLS3_HASH_OP_TBL_ADDR);
	regVal |= (1 << MV_PP2_CLS3_HASH_OP_ADD);
	regVal |= MV_PP2_CLS3_MISS_PTR_MASK;/*set miss bit to 1, ppv2.1 mas 3.16*/

	/*index to miss table */
	mvPp2WrReg(MV_PP2_CLS3_HASH_OP_REG, regVal);

	/* write action table registers */
	mvPp2WrReg(MV_PP2_CLS3_ACT_REG, c3->sram.regs.actions);
	mvPp2WrReg(MV_PP2_CLS3_ACT_QOS_ATTR_REG, c3->sram.regs.qos_attr);
	mvPp2WrReg(MV_PP2_CLS3_ACT_HWF_ATTR_REG, c3->sram.regs.hwf_attr);
	mvPp2WrReg(MV_PP2_CLS3_ACT_DUP_ATTR_REG, c3->sram.regs.dup_attr);
	mvPp2WrReg(MV_PP2_CLS3_ACT_SEQ_L_ATTR_REG, c3->sram.regs.seq_l_attr);
	mvPp2WrReg(MV_PP2_CLS3_ACT_SEQ_H_ATTR_REG, c3->sram.regs.seq_h_attr);
	/*clear hit counter, clear on read */
	mvPp2ClsC3HitCntrsMissRead(lkp_type, &regVal);

	return MV_OK;
}
/*-------------------------------------------------------------------------------*/

int mvPp2ClsC3HwDel(int index)
{

	unsigned int regVal = 0;
	int iter = 0;

	POS_RANGE_VALIDATE(index, MV_PP2_CLS3_HASH_OP_TBL_ADDR_MAX);

	regVal |= (index << MV_PP2_CLS3_HASH_OP_TBL_ADDR);
	regVal |= (1 << MV_PP2_CLS3_HASH_OP_DEL);
	regVal &= ~MV_PP2_CLS3_MISS_PTR_MASK;/*set miss bit to 1, ppv2.1 mas 3.16*/


	/*trigger del operation*/
	mvPp2WrReg(MV_PP2_CLS3_HASH_OP_REG, regVal);

	/* wait to cpu access done bit */
	while (!mvPp2ClsC3CpuIsDone())
		if (++iter >= RETRIES_EXCEEDED) {
			mvOsPrintf("%s:Error - retries exceeded.\n", __func__);
			return MV_CLS3_RETRIES_EXCEEDED;
		}

	/* delete form shadow and extension shadow if exist */
	mvPp2C3ShadowClear(index);

	return MV_OK;
}
/*-------------------------------------------------------------------------------*/

int mvPp2ClsC3HwDelAll()
{
	int index, status;

	for (index = 0; index < MV_PP2_CLS_C3_HASH_TBL_SIZE; index++) {
		status = mvPp2ClsC3HwDel(index);
		if (status != MV_OK)
			return status;
	}
	return MV_OK;
}
/*-------------------------------------------------------------------------------*/

void mvPp2ClsC3HwInitCtrSet(int cntVal)
{
	SwInitCntSet = cntVal;
}

/*-------------------------------------------------------------------------------*/

static int mvPp2ClsC3HwQueryAddRelocate(int new_idx, int max_depth, int cur_depth)
{
	int status, index_free, idx = 0;
	unsigned char occupied_bmp;
	MV_PP2_CLS_C3_ENTRY local_c3;
	int usedIndex[MV_PP2_CLS3_HASH_BANKS_NUM] = {0};

	if (cur_depth >= max_depth)
		return MV_CLS3_RETRIES_EXCEEDED;


	mvPp2ClsC3SwClear(&local_c3);

	if (mvPp2ClsC3HwRead(&local_c3, new_idx)) {
		mvOsPrintf("%s could not get key for index [0x%x]\n", __func__, new_idx);
		return MV_CLS3_RETRIES_EXCEEDED;
	}

	if (mvPp2ClsC3HwQuery(&local_c3, &occupied_bmp, usedIndex)) {
		mvOsPrintf("%s: mvPp2ClsC3HwQuery failed, depth = %d\n", __func__, cur_depth);
		return MV_CLS3_ERR;
	}

	/* fill in indices for this key */
	for (idx = 0; idx < MV_PP2_CLS3_HASH_BANKS_NUM; idx++) {
		/* if new index is in the bank index, skip it */
		if (new_idx == usedIndex[idx] || mvPp2ClsC3IsReservedIndex(usedIndex[idx])) {
			usedIndex[idx] = 0;
			continue;
		}

		/* found a vacant index */
		if (!(occupied_bmp & (1 << idx))) {
			index_free = usedIndex[idx];
			break;
		}
	}

	/* no free index, recurse and relocate another key */
	if (idx == MV_PP2_CLS3_HASH_BANKS_NUM) {
#ifdef MV_DEBUG
		mvOsPrintf("new[0x%.3x]:%.1d ", new_idx, cur_depth);
		for (idx = 0; idx < MV_PP2_CLS3_HASH_BANKS_NUM; idx++)
			mvOsPrintf("0x%.3x ", usedIndex[idx]);
		mvOsPrintf("\n");
#endif

		/* recurse over all valid indices */
		for (idx = 0; idx < MV_PP2_CLS3_HASH_BANKS_NUM; idx++) {
			if (usedIndex[idx] == 0)
				continue;

			if (mvPp2ClsC3HwQueryAddRelocate(usedIndex[idx], max_depth, cur_depth+1) == MV_OK)
				break;
		}

		/* tried relocate, no valid entries found */
		if (idx == MV_PP2_CLS3_HASH_BANKS_NUM)
			return MV_CLS3_RETRIES_EXCEEDED;

	}

	/* if we reached here, we found a valid free index */
	index_free = usedIndex[idx];

	/* new_idx del is not necessary */

	/*We do not chage extension tabe*/
	status = mvPp2ClsC3HwAdd(&local_c3, index_free, local_c3.ext_index);

	if (status != MV_OK) {
		mvOsPrintf("%s:Error - mvPp2ClsC3HwAdd failed, depth = %d\\n", __func__, cur_depth);
		return status;
	}

	mvOsPrintf("key relocated  0x%.3x->0x%.3x\n", new_idx, index_free);

	return MV_OK;
}


/*-------------------------------------------------------------------------------*/
int mvPp2ClsC3HwQueryAdd(MV_PP2_CLS_C3_ENTRY *c3, int max_search_depth)
{
	int usedIndex[MV_PP2_CLS3_HASH_BANKS_NUM] = {0};
	unsigned char occupied_bmp;
	int idx, index_free, hekSize, status, ext_index = 0;

	status = mvPp2ClsC3HwQuery(c3, &occupied_bmp, usedIndex);

	if (status != MV_OK) {
		mvOsPrintf("%s:Error - mvPp2ClsC3HwQuery failed\n", __func__);
		return status;
	}

	/* Select avaliable entry index */
	for (idx = 0; idx < MV_PP2_CLS3_HASH_BANKS_NUM; idx++) {
		if (!(occupied_bmp & (1 << idx)))
			if (!mvPp2ClsC3IsReservedIndex(usedIndex[idx]))
				break;
	}

	/* Avaliable index did not found, try to relocate another key */

	if (idx == MV_PP2_CLS3_HASH_BANKS_NUM) {

		/* save all valid bank indices */
		for (idx = 0; idx < MV_PP2_CLS3_HASH_BANKS_NUM; idx++) {
			if (mvPp2ClsC3IsReservedIndex(usedIndex[idx]))
				usedIndex[idx] = 0;
		}

		for (idx = 0; idx < MV_PP2_CLS3_HASH_BANKS_NUM; idx++) {
			if (mvPp2ClsC3IsReservedIndex(usedIndex[idx]))
				continue;

			if (mvPp2ClsC3HwQueryAddRelocate(usedIndex[idx], max_search_depth, 0 /*curren depth*/) == MV_OK)
				break;
		}

		if (idx == MV_PP2_CLS3_HASH_BANKS_NUM) {
			/* Avaliable index did not found*/
			mvOsPrintf("%s:Error - HASH table is full.\n", __func__);
			return MV_CLS3_ERR;
		}
	}

	index_free = usedIndex[idx];

	hekSize = ((c3->key.key_ctrl & KEY_CTRL_HEK_SIZE_MASK) >> KEY_CTRL_HEK_SIZE);

	if (hekSize > MV_PP2_CLS_C3_HEK_BYTES) {
		/* Get Free Extension Index */
		ext_index = mvPp2ClsC3ShadowExtFreeGet();

		if (ext_index == MV_PP2_CLS_C3_HASH_TBL_SIZE) {
			mvOsPrintf("%s:Error - Extension table is full.\n", __func__);
			return MV_CLS3_ERR;
		}
	}

	status = mvPp2ClsC3HwAdd(c3, index_free, ext_index);

	if (status != MV_OK) {
		mvOsPrintf("%s:Error - mvPp2ClsC3HwAdd failed\n", __func__);
		return status;
	}

	if (hekSize > MV_PP2_CLS_C3_HEK_BYTES)
		mvOsPrintf("Added C3 entry @ index=0x%.3x ext=0x%.3x\n", index_free, ext_index);
	else
		mvOsPrintf("Added C3 entry @ index=0x%.3x\n", index_free);

	return MV_OK;
}

/*-------------------------------------------------------------------------------*/
/*if index or occupied_bmp is NULL dump the data 				 */
/* index[] size must be 8							 */
/*-------------------------------------------------------------------------------*/

int mvPp2ClsC3HwQuery(MV_PP2_CLS_C3_ENTRY *c3, unsigned char *occupied_bmp, int index[])
{
	int idx = 0;
	unsigned int regVal = 0;

	PTR_VALIDATE(c3);

	/* write key control */
	mvPp2WrReg(MV_PP2_CLS3_KEY_CTRL_REG, c3->key.key_ctrl);

	/* write hek */
	for (idx = 0; idx < MV_PP2_CLS_C3_EXT_HEK_WORDS; idx++)
		mvPp2WrReg(MV_PP2_CLS3_KEY_HEK_REG(idx), c3->key.hek.words[idx]);

	/*trigger query operation*/
	mvPp2WrReg(MV_PP2_CLS3_QRY_ACT_REG, (1 << MV_PP2_CLS3_QRY_ACT));

	idx = 0;
	while (!mvPp2ClsC3CpuIsDone())
		if (++idx >= RETRIES_EXCEEDED) {
			mvOsPrintf("%s:Error - retries exceeded.\n", __func__);
			return MV_CLS3_RETRIES_EXCEEDED;
		}

	regVal = mvPp2RdReg(MV_PP2_CLS3_STATE_REG) & MV_PP2_CLS3_STATE_OCCIPIED_MASK;
	regVal = regVal >> MV_PP2_CLS3_STATE_OCCIPIED;

	if ((!occupied_bmp) || (!index)) {
		/* print to screen - call from sysfs*/
		for (idx = 0; idx < MV_PP2_CLS3_HASH_BANKS_NUM; idx++)
			mvOsPrintf("0x%8.8x	%s\n",
				mvPp2RdReg(MV_PP2_CLS3_QRY_RES_HASH_REG(idx)),
				(regVal & (1 << idx)) ? "OCCUPIED" : "FREE");
		return MV_OK;
	}

	*occupied_bmp = regVal;
	for (idx = 0; idx < MV_PP2_CLS3_HASH_BANKS_NUM; idx++)
		index[idx] = mvPp2RdReg(MV_PP2_CLS3_QRY_RES_HASH_REG(idx));

	return MV_OK;
}

/*-------------------------------------------------------------------------------*/
int mvPp2ClsC3HwRead(MV_PP2_CLS_C3_ENTRY *c3, int index)
{
	int i, isExt;
	MV_U32 regVal = 0;

	unsigned int hashData[MV_PP2_CLS3_HASH_DATA_REG_NUM];
	unsigned int hashExtData[MV_PP2_CLS3_HASH_EXT_DATA_REG_NUM];

	PTR_VALIDATE(c3);
	POS_RANGE_VALIDATE(index, MV_PP2_CLS3_HASH_OP_TBL_ADDR_MAX);

	mvPp2ClsC3SwClear(c3);

	c3->index = index;
	c3->ext_index = NOT_IN_USE;

	/* write index */
	mvPp2WrReg(MV_PP2_CLS3_DB_INDEX_REG, index);

	regVal |= (index << MV_PP2_CLS3_HASH_OP_TBL_ADDR);
	mvPp2WrReg(MV_PP2_CLS3_HASH_OP_REG, regVal);

	/* read action table */
	c3->sram.regs.actions = mvPp2RdReg(MV_PP2_CLS3_ACT_REG);
	c3->sram.regs.qos_attr = mvPp2RdReg(MV_PP2_CLS3_ACT_QOS_ATTR_REG);
	c3->sram.regs.hwf_attr = mvPp2RdReg(MV_PP2_CLS3_ACT_HWF_ATTR_REG);
	c3->sram.regs.dup_attr = mvPp2RdReg(MV_PP2_CLS3_ACT_DUP_ATTR_REG);

#ifdef CONFIG_MV_ETH_PP2_1
	c3->sram.regs.seq_l_attr = mvPp2RdReg(MV_PP2_CLS3_ACT_SEQ_L_ATTR_REG);
	c3->sram.regs.seq_h_attr = mvPp2RdReg(MV_PP2_CLS3_ACT_SEQ_H_ATTR_REG);
#endif

	/* read hash data*/
	for (i = 0; i < MV_PP2_CLS3_HASH_DATA_REG_NUM; i++)
		hashData[i] = mvPp2RdReg(MV_PP2_CLS3_HASH_DATA_REG(i));

	if (mvCls3ShadowTbl[index].size == 0) {
		/* entry not in use */
		return MV_OK;
	}

	c3->key.key_ctrl = 0;

	if (mvCls3ShadowTbl[index].ext_ptr == NOT_IN_USE) {
		isExt = 0;
		/* TODO REMOVE NEXT LINES- ONLY FOR INTERNAL VALIDATION */
		if ((mvCls3ShadowTbl[index].size == 0) ||
			 (mvCls3ShadowTbl[index].ext_ptr != NOT_IN_USE)) {
				mvOsPrintf("%s: SW internal error.\n", __func__);
				return MV_CLS3_SW_INTERNAL;
		}

		/*read Multihash entry data*/
		c3->key.hek.words[6] = hashData[0]; /* hek 0*/
		c3->key.hek.words[7] = hashData[1]; /* hek 1*/
		c3->key.hek.words[8] = hashData[2]; /* hek 2*/

		/* write key control data to SW */
		c3->key.key_ctrl |= (((hashData[3] & KEY_PRT_ID_MASK(isExt)) >>
					(KEY_PRT_ID(isExt) % DWORD_BITS_LEN)) << KEY_CTRL_PRT_ID);

		c3->key.key_ctrl |= (((hashData[3] & KEY_PRT_ID_TYPE_MASK(isExt)) >>
					(KEY_PRT_ID_TYPE(isExt) % DWORD_BITS_LEN)) << KEY_CTRL_PRT_ID_TYPE);

		c3->key.key_ctrl |= (((hashData[3] & KEY_LKP_TYPE_MASK(isExt)) >>
					(KEY_LKP_TYPE(isExt) % DWORD_BITS_LEN)) << KEY_CTRL_LKP_TYPE);

		c3->key.key_ctrl |= (((hashData[3] & KEY_L4_INFO_MASK(isExt)) >>
					(KEY_L4_INFO(isExt) % DWORD_BITS_LEN)) << KEY_CTRL_L4);

	} else {
		isExt = 1;
		/* TODO REMOVE NEXT LINES- ONLY FOR INTERNAL VALIDATION */
		if ((mvCls3ShadowTbl[index].size == 0) ||
			 (mvCls3ShadowTbl[index].ext_ptr == NOT_IN_USE)) {
				mvOsPrintf("%s: SW internal error.\n", __func__);
				return MV_CLS3_SW_INTERNAL;
		}
		c3->ext_index = mvCls3ShadowTbl[index].ext_ptr;

		/* write extension index */
		mvPp2WrReg(MV_PP2_CLS3_DB_INDEX_REG, mvCls3ShadowTbl[index].ext_ptr);

		/* read hash extesion data*/
		for (i = 0; i < MV_PP2_CLS3_HASH_EXT_DATA_REG_NUM; i++)
			hashExtData[i] = mvPp2RdReg(MV_PP2_CLS3_HASH_EXT_DATA_REG(i));


		/* heks bytes 35 - 32 */
		c3->key.hek.words[8] = ((hashData[2] & 0x00FFFFFF) << 8) | ((hashData[1] & 0xFF000000) >> 24);

		/* heks bytes 31 - 28 */
		c3->key.hek.words[7] = ((hashData[1] & 0x00FFFFFF) << 8) | ((hashData[0] & 0xFF000000) >> 24);

		/* heks bytes 27 - 24 */
		c3->key.hek.words[6] = ((hashData[0] & 0x00FFFFFF) << 8) | (hashExtData[6] & 0x000000FF);

		c3->key.hek.words[5] = hashExtData[5]; /* heks bytes 23 - 20 */
		c3->key.hek.words[4] = hashExtData[4]; /* heks bytes 19 - 16 */
		c3->key.hek.words[3] = hashExtData[3]; /* heks bytes 15 - 12 */
		c3->key.hek.words[2] = hashExtData[2]; /* heks bytes 11 - 8  */
		c3->key.hek.words[1] = hashExtData[1]; /* heks bytes 7 - 4   */
		c3->key.hek.words[0] = hashExtData[0]; /* heks bytes 3 - 0   */

		/* write key control data to SW*/

		c3->key.key_ctrl |= (((hashData[3] & KEY_PRT_ID_MASK(isExt)) >>
					(KEY_PRT_ID(isExt) % DWORD_BITS_LEN)) << KEY_CTRL_PRT_ID);

#ifdef CONFIG_MV_ETH_PP2_1
		/* PPv2.1 (feature MAS 3.16) LKP_TYPE size and offset changed */

		c3->key.key_ctrl |= (((hashData[3] & KEY_PRT_ID_TYPE_MASK(isExt)) >>
					(KEY_PRT_ID_TYPE(isExt) % DWORD_BITS_LEN)) << KEY_CTRL_PRT_ID_TYPE);

		c3->key.key_ctrl |= ((((hashData[2] & 0xf8000000) >> 27) |
					((hashData[3] & 0x1) << 5)) << KEY_CTRL_LKP_TYPE);

#else
		c3->key.key_ctrl |= ((((hashData[2] & 0x80000000) >> 31) |
					((hashData[3] & 0x1) << 1)) << KEY_CTRL_PRT_ID_TYPE);

		c3->key.key_ctrl |= (((hashData[2] & KEY_LKP_TYPE_MASK(isExt)) >>
					(KEY_LKP_TYPE(isExt) % DWORD_BITS_LEN)) << KEY_CTRL_LKP_TYPE);

#endif /* CONFIG_MV_ETH_PP2_1 */

		c3->key.key_ctrl |= (((hashData[2] & KEY_L4_INFO_MASK(isExt)) >>
					(KEY_L4_INFO(isExt) % DWORD_BITS_LEN)) << KEY_CTRL_L4);
	}

	/* update hek size */
	c3->key.key_ctrl |= ((mvCls3ShadowTbl[index].size << KEY_CTRL_HEK_SIZE) & KEY_CTRL_HEK_SIZE_MASK);

	return MV_OK;
}

/*-------------------------------------------------------------------------------*/
/* ppv2.1 MAS 3.12								*/
/*-------------------------------------------------------------------------------*/

int mvPp2ClsC3HwMissRead(MV_PP2_CLS_C3_ENTRY *c3, int lkp_type)
{
	unsigned int regVal = 0;

	PTR_VALIDATE(c3);
	POS_RANGE_VALIDATE(lkp_type, MV_PP2_CLS_C3_MISS_TBL_SIZE - 1);

	mvPp2ClsC3SwClear(c3);

	c3->index = lkp_type;
	c3->ext_index = NOT_IN_USE;

	regVal = (lkp_type << MV_PP2_CLS3_HASH_OP_TBL_ADDR) | MV_PP2_CLS3_MISS_PTR_MASK;
	mvPp2WrReg(MV_PP2_CLS3_HASH_OP_REG, regVal);

	/* read action table */
	c3->sram.regs.actions = mvPp2RdReg(MV_PP2_CLS3_ACT_REG);
	c3->sram.regs.qos_attr = mvPp2RdReg(MV_PP2_CLS3_ACT_QOS_ATTR_REG);
	c3->sram.regs.hwf_attr = mvPp2RdReg(MV_PP2_CLS3_ACT_HWF_ATTR_REG);
	c3->sram.regs.dup_attr = mvPp2RdReg(MV_PP2_CLS3_ACT_DUP_ATTR_REG);
	c3->sram.regs.seq_l_attr = mvPp2RdReg(MV_PP2_CLS3_ACT_SEQ_L_ATTR_REG);
	c3->sram.regs.seq_h_attr = mvPp2RdReg(MV_PP2_CLS3_ACT_SEQ_H_ATTR_REG);

	return MV_OK;
}

/*-------------------------------------------------------------------------------*/
int mvPp2ClsC3SwDump(MV_PP2_CLS_C3_ENTRY *c3)
{
	int hekSize;

	PTR_VALIDATE(c3);

	mvOsPrintf("\n");
	mvOsPrintf("INDEX[0x%3.3x] ", c3->index);

	hekSize = ((c3->key.key_ctrl & KEY_CTRL_HEK_SIZE_MASK) >> KEY_CTRL_HEK_SIZE);

	/* print extension index if exist*/
	if (hekSize > MV_PP2_CLS_C3_HEK_BYTES)
		/* extension */
		mvOsPrintf("EXT_INDEX[0x%2.2x] ", c3->ext_index);
	else
		/* without extension */
		mvOsPrintf("EXT_INDEX[ NA ] ");

	mvOsPrintf("SIZE[0x%2.2x] ", hekSize);
	mvOsPrintf("PRT[ID = 0x%2.2x,TYPE = 0x%1.1x] ",
			((c3->key.key_ctrl & KEY_CTRL_PRT_ID_MASK) >> KEY_CTRL_PRT_ID),
			((c3->key.key_ctrl & KEY_CTRL_PRT_ID_TYPE_MASK) >> KEY_CTRL_PRT_ID_TYPE));

	mvOsPrintf("LKP_TYPE[0x%1.1x] ",
			((c3->key.key_ctrl & KEY_CTRL_LKP_TYPE_MASK) >> KEY_CTRL_LKP_TYPE));

	mvOsPrintf("L4INFO[0x%1.1x] ",
			((c3->key.key_ctrl & KEY_CTRL_L4_MASK) >> KEY_CTRL_L4));

	mvOsPrintf("\n\n");
	mvOsPrintf("HEK	");
	if (hekSize > MV_PP2_CLS_C3_HEK_BYTES)
		/* extension */
		mvOsPrintf(HEK_EXT_FMT, HEK_EXT_VAL(c3->key.hek.words));
	else
		/* without extension */
		mvOsPrintf(HEK_FMT, HEK_VAL(c3->key.hek.words));
	mvOsPrintf("\n");
	return mvPp2ClsC3SwActDump(c3);
}

/*-------------------------------------------------------------------------------*/

static int mvPp2ClsC3SwActDump(MV_PP2_CLS_C3_ENTRY *c3)
{
	PTR_VALIDATE(c3);
	mvOsPrintf("\n");

#ifdef CONFIG_MV_ETH_PP2_1
	/*------------------------------*/
	/*	actions 0x1D40		*/
	/*------------------------------*/

	mvOsPrintf("ACT_TBL: COLOR   LOW_Q   HIGH_Q     FWD   POLICER  FID\n");
	mvOsPrintf("CMD:     [%1d]      [%1d]    [%1d]        [%1d]   [%1d]      [%1d]\n",
			((c3->sram.regs.actions & (ACT_COLOR_MASK)) >> ACT_COLOR),
			((c3->sram.regs.actions & (ACT_LOW_Q_MASK)) >> ACT_LOW_Q),
			((c3->sram.regs.actions & (ACT_HIGH_Q_MASK)) >> ACT_HIGH_Q),
			((c3->sram.regs.actions & ACT_FWD_MASK) >> ACT_FWD),
			((c3->sram.regs.actions & (ACT_POLICER_SELECT_MASK)) >> ACT_POLICER_SELECT),
			((c3->sram.regs.actions & ACT_FLOW_ID_EN_MASK) >> ACT_FLOW_ID_EN));

	mvOsPrintf("VAL:              [%1d]    [0x%x]\n",
			((c3->sram.regs.qos_attr & (ACT_QOS_ATTR_MDF_LOW_Q_MASK)) >> ACT_QOS_ATTR_MDF_LOW_Q),
			((c3->sram.regs.qos_attr & (ACT_QOS_ATTR_MDF_HIGH_Q_MASK)) >> ACT_QOS_ATTR_MDF_HIGH_Q));

	mvOsPrintf("\n");
	/*------------------------------*/
	/*	hwf_attr 0x1D48		*/
	/*------------------------------*/

	mvOsPrintf("HWF_ATTR: IPTR	DPTR	 CHKSM     MTU_IDX\n");
	mvOsPrintf("          0x%1.1x   0x%4.4x   %s   0x%1.1x\n",

			((c3->sram.regs.hwf_attr & ACT_HWF_ATTR_IPTR_MASK) >> ACT_HWF_ATTR_IPTR),
			((c3->sram.regs.hwf_attr & ACT_HWF_ATTR_DPTR_MASK) >> ACT_HWF_ATTR_DPTR),
			(((c3->sram.regs.hwf_attr &
				ACT_HWF_ATTR_CHKSM_EN_MASK) >> ACT_HWF_ATTR_CHKSM_EN) ? "ENABLE" : "DISABLE"),
			((c3->sram.regs.hwf_attr & ACT_HWF_ATTR_MTU_INX_MASK) >> ACT_HWF_ATTR_MTU_INX));
	mvOsPrintf("\n");
	/*------------------------------*/
	/*	dup_attr 0x1D4C		*/
	/*------------------------------*/
	mvOsPrintf("DUP_ATTR:FID	COUNT	POLICER [id    bank]\n");
	mvOsPrintf("         0x%2.2x\t0x%1.1x\t\t[0x%2.2x   0x%1.1x]\n",
		((c3->sram.regs.dup_attr & ACT_DUP_FID_MASK) >> ACT_DUP_FID),
		((c3->sram.regs.dup_attr & ACT_DUP_COUNT_MASK) >> ACT_DUP_COUNT),
		((c3->sram.regs.dup_attr & ACT_DUP_POLICER_MASK) >> ACT_DUP_POLICER_ID),
		((c3->sram.regs.dup_attr & ACT_DUP_POLICER_BANK_MASK) >> ACT_DUP_POLICER_BANK_BIT));
	mvOsPrintf("\n");
	mvOsPrintf("SEQ_ATTR: HIGH[32:37] LOW[0:31]\n");
	mvOsPrintf("          0x%2.2x        0x%8.8x", c3->sram.regs.seq_h_attr, c3->sram.regs.seq_l_attr);

#else
	/*------------------------------*/
	/*	actions 0x1D40		*/
	/*------------------------------*/

	mvOsPrintf("ACT_TBL: COLOR   LOW_Q   HIGH_Q     FWD   POLICER  FID\n");
	mvOsPrintf("CMD:     [%1d]      [%1d]    [%1d]        [%1d]   [%1d]      [%1d]\n",
			((c3->sram.regs.actions & (ACT_COLOR_MASK)) >> ACT_COLOR),
			((c3->sram.regs.actions & (ACT_LOW_Q_MASK)) >> ACT_LOW_Q),
			((c3->sram.regs.actions & (ACT_HIGH_Q_MASK)) >> ACT_HIGH_Q),
			((c3->sram.regs.actions & ACT_FWD_MASK) >> ACT_FWD),
			((c3->sram.regs.actions & (ACT_POLICER_SELECT_MASK)) >> ACT_POLICER_SELECT),
			((c3->sram.regs.actions & ACT_FLOW_ID_EN_MASK) >> ACT_FLOW_ID_EN));

	mvOsPrintf("VAL:              [%1d]    [0x%x]            [0x%x]\n",
			((c3->sram.regs.qos_attr & (ACT_QOS_ATTR_MDF_LOW_Q_MASK)) >> ACT_QOS_ATTR_MDF_LOW_Q),
			((c3->sram.regs.qos_attr & (ACT_QOS_ATTR_MDF_HIGH_Q_MASK)) >> ACT_QOS_ATTR_MDF_HIGH_Q),
			((c3->sram.regs.dup_attr & (ACT_DUP_POLICER_MASK)) >> ACT_DUP_POLICER_ID));
	mvOsPrintf("\n");

	/*------------------------------*/
	/*	hwf_attr 0x1D48		*/
	/*------------------------------*/

	mvOsPrintf("HWF_ATTR: IPTR    DPTR   CHKSM\n");
	mvOsPrintf("          0x%1.1x     0x%4.4x %s\t",
			((c3->sram.regs.hwf_attr & ACT_HWF_ATTR_IPTR_MASK) >> ACT_HWF_ATTR_IPTR),
			((c3->sram.regs.hwf_attr & ACT_HWF_ATTR_DPTR_MASK) >> ACT_HWF_ATTR_DPTR),
			(((c3->sram.regs.hwf_attr & ACT_HWF_ATTR_CHKSM_EN_MASK) >> ACT_HWF_ATTR_CHKSM_EN) ? "ENABLE" : "DISABLE"));

	mvOsPrintf("\n");

	/*------------------------------*/
	/*	dup_attr 0x1D4C		*/
	/*------------------------------*/

	mvOsPrintf("DUP_ATTR: FID   COUNT\n");
	mvOsPrintf("          0x%2.2x  0x%1.1x\n",
			((c3->sram.regs.dup_attr & ACT_DUP_FID_MASK) >> ACT_DUP_FID),
			((c3->sram.regs.dup_attr & ACT_DUP_COUNT_MASK) >> ACT_DUP_COUNT));


#endif /* CONFIG_MV_ETH_PP2_1 */

	mvOsPrintf("\n\n");

	return MV_OK;
}

/*-------------------------------------------------------------------------------*/
int mvPp2ClsC3HwDump()
{
	int index;
	MV_PP2_CLS_C3_ENTRY c3;

	mvPp2ClsC3SwClear(&c3);

	for (index = 0; index < MV_PP2_CLS_C3_HASH_TBL_SIZE; index++) {
		if (mvCls3ShadowTbl[index].size > 0) {
			mvPp2ClsC3HwRead(&c3, index);
			mvPp2ClsC3SwDump(&c3);
			mvOsPrintf("----------------------------------------------------------------------\n");
		}
	}

	return MV_OK;
}
/*-------------------------------------------------------------------------------*/
/*
All miss entries are valid,
the key+heks in miss entries are hot in use and this is the
reason that we dump onlt action table fields
*/
int mvPp2ClsC3HwMissDump()
{
	int index;
	MV_PP2_CLS_C3_ENTRY c3;

	mvPp2ClsC3SwClear(&c3);

	for (index = 0; index < MV_PP2_CLS_C3_MISS_TBL_SIZE; index++) {
		mvPp2ClsC3HwMissRead(&c3, index);
		mvOsPrintf("INDEX[0x%3.3X]\n", index);
		mvPp2ClsC3SwActDump(&c3);
		mvOsPrintf("----------------------------------------------------------------------\n");
	}

	return MV_OK;
}


/*-------------------------------------------------------------------------------*/
int mvPp2ClsC3HwExtDump()
{
	int index, i;
	unsigned int hashExtData[MV_PP2_CLS3_HASH_EXT_DATA_REG_NUM];

	mvOsPrintf("INDEX    DATA\n");

	for (index = 0; index <  MV_PP2_CLS_C3_EXT_TBL_SIZE; index++)
		if (mvCls3ShadowExtTbl[index] == IN_USE) {
			/* write extension index */
			mvPp2WrReg(MV_PP2_CLS3_DB_INDEX_REG, index);

			/* read hash extesion data*/
			for (i = 0; i < MV_PP2_CLS3_HASH_EXT_DATA_REG_NUM; i++)
				hashExtData[i] = mvPp2RdReg(MV_PP2_CLS3_HASH_EXT_DATA_REG(i));

			mvOsPrintf("[0x%2.2x] %8.8x %8.8x %8.8x %8.8x %8.8x %8.8x %8.8x\n",
					index, hashExtData[6], hashExtData[5], hashExtData[4],
					hashExtData[3], hashExtData[2], hashExtData[1], hashExtData[0]);
		} /* if */

	return MV_OK;
}

/*-------------------------------------------------------------------------------*/
/*		APIs for Classification C3 key fields			   	 */
/*-------------------------------------------------------------------------------*/

int mvPp2ClsC3SwL4infoSet(MV_PP2_CLS_C3_ENTRY *c3, int l4info)
{
	PTR_VALIDATE(c3);
	POS_RANGE_VALIDATE(l4info, KEY_CTRL_L4_MAX);

	c3->key.key_ctrl &= ~KEY_CTRL_L4_MASK;
	c3->key.key_ctrl |= (l4info << KEY_CTRL_L4);
	return MV_OK;
}

/*-------------------------------------------------------------------------------*/
int mvPp2ClsC3SwLkpTypeSet(MV_PP2_CLS_C3_ENTRY *c3, int lkp_type)
{
	PTR_VALIDATE(c3);
	POS_RANGE_VALIDATE(lkp_type, KEY_CTRL_LKP_TYPE_MAX);

	c3->key.key_ctrl &= ~KEY_CTRL_LKP_TYPE_MASK;
	c3->key.key_ctrl |= (lkp_type << KEY_CTRL_LKP_TYPE);
	return MV_OK;
}
/*-------------------------------------------------------------------------------*/
int mvPp2ClsC3SwPortIDSet(MV_PP2_CLS_C3_ENTRY *c3, int type, int portid)
{
	PTR_VALIDATE(c3);
	POS_RANGE_VALIDATE(portid, KEY_CTRL_PRT_ID_MAX);
	POS_RANGE_VALIDATE(type, KEY_CTRL_PRT_ID_TYPE_MAX);

	c3->key.key_ctrl &= ~(KEY_CTRL_PRT_ID_MASK | KEY_CTRL_PRT_ID_TYPE_MASK);
	c3->key.key_ctrl |= ((portid << KEY_CTRL_PRT_ID) | (type << KEY_CTRL_PRT_ID_TYPE));

	return MV_OK;
}
/*-------------------------------------------------------------------------------*/
int mvPp2ClsC3SwHekSizeSet(MV_PP2_CLS_C3_ENTRY *c3, int hekSize)
{
	PTR_VALIDATE(c3);
	POS_RANGE_VALIDATE(hekSize, KEY_CTRL_HEK_SIZE_MAX);

	c3->key.key_ctrl &= ~KEY_CTRL_HEK_SIZE_MASK;
	c3->key.key_ctrl |= (hekSize << KEY_CTRL_HEK_SIZE);
	return MV_OK;
}

/*-------------------------------------------------------------------------------*/
int mvPp2ClsC3SwHekByteSet(MV_PP2_CLS_C3_ENTRY *c3, unsigned int offs, unsigned char byte)
{
	PTR_VALIDATE(c3);
	POS_RANGE_VALIDATE(offs, ((MV_PP2_CLS_C3_EXT_HEK_WORDS*4) - 1));

	c3->key.hek.bytes[HW_BYTE_OFFS(offs)] = byte;

	return MV_OK;
}
/*-------------------------------------------------------------------------------*/
int mvPp2ClsC3SwHekWordSet(MV_PP2_CLS_C3_ENTRY *c3, unsigned int offs, unsigned int word)
{
	PTR_VALIDATE(c3);
	POS_RANGE_VALIDATE(offs, ((MV_PP2_CLS_C3_EXT_HEK_WORDS) - 1));

	c3->key.hek.words[offs] = word;

	return MV_OK;
}


/*-------------------------------------------------------------------------------*/
/*		APIs for Classification C3 action table fields		   	 */
/*-------------------------------------------------------------------------------*/

int mvPp2ClsC3ColorSet(MV_PP2_CLS_C3_ENTRY *c3, int cmd)
{
	PTR_VALIDATE(c3);
	POS_RANGE_VALIDATE(cmd, COLOR_RED_AND_LOCK);

	c3->sram.regs.actions &= ~ACT_COLOR_MASK;
	c3->sram.regs.actions |= (cmd << ACT_COLOR);

	return MV_OK;
}
/*-------------------------------------------------------------------------------*/
int mvPp2ClsC3QueueHighSet(MV_PP2_CLS_C3_ENTRY *c3, int cmd, int queue)
{
	PTR_VALIDATE(c3);


	POS_RANGE_VALIDATE(cmd, UPDATE_AND_LOCK);
	POS_RANGE_VALIDATE(queue, ACT_QOS_ATTR_MDF_HIGH_Q_MAX);

	/*set command*/
	c3->sram.regs.actions &= ~ACT_HIGH_Q_MASK;
	c3->sram.regs.actions |= (cmd << ACT_HIGH_Q);

	/*set modify High queue value*/
	c3->sram.regs.qos_attr &= ~ACT_QOS_ATTR_MDF_HIGH_Q_MASK;
	c3->sram.regs.qos_attr |= (queue << ACT_QOS_ATTR_MDF_HIGH_Q);

	return MV_OK;
}

/*-------------------------------------------------------------------------------*/
int mvPp2ClsC3QueueLowSet(MV_PP2_CLS_C3_ENTRY *c3, int cmd, int queue)
{
	PTR_VALIDATE(c3);

	POS_RANGE_VALIDATE(cmd, UPDATE_AND_LOCK);
	POS_RANGE_VALIDATE(queue, ACT_QOS_ATTR_MDF_LOW_Q_MAX);

	/*set command*/
	c3->sram.regs.actions &= ~ACT_LOW_Q_MASK;
	c3->sram.regs.actions |= (cmd << ACT_LOW_Q);

	/*set modify High queue value*/
	c3->sram.regs.qos_attr &= ~ACT_QOS_ATTR_MDF_LOW_Q_MASK;
	c3->sram.regs.qos_attr |= (queue << ACT_QOS_ATTR_MDF_LOW_Q);

	return MV_OK;
}
/*-------------------------------------------------------------------------------*/

int mvPp2ClsC3QueueSet(MV_PP2_CLS_C3_ENTRY *c3, int cmd, int queue)
{
	int status = MV_OK;
	int qHigh, qLow;

	PTR_VALIDATE(c3);
	POS_RANGE_VALIDATE(queue, ACT_QOS_ATTR_Q_MAX);

	/* cmd validation in set functions */

	qHigh = (queue & ACT_QOS_ATTR_MDF_HIGH_Q_MASK) >> ACT_QOS_ATTR_MDF_HIGH_Q;
	qLow = (queue & ACT_QOS_ATTR_MDF_LOW_Q_MASK) >> ACT_QOS_ATTR_MDF_LOW_Q;

	status |= mvPp2ClsC3QueueLowSet(c3, cmd, qLow);
	status |= mvPp2ClsC3QueueHighSet(c3, cmd, qHigh);

	return status;
}

/*-------------------------------------------------------------------------------*/

int mvPp2ClsC3ForwardSet(MV_PP2_CLS_C3_ENTRY *c3, int cmd)
{
	PTR_VALIDATE(c3);
	POS_RANGE_VALIDATE(cmd, HWF_AND_LOW_LATENCY_AND_LOCK);

	c3->sram.regs.actions &= ~ACT_FWD_MASK;
	c3->sram.regs.actions |= (cmd << ACT_FWD);
	return MV_OK;
}
/*-------------------------------------------------------------------------------*/
#ifdef CONFIG_MV_ETH_PP2_1
int mvPp2ClsC3PolicerSet(MV_PP2_CLS_C3_ENTRY *c3, int cmd, int policerId, int bank)
{
	PTR_VALIDATE(c3);
	POS_RANGE_VALIDATE(cmd, UPDATE_AND_LOCK);
	POS_RANGE_VALIDATE(policerId, ACT_DUP_POLICER_MAX);
	BIT_RANGE_VALIDATE(bank);

	c3->sram.regs.actions &= ~ACT_POLICER_SELECT_MASK;
	c3->sram.regs.actions |= (cmd << ACT_POLICER_SELECT);

	c3->sram.regs.dup_attr &= ~ACT_DUP_POLICER_MASK;
	c3->sram.regs.dup_attr |= (policerId << ACT_DUP_POLICER_ID);

	if (bank)
		c3->sram.regs.dup_attr |= ACT_DUP_POLICER_BANK_MASK;
	else
		c3->sram.regs.dup_attr &= ~ACT_DUP_POLICER_BANK_MASK;

	return MV_OK;
}
#else
int mvPp2ClsC3PolicerSet(MV_PP2_CLS_C3_ENTRY *c3, int cmd, int policerId)
{
	PTR_VALIDATE(c3);
	POS_RANGE_VALIDATE(cmd, UPDATE_AND_LOCK);
	POS_RANGE_VALIDATE(policerId, ACT_DUP_POLICER_MAX);

	c3->sram.regs.actions &= ~ACT_POLICER_SELECT_MASK;
	c3->sram.regs.actions |= (cmd << ACT_POLICER_SELECT);

	c3->sram.regs.dup_attr &= ~ACT_DUP_POLICER_MASK;
	c3->sram.regs.dup_attr |= (policerId << ACT_DUP_POLICER_ID);
	return MV_OK;
}
#endif /*CONFIG_MV_ETH_PP2_1*/
 /*-------------------------------------------------------------------------------*/
int mvPp2ClsC3FlowIdEn(MV_PP2_CLS_C3_ENTRY *c3, int flowid_en)
{
	PTR_VALIDATE(c3);

	/*set Flow ID enable or disable*/
	if (flowid_en)
		c3->sram.regs.actions |= (1 << ACT_FLOW_ID_EN);
	else
		c3->sram.regs.actions &= ~(1 << ACT_FLOW_ID_EN);

	return MV_OK;
}
/*-------------------------------------------------------------------------------*/
/*
  PPv2.1 (feature MAS 3.7) function changed , get also MTU index as parameter
 */
int mvPp2ClsC3ModSet(MV_PP2_CLS_C3_ENTRY *c3, int data_ptr, int instr_offs, int l4_csum)
{
	PTR_VALIDATE(c3);
	POS_RANGE_VALIDATE(data_ptr, ACT_HWF_ATTR_DPTR_MAX);
	POS_RANGE_VALIDATE(instr_offs, ACT_HWF_ATTR_IPTR_MAX);
	POS_RANGE_VALIDATE(l4_csum, 1);

	c3->sram.regs.hwf_attr &= ~ACT_HWF_ATTR_DPTR_MASK;
	c3->sram.regs.hwf_attr &= ~ACT_HWF_ATTR_IPTR_MASK;
	c3->sram.regs.hwf_attr &= ~ACT_HWF_ATTR_CHKSM_EN_MASK;

	c3->sram.regs.hwf_attr |= (data_ptr << ACT_HWF_ATTR_DPTR);
	c3->sram.regs.hwf_attr |= (instr_offs << ACT_HWF_ATTR_IPTR);
	c3->sram.regs.hwf_attr |= (l4_csum << ACT_HWF_ATTR_CHKSM_EN);

	return MV_OK;
}


/*-------------------------------------------------------------------------------*/
/*
  PPv2.1 (feature MAS 3.7) mtu - new field at action table
*/

int mvPp2ClsC3MtuSet(MV_PP2_CLS_C3_ENTRY *c3, int mtu_inx)
{
	PTR_VALIDATE(c3);
	POS_RANGE_VALIDATE(mtu_inx, ACT_HWF_ATTR_MTU_INX_MAX);

	c3->sram.regs.hwf_attr &= ~ACT_HWF_ATTR_MTU_INX_MASK;
	c3->sram.regs.hwf_attr |= (mtu_inx << ACT_HWF_ATTR_MTU_INX);
	return MV_OK;
}
/*-------------------------------------------------------------------------------*/

int mvPp2ClsC3DupSet(MV_PP2_CLS_C3_ENTRY *c3, int dupid, int count)
{
	PTR_VALIDATE(c3);
	POS_RANGE_VALIDATE(count, ACT_DUP_COUNT_MAX);
	POS_RANGE_VALIDATE(dupid, ACT_DUP_FID_MAX);

	/*set flowid and count*/
	c3->sram.regs.dup_attr &= ~(ACT_DUP_FID_MASK | ACT_DUP_COUNT_MASK);
	c3->sram.regs.dup_attr |= (dupid << ACT_DUP_FID);
	c3->sram.regs.dup_attr |= (count << ACT_DUP_COUNT);

	return MV_OK;
}

/*-------------------------------------------------------------------------------*/
/* PPv2.1 (feature MAS 3.14) cls sequence */
int mvPp2ClsC3SeqSet(MV_PP2_CLS_C3_ENTRY *c3, int id,  int bits_offs,  int bits)
{
	unsigned int low_bits, high_bits = 0;

	PTR_VALIDATE(c3);
	POS_RANGE_VALIDATE(bits, MV_PP2_CLS_SEQ_SIZE_MAX);
	POS_RANGE_VALIDATE(id, (1 << bits) - 1);
	POS_RANGE_VALIDATE(bits_offs + bits, MV_PP2_CLS3_ACT_SEQ_SIZE);

	if (bits_offs >= DWORD_BITS_LEN)
		high_bits = bits;

	else if (bits_offs + bits > DWORD_BITS_LEN)
		high_bits = (bits_offs + bits) % DWORD_BITS_LEN;

	low_bits = bits - high_bits;

	/*
	high_bits hold the num of bits that we need to write in seq_h_attr
	low_bits hold the num of bits that we need to write in seq_l_attr
	*/

	if (low_bits) {
		/* mask and set new value in seq_l_attr*/
		c3->sram.regs.seq_l_attr &= ~(((1 << low_bits) - 1)  << bits_offs);
		c3->sram.regs.seq_l_attr |= (id  << bits_offs);
	}

	if (high_bits) {
		int high_id = id >> low_bits;
		int high_offs = (low_bits == 0) ? (bits_offs % DWORD_BITS_LEN) : 0;

		/* mask and set new value in seq_h_attr*/
		c3->sram.regs.seq_h_attr &= ~(((1 << high_bits) - 1)  << high_offs);
		c3->sram.regs.seq_h_attr |= (high_id << high_offs);
	}

	return MV_OK;

}

/*-------------------------------------------------------------------------------*/
/*		APIs for Classification C3 Hit counters management	   	 */
/*-------------------------------------------------------------------------------*/

int mvPp2ClsC3HitCntrsClear(int lkpType)
{
	/* clear all counters that entry lookup type corresponding to lkpType */
	int iter = 0;

	POS_RANGE_VALIDATE(lkpType, KEY_CTRL_LKP_TYPE_MAX);

	mvPp2WrReg(MV_PP2_CLS3_CLEAR_COUNTERS_REG, lkpType);

	/* wait to clear het counters done bit */
	while (!mvPp2ClsC3HitCntrClearDone())
		if (++iter >= RETRIES_EXCEEDED) {
			mvOsPrintf("%s:Error - retries exceeded.\n", __func__);
			return MV_CLS3_RETRIES_EXCEEDED;
		}

	return MV_OK;
}
/*-------------------------------------------------------------------------------*/

int mvPp2ClsC3HitCntrsClearAll(void)
{
	int iter = 0;
/*
  PPv2.1 (feature MAS 3.16)  CLEAR_COUNTERS size changed, clear all code changed from 0x1f to 0x3f
*/

#ifdef CONFIG_MV_ETH_PP2_1
	mvPp2WrReg(MV_PP2_CLS3_CLEAR_COUNTERS_REG, MV_PP2_V1_CLS3_CLEAR_ALL);
#else
	mvPp2WrReg(MV_PP2_CLS3_CLEAR_COUNTERS_REG, MV_PP2_V0_CLS3_CLEAR_ALL);
#endif
	/* wait to clear het counters done bit */
	while (!mvPp2ClsC3HitCntrClearDone())
		if (++iter >= RETRIES_EXCEEDED) {
			mvOsPrintf("%s:Error - retries exceeded.\n", __func__);
			return MV_CLS3_RETRIES_EXCEEDED;
		}

	return MV_OK;
}
/*-------------------------------------------------------------------------------*/

int mvPp2ClsC3HitCntrsRead(int index, MV_U32 *cntr)
{
	unsigned int counter;

	POS_RANGE_VALIDATE(index, MV_PP2_CLS3_HASH_OP_TBL_ADDR_MAX);

	/*write entry index*/
	mvPp2WrReg(MV_PP2_CLS3_DB_INDEX_REG, index);

	/*counter read*/
#ifdef CONFIG_MV_ETH_PP2_1
	counter = mvPp2RdReg(MV_PP2_CLS3_HIT_COUNTER_REG) & MV_PP2_V1_CLS3_HIT_COUNTER_MASK;
#else
	counter = mvPp2RdReg(MV_PP2_CLS3_HIT_COUNTER_REG) & MV_PP2_V0_CLS3_HIT_COUNTER_MASK;
#endif

	if (!cntr)
		mvOsPrintf("ADDR:0x%3.3x	COUNTER VAL:0x%6.6x\n", index, counter);
	else
		*cntr = counter;
	return MV_OK;
}
/*-------------------------------------------------------------------------------*/

int mvPp2ClsC3HitCntrsMissRead(int lkp_type, MV_U32 *cntr)
{
	unsigned int counter;
	int index;

	POS_RANGE_VALIDATE(lkp_type, MV_PP2_CLS_C3_MISS_TBL_SIZE - 1);


	/*set miss bit to 1, ppv2.1 mas 3.16*/
	index = (lkp_type | MV_PP2_CLS3_DB_MISS_MASK);

	/*write entry index*/
	mvPp2WrReg(MV_PP2_CLS3_DB_INDEX_REG, index);

	/*counter read*/
	counter = mvPp2RdReg(MV_PP2_CLS3_HIT_COUNTER_REG) & MV_PP2_V1_CLS3_HIT_COUNTER_MASK;

	if (!cntr)
		mvOsPrintf("LKPT:0x%3.3x	COUNTER VAL:0x%6.6x\n", lkp_type, counter);
	else
		*cntr = counter;
	return MV_OK;
}

/*-------------------------------------------------------------------------------*/

int mvPp2ClsC3HitCntrsReadAll(void)
{
	unsigned int counter, index;

	for (index = 0; index < MV_PP2_CLS_C3_HASH_TBL_SIZE; index++) {
		mvPp2ClsC3HitCntrsRead(index, &counter);

		/* skip initial counter value */
		if (counter == 0)
			continue;

		mvOsPrintf("ADDR:0x%3.3x	COUNTER VAL:0x%6.6x\n", index, counter);
	}

#ifdef CONFIG_MV_ETH_PP2_1
	for (index = 0; index < MV_PP2_CLS_C3_MISS_TBL_SIZE; index++) {
		mvPp2ClsC3HitCntrsMissRead(index, &counter);

		/* skip initial counter value */
		if (counter == 0)
			continue;

		mvOsPrintf("LKPT:0x%3.3x	COUNTER VAL:0x%6.6x\n", index, counter);
	}
#endif
	return MV_OK;
}

/*-------------------------------------------------------------------------------*/
/*	 APIs for Classification C3 hit counters scan fields operation 		 */
/*-------------------------------------------------------------------------------*/
int mvPp2ClsC3ScanStart()
{
	int complete, iter = 0;

	/* trigger scan operation */
	mvPp2WrReg(MV_PP2_CLS3_SC_ACT_REG, (1 << MV_PP2_CLS3_SC_ACT));

	do {
		complete = mvPp2ClsC3ScanIsComplete();

	} while ((!complete) && ((iter++) < RETRIES_EXCEEDED));/*scan compleated*/

	if (iter >= RETRIES_EXCEEDED) {
		return MV_CLS3_RETRIES_EXCEEDED;
	}

	return MV_OK;
}

/*-------------------------------------------------------------------------------*/
int mvPp2ClsC3ScanRegs()
{
	unsigned int prop, propVal;
#ifdef CONFIG_MV_ETH_PP2_1
	unsigned int treshHold;
	treshHold = mvPp2RdReg(MV_PP2_CLS3_SC_TH_REG);
#endif
	prop = mvPp2RdReg(MV_PP2_CLS3_SC_PROP_REG);
	propVal = mvPp2RdReg(MV_PP2_CLS3_SC_PROP_VAL_REG);


	mvOsPrintf("%-32s: 0x%x = 0x%08x\n", "MV_PP2_CLS3_SC_PROP_REG", MV_PP2_CLS3_SC_PROP_REG, prop);
	mvOsPrintf("%-32s: 0x%x = 0x%08x\n", "MV_PP2_CLS3_SC_PROP_VAL_REG", MV_PP2_CLS3_SC_PROP_VAL_REG, propVal);
	mvOsPrintf("\n");

	mvOsPrintf("MODE      = %s\n", ((MV_PP2_CLS3_SC_PROP_TH_MODE_MASK & prop) == 0) ? "Below" : "Above");
	mvOsPrintf("CLEAR     = %s\n", ((MV_PP2_CLS3_SC_PROP_CLEAR_MASK & prop) == 0) ? "NoClear" : "Clear  ");

	/* lookup type */
	((MV_PP2_CLS3_SC_PROP_LKP_TYPE_EN_MASK & prop) == 0) ?
		mvOsPrintf("LKP_TYPE  = NA\n") :
		mvOsPrintf("LKP_TYPE  = 0x%x\n", ((MV_PP2_CLS3_SC_PROP_LKP_TYPE_MASK & prop) >> MV_PP2_CLS3_SC_PROP_LKP_TYPE));

	/* start index */
	mvOsPrintf("START     = 0x%x\n", (MV_PP2_CLS3_SC_PROP_START_ENTRY_MASK & prop) >> MV_PP2_CLS3_SC_PROP_START_ENTRY);
#ifdef CONFIG_MV_ETH_PP2_1
	/* threshold */
	mvOsPrintf("THRESHOLD = 0x%x\n", (MV_PP2_CLS3_SC_TH_MASK & treshHold) >> MV_PP2_CLS3_SC_TH);

	/* delay value */
	mvOsPrintf("DELAY     = 0x%x\n\n",
			(MV_PP2_V1_CLS3_SC_PROP_VAL_DELAY_MASK & propVal) >> MV_PP2_V1_CLS3_SC_PROP_VAL_DELAY);

#else
	/* threshold */
	mvOsPrintf("THRESHOLD = 0x%x\n",
			(MV_PP2_V0_CLS3_SC_PROP_VAL_TH_MASK & propVal) >> MV_PP2_V0_CLS3_SC_PROP_VAL_TH);

	/* delay value */
	mvOsPrintf("DELAY     = 0x%x\n\n",
			(MV_PP2_V0_CLS3_SC_PROP_VAL_DELAY_MASK & propVal) >> MV_PP2_V0_CLS3_SC_PROP_VAL_DELAY);
#endif
	return MV_OK;
}
/*-------------------------------------------------------------------------------*/

/*mod = 0 below th . mode = 1 above threshold*/
int mvPp2ClsC3ScanThreshSet(int mode, int thresh)
{
	unsigned int regVal;

	POS_RANGE_VALIDATE(mode, 1); /* one bit */
#ifdef CONFIG_MV_ETH_PP2_1
	POS_RANGE_VALIDATE(thresh, MV_PP2_CLS3_SC_TH_MAX);
#else
	POS_RANGE_VALIDATE(thresh, MV_PP2_V0_CLS3_SC_PROP_VAL_TH_MAX);
#endif

	regVal = mvPp2RdReg(MV_PP2_CLS3_SC_PROP_REG);
	regVal &= ~MV_PP2_CLS3_SC_PROP_TH_MODE_MASK;
	regVal |= (mode << MV_PP2_CLS3_SC_PROP_TH_MODE);
	mvPp2WrReg(MV_PP2_CLS3_SC_PROP_REG, regVal);

#ifdef CONFIG_MV_ETH_PP2_1
	regVal = mvPp2RdReg(MV_PP2_CLS3_SC_TH_REG);
	regVal &= ~MV_PP2_CLS3_SC_TH_MASK;
	regVal |= (thresh << MV_PP2_CLS3_SC_TH);
	mvPp2WrReg(MV_PP2_CLS3_SC_TH_REG, regVal);
#else
	regVal = mvPp2RdReg(MV_PP2_CLS3_SC_PROP_VAL_REG);
	regVal &= ~MV_PP2_V0_CLS3_SC_PROP_VAL_TH_MASK;
	regVal |= (thresh << MV_PP2_V0_CLS3_SC_PROP_VAL_TH);
	mvPp2WrReg(MV_PP2_CLS3_SC_PROP_VAL_REG, regVal);
#endif

	return MV_OK;
}

/*-------------------------------------------------------------------------------*/
int mvPp2ClsC3ScanLkpTypeSet(int type)
{
	unsigned int prop;

	RANGE_VALIDATE(type, -1, MV_PP2_CLS3_SC_PROP_LKP_TYPE_MAX);
	prop = mvPp2RdReg(MV_PP2_CLS3_SC_PROP_REG);

	if (type == -1)
		/* scan all entries */
		prop &= ~(1 << MV_PP2_CLS3_SC_PROP_LKP_TYPE_EN);
	else {
		/* scan according to lookup type */
		prop |= (1 << MV_PP2_CLS3_SC_PROP_LKP_TYPE_EN);
		prop &= ~MV_PP2_CLS3_SC_PROP_LKP_TYPE_MASK;
		prop |= (type << MV_PP2_CLS3_SC_PROP_LKP_TYPE);
	}

	mvPp2WrReg(MV_PP2_CLS3_SC_PROP_REG, prop);

	return MV_OK;
}
/*-------------------------------------------------------------------------------*/
int mvPp2ClsC3ScanClearBeforeEnSet(int en)
{
	unsigned int prop;

	POS_RANGE_VALIDATE(en, 1); /* one bit */

	prop = mvPp2RdReg(MV_PP2_CLS3_SC_PROP_REG);

	prop &= ~MV_PP2_CLS3_SC_PROP_CLEAR_MASK;
	prop |= (en << MV_PP2_CLS3_SC_PROP_CLEAR);

	mvPp2WrReg(MV_PP2_CLS3_SC_PROP_REG, prop);

	return MV_OK;
}
/*-------------------------------------------------------------------------------*/

int mvPp2ClsC3ScanStartIndexSet(int idx)
{
	unsigned int prop;

	POS_RANGE_VALIDATE(idx, MV_PP2_CLS3_HASH_OP_TBL_ADDR_MAX); /* one bit */

	prop = mvPp2RdReg(MV_PP2_CLS3_SC_PROP_REG);

	prop &= ~MV_PP2_CLS3_SC_PROP_START_ENTRY_MASK;
	prop |= (idx << MV_PP2_CLS3_SC_PROP_START_ENTRY);

	mvPp2WrReg(MV_PP2_CLS3_SC_PROP_REG, prop);

	return MV_OK;
}
/*-------------------------------------------------------------------------------*/

int mvPp2ClsC3ScanDelaySet(int time)
{
	unsigned int propVal;

	POS_RANGE_VALIDATE(time, MV_PP2_CLS3_SC_PROP_VAL_DELAY_MAX);

	propVal = mvPp2RdReg(MV_PP2_CLS3_SC_PROP_VAL_REG);
#ifdef CONFIG_MV_ETH_PP2_1
	propVal &= ~MV_PP2_V1_CLS3_SC_PROP_VAL_DELAY_MASK;
	propVal |= (time << MV_PP2_V1_CLS3_SC_PROP_VAL_DELAY);
#else
	propVal &= ~MV_PP2_V0_CLS3_SC_PROP_VAL_DELAY_MASK;
	propVal |= (time << MV_PP2_V0_CLS3_SC_PROP_VAL_DELAY);
#endif
	mvPp2WrReg(MV_PP2_CLS3_SC_PROP_VAL_REG, propVal);

	return MV_OK;
}

/*-------------------------------------------------------------------------------*/
int mvPp2ClsC3ScanResRead(int index, int *addr, int *cnt)
{
	unsigned int regVal, scState, addres, counter;
	int iter = 0;

	POS_RANGE_VALIDATE(index, MV_PP2_CLS_C3_SC_RES_TBL_SIZE-1);

	do {
		mvPp2ClsC3ScanStateGet(&scState);
	} while (scState != 0 && ((iter++) < RETRIES_EXCEEDED));/*scan compleated*/

	if (iter >= RETRIES_EXCEEDED) {
		mvOsPrintf("%s:Error - retries exceeded.\n", __func__);
		return MV_CLS3_RETRIES_EXCEEDED;
	}

	/*write index*/
	mvPp2WrReg(MV_PP2_CLS3_SC_INDEX_REG, index);

	/*read date*/
	regVal = mvPp2RdReg(MV_PP2_CLS3_SC_RES_REG);
	addres = (regVal & MV_PP2_CLS3_SC_RES_ENTRY_MASK) >> MV_PP2_CLS3_SC_RES_ENTRY;
#ifdef CONFIG_MV_ETH_PP2_1
	counter = (regVal & MV_PP2_V1_CLS3_SC_RES_CTR_MASK) >> MV_PP2_V1_CLS3_SC_RES_CTR;
#else
	counter = (regVal & MV_PP2_V0_CLS3_SC_RES_CTR_MASK) >> MV_PP2_V0_CLS3_SC_RES_CTR;
#endif
	/* if one of parameters is null - func call from sysfs*/
	if ((!addr) | (!cnt))
		mvOsPrintf("INDEX:0x%2.2x	ADDR:0x%3.3x	COUNTER VAL:0x%6.6x\n", index, addres, counter);
	else {
		*addr = addres;
		*cnt = counter;
	}

	return MV_OK;
}
/*-------------------------------------------------------------------------------*/
int mvPp2ClsC3ScanResDump()
{
	int addr, cnt, resNum, index;

	mvPp2ClsC3ScanNumOfResGet(&resNum);

	mvOsPrintf("INDEX	ADDRESS		COUNTER\n");
	for (index = 0; index < resNum; index++) {
		mvPp2ClsC3ScanResRead(index, &addr, &cnt);
		mvOsPrintf("[0x%2.2x]\t[0x%3.3x]\t[0x%6.6x]\n", index, addr, cnt);
	}

	return MV_OK;
}

/*-------------------------------------------------------------------------------*/
int mvPp2ClsC3ScanNumOfResGet(int *resNum)
{
	unsigned int regVal, scState;
	int iter = 0;

	do {
		mvPp2ClsC3ScanStateGet(&scState);
	} while (scState != 0 && ((iter++) < RETRIES_EXCEEDED));/*scan compleated*/

	if (iter >= RETRIES_EXCEEDED) {
		mvOsPrintf("%s:Error - retries exceeded.\n", __func__);
		return MV_CLS3_RETRIES_EXCEEDED;
	}

	regVal = mvPp2RdReg(MV_PP2_CLS3_STATE_REG);
	regVal &= MV_PP2_CLS3_STATE_NO_OF_SC_RES_MASK;
	regVal >>= MV_PP2_CLS3_STATE_NO_OF_SC_RES;
	*resNum = regVal;
	return MV_OK;
}
/*-------------------------------------------------------------------------------

int mvPp2ClsC3ScanTimerGet(int *timer)
{
	unsigned int regVal;

	if (timer == NULL) {
		mvOsPrintf("mvCls3Hw %s: null pointer.\n", __func__);
		return MV_CLS3_ERR;
	}

	regVal = mvPp2RdReg(MV_PP2_CLS3_SC_TIMER_REG);
	regVal &= MV_PP2_CLS3_SC_TIMER_MASK;
	regVal >>= MV_PP2_CLS3_SC_TIMER;
	*timer = regVal;
	return MV_OK;
}
-------------------------------------------------------------------------------------*/
