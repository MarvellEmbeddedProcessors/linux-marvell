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

/* Marvell Telephony Adaptation Layer */
#include "tal.h"

static struct tal_if *tal_if;
static struct tal_mmp_ops *tal_mmp;

enum tal_status tal_init(struct tal_params *tal_params,
			 struct tal_mmp_ops *mmp_ops)
{
	if (!tal_params || !mmp_ops) {
		pr_err("%s: Error, bad parameters.\n", __func__);
		return TAL_STAT_BAD_PARAM;
	}

	if (!mmp_ops->tal_mmp_rx_callback || !mmp_ops->tal_mmp_tx_callback) {
		pr_err("%s: Error, MMP callbacks are missing.\n", __func__);
		return TAL_STAT_BAD_PARAM;
	}

	tal_mmp = mmp_ops;
	if (tal_if && tal_if->init)
		if (tal_if->init(tal_params) != 0)
			return TAL_STAT_INIT_ERROR;

	return TAL_STAT_OK;
}
EXPORT_SYMBOL(tal_init);

void tal_exit(void)
{
	if (tal_if && tal_if->exit)
		tal_if->exit();

	tal_mmp = NULL;
}
EXPORT_SYMBOL(tal_exit);

void tal_pcm_start(void)
{
	if (tal_if && tal_if->pcm_start)
		tal_if->pcm_start();
}
EXPORT_SYMBOL(tal_pcm_start);

void tal_pcm_stop(void)
{
	if (tal_if && tal_if->pcm_stop)
		tal_if->pcm_stop();
}
EXPORT_SYMBOL(tal_pcm_stop);

int tal_control(int cmd, void *data)
{
	if (tal_if && tal_if->control)
		return tal_if->control(cmd, data);

	return -EINVAL;
}
EXPORT_SYMBOL(tal_control);

enum tal_status tal_write(u8 *buffer, int size)
{
	if (tal_if && tal_if->write)
		if (tal_if->write(buffer, size) != 0)
			return TAL_STAT_BAD_PARAM;

	return TAL_STAT_OK;
}
EXPORT_SYMBOL(tal_write);

enum tal_status tal_stats_get(struct tal_stats *tal_stats)
{
	if (tal_stats && tal_if && tal_if->stats_get) {
		tal_if->stats_get(tal_stats);
		return TAL_STAT_OK;
	}

	return TAL_STAT_BAD_PARAM;
}
EXPORT_SYMBOL(tal_stats_get);

enum tal_status tal_set_if(struct tal_if *interface)
{
	if (interface && (!interface->init || !interface->exit ||
			  !interface->pcm_start || !interface->pcm_stop)) {
		pr_err("%s: Error, TAL callbacks are missing.\n", __func__);
		return TAL_STAT_BAD_PARAM;
	}

	tal_if = interface;

	return TAL_STAT_OK;
}
EXPORT_SYMBOL(tal_set_if);

enum tal_status tal_mmp_rx(u8 *buffer, int size)
{
	if (tal_mmp && tal_mmp->tal_mmp_rx_callback) {
		tal_mmp->tal_mmp_rx_callback(buffer, size);
		return TAL_STAT_OK;
	}

	return TAL_STAT_BAD_PARAM;
}
EXPORT_SYMBOL(tal_mmp_rx);

enum tal_status tal_mmp_tx(u8 *buffer, int size)
{
	if (tal_mmp && tal_mmp->tal_mmp_tx_callback) {
		tal_mmp->tal_mmp_tx_callback(buffer, size);
		return TAL_STAT_OK;
	}

	return TAL_STAT_BAD_PARAM;
}
EXPORT_SYMBOL(tal_mmp_tx);
