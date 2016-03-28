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

#include "mv_tm_sched.h"
#include "tm_nodes_tree.h"
#include "tm_nodes_update.h"
#include "tm_elig_prio_func.h"


int mv_tm_quantum_range_get(uint32_t mtu, uint32_t *min_quantum, uint32_t *max_quantum)
{
	uint16_t quantum;

	TM_WRAPPER_BEGIN(qmtm_hndl, ctl, henv);

	quantum = (mtu + ctl->min_pkg_size)/TM_NODE_QUANTUM_UNIT;
	*min_quantum = quantum;
	*max_quantum = 256 * quantum;

	TM_WRAPPER_END(qmtm_hndl);
}
EXPORT_SYMBOL(mv_tm_quantum_range_get);

int mv_tm_mtu_set(uint32_t mtu)
{
	TM_WRAPPER_BEGIN(qmtm_hndl, ctl, henv);

	rc = tm_mtu_set(ctl, mtu);
	if (rc != 0)
		pr_info("mv_tm_mtu_set error: %d\n", rc);

	TM_WRAPPER_END(qmtm_hndl);
}
EXPORT_SYMBOL(mv_tm_mtu_set);

int mv_tm_tree_status_set(int status)
{
	TM_WRAPPER_BEGIN(qmtm_hndl, ctl, henv);

	rc = tm_tree_change_status(ctl, (uint8_t)status);
	if (rc != 0)
		pr_info("tm_tree_change_status error: %d\n", rc);

	TM_WRAPPER_END(qmtm_hndl);
}
EXPORT_SYMBOL(mv_tm_tree_status_set);

int mv_tm_prio_set(enum mv_tm_level level,
					uint32_t index,
					uint8_t prio)
{
	TM_WRAPPER_BEGIN(qmtm_hndl, ctl, henv);

	if (prio >= MV_TM_NUM_OF_PRIO) {
		rc = -1;
		goto out;
	}

	if (level == TM_Q_LEVEL)
		rc = tm_queue_elig_set(ctl, index, prio);
	else
		rc = tm_node_elig_set(ctl, TM_LEVEL(level), index, prio);
out:
	if (rc)
		pr_info("tm_node_prio_set error: %d\n", rc);
	TM_WRAPPER_END(qmtm_hndl);
}
EXPORT_SYMBOL(mv_tm_prio_set);

int mv_tm_prio_set_propagated(enum mv_tm_level level,
					uint32_t index)
{
	TM_WRAPPER_BEGIN(qmtm_hndl, ctl, henv);

	if (level == TM_Q_LEVEL)
		pr_info("not applicable for queues!\n");
	else
		rc = tm_node_elig_set_propagated(ctl, TM_LEVEL(level), index);

	if (rc)
		pr_info("tm_node_prio_set_propagated error: %d\n", rc);
	TM_WRAPPER_END(qmtm_hndl);
}
EXPORT_SYMBOL(mv_tm_prio_set_propagated);

int mv_tm_dwrr_weight(enum mv_tm_level level,
					uint32_t index,
					uint32_t quantum)
{
	int i;
	struct tm_queue_params q_params;
	struct tm_a_node_params a_params;
	struct tm_b_node_params b_params;
	struct tm_c_node_params c_params;

	TM_WRAPPER_BEGIN(qmtm_hndl, ctl, henv);

	switch (level) {
	case TM_Q_LEVEL:
		q_params.wred_profile_ref = (uint8_t) TM_INVAL;
		q_params.elig_prio_func_ptr = (uint8_t) TM_INVAL;
		q_params.quantum = quantum;
		rc = tm_update_queue(ctl, index, &q_params);
		break;
	case TM_A_LEVEL:
		a_params.wred_profile_ref = (uint8_t) TM_INVAL;
		a_params.elig_prio_func_ptr = (uint8_t) TM_INVAL;
		a_params.quantum = quantum;
		for (i = 0; i < 8; i++)
			a_params.dwrr_priority[i] = (uint8_t) TM_INVAL;
		rc = tm_update_a_node(ctl, index, &a_params);
		break;
	case TM_B_LEVEL:
		b_params.wred_profile_ref = (uint8_t) TM_INVAL;
		b_params.elig_prio_func_ptr = (uint8_t) TM_INVAL;
		b_params.quantum = quantum;
		for (i = 0; i < 8; i++)
			b_params.dwrr_priority[i] = (uint8_t) TM_INVAL;
		rc = tm_update_b_node(ctl, index, &b_params);
		break;
	case TM_C_LEVEL:
		c_params.wred_cos = 0xff;
		for (i = 0; i < TM_WRED_COS; i++)
			c_params.wred_profile_ref[i] = (uint8_t) TM_INVAL;
		c_params.elig_prio_func_ptr = (uint8_t) TM_INVAL;
		c_params.quantum = quantum;
		for (i = 0; i < 8; i++)
			c_params.dwrr_priority[i] = (uint8_t) TM_INVAL;
		rc = tm_update_c_node(ctl, index, &c_params);
		break;
	case TM_P_LEVEL:
		pr_info("DWRR Quantum is not supported on Port level!\n");
		rc = -1;
	default:
		rc = -3;
		break;
	}
	if (rc != 0)
		pr_info("tm_dwrr_weight error: %d\n", rc);

	TM_WRAPPER_END(qmtm_hndl);
}
EXPORT_SYMBOL(mv_tm_dwrr_weight);


int mv_tm_dwrr_enable(enum mv_tm_level level,
					uint32_t index,
					uint8_t prio,
					int en)
{
	int i;
	struct tm_a_node_params a_params;
	struct tm_b_node_params b_params;
	struct tm_c_node_params c_params;
	uint8_t dwrr_prio[8] = {0};

	TM_WRAPPER_BEGIN(qmtm_hndl, ctl, henv);

	if ((en != TM_ENABLE) && (en != TM_DISABLE)) {
		rc = -1;
		goto out;
	}

	if (prio >= MV_TM_NUM_OF_PRIO) {
		rc = -2;
		goto out;
	}

	switch (level) {
	case TM_Q_LEVEL:
		pr_info("DWRR priority for Queue level should be set on A-node.\n");
		rc = -1;
		break;
	case TM_A_LEVEL:
		a_params.wred_profile_ref = (uint8_t) TM_INVAL;
		a_params.elig_prio_func_ptr = (uint8_t) TM_INVAL;
		a_params.quantum = (uint16_t) TM_INVAL;
		for (i = 0; i < 8; i++)
			a_params.dwrr_priority[i] = (uint8_t) TM_INVAL;
		a_params.dwrr_priority[prio] = (uint8_t) en;
		rc = tm_update_a_node(ctl, index, &a_params);
		break;
	case TM_B_LEVEL:
		b_params.wred_profile_ref = (uint8_t) TM_INVAL;
		b_params.elig_prio_func_ptr = (uint8_t) TM_INVAL;
		b_params.quantum = (uint16_t) TM_INVAL;
		for (i = 0; i < 8; i++)
			b_params.dwrr_priority[i] = (uint8_t) TM_INVAL;
		b_params.dwrr_priority[prio] = (uint8_t) en;
		rc = tm_update_b_node(ctl, index, &b_params);
		break;
	case TM_C_LEVEL:
		c_params.wred_cos = 0xff;
		for (i = 0; i < TM_WRED_COS; i++)
			c_params.wred_profile_ref[i] = (uint8_t) TM_INVAL;
		c_params.elig_prio_func_ptr = (uint8_t) TM_INVAL;
		c_params.quantum = (uint16_t) TM_INVAL;
		for (i = 0; i < 8; i++)
			c_params.dwrr_priority[i] = (uint8_t) TM_INVAL;
		c_params.dwrr_priority[prio] = (uint8_t) en;
		rc = tm_update_c_node(ctl, index, &c_params);
		break;
	case TM_P_LEVEL:
		for (i = 0; i < 8; i++)
			dwrr_prio[i] = (uint8_t)TM_INVAL;
		dwrr_prio[prio] = (uint8_t) en;
		rc = tm_update_port_scheduling(ctl, index, (uint8_t)TM_INVAL, dwrr_prio);
		break;
	default:
		rc = -3;
		break;
	}
out:
	if (rc)
		pr_info("tm_enable_dwrr error: %d\n", rc);
	TM_WRAPPER_END(qmtm_hndl);
}
EXPORT_SYMBOL(mv_tm_dwrr_enable);
