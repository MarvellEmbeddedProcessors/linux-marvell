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

#include "rm_internal_types.h"
#include "rm_reorder.h"
#include "tm/core/tm_defs.h"

/**
 */
int rm_nodes_move(rmctl_t hndl,
		enum rm_level level,
		uint32_t from_node_ind,
		uint32_t to_node_ind,
		uint32_t number_of_children,
		uint32_t first_child_to_move)
{
	struct rm_node *node = NULL;
	struct rm_node *from_node = NULL;
	struct rm_node *to_node = NULL;
	uint32_t prev = 0;
	uint32_t i;
	uint32_t j;
	uint32_t last_child_to_move = first_child_to_move + number_of_children - 1;

	DECLARE_RM_HANDLE(ctl, hndl)
	CHECK_RM_HANDLE(ctl)


	switch (level) {
	case RM_Q_LVL:
	if ((from_node_ind >= ctl->rm_total_a_nodes) ||
	(to_node_ind >= ctl->rm_total_a_nodes))
		return -EFAULT;

	from_node = &(ctl->rm_a_node_array[from_node_ind]);
	to_node = &(ctl->rm_a_node_array[to_node_ind]);

	for (i = first_child_to_move; i <= last_child_to_move; i++) {
		node = &(ctl->rm_queue_array[i]);
		node->parent_ind = to_node_ind;
		if (node->used == RM_FALSE) {
			/* pop the node form the list of free nodes under the "from_node" */
			for (j = from_node->first_child; j != TM_INVAL;
				j = ctl->rm_queue_array[j].next_free_ind) {
				if (j == i) {
					if (j == from_node->first_child) /* head */
						from_node->first_child = ctl->rm_queue_array[j].next_free_ind;
					else
						ctl->rm_queue_array[prev].next_free_ind =
						ctl->rm_queue_array[j].next_free_ind;
					from_node->cnt--;
					if (from_node->cnt == 0)
						from_node->last_child = (uint16_t) TM_INVAL;
					if (from_node->last_child == j)
						from_node->last_child = prev;
					break; /* for */
				}
				prev = j;
			}
			if (j == TM_INVAL)
				return -ENOBUFS;

			/* move the node to the list of free nodes under the "to_node" */
			ctl->rm_queue_array[j].next_free_ind = to_node->first_child;
			to_node->first_child = j;
			if (to_node->cnt == 0) /* empty */
			to_node->last_child = j;
			to_node->cnt++;
		}
	/* no action required when the node in use */
	}
	break; /* switch */
	case RM_A_LVL:
	if ((from_node_ind >= ctl->rm_total_b_nodes) ||
	(to_node_ind >= ctl->rm_total_b_nodes))
		return -EFAULT;

	from_node = &(ctl->rm_b_node_array[from_node_ind]);
	to_node = &(ctl->rm_b_node_array[to_node_ind]);

	for (i = first_child_to_move; i <= last_child_to_move; i++) {
		node = &(ctl->rm_a_node_array[i]);
		node->parent_ind = to_node_ind;
		if (node->used == RM_FALSE) {
			/* pop the node form the list of free nodes under the "from_node" */
			for (j = from_node->first_child; j != TM_INVAL;
				j = ctl->rm_a_node_array[j].next_free_ind) {
				if (j == i) {
					if (j == from_node->first_child) /* head */
						from_node->first_child = ctl->rm_a_node_array[j].next_free_ind;
					else
						ctl->rm_a_node_array[prev].next_free_ind =
						ctl->rm_a_node_array[j].next_free_ind;
					from_node->cnt--;
					if (from_node->cnt == 0)
						from_node->last_child = (uint16_t) TM_INVAL;
					if (from_node->last_child == j)
						from_node->last_child = prev;
					break; /* for */
				}
				prev = j;
			}
			if (j == TM_INVAL)
				return -ENOBUFS;

			/* move the node to the list of free nodes under the "to_node" */
			ctl->rm_a_node_array[j].next_free_ind = to_node->first_child;
			to_node->first_child = j;
			if (to_node->cnt == 0) /* empty */
			to_node->last_child = j;
			to_node->cnt++;
		}
		/* no action required when the node in use */
	}
	break; /* switch */
	case RM_B_LVL:
	if ((from_node_ind >= ctl->rm_total_c_nodes) ||
	(to_node_ind >= ctl->rm_total_c_nodes))
		return -EFAULT;

	from_node = &(ctl->rm_c_node_array[from_node_ind]);
	to_node = &(ctl->rm_c_node_array[to_node_ind]);

	for (i = first_child_to_move; i <= last_child_to_move; i++) {
		node = &(ctl->rm_b_node_array[i]);
		node->parent_ind = to_node_ind;
		if (node->used == RM_FALSE) {
			/* pop the node form the list of free nodes under the "from_node" */
			for (j = from_node->first_child; j != TM_INVAL;
				j = ctl->rm_b_node_array[j].next_free_ind) {
				if (j == i) {
					if (j == from_node->first_child) /* head */
						from_node->first_child = ctl->rm_b_node_array[j].next_free_ind;
					else
						ctl->rm_b_node_array[prev].next_free_ind =
						ctl->rm_b_node_array[j].next_free_ind;
					from_node->cnt--;
					if (from_node->cnt == 0)
						from_node->last_child = (uint16_t) TM_INVAL;
					if (from_node->last_child == j)
						from_node->last_child = prev;
					break; /* for */
				}
				prev = j;
			}
			if (j == TM_INVAL)
				return -ENOBUFS;

			/* move the node to the list of free nodes under the "to_node" */
			ctl->rm_b_node_array[j].next_free_ind = to_node->first_child;
			to_node->first_child = j;
			if (to_node->cnt == 0) /* empty */
				to_node->last_child = j;
			to_node->cnt++;
		}
		/* no action required when the node in use */
	}
	break; /* switch */
	case RM_C_LVL:
	if ((from_node_ind >= ctl->rm_total_ports) ||
	(to_node_ind >= ctl->rm_total_ports))
		return -EFAULT;

	from_node = &(ctl->rm_port_array[from_node_ind]);
	to_node = &(ctl->rm_port_array[to_node_ind]);

	for (i = first_child_to_move; i <= last_child_to_move; i++) {
		node = &(ctl->rm_c_node_array[i]);
		node->parent_ind = to_node_ind;
		if (node->used == RM_FALSE) {
			/* pop the node form the list of free nodes under the "from_node" */
			for (j = from_node->first_child; j != TM_INVAL;
				j = ctl->rm_c_node_array[j].next_free_ind) {
				if (j == i) {
					if (j == from_node->first_child) /* head */
						from_node->first_child = ctl->rm_c_node_array[j].next_free_ind;
					else
						ctl->rm_c_node_array[prev].next_free_ind =
						ctl->rm_c_node_array[j].next_free_ind;
					from_node->cnt--;
					if (from_node->cnt == 0)
						from_node->last_child = (uint16_t) TM_INVAL;
					if (from_node->last_child == j)
						from_node->last_child = prev;
					break; /* for */
				}
				prev = j;
			}
			if (j == TM_INVAL)
				return -ENOBUFS;

			/* move the node to the list of free nodes under the "to_node" */
			ctl->rm_c_node_array[j].next_free_ind = to_node->first_child;
			to_node->first_child = j;
			if (to_node->cnt == 0) /* empty */
			to_node->last_child = j;
			to_node->cnt++;
		}
	/* no action required when the node in use */
	}
	break; /* switch */
	default:
		return -ERANGE;
	}
	return 0;
}


/**
 */
int rm_nodes_switch(rmctl_t hndl,
			enum rm_level level,
			uint32_t node_a_ind,
			uint32_t node_b_ind,
			uint32_t first_a_child,
			uint32_t last_a_child,
			uint32_t first_b_child,
			uint32_t last_b_child)
{
	struct rm_node *node_a = NULL;
	struct rm_node *node_b = NULL;
	uint32_t first_child;
	uint32_t last_child;
	uint32_t cnt;
	uint32_t i;

	DECLARE_RM_HANDLE(ctl, hndl)
	CHECK_RM_HANDLE(ctl)


	switch (level) {
	case RM_Q_LVL:
		if ((node_a_ind >= ctl->rm_total_a_nodes) ||
		(node_b_ind >= ctl->rm_total_a_nodes))
			return -EFAULT;

		node_a = &(ctl->rm_a_node_array[node_a_ind]);
		node_b = &(ctl->rm_a_node_array[node_b_ind]);

		for (i = first_a_child; i <= last_a_child; i++)
			ctl->rm_queue_array[i].parent_ind = node_b_ind;

		for (i = first_b_child; i <= last_b_child; i++)
			ctl->rm_queue_array[i].parent_ind = node_a_ind;

		break;
	case RM_A_LVL:
		if ((node_a_ind >= ctl->rm_total_b_nodes) ||
		(node_b_ind >= ctl->rm_total_b_nodes))
			return -EFAULT;

		node_a = &(ctl->rm_b_node_array[node_a_ind]);
		node_b = &(ctl->rm_b_node_array[node_b_ind]);

		for (i = first_a_child; i <= last_a_child; i++)
			ctl->rm_a_node_array[i].parent_ind = node_b_ind;

		for (i = first_b_child; i <= last_b_child; i++)
			ctl->rm_a_node_array[i].parent_ind = node_a_ind;

		break;
	case RM_B_LVL:
		if ((node_a_ind >= ctl->rm_total_c_nodes) ||
		(node_b_ind >= ctl->rm_total_c_nodes))
			return -EFAULT;

		node_a = &(ctl->rm_c_node_array[node_a_ind]);
		node_b = &(ctl->rm_c_node_array[node_b_ind]);

		for (i = first_a_child; i <= last_a_child; i++)
			ctl->rm_b_node_array[i].parent_ind = node_b_ind;

		for (i = first_b_child; i <= last_b_child; i++)
			ctl->rm_b_node_array[i].parent_ind = node_a_ind;

		break;
	case RM_C_LVL:
		if ((node_a_ind >= ctl->rm_total_ports) ||
		(node_b_ind >= ctl->rm_total_ports))
			return -EFAULT;

		node_a = &(ctl->rm_port_array[node_a_ind]);
		node_b = &(ctl->rm_port_array[node_b_ind]);

		for (i = first_a_child; i <= last_a_child; i++)
			ctl->rm_c_node_array[i].parent_ind = node_b_ind;

		for (i = first_b_child; i <= last_b_child; i++)
			ctl->rm_c_node_array[i].parent_ind = node_a_ind;

		break;
	default:
		return -ERANGE;
	}

	first_child = node_a->first_child;
	last_child = node_a->last_child;
	cnt = node_a->cnt;

	node_a->first_child = node_b->first_child;
	node_a->last_child = node_b->last_child;
	node_a->cnt = node_b->cnt;

	node_b->first_child = first_child;
	node_b->last_child = last_child;
	node_b->cnt = cnt;

	return 0;
}

