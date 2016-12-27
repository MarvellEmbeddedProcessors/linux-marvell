/*******************************************************************************
 * Copyright (C) 2016 Marvell International Ltd.
 *
 * This software file (the "File") is owned and distributed by Marvell
 * International Ltd. and/or its affiliates ("Marvell") under the following
 * alternative licensing terms.  Once you have made an election to distribute the
 * File under one of the following license alternatives, please (i) delete this
 * introductory statement regarding license alternatives, (ii) delete the three
 * license alternatives that you have not elected to use and (iii) preserve the
 * Marvell copyright notice above.
 *
 * ********************************************************************************
 * Marvell Commercial License Option
 *
 * If you received this File from Marvell and you have entered into a commercial
 * license agreement (a "Commercial License") with Marvell, the File is licensed
 * to you under the terms of the applicable Commercial License.
 *
 * ********************************************************************************
 * Marvell GPL License Option
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the Free
 * Software Foundation, either version 2 of the License, or any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * ********************************************************************************
 * Marvell GNU General Public License FreeRTOS Exception
 *
 * If you received this File from Marvell, you may opt to use, redistribute and/or
 * modify this File in accordance with the terms and conditions of the Lesser
 * General Public License Version 2.1 plus the following FreeRTOS exception.
 * An independent module is a module which is not derived from or based on
 * FreeRTOS.
 * Clause 1:
 * Linking FreeRTOS statically or dynamically with other modules is making a
 * combined work based on FreeRTOS. Thus, the terms and conditions of the GNU
 * General Public License cover the whole combination.
 * As a special exception, the copyright holder of FreeRTOS gives you permission
 * to link FreeRTOS with independent modules that communicate with FreeRTOS solely
 * through the FreeRTOS API interface, regardless of the license terms of these
 * independent modules, and to copy and distribute the resulting combined work
 * under terms of your choice, provided that:
 * 1. Every copy of the combined work is accompanied by a written statement that
 * details to the recipient the version of FreeRTOS used and an offer by yourself
 * to provide the FreeRTOS source code (including any modifications you may have
 * made) should the recipient request it.
 * 2. The combined work is not itself an RTOS, scheduler, kernel or related
 * product.
 * 3. The independent modules add significant and primary functionality to
 * FreeRTOS and do not merely extend the existing functionality already present in
 * FreeRTOS.
 * Clause 2:
 * FreeRTOS may not be used for any competitive or comparative purpose, including
 * the publication of any form of run time or compile time metric, without the
 * express permission of Real Time Engineers Ltd. (this is the norm within the
 * industry and is intended to ensure information accuracy).
 *
 * ********************************************************************************
 * Marvell BSD License Option
 *
 * If you received this File from Marvell, you may opt to use, redistribute and/or
 * modify this File under the following licensing terms.
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted provided that the following conditions are met:
 *
 *	* Redistributions of source code must retain the above copyright notice,
 *	  this list of conditions and the following disclaimer.
 *
 *	* Redistributions in binary form must reproduce the above copyright
 *	  notice, this list of conditions and the following disclaimer in the
 *	  documentation and/or other materials provided with the distribution.
 *
 *	* Neither the name of Marvell nor the names of its contributors may be
 *	  used to endorse or promote products derived from this software without
 *	  specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
 * ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef _TDMMC_H_
#define _TDMMC_H_

/****************************************************************/
/*	Time Division Multiplexing Interrupt Controller		*/
/****************************************************************/
#define COMM_UNIT_TOP_CAUSE_REG			0x8C00
#define TDM_CAUSE_REG				0x8C40
#define COMM_UNIT_TOP_MASK_REG			0x8C80
#define VOICE_PERIODICAL_INT_CONTROL_REG	0x8C90
#define TDM_MASK_REG				0x8CC0

/* COMM_UNIT_TOP_CAUSE_REG bits */
#define TDM_SUM_INT_OFFS			6
#define TDM_SUM_INT_MASK			(1 << TDM_SUM_INT_OFFS)
#define MCSC_SUM_INT_OFFS			28
#define MCSC_SUM_INT_MASK			(1 << MCSC_SUM_INT_OFFS)

/* TDM_CAUSE_REG bits */
#define FLEX_TDM_RX_SYNC_LOSS_OFFS		3
#define FLEX_TDM_RX_SYNC_LOSS_MASK		(1 << FLEX_TDM_RX_SYNC_LOSS_OFFS)
#define FLEX_TDM_TX_SYNC_LOSS_OFFS		7
#define FLEX_TDM_TX_SYNC_LOSS_MASK		(1 << FLEX_TDM_TX_SYNC_LOSS_OFFS)
#define RX_VOICE_INT_PULSE_OFFS			8
#define RX_VOICE_INT_PULSE_MASK			(1 << RX_VOICE_INT_PULSE_OFFS)
#define TX_VOICE_INT_PULSE_OFFS			9
#define TX_VOICE_INT_PULSE_MASK			(1 << TX_VOICE_INT_PULSE_OFFS)
#define COMM_UNIT_PAR_ERR_SUM_OFFS		18
#define COMM_UNIT_PAR_ERR_SUM_MASK		(1 << COMM_UNIT_PAR_ERR_SUM_OFFS)
#define TDM_RX_PAR_ERR_SUM_OFFS			19
#define TDM_RX_PAR_ERR_SUM_MASK			(1 << TDM_RX_PAR_ERR_SUM_OFFS)
#define TDM_TX_PAR_ERR_SUM_OFFS			20
#define TDM_TX_PAR_ERR_SUM_MASK			(1 << TDM_TX_PAR_ERR_SUM_OFFS)
#define MCSC_PAR_ERR_SUM_OFFS			21
#define MCSC_PAR_ERR_SUM_MASK			(1 << MCSC_PAR_ERR_SUM_OFFS)
#define MCDMA_PAR_ERR_SUM_OFFS			22
#define MCDMA_PAR_ERR_SUM_MASK			(1 << MCDMA_PAR_ERR_SUM_OFFS)

/*  VOICE_PERIODICAL_INT_CONTROL_REG bits  */
#define RX_VOICE_INT_CNT_REF_OFFS		0
#define RX_VOICE_INT_CNT_REF_MASK		(0xff << RX_VOICE_INT_CNT_REF_OFFS)
#define TX_VOICE_INT_CNT_REF_OFFS		8
#define TX_VOICE_INT_CNT_REF_MASK		(0xff << TX_VOICE_INT_CNT_REF_OFFS)
#define RX_FIRST_DELAY_REF_OFFS			16
#define RX_FIRST_DELAY_REF_MASK			(0xff << RX_FIRST_DELAY_REF_OFFS)
#define TX_FIRST_DELAY_REF_OFFS			24
#define TX_FIRST_DELAY_REF_MASK			(0xff << TX_FIRST_DELAY_REF_OFFS)

/* Multi-Channel Serial Controller (MCSC) */
#define MCSC_CHx_RECEIVE_CONFIG_REG(ch)		(0x400 + (ch << 2))
#define MCSC_CHx_TRANSMIT_CONFIG_REG(ch)	(0x1800 + (ch<<2))
#define MCSC_GLOBAL_CONFIG_REG			0x2800
#define MCSC_GLOBAL_INT_CAUSE_REG		0x2804
#define MCSC_EXTENDED_INT_CAUSE_REG		0x2808
#define MCSC_GLOBAL_INT_MASK_REG		0x280C
#define MCSC_EXTENDED_INT_MASK_REG		0x2810
#define MCSC_GLOBAL_CONFIG_EXTENDED_REG		0x2890

/* MCSC_RECEIVE_CONFIG_REG(MRCRx) bits */
#define MRCRx_ER_OFFS				27
#define MRCRx_ER_MASK				(1 << MRCRx_ER_OFFS)
#define MRCRx_RRVD_OFFS				30
#define MRCRx_RRVD_MASK				(1 << MRCRx_RRVD_OFFS)
#define MRCRx_MODE_OFFS				31
#define MRCRx_MODE_MASK				(1 << MRCRx_MODE_OFFS)

/* MCSC_TRANSMIT_CONFIG_REG(MTCRx) bits */
#define MTCRx_ET_OFFS				27
#define MTCRx_ET_MASK				(1 << MTCRx_ET_OFFS)
#define MTCRx_TRVD_OFFS				30
#define MTCRx_TRVD_MASK				(1 << MTCRx_TRVD_OFFS)
#define MTCRx_MODE_OFFS				31
#define MTCRx_MODE_MASK				(1 << MTCRx_MODE_OFFS)

/* MCSC_GLOBAL_CONFIG_REG bits */
#define MCSC_GLOBAL_CONFIG_TCBD_OFFS		20
#define MCSC_GLOBAL_CONFIG_TCBD_MASK		(1 << MCSC_GLOBAL_CONFIG_TCBD_OFFS)
#define MCSC_GLOBAL_CONFIG_MAI_OFFS		21
#define MCSC_GLOBAL_CONFIG_MAI_MASK		(1 << MCSC_GLOBAL_CONFIG_MAI_OFFS)
#define MCSC_GLOBAL_CONFIG_RXEN_OFFS		30
#define MCSC_GLOBAL_CONFIG_RXEN_MASK		(1 << MCSC_GLOBAL_CONFIG_RXEN_OFFS)
#define MCSC_GLOBAL_CONFIG_TXEN_OFFS		31
#define MCSC_GLOBAL_CONFIG_TXEN_MASK		(1 << MCSC_GLOBAL_CONFIG_TXEN_OFFS)

/* MCSC_GLOBAL_INT_CAUSE_REG */
#define  MCSC_GLOBAL_INT_CAUSE_INIT_DONE_OFFS	25
#define  MCSC_GLOBAL_INT_CAUSE_INIT_DONE_MASK	(1 << MCSC_GLOBAL_INT_CAUSE_INIT_DONE_OFFS)

/* MCSC_GLOBAL_CONFIG_EXTENDED_REG bits */
#define  MCSC_GLOBAL_CONFIG_LINEAR_TX_SWAP_OFFS	2
#define  MCSC_GLOBAL_CONFIG_LINEAR_TX_SWAP_MASK	(1 << MCSC_GLOBAL_CONFIG_LINEAR_TX_SWAP_OFFS)
#define  MCSC_GLOBAL_CONFIG_LINEAR_RX_SWAP_OFFS	3
#define  MCSC_GLOBAL_CONFIG_LINEAR_RX_SWAP_MASK	(1 << MCSC_GLOBAL_CONFIG_LINEAR_RX_SWAP_OFFS)

/* Multi-Channel DMA(MCDMA) */
#define MCDMA_RECEIVE_CONTROL_REG(ch)		(0x3000 + (ch<<2))
#define MCDMA_CURRENT_RECEIVE_DESC_PTR_REG(ch)	(0x4000 + (ch<<2))
#define MCDMA_GLOBAL_CONTROL_REG		0x4400
#define RX_SERVICE_QUEUE_ARBITER_WEIGHT_REG	0x4408
#define MCDMA_TRANSMIT_CONTROL_REG(ch)		(0x5000 + (ch<<2))
#define MCDMA_CURRENT_TRANSMIT_DESC_PTR_REG(ch)	(0x7000 + (ch<<2))
#define TX_SERVICE_QUEUE_ARBITER_WEIGHT_REG	0x7408

/* MCDMA_RECEIVE_CONTROL_REG bits */
#define MCDMA_RBSZ_16BYTE			0x1

#define MCDMA_BLMR_OFFS				2
#define MCDMA_BLMR_MASK				(1 << MCDMA_BLMR_OFFS)
#define MCDMA_ERD_OFFS				6
#define MCDMA_ERD_MASK				(1 << MCDMA_ERD_OFFS)

/* MCDMA_GLOBAL_CONTROL_REG bits */
#define MCDMA_RID_OFFS				1
#define MCDMA_RID_MASK				(1 << MCDMA_RID_OFFS)

/* MCDMA_TRANSMIT_CONTROL_REG bits */
#define MCDMA_FSIZE_1BLK			0x1
#define MCDMA_TBSZ_OFFS				8
#define MCDMA_TBSZ_16BYTE			(0x1 << MCDMA_TBSZ_OFFS)
#define MCDMA_BLMT_OFFS				10
#define MCDMA_BLMT_MASK				(1 << MCDMA_BLMT_OFFS)
#define MCDMA_TXD_OFFS				17
#define MCDMA_TXD_MASK				(1 << MCDMA_TXD_OFFS)

/* Time Division Multiplexing(TDM) */
#define FLEX_TDM_TDPR_REG(entry)		(0x8000 + (entry<<2))
#define FLEX_TDM_RDPR_REG(entry)		(0x8400 + (entry<<2))
#define FLEX_TDM_CONFIG_REG			0x8808
#define TDM_CLK_AND_SYNC_CONTROL_REG		0x881C
#define TDM_OUTPUT_SYNC_BIT_COUNT_REG		0x8C8C
#define TDM_DATA_DELAY_AND_CLK_CTRL_REG		0x8CD0

/* TDM_CLK_AND_SYNC_CONTROL_REG bits */
#define TDM_TX_FSYNC_OUT_ENABLE_OFFS		0
#define TDM_TX_FSYNC_OUT_ENABLE_MASK		(1 << TDM_TX_FSYNC_OUT_ENABLE_OFFS)
#define TDM_RX_FSYNC_OUT_ENABLE_OFFS		1
#define TDM_RX_FSYNC_OUT_ENABLE_MASK		(1 << TDM_RX_FSYNC_OUT_ENABLE_OFFS)
#define TDM_TX_CLK_OUT_ENABLE_OFFS		2
#define TDM_TX_CLK_OUT_ENABLE_MASK		(1 << TDM_TX_CLK_OUT_ENABLE_OFFS)
#define TDM_RX_CLK_OUT_ENABLE_OFFS		3
#define TDM_RX_CLK_OUT_ENABLE_MASK		(1 << TDM_RX_CLK_OUT_ENABLE_OFFS)
#define TDM_REFCLK_DIVIDER_BYPASS_OFFS		20
#define TDM_REFCLK_DIVIDER_BYPASS_MASK		(3 << TDM_REFCLK_DIVIDER_BYPASS_OFFS)
#define TDM_OUT_CLK_SRC_CTRL_OFFS		24
#define TDM_OUT_CLK_SRC_CTRL_AFTER_DIV		(1 << TDM_OUT_CLK_SRC_CTRL_OFFS)
#define TDM_PROG_TDM_SLIC_RESET_OFFS		31
#define TDM_PROG_TDM_SLIC_RESET_MASK		(1 << TDM_PROG_TDM_SLIC_RESET_OFFS)

/* FLEX_TDM_CONFIG_REG bits */
#define TDM_RR2HALF_OFFS			15
#define TDM_RR2HALF_MASK			(1 << TDM_RR2HALF_OFFS)
#define TDM_TR2HALF_OFFS			16
#define TDM_TR2HALF_MASK			(1 << TDM_TR2HALF_OFFS)
#define TDM_SE_OFFS				20
#define TDM_SE_MASK				(1 << TDM_SE_OFFS)
#define TDM_COMMON_RX_TX_OFFS			23
#define TDM_COMMON_RX_TX_MASK			(1 << TDM_COMMON_RX_TX_OFFS)
#define TSD_OFFS				25
#define TSD_NO_DELAY				(0 << TSD_OFFS)
#define RSD_OFFS				27
#define RSD_NO_DELAY				(0 << RSD_OFFS)
#define TDM_TEN_OFFS				31
#define TDM_TEN_MASK				(1 << TDM_TEN_OFFS)

/* TDM_OUTPUT_SYNC_BIT_COUNT_REG bits */
#define TDM_SYNC_BIT_TX_OFFS			0
#define TDM_SYNC_BIT_TX_MASK			(0xffff << TDM_SYNC_BIT_TX_OFFS)
#define TDM_SYNC_BIT_RX_OFFS			16
#define TDM_SYNC_BIT_RX_MASK			(0xffff << TDM_SYNC_BIT_RX_OFFS)

/* TDM_DATA_DELAY_AND_CLK_CTRL_REG bits */
#define TX_CLK_OUT_ENABLE_OFFS			0
#define TX_CLK_OUT_ENABLE_MASK			(1 << TX_CLK_OUT_ENABLE_OFFS)
#define RX_CLK_OUT_ENABLE_OFFS			1
#define RX_CLK_OUT_ENABLE_MASK			(1 << RX_CLK_OUT_ENABLE_OFFS)

/************************************************/
/*	Shared Bus to Crossbar Bridge		*/
/************************************************/
#define COMM_UNIT_MBUS_MAX_WIN			12

#define COMM_UNIT_WIN_CTRL_REG(win)		(0x8A00 + (win<<3))
#define COMM_UNIT_WIN_SIZE_REG(win)		(0x8A04 + (win<<3))
#define COMM_UNIT_WIN_ENABLE_REG(win)		(0x8B04 + (win<<2))
#define COMM_UNIT_WINDOWS_ACCESS_PROTECT_REG	0x8B00
#define TIME_OUT_COUNTER_REG			0x8ADC

/* TIME_OUT_COUNTER_REG bits */
#define	TIME_OUT_THRESHOLD_COUNT_OFFS		16
#define	TIME_OUT_THRESHOLD_COUNT_MASK		(0xffff << TIME_OUT_THRESHOLD_COUNT_OFFS)

/* Defines */
#define MV_TDMMC_TOTAL_CHANNELS			32
#define MV_TDM_MAX_HALF_DPRAM_ENTRIES		128

/* IRQ types */
#define TDM_TX_INT		 TX_VOICE_INT_PULSE_MASK
#define TDM_RX_INT		 RX_VOICE_INT_PULSE_MASK
#define TDM_ERROR_INT \
	(FLEX_TDM_RX_SYNC_LOSS_MASK | FLEX_TDM_TX_SYNC_LOSS_MASK |	\
	 COMM_UNIT_PAR_ERR_SUM_MASK | TDM_RX_PAR_ERR_SUM_MASK |		\
	 TDM_TX_PAR_ERR_SUM_MASK | MCSC_PAR_ERR_SUM_MASK |		\
	 MCDMA_PAR_ERR_SUM_MASK)

/* MCDMA Descriptor Command/Status Bits */
#define	LAST_BIT	0x00010000
#define	FIRST_BIT	0x00020000
#define	AUTO_MODE	0x40000000
#define	OWNER		0x80000000

/* MCDMA */
#define CONFIG_MCDMA_DESC_CMD_STATUS	(FIRST_BIT | AUTO_MODE | OWNER)
#define CONFIG_RMCCx			(MCDMA_RBSZ_16BYTE | MCDMA_BLMR_MASK)
#define CONFIG_TMCCx \
	(MCDMA_FSIZE_1BLK | MCDMA_TBSZ_16BYTE | MCDMA_BLMT_MASK)

/* MCSC */
#define CONFIG_MRCRx			(MRCRx_RRVD_MASK | MRCRx_MODE_MASK)
#define CONFIG_MTCRx			(MTCRx_TRVD_MASK | MTCRx_MODE_MASK)
#define CONFIG_LINEAR_BYTE_SWAP \
	(MCSC_GLOBAL_CONFIG_LINEAR_TX_SWAP_MASK |	\
	MCSC_GLOBAL_CONFIG_LINEAR_RX_SWAP_MASK)
/* TDM */
#if defined(MV_TDM_USE_EXTERNAL_PCLK_SOURCE)
#define CONFIG_TDM_CLK_AND_SYNC_CONTROL	\
	(TDM_TX_CLK_OUT_ENABLE_MASK | TDM_RX_CLK_OUT_ENABLE_MASK |	\
	TDM_REFCLK_DIVIDER_BYPASS_MASK)
#else
#define CONFIG_TDM_CLK_AND_SYNC_CONTROL	\
	(TDM_REFCLK_DIVIDER_BYPASS_MASK | TDM_OUT_CLK_SRC_CTRL_AFTER_DIV)
#endif

#define CONFIG_VOICE_PERIODICAL_INT_CONTROL \
	(((MV_TDM_TOTAL_CH_SAMPLES) << RX_VOICE_INT_CNT_REF_OFFS) |	\
	((MV_TDM_TOTAL_CH_SAMPLES) << TX_VOICE_INT_CNT_REF_OFFS) |	\
	(2 << RX_FIRST_DELAY_REF_OFFS) | (4 << TX_FIRST_DELAY_REF_OFFS))
#define CONFIG_VOICE_PERIODICAL_INT_CONTROL_WA \
	(((MV_TDM_TOTAL_CH_SAMPLES - 1) << RX_VOICE_INT_CNT_REF_OFFS) |	\
	((MV_TDM_TOTAL_CH_SAMPLES - 1) << TX_VOICE_INT_CNT_REF_OFFS) |	\
	(2 << RX_FIRST_DELAY_REF_OFFS) | (4 << TX_FIRST_DELAY_REF_OFFS))
#define CONFIG_TDM_CAUSE \
	(TDM_RX_INT | TDM_TX_INT)
#define CONFIG_COMM_UNIT_TOP_MASK \
	(TDM_SUM_INT_MASK | MCSC_SUM_INT_MASK)
#define CONFIG_FLEX_TDM_CONFIG \
	(TDM_SE_MASK | TDM_COMMON_RX_TX_MASK | TSD_NO_DELAY | RSD_NO_DELAY)
#define	CONFIG_TDM_DATA_DELAY_AND_CLK_CTRL \
	(TX_CLK_OUT_ENABLE_MASK | RX_CLK_OUT_ENABLE_MASK)

/* Defines */
#define TOTAL_CHAINS		2
#define CONFIG_RBSZ		16
#define NEXT_BUFF(buff)		((buff + 1) % TOTAL_CHAINS)
#define PREV_BUFF(buff)		(buff == 0 ? (TOTAL_CHAINS - 1) : (buff - 1))
#define MAX_POLL_USEC		100000	/* 100ms */
#define COMM_UNIT_SW_RST	(1 << 5)
#define OLD_INT_WA_BIT		(1 << 15)
#define MV_TDM_PCM_CLK_8MHZ	1

/* Enums */
enum tdmmc_ip_version {
	TDMMC_REV0 = 0,
	TDMMC_REV1
};

/* Structures */
struct tdmmc_mcdma_rx_desc {
	u32 cmd_status;
	u16 byte_cnt;
	u16 buff_size;
	u32 phys_buff_ptr;
	u32 phys_next_desc_ptr;
};

struct tdmmc_mcdma_tx_desc {
	u32 cmd_status;
	u16 shadow_byte_cnt;
	u16 byte_cnt;
	u32 phys_buff_ptr;
	u32 phys_next_desc_ptr;
};

struct tdmmc_dram_entry {
	u32 mask:8;
	u32 ch:8;
	u32 mgs:2;
	u32 byte:1;
	u32 strb:2;
	u32 elpb:1;
	u32 tbs:1;
	u32 rpt:2;
	u32 last:1;
	u32 ftint:1;
	u32 reserved31_27:5;
};

/* Main TDM structure definition */
struct tdmmc_dev {
	/* Resources */
	void __iomem *regs;
	struct device *dev;

	/* Silicon revision */
	enum tdmmc_ip_version ip_ver;

	/* Buffers */
	u8 *rx_buff_virt[TOTAL_CHAINS];
	u8 *tx_buff_virt[TOTAL_CHAINS];
	dma_addr_t rx_buff_phys[TOTAL_CHAINS];
	dma_addr_t tx_buff_phys[TOTAL_CHAINS];
	u8 prev_rx;
	u8 next_tx;

	/* MCDMA descriptors */
	struct tdmmc_mcdma_rx_desc *rx_desc_virt[TOTAL_CHAINS];
	struct tdmmc_mcdma_tx_desc *tx_desc_virt[TOTAL_CHAINS];
	dma_addr_t rx_desc_phys[TOTAL_CHAINS];
	dma_addr_t tx_desc_phys[TOTAL_CHAINS];

	/* Flags */
	bool tdm_enable;
	bool pcm_enable;

	/* Parameters */
	u8 sample_size;
	u8 sampling_coeff;
	u16 total_channels;
};

/* TDMMC APIs */
void tdmmc_pcm_start(void);
void tdmmc_pcm_stop(void);
int tdmmc_tx(u8 *tdm_tx_buff);
int tdmmc_rx(u8 *tdm_rx_buff);
void tdmmc_show(void);
void tdmmc_release(void);
void tdmmc_intr_enable(u8 device_id);
void tdmmc_intr_disable(u8 device_id);
int tdmmc_reset_slic(void);
int tdmmc_set_mbus_windows(struct device *dev, void __iomem *regs);
int tdmmc_set_a8k_windows(struct device *dev, void __iomem *regs);

#endif /* _TDMMC_H_ */

