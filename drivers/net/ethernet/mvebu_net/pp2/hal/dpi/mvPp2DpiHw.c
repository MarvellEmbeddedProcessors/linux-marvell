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
#include "mvPp2DpiHw.h"

MV_PP2_QUEUE_CTRL *mvPp2DpiReqQ;
MV_PP2_QUEUE_CTRL *mvPp2DpiResQ;


void	mvPp2DpiInit(void)
{
	int i;

	/* Reset all counters */
	mvPp2WrReg(MV_PP2_DPI_INIT_REG, 1);

	/* Clear all counters control  registers */
	for (i = 0; i < MV_PP2_DPI_BYTE_VAL_MAX; i++)
		mvPp2DpiByteConfig(i, 0);

	/* Clear all counters window registers */
	for (i = 0; i < MV_PP2_DPI_CNTRS; i++)
		mvPp2DpiCntrWinSet(i, 0, 0);

	/* Create Request and Result queues */
	mvPp2DpiQueuesCreate(MV_PP2_DPI_Q_SIZE);
}

void	mvPp2DpiRegs(void)
{
	int    i;

	mvOsPrintf("\n[DIP registers: %d counters]\n", MV_PP2_DPI_CNTRS);

	mvPp2PrintReg(MV_PP2_DPI_INIT_REG,	 "MV_PP2_DPI_INIT_REG");
	mvPp2PrintReg(MV_PP2_DPI_REQ_Q_ADDR_REG, "MV_PP2_DPI_REQ_Q_ADDR_REG");
	mvPp2PrintReg(MV_PP2_DPI_RES_Q_ADDR_REG, "MV_PP2_DPI_RES_Q_ADDR_REG");
	mvPp2PrintReg(MV_PP2_DPI_Q_SIZE_REG,     "MV_PP2_DPI_Q_SIZE_REG");
	mvPp2PrintReg(MV_PP2_DPI_Q_STATUS_REG,   "MV_PP2_DPI_Q_STATUS_REG");
	mvPp2PrintReg(MV_PP2_DPI_Q_INDEX_REG,    "MV_PP2_DPI_Q_INDEX_REG");
	mvPp2PrintReg(MV_PP2_DPI_Q_PEND_REG,     "MV_PP2_DPI_Q_PEND_REG");
	mvPp2PrintReg(MV_PP2_DPI_Q_THRESH_REG,   "MV_PP2_DPI_Q_THRESH_REG");

	mvOsPrintf("\nDPI Bytes per counter configuration\n");
	for (i = 0; i < MV_PP2_DPI_BYTE_VAL_MAX; i++) {
		mvPp2WrReg(MV_PP2_DPI_BYTE_VAL_REG, i);
		mvPp2RegPrintNonZero2(MV_PP2_DPI_CNTR_CTRL_REG, "MV_PP2_DPI_CNTR_CTRL_REG", i);
	}

	mvOsPrintf("\nDPI counters window offset and size confuration\n");
	for (i = 0; i < MV_PP2_DPI_CNTRS; i++)
		mvPp2PrintReg2(MV_PP2_DPI_CNTR_WIN_REG(i),   "MV_PP2_DPI_CNTR_WIN_REG", i);
}


MV_STATUS	mvPp2DpiCntrWinSet(int cntr, int offset, int size)
{
	MV_U32 regVal;

	if (mvPp2MaxCheck(cntr, MV_PP2_DPI_CNTRS, "DPI counter"))
		return MV_BAD_PARAM;

	if (mvPp2MaxCheck(offset, (MV_PP2_DPI_WIN_OFFSET_MAX + 1), "DPI win offset"))
		return MV_BAD_PARAM;

	if (mvPp2MaxCheck(size, (MV_PP2_DPI_WIN_SIZE_MAX + 1), "DPI win size"))
		return MV_BAD_PARAM;

	regVal = MV_PP2_DPI_WIN_OFFSET_MASK(offset) | MV_PP2_DPI_WIN_SIZE_MASK(size);
	mvPp2WrReg(MV_PP2_DPI_CNTR_WIN_REG(cntr), regVal);

	return MV_OK;
}

MV_STATUS	mvPp2DpiByteConfig(MV_U8 byte, MV_U16 cntrs_map)
{
	mvPp2WrReg(MV_PP2_DPI_BYTE_VAL_REG, byte);
	mvPp2WrReg(MV_PP2_DPI_CNTR_CTRL_REG, cntrs_map);

	return MV_OK;
}

MV_STATUS	mvPp2DpiCntrByteSet(int cntr, MV_U8 byte, int en)
{
	MV_U32 regVal;

	if (mvPp2MaxCheck(cntr, MV_PP2_DPI_CNTRS, "DPI counter"))
		return MV_BAD_PARAM;

	mvPp2WrReg(MV_PP2_DPI_BYTE_VAL_REG, byte);
	regVal = mvPp2RdReg(MV_PP2_DPI_CNTR_CTRL_REG);

	if (en)
		regVal |= (1 << cntr);
	else
		regVal &= ~(1 << cntr);

	mvPp2WrReg(MV_PP2_DPI_CNTR_CTRL_REG, regVal);

	return MV_OK;
}

MV_STATUS	mvPp2DpiCntrDisable(int cntr)
{
	int i;

	if (mvPp2MaxCheck(cntr, MV_PP2_DPI_CNTRS, "DPI counter"))
		return MV_BAD_PARAM;

	for (i = 0; i < MV_PP2_DPI_BYTE_VAL_MAX; i++)
		mvPp2DpiCntrByteSet(cntr, i, 0);

	return MV_OK;
}

void	mvPp2DpiQueueShow(int mode)
{
	MV_PP2_QUEUE_CTRL *pQueueCtrl;
	int i;

	pQueueCtrl = mvPp2DpiReqQ;
	mvOsPrintf("\n[PPv2 DPI Requests Queue]\n");

	if (pQueueCtrl) {
		mvOsPrintf("nextToProc=%d (%p), PendingRequests=%d, NextRequestIndex=%d\n",
			pQueueCtrl->nextToProc, MV_PP2_QUEUE_DESC_PTR(pQueueCtrl, pQueueCtrl->nextToProc),
			mvPp2DpiReqPendGet(), mvPp2DpiReqNextIdx());

		mvOsPrintf("pFirst=%p (0x%x), descSize=%d, numOfDescr=%d\n",
			pQueueCtrl->pFirst, (MV_U32) pp2DescVirtToPhys(pQueueCtrl, (MV_U8 *) pQueueCtrl->pFirst),
			pQueueCtrl->descSize, pQueueCtrl->lastDesc + 1);

		if (mode > 0) {
			for (i = 0; i <= pQueueCtrl->lastDesc; i++) {
				PP2_DPI_REQ_DESC *pReqDesc = (PP2_DPI_REQ_DESC *) MV_PP2_QUEUE_DESC_PTR(pQueueCtrl, i);

				mvOsPrintf("%3d. pReqDesc=%p, 0x%08x, %d\n",
					i, pReqDesc, pReqDesc->bufPhysAddr, pReqDesc->dataSize);
				mvOsCacheLineInv(NULL, pReqDesc);
			}
		}
	}

	pQueueCtrl = mvPp2DpiResQ;
	mvOsPrintf("\n[PPv2 DPI Results Queue]\n");

	if (pQueueCtrl) {
		mvOsPrintf("nextToProc=%d (%p), PendingResults=%d, NextResultIndex=%d\n",
			pQueueCtrl->nextToProc, MV_PP2_QUEUE_DESC_PTR(pQueueCtrl, pQueueCtrl->nextToProc),
			mvPp2DpiResOccupGet(), mvPp2DpiResNextIdx());

		mvOsPrintf("pFirst=%p (0x%x), descSize=%d, numOfDescr=%d\n",
			pQueueCtrl->pFirst, (MV_U32) pp2DescVirtToPhys(pQueueCtrl, (MV_U8 *) pQueueCtrl->pFirst),
			pQueueCtrl->descSize, pQueueCtrl->lastDesc + 1);

		if (mode > 0) {
			for (i = 0; i <= pQueueCtrl->lastDesc; i++) {
				/* Result Queue */
				PP2_DPI_RES_DESC *pResDesc = (PP2_DPI_RES_DESC *) MV_PP2_QUEUE_DESC_PTR(pQueueCtrl, i);
				int j;

				mvOsPrintf("%3d. pResDesc=%p, ", i, pResDesc);
				for (j = 0; j < MV_PP2_DPI_CNTRS; j++)
					mvOsPrintf("%-2d ", pResDesc->counter[j]);

				mvOsPrintf("\n");
				mvOsCacheLineInv(NULL, pResDesc);
			}
		}
	}
}

MV_STATUS mvPp2DpiQueuesCreate(int num)
{
	MV_PP2_QUEUE_CTRL *pQueueCtrl;
	int size;

	mvPp2WrReg(MV_PP2_DPI_Q_SIZE_REG, num);

	/* Allocate memory for DPI request queue */
	pQueueCtrl = mvOsMalloc(sizeof(MV_PP2_QUEUE_CTRL));
	if (pQueueCtrl == NULL) {
		mvOsPrintf("%s: Can't allocate %d bytes for mvPp2DpiReqQ\n", __func__, sizeof(MV_PP2_QUEUE_CTRL));
		return MV_OUT_OF_CPU_MEM;
	}
	mvOsMemset(pQueueCtrl, 0, sizeof(MV_PP2_QUEUE_CTRL));

	size = (num * sizeof(PP2_DPI_REQ_DESC) + MV_PP2_DPI_Q_ALIGN);
	pQueueCtrl->descBuf.bufVirtPtr =
	    mvPp2DescrMemoryAlloc(size, &pQueueCtrl->descBuf.bufPhysAddr, &pQueueCtrl->descBuf.memHandle);
	pQueueCtrl->descBuf.bufSize = size;
	pQueueCtrl->descSize = sizeof(PP2_DPI_REQ_DESC);

	if (pQueueCtrl->descBuf.bufVirtPtr == NULL) {
		mvOsPrintf("%s: Can't allocate %d bytes for %d descr\n", __func__, size, num);
		return MV_OUT_OF_CPU_MEM;
	}

	/* Make sure descriptor address is aligned */
	pQueueCtrl->pFirst = (char *)MV_ALIGN_UP((MV_ULONG) pQueueCtrl->descBuf.bufVirtPtr, MV_PP2_DPI_Q_ALIGN);
	pQueueCtrl->lastDesc = (num - 1);
	mvPp2WrReg(MV_PP2_DPI_REQ_Q_ADDR_REG, pp2DescVirtToPhys(pQueueCtrl, (MV_U8 *)pQueueCtrl->pFirst));
	mvPp2DpiReqQ = pQueueCtrl;

	/* Allocate memory for DPI result queue */
	pQueueCtrl = mvOsMalloc(sizeof(MV_PP2_QUEUE_CTRL));
	if (pQueueCtrl == NULL) {
		mvOsPrintf("%s: Can't allocate %d bytes for mvPp2DpiResQ\n", __func__, sizeof(MV_PP2_QUEUE_CTRL));
		return MV_OUT_OF_CPU_MEM;
	}
	mvOsMemset(pQueueCtrl, 0, sizeof(MV_PP2_QUEUE_CTRL));

	size = (num * sizeof(PP2_DPI_RES_DESC) + MV_PP2_DPI_Q_ALIGN);
	pQueueCtrl->descBuf.bufVirtPtr =
	    mvPp2DescrMemoryAlloc(size, &pQueueCtrl->descBuf.bufPhysAddr, &pQueueCtrl->descBuf.memHandle);
	pQueueCtrl->descBuf.bufSize = size;
	pQueueCtrl->descSize = sizeof(PP2_DPI_RES_DESC);

	if (pQueueCtrl->descBuf.bufVirtPtr == NULL) {
		mvOsPrintf("%s: Can't allocate %d bytes for %d descr\n", __func__, size, num);
		return MV_OUT_OF_CPU_MEM;
	}
	/* Make sure descriptor address is aligned */
	pQueueCtrl->pFirst = (char *)MV_ALIGN_UP((MV_ULONG) pQueueCtrl->descBuf.bufVirtPtr, MV_PP2_DPI_Q_ALIGN);
	pQueueCtrl->lastDesc = (num - 1);
	mvPp2WrReg(MV_PP2_DPI_RES_Q_ADDR_REG, pp2DescVirtToPhys(pQueueCtrl, (MV_U8 *)pQueueCtrl->pFirst));
	mvPp2DpiResQ = pQueueCtrl;

	return MV_OK;
}

MV_STATUS mvPp2DpiQueuesDelete(void)
{
	if (mvPp2DpiReqQ) {
		mvPp2DescrMemoryFree(mvPp2DpiReqQ->descBuf.bufSize, (MV_ULONG *)mvPp2DpiReqQ->descBuf.bufPhysAddr,
				mvPp2DpiReqQ->descBuf.bufVirtPtr, (MV_U32 *)mvPp2DpiReqQ->descBuf.memHandle);
		mvOsFree(mvPp2DpiReqQ);
		mvPp2DpiReqQ = NULL;
	} else
		mvOsPrintf("%s: DPI Request queue is not initialized\n", __func__);


	if (mvPp2DpiResQ) {
		mvPp2DescrMemoryFree(mvPp2DpiResQ->descBuf.bufSize, (MV_ULONG *)mvPp2DpiResQ->descBuf.bufPhysAddr,
				mvPp2DpiResQ->descBuf.bufVirtPtr, (MV_U32 *)mvPp2DpiResQ->descBuf.memHandle);
		mvOsFree(mvPp2DpiResQ);
		mvPp2DpiResQ = NULL;
	} else
		mvOsPrintf("%s: DPI Result queue is not initialized\n", __func__);

	/* Reset all counters */
	mvPp2WrReg(MV_PP2_DPI_INIT_REG, 1);

	return MV_OK;
}

MV_STATUS mvPp2DpiRequestSet(unsigned long paddr, int size)
{
	MV_PP2_QUEUE_CTRL *pQueueCtrl = mvPp2DpiReqQ;
	PP2_DPI_REQ_DESC  *pReqDesc;
	int reqDesc = pQueueCtrl->nextToProc;

	if (mvPp2DpiReqQ == NULL) {
		mvOsPrintf("%s: DPI Request queue is not initialized\n", __func__);
		return MV_NOT_READY;
	}
	/* Check if request queue is not Full */
	if (mvPp2DpiReqIsFull(pQueueCtrl))
		return MV_FULL;

	pReqDesc = (PP2_DPI_REQ_DESC *)MV_PP2_QUEUE_DESC_PTR(pQueueCtrl, reqDesc);
	pReqDesc->bufPhysAddr = paddr;
	pReqDesc->dataSize = size;

	pQueueCtrl->nextToProc = MV_PP2_QUEUE_NEXT_DESC(pQueueCtrl, reqDesc);

	return MV_OK;
}

MV_STATUS mvPp2DpiResultGet(MV_U8 *counters, int num)
{
	MV_PP2_QUEUE_CTRL *pQueueCtrl = mvPp2DpiResQ;
	PP2_DPI_RES_DESC  *pResDesc;
	int resDesc = pQueueCtrl->nextToProc;

	if (mvPp2DpiResQ == NULL) {
		mvOsPrintf("%s: DPI Result queue is not initialized\n", __func__);
		return MV_NOT_READY;
	}
	if (num > MV_PP2_DPI_CNTRS) {
		mvOsPrintf("%s: Number of DPI counters %d is out of maximium %d\n",
				__func__, num, MV_PP2_DPI_CNTRS);
		num = MV_PP2_DPI_CNTRS;
	}

	pResDesc = (PP2_DPI_RES_DESC *)MV_PP2_QUEUE_DESC_PTR(pQueueCtrl, resDesc);
	pQueueCtrl->nextToProc = MV_PP2_QUEUE_NEXT_DESC(pQueueCtrl, resDesc);
	if (counters)
		memcpy(counters, pResDesc->counter, num);

	return MV_OK;
}

