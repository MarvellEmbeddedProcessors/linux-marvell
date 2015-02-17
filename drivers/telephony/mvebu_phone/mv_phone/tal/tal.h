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

#ifndef _TAL_H_
#define _TAL_H_

#include "mvOs.h"

/* Defines */
#define TAL_MAX_PHONE_LINES	32

/* Enumerators */
typedef enum {
	TAL_PCM_FORMAT_1BYTE = 1,
	TAL_PCM_FORMAT_2BYTES = 2,
	TAL_PCM_FORMAT_4BYTES = 4,
} tal_pcm_format_t;

typedef enum {
	TAL_STAT_OK = 0,
	TAL_STAT_BAD_PARAM,
	TAL_STAT_INIT_ERROR,
} tal_stat_t;

/* Structures */
typedef struct {
	tal_pcm_format_t pcm_format;
	unsigned short pcm_slot[TAL_MAX_PHONE_LINES];
	unsigned char sampling_period;
	unsigned short total_lines;
} tal_params_t;

typedef struct {
	int tdm_init;
	unsigned int rx_miss;
	unsigned int tx_miss;
	unsigned int rx_over;
	unsigned int tx_under;
#ifdef MV_TDM_EXT_STATS
	MV_TDM_EXTENDED_STATS tdm_ext_stats;
#endif
} tal_stats_t;

typedef struct {
	void (*tal_mmp_rx_callback)(unsigned char *rx_buff, int size);
	void (*tal_mmp_tx_callback)(unsigned char *tx_buff, int size);
} tal_mmp_ops_t;

typedef struct {
	MV_STATUS (*init)(tal_params_t *tal_params);
	void (*exit)(void);
	void (*pcm_start)(void);
	void (*pcm_stop)(void);
	int (*control)(int cmd, void *data);
	MV_STATUS (*write)(unsigned char *buffer, int size);
	void (*stats_get)(tal_stats_t *tal_stats);
} tal_if_t;

/* API */
tal_stat_t tal_init(tal_params_t *tal_params, tal_mmp_ops_t *mmp_ops);
tal_stat_t tal_stats_get(tal_stats_t *tal_stats);
void tal_exit(void);
void tal_pcm_start(void);
void tal_pcm_stop(void);
int tal_control(int cmd, void *data);

tal_stat_t tal_set_if(tal_if_t *interface);
tal_stat_t tal_mmp_rx(unsigned char *buffer, int size);
tal_stat_t tal_mmp_tx(unsigned char *buffer, int size);
tal_stat_t tal_write(unsigned char *buffer, int size);

#endif /* _TAL_H */
