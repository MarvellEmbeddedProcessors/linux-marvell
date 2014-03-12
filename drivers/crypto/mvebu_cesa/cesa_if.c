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

#include "mvCommon.h"
#include "mvOs.h"
#include "cesa_if.h"
#include <linux/interrupt.h>
#include <linux/spinlock.h>
#include <linux/spinlock_types.h>


#define MV_CESA_IF_MAX_WEIGHT	0xFFFFFFFF

/* Globals */
static MV_CESA_RESULT **pResQ;
static MV_CESA_RESULT *resQ;
static MV_CESA_RESULT *pEmptyResult;
static MV_CESA_FLOW_TYPE flowType[MV_CESA_CHANNELS];
static MV_U32 chanWeight[MV_CESA_CHANNELS];
static MV_STATUS isReady[MV_CESA_CHANNELS];
static MV_CESA_POLICY cesaPolicy;
static MV_U8 splitChanId;
static MV_U32 resQueueDepth;
static MV_U32 reqId;
static MV_U32 resId;
static spinlock_t chanLock[MV_CESA_CHANNELS];
static DEFINE_SPINLOCK(cesaIfLock);
static DEFINE_SPINLOCK(cesaIsrLock);

MV_STATUS mvCesaIfInit(int numOfSession, int queueDepth, void *osHandle, MV_CESA_HAL_DATA *halData)
{
	MV_U8 chan = 0;

	/* Init parameters */
	reqId = 0;
	resId = 0;
	resQueueDepth = (MV_CESA_CHANNELS * queueDepth);

#if (CONFIG_MV_CESA_CHANNELS == 1)
	cesaPolicy = CESA_SINGLE_CHAN_POLICY;
#else
	cesaPolicy = CESA_DUAL_CHAN_BALANCED_POLICY;
#endif

	/* Allocate reordered queue for completed results */
	resQ = (MV_CESA_RESULT *)mvOsMalloc(resQueueDepth * sizeof(MV_CESA_RESULT));
	if (resQ == NULL) {
		mvOsPrintf("%s: Error, resQ malloc failed\n", __func__);
		return MV_ERROR;
	}
	pEmptyResult = &resQ[0];

	/* Allocate result pointers queue */
	pResQ = (MV_CESA_RESULT **)mvOsMalloc(resQueueDepth * sizeof(MV_CESA_RESULT*));
	if (pResQ == NULL) {
		mvOsPrintf("%s: Error, pResQ malloc failed\n", __func__);
		return MV_ERROR;
	}

	/* Init shared spinlocks */
	spin_lock_init(&cesaIfLock);
	spin_lock_init(&cesaIsrLock);

	/* Per channel init */
	for (chan = 0; chan < MV_CESA_CHANNELS; chan++) {
		spin_lock_init(&chanLock[chan]);
		chanWeight[chan] = 0;
		flowType[chan] = 0;
		isReady[chan] = MV_TRUE;
	}

	/* Clear global resources */
	memset(pResQ, 0, (resQueueDepth * sizeof(MV_CESA_RESULT *)));
	memset(resQ, 0, (resQueueDepth * sizeof(MV_CESA_RESULT)));

	return mvCesaHalInit(numOfSession, queueDepth, osHandle, halData);
}

MV_STATUS mvCesaIfAction(MV_CESA_COMMAND *pCmd)
{
	MV_U8 chan = 0, chanId = 0xff;
	MV_U32 min = MV_CESA_IF_MAX_WEIGHT; /* max possible value */
	MV_STATUS status;
	MV_ULONG flags = 0;

	/* Handle request according to selected policy */
	switch (cesaPolicy) {
	case CESA_WEIGHTED_CHAN_POLICY:
	case CESA_NULL_POLICY:
		for (chan = 0; chan < MV_CESA_CHANNELS; chan++) {
			if ((cesaReqResources[chan] > 0) && (chanWeight[chan] < min)) {
				min = chanWeight[chan];
				chanId = chan;
			}
		}

		/* Any room for the request ? */
		if (cesaReqResources[chanId] <= 1)
			return MV_NO_RESOURCE;

		spin_lock_irqsave(&chanLock[chanId], flags);
		chanWeight[chanId] += pCmd->pSrc->mbufSize;
		spin_unlock_irqrestore(&chanLock[chanId], flags);
		break;

	case CESA_DUAL_CHAN_BALANCED_POLICY:
		spin_lock(&cesaIfLock);
		chanId = (reqId % 2);
		spin_unlock(&cesaIfLock);

		/* Any room for the request ? */
		if (cesaReqResources[chanId] <= 1)
			return MV_NO_RESOURCE;

		break;

	case CESA_FLOW_ASSOC_CHAN_POLICY:
		for (chan = 0; chan < MV_CESA_CHANNELS; chan++) {
			if (flowType[chan] == pCmd->flowType) {
				chanId = chan;
				break;
			}
		}

		if(chanId == 0xff) {
			mvOsPrintf("%s: Error, policy was not set correctly\n", __func__);
			return MV_ERROR;
		}

		break;

	case CESA_SINGLE_CHAN_POLICY:
		spin_lock(&cesaIfLock);
		chanId = 0;
		spin_unlock(&cesaIfLock);

		/* Any room for the request ? */
		if (cesaReqResources[chanId] <= 1)
			return MV_NO_RESOURCE;

		break;

	default:
		mvOsPrintf("%s: Error, policy not supported\n", __func__);
		return MV_ERROR;
	}

	/* Check if we need to handle split packet */
	if (pCmd->split != MV_CESA_SPLIT_NONE) {
		if (pCmd->split == MV_CESA_SPLIT_FIRST) {
			spin_lock(&cesaIfLock);
			splitChanId = chanId;
			spin_unlock(&cesaIfLock);
		} else	/* MV_CESA_SPLIT_SECOND */
			chanId = splitChanId;
	}

	/* Update current request id then increment */
	spin_lock(&cesaIfLock);
	pCmd->reqId = reqId;
	spin_unlock(&cesaIfLock);

	/* Inject request to CESA driver */
	spin_lock_irqsave(&chanLock[chanId], flags);
	status = mvCesaAction(chanId, pCmd);

	/* Check status */
	if ((status == MV_OK) || (status == MV_NO_MORE))
		reqId = ((reqId + 1) % resQueueDepth);

	spin_unlock_irqrestore(&chanLock[chanId], flags);

	return status;
}

MV_STATUS mvCesaIfReadyGet(MV_U8 chan, MV_CESA_RESULT *pResult)
{
	MV_STATUS status;
	MV_CESA_RESULT *pCurrResult;
	MV_ULONG flags;

	/* Validate channel index */
	if (chan >= MV_CESA_CHANNELS) {
		printk("%s: Error, bad channel index(%d)\n", __func__, chan);
		return MV_ERROR;
	}

	/* Prevent pushing requests till finish to extract pending requests */
	spin_lock_irqsave(&chanLock[chan], flags);

	/* Are there any pending requests in CESA driver ? */
	if (isReady[chan] == MV_FALSE)
		goto out;

	while (1) {

		spin_lock(&cesaIsrLock);
		pCurrResult = pEmptyResult;
		spin_unlock(&cesaIsrLock);

		/* Get next result */
		status = mvCesaReadyGet(chan, pCurrResult);

		if (status != MV_OK)
			break;

		spin_lock(&cesaIsrLock);
		pEmptyResult = ((pEmptyResult != &resQ[resQueueDepth-1]) ? (pEmptyResult + 1) : &resQ[0]);
		spin_unlock(&cesaIsrLock);

		/* Handle request according to selected policy */
		switch (cesaPolicy) {
		case CESA_WEIGHTED_CHAN_POLICY:
		case CESA_NULL_POLICY:
			chanWeight[chan] -= pCurrResult->mbufSize;
			break;

		case CESA_FLOW_ASSOC_CHAN_POLICY:
			/* TBD - handle policy */
			break;

		case CESA_DUAL_CHAN_BALANCED_POLICY:
		case CESA_SINGLE_CHAN_POLICY:
			break;

		default:
			mvOsPrintf("%s: Error, policy not supported\n", __func__);
			return MV_ERROR;
		}


		if (pResQ[pCurrResult->reqId] != NULL)
			mvOsPrintf("%s: Warning, result entry not empty(reqId=%d, chan=%d, resId=%d)\n", __func__, pCurrResult->reqId, chan, resId);

		/* Save current result */
		spin_lock(&cesaIsrLock);
		pResQ[pCurrResult->reqId] = pCurrResult;
		spin_unlock(&cesaIsrLock);

#ifdef CONFIG_MV_CESA_INT_PER_PACKET
		break;
#endif
	}

out:
	spin_lock(&cesaIsrLock);

	if (pResQ[resId] == NULL) {
		isReady[chan] = MV_TRUE;
		status = MV_NOT_READY;
	} else {
		/* Send results in order */
		isReady[chan] = MV_FALSE;
		/* Fill result data */
		pResult->retCode = pResQ[resId]->retCode;
		pResult->pReqPrv = pResQ[resId]->pReqPrv;
		pResult->sessionId = pResQ[resId]->sessionId;
		pResult->mbufSize = pResQ[resId]->mbufSize;
		pResult->reqId = pResQ[resId]->reqId;
		pResQ[resId] = NULL;
		resId = ((resId + 1) % resQueueDepth);
		status = MV_OK;
	}

	spin_unlock(&cesaIsrLock);

	/* Release per channel lock */
	spin_unlock_irqrestore(&chanLock[chan], flags);

	return status;
}

MV_STATUS mvCesaIfPolicySet(MV_CESA_POLICY policy, MV_CESA_FLOW_TYPE flow)
{
	MV_U8 chan = 0;

	spin_lock(&cesaIfLock);

	if (cesaPolicy == CESA_NULL_POLICY) {
		cesaPolicy = policy;
	} else {
		/* Check if more than 1 policy was assigned */
		if (cesaPolicy != policy) {
			spin_unlock(&cesaIfLock);
			mvOsPrintf("%s: Error, can not support multiple policies\n", __func__);
			return MV_ERROR;
		}
	}

	if (policy == CESA_FLOW_ASSOC_CHAN_POLICY) {

		if (flow == CESA_NULL_FLOW_TYPE) {
			spin_unlock(&cesaIfLock);
			mvOsPrintf("%s: Error, bad policy configuration\n", __func__);
			return MV_ERROR;
		}

		/* Find next empty entry */
		for (chan = 0; chan < MV_CESA_CHANNELS; chan++) {
			if (flowType[chan] == CESA_NULL_FLOW_TYPE)
				flowType[chan] = flow;
		}

		if (chan == MV_CESA_CHANNELS) {
			spin_unlock(&cesaIfLock);
			mvOsPrintf("%s: Error, no empty entry is available\n", __func__);
			return MV_ERROR;
		}

	}

	spin_unlock(&cesaIfLock);

	return MV_OK;
}

MV_STATUS mvCesaIfPolicyGet(MV_CESA_POLICY *pCesaPolicy)
{
	*pCesaPolicy = cesaPolicy;

	return MV_OK;
}

MV_STATUS mvCesaIfFinish(void)
{
	/* Free global resources */
	mvOsFree(pResQ);
	mvOsFree(resQ);

	return mvCesaFinish();
}

MV_STATUS mvCesaIfSessionOpen(MV_CESA_OPEN_SESSION *pSession, short *pSid)
{
	return mvCesaSessionOpen(pSession, pSid);
}

MV_STATUS mvCesaIfSessionClose(short sid)
{
	return mvCesaSessionClose(sid);
}

MV_VOID mvCesaIfDebugMbuf(const char *str, MV_CESA_MBUF *pMbuf, int offset, int size)
{
	return mvCesaDebugMbuf(str, pMbuf, offset, size);
}

void mv_bin_to_hex(const MV_U8 *bin, char *hexStr, int size)
{
	int i;

	for (i = 0; i < size; i++)
		mvOsSPrintf(&hexStr[i * 2], "%02x", bin[i]);

	hexStr[i * 2] = '\0';
}

/*******************************************************************************
* mv_hex_to_bin - Convert hex to binary
*
* DESCRIPTION:
*		This function Convert hex to binary.
*
* INPUT:
*       pHexStr - hex buffer pointer.
*       size    - Size to convert.
*
* OUTPUT:
*       pBin - Binary buffer pointer.
*
* RETURN:
*       None.
*
*******************************************************************************/
MV_VOID mv_hex_to_bin(const char *pHexStr, MV_U8 *pBin, int size)
{
	int j, i;
	char tmp[3];
	MV_U8 byte;

	for (j = 0, i = 0; j < size; j++, i += 2) {
		tmp[0] = pHexStr[i];
		tmp[1] = pHexStr[i + 1];
		tmp[2] = '\0';
		byte = (MV_U8) (strtol(tmp, NULL, 16) & 0xFF);
		pBin[j] = byte;
	}
}

/* Dump memory in specific format:
 * address: X1X1X1X1 X2X2X2X2 ... X8X8X8X8
 */
void mv_debug_mem_dump(void *addr, int size, int access)
{
	int i, j;
	MV_U32 memAddr = (MV_U32) addr;

	if (access == 0)
		access = 1;

	if ((access != 4) && (access != 2) && (access != 1)) {
		mvOsPrintf("%d wrong access size. Access must be 1 or 2 or 4\n", access);
		return;
	}
	memAddr = MV_ALIGN_DOWN((unsigned int)addr, 4);
	size = MV_ALIGN_UP(size, 4);
	addr = (void *)MV_ALIGN_DOWN((unsigned int)addr, access);
	while (size > 0) {
		mvOsPrintf("%08x: ", memAddr);
		i = 0;
		/* 32 bytes in the line */
		while (i < 32) {
			if (memAddr >= (MV_U32) addr) {
				switch (access) {
				case 1:
					mvOsPrintf("%02x ", MV_MEMIO8_READ(memAddr));
					break;

				case 2:
					mvOsPrintf("%04x ", MV_MEMIO16_READ(memAddr));
					break;

				case 4:
					mvOsPrintf("%08x ", MV_MEMIO32_READ(memAddr));
					break;
				}
			} else {
				for (j = 0; j < (access * 2 + 1); j++)
					mvOsPrintf(" ");
			}
			i += access;
			memAddr += access;
			size -= access;
			if (size <= 0)
				break;
		}
		mvOsPrintf("\n");
	}
}
