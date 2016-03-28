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

#include "tm_drop.h"
#include "tm_errcodes.h"

#include "rm_internal_types.h"
#include "rm_alloc.h"
#include "rm_free.h"
#include "rm_list.h"
#include "rm_status.h"

#include "set_hw_registers.h"
#include "tm_set_local_db_defaults.h"
#include "tm_os_interface.h"
#include "tm_locking_interface.h"
#include "tm_hw_configuration_interface.h"


static int tm_round_int(uint32_t val, uint32_t divider)
{
	return (val * 100 + 50) / (divider * 100);
}


/**
 */
int update_curves_at_level(tm_handle hndl,
						enum tm_level level,
						uint8_t curve_ind,
						uint8_t max_prob,
						uint8_t old_mode)
{
	int i;
	uint8_t j;
	uint8_t k;
	uint8_t status;
	struct tm_wred_curve *curve = NULL;
	int rc = 0;

	DECLARE_TM_CTL_PTR(ctl, hndl)
	DECLARE_RM_HANDLE(rm, ctl->rm)
	CHECK_TM_CTL_PTR(ctl)

	switch (level) {
	case Q_LEVEL:
		for (j = 0; j < TM_NUM_WRED_QUEUE_CURVES; j++) {
			rc = rm_wred_queue_curve_status(rm, j, &status);
			if (rc)
				return rc;

			if ((status == TM_DISABLE) || (j == curve_ind)) /* end */
				break;

			curve = &(ctl->tm_wred_q_lvl_curves[j]);
			for (i = 0; i < TM_WRED_CURVE_POINTS; i++)
				curve->prob[i] =
					(uint8_t)tm_round_int(curve->prob[i]*old_mode, max_prob);

			/* Download to HW */
			rc = set_hw_queues_wred_curve(ctl, j);
			if (rc)
				return rc;
		}
		break;
	case A_LEVEL:
		for (j = 0; j < TM_NUM_WRED_A_NODE_CURVES; j++) {
			rc = rm_wred_a_node_curve_status(rm, j, &status);
			if (rc)
				return rc;
			if ((status == TM_DISABLE) || (j == curve_ind)) /* end */
				break;

			curve = &(ctl->tm_wred_a_lvl_curves[j]);
			for (i = 0; i < TM_WRED_CURVE_POINTS; i++)
				curve->prob[i] =
					(uint8_t)tm_round_int(curve->prob[i]*old_mode, max_prob);

			/* Download to HW */
			rc = set_hw_a_nodes_wred_curve(ctl, j);
			if (rc)
				return rc;
		}
		break;
	case B_LEVEL:
		for (j = 0; j < TM_NUM_WRED_B_NODE_CURVES; j++) {
			rc = rm_wred_b_node_curve_status(rm, j, &status);
			if (rc)
				return rc;
			if ((status == TM_DISABLE) || (j == curve_ind)) /* end */
				break;

			curve = &(ctl->tm_wred_b_lvl_curves[j]);
			for (i = 0; i < TM_WRED_CURVE_POINTS; i++)
				curve->prob[i] =
					(uint8_t)tm_round_int(curve->prob[i]*old_mode, max_prob);
			/* Download to HW */
			rc = set_hw_b_nodes_wred_curve(ctl, j);
			if (rc)
				return rc;
		}
		break;
	case C_LEVEL:
		for (k = 0; k < TM_WRED_COS; k++) {
			for (j = 0; j < TM_NUM_WRED_C_NODE_CURVES; j++) {
				rc = rm_wred_c_node_curve_status(rm, k, j, &status);
				if (rc)
					return rc;
				if ((status == TM_DISABLE) || (j == curve_ind)) /* end */
					break;

				curve = &(ctl->tm_wred_c_lvl_curves[k][j]);
				for (i = 0; i < TM_WRED_CURVE_POINTS; i++)
					curve->prob[i] =
						(uint8_t)tm_round_int(curve->prob[i]*old_mode, max_prob);
				/* Download to HW */
				rc = set_hw_c_nodes_wred_curve(ctl, k, j);
				if (rc)
					return rc;
			}
		}
		break;
	case P_LEVEL:
		for (j = 0; j < TM_NUM_WRED_PORT_CURVES; j++) {
			rc = rm_wred_port_curve_status(rm, j, &status);
			if (rc)
				return rc;
			if ((status == TM_DISABLE) || (j == curve_ind)) /* end */
				break;

			curve = &(ctl->tm_wred_ports_curves[j]);
			for (i = 0; i < TM_WRED_CURVE_POINTS; i++)
				curve->prob[i] =
					(uint8_t)tm_round_int(curve->prob[i]*old_mode, max_prob);
			/* Download to HW */
			rc = set_hw_ports_wred_curve(ctl, j);
			if (rc)
				return rc;
		}
		for (k = 0; k < TM_WRED_COS; k++) {
			for (j = 0; j < TM_NUM_WRED_PORT_CURVES; j++) {
				rc = rm_wred_port_curve_status_cos(rm, k, j, &status);
				if (rc)
					return rc;
				if ((status == TM_DISABLE) || (j == curve_ind)) /* end */
					break;

				curve = &(ctl->tm_wred_ports_curves_cos[k][j]);
				for (i = 0; i < TM_WRED_CURVE_POINTS; i++)
					curve->prob[i] =
						(uint8_t)tm_round_int(curve->prob[i]*old_mode, max_prob);
				/* Download to HW */
				rc = set_hw_ports_wred_curve_cos(ctl, k, j);
				if (rc)
					return rc;
			}
		}
		break;
	}
	return rc;
}


/**
 */
int tm_create_wred_curve(tm_handle hndl,
						enum tm_level level,
						uint8_t cos,
						uint8_t *prob,
						uint8_t *curve_index)
{

	struct tm_wred_curve *curve;
	uint8_t max_prob = 0;
	uint8_t old_mode = 0;
	uint8_t res;
	uint8_t exp;

	int i;
	int rc;
	int curve_ind = (int)TM_INVAL;
	uint8_t flag_mode_change = TM_DISABLE;

	DECLARE_TM_CTL_PTR(ctl, hndl)
	DECLARE_RM_HANDLE(rm, ctl->rm)
	CHECK_TM_CTL_PTR(ctl)

	rc = tm_glob_lock(TM_ENV(ctl));
	if (rc)
		return rc;

	if (level > P_LEVEL) {
		rc = -EADDRNOTAVAIL;
		goto out;
	}

	max_prob = prob[0];
	for (i = 0; i < TM_WRED_CURVE_POINTS; i++) {
		if (prob[i] > 100) {
			rc = -EPERM;
			goto out;
		}
		if (prob[i] > max_prob)
			max_prob = prob[i];
	}


	/* find free curve */
	switch (level) {
	case Q_LEVEL:
		curve_ind = rm_find_free_wred_queue_curve(rm);
		break;
	case A_LEVEL:
		curve_ind = rm_find_free_wred_a_node_curve(rm);
		break;
	case B_LEVEL:
		curve_ind = rm_find_free_wred_b_node_curve(rm);
		break;
	case C_LEVEL:
		if (cos >= TM_WRED_COS) {
			rc = -EDOM;
			goto out;
		}
		curve_ind = rm_find_free_wred_c_node_curve(rm, cos);
		break;
	case P_LEVEL:
		if (cos == (uint8_t)TM_INVAL)
			curve_ind = rm_find_free_wred_port_curve(rm);
		else {
			if (cos >= TM_WRED_COS) {
				rc = -EDOM;
				goto out;
			}
			curve_ind = rm_find_free_wred_port_curve_cos(rm, cos);
		}
		break;
	default:
		break;
	}

	if (curve_ind < 0) {
		rc = -ENOSPC;
		goto out;
	} else
		*curve_index = (uint8_t)curve_ind;


	/* Check that max_p doesn't exceed dp_max mode. If - yes, update
	 * all existing curves at this level and for this color according to the new mode. */
	switch (ctl->dp_unit.local[level].max_p_mode[0]) {
	case TM_MAX_PROB_100: /* 100% */
		max_prob = 100;
		break;
	case TM_MAX_PROB_50: /* 50% */
		if (max_prob > 50) {
			max_prob = 100;
			old_mode = 50;
			for (i = 0; i < 3; i++)
				ctl->dp_unit.local[level].max_p_mode[i] = TM_MAX_PROB_100;
			rc = set_hw_max_dp_mode(ctl);

			/* Update all the rest of curves at this level */
			flag_mode_change = TM_ENABLE;
		} else
			max_prob = 50;
		break;
	case TM_MAX_PROB_25: /* 25% */
		if (max_prob > 25) {
			old_mode = 25;
			if (max_prob <= 50) {
				max_prob = 50;
				for (i = 0; i < 3; i++)
					ctl->dp_unit.local[level].max_p_mode[i] = TM_MAX_PROB_50;
			} else {
				max_prob = 100;
				for (i = 0; i < 3; i++)
					ctl->dp_unit.local[level].max_p_mode[i] = TM_MAX_PROB_100;
			}
			rc = set_hw_max_dp_mode(ctl);

			/* Update all the rest of curves at this level */
			flag_mode_change = TM_ENABLE;
		} else
			max_prob = 25;
		break;
	case TM_MAX_PROB_12H: /* 12.5% */
		if (max_prob > 12) {
			old_mode = 12;
			if (max_prob <= 25) {
				max_prob = 25;
				for (i = 0; i < 3; i++)
					ctl->dp_unit.local[level].max_p_mode[i] = TM_MAX_PROB_25;
			} else {
				if (max_prob <= 50) {
					max_prob = 50;
					for (i = 0; i < 3; i++)
						ctl->dp_unit.local[level].max_p_mode[i] =
							TM_MAX_PROB_50;
				} else {
					max_prob = 100;
					for (i = 0; i < 3; i++)
						ctl->dp_unit.local[level].max_p_mode[i] =
							TM_MAX_PROB_100;
				}
			}
			rc = set_hw_max_dp_mode(ctl);

			/* Update all the rest of curves at this level */
			flag_mode_change = TM_ENABLE;
		} else
			max_prob = 12;
		break;
	}

	if (rc) {
		rc = TM_HW_WRED_CURVE_FAILED;
		goto out;
	}

	if (flag_mode_change == TM_ENABLE) {
		/* Update all the rest of curves at this level */
		rc = update_curves_at_level(ctl, level, (uint8_t)curve_ind, max_prob, old_mode);
		if (rc) {
			rc = TM_HW_WRED_CURVE_FAILED;
			goto out;
		}
	}

	/* Calculate prob values of current curve */
	res = ctl->dp_unit.local[level].resolution;
	exp = (uint8_t)((1 << res) - 1);

	switch (level) {
	case Q_LEVEL:
		/* update SW image */
		curve = &(ctl->tm_wred_q_lvl_curves[*curve_index]);
		for (i = 0; i < TM_WRED_CURVE_POINTS; i++)
			curve->prob[i] = (uint8_t)tm_round_int(prob[i] * exp, max_prob);
		/* update HW */
		rc = set_hw_queues_wred_curve(hndl, *curve_index);
		break;
	case A_LEVEL:
		/* update SW image */
		curve = &(ctl->tm_wred_a_lvl_curves[*curve_index]);
		for (i = 0; i < TM_WRED_CURVE_POINTS; i++)
			curve->prob[i] = (uint8_t)tm_round_int(prob[i] * exp, max_prob);
		/* update HW */
		rc = set_hw_a_nodes_wred_curve(hndl, *curve_index);
		break;
	case B_LEVEL:
		/* update SW image */
		curve = &(ctl->tm_wred_b_lvl_curves[*curve_index]);
		for (i = 0; i < TM_WRED_CURVE_POINTS; i++)
			curve->prob[i] = (uint8_t)tm_round_int(prob[i] * exp, max_prob);
		/* update HW */
		rc = set_hw_b_nodes_wred_curve(hndl, *curve_index);
		break;
	case C_LEVEL:
		/* update SW image */
		curve = &(ctl->tm_wred_c_lvl_curves[cos][*curve_index]);
		for (i = 0; i < TM_WRED_CURVE_POINTS; i++)
			curve->prob[i] = (uint8_t)tm_round_int(prob[i] * exp, max_prob);
		/* update HW */
		rc = set_hw_c_nodes_wred_curve(hndl, cos, *curve_index);
		break;
	case P_LEVEL:
		/* update SW image */
		if (cos == (uint8_t)TM_INVAL) /* Global Port */
			curve = &(ctl->tm_wred_ports_curves[*curve_index]);
		else
			curve = &(ctl->tm_wred_ports_curves_cos[cos][*curve_index]);
		for (i = 0; i < TM_WRED_CURVE_POINTS; i++)
			curve->prob[i] = (uint8_t)tm_round_int(prob[i] * exp, max_prob);
		/* update HW */
		if (cos == (uint8_t)TM_INVAL) /* Global Port */
			rc = set_hw_ports_wred_curve(hndl, *curve_index);
		else
			rc = set_hw_ports_wred_curve_cos(hndl, cos, *curve_index);
	default:
		break;
	}
	if (rc)
		rc = TM_HW_WRED_CURVE_FAILED;
out:
	if (rc) {
		switch (level) {
		case Q_LEVEL:
			if (curve_ind >= 0)
				rm_free_wred_queue_curve(rm, *curve_index);
			if (rc == TM_HW_WRED_CURVE_FAILED)
				set_sw_wred_curve_default(ctl->tm_wred_a_lvl_curves,
										*curve_index);
			break;
		case A_LEVEL:
			if (curve_ind >= 0)
				rm_free_wred_a_node_curve(rm, *curve_index);
			if (rc == TM_HW_WRED_CURVE_FAILED)
				set_sw_wred_curve_default(ctl->tm_wred_a_lvl_curves,
										*curve_index);
			break;
		case B_LEVEL:
			if (curve_ind >= 0)
				rm_free_wred_b_node_curve(rm, *curve_index);
			if (rc == TM_HW_WRED_CURVE_FAILED)
				set_sw_wred_curve_default(ctl->tm_wred_b_lvl_curves,
										*curve_index);
			break;
		case C_LEVEL:
			if (curve_ind >= 0)
				rm_free_wred_c_node_curve(rm, cos, *curve_index);
			if (rc == TM_HW_WRED_CURVE_FAILED)
				set_sw_wred_curve_default(ctl->tm_wred_c_lvl_curves[cos],
										*curve_index);
			break;
		case P_LEVEL:
			if (curve_ind >= 0) {
				if (cos == (uint8_t)TM_INVAL) /* Global Port */
					rm_free_wred_port_curve(rm, *curve_index);
				else
					rm_free_wred_port_curve_cos(rm, cos, *curve_index);
			}
			if (rc == TM_HW_WRED_CURVE_FAILED) {
				if (cos == (uint8_t)TM_INVAL) /* Global Port */
					set_sw_wred_curve_default(ctl->tm_wred_ports_curves, *curve_index);
				else
					set_sw_wred_curve_default(ctl->tm_wred_ports_curves_cos[cos], *curve_index);
			}
			break;
		default:
			break;
		}
	}
	tm_glob_unlock(TM_ENV(ctl));
	return rc;
}


/**
*/
int tm_create_wred_traditional_curve(tm_handle hndl,
									enum tm_level level,
									uint8_t cos,
									uint8_t mp,
									uint8_t *curve_index)
{
	uint8_t res;
	uint8_t exp;
	int i;
	int rc;
	struct tm_wred_curve *curve = NULL;
	int curve_ind = (int)TM_INVAL;
	uint8_t mp_scaled = 0;
	uint8_t max_prob = 0;
	uint8_t old_mode = 0;
	uint8_t flag_mode_change = TM_DISABLE;

	DECLARE_TM_CTL_PTR(ctl, hndl)
	DECLARE_RM_HANDLE(rm, ctl->rm)
	CHECK_TM_CTL_PTR(ctl)

	rc = tm_glob_lock(TM_ENV(ctl));
	if (rc)
		return rc;

	/* check parameters */
	if (level > P_LEVEL) {
		rc = -EADDRNOTAVAIL;
		goto out;
	}

	if ((mp == 0) || (mp > 100)) {
		rc = -EPERM;
		goto out;
	}

	/* find free curve */
	switch (level) {
	case Q_LEVEL:
		curve_ind = rm_find_free_wred_queue_curve(rm);
		break;
	case A_LEVEL:
		curve_ind = rm_find_free_wred_a_node_curve(rm);
		break;
	case B_LEVEL:
		curve_ind = rm_find_free_wred_b_node_curve(rm);
		break;
	case C_LEVEL:
		if (cos >= TM_WRED_COS) {
			rc = -EDOM;
			goto out;
		}
		curve_ind = rm_find_free_wred_c_node_curve(rm, cos);
		break;
	case P_LEVEL:
		if (cos == (uint8_t)TM_INVAL)
			curve_ind = rm_find_free_wred_port_curve(rm);
		else {
			if (cos >= TM_WRED_COS) {
				rc = -EDOM;
				goto out;
			}
			curve_ind = rm_find_free_wred_port_curve_cos(rm, cos);
		}
		break;
	default:
		break;
	}
	if (curve_ind < 0) {
		rc = -ENOSPC;
		goto out;
	} else
		*curve_index = (uint8_t)curve_ind;

	res = ctl->dp_unit.local[level].resolution;
	exp = (uint8_t)((1 << res) - 1);

	switch (ctl->dp_unit.local[level].max_p_mode[0]) {
	case TM_MAX_PROB_100: /* 100% */
		mp_scaled = (uint8_t)tm_round_int(mp*exp, 100);
		max_prob = 100;
		break;
	case TM_MAX_PROB_50: /* 50% */
		if (mp > 50) {
			mp_scaled = (uint8_t)tm_round_int(mp*exp, 100);
			old_mode = 50;
			max_prob = 100;
			for (i = 0; i < 3; i++)
				ctl->dp_unit.local[level].max_p_mode[i] = TM_MAX_PROB_100;
			rc = set_hw_max_dp_mode(ctl);

			/* Update all the rest of curves at this level */
			flag_mode_change = TM_ENABLE;
		} else
			mp_scaled = (uint8_t)tm_round_int(mp*exp, 50);
		break;
	case TM_MAX_PROB_25: /* 25% */
		if (mp > 25) {
			old_mode = 25;
			if (mp <= 50) {
				mp_scaled = (uint8_t)tm_round_int(mp*exp, 50);
				max_prob = 50;
				for (i = 0; i < 3; i++)
					ctl->dp_unit.local[level].max_p_mode[i] = TM_MAX_PROB_50;
			} else {
				mp_scaled = (uint8_t)tm_round_int(mp*exp, 100);
				max_prob = 100;
				for (i = 0; i < 3; i++)
					ctl->dp_unit.local[level].max_p_mode[i] = TM_MAX_PROB_100;
			}
			rc = set_hw_max_dp_mode(ctl);

			/* Update all the rest of curves at this level */
			flag_mode_change = TM_ENABLE;
		} else
			mp_scaled = (uint8_t)tm_round_int(mp*exp, 25);
		break;
	case TM_MAX_PROB_12H: /* 12.5% */
		if (mp > 12) {
			old_mode = 12;
			if (mp <= 25) {
				mp_scaled = (uint8_t)tm_round_int(mp*exp, 25);
				max_prob = 25;
				for (i = 0; i < 3; i++)
					ctl->dp_unit.local[level].max_p_mode[i] = TM_MAX_PROB_25;
			} else {
				if (mp <= 50) {
					mp_scaled = (uint8_t)tm_round_int(mp*exp, 50);
					max_prob = 50;
					for (i = 0; i < 3; i++)
						ctl->dp_unit.local[level].max_p_mode[i] =
							TM_MAX_PROB_50;
				} else {
					mp_scaled = (uint8_t)tm_round_int(mp*exp, 100);
					max_prob = 100;
					for (i = 0; i < 3; i++)
						ctl->dp_unit.local[level].max_p_mode[i] =
							TM_MAX_PROB_100;
				}
			}
			rc = set_hw_max_dp_mode(ctl);

			/* Update all the rest of curves at this level */
			flag_mode_change = TM_ENABLE;
		} else
			mp_scaled = (uint8_t)tm_round_int(mp*exp, 12);
		break;
	}
	if (rc) {
		rc = TM_HW_WRED_CURVE_FAILED;
		goto out;
	}


	if (flag_mode_change == TM_ENABLE) {
		/* Update all the rest of curves at this level */
		rc = update_curves_at_level(ctl, level, (uint8_t)curve_ind, max_prob, old_mode);
		if (rc) {
			rc = TM_HW_WRED_CURVE_FAILED;
			goto out;
		}
	}


	/* update SW image */
	switch (level) {
	case Q_LEVEL:
		curve = &(ctl->tm_wred_q_lvl_curves[*curve_index]);
		break;
	case A_LEVEL:
		curve = &(ctl->tm_wred_a_lvl_curves[*curve_index]);
		break;
	case B_LEVEL:
		curve = &(ctl->tm_wred_b_lvl_curves[*curve_index]);
		break;
	case C_LEVEL:
		curve = &(ctl->tm_wred_c_lvl_curves[cos][*curve_index]);
		break;
	case P_LEVEL:
		if (cos == (uint8_t)TM_INVAL) /* Global Port */
			curve = &(ctl->tm_wred_ports_curves[*curve_index]);
		else
			curve = &(ctl->tm_wred_ports_curves_cos[cos][*curve_index]);
		break;
	default:
		break;
	}

	/* Calculate drop probability for each of 32 points */
	for (i = 0; i < TM_WRED_CURVE_POINTS; i++)
		curve->prob[i] = (uint8_t)tm_round_int((uint8_t)mp_scaled * (i+1), (uint8_t)TM_WRED_CURVE_POINTS);


	/* update HW */
	switch (level) {
	case Q_LEVEL:
		rc = set_hw_queues_wred_curve(hndl, *curve_index);
		break;
	case A_LEVEL:
		rc = set_hw_a_nodes_wred_curve(hndl, *curve_index);
		break;
	case B_LEVEL:
		rc = set_hw_b_nodes_wred_curve(hndl, *curve_index);
		break;
	case C_LEVEL:
		rc = set_hw_c_nodes_wred_curve(hndl, cos, *curve_index);
		break;
	case P_LEVEL:
		if (cos == (uint8_t)TM_INVAL) /* Global Port */
			rc = set_hw_ports_wred_curve(hndl, *curve_index);
		else
			rc = set_hw_ports_wred_curve_cos(hndl, cos, *curve_index);
		break;
	default:
		break;
	}
	if (rc)
		rc = TM_HW_WRED_CURVE_FAILED;
out:
	if (rc) {
		switch (level) {
		case Q_LEVEL:
			if (curve_ind >= 0)
				rm_free_wred_queue_curve(rm, *curve_index);
			if (rc == TM_HW_WRED_CURVE_FAILED)
				set_sw_wred_curve_default(ctl->tm_wred_q_lvl_curves,
										*curve_index);
			break;
		case A_LEVEL:
			if (curve_ind >= 0)
				rm_free_wred_a_node_curve(rm, *curve_index);
			if (rc == TM_HW_WRED_CURVE_FAILED)
				set_sw_wred_curve_default(ctl->tm_wred_a_lvl_curves,
										*curve_index);
			break;
		case B_LEVEL:
			if (curve_ind >= 0)
				rm_free_wred_b_node_curve(rm, *curve_index);
			if (rc == TM_HW_WRED_CURVE_FAILED)
				set_sw_wred_curve_default(ctl->tm_wred_b_lvl_curves,
										*curve_index);
			break;
		case C_LEVEL:
			if (curve_ind >= 0)
				rm_free_wred_c_node_curve(rm, cos, *curve_index);
			if (rc == TM_HW_WRED_CURVE_FAILED)
				set_sw_wred_curve_default(ctl->tm_wred_c_lvl_curves[cos],
										*curve_index);
			break;
		case P_LEVEL:
			if (curve_ind >= 0) {
				if (cos == (uint8_t)TM_INVAL) /* Global Port */
					rm_free_wred_port_curve(rm, *curve_index);
				else
					rm_free_wred_port_curve_cos(rm, cos, *curve_index);
			}
			if (rc == TM_HW_WRED_CURVE_FAILED) {
				if (cos == (uint8_t)TM_INVAL) /* Global Port */
					set_sw_wred_curve_default(ctl->tm_wred_ports_curves, *curve_index);
				else
					set_sw_wred_curve_default(ctl->tm_wred_ports_curves_cos[cos], *curve_index);
			}
			break;
		default:
			break;
		}
	}
	tm_glob_unlock(TM_ENV(ctl));
	return rc;
}
int tm_create_wred_flat_curve(tm_handle hndl,
									 enum tm_level level,
									 uint8_t cos,
									 uint8_t cp,
									 uint8_t *curve_index)
{
	uint8_t prob[32];
	int i;
	for (i = 0 ; i < 32 ; i++)
		prob[i] = cp;
	return tm_create_wred_curve(hndl, level, cos, prob, curve_index);
}


/**
*/
int tm_create_drop_profile(tm_handle hndl,
						enum tm_level level,
						uint8_t cos,
						struct tm_drop_profile_params *profile,
						uint16_t *prof_index)
{
	struct tm_drop_profile *node_profile;
	int i;
	int rc;
	uint8_t status;
	uint32_t td_threshold;
	uint8_t exp;
	uint32_t max_th;
	uint32_t max_thresh_scaled = 0;
	uint32_t min_th;
	uint32_t min_thresh_scaled = 0;
	uint16_t ratio;
	uint32_t out_bw = profile->wred_catd_bw;
	uint8_t color_num;
	struct rm_list *list = NULL;
	int prof_ind = (int)TM_INVAL;

	DECLARE_TM_CTL_PTR(ctl, hndl)
	DECLARE_RM_HANDLE(rm, ctl->rm)
	CHECK_TM_CTL_PTR(ctl)

	color_num = ctl->dp_unit.local[level].color_num;

	rc = tm_glob_lock(TM_ENV(ctl));
	if (rc)
		return rc;

	if (level > P_LEVEL) {
		rc = -EPERM;
		goto out;
	}

	if (profile->wred_catd_bw > TM_MAX_BW) {
		rc = -EACCES;
		goto out;
	}

	if (profile->cbtd_bw > TM_MAX_BW) {
		rc = -EACCES;
		goto out;
	}

	if (profile->aql_exp > 0xF) {
		rc = -EFAULT;
		goto out;
	}

	if ((profile->wred_catd_mode != WRED) &&
		(profile->wred_catd_mode != CATD) &&
		(profile->wred_catd_mode != DISABLED)) {
		rc = -ENODATA;
		goto out;
	}

	if (profile->wred_catd_bw == 0) {
		if (profile->wred_catd_mode != DISABLED) {
			rc = -ENODATA;
			goto out;
		}
	}

	if (profile->wred_catd_mode == WRED) {
		for (i = 0; i < color_num; i++) {
			if (profile->max_th[i] < profile->min_th[i]) {
				rc = -ERANGE;
				goto out;
			}

			if (i == 2) {
				if (profile->dp_ratio[i] > 16) { /* 4 bits */
					rc = -ERANGE;
					goto out;
				}
			} else {
				if (profile->dp_ratio[i] > 63) { /* 6 bits */
					rc = -ERANGE;
					goto out;
				}
			}
		}
	}

	for (i = 0; i < color_num; i++) {
		/* check if the assigned WRED curve exists */
		switch (level) {
		case Q_LEVEL:
			rc = rm_wred_queue_curve_status(rm, profile->curve_id[i],
											&status);
			break;
		case A_LEVEL:
			rc = rm_wred_a_node_curve_status(rm, profile->curve_id[i],
											 &status);
			break;
		case B_LEVEL:
			rc = rm_wred_b_node_curve_status(rm, profile->curve_id[i],
											 &status);
			break;
		case C_LEVEL:
			rc = rm_wred_c_node_curve_status(rm, cos, profile->curve_id[i],
											 &status);
			break;
		case P_LEVEL:
			if (cos == (uint8_t)TM_INVAL) /* Global Port */
				rc = rm_wred_port_curve_status(rm, profile->curve_id[i],
											&status);
			else
				rc = rm_wred_port_curve_status_cos(rm, cos, profile->curve_id[i],
										&status);
			break;
		default:
			break;
		}

		/* if referenced WRED curve doesn't exist */
		if ((rc < 0) || (status != RM_TRUE)) {
			rc = -ENODEV;
			goto out;
		}
	}

	/* find free profile */
	switch (level) {
	case Q_LEVEL:
		prof_ind = rm_find_free_queue_drop_profile(rm);
		break;
	case A_LEVEL:
		prof_ind = rm_find_free_a_node_drop_profile(rm);
		break;
	case B_LEVEL:
		prof_ind = rm_find_free_b_node_drop_profile(rm);
		break;
	case C_LEVEL:
		prof_ind = rm_find_free_c_node_drop_profile(rm, cos);
		break;
	case P_LEVEL:
		if (cos == (uint8_t)TM_INVAL) /* Global Port */
			prof_ind = rm_find_free_port_drop_profile(rm);
		else
			prof_ind = rm_find_free_port_drop_profile_cos(rm, cos);
		rc = rm_list_create(rm, &list);
		break;
	default:
		break;
	}

	if ((prof_ind < 0) || rc) {
		rc = -ENOSPC;
		goto out;
	} else
		*prof_index = prof_ind;

	switch (level) {
	case Q_LEVEL:
		node_profile = &(ctl->tm_q_lvl_drop_profiles[*prof_index]);
		break;
	case A_LEVEL:
		node_profile = &(ctl->tm_a_lvl_drop_profiles[*prof_index]);
		break;
	case B_LEVEL:
		node_profile = &(ctl->tm_b_lvl_drop_profiles[*prof_index]);
		break;
	case C_LEVEL:
		node_profile = &(ctl->tm_c_lvl_drop_profiles[cos][*prof_index]);
		break;
	case P_LEVEL:
		if (cos == (uint8_t)TM_INVAL) /* Global Port */
			node_profile = &(ctl->tm_p_lvl_drop_profiles[*prof_index]);
		else
			node_profile = &(ctl->tm_p_lvl_drop_profiles_cos[cos][*prof_index]);
		break;
	default:
		rc = -EPERM;
		goto out;
		break;
	}

	/* update SW image */
	for (i = 0; i < 3; i++) {
		node_profile->min_th_sw[i] = profile->min_th[i];  /**< Min Threshold ratio from RTT in % per color */
		node_profile->max_th_sw[i] = profile->max_th[i];  /**< Max Threshold ratio from RTT in % per color */
	}

	node_profile->use_list = list;

	node_profile->out_bw = out_bw;
	node_profile->aql_exp = profile->aql_exp;
	if (profile->wred_catd_mode != DISABLED) {
		if (profile->wred_catd_mode == WRED) {
			node_profile->color_td_en = TM_DISABLE;
			/* calculate min_th, scale_exp and scale_ratio for each color */
			for (i = 0; i < color_num; i++) {
				node_profile->curve_id[i].index = profile->curve_id[i];
				max_th = profile->max_th[i];
				min_th = profile->min_th[i];
				/* find highest value for exp when max_thresh_scaled smaller from 1024 */
				for (exp = 0; exp < 22; exp++) {
					/* 64 = (8[byte]*16[burst]*100[%]*1000[msec])/(TM_KBITS*TM_RTT) */
					max_thresh_scaled =
						((uint8_t)max_th*(uint8_t)out_bw/(uint8_t)64)/(uint8_t)(1 << exp);
					if (max_thresh_scaled < 1024)
						break;
				}
				min_thresh_scaled = ((min_th*out_bw)/64)/(1<<exp);
				node_profile->scale_exp[i].exp = exp;
				/* 1024 * 32 = 0x8000 */
				ratio =
					(uint16_t)tm_round_int(0x8000, (max_thresh_scaled-min_thresh_scaled+1));
				if (ratio > 1023)
					node_profile->scale_ratio[i].ratio = 1023;
				else
					node_profile->scale_ratio[i].ratio = ratio;
				node_profile->min_threshold[i].thresh = (uint16_t)min_thresh_scaled;
				node_profile->dp_ratio[i].ratio = profile->dp_ratio[i];
			}
			for (i = color_num; i < 3; i++) {
				/* Disable WRED for not used colors */
				node_profile->curve_id[i].index = 0;
				node_profile->scale_exp[i].exp = 22;
				node_profile->scale_ratio[i].ratio = 0;
				node_profile->min_threshold[i].thresh = 1023;
				node_profile->dp_ratio[i].ratio = 0;
			}
		} else { /* CATD */
			node_profile->color_td_en = TM_ENABLE;
			for (i = 0; i < color_num; i++) {
				min_th = profile->min_th[i];
				/* find highest value for exp when min_thresh_scaled smaller from 1024 */
				for (exp = 0; exp < 22; exp++) {
					/* 64 = 8[byte]*16[burst]*100[%]*1000[msec]/(TM_RTT*TM_KBITS) */
					min_thresh_scaled = ((min_th*out_bw)/64)/(1<<exp);
					if (min_thresh_scaled < 1024)
						break;
				}
				node_profile->min_threshold[i].thresh = (uint16_t)min_thresh_scaled;
				node_profile->curve_id[i].index = 0;
				node_profile->scale_exp[i].exp = exp;
				node_profile->scale_ratio[i].ratio = 0;
				node_profile->dp_ratio[i].ratio = 0;
			}
			for (i = color_num; i < 3; i++) {
				/* Disable WRED for not used colors */
				node_profile->curve_id[i].index = 0;
				node_profile->scale_exp[i].exp = 22;
				node_profile->scale_ratio[i].ratio = 0;
				node_profile->min_threshold[i].thresh = 1023;
				node_profile->dp_ratio[i].ratio = 0;
			}
		}
	} else { /* DISABLED = CBTD only */
		node_profile->color_td_en = TM_DISABLE;
		node_profile->aql_exp = 0;
		for (i = 0; i < 3; i++) {
			node_profile->curve_id[i].index = 0;
			node_profile->scale_exp[i].exp = 22;
			node_profile->scale_ratio[i].ratio = 0;
			node_profile->min_threshold[i].thresh = 1023;
			node_profile->dp_ratio[i].ratio = 0;
		}
	}

	/* Calculate TD threshold */
	if (profile->cbtd_bw == TM_MAX_BW) {
		/* Disable CBTD */
		node_profile->td_thresh_res = TM_ENABLE;
		node_profile->td_threshold = get_drop_threshold_definition();
	} else {
		/* 64 = 8[byte]*16[burst]*100[%]*1000[usec]/(TM_RTT*TM_KBITS) */
		td_threshold = (profile->cbtd_bw*profile->cbtd_rtt_ratio)/64;
		if (td_threshold > get_drop_threshold_definition()) {
			node_profile->td_thresh_res = TM_ENABLE;
			node_profile->td_threshold = td_threshold/1024;
		} else {
			node_profile->td_thresh_res = TM_DISABLE;
			node_profile->td_threshold = td_threshold;
		}
	}
	node_profile->cbtd_bw = profile->cbtd_bw;

	/* update HW */
	switch (level) {
	case Q_LEVEL:
		rc = set_hw_queue_drop_profile(hndl, *prof_index);
		break;
	case A_LEVEL:
		rc = set_hw_a_nodes_drop_profile(hndl, *prof_index);
		break;
	case B_LEVEL:
		rc = set_hw_b_nodes_drop_profile(hndl, *prof_index);
		break;
	case C_LEVEL:
		rc = set_hw_c_nodes_drop_profile(hndl, cos, *prof_index);
		break;
	case P_LEVEL:
		/* Port Drop profile should be set to hw later in time of port's
		 * creation */
		break;
	default:
		break;
	}
	if (rc)
		rc = TM_HW_DROP_PROFILE_FAILED;
out:
	if (rc) {
		switch (level) {
		case Q_LEVEL:
			if (prof_ind >= 0)
				rm_free_queue_drop_profile(rm, *prof_index);
			if (rc == TM_HW_DROP_PROFILE_FAILED)
				set_sw_drop_profile_default(ctl->tm_q_lvl_drop_profiles,
											*prof_index);
			break;
		case A_LEVEL:
			if (prof_ind >= 0)
				rm_free_a_node_drop_profile(rm, *prof_index);
			if (rc == TM_HW_DROP_PROFILE_FAILED)
				set_sw_drop_profile_default(ctl->tm_a_lvl_drop_profiles,
											*prof_index);
			break;
		case B_LEVEL:
			if (prof_ind >= 0)
				rm_free_b_node_drop_profile(rm, *prof_index);
			if (rc == TM_HW_DROP_PROFILE_FAILED)
				set_sw_drop_profile_default(ctl->tm_b_lvl_drop_profiles,
											*prof_index);
			break;
		case C_LEVEL:
			if (prof_ind >= 0)
				rm_free_c_node_drop_profile(rm, cos, *prof_index);
			if (rc == TM_HW_DROP_PROFILE_FAILED)
				set_sw_drop_profile_default(ctl->tm_c_lvl_drop_profiles[cos],
											*prof_index);
			break;
		case P_LEVEL:
			if (list)
				rm_list_delete(rm, list);
			if (prof_ind >= 0) {
				if (cos == (uint8_t)TM_INVAL) /* Global Port */
					rm_free_port_drop_profile(rm, *prof_index);
				else
					rm_free_port_drop_profile_cos(rm, cos, *prof_index);
			}
			if (rc == TM_HW_DROP_PROFILE_FAILED) {
				if (cos == (uint8_t)TM_INVAL) /* Global Port */
					set_sw_drop_profile_default(ctl->tm_p_lvl_drop_profiles, *prof_index);
				else
					set_sw_drop_profile_default(ctl->tm_p_lvl_drop_profiles_cos[cos], *prof_index);
			}
			break;
		default:
			break;
		}
	}
	tm_glob_unlock(TM_ENV(ctl));
	return rc;
}


/**
*/
int tm_update_drop_profile(tm_handle hndl,
						enum tm_level level,
						uint8_t cos,
						uint16_t index,
						struct tm_drop_profile_params *profile)
{
	struct tm_drop_profile *dp;
	int i;
	int rc;
	uint8_t fl_change = TM_DISABLE;

	uint8_t status;
	uint8_t color_num;
	uint32_t td_threshold;
	uint8_t exp;
	uint32_t max_th;
	uint32_t max_thresh_scaled = 0;
	uint32_t min_th;
	uint32_t min_thresh_scaled = 0;
	uint16_t ratio;
	uint32_t out_bw = profile->wred_catd_bw;

	uint32_t ind;
	uint8_t lvl;

	DECLARE_TM_CTL_PTR(ctl, hndl)
	DECLARE_RM_HANDLE(rm, ctl->rm)
	CHECK_TM_CTL_PTR(ctl)

	rc = tm_glob_lock(TM_ENV(ctl));
	if (rc)
		return rc;

	if (level > P_LEVEL) {
		rc = -EPERM;
		goto out;
	}

	/* Must parameter! */
	if ((profile->wred_catd_mode != WRED) &&
		(profile->wred_catd_mode != CATD) &&
		(profile->wred_catd_mode != DISABLED)) {
		rc = -ENODATA;
		goto out;
	}

	color_num = ctl->dp_unit.local[level].color_num;

	switch (level) {
	case Q_LEVEL:
		rc = rm_queue_drop_profile_status(rm, index, &status);
		if ((rc < 0) || (status != RM_TRUE))
			return -EFAULT;
		dp = &(ctl->tm_q_lvl_drop_profiles[index]);
		break;
	case A_LEVEL:
		rc = rm_a_node_drop_profile_status(rm, index, &status);
		if ((rc < 0) || (status != RM_TRUE))
			return -EFAULT;
		dp = &(ctl->tm_a_lvl_drop_profiles[index]);
		break;
	case B_LEVEL:
		rc = rm_b_node_drop_profile_status(rm, index, &status);
		if ((rc < 0) || (status != RM_TRUE))
			return -EFAULT;
		dp = &(ctl->tm_b_lvl_drop_profiles[index]);
		break;
	case C_LEVEL:
		rc = rm_c_node_drop_profile_status(rm, cos, index, &status);
		if ((rc < 0) || (status != RM_TRUE))
			return -EFAULT;
		dp = &(ctl->tm_c_lvl_drop_profiles[cos][index]);
		break;
	case P_LEVEL:
		if (cos == (uint8_t)TM_INVAL) { /* Global Port */
			rc = rm_port_drop_profile_status(rm, index, &status);
			if ((rc < 0) || (status != RM_TRUE))
				return -EFAULT;
			dp = &(ctl->tm_p_lvl_drop_profiles[index]);
		} else {
			rc = rm_port_drop_profile_status_cos(rm, cos, index, &status);
			if ((rc < 0) || (status != RM_TRUE))
				return -EFAULT;
			dp = &(ctl->tm_p_lvl_drop_profiles_cos[cos][index]);
		}
		break;
	default:
		return -EPERM;
		break;
	}

	/* CBTD */
	if (profile->cbtd_bw != TM_INVAL) {
		if (profile->cbtd_bw > TM_MAX_BW) {
			rc = -EACCES;
			goto out;
		}

		if (profile->cbtd_bw == TM_MAX_BW) { /* Disable CBTD */
			dp->td_thresh_res = TM_ENABLE;
			dp->td_threshold = get_drop_threshold_definition();
		} else {
			/* 64 = 8[byte]*16[burst]*100[%]*1000[usec]/(TM_RTT*TM_KBITS) */
			td_threshold = (profile->cbtd_bw*profile->cbtd_rtt_ratio)/64;
			if (td_threshold > get_drop_threshold_definition()) {
				dp->td_thresh_res = TM_ENABLE;
				dp->td_threshold = td_threshold/1024;
			} else {
				dp->td_thresh_res = TM_DISABLE;
				dp->td_threshold = td_threshold;
			}
		}
		dp->cbtd_bw = profile->cbtd_bw;
		fl_change = TM_ENABLE;
	}

	if ((profile->wred_catd_mode == DISABLED) && (dp->out_bw != 0)) {
		dp->color_td_en = TM_DISABLE;
		dp->out_bw = 0;
		dp->aql_exp = 0;
		for (i = 0; i < 3; i++) {
			dp->curve_id[i].index = 0;
			dp->scale_exp[i].exp = 22;
			dp->scale_ratio[i].ratio = 0;
			dp->min_threshold[i].thresh = 1023;
			dp->dp_ratio[i].ratio = 0;
			dp->min_th_sw[i] = 0;
			dp->max_th_sw[i] = 0;
		}
		fl_change = TM_ENABLE;
	}

	/* CA/WRED */
	if (profile->wred_catd_mode != DISABLED) {
		if (profile->aql_exp != (uint8_t) TM_INVAL) {
			if (profile->aql_exp > 0xF) {
				rc = -EFAULT;
				goto out;
			}

			dp->aql_exp = profile->aql_exp;
			fl_change = TM_ENABLE;
		}

		if (profile->wred_catd_bw > TM_MAX_BW) {
			rc = -EACCES;
			goto out;
		}
	}

	/* WRED */
	if (profile->wred_catd_mode == WRED) {
		for (i = 0; i < color_num; i++) {
			if (profile->max_th[i] < profile->min_th[i]) {
				rc = -ERANGE;
				goto out;
			}

			if (i == 2) {
				if (profile->dp_ratio[i] > 16) { /* 4 bits */
					rc = -ERANGE;
					goto out;
				}
			} else
				if (profile->dp_ratio[i] > 63) { /* 6 bits */
					rc = -ERANGE;
					goto out;
				}

			switch (level) {
			case Q_LEVEL:
				if (profile->curve_id[i] >= TM_NUM_WRED_QUEUE_CURVES) {
					rc = -EADDRNOTAVAIL;
					goto out;
				}
				/* check if the assigned WRED curve exists */
				rc = rm_wred_queue_curve_status(rm, profile->curve_id[i],
											&status);
				break;
			case A_LEVEL:
				if (profile->curve_id[i] >= TM_NUM_WRED_A_NODE_CURVES) {
					rc = -EADDRNOTAVAIL;
					goto out;
				}
				rc = rm_wred_a_node_curve_status(rm, profile->curve_id[i],
											 &status);
				break;
			case B_LEVEL:
				if (profile->curve_id[i] >= TM_NUM_WRED_B_NODE_CURVES) {
					rc = -EADDRNOTAVAIL;
					goto out;
				}
				rc = rm_wred_b_node_curve_status(rm, profile->curve_id[i],
											 &status);
				break;
			case C_LEVEL:
				if (cos > TM_WRED_COS) {
					rc = -EADDRNOTAVAIL;
					goto out;
				}
				if (profile->curve_id[i] >= TM_NUM_WRED_C_NODE_CURVES) {
					rc = -EADDRNOTAVAIL;
					goto out;
				}
				rc = rm_wred_c_node_curve_status(rm, cos, profile->curve_id[i],
											 &status);
				break;
			case P_LEVEL:
				if (profile->curve_id[i] >= TM_NUM_WRED_PORT_CURVES) {
					rc = -EADDRNOTAVAIL;
					goto out;
				}
				if (cos == (uint8_t)TM_INVAL) /* Global Port */
					rc = rm_wred_port_curve_status(rm, profile->curve_id[i],
											&status);
				else
					rc = rm_wred_port_curve_status_cos(rm, cos, profile->curve_id[i],
										&status);
				break;
			default:
				break;
			}
			/* if referenced WRED curve doesn't exist */
			if ((rc < 0) || (status != RM_TRUE)) {
				rc = -ENODEV;
				goto out;
			}

			dp->min_th_sw[i] = profile->min_th[i];
			dp->max_th_sw[i] = profile->max_th[i];
		}

		dp->color_td_en = TM_DISABLE; /* WRED mode */
		dp->out_bw = out_bw;

		/* calculate min_th, scale_exp and scale_ratio for each color */
		for (i = 0; i < color_num; i++) {
			dp->curve_id[i].index = profile->curve_id[i];
			max_th = profile->max_th[i];
			min_th = profile->min_th[i];
			/* find highest value for exp when max_thresh_scaled smaller from 1024 */
			for (exp = 0; exp < 22; exp++) {
				/* 64 = (8[byte]*16[burst]*100[%]*1000[msec])/(TM_KBITS*TM_RTT) */
				max_thresh_scaled =
					((uint8_t)max_th*(uint8_t)out_bw/(uint8_t)64)/(uint8_t)(1 << exp);
				if (max_thresh_scaled < 1024)
					break;
			}
			min_thresh_scaled = ((min_th*out_bw)/64)/(1<<exp);
			dp->scale_exp[i].exp = exp;
			/* 1024 * 32 = 0x8000 */
			ratio =
			(uint16_t)tm_round_int(0x8000, (max_thresh_scaled-min_thresh_scaled+1));
			if (ratio > 1023)
				dp->scale_ratio[i].ratio = 1023;
			else
				dp->scale_ratio[i].ratio = ratio;
			dp->min_threshold[i].thresh = (uint16_t)min_thresh_scaled;
			dp->dp_ratio[i].ratio = profile->dp_ratio[i];
		}
		for (i = color_num; i < 3; i++) {
			dp->curve_id[i].index = 0;
			dp->scale_exp[i].exp = 22;
			dp->scale_ratio[i].ratio = 0;
			dp->min_threshold[i].thresh = 1023;
			dp->dp_ratio[i].ratio = 0;
		}
		fl_change = TM_ENABLE;
	}

	/* CATD */
	if (profile->wred_catd_mode == CATD) {
		dp->color_td_en = TM_ENABLE;
		dp->out_bw = out_bw;
		for (i = 0; i < color_num; i++) {
			min_th = profile->min_th[i];
			/* find highest value for exp when min_thresh_scaled smaller from 1024 */
			for (exp = 0; exp < 22; exp++) {
				/* 64 = 8[byte]*16[burst]*100[%]*1000[msec]/(TM_RTT*TM_KBITS) */
				min_thresh_scaled = ((min_th*out_bw)/64)/(1<<exp);
				if (min_thresh_scaled < 1024)
					break;
			}
			dp->min_threshold[i].thresh = (uint16_t)min_thresh_scaled;
			dp->curve_id[i].index = 0;
			dp->scale_exp[i].exp = exp;
			dp->scale_ratio[i].ratio = 0;
			dp->dp_ratio[i].ratio = 0;
		}
		for (i = color_num; i < 3; i++) {
			dp->curve_id[i].index = 0;
			dp->scale_exp[i].exp = 22;
			dp->scale_ratio[i].ratio = 0;
			dp->min_threshold[i].thresh = 1023;
			dp->dp_ratio[i].ratio = 0;
		}
		fl_change = TM_ENABLE;
	}

	if (fl_change == TM_ENABLE) {
		/* update HW */
		switch (level) {
		case Q_LEVEL:
			rc = set_hw_queue_drop_profile(hndl, index);
			break;
		case A_LEVEL:
			rc = set_hw_a_nodes_drop_profile(hndl, index);
			break;
		case B_LEVEL:
			rc = set_hw_b_nodes_drop_profile(hndl, index);
			break;
		case C_LEVEL:
			rc = set_hw_c_nodes_drop_profile(hndl, cos, index);
			break;
		case P_LEVEL:
			i = dp->use_counter;
			if (i == 0) /* list empty */
				break;
			for (; i > 0; i--) {
				if (i == dp->use_counter)
					rc = rm_list_reset_to_start(rm, dp->use_list, &ind, &lvl);
				else
					rc = rm_list_next_index(rm, dp->use_list, &ind, &lvl);
				if (rc || (lvl != P_LEVEL))
					goto out;
				if (cos == (uint8_t)TM_INVAL) /* Global Port */
					rc = set_hw_port_drop(hndl, ind);
				else
					rc = set_hw_port_drop_cos(hndl, ind, cos);
				if (rc)
					rc = TM_HW_PORT_CONFIG_FAIL;
			}
			break;
		default:
			break;
		}
		if (rc)
			rc = TM_HW_DROP_PROFILE_FAILED;
	}
out:
	if (rc == TM_HW_DROP_PROFILE_FAILED)
		switch (level) {
		case Q_LEVEL:
			set_sw_drop_profile_default(ctl->tm_q_lvl_drop_profiles, index);
			break;
		case A_LEVEL:
			set_sw_drop_profile_default(ctl->tm_a_lvl_drop_profiles, index);
			break;
		case B_LEVEL:
			set_sw_drop_profile_default(ctl->tm_b_lvl_drop_profiles, index);
			break;
		case C_LEVEL:
			set_sw_drop_profile_default(ctl->tm_c_lvl_drop_profiles[cos], index);
			break;
		case P_LEVEL:
			if (cos == (uint8_t)TM_INVAL) /* Global Port */
				set_sw_drop_profile_default(ctl->tm_p_lvl_drop_profiles, index);
			else
				set_sw_drop_profile_default(ctl->tm_p_lvl_drop_profiles_cos[cos], index);
			break;
		default:
			break;
		}
	tm_glob_unlock(TM_ENV(ctl));
	return rc;
}


/**
*/
int tm_drop_profile_hw_set(tm_handle hndl,
						enum tm_level level,
						uint8_t cos,
						uint16_t index)
{
	struct tm_drop_profile *dp;
	int i = 0;
	int rc;

	uint32_t ind;
	uint8_t lvl;

	DECLARE_TM_CTL_PTR(ctl, hndl)
	DECLARE_RM_HANDLE(rm, ctl->rm)
	CHECK_TM_CTL_PTR(ctl)

	rc = tm_glob_lock(TM_ENV(ctl));
	if (rc)
		return rc;

	if (level > P_LEVEL) {
		rc = -EPERM;
		goto out;
	}

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
		if (cos == (uint8_t) TM_INVAL)
			dp = &(ctl->tm_p_lvl_drop_profiles[index]);
		else
			dp = &(ctl->tm_p_lvl_drop_profiles_cos[cos][index]);
		break;
	default:
		rc = -EPERM;
		goto out;
	}

	/* update HW */
	switch (level) {
	case Q_LEVEL:
		rc = set_hw_queue_drop_profile(ctl, index);
		break;
	case A_LEVEL:
		rc = set_hw_a_nodes_drop_profile(ctl, index);
		break;
	case B_LEVEL:
		rc = set_hw_b_nodes_drop_profile(ctl, index);
		break;
	case C_LEVEL:
		rc = set_hw_c_nodes_drop_profile(ctl, cos, index);
		break;
	case P_LEVEL:
		i = dp->use_counter;
		if (i == 0) /* list empty */
			break;
		for (; i > 0; i--) {
			if (i == dp->use_counter)
				rc = rm_list_reset_to_start(rm, dp->use_list, &ind, &lvl);
			else
				rc = rm_list_next_index(rm, dp->use_list, &ind, &lvl);
			if (rc || (lvl != P_LEVEL))
				rc = TM_HW_DROP_PROFILE_FAILED;
			if (cos == (uint8_t)TM_INVAL) /* Global Port */
				rc = set_hw_port_drop(ctl, ind);
			else
				rc = set_hw_port_drop_cos(ctl, ind, cos);
		}
		break;
	default:
		rc = -EPERM;
		goto out;
	}
	if (rc)
		rc = TM_HW_DROP_PROFILE_FAILED;
out:
	tm_glob_unlock(TM_ENV(ctl));
	return rc;
}


/**
*/
int tm_create_drop_profile_1G(tm_handle hndl,
						enum tm_level level,
						uint8_t cos,
						uint16_t *prof_index)
{
	struct tm_drop_profile *node_profile;
	int i;
	int rc;
	uint32_t td_threshold;
	struct rm_list *list = NULL;
	int prof_ind = (int)TM_INVAL;

	DECLARE_TM_CTL_PTR(ctl, hndl)
	DECLARE_RM_HANDLE(rm, ctl->rm)
	CHECK_TM_CTL_PTR(ctl)

	rc = tm_glob_lock(TM_ENV(ctl));
	if (rc)
		return rc;

	if (level > P_LEVEL) {
		rc = -EPERM;
		goto out;
	}

	/* find free profile */
	switch (level) {
	case Q_LEVEL:
		prof_ind = rm_find_free_queue_drop_profile(rm);
		break;
	case A_LEVEL:
		prof_ind = rm_find_free_a_node_drop_profile(rm);
		break;
	case B_LEVEL:
		prof_ind = rm_find_free_b_node_drop_profile(rm);
		break;
	case C_LEVEL:
		prof_ind = rm_find_free_c_node_drop_profile(rm, cos);
		break;
	case P_LEVEL:
		if (cos == (uint8_t)TM_INVAL) /* Global Port */
			prof_ind = rm_find_free_port_drop_profile(rm);
		else
			prof_ind = rm_find_free_port_drop_profile_cos(rm, cos);
		rc = rm_list_create(rm, &list);
		break;
	default:
		break;
	}

	if ((prof_ind < 0) || rc) {
		rc = -ENOSPC;
		goto out;
	} else
		*prof_index = prof_ind;


	switch (level) {
	case Q_LEVEL:
		node_profile = &(ctl->tm_q_lvl_drop_profiles[*prof_index]);
		break;
	case A_LEVEL:
		node_profile = &(ctl->tm_a_lvl_drop_profiles[*prof_index]);
		break;
	case B_LEVEL:
		node_profile = &(ctl->tm_b_lvl_drop_profiles[*prof_index]);
		break;
	case C_LEVEL:
		node_profile = &(ctl->tm_c_lvl_drop_profiles[cos][*prof_index]);
		break;
	case P_LEVEL:
		if (cos == (uint8_t)TM_INVAL) /* Global Port */
			node_profile = &(ctl->tm_p_lvl_drop_profiles[*prof_index]);
		else
			node_profile = &(ctl->tm_p_lvl_drop_profiles_cos[cos][*prof_index]);
		break;
	default:
		rc = -EPERM;
		goto out;
		break;
	}

	/* update SW image */
	for (i = 0; i < 3; i++) {
		node_profile->min_th_sw[i] = TM_DISABLE;  /**< Min Threshold ratio from RTT in % per color */
		node_profile->max_th_sw[i] = TM_DISABLE;  /**< Max Threshold ratio from RTT in % per color */
	}
	node_profile->use_list = list;

	/* DISABLED = CBTD only */
	node_profile->out_bw = TM_DISABLE;
	node_profile->aql_exp = TM_DISABLE;
	node_profile->color_td_en = TM_DISABLE;
	node_profile->aql_exp = 0;
	for (i = 0; i < 3; i++) {
		node_profile->curve_id[i].index = 0;
		node_profile->scale_exp[i].exp = 22;
		node_profile->scale_ratio[i].ratio = 0;
		node_profile->min_threshold[i].thresh = 1023;
		node_profile->dp_ratio[i].ratio = 0;
	 }

	/* TD threshold for 1G */
	td_threshold = 1638400;
	if (td_threshold > get_drop_threshold_definition()) {
		node_profile->td_thresh_res = TM_ENABLE;
		node_profile->td_threshold = td_threshold/1024;
	} else {
		node_profile->td_thresh_res = TM_DISABLE;
		node_profile->td_threshold = td_threshold;
	}
	node_profile->cbtd_bw = 1000000;

	/* update HW */
	switch (level) {
	case Q_LEVEL:
		rc = set_hw_queue_drop_profile(hndl, *prof_index);
		break;
	case A_LEVEL:
		rc = set_hw_a_nodes_drop_profile(hndl, *prof_index);
		break;
	case B_LEVEL:
		rc = set_hw_b_nodes_drop_profile(hndl, *prof_index);
		break;
	case C_LEVEL:
		rc = set_hw_c_nodes_drop_profile(hndl, cos, *prof_index);
		break;
	case P_LEVEL:
		/* Port Drop profile should be set to hw later in time of port's
		 * creation */
		break;
	default:
		break;
	}
	if (rc)
		rc = TM_HW_DROP_PROFILE_FAILED;
out:
	if (rc) {
		switch (level) {
		case Q_LEVEL:
			if (prof_ind >= 0)
				rm_free_queue_drop_profile(rm, *prof_index);
			if (rc == TM_HW_DROP_PROFILE_FAILED)
				set_sw_drop_profile_default(ctl->tm_q_lvl_drop_profiles,
											*prof_index);
			break;
		case A_LEVEL:
			if (prof_ind >= 0)
				rm_free_a_node_drop_profile(rm, *prof_index);
			if (rc == TM_HW_DROP_PROFILE_FAILED)
				set_sw_drop_profile_default(ctl->tm_a_lvl_drop_profiles,
											*prof_index);
			break;
		case B_LEVEL:
			if (prof_ind >= 0)
				rm_free_b_node_drop_profile(rm, *prof_index);
			if (rc == TM_HW_DROP_PROFILE_FAILED)
				set_sw_drop_profile_default(ctl->tm_b_lvl_drop_profiles,
											*prof_index);
			break;
		case C_LEVEL:
			if (prof_ind >= 0)
				rm_free_c_node_drop_profile(rm, cos, *prof_index);
			if (rc == TM_HW_DROP_PROFILE_FAILED)
				set_sw_drop_profile_default(ctl->tm_c_lvl_drop_profiles[cos],
											*prof_index);
			break;
		case P_LEVEL:
			if (list)
				rm_list_delete(rm, list);
			if (prof_ind >= 0) {
				if (cos == (uint8_t)TM_INVAL) /* Global Port */
					rm_free_port_drop_profile(rm, *prof_index);
				else
					rm_free_port_drop_profile_cos(rm, cos, *prof_index);
			}
			if (rc == TM_HW_DROP_PROFILE_FAILED) {
				if (cos == (uint8_t)TM_INVAL) /* Global Port */
					set_sw_drop_profile_default(ctl->tm_p_lvl_drop_profiles, *prof_index);
				else
					set_sw_drop_profile_default(ctl->tm_p_lvl_drop_profiles_cos[cos], *prof_index);
			}
			break;
		default:
			break;
		}
	}
	tm_glob_unlock(TM_ENV(ctl));
	return rc;
}


/**
*/
int tm_create_drop_profile_2_5G(tm_handle hndl,
						enum tm_level level,
						uint8_t cos,
						uint16_t *prof_index)
{
	struct tm_drop_profile *node_profile;
	int i;
	int rc;
	uint32_t td_threshold;
	struct rm_list *list = NULL;
	int prof_ind = (int)TM_INVAL;

	DECLARE_TM_CTL_PTR(ctl, hndl)
	DECLARE_RM_HANDLE(rm, ctl->rm)
	CHECK_TM_CTL_PTR(ctl)

	rc = tm_glob_lock(TM_ENV(ctl));
	if (rc)
		return rc;

	if (level > P_LEVEL) {
		rc = -EPERM;
		goto out;
	}

	/* find free profile */
	switch (level) {
	case Q_LEVEL:
		prof_ind = rm_find_free_queue_drop_profile(rm);
		break;
	case A_LEVEL:
		prof_ind = rm_find_free_a_node_drop_profile(rm);
		break;
	case B_LEVEL:
		prof_ind = rm_find_free_b_node_drop_profile(rm);
		break;
	case C_LEVEL:
		prof_ind = rm_find_free_c_node_drop_profile(rm, cos);
		break;
	case P_LEVEL:
		if (cos == (uint8_t)TM_INVAL) /* Global Port */
			prof_ind = rm_find_free_port_drop_profile(rm);
		else
			prof_ind = rm_find_free_port_drop_profile_cos(rm, cos);
		rc = rm_list_create(rm, &list);
		break;
	default:
		break;
	}

	if ((prof_ind < 0) || rc) {
		rc = -ENOSPC;
		goto out;
	} else
		*prof_index = prof_ind;


	switch (level) {
	case Q_LEVEL:
		node_profile = &(ctl->tm_q_lvl_drop_profiles[*prof_index]);
		break;
	case A_LEVEL:
		node_profile = &(ctl->tm_a_lvl_drop_profiles[*prof_index]);
		break;
	case B_LEVEL:
		node_profile = &(ctl->tm_b_lvl_drop_profiles[*prof_index]);
		break;
	case C_LEVEL:
		node_profile = &(ctl->tm_c_lvl_drop_profiles[cos][*prof_index]);
		break;
	case P_LEVEL:
		if (cos == (uint8_t)TM_INVAL) /* Global Port */
			node_profile = &(ctl->tm_p_lvl_drop_profiles[*prof_index]);
		else
			node_profile = &(ctl->tm_p_lvl_drop_profiles_cos[cos][*prof_index]);
		break;
	default:
		rc = -EPERM;
		goto out;
		break;
	}

	/* update SW image */
	for (i = 0; i < 3; i++) {
		node_profile->min_th_sw[i] = TM_DISABLE;  /**< Min Threshold ratio from RTT in % per color */
		node_profile->max_th_sw[i] = TM_DISABLE;  /**< Max Threshold ratio from RTT in % per color */
	}
	node_profile->use_list = list;

	/* DISABLED = CBTD only */
	node_profile->out_bw = TM_DISABLE;
	node_profile->aql_exp = TM_DISABLE;
	node_profile->color_td_en = TM_DISABLE;
	node_profile->aql_exp = 0;
	for (i = 0; i < 3; i++) {
		node_profile->curve_id[i].index = 0;
		node_profile->scale_exp[i].exp = 22;
		node_profile->scale_ratio[i].ratio = 0;
		node_profile->min_threshold[i].thresh = 1023;
		node_profile->dp_ratio[i].ratio = 0;
	 }

	/* TD threshold for 2.5G */
	td_threshold = 4096000;
	if (td_threshold > get_drop_threshold_definition()) {
		node_profile->td_thresh_res = TM_ENABLE;
		node_profile->td_threshold = td_threshold/1024;
	} else {
		node_profile->td_thresh_res = TM_DISABLE;
		node_profile->td_threshold = td_threshold;
	}
	node_profile->cbtd_bw = 2500000;

	/* update HW */
	switch (level) {
	case Q_LEVEL:
		rc = set_hw_queue_drop_profile(hndl, *prof_index);
		break;
	case A_LEVEL:
		rc = set_hw_a_nodes_drop_profile(hndl, *prof_index);
		break;
	case B_LEVEL:
		rc = set_hw_b_nodes_drop_profile(hndl, *prof_index);
		break;
	case C_LEVEL:
		rc = set_hw_c_nodes_drop_profile(hndl, cos, *prof_index);
		break;
	case P_LEVEL:
		/* Port Drop profile should be set to hw later in time of port's
		 * creation */
		break;
	default:
		break;
	}
	if (rc)
		rc = TM_HW_DROP_PROFILE_FAILED;
out:
	if (rc) {
		switch (level) {
		case Q_LEVEL:
			if (prof_ind >= 0)
				rm_free_queue_drop_profile(rm, *prof_index);
			if (rc == TM_HW_DROP_PROFILE_FAILED)
				set_sw_drop_profile_default(ctl->tm_q_lvl_drop_profiles,
											*prof_index);
			break;
		case A_LEVEL:
			if (prof_ind >= 0)
				rm_free_a_node_drop_profile(rm, *prof_index);
			if (rc == TM_HW_DROP_PROFILE_FAILED)
				set_sw_drop_profile_default(ctl->tm_a_lvl_drop_profiles,
											*prof_index);
			break;
		case B_LEVEL:
			if (prof_ind >= 0)
				rm_free_b_node_drop_profile(rm, *prof_index);
			if (rc == TM_HW_DROP_PROFILE_FAILED)
				set_sw_drop_profile_default(ctl->tm_b_lvl_drop_profiles,
											*prof_index);
			break;
		case C_LEVEL:
			if (prof_ind >= 0)
				rm_free_c_node_drop_profile(rm, cos, *prof_index);
			if (rc == TM_HW_DROP_PROFILE_FAILED)
				set_sw_drop_profile_default(ctl->tm_c_lvl_drop_profiles[cos],
											*prof_index);
			break;
		case P_LEVEL:
			if (list)
				rm_list_delete(rm, list);
			if (prof_ind >= 0) {
				if (cos == (uint8_t)TM_INVAL) /* Global Port */
					rm_free_port_drop_profile(rm, *prof_index);
				else
					rm_free_port_drop_profile_cos(rm, cos, *prof_index);
			}
			if (rc == TM_HW_DROP_PROFILE_FAILED) {
				if (cos == (uint8_t)TM_INVAL) /* Global Port */
					set_sw_drop_profile_default(ctl->tm_p_lvl_drop_profiles, *prof_index);
				else
					set_sw_drop_profile_default(ctl->tm_p_lvl_drop_profiles_cos[cos], *prof_index);
			}
			break;
		default:
			break;
		}
	}
	tm_glob_unlock(TM_ENV(ctl));
	return rc;
}


/**
*/
int tm_delete_drop_profile(tm_handle hndl,
						enum tm_level level,
						uint8_t cos,
						uint16_t prof_index)
{
	int rc;
	DECLARE_TM_CTL_PTR(ctl, hndl)
	DECLARE_RM_HANDLE(rm, ctl->rm)
	CHECK_TM_CTL_PTR(ctl)

	rc = tm_glob_lock(TM_ENV(ctl));
	if (rc)
		return rc;

	if (level > P_LEVEL) {
		rc = -EDOM;
		goto out;
	}

	if (prof_index == 0) { /* reserved profile */
		rc = -EPERM;
		goto out;
	}

	switch (level) {
	case Q_LEVEL:
		if (prof_index >= TM_NUM_QUEUE_DROP_PROF) {
			rc = -EADDRNOTAVAIL;
			goto out;
		}

		if (ctl->tm_q_lvl_drop_profiles[prof_index].use_counter > 0) {
			rc = -EPERM;
			goto out;
		}

		/* free profile's entry in the RM (status is checked inside) */
		rc = rm_free_queue_drop_profile(rm, prof_index);
		if (rc < 0) {
			rc = -EFAULT;
			goto out;
		}

		/* set SW image entry to default values */
		set_sw_drop_profile_default(ctl->tm_q_lvl_drop_profiles, prof_index);
		break;
	case A_LEVEL:
		if (prof_index >= TM_NUM_A_NODE_DROP_PROF) {
			rc = -EADDRNOTAVAIL;
			goto out;
		}

		if (ctl->tm_a_lvl_drop_profiles[prof_index].use_counter > 0) {
			rc = -EPERM;
			goto out;
		}
		/* free profile's entry in the RM (status is checked inside) */
		rc = rm_free_a_node_drop_profile(rm, prof_index);
		if (rc < 0) {
			rc = -EFAULT;
			goto out;
		}

		/* set SW image entry to default values */
		set_sw_drop_profile_default(ctl->tm_a_lvl_drop_profiles, prof_index);
		break;
	case B_LEVEL:
		if (prof_index >= TM_NUM_B_NODE_DROP_PROF) {
			rc = -EADDRNOTAVAIL;
			goto out;
		}

		if (ctl->tm_b_lvl_drop_profiles[prof_index].use_counter > 0) {
			rc = -EPERM;
			goto out;
		}
		/* free profile's entry in the RM (status is checked inside) */
		rc = rm_free_b_node_drop_profile(rm, prof_index);
		if (rc < 0) {
			rc = -EFAULT;
			goto out;
		}

		/* set SW image entry to default values */
		set_sw_drop_profile_default(ctl->tm_b_lvl_drop_profiles, prof_index);
		break;
	case C_LEVEL:
		if (prof_index >= TM_NUM_C_NODE_DROP_PROF) {
			rc = -EADDRNOTAVAIL;
			goto out;
		}

		if (cos >= TM_WRED_COS) {
			rc = -ENODEV;
			goto out;
		}

		if (ctl->tm_c_lvl_drop_profiles[cos][prof_index].use_counter > 0) {
			rc = -EPERM;
			goto out;
		}
		/* free profile's entry in the RM (status is checked inside) */
		rc = rm_free_c_node_drop_profile(rm, cos, prof_index);
		if (rc < 0) {
			rc = -EFAULT;
			goto out;
		}

		/* set SW image entry to default values */
		set_sw_drop_profile_default(ctl->tm_c_lvl_drop_profiles[cos],
									prof_index);
		break;
	case P_LEVEL:
		if (prof_index >= TM_NUM_PORT_DROP_PROF) {
			rc = -EADDRNOTAVAIL;
			goto out;
		}

		if (cos == (uint8_t)TM_INVAL) { /* Global Port */
			if (ctl->tm_p_lvl_drop_profiles[prof_index].use_counter > 0) {
				rc = -EPERM;
				goto out;
			}

			rc = rm_list_delete(rm,
								ctl->tm_p_lvl_drop_profiles[prof_index].use_list);
			if (rc < 0) {
				rc = -EPERM;
				goto out;
			}

			/* free profile's entry in the RM (status is checked inside) */
			rc = rm_free_port_drop_profile(rm, prof_index);
			if (rc < 0) {
				rc = -EFAULT;
				goto out;
			}

			/* set SW image entry to default values */
			set_sw_drop_profile_default(ctl->tm_p_lvl_drop_profiles, prof_index);
		} else {
			if (cos >= TM_WRED_COS) {
				rc = -ENODEV;
				goto out;
			}

			if (ctl->tm_p_lvl_drop_profiles_cos[cos][prof_index].use_counter > 0) {
				rc = -EPERM;
				goto out;
			}

			rc = rm_list_delete(rm,
				ctl->tm_p_lvl_drop_profiles_cos[cos][prof_index].use_list);
			if (rc < 0) {
				rc = -EPERM;
				goto out;
			}

			/* free profile's entry in the RM (status is checked inside) */
			rc = rm_free_port_drop_profile_cos(rm, cos, prof_index);
			if (rc < 0) {
				rc = -EFAULT;
				goto out;
			}

			/* set SW image entry to default values */
			set_sw_drop_profile_default(ctl->tm_p_lvl_drop_profiles_cos[cos], prof_index);
		}
		break;
	default:
		break;
	}

	/* set HW to default */
	switch (level) {
	case Q_LEVEL:
		rc = set_hw_queue_drop_profile(hndl, prof_index);
		break;
	case A_LEVEL:
		rc = set_hw_a_nodes_drop_profile(hndl, prof_index);
		break;
	case B_LEVEL:
		rc = set_hw_b_nodes_drop_profile(hndl, prof_index);
		break;
	case C_LEVEL:
		rc = set_hw_c_nodes_drop_profile(hndl, cos, prof_index);
		break;
	default:
		break;
	}
	if (rc)
		rc = TM_HW_DROP_PROFILE_FAILED;
out:
	tm_glob_unlock(TM_ENV(ctl));
	return rc;
}


/**
*/
int tm_read_drop_profile(tm_handle hndl,
						 enum tm_level level,
						 uint8_t cos,
						 uint16_t prof_index,
						 struct tm_drop_profile_params *profile)
{

	uint8_t status;
	int rc;
	int i;
	uint32_t td_threshold;
	struct tm_drop_profile *drop_profile = NULL;
	uint8_t color_num;

	DECLARE_TM_CTL_PTR(ctl, hndl)
	DECLARE_RM_HANDLE(rm, ctl->rm)
	CHECK_TM_CTL_PTR(ctl)

	color_num = ctl->dp_unit.local[level].color_num;
	rc = tm_glob_lock(TM_ENV(ctl));
	if (rc)
		return rc;

	if (level > P_LEVEL) {
		rc = -EPERM;
		goto out;
	}

	switch (level) {
	case Q_LEVEL:
		if (prof_index >= TM_NUM_QUEUE_DROP_PROF) {
			rc = -EADDRNOTAVAIL;
			goto out;
		}

		/* check profile status */
		rc = rm_queue_drop_profile_status(rm, prof_index, &status);
		if ((rc < 0) || (status != RM_TRUE)) {
			rc = -EFAULT;
			goto out;
		}
		drop_profile = &(ctl->tm_q_lvl_drop_profiles[prof_index]);
		break;
	case A_LEVEL:
		if (prof_index >= TM_NUM_A_NODE_DROP_PROF) {
			rc = -EADDRNOTAVAIL;
			goto out;
		}
		/* check profile status */
		rc = rm_a_node_drop_profile_status(rm, prof_index, &status);
		if ((rc < 0) || (status != RM_TRUE)) {
			rc = -EFAULT;
			goto out;
		}
		drop_profile = &(ctl->tm_a_lvl_drop_profiles[prof_index]);
		break;
	case B_LEVEL:
		if (prof_index >= TM_NUM_B_NODE_DROP_PROF) {
			rc = -EADDRNOTAVAIL;
			goto out;
		}

		/* check profile status */
		rc = rm_b_node_drop_profile_status(rm, prof_index, &status);
		if ((rc < 0) || (status != RM_TRUE)) {
			rc = -EFAULT;
			goto out;
		}
		drop_profile = &(ctl->tm_b_lvl_drop_profiles[prof_index]);
		break;
	case C_LEVEL:
		if (prof_index >= TM_NUM_C_NODE_DROP_PROF) {
			rc = -EADDRNOTAVAIL;
			goto out;
		}
		/* check profile status */
		rc = rm_c_node_drop_profile_status(rm, cos, prof_index, &status);
		if ((rc < 0) || (status != RM_TRUE)) {
			rc = -EFAULT;
			goto out;
		}
		drop_profile = &(ctl->tm_c_lvl_drop_profiles[cos][prof_index]);
		break;
	case P_LEVEL:
		if (prof_index >= TM_NUM_PORT_DROP_PROF) {
			rc = -EADDRNOTAVAIL;
			goto out;
		}
		/* check profile status */
		if (cos == (uint8_t)TM_INVAL) { /* Global Port */
			rc = rm_port_drop_profile_status(rm, prof_index, &status);
			if ((rc < 0) || (status != RM_TRUE)) {
				rc = -EFAULT;
				goto out;
			}
			drop_profile = &(ctl->tm_p_lvl_drop_profiles[prof_index]);
		} else {
			rc = rm_port_drop_profile_status_cos(rm, cos, prof_index, &status);
			if ((rc < 0) || (status != RM_TRUE)) {
				rc = -EFAULT;
				goto out;
			}
			drop_profile = &(ctl->tm_p_lvl_drop_profiles_cos[cos][prof_index]);
		}
		break;
	default:
		rc = -EPERM;
		goto out;
		break;
	}

	profile->wred_catd_bw = drop_profile->out_bw;
	profile->aql_exp = drop_profile->aql_exp;
	if (drop_profile->out_bw == 0)
		profile->wred_catd_mode = DISABLED;
	else
		profile->wred_catd_mode = drop_profile->color_td_en;

	if (drop_profile->td_thresh_res == TM_ENABLE)
		td_threshold = drop_profile->td_threshold*1024;
	else
		td_threshold = drop_profile->td_threshold;
	profile->cbtd_bw = drop_profile->cbtd_bw;
	if (profile->cbtd_bw != 0)
		if (profile->cbtd_bw == TM_MAX_BW)
			profile->cbtd_rtt_ratio = 0;
		else
			/* 64 = 100*1000*8*16/(TM_KBITS*TM_RTT) */
			profile->cbtd_rtt_ratio = (td_threshold*64)/profile->cbtd_bw;
	else
		profile->cbtd_rtt_ratio = 0;
	if (profile->wred_catd_mode != DISABLED) {
		if (profile->wred_catd_mode == WRED) {
			for (i = 0; i < color_num; i++) {/* for each configured color */
				profile->curve_id[i] = drop_profile->curve_id[i].index;
				profile->min_th[i] = drop_profile->min_th_sw[i];
				profile->max_th[i] = drop_profile->max_th_sw[i];
				profile->dp_ratio[i] = drop_profile->dp_ratio[i].ratio;
			}

			for (i = color_num; i < 3; i++) { /* for not configured color */
				profile->curve_id[i] = 0;
				profile->min_th[i] = 0;
				profile->max_th[i] = 0;
				profile->dp_ratio[i] = 0;
			}
		} else { /* CATD */
			for (i = 0; i < color_num; i++) { /* for each configured color */
				profile->min_th[i] = drop_profile->min_th_sw[i];
				/* Unused fields */
				profile->max_th[i] = 0;
				profile->curve_id[i] = 0;
				profile->dp_ratio[i] = 0;
			}
			for (i = color_num; i < 3; i++) { /* for not configured color */
				profile->curve_id[i] = 0;
				profile->min_th[i] = 0;
				profile->max_th[i] = 0;
				profile->dp_ratio[i] = 0;
			}
		}
	} else {
		for (i = 0; i < 3; i++) { /* for each color */
			profile->curve_id[i] = 0;
			profile->min_th[i] = 0;
			profile->max_th[i] = 0;
			profile->dp_ratio[i] = 0;
		}
	}

out:
	tm_glob_unlock(TM_ENV(ctl));
	return rc;
}


/**
 */
int tm_set_drop_color_num(tm_handle hndl,
						enum tm_level lvl,
						enum tm_color_num num)
{
	int rc;

	DECLARE_TM_CTL_PTR(ctl, hndl)
	CHECK_TM_CTL_PTR(ctl)

	rc = tm_glob_lock(TM_ENV(ctl));
	if (rc)
		return rc;

	if (lvl > P_LEVEL) {
		rc = -EPERM;
		goto out;
	}

	if (num > TM_3_COLORS) {
		rc = -EFAULT;
		goto out;
	}

	ctl->dp_unit.local[lvl].resolution = 6;

	switch (num) {
	case TM_1_COLORS:
		ctl->dp_unit.local[lvl].color_num = 1;
		break;
	case TM_2_COLORS:
		ctl->dp_unit.local[lvl].color_num = 2;
		break;
	case TM_3_COLORS:
		ctl->dp_unit.local[lvl].color_num = 3;
		ctl->dp_unit.local[lvl].resolution = 4;
		break;
	}

	rc = set_hw_drop_color_num(hndl);
	if (rc)
		rc = TM_HW_COLOR_NUM_CONFIG_FAIL;

out:
	tm_glob_unlock(TM_ENV(ctl));
	return rc;
}


/**
 */
int tm_dp_source_set(tm_handle hndl,
					 enum tm_level lvl,
					 uint8_t color,
					 enum tm_dp_source src)
{
	enum tm_dp_source src_old;
	int rc;
	DECLARE_TM_CTL_PTR(ctl, hndl)
	CHECK_TM_CTL_PTR(ctl)

	rc = tm_glob_lock(TM_ENV(ctl));
	if (rc)
		return rc;

	/* check parameters validity */
	if (lvl > P_LEVEL) {
		rc = -EPERM;
		goto out;
	}

	if (color > 2) {
		rc = -ENODEV;
		goto out;
	}

	if (src > TM_QL) {
		rc = -EFAULT;
		goto out;
	}

	src_old = ctl->dp_unit.local[lvl].dp_src[color];
	if (src_old != src)
	{
		ctl->dp_unit.local[lvl].dp_src[color] = src;
		rc = set_hw_dp_source(ctl);
		if (rc < 0)
			rc = TM_HW_AQM_CONFIG_FAIL;
	}
out:
	tm_glob_unlock(TM_ENV(ctl));
	return rc;
}


/**
 */
int tm_set_drop_query_responce(tm_handle hndl,
							uint8_t port_dp,
							enum tm_level lvl)
{
	int rc;

	DECLARE_TM_CTL_PTR(ctl, hndl)
	CHECK_TM_CTL_PTR(ctl)

	rc = tm_glob_lock(TM_ENV(ctl));
	if (rc)
		return rc;

	/* check parameters validity */
	if (lvl > P_LEVEL) {
		rc = -EPERM;
		goto out;
	}

	if ((port_dp != TM_ENABLE) && (port_dp != TM_DISABLE)) {
		rc = -EFAULT;
		goto out;
	}

	rc = set_hw_dp_local_resp(ctl, port_dp, lvl);
	if (rc)
		rc = TM_HW_DP_QUERY_RESP_CONF_FAILED;
out:
	tm_glob_unlock(TM_ENV(ctl));
	return rc;
}


/**
 */
int tm_set_drop_queue_cos(tm_handle hndl,
						uint32_t index,
						uint8_t cos)
{
	int rc;

	DECLARE_TM_CTL_PTR(ctl, hndl)
	DECLARE_RM_HANDLE(rm, ctl->rm)
	CHECK_TM_CTL_PTR(ctl)

	rc = tm_glob_lock(TM_ENV(ctl));
	if (rc)
		return rc;

	/* check parameters validity */
	if (index >= rm->rm_total_queues) {
		rc = -EFAULT;
		goto out;
	}

	if (cos >= TM_WRED_COS) {
		rc = -EFAULT;
		goto out;
	}

	ctl->tm_q_cos[index] = cos;

	rc = set_hw_queue_cos(ctl, index);
	if (rc)
		rc = TM_HW_QUEUE_COS_CONF_FAILED;
out:
	tm_glob_unlock(TM_ENV(ctl));
	return rc;
}


#define SET_CURVE_DEFAULT       \
	do {                        \
		for (j = 0; j < TM_WRED_CURVE_POINTS; j++)\
			curve->prob[j] = (uint8_t)tm_round_int((uint8_t)63 * (j+1), (uint8_t)TM_WRED_CURVE_POINTS); \
	} while (0);


int _tm_config_default_curves_sw(tm_handle hndl)
{
	struct tm_wred_curve *curve = NULL;
	uint8_t cos;
	int rc = -ENOSPC;
	int j;
	int curve_ind = -1;
	DECLARE_TM_CTL_PTR(ctl, hndl)
	DECLARE_RM_HANDLE(rm, ctl->rm)
	CHECK_TM_CTL_PTR(ctl)

	curve_ind = rm_find_free_wred_queue_curve(rm);
	if (curve_ind < 0)
		return rc;
	curve = &(ctl->tm_wred_q_lvl_curves[curve_ind]);
	SET_CURVE_DEFAULT

	curve_ind = rm_find_free_wred_a_node_curve(rm);
	if (curve_ind < 0)
		return rc;
	curve = &(ctl->tm_wred_a_lvl_curves[curve_ind]);
	SET_CURVE_DEFAULT

	curve_ind = rm_find_free_wred_b_node_curve(rm);
	if (curve_ind < 0)
		return rc;
	curve = &(ctl->tm_wred_b_lvl_curves[curve_ind]);
	SET_CURVE_DEFAULT

	for (cos = 0; cos < TM_WRED_COS; cos++) {
		curve_ind = rm_find_free_wred_c_node_curve(rm, cos);
		if (curve_ind < 0)
			return rc;
		curve = &(ctl->tm_wred_c_lvl_curves[cos][curve_ind]);
		SET_CURVE_DEFAULT
	}

	curve_ind = rm_find_free_wred_port_curve(rm);
	if (curve_ind < 0)
		return rc;
	curve = &(ctl->tm_wred_ports_curves[curve_ind]);
	SET_CURVE_DEFAULT
	for (cos = 0; cos < TM_WRED_COS; cos++) {
		curve_ind = rm_find_free_wred_port_curve_cos(rm, cos);
		if (curve_ind < 0)
			return rc;
		curve = &(ctl->tm_wred_ports_curves_cos[cos][curve_ind]);
		SET_CURVE_DEFAULT
	}
	return 0;
}


int _tm_config_default_curves_hw(tm_handle hndl)
{
	struct tm_wred_curve curve;
	uint8_t cos;
	int j;
	int rc = TM_HW_WRED_CURVE_FAILED;
	DECLARE_TM_CTL_PTR(ctl, hndl)
	CHECK_TM_CTL_PTR(ctl)

	for (j = 0; j < TM_WRED_CURVE_POINTS; j++)
		curve.prob[j] = (uint8_t)tm_round_int((uint8_t)63 * (j+1), (uint8_t)TM_WRED_CURVE_POINTS);

	rc = set_hw_queues_default_wred_curve(ctl, curve.prob);
	if (rc)
		return rc;
	rc = set_hw_a_nodes_default_wred_curve(ctl, curve.prob);
	if (rc)
		return rc;
	rc = set_hw_b_nodes_default_wred_curve(ctl, curve.prob);
	if (rc)
		return rc;

	for (cos = 0; cos < TM_WRED_COS; cos++) {
		rc = set_hw_c_nodes_default_wred_curve(ctl, cos, curve.prob);
		if (rc)
			return rc;
	}

	rc = set_hw_ports_default_wred_curve(ctl, curve.prob);
	for (cos = 0; cos < TM_WRED_COS; cos++) {
		rc = set_hw_ports_default_wred_curve_cos(ctl, cos, curve.prob);
		if (rc)
			return rc;
	}
	return 0;
}


int _tm_config_default_dp_mode(tm_handle hndl, int write_to_hw)
{
	int i;
	int j;
	int rc;
	DECLARE_TM_CTL_PTR(ctl, hndl)
	CHECK_TM_CTL_PTR(ctl)

	rc = 0;
	for (i = Q_LEVEL; i <= P_LEVEL; i++) {
		ctl->dp_unit.local[i].resolution = 6;
		ctl->dp_unit.local[i].color_num = 2;
		for (j = 0; j < 3; j++) {
			ctl->dp_unit.local[i].max_p_mode[j] = TM_MAX_PROB_50/* TM_MAX_PROB_12H */;
			ctl->dp_unit.local[i].dp_src[j] = TM_AQL;
		}
	}
	if (write_to_hw) {
		/* Download to HW */
		/* Only Max Probability Mode should be downloaded, because all
		 * the rest of parameters configured in HW by default */
		rc = set_hw_max_dp_mode(ctl);
		if (rc) rc = TM_HW_WRED_CURVE_FAILED;
	}
	return rc;
}


void __set_default_profile(struct tm_drop_profile *profile)
{
	int j;
	profile->use_list = NULL;
	profile->out_bw = 0;
	profile->cbtd_bw = TM_MAX_BW;
	profile->aql_exp = 0;
	profile->td_thresh_res = TM_ENABLE;
	profile->td_threshold = get_drop_threshold_definition();
	profile->color_td_en = TM_DISABLE;
	for (j = 0; j < 3; j++) {
		profile->curve_id[j].index = 0;
		profile->scale_exp[j].exp = 22;
		profile->scale_ratio[j].ratio = 0;
		profile->min_threshold[j].thresh = 1023;
		profile->dp_ratio[j].ratio = 0;
	}
}


int _tm_config_default_profiles_sw(tm_handle hndl)
{
	struct tm_drop_profile *profile = NULL;
	uint8_t cos;
	int rc = -ENOSPC;
	struct rm_list *list = NULL;
	int prof_ind = -1;

	DECLARE_TM_CTL_PTR(ctl, hndl)
	DECLARE_RM_HANDLE(rm, ctl->rm)
	CHECK_TM_CTL_PTR(ctl)

	prof_ind = rm_find_free_queue_drop_profile(rm);
	if (prof_ind < 0)
		return rc;
	profile = &(ctl->tm_q_lvl_drop_profiles[prof_ind]);
	__set_default_profile(profile);

	prof_ind = rm_find_free_a_node_drop_profile(rm);
	if (prof_ind < 0)
		return rc;
	profile = &(ctl->tm_a_lvl_drop_profiles[prof_ind]);
	__set_default_profile(profile);

	prof_ind = rm_find_free_b_node_drop_profile(rm);
	if (prof_ind < 0)
		return rc;
	profile = &(ctl->tm_b_lvl_drop_profiles[prof_ind]);
	__set_default_profile(profile);

	for (cos = 0; cos < TM_WRED_COS; cos++) {
		prof_ind = rm_find_free_c_node_drop_profile(rm, cos);
		if (prof_ind < 0)
			return rc;
		profile = &(ctl->tm_c_lvl_drop_profiles[cos][prof_ind]);
		__set_default_profile(profile);
	}
	prof_ind = rm_find_free_port_drop_profile(rm);
	if (prof_ind < 0) return rc;
	profile = &(ctl->tm_p_lvl_drop_profiles[prof_ind]);
	__set_default_profile(profile);
	/* for port profiles  list is created ?*/
	if (rm_list_create(rm, &list))
		return rc;
	profile->use_list = list;
	for (cos = 0; cos < TM_WRED_COS; cos++) {
		prof_ind = rm_find_free_port_drop_profile_cos(rm, cos);
		profile = &(ctl->tm_p_lvl_drop_profiles_cos[cos][prof_ind]);
		__set_default_profile(profile);
		if (rm_list_create(rm, &list))
			return rc;
		profile->use_list = list;
	}
	return 0;
}


int _tm_config_default_profiles_hw(tm_handle hndl)
{
	struct tm_drop_profile profile;
	uint8_t cos;
	int rc = TM_HW_WRED_CURVE_FAILED;
	uint32_t portNo;
	DECLARE_TM_CTL_PTR(ctl, hndl)
	CHECK_TM_CTL_PTR(ctl)

	__set_default_profile(&profile);

	if (set_hw_queue_default_drop_profile(ctl, &profile) < 0)
		return rc;
	if (set_hw_a_nodes_default_drop_profile(ctl, &profile) < 0)
		return rc;
	if (set_hw_b_nodes_default_drop_profile(ctl, &profile) < 0)
		return rc;
	for (cos = 0; cos < TM_WRED_COS; cos++) {
		if (set_hw_c_nodes_default_drop_profile(ctl,  &profile, cos) < 0)
			return rc;
	}
	for (portNo = 0; portNo < get_tm_port_count() ; portNo++) {
		if (set_hw_ports_default_drop_profile(ctl,  &profile, portNo) < 0)
			return rc;
		for (cos = 0; cos < TM_WRED_COS; cos++) {
			if (set_hw_ports_default_drop_profile_cos(ctl,  &profile, cos, portNo) < 0)
				return rc;
		}
	}
	return 0;
}


int _tm_config_default_drop_sw(tm_handle hndl)
{
	int rc;

	DECLARE_TM_CTL_PTR(ctl, hndl)
	CHECK_TM_CTL_PTR(ctl)

	rc = tm_glob_lock(TM_ENV(ctl));
	if (rc)
		return rc;

	rc = _tm_config_default_dp_mode(hndl, 0);
	if (rc)
		goto out;

	rc = _tm_config_default_curves_sw(hndl);
	if (rc)
		goto out;

	rc = _tm_config_default_profiles_sw(hndl);
	if (rc)
		goto out;

out:
	tm_glob_unlock(TM_ENV(ctl));
	return rc;
}


int _tm_config_default_drop_hw(tm_handle hndl)
{
	int rc;

	rc = _tm_config_default_dp_mode(hndl, 1);
	if (rc)
		return rc;

	rc = _tm_config_default_curves_hw(hndl);
	if (rc)
		return rc;

	rc = _tm_config_default_profiles_hw(hndl);
	if (rc)
		return rc;
	/* successful */
	return 0;
}


/* Predefined Drop profiles */
int tm_create_drop_profile_cbtd_100Mb(tm_handle hndl,
						enum tm_level level,
						uint8_t cos,
						uint16_t *prof_index)
{
	struct tm_drop_profile_params profile;
	int rc;
	int i;

	DECLARE_TM_CTL_PTR(ctl, hndl)
	CHECK_TM_CTL_PTR(ctl)

	if (level > P_LEVEL)
		return -EPERM;

	profile.cbtd_bw = 100000;
	profile.cbtd_rtt_ratio = 0;
	profile.wred_catd_bw = 0;
	profile.aql_exp = 0;
	profile.wred_catd_mode = DISABLED;
	for (i = 0; i < 3; i++) {
		profile.curve_id[i] = 0;
		profile.dp_ratio[i] = 0;
		profile.min_th[i] = 0;
		profile.max_th[i] = 100;
	}

	rc = tm_create_drop_profile(ctl, level, cos, &profile, prof_index);
	return rc;
}


int tm_create_drop_profile_wred_10Mb(tm_handle hndl,
						enum tm_level level,
						uint8_t cos,
						uint16_t *prof_index)
{
	struct tm_drop_profile_params profile;
	int rc;
	int i;
	uint8_t curve_ind;

	DECLARE_TM_CTL_PTR(ctl, hndl)
	CHECK_TM_CTL_PTR(ctl)

	if (level > P_LEVEL)
		return -EPERM;

	rc = tm_create_wred_traditional_curve(ctl, level, cos, 50, &curve_ind);
	if (rc)
		return -ENOSPC;

	profile.cbtd_bw = TM_MAX_BW;
	profile.cbtd_rtt_ratio = 0;
	profile.wred_catd_bw = 10000; /* 10Mb */
	profile.aql_exp = 0;
	profile.wred_catd_mode = WRED;
	for (i = 0; i < 3; i++) {
		profile.curve_id[i] = curve_ind;
		profile.dp_ratio[i] = 0;
		profile.min_th[i] = 10;
		profile.max_th[i] = 100;
	}

	rc = tm_create_drop_profile(ctl, level, cos, &profile, prof_index);
	return rc;
}


int tm_create_drop_profile_mixed_cbtd_100Mb_wred_10Mb(tm_handle hndl,
						enum tm_level level,
						uint8_t cos,
						uint16_t *prof_index)
{
	struct tm_drop_profile_params profile;
	int rc;
	int i;
	uint8_t curve_ind;

	DECLARE_TM_CTL_PTR(ctl, hndl)
	CHECK_TM_CTL_PTR(ctl)

	if (level > P_LEVEL)
		return -EPERM;

	rc = tm_create_wred_traditional_curve(ctl, level, cos, 50, &curve_ind);
	if (rc)
		return -ENOSPC;

	profile.cbtd_bw = 100000;
	profile.cbtd_rtt_ratio = 100;
	profile.wred_catd_bw = 10000; /* 10Mb */
	profile.aql_exp = 0;
	profile.wred_catd_mode = WRED;
	for (i = 0; i < 3; i++) {
		profile.curve_id[i] = curve_ind;
		profile.dp_ratio[i] = 0;
		profile.min_th[i] = 0;
		profile.max_th[i] = 100;
	}

	rc = tm_create_drop_profile(ctl, level, cos, &profile, prof_index);
	return rc;
}
