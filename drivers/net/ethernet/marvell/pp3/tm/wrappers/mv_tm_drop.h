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
*******************************************************************************/

#ifndef MV_TM_DROP__H
#define MV_TM_DROP__H

#include "common/mv_sw_if.h"
#include "tm/mv_tm.h"
#include "tm_core_types.h"

#define MV_TM_NUM_OF_COLORS				3
#define MV_TM_WRED_COS					TM_WRED_COS

#define MV_TM_NUM_QUEUE_DROP_PROF		TM_NUM_QUEUE_DROP_PROF
#define MV_TM_NUM_A_NODE_DROP_PROF		TM_NUM_A_NODE_DROP_PROF
#define MV_TM_NUM_B_NODE_DROP_PROF		TM_NUM_B_NODE_DROP_PROF
#define MV_TM_NUM_C_NODE_DROP_PROF		TM_NUM_C_NODE_DROP_PROF
#define MV_TM_NUM_PORT_DROP_PROF		TM_NUM_PORT_DROP_PROF

/* NSS Drop profile */
struct mv_tm_drop_profile {
	/* CBTD */
	uint32_t cbtd_threshold;                     /* Color Blind Tail Drop Threshold in resolution 16B */

	/* CATD/WRED */
	uint8_t  color_td_en;                        /* Colored Tail Drop Enable: 0 - WRED, 1 - CATD */
	uint32_t min_threshold[MV_TM_NUM_OF_COLORS]; /* RED curve Min threshold per color [0..2] */
	uint32_t max_threshold[MV_TM_NUM_OF_COLORS]; /* RED curve Max threshold per color [0..2] */
	uint8_t  curve_id[MV_TM_NUM_OF_COLORS];      /* RED curve index per color[0..2] */
	uint8_t  curve_scale[MV_TM_NUM_OF_COLORS];   /* Used for scaling of DP [0..2] */
};

/* create traditional WRED curve, where mp - max probability [1-100%] */
int mv_tm_create_wred_curve(enum mv_tm_level level, int cos, uint8_t mp, uint8_t *curve_index);

/* create flat WRED curve, where mp - max probability [1-100%] */
int mv_tm_create_flat_wred_curve(enum mv_tm_level level, int cos, uint8_t cp, uint8_t *curve_index);

/* update Drop Profile params to HW */
int mv_tm_drop_profile_set(enum mv_tm_level level, uint32_t profile_index, int cos,
							struct mv_tm_drop_profile *profile);

/* get Drop Profile */
int mv_tm_drop_profile_get(enum mv_tm_level level, uint32_t profile_index, int cos,
							struct mv_tm_drop_profile *profile);

/* set Drop Profile to default */
int mv_tm_drop_profile_clear(enum mv_tm_level level, uint32_t profile_index, int cos);

/* update Drop Profile (CBTD & WRED), parameters in Kbps */
int mv_tm_update_drop_profile_wred(enum mv_tm_level level, int cos, uint32_t profile_index,
									uint32_t cb_bw, uint32_t wred_bw);

/* update Drop Profile (CBTD & CATD), parameters in Kbps */
int mv_tm_update_drop_profile_catd(enum mv_tm_level level, int cos, uint32_t profile_index,
									uint32_t cb_bw, uint32_t ca_bw);

/* set number of colors in system (by default there are 2 colors in the system for better resolution) */
int mv_tm_color_num_set(enum mv_tm_level level, int color);

/* queue cos select */
int mv_tm_queue_cos_set(uint32_t node_index, int cos);

/* update Queue/Node/Port pointer to Drop Profile */
int mv_tm_dp_set(enum mv_tm_level level, uint32_t node_index, int cos, uint32_t drop_profile);

/* update Queue/Node/Port queue length */
int mv_tm_queue_length_get(enum mv_tm_level level, uint32_t index, uint32_t *av_queue_length);

#endif /* MV_TM_DROP__H */
