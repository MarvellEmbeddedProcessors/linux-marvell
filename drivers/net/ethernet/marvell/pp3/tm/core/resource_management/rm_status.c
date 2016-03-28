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
#include "rm_status.h"
#include "tm/core/tm_core_types.h"
#include "tm/core/tm_hw_configuration_interface.h"


/**
 */
int rm_node_status(rmctl_t hndl,
					enum tm_level lvl,
					uint32_t node_ind,
					uint8_t *status)
{
	DECLARE_RM_HANDLE(ctl, hndl)
	CHECK_RM_HANDLE(ctl)


	switch (lvl) {
	case P_LEVEL:
		if (node_ind >= ctl->rm_total_ports)
			return  -EFAULT;
		*status = ctl->rm_port_array[node_ind].used;
		break;
	case C_LEVEL:
		if (node_ind >= ctl->rm_total_c_nodes)
			return  -EFAULT;
		*status = ctl->rm_c_node_array[node_ind].used;
		break;
	case B_LEVEL:
		if (node_ind >= ctl->rm_total_b_nodes)
			return  -EFAULT;
		*status = ctl->rm_b_node_array[node_ind].used;
		break;
	case A_LEVEL:
		if (node_ind >= ctl->rm_total_a_nodes)
			return  -EFAULT;
		*status = ctl->rm_a_node_array[node_ind].used;
		break;
	case Q_LEVEL:
		if (node_ind >= ctl->rm_total_queues)
			return  -EFAULT;
		*status = ctl->rm_queue_array[node_ind].used;
		break;
	default:
		return -ERANGE;
	}

	return 0;
}


/**
 */
int rm_wred_queue_curve_status(rmctl_t hndl,
								uint8_t entry_ind,
								uint8_t *status)
{
	DECLARE_RM_HANDLE(ctl, hndl)
	CHECK_RM_HANDLE(ctl)


	if (entry_ind >= TM_NUM_WRED_QUEUE_CURVES)
		return -EFAULT;

	*status = ctl->rm_wred_queue_curves[entry_ind].used;
	return 0;
}


/**
 */
int rm_wred_a_node_curve_status(rmctl_t hndl,
								uint8_t entry_ind,
								uint8_t *status)
{
	DECLARE_RM_HANDLE(ctl, hndl)
	CHECK_RM_HANDLE(ctl)


	if (entry_ind >= TM_NUM_WRED_A_NODE_CURVES)
		return -EFAULT;

	*status = ctl->rm_wred_a_node_curves[entry_ind].used;
	return 0;
}


/**
 */
int rm_wred_b_node_curve_status(rmctl_t hndl,
								uint8_t entry_ind,
								uint8_t *status)
{
	DECLARE_RM_HANDLE(ctl, hndl)
	CHECK_RM_HANDLE(ctl)


	if (entry_ind >= TM_NUM_WRED_B_NODE_CURVES)
		return -EFAULT;

	*status = ctl->rm_wred_b_node_curves[entry_ind].used;
	return 0;
}


/**
 */
int rm_wred_c_node_curve_status(rmctl_t hndl,
								uint8_t cos,
								uint8_t entry_ind,
								uint8_t *status)
{
	DECLARE_RM_HANDLE(ctl, hndl)
	CHECK_RM_HANDLE(ctl)


	if (cos >= TM_WRED_COS)
		return -EFAULT;

	if (entry_ind >= TM_NUM_WRED_C_NODE_CURVES)
		return -EFAULT;

	*status = ctl->rm_wred_c_node_curves[cos][entry_ind].used;
	return 0;
}


/**
 */
int rm_wred_port_curve_status(rmctl_t hndl,
								uint8_t entry_ind,
								uint8_t *status)
{
	DECLARE_RM_HANDLE(ctl, hndl)
	CHECK_RM_HANDLE(ctl)


	if (entry_ind >= TM_NUM_WRED_PORT_CURVES)
		return -EFAULT;

	*status = ctl->rm_wred_port_curves[entry_ind].used;
	return 0;
}


/**
 */
int rm_wred_port_curve_status_cos(rmctl_t hndl,
									uint8_t cos,
									uint8_t entry_ind,
									uint8_t *status)
{
	struct rmctl *ctl = hndl;

	if (!ctl)
		return -EINVAL;
	if (ctl->magic != RM_MAGIC)
		return -EBADF;

	if (cos >= TM_WRED_COS)
		return -EFAULT;

	if (entry_ind >= TM_NUM_WRED_PORT_CURVES)
		return -EFAULT;

	*status = ctl->rm_wred_port_curves_cos[cos][entry_ind].used;
	return 0;
}



/**
 */
int rm_queue_drop_profile_status(rmctl_t hndl,
								uint16_t entry_ind,
								uint8_t *status)
{
	DECLARE_RM_HANDLE(ctl, hndl)
	CHECK_RM_HANDLE(ctl)


	if (entry_ind >= TM_NUM_QUEUE_DROP_PROF)
		return -EFAULT;

	*status = ctl->rm_queue_drop_profiles[entry_ind].used;
	return 0;
}


/**
 */
int rm_a_node_drop_profile_status(rmctl_t hndl,
									uint16_t entry_ind,
									uint8_t *status)
{
	DECLARE_RM_HANDLE(ctl, hndl)
	CHECK_RM_HANDLE(ctl)


	if (entry_ind >= TM_NUM_A_NODE_DROP_PROF)
		return -EFAULT;

	*status = ctl->rm_a_node_drop_profiles[entry_ind].used;
	return 0;
}


/**
 */
int rm_b_node_drop_profile_status(rmctl_t hndl,
									uint16_t entry_ind,
									uint8_t *status)
{
	DECLARE_RM_HANDLE(ctl, hndl)
	CHECK_RM_HANDLE(ctl)


	if (entry_ind >= TM_NUM_B_NODE_DROP_PROF)
		return -EFAULT;

	*status = ctl->rm_b_node_drop_profiles[entry_ind].used;
	return 0;
}


/**
 */
int rm_c_node_drop_profile_status(rmctl_t hndl,
									uint8_t cos,
									uint16_t entry_ind,
									uint8_t *status)
{
	DECLARE_RM_HANDLE(ctl, hndl)
	CHECK_RM_HANDLE(ctl)


	if (cos >= TM_WRED_COS)
		return -EFAULT;

	if (entry_ind >= TM_NUM_C_NODE_DROP_PROF)
		return -EFAULT;

	*status = ctl->rm_c_node_drop_profiles[cos][entry_ind].used;
	return 0;
}


/**
 */
int rm_port_drop_profile_status(rmctl_t hndl,
								uint16_t entry_ind,
								uint8_t *status)
{
	DECLARE_RM_HANDLE(ctl, hndl)
	CHECK_RM_HANDLE(ctl)

	if (entry_ind >= TM_NUM_PORT_DROP_PROF)
		return -EFAULT;

	*status = ctl->rm_port_drop_profiles[entry_ind].used;
	return 0;
}


/**
 */
int rm_port_drop_profile_status_cos(rmctl_t hndl,
									uint8_t cos,
									uint16_t entry_ind,
									uint8_t *status)
{
	struct rmctl *ctl = hndl;

	if (!ctl)
		return -EINVAL;
	if (ctl->magic != RM_MAGIC)
		return -EBADF;

	if (entry_ind >= TM_NUM_PORT_DROP_PROF)
		return -EFAULT;

	if (cos >= TM_WRED_COS)
		return -EFAULT;

	*status = ctl->rm_port_drop_profiles_cos[cos][entry_ind].used;
	return 0;
}

