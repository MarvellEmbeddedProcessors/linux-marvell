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
#include "rm_alloc.h"
#include "rm_chunk.h"
#include "rm_free.h"
#include "tm/core/tm_defs.h"


/**
 */
int rm_find_free_queue(rmctl_t hndl, uint32_t a_node_ind)
{
	struct rm_node *queue = 0;
	uint32_t free_node;

	DECLARE_RM_HANDLE(ctl, hndl)
	CHECK_RM_HANDLE(ctl)

	if (a_node_ind >= ctl->rm_total_a_nodes)
		return -EFAULT;

	if (ctl->rm_a_node_array[a_node_ind].cnt == 0)
		return -ENOBUFS;

	free_node = ctl->rm_a_node_array[a_node_ind].first_child;

	/* Pop free queue */
	queue = &(ctl->rm_queue_array[free_node]);
	queue->used = RM_TRUE;
	ctl->rm_queue_cnt--;
	ctl->rm_a_node_array[a_node_ind].first_child = queue->next_free_ind;
	ctl->rm_a_node_array[a_node_ind].cnt--;

	/* The last one */
	if ((free_node == ctl->rm_a_node_array[a_node_ind].last_child) &&
		(ctl->rm_a_node_array[a_node_ind].first_child == (uint16_t)TM_INVAL))
		ctl->rm_a_node_array[a_node_ind].last_child = (uint16_t)TM_INVAL;

	return free_node;
}


/**
 */
int rm_find_free_a_node(rmctl_t hndl, uint32_t b_node_ind, uint32_t num_of_children)
{
	enum rm_level low_lvl = RM_Q_LVL;
	struct rm_node *node = 0;
	struct rm_node *child = 0;
	uint32_t free_node;
	uint32_t ind;
	int rc;
	int i;

	DECLARE_RM_HANDLE(ctl, hndl)
	CHECK_RM_HANDLE(ctl)

	if (b_node_ind >= ctl->rm_total_b_nodes)
		return -EFAULT;

	free_node = ctl->rm_b_node_array[b_node_ind].first_child;
	if (free_node == (uint16_t)TM_INVAL)
		return -ENOBUFS;

	rc = rm_get_chunk(hndl, low_lvl, num_of_children, &ind);
	if (rc)
		return rc;

	node = &(ctl->rm_a_node_array[free_node]);
	node->first_child = ind;
	node->last_child = ind + num_of_children - 1;

	for (i = ind; i < node->last_child; i++) {
		child = &(ctl->rm_queue_array[i]);
		child->parent_ind = free_node;
		child->next_free_ind = i + 1;
	}
	/* for last child in range */
	child = &(ctl->rm_queue_array[i]);
	child->parent_ind = free_node;
	child->next_free_ind = (uint16_t)TM_INVAL;

	/* Pop free node */
	node->used = RM_TRUE;
	node->cnt = num_of_children;
	ctl->rm_a_node_cnt--;
	ctl->rm_b_node_array[b_node_ind].first_child = node->next_free_ind;
	ctl->rm_b_node_array[b_node_ind].cnt--;

	/* The last one */
	if ((free_node == ctl->rm_b_node_array[b_node_ind].last_child) &&
		(ctl->rm_b_node_array[b_node_ind].first_child == (uint16_t)TM_INVAL))
		ctl->rm_b_node_array[b_node_ind].last_child = (uint16_t)TM_INVAL;

	return free_node;
}


/**
 */
int rm_find_free_b_node(rmctl_t hndl, uint32_t c_node_ind, uint32_t num_of_children)
{
	enum rm_level low_lvl = RM_A_LVL;
	struct rm_node *node = 0;
	struct rm_node *child = 0;
	uint32_t free_node;
	uint32_t ind;
	int rc;
	int i;

	DECLARE_RM_HANDLE(ctl, hndl)
	CHECK_RM_HANDLE(ctl)

	if (c_node_ind >= ctl->rm_total_c_nodes)
		return -EFAULT;

	free_node = ctl->rm_c_node_array[c_node_ind].first_child;
	if (free_node == (uint16_t)TM_INVAL)
		return -ENOBUFS;

	rc = rm_get_chunk(hndl, low_lvl, num_of_children, &ind);
	if (rc)
		return rc;

	node = &(ctl->rm_b_node_array[free_node]);
	node->first_child = ind;
	node->last_child = ind + num_of_children - 1;

	for (i = ind; i < node->last_child; i++) {
		child = &(ctl->rm_a_node_array[i]);
		child->parent_ind = free_node;
		child->next_free_ind = i + 1;
	}
	/* for last child in range */
	child = &(ctl->rm_a_node_array[i]);
	child->parent_ind = free_node;
	child->next_free_ind = (uint16_t) TM_INVAL;

	/* Pop free node */
	node->used = RM_TRUE;
	node->cnt = num_of_children;
	ctl->rm_b_node_cnt--;
	ctl->rm_c_node_array[c_node_ind].first_child = node->next_free_ind;
	ctl->rm_c_node_array[c_node_ind].cnt--;

	/* The last one */
	if ((free_node == ctl->rm_c_node_array[c_node_ind].last_child) &&
		(ctl->rm_c_node_array[c_node_ind].first_child == (uint16_t)TM_INVAL))
		ctl->rm_c_node_array[c_node_ind].last_child = (uint16_t)TM_INVAL;

	return free_node;
}


/**
 */
int rm_find_free_c_node(rmctl_t hndl, uint8_t port_ind, uint32_t num_of_children)
{
	enum rm_level low_lvl = RM_B_LVL;
	struct rm_node *node = 0; /*NULL;*/
	struct rm_node *child = 0;/*NULL;*/
	uint32_t free_node;
	uint32_t ind;
	int rc;
	int i;

	DECLARE_RM_HANDLE(ctl, hndl)
	CHECK_RM_HANDLE(ctl)

	if (port_ind >= ctl->rm_total_ports)
		return -EFAULT;

	free_node = ctl->rm_port_array[port_ind].first_child;
	if (free_node == (uint16_t)TM_INVAL)
		return -ENOBUFS;

	rc = rm_get_chunk(hndl, low_lvl, num_of_children, &ind);
	if (rc)
		return rc;

	node = &(ctl->rm_c_node_array[free_node]);
	node->first_child = ind;
	node->last_child = ind + num_of_children - 1;

	for (i = ind; i < node->last_child; i++) {
		child = &(ctl->rm_b_node_array[i]);
		child->parent_ind = free_node;
		child->next_free_ind = i + 1;
	}
	/* for last child in range */
	child = &(ctl->rm_b_node_array[i]);
	child->parent_ind = free_node;
	child->next_free_ind = (uint16_t) TM_INVAL;

	/* Pop free node */
	node->used = RM_TRUE;
	node->cnt = num_of_children;
	ctl->rm_c_node_cnt--;
	ctl->rm_port_array[port_ind].first_child = node->next_free_ind;
	ctl->rm_port_array[port_ind].cnt--;

	/* The last one */
	if ((free_node == ctl->rm_port_array[port_ind].last_child) &&
		(ctl->rm_port_array[port_ind].first_child == (uint16_t)TM_INVAL)) /* OR: cnt == 0 */
		ctl->rm_port_array[port_ind].last_child = (uint16_t)TM_INVAL;

	return free_node;
}


/**
 */
int rm_init_port(rmctl_t hndl, uint8_t port_id, uint32_t num_of_children)
{
	enum rm_level low_lvl = RM_C_LVL;
	struct rm_node *node = 0;/*NULL;*/
	struct rm_node *child = 0;/*NULL;*/
	uint32_t ind;
	int rc;
	int i;

	DECLARE_RM_HANDLE(ctl, hndl)
	CHECK_RM_HANDLE(ctl)

	if (port_id >= ctl->rm_total_ports)
		return -EFAULT;

	/* No free ports */
	if (ctl->rm_port_cnt == 0)
		return -ENOMEM;

	rc = rm_get_chunk(hndl, low_lvl, num_of_children, &ind);
	if (rc)
		return rc;

	node = &(ctl->rm_port_array[port_id]);
	node->parent_ind = 0;
	node->next_free_ind = 0;


	node->first_child = ind;
	node->last_child = ind + num_of_children - 1;

	for (i = ind; i < node->last_child; i++) {
		child = &(ctl->rm_c_node_array[i]);
		child->parent_ind = port_id;
		child->next_free_ind = i + 1;
	}
	/* for last child in range */
	child = &(ctl->rm_c_node_array[i]);
	child->parent_ind = port_id;
	child->next_free_ind = (uint16_t) TM_INVAL;

	node->used = RM_TRUE;
	node->cnt = num_of_children;
	ctl->rm_port_cnt--;

	return 0;
}


/** For reshuffling purposes.
 */
int rm_expand_range(rmctl_t hndl, enum tm_level level, uint32_t index, uint32_t parent_index)
{
	struct rm_node *node = 0;/*NULL;*/
	struct rm_node *parent = 0;/*NULL;*/
	int rc;

	DECLARE_RM_HANDLE(ctl, hndl)
	CHECK_RM_HANDLE(ctl)

	switch (level) {
	case A_LEVEL:
		if (index >= ctl->rm_total_a_nodes)
			return -EFAULT;

		if (parent_index >= ctl->rm_total_b_nodes)
			return -EFAULT;

		rc = rm_expand_chunk(hndl, RM_A_LVL, index);
		if (rc)
			return rc;

		parent = &(ctl->rm_b_node_array[parent_index]);
		node = &(ctl->rm_a_node_array[index]);

		/* Assumption: no free nodes to the parent,
		because of the reshuffling is performed on full parent only */

		/* Init parent */
		parent->first_child = index;
		parent->last_child = index;
		parent->cnt++;

		/* Init node */
		node->used = RM_FALSE;
		node->parent_ind = parent_index;
		node->next_free_ind = (uint16_t) TM_INVAL;
		break;
	case B_LEVEL:
		if (index >= ctl->rm_total_b_nodes)
			return -EFAULT;

		if (parent_index >= ctl->rm_total_c_nodes)
			return -EFAULT;

		rc = rm_expand_chunk(hndl, RM_B_LVL, index);
		if (rc)
			return rc;

		parent = &(ctl->rm_c_node_array[parent_index]);
		node = &(ctl->rm_b_node_array[index]);

		/* Assumption: no free nodes to the parent,
		because of the reshuffling is performed on full parent only */

		/* Init parent */
		parent->first_child = index;
		parent->last_child = index;
		parent->cnt++;

		/* Init node */
		node->used = RM_FALSE;
		node->parent_ind = parent_index;
		node->next_free_ind = (uint16_t) TM_INVAL;
		break;
	default:
		return -ERANGE;
	}

	return 0;
}


/**
 */
int rm_find_free_wred_queue_curve(rmctl_t hndl)
{
	uint16_t free_entry;

	DECLARE_RM_HANDLE(ctl, hndl)
	CHECK_RM_HANDLE(ctl)

	free_entry = ctl->rm_first_free_entry[RM_WRED_Q_CURVE];
	if (free_entry == (uint16_t)TM_INVAL)
		return -ENOBUFS;

	/* Pop free entry */
	ctl->rm_wred_queue_curves[free_entry].used = RM_TRUE;
	ctl->rm_first_free_entry[RM_WRED_Q_CURVE] =
		ctl->rm_wred_queue_curves[free_entry].next_free_ind;

	/* The last one */
	if ((free_entry == ctl->rm_last_free_entry[RM_WRED_Q_CURVE]) &&
		(ctl->rm_first_free_entry[RM_WRED_Q_CURVE] == (uint16_t)TM_INVAL))
		ctl->rm_last_free_entry[RM_WRED_Q_CURVE] = (uint16_t)TM_INVAL;

	return free_entry;
}


/**
 */
int rm_find_free_wred_a_node_curve(rmctl_t hndl)
{
	uint16_t free_entry;

	DECLARE_RM_HANDLE(ctl, hndl)
	CHECK_RM_HANDLE(ctl)

	free_entry = ctl->rm_first_free_entry[RM_WRED_A_CURVE];
	if (free_entry == (uint16_t)TM_INVAL)
		return -ENOBUFS;

	/* Pop free entry */
	ctl->rm_wred_a_node_curves[free_entry].used = RM_TRUE;
	ctl->rm_first_free_entry[RM_WRED_A_CURVE] =
		ctl->rm_wred_a_node_curves[free_entry].next_free_ind;

	/* The last one */
	if ((free_entry == ctl->rm_last_free_entry[RM_WRED_A_CURVE]) &&
		(ctl->rm_first_free_entry[RM_WRED_A_CURVE] == (uint16_t)TM_INVAL))
		ctl->rm_last_free_entry[RM_WRED_A_CURVE] = (uint16_t)TM_INVAL;

	return free_entry;
}


/**
 */
int rm_find_free_wred_b_node_curve(rmctl_t hndl)
{
	uint16_t free_entry;

	DECLARE_RM_HANDLE(ctl, hndl)
	CHECK_RM_HANDLE(ctl)

	free_entry = ctl->rm_first_free_entry[RM_WRED_B_CURVE];
	if (free_entry == (uint16_t)TM_INVAL)
		return -ENOBUFS;

	/* Pop free entry */
	ctl->rm_wred_b_node_curves[free_entry].used = RM_TRUE;
	ctl->rm_first_free_entry[RM_WRED_B_CURVE] =
		ctl->rm_wred_b_node_curves[free_entry].next_free_ind;

	/* The last one */
	if ((free_entry == ctl->rm_last_free_entry[RM_WRED_B_CURVE]) &&
		(ctl->rm_first_free_entry[RM_WRED_B_CURVE] == (uint16_t)TM_INVAL))
		ctl->rm_last_free_entry[RM_WRED_B_CURVE] = (uint16_t)TM_INVAL;

	return free_entry;
}


/**
 */
int rm_find_free_wred_c_node_curve(rmctl_t hndl, uint8_t cos)
{
	uint16_t free_entry;
	enum rm_prf_level ptr_lvl = RM_WRED_C_CURVE_COS_0;

	DECLARE_RM_HANDLE(ctl, hndl)
	CHECK_RM_HANDLE(ctl)

	switch (cos) {
	case 0:
		ptr_lvl = RM_WRED_C_CURVE_COS_0;
		break;
	case 1:
		ptr_lvl = RM_WRED_C_CURVE_COS_1;
		break;
	case 2:
		ptr_lvl = RM_WRED_C_CURVE_COS_2;
		break;
	case 3:
		ptr_lvl = RM_WRED_C_CURVE_COS_3;
		break;
	case 4:
		ptr_lvl = RM_WRED_C_CURVE_COS_4;
		break;
	case 5:
		ptr_lvl = RM_WRED_C_CURVE_COS_5;
		break;
	case 6:
		ptr_lvl = RM_WRED_C_CURVE_COS_6;
		break;
	case 7:
		ptr_lvl = RM_WRED_C_CURVE_COS_7;
		break;
	default:
		return -EFAULT;
	}

	free_entry = ctl->rm_first_free_entry[ptr_lvl];
	if (free_entry == (uint16_t)TM_INVAL)
		return -ENOBUFS;

	/* Pop free entry */
	ctl->rm_wred_c_node_curves[cos][free_entry].used = RM_TRUE;
	ctl->rm_first_free_entry[ptr_lvl] =
		ctl->rm_wred_c_node_curves[cos][free_entry].next_free_ind;

	/* The last one */
	if ((free_entry == ctl->rm_last_free_entry[ptr_lvl]) &&
		(ctl->rm_first_free_entry[ptr_lvl] == (uint16_t)TM_INVAL))
		ctl->rm_last_free_entry[ptr_lvl] = (uint16_t)TM_INVAL;

	return free_entry;
}


/**
 */
int rm_find_free_wred_port_curve(rmctl_t hndl)
{
	uint16_t free_entry;

	DECLARE_RM_HANDLE(ctl, hndl)
	CHECK_RM_HANDLE(ctl)


	free_entry = ctl->rm_first_free_entry[RM_WRED_P_CURVE];
	if (free_entry == (uint16_t)TM_INVAL)
		return -ENOBUFS;

	/* Pop free entry */
	ctl->rm_wred_port_curves[free_entry].used = RM_TRUE;
	ctl->rm_first_free_entry[RM_WRED_P_CURVE] =
		ctl->rm_wred_port_curves[free_entry].next_free_ind;

	/* The last one */
	if ((free_entry == ctl->rm_last_free_entry[RM_WRED_P_CURVE]) &&
		(ctl->rm_first_free_entry[RM_WRED_P_CURVE] == (uint16_t)TM_INVAL))
		ctl->rm_last_free_entry[RM_WRED_P_CURVE] = (uint16_t)TM_INVAL;

	return free_entry;
}


/**
 */
int rm_find_free_wred_port_curve_cos(rmctl_t hndl, uint8_t cos)
{
	uint16_t free_entry;
	enum rm_prf_level ptr_lvl = RM_WRED_P_CURVE_COS_0;

	DECLARE_RM_HANDLE(ctl, hndl)
	CHECK_RM_HANDLE(ctl)

	switch (cos) {
	case 0:
		ptr_lvl = RM_WRED_P_CURVE_COS_0;
		break;
	case 1:
		ptr_lvl = RM_WRED_P_CURVE_COS_1;
		break;
	case 2:
		ptr_lvl = RM_WRED_P_CURVE_COS_2;
		break;
	case 3:
		ptr_lvl = RM_WRED_P_CURVE_COS_3;
		break;
	case 4:
		ptr_lvl = RM_WRED_P_CURVE_COS_4;
		break;
	case 5:
		ptr_lvl = RM_WRED_P_CURVE_COS_5;
		break;
	case 6:
		ptr_lvl = RM_WRED_P_CURVE_COS_6;
		break;
	case 7:
		ptr_lvl = RM_WRED_P_CURVE_COS_7;
		break;
	default:
		return -EFAULT;
	}

	free_entry = ctl->rm_first_free_entry[ptr_lvl];
	if (free_entry == (uint16_t)TM_INVAL)
		return -ENOBUFS;

	/* Pop free entry */
	ctl->rm_wred_port_curves_cos[cos][free_entry].used = RM_TRUE;
	ctl->rm_first_free_entry[ptr_lvl] =
		ctl->rm_wred_port_curves_cos[cos][free_entry].next_free_ind;

	/* The last one */
	if ((free_entry == ctl->rm_last_free_entry[ptr_lvl]) &&
		(ctl->rm_first_free_entry[ptr_lvl] == (uint16_t)TM_INVAL))
		ctl->rm_last_free_entry[ptr_lvl] = (uint16_t)TM_INVAL;

	return free_entry;
}


/**
 */
int rm_find_free_queue_drop_profile(rmctl_t hndl)
{
	uint16_t free_entry;

	DECLARE_RM_HANDLE(ctl, hndl)
	CHECK_RM_HANDLE(ctl)

	free_entry = ctl->rm_first_free_entry[RM_Q_DROP_PRF];
	if (free_entry == (uint16_t)TM_INVAL)
		return -ENOBUFS;

	/* Pop free entry */
	ctl->rm_queue_drop_profiles[free_entry].used = RM_TRUE;
	ctl->rm_first_free_entry[RM_Q_DROP_PRF] =
		ctl->rm_queue_drop_profiles[free_entry].next_free_ind;

	/* The last one */
	if ((free_entry == ctl->rm_last_free_entry[RM_Q_DROP_PRF]) &&
		(ctl->rm_first_free_entry[RM_Q_DROP_PRF] == (uint16_t)TM_INVAL))
		ctl->rm_last_free_entry[RM_Q_DROP_PRF] = (uint16_t)TM_INVAL;

	return free_entry;
}


/**
 */
int rm_find_free_a_node_drop_profile(rmctl_t hndl)
{
	uint16_t free_entry;

	DECLARE_RM_HANDLE(ctl, hndl)
	CHECK_RM_HANDLE(ctl)

	free_entry = ctl->rm_first_free_entry[RM_A_DROP_PRF];
	if (free_entry == (uint16_t)TM_INVAL)
		return -ENOBUFS;

	/* Pop free entry */
	ctl->rm_a_node_drop_profiles[free_entry].used = RM_TRUE;
	ctl->rm_first_free_entry[RM_A_DROP_PRF] =
		ctl->rm_a_node_drop_profiles[free_entry].next_free_ind;

	/* The last one */
	if ((free_entry == ctl->rm_last_free_entry[RM_A_DROP_PRF]) &&
		(ctl->rm_first_free_entry[RM_A_DROP_PRF] == (uint16_t)TM_INVAL))
		ctl->rm_last_free_entry[RM_A_DROP_PRF] = (uint16_t)TM_INVAL;

	return free_entry;
}


/**
 */
int rm_find_free_b_node_drop_profile(rmctl_t hndl)
{
	uint16_t free_entry;

	DECLARE_RM_HANDLE(ctl, hndl)
	CHECK_RM_HANDLE(ctl)

	free_entry = ctl->rm_first_free_entry[RM_B_DROP_PRF];
	if (free_entry == (uint16_t)TM_INVAL)
		return -ENOBUFS;

	/* Pop free entry */
	ctl->rm_b_node_drop_profiles[free_entry].used = RM_TRUE;
	ctl->rm_first_free_entry[RM_B_DROP_PRF] =
		ctl->rm_b_node_drop_profiles[free_entry].next_free_ind;

	/* The last one */
	if ((free_entry == ctl->rm_last_free_entry[RM_B_DROP_PRF]) &&
		(ctl->rm_first_free_entry[RM_B_DROP_PRF] == (uint16_t)TM_INVAL))
		ctl->rm_last_free_entry[RM_B_DROP_PRF] = (uint16_t)TM_INVAL;

	return free_entry;
}


/**
 */
int rm_find_free_c_node_drop_profile(rmctl_t hndl, uint8_t cos)
{
	uint16_t free_entry;
	enum rm_prf_level ptr_lvl = RM_C_DROP_PRF_COS_0;

	DECLARE_RM_HANDLE(ctl, hndl)
	CHECK_RM_HANDLE(ctl)

	switch (cos) {
	case 0:
		ptr_lvl = RM_C_DROP_PRF_COS_0;
		break;
	case 1:
		ptr_lvl = RM_C_DROP_PRF_COS_1;
		break;
	case 2:
		ptr_lvl = RM_C_DROP_PRF_COS_2;
		break;
	case 3:
		ptr_lvl = RM_C_DROP_PRF_COS_3;
		break;
	case 4:
		ptr_lvl = RM_C_DROP_PRF_COS_4;
		break;
	case 5:
		ptr_lvl = RM_C_DROP_PRF_COS_5;
		break;
	case 6:
		ptr_lvl = RM_C_DROP_PRF_COS_6;
		break;
	case 7:
		ptr_lvl = RM_C_DROP_PRF_COS_7;
		break;
	default:
		return -EFAULT;
	}

	free_entry = ctl->rm_first_free_entry[ptr_lvl];
	if (free_entry == (uint16_t)TM_INVAL)
		return -ENOBUFS;

	/* Pop free entry */
	ctl->rm_c_node_drop_profiles[cos][free_entry].used = RM_TRUE;
	ctl->rm_first_free_entry[ptr_lvl] =
		ctl->rm_c_node_drop_profiles[cos][free_entry].next_free_ind;

	/* The last one */
	if ((free_entry == ctl->rm_last_free_entry[ptr_lvl]) &&
		(ctl->rm_first_free_entry[ptr_lvl] == (uint16_t)TM_INVAL))
		ctl->rm_last_free_entry[ptr_lvl] = (uint16_t)TM_INVAL;

	return free_entry;
}


/**
 */
int rm_find_free_port_drop_profile(rmctl_t hndl)
{
	uint16_t free_entry;

	DECLARE_RM_HANDLE(ctl, hndl)
	CHECK_RM_HANDLE(ctl)

	free_entry = ctl->rm_first_free_entry[RM_P_DROP_PRF];
	if (free_entry == (uint16_t)TM_INVAL)
		return -ENOBUFS;

	/* Pop free entry */
	ctl->rm_port_drop_profiles[free_entry].used = RM_TRUE;
	ctl->rm_first_free_entry[RM_P_DROP_PRF] =
		ctl->rm_port_drop_profiles[free_entry].next_free_ind;

	/* The last one */
	if ((free_entry == ctl->rm_last_free_entry[RM_P_DROP_PRF]) &&
		(ctl->rm_first_free_entry[RM_P_DROP_PRF] == (uint16_t)TM_INVAL))
		ctl->rm_last_free_entry[RM_P_DROP_PRF] = (uint16_t)TM_INVAL;

	return free_entry;
}


/* for BC2  only */
int rm_find_free_port_drop_profile_cos(rmctl_t hndl, uint8_t cos)
{
	uint16_t free_entry;
	enum rm_prf_level ptr_lvl = RM_P_DROP_PRF_COS_0;

	DECLARE_RM_HANDLE(ctl, hndl)
	CHECK_RM_HANDLE(ctl)

	switch (cos) {
	case 0:
		ptr_lvl = RM_P_DROP_PRF_COS_0;
		break;
	case 1:
		ptr_lvl = RM_P_DROP_PRF_COS_1;
		break;
	case 2:
		ptr_lvl = RM_P_DROP_PRF_COS_2;
		break;
	case 3:
		ptr_lvl = RM_P_DROP_PRF_COS_3;
		break;
	case 4:
		ptr_lvl = RM_P_DROP_PRF_COS_4;
		break;
	case 5:
		ptr_lvl = RM_P_DROP_PRF_COS_5;
		break;
	case 6:
		ptr_lvl = RM_P_DROP_PRF_COS_6;
		break;
	case 7:
		ptr_lvl = RM_P_DROP_PRF_COS_7;
		break;
	default:
		return -EFAULT;
	}

	free_entry = ctl->rm_first_free_entry[ptr_lvl];
	if (free_entry == (uint16_t)TM_INVAL)
		return -ENOBUFS;

	/* Pop free entry */
	ctl->rm_port_drop_profiles_cos[cos][free_entry].used = RM_TRUE;
	ctl->rm_first_free_entry[ptr_lvl] =
		ctl->rm_port_drop_profiles_cos[cos][free_entry].next_free_ind;

	/* The last one */
	if ((free_entry == ctl->rm_last_free_entry[ptr_lvl]) &&
		(ctl->rm_first_free_entry[ptr_lvl] == (uint16_t)TM_INVAL))
		ctl->rm_last_free_entry[ptr_lvl] = (uint16_t)TM_INVAL;

	return free_entry;
}
