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

******************************************************************************/

#include <linux/interrupt.h>
#include <linux/module.h>
#include <linux/init.h>
#include <plat/ipc_dfev.h>
#include "../tal/tal.h"
#include "ctrlEnv/mvCtrlEnvLib.h"
#include <plat/sdd_if.h>

static struct ipc_dfev_data_handle *ipc_handle;
static int pcm_active;
static int wideband;
static int dfev_init;
static unsigned int rx_miss;
static unsigned int tx_miss;
static unsigned int rx_over;
static unsigned int tx_under;

static void dfev_if_rx_callback(struct ipc_dfev_data_msg *msg)
{
	if (pcm_active) {
		tal_mmp_rx((unsigned char *)msg->samples, sizeof(msg->samples));
		if (ipc_dfev_send_rx(ipc_handle, msg)) {
			pr_err("%s(): Data RX message lost!\n", __func__);
			ipc_dfev_data_msg_put(ipc_handle, msg);
		}
	} else {
		ipc_dfev_data_msg_put(ipc_handle, msg);
	}
}

static void dfev_if_tx_callback(struct ipc_dfev_data_msg *msg)
{
	if (pcm_active) {
		tal_mmp_tx((unsigned char *)msg->samples, sizeof(msg->samples));
		if (ipc_dfev_send_tx(ipc_handle, msg)) {
			pr_err("%s(): Data TX message lost!\n", __func__);
			ipc_dfev_data_msg_put(ipc_handle, msg);
		}
	} else {
		ipc_dfev_data_msg_put(ipc_handle, msg);
	}
}

static struct ipc_dfev_data_ops dfev_if_data_ops = {
	.ipc_dfev_rx_callback		= dfev_if_rx_callback,
	.ipc_dfev_tx_callback		= dfev_if_tx_callback,
};

static MV_STATUS dfev_if_init(tal_params_t *tal_params)
{
	if (tal_params->total_lines != 2)
		return MV_ERROR;


	switch (tal_params->pcm_format) {
	case TAL_PCM_FORMAT_1BYTE:
		wideband = 0;
		break;
	case TAL_PCM_FORMAT_2BYTES:
		wideband = 0;
		break;
	case TAL_PCM_FORMAT_4BYTES:
		wideband = 1;
		break;
	default:
		return MV_ERROR;
	}

	ipc_handle = ipc_dfev_data_init(IPC_DFEV_MODE_INTERRUPT, &dfev_if_data_ops);
	if (ipc_handle == NULL)
		return MV_ERROR;

	dfev_init = 1;
	return MV_OK;
}

static void dfev_if_exit(void)
{
	ipc_dfev_data_exit(ipc_handle);

	dfev_init = 0;
}

static void dfev_if_pcm_start(void)
{
	struct ipc_dfev_data_msg *msg[4];
	int i;

	for (i = 0; i < ARRAY_SIZE(msg); i++) {
		msg[i] = ipc_dfev_data_msg_get(ipc_handle);
		if (!msg[i]) {
			pr_err("%s(): Data message allocation error!\n", __func__);
			break;
		}

		msg[i]->wideband = wideband;
		memset(msg[i]->samples, 0, sizeof(msg[i]->samples));
	}

	pcm_active = 1;

	ipc_dfev_send_rx(ipc_handle, msg[1]);
	ipc_dfev_send_tx(ipc_handle, msg[0]);

	dfev_if_rx_callback(msg[2]);
	dfev_if_tx_callback(msg[3]);
}

static void dfev_if_pcm_stop(void)
{
	pcm_active = 0;
}

void dfev_if_stats_get(tal_stats_t *dfev_if_stats)
{
	if (dfev_init == 0)
		return;

	dfev_if_stats->tdm_init = dfev_init;
	dfev_if_stats->rx_miss = rx_miss;
	dfev_if_stats->tx_miss = tx_miss;
	dfev_if_stats->rx_over = rx_over;
	dfev_if_stats->tx_under = tx_under;
#ifdef CONFIG_MV_TDM_EXT_STATS
	memset(&dfev_if_stats->tdm_ext_stats, 0, sizeof(MV_TDM_EXTENDED_STATS));
#endif
	return;
}

static tal_if_t dfev_if = {
	.init		= dfev_if_init,
	.exit		= dfev_if_exit,
	.pcm_start	= dfev_if_pcm_start,
	.pcm_stop	= dfev_if_pcm_stop,
	.stats_get	= dfev_if_stats_get,
};

static int __init dfev_if_module_init(void)
{
	if (MV_TDM_UNIT_DFEV == mvCtrlTdmUnitTypeGet())
		tal_set_if(&dfev_if);
	return 0;
}

static void __exit dfev_if_module_exit(void)
{
	if (MV_TDM_UNIT_DFEV == mvCtrlTdmUnitTypeGet())
		tal_set_if(NULL);
	return;
}

/* Module stuff */
module_init(dfev_if_module_init);
module_exit(dfev_if_module_exit);
MODULE_DESCRIPTION("Marvell DFEV I/F Device Driver - www.marvell.com");
MODULE_AUTHOR("Piotr Ziecik <kosmo@semihalf.com>");
MODULE_LICENSE("GPL");
