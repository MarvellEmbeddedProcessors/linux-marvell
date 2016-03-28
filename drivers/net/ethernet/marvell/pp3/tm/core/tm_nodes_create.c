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

#include "tm_nodes_create.h"
#include "rm_status.h"
#include "rm_alloc.h"
#include "rm_free.h"
#include "rm_reorder.h"
#include "rm_list.h"
#include "tm_locking_interface.h"
#include "tm_errcodes.h"

#include "tm_set_local_db_defaults.h"
#include "set_hw_registers.h"
#include "tm_os_interface.h"

#include "tm_nodes_utils.h"
#include "tm_nodes_ctl.h"
#include "tm_hw_configuration_interface.h"


/**
 */
static int tm_nodes_on_off(tm_handle hndl,
			enum tm_level level,
			uint32_t parent_node,
			uint32_t first_child_node,
			uint32_t last_child_node,
			uint8_t enable)
{
	int rc;
	uint32_t i;
	uint8_t state;

	DECLARE_TM_CTL_PTR(ctl, hndl)
	CHECK_TM_CTL_PTR(ctl)

	if (enable) {
		rc = set_hw_deq_status(hndl, level, parent_node);
		if (rc)
			return rc;

		for (i = first_child_node; i <= last_child_node; i++) {
			rc = set_hw_deq_status(hndl, level-1, i);
			if (rc)
				return rc;
		}
	} else {
		switch (level) {
		case A_LEVEL:
			state = ctl->tm_a_node_array[parent_node].elig_prio_func_ptr;
			ctl->tm_a_node_array[parent_node].elig_prio_func_ptr = TM_ELIG_DEQ_DISABLE;
			rc = set_hw_deq_status(ctl, A_LEVEL, parent_node);
			if (rc)
				return rc;
			ctl->tm_a_node_array[parent_node].elig_prio_func_ptr = state;

			for (i = first_child_node; i <= last_child_node; i++) {
				state = ctl->tm_queue_array[i].elig_prio_func_ptr;
				ctl->tm_queue_array[i].elig_prio_func_ptr = TM_ELIG_DEQ_DISABLE;
				rc = set_hw_deq_status(ctl, Q_LEVEL, i);
				if (rc)
					return rc;
				ctl->tm_queue_array[i].elig_prio_func_ptr = state;
			}
			break;
		case B_LEVEL:
			state = ctl->tm_b_node_array[parent_node].elig_prio_func_ptr;
			ctl->tm_b_node_array[parent_node].elig_prio_func_ptr = TM_ELIG_DEQ_DISABLE;
			rc = set_hw_deq_status(ctl, B_LEVEL, parent_node);
			if (rc)
				return rc;
			ctl->tm_b_node_array[parent_node].elig_prio_func_ptr = state;

			for (i = first_child_node; i <= last_child_node; i++) {
				state = ctl->tm_a_node_array[i].elig_prio_func_ptr;
				ctl->tm_a_node_array[i].elig_prio_func_ptr = TM_ELIG_DEQ_DISABLE;
				rc = set_hw_deq_status(ctl, A_LEVEL, i);
				if (rc)
					return rc;
				ctl->tm_a_node_array[i].elig_prio_func_ptr = state;
			}
			break;
		case C_LEVEL:
			state = ctl->tm_c_node_array[parent_node].elig_prio_func_ptr;
			ctl->tm_c_node_array[parent_node].elig_prio_func_ptr = TM_ELIG_DEQ_DISABLE;
			rc = set_hw_deq_status(ctl, C_LEVEL, parent_node);
			if (rc)
				return rc;
			ctl->tm_c_node_array[parent_node].elig_prio_func_ptr = state;

			for (i = first_child_node; i <= last_child_node; i++) {
				state = ctl->tm_b_node_array[i].elig_prio_func_ptr;
				ctl->tm_b_node_array[i].elig_prio_func_ptr = TM_ELIG_DEQ_DISABLE;
				rc = set_hw_deq_status(ctl, B_LEVEL, i);
				if (rc)
					return rc;
				ctl->tm_b_node_array[i].elig_prio_func_ptr = state;
			}
			break;
		case P_LEVEL:
			state = ctl->tm_port_array[parent_node].elig_prio_func_ptr;
			ctl->tm_port_array[parent_node].elig_prio_func_ptr = TM_ELIG_DEQ_DISABLE;
			rc = set_hw_deq_status(ctl, P_LEVEL, parent_node);
			if (rc)
				return rc;
			ctl->tm_port_array[parent_node].elig_prio_func_ptr = state;

			for (i = first_child_node; i <= last_child_node; i++) {
				state = ctl->tm_c_node_array[i].elig_prio_func_ptr;
				ctl->tm_c_node_array[i].elig_prio_func_ptr = TM_ELIG_DEQ_DISABLE;
				rc = set_hw_deq_status(ctl, C_LEVEL, i);
				if (rc)
					return rc;
				ctl->tm_c_node_array[i].elig_prio_func_ptr = state;
			}
			break;
		default:
			return -ERANGE;
		}
	}
	return 0;
}


/**
 */
static int tm_copy_b_node(tm_handle hndl,
			uint32_t index, /* copy this node */
			uint32_t range, /* range of this node */
			uint32_t parent, /* where to copy */
			uint32_t *new_index)
{
	struct rm_node *rm_node = NULL;
	struct tm_b_node *node1 = NULL;
	struct tm_b_node *node2 = NULL;
	int rc;
	int i;

	DECLARE_TM_CTL_PTR(ctl, hndl)
	DECLARE_RM_HANDLE(rm, ctl->rm)
	CHECK_TM_CTL_PTR(ctl);

	node1 = &(ctl->tm_b_node_array[index]);
	rc = rm_find_free_b_node(rm, parent, range);
	if (rc < 0)
		return -ENOBUFS;
	*new_index = rc;

	node2 = &(ctl->tm_b_node_array[*new_index]);
	rm_node = &(rm->rm_b_node_array[*new_index]);
	node2->parent_c_node = parent;
	node2->first_child_a_node = rm_node->first_child;
	node2->last_child_a_node = rm_node->last_child;
	for (i = node2->first_child_a_node; i <= node2->last_child_a_node; i++)
		ctl->tm_a_node_array[i].parent_b_node = *new_index;

	node2->elig_prio_func_ptr = node1->elig_prio_func_ptr;
	node2->dwrr_quantum = node1->dwrr_quantum;
	node2->dwrr_priority = node1->dwrr_priority;

	ctl->tm_b_lvl_drop_prof_ptr[*new_index] = node1->wred_profile_ref;
	node2->wred_profile_ref = node1->wred_profile_ref;
	ctl->tm_b_lvl_drop_profiles[node2->wred_profile_ref].use_counter++;

	rc = set_hw_b_node(hndl, *new_index);
	if (rc)
		return TM_HW_B_NODE_CONFIG_FAIL;

	return rc;
}


/**
 */
static int tm_copy_a_node(tm_handle hndl,
			uint32_t index, /* copy this node */
			uint32_t range, /* range of this node */
			uint32_t parent, /* where to copy */
			uint32_t *new_index)
{
	struct rm_node *rm_node = NULL;
	struct tm_a_node *node1 = NULL;
	struct tm_a_node *node2 = NULL;
	int rc;
	uint32_t i;

	DECLARE_TM_CTL_PTR(ctl, hndl)
	DECLARE_RM_HANDLE(rm, ctl->rm)
	CHECK_TM_CTL_PTR(ctl);

	node1 = &(ctl->tm_a_node_array[index]);
	rc = rm_find_free_a_node(rm, parent, range);
	if (rc < 0)
		return -ENOBUFS;
	*new_index = rc;

	node2 = &(ctl->tm_a_node_array[*new_index]);
	rm_node = &(rm->rm_a_node_array[*new_index]);
	node2->parent_b_node = parent;
	node2->first_child_queue = rm_node->first_child;
	node2->last_child_queue = rm_node->last_child;
	for (i = node2->first_child_queue; i <= node2->last_child_queue; i++)
		ctl->tm_queue_array[i].parent_a_node = *new_index;

	node2->elig_prio_func_ptr = node1->elig_prio_func_ptr;
	node2->dwrr_quantum = node1->dwrr_quantum;
	node2->dwrr_priority = node1->dwrr_priority;

	ctl->tm_a_lvl_drop_prof_ptr[*new_index] = node1->wred_profile_ref;
	node2->wred_profile_ref = node1->wred_profile_ref;
	ctl->tm_a_lvl_drop_profiles[node2->wred_profile_ref].use_counter++;

	rc = set_hw_a_node(hndl, *new_index);
	if (rc)
		return TM_HW_A_NODE_CONFIG_FAIL;

	return rc;
}


/**
 */
int tm_c_nodes_switch(tm_handle hndl,
			uint32_t index1,
			uint32_t index2)
{

	int rc;
	uint32_t i;
	struct tm_c_node *node1 = NULL;
	struct tm_c_node *node2 = NULL;
	uint32_t first_child1;
	uint32_t last_child1;
	uint32_t first_child2;
	uint32_t last_child2;

	DECLARE_TM_CTL_PTR(ctl, hndl)
	DECLARE_RM_HANDLE(rm, ctl->rm)
	CHECK_TM_CTL_PTR(ctl);

	node1 = &(ctl->tm_c_node_array[index1]);
	node2 = &(ctl->tm_c_node_array[index2]);

	first_child1 = node1->first_child_b_node;
	last_child1  = node1->last_child_b_node;
	first_child2 = node2->first_child_b_node;
	last_child2  = node2->last_child_b_node;

	/* Assumption: DeQ disable was performed outside */

	rc = rm_nodes_switch(rm, RM_B_LVL, index1, index2,
			 first_child1, last_child1,
			 first_child2, last_child2);
	if (rc)
		return rc;

	/* Update node1 */
	node1->first_child_b_node = first_child2;
	node1->last_child_b_node = last_child2;

	rc = set_hw_map(ctl, C_LEVEL, index1);
	if (rc)
		return rc;

	for (i = first_child2; i <= last_child2 ; i++) {
		ctl->tm_b_node_array[i].parent_c_node = index1;
		rc = set_hw_map(ctl, B_LEVEL, i);
		if (rc)
			return rc;
	}

	/* Update node2 */
	node2->first_child_b_node = first_child1;
	node2->last_child_b_node = last_child1;
	rc = set_hw_map(ctl, C_LEVEL, index2);
	if (rc)
		return rc;

	for (i = first_child1; i <= last_child1 ; i++) {
		ctl->tm_b_node_array[i].parent_c_node = index2;
		rc = set_hw_map(ctl, B_LEVEL, i);
		if (rc)
			return rc;
	}

	return 0;
}


/**
 */
int tm_b_nodes_switch(tm_handle hndl,
			uint32_t index1,
			uint32_t index2)
{
	int rc;
	uint32_t i;
	struct tm_b_node *node1 = NULL;
	struct tm_b_node *node2 = NULL;
	uint32_t first_child1;
	uint32_t last_child1;
	uint32_t first_child2;
	uint32_t last_child2;

	DECLARE_TM_CTL_PTR(ctl, hndl)
	DECLARE_RM_HANDLE(rm, ctl->rm)
	CHECK_TM_CTL_PTR(ctl);

	node1 = &(ctl->tm_b_node_array[index1]);
	node2 = &(ctl->tm_b_node_array[index2]);

	first_child1 = node1->first_child_a_node;
	last_child1  = node1->last_child_a_node;
	first_child2 = node2->first_child_a_node;
	last_child2  = node2->last_child_a_node;

	/* Assumption: DeQ disable was performed outside */

	rc = rm_nodes_switch(rm, RM_A_LVL, index1, index2,
			first_child1, last_child1,
			first_child2, last_child2);
	if (rc)
		return rc;

	/* Update node1 */
	node1->first_child_a_node = first_child2;
	node1->last_child_a_node = last_child2;
	rc = set_hw_map(ctl, B_LEVEL, index1);
	if (rc)
		return rc;

	for (i = first_child2; i <= last_child2 ; i++) {
		ctl->tm_a_node_array[i].parent_b_node = index1;
		rc = set_hw_map(ctl, A_LEVEL, i);
		if (rc)
			return rc;
	}

	/* Update node2 */
	node2->first_child_a_node = first_child1;
	node2->last_child_a_node = last_child1;
	rc = set_hw_map(ctl, B_LEVEL, index2);
	if (rc)
		return rc;

	for (i = first_child1; i <= last_child1 ; i++) {
		ctl->tm_a_node_array[i].parent_b_node = index2;
		rc = set_hw_map(ctl, A_LEVEL, i);
		if (rc)
			return rc;
	}

	return 0;
}


/**
 */
int tm_a_nodes_switch(tm_handle hndl,
			uint32_t index1,
			uint32_t index2)
{
	int rc;
	uint32_t i;
	struct tm_a_node *node1 = NULL;
	struct tm_a_node *node2 = NULL;
	uint32_t first_child1;
	uint32_t last_child1;
	uint32_t first_child2;
	uint32_t last_child2;

	DECLARE_TM_CTL_PTR(ctl, hndl)
	DECLARE_RM_HANDLE(rm, ctl->rm)
	CHECK_TM_CTL_PTR(ctl);

	node1 = &(ctl->tm_a_node_array[index1]);
	node2 = &(ctl->tm_a_node_array[index2]);

	first_child1 = node1->first_child_queue;
	last_child1  = node1->last_child_queue;
	first_child2 = node2->first_child_queue;
	last_child2  = node2->last_child_queue;

	/* Assumption: DeQ disable was performed outside */

	rc = rm_nodes_switch(rm, RM_Q_LVL, index1, index2,
			first_child1, last_child1,
			first_child2, last_child2);
	if (rc)
		return rc;

	/* Update node1 */
	node1->first_child_queue = first_child2;
	node1->last_child_queue = last_child2;
	rc = set_hw_map(ctl, A_LEVEL, index1);
	if (rc)
		return rc;

	for (i = first_child2; i <= last_child2 ; i++) {
		ctl->tm_queue_array[i].parent_a_node = index1;
		rc = set_hw_map(ctl, Q_LEVEL, i);
		if (rc)
			return rc;
	}

	/* Update node2 */
	node2->first_child_queue = first_child1;
	node2->last_child_queue = last_child1;
	rc = set_hw_map(ctl, A_LEVEL, index2);
	if (rc)
		return rc;

	for (i = first_child1; i <= last_child1 ; i++) {
		ctl->tm_queue_array[i].parent_a_node = index2;
		rc = set_hw_map(ctl, Q_LEVEL, i);
		if (rc)
			return rc;
	}

	return 0;
}


/** Copied from tm_nodes_ctl.
 */
static int tm_delete_a_node(tm_handle hndl, uint32_t index)
{
	struct tm_a_node *a_node = NULL;
	uint32_t range;
	int rc;

	DECLARE_TM_CTL_PTR(ctl, hndl)
	DECLARE_RM_HANDLE(rm, ctl->rm)
	CHECK_TM_CTL_PTR(ctl);

	a_node = &(ctl->tm_a_node_array[index]);
	range = a_node->last_child_queue - a_node->first_child_queue + 1;
	rc = rm_free_a_node(rm, index, range);
	if (rc)
		return rc;

	ctl->tm_a_lvl_drop_prof_ptr[index] = 0;
	ctl->tm_a_lvl_drop_profiles[a_node->wred_profile_ref].use_counter--;
	set_sw_a_node_default(ctl->tm_a_node_array, index, rm);

	/* Update SW image with DeQ disable function pointer */
	ctl->tm_a_node_array[index].elig_prio_func_ptr = 63;

	rc = set_hw_a_node(hndl, index);
	if (rc)
		return TM_HW_A_NODE_CONFIG_FAIL;

	/* Clear DWRR Deficit */
	rc = set_hw_a_node_deficit_clear(ctl, index);
	if (rc)
		return TM_HW_A_NODE_CONFIG_FAIL;

	return 0;
}


/** Copied from tm_nodes_ctl.
 */
static int tm_delete_b_node(tm_handle hndl, uint32_t index)
{
	struct tm_b_node *b_node = NULL;
	uint32_t range;
	int rc;

	DECLARE_TM_CTL_PTR(ctl, hndl)
	DECLARE_RM_HANDLE(rm, ctl->rm)
	CHECK_TM_CTL_PTR(ctl);

	b_node = &(ctl->tm_b_node_array[index]);
	range = b_node->last_child_a_node - b_node->first_child_a_node + 1;
	rc = rm_free_b_node(rm, index, range);
	if (rc)
		return rc;

	ctl->tm_b_lvl_drop_prof_ptr[index] = 0;
	ctl->tm_b_lvl_drop_profiles[b_node->wred_profile_ref].use_counter--;
	set_sw_b_node_default(ctl->tm_b_node_array, index, rm);

	/* Update SW image with DeQ disable function pointer */
	ctl->tm_b_node_array[index].elig_prio_func_ptr = 63;

	rc = set_hw_b_node(hndl, index);
	if (rc)
		return TM_HW_B_NODE_CONFIG_FAIL;

	/* Clear DWRR Deficit */
	rc = set_hw_b_node_deficit_clear(ctl, index);
	if (rc)
		return TM_HW_B_NODE_CONFIG_FAIL;

	return 0;
}


/** Move first child of the from_node to the end of the to_node.
 *  Assumption: first child of the from_node should free and consequent to the last child of to_node.
 */
int tm_node_move(tm_handle hndl,
		 enum tm_level level,
		 uint32_t from_node,
		 uint32_t to_node)
{
	int rc;
	uint32_t first_child_to_move;
	uint32_t first_child_from_node;
	uint32_t last_child_from_node;
	uint32_t last_child_to_node;

	DECLARE_TM_CTL_PTR(ctl, hndl)
	DECLARE_RM_HANDLE(rm, ctl->rm)
	CHECK_TM_CTL_PTR(ctl);

	switch (level) {
	case B_LEVEL:
		first_child_from_node = ctl->tm_b_node_array[from_node].first_child_a_node;
		last_child_from_node = ctl->tm_b_node_array[from_node].last_child_a_node;
		last_child_to_node = ctl->tm_b_node_array[to_node].last_child_a_node;
		break;
	case C_LEVEL:
		first_child_from_node = ctl->tm_c_node_array[from_node].first_child_b_node;
		last_child_from_node = ctl->tm_c_node_array[from_node].last_child_b_node;
		last_child_to_node = ctl->tm_c_node_array[to_node].last_child_b_node;
		break;
	default:
		return -ERANGE;
	}

	/* Check if the from_node will be not empty after the move */
	if (last_child_from_node - first_child_from_node < 1) {
		rc = -EFAULT;
		goto err_out;
	}

	/* Ensure the children of the from_node and to_node are adjecent */
	if (last_child_to_node + 1 != first_child_from_node) {
		rc = -EBUSY;
		goto err_out;
	}

	/* Calculate new values for parent nodes */
	first_child_to_move = first_child_from_node;
	first_child_from_node = first_child_to_move + 1;
	last_child_to_node = first_child_from_node - 1;

	/* Disable dequeing on all child and parent node */
	rc = tm_nodes_on_off(hndl, level, from_node,
			first_child_to_move,
			first_child_to_move,
			TM_DISABLE);
	if (rc)
		goto err_out;

	rc = tm_nodes_on_off(hndl, level, to_node, 0, 0, TM_DISABLE);
	if (rc)
		goto err_out;

	/* Move children, reconfigure parents */
	switch (level) {
	case B_LEVEL:
		/* RM */
		rc = rm_nodes_move(rm, RM_A_LVL, from_node, to_node, 1, first_child_to_move);
		if (rc)
			goto err_out;

		/* TM */
		ctl->tm_b_node_array[from_node].first_child_a_node = first_child_from_node;
		rc = set_hw_map(ctl, level, from_node);
		if (rc)
			goto err_out;

		ctl->tm_b_node_array[to_node].last_child_a_node = last_child_to_node;
		rc = set_hw_map(ctl, level, to_node);
		if (rc)
			goto err_out;

		ctl->tm_a_node_array[first_child_to_move].parent_b_node = to_node;
		rc = set_hw_map(ctl, A_LEVEL, first_child_to_move);
		if (rc)
			goto err_out;
		break;
	case C_LEVEL:
		/* RM */
		rc = rm_nodes_move(rm, RM_B_LVL, from_node, to_node, 1, first_child_to_move);
		if (rc)
			goto err_out;

		/* TM */
		ctl->tm_c_node_array[from_node].first_child_b_node = first_child_from_node;
		rc = set_hw_map(ctl, level, from_node);
		if (rc)
			goto err_out;

		ctl->tm_c_node_array[to_node].last_child_b_node = last_child_to_node;
		rc = set_hw_map(ctl, level, to_node);
		if (rc)
			goto err_out;

		ctl->tm_b_node_array[first_child_to_move].parent_c_node = to_node;
		rc = set_hw_map(ctl, B_LEVEL, first_child_to_move);
		if (rc)
			goto err_out;
		break;
	default:
		return -EINVAL;
	}

	/* Enable all nodes again in HW from SW model*/
	rc = tm_nodes_on_off(hndl, level, from_node,
			first_child_to_move,
			first_child_to_move,
			TM_ENABLE);
	if (rc)
		goto err_out;

	rc = tm_nodes_on_off(hndl, level, to_node, 0, 0, TM_ENABLE);

err_out:
	 return rc;
}



/**
 */
static int tm_nodes_reshuffling(tm_handle hndl,
				enum tm_level level, /* level of reshuffled nodes */
				uint32_t index)      /* parent index */
{
	int rc;
	struct tm_a_node *a_node = NULL;
	struct tm_b_node *b_node = NULL;
	struct tm_c_node *c_node = NULL;
	uint32_t last_child;
	uint32_t next_index;
	uint32_t new_index;
	uint32_t b_index;
	uint32_t c_index;
	uint32_t p_index;
	uint32_t range;
	uint8_t status;
	struct tm_tree_change *change = NULL;

	DECLARE_TM_CTL_PTR(ctl, hndl)
	DECLARE_RM_HANDLE(rm, ctl->rm)
	CHECK_TM_CTL_PTR(ctl);

	/* Check assymetric mode */
	switch (level) {
	case A_LEVEL:
		b_node = &(ctl->tm_b_node_array[index]); /* parent node */
		c_index = b_node->parent_c_node;
		c_node = &(ctl->tm_c_node_array[c_index]);
		break;
	case B_LEVEL:
		c_node = &(ctl->tm_c_node_array[index]); /* parent node */
		break;
	default:
		return -ERANGE;
	}
	p_index = c_node->parent_port;
	if (ctl->tm_port_array[p_index].sym_mode == TM_ENABLE)
		return -EFAULT; /* symetric mode under the port */

	switch (level) {
	case A_LEVEL:
		last_child = b_node->last_child_a_node;
		next_index = last_child + 1;

		if (next_index >= rm->rm_total_a_nodes)
			return -ENOBUFS;

		a_node = &(ctl->tm_a_node_array[next_index]);
		b_index = a_node->parent_b_node; /* parent of neighbor node */

		rc = rm_node_status(rm, A_LEVEL, next_index, &status);
		if (rc)
			goto err_out;

		if (status == RM_FALSE) { /* free node */
			rc = rm_node_status(rm, B_LEVEL, b_index, &status);
			if (rc)
				goto err_out;

			/* parent initialization for all nodes in the last possible index */
			if ((next_index < ctl->tm_b_node_array[b_index].first_child_a_node) ||
				/* not in this parent's range */
				(next_index > ctl->tm_b_node_array[b_index].last_child_a_node)  ||
				/* not in this parent's range */
				(status == RM_FALSE)) { /* parent is free too */
			/* so the node is in free nodes pool */
			rc = rm_expand_range(rm, RM_A_LVL, next_index, index);
			if (rc)
				goto err_out;

			/* Update the range of current parent to include this node */
			b_node->last_child_a_node++;

			status = b_node->elig_prio_func_ptr;
			b_node->elig_prio_func_ptr = TM_ELIG_DEQ_DISABLE;
			rc = set_hw_deq_status(ctl, B_LEVEL, index);
			if (rc)
				goto err_out;

			rc = set_hw_map(ctl, B_LEVEL, index);
			if (rc)
				goto err_out;

			b_node->elig_prio_func_ptr = status;
			rc = set_hw_deq_status(ctl, B_LEVEL, index);
			if (rc)
				goto err_out;

			/* Update the range of the allocated child node */
			a_node->parent_b_node = index;
		} else {
			/* parent is allocated, but neighbor node is free */
			/* reallocate the node to expand the range of our current parent */
			rc = tm_node_move(ctl, B_LEVEL, b_index, index);
			if (rc)
				goto err_out;

			change = (struct tm_tree_change *)tm_malloc(sizeof(struct tm_tree_change));
			change->type = TM_DISABLE;
			change->index = b_index;
			change->old_index = ctl->tm_b_node_array[b_index].last_child_a_node -
				ctl->tm_b_node_array[b_index].first_child_a_node + 2;
			change->new_index = change->old_index - 1;
			change->next = ctl->list.next;
			ctl->list.next = change;
			}
		} else {
			/* the node is allocated and used */
			/* copy next_index to spare node under the same parent */
			range = a_node->last_child_queue - a_node->first_child_queue + 1;
			rc = tm_copy_a_node(ctl, next_index, range, b_index, &new_index);
			if (rc == -ENOBUFS) {
				/* this parent is full */
			rc = tm_nodes_reshuffling(ctl, A_LEVEL, b_index);
			if (rc)
				goto err_out;
			else {
				/* try copy A-node again */
				rc = tm_copy_a_node(ctl, next_index, range, b_index, &new_index);
				if (rc)
					goto err_out;
			}

			/* switch ranges between copied nodes */
			rc = tm_a_nodes_switch(ctl, next_index, new_index);
			if (rc)
				goto err_out;

			/* delete next_index A-node */
			rc = tm_delete_a_node(ctl, next_index);
			if (rc)
				goto err_out;

			/* expand range of our original parent with freed node */
			rc = tm_node_move(ctl, B_LEVEL, b_index, index);
			if (rc)
				goto err_out;

			change = (struct tm_tree_change *)tm_malloc(sizeof(struct tm_tree_change));
			change->type = TM_ENABLE;
			change->index = b_index;
			change->old_index = next_index;
			change->new_index = new_index;
			change->next = ctl->list.next;
			ctl->list.next = change;
		} else /* other error */
		{
			if (rc)
				goto err_out;
		}
		}
		break;
	case B_LEVEL:
		last_child = c_node->last_child_b_node;
		next_index = last_child + 1;

		if (next_index >= rm->rm_total_b_nodes)
			return -ENOBUFS;

		rc = rm_node_status(rm, B_LEVEL, next_index, &status);
		if (rc)
			goto err_out;

		b_node = &(ctl->tm_b_node_array[next_index]);
		c_index = b_node->parent_c_node; /* parent of neighbor node */

		if (status == RM_FALSE) { /* free node */
			rc = rm_node_status(rm, C_LEVEL, c_index, &status);
			if (rc)
				goto err_out;

			/* parent initialization for all nodes in the last possible index */
			if ((next_index < ctl->tm_c_node_array[c_index].first_child_b_node) ||
				/* not in this parent's range */
			(next_index > ctl->tm_c_node_array[c_index].last_child_b_node)  ||
			/* not in this parent's range */
			(status == RM_FALSE)) { /* parent is free too */
			/* so the node is in free nodes pool */
			rc = rm_expand_range(rm, RM_B_LVL, next_index, index);
			if (rc)
				goto err_out;
			/* Update the range of current parent to include this node */
			c_node->last_child_b_node++;

			status = c_node->elig_prio_func_ptr;
			c_node->elig_prio_func_ptr = TM_ELIG_DEQ_DISABLE;
			rc = set_hw_deq_status(ctl, C_LEVEL, index);
			if (rc)
				goto err_out;

			rc = set_hw_map(ctl, C_LEVEL, index);
			if (rc)
				goto err_out;

			c_node->elig_prio_func_ptr = status;
			rc = set_hw_deq_status(ctl, C_LEVEL, index);
			if (rc)
				goto err_out;

			/* Update the range of the allocated child node */
			b_node->parent_c_node = index;
		} else {
			/* parent is allocated, but neighbor node is free */
			/* reallocate the node to expand the range of our current parent */
			rc = tm_node_move(ctl, C_LEVEL, c_index, index);
			if (rc)
				goto err_out;

			change = (struct tm_tree_change *)tm_malloc(sizeof(struct tm_tree_change));
			change->type = TM_DISABLE;
			change->index = c_index;
			change->old_index = ctl->tm_c_node_array[c_index].last_child_b_node -
				ctl->tm_c_node_array[c_index].first_child_b_node + 2;
			change->new_index = change->old_index - 1;
			change->next = ctl->list.next;
			ctl->list.next = change;
		}
		} else {
			/* the node is allocated and used */
			/* copy next_index to spare node under the same parent */
			range = b_node->last_child_a_node - b_node->first_child_a_node + 1;
			rc = tm_copy_b_node(ctl, next_index, range, c_index, &new_index);
			if (rc == -ENOBUFS) { /* this parent is full */
			rc = tm_nodes_reshuffling(ctl, B_LEVEL, c_index);
			if (rc)
				goto err_out;
			else {
			/* try copy B-node again */
			rc = tm_copy_b_node(ctl, next_index, range, c_index, &new_index);
			if (rc)
				goto err_out;
			}

			/* switch ranges between copied nodes */
			rc = tm_b_nodes_switch(ctl, next_index, new_index);
			if (rc)
				goto err_out;

			/* delete next_index B-node */
			rc = tm_delete_b_node(ctl, next_index);
			if (rc)
				goto err_out;

			/* expand range of our original parent with freed node */
			rc = tm_node_move(ctl, C_LEVEL, c_index, index);
			if (rc)
				goto err_out;

			change = (struct tm_tree_change *)tm_malloc(sizeof(struct tm_tree_change));
			change->type = TM_ENABLE;
			change->index = c_index;
			change->old_index = next_index;
			change->new_index = new_index;
			change->next = ctl->list.next;
			ctl->list.next = change;
		} else /* other error */
		{
			if (rc)
				goto err_out;
		}
		}
		break;
	default:
		return -ERANGE;
	}

err_out:
	return rc;
}


static int port_update_sw_image(tm_handle hndl,
								enum tm_port_bw port_speed,
								struct tm_port_params *params,
								uint8_t port_index)
{
	int i;
	int rc;
	struct tm_port *port = NULL;
	struct tm_drop_profile *profile;

	DECLARE_TM_CTL_PTR(ctl, hndl)
	CHECK_TM_CTL_PTR(ctl)

	port = &(ctl->tm_port_array[port_index]);

	/* Update Port SW DB */
	port->port_speed = port_speed;
	port->wred_profile_ref = params->wred_profile_ref;
	ctl->tm_p_lvl_drop_prof_ptr[port_index] = params->wred_profile_ref;
	ctl->tm_p_lvl_drop_profiles[params->wred_profile_ref].use_counter++;
	profile = &(ctl->tm_p_lvl_drop_profiles[port->wred_profile_ref]);
	rc = rm_list_add_index(ctl->rm, profile->use_list, port_index, P_LEVEL);
	if (rc)
		return TM_CONF_P_WRED_PROF_REF_OOR;

#ifdef MV_QMTM_NSS_A0
	/* DWRR for Port */
	for (i = 0; i < 8; i++)
		port->dwrr_quantum[i].quantum = params->quantum[i];
#endif
	/* DWRR for C-nodes in Port's range */
	port->dwrr_priority = 0;
	for (i = 0; i < 8; i++)
		port->dwrr_priority =
			port->dwrr_priority | (params->dwrr_priority[i] << i);

	/* Update sw image with eligible priority function pointer */
	port->elig_prio_func_ptr = params->elig_prio_func_ptr;

	return rc;
}


/**
 */
static int port_calc_ranges(uint16_t parents,
				uint32_t children,
				uint32_t *norm_range,
				uint32_t *last_range)
{
	uint32_t range;
	uint32_t remainder;

	if (parents == 1) {
		*norm_range = 0;
		*last_range = children;
		return 0;
	}

	range = children / parents;
	if (range * parents != children)
		range = range + 1;

	if (range * parents != children)
		remainder = children - range * (parents - 1);
	else
		remainder = range;

	if (remainder > range)
		return -ENOMEM;

	*norm_range = range;
	*last_range = remainder;
	return 0;
}


/**
 */
static void set_a_node_params_default(struct tm_a_node_params *params)
{
	int i;

	if (params) {
		params->quantum = 0x40;
		params->wred_profile_ref = 0;
		for (i = 0; i < 8; i++)
			params->dwrr_priority[i] = 0;
		params->elig_prio_func_ptr = 0;
	}
}


/**
 */
static void set_b_node_params_default(struct tm_b_node_params *params)
{
	int i;

	if (params) {
		params->quantum = 0x40;
		params->wred_profile_ref = 0;
		for (i = 0; i < 8; i++)
			params->dwrr_priority[i] = 0;
		params->elig_prio_func_ptr = 0;
	}
}


/**
 */
static void set_c_node_params_default(struct tm_c_node_params *params)
{
	int i;

	if (params) {
		params->quantum = 0x40;
		params->wred_cos = 0;
		for (i = 0; i < 8; i++) {
			params->dwrr_priority[i] = 0;
			params->wred_profile_ref[i] = 0;
		}
		params->elig_prio_func_ptr = 0;
	}
}

/***************************************************************************
 * Port Creation
 ***************************************************************************/

/**
 */
int tm_create_asym_port(tm_handle hndl, uint8_t port_index,
						enum tm_port_bw port_speed,
						struct tm_port_params *params)
{
	struct tm_port *port = NULL;
	struct rm_node *rm_port = NULL;
	uint8_t status;
	int rc = 0;
	int i;
#ifdef MV_QMTM_NSS_A0
	uint32_t min_port_quant;
	uint32_t max_pkg_len_bursts;
	uint8_t res_min_bw = 0;
	uint8_t res_max_bw = 0;
	uint8_t div_exp = 0;
#endif
	struct tm_drop_profile *profile = NULL;

	DECLARE_TM_CTL_PTR(ctl, hndl)
	DECLARE_RM_HANDLE(rm, ctl->rm)
	CHECK_TM_CTL_PTR(ctl);

	rc = tm_nodes_lock(TM_ENV(ctl));
	if (rc)
		return rc;

	/* Check parameters validity */
	if (port_index >= rm->rm_total_ports) {
		rc = TM_CONF_PORT_IND_OOR;
		goto out;
	}

	rc = rm_node_status(rm, P_LEVEL, port_index, &status);
	if ((rc) || (status != RM_FALSE))
	{
		rc = TM_CONF_PORT_IND_USED;
		goto out;
	}

	/* check port drop profile reference */
	if (params->wred_profile_ref > TM_NUM_PORT_DROP_PROF)
	{
		rc = TM_CONF_P_WRED_PROF_REF_OOR;
		goto out;
	}

	/* check if the referenced port drop profile exists */
	rc = rm_port_drop_profile_status(rm, params->wred_profile_ref, &status);
	if ((rc) || (status != RM_TRUE))
	{
		rc = TM_CONF_P_WRED_PROF_REF_OOR;
		goto out;
	}

	/* Port params */
	/* DWRR for Port */
#ifdef MV_QMTM_NSS_A0
	min_port_quant = 4*ctl->port_ch_emit*ctl->dwrr_bytes_burst_limit;
	max_pkg_len_bursts = (ctl->mtu + ctl->min_pkg_size)/16;
	max_pkg_len_bursts = max_pkg_len_bursts/0x40; /* in 64B units */
	if (min_port_quant < max_pkg_len_bursts)
		min_port_quant = max_pkg_len_bursts;
	min_port_quant = min_port_quant/TM_PORT_QUANTUM_UNIT; /* in 64B units */
	for (i = 0; i < 8; i++)
		if ((params->quantum[i] < min_port_quant) ||
			(params->quantum[i] > 0x1FF))
		{ /* 9 bits */
			rc = TM_CONF_PORT_QUANTUM_OOR;
			goto out;
		}
#endif
	/* DWRR for C-nodes in Port's range */
	for (i = 0; i < 8; i++) {
		if ((params->dwrr_priority[i] != TM_DISABLE) &&
			(params->dwrr_priority[i] != TM_ENABLE)) {
			rc = TM_CONF_PORT_DWRR_PRIO_OOR;
			goto out;
		}
	}

	if (params->elig_prio_func_ptr > 63)
	{ /* maximum function id 0..63 */
		rc = TM_CONF_ELIG_PRIO_FUNC_ID_OOR;
		goto out;
	}

	if ((params->num_of_children > rm->rm_c_node_cnt) ||
		(params->num_of_children == 0)) {
		rc = TM_CONF_INVALID_NUM_OF_C_NODES;
		goto out;
	}


	rc = rm_init_port(rm, port_index, params->num_of_children);
	if (rc)
		goto out;

	port = &(ctl->tm_port_array[port_index]);
	rm_port = &(rm->rm_port_array[port_index]);
	port->sym_mode = TM_DISABLE;
	port->first_child_c_node = rm_port->first_child;
	port->last_child_c_node = rm_port->last_child;
	for (i = port->first_child_c_node; i <= port->last_child_c_node; i++)
		ctl->tm_c_node_array[i].parent_port = port_index;


	port_update_sw_image(hndl, port_speed, params, port_index);
	/* Download to HW */
	rc = set_hw_port(hndl, port_index);
	if (rc < 0)
		rc = TM_HW_PORT_CONFIG_FAIL;

out:
	if (rc) {
		if (rc == TM_HW_PORT_CONFIG_FAIL) {
			rm_free_port(rm, port_index, 0);
			set_sw_port_default(ctl->tm_port_array, port_index, rm);
			ctl->tm_p_lvl_drop_prof_ptr[port_index] = 0;
			ctl->tm_p_lvl_drop_profiles[params->wred_profile_ref].use_counter--;
			profile = &(ctl->tm_p_lvl_drop_profiles[port->wred_profile_ref]);
			rm_list_del_index(rm, profile->use_list, port_index, P_LEVEL);
		}
	}
	tm_sched_unlock(TM_ENV(ctl));
	tm_nodes_unlock(TM_ENV(ctl));
	return rc;
}


/**
 */
int tm_create_port(tm_handle hndl, uint8_t port_index,
				enum tm_port_bw port_speed,
				struct tm_port_params *params,
				uint16_t num_of_c_nodes,
				uint16_t num_of_b_nodes,
				uint16_t num_of_a_nodes,
				uint32_t num_of_queues)
{
	struct tm_port *port = NULL;
	uint16_t parents;
	uint32_t children;
	uint32_t norm_range;
	uint32_t last_range;
	uint8_t status;
	int rc = 0;

	DECLARE_TM_CTL_PTR(ctl, hndl)
	DECLARE_RM_HANDLE(rm, ctl->rm)
	CHECK_TM_CTL_PTR(ctl);

	/* Check parameters validity */
	if (port_index >= rm->rm_total_ports)
	{
		rc = TM_CONF_PORT_IND_OOR;
		goto out;
	}

	rc = rm_node_status(rm, P_LEVEL, port_index, &status);
	if ((rc) || (status != RM_FALSE))
	{
		rc = TM_CONF_PORT_IND_USED;
		goto out;
	}

	if ((num_of_c_nodes > rm->rm_c_node_cnt) ||
		(num_of_c_nodes == 0))
	{
		rc = TM_CONF_INVALID_NUM_OF_C_NODES;
		goto out;
	}

	if ((num_of_b_nodes > rm->rm_b_node_cnt) ||
		(num_of_c_nodes > num_of_b_nodes))
	{
		rc = TM_CONF_INVALID_NUM_OF_B_NODES;
		goto out;
	}

	if ((num_of_a_nodes > rm->rm_a_node_cnt) ||
		(num_of_b_nodes > num_of_a_nodes))
	{
		rc = TM_CONF_INVALID_NUM_OF_A_NODES;
		goto out;
	}

	if ((num_of_queues > rm->rm_queue_cnt) ||
		(num_of_a_nodes > num_of_queues))
	{
		rc = TM_CONF_INVALID_NUM_OF_QUEUES;
		goto out;
	}

	/* Calculate ranges */
	port = &(ctl->tm_port_array[port_index]);
	params->num_of_children = num_of_c_nodes;

	/* C to B levels ranges */
	parents = num_of_c_nodes;
	children = num_of_b_nodes;

	rc = port_calc_ranges(parents, children, &norm_range, &last_range);
	if (rc < 0)
		goto out;

	port->children_range.norm_range[C_LEVEL-1] = norm_range;
	port->children_range.last_range[C_LEVEL-1] = last_range;

	/* B to A levels ranges */
	parents = num_of_b_nodes;
	children = num_of_a_nodes;

	rc = port_calc_ranges(parents, children, &norm_range, &last_range);
	if (rc < 0)
		goto out;

	port->children_range.norm_range[B_LEVEL-1] = norm_range;
	port->children_range.last_range[B_LEVEL-1] = last_range;

	/* A to Q levels ranges */
	parents = num_of_a_nodes;
	children = num_of_queues;

	rc = port_calc_ranges(parents, children, &norm_range, &last_range);
	if (rc < 0)
		goto out;

	port->children_range.norm_range[A_LEVEL-1] = norm_range;
	port->children_range.last_range[A_LEVEL-1] = last_range;

	rc = tm_create_asym_port(ctl, port_index, port_speed, params);
	if (rc)
		goto out;

	/* Set sym mode */
	port->sym_mode = TM_ENABLE;

out:
	return rc;
}


/**
 */
int tm_config_port_drop_per_cos(tm_handle hndl, uint8_t port_index,
				struct tm_port_drop_per_cos *params)
{
	struct tm_ctl *ctl = hndl;
	struct rmctl *rm = NULL;
	struct tm_port *port = NULL;
	struct tm_drop_profile *profile = NULL;
	uint8_t status;
	int rc = 0;
	int i;

	if (!ctl)
		return -EINVAL;
	if (ctl->magic != TM_MAGIC)
		return -EBADF;

	rc = tm_nodes_lock(TM_ENV(ctl));
	if (rc)
		return rc;
	rm = ctl->rm;

	/* Check if port exists */
	if (port_index >= rm->rm_total_ports) {
		rc = TM_CONF_PORT_IND_OOR;
		goto out;
	}
	rc = rm_node_status(rm, P_LEVEL, port_index, &status);
	if ((rc) || (status != RM_TRUE)) {
		rc = TM_CONF_PORT_IND_OOR;
		goto out;
	}

	/* check port drop profile reference */
	for (i = 0; i < TM_WRED_COS; i++) {
		if (params->wred_cos & (1<<i)) {
			if (params->wred_profile_ref[i] >= TM_NUM_PORT_DROP_PROF) {
				rc = TM_CONF_P_WRED_PROF_REF_OOR;
				goto out;
			} else {
				/* check if the referenced c-node drop profile exists */
				rc = rm_port_drop_profile_status_cos(rm, (uint8_t)i,
												params->
												wred_profile_ref[i],
												&status);
				if ((rc) || (status != RM_TRUE)) {
					rc = TM_CONF_P_WRED_PROF_REF_OOR;
					goto out;
				}
			}
		}
	}

	port = &(ctl->tm_port_array[port_index]);
	for (i = 0; i < TM_WRED_COS; i++) {
		/* Update Drop profile pointer */
		if (params->wred_cos & (1<<i)) {
			ctl->tm_p_lvl_drop_prof_ptr_cos[i][port_index] = params->wred_profile_ref[i];
			profile = &(ctl->tm_p_lvl_drop_profiles_cos[i][params->wred_profile_ref[i]]);
			ctl->tm_p_lvl_drop_profiles_cos[i][params->wred_profile_ref[i]].use_counter++;

			rc = rm_list_add_index(rm, profile->use_list, port_index, P_LEVEL);
			if (rc) {
				rc = TM_CONF_P_WRED_PROF_REF_OOR;
				goto out;
			}
			port->wred_profile_ref_cos[i] = params->wred_profile_ref[i];
		} else {
			ctl->tm_p_lvl_drop_prof_ptr_cos[i][port_index] = 0;
			ctl->tm_p_lvl_drop_profiles_cos[i][0].use_counter++;
			port->wred_profile_ref_cos[i] = 0;
		}
	}
	port->wred_cos = params->wred_cos;

	/* Download to HW */
	for (i = 0; i < TM_WRED_COS; i++)
	{
		rc = set_hw_port_drop_cos(hndl, port_index, (uint8_t)i);
		if (rc < 0) {
			rc = TM_HW_PORT_CONFIG_FAIL;
			goto out;
		}
	}

out:
	if (rc) {
		if (rc == TM_HW_PORT_CONFIG_FAIL) {
			for (i = 0; i < TM_WRED_COS; i++) {
				if (params->wred_cos & (1<<i)) {
					ctl->tm_p_lvl_drop_prof_ptr_cos[i][port_index] = 0;
					ctl->tm_p_lvl_drop_profiles_cos[i][params->wred_profile_ref[i]].use_counter--;
					profile = &(ctl->tm_p_lvl_drop_profiles_cos[i][port->wred_profile_ref_cos[i]]);
					rm_list_del_index(rm, profile->use_list, port_index, P_LEVEL);
					port->wred_profile_ref_cos[i] = 0;
				}
			}
			port->wred_cos = 0; /* bit map */
		}
	}
	tm_nodes_unlock(TM_ENV(ctl));
	return rc;
}


/***************************************************************************
 * Queue Creation
 ***************************************************************************/

/**
 */
int tm_create_queue_to_port(tm_handle hndl, uint8_t port_index,
							struct tm_queue_params *q_params,
							struct tm_a_node_params *a_params,
							struct tm_b_node_params *b_params,
							struct tm_c_node_params *c_params,
							uint32_t *queue_index,
							uint32_t *a_node_index,
							uint32_t *b_node_index,
							uint32_t *c_node_index)
{
	int rc = 0;

	DECLARE_TM_CTL_PTR(ctl, hndl)
	DECLARE_RM_HANDLE(rm, ctl->rm)
	CHECK_TM_CTL_PTR(ctl);

	rc = tm_create_a_node_to_port(hndl, port_index, a_params, b_params, c_params,
								  a_node_index, b_node_index, c_node_index);
	if (rc)
		return rc;

	rc = tm_create_queue_to_a_node(hndl, *a_node_index, q_params, queue_index);
	if (rc)
	{
		rm_free_a_node(rm, *a_node_index, 0);
		rm_free_b_node(rm, *b_node_index, 0);
		rm_free_c_node(rm, *c_node_index, 0);
		set_sw_a_node_default(ctl->tm_a_node_array, *a_node_index, rm);
		set_sw_b_node_default(ctl->tm_b_node_array, *b_node_index, rm);
		set_sw_c_node_default(ctl->tm_c_node_array, *c_node_index, rm);
	}
	return rc;
}


/**
 */
int tm_create_trans_queue_to_port(tm_handle hndl,
								uint8_t port_index,
								struct tm_queue_params *q_params,
								uint32_t *queue_index)
{
	struct tm_port *port = NULL;
	struct tm_c_node_params c_params;
	struct tm_b_node_params b_params;
	struct tm_a_node_params a_params;
	uint32_t c_index;
	uint32_t b_index;
	uint32_t a_index;
	uint8_t status;
	int rc = 0;
	int q_flag = TM_DISABLE;
	int p_flag = TM_DISABLE;

	DECLARE_TM_CTL_PTR(ctl, hndl)
	DECLARE_RM_HANDLE(rm, ctl->rm)
	CHECK_TM_CTL_PTR(ctl);

	/* Check parameters validity */
	if (port_index >= rm->rm_total_ports) {
		rc = TM_CONF_PORT_IND_OOR;
		goto out;
	}

	rc = rm_node_status(rm, P_LEVEL, port_index, &status);
	if ((rc) || (status != RM_TRUE)) {
		rc = TM_CONF_PORT_IND_NOT_EXIST;
		goto out;
	}

	if (ctl->tm_port_array[port_index].sym_mode != TM_ENABLE) {
		rc = -EFAULT;
		goto out;
	}

	port = &(ctl->tm_port_array[port_index]);
	c_index = port->first_child_c_node;

	/* Check that there is only one possible child in path to queues */
	if (c_index != port->last_child_c_node) {
		rc = -EFAULT;
		goto out;
	}

	if ((port->children_range.norm_range[B_LEVEL] != 0) ||
	   (port->children_range.last_range[B_LEVEL] != 1)) {
		rc = -EFAULT;
		goto out;
	}

	if ((port->children_range.norm_range[A_LEVEL] != 0) ||
	(port->children_range.last_range[A_LEVEL] != 1)) {
		rc = -EFAULT;
		goto out;
	}

	/* q_params are checked inside one of the internal calls */

	/* Find free queue */
	rc = rm_node_status(rm, C_LEVEL, c_index, &status);
	if (rc)
		goto out;
	if (status != RM_TRUE) {
		/* Transparent path C-B-A-nodes doesn't exist */
		/* Create transparent path */
		set_a_node_params_default(&a_params);
		set_b_node_params_default(&b_params);
		set_c_node_params_default(&c_params);
		rc = tm_create_a_node_to_port(ctl, port_index,
					&a_params, &b_params, &c_params,
					&a_index, &b_index, &c_index);
		if (rc)
			goto out;
		p_flag = TM_ENABLE;
		q_flag = TM_ENABLE;
	} else {
		/* Transparent path C-B-A-nodes exists */
		b_index = ctl->tm_c_node_array[c_index].first_child_b_node;
		a_index = ctl->tm_b_node_array[b_index].first_child_a_node;
		if (rm->rm_a_node_array[a_index].cnt != 0)
			q_flag = TM_ENABLE;
	}
	/* No free queue found */
	if (q_flag != TM_ENABLE) {
		rc = -ENOBUFS;
		goto out;
	}
	rc = tm_create_queue_to_a_node(hndl, a_index, q_params, queue_index);

out:
	if (rc) {
		if (p_flag == TM_ENABLE) {
			tm_delete_node(ctl, A_LEVEL, a_index);
			tm_delete_node(ctl, B_LEVEL, b_index);
			tm_delete_node(ctl, C_LEVEL, c_index);
		}
	}
	return rc;
}


/**
 */
int tm_create_queue_to_c_node(tm_handle hndl, uint32_t c_node_index,
							  struct tm_queue_params *q_params,
							  struct tm_a_node_params *a_params,
							  struct tm_b_node_params *b_params,
							  uint32_t *queue_index,
							  uint32_t *a_node_index,
							  uint32_t *b_node_index)
{
	int rc = 0;

	DECLARE_TM_CTL_PTR(ctl, hndl)
	DECLARE_RM_HANDLE(rm, ctl->rm)
	CHECK_TM_CTL_PTR(ctl);

	rc = tm_create_a_node_to_c_node(hndl, c_node_index, a_params, b_params,
									a_node_index, b_node_index);
	if (rc)
		return rc;

	rc = tm_create_queue_to_a_node(hndl, *a_node_index, q_params, queue_index);
	if (rc)
	{
		rm_free_a_node(rm, *a_node_index, 0);
		rm_free_b_node(rm, *b_node_index, 0);
		set_sw_a_node_default(ctl->tm_a_node_array, *a_node_index, rm);
		set_sw_b_node_default(ctl->tm_b_node_array, *b_node_index, rm);
	}
	return rc;
}


/**
 */
int tm_create_queue_to_b_node(tm_handle hndl, uint32_t b_node_index,
							  struct tm_queue_params *q_params,
							  struct tm_a_node_params *a_params,
							  uint32_t *queue_index,
							  uint32_t *a_node_index)
{
	int rc = 0;

	DECLARE_TM_CTL_PTR(ctl, hndl)
	DECLARE_RM_HANDLE(rm, ctl->rm)
	CHECK_TM_CTL_PTR(ctl);

	rc = tm_create_a_node_to_b_node(hndl, b_node_index, a_params, a_node_index);
	if (rc)
		return rc;

	rc = tm_create_queue_to_a_node(hndl, *a_node_index, q_params, queue_index);
	if (rc)
	{
		rm_free_a_node(rm, *a_node_index, 0);
		set_sw_a_node_default(ctl->tm_a_node_array, *a_node_index, rm);
	}
	return rc;
}


/**
 */
int tm_create_queue_to_a_node(tm_handle hndl, uint32_t a_node_index,
							struct tm_queue_params *q_params,
							uint32_t *queue_index)
{
	struct tm_queue *queue = NULL;
	uint8_t status;
	int rc = 0;

	DECLARE_TM_CTL_PTR(ctl, hndl)
	DECLARE_RM_HANDLE(rm, ctl->rm)
	CHECK_TM_CTL_PTR(ctl);

	rc = tm_nodes_lock(TM_ENV(ctl));
	if (rc)
		return rc;

	/* Check parameters validity */
	if (a_node_index >= rm->rm_total_a_nodes) {
		rc = TM_CONF_A_NODE_IND_OOR;
		goto out;
	}

	rc = rm_node_status(rm, A_LEVEL, a_node_index, &status);
	if ((rc) || (status != RM_TRUE)) {
		rc = TM_CONF_A_NODE_IND_NOT_EXIST;
		goto out;
	}

	/* Queue params */
	if (q_params->elig_prio_func_ptr > 63) { /* maximum function id 0..63 */
		rc = TM_CONF_ELIG_PRIO_FUNC_ID_OOR;
		goto out;
	}

	/* check if queue drop profile already exists */
	rc = rm_queue_drop_profile_status(rm, q_params->wred_profile_ref, &status);
	if ((rc) || (status != RM_TRUE)) {
		rc = TM_CONF_Q_WRED_PROF_REF_OOR;
		goto out;
	}
#ifdef MV_QMTM_NOT_NSS
	/* DWRR for Queue */
	if ((q_params->quantum < ctl->min_quantum)
		|| (q_params->quantum > 256 * ctl->min_quantum)) {
		rc = TM_CONF_Q_QUANTUM_OOR;
		goto out;
	}
#endif
	/* Find free Queue */
	rc = rm_find_free_queue(rm, a_node_index);
	if (rc < 0)
		goto out;
	*queue_index = rc;

	queue = &(ctl->tm_queue_array[*queue_index]);
	queue->parent_a_node = a_node_index;

	/* Update Queue SW DB */
	queue->wred_profile_ref = q_params->wred_profile_ref;
	ctl->tm_q_lvl_drop_prof_ptr[*queue_index] = q_params->wred_profile_ref;
	ctl->tm_q_lvl_drop_profiles[q_params->wred_profile_ref].use_counter++;
	queue->elig_prio_func_ptr = q_params->elig_prio_func_ptr;

	/* DWRR for Queue - update even if disabled */
	queue->dwrr_quantum = q_params->quantum;

	/* Download Queue to HW */
	rc = set_hw_queue(ctl, *queue_index);
	if (rc) {
		rc = TM_HW_QUEUE_CONFIG_FAIL;
		goto out;
	}

	rc = set_hw_queue_shaping_def(ctl, *queue_index);
	if (rc)
		rc = TM_HW_QUEUE_CONFIG_FAIL;
out:
	if (rc) {
		if (rc == TM_HW_QUEUE_CONFIG_FAIL) {
			rm_free_queue(rm, *queue_index);
			set_sw_queue_default(ctl->tm_queue_array, *queue_index, rm);
			ctl->tm_q_lvl_drop_prof_ptr[*queue_index] = 0;
			ctl->tm_q_lvl_drop_profiles[q_params->wred_profile_ref].use_counter--;
		}
	}
	tm_nodes_unlock(TM_ENV(ctl));
	return rc;
}


/***************************************************************************
 * A-node Creation
 ***************************************************************************/

/**
 */
int tm_create_a_node_to_port(tm_handle hndl, uint8_t port_index,
							 struct tm_a_node_params *a_params,
							 struct tm_b_node_params *b_params,
							 struct tm_c_node_params *c_params,
							 uint32_t *a_node_index,
							 uint32_t *b_node_index,
							 uint32_t *c_node_index)
{
	int rc = 0;

	DECLARE_TM_CTL_PTR(ctl, hndl)
	DECLARE_RM_HANDLE(rm, ctl->rm)
	CHECK_TM_CTL_PTR(ctl);

	rc = tm_create_b_node_to_port(hndl, port_index, b_params, c_params,
								  b_node_index, c_node_index);
	if (rc)
		return rc;

	rc = tm_create_a_node_to_b_node(hndl, *b_node_index, a_params,
									a_node_index);
	if (rc)
	{
		rm_free_b_node(rm, *b_node_index, 0);
		rm_free_c_node(rm, *c_node_index, 0);
		set_sw_b_node_default(ctl->tm_b_node_array, *b_node_index, rm);
		set_sw_c_node_default(ctl->tm_c_node_array, *c_node_index, rm);
	}
	return rc;
}


/**
 */
int tm_create_a_node_to_c_node(tm_handle hndl, uint32_t c_node_index,
							   struct tm_a_node_params *a_params,
							   struct tm_b_node_params *b_params,
							   uint32_t *a_node_index,
							   uint32_t *b_node_index)
{
	int rc = 0;

	DECLARE_TM_CTL_PTR(ctl, hndl)
	DECLARE_RM_HANDLE(rm, ctl->rm)
	CHECK_TM_CTL_PTR(ctl);

	rc = tm_create_b_node_to_c_node(hndl, c_node_index, b_params, b_node_index);
	if (rc)
		return rc;

	rc = tm_create_a_node_to_b_node(hndl, *b_node_index, a_params,
									a_node_index);
	if (rc)
	{
		rm_free_b_node(rm, *b_node_index, 0);
		set_sw_b_node_default(ctl->tm_b_node_array, *b_node_index, rm);
	}
	return rc;
}


/**
 */
int tm_create_a_node_to_b_node(tm_handle hndl, uint32_t b_node_index,
							   struct tm_a_node_params *a_params,
							   uint32_t *a_node_index)
{
	struct rm_node *rm_node = NULL;
	struct tm_a_node *node = NULL;
	uint8_t status;
	uint32_t range;
	uint16_t c_node_index;
	uint16_t port_index;
	int rc = 0;
	int i;

	DECLARE_TM_CTL_PTR(ctl, hndl)
	DECLARE_RM_HANDLE(rm, ctl->rm)
	CHECK_TM_CTL_PTR(ctl);

	rc = tm_nodes_lock(TM_ENV(ctl));
	if (rc)
		return rc;

	/* Check parameters validity */
	if (b_node_index >= rm->rm_total_b_nodes)
	{
		rc = TM_CONF_B_NODE_IND_OOR;
		goto out;
	}

	rc = rm_node_status(rm, B_LEVEL, b_node_index, &status);
	if ((rc) || (status != RM_TRUE))
	{
		rc = TM_CONF_B_NODE_IND_NOT_EXIST;
		goto out;
	}

	/* A-node params */
#ifdef MV_QMTM_NOT_NSS
	/* DWRR for A-node */
	if ((a_params->quantum < ctl->min_quantum)
		|| (a_params->quantum > 256 * ctl->min_quantum)) {
		rc = TM_CONF_A_QUANTUM_OOR;
		goto out;
	}
#endif
	/* DWRR for Queues in A-node's range */
	for (i = 0; i < 8; i++) {
		if ((a_params->dwrr_priority[i] != TM_DISABLE) &&
			(a_params->dwrr_priority[i] != TM_ENABLE)) {
			rc = TM_CONF_A_DWRR_PRIO_OOR;
			goto out;
		}
	}

	if (a_params->elig_prio_func_ptr > 63) { /* maximum function id 0..63 */
		rc = TM_CONF_ELIG_PRIO_FUNC_ID_OOR;
		goto out;
	}


	/* check if the referenced a-node drop profile exists */
	rc = rm_a_node_drop_profile_status(rm, a_params->wred_profile_ref, &status);
	if ((rc) || (status != RM_TRUE)) {
		rc = TM_CONF_A_WRED_PROF_REF_OOR;
		goto out;
	}

	c_node_index = ctl->tm_b_node_array[b_node_index].parent_c_node;
	port_index = ctl->tm_c_node_array[c_node_index].parent_port;
	if (ctl->tm_port_array[port_index].sym_mode == TM_ENABLE) /* symetric mode under the port */ {
		if (rm->rm_b_node_array[b_node_index].cnt == 1) /* the A node is the last child in range */
			range = ctl->tm_port_array[port_index].children_range.last_range[A_LEVEL-1];
		else
			range = ctl->tm_port_array[port_index].children_range.norm_range[A_LEVEL-1];
	} else /* asymetric mode under the port */
		range = a_params->num_of_children;

	if ((range > rm->rm_queue_cnt) || (range == 0)) {
		rc = TM_CONF_INVALID_NUM_OF_QUEUES;
		goto out;
	}

	/* Find free A-node */
	rc = rm_find_free_a_node(rm, b_node_index, range);
	if (rc == -ENOBUFS) {
		rc = tm_nodes_reshuffling(ctl, A_LEVEL, b_node_index);
		if (rc < 0)
			goto out;
		rc = rm_find_free_a_node(rm, b_node_index, range);
	}
	if (rc < 0)
		goto out;
	*a_node_index = rc;

	node = &(ctl->tm_a_node_array[*a_node_index]);
	rm_node = &(rm->rm_a_node_array[*a_node_index]);
	node->first_child_queue = rm_node->first_child;
	node->last_child_queue = rm_node->last_child;
	for (i = node->first_child_queue; i <= node->last_child_queue; i++)
		ctl->tm_queue_array[i].parent_a_node = *a_node_index;
	node = &(ctl->tm_a_node_array[*a_node_index]);

	/* Update A-Node SW DB */
	/* Update Drop profile pointer */
	ctl->tm_a_lvl_drop_prof_ptr[*a_node_index] = a_params->wred_profile_ref;
	node->wred_profile_ref = a_params->wred_profile_ref;
	ctl->tm_a_lvl_drop_profiles[a_params->wred_profile_ref].use_counter++;
	node->elig_prio_func_ptr = a_params->elig_prio_func_ptr;

	/* DWRR for A-node - update even if disabled */
	node->dwrr_quantum = a_params->quantum;

	/* DWRR for Queues in A-node's range */
	node->dwrr_priority = 0;
	for (i = 0; i < 8; i++)
		node->dwrr_priority =
			node->dwrr_priority | (a_params->dwrr_priority[i] << i);

	/* Download A-Node to HW */
	rc = set_hw_a_node(hndl, *a_node_index);
	if (rc) {
		rc = TM_HW_A_NODE_CONFIG_FAIL;
		goto out;
	}

	rc = set_hw_a_node_shaping_def(hndl, *a_node_index);
	if (rc)
		rc = TM_HW_SHAPING_PROF_FAILED;

out:
	if (rc)
	{
		if (rc == TM_HW_A_NODE_CONFIG_FAIL)
		{
			rm_free_a_node(rm, *a_node_index, 0);
			set_sw_a_node_default(ctl->tm_a_node_array, *a_node_index, rm);
			ctl->tm_a_lvl_drop_prof_ptr[*a_node_index] = 0;
			ctl->tm_a_lvl_drop_profiles[a_params->wred_profile_ref].use_counter--;
		}
	}
	tm_nodes_unlock(TM_ENV(ctl));
	return rc;
}


/***************************************************************************
 * B-node Creation
 ***************************************************************************/

/**
 */
int tm_create_b_node_to_port(tm_handle hndl, uint8_t port_index,
							 struct tm_b_node_params *b_params,
							 struct tm_c_node_params *c_params,
							 uint32_t *b_node_index,
							 uint32_t *c_node_index)
{
	int rc = 0;

	DECLARE_TM_CTL_PTR(ctl, hndl)
	DECLARE_RM_HANDLE(rm, ctl->rm)
	CHECK_TM_CTL_PTR(ctl);

	rc = tm_create_c_node_to_port(hndl, port_index, c_params, c_node_index);
	if (rc)
		return rc;

	rc = tm_create_b_node_to_c_node(hndl, *c_node_index, b_params,
									b_node_index);
	if (rc)
	{
		rm_free_c_node(rm, *c_node_index, 0);
		set_sw_c_node_default(ctl->tm_c_node_array, *c_node_index, rm);
	}
	return rc;
}


/**
 */
int tm_create_b_node_to_c_node(tm_handle hndl, uint32_t c_node_index,
							   struct tm_b_node_params *b_params,
							   uint32_t *b_node_index)
{
	struct rm_node *rm_node = NULL;
	struct tm_b_node *node = NULL;
	uint8_t status;
	uint16_t port_index;
	uint32_t range;
	int rc = 0;
	int i;

	DECLARE_TM_CTL_PTR(ctl, hndl)
	DECLARE_RM_HANDLE(rm, ctl->rm)
	CHECK_TM_CTL_PTR(ctl);

	rc = tm_nodes_lock(TM_ENV(ctl));
	if (rc)
		return rc;

	/* Check parameters validity */
	if (c_node_index >= rm->rm_total_c_nodes)
	{
		rc = TM_CONF_C_NODE_IND_OOR;
		goto out;
	}

	rc = rm_node_status(rm, C_LEVEL, c_node_index, &status);
	if ((rc) || (status != RM_TRUE))
	{
		rc = TM_CONF_C_NODE_IND_NOT_EXIST;
		goto out;
	}

	/* B-node params */
#ifdef MV_QMTM_NOT_NSS
	/* DWRR for B-node */
	if ((b_params->quantum < ctl->min_quantum)
		|| (b_params->quantum > 256 * ctl->min_quantum)) {
		rc = TM_CONF_B_QUANTUM_OOR;
		goto out;
	}
#endif
	/* DWRR for A-nodes in B-node's range */
	for (i = 0; i < 8; i++) {
		if ((b_params->dwrr_priority[i] != TM_DISABLE) &&
			(b_params->dwrr_priority[i] != TM_ENABLE)) {
			rc = TM_CONF_B_DWRR_PRIO_OOR;
			goto out;
		}
	}

	if (b_params->wred_profile_ref >= TM_NUM_B_NODE_DROP_PROF) {
		rc = TM_CONF_B_WRED_PROF_REF_OOR;
		goto out;
	}

	if (b_params->elig_prio_func_ptr > 63) { /* maximum function id 0..63 */
		rc = TM_CONF_ELIG_PRIO_FUNC_ID_OOR;
		goto out;
	}

	/* check if the referenced b-node drop profile exists */
	rc = rm_b_node_drop_profile_status(rm, b_params->wred_profile_ref, &status);
	if ((rc) || (status != RM_TRUE)) {
		rc = TM_CONF_B_WRED_PROF_REF_OOR;
		goto out;
	}

	port_index = ctl->tm_c_node_array[c_node_index].parent_port;
	if (ctl->tm_port_array[port_index].sym_mode == TM_ENABLE) /* symetric mode under the port */ {
		if (rm->rm_c_node_array[c_node_index].cnt == 1) /* the B node is the last child in range */
			range = ctl->tm_port_array[port_index].children_range.last_range[B_LEVEL-1];
		else
			range = ctl->tm_port_array[port_index].children_range.norm_range[B_LEVEL-1];
	} else /* asymetric mode under the port */
		range = b_params->num_of_children;
	if ((range > rm->rm_a_node_cnt) || (range == 0)) {
		rc = TM_CONF_INVALID_NUM_OF_A_NODES;
		goto out;
	}

	/* Find free B-node */
	rc = rm_find_free_b_node(rm, c_node_index, range);
	if (rc == -ENOBUFS) {
		rc = tm_nodes_reshuffling(ctl, B_LEVEL, c_node_index);
		if (rc < 0)
			goto out;
		rc = rm_find_free_b_node(rm, c_node_index, range);
	}
	if (rc < 0)
		goto out;
	*b_node_index = rc;

	node = &(ctl->tm_b_node_array[*b_node_index]);
	rm_node = &(rm->rm_b_node_array[*b_node_index]);
	node->first_child_a_node = rm_node->first_child;
	node->last_child_a_node = rm_node->last_child;
	for (i = node->first_child_a_node; i <= node->last_child_a_node; i++)
		ctl->tm_a_node_array[i].parent_b_node = *b_node_index;

	/* Update B-Node SW DB */
	node->elig_prio_func_ptr = b_params->elig_prio_func_ptr;

	/* Update Drop profile pointer */
	ctl->tm_b_lvl_drop_prof_ptr[*b_node_index] = b_params->wred_profile_ref;
	node->wred_profile_ref = b_params->wred_profile_ref;
	ctl->tm_b_lvl_drop_profiles[b_params->wred_profile_ref].use_counter++;

	/* DWRR for B-node - update even if disabled */
	node->dwrr_quantum = b_params->quantum;

	/* DWRR for A-nodes in B-node's range */
	node->dwrr_priority = 0;
	for (i = 0; i < 8; i++)
		node->dwrr_priority =
			node->dwrr_priority | (b_params->dwrr_priority[i] << i);

	/* Download B-Node to HW */
	rc = set_hw_b_node(hndl, *b_node_index);
	if (rc) {
		rc = TM_HW_B_NODE_CONFIG_FAIL;
		goto out;
	}

	rc = set_hw_b_node_shaping_def(hndl, *b_node_index);
	if (rc)
		rc = TM_HW_SHAPING_PROF_FAILED;

out:
	if (rc) {
		if (rc == TM_HW_B_NODE_CONFIG_FAIL) {
			rm_free_b_node(rm, *b_node_index, 0);
			set_sw_b_node_default(ctl->tm_b_node_array, *b_node_index, rm);
			ctl->tm_b_lvl_drop_prof_ptr[*b_node_index] = 0;
			ctl->tm_b_lvl_drop_profiles[b_params->wred_profile_ref].use_counter--;
		}
	}
	tm_nodes_unlock(TM_ENV(ctl));
	return rc;
}


/***************************************************************************
 * C-node Creation
 ***************************************************************************/

/**
 */
int tm_create_c_node_to_port(tm_handle hndl, uint8_t port_index,
							 struct tm_c_node_params *c_params,
							 uint32_t *c_node_index)
{
	struct rm_node *rm_node = NULL;
	struct tm_c_node *node = NULL;
	uint8_t status;
	uint16_t range;
	int rc = 0;
	int i;

	DECLARE_TM_CTL_PTR(ctl, hndl)
	DECLARE_RM_HANDLE(rm, ctl->rm)
	CHECK_TM_CTL_PTR(ctl);

	rc = tm_nodes_lock(TM_ENV(ctl));
	if (rc)
		return rc;
	/* Check parameters validity */
	if (port_index >= rm->rm_total_ports) {
		rc = TM_CONF_PORT_IND_OOR;
		goto out;
	}

	rc = rm_node_status(rm, P_LEVEL, port_index, &status);
	if ((rc) || (status != RM_TRUE)) {
		rc = TM_CONF_PORT_IND_NOT_EXIST;
		goto out;
	}

	/* C-node params */
	if (ctl->tm_port_array[port_index].sym_mode == TM_ENABLE) /* symetric mode under the port */ {
		if (rm->rm_port_array[port_index].cnt == 1) /* the C node is the last child in range */
			range = ctl->tm_port_array[port_index].children_range.last_range[C_LEVEL-1];
		else
			range = ctl->tm_port_array[port_index].children_range.norm_range[C_LEVEL-1];
	} else /* asymetric mode under the port */
		range = c_params->num_of_children;
	if ((range > rm->rm_b_node_cnt) || (range == 0)) {
		rc = TM_CONF_INVALID_NUM_OF_B_NODES;
		goto out;
	}
#ifdef MV_QMTM_NOT_NSS
	/* DWRR for C-node */
	if ((c_params->quantum < ctl->min_quantum)
		|| (c_params->quantum > 256 * ctl->min_quantum)) {
		rc = TM_CONF_C_QUANTUM_OOR;
		goto out;
	}
#endif
	if (c_params->elig_prio_func_ptr > 63) { /* maximum function id 0..63 */
		rc = TM_CONF_ELIG_PRIO_FUNC_ID_OOR;
		goto out;
	}

	/* DWRR for B-nodes in C-node's range */
	for (i = 0; i < 8; i++) {
		if ((c_params->dwrr_priority[i] != TM_DISABLE) &&
			(c_params->dwrr_priority[i] != TM_ENABLE)) {
			rc = TM_CONF_C_DWRR_PRIO_OOR;
			goto out;
		}
	}

	for (i = 0; i < TM_WRED_COS; i++) {
		if (c_params->wred_cos & (1<<i)) {
			if (c_params->wred_profile_ref[i] >= TM_NUM_C_NODE_DROP_PROF) {
				rc = TM_CONF_C_WRED_PROF_REF_OOR;
				goto out;
			} else {
				/* check if the referenced c-node drop profile exists */
				rc = rm_c_node_drop_profile_status(rm, (uint8_t)i,
												   c_params->
												   wred_profile_ref[i],
												   &status);
				if ((rc) || (status != RM_TRUE)) {
					rc = TM_CONF_C_WRED_PROF_REF_OOR;
					goto out;
				}
			}
		}
	}

	/* Find free C-node */
	rc = rm_find_free_c_node(rm, port_index, range);
	if (rc < 0)
		goto out;
	*c_node_index = rc;

	node = &(ctl->tm_c_node_array[*c_node_index]);
	rm_node = &(rm->rm_c_node_array[*c_node_index]);
	node->first_child_b_node = rm_node->first_child;
	node->last_child_b_node = rm_node->last_child;
	for (i = node->first_child_b_node; i <= node->last_child_b_node; i++)
		ctl->tm_b_node_array[i].parent_c_node = *c_node_index;

	/* Update C-node SW DB */
	node->elig_prio_func_ptr = c_params->elig_prio_func_ptr;


	for (i = 0; i < TM_WRED_COS; i++) {
		/* Update Drop profile pointer */
		if (c_params->wred_cos & (1<<i)) {
			ctl->tm_c_lvl_drop_prof_ptr[i][*c_node_index] = c_params->wred_profile_ref[i];
			ctl->tm_c_lvl_drop_profiles[i][c_params->wred_profile_ref[i]].use_counter++;
			node->wred_profile_ref[i] = c_params->wred_profile_ref[i];
		} else {
			ctl->tm_c_lvl_drop_prof_ptr[i][*c_node_index] = 0;
			ctl->tm_c_lvl_drop_profiles[i][0].use_counter++;
			node->wred_profile_ref[i] = 0;
		}
	}

	/* update Wred CoS for future usage within the HW update */
	node->wred_cos = c_params->wred_cos;

	/* DWRR for C-node - update even if disabled */
	node->dwrr_quantum = c_params->quantum;

	/* DWRR for B-nodes in C-node's range */
	node->dwrr_priority = 0;
	for (i = 0; i < 8; i++)
		node->dwrr_priority =
			node->dwrr_priority | (c_params->dwrr_priority[i] << i);

	/* Download C-node to HW */
	rc = set_hw_c_node(hndl, *c_node_index);
	if (rc) {
		rc = TM_HW_C_NODE_CONFIG_FAIL;
		goto out;
	}

	rc = set_hw_c_node_shaping_def(hndl, *c_node_index);
	if (rc)
		rc = TM_HW_SHAPING_PROF_FAILED;

out:
	if (rc) {
		if (rc == TM_HW_C_NODE_CONFIG_FAIL) {
			rm_free_c_node(rm, *c_node_index, 0);
			set_sw_c_node_default(ctl->tm_c_node_array, *c_node_index, rm);
			for (i = 0; i < TM_WRED_COS; i++) {
				if (c_params->wred_cos & (1<<i)) {
					ctl->tm_c_lvl_drop_prof_ptr[i][*c_node_index] = 0;
					ctl->tm_c_lvl_drop_profiles[i][c_params->wred_profile_ref[i]].use_counter--;
				}
			}
		}
	}
	tm_nodes_unlock(TM_ENV(ctl));
	return rc;
}
