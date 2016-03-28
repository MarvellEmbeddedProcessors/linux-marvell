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

#include "common/mv_sw_if.h"

#include "rm_status.h"
#include "rm_free.h"
#include "rm_list.h"
#include "tm_locking_interface.h"
#include "tm_core_types.h"
#include "tm_errcodes.h"

#include "tm_set_local_db_defaults.h"
#include "set_hw_registers.h"
#include "tm_os_interface.h"


/**
 */
static int tm_delete_port(tm_handle hndl, uint8_t index)
{
	struct tm_port *port = NULL;
	struct tm_drop_profile *profile = NULL;
	uint16_t range;
	int rc;
	int i;

	DECLARE_TM_CTL_PTR(ctl, hndl)
	CHECK_TM_CTL_PTR(ctl)

	port = &(ctl->tm_port_array[index]);
	range = port->last_child_c_node - port->first_child_c_node + 1;

	rc = rm_free_port(ctl->rm, index, range);
	if (rc)
		return rc;

	profile = &(ctl->tm_p_lvl_drop_profiles[port->wred_profile_ref]);
	rc = rm_list_del_index(ctl->rm, profile->use_list, index, P_LEVEL);
	if (rc)
		return rc;

	ctl->tm_p_lvl_drop_profiles[ctl->tm_p_lvl_drop_prof_ptr[index]].
		use_counter--;
	ctl->tm_p_lvl_drop_prof_ptr[index] = 0;


	for (i = 0; i < TM_WRED_COS; i++) {
		if (port->wred_cos & (1<<i)) {
			profile = &(ctl->tm_p_lvl_drop_profiles_cos[i][port->wred_profile_ref_cos[i]]);
			rc = rm_list_del_index(ctl->rm, profile->use_list, index, P_LEVEL);
			if (rc)
				return rc;
		}
		ctl->tm_p_lvl_drop_profiles_cos[i][ctl->tm_p_lvl_drop_prof_ptr_cos[i][index]].
			use_counter--;
		ctl->tm_p_lvl_drop_prof_ptr_cos[i][index] = 0;
	}


	set_sw_port_default(ctl->tm_port_array, index, ctl->rm);

	/* Update SW image with DeQ disable function pointer */
	ctl->tm_port_array[index].elig_prio_func_ptr = 63;

	rc = set_hw_port(hndl, index);
	if (rc)
		return TM_HW_PORT_CONFIG_FAIL;

	/* Clear DWRR Deficit */
	rc = set_hw_port_deficit_clear(hndl, index);
	if (rc)
		return TM_HW_PORT_CONFIG_FAIL;

	return 0;
}


/**
 */
static int tm_delete_c_node(tm_handle hndl, uint32_t index)
{

	struct tm_c_node *c_node = NULL;
	uint16_t range;
	int rc;
	int i;

	DECLARE_TM_CTL_PTR(ctl, hndl)
	CHECK_TM_CTL_PTR(ctl)

	c_node = &(ctl->tm_c_node_array[index]);
	range = c_node->last_child_b_node - c_node->first_child_b_node + 1;
	rc = rm_free_c_node(ctl->rm, index, range);
	if (rc)
		return rc;

	for (i = 0; i < TM_WRED_COS; i++) {
		ctl->tm_c_lvl_drop_profiles[i][ctl->tm_c_lvl_drop_prof_ptr[i][index]].
			use_counter--;
		ctl->tm_c_lvl_drop_prof_ptr[i][index] = 0;
	}

	set_sw_c_node_default(ctl->tm_c_node_array, index, ctl->rm);

	/* Update SW image with DeQ disable function pointer */
	ctl->tm_c_node_array[index].elig_prio_func_ptr = 63;

	rc = set_hw_c_node(hndl, index);
	if (rc)
		return TM_HW_C_NODE_CONFIG_FAIL;

	rc = set_hw_c_node_shaping_def(ctl, index);
	if (rc)
		return TM_HW_C_NODE_CONFIG_FAIL;

	/* Clear DWRR Deficit */
	rc = set_hw_c_node_deficit_clear(hndl, index);
	if (rc)
		return TM_HW_C_NODE_CONFIG_FAIL;

	return rc;
}


/**
 */
static int tm_delete_b_node(tm_handle hndl, uint32_t index)
{

	struct tm_b_node *b_node = NULL;
	uint16_t range;
	int rc;

	DECLARE_TM_CTL_PTR(ctl, hndl)
	CHECK_TM_CTL_PTR(ctl)

	b_node = &(ctl->tm_b_node_array[index]);
	range = b_node->last_child_a_node - b_node->first_child_a_node + 1;
	rc = rm_free_b_node(ctl->rm, index, range);
	if (rc)
		return rc;

	ctl->tm_b_lvl_drop_prof_ptr[index] = 0;
	ctl->tm_b_lvl_drop_profiles[b_node->wred_profile_ref].use_counter--;
	set_sw_b_node_default(ctl->tm_b_node_array, index, ctl->rm);

	/* Update SW image with DeQ disable function pointer */
	ctl->tm_b_node_array[index].elig_prio_func_ptr = 63;

	rc = set_hw_b_node(hndl, index);
	if (rc)
		return TM_HW_B_NODE_CONFIG_FAIL;

	rc = set_hw_b_node_shaping_def(ctl, index);
	if (rc)
		return TM_HW_B_NODE_CONFIG_FAIL;

	/* Clear DWRR Deficit */
	rc = set_hw_b_node_deficit_clear(hndl, index);
	if (rc)
		return TM_HW_B_NODE_CONFIG_FAIL;

	return rc;
}


/**
 */
static int tm_delete_a_node(tm_handle hndl, uint32_t index)
{
	struct tm_a_node *a_node = NULL;
	uint32_t range;
	int rc;

	DECLARE_TM_CTL_PTR(ctl, hndl)
	CHECK_TM_CTL_PTR(ctl)

	a_node = &(ctl->tm_a_node_array[index]);
	range = a_node->last_child_queue - a_node->first_child_queue + 1;
	rc = rm_free_a_node(ctl->rm, index, range);
	if (rc)
		return rc;

	ctl->tm_a_lvl_drop_prof_ptr[index] = 0;
	ctl->tm_a_lvl_drop_profiles[a_node->wred_profile_ref].use_counter--;
	set_sw_a_node_default(ctl->tm_a_node_array, index, ctl->rm);

	/* Update SW image with DeQ disable function pointer */
	ctl->tm_a_node_array[index].elig_prio_func_ptr = 63;

	rc = set_hw_a_node(hndl, index);
	if (rc)
		return TM_HW_A_NODE_CONFIG_FAIL;

	rc = set_hw_a_node_shaping_def(ctl, index);
	if (rc)
		return TM_HW_A_NODE_CONFIG_FAIL;

	/* Clear DWRR Deficit */
	rc = set_hw_a_node_deficit_clear(hndl, index);
	if (rc)
		return TM_HW_A_NODE_CONFIG_FAIL;
	return rc;
}


/**
 */
static int tm_delete_queue(tm_handle hndl, uint32_t index)
{
	struct tm_queue *queue = NULL;
	int rc;

	DECLARE_TM_CTL_PTR(ctl, hndl)
	DECLARE_RM_HANDLE(rm, ctl->rm)
	CHECK_TM_CTL_PTR(ctl)

	queue = &(ctl->tm_queue_array[index]);
	rc = rm_free_queue(ctl->rm, index);
	if (rc)
		return rc;

	ctl->tm_q_lvl_drop_prof_ptr[index] = 0;
	ctl->tm_q_lvl_drop_profiles[queue->wred_profile_ref].use_counter--;
	set_sw_queue_default(ctl->tm_queue_array, index, rm);

	/* Update SW image with DeQ disable function pointer */
	ctl->tm_queue_array[index].elig_prio_func_ptr = 63;

	rc = set_hw_queue(ctl, index);
	if (rc)
		return TM_HW_QUEUE_CONFIG_FAIL;

	rc = set_hw_queue_shaping_def(ctl, index);
	if (rc)
		return TM_HW_QUEUE_CONFIG_FAIL;

	/* Clear Queue DWRR Deficit */
	rc = set_hw_queue_deficit_clear(ctl, index);
	if (rc)
		return TM_HW_QUEUE_CONFIG_FAIL;

	return rc;
}


/**
 */
int tm_delete_node(tm_handle hndl, enum tm_level lvl, uint32_t index)
{
	uint8_t status;
	int rc;

	DECLARE_TM_CTL_PTR(ctl, hndl)
	CHECK_TM_CTL_PTR(ctl)

	rc = tm_nodes_lock(TM_ENV(ctl));
	if (rc)
		return rc;

	switch (lvl) {
	case P_LEVEL:
		rc = rm_node_status(ctl->rm, P_LEVEL, index, &status);
		if ((!rc) && (status != RM_TRUE)) rc = -ENODATA;
		if (rc) goto out;
		rc = tm_delete_port(hndl, (uint8_t)index);
		break;
	case C_LEVEL:
		rc = rm_node_status(ctl->rm, C_LEVEL, index, &status);
		if ((!rc) && (status != RM_TRUE)) rc = -ENODATA;
		if (rc) goto out;
		rc = tm_delete_c_node(hndl, index);
		break;
	case B_LEVEL:
		rc = rm_node_status(ctl->rm, B_LEVEL, index, &status);
		if ((!rc) && (status != RM_TRUE)) rc = -ENODATA;
		if (rc) goto out;
		rc = tm_delete_b_node(hndl, index);
		break;
	case A_LEVEL:
		rc = rm_node_status(ctl->rm, A_LEVEL, index, &status);
		if ((!rc) && (status != RM_TRUE)) rc = -ENODATA;
		if (rc) goto out;
		rc = tm_delete_a_node(hndl, index);
		break;
	case Q_LEVEL:
		rc = rm_node_status(ctl->rm, Q_LEVEL, index, &status);
		if ((!rc) && (status != RM_TRUE)) rc = -ENODATA;
		if (rc) goto out;
		rc = tm_delete_queue(hndl, index);
		break;
	default:
		/* can't happend*/
		rc = -ERANGE;
	}

out:
	tm_nodes_unlock(TM_ENV(ctl));
	return rc;
}


/**
 */
int tm_delete_trans_port(tm_handle hndl, uint8_t index)
{
	uint8_t status;
	int rc;
	int i;
	int j;
	int k;
	int l;

	DECLARE_TM_CTL_PTR(ctl, hndl)
	CHECK_TM_CTL_PTR(ctl)


	rc = tm_nodes_lock(TM_ENV(ctl));
	if (rc)
		return rc;

	rc = rm_node_status(ctl->rm, P_LEVEL, index, &status);
	if ((rc) || (status != RM_TRUE)) {
		rc = -ENOMSG;
		goto out;
	}


	for (i = ctl->tm_port_array[index].first_child_c_node;
		 i <= ctl->tm_port_array[index].last_child_c_node; i++)
	{
		rc = rm_node_status(ctl->rm, C_LEVEL, i, &status);
		if (rc)
			goto out;
		if (status != RM_TRUE)
			break;

		for (j = ctl->tm_c_node_array[i].first_child_b_node;
			 j <= ctl->tm_c_node_array[i].last_child_b_node; j++)
		{
			rc = rm_node_status(ctl->rm, B_LEVEL, j, &status);
			if (rc)
				goto out;
			if (status != RM_TRUE)
				break;

			for (k = ctl->tm_b_node_array[j].first_child_a_node;
				 k <= ctl->tm_b_node_array[j].last_child_a_node; k++)
			{
				rc = rm_node_status(ctl->rm, A_LEVEL, k, &status);
				if (rc)
					goto out;
				if (status != RM_TRUE)
					break;

				for (l = ctl->tm_a_node_array[k].first_child_queue;
					 l <= ctl->tm_a_node_array[k].last_child_queue; l++)
				{
					rc = tm_delete_queue(hndl, l);
					if ((rc) && (rc != -ENOMSG))
						goto out;
				}
				rc = tm_delete_a_node(hndl, k);
				if (rc)
					goto out;
			}
			rc = tm_delete_b_node(hndl, j);
			if (rc)
				goto out;
		}
		rc = tm_delete_c_node(hndl, i);
		if (rc)
			goto out;
	}

	rc = tm_delete_port(hndl, index);

out:
	tm_nodes_unlock(TM_ENV(ctl));
	return rc;
}


/**
 */
int tm_nodes_read_next_change(tm_handle hndl, struct tm_tree_change *change)
{
	struct tm_tree_change *elem = NULL;
	int rc = 0;

	DECLARE_TM_CTL_PTR(ctl, hndl)
	CHECK_TM_CTL_PTR(ctl)

	rc = tm_nodes_lock(TM_ENV(ctl));
	if (rc)
		return rc;
	elem = ctl->list.next;
	if (elem == NULL) { /* empty list */
		rc = -ENOBUFS;
		goto out;
	}

	ctl->list.next = elem->next;
	change->type = elem->type;
	change->index = elem->index;
	change->old_index = elem->old_index;
	change->new_index = elem->new_index;
	change->next = NULL;
	tm_free(elem);

out:
	tm_nodes_unlock(TM_ENV(ctl));
	return rc;
}


/**
 */
int tm_clean_list(tm_handle hndl)
{

	struct tm_tree_change *elem = NULL;
	int rc = 0;
	DECLARE_TM_CTL_PTR(ctl, hndl)
	CHECK_TM_CTL_PTR(ctl)

	rc = tm_nodes_lock(TM_ENV(ctl));
	if (rc)
		return rc;

	while (ctl->list.next != NULL) {
		elem = ctl->list.next;
		ctl->list.next = elem->next;
		tm_free(elem);
	}
	tm_nodes_unlock(TM_ENV(ctl));

	return rc;
}

