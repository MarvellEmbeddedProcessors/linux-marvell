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

#include "mvCommon.h"		/* Should be included before mvSysHwConfig */
#include "mvTypes.h"
#include "mv802_3.h"
#include "mvDebug.h"
#include "mvCommon.h"
#include "mvOs.h"
#include "mvSysEthConfig.h"

#include "mvBm.h"

MV_U8 *mvBmVirtBase = 0;
static MV_BM_POOL	mvBmPools[MV_BM_POOLS];

/* Initialize Hardware Buffer management unit */
MV_STATUS mvBmInit(MV_U8 *virtBase)
{

	mvBmVirtBase = virtBase;

	mvBmRegsInit();

	memset(mvBmPools, 0, sizeof(mvBmPools));

	return MV_OK;
}

void mvBmRegsInit(void)
{
	MV_U32 regVal;

	/* Mask BM all interrupts */
	MV_REG_WRITE(MV_BM_INTR_MASK_REG, 0);

	/* Clear BM cause register */
	MV_REG_WRITE(MV_BM_INTR_CAUSE_REG, 0);

	/* Set BM configuration register */
	regVal = MV_REG_READ(MV_BM_CONFIG_REG);

	/* Reduce MaxInBurstSize from 32 BPs to 16 BPs */
	regVal &= ~MV_BM_MAX_IN_BURST_SIZE_MASK;
	regVal |= MV_BM_MAX_IN_BURST_SIZE_16BP;
	MV_REG_WRITE(MV_BM_CONFIG_REG, regVal);

	return;
}

MV_STATUS mvBmControl(MV_COMMAND cmd)
{
	MV_U32 regVal = 0;

	switch (cmd) {
	case MV_START:
		regVal = MV_BM_START_MASK;
		break;

	case MV_STOP:
		regVal = MV_BM_STOP_MASK;
		break;

	case MV_PAUSE:
		regVal = MV_BM_PAUSE_MASK;
		break;

	default:
		mvOsPrintf("bmControl: Unknown command %d\n", cmd);
		return MV_FAIL;
	}
	MV_REG_WRITE(MV_BM_COMMAND_REG, regVal);
	return MV_OK;
}

MV_STATE mvBmStateGet(void)
{
	MV_U32 regVal;
	MV_STATE state;

	regVal = MV_REG_READ(MV_BM_COMMAND_REG);

	switch ((regVal >> MV_BM_STATUS_OFFS) & MV_BM_STATUS_ALL_MASK) {
	case MV_BM_STATUS_ACTIVE:
		state = MV_ACTIVE;
		break;

	case MV_BM_STATUS_NOT_ACTIVE:
		state = MV_IDLE;
		break;

	case MV_BM_STATUS_PAUSED:
		state = MV_PAUSED;
		break;

	default:
		mvOsPrintf("bmStateGet: Unexpected state 0x%x\n", regVal);
		state = MV_UNDEFINED_STATE;
	}
	return state;
}

void      mvBmConfigSet(MV_U32 mask)
{
	MV_U32	regVal;

	regVal = MV_REG_READ(MV_BM_CONFIG_REG);
	regVal |= mask;
	MV_REG_WRITE(MV_BM_CONFIG_REG, regVal);
}

void      mvBmConfigClear(MV_U32 mask)
{
	MV_U32	regVal;

	regVal = MV_REG_READ(MV_BM_CONFIG_REG);
	regVal &= ~mask;
	MV_REG_WRITE(MV_BM_CONFIG_REG, regVal);
}

void mvBmPoolTargetSet(int pool, MV_U8 targetId, MV_U8 attr)
{
	MV_U32 regVal;

	/* validate poolId */
	if ((pool < 0) || (pool >= MV_BM_POOLS)) {
		mvOsPrintf("bmPoolId = %d is invalid \n", pool);
		return;
	}
	/* Read modify write */
	regVal = MV_REG_READ(MV_BM_XBAR_POOL_REG(pool));

	regVal &= ~MV_BM_TARGET_ID_MASK(pool);
	regVal &= ~MV_BM_XBAR_ATTR_MASK(pool);
	regVal |= MV_BM_TARGET_ID_VAL(pool, targetId);
	regVal |= MV_BM_XBAR_ATTR_VAL(pool, attr);

	MV_REG_WRITE(MV_BM_XBAR_POOL_REG(pool), regVal);
}

void mvBmPoolEnable(int pool)
{
	MV_U32 regVal;

	/* validate poolId */
	if ((pool < 0) || (pool >= MV_BM_POOLS)) {
		mvOsPrintf("bmPoolId = %d is invalid \n", pool);
		return;
	}
	regVal = MV_REG_READ(MV_BM_POOL_BASE_REG(pool));
	regVal |= MV_BM_POOL_ENABLE_MASK;
	MV_REG_WRITE(MV_BM_POOL_BASE_REG(pool), regVal);

	/* Clear BM cause register */
	MV_REG_WRITE(MV_BM_INTR_CAUSE_REG, 0);

}

void mvBmPoolDisable(int pool)
{
	MV_U32 regVal;

	/* validate poolId */
	if ((pool < 0) || (pool >= MV_BM_POOLS)) {
		mvOsPrintf("bmPoolId = %d is invalid \n", pool);
		return;
	}
	regVal = MV_REG_READ(MV_BM_POOL_BASE_REG(pool));
	regVal &= ~MV_BM_POOL_ENABLE_MASK;
	MV_REG_WRITE(MV_BM_POOL_BASE_REG(pool), regVal);
}

MV_BOOL mvBmPoolIsEnabled(int pool)
{
	MV_U32 regVal;

	/* validate poolId */
	if ((pool < 0) || (pool >= MV_BM_POOLS)) {
		mvOsPrintf("bmPoolId = %d is invalid \n", pool);
		return MV_FALSE;
	}
	regVal = MV_REG_READ(MV_BM_POOL_BASE_REG(pool));
	return (regVal & MV_BM_POOL_ENABLE_MASK);
}

/* Configure BM specific pool of "capacity" size. */
MV_STATUS mvBmPoolInit(int pool, void *virtPoolBase, MV_ULONG physPoolBase, int capacity)
{
	MV_BM_POOL	*pBmPool;

	/* validate poolId */
	if ((pool < 0) || (pool >= MV_BM_POOLS)) {
		mvOsPrintf("bmPoolId = %d is invalid \n", pool);
		return MV_BAD_PARAM;
	}
	/* poolBase must be 4 byte aligned */
	if (MV_IS_NOT_ALIGN(physPoolBase, MV_BM_POOL_PTR_ALIGN)) {
		mvOsPrintf("bmPoolBase = 0x%lx is not aligned 4 bytes\n", physPoolBase);
		return MV_NOT_ALIGNED;
	}
	/* Minimum pool capacity is 128 entries */
	if (capacity < MV_BM_POOL_CAP_MIN) {
		mvOsPrintf("bmPool capacity = %d is smaller than minimum (%d)\n", capacity, MV_BM_POOL_CAP_MIN);
		return MV_BAD_SIZE;
	}
	/* Update data structure */
	pBmPool = &mvBmPools[pool];
	if (pBmPool->pVirt != NULL) {
		mvOsPrintf("bmPool = %d is already busy\n", pool);
		/* necessary for power managemaent resume process*/
		/*return MV_BUSY;*/
	}

	pBmPool->pool = pool;
	pBmPool->capacity = capacity;
	pBmPool->pVirt = virtPoolBase;
	pBmPool->physAddr = physPoolBase;

	/* Maximum pool capacity is 16K entries (2^14) */
	if (capacity > MV_BM_POOL_CAP_MAX) {
		mvOsPrintf("bmPool capacity = %d is larger than maximum (%d)\n", capacity, MV_BM_POOL_CAP_MAX);
		return MV_BAD_SIZE;
	}

	/* Set poolBase address */
	MV_REG_WRITE(MV_BM_POOL_BASE_REG(pool), physPoolBase);

	/* Set Read pointer to 0 */
	MV_REG_WRITE(MV_BM_POOL_READ_PTR_REG(pool), 0);

	/* Set Read pointer to 0 */
	MV_REG_WRITE(MV_BM_POOL_WRITE_PTR_REG(pool), 0);

	/* Set Pool size */
	MV_REG_WRITE(MV_BM_POOL_SIZE_REG(pool), MV_BM_POOL_SIZE_VAL(capacity));

	return MV_OK;
}

MV_STATUS mvBmPoolBufSizeSet(int pool, int buf_size)
{
	MV_BM_POOL *pBmPool;

	/* validate poolId */
	if ((pool < 0) || (pool >= MV_BM_POOLS)) {
		mvOsPrintf("bmPoolId = %d is invalid \n", pool);
		return MV_BAD_PARAM;
	}
	pBmPool = &mvBmPools[pool];

	pBmPool->bufSize = buf_size;

	return MV_OK;
}

MV_STATUS mvBmPoolBufNumUpdate(int pool, int buf_num)
{
	MV_BM_POOL *pBmPool;

	/* validate poolId */
	if ((pool < 0) || (pool >= MV_BM_POOLS)) {
		mvOsPrintf("bmPoolId = %d is invalid \n", pool);
		return MV_BAD_PARAM;
	}

	pBmPool = &mvBmPools[pool];
	if (pBmPool->bufSize == 0) {
		mvOsPrintf("bmPoolId = %d has unknown buf_size  \n", pool);
		return MV_BAD_PARAM;
	}
	pBmPool->bufNum += buf_num;
	return MV_OK;
}

void mvBmPoolPrint(int pool)
{
	MV_BM_POOL *pBmPool;

	/* validate poolId */
	if ((pool < 0) || (pool >= MV_BM_POOLS)) {
		mvOsPrintf("bmPoolId = %d is invalid \n", pool);
		return;
	}

	pBmPool = &mvBmPools[pool];
	if (pBmPool->pVirt == NULL) {
		mvOsPrintf("bmPool = %d is not created yet\n", pool);
		return;
	}

	mvOsPrintf("  %2d:     %4d       %4d       %4d      %p      0x%08x\n",
						pBmPool->pool, pBmPool->capacity, pBmPool->bufSize, pBmPool->bufNum,
						pBmPool->pVirt, (unsigned)pBmPool->physAddr);
}

void mvBmStatus(void)
{
	int i;

	mvOsPrintf("BM Pools status\n");
	mvOsPrintf("pool:    capacity    bufSize    bufNum      virtPtr       physAddr\n");
	for (i = 0; i < MV_BM_POOLS; i++)
		mvBmPoolPrint(i);
}

void mvBmPoolDump(int pool, int mode)
{
	MV_U32     regVal;
	MV_ULONG   *pBufAddr;
	MV_BM_POOL *pBmPool;
	int setReadIdx, getReadIdx, setWriteIdx, getWriteIdx, freeBuffs, i;

	/* validate poolId */
	if ((pool < 0) || (pool >= MV_BM_POOLS)) {
		mvOsPrintf("bmPoolId = %d is invalid \n", pool);
		return;
	}

	pBmPool = &mvBmPools[pool];
	if (pBmPool->pVirt == NULL) {
		mvOsPrintf("bmPool = %d is not created yet\n", pool);
		return;
	}

	mvOsPrintf("\n[NETA BM: pool=%d, mode=%d]\n", pool, mode);

	mvOsPrintf("poolBase=%p (0x%x), capacity=%d, buf_num=%d, buf_size=%d\n",
		   pBmPool->pVirt, (unsigned)pBmPool->physAddr, pBmPool->capacity, pBmPool->bufNum, pBmPool->bufSize);

	regVal = MV_REG_READ(MV_BM_POOL_READ_PTR_REG(pool));
	setReadIdx = ((regVal & MV_BM_POOL_SET_READ_PTR_MASK) >> MV_BM_POOL_SET_READ_PTR_OFFS) / 4;
	getReadIdx = ((regVal & MV_BM_POOL_GET_READ_PTR_MASK) >> MV_BM_POOL_GET_READ_PTR_OFFS) / 4;

	regVal = MV_REG_READ(MV_BM_POOL_WRITE_PTR_REG(pool));
	setWriteIdx = ((regVal & MV_BM_POOL_SET_WRITE_PTR_MASK) >> MV_BM_POOL_SET_WRITE_PTR_OFFS) / 4;
	getWriteIdx = ((regVal & MV_BM_POOL_GET_WRITE_PTR_MASK) >> MV_BM_POOL_GET_WRITE_PTR_OFFS) / 4;
	if (getWriteIdx >= getReadIdx)
		freeBuffs = getWriteIdx - getReadIdx;
	else
		freeBuffs = (pBmPool->capacity - getReadIdx) + getWriteIdx;

	mvOsPrintf("nextToRead: set=%d, get=%d, nextToWrite: set=%d, get=%d, freeBuffs=%d\n",
		setReadIdx, getReadIdx, setWriteIdx, getWriteIdx, freeBuffs);

	if (mode > 0) {
		/* Print the content of BM pool */
		i = getReadIdx;
		while (i != getWriteIdx) {
			pBufAddr = (MV_ULONG *)pBmPool->pVirt + i;
			mvOsPrintf("%3d. pBufAddr=%p, bufAddr=%08x\n",
				   i, pBufAddr, (MV_U32)(*pBufAddr));
			i++;
			if (i == pBmPool->capacity)
				i = 0;
		}
	}
}

void mvBmRegs(void)
{
	int pool;

	mvOsPrintf("\n\t Hardware Buffer Management Registers:\n");

	mvOsPrintf("MV_BM_CONFIG_REG                : 0x%X = 0x%08x\n",
		   MV_BM_CONFIG_REG, MV_REG_READ(MV_BM_CONFIG_REG));

	mvOsPrintf("MV_BM_COMMAND_REG               : 0x%X = 0x%08x\n",
		   MV_BM_COMMAND_REG, MV_REG_READ(MV_BM_COMMAND_REG));

	mvOsPrintf("MV_BM_INTR_CAUSE_REG            : 0x%X = 0x%08x\n",
		   MV_BM_INTR_CAUSE_REG, MV_REG_READ(MV_BM_INTR_CAUSE_REG));

	mvOsPrintf("MV_BM_INTR_MASK_REG             : 0x%X = 0x%08x\n",
		   MV_BM_INTR_MASK_REG, MV_REG_READ(MV_BM_INTR_MASK_REG));

	mvOsPrintf("MV_BM_XBAR_01_REG               : 0x%X = 0x%08x\n",
		   MV_BM_XBAR_01_REG, MV_REG_READ(MV_BM_XBAR_01_REG));

	mvOsPrintf("MV_BM_XBAR_23_REG               : 0x%X = 0x%08x\n",
		   MV_BM_XBAR_23_REG, MV_REG_READ(MV_BM_XBAR_23_REG));

	for (pool = 0; pool < MV_BM_POOLS; pool++) {
		mvOsPrintf("\n\t BM Pool #%d registers:\n", pool);

		mvOsPrintf("MV_BM_POOL_BASE_REG             : 0x%X = 0x%08x\n",
			MV_BM_POOL_BASE_REG(pool), MV_REG_READ(MV_BM_POOL_BASE_REG(pool)));

		mvOsPrintf("MV_BM_POOL_READ_PTR_REG         : 0x%X = 0x%08x\n",
			MV_BM_POOL_READ_PTR_REG(pool), MV_REG_READ(MV_BM_POOL_READ_PTR_REG(pool)));

		mvOsPrintf("MV_BM_POOL_WRITE_PTR_REG        : 0x%X = 0x%08x\n",
			MV_BM_POOL_WRITE_PTR_REG(pool), MV_REG_READ(MV_BM_POOL_WRITE_PTR_REG(pool)));

		mvOsPrintf("MV_BM_POOL_SIZE_REG             : 0x%X = 0x%08x\n",
			MV_BM_POOL_SIZE_REG(pool), MV_REG_READ(MV_BM_POOL_SIZE_REG(pool)));
	}
	mvOsPrintf("\n");

	mvOsPrintf("MV_BM_DEBUG_REG                 : 0x%X = 0x%08x\n", MV_BM_DEBUG_REG, MV_REG_READ(MV_BM_DEBUG_REG));

	mvOsPrintf("MV_BM_READ_PTR_REG              : 0x%X = 0x%08x\n",
		   MV_BM_READ_PTR_REG, MV_REG_READ(MV_BM_READ_PTR_REG));

	mvOsPrintf("MV_BM_WRITE_PTR_REG             : 0x%X = 0x%08x\n",
		   MV_BM_WRITE_PTR_REG, MV_REG_READ(MV_BM_WRITE_PTR_REG));

	mvOsPrintf("\n");

}
