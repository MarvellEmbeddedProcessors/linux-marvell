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

#include "tm_elig_prio_func.h"
#include "tm_errcodes.h"
#include "set_hw_registers.h"
#include "tm_set_local_db_defaults.h"
#include "tm_locking_interface.h"
#include "rm_status.h"


/* Auxiliary function */
static uint16_t convert_elig_func_to_value(struct tm_elig_prio_func_out *elig_func)
{
	uint16_t var;

	var = elig_func->max_tb;
	var = var | (elig_func->min_tb << 1);
	var = var | (elig_func->prop_prio << 2);
	var = var | (elig_func->sched_prio << 5);
	var = var | (elig_func->elig << 8);

	return var;
}

#if 0
static void convert_value_to_elig_func(uint16_t elig_val, struct tm_elig_prio_func_out *elig_func)
{
	/*
	FuncOut[8]   - Elig.
	FuncOut[7:5] - Scheduling Priority.
	FuncOut[4:2] - Propagated Priority.
	FuncOut[1]   - Use Min Token Bucket.
	FuncOut[0]   - Use Max Token Bucket.
	*/
	uint8_t mask = 0x07;

	elig_func->elig = (elig_val >> 8) & 0x01;
	elig_func->max_tb = elig_val & 0x01;
	elig_func->min_tb = elig_val & 0x02;
	elig_func->prop_prio = (elig_val >> 2) & mask;
	elig_func->sched_prio = (elig_val >> 5) & mask;
}
#endif

static void set_sw_q_elig_prio_func(struct tm_elig_prio_func_queue *func_table,
									uint16_t func_offset,
									union tm_elig_prio_func *queue_func_out_arr)
{
	func_table[func_offset].tbl_entry.func_out[0] =
		convert_elig_func_to_value(&(queue_func_out_arr->queue_elig_prio_func[0]));
	func_table[func_offset].tbl_entry.func_out[1] =
		convert_elig_func_to_value(&(queue_func_out_arr->queue_elig_prio_func[1]));
	func_table[func_offset].tbl_entry.func_out[2] =
		convert_elig_func_to_value(&(queue_func_out_arr->queue_elig_prio_func[2]));
	func_table[func_offset].tbl_entry.func_out[3] =
		convert_elig_func_to_value(&(queue_func_out_arr->queue_elig_prio_func[3]));
}

/**
 */
static void set_sw_n_elig_prio_func(struct tm_elig_prio_func_node *func_table,
									uint16_t func_offset,
									union tm_elig_prio_func *node_func_out_arr)
{
	int i;
	for (i = 0; i < 8; i++)	{
		/* Entry ID */
		func_table[func_offset].tbl_entry[i].func_out[0] =
			convert_elig_func_to_value(&(node_func_out_arr->node_elig_prio_func[i][0]));
		func_table[func_offset].tbl_entry[i].func_out[1] =
			convert_elig_func_to_value(&(node_func_out_arr->node_elig_prio_func[i][1]));
		func_table[func_offset].tbl_entry[i].func_out[2] =
			convert_elig_func_to_value(&(node_func_out_arr->node_elig_prio_func[i][2]));
		func_table[func_offset].tbl_entry[i].func_out[3] =
			convert_elig_func_to_value(&(node_func_out_arr->node_elig_prio_func[i][3]));
	}
}
/**
 */
int tm_elig_prio_func_config(tm_handle hndl,
							uint16_t elig_prio_func_index,
							enum tm_level level,
							union tm_elig_prio_func *func_value_arr)
{
	int rc = 0;

	DECLARE_TM_CTL_PTR(ctl, hndl)
	CHECK_TM_CTL_PTR(ctl)

	if (elig_prio_func_index >= TM_ELIG_FUNC_TABLE_SIZE)
		return -EDOM;
	/* TM_ELIG_Q_DEQ_DIS function udate is not allowed */
	if (elig_prio_func_index == TM_ELIG_DEQ_DISABLE)
		return -EDOM;
	/* TM_NODE_DISABLED_FUN  slot is for internal use */
	if (elig_prio_func_index == TM_NODE_DISABLED_FUN)
		return -EDOM;

	switch (level)
	{
		case Q_LEVEL:	{
			/* Update SW image for queue level */
			set_sw_q_elig_prio_func(ctl->tm_elig_prio_q_lvl_tbl, elig_prio_func_index, func_value_arr);
			/* update HW */
			rc = set_hw_q_elig_prio_func_entry(hndl, elig_prio_func_index);
			break;
		}
		case A_LEVEL:	{
			/* Update SW image for A node level */
			set_sw_n_elig_prio_func(ctl->tm_elig_prio_a_lvl_tbl, elig_prio_func_index, func_value_arr);
			/* update HW */
			rc = set_hw_a_lvl_elig_prio_func_entry(hndl, elig_prio_func_index);
			break;
		}
		case B_LEVEL:	{
			/* Update SW image for B node level */
			set_sw_n_elig_prio_func(ctl->tm_elig_prio_b_lvl_tbl, elig_prio_func_index, func_value_arr);
			/* update HW */
			rc = set_hw_b_lvl_elig_prio_func_entry(hndl, elig_prio_func_index);
			break;
		}
		case C_LEVEL:	{
			/* Update SW image for C node level */
			set_sw_n_elig_prio_func(ctl->tm_elig_prio_c_lvl_tbl, elig_prio_func_index, func_value_arr);
			/* update HW */
			rc = set_hw_c_lvl_elig_prio_func_entry(hndl, elig_prio_func_index);
			break;
		}
		case P_LEVEL:	{
			/* Update SW image for Port level */
			set_sw_n_elig_prio_func(ctl->tm_elig_prio_p_lvl_tbl, elig_prio_func_index, func_value_arr);
			/* update HW */
			rc = set_hw_p_lvl_elig_prio_func_entry(hndl, elig_prio_func_index);
			break;
		}
		default: return -EADDRNOTAVAIL;
	}
	if (rc)
		rc = -TM_HW_ELIG_PRIO_FUNC_FAILED;
	return rc;
}

/**
 */
int tm_elig_prio_func_config_all_levels(tm_handle hndl,
									uint16_t elig_prio_func_ptr,
									union tm_elig_prio_func *func_value_arr)
{

	enum tm_level lvl;
	int rc = 0;

	DECLARE_TM_CTL_PTR(ctl, hndl)
	CHECK_TM_CTL_PTR(ctl)

	if (elig_prio_func_ptr >= TM_ELIG_FUNC_TABLE_SIZE)	{
		rc = -EDOM;
		goto out;
	}

	for (lvl = A_LEVEL; lvl <= P_LEVEL; lvl++)	{
		rc = tm_elig_prio_func_config(ctl, elig_prio_func_ptr, lvl, func_value_arr);
		if (rc)	{
			rc = -TM_HW_ELIG_PRIO_FUNC_FAILED;
			goto out;
		}
	}
out:
	return rc;
}

int tm_node_elig_set(tm_handle hndl, enum tm_level level, uint32_t index, uint8_t prio)
{
	int rc = 0;
	uint8_t status;
	uint8_t *elig_prio_func_ptr;
	uint8_t type;
	uint8_t current_prio;

	DECLARE_TM_CTL_PTR(ctl, hndl)
	DECLARE_RM_HANDLE(rm, ctl->rm)
	CHECK_TM_CTL_PTR(ctl)

	rc = tm_sched_lock(TM_ENV(ctl));
	if (rc)
		return rc;

	/* check if node exists */
	switch (level) {
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

	DECODE_ELIGIBLE_FUN(*elig_prio_func_ptr, type, current_prio);
	/* test validity */
	if (IS_VALID_N_TYPE_PRIO(type, current_prio)) /* valid previous eligible function */
		*elig_prio_func_ptr = ENCODE_ELIGIBLE_FUN(type, prio);
	else {
		/* in case of disabled nodes or invalid eligible functions (set through direct API)
		   eligible function is set to fixed priority
		*/
		*elig_prio_func_ptr = ENCODE_ELIGIBLE_FUN(FIXED_PRIORITY, prio);
	}

	/* Download to HW */
	switch (level) {
	case A_LEVEL:
		rc = set_hw_a_node_elig_prio_func_ptr(ctl, index);
		break;
	case B_LEVEL:
		rc = set_hw_b_node_elig_prio_func_ptr(ctl, index);
		break;
	case C_LEVEL:
		rc = set_hw_c_node_elig_prio_func_ptr(ctl, index);
		break;
	case P_LEVEL:
		rc = set_hw_port_elig_prio_func_ptr(ctl, index);
		break;
	case Q_LEVEL:
	default:
		rc = -EACCES;
	}
	if (rc)
		rc = TM_HW_ELIG_PRIO_FUNC_FAILED;
out:
	tm_sched_unlock(TM_ENV(ctl));
	return rc;
}
int tm_node_elig_set_propagated(tm_handle hndl, enum tm_level level, uint32_t index)
{
	int rc = 0;
	uint8_t status;
	uint8_t *elig_prio_func_ptr;
	uint8_t type;
	uint8_t current_prio;

	DECLARE_TM_CTL_PTR(ctl, hndl)
	DECLARE_RM_HANDLE(rm, ctl->rm)
	CHECK_TM_CTL_PTR(ctl)

	rc = tm_sched_lock(TM_ENV(ctl));
	if (rc)
		return rc;

	/* check if node exists */
	switch (level) {
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

	DECODE_ELIGIBLE_FUN(*elig_prio_func_ptr, type, current_prio);
	/* test validity */
	if (IS_VALID_N_TYPE_PRIO(type, current_prio)) /* valid previous eligible function */
		*elig_prio_func_ptr = ENCODE_ELIGIBLE_FUN(type, PRIO_P);
	else {
		/* in case of disabled nodes or invalid eligible functions (set through direct API)
		   eligible function is set to fixed priority
		*/
		*elig_prio_func_ptr = ENCODE_ELIGIBLE_FUN(FIXED_PRIORITY, PRIO_P);
	}

	/* Download to HW */
	switch (level) {
	case A_LEVEL:
		rc = set_hw_a_node_elig_prio_func_ptr(ctl, index);
		break;
	case B_LEVEL:
		rc = set_hw_b_node_elig_prio_func_ptr(ctl, index);
		break;
	case C_LEVEL:
		rc = set_hw_c_node_elig_prio_func_ptr(ctl, index);
		break;
	case P_LEVEL:
		rc = set_hw_port_elig_prio_func_ptr(ctl, index);
		break;
	case Q_LEVEL:
	default:
		rc = -EACCES;
	}
	if (rc)
		rc = TM_HW_ELIG_PRIO_FUNC_FAILED;
out:
	tm_sched_unlock(TM_ENV(ctl));
	return rc;
}

int tm_queue_elig_set(tm_handle hndl, uint32_t index, uint8_t prio)
{
	int rc = 0;
	uint8_t status;
	uint8_t type;
	uint8_t current_prio;
	uint8_t *elig_prio_func_ptr;

	DECLARE_TM_CTL_PTR(ctl, hndl)
	DECLARE_RM_HANDLE(rm, ctl->rm)
	CHECK_TM_CTL_PTR(ctl)

	rc = tm_sched_lock(TM_ENV(ctl));
	if (rc)
		return rc;

	/* check if queue exists */
	if (index >= rm->rm_total_queues) {
		rc = -EBADMSG;
		goto out;
	}

	rc = rm_node_status(rm, Q_LEVEL, index, &status);
	if ((rc) || (status != RM_TRUE)) {
		rc = -EPERM;
		goto out;
	}

	elig_prio_func_ptr = &(ctl->tm_queue_array[index].elig_prio_func_ptr);

	DECODE_ELIGIBLE_FUN(*elig_prio_func_ptr, type, current_prio);
	/* test validity */
	if (IS_VALID_Q_TYPE_PRIO(type, current_prio)) /* valid previous eligible function */
		*elig_prio_func_ptr = ENCODE_ELIGIBLE_FUN(type, prio);
	else {
		/* in case of disabled nodes or invalid eligible functions (set through direct API)
		   eligible function is set to fixed priority
		*/
		*elig_prio_func_ptr = ENCODE_ELIGIBLE_FUN(FIXED_PRIORITY, prio);
	}

	/* Download to HW */
	rc = set_hw_queue_elig_prio_func_ptr(ctl, index);
	if (rc)
		rc = TM_HW_ELIG_PRIO_FUNC_FAILED;
out:
	tm_sched_unlock(TM_ENV(ctl));
	return rc;
}


int tm_get_node_elig_prio_fun_info(tm_handle hndl, enum tm_level level, uint32_t index, uint8_t *prio, uint8_t *pmask)
{
	int rc = -EFAULT;
	uint8_t	prio_fun;
	uint8_t	mask;
	uint16_t elig_func_table[32];
	int i;

	DECLARE_TM_CTL_PTR(ctl, hndl)
	CHECK_TM_CTL_PTR(ctl)

	mask = 0;

	switch (level) {
	case Q_LEVEL:
		rc = get_hw_queue_elig_prio_func_ptr(hndl, index, &prio_fun);
		if (rc == 0) {
			rc = get_hw_elig_prio_func(hndl, level, prio_fun , elig_func_table);
			if (rc == 0) {
				for (i = 0; i < 4 ; i++)
					mask = mask | (elig_func_table[i] & 0x3);
			}
		}
		break;
	case A_LEVEL:
		rc = get_hw_a_node_elig_prio_func_ptr(hndl, index, &prio_fun);
		if (rc == 0) {
			rc = get_hw_elig_prio_func(hndl, level, prio_fun , elig_func_table);
			if (rc == 0) {
				for (i = 0; i < 32 ; i++)
					mask = mask | (elig_func_table[i] & 0x3);
			}
		}
		break;
	case B_LEVEL:
		rc = get_hw_b_node_elig_prio_func_ptr(hndl, index, &prio_fun);
		if (rc == 0) {
			rc = get_hw_elig_prio_func(hndl, level, prio_fun , elig_func_table);
			if (rc == 0) {
				for (i = 0; i < 32 ; i++)
					mask = mask | (elig_func_table[i] & 0x3);
			}
		}
		break;
	case C_LEVEL:
		rc = get_hw_c_node_elig_prio_func_ptr(hndl, index, &prio_fun);
		if (rc == 0) {
			rc = get_hw_elig_prio_func(hndl, level, prio_fun , elig_func_table);
			if (rc == 0) {
				for (i = 0; i < 32 ; i++)
					mask = mask | (elig_func_table[i] & 0x3);
			}
		}
		break;
	case P_LEVEL:
		rc = get_hw_port_elig_prio_func_ptr(hndl, index, &prio_fun);
		if (rc == 0) {
			rc = get_hw_elig_prio_func(hndl, level, prio_fun , elig_func_table);
			if (rc == 0) {
				for (i = 0; i < 32 ; i++)
					mask = mask | (elig_func_table[i] & 0x3);
			}
		}
		break;
	}
	if (rc == 0) {
		if (prio)
			*prio = prio_fun;
		if (pmask)
			*pmask = mask;
	}
	return rc;
}



int is_queue_elig_fun_uses_shaper(struct tm_elig_prio_func_queue *queue_func_table, uint8_t func_index)
{
	int mask;
	if (func_index >= TM_ELIG_FUNC_TABLE_SIZE)
		return -1;
	/* if minTB or maxTB bit is used in at least one entry - this function uses shaper */
	mask = 0;
	mask = mask | (queue_func_table[func_index].tbl_entry.func_out[0] & 0x3);
	mask = mask | (queue_func_table[func_index].tbl_entry.func_out[1] & 0x3);
	mask = mask | (queue_func_table[func_index].tbl_entry.func_out[2] & 0x3);
	mask = mask | (queue_func_table[func_index].tbl_entry.func_out[3] & 0x3);
	return mask;
}

int is_node_elig_fun_uses_shaper(struct tm_elig_prio_func_node *node_func_table, uint8_t func_index)
{
	int i;
	int mask;
	if (func_index >= TM_ELIG_FUNC_TABLE_SIZE)
		return -1;
	mask = 0;
	for (i = 0; i < 8 ; i++) {
		/* if minTB or maxTB bit is used in at least one entry - this function uses shaper */
		mask = mask | (node_func_table[func_index].tbl_entry[i].func_out[0] & 0x3);
		mask = mask | (node_func_table[func_index].tbl_entry[i].func_out[1] & 0x3);
		mask = mask | (node_func_table[func_index].tbl_entry[i].func_out[2] & 0x3);
		mask = mask | (node_func_table[func_index].tbl_entry[i].func_out[3] & 0x3);
	}
	return mask;
}


/**
 */
