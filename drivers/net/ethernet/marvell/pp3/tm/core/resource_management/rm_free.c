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
#include "rm_free.h"
#include "rm_chunk.h"
#include "tm/core/tm_core_types.h"
#include "tm/core/tm_hw_configuration_interface.h"

/**
 */
int rm_free_queue(rmctl_t hndl, uint32_t queue_ind)
{
	struct rm_node *node = NULL;
	uint32_t parent_ind;

	DECLARE_RM_HANDLE(ctl, hndl)
	CHECK_RM_HANDLE(ctl)

	if (queue_ind >= (ctl->rm_total_queues))
		return -EFAULT;

	node = &(ctl->rm_queue_array[queue_ind]);
	if (node->used != RM_TRUE)
		return -ENOMSG;

	parent_ind = node->parent_ind;

	/* Insert as free node */
	node->used = RM_FALSE;
	ctl->rm_queue_cnt++;
	node->next_free_ind = ctl->rm_a_node_array[parent_ind].first_child;
	ctl->rm_a_node_array[parent_ind].first_child = queue_ind;
	ctl->rm_a_node_array[parent_ind].cnt++;

	/* Was empty */
	if (ctl->rm_a_node_array[parent_ind].last_child == (uint16_t)TM_INVAL)
		ctl->rm_a_node_array[parent_ind].last_child = queue_ind;

	return 0;
}


/**
 */
int rm_free_a_node(rmctl_t hndl, uint32_t a_node_ind, uint32_t range)
{
	struct rm_node *node = NULL;
	uint32_t parent_ind;
	uint32_t cur_ind;
	uint32_t min_ind;
	int rc = 0;
	uint32_t i;

	DECLARE_RM_HANDLE(ctl, hndl)
	CHECK_RM_HANDLE(ctl)

	if (a_node_ind >= ctl->rm_total_a_nodes)
		return -EFAULT;

	node = &(ctl->rm_a_node_array[a_node_ind]);
	if (node->used != RM_TRUE)
		return -ENOMSG;

	if (node->cnt < range) /* any child still in use */
		return -EBUSY;

	parent_ind = node->parent_ind;

	/* Insert as free node */
	node->used = RM_FALSE;
	ctl->rm_a_node_cnt++;
	node->next_free_ind = ctl->rm_b_node_array[parent_ind].first_child;
	ctl->rm_b_node_array[parent_ind].first_child = a_node_ind;
	ctl->rm_b_node_array[parent_ind].cnt++;

	/* find the lowest index in range */
	cur_ind = node->first_child;
	min_ind = cur_ind;
	for (i = 1; i < range; i++) {
		cur_ind = ctl->rm_queue_array[cur_ind].next_free_ind;
		if (min_ind > cur_ind)
			min_ind = cur_ind;
	}
	rc = rm_release_chunk(hndl, RM_Q_LVL, node->cnt, min_ind);
	if (rc)
		return rc;

	/* Was empty */
	if (ctl->rm_b_node_array[parent_ind].last_child == (uint16_t)TM_INVAL)
		ctl->rm_b_node_array[parent_ind].last_child = a_node_ind;

	return rc;
}


/**
 */
int rm_free_b_node(rmctl_t hndl, uint32_t b_node_ind, uint32_t range)
{
	struct rm_node *node = NULL;
	uint32_t parent_ind;
	uint32_t cur_ind;
	uint32_t min_ind;
	int rc = 0;
	uint32_t i;

	DECLARE_RM_HANDLE(ctl, hndl)
	CHECK_RM_HANDLE(ctl)

	if (b_node_ind >= ctl->rm_total_b_nodes)
		return -EFAULT;

	node = &(ctl->rm_b_node_array[b_node_ind]);
	if (node->used != RM_TRUE)
		return -ENOMSG;

	if (node->cnt < range) /* any child still in use */
		return -EBUSY;

	parent_ind = node->parent_ind;

	/* Insert as free node */
	node->used = RM_FALSE;
	ctl->rm_b_node_cnt++;
	node->next_free_ind = ctl->rm_c_node_array[parent_ind].first_child;
	ctl->rm_c_node_array[parent_ind].first_child = b_node_ind;
	ctl->rm_c_node_array[parent_ind].cnt++;

	/* find the lowest index in range */
	cur_ind = node->first_child;
	min_ind = cur_ind;
	for (i = 1; i < range; i++) {
		cur_ind = ctl->rm_a_node_array[cur_ind].next_free_ind;
		if (min_ind > cur_ind)
			min_ind = cur_ind;
	}
	rc = rm_release_chunk(hndl, RM_A_LVL, node->cnt, min_ind);
	if (rc)
		return rc;

	/* Was empty */
	if (ctl->rm_c_node_array[parent_ind].last_child == (uint16_t)TM_INVAL)
		ctl->rm_c_node_array[parent_ind].last_child = b_node_ind;

	return rc;
}


/**
 */
int rm_free_c_node(rmctl_t hndl, uint32_t c_node_ind, uint32_t range)
{
	struct rm_node *node = NULL;
	uint32_t parent_ind;
	uint32_t cur_ind;
	uint32_t min_ind;
	int rc = 0;
	uint32_t i;

	DECLARE_RM_HANDLE(ctl, hndl)
	CHECK_RM_HANDLE(ctl)

	if (c_node_ind >= ctl->rm_total_c_nodes)
		return -EFAULT;

	node = &(ctl->rm_c_node_array[c_node_ind]);
	if (node->used != RM_TRUE)
		return -ENOMSG;

	if (node->cnt < range)   /* any child still in use */
		return -EBUSY;

	parent_ind = node->parent_ind;

	/* Insert as free node */
	node->used = RM_FALSE;
	ctl->rm_c_node_cnt++;
	node->next_free_ind = ctl->rm_port_array[parent_ind].first_child;
	ctl->rm_port_array[parent_ind].first_child = c_node_ind;
	ctl->rm_port_array[parent_ind].cnt++;

	/* find the lowest index in range */
	cur_ind = node->first_child;
	min_ind = cur_ind;
	for (i = 1; i < range; i++) {
		cur_ind = ctl->rm_b_node_array[cur_ind].next_free_ind;
		if (min_ind > cur_ind)
			min_ind = cur_ind;
	}
	rc = rm_release_chunk(hndl, RM_B_LVL, node->cnt, min_ind);
	if (rc)
		return rc;

	/* Was empty */
	if (ctl->rm_port_array[parent_ind].last_child == (uint16_t)TM_INVAL)
		ctl->rm_port_array[parent_ind].last_child = c_node_ind;

	return rc;
}


/**
 */
int rm_free_port(rmctl_t hndl, uint8_t port_ind, uint32_t range)
{
	struct rm_node *node = NULL;
	uint32_t cur_ind;
	uint32_t min_ind;
	int rc = 0;
	uint32_t i;

	DECLARE_RM_HANDLE(ctl, hndl)
	CHECK_RM_HANDLE(ctl)

	if (port_ind >= ctl->rm_total_ports)
		return -EFAULT;

	node = &(ctl->rm_port_array[port_ind]);
	if (node->used != RM_TRUE)
		return -ENOMSG;

	if (node->cnt < range) /* any child still in use */
		return -EBUSY;

	/* Insert as free port */
	node->used = RM_FALSE;
	ctl->rm_port_cnt++;

	/* find the lowest index in range */
	cur_ind = node->first_child;
	min_ind = cur_ind;
	for (i = 1; i < range; i++) {
		cur_ind = ctl->rm_c_node_array[cur_ind].next_free_ind;
		if (min_ind > cur_ind)
			min_ind = cur_ind;
	}
	/* Manage c-nodes chunks */
	rc = rm_release_chunk(hndl, RM_C_LVL, node->cnt, min_ind);
	if (rc)
		return rc;

	return 0;
}


/**
 */
int rm_free_wred_queue_curve(rmctl_t hndl, uint16_t entry_ind)
{
	DECLARE_RM_HANDLE(ctl, hndl)
	CHECK_RM_HANDLE(ctl)

	if (entry_ind >= TM_NUM_WRED_QUEUE_CURVES)
		return -EFAULT;

	if (ctl->rm_wred_queue_curves[entry_ind].used != RM_TRUE)
		return -ENOMSG;

	/* Insert as free entry */
	ctl->rm_wred_queue_curves[entry_ind].used = RM_FALSE;

	/* Was empty */
	if (ctl->rm_last_free_entry[RM_WRED_Q_CURVE] == (uint16_t)TM_INVAL)
		ctl->rm_last_free_entry[RM_WRED_Q_CURVE] = entry_ind;
	else
		ctl->rm_wred_queue_curves[entry_ind].next_free_ind =
			ctl->rm_first_free_entry[RM_WRED_Q_CURVE];

	ctl->rm_first_free_entry[RM_WRED_Q_CURVE] = entry_ind;

	return 0;
}


/**
 */
int rm_free_wred_a_node_curve(rmctl_t hndl, uint16_t entry_ind)
{
	DECLARE_RM_HANDLE(ctl, hndl)
	CHECK_RM_HANDLE(ctl)

	if (entry_ind >= TM_NUM_WRED_A_NODE_CURVES)
		return -EFAULT;

	if (ctl->rm_wred_a_node_curves[entry_ind].used != RM_TRUE)
		return -ENOMSG;

	/* Insert as free entry */
	ctl->rm_wred_a_node_curves[entry_ind].used = RM_FALSE;

	/* Was empty */
	if (ctl->rm_last_free_entry[RM_WRED_A_CURVE] == (uint16_t)TM_INVAL)
		ctl->rm_last_free_entry[RM_WRED_A_CURVE] = entry_ind;
	else
		ctl->rm_wred_a_node_curves[entry_ind].next_free_ind =
			ctl->rm_first_free_entry[RM_WRED_A_CURVE];

	ctl->rm_first_free_entry[RM_WRED_A_CURVE] = entry_ind;

	return 0;
}


/**
 */
int rm_free_wred_b_node_curve(rmctl_t hndl, uint16_t entry_ind)
{
	DECLARE_RM_HANDLE(ctl, hndl)
	CHECK_RM_HANDLE(ctl)

	if (entry_ind >= TM_NUM_WRED_B_NODE_CURVES)
		return -EFAULT;

	if (ctl->rm_wred_b_node_curves[entry_ind].used != RM_TRUE)
		return -ENOMSG;

	/* Insert as free entry */
	ctl->rm_wred_b_node_curves[entry_ind].used = RM_FALSE;

	/* Was empty */
	if (ctl->rm_last_free_entry[RM_WRED_B_CURVE] == (uint16_t)TM_INVAL)
		ctl->rm_last_free_entry[RM_WRED_B_CURVE] = entry_ind;
	else
		ctl->rm_wred_b_node_curves[entry_ind].next_free_ind =
			ctl->rm_first_free_entry[RM_WRED_B_CURVE];

	ctl->rm_first_free_entry[RM_WRED_B_CURVE] = entry_ind;

	return 0;
}


/**
 */
int rm_free_wred_c_node_curve(rmctl_t hndl, uint8_t cos, uint16_t entry_ind)
{
	enum rm_prf_level ptr_lvl;

	DECLARE_RM_HANDLE(ctl, hndl)
	CHECK_RM_HANDLE(ctl)

	if (entry_ind >= TM_NUM_WRED_C_NODE_CURVES)
		return -EFAULT;

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
		return -EFAULT; /* cos >= TM_WRED_COS */
	}

	if (ctl->rm_wred_c_node_curves[cos][entry_ind].used != RM_TRUE)
		return -ENOMSG;

	/* Insert as free entry */
	ctl->rm_wred_c_node_curves[cos][entry_ind].used = RM_FALSE;

	/* Was empty */
	if (ctl->rm_last_free_entry[ptr_lvl] == (uint16_t)TM_INVAL)
		ctl->rm_last_free_entry[ptr_lvl] = entry_ind;
	else
		ctl->rm_wred_c_node_curves[cos][entry_ind].next_free_ind =
			ctl->rm_first_free_entry[ptr_lvl];

	ctl->rm_first_free_entry[ptr_lvl] = entry_ind;

	return 0;
}


/**
 */
int rm_free_wred_port_curve(rmctl_t hndl, uint16_t entry_ind)
{
	DECLARE_RM_HANDLE(ctl, hndl)
	CHECK_RM_HANDLE(ctl)

	if (entry_ind >= TM_NUM_WRED_PORT_CURVES)
		return -EFAULT;

	if (ctl->rm_wred_port_curves[entry_ind].used != RM_TRUE)
		return -ENOMSG;

	/* Insert as free entry */
	ctl->rm_wred_port_curves[entry_ind].used = RM_FALSE;

	/* Was empty */
	if (ctl->rm_last_free_entry[RM_WRED_P_CURVE] == (uint16_t)TM_INVAL)
		ctl->rm_last_free_entry[RM_WRED_P_CURVE] = entry_ind;
	else
		ctl->rm_wred_port_curves[entry_ind].next_free_ind =
			ctl->rm_first_free_entry[RM_WRED_P_CURVE];

	ctl->rm_first_free_entry[RM_WRED_P_CURVE] = entry_ind;

	return 0;
}


/**
 */
int rm_free_wred_port_curve_cos(rmctl_t hndl, uint8_t cos, uint16_t entry_ind)
{
	enum rm_prf_level ptr_lvl;

	DECLARE_RM_HANDLE(ctl, hndl)
	CHECK_RM_HANDLE(ctl)

	if (!ctl)
		return -EINVAL;
	if (ctl->magic != RM_MAGIC)
		return -EBADF;

	if (entry_ind >= TM_NUM_WRED_PORT_CURVES)
		return -EFAULT;

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
		return -EFAULT; /* cos >= TM_WRED_COS */
	}

	if (ctl->rm_wred_port_curves_cos[cos][entry_ind].used != RM_TRUE)
		return -ENOMSG;

	/* Insert as free entry */
	ctl->rm_wred_port_curves_cos[cos][entry_ind].used = RM_FALSE;

	/* Was empty */
	if (ctl->rm_last_free_entry[ptr_lvl] == (uint16_t)TM_INVAL)
		ctl->rm_last_free_entry[ptr_lvl] = entry_ind;
	else
		ctl->rm_wred_port_curves_cos[cos][entry_ind].next_free_ind =
			ctl->rm_first_free_entry[ptr_lvl];

	ctl->rm_first_free_entry[ptr_lvl] = entry_ind;

	return 0;
}


/**
 */
int rm_free_queue_drop_profile(rmctl_t hndl, uint16_t entry_ind)
{
	DECLARE_RM_HANDLE(ctl, hndl)
	CHECK_RM_HANDLE(ctl)

	if (entry_ind >= TM_NUM_QUEUE_DROP_PROF)
		return -EFAULT;

	if (ctl->rm_queue_drop_profiles[entry_ind].used != RM_TRUE)
		return -ENOMSG;

	/* Insert as free entry */
	ctl->rm_queue_drop_profiles[entry_ind].used = RM_FALSE;

	/* Was empty */
	if (ctl->rm_last_free_entry[RM_Q_DROP_PRF] == (uint16_t)TM_INVAL)
		ctl->rm_last_free_entry[RM_Q_DROP_PRF] = entry_ind;
	else
		ctl->rm_queue_drop_profiles[entry_ind].next_free_ind =
			ctl->rm_first_free_entry[RM_Q_DROP_PRF];

	ctl->rm_first_free_entry[RM_Q_DROP_PRF] = entry_ind;

	return 0;
}


/**
 */
int rm_free_a_node_drop_profile(rmctl_t hndl, uint16_t entry_ind)
{
	DECLARE_RM_HANDLE(ctl, hndl)
	CHECK_RM_HANDLE(ctl)

	if (entry_ind >= TM_NUM_A_NODE_DROP_PROF)
		return -EFAULT;

	if (ctl->rm_a_node_drop_profiles[entry_ind].used != RM_TRUE)
		return -ENOMSG;

	/* Insert as free entry */
	ctl->rm_a_node_drop_profiles[entry_ind].used = RM_FALSE;

	/* Was empty */
	if (ctl->rm_last_free_entry[RM_A_DROP_PRF] == (uint16_t)TM_INVAL)
		ctl->rm_last_free_entry[RM_A_DROP_PRF] = entry_ind;
	else
		ctl->rm_a_node_drop_profiles[entry_ind].next_free_ind =
			ctl->rm_first_free_entry[RM_A_DROP_PRF];

	ctl->rm_first_free_entry[RM_A_DROP_PRF] = entry_ind;

	return 0;
}


/**
 */
int rm_free_b_node_drop_profile(rmctl_t hndl, uint16_t entry_ind)
{
	DECLARE_RM_HANDLE(ctl, hndl)
	CHECK_RM_HANDLE(ctl)

	if (entry_ind >= TM_NUM_B_NODE_DROP_PROF)
		return -EFAULT;

	if (ctl->rm_b_node_drop_profiles[entry_ind].used != RM_TRUE)
		return -ENOMSG;

	/* Insert as free entry */
	ctl->rm_b_node_drop_profiles[entry_ind].used = RM_FALSE;

	/* Was empty */
	if (ctl->rm_last_free_entry[RM_B_DROP_PRF] == (uint16_t)TM_INVAL)
		ctl->rm_last_free_entry[RM_B_DROP_PRF] = entry_ind;
	else
		ctl->rm_b_node_drop_profiles[entry_ind].next_free_ind =
			ctl->rm_first_free_entry[RM_B_DROP_PRF];

	ctl->rm_first_free_entry[RM_B_DROP_PRF] = entry_ind;

	return 0;
}


/**
 */
int rm_free_c_node_drop_profile(rmctl_t hndl, uint8_t cos, uint16_t entry_ind)
{
	enum rm_prf_level ptr_lvl;

	DECLARE_RM_HANDLE(ctl, hndl)
	CHECK_RM_HANDLE(ctl)

	if (entry_ind >= TM_NUM_C_NODE_DROP_PROF)
		return -EFAULT;

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
		return -EFAULT; /* cos >= TM_WRED_COS */
	}

	if (ctl->rm_c_node_drop_profiles[cos][entry_ind].used != RM_TRUE)
		return -ENOMSG;

	/* Insert as free entry */
	ctl->rm_c_node_drop_profiles[cos][entry_ind].used = RM_FALSE;

	/* Was empty */
	if (ctl->rm_last_free_entry[ptr_lvl] == (uint16_t)TM_INVAL)
		ctl->rm_last_free_entry[ptr_lvl] = entry_ind;
	else
		ctl->rm_c_node_drop_profiles[cos][entry_ind].next_free_ind =
			ctl->rm_first_free_entry[ptr_lvl];

	ctl->rm_first_free_entry[ptr_lvl] = entry_ind;

	return 0;
}


/**
 */
int rm_free_port_drop_profile(rmctl_t hndl, uint16_t entry_ind)
{
	DECLARE_RM_HANDLE(ctl, hndl)
	CHECK_RM_HANDLE(ctl)

	if (entry_ind >= TM_NUM_PORT_DROP_PROF)
		return -EFAULT;

	if (ctl->rm_port_drop_profiles[entry_ind].used != RM_TRUE)
		return -ENOMSG;

	/* Insert as free entry */
	ctl->rm_port_drop_profiles[entry_ind].used = RM_FALSE;

	/* Was empty */
	if (ctl->rm_last_free_entry[RM_WRED_P_CURVE] == (uint16_t)TM_INVAL)
		ctl->rm_last_free_entry[RM_WRED_P_CURVE] = entry_ind;
	else
		ctl->rm_port_drop_profiles[entry_ind].next_free_ind =
			ctl->rm_first_free_entry[RM_WRED_P_CURVE];

	ctl->rm_first_free_entry[RM_WRED_P_CURVE] = entry_ind;

	return 0;
}


/* not used for HX/AX*/
/**
 */
int rm_free_port_drop_profile_cos(rmctl_t hndl, uint8_t cos, uint16_t entry_ind)
{
	enum rm_prf_level ptr_lvl;

	DECLARE_RM_HANDLE(ctl, hndl)
	CHECK_RM_HANDLE(ctl)

	if (!ctl)
		return -EINVAL;
	if (ctl->magic != RM_MAGIC)
		return -EBADF;

	if (entry_ind >= TM_NUM_PORT_DROP_PROF)
		return -EFAULT;

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
		return -EFAULT; /* cos >= TM_WRED_COS */
	}

	if (ctl->rm_port_drop_profiles_cos[cos][entry_ind].used != RM_TRUE)
		return -ENOMSG;

	/* Insert as free entry */
	ctl->rm_port_drop_profiles_cos[cos][entry_ind].used = RM_FALSE;

	/* Was empty */
	if (ctl->rm_last_free_entry[ptr_lvl] == (uint16_t)TM_INVAL)
		ctl->rm_last_free_entry[ptr_lvl] = entry_ind;
	else
		ctl->rm_port_drop_profiles_cos[cos][entry_ind].next_free_ind =
			ctl->rm_first_free_entry[ptr_lvl];

	ctl->rm_first_free_entry[ptr_lvl] = entry_ind;

	return 0;
}

