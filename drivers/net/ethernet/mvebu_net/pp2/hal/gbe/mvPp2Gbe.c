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
#include "mvOs.h"

#include "mvPp2Gbe.h"
#include "pp2/prs/mvPp2Prs.h"
#include "pp2/bm/mvBm.h"

#define MV_PP2_RXQ_FREE 	-1

#define TX_DISABLE_TIMEOUT_MSEC     1000
#define RX_DISABLE_TIMEOUT_MSEC     1000
#define TX_FIFO_EMPTY_TIMEOUT_MSEC  10000
#define PORT_DISABLE_WAIT_TCLOCKS   5000

/* physical TXQs */
MV_PP2_PHYS_TXQ_CTRL *mvPp2PhysTxqs;

/* aggregated TXQs */
MV_PP2_AGGR_TXQ_CTRL *mvPp2AggrTxqs;

/* physical RXQs */
MV_PP2_PHYS_RXQ_CTRL *mvPp2PhysRxqs;

/* ports control */
MV_PP2_PORT_CTRL **mvPp2PortCtrl;

/* HW data */
MV_PP2_HAL_DATA mvPp2HalData;

/*-------------------------------------------------------------------------------*/

int mvPp2MaxCheck(int value, int limit, char *name)
{
	if ((value < 0) || (value >= limit)) {
		mvOsPrintf("%s %d is out of range [0..%d]\n",
			name ? name : "value", value, (limit - 1));
		return 1;
	}
	return 0;
}

int mvPp2PortCheck(int port)
{
	return mvPp2MaxCheck(port, mvPp2HalData.maxPort, "port");
}

int mvPp2TxpCheck(int port, int txp)
{
	int txpMax = 1;

	if (mvPp2PortCheck(port))
		return 1;

	if (MV_PON_PORT(port))
		txpMax = mvPp2HalData.maxTcont;

	return mvPp2MaxCheck(txp, txpMax, "txp");
}

int mvPp2CpuCheck(int cpu)
{
	return mvPp2MaxCheck(cpu, mvPp2HalData.maxCPUs, "cpu");
}

int mvPp2EgressPort(int port, int txp)
{
	if (!MV_PON_PORT(port))
		return (MV_ETH_MAX_TCONT + port + txp);
	return txp;
}
/*-------------------------------------------------------------------------------*/
MV_STATUS mvPp2HalInit(MV_PP2_HAL_DATA *halData)
{
	int bytes, i;
	MV_STATUS status;

	mvPp2HalData = *halData;
	bytes = mvPp2HalData.maxPort * sizeof(MV_PP2_PORT_CTRL *);

	/* Allocate port data structures */
	mvPp2PortCtrl = mvOsMalloc(bytes);
	if (mvPp2PortCtrl == NULL) {
		mvOsPrintf("%s: Can't allocate %d bytes for %d ports\n", __func__,
			   mvPp2HalData.maxPort * sizeof(MV_PP2_PORT_CTRL), mvPp2HalData.maxPort);
		return MV_OUT_OF_CPU_MEM;
	}

	mvOsMemset(mvPp2PortCtrl, 0, bytes);

	/* Allocate physical TXQs */
	status = mvPp2PhysTxqsAlloc();
	if (status != MV_OK) {
		mvOsPrintf("%s: mvPp2PhysTxqsAlloc failed\n", __func__);
		return status;
	}

	/* Allocate aggregated TXQs */
	status = mvPp2AggrTxqsAlloc(mvPp2HalData.maxCPUs);
	if (status != MV_OK) {
		mvOsPrintf("%s: mvPp2AggrTxqsAlloc failed\n", __func__);
		return status;
	}

	/* Allocate physical RXQs */
	status = mvPp2PhysRxqsAlloc();
	if (status != MV_OK) {
		mvOsPrintf("%s: mvPp2PhysRxqsAlloc failed\n", __func__);
		return status;
	}

	mvBmInit();

	/* Rx Fifo Init */
	mvPp2RxFifoInit(mvPp2HalData.maxPort);

	/* Init all interrupt rxqs groups - each port has 0 rxqs */
	for (i = 0; i <= MV_PON_PORT_ID; i++)
		mvPp2GbeIsrRxqGroup(i, 0);

	MV_REG_WRITE(ETH_MNG_EXTENDED_GLOBAL_CTRL_REG, 0x27);

	/* Allow cache snoop when transmiting packets */
	if (mvPp2HalData.iocc)
		mvPp2WrReg(MV_PP2_TX_SNOOP_REG, 0x1);

	return MV_OK;
}

/*-------------------------------------------------------------------------------*/
MV_VOID mvPp2HalDestroy(MV_VOID)
{
	mvPp2PhysTxqsDestroy();
	mvPp2AggrTxqsDestroy();
	mvPp2PhysRxqsDestroy();
	mvOsFree(mvPp2PortCtrl);
	memset(&mvPp2HalData, 0, sizeof(mvPp2HalData));
}

/*******************************************************************************
* mvNetaDefaultsSet - Set defaults to the NETA port
*
* DESCRIPTION:
*       This
function sets default values to the NETA port.
*       1) Clears interrupt Cause and Mask registers.
*       2) Clears all MAC tables.
*       3) Sets defaults to all registers.
*       4) Resets RX and TX descriptor rings.
*       5) Resets PHY.
*
* INPUT:
*   int     portNo		- Port number.
*
* RETURN:   MV_STATUS
*               MV_OK - Success, Others - Failure
* NOTE:
*   This function updates all the port configurations except those set
*   initialy by the OsGlue by MV_NETA_PORT_INIT.
*   This function can be called after portDown to return the port settings
*   to defaults.
*******************************************************************************/
MV_STATUS mvPp2DefaultsSet(int port)
{
	MV_U32 regVal;
	int txp, queue, txPortNum, i;
	MV_PP2_PORT_CTRL *pPortCtrl = mvPp2PortHndlGet(port);

	if (!MV_PON_PORT(port))
		mvGmacDefaultsSet(port);

	/* avoid unused variable compilation warninig */
	regVal = 0;

	for (txp = 0; txp < pPortCtrl->txpNum; txp++) {
		/* Disable Legacy WRR, Disable EJP, Release from reset */
		txPortNum = mvPp2EgressPort(port, txp);

		mvPp2WrReg(MV_PP2_TXP_SCHED_PORT_INDEX_REG, txPortNum);

		mvPp2WrReg(MV_PP2_TXP_SCHED_CMD_1_REG, 0);
		/* Close bandwidth for all queues */
		for (queue = 0; queue < MV_ETH_MAX_TXQ; queue++)
			mvPp2WrReg(MV_PP2_TXQ_SCHED_TOKEN_CNTR_REG(MV_PPV2_TXQ_PHYS(port, txp, queue)), 0);

		/* Set refill period to 1 usec, refill tokens and bucket size to maximum */
		mvPp2WrReg(MV_PP2_TXP_SCHED_PERIOD_REG, mvPp2HalData.tClk / 1000000);
		mvPp2TxpMaxRateSet(port, txp);
	}
	/* Set MaximumLowLatencyPacketSize value to 256 */
	mvPp2WrReg(MV_PP2_RX_CTRL_REG(port), MV_PP2_RX_USE_PSEUDO_FOR_CSUM_MASK |
						MV_PP2_RX_LOW_LATENCY_PKT_SIZE_MASK(256));

	/* Enable Rx cache snoop */
	if (mvPp2HalData.iocc) {
		for (i = 0; i < pPortCtrl->rxqNum; i++) {
			queue = mvPp2LogicRxqToPhysRxq(port, i);
#ifdef CONFIG_MV_ETH_PP2_1
			regVal = mvPp2RdReg(MV_PP2_RXQ_CONFIG_REG(queue));
			regVal |= MV_PP2_SNOOP_PKT_SIZE_MASK | MV_PP2_SNOOP_BUF_HDR_MASK;
			mvPp2WrReg(MV_PP2_RXQ_CONFIG_REG(queue), regVal);
#else
			regVal = MV_PP2_V0_SNOOP_PKT_SIZE_MASK | MV_PP2_V0_SNOOP_BUF_HDR_MASK;
			mvPp2WrReg(MV_PP2_V0_RXQ_SNOOP_REG(queue), regVal);
#endif
		}
	}

	/* At default, mask all interrupts to all cpus */
	mvPp2GbeCpuInterruptsDisable(port, (1 << mvPp2HalData.maxCPUs) - 1);

	return MV_OK;

}

/*-------------------------------------------------------------------------------*/
/* Mapping */
/* Add a mapping prxq <-> (port, lrxq) */
MV_STATUS mvPp2PhysRxqMapAdd(int prxq, int port, int lrxq)
{
	MV_PP2_PORT_CTRL *pCtrl;

	if (mvPp2PortCheck(port)) {
		mvOsPrintf("Bad port number: %d\n", port);
		return MV_BAD_PARAM;
	}
	if (lrxq < 0 || lrxq > MV_ETH_MAX_RXQ) {
		mvOsPrintf("Bad logical RXQ number: %d\n", lrxq);
		return MV_BAD_PARAM;
	}
	if (mvPp2PhysRxqs == NULL)
		return MV_ERROR;
	if (prxq < 0 || prxq >= MV_ETH_RXQ_TOTAL_NUM)
		return MV_BAD_PARAM;
	if (mvPp2PhysRxqs[prxq].port != MV_PP2_RXQ_FREE || mvPp2PhysRxqs[prxq].logicRxq != MV_PP2_RXQ_FREE)
		return MV_BAD_PARAM;

	pCtrl = mvPp2PortCtrl[port];
	/* map prxq <- (port, lrxq) */
	if (pCtrl == NULL || pCtrl->pRxQueue == NULL)
		return MV_BAD_PARAM;
	if (lrxq < 0 || lrxq >= MV_ETH_MAX_RXQ)
		return MV_BAD_PARAM;
	if (pCtrl->rxqNum >= MV_ETH_MAX_RXQ)
		return MV_FAIL;

	pCtrl->pRxQueue[lrxq] = &mvPp2PhysRxqs[prxq];
	pCtrl->rxqNum++;

	/* map prxq -> (port, lrxq) */
	mvPp2PhysRxqs[prxq].port = port;
	mvPp2PhysRxqs[prxq].logicRxq = lrxq;

	return MV_OK;
}

/* Free the relevant physical rxq */
MV_STATUS mvPp2PhysRxqMapDel(int prxq)
{
	int port, lrxq;

	if (mvPp2PhysRxqs == NULL)
		return MV_ERROR;
	if (prxq < 0 || prxq >= MV_ETH_RXQ_TOTAL_NUM)
		return MV_BAD_PARAM;

	port = mvPp2PhysRxqs[prxq].port;
	lrxq = mvPp2PhysRxqs[prxq].logicRxq;
	mvPp2PhysRxqs[prxq].port = MV_PP2_RXQ_FREE;
	mvPp2PhysRxqs[prxq].logicRxq = MV_PP2_RXQ_FREE;

	if (port != MV_PP2_RXQ_FREE && lrxq != MV_PP2_RXQ_FREE &&
		mvPp2PortCtrl[port] && mvPp2PortCtrl[port]->pRxQueue[lrxq]) {
		mvPp2PortCtrl[port]->pRxQueue[lrxq] = NULL;
		mvPp2PortCtrl[port]->rxqNum--;
	}

	return MV_OK;
}

MV_STATUS mvPp2PortLogicRxqMapDel(int port, int lrxq)
{
	MV_PP2_PHYS_RXQ_CTRL *prxqCtrl;

	if (mvPp2PortCheck(port)) {
		mvOsPrintf("Bad port number: %d\n", port);
		return MV_BAD_PARAM;
	}
	if (lrxq < 0 || lrxq > MV_ETH_MAX_RXQ) {
		mvOsPrintf("Bad logical RXQ number: %d\n", lrxq);
		return MV_BAD_PARAM;
	}
	if (mvPp2PhysRxqs == NULL)
		return MV_ERROR;
	if (mvPp2PortCtrl[port] == NULL || mvPp2PortCtrl[port]->pRxQueue == NULL)
		return MV_BAD_PARAM;

	prxqCtrl = mvPp2PortCtrl[port]->pRxQueue[lrxq];
	mvPp2PortCtrl[port]->pRxQueue[lrxq] = NULL;
	if (prxqCtrl) {
		prxqCtrl->logicRxq = MV_PP2_RXQ_FREE;
		prxqCtrl->port = MV_PP2_RXQ_FREE;
		mvPp2PortCtrl[port]->rxqNum--;
	}

	return MV_OK;
}

/*-------------------------------------------------------------------------------*/
/* General descriptor management */
static void mvPp2DescRingReset(MV_PP2_QUEUE_CTRL *pQueueCtrl)
{
	int descrNum = (pQueueCtrl->lastDesc + 1);
	char *pDesc  = pQueueCtrl->pFirst;

	if (pDesc == NULL)
		return;

	/* reset ring of descriptors */
	mvOsMemset(pDesc, 0, (descrNum * MV_PP2_DESC_ALIGNED_SIZE));
	mvOsCacheFlush(NULL, pDesc, (descrNum * MV_PP2_DESC_ALIGNED_SIZE));
	pQueueCtrl->nextToProc = 0;
}

/* allocate descriptors */
MV_U8 *mvPp2DescrMemoryAlloc(int descSize, MV_ULONG *pPhysAddr, MV_U32 *memHandle)
{
	MV_U8 *pVirt;
#ifdef ETH_DESCR_UNCACHED
	pVirt = (MV_U8 *)mvOsIoUncachedMalloc(NULL, descSize, pPhysAddr, memHandle);
#else
	pVirt = (MV_U8 *)mvOsIoCachedMalloc(NULL, descSize, pPhysAddr, memHandle);
#endif /* ETH_DESCR_UNCACHED */
	if (pVirt)
		mvOsMemset(pVirt, 0, descSize);

	return pVirt;
}

void mvPp2DescrMemoryFree(int descSize, MV_ULONG *pPhysAddr, MV_U8 *pVirt, MV_U32 *memHandle)
{
#ifdef ETH_DESCR_UNCACHED
	mvOsIoUncachedFree(NULL, descSize, (MV_ULONG)pPhysAddr, pVirt, (MV_U32)memHandle);
#else
	mvOsIoCachedFree(NULL, descSize, (MV_ULONG)pPhysAddr, pVirt, (MV_U32)memHandle);
#endif /* ETH_DESCR_UNCACHED */
}

MV_STATUS mvPp2DescrCreate(MV_PP2_QUEUE_CTRL *qCtrl, int descNum)
{
	int descSize;

	/* Allocate memory for descriptors */
	descSize = ((descNum * MV_PP2_DESC_ALIGNED_SIZE) + MV_PP2_DESC_Q_ALIGN);
	qCtrl->descBuf.bufVirtPtr =
	    mvPp2DescrMemoryAlloc(descSize, &qCtrl->descBuf.bufPhysAddr, &qCtrl->descBuf.memHandle);

	qCtrl->descBuf.bufSize = descSize;
	qCtrl->descSize = MV_PP2_DESC_ALIGNED_SIZE;

	if (qCtrl->descBuf.bufVirtPtr == NULL) {
		mvOsPrintf("%s: Can't allocate %d bytes for %d descr\n", __func__, descSize, descNum);
		return MV_OUT_OF_CPU_MEM;
	}

	/* Make sure descriptor address is aligned */
	qCtrl->pFirst = (char *)MV_ALIGN_UP((MV_ULONG) qCtrl->descBuf.bufVirtPtr, MV_PP2_DESC_Q_ALIGN);

	qCtrl->lastDesc = (descNum - 1);
	return MV_OK;
}
/*-------------------------------------------------------------------------------*/
/* RXQ */
/* Allocate and initialize descriptors for RXQ */
MV_PP2_PHYS_RXQ_CTRL *mvPp2RxqInit(int port, int rxq, int descNum)
{
	MV_STATUS status;
	int prxq;
	MV_PP2_PHYS_RXQ_CTRL *pRxq;
	MV_PP2_QUEUE_CTRL *qCtrl;

	prxq = mvPp2LogicRxqToPhysRxq(port, rxq);
	if (prxq < 0) {
		mvOsPrintf("bad (port,rxq): (%d, %d), no mapping to physical rxq\n", port, rxq);
		return NULL;
	}
	pRxq = &mvPp2PhysRxqs[prxq];
	qCtrl = &pRxq->queueCtrl;

	/* Number of descriptors must be multiple of 16 */
	if (descNum % 16 != 0) {
		mvOsPrintf("Descriptor number %d, must be a multiple of 16\n", descNum);
		return NULL;
	}

	status = mvPp2DescrCreate(qCtrl, descNum);
	if (status != MV_OK)
		return NULL;

	mvPp2DescRingReset(qCtrl);

	/* zero occupied and non-occupied counters - direct access */
	mvPp2WrReg(MV_PP2_RXQ_STATUS_REG(prxq), 0);

	/* Set Rx descriptors queue starting address */
	/* indirect access */
	mvPp2WrReg(MV_PP2_RXQ_NUM_REG, prxq);
	mvPp2WrReg(MV_PP2_RXQ_DESC_ADDR_REG, pp2DescVirtToPhys(qCtrl, (MV_U8 *)qCtrl->pFirst));
	mvPp2WrReg(MV_PP2_RXQ_DESC_SIZE_REG, descNum);
	mvPp2WrReg(MV_PP2_RXQ_INDEX_REG, 0);

	return pRxq;
}

void mvPp2RxqDelete(int port, int rxq)
{
	int prxq;
	MV_PP2_PHYS_RXQ_CTRL *pRxq;
	MV_PP2_QUEUE_CTRL *pQueueCtrl;
	MV_BUF_INFO *pDescBuf;

	prxq = mvPp2LogicRxqToPhysRxq(port, rxq);

	if (prxq < 0) {
		mvOsPrintf("bad (port,rxq): (%d, %d), no mapping to physical rxq\n", port, rxq);
		return;
	}
	pRxq = &mvPp2PhysRxqs[prxq];
	pQueueCtrl = &pRxq->queueCtrl;
	pDescBuf = &pQueueCtrl->descBuf;

	mvPp2DescrMemoryFree(pDescBuf->bufSize, (MV_ULONG *)pDescBuf->bufPhysAddr,
				pDescBuf->bufVirtPtr, (MV_U32 *)pDescBuf->memHandle);
	mvOsMemset(pQueueCtrl, 0, sizeof(*pQueueCtrl));

	/* Clear Rx descriptors queue starting address, size and free descr number */
	mvPp2WrReg(MV_PP2_RXQ_STATUS_REG(prxq), 0);
	mvPp2WrReg(MV_PP2_RXQ_NUM_REG, prxq);
	mvPp2WrReg(MV_PP2_RXQ_DESC_ADDR_REG, 0);
	mvPp2WrReg(MV_PP2_RXQ_DESC_SIZE_REG, 0);
}

/* Allocate and initialize all physical RXQs.
   This function must be called before any use of RXQ */
MV_STATUS mvPp2PhysRxqsAlloc(MV_VOID)
{
	int i, bytes;

	bytes = MV_ETH_RXQ_TOTAL_NUM * sizeof(MV_PP2_PHYS_RXQ_CTRL);
	mvPp2PhysRxqs = mvOsMalloc(bytes);

	if (!mvPp2PhysRxqs) {
		mvOsPrintf("mvPp2 Can't allocate %d Bytes for %d RXQs controls\n",
			   bytes, MV_ETH_RXQ_TOTAL_NUM);
		return MV_OUT_OF_CPU_MEM;
	}

	memset(mvPp2PhysRxqs, 0, bytes);

	for (i = 0; i < MV_ETH_RXQ_TOTAL_NUM; i++) {
		mvPp2PhysRxqs[i].port = MV_PP2_RXQ_FREE;
		mvPp2PhysRxqs[i].logicRxq = MV_PP2_RXQ_FREE;
		mvPp2PhysRxqs[i].rxq = i;
	}
	return MV_OK;
}

/* Destroy all physical RXQs */
MV_STATUS mvPp2PhysRxqsDestroy(MV_VOID)
{
	mvOsFree(mvPp2PhysRxqs);
	return MV_OK;
}

/* Associate <num_rxqs> RXQs for Port number <port>, starting from RXQ number <firstRxq>
   Port and physical RXQs must be initialized.
   Opperation succeeds only if ALL RXQs can be added to this port - otherwise do nothing */
MV_STATUS mvPp2PortRxqsInit(int port, int firstRxq, int numRxqs)
{
	int i;
	MV_PP2_PORT_CTRL *pCtrl = mvPp2PortCtrl[port];

	if (firstRxq < 0 || firstRxq + numRxqs > MV_ETH_RXQ_TOTAL_NUM) {
		mvOsPrintf("%s: Bad RXQ parameters. first RXQ = %d,  num of RXQS = %d\n", __func__, firstRxq, numRxqs);
		return MV_BAD_PARAM;
	}
	/* Check resources */
	for (i = firstRxq; i < firstRxq + numRxqs; i++) {
		if (mvPp2PhysRxqs[i].port != MV_PP2_RXQ_FREE || mvPp2PhysRxqs[i].logicRxq != MV_PP2_RXQ_FREE) {
			mvOsPrintf("%s: Failed to init port#%d RXQ#%d: RXQ is already occupied\n", __func__, port, i);
			return MV_FAIL;
		}
	}

	/* Allocate logical RXQs */
	if (!pCtrl->pRxQueue)
		pCtrl->pRxQueue = mvOsMalloc(MV_ETH_MAX_RXQ * sizeof(MV_PP2_PHYS_RXQ_CTRL *));
	if (!pCtrl->pRxQueue)
		return MV_OUT_OF_CPU_MEM;

	mvOsMemset(pCtrl->pRxQueue, 0, (MV_ETH_MAX_RXQ * sizeof(MV_PP2_PHYS_RXQ_CTRL *)));

	/* Associate requested RXQs with port */
	for (i = firstRxq; i < firstRxq + numRxqs; i++)
		mvPp2PhysRxqMapAdd(i, port, i - firstRxq);

	return MV_OK;
}

MV_STATUS mvPp2RxqPktsCoalSet(int port, int rxq, MV_U32 pkts)
{
	MV_U32 regVal;
	int prxq = mvPp2LogicRxqToPhysRxq(port, rxq);

	regVal = (pkts << MV_PP2_OCCUPIED_THRESH_OFFSET) & MV_PP2_OCCUPIED_THRESH_MASK;
	mvPp2WrReg(MV_PP2_RXQ_NUM_REG, prxq);
	mvPp2WrReg(MV_PP2_RXQ_THRESH_REG, regVal);

	return MV_OK;
}

int mvPp2RxqPktsCoalGet(int port, int rxq)
{
	MV_U32 regVal;
	int prxq = mvPp2LogicRxqToPhysRxq(port, rxq);

	mvPp2WrReg(MV_PP2_RXQ_NUM_REG, prxq);
	regVal = mvPp2RdReg(MV_PP2_RXQ_THRESH_REG);

	return (regVal & MV_PP2_OCCUPIED_THRESH_MASK) >> MV_PP2_OCCUPIED_THRESH_OFFSET;
}

void mvPp2RxqReset(int port, int rxq)
{
	MV_PP2_PHYS_RXQ_CTRL *pRxq;
	int prxq;

	prxq = mvPp2LogicRxqToPhysRxq(port, rxq);
	pRxq = &mvPp2PhysRxqs[prxq];

	mvPp2DescRingReset(&pRxq->queueCtrl);
	/* zero occupied and non-occupied counters - direct access */
	mvPp2WrReg(MV_PP2_RXQ_STATUS_REG(prxq), 0);

	/* zero next descriptor index - indirect access */
	mvPp2WrReg(MV_PP2_RXQ_NUM_REG, prxq);
	mvPp2WrReg(MV_PP2_RXQ_INDEX_REG, 0);
}

/* Reset all RXQs */
void mvPp2RxReset(int port)
{
	MV_PP2_PORT_CTRL *pPortCtrl = mvPp2PortCtrl[port];
	int rxq;

	for (rxq = 0; rxq < pPortCtrl->rxqNum ; rxq++)
		mvPp2RxqReset(port, rxq);
}
/*-------------------------------------------------------------------------------*/
void mvPp2TxqHwfSizeSet(int port, int txp, int txq, int hwfNum)
{
	int ptxq = MV_PPV2_TXQ_PHYS(port, txp, txq);

	mvPp2WrReg(MV_PP2_TXQ_NUM_REG, ptxq);
	mvPp2WrReg(MV_PP2_TXQ_DESC_HWF_SIZE_REG, hwfNum & MV_PP2_TXQ_DESC_HWF_SIZE_MASK);
}

/* TXQ */
/* Allocate and initialize descriptors for TXQ */
MV_PP2_PHYS_TXQ_CTRL *mvPp2TxqInit(int port, int txp, int txq, int descNum, int hwfNum)
{
	MV_STATUS status;
	MV_U32 regVal;
	int desc, descPerTxq, ptxq = MV_PPV2_TXQ_PHYS(port, txp, txq);
	MV_PP2_PHYS_TXQ_CTRL *pTxq = &mvPp2PhysTxqs[ptxq];
	MV_PP2_QUEUE_CTRL *qCtrl = &pTxq->queueCtrl;

	status = mvPp2DescrCreate(qCtrl, descNum);
	if (status != MV_OK)
		return NULL;

	mvPp2DescRingReset(qCtrl);

	/* Set Tx descriptors queue starting address */
	/* indirect access */
	mvPp2WrReg(MV_PP2_TXQ_NUM_REG, ptxq);
	mvPp2WrReg(MV_PP2_TXQ_DESC_ADDR_REG, pp2DescVirtToPhys(qCtrl, (MV_U8 *)qCtrl->pFirst));
	mvPp2WrReg(MV_PP2_TXQ_DESC_SIZE_REG, descNum & MV_PP2_TXQ_DESC_SIZE_MASK);
	mvPp2WrReg(MV_PP2_TXQ_DESC_HWF_SIZE_REG, hwfNum & MV_PP2_TXQ_DESC_HWF_SIZE_MASK);
	mvPp2WrReg(MV_PP2_TXQ_INDEX_REG, 0);

	/* Sanity check: Pending descriptors counter and sent descriptors counter must be 0 */
	/* Pending counter read - indirect access */
	regVal = mvPp2RdReg(MV_PP2_TXQ_PENDING_REG);
	if (regVal != 0) {
		mvOsPrintf("port=%d, txp=%d txq=%d, ptxq=%d, pend=0x%08x - Pending packets\n",
			port, txp, txq, ptxq, regVal);
	}
	/* Sent descriptors counter - direct access */
	regVal = mvPp2RdReg(MV_PP2_TXQ_SENT_REG(ptxq));
	if (regVal != 0) {
		mvOsPrintf("port=%d, txp=%d txq=%d, ptxq=%d, sent=0x%08x - Sent packets\n",
			port, txp, txq, ptxq, regVal);
	}

	/* Calculate base address in prefetch buffer. We reserve 16 descriptors for each existing TXQ */
	/* TCONTS for PON port must be continious from 0 to mvPp2HalData.maxTcont */
	/* GBE ports assumed to be continious from 0 to (mvPp2HalData.maxPort - 1) */
	descPerTxq = 16;
	if (MV_PON_PORT(port))
		desc = ptxq * descPerTxq;
	else
		desc = (mvPp2HalData.maxTcont * MV_ETH_MAX_TXQ * descPerTxq) + (port * MV_ETH_MAX_TXQ * descPerTxq);

	mvPp2WrReg(MV_PP2_TXQ_PREF_BUF_REG, MV_PP2_PREF_BUF_PTR(desc) | MV_PP2_PREF_BUF_SIZE_16 |
				MV_PP2_PREF_BUF_THRESH(descPerTxq/2));

	mvPp2TxqMaxRateSet(port, txp, txq);

	return pTxq;
}

MV_STATUS mvPp2TxqDelete(int port, int txp, int txq)
{
	int ptxq = MV_PPV2_TXQ_PHYS(port, txp, txq);
	MV_PP2_QUEUE_CTRL *pQueueCtrl = &mvPp2PhysTxqs[ptxq].queueCtrl;
	MV_BUF_INFO *pDescBuf = &pQueueCtrl->descBuf;

	mvPp2DescrMemoryFree(pDescBuf->bufSize, (MV_ULONG *)pDescBuf->bufPhysAddr,
				pDescBuf->bufVirtPtr, (MV_U32 *)pDescBuf->memHandle);

	mvOsMemset(pQueueCtrl, 0, sizeof(*pQueueCtrl));

	/* Set minimum bandwidth for disabled TXQs */
	mvPp2WrReg(MV_PP2_TXQ_SCHED_TOKEN_CNTR_REG(ptxq), 0);

	/* Set Tx descriptors queue starting address and size */
	mvPp2WrReg(MV_PP2_TXQ_NUM_REG, ptxq);

	mvPp2WrReg(MV_PP2_TXQ_DESC_ADDR_REG, 0);
	mvPp2WrReg(MV_PP2_TXQ_DESC_SIZE_REG, 0);
	mvPp2WrReg(MV_PP2_TXQ_DESC_HWF_SIZE_REG, 0);

	return MV_OK;
}

/* Allocate and initialize all physical TXQs.
   This function must be called before any use of TXQ */
MV_STATUS mvPp2PhysTxqsAlloc(void)
{
	int i, bytes;

	/* Alloc one extra element for temporary TXQ */
	bytes = (MV_PP2_TXQ_TOTAL_NUM + 1) * sizeof(MV_PP2_PHYS_TXQ_CTRL);

	mvPp2PhysTxqs = mvOsMalloc(bytes);

	if (!mvPp2PhysTxqs) {
		mvOsPrintf("mvPp2 Can't allocate %d Bytes for %d TXQs control\n",
			   bytes, MV_PP2_TXQ_TOTAL_NUM);
		return MV_OUT_OF_CPU_MEM;
	}

	memset(mvPp2PhysTxqs, 0, bytes);

	for (i = 0; i < (MV_PP2_TXQ_TOTAL_NUM + 1); i++)
		mvPp2PhysTxqs[i].txq = i;

	return MV_OK;
}

/* Destroy all physical TXQs */
MV_VOID mvPp2PhysTxqsDestroy(MV_VOID)
{
	mvOsFree(mvPp2PhysTxqs);
}

/* Associate TXQs for this port
   Physical TXQS must be initialized (by using mvPp2PhysTxqsAlloc)
   Notice that TXQ mapping is predefined */
MV_STATUS mvPp2PortTxqsInit(int port)
{
	int txp, txq, ptxq;
	MV_PP2_PORT_CTRL *pCtrl = mvPp2PortCtrl[port];

	if (!pCtrl->pTxQueue)
		pCtrl->pTxQueue = mvOsMalloc(pCtrl->txqNum * pCtrl->txpNum * sizeof(MV_PP2_PHYS_TXQ_CTRL *));
	if (!pCtrl->pTxQueue)
		return MV_OUT_OF_CPU_MEM;

	for (txp = 0; txp < pCtrl->txpNum; txp++) {
		for (txq = 0; txq < pCtrl->txqNum; txq++) {
			ptxq = MV_PPV2_TXQ_PHYS(port, txp, txq);
			pCtrl->pTxQueue[txp * CONFIG_MV_ETH_TXQ + txq] = &mvPp2PhysTxqs[ptxq];
		}
	}

	return MV_OK;
}

/* Allocate and initialize descriptors for Aggr TXQ */
MV_STATUS mvPp2AggrTxqDescInit(MV_PP2_AGGR_TXQ_CTRL *txqCtrl, int descNum, int cpu)
{
	MV_STATUS status;
	MV_PP2_QUEUE_CTRL *qCtrl = &txqCtrl->queueCtrl;

	status = mvPp2DescrCreate(qCtrl, descNum);
	if (status != MV_OK)
		return status;

	mvPp2DescRingReset(qCtrl);

	/* Aggr TXQ no reset WA */
	qCtrl->nextToProc = mvPp2RdReg(MV_PP2_AGGR_TXQ_INDEX_REG(cpu));

	/* Set Tx descriptors queue starting address */
	/* indirect access */
	mvPp2WrReg(MV_PP2_AGGR_TXQ_DESC_ADDR_REG(cpu), pp2DescVirtToPhys(qCtrl, (MV_U8 *)qCtrl->pFirst));
	mvPp2WrReg(MV_PP2_AGGR_TXQ_DESC_SIZE_REG(cpu), descNum & MV_PP2_AGGR_TXQ_DESC_SIZE_MASK);
	/* RO - mvPp2WrReg(MV_PP2_AGGR_TXQ_INDEX_REG(cpu), 0); */

	return MV_OK;
}

/* Allocate all aggregated TXQs.
   This function must be called before any use of aggregated TXQ */
MV_STATUS mvPp2AggrTxqsAlloc(int cpuNum)
{
	/* Alloc one extra element for temporary TXQ */
	int bytes = cpuNum * sizeof(MV_PP2_PHYS_TXQ_CTRL);

	mvPp2AggrTxqs = mvOsMalloc(bytes);

	if (!mvPp2AggrTxqs) {
		mvOsPrintf("mvPp2 Can't allocate %d Bytes for %d aggr TXQs control\n", bytes, cpuNum);
		return MV_OUT_OF_CPU_MEM;
	}

	memset(mvPp2AggrTxqs, 0, bytes);

	return MV_OK;
}

/* release all aggregated TXQs */
MV_VOID mvPp2AggrTxqsDestroy(MV_VOID)
{
	mvOsFree(mvPp2AggrTxqs);
}


/* Destroy all aggregated TXQs */
MV_VOID mvPp2AggrTxqDelete(int cpu)
{
	MV_PP2_AGGR_TXQ_CTRL *pTxqCtrl = &mvPp2AggrTxqs[cpu];
	MV_PP2_QUEUE_CTRL *pQueuCtrl = &pTxqCtrl->queueCtrl;
	MV_BUF_INFO *pDescBuf = &pQueuCtrl->descBuf;

	mvPp2DescrMemoryFree(pDescBuf->bufSize, (MV_ULONG *)pDescBuf->bufPhysAddr,
				pDescBuf->bufVirtPtr, (MV_U32 *)pDescBuf->memHandle);

	mvOsMemset(pQueuCtrl, 0, sizeof(*pQueuCtrl));
}

/* Initialize aggregated TXQ */
MV_PP2_AGGR_TXQ_CTRL *mvPp2AggrTxqInit(int cpu, int descNum)
{
	MV_STATUS status;

	if (!mvPp2AggrTxqs)
		return NULL;

	/* Number of descriptors must be multiple of 16 */
	if (descNum % 16 != 0) {
		mvOsPrintf("Descriptor number %d, must be a multiple of 16\n", descNum);
		return NULL;
	}

	mvPp2AggrTxqs[cpu].cpu = cpu;
	status = mvPp2AggrTxqDescInit(&mvPp2AggrTxqs[cpu], descNum, cpu);
	if (status != MV_OK) {
		mvOsPrintf("mvPp2 failed to initialize descriptor ring for aggr TXQ %d\n", cpu);
		return NULL;
	}

	return &mvPp2AggrTxqs[cpu];
}

MV_STATUS mvPp2TxDonePktsCoalSet(int port, int txp, int txq, MV_U32 pkts)
{
	MV_U32 regVal;
	int ptxq = MV_PPV2_TXQ_PHYS(port, txp, txq);

	regVal = (pkts << MV_PP2_TRANSMITTED_THRESH_OFFSET) & MV_PP2_TRANSMITTED_THRESH_MASK;
	mvPp2WrReg(MV_PP2_TXQ_NUM_REG, ptxq);
	mvPp2WrReg(MV_PP2_TXQ_THRESH_REG, regVal);

	return MV_OK;
}

int mvPp2TxDonePktsCoalGet(int port, int txp, int txq)
{
	MV_U32 regVal;
	int ptxq = MV_PPV2_TXQ_PHYS(port, txp, txq);

	mvPp2WrReg(MV_PP2_TXQ_NUM_REG, ptxq);
	regVal = mvPp2RdReg(MV_PP2_TXQ_THRESH_REG);

	return (regVal & MV_PP2_TRANSMITTED_THRESH_MASK) >> MV_PP2_TRANSMITTED_THRESH_OFFSET;
}

void mvPp2TxqReset(int port, int txp, int txq)
{
	int ptxq;
	MV_PP2_PHYS_TXQ_CTRL *pTxq;

	ptxq = MV_PPV2_TXQ_PHYS(port, txp, txq);
	pTxq = &mvPp2PhysTxqs[ptxq];

	mvPp2DescRingReset(&pTxq->queueCtrl);
	mvPp2WrReg(MV_PP2_TXQ_NUM_REG, ptxq);
	mvPp2WrReg(MV_PP2_TXQ_INDEX_REG, 0);
}

/* Reset all TXQs */
void mvPp2TxpReset(int port, int txp)
{
	int txq;
	MV_PP2_PORT_CTRL *pPortCtrl = mvPp2PortCtrl[port];

	for (txq = 0; txq < pPortCtrl->txqNum; txq++)
		mvPp2TxqReset(port, txp, txq);
}

/* Allocate and initialize descriptors for temporary TXQ */
MV_STATUS mvPp2TxqTempInit(int descNum, int hwfNum)
{
	MV_STATUS status;
	int ptxq = MV_PP2_TXQ_TOTAL_NUM;
	MV_PP2_PHYS_TXQ_CTRL *pTxq = &mvPp2PhysTxqs[ptxq];
	MV_PP2_QUEUE_CTRL *qCtrl = &pTxq->queueCtrl;

	status = mvPp2DescrCreate(qCtrl, descNum);
	if (status != MV_OK)
		return MV_FAIL;

	mvPp2DescRingReset(qCtrl);

	/* Set Tx descriptors queue starting address */
	/* indirect access */
	mvPp2WrReg(MV_PP2_TXQ_NUM_REG, ptxq);
	mvPp2WrReg(MV_PP2_TXQ_DESC_ADDR_REG, pp2DescVirtToPhys(qCtrl, (MV_U8 *)qCtrl->pFirst));
	mvPp2WrReg(MV_PP2_TXQ_DESC_SIZE_REG, descNum & MV_PP2_TXQ_DESC_SIZE_MASK);
	mvPp2WrReg(MV_PP2_TXQ_DESC_HWF_SIZE_REG, hwfNum & MV_PP2_TXQ_DESC_HWF_SIZE_MASK);
	mvPp2WrReg(MV_PP2_TXQ_INDEX_REG, 0);

	mvPp2WrReg(MV_PP2_TXQ_PREF_BUF_REG, MV_PP2_PREF_BUF_PTR(ptxq * 4) | MV_PP2_PREF_BUF_SIZE_4 | MV_PP2_PREF_BUF_THRESH(2));

	return MV_OK;
}

void mvPp2TxqTempDelete(void)
{
	int ptxq = MV_PP2_TXQ_TOTAL_NUM;

	MV_PP2_PHYS_TXQ_CTRL *pTxq = &mvPp2PhysTxqs[ptxq];
	MV_PP2_QUEUE_CTRL *qCtrl = &pTxq->queueCtrl;
	MV_BUF_INFO *pDescBuf = &qCtrl->descBuf;
	mvPp2DescrMemoryFree(pDescBuf->bufSize, (MV_ULONG *)pDescBuf->bufPhysAddr,
				pDescBuf->bufVirtPtr, (MV_U32 *)pDescBuf->memHandle);

	mvOsMemset(qCtrl, 0, sizeof(*qCtrl));
}
/*-------------------------------------------------------------------------------*/
/* Port */
/* Allocate and initialize port structure
   Alocate an initialize TXQs for this port
   Associate <numRxqs> RXQs for Port number <port>, starting from RXQ number <firstRxq>
   Note: mvPp2PortCtrl must be initialized, i.e. must call mvPp2HalInit before this function */
void *mvPp2PortInit(int port, int firstRxq, int numRxqs, void *osHandle)
{
	MV_STATUS status;
	MV_PP2_PORT_CTRL *pCtrl;

	if (mvPp2PortCheck(port)) {
		mvOsPrintf("%s: Bad port number: %d\n", __func__, port);
		return NULL;
	}
	if (!mvPp2PortCtrl) {
		mvOsPrintf("%s: Port control is uninitialized\n", __func__);
		return NULL;
	}

	if (!mvPp2PortCtrl[port])
		mvPp2PortCtrl[port] = (MV_PP2_PORT_CTRL *)mvOsMalloc(sizeof(MV_PP2_PORT_CTRL));
	if (!mvPp2PortCtrl[port]) {
		mvOsPrintf("%s: Could not allocate %d bytes for port structure\n", __func__, sizeof(MV_PP2_PORT_CTRL));
		return NULL;
	}

	mvOsMemset(mvPp2PortCtrl[port], 0, sizeof(MV_PP2_PORT_CTRL));

	pCtrl = mvPp2PortCtrl[port];
	pCtrl->portNo = port;
	pCtrl->osHandle = osHandle;

	/* associate TXQs to this port */
#ifdef CONFIG_MV_INCLUDE_PON
	pCtrl->txpNum = MV_PON_PORT(port) ? mvPp2HalData.maxTcont : 1;
#else
	pCtrl->txpNum = 1;
#endif
	pCtrl->txqNum = CONFIG_MV_ETH_TXQ;
	status = mvPp2PortTxqsInit(port);
	if (status != MV_OK)
		return NULL;

	/* associate RXQs to this port */
	pCtrl->rxqNum = 0;
	status = mvPp2PortRxqsInit(port, firstRxq, numRxqs);
	if (status != MV_OK)
		return NULL;

	/* associate interrupt from relevant rxqs group to this port */
	status = mvPp2GbeIsrRxqGroup(port, numRxqs);
	if (status != MV_OK)
		return NULL;

	/* Disable port */
	mvPp2PortIngressEnable(port, MV_FALSE);
	mvPp2PortEgressEnable(port, MV_FALSE);
	mvPp2PortEnable(port, MV_FALSE);

	mvPp2DefaultsSet(port);

	return pCtrl;
}

void mvPp2PortDestroy(int portNo)
{
	MV_PP2_PORT_CTRL *pPortCtrl = mvPp2PortHndlGet(portNo);

	if (pPortCtrl->pTxQueue)
		mvOsFree(pPortCtrl->pTxQueue);

	if (pPortCtrl->pRxQueue)
		mvOsFree(pPortCtrl->pRxQueue);

	if (pPortCtrl)
		mvOsFree(pPortCtrl);

	mvPp2PortCtrl[portNo] = NULL;
}

/*******************************************************************************
* mvPp2PortEgressEnable
*
* DESCRIPTION:
*	Disable fetch descriptors from initialized TXQs
*
*       Note: Effects TXQs initialized prior to calling this function.
*
* INPUT:
*	int     portNo		- Port number.
*
* RETURN:   MV_STATUS
*               MV_OK - Success, Others - Failure.
*
*******************************************************************************/
MV_STATUS mvPp2PortEgressEnable(int port, MV_BOOL en)
{
	int	         txp;
	MV_PP2_PORT_CTRL *pPortCtrl = mvPp2PortHndlGet(port);

	/* Disable all physical TXQs */
	for (txp = 0; txp < pPortCtrl->txpNum; txp++) {
		if (en)
			mvPp2TxpEnable(port, txp);
		else
			mvPp2TxpDisable(port, txp);
	}
	return MV_OK;
}
/*-------------------------------------------------------------------------------*/

MV_STATUS mvPp2PortEnable(int port, MV_BOOL en)
{
	if (!MV_PON_PORT(port)) {
		/* Enable port */
		if (en)
			mvGmacPortEnable(port);
		else
			mvGmacPortDisable(port);
	}
	return MV_OK;
}
/*-------------------------------------------------------------------------------*/

/* BM */
MV_STATUS mvPp2BmPoolBufSizeSet(int pool, int bufsize)
{
	MV_U32 regVal;

	mvBmPoolBufSizeSet(pool, bufsize);
	regVal = MV_ALIGN_UP(bufsize, 1 << MV_PP2_POOL_BUF_SIZE_OFFSET);
	mvPp2WrReg(MV_PP2_POOL_BUF_SIZE_REG(pool), regVal);

	return MV_OK;
}

#ifdef CONFIG_MV_ETH_PP2_1

/*******************************************************************************
* mvPp2PortIngressEnable
*
* DESCRIPTION:
*	Enable/Disable receive packets to RXQs for SWF and receive packets to TXQs for HWF.
*
*       Note: Effects only Rx and Tx queues initialized prior to calling this function.
*
* INPUT:
*	int     portNo		- Port number.
*
* RETURN:   MV_STATUS
*           MV_OK - Success, Others - Failure.
*
*******************************************************************************/
MV_STATUS mvPp2PortIngressEnable(int port, MV_BOOL en)
{
	int txp, txq, rxq;
	MV_PP2_PORT_CTRL *pPortCtrl = mvPp2PortHndlGet(port);

	/* Enable all initialized RXQs */
	for (rxq = 0; rxq < pPortCtrl->rxqNum ; rxq++) {
		if (pPortCtrl->pRxQueue[rxq] != NULL)
			mvPp2RxqEnable(port, rxq, en);
	}

	/* Enable HWF for all initialized TXQs. */
	for (txp = 0; txp < pPortCtrl->txpNum; txp++) {
		for (txq = 0; txq < pPortCtrl->txqNum; txq++) {
			if (pPortCtrl->pTxQueue[txp * pPortCtrl->txqNum + txq] != NULL)
				mvPp2HwfTxqEnable(port, txp, txq, en);
		}
	}
	return MV_OK;
}

MV_STATUS mvPp2RxqOffsetSet(int port, int rxq, int offset)
{
	MV_U32 regVal;
	int prxq = mvPp2LogicRxqToPhysRxq(port, rxq);

	if (offset % 32 != 0) {
		mvOsPrintf("%s: offset must be in units of 32\n", __func__);
		return MV_BAD_PARAM;
	}

	/* convert offset from bytes to units of 32 bytes */
	offset = offset >> 5;

	regVal = mvPp2RdReg(MV_PP2_RXQ_CONFIG_REG(prxq));
	regVal &= ~MV_PP2_RXQ_PACKET_OFFSET_MASK;

	/* Offset is in */
	regVal |= ((offset << MV_PP2_RXQ_PACKET_OFFSET_OFFS) & MV_PP2_RXQ_PACKET_OFFSET_MASK);

	mvPp2WrReg(MV_PP2_RXQ_CONFIG_REG(prxq), regVal);

	return MV_OK;
}

MV_STATUS mvPp2RxqBmLongPoolSet(int port, int rxq, int longPool)
{
	MV_U32 regVal = 0;
	int prxq = mvPp2LogicRxqToPhysRxq(port, rxq);

	regVal = mvPp2RdReg(MV_PP2_RXQ_CONFIG_REG(prxq));
	regVal &= ~MV_PP2_RXQ_POOL_LONG_MASK;
	regVal |= ((longPool << MV_PP2_RXQ_POOL_LONG_OFFS) & MV_PP2_RXQ_POOL_LONG_MASK);

	mvPp2WrReg(MV_PP2_RXQ_CONFIG_REG(prxq), regVal);

	/* Update default BM priority rule */
	mvBmRxqToQsetLongClean(prxq);
	mvBmRxqToQsetLongSet(prxq, mvBmDefaultQsetNumGet(longPool));

	return MV_OK;
}

MV_STATUS mvPp2RxqBmShortPoolSet(int port, int rxq, int shortPool)
{
	MV_U32 regVal = 0;
	int prxq = mvPp2LogicRxqToPhysRxq(port, rxq);

	regVal = mvPp2RdReg(MV_PP2_RXQ_CONFIG_REG(prxq));
	regVal &= ~MV_PP2_RXQ_POOL_SHORT_MASK;
	regVal |= ((shortPool << MV_PP2_RXQ_POOL_SHORT_OFFS) & MV_PP2_RXQ_POOL_SHORT_MASK);

	mvPp2WrReg(MV_PP2_RXQ_CONFIG_REG(prxq), regVal);

	/* Update default BM priority rule */
	mvBmRxqToQsetShortClean(prxq);
	mvBmRxqToQsetShortSet(prxq, mvBmDefaultQsetNumGet(shortPool));

	return MV_OK;
}

MV_STATUS mvPp2TxqBmShortPoolSet(int port, int txp, int txq, int shortPool)
{
	MV_U32 regVal = 0;
	int ptxq = MV_PPV2_TXQ_PHYS(port, txp, txq);

	regVal = mvPp2RdReg(MV_PP2_HWF_TXQ_CONFIG_REG(ptxq));
	regVal &= ~MV_PP2_HWF_TXQ_POOL_SHORT_MASK;

	regVal |= ((shortPool << MV_PP2_HWF_TXQ_POOL_SHORT_OFFS) & MV_PP2_HWF_TXQ_POOL_SHORT_MASK);

	mvPp2WrReg(MV_PP2_HWF_TXQ_CONFIG_REG(ptxq), regVal);

	mvBmTxqToQsetShortClean(ptxq);
	mvBmTxqToQsetShortSet(ptxq, mvBmDefaultQsetNumGet(shortPool));

	return MV_OK;
}

MV_STATUS mvPp2TxqBmLongPoolSet(int port, int txp, int txq, int longPool)
{
	MV_U32 regVal = 0;
	int ptxq = MV_PPV2_TXQ_PHYS(port, txp, txq);

	regVal = mvPp2RdReg(MV_PP2_HWF_TXQ_CONFIG_REG(ptxq));
	regVal &= ~MV_PP2_HWF_TXQ_POOL_LONG_MASK;

	regVal |= ((longPool << MV_PP2_HWF_TXQ_POOL_LONG_OFFS) & MV_PP2_HWF_TXQ_POOL_LONG_MASK);

	mvPp2WrReg(MV_PP2_HWF_TXQ_CONFIG_REG(ptxq), regVal);

	mvBmTxqToQsetLongClean(ptxq);
	mvBmTxqToQsetLongSet(ptxq, mvBmDefaultQsetNumGet(longPool));

	return MV_OK;
}

#else

MV_STATUS mvPp2PortIngressEnable(int port, MV_BOOL en)
{
	if (en)
		mvPrsMacDropAllSet(port, 0);
	else
		mvPrsMacDropAllSet(port, 1);

	return MV_OK;
}

MV_STATUS mvPp2RxqOffsetSet(int port, int rxq, int offset)
{
	MV_U32 regVal;
	int prxq = mvPp2LogicRxqToPhysRxq(port, rxq);

	if (offset % 32 != 0) {
		mvOsPrintf("%s: offset must be in units of 32\n", __func__);
		return MV_BAD_PARAM;
	}

	/* convert offset from bytes to units of 32 bytes */
	offset = offset >> 5;

	regVal = mvPp2RdReg(MV_PP2_V0_RXQ_CONFIG_REG(prxq));

	regVal &= ~MV_PP2_V0_RXQ_PACKET_OFFSET_MASK;
	regVal |= ((offset << MV_PP2_V0_RXQ_PACKET_OFFSET_OFFS) & MV_PP2_V0_RXQ_PACKET_OFFSET_MASK);

	mvPp2WrReg(MV_PP2_V0_RXQ_CONFIG_REG(prxq), regVal);

	return MV_OK;
}

MV_STATUS mvPp2RxqBmLongPoolSet(int port, int rxq, int longPool)
{
	MV_U32 regVal = 0;
	int prxq = mvPp2LogicRxqToPhysRxq(port, rxq);

	regVal = mvPp2RdReg(MV_PP2_V0_RXQ_CONFIG_REG(prxq));
	regVal &= ~MV_PP2_V0_RXQ_POOL_LONG_MASK;
	regVal |= ((longPool << MV_PP2_V0_RXQ_POOL_LONG_OFFS) & MV_PP2_V0_RXQ_POOL_LONG_MASK);

	mvPp2WrReg(MV_PP2_V0_RXQ_CONFIG_REG(prxq), regVal);

	return MV_OK;
}

MV_STATUS mvPp2RxqBmShortPoolSet(int port, int rxq, int shortPool)
{
	MV_U32 regVal = 0;
	int prxq = mvPp2LogicRxqToPhysRxq(port, rxq);

	regVal = mvPp2RdReg(MV_PP2_V0_RXQ_CONFIG_REG(prxq));
	regVal &= ~MV_PP2_V0_RXQ_POOL_SHORT_MASK;
	regVal |= ((shortPool << MV_PP2_V0_RXQ_POOL_SHORT_OFFS) & MV_PP2_V0_RXQ_POOL_SHORT_MASK);

	mvPp2WrReg(MV_PP2_V0_RXQ_CONFIG_REG(prxq), regVal);

	return MV_OK;
}

MV_STATUS mvPp2PortHwfBmPoolSet(int port, int shortPool, int longPool)
{
	MV_U32 regVal = 0;

	regVal |= ((shortPool << MV_PP2_V0_PORT_HWF_POOL_SHORT_OFFS) & MV_PP2_V0_PORT_HWF_POOL_SHORT_MASK);
	regVal |= ((longPool << MV_PP2_V0_PORT_HWF_POOL_LONG_OFFS) & MV_PP2_V0_PORT_HWF_POOL_LONG_MASK);

	mvPp2WrReg(MV_PP2_V0_PORT_HWF_CONFIG_REG(MV_PPV2_PORT_PHYS(port)), regVal);

	return MV_OK;
}
#endif /* CONFIG_MV_ETH_PP2_1 */

/*-------------------------------------------------------------------------------*/

MV_STATUS mvPp2MhSet(int port, MV_TAG_TYPE mh)
{
	MV_U32 regVal;

	regVal = mvPp2RdReg(MV_PP2_MH_REG(MV_PPV2_PORT_PHYS(port)));
	/* Clear relevant fields */
	regVal &= ~(MV_PP2_DSA_EN_MASK | MV_PP2_MH_EN_MASK);
	switch (mh) {
	case MV_TAG_TYPE_NONE:
		break;

	case MV_TAG_TYPE_MH:
		regVal |= MV_PP2_MH_EN_MASK;
		break;

	case MV_TAG_TYPE_DSA:
		regVal |= MV_PP2_DSA_EN_MASK;
		break;

	case MV_TAG_TYPE_EDSA:
		regVal |= MV_PP2_DSA_EXTENDED;

	default:
		mvOsPrintf("port=%d: Unexpected MH = %d value\n", port, mh);
		return MV_BAD_PARAM;
	}
	mvPp2WrReg(MV_PP2_MH_REG(MV_PPV2_PORT_PHYS(port)), regVal);
	return MV_OK;
}
/*-------------------------------------------------------------------------------*/
MV_STATUS mvPp2RxFifoInit(int portNum)
{
	int i, port;

	for (i = 0; i < portNum; i++) {
		port = MV_PPV2_PORT_PHYS(i);
		mvPp2WrReg(MV_PP2_RX_DATA_FIFO_SIZE_REG(port), MV_PP2_RX_FIFO_PORT_DATA_SIZE);
		mvPp2WrReg(MV_PP2_RX_ATTR_FIFO_SIZE_REG(port), MV_PP2_RX_FIFO_PORT_ATTR_SIZE);
	}

	mvPp2WrReg(MV_PP2_RX_MIN_PKT_SIZE_REG, MV_PP2_RX_FIFO_PORT_MIN_PKT);
	mvPp2WrReg(MV_PP2_RX_FIFO_INIT_REG, 0x1);

	return MV_OK;
}

/*-------------------------------------------------------------------------------*/
/*******************************/
/*       Interrupts API        */
/*******************************/
MV_VOID mvPp2GbeCpuInterruptsDisable(int port, int cpuMask)
{
	if (mvPp2PortCheck(port))
		return;

	mvPp2WrReg(MV_PP2_ISR_ENABLE_REG(port), MV_PP2_ISR_DISABLE_INTERRUPT(cpuMask));
}

MV_VOID mvPp2GbeCpuInterruptsEnable(int port, int cpuMask)
{
	if (mvPp2PortCheck(port))
		return;

	mvPp2WrReg(MV_PP2_ISR_ENABLE_REG(port), MV_PP2_ISR_ENABLE_INTERRUPT(cpuMask));
}

MV_STATUS mvPp2RxqTimeCoalSet(int port, int rxq, MV_U32 uSec)
{
	MV_U32 regVal;
	int prxq = mvPp2LogicRxqToPhysRxq(port, rxq);

	regVal = uSec * (mvPp2HalData.tClk / 1000000);

	mvPp2WrReg(MV_PP2_ISR_RX_THRESHOLD_REG(prxq), regVal);

	return MV_OK;
}

unsigned int mvPp2RxqTimeCoalGet(int port, int rxq)
{
	MV_U32 regVal;
	int prxq = mvPp2LogicRxqToPhysRxq(port, rxq);
	unsigned int res, tClkUsec;

	regVal = mvPp2RdReg(MV_PP2_ISR_RX_THRESHOLD_REG(prxq));

	tClkUsec = mvPp2HalData.tClk / 1000000;
	res = regVal / tClkUsec;

	return res;
}

/* unmask the current CPU's rx/tx interrupts                   *
 *  - rxq_mask: support rxq to cpu granularity                 *
 *  - isTxDoneIsr: if 0 then Tx Done interruptare not unmasked */
MV_STATUS mvPp2GbeIsrRxTxUnmask(int port, MV_U16 rxq_mask, int isTxDoneIsr)
{
	if (MV_PON_PORT(port)) {
		mvPp2WrReg(MV_PP2_ISR_PON_RX_TX_MASK_REG,
			(MV_PP2_PON_CAUSE_MISC_SUM_MASK |
			((isTxDoneIsr) ? MV_PP2_PON_CAUSE_TXP_OCCUP_DESC_ALL_MASK : 0) |
			(MV_PP2_PON_CAUSE_RXQ_OCCUP_DESC_ALL_MASK & rxq_mask)));
	} else {
		mvPp2WrReg(MV_PP2_ISR_RX_TX_MASK_REG(MV_PPV2_PORT_PHYS(port)),
			(MV_PP2_CAUSE_MISC_SUM_MASK |
			((isTxDoneIsr) ? MV_PP2_CAUSE_TXQ_OCCUP_DESC_ALL_MASK : 0) |
			(MV_PP2_CAUSE_RXQ_OCCUP_DESC_ALL_MASK & rxq_mask)));
	}

	return MV_OK;
}

/* mask the current CPU's rx/tx interrupts */
MV_STATUS mvPp2GbeIsrRxTxMask(int port)
{
	if (MV_PON_PORT(port))
		mvPp2WrReg(MV_PP2_ISR_PON_RX_TX_MASK_REG, 0);
	else
		mvPp2WrReg(MV_PP2_ISR_RX_TX_MASK_REG(MV_PPV2_PORT_PHYS(port)), 0);

	return MV_OK;
}

MV_STATUS mvPp2GbeIsrRxqGroup(int port, int rxqNum)
{
	if ((rxqNum % 4 != 0) || (rxqNum > MV_ETH_MAX_RXQ)) {
		mvOsPrintf("%s: bad number of rxqs - %d.  Must be multiple of 4 and less than %d\n",
			__func__, rxqNum, MV_ETH_MAX_RXQ);
		return MV_BAD_PARAM;
	}

	mvPp2WrReg(MV_PP2_ISR_RXQ_GROUP_REG(port), rxqNum);

	return MV_OK;
}
/*-------------------------------------------------------------------------------*/
/* WRR / EJP configuration routines */

MV_STATUS mvPp2TxpMaxRateSet(int port, int txp)
{
	MV_U32 regVal;
	int eport;

	eport = mvPp2EgressPort(port, txp);
	mvPp2WrReg(MV_PP2_TXP_SCHED_PORT_INDEX_REG, eport);

	regVal = mvPp2RdReg(MV_PP2_TXP_SCHED_REFILL_REG);
	regVal &= ~MV_PP2_TXP_REFILL_PERIOD_ALL_MASK;
	regVal |= MV_PP2_TXP_REFILL_PERIOD_MASK(1);
	regVal |= MV_PP2_TXP_REFILL_TOKENS_ALL_MASK;
	mvPp2WrReg(MV_PP2_TXP_SCHED_REFILL_REG, regVal);

	regVal = MV_PP2_TXP_TOKEN_CNTR_MAX;
	mvPp2WrReg(MV_PP2_TXP_SCHED_TOKEN_SIZE_REG, regVal);

	return MV_OK;
}

MV_STATUS mvPp2TxqMaxRateSet(int port, int txp, int txq)
{
	MV_U32 regVal;
	int eport;

	eport = mvPp2EgressPort(port, txp);
	mvPp2WrReg(MV_PP2_TXP_SCHED_PORT_INDEX_REG, eport);

	regVal = mvPp2RdReg(MV_PP2_TXQ_SCHED_REFILL_REG(txq));
	regVal &= ~MV_PP2_TXQ_REFILL_PERIOD_ALL_MASK;
	regVal |= MV_PP2_TXQ_REFILL_PERIOD_MASK(1);
	regVal |= MV_PP2_TXQ_REFILL_TOKENS_ALL_MASK;
	mvPp2WrReg(MV_PP2_TXQ_SCHED_REFILL_REG(txq), regVal);

	regVal = MV_PP2_TXQ_TOKEN_CNTR_MAX;
	mvPp2WrReg(MV_PP2_TXQ_SCHED_TOKEN_SIZE_REG(txq), regVal);

	return MV_OK;
}

/* Calculate period and tokens accordingly with required rate and accuracy */
MV_STATUS mvPp2RateCalc(int rate, unsigned int accuracy, unsigned int *pPeriod, unsigned int *pTokens)
{
	/* Calculate refill tokens and period - rate [Kbps] = tokens [bits] * 1000 / period [usec] */
	/* Assume:  Tclock [MHz] / BasicRefillNoOfClocks = 1 */
	unsigned int period, tokens, calc;

	if (rate == 0) {
		/* Disable traffic from the port: tokens = 0 */
		if (pPeriod != NULL)
			*pPeriod = 1000;

		if (pTokens != NULL)
			*pTokens = 0;

		return MV_OK;
	}

	/* Find values of "period" and "tokens" match "rate" and "accuracy" when period is minimal */
	for (period = 1; period <= 1000; period++) {
		tokens = 1;
		while (MV_TRUE)	{
			calc = (tokens * 1000) / period;
			if (((MV_ABS(calc - rate) * 100) / rate) <= accuracy) {
				if (pPeriod != NULL)
					*pPeriod = period;

				if (pTokens != NULL)
					*pTokens = tokens;

				return MV_OK;
			}
			if (calc > rate)
				break;

			tokens++;
		}
	}
	return MV_FAIL;
}

/* Set bandwidth limitation for TX port
 *   rate [Kbps]    - steady state TX bandwidth limitation
 */
MV_STATUS   mvPp2TxpRateSet(int port, int txp, int rate)
{
	MV_U32		regVal;
	unsigned int	tokens, period, txPortNum, accuracy = 0;
	MV_STATUS	status;

	if (mvPp2TxpCheck(port, txp))
		return MV_BAD_PARAM;

	txPortNum = mvPp2EgressPort(port, txp);
	mvPp2WrReg(MV_PP2_TXP_SCHED_PORT_INDEX_REG, txPortNum);

	regVal = mvPp2RdReg(MV_PP2_TXP_SCHED_PERIOD_REG);

	status = mvPp2RateCalc(rate, accuracy, &period, &tokens);
	if (status != MV_OK) {
		mvOsPrintf("%s: Can't provide rate of %d [Kbps] with accuracy of %d [%%]\n",
				__func__, rate, accuracy);
		return status;
	}
	if (tokens > MV_PP2_TXP_REFILL_TOKENS_MAX)
		tokens = MV_PP2_TXP_REFILL_TOKENS_MAX;

	if (period > MV_PP2_TXP_REFILL_PERIOD_MAX)
		period = MV_PP2_TXP_REFILL_PERIOD_MAX;

	regVal = mvPp2RdReg(MV_PP2_TXP_SCHED_REFILL_REG);

	regVal &= ~MV_PP2_TXP_REFILL_TOKENS_ALL_MASK ;
	regVal |= MV_PP2_TXP_REFILL_TOKENS_MASK(tokens);

	regVal &= ~MV_PP2_TXP_REFILL_PERIOD_ALL_MASK;
	regVal |= MV_PP2_TXP_REFILL_PERIOD_MASK(period);

	mvPp2WrReg(MV_PP2_TXP_SCHED_REFILL_REG, regVal);

	return MV_OK;
}

/* Set maximum burst size for TX port
 *   burst [bytes] - number of bytes to be sent with maximum possible TX rate,
 *                    before TX rate limitation will take place.
 */
MV_STATUS mvPp2TxpBurstSet(int port, int txp, int burst)
{
	MV_U32  size, mtu;
	int txPortNum;

	if (mvPp2TxpCheck(port, txp))
		return MV_BAD_PARAM;

	txPortNum = mvPp2EgressPort(port, txp);
	mvPp2WrReg(MV_PP2_TXP_SCHED_PORT_INDEX_REG, txPortNum);

	/* Calulate Token Bucket Size */
	size = 8 * burst;

	if (size > MV_PP2_TXP_TOKEN_SIZE_MAX)
		size = MV_PP2_TXP_TOKEN_SIZE_MAX;

	/* Token bucket size must be larger then MTU */
	mtu = mvPp2RdReg(MV_PP2_TXP_SCHED_MTU_REG);
	if (mtu > size) {
		mvOsPrintf("%s Error: Bucket size (%d bytes) < MTU (%d bytes)\n",
					__func__, (size / 8), (mtu / 8));
		return MV_BAD_PARAM;
	}
	mvPp2WrReg(MV_PP2_TXP_SCHED_TOKEN_SIZE_REG, size);

	return MV_OK;
}

/* Set bandwidth limitation for TXQ
 *   rate  [Kbps]  - steady state TX rate limitation
 */
MV_STATUS   mvPp2TxqRateSet(int port, int txp, int txq, int rate)
{
	MV_U32		regVal;
	unsigned int	txPortNum, period, tokens, accuracy = 0;
	MV_STATUS	status;

	if (mvPp2TxpCheck(port, txp))
		return MV_BAD_PARAM;

	if (txq >= MV_ETH_MAX_TXQ)
		return MV_BAD_PARAM;

	status = mvPp2RateCalc(rate, accuracy, &period, &tokens);
	if (status != MV_OK) {
		mvOsPrintf("%s: Can't provide rate of %d [Kbps] with accuracy of %d [%%]\n",
				__func__, rate, accuracy);
		return status;
	}

	txPortNum = mvPp2EgressPort(port, txp);
	mvPp2WrReg(MV_PP2_TXP_SCHED_PORT_INDEX_REG, txPortNum);

	if (tokens > MV_PP2_TXQ_REFILL_TOKENS_MAX)
		tokens = MV_PP2_TXQ_REFILL_TOKENS_MAX;

	if (period > MV_PP2_TXQ_REFILL_PERIOD_MAX)
		period = MV_PP2_TXQ_REFILL_PERIOD_MAX;

	regVal = mvPp2RdReg(MV_PP2_TXQ_SCHED_REFILL_REG(txq));

	regVal &= ~MV_PP2_TXQ_REFILL_TOKENS_ALL_MASK;
	regVal |= MV_PP2_TXQ_REFILL_TOKENS_MASK(tokens);

	regVal &= ~MV_PP2_TXQ_REFILL_PERIOD_ALL_MASK;
	regVal |= MV_PP2_TXQ_REFILL_PERIOD_MASK(period);

	mvPp2WrReg(MV_PP2_TXQ_SCHED_REFILL_REG(txq), regVal);

	return MV_OK;
}

/* Set maximum burst size for TX port
 *   burst [bytes] - number of bytes to be sent with maximum possible TX rate,
 *                    before TX bandwidth limitation will take place.
 */
MV_STATUS mvPp2TxqBurstSet(int port, int txp, int txq, int burst)
{
	MV_U32  size, mtu;
	int txPortNum;

	if (mvPp2TxpCheck(port, txp))
		return MV_BAD_PARAM;

	if (txq >= MV_ETH_MAX_TXQ)
		return MV_BAD_PARAM;

	txPortNum = mvPp2EgressPort(port, txp);
	mvPp2WrReg(MV_PP2_TXP_SCHED_PORT_INDEX_REG, txPortNum);

	/* Calulate Tocket Bucket Size */
	size = 8 * burst;

	if (size > MV_PP2_TXQ_TOKEN_SIZE_MAX)
		size = MV_PP2_TXQ_TOKEN_SIZE_MAX;

	/* Tocken bucket size must be larger then MTU */
	mtu = mvPp2RdReg(MV_PP2_TXP_SCHED_MTU_REG);
	if (mtu > size) {
		mvOsPrintf("%s Error: Bucket size (%d bytes) < MTU (%d bytes)\n",
					__func__, (size / 8), (mtu / 8));
		return MV_BAD_PARAM;
	}

	mvPp2WrReg(MV_PP2_TXQ_SCHED_TOKEN_SIZE_REG(txq), size);

	return MV_OK;
}

/* Set TXQ to work in FIX priority mode */
MV_STATUS mvPp2TxqFixPrioSet(int port, int txp, int txq)
{
	MV_U32 regVal;
	int txPortNum;

	if (mvPp2TxpCheck(port, txp))
		return MV_BAD_PARAM;

	if (txq >= MV_ETH_MAX_TXQ)
		return MV_BAD_PARAM;

	txPortNum = mvPp2EgressPort(port, txp);
	mvPp2WrReg(MV_PP2_TXP_SCHED_PORT_INDEX_REG, txPortNum);

	regVal = mvPp2RdReg(MV_PP2_TXP_SCHED_FIXED_PRIO_REG);
	regVal |= (1 << txq);
	mvPp2WrReg(MV_PP2_TXP_SCHED_FIXED_PRIO_REG, regVal);

	return MV_OK;
}

/* Set TXQ to work in WRR mode and set relative weight. */
/*   Weight range [1..N] */
MV_STATUS mvPp2TxqWrrPrioSet(int port, int txp, int txq, int weight)
{
	MV_U32 regVal, mtu;
	int txPortNum;

	if (mvPp2TxpCheck(port, txp))
		return MV_BAD_PARAM;

	if (txq >= MV_ETH_MAX_TXQ)
		return MV_BAD_PARAM;

	txPortNum = mvPp2EgressPort(port, txp);
	mvPp2WrReg(MV_PP2_TXP_SCHED_PORT_INDEX_REG, txPortNum);

	/* Weight * 256 bytes * 8 bits must be larger then MTU [bits] */
	mtu = mvPp2RdReg(MV_PP2_TXP_SCHED_MTU_REG);
	/* MTU [bits] -> MTU [256 bytes] */
	mtu = ((mtu / 8) / 256) + 1;

	if ((weight < mtu) || (weight > MV_PP2_TXQ_WRR_WEIGHT_MAX)) {
		mvOsPrintf("%s Error: weight=%d is out of range %d...%d\n",
				__func__, weight, mtu, MV_PP2_TXQ_WRR_WEIGHT_MAX);
		return MV_FAIL;
	}

	regVal = mvPp2RdReg(MV_PP2_TXQ_SCHED_WRR_REG(txq));

	regVal &= ~MV_PP2_TXQ_WRR_WEIGHT_ALL_MASK;
	regVal |= MV_PP2_TXQ_WRR_WEIGHT_MASK(weight);
	mvPp2WrReg(MV_PP2_TXQ_SCHED_WRR_REG(txq), regVal);

	regVal = mvPp2RdReg(MV_PP2_TXP_SCHED_FIXED_PRIO_REG);
	regVal &= ~(1 << txq);
	mvPp2WrReg(MV_PP2_TXP_SCHED_FIXED_PRIO_REG, regVal);

	return MV_OK;
}

/* Set minimum number of tockens to start transmit for TX port
 *   maxTxSize [bytes]    - maximum packet size can be sent via this TX port
 */
MV_STATUS   mvPp2TxpMaxTxSizeSet(int port, int txp, int maxTxSize)
{
	MV_U32	regVal, size, mtu;
	int	txq, txPortNum;

	if (mvPp2TxpCheck(port, txp))
		return MV_BAD_PARAM;

	mtu = maxTxSize * 8;
	if (mtu > MV_PP2_TXP_MTU_MAX)
		mtu = MV_PP2_TXP_MTU_MAX;

	/* WA for wrong Token bucket update: Set MTU value = 3*real MTU value */
	mtu = 3 * mtu;

	txPortNum = mvPp2EgressPort(port, txp);

	mvPp2WrReg(MV_PP2_TXP_SCHED_PORT_INDEX_REG, txPortNum);
	/* set MTU */
	regVal = mvPp2RdReg(MV_PP2_TXP_SCHED_MTU_REG);
	regVal &= ~MV_PP2_TXP_MTU_ALL_MASK;
	regVal |= MV_PP2_TXP_MTU_MASK(mtu);

	mvPp2WrReg(MV_PP2_TXP_SCHED_MTU_REG, regVal);

	/* TXP token size and all TXQs token size must be larger that MTU */
	regVal = mvPp2RdReg(MV_PP2_TXP_SCHED_TOKEN_SIZE_REG);
	size = regVal & MV_PP2_TXP_TOKEN_SIZE_MAX;
	if (size < mtu) {
		size = mtu;
		regVal &= ~MV_PP2_TXP_TOKEN_SIZE_MAX;
		regVal |= size;
		mvPp2WrReg(MV_PP2_TXP_SCHED_TOKEN_SIZE_REG, regVal);
	}
	for (txq = 0; txq < CONFIG_MV_ETH_TXQ; txq++) {
		regVal = mvPp2RdReg(MV_PP2_TXQ_SCHED_TOKEN_SIZE_REG(txq));
		size = regVal & MV_PP2_TXQ_TOKEN_SIZE_MAX;
		if (size < mtu) {
			size = mtu;
			regVal &= ~MV_PP2_TXQ_TOKEN_SIZE_MAX;
			regVal |= size;
			mvPp2WrReg(MV_PP2_TXQ_SCHED_TOKEN_SIZE_REG(txq), regVal);
		}
	}
	return MV_OK;
}

/* Disable transmit via physical egress queue - HW doesn't take descriptors from DRAM */
MV_STATUS mvPp2TxpDisable(int port, int txp)
{
	MV_U32 regData;
	int    mDelay;
	int    txPortNum = mvPp2EgressPort(port, txp);

	/* Issue stop command for active channels only */
	mvPp2WrReg(MV_PP2_TXP_SCHED_PORT_INDEX_REG, txPortNum);
	regData = (mvPp2RdReg(MV_PP2_TXP_SCHED_Q_CMD_REG)) & MV_PP2_TXP_SCHED_ENQ_MASK;
	if (regData != 0)
		mvPp2WrReg(MV_PP2_TXP_SCHED_Q_CMD_REG, (regData << MV_PP2_TXP_SCHED_DISQ_OFFSET));

	/* Wait for all Tx activity to terminate. */
	mDelay = 0;
	do {
		if (mDelay >= TX_DISABLE_TIMEOUT_MSEC) {
			mvOsPrintf("port=%d, txp=%d: TIMEOUT for TX stopped !!! txQueueCmd - 0x%08x\n",
				   port, txp, regData);
			return MV_TIMEOUT;
		}
		mvOsDelay(1);
		mDelay++;

		/* Check port TX Command register that all Tx queues are stopped */
		regData = mvPp2RdReg(MV_PP2_TXP_SCHED_Q_CMD_REG);
	} while (regData & MV_PP2_TXP_SCHED_ENQ_MASK);

	return MV_OK;
}

/* Enable transmit via physical egress queue - HW starts take descriptors from DRAM */
MV_STATUS mvPp2TxpEnable(int port, int txp)
{
	MV_PP2_PORT_CTRL *pPortCtrl = mvPp2PortHndlGet(port);
	MV_U32 qMap;
	int    txq, eport = mvPp2EgressPort(port, txp);

	/* Enable all initialized TXs. */
	qMap = 0;
	for (txq = 0; txq < pPortCtrl->txqNum; txq++) {
		if (pPortCtrl->pTxQueue[txp * CONFIG_MV_ETH_TXQ + txq] != NULL)
			qMap |= (1 << txq);
	}
	/* Indirect access to register */
	mvPp2WrReg(MV_PP2_TXP_SCHED_PORT_INDEX_REG, eport);
	mvPp2WrReg(MV_PP2_TXP_SCHED_Q_CMD_REG, qMap);

	return MV_OK;
}

#ifdef CONFIG_MV_ETH_PP2_1
/* Functions implemented only for PPv2.1 version (A0 and later) */
MV_STATUS mvPp2RxqEnable(int port, int rxq, MV_BOOL en)
{
	MV_U32 regVal;
	int prxq = mvPp2LogicRxqToPhysRxq(port, rxq);

	regVal = mvPp2RdReg(MV_PP2_RXQ_CONFIG_REG(prxq));
	if (en)
		regVal &= ~MV_PP2_RXQ_DISABLE_MASK;
	else
		regVal |= MV_PP2_RXQ_DISABLE_MASK;

	mvPp2WrReg(MV_PP2_RXQ_CONFIG_REG(prxq), regVal);

	return MV_OK;
}

MV_STATUS mvPp2HwfTxqEnable(int port, int txp, int txq, MV_BOOL en)
{
	MV_U32 regVal;
	int ptxq = MV_PPV2_TXQ_PHYS(port, txp, txq);

	regVal = mvPp2RdReg(MV_PP2_HWF_TXQ_CONFIG_REG(ptxq));

	if (en)
		regVal &= ~MV_PP2_HWF_TXQ_DISABLE_MASK;
	else
		regVal |= MV_PP2_HWF_TXQ_DISABLE_MASK;

	mvPp2WrReg(MV_PP2_HWF_TXQ_CONFIG_REG(ptxq), regVal);

	return MV_OK;
}

MV_BOOL mvPp2DisableCmdInProgress(void)
{
	MV_U32 regVal;

	regVal = mvPp2RdReg(MV_PP2_RX_STATUS);
	regVal &= MV_PP2_DISABLE_IN_PROG_MASK;

	return regVal;
}


MV_STATUS mvPp2TxqDrainSet(int port, int txp, int txq, MV_BOOL en)
{
	MV_U32 regVal;
	int ptxq = MV_PPV2_TXQ_PHYS(port, txp, txq);

	mvPp2WrReg(MV_PP2_TXQ_NUM_REG, ptxq);
	regVal = mvPp2RdReg(MV_PP2_TXQ_PREF_BUF_REG);

	if (en)
		regVal |= MV_PP2_HWF_TXQ_DISABLE_MASK;
	else
		regVal &= ~MV_PP2_HWF_TXQ_DISABLE_MASK;

	mvPp2WrReg(MV_PP2_TXQ_PREF_BUF_REG, regVal);

	return MV_OK;
}

MV_STATUS mvPp2TxPortFifoFlush(int port, MV_BOOL en)
{
	MV_U32 regVal;

	/* valid only for ethernet ports (not for xPON) */
	if (MV_PON_PORT(port))
		return MV_NOT_SUPPORTED;

	regVal = mvPp2RdReg(MV_PP2_TX_PORT_FLUSH_REG);

	if (en)
		regVal |= MV_PP2_TX_PORT_FLUSH_MASK(port);
	else
		regVal &= ~MV_PP2_TX_PORT_FLUSH_MASK(port);

	mvPp2WrReg(MV_PP2_TX_PORT_FLUSH_REG, regVal);

	return MV_OK;
}

#else /* Stabs for Z1 */

MV_STATUS mvPp2TxqDrainSet(int port, int txp, int txq, MV_BOOL en)
{
	return MV_OK;
}

MV_STATUS mvPp2TxPortFifoFlush(int port, MV_BOOL en)
{
	return MV_OK;
}
#endif /* CONFIG_MV_ETH_PP2_1 */

/* Function for swithcing SWF to HWF */
/* txq is physical (global) txq in range 0..MV_PP2_TXQ_TOTAL_NUM */
/* txq is physical (global) rxq in range 0..MV_ETH_RXQ_TOTAL_NUM */

MV_STATUS mvPp2FwdSwitchCtrl(MV_U32 flowId, int txq, int rxq, int msec)
{
	MV_U32 regVal;
	int timeout, max;

	/* Check validity of parameters */
	if (mvPp2MaxCheck(txq, MV_PP2_TXQ_TOTAL_NUM, "global txq"))
		return MV_BAD_PARAM;

	if (mvPp2MaxCheck(rxq, MV_ETH_RXQ_TOTAL_NUM, "global rxq"))
		return MV_BAD_PARAM;

	timeout = MV_PP2_FWD_SWITCH_TIMEOUT_MAX * 1024;
	max = timeout / (mvPp2HalData.tClk / 1000);
	if (mvPp2MaxCheck(msec, max + 1, "timeout msec"))
		return MV_BAD_PARAM;

	mvPp2WrReg(MV_PP2_FWD_SWITCH_FLOW_ID_REG, flowId);
	timeout = ((mvPp2HalData.tClk / 1000) * msec) / 1024;
	regVal = MV_PP2_FWD_SWITCH_TXQ_VAL(txq) | MV_PP2_FWD_SWITCH_RXQ_VAL(rxq) |
		MV_PP2_FWD_SWITCH_TIMEOUT_VAL(timeout);
	mvPp2WrReg(MV_PP2_FWD_SWITCH_CTRL_REG, regVal);

	return MV_OK;
}

int       mvPp2FwdSwitchStatus(int *hwState, int *msec)
{
	MV_U32 regVal, cycles;

	regVal = mvPp2RdReg(MV_PP2_FWD_SWITCH_STATUS_REG);
	if (hwState)
		*hwState = (regVal & MV_PP2_FWD_SWITCH_STATE_MASK) >> MV_PP2_FWD_SWITCH_STATE_OFFS;

	cycles = (regVal & MV_PP2_FWD_SWITCH_TIMER_MASK) >> MV_PP2_FWD_SWITCH_TIMER_OFFS;
	cycles *= 1024;
	if (msec)
		*msec = cycles / (mvPp2HalData.tClk / 1000);

	return (regVal & MV_PP2_FWD_SWITCH_STATUS_MASK) >> MV_PP2_FWD_SWITCH_STATUS_OFFS;
}

