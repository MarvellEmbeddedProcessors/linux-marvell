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

/* Main TDM structure definition */
struct tdm2c_dev {
	/* Resources */
	void __iomem *regs;
	struct device *dev;

	/* Buffers */
	u8 *rx_aggr_buff_virt;
	u8 *tx_aggr_buff_virt;

	/* Flags and counters */
	u16 rx_full;
	u16 tx_empty;
	u8 rx_int;
	u8 tx_int;
	bool enable;
	bool int_lock;
	int chan_stop_count;

	/* Parameters */
	u8 factor;
	enum mv_phone_pcm_format pcm_format;
	enum mv_phone_band_mode band_mode;

	/* Channels' data */
	struct tdm2c_ch_info *ch_info[MV_TDM2C_TOTAL_CHANNELS];

	/* Statistics */
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

struct tdm2c_dev *tdm2c;

static void tdm2c_daisy_chain_mode_set(void)
{
	while ((readl(tdm2c->regs + SPI_CTRL_REG) & SPI_STAT_MASK) == SPI_ACTIVE)
		continue;
	writel((0x80 << 8) | 0, tdm2c->regs + SPI_CODEC_CMD_LO_REG);
	writel(TRANSFER_BYTES(2) | ENDIANNESS_MSB_MODE | WR_MODE | CLK_SPEED_LO_DIV,
	       tdm2c->regs + SPI_CODEC_CTRL_REG);
	writel(readl(tdm2c->regs + SPI_CTRL_REG) | SPI_ACTIVE, tdm2c->regs + SPI_CTRL_REG);
	/* Poll for ready indication */
	while ((readl(tdm2c->regs + SPI_CTRL_REG) & SPI_STAT_MASK) == SPI_ACTIVE)
		continue;

	dev_dbg(tdm2c->dev, "%s: Exit\n", __func__);
}

static int tdm2c_ch_init(u8 ch)
{
	struct tdm2c_ch_info *ch_info;
	u32 buff;

	dev_dbg(tdm2c->dev, "%s: Enter, ch%d\n", __func__, ch);

	if (ch >= MV_TDM2C_TOTAL_CHANNELS) {
		dev_err(tdm2c->dev, "%s: error, channel(%d) exceeds maximum(%d)\n",
			__func__, ch, MV_TDM2C_TOTAL_CHANNELS);
		return -EINVAL;
	}

	tdm2c->ch_info[ch] = kmalloc(sizeof(struct tdm2c_ch_info), GFP_ATOMIC);
	if (!tdm2c->ch_info) {
		dev_err(tdm2c->dev, "%s: error malloc failed\n", __func__);
		return -ENOMEM;
	}

	ch_info = tdm2c->ch_info[ch];
	ch_info->ch = ch;

	/* Per channel TDM init */
	/* Disable channel (enable in pcm start) */
	writel(CH_DISABLE, tdm2c->regs + CH_ENABLE_REG(ch));
	/* Set total samples and int sample */
	writel(CONFIG_CH_SAMPLE(tdm2c->band_mode, tdm2c->factor), tdm2c->regs + CH_SAMPLE_REG(ch));

	for (buff = 0; buff < TOTAL_BUFFERS; buff++) {
		/* Buffers must be 32B aligned */
		ch_info->rx_buff_virt[buff] = dma_alloc_coherent(tdm2c->dev,
				MV_TDM_CH_BUFF_SIZE(tdm2c->pcm_format, tdm2c->band_mode, tdm2c->factor),
				&(ch_info->rx_buff_phys[buff]), GFP_KERNEL);
		ch_info->rx_buff_full[buff] = BUFF_IS_EMPTY;

		ch_info->tx_buff_virt[buff] = dma_alloc_coherent(tdm2c->dev,
				MV_TDM_CH_BUFF_SIZE(tdm2c->pcm_format, tdm2c->band_mode, tdm2c->factor),
				&(ch_info->tx_buff_phys[buff]), GFP_KERNEL);
		ch_info->tx_buff_empty[buff] = BUFF_IS_FULL;

		memset(ch_info->tx_buff_virt[buff], 0,
				MV_TDM_CH_BUFF_SIZE(tdm2c->pcm_format, tdm2c->band_mode, tdm2c->factor));

		if (((ulong) ch_info->rx_buff_virt[buff] | ch_info->rx_buff_phys[buff] |
		     (ulong) ch_info->tx_buff_virt[buff] | ch_info->tx_buff_phys[buff]) & 0x1f) {
			dev_err(tdm2c->dev, "%s: error, unaligned buffer allocation\n", __func__);
		}
	}

	return 0;
}

static inline int tdm2c_ch_tx_low(u8 ch)
{
	u32 max_poll = 0;
	struct tdm2c_ch_info *ch_info = tdm2c->ch_info[ch];

	dev_dbg(tdm2c->dev, "%s: Enter, ch%d\n", __func__, ch);

	/* Count tx interrupts */
	tdm2c->tx_int++;

	if (ch_info->tx_buff_empty[ch_info->tx_curr_buff] == BUFF_IS_FULL)
		dev_dbg(tdm2c->dev, "curr buff full for hw [MMP ok]\n");
	else
		dev_warn(tdm2c->dev, "curr buf is empty [MMP miss write]\n");

	/* Change buffers */
	ch_info->tx_curr_buff = MV_TDM_NEXT_BUFFER(ch_info->tx_curr_buff);

	/*
	 * Mark next buff to be transmitted by HW as empty. Give it to the HW
	 * for next frame. The app need to write the data before HW takes it.
	 */
	ch_info->tx_buff_empty[ch_info->tx_curr_buff] = BUFF_IS_EMPTY;
	dev_dbg(tdm2c->dev, "->%s clear buf(%d) for channel(%d)\n", __func__, ch_info->tx_curr_buff, ch);

	/* Poll on SW ownership (single check) */
	dev_dbg(tdm2c->dev, "start poll for SW ownership\n");
	while (((readb(tdm2c->regs + CH_BUFF_OWN_REG(ch_info->ch) + TX_OWN_BYTE_OFFS) & OWNER_MASK) == OWN_BY_HW)
	       && (max_poll < 2000)) {
		udelay(1);
		max_poll++;
	}
	if (max_poll == 2000) {
		dev_err(tdm2c->dev, "poll timeout (~2ms)\n");
		return -ETIME;
	}

	dev_dbg(tdm2c->dev, "ch%d, start tx buff %d\n", ch, ch_info->tx_curr_buff);

	/* Set TX buff address (must be 32 byte aligned) */
	writel(ch_info->tx_buff_phys[ch_info->tx_curr_buff], tdm2c->regs + CH_TX_ADDR_REG(ch_info->ch));

	/* Set HW ownership */
	writeb(OWN_BY_HW, tdm2c->regs + CH_BUFF_OWN_REG(ch_info->ch) + TX_OWN_BYTE_OFFS);

	/* Enable Tx */
	writeb(CH_ENABLE, tdm2c->regs + CH_ENABLE_REG(ch_info->ch) + TX_ENABLE_BYTE_OFFS);

	/* Did we get the required amount of irqs for Tx wakeup ? */
	if (tdm2c->tx_int < MV_TDM_INT_COUNTER)
		return -EBUSY;

	tdm2c->tx_int = 0;
	tdm2c->tx_empty = ch_info->tx_curr_buff;

	return 0;
}

static inline int tdm2c_ch_rx_low(u8 ch)
{
	u32 max_poll = 0;
	struct tdm2c_ch_info *ch_info = tdm2c->ch_info[ch];

	dev_dbg(tdm2c->dev, "%s: Enter, ch%d\n", __func__, ch);

	if (ch_info->rx_first)
		ch_info->rx_first = !FIRST_INT;
	else
		tdm2c->rx_int++;

	if (ch_info->rx_buff_full[ch_info->rx_curr_buff] == BUFF_IS_EMPTY)
		dev_dbg(tdm2c->dev, "curr buff empty for hw [MMP ok]\n");
	else
		dev_warn(tdm2c->dev, "curr buf is full [MMP miss read]\n");

	/*
	 * Mark last buff that was received by HW as full. Give next buff to HW for
	 * next frame. The app need to read the data before next irq
	 */
	ch_info->rx_buff_full[ch_info->rx_curr_buff] = BUFF_IS_FULL;

	/* Change buffers */
	ch_info->rx_curr_buff = MV_TDM_NEXT_BUFFER(ch_info->rx_curr_buff);

	/* Poll on SW ownership (single check) */
	dev_dbg(tdm2c->dev, "start poll for ownership\n");
	while (((readb(tdm2c->regs + CH_BUFF_OWN_REG(ch_info->ch) + RX_OWN_BYTE_OFFS) & OWNER_MASK) == OWN_BY_HW)
	       && (max_poll < 2000)) {
		udelay(1);
		max_poll++;
	}

	if (max_poll == 2000) {
		dev_err(tdm2c->dev, "poll timeout (~2ms)\n");
		return -ETIME;
	}

	dev_dbg(tdm2c->dev, "ch%d, start rx buff %d\n", ch, ch_info->rx_curr_buff);

	/* Set RX buff address (must be 32 byte aligned) */
	writel(ch_info->rx_buff_phys[ch_info->rx_curr_buff], tdm2c->regs + CH_RX_ADDR_REG(ch_info->ch));

	/* Set HW ownership */
	writeb(OWN_BY_HW, tdm2c->regs + CH_BUFF_OWN_REG(ch_info->ch) + RX_OWN_BYTE_OFFS);

	/* Enable Rx */
	writeb(CH_ENABLE, tdm2c->regs + CH_ENABLE_REG(ch_info->ch) + RX_ENABLE_BYTE_OFFS);

	/* Did we get the required amount of irqs for Rx wakeup ? */
	if (tdm2c->rx_int < MV_TDM_INT_COUNTER)
		return -EBUSY;

	tdm2c->rx_int = 0;
	tdm2c->rx_full = MV_TDM_PREV_BUFFER(ch_info->rx_curr_buff, 2);
	dev_dbg(tdm2c->dev, "buff %d is FULL for ch0/1\n", tdm2c->rx_full);

	return 0;
}

static int tdm2c_ch_remove(u8 ch)
{
	struct tdm2c_ch_info *ch_info;
	u8 buff;

	dev_dbg(tdm2c->dev, "%s: Enter, ch%d\n", __func__, ch);

	if (ch >= MV_TDM2C_TOTAL_CHANNELS) {
		dev_err(tdm2c->dev, "%s: error, channel(%d) exceeds maximum(%d)\n",
			__func__, ch, MV_TDM2C_TOTAL_CHANNELS);
		return -EINVAL;
	}

	ch_info = tdm2c->ch_info[ch];

	for (buff = 0; buff < TOTAL_BUFFERS; buff++) {
		dma_free_coherent(tdm2c->dev, MV_TDM_CH_BUFF_SIZE(tdm2c->pcm_format, tdm2c->band_mode, tdm2c->factor),
				  ch_info->rx_buff_virt[buff], (dma_addr_t)ch_info->rx_buff_phys[buff]);
		dma_free_coherent(tdm2c->dev, MV_TDM_CH_BUFF_SIZE(tdm2c->pcm_format, tdm2c->band_mode, tdm2c->factor),
				  ch_info->tx_buff_virt[buff], (dma_addr_t)ch_info->tx_buff_phys[buff]);
	}

	kfree(ch_info);

	return 0;
}

static void tdm2c_reset(void)
{
	struct tdm2c_ch_info *ch_info;
	u8 buff, ch;

	dev_dbg(tdm2c->dev, "%s: Enter, ch%d\n", __func__, ch);

	/* Reset globals */
	tdm2c->rx_int = 0;
	tdm2c->tx_int = 0;
	tdm2c->rx_full = BUFF_INVALID;
	tdm2c->tx_empty = BUFF_INVALID;

	for (ch = 0; ch < MV_TDM2C_TOTAL_CHANNELS; ch++) {
		ch_info = tdm2c->ch_info[ch];
		ch_info->rx_first = FIRST_INT;
		ch_info->tx_curr_buff = ch_info->rx_curr_buff = 0;
		for (buff = 0; buff < TOTAL_BUFFERS; buff++) {
			ch_info->rx_buff_full[buff] = BUFF_IS_EMPTY;
			ch_info->tx_buff_empty[buff] = BUFF_IS_FULL;

		}
	}
}

int tdm2c_init(void __iomem *base, struct device *dev,
	       struct mv_phone_params *tdm_params, enum mv_phone_frame_ts frame_ts,
	       enum mv_phone_spi_mode spi_mode, bool use_pclk_external)
{
	u8 ch;
	u32 pcm_ctrl_reg, nb_delay = 0, wb_delay = 0;
	u32 ch_delay[4] = { 0, 0, 0, 0 };
	int ret;

	/* Initialize or reset main structure */
	if (!tdm2c) {
		tdm2c = devm_kzalloc(dev, sizeof(struct tdm2c_dev), GFP_KERNEL);
		if (!tdm2c)
			return -ENOMEM;
	} else {
		memset(tdm2c, 0,  sizeof(struct tdm2c_dev));
	}

	/* Initialize remaining parameters */
	tdm2c->regs = base;
	tdm2c->pcm_format = tdm_params->pcm_format;
	tdm2c->rx_full = BUFF_INVALID;
	tdm2c->tx_empty = BUFF_INVALID;
	tdm2c->dev = dev;

	dev_info(dev, "TDM dual channel device rev 0x%x\n",
		 readl(tdm2c->regs + TDM_REV_REG));

	if (tdm_params->sampling_period > MV_TDM_MAX_SAMPLING_PERIOD)
		/* Use base sample period(10ms) */
		tdm2c->factor = 1;
	else
		tdm2c->factor = (tdm_params->sampling_period / MV_TDM_BASE_SAMPLING_PERIOD);

	/* Extract pcm format & band mode */
	if (tdm2c->pcm_format == MV_PCM_FORMAT_4BYTES) {
		tdm2c->pcm_format = MV_PCM_FORMAT_2BYTES;
		tdm2c->band_mode = MV_WIDE_BAND;
	} else {
		tdm2c->band_mode = MV_NARROW_BAND;
	}

	/* Allocate aggregated buffers for data transport */
	dev_dbg(tdm2c->dev, "allocate %d bytes for aggregated buffer\n",
		MV_TDM_AGGR_BUFF_SIZE(tdm2c->pcm_format, tdm2c->band_mode, tdm2c->factor));
	tdm2c->rx_aggr_buff_virt = alloc_pages_exact(
			MV_TDM_AGGR_BUFF_SIZE(tdm2c->pcm_format, tdm2c->band_mode, tdm2c->factor),
			GFP_KERNEL);
	tdm2c->tx_aggr_buff_virt = alloc_pages_exact(
			MV_TDM_AGGR_BUFF_SIZE(tdm2c->pcm_format, tdm2c->band_mode, tdm2c->factor),
			GFP_KERNEL);
	if (!tdm2c->rx_aggr_buff_virt || !tdm2c->tx_aggr_buff_virt) {
		dev_err(tdm2c->dev, "%s: Error malloc failed\n", __func__);
		return -ENOMEM;
	}

	/* Clear buffers */
	memset(tdm2c->rx_aggr_buff_virt, 0,
	       MV_TDM_AGGR_BUFF_SIZE(tdm2c->pcm_format, tdm2c->band_mode, tdm2c->factor));
	memset(tdm2c->tx_aggr_buff_virt, 0,
	       MV_TDM_AGGR_BUFF_SIZE(tdm2c->pcm_format, tdm2c->band_mode, tdm2c->factor));

	/* Calculate CH(0/1) Delay Control for narrow/wideband modes */
	for (ch = 0; ch < MV_TDM2C_TOTAL_CHANNELS; ch++) {
		nb_delay = ((tdm_params->pcm_slot[ch] * PCM_SLOT_PCLK) + 1);
		/* Offset required by ZARLINK VE880 SLIC */
		wb_delay = (nb_delay + ((frame_ts / 2) * PCM_SLOT_PCLK));
		ch_delay[ch] = ((nb_delay << CH_RX_DELAY_OFFS) | (nb_delay << CH_TX_DELAY_OFFS));
		ch_delay[(ch + 2)] = ((wb_delay << CH_RX_DELAY_OFFS) | (wb_delay << CH_TX_DELAY_OFFS));
	}

	/* Enable TDM/SPI interface */
	mv_phone_reset_bit(tdm2c->regs + TDM_SPI_MUX_REG, 0x00000001);
	/* Interrupt cause is not clear on read */
	writel(CLEAR_ON_ZERO, tdm2c->regs + INT_RESET_SELECT_REG);
	/* All interrupt bits latched in status */
	writel(0x3ffff, tdm2c->regs + INT_EVENT_MASK_REG);
	/* Disable interrupts */
	writel(0, tdm2c->regs + INT_STATUS_MASK_REG);
	/* Clear int status register */
	writel(0, tdm2c->regs + INT_STATUS_REG);

	/* Bypass clock divider - PCM PCLK freq */
	writel(PCM_DIV_PASS, tdm2c->regs + PCM_CLK_RATE_DIV_REG);

	/* Padding on Rx completion */
	writel(0, tdm2c->regs + DUMMY_RX_WRITE_DATA_REG);
	writeb(readl(tdm2c->regs + SPI_GLOBAL_CTRL_REG) | SPI_GLOBAL_ENABLE,
	       tdm2c->regs + SPI_GLOBAL_CTRL_REG);
	/* SPI SCLK freq */
	writel(SPI_CLK_2MHZ, tdm2c->regs + SPI_CLK_PRESCALAR_REG);
	/* Number of timeslots (PCLK) */
	writel((u32)frame_ts, tdm2c->regs + FRAME_TIMESLOT_REG);

	if (tdm2c->band_mode == MV_NARROW_BAND) {
		pcm_ctrl_reg = (CONFIG_PCM_CRTL | (((u8)tdm2c->pcm_format - 1) << PCM_SAMPLE_SIZE_OFFS));

		if (use_pclk_external)
			pcm_ctrl_reg |= MASTER_PCLK_EXTERNAL;

		/* PCM configuration */
		writel(pcm_ctrl_reg, tdm2c->regs + PCM_CTRL_REG);
		/* CH0 delay control register */
		writel(ch_delay[0], tdm2c->regs + CH_DELAY_CTRL_REG(0));
		/* CH1 delay control register */
		writel(ch_delay[1], tdm2c->regs + CH_DELAY_CTRL_REG(1));
	} else {		/* MV_WIDE_BAND */

		pcm_ctrl_reg = (CONFIG_WB_PCM_CRTL | (((u8)tdm2c->pcm_format - 1) << PCM_SAMPLE_SIZE_OFFS));

		if (use_pclk_external)
			pcm_ctrl_reg |= MASTER_PCLK_EXTERNAL;

		/* PCM configuration - WB support */
		writel(pcm_ctrl_reg, tdm2c->regs + PCM_CTRL_REG);
		/* CH0 delay control register */
		writel(ch_delay[0], tdm2c->regs + CH_DELAY_CTRL_REG(0));
		/* CH1 delay control register */
		writel(ch_delay[1], tdm2c->regs + CH_DELAY_CTRL_REG(1));
		/* CH0 WB delay control register */
		writel(ch_delay[2], tdm2c->regs + CH_WB_DELAY_CTRL_REG(0));
		/* CH1 WB delay control register */
		writel(ch_delay[3], tdm2c->regs + CH_WB_DELAY_CTRL_REG(1));
	}

	/* Issue reset to codec(s) */
	dev_dbg(tdm2c->dev, "resetting voice unit(s)\n");
	writel(0, tdm2c->regs + MISC_CTRL_REG);
	mdelay(1);
	writel(1, tdm2c->regs + MISC_CTRL_REG);

	if (spi_mode == MV_SPI_MODE_DAISY_CHAIN) {
		/* Configure TDM to work in daisy chain mode */
		tdm2c_daisy_chain_mode_set();
	}

	/* Initialize all HW units */
	for (ch = 0; ch < MV_TDM2C_TOTAL_CHANNELS; ch++) {
		ret = tdm2c_ch_init(ch);
		if (ret) {
			dev_err(tdm2c->dev, "tdm2c_ch_init(%d) failed !\n", ch);
			return ret;
		}
	}

	/* Enable SLIC/DAA interrupt detection(before pcm is active) */
	writel((readl(tdm2c->regs + INT_STATUS_MASK_REG) | TDM_INT_SLIC), tdm2c->regs + INT_STATUS_MASK_REG);

	return 0;
}

void tdm2c_release(void)
{
	u8 ch;

	/* Free Rx/Tx aggregated buffers */
	free_pages_exact(tdm2c->rx_aggr_buff_virt,
			 MV_TDM_AGGR_BUFF_SIZE(tdm2c->pcm_format, tdm2c->band_mode, tdm2c->factor));
	free_pages_exact(tdm2c->tx_aggr_buff_virt,
			 MV_TDM_AGGR_BUFF_SIZE(tdm2c->pcm_format, tdm2c->band_mode, tdm2c->factor));

	/* Release HW channel resources */
	for (ch = 0; ch < MV_TDM2C_TOTAL_CHANNELS; ch++)
		tdm2c_ch_remove(ch);

	/* Disable TDM/SPI interface */
	mv_phone_set_bit(tdm2c->regs + TDM_SPI_MUX_REG, 0x00000001);
}

void tdm2c_pcm_start(void)
{
	struct tdm2c_ch_info *ch_info;
	u8 ch;

	/* TDM is enabled */
	tdm2c->enable = true;
	tdm2c->int_lock = false;
	tdm2c->chan_stop_count = 0;
	tdm2c_reset();

	for (ch = 0; ch < MV_TDM2C_TOTAL_CHANNELS; ch++) {
		ch_info = tdm2c->ch_info[ch];

		/* Set Tx buff */
		writel(ch_info->tx_buff_phys[ch_info->tx_curr_buff], tdm2c->regs + CH_TX_ADDR_REG(ch));
		writeb(OWN_BY_HW, tdm2c->regs + CH_BUFF_OWN_REG(ch) + TX_OWN_BYTE_OFFS);

		/* Set Rx buff */
		writel(ch_info->rx_buff_phys[ch_info->rx_curr_buff], tdm2c->regs + CH_RX_ADDR_REG(ch));
		writeb(OWN_BY_HW, tdm2c->regs + CH_BUFF_OWN_REG(ch) + RX_OWN_BYTE_OFFS);

	}

	/* Enable Tx */
	writeb(CH_ENABLE, tdm2c->regs + CH_ENABLE_REG(0) + TX_ENABLE_BYTE_OFFS);
	writeb(CH_ENABLE, tdm2c->regs + CH_ENABLE_REG(1) + TX_ENABLE_BYTE_OFFS);

	/* Enable Rx */
	writeb(CH_ENABLE, tdm2c->regs + CH_ENABLE_REG(0) + RX_ENABLE_BYTE_OFFS);
	writeb(CH_ENABLE, tdm2c->regs + CH_ENABLE_REG(1) + RX_ENABLE_BYTE_OFFS);

	/* Enable Tx interrupts */
	writel(readl(tdm2c->regs + INT_STATUS_REG) & (~(TDM_INT_TX(0) | TDM_INT_TX(1))),
	       tdm2c->regs + INT_STATUS_REG);
	writel((readl(tdm2c->regs + INT_STATUS_MASK_REG) | TDM_INT_TX(0) | TDM_INT_TX(1)),
	       tdm2c->regs + INT_STATUS_MASK_REG);

	/* Enable Rx interrupts */
	writel((readl(tdm2c->regs + INT_STATUS_REG) & (~(TDM_INT_RX(0) | TDM_INT_RX(1)))),
	       tdm2c->regs + INT_STATUS_REG);
	writel((readl(tdm2c->regs + INT_STATUS_MASK_REG) | TDM_INT_RX(0) | TDM_INT_RX(1)),
	       tdm2c->regs + INT_STATUS_MASK_REG);
}

void tdm2c_pcm_stop(void)
{
	tdm2c->enable = false;

	tdm2c_reset();
}

int tdm2c_tx(u8 *tdm_tx_buff)
{
	struct tdm2c_ch_info *ch_info;
	u8 ch;
	u8 *tx_buff;

	/* Sanity check */
	if (tdm_tx_buff != tdm2c->tx_aggr_buff_virt) {
		dev_err(tdm2c->dev, "%s: Error, invalid Tx buffer !!!\n", __func__);
		return -EINVAL;
	}

	if (!tdm2c->enable) {
		dev_err(tdm2c->dev, "%s: Error, no active Tx channels are available\n", __func__);
		return -EINVAL;
	}

	if (tdm2c->tx_empty == BUFF_INVALID) {
		dev_err(tdm2c->dev, "%s: Tx not ready\n", __func__);
		return -EINVAL;
	}

	for (ch = 0; ch < MV_TDM2C_TOTAL_CHANNELS; ch++) {
		ch_info = tdm2c->ch_info[ch];
		dev_dbg(tdm2c->dev, "ch%d: fill buf %d with %d bytes\n",
			ch, tdm2c->tx_empty,
			MV_TDM_CH_BUFF_SIZE(tdm2c->pcm_format, tdm2c->band_mode, tdm2c->factor));
		ch_info->tx_buff_empty[tdm2c->tx_empty] = BUFF_IS_FULL;
		tx_buff = tdm_tx_buff +
			  (ch * MV_TDM_CH_BUFF_SIZE(tdm2c->pcm_format, tdm2c->band_mode, tdm2c->factor));

		/* Copy data from voice engine buffer to DMA */
		memcpy(ch_info->tx_buff_virt[tdm2c->tx_empty], tx_buff,
		       MV_TDM_CH_BUFF_SIZE(tdm2c->pcm_format, tdm2c->band_mode, tdm2c->factor));
	}

	tdm2c->tx_empty = BUFF_INVALID;

	return 0;
}

int tdm2c_rx(u8 *tdm_rx_buff)
{
	struct tdm2c_ch_info *ch_info;
	u8 ch;
	u8 *rx_buff;

	/* Sanity check */
	if (tdm_rx_buff != tdm2c->rx_aggr_buff_virt) {
		dev_err(tdm2c->dev, "%s: invalid Rx buffer !!!\n", __func__);
		return -EINVAL;
	}

	if (!tdm2c->enable) {
		dev_err(tdm2c->dev, "%s: Error, no active Rx channels are available\n", __func__);
		return -EINVAL;
	}

	if (tdm2c->rx_full == BUFF_INVALID) {
		dev_err(tdm2c->dev, "%s: Rx not ready\n", __func__);
		return -EINVAL;
	}

	for (ch = 0; ch < MV_TDM2C_TOTAL_CHANNELS; ch++) {
		ch_info = tdm2c->ch_info[ch];
		ch_info->rx_buff_full[tdm2c->rx_full] = BUFF_IS_EMPTY;
		dev_dbg(tdm2c->dev, "%s get Rx buffer(%d) for channel(%d)\n",
			__func__, tdm2c->rx_full, ch);
		rx_buff = tdm_rx_buff +
			  (ch * MV_TDM_CH_BUFF_SIZE(tdm2c->pcm_format, tdm2c->band_mode, tdm2c->factor));

		/* Copy data from DMA to voice engine buffer */
		memcpy(rx_buff, ch_info->rx_buff_virt[tdm2c->rx_full],
		       MV_TDM_CH_BUFF_SIZE(tdm2c->pcm_format, tdm2c->band_mode, tdm2c->factor));
	}

	tdm2c->rx_full = BUFF_INVALID;

	return 0;
}

int tdm2c_pcm_stop_int_miss(void)
{
	u32 status_reg, mask_reg, status_stop_int, status_mask = 0, int_mask = 0;

	status_reg = readl(tdm2c->regs + INT_STATUS_REG);
	mask_reg = readl(tdm2c->regs + INT_STATUS_MASK_REG);

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
		dev_err(tdm2c->dev, "Stop Interrupt missing found STATUS=%x, MASK=%x\n", status_reg, mask_reg);
		writel(~(status_mask), tdm2c->regs + INT_STATUS_REG);
		writel(readl(tdm2c->regs + INT_STATUS_MASK_REG) & (~(int_mask)),
		       tdm2c->regs + INT_STATUS_MASK_REG);

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
	status_reg = readl(tdm2c->regs + INT_STATUS_REG);
	mask_reg = readl(tdm2c->regs + INT_STATUS_MASK_REG);
	dev_dbg(tdm2c->dev, "CAUSE(0x%x), MASK(0x%x)\n", status_reg, mask_reg);

	/* Refer only to unmasked bits */
	status_and_mask = status_reg & mask_reg;

	/* Reset params */
	tdm_intr_info->tdm_rx_buff = NULL;
	tdm_intr_info->tdm_tx_buff = NULL;
	tdm_intr_info->int_type = MV_EMPTY_INT;
	tdm_intr_info->cs = MV_TDM_CS;

	/* Handle SLIC/DAA int */
	if (status_and_mask & SLIC_INT_BIT) {
		dev_dbg(tdm2c->dev, "Phone interrupt !!!\n");
		tdm_intr_info->int_type |= MV_PHONE_INT;
	}

	if (status_and_mask & DMA_ABORT_BIT) {
		dev_err(tdm2c->dev, "DMA data abort. Address: 0x%08x, Info: 0x%08x\n",
			readl(tdm2c->regs + DMA_ABORT_ADDR_REG),
			readl(tdm2c->regs + DMA_ABORT_INFO_REG));
		tdm_intr_info->int_type |= MV_DMA_ERROR_INT;
	}

	for (ch = 0; ch < MV_TDM2C_TOTAL_CHANNELS; ch++) {

		/* Give next buff to TDM and set curr buff as empty */
		if ((status_and_mask & TX_BIT(ch)) && tdm2c->enable && !tdm2c->int_lock) {
			dev_dbg(tdm2c->dev, "Tx interrupt(ch%d)\n", ch);

			tdm2c->int_tx_count++;
			if (ch == 0) {
				tdm2c->int_tx0_count++;
				if (tdm2c->int_tx0_count <= tdm2c->int_tx1_count) {
					int_tx_miss = 0;
					tdm2c->int_tx0_miss++;
				}
			} else {
				tdm2c->int_tx1_count++;
				if (tdm2c->int_tx1_count < tdm2c->int_tx0_count) {
					int_tx_miss = 1;
					tdm2c->int_tx1_miss++;
				}
			}

			/* 0 -> Tx is done for both channels */
			if (tdm2c_ch_tx_low(ch) == 0) {
				dev_dbg(tdm2c->dev, "Assign Tx aggregate buffer for further processing\n");
				tdm_intr_info->tdm_tx_buff = tdm2c->tx_aggr_buff_virt;
				tdm_intr_info->int_type |= MV_TX_INT;
			}
		}
	}

	for (ch = 0; ch < MV_TDM2C_TOTAL_CHANNELS; ch++) {

		if ((status_and_mask & RX_BIT(ch)) && tdm2c->enable && !tdm2c->int_lock) {
			dev_dbg(tdm2c->dev, "Rx interrupt(ch%d)\n", ch);

			tdm2c->int_rx_count++;
			if (ch == 0) {
				tdm2c->int_rx0_count++;
				if (tdm2c->int_rx0_count <= tdm2c->int_rx1_count) {
					int_rx_miss = 0;
					tdm2c->int_rx0_miss++;
				}
			} else {
				tdm2c->int_rx1_count++;
				if (tdm2c->int_rx1_count < tdm2c->int_rx0_count) {
					int_rx_miss = 1;
					tdm2c->int_rx1_miss++;
				}
			}

			/* 0 -> Rx is done for both channels */
			if (tdm2c_ch_rx_low(ch) == 0) {
				dev_dbg(tdm2c->dev, "Assign Rx aggregate buffer for further processing\n");
				tdm_intr_info->tdm_rx_buff = tdm2c->rx_aggr_buff_virt;
				tdm_intr_info->int_type |= MV_RX_INT;
			}
		}
	}

	for (ch = 0; ch < MV_TDM2C_TOTAL_CHANNELS; ch++) {

		if (status_and_mask & TX_UNDERFLOW_BIT(ch)) {

			dev_dbg(tdm2c->dev, "Tx underflow(ch%d) - checking for root cause...\n",
				    ch);
			if (tdm2c->enable) {
				dev_dbg(tdm2c->dev, "Tx underflow ERROR\n");
				tdm_intr_info->int_type |= MV_TX_ERROR_INT;
				if (!(status_and_mask & TX_BIT(ch))) {
					ret = -1;
					/* 0 -> Tx is done for both channels */
					if (tdm2c_ch_tx_low(ch) == 0) {
						dev_dbg(tdm2c->dev, "Assign Tx aggregate buffer for further processing\n");
						tdm_intr_info->tdm_tx_buff = tdm2c->tx_aggr_buff_virt;
						tdm_intr_info->int_type |= MV_TX_INT;
					}
				}
			} else {
				dev_dbg(tdm2c->dev, "Expected Tx underflow(not an error)\n");
				tdm_intr_info->int_type |= MV_CHAN_STOP_INT;
				/* Update number of channels already stopped */
				tdm_intr_info->data = ++tdm2c->chan_stop_count;
				writel(readl(tdm2c->regs + INT_STATUS_MASK_REG) & (~(TDM_INT_TX(ch))),
				       tdm2c->regs + INT_STATUS_MASK_REG);
			}
		}


		if (status_and_mask & RX_OVERFLOW_BIT(ch)) {
			dev_dbg(tdm2c->dev, "Rx overflow(ch%d) - checking for root cause...\n", ch);
			if (tdm2c->enable) {
				dev_dbg(tdm2c->dev, "Rx overflow ERROR\n");
				tdm_intr_info->int_type |= MV_RX_ERROR_INT;
				if (!(status_and_mask & RX_BIT(ch))) {
					ret = -1;
					/* 0 -> Rx is done for both channels */
					if (tdm2c_ch_rx_low(ch) == 0) {
						dev_dbg(tdm2c->dev, "Assign Rx aggregate buffer for further processing\n");
						tdm_intr_info->tdm_rx_buff = tdm2c->rx_aggr_buff_virt;
						tdm_intr_info->int_type |= MV_RX_INT;
					}
				}
			} else {
				dev_dbg(tdm2c->dev, "Expected Rx overflow(not an error)\n");
				tdm_intr_info->int_type |= MV_CHAN_STOP_INT;
				/* Update number of channels already stopped */
				tdm_intr_info->data = ++tdm2c->chan_stop_count;
				writel(readl(tdm2c->regs + INT_STATUS_MASK_REG) & (~(TDM_INT_RX(ch))),
				       tdm2c->regs + INT_STATUS_MASK_REG);
			}
		}
	}

	/* clear TDM interrupts */
	writel(~status_reg, tdm2c->regs + INT_STATUS_REG);

	/* Check if interrupt was missed -> restart */
	if  (int_tx_miss != -1)  {
		dev_err(tdm2c->dev, "Missing Tx Interrupt Detected ch%d!!!\n", int_tx_miss);
		if (int_tx_miss)
			tdm2c->int_tx1_count = tdm2c->int_tx0_count;
		else
			tdm2c->int_tx0_count  = (tdm2c->int_tx1_count + 1);
		ret = -1;
	}

	if  (int_rx_miss != -1)  {
		dev_err(tdm2c->dev, "Missing Rx Interrupt Detected ch%d!!!\n", int_rx_miss);
		if (int_rx_miss)
			tdm2c->int_rx1_count = tdm2c->int_rx0_count;
		else
			tdm2c->int_rx0_count  = (tdm2c->int_rx1_count + 1);
		ret = -1;
	}

	if (ret == -1) {
		tdm2c->int_lock = true;
		tdm2c->pcm_restart_count++;
	}

	return ret;
}

void tdm2c_intr_enable(void)
{
	writel((readl(tdm2c->regs + INT_STATUS_MASK_REG) | TDM_INT_SLIC),
	       tdm2c->regs + INT_STATUS_MASK_REG);
}

void tdm2c_intr_disable(void)
{
	u32 val = ~TDM_INT_SLIC;

	writel((readl(tdm2c->regs + INT_STATUS_MASK_REG) & val),
	       tdm2c->regs + INT_STATUS_MASK_REG);
}

void tdm2c_pcm_if_reset(void)
{
	/* SW PCM reset assert */
	mv_phone_reset_bit(tdm2c->regs + TDM_MISC_REG, 0x00000001);

	mdelay(10);

	/* SW PCM reset de-assert */
	mv_phone_set_bit(tdm2c->regs + TDM_MISC_REG, 0x00000001);

	/* Wait a bit more - might be fine tuned */
	mdelay(50);

	dev_dbg(tdm2c->dev, "%s: Exit\n", __func__);
}

/* Debug routines */
void tdm2c_reg_dump(u32 offset)
{
	dev_info(tdm2c->dev, "0x%05x: %08x\n", offset, readl(tdm2c->regs + offset));
}

void tdm2c_regs_dump(void)
{
	u8 i;
	struct tdm2c_ch_info *ch_info;

	dev_info(tdm2c->dev, "TDM Control:\n");
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
	dev_info(tdm2c->dev, "TDM Channel Control:\n");
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
	dev_info(tdm2c->dev, "TDM interrupts:\n");
	tdm2c_reg_dump(INT_EVENT_MASK_REG);
	tdm2c_reg_dump(INT_STATUS_MASK_REG);
	tdm2c_reg_dump(INT_STATUS_REG);
	for (i = 0; i < MV_TDM2C_TOTAL_CHANNELS; i++) {
		dev_info(tdm2c->dev, "ch%d info:\n", i);
		ch_info = tdm2c->ch_info[i];
		dev_info(tdm2c->dev, "RX buffs:\n");
		dev_info(tdm2c->dev, "buff0: virt=%p phys=%p\n",
			 ch_info->rx_buff_virt[0], (u32 *) (ch_info->rx_buff_phys[0]));
		dev_info(tdm2c->dev, "buff1: virt=%p phys=%p\n",
			 ch_info->rx_buff_virt[1], (u32 *) (ch_info->rx_buff_phys[1]));
		dev_info(tdm2c->dev, "TX buffs:\n");
		dev_info(tdm2c->dev, "buff0: virt=%p phys=%p\n",
			 ch_info->tx_buff_virt[0], (u32 *) (ch_info->tx_buff_phys[0]));
		dev_info(tdm2c->dev, "buff1: virt=%p phys=%p\n",
			 ch_info->tx_buff_virt[1], (u32 *) (ch_info->tx_buff_phys[1]));
	}
}

void tdm2c_ext_stats_get(struct mv_phone_extended_stats *tdm_ext_stats)
{
	tdm_ext_stats->int_rx_count = tdm2c->int_rx_count;
	tdm_ext_stats->int_tx_count = tdm2c->int_tx_count;
	tdm_ext_stats->int_rx0_count = tdm2c->int_rx0_count;
	tdm_ext_stats->int_tx0_count = tdm2c->int_tx0_count;
	tdm_ext_stats->int_rx1_count = tdm2c->int_rx1_count;
	tdm_ext_stats->int_tx1_count = tdm2c->int_tx1_count;
	tdm_ext_stats->int_rx0_miss = tdm2c->int_rx0_miss;
	tdm_ext_stats->int_tx0_miss = tdm2c->int_tx0_miss;
	tdm_ext_stats->int_rx1_miss = tdm2c->int_rx1_miss;
	tdm_ext_stats->int_tx1_miss = tdm2c->int_tx1_miss;
	tdm_ext_stats->pcm_restart_count = tdm2c->pcm_restart_count;
}

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
