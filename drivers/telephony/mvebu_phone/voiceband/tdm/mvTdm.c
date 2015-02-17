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
#include "voiceband/tdm/mvTdm.h"

/* defines */
#define INT_SAMPLE			2
#define BUFF_IS_FULL			1
#define BUFF_IS_EMPTY			0
#define FIRST_INT			1
#define TOTAL_BUFFERS			2
#define MV_TDM_NEXT_BUFFER(buf)		((buf + 1) % TOTAL_BUFFERS)
#define MV_TDM_PREV_BUFFER(buf, step) 	((TOTAL_BUFFERS + buf - step) % TOTAL_BUFFERS)
#define MV_TDM_CS			0
#define BUFF_INVALID			-1

/* static APIs */
static MV_STATUS mvTdmChInit(MV_U8 ch);
static MV_STATUS mvTdmChRemove(MV_U8 ch);
static MV_VOID mvTdmReset(MV_VOID);
static MV_VOID mvTdmDaisyChainModeSet(MV_VOID);
static MV_VOID mvTdmShowProperties(MV_VOID);
/* Tx */
static INLINE MV_STATUS mvTdmChTxLow(MV_U8 ch);
/* Rx */
static INLINE MV_STATUS mvTdmChRxLow(MV_U8 ch);

/* TDM channel info structure */
typedef struct _mv_tdm_ch_info {
	MV_U8 ch;
	MV_U8 *rxBuffVirt[TOTAL_BUFFERS], *txBuffVirt[TOTAL_BUFFERS];
	MV_ULONG rxBuffPhys[TOTAL_BUFFERS], txBuffPhys[TOTAL_BUFFERS];
	MV_U8 rxBuffFull[TOTAL_BUFFERS], txBuffFull[TOTAL_BUFFERS];
	MV_U8 rxCurrBuff, txCurrBuff;
	MV_U8 rxFirst;
} MV_TDM_CH_INFO;

/* globals */
static MV_U8 *rxAggrBuffVirt, *txAggrBuffVirt;
static MV_ULONG rxAggrBuffPhys, txAggrBuffPhys;
static MV_U8 rxInt, txInt;
static MV_16 rxFull, txEmpty;
static MV_U8 tdmEnable;
static MV_U8 spiMode;
static MV_U8 factor;
static MV_PCM_FORMAT pcmFormat;
static MV_BAND_MODE tdmBandMode;
static MV_TDM_CH_INFO *tdmChInfo[MV_TDM2C_TOTAL_CHANNELS] = { NULL, NULL };
static volatile MV_U8 chanStopCount;

static MV_U8 intLock;
static MV_U32 intRxCount;
static MV_U32 intTxCount;
static MV_U32 intRx0Count;
static MV_U32 intTx0Count;
static MV_U32 intRx1Count;
static MV_U32 intTx1Count;
static MV_U32 intRx0Miss;
static MV_U32 intTx0Miss;
static MV_U32 intRx1Miss;
static MV_U32 intTx1Miss;
static MV_U32 pcmRestartCount;

MV_STATUS mvTdmHalInit(MV_TDM_PARAMS *tdmParams, MV_TDM_HAL_DATA *halData)
{
	MV_U8 ch;
	MV_U32 pcmCtrlReg, nbDelay = 0, wbDelay = 0;
	MV_U32 chDelay[4] = { 0, 0, 0, 0 };

	MV_TRC_REC("->%s\n", __func__);
	mvTdmShowProperties();

	/* Init globals */
	rxInt = txInt = 0;
	rxFull = txEmpty = BUFF_INVALID;
	tdmEnable = 0, intLock = 0;
	spiMode = halData->spiMode;
	pcmFormat = tdmParams->pcmFormat;
	intRxCount = 0, intTxCount = 0;
	intRx0Count = 0, intTx0Count = 0;
	intRx1Count = 0, intTx1Count = 0;
	intRx0Miss = 0, intTx0Miss = 0;
	intRx1Miss = 0, intTx1Miss = 0;
	pcmRestartCount = 0;

	if (tdmParams->samplingPeriod > MV_TDM_MAX_SAMPLING_PERIOD)
		factor = 1;	/* use base sample period(10ms) */
	else
		factor = (tdmParams->samplingPeriod / MV_TDM_BASE_SAMPLING_PERIOD);

	/* Extract pcm format & band mode */
	if (pcmFormat == MV_PCM_FORMAT_4BYTES) {
		pcmFormat = MV_PCM_FORMAT_2BYTES;
		tdmBandMode = MV_WIDE_BAND;
	} else {
		tdmBandMode = MV_NARROW_BAND;
	}

	/* Allocate aggregated buffers for data transport */
	MV_TRC_REC("allocate %d bytes for aggregated buffer\n", MV_TDM_AGGR_BUFF_SIZE(pcmFormat, tdmBandMode, factor));
	rxAggrBuffVirt = (MV_U8 *) mvOsIoCachedMalloc(NULL, MV_TDM_AGGR_BUFF_SIZE(pcmFormat, tdmBandMode, factor),
						      &rxAggrBuffPhys, NULL);
	txAggrBuffVirt = (MV_U8 *) mvOsIoCachedMalloc(NULL, MV_TDM_AGGR_BUFF_SIZE(pcmFormat, tdmBandMode, factor),
						      &txAggrBuffPhys, NULL);
	if (!rxAggrBuffVirt || !txAggrBuffVirt) {
		mvOsPrintf("%s: Error malloc failed\n", __func__);
		return MV_NO_RESOURCE;
	}

	/* Clear buffers */
	memset(rxAggrBuffVirt, 0, MV_TDM_AGGR_BUFF_SIZE(pcmFormat, tdmBandMode, factor));
	memset(txAggrBuffVirt, 0, MV_TDM_AGGR_BUFF_SIZE(pcmFormat, tdmBandMode, factor));

	/* Calculate CH(0/1) Delay Control for narrow/wideband modes */
	for (ch = 0; ch < MV_TDM2C_TOTAL_CHANNELS; ch++) {
		nbDelay = ((tdmParams->pcmSlot[ch] * PCM_SLOT_PCLK) + 1);
		wbDelay = (nbDelay + ((halData->frameTs / 2) * PCM_SLOT_PCLK)); /* Offset required by ZARLINK VE880 SLIC */
		chDelay[ch] = ((nbDelay << CH_RX_DELAY_OFFS) | (nbDelay << CH_TX_DELAY_OFFS));
		chDelay[(ch + 2)] = ((wbDelay << CH_RX_DELAY_OFFS) | (wbDelay << CH_TX_DELAY_OFFS));
	}

	/* Config TDM */
	MV_REG_BIT_RESET(TDM_SPI_MUX_REG, BIT0);	/* enable TDM/SPI interface */
	MV_REG_BIT_SET(TDM_MISC_REG, BIT0);		/* sw reset to TDM for 5181L-A1 & up */
	MV_REG_WRITE(INT_RESET_SELECT_REG, CLEAR_ON_ZERO);	/* int cause is not clear on read */
	MV_REG_WRITE(INT_EVENT_MASK_REG, 0x3ffff);	/* all interrupt bits latched in status */
	MV_REG_WRITE(INT_STATUS_MASK_REG, 0);	/* disable interrupts */
	MV_REG_WRITE(INT_STATUS_REG, 0);	/* clear int status register */

	/* Bypass clock divider */
	MV_REG_WRITE(PCM_CLK_RATE_DIV_REG, PCM_DIV_PASS);	/* PCM PCLK freq */

	MV_REG_WRITE(DUMMY_RX_WRITE_DATA_REG, 0);	/* Padding on Rx completion */
	MV_REG_BYTE_WRITE(SPI_GLOBAL_CTRL_REG, MV_REG_READ(SPI_GLOBAL_CTRL_REG) | SPI_GLOBAL_ENABLE);
	MV_REG_BYTE_WRITE(SPI_CLK_PRESCALAR_REG, SPI_CLK_2MHZ);	/* SPI SCLK freq */
	MV_REG_WRITE(FRAME_TIMESLOT_REG, (MV_U32)halData->frameTs); /* Number of timeslots (PCLK) */

	if (tdmBandMode == MV_NARROW_BAND) {
		pcmCtrlReg = (CONFIG_PCM_CRTL | (((MV_U8)pcmFormat - 1) << PCM_SAMPLE_SIZE_OFFS));
		MV_REG_WRITE(PCM_CTRL_REG, pcmCtrlReg);	/* PCM configuration */
		MV_REG_WRITE(CH_DELAY_CTRL_REG(0), chDelay[0]);	/* CH0 delay control register */
		MV_REG_WRITE(CH_DELAY_CTRL_REG(1), chDelay[1]);	/* CH1 delay control register */
	} else {		/* MV_WIDE_BAND */

		pcmCtrlReg = (CONFIG_WB_PCM_CRTL | (((MV_U8)pcmFormat - 1) << PCM_SAMPLE_SIZE_OFFS));
		MV_REG_WRITE(PCM_CTRL_REG, pcmCtrlReg);	/* PCM configuration - WB support */
		MV_REG_WRITE(CH_DELAY_CTRL_REG(0), chDelay[0]);	/* CH0 delay control register */
		MV_REG_WRITE(CH_DELAY_CTRL_REG(1), chDelay[1]);	/* CH1 delay control register */
		MV_REG_WRITE(CH_WB_DELAY_CTRL_REG(0), chDelay[2]);	/* CH0 WB delay control register */
		MV_REG_WRITE(CH_WB_DELAY_CTRL_REG(1), chDelay[3]);	/* CH1 WB delay control register */
	}

	/* Issue reset to codec(s) */
	MV_TRC_REC("reseting voice unit(s)\n");
	MV_REG_WRITE(MISC_CTRL_REG, 0);
	mvOsDelay(1);
	MV_REG_WRITE(MISC_CTRL_REG, 1);

	if (spiMode) {
		/* Configure TDM to work in daisy chain mode */
		mvTdmDaisyChainModeSet();
	}

	/* Initialize all HW units */
	for (ch = 0; ch < MV_TDM2C_TOTAL_CHANNELS; ch++) {
		if (mvTdmChInit(ch) != MV_OK) {
			mvOsPrintf("mvTdmChInit(%d) failed !\n", ch);
			return MV_ERROR;
		}
	}

	/* Enable SLIC/DAA interrupt detection(before pcm is active) */
	MV_REG_WRITE(INT_STATUS_MASK_REG, (MV_REG_READ(INT_STATUS_MASK_REG) | TDM_INT_SLIC));

	MV_TRC_REC("<-%s\n", __func__);
	return MV_OK;
}

static MV_STATUS mvTdmChInit(MV_U8 ch)
{
	MV_TDM_CH_INFO *chInfo;
	MV_U32 buff;

	MV_TRC_REC("->%s ch%d\n", __func__, ch);

	if (ch >= MV_TDM2C_TOTAL_CHANNELS) {
		mvOsPrintf("%s: error, channel(%d) exceeds maximum(%d)\n", __func__, ch, MV_TDM2C_TOTAL_CHANNELS);
		return MV_BAD_PARAM;
	}

	tdmChInfo[ch] = chInfo = (MV_TDM_CH_INFO *) mvOsMalloc(sizeof(MV_TDM_CH_INFO));
	if (!chInfo) {
		mvOsPrintf("%s: error malloc failed\n", __func__);
		return MV_NO_RESOURCE;
	}

	chInfo->ch = ch;

	/* Per channel TDM init */
	MV_REG_WRITE(CH_ENABLE_REG(ch), CH_DISABLE);	/* disable channel (enable in pcm start) */
	MV_REG_WRITE(CH_SAMPLE_REG(ch), CONFIG_CH_SAMPLE(tdmBandMode, factor));	/* set total samples and int sample */

	for (buff = 0; buff < TOTAL_BUFFERS; buff++) {
		/* Buffers must be 32B aligned */
		chInfo->rxBuffVirt[buff] =
		    (MV_U8 *) mvOsIoUncachedMalloc(NULL, MV_TDM_CH_BUFF_SIZE(pcmFormat, tdmBandMode, factor),
						   &(chInfo->rxBuffPhys[buff]), NULL);
		chInfo->rxBuffFull[buff] = BUFF_IS_EMPTY;

		chInfo->txBuffVirt[buff] =
		    (MV_U8 *) mvOsIoUncachedMalloc(NULL, MV_TDM_CH_BUFF_SIZE(pcmFormat, tdmBandMode, factor),
						   &(chInfo->txBuffPhys[buff]), NULL);
		chInfo->txBuffFull[buff] = BUFF_IS_FULL;

		memset(chInfo->txBuffVirt[buff], 0, MV_TDM_CH_BUFF_SIZE(pcmFormat, tdmBandMode, factor));

		if (((MV_ULONG) chInfo->rxBuffVirt[buff] | chInfo->rxBuffPhys[buff] |
		     (MV_ULONG) chInfo->txBuffVirt[buff] | chInfo->txBuffPhys[buff]) & 0x1f) {
			mvOsPrintf("%s: error, unaligned buffer allocation\n", __func__);
		}
	}

	MV_TRC_REC("<-%s\n", __func__);
	return MV_OK;
}

MV_VOID mvTdmRelease(MV_VOID)
{
	MV_U8 ch;

	MV_TRC_REC("->%s\n", __func__);

	/* Free Rx/Tx aggregated buffers */
	mvOsIoCachedFree(NULL, MV_TDM_AGGR_BUFF_SIZE(pcmFormat, tdmBandMode, factor), rxAggrBuffPhys,
			 rxAggrBuffVirt, 0);
	mvOsIoCachedFree(NULL, MV_TDM_AGGR_BUFF_SIZE(pcmFormat, tdmBandMode, factor), txAggrBuffPhys,
			 txAggrBuffVirt, 0);

	/* Release HW channel resources */
	for (ch = 0; ch < MV_TDM2C_TOTAL_CHANNELS; ch++)
		mvTdmChRemove(ch);

	MV_TRC_REC("<-%s\n", __func__);
}

static MV_STATUS mvTdmChRemove(MV_U8 ch)
{
	MV_TDM_CH_INFO *chInfo;
	MV_U8 buff;

	MV_TRC_REC("->%s ch%d\n", __func__, ch);

	if (ch >= MV_TDM2C_TOTAL_CHANNELS) {
		mvOsPrintf("%s: error, channel(%d) exceeds maximum(%d)\n", __func__, ch, MV_TDM2C_TOTAL_CHANNELS);
		return MV_BAD_PARAM;
	}

	chInfo = tdmChInfo[ch];

	for (buff = 0; buff < TOTAL_BUFFERS; buff++) {
		mvOsIoUncachedFree(NULL, MV_TDM_CH_BUFF_SIZE(pcmFormat, tdmBandMode, factor), chInfo->rxBuffPhys[buff],
				   chInfo->rxBuffVirt[buff], 0);
		mvOsIoUncachedFree(NULL, MV_TDM_CH_BUFF_SIZE(pcmFormat, tdmBandMode, factor), chInfo->txBuffPhys[buff],
				   chInfo->txBuffVirt[buff], 0);
	}

	mvOsFree(chInfo);

	MV_TRC_REC("<-%s ch%d\n", __func__, ch);
	return MV_OK;
}

static MV_VOID mvTdmReset(MV_VOID)
{
	MV_TDM_CH_INFO *chInfo;
	MV_U8 buff, ch;

	MV_TRC_REC("->%s\n", __func__);

	/* Reset globals */
	rxInt = txInt = 0;
	rxFull = txEmpty = BUFF_INVALID;

	for (ch = 0; ch < MV_TDM2C_TOTAL_CHANNELS; ch++) {
		chInfo = tdmChInfo[ch];
		chInfo->rxFirst = FIRST_INT;
		chInfo->txCurrBuff = chInfo->rxCurrBuff = 0;
		for (buff = 0; buff < TOTAL_BUFFERS; buff++) {
			chInfo->rxBuffFull[buff] = BUFF_IS_EMPTY;
			chInfo->txBuffFull[buff] = BUFF_IS_FULL;

		}
	}

	MV_TRC_REC("<-%s\n", __func__);
	return;
}

MV_VOID mvTdmPcmStart(MV_VOID)
{
	MV_TDM_CH_INFO *chInfo;
	MV_U8 ch;

	MV_TRC_REC("->%s\n", __func__);

	tdmEnable = 1;		/* TDM is enabled  */
	intLock = 0;
	chanStopCount = 0;
	mvTdmReset();

	for (ch = 0; ch < MV_TDM2C_TOTAL_CHANNELS; ch++) {
		chInfo = tdmChInfo[ch];

		/* Set Tx buff */
		MV_REG_WRITE(CH_TX_ADDR_REG(ch), chInfo->txBuffPhys[chInfo->txCurrBuff]);
		MV_REG_BYTE_WRITE(CH_BUFF_OWN_REG(ch) + TX_OWN_BYTE_OFFS, OWN_BY_HW);

		/* Set Rx buff */
		MV_REG_WRITE(CH_RX_ADDR_REG(ch), chInfo->rxBuffPhys[chInfo->rxCurrBuff]);
		MV_REG_BYTE_WRITE(CH_BUFF_OWN_REG(ch) + RX_OWN_BYTE_OFFS, OWN_BY_HW);

	}

	/* Enable Tx */
	MV_REG_BYTE_WRITE(CH_ENABLE_REG(0) + TX_ENABLE_BYTE_OFFS, CH_ENABLE);
	MV_REG_BYTE_WRITE(CH_ENABLE_REG(1) + TX_ENABLE_BYTE_OFFS, CH_ENABLE);

	/* Enable Rx */
	MV_REG_BYTE_WRITE(CH_ENABLE_REG(0) + RX_ENABLE_BYTE_OFFS, CH_ENABLE);
	MV_REG_BYTE_WRITE(CH_ENABLE_REG(1) + RX_ENABLE_BYTE_OFFS, CH_ENABLE);

	/* Enable Tx interrupts */
	MV_REG_WRITE(INT_STATUS_REG, MV_REG_READ(INT_STATUS_REG) & (~(TDM_INT_TX(0) | TDM_INT_TX(1))));
	MV_REG_WRITE(INT_STATUS_MASK_REG, (MV_REG_READ(INT_STATUS_MASK_REG) | TDM_INT_TX(0) | TDM_INT_TX(1)));

	/* Enable Rx interrupts */
	MV_REG_WRITE(INT_STATUS_REG, (MV_REG_READ(INT_STATUS_REG) & (~(TDM_INT_RX(0) | TDM_INT_RX(1)))));
	MV_REG_WRITE(INT_STATUS_MASK_REG, (MV_REG_READ(INT_STATUS_MASK_REG) | TDM_INT_RX(0) | TDM_INT_RX(1)));

	MV_TRC_REC("<-%s\n", __func__);
}

MV_VOID mvTdmPcmStop(MV_VOID)
{
	MV_TRC_REC("->%s\n", __func__);

	tdmEnable = 0;
	mvTdmReset();

	/* Mask all interrpts except SLIC/DAA */
	/*MV_REG_WRITE(INT_STATUS_MASK_REG, (MV_U32)TDM_INT_SLIC);*/

	MV_TRC_REC("<-%s\n", __func__);
}

MV_STATUS mvTdmTx(MV_U8 *tdmTxBuff)
{
	MV_TDM_CH_INFO *chInfo;
	MV_U8 ch;
	MV_U8 *pTdmTxBuff;

	MV_TRC_REC("->%s\n", __func__);

	/* Sanity check */
	if (tdmTxBuff != txAggrBuffVirt) {
		mvOsPrintf("%s: Error, invalid Tx buffer !!!\n", __func__);
		return MV_ERROR;
	}

	if (!tdmEnable) {
		mvOsPrintf("%s: Error, no active Tx channels are available\n", __func__);
		return MV_ERROR;
	}

	if (txEmpty == BUFF_INVALID) {
		MV_TRC_REC("%s: Tx not ready\n", __func__);
		return MV_NOT_READY;
	}

	for (ch = 0; ch < MV_TDM2C_TOTAL_CHANNELS; ch++) {
		chInfo = tdmChInfo[ch];
		MV_TRC_REC("ch%d: fill buf %d with %d bytes\n", ch, txEmpty,
			   MV_TDM_CH_BUFF_SIZE(pcmFormat, tdmBandMode, factor));
		chInfo->txBuffFull[txEmpty] = BUFF_IS_FULL;
		pTdmTxBuff = tdmTxBuff + (ch * MV_TDM_CH_BUFF_SIZE(pcmFormat, tdmBandMode, factor));
		/* Copy data from voice engine buffer to DMA */
		mvOsMemcpy(chInfo->txBuffVirt[txEmpty], pTdmTxBuff,
			   MV_TDM_CH_BUFF_SIZE(pcmFormat, tdmBandMode, factor));
	}

	txEmpty = BUFF_INVALID;

	MV_TRC_REC("<-%s\n", __func__);
	return MV_OK;
}

MV_STATUS mvTdmRx(MV_U8 *tdmRxBuff)
{
	MV_TDM_CH_INFO *chInfo;
	MV_U8 ch;
	MV_U8 *pTdmRxBuff;

	MV_TRC_REC("->%s\n", __func__);

	/* Sanity check */
	if (tdmRxBuff != rxAggrBuffVirt) {
		mvOsPrintf("%s: invalid Rx buffer !!!\n", __func__);
		return MV_ERROR;
	}

	if (!tdmEnable) {
		mvOsPrintf("%s: Error, no active Rx channels are available\n", __func__);
		return MV_ERROR;
	}

	if (rxFull == BUFF_INVALID) {
		MV_TRC_REC("%s: Rx not ready\n", __func__);
		return MV_NOT_READY;
	}

	for (ch = 0; ch < MV_TDM2C_TOTAL_CHANNELS; ch++) {
		chInfo = tdmChInfo[ch];
		chInfo->rxBuffFull[rxFull] = BUFF_IS_EMPTY;
		MV_TRC_REC("%s get Rx buffer(%d) for channel(%d)\n", __func__, rxFull, ch);
		pTdmRxBuff = tdmRxBuff + (ch * MV_TDM_CH_BUFF_SIZE(pcmFormat, tdmBandMode, factor));
		/* Copy data from DMA to voice engine buffer */
		mvOsMemcpy(pTdmRxBuff, chInfo->rxBuffVirt[rxFull], MV_TDM_CH_BUFF_SIZE(pcmFormat, tdmBandMode, factor));
	}

	rxFull = BUFF_INVALID;
	MV_TRC_REC("<-%s\n", __func__);
	return MV_OK;
}

MV_32 mvPcmStopIntMiss(void)
{
	MV_U32 statusReg, maskReg, statusStopInt, statusMask = 0, intMask = 0, ret = 0;

	MV_TRC_REC("->%s\n", __func__);

	statusReg = MV_REG_READ(INT_STATUS_REG);
	maskReg = MV_REG_READ(INT_STATUS_MASK_REG);

	/* Refer only to unmasked bits */
	statusStopInt = statusReg & maskReg;

	if (statusStopInt & TX_UNDERFLOW_BIT(1)) {
		statusMask |= TX_UNDERFLOW_BIT(1);
		intMask |= TDM_INT_TX(1);
	}

	if (statusStopInt & TX_UNDERFLOW_BIT(0)) {
		statusMask |= TX_UNDERFLOW_BIT(0);
		intMask |= TDM_INT_TX(0);
	}

	if (statusStopInt & RX_OVERFLOW_BIT(1)) {
		statusMask |= RX_OVERFLOW_BIT(1);
		intMask |= TDM_INT_RX(1);
	}

	if (statusStopInt & RX_OVERFLOW_BIT(0)) {
		statusMask |= TX_UNDERFLOW_BIT(0);
		intMask |= TDM_INT_RX(0);
	}

	if (intMask != 0) {
		MV_TRC_REC("Stop Interrupt missing found STATUS=%x, MASK=%x\n", statusReg, maskReg);
		MV_REG_WRITE(INT_STATUS_REG, ~(statusMask));
		MV_REG_WRITE(INT_STATUS_MASK_REG,
					     MV_REG_READ(INT_STATUS_MASK_REG) & (~(intMask)));
		ret = 1;
	}
	MV_TRC_REC("<-%s\n", __func__);
	return ret;
}

/* Low level TDM interrupt service routine */
MV_32 mvTdmIntLow(MV_TDM_INT_INFO *tdmIntInfo)
{
	MV_U32 statusReg, maskReg, statusAndMask;
	MV_32 ret = 0;
	MV_32 intTxMiss = -1;
	MV_32 intRxMiss = -1;
	MV_U8 ch;

	MV_TRC_REC("->%s\n", __func__);

	/* Read Status & mask registers */
	statusReg = MV_REG_READ(INT_STATUS_REG);
	maskReg = MV_REG_READ(INT_STATUS_MASK_REG);
	MV_TRC_REC("CAUSE(0x%x), MASK(0x%x)\n", statusReg, maskReg);

	/* Refer only to unmasked bits */
	statusAndMask = statusReg & maskReg;

	/* Reset params */
	tdmIntInfo->tdmRxBuff = NULL;
	tdmIntInfo->tdmTxBuff = NULL;
	tdmIntInfo->intType = MV_EMPTY_INT;
	tdmIntInfo->cs = MV_TDM_CS;

	/* Handle SLIC/DAA int */
	if (statusAndMask & SLIC_INT_BIT) {
		MV_TRC_REC("Phone interrupt !!!\n");
		tdmIntInfo->intType |= MV_PHONE_INT;
	}
#if 0
	/* Return in case TDM is disabled */
	if (!tdmEnable) {
		MV_TRC_REC("TDM is disabled - quit low level ISR\n");
		MV_REG_WRITE(INT_STATUS_REG, ~statusReg);
		return;
	}
#endif
	if (statusAndMask & DMA_ABORT_BIT) {
		mvOsPrintf("DMA data abort. Address: 0x%08x, Info: 0x%08x\n",
			   MV_REG_READ(DMA_ABORT_ADDR_REG), MV_REG_READ(DMA_ABORT_INFO_REG));
		tdmIntInfo->intType |= MV_DMA_ERROR_INT;
	}

	for (ch = 0; ch < MV_TDM2C_TOTAL_CHANNELS; ch++) {

		/* Give next buff to TDM and set curr buff as empty */
		if ((statusAndMask & TX_BIT(ch)) && tdmEnable && !intLock) {
			MV_TRC_REC("Tx interrupt(ch%d) !!!\n", ch);

			intTxCount++;
			if (ch == 0) {
				intTx0Count++;
				if (intTx0Count <= intTx1Count) {
					intTxMiss = 0;
					intTx0Miss++;
				}
			} else {
				intTx1Count++;
				if (intTx1Count < intTx0Count) {
					intTxMiss = 1;
					intTx1Miss++;
				}
			}

			/* MV_OK -> Tx is done for both channels */
			if (mvTdmChTxLow(ch) == MV_OK) {
				MV_TRC_REC("Assign Tx aggregate buffer for further processing\n");
				tdmIntInfo->tdmTxBuff = txAggrBuffVirt;
				tdmIntInfo->intType |= MV_TX_INT;
			}
		}
	}

	for (ch = 0; ch < MV_TDM2C_TOTAL_CHANNELS; ch++) {

		if ((statusAndMask & RX_BIT(ch)) && tdmEnable && !intLock) {
			MV_TRC_REC("Rx interrupt(ch%d) !!!\n", ch);

			intRxCount++;
			if (ch == 0) {
				intRx0Count++;
				if (intRx0Count <= intRx1Count) {
					intRxMiss = 0;
					intRx0Miss++;
				}
			} else {
				intRx1Count++;
				if (intRx1Count < intRx0Count) {
					intRxMiss = 1;
					intRx1Miss++;
				}
			}

			/* MV_OK -> Rx is done for both channels */
			if (mvTdmChRxLow(ch) == MV_OK) {
				MV_TRC_REC("Assign Rx aggregate buffer for further processing\n");
				tdmIntInfo->tdmRxBuff = rxAggrBuffVirt;
				tdmIntInfo->intType |= MV_RX_INT;
			}
		}
	}

	for (ch = 0; ch < MV_TDM2C_TOTAL_CHANNELS; ch++) {

		if (statusAndMask & TX_UNDERFLOW_BIT(ch)) {

			MV_TRC_REC("Tx underflow(ch%d) - checking for root cause...\n", ch);
			if (tdmEnable) {
				MV_TRC_REC("Tx underflow ERROR\n");
				tdmIntInfo->intType |= MV_TX_ERROR_INT;
				if (!(statusAndMask & TX_BIT(ch))) {
					ret = -1;
					/* MV_OK -> Tx is done for both channels */
					if (mvTdmChTxLow(ch) == MV_OK) {
						MV_TRC_REC("Assign Tx aggregate buffer for further processing\n");
						tdmIntInfo->tdmTxBuff = txAggrBuffVirt;
						tdmIntInfo->intType |= MV_TX_INT;
					}
				}
			} else {
				MV_TRC_REC("Expected Tx underflow(not an error)\n");
				tdmIntInfo->intType |= MV_CHAN_STOP_INT;
				tdmIntInfo->data = ++chanStopCount; /* Update number of channels already stopped */
				MV_REG_WRITE(INT_STATUS_MASK_REG,
					MV_REG_READ(INT_STATUS_MASK_REG) & (~(TDM_INT_TX(ch))));
			}
		}


		if (statusAndMask & RX_OVERFLOW_BIT(ch)) {
			MV_TRC_REC("Rx overflow(ch%d) - checking for root cause...\n", ch);
			if (tdmEnable) {
				MV_TRC_REC("Rx overflow ERROR\n");
				tdmIntInfo->intType |= MV_RX_ERROR_INT;
				if (!(statusAndMask & RX_BIT(ch))) {
					ret = -1;
					/* MV_OK -> Rx is done for both channels */
					if (mvTdmChRxLow(ch) == MV_OK) {
						MV_TRC_REC("Assign Rx aggregate buffer for further processing\n");
						tdmIntInfo->tdmRxBuff = rxAggrBuffVirt;
						tdmIntInfo->intType |= MV_RX_INT;
					}
				}
			} else {
				MV_TRC_REC("Expected Rx overflow(not an error)\n");
				tdmIntInfo->intType |= MV_CHAN_STOP_INT;
				tdmIntInfo->data = ++chanStopCount; /* Update number of channels already stopped */
				MV_REG_WRITE(INT_STATUS_MASK_REG,
					     MV_REG_READ(INT_STATUS_MASK_REG) & (~(TDM_INT_RX(ch))));
			}
		}
	}

	/* clear TDM interrupts */
	MV_REG_WRITE(INT_STATUS_REG, ~statusReg);

	/* Check if interrupt was missed -> restart */
	if  (intTxMiss != -1)  {
		MV_TRC_REC("Missing Tx Interrupt Detected ch%d!!!\n", intTxMiss);
		if (intTxMiss)
			intTx1Count = intTx0Count;
		else
			intTx0Count  = (intTx1Count + 1);
		ret = -1;
	}

	if  (intRxMiss != -1)  {
		MV_TRC_REC("Missing Rx Interrupt Detected ch%d!!!\n", intRxMiss);
		if (intRxMiss)
			intRx1Count = intRx0Count;
		else
			intRx0Count  = (intRx1Count + 1);
		ret = -1;
	}

	if (ret == -1) {
		intLock = 1;
		pcmRestartCount++;
	}

	MV_TRC_REC("<-%s\n", __func__);
	return ret;
}

static INLINE MV_STATUS mvTdmChTxLow(MV_U8 ch)
{
	MV_U32 max_poll = 0;
	MV_TDM_CH_INFO *chInfo = tdmChInfo[ch];

	MV_TRC_REC("->%s ch%d\n", __func__, ch);

	/* count tx interrupts */
	txInt++;
	MV_TRC_REC("txInt(%d)\n", txInt);

	if (chInfo->txBuffFull[chInfo->txCurrBuff] == BUFF_IS_FULL)
		MV_TRC_REC("curr buff full for hw [MMP ok]\n");
	else
		MV_TRC_REC("curr buf is empty [MMP miss write]\n");

	/* Change buffers */
	chInfo->txCurrBuff = MV_TDM_NEXT_BUFFER(chInfo->txCurrBuff);

	/* Mark next buff to be transmitted by HW as empty. Give it to the HW
	   for next frame. The app need to write the data before HW takes it.  */
	chInfo->txBuffFull[chInfo->txCurrBuff] = BUFF_IS_EMPTY;
	MV_TRC_REC("->%s clear buf(%d) for channel(%d)\n", __func__, chInfo->txCurrBuff, ch);

	/* Poll on SW ownership (single check) */
	MV_TRC_REC("start poll for SW ownership\n");
	while (((MV_REG_BYTE_READ(CH_BUFF_OWN_REG(chInfo->ch) + TX_OWN_BYTE_OFFS) & OWNER_MASK) == OWN_BY_HW)
	       && (max_poll < 2000)) {
		mvOsUDelay(1);
		max_poll++;
	}
	if (max_poll == 2000) {
		MV_TRC_REC("poll timeout (~2ms)\n");
		return MV_TIMEOUT;
	} else {
		MV_TRC_REC("tx-low poll stop ok\n");
	}
	MV_TRC_REC("ch%d, start tx buff %d\n", ch, chInfo->txCurrBuff);

	/*Set TX buff address (must be 32 byte aligned) */
	MV_REG_WRITE(CH_TX_ADDR_REG(chInfo->ch), chInfo->txBuffPhys[chInfo->txCurrBuff]);

	/* Set HW ownership */
	MV_REG_BYTE_WRITE(CH_BUFF_OWN_REG(chInfo->ch) + TX_OWN_BYTE_OFFS, OWN_BY_HW);

	/* Enable Tx */
	MV_REG_BYTE_WRITE(CH_ENABLE_REG(chInfo->ch) + TX_ENABLE_BYTE_OFFS, CH_ENABLE);

	MV_TRC_REC("<-%s\n", __func__);

	/* Did we get the required amount of irqs for Tx wakeup ? */
	if (txInt < MV_TDM_INT_COUNTER) {
		return MV_NOT_READY;
	} else {
		txInt = 0;
		txEmpty = chInfo->txCurrBuff;
		return MV_OK;
	}
}

static INLINE MV_STATUS mvTdmChRxLow(MV_U8 ch)
{
	MV_U32 max_poll = 0;
	MV_TDM_CH_INFO *chInfo = tdmChInfo[ch];

	MV_TRC_REC("->%s ch%d\n", __func__, ch);

	if (chInfo->rxFirst)
		chInfo->rxFirst = !FIRST_INT;
	else
		rxInt++;

	MV_TRC_REC("rxInt(%d)\n", rxInt);

	if (chInfo->rxBuffFull[chInfo->rxCurrBuff] == BUFF_IS_EMPTY)
		MV_TRC_REC("curr buff empty for hw [MMP ok]\n");
	else
		MV_TRC_REC("curr buf is full [MMP miss read]\n");

	/* Mark last buff that was received by HW as full. Give next buff to HW for */
	/* next frame. The app need to read the data before next irq */
	chInfo->rxBuffFull[chInfo->rxCurrBuff] = BUFF_IS_FULL;

	/* Change buffers */
	chInfo->rxCurrBuff = MV_TDM_NEXT_BUFFER(chInfo->rxCurrBuff);

	/* Poll on SW ownership (single check) */
	MV_TRC_REC("start poll for ownership\n");
	while (((MV_REG_BYTE_READ(CH_BUFF_OWN_REG(chInfo->ch) + RX_OWN_BYTE_OFFS) & OWNER_MASK) == OWN_BY_HW)
	       && (max_poll < 2000)) {
		mvOsUDelay(1);
		max_poll++;
	}
	if (max_poll == 2000) {
		MV_TRC_REC("poll timeout (~2ms)\n");
		return MV_TIMEOUT;
	} else {
		MV_TRC_REC("poll stop ok\n");
	}
	MV_TRC_REC("%s ch%d, start rx buff %d\n", __func__, ch, chInfo->rxCurrBuff);

	/* Set RX buff address (must be 32 byte aligned) */
	MV_REG_WRITE(CH_RX_ADDR_REG(chInfo->ch), chInfo->rxBuffPhys[chInfo->rxCurrBuff]);

	/* Set HW ownership */
	MV_REG_BYTE_WRITE(CH_BUFF_OWN_REG(chInfo->ch) + RX_OWN_BYTE_OFFS, OWN_BY_HW);

	/* Enable Rx */
	MV_REG_BYTE_WRITE(CH_ENABLE_REG(chInfo->ch) + RX_ENABLE_BYTE_OFFS, CH_ENABLE);

	MV_TRC_REC("<-%s\n", __func__);

	/* Did we get the required amount of irqs for Rx wakeup ? */
	if (rxInt < MV_TDM_INT_COUNTER) {
		return MV_NOT_READY;
	} else {
		rxInt = 0;
		rxFull = MV_TDM_PREV_BUFFER(chInfo->rxCurrBuff, 2);
		MV_TRC_REC("buff %d is FULL for ch0\n", rxFull);
		MV_TRC_REC("buff %d is FULL for ch1\n", rxFull);
		return MV_OK;
	}
}

/****************************
**        SPI Stuff        **
****************************/

static MV_VOID mvTdmSetCurrentUnit(MV_32 cs)
{
	if (!spiMode) {
		if (!cs)
			MV_REG_WRITE(PCM_CTRL_REG, (MV_REG_READ(PCM_CTRL_REG) & ~CS_CTRL));
		else
			MV_REG_WRITE(PCM_CTRL_REG, (MV_REG_READ(PCM_CTRL_REG) | CS_CTRL));
	} else
		MV_REG_WRITE(PCM_CTRL_REG, (MV_REG_READ(PCM_CTRL_REG) & ~CS_CTRL));
}

static MV_VOID mvTdmDaisyChainModeSet(MV_VOID)
{
	mvOsPrintf("Setting Daisy Chain Mode\n");
	while ((MV_REG_READ(SPI_CTRL_REG) & SPI_STAT_MASK) == SPI_ACTIVE)
		continue;
	MV_REG_WRITE(SPI_CODEC_CMD_LO_REG, (0x80 << 8) | 0);
	MV_REG_WRITE(SPI_CODEC_CTRL_REG, TRANSFER_BYTES(2) | ENDIANESS_MSB_MODE | WR_MODE | CLK_SPEED_LO_DIV);
	MV_REG_WRITE(SPI_CTRL_REG, MV_REG_READ(SPI_CTRL_REG) | SPI_ACTIVE);
	/* Poll for ready indication */
	while ((MV_REG_READ(SPI_CTRL_REG) & SPI_STAT_MASK) == SPI_ACTIVE)
		continue;
}

MV_STATUS mvTdmSpiWrite(MV_U8 *cmdBuff, MV_U8 cmdSize, MV_U8 *dataBuff, MV_U8 dataSize, MV_U8 cs)
{
	MV_U32 i, val1 = 0, val2 = 0, cmd;

	/*MV_TRC_REC("%s: cs = %d val1 = 0x%x val2 = 0x%x\n",__func__,cs, val1, val2); */

	/* Poll for ready indication */
	while ((MV_REG_READ(SPI_CTRL_REG) & SPI_STAT_MASK) == SPI_ACTIVE)
		continue;

	if (cmdSize > 0)
		val1 = (MV_U32) (cmdBuff[0] & 0xff);

	if (cmdSize > 1)
		val1 |= (MV_U32) (cmdBuff[1] << 8);

	if (cmdSize > 2)
		val2 = (MV_U32) (cmdBuff[2] & 0xff);

	mvTdmSetCurrentUnit(cs);

	/* Prepare codec control parameters for command transmission */
	cmd = TRANSFER_BYTES(cmdSize) | ENDIANESS_MSB_MODE | WR_MODE | CLK_SPEED_LO_DIV;

	MV_REG_WRITE(SPI_CODEC_CMD_LO_REG, val1);
	MV_REG_WRITE(SPI_CODEC_CMD_HI_REG, val2);
	MV_REG_WRITE(SPI_CODEC_CTRL_REG, cmd);

	/* Activate */
	MV_REG_WRITE(SPI_CTRL_REG, MV_REG_READ(SPI_CTRL_REG) | SPI_ACTIVE);

	/* Poll for ready indication */
	while ((MV_REG_READ(SPI_CTRL_REG) & SPI_STAT_MASK) == SPI_ACTIVE)
		continue;

	/* Update the command to 1 data byte transfer */
	cmd = TRANSFER_BYTES(1) | ENDIANESS_MSB_MODE | WR_MODE | CLK_SPEED_LO_DIV;

	for (i = 0; i < dataSize; i++) {
		val1 = dataBuff[i];

		mvTdmSetCurrentUnit(cs);

		MV_REG_WRITE(SPI_CODEC_CMD_LO_REG, val1);
		MV_REG_WRITE(SPI_CODEC_CTRL_REG, cmd);

		/* Activate */
		MV_REG_WRITE(SPI_CTRL_REG, MV_REG_READ(SPI_CTRL_REG) | SPI_ACTIVE);

		/* Poll for ready indication */
		while ((MV_REG_READ(SPI_CTRL_REG) & SPI_STAT_MASK) == SPI_ACTIVE)
			continue;
	}

	return MV_OK;
}

MV_STATUS mvTdmSpiRead(MV_U8 *cmdBuff, MV_U8 cmdSize, MV_U8 *dataBuff, MV_U8 dataSize, MV_U8 cs)
{
	MV_U32 val1 = 0, val2 = 0, cmd;
	MV_U32 data;

	/*MV_TRC_REC("%s: cs = %d val1 = 0x%x val2 = 0x%x\n",__func__,cs, val1, val2); */

	if (dataSize > 2) {
		mvOsPrintf("%s: Error, exceeded max read size(%d)\n", __func__, dataSize);
		return MV_ERROR;
	}

	/* Poll for ready indication */
	while ((MV_REG_READ(SPI_CTRL_REG) & SPI_STAT_MASK) == SPI_ACTIVE)
		continue;

	val1 = (MV_U32) (cmdBuff[0] & 0xff);

	if (cmdSize > 1)
		val1 |= (MV_U32) (cmdBuff[1] << 8);

	if (cmdSize > 2)
		val2 = (MV_U32) (cmdBuff[2] & 0xff);

	if (cmdSize > 3)
		val2 = (MV_U32) (cmdBuff[3] << 8);

	mvTdmSetCurrentUnit(cs);

	/* Prepare codec control parameters for command transmission */
	if (dataSize == 1)
		cmd = TRANSFER_BYTES(cmdSize) | ENDIANESS_MSB_MODE | RD_MODE | READ_1_BYTE | CLK_SPEED_LO_DIV;
	else
		cmd = TRANSFER_BYTES(cmdSize) | ENDIANESS_MSB_MODE | RD_MODE | READ_2_BYTE | CLK_SPEED_LO_DIV;

	MV_REG_WRITE(SPI_CODEC_CMD_LO_REG, val1);
	MV_REG_WRITE(SPI_CODEC_CMD_HI_REG, val2);
	MV_REG_WRITE(SPI_CODEC_CTRL_REG, cmd);

	/* Activate */
	MV_REG_WRITE(SPI_CTRL_REG, MV_REG_READ(SPI_CTRL_REG) | SPI_ACTIVE);

	/* Poll for ready indication */
	while ((MV_REG_READ(SPI_CTRL_REG) & SPI_STAT_MASK) == SPI_ACTIVE)
		continue;

	/* Read the data received from codec */
	data = MV_REG_READ(SPI_CODEC_READ_DATA_REG);
	dataBuff[dataSize - 1] = (data & 0xff);

	if (dataSize == 2)
		dataBuff[0] = ((data & 0xff00) >> 8);

	return MV_OK;
}

/******************
** Debug Display **
******************/
MV_VOID mvOsRegDump(MV_U32 reg)
{
	mvOsPrintf("0x%05x: %08x\n", reg, MV_REG_READ(reg));
}

MV_VOID mvTdmRegsDump(MV_VOID)
{
	MV_U8 i;
	MV_TDM_CH_INFO *chInfo;

	mvOsPrintf("TDM Control:\n");
	mvOsRegDump(TDM_SPI_MUX_REG);
	mvOsRegDump(INT_RESET_SELECT_REG);
	mvOsRegDump(INT_STATUS_MASK_REG);
	mvOsRegDump(INT_STATUS_REG);
	mvOsRegDump(INT_EVENT_MASK_REG);
	mvOsRegDump(PCM_CTRL_REG);
	mvOsRegDump(TIMESLOT_CTRL_REG);
	mvOsRegDump(PCM_CLK_RATE_DIV_REG);
	mvOsRegDump(FRAME_TIMESLOT_REG);
	mvOsRegDump(DUMMY_RX_WRITE_DATA_REG);
	mvOsRegDump(MISC_CTRL_REG);
	mvOsPrintf("TDM Channel Control:\n");
	for (i = 0; i < MV_TDM2C_TOTAL_CHANNELS; i++) {
		mvOsRegDump(CH_DELAY_CTRL_REG(i));
		mvOsRegDump(CH_SAMPLE_REG(i));
		mvOsRegDump(CH_DBG_REG(i));
		mvOsRegDump(CH_TX_CUR_ADDR_REG(i));
		mvOsRegDump(CH_RX_CUR_ADDR_REG(i));
		mvOsRegDump(CH_ENABLE_REG(i));
		mvOsRegDump(CH_BUFF_OWN_REG(i));
		mvOsRegDump(CH_TX_ADDR_REG(i));
		mvOsRegDump(CH_RX_ADDR_REG(i));
	}
	mvOsPrintf("TDM interrupts:\n");
	mvOsRegDump(INT_EVENT_MASK_REG);
	mvOsRegDump(INT_STATUS_MASK_REG);
	mvOsRegDump(INT_STATUS_REG);
	for (i = 0; i < MV_TDM2C_TOTAL_CHANNELS; i++) {
		mvOsPrintf("ch%d info:\n", i);
		chInfo = tdmChInfo[i];
		mvOsPrintf("RX buffs:\n");
		mvOsPrintf("buff0: virt=%p phys=%p\n", chInfo->rxBuffVirt[0], (MV_U32 *) (chInfo->rxBuffPhys[0]));
		mvOsPrintf("buff1: virt=%p phys=%p\n", chInfo->rxBuffVirt[1], (MV_U32 *) (chInfo->rxBuffPhys[1]));
		mvOsPrintf("TX buffs:\n");
		mvOsPrintf("buff0: virt=%p phys=%p\n", chInfo->txBuffVirt[0], (MV_U32 *) (chInfo->txBuffPhys[0]));
		mvOsPrintf("buff1: virt=%p phys=%p\n", chInfo->txBuffVirt[1], (MV_U32 *) (chInfo->txBuffPhys[1]));
	}
}

static MV_VOID mvTdmShowProperties(MV_VOID)
{
	mvOsPrintf("TDM dual channel device rev 0x%x\n", MV_REG_READ(TDM_REV_REG));
}

MV_U8 currRxSampleGet(MV_U8 ch)
{
	return (MV_REG_BYTE_READ(CH_DBG_REG(ch) + 1));
}

MV_U8 currTxSampleGet(MV_U8 ch)
{
	return (MV_REG_BYTE_READ(CH_DBG_REG(ch) + 3));
}

MV_VOID mvTdmIntEnable(MV_VOID)
{
	MV_REG_WRITE(INT_STATUS_MASK_REG, (MV_REG_READ(INT_STATUS_MASK_REG) | TDM_INT_SLIC));
}

MV_VOID mvTdmIntDisable(MV_VOID)
{
	MV_U32 val = ~TDM_INT_SLIC;

	MV_TRC_REC("->%s\n", __func__);

	MV_REG_WRITE(INT_STATUS_MASK_REG, (MV_REG_READ(INT_STATUS_MASK_REG) & val));

	MV_TRC_REC("<-%s\n", __func__);
}

MV_VOID mvTdmPcmIfReset(MV_VOID)
{
	MV_TRC_REC("->%s\n", __func__);

	MV_REG_BIT_RESET(PCM_CTRL_REG, BIT0);

	/* Wait a bit - might be fine tuned */
	mvOsDelay(5);

	MV_REG_BIT_SET(TDM_SPI_MUX_REG, BIT0);	/* Disable TDM/SPI interface */

	MV_REG_WRITE(TDM_MISC_REG, 0);		/* SW PCM reset */

	/* Wait a bit more - might be fine tuned */
	mvOsDelay(100);

	MV_TRC_REC("<-%s\n", __func__);
}

#ifdef MV_TDM_EXT_STATS
MV_VOID mvTdmExtStatsGet(MV_TDM_EXTENDED_STATS *tdmExtStats)
{
	tdmExtStats->intRxCount = intRxCount;
	tdmExtStats->intTxCount = intTxCount;
	tdmExtStats->intRx0Count = intRx0Count;
	tdmExtStats->intTx0Count = intTx0Count;
	tdmExtStats->intRx1Count = intRx1Count;
	tdmExtStats->intTx1Count = intTx1Count;
	tdmExtStats->intRx0Miss = intRx0Miss;
	tdmExtStats->intTx0Miss = intTx0Miss;
	tdmExtStats->intRx1Miss = intRx1Miss;
	tdmExtStats->intTx1Miss = intTx1Miss;
	tdmExtStats->pcmRestartCount = pcmRestartCount;

	return;
}
#endif
