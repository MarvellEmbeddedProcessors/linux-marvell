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

#ifndef _TDM2C_H_
#define _TDM2C_H_

/************************************************/
/*        TDM to Mbus Bridge Register Map       */
/************************************************/
#define TDM_SPI_MUX_REG		0x4000
#define TDM_MBUS_CONFIG_REG	0x4010
#define TDM_MISC_REG		0x4070

/* TDM Control Register Map */
#define PCM_CTRL_REG		0x00
#define TIMESLOT_CTRL_REG	0x04
#define FRAME_TIMESLOT_REG	0x38
#define PCM_CLK_RATE_DIV_REG	0x3c
#define INT_EVENT_MASK_REG	0x40
#define INT_STATUS_MASK_REG	0x48
#define INT_RESET_SELECT_REG	0x4c
#define INT_STATUS_REG		0x50
#define DUMMY_RX_WRITE_DATA_REG 0x54
#define MISC_CTRL_REG		0x58
#define TDM_REV_REG		0x74
#define DMA_ABORT_ADDR_REG	0x80
#define DMA_ABORT_INFO_REG	0x84

#define CH_WB_DELAY_CTRL_REG(ch)(0x88 | ((ch) << 2))
#define CH_DELAY_CTRL_REG(ch)	(0x08 | ((ch) << 2))
#define CH_SAMPLE_REG(ch)	(0x30 | ((ch) << 2))
#define CH_DBG_REG(ch)		(0x78 | ((ch) << 2))
#define CH_TX_CUR_ADDR_REG(ch)	(0x60 | ((ch) << 3))
#define CH_RX_CUR_ADDR_REG(ch)	(0x64 | ((ch) << 3))
#define CH_ENABLE_REG(ch)	(((ch) + 1) << 4)
#define CH_RXTX_EN_MASK		0x101
#define CH_BUFF_OWN_REG(ch)	(0x04 | (((ch) + 1) << 4))
#define CH_TX_ADDR_REG(ch)	(0x08 | (((ch) + 1) << 4))
#define CH_RX_ADDR_REG(ch)	(0x0c | (((ch) + 1) << 4))

#define PCM_DIV_PASS	(1 << 6)

/* PCM_CTRL_REG bits */
#define MASTER_PCLK_OFFS	0
#define MASTER_PCLK_TDM		(0 << MASTER_PCLK_OFFS)
#define MASTER_PCLK_EXTERNAL	(1 << MASTER_PCLK_OFFS)
#define MASTER_FS_OFFS		1
#define MASTER_FS_TDM		(0 << MASTER_FS_OFFS)
#define DATA_POLAR_OFFS		2
#define DATA_POLAR_NEG		(0 << DATA_POLAR_OFFS)
#define FS_POLAR_OFFS		3
#define FS_POLAR_NEG		(0 << FS_POLAR_OFFS)
#define INVERT_FS_OFFS		4
#define INVERT_FS_HI		(0 << INVERT_FS_OFFS)
#define FS_TYPE_OFFS		5
#define FS_TYPE_SHORT		(0 << FS_TYPE_OFFS)
#define PCM_SAMPLE_SIZE_OFFS	6
#define CH_DELAY_OFFS		8
#define CH_DELAY_ENABLE		(3 << CH_DELAY_OFFS)
#define CH_QUALITY_OFFS		10
#define CH_QUALITY_DISABLE	(0 << CH_QUALITY_OFFS)
#define QUALITY_POLARITY_OFFS	12
#define QUALITY_POLARITY_NEG	(0 << QUALITY_POLARITY_OFFS)
#define QUALITY_TYPE_OFFS	13
#define QUALITY_TYPE_TIME_SLOT	(0 << QUALITY_TYPE_OFFS)
#define CS_CTRL_OFFS		15
#define CS_CTRL_DONT_CARE	(0 << CS_CTRL_OFFS)
#define CS_CTRL			(1 << CS_CTRL_OFFS)
#define WIDEBAND_OFFS		16
#define WIDEBAND_OFF		(0 << WIDEBAND_OFFS)
#define WIDEBAND_ON		(3 << WIDEBAND_OFFS)
#define PERF_GBUS_OFFS		31
#define PERF_GBUS_TWO_ACCESS	(1 << PERF_GBUS_OFFS)

/* CH_SAMPLE_REG bits */
#define TOTAL_CNT_OFFS		0
#define INT_CNT_OFFS		8

/* CH_BUFF_OWN_REG bits */
#define RX_OWN_BYTE_OFFS	0
#define TX_OWN_BYTE_OFFS	1
#define OWNER_MASK		1
#define OWN_BY_HW		1

/* CH_ENABLE_REG bits */
#define RX_ENABLE_BYTE_OFFS	0
#define TX_ENABLE_BYTE_OFFS	1
#define CH_ENABLE		1
#define CH_DISABLE		0

/* INT_STATUS_REG bits */
#define RX_OVERFLOW_BIT(ch)	(1 << (0 + ((ch) * 2)))
#define TX_UNDERFLOW_BIT(ch)	(1 << (1 + ((ch) * 2)))
#define RX_BIT(ch)		(1 << (4 + ((ch) * 2)))
#define TX_BIT(ch)		(1 << (5 + ((ch) * 2)))
#define RX_IDLE_BIT(ch)		(1 << (8 + ((ch) * 2)))
#define TX_IDLE_BIT(ch)		(1 << (9 + ((ch) * 2)))
#define DMA_ABORT_BIT		(1 << 16)
#define SLIC_INT_BIT		(1 << 17)

/* TDU_INTR_SET_RESET bits */
#define CLEAR_MODE_OFFS		0
#define CLEAR_ON_ZERO		(0 << CLEAR_MODE_OFFS)

/* CH_DELAY_CTRL_REG bits */
#define CH_RX_DELAY_OFFS	0
#define CH_TX_DELAY_OFFS	16

/* SPI Register Map */
#define SPI_CLK_PRESCALAR_REG	0x3100
#define SPI_GLOBAL_CTRL_REG	0x3104
#define SPI_CTRL_REG		0x3108
#define SPI_CODEC_CMD_LO_REG	0x3130
#define SPI_CODEC_CMD_HI_REG	0x3134
#define SPI_CODEC_CTRL_REG	0x3138
#define SPI_CODEC_READ_DATA_REG	0x313c

/* SPI CLK_PRESCALAR_REG bits */
#define SPI_CLK_2MHZ	0x2A64  /* refers to tclk = 200MHz */

/* SPI_CTRL_REG bits */
#define SPI_STAT_OFFS	10
#define SPI_STAT_MASK	(1 << SPI_STAT_OFFS)
#define SPI_ACTIVE	(1 << SPI_STAT_OFFS)

/* SPI_GLOBAL_CTRL_REG bits */
#define SPI_GLOBAL_ENABLE_OFFS	0
#define SPI_GLOBAL_ENABLE	(1 << SPI_GLOBAL_ENABLE_OFFS)

/* SPI_CODEC_CTRL_REG bits */
#define TRANSFER_BYTES_OFFS	0
#define TRANSFER_BYTES(count)	((count-1) << TRANSFER_BYTES_OFFS)
#define ENDIANNESS_MODE_OFFS	2
#define ENDIANNESS_MSB_MODE	(0 << ENDIANNESS_MODE_OFFS)
#define RD_WR_MODE_OFFS		3
#define WR_MODE			(0 << RD_WR_MODE_OFFS)
#define RD_MODE			(1 << RD_WR_MODE_OFFS)
#define READ_BYTES_OFFS		4
#define READ_1_BYTE		(0 << READ_BYTES_OFFS)
#define READ_2_BYTE		(1 << READ_BYTES_OFFS)
#define CLK_SPEED_OFFS		5
#define CLK_SPEED_LO_DIV	(0 << CLK_SPEED_OFFS)

/* TDM Address Decoding */
#define TDM_MBUS_MAX_WIN	4
#define TDM_WIN_CTRL_REG(win)	(0x4030 + (win<<4))
#define TDM_WIN_BASE_REG(win)	(0x4034 + (win<<4))

/* Defines */
#define SAMPLES_BUFF_SIZE(band_mode, factor)  \
	 ((band_mode == MV_NARROW_BAND) ? (factor * 80) : (factor * 160))

#define MV_TDM_CH_BUFF_SIZE(pcm_format, band_mode, factor)	\
	(pcm_format == MV_PCM_FORMAT_2BYTES ?			\
	(2 * SAMPLES_BUFF_SIZE(band_mode, factor)) :		\
	SAMPLES_BUFF_SIZE(band_mode, factor))

#define MV_TDM_AGGR_BUFF_SIZE(pcm_format, band_mode, factor)	\
	(2 * MV_TDM_CH_BUFF_SIZE(pcm_format, band_mode, factor))
#define MV_TDM2C_TOTAL_CHANNELS			2
#define MV_TDM_INT_COUNTER			2
#define MV_TDM_MAX_SAMPLING_PERIOD		30	/* ms */
#define MV_TDM_BASE_SAMPLING_PERIOD		10	/* ms */
#define MV_TDM_TOTAL_CH_SAMPLES			80	/* samples */
#define MV_TDM_STOP_POLLING_TIMEOUT		30	/* ms */

/* TDM IRQ types */
#define MV_EMPTY_INT		0
#define MV_RX_INT		0x00000001
#define	MV_TX_INT		0x00000002
#define	MV_PHONE_INT		0x00000004
#define	MV_RX_ERROR_INT		0x00000008
#define	MV_TX_ERROR_INT		0x00000010
#define MV_DMA_ERROR_INT	0x00000020
#define MV_CHAN_STOP_INT	0x00000040
#define MV_ERROR_INT		(MV_RX_ERROR_INT | MV_TX_ERROR_INT | MV_DMA_ERROR_INT)

/* PCM SLOT configuration */
#define PCM_SLOT_PCLK	8

#define TDM_INT_SLIC	(DMA_ABORT_BIT | SLIC_INT_BIT)
#define TDM_INT_TX(ch)	(TX_UNDERFLOW_BIT(ch) | TX_BIT(ch) | TX_IDLE_BIT(ch))
#define TDM_INT_RX(ch)	(RX_OVERFLOW_BIT(ch) | RX_BIT(ch) | RX_IDLE_BIT(ch))

/* TDM Registers Configuration */
#if defined(MV_TDM_USE_EXTERNAL_PCLK_SOURCE)
#define CONFIG_PCM_CRTL (MASTER_PCLK_EXTERNAL | MASTER_FS_TDM |		\
			 DATA_POLAR_NEG | FS_POLAR_NEG | INVERT_FS_HI |	\
			 FS_TYPE_SHORT | CH_DELAY_ENABLE |		\
			 CH_QUALITY_DISABLE | QUALITY_POLARITY_NEG |	\
			 QUALITY_TYPE_TIME_SLOT | CS_CTRL_DONT_CARE |	\
			 WIDEBAND_OFF | PERF_GBUS_TWO_ACCESS)

#else
#define CONFIG_PCM_CRTL (MASTER_PCLK_TDM | MASTER_FS_TDM |		\
			 DATA_POLAR_NEG | FS_POLAR_NEG | INVERT_FS_HI |	\
			 FS_TYPE_SHORT | CH_DELAY_ENABLE |		\
			 CH_QUALITY_DISABLE | QUALITY_POLARITY_NEG |	\
			 QUALITY_TYPE_TIME_SLOT | CS_CTRL_DONT_CARE |	\
			 WIDEBAND_OFF | PERF_GBUS_TWO_ACCESS)
#endif

#if defined(MV_TDM_USE_EXTERNAL_PCLK_SOURCE)
#define CONFIG_WB_PCM_CRTL (MASTER_PCLK_EXTERNAL | MASTER_FS_TDM |	\
			    DATA_POLAR_NEG | FS_POLAR_NEG |		\
			    INVERT_FS_HI | FS_TYPE_SHORT	 |	\
			    CH_DELAY_ENABLE | CH_QUALITY_DISABLE |	\
			    QUALITY_POLARITY_NEG |			\
			    QUALITY_TYPE_TIME_SLOT | CS_CTRL_DONT_CARE |\
			    WIDEBAND_ON | PERF_GBUS_TWO_ACCESS)
#else
#define CONFIG_WB_PCM_CRTL (MASTER_PCLK_TDM | MASTER_FS_TDM |		\
			    DATA_POLAR_NEG | FS_POLAR_NEG |		\
			    INVERT_FS_HI | FS_TYPE_SHORT |		\
			    CH_DELAY_ENABLE | CH_QUALITY_DISABLE |	\
			    QUALITY_POLARITY_NEG |			\
			    QUALITY_TYPE_TIME_SLOT | CS_CTRL_DONT_CARE |\
			    WIDEBAND_ON | PERF_GBUS_TWO_ACCESS)
#endif

#define CONFIG_CH_SAMPLE(bandMode, factor)	\
	((SAMPLES_BUFF_SIZE(bandMode, factor)<<TOTAL_CNT_OFFS) |	\
	(INT_SAMPLE<<INT_CNT_OFFS))

/* Defines */
#define INT_SAMPLE			2
#define BUFF_IS_FULL			1
#define BUFF_IS_EMPTY			0
#define FIRST_INT			1
#define TOTAL_BUFFERS			2
#define MV_TDM_NEXT_BUFFER(buf)		((buf + 1) % TOTAL_BUFFERS)
#define MV_TDM_PREV_BUFFER(buf, step)	((TOTAL_BUFFERS + buf - step) % TOTAL_BUFFERS)
#define MV_TDM_CS			0
#define BUFF_INVALID			-1

/* TDM channel info structure */
struct tdm2c_ch_info {
	u8 ch;
	u8 *rx_buff_virt[TOTAL_BUFFERS];
	u8 *tx_buff_virt[TOTAL_BUFFERS];
	dma_addr_t rx_buff_phys[TOTAL_BUFFERS];
	dma_addr_t tx_buff_phys[TOTAL_BUFFERS];
	u8 rx_buff_full[TOTAL_BUFFERS];
	u8 tx_buff_empty[TOTAL_BUFFERS];
	u8 rx_curr_buff;
	u8 tx_curr_buff;
	u8 rx_first;
};

/* APIs */
void tdm2c_release(void);
int tdm2c_pcm_stop_int_miss(void);
void tdm2c_pcm_start(void);
void tdm2c_pcm_stop(void);
int tdm2c_tx(u8 *tdmTxBuff);
int tdm2c_rx(u8 *tdmRxBuff);
void tdm2c_regs_dump(void);
void tdm2c_intr_enable(void);
void tdm2c_intr_disable(void);
void tdm2c_pcm_if_reset(void);
int tdm2c_set_mbus_windows(struct device *dev, void __iomem *regs,
			   const struct mbus_dram_target_info *dram);

#endif /* _TDM2C_H_ */
