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

#ifndef __INCmvCommUnith
#define __INCmvCommUnith

#ifdef __cplusplus
extern "C" {
#endif

#include "voiceband/commUnit/mvCommUnitRegs.h"
#include "voiceband/mvSysTdmSpi.h"
#include "voiceband/common/mvTdmComm.h"
#include "mvSysTdmConfig.h"

/* Defines */
#define MV_TDMMC_TOTAL_CHANNELS			32
#define MV_TDM_MAX_HALF_DPRAM_ENTRIES		128
#define MV_TDM_MAX_SAMPLING_PERIOD		30	/* ms */
#define MV_TDM_BASE_SAMPLING_PERIOD		10	/* ms */
#define MV_TDM_TOTAL_CH_SAMPLES			80	/* samples */

typedef enum {
	MV_COMMUNIT_IP_VER_ORIGIN   = 0,
	MV_COMMUNIT_IP_VER_REVISE_1,
} MV_COMMUNIT_IP_VERSION_T;

/* IRQ types */
#define MV_EMPTY_INT			0
#define MV_RX_INT			BIT0
#define	MV_TX_INT			BIT1
#define	MV_PHONE_INT			BIT2
#define	MV_RX_ERROR_INT			BIT3
#define	MV_TX_ERROR_INT			BIT4
#define MV_DMA_ERROR_INT		BIT5
#define MV_ERROR_INT			(MV_RX_ERROR_INT | MV_TX_ERROR_INT | MV_DMA_ERROR_INT)

#define TDM_SLIC_INT		(EXT_INT_SLIC0_MASK | EXT_INT_SLIC1_MASK | EXT_INT_SLIC2_MASK | EXT_INT_SLIC3_MASK)
#define TDM_TX_INT		 TX_VOICE_INT_PULSE_MASK
#define TDM_RX_INT		 RX_VOICE_INT_PULSE_MASK
#define TDM_ERROR_INT		(FLEX_TDM_RX_SYNC_LOSS_MASK | FLEX_TDM_TX_SYNC_LOSS_MASK | \
				 COMM_UNIT_PAR_ERR_SUM_MASK | TDM_RX_PAR_ERR_SUM_MASK | TDM_TX_PAR_ERR_SUM_MASK | \
				 MCSC_PAR_ERR_SUM_MASK | MCDMA_PAR_ERR_SUM_MASK)

/* MCDMA Descriptor Command/Status Bits */
#define	LAST_BIT	BIT16
#define	FIRST_BIT	BIT17
#define	EOPI		BIT21
#define	ENABLE_INT	BIT23
#define	AUTO_MODE	BIT30
#define	OWNER		BIT31

/* MCDMA */
#define CONFIG_MCDMA_DESC_CMD_STATUS	(FIRST_BIT | AUTO_MODE | OWNER)
#define CONFIG_RMCCx			(MCDMA_RBSZ_16BYTE | MCDMA_BLMR_MASK)
#define CONFIG_TMCCx			(MCDMA_FSIZE_1BLK | MCDMA_TBSZ_16BYTE | MCDMA_BLMT_MASK)

/* MCSC */
#define CONFIG_MRCRx			(MRCRx_RRVD_MASK | MRCRx_MODE_MASK)
#define CONFIG_MTCRx			(MTCRx_TRVD_MASK | MTCRx_MODE_MASK)
#define CONFIG_LINEAR_BYTE_SWAP		(MCSC_GLOBAL_CONFIG_LINEAR_TX_SWAP_MASK | \
					 MCSC_GLOBAL_CONFIG_LINEAR_RX_SWAP_MASK)

/* TDM */
#if defined(MV_TDM_USE_EXTERNAL_PCLK_SOURCE)
#define CONFIG_TDM_CLK_AND_SYNC_CONTROL	(TDM_TX_CLK_OUT_ENABLE_MASK | TDM_RX_CLK_OUT_ENABLE_MASK | \
					 TDM_REFCLK_DIVIDER_BYPASS_MASK)
#else
#define CONFIG_TDM_CLK_AND_SYNC_CONTROL	(TDM_REFCLK_DIVIDER_BYPASS_MASK | TDM_OUT_CLK_SRC_CTRL_AFTER_DIV)
#endif				/* MV_TDM_USE_EXTERNAL_PCLK_SOURCE */

#define CONFIG_VOICE_PERIODICAL_INT_CONTROL (((MV_TDM_TOTAL_CH_SAMPLES) << RX_VOICE_INT_CNT_REF_OFFS) | \
					     ((MV_TDM_TOTAL_CH_SAMPLES) << TX_VOICE_INT_CNT_REF_OFFS) | \
					     (1 << RX_FIRST_DELAY_REF_OFFS) | (4 << TX_FIRST_DELAY_REF_OFFS))
#define CONFIG_VOICE_PERIODICAL_INT_CONTROL_WA (((MV_TDM_TOTAL_CH_SAMPLES - 1) << RX_VOICE_INT_CNT_REF_OFFS) | \
					     ((MV_TDM_TOTAL_CH_SAMPLES - 1) << TX_VOICE_INT_CNT_REF_OFFS) | \
					     (1 << RX_FIRST_DELAY_REF_OFFS) | (4 << TX_FIRST_DELAY_REF_OFFS))
#define CONFIG_TDM_CAUSE    		    (TDM_RX_INT | TDM_TX_INT /*| TDM_ERROR_INT*/)
#define CONFIG_COMM_UNIT_TOP_MASK	    (TDM_SUM_INT_MASK | MCSC_SUM_INT_MASK)
#define CONFIG_FLEX_TDM_CONFIG		    (TDM_SE_MASK | TDM_COMMON_RX_TX_MASK | TSD_NO_DELAY | RSD_NO_DELAY)
#define	CONFIG_TDM_DATA_DELAY_AND_CLK_CTRL		(TX_CLK_OUT_ENABLE_MASK | RX_CLK_OUT_ENABLE_MASK)
#define CONFIG_TDM_PLUS_MINUS_DELAY_CTRL_FSYNC_OUT	(TX_SYNC_DELAY_OUT_MINUS | RX_SYNC_DELAY_OUT_MINUS)
#define CONFIG_TDM_PLUS_MINUS_DELAY_CTRL_FSYNC_IN	(TX_SYNC_DELAY_IN_MINUS | RX_SYNC_DELAY_IN_MINUS)

/* Structures */
typedef struct {
	MV_U32 cmdStatus;
	MV_U16 byteCnt;
	MV_U16 buffSize;
	MV_U32 physBuffPtr;
	MV_U32 physNextDescPtr;
} MV_TDM_MCDMA_RX_DESC;

typedef struct {
	MV_U32 cmdStatus;
	MV_U16 shadowByteCnt;
	MV_U16 byteCnt;
	MV_U32 physBuffPtr;
	MV_U32 physNextDescPtr;
} MV_TDM_MCDMA_TX_DESC;

typedef struct {
	MV_U32 mask:8;
	MV_U32 ch:8;
	MV_U32 mgs:2;
	MV_U32 byte:1;
	MV_U32 strb:2;
	MV_U32 elpb:1;
	MV_U32 tbs:1;
	MV_U32 rpt:2;
	MV_U32 last:1;
	MV_U32 ftint:1;
	MV_U32 reserved31_27:5;
} MV_TDM_DPRAM_ENTRY;

/* CommUnit APIs */
	MV_STATUS mvCommUnitHalInit(MV_TDM_PARAMS *pTdmParams, MV_TDM_HAL_DATA *halData);
	MV_STATUS mvCommUnitWinInit(MV_UNIT_WIN_INFO *pAddrWinMap);
	MV_32 mvCommUnitIntLow(MV_TDM_INT_INFO *pTdmIntInfo);
	MV_VOID mvCommUnitPcmStart(MV_VOID);
	MV_VOID mvCommUnitPcmStop(MV_VOID);
	MV_STATUS mvCommUnitTx(MV_U8 *pTdmTxBuff);
	MV_STATUS mvCommUnitRx(MV_U8 *pTdmRxBuff);
	MV_VOID mvCommUnitShow(MV_VOID);
	MV_VOID mvCommUnitRelease(MV_VOID);
	MV_VOID mvCommUnitIntEnable(MV_U8 deviceId);
	MV_VOID mvCommUnitIntDisable(MV_U8 deviceId);
	MV_STATUS mvCommUnitResetSlic(MV_VOID);

#ifdef __cplusplus
}
#endif
#endif				/* __INCmvCommUnith */
