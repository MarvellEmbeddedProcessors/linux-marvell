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

#include "mvBm.h"

static MV_BM_POOL	mvBmPools[MV_BM_POOLS];

static MV_BM_QSET	mvBmQsets[MV_BM_QSET_MAX];
static int		mvBmRxqToQsetLong[MV_BM_QSET_PRIO_MAX];
static int		mvBmRxqToQsetShort[MV_BM_QSET_PRIO_MAX];
static int		mvBmTxqToQsetLong[MV_BM_QSET_PRIO_MAX];
static int		mvBmTxqToQsetShort[MV_BM_QSET_PRIO_MAX];

/* Initialize Hardware Buffer management unit */
MV_STATUS mvBmInit()
{
	int i;

	for (i = 0; i < MV_BM_POOLS; i++) {
		/* Mask BM all interrupts */
		mvPp2WrReg(MV_BM_INTR_MASK_REG(i), 0);
		/* Clear BM cause register */
		mvPp2WrReg(MV_BM_INTR_CAUSE_REG(i), 0);
	}

	memset(mvBmPools, 0, sizeof(mvBmPools));

#ifdef CONFIG_MV_ETH_PP2_1
	/* Enable BM priority */
	mvPp2WrReg(MV_BM_PRIO_CTRL_REG, 1);

	/* Initialize Qsets */
	for (i = 0; i < MV_BM_QSET_MAX; i++) {
		mvBmQsets[i].id = i;
		mvBmQsets[i].pool = -1;
	}

	for (i = 0; i < MV_BM_QSET_PRIO_MAX; i++) {
		mvBmRxqToQsetLong[i] = -1;
		mvBmRxqToQsetShort[i] = -1;
		mvBmTxqToQsetLong[i] = -1;
		mvBmTxqToQsetShort[i] = -1;
	}
#endif

	return MV_OK;
}

MV_STATUS mvBmPoolControl(int pool, MV_COMMAND cmd)
{
	MV_U32 regVal = 0;
	regVal = mvPp2RdReg(MV_BM_POOL_CTRL_REG(pool));

	switch (cmd) {
	case MV_START:
		regVal |= MV_BM_START_MASK;
		break;

	case MV_STOP:
		regVal |= MV_BM_STOP_MASK;
		break;

	default:
		mvOsPrintf("bmControl: Unknown command %d\n", cmd);
		return MV_FAIL;
	}
	mvPp2WrReg(MV_BM_POOL_CTRL_REG(pool), regVal);
	return MV_OK;
}

MV_STATE mvBmPoolStateGet(int pool)
{
	MV_U32 regVal;
	MV_STATE state;

	regVal = mvPp2RdReg(MV_BM_POOL_CTRL_REG(pool));

	if (regVal & MV_BM_STATE_MASK)
		state = MV_ACTIVE;
	else
		state = MV_IDLE;

	return state;
}

/* Configure BM specific pool of "capacity" size. */
MV_STATUS mvBmPoolInit(int pool, MV_U32 *virtPoolBase, MV_ULONG physPoolBase, int capacity)
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
	if (MV_IS_NOT_ALIGN(capacity, 16)) {
		mvOsPrintf("%s: Illegal pool capacity %d, ", __func__, capacity);
		capacity = MV_ALIGN_UP(capacity, 16);
		mvOsPrintf("round to: %d\n", capacity);
	}
	/* Minimum pool capacity is 128 entries */
	if (capacity < MV_BM_POOL_CAP_MIN) {
		mvOsPrintf("bmPool capacity = %d is smaller than minimum (%d)\n", capacity, MV_BM_POOL_CAP_MIN);
		return MV_BAD_SIZE;
	}
	/* Maximum pool capacity is 16K entries (2^14) */
	if (capacity > MV_BM_POOL_CAP_MAX) {
		mvOsPrintf("bmPool capacity = %d is larger than maximum (%d)\n", capacity, MV_BM_POOL_CAP_MAX);
		return MV_BAD_SIZE;
	}
	/* Update data structure */
	pBmPool = &mvBmPools[pool];
	if (pBmPool->physAddr) {
		mvOsPrintf("bmPool = %d is already busy\n", pool);
		return MV_BUSY;
	}

	pBmPool->pool = pool;
	pBmPool->capacity = capacity;
	pBmPool->physAddr = physPoolBase;
	pBmPool->pVirt = virtPoolBase;

	mvBmPoolControl(pool, MV_STOP);

#ifdef CONFIG_MV_ETH_PP2_1
	/* Init Qsets list for this pool */
	pBmPool->qsets = mvListCreate();
	if (pBmPool->qsets == NULL) {
		mvOsPrintf("%s: failed to create Qsets list\n", __func__);
		return MV_FAIL;
	}

	/* Assign and init default Qset for this pool */
	pBmPool->defQset = &mvBmQsets[MV_BM_POOL_QSET_BASE + pool];

	mvBmQsetCreate(pBmPool->defQset->id, pool);
	mvBmQsetBuffMaxSet(pBmPool->defQset->id, 0, 0);
	mvBmQsetBuffCountersSet(pBmPool->defQset->id, 0, 0);

	/* Assign and init MC Qset for this pool */
	pBmPool->mcQset = &mvBmQsets[pool];

	mvBmQsetCreate(pBmPool->mcQset->id, pool);
	mvBmQsetBuffMaxSet(pBmPool->mcQset->id, 0, 0);
	mvBmQsetBuffCountersSet(pBmPool->mcQset->id, 0, 0);

	/* Init default priority counters for this pool */
	mvBmPoolBuffNumSet(pool, 0);
	mvBmPoolBuffCountersSet(pool, 0, 0);
#endif

	/* Set poolBase address */
	mvPp2WrReg(MV_BM_POOL_BASE_REG(pool), physPoolBase);

	/* Set Pool size */
	mvPp2WrReg(MV_BM_POOL_SIZE_REG(pool), capacity);

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

MV_STATUS mvBmPoolBufNumUpdate(int pool, int buf_num, int add)
{
	MV_BM_POOL *pBmPool;

	/* validate poolId */
	if ((pool < 0) || (pool >= MV_BM_POOLS)) {
		mvOsPrintf("bmPoolId = %d is invalid \n", pool);
		return MV_BAD_PARAM;
	}

	pBmPool = &mvBmPools[pool];

	if (add)
		pBmPool->bufNum += buf_num;
	else
		pBmPool->bufNum -= buf_num;

#ifdef CONFIG_MV_ETH_PP2_1
	/* Update max buffers of default Qset, MC Qset and pool shared */
	if (add) {
		mvBmQsetBuffMaxSet(pBmPool->defQset->id,
			pBmPool->defQset->maxGrntd, pBmPool->defQset->maxShared + buf_num);
		mvBmQsetBuffMaxSet(pBmPool->mcQset->id,
			pBmPool->mcQset->maxGrntd, pBmPool->mcQset->maxShared + buf_num);
		mvBmPoolBuffNumSet(pool, pBmPool->maxShared + buf_num);
	} else {
		mvBmQsetBuffMaxSet(pBmPool->defQset->id,
			pBmPool->defQset->maxGrntd, pBmPool->defQset->maxShared - buf_num);
		mvBmQsetBuffMaxSet(pBmPool->mcQset->id,
			pBmPool->mcQset->maxGrntd, pBmPool->mcQset->maxShared - buf_num);
		mvBmPoolBuffNumSet(pool, pBmPool->maxShared - buf_num);
	}
#endif

	return MV_OK;
}

/******************************************************************************/
/* BM priority API */
MV_STATUS mvBmQsetCreate(int qset, int pool)
{
	if ((pool < 0) || (pool >= MV_BM_POOLS)) {
		mvOsPrintf("%s: bmPoolId = %d is invalid\n", __func__, pool);
		return MV_BAD_PARAM;
	}
	if (qset < 0 || qset >= MV_BM_QSET_MAX) {
		mvOsPrintf("%s: Bad qset number = %d\n", __func__, qset);
		return MV_BAD_PARAM;
	}
	if (mvBmQsets[qset].pool != -1) {
		mvOsPrintf("%s: qset %d is already attached to pool %d\n", __func__, qset, mvBmQsets[qset].pool);
		return MV_FAIL;
	}

	mvBmQsets[qset].pool = pool;
	mvBmQsets[qset].refCount = 0;
	mvBmQsets[qset].maxShared = 0;
	mvBmQsets[qset].maxGrntd = 0;

	mvListAddHead(mvBmPools[pool].qsets, (MV_ULONG)qset);

	return MV_OK;
}

MV_STATUS mvBmQsetDelete(int qset)
{
	int pool;

	if (qset < 0 || qset >= MV_BM_QSET_MAX) {
		mvOsPrintf("%s: Bad qset number = %d\n", __func__, qset);
		return MV_BAD_PARAM;
	}

	if (mvBmQsets[qset].refCount > 0) {
		mvOsPrintf("%s: qset number %d has RXQs/TXQs that use it\n", __func__, qset);
		return MV_FAIL;
	}

	pool = mvBmQsets[qset].pool;
	if (pool != -1) {
		MV_LIST_ELEMENT *elm;

		if (qset == mvBmDefaultQsetNumGet(pool)) {
			mvOsPrintf("%s: Can't delete Bm pool's default Qset (%d)\n", __func__, qset);
			return MV_BAD_PARAM;
		}

		elm = mvListFind(mvBmPools[pool].qsets, (MV_ULONG)qset);

		mvListDel(elm);
	}

	mvBmQsets[qset].pool = -1;

	return MV_OK;
}

int mvBmDefaultQsetNumGet(int pool)
{
	/* validate poolId */
	if ((pool < 0) || (pool >= MV_BM_POOLS)) {
		mvOsPrintf("%s: bmPoolId = %d is invalid\n", __func__, pool);
		return MV_BAD_PARAM;
	}

	return mvBmPools[pool].defQset->id;
}

MV_STATUS mvBmRxqToQsetLongClean(int queue)
{
	int oldQset;

	if (queue < 0 || queue > MV_BM_PRIO_IDX_MASK) {
		mvOsPrintf("%s: Bad queue number = %d\n", __func__, queue);
		return MV_BAD_PARAM;
	}

	oldQset = mvBmRxqToQsetLong[queue];
	if (oldQset != -1)
		mvBmQsets[oldQset].refCount--;

	mvBmRxqToQsetLong[queue] = -1;

	return MV_OK;
}

MV_STATUS mvBmRxqToQsetShortClean(int queue)
{
	int oldQset;

	if (queue < 0 || queue > MV_BM_PRIO_IDX_MASK) {
		mvOsPrintf("%s: Bad queue number = %d\n", __func__, queue);
		return MV_BAD_PARAM;
	}

	oldQset = mvBmRxqToQsetShort[queue];
	if (oldQset != -1)
		mvBmQsets[oldQset].refCount--;

	mvBmRxqToQsetShort[queue] = -1;

	return MV_OK;
}

MV_STATUS mvBmTxqToQsetLongClean(int queue)
{
	int oldQset;

	if (queue < 0 || queue > MV_BM_PRIO_IDX_MASK) {
		mvOsPrintf("%s: Bad queue number = %d\n", __func__, queue);
		return MV_BAD_PARAM;
	}

	oldQset = mvBmTxqToQsetLong[queue];
	if (oldQset != -1)
		mvBmQsets[oldQset].refCount--;

	mvBmTxqToQsetLong[queue] = -1;

	return MV_OK;
}

MV_STATUS mvBmTxqToQsetShortClean(int queue)
{
	int oldQset;

	if (queue < 0 || queue > MV_BM_PRIO_IDX_MASK) {
		mvOsPrintf("%s: Bad queue number = %d\n", __func__, queue);
		return MV_BAD_PARAM;
	}

	oldQset = mvBmTxqToQsetShort[queue];
	if (oldQset != -1)
		mvBmQsets[oldQset].refCount--;

	mvBmTxqToQsetShort[queue] = -1;

	return MV_OK;
}

MV_STATUS mvBmRxqToQsetLongSet(int queue, int qset)
{
	MV_U32 regVal;

	if (queue < 0 || queue > MV_BM_PRIO_IDX_MASK) {
		mvOsPrintf("%s: Bad queue number = %d\n", __func__, queue);
		return MV_BAD_PARAM;
	}
	if (qset < 0 || qset >= MV_BM_QSET_MAX) {
		mvOsPrintf("%s: Bad qset number = %d\n", __func__, qset);
		return MV_BAD_PARAM;
	}
	if (mvBmQsets[qset].pool == -1) {
		mvOsPrintf("%s: qset %d is not attached to BM pool\n", __func__, qset);
		return MV_FAIL;
	}

	/* Same Qset */
	if (mvBmRxqToQsetLong[queue] == qset)
		return MV_OK;

	/* Remove old Qset */
	if (mvBmRxqToQsetLong[queue] != -1) {
		int oldQset = mvBmRxqToQsetLong[queue];

		/* Check that queue is using the same BM pool */
		if (mvBmQsets[qset].pool != mvBmQsets[oldQset].pool) {
			mvOsPrintf("%s: queue %d is attached BM pool %d, but new qset %d is attached to BM pool %d\n",
				__func__, queue, mvBmQsets[oldQset].pool, qset, mvBmQsets[qset].pool);
			return MV_FAIL;
		}
		mvBmQsets[oldQset].refCount--;
	}

	mvPp2WrReg(MV_BM_PRIO_IDX_REG, queue);

	regVal = mvPp2RdReg(MV_BM_CPU_QSET_REG);
	regVal &= ~MV_BM_CPU_LONG_QSET_MASK;
	regVal |= ((qset << MV_BM_CPU_LONG_QSET_OFFS) & MV_BM_CPU_LONG_QSET_MASK);

	mvPp2WrReg(MV_BM_CPU_QSET_REG, regVal);

	mvBmRxqToQsetLong[queue] = qset;
	mvBmQsets[qset].refCount++;

	return MV_OK;
}

MV_STATUS mvBmRxqToQsetShortSet(int queue, int qset)
{
	MV_U32 regVal;

	if (queue < 0 || queue > MV_BM_PRIO_IDX_MASK) {
		mvOsPrintf("%s: Bad queue number = %d\n", __func__, queue);
		return MV_BAD_PARAM;
	}
	if (qset < 0 || qset >= MV_BM_QSET_MAX) {
		mvOsPrintf("%s: Bad qset number = %d\n", __func__, qset);
		return MV_BAD_PARAM;
	}
	if (mvBmQsets[qset].pool == -1) {
		mvOsPrintf("%s: qset %d is not attached to BM pool\n", __func__, qset);
		return MV_FAIL;
	}

	/* Same Qset */
	if (mvBmRxqToQsetShort[queue] == qset)
		return MV_OK;

	/* Remove old Qset */
	if (mvBmRxqToQsetShort[queue] != -1) {
		int oldQset = mvBmRxqToQsetShort[queue];

		/* Check that queue is using the same BM pool */
		if (mvBmQsets[qset].pool != mvBmQsets[oldQset].pool) {
			mvOsPrintf("%s: queue %d is attached BM pool %d, but new qset %d is attached to BM pool %d\n",
				__func__, queue, mvBmQsets[oldQset].pool, qset, mvBmQsets[qset].pool);
			return MV_FAIL;
		}
		mvBmQsets[oldQset].refCount--;
	}

	mvPp2WrReg(MV_BM_PRIO_IDX_REG, queue);

	regVal = mvPp2RdReg(MV_BM_CPU_QSET_REG);
	regVal &= ~MV_BM_CPU_SHORT_QSET_MASK;
	regVal |= ((qset << MV_BM_CPU_SHORT_QSET_OFFS) & MV_BM_CPU_SHORT_QSET_MASK);

	mvPp2WrReg(MV_BM_CPU_QSET_REG, regVal);

	mvBmRxqToQsetShort[queue] = qset;
	mvBmQsets[qset].refCount++;

	return MV_OK;
}

MV_STATUS mvBmTxqToQsetLongSet(int queue, int qset)
{
	MV_U32 regVal;

	if (queue < 0 || queue > MV_BM_PRIO_IDX_MASK) {
		mvOsPrintf("%s: Bad queue number = %d\n", __func__, queue);
		return MV_BAD_PARAM;
	}
	if (qset < 0 || qset >= MV_BM_QSET_MAX) {
		mvOsPrintf("%s: Bad qset number = %d\n", __func__, qset);
		return MV_BAD_PARAM;
	}
	if (mvBmQsets[qset].pool == -1) {
		mvOsPrintf("%s: qset %d is not attached to BM pool\n", __func__, qset);
		return MV_FAIL;
	}

	/* Same Qset */
	if (mvBmTxqToQsetLong[queue] == qset)
		return MV_OK;

	/* Remove old Qset */
	if (mvBmTxqToQsetLong[queue] != -1) {
		int oldQset = mvBmTxqToQsetLong[queue];

		/* Check that queue is using the same BM pool */
		if (mvBmQsets[qset].pool != mvBmQsets[oldQset].pool) {
			mvOsPrintf("%s: queue %d is attached BM pool %d, but new qset %d is attached to BM pool %d\n",
				__func__, queue, mvBmQsets[oldQset].pool, qset, mvBmQsets[qset].pool);
			return MV_FAIL;
		}
		mvBmQsets[oldQset].refCount--;
	}

	mvPp2WrReg(MV_BM_PRIO_IDX_REG, queue);

	regVal = mvPp2RdReg(MV_BM_HWF_QSET_REG);
	regVal &= ~MV_BM_HWF_LONG_QSET_MASK;
	regVal |= ((qset << MV_BM_HWF_LONG_QSET_OFFS) & MV_BM_HWF_LONG_QSET_MASK);

	mvPp2WrReg(MV_BM_HWF_QSET_REG, regVal);

	mvBmTxqToQsetLong[queue] = qset;
	mvBmQsets[qset].refCount++;

	return MV_OK;
}

MV_STATUS mvBmTxqToQsetShortSet(int queue, int qset)
{
	MV_U32 regVal;

	if (queue < 0 || queue > MV_BM_PRIO_IDX_MASK) {
		mvOsPrintf("%s: Bad queue number = %d\n", __func__, queue);
		return MV_BAD_PARAM;
	}
	if (qset < 0 || qset >= MV_BM_QSET_MAX) {
		mvOsPrintf("%s: Bad qset number = %d\n", __func__, qset);
		return MV_BAD_PARAM;
	}
	if (mvBmQsets[qset].pool == -1) {
		mvOsPrintf("%s: qset %d is not attached to BM pool\n", __func__, qset);
		return MV_FAIL;
	}

	/* Same Qset */
	if (mvBmTxqToQsetShort[queue] == qset)
		return MV_OK;

	/* Remove old Qset */
	if (mvBmTxqToQsetShort[queue] != -1) {
		int oldQset = mvBmTxqToQsetShort[queue];

		/* Check that queue is using the same BM pool */
		if (mvBmQsets[qset].pool != mvBmQsets[oldQset].pool) {
			mvOsPrintf("%s: queue %d is attached BM pool %d, but new qset %d is attached to BM pool %d\n",
				__func__, queue, mvBmQsets[oldQset].pool, qset, mvBmQsets[qset].pool);
			return MV_FAIL;
		}
		mvBmQsets[oldQset].refCount--;
	}

	mvPp2WrReg(MV_BM_PRIO_IDX_REG, queue);

	regVal = mvPp2RdReg(MV_BM_HWF_QSET_REG);
	regVal &= ~MV_BM_HWF_SHORT_QSET_MASK;
	regVal |= ((qset << MV_BM_HWF_SHORT_QSET_OFFS) & MV_BM_HWF_SHORT_QSET_MASK);

	mvPp2WrReg(MV_BM_HWF_QSET_REG, regVal);

	mvBmTxqToQsetShort[queue] = qset;
	mvBmQsets[qset].refCount++;

	return MV_OK;
}

int mvBmRxqToQsetLongGet(int queue)
{
	if (queue < 0 || queue > MV_BM_PRIO_IDX_MASK) {
		mvOsPrintf("%s: Bad queue number = %d\n", __func__, queue);
		return -1;
	}

	return mvBmRxqToQsetLong[queue];
}

int mvBmRxqToQsetShortGet(int queue)
{
	if (queue < 0 || queue > MV_BM_PRIO_IDX_MASK) {
		mvOsPrintf("%s: Bad queue number = %d\n", __func__, queue);
		return -1;
	}

	return mvBmRxqToQsetShort[queue];
}

int mvBmTxqToQsetLongGet(int queue)
{
	if (queue < 0 || queue > MV_BM_PRIO_IDX_MASK) {
		mvOsPrintf("%s: Bad queue number = %d\n", __func__, queue);
		return -1;
	}

	return mvBmTxqToQsetLong[queue];
}

int mvBmTxqToQsetShortGet(int queue)
{
	if (queue < 0 || queue > MV_BM_PRIO_IDX_MASK) {
		mvOsPrintf("%s: Bad queue number = %d\n", __func__, queue);
		return -1;
	}

	return mvBmTxqToQsetShort[queue];
}

MV_STATUS mvBmQsetBuffMaxSet(int qset, int maxGrntd, int maxShared)
{
	MV_U32 regVal = 0;
	int pool, delta;
	MV_BM_POOL *pBmPool;
	MV_BM_QSET *pQset;

	if (qset < 0 || qset >= MV_BM_QSET_MAX) {
		mvOsPrintf("%s: Bad qset number = %d\n", __func__, qset);
		return MV_BAD_PARAM;
	}

	pQset = &mvBmQsets[qset];
	pool = pQset->pool;
	if (pool == -1) {
		mvOsPrintf("%s: Qset (%d) is not attached to any BM pool\n", __func__, qset);
		return MV_FAIL;
	}

	/* number of requested guaranteed buffers from BM pool (after Qset max update) */
	delta = maxGrntd - pQset->maxGrntd;

	pBmPool = &mvBmPools[pool];
	if (pBmPool->maxShared < delta) {
		mvOsPrintf("%s: Not enough buffers (%d) in BM pool %d to guarantee %d buffer for qset %d\n",
				__func__, pBmPool->maxShared, pool, delta, qset);
		return MV_FAIL;
	}

	/* Update BM pool shared buffers num */
	mvBmPoolBuffNumSet(pool, pBmPool->maxShared - delta);

	pQset->maxShared = maxShared;
	pQset->maxGrntd = maxGrntd;

	regVal |= maxShared << MV_BM_QSET_MAX_SHARED_OFFS;
	regVal |= maxGrntd << MV_BM_QSET_MAX_GRNTD_OFFS;

	mvPp2WrReg(MV_BM_PRIO_IDX_REG, qset);
	mvPp2WrReg(MV_BM_QSET_SET_MAX_REG, regVal);

	return MV_OK;
}

MV_STATUS mvBmQsetBuffCountersSet(int qset, int cntrGrntd, int cntrShared)
{
	MV_U32 regVal = 0;

	if (qset < 0 || qset >= MV_BM_QSET_MAX) {
		mvOsPrintf("%s: Bad qset number = %d\n", __func__, qset);
		return MV_BAD_PARAM;
	}

	regVal |= cntrShared << MV_BM_QSET_CNTR_SHARED_OFFS;
	regVal |= cntrGrntd << MV_BM_QSET_CNTR_GRNTD_OFFS;

	mvPp2WrReg(MV_BM_PRIO_IDX_REG, qset);
	mvPp2WrReg(MV_BM_QSET_SET_CNTRS_REG, regVal);

	return MV_OK;
}

/* Set number of SHARED buffers for this pool */
MV_STATUS mvBmPoolBuffNumSet(int pool, int buffNum)
{
	if ((pool < 0) || (pool >= MV_BM_POOLS)) {
		mvOsPrintf("bmPoolId = %d is invalid\n", pool);
		return MV_BAD_PARAM;
	}

	mvBmPools[pool].maxShared = buffNum;

	mvPp2WrReg(MV_BM_POOL_MAX_SHARED_REG(pool), buffNum);

	return MV_OK;
}

MV_STATUS mvBmPoolBuffCountersSet(int pool, int cntrGrntd, int cntrShared)
{
	MV_U32 regVal = 0;

	if ((pool < 0) || (pool >= MV_BM_POOLS)) {
		mvOsPrintf("bmPoolId = %d is invalid\n", pool);
		return MV_BAD_PARAM;
	}


	regVal |= cntrShared << MV_BM_POOL_CNTR_SHARED_OFFS;
	regVal |= cntrGrntd << MV_BM_POOL_CNTR_GRNTD_OFFS;

	mvPp2WrReg(MV_BM_POOL_SET_CNTRS_REG(pool), regVal);

	return MV_OK;
}
/******************************************************************************/

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

	mvOsPrintf("  %2d:     %4d       %4d       %4d      0x%08x\n",
						pBmPool->pool, pBmPool->capacity, pBmPool->bufSize, pBmPool->bufNum,
						(unsigned)pBmPool->physAddr);
}

void mvBmStatus(void)
{
	int i;

	mvOsPrintf("BM Pools status\n");
	mvOsPrintf("pool:    capacity    bufSize    bufNum       physAddr\n");
	for (i = 0; i < MV_BM_POOLS; i++)
		mvBmPoolPrint(i);
}
/* PPv2.1 MAS 3.20 new counters */
void mvBmV1PoolDropCntDump(int pool)
{
	mvPp2PrintReg2(MV_BM_V1_PKT_DROP_REG(pool), "MV_BM_V1_PKT_DROP_REG", pool);
	mvPp2PrintReg2(MV_BM_V1_PKT_MC_DROP_REG(pool), "MV_BM_V1_PKT_MC_DROP_REG", pool);
}

void mvBmPoolDump(int pool, int mode)
{
/*	MV_U32     regVal;
	MV_ULONG   *pBufAddr;
	MV_BM_POOL *pBmPool;
	int setReadIdx, getReadIdx, setWriteIdx, getWriteIdx, freeBuffs, i;
*/
	/* validate poolId */
	if ((pool < 0) || (pool >= MV_BM_POOLS)) {
		mvOsPrintf("bmPoolId = %d is invalid \n", pool);
		return;
	}
/*
	pBmPool = &mvBmPools[pool];
	if (pBmPool->pVirt == NULL) {
		mvOsPrintf("bmPool = %d is not created yet\n", pool);
		return;
	}

	mvOsPrintf("\n[NETA BM: pool=%d, mode=%d]\n", pool, mode);

	mvOsPrintf("poolBase=%p (0x%x), capacity=%d, buf_num=%d, buf_size=%d\n",
		   pBmPool->pVirt, (unsigned)pBmPool->physAddr, pBmPool->capacity, pBmPool->bufNum, pBmPool->bufSize);

	regVal = mvPp2RdReg(MV_BM_POOL_READ_PTR_REG(pool));
	setReadIdx = ((regVal & MV_BM_POOL_SET_READ_PTR_MASK) >> MV_BM_POOL_SET_READ_PTR_OFFS) / 4;
	getReadIdx = ((regVal & MV_BM_POOL_GET_READ_PTR_MASK) >> MV_BM_POOL_GET_READ_PTR_OFFS) / 4;

	regVal = mvPp2RdReg(MV_BM_POOL_WRITE_PTR_REG(pool));
	setWriteIdx = ((regVal & MV_BM_POOL_SET_WRITE_PTR_MASK) >> MV_BM_POOL_SET_WRITE_PTR_OFFS) / 4;
	getWriteIdx = ((regVal & MV_BM_POOL_GET_WRITE_PTR_MASK) >> MV_BM_POOL_GET_WRITE_PTR_OFFS) / 4;
	if (getWriteIdx >= getReadIdx)
		freeBuffs = getWriteIdx - getReadIdx;
	else
		freeBuffs = (pBmPool->capacity - getReadIdx) + getWriteIdx;

	mvOsPrintf("nextToRead: set=%d, get=%d, nextToWrite: set=%d, get=%d, freeBuffs=%d\n",
		setReadIdx, getReadIdx, setWriteIdx, getWriteIdx, freeBuffs);

	if (mode > 0) {
*/
		/* Print the content of BM pool */
/*		i = getReadIdx;
		while (i != getWriteIdx) {
			pBufAddr = (MV_ULONG *)pBmPool->pVirt + i;
			mvOsPrintf("%3d. pBufAddr=%p, bufAddr=%08x\n",
				   i, pBufAddr, (MV_U32)(*pBufAddr));
			i++;
			if (i == pBmPool->capacity)
				i = 0;
		}
	}
*/
}

/******************************************************************************/
MV_STATUS mvBmQsetShow(int qset)
{
	MV_BM_QSET *pQset;
	MV_U32 regVal;

	if (qset < 0 || qset >= MV_BM_QSET_MAX) {
		mvOsPrintf("%s: Bad qset number = %d\n", __func__, qset);
		return MV_BAD_PARAM;
	}

	pQset = &mvBmQsets[qset];

	mvPp2WrReg(MV_BM_PRIO_IDX_REG, qset);
	regVal = mvPp2RdReg(MV_BM_QSET_SET_MAX_REG);

	mvOsPrintf("Qset[%03d]: pool=%d,  refCount=%03d,  maxShared=%04d,  maxGrntd=%04d,  MaxBuff reg(0x%x)=0x%08x\n",
			qset, pQset->pool, pQset->refCount, pQset->maxShared,
			pQset->maxGrntd, MV_BM_QSET_SET_MAX_REG, regVal);

	return MV_OK;
}

static MV_BOOL mvBmPriorityEn(void)
{
	return ((mvPp2RdReg(MV_BM_PRIO_CTRL_REG) == 0) ? MV_FALSE : MV_TRUE);
}

void mvBmQsetConfigDumpAll(void)
{
	int qset;

	if (!mvBmPriorityEn())
		mvOsPrintf("Note: The buffers priority algorithms is disabled.\n");

	for (qset = 0; qset < MV_BM_QSET_MAX; qset++) {
		/* skip qsets that not attached to any pool */
		if (mvBmQsets[qset].pool == -1)
			continue;

		mvBmQsetShow(qset);
	}
}

static void mvBmQueueMapDump(int queue)
{
	unsigned int regVal, shortQset, longQset;

	mvOsPrintf("-------- queue #%d --------\n", queue);

	mvPp2WrReg(MV_BM_PRIO_IDX_REG, queue);
	regVal = mvPp2RdReg(MV_BM_CPU_QSET_REG);

	shortQset = ((regVal & (MV_BM_CPU_SHORT_QSET_MASK)) >> MV_BM_CPU_SHORT_QSET_OFFS);
	longQset = ((regVal & (MV_BM_CPU_LONG_QSET_MASK)) >> MV_BM_CPU_LONG_QSET_OFFS);
	mvOsPrintf("CPU SHORT QSET = 0x%02x\n", shortQset);
	mvOsPrintf("CPU LONG QSET  = 0x%02x\n", longQset);

	regVal = mvPp2RdReg(MV_BM_HWF_QSET_REG);
	shortQset = ((regVal & (MV_BM_HWF_SHORT_QSET_MASK)) >> MV_BM_HWF_SHORT_QSET_OFFS);
	longQset = ((regVal & (MV_BM_HWF_LONG_QSET_MASK)) >> MV_BM_HWF_LONG_QSET_OFFS);
	mvOsPrintf("HWF SHORT QSET = 0x%02x\n", shortQset);
	mvOsPrintf("HWF LONG QSET  = 0x%02x\n", longQset);
}

void mvBmQueueMapDumpAll(void)
{
	int queue;

	if (!mvBmPriorityEn())
		mvOsPrintf("Note: The buffers priority algorithms is disabled.\n");

	for (queue = 0; queue < 256 /* TODO MAX(RXQ_NUM, TXQ_NUM)*/; queue++)
		mvBmQueueMapDump(queue);
}

/*
void mvBmPoolConfigDumpAll(void)

	regVal = mvPp2RdReg(MV_BM_POOL_MAX_SHARED_REG(pool));

	maxSherd = ((regVal & MV_BM_POOL_MAX_SHARED_MASK) >> MV_BM_POOL_MAX_SHARED_OFFS);
	mvOsPrintf("POOL MAX SHERD = 0x%04x\n", maxSherd);
	mvOsPrintf("\n");
*/

