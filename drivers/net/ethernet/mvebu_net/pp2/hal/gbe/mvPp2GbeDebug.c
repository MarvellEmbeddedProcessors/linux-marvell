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
#include "bm/mvBm.h"

#include "mvPp2GbeRegs.h"

MV_VOID mvPp2RxDmaRegsPrint(void)
{
	int i;

	mvOsPrintf("\n[RX DMA regs]\n");

	mvOsPrintf("\nRXQs [0..%d] registers\n", MV_ETH_RXQ_TOTAL_NUM);
	for (i = 0; i < MV_ETH_RXQ_TOTAL_NUM; i++) {
#ifdef CONFIG_MV_ETH_PP2_1
		mvPp2PrintReg(MV_PP2_RX_STATUS, "MV_PP2_RX_STATUS");
		mvPp2PrintReg2(MV_PP2_RXQ_CONFIG_REG(i), "MV_PP2_RXQ_CONFIG_REG", i);
#else
		mvPp2PrintReg2(MV_PP2_V0_RXQ_SNOOP_REG(i), "MV_PP2_RXQ_SNOOP_REG", i);
		mvPp2PrintReg2(MV_PP2_V0_RXQ_CONFIG_REG(i), "MV_PP2_RXQ_CONFIG_REG", i);
#endif
	}
	mvOsPrintf("\nBM pools [0..%d] registers\n", MV_BM_POOLS);
	for (i = 0; i < MV_BM_POOLS; i++)
		mvPp2PrintReg2(MV_PP2_POOL_BUF_SIZE_REG(i), "MV_PP2_POOL_BUF_SIZE_REG", i);

	mvOsPrintf("\nIngress ports [0..%d] registers\n", MV_PP2_MAX_PORTS);
	for (i = 0; i < MV_PP2_MAX_PORTS; i++) {
#ifndef CONFIG_MV_ETH_PP2_1
		mvPp2PrintReg2(MV_PP2_V0_PORT_HWF_CONFIG_REG(i), "MV_PP2_PORT_HWF_CONFIG_REG", i);
#endif
		mvPp2PrintReg2(MV_PP2_RX_CTRL_REG(i), "MV_PP2_RX_CTRL_REG", i);
	}
	mvOsPrintf("\n");
}

static void mvPp2QueueShow(MV_PP2_QUEUE_CTRL *pQueueCtrl, int mode, int isTxq)
{
	mvOsPrintf("pFirst=%p (0x%x), descSize=%d, numOfDescr=%d\n",
			pQueueCtrl->pFirst,
			(MV_U32) pp2DescVirtToPhys(pQueueCtrl, (MV_U8 *) pQueueCtrl->pFirst),
			pQueueCtrl->descSize, pQueueCtrl->lastDesc + 1);

	if (mode > 0) {
		int i;

		for (i = 0; i <= pQueueCtrl->lastDesc; i++) {
			if (isTxq) {
				PP2_TX_DESC *pTxDesc = (PP2_TX_DESC *) MV_PP2_QUEUE_DESC_PTR(pQueueCtrl, i);
				mvOsPrintf("%3d. pTxDesc=%p, cmd=%08x, offs=%08x, txq=%4d, data=%4d, bufAddr=%08x, cookie=%x\n",
					   i, pTxDesc, pTxDesc->command, pTxDesc->pktOffset, pTxDesc->physTxq, pTxDesc->dataSize,
					   (MV_U32) pTxDesc->bufPhysAddr, (MV_U32) pTxDesc->bufCookie);

				mvOsCacheLineInv(NULL, pTxDesc);
			} else { /* RXQ */
				PP2_RX_DESC *pRxDesc;

				pRxDesc = (PP2_RX_DESC *) MV_PP2_QUEUE_DESC_PTR(pQueueCtrl, i);
				mvOsPrintf("%3d. desc=%p, status=%08x, data=%4d, bufAddr=%08x, bufCookie=%08x, parserInfo=%03x\n",
					   i, pRxDesc, pRxDesc->status,
					   pRxDesc->dataSize, (MV_U32) pRxDesc->bufPhysAddr,
					   (MV_U32) pRxDesc->bufCookie, (MV_U16) pRxDesc->parserInfo);
				mvOsCacheLineInv(NULL, pRxDesc);
			}
		}
	}
}

/* Show Port/Rxq descriptors ring */
MV_VOID mvPp2RxqShow(int port, int rxq, int mode)
{
	int pRxq;
	MV_PP2_PORT_CTRL *pCtrl;
	MV_PP2_PHYS_RXQ_CTRL *pRxqCtrl;
	MV_PP2_QUEUE_CTRL *pQueueCtrl;

	if (mvPp2PortCheck(port))
		return;

	pCtrl = mvPp2PortCtrl[port];
	if ((pCtrl == NULL) || (pCtrl->pRxQueue == NULL)) {
		mvOsPrintf("port #%d is not initialized\n", port);
		return;
	}

	if (mvPp2MaxCheck(rxq, MV_ETH_MAX_RXQ, "logical rxq"))
		return;

	if (pCtrl->pRxQueue[rxq] == NULL) {
		mvOsPrintf("rxq #%d of port #%d is not initialized\n", rxq, port);
		return;
	}

	pRxq = mvPp2LogicRxqToPhysRxq(port, rxq);
	if (pRxq < 0) {
		mvOsPrintf("Port/RXQ is not mapped to physical RXQ\n");
		return;
	}
	mvOsPrintf("\n[PPv2 RxQ show: port=%d, logical rxq=%d -> physical rxq=%d]\n",
			port, rxq, pRxq);

	pRxqCtrl = &mvPp2PhysRxqs[pRxq];
	pQueueCtrl = &pRxqCtrl->queueCtrl;
	if (!pQueueCtrl->pFirst) {
		mvOsPrintf("rxq %d wasn't created\n", rxq);
		return;
	}

	mvOsPrintf("nextToProc=%d (%p), rxqOccupied=%d, rxqNonOccupied=%d\n",
		   pQueueCtrl->nextToProc,
		   MV_PP2_QUEUE_DESC_PTR(pQueueCtrl, pQueueCtrl->nextToProc),
		   mvPp2RxqBusyDescNumGet(port, rxq), mvPp2RxqFreeDescNumGet(port, rxq));

	mvPp2QueueShow(pQueueCtrl, mode, 0);
}

/* Show Port/TXQ descriptors ring */
MV_VOID mvPp2TxqShow(int port, int txp, int txq, int mode)
{
	int pTxq;
	MV_PP2_PHYS_TXQ_CTRL *pTxqCtrl;
	MV_PP2_QUEUE_CTRL *pQueueCtrl;

	mvOsPrintf("\n[PPv2 TxQ show: port=%d, txp=%d, txq=%d]\n", port, txp, txq);

	if (mvPp2TxpCheck(port, txp))
		return;

	if (mvPp2MaxCheck(txq, MV_ETH_MAX_TXQ, "logical txq"))
		return;

	pTxq = MV_PPV2_TXQ_PHYS(port, txp, txq);

	pTxqCtrl = &mvPp2PhysTxqs[pTxq];
	pQueueCtrl = &pTxqCtrl->queueCtrl;
	if (!pQueueCtrl->pFirst) {
		mvOsPrintf("txq %d wasn't created\n", txq);
		return;
	}

	mvOsPrintf("nextToProc=%d (%p), txqPending=%d\n",
		   pQueueCtrl->nextToProc,
		   MV_PP2_QUEUE_DESC_PTR(pQueueCtrl, pQueueCtrl->nextToProc),
		   mvPp2TxqPendDescNumGet(port, txp, txq));

	mvPp2QueueShow(pQueueCtrl, mode, 1);
}

/* Show CPU aggregation TXQ descriptors ring */
MV_VOID mvPp2AggrTxqShow(int cpu, int mode)
{
	MV_PP2_AGGR_TXQ_CTRL *pTxqCtrl;
	MV_PP2_QUEUE_CTRL *pQueueCtrl;

	mvOsPrintf("\n[PPv2 AggrTxQ: cpu=%d]\n", cpu);

	if (mvPp2CpuCheck(cpu))
		return;

	pTxqCtrl = &mvPp2AggrTxqs[cpu];
	pQueueCtrl = &pTxqCtrl->queueCtrl;
	if (!pQueueCtrl->pFirst) {
		mvOsPrintf("aggr tx queue for cpu %d wasn't created\n", cpu);
		return;
	}
	mvOsPrintf("nextToProc=%d (%p), txqPending=%d\n",
		   pQueueCtrl->nextToProc,
		   MV_PP2_QUEUE_DESC_PTR(pQueueCtrl, pQueueCtrl->nextToProc),
		   mvPp2AggrTxqPendDescNumGet(cpu));

	mvPp2QueueShow(pQueueCtrl, mode, 1);
}

void mvPp2IsrRegs(int port)
{
	int physPort;

	if (mvPp2PortCheck(port))
		return;

	physPort = MV_PPV2_PORT_PHYS(port);

	mvOsPrintf("\n[PPv2 ISR registers: port=%d - %s]\n", port, MV_PON_PORT(port) ? "PON" : "GMAC");
	mvPp2PrintReg(MV_PP2_ISR_RXQ_GROUP_REG(port), "MV_PP2_ISR_RXQ_GROUP_REG");
	mvPp2PrintReg(MV_PP2_ISR_ENABLE_REG(port), "MV_PP2_ISR_ENABLE_REG");
	mvPp2PrintReg(MV_PP2_ISR_RX_TX_CAUSE_REG(physPort), "MV_PP2_ISR_RX_TX_CAUSE_REG");
	mvPp2PrintReg(MV_PP2_ISR_RX_TX_MASK_REG(physPort), "MV_PP2_ISR_RX_TX_MASK_REG");

	mvPp2PrintReg(MV_PP2_ISR_RX_ERR_CAUSE_REG(physPort), "MV_PP2_ISR_RX_ERR_CAUSE_REG");
	mvPp2PrintReg(MV_PP2_ISR_RX_ERR_MASK_REG(physPort), "MV_PP2_ISR_RX_ERR_MASK_REG");

	if (MV_PON_PORT(port)) {
		mvPp2PrintReg(MV_PP2_ISR_PON_TX_UNDR_CAUSE_REG, "MV_PP2_ISR_PON_TX_UNDR_CAUSE_REG");
		mvPp2PrintReg(MV_PP2_ISR_PON_TX_UNDR_MASK_REG, "MV_PP2_ISR_PON_TX_UNDR_MASK_REG");
	} else {
		mvPp2PrintReg(MV_PP2_ISR_TX_ERR_CAUSE_REG(physPort), "MV_PP2_ISR_TX_ERR_CAUSE_REG");
		mvPp2PrintReg(MV_PP2_ISR_TX_ERR_MASK_REG(physPort), "MV_PP2_ISR_TX_ERR_MASK_REG");
	}
	mvPp2PrintReg(MV_PP2_ISR_MISC_CAUSE_REG, "MV_PP2_ISR_MISC_CAUSE_REG");
	mvPp2PrintReg(MV_PP2_ISR_MISC_MASK_REG, "MV_PP2_ISR_MISC_MASK_REG");
}

void mvPp2PhysRxqRegs(int rxq)
{
	mvOsPrintf("\n[PPv2 RxQ registers: global rxq=%d]\n", rxq);

	if (mvPp2MaxCheck(rxq, MV_ETH_RXQ_TOTAL_NUM, "global rxq"))
		return;

	mvPp2WrReg(MV_PP2_RXQ_NUM_REG, rxq);
	mvPp2PrintReg(MV_PP2_RXQ_NUM_REG, "MV_PP2_RXQ_NUM_REG");
	mvPp2PrintReg(MV_PP2_RXQ_DESC_ADDR_REG, "MV_PP2_RXQ_DESC_ADDR_REG");
	mvPp2PrintReg(MV_PP2_RXQ_DESC_SIZE_REG, "MV_PP2_RXQ_DESC_SIZE_REG");
	mvPp2PrintReg(MV_PP2_RXQ_STATUS_REG(rxq), "MV_PP2_RXQ_STATUS_REG");
	mvPp2PrintReg(MV_PP2_RXQ_THRESH_REG, "MV_PP2_RXQ_THRESH_REG");
	mvPp2PrintReg(MV_PP2_RXQ_INDEX_REG, "MV_PP2_RXQ_INDEX_REG");
#ifdef CONFIG_MV_ETH_PP2_1
	mvPp2PrintReg(MV_PP2_RXQ_CONFIG_REG(rxq), "MV_PP2_RXQ_CONFIG_REG");
#else
	mvPp2PrintReg(MV_PP2_V0_RXQ_CONFIG_REG(rxq), "MV_PP2_RXQ_CONFIG_REG");
	mvPp2PrintReg(MV_PP2_V0_RXQ_SNOOP_REG(rxq), "MV_PP2_RXQ_SNOOP_REG");
	mvPp2PrintReg(MV_PP2_V0_RX_EARLY_DROP_REG(rxq), "MV_PP2_V0_RX_EARLY_DROP_REG");
	mvPp2PrintReg(MV_PP2_V0_RX_DESC_DROP_REG(rxq), "MV_PP2_V0_RX_DESC_DROP_REG");
#endif

}

void mvPp2PortRxqRegs(int port, int rxq)
{
	mvOsPrintf("\n[PPv2 RxQ registers: port=%d, local rxq=%d]\n", port, rxq);

	if (mvPp2PortCheck(port))
		return;

	if (mvPp2MaxCheck(rxq, MV_ETH_MAX_RXQ, "local rxq"))
		return;

	mvPp2PhysRxqRegs(mvPp2LogicRxqToPhysRxq(port, rxq));
}

void mvPp2PhysTxqRegs(int txq)
{
	mvOsPrintf("\n[PPv2 TxQ registers: global txq=%d]\n", txq);

	if (mvPp2MaxCheck(txq, MV_PP2_TXQ_TOTAL_NUM, "global txq"))
		return;

	mvPp2WrReg(MV_PP2_TXQ_NUM_REG, txq);
	mvPp2PrintReg(MV_PP2_TXQ_NUM_REG, "MV_PP2_TXQ_NUM_REG");
	mvPp2PrintReg(MV_PP2_TXQ_DESC_ADDR_REG, "MV_PP2_TXQ_DESC_ADDR_REG");
	mvPp2PrintReg(MV_PP2_TXQ_DESC_SIZE_REG, "MV_PP2_TXQ_DESC_SIZE_REG");
	mvPp2PrintReg(MV_PP2_TXQ_DESC_HWF_SIZE_REG, "MV_PP2_TXQ_DESC_HWF_SIZE_REG");
	mvPp2PrintReg(MV_PP2_TXQ_INDEX_REG, "MV_PP2_TXQ_INDEX_REG");
	mvPp2PrintReg(MV_PP2_TXQ_PREF_BUF_REG, "MV_PP2_TXQ_PREF_BUF_REG");
	mvPp2PrintReg(MV_PP2_TXQ_PENDING_REG, "MV_PP2_TXQ_PENDING_REG");
	mvPp2PrintReg(MV_PP2_TXQ_SENT_REG(txq), "MV_PP2_TXQ_SENT_REG");
	mvPp2PrintReg(MV_PP2_TXQ_INT_STATUS_REG, "MV_PP2_TXQ_INT_STATUS_REG");
}

void mvPp2PortTxqRegs(int port, int txp, int txq)
{
	mvOsPrintf("\n[PPv2 TxQ registers: port=%d, txp=%d, local txq=%d]\n", port, txp, txq);

	if (mvPp2TxpCheck(port, txp))
		return;

	if (mvPp2MaxCheck(txq, MV_ETH_MAX_TXQ, "local txq"))
		return;

	mvPp2PhysTxqRegs(MV_PPV2_TXQ_PHYS(port, txp, txq));
}

void mvPp2AggrTxqRegs(int cpu)
{
	mvOsPrintf("\n[PP2 Aggr TXQ registers: cpu=%d]\n", cpu);

	if (mvPp2CpuCheck(cpu))
		return;

	mvPp2PrintReg(MV_PP2_AGGR_TXQ_DESC_ADDR_REG(cpu), "MV_PP2_AGGR_TXQ_DESC_ADDR_REG");
	mvPp2PrintReg(MV_PP2_AGGR_TXQ_DESC_SIZE_REG(cpu), "MV_PP2_AGGR_TXQ_DESC_SIZE_REG");
	mvPp2PrintReg(MV_PP2_AGGR_TXQ_STATUS_REG(cpu), "MV_PP2_AGGR_TXQ_STATUS_REG");
	mvPp2PrintReg(MV_PP2_AGGR_TXQ_INDEX_REG(cpu), "MV_PP2_AGGR_TXQ_INDEX_REG");
}

void mvPp2AddrDecodeRegs(void)
{
	MV_U32 regValue;
	int win;

	/* ToDo - print Misc interrupt Cause and Mask registers */

	mvPp2PrintReg(ETH_BASE_ADDR_ENABLE_REG, "ETH_BASE_ADDR_ENABLE_REG");
	mvPp2PrintReg(ETH_TARGET_DEF_ADDR_REG, "ETH_TARGET_DEF_ADDR_REG");
	mvPp2PrintReg(ETH_TARGET_DEF_ID_REG, "ETH_TARGET_DEF_ID_REG");

	regValue = mvPp2RdReg(ETH_BASE_ADDR_ENABLE_REG);
	for (win = 0; win < ETH_MAX_DECODE_WIN; win++) {
		if ((regValue & (1 << win)) == 0)
			continue; /* window is disable */

		mvOsPrintf("\t win[%d]\n", win);
		mvPp2PrintReg(ETH_WIN_BASE_REG(win), "\t ETH_WIN_BASE_REG");
		mvPp2PrintReg(ETH_WIN_SIZE_REG(win), "\t ETH_WIN_SIZE_REG");
		if (win < ETH_MAX_HIGH_ADDR_REMAP_WIN)
			mvPp2PrintReg(ETH_WIN_REMAP_REG(win), "\t ETH_WIN_REMAP_REG");
	}
}


void mvPp2TxSchedRegs(int port, int txp)
{
	int physTxp, txq;

	physTxp = MV_PPV2_TXP_PHYS(port, txp);

	mvOsPrintf("\n[TXP Scheduler registers: port=%d, txp=%d, physPort=%d]\n", port, txp, physTxp);

	mvPp2WrReg(MV_PP2_TXP_SCHED_PORT_INDEX_REG, physTxp);
	mvPp2PrintReg(MV_PP2_TXP_SCHED_PORT_INDEX_REG, "MV_PP2_TXP_SCHED_PORT_INDEX_REG");
	mvPp2PrintReg(MV_PP2_TXP_SCHED_Q_CMD_REG, "MV_PP2_TXP_SCHED_Q_CMD_REG");
	mvPp2PrintReg(MV_PP2_TXP_SCHED_CMD_1_REG, "MV_PP2_TXP_SCHED_CMD_1_REG");
	mvPp2PrintReg(MV_PP2_TXP_SCHED_FIXED_PRIO_REG, "MV_PP2_TXP_SCHED_FIXED_PRIO_REG");
	mvPp2PrintReg(MV_PP2_TXP_SCHED_PERIOD_REG, "MV_PP2_TXP_SCHED_PERIOD_REG");
	mvPp2PrintReg(MV_PP2_TXP_SCHED_MTU_REG, "MV_PP2_TXP_SCHED_MTU_REG");
	mvPp2PrintReg(MV_PP2_TXP_SCHED_REFILL_REG, "MV_PP2_TXP_SCHED_REFILL_REG");
	mvPp2PrintReg(MV_PP2_TXP_SCHED_TOKEN_SIZE_REG, "MV_PP2_TXP_SCHED_TOKEN_SIZE_REG");
	mvPp2PrintReg(MV_PP2_TXP_SCHED_TOKEN_CNTR_REG, "MV_PP2_TXP_SCHED_TOKEN_CNTR_REG");

	for (txq = 0; txq < MV_ETH_MAX_TXQ; txq++) {
		mvOsPrintf("\n[TxQ Scheduler registers: port=%d, txp=%d, txq=%d]\n", port, txp, txq);
		mvPp2PrintReg(MV_PP2_TXQ_SCHED_REFILL_REG(txq), "MV_PP2_TXQ_SCHED_REFILL_REG");
		mvPp2PrintReg(MV_PP2_TXQ_SCHED_TOKEN_SIZE_REG(txq), "MV_PP2_TXQ_SCHED_TOKEN_SIZE_REG");
		mvPp2PrintReg(MV_PP2_TXQ_SCHED_TOKEN_CNTR_REG(txq), "MV_PP2_TXQ_SCHED_TOKEN_CNTR_REG");
	}
}

void      mvPp2FwdSwitchRegs(void)
{
	mvOsPrintf("\n[FWD Switch registers]\n");

	mvPp2PrintReg(MV_PP2_FWD_SWITCH_FLOW_ID_REG, "MV_PP2_FWD_SWITCH_FLOW_ID_REG");
	mvPp2PrintReg(MV_PP2_FWD_SWITCH_CTRL_REG, "MV_PP2_FWD_SWITCH_CTRL_REG");
	mvPp2PrintReg(MV_PP2_FWD_SWITCH_STATUS_REG, "MV_PP2_FWD_SWITCH_STATUS_REG");
}

void mvPp2BmPoolRegs(int pool)
{
	if (mvPp2MaxCheck(pool, MV_BM_POOLS, "bm_pool"))
		return;

	mvOsPrintf("\n[BM pool registers: pool=%d]\n", pool);
	mvPp2PrintReg(MV_BM_POOL_BASE_REG(pool), "MV_BM_POOL_BASE_REG");
	mvPp2PrintReg(MV_BM_POOL_SIZE_REG(pool), "MV_BM_POOL_SIZE_REG");
	mvPp2PrintReg(MV_BM_POOL_READ_PTR_REG(pool), "MV_BM_POOL_READ_PTR_REG");
	mvPp2PrintReg(MV_BM_POOL_PTRS_NUM_REG(pool), "MV_BM_POOL_PTRS_NUM_REG");
	mvPp2PrintReg(MV_BM_BPPI_READ_PTR_REG(pool), "MV_BM_BPPI_READ_PTR_REG");
	mvPp2PrintReg(MV_BM_BPPI_PTRS_NUM_REG(pool), "MV_BM_BPPI_PTRS_NUM_REG");
	mvPp2PrintReg(MV_BM_POOL_CTRL_REG(pool), "MV_BM_POOL_CTRL_REG");
	mvPp2PrintReg(MV_BM_INTR_CAUSE_REG(pool), "MV_BM_INTR_CAUSE_REG");
	mvPp2PrintReg(MV_BM_INTR_MASK_REG(pool), "MV_BM_INTR_MASK_REG");
}

void mvPp2V0DropCntrs(int port)
{
	int i;

	mvOsPrintf("\n[Port #%d Drop counters]\n", port);
	mvPp2PrintReg(MV_PP2_OVERRUN_DROP_REG(MV_PPV2_PORT_PHYS(port)), "MV_PP2_OVERRUN_DROP_REG");
	mvPp2PrintReg(MV_PP2_CLS_DROP_REG(MV_PPV2_PORT_PHYS(port)), "MV_PP2_CLS_DROP_REG");

	if (MV_PON_PORT(port)) {
		for (i = 0; i < mvPp2HalData.maxTcont; i++) {
			mvPp2PrintReg2(MV_PP2_V0_TX_EARLY_DROP_REG(i), "MV_PP2_TX_EARLY_DROP_REG", i);
			mvPp2PrintReg2(MV_PP2_V0_TX_DESC_DROP_REG(i), "MV_PP2_TX_DESC_DROP_REG", i);
		}
	} else {
		i = MV_ETH_MAX_TCONT + port;
		mvPp2PrintReg2(MV_PP2_V0_TX_EARLY_DROP_REG(i), "MV_PP2_TX_EARLY_DROP_REG", i);
		mvPp2PrintReg2(MV_PP2_V0_TX_DESC_DROP_REG(i), "MV_PP2_TX_DESC_DROP_REG", i);
	}
	for (i = port * CONFIG_MV_ETH_RXQ; i < (port * CONFIG_MV_ETH_RXQ + CONFIG_MV_ETH_RXQ); i++) {
		mvPp2PrintReg2(MV_PP2_V0_RX_EARLY_DROP_REG(i), "MV_PP2_RX_EARLY_DROP_REG", i);
		mvPp2PrintReg2(MV_PP2_V0_RX_DESC_DROP_REG(i), "MV_PP2_RX_DESC_DROP_REG", i);
	}
}

void mvPp2V1DropCntrs(int port)
{
	int txp, phyRxq, q;
	MV_PP2_PORT_CTRL *pPortCtrl = mvPp2PortHndlGet(port);
	int physPort = MV_PPV2_PORT_PHYS(port);


	mvOsPrintf("\n[global drop counters]\n");
	mvPp2RegPrintNonZero(MV_PP2_V1_OVERFLOW_MC_DROP_REG, "MV_PP2_OVERRUN_DROP_REG");

	mvOsPrintf("\n[Port #%d Drop counters]\n", port);
	mvPp2RegPrintNonZero(MV_PP2_OVERRUN_DROP_REG(physPort), "MV_PP2_OVERRUN_DROP_REG");
	mvPp2RegPrintNonZero(MV_PP2_CLS_DROP_REG(physPort), "MV_PP2_CLS_DROP_REG");

	for (txp = 0; txp < pPortCtrl->txpNum; txp++) {
		for (q = 0; q < MV_ETH_MAX_TXQ; q++) {
			mvOsPrintf("\n------ [Port #%d txp #%d txq #%d counters] -----\n", port, txp, q);
			mvPp2WrReg(MV_PP2_V1_CNT_IDX_REG, TX_CNT_IDX(port, txp, q));
			mvPp2RegPrintNonZero(MV_PP2_V1_TX_PKT_FULLQ_DROP_REG, "MV_PP2_V1_TX_PKT_FULLQ_DROP_REG");
			mvPp2RegPrintNonZero(MV_PP2_V1_TX_PKT_EARLY_DROP_REG, "MV_PP2_V1_TX_PKT_EARLY_DROP_REG");
			mvPp2RegPrintNonZero(MV_PP2_V1_TX_PKT_BM_DROP_REG, "MV_PP2_V1_TX_PKT_BM_DROP_REG");
			mvPp2RegPrintNonZero(MV_PP2_V1_TX_PKT_BM_MC_DROP_REG, "MV_PP2_V1_TX_PKT_BM_MC_DROP_REG");
		}
	}

	for (q = 0; q < CONFIG_MV_ETH_RXQ; q++) {
		mvOsPrintf("\n------ [Port #%d, rxq #%d counters] -----\n", port, q);
		phyRxq = mvPp2LogicRxqToPhysRxq(port, q);
		mvPp2WrReg(MV_PP2_V1_CNT_IDX_REG, phyRxq);
		mvPp2RegPrintNonZero(MV_PP2_V1_RX_PKT_FULLQ_DROP_REG, "MV_PP2_V1_RX_PKT_FULLQ_DROP_REG");
		mvPp2RegPrintNonZero(MV_PP2_V1_RX_PKT_EARLY_DROP_REG, "MV_PP2_V1_RX_PKT_EARLY_DROP_REG");
		mvPp2RegPrintNonZero(MV_PP2_V1_RX_PKT_BM_DROP_REG, "MV_PP2_V1_RX_PKT_BM_DROP_REG");
	}
}

void mvPp2V1TxqDbgCntrs(int port, int txp, int txq)
{
	mvOsPrintf("\n------ [Port #%d txp #%d txq #%d counters] -----\n", port, txp, txq);
	mvPp2WrReg(MV_PP2_V1_CNT_IDX_REG, TX_CNT_IDX(port, txp, txq));
	mvPp2PrintReg(MV_PP2_V1_TX_DESC_ENQ_REG, "MV_PP2_V1_TX_DESC_ENQ_REG");
	mvPp2PrintReg(MV_PP2_V1_TX_DESC_ENQ_TO_DRAM_REG, "MV_PP2_V1_TX_DESC_ENQ_TO_DRAM_REG");
	mvPp2PrintReg(MV_PP2_V1_TX_BUF_ENQ_TO_DRAM_REG, "MV_PP2_V1_TX_BUF_ENQ_TO_DRAM_REG");
	mvPp2PrintReg(MV_PP2_V1_TX_DESC_HWF_ENQ_REG, "MV_PP2_V1_TX_DESC_HWF_ENQ_REG");
	mvPp2PrintReg(MV_PP2_V1_TX_PKT_DQ_REG, "MV_PP2_V1_TX_PKT_DQ_REG");
	mvPp2PrintReg(MV_PP2_V1_TX_PKT_FULLQ_DROP_REG, "MV_PP2_V1_TX_PKT_FULLQ_DROP_REG");
	mvPp2PrintReg(MV_PP2_V1_TX_PKT_EARLY_DROP_REG, "MV_PP2_V1_TX_PKT_EARLY_DROP_REG");
	mvPp2PrintReg(MV_PP2_V1_TX_PKT_BM_DROP_REG, "MV_PP2_V1_TX_PKT_BM_DROP_REG");
	mvPp2PrintReg(MV_PP2_V1_TX_PKT_BM_MC_DROP_REG, "MV_PP2_V1_TX_PKT_BM_MC_DROP_REG");
}

void mvPp2V1RxqDbgCntrs(int port, int rxq)
{
	int phyRxq = mvPp2LogicRxqToPhysRxq(port, rxq);

	mvOsPrintf("\n------ [Port #%d, rxq #%d counters] -----\n", port, rxq);
	mvPp2WrReg(MV_PP2_V1_CNT_IDX_REG, phyRxq);
	mvPp2PrintReg(MV_PP2_V1_RX_PKT_FULLQ_DROP_REG, "MV_PP2_V1_RX_PKT_FULLQ_DROP_REG");
	mvPp2PrintReg(MV_PP2_V1_RX_PKT_EARLY_DROP_REG, "MV_PP2_V1_RX_PKT_EARLY_DROP_REG");
	mvPp2PrintReg(MV_PP2_V1_RX_PKT_BM_DROP_REG, "MV_PP2_V1_RX_PKT_BM_DROP_REG");
	mvPp2PrintReg(MV_PP2_V1_RX_DESC_ENQ_REG, "MV_PP2_V1_RX_DESC_ENQ_REG");
}

void mvPp2TxRegs(void)
{
	mvOsPrintf("\n[TX general registers]\n");

	mvPp2PrintReg(MV_PP2_TX_SNOOP_REG, "MV_PP2_TX_SNOOP_REG");
	mvPp2PrintReg(MV_PP2_TX_FIFO_THRESH_REG, "MV_PP2_TX_FIFO_THRESH_REG");
	mvPp2PrintReg(MV_PP2_TX_PORT_FLUSH_REG, "MV_PP2_TX_PORT_FLUSH_REG");
}

void mvPp2RxFifoRegs(int port)
{
	int p = MV_PPV2_PORT_PHYS(port);

	mvOsPrintf("\n[Port #%d RX Fifo]\n", p);
	mvPp2PrintReg(MV_PP2_RX_DATA_FIFO_SIZE_REG(p), "MV_PP2_RX_DATA_FIFO_SIZE_REG");
	mvPp2PrintReg(MV_PP2_RX_ATTR_FIFO_SIZE_REG(p), "MV_PP2_RX_ATTR_FIFO_SIZE_REG");
	mvOsPrintf("\n[Global RX Fifo regs]\n");
	mvPp2PrintReg(MV_PP2_RX_MIN_PKT_SIZE_REG, "MV_PP2_RX_MIN_PKT_SIZE_REG");
}


/* Print status of Ethernet port */
void mvPp2PortStatus(int port)
{
	int i, txp, txq;
	MV_ETH_PORT_STATUS	link;
	MV_PP2_PORT_CTRL 	*pPortCtrl;

	if (mvPp2PortCheck(port))
		return;

	pPortCtrl = mvPp2PortHndlGet(port);
	if (!pPortCtrl)
		return;

	mvOsPrintf("\n[RXQ mapping: port=%d, ctrl=%p]\n", port, pPortCtrl);
	if (pPortCtrl->pRxQueue) {
		mvOsPrintf("         RXQ: ");
		for (i = 0; i < pPortCtrl->rxqNum; i++)
			mvOsPrintf(" %4d", i);

		mvOsPrintf("\nphysical RXQ: ");
		for (i = 0; i < pPortCtrl->rxqNum; i++) {
			if (pPortCtrl->pRxQueue[i])
				mvOsPrintf(" %4d", pPortCtrl->pRxQueue[i]->rxq);
			else
				mvOsPrintf(" NULL");
		}
		mvOsPrintf("\n");
	}

	mvOsPrintf("\n[BM queue to Qset mapping]\n");
	if (pPortCtrl->pRxQueue) {
		mvOsPrintf("       RXQ: ");
		for (i = 0; i < pPortCtrl->rxqNum; i++)
			mvOsPrintf(" %4d", i);

		mvOsPrintf("\n long Qset: ");
		for (i = 0; i < pPortCtrl->rxqNum; i++)
			mvOsPrintf(" %4d", mvBmRxqToQsetLongGet(mvPp2LogicRxqToPhysRxq(port, i)));

		mvOsPrintf("\nshort Qset: ");
		for (i = 0; i < pPortCtrl->rxqNum; i++)
			mvOsPrintf(" %4d", mvBmRxqToQsetShortGet(mvPp2LogicRxqToPhysRxq(port, i)));

		mvOsPrintf("\n");
	}
	if (pPortCtrl->pTxQueue) {
		for (txp = 0; txp < pPortCtrl->txpNum; txp++) {
			mvOsPrintf("\nTXP %2d, TXQ:", txp);
			for (txq = 0; txq < pPortCtrl->txqNum; txq++)
				mvOsPrintf(" %4d", txq);

			mvOsPrintf("\n long Qset: ");
			for (txq = 0; txq < pPortCtrl->txqNum; txq++)
				mvOsPrintf(" %4d", mvBmTxqToQsetLongGet(MV_PPV2_TXQ_PHYS(port, txp, txq)));

			mvOsPrintf("\nshort Qset: ");
			for (txq = 0; txq < pPortCtrl->txqNum; txq++)
				mvOsPrintf(" %4d", mvBmTxqToQsetShortGet(MV_PPV2_TXQ_PHYS(port, txp, txq)));

			mvOsPrintf("\n");
		}
	}

	mvOsPrintf("\n[Link: port=%d, ctrl=%p]\n", port, pPortCtrl);

	if (!MV_PON_PORT(port)) {

		mvGmacLinkStatus(port, &link);

		if (link.linkup) {
			mvOsPrintf("link up");
			mvOsPrintf(", %s duplex", (link.duplex == MV_ETH_DUPLEX_FULL) ? "full" : "half");
			mvOsPrintf(", speed ");

			if (link.speed == MV_ETH_SPEED_1000)
				mvOsPrintf("1 Gbps\n");
			else if (link.speed == MV_ETH_SPEED_100)
				mvOsPrintf("100 Mbps\n");
			else
				mvOsPrintf("10 Mbps\n");

			mvOsPrintf("rxFC - %s, txFC - %s\n",
				(link.rxFc == MV_ETH_FC_DISABLE) ? "disabled" : "enabled",
				(link.txFc == MV_ETH_FC_DISABLE) ? "disabled" : "enabled");
		} else
			mvOsPrintf("link down\n");
	}
}
