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

#include "voiceband/commUnit/mvCommUnit.h"

#undef	MV_COMM_UNIT_DEBUG
#define	MV_COMM_UNIT_RPT_SUPPORT /* Repeat mode must be set */
#undef	MV_COMM_UNIT_TEST_SUPPORT

/* defines */
#define TOTAL_CHAINS		2
#define CONFIG_RBSZ		16
#define NEXT_BUFF(buff)		((buff + 1) % TOTAL_CHAINS)
#define PREV_BUFF(buff)		(buff == 0 ? (TOTAL_CHAINS-1) : (buff-1))
#define MAX_POLL_USEC		100000	/* 100ms */
#define COMM_UNIT_SW_RST	(1 << 5)
#define OLD_INT_WA_BIT		(1 << 15)

/* globals */
static MV_STATUS tdmEnable;
static MV_STATUS pcmEnable;
static MV_U8 spiMode;
static MV_U8 maxCs;
static MV_U8 sampleSize;
static MV_U8 samplingCoeff;
static MV_U16 totalChannels;
static MV_U8 prevRxBuff, nextTxBuff;
static MV_U8 *rxBuffVirt[TOTAL_CHAINS], *txBuffVirt[TOTAL_CHAINS];
static MV_ULONG rxBuffPhys[TOTAL_CHAINS], txBuffPhys[TOTAL_CHAINS];
static MV_TDM_MCDMA_RX_DESC *mcdmaRxDescPtr[TOTAL_CHAINS];
static MV_TDM_MCDMA_TX_DESC *mcdmaTxDescPtr[TOTAL_CHAINS];
static MV_ULONG mcdmaRxDescPhys[TOTAL_CHAINS], mcdmaTxDescPhys[TOTAL_CHAINS];
static MV_TDM_DPRAM_ENTRY defDpramEntry = { 0, 0, 0x1, 0x1, 0, 0, 0x1, 0, 0, 0, 0 };
static MV_U32 ctrlFamilyId;
static MV_U16 ctrlModel;
static MV_U16 ctrlRev;

/* Static APIs */
static MV_VOID mvCommUnitDescChainBuild(MV_VOID);
static MV_VOID mvCommUnitMcdmaMcscStart(MV_VOID);
static MV_VOID mvCommUnitMcdmaStop(MV_VOID);
static MV_VOID mvCommUnitMcdmaMcscAbort(MV_VOID);

static MV_COMMUNIT_IP_VERSION_T mvCommUnitIpVerGet(MV_U32 ctrlFamilyId)
{
	switch (ctrlFamilyId) {
	case MV_65XX_DEV_ID:
		return MV_COMMUNIT_IP_VER_ORIGIN;
	case MV_78XX0:
	case MV_88F66X0:
	case MV_88F67X0:
		return MV_COMMUNIT_IP_VER_REVISE_1;
	default:
		return MV_COMMUNIT_IP_VER_ORIGIN;
	}
}

MV_STATUS mvCommUnitHalInit(MV_TDM_PARAMS *tdmParams, MV_TDM_HAL_DATA *halData)
{
	MV_U16 pcmSlot, index;
	MV_U32 buffSize, chan;
	MV_U32 totalRxDescSize, totalTxDescSize;
	MV_U32 maxPoll, clkSyncCtrlReg;
#if 0
	MV_U32 chMask;
#endif
	MV_U32 count;
	MV_TDM_DPRAM_ENTRY actDpramEntry, *pActDpramEntry;

	MV_TRC_REC("->%s\n", __func__);

	/* Initialize driver resources */
	tdmEnable = MV_FALSE;
	pcmEnable = MV_FALSE;
	spiMode = halData->spiMode;
	maxCs = halData->maxCs;
	totalChannels = tdmParams->totalChannels;
	prevRxBuff = 0;
	nextTxBuff = 0;
	ctrlFamilyId = halData->familyId;
	ctrlModel = halData->model;
	ctrlRev = halData->ctrlRev;

	/* Check parameters */
	if ((tdmParams->totalChannels > MV_TDMMC_TOTAL_CHANNELS) ||
	    (tdmParams->samplingPeriod > MV_TDM_MAX_SAMPLING_PERIOD)) {
		mvOsPrintf("%s: Error, bad parameters\n", __func__);
		return MV_ERROR;
	}

	/* Extract sampling period coefficient */
	samplingCoeff = (tdmParams->samplingPeriod / MV_TDM_BASE_SAMPLING_PERIOD);

	sampleSize = tdmParams->pcmFormat;

	/* Calculate single Rx/Tx buffer size */
	buffSize = (sampleSize * MV_TDM_TOTAL_CH_SAMPLES * samplingCoeff);

	/* Allocate cached data buffers for all channels */
	TRC_REC("%s: allocate %dB for data buffers totalChannels=%d\n",
		__func__, (buffSize * totalChannels), totalChannels);
	for (index = 0; index < TOTAL_CHAINS; index++) {
		rxBuffVirt[index] =
		    (MV_U8 *) mvOsIoCachedMalloc(NULL, ((buffSize * totalChannels) + CPU_D_CACHE_LINE_SIZE),
						 &rxBuffPhys[index], NULL);
		txBuffVirt[index] =
		    (MV_U8 *) mvOsIoCachedMalloc(NULL, ((buffSize * totalChannels) + CPU_D_CACHE_LINE_SIZE),
						 &txBuffPhys[index], NULL);

		/* Check Rx buffer address & size alignment */
		if (((MV_U32) rxBuffVirt[index] | buffSize) & (CONFIG_RBSZ - 1)) {
			mvOsPrintf("%s: Error, unaligned Rx buffer address or size\n", __func__);
			return MV_ERROR;
		}

		/* Clear buffers */
		memset(rxBuffVirt[index], 0, (buffSize * totalChannels));
		memset(txBuffVirt[index], 0, (buffSize * totalChannels));
#ifdef MV_COMM_UNIT_TEST_SUPPORT
	/* Fill Tx buffers with incremental pattern */
		{
			int i, j;
			for (j = 0; j < totalChannels; j++) {
				for (i = 0; i < buffSize; i++)
					*(MV_U8 *) (txBuffVirt[index]+i+(j*buffSize)) = (MV_U8)(i+1);
			}
		}
#endif

		/* Flush+Inv buffers */
		mvOsCacheFlushInv(NULL, rxBuffVirt[index], (buffSize * totalChannels));
		mvOsCacheFlushInv(NULL, txBuffVirt[index], (buffSize * totalChannels));
	}

	/* Allocate non-cached MCDMA Rx/Tx descriptors */
	totalRxDescSize = totalChannels * sizeof(MV_TDM_MCDMA_RX_DESC);
	totalTxDescSize = totalChannels * sizeof(MV_TDM_MCDMA_TX_DESC);

	TRC_REC("%s: allocate %dB for Rx/Tx descriptors\n", __func__, totalRxDescSize);
	for (index = 0; index < TOTAL_CHAINS; index++) {
		mcdmaRxDescPtr[index] = (MV_TDM_MCDMA_RX_DESC *) mvOsIoUncachedMalloc(NULL, totalRxDescSize,
										      &mcdmaRxDescPhys[index], NULL);
		mcdmaTxDescPtr[index] = (MV_TDM_MCDMA_TX_DESC *) mvOsIoUncachedMalloc(NULL, totalTxDescSize,
										      &mcdmaTxDescPhys[index], NULL);

		/* Check descriptors alignment */
		if (((MV_U32) mcdmaRxDescPtr[index] | (MV_U32) mcdmaTxDescPtr[index]) &
		    (sizeof(MV_TDM_MCDMA_RX_DESC) - 1)) {
			mvOsPrintf("%s: Error, unaligned MCDMA Rx/Tx descriptors\n", __func__);
			return MV_ERROR;
		}

		/* Clear descriptors */
		memset(mcdmaRxDescPtr[index], 0, totalRxDescSize);
		memset(mcdmaTxDescPtr[index], 0, totalTxDescSize);
	}

	/* Poll MCDMA for reset completion */
	maxPoll = 0;
	while ((maxPoll < MAX_POLL_USEC) && !(MV_REG_READ(MCDMA_GLOBAL_CONTROL_REG) & MCDMA_RID_MASK)) {
		mvOsUDelay(1);
		maxPoll++;
	}

	if (maxPoll >= MAX_POLL_USEC) {
		mvOsPrintf("Error, MCDMA reset completion timout\n");
		return MV_ERROR;
	}

	/* Poll MCSC for RAM initialization done */
	if (!(MV_REG_READ(MCSC_GLOBAL_INT_CAUSE_REG) & MCSC_GLOBAL_INT_CAUSE_INIT_DONE_MASK)) {
		maxPoll = 0;
		while ((maxPoll < MAX_POLL_USEC) &&
		       !(MV_REG_READ(MCSC_GLOBAL_INT_CAUSE_REG) & MCSC_GLOBAL_INT_CAUSE_INIT_DONE_MASK)) {
			mvOsUDelay(1);
			maxPoll++;
		}

		if (maxPoll >= MAX_POLL_USEC) {
			mvOsPrintf("Error, MCDMA RAM initialization timout\n");
			return MV_ERROR;
		}
	}

	/***************************************************************/
	/* MCDMA Configuration(use default MCDMA linked-list settings) */
	/***************************************************************/
	/* Set Rx Service Queue Arbiter Weight Register */
	MV_REG_WRITE(RX_SERVICE_QUEUE_ARBITER_WEIGHT_REG,
			(MV_REG_READ(RX_SERVICE_QUEUE_ARBITER_WEIGHT_REG) & ~(0x1f << 24))); /*| MCDMA_RSQW_MASK));*/

	/* Set Tx Service Queue Arbiter Weight Register */
	MV_REG_WRITE(TX_SERVICE_QUEUE_ARBITER_WEIGHT_REG,
			(MV_REG_READ(TX_SERVICE_QUEUE_ARBITER_WEIGHT_REG) & ~(0x1f << 24)));	/*| MCDMA_TSQW_MASK));*/

	for (chan = 0; chan < totalChannels; chan++) {
		/* Set RMCCx */
		MV_REG_WRITE(MCDMA_RECEIVE_CONTROL_REG(chan), CONFIG_RMCCx);

		/* Set TMCCx */
		MV_REG_WRITE(MCDMA_TRANSMIT_CONTROL_REG(chan), CONFIG_TMCCx);
	}

	/**********************/
	/* MCSC Configuration */
	/**********************/
	/* Disable Rx/Tx channel balancing & Linear mode fix */
	MV_REG_BIT_SET(MCSC_GLOBAL_CONFIG_REG, MCSC_GLOBAL_CONFIG_TCBD_MASK);
#if 0
	/* Unmask Rx/Tx channel balancing */
	chMask = (0xffffffff & ~((MV_U32)((1 << totalChannels) - 1)));
	MV_REG_WRITE(MCSC_RX_CHANNEL_BALANCING_MASK_REG, chMask);
	MV_REG_WRITE(MCSC_TX_CHANNEL_BALANCING_MASK_REG, chMask);
#endif

	for (chan = 0; chan < totalChannels; chan++) {
		MV_REG_WRITE(MCSC_CHx_RECEIVE_CONFIG_REG(chan), CONFIG_MRCRx);
		MV_REG_WRITE(MCSC_CHx_TRANSMIT_CONFIG_REG(chan), CONFIG_MTCRx);
	}

	/* Enable RX/TX linear byte swap, only in linear mode */
	if (MV_PCM_FORMAT_1BYTE == tdmParams->pcmFormat)
		MV_REG_WRITE(MCSC_GLOBAL_CONFIG_EXTENDED_REG,
			    (MV_REG_READ(MCSC_GLOBAL_CONFIG_EXTENDED_REG) & (~CONFIG_LINEAR_BYTE_SWAP)));
	else
		MV_REG_WRITE(MCSC_GLOBAL_CONFIG_EXTENDED_REG,
			    (MV_REG_READ(MCSC_GLOBAL_CONFIG_EXTENDED_REG) | CONFIG_LINEAR_BYTE_SWAP));

	/***********************************************/
	/* Shared Bus to Crossbar Bridge Configuration */
	/***********************************************/
	/* Set Timeout Counter Register */
	MV_REG_WRITE(TIME_OUT_COUNTER_REG, (MV_REG_READ(TIME_OUT_COUNTER_REG) | TIME_OUT_THRESHOLD_COUNT_MASK));

	/*************************************************/
	/* Time Division Multiplexing(TDM) Configuration */
	/*************************************************/
	pActDpramEntry = &actDpramEntry;
	memcpy(&actDpramEntry, &defDpramEntry, sizeof(MV_TDM_DPRAM_ENTRY));
	/* Set repeat mode bits for (sampleSize > 1) */
	pActDpramEntry->rpt = ((sampleSize == MV_PCM_FORMAT_1BYTE) ? 0 : 1);

	/* Reset all Rx/Tx DPRAM entries to default value */
	for (index = 0; index < (2 * MV_TDM_MAX_HALF_DPRAM_ENTRIES); index++) {
		MV_REG_WRITE(FLEX_TDM_RDPR_REG(index), *((MV_U32 *) pActDpramEntry));
		MV_REG_WRITE(FLEX_TDM_TDPR_REG(index), *((MV_U32 *) pActDpramEntry));
	}

	/* Set active Rx/Tx DPRAM entries */
	for (chan = 0; chan < totalChannels; chan++) {
		/* Same time slot number for both Rx & Tx */
		pcmSlot = tdmParams->pcmSlot[chan];

		/* Verify time slot is within frame boundries */
		if (pcmSlot >= halData->frameTs) {
			mvOsPrintf("Error, time slot(%d) exceeded maximum(%d)\n", pcmSlot, halData->frameTs);
			return MV_ERROR;
		}

		/* Verify time slot is aligned to sample size */
		if ((sampleSize > MV_PCM_FORMAT_1BYTE) && (pcmSlot & 1)) {
			mvOsPrintf("Error, time slot(%d) not aligned to Linear PCM sample size\n", pcmSlot);
			return MV_ERROR;
		}

		/* Update relevant DPRAM fields */
		pActDpramEntry->ch = chan;
		pActDpramEntry->mask = 0xff;

		/* Extract physical DPRAM entry id */
		index = ((sampleSize == MV_PCM_FORMAT_1BYTE) ? pcmSlot : (pcmSlot / 2));

		/* DPRAM low half */
		MV_REG_WRITE(FLEX_TDM_RDPR_REG(index), *((MV_U32 *) pActDpramEntry));
		MV_REG_WRITE(FLEX_TDM_TDPR_REG(index), *((MV_U32 *) pActDpramEntry));

		/* DPRAM high half(mirroring DPRAM low half) */
		pActDpramEntry->mask = 0;
		MV_REG_WRITE(FLEX_TDM_RDPR_REG((MV_TDM_MAX_HALF_DPRAM_ENTRIES + index)), *((MV_U32 *) pActDpramEntry));
		MV_REG_WRITE(FLEX_TDM_TDPR_REG((MV_TDM_MAX_HALF_DPRAM_ENTRIES + index)), *((MV_U32 *) pActDpramEntry));

		/* WideBand mode */
		if (sampleSize == MV_PCM_FORMAT_4BYTES) {
			index = (index + (halData->frameTs / sampleSize));
			/* DPRAM low half */
			pActDpramEntry->mask = 0xff;
			MV_REG_WRITE(FLEX_TDM_RDPR_REG(index), *((MV_U32 *) pActDpramEntry));
			MV_REG_WRITE(FLEX_TDM_TDPR_REG(index), *((MV_U32 *) pActDpramEntry));

			/* DPRAM high half(mirroring DPRAM low half) */
			pActDpramEntry->mask = 0;
			MV_REG_WRITE(FLEX_TDM_RDPR_REG((MV_TDM_MAX_HALF_DPRAM_ENTRIES + index)), *((MV_U32 *) pActDpramEntry));
			MV_REG_WRITE(FLEX_TDM_TDPR_REG((MV_TDM_MAX_HALF_DPRAM_ENTRIES + index)), *((MV_U32 *) pActDpramEntry));
		}
	}

	/* Fill last Tx/Rx DPRAM entry('LAST'=1) */
	pActDpramEntry->mask = 0;
	pActDpramEntry->ch = 0;
	pActDpramEntry->last = 1;

	/* Index for last entry */
	if (sampleSize == MV_PCM_FORMAT_1BYTE)
		index = (halData->frameTs - 1);
	else
		index = ((halData->frameTs / 2) - 1);

	/* Low half */
	MV_REG_WRITE(FLEX_TDM_TDPR_REG(index), *((MV_U32 *) pActDpramEntry));
	MV_REG_WRITE(FLEX_TDM_RDPR_REG(index), *((MV_U32 *) pActDpramEntry));
	/* High half */
	MV_REG_WRITE(FLEX_TDM_TDPR_REG((MV_TDM_MAX_HALF_DPRAM_ENTRIES + index)), *((MV_U32 *) pActDpramEntry));
	MV_REG_WRITE(FLEX_TDM_RDPR_REG((MV_TDM_MAX_HALF_DPRAM_ENTRIES + index)), *((MV_U32 *) pActDpramEntry));

	/* Set TDM_CLK_AND_SYNC_CONTROL register */
	clkSyncCtrlReg = MV_REG_READ(TDM_CLK_AND_SYNC_CONTROL_REG);
	clkSyncCtrlReg &= ~(TDM_TX_FSYNC_OUT_ENABLE_MASK | TDM_RX_FSYNC_OUT_ENABLE_MASK |
			TDM_TX_CLK_OUT_ENABLE_MASK | TDM_RX_CLK_OUT_ENABLE_MASK);
	clkSyncCtrlReg |= CONFIG_TDM_CLK_AND_SYNC_CONTROL;
	MV_REG_WRITE(TDM_CLK_AND_SYNC_CONTROL_REG, clkSyncCtrlReg);

	/* Set TDM TCR register */
	MV_REG_WRITE(FLEX_TDM_CONFIG_REG, (MV_REG_READ(FLEX_TDM_CONFIG_REG) | CONFIG_FLEX_TDM_CONFIG));

#if 0
	/* Set TDM_PLUS_MINUS_DELAY_CTRL_FSYNC_OUT register */
	MV_REG_WRITE(TDM_PLUS_MINUS_DELAY_CTRL_FSYNC_OUT_REG, CONFIG_TDM_PLUS_MINUS_DELAY_CTRL_FSYNC_OUT);

	/* Set TDM_PLUS_MINUS_DELAY_CTRL_FSYNC_IN register */
	MV_REG_WRITE(TDM_PLUS_MINUS_DELAY_CTRL_FSYNC_IN_REG, CONFIG_TDM_PLUS_MINUS_DELAY_CTRL_FSYNC_IN);

	/* Restart calculation */
	MV_REG_BIT_SET(TDM_PLUS_MINUS_DELAY_CTRL_FSYNC_OUT_REG,
				(TX_SYNC_DELAY_OUT_RESTART_CALC_MASK | RX_SYNC_DELAY_OUT_RESTART_CALC_MASK));
	MV_REG_BIT_SET(TDM_PLUS_MINUS_DELAY_CTRL_FSYNC_IN_REG,
				(TX_SYNC_DELAY_IN_RESTART_CALC_MASK | RX_SYNC_DELAY_IN_RESTART_CALC_MASK));
	mvOsDelay(1);
	MV_REG_BIT_RESET(TDM_PLUS_MINUS_DELAY_CTRL_FSYNC_OUT_REG,
				(TX_SYNC_DELAY_OUT_RESTART_CALC_MASK | RX_SYNC_DELAY_OUT_RESTART_CALC_MASK));
	MV_REG_BIT_RESET(TDM_PLUS_MINUS_DELAY_CTRL_FSYNC_IN_REG,
				(TX_SYNC_DELAY_IN_RESTART_CALC_MASK | RX_SYNC_DELAY_IN_RESTART_CALC_MASK));
#endif

	/* Set TDM_CLK_DIVIDER_CONTROL register */
	/*MV_REG_WRITE(TDM_CLK_DIVIDER_CONTROL_REG, TDM_RX_FIXED_DIV_ENABLE_MASK); */

	/* Enable SLIC/s interrupt detection(before Rx/Tx are active) */
	/*MV_REG_WRITE(TDM_MASK_REG, TDM_SLIC_INT); */

	/**********************************************************************/
	/* Time Division Multiplexing(TDM) Interrupt Controller Configuration */
	/**********************************************************************/
	/* Clear TDM cause and mask registers */
	MV_REG_WRITE(COMM_UNIT_TOP_MASK_REG, 0);
	MV_REG_WRITE(TDM_MASK_REG, 0);
	MV_REG_WRITE(COMM_UNIT_TOP_CAUSE_REG, 0);
	MV_REG_WRITE(TDM_CAUSE_REG, 0);

	/* Clear MCSC cause and mask registers(except InitDone bit) */
	MV_REG_WRITE(MCSC_GLOBAL_INT_MASK_REG, 0);
	MV_REG_WRITE(MCSC_EXTENDED_INT_MASK_REG, 0);
	MV_REG_WRITE(MCSC_GLOBAL_INT_CAUSE_REG, MCSC_GLOBAL_INT_CAUSE_INIT_DONE_MASK);
	MV_REG_WRITE(MCSC_EXTENDED_INT_CAUSE_REG, 0);

	/* Set output sync counter bits for FS */
#if defined(MV_TDM_PCM_CLK_8MHZ)
	count = MV_FRAME_128TS * 8;
#elif defined(MV_TDM_PCM_CLK_4MHZ)
	count = MV_FRAME_64TS * 8;
#else /* MV_TDM_PCM_CLK_2MHZ */
	count = MV_FRAME_32TS * 8;
#endif
	MV_REG_WRITE(TDM_OUTPUT_SYNC_BIT_COUNT_REG,
		((count << TDM_SYNC_BIT_RX_OFFS) & TDM_SYNC_BIT_RX_MASK) | (count & TDM_SYNC_BIT_TX_MASK));

#ifdef MV_COMM_UNIT_DEBUG
	mvCommUnitShow();
#endif

	/* Enable PCM */
	mvCommUnitPcmStart();

	/* Mark TDM I/F as enabled */
	tdmEnable = MV_TRUE;

	/* Enable PCLK */
	MV_REG_WRITE(TDM_DATA_DELAY_AND_CLK_CTRL_REG, (MV_REG_READ(TDM_DATA_DELAY_AND_CLK_CTRL_REG) |
				CONFIG_TDM_DATA_DELAY_AND_CLK_CTRL));

	/* Restore old periodic interrupt mechanism WA */
	/* MV_REG_BIT_SET(TDM_DATA_DELAY_AND_CLK_CTRL_REG, OLD_INT_WA_BIT); */

	/* Keep the software workaround to enable TEN while set Fsync for none-ALP chips */
	/* Enable TDM */
	if (MV_COMMUNIT_IP_VER_ORIGIN == mvCommUnitIpVerGet(ctrlFamilyId))
		MV_REG_BIT_SET(FLEX_TDM_CONFIG_REG, TDM_TEN_MASK);

#if 0
	/* Poll for Enter Hunt Execution Status */
	for (chan = 0; chan < totalChannels; chan++) {
		maxPoll = 0;
		while ((maxPoll < MAX_POLL_USEC) &&
			!(MV_REG_READ(MCSC_CHx_COMM_EXEC_STAT_REG(chan)) & MCSC_EH_E_STAT_MASK)) {
			mvOsUDelay(1);
			maxPoll++;
		}

		if (maxPoll >= MAX_POLL_USEC) {
			mvOsPrintf("%s: Error, enter hunt execution timeout(ch%d)\n", __func__, chan);
			return MV_ERROR;
		}

		MV_REG_BIT_RESET(MCSC_CHx_RECEIVE_CONFIG_REG(chan), MRCRx_ENTER_HUNT_MASK);
	}
#endif

	MV_TRC_REC("<-%s\n", __func__);
	return MV_OK;
}

MV_VOID mvCommUnitRelease(MV_VOID)
{
	MV_U32 buffSize, totalRxDescSize, totalTxDescSize, index;

	MV_TRC_REC("->%s\n", __func__);

	if (tdmEnable == MV_TRUE) {

		/* Mark TDM I/F as disabled */
		tdmEnable = MV_FALSE;

		mvCommUnitPcmStop();

		mvCommUnitMcdmaMcscAbort();

		mvOsUDelay(10);
		MV_REG_BIT_RESET(MCSC_GLOBAL_CONFIG_REG, MCSC_GLOBAL_CONFIG_MAI_MASK);

		/* Disable TDM */
		if (MV_COMMUNIT_IP_VER_ORIGIN == mvCommUnitIpVerGet(ctrlFamilyId))
			MV_REG_BIT_RESET(FLEX_TDM_CONFIG_REG, TDM_TEN_MASK);

		/* Disable PCLK */
		MV_REG_BIT_RESET(TDM_DATA_DELAY_AND_CLK_CTRL_REG, (TX_CLK_OUT_ENABLE_MASK | RX_CLK_OUT_ENABLE_MASK));

		/* Reset CommUnit blocks to default settings */
		MV_REG_BIT_RESET(0x18220, COMM_UNIT_SW_RST);
		mvOsUDelay(1);
		MV_REG_BIT_SET(0x18220, COMM_UNIT_SW_RST);
		mvOsDelay(10);

		/* Calculate total Rx/Tx buffer size */
		buffSize = (sampleSize * MV_TDM_TOTAL_CH_SAMPLES * samplingCoeff * totalChannels) + CPU_D_CACHE_LINE_SIZE;

		/* Calculate total MCDMA Rx/Tx descriptors chain size */
		totalRxDescSize = totalChannels * sizeof(MV_TDM_MCDMA_RX_DESC);
		totalTxDescSize = totalChannels * sizeof(MV_TDM_MCDMA_TX_DESC);

		for (index = 0; index < TOTAL_CHAINS; index++) {
			/* Release Rx/Tx data buffers */
			mvOsIoCachedFree(NULL, buffSize, rxBuffPhys[index], rxBuffVirt[index], 0);
			mvOsIoCachedFree(NULL, buffSize, txBuffPhys[index], txBuffVirt[index], 0);

			/* Release MCDMA Rx/Tx descriptors */
			mvOsIoUncachedFree(NULL, totalRxDescSize, mcdmaRxDescPhys[index], mcdmaRxDescPtr[index], 0);
			mvOsIoUncachedFree(NULL, totalTxDescSize, mcdmaTxDescPhys[index], mcdmaTxDescPtr[index], 0);
		}
	}

	MV_TRC_REC("<-%s\n", __func__);
}

static MV_VOID mvCommUnitMcdmaMcscStart(MV_VOID)
{
	MV_U32 chan, rxDescPhysAddr, txDescPhysAddr;

	MV_TRC_REC("->%s\n", __func__);

	mvCommUnitDescChainBuild();

	/* Set current Rx/Tx descriptors  */
	for (chan = 0; chan < totalChannels; chan++) {
		rxDescPhysAddr = mcdmaRxDescPhys[0] + (chan * sizeof(MV_TDM_MCDMA_RX_DESC));
		txDescPhysAddr = mcdmaTxDescPhys[0] + (chan * sizeof(MV_TDM_MCDMA_TX_DESC));
		MV_REG_WRITE(MCDMA_CURRENT_RECEIVE_DESC_PTR_REG(chan), rxDescPhysAddr);
		MV_REG_WRITE(MCDMA_CURRENT_TRANSMIT_DESC_PTR_REG(chan), txDescPhysAddr);
	}

	/* Restore MCDMA Rx/Tx control registers */
	for (chan = 0; chan < totalChannels; chan++) {
		/* Set RMCCx */
		MV_REG_WRITE(MCDMA_RECEIVE_CONTROL_REG(chan), CONFIG_RMCCx);

		/* Set TMCCx */
		MV_REG_WRITE(MCDMA_TRANSMIT_CONTROL_REG(chan), CONFIG_TMCCx);
	}

	/* Set Rx/Tx periodical interrupts */
	if (MV_COMMUNIT_IP_VER_ORIGIN == mvCommUnitIpVerGet(ctrlFamilyId))
		MV_REG_WRITE(VOICE_PERIODICAL_INT_CONTROL_REG, CONFIG_VOICE_PERIODICAL_INT_CONTROL_WA);
	else
		MV_REG_WRITE(VOICE_PERIODICAL_INT_CONTROL_REG, CONFIG_VOICE_PERIODICAL_INT_CONTROL);

	/* MCSC Global Tx Enable */
	if (tdmEnable == MV_FALSE)
		MV_REG_BIT_SET(MCSC_GLOBAL_CONFIG_REG, MCSC_GLOBAL_CONFIG_TXEN_MASK);

	/* Enable MCSC-Tx & MCDMA-Rx */
	for (chan = 0; chan < totalChannels; chan++) {
		/* Enable Tx in TMCCx */
		if (tdmEnable == MV_FALSE)
			MV_REG_BIT_SET(MCSC_CHx_TRANSMIT_CONFIG_REG(chan), MTCRx_ET_MASK);

		/* Enable Rx in: MCRDPx */
		MV_REG_BIT_SET(MCDMA_RECEIVE_CONTROL_REG(chan), MCDMA_ERD_MASK);
	}

	/* MCSC Global Rx Enable */
	if (tdmEnable == MV_FALSE)
		MV_REG_BIT_SET(MCSC_GLOBAL_CONFIG_REG, MCSC_GLOBAL_CONFIG_RXEN_MASK);

	/* Enable MCSC-Rx & MCDMA-Tx */
	for (chan = 0; chan < totalChannels; chan++) {
		/* Enable Rx in RMCCx */
		if (tdmEnable == MV_FALSE)
			MV_REG_BIT_SET(MCSC_CHx_RECEIVE_CONFIG_REG(chan), MRCRx_ER_MASK);

		/* Enable Tx in MCTDPx */
		MV_REG_BIT_SET(MCDMA_TRANSMIT_CONTROL_REG(chan), MCDMA_TXD_MASK);
	}

	/* Disable Rx/Tx return to half */
	MV_REG_BIT_RESET(FLEX_TDM_CONFIG_REG, (TDM_RR2HALF_MASK | TDM_TR2HALF_MASK));
	/* Wait at least 1 frame */
	mvOsUDelay(200);

	MV_TRC_REC("<-%s\n", __func__);
}

MV_VOID mvCommUnitPcmStart(MV_VOID)
{
	MV_U32 maskReg;

	MV_TRC_REC("->%s\n", __func__);

	if (pcmEnable == MV_FALSE) {

		/* Mark PCM I/F as enabled  */
		pcmEnable = MV_TRUE;

		mvCommUnitMcdmaMcscStart();

		/* Clear TDM cause and mask registers */
		MV_REG_WRITE(COMM_UNIT_TOP_MASK_REG, 0);
		MV_REG_WRITE(TDM_MASK_REG, 0);
		MV_REG_WRITE(COMM_UNIT_TOP_CAUSE_REG, 0);
		MV_REG_WRITE(TDM_CAUSE_REG, 0);

		/* Clear MCSC cause and mask registers(except InitDone bit) */
		MV_REG_WRITE(MCSC_GLOBAL_INT_MASK_REG, 0);
		MV_REG_WRITE(MCSC_EXTENDED_INT_MASK_REG, 0);
		MV_REG_WRITE(MCSC_GLOBAL_INT_CAUSE_REG, MCSC_GLOBAL_INT_CAUSE_INIT_DONE_MASK);
		MV_REG_WRITE(MCSC_EXTENDED_INT_CAUSE_REG, 0);

		/* Enable unit interrupts */
		maskReg = MV_REG_READ(TDM_MASK_REG);
		MV_REG_WRITE(TDM_MASK_REG, (maskReg | CONFIG_TDM_CAUSE));
		MV_REG_WRITE(COMM_UNIT_TOP_MASK_REG, CONFIG_COMM_UNIT_TOP_MASK);

		/* Enable TDM */
		if (MV_COMMUNIT_IP_VER_REVISE_1 == mvCommUnitIpVerGet(ctrlFamilyId))
			MV_REG_BIT_SET(FLEX_TDM_CONFIG_REG, TDM_TEN_MASK);
	}

	MV_TRC_REC("<-%s\n", __func__);
}

static MV_VOID mvCommUnitMcdmaMcscAbort(MV_VOID)
{
	MV_U32 chan;

	MV_TRC_REC("->%s\n", __func__);

	/* Abort MCSC/MCDMA in case we got here from mvCommUnitRelease() */
	if (tdmEnable == MV_FALSE) {

#if 0
		/* MCSC Rx Abort */
		for (chan = 0; chan < totalChannels; chan++)
			MV_REG_BIT_SET(MCSC_CHx_RECEIVE_CONFIG_REG(chan), MRCRx_ABORT_MASK);

		for (chan = 0; chan < totalChannels; chan++) {
			maxPoll = 0;
			while ((maxPoll < MAX_POLL_USEC) &&
				!(MV_REG_READ(MCSC_CHx_COMM_EXEC_STAT_REG(chan)) & MCSC_ABR_E_STAT_MASK)) {
				mvOsUDelay(1);
				maxPoll++;
			}

			if (maxPoll >= MAX_POLL_USEC) {
				mvOsPrintf("%s: Error, MCSC Rx abort timeout(ch%d)\n", __func__, chan);
				return;
			}

			MV_REG_BIT_RESET(MCSC_CHx_RECEIVE_CONFIG_REG(chan), MRCRx_ABORT_MASK);
		}

		/* MCDMA Tx Abort */
		for (chan = 0; chan < totalChannels; chan++)
			MV_REG_BIT_SET(MCDMA_TRANSMIT_CONTROL_REG(chan), MCDMA_AT_MASK);

		for (chan = 0; chan < totalChannels; chan++) {
			maxPoll = 0;
			while ((maxPoll < MAX_POLL_USEC) &&
				(MV_REG_READ(MCDMA_RECEIVE_CONTROL_REG(chan)) & MCDMA_ERD_MASK)) {
				mvOsUDelay(1);
				maxPoll++;
			}

			if (maxPoll >= MAX_POLL_USEC) {
				mvOsPrintf("%s: Error, MCDMA Rx abort timeout(ch%d)\n", __func__, chan);
				return;
			}

			maxPoll = 0;
			while ((maxPoll < MAX_POLL_USEC) &&
				(MV_REG_READ(MCDMA_TRANSMIT_CONTROL_REG(chan)) & MCDMA_AT_MASK)) {
				mvOsUDelay(1);
				maxPoll++;
			}

			if (maxPoll >= MAX_POLL_USEC) {
				mvOsPrintf("%s: Error, MCDMA Tx abort timeout(ch%d)\n", __func__, chan);
				return;
			}
		}
#endif
		/* Clear MCSC Rx/Tx channel enable */
		for (chan = 0; chan < totalChannels; chan++) {
			MV_REG_BIT_RESET(MCSC_CHx_RECEIVE_CONFIG_REG(chan), MRCRx_ER_MASK);
			MV_REG_BIT_RESET(MCSC_CHx_TRANSMIT_CONFIG_REG(chan), MTCRx_ET_MASK);
		}

		/* MCSC Global Rx/Tx Disable */
		MV_REG_BIT_RESET(MCSC_GLOBAL_CONFIG_REG, MCSC_GLOBAL_CONFIG_RXEN_MASK);
		MV_REG_BIT_RESET(MCSC_GLOBAL_CONFIG_REG, MCSC_GLOBAL_CONFIG_TXEN_MASK);
	}

	MV_TRC_REC("<-%s\n", __func__);
}

static MV_VOID mvCommUnitMcdmaStop(MV_VOID)
{
	MV_U32 index, chan, maxPoll, currTxDesc;
	MV_U32 currRxDesc, nextTxBuff = 0, nextRxBuff = 0;

	MV_TRC_REC("->%s\n", __func__);

	/***************************/
	/*    Stop MCDMA - Rx/Tx   */
	/***************************/
	for (chan = 0; chan < totalChannels; chan++) {
		currRxDesc = MV_REG_READ(MCDMA_CURRENT_RECEIVE_DESC_PTR_REG(chan));
		for (index = 0; index < TOTAL_CHAINS; index++) {
			if (currRxDesc == (mcdmaRxDescPhys[index] + (chan*(sizeof(MV_TDM_MCDMA_RX_DESC))))) {
				nextRxBuff = NEXT_BUFF(index);
				break;
			}
		}

		if (index == TOTAL_CHAINS) {
			mvOsPrintf("%s: ERROR, couldn't Rx descriptor match for chan(%d)\n", __func__, chan);
			break;
		}

		((MV_TDM_MCDMA_RX_DESC *) (mcdmaRxDescPtr[nextRxBuff] + chan))->physNextDescPtr = 0;
		((MV_TDM_MCDMA_RX_DESC *) (mcdmaRxDescPtr[nextRxBuff] + chan))->cmdStatus = (LAST_BIT | OWNER);
	}

	for (chan = 0; chan < totalChannels; chan++) {
		currTxDesc = MV_REG_READ(MCDMA_CURRENT_TRANSMIT_DESC_PTR_REG(chan));
		for (index = 0; index < TOTAL_CHAINS; index++) {
			if (currTxDesc == (mcdmaTxDescPhys[index] + (chan*(sizeof(MV_TDM_MCDMA_TX_DESC))))) {
				nextTxBuff = NEXT_BUFF(index);
				break;
			}
		}

		if (index == TOTAL_CHAINS) {
			mvOsPrintf("%s: ERROR, couldn't Tx descriptor match for chan(%d)\n", __func__, chan);
			return;
		}

		((MV_TDM_MCDMA_TX_DESC *) (mcdmaTxDescPtr[nextTxBuff] + chan))->physNextDescPtr = 0;
		((MV_TDM_MCDMA_TX_DESC *) (mcdmaTxDescPtr[nextTxBuff] + chan))->cmdStatus = (LAST_BIT | OWNER);
	}

	for (chan = 0; chan < totalChannels; chan++) {
		maxPoll = 0;
		while ((maxPoll < MAX_POLL_USEC) &&
			(MV_REG_READ(MCDMA_TRANSMIT_CONTROL_REG(chan)) & MCDMA_TXD_MASK)) {
			mvOsUDelay(1);
			maxPoll++;
		}

		if (maxPoll >= MAX_POLL_USEC) {
			mvOsPrintf("%s: Error, MCDMA TXD polling timeout(ch%d)\n", __func__, chan);
			return;
		}

		maxPoll = 0;
		while ((maxPoll < MAX_POLL_USEC) &&
			(MV_REG_READ(MCDMA_RECEIVE_CONTROL_REG(chan)) & MCDMA_ERD_MASK)) {
			mvOsUDelay(1);
			maxPoll++;
		}

		if (maxPoll >= MAX_POLL_USEC) {
			mvOsPrintf("%s: Error, MCDMA ERD polling timeout(ch%d)\n", __func__, chan);
			return;
		}
	}

	/* Disable Rx/Tx periodical interrupts */
	MV_REG_WRITE(VOICE_PERIODICAL_INT_CONTROL_REG, 0xffffffff);

	/* Enable Rx/Tx return to half */
	MV_REG_BIT_SET(FLEX_TDM_CONFIG_REG, (TDM_RR2HALF_MASK | TDM_TR2HALF_MASK));
	/* Wait at least 1 frame */
	mvOsUDelay(200);

	/* Manual reset to channel-balancing mechanism */
	MV_REG_BIT_SET(MCSC_GLOBAL_CONFIG_REG, MCSC_GLOBAL_CONFIG_MAI_MASK);
	mvOsUDelay(1);

	MV_TRC_REC("<-%s\n", __func__);
}

MV_VOID mvCommUnitPcmStop(MV_VOID)
{
	MV_U32 buffSize, index;

	MV_TRC_REC("->%s\n", __func__);

	if (pcmEnable == MV_TRUE) {
		/* Mark PCM I/F as disabled  */
		pcmEnable = MV_FALSE;

		/* Clear TDM cause and mask registers */
		MV_REG_WRITE(COMM_UNIT_TOP_MASK_REG, 0);
		MV_REG_WRITE(TDM_MASK_REG, 0);
		MV_REG_WRITE(COMM_UNIT_TOP_CAUSE_REG, 0);
		MV_REG_WRITE(TDM_CAUSE_REG, 0);

		/* Clear MCSC cause and mask registers(except InitDone bit) */
		MV_REG_WRITE(MCSC_GLOBAL_INT_MASK_REG, 0);
		MV_REG_WRITE(MCSC_EXTENDED_INT_MASK_REG, 0);
		MV_REG_WRITE(MCSC_GLOBAL_INT_CAUSE_REG, MCSC_GLOBAL_INT_CAUSE_INIT_DONE_MASK);
		MV_REG_WRITE(MCSC_EXTENDED_INT_CAUSE_REG, 0);

		mvCommUnitMcdmaStop();

		/* Calculate total Rx/Tx buffer size */
		buffSize = (sampleSize * MV_TDM_TOTAL_CH_SAMPLES * samplingCoeff * totalChannels);

		/* Clear Rx buffers */
		for (index = 0; index < TOTAL_CHAINS; index++) {
			memset(rxBuffVirt[index], 0, buffSize);

			/* Flush+Inv buffers */
			mvOsCacheFlushInv(NULL, txBuffVirt[index], buffSize);
		}

		/* Disable TDM */
		if (MV_COMMUNIT_IP_VER_REVISE_1 == mvCommUnitIpVerGet(ctrlFamilyId))
			MV_REG_BIT_RESET(FLEX_TDM_CONFIG_REG, TDM_TEN_MASK);
	}

	MV_TRC_REC("<-%s\n", __func__);
}

MV_STATUS mvCommUnitTx(MV_U8 *pTdmTxBuff)
{
	MV_U32 buffSize;
	MV_U8 tmp;
	MV_U32 index;

	MV_TRC_REC("->%s\n", __func__);

	/* Calculate total Tx buffer size */
	buffSize = (sampleSize * MV_TDM_TOTAL_CH_SAMPLES * samplingCoeff * totalChannels);

	if (MV_COMMUNIT_IP_VER_ORIGIN == mvCommUnitIpVerGet(ctrlFamilyId)) {
		if (sampleSize > MV_PCM_FORMAT_1BYTE) {
			TRC_REC("Linear mode(Tx): swapping bytes\n");
			for (index = 0; index < buffSize; index += 2) {
				tmp = pTdmTxBuff[index];
				pTdmTxBuff[index] = pTdmTxBuff[index+1];
				pTdmTxBuff[index+1] = tmp;
			}
			TRC_REC("Linear mode(Tx): swapping bytes...done.\n");
		}
	}

	/* Flush+Invalidate the next Tx buffer */
	mvOsCacheFlush(NULL, pTdmTxBuff, buffSize);
	mvOsCacheInvalidate(NULL, pTdmTxBuff, buffSize);

	MV_TRC_REC("<-%s\n", __func__);
	return MV_OK;
}

MV_STATUS mvCommUnitRx(MV_U8 *pTdmRxBuff)
{
	MV_U32 buffSize;
	MV_U8 tmp;
	MV_U32 index;

	MV_TRC_REC("->%s\n", __func__);

	/* Calculate total Rx buffer size */
	buffSize = (sampleSize * MV_TDM_TOTAL_CH_SAMPLES * samplingCoeff * totalChannels);

	/* Invalidate current received buffer from cache */
	mvOsCacheInvalidate(NULL, pTdmRxBuff, buffSize);

	if (MV_COMMUNIT_IP_VER_ORIGIN == mvCommUnitIpVerGet(ctrlFamilyId)) {
		if (sampleSize > MV_PCM_FORMAT_1BYTE) {
			TRC_REC("  -> Linear mode(Rx): swapping bytes\n");
			for (index = 0; index < buffSize; index += 2) {
				tmp = pTdmRxBuff[index];
				pTdmRxBuff[index] = pTdmRxBuff[index+1];
				pTdmRxBuff[index+1] = tmp;
			}
			TRC_REC("  <- Linear mode(Rx): swapping bytes...done.\n");
		}
	}
	MV_TRC_REC("<-%s\n", __func__);
	return MV_OK;
}

/* Low level TDM interrupt service routine */
MV_32 mvCommUnitIntLow(MV_TDM_INT_INFO *pTdmIntInfo)
{
	MV_U32 causeReg, maskReg, causeAndMask;
	MV_U32 intAckBits = 0, currDesc;
	MV_U8 index;

	MV_TRC_REC("->%s\n", __func__);

	/* Read TDM cause & mask registers */
	causeReg = MV_REG_READ(TDM_CAUSE_REG);
	maskReg = MV_REG_READ(TDM_MASK_REG);

	MV_TRC_REC("CAUSE(0x%x), MASK(0x%x)\n", causeReg, maskReg);

	/* Refer only to unmasked bits */
	causeAndMask = causeReg & maskReg;

	/* Reset ISR params */
	pTdmIntInfo->tdmRxBuff = NULL;
	pTdmIntInfo->tdmTxBuff = NULL;
	pTdmIntInfo->intType = MV_EMPTY_INT;

#if 0
	/* Handle SLIC interrupt */
	slicInt = (causeAndMask & TDM_SLIC_INT);
	if (slicInt) {
		MV_TRC_REC("SLIC interrupt !!!\n");
		pTdmIntInfo->intType |= MV_PHONE_INT;
		for (cs = 0; cs < maxCs; cs++) {
			if (slicInt & MV_BIT_MASK(cs + EXT_INT_SLIC0_OFFS)) {
				pTdmIntInfo->cs = cs;
				mvOsPrintf("");
				intAckBits |= MV_BIT_MASK(cs + EXT_INT_SLIC0_OFFS);
				break;
			}

		}
		mvOsPrintf("pTdmIntInfo->cs = %d\n", pTdmIntInfo->cs);
	}
#endif
	/* Return in case TDM is disabled */
	if (tdmEnable == MV_FALSE) {
		MV_TRC_REC("TDM is disabled - quit low level ISR\n");
		MV_REG_WRITE(TDM_CAUSE_REG, ~intAckBits);
		return 0;
	}

	/* Handle TDM Error/s */
	if (causeAndMask & TDM_ERROR_INT) {
		mvOsPrintf("TDM Error: TDM_CAUSE_REG = 0x%x\n", causeReg);
		/*pTdmIntInfo->intType |= MV_ERROR_INT;*/
		intAckBits |= (causeAndMask & TDM_ERROR_INT);
	}

	if (causeAndMask & (TDM_TX_INT | TDM_RX_INT)) {
		/* MCDMA current Tx desc. pointer is unreliable, thus, checking Rx desc. pointer only */
		currDesc = MV_REG_READ(MCDMA_CURRENT_RECEIVE_DESC_PTR_REG(0));
		MV_TRC_REC("currDesc = 0x%x\n", currDesc);

		/* Handle Tx */
		if (causeAndMask & TDM_TX_INT) {
			for (index = 0; index < TOTAL_CHAINS; index++) {
				if (currDesc == mcdmaRxDescPhys[index]) {
					nextTxBuff = NEXT_BUFF(index);
					break;
				}
			}
			MV_TRC_REC("Tx interrupt(nextTxBuff=%d)!!!\n", nextTxBuff);
			pTdmIntInfo->tdmTxBuff = txBuffVirt[nextTxBuff];
			pTdmIntInfo->intType |= MV_TX_INT;
			intAckBits |= TDM_TX_INT;
		}

		/* Handle Rx */
		if (causeAndMask & TDM_RX_INT) {
			for (index = 0; index < TOTAL_CHAINS; index++) {
				if (currDesc == mcdmaRxDescPhys[index]) {
					prevRxBuff = PREV_BUFF(index);
					break;
				}
			}
			MV_TRC_REC("Rx interrupt(prevRxBuff=%d)!!!\n", prevRxBuff);
			pTdmIntInfo->tdmRxBuff = rxBuffVirt[prevRxBuff];
			pTdmIntInfo->intType |= MV_RX_INT;
			intAckBits |= TDM_RX_INT;
		}
	}

	/* Clear TDM interrupts */
	MV_REG_WRITE(TDM_CAUSE_REG, ~intAckBits);

	TRC_REC("<-%s\n", __func__);
	return 0;
}

static MV_VOID mvCommUnitDescChainBuild(MV_VOID)
{
	MV_U32 chan, index, buffSize;

	TRC_REC("->%s\n", __func__);

	/* Calculate single Rx/Tx buffer size */
	buffSize = (sampleSize * MV_TDM_TOTAL_CH_SAMPLES * samplingCoeff);

	/* Initialize descriptors fields */
	for (chan = 0; chan < totalChannels; chan++) {
		for (index = 0; index < TOTAL_CHAINS; index++) {
			/* Associate data buffers to descriptors physBuffPtr */
			((MV_TDM_MCDMA_RX_DESC *) (mcdmaRxDescPtr[index] + chan))->physBuffPtr =
			    (MV_U32) (rxBuffPhys[index] + (chan * buffSize));
			((MV_TDM_MCDMA_TX_DESC *) (mcdmaTxDescPtr[index] + chan))->physBuffPtr =
			    (MV_U32) (txBuffPhys[index] + (chan * buffSize));

			/* Build cyclic descriptors chain for each channel */
			((MV_TDM_MCDMA_RX_DESC *) (mcdmaRxDescPtr[index] + chan))->physNextDescPtr =
			    (MV_U32) (mcdmaRxDescPhys[((index + 1) % TOTAL_CHAINS)] +
				      (chan * sizeof(MV_TDM_MCDMA_RX_DESC)));

			((MV_TDM_MCDMA_TX_DESC *) (mcdmaTxDescPtr[index] + chan))->physNextDescPtr =
			    (MV_U32) (mcdmaTxDescPhys[((index + 1) % TOTAL_CHAINS)] +
				      (chan * sizeof(MV_TDM_MCDMA_TX_DESC)));

			/* Set Byte_Count/Buffer_Size Rx descriptor fields */
			((MV_TDM_MCDMA_RX_DESC *) (mcdmaRxDescPtr[index] + chan))->byteCnt = 0;
			((MV_TDM_MCDMA_RX_DESC *) (mcdmaRxDescPtr[index] + chan))->buffSize = buffSize;

			/* Set Shadow_Byte_Count/Byte_Count Tx descriptor fields */
			((MV_TDM_MCDMA_TX_DESC *) (mcdmaTxDescPtr[index] + chan))->shadowByteCnt = buffSize;
			((MV_TDM_MCDMA_TX_DESC *) (mcdmaTxDescPtr[index] + chan))->byteCnt = buffSize;

			/* Set Command/Status Rx/Tx descriptor fields */
			((MV_TDM_MCDMA_RX_DESC *) (mcdmaRxDescPtr[index] + chan))->cmdStatus =
			    (CONFIG_MCDMA_DESC_CMD_STATUS);
			((MV_TDM_MCDMA_TX_DESC *) (mcdmaTxDescPtr[index] + chan))->cmdStatus =
			    (CONFIG_MCDMA_DESC_CMD_STATUS);
		}
	}

	TRC_REC("<-%s\n", __func__);
	return;
}

MV_VOID mvCommUnitIntEnable(MV_U8 deviceId)
{
	/* MV_REG_BIT_SET(MV_GPP_IRQ_MASK_REG(0), BIT23); */
}

MV_VOID mvCommUnitIntDisable(MV_U8 deviceId)
{
	/* MV_REG_BIT_RESET(MV_GPP_IRQ_MASK_REG(0), BIT23); */
}

MV_VOID mvCommUnitShow(MV_VOID)
{
	MV_U32 index;

	/* Dump data buffers & descriptors addresses */
	for (index = 0; index < TOTAL_CHAINS; index++) {
		mvOsPrintf("Rx Buff(%d): virt = 0x%x, phys = 0x%x\n", index, (MV_U32) rxBuffVirt[index],
			   (MV_U32) rxBuffPhys[index]);
		mvOsPrintf("Tx Buff(%d): virt = 0x%x, phys = 0x%x\n", index, (MV_U32) txBuffVirt[index],
			   (MV_U32) txBuffPhys[index]);

		mvOsPrintf("Rx Desc(%d): virt = 0x%x, phys = 0x%x\n", index,
			   (MV_U32) mcdmaRxDescPtr[index], (MV_U32) mcdmaRxDescPhys[index]);

		mvOsPrintf("Tx Desc(%d): virt = 0x%x, phys = 0x%x\n", index,
			   (MV_U32) mcdmaTxDescPtr[index], (MV_U32) mcdmaTxDescPhys[index]);

	}
}

MV_STATUS mvCommUnitResetSlic(MV_VOID)
{
	/* Enable SLIC reset */
	MV_REG_BIT_RESET(TDM_CLK_AND_SYNC_CONTROL_REG, TDM_PROG_TDM_SLIC_RESET_MASK);

	mvOsUDelay(60);

	/* Release SLIC reset */
	MV_REG_BIT_SET(TDM_CLK_AND_SYNC_CONTROL_REG, TDM_PROG_TDM_SLIC_RESET_MASK);

	return MV_OK;
}
