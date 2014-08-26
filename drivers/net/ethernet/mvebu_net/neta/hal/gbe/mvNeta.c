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

#include "mvNeta.h"
#include "pnc/mvPnc.h"
#include "bm/mvBm.h"

/* This array holds the control structure of each port */
MV_NETA_PORT_CTRL **mvNetaPortCtrl = NULL;
MV_NETA_HAL_DATA mvNetaHalData;

/* Bitmap of NETA dymamic capabilities, such as PnC, BM, HWF and PME
	 * Pnc - 0x01
	 * BM  - 0x02
	 * HWF - 0x04
	 * PME - 0x08
*/
unsigned int neta_cap_bitmap = 0x0;

/* Function prototypes */
/* Legacy parse function start */
static MV_BOOL netaSetUcastAddr(int port, MV_U8 lastNibble, int queue);
static MV_BOOL netaSetSpecialMcastAddr(int port, MV_U8 lastByte, int queue);
static MV_BOOL netaSetOtherMcastAddr(int port, MV_U8 crc8, int queue);
/* Legacy parse function end */

static void mvNetaPortSgmiiConfig(int port, MV_BOOL isInband);
static MV_U8 *mvNetaDescrMemoryAlloc(MV_NETA_PORT_CTRL * pPortCtrl, int descSize,
				   MV_ULONG *pPhysAddr, MV_U32 *memHandle);
static void mvNetaDescrMemoryFree(MV_NETA_PORT_CTRL *pPortCtrl, MV_BUF_INFO *pDescBuf);
static void mvNetaDescRingReset(MV_NETA_QUEUE_CTRL *pQueueHndl);

#define TX_DISABLE_TIMEOUT_MSEC     1000
#define RX_DISABLE_TIMEOUT_MSEC     1000
#define TX_FIFO_EMPTY_TIMEOUT_MSEC  10000
#define PORT_DISABLE_WAIT_TCLOCKS   5000


int mvNetaMaxCheck(int value, int limit, char *name)
{
	if ((value < 0) || (value >= limit)) {
		mvOsPrintf("%s %d is out of range [0..%d]\n",
			name ? name : "value", value, (limit - 1));
		return 1;
	}
	return 0;
}

int mvNetaPortCheck(int port)
{
	return mvNetaMaxCheck(port, mvNetaHalData.maxPort, "port");
}

int mvNetaTxpCheck(int port, int txp)
{
	int txpMax = 1;

	if (mvNetaPortCheck(port))
		return 1;

	if (MV_PON_PORT(port))
		txpMax = MV_ETH_MAX_TCONT;

	return mvNetaMaxCheck(txp, txpMax, "txp");
}

int mvNetaCpuCheck(int cpu)
{
	return mvNetaMaxCheck(cpu, NETA_MAX_CPU_REGS, "cpu");
}

/******************************************************************************/
/*                      Port Initialization functions                         */
/******************************************************************************/

/*******************************************************************************
* mvNetaPortInit - Initialize the NETA port
*
* DESCRIPTION:
*       This function initializes the NETA port.
*       1) Allocates and initializes internal port control structure.
*       2) Creates RX and TX descriptor rings.
*       3) Disables RX and TX operations, clears cause registers and
*	   masks all interrupts.
*       4) Sets all registers to default values and cleans all MAC tables.
*
* INPUT:
*       int			portNo          - NETA port number
*
* RETURN:
*       void* - NETA port handler that should be passed to most other
*               functions dealing with this port.
*
* NOTE: This function is called once per port when loading the NETA module.
*******************************************************************************/
void *mvNetaPortInit(int portNo, void *osHandle)
{
	MV_NETA_PORT_CTRL *pPortCtrl;

	/* Check validity of parameters */
	if ((portNo < 0) || (portNo >= mvNetaHalData.maxPort)) {
		mvOsPrintf("EthPort #%d: Bad initialization parameters\n", portNo);
		return NULL;
	}

	pPortCtrl = (MV_NETA_PORT_CTRL *)mvOsMalloc(sizeof(MV_NETA_PORT_CTRL));
	if (pPortCtrl == NULL) {
		mvOsPrintf("EthDrv: Can't allocate %dB for port #%d control structure!\n",
			   (int)sizeof(MV_NETA_PORT_CTRL), portNo);
		return NULL;
	}

	memset(pPortCtrl, 0, sizeof(MV_NETA_PORT_CTRL));
	mvNetaPortCtrl[portNo] = pPortCtrl;

	pPortCtrl->portNo = portNo;
	pPortCtrl->osHandle = osHandle;

	pPortCtrl->txpNum = 1;

#ifdef CONFIG_MV_PON
	if (MV_PON_PORT(portNo))
		pPortCtrl->txpNum = MV_ETH_MAX_TCONT;
#endif /* CONFIG_MV_PON */

	pPortCtrl->rxqNum = CONFIG_MV_ETH_RXQ;
	pPortCtrl->txqNum = CONFIG_MV_ETH_TXQ;

	/* Allocate RXQ and TXQ structures */
	pPortCtrl->pRxQueue = mvOsMalloc(pPortCtrl->rxqNum * sizeof(MV_NETA_RXQ_CTRL));
	if (pPortCtrl->pRxQueue == NULL) {
		mvOsPrintf("mvNeta port%d: Can't allocate %d Bytes for %d RXQs controls\n",
			   portNo, (int)pPortCtrl->rxqNum * sizeof(MV_NETA_RXQ_CTRL), pPortCtrl->rxqNum);
		return NULL;
	}
	memset(pPortCtrl->pRxQueue, 0, pPortCtrl->rxqNum * sizeof(MV_NETA_RXQ_CTRL));

	pPortCtrl->pTxQueue = mvOsMalloc(pPortCtrl->txpNum * pPortCtrl->txqNum * sizeof(MV_NETA_TXQ_CTRL));
	if (pPortCtrl->pTxQueue == NULL) {
		mvOsPrintf("mvNeta port%d: Can't allocate %d Bytes for %d TXQs controls\n",
			   portNo, (int)pPortCtrl->txqNum * pPortCtrl->txpNum * sizeof(MV_NETA_TXQ_CTRL),
			   pPortCtrl->txqNum * pPortCtrl->txpNum);
		return NULL;
	}
	memset(pPortCtrl->pTxQueue, 0, pPortCtrl->txpNum * pPortCtrl->txqNum * sizeof(MV_NETA_TXQ_CTRL));

	/* Disable port */
	mvNetaPortDisable(portNo);
	mvNetaDefaultsSet(portNo);

	return pPortCtrl;
}

/*******************************************************************************
* mvNetaPortDestroy - Free the memory allocated for a NETA port
*
* DESCRIPTION:
*       This function frees the memory allocated for the NETA port in mvNetaPortInit().
*
* INPUT:
*       int			portNo          - NETA port number
*
*******************************************************************************/
void mvNetaPortDestroy(int portNo)
{
	MV_NETA_PORT_CTRL *pPortCtrl = mvNetaPortHndlGet(portNo);

	if (pPortCtrl->pTxQueue)
		mvOsFree(pPortCtrl->pTxQueue);

	if (pPortCtrl->pRxQueue)
		mvOsFree(pPortCtrl->pRxQueue);

	if (pPortCtrl)
		mvOsFree(pPortCtrl);

	mvNetaPortCtrl[portNo] = NULL;
}


/*******************************************************************************
* mvNetaAccMode - Get NETA Acceleration mode
*
* DESCRIPTION:
*
* INPUT:
*
* RETURN:
*       int - NETA Acceleration mode
*
* NOTE: This function is called once on loading the NETA module.
*******************************************************************************/
int mvNetaAccMode(void)
{
	int mode;

	if (MV_NETA_BM_CAP() && MV_NETA_PNC_CAP())
		mode = NETA_ACC_MODE_MASK(NETA_ACC_MODE_EXT_PNC_BMU);
	else if (MV_NETA_BM_CAP())
		mode = NETA_ACC_MODE_MASK(NETA_ACC_MODE_EXT_BMU);
	else if (MV_NETA_PNC_CAP())
		mode = NETA_ACC_MODE_MASK(NETA_ACC_MODE_EXT_PNC);
	else
		mode = NETA_ACC_MODE_MASK(NETA_ACC_MODE_EXT);

	return mode;
}

/*******************************************************************************
* mvNetaDefaultsSet - Set defaults to the NETA port
*
* DESCRIPTION:
*       This function sets default values to the NETA port.
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
MV_STATUS mvNetaDefaultsSet(int port)
{
	int cpu;
	int queue, txp;
	MV_U32 regVal;
	MV_NETA_PORT_CTRL *pPortCtrl = mvNetaPortHndlGet(port);

	/* Clear all Cause registers */
	MV_REG_WRITE(NETA_INTR_NEW_CAUSE_REG(port), 0);
	MV_REG_WRITE(NETA_INTR_OLD_CAUSE_REG(port), 0);
	MV_REG_WRITE(NETA_INTR_MISC_CAUSE_REG(port), 0);

	/* Mask all interrupts */
	MV_REG_WRITE(NETA_INTR_NEW_MASK_REG(port), 0);
	MV_REG_WRITE(NETA_INTR_OLD_MASK_REG(port), 0);
	MV_REG_WRITE(NETA_INTR_MISC_MASK_REG(port), 0);

	MV_REG_WRITE(NETA_INTR_ENABLE_REG(port), 0);

	/* Enable MBUS Retry bit16 */
	MV_REG_WRITE(NETA_MBUS_RETRY_REG(port), NETA_MBUS_RETRY_CYCLES(0x20));

	/* Set CPU queue access map - all CPUs have access to all RX queues and to all TX queues */

	for (cpu = 0; cpu < NETA_MAX_CPU_REGS; cpu++)
		if (MV_BIT_CHECK(mvNetaHalData.cpuMask, cpu))
			MV_REG_WRITE(NETA_CPU_MAP_REG(port, cpu), (NETA_CPU_RXQ_ACCESS_ALL_MASK | NETA_CPU_TXQ_ACCESS_ALL_MASK));

	/* Reset RX and TX DMAs */
	MV_REG_WRITE(NETA_PORT_RX_RESET_REG(port), NETA_PORT_RX_DMA_RESET_MASK);

	for (txp = 0; txp < pPortCtrl->txpNum; txp++) {
		MV_REG_WRITE(NETA_PORT_TX_RESET_REG(port, txp), NETA_PORT_TX_DMA_RESET_MASK);

#ifdef CONFIG_MV_PON
		if ((txp * MV_ETH_MAX_TXQ % 32) == 0) {
			MV_REG_WRITE(GPON_TXQ_INTR_NEW_CAUSE_REG(txp * MV_ETH_MAX_TXQ), 0);
			MV_REG_WRITE(GPON_TXQ_INTR_NEW_MASK_REG(txp * MV_ETH_MAX_TXQ), 0);
			MV_REG_WRITE(GPON_TXQ_INTR_OLD_CAUSE_REG(txp * MV_ETH_MAX_TXQ), 0);
			MV_REG_WRITE(GPON_TXQ_INTR_OLD_MASK_REG(txp * MV_ETH_MAX_TXQ), 0);
			MV_REG_WRITE(GPON_TXQ_INTR_ERR_CAUSE_REG(txp * MV_ETH_MAX_TXQ), 0);
			MV_REG_WRITE(GPON_TXQ_INTR_ERR_MASK_REG(txp * MV_ETH_MAX_TXQ), 0);

			MV_REG_WRITE(GPON_TXQ_INTR_ENABLE_REG(txp * MV_ETH_MAX_TXQ), 0xFFFFFFFF);
		}
#endif /* CONFIG_MV_PON */

		/* Disable Legacy WRR, Disable EJP, Release from reset */
		MV_REG_WRITE(NETA_TX_CMD_1_REG(port, txp), 0);

		/* Close bandwidth for all queues */
		for (queue = 0; queue < MV_ETH_MAX_TXQ; queue++)
			MV_REG_WRITE(NETA_TXQ_TOKEN_CNTR_REG(port, txp, queue),  0);

		/* Set basic period to  1 usec */
		MV_REG_WRITE(NETA_TX_REFILL_PERIOD_REG(port, txp),  mvNetaHalData.tClk / 1000000);
		mvNetaTxpRateMaxSet(port, txp);

		MV_REG_WRITE(NETA_PORT_TX_RESET_REG(port, txp), 0);
	}

	MV_REG_WRITE(NETA_PORT_RX_RESET_REG(port), 0);

	/* Set Port Acceleration Mode */
	regVal = mvNetaAccMode();
	MV_REG_WRITE(NETA_ACC_MODE_REG(port), regVal);

#ifdef CONFIG_MV_ETH_BM
	/* Set address of Buffer Management Unit */
	if (MV_NETA_BM_CAP())
		MV_REG_WRITE(NETA_BM_ADDR_REG(port), mvNetaHalData.bmPhysBase);
#endif /* CONFIG_MV_ETH_BM */

	/* Update value of portCfg register accordingly with all RxQueue types */
	regVal = PORT_CONFIG_VALUE(CONFIG_MV_ETH_RXQ_DEF);
	MV_REG_WRITE(ETH_PORT_CONFIG_REG(port), regVal);

	regVal = PORT_CONFIG_EXTEND_VALUE;
	MV_REG_WRITE(ETH_PORT_CONFIG_EXTEND_REG(port), regVal);

	if (MV_PON_PORT(port))
		MV_REG_WRITE(ETH_RX_MINIMAL_FRAME_SIZE_REG(port), 40);
	else
		MV_REG_WRITE(ETH_RX_MINIMAL_FRAME_SIZE_REG(port), 64);

#ifndef MV_ETH_GMAC_NEW
	if (!MV_PON_PORT(port)) {
		regVal = PORT_SERIAL_CONTROL_VALUE;

		regVal &= ~ETH_MAX_RX_PACKET_SIZE_MASK;
		regVal |= ETH_MAX_RX_PACKET_1522BYTE;

		MV_REG_WRITE(ETH_PORT_SERIAL_CTRL_REG(port), regVal);

		/* Allow receiving packes with odd number of preamble nibbles */
		regVal = MV_REG_READ(ETH_PORT_SERIAL_CTRL_1_REG(port));
		regVal |= ETH_EN_MII_ODD_PRE_MASK;
		MV_REG_WRITE(ETH_PORT_SERIAL_CTRL_1_REG(port), regVal);
	}
#endif /* !MV_ETH_GMAC_NEW */

	/* build PORT_SDMA_CONFIG_REG */
	regVal = 0;

#ifdef CONFIG_MV_ETH_REDUCE_BURST_SIZE_WA
	/* This is a WA for the IOCC HW BUG involve in using 128B burst size */
	regVal |= ETH_TX_BURST_SIZE_MASK(ETH_BURST_SIZE_2_64BIT_VALUE);
	regVal |= ETH_RX_BURST_SIZE_MASK(ETH_BURST_SIZE_2_64BIT_VALUE);
#else
	/* Default burst size */
	regVal |= ETH_TX_BURST_SIZE_MASK(ETH_BURST_SIZE_16_64BIT_VALUE);
	regVal |= ETH_RX_BURST_SIZE_MASK(ETH_BURST_SIZE_16_64BIT_VALUE);
#endif /* CONFIG_MV_ETH_REDUCE_BURST_SIZE_WA */

#if defined(MV_CPU_BE) && !defined(CONFIG_MV_ETH_BE_WA)
    /* big endian */
    regVal |= (ETH_RX_NO_DATA_SWAP_MASK | ETH_TX_NO_DATA_SWAP_MASK | ETH_DESC_SWAP_MASK);
#else /* MV_CPU_LE */
    /* little endian */
	regVal |= (ETH_RX_NO_DATA_SWAP_MASK | ETH_TX_NO_DATA_SWAP_MASK | ETH_NO_DESC_SWAP_MASK);
#endif /* MV_CPU_BE && !CONFIG_MV_ETH_BE_WA */

	/* Assign port SDMA configuration */
	MV_REG_WRITE(ETH_SDMA_CONFIG_REG(port), regVal);

	mvNetaSetUcastTable(port, -1);
	mvNetaSetSpecialMcastTable(port, -1);
	mvNetaSetOtherMcastTable(port, -1);

#if defined(CONFIG_MV_PON) && defined(MV_PON_MIB_SUPPORT)
	if (MV_PON_PORT(port))
		/* Set default MibNo = 0 for PON RX counters */
		mvNetaPonRxMibDefault(0);
#endif /* CONFIG_MV_PON && MV_PON_MIB_SUPPORT */

	/* Set port interrupt enable register - default enable all */
	MV_REG_WRITE(NETA_INTR_ENABLE_REG(port),
		     (NETA_RXQ_PKT_INTR_ENABLE_ALL_MASK | NETA_TXQ_PKT_INTR_ENABLE_ALL_MASK));

	return MV_OK;
}

/*******************************************************************************
* mvNetaHalInit - Initialize the HAL and the NETA unit
*
* DESCRIPTION:
*       This function:
*	1) Initializes HAL global data structures.
*       2) Clears and disables NETA unit interrupts.
*
* INPUT:  NONE
*
* RETURN: NONE
*
* NOTE: this function is called once in the boot process.
*******************************************************************************/
MV_STATUS mvNetaHalInit(MV_NETA_HAL_DATA *halData)
{
	int port;

	mvNetaHalData = *halData;

	/* Allocate port data structures */
	mvNetaPortCtrl = mvOsMalloc(mvNetaHalData.maxPort * sizeof(MV_NETA_PORT_CTRL *));
	if (mvNetaPortCtrl == NULL) {
		mvOsPrintf("%s: Can't allocate %d bytes for %d ports\n", __func__,
			   mvNetaHalData.maxPort * sizeof(MV_NETA_PORT_CTRL), mvNetaHalData.maxPort);
		return MV_FAIL;
	}
	for (port = 0; port < mvNetaHalData.maxPort; port++)
		mvNetaPortCtrl[port] = NULL;

#ifdef CONFIG_MV_ETH_BM
	if (MV_NETA_BM_CAP())
		mvNetaBmInit(mvNetaHalData.bmVirtBase);
#endif /* CONFIG_MV_ETH_BM */

#ifdef CONFIG_MV_ETH_PNC
	if (MV_NETA_PNC_CAP())
		mvPncInit(mvNetaHalData.pncVirtBase, mvNetaHalData.pncTcamSize);
#endif /* CONFIG_MV_ETH_PNC */

	return MV_OK;
}

/* Update CPUs that can process packets incoming to specific RXQ */
MV_STATUS	mvNetaRxqCpuMaskSet(int port, int rxq_mask, int cpu)
{
	MV_U32	regVal;

	if (mvNetaPortCheck(port))
		return MV_ERROR;

	if (mvNetaCpuCheck(cpu))
		return MV_ERROR;

	regVal = MV_REG_READ(NETA_CPU_MAP_REG(port, cpu));
	regVal &= ~NETA_CPU_RXQ_ACCESS_ALL_MASK;
	regVal |= (rxq_mask << NETA_CPU_RXQ_ACCESS_OFFS);
	MV_REG_WRITE(NETA_CPU_MAP_REG(port, cpu), regVal);

	return MV_OK;
}

/* Update specific CPU that can process packets outcoming to TXQs */
MV_STATUS	mvNetaTxqCpuMaskSet(int port, int txq_mask, int cpu)
{
	MV_U32	regVal;

	if (mvNetaPortCheck(port))
		return MV_ERROR;

	if (mvNetaCpuCheck(cpu))
		return MV_ERROR;

	regVal = MV_REG_READ(NETA_CPU_MAP_REG(port, cpu));
	regVal &= ~NETA_CPU_TXQ_ACCESS_ALL_MASK;
	regVal |= (txq_mask << NETA_CPU_TXQ_ACCESS_OFFS);
	MV_REG_WRITE(NETA_CPU_MAP_REG(port, cpu), regVal);

	return MV_OK;
}

/*****************************************************************/
/* Functions below are different for old and new version of GMAC */
/*****************************************************************/

#ifdef MV_ETH_GMAC_NEW

MV_STATUS       mvEthGmacRgmiiSet(int port, int enable)
{
	MV_U32  regVal;

	regVal = MV_REG_READ(NETA_GMAC_CTRL_2_REG(port));
	if (enable)
		regVal |= NETA_GMAC_PORT_RGMII_MASK;
	else
		regVal &= ~NETA_GMAC_PORT_RGMII_MASK;

	MV_REG_WRITE(NETA_GMAC_CTRL_2_REG(port), regVal);

	return MV_OK;
}

static void mvNetaPortSgmiiConfig(int port, MV_BOOL isInband)
{
	MV_U32 regVal;


	regVal = MV_REG_READ(NETA_GMAC_CTRL_2_REG(port));
	regVal |= (NETA_GMAC_PSC_ENABLE_MASK);
	MV_REG_WRITE(NETA_GMAC_CTRL_2_REG(port), regVal);

	if (isInband) {

		/* set Inband AN enable in MAC Control 2 */
		regVal = MV_REG_READ(NETA_GMAC_CTRL_2_REG(port));
		regVal |= NETA_GMAC_INBAND_AN_MODE_MASK;
		MV_REG_WRITE(NETA_GMAC_CTRL_2_REG(port), regVal);

		/* set portType to SGMII (encoding) in MAC Control 0 */
		regVal = MV_REG_READ(NETA_GMAC_CTRL_0_REG(port));
		regVal &= ~NETA_GMAC_PORT_TYPE_MASK;
		regVal |= NETA_GMAC_PORT_TYPE_SGMII;
		MV_REG_WRITE(NETA_GMAC_CTRL_0_REG(port), regVal);

		/* in case of SGMII mode enable InBand AutoNeg */
		regVal = MV_REG_READ(NETA_GMAC_AN_CTRL_REG(port));
		regVal &= ~NETA_ENABLE_FLOW_CONTROL_AUTO_NEG_MASK;
		regVal |= NETA_INBAND_AN_EN_MASK;
		MV_REG_WRITE(NETA_GMAC_AN_CTRL_REG(port), regVal);

		/* Enable 1MS clock generation for SGMII */
		regVal = MV_REG_READ(NETA_GMAC_CLOCK_DIVIDER_REG(port));
		regVal |= NETA_GMAC_1MS_CLOCK_ENABLE_BIT_MASK;
		MV_REG_WRITE(NETA_GMAC_CLOCK_DIVIDER_REG(port), regVal);

	}

}


void mvNetaPortPowerUp(int port, MV_BOOL isSgmii, MV_BOOL isRgmii, MV_BOOL isInband)
{
	MV_U32 regVal;

	/* MAC Cause register should be cleared */
	MV_REG_WRITE(ETH_UNIT_INTR_CAUSE_REG(port), 0);

	if (isSgmii)
		mvNetaPortSgmiiConfig(port, isInband);

	mvEthGmacRgmiiSet(port, isRgmii);

	/* Cancel Port Reset */
	regVal = MV_REG_READ(NETA_GMAC_CTRL_2_REG(port));
	regVal &= (~NETA_GMAC_PORT_RESET_MASK);
	MV_REG_WRITE(NETA_GMAC_CTRL_2_REG(port), regVal);
	while ((MV_REG_READ(NETA_GMAC_CTRL_2_REG(port)) & NETA_GMAC_PORT_RESET_MASK) != 0)
		continue;
}

void mvNetaPortPowerDown(int port)
{
}

/******************************************************************************/
/*                          Port Configuration functions                      */
/******************************************************************************/

/*******************************************************************************
* mvNetaMaxRxSizeSet -
*
* DESCRIPTION:
*       Change maximum receive size of the port. This configuration will take place
*       imidiately.
*
* INPUT:
*
* RETURN:
*******************************************************************************/
MV_STATUS mvNetaMaxRxSizeSet(int portNo, int maxRxSize)
{
    MV_U32		regVal;

	if (!MV_PON_PORT(portNo)) {

		regVal =  MV_REG_READ(NETA_GMAC_CTRL_0_REG(portNo));
		regVal &= ~NETA_GMAC_MAX_RX_SIZE_MASK;
		regVal |= (((maxRxSize - MV_ETH_MH_SIZE) / 2) << NETA_GMAC_MAX_RX_SIZE_OFFS);
		MV_REG_WRITE(NETA_GMAC_CTRL_0_REG(portNo), regVal);
/*
		mvOsPrintf("%s: port=%d, maxRxSize=%d, regAddr=0x%x, regVal=0x%x\n",
			__func__, portNo, maxRxSize, NETA_GMAC_CTRL_0_REG(portNo), regVal);
*/
	}
	return MV_OK;
}

/*******************************************************************************
* mvNetaForceLinkModeSet -
*
* DESCRIPTION:
*       Sets "Force Link Pass" and "Do Not Force Link Fail" bits.
* 	Note: This function should only be called when the port is disabled.
*
* INPUT:
* 	int		portNo			- port number
* 	MV_BOOL force_link_pass	- Force Link Pass
* 	MV_BOOL force_link_fail - Force Link Failure
*		0, 0 - normal state: detect link via PHY and connector
*		1, 1 - prohibited state.
*
* RETURN:
*******************************************************************************/
MV_STATUS mvNetaForceLinkModeSet(int portNo, MV_BOOL force_link_up, MV_BOOL force_link_down)
{
	MV_U32	regVal;

	if ((portNo < 0) || (portNo >= mvNetaHalData.maxPort))
		return MV_BAD_PARAM;

	/* Can't force link pass and link fail at the same time */
	if ((force_link_up) && (force_link_down))
		return MV_BAD_PARAM;

	regVal = MV_REG_READ(NETA_GMAC_AN_CTRL_REG(portNo));

	if (force_link_up)
		regVal |= NETA_FORCE_LINK_PASS_MASK;
	else
		regVal &= ~NETA_FORCE_LINK_PASS_MASK;

	if (force_link_down)
		regVal |= NETA_FORCE_LINK_FAIL_MASK;
	else
		regVal &= ~NETA_FORCE_LINK_FAIL_MASK;

	MV_REG_WRITE(NETA_GMAC_AN_CTRL_REG(portNo), regVal);

    return MV_OK;
}

/*******************************************************************************
* mvNetaSpeedDuplexSet -
*
* DESCRIPTION:
*       Sets port speed to Auto Negotiation / 1000 / 100 / 10 Mbps.
*	Sets port duplex to Auto Negotiation / Full / Half Duplex.
*
* INPUT:
* 	int portNo - port number
* 	MV_ETH_PORT_SPEED speed - port speed
*	MV_ETH_PORT_DUPLEX duplex - port duplex mode
*
* RETURN:
*******************************************************************************/
MV_STATUS mvNetaSpeedDuplexSet(int portNo, MV_ETH_PORT_SPEED speed, MV_ETH_PORT_DUPLEX duplex)
{
	MV_U32 regVal;

	if ((portNo < 0) || (portNo >= mvNetaHalData.maxPort))
		return MV_BAD_PARAM;

	/* Check validity */
	if ((speed == MV_ETH_SPEED_1000) && (duplex == MV_ETH_DUPLEX_HALF))
		return MV_BAD_PARAM;

	regVal = MV_REG_READ(NETA_GMAC_AN_CTRL_REG(portNo));

	switch (speed) {
	case MV_ETH_SPEED_AN:
		regVal |= NETA_ENABLE_SPEED_AUTO_NEG_MASK;
		/* the other bits don't matter in this case */
		break;
	case MV_ETH_SPEED_1000:
		regVal &= ~NETA_ENABLE_SPEED_AUTO_NEG_MASK;
		regVal |= NETA_SET_GMII_SPEED_1000_MASK;
		regVal &= ~NETA_SET_MII_SPEED_100_MASK;
		/* the 100/10 bit doesn't matter in this case */
		break;
	case MV_ETH_SPEED_100:
		regVal &= ~NETA_ENABLE_SPEED_AUTO_NEG_MASK;
		regVal &= ~NETA_SET_GMII_SPEED_1000_MASK;
		regVal |= NETA_SET_MII_SPEED_100_MASK;
		break;
	case MV_ETH_SPEED_10:
		regVal &= ~NETA_ENABLE_SPEED_AUTO_NEG_MASK;
		regVal &= ~NETA_SET_GMII_SPEED_1000_MASK;
		regVal &= ~NETA_SET_MII_SPEED_100_MASK;
		break;
	default:
		mvOsPrintf("Unexpected Speed value %d\n", speed);
		return MV_BAD_PARAM;
	}

	switch (duplex) {
	case MV_ETH_DUPLEX_AN:
		regVal  |= NETA_ENABLE_DUPLEX_AUTO_NEG_MASK;
		/* the other bits don't matter in this case */
		break;
	case MV_ETH_DUPLEX_HALF:
		regVal &= ~NETA_ENABLE_DUPLEX_AUTO_NEG_MASK;
		regVal &= ~NETA_SET_FULL_DUPLEX_MASK;
		break;
	case MV_ETH_DUPLEX_FULL:
		regVal &= ~NETA_ENABLE_DUPLEX_AUTO_NEG_MASK;
		regVal |= NETA_SET_FULL_DUPLEX_MASK;
		break;
	default:
		mvOsPrintf("Unexpected Duplex value %d\n", duplex);
		return MV_BAD_PARAM;
	}

	MV_REG_WRITE(NETA_GMAC_AN_CTRL_REG(portNo), regVal);
	return MV_OK;
}

/*******************************************************************************
* mvNetaSpeedDuplexGet -
*
* DESCRIPTION:
*       Gets port speed
*	Gets port duplex
*
* INPUT:
* 	int portNo - port number
* OUTPUT:
* 	MV_ETH_PORT_SPEED *speed - port speed
*	MV_ETH_PORT_DUPLEX *duplex - port duplex mode
*
* RETURN:
*******************************************************************************/
MV_STATUS mvNetaSpeedDuplexGet(int portNo, MV_ETH_PORT_SPEED *speed, MV_ETH_PORT_DUPLEX *duplex)
{
	MV_U32 regVal;
	if ((portNo < 0) || (portNo >= mvNetaHalData.maxPort))
		return MV_BAD_PARAM;

	/* Check validity */
	if (!speed || !duplex)
		return MV_BAD_PARAM;

	regVal = MV_REG_READ(NETA_GMAC_AN_CTRL_REG(portNo));
	if (regVal & NETA_ENABLE_SPEED_AUTO_NEG_MASK)
		*speed = MV_ETH_SPEED_AN;
	else if (regVal & NETA_SET_GMII_SPEED_1000_MASK)
		*speed = MV_ETH_SPEED_1000;
	else if (regVal & NETA_SET_MII_SPEED_100_MASK)
		*speed = MV_ETH_SPEED_100;
	else
		*speed = MV_ETH_SPEED_10;

	if (regVal & NETA_ENABLE_DUPLEX_AUTO_NEG_MASK)
		*duplex = MV_ETH_DUPLEX_AN;
	else if (regVal & NETA_SET_FULL_DUPLEX_MASK)
		*duplex = MV_ETH_DUPLEX_FULL;
	else
		*duplex = MV_ETH_DUPLEX_HALF;

	return MV_OK;
}

/*******************************************************************************
* mvNetaFlowCtrlSet - Set Flow Control of the port.
*
* DESCRIPTION:
*       This function configures the port's Flow Control properties.
*
* INPUT:
*       int				port		- Port number
*       MV_ETH_PORT_FC  flowControl - Flow control of the port.
*
* RETURN:   MV_STATUS
*       MV_OK           - Success
*       MV_OUT_OF_RANGE - Failed. Port is out of valid range
*       MV_BAD_VALUE    - Value flowControl parameters is not valid
*
*******************************************************************************/
MV_STATUS mvNetaFlowCtrlSet(int port, MV_ETH_PORT_FC flowControl)
{
	MV_U32 regVal;

	if ((port < 0) || (port >= mvNetaHalData.maxPort))
		return MV_OUT_OF_RANGE;

	regVal = MV_REG_READ(NETA_GMAC_AN_CTRL_REG(port));

	switch (flowControl) {
	case MV_ETH_FC_AN_NO:
		regVal |= NETA_ENABLE_FLOW_CONTROL_AUTO_NEG_MASK;
		regVal &= ~NETA_FLOW_CONTROL_ADVERTISE_MASK;
		regVal &= ~NETA_FLOW_CONTROL_ASYMETRIC_MASK;
		break;

	case MV_ETH_FC_AN_SYM:
		regVal |= NETA_ENABLE_FLOW_CONTROL_AUTO_NEG_MASK;
		regVal |= NETA_FLOW_CONTROL_ADVERTISE_MASK;
		regVal &= ~NETA_FLOW_CONTROL_ASYMETRIC_MASK;
		break;

	case MV_ETH_FC_AN_ASYM:
		regVal |= NETA_ENABLE_FLOW_CONTROL_AUTO_NEG_MASK;
		regVal |= NETA_FLOW_CONTROL_ADVERTISE_MASK;
		regVal |= NETA_FLOW_CONTROL_ASYMETRIC_MASK;
		break;

	case MV_ETH_FC_DISABLE:
		regVal &= ~NETA_ENABLE_FLOW_CONTROL_AUTO_NEG_MASK;
		regVal &= ~NETA_SET_FLOW_CONTROL_MASK;
		break;

	case MV_ETH_FC_ENABLE:
		regVal &= ~NETA_ENABLE_FLOW_CONTROL_AUTO_NEG_MASK;
		regVal |= NETA_SET_FLOW_CONTROL_MASK;
		break;

	default:
		mvOsPrintf("ethDrv: Unexpected FlowControl value %d\n", flowControl);
		return MV_BAD_VALUE;
	}

	MV_REG_WRITE(NETA_GMAC_AN_CTRL_REG(port), regVal);

	return MV_OK;
}

/*******************************************************************************
* mvNetaFlowCtrlGet - Get Flow Control configuration of the port.
*
* DESCRIPTION:
*       This function returns the port's Flow Control properties.
*
* INPUT:
*       int				port		- Port number
*
* OUTPUT:
*       MV_ETH_PORT_FC  *flowCntrl	- Flow control of the port.
*
* RETURN:   MV_STATUS
*       MV_OK           - Success
*       MV_OUT_OF_RANGE - Failed. Port is out of valid range
*
*******************************************************************************/
MV_STATUS mvNetaFlowCtrlGet(int port, MV_ETH_PORT_FC *pFlowCntrl)
{
	MV_U32 regVal;

	if ((port < 0) || (port >= mvNetaHalData.maxPort))
		return MV_OUT_OF_RANGE;

	regVal = MV_REG_READ(NETA_GMAC_AN_CTRL_REG(port));

	if (regVal & NETA_ENABLE_FLOW_CONTROL_AUTO_NEG_MASK) {
		/* Auto negotiation is enabled */
		if (regVal & NETA_FLOW_CONTROL_ADVERTISE_MASK) {
			if (regVal & NETA_FLOW_CONTROL_ASYMETRIC_MASK)
				*pFlowCntrl = MV_ETH_FC_AN_ASYM;
			else
				*pFlowCntrl = MV_ETH_FC_AN_SYM;
		} else
			*pFlowCntrl = MV_ETH_FC_AN_NO;
	} else {
		/* Auto negotiation is disabled */
		if (regVal & NETA_SET_FLOW_CONTROL_MASK)
			*pFlowCntrl = MV_ETH_FC_ENABLE;
		else
			*pFlowCntrl = MV_ETH_FC_DISABLE;
	}
	return MV_OK;
}

MV_STATUS mvNetaPortEnable(int port)
{
	if (!MV_PON_PORT(port)) {
		MV_U32 regVal;

		/* Enable port */
		regVal = MV_REG_READ(NETA_GMAC_CTRL_0_REG(port));
		regVal |= NETA_GMAC_PORT_EN_MASK;

		MV_REG_WRITE(NETA_GMAC_CTRL_0_REG(port), regVal);

		/* If Link is UP, Start RX and TX traffic */
		if (MV_REG_READ(NETA_GMAC_STATUS_REG(port)) & NETA_GMAC_LINK_UP_MASK)
			return mvNetaPortUp(port);
	}
	return MV_NOT_READY;
}

MV_STATUS mvNetaPortDisable(int port)
{
	MV_U32 regData;

	mvNetaPortDown(port);

	if (!MV_PON_PORT(port)) {
		/* Reset the Enable bit in the Serial Control Register */
		regData = MV_REG_READ(NETA_GMAC_CTRL_0_REG(port));
		regData &= ~(NETA_GMAC_PORT_EN_MASK);
		MV_REG_WRITE(NETA_GMAC_CTRL_0_REG(port), regData);
	}
	/* Wait about 200 usec */
	mvOsUDelay(200);

	return MV_OK;
}

MV_BOOL mvNetaLinkIsUp(int port)
{
	MV_U32	regVal;

	if (MV_PON_PORT(port))
		return MV_TRUE;

	regVal = MV_REG_READ(NETA_GMAC_STATUS_REG(port));
	if (regVal & NETA_GMAC_LINK_UP_MASK)
		return MV_TRUE;

	return MV_FALSE;
}

MV_STATUS mvNetaLinkStatus(int port, MV_ETH_PORT_STATUS *pStatus)
{
	MV_U32 regVal;

	if (MV_PON_PORT(port)) {
		/* FIXME: --BK */
		pStatus->linkup = MV_TRUE;
		pStatus->speed = MV_ETH_SPEED_1000;
		pStatus->duplex = MV_ETH_DUPLEX_FULL;
		pStatus->rxFc = MV_ETH_FC_DISABLE;
		pStatus->txFc = MV_ETH_FC_DISABLE;
		return MV_OK;
	}

	regVal = MV_REG_READ(NETA_GMAC_STATUS_REG(port));

	if (regVal & NETA_GMAC_SPEED_1000_MASK)
		pStatus->speed = MV_ETH_SPEED_1000;
	else if (regVal & NETA_GMAC_SPEED_100_MASK)
		pStatus->speed = MV_ETH_SPEED_100;
	else
		pStatus->speed = MV_ETH_SPEED_10;

	if (regVal & NETA_GMAC_LINK_UP_MASK)
		pStatus->linkup = MV_TRUE;
	else
		pStatus->linkup = MV_FALSE;

	if (regVal & NETA_GMAC_FULL_DUPLEX_MASK)
		pStatus->duplex = MV_ETH_DUPLEX_FULL;
	else
		pStatus->duplex = MV_ETH_DUPLEX_HALF;

	if (regVal & NETA_TX_FLOW_CTRL_ACTIVE_MASK)
		pStatus->txFc = MV_ETH_FC_ACTIVE;
	else if (regVal & NETA_TX_FLOW_CTRL_ENABLE_MASK)
		pStatus->txFc = MV_ETH_FC_ENABLE;
	else
		pStatus->txFc = MV_ETH_FC_DISABLE;

	if (regVal & NETA_RX_FLOW_CTRL_ACTIVE_MASK)
		pStatus->rxFc = MV_ETH_FC_ACTIVE;
	else if (regVal & NETA_RX_FLOW_CTRL_ENABLE_MASK)
		pStatus->rxFc = MV_ETH_FC_ENABLE;
	else
		pStatus->rxFc = MV_ETH_FC_DISABLE;

	return MV_OK;
}

/* Set Low Power Idle mode for GMAC port */
MV_STATUS           mvNetaGmacLpiSet(int port, int mode)
{
	if (!MV_PON_PORT(port))	{
		MV_U32  regVal;

		regVal = MV_REG_READ(NETA_LOW_POWER_CTRL_1_REG(port));
		if (mode)
			regVal |= NETA_LPI_REQUEST_EN_MASK;
		else
			regVal &= ~NETA_LPI_REQUEST_EN_MASK;

		MV_REG_WRITE(NETA_LOW_POWER_CTRL_1_REG(port), regVal);

		return MV_OK;
	}
	return MV_FAIL;
}

#else	/* Old GMAC functions */

static void mvNetaPortSgmiiConfig(int port, MV_BOOL isInband)
{
	MV_U32 regVal;

	regVal = MV_REG_READ(ETH_PORT_SERIAL_CTRL_1_REG(port));
	regVal |= (/*ETH_SGMII_MODE_MASK |*/ ETH_PSC_ENABLE_MASK /*| ETH_INBAND_AUTO_NEG_ENABLE_MASK */);
	/* regVal &= (~ETH_INBAND_AUTO_NEG_BYPASS_MASK); */
	MV_REG_WRITE(ETH_PORT_SERIAL_CTRL_1_REG(port), regVal);
}

void mvNetaPortPowerUp(int port, MV_BOOL isSgmii, MV_BOOL isRgmii,  MV_BOOL isInband)
{
	MV_U32 regVal;

	/* MAC Cause register should be cleared */
	MV_REG_WRITE(ETH_UNIT_INTR_CAUSE_REG(port), 0);


	if (isSgmii)
		mvNetaPortSgmiiConfig(port, isInband);

	/* Cancel Port Reset */
	regVal = MV_REG_READ(ETH_PORT_SERIAL_CTRL_1_REG(port));
	regVal &= (~ETH_PORT_RESET_MASK);

	if (isRgmii)
		regVal |= ETH_RGMII_ENABLE_MASK;
	else
		regVal &= (~ETH_RGMII_ENABLE_MASK);

	MV_REG_WRITE(ETH_PORT_SERIAL_CTRL_1_REG(port), regVal);
	while ((MV_REG_READ(ETH_PORT_SERIAL_CTRL_1_REG(port)) & ETH_PORT_RESET_MASK) != 0)
		continue;
}

void mvNetaPortPowerDown(int port)
{
}

/******************************************************************************/
/*                          Port Configuration functions                      */
/******************************************************************************/

/*******************************************************************************
* netaMruGet - Get MRU configuration for Max Rx packet size.
*
* INPUT:
*           MV_U32 maxRxPktSize - max  packet size.
*
* RETURN:   MV_U32 - MRU configuration.
*
*******************************************************************************/
static MV_U32 netaMruGet(MV_U32 maxRxPktSize)
{
	MV_U32 portSerialCtrlReg = 0;

	if (maxRxPktSize > 9192)
		portSerialCtrlReg |= ETH_MAX_RX_PACKET_9700BYTE;
	else if (maxRxPktSize > 9022)
		portSerialCtrlReg |= ETH_MAX_RX_PACKET_9192BYTE;
	else if (maxRxPktSize > 1552)
		portSerialCtrlReg |= ETH_MAX_RX_PACKET_9022BYTE;
	else if (maxRxPktSize > 1522)
		portSerialCtrlReg |= ETH_MAX_RX_PACKET_1552BYTE;
	else if (maxRxPktSize > 1518)
		portSerialCtrlReg |= ETH_MAX_RX_PACKET_1522BYTE;
	else
		portSerialCtrlReg |= ETH_MAX_RX_PACKET_1518BYTE;

	return portSerialCtrlReg;
}

/*******************************************************************************
* mvNetaMaxRxSizeSet -
*
* DESCRIPTION:
*       Change maximum receive size of the port. This configuration will take place
*       imidiately.
*
* INPUT:
*
* RETURN:
*******************************************************************************/
MV_STATUS mvNetaMaxRxSizeSet(int portNo, int maxRxSize)
{
	MV_U32 portSerialCtrlReg;

	portSerialCtrlReg = MV_REG_READ(ETH_PORT_SERIAL_CTRL_REG(portNo));
	portSerialCtrlReg &= ~ETH_MAX_RX_PACKET_SIZE_MASK;
	portSerialCtrlReg |= netaMruGet(maxRxSize);
	MV_REG_WRITE(ETH_PORT_SERIAL_CTRL_REG(portNo), portSerialCtrlReg);

	return MV_OK;
}

/*******************************************************************************
* mvNetaForceLinkModeSet -
*
* DESCRIPTION:
*       Sets "Force Link Pass" and "Do Not Force Link Fail" bits.
* 	Note: This function should only be called when the port is disabled.
*
* INPUT:
* 	int portNo - port number
* 	MV_BOOL force_link_pass - value for Force Link Pass bit (bit 1): 0 or 1
* 	MV_BOOL do_not_force_link_fail - value for Do Not Force Link Fail bit (bit 10): 0 or 1
*
* RETURN:
*******************************************************************************/
MV_STATUS mvNetaForceLinkModeSet(int portNo, MV_BOOL force_link_up, MV_BOOL force_link_down)
{
	MV_U32 portSerialCtrlReg;

	if ((portNo < 0) || (portNo >= mvNetaHalData.maxPort))
		return MV_BAD_PARAM;

	/* Can't force link pass and link fail at the same time */
	if ((force_link_up) && (force_link_down))
		return MV_BAD_PARAM;

	portSerialCtrlReg = MV_REG_READ(ETH_PORT_SERIAL_CTRL_REG(portNo));

	if (force_link_up)
		portSerialCtrlReg |= ETH_FORCE_LINK_PASS_MASK | ETH_DO_NOT_FORCE_LINK_FAIL_MASK;
	else if (force_link_down)
		portSerialCtrlReg &= ~(ETH_FORCE_LINK_PASS_MASK | ETH_DO_NOT_FORCE_LINK_FAIL_MASK);

	MV_REG_WRITE(ETH_PORT_SERIAL_CTRL_REG(portNo), portSerialCtrlReg);

	return MV_OK;
}

/*******************************************************************************
* mvNetaSpeedDuplexSet -
*
* DESCRIPTION:
*       Sets port speed to Auto Negotiation / 1000 / 100 / 10 Mbps.
*	Sets port duplex to Auto Negotiation / Full / Half Duplex.
*
* INPUT:
* 	int portNo - port number
* 	MV_ETH_PORT_SPEED speed - port speed
*	MV_ETH_PORT_DUPLEX duplex - port duplex mode
*
* RETURN:
*******************************************************************************/
MV_STATUS mvNetaSpeedDuplexSet(int portNo, MV_ETH_PORT_SPEED speed, MV_ETH_PORT_DUPLEX duplex)
{
	MV_U32 portSerialCtrlReg;

	if ((portNo < 0) || (portNo >= mvNetaHalData.maxPort))
		return MV_BAD_PARAM;

	/* Check validity */
	if ((speed == MV_ETH_SPEED_1000) && (duplex == MV_ETH_DUPLEX_HALF))
		return MV_BAD_PARAM;

	portSerialCtrlReg = MV_REG_READ(ETH_PORT_SERIAL_CTRL_REG(portNo));

	switch (speed) {
	case MV_ETH_SPEED_AN:
		portSerialCtrlReg &= ~ETH_DISABLE_SPEED_AUTO_NEG_MASK;
		/* the other bits don't matter in this case */
		break;
	case MV_ETH_SPEED_1000:
		portSerialCtrlReg |= ETH_DISABLE_SPEED_AUTO_NEG_MASK;
		portSerialCtrlReg |= ETH_SET_GMII_SPEED_1000_MASK;
		/* the 100/10 bit doesn't matter in this case */
		break;
	case MV_ETH_SPEED_100:
		portSerialCtrlReg |= ETH_DISABLE_SPEED_AUTO_NEG_MASK;
		portSerialCtrlReg &= ~ETH_SET_GMII_SPEED_1000_MASK;
		portSerialCtrlReg |= ETH_SET_MII_SPEED_100_MASK;
		break;
	case MV_ETH_SPEED_10:
		portSerialCtrlReg |= ETH_DISABLE_SPEED_AUTO_NEG_MASK;
		portSerialCtrlReg &= ~ETH_SET_GMII_SPEED_1000_MASK;
		portSerialCtrlReg &= ~ETH_SET_MII_SPEED_100_MASK;
		break;
	default:
		mvOsPrintf("Unexpected Speed value %d\n", speed);
		return MV_BAD_PARAM;
	}

	switch (duplex) {
	case MV_ETH_DUPLEX_AN:
		portSerialCtrlReg &= ~ETH_DISABLE_DUPLEX_AUTO_NEG_MASK;
		/* the other bits don't matter in this case */
		break;
	case MV_ETH_DUPLEX_HALF:
		portSerialCtrlReg |= ETH_DISABLE_DUPLEX_AUTO_NEG_MASK;
		portSerialCtrlReg &= ~ETH_SET_FULL_DUPLEX_MASK;
		break;
	case MV_ETH_DUPLEX_FULL:
		portSerialCtrlReg |= ETH_DISABLE_DUPLEX_AUTO_NEG_MASK;
		portSerialCtrlReg |= ETH_SET_FULL_DUPLEX_MASK;
		break;
	default:
		mvOsPrintf("Unexpected Duplex value %d\n", duplex);
		return MV_BAD_PARAM;
	}

	MV_REG_WRITE(ETH_PORT_SERIAL_CTRL_REG(portNo), portSerialCtrlReg);

	return MV_OK;

}

/*******************************************************************************
* mvNetaSpeedDuplexGet -
*
* DESCRIPTION:
*       Gets port speed
*	Gets port duplex
*
* INPUT:
* 	int portNo - port number
* OUTPUT:
* 	MV_ETH_PORT_SPEED *speed - port speed
*	MV_ETH_PORT_DUPLEX *duplex - port duplex mode
*
* RETURN:
*******************************************************************************/
MV_STATUS mvNetaSpeedDuplexGet(int portNo, MV_ETH_PORT_SPEED *speed, MV_ETH_PORT_DUPLEX *duplex)
{
	MV_U32 regVal;
	if ((portNo < 0) || (portNo >= mvNetaHalData.maxPort))
		return MV_BAD_PARAM;

	/* Check validity */
	if (!speed || !duplex)
		return MV_BAD_PARAM;

	regVal = MV_REG_READ(ETH_PORT_SERIAL_CTRL_REG(portNo));
	if (!(regVal & ETH_DISABLE_SPEED_AUTO_NEG_MASK))
		*speed = MV_ETH_SPEED_AN;
	else if (regVal & ETH_SET_GMII_SPEED_1000_MASK)
		*speed = MV_ETH_SPEED_1000;
	else if (regVal & ETH_SET_MII_SPEED_100_MASK)
		*speed = MV_ETH_SPEED_100;
	else
		*speed = MV_ETH_SPEED_10;

	if (!(regVal & ETH_DISABLE_DUPLEX_AUTO_NEG_MASK))
		*duplex = MV_ETH_DUPLEX_AN;
	else if (regVal & ETH_SET_FULL_DUPLEX_MASK)
		*duplex = MV_ETH_DUPLEX_FULL;
	else
		*duplex = MV_ETH_DUPLEX_HALF;

	return MV_OK;
}

/*******************************************************************************
* mvNetaFlowCtrlSet - Set Flow Control of the port.
*
* DESCRIPTION:
*       This function configures the port's Flow Control properties.
*
* INPUT:
*       int				port		- Port number
*       MV_ETH_PORT_FC  flowControl - Flow control of the port.
*
* RETURN:   MV_STATUS
*       MV_OK           - Success
*       MV_OUT_OF_RANGE - Failed. Port is out of valid range
*       MV_BAD_VALUE    - Value flowControl parameters is not valid
*
*******************************************************************************/
MV_STATUS mvNetaFlowCtrlSet(int port, MV_ETH_PORT_FC flowControl)
{
	MV_U32 regVal;

	if ((port < 0) || (port >= mvNetaHalData.maxPort))
		return MV_OUT_OF_RANGE;

	regVal = MV_REG_READ(ETH_PORT_SERIAL_CTRL_REG(port));

	switch (flowControl) {
	case MV_ETH_FC_AN_NO:
		regVal &= ~ETH_DISABLE_FC_AUTO_NEG_MASK;
		regVal &= ~ETH_ADVERTISE_SYM_FC_MASK;
		break;

	case MV_ETH_FC_AN_SYM:
		regVal &= ~ETH_DISABLE_FC_AUTO_NEG_MASK;
		regVal |= ETH_ADVERTISE_SYM_FC_MASK;
		break;

	case MV_ETH_FC_DISABLE:
		regVal |= ETH_DISABLE_FC_AUTO_NEG_MASK;
		regVal &= ~ETH_SET_FLOW_CTRL_MASK;
		break;

	case MV_ETH_FC_ENABLE:
		regVal |= ETH_DISABLE_FC_AUTO_NEG_MASK;
		regVal |= ETH_SET_FLOW_CTRL_MASK;
		break;

	default:
		mvOsPrintf("ethDrv: Unexpected FlowControl value %d\n", flowControl);
		return MV_BAD_VALUE;
	}

	MV_REG_WRITE(ETH_PORT_SERIAL_CTRL_REG(port), regVal);
	return MV_OK;
}

/*******************************************************************************
* mvNetaFlowCtrlGet - Get Flow Control configuration of the port.
*
* DESCRIPTION:
*       This function returns the port's Flow Control properties.
*
* INPUT:
*       int				port		- Port number
*
* OUTPUT:
*       MV_ETH_PORT_FC  *flowCntrl	- Flow control of the port.
*
* RETURN:   MV_STATUS
*       MV_OK           - Success
*       MV_OUT_OF_RANGE - Failed. Port is out of valid range
*
*******************************************************************************/
MV_STATUS mvNetaFlowCtrlGet(int port, MV_ETH_PORT_FC *pFlowCntrl)
{
	MV_U32 regVal;

	if ((port < 0) || (port >= mvNetaHalData.maxPort))
		return MV_OUT_OF_RANGE;

	regVal = MV_REG_READ(ETH_PORT_SERIAL_CTRL_REG(port));

	if (regVal & ETH_DISABLE_FC_AUTO_NEG_MASK) {
		/* Auto negotiation is disabled */
		if (regVal & ETH_SET_FLOW_CTRL_MASK)
			*pFlowCntrl = MV_ETH_FC_ENABLE;
		else
			*pFlowCntrl = MV_ETH_FC_DISABLE;
	} else {
		/* Auto negotiation is enabled */
		if (regVal & ETH_ADVERTISE_SYM_FC_MASK)
			*pFlowCntrl = MV_ETH_FC_AN_SYM;
		else
			*pFlowCntrl = MV_ETH_FC_AN_NO;
	}
	return MV_OK;
}

MV_STATUS mvNetaPortEnable(int port)
{
	if (!MV_PON_PORT(port)) {
		MV_U32 regVal;

		/* Enable port */
		regVal = MV_REG_READ(ETH_PORT_SERIAL_CTRL_REG(port));
		regVal |= (ETH_DO_NOT_FORCE_LINK_FAIL_MASK | ETH_PORT_ENABLE_MASK);

		MV_REG_WRITE(ETH_PORT_SERIAL_CTRL_REG(port), regVal);

		/* If Link is UP, Start RX and TX traffic */
		if (MV_REG_READ(ETH_PORT_STATUS_REG(port)) & ETH_LINK_UP_MASK)
			return mvNetaPortUp(port);
	}
	return MV_NOT_READY;
}

MV_STATUS mvNetaPortDisable(int port)
{
	MV_U32 regData;

	mvNetaPortDown(port);

	if (!MV_PON_PORT(port)) {
		/* Reset the Enable bit in the Serial Control Register */
		regData = MV_REG_READ(ETH_PORT_SERIAL_CTRL_REG(port));
		regData &= ~(ETH_PORT_ENABLE_MASK);
		MV_REG_WRITE(ETH_PORT_SERIAL_CTRL_REG(port), regData);
	}
	/* Wait about 200 usec */
	mvOsUDelay(200);

	return MV_OK;
}

MV_BOOL mvNetaLinkIsUp(int port)
{
	MV_U32	regVal;

	if (MV_PON_PORT(port))
		return MV_TRUE;

	regVal = MV_REG_READ(ETH_PORT_STATUS_REG(port));
	if (regVal & ETH_LINK_UP_MASK)
		return MV_TRUE;

	return MV_FALSE;
}

MV_STATUS mvNetaLinkStatus(int port, MV_ETH_PORT_STATUS *pStatus)
{
	MV_U32 regVal;

	if (MV_PON_PORT(port)) {
		/* FIXME: --BK */
		pStatus->linkup = MV_TRUE;
		pStatus->speed = MV_ETH_SPEED_1000;
		pStatus->duplex = MV_ETH_DUPLEX_FULL;
		pStatus->rxFc = MV_ETH_FC_DISABLE;
		pStatus->txFc = MV_ETH_FC_DISABLE;
		return MV_OK;
	}

	regVal = MV_REG_READ(ETH_PORT_STATUS_REG(port));

	if (regVal & ETH_GMII_SPEED_1000_MASK)
		pStatus->speed = MV_ETH_SPEED_1000;
	else if (regVal & ETH_MII_SPEED_100_MASK)
		pStatus->speed = MV_ETH_SPEED_100;
	else
		pStatus->speed = MV_ETH_SPEED_10;

	if (regVal & ETH_LINK_UP_MASK)
		pStatus->linkup = MV_TRUE;
	else
		pStatus->linkup = MV_FALSE;

	if (regVal & ETH_FULL_DUPLEX_MASK)
		pStatus->duplex = MV_ETH_DUPLEX_FULL;
	else
		pStatus->duplex = MV_ETH_DUPLEX_HALF;

	pStatus->txFc = MV_ETH_FC_DISABLE;
	if (regVal & ETH_FLOW_CTRL_ENABLED_MASK)
		pStatus->rxFc = MV_ETH_FC_ENABLE;
	else
		pStatus->rxFc = MV_ETH_FC_DISABLE;

	return MV_OK;
}

#endif /* MV_ETH_GMAC_NEW */

/******************************************************************************/
/*                      MAC Filtering functions                               */
/******************************************************************************/

/************************ Legacy parse function start *******************************/
/*******************************************************************************
* netaSetUcastAddr - This function Set the port unicast address table
*
* DESCRIPTION:
*       This function locates the proper entry in the Unicast table for the
*       specified MAC nibble and sets its properties according to function
*       parameters.
*
* INPUT:
*       int     portNo		- Port number.
*       MV_U8   lastNibble	- Unicast MAC Address last nibble.
*       int     queue		- Rx queue number for this MAC address.
*			value "-1" means remove address.
*
* OUTPUT:
*       This function add/removes MAC addresses from the port unicast address
*       table.
*
* RETURN:
*       MV_TRUE is output succeeded.
*       MV_FALSE if option parameter is invalid.
*
*******************************************************************************/
static MV_BOOL netaSetUcastAddr(int portNo, MV_U8 lastNibble, int queue)
{
	unsigned int unicastReg;
	unsigned int tblOffset;
	unsigned int regOffset;

	/* Locate the Unicast table entry */
	lastNibble = (0xf & lastNibble);
	tblOffset = (lastNibble / 4) * 4;	/* Register offset from unicast table base */
	regOffset = lastNibble % 4;	/* Entry offset within the above register */

	unicastReg = MV_REG_READ((ETH_DA_FILTER_UCAST_BASE(portNo) + tblOffset));

	if (queue == -1) {
		/* Clear accepts frame bit at specified unicast DA table entry */
		unicastReg &= ~(0xFF << (8 * regOffset));
	} else {
		unicastReg &= ~(0xFF << (8 * regOffset));
		unicastReg |= ((0x01 | (queue << 1)) << (8 * regOffset));
	}
	MV_REG_WRITE((ETH_DA_FILTER_UCAST_BASE(portNo) + tblOffset), unicastReg);

	return MV_TRUE;
}

/*******************************************************************************
* netaSetSpecialMcastAddr - Special Multicast address settings.
*
* DESCRIPTION:
*       This routine controls the MV device special MAC multicast support.
*       The Special Multicast Table for MAC addresses supports MAC of the form
*       0x01-00-5E-00-00-XX (where XX is between 0x00 and 0xFF).
*       The MAC DA[7:0] bits are used as a pointer to the Special Multicast
*       Table entries in the DA-Filter table.
*       This function set the Special Multicast Table appropriate entry
*       according to the argument given.
*
* INPUT:
*       int     port      Port number.
*       unsigned char   mcByte      Multicast addr last byte (MAC DA[7:0] bits).
*       int          queue      Rx queue number for this MAC address.
*       int             option      0 = Add, 1 = remove address.
*
* OUTPUT:
*       See description.
*
* RETURN:
*       MV_TRUE is output succeeded.
*       MV_FALSE if option parameter is invalid.
*
*******************************************************************************/
static MV_BOOL netaSetSpecialMcastAddr(int port, MV_U8 lastByte, int queue)
{
	unsigned int smcTableReg;
	unsigned int tblOffset;
	unsigned int regOffset;

	/* Locate the SMC table entry */
	tblOffset = (lastByte / 4);	/* Register offset from SMC table base    */
	regOffset = lastByte % 4;	/* Entry offset within the above register */

	smcTableReg = MV_REG_READ((ETH_DA_FILTER_SPEC_MCAST_BASE(port) + tblOffset * 4));

	if (queue == -1) {
		/* Clear accepts frame bit at specified Special DA table entry */
		smcTableReg &= ~(0xFF << (8 * regOffset));
	} else {
		smcTableReg &= ~(0xFF << (8 * regOffset));
		smcTableReg |= ((0x01 | (queue << 1)) << (8 * regOffset));
	}
	MV_REG_WRITE((ETH_DA_FILTER_SPEC_MCAST_BASE(port) + tblOffset * 4), smcTableReg);

	return MV_TRUE;
}

/*******************************************************************************
* netaSetOtherMcastAddr - Multicast address settings.
*
* DESCRIPTION:
*       This routine controls the MV device Other MAC multicast support.
*       The Other Multicast Table is used for multicast of another type.
*       A CRC-8bit is used as an index to the Other Multicast Table entries
*       in the DA-Filter table.
*       The function gets the CRC-8bit value from the calling routine and
*       set the Other Multicast Table appropriate entry according to the
*       CRC-8 argument given.
*
* INPUT:
*       int     port        Port number.
*       MV_U8   crc8        A CRC-8bit (Polynomial: x^8+x^2+x^1+1).
*       int     queue       Rx queue number for this MAC address.
*
* OUTPUT:
*       See description.
*
* RETURN:
*       MV_TRUE is output succeeded.
*       MV_FALSE if option parameter is invalid.
*
*******************************************************************************/
static MV_BOOL netaSetOtherMcastAddr(int port, MV_U8 crc8, int queue)
{
	unsigned int omcTableReg;
	unsigned int tblOffset;
	unsigned int regOffset;

	/* Locate the OMC table entry */
	tblOffset = (crc8 / 4) * 4;	/* Register offset from OMC table base    */
	regOffset = crc8 % 4;	/* Entry offset within the above register */

	omcTableReg = MV_REG_READ((ETH_DA_FILTER_OTH_MCAST_BASE(port) + tblOffset));

	if (queue == -1) {
		/* Clear accepts frame bit at specified Other DA table entry */
		omcTableReg &= ~(0xFF << (8 * regOffset));
	} else {
		omcTableReg &= ~(0xFF << (8 * regOffset));
		omcTableReg |= ((0x01 | (queue << 1)) << (8 * regOffset));
	}

	MV_REG_WRITE((ETH_DA_FILTER_OTH_MCAST_BASE(port) + tblOffset), omcTableReg);

	return MV_TRUE;
}

/*******************************************************************************
* mvNetaRxUnicastPromiscSet - Configure Fitering mode of Ethernet port
*
* DESCRIPTION:
*       This routine used to free buffers attached to the Rx ring and should
*       be called only when Giga Ethernet port is Down
*
* INPUT:
*		int			portNo			- Port number.
*       MV_BOOL     isPromisc       - Promiscous mode
*                                   MV_TRUE  - accept all Broadcast, Multicast
*                                              and Unicast packets
*                                   MV_FALSE - accept all Broadcast,
*                                              specially added Multicast and
*                                              single Unicast packets
*
* RETURN:   MV_STATUS   MV_OK - Success, Other - Failure
*
*******************************************************************************/
MV_STATUS mvNetaRxUnicastPromiscSet(int port, MV_BOOL isPromisc)
{
	MV_U32 portCfgReg, regVal;

	portCfgReg = MV_REG_READ(ETH_PORT_CONFIG_REG(port));
	regVal = MV_REG_READ(ETH_TYPE_PRIO_REG(port));

	/* Set / Clear UPM bit in port configuration register */
	if (isPromisc == MV_TRUE) {
		/* Accept all Unicast addresses */
		portCfgReg |= ETH_UNICAST_PROMISCUOUS_MODE_MASK;
		regVal |= ETH_FORCE_UNICAST_MASK;
		MV_REG_WRITE(ETH_MAC_ADDR_LOW_REG(port), 0xFFFF);
		MV_REG_WRITE(ETH_MAC_ADDR_HIGH_REG(port), 0xFFFFFFFF);
	} else {
		/* Reject all Unicast addresses */
		portCfgReg &= ~ETH_UNICAST_PROMISCUOUS_MODE_MASK;
		regVal &= ~ETH_FORCE_UNICAST_MASK;
	}
	MV_REG_WRITE(ETH_PORT_CONFIG_REG(port), portCfgReg);
	MV_REG_WRITE(ETH_TYPE_PRIO_REG(port), regVal);

	return MV_OK;
}

/*******************************************************************************
* mvNetaMacAddrSet - This function Set the port Unicast address.
*
* DESCRIPTION:
*       This function Set the port Ethernet MAC address. This address
*       will be used to send Pause frames if enabled. Packets with this
*       address will be accepted and dispatched to default RX queue
*
* INPUT:
*       int*    port    - Ethernet port.
*       char*   pAddr   - Address to be set
*
* RETURN:   MV_STATUS
*               MV_OK - Success,  Other - Faulure
*
*******************************************************************************/
MV_STATUS mvNetaMacAddrSet(int portNo, unsigned char *pAddr, int queue)
{
	unsigned int macH;
	unsigned int macL;

	if (queue >= CONFIG_MV_ETH_RXQ) {
		mvOsPrintf("ethDrv: RX queue #%d is out of range\n", queue);
		return MV_BAD_PARAM;
	}

	if (queue != -1) {
		macL = (pAddr[4] << 8) | (pAddr[5]);
		macH = (pAddr[0] << 24) | (pAddr[1] << 16) | (pAddr[2] << 8) | (pAddr[3] << 0);

		MV_REG_WRITE(ETH_MAC_ADDR_LOW_REG(portNo), macL);
		MV_REG_WRITE(ETH_MAC_ADDR_HIGH_REG(portNo), macH);
	}

	/* Accept frames of this address */
	netaSetUcastAddr(portNo, pAddr[5], queue);

	return MV_OK;
}

/*******************************************************************************
* mvNetaMacAddrGet - This function returns the port Unicast address.
*
* DESCRIPTION:
*       This function returns the port Ethernet MAC address.
*
* INPUT:
*       int     portNo          - Ethernet port number.
*       char*   pAddr           - Pointer where address will be written to
*
* RETURN:   MV_STATUS
*               MV_OK - Success,  Other - Faulure
*
*******************************************************************************/
MV_STATUS mvNetaMacAddrGet(int portNo, unsigned char *pAddr)
{
	unsigned int macH;
	unsigned int macL;

	if (pAddr == NULL) {
		mvOsPrintf("mvNetaMacAddrGet: NULL pointer.\n");
		return MV_BAD_PARAM;
	}

	macH = MV_REG_READ(ETH_MAC_ADDR_HIGH_REG(portNo));
	macL = MV_REG_READ(ETH_MAC_ADDR_LOW_REG(portNo));
	pAddr[0] = (macH >> 24) & 0xff;
	pAddr[1] = (macH >> 16) & 0xff;
	pAddr[2] = (macH >> 8) & 0xff;
	pAddr[3] = macH & 0xff;
	pAddr[4] = (macL >> 8) & 0xff;
	pAddr[5] = macL & 0xff;

	return MV_OK;
}

/*******************************************************************************
* mvNetaMcastCrc8Get - Calculate CRC8 of MAC address.
*
* DESCRIPTION:
*
* INPUT:
*       MV_U8*  pAddr           - Address to calculate CRC-8
*
* RETURN: MV_U8 - CRC-8 of this MAC address
*
*******************************************************************************/
static MV_U8 mvNetaMcastCrc8Get(MV_U8 *pAddr)
{
	unsigned int macH;
	unsigned int macL;
	int macArray[48];
	int crc[8];
	int i;
	unsigned char crcResult = 0;

	/* Calculate CRC-8 out of the given address */
	macH = (pAddr[0] << 8) | (pAddr[1]);
	macL = (pAddr[2] << 24) | (pAddr[3] << 16) | (pAddr[4] << 8) | (pAddr[5] << 0);

	for (i = 0; i < 32; i++)
		macArray[i] = (macL >> i) & 0x1;

	for (i = 32; i < 48; i++)
		macArray[i] = (macH >> (i - 32)) & 0x1;

	crc[0] = macArray[45] ^ macArray[43] ^ macArray[40] ^ macArray[39] ^
	    macArray[35] ^ macArray[34] ^ macArray[31] ^ macArray[30] ^
	    macArray[28] ^ macArray[23] ^ macArray[21] ^ macArray[19] ^
	    macArray[18] ^ macArray[16] ^ macArray[14] ^ macArray[12] ^
	    macArray[8] ^ macArray[7] ^ macArray[6] ^ macArray[0];

	crc[1] = macArray[46] ^ macArray[45] ^ macArray[44] ^ macArray[43] ^
	    macArray[41] ^ macArray[39] ^ macArray[36] ^ macArray[34] ^
	    macArray[32] ^ macArray[30] ^ macArray[29] ^ macArray[28] ^
	    macArray[24] ^ macArray[23] ^ macArray[22] ^ macArray[21] ^
	    macArray[20] ^ macArray[18] ^ macArray[17] ^ macArray[16] ^
	    macArray[15] ^ macArray[14] ^ macArray[13] ^ macArray[12] ^
	    macArray[9] ^ macArray[6] ^ macArray[1] ^ macArray[0];

	crc[2] = macArray[47] ^ macArray[46] ^ macArray[44] ^ macArray[43] ^
	    macArray[42] ^ macArray[39] ^ macArray[37] ^ macArray[34] ^
	    macArray[33] ^ macArray[29] ^ macArray[28] ^ macArray[25] ^
	    macArray[24] ^ macArray[22] ^ macArray[17] ^ macArray[15] ^
	    macArray[13] ^ macArray[12] ^ macArray[10] ^ macArray[8] ^
	    macArray[6] ^ macArray[2] ^ macArray[1] ^ macArray[0];

	crc[3] = macArray[47] ^ macArray[45] ^ macArray[44] ^ macArray[43] ^
	    macArray[40] ^ macArray[38] ^ macArray[35] ^ macArray[34] ^
	    macArray[30] ^ macArray[29] ^ macArray[26] ^ macArray[25] ^
	    macArray[23] ^ macArray[18] ^ macArray[16] ^ macArray[14] ^
	    macArray[13] ^ macArray[11] ^ macArray[9] ^ macArray[7] ^ macArray[3] ^ macArray[2] ^ macArray[1];

	crc[4] = macArray[46] ^ macArray[45] ^ macArray[44] ^ macArray[41] ^
	    macArray[39] ^ macArray[36] ^ macArray[35] ^ macArray[31] ^
	    macArray[30] ^ macArray[27] ^ macArray[26] ^ macArray[24] ^
	    macArray[19] ^ macArray[17] ^ macArray[15] ^ macArray[14] ^
	    macArray[12] ^ macArray[10] ^ macArray[8] ^ macArray[4] ^ macArray[3] ^ macArray[2];

	crc[5] = macArray[47] ^ macArray[46] ^ macArray[45] ^ macArray[42] ^
	    macArray[40] ^ macArray[37] ^ macArray[36] ^ macArray[32] ^
	    macArray[31] ^ macArray[28] ^ macArray[27] ^ macArray[25] ^
	    macArray[20] ^ macArray[18] ^ macArray[16] ^ macArray[15] ^
	    macArray[13] ^ macArray[11] ^ macArray[9] ^ macArray[5] ^ macArray[4] ^ macArray[3];

	crc[6] = macArray[47] ^ macArray[46] ^ macArray[43] ^ macArray[41] ^
	    macArray[38] ^ macArray[37] ^ macArray[33] ^ macArray[32] ^
	    macArray[29] ^ macArray[28] ^ macArray[26] ^ macArray[21] ^
	    macArray[19] ^ macArray[17] ^ macArray[16] ^ macArray[14] ^
	    macArray[12] ^ macArray[10] ^ macArray[6] ^ macArray[5] ^ macArray[4];

	crc[7] = macArray[47] ^ macArray[44] ^ macArray[42] ^ macArray[39] ^
	    macArray[38] ^ macArray[34] ^ macArray[33] ^ macArray[30] ^
	    macArray[29] ^ macArray[27] ^ macArray[22] ^ macArray[20] ^
	    macArray[18] ^ macArray[17] ^ macArray[15] ^ macArray[13] ^
	    macArray[11] ^ macArray[7] ^ macArray[6] ^ macArray[5];

	for (i = 0; i < 8; i++)
		crcResult = crcResult | (crc[i] << i);

	return crcResult;
}

/*******************************************************************************
* mvNetaMcastAddrSet - Multicast address settings.
*
* DESCRIPTION:
*       This API controls the MV device MAC multicast support.
*       The MV device supports multicast using two tables:
*       1) Special Multicast Table for MAC addresses of the form
*          0x01-00-5E-00-00-XX (where XX is between 0x00 and 0xFF).
*          The MAC DA[7:0] bits are used as a pointer to the Special Multicast
*          Table entries in the DA-Filter table.
*          In this case, the function calls netaPortSmcAddr() routine to set the
*          Special Multicast Table.
*       2) Other Multicast Table for multicast of another type. A CRC-8bit
*          is used as an index to the Other Multicast Table entries in the
*          DA-Filter table.
*          In this case, the function calculates the CRC-8bit value and calls
*          netaPortOmcAddr() routine to set the Other Multicast Table.
*
* INPUT:
*       void*   port            - Ethernet port.
*       MV_U8*  pAddr           - Address to be set
*       int     queue           - RX queue to capture all packets with this
*                               Multicast MAC address.
*                               -1 means delete this Multicast address.
*
* RETURN: MV_STATUS
*       MV_TRUE - Success, Other - Failure
*
*******************************************************************************/
MV_STATUS mvNetaMcastAddrSet(int port, MV_U8 *pAddr, int queue)
{
	if (queue >= CONFIG_MV_ETH_RXQ) {
		mvOsPrintf("ethPort %d: RX queue #%d is out of range\n", port, queue);
		return MV_BAD_PARAM;
	}

	if ((pAddr[0] == 0x01) && (pAddr[1] == 0x00) && (pAddr[2] == 0x5E) && (pAddr[3] == 0x00) && (pAddr[4] == 0x00)) {
		netaSetSpecialMcastAddr(port, pAddr[5], queue);
	} else {
		unsigned char crcResult = 0;
		MV_NETA_PORT_CTRL *pPortCtrl = mvNetaPortHndlGet(port);

		crcResult = mvNetaMcastCrc8Get(pAddr);

		/* Check Add counter for this CRC value */
		if (queue == -1) {
			if (pPortCtrl->mcastCount[crcResult] == 0) {
				mvOsPrintf("ethPort #%d: No valid Mcast for crc8=0x%02x\n", port, (unsigned)crcResult);
				return MV_NO_SUCH;
			}

			pPortCtrl->mcastCount[crcResult]--;
			if (pPortCtrl->mcastCount[crcResult] != 0) {
				mvNetaDebugPrintf("ethPort #%d: Left %d valid Mcast for crc8=0x%02x\n",
					   pPortCtrl->portNo, pPortCtrl->mcastCount[crcResult], (unsigned)crcResult);
				return MV_NO_CHANGE;
			}
		} else {
			pPortCtrl->mcastCount[crcResult]++;
			if (pPortCtrl->mcastCount[crcResult] > 1) {
				mvNetaDebugPrintf("ethPort #%d: Exist %d valid Mcast for crc8=0x%02x\n",
					   port, pPortCtrl->mcastCount[crcResult], (unsigned)crcResult);
				return MV_NO_CHANGE;
			}
		}
		netaSetOtherMcastAddr(port, crcResult, queue);
	}
	return MV_OK;
}
/************************ Legacy parse function end *******************************/

/*******************************************************************************
* mvNetaSetUcastTable - Unicast address settings.
*
* DESCRIPTION:
*      Set all entries in the Unicast MAC Table queue==-1 means reject all
* INPUT:
*
* RETURN:
*
*******************************************************************************/
void mvNetaSetUcastTable(int port, int queue)
{
	int offset;
	MV_U32 regValue;

	if (queue == -1) {
		regValue = 0;
	} else {
		regValue = (((0x01 | (queue << 1)) << 0) |
			    ((0x01 | (queue << 1)) << 8) |
			    ((0x01 | (queue << 1)) << 16) | ((0x01 | (queue << 1)) << 24));
	}

	for (offset = 0; offset <= 0xC; offset += 4)
		MV_REG_WRITE((ETH_DA_FILTER_UCAST_BASE(port) + offset), regValue);
}

/*******************************************************************************
* mvNetaSetSpecialMcastTable - Special Multicast address settings.
*
* DESCRIPTION:
*   Set all entries to the Special Multicast MAC Table. queue==-1 means reject all
* INPUT:
*
* RETURN:
*
*******************************************************************************/
MV_VOID mvNetaSetSpecialMcastTable(int portNo, int queue)
{
	int offset;
	MV_U32 regValue;

	if (queue == -1) {
		regValue = 0;
	} else {
		regValue = (((0x01 | (queue << 1)) << 0) |
			    ((0x01 | (queue << 1)) << 8) |
			    ((0x01 | (queue << 1)) << 16) | ((0x01 | (queue << 1)) << 24));
	}

	for (offset = 0; offset <= 0xFC; offset += 4)
		MV_REG_WRITE((ETH_DA_FILTER_SPEC_MCAST_BASE(portNo) + offset), regValue);
}

/*******************************************************************************
* mvNetaSetOtherMcastTable - Other Multicast address settings.
*
* DESCRIPTION:
*   Set all entries to the Other Multicast MAC Table. queue==-1 means reject all
* INPUT:
*
* RETURN:
*
*******************************************************************************/
MV_VOID mvNetaSetOtherMcastTable(int portNo, int queue)
{
	int offset;
	MV_U32 regValue;
	MV_NETA_PORT_CTRL *pPortCtrl = mvNetaPortHndlGet(portNo);

	if (queue == -1) {
		memset(pPortCtrl->mcastCount, 0, sizeof(pPortCtrl->mcastCount));
		regValue = 0;
	} else {
		memset(pPortCtrl->mcastCount, 1, sizeof(pPortCtrl->mcastCount));
		regValue = (((0x01 | (queue << 1)) << 0) |
			    ((0x01 | (queue << 1)) << 8) |
			    ((0x01 | (queue << 1)) << 16) | ((0x01 | (queue << 1)) << 24));
	}

	for (offset = 0; offset <= 0xFC; offset += 4)
		MV_REG_WRITE((ETH_DA_FILTER_OTH_MCAST_BASE(portNo) + offset), regValue);
}

/************************ Legacy parse function start *******************************/
/*******************************************************************************
* mvNetaTosToRxqSet - Map packets with special TOS value to special RX queue
*
* DESCRIPTION:
*
* INPUT:
*		int     portNo		- Port number.
*       int     tos         - TOS value in the IP header of the packet
*       int     rxq         - RX Queue for packets with the configured TOS value
*                           Negative value (-1) means no special processing for these packets,
*                           so they will be processed as regular packets.
*
* RETURN:   MV_STATUS
*******************************************************************************/
MV_STATUS   mvNetaTosToRxqSet(int port, int tos, int rxq)
{
	MV_U32          regValue;
	int             regIdx, regOffs;

	if ((rxq < 0) || (rxq >= MV_ETH_MAX_RXQ)) {
		mvOsPrintf("eth_%d: RX queue #%d is out of range\n", port, rxq);
		return MV_BAD_PARAM;
	}
	if (tos > 0xFF) {
		mvOsPrintf("eth_%d: tos=0x%x is out of range\n", port, tos);
		return MV_BAD_PARAM;
	}
	regIdx  = mvOsDivide(tos >> 2, 10);
	regOffs = mvOsReminder(tos >> 2, 10);

	regValue = MV_REG_READ(ETH_DIFF_SERV_PRIO_REG(port, regIdx));
	regValue &= ~(0x7 << (regOffs*3));
	regValue |= (rxq << (regOffs*3));

	MV_REG_WRITE(ETH_DIFF_SERV_PRIO_REG(port, regIdx), regValue);
	return MV_OK;
}

int     mvNetaTosToRxqGet(int port, int tos)
{
	MV_U32          regValue;
	int             regIdx, regOffs, rxq;

	if (tos > 0xFF) {
		mvOsPrintf("eth_%d: tos=0x%x is out of range\n", port, tos);
		return -1;
	}
	regIdx  = mvOsDivide(tos >> 2, 10);
	regOffs = mvOsReminder(tos >> 2, 10);

	regValue = MV_REG_READ(ETH_DIFF_SERV_PRIO_REG(port, regIdx));
	rxq = (regValue >> (regOffs * 3));
	rxq &= 0x7;

	return rxq;
}

/*******************************************************************************
* mvNetaVprioToRxqSet - Map packets with special VLAN priority to special RX queue
*
* DESCRIPTION:
*
* INPUT:
*       int     portNo  - Port number.
*       int     vprio   - Vlan Priority value in packet header
*       int     rxq     - RX Queue for packets with the configured TOS value
*                         Negative value (-1) means no special processing for these packets,
*                         so they will be processed as regular packets.
*
* RETURN:   MV_STATUS
*******************************************************************************/
MV_STATUS   mvNetaVprioToRxqSet(int port, int vprio, int rxq)
{
	MV_U32          regValue;

	if ((rxq < 0) || (rxq >= MV_ETH_MAX_RXQ)) {
		mvOsPrintf("eth_%d: RX queue #%d is out of range\n", port, rxq);
		return MV_BAD_PARAM;
	}
	if (vprio > 0x7) {
		mvOsPrintf("eth_%d: vprio=0x%x is out of range\n", port, vprio);
		return MV_BAD_PARAM;
	}

	regValue = MV_REG_READ(ETH_VLAN_TAG_TO_PRIO_REG(port));
	regValue &= ~(0x7 << (vprio * 3));
	regValue |= (rxq << (vprio * 3));

	MV_REG_WRITE(ETH_VLAN_TAG_TO_PRIO_REG(port), regValue);
	return MV_OK;
}

int     mvNetaVprioToRxqGet(int port, int vprio)
{
	MV_U32          regValue;
	int             rxq;

	if (vprio > 0x7) {
		mvOsPrintf("eth_%d: vprio=0x%x is out of range\n", port, vprio);
		return -1;
	}

	regValue = MV_REG_READ(ETH_VLAN_TAG_TO_PRIO_REG(port));
	rxq = (regValue >> (vprio * 3));
	rxq &= 0x7;

	return rxq;
}
/************************ Legacy parse function end *******************************/

/******************************************************************************/
/*                         PHY Control Functions                              */
/******************************************************************************/

/*******************************************************************************
* mvNetaPhyAddrSet - Set the ethernet port PHY address.
*
* DESCRIPTION:
*       This routine set the ethernet port PHY address according to given
*       parameter.
*
* INPUT:
*       int     portNo		- Port number.
*       int     phyAddr     - PHY address
*
* RETURN:
*       None.
*
*******************************************************************************/
void mvNetaPhyAddrSet(int port, int phyAddr)
{
	unsigned int regData;

	regData = MV_REG_READ(ETH_PHY_ADDR_REG(port));

	regData &= ~ETH_PHY_ADDR_MASK;
	regData |= phyAddr;

	MV_REG_WRITE(ETH_PHY_ADDR_REG(port), regData);

	/* Enable PHY polling */
	regData = MV_REG_READ(ETH_UNIT_CONTROL_REG(port));
	regData |= ETH_PHY_POLLING_ENABLE_MASK;
	MV_REG_WRITE(ETH_UNIT_CONTROL_REG(port), regData);

	return;
}
/*******************************************************************************
* mvNetaPhyAddrPollingDisable - disable PHY polling
*
* DESCRIPTION:
*       This routine diable PHY polling
*
* INPUT:
*       int     portNo		- Port number.
*
* RETURN:
*       None.
*
*******************************************************************************/
void mvNetaPhyAddrPollingDisable(int port)
{
	unsigned int regData;

	/* Enable PHY polling */
	regData = MV_REG_READ(ETH_UNIT_CONTROL_REG(port));
	regData &= ~ETH_PHY_POLLING_ENABLE_MASK;
	MV_REG_WRITE(ETH_UNIT_CONTROL_REG(port), regData);

	return;
}

/*******************************************************************************
* mvNetaPhyAddrGet - Get the ethernet port PHY address.
*
* DESCRIPTION:
*       This routine returns the given ethernet port PHY address.
*
* INPUT:
*   int     portNo		- Port number.
*
*
* RETURN: int - PHY address.
*
*******************************************************************************/
int mvNetaPhyAddrGet(int port)
{
	unsigned int 	regData;
	int		phy;

	regData = MV_REG_READ(ETH_PHY_ADDR_REG(port));

	phy = (regData >> (5 * port));
	phy &= 0x1F;
	return phy;
}

/******************************************************************************/
/*                Descriptor handling Functions                               */
/******************************************************************************/

/*******************************************************************************
* mvNetaDescRingReset -
*
* DESCRIPTION:
*
* INPUT:
*       MV_NETA_PORT_CTRL	*pPortCtrl	NETA Port Control srtucture.
*       int			queue		Number of Rx queue.
*
* OUTPUT:
*
* RETURN: None
*
*******************************************************************************/
static void mvNetaDescRingReset(MV_NETA_QUEUE_CTRL *pQueueCtrl)
{
	int		descrNum = (pQueueCtrl->lastDesc + 1);
	char	*pDesc = pQueueCtrl->pFirst;

	if (pDesc == NULL)
		return;

	/* reset ring of descriptors */
	memset(pDesc, 0, (descrNum * NETA_DESC_ALIGNED_SIZE));
	pQueueCtrl->nextToProc = 0;
}


/* Reset all RXQs */
void mvNetaRxReset(int port)
{
	int rxq;
	MV_NETA_RXQ_CTRL *pRxqCtrl;
	MV_NETA_PORT_CTRL *pPortCtrl = mvNetaPortCtrl[port];

	MV_REG_WRITE(NETA_PORT_RX_RESET_REG(port), NETA_PORT_RX_DMA_RESET_MASK);
	for (rxq = 0; rxq < pPortCtrl->rxqNum ; rxq++) {
		pRxqCtrl = mvNetaRxqHndlGet(port, rxq);
		/* Check queue is initialized or not, if not init, skip reset */
		if (NULL == pRxqCtrl->queueCtrl.pFirst)
			continue;
		mvNetaDescRingReset(&pRxqCtrl->queueCtrl);
		mvOsCacheFlush(pPortCtrl->osHandle, pRxqCtrl->queueCtrl.pFirst,
		((pRxqCtrl->queueCtrl.lastDesc + 1) * NETA_DESC_ALIGNED_SIZE));
	}
	MV_REG_WRITE(NETA_PORT_RX_RESET_REG(port), 0);
}

/* Reset all TXQs */
void mvNetaTxpReset(int port, int txp)
{
	int txq;
	MV_NETA_TXQ_CTRL *pTxqCtrl;
	MV_NETA_PORT_CTRL *pPortCtrl = mvNetaPortCtrl[port];

	MV_REG_WRITE(NETA_PORT_TX_RESET_REG(port, txp), NETA_PORT_TX_DMA_RESET_MASK);
	for (txq = 0; txq < pPortCtrl->txqNum; txq++) {
		pTxqCtrl = mvNetaTxqHndlGet(port, txp, txq);
		/* Check queue is initialized or not, if not init, skip reset */
		if (NULL == pTxqCtrl->queueCtrl.pFirst)
			continue;
		mvNetaDescRingReset(&pTxqCtrl->queueCtrl);
		mvOsCacheFlush(pPortCtrl->osHandle, pTxqCtrl->queueCtrl.pFirst,
		((pTxqCtrl->queueCtrl.lastDesc + 1) * NETA_DESC_ALIGNED_SIZE));
	}
	MV_REG_WRITE(NETA_PORT_TX_RESET_REG(port, txp), 0);
}

/*******************************************************************************
* mvNetaRxqInit -
*
* DESCRIPTION:
*
* INPUT:
*   int     portNo		- Port number.
*   int		queue		- Number of Rx queue.
*	int		descrNum	- Number of descriptors
*
* OUTPUT:
*
* RETURN: None
*
*******************************************************************************/
MV_NETA_RXQ_CTRL *mvNetaRxqInit(int port, int queue, int descrNum)
{
	MV_NETA_PORT_CTRL *pPortCtrl = mvNetaPortCtrl[port];
	MV_NETA_RXQ_CTRL *pRxqCtrl = &pPortCtrl->pRxQueue[queue];
	MV_NETA_QUEUE_CTRL *pQueueCtrl = &pRxqCtrl->queueCtrl;
	int descSize;

	/* Allocate memory for RX descriptors */
	descSize = ((descrNum * NETA_DESC_ALIGNED_SIZE) + CPU_D_CACHE_LINE_SIZE);
	pQueueCtrl->descBuf.bufVirtPtr =
	    mvNetaDescrMemoryAlloc(pPortCtrl, descSize, &pQueueCtrl->descBuf.bufPhysAddr, &pQueueCtrl->descBuf.memHandle);

	pQueueCtrl->descBuf.bufSize = descSize;

	if (pQueueCtrl->descBuf.bufVirtPtr == NULL) {
		mvOsPrintf("EthPort #%d, rxQ=%d: Can't allocate %d bytes for %d RX descr\n",
			   pPortCtrl->portNo, queue, descSize, descrNum);
		return NULL;
	}

	/* Make sure descriptor address is cache line size aligned  */
	pQueueCtrl->pFirst = (char *)MV_ALIGN_UP((MV_ULONG) pQueueCtrl->descBuf.bufVirtPtr, CPU_D_CACHE_LINE_SIZE);

	pQueueCtrl->lastDesc = (descrNum - 1);

	mvNetaDescRingReset(pQueueCtrl);
	mvOsCacheFlush(pPortCtrl->osHandle, pQueueCtrl->pFirst, (pQueueCtrl->lastDesc + 1) * NETA_DESC_ALIGNED_SIZE);

	mvNetaRxqAddrSet(port, queue, descrNum);

	return pRxqCtrl;
}

/* Set Rx descriptors queue starting address */
void mvNetaRxqAddrSet(int port, int queue, int descrNum)
{
	MV_NETA_PORT_CTRL *pPortCtrl = mvNetaPortCtrl[port];
	MV_NETA_RXQ_CTRL *pRxqCtrl = &pPortCtrl->pRxQueue[queue];
	MV_NETA_QUEUE_CTRL *pQueueCtrl = &pRxqCtrl->queueCtrl;

	/* Check queue is initialized or not, if not init, return */
	if (NULL == pQueueCtrl->pFirst)
		return;

	/* Set Rx descriptors queue starting address */
	MV_REG_WRITE(NETA_RXQ_BASE_ADDR_REG(pPortCtrl->portNo, queue),
		     netaDescVirtToPhys(pQueueCtrl, (MV_U8 *)pQueueCtrl->pFirst));
	MV_REG_WRITE(NETA_RXQ_SIZE_REG(pPortCtrl->portNo, queue), descrNum);
}

/*******************************************************************************
* mvNetaTxqInit - Allocate required memory and initialize TXQ descriptor ring.
*
* DESCRIPTION:
*
* INPUT:
*		int     portNo		- Port number.
*		int		txp			- Number of T-CONT instance.
*		int		queue		- Number of Tx queue.
*		int		descrNum	- Number of descriptors
*
* OUTPUT:
*
* RETURN: None
*
*******************************************************************************/
MV_NETA_TXQ_CTRL *mvNetaTxqInit(int port, int txp, int queue, int descrNum)
{
	MV_NETA_PORT_CTRL *pPortCtrl = mvNetaPortCtrl[port];
	MV_NETA_TXQ_CTRL *pTxqCtrl;
	MV_NETA_QUEUE_CTRL *pQueueCtrl;
	int descSize;

	pTxqCtrl = mvNetaTxqHndlGet(port, txp, queue);
	pQueueCtrl = &pTxqCtrl->queueCtrl;

	/* Allocate memory for TX descriptors */
	descSize = ((descrNum * NETA_DESC_ALIGNED_SIZE) + CPU_D_CACHE_LINE_SIZE);
	pQueueCtrl->descBuf.bufVirtPtr =
	    mvNetaDescrMemoryAlloc(pPortCtrl, descSize, &pQueueCtrl->descBuf.bufPhysAddr, &pQueueCtrl->descBuf.memHandle);

	pQueueCtrl->descBuf.bufSize = descSize;

	if (pQueueCtrl->descBuf.bufVirtPtr == NULL) {
		mvOsPrintf("EthPort #%d, txQ=%d: Can't allocate %d bytes for %d TX descr\n",
			   pPortCtrl->portNo, queue, descSize, descrNum);
		return NULL;
	}

	/* Make sure descriptor address is cache line size aligned  */
	pQueueCtrl->pFirst = (char *)MV_ALIGN_UP((MV_ULONG) pQueueCtrl->descBuf.bufVirtPtr, CPU_D_CACHE_LINE_SIZE);

	pQueueCtrl->lastDesc = (descrNum - 1);

	mvNetaDescRingReset(pQueueCtrl);
	mvOsCacheFlush(pPortCtrl->osHandle, pQueueCtrl->pFirst, (pQueueCtrl->lastDesc + 1) * NETA_DESC_ALIGNED_SIZE);

	mvNetaTxqBandwidthSet(port, txp, queue);

	mvNetaTxqAddrSet(port, txp, queue, descrNum);

	return pTxqCtrl;
}


/* Set Rx descriptors queue starting address */
void mvNetaTxqAddrSet(int port, int txp, int queue, int descrNum)
{
	MV_NETA_TXQ_CTRL *pTxqCtrl;
	MV_NETA_QUEUE_CTRL *pQueueCtrl;

	pTxqCtrl = mvNetaTxqHndlGet(port, txp, queue);
	pQueueCtrl = &pTxqCtrl->queueCtrl;

	/* Check queue is initialized or not, if not init, return */
	if (NULL == pQueueCtrl->pFirst)
		return;

	/* Set Tx descriptors queue starting address */
	MV_REG_WRITE(NETA_TXQ_BASE_ADDR_REG(port, txp, queue), netaDescVirtToPhys(pQueueCtrl, (MV_U8 *)pQueueCtrl->pFirst));

	MV_REG_WRITE(NETA_TXQ_SIZE_REG(port, txp, queue), NETA_TXQ_DESC_NUM_MASK(descrNum));
}

/* Set maximum bandwidth for TX port */
void mvNetaTxpRateMaxSet(int port, int txp)
{
	MV_U32 regVal = NETA_TXP_REFILL_TOKENS_ALL_MASK | NETA_TXP_REFILL_PERIOD_MASK(1);

	MV_REG_WRITE(NETA_TXP_REFILL_REG(port, txp), regVal);
	MV_REG_WRITE(NETA_TXP_TOKEN_CNTR_REG(port, txp), NETA_TXP_TOKEN_CNTR_MAX);
}

/* Set maximum bandwidth for enabled TXQs */
void mvNetaTxqBandwidthSet(int port, int txp,  int queue)
{
	MV_U32 regVal = NETA_TXQ_REFILL_TOKENS_ALL_MASK | NETA_TXQ_REFILL_PERIOD_MASK(1);

	MV_REG_WRITE(NETA_TXQ_REFILL_REG(port, txp, queue), regVal);
	MV_REG_WRITE(NETA_TXQ_TOKEN_CNTR_REG(port, txp, queue), NETA_TXQ_TOKEN_CNTR_MAX);
}


/*******************************************************************************
* mvNetaRxqDelete - Delete RXQ and free memory allocated for descriptors ring.
*
* DESCRIPTION:
*
* INPUT:
*		int     port		- Port number.
*		int		queue		- Number of RX queue.
*
* OUTPUT:
*
* RETURN: None
*
*******************************************************************************/
void mvNetaRxqDelete(int port, int queue)
{
	MV_NETA_PORT_CTRL *pPortCtrl =  mvNetaPortCtrl[port];
	MV_NETA_QUEUE_CTRL *pQueueCtrl = &pPortCtrl->pRxQueue[queue].queueCtrl;

	mvNetaDescrMemoryFree(pPortCtrl, &pQueueCtrl->descBuf);

	memset(pQueueCtrl, 0, sizeof(*pQueueCtrl));

	/* Clear Rx descriptors queue starting address and size */
	MV_REG_WRITE(NETA_RXQ_BASE_ADDR_REG(port, queue), 0);
	MV_REG_WRITE(NETA_RXQ_SIZE_REG(port, queue), 0);
}

/*******************************************************************************
* mvNetaTxqDelete - Delete TXQ and free memory allocated for descriptors ring.
*
* DESCRIPTION:
*
* INPUT:
*		int     port		- Port number.
*		int		txp			- Number of T-CONT instance.
*		int		queue		- Number of Tx queue.
*
* OUTPUT:
*
* RETURN: None
*
*******************************************************************************/
void mvNetaTxqDelete(int port, int txp, int queue)
{
	MV_NETA_PORT_CTRL *pPortCtrl =  mvNetaPortCtrl[port];
	MV_NETA_QUEUE_CTRL *pQueueCtrl = &pPortCtrl->pTxQueue[queue].queueCtrl;

	mvNetaDescrMemoryFree(pPortCtrl, &pQueueCtrl->descBuf);

	memset(pQueueCtrl, 0, sizeof(*pQueueCtrl));

	/* Set minimum bandwidth for disabled TXQs */
	MV_REG_WRITE(NETA_TXQ_TOKEN_CNTR_REG(port, txp, queue), 0);

	/* Set Tx descriptors queue starting address and size */
	MV_REG_WRITE(NETA_TXQ_BASE_ADDR_REG(port, txp, queue), 0);
	MV_REG_WRITE(NETA_TXQ_SIZE_REG(port, txp, queue), 0);
}


/*******************************************************************************
* mvNetaDescrMemoryFree - Free memory allocated for RX and TX descriptors.
*
* DESCRIPTION:
*       This function frees memory allocated for RX and TX descriptors.
*
* INPUT:
*
* RETURN: None
*
*******************************************************************************/
static void mvNetaDescrMemoryFree(MV_NETA_PORT_CTRL *pPortCtrl, MV_BUF_INFO *pDescBuf)
{
	if ((pDescBuf == NULL) || (pDescBuf->bufVirtPtr == NULL))
		return;

#ifdef ETH_DESCR_UNCACHED
	mvOsIoUncachedFree(pPortCtrl->osHandle, pDescBuf->bufSize, pDescBuf->bufPhysAddr,
			   pDescBuf->bufVirtPtr, pDescBuf->memHandle);
#else
	mvOsIoCachedFree(pPortCtrl->osHandle, pDescBuf->bufSize, pDescBuf->bufPhysAddr,
			 pDescBuf->bufVirtPtr, pDescBuf->memHandle);
#endif /* ETH_DESCR_UNCACHED */
}

/*******************************************************************************
* mvNetaDescrMemoryAlloc - Allocate memory for RX and TX descriptors.
*
* DESCRIPTION:
*       This function allocates memory for RX and TX descriptors.
*
* INPUT:
*
* RETURN: None
*
*******************************************************************************/
static MV_U8 *mvNetaDescrMemoryAlloc(MV_NETA_PORT_CTRL *pPortCtrl, int descSize,
				   MV_ULONG *pPhysAddr, MV_U32 *memHandle)
{
	MV_U8 *pVirt;

#ifdef ETH_DESCR_UNCACHED
	pVirt = (MV_U8 *)mvOsIoUncachedMalloc(pPortCtrl->osHandle, descSize, pPhysAddr, memHandle);
#else
	pVirt = (MV_U8 *)mvOsIoCachedMalloc(pPortCtrl->osHandle, descSize, pPhysAddr, memHandle);
#endif /* ETH_DESCR_UNCACHED */

	if (pVirt)
		memset(pVirt, 0, descSize);

	return pVirt;
}

/***************** Configuration functions ************************/

MV_STATUS mvNetaMhSet(int port, MV_NETA_MH_MODE mh)
{
	MV_U32 regVal;

	regVal = MV_REG_READ(ETH_PORT_MARVELL_HEADER_REG(port));
	/* Clear relevant fields */
	regVal &= ~(ETH_DSA_EN_MASK | ETH_MH_EN_MASK);
	switch (mh) {
	case MV_NETA_MH_NONE:
		break;

	case MV_NETA_MH:
		regVal |= ETH_MH_EN_MASK;
		break;

	case MV_NETA_DSA:
		regVal |= ETH_DSA_MASK;
		break;

	case MV_NETA_DSA_EXT:
		regVal |= ETH_DSA_EXT_MASK;

	default:
		mvOsPrintf("port=%d: Unexpected MH = %d value\n", port, mh);
		return MV_BAD_PARAM;
	}
	MV_REG_WRITE(ETH_PORT_MARVELL_HEADER_REG(port), regVal);
	return MV_OK;
}

MV_STATUS mvNetaTagSet(int port, MV_TAG_TYPE mh)
{
	MV_U32 regVal;

	regVal = MV_REG_READ(ETH_PORT_MARVELL_HEADER_REG(port));
	/* Clear relevant fields */
	regVal &= ~(ETH_DSA_EN_MASK | ETH_MH_EN_MASK);
	switch (mh) {
	case MV_TAG_TYPE_NONE:
		break;

	case MV_TAG_TYPE_MH:
		regVal |= ETH_MH_EN_MASK;
		break;

	case MV_TAG_TYPE_DSA:
		regVal |= ETH_DSA_MASK;
		break;

	case MV_TAG_TYPE_EDSA:
		regVal |= ETH_DSA_EXT_MASK;

	default:
		mvOsPrintf("port=%d: Unexpected MH = %d value\n", port, mh);
		return MV_BAD_PARAM;
	}
	MV_REG_WRITE(ETH_PORT_MARVELL_HEADER_REG(port), regVal);
	return MV_OK;
}

/* Set one of NETA_TX_MAX_MH_REGS registers */
MV_STATUS mvNetaTxMhRegSet(int port, int txp, int reg, MV_U16 mh)
{
	/* Check params */
	if (mvNetaTxpCheck(port, txp))
		return MV_BAD_PARAM;

	if (reg >= NETA_TX_MAX_MH_REGS)
		return MV_BAD_PARAM;

	/* Write register */
	MV_REG_WRITE(NETA_TX_MH_REG(port, txp, reg), (MV_U32)mh);
		return MV_OK;
}


MV_STATUS mvNetaRxqBufSizeSet(int port, int rxq, int bufSize)
{
	MV_U32 regVal;

	regVal = MV_REG_READ(NETA_RXQ_SIZE_REG(port, rxq));

	regVal &= ~NETA_RXQ_BUF_SIZE_MASK;
	regVal |= ((bufSize >> 3) << NETA_RXQ_BUF_SIZE_OFFS);

	MV_REG_WRITE(NETA_RXQ_SIZE_REG(port, rxq), regVal);

	return MV_OK;
}

MV_STATUS mvNetaRxqTimeCoalSet(int port, int rxq, MV_U32 uSec)
{
	MV_U32 regVal;

	regVal = (mvNetaHalData.tClk / 1000000) * uSec;

	MV_REG_WRITE(NETA_RXQ_INTR_TIME_COAL_REG(port, rxq), regVal);

	return MV_OK;
}

MV_STATUS mvNetaRxqPktsCoalSet(int port, int rxq, MV_U32 pkts)
{
	MV_REG_WRITE(NETA_RXQ_THRESHOLD_REG(port, rxq),
		     (NETA_RXQ_OCCUPIED_DESC_MASK(pkts) | NETA_RXQ_NON_OCCUPIED_DESC_MASK(0)));

	return MV_OK;
}

MV_STATUS mvNetaTxDonePktsCoalSet(int port, int txp, int txq, MV_U32 pkts)
{
	MV_U32 regVal;

	regVal = MV_REG_READ(NETA_TXQ_SIZE_REG(port, txp, txq));

	regVal &= ~NETA_TXQ_SENT_DESC_TRESH_ALL_MASK;
	regVal |= NETA_TXQ_SENT_DESC_TRESH_MASK(pkts);

	MV_REG_WRITE(NETA_TXQ_SIZE_REG(port, txp, txq), regVal);

	return MV_OK;
}

MV_U32 mvNetaRxqTimeCoalGet(int port, int rxq)
{
	MV_U32 regVal, uSec;

	regVal = MV_REG_READ(NETA_RXQ_INTR_TIME_COAL_REG(port, rxq));

	uSec = regVal / (mvNetaHalData.tClk / 1000000);

	return uSec;
}

MV_U32 mvNetaRxqPktsCoalGet(int port, int rxq)
{
	MV_U32 regVal;

	regVal = MV_REG_READ(NETA_RXQ_THRESHOLD_REG(port, rxq));

	return ((regVal & NETA_RXQ_OCCUPIED_DESC_ALL_MASK) >> NETA_RXQ_OCCUPIED_DESC_OFFS);
}

MV_U32 mvNetaTxDonePktsCoalGet(int port, int txp, int txq)
{
	MV_U32 regVal;

	regVal = MV_REG_READ(NETA_TXQ_SIZE_REG(port, txp, txq));

	return ((regVal & NETA_TXQ_SENT_DESC_TRESH_ALL_MASK) >> NETA_TXQ_SENT_DESC_TRESH_OFFS);
}

/*******************************************************************************
* mvNetaPortUp - Start the Ethernet port RX and TX activity.
*
* DESCRIPTION:
*       This routine start Rx and Tx activity:
*
*       Note: Each Rx and Tx queue descriptor's list must be initialized prior
*       to calling this function (use etherInitTxDescRing for Tx queues and
*       etherInitRxDescRing for Rx queues).
*
* INPUT:
*		int     portNo		- Port number.
*
* RETURN:   MV_STATUS
*           MV_OK - Success, Others - Failure.
*
*******************************************************************************/
MV_STATUS mvNetaPortUp(int port)
{
	int queue, txp;
	MV_U32 qMap;
	MV_NETA_PORT_CTRL *pPortCtrl = mvNetaPortHndlGet(port);
	MV_NETA_QUEUE_CTRL *pQueueCtrl;

	/* Enable all initialized TXs. */
	for (txp = 0; txp < pPortCtrl->txpNum; txp++) {
		mvNetaMibCountersClear(port, txp);

		qMap = 0;
		for (queue = 0; queue < CONFIG_MV_ETH_TXQ; queue++) {
			pQueueCtrl = &pPortCtrl->pTxQueue[txp * CONFIG_MV_ETH_TXQ + queue].queueCtrl;

			if (pQueueCtrl->pFirst != NULL)
				qMap |= (1 << queue);
		}
		MV_REG_WRITE(ETH_TX_QUEUE_COMMAND_REG(pPortCtrl->portNo, txp), qMap);
	}
	/* Enable all initialized RXQs. */
	qMap = 0;
	for (queue = 0; queue < CONFIG_MV_ETH_RXQ; queue++) {
		pQueueCtrl = &pPortCtrl->pRxQueue[queue].queueCtrl;

		if (pQueueCtrl->pFirst != NULL)
			qMap |= (1 << queue);
	}
	MV_REG_WRITE(ETH_RX_QUEUE_COMMAND_REG(pPortCtrl->portNo), qMap);

/*
	mvOsPrintf("Start TX port activity: regData=0x%x (0x%x)\n",
		pPortCtrl->txqMap, MV_REG_READ(ETH_TX_QUEUE_COMMAND_REG(pPortCtrl->portNo)));
*/
	return MV_OK;
}

/*******************************************************************************
* mvNetaPortDown - Stop the Ethernet port activity.
*
* DESCRIPTION:
*
* INPUT:
*		int     portNo		- Port number.
*
* RETURN:   MV_STATUS
*               MV_OK - Success, Others - Failure.
*
* NOTE : used for port link down.
*******************************************************************************/
MV_STATUS mvNetaPortDown(int port)
{
	int	          txp;
	MV_NETA_PORT_CTRL *pPortCtrl = mvNetaPortHndlGet(port);
	MV_U32 		  regData, txFifoEmptyMask = 0, txInProgMask = 0;
	int 		  mDelay;

	/* Stop Rx port activity. Check port Rx activity. */
	regData = (MV_REG_READ(ETH_RX_QUEUE_COMMAND_REG(port))) & ETH_RXQ_ENABLE_MASK;
	if (regData != 0) {
		/* Issue stop command for active channels only */
		MV_REG_WRITE(ETH_RX_QUEUE_COMMAND_REG(port), (regData << ETH_RXQ_DISABLE_OFFSET));
	}

	if (!MV_PON_PORT(port)) {
		/* Wait for all Rx activity to terminate. */
		mDelay = 0;
		do {
			if (mDelay >= RX_DISABLE_TIMEOUT_MSEC) {
				mvOsPrintf("ethPort_%d: TIMEOUT for RX stopped !!! rxQueueCmd - 0x08%x\n", port, regData);
				break;
			}
			mvOsDelay(1);
			mDelay++;

			/* Check port RX Command register that all Rx queues are stopped */
			regData = MV_REG_READ(ETH_RX_QUEUE_COMMAND_REG(port));
		} while (regData & 0xFF);
	}

	if (!MV_PON_PORT(port)) {
		/* Stop Tx port activity. Check port Tx activity. */
		for (txp = 0; txp < pPortCtrl->txpNum; txp++) {
			/* Issue stop command for active channels only */
			regData = (MV_REG_READ(ETH_TX_QUEUE_COMMAND_REG(port, txp))) & ETH_TXQ_ENABLE_MASK;
			if (regData != 0)
				MV_REG_WRITE(ETH_TX_QUEUE_COMMAND_REG(port, txp), (regData << ETH_TXQ_DISABLE_OFFSET));

			/* Wait for all Tx activity to terminate. */
			mDelay = 0;
			do {
				if (mDelay >= TX_DISABLE_TIMEOUT_MSEC) {
					mvOsPrintf("port=%d, txp=%d: TIMEOUT for TX stopped !!! txQueueCmd - 0x%08x\n",
						   port, txp, regData);
					break;
				}
				mvOsDelay(1);
				mDelay++;

				/* Check port TX Command register that all Tx queues are stopped */
				regData = MV_REG_READ(ETH_TX_QUEUE_COMMAND_REG(port, txp));
			} while (regData & 0xFF);
#ifdef MV_ETH_GMAC_NEW
			txFifoEmptyMask |= ETH_TX_FIFO_EMPTY_MASK(txp);
			txInProgMask    |= ETH_TX_IN_PROGRESS_MASK(txp);
#else
			if (MV_PON_PORT(port)) {
				txFifoEmptyMask |= PON_TX_FIFO_EMPTY_MASK(txp);
				txInProgMask |= PON_TX_IN_PROGRESS_MASK(txp);
			} else {
				txFifoEmptyMask = ETH_TX_FIFO_EMPTY_MASK;
				txInProgMask = ETH_TX_IN_PROGRESS_MASK;
			}
#endif /* MV_ETH_GMAC_NEW */
		}

		/* Double check to Verify that TX FIFO is Empty */
		mDelay = 0;
		while (MV_TRUE) {
			do {
				if (mDelay >= TX_FIFO_EMPTY_TIMEOUT_MSEC) {
					mvOsPrintf("\n port=%d, TX FIFO empty timeout. status=0x08%x, empty=0x%x, inProg=0x%x\n",
						port, regData, txFifoEmptyMask, txInProgMask);
					break;
				}
				mvOsDelay(1);
				mDelay++;

				regData = MV_REG_READ(ETH_PORT_STATUS_REG(port));
			} while (((regData & txFifoEmptyMask) != txFifoEmptyMask) || ((regData & txInProgMask) != 0));

			if (mDelay >= TX_FIFO_EMPTY_TIMEOUT_MSEC)
				break;

			/* Double check */
			regData = MV_REG_READ(ETH_PORT_STATUS_REG(port));
			if (((regData & txFifoEmptyMask) == txFifoEmptyMask) && ((regData & txInProgMask) == 0)) {
				break;
			} else
				mvOsPrintf("port=%d: TX FIFO Empty double check failed. %d msec, status=0x%x, empty=0x%x, inProg=0x%x\n",
					 port, mDelay, regData, txFifoEmptyMask, txInProgMask);
		}
	}
	/* Wait about 200 usec */
	mvOsUDelay(200);

	return MV_OK;
}


MV_STATUS mvNetaRxqOffsetSet(int port, int rxq, int offset)
{
	MV_U32 regVal;

	regVal = MV_REG_READ(NETA_RXQ_CONFIG_REG(port, rxq));
	regVal &= ~NETA_RXQ_PACKET_OFFSET_ALL_MASK;

	/* Offset is in */
	regVal |= NETA_RXQ_PACKET_OFFSET_MASK(offset >> 3);

	MV_REG_WRITE(NETA_RXQ_CONFIG_REG(port, rxq), regVal);

	return MV_OK;
}

MV_STATUS mvNetaBmPoolBufSizeSet(int port, int pool, int bufsize)
{
	MV_U32 regVal;

	regVal = MV_ALIGN_UP(bufsize, NETA_POOL_BUF_SIZE_ALIGN);
	MV_REG_WRITE(NETA_POOL_BUF_SIZE_REG(port, pool), regVal);

	return MV_OK;
}

MV_STATUS mvNetaRxqBmEnable(int port, int rxq, int shortPool, int longPool)
{
	MV_U32 regVal;

	regVal = MV_REG_READ(NETA_RXQ_CONFIG_REG(port, rxq));

	regVal &= ~(NETA_RXQ_SHORT_POOL_ID_MASK | NETA_RXQ_LONG_POOL_ID_MASK);
	regVal |= (shortPool << NETA_RXQ_SHORT_POOL_ID_OFFS);
	regVal |= (longPool << NETA_RXQ_LONG_POOL_ID_OFFS);
	regVal |= NETA_RXQ_HW_BUF_ALLOC_MASK;

	MV_REG_WRITE(NETA_RXQ_CONFIG_REG(port, rxq), regVal);

	return MV_OK;
}

MV_STATUS mvNetaRxqBmDisable(int port, int rxq)
{
	MV_U32 regVal;

	regVal = MV_REG_READ(NETA_RXQ_CONFIG_REG(port, rxq));

	regVal &= ~NETA_RXQ_HW_BUF_ALLOC_MASK;

	MV_REG_WRITE(NETA_RXQ_CONFIG_REG(port, rxq), regVal);

	return MV_OK;
}

/******************************************************************************/
/*                        WRR / EJP configuration routines                    */
/******************************************************************************/

/* Set maximum burst rate (using IPG configuration) */
MV_STATUS mvNetaTxpEjpBurstRateSet(int port, int txp, int txq, int rate)
{

	if (mvNetaTxpCheck(port, txp))
		return MV_BAD_PARAM;

	/* Only TXQs 2 and 3 are valid */
	if ((txq != 2) && (txq != 3)) {
		mvOsPrintf("%s: txq=%d is INVALID. Only TXQs 2 and 3 are supported\n", __func__, txq);
		return MV_BAD_PARAM;
	}

	mvOsPrintf("Not supported\n");

	return MV_OK;
}

/* Set maximum packet size for each one of EJP priorities (IsoLo, Async) */
MV_STATUS mvNetaTxpEjpMaxPktSizeSet(int port, int txp, int type, int size)
{
	if (mvNetaTxpCheck(port, txp))
		return MV_BAD_PARAM;

	mvOsPrintf("Not supported\n");

	/* TBD */
	return MV_OK;
}

/* TBD - Set Transmit speed for EJP calculations */
MV_STATUS mvNetaTxpEjpTxSpeedSet(int port, int txp, int type, int speed)
{

	if (mvNetaTxpCheck(port, txp))
		return MV_BAD_PARAM;

	/* TBD */
	mvOsPrintf("Not supported\n");

	return MV_OK;
}

/* Calculate period and tokens accordingly with required rate and accuracy */
MV_STATUS mvNetaRateCalc(int rate, unsigned int accuracy, unsigned int *pPeriod, unsigned int *pTokens)
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

/* Enable / Disable EJP mode */
MV_STATUS mvNetaTxpEjpSet(int port, int txp, int enable)
{
	MV_U32  regVal;

	if (mvNetaTxpCheck(port, txp))
		return MV_BAD_PARAM;

	if (enable)
		regVal = NETA_TX_EJP_ENABLE_MASK;
	else
		regVal = 0;

	MV_REG_WRITE(NETA_TX_CMD_1_REG(port, txp), regVal);

	return MV_OK;
}



/* Set TXQ to work in FIX priority mode */
MV_STATUS mvNetaTxqFixPrioSet(int port, int txp, int txq)
{
	MV_U32 regVal;

	if (mvNetaTxpCheck(port, txp))
		return MV_BAD_PARAM;

	if (mvNetaMaxCheck(txq, MV_ETH_MAX_TXQ, "txq"))
		return MV_BAD_PARAM;

	regVal = MV_REG_READ(NETA_TX_FIXED_PRIO_CFG_REG(port, txp));
	regVal |= (1 << txq);
	MV_REG_WRITE(NETA_TX_FIXED_PRIO_CFG_REG(port, txp), regVal);

	return MV_OK;
}

/* Set TXQ to work in WRR mode and set relative weight. */
/*   Weight range [1..N] */
MV_STATUS mvNetaTxqWrrPrioSet(int port, int txp, int txq, int weight)
{
	MV_U32 regVal, mtu;

	if (mvNetaTxpCheck(port, txp))
		return MV_BAD_PARAM;

	if (mvNetaMaxCheck(txq, MV_ETH_MAX_TXQ, "txq"))
		return MV_BAD_PARAM;

	/* Weight * 256 bytes * 8 bits must be larger then MTU [bits] */
	mtu = MV_REG_READ(NETA_TXP_MTU_REG(port, txp));
	/* MTU [bits] -> MTU [256 bytes] */
	mtu = ((mtu / 8) / 256) + 1;
/*
	mvOsPrintf("%s: port=%d, txp=%d, txq=%d, weight=%d, mtu=%d\n",
			__func__, port, txp, txq, weight, mtu);
*/
	if ((weight < mtu) || (weight > NETA_TXQ_WRR_WEIGHT_MAX)) {
		mvOsPrintf("%s Error: weight=%d is out of range %d...%d\n",
				__func__, weight, mtu, NETA_TXQ_WRR_WEIGHT_MAX);
		return MV_FAIL;
	}

	regVal = MV_REG_READ(NETA_TXQ_WRR_ARBITER_REG(port, txp, txq));

	regVal &= ~NETA_TXQ_WRR_WEIGHT_ALL_MASK;
	regVal |= NETA_TXQ_WRR_WEIGHT_MASK(weight);
	MV_REG_WRITE(NETA_TXQ_WRR_ARBITER_REG(port, txp, txq), regVal);

	regVal = MV_REG_READ(NETA_TX_FIXED_PRIO_CFG_REG(port, txp));
	regVal &= ~(1 << txq);
	MV_REG_WRITE(NETA_TX_FIXED_PRIO_CFG_REG(port, txp), regVal);

	return MV_OK;
}

/* Set minimum number of tockens to start transmit for TX port
 *   maxTxSize [bytes]    - maximum packet size can be sent via this TX port
 */
MV_STATUS   mvNetaTxpMaxTxSizeSet(int port, int txp, int maxTxSize)
{
	MV_U32	regVal, size, mtu;
	int		txq;

	if (mvNetaTxpCheck(port, txp))
		return MV_BAD_PARAM;

	mtu = maxTxSize * 8;
	if (mtu > NETA_TXP_MTU_MAX)
		mtu = NETA_TXP_MTU_MAX;

	/* set MTU */
	regVal = MV_REG_READ(NETA_TXP_MTU_REG(port, txp));
	regVal &= ~NETA_TXP_MTU_ALL_MASK;
	regVal |= NETA_TXP_MTU_MASK(mtu);

	MV_REG_WRITE(NETA_TXP_MTU_REG(port, txp), regVal);

	/* TXP token size and all TXQs token size must be larger that MTU */
	regVal = MV_REG_READ(NETA_TXP_TOKEN_SIZE_REG(port, txp));
	size = regVal & NETA_TXP_TOKEN_SIZE_MAX;
	if (size < mtu) {
		size = mtu;
		regVal &= ~NETA_TXP_TOKEN_SIZE_MAX;
		regVal |= size;
		MV_REG_WRITE(NETA_TXP_TOKEN_SIZE_REG(port, txp), regVal);
	}
	for (txq = 0; txq < CONFIG_MV_ETH_TXQ; txq++) {
		regVal = MV_REG_READ(NETA_TXQ_TOKEN_SIZE_REG(port, txp, txq));
		size = regVal & NETA_TXQ_TOKEN_SIZE_MAX;
		if (size < mtu) {
			size = mtu;
			regVal &= ~NETA_TXQ_TOKEN_SIZE_MAX;
			regVal |= size;
			MV_REG_WRITE(NETA_TXQ_TOKEN_SIZE_REG(port, txp, txq), regVal);
		}
	}
	return MV_OK;
}

/* Set bandwidth limitation for TX port
 *   rate [Kbps]    - steady state TX bandwidth limitation
 */
MV_STATUS   mvNetaTxpRateSet(int port, int txp, int rate)
{
	MV_U32		regVal;
	unsigned int	tokens, period, accuracy = 0;
	MV_STATUS	status;

	if (mvNetaTxpCheck(port, txp))
		return MV_BAD_PARAM;

	regVal = MV_REG_READ(NETA_TX_REFILL_PERIOD_REG(port, txp));

	status = mvNetaRateCalc(rate, accuracy, &period, &tokens);
	if (status != MV_OK) {
		mvOsPrintf("%s: Can't provide rate of %d [Kbps] with accuracy of %d [%%]\n",
				__func__, rate, accuracy);
		return status;
	}
	if (tokens > NETA_TXP_REFILL_TOKENS_MAX)
		tokens = NETA_TXP_REFILL_TOKENS_MAX;

	if (period > NETA_TXP_REFILL_PERIOD_MAX)
		period = NETA_TXP_REFILL_PERIOD_MAX;

	regVal = MV_REG_READ(NETA_TXP_REFILL_REG(port, txp));

	regVal &= ~NETA_TXP_REFILL_TOKENS_ALL_MASK;
	regVal |= NETA_TXP_REFILL_TOKENS_MASK(tokens);

	regVal &= ~NETA_TXP_REFILL_PERIOD_ALL_MASK;
	regVal |= NETA_TXP_REFILL_PERIOD_MASK(period);

	MV_REG_WRITE(NETA_TXP_REFILL_REG(port, txp), regVal);

	return MV_OK;
}

/* Set maximum burst size for TX port
 *   burst [bytes] - number of bytes to be sent with maximum possible TX rate,
 *                    before TX rate limitation will take place.
 */
MV_STATUS mvNetaTxpBurstSet(int port, int txp, int burst)
{
	MV_U32  size, mtu;

	if (mvNetaTxpCheck(port, txp))
		return MV_BAD_PARAM;

	/* Calulate Token Bucket Size */
	size = 8 * burst;

	if (size > NETA_TXP_TOKEN_SIZE_MAX)
		size = NETA_TXP_TOKEN_SIZE_MAX;

	/* Token bucket size must be larger then MTU */
	mtu = MV_REG_READ(NETA_TXP_MTU_REG(port, txp));
	if (mtu > size) {
		mvOsPrintf("%s Error: Bucket size (%d bytes) < MTU (%d bytes)\n",
					__func__, (size / 8), (mtu / 8));
		return MV_BAD_PARAM;
	}
	MV_REG_WRITE(NETA_TXP_TOKEN_SIZE_REG(port, txp), size);

	return MV_OK;
}

/* Set bandwidth limitation for TXQ
 *   rate  [Kbps]  - steady state TX rate limitation
 */
MV_STATUS   mvNetaTxqRateSet(int port, int txp, int txq, int rate)
{
	MV_U32		regVal;
	unsigned int	period, tokens, accuracy = 0;
	MV_STATUS	status;

	if (mvNetaTxpCheck(port, txp))
		return MV_BAD_PARAM;

	if (mvNetaMaxCheck(txq, MV_ETH_MAX_TXQ, "txq"))
		return MV_BAD_PARAM;

	status = mvNetaRateCalc(rate, accuracy, &period, &tokens);
	if (status != MV_OK) {
		mvOsPrintf("%s: Can't provide rate of %d [Kbps] with accuracy of %d [%%]\n",
				__func__, rate, accuracy);
		return status;
	}

	if (tokens > NETA_TXQ_REFILL_TOKENS_MAX)
		tokens = NETA_TXQ_REFILL_TOKENS_MAX;

	if (period > NETA_TXQ_REFILL_PERIOD_MAX)
		period = NETA_TXQ_REFILL_PERIOD_MAX;

	regVal = MV_REG_READ(NETA_TXQ_REFILL_REG(port, txp, txq));

	regVal &= ~NETA_TXQ_REFILL_TOKENS_ALL_MASK;
	regVal |= NETA_TXQ_REFILL_TOKENS_MASK(tokens);

	regVal &= ~NETA_TXQ_REFILL_PERIOD_ALL_MASK;
	regVal |= NETA_TXQ_REFILL_PERIOD_MASK(period);

	MV_REG_WRITE(NETA_TXQ_REFILL_REG(port, txp, txq), regVal);

	return MV_OK;
}

/* Set maximum burst size for TX port
 *   burst [bytes] - number of bytes to be sent with maximum possible TX rate,
 *                    before TX bandwidth limitation will take place.
 */
MV_STATUS mvNetaTxqBurstSet(int port, int txp, int txq, int burst)
{
	MV_U32  size, mtu;

	if (mvNetaTxpCheck(port, txp))
		return MV_BAD_PARAM;

	if (mvNetaMaxCheck(txq, MV_ETH_MAX_TXQ, "txq"))
		return MV_BAD_PARAM;

	/* Calulate Tocket Bucket Size */
	size = 8 * burst;

	if (size > NETA_TXQ_TOKEN_SIZE_MAX)
		size = NETA_TXQ_TOKEN_SIZE_MAX;

	/* Tocken bucket size must be larger then MTU */
	mtu = MV_REG_READ(NETA_TXP_MTU_REG(port, txp));
	if (mtu > size) {
		mvOsPrintf("%s Error: Bucket size (%d bytes) < MTU (%d bytes)\n",
					__func__, (size / 8), (mtu / 8));
		return MV_BAD_PARAM;
	}

	MV_REG_WRITE(NETA_TXQ_TOKEN_SIZE_REG(port, txp, txq), size);

	return MV_OK;
}

/************************ Legacy parse function start *******************************/
/******************************************************************************/
/*                        RX Dispatching configuration routines               */
/******************************************************************************/

MV_STATUS mvNetaTcpRxq(int port, int rxq)
{
	MV_U32 portCfgReg;

	if ((rxq < 0) || (rxq >= MV_ETH_MAX_RXQ)) {
		mvOsPrintf("ethDrv: RX queue #%d is out of range\n", rxq);
		return MV_BAD_PARAM;
	}
	portCfgReg = MV_REG_READ(ETH_PORT_CONFIG_REG(port));

	portCfgReg &= ~ETH_DEF_RX_TCP_QUEUE_ALL_MASK;
	portCfgReg |= ETH_DEF_RX_TCP_QUEUE_MASK(rxq);
	portCfgReg |= ETH_CAPTURE_TCP_FRAMES_ENABLE_MASK;

	MV_REG_WRITE(ETH_PORT_CONFIG_REG(port), portCfgReg);

	return MV_OK;
}

MV_STATUS mvNetaUdpRxq(int port, int rxq)
{
	MV_U32 portCfgReg;

	if ((rxq < 0) || (rxq >= MV_ETH_MAX_RXQ)) {
		mvOsPrintf("ethDrv: RX queue #%d is out of range\n", rxq);
		return MV_BAD_PARAM;
	}

	portCfgReg = MV_REG_READ(ETH_PORT_CONFIG_REG(port));

	portCfgReg &= ~ETH_DEF_RX_UDP_QUEUE_ALL_MASK;
	portCfgReg |= ETH_DEF_RX_UDP_QUEUE_MASK(rxq);
	portCfgReg |= ETH_CAPTURE_UDP_FRAMES_ENABLE_MASK;

	MV_REG_WRITE(ETH_PORT_CONFIG_REG(port), portCfgReg);

	return MV_OK;
}

MV_STATUS mvNetaArpRxq(int port, int rxq)
{
	MV_U32 portCfgReg;

	if ((rxq < 0) || (rxq >= MV_ETH_MAX_RXQ)) {
		mvOsPrintf("ethDrv: RX queue #%d is out of range\n", rxq);
		return MV_BAD_PARAM;
	}

	portCfgReg = MV_REG_READ(ETH_PORT_CONFIG_REG(port));

	portCfgReg &= ~ETH_DEF_RX_ARP_QUEUE_ALL_MASK;
	portCfgReg |= ETH_DEF_RX_ARP_QUEUE_MASK(rxq);
	portCfgReg &= (~ETH_REJECT_ARP_BCAST_MASK);

	MV_REG_WRITE(ETH_PORT_CONFIG_REG(port), portCfgReg);

	return MV_OK;
}

MV_STATUS mvNetaBpduRxq(int port, int rxq)
{
	MV_U32 portCfgReg;
	MV_U32 portCfgExtReg;

	if ((rxq < 0) || (rxq >= MV_ETH_MAX_RXQ)) {
		mvOsPrintf("ethDrv: RX queue #%d is out of range\n", rxq);
		return MV_BAD_PARAM;
	}

	portCfgExtReg = MV_REG_READ(ETH_PORT_CONFIG_EXTEND_REG(port));
	portCfgReg = MV_REG_READ(ETH_PORT_CONFIG_REG(port));

	portCfgReg &= ~ETH_DEF_RX_BPDU_QUEUE_ALL_MASK;
	portCfgReg |= ETH_DEF_RX_BPDU_QUEUE_MASK(rxq);

	MV_REG_WRITE(ETH_PORT_CONFIG_REG(port), portCfgReg);

	portCfgExtReg |= ETH_CAPTURE_SPAN_BPDU_ENABLE_MASK;

	MV_REG_WRITE(ETH_PORT_CONFIG_EXTEND_REG(port), portCfgExtReg);

	return MV_OK;
}
/************************ Legacy parse function end *******************************/

/******************************************************************************/
/*                      MIB Counters functions                                */
/******************************************************************************/

/*******************************************************************************
* mvNetaMibCounterRead - Read a MIB counter
*
* DESCRIPTION:
*       This function reads a MIB counter of a specific ethernet port.
*       NOTE - Read from ETH_MIB_GOOD_OCTETS_RECEIVED_LOW or
*              ETH_MIB_GOOD_OCTETS_SENT_LOW counters will return 64 bits value,
*              so pHigh32 pointer should not be NULL in this case.
*
* INPUT:
*       port        - Ethernet Port number.
*       mib         - MIB number
*       mibOffset   - MIB counter offset.
*
* OUTPUT:
*       MV_U32*       pHigh32 - pointer to place where 32 most significant bits
*                             of the counter will be stored.
*
* RETURN:
*       32 low sgnificant bits of MIB counter value.
*
*******************************************************************************/
MV_U32 mvNetaMibCounterRead(int port, int mib, unsigned int mibOffset, MV_U32 *pHigh32)
{
	MV_U32 valLow32, valHigh32;

	valLow32 = MV_REG_READ(ETH_MIB_COUNTERS_BASE(port, mib) + mibOffset);

	/* Implement FEr ETH. Erroneous Value when Reading the Upper 32-bits    */
	/* of a 64-bit MIB Counter.                                             */
	if ((mibOffset == ETH_MIB_GOOD_OCTETS_RECEIVED_LOW) || (mibOffset == ETH_MIB_GOOD_OCTETS_SENT_LOW)) {
		valHigh32 = MV_REG_READ(ETH_MIB_COUNTERS_BASE(port, mib) + mibOffset + 4);
		if (pHigh32 != NULL)
			*pHigh32 = valHigh32;
	}
	return valLow32;
}

/*******************************************************************************
* mvNetaMibCountersClear - Clear all MIB counters
*
* DESCRIPTION:
*       This function clears all MIB counters
*
* INPUT:
*       port      - Ethernet Port number.
*       mib       - MIB number
*
*
* RETURN:   void
*
*******************************************************************************/
void mvNetaMibCountersClear(int port, int mib)
{
	int i;

#if defined(CONFIG_MV_PON) && !defined(MV_PON_MIB_SUPPORT)
	if (MV_PON_PORT(port))
		return;
#endif /* CONFIG_MV_PON && !MV_PON_MIB_SUPPORT */

	/* Perform dummy reads from MIB counters */
	for (i = ETH_MIB_GOOD_OCTETS_RECEIVED_LOW; i < ETH_MIB_LATE_COLLISION; i += 4)
		 MV_REG_READ((ETH_MIB_COUNTERS_BASE(port, mib) + i));
}

#if defined(CONFIG_MV_PON) && defined(MV_PON_MIB_SUPPORT)

/* Set default MIB counters set for RX packets. mib==-1 means don't count */
MV_STATUS   mvNetaPonRxMibDefault(int mib)
{
	MV_U32  regVal = 0;

	regVal = MV_REG_READ(NETA_PON_MIB_RX_DEF_REG);
	if (mib == -1) {
		/* Don't count default packets that not match Gem portID */
		regVal &= ~NETA_PON_MIB_RX_VALID_MASK;
	} else {
		if (mvNetaMaxCheck(mib, MV_ETH_MAX_TCONT, "tcont"))
			return MV_BAD_PARAM;

		regVal &= ~NETA_PON_MIB_RX_MIB_NO_MASK;
		regVal |= NETA_PON_MIB_RX_VALID_MASK | NETA_PON_MIB_RX_MIB_NO(mib);
	}
	MV_REG_WRITE(NETA_PON_MIB_RX_DEF_REG, regVal);

	return MV_OK;
}

/* Set MIB counters set used for RX packets with special gemPid. mib==-1 means delete entry */
MV_STATUS   mvNetaPonRxMibGemPid(int mib, MV_U16 gemPid)
{
	MV_U32	regVal;
	int	i, free = -1;

	if ((mib != -1) && mvNetaMaxCheck(mib, MV_ETH_MAX_TCONT, "tcont"))
		return MV_BAD_PARAM;

	/* look for gemPid if exist of first free entry */
	for (i = 0; i < NETA_PON_MIB_MAX_GEM_PID; i++) {
		regVal = MV_REG_READ(NETA_PON_MIB_RX_CTRL_REG(i));
		if ((regVal & NETA_PON_MIB_RX_VALID_MASK) &&
		    ((regVal & NETA_PON_MIB_RX_GEM_PID_ALL_MASK) ==
			NETA_PON_MIB_RX_GEM_PID_MASK(gemPid))) {
			/* Entry for this gemPid exist */
			if (mib == -1) {
				/* Delete entry */
				regVal &= ~NETA_PON_MIB_RX_VALID_MASK;
			} else {
				/* update mibNo */
				regVal &= ~NETA_PON_MIB_RX_MIB_NO_MASK;
				regVal |= NETA_PON_MIB_RX_MIB_NO(mib);
			}
			MV_REG_WRITE(NETA_PON_MIB_RX_CTRL_REG(i), regVal);

			return MV_OK;
		}
		if ((free == -1) && ((regVal & NETA_PON_MIB_RX_VALID_MASK) == 0)) {
			/* remember first invalid entry */
			free = i;
		}
	}
	if (mib == -1)	{
		mvOsPrintf("%s: Can't delete entry for gemPid=0x%x - NOT found\n",
					__func__, gemPid);
		return MV_NOT_FOUND;
	}
	if (free == -1)	{
		mvOsPrintf("%s: No free entry for gemPid=0x%x, rxMib=%d\n",
					__func__, gemPid, mib);
		return MV_FULL;
	}
	regVal = NETA_PON_MIB_RX_VALID_MASK | NETA_PON_MIB_RX_MIB_NO(mib) | NETA_PON_MIB_RX_GEM_PID_MASK(gemPid);
	MV_REG_WRITE(NETA_PON_MIB_RX_CTRL_REG(free), regVal);

    return MV_OK;
}
#endif /* CONFIG_MV_PON && MV_PON_MIB_SUPPORT */
