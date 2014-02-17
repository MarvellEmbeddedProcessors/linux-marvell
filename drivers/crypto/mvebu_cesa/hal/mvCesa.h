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

/*******************************************************************************
* mvCesa.h - Header File for Cryptographic Engines and Security Accelerator
*
* DESCRIPTION:
*       This header file contains macros typedefs and function declaration for
*       the Marvell Cryptographic Engines and Security Accelerator.
*
*******************************************************************************/

#ifndef __mvCesa_h__
#define __mvCesa_h__

#ifdef __cplusplus
extern "C" {
#endif

#include "ctrlEnv/mvCtrlEnvSpec.h"
#include "mvSysCesaConfig.h"
#include "mvCesaRegs.h"

typedef enum {
	MV_CESA_SPLIT_NONE = 0,
	MV_CESA_SPLIT_FIRST = 1,
	MV_CESA_SPLIT_SECOND = 2
} MV_CESA_SPLIT;

typedef enum {
	CESA_NULL_POLICY = 0,
	CESA_SINGLE_CHAN_POLICY,
	CESA_DUAL_CHAN_BALANCED_POLICY,
	CESA_WEIGHTED_CHAN_POLICY,
	CESA_FLOW_ASSOC_CHAN_POLICY
} MV_CESA_POLICY;

typedef enum {
	CESA_NULL_FLOW_TYPE = 0,
	CESA_IPSEC_FLOW_TYPE,
	CESA_SSL_FLOW_TYPE,
	CESA_DISK_FLOW_TYPE,
	CESA_NFPSEC_FLOW_TYPE
} MV_CESA_FLOW_TYPE;

typedef struct {
	MV_ULONG sramPhysBase[MV_CESA_CHANNELS];
	MV_U8 *sramVirtBase[MV_CESA_CHANNELS];
	MV_U16 sramOffset[MV_CESA_CHANNELS];
	MV_U16 ctrlModel;	/* Controller Model     */
	MV_U8 ctrlRev;		/* Controller Revision  */
} MV_CESA_HAL_DATA;

/* Redefine MV_DMA_DESC structure */
typedef struct _mvDmaDesc {
	MV_U32 byteCnt;	/* The total number of bytes to transfer        */
	MV_U32 phySrcAdd;	/* The physical source address                  */
	MV_U32 phyDestAdd;	/* The physical destination address             */
	MV_U32 phyNextDescPtr;	/* If we are using chain mode DMA transfer,     */
	/* then this pointer should point to the        */
	/* physical address of the next descriptor,     */
	/* otherwise it should be NULL.                 */
} MV_DMA_DESC;

#define MV_CESA_AUTH_BLOCK_SIZE         64	/* bytes */

#define MV_CESA_MD5_DIGEST_SIZE         16	/* bytes */
#define MV_CESA_SHA1_DIGEST_SIZE        20	/* bytes */
#define MV_CESA_SHA2_DIGEST_SIZE	32	/* bytes */

#define MV_CESA_MAX_DIGEST_SIZE         MV_CESA_SHA2_DIGEST_SIZE

#define MV_CESA_DES_KEY_LENGTH          8	/* bytes = 64 bits */
#define MV_CESA_3DES_KEY_LENGTH         24	/* bytes = 192 bits */
#define MV_CESA_AES_128_KEY_LENGTH      16	/* bytes = 128 bits */
#define MV_CESA_AES_192_KEY_LENGTH      24	/* bytes = 192 bits */
#define MV_CESA_AES_256_KEY_LENGTH      32	/* bytes = 256 bits */

#define MV_CESA_MAX_CRYPTO_KEY_LENGTH   MV_CESA_AES_256_KEY_LENGTH

#define MV_CESA_DES_BLOCK_SIZE          8	/* bytes = 64 bits */
#define MV_CESA_3DES_BLOCK_SIZE         8	/* bytes = 64 bits */

#define MV_CESA_AES_BLOCK_SIZE          16	/* bytes = 128 bits */

#define MV_CESA_MAX_IV_LENGTH           MV_CESA_AES_BLOCK_SIZE

#define MV_CESA_MAX_MAC_KEY_LENGTH      64	/* bytes */

typedef struct {
	MV_U8 cryptoKey[MV_CESA_MAX_CRYPTO_KEY_LENGTH];
	MV_U8 macKey[MV_CESA_MAX_MAC_KEY_LENGTH];
	MV_CESA_OPERATION operation;
	MV_CESA_DIRECTION direction;
	MV_CESA_CRYPTO_ALG cryptoAlgorithm;
	MV_CESA_CRYPTO_MODE cryptoMode;
	MV_U8 cryptoKeyLength;
	MV_CESA_MAC_MODE macMode;
	MV_U8 macKeyLength;
	MV_U8 digestSize;
} MV_CESA_OPEN_SESSION;

typedef struct {
	MV_BUF_INFO *pFrags;
	MV_U16 numFrags;
	MV_U16 mbufSize;
} MV_CESA_MBUF;

typedef struct {
	void *pReqPrv;	/* instead of reqId */
	MV_U32 retCode;
	MV_16 sessionId;
	MV_U16 mbufSize;
	MV_U32 reqId;	/* Driver internal */
	MV_U32 chanId;
} MV_CESA_RESULT;

typedef void (*MV_CESA_CALLBACK) (MV_CESA_RESULT *pResult);

typedef struct {
	void *pReqPrv;	/* instead of reqId */
	MV_CESA_MBUF *pSrc;
	MV_CESA_MBUF *pDst;
	MV_CESA_CALLBACK *pFuncCB;
	MV_16 sessionId;
	MV_U16 ivFromUser;
	MV_U16 ivOffset;
	MV_U16 cryptoOffset;
	MV_U16 cryptoLength;
	MV_U16 digestOffset;
	MV_U16 macOffset;
	MV_U16 macLength;
	MV_BOOL skipFlush;
	MV_CESA_SPLIT split;
	MV_U32 reqId;	/* Driver internal */
	MV_CESA_FLOW_TYPE flowType;
} MV_CESA_COMMAND;

MV_STATUS mvCesaHalInit(int numOfSession, int queueDepth, void *osHandle, MV_CESA_HAL_DATA *halData);
MV_STATUS mvCesaTdmaWinInit(MV_U8 chan, MV_UNIT_WIN_INFO *addrWinMap);
MV_STATUS mvCesaFinish(void);
MV_STATUS mvCesaSessionOpen(MV_CESA_OPEN_SESSION *pSession, short *pSid);
MV_STATUS mvCesaSessionClose(short sid);
MV_STATUS mvCesaCryptoIvSet(MV_U8 chan, MV_U8 *pIV, int ivSize);
MV_STATUS mvCesaAction(MV_U8 chan, MV_CESA_COMMAND *pCmd);
MV_STATUS mvCesaReadyGet(MV_U8 chan, MV_CESA_RESULT *pResult);
int mvCesaMbufOffset(MV_CESA_MBUF *pMbuf, int offset, int *pBufOffset);
MV_STATUS mvCesaCopyFromMbuf(MV_U8 *pDst, MV_CESA_MBUF *pSrcMbuf, int offset, int size);
MV_STATUS mvCesaCopyToMbuf(MV_U8 *pSrc, MV_CESA_MBUF *pDstMbuf, int offset, int size);
MV_STATUS mvCesaMbufCopy(MV_CESA_MBUF *pMbufDst, int dstMbufOffset,
			MV_CESA_MBUF *pMbufSrc, int srcMbufOffset, int size);

/********** Debug functions ********/
void mvCesaDebugMbuf(const char *str, MV_CESA_MBUF *pMbuf, int offset, int size);
void mvCesaDebugSA(short sid, int mode);
void mvCesaDebugStats(void);
void mvCesaDebugStatsClear(void);
void mvCesaDebugRegs(void);
void mvCesaDebugStatus(void);
void mvCesaDebugQueue(int mode);
void mvCesaDebugSram(int mode);
void mvCesaDebugSAD(int mode);

/********  CESA Private definitions ********/
#define MV_CESA_TDMA_CTRL_VALUE       (MV_CESA_TDMA_DST_BURST_MASK(MV_CESA_TDMA_BURST_128B) 		\
						| MV_CESA_TDMA_SRC_BURST_MASK(MV_CESA_TDMA_BURST_128B)  \
						| MV_CESA_TDMA_OUTSTAND_READ_EN_MASK                    \
						| MV_CESA_TDMA_NO_BYTE_SWAP_MASK			\
						| MV_CESA_TDMA_ENABLE_MASK)

#define MV_CESA_MAX_PKT_SIZE        (64 * 1024)
#define MV_CESA_MAX_MBUF_FRAGS      20

#define MV_CESA_MAX_REQ_FRAGS       ((MV_CESA_MAX_PKT_SIZE / MV_CESA_MAX_BUF_SIZE) + 1)

#define MV_CESA_MAX_DMA_DESC    (MV_CESA_MAX_MBUF_FRAGS*2 + 5)

#define MAX_CESA_CHAIN_LENGTH	20

typedef enum {
	MV_CESA_IDLE = 0,
	MV_CESA_PENDING,
	MV_CESA_PROCESS,
	MV_CESA_READY,
	MV_CESA_CHAIN,
} MV_CESA_STATE;

/* Session database */

/* Map of Key materials of the session in SRAM.
 * Each field must be 8 byte aligned
 * Total size: 32 + 24 + 24 = 80 bytes
 */
typedef struct {
	MV_U8 cryptoKey[MV_CESA_MAX_CRYPTO_KEY_LENGTH];
	MV_U8 macInnerIV[MV_CESA_MAX_DIGEST_SIZE];
	MV_U8 reservedInner[4];
	MV_U8 macOuterIV[MV_CESA_MAX_DIGEST_SIZE];
	MV_U8 reservedOuter[4];
} MV_CESA_SRAM_SA;

typedef struct {
	MV_CESA_SRAM_SA *pSramSA;
	MV_U8 *sramSABuff;	/* holds initial allocation virtual address */
	MV_U32 sramSABuffSize;
	MV_ULONG sramSAPhysAddr;	/* holds initial allocation physical address  */
	MV_U32 memHandle;
	MV_U32 config;
	MV_U8 cryptoKeyLength;
	MV_U8 cryptoIvSize;
	MV_U8 cryptoBlockSize;
	MV_U8 digestSize;
	MV_U8 macKeyLength;
	MV_U8 ctrMode;
	MV_U32 count;
} MV_CESA_SA;

/* DMA list management */
typedef struct {
	MV_DMA_DESC *pDmaFirst;
	MV_DMA_DESC *pDmaLast;
} MV_CESA_DMA;

typedef struct {
	MV_U8 numFrag;
	MV_U8 nextFrag;
	int bufOffset;
	int cryptoSize;
	int macSize;
	int newDigestOffset;
	MV_U8 orgDigest[MV_CESA_MAX_DIGEST_SIZE];
} MV_CESA_FRAGS;

/* Request queue */
typedef struct {
	MV_U8 state;
	MV_U8 fragMode;
	MV_U8 fixOffset;
	MV_CESA_COMMAND *pCmd;
	MV_CESA_COMMAND *pOrgCmd;
	MV_BUF_INFO dmaDescBuf;
	MV_CESA_DMA dma[MV_CESA_MAX_REQ_FRAGS];
	MV_BUF_INFO cesaDescBuf;
	MV_CESA_DESC *pCesaDesc;
	MV_CESA_FRAGS frags;
	MV_32 use;
} MV_CESA_REQ;

/* SRAM map */
/* Total SRAM size calculation */
/*  SRAM size =
 *              MV_CESA_MAX_BUF_SIZE  +
 *              sizeof(MV_CESA_DESC)  +
 *              MV_CESA_MAX_IV_LENGTH +
 *              MV_CESA_MAX_IV_LENGTH +
 *              MV_CESA_MAX_DIGEST_SIZE +
 *              sizeof(MV_CESA_SRAM_SA)
 *            = 1600 + 32 + 16 + 16 + 24 + 80 + 280 (reserved) = 2048 bytes
 *            = 3200 + 32 + 16 + 16 + 24 + 80 + 728 (reserved) = 4096 bytes
 */
typedef struct {
	MV_U8 buf[MV_CESA_MAX_BUF_SIZE];
	MV_CESA_DESC desc;
	MV_U8 cryptoIV[MV_CESA_MAX_IV_LENGTH];
	MV_U8 tempCryptoIV[MV_CESA_MAX_IV_LENGTH];
	MV_U8 tempDigest[MV_CESA_MAX_DIGEST_SIZE + 4];
	MV_CESA_SRAM_SA sramSA;
} MV_CESA_SRAM_MAP;

typedef struct {
	MV_U32 openedCount;
	MV_U32 closedCount;
	MV_U32 fragCount;
	MV_U32 reqCount;
	MV_U32 maxReqCount;
	MV_U32 procCount;
	MV_U32 readyCount;
	MV_U32 notReadyCount;
	MV_U32 startCount;
#ifdef MV_CESA_CHAIN_MODE
	MV_U32 maxChainUsage;
#endif			/* MV_CESA_CHAIN_MODE */
} MV_CESA_STATS;

/* External variables */

extern MV_CESA_STATS cesaStats;
extern MV_CESA_FRAGS cesaFrags;
extern MV_CESA_SA **pCesaSAD;
extern MV_U32 cesaMaxSA;
extern MV_CESA_REQ *pCesaReqFirst[MV_CESA_CHANNELS];
extern MV_CESA_REQ *pCesaReqLast[MV_CESA_CHANNELS];
extern MV_CESA_REQ *pCesaReqEmpty[MV_CESA_CHANNELS];
extern MV_CESA_REQ *pCesaReqProcess[MV_CESA_CHANNELS];
extern int cesaQueueDepth[MV_CESA_CHANNELS];
extern int cesaReqResources[MV_CESA_CHANNELS];

#ifdef MV_CESA_CHAIN_MODE
	extern MV_U32 cesaChainLength[MV_CESA_CHANNELS];
#endif				/* MV_CESA_CHAIN_MODE */

extern MV_CESA_SRAM_MAP *cesaSramVirtPtr[MV_CESA_CHANNELS];

static INLINE MV_ULONG mvCesaVirtToPhys(MV_BUF_INFO *pBufInfo, void *pVirt)
{
	return (pBufInfo->bufPhysAddr + ((MV_U8 *)pVirt - pBufInfo->bufVirtPtr));
}

/* Additional DEBUG functions */
void mvCesaDebugSramSA(MV_CESA_SRAM_SA *pSramSA, int mode);
void mvCesaDebugCmd(MV_CESA_COMMAND *pCmd, int mode);
void mvCesaDebugDescriptor(MV_CESA_DESC *pDesc);

#ifdef __cplusplus
}
#endif

#endif /* __mvCesa_h__ */
