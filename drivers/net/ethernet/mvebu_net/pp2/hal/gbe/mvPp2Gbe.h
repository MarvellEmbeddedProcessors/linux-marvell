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

#ifndef __MV_PP2_GBE_H__
#define __MV_PP2_GBE_H__

#include "mvTypes.h"
#include "mvCommon.h"
#include "mv802_3.h"
#include "mvOs.h"

#include "mvPp2GbeRegs.h"
#include "bm/mvBm.h"
#include "gmac/mvEthGmacApi.h"
#include "common/mvPp2Common.h"
#include "prs/mvPp2PrsHw.h"

#define PP2_CPU_CODE_IS_RX_SPECIAL(cpu_code)		((cpu_code) & RI_CPU_CODE_RX_SPEC_VAL)
#define MV_AMPLIFY_FACTOR_MTU				(3)
#define MV_BIT_NUM_OF_BYTE				(8)
#define MV_WRR_WEIGHT_UNIT				(256)

static inline int mvPp2IsRxSpecial(MV_U16 parser_info)
{
	MV_U16 cpu_code = (parser_info & PP2_RX_CPU_CODE_MASK) >> PP2_RX_CPU_CODE_OFFS;

	return PP2_CPU_CODE_IS_RX_SPECIAL(cpu_code);
}

static inline int mvPp2RxBmPoolId(PP2_RX_DESC *rxDesc)
{
	return (rxDesc->status & PP2_RX_BM_POOL_ALL_MASK) >> PP2_RX_BM_POOL_ID_OFFS;
}

/************************** PPv2 HW Configuration ***********************/
typedef struct eth_pbuf {
	void	*osInfo;
	MV_ULONG physAddr;
	MV_U8	*pBuf;
	MV_U16	bytes;
	MV_U16	offset;
	MV_U8	pool;
	MV_U8	qset;
	MV_U8	grntd;
	MV_U8	reserved;
	MV_U16	vlanId;
} MV_ETH_PKT;

/************************** Port + Queue Control Structures ******************************/
typedef struct {
	char *pFirst;
	int lastDesc;
	int nextToProc;
	int descSize;
	MV_BUF_INFO descBuf;
} MV_PP2_QUEUE_CTRL;

#define MV_PP2_QUEUE_DESC_PTR(pQueueCtrl, descIdx)                 \
	((pQueueCtrl)->pFirst + ((descIdx) * (pQueueCtrl)->descSize))

#define MV_PP2_QUEUE_NEXT_DESC(pQueueCtrl, descIdx)  \
	(((descIdx) < (pQueueCtrl)->lastDesc) ? ((descIdx) + 1) : 0)

#define MV_PP2_QUEUE_PREV_DESC(pQueueCtrl, descIdx)  \
	(((descIdx) > 0) ? ((descIdx) - 1) : (pQueueCtrl)->lastDesc)

/*-------------------------------------------------------------------------------*/
/* TXQ */
typedef struct {
	MV_PP2_QUEUE_CTRL queueCtrl;
	int txq;
} MV_PP2_PHYS_TXQ_CTRL;

typedef struct {
	MV_PP2_QUEUE_CTRL queueCtrl;
	int cpu;
} MV_PP2_AGGR_TXQ_CTRL;

/* physical TXQs */
extern MV_PP2_PHYS_TXQ_CTRL *mvPp2PhysTxqs;

/* aggregated TXQs */
extern MV_PP2_AGGR_TXQ_CTRL *mvPp2AggrTxqs;

/*-------------------------------------------------------------------------------*/
/* RXQ */

typedef struct {
	MV_PP2_QUEUE_CTRL queueCtrl;
	int rxq;
	int port;
	int logicRxq;
} MV_PP2_PHYS_RXQ_CTRL;

/* physical RXQs */
extern MV_PP2_PHYS_RXQ_CTRL *mvPp2PhysRxqs;

/*-------------------------------------------------------------------------------*/
/* Port */
typedef struct {
	int portNo;
	MV_PP2_PHYS_RXQ_CTRL **pRxQueue;
	MV_PP2_PHYS_TXQ_CTRL **pTxQueue;
	int rxqNum;
	int txpNum;
	int txqNum;
	void *osHandle;
} MV_PP2_PORT_CTRL;

/* ports control */
extern MV_PP2_PORT_CTRL **mvPp2PortCtrl;

/*-------------------------------------------------------------------------------*/
/* HW data */
typedef struct {
	MV_U32 maxPort;
	MV_U32 maxTcont;
	MV_U32 aggrTxqSize;
	MV_U32 physTxqHwfSize;
	MV_U32 maxCPUs;
	MV_U32 portMask;
	MV_U32 cpuMask;
	MV_U32 pClk;
	MV_U32 tClk;
	MV_BOOL	iocc;
	MV_U16 ctrlModel;       /* Controller Model     */
	MV_U8  ctrlRev;         /* Controller Revision  */
} MV_PP2_HAL_DATA;

extern MV_PP2_HAL_DATA mvPp2HalData;

/************************** RXQ: Physical - Logical Mapping ******************************/
static INLINE int mvPp2PhysRxqToPort(int prxq)
{
	return mvPp2PhysRxqs[prxq].port;
}

static INLINE int mvPp2PhysRxqToLogicRxq(int prxq)
{
	return mvPp2PhysRxqs[prxq].logicRxq;
}

static INLINE int mvPp2LogicRxqToPhysRxq(int port, int rxq)
{
	if (mvPp2PortCtrl[port]->pRxQueue[rxq])
		return mvPp2PortCtrl[port]->pRxQueue[rxq]->rxq;

	return -1;
}


/************************** TXQ: Physical - Logical Mapping ******************************/
#ifdef MV_PON_EXIST

#define MV_PON_LOGIC_PORT_GET()			(mvPp2HalData.maxPort - 1)
#define MV_PON_PORT(p)				((p) == MV_PON_LOGIC_PORT_GET())
#define MV_PON_PHYS_PORT_GET()			MV_PON_PORT_ID
#define MV_PON_PHYS_PORT(p)			((p) == MV_PON_PORT_ID)
#define MV_PP2_TOTAL_TXP_NUM			(MV_ETH_MAX_TCONT + MV_ETH_MAX_PORTS - 1)
#define MV_PP2_TOTAL_PON_TXQ_NUM		(MV_ETH_MAX_TCONT * MV_ETH_MAX_TXQ)

#define MV_PPV2_PORT_PHYS(port)			((MV_PON_PORT(port)) ? MV_PON_PHYS_PORT_GET() : (port))
#define MV_PPV2_TXP_PHYS(port, txp)		((MV_PON_PORT(port)) ? txp : (MV_ETH_MAX_TCONT + port))
#define MV_PPV2_TXQ_PHYS(port, txp, txq)	((MV_PON_PORT(port)) ? txp * MV_ETH_MAX_TXQ + txq :\
											MV_PP2_TOTAL_PON_TXQ_NUM + port * MV_ETH_MAX_TXQ + txq)

#define MV_PPV2_TXQ_LOGICAL_PORT(physTxq)	((physTxq < MV_PP2_TOTAL_PON_TXQ_NUM) ? MV_PON_LOGIC_PORT_ID_GET() :\
							(physTxq - MV_PP2_TOTAL_PON_TXQ_NUM) / MV_ETH_MAX_TXQ)

#define MV_PPV2_TXQ_LOGICAL_TXP(physTxq)	((physTxq < MV_PP2_TOTAL_PON_TXQ_NUM) ? (physTxq / MV_ETH_MAX_TXQ) : 0)

#else /* Without PON */

#define MV_PON_PORT(p)				MV_FALSE
#define MV_PON_PHYS_PORT(p)			MV_FALSE
#define MV_PP2_TOTAL_TXP_NUM			(MV_ETH_MAX_PORTS)

#define MV_PPV2_PORT_PHYS(port)                 (port)
#define MV_PPV2_TXP_PHYS(port, txp)		(port)
#define MV_PPV2_TXQ_PHYS(port, txp, txq)	(port * MV_ETH_MAX_TXQ + txq)
#define MV_PPV2_TXQ_LOGICAL_PORT(physTxq)	(physTxq / MV_ETH_MAX_TXQ)
#define MV_PPV2_TXQ_LOGICAL_TXP(physTxq)	0

#endif /* MV_PON_EXIST */

#define MV_PPV2_TXQ_LOGICAL_TXQ(physTxq)	(physTxq % MV_ETH_MAX_TXQ)
#define MV_PP2_TXQ_TOTAL_NUM			(MV_PP2_TOTAL_TXP_NUM * MV_ETH_MAX_TXQ)

/************************** Data Path functions ******************************/
/* Set TXQ descriptors fields relevant for CSUM calculation */
static INLINE MV_U32 mvPp2TxqDescCsum(int l3_offs, int l3_proto, int ip_hdr_len, int l4_proto)
{
	MV_U32 command;

	/* fields: L3_offset, IP_hdrlen, L3_type, G_IPv4_chk, G_L4_chk, L4_type */
	/* required only for checksum calculation */
	command = (l3_offs << PP2_TX_L3_OFFSET_OFFS);
	command |= (ip_hdr_len << PP2_TX_IP_HLEN_OFFS);
	command |= PP2_TX_IP_CSUM_DISABLE_MASK;

	if (l3_proto == MV_16BIT_BE(MV_IP_TYPE)) {
		command &= ~PP2_TX_IP_CSUM_DISABLE_MASK; /* enable IP CSUM */
		command |= PP2_TX_L3_IP4;
	} else
		command |= PP2_TX_L3_IP6;

	if (l4_proto == MV_IP_PROTO_TCP)
		command |= (PP2_TX_L4_TCP | PP2_TX_L4_CSUM);
	else if (l4_proto == MV_IP_PROTO_UDP)
		command |= (PP2_TX_L4_UDP | PP2_TX_L4_CSUM);
	else
		command |= PP2_TX_L4_CSUM_NOT;

	return command;
}

static INLINE MV_VOID mvPp2GbeCpuInterruptsDisable(int port, int cpuMask)
{
	mvPp2WrReg(MV_PP2_ISR_ENABLE_REG(port), MV_PP2_ISR_DISABLE_INTERRUPT(cpuMask));
}

static INLINE MV_VOID mvPp2GbeCpuInterruptsEnable(int port, int cpuMask)
{
	mvPp2WrReg(MV_PP2_ISR_ENABLE_REG(port), MV_PP2_ISR_ENABLE_INTERRUPT(cpuMask));
}

/* Get Giga port handler */
static INLINE MV_PP2_PORT_CTRL *mvPp2PortHndlGet(int port)
{
	return mvPp2PortCtrl[port];
}

/* Get physical RX queue handler */
static INLINE MV_PP2_PHYS_RXQ_CTRL *mvPp2RxqHndlGet(int port, int rxq)
{
	return mvPp2PortCtrl[port]->pRxQueue[rxq];
}

/* Get physical TX queue handler */
static INLINE MV_PP2_PHYS_TXQ_CTRL *mvPp2TxqHndlGet(int port, int txp, int txq)
{
	MV_PP2_PORT_CTRL *pPortCtrl = mvPp2PortCtrl[port];

	return pPortCtrl->pTxQueue[txp * pPortCtrl->txqNum + txq];
}

/* Get Aggregated TX queue handler */
static INLINE MV_PP2_AGGR_TXQ_CTRL *mvPp2AggrTxqHndlGet(int cpu)
{
	return &mvPp2AggrTxqs[cpu];
}


/* Get pointer to next RX descriptor to be processed by SW */
static INLINE PP2_RX_DESC *mvPp2RxqNextDescGet(MV_PP2_PHYS_RXQ_CTRL *pRxq)
{
	PP2_RX_DESC	*pRxDesc;
	int		rxDesc = pRxq->queueCtrl.nextToProc;

	pRxq->queueCtrl.nextToProc = MV_PP2_QUEUE_NEXT_DESC(&(pRxq->queueCtrl), rxDesc);

	pRxDesc = ((PP2_RX_DESC *)pRxq->queueCtrl.pFirst) + rxDesc;

	return pRxDesc;
}

static INLINE PP2_RX_DESC *mvPp2RxqDescGet(MV_PP2_PHYS_RXQ_CTRL *pRxq)
{
	PP2_RX_DESC	*pRxDesc;

	pRxDesc = ((PP2_RX_DESC *)pRxq->queueCtrl.pFirst) + pRxq->queueCtrl.nextToProc;

	return pRxDesc;
}

#if defined(MV_CPU_BE)
/* Swap RX descriptor to be BE */
static INLINE void mvPPv2RxqDescSwap(PP2_RX_DESC *pRxDesc)
{
	pRxDesc->status = MV_BYTE_SWAP_32BIT(pRxDesc->status);
	pRxDesc->parserInfo = MV_BYTE_SWAP_16BIT(pRxDesc->parserInfo);
	pRxDesc->dataSize =  MV_BYTE_SWAP_16BIT(pRxDesc->dataSize);
	pRxDesc->bufPhysAddr = MV_BYTE_SWAP_32BIT(pRxDesc->bufPhysAddr);
	pRxDesc->bufCookie = MV_BYTE_SWAP_32BIT(pRxDesc->bufCookie);
	pRxDesc->gemPortIdPktColor = MV_BYTE_SWAP_16BIT(pRxDesc->gemPortIdPktColor);
	pRxDesc->csumL4 = MV_BYTE_SWAP_16BIT(pRxDesc->csumL4);
	pRxDesc->classifyInfo = MV_BYTE_SWAP_16BIT(pRxDesc->classifyInfo);
	pRxDesc->flowId = MV_BYTE_SWAP_32BIT(pRxDesc->flowId);
}

/* Swap TX descriptor to be BE */
static INLINE void mvPPv2TxqDescSwap(PP2_TX_DESC *pTxDesc)
{
	pTxDesc->command = MV_BYTE_SWAP_32BIT(pTxDesc->command);
	pTxDesc->dataSize = MV_BYTE_SWAP_16BIT(pTxDesc->dataSize);
	pTxDesc->bufPhysAddr = MV_BYTE_SWAP_32BIT(pTxDesc->bufPhysAddr);
	pTxDesc->bufCookie = MV_BYTE_SWAP_32BIT(pTxDesc->bufCookie);
	pTxDesc->hwCmd[0] = MV_BYTE_SWAP_32BIT(pTxDesc->hwCmd[0]);
	pTxDesc->hwCmd[1] = MV_BYTE_SWAP_32BIT(pTxDesc->hwCmd[1]);
	pTxDesc->hwCmd[2] = MV_BYTE_SWAP_32BIT(pTxDesc->hwCmd[2]);
}
#endif
/*-------------------------------------------------------------------------------*/
/* Get number of RX descriptors occupied by received packets */
static INLINE int mvPp2RxqBusyDescNumGet(int port, int rxq)
{
	MV_U32 regVal;
	int prxq = mvPp2LogicRxqToPhysRxq(port, rxq);

	if (prxq < 0)
		return 0;

	regVal = mvPp2RdReg(MV_PP2_RXQ_STATUS_REG(prxq));

	return (regVal & MV_PP2_RXQ_OCCUPIED_MASK) >> MV_PP2_RXQ_OCCUPIED_OFFSET;
}

/* Get number of free RX descriptors ready to received new packets */
static INLINE int mvPp2RxqFreeDescNumGet(int port, int rxq)
{
	MV_U32 regVal;
	int prxq = mvPp2LogicRxqToPhysRxq(port, rxq);

	if (prxq < 0)
		return 0;

	regVal = mvPp2RdReg(MV_PP2_RXQ_STATUS_REG(prxq));

	return (regVal & MV_PP2_RXQ_NON_OCCUPIED_MASK) >> MV_PP2_RXQ_NON_OCCUPIED_OFFSET;
}

/* Update HW with number of RX descriptors processed by SW:
 *    - decrement number of occupied descriptors
 *    - increment number of Non-occupied descriptors
 */
static INLINE void mvPp2RxqDescNumUpdate(int port, int rxq, int rx_done, int rx_filled)
{
	MV_U32 regVal;
	int prxq = mvPp2LogicRxqToPhysRxq(port, rxq);

	regVal = (rx_done << MV_PP2_RXQ_NUM_PROCESSED_OFFSET) | (rx_filled << MV_PP2_RXQ_NUM_NEW_OFFSET);
	mvPp2WrReg(MV_PP2_RXQ_STATUS_UPDATE_REG(prxq), regVal);
}

/* Add number of descriptors are ready to receive new packets */
static INLINE void mvPp2RxqNonOccupDescAdd(int port, int rxq, int rx_desc)
{
	MV_U32	regVal;
	int prxq = mvPp2LogicRxqToPhysRxq(port, rxq);

	regVal = (rx_desc << MV_PP2_RXQ_NUM_NEW_OFFSET);
	mvPp2WrReg(MV_PP2_RXQ_STATUS_UPDATE_REG(prxq), regVal);
}

/* Decrement number of processed descriptors */
static INLINE void mvPp2RxqOccupDescDec(int port, int rxq, int rx_done)
{
	MV_U32 regVal;
	int prxq = mvPp2LogicRxqToPhysRxq(port, rxq);

	regVal = (rx_done << MV_PP2_RXQ_NUM_PROCESSED_OFFSET);
	mvPp2WrReg(MV_PP2_RXQ_STATUS_UPDATE_REG(prxq), regVal);
}

/*-------------------------------------------------------------------------------*/
/*
   PPv2 new feature MAS 3.16
   reserved TXQ descriptorts allocation request
*/
static INLINE int mvPp2TxqAllocReservedDesc(int port, int txp, int txq, int num)
{
	MV_U32 regVal, ptxq;

	ptxq = MV_PPV2_TXQ_PHYS(port, txp, txq);
	regVal = (ptxq << MV_PP2_TXQ_RSVD_REQ_Q_OFFSET) | (num << MV_PP2_TXQ_RSVD_REQ_DESC_OFFSET);
	mvPp2WrReg(MV_PP2_TXQ_RSVD_REQ_REG, regVal);

	regVal = mvPp2RdReg(MV_PP2_TXQ_RSVD_RSLT_REG);

	return (regVal & MV_PP2_TXQ_RSVD_REQ_DESC_MASK) >> MV_PP2_TXQ_RSVD_RSLT_OFFSET;
}

/* Free all descriptors reserved */
static INLINE void mvPp2TxqFreeReservedDesc(int port, int txp, int txq)
{
	MV_U32 regVal, ptxq;

	ptxq = MV_PPV2_TXQ_PHYS(port, txp, txq);
	regVal = (ptxq << MV_PP2_TXQ_RSVD_CLR_Q_OFFSET);
	mvPp2WrReg(MV_PP2_TXQ_RSVD_CLR_REG, regVal);
}

/* Get number of TXQ descriptors waiting to be transmitted by HW */
static INLINE int mvPp2TxqPendDescNumGet(int port, int txp, int txq)
{
	MV_U32 regVal, ptxq;

	ptxq = MV_PPV2_TXQ_PHYS(port, txp, txq);
	mvPp2WrReg(MV_PP2_TXQ_NUM_REG, ptxq);

	regVal = mvPp2RdReg(MV_PP2_TXQ_PENDING_REG);

	return (regVal & MV_PP2_TXQ_PENDING_MASK) >> MV_PP2_TXQ_PENDING_OFFSET;
}

/*
   PPv2.1 new feature MAS 3.16
   Get number of SWF reserved descriptors
*/
static INLINE int mvPp2TxqPendRsrvdDescNumGet(int port, int txp, int txq)
{
	MV_U32 regVal, ptxq;

	ptxq = MV_PPV2_TXQ_PHYS(port, txp, txq);
	mvPp2WrReg(MV_PP2_TXQ_NUM_REG, ptxq);

	regVal = mvPp2RdReg(MV_PP2_TXQ_PENDING_REG);

	return (regVal & MV_PP2_TXQ_RSVD_DESC_OFFSET) >> MV_PP2_TXQ_RSVD_DESC_OFFSET;
}


/*
   PPv2.1 field removed, MAS 3.16
   Relevant only for ppv2.0
   Get number of TXQ HWF descriptors waiting to be transmitted by HW
*/
static INLINE int mvPp2TxqPendHwfDescNumGet(int port, int txp, int txq)
{
	MV_U32 regVal, ptxq;

	ptxq = MV_PPV2_TXQ_PHYS(port, txp, txq);
	mvPp2WrReg(MV_PP2_TXQ_NUM_REG, ptxq);

	regVal = mvPp2RdReg(MV_PP2_TXQ_PENDING_REG);

	return (regVal & MV_PP2_TXQ_HWF_PENDING_MASK) >> MV_PP2_TXQ_HWF_PENDING_OFFSET;
}

/* Get next aggregated TXQ descriptor */
static INLINE PP2_TX_DESC *mvPp2AggrTxqNextDescGet(MV_PP2_AGGR_TXQ_CTRL *pTxq)
{
	PP2_TX_DESC	*pTxDesc;
	int		txDesc = pTxq->queueCtrl.nextToProc;

	pTxq->queueCtrl.nextToProc = MV_PP2_QUEUE_NEXT_DESC(&(pTxq->queueCtrl), txDesc);

	pTxDesc = ((PP2_TX_DESC *)pTxq->queueCtrl.pFirst) + txDesc;

	return pTxDesc;
}

/* Get pointer to previous aggregated TX descriptor for rollback when needed */
static INLINE PP2_TX_DESC *mvPp2AggrTxqPrevDescGet(MV_PP2_AGGR_TXQ_CTRL *pTxq)
{
	int txDesc = pTxq->queueCtrl.nextToProc;

	pTxq->queueCtrl.nextToProc = MV_PP2_QUEUE_PREV_DESC(&(pTxq->queueCtrl), txDesc);

	return ((PP2_TX_DESC *) pTxq->queueCtrl.pFirst) + txDesc;
}

/* Get number of aggregated TXQ descriptors didn't send by HW to relevant physical TXQ */
static INLINE int mvPp2AggrTxqPendDescNumGet(int cpu)
{
	MV_U32 regVal;

	regVal = mvPp2RdReg(MV_PP2_AGGR_TXQ_STATUS_REG(cpu));

	return (regVal & MV_PP2_AGGR_TXQ_PENDING_MASK) >> MV_PP2_AGGR_TXQ_PENDING_OFFSET;
}

/* Update HW with number of Aggr-TX descriptors to be sent - user responsible for writing TXQ in TX descriptor */
static INLINE void mvPp2AggrTxqPendDescAdd(int pending)
{
	/* aggregated access - relevant TXQ number is written in TX descriptor */
	mvPp2WrReg(MV_PP2_AGGR_TXQ_UPDATE_REG, pending);
}

/* Get number of sent descriptors and descrement counter.
   Clear sent descriptor counter.
   Number of sent descriptors is returned. */
static INLINE int mvPp2TxqSentDescProc(int port, int txp, int txq)
{
	MV_U32  regVal, ptxq;

	ptxq = MV_PPV2_TXQ_PHYS(port, txp, txq);
	/* reading status reg also cause to reset transmitted counter */
	regVal = mvPp2RdReg(MV_PP2_TXQ_SENT_REG(ptxq));

	return (regVal & MV_PP2_TRANSMITTED_COUNT_MASK) >> MV_PP2_TRANSMITTED_COUNT_OFFSET;
}

/*-------------------------------------------------------------------------------*/

static INLINE MV_U32 mvPp2GbeIsrCauseRxTxGet(int port)
{
	MV_U32 val;

	if (MV_PON_PORT(port)) {
		val = mvPp2RdReg(MV_PP2_ISR_PON_RX_TX_CAUSE_REG);
		val &= (MV_PP2_PON_CAUSE_RXQ_OCCUP_DESC_ALL_MASK |
			MV_PP2_PON_CAUSE_TXP_OCCUP_DESC_ALL_MASK | MV_PP2_PON_CAUSE_MISC_SUM_MASK);
	} else {
		val = mvPp2RdReg(MV_PP2_ISR_RX_TX_CAUSE_REG(MV_PPV2_PORT_PHYS(port)));
		val &= (MV_PP2_CAUSE_RXQ_OCCUP_DESC_ALL_MASK |
			MV_PP2_CAUSE_TXQ_OCCUP_DESC_ALL_MASK | MV_PP2_CAUSE_MISC_SUM_MASK);
	}

	return val;
}

static INLINE MV_BOOL mvPp2GbeIsrCauseTxDoneIsSet(int port, MV_U32 causeRxTx)
{
	if (MV_PON_PORT(port))
		return (causeRxTx & MV_PP2_PON_CAUSE_TXP_OCCUP_DESC_ALL_MASK);

	return (causeRxTx & MV_PP2_CAUSE_TXQ_OCCUP_DESC_ALL_MASK);
}

static INLINE MV_U32 mvPp2GbeIsrCauseTxDoneOffset(int port, MV_U32 causeRxTx)
{
	if (MV_PON_PORT(port))
		return (causeRxTx & MV_PP2_PON_CAUSE_TXP_OCCUP_DESC_ALL_MASK);

	return (causeRxTx & MV_PP2_CAUSE_TXQ_OCCUP_DESC_ALL_MASK);
}
/************************** function declaration ******************************/
MV_STATUS 	mvPp2WinInit(MV_U32 dummy/*backward compability*/,   MV_UNIT_WIN_INFO *addrWinMap);
MV_STATUS 	mvPp2WinWrite(MV_U32 dummy/*backward compability*/,  MV_U32 winNum, MV_UNIT_WIN_INFO *pAddrDecWin);
MV_STATUS 	mvPp2WinRead(MV_U32 dummy/*backward compability*/,   MV_U32 winNum, MV_UNIT_WIN_INFO *pAddrDecWin);
MV_STATUS 	mvPp2WinEnable(MV_U32 dummy/*backward compability*/, MV_U32 winNum, MV_BOOL enable);

int mvPp2MaxCheck(int value, int limit, char *name);
int mvPp2PortCheck(int port);
int mvPp2TxpCheck(int port, int txp);
int mvPp2CpuCheck(int cpu);

static INLINE MV_ULONG pp2DescVirtToPhys(MV_PP2_QUEUE_CTRL *pQueueCtrl, MV_U8 *pDesc)
{
	return (pQueueCtrl->descBuf.bufPhysAddr + (pDesc - pQueueCtrl->descBuf.bufVirtPtr));
}

MV_U8 *mvPp2DescrMemoryAlloc(int descSize, MV_ULONG *pPhysAddr, MV_U32 *memHandle);
void mvPp2DescrMemoryFree(int descSize, MV_ULONG *pPhysAddr, MV_U8 *pVirt, MV_U32 *memHandle);

MV_STATUS mvPp2HalInit(MV_PP2_HAL_DATA *halData);
MV_VOID mvPp2HalDestroy(MV_VOID);

/* Add a mapping prxq <-> (port, lrxq) */
MV_STATUS mvPp2PhysRxqMapAdd(int prxq, int port, int lrxq);
/* Free the relevant physical rxq */
MV_STATUS mvPp2PhysRxqMapDel(int prxq);
MV_STATUS mvPp2PortLogicRxqMapDel(int port, int lrxq);

/* Allocate and initialize descriptors for RXQ */
MV_PP2_PHYS_RXQ_CTRL *mvPp2RxqInit(int port, int rxq, int descNum);
void mvPp2RxqDelete(int port, int queue);
void mvPp2RxqReset(int port, int queue);

/* Allocate and initialize all physical RXQs.
   This function must be called before any use of RXQ */
MV_STATUS mvPp2PhysRxqsAlloc(MV_VOID);

/* Destroy all physical RXQs */
MV_STATUS mvPp2PhysRxqsDestroy(MV_VOID);

MV_STATUS mvPp2RxqOffsetSet(int port, int rxq, int offset);

MV_STATUS mvPp2RxqPktsCoalSet(int port, int rxq, MV_U32 pkts);
int mvPp2RxqPktsCoalGet(int port, int rxq);

void mvPp2RxReset(int port);

void mvPp2TxqHwfSizeSet(int port, int txp, int txq, int hwfNum);

/* Allocate and initialize descriptors for TXQ */
MV_PP2_PHYS_TXQ_CTRL *mvPp2TxqInit(int port, int txp, int txq, int descNum, int hwfNum);

MV_STATUS mvPp2TxqDelete(int port, int txp, int txq);

/* Allocate and initialize all physical TXQs.
   This function must be called before any use of TXQ */
MV_STATUS mvPp2PhysTxqsAlloc(MV_VOID);

/* Destroy all physical TXQs */
MV_VOID mvPp2PhysTxqsDestroy(MV_VOID);

/* Allocate and initialize all aggregated TXQs.
   This function must be called before any use of aggregated TXQ */
MV_STATUS mvPp2AggrTxqsAlloc(int cpuNum);

/* Destroy all aggregated TXQs */
MV_VOID mvPp2AggrTxqsDestroy(MV_VOID);
MV_VOID mvPp2AggrTxqDelete(int cpu);
/* Initialize aggregated TXQ */
MV_PP2_AGGR_TXQ_CTRL *mvPp2AggrTxqInit(int cpu, int descNum);

MV_STATUS mvPp2TxDonePktsCoalSet(int port, int txp, int txq, MV_U32 pkts);
int mvPp2TxDonePktsCoalGet(int port, int txp, int txq);

void mvPp2TxpReset(int port, int txp);
void mvPp2TxqReset(int port, int txp, int txq);

MV_STATUS mvPp2TxqTempInit(int descNum, int hwfNum);
MV_VOID mvPp2TxqTempDelete(MV_VOID);

/* Allocate and initialize port structure
   Associate relevant TXQs for this port (predefined)
   Associate <numRxqs> RXQs for Port number <port>, starting from RXQ number <firstRxq>
   Note: mvPp2PortCtrl must be initialized, i.e. must call mvPp2HalInit before this function */
void *mvPp2PortInit(int port, int firstRxq, int numRxqs, void *osHandle);
void mvPp2PortDestroy(int port);

/* Low Level APIs */
MV_STATUS mvPp2RxqEnable(int port, int rxq, MV_BOOL en);
MV_STATUS mvPp2HwfTxqEnable(int port, int txp, int txq, MV_BOOL en);
MV_BOOL   mvPp2DisableCmdInProgress(void);
MV_STATUS mvPp2TxqDrainSet(int port, int txp, int txq, MV_BOOL en);
MV_STATUS mvPp2TxPortFifoFlush(int port, MV_BOOL en);
MV_STATUS mvPp2TxpEnable(int port, int txp);
MV_STATUS mvPp2TxpDisable(int port, int txp);
MV_STATUS mvPp2PortEnable(int port, MV_BOOL en);

/* High Level APIs */
MV_STATUS mvPp2PortIngressEnable(int port, MV_BOOL en);
MV_STATUS mvPp2PortEgressEnable(int port, MV_BOOL en);

MV_STATUS mvPp2BmPoolBufSizeSet(int pool, int bufsize);
MV_STATUS mvPp2RxqBmShortPoolSet(int port, int rxq, int shortPool);
MV_STATUS mvPp2RxqBmLongPoolSet(int port, int rxq, int longPool);
MV_STATUS mvPp2TxqBmShortPoolSet(int port, int txp, int txq, int shortPool);
MV_STATUS mvPp2TxqBmLongPoolSet(int port, int txp, int txq, int longPool);
MV_STATUS mvPp2PortHwfBmPoolSet(int port, int shortPool, int longPool);

MV_STATUS mvPp2MhSet(int port, MV_TAG_TYPE mh);

MV_STATUS mvPp2RxFifoInit(int portNum);

MV_STATUS mvPp2TxpMaxTxSizeSet(int port, int txp, int maxTxSize);
MV_STATUS mvPp2TxpMaxRateSet(int port, int txp);
MV_STATUS mvPp2TxqMaxRateSet(int port, int txp, int txq);
MV_STATUS mvPp2TxpRateSet(int port, int txp, int rate);
MV_STATUS mvPp2TxpBurstSet(int port, int txp, int burst);
MV_STATUS mvPp2TxqRateSet(int port, int txp, int txq, int rate);
MV_STATUS mvPp2TxqBurstSet(int port, int txp, int txq, int burst);
MV_STATUS mvPp2TxqFixPrioSet(int port, int txp, int txq);
MV_STATUS mvPp2TxqWrrPrioSet(int port, int txp, int txq, int weight);

/* Function for swithcing SWF to HWF */
MV_STATUS mvPp2FwdSwitchCtrl(MV_U32 flowId, int txq, int rxq, int usec);
int       mvPp2FwdSwitchStatus(int *hwState, int *usec);
void      mvPp2FwdSwitchRegs(void);

/*****************************/
/*      Interrupts API       */
/*****************************/
MV_VOID		mvPp2GbeCpuInterruptsDisable(int port, int cpuMask);
MV_VOID		mvPp2GbeCpuInterruptsEnable(int port, int cpuMask);
MV_STATUS	mvPp2RxqTimeCoalSet(int port, int rxq, MV_U32 uSec);
unsigned int	mvPp2RxqTimeCoalGet(int port, int rxq);
MV_STATUS	mvPp2GbeIsrRxqGroup(int port, int rxqNum);

/* unmask the current CPU's rx/tx interrupts                   *
 *  - rxq_mask: support rxq to cpu granularity                 *
 *  - isTxDoneIsr: if 0 then Tx Done interruptare not unmasked */
MV_STATUS mvPp2GbeIsrRxTxUnmask(int port, MV_U16 rxq_mask, int isTxDoneIsr);

/* mask the current CPU's rx/tx interrupts */
MV_STATUS mvPp2GbeIsrRxTxMask(int port);
/*****************************/

/*****************************/
/*      Debug functions      */
/*****************************/
MV_VOID mvPp2RxDmaRegsPrint(MV_VOID);
MV_VOID mvPp2DescMgrRegsRxPrint(MV_VOID);
MV_VOID mvPp2DescMgrRegsTxPrint(MV_VOID);
MV_VOID mvPp2AddressDecodeRegsPrint(MV_VOID);
void mvPp2IsrRegs(int port);
void mvPp2PhysRxqRegs(int rxq);
void mvPp2PortRxqRegs(int port, int rxq);
MV_VOID mvPp2RxqShow(int port, int rxq, int mode);
MV_VOID mvPp2TxqShow(int port, int txp, int txq, int mode);
MV_VOID mvPp2AggrTxqShow(int cpu, int mode);
void mvPp2PhysTxqRegs(int txq);
void mvPp2PortTxqRegs(int port, int txp, int txq);
void mvPp2AggrTxqRegs(int cpu);
void mvPp2TxRegs(void);
void mvPp2AddrDecodeRegs(void);
void mvPp2TxSchedRegs(int port, int txp);
void mvPp2BmPoolRegs(int pool);
void mvPp2V0DropCntrs(int port);
/* PPv2.1 MAS 3.20 - counters change */
void mvPp2V1DropCntrs(int port);
void mvPp2V1TxqDbgCntrs(int port, int txp, int txq);
void mvPp2V1RxqDbgCntrs(int port, int rxq);
void mvPp2RxFifoRegs(int port);
void mvPp2PortStatus(int port);
#endif /* MV_PP2_GBE_H */
