
/*******************************************************************************
Copyright (C) Marvell International Ltd. and its affiliates
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

********************************************************************************/

/* Marvell Telephony Adaptation Layer */
#include "tal.h"

static tal_if_t *tal_if;
static tal_mmp_ops_t *tal_mmp;

tal_stat_t tal_init(tal_params_t *tal_params, tal_mmp_ops_t *mmp_ops)
{
	if (!tal_params || !mmp_ops) {
		mvOsPrintf("%s: Error, bad parameters.\n", __func__);
		return TAL_STAT_BAD_PARAM;
	}

	if (!mmp_ops->tal_mmp_rx_callback || !mmp_ops->tal_mmp_tx_callback) {
		mvOsPrintf("%s: Error, MMP callbacks are missing.\n", __func__);
		return TAL_STAT_BAD_PARAM;
	}

	tal_mmp = mmp_ops;
	if (tal_if && tal_if->init)
		if (tal_if->init(tal_params) != MV_OK)
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

tal_stat_t tal_write(unsigned char *buffer, int size)
{
	if (tal_if && tal_if->write)
		if (tal_if->write(buffer, size) != MV_OK)
			return TAL_STAT_BAD_PARAM;

	return TAL_STAT_OK;
}
EXPORT_SYMBOL(tal_write);

tal_stat_t tal_stats_get(tal_stats_t *tal_stats)
{
	if (tal_stats && tal_if && tal_if->stats_get) {
		tal_if->stats_get(tal_stats);
		return TAL_STAT_OK;
	}

	return TAL_STAT_BAD_PARAM;
}
EXPORT_SYMBOL(tal_stats_get);

tal_stat_t tal_set_if(tal_if_t *interface)
{
	if (interface && (!interface->init || !interface->exit ||
			  !interface->pcm_start || !interface->pcm_stop)) {
		mvOsPrintf("%s: Error, TAL callbacks are missing.\n", __func__);
		return TAL_STAT_BAD_PARAM;
	}

	tal_if = interface;

	return TAL_STAT_OK;
}
EXPORT_SYMBOL(tal_set_if);

tal_stat_t tal_mmp_rx(unsigned char *buffer, int size)
{
	if (tal_mmp && tal_mmp->tal_mmp_rx_callback) {
		tal_mmp->tal_mmp_rx_callback(buffer, size);
		return TAL_STAT_OK;
	}

	return TAL_STAT_BAD_PARAM;
}
EXPORT_SYMBOL(tal_mmp_rx);

tal_stat_t tal_mmp_tx(unsigned char *buffer, int size)
{
	if (tal_mmp && tal_mmp->tal_mmp_tx_callback) {
		tal_mmp->tal_mmp_tx_callback(buffer, size);
		return TAL_STAT_OK;
	}

	return TAL_STAT_BAD_PARAM;
}
EXPORT_SYMBOL(tal_mmp_tx);
