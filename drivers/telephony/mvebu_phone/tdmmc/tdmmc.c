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

/* Globals */
static struct tdmmc_dram_entry def_dpram_entry = { 0, 0, 0x1, 0x1, 0, 0, 0x1, 0, 0, 0, 0 };
static struct tdmmc_dev *tdmmc;

static void tdmmc_desc_chain_build(void)
{
	u32 chan, index, buff_size;

	/* Calculate single Rx/Tx buffer size */
	buff_size = (tdmmc->sample_size * MV_TDM_TOTAL_CH_SAMPLES * tdmmc->sampling_coeff);

	/* Initialize descriptors fields */
	for (chan = 0; chan < tdmmc->total_channels; chan++) {
		for (index = 0; index < TOTAL_CHAINS; index++) {
			/* Associate data buffers to descriptors physBuffPtr */
			((struct tdmmc_mcdma_rx_desc *) (tdmmc->rx_desc_virt[index] + chan))->phys_buff_ptr =
			    (u32) (tdmmc->rx_buff_phys[index] + (chan * buff_size));
			((struct tdmmc_mcdma_tx_desc *) (tdmmc->tx_desc_virt[index] + chan))->phys_buff_ptr =
			    (u32) (tdmmc->tx_buff_phys[index] + (chan * buff_size));

			/* Build cyclic descriptors chain for each channel */
			((struct tdmmc_mcdma_rx_desc *) (tdmmc->rx_desc_virt[index] + chan))->phys_next_desc_ptr =
			    (u32) (tdmmc->rx_desc_phys[((index + 1) % TOTAL_CHAINS)] +
				      (chan * sizeof(struct tdmmc_mcdma_rx_desc)));

			((struct tdmmc_mcdma_tx_desc *) (tdmmc->tx_desc_virt[index] + chan))->phys_next_desc_ptr =
			    (u32) (tdmmc->tx_desc_phys[((index + 1) % TOTAL_CHAINS)] +
				      (chan * sizeof(struct tdmmc_mcdma_tx_desc)));

			/* Set Byte_Count/Buffer_Size Rx descriptor fields */
			((struct tdmmc_mcdma_rx_desc *) (tdmmc->rx_desc_virt[index] + chan))->byte_cnt = 0;
			((struct tdmmc_mcdma_rx_desc *) (tdmmc->rx_desc_virt[index] + chan))->buff_size = buff_size;

			/* Set Shadow_Byte_Count/Byte_Count Tx descriptor fields */
			((struct tdmmc_mcdma_tx_desc *) (tdmmc->tx_desc_virt[index] + chan))->shadow_byte_cnt =
													 buff_size;
			((struct tdmmc_mcdma_tx_desc *) (tdmmc->tx_desc_virt[index] + chan))->byte_cnt = buff_size;

			/* Set Command/Status Rx/Tx descriptor fields */
			((struct tdmmc_mcdma_rx_desc *) (tdmmc->rx_desc_virt[index] + chan))->cmd_status =
			    (CONFIG_MCDMA_DESC_CMD_STATUS);
			((struct tdmmc_mcdma_tx_desc *) (tdmmc->tx_desc_virt[index] + chan))->cmd_status =
			    (CONFIG_MCDMA_DESC_CMD_STATUS);
		}
	}
}

static void tdmmc_mcdma_mcsc_start(void)
{
	u32 chan;
	dma_addr_t rx_desc_phys_addr, tx_desc_phys_addr;

	tdmmc_desc_chain_build();

	/* Set current Rx/Tx descriptors  */
	for (chan = 0; chan < tdmmc->total_channels; chan++) {
		rx_desc_phys_addr = tdmmc->rx_desc_phys[0] + (chan * sizeof(struct tdmmc_mcdma_rx_desc));
		tx_desc_phys_addr = tdmmc->tx_desc_phys[0] + (chan * sizeof(struct tdmmc_mcdma_tx_desc));
		writel(rx_desc_phys_addr, tdmmc->regs + MCDMA_CURRENT_RECEIVE_DESC_PTR_REG(chan));
		writel(tx_desc_phys_addr, tdmmc->regs + MCDMA_CURRENT_TRANSMIT_DESC_PTR_REG(chan));
	}

	/* Restore MCDMA Rx/Tx control registers */
	for (chan = 0; chan < tdmmc->total_channels; chan++) {
		/* Set RMCCx */
		writel(CONFIG_RMCCx, tdmmc->regs + MCDMA_RECEIVE_CONTROL_REG(chan));

		/* Set TMCCx */
		writel(CONFIG_TMCCx, tdmmc->regs + MCDMA_TRANSMIT_CONTROL_REG(chan));
	}

	/* Set Rx/Tx periodical interrupts */
	if (tdmmc->ip_ver == TDMMC_REV0)
		writel(CONFIG_VOICE_PERIODICAL_INT_CONTROL_WA,
		       tdmmc->regs + VOICE_PERIODICAL_INT_CONTROL_REG);
	else
		writel(CONFIG_VOICE_PERIODICAL_INT_CONTROL,
		       tdmmc->regs + VOICE_PERIODICAL_INT_CONTROL_REG);

	/* MCSC Global Tx Enable */
	if (!tdmmc->tdm_enable)
		mv_phone_set_bit(tdmmc->regs + MCSC_GLOBAL_CONFIG_REG, MCSC_GLOBAL_CONFIG_TXEN_MASK);

	/* Enable MCSC-Tx & MCDMA-Rx */
	for (chan = 0; chan < tdmmc->total_channels; chan++) {
		/* Enable Tx in TMCCx */
		if (!tdmmc->tdm_enable)
			mv_phone_set_bit(tdmmc->regs + MCSC_CHx_TRANSMIT_CONFIG_REG(chan), MTCRx_ET_MASK);

		/* Enable Rx in: MCRDPx */
		mv_phone_set_bit(tdmmc->regs + MCDMA_RECEIVE_CONTROL_REG(chan), MCDMA_ERD_MASK);
	}

	/* MCSC Global Rx Enable */
	if (!tdmmc->tdm_enable)
		mv_phone_set_bit(tdmmc->regs + MCSC_GLOBAL_CONFIG_REG, MCSC_GLOBAL_CONFIG_RXEN_MASK);

	/* Enable MCSC-Rx & MCDMA-Tx */
	for (chan = 0; chan < tdmmc->total_channels; chan++) {
		/* Enable Rx in RMCCx */
		if (!tdmmc->tdm_enable)
			mv_phone_set_bit(tdmmc->regs + MCSC_CHx_RECEIVE_CONFIG_REG(chan), MRCRx_ER_MASK);

		/* Enable Tx in MCTDPx */
		mv_phone_set_bit(tdmmc->regs + MCDMA_TRANSMIT_CONTROL_REG(chan), MCDMA_TXD_MASK);
	}

	/* Disable Rx/Tx return to half */
	mv_phone_reset_bit(tdmmc->regs + FLEX_TDM_CONFIG_REG, (TDM_RR2HALF_MASK | TDM_TR2HALF_MASK));
	/* Wait at least 1 frame */
	udelay(200);
}

static void tdmmc_mcdma_mcsc_abort(void)
{
	u32 chan;

	/* Abort MCSC/MCDMA in case we got here from tdmmc_release() */
	if (!tdmmc->tdm_enable) {
		/* Clear MCSC Rx/Tx channel enable */
		for (chan = 0; chan < tdmmc->total_channels; chan++) {
			mv_phone_reset_bit(tdmmc->regs + MCSC_CHx_RECEIVE_CONFIG_REG(chan), MRCRx_ER_MASK);
			mv_phone_reset_bit(tdmmc->regs + MCSC_CHx_TRANSMIT_CONFIG_REG(chan), MTCRx_ET_MASK);
		}

		/* MCSC Global Rx/Tx Disable */
		mv_phone_reset_bit(tdmmc->regs + MCSC_GLOBAL_CONFIG_REG, MCSC_GLOBAL_CONFIG_RXEN_MASK);
		mv_phone_reset_bit(tdmmc->regs + MCSC_GLOBAL_CONFIG_REG, MCSC_GLOBAL_CONFIG_TXEN_MASK);
	}
}

static void tdmmc_mcdma_stop(void)
{
	u32 index, chan, max_poll;
	u32 curr_rx_desc, curr_tx_desc, next_tx_buff = 0, next_rx_buff = 0;

	/***************************/
	/*    Stop MCDMA - Rx/Tx   */
	/***************************/
	for (chan = 0; chan < tdmmc->total_channels; chan++) {
		curr_rx_desc = readl(tdmmc->regs + MCDMA_CURRENT_RECEIVE_DESC_PTR_REG(chan));
		for (index = 0; index < TOTAL_CHAINS; index++) {
			if (curr_rx_desc == (tdmmc->rx_desc_phys[index] +
			    (chan * (sizeof(struct tdmmc_mcdma_rx_desc))))) {
				next_rx_buff = NEXT_BUFF(index);
				break;
			}
		}

		if (index == TOTAL_CHAINS) {
			dev_err(tdmmc->dev, "%s: ERROR, couldn't Rx descriptor match for chan(%d)\n",
				__func__, chan);
			break;
		}

		((struct tdmmc_mcdma_rx_desc *)
			(tdmmc->rx_desc_virt[next_rx_buff] + chan))->phys_next_desc_ptr = 0;
		((struct tdmmc_mcdma_rx_desc *)
			(tdmmc->rx_desc_virt[next_rx_buff] + chan))->cmd_status = (LAST_BIT | OWNER);
	}

	for (chan = 0; chan < tdmmc->total_channels; chan++) {
		curr_tx_desc = readl(tdmmc->regs + MCDMA_CURRENT_TRANSMIT_DESC_PTR_REG(chan));
		for (index = 0; index < TOTAL_CHAINS; index++) {
			if (curr_tx_desc == (tdmmc->tx_desc_phys[index] +
			    (chan * (sizeof(struct tdmmc_mcdma_tx_desc))))) {
				next_tx_buff = NEXT_BUFF(index);
				break;
			}
		}

		if (index == TOTAL_CHAINS) {
			dev_err(tdmmc->dev, "%s: ERROR, couldn't Tx descriptor match for chan(%d)\n",
				__func__, chan);
			return;
		}

		((struct tdmmc_mcdma_tx_desc *)
			(tdmmc->tx_desc_virt[next_tx_buff] + chan))->phys_next_desc_ptr = 0;
		((struct tdmmc_mcdma_tx_desc *)
			(tdmmc->tx_desc_virt[next_tx_buff] + chan))->cmd_status = (LAST_BIT | OWNER);
	}

	for (chan = 0; chan < tdmmc->total_channels; chan++) {
		max_poll = 0;
		while ((max_poll < MAX_POLL_USEC) &&
			(readl(tdmmc->regs + MCDMA_TRANSMIT_CONTROL_REG(chan)) & MCDMA_TXD_MASK)) {
			udelay(1);
			max_poll++;
		}

		if (max_poll >= MAX_POLL_USEC) {
			dev_err(tdmmc->dev, "%s: Error, MCDMA TXD polling timeout(ch%d)\n", __func__, chan);
			return;
		}

		max_poll = 0;
		while ((max_poll < MAX_POLL_USEC) &&
			(readl(tdmmc->regs + MCDMA_RECEIVE_CONTROL_REG(chan)) & MCDMA_ERD_MASK)) {
			udelay(1);
			max_poll++;
		}

		if (max_poll >= MAX_POLL_USEC) {
			dev_err(tdmmc->dev, "%s: Error, MCDMA ERD polling timeout(ch%d)\n", __func__, chan);
			return;
		}
	}

	/* Disable Rx/Tx periodical interrupts */
	writel(0xffffffff, tdmmc->regs + VOICE_PERIODICAL_INT_CONTROL_REG);

	/* Enable Rx/Tx return to half */
	mv_phone_set_bit(tdmmc->regs + FLEX_TDM_CONFIG_REG, (TDM_RR2HALF_MASK | TDM_TR2HALF_MASK));
	/* Wait at least 1 frame */
	udelay(200);

	/* Manual reset to channel-balancing mechanism */
	mv_phone_set_bit(tdmmc->regs + MCSC_GLOBAL_CONFIG_REG, MCSC_GLOBAL_CONFIG_MAI_MASK);
	udelay(1);
}

void tdmmc_intr_enable(u8 device_id)
{
}

void tdmmc_intr_disable(u8 device_id)
{
}

void tdmmc_show(void)
{
	u32 index;

	/* Dump data buffers & descriptors addresses */
	for (index = 0; index < TOTAL_CHAINS; index++) {
		dev_dbg(tdmmc->dev, "Rx Buff(%d): virt = 0x%lx, phys = 0x%lx\n",
			index, (ulong)tdmmc->rx_buff_virt[index],
			(ulong)tdmmc->rx_buff_phys[index]);
		dev_dbg(tdmmc->dev, "Tx Buff(%d): virt = 0x%lx, phys = 0x%lx\n",
			index, (ulong)tdmmc->tx_buff_virt[index],
			(ulong)tdmmc->tx_buff_phys[index]);
		dev_dbg(tdmmc->dev, "Rx Desc(%d): virt = 0x%lx, phys = 0x%lx\n",
			index, (ulong)tdmmc->rx_desc_virt[index],
			(ulong) tdmmc->rx_desc_phys[index]);
		dev_dbg(tdmmc->dev, "Tx Desc(%d): virt = 0x%lx, phys = 0x%lx\n",
			index, (ulong)tdmmc->tx_desc_virt[index],
			(ulong)tdmmc->tx_desc_phys[index]);
	}
}

int tdmmc_init(void __iomem *base, struct device *dev,
	       struct mv_phone_params *tdm_params, enum mv_phone_frame_ts frame_ts,
	       enum tdmmc_ip_version tdmmc_ip_ver)
{
	u16 pcm_slot, index;
	u32 buff_size, chan, total_rx_desc_size, total_tx_desc_size;
	u32 max_poll, clk_sync_ctrl_reg, count;
	struct tdmmc_dram_entry *act_dpram_entry;
	int ret;

	/* Initialize or reset main structure */
	if (!tdmmc) {
		tdmmc = devm_kzalloc(dev, sizeof(struct tdmmc_dev), GFP_KERNEL);
		if (!tdmmc)
			return -ENOMEM;
	} else {
		memset(tdmmc, 0,  sizeof(struct tdmmc_dev));
	}

	/* Initialize remaining parameters */
	tdmmc->regs = base;
	tdmmc->tdm_enable = false;
	tdmmc->pcm_enable = false;
	tdmmc->total_channels = tdm_params->total_channels;
	tdmmc->prev_rx = 0;
	tdmmc->next_tx = 0;
	tdmmc->ip_ver = tdmmc_ip_ver;
	tdmmc->dev = dev;

	/* Check parameters */
	if ((tdm_params->total_channels > MV_TDMMC_TOTAL_CHANNELS) ||
	    (tdm_params->sampling_period > MV_TDM_MAX_SAMPLING_PERIOD)) {
		dev_err(tdmmc->dev, "%s: Error, bad parameters\n", __func__);
		return -EINVAL;
	}

	/* Extract sampling period coefficient */
	tdmmc->sampling_coeff = (tdm_params->sampling_period / MV_TDM_BASE_SAMPLING_PERIOD);

	tdmmc->sample_size = tdm_params->pcm_format;

	/* Calculate single Rx/Tx buffer size */
	buff_size = (tdmmc->sample_size * MV_TDM_TOTAL_CH_SAMPLES * tdmmc->sampling_coeff);

	/* Allocate non-cached data buffers for all channels */
	dev_dbg(tdmmc->dev, "%s: allocate 0x%x for data buffers total channels = %d\n",
		__func__, (buff_size * tdmmc->total_channels), tdmmc->total_channels);

	for (index = 0; index < TOTAL_CHAINS; index++) {
		tdmmc->rx_buff_virt[index] = dma_alloc_coherent(tdmmc->dev, buff_size * tdmmc->total_channels,
						       &tdmmc->rx_buff_phys[index], GFP_KERNEL);
		tdmmc->tx_buff_virt[index] = dma_alloc_coherent(tdmmc->dev, buff_size * tdmmc->total_channels,
						       &tdmmc->tx_buff_phys[index], GFP_KERNEL);

		if (!tdmmc->rx_buff_virt[index] || !tdmmc->tx_buff_virt[index]) {
			ret = -ENOMEM;
			goto err_buff_virt;
		}
	}

	/* Allocate non-cached MCDMA Rx/Tx descriptors */
	total_rx_desc_size = tdmmc->total_channels * sizeof(struct tdmmc_mcdma_rx_desc);
	total_tx_desc_size = tdmmc->total_channels * sizeof(struct tdmmc_mcdma_tx_desc);

	dev_dbg(dev, "%s: allocate %dB for Rx/Tx descriptors\n",
		__func__, total_tx_desc_size);
	for (index = 0; index < TOTAL_CHAINS; index++) {
		tdmmc->rx_desc_virt[index] = dma_alloc_coherent(tdmmc->dev, total_rx_desc_size,
							   &tdmmc->rx_desc_phys[index], GFP_KERNEL);
		tdmmc->tx_desc_virt[index] = dma_alloc_coherent(tdmmc->dev, total_tx_desc_size,
							   &tdmmc->tx_desc_phys[index], GFP_KERNEL);

		if (!tdmmc->rx_desc_virt[index] || !tdmmc->tx_desc_virt[index]) {
			ret = -ENOMEM;
			goto err_mcdma_desc;
		}

		/* Check descriptors alignment */
		if (((ulong) tdmmc->rx_desc_virt[index] | (ulong)tdmmc->tx_desc_virt[index]) &
		    (sizeof(struct tdmmc_mcdma_rx_desc) - 1)) {
			dev_err(tdmmc->dev, "%s: Error, unaligned MCDMA Rx/Tx descriptors\n", __func__);
			ret = -ENOMEM;
			goto err_mcdma_desc;
		}
	}

	/* Poll MCDMA for reset completion */
	max_poll = 0;
	while ((max_poll < MAX_POLL_USEC) && !(readl(tdmmc->regs + MCDMA_GLOBAL_CONTROL_REG) & MCDMA_RID_MASK)) {
		udelay(1);
		max_poll++;
	}

	if (max_poll >= MAX_POLL_USEC) {
		dev_err(tdmmc->dev, "Error, MCDMA reset completion timout\n");
		ret = -ETIME;
		goto err_mcdma_desc;
	}

	/* Poll MCSC for RAM initialization done */
	if (!(readl(tdmmc->regs + MCSC_GLOBAL_INT_CAUSE_REG) & MCSC_GLOBAL_INT_CAUSE_INIT_DONE_MASK)) {
		max_poll = 0;
		while ((max_poll < MAX_POLL_USEC) &&
		       !(readl(tdmmc->regs + MCSC_GLOBAL_INT_CAUSE_REG) & MCSC_GLOBAL_INT_CAUSE_INIT_DONE_MASK)) {
			udelay(1);
			max_poll++;
		}

		if (max_poll >= MAX_POLL_USEC) {
			dev_err(tdmmc->dev, "Error, MCDMA RAM initialization timout\n");
			ret = -ETIME;
			goto err_mcdma_desc;
		}
	}

	/***************************************************************/
	/* MCDMA Configuration(use default MCDMA linked-list settings) */
	/***************************************************************/
	/* Set Rx Service Queue Arbiter Weight Register */
	writel((readl(tdmmc->regs + RX_SERVICE_QUEUE_ARBITER_WEIGHT_REG) & ~(0x1f << 24)),
	       tdmmc->regs + RX_SERVICE_QUEUE_ARBITER_WEIGHT_REG);

	/* Set Tx Service Queue Arbiter Weight Register */
	writel((readl(tdmmc->regs + TX_SERVICE_QUEUE_ARBITER_WEIGHT_REG) & ~(0x1f << 24)),
	       tdmmc->regs + TX_SERVICE_QUEUE_ARBITER_WEIGHT_REG);

	for (chan = 0; chan < tdmmc->total_channels; chan++) {
		/* Set RMCCx */
		writel(CONFIG_RMCCx, tdmmc->regs + MCDMA_RECEIVE_CONTROL_REG(chan));

		/* Set TMCCx */
		writel(CONFIG_TMCCx, tdmmc->regs + MCDMA_TRANSMIT_CONTROL_REG(chan));
	}

	/**********************/
	/* MCSC Configuration */
	/**********************/
	/* Disable Rx/Tx channel balancing & Linear mode fix */
	mv_phone_set_bit(tdmmc->regs + MCSC_GLOBAL_CONFIG_REG, MCSC_GLOBAL_CONFIG_TCBD_MASK);

	for (chan = 0; chan < tdmmc->total_channels; chan++) {
		writel(CONFIG_MRCRx, tdmmc->regs + MCSC_CHx_RECEIVE_CONFIG_REG(chan));
		writel(CONFIG_MTCRx, tdmmc->regs + MCSC_CHx_TRANSMIT_CONFIG_REG(chan));
	}

	/* Enable RX/TX linear byte swap, only in linear mode */
	if (tdm_params->pcm_format == MV_PCM_FORMAT_1BYTE)
		writel((readl(tdmmc->regs + MCSC_GLOBAL_CONFIG_EXTENDED_REG) & (~CONFIG_LINEAR_BYTE_SWAP)),
		       tdmmc->regs + MCSC_GLOBAL_CONFIG_EXTENDED_REG);
	else
		writel((readl(tdmmc->regs + MCSC_GLOBAL_CONFIG_EXTENDED_REG) | CONFIG_LINEAR_BYTE_SWAP),
		       tdmmc->regs + MCSC_GLOBAL_CONFIG_EXTENDED_REG);

	/***********************************************/
	/* Shared Bus to Crossbar Bridge Configuration */
	/***********************************************/
	/* Set Timeout Counter Register */
	writel((readl(tdmmc->regs + TIME_OUT_COUNTER_REG) | TIME_OUT_THRESHOLD_COUNT_MASK),
	       tdmmc->regs + TIME_OUT_COUNTER_REG);

	/*************************************************/
	/* Time Division Multiplexing(TDM) Configuration */
	/*************************************************/
	act_dpram_entry = kmalloc(sizeof(struct tdmmc_dram_entry), GFP_KERNEL);
	if (!act_dpram_entry) {
		ret = -EINVAL;
		goto err_mcdma_desc;
	}

	memcpy(act_dpram_entry, &def_dpram_entry, sizeof(struct tdmmc_dram_entry));
	/* Set repeat mode bits for (tdmmc->sample_size > 1) */
	act_dpram_entry->rpt = ((tdmmc->sample_size == MV_PCM_FORMAT_1BYTE) ? 0 : 1);

	/* Reset all Rx/Tx DPRAM entries to default value */
	for (index = 0; index < (2 * MV_TDM_MAX_HALF_DPRAM_ENTRIES); index++) {
		writel(*((u32 *) act_dpram_entry), tdmmc->regs + FLEX_TDM_RDPR_REG(index));
		writel(*((u32 *) act_dpram_entry), tdmmc->regs + FLEX_TDM_TDPR_REG(index));
	}

	/* Set active Rx/Tx DPRAM entries */
	for (chan = 0; chan < tdmmc->total_channels; chan++) {
		/* Same time slot number for both Rx & Tx */
		pcm_slot = tdm_params->pcm_slot[chan];

		/* Verify time slot is within frame boundries */
		if (pcm_slot >= frame_ts) {
			dev_err(tdmmc->dev, "Error, time slot(%d) exceeded maximum(%d)\n",
				pcm_slot, frame_ts);
			ret = -ETIME;
			goto err_dpram;
		}

		/* Verify time slot is aligned to sample size */
		if ((tdmmc->sample_size > MV_PCM_FORMAT_1BYTE) && (pcm_slot & 1)) {
			dev_err(tdmmc->dev, "Error, time slot(%d) not aligned to Linear PCM sample size\n",
				pcm_slot);
			ret = -EINVAL;
			goto err_dpram;
		}

		/* Update relevant DPRAM fields */
		act_dpram_entry->ch = chan;
		act_dpram_entry->mask = 0xff;

		/* Extract physical DPRAM entry id */
		index = ((tdmmc->sample_size == MV_PCM_FORMAT_1BYTE) ? pcm_slot : (pcm_slot / 2));

		/* DPRAM low half */
		writel(*((u32 *) act_dpram_entry), tdmmc->regs + FLEX_TDM_RDPR_REG(index));
		writel(*((u32 *) act_dpram_entry), tdmmc->regs + FLEX_TDM_TDPR_REG(index));

		/* DPRAM high half(mirroring DPRAM low half) */
		act_dpram_entry->mask = 0;
		writel(*((u32 *) act_dpram_entry),
		       tdmmc->regs + FLEX_TDM_RDPR_REG((MV_TDM_MAX_HALF_DPRAM_ENTRIES + index)));
		writel(*((u32 *) act_dpram_entry),
		       tdmmc->regs + FLEX_TDM_TDPR_REG((MV_TDM_MAX_HALF_DPRAM_ENTRIES + index)));

		/* WideBand mode */
		if (tdmmc->sample_size == MV_PCM_FORMAT_4BYTES) {
			index = (index + (frame_ts / tdmmc->sample_size));
			/* DPRAM low half */
			act_dpram_entry->mask = 0xff;
			writel(*((u32 *) act_dpram_entry), tdmmc->regs + FLEX_TDM_RDPR_REG(index));
			writel(*((u32 *) act_dpram_entry), tdmmc->regs + FLEX_TDM_TDPR_REG(index));

			/* DPRAM high half(mirroring DPRAM low half) */
			act_dpram_entry->mask = 0;
			writel(*((u32 *) act_dpram_entry),
			       tdmmc->regs + FLEX_TDM_RDPR_REG((MV_TDM_MAX_HALF_DPRAM_ENTRIES + index)));
			writel(*((u32 *) act_dpram_entry),
			       tdmmc->regs + FLEX_TDM_TDPR_REG((MV_TDM_MAX_HALF_DPRAM_ENTRIES + index)));
		}
	}

	/* Fill last Tx/Rx DPRAM entry('LAST'=1) */
	act_dpram_entry->mask = 0;
	act_dpram_entry->ch = 0;
	act_dpram_entry->last = 1;

	/* Index for last entry */
	if (tdmmc->sample_size == MV_PCM_FORMAT_1BYTE)
		index = (frame_ts - 1);
	else
		index = ((frame_ts / 2) - 1);

	/* Low half */
	writel(*((u32 *) act_dpram_entry), tdmmc->regs + FLEX_TDM_TDPR_REG(index));
	writel(*((u32 *) act_dpram_entry), tdmmc->regs + FLEX_TDM_RDPR_REG(index));
	/* High half */
	writel(*((u32 *) act_dpram_entry),
	       tdmmc->regs + FLEX_TDM_TDPR_REG((MV_TDM_MAX_HALF_DPRAM_ENTRIES + index)));
	writel(*((u32 *) act_dpram_entry),
	       tdmmc->regs + FLEX_TDM_RDPR_REG((MV_TDM_MAX_HALF_DPRAM_ENTRIES + index)));

	/* Set TDM_CLK_AND_SYNC_CONTROL register */
	clk_sync_ctrl_reg = readl(tdmmc->regs + TDM_CLK_AND_SYNC_CONTROL_REG);
	clk_sync_ctrl_reg &= ~(TDM_TX_FSYNC_OUT_ENABLE_MASK | TDM_RX_FSYNC_OUT_ENABLE_MASK |
			TDM_TX_CLK_OUT_ENABLE_MASK | TDM_RX_CLK_OUT_ENABLE_MASK);
	clk_sync_ctrl_reg |= CONFIG_TDM_CLK_AND_SYNC_CONTROL;
	writel(clk_sync_ctrl_reg, tdmmc->regs + TDM_CLK_AND_SYNC_CONTROL_REG);

	/* Set TDM TCR register */
	writel((readl(tdmmc->regs + FLEX_TDM_CONFIG_REG) | CONFIG_FLEX_TDM_CONFIG),
	       tdmmc->regs + FLEX_TDM_CONFIG_REG);

	/**********************************************************************/
	/* Time Division Multiplexing(TDM) Interrupt Controller Configuration */
	/**********************************************************************/
	/* Clear TDM cause and mask registers */
	writel(0, tdmmc->regs + COMM_UNIT_TOP_MASK_REG);
	writel(0, tdmmc->regs + TDM_MASK_REG);
	writel(0, tdmmc->regs + COMM_UNIT_TOP_CAUSE_REG);
	writel(0, tdmmc->regs + TDM_CAUSE_REG);

	/* Clear MCSC cause and mask registers(except InitDone bit) */
	writel(0, tdmmc->regs + MCSC_GLOBAL_INT_MASK_REG);
	writel(0, tdmmc->regs + MCSC_EXTENDED_INT_MASK_REG);
	writel(MCSC_GLOBAL_INT_CAUSE_INIT_DONE_MASK, tdmmc->regs + MCSC_GLOBAL_INT_CAUSE_REG);
	writel(0, tdmmc->regs + MCSC_EXTENDED_INT_CAUSE_REG);

	/* Set output sync counter bits for FS */
	count = frame_ts * 8;
	writel(((count << TDM_SYNC_BIT_RX_OFFS) & TDM_SYNC_BIT_RX_MASK) | (count & TDM_SYNC_BIT_TX_MASK),
	       tdmmc->regs + TDM_OUTPUT_SYNC_BIT_COUNT_REG);

	tdmmc_show();

	/* Enable PCM */
	tdmmc_pcm_start();

	/* Mark TDM I/F as enabled */
	tdmmc->tdm_enable = true;

	/* Enable PCLK */
	writel((readl(tdmmc->regs + TDM_DATA_DELAY_AND_CLK_CTRL_REG) | CONFIG_TDM_DATA_DELAY_AND_CLK_CTRL),
	       tdmmc->regs + TDM_DATA_DELAY_AND_CLK_CTRL_REG);

	/* Keep the software workaround to enable TEN while set Fsync for none-ALP chips */
	/* Enable TDM */
	if (tdmmc->ip_ver == TDMMC_REV0)
		mv_phone_set_bit(tdmmc->regs + FLEX_TDM_CONFIG_REG, TDM_TEN_MASK);

	dev_dbg(tdmmc->dev, "%s: Exit\n", __func__);

	kfree(act_dpram_entry);
	return 0;

err_dpram:
	kfree(act_dpram_entry);
err_mcdma_desc:
	for (index = 0; index < TOTAL_CHAINS; index++) {
		if (tdmmc->rx_desc_virt[index])
			dma_free_coherent(tdmmc->dev, total_rx_desc_size,
					  tdmmc->rx_desc_virt[index], tdmmc->rx_desc_phys[index]);
		if (tdmmc->tx_desc_virt[index])
			dma_free_coherent(tdmmc->dev, total_tx_desc_size,
					  tdmmc->tx_desc_virt[index], tdmmc->tx_desc_phys[index]);
	}
err_buff_virt:
	for (index = 0; index < TOTAL_CHAINS; index++) {
		if (tdmmc->rx_buff_phys[index])
			dma_free_coherent(tdmmc->dev, buff_size, tdmmc->rx_buff_virt[index],
					  tdmmc->rx_buff_phys[index]);
		if (tdmmc->tx_buff_phys[index])
			dma_free_coherent(tdmmc->dev, buff_size, tdmmc->tx_buff_virt[index],
					  tdmmc->tx_buff_phys[index]);
	}

	return ret;
}

void tdmmc_release(void)
{
	u32 buff_size, total_rx_desc_size, total_tx_desc_size, index;

	if (tdmmc->tdm_enable) {

		/* Mark TDM I/F as disabled */
		tdmmc->tdm_enable = false;

		tdmmc_pcm_stop();

		tdmmc_mcdma_mcsc_abort();

		udelay(10);
		mv_phone_reset_bit(tdmmc->regs + MCSC_GLOBAL_CONFIG_REG, MCSC_GLOBAL_CONFIG_MAI_MASK);

		/* Disable TDM */
		if (tdmmc->ip_ver == TDMMC_REV0)
			mv_phone_reset_bit(tdmmc->regs + FLEX_TDM_CONFIG_REG, TDM_TEN_MASK);

		/* Disable PCLK */
		mv_phone_reset_bit(tdmmc->regs + TDM_DATA_DELAY_AND_CLK_CTRL_REG,
				   (TX_CLK_OUT_ENABLE_MASK |
				    RX_CLK_OUT_ENABLE_MASK));

		/* Calculate total Rx/Tx buffer size */
		buff_size = (tdmmc->sample_size * MV_TDM_TOTAL_CH_SAMPLES *
			     tdmmc->sampling_coeff * tdmmc->total_channels);

		/* Calculate total MCDMA Rx/Tx descriptors chain size */
		total_rx_desc_size = tdmmc->total_channels * sizeof(struct tdmmc_mcdma_rx_desc);
		total_tx_desc_size = tdmmc->total_channels * sizeof(struct tdmmc_mcdma_tx_desc);

		for (index = 0; index < TOTAL_CHAINS; index++) {
			/* Release Rx/Tx data buffers */
			dma_free_coherent(tdmmc->dev, buff_size, tdmmc->rx_buff_virt[index],
					  tdmmc->rx_buff_phys[index]);
			dma_free_coherent(tdmmc->dev, buff_size, tdmmc->tx_buff_virt[index],
					  tdmmc->tx_buff_phys[index]);

			/* Release MCDMA Rx/Tx descriptors */
			dma_free_coherent(tdmmc->dev, total_rx_desc_size,
					  tdmmc->rx_desc_virt[index], tdmmc->rx_desc_phys[index]);
			dma_free_coherent(tdmmc->dev, total_tx_desc_size,
					  tdmmc->tx_desc_virt[index], tdmmc->tx_desc_phys[index]);
		}
	}
}

void tdmmc_pcm_start(void)
{
	u32 mask_reg;

	if (!tdmmc->pcm_enable) {

		/* Mark PCM I/F as enabled  */
		tdmmc->pcm_enable = true;

		tdmmc_mcdma_mcsc_start();

		/* Clear TDM cause and mask registers */
		writel(0, tdmmc->regs + COMM_UNIT_TOP_MASK_REG);
		writel(0, tdmmc->regs + TDM_MASK_REG);
		writel(0, tdmmc->regs + COMM_UNIT_TOP_CAUSE_REG);
		writel(0, tdmmc->regs + TDM_CAUSE_REG);

		/* Clear MCSC cause and mask registers(except InitDone bit) */
		writel(0, tdmmc->regs + MCSC_GLOBAL_INT_MASK_REG);
		writel(0, tdmmc->regs + MCSC_EXTENDED_INT_MASK_REG);
		writel(MCSC_GLOBAL_INT_CAUSE_INIT_DONE_MASK, tdmmc->regs + MCSC_GLOBAL_INT_CAUSE_REG);
		writel(0, tdmmc->regs + MCSC_EXTENDED_INT_CAUSE_REG);

		/* Enable unit interrupts */
		mask_reg = readl(tdmmc->regs + TDM_MASK_REG);
		writel(mask_reg | CONFIG_TDM_CAUSE, tdmmc->regs + TDM_MASK_REG);
		writel(CONFIG_COMM_UNIT_TOP_MASK, tdmmc->regs + COMM_UNIT_TOP_MASK_REG);

		/* Enable TDM */
		if (tdmmc->ip_ver == TDMMC_REV1)
			mv_phone_set_bit(tdmmc->regs + FLEX_TDM_CONFIG_REG, TDM_TEN_MASK);
	}
}

void tdmmc_pcm_stop(void)
{
	u32 buff_size, index;

	if (tdmmc->pcm_enable) {
		/* Mark PCM I/F as disabled  */
		tdmmc->pcm_enable = false;

		/* Clear TDM cause and mask registers */
		writel(0, tdmmc->regs + COMM_UNIT_TOP_MASK_REG);
		writel(0, tdmmc->regs + TDM_MASK_REG);
		writel(0, tdmmc->regs + COMM_UNIT_TOP_CAUSE_REG);
		writel(0, tdmmc->regs + TDM_CAUSE_REG);

		/* Clear MCSC cause and mask registers(except InitDone bit) */
		writel(0, tdmmc->regs + MCSC_GLOBAL_INT_MASK_REG);
		writel(0, tdmmc->regs + MCSC_EXTENDED_INT_MASK_REG);
		writel(MCSC_GLOBAL_INT_CAUSE_INIT_DONE_MASK, tdmmc->regs + MCSC_GLOBAL_INT_CAUSE_REG);
		writel(0, tdmmc->regs + MCSC_EXTENDED_INT_CAUSE_REG);

		tdmmc_mcdma_stop();

		/* Calculate total Rx/Tx buffer size */
		buff_size = (tdmmc->sample_size * MV_TDM_TOTAL_CH_SAMPLES *
			     tdmmc->sampling_coeff * tdmmc->total_channels);

		/* Clear Rx buffers */
		for (index = 0; index < TOTAL_CHAINS; index++)
			memset(tdmmc->rx_buff_virt[index], 0, buff_size);

		/* Disable TDM */
		if (tdmmc->ip_ver == TDMMC_REV1)
			mv_phone_reset_bit(tdmmc->regs + FLEX_TDM_CONFIG_REG, TDM_TEN_MASK);
	}
}

int tdmmc_tx(u8 *tdm_tx_buff)
{
	u32 buff_size, index;
	u8 tmp;

	/* Calculate total Tx buffer size */
	buff_size = (tdmmc->sample_size * MV_TDM_TOTAL_CH_SAMPLES *
		     tdmmc->sampling_coeff * tdmmc->total_channels);

	if (tdmmc->ip_ver == TDMMC_REV0) {
		if (tdmmc->sample_size > MV_PCM_FORMAT_1BYTE) {
			dev_dbg(tdmmc->dev, "Linear mode (Tx): swapping bytes\n");
			for (index = 0; index < buff_size; index += 2) {
				tmp = tdm_tx_buff[index];
				tdm_tx_buff[index] = tdm_tx_buff[index+1];
				tdm_tx_buff[index+1] = tmp;
			}
			dev_dbg(tdmmc->dev, "Linear mode (Tx): swapping bytes...done.\n");
		}
	}

	return 0;
}

int tdmmc_rx(u8 *tdm_rx_buff)
{
	u32 buff_size, index;
	u8 tmp;

	/* Calculate total Rx buffer size */
	buff_size = (tdmmc->sample_size * MV_TDM_TOTAL_CH_SAMPLES *
		     tdmmc->sampling_coeff * tdmmc->total_channels);

	if (tdmmc->ip_ver == TDMMC_REV0) {
		if (tdmmc->sample_size > MV_PCM_FORMAT_1BYTE) {
			dev_dbg(tdmmc->dev, "Linear mode (Rx): swapping bytes\n");
			for (index = 0; index < buff_size; index += 2) {
				tmp = tdm_rx_buff[index];
				tdm_rx_buff[index] = tdm_rx_buff[index+1];
				tdm_rx_buff[index+1] = tmp;
			}
			dev_dbg(tdmmc->dev, "Linear mode (Rx): swapping bytes...done.\n");
		}
	}

	return 0;
}

/* Low level TDM interrupt service routine */
int tdmmc_intr_low(struct mv_phone_intr_info *tdm_intr_info)
{
	u32 cause_reg, mask_reg, cause_and_mask, curr_desc, int_ack_bits = 0;
	u8 index;

	/* Read TDM cause & mask registers */
	cause_reg = readl(tdmmc->regs + TDM_CAUSE_REG);
	mask_reg = readl(tdmmc->regs + TDM_MASK_REG);

	dev_dbg(tdmmc->dev, "%s: Cause register = 0x%x, Mask register = 0x%x\n",
		__func__, cause_reg, mask_reg);

	/* Refer only to unmasked bits */
	cause_and_mask = cause_reg & mask_reg;

	/* Reset ISR params */
	tdm_intr_info->tdm_rx_buff = NULL;
	tdm_intr_info->tdm_tx_buff = NULL;
	tdm_intr_info->int_type = MV_EMPTY_INT;

	/* Return in case TDM is disabled */
	if (!tdmmc->tdm_enable) {
		dev_dbg(tdmmc->dev, "%s: TDM is disabled - quit low lever ISR\n", __func__);
		writel(~int_ack_bits, tdmmc->regs + TDM_CAUSE_REG);
		return 0;
	}

	/* Handle TDM Error/s */
	if (cause_and_mask & TDM_ERROR_INT) {
		dev_err(tdmmc->dev, "TDM Error: TDM_CAUSE_REG = 0x%x\n", cause_reg);
		int_ack_bits |= (int_ack_bits & TDM_ERROR_INT);
	}

	if (cause_and_mask & (TDM_TX_INT | TDM_RX_INT)) {
		/* MCDMA current Tx desc. pointer is unreliable, thus, checking Rx desc. pointer only */
		curr_desc = readl(tdmmc->regs + MCDMA_CURRENT_RECEIVE_DESC_PTR_REG(0));
		dev_dbg(tdmmc->dev, "%s: current descriptor = 0x%x\n", __func__, curr_desc);

		/* Handle Tx */
		if (cause_and_mask & TDM_TX_INT) {
			for (index = 0; index < TOTAL_CHAINS; index++) {
				if (curr_desc == tdmmc->rx_desc_phys[index]) {
					tdmmc->next_tx = NEXT_BUFF(index);
					break;
				}
			}
			dev_dbg(tdmmc->dev, "%s: TX interrupt (next_tx_buff = %d\n",
				__func__, tdmmc->next_tx);
			tdm_intr_info->tdm_tx_buff = tdmmc->tx_buff_virt[tdmmc->next_tx];
			tdm_intr_info->int_type |= MV_TX_INT;
			int_ack_bits |= TDM_TX_INT;
		}

		/* Handle Rx */
		if (cause_and_mask & TDM_RX_INT) {
			for (index = 0; index < TOTAL_CHAINS; index++) {
				if (curr_desc == tdmmc->rx_desc_phys[index]) {
					tdmmc->prev_rx = PREV_BUFF(index);
					break;
				}
			}
			dev_dbg(tdmmc->dev, "%s: RX interrupt (prev_rx_buff = %d)\n",
				__func__, tdmmc->prev_rx);
			tdm_intr_info->tdm_rx_buff = tdmmc->rx_buff_virt[tdmmc->prev_rx];
			tdm_intr_info->int_type |= MV_RX_INT;
			int_ack_bits |= TDM_RX_INT;
		}
	}

	/* Clear TDM interrupts */
	writel(~int_ack_bits, tdmmc->regs + TDM_CAUSE_REG);

	return 0;
}

int tdmmc_reset_slic(void)
{
	/* Enable SLIC reset */
	mv_phone_reset_bit(tdmmc->regs + TDM_CLK_AND_SYNC_CONTROL_REG, TDM_PROG_TDM_SLIC_RESET_MASK);

	udelay(60);

	/* Release SLIC reset */
	mv_phone_set_bit(tdmmc->regs + TDM_CLK_AND_SYNC_CONTROL_REG, TDM_PROG_TDM_SLIC_RESET_MASK);

	return 0;
}

/* Initialize decoding windows */
int tdmmc_set_mbus_windows(struct device *dev, void __iomem *regs)
{
	const struct mbus_dram_target_info *dram = mv_mbus_dram_info();
	u32 win_protect, win_enable;
	int i;

	if (!dram) {
		dev_err(dev, "no mbus dram info\n");
		return -EINVAL;
	}

	for (i = 0; i < COMM_UNIT_MBUS_MAX_WIN; i++) {
		writel(0, regs + COMM_UNIT_WIN_CTRL_REG(i));
		writel(0, regs + COMM_UNIT_WIN_SIZE_REG(i));
		writel(0, regs + COMM_UNIT_WIN_ENABLE_REG(i));
	}

	win_enable = 0xff;
	win_protect = 0;

	for (i = 0; i < dram->num_cs; i++) {
		const struct mbus_dram_window *cs = dram->cs + i;

		writel((cs->base & 0xffff0000) |
		       (cs->mbus_attr << 8) |
		       dram->mbus_dram_target_id,
		       regs + COMM_UNIT_WIN_CTRL_REG(i));

		writel((cs->size - 1) & 0xffff0000,
		       regs + COMM_UNIT_WIN_SIZE_REG(i));

		writel(win_enable, regs + COMM_UNIT_WIN_ENABLE_REG(i));
		win_protect |= 3 << (2 * i);
	}

	/* Configure an extra window for PCIE0 */
	writel(0x8000e804, regs + COMM_UNIT_WIN_CTRL_REG(i));
	writel(0x1fff0000, regs + COMM_UNIT_WIN_SIZE_REG(i));
	writel(win_enable, regs + COMM_UNIT_WIN_ENABLE_REG(i));
	win_protect |= 3 << (2 * i);

	writel(win_protect, regs + COMM_UNIT_WINDOWS_ACCESS_PROTECT_REG);

	return 0;
}

/* Initialize decoding windows for Armada 8k SoC */
int tdmmc_set_a8k_windows(struct device *dev, void __iomem *regs)
{
	int i;

	for (i = 0; i < COMM_UNIT_MBUS_MAX_WIN; i++) {
		writel(0xce00, regs + COMM_UNIT_WIN_CTRL_REG(i));
		writel(0xffff0000, regs + COMM_UNIT_WIN_SIZE_REG(i));
		if (i > 0)
			writel(0x0, regs + COMM_UNIT_WIN_ENABLE_REG(i));
	}

	return 0;
}
