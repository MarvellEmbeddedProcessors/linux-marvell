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

#include "tm_nodes_update.h"
#include "tm_errcodes.h"
#include "tm_locking_interface.h"
#include "rm_internal_types.h"
#include "rm_status.h"
#include "rm_list.h"
#include "tm_errcodes.h"
#include "tm_nodes_utils.h"
#include "set_hw_registers.h"
#include "tm_hw_configuration_interface.h"
#include "tm_elig_prio_func.h"


/***************************************************************************
 * Parameters Update
 ***************************************************************************/

/**
 */
int tm_update_queue(tm_handle hndl, uint32_t index,
					struct tm_queue_params *params)
{
	uint8_t status;
	int rc;
	uint8_t fl_change = TM_DISABLE;
	struct tm_queue *queue = NULL;
	uint8_t shaping = TM_ENABLE;

	DECLARE_TM_CTL_PTR(ctl, hndl)
	DECLARE_RM_HANDLE(rm, ctl->rm)
	CHECK_TM_CTL_PTR(ctl);

	rc = tm_nodes_lock(TM_ENV(ctl));
	if (rc)
		return rc;

	/* Check parameters validity */
	if (index >= rm->rm_total_queues) {
		rc = -EFAULT;
		goto out;
	}

	rc = rm_node_status(rm, Q_LEVEL, index, &status);
	if ((rc) || (status != RM_TRUE)) {
		rc = -ENODATA;
		goto out;
	}

	queue = &(ctl->tm_queue_array[index]);

	if (params->elig_prio_func_ptr != (uint8_t)TM_INVAL) {

		switch (is_queue_elig_fun_uses_shaper(ctl->tm_elig_prio_q_lvl_tbl, params->elig_prio_func_ptr)) {
		case -1:
			rc = TM_CONF_ELIG_PRIO_FUNC_ID_OOR;
			goto out;
			break;
		case 0:
			shaping = TM_DISABLE;
			break;
		default:
			shaping = TM_ENABLE;
			break;
		}
		if (shaping == TM_ENABLE) {
			/* TBD: check if shaping enabled on this level */
			/* if (ctl->level_data[level].shaping_status == TM_DISABLE) {
				rc = TM_CONF_ELIG_PRIO_FUNC_ID_OOR;
				goto out;
			} */
		}

		queue->elig_prio_func_ptr = params->elig_prio_func_ptr;
		fl_change = TM_ENABLE;
	}

	if (params->quantum != (uint16_t)TM_INVAL) {
#ifdef MV_QMTM_NOT_NSS
		/* Check param */
		if ((params->quantum < ctl->min_quantum)
			|| (params->quantum > 256 * ctl->min_quantum)) {
			rc = TM_CONF_Q_QUANTUM_OOR;
			goto out;
		}
#endif
		/* Update SW DB */
		/* Update quantum even if DWRR is disabled */
		queue->dwrr_quantum = params->quantum;
		fl_change = TM_ENABLE;
	}

	if ((params->wred_profile_ref != queue->wred_profile_ref) &&
		(params->wred_profile_ref != (uint8_t)TM_INVAL)) {
		/* check if queue drop profile already exists */
		rc = rm_queue_drop_profile_status(rm, params->wred_profile_ref, &status);
		if ((rc) || (status != RM_TRUE))
		{
			rc = TM_CONF_Q_WRED_PROF_REF_OOR;
			goto out;
		}

		/* Update SW DB */
		ctl->tm_q_lvl_drop_profiles[queue->wred_profile_ref].use_counter--;
		/* assert(ctl->tm_q_lvl_drop_profiles[queue->wred_profile_ref].use_counter >= 0); */
		queue->wred_profile_ref = params->wred_profile_ref;
		ctl->tm_q_lvl_drop_prof_ptr[index] = params->wred_profile_ref;
		ctl->tm_q_lvl_drop_profiles[queue->wred_profile_ref].use_counter++;
		fl_change = TM_ENABLE;
	}

	/* Download to HW */
	if (fl_change == TM_ENABLE) {
		rc = set_hw_queue(ctl, index);
		if (rc)
			rc = TM_HW_QUEUE_CONFIG_FAIL;
	}

out:
	tm_nodes_unlock(TM_ENV(ctl));
	return rc;
}


/**
 */
int tm_update_a_node(tm_handle hndl, uint32_t index,
					 struct tm_a_node_params *params)
{
	uint8_t status;
	uint8_t dwrr_priority_t;
	int rc;
	int i;
	/*uint8_t fl_change = TM_DISABLE;*/
	struct tm_a_node *node = NULL;
	uint8_t shaping = TM_ENABLE;

	DECLARE_TM_CTL_PTR(ctl, hndl)
	DECLARE_RM_HANDLE(rm, ctl->rm)
	CHECK_TM_CTL_PTR(ctl);

	rc = tm_nodes_lock(TM_ENV(ctl));
	if (rc)
		return rc;

	/* Check parameters validity */
	if (index >= rm->rm_total_a_nodes) {
		rc = -EFAULT;
		goto out;
	}

	rc = rm_node_status(rm, A_LEVEL, index, &status);
	if ((rc) || (status != RM_TRUE)) {
		rc = -ENODATA;
		goto out;
	}

	node = &(ctl->tm_a_node_array[index]);

	if (params->elig_prio_func_ptr != (uint8_t) TM_INVAL) {
		switch (is_node_elig_fun_uses_shaper(ctl->tm_elig_prio_a_lvl_tbl, params->elig_prio_func_ptr)) {
		case -1:
			rc = TM_CONF_ELIG_PRIO_FUNC_ID_OOR;
			goto out;
			break;
		case 0:
			shaping = TM_DISABLE;
			break;
		default:
			shaping = TM_ENABLE;
			break;
		}
		if (shaping == TM_ENABLE) {
			/* TBD: check if shaping enabled on this level */
			/* if (ctl->level_data[level].shaping_status == TM_DISABLE) {
				rc = TM_CONF_ELIG_PRIO_FUNC_ID_OOR;
				goto out;
			} */
		}

		node->elig_prio_func_ptr = params->elig_prio_func_ptr;
		/*fl_change = TM_ENABLE;*/
	}

	/* DWRR for A-node */
	if (params->quantum != (uint16_t)TM_INVAL) {
#ifdef MV_QMTM_NOT_NSS
		if ((params->quantum < ctl->min_quantum)
			|| (params->quantum > 256 * ctl->min_quantum)) {
			rc = TM_CONF_A_QUANTUM_OOR;
			goto out;
		}
#endif
		/* Update SW DB */
		node->dwrr_quantum = params->quantum;
		/*fl_change = TM_ENABLE;*/
	}

	/* DWRR for Queues in A-node's range */
	if ((params->dwrr_priority[0] != (uint8_t)TM_INVAL) ||
		(params->dwrr_priority[1] != (uint8_t)TM_INVAL) ||
		(params->dwrr_priority[2] != (uint8_t)TM_INVAL) ||
		(params->dwrr_priority[3] != (uint8_t)TM_INVAL) ||
		(params->dwrr_priority[4] != (uint8_t)TM_INVAL) ||
		(params->dwrr_priority[5] != (uint8_t)TM_INVAL) ||
		(params->dwrr_priority[6] != (uint8_t)TM_INVAL) ||
		(params->dwrr_priority[7] != (uint8_t)TM_INVAL)) {
		/* Check params */
		dwrr_priority_t = 0;
		for (i = 0; i < 8; i++) {
			if (params->dwrr_priority[i] != (uint8_t)TM_INVAL) {
				if ((params->dwrr_priority[i] != TM_DISABLE) &&
					(params->dwrr_priority[i] != TM_ENABLE)) {
					rc = TM_CONF_A_DWRR_PRIO_OOR;
					goto out;
				}
				dwrr_priority_t =
					dwrr_priority_t | (params->dwrr_priority[i] << i);
			} else
				dwrr_priority_t = (dwrr_priority_t & ~(0x1 << i)) |
					(node->dwrr_priority & (0x1 << i));
		}

		/* Update SW DB */
		node->dwrr_priority = dwrr_priority_t;
		/*fl_change = TM_ENABLE;*/
	}

	if ((params->wred_profile_ref != node->wred_profile_ref) &&
		(params->wred_profile_ref != (uint8_t)TM_INVAL)) {
		/* check if a node drop profile already exists */
		rc = rm_a_node_drop_profile_status(rm, params->wred_profile_ref, &status);
		if ((rc) || (status != RM_TRUE)) {
			rc = TM_CONF_A_WRED_PROF_REF_OOR;
			goto out;
		}

		/* Update SW DB */
		ctl->tm_a_lvl_drop_profiles[node->wred_profile_ref].use_counter--;
		/* assert(ctl->tm_a_lvl_drop_profiles[node->wred_profile_ref].use_counter >= 0); */
		node->wred_profile_ref = params->wred_profile_ref;
		ctl->tm_a_lvl_drop_prof_ptr[index] = params->wred_profile_ref;
		ctl->tm_a_lvl_drop_profiles[node->wred_profile_ref].use_counter++;
		/*fl_change = TM_ENABLE;*/
	}

	/* Download to HW */
	rc = set_hw_a_node(ctl, index);
	if (rc)
		rc = TM_HW_A_NODE_CONFIG_FAIL;

out:
	tm_nodes_unlock(TM_ENV(ctl));
	return rc;
}


/**
 */
int tm_update_b_node(tm_handle hndl, uint32_t index,
					 struct tm_b_node_params *params)
{
	uint8_t status;
	uint8_t dwrr_priority_t;
	int rc;
	int i;
	/*uint8_t fl_change = TM_DISABLE;*/
	struct tm_b_node *node = NULL;
	uint8_t shaping = TM_ENABLE;

	DECLARE_TM_CTL_PTR(ctl, hndl)
	DECLARE_RM_HANDLE(rm, ctl->rm)
	CHECK_TM_CTL_PTR(ctl);

	rc = tm_nodes_lock(TM_ENV(ctl));
	if (rc)
		return rc;

	/* Check parameters validity */
	if (index >= rm->rm_total_b_nodes) {
		rc = -EFAULT;
		goto out;
	}

	rc = rm_node_status(rm, B_LEVEL, index, &status);
	if ((rc) || (status != RM_TRUE)) {
		rc = -ENODATA;
		goto out;
	}

	node = &(ctl->tm_b_node_array[index]);

	if (params->elig_prio_func_ptr != (uint8_t)TM_INVAL) {
		switch (is_node_elig_fun_uses_shaper(ctl->tm_elig_prio_b_lvl_tbl, params->elig_prio_func_ptr)) {
		case -1:
			rc = TM_CONF_ELIG_PRIO_FUNC_ID_OOR;
			goto out;
			break;
		case 0:
			shaping = TM_DISABLE;
			break;
		default:
			shaping = TM_ENABLE;
			break;
		}

		if (shaping == TM_ENABLE) {
			/* TBD: check if shaping enabled on this level */
			/* if (ctl->level_data[level].shaping_status == TM_DISABLE) {
				rc = TM_CONF_ELIG_PRIO_FUNC_ID_OOR;
				goto out;
			} */
		}

		node->elig_prio_func_ptr = params->elig_prio_func_ptr;
		/*fl_change = TM_ENABLE;*/
	}


	/* DWRR for B-node */
	if (params->quantum != (uint16_t)TM_INVAL) {
#ifdef MV_QMTM_NOT_NSS
		/* Check params */
		if ((params->quantum < ctl->min_quantum)
			|| (params->quantum > 256 * ctl->min_quantum)) {
			rc = TM_CONF_B_QUANTUM_OOR;
			goto out;
		}
#endif
		/* Update SW DB */
		node->dwrr_quantum = params->quantum;
		/*fl_change = TM_ENABLE;*/
	}

	/* DWRR for A-nodes in B-node's range */
	if ((params->dwrr_priority[0] != (uint8_t)TM_INVAL) ||
		(params->dwrr_priority[1] != (uint8_t)TM_INVAL) ||
		(params->dwrr_priority[2] != (uint8_t)TM_INVAL) ||
		(params->dwrr_priority[3] != (uint8_t)TM_INVAL) ||
		(params->dwrr_priority[4] != (uint8_t)TM_INVAL) ||
		(params->dwrr_priority[5] != (uint8_t)TM_INVAL) ||
		(params->dwrr_priority[6] != (uint8_t)TM_INVAL) ||
		(params->dwrr_priority[7] != (uint8_t)TM_INVAL)) {
		/* Check params */
		dwrr_priority_t = 0;
		for (i = 0; i < 8; i++) {
			if (params->dwrr_priority[i] != (uint8_t)TM_INVAL) {
				if ((params->dwrr_priority[i] != TM_DISABLE) &&
					(params->dwrr_priority[i] != TM_ENABLE)) {
					rc = TM_CONF_B_DWRR_PRIO_OOR;
					goto out;
				}
				dwrr_priority_t =
					dwrr_priority_t | (params->dwrr_priority[i] << i);
			} else
				dwrr_priority_t = (dwrr_priority_t & ~(0x1 << i)) |
					(node->dwrr_priority & (0x1 << i));
		}

		/* Update SW DB */
		node->dwrr_priority = dwrr_priority_t;
		/*fl_change = TM_ENABLE;*/
	}

	if ((params->wred_profile_ref != (uint8_t)TM_INVAL) &&
		(params->wred_profile_ref != (uint8_t)TM_INVAL)) {
		/* Check param */
		if (params->wred_profile_ref >= TM_NUM_B_NODE_DROP_PROF)
		{
			rc = TM_CONF_B_WRED_PROF_REF_OOR;
			goto out;
		}

		/* check if b node drop profile already exists */
		rc = rm_b_node_drop_profile_status(rm, params->wred_profile_ref, &status);
		if ((rc) || (status != RM_TRUE))
		{
			rc = TM_CONF_B_WRED_PROF_REF_OOR;
			goto out;
		}

		/* Update SW DB */
		ctl->tm_b_lvl_drop_profiles[node->wred_profile_ref].use_counter--;
		/* assert(ctl->tm_b_lvl_drop_profiles[node->wred_profile_ref].use_counter >= 0); */
		node->wred_profile_ref = params->wred_profile_ref;
		ctl->tm_b_lvl_drop_prof_ptr[index] = params->wred_profile_ref;
		ctl->tm_b_lvl_drop_profiles[node->wred_profile_ref].use_counter++;
		/*fl_change = TM_ENABLE;*/
	}

	/* Download to HW */
	rc = set_hw_b_node(ctl, index);
	if (rc)
		rc = TM_HW_B_NODE_CONFIG_FAIL;

out:
	tm_nodes_unlock(TM_ENV(ctl));
	return rc;
}


/**
 */
int tm_update_c_node(tm_handle hndl, uint32_t index,
					 struct tm_c_node_params *params)
{
	uint8_t status;
	uint8_t dwrr_priority_t;
	uint8_t cos_map;
	uint8_t old_wred_profile_ref;
	int rc = 0;
	int i;
	/*uint8_t fl_change = TM_DISABLE;*/
	struct tm_c_node *node = NULL;
	uint8_t shaping = TM_ENABLE;

	DECLARE_TM_CTL_PTR(ctl, hndl)
	DECLARE_RM_HANDLE(rm, ctl->rm)
	CHECK_TM_CTL_PTR(ctl);

	rc = tm_nodes_lock(TM_ENV(ctl));
	if (rc)
		return rc;

	/* Check parameters validity */
	if (index >= rm->rm_total_c_nodes) {
		rc = -EFAULT;
		goto out;
	}

	rc = rm_node_status(rm, C_LEVEL, index, &status);
	if ((rc) || (status != RM_TRUE)) {
		rc = -ENODATA;
		goto out;
	}

	node = &(ctl->tm_c_node_array[index]);

	if (params->elig_prio_func_ptr != (uint8_t)TM_INVAL) {
		switch (is_node_elig_fun_uses_shaper(ctl->tm_elig_prio_c_lvl_tbl, params->elig_prio_func_ptr)) {
		case -1:
			rc = TM_CONF_ELIG_PRIO_FUNC_ID_OOR;
			goto out;
			break;
		case 0:
			shaping = TM_DISABLE;
			break;
		default:
			shaping = TM_ENABLE;
			break;
		}

		if (shaping == TM_ENABLE) {
			/* TBD: check if shaping enabled on this level */
			/* if (ctl->level_data[level].shaping_status == TM_DISABLE) {
				rc = TM_CONF_ELIG_PRIO_FUNC_ID_OOR;
				goto out;
			} */
		}

		node->elig_prio_func_ptr = params->elig_prio_func_ptr;
		/*fl_change = TM_ENABLE;*/
	}


	/* DWRR for C-node */
	if (params->quantum != (uint16_t)TM_INVAL) {
#ifdef MV_QMTM_NOT_NSS
		/* Check params */
		if ((params->quantum < ctl->min_quantum)
			|| (params->quantum > 256 * ctl->min_quantum)) {
			rc = TM_CONF_C_QUANTUM_OOR;
			goto out;
		}
#endif
		/* Update SW DB */
		node->dwrr_quantum = params->quantum;
		/*fl_change = TM_ENABLE;*/
	}

	/* DWRR for B-nodes in C-node's range */
	if ((params->dwrr_priority[0] != (uint8_t)TM_INVAL) ||
		(params->dwrr_priority[1] != (uint8_t)TM_INVAL) ||
		(params->dwrr_priority[2] != (uint8_t)TM_INVAL) ||
		(params->dwrr_priority[3] != (uint8_t)TM_INVAL) ||
		(params->dwrr_priority[4] != (uint8_t)TM_INVAL) ||
		(params->dwrr_priority[5] != (uint8_t)TM_INVAL) ||
		(params->dwrr_priority[6] != (uint8_t)TM_INVAL) ||
		(params->dwrr_priority[7] != (uint8_t)TM_INVAL)) {
		/* Check params */
		dwrr_priority_t = 0;
		for (i = 0; i < 8; i++) {
			if (params->dwrr_priority[i] != (uint8_t)TM_INVAL) {
				if ((params->dwrr_priority[i] != TM_DISABLE) &&
					(params->dwrr_priority[i] != TM_ENABLE)) {
					rc = TM_CONF_C_DWRR_PRIO_OOR;
					goto out;
				}
				dwrr_priority_t =
					dwrr_priority_t | (params->dwrr_priority[i] << i);
			} else
				dwrr_priority_t = (dwrr_priority_t & ~(0x1 << i)) |
					(node->dwrr_priority & (0x1 << i));
		}

		/* Update SW DB */
		node->dwrr_priority = dwrr_priority_t;
		/*fl_change = TM_ENABLE;*/
	}

	cos_map = node->wred_cos;
	for (i = 0; i < TM_WRED_COS; i++) {
		if (params->wred_profile_ref[i] != (uint8_t)TM_INVAL) {
			if (params->wred_cos & (1<<i)) {
				/* apply new drop profile for this cos */
				/* Check param */
				if (params->wred_profile_ref[i] >= TM_NUM_C_NODE_DROP_PROF) {
					rc = TM_CONF_C_WRED_PROF_REF_OOR;
					goto out;
				}

				/* check if c node drop profile already exists */
				rc = rm_c_node_drop_profile_status(rm, (uint8_t)i,
					params->wred_profile_ref[i], &status);
				if ((rc) || (status != RM_TRUE)) {
					rc = TM_CONF_C_WRED_PROF_REF_OOR;
					goto out;
				}

				if (params->wred_profile_ref[i] != node->wred_profile_ref[i]) {
					/* Update SW DB */
					old_wred_profile_ref = ctl->tm_c_lvl_drop_prof_ptr[i][index];
					ctl->tm_c_lvl_drop_profiles[i][old_wred_profile_ref].use_counter--;
					ctl->tm_c_lvl_drop_prof_ptr[i][index] = params->wred_profile_ref[i];
					ctl->tm_c_lvl_drop_profiles[i][params->wred_profile_ref[i]].use_counter++;
					node->wred_profile_ref[i] = params->wred_profile_ref[i];

					/* update Wred CoS for future usage within the HW update */
					cos_map =
						(cos_map & ~(0x1<<i)) | (params->wred_cos & (0x1<<i));
					/*fl_change = TM_ENABLE;*/
				} else {
					/* set wred_cos bit is the same */
					cos_map =
						(cos_map & ~(0x1<<i)) | (params->wred_cos & (0x1<<i));
				}
			} else { /* remove the use in old drop profile */
				if (node->wred_cos & (0x1<<i)) {
					/* the profile was in use */
					/* Update SW DB */
					old_wred_profile_ref = ctl->tm_c_lvl_drop_prof_ptr[i][index];
					ctl->tm_c_lvl_drop_profiles[i][old_wred_profile_ref].use_counter--;
					/* assert(ctl->tm_c_lvl_drop_profiles[i][old_wred_profile_ref].use_counter >= 0); */
					ctl->tm_c_lvl_drop_prof_ptr[i][index] = TM_NO_DROP_PROFILE;
					ctl->tm_c_lvl_drop_profiles[i]
					[params->wred_profile_ref[TM_NO_DROP_PROFILE]].use_counter++;
					node->wred_profile_ref[i] = TM_NO_DROP_PROFILE;

					/* update Wred CoS for future usage within the HW update */
					cos_map = (cos_map & ~(0x1 << i));
					/*fl_change = TM_ENABLE;*/
				}
			}
		}
	}
	node->wred_cos = cos_map;

	rc = set_hw_c_node(ctl, index);
	if (rc)
		rc = TM_HW_C_NODE_CONFIG_FAIL;

out:
	tm_nodes_unlock(TM_ENV(ctl));
	return rc;
}


/**
 */
int tm_update_port_scheduling(tm_handle hndl,
				uint8_t index,
				uint8_t elig_prio_func_ptr,
#ifdef MV_QMTM_NSS_A0
				uint16_t *quantum, /* 8 cells array */
#endif
				uint8_t *dwrr_priority) /* 8 cells array */
{
	struct tm_port *port = NULL;
	uint8_t status;
#ifdef MV_QMTM_NSS_A0
	uint32_t min_port_quant;
	uint32_t max_pkg_len_bursts;
#endif
	uint8_t dwrr_priority_t;
	int rc = 0;
	int i;
	uint8_t shaping = TM_ENABLE;

	DECLARE_TM_CTL_PTR(ctl, hndl)
	DECLARE_RM_HANDLE(rm, ctl->rm)
	CHECK_TM_CTL_PTR(ctl);

	rc = tm_nodes_lock(TM_ENV(ctl));
	if (rc)
		return rc;
	rc = tm_sched_lock(TM_ENV(ctl));
	if (rc) {
		tm_nodes_unlock(TM_ENV(ctl));
		return rc;
	}

	if (index >= rm->rm_total_ports) {
		rc = -EFAULT;
		goto out;
	}

	rc = rm_node_status(rm, P_LEVEL, index, &status);
	if ((rc) || (status != RM_TRUE)) {
		rc = -ENODATA;
		goto out;
	}

	port = &(ctl->tm_port_array[index]);

	if (elig_prio_func_ptr != (uint8_t)TM_INVAL) {
		switch (is_node_elig_fun_uses_shaper(ctl->tm_elig_prio_p_lvl_tbl, elig_prio_func_ptr)) {
		case -1:
			rc = TM_CONF_ELIG_PRIO_FUNC_ID_OOR;
			goto out;
			break;
		case 0:
			shaping = TM_DISABLE;
			break;
		default:
			shaping = TM_ENABLE;
			break;
		}

		if (shaping == TM_ENABLE) {
			/* TBD: check if shaping enabled on this level */
			/* if (ctl->level_data[level].shaping_status == TM_DISABLE) {
				rc = TM_CONF_ELIG_PRIO_FUNC_ID_OOR;
				goto out;
			} */
		}
		port->elig_prio_func_ptr = elig_prio_func_ptr;
	}

#ifdef MV_QMTM_NSS_A0
	/* DWRR for Port */
	if ((quantum[0] != (uint16_t)TM_INVAL) ||
		(quantum[1] != (uint16_t)TM_INVAL) ||
		(quantum[2] != (uint16_t)TM_INVAL) ||
		(quantum[3] != (uint16_t)TM_INVAL) ||
		(quantum[4] != (uint16_t)TM_INVAL) ||
		(quantum[5] != (uint16_t)TM_INVAL) ||
		(quantum[6] != (uint16_t)TM_INVAL) ||
		(quantum[7] != (uint16_t)TM_INVAL))
	{
		/* Check quantum */
		min_port_quant = 4*ctl->port_ch_emit*ctl->dwrr_bytes_burst_limit;
		/* 4*PortChunksEmitPerSel*PortDWRRBytesPerBurstsLimit */
		max_pkg_len_bursts = (ctl->mtu + ctl->min_pkg_size)/16;
		if (min_port_quant < max_pkg_len_bursts)
			min_port_quant = max_pkg_len_bursts;
		min_port_quant = min_port_quant/0x40; /* in 64B units */
		for (i = 0; i < 8; i++)
		{
			if (quantum[i] != (uint16_t)TM_INVAL)
			{
				if (quantum[i] > 0x200)
				{ /* maximum 9 bits */
					rc = TM_CONF_PORT_QUANTUM_OOR;
					goto out;
				}
				if (quantum[i] < min_port_quant)
				{
					rc = TM_CONF_PORT_QUANTUM_OOR;
					goto out;
				} else {
					/* Update SW DB */
					port->dwrr_quantum[i].quantum = quantum[i];
				}
			}
		}
	}
#endif

	/* DWRR for C-nodes in Port's range */
	dwrr_priority_t = 0;
	if ((dwrr_priority[0] != (uint8_t)TM_INVAL) ||
		(dwrr_priority[1] != (uint8_t)TM_INVAL) ||
		(dwrr_priority[2] != (uint8_t)TM_INVAL) ||
		(dwrr_priority[3] != (uint8_t)TM_INVAL) ||
		(dwrr_priority[4] != (uint8_t)TM_INVAL) ||
		(dwrr_priority[5] != (uint8_t)TM_INVAL) ||
		(dwrr_priority[6] != (uint8_t)TM_INVAL) ||
		(dwrr_priority[7] != (uint8_t)TM_INVAL)) {
		/* Check params and update SW DB */
		for (i = 0; i < 8; i++) {
			if (dwrr_priority[i] != (uint8_t)TM_INVAL) {
				if ((dwrr_priority[i] != TM_DISABLE) &&
					(dwrr_priority[i] != TM_ENABLE)) {
					rc = TM_CONF_PORT_DWRR_PRIO_OOR;
					goto out;
				}
				dwrr_priority_t = dwrr_priority_t | (dwrr_priority[i] << i);
			} else
				dwrr_priority_t = (dwrr_priority_t & ~(0x1 << i)) |
					(port->dwrr_priority & (0x1 << i));
		}
	}

	/* Update SW DB */
	port->dwrr_priority = dwrr_priority_t;
	rc = set_hw_port_scheduling(hndl, index);
	if (rc) {
		rc = TM_HW_PORT_CONFIG_FAIL;
		goto out;
	}

	/* Download Queue eligible function pointer (from profile) to HW */
	rc = set_hw_deq_status(hndl, P_LEVEL, index);
	if (rc)
		rc = TM_HW_ELIG_PRIO_FUNC_FAILED;

out:
	tm_sched_unlock(TM_ENV(ctl));
	tm_nodes_unlock(TM_ENV(ctl));
	return rc;
}

/**
 */
int tm_update_port_drop(tm_handle hndl, uint8_t index, uint8_t wred_profile_ref)
{
	struct tm_port *port = NULL;
	struct tm_drop_profile *profile = NULL;
	uint8_t status;
	int rc = 0;

	DECLARE_TM_CTL_PTR(ctl, hndl)
	DECLARE_RM_HANDLE(rm, ctl->rm)
	CHECK_TM_CTL_PTR(ctl);

	rc = tm_nodes_lock(TM_ENV(ctl));
	if (rc)
		return rc;

	if (index >= rm->rm_total_ports) {
		rc = -EFAULT;
		goto out;
	}

	rc = rm_node_status(rm, P_LEVEL, index, &status);
	if ((rc) || (status != RM_TRUE)) {
		rc = -ENODATA;
		goto out;
	}

	port = &(ctl->tm_port_array[index]);

	if ((wred_profile_ref != (uint8_t)TM_INVAL) &&
		(wred_profile_ref != port->wred_profile_ref)) {
		/* Check param */
		if (wred_profile_ref >= TM_NUM_PORT_DROP_PROF) {
			rc = TM_CONF_P_WRED_PROF_REF_OOR;
			goto out;
		}

		/* check if port drop profile already exists */
		rc = rm_port_drop_profile_status(rm, wred_profile_ref, &status);
		if ((rc) || (status != RM_TRUE)) {
			rc = TM_CONF_P_WRED_PROF_REF_OOR;
			goto out;
		}

		/* Update SW DB */
		profile = &(ctl->tm_p_lvl_drop_profiles[port->wred_profile_ref]);
		profile->use_counter--;
		/* assert(profile->use_counter >= 0); */

		/* remove node from list of old drop profile */
		rc = rm_list_del_index(rm, profile->use_list, index, P_LEVEL);
		if (rc) {
			rc = TM_CONF_P_WRED_PROF_REF_OOR;
			goto out;
		}
		port->wred_profile_ref = wred_profile_ref;
		/* add node to the list of new drop profile */
		profile = &(ctl->tm_p_lvl_drop_profiles[port->wred_profile_ref]);
		rc = rm_list_add_index(rm, profile->use_list, index, P_LEVEL);
		if (rc) {
			rc = TM_CONF_P_WRED_PROF_REF_OOR;
			goto out;
		}
		ctl->tm_p_lvl_drop_prof_ptr[index] = wred_profile_ref;
		profile->use_counter++;

		/* Download to HW the values of new drop profile to the port */
		rc = set_hw_port_drop(hndl, index);
		if (rc)
			rc = TM_HW_PORT_CONFIG_FAIL;
	}
out:
	tm_nodes_unlock(TM_ENV(ctl));
	return rc;
}


/**
 */
int tm_update_port_drop_cos(tm_handle hndl,
							uint8_t index,
							struct tm_port_drop_per_cos *params)
{
	struct tm_port *port = NULL;
	struct tm_drop_profile *profile = NULL;
	uint8_t status;
	uint8_t new_cos_status;
	uint8_t old_cos_status;
	int rc = 0;
	uint8_t i;

	DECLARE_TM_CTL_PTR(ctl, hndl)
	DECLARE_RM_HANDLE(rm, ctl->rm)
	CHECK_TM_CTL_PTR(ctl);

	rc = tm_nodes_lock(TM_ENV(ctl));
	if (rc)
		return rc;

	if (index >= rm->rm_total_ports) {
		rc = -EFAULT;
		goto out;
	}

	rc = rm_node_status(rm, P_LEVEL, index, &status);
	if ((rc) || (status != RM_TRUE)) {
		rc = -ENODATA;
		goto out;
	}

	port = &(ctl->tm_port_array[index]);

	for (i = 0; i < TM_WRED_COS; i++) {
		if (params->wred_profile_ref[i] != (uint8_t)TM_INVAL) {
			new_cos_status = params->wred_cos & (1<<i);
			old_cos_status = port->wred_cos & (1<<i);

			if ((old_cos_status == new_cos_status) && (old_cos_status >= 1)
				&& (params->wred_profile_ref[i] == port->wred_profile_ref_cos[i]))
				goto out;

			/* Old Drop Profile reference should be replaced by new */
			if ((old_cos_status == new_cos_status) && (old_cos_status >= 1)) {
				/* Check new Drop Profile reference */
				if (params->wred_profile_ref[i] >= TM_NUM_PORT_DROP_PROF) {
					rc = TM_CONF_P_WRED_PROF_REF_OOR;
					goto out;
				}

				/* Check if new Drop Profile already exists */
				rc = rm_port_drop_profile_status_cos(rm, i, params->wred_profile_ref[i], &status);
				if ((rc) || (status != RM_TRUE)) {
					rc = TM_CONF_P_WRED_PROF_REF_OOR;
					goto out;
				}

				/* Remove old Drop Profile reference */
				profile = &(ctl->tm_p_lvl_drop_profiles_cos[i][port->wred_profile_ref_cos[i]]);
				profile->use_counter--;
				/* assert(profile->use_counter >= 0); */

				/* remove node from list of old drop profile */
				rc = rm_list_del_index(rm, profile->use_list, index, P_LEVEL);
				if (rc) {
					rc = TM_CONF_P_WRED_PROF_REF_OOR;
					goto out;
				}

				/* Add new Drop Profile reference */
				port->wred_profile_ref_cos[i] = params->wred_profile_ref[i];

				/* add node to the list of new drop profile */
				profile = &(ctl->tm_p_lvl_drop_profiles_cos[i][port->wred_profile_ref_cos[i]]);
				rc = rm_list_add_index(rm, profile->use_list, index, P_LEVEL);
				if (rc) {
					rc = TM_CONF_P_WRED_PROF_REF_OOR;
					goto out;
				}
				ctl->tm_p_lvl_drop_prof_ptr_cos[i][index] = params->wred_profile_ref[i];
				profile->use_counter++;

				/* Download to HW the values of new drop profile to the port */
				rc = set_hw_port_drop_cos(hndl, index, i);
				if (rc)
					rc = TM_HW_PORT_CONFIG_FAIL;
			} else {
				/* Add new Drop Profile reference (wred_cos bit 0->1) */
				if (old_cos_status == 0) {
					/* Check new Drop Profile reference */
					if (params->wred_profile_ref[i] >= TM_NUM_PORT_DROP_PROF) {
						rc = TM_CONF_P_WRED_PROF_REF_OOR;
						goto out;
					}

					/* Check if new Drop Profile already exists */
					rc = rm_port_drop_profile_status_cos(rm, i, params->wred_profile_ref[i],
						&status);
					if ((rc) || (status != RM_TRUE)) {
						rc = TM_CONF_P_WRED_PROF_REF_OOR;
						goto out;
					}

					/* Add new Drop Profile reference */
					port->wred_profile_ref_cos[i] = params->wred_profile_ref[i];

					/* add node to the list of new drop profile */
					profile = &(ctl->tm_p_lvl_drop_profiles_cos[i][port->wred_profile_ref_cos[i]]);
					rc = rm_list_add_index(rm, profile->use_list, index, P_LEVEL);
					if (rc) {
						rc = TM_CONF_P_WRED_PROF_REF_OOR;
						goto out;
					}
					ctl->tm_p_lvl_drop_prof_ptr_cos[i][index] = params->wred_profile_ref[i];
					profile->use_counter++;

					port->wred_cos |= (1<<i);

					/* Download to HW the values of new drop profile to the port */
					rc = set_hw_port_drop_cos(hndl, index, i);
					if (rc)
						rc = TM_HW_PORT_CONFIG_FAIL;
				}
				/* Remove old Drop Profile reference (wred_cos bit 1->0) */
				else {
					profile = &(ctl->tm_p_lvl_drop_profiles_cos[i][port->wred_profile_ref_cos[i]]);
					profile->use_counter--;
					/* assert(profile->use_counter >= 0); */

					/* remove node from list of old drop profile */
					rc = rm_list_del_index(rm, profile->use_list, index, P_LEVEL);
					if (rc) {
						rc = TM_CONF_P_WRED_PROF_REF_OOR;
						goto out;
					}

					/* set default values */
					port->wred_profile_ref_cos[i] = TM_NO_DROP_PROFILE;

					/* add node to the list of default drop profile */
					profile = &(ctl->tm_p_lvl_drop_profiles_cos[i][TM_NO_DROP_PROFILE]);
					rc = rm_list_add_index(rm, profile->use_list, index, P_LEVEL);
					if (rc) {
						rc = TM_CONF_P_WRED_PROF_REF_OOR;
						goto out;
					}
					ctl->tm_p_lvl_drop_prof_ptr_cos[i][index] = TM_NO_DROP_PROFILE;
					profile->use_counter++;

					port->wred_cos &= ~(1<<i);

					/* Download to HW the default values to the port */
					rc = set_hw_port_drop_cos(hndl, index, i);
					if (rc)
						rc = TM_HW_PORT_CONFIG_FAIL;
				}
			}
		}
	}
out:
	tm_nodes_unlock(TM_ENV(ctl));
	return rc;
}
