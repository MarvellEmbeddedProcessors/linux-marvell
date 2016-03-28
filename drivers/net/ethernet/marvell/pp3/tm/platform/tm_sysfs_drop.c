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

#include "tm_sysfs_drop.h"
#include "mv_tm_drop.h"
#include "tm/mv_tm.h"
#include "tm_drop.h"
#include "tm_hw_configuration_interface.h"
#include "rm_list.h"
#include "rm_status.h"

#define TM_ALL_COLORS			7

static struct mv_tm_drop_profile context;
static uint8_t flag = TM_DISABLE;
static char *level_names_arr[] = { "Q", "A", "B", "C", "P"};

static void set_dp_default(void)
{
	int i;

	context.color_td_en = TM_DISABLE;
	context.cbtd_threshold = get_drop_threshold_definition() * TM_1K; /* Max */

	for (i = 0; i < MV_TM_NUM_OF_COLORS; i++) {
		context.curve_id[i] = 0;
		context.curve_scale[i] = 0;
		context.min_threshold[i] = 0x0; /* 0 Bursts */
		context.max_threshold[i] = 0x0; /* 0 Bursts */
	}
}

int tm_sysfs_read_drop_profiles(void)
{
	uint8_t cos;
	uint16_t prof_index;
	struct tm_drop_profile_params dp_profile;
	int level;

	TM_WRAPPER_BEGIN(qmtm_hndl, ctl, henv);

	for (level = P_LEVEL; level >= Q_LEVEL; level--) {
		if ((level == P_LEVEL) || (level == C_LEVEL)) {
			pr_info("TM %s Level Drop Profiles Indexes (per Cos):\n", level_names_arr[level]);
			for (cos = 0; cos < TM_WRED_COS; cos++) {
				rc = 0;
				for (prof_index = 1; rc == 0; prof_index++) {
					rc = tm_read_drop_profile(ctl,
						TM_LEVEL(level),
						cos,
						prof_index,
						&dp_profile);
					if (rc)
						break;
				}
				if (prof_index - 1)
					pr_info("   Cos %d: %d-%d\n", cos, 1, prof_index - 1);
				else
					pr_info("   Cos %d: None\n", cos);
			}
		}
		if (level == C_LEVEL)
			continue;

		rc = 0;
		for (prof_index = 1; rc == 0; prof_index++) {
			rc = tm_read_drop_profile(ctl,
				TM_LEVEL(level),
				(uint8_t)TM_INVAL,
				prof_index,
				&dp_profile);
			if (rc)
				break;
		}
		if (prof_index - 1)
			pr_info("TM %s Level Drop Profiles Indexes: %d-%d\n",
					level_names_arr[level], 1, prof_index - 1);
		else
			pr_info("TM %s Level Drop Profiles Indexes: None\n",
					level_names_arr[level]);
	}

	rc = 0;

	TM_WRAPPER_END(tm_hndl);
}

int tm_sysfs_read_wred_curves(void)
{
	char *level_names_arr[] = { "Q", "A", "B", "C", "P"};

	uint8_t cos;
	uint8_t status;
	uint16_t index;
	int level;

	TM_WRAPPER_BEGIN(qmtm_hndl, ctl, henv);

	for (level = P_LEVEL; level >= Q_LEVEL; level--) {
		switch (level) {
		case P_LEVEL:
			for (index = 1; index <= TM_NUM_WRED_PORT_CURVES; index++) {
				rc = rm_wred_port_curve_status(ctl->rm, index, &status);
				if (rc) {
					rc = -1;
					TM_WRAPPER_END(tm_hndl);
				}

				if (status == TM_DISABLE) /* end */
					break;
			}
			if (index - 1)
				pr_info("TM %s Level WRED Curves Indexes (Global): %d-%d\n",
						level_names_arr[level], 1, index - 1);
			else
				pr_info("P_LEVEL!: index = %d, rc = %d, status = %d, level = %s\n", index, rc, status, level_names_arr[level]);
				pr_info("TM %s Level WRED Curves Indexes: None\n",
						level_names_arr[level]);

			pr_info("TM %s Level WRED Curves Indexes (per Cos):\n", level_names_arr[level]);
			for (cos = 0; cos < TM_WRED_COS; cos++) {
				for (index = 1; index <= TM_NUM_WRED_PORT_CURVES; index++) {
					rc = rm_wred_port_curve_status_cos(ctl->rm, cos, index, &status);
					if (rc) {
						rc = -1;
						TM_WRAPPER_END(tm_hndl);
					}

					if (status == TM_DISABLE) /* end */
						break;
				}
				if (index - 1)
					pr_info("   Cos %d: %d-%d\n", cos, 1, index - 1);
				else
					pr_info("   Cos %d: None\n", cos);
			}
			break;

		case C_LEVEL:
			pr_info("TM %s Level WRED Curves Indexes (per Cos):\n", level_names_arr[level]);
			for (cos = 0; cos < TM_WRED_COS; cos++) {
				for (index = 1; index <= TM_NUM_WRED_PORT_CURVES; index++) {
					rc = rm_wred_c_node_curve_status(ctl->rm, cos, index, &status);
					if (rc) {
						rc = -1;
						TM_WRAPPER_END(tm_hndl);
					}

					if (status == TM_DISABLE) /* end */
						break;
				}
				if (index - 1)
					pr_info("   Cos %d: %d-%d\n", cos, 1, index - 1);
				else
					pr_info("   Cos %d: None\n", cos);
			}
			break;

		case B_LEVEL:
			for (index = 1; index <= TM_NUM_WRED_B_NODE_CURVES; index++) {
				rc = rm_wred_b_node_curve_status(ctl->rm, index, &status);
				if (rc) {
					rc = -1;
					TM_WRAPPER_END(tm_hndl);
				}

				if (status == TM_DISABLE) /* end */
					break;
			}
			if (index - 1)
				pr_info("TM %s Level WRED Curves Indexes: %d-%d\n",
						level_names_arr[level], 1, index - 1);
			else
				pr_info("TM %s Level WRED Curves Indexes: None\n",
						level_names_arr[level]);
			break;

		case A_LEVEL:
			for (index = 1; index <= TM_NUM_WRED_A_NODE_CURVES; index++) {
				rc = rm_wred_a_node_curve_status(ctl->rm, index, &status);
				if (rc) {
					rc = -1;
					TM_WRAPPER_END(tm_hndl);
				}

				if (status == TM_DISABLE) /* end */
					break;
			}
			if (index - 1)
				pr_info("TM %s Level WRED Curves Indexes: %d-%d\n",
						level_names_arr[level], 1, index - 1);
			else
				pr_info("TM %s Level WRED Curves Indexes: None\n",
						level_names_arr[level]);
			break;

		case Q_LEVEL:
			for (index = 1; index <= TM_NUM_WRED_QUEUE_CURVES; index++) {
				rc = rm_wred_queue_curve_status(ctl->rm, index, &status);
				if (rc) {
					rc = -1;
					TM_WRAPPER_END(tm_hndl);
				}

				if (status == TM_DISABLE) /* end */
					break;
			}
			if (index - 1)
				pr_info("TM %s Level WRED Curves Indexes: %d-%d\n",
						level_names_arr[level], 1, index - 1);
			else
				pr_info("TM %s Level WRED Curves Indexes: None\n",
						level_names_arr[level]);
			break;
		} /* switch */
	}

	TM_WRAPPER_END(tm_hndl);
}

int tm_sysfs_params_show(void)
{
	TM_WRAPPER_BEGIN(qmtm_hndl, ctl, henv);

	if (flag == TM_DISABLE) {
		set_dp_default();
		flag = TM_ENABLE;
	}

	pr_info("cbtd threshold:			%d\n", context.cbtd_threshold);
	if (context.color_td_en)
		pr_info("CATD mode:");
	else
		pr_info("WRED mode:");

	pr_info("min_th[]:			%d %d %d\n",
		context.min_threshold[0],
		context.min_threshold[1],
		context.min_threshold[2]);

	if (context.color_td_en == TM_DISABLE) {
		pr_info("max_th[]:			%d %d %d\n",
			context.max_threshold[0],
			context.max_threshold[1],
			context.max_threshold[2]);

		pr_info("curve_id[]:		%d %d %d\n",
			context.curve_id[0],
			context.curve_id[1],
			context.curve_id[2]);

		pr_info("curve_scale[]:		%d %d %d\n",
			context.curve_scale[0],
			context.curve_scale[1],
			context.curve_scale[2]);
	}

	TM_WRAPPER_END(tm_hndl);
}

int tm_sysfs_read_drop_profile(int level, int cos, uint16_t index)
{
	struct tm_drop_profile *dp;
	int i;
	uint8_t lvl;
	uint32_t ind;

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
		goto out;
	}

	if (dp->td_thresh_res)
		pr_info("cbtd threshold:			%d\n", dp->td_threshold * TM_1K);
	else
		pr_info("cbtd threshold:			%d\n", dp->td_threshold);

	if (dp->color_td_en)
		pr_info("CATD mode:\n");
	else
		pr_info("WRED mode:\n");

	pr_info("min_th[]:			%d %d %d\n",
			dp->min_th_sw[0],
			dp->min_th_sw[1],
			dp->min_th_sw[2]);

	if (dp->color_td_en == TM_DISABLE) {
		pr_info("max_th[]:			%d %d %d\n",
			dp->max_th_sw[0],
			dp->max_th_sw[1],
			dp->max_th_sw[2]);

		pr_info("curve_id[]:		%d %d %d\n",
			dp->curve_id[0].index,
			dp->curve_id[1].index,
			dp->curve_id[2].index);

		pr_info("curve_scale[]:		%d %d %d\n",
			dp->dp_ratio[0].ratio,
			dp->dp_ratio[1].ratio,
			dp->dp_ratio[2].ratio);
	}

	if ((level == P_LEVEL) && (dp->use_counter != 0)) {
		/* list not empty */
		pr_info("Ports list:");
		for (i = dp->use_counter; i > 0; i--) {
			if (i == dp->use_counter) {
				rc = rm_list_reset_to_start(ctl->rm, dp->use_list, &ind, &lvl);
				pr_info(" %d", ind);
			} else {
				rc = rm_list_next_index(ctl->rm, dp->use_list, &ind, &lvl);
				pr_info(" %d", ind);
			}
		}
		pr_info("\n");
	}
out:
	if (rc)
		pr_info("tm_sysfs_read_drop_profile error: %d\n", rc);
	TM_WRAPPER_END(qmtm_hndl);
}

int tm_sysfs_read_drop_profile_bw(int level, int cos, uint16_t prof_index)
{
	struct tm_drop_profile_params dp_profile = {0};

	TM_WRAPPER_BEGIN(qmtm_hndl, ctl, henv);

	if (cos == -1)
		rc = tm_read_drop_profile(ctl, TM_LEVEL(level), (uint8_t) TM_INVAL, prof_index, &dp_profile);
	else
		rc = tm_read_drop_profile(ctl, TM_LEVEL(level), (uint8_t) cos, prof_index, &dp_profile);
	if (rc == 0) {
		pr_info("wred_catd_bw:		%d\n", dp_profile.wred_catd_bw);
		pr_info("cbtd_bw:			%d\n", dp_profile.cbtd_bw);
		/*pr_info("cbtd_rtt_ratio:	%d\n", dp_profile.cbtd_rtt_ratio);*/
		/*pr_info("aql_exp:			%d\n", dp_profile.aql_exp);*/
		pr_info("wred_catd_mode:	%d\n", dp_profile.wred_catd_mode);
		pr_info("curve_id[]:		%d %d %d\n",
				dp_profile.curve_id[0],
				dp_profile.curve_id[1],
				dp_profile.curve_id[2]);

		pr_info("dp_ratio[]:        %d %d %d\n",
				dp_profile.dp_ratio[0],
				dp_profile.dp_ratio[1],
				dp_profile.dp_ratio[2]);

		pr_info("min_th[]:          %d %d %d\n",
				dp_profile.min_th[0],
				dp_profile.min_th[1],
				dp_profile.min_th[2]);

		pr_info("max_th[]:          %d %d %d\n",
				dp_profile.max_th[0],
				dp_profile.max_th[1],
				dp_profile.max_th[2]);
	} else
		pr_info("tm_read_drop_profile error: %d\n", rc);

	TM_WRAPPER_END(qmtm_hndl);
}

int tm_sysfs_cbtd_thr_set(uint32_t threshold)
{
	uint32_t max_thresh;

	TM_WRAPPER_BEGIN(qmtm_hndl, ctl, henv);

	if (flag == TM_DISABLE) {
		set_dp_default();
		flag = TM_ENABLE;
	}

	max_thresh = get_drop_threshold_definition() * TM_1K;
	if (threshold > max_thresh) {
		rc = -1;
		pr_info("tm_sysfs_cbtd_thr_set error: %d\n", rc);
		TM_WRAPPER_END(qmtm_hndl);
	}
	context.cbtd_threshold = threshold;

	TM_WRAPPER_END(qmtm_hndl);
}

int tm_sysfs_catd_thr_set(int color, uint32_t threshold)
{
	int i;
	uint32_t max_thresh;

	TM_WRAPPER_BEGIN(qmtm_hndl, ctl, henv);

	if (flag == TM_DISABLE) {
		set_dp_default();
		flag = TM_ENABLE;
	}

	max_thresh = 1023 * (uint32_t)(1 << 22);
	if (threshold > max_thresh) {
		rc = -1;
		pr_info("tm_sysfs_catd_thr_set error: %d\n", rc);
		TM_WRAPPER_END(qmtm_hndl);
	}

	if (((color > 3) && (color != TM_ALL_COLORS)) || (color < 0)) {
		rc = -1;
		pr_info("tm_sysfs_catd_thr_set error: %d\n", rc);
		TM_WRAPPER_END(qmtm_hndl);
	}

	if (color == TM_ALL_COLORS) {
		for (i = 0; i < MV_TM_NUM_OF_COLORS; i++) {
			context.min_threshold[i] = threshold;
			context.max_threshold[i] = 0;
		}
	} else {
		context.min_threshold[color] = threshold;
		context.max_threshold[color] = 0;
	}
	context.color_td_en = TM_ENABLE;

	TM_WRAPPER_END(qmtm_hndl);
}

int tm_sysfs_wred_thr_set(int color, uint32_t min, uint32_t max)
{
	int i;
	uint32_t max_thresh;

	TM_WRAPPER_BEGIN(qmtm_hndl, ctl, henv);

	if (flag == TM_DISABLE) {
		set_dp_default();
		flag = TM_ENABLE;
	}

	max_thresh = 1023 * (uint32_t)(1 << 22);
	if ((max > max_thresh) || (min > max)) {
		rc = -1;
		pr_info("tm_sysfs_wred_thr_set error: %d\n", rc);
		TM_WRAPPER_END(qmtm_hndl);
	}

	if (((color > 3) && (color != TM_ALL_COLORS)) || (color < 0)) {
		rc = -1;
		pr_info("tm_sysfs_catd_thr_set error: %d\n", rc);
		TM_WRAPPER_END(qmtm_hndl);
	}

	if (color == TM_ALL_COLORS) {
		for (i = 0; i < MV_TM_NUM_OF_COLORS; i++) {
			context.min_threshold[i] = min;
			context.max_threshold[i] = max;
		}
	} else {
		context.min_threshold[color] = min;
		context.max_threshold[color] = max;
	}
	context.color_td_en = TM_DISABLE;

	TM_WRAPPER_END(qmtm_hndl);
}

int tm_sysfs_wred_curve_set(int color, uint32_t curve_ind, uint32_t curve_scale)
{
	int i;

	TM_WRAPPER_BEGIN(qmtm_hndl, ctl, henv);

	if (flag == TM_DISABLE) {
		set_dp_default();
		flag = TM_ENABLE;
	}

	if (curve_ind > 8) {
		rc = -1;
		pr_info("tm_sysfs_wred_curve_set error: %d\n", rc);
		TM_WRAPPER_END(qmtm_hndl);
	}

	if (((color > 3) && (color != TM_ALL_COLORS)) || (color < 0)) {
		rc = -1;
		pr_info("tm_sysfs_catd_thr_set error: %d\n", rc);
		TM_WRAPPER_END(qmtm_hndl);
	}

	if (color == TM_ALL_COLORS)
		for (i = 0; i < MV_TM_NUM_OF_COLORS; i++) {
			context.curve_id[i] = curve_ind;
			context.curve_scale[i] = curve_scale;
		}
	else {
		context.curve_id[color] = curve_ind;
		context.curve_scale[color] = curve_scale;
	}

	TM_WRAPPER_END(qmtm_hndl);
}

int tm_sysfs_drop_profile_set(int level, uint16_t index, int cos)
{
	TM_WRAPPER_BEGIN(qmtm_hndl, ctl, henv);

	if (flag == TM_DISABLE) {
		rc = -2;
		pr_info("tm_sysfs_drop_profile_set error: %d\n", rc);
		TM_WRAPPER_END(qmtm_hndl);
	}

	rc = mv_tm_drop_profile_set(level, index, cos, &context);
	set_dp_default();

	TM_WRAPPER_END(qmtm_hndl);
}


