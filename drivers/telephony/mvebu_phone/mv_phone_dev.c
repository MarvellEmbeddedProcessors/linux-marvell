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

#include <linux/init.h>
#include <linux/interrupt.h>
#include <linux/module.h>
#include <linux/proc_fs.h>
#include <slic/drv_dxt_if.h>
#include <slic/silabs_if.h>
#include <slic/zarlink_if.h>
#include <tal/tal.h>
#include <tal/tal_dev.h>
#include "mv_phone.h"

#define DRV_NAME "mvebu_phone"

/* TDM Interrupt Service Routine */
static irqreturn_t tdm_if_isr(int irq, void *dev_id);

/* Rx/Tx Tasklets  */
static void tdm2c_if_pcm_rx_process(unsigned long arg);
static void tdmmc_if_pcm_rx_process(unsigned long arg);
static void tdm2c_if_pcm_tx_process(unsigned long arg);
static void tdmmc_if_pcm_tx_process(unsigned long arg);
static void tdm2c_if_reset_channels(unsigned long arg);

/* Globals */
static struct mv_phone_dev *priv;
static DECLARE_TASKLET(tdm2c_if_rx_tasklet, tdm2c_if_pcm_rx_process, 0);
static DECLARE_TASKLET(tdmmc_if_rx_tasklet, tdmmc_if_pcm_rx_process, 0);
static DECLARE_TASKLET(tdm2c_if_tx_tasklet, tdm2c_if_pcm_tx_process, 0);
static DECLARE_TASKLET(tdmmc_if_tx_tasklet, tdmmc_if_pcm_tx_process, 0);
static DECLARE_TASKLET(tdm2c_if_reset_tasklet, tdm2c_if_reset_channels, 0);

/* Statistic printout in userspace via /proc/tdm */
static int mv_phone_status_show(struct seq_file *m, void *v)
{
	struct mv_phone_extended_stats tdm_ext_stats;

	seq_printf(m, "tdm_init:	%u\n", priv->tdm_init);
	seq_printf(m, "rx_miss:		%u\n", priv->rx_miss);
	seq_printf(m, "tx_miss:		%u\n", priv->tx_miss);
	seq_printf(m, "rx_over:		%u\n", priv->rx_over);
	seq_printf(m, "tx_under:	%u\n", priv->tx_under);

	if (!priv->use_tdm_ext_stats)
		return 0;

	tdm2c_ext_stats_get(&tdm_ext_stats);

	seq_puts(m, "\nTDM Extended Statistics:\n");
	seq_printf(m, "int_rx_count	= %u\n", tdm_ext_stats.int_rx_count);
	seq_printf(m, "int_tx_count	= %u\n", tdm_ext_stats.int_tx_count);
	seq_printf(m, "int_rx0_count	= %u\n", tdm_ext_stats.int_rx0_count);
	seq_printf(m, "int_tx0_count	= %u\n", tdm_ext_stats.int_tx0_count);
	seq_printf(m, "int_rx1_count	= %u\n", tdm_ext_stats.int_rx1_count);
	seq_printf(m, "int_tx1_count	= %u\n", tdm_ext_stats.int_tx1_count);
	seq_printf(m, "int_rx0_miss	= %u\n", tdm_ext_stats.int_rx0_miss);
	seq_printf(m, "int_tx0_miss	= %u\n", tdm_ext_stats.int_tx0_miss);
	seq_printf(m, "int_tx1_miss	= %u\n", tdm_ext_stats.int_rx1_miss);
	seq_printf(m, "int_tx1_miss	= %u\n", tdm_ext_stats.int_tx1_miss);
	seq_printf(m, "pcm_restart_count= %u\n", tdm_ext_stats.pcm_restart_count);
	seq_printf(m, "pcm_stop_fail	= %u\n", priv->pcm_stop_fail);

	return 0;
}

static int mv_phone_status_open(struct inode *inode, struct file *file)
{
	return single_open(file, mv_phone_status_show, PDE_DATA(inode));
}

static const struct file_operations mv_phone_operations = {
	.open		= mv_phone_status_open,
	.read		= seq_read,
	.llseek		= seq_lseek,
	.release	= seq_release,
};

/* TAL callbacks */

/* PCM start */
static void tdm2c_if_pcm_start(void)
{
	unsigned long flags;
	u32 max_poll = 0;

	spin_lock_irqsave(&priv->lock, flags);

	if (priv->pcm_enable) {
		spin_unlock_irqrestore(&priv->lock, flags);
		return;
	}

	priv->pcm_enable = true;
	if (!priv->pcm_is_stopping) {
		priv->pcm_stop_flag = false;
		priv->pcm_stop_status = false;
		priv->pcm_start_stop_state = 0;
		priv->rx_buff = NULL;
		priv->tx_buff = NULL;
		tdm2c_pcm_start();
	} else {
		priv->pcm_start_stop_state++;
		while (priv->pcm_is_stopping && max_poll < MV_TDM_STOP_POLLING_TIMEOUT) {
			spin_unlock_irqrestore(&priv->lock, flags);
			mdelay(1);
			max_poll++;
			spin_lock_irqsave(&priv->lock, flags);
		}

		if (priv->pcm_is_stopping) {
			/* Issue found or timeout */
			if (tdm2c_pcm_stop_int_miss())
				dev_dbg(priv->dev, "pcm stop issue found\n");
			else
				dev_dbg(priv->dev, "pcm stop timeout\n");

			priv->pcm_is_stopping = false;
			priv->pcm_stop_flag = false;
			priv->pcm_stop_status = false;
			priv->pcm_start_stop_state = 0;
			priv->rx_buff = NULL;
			priv->tx_buff = NULL;
			tdm2c_pcm_start();
		} else {
			dev_dbg(priv->dev, "pcm_start_stop_state(%d), max_poll=%d\n",
				priv->pcm_start_stop_state, max_poll);
		}
	}

	spin_unlock_irqrestore(&priv->lock, flags);
}

static void tdmmc_if_pcm_start(void)
{
	unsigned long flags;

	spin_lock_irqsave(&priv->lock, flags);

	if (priv->pcm_enable) {
		spin_unlock_irqrestore(&priv->lock, flags);
		return;
	}

	priv->pcm_enable = true;
	priv->rx_buff = NULL;
	priv->tx_buff = NULL;
	tdmmc_pcm_start();

	spin_unlock_irqrestore(&priv->lock, flags);
}

/* PCM stop */
static void tdm2c_if_pcm_stop(void)
{
	unsigned long flags;

	spin_lock_irqsave(&priv->lock, flags);

	if (!priv->pcm_enable) {
		spin_unlock_irqrestore(&priv->lock, flags);
		return;
	}

	priv->pcm_enable = false;
	if (!priv->pcm_is_stopping) {
		priv->pcm_is_stopping = true;
		tdm2c_pcm_stop();
	} else {
		priv->pcm_start_stop_state--;
		dev_dbg(priv->dev, "pcm_start_stop_state(%d)\n",
			priv->pcm_start_stop_state);
	}

	spin_unlock_irqrestore(&priv->lock, flags);
}

static void tdmmc_if_pcm_stop(void)
{
	unsigned long flags;

	spin_lock_irqsave(&priv->lock, flags);

	if (!priv->pcm_enable) {
		spin_unlock_irqrestore(&priv->lock, flags);
		return;
	}

	priv->pcm_enable = false;
	tdmmc_pcm_stop();

	spin_unlock_irqrestore(&priv->lock, flags);
}

/* TDM low-level initialization */
static int tdm_hw_init(struct mv_phone_params *tdm_params)
{
	enum mv_phone_frame_ts frame_ts;
	int ret;

	switch (priv->pclk_freq_mhz) {
	case 8:
		frame_ts = MV_FRAME_128TS;
		break;
	case 4:
		frame_ts = MV_FRAME_64TS;
		break;
	case 2:
		frame_ts = MV_FRAME_32TS;
		break;
	default:
		frame_ts = MV_FRAME_128TS;
		break;
	}

	switch (priv->tdm_type) {
	case MV_TDM_UNIT_TDM2C:
		ret = tdm2c_init(priv->tdm_base, priv->dev, tdm_params,
				 frame_ts, priv->tdm2c_spi_mode,
				 priv->use_pclk_external);

		/* Soft reset to PCM I/F */
		tdm2c_pcm_if_reset();

		break;
	case MV_TDM_UNIT_TDMMC:
		ret = tdmmc_init(priv->tdm_base, priv->dev, tdm_params,
				 frame_ts, priv->tdmmc_ip_ver);

		/* Issue SLIC reset */
		ret |= tdmmc_reset_slic();

		/* WA to stop the MCDMA gracefully after tdmmc initialization */
		tdmmc_if_pcm_stop();

		break;
	default:
		dev_err(&priv->parent->dev, "%s: undefined TDM type\n",
			__func__);
		return -EINVAL;
	}

	priv->tdm_params = tdm_params;

	return ret;
}

/* Main TDM initialization routine */
int tdm_if_init(struct tal_params *tal_params)
{
	struct mv_phone_params tdm_params;
	int i, irqs_requested, ret;

	if (priv->tdm_init) {
		dev_warn(priv->dev, "Marvell Telephony Driver already started...\n");
		return 0;
	}

	dev_info(priv->dev, "Loading Marvell Telephony Driver\n");

	if (!tal_params) {
		dev_err(priv->dev, "%s: bad parameters\n", __func__);
		return -EINVAL;

	}

	/* Reset operation flags */
	priv->tdm_init = false;
	priv->pcm_enable = false;
	priv->pcm_is_stopping = false;
	priv->pcm_stop_flag = false;
	priv->pcm_stop_status = false;
	priv->pcm_stop_fail = 0;

	/* Calculate Rx/Tx buffer size(use in callbacks) */
	priv->buff_size = (tal_params->pcm_format * tal_params->total_lines * 80 *
			  (tal_params->sampling_period/MV_TDM_BASE_SAMPLING_PERIOD));

	/* Assign TDM parameters */
	memcpy(&tdm_params, tal_params, sizeof(struct mv_phone_params));

	/* TDM hardware initialization */
	ret = tdm_hw_init(&tdm_params);
	if (ret) {
		dev_err(priv->dev, "%s: TDM initialization failed\n", __func__);
		return ret;
	}

	/* Create TDM procfs statistics */
	priv->tdm_stats = proc_mkdir("tdm", NULL);
	if (priv->tdm_stats) {
		if (!proc_create("tdm_stats", S_IRUGO,
				 priv->tdm_stats, &mv_phone_operations))
			return -ENOMEM;
	}

	/* Register TDM interrupts */
	irqs_requested = 0;
	for (i = 0; i < priv->irq_count; i++) {
		ret = request_irq(priv->irq[i], tdm_if_isr, 0x0, "tdm", NULL);
		if (ret) {
			dev_err(priv->dev, "%s: Failed to connect irq(%d)\n",
				__func__, priv->irq[i]);
			goto err_irq;
		}
		irqs_requested++;
	}

	priv->tdm_init = true;

	return 0;

err_irq:
	for (i = 0; i < irqs_requested; i++)
		free_irq(priv->irq[i], NULL);

	return ret;
}

/* Disable TDM2C PCM */
void tdm2c_pcm_disable(void)
{
	u32 max_poll = 0;

	tdm2c_if_pcm_stop();

	while (priv->pcm_is_stopping && (max_poll < MV_TDM_STOP_POLLING_TIMEOUT)) {
		mdelay(1);
		max_poll++;
	}

	if (max_poll >= MV_TDM_STOP_POLLING_TIMEOUT)
		dev_warn(priv->dev, "\n%s: Channels disabling timeout (%dms)\n",
			 __func__, MV_TDM_STOP_POLLING_TIMEOUT);

}

/* Main TDM deinitialization routine */
void tdm_if_exit(void)
{
	int i;

	/* Check if already stopped */
	if (!priv->pcm_enable && !priv->tdm_init)
		return;

	/* Stop PCM channels */
	if (priv->pcm_enable) {
		switch (priv->tdm_type) {
		case MV_TDM_UNIT_TDM2C:
			tdm2c_pcm_disable();
			break;
		case MV_TDM_UNIT_TDMMC:
			tdmmc_if_pcm_stop();
			break;
		default:
			dev_err(&priv->parent->dev, "%s: undefined TDM type\n",
				__func__);
		}
	}

	/* Disable TDM and release resources */
	if (priv->tdm_init) {
		switch (priv->tdm_type) {
		case MV_TDM_UNIT_TDM2C:
			tdm2c_release();
			break;
		case MV_TDM_UNIT_TDMMC:
			tdmmc_release();
			break;
		default:
			dev_err(&priv->parent->dev, "%s: undefined TDM type\n",
				__func__);
		}

		/* Remove proc directory & entries */
		remove_proc_entry("tdm_stats", priv->tdm_stats);
		remove_proc_entry("tdm", NULL);

		/* Release interrupt */
		for (i = 0; i < priv->irq_count; i++)
			free_irq(priv->irq[i], NULL);

		priv->tdm_init = false;
	}
}

static int tdm_if_control(int cmd, void *arg)
{
	switch (cmd) {
	case TDM_DEV_TDM_TEST_MODE_ENABLE:
		priv->test_enable = true;
		break;

	case TDM_DEV_TDM_TEST_MODE_DISABLE:
		priv->test_enable = false;
		break;

	default:
		return -EINVAL;
	};

	return 0;
}

static int tdm2c_if_write(u8 *buffer, int size)
{
	if (priv->test_enable)
		return tdm2c_tx(buffer);

	return 0;
}

static int tdmmc_if_write(u8 *buffer, int size)
{
	if (priv->test_enable)
		return tdmmc_tx(buffer);

	return 0;
}

static void tdm_if_stats_get(struct tal_stats *tdm_if_stats)
{
	if (!priv->tdm_init)
		return;

	tdm_if_stats->tdm_init = priv->tdm_init;
	tdm_if_stats->rx_miss = priv->rx_miss;
	tdm_if_stats->tx_miss = priv->tx_miss;
	tdm_if_stats->rx_over = priv->rx_over;
	tdm_if_stats->tx_under = priv->tx_under;

	if (priv->use_tdm_ext_stats)
		tdm2c_ext_stats_get(&tdm_if_stats->tdm_ext_stats);
}

static struct tal_if tdm2c_if = {
	.pcm_start	= tdm2c_if_pcm_start,
	.pcm_stop	= tdm2c_if_pcm_stop,
	.init		= tdm_if_init,
	.exit		= tdm_if_exit,
	.control	= tdm_if_control,
	.write		= tdm2c_if_write,
	.stats_get	= tdm_if_stats_get,
};

static struct tal_if tdmmc_if = {
	.pcm_start	= tdmmc_if_pcm_start,
	.pcm_stop	= tdmmc_if_pcm_stop,
	.init		= tdm_if_init,
	.exit		= tdm_if_exit,
	.control	= tdm_if_control,
	.write		= tdmmc_if_write,
	.stats_get	= tdm_if_stats_get,
};

/* Interrupt handling and tasklet callbacks */

/* Common interrupt top-half handler */
static irqreturn_t tdm_if_isr(int irq, void *dev_id)
{
	struct mv_phone_intr_info tdm_int_info;
	struct tasklet_struct *tdm_rx_tasklet = NULL;
	struct tasklet_struct *tdm_tx_tasklet = NULL;
	u32 int_type;
	int ret = 0;

	/* Extract interrupt information from low level ISR */
	switch (priv->tdm_type) {
	case MV_TDM_UNIT_TDM2C:
		ret = tdm2c_intr_low(&tdm_int_info);
		tdm_rx_tasklet = &tdm2c_if_rx_tasklet;
		tdm_tx_tasklet = &tdm2c_if_tx_tasklet;
		break;
	case MV_TDM_UNIT_TDMMC:
		tdmmc_intr_low(&tdm_int_info);
		tdm_rx_tasklet = &tdmmc_if_rx_tasklet;
		tdm_tx_tasklet = &tdmmc_if_tx_tasklet;
		break;
	default:
		dev_err(&priv->parent->dev, "%s: undefined TDM type\n", __func__);
		return IRQ_NONE;
	}

	int_type = tdm_int_info.int_type;

	/* Nothing to do - return */
	if (int_type == MV_EMPTY_INT)
		return IRQ_HANDLED;

	/* Handle ZSI interrupts */
	if (mv_phone_get_slic_board_type() == MV_BOARD_SLIC_ZSI_ID)
		zarlink_if_zsi_interrupt();
	/* Handle ISI interrupts */
	else if (mv_phone_get_slic_board_type() == MV_BOARD_SLIC_ISI_ID)
		silabs_if_isi_interrupt();

	if (ret && !priv->pcm_stop_status)	{
		priv->pcm_stop_status = true;

		/* If Rx/Tx tasklets are already scheduled, let them do the work */
		if (!priv->rx_buff && !priv->tx_buff) {
			dev_dbg(priv->dev, "Stopping the TDM\n");
			tdm2c_if_pcm_stop();
			priv->pcm_stop_flag = false;
			tasklet_hi_schedule(&tdm2c_if_reset_tasklet);
		} else {
			dev_dbg(priv->dev, "Tasklet already runningstop_flag\n");
			priv->pcm_stop_flag = true;
		}
	}

	/* Restarting PCM, skip Rx/Tx handling */
	if (priv->pcm_stop_status)
		goto skip_rx_tx;

	/* Support multiple interrupt handling */
	/* RX interrupt */
	if (int_type & MV_RX_INT) {
		if (priv->rx_buff) {
			priv->rx_miss++;
			dev_dbg(priv->dev, "%s: Rx buffer not ready\n", __func__);
		} else {
			priv->rx_buff = tdm_int_info.tdm_rx_buff;
			/* Schedule Rx processing within SOFT_IRQ context */
			dev_dbg(priv->dev, "%s: schedule Rx tasklet\n", __func__);
			tasklet_hi_schedule(tdm_rx_tasklet);
		}
	}

	/* TX interrupt */
	if (int_type & MV_TX_INT) {
		if (priv->tx_buff) {
			priv->tx_miss++;
			dev_dbg(priv->dev, "%s: Tx buffer not ready\n", __func__);
		} else {
			priv->tx_buff = tdm_int_info.tdm_tx_buff;
			/* Schedule Tx processing within SOFT_IRQ context */
			dev_dbg(priv->dev, "%s: schedule Tx tasklet\n", __func__);
			tasklet_hi_schedule(tdm_tx_tasklet);
		}
	}


	/* TDM2CH PCM channels stop indication */
	if ((int_type & MV_CHAN_STOP_INT) && (tdm_int_info.data == 4)) {
		dev_dbg(priv->dev, "%s: Received MV_CHAN_STOP_INT indication\n",
			__func__);
		priv->pcm_is_stopping = false;
		if (priv->pcm_start_stop_state) {
			dev_dbg(priv->dev, "%s: Resetting controller\n", __func__);
			priv->pcm_enable = false;
			/* Issue SW reset */
			tasklet_hi_schedule(&tdm2c_if_reset_tasklet);
		}
	}

skip_rx_tx:
	/* PHONE interrupt, Lantiq specific */
	if (int_type & MV_PHONE_INT)
		drv_dxt_if_signal_interrupt();

	/* ERROR interrupt */
	if (int_type & MV_RX_ERROR_INT)
		priv->rx_over++;

	if (int_type & MV_TX_ERROR_INT)
		priv->tx_under++;

	return IRQ_HANDLED;
}

/* Rx tasklets */
static void tdm2c_if_pcm_rx_process(unsigned long arg)
{
	unsigned long flags;

	if (priv->pcm_enable) {
		if (!priv->rx_buff) {
			dev_warn(priv->dev, "%s: Error, empty Rx processing\n", __func__);
			return;
		}

		/* Fill TDM Rx aggregated buffer */
		if (tdm2c_rx(priv->rx_buff) == 0)
			/* Dispatch Rx handler */
			tal_mmp_rx(priv->rx_buff, priv->buff_size);
		else
			dev_warn(priv->dev, "%s: Could not fill Rx buffer\n", __func__);
	}

	spin_lock_irqsave(&priv->lock, flags);
	/* Clear Rx buff for next iteration */
	priv->rx_buff = NULL;
	spin_unlock_irqrestore(&priv->lock, flags);

	if (priv->pcm_stop_flag && !priv->tx_buff) {
		dev_dbg(priv->dev, "Stopping TDM from Rx tasklet\n");
		tdm2c_if_pcm_stop();
		spin_lock_irqsave(&priv->lock, flags);
		priv->pcm_stop_flag = false;
		spin_unlock_irqrestore(&priv->lock, flags);
		tasklet_hi_schedule(&tdm2c_if_reset_tasklet);
	}
}

static void tdmmc_if_pcm_rx_process(unsigned long arg)
{
	unsigned long flags;

	if (priv->pcm_enable) {
		if (!priv->rx_buff) {
			dev_warn(priv->dev, "%s: Error, empty Rx processing\n", __func__);
			return;
		}

		if (tdmmc_rx(priv->rx_buff) == 0)
			/* Dispatch Rx handler */
			tal_mmp_rx(priv->rx_buff, priv->buff_size);
		else
			dev_warn(priv->dev, "%s: could not fill Rx buffer\n", __func__);
	}

	spin_lock_irqsave(&priv->lock, flags);
	/* Clear priv->rx_buff for next iteration */
	priv->rx_buff = NULL;
	spin_unlock_irqrestore(&priv->lock, flags);
}

/* Tx tasklets */
static void tdm2c_if_pcm_tx_process(unsigned long arg)
{
	unsigned long flags;

	if (priv->pcm_enable) {
		if (!priv->tx_buff) {
			dev_warn(priv->dev, "%s: Error, empty Tx processing\n", __func__);
			return;
		}

		/* Dispatch Tx handler */
		tal_mmp_tx(priv->tx_buff, priv->buff_size);

		if (!priv->test_enable) {
			/* Fill Tx aggregated buffer */
			if (tdm2c_tx(priv->tx_buff) != 0)
				dev_warn(priv->dev, "%s: Could not fill Tx buffer\n", __func__);
		}
	}

	spin_lock_irqsave(&priv->lock, flags);
	/* Clear Tx buff for next iteration */
	priv->tx_buff = NULL;
	spin_unlock_irqrestore(&priv->lock, flags);

	if (priv->pcm_stop_flag && !priv->rx_buff) {
		dev_dbg(priv->dev, "Stopping TDM from Tx tasklet\n");
		tdm2c_if_pcm_stop();
		spin_lock_irqsave(&priv->lock, flags);
		priv->pcm_stop_flag = false;
		spin_unlock_irqrestore(&priv->lock, flags);
		tasklet_hi_schedule(&tdm2c_if_reset_tasklet);
	}
}

static void tdmmc_if_pcm_tx_process(unsigned long arg)
{
	unsigned long flags;

	if (priv->pcm_enable) {
		if (!priv->tx_buff) {
			dev_warn(priv->dev, "%s: Error, empty Tx processing\n", __func__);
			return;
		}

		/* Dispatch Tx handler */
		tal_mmp_tx(priv->tx_buff, priv->buff_size);

		if (!priv->test_enable) {
			if (tdmmc_tx(priv->tx_buff) != 0)
				dev_warn(priv->dev, "%s: Could not fill Tx buffer\n", __func__);
		}
	}

	spin_lock_irqsave(&priv->lock, flags);
	/* Clear Tx buff for next iteration */
	priv->tx_buff = NULL;
	spin_unlock_irqrestore(&priv->lock, flags);
}

/* TDM2C restart channel callback */
static void tdm2c_if_reset_channels(unsigned long arg)
{
	u32 max_poll = 0;
	unsigned long flags;

	/* Wait for all channels to stop  */
	while (((readl(priv->tdm_base + CH_ENABLE_REG(0)) & CH_RXTX_EN_MASK) ||
		(readl(priv->tdm_base + CH_ENABLE_REG(1)) & CH_RXTX_EN_MASK)) &&
		(max_poll < MV_TDM_STOP_POLLING_TIMEOUT)) {

		mdelay(1);
		max_poll++;
	}

	dev_dbg(priv->dev, "Finished polling on channels disable\n");
	if (max_poll >= MV_TDM_STOP_POLLING_TIMEOUT) {
		writel(0, priv->tdm_base + CH_ENABLE_REG(0));
		writel(0, priv->tdm_base + CH_ENABLE_REG(1));
		dev_warn(priv->dev, "\n%s: Channels disabling timeout (%dms)\n",
			 __func__, MV_TDM_STOP_POLLING_TIMEOUT);
		priv->pcm_stop_fail++;
		mdelay(10);
	}

	spin_lock_irqsave(&priv->lock, flags);
	priv->pcm_is_stopping = false;
	spin_unlock_irqrestore(&priv->lock, flags);

	/* Restart channels */
	tdm2c_if_pcm_start();
}

/* Enable device interrupts. */
void mv_phone_intr_enable(u8 dev_id)
{
	switch (priv->tdm_type) {
	case MV_TDM_UNIT_TDM2C:
		tdm2c_intr_enable();
		break;
	case MV_TDM_UNIT_TDMMC:
		tdmmc_intr_enable(dev_id);
		break;
	default:
		dev_err(&priv->parent->dev, "%s: undefined TDM type\n",
			__func__);
	}
}

/* Disable device interrupts. */
void mv_phone_intr_disable(u8 dev_id)
{
	switch (priv->tdm_type) {
	case MV_TDM_UNIT_TDM2C:
		tdm2c_intr_disable();
		break;
	case MV_TDM_UNIT_TDMMC:
		tdmmc_intr_disable(dev_id);
		break;
	default:
		dev_err(&priv->parent->dev, "%s: undefined TDM type\n",
			__func__);
	}
}

/* Get board type for SLIC unit (pre-defined). */
u32 mv_phone_get_slic_board_type(void)
{
	return MV_BOARD_SLIC_DISABLED;
}

/* Configure PLL to 24MHz */
static int mv_phone_tdm_clk_pll_config(struct platform_device *pdev)
{
	struct resource *mem;
	u32 reg_val;
	u16 freq_offset = 0x22b0;
	u8 tdm_postdiv = 0x6, fb_clk_div = 0x1d;

	if (!priv->pll_base) {
		mem = platform_get_resource_byname(pdev, IORESOURCE_MEM,
						   "pll_regs");
		priv->pll_base = devm_ioremap_resource(&pdev->dev, mem);
		if (IS_ERR(priv->pll_base))
			return -ENOMEM;
	}

	/* Set frequency offset value to not valid and enable PLL reset */
	reg_val = readl(priv->pll_base + TDM_PLL_CONF_REG1);
	reg_val &= ~TDM_PLL_FREQ_OFFSET_VALID;
	reg_val &= ~TDM_PLL_SW_RESET;
	writel(reg_val, priv->pll_base + TDM_PLL_CONF_REG1);

	udelay(1);

	/* Update PLL parameters */
	reg_val = readl(priv->pll_base + TDM_PLL_CONF_REG0);
	reg_val &= ~TDM_PLL_FB_CLK_DIV_MASK;
	reg_val |= (fb_clk_div << TDM_PLL_FB_CLK_DIV_OFFSET);
	writel(reg_val, priv->pll_base + TDM_PLL_CONF_REG0);

	reg_val = readl(priv->pll_base + TDM_PLL_CONF_REG2);
	reg_val &= ~TDM_PLL_POSTDIV_MASK;
	reg_val |= tdm_postdiv;
	writel(reg_val, priv->pll_base + TDM_PLL_CONF_REG2);

	reg_val = readl(priv->pll_base + TDM_PLL_CONF_REG1);
	reg_val &= ~TDM_PLL_FREQ_OFFSET_MASK;
	reg_val |= freq_offset;
	writel(reg_val, priv->pll_base + TDM_PLL_CONF_REG1);

	udelay(1);

	/* Disable reset */
	reg_val |= TDM_PLL_SW_RESET;
	writel(reg_val, priv->pll_base + TDM_PLL_CONF_REG1);

	/* Wait 50us for PLL to lock */
	udelay(50);

	/* Restore frequency offset value validity */
	reg_val |= TDM_PLL_FREQ_OFFSET_VALID;
	writel(reg_val, priv->pll_base + TDM_PLL_CONF_REG1);

	return 0;
}

/* Set DCO post divider in respect of 24MHz PLL output */
static int mv_phone_dco_post_div_config(struct platform_device *pdev,
					u32 pclk_freq_mhz)
{
	struct resource *mem;
	u32 reg_val, pcm_clk_ratio;

	if (!priv->dco_div_reg) {
		mem = platform_get_resource_byname(pdev, IORESOURCE_MEM,
						   "dco_div");
		priv->dco_div_reg = devm_ioremap_resource(&pdev->dev, mem);
		if (IS_ERR(priv->dco_div_reg))
			return -ENOMEM;
	}

	switch (pclk_freq_mhz) {
	case 8:
		pcm_clk_ratio = DCO_CLK_DIV_RATIO_8M;
		break;
	case 4:
		pcm_clk_ratio = DCO_CLK_DIV_RATIO_4M;
		break;
	case 2:
		pcm_clk_ratio = DCO_CLK_DIV_RATIO_2M;
		break;
	default:
		pcm_clk_ratio = DCO_CLK_DIV_RATIO_8M;
		break;
	}

	/* Disable output clock */
	reg_val = readl(priv->dco_div_reg);
	reg_val &= ~DCO_CLK_DIV_RESET_MASK;
	writel(reg_val, priv->dco_div_reg);

	/* Set DCO source ratio */
	reg_val = readl(priv->dco_div_reg);
	writel((reg_val & ~DCO_CLK_DIV_RATIO_MASK) | pcm_clk_ratio,
	       priv->dco_div_reg);

	/* Reload new DCO source ratio */
	reg_val = readl(priv->dco_div_reg);
	reg_val |= DCO_CLK_DIV_APPLY_MASK;
	writel(reg_val, priv->dco_div_reg);
	mdelay(1);

	reg_val = readl(priv->dco_div_reg);
	reg_val &= ~DCO_CLK_DIV_APPLY_MASK;
	writel(reg_val, priv->dco_div_reg);
	mdelay(1);

	/* Enable output clock */
	reg_val = readl(priv->dco_div_reg);
	reg_val |= DCO_CLK_DIV_RESET_MASK;
	writel(reg_val, priv->dco_div_reg);

	return 0;
}

static int mvebu_phone_probe(struct platform_device *pdev)
{
	struct device_node *np = pdev->dev.of_node;
	struct resource *mem;
	int err, i;

	priv = devm_kzalloc(&pdev->dev, sizeof(struct mv_phone_dev),
			    GFP_KERNEL);
	if (!priv)
		return -ENOMEM;

	priv->np = np;

	mem = platform_get_resource_byname(pdev, IORESOURCE_MEM, "tdm_regs");
	priv->tdm_base = devm_ioremap_resource(&pdev->dev, mem);
	if (IS_ERR(priv->tdm_base))
		return PTR_ERR(priv->tdm_base);

	priv->clk = devm_clk_get(&pdev->dev, "gateclk");
	if (PTR_ERR(priv->clk) == -EPROBE_DEFER)
		return -EPROBE_DEFER;

	if (IS_ERR(priv->clk)) {
		dev_err(&pdev->dev, "no clock\n");
		return PTR_ERR(priv->clk);
	}

	err = clk_prepare_enable(priv->clk);
	if (err)
		return err;

	if (of_property_read_u32(np, "pclk-freq-mhz", &priv->pclk_freq_mhz) ||
	    (priv->pclk_freq_mhz != 8 && priv->pclk_freq_mhz != 4 &&
	     priv->pclk_freq_mhz != 2)) {
		priv->pclk_freq_mhz = 8;
		dev_info(&pdev->dev, "wrong pclk frequency in the DT\n");
	}
	dev_info(&pdev->dev, "setting pclk frequency to %d MHz\n",
		 priv->pclk_freq_mhz);

	if (of_device_is_compatible(np, "marvell,armada-380-tdm")) {
		priv->tdm_type = MV_TDM_UNIT_TDM2C;
		err = mv_phone_tdm_clk_pll_config(pdev);
		err |= mv_phone_dco_post_div_config(pdev, priv->pclk_freq_mhz);
		err |= tdm2c_set_mbus_windows(&pdev->dev, priv->tdm_base,
					      mv_mbus_dram_info());
		if (err < 0)
			goto err_clk;

		priv->irq_count = 1;

		tal_set_if(&tdm2c_if);
	}

	if (of_device_is_compatible(priv->np, "marvell,armada-xp-tdm")) {
		priv->tdm_type = MV_TDM_UNIT_TDMMC;
		err = tdmmc_set_mbus_windows(&pdev->dev, priv->tdm_base);
		if (err < 0)
			goto err_clk;

		priv->irq_count = 1;
		priv->tdmmc_ip_ver = TDMMC_REV1;

		tal_set_if(&tdmmc_if);
	}

	if (of_device_is_compatible(priv->np, "marvell,armada-a8k-tdm")) {
		priv->tdm_type = MV_TDM_UNIT_TDMMC;
		tdmmc_set_a8k_windows(&pdev->dev, priv->tdm_base);

		priv->irq_count = 3;
		priv->tdmmc_ip_ver = TDMMC_REV1;

		tal_set_if(&tdmmc_if);
	}

	/* Obtain IRQ numbers */
	for (i = 0; i < priv->irq_count; i++) {
		priv->irq[i] = platform_get_irq(pdev, i);
		if (priv->irq[i] <= 0) {
			dev_err(&pdev->dev, "platform_get_irq %d failed\n", i);
			err = priv->irq[i];
			goto err_clk;
		}
	}

	if (priv->tdm_type == MV_TDM_UNIT_TDM2C) {
		priv->use_pclk_external = of_property_read_bool(np, "use-external-pclk");
		dev_info(&pdev->dev, "using %s pclk\n",
			 priv->use_pclk_external ? "external" : "internal");

		if (of_property_read_u32(np, "spi-mode", &priv->tdm2c_spi_mode) ||
		    (priv->tdm2c_spi_mode != 0 && priv->tdm2c_spi_mode != 1))
			priv->tdm2c_spi_mode = 0;

		dev_info(&pdev->dev, "using %s SPI mode\n",
			 priv->tdm2c_spi_mode ? "daisy-chain" : "direct");

#ifdef CONFIG_MV_TDM_EXT_STATS
		priv->use_tdm_ext_stats = true;
#endif
	}

	spin_lock_init(&priv->lock);

	priv->dev = &pdev->dev;
	return 0;

err_clk:
	clk_disable_unprepare(priv->clk);

	return err;
}

static int mvebu_phone_remove(struct platform_device *pdev)
{
	tal_set_if(NULL);

	clk_disable_unprepare(priv->clk);

	return 0;
}

#ifdef CONFIG_PM_SLEEP
static int mvebu_phone_suspend(struct device *dev)
{
	int i;

	for (i = 0; i < TDM_CTRL_REGS_NUM; i++)
		priv->tdm_ctrl_regs[i] = readl(priv->tdm_base + i);

	for (i = 0; i < TDM_SPI_REGS_NUM; i++)
		priv->tdm_spi_regs[i] = readl(priv->tdm_base +
					      TDM_SPI_REGS_OFFSET + i);

	priv->tdm_spi_mux_reg = readl(priv->tdm_base + TDM_SPI_MUX_REG);
	priv->tdm_mbus_config_reg = readl(priv->tdm_base + TDM_MBUS_CONFIG_REG);
	priv->tdm_misc_reg = readl(priv->tdm_base + TDM_MISC_REG);

	return 0;
}

static int mvebu_phone_resume(struct device *dev)
{
	struct platform_device *pdev = priv->parent;
	int err, i;

	err = tdm2c_set_mbus_windows(dev, priv->tdm_base,
				     mv_mbus_dram_info());
	if (err < 0)
		return err;

	if (of_device_is_compatible(priv->np, "marvell,armada-380-tdm")) {
		err = mv_phone_tdm_clk_pll_config(pdev);
		err |= mv_phone_dco_post_div_config(pdev, priv->pclk_freq_mhz);
		if (err < 0)
			return err;
	}

	for (i = 0; i < TDM_CTRL_REGS_NUM; i++)
		writel(priv->tdm_ctrl_regs[i], priv->tdm_base + i);

	for (i = 0; i < TDM_SPI_REGS_NUM; i++)
		writel(priv->tdm_spi_regs[i], priv->tdm_base +
					      TDM_SPI_REGS_OFFSET + i);

	writel(priv->tdm_spi_mux_reg, priv->tdm_base + TDM_SPI_MUX_REG);
	writel(priv->tdm_mbus_config_reg, priv->tdm_base + TDM_MBUS_CONFIG_REG);
	writel(priv->tdm_misc_reg, priv->tdm_base + TDM_MISC_REG);

	return 0;
}
#endif

#ifdef CONFIG_PM
static const struct dev_pm_ops mvebu_phone_pmops = {
	SET_SYSTEM_SLEEP_PM_OPS(mvebu_phone_suspend, mvebu_phone_resume)
};

#define MVEBU_PHONE_PMOPS (&mvebu_phone_pmops)

#else
#define MVEBU_PHONE_PMOPS NULL
#endif

static const struct of_device_id mvebu_phone_match[] = {
	{ .compatible = "marvell,armada-380-tdm" },
	{ .compatible = "marvell,armada-a8k-tdm" },
	{ .compatible = "marvell,armada-xp-tdm" },
	{ }
};
MODULE_DEVICE_TABLE(of, mvebu_phone_match);

static struct platform_driver mvebu_phone_driver = {
	.probe	= mvebu_phone_probe,
	.remove	= mvebu_phone_remove,
	.driver	= {
		.name	= DRV_NAME,
		.of_match_table = mvebu_phone_match,
		.owner	= THIS_MODULE,
		.pm	= MVEBU_PHONE_PMOPS,
	},
};

module_platform_driver(mvebu_phone_driver);

MODULE_DESCRIPTION("Marvell Telephony Driver");
MODULE_AUTHOR("Marcin Wojtas <mw@semihalf.com>");
