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

#include "mvTypes.h"
#include "ctrlEnv/mvCtrlEnvLib.h"
#include "ctrlEnv/sys/mvCpuIf.h"
#include "cpu/mvCpu.h"
#include "mvIpc.h"
#include "mvOs.h"

#include "mv_ipc_common.h"

//#define MV_IPC_DEBUG
#ifdef MV_IPC_DEBUG
#define mvIpcDbgPrintf mvOsPrintf
#else
#define mvIpcDbgPrintf(x ...)
#endif

#define mvIpcDbgWrite(x, y)  (x = y);
/*#define mvIpcDbgWrite(x, y)*/

#define mvIpcErrPrintf mvOsPrintf

/*main data structure - links array*/
MV_IPC_LINK mv_ipc_links[MV_IPC_LINKS_NUM];

/***********************************************************************************
 * mvIpcChannelsOffsetsFix
 *
 * DESCRIPTION:
 *		This add base address to all addresses in link and channel structures
 *
 * INPUT:
 *		link  - Link structure to be fixed
 *		base  - Base address
 * OUTPUT:
 *       None
 * RETURN:
 *		status
 *
 ************************************************************************************/
static MV_VOID mvIpcChannelsOffsetsFix(MV_IPC_LINK *link, MV_U32 base)
{
	int chnIdx;

	/*Fixup all offsets to shmem to local addresses*/
	for (chnIdx = 0; chnIdx < link->numOfChannels; chnIdx++) {
		link->channels[chnIdx].rxMsgQueVa =
			(MV_IPC_MSG *)(base + (MV_U32)link->channels[chnIdx].rxMsgQueVa);
		link->channels[chnIdx].txMsgQueVa =
			(MV_IPC_MSG *)(base + (MV_U32)link->channels[chnIdx].txMsgQueVa);

		link->channels[chnIdx].rxCtrlMsg    = &link->channels[chnIdx].rxMsgQueVa[0];
		link->channels[chnIdx].txCtrlMsg    = &link->channels[chnIdx].txMsgQueVa[0];

		link->channels[chnIdx].txMessageFlag += base;
		link->channels[chnIdx].rxMessageFlag += base;
	}

	link->txSharedHeapAddr += base;
	link->rxSharedHeapAddr += base;
}

/***********************************************************************************
 * mvIpcSlaveConfig
 *
 * DESCRIPTION:
 *		This routine read configuration from shared memory and fill local
 *		structires with data configured by Master.
 *		Can be called from mvIpcInit or postponed by and called from mvIpcOpenChannel
 *
 * INPUT:
 *		linkId  - Link id to be configred
 * OUTPUT:
 *       None
 * RETURN:
 *		status
 *
 ************************************************************************************/
static MV_STATUS mvIpcSlaveConfig(MV_U32 linkId)
{
	MV_U32 chnIdx;
	MV_IPC_LINK *link;
	MV_U32 tempAddr;
	MV_IPC_MSG *tempQueVa;

	/* Verify parameters */
	if (linkId > MV_IPC_LINKS_NUM) {
		mvIpcErrPrintf("IPC ERROR: IPC Init: Bad link id %d\n", linkId);
		return MV_FALSE;
	}

	link = &mv_ipc_links[linkId];
	/*Read link structure from shared mem*/
	mvOsMemcpy(link, mvIpcGetShmemAddr(linkId), sizeof(MV_IPC_LINK));

	/*Override local paramters for link*/
	link->nodeId            = mvIpcWhoAmI();
	link->shmemBaseAddr = (MV_U32)mvIpcGetShmemAddr(linkId);
	link->remoteNodeId      = mvIpcGetlinkRemoteNodeId(linkId);
	link->channels = mvOsMalloc(sizeof(MV_IPC_CHANNEL) * link->numOfChannels);

	/*Swap rx and tx fields for Heap region partition*/
	tempAddr = link->txSharedHeapAddr;
	link->txSharedHeapAddr = link->rxSharedHeapAddr;
	link->rxSharedHeapAddr = tempAddr;
	tempAddr = link->txSharedHeapSize;
	link->txSharedHeapSize = link->rxSharedHeapSize;
	link->rxSharedHeapSize = tempAddr;

	/* Initialize all channels */
	for (chnIdx = 0; chnIdx < link->numOfChannels; chnIdx++) {
		/*Read channel structure from shared mem*/
		mvOsMemcpy(&link->channels[chnIdx],
			   (MV_VOID *)(link->shmemBaseAddr + sizeof(MV_IPC_LINK) + (chnIdx * sizeof(MV_IPC_CHANNEL))),
			   sizeof(MV_IPC_CHANNEL));

		link->channels[chnIdx].state        = MV_CHN_CLOSED;
		link->channels[chnIdx].txEnable     = MV_FALSE;
		link->channels[chnIdx].rxEnable     = MV_FALSE;
		link->channels[chnIdx].nextRxMsgIdx = 1;
		link->channels[chnIdx].nextTxMsgIdx = 1;

		/*Swap RX and TX queue start */
		tempQueVa = link->channels[chnIdx].rxMsgQueVa;
		link->channels[chnIdx].rxMsgQueVa = link->channels[chnIdx].txMsgQueVa;
		link->channels[chnIdx].txMsgQueVa   = tempQueVa;

		mvIpcDbgPrintf("IPC HAL: Init channel %d with RxQ = 0x%08x; TxQ = 0x%08x\n",
			       chnIdx, (unsigned int)link->channels[chnIdx].rxMsgQueVa,
			       (unsigned int)link->channels[chnIdx].txMsgQueVa);

		/*Set rx and tx functions*/
		link->channels[chnIdx].sendTrigger = mvIpcGetChnTxHwPtr(linkId);
		link->channels[chnIdx].registerChnInISR = mvIpcGetChnRxHwPtr(linkId);

		tempAddr = link->channels[chnIdx].txMessageFlag;
		link->channels[chnIdx].txMessageFlag = link->channels[chnIdx].rxMessageFlag;
		link->channels[chnIdx].rxMessageFlag = tempAddr;
	}

	/*Fixup all offsets to shmem to local addresses*/
	mvIpcChannelsOffsetsFix(link, link->shmemBaseAddr);

	return MV_OK;
}

/***********************************************************************************
 * mvIpcLinkStart
 *
 * DESCRIPTION:
 *		Initializes the IPC mechanism. reset all queues and sets global variables
 *
 * INPUT:
 *		linkId  - Link id to be configred
 * OUTPUT:
 *       None
 * RETURN:
 *		MV_OK or MV_ERROR
 *
 ************************************************************************************/
MV_STATUS mvIpcLinkStart(MV_U32 linkId)
{
	MV_U32 chnIdx;
	MV_IPC_LINK             *link;
	/*runningOffset is offset in shared memory,
	used to compute addreses of queues and heap*/
	MV_U32 runningOffset = 0, flagsOffset;
	MV_U32 heapSize;

	/* Verify parameters */
	if (linkId > MV_IPC_LINKS_NUM) {
		mvIpcErrPrintf("IPC ERROR: IPC Init: Bad link id %d\n", linkId);
		return MV_FALSE;
	}

	link = &mv_ipc_links[linkId];

	if (MV_TRUE == mvIpcGetlinkMaster(linkId)) {
		/*master configuration*/

		link->nodeId            = mvIpcWhoAmI();
		link->shmemBaseAddr =   (MV_U32)mvIpcGetShmemAddr(linkId);
		link->shmemSize =               (MV_U32)mvIpcGetShmemSize(linkId);
		link->numOfChannels     = mvIpcChnNum(linkId);
		link->remoteNodeId      = mvIpcGetlinkRemoteNodeId(linkId);
		link->channels = mvOsMalloc(sizeof(MV_IPC_CHANNEL) * link->numOfChannels);

		/*Skip the control structures in Shared mem*/
		/*Note: all pointers to shmem will be offsets,
		after controle structures will be copied ti shmem, them will be fixed to addresses*/
		runningOffset += sizeof(MV_IPC_LINK);
		runningOffset += sizeof(MV_IPC_CHANNEL) * link->numOfChannels;
		/*Skip the RX/TX flags in Shared mem*/
		flagsOffset = runningOffset;
		runningOffset += 2 * sizeof(MV_U32) * link->numOfChannels;

		/* Initialize all channels */
		for (chnIdx = 0; chnIdx < link->numOfChannels; chnIdx++) {
			link->channels[chnIdx].state        = MV_CHN_CLOSED;
			link->channels[chnIdx].txEnable     = MV_FALSE;
			link->channels[chnIdx].rxEnable     = MV_FALSE;
			link->channels[chnIdx].queSizeInMsg = mvIpcGetChnQueueSize(linkId, chnIdx);
			link->channels[chnIdx].nextRxMsgIdx = 1;
			link->channels[chnIdx].nextTxMsgIdx = 1;

			/*set RX queue start move offset to queue size * message size*/
			link->channels[chnIdx].rxMsgQueVa   = (MV_IPC_MSG *)runningOffset;
			runningOffset += link->channels[chnIdx].queSizeInMsg * sizeof(MV_IPC_MSG);

			/*set TX queue start move offset to queue size * message size*/
			link->channels[chnIdx].txMsgQueVa   = (MV_IPC_MSG *)runningOffset;
			runningOffset += link->channels[chnIdx].queSizeInMsg * sizeof(MV_IPC_MSG);

			mvOsMemset((MV_VOID *)(link->shmemBaseAddr + (MV_U32)link->channels[chnIdx].rxMsgQueVa), 0,
				   link->channels[chnIdx].queSizeInMsg * sizeof(MV_IPC_MSG));
			mvOsMemset((MV_VOID *)(link->shmemBaseAddr + (MV_U32)link->channels[chnIdx].txMsgQueVa), 0,
				   link->channels[chnIdx].queSizeInMsg * sizeof(MV_IPC_MSG));

			mvIpcDbgPrintf("IPC HAL: Init channel %d with RxQ = 0x%08x; TxQ = 0x%08x\n",
				       chnIdx, (unsigned int)link->channels[chnIdx].rxMsgQueVa,
				       (unsigned int)link->channels[chnIdx].txMsgQueVa);

			/*Set rx and tx functions*/
			link->channels[chnIdx].sendTrigger = mvIpcGetChnTxHwPtr(linkId);
			link->channels[chnIdx].registerChnInISR = mvIpcGetChnRxHwPtr(linkId);

			link->channels[chnIdx].txMessageFlag = flagsOffset + 2 * chnIdx * sizeof(MV_U32);
			link->channels[chnIdx].rxMessageFlag = flagsOffset + (2 * chnIdx + 1) * sizeof(MV_U32);
		}

		/*Check if we have enouth shared memory for all channels*/
		if (runningOffset > mvIpcGetShmemSize(linkId)) {
			mvIpcDbgPrintf("IPC HAL: Init channels allocated 0x%X bytes, shmem is 0x%X bytes\n",
				       runningOffset, mvIpcGetShmemSize(linkId));

			return MV_FAIL;
		}

		/*Heap region partition*/
		heapSize = mvIpcGetShmemSize(linkId) - runningOffset;
		link->txSharedHeapAddr = runningOffset;
		link->txSharedHeapSize = (heapSize * mvIpcGetFreeMemMasterPercent(linkId))/100;
		runningOffset += link->txSharedHeapSize;
		link->rxSharedHeapAddr = runningOffset;
		link->rxSharedHeapSize = mvIpcGetShmemSize(linkId) - runningOffset;

		/*Link and channel structures ready, copy channels first to shared mem*/
		runningOffset = sizeof(MV_IPC_LINK);
		for (chnIdx = 0; chnIdx < link->numOfChannels; chnIdx++) {
			mvOsMemcpy((MV_VOID *)(link->shmemBaseAddr + runningOffset),
				   &link->channels[chnIdx], sizeof(MV_IPC_CHANNEL));
			runningOffset += sizeof(MV_IPC_CHANNEL);
		}

		/*Set magic value in link structures and copy to shared memory,
		this is ready state for client */
		link->masterConfigDone = MV_IPC_MASTER_CONFIG_MAGIC;
		mvOsMemcpy((MV_VOID *)link->shmemBaseAddr, link, sizeof(MV_IPC_LINK));

		/*Fixup all offsets to shmem to local addresses*/
		mvIpcChannelsOffsetsFix(link, link->shmemBaseAddr);

		mvIpcDbgPrintf("IPC HAL: Initialized interface as Master\n");
	} else {
		/*Slave configuration*/

		/*Read link structure from shared mem*/
		mvOsMemcpy((MV_VOID *)link, mvIpcGetShmemAddr(linkId), sizeof(MV_IPC_LINK));
		if (link->masterConfigDone == MV_IPC_MASTER_CONFIG_MAGIC) {
			/*Master finished the init, Slave get the configuration*/
			mvIpcSlaveConfig(linkId);

			/*Clear magic*/
			link->masterConfigDone = 0;
			mvOsMemcpy((MV_VOID *)link->shmemBaseAddr, link, sizeof(MV_IPC_LINK));
			link->slaveLinkInitialized = 0;

			mvIpcDbgPrintf("IPC HAL: Initialized interface as Slave\n");
		} else {
			/*postpone the Slave init, will be done in mvIpcOpenChannel*/
			link->slaveLinkInitialized = MV_IPC_MASTER_CONFIG_MAGIC;
			mvIpcDbgPrintf("IPC HAL: Initialized interface as Slave, config postponed\n");
		}
	}

	return MV_OK;
}

/***********************************************************************************
 * mvIpcClose
 *
 * DESCRIPTION:
 *		Closes all IPC channels
 *
 * INPUT:
 *		linkId  - Link id to be configred
 * OUTPUT:
 *       None
 * RETURN:
 *		MV_OK or MV_ERROR
 *
 ************************************************************************************/
MV_STATUS mvIpcClose(MV_U32 linkId)
{
	MV_IPC_LINK             *link;
	MV_U32 chnIdx;

	/* Verify parameters */
	if (linkId > MV_IPC_LINKS_NUM) {
		mvIpcErrPrintf("IPC ERROR: IPC close: Bad link id %d\n", linkId);
		return MV_FALSE;
	}

	link = &mv_ipc_links[linkId];

	/* De-activate all channels */
	for (chnIdx = 0; chnIdx < link->numOfChannels; chnIdx++) {
		if (link->channels[chnIdx].state == MV_CHN_ATTACHED)
			mvIpcDettachChannel(linkId, chnIdx);

		if (link->channels[chnIdx].state == MV_CHN_OPEN)
			mvIpcCloseChannel(linkId, chnIdx);
	}

	mvIpcDbgPrintf("IPC HAL: CLosed IPC interface\n");

	return MV_OK;
}

/***********************************************************************************
 * mvIpcOpenChannel
 *
 * DESCRIPTION:
 *		Opens a ipc channel and prepares it for receiving messages
 *
 * INPUT:
 *		linkId  - Link id to open
 *		chnId - the channel ID to open
 * OUTPUT:
 *       None
 * RETURN:
 *		MV_OK or MV_ERROR
 *		MV_NOT_STARTED if slave and master still not wake.
 *
 ************************************************************************************/
MV_STATUS mvIpcOpenChannel(MV_U32 linkId, MV_U32 chnId, MV_IPC_RX_CLBK rx_clbk)
{
	MV_IPC_LINK             *link;
	MV_IPC_CHANNEL  *chn;
	MV_U32 msgId;
	MV_STATUS status;

	/* Verify parameters */
	if (linkId > MV_IPC_LINKS_NUM) {
		mvIpcErrPrintf("IPC ERROR: Open Chn: Bad link id %d\n", chnId);
		return MV_FALSE;
	}

	link = &mv_ipc_links[linkId];

	/*Check if posponed Slave init needed*/
	if (link->slaveLinkInitialized == MV_IPC_MASTER_CONFIG_MAGIC) {
		/*Read link structure from shared mem*/
		mvOsMemcpy((MV_VOID *)link, mvIpcGetShmemAddr(linkId), sizeof(MV_IPC_LINK));
		if (link->masterConfigDone == MV_IPC_MASTER_CONFIG_MAGIC) {
			/*Master finished the init, Slave get the configuration*/
			status = mvIpcSlaveConfig(linkId);
			mvIpcErrPrintf("IPC MESSG: Open Chn:Postponed init done with status %d\n", status);

			/*Clear magic*/
			link->masterConfigDone = 0;
			mvOsMemcpy((MV_VOID *)link->shmemBaseAddr, link, sizeof(MV_IPC_LINK));
			link->slaveLinkInitialized = 0;
		} else {
			/*Master still not wake, cannot open the channel*/
			mvIpcErrPrintf("IPC WARNG: Open Chn: Master not ready\n");
			link->slaveLinkInitialized = MV_IPC_MASTER_CONFIG_MAGIC;
			return MV_NOT_STARTED;
		}
	}

	if (chnId > link->numOfChannels) {
		mvIpcErrPrintf("IPC ERROR: Open Chn: Bad channel id %d\n", chnId);
		return MV_FALSE;
	}

	chn = &link->channels[chnId];

	if (chn->state != MV_CHN_CLOSED) {
		mvIpcErrPrintf("IPC ERROR: Can't open channel %d. It is already open %d\n",
			       chnId, chn->state);
		return MV_ERROR;
	}

	/* Initialize the transmit queue */
	for (msgId = 0; msgId < chn->queSizeInMsg; msgId++)
		chn->txMsgQueVa[msgId].isUsed = MV_FALSE;

	/* Initialize channel members */
	chn->state                = MV_CHN_OPEN;
	chn->nextRxMsgIdx = 1;
	chn->nextTxMsgIdx = 1;
	chn->rxEnable     = MV_TRUE;
	chn->rxCallback   = rx_clbk;

	mvIpcDbgPrintf("IPC HAL: Opened channel %d successfully\n", chnId);

	return MV_OK;
}

/***********************************************************************************
 * mvIpcAckAttach
 *
 * DESCRIPTION:
 *		Acknowledges and Attach request from receiver.
 *
 * INPUT:
 *		linkId  - the link ID
 *		chnId - the channel ID
 *		cpuId - the CPU ID to attach to
 *		acknowledge - do i need to acknowledge the message
 * OUTPUT:
 *       None
 * RETURN:
 *		MV_OK or MV_ERROR
 *
 ************************************************************************************/
static MV_STATUS mvIpcAckAttach(MV_U32 linkId, MV_U32 chnId, MV_BOOL acknowledge)
{
	MV_IPC_LINK             *link;
	MV_IPC_CHANNEL  *chn;
	MV_IPC_MSG attachMsg;
	MV_STATUS status;

	/* Verify parameters */
	if (linkId > MV_IPC_LINKS_NUM) {
		mvIpcErrPrintf("IPC ERROR: Ack attach: Bad link id %d\n", chnId);
		return MV_FALSE;
	}

	link = &mv_ipc_links[linkId];

	if (chnId > link->numOfChannels) {
		mvIpcErrPrintf("IPC ERROR:Ack attach: Bad channel id %d\n", chnId);
		return MV_FALSE;
	}

	chn = &link->channels[chnId];

	/* Cannot acknowledge remote attach until local attach was requested*/
	if ((chn->state != MV_CHN_ATTACHED) && (chn->state != MV_CHN_LINKING)) {
		mvIpcDbgPrintf("IPC HAL: Can't acknowledge attach. channel in state %d\n", chn->state);
		return MV_ERROR;
	}

	if (acknowledge == MV_TRUE) {
		/* Check that channel is not already coupled to another CPU*/
		if (chn->remoteNodeId != link->remoteNodeId) {
			mvIpcDbgPrintf("IPC HAL: Can't acknowledge attach. CPU %d != %d\n",
				       chn->remoteNodeId, link->remoteNodeId);
			return MV_ERROR;
		}

		mvIpcDbgPrintf("IPC HAL: Acknowledging attach from CPU %d\n", link->remoteNodeId);

		/* Send the attach acknowledge message */
		attachMsg.type  = IPC_MSG_ATTACH_ACK;
		attachMsg.value = link->remoteNodeId;
		attachMsg.size  = 0;
		attachMsg.ptr   = 0;
		status = mvIpcTxCtrlMsg(linkId, chnId, &attachMsg);
		if (status != MV_OK) {
			mvIpcErrPrintf("IPC ERROR: Cannot Send attach acknowledge message\n");
			return MV_ERROR;
		}
	}

	/* Now change my own state to attached */
	chn->state = MV_CHN_ATTACHED;

	return MV_OK;
}

/***********************************************************************************
 * mvIpcAckDetach
 *
 * DESCRIPTION:
 *		Acknowledges detach request from receiver. this closes the channel for
 *		transmission and resets the queues
 *
 * INPUT:
 *		linkId  - the link ID
 *		chnId - the channel ID
 *		acknowledge - do i need to acknowledge the message
 * OUTPUT:
 *       None
 * RETURN:
 *		MV_OK or MV_ERROR
 *
 ************************************************************************************/
static MV_STATUS mvIpcAckDetach(MV_U32 linkId, MV_U32 chnId, MV_BOOL acknowledge)
{
	MV_IPC_LINK             *link;
	MV_IPC_CHANNEL *chn;
	MV_IPC_MSG dettachMsg;
	MV_STATUS status;
	MV_U32 msgId;

	/* Verify parameters */
	if (linkId > MV_IPC_LINKS_NUM) {
		mvIpcErrPrintf("IPC ERROR: Ack detach: Bad link id %d\n", chnId);
		return MV_FALSE;
	}

	link = &mv_ipc_links[linkId];

	if (chnId > link->numOfChannels) {
		mvIpcErrPrintf("IPC ERROR:Ack detach: Bad channel id %d\n", chnId);
		return MV_FALSE;
	}

	chn = &link->channels[chnId];

	/* Cannot acknowledge remote detach until local attach was requested*/
	if ((chn->state != MV_CHN_ATTACHED) && (chn->state != MV_CHN_UNLINKING)) {
		mvIpcDbgPrintf("IPC HAL: Can't acknowledge detach. channel in state %d\n", chn->state);
		return MV_ERROR;
	}

	if (acknowledge == MV_TRUE) {
		/* Send the attach acknowledge message */
		dettachMsg.type  = IPC_MSG_DETACH_ACK;
		dettachMsg.size  = 0;
		dettachMsg.ptr   = 0;
		dettachMsg.value = 0;

		status = mvIpcTxCtrlMsg(linkId, chnId, &dettachMsg);
		if (status != MV_OK) {
			mvIpcErrPrintf("IPC ERROR: Cannot Send dettach acknowledge message\n");
			return MV_ERROR;
		}
	}

	/* Now change my own state to attached */
	chn->state                = MV_CHN_OPEN;
	chn->txEnable     = MV_FALSE;
	chn->nextRxMsgIdx = 1;
	chn->nextTxMsgIdx = 1;

	/* Initialize the transmit queue */
	for (msgId = 1; msgId < chn->queSizeInMsg; msgId++)
		chn->txMsgQueVa[msgId].isUsed = MV_FALSE;

	return MV_OK;

	mvIpcDbgPrintf("IPC HAL: Acknowledging dettach message\n");
}

/***********************************************************************************
 * mvIpcReqAttach
 *
 * DESCRIPTION:
 *		Ask receiver to acknowledge attach request. To verify reception, message
 *		transmission is possible only after receiver acknowledges the attach
 *
 * INPUT:
 *		chn   - pointer to channel structure
 *		chnId - the channel ID
 * OUTPUT:
 *       None
 * RETURN:
 *		MV_OK or MV_ERROR
 *
 ************************************************************************************/
static MV_STATUS mvIpcReqAttach(MV_U32 linkId, MV_IPC_CHANNEL *chn, MV_U32 chnId)
{
	MV_IPC_MSG attachMsg;
	MV_STATUS status;
	int backoff = 10, timeout = 10;

	mvIpcDbgPrintf("IPC HAL: Requesting attach from cpu %d\n", chn->remoteNodeId);

	/* Send the attach message */
	attachMsg.type  = IPC_MSG_ATTACH_REQ;
	attachMsg.value = mvIpcWhoAmI();
	status = mvIpcTxCtrlMsg(linkId, chnId, &attachMsg);
	if (status != MV_OK) {
		mvIpcErrPrintf("IPC ERROR: Cannot Send attach req message\n");
		return MV_ERROR;
	}

	/* Give the receiver 10 seconds to reply */
	while ((chn->state != MV_CHN_ATTACHED) && timeout) {
		udelay(backoff);
		timeout--;
	}

	if (chn->state != MV_CHN_ATTACHED) {
		mvIpcDbgPrintf("IPC HAL: Cannot complete attach sequence. no reply from receiver after %d usec\n",
			       timeout * backoff);
		return MV_ERROR;
	}

	mvIpcDbgPrintf("IPC HAL: Attached channel %d\n", chnId);

	return MV_OK;
}

/***********************************************************************************
 * mvIpcAttachChannel
 *
 * DESCRIPTION:
 *		Attempts to attach the TX queue to a remote CPU by sending a ATTACH ACK
 *		messages to receiver. if the message is acknowledged the the channel state
 *		becomes attached and message transmission is enabled.
 *
 * INPUT:
 *		linkId  - the link ID
 *		chnId           - The channel ID
 *		remoteNodeId - CPU ID of receiver
 * OUTPUT:
 *		attached   - indicates if channel is attached
 * RETURN:
 *		MV_OK or MV_ERROR
 *
 ************************************************************************************/
MV_STATUS mvIpcAttachChannel(MV_U32 linkId, MV_U32 chnId, MV_U32 remoteNodeId, MV_BOOL *attached)
{
	MV_IPC_LINK             *link;
	MV_IPC_CHANNEL *chn;
	MV_U32 msgId;
	MV_STATUS status;

	(*attached) = 0;

	/* Verify parameters */
	if (linkId > MV_IPC_LINKS_NUM) {
		mvIpcErrPrintf("IPC ERROR: Chn attach: Bad link id %d\n", chnId);
		return MV_FALSE;
	}

	link = &mv_ipc_links[linkId];

	if (chnId > link->numOfChannels) {
		mvIpcErrPrintf("IPC ERROR: Chn attach: Bad channel id %d\n", chnId);
		return MV_FALSE;
	}

	chn = &link->channels[chnId];

	if (chn->state == MV_CHN_CLOSED) {
		mvIpcErrPrintf("IPC ERROR: Can't attach channel %d. It is closed\n", chnId);
		return MV_ERROR;
	}

	if (chn->state == MV_CHN_ATTACHED) {
		(*attached) = 1;
		return MV_OK;
	}

	chn->state                = MV_CHN_LINKING;
	chn->remoteNodeId  = remoteNodeId;
	chn->txEnable     = MV_TRUE;

	/* Initialize the transmit queue */
	for (msgId = 1; msgId < chn->queSizeInMsg; msgId++)
		chn->txMsgQueVa[msgId].isUsed = MV_FALSE;

	/* Send req for attach to other side */
	status = mvIpcReqAttach(linkId, chn, chnId);
	if (status == MV_OK) {
		(*attached) = 1;
		mvIpcDbgPrintf("IPC HAL: Attached channel %d to link %d\n", chnId, linkId);
	}

	return MV_OK;
}

/***********************************************************************************
 * mvIpcDettachChannel
 *
 * DESCRIPTION:
 *		Detaches the channel from remote cpu. it notifies the remote cpu by sending
 *		control message and waits for acknowledge. after calling this function
 *		data messages cannot be sent anymore
 *
 * INPUT:
 *		linkId  - the link ID
 *		chnId           - The channel ID
 * OUTPUT:
 *       None
 * RETURN:
 *		MV_OK or MV_ERROR
 *
 ************************************************************************************/
MV_STATUS mvIpcDettachChannel(MV_U32 linkId, MV_U32 chnId)
{
	MV_IPC_LINK             *link;
	MV_IPC_CHANNEL *chn;
	MV_IPC_MSG msg;
	MV_STATUS status;

	/* Verify parameters */
	if (linkId > MV_IPC_LINKS_NUM) {
		mvIpcErrPrintf("IPC ERROR: Chn detach: Bad link id %d\n", chnId);
		return MV_FALSE;
	}

	link = &mv_ipc_links[linkId];

	if (chnId > link->numOfChannels) {
		mvIpcErrPrintf("IPC ERROR: Chn detach: Bad channel id %d\n", chnId);
		return MV_FALSE;
	}

	chn = &link->channels[chnId];

	if (chn->state != MV_CHN_ATTACHED) {
		mvIpcErrPrintf("IPC ERROR: Detach: channel %d is not attached\n", chnId);
		return MV_ERROR;
	}

	msg.type  = IPC_MSG_DETACH_REQ;
	msg.size  = 0;
	msg.ptr   = 0;
	msg.value = 0;

	status = mvIpcTxCtrlMsg(linkId, chnId, &msg);
	if (status != MV_OK) {
		mvIpcErrPrintf("IPC ERROR: Cannot Send detach request message\n");
		return MV_ERROR;
	}

	chn->remoteNodeId  = 0;
	chn->state        = MV_CHN_UNLINKING;

	return MV_OK;
}

/***********************************************************************************
 * mvIpcCloseChannel - CLose and IPC channel
 *
 * DESCRIPTION:
 *		Closes the      IPC channels. this disables the channels ability to receive messages
 *
 * INPUT:
 *		linkId          - the link ID
 *		chnId           - The channel ID
 * OUTPUT:
 *       None
 * RETURN:
 *		MV_OK or MV_ERROR
 *
 ************************************************************************************/
MV_STATUS mvIpcCloseChannel(MV_U32 linkId, MV_U32 chnId)
{
	MV_IPC_LINK             *link;
	MV_IPC_CHANNEL *chn;

	/* Verify parameters */
	if (linkId > MV_IPC_LINKS_NUM) {
		mvIpcErrPrintf("IPC ERROR: Chn close: Bad link id %d\n", chnId);
		return MV_FALSE;
	}

	link = &mv_ipc_links[linkId];

	if (chnId > link->numOfChannels) {
		mvIpcErrPrintf("IPC ERROR: Chn close: Bad channel id %d\n", chnId);
		return MV_FALSE;
	}

	chn = &link->channels[chnId];

	if (chn->state == MV_CHN_CLOSED) {
		mvIpcErrPrintf("IPC ERROR: Close channel: Channel %d is already closed\n", chnId);
		return MV_ERROR;
	}

	chn->state       = MV_CHN_CLOSED;
	chn->txEnable    = MV_FALSE;
	chn->rxEnable    = MV_FALSE;
	chn->remoteNodeId = 0;

	mvIpcDbgPrintf("IPC HAL: Closed channel %d successfully\n", chnId);

	return MV_OK;
}

/***********************************************************************************
 * mvIpcIsTxReady
 *
 * DESCRIPTION:
 *		Checks if the channel is ready to transmit
 *
 * INPUT:
 *		chnId           - The channel ID
 * OUTPUT:
 *       None
 * RETURN:
 *		MV_OK or MV_ERROR
 *
 ************************************************************************************/
MV_BOOL mvIpcIsTxReady(MV_U32 linkId, MV_U32 chnId)
{
	MV_IPC_LINK             *link;
	MV_IPC_CHANNEL *chn;

	/* Verify parameters */
	if (linkId > MV_IPC_LINKS_NUM) {
		mvIpcErrPrintf("IPC ERROR: Chn is ready: Bad link id %d\n", chnId);
		return MV_FALSE;
	}

	link = &mv_ipc_links[linkId];

	if (chnId > link->numOfChannels) {
		mvIpcErrPrintf("IPC ERROR: Chn is ready: Bad channel id %d\n", chnId);
		return MV_FALSE;
	}

	chn = &link->channels[chnId];

	if (chn->state != MV_CHN_ATTACHED) {
		mvIpcErrPrintf("IPC ERROR: Tx Test: channel not attached, state is %d\n", chn->state);
		return MV_FALSE;
	}

	/* Is next message still used by receiver, yes means full queue or bug */
	if (chn->txMsgQueVa[chn->nextTxMsgIdx].isUsed != MV_FALSE) {
		mvIpcDbgPrintf("IPC HAL: Tx Test: Can't send, Msg %d used flag = %d\n",
			       chn->nextTxMsgIdx, chn->txMsgQueVa[chn->nextTxMsgIdx].isUsed);
		return MV_FALSE;
	}

	return MV_TRUE;
}

/***********************************************************************************
 * mvIpcTxCtrlMsg
 *
 * DESCRIPTION:
 *		Sends a control message to other side. these messages are not forwarded
 *		to user
 *
 * INPUT:
 *		linkId  - the link ID
 *		chnId - The channel ID
 *		inMsg - Pointer to message to send
 * OUTPUT:
 *       None
 * RETURN:
 *		MV_OK or MV_ERROR
 *
 ************************************************************************************/
MV_STATUS mvIpcTxCtrlMsg(MV_U32 linkId, MV_U32 chnId, MV_IPC_MSG *inMsg)
{
	MV_IPC_LINK             *link;
	MV_IPC_CHANNEL *chn;

	if (linkId > MV_IPC_LINKS_NUM) {
		mvIpcErrPrintf("IPC ERROR: Tx Ctrl Msg: Bad link id %d\n", chnId);
		return MV_FALSE;
	}

	link = &mv_ipc_links[linkId];

	if (chnId > link->numOfChannels) {
		mvIpcErrPrintf("IPC ERROR: Tx Ctr Msg: Bad channel id %d\n", chnId);
		return MV_FALSE;
	}

	chn = &link->channels[chnId];

	if (chn->txEnable == MV_FALSE) {
		mvIpcErrPrintf("IPC ERROR: Tx Ctrl msg: Tx not enabled\n");
		return MV_ERROR;
	}

	/* Write the message and pass */
	chn->txCtrlMsg->type  = inMsg->type;
	chn->txCtrlMsg->size  = inMsg->size;
	chn->txCtrlMsg->ptr   = inMsg->ptr;
	chn->txCtrlMsg->value = inMsg->value;

	/* Make sure the msg values are written before the used flag
	 * to ensure the polling receiver will get valid message once
	 * it detects isUsed == MV_TRUE.
	 */
	mvOsSync();

	chn->txCtrlMsg->isUsed   = MV_TRUE;

	mvIpcDbgWrite(chn->txCtrlMsg->align[0], MV_IPC_HAND_SHAKE_MAGIC);
	mvIpcDbgWrite(chn->txCtrlMsg->align[1], 0);
	mvIpcDbgWrite(chn->txCtrlMsg->align[2], 0);

	mvIpcDbgPrintf("IPC HAL: Sent control message 0x%8x on channel %d to link %d\n",
			(int)chn->txCtrlMsg, chnId, linkId);

	/*Raise the TX ready flag and send the trigger*/
	*((MV_U32 *)chn->txMessageFlag) = 0x1;
	chn->sendTrigger(chn->remoteNodeId, chnId);

	return MV_OK;
}

/***********************************************************************************
 * mvIpcTxMsg
 *
 * DESCRIPTION:
 *		Main transmit function
 *
 * INPUT:
 *		linkId  - the link ID
 *		chnId - The channel ID
 *		inMsg - Pointer to message to send
 * OUTPUT:
 *       None
 * RETURN:
 *		MV_OK or MV_ERROR
 *
 ************************************************************************************/
MV_STATUS mvIpcTxMsg(MV_U32 linkId, MV_U32 chnId, MV_IPC_MSG *inMsg)
{
	MV_IPC_LINK             *link;
	MV_IPC_CHANNEL *chn;
	MV_IPC_MSG     *currMsg;

	if (linkId > MV_IPC_LINKS_NUM) {
		mvIpcErrPrintf("IPC ERROR: Tx Msg: Bad link id %d\n", chnId);
		return MV_FALSE;
	}

	link = &mv_ipc_links[linkId];

	if (chnId > link->numOfChannels) {
		mvIpcErrPrintf("IPC ERROR: Tx Msg: Bad channel id %d\n", chnId);
		return MV_FALSE;
	}

	chn = &link->channels[chnId];

	/*Test if TX ready to send*/
	if (chn->state != MV_CHN_ATTACHED) {
		mvIpcErrPrintf("IPC ERROR: Tx Msg: channel not attached, state is %d\n", chn->state);
		return MV_FALSE;
	}

	/* Is next message still used by receiver, yes means full queue or bug */
	if (chn->txMsgQueVa[chn->nextTxMsgIdx].isUsed != MV_FALSE) {
		mvIpcDbgPrintf("IPC HAL: Tx Msg: Can't send, Msg %d used flag = %d\n",
			chn->nextTxMsgIdx, chn->txMsgQueVa[chn->nextTxMsgIdx].isUsed);
		return MV_FALSE;
	}

	/* Write the message */
	currMsg  = &chn->txMsgQueVa[chn->nextTxMsgIdx];

	currMsg->type  = inMsg->type;
	currMsg->size  = inMsg->size;
	currMsg->ptr   = inMsg->ptr;
	currMsg->value = inMsg->value;

	/* Make sure the msg values are written before the used flag
	 * to ensure the polling receiver will get valid message once
	 * it detects isUsed == MV_TRUE.
	 */
	mvOsSync();

	/* Pass ownership to remote cpu */
	currMsg->isUsed   = MV_TRUE;

	mvIpcDbgWrite(currMsg->align[0], MV_IPC_HAND_SHAKE_MAGIC);
	mvIpcDbgWrite(currMsg->align[1], 0);
	mvIpcDbgWrite(currMsg->align[2], 0);

	chn->nextTxMsgIdx++;
	if (chn->nextTxMsgIdx == chn->queSizeInMsg)
		chn->nextTxMsgIdx = 1;

	mvIpcDbgPrintf("IPC HAL: Sent message %d on channel %d to link %d\n",
			chn->nextTxMsgIdx - 1, chnId, linkId);

	/*Raise the TX ready flag and send the trigger*/
	*((MV_U32 *)chn->txMessageFlag) = 0x1;
	chn->sendTrigger(chn->remoteNodeId, chnId);

	return MV_OK;
}

/***********************************************************************************
 * mvIpcRxCtrlMsg
 *
 * DESCRIPTION:
 *		This routine initializes IPC channel: setup receive queue and enable data receiving
 *		This routine receives IPC control structure (ipcCtrl) as input parameter.
 *		The following ipcCtrl members must be initialized prior calling this function:
 *
 * INPUT:
 *		linkId  - the link ID
 *		chnId - The channel ID
 *		msg   - Pointer to received control message
 * OUTPUT:
 *       None
 * RETURN:
 *		void
 *
 ************************************************************************************/
static void mvIpcRxCtrlMsg(MV_U32 linkId, MV_U32 chnId, MV_IPC_MSG *msg)
{
	mvIpcDbgPrintf("IPC HAL: Processing control message %d\n", msg->type);

	switch (msg->type) {
	case IPC_MSG_ATTACH_REQ:
		mvIpcAckAttach(linkId, chnId, MV_TRUE);
		break;

	case IPC_MSG_ATTACH_ACK:
		mvIpcAckAttach(linkId, chnId, MV_FALSE);
		break;

	case IPC_MSG_DETACH_REQ:
		mvIpcAckDetach(linkId, chnId, MV_TRUE);
		break;

	case IPC_MSG_DETACH_ACK:
		mvIpcAckDetach(linkId, chnId, MV_FALSE);
		break;

	default:
		mvIpcDbgPrintf("IPC HAL: Unknown internal message type %d\n", msg->type);
	}

	mvIpcDbgWrite(msg->align[2], MV_IPC_HAND_SHAKE_MAGIC);

	mvIpcReleaseMsg(linkId, chnId, msg);
}

/***********************************************************************************
 * mvIpcDisableChnRx
 *
 * DESCRIPTION:
 *		Masks the given channel in ISR
 *
 * INPUT:
 *		linkId  - the link ID
 *		chnId - The channel ID
 * OUTPUT:
 *       None
 * RETURN:
 *		MV_OK or MV_ERROR
 *
 ************************************************************************************/
MV_VOID mvIpcDisableChnRx(MV_U32 linkId, MV_U32 chnId)
{
	MV_IPC_LINK             *link;
	MV_IPC_CHANNEL *chn;

	if (linkId > MV_IPC_LINKS_NUM) {
		mvIpcErrPrintf("IPC ERROR:  Dis Chn RX: Bad link id %d\n", chnId);
		return;
	}

	link = &mv_ipc_links[linkId];

	if (chnId > link->numOfChannels) {
		mvIpcErrPrintf("IPC ERROR: Dis Chn RX: Bad channel id %d\n", chnId);
		return;
	}

	chn = &link->channels[chnId];

	chn->registerChnInISR(linkId, chnId, MV_FALSE);

	mvIpcDbgPrintf("IPC HAL: Disabled ISR for link %d, channel %d\n", linkId, chnId);
	return;
}

/***********************************************************************************
 * mvIpcEnableChnRx
 *
 * DESCRIPTION:
 *		Unmasks the given channel in ISR
 *
 * INPUT:
 *		linkId  - the link ID
 *		chnId - The channel ID
 * OUTPUT:
 *       None
 * RETURN:
 *		MV_OK or MV_ERROR
 *
 ************************************************************************************/
MV_VOID mvIpcEnableChnRx(MV_U32 linkId, MV_U32 chnId)
{
	MV_IPC_LINK             *link;
	MV_IPC_CHANNEL *chn;

	if (linkId > MV_IPC_LINKS_NUM) {
		mvIpcErrPrintf("IPC ERROR:  Ena Chn RX: Bad link id %d\n", chnId);
		return;
	}

	link = &mv_ipc_links[linkId];

	if (chnId > link->numOfChannels) {
		mvIpcErrPrintf("IPC ERROR: Ena Chn RX: Bad channel id %d\n", chnId);
		return;
	}

	chn = &link->channels[chnId];

	chn->registerChnInISR(linkId, chnId, MV_TRUE);

	mvIpcDbgPrintf("IPC HAL: Enabled ISR for link %d, channel %d\n", linkId, chnId);
	return;
}

/***********************************************************************************
 * mvIpcRxMsg
 *
 * DESCRIPTION:
 *		Main Rx routine - should be called from interrupt routine
 *
 * INPUT:
 *		drblNum  - number of doorbel received
 * OUTPUT:
 *		linkId  - the link ID
 *       chnId - the channel id that received a message
 *       outMsg   - pointer to the message received
 * RETURN:
 *		MV_TRUE  - if a message was received
 *		MV_FALSE - if no message exists
 *
 ************************************************************************************/
MV_STATUS mvIpcRxMsg(MV_U32 linkId, MV_U32 chnId)
{
	MV_IPC_LINK             *link;
	MV_IPC_CHANNEL  *chn;
	MV_IPC_MSG     *currMsg;
	MV_U32 msgIndx;

	if (linkId > MV_IPC_LINKS_NUM) {
		mvIpcErrPrintf("IPC ERROR: Rx msg: Bad link id %d\n", chnId);
		return MV_FAIL;
	}

	link = &mv_ipc_links[linkId];

	if (chnId > link->numOfChannels) {
		mvIpcErrPrintf("IPC ERROR: Rx msg: Bad channel id %d\n", chnId);
		return MV_FAIL;
	}

	chn = &link->channels[chnId];

	if (chn->state == MV_CHN_CLOSED)
		return MV_FALSE;

	/* First process control messages like attach, detach, close */
	if (chn->rxCtrlMsg->isUsed == MV_TRUE)
		mvIpcRxCtrlMsg(linkId, chnId, chn->rxCtrlMsg);

	msgIndx = chn->nextRxMsgIdx;
	currMsg = &chn->rxMsgQueVa[msgIndx];

	// Check for unread data messages in queue */
	if (currMsg->isUsed != MV_TRUE) {
		/*No more messages, disable RX ready flag*/
		*((MV_U32 *)chn->rxMessageFlag) = 0x0;
		return MV_NO_MORE;
	}

	/* Increment msg idx to keep in sync with sender */
	chn->nextRxMsgIdx++;
	if (chn->nextRxMsgIdx == chn->queSizeInMsg)
		chn->nextRxMsgIdx = 1;

	// Check if channel is ready to receive messages */
	if (chn->state < MV_CHN_OPEN) {
		mvIpcErrPrintf("IPC ERROR: Rx msg: Channel not ready, state = %d\n", chn->state);
		return MV_FAIL;
	}

	mvIpcDbgWrite(currMsg->align[2], MV_IPC_HAND_SHAKE_MAGIC);

	/* Now process user messages */
	mvIpcDbgPrintf("IPC HAL: Received message %d on channel %d\n",
			chn->nextRxMsgIdx - 1, chnId);

	/*Call user function to care the message*/
	chn->rxCallback(currMsg);

	return MV_OK;
}

/***********************************************************************************
 * mvIpcRxMsgFlagCheck
 *
 * DESCRIPTION:
 *		Check if RX flag raided
 *
 * INPUT:
 *		linkId  - the link ID
 *       chnId - the channel id that received a message
 * OUTPUT:
 * RETURN:
 *		MV_TRUE  - if a RX flag raised
 *		MV_FALSE - if no RX waiting
 *
 ************************************************************************************/
MV_BOOL mvIpcRxMsgFlagCheck(MV_U32 linkId, MV_U32 chnId)
{
	MV_IPC_LINK             *link;
	MV_IPC_CHANNEL  *chn;

	if (linkId > MV_IPC_LINKS_NUM) {
		mvIpcErrPrintf("IPC ERROR: Rx msg: Bad link id %d\n", chnId);
		return MV_FALSE;
	}

	link = &mv_ipc_links[linkId];

	if (chnId > link->numOfChannels) {
		mvIpcErrPrintf("IPC ERROR: Rx msg: Bad channel id %d\n", chnId);
		return MV_FALSE;
	}

	chn = &link->channels[chnId];

	if (chn->state == MV_CHN_CLOSED)
		return MV_FALSE;

	if (*((MV_U32 *)chn->rxMessageFlag) == 0x1)
		return MV_TRUE;
	else
		return MV_FALSE;
}

/***********************************************************************************
 * mvIpcReleaseMsg
 *
 * DESCRIPTION:
 *		Return ownership on message to transmitter
 *
 * INPUT:
 *		linkId  - the link ID
 *		chnId - The channel ID
 *		msg   - Pointer to message to release
 * OUTPUT:
 *       None
 * RETURN:
 *		MV_OK or MV_ERROR
 *
 ************************************************************************************/
MV_STATUS mvIpcReleaseMsg(MV_U32 linkId, MV_U32 chnId, MV_IPC_MSG *msg)
{
	MV_IPC_LINK             *link;
	MV_IPC_CHANNEL *chn;

	if (linkId > MV_IPC_LINKS_NUM) {
		mvIpcErrPrintf("IPC ERROR: Tx Msg: Bad link id %d\n", chnId);
		return MV_FALSE;
	}

	link = &mv_ipc_links[linkId];

	if (chnId > link->numOfChannels) {
		mvIpcErrPrintf("IPC ERROR: Tx Msg: Bad channel id %d\n", chnId);
		return MV_FALSE;
	}

	chn = &link->channels[chnId];

	if (chn->state == MV_CHN_CLOSED) {
		mvIpcErrPrintf("IPC ERROR: Msg release: Inactive channel id %d\n", chnId);
		return MV_ERROR;
	}

	if (msg->isUsed == MV_FALSE) {
		mvIpcErrPrintf("IPC ERROR: Msg release: Msg %d owned by %d\n",
			chn->nextRxMsgIdx, msg->isUsed);
		return MV_ERROR;
	}

	msg->isUsed   = MV_FALSE;
	mvIpcDbgWrite(msg->align[1], MV_IPC_HAND_SHAKE_MAGIC);

	mvIpcDbgPrintf("IPC HAL: Released message 0x%8x on channel %d\n", (int)msg, chnId);

	return MV_OK;
}

/***********************************************************************************
 * mvIpcShmemMalloc
 *
 * DESCRIPTION:
 *		Malloc buffer in shared memory heap for TX buffers
 *		(Sequentual malloc, no free allowed)
 *
 * INPUT:
 *		linkId  - the link ID
 *		size - requested buffer size
 * OUTPUT:
 *       offset of the buffer
 * RETURN:
 *		MV_OK or MV_ERROR
 *
 ************************************************************************************/
MV_VOID *mvIpcShmemMalloc(MV_U32 linkId, MV_U32 size)
{
	MV_IPC_LINK             *link;
	MV_VOID *ptr;

	if (linkId > MV_IPC_LINKS_NUM) {
		mvIpcErrPrintf("IPC ERROR: Tx Msg: Bad link id %d\n", linkId);
		return MV_FALSE;
	}

	link = &mv_ipc_links[linkId];

	if (size > link->txSharedHeapSize)
		return NULL;

	ptr = (MV_VOID *)link->txSharedHeapAddr;

	link->txSharedHeapAddr  += size;
	link->txSharedHeapSize -= size;

	return ptr;
}
