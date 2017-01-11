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

/*******************************************************************************
* mv_phone.h - Marvell TDM unit specific configurations
*
* DESCRIPTION:
*       None.
*
* DEPENDENCIES:
*       None.
*
*******************************************************************************/
#ifndef _MV_PHONE_H_
#define _MV_PHONE_H_

#include <linux/clk.h>
#include <linux/delay.h>
#include <linux/dma-mapping.h>
#include <linux/init.h>
#include <linux/io.h>
#include <linux/mbus.h>
#include <linux/module.h>
#include <linux/of.h>
#include <linux/platform_device.h>
#include <linux/slab.h>
#include "tdm2c/tdm2c.h"
#include "tdmmc/tdmmc.h"

/****************************************************************/
/*************** Telephony configuration ************************/
/****************************************************************/

/* Core DivClk Control Register */

/* DCO clock apply/reset bits */
#define DCO_CLK_DIV_MOD_OFFS			24
#define DCO_CLK_DIV_APPLY_MASK			(0x1 << DCO_CLK_DIV_MOD_OFFS)
#define DCO_CLK_DIV_RESET_OFFS			25
#define DCO_CLK_DIV_RESET_MASK			(0x1 << DCO_CLK_DIV_RESET_OFFS)

/* DCO clock ratio is 24Mhz/x */
#define DCO_CLK_DIV_RATIO_OFFS			26
#define DCO_CLK_DIV_RATIO_MASK			0xfc000000
#define DCO_CLK_DIV_RATIO_8M			(0x3 << DCO_CLK_DIV_RATIO_OFFS)
#define DCO_CLK_DIV_RATIO_4M			(0x6 << DCO_CLK_DIV_RATIO_OFFS)
#define DCO_CLK_DIV_RATIO_2M			(0xc << DCO_CLK_DIV_RATIO_OFFS)

/* TDM PLL configuration registers */
#define TDM_PLL_CONF_REG0			0x0
#define TDM_PLL_FB_CLK_DIV_OFFSET		10
#define TDM_PLL_FB_CLK_DIV_MASK			0x7fc00

#define TDM_PLL_CONF_REG1			0x4
#define TDM_PLL_FREQ_OFFSET_MASK		0xffff
#define TDM_PLL_FREQ_OFFSET_VALID		0x00010000
#define TDM_PLL_SW_RESET			0x80000000

#define TDM_PLL_CONF_REG2			0x8
#define TDM_PLL_POSTDIV_MASK			0x7f

/* TDM control/SPI registers used for suspend/resume */
#define TDM_CTRL_REGS_NUM			36
#define TDM_SPI_REGS_OFFSET			0x3100
#define TDM_SPI_REGS_NUM			16

/* Structures and enums */
enum mv_phone_unit_type {
	MV_TDM_UNIT_NONE,
	MV_TDM_UNIT_TDM2C,
	MV_TDM_UNIT_TDMMC
};

enum mv_phone_band_mode {
	MV_NARROW_BAND = 0,
	MV_WIDE_BAND,
};

enum mv_phone_pcm_format {
	MV_PCM_FORMAT_1BYTE = 1,
	MV_PCM_FORMAT_2BYTES = 2,
	MV_PCM_FORMAT_4BYTES = 4
};

enum mv_phone_frame_ts {
	MV_FRAME_32TS = 32,
	MV_FRAME_64TS = 64,
	MV_FRAME_128TS = 128
};

enum mv_phone_spi_mode {
	MV_SPI_MODE_DIRECT = 0,
	MV_SPI_MODE_DAISY_CHAIN = 1
};

struct mv_phone_extended_stats {
	u32 int_rx_count;
	u32 int_tx_count;
	u32 int_rx0_count;
	u32 int_tx0_count;
	u32 int_rx1_count;
	u32 int_tx1_count;
	u32 int_rx0_miss;
	u32 int_tx0_miss;
	u32 int_rx1_miss;
	u32 int_tx1_miss;
	u32 pcm_restart_count;
};

struct mv_phone_intr_info {
	u8 *tdm_rx_buff;
	u8 *tdm_tx_buff;
	u32 int_type;
	u8 cs;
	u8 data;
};

struct mv_phone_params {
	enum mv_phone_pcm_format pcm_format;
	u16 pcm_slot[32];
	u8 sampling_period;
	u16 total_channels;
};

struct mv_phone_dev {
	void __iomem *tdm_base;
	void __iomem *pll_base;
	void __iomem *dco_div_reg;
	struct mv_phone_params *tdm_params;
	enum mv_phone_unit_type tdm_type;
	struct platform_device *parent;
	struct device_node *np;
	struct clk *clk;
	u32 pclk_freq_mhz;
	u8 irq_count;
	int irq[3];
	spinlock_t lock;

	/* Used to preserve TDM registers across suspend/resume */
	u32 tdm_ctrl_regs[TDM_CTRL_REGS_NUM];
	u32 tdm_spi_regs[TDM_SPI_REGS_NUM];
	u32 tdm_spi_mux_reg;
	u32 tdm_mbus_config_reg;
	u32 tdm_misc_reg;

	struct device *dev;

	/* Transmit buffers */
	u8 *rx_buff;
	u8 *tx_buff;
	int buff_size;

	/* Transmit statistics */
	struct proc_dir_entry *tdm_stats;
	u32 rx_miss;
	u32 tx_miss;
	u32 rx_over;
	u32 tx_under;

	/* TDM operation flags */
	bool tdm_init;
	bool test_enable;
	bool pcm_enable;
	bool pcm_stop_flag;
	bool pcm_stop_status;
	int pcm_start_stop_state;
	bool pcm_is_stopping;
	u32 pcm_stop_fail;
	bool use_tdm_ext_stats;

	/* TDM2C SPI operation mode */
	enum mv_phone_spi_mode tdm2c_spi_mode;
	/* TDM2C PCLK source */
	bool use_pclk_external;

	/* TDMMC silicon revision */
	enum tdmmc_ip_version tdmmc_ip_ver;
};

/* This enumerator defines the Marvell Units ID */
enum mv_phone_slic_unit_type {
	SLIC_EXTERNAL_ID,
	SLIC_ZARLINK_ID,
	SLIC_SILABS_ID,
	SLIC_LANTIQ_ID
};

enum mv_phone_spi_type {
	SPI_TYPE_FLASH = 0,
	SPI_TYPE_SLIC_ZARLINK_SILABS,
	SPI_TYPE_SLIC_LANTIQ,
	SPI_TYPE_SLIC_ZSI,
	SPI_TYPE_SLIC_ISI
};

enum mv_phone_board_slic_type {
	MV_BOARD_SLIC_DISABLED,
	MV_BOARD_SLIC_SSI_ID, /* Lantiq Integrated SLIC */
	MV_BOARD_SLIC_ISI_ID, /* Silicon Labs ISI Bus */
	MV_BOARD_SLIC_ZSI_ID, /* Zarlink ZSI Bus */
	MV_BOARD_SLIC_EXTERNAL_ID /* Cross vendor external SLIC */
};

/* Helper macros and routines */
static inline void mv_phone_set_bit(void __iomem *addr, u32 bit_mask)
{
	writel(readl(addr) | bit_mask, addr);
}

static inline void mv_phone_reset_bit(void __iomem *addr, u32 bit_mask)
{
	writel(readl(addr) & ~bit_mask, addr);
}

/* MV Phone */
u32 mv_phone_get_slic_board_type(void);
void mv_phone_spi_write(u16 line_id, u8 *cmd_buff, u8 cmd_size,
			u8 *data_buff, u8 data_size, u32 spi_type);
void mv_phone_spi_read(u16 line_id, u8 *cmd_buff, u8 cmd_size,
		       u8 *data_buff, u8 data_size, u32 spi_type);
void mv_phone_intr_enable(u8 device_id);
void mv_phone_intr_disable(u8 device_id);

/* TDM2C */
int tdm2c_init(void __iomem *base, struct device *dev,
	       struct mv_phone_params *tdm_params,
	       enum mv_phone_frame_ts frame_ts,
	       enum mv_phone_spi_mode spi_mode,
	       bool use_pclk_external);
int tdm2c_intr_low(struct mv_phone_intr_info *tdm_intr_info);
void tdm2c_ext_stats_get(struct mv_phone_extended_stats *tdm_ext_stats);

/* TDMMC */
int tdmmc_init(void __iomem *base, struct device *dev, struct mv_phone_params *tdm_params,
	       enum mv_phone_frame_ts frame_ts, enum tdmmc_ip_version tdmmc_ip_ver);
int tdmmc_intr_low(struct mv_phone_intr_info *tdm_intr_info);

#endif /* _MV_PHONE_H_ */

