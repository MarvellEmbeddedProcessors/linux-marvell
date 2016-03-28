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

#include "tm_sysfs_debug.h"
#include "tm/mv_tm.h"

#include "tm_os_interface.h"
#include "tm_nodes_update.h"
#include "tm_nodes_read.h"
#include "tm_nodes_status.h"
#include "tm_sched.h"
#include "set_hw_registers.h"

uint8_t tm_debug_on;

const char *tm_sysfs_level_str(int level)
{
	const char *str;

	switch (level) {
	case Q_LEVEL:
		str = "Queue";
		break;
	case A_LEVEL:
		str = "Anode";
		break;
	case B_LEVEL:
		str = "Bnode";
		break;
	case C_LEVEL:
		str = "Cnode";
		break;
	case P_LEVEL:
		str = "Port";
		break;
	default:
		str = "Unknown";
	}
	return str;
}

int tm_sysfs_enable_debug(uint8_t en)
{
	TM_WRAPPER_BEGIN(qmtm_hndl, ctl, henv);

	if ((en != TM_ENABLE) && (en != TM_DISABLE))
		pr_info("Error: en should be TM_ENABLE/TM_DISABLE\n");
	tm_debug_on = en;

	TM_WRAPPER_END(qmtm_hndl);
}

int tm_sysfs_read_node(int level, uint16_t index)
{
	int i;
	uint32_t cos = TM_INVAL;
	struct tm_queue_params q_params = {0};
	struct tm_a_node_params a_params = {0};
	struct tm_b_node_params b_params = {0};
	struct tm_c_node_params c_params = {0};
	struct tm_port_params params;
	struct tm_port_drop_per_cos cos_params = {0};
	uint32_t av_queue_length;
	uint32_t first_child;
	uint32_t last_child;

	TM_WRAPPER_BEGIN(qmtm_hndl, ctl, henv);

	tm_memset(&params, 0, sizeof(struct tm_port_params));

	switch (level) {
	case Q_LEVEL:
		rc = tm_read_queue_configuration(ctl, index, &q_params);
		if (rc == 0) {
			pr_info("Eligible priority function:	%d\n", q_params.elig_prio_func_ptr);
			pr_info("Priority:		%d\n", tm_elig_to_prio(ctl, Q_LEVEL, q_params.elig_prio_func_ptr));
			pr_info("Drop profile index:		%d\n", q_params.wred_profile_ref);
			pr_info("Quantum value:		%d\n", q_params.quantum);
		}
		break;
	case A_LEVEL:
		rc = tm_read_a_node_configuration(ctl, index, &a_params, &first_child, &last_child);
		if (rc == 0) {
			pr_info("Eligible priority function:	%d\n", a_params.elig_prio_func_ptr);
			i = tm_elig_to_prio(ctl, A_LEVEL, a_params.elig_prio_func_ptr);
			if (i == -1)
				pr_info("Priority:		propagated\n");
			else
				pr_info("Priority:		%d\n", i);
			pr_info("Drop profile index:		%d\n", a_params.wred_profile_ref);
			pr_info("Quantum value:		%d\n", a_params.quantum);
			for (i = 0; i < 8; i++)
				pr_info("DWRR priority %d:		%d\n", i, a_params.dwrr_priority[i]);
			pr_info("First child queue : %d, Last child queue : %d\n", first_child, last_child);
		}
		break;
	case B_LEVEL:
		rc = tm_read_b_node_configuration(ctl, index, &b_params, &first_child, &last_child);
		if (rc == 0) {
			pr_info("Eligible priority function:	%d\n", b_params.elig_prio_func_ptr);
			i = tm_elig_to_prio(ctl, B_LEVEL, b_params.elig_prio_func_ptr);
			if (i == -1)
				pr_info("Priority:		propagated\n");
			else
				pr_info("Priority:		%d\n", i);
			pr_info("Drop profile index:		%d\n", b_params.wred_profile_ref);
			pr_info("Quantum value:		%d\n", b_params.quantum);
			for (i = 0; i < 8; i++)
				pr_info("DWRR priority %d:		%d\n", i, b_params.dwrr_priority[i]);
			pr_info("First child A-node : %d, Last child A-node : %d\n", first_child, last_child);
		}
		break;
	case C_LEVEL:
		rc = tm_read_c_node_configuration(ctl, index, &c_params, &first_child, &last_child);
		if (rc == 0) {
			pr_info("Eligible priority function:	%d\n", c_params.elig_prio_func_ptr);
			i = tm_elig_to_prio(ctl, C_LEVEL, c_params.elig_prio_func_ptr);
			if (i == -1)
				pr_info("Priority:		propagated\n");
			else
				pr_info("Priority:		%d\n", i);
			pr_info("Drop profile index (per Cos):");
			for (cos = 0; cos < 8; cos++)
				if (c_params.wred_cos & (1 << cos))
					pr_info("	Cos %d:		%d\n", cos, c_params.wred_profile_ref[cos]);
			pr_info("Quantum value:		%d\n", c_params.quantum);
			for (i = 0; i < 8; i++)
				pr_info("DWRR priority %d:		%d\n", i, c_params.dwrr_priority[i]);
			pr_info("First child B-node : %d, Last child B-node : %d\n", first_child, last_child);
		}
		break;
	case P_LEVEL:
		rc = tm_read_port_configuration(ctl, index, &params, &cos_params, &first_child, &last_child);
		if (rc == 0) {
			pr_info("Eligible priority function:	%d\n", params.elig_prio_func_ptr);
			i = tm_elig_to_prio(ctl, P_LEVEL, params.elig_prio_func_ptr);
			if (i == -1)
				pr_info("Priority:		propagated\n");
			else
				pr_info("Priority:		%d\n", i);
			pr_info("Drop profile index (Global):	%d\n", params.wred_profile_ref);
			pr_info("Drop profile index (per Cos):");
			for (cos = 0; cos < 8; cos++)
				if (cos_params.wred_cos & (1 << cos))
					pr_info("	Cos %d:	%d\n", cos, cos_params.wred_profile_ref[cos]);
			for (i = 0; i < 8; i++)
				pr_info("DWRR priority %d:		%d\n", i, params.dwrr_priority[i]);
			pr_info("First child C-node : %d, Last child C-node : %d\n", first_child, last_child);
		}
		break;
	}
	if (rc) {
		pr_info("tm_read_node error: %d\n", rc);
		goto out;
	}

	rc = tm_drop_get_queue_length(ctl, level, index, &av_queue_length);
	if (rc != 0)
		pr_info("tm_drop_get_queue_length error: %d\n", rc);
	else
		pr_info("Queue Length:	%d\n", (int)av_queue_length);

out:
	TM_WRAPPER_END(qmtm_hndl);
}

int tm_sysfs_read_node_hw(int level, uint16_t index)
{
	int i;
	uint32_t cos = TM_INVAL;
	struct tm_queue_params q_params = {0};
	struct tm_a_node_params a_params = {0};
	struct tm_b_node_params b_params = {0};
	struct tm_c_node_params c_params = {0};
	struct tm_port_params params;
	struct tm_port_drop_per_cos cos_params = {0};
	uint32_t av_queue_length;
	uint32_t first_child;
	uint32_t last_child;

	TM_WRAPPER_BEGIN(qmtm_hndl, ctl, henv);

	tm_memset(&params, 0, sizeof(struct tm_port_params));

	switch (level) {
	case Q_LEVEL:
		rc = tm_read_queue_configuration_hw(ctl, index, &q_params);
		if (rc == 0) {
			pr_info("Eligible priority function:	%d\n", q_params.elig_prio_func_ptr);
			pr_info("Priority:		%d\n", tm_elig_to_prio(ctl, Q_LEVEL, q_params.elig_prio_func_ptr));
			pr_info("Drop profile index:		%d\n", q_params.wred_profile_ref);
			pr_info("Quantum value:		%d\n", q_params.quantum);
		}
		break;
	case A_LEVEL:
		rc = tm_read_a_node_configuration_hw(ctl, index, &a_params, &first_child, &last_child);
		if (rc == 0) {
			pr_info("Eligible priority function:	%d\n", a_params.elig_prio_func_ptr);
			i = tm_elig_to_prio(ctl, A_LEVEL, a_params.elig_prio_func_ptr);
			if (i == -1)
				pr_info("Priority:		propagated\n");
			else
				pr_info("Priority:		%d\n", i);
			pr_info("Drop profile index:		%d\n", a_params.wred_profile_ref);
			pr_info("Quantum value:		%d\n", a_params.quantum);
			for (i = 0; i < 8; i++)
				pr_info("DWRR priority %d:		%d\n", i, a_params.dwrr_priority[i]);
			pr_info("First child queue : %d, Last child queue : %d\n", first_child, last_child);
		}
		break;
	case B_LEVEL:
		rc = tm_read_b_node_configuration_hw(ctl, index, &b_params, &first_child, &last_child);
		if (rc == 0) {
			pr_info("Eligible priority function:	%d\n", b_params.elig_prio_func_ptr);
			i = tm_elig_to_prio(ctl, B_LEVEL, b_params.elig_prio_func_ptr);
			if (i == -1)
				pr_info("Priority:		propagated\n");
			else
				pr_info("Priority:		%d\n", i);
			pr_info("Drop profile index:		%d\n", b_params.wred_profile_ref);
			pr_info("Quantum value:		%d\n", b_params.quantum);
			for (i = 0; i < 8; i++)
				pr_info("DWRR priority %d:		%d\n", i, b_params.dwrr_priority[i]);
			pr_info("First child A-node : %d, Last child A-node : %d\n", first_child, last_child);
		}
		break;
	case C_LEVEL:
		rc = tm_read_c_node_configuration_hw(ctl, index, &c_params, &first_child, &last_child);
		if (rc == 0) {
			pr_info("Eligible priority function:	%d\n", c_params.elig_prio_func_ptr);
			i = tm_elig_to_prio(ctl, C_LEVEL, c_params.elig_prio_func_ptr);
			if (i == -1)
				pr_info("Priority:		propagated\n");
			else
				pr_info("Priority:		%d\n", i);
			pr_info("Drop profile index (per Cos) - all Cos printed:\n");
			for (cos = 0; cos < 8; cos++)
				pr_info("	Cos %d:		%d\n", cos, c_params.wred_profile_ref[cos]);
			pr_info("Quantum value:		%d\n", c_params.quantum);
			for (i = 0; i < 8; i++)
				pr_info("DWRR priority %d:		%d\n", i, c_params.dwrr_priority[i]);
			pr_info("First child B-node : %d, Last child B-node : %d\n", first_child, last_child);
		}
		break;
	case P_LEVEL:
		rc = tm_read_port_configuration_hw(ctl, index, &params, &cos_params, &first_child, &last_child);
		if (rc == 0) {
			pr_info("Eligible priority function:	%d\n", params.elig_prio_func_ptr);
			i = tm_elig_to_prio(ctl, P_LEVEL, params.elig_prio_func_ptr);
			if (i == -1)
				pr_info("Priority:		propagated\n");
			else
				pr_info("Priority:		%d\n", i);
			pr_info("Drop profile index (Global):	%d\n", params.wred_profile_ref);
			pr_info("Drop profile index (per Cos):");
			for (cos = 0; cos < 8; cos++)
				if (cos_params.wred_cos & (1 << cos))
					pr_info("	Cos %d:	%d\n", cos, cos_params.wred_profile_ref[cos]);
			for (i = 0; i < 8; i++)
				pr_info("DWRR priority %d:		%d\n", i, params.dwrr_priority[i]);
			pr_info("First child C-node : %d, Last child C-node : %d\n", first_child, last_child);
		}
		break;
	}
	if (rc) {
		pr_info("tm_read_node_hw error: %d\n", rc);
		goto out;
	}

	rc = tm_drop_get_queue_length(ctl, level, index, &av_queue_length);
	if (rc != 0)
		pr_info("tm_drop_get_queue_length error: %d\n", rc);
	else
		pr_info("Queue Length:	%d\n", (int)av_queue_length);

out:
	TM_WRAPPER_END(qmtm_hndl);
}

int tm_sysfs_print_ports_name(void)
{
	int i;

	for (i = 0; i < MV_TM_MAX_PORTS; i++)
		switch (i) {
		case TM_A0_PORT_PPC0_0:
			pr_info("port%02d: PPC0_0\n", i);
			break;
		case TM_A0_PORT_PPC0_1:
			pr_info("port%02d: PPC0_1\n", i);
			break;
		case TM_A0_PORT_PPC1_MNT0:
			pr_info("port%02d: PPC1_MNT0\n", i);
			break;
		case TM_A0_PORT_PPC1_MNT1:
			pr_info("port%02d: PPC1_MNT1\n", i);
			break;
		case TM_A0_PORT_EMAC0:
			pr_info("port%02d: EMAC0\n", i);
			break;
		case TM_A0_PORT_EMAC1:
			pr_info("port%02d: EMAC1\n", i);
			break;
		case TM_A0_PORT_EMAC2:
			pr_info("port%02d: EMAC2\n", i);
			break;
		case TM_A0_PORT_EMAC3:
			pr_info("port%02d: EMAC3\n", i);
			break;
		case TM_A0_PORT_EMAC4:
			pr_info("port%02d: EMAC4\n", i);
			break;
		case TM_A0_PORT_EMAC_LPB:
			pr_info("port%02d: EMAC_LPB\n", i);
			break;
		case TM_A0_PORT_CMAC_IN:
			pr_info("port%02d: CMAC_IN\n", i);
			break;
		case TM_A0_PORT_CMAC_LA:
			pr_info("port%02d: CMAC_LA\n", i);
			break;
		case TM_A0_PORT_HMAC:
			pr_info("port%02d: HMAC\n", i);
			break;
		case TM_A0_PORT_UNUSED0:
			pr_info("port%02d: UNUSED0\n", i);
			break;
		case TM_A0_PORT_DROP0:
			pr_info("port%02d: DROP0\n", i);
			break;
		case TM_A0_PORT_DROP1:
			pr_info("port%02d: DROP1\n", i);
			break;
		default:
			pr_info("Error: Undefined port!\n");
		}

	return 0;
}

int tm_sysfs_dump_port_hw(uint32_t port_index)
{
	TM_WRAPPER_BEGIN(qmtm_hndl, ctl, henv);

	rc = tm_dump_port_hw(ctl, port_index);

	TM_WRAPPER_END(qmtm_hndl);
}

int tm_sysfs_trace_queues(uint32_t timeout, uint8_t full_path)
{
	TM_WRAPPER_BEGIN(qmtm_hndl, ctl, henv);

	if (timeout < 1000) /* else its ms*/
		timeout *= 1000;

	rc = check_hw_drop_path(ctl, timeout, full_path);
	if (rc != 0)
		pr_info("check_hw_drop_path error: %d\n", rc);

	TM_WRAPPER_END(qmtm_hndl);
}

int tm_sysfs_set_elig(int level,
					uint16_t index,
					uint32_t eligible)
{
	int i;
	struct tm_queue_params q_params;
	struct tm_a_node_params a_params;
	struct tm_b_node_params b_params;
	struct tm_c_node_params c_params;
	uint8_t dwrr_priority[8] = {0, 0, 0, 0, 0, 0, 0, 0};

	TM_WRAPPER_BEGIN(qmtm_hndl, ctl, henv);

	switch (level) {
	case Q_LEVEL:
		q_params.wred_profile_ref = (uint8_t) TM_INVAL;
		q_params.elig_prio_func_ptr = (uint8_t) eligible;
		q_params.quantum = (uint16_t) TM_INVAL;
		rc = tm_update_queue(ctl, index, &q_params);
		break;
	case A_LEVEL:
		a_params.wred_profile_ref = (uint8_t) TM_INVAL;
		a_params.elig_prio_func_ptr = (uint8_t) eligible;
		a_params.quantum = (uint16_t) TM_INVAL;
		for (i = 0; i < 8; i++)
			a_params.dwrr_priority[i] = (uint8_t) TM_INVAL;
		rc = tm_update_a_node(ctl, index, &a_params);
		break;
	case B_LEVEL:
		b_params.wred_profile_ref = (uint8_t) TM_INVAL;
		b_params.elig_prio_func_ptr = (uint8_t) eligible;
		b_params.quantum = (uint16_t) TM_INVAL;
		for (i = 0; i < 8; i++)
			b_params.dwrr_priority[i] = (uint8_t) TM_INVAL;
		rc = tm_update_b_node(ctl, index, &b_params);
		break;
	case C_LEVEL:
		c_params.wred_cos = 0xff;
		for (i = 0; i < TM_WRED_COS; i++)
			c_params.wred_profile_ref[i] = (uint8_t) TM_INVAL;
		c_params.elig_prio_func_ptr = (uint8_t) eligible;
		c_params.quantum = (uint16_t) TM_INVAL;
		for (i = 0; i < 8; i++)
			c_params.dwrr_priority[i] = (uint8_t) TM_INVAL;
		rc = tm_update_c_node(ctl, index, &c_params);
		break;
	case P_LEVEL:
		for (i = 0; i < 8; i++)
			dwrr_priority[i] = (uint8_t) TM_INVAL;
		rc = tm_update_port_scheduling(ctl, index, (uint8_t) eligible, dwrr_priority);
		break;
	default:
		rc = -3;
		break;
	}
	if (rc != 0)
		pr_info("tm_set_elig error: %d\n", rc);

	TM_WRAPPER_END(qmtm_hndl);
}

int tm_sysfs_set_elig_per_queue_range(uint32_t startInd, uint32_t endInd, uint8_t elig)
{
	TM_WRAPPER_BEGIN(qmtm_hndl, ctl, henv);

	rc = set_hw_elig_per_queue_range(ctl, startInd, endInd, elig);
	if (rc != 0)
		pr_info("set_hw_elig_per_queue_range error: %d\n", rc);

	TM_WRAPPER_END(qmtm_hndl);
}

int tm_sysfs_show_elig_func(int level, uint32_t func_index)
{
	TM_WRAPPER_BEGIN(qmtm_hndl, ctl, henv);

	rc = show_hw_elig_prio_func(ctl, level, func_index);
	if (rc != 0)
		pr_info("show_hw_elig_prio_func error: %d\n", rc);

	TM_WRAPPER_END(qmtm_hndl);
}

