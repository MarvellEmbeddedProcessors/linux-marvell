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
#ifndef __mvIpc_h
#define __mvIpc_h

/*Channel statuses*/
typedef enum {
	MV_CHN_CLOSED =  0,
	MV_CHN_OPEN,
	MV_CHN_LINKING,
	MV_CHN_UNLINKING,
	MV_CHN_ATTACHED

}MV_IPC_CHN_STATE;

/*Message struct(channel queue entry)*/
typedef struct __ipc_message_struct {
	MV_U32 type;
	MV_U32 size;                    /*buffer size*/
	MV_VOID *ptr;                   /*buffer virtual address for Rx side*/
	MV_U32 value;                   /*User data*/
	MV_U32 isUsed;                  /*CPU Id and optional oob message*/
	MV_U32 align[3];                /* Align message size to cache line */
} MV_IPC_MSG;

/*Function types*/
typedef int (*MV_IPC_RX_CLBK)(MV_IPC_MSG *msg);
typedef MV_VOID (*MV_IPC_SEND_TRIGGER)(MV_U32 linkId, MV_U32 chnId);
typedef MV_VOID (*MV_IPC_RX_CHANNEL_REGISTER)(MV_U32 linkId, MV_U32 chnId, MV_BOOL enable);

/*Channel struct*/
typedef struct __ipc_channel_struct {
	MV_IPC_MSG *rxMsgQueVa;         /*buffer virtual address for Rx side*/
	MV_IPC_MSG *txMsgQueVa;         /*buffer virtual address for Tx side*/
	MV_IPC_MSG *rxCtrlMsg;          /*buffer virtual address for Rx side*/
	MV_IPC_MSG *txCtrlMsg;          /*buffer virtual address for Tx side*/
	MV_U32 nextRxMsgIdx;
	MV_U32 nextTxMsgIdx;
	MV_U32 queSizeInMsg;
	MV_U32 remoteNodeId;
	MV_BOOL txEnable;
	MV_BOOL rxEnable;
	MV_IPC_CHN_STATE state;

	MV_U32 txMessageFlag;                           /*Shared memory flag raised for message in queue*/
	MV_U32 rxMessageFlag;                           /*Shared memory flag raised for message in queue*/

	MV_IPC_RX_CLBK rxCallback;                      /*Called for for each RX*/
	MV_IPC_SEND_TRIGGER sendTrigger;                /*Trigger to remote node to start RX*/
	MV_IPC_RX_CHANNEL_REGISTER registerChnInISR;    /*Register the channel in RX ISR/Timer*/
} MV_IPC_CHANNEL;

/*Magic for masterConfigDone, wrote by master and clean by slave*/
#define MV_IPC_MASTER_CONFIG_MAGIC      0x12345678
#define MV_IPC_HAND_SHAKE_MAGIC         0x87654321
/*Link struct(hold array of channels)*/
typedef struct __ipc_link_struct {
	MV_IPC_CHANNEL *channels;       /*Array of channels*/
	MV_U32 numOfChannels;           /*Number of channels*/
	MV_U32 shmemBaseAddr;           /*Shared mem physycal addr*/
	MV_U32 shmemSize;               /*Shared mem physycal addr*/
	MV_U32 nodeId;                  /*I node ID*/
	MV_U32 remoteNodeId;            /*remote node ID*/
	MV_U32 txSharedHeapAddr;        /*offset of heap node memory*/
	MV_U32 txSharedHeapSize;        /*size of heap node memory*/
	MV_U32 rxSharedHeapAddr;        /*offset of heap for remote node memory*/
	MV_U32 rxSharedHeapSize;        /*size of heap node memory*/
	MV_U32 masterConfigDone;        /*if master finished the configuration*/
	MV_U32 slaveLinkInitialized;   /*if master not finished the configuration
									and configuration was postponed by slave*/
} MV_IPC_LINK;

/*Control messages types*/
typedef enum {
	IPC_MSG_ATTACH_REQ = 0,
	IPC_MSG_ATTACH_ACK,
	IPC_MSG_DETACH_REQ,
	IPC_MSG_DETACH_ACK
}MV_IPC_CTRL_MSG_TYPE;

MV_STATUS mvIpcLinkStart(MV_U32 linkId);
MV_STATUS mvIpcClose(MV_U32 linkId);
MV_STATUS mvIpcOpenChannel(MV_U32 linkId, MV_U32 chnId, MV_IPC_RX_CLBK rx_clbk);
MV_STATUS mvIpcCloseChannel(MV_U32 linkId, MV_U32 chnId);
MV_STATUS mvIpcAttachChannel(MV_U32 linkId, MV_U32 chnId, MV_U32 remoteCpuId, MV_BOOL *attached);
MV_STATUS mvIpcDettachChannel(MV_U32 linkId, MV_U32 chnId);
MV_BOOL   mvIpcIsTxReady(MV_U32 linkId, MV_U32 chnId);
MV_STATUS mvIpcTxMsg(MV_U32 linkId, MV_U32 chnId, MV_IPC_MSG *inMsg);
MV_STATUS mvIpcTxCtrlMsg(MV_U32 linkId, MV_U32 chnId, MV_IPC_MSG *inMsg);
MV_STATUS mvIpcRxMsg(MV_U32 linkId, MV_U32 chnId);
MV_BOOL mvIpcRxMsgFlagCheck(MV_U32 linkId, MV_U32 chnId);
MV_STATUS mvIpcReleaseMsg(MV_U32 linkId, MV_U32 chnId, MV_IPC_MSG *msg);
MV_VOID   mvIpcDisableChnRx(MV_U32 linkId, MV_U32 chnId);
MV_VOID   mvIpcEnableChnRx(MV_U32 linkId, MV_U32 chnId);
MV_VOID *mvIpcShmemMalloc(MV_U32 linkId, MV_U32 size);

#endif /*__mvIpc_h */
