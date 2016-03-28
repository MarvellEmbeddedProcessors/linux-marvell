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

#include "tm_shaping.h"
#include "tm_defs.h"
#include "tm_errcodes.h"
#include "tm_locking_interface.h"
#include "rm_alloc.h"
#include "rm_status.h"
#include "rm_free.h"
#include "rm_list.h"
#include "rm_internal_types.h"

#include "tm_set_local_db_defaults.h"
#include "set_hw_registers.h"
#include "tm_nodes_utils.h"

int tm_fixed_port_shaping_change_status(tm_handle hndl, uint8_t status)
{
	int rc;

	DECLARE_TM_CTL_PTR(ctl, hndl)
	CHECK_TM_CTL_PTR(ctl)

	rc = tm_sched_lock(TM_ENV(ctl));
	if (rc)
		return rc;

	/* check parameters validity */
	if ((status != TM_ENABLE) && (status != TM_DISABLE)) {
		rc = -EACCES;
		goto out;
	}

	rc = set_hw_fixed_port_shaping_status((tm_handle)ctl, status);
	if (rc < 0)
		rc = TM_HW_CHANGE_SHAPING_STATUS_FAILED;
out:
	tm_sched_unlock(TM_ENV(ctl));
	return rc;
}


int tm_set_shaping_ex(tm_handle hndl,
					enum tm_level level,
					uint32_t index,
					uint32_t cbw,
					uint32_t ebw,
					uint32_t *pcbs,
					uint32_t *pebs
					)
{
	int rc = 0;
	uint8_t status;
	uint8_t type;
	uint8_t prio;
	uint8_t *elig_prio_func_ptr;

	DECLARE_TM_CTL_PTR(ctl, hndl)
	DECLARE_RM_HANDLE(rm, ctl->rm)
	CHECK_TM_CTL_PTR(ctl)

	rc = tm_sched_lock(TM_ENV(ctl));
	if (rc)
		return rc;

	/* check if queue/node exists */
	switch (level) {
	case Q_LEVEL:
		if (index >= rm->rm_total_queues) {
			rc = -EBADMSG;
			goto out;
		}
		break;
	case A_LEVEL:
		if (index >= rm->rm_total_a_nodes) {
			rc = -EBADMSG;
			goto out;
		}
		break;
	case B_LEVEL:
		if (index >= rm->rm_total_b_nodes) {
			rc = -EBADMSG;
			goto out;
		}
		break;
	case C_LEVEL:
		if (index >= rm->rm_total_c_nodes) {
			rc = -EBADMSG;
			goto out;
		}
		break;
	case P_LEVEL:
		if (index >= rm->rm_total_ports) {
			rc = -EBADMSG;
			goto out;
		}
		break;
	default:
		rc = -EACCES;
		goto out;
	}
	rc = rm_node_status(rm, level, index, &status);
	if ((rc) || (status != RM_TRUE)) {
		rc = -EPERM;
		goto out;
	}

	if ((cbw > TM_MAX_SHAPING_BW) || (ebw > TM_MAX_SHAPING_BW)) {
		rc = -EFAULT;
		goto out;
	}

	switch (level) {
	case Q_LEVEL:
		elig_prio_func_ptr = &(ctl->tm_queue_array[index].elig_prio_func_ptr);
		break;
	case A_LEVEL:
		elig_prio_func_ptr = &(ctl->tm_a_node_array[index].elig_prio_func_ptr);
		break;
	case B_LEVEL:
		elig_prio_func_ptr = &(ctl->tm_b_node_array[index].elig_prio_func_ptr);
		break;
	case C_LEVEL:
		elig_prio_func_ptr = &(ctl->tm_c_node_array[index].elig_prio_func_ptr);
		break;
	case P_LEVEL:
		elig_prio_func_ptr = &(ctl->tm_port_array[index].elig_prio_func_ptr);
		break;
	default:
		rc = -EACCES;
		goto out;
	}

	DECODE_ELIGIBLE_FUN(*elig_prio_func_ptr, type, prio);

	if (ebw) {
		/*  using extra b/w */
		if (level == Q_LEVEL) {
			if (IS_VALID_Q_TYPE_PRIO(type, prio))
				*elig_prio_func_ptr = ENCODE_ELIGIBLE_FUN(MINMAXTB_SHAPING, prio);
			else {
				/* if eligible function is invalid or disabled - it is set to  min/max TB  priority 0 */
				*elig_prio_func_ptr = ENCODE_ELIGIBLE_FUN(MINMAXTB_SHAPING, PRIO_0);
			}
		} else {
			if (IS_VALID_N_TYPE_PRIO(type, prio))
				*elig_prio_func_ptr = ENCODE_ELIGIBLE_FUN(MINMAXTB_SHAPING, prio);
			else {
				/* if eligible function is invalid or disabled - it is set to  min/max TB  priority 0 */
				*elig_prio_func_ptr = ENCODE_ELIGIBLE_FUN(MINMAXTB_SHAPING, PRIO_0);
			}
		}
	} else if (cbw) {
		/* using only min shaping */
		if (level == Q_LEVEL) {
			if (IS_VALID_Q_TYPE_PRIO(type, prio))
				*elig_prio_func_ptr = ENCODE_ELIGIBLE_FUN(MINTB_SHAPING, prio);
			else {
				/* if eligible function is invalid or disabled - it is set to  min/max TB  priority 0 */
				*elig_prio_func_ptr = ENCODE_ELIGIBLE_FUN(MINTB_SHAPING, PRIO_0);
			}
		} else {
			if (IS_VALID_N_TYPE_PRIO(type, prio))
				*elig_prio_func_ptr = ENCODE_ELIGIBLE_FUN(MINTB_SHAPING, prio);
			else {
				/* if eligible function is invalid or disabled - it is set to  min/max TB  priority 0 */
				*elig_prio_func_ptr = ENCODE_ELIGIBLE_FUN(MINTB_SHAPING, PRIO_0);
			}
		}
	} else {
		/* using fixed priority (without shaping) */
		if (level == Q_LEVEL) {
			if (IS_VALID_Q_TYPE_PRIO(type, prio))
				*elig_prio_func_ptr = ENCODE_ELIGIBLE_FUN(FIXED_PRIORITY, prio);
			else {
				/* if eligible function is invalid or disabled - it is set to  min/max TB  priority 0 */
				*elig_prio_func_ptr = ENCODE_ELIGIBLE_FUN(FIXED_PRIORITY, PRIO_0);
			}
		} else {
			if (IS_VALID_N_TYPE_PRIO(type, prio))
				*elig_prio_func_ptr = ENCODE_ELIGIBLE_FUN(FIXED_PRIORITY, prio);
			else {
				/* if eligible function is invalid or disabled - it is set to  min/max TB  priority 0 */
				*elig_prio_func_ptr = ENCODE_ELIGIBLE_FUN(FIXED_PRIORITY, PRIO_0);
			}
		}
	}
	/* Download to HW */
	switch (level) {
	case Q_LEVEL:
		rc = set_hw_queue_shaping_ex(ctl, index, cbw, ebw, pcbs, pebs);
		if (rc)
			goto out;
		rc = set_hw_queue_elig_prio_func_ptr(ctl, index);
		break;
	case A_LEVEL:
		rc = set_hw_a_node_shaping_ex(ctl, index, cbw, ebw, pcbs, pebs);
		if (rc)
			goto out;
		rc = set_hw_a_node_elig_prio_func_ptr(ctl, index);
		break;
	case B_LEVEL:
		rc = set_hw_b_node_shaping_ex(ctl, index, cbw, ebw, pcbs, pebs);
		if (rc)
			goto out;
		rc = set_hw_b_node_elig_prio_func_ptr(ctl, index);
		break;
	case C_LEVEL:
		rc = set_hw_c_node_shaping_ex(ctl, index, cbw, ebw, pcbs, pebs);
		if (rc)
			goto out;
		rc = set_hw_c_node_elig_prio_func_ptr(ctl, index);
		break;
	case P_LEVEL:
		rc = set_hw_port_shaping_ex(ctl, index, cbw, ebw, pcbs, pebs);
		if (rc)
			goto out;
		rc = set_hw_port_elig_prio_func_ptr(ctl, index);
		break;
	}
out:
	tm_sched_unlock(TM_ENV(ctl));
	return rc;
}


int tm_set_shaping(tm_handle hndl,
					enum tm_level level,
					uint32_t index,
					uint32_t cbw,
					uint32_t ebw)
{
	return tm_set_shaping_ex(hndl, level, index, cbw, ebw, NULL, NULL);
}

int tm_set_min_shaping(tm_handle hndl,
					enum tm_level level,
					uint32_t index,
					uint32_t cbw)
{
	return tm_set_shaping_ex(hndl, level, index, cbw, 0, NULL, NULL);
}

int tm_set_no_shaping(tm_handle hndl,
					enum tm_level level,
					uint32_t index)
{
	return tm_set_shaping_ex(hndl, level, index, 0, 0, NULL, NULL);
}




int tm_read_shaping(tm_handle hndl,
					enum tm_level level,
					uint32_t index,
					uint32_t *cir,
					uint32_t *eir,
					uint32_t *cbs,
					uint32_t *ebs
					)
{
	int rc = 0;
	uint8_t status;

	DECLARE_TM_CTL_PTR(ctl, hndl)
	DECLARE_RM_HANDLE(rm, ctl->rm)
	CHECK_TM_CTL_PTR(ctl)

	rc = tm_sched_lock(TM_ENV(ctl));
	if (rc)
		return rc;

	/* check if queue/node exists */
	switch (level) {
	case Q_LEVEL:
		if (index >= rm->rm_total_queues) {
			rc = -EBADMSG;
			goto out;
		}
		break;
	case A_LEVEL:
		if (index >= rm->rm_total_a_nodes) {
			rc = -EBADMSG;
			goto out;
		}
		break;
	case B_LEVEL:
		if (index >= rm->rm_total_b_nodes) {
			rc = -EBADMSG;
			goto out;
		}
		break;
	case C_LEVEL:
		if (index >= rm->rm_total_c_nodes) {
			rc = -EBADMSG;
			goto out;
		}
		break;
	case P_LEVEL:
		if (index >= rm->rm_total_ports) {
			rc = -EBADMSG;
			goto out;
		}
		break;
	default:
		rc = -EACCES;
		goto out;
	}
	rc = rm_node_status(rm, level, index, &status);
	if ((rc) || (status != RM_TRUE)) {
		rc = -EPERM;
		goto out;
	}

	switch (level) {
	case Q_LEVEL:
		rc = get_hw_queue_shaping(hndl, index, cir, eir, cbs, ebs);
		break;
	case A_LEVEL:
		rc = get_hw_a_node_shaping(hndl, index, cir, eir, cbs, ebs);
		break;
	case B_LEVEL:
		rc = get_hw_b_node_shaping(hndl, index, cir, eir, cbs, ebs);
		break;
	case C_LEVEL:
		rc = get_hw_c_node_shaping(hndl, index, cir, eir, cbs, ebs);
		break;
	case P_LEVEL:
		rc = get_hw_port_shaping(hndl, index, cir, eir, cbs, ebs);
		break;
	default:
		rc = -EACCES;
		goto out;
	}

out:
	tm_sched_unlock(TM_ENV(ctl));
	return rc;
}
