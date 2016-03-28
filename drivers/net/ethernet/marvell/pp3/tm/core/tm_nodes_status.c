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

#include "tm_nodes_status.h"
#include "rm_internal_types.h"
#include "rm_status.h"
#include "tm_errcodes.h"
#include "tm_nodes_utils.h"
#include "tm_locking_interface.h"
#include "set_hw_registers.h"


/**
 */
int tm_read_port_status(tm_handle hndl,
						uint8_t index,
						struct tm_port_status *tm_status)
{
	uint8_t status;
	int rc;

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
	if ((rc) || (status != RM_TRUE))
	{
		rc = -ENODATA;
		goto out;
	}

	rc = get_hw_port_status(ctl, index, tm_status);
	if (rc)
		rc = TM_HW_READ_PORT_STATUS_FAIL;

out:
	tm_nodes_unlock(TM_ENV(ctl));
	return rc;
}


/**
 */
int tm_read_c_node_status(tm_handle hndl,
						  uint32_t index,
						  struct tm_node_status *tm_status)
{
	uint8_t status;
	int rc;

	DECLARE_TM_CTL_PTR(ctl, hndl)
	DECLARE_RM_HANDLE(rm, ctl->rm)
	CHECK_TM_CTL_PTR(ctl);

	rc = tm_nodes_lock(TM_ENV(ctl));
	if (rc)
		return rc;

	if (index >= rm->rm_total_c_nodes)
	{
		rc = -EFAULT;
		goto out;
	}

	rc = rm_node_status(rm, C_LEVEL, index, &status);
	if ((rc) || (status != RM_TRUE))
	{
		rc = -ENODATA;
		goto out;
	}

	rc = get_hw_c_node_status(ctl, index, tm_status);
	if (rc)
		rc = TM_HW_READ_C_NODE_STATUS_FAIL;
out:
	tm_nodes_unlock(TM_ENV(ctl));
	return rc;
}


/**
 */
int tm_read_b_node_status(tm_handle hndl,
						  uint32_t index,
						  struct tm_node_status *tm_status)
{
	uint8_t status;
	int rc;

	DECLARE_TM_CTL_PTR(ctl, hndl)
	DECLARE_RM_HANDLE(rm, ctl->rm)
	CHECK_TM_CTL_PTR(ctl);

	rc = tm_nodes_lock(TM_ENV(ctl));
	if (rc)
		return rc;

	if (index >= rm->rm_total_b_nodes)
	{
		rc = -EFAULT;
		goto out;
	}

	rc = rm_node_status(rm, B_LEVEL, index, &status);
	if ((rc) || (status != RM_TRUE))
	{
		rc = -ENODATA;
		goto out;
	}

	rc = get_hw_b_node_status(ctl, index, tm_status);
	if (rc)
		rc = TM_HW_READ_B_NODE_STATUS_FAIL;

out:
	tm_nodes_unlock(TM_ENV(ctl));
	return rc;
}


/**
 */
int tm_read_a_node_status(tm_handle hndl,
						  uint32_t index,
						  struct tm_node_status *tm_status)
{
	uint8_t status;
	int rc;

	DECLARE_TM_CTL_PTR(ctl, hndl)
	DECLARE_RM_HANDLE(rm, ctl->rm)
	CHECK_TM_CTL_PTR(ctl);

	rc = tm_nodes_lock(TM_ENV(ctl));
	if (rc)
		return rc;

	if (index >= rm->rm_total_a_nodes)
	{
		rc = -EFAULT;
		goto out;
	}

	rc = rm_node_status(rm, A_LEVEL, index, &status);
	if ((rc) || (status != RM_TRUE))
	{
		rc = -ENODATA;
		goto out;
	}


	rc = get_hw_a_node_status(ctl, index, tm_status);
	if (rc)
		rc = TM_HW_READ_A_NODE_STATUS_FAIL;

out:
	tm_nodes_unlock(TM_ENV(ctl));
	return rc;
}


/**
 */
int tm_read_queue_status(tm_handle hndl,
						 uint32_t index,
						 struct tm_node_status *tm_status)
{
	uint8_t status;
	int rc;

	DECLARE_TM_CTL_PTR(ctl, hndl)
	DECLARE_RM_HANDLE(rm, ctl->rm)
	CHECK_TM_CTL_PTR(ctl);

	rc = tm_nodes_lock(TM_ENV(ctl));
	if (rc)
		return rc;

	if (index >= rm->rm_total_queues) {
		rc = -EFAULT;
		goto out;
	}

	rc = rm_node_status(rm, Q_LEVEL, index, &status);
	if ((rc) || (status != RM_TRUE)) {
		rc = -ENODATA;
		goto out;
	}

	rc = get_hw_queue_status(ctl, index, tm_status);
	if (rc)
		rc = TM_HW_READ_QUEUE_STATUS_FAIL;

out:
	tm_nodes_unlock(TM_ENV(ctl));
	return rc;
}


/**
 */
int tm_drop_get_queue_length(tm_handle hndl,
							 enum tm_level level,
							 uint32_t index,
							 uint32_t *av_q_length)
{
	uint8_t status;
	int rc;

	DECLARE_TM_CTL_PTR(ctl, hndl)
	DECLARE_RM_HANDLE(rm, ctl->rm)
	CHECK_TM_CTL_PTR(ctl);

	rc = tm_nodes_lock(TM_ENV(ctl));
	if (rc)
		return rc;

	switch (level)
	{
	case Q_LEVEL:
		if (index >= rm->rm_total_queues)
			rc = -EFAULT;
		break;
	case A_LEVEL:
		if (index >= rm->rm_total_a_nodes)
			rc = -EFAULT;
		break;
	case B_LEVEL:
		if (index >= rm->rm_total_b_nodes)
			rc = -EFAULT;
		break;
	case C_LEVEL:
		if (index >= rm->rm_total_c_nodes)
			rc = -EFAULT;
		break;
	case P_LEVEL:
		if (index >= rm->rm_total_ports)
			rc = -EFAULT;
		break;
	default:
		rc = -ERANGE;
	}
	if (rc)
		goto out;

	rc = rm_node_status(rm, level, index, &status);
	if ((rc) || (status != RM_TRUE))
	{
		rc = -ENODATA;
		goto out;
	}

	rc = get_hw_queue_length(ctl, level, index,
							av_q_length);
	if (rc)
		rc = TM_HW_GET_QUEUE_LENGTH_FAIL;

out:
	tm_nodes_unlock(TM_ENV(ctl));
	return rc;
}

