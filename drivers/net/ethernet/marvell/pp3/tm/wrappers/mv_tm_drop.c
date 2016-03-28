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

#include "mv_tm_drop.h"
#include "tm_core_types.h"
#include "tm_drop.h"
#include "rm_status.h"
#include "tm_hw_configuration_interface.h"
#include "tm_nodes_update.h"
#include "tm_nodes_status.h"

static int tm_round_int(uint32_t val, uint32_t divider)
{
	return (val * 100 + 50) / (divider * 100);
}

static int check_curve(rmctl_t hndl, enum mv_tm_level level, uint8_t index, int cos, uint8_t *status)
{
	int rc = 0;

	switch (level) {
	case TM_Q_LEVEL:
		if (index >= TM_NUM_WRED_QUEUE_CURVES)
			return -1;
		rc = rm_wred_queue_curve_status(hndl, index, status);
		break;
	case TM_A_LEVEL:
		if (index >= TM_NUM_WRED_A_NODE_CURVES)
			return -1;
		rc = rm_wred_a_node_curve_status(hndl, index, status);
		break;
	case TM_B_LEVEL:
		if (index >= TM_NUM_WRED_B_NODE_CURVES)
			return -1;
		rc = rm_wred_b_node_curve_status(hndl, index, status);
		break;
	case TM_C_LEVEL:
		if (cos > TM_WRED_COS)
			return -2;
		if (index >= TM_NUM_WRED_C_NODE_CURVES)
			return -1;
		rc = rm_wred_c_node_curve_status(hndl, cos, index, status);
		break;
	case TM_P_LEVEL:
		if (index >= TM_NUM_WRED_PORT_CURVES)
			return -1;
		if (cos == -1) /* Global Port */
			rc = rm_wred_port_curve_status(hndl, index, status);
		else
			rc = rm_wred_port_curve_status_cos(hndl, cos, index, status);
		break;
	default:
		return -3;
	}
	return rc;
}

/* Public functions to be called from external modules */
int mv_tm_create_wred_curve(enum mv_tm_level level, int cos, uint8_t mp, uint8_t *index)
{
	TM_WRAPPER_BEGIN(qmtm_hndl, ctl, henv);

	if (cos == -1)
		rc = tm_create_wred_traditional_curve(ctl, TM_LEVEL(level), (uint8_t) TM_INVAL, mp, index);
	else
		rc = tm_create_wred_traditional_curve(ctl, TM_LEVEL(level), cos, mp, index);
	if (rc != 0)
		pr_info("tm_create_wred_traditional_curve error: %d\n", rc);
	else
		pr_info("tm_create_wred_traditional_curve level=%d, cos=%d, mp=%d, index=%d\n",
			level, cos, mp, (int)(*index));

	TM_WRAPPER_END(qmtm_hndl);
}
EXPORT_SYMBOL(mv_tm_create_wred_curve);

int mv_tm_create_flat_wred_curve(enum mv_tm_level level, int cos, uint8_t cp, uint8_t *curve_index)
{
	TM_WRAPPER_BEGIN(qmtm_hndl, ctl, henv);

	if (cos == -1)
		rc = tm_create_wred_flat_curve(ctl, TM_LEVEL(level), (uint8_t) TM_INVAL, cp, curve_index);
	else
		rc = tm_create_wred_flat_curve(ctl, TM_LEVEL(level), cos, cp, curve_index);
	if (rc != 0)
		pr_info("tm_create_wred_flat_curve error: %d\n", rc);
	else
		pr_info("tm_create_wred_flat_curve level=%d, cos=%d, cp=%d, curve index=%d\n",
			level, cos, cp, (int)(*curve_index));

	TM_WRAPPER_END(qmtm_hndl);
}
EXPORT_SYMBOL(mv_tm_create_flat_wred_curve);



int mv_tm_drop_profile_set(enum mv_tm_level level, uint32_t index, int cos, struct mv_tm_drop_profile *profile)
{
	struct tm_drop_profile *dp;
	int i;
	uint8_t exp;
	uint8_t status;
	uint16_t ratio;
	uint32_t max_thresh;
	uint32_t threshold;
	uint32_t min;
	uint32_t max;
	uint32_t thresh_scaled = 0;
	uint32_t min_thresh_scaled = 0;
	uint32_t max_thresh_scaled = 0;

	TM_WRAPPER_BEGIN(qmtm_hndl, ctl, henv);

	switch (level) {
	case TM_Q_LEVEL:
		if (index >= MV_TM_NUM_QUEUE_DROP_PROF) {
			rc = -1;
			goto out;
		}
		dp = &(ctl->tm_q_lvl_drop_profiles[index]);
		break;
	case TM_A_LEVEL:
		if (index >= MV_TM_NUM_A_NODE_DROP_PROF) {
			rc = -1;
			goto out;
		}
		dp = &(ctl->tm_a_lvl_drop_profiles[index]);
		break;
	case TM_B_LEVEL:
		if (index >= MV_TM_NUM_B_NODE_DROP_PROF) {
			rc = -1;
			goto out;
		}
		dp = &(ctl->tm_b_lvl_drop_profiles[index]);
		break;
	case TM_C_LEVEL:
		if ((cos >= MV_TM_WRED_COS) || (cos <= 0) || (index >= MV_TM_NUM_C_NODE_DROP_PROF)) {
			rc = -1;
			goto out;
		}
		dp = &(ctl->tm_c_lvl_drop_profiles[cos][index]);
		break;
	case TM_P_LEVEL:
		if (index >= MV_TM_NUM_PORT_DROP_PROF) {
			rc = -1;
			goto out;
		}
		if (cos == -1)
			dp = &(ctl->tm_p_lvl_drop_profiles[index]);
		else {
			if ((cos >= MV_TM_WRED_COS) || (cos <= 0)) {
				rc = -1;
				goto out;
			}
			dp = &(ctl->tm_p_lvl_drop_profiles_cos[cos][index]);
		}
		break;
	default:
		rc = -1;
		goto out;
	}

	/* CBTD */
	threshold = profile->cbtd_threshold;
	max_thresh = get_drop_threshold_definition() * TM_1K;
	if (threshold > max_thresh) {
		rc = -2;
		goto out;
	}

	if (threshold > get_drop_threshold_definition()) {
		dp->td_thresh_res = TM_ENABLE;
		dp->td_threshold = threshold/TM_1K;
	} else {
		dp->td_thresh_res = TM_DISABLE;
		dp->td_threshold = threshold;
	}

	/* CATD/WRED */
	dp->color_td_en = profile->color_td_en;
	max_thresh = 1023 * (uint32_t)(1 << 22);
	for (i = 0; i < MV_TM_NUM_OF_COLORS; i++) {
		if (profile->color_td_en == TM_ENABLE) { /* CATD mode */
			threshold = profile->min_threshold[i];
			if (threshold > max_thresh) {
				rc = -2;
				goto out;
			}

			for (exp = 0; exp < 22; exp++) {
				thresh_scaled = threshold/(uint32_t)(1<<exp);
				if (thresh_scaled < TM_1K)
					break;
			}
			dp->min_threshold[i].thresh = (uint16_t)thresh_scaled;
			dp->scale_exp[i].exp = exp;
			dp->scale_ratio[i].ratio = 0;
			dp->curve_id[i].index = 0;
			dp->dp_ratio[i].ratio = 0;
			dp->min_th_sw[i] = threshold;
			dp->max_th_sw[i] = 0;
		} else { /* WRED mode */
			min = profile->min_threshold[i];
			max = profile->max_threshold[i];
			dp->min_th_sw[i] = min;
			dp->max_th_sw[i] = max;
			if ((min == 0) && (max == 0)) {
				dp->curve_id[i].index = 0;
				dp->dp_ratio[i].ratio = 0;
				dp->scale_exp[i].exp = 22;
				dp->scale_ratio[i].ratio = 0;
				dp->min_threshold[i].thresh = 1023;
			} else {
				if ((max > max_thresh) || (min > max)) {
					rc = -2;
					goto out;
				}

				/* check that curve exists */
				rc = check_curve(ctl->rm, level, profile->curve_id[i], cos, &status);
				if ((rc < 0) || (status != RM_TRUE)) {
					rc = -3;
					goto out;
				}
				dp->curve_id[i].index = profile->curve_id[i];
				dp->dp_ratio[i].ratio = profile->curve_scale[i];

				for (exp = 0; exp < 22; exp++) {
					max_thresh_scaled = max/(uint32_t)(1 << exp);
					if (max_thresh_scaled < TM_1K)
						break;
				}
				min_thresh_scaled = min/(uint32_t)(1 << exp);
				/* 1024 * 32 = 0x8000 */
				ratio = (uint16_t)tm_round_int(0x8000, (max_thresh_scaled - min_thresh_scaled + 1));

				dp->scale_exp[i].exp = exp;
				if (ratio > 1023)
					dp->scale_ratio[i].ratio = 1023;
				else
					dp->scale_ratio[i].ratio = ratio;
				dp->min_threshold[i].thresh = (uint16_t)min_thresh_scaled;
			}
		}
	}

	dp->out_bw = TM_INVAL;
	dp->cbtd_bw = TM_INVAL;
	dp->aql_exp = 0; /* Forget Factor = 1, AQL=QL */;

	if (cos == -1)
		rc = tm_drop_profile_hw_set(ctl, TM_LEVEL(level), (uint8_t) TM_INVAL, index);
	else
		rc = tm_drop_profile_hw_set(ctl, TM_LEVEL(level), cos, index);
out:
	if (rc)
		pr_info("mv_tm_drop_profile_set error: %d\n", rc);
	TM_WRAPPER_END(qmtm_hndl);
}
EXPORT_SYMBOL(mv_tm_drop_profile_set);

int mv_tm_drop_profile_get(enum mv_tm_level level, uint32_t index, int cos, struct mv_tm_drop_profile *profile)
{
	struct tm_drop_profile *dp;
	int i;

	TM_WRAPPER_BEGIN(qmtm_hndl, ctl, henv);

	switch (level) {
	case Q_LEVEL:
		dp = &(ctl->tm_q_lvl_drop_profiles[index]);
		break;
	case A_LEVEL:
		dp = &(ctl->tm_a_lvl_drop_profiles[index]);
		break;
	case B_LEVEL:
		dp = &(ctl->tm_b_lvl_drop_profiles[index]);
		break;
	case C_LEVEL:
		dp = &(ctl->tm_c_lvl_drop_profiles[cos][index]);
		break;
	case P_LEVEL:
		if (cos == -1)
			dp = &(ctl->tm_p_lvl_drop_profiles[index]);
		else
			dp = &(ctl->tm_p_lvl_drop_profiles_cos[cos][index]);
		break;
	default:
		rc = -1;
		pr_info("mv_tm_drop_profile_get error: %d\n", rc);
		TM_WRAPPER_END(qmtm_hndl);
	}

	if (dp->td_thresh_res)
		profile->cbtd_threshold = dp->td_threshold * TM_1K;
	else
		profile->cbtd_threshold = dp->td_threshold;

	profile->color_td_en = dp->color_td_en;

	for (i = 0; i < MV_TM_NUM_OF_COLORS; i++) {
		profile->min_threshold[i] = dp->min_th_sw[i];
#if 0 /* TBD: remove */
		if ((dp->scale_ratio[i].ratio == 0) && (dp->color_td_en == TM_DISABLE))
			profile->min_threshold[i] = 0;
		else
			profile->min_threshold[i] = (dp->min_threshold[i].thresh) * (1 << (dp->scale_exp[i].exp));
#endif

		if (dp->color_td_en == TM_DISABLE) {
			profile->max_threshold[i] = dp->max_th_sw[i];
			profile->curve_id[i] = dp->curve_id[i].index;
			profile->curve_scale[i] = dp->dp_ratio[i].ratio;
#if 0 /* TBD: remove */
		for (i = 0; i < 3; i++)
			if (dp->scale_ratio[i].ratio == 0)
				max_th[i] = 0;
			else {
				/* 1024 * 32 = 0x8000 */
				max_th[i] = (uint16_t)tm_round_int(0x8000,
					(dp->scale_ratio[i].ratio)) + dp->min_threshold[i].thresh - 1;
				max_th[i] = max_th[i] * (1 << (dp->scale_exp[i].exp));
			}
#endif
		} else {
			profile->max_threshold[i] = 0;
			profile->curve_id[i] = 0;
			profile->curve_scale[i] = 0;
		}
	}

	TM_WRAPPER_END(qmtm_hndl);
}
EXPORT_SYMBOL(mv_tm_drop_profile_get);

int mv_tm_drop_profile_clear(enum mv_tm_level level, uint32_t index, int cos)
{
	struct tm_drop_profile *dp;
	int i;

	TM_WRAPPER_BEGIN(qmtm_hndl, ctl, henv);

	switch (level) {
	case Q_LEVEL:
		dp = &(ctl->tm_q_lvl_drop_profiles[index]);
		break;
	case A_LEVEL:
		dp = &(ctl->tm_a_lvl_drop_profiles[index]);
		break;
	case B_LEVEL:
		dp = &(ctl->tm_b_lvl_drop_profiles[index]);
		break;
	case C_LEVEL:
		dp = &(ctl->tm_c_lvl_drop_profiles[cos][index]);
		break;
	case P_LEVEL:
		if (cos == -1)
			dp = &(ctl->tm_p_lvl_drop_profiles[index]);
		else
			dp = &(ctl->tm_p_lvl_drop_profiles_cos[cos][index]);
		break;
	default:
		rc = -1;
		pr_info("mv_tm_drop_profile_clear error: %d\n", rc);
		TM_WRAPPER_END(qmtm_hndl);
	}

	dp->out_bw = 0;
	dp->cbtd_bw = 0;
	dp->aql_exp = 0; /* Forget Factor = 1, AQL=QL */;
	dp->td_thresh_res = TM_ENABLE; /* 16KB */
	dp->td_threshold = get_drop_threshold_definition(); /* Max */
	dp->color_td_en = TM_DISABLE;
	for (i = 0; i < MV_TM_NUM_OF_COLORS; i++) {
		dp->scale_exp[i].exp = 22; /* TBD: default in Cider is incorrect: 1 Burst unit */
		dp->scale_ratio[i].ratio = 0;
		dp->curve_id[i].index = 0;
		dp->dp_ratio[i].ratio = 0;
		dp->min_threshold[i].thresh = 0x3FF; /* TBD: default in Cider is incorrect: 0 Bursts */
		dp->min_th_sw[i] = 0x3FF;
		dp->max_th_sw[i] = dp->min_th_sw[i] * (uint32_t)(1 << dp->scale_exp[i].exp);
	}

	if (cos == -1)
		rc = tm_drop_profile_hw_set(ctl, TM_LEVEL(level), (uint8_t) TM_INVAL, index);
	else
		rc = tm_drop_profile_hw_set(ctl, TM_LEVEL(level), cos, index);

	TM_WRAPPER_END(qmtm_hndl);
}
EXPORT_SYMBOL(mv_tm_drop_profile_clear);

int mv_tm_update_drop_profile_wred(enum mv_tm_level level, int cos, uint32_t prof_index,
								uint32_t cb_bw, uint32_t wred_bw)
{
	struct tm_drop_profile_params profile;
	int i;

	TM_WRAPPER_BEGIN(qmtm_hndl, ctl, henv);

	profile.wred_catd_bw = wred_bw;
	profile.cbtd_bw = cb_bw;

	profile.cbtd_rtt_ratio = 100;
	profile.aql_exp = 0;
	profile.wred_catd_mode = WRED;
	for (i = 0; i < MV_TM_NUM_OF_COLORS; i++) {
		profile.curve_id[i] = 0;
		profile.dp_ratio[i] = 0;
		profile.min_th[i] = 0;
		profile.max_th[i] = 100;
	}

	if (cos == -1)
		rc = tm_update_drop_profile(ctl, TM_LEVEL(level), (uint8_t) TM_INVAL, prof_index, &profile);
	else
		rc = tm_update_drop_profile(ctl, TM_LEVEL(level), cos, prof_index, &profile);
	if (rc == -ENODEV) {
		rc = tm_create_wred_traditional_curve(ctl, TM_LEVEL(level), cos, 50, &profile.curve_id[0]);
		if (rc)
			return -ENOSPC;
		profile.curve_id[1] = profile.curve_id[0];
		profile.curve_id[2] = profile.curve_id[0];

		/* Try again */
		if (cos == -1)
			rc = tm_update_drop_profile(ctl, TM_LEVEL(level), (uint8_t) TM_INVAL, prof_index, &profile);
		else
			rc = tm_update_drop_profile(ctl, TM_LEVEL(level), cos, prof_index, &profile);
	}
	if (rc != 0)
		pr_info("tm_update_drop_profile_wred error: %d\n", rc);

	TM_WRAPPER_END(qmtm_hndl);
}
EXPORT_SYMBOL(mv_tm_update_drop_profile_wred);

int mv_tm_update_drop_profile_catd(enum mv_tm_level level, int cos, uint32_t prof_index,
								uint32_t cb_bw, uint32_t ca_bw)
{
	struct tm_drop_profile_params profile;
	int i;

	TM_WRAPPER_BEGIN(qmtm_hndl, ctl, henv);

	profile.wred_catd_bw = ca_bw;
	profile.cbtd_bw = cb_bw;

	profile.cbtd_rtt_ratio = 100;
	profile.aql_exp = 0;
	profile.wred_catd_mode = CATD;
	for (i = 0; i < MV_TM_NUM_OF_COLORS; i++) {
		profile.curve_id[i] = 0;
		profile.dp_ratio[i] = 0;
		profile.min_th[i] = 100;
		profile.max_th[i] = 100;
	}

	if (cos == -1)
		rc = tm_update_drop_profile(ctl, TM_LEVEL(level), (uint8_t) TM_INVAL, prof_index, &profile);
	else
		rc = tm_update_drop_profile(ctl, TM_LEVEL(level), cos, prof_index, &profile);
	if (rc == -ENODEV) {
		rc = tm_create_wred_traditional_curve(ctl, TM_LEVEL(level), cos, 50, &profile.curve_id[0]);
		if (rc)
			return -ENOSPC;
		profile.curve_id[1] = profile.curve_id[0];
		profile.curve_id[2] = profile.curve_id[0];

		/* Try again */
		if (cos == -1)
			rc = tm_update_drop_profile(ctl, TM_LEVEL(level), (uint8_t) TM_INVAL, prof_index, &profile);
		else
			rc = tm_update_drop_profile(ctl, TM_LEVEL(level), cos, prof_index, &profile);
	}
	if (rc != 0)
		pr_info("tm_update_drop_profile_catd error: %d\n", rc);

	TM_WRAPPER_END(qmtm_hndl);
}
EXPORT_SYMBOL(mv_tm_update_drop_profile_catd);

int mv_tm_color_num_set(enum mv_tm_level level, int color)
{
	TM_WRAPPER_BEGIN(qmtm_hndl, ctl, henv);

	if ((color < 0) || (color > 3)) {
		rc = -1;
		goto out;
	}

	rc = tm_set_drop_color_num(ctl, TM_LEVEL(level), color - 1);

out:
	if (rc != 0)
		pr_info("mv_tm_color_num_set error: %d\n", rc);
	TM_WRAPPER_END(qmtm_hndl);
}
EXPORT_SYMBOL(mv_tm_color_num_set);

int mv_tm_queue_cos_set(uint32_t index, int cos)
{
	TM_WRAPPER_BEGIN(qmtm_hndl, ctl, henv);

	if (cos < 0) {
		rc = -1;
		goto out;
	}

	rc = tm_set_drop_queue_cos(ctl, index, cos);

out:
	if (rc != 0)
		pr_info("mv_tm_queue_cos_set error: %d\n", rc);
	TM_WRAPPER_END(qmtm_hndl);
}
EXPORT_SYMBOL(mv_tm_queue_cos_set);

int mv_tm_dp_set(enum mv_tm_level level, uint32_t index, int cos, uint32_t drop_profile)
{
	int i;
	struct tm_queue_params q_params;
	struct tm_a_node_params a_params;
	struct tm_b_node_params b_params;
	struct tm_c_node_params c_params;
	struct tm_port_drop_per_cos params;

	TM_WRAPPER_BEGIN(qmtm_hndl, ctl, henv);

	switch (level) {
	case TM_Q_LEVEL:
		q_params.wred_profile_ref = (uint8_t) drop_profile;
		q_params.elig_prio_func_ptr = (uint8_t) TM_INVAL;
		q_params.quantum = (uint16_t) TM_INVAL;
		rc = tm_update_queue(ctl, index, &q_params);
		break;
	case TM_A_LEVEL:
		a_params.wred_profile_ref = (uint8_t) drop_profile;
		a_params.elig_prio_func_ptr = (uint8_t) TM_INVAL;
		a_params.quantum = (uint16_t) TM_INVAL;
		for (i = 0; i < 8; i++)
			a_params.dwrr_priority[i] = (uint8_t) TM_INVAL;
		rc = tm_update_a_node(ctl, index, &a_params);
		break;
	case TM_B_LEVEL:
		b_params.wred_profile_ref = (uint8_t) drop_profile;
		b_params.elig_prio_func_ptr = (uint8_t) TM_INVAL;
		b_params.quantum = (uint16_t) TM_INVAL;
		for (i = 0; i < 8; i++)
			b_params.dwrr_priority[i] = (uint8_t) TM_INVAL;
		rc = tm_update_b_node(ctl, index, &b_params);
		break;
	case TM_C_LEVEL:
		c_params.wred_cos = 0xff;
		for (i = 0; i < TM_WRED_COS; i++)
			c_params.wred_profile_ref[i] = (uint8_t) TM_INVAL;
		c_params.wred_profile_ref[cos] = (uint8_t) drop_profile;
		c_params.elig_prio_func_ptr = (uint8_t) TM_INVAL;
		c_params.quantum = (uint16_t) TM_INVAL;
		for (i = 0; i < 8; i++)
			c_params.dwrr_priority[i] = (uint8_t) TM_INVAL;
		rc = tm_update_c_node(ctl, index, &c_params);
		break;
	case TM_P_LEVEL:
		if (cos == -1)
			rc = tm_update_port_drop(ctl, index, (uint8_t) drop_profile);
		else {
			params.wred_cos = 0xff;
			for (i = 0; i < TM_WRED_COS; i++)
				params.wred_profile_ref[i] = (uint8_t)TM_INVAL;
			params.wred_profile_ref[cos] = (uint8_t) drop_profile;
			rc = tm_update_port_drop_cos(ctl, index, &params);
		}
		break;
	default:
		rc = -3;
		break;
	}
	if (rc != 0)
		pr_info("mv_tm_dp_set error: %d\n", rc);

	TM_WRAPPER_END(qmtm_hndl);
}
EXPORT_SYMBOL(mv_tm_dp_set);

int mv_tm_queue_length_get(enum mv_tm_level level, uint32_t index, uint32_t *av_queue_length)
{
	TM_WRAPPER_BEGIN(qmtm_hndl, ctl, henv);

	rc = tm_drop_get_queue_length(ctl, TM_LEVEL(level), index, av_queue_length);
	if (rc)
		pr_info("mv_tm_queue_length_get error: %d\n", rc);

	TM_WRAPPER_END(qmtm_hndl);
}
EXPORT_SYMBOL(mv_tm_queue_length_get);
