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

#ifndef __INCmvTdmh
#define __INCmvTdmh

#include "voiceband/tdm/mvTdmRegs.h"
#include "voiceband/mvSysTdmSpi.h"
#include "voiceband/common/mvTdmComm.h"
#include "mvSysTdmConfig.h"

/* Defines */
#define SAMPLES_BUFF_SIZE(bandMode, factor)  \
	 ((bandMode == MV_NARROW_BAND) ? (factor * 80) : (factor * 160))

#define MV_TDM_CH_BUFF_SIZE(pcmFormat, bandMode, factor) \
	(pcmFormat == MV_PCM_FORMAT_2BYTES ? (2 * SAMPLES_BUFF_SIZE(bandMode, factor)) : \
						  SAMPLES_BUFF_SIZE(bandMode, factor))

#define MV_TDM_AGGR_BUFF_SIZE(pcmFormat, bandMode, factor)	(2 * MV_TDM_CH_BUFF_SIZE(pcmFormat, bandMode, factor))
#define MV_TDM2C_TOTAL_CHANNELS			2
#define MV_TDM_INT_COUNTER				2
#define MV_TDM_MAX_SAMPLING_PERIOD		30	/* ms */
#define MV_TDM_BASE_SAMPLING_PERIOD		10	/* ms */
#define MV_TDM_TOTAL_CH_SAMPLES			80	/* samples */

/* TDM IRQ types */
#define MV_EMPTY_INT		0
#define MV_RX_INT			BIT0
#define	MV_TX_INT			BIT1
#define	MV_PHONE_INT 		BIT2
#define	MV_RX_ERROR_INT 	BIT3
#define	MV_TX_ERROR_INT 	BIT4
#define MV_DMA_ERROR_INT	BIT5
#define MV_CHAN_STOP_INT	BIT6
#define MV_ERROR_INT		(MV_RX_ERROR_INT | MV_TX_ERROR_INT | MV_DMA_ERROR_INT)

/* PCM SLOT configuration */

#define PCM_SLOT_PCLK	8

#define TDM_INT_SLIC	(DMA_ABORT_BIT|SLIC_INT_BIT)
#define TDM_INT_TX(ch)	(TX_UNDERFLOW_BIT(ch)|TX_BIT(ch)|TX_IDLE_BIT(ch))
#define TDM_INT_RX(ch)	(RX_OVERFLOW_BIT(ch)|RX_BIT(ch)|RX_IDLE_BIT(ch))

/* TDM Registers Configuration */
#if defined(MV_TDM_USE_EXTERNAL_PCLK_SOURCE)
#define CONFIG_PCM_CRTL (MASTER_PCLK_EXTERNAL | MASTER_FS_TDM | DATA_POLAR_NEG | \
			 FS_POLAR_NEG | INVERT_FS_HI | FS_TYPE_SHORT	 | \
			 CH_DELAY_ENABLE 		 		 | \
			 CH_QUALITY_DISABLE | QUALITY_POLARITY_NEG	 | \
			 QUALITY_TYPE_TIME_SLOT | CS_CTRL_DONT_CARE 	 | \
			 WIDEBAND_OFF | PERF_GBUS_TWO_ACCESS)

#else
#define CONFIG_PCM_CRTL (MASTER_PCLK_TDM | MASTER_FS_TDM | DATA_POLAR_NEG | \
			 FS_POLAR_NEG | INVERT_FS_HI | FS_TYPE_SHORT	 | \
			 CH_DELAY_ENABLE 				 | \
			 CH_QUALITY_DISABLE | QUALITY_POLARITY_NEG	 | \
			 QUALITY_TYPE_TIME_SLOT | CS_CTRL_DONT_CARE 	 | \
			 WIDEBAND_OFF | PERF_GBUS_TWO_ACCESS)
#endif

#if defined(MV_TDM_USE_EXTERNAL_PCLK_SOURCE)
#define CONFIG_WB_PCM_CRTL (MASTER_PCLK_EXTERNAL | MASTER_FS_TDM | DATA_POLAR_NEG | \
			    FS_POLAR_NEG | INVERT_FS_HI | FS_TYPE_SHORT	 | \
			    CH_DELAY_ENABLE 				 | \
			    CH_QUALITY_DISABLE | QUALITY_POLARITY_NEG	 | \
			    QUALITY_TYPE_TIME_SLOT | CS_CTRL_DONT_CARE 	 | \
			    WIDEBAND_ON | PERF_GBUS_TWO_ACCESS)
#else
#define CONFIG_WB_PCM_CRTL (MASTER_PCLK_TDM | MASTER_FS_TDM | DATA_POLAR_NEG | \
			    FS_POLAR_NEG | INVERT_FS_HI | FS_TYPE_SHORT	 | \
			    CH_DELAY_ENABLE 				 | \
			    CH_QUALITY_DISABLE | QUALITY_POLARITY_NEG	 | \
			    QUALITY_TYPE_TIME_SLOT | CS_CTRL_DONT_CARE 	 | \
			    WIDEBAND_ON | PERF_GBUS_TWO_ACCESS)
#endif

#define CONFIG_CH_SAMPLE(bandMode, factor) ((SAMPLES_BUFF_SIZE(bandMode, factor)<<TOTAL_CNT_OFFS) |\
									 (INT_SAMPLE<<INT_CNT_OFFS))

/* Enumerators */

/* Structures */
#ifdef MV_TDM_EXT_STATS
typedef struct {
	MV_U32 intRxCount;
	MV_U32 intTxCount;
	MV_U32 intRx0Count;
	MV_U32 intTx0Count;
	MV_U32 intRx1Count;
	MV_U32 intTx1Count;
	MV_U32 intRx0Miss;
	MV_U32 intTx0Miss;
	MV_U32 intRx1Miss;
	MV_U32 intTx1Miss;
	MV_U32 pcmRestartCount;
} MV_TDM_EXTENDED_STATS;
#endif

/* APIs */
MV_STATUS mvTdmHalInit(MV_TDM_PARAMS *tdmParams, MV_TDM_HAL_DATA *halData);
MV_STATUS mvTdmWinInit(MV_UNIT_WIN_INFO *addrWinMap);
MV_VOID mvTdmRelease(MV_VOID);
MV_32 mvPcmStopIntMiss(void);
MV_32 mvTdmIntLow(MV_TDM_INT_INFO *tdmIntInfo);
MV_VOID mvTdmPcmStart(MV_VOID);
MV_VOID mvTdmPcmStop(MV_VOID);
MV_STATUS mvTdmTx(MV_U8 *tdmTxBuff);
MV_STATUS mvTdmRx(MV_U8 *tdmRxBuff);
MV_VOID mvTdmRegsDump(MV_VOID);
MV_STATUS mvTdmSpiRead(MV_U8 *cmdBuff, MV_U8 cmdSize, MV_U8 *dataBuff, MV_U8 dataSize, MV_U8 cs);
MV_STATUS mvTdmSpiWrite(MV_U8 *cmdBuff, MV_U8 cmdSize, MV_U8 *dataBuff, MV_U8 dataSize, MV_U8 cs);
MV_U8 currRxSampleGet(MV_U8 ch);
MV_U8 currTxSampleGet(MV_U8 ch);
MV_VOID mvTdmIntEnable(MV_VOID);
MV_VOID mvTdmIntDisable(MV_VOID);
MV_VOID mvTdmPcmIfReset(MV_VOID);
#ifdef MV_TDM_EXT_STATS
MV_VOID mvTdmExtStatsGet(MV_TDM_EXTENDED_STATS *tdmExtStats);
#endif

#endif /* __INCmvTdmh */
