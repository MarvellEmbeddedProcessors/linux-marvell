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

#include "tm_nodes_reorder.h"
#include "tm_errcodes.h"
#include "rm_internal_types.h"
#include "rm_reorder.h"
#include "rm_status.h"
#include "set_hw_registers.h"
#include "tm_locking_interface.h"

/**
 */
static int tm_reorder_check_node(tm_handle hndl, enum tm_level level, uint16_t node)
{
	DECLARE_TM_CTL_PTR(ctl, hndl)
	DECLARE_RM_HANDLE(rm, ctl->rm)
	CHECK_TM_CTL_PTR(ctl);

	switch (level) {
	case A_LEVEL:
		if (node >= rm->rm_total_a_nodes)
			return TM_CONF_A_NODE_IND_OOR;
		break;
	case B_LEVEL:
		if (node >= rm->rm_total_b_nodes)
			return TM_CONF_B_NODE_IND_OOR;
		break;
	case C_LEVEL:
		if (node >= rm->rm_total_c_nodes)
			return TM_CONF_C_NODE_IND_OOR;
		break;
	case P_LEVEL:
		if (node >= rm->rm_total_ports)
			return TM_CONF_PORT_IND_OOR;
		break;
	default:
		return -ERANGE;
	}
	return 0;
}


/**
 */
static int tm_reorder_get_node_children(tm_handle hndl,
										enum tm_level level,
										uint16_t node_index,
										uint16_t *first_child_node,
										uint16_t *last_child_node)
{
	DECLARE_TM_CTL_PTR(ctl, hndl)
	CHECK_TM_CTL_PTR(ctl)

	switch (level) {
	case A_LEVEL:
		*first_child_node = ctl->tm_a_node_array[node_index].first_child_queue;
		*last_child_node = ctl->tm_a_node_array[node_index].last_child_queue;
		break;
	case B_LEVEL:
		*first_child_node = (uint16_t) ctl->tm_b_node_array[node_index].first_child_a_node;
		*last_child_node = (uint16_t) ctl->tm_b_node_array[node_index].last_child_a_node;
		break;
	case C_LEVEL:
		*first_child_node = (uint16_t) ctl->tm_c_node_array[node_index].first_child_b_node;
		*last_child_node = (uint16_t) ctl->tm_c_node_array[node_index].last_child_b_node;
		break;
	case P_LEVEL:
		*first_child_node = (uint16_t) ctl->tm_port_array[node_index].first_child_c_node;
		*last_child_node = (uint16_t) ctl->tm_port_array[node_index].last_child_c_node;
		break;
	default:
		return -ERANGE;
	}
	return 0;
}


/**
 */
static int tm_reorder_nodes_on_off(tm_handle hndl,
								   enum tm_level level,
								   uint16_t parent_node,
								   uint16_t first_child_node,
								   uint16_t last_child_node,
								   uint8_t enable)
{
	int rc;
	uint16_t i;
	uint8_t elig_status;

	DECLARE_TM_CTL_PTR(ctl, hndl)
	CHECK_TM_CTL_PTR(ctl)

	if (enable) {
		rc = set_hw_deq_status(hndl, level, parent_node);
		if (rc)
			return rc;
		for (i = first_child_node; i <= last_child_node; i++) {
			rc = set_hw_deq_status(hndl, (enum tm_level)(level-1), i);
			if (rc)
				return rc;
		}
	} else { /* Disable all DeQ */
		switch (level) {
		case A_LEVEL:
			elig_status = ctl->tm_a_node_array[parent_node].elig_prio_func_ptr;
			ctl->tm_a_node_array[parent_node].elig_prio_func_ptr = TM_ELIG_DEQ_DISABLE;
			rc = set_hw_deq_status(ctl, level, parent_node);
			if (rc)
				return rc;
			ctl->tm_a_node_array[parent_node].elig_prio_func_ptr = elig_status;

			for (i = first_child_node; i <= last_child_node; i++) {
				elig_status = ctl->tm_queue_array[i].elig_prio_func_ptr;
				ctl->tm_queue_array[i].elig_prio_func_ptr = TM_ELIG_DEQ_DISABLE;
				rc = set_hw_deq_status(ctl, Q_LEVEL, i);
				if (rc)
					return rc;
				ctl->tm_queue_array[i].elig_prio_func_ptr = elig_status;
			}
			break;
		case B_LEVEL:
			elig_status = ctl->tm_b_node_array[parent_node].elig_prio_func_ptr;
			ctl->tm_b_node_array[parent_node].elig_prio_func_ptr = TM_ELIG_DEQ_DISABLE;
			rc = set_hw_deq_status(ctl, level, parent_node);
			if (rc)
				return rc;
			ctl->tm_b_node_array[parent_node].elig_prio_func_ptr = elig_status;

			for (i = first_child_node; i <= last_child_node; i++) {
				elig_status = ctl->tm_a_node_array[i].elig_prio_func_ptr;
				ctl->tm_a_node_array[i].elig_prio_func_ptr = TM_ELIG_DEQ_DISABLE;
				rc = set_hw_deq_status(ctl, A_LEVEL, i);
				if (rc)
					return rc;
				ctl->tm_a_node_array[i].elig_prio_func_ptr = elig_status;
			}
			break;
		case C_LEVEL:
			elig_status = ctl->tm_c_node_array[parent_node].elig_prio_func_ptr;
			ctl->tm_c_node_array[parent_node].elig_prio_func_ptr = TM_ELIG_DEQ_DISABLE;
			rc = set_hw_deq_status(ctl, level, parent_node);
			if (rc)
				return rc;
			ctl->tm_c_node_array[parent_node].elig_prio_func_ptr = elig_status;

			for (i = first_child_node; i <= last_child_node; i++) {
				elig_status = ctl->tm_b_node_array[i].elig_prio_func_ptr;
				ctl->tm_b_node_array[i].elig_prio_func_ptr = TM_ELIG_DEQ_DISABLE;
				rc = set_hw_deq_status(ctl, B_LEVEL, i);
				if (rc)
					return rc;
				ctl->tm_b_node_array[i].elig_prio_func_ptr = elig_status;
			}
			break;
		case P_LEVEL:
			elig_status = ctl->tm_port_array[parent_node].elig_prio_func_ptr;
			ctl->tm_port_array[parent_node].elig_prio_func_ptr = TM_ELIG_DEQ_DISABLE;
			rc = set_hw_deq_status(ctl, level, parent_node);
			if (rc)
				return rc;
			ctl->tm_port_array[parent_node].elig_prio_func_ptr = elig_status;

			for (i = first_child_node; i <= last_child_node; i++) {
				elig_status = ctl->tm_c_node_array[i].elig_prio_func_ptr;
				ctl->tm_c_node_array[i].elig_prio_func_ptr = TM_ELIG_DEQ_DISABLE;
				rc = set_hw_deq_status(ctl, C_LEVEL, i);
				if (rc)
					return rc;
				ctl->tm_c_node_array[i].elig_prio_func_ptr = elig_status;
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
int tm_nodes_move(tm_handle hndl,
		enum tm_level level,
		uint16_t number_of_children,
		uint16_t from_node,
		uint16_t to_node)
{
	int rc;
	int i;
	int direction = 0;
	uint8_t status;
	uint16_t first_child_node_from_node;
	uint16_t last_child_node_from_node;
	uint16_t first_child_node_to_node;
	uint16_t last_child_node_to_node;
	uint16_t first_child_to_move;

	DECLARE_TM_CTL_PTR(ctl, hndl)
	DECLARE_RM_HANDLE(rm, ctl->rm)
	CHECK_TM_CTL_PTR(ctl);

	if ((level < A_LEVEL) || (level > P_LEVEL))
		return -ERANGE;

	rc = tm_reorder_check_node(hndl, level, from_node);
	if (rc)
		return rc;

	rc = tm_reorder_check_node(hndl, level, to_node);
	if (rc)
		return rc;

	rc = tm_nodes_lock(TM_ENV(ctl));
	if (rc)
		return rc;

	/* Check SW model that operation is possible */
	rc = rm_node_status(rm, level, from_node, &status);
	if (rc || (status != RM_TRUE))
		goto err_out;

	rc = rm_node_status(rm, level, to_node, &status);
	if (rc || (status != RM_TRUE))
		goto err_out;

	rc = tm_reorder_get_node_children(hndl, level, from_node,
									  &first_child_node_from_node,
									  &last_child_node_from_node);
	if (rc)
		goto err_out;

	rc = tm_reorder_get_node_children(hndl, level, to_node,
									  &first_child_node_to_node,
									  &last_child_node_to_node);
	if (rc)
		goto err_out;

	/* Check special node modification conditions */
	if (last_child_node_from_node - first_child_node_from_node < number_of_children)
	{
		rc = TM_CONF_REORDER_CHILDREN_NOT_AVAIL;
		goto err_out;
	}

	/* Ensure the children of the from_node and to_node are adjecent */
	if (last_child_node_from_node + 1 == first_child_node_to_node)
		direction = 1;
	else if (last_child_node_to_node + 1 == first_child_node_from_node)
		direction = -1;
	if (!direction)
	{
		rc = TM_CONF_REORDER_NODES_NOT_ADJECENT;
		goto err_out;
	}

	/* Calculate new values for parent nodes */
	if (direction > 0)
	{
		first_child_to_move = first_child_node_to_node - number_of_children;
		first_child_node_to_node = first_child_to_move;
		last_child_node_from_node = first_child_to_move - 1;
	} else {
		first_child_to_move = first_child_node_from_node;
		first_child_node_from_node = first_child_to_move + number_of_children;
		last_child_node_to_node = first_child_node_from_node - 1;
	}


	/* Disable dequeing on all child and parent node */
	rc = tm_reorder_nodes_on_off(hndl, level, from_node, first_child_to_move,
		(uint16_t)(first_child_to_move + number_of_children - 1), TM_DISABLE);
	if (rc)
		goto err_out;

	rc = tm_reorder_nodes_on_off(hndl, level, to_node, 0, 0, TM_DISABLE);
	if (rc)
		goto err_out;

	/* Move children, reconfigure parents */
	switch (level)
	{
	case A_LEVEL:
		/* RM */
		rc = rm_nodes_move(rm, RM_Q_LVL, from_node, to_node, number_of_children, first_child_to_move);
		if (rc)
			goto err_out;

		/* TM */
		ctl->tm_a_node_array[from_node].first_child_queue = first_child_node_from_node;
		ctl->tm_a_node_array[from_node].last_child_queue = last_child_node_from_node;
		rc = set_hw_map(ctl, level, from_node);
		if (rc)
			goto err_out;

		ctl->tm_a_node_array[to_node].first_child_queue = first_child_node_to_node;
		ctl->tm_a_node_array[to_node].last_child_queue = last_child_node_to_node;
		rc = set_hw_map(ctl, level, to_node);
		if (rc)
			goto err_out;

		for (i = first_child_to_move; i < first_child_to_move + number_of_children; i++)
		{
			ctl->tm_queue_array[i].parent_a_node = to_node;
			rc = set_hw_map(ctl, Q_LEVEL, i);
			if (rc)
				goto err_out;
		}
		break;
	case B_LEVEL:
		/* RM */
		rc = rm_nodes_move(rm, RM_A_LVL, from_node, to_node, number_of_children, first_child_to_move);
		if (rc)
			goto err_out;

		/* TM */
		ctl->tm_b_node_array[from_node].first_child_a_node = first_child_node_from_node;
		ctl->tm_b_node_array[from_node].last_child_a_node = last_child_node_from_node;
		rc = set_hw_map(ctl, level, from_node);
		if (rc)
			goto err_out;

		ctl->tm_b_node_array[to_node].first_child_a_node = first_child_node_to_node;
		ctl->tm_b_node_array[to_node].last_child_a_node = last_child_node_to_node;
		rc = set_hw_map(ctl, level, to_node);
		if (rc)
			goto err_out;

		for (i = first_child_to_move; i < first_child_to_move + number_of_children; i++)
		{
			ctl->tm_a_node_array[i].parent_b_node = to_node;
			rc = set_hw_map(ctl, A_LEVEL, i);
			if (rc)
				goto err_out;
		}
		break;
	case C_LEVEL:
		/* RM */
		rc = rm_nodes_move(rm, RM_B_LVL, from_node, to_node, number_of_children, first_child_to_move);
		if (rc)
			goto err_out;

		/* TM */
		ctl->tm_c_node_array[from_node].first_child_b_node = first_child_node_from_node;
		ctl->tm_c_node_array[from_node].last_child_b_node = last_child_node_from_node;
		rc = set_hw_map(ctl, level, from_node);
		if (rc)
			goto err_out;

		ctl->tm_c_node_array[to_node].first_child_b_node = first_child_node_to_node;
		ctl->tm_c_node_array[to_node].last_child_b_node = last_child_node_to_node;
		rc = set_hw_map(ctl, level, to_node);
		if (rc)
			goto err_out;

		for (i = first_child_to_move; i < first_child_to_move + number_of_children; i++)
		{
			ctl->tm_b_node_array[i].parent_c_node = to_node;
			rc = set_hw_map(ctl, B_LEVEL, i);
			if (rc)
				goto err_out;
		}
		break;
	case P_LEVEL:
		/* RM */
		rc = rm_nodes_move(rm, RM_C_LVL, from_node, to_node, number_of_children, first_child_to_move);
		if (rc)
			goto err_out;

		/* TM */
		ctl->tm_port_array[from_node].first_child_c_node = first_child_node_from_node;
		ctl->tm_port_array[from_node].last_child_c_node = last_child_node_from_node;
		rc = set_hw_map(ctl, level, from_node);
		if (rc)
			goto err_out;

		ctl->tm_port_array[to_node].first_child_c_node = first_child_node_to_node;
		ctl->tm_port_array[to_node].last_child_c_node = last_child_node_to_node;
		rc = set_hw_map(ctl, level, to_node);
		if (rc)
			goto err_out;

		for (i = first_child_to_move; i < first_child_to_move + number_of_children; i++)
		{
			ctl->tm_c_node_array[i].parent_port = (uint8_t)to_node;
			rc = set_hw_map(ctl, C_LEVEL, i);
			if (rc)
				goto err_out;
		}
		break;
	default:
		return -EINVAL;
	}

	/* Enable all nodes again in HW from SW model*/
	rc = tm_reorder_nodes_on_off(hndl, level, from_node, first_child_to_move,
		(uint16_t)(first_child_to_move + number_of_children - 1), TM_ENABLE);
	if (rc)
		goto err_out;

	rc = tm_reorder_nodes_on_off(hndl, level, to_node, 0, 0, TM_ENABLE);

err_out:
	tm_nodes_unlock(TM_ENV(ctl));
	return rc;
}


/**
 */
int tm_nodes_switch(tm_handle hndl,
					enum tm_level level,
					uint16_t node_a,
					uint16_t node_b)
{
	int rc, i;
	uint8_t status;
	uint16_t first_child_node_a;
	uint16_t last_child_node_a;
	uint16_t first_child_node_b;
	uint16_t last_child_node_b;

	DECLARE_TM_CTL_PTR(ctl, hndl)
	DECLARE_RM_HANDLE(rm, ctl->rm)
	CHECK_TM_CTL_PTR(ctl);

	if ((level < A_LEVEL) || (level > P_LEVEL))
		return -ERANGE;

	rc = tm_reorder_check_node(hndl, level, node_a);
	if (rc)
		return rc;

	rc = tm_reorder_check_node(hndl, level, node_b);
	if (rc)
		return rc;

	rc = tm_nodes_lock(TM_ENV(ctl));
	if (rc)
		return rc;

	/* Check SW model that operation is possible */
	rc = rm_node_status(rm, level, node_a, &status);
	if (rc || (status != RM_TRUE))
		goto err_out;

	rc = rm_node_status(rm, level, node_b, &status);
	if (rc || (status != RM_TRUE))
		goto err_out;

	/* Retrieve values for both parents on children used */
	rc = tm_reorder_get_node_children(hndl, level, node_a, &first_child_node_a, &last_child_node_a);
	if (rc)
		goto err_out;

	rc = tm_reorder_get_node_children(hndl, level, node_b, &first_child_node_b, &last_child_node_b);
	if (rc)
		goto err_out;

	/* Disable dequeing on all child and parent node */
	rc = tm_reorder_nodes_on_off(hndl, level, node_a, first_child_node_a, last_child_node_a, TM_DISABLE);
	if (rc)
		goto err_out;

	rc = tm_reorder_nodes_on_off(hndl, level, node_b, first_child_node_b, last_child_node_b, TM_DISABLE);
	if (rc)
		goto err_out;

	/* Move children, switch parents */
	switch (level)
	{
	case A_LEVEL:
		rc  = rm_nodes_switch(rm, RM_Q_LVL, node_a, node_b,
							  first_child_node_a, last_child_node_a,
							  first_child_node_b, last_child_node_b);
		if (rc)
			goto err_out;

		/* Update node_a... */
		ctl->tm_a_node_array[node_a].first_child_queue = first_child_node_b;
		ctl->tm_a_node_array[node_a].last_child_queue = last_child_node_b;
		rc = set_hw_map(ctl, level, node_a);
		if (rc)
			goto err_out;

		for (i = first_child_node_b; i <= last_child_node_b ; i++)
		{
			ctl->tm_queue_array[i].parent_a_node = node_a;
			rc = set_hw_map(ctl, Q_LEVEL, i);
			if (rc)
				goto err_out;
		}

		/* Update node_b... */
		ctl->tm_a_node_array[node_b].first_child_queue = first_child_node_a;
		ctl->tm_a_node_array[node_b].last_child_queue = last_child_node_a;
		rc = set_hw_map(ctl, level, node_b);
		if (rc)
			goto err_out;

		for (i = first_child_node_a; i <= last_child_node_a ; i++)
		{
			ctl->tm_queue_array[i].parent_a_node = node_b;
			rc = set_hw_map(ctl, Q_LEVEL, i);
			if (rc)
				goto err_out;
		}
		break;
	case B_LEVEL:
		rc  = rm_nodes_switch(rm, RM_A_LVL, node_a, node_b,
							  first_child_node_a, last_child_node_a,
							  first_child_node_b, last_child_node_b);
		if (rc)
			goto err_out;

		/* Update node_a... */
		ctl->tm_b_node_array[node_a].first_child_a_node = first_child_node_b;
		ctl->tm_b_node_array[node_a].last_child_a_node = last_child_node_b;
		rc = set_hw_map(ctl, level, node_a);
		if (rc)
			goto err_out;

		for (i = first_child_node_b; i <= last_child_node_b ; i++)
		{
			ctl->tm_a_node_array[i].parent_b_node = node_a;
			rc = set_hw_map(ctl, A_LEVEL, i);
			if (rc)
				goto err_out;
		}

		/* Update node_b... */
		ctl->tm_b_node_array[node_b].first_child_a_node = first_child_node_a;
		ctl->tm_b_node_array[node_b].last_child_a_node = last_child_node_a;
		rc = set_hw_map(ctl, level, node_b);
		if (rc)
			goto err_out;

		for (i = first_child_node_a; i <= last_child_node_a ; i++)
		{
			ctl->tm_a_node_array[i].parent_b_node = node_b;
			rc = set_hw_map(ctl, A_LEVEL, i);
			if (rc)
				goto err_out;
		}
		break;
	case C_LEVEL:
		rc  = rm_nodes_switch(rm, RM_B_LVL, node_a, node_b,
							  first_child_node_a, last_child_node_a,
							  first_child_node_b, last_child_node_b);
		if (rc)
			goto err_out;

		/* Update node_a... */
		ctl->tm_c_node_array[node_a].first_child_b_node = first_child_node_b;
		ctl->tm_c_node_array[node_a].last_child_b_node = last_child_node_b;
		rc = set_hw_map(ctl, level, node_a);
		if (rc)
			goto err_out;

		for (i = first_child_node_b; i <= last_child_node_b ; i++)
		{
			ctl->tm_b_node_array[i].parent_c_node = node_a;
			rc = set_hw_map(ctl, B_LEVEL, i);
			if (rc)
				goto err_out;
		}

		/* Update node_b... */
		ctl->tm_c_node_array[node_b].first_child_b_node = first_child_node_a;
		ctl->tm_c_node_array[node_b].last_child_b_node = last_child_node_a;
		rc = set_hw_map(ctl, level, node_b);
		if (rc)
			goto err_out;

		for (i = first_child_node_a; i <= last_child_node_a ; i++)
		{
			ctl->tm_b_node_array[i].parent_c_node = node_b;
			rc = set_hw_map(ctl, B_LEVEL, i);
			if (rc)
				goto err_out;
		}
		break;
	case P_LEVEL:
		rc  = rm_nodes_switch(rm, RM_C_LVL, node_a, node_b,
							  first_child_node_a, last_child_node_a,
							  first_child_node_b, last_child_node_b);
		if (rc)
			goto err_out;

		/* Update node_a... */
		ctl->tm_port_array[node_a].first_child_c_node = first_child_node_b;
		ctl->tm_port_array[node_a].last_child_c_node = last_child_node_b;
		rc = set_hw_map(ctl, level, node_a);
		if (rc)
			goto err_out;

		for (i = first_child_node_b; i <= last_child_node_b ; i++)
		{
			ctl->tm_c_node_array[i].parent_port = (uint8_t)node_a;
			rc = set_hw_map(ctl, C_LEVEL, i);
			if (rc)
				goto err_out;
		}

		/* Update node_b... */
		ctl->tm_port_array[node_b].first_child_c_node = first_child_node_a;
		ctl->tm_port_array[node_b].last_child_c_node = last_child_node_a;
		rc = set_hw_map(ctl, level, node_b);
		if (rc)
			goto err_out;

		for (i = first_child_node_a; i <= last_child_node_a ; i++)
		{
			ctl->tm_c_node_array[i].parent_port = (uint8_t)node_b;
			rc = set_hw_map(ctl, C_LEVEL, i);
			if (rc)
				goto err_out;
		}
		break;
	default:
		return -EINVAL;
	}

	/* Enable all nodes again in HW */
	rc = tm_reorder_nodes_on_off(hndl, level, node_a, first_child_node_a, last_child_node_a, TM_ENABLE);
	if (rc)
		goto err_out;

	rc = tm_reorder_nodes_on_off(hndl, level, node_b, first_child_node_b, last_child_node_b, TM_ENABLE);

err_out:
	tm_nodes_unlock(TM_ENV(ctl));
	return rc;
}

