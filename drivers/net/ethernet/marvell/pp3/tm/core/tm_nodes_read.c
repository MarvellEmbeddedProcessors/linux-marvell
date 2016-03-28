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

#include "tm_nodes_read.h"
#include "tm_errcodes.h"
#include "tm_nodes_utils.h"
#include "tm_os_interface.h"
#include "tm_locking_interface.h"

#include "rm_internal_types.h"
#include "rm_status.h"
#include "set_hw_registers.h"


/***************************************************************************
 * Read Configuration
 ***************************************************************************/

/**
 */
int tm_read_queue_configuration(tm_handle hndl, uint32_t queue_index,
								struct tm_queue_params *params)
{
	struct tm_queue *node = NULL;
	uint8_t status;
	int rc;

	DECLARE_TM_CTL_PTR(ctl, hndl)
	DECLARE_RM_HANDLE(rm, ctl->rm)
	CHECK_TM_CTL_PTR(ctl);

	rc = tm_nodes_lock(TM_ENV(ctl));
	if (rc)
		return rc;

	/* Check parameters validity */
	if (queue_index >= rm->rm_total_queues) {
		rc = -EFAULT;
		goto out;
	}

	rc = rm_node_status(ctl->rm, Q_LEVEL, queue_index, &status);
	if ((rc) || (status != RM_TRUE)) {
		rc = -ENODATA;
		goto out;
	}

	node = &(ctl->tm_queue_array[queue_index]);
	params->quantum = node->dwrr_quantum;
	params->wred_profile_ref = node->wred_profile_ref;
	params->elig_prio_func_ptr = node->elig_prio_func_ptr;
out:
	tm_nodes_unlock(TM_ENV(ctl));
	return rc;
}


/**
 */
int tm_read_a_node_configuration(tm_handle hndl, uint32_t node_index,
								struct tm_a_node_params *params,
								uint32_t *p_first_child,
								uint32_t *p_last_child)
{
	struct tm_a_node *node = NULL;
	uint8_t status;
	int rc = 0;
	int i;

	DECLARE_TM_CTL_PTR(ctl, hndl)
	DECLARE_RM_HANDLE(rm, ctl->rm)
	CHECK_TM_CTL_PTR(ctl);

	rc = tm_nodes_lock(TM_ENV(ctl));
	if (rc)
		return rc;

	/* Check parameters validity */
	if (node_index >= rm->rm_total_a_nodes) {
		rc = -EFAULT;
		goto out;
	}

	rc = rm_node_status(rm, A_LEVEL, node_index, &status);
	if ((rc) || (status != RM_TRUE)) {
		rc = -ENODATA;
		goto out;
	}

	node = &(ctl->tm_a_node_array[node_index]);
	params->quantum = node->dwrr_quantum;
	for (i = 0; i < 8; i++) {
		if ((node->dwrr_priority & (1 << i)) != 0)
			params->dwrr_priority[i] = TM_ENABLE;
		else
			params->dwrr_priority[i] = TM_DISABLE;
	}
	params->wred_profile_ref = node->wred_profile_ref;
	params->elig_prio_func_ptr = node->elig_prio_func_ptr;
	params->num_of_children = node->last_child_queue - node->first_child_queue + 1;

	*p_first_child = node->first_child_queue;
	*p_last_child = node->last_child_queue;
out:
	tm_nodes_unlock(TM_ENV(ctl));
	return rc;
}


/**
 */
int tm_read_b_node_configuration(tm_handle hndl, uint32_t node_index,
								struct tm_b_node_params *params,
								uint32_t *p_first_child,
								uint32_t *p_last_child)
{
	struct tm_b_node *node = NULL;
	uint8_t status;
	int rc = 0;
	int i;

	DECLARE_TM_CTL_PTR(ctl, hndl)
	DECLARE_RM_HANDLE(rm, ctl->rm)
	CHECK_TM_CTL_PTR(ctl);

	rc = tm_nodes_lock(TM_ENV(ctl));
	if (rc)
		return rc;

	/* Check parameters validity */
	if (node_index >= rm->rm_total_b_nodes) {
		rc = -EFAULT;
		goto out;
	}

	rc = rm_node_status(rm, B_LEVEL, node_index, &status);
	if ((rc) || (status != RM_TRUE)) {
		rc = -ENODATA;
		goto out;
	}

	node = &(ctl->tm_b_node_array[node_index]);
	params->quantum = node->dwrr_quantum;
	for (i = 0; i < 8; i++) {
		if ((node->dwrr_priority & (1 << i)) != 0)
			params->dwrr_priority[i] = TM_ENABLE;
		else
			params->dwrr_priority[i] = TM_DISABLE;
	}
	params->wred_profile_ref = node->wred_profile_ref;
	params->elig_prio_func_ptr = node->elig_prio_func_ptr;
	params->num_of_children = node->last_child_a_node - node->first_child_a_node + 1;
	*p_first_child = node->first_child_a_node;
	*p_last_child = node->last_child_a_node;
out:
	tm_nodes_unlock(TM_ENV(ctl));
	return rc;
}


/**
 */
int tm_read_c_node_configuration(tm_handle hndl, uint32_t node_index,
								struct tm_c_node_params *params,
								uint32_t *p_first_child,
								uint32_t *p_last_child)
{
	struct tm_c_node *node = NULL;
	uint8_t status;
	int rc = 0;
	int i;

	DECLARE_TM_CTL_PTR(ctl, hndl)
	DECLARE_RM_HANDLE(rm, ctl->rm)
	CHECK_TM_CTL_PTR(ctl);

	rc = tm_nodes_lock(TM_ENV(ctl));
	if (rc)
		return rc;

	/* Check parameters validity */
	if (node_index >= rm->rm_total_c_nodes) {
		rc = -EFAULT;
		goto out;
	}

	rc = rm_node_status(rm, C_LEVEL, node_index, &status);
	if ((rc) || (status != RM_TRUE)) {
		rc = -ENODATA;
		goto out;
	}

	node = &(ctl->tm_c_node_array[node_index]);
	params->quantum = node->dwrr_quantum;
	for (i = 0; i < 8; i++)
		if ((node->dwrr_priority & (1 << i)) != 0)
			params->dwrr_priority[i] = TM_ENABLE;
		else
			params->dwrr_priority[i] = TM_DISABLE;
	params->wred_cos = node->wred_cos;
	for (i = 0; i < TM_WRED_COS; i++)
		params->wred_profile_ref[i] = node->wred_profile_ref[i];
	params->elig_prio_func_ptr = node->elig_prio_func_ptr;
	params->num_of_children = node->last_child_b_node - node->first_child_b_node + 1;

	*p_first_child = node->first_child_b_node;
	*p_last_child = node->last_child_b_node;

out:
	tm_nodes_unlock(TM_ENV(ctl));
	return rc;
}

/**
 */
int tm_read_port_configuration(tm_handle hndl, uint32_t port_index,
							struct tm_port_params *params,
							struct tm_port_drop_per_cos *cos_params,
							uint32_t *p_first_child,
							uint32_t *p_last_child)
{
	struct tm_port *port = NULL;
	uint8_t status;
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
		rc = -EFAULT;
		goto out;
	}

	rc = rm_node_status(ctl->rm, P_LEVEL, port_index, &status);
	if ((rc) || (status != RM_TRUE)) {
		rc = -ENODATA;
		goto out;
	}

	port = &(ctl->tm_port_array[port_index]);

	for (i = 0; i < 8; i++) {
		if ((port->dwrr_priority & (1 << i)) != 0)
			params->dwrr_priority[i] = TM_ENABLE;
		else
			params->dwrr_priority[i] = TM_DISABLE;
	}

	params->elig_prio_func_ptr = port->elig_prio_func_ptr;
	params->wred_profile_ref = port->wred_profile_ref;

#ifdef MV_QMTM_NSS_A0
	for (i = 0; i < 8; i++)
		params->quantum[i] = port->dwrr_quantum[i].quantum;
#endif

	cos_params->wred_cos = port->wred_cos;
	for (i = 0; i < TM_WRED_COS; i++)
		cos_params->wred_profile_ref[i] = port->wred_profile_ref_cos[i];
	params->num_of_children = port->last_child_c_node - port->first_child_c_node + 1;
	*p_first_child = port->first_child_c_node;
	*p_last_child = port->last_child_c_node;

out:
	tm_nodes_unlock(TM_ENV(ctl));
	return rc;
}

/**
 */
int tm_read_queue_configuration_hw(tm_handle hndl, uint32_t index,
								struct tm_queue_params *params)
{
	struct tm_queue node = {0};
	int rc;

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

	rc = get_hw_queue(ctl, index, &node);
	if (rc)
		goto out;
	params->quantum = node.dwrr_quantum;
	params->wred_profile_ref = node.wred_profile_ref;
	params->elig_prio_func_ptr = node.elig_prio_func_ptr;
out:
	tm_nodes_unlock(TM_ENV(ctl));
	return rc;
}


/**
 */
int tm_read_a_node_configuration_hw(tm_handle hndl, uint32_t node_index,
								struct tm_a_node_params *params,
								uint32_t *p_first_child,
								uint32_t *p_last_child)
{
	struct tm_a_node node = {0};
	int rc = 0;
	int i;

	DECLARE_TM_CTL_PTR(ctl, hndl)
	DECLARE_RM_HANDLE(rm, ctl->rm)
	CHECK_TM_CTL_PTR(ctl);

	rc = tm_nodes_lock(TM_ENV(ctl));
	if (rc)
		return rc;

	/* Check parameters validity */
	if (node_index >= rm->rm_total_a_nodes) {
		rc = -EFAULT;
		goto out;
	}

	rc = get_hw_a_node(ctl, node_index, &node);
	if (rc)
		goto out;
	params->quantum = node.dwrr_quantum;
	for (i = 0; i < 8; i++) {
		if ((node.dwrr_priority & (1 << i)) != 0)
			params->dwrr_priority[i] = TM_ENABLE;
		else
			params->dwrr_priority[i] = TM_DISABLE;
	}
	params->wred_profile_ref = node.wred_profile_ref;
	params->elig_prio_func_ptr = node.elig_prio_func_ptr;
	params->num_of_children = node.last_child_queue - node.first_child_queue + 1;

	*p_first_child = node.first_child_queue;
	*p_last_child = node.last_child_queue;

out:
	tm_nodes_unlock(TM_ENV(ctl));
	return rc;
}


/**
 */
int tm_read_b_node_configuration_hw(tm_handle hndl, uint32_t node_index,
								struct tm_b_node_params *params,
								uint32_t *p_first_child,
								uint32_t *p_last_child)
{
	struct tm_b_node node = {0};
	int rc = 0;
	int i;

	DECLARE_TM_CTL_PTR(ctl, hndl)
	DECLARE_RM_HANDLE(rm, ctl->rm)
	CHECK_TM_CTL_PTR(ctl);

	rc = tm_nodes_lock(TM_ENV(ctl));
	if (rc)
		return rc;

	/* Check parameters validity */
	if (node_index >= rm->rm_total_b_nodes) {
		rc = -EFAULT;
		goto out;
	}

	rc = get_hw_b_node(ctl, node_index, &node);
	if (rc)
		goto out;
	params->quantum = node.dwrr_quantum;
	for (i = 0; i < 8; i++) {
		if ((node.dwrr_priority & (1 << i)) != 0)
			params->dwrr_priority[i] = TM_ENABLE;
		else
			params->dwrr_priority[i] = TM_DISABLE;
	}
	params->wred_profile_ref = node.wred_profile_ref;
	params->elig_prio_func_ptr = node.elig_prio_func_ptr;
	params->num_of_children = node.last_child_a_node - node.first_child_a_node + 1;

	*p_first_child = node.first_child_a_node;
	*p_last_child = node.last_child_a_node;
out:
	tm_nodes_unlock(TM_ENV(ctl));
	return rc;
}


/**
 */
int tm_read_c_node_configuration_hw(tm_handle hndl, uint32_t node_index,
								struct tm_c_node_params *params,
								uint32_t *p_first_child,
								uint32_t *p_last_child)
{
	struct tm_c_node node = {0};
	int rc = 0;
	int i;

	DECLARE_TM_CTL_PTR(ctl, hndl)
	DECLARE_RM_HANDLE(rm, ctl->rm)
	CHECK_TM_CTL_PTR(ctl);

	rc = tm_nodes_lock(TM_ENV(ctl));
	if (rc)
		return rc;

	/* Check parameters validity */
	if (node_index >= rm->rm_total_c_nodes) {
		rc = -EFAULT;
		goto out;
	}

	rc = get_hw_c_node(ctl, node_index, &node);
	if (rc)
		goto out;
	params->quantum = node.dwrr_quantum;
	for (i = 0; i < 8; i++)
		if ((node.dwrr_priority & (1 << i)) != 0)
			params->dwrr_priority[i] = TM_ENABLE;
		else
			params->dwrr_priority[i] = TM_DISABLE;
	for (i = 0; i < TM_WRED_COS; i++)
		params->wred_profile_ref[i] = node.wred_profile_ref[i];
	params->elig_prio_func_ptr = node.elig_prio_func_ptr;
	params->num_of_children = node.last_child_b_node - node.first_child_b_node + 1;

	*p_first_child = node.first_child_b_node;
	*p_last_child = node.last_child_b_node;

out:
	tm_nodes_unlock(TM_ENV(ctl));
	return rc;
}

/**
 */
int tm_read_port_configuration_hw(tm_handle hndl, uint32_t port_index,
							struct tm_port_params *params,
							struct tm_port_drop_per_cos *cos_params,
							uint32_t *p_first_child,
							uint32_t *p_last_child)
{
	struct tm_port port = {0};
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
		rc = -EFAULT;
		goto out;
	}

	rc = get_hw_port(ctl, port_index, &port);
	if (rc)
		goto out;

	for (i = 0; i < 8; i++) {
		if ((port.dwrr_priority & (1 << i)) != 0)
			params->dwrr_priority[i] = TM_ENABLE;
		else
			params->dwrr_priority[i] = TM_DISABLE;
	}

	params->elig_prio_func_ptr = port.elig_prio_func_ptr;
	params->num_of_children = port.last_child_c_node - port.first_child_c_node + 1;


	*p_first_child = port.first_child_c_node;
	*p_last_child = port.last_child_c_node;

#ifdef MV_QMTM_NSS_A0
	for (i = 0; i < 8; i++)
		params->quantum[i] = port.dwrr_quantum[i].quantum;
#endif

	/* No Drop Profile Ptrs in HW, will be replaced by SW */
	params->wred_profile_ref = ctl->tm_port_array[port_index].wred_profile_ref;
	cos_params->wred_cos = ctl->tm_port_array[port_index].wred_cos;
	for (i = 0; i < TM_WRED_COS; i++)
		cos_params->wred_profile_ref[i] = ctl->tm_port_array[port_index].wred_profile_ref_cos[i];

out:
	tm_nodes_unlock(TM_ENV(ctl));
	return rc;
}
