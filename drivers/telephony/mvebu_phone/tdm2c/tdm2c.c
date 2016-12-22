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

#include "mv_phone.h"

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
	u8 *rxBuffVirt[TOTAL_BUFFERS], *txBuffVirt[TOTAL_BUFFERS];
	dma_addr_t rxBuffPhys[TOTAL_BUFFERS], txBuffPhys[TOTAL_BUFFERS];
	u8 rxBuffFull[TOTAL_BUFFERS], txBuffFull[TOTAL_BUFFERS];
	u8 rxCurrBuff, txCurrBuff;
	u8 rxFirst;
};

/* Globals */
static u8 *rx_aggr_buff_virt, *tx_aggr_buff_virt;
static u8 rx_int, tx_int;
static u16 rx_full, tx_empty;
static u8 tdm_enable;
static u8 spi_mode;
static u8 factor;
static enum mv_phone_pcm_format pcm_format;
static enum mv_phone_band_mode tdm_band_mode;
static struct tdm2c_ch_info *tdm_ch_info[MV_TDM2C_TOTAL_CHANNELS] = { NULL, NULL };
static u8 chan_stop_count;
static u8 int_lock;
static struct device *pdev;
static void __iomem *regs;

/* Stats */
static u32 int_rx_count;
static u32 int_tx_count;
static u32 int_rx0_count;
static u32 int_tx0_count;
static u32 int_rx1_count;
static u32 int_tx1_count;
static u32 int_rx0_miss;
static u32 int_tx0_miss;
static u32 int_rx1_miss;
static u32 int_tx1_miss;
static u32 pcm_restart_count;

static void tdm2c_daisy_chain_mode_set(void)
{
	while ((readl(regs + SPI_CTRL_REG) & SPI_STAT_MASK) == SPI_ACTIVE)
		continue;
	writel((0x80 << 8) | 0, regs + SPI_CODEC_CMD_LO_REG);
	writel(TRANSFER_BYTES(2) | ENDIANNESS_MSB_MODE | WR_MODE | CLK_SPEED_LO_DIV, regs + SPI_CODEC_CTRL_REG);
	writel(readl(regs + SPI_CTRL_REG) | SPI_ACTIVE, regs + SPI_CTRL_REG);
	/* Poll for ready indication */
	while ((readl(regs + SPI_CTRL_REG) & SPI_STAT_MASK) == SPI_ACTIVE)
		continue;

	dev_dbg(pdev, "%s: Exit\n", __func__);
}

static int tdm2c_ch_init(u8 ch)
{
	struct tdm2c_ch_info *ch_info;
	u32 buff;

	dev_dbg(pdev, "%s: Enter, ch%d\n", __func__, ch);

	if (ch >= MV_TDM2C_TOTAL_CHANNELS) {
		dev_err(pdev, "%s: error, channel(%d) exceeds maximum(%d)\n",
			__func__, ch, MV_TDM2C_TOTAL_CHANNELS);
		return -EINVAL;
	}

	tdm_ch_info[ch] = ch_info = kmalloc(sizeof(struct tdm2c_ch_info),
					    GFP_ATOMIC);
	if (!ch_info) {
		dev_err(pdev, "%s: error malloc failed\n", __func__);
		return -ENOMEM;
	}

	ch_info->ch = ch;

	/* Per channel TDM init */
	/* Disable channel (enable in pcm start) */
	writel(CH_DISABLE, regs + CH_ENABLE_REG(ch));
	/* Set total samples and int sample */
	writel(CONFIG_CH_SAMPLE(tdm_band_mode, factor), regs + CH_SAMPLE_REG(ch));

	for (buff = 0; buff < TOTAL_BUFFERS; buff++) {
		/* Buffers must be 32B aligned */
		ch_info->rxBuffVirt[buff] = dma_alloc_coherent(pdev,
							      MV_TDM_CH_BUFF_SIZE(pcm_format, tdm_band_mode, factor),
							      &(ch_info->rxBuffPhys[buff]), GFP_KERNEL);
		ch_info->rxBuffFull[buff] = BUFF_IS_EMPTY;

		ch_info->txBuffVirt[buff] = dma_alloc_coherent(pdev,
							      MV_TDM_CH_BUFF_SIZE(pcm_format, tdm_band_mode, factor),
							      &(ch_info->txBuffPhys[buff]), GFP_KERNEL);
		ch_info->txBuffFull[buff] = BUFF_IS_FULL;

		memset(ch_info->txBuffVirt[buff], 0, MV_TDM_CH_BUFF_SIZE(pcm_format, tdm_band_mode, factor));

		if (((ulong) ch_info->rxBuffVirt[buff] | ch_info->rxBuffPhys[buff] |
		     (ulong) ch_info->txBuffVirt[buff] | ch_info->txBuffPhys[buff]) & 0x1f) {
			dev_err(pdev, "%s: error, unaligned buffer allocation\n", __func__);
		}
	}

	return 0;
}

static inline int tdm2c_ch_tx_low(u8 ch)
{
	u32 max_poll = 0;
	struct tdm2c_ch_info *ch_info = tdm_ch_info[ch];

	dev_dbg(pdev, "%s: Enter, ch%d\n", __func__, ch);

	/* Count tx interrupts */
	tx_int++;

	if (ch_info->txBuffFull[ch_info->txCurrBuff] == BUFF_IS_FULL)
		dev_dbg(pdev, "curr buff full for hw [MMP ok]\n");
	else
		dev_warn(pdev, "curr buf is empty [MMP miss write]\n");

	/* Change buffers */
	ch_info->txCurrBuff = MV_TDM_NEXT_BUFFER(ch_info->txCurrBuff);

	/*
	 * Mark next buff to be transmitted by HW as empty. Give it to the HW
	 * for next frame. The app need to write the data before HW takes it.
	 */
	ch_info->txBuffFull[ch_info->txCurrBuff] = BUFF_IS_EMPTY;
	dev_dbg(pdev, "->%s clear buf(%d) for channel(%d)\n", __func__, ch_info->txCurrBuff, ch);

	/* Poll on SW ownership (single check) */
	dev_dbg(pdev, "start poll for SW ownership\n");
	while (((readb(regs + CH_BUFF_OWN_REG(ch_info->ch) + TX_OWN_BYTE_OFFS) & OWNER_MASK) == OWN_BY_HW)
	       && (max_poll < 2000)) {
		udelay(1);
		max_poll++;
	}
	if (max_poll == 2000) {
		dev_err(pdev, "poll timeout (~2ms)\n");
		return -ETIME;
	}

	dev_dbg(pdev, "ch%d, start tx buff %d\n", ch, ch_info->txCurrBuff);

	/* Set TX buff address (must be 32 byte aligned) */
	writel(ch_info->txBuffPhys[ch_info->txCurrBuff], regs + CH_TX_ADDR_REG(ch_info->ch));

	/* Set HW ownership */
	writeb(OWN_BY_HW, regs + CH_BUFF_OWN_REG(ch_info->ch) + TX_OWN_BYTE_OFFS);

	/* Enable Tx */
	writeb(CH_ENABLE, regs + CH_ENABLE_REG(ch_info->ch) + TX_ENABLE_BYTE_OFFS);

	/* Did we get the required amount of irqs for Tx wakeup ? */
	if (tx_int < MV_TDM_INT_COUNTER)
		return -EBUSY;

	tx_int = 0;
	tx_empty = ch_info->txCurrBuff;

	return 0;
}

static inline int tdm2c_ch_rx_low(u8 ch)
{
	u32 max_poll = 0;
	struct tdm2c_ch_info *ch_info = tdm_ch_info[ch];

	dev_dbg(pdev, "%s: Enter, ch%d\n", __func__, ch);

	if (ch_info->rxFirst)
		ch_info->rxFirst = !FIRST_INT;
	else
		rx_int++;

	if (ch_info->rxBuffFull[ch_info->rxCurrBuff] == BUFF_IS_EMPTY)
		dev_dbg(pdev, "curr buff empty for hw [MMP ok]\n");
	else
		dev_warn(pdev, "curr buf is full [MMP miss read]\n");

	/*
	 * Mark last buff that was received by HW as full. Give next buff to HW for
	 * next frame. The app need to read the data before next irq
	 */
	ch_info->rxBuffFull[ch_info->rxCurrBuff] = BUFF_IS_FULL;

	/* Change buffers */
	ch_info->rxCurrBuff = MV_TDM_NEXT_BUFFER(ch_info->rxCurrBuff);

	/* Poll on SW ownership (single check) */
	dev_dbg(pdev, "start poll for ownership\n");
	while (((readb(regs + CH_BUFF_OWN_REG(ch_info->ch) + RX_OWN_BYTE_OFFS) & OWNER_MASK) == OWN_BY_HW)
	       && (max_poll < 2000)) {
		udelay(1);
		max_poll++;
	}

	if (max_poll == 2000) {
		dev_err(pdev, "poll timeout (~2ms)\n");
		return -ETIME;
	}

	dev_dbg(pdev, "ch%d, start rx buff %d\n", ch, ch_info->rxCurrBuff);

	/* Set RX buff address (must be 32 byte aligned) */
	writel(ch_info->rxBuffPhys[ch_info->rxCurrBuff], regs + CH_RX_ADDR_REG(ch_info->ch));

	/* Set HW ownership */
	writeb(OWN_BY_HW, regs + CH_BUFF_OWN_REG(ch_info->ch) + RX_OWN_BYTE_OFFS);

	/* Enable Rx */
	writeb(CH_ENABLE, regs + CH_ENABLE_REG(ch_info->ch) + RX_ENABLE_BYTE_OFFS);

	/* Did we get the required amount of irqs for Rx wakeup ? */
	if (rx_int < MV_TDM_INT_COUNTER)
		return -EBUSY;

	rx_int = 0;
	rx_full = MV_TDM_PREV_BUFFER(ch_info->rxCurrBuff, 2);
	dev_dbg(pdev, "buff %d is FULL for ch0/1\n", rx_full);

	return 0;
}

static int tdm2c_ch_remove(u8 ch)
{
	struct tdm2c_ch_info *ch_info;
	u8 buff;

	dev_dbg(pdev, "%s: Enter, ch%d\n", __func__, ch);

	if (ch >= MV_TDM2C_TOTAL_CHANNELS) {
		dev_err(pdev, "%s: error, channel(%d) exceeds maximum(%d)\n",
			__func__, ch, MV_TDM2C_TOTAL_CHANNELS);
		return -EINVAL;
	}

	ch_info = tdm_ch_info[ch];

	for (buff = 0; buff < TOTAL_BUFFERS; buff++) {
		dma_free_coherent(pdev, MV_TDM_CH_BUFF_SIZE(pcm_format, tdm_band_mode, factor),
				  ch_info->rxBuffVirt[buff], (dma_addr_t)ch_info->rxBuffPhys[buff]);
		dma_free_coherent(pdev, MV_TDM_CH_BUFF_SIZE(pcm_format, tdm_band_mode, factor),
				  ch_info->txBuffVirt[buff], (dma_addr_t)ch_info->txBuffPhys[buff]);
	}

	kfree(ch_info);

	return 0;
}

static void tdm2c_reset(void)
{
	struct tdm2c_ch_info *ch_info;
	u8 buff, ch;

	dev_dbg(pdev, "%s: Enter, ch%d\n", __func__, ch);

	/* Reset globals */
	rx_int = tx_int = 0;
	rx_full = tx_empty = BUFF_INVALID;

	for (ch = 0; ch < MV_TDM2C_TOTAL_CHANNELS; ch++) {
		ch_info = tdm_ch_info[ch];
		ch_info->rxFirst = FIRST_INT;
		ch_info->txCurrBuff = ch_info->rxCurrBuff = 0;
		for (buff = 0; buff < TOTAL_BUFFERS; buff++) {
			ch_info->rxBuffFull[buff] = BUFF_IS_EMPTY;
			ch_info->txBuffFull[buff] = BUFF_IS_FULL;

		}
	}
}

void __iomem *get_tdm_base(void)
{
	return regs;
}

int tdm2c_init(void __iomem *base, struct device *dev,
	       struct mv_phone_params *tdmParams, struct mv_phone_data *halData)
{
	u8 ch;
	u32 pcm_ctrl_reg, nb_delay = 0, wb_delay = 0;
	u32 ch_delay[4] = { 0, 0, 0, 0 };
	int ret;

	regs = base;
	dev_info(dev, "TDM dual channel device rev 0x%x\n",
		 readl(regs + TDM_REV_REG));

	/* Init globals */
	rx_int = tx_int = 0;
	rx_full = tx_empty = BUFF_INVALID;
	tdm_enable = 0, int_lock = 0;
	spi_mode = halData->spi_mode;
	pcm_format = tdmParams->pcm_format;
	int_rx_count = 0, int_tx_count = 0;
	int_rx0_count = 0, int_tx0_count = 0;
	int_rx1_count = 0, int_tx1_count = 0;
	int_rx0_miss = 0, int_tx0_miss = 0;
	int_rx1_miss = 0, int_tx1_miss = 0;
	pcm_restart_count = 0;
	pdev = dev;

	if (tdmParams->sampling_period > MV_TDM_MAX_SAMPLING_PERIOD)
		/* Use base sample period(10ms) */
		factor = 1;
	else
		factor = (tdmParams->sampling_period / MV_TDM_BASE_SAMPLING_PERIOD);

	/* Extract pcm format & band mode */
	if (pcm_format == MV_PCM_FORMAT_4BYTES) {
		pcm_format = MV_PCM_FORMAT_2BYTES;
		tdm_band_mode = MV_WIDE_BAND;
	} else {
		tdm_band_mode = MV_NARROW_BAND;
	}

	/* Allocate aggregated buffers for data transport */
	dev_dbg(pdev, "allocate %d bytes for aggregated buffer\n",
		MV_TDM_AGGR_BUFF_SIZE(pcm_format, tdm_band_mode, factor));
	rx_aggr_buff_virt = alloc_pages_exact(MV_TDM_AGGR_BUFF_SIZE(pcm_format, tdm_band_mode, factor), GFP_KERNEL);
	tx_aggr_buff_virt = alloc_pages_exact(MV_TDM_AGGR_BUFF_SIZE(pcm_format, tdm_band_mode, factor), GFP_KERNEL);
	if (!rx_aggr_buff_virt || !tx_aggr_buff_virt) {
		dev_err(pdev, "%s: Error malloc failed\n", __func__);
		return -ENOMEM;
	}

	/* Clear buffers */
	memset(rx_aggr_buff_virt, 0, MV_TDM_AGGR_BUFF_SIZE(pcm_format, tdm_band_mode, factor));
	memset(tx_aggr_buff_virt, 0, MV_TDM_AGGR_BUFF_SIZE(pcm_format, tdm_band_mode, factor));

	/* Calculate CH(0/1) Delay Control for narrow/wideband modes */
	for (ch = 0; ch < MV_TDM2C_TOTAL_CHANNELS; ch++) {
		nb_delay = ((tdmParams->pcm_slot[ch] * PCM_SLOT_PCLK) + 1);
		/* Offset required by ZARLINK VE880 SLIC */
		wb_delay = (nb_delay + ((halData->frame_ts / 2) * PCM_SLOT_PCLK));
		ch_delay[ch] = ((nb_delay << CH_RX_DELAY_OFFS) | (nb_delay << CH_TX_DELAY_OFFS));
		ch_delay[(ch + 2)] = ((wb_delay << CH_RX_DELAY_OFFS) | (wb_delay << CH_TX_DELAY_OFFS));
	}

	/* Enable TDM/SPI interface */
	mv_phone_reset_bit(regs + TDM_SPI_MUX_REG, 0x00000001);
	/* Interrupt cause is not clear on read */
	writel(CLEAR_ON_ZERO, regs + INT_RESET_SELECT_REG);
	/* All interrupt bits latched in status */
	writel(0x3ffff, regs + INT_EVENT_MASK_REG);
	/* Disable interrupts */
	writel(0, regs + INT_STATUS_MASK_REG);
	/* Clear int status register */
	writel(0, regs + INT_STATUS_REG);

	/* Bypass clock divider - PCM PCLK freq */
	writel(PCM_DIV_PASS, regs + PCM_CLK_RATE_DIV_REG);

	/* Padding on Rx completion */
	writel(0, regs + DUMMY_RX_WRITE_DATA_REG);
	writeb(readl(regs + SPI_GLOBAL_CTRL_REG) | SPI_GLOBAL_ENABLE, regs + SPI_GLOBAL_CTRL_REG);
	/* SPI SCLK freq */
	writel(SPI_CLK_2MHZ, regs + SPI_CLK_PRESCALAR_REG);
	/* Number of timeslots (PCLK) */
	writel((u32)halData->frame_ts, regs + FRAME_TIMESLOT_REG);

	if (tdm_band_mode == MV_NARROW_BAND) {
		pcm_ctrl_reg = (CONFIG_PCM_CRTL | (((u8)pcm_format - 1) << PCM_SAMPLE_SIZE_OFFS));

		if (use_pclk_external)
			pcm_ctrl_reg |= MASTER_PCLK_EXTERNAL;

		/* PCM configuration */
		writel(pcm_ctrl_reg, regs + PCM_CTRL_REG);
		/* CH0 delay control register */
		writel(ch_delay[0], regs + CH_DELAY_CTRL_REG(0));
		/* CH1 delay control register */
		writel(ch_delay[1], regs + CH_DELAY_CTRL_REG(1));
	} else {		/* MV_WIDE_BAND */

		pcm_ctrl_reg = (CONFIG_WB_PCM_CRTL | (((u8)pcm_format - 1) << PCM_SAMPLE_SIZE_OFFS));

		if (use_pclk_external)
			pcm_ctrl_reg |= MASTER_PCLK_EXTERNAL;

		/* PCM configuration - WB support */
		writel(pcm_ctrl_reg, regs + PCM_CTRL_REG);
		/* CH0 delay control register */
		writel(ch_delay[0], regs + CH_DELAY_CTRL_REG(0));
		/* CH1 delay control register */
		writel(ch_delay[1], regs + CH_DELAY_CTRL_REG(1));
		/* CH0 WB delay control register */
		writel(ch_delay[2], regs + CH_WB_DELAY_CTRL_REG(0));
		/* CH1 WB delay control register */
		writel(ch_delay[3], regs + CH_WB_DELAY_CTRL_REG(1));
	}

	/* Issue reset to codec(s) */
	dev_dbg(pdev, "resetting voice unit(s)\n");
	writel(0, regs + MISC_CTRL_REG);
	mdelay(1);
	writel(1, regs + MISC_CTRL_REG);

	if (spi_mode) {
		/* Configure TDM to work in daisy chain mode */
		tdm2c_daisy_chain_mode_set();
	}

	/* Initialize all HW units */
	for (ch = 0; ch < MV_TDM2C_TOTAL_CHANNELS; ch++) {
		ret = tdm2c_ch_init(ch);
		if (ret) {
			dev_err(pdev, "tdm2c_ch_init(%d) failed !\n", ch);
			return ret;
		}
	}

	/* Enable SLIC/DAA interrupt detection(before pcm is active) */
	writel((readl(regs + INT_STATUS_MASK_REG) | TDM_INT_SLIC), regs + INT_STATUS_MASK_REG);

	return 0;
}

void tdm2c_release(void)
{
	u8 ch;

	/* Free Rx/Tx aggregated buffers */
	free_pages_exact(rx_aggr_buff_virt, MV_TDM_AGGR_BUFF_SIZE(pcm_format, tdm_band_mode, factor));
	free_pages_exact(tx_aggr_buff_virt, MV_TDM_AGGR_BUFF_SIZE(pcm_format, tdm_band_mode, factor));

	/* Release HW channel resources */
	for (ch = 0; ch < MV_TDM2C_TOTAL_CHANNELS; ch++)
		tdm2c_ch_remove(ch);

	/* Disable TDM/SPI interface */
	mv_phone_set_bit(regs + TDM_SPI_MUX_REG, 0x00000001);
}

void tdm2c_pcm_start(void)
{
	struct tdm2c_ch_info *ch_info;
	u8 ch;

	/* TDM is enabled */
	tdm_enable = 1;
	int_lock = 0;
	chan_stop_count = 0;
	tdm2c_reset();

	for (ch = 0; ch < MV_TDM2C_TOTAL_CHANNELS; ch++) {
		ch_info = tdm_ch_info[ch];

		/* Set Tx buff */
		writel(ch_info->txBuffPhys[ch_info->txCurrBuff], regs + CH_TX_ADDR_REG(ch));
		writeb(OWN_BY_HW, regs + CH_BUFF_OWN_REG(ch) + TX_OWN_BYTE_OFFS);

		/* Set Rx buff */
		writel(ch_info->rxBuffPhys[ch_info->rxCurrBuff], regs + CH_RX_ADDR_REG(ch));
		writeb(OWN_BY_HW, regs + CH_BUFF_OWN_REG(ch) + RX_OWN_BYTE_OFFS);

	}

	/* Enable Tx */
	writeb(CH_ENABLE, regs + CH_ENABLE_REG(0) + TX_ENABLE_BYTE_OFFS);
	writeb(CH_ENABLE, regs + CH_ENABLE_REG(1) + TX_ENABLE_BYTE_OFFS);

	/* Enable Rx */
	writeb(CH_ENABLE, regs + CH_ENABLE_REG(0) + RX_ENABLE_BYTE_OFFS);
	writeb(CH_ENABLE, regs + CH_ENABLE_REG(1) + RX_ENABLE_BYTE_OFFS);

	/* Enable Tx interrupts */
	writel(readl(regs + INT_STATUS_REG) & (~(TDM_INT_TX(0) | TDM_INT_TX(1))), regs + INT_STATUS_REG);
	writel((readl(regs + INT_STATUS_MASK_REG) | TDM_INT_TX(0) | TDM_INT_TX(1)), regs + INT_STATUS_MASK_REG);

	/* Enable Rx interrupts */
	writel((readl(regs + INT_STATUS_REG) & (~(TDM_INT_RX(0) | TDM_INT_RX(1)))), regs + INT_STATUS_REG);
	writel((readl(regs + INT_STATUS_MASK_REG) | TDM_INT_RX(0) | TDM_INT_RX(1)), regs + INT_STATUS_MASK_REG);
}

void tdm2c_pcm_stop(void)
{
	tdm_enable = 0;

	tdm2c_reset();
}

int tdm2c_tx(u8 *tdm_tx_buff)
{
	struct tdm2c_ch_info *ch_info;
	u8 ch;
	u8 *tx_buff;

	/* Sanity check */
	if (tdm_tx_buff != tx_aggr_buff_virt) {
		dev_err(pdev, "%s: Error, invalid Tx buffer !!!\n", __func__);
		return -EINVAL;
	}

	if (!tdm_enable) {
		dev_err(pdev, "%s: Error, no active Tx channels are available\n", __func__);
		return -EINVAL;
	}

	if (tx_empty == BUFF_INVALID) {
		dev_err(pdev, "%s: Tx not ready\n", __func__);
		return -EINVAL;
	}

	for (ch = 0; ch < MV_TDM2C_TOTAL_CHANNELS; ch++) {
		ch_info = tdm_ch_info[ch];
		dev_dbg(pdev, "ch%d: fill buf %d with %d bytes\n",
			ch, tx_empty,
			MV_TDM_CH_BUFF_SIZE(pcm_format, tdm_band_mode, factor));
		ch_info->txBuffFull[tx_empty] = BUFF_IS_FULL;
		tx_buff = tdm_tx_buff + (ch * MV_TDM_CH_BUFF_SIZE(pcm_format, tdm_band_mode, factor));

		/* Copy data from voice engine buffer to DMA */
		memcpy(ch_info->txBuffVirt[tx_empty], tx_buff,
		       MV_TDM_CH_BUFF_SIZE(pcm_format, tdm_band_mode, factor));
	}

	tx_empty = BUFF_INVALID;

	return 0;
}

int tdm2c_rx(u8 *tdm_rx_buff)
{
	struct tdm2c_ch_info *ch_info;
	u8 ch;
	u8 *rx_buff;

	/* Sanity check */
	if (tdm_rx_buff != rx_aggr_buff_virt) {
		dev_err(pdev, "%s: invalid Rx buffer !!!\n", __func__);
		return -EINVAL;
	}

	if (!tdm_enable) {
		dev_err(pdev, "%s: Error, no active Rx channels are available\n", __func__);
		return -EINVAL;
	}

	if (rx_full == BUFF_INVALID) {
		dev_err(pdev, "%s: Rx not ready\n", __func__);
		return -EINVAL;
	}

	for (ch = 0; ch < MV_TDM2C_TOTAL_CHANNELS; ch++) {
		ch_info = tdm_ch_info[ch];
		ch_info->rxBuffFull[rx_full] = BUFF_IS_EMPTY;
		dev_dbg(pdev, "%s get Rx buffer(%d) for channel(%d)\n",
			__func__, rx_full, ch);
		rx_buff = tdm_rx_buff + (ch * MV_TDM_CH_BUFF_SIZE(pcm_format, tdm_band_mode, factor));

		/* Copy data from DMA to voice engine buffer */
		memcpy(rx_buff, ch_info->rxBuffVirt[rx_full],
		       MV_TDM_CH_BUFF_SIZE(pcm_format, tdm_band_mode, factor));
	}

	rx_full = BUFF_INVALID;

	return 0;
}

int tdm2c_pcm_stop_int_miss(void)
{
	u32 status_reg, mask_reg, status_stop_int, status_mask = 0, int_mask = 0;

	status_reg = readl(regs + INT_STATUS_REG);
	mask_reg = readl(regs + INT_STATUS_MASK_REG);

	/* Refer only to unmasked bits */
	status_stop_int = status_reg & mask_reg;

	if (status_stop_int & TX_UNDERFLOW_BIT(1)) {
		status_mask |= TX_UNDERFLOW_BIT(1);
		int_mask |= TDM_INT_TX(1);
	}

	if (status_stop_int & TX_UNDERFLOW_BIT(0)) {
		status_mask |= TX_UNDERFLOW_BIT(0);
		int_mask |= TDM_INT_TX(0);
	}

	if (status_stop_int & RX_OVERFLOW_BIT(1)) {
		status_mask |= RX_OVERFLOW_BIT(1);
		int_mask |= TDM_INT_RX(1);
	}

	if (status_stop_int & RX_OVERFLOW_BIT(0)) {
		status_mask |= TX_UNDERFLOW_BIT(0);
		int_mask |= TDM_INT_RX(0);
	}

	if (int_mask != 0) {
		dev_err(pdev, "Stop Interrupt missing found STATUS=%x, MASK=%x\n", status_reg, mask_reg);
		writel(~(status_mask), regs + INT_STATUS_REG);
		writel(readl(regs + INT_STATUS_MASK_REG) & (~(int_mask)),
		       regs + INT_STATUS_MASK_REG);

		return -EINVAL;
	}

	return 0;
}

/* Low level TDM interrupt service routine */
int tdm2c_intr_low(struct mv_phone_intr_info *tdm_intr_info)
{
	u32 status_reg, mask_reg, status_and_mask;
	int ret = 0;
	int int_tx_miss = -1;
	int int_rx_miss = -1;
	u8 ch;

	/* Read Status & mask registers */
	status_reg = readl(regs + INT_STATUS_REG);
	mask_reg = readl(regs + INT_STATUS_MASK_REG);
	dev_dbg(pdev, "CAUSE(0x%x), MASK(0x%x)\n", status_reg, mask_reg);

	/* Refer only to unmasked bits */
	status_and_mask = status_reg & mask_reg;

	/* Reset params */
	tdm_intr_info->tdm_rx_buff = NULL;
	tdm_intr_info->tdm_tx_buff = NULL;
	tdm_intr_info->int_type = MV_EMPTY_INT;
	tdm_intr_info->cs = MV_TDM_CS;

	/* Handle SLIC/DAA int */
	if (status_and_mask & SLIC_INT_BIT) {
		dev_dbg(pdev, "Phone interrupt !!!\n");
		tdm_intr_info->int_type |= MV_PHONE_INT;
	}

	if (status_and_mask & DMA_ABORT_BIT) {
		dev_err(pdev, "DMA data abort. Address: 0x%08x, Info: 0x%08x\n",
			readl(regs + DMA_ABORT_ADDR_REG),
			readl(regs + DMA_ABORT_INFO_REG));
		tdm_intr_info->int_type |= MV_DMA_ERROR_INT;
	}

	for (ch = 0; ch < MV_TDM2C_TOTAL_CHANNELS; ch++) {

		/* Give next buff to TDM and set curr buff as empty */
		if ((status_and_mask & TX_BIT(ch)) && tdm_enable && !int_lock) {
			dev_dbg(pdev, "Tx interrupt(ch%d)\n", ch);

			int_tx_count++;
			if (ch == 0) {
				int_tx0_count++;
				if (int_tx0_count <= int_tx1_count) {
					int_tx_miss = 0;
					int_tx0_miss++;
				}
			} else {
				int_tx1_count++;
				if (int_tx1_count < int_tx0_count) {
					int_tx_miss = 1;
					int_tx1_miss++;
				}
			}

			/* 0 -> Tx is done for both channels */
			if (tdm2c_ch_tx_low(ch) == 0) {
				dev_dbg(pdev, "Assign Tx aggregate buffer for further processing\n");
				tdm_intr_info->tdm_tx_buff = tx_aggr_buff_virt;
				tdm_intr_info->int_type |= MV_TX_INT;
			}
		}
	}

	for (ch = 0; ch < MV_TDM2C_TOTAL_CHANNELS; ch++) {

		if ((status_and_mask & RX_BIT(ch)) && tdm_enable && !int_lock) {
			dev_dbg(pdev, "Rx interrupt(ch%d)\n", ch);

			int_rx_count++;
			if (ch == 0) {
				int_rx0_count++;
				if (int_rx0_count <= int_rx1_count) {
					int_rx_miss = 0;
					int_rx0_miss++;
				}
			} else {
				int_rx1_count++;
				if (int_rx1_count < int_rx0_count) {
					int_rx_miss = 1;
					int_rx1_miss++;
				}
			}

			/* 0 -> Rx is done for both channels */
			if (tdm2c_ch_rx_low(ch) == 0) {
				dev_dbg(pdev, "Assign Rx aggregate buffer for further processing\n");
				tdm_intr_info->tdm_rx_buff = rx_aggr_buff_virt;
				tdm_intr_info->int_type |= MV_RX_INT;
			}
		}
	}

	for (ch = 0; ch < MV_TDM2C_TOTAL_CHANNELS; ch++) {

		if (status_and_mask & TX_UNDERFLOW_BIT(ch)) {

			dev_dbg(pdev, "Tx underflow(ch%d) - checking for root cause...\n",
				    ch);
			if (tdm_enable) {
				dev_dbg(pdev, "Tx underflow ERROR\n");
				tdm_intr_info->int_type |= MV_TX_ERROR_INT;
				if (!(status_and_mask & TX_BIT(ch))) {
					ret = -1;
					/* 0 -> Tx is done for both channels */
					if (tdm2c_ch_tx_low(ch) == 0) {
						dev_dbg(pdev, "Assign Tx aggregate buffer for further processing\n");
						tdm_intr_info->tdm_tx_buff = tx_aggr_buff_virt;
						tdm_intr_info->int_type |= MV_TX_INT;
					}
				}
			} else {
				dev_dbg(pdev, "Expected Tx underflow(not an error)\n");
				tdm_intr_info->int_type |= MV_CHAN_STOP_INT;
				/* Update number of channels already stopped */
				tdm_intr_info->data = ++chan_stop_count;
				writel(readl(regs + INT_STATUS_MASK_REG) & (~(TDM_INT_TX(ch))),
				       regs + INT_STATUS_MASK_REG);
			}
		}


		if (status_and_mask & RX_OVERFLOW_BIT(ch)) {
			dev_dbg(pdev, "Rx overflow(ch%d) - checking for root cause...\n", ch);
			if (tdm_enable) {
				dev_dbg(pdev, "Rx overflow ERROR\n");
				tdm_intr_info->int_type |= MV_RX_ERROR_INT;
				if (!(status_and_mask & RX_BIT(ch))) {
					ret = -1;
					/* 0 -> Rx is done for both channels */
					if (tdm2c_ch_rx_low(ch) == 0) {
						dev_dbg(pdev, "Assign Rx aggregate buffer for further processing\n");
						tdm_intr_info->tdm_rx_buff = rx_aggr_buff_virt;
						tdm_intr_info->int_type |= MV_RX_INT;
					}
				}
			} else {
				dev_dbg(pdev, "Expected Rx overflow(not an error)\n");
				tdm_intr_info->int_type |= MV_CHAN_STOP_INT;
				tdm_intr_info->data = ++chan_stop_count; /* Update number of channels already stopped */
				writel(readl(regs + INT_STATUS_MASK_REG) & (~(TDM_INT_RX(ch))),
				       regs + INT_STATUS_MASK_REG);
			}
		}
	}

	/* clear TDM interrupts */
	writel(~status_reg, regs + INT_STATUS_REG);

	/* Check if interrupt was missed -> restart */
	if  (int_tx_miss != -1)  {
		dev_err(pdev, "Missing Tx Interrupt Detected ch%d!!!\n", int_tx_miss);
		if (int_tx_miss)
			int_tx1_count = int_tx0_count;
		else
			int_tx0_count  = (int_tx1_count + 1);
		ret = -1;
	}

	if  (int_rx_miss != -1)  {
		dev_err(pdev, "Missing Rx Interrupt Detected ch%d!!!\n", int_rx_miss);
		if (int_rx_miss)
			int_rx1_count = int_rx0_count;
		else
			int_rx0_count  = (int_rx1_count + 1);
		ret = -1;
	}

	if (ret == -1) {
		int_lock = 1;
		pcm_restart_count++;
	}

	return ret;
}

void tdm2c_intr_enable(void)
{
	writel((readl(regs + INT_STATUS_MASK_REG) | TDM_INT_SLIC),
	       regs + INT_STATUS_MASK_REG);
}

void tdm2c_intr_disable(void)
{
	u32 val = ~TDM_INT_SLIC;

	writel((readl(regs + INT_STATUS_MASK_REG) & val),
	       regs + INT_STATUS_MASK_REG);
}

void tdm2c_pcm_if_reset(void)
{
	/* SW PCM reset assert */
	mv_phone_reset_bit(regs + TDM_MISC_REG, 0x00000001);

	mdelay(10);

	/* SW PCM reset de-assert */
	mv_phone_set_bit(regs + TDM_MISC_REG, 0x00000001);

	/* Wait a bit more - might be fine tuned */
	mdelay(50);

	dev_dbg(pdev, "%s: Exit\n", __func__);
}

/* Debug routines */
void tdm2c_reg_dump(u32 offset)
{
	dev_info(pdev, "0x%05x: %08x\n", offset, readl(regs + offset));
}

void tdm2c_regs_dump(void)
{
	u8 i;
	struct tdm2c_ch_info *ch_info;

	dev_info(pdev, "TDM Control:\n");
	tdm2c_reg_dump(TDM_SPI_MUX_REG);
	tdm2c_reg_dump(INT_RESET_SELECT_REG);
	tdm2c_reg_dump(INT_STATUS_MASK_REG);
	tdm2c_reg_dump(INT_STATUS_REG);
	tdm2c_reg_dump(INT_EVENT_MASK_REG);
	tdm2c_reg_dump(PCM_CTRL_REG);
	tdm2c_reg_dump(TIMESLOT_CTRL_REG);
	tdm2c_reg_dump(PCM_CLK_RATE_DIV_REG);
	tdm2c_reg_dump(FRAME_TIMESLOT_REG);
	tdm2c_reg_dump(DUMMY_RX_WRITE_DATA_REG);
	tdm2c_reg_dump(MISC_CTRL_REG);
	dev_info(pdev, "TDM Channel Control:\n");
	for (i = 0; i < MV_TDM2C_TOTAL_CHANNELS; i++) {
		tdm2c_reg_dump(CH_DELAY_CTRL_REG(i));
		tdm2c_reg_dump(CH_SAMPLE_REG(i));
		tdm2c_reg_dump(CH_DBG_REG(i));
		tdm2c_reg_dump(CH_TX_CUR_ADDR_REG(i));
		tdm2c_reg_dump(CH_RX_CUR_ADDR_REG(i));
		tdm2c_reg_dump(CH_ENABLE_REG(i));
		tdm2c_reg_dump(CH_BUFF_OWN_REG(i));
		tdm2c_reg_dump(CH_TX_ADDR_REG(i));
		tdm2c_reg_dump(CH_RX_ADDR_REG(i));
	}
	dev_info(pdev, "TDM interrupts:\n");
	tdm2c_reg_dump(INT_EVENT_MASK_REG);
	tdm2c_reg_dump(INT_STATUS_MASK_REG);
	tdm2c_reg_dump(INT_STATUS_REG);
	for (i = 0; i < MV_TDM2C_TOTAL_CHANNELS; i++) {
		dev_info(pdev, "ch%d info:\n", i);
		ch_info = tdm_ch_info[i];
		dev_info(pdev, "RX buffs:\n");
		dev_info(pdev, "buff0: virt=%p phys=%p\n",
			 ch_info->rxBuffVirt[0], (u32 *) (ch_info->rxBuffPhys[0]));
		dev_info(pdev, "buff1: virt=%p phys=%p\n",
			 ch_info->rxBuffVirt[1], (u32 *) (ch_info->rxBuffPhys[1]));
		dev_info(pdev, "TX buffs:\n");
		dev_info(pdev, "buff0: virt=%p phys=%p\n",
			 ch_info->txBuffVirt[0], (u32 *) (ch_info->txBuffPhys[0]));
		dev_info(pdev, "buff1: virt=%p phys=%p\n",
			 ch_info->txBuffVirt[1], (u32 *) (ch_info->txBuffPhys[1]));
	}
}

#ifdef CONFIG_MV_TDM_EXT_STATS
void tdm2c_ext_stats_get(struct mv_phone_extended_stats *tdmExtStats)
{
	tdmExtStats->int_rx_count = int_rx_count;
	tdmExtStats->int_tx_count = int_tx_count;
	tdmExtStats->int_rx0_count = int_rx0_count;
	tdmExtStats->int_tx0_count = int_tx0_count;
	tdmExtStats->int_rx1_count = int_rx1_count;
	tdmExtStats->int_tx1_count = int_tx1_count;
	tdmExtStats->int_rx0_miss = int_rx0_miss;
	tdmExtStats->int_tx0_miss = int_tx0_miss;
	tdmExtStats->int_rx1_miss = int_rx1_miss;
	tdmExtStats->int_tx1_miss = int_tx1_miss;
	tdmExtStats->pcm_restart_count = pcm_restart_count;
}
#endif

/* Initialize decoding windows */
int tdm2c_set_mbus_windows(struct device *dev, void __iomem *regs,
			   const struct mbus_dram_target_info *dram)
{
	int i;

	if (!dram) {
		dev_err(dev, "no mbus dram info\n");
		return -EINVAL;
	}

	for (i = 0; i < TDM_MBUS_MAX_WIN; i++) {
		writel(0, regs + TDM_WIN_CTRL_REG(i));
		writel(0, regs + TDM_WIN_BASE_REG(i));
	}

	for (i = 0; i < dram->num_cs; i++) {
		const struct mbus_dram_window *cs = dram->cs + i;

		/* Write size, attributes and target id to control register */
		writel(((cs->size - 1) & 0xffff0000) |
			(cs->mbus_attr << 8) |
			(dram->mbus_dram_target_id << 4) | 1,
			regs + TDM_WIN_CTRL_REG(i));
		/* Write base address to base register */
		writel(cs->base, regs + TDM_WIN_BASE_REG(i));
	}

	return 0;
}
