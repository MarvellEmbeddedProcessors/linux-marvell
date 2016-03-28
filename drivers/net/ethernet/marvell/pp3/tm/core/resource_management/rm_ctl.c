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

#include "tm/core/tm_core_types.h"
#include "rm_internal_types.h"
#include "rm_ctl.h"
#include "rm_chunk.h"
#include "tm/core/tm_os_interface.h"
#include "tm/core/tm_hw_configuration_interface.h"


/**
 */
int rm_open(uint8_t total_ports,
			uint16_t total_c_nodes,
			uint16_t total_b_nodes,
			uint16_t total_a_nodes,
			uint32_t total_queues,
			rmctl_t *hndl)
{
	struct rmctl *ctl = NULL;
	int i;
	int j;

	int rc = 0;

	/* check that it is new handle to create*/
	if (!hndl) {
		rc = -EINVAL;
		goto out;
	}
	*hndl = NULL;

	/* Create rmctl instance */
	ctl = tm_malloc(sizeof(*ctl));
	if (!ctl) {
		rc = -ENOMEM;
		goto out;
	}
	tm_memset(ctl, 0, sizeof(*ctl));

	/* Fill in ctl structure */
	ctl->magic = RM_MAGIC;


	/* Allocate arrays */
	ctl->rm_queue_array = tm_malloc(total_queues * sizeof(struct rm_node));
	if (!ctl->rm_queue_array) {
		rc = -ENOMEM;
		goto out;
	}
	tm_memset(ctl->rm_queue_array, 0, total_queues * sizeof(struct rm_node));

	ctl->rm_a_node_array = tm_malloc(total_a_nodes * sizeof(struct rm_node));
	if (!ctl->rm_a_node_array) {
		rc = -ENOMEM;
		goto out;
	}
	tm_memset(ctl->rm_a_node_array, 0, total_a_nodes * sizeof(struct rm_node));

	ctl->rm_b_node_array = tm_malloc(total_b_nodes * sizeof(struct rm_node));
	if (!ctl->rm_b_node_array) {
		rc = -ENOMEM;
		goto out;
	}
	tm_memset(ctl->rm_b_node_array, 0, total_b_nodes * sizeof(struct rm_node));

	ctl->rm_c_node_array = tm_malloc(total_c_nodes * sizeof(struct rm_node));
	if (!ctl->rm_c_node_array) {
		rc = -ENOMEM;
		goto out;
	}
	tm_memset(ctl->rm_c_node_array, 0, total_c_nodes * sizeof(struct rm_node));

	ctl->rm_port_array = tm_malloc(total_ports * sizeof(struct rm_node));
	if (!ctl->rm_port_array) {
		rc = -ENOMEM;
		goto out;
	}
	tm_memset(ctl->rm_port_array, 0, total_ports * sizeof(struct rm_node));

	ctl->rm_wred_queue_curves =
		tm_malloc(TM_NUM_WRED_QUEUE_CURVES * sizeof(struct rm_entry));
	if (!ctl->rm_wred_queue_curves) {
		rc = -ENOMEM;
		goto out;
	}

	ctl->rm_wred_a_node_curves =
		tm_malloc(TM_NUM_WRED_A_NODE_CURVES * sizeof(struct rm_entry));
	if (!ctl->rm_wred_a_node_curves) {
		rc = -ENOMEM;
		goto out;
	}

	ctl->rm_wred_b_node_curves =
		tm_malloc(TM_NUM_WRED_B_NODE_CURVES * sizeof(struct rm_entry));
	if (!ctl->rm_wred_b_node_curves) {
		rc = -ENOMEM;
		goto out;
	}


	for (i = 0; i < RM_COS; i++) {
		ctl->rm_wred_c_node_curves[i] =
			tm_malloc(TM_NUM_WRED_C_NODE_CURVES * sizeof(struct rm_entry));
		if (!ctl->rm_wred_c_node_curves[i]) {
			rc = -ENOMEM;
			goto out;
		}
	}


	ctl->rm_wred_port_curves =
		tm_malloc(TM_NUM_WRED_PORT_CURVES * sizeof(struct rm_entry));
	if (!ctl->rm_wred_port_curves) {
		rc = -ENOMEM;
		goto out;
	}

	for (i = 0; i < RM_COS; i++) {
		ctl->rm_wred_port_curves_cos[i] =
			tm_malloc(TM_NUM_WRED_PORT_CURVES * sizeof(struct rm_entry));
		if (!ctl->rm_wred_port_curves_cos[i]) {
			rc = -ENOMEM;
			goto out;
		}
	}

	ctl->rm_queue_drop_profiles =
		tm_malloc(TM_NUM_QUEUE_DROP_PROF * sizeof(struct rm_entry));
	if (!ctl->rm_queue_drop_profiles) {
		rc = -ENOMEM;
		goto out;
	}

	ctl->rm_a_node_drop_profiles =
		tm_malloc(TM_NUM_A_NODE_DROP_PROF * sizeof(struct rm_entry));
	if (!ctl->rm_a_node_drop_profiles) {
		rc = -ENOMEM;
		goto out;
	}

	ctl->rm_b_node_drop_profiles =
		tm_malloc(TM_NUM_B_NODE_DROP_PROF * sizeof(struct rm_entry));
	if (!ctl->rm_b_node_drop_profiles) {
		rc = -ENOMEM;
		goto out;
	}


	/* not used in HX/AX  */
	for (i = 0; i < RM_COS; i++) {
		ctl->rm_c_node_drop_profiles[i] =
			tm_malloc(TM_NUM_C_NODE_DROP_PROF * sizeof(struct rm_entry));
		if (!ctl->rm_c_node_drop_profiles[i]) {
			rc = -ENOMEM;
			goto out;
		}
	}

	ctl->rm_port_drop_profiles = tm_malloc(TM_NUM_PORT_DROP_PROF * sizeof(struct rm_entry));
	if (!ctl->rm_port_drop_profiles) {
		rc = -ENOMEM;
		goto out;
	}

	/* not used in HX/AX*/
	for (i = 0; i < RM_COS; i++) {
		ctl->rm_port_drop_profiles_cos[i] = tm_malloc(TM_NUM_PORT_DROP_PROF * sizeof(struct rm_entry));
		if (!ctl->rm_port_drop_profiles_cos[i]) {
			rc = -ENOMEM;
			goto out;
		}
	}

	for (i = 0; i < TM_NUM_WRED_QUEUE_CURVES; i++) {
		ctl->rm_wred_queue_curves[i].used = RM_FALSE;
		ctl->rm_wred_queue_curves[i].next_free_ind = i+1;
	}
	ctl->rm_wred_queue_curves[TM_NUM_WRED_QUEUE_CURVES-1].next_free_ind =
		(uint16_t)TM_INVAL;

	for (i = 0; i < TM_NUM_WRED_A_NODE_CURVES; i++) {
		ctl->rm_wred_a_node_curves[i].used = RM_FALSE;
		ctl->rm_wred_a_node_curves[i].next_free_ind = i+1;
	}
	ctl->rm_wred_a_node_curves[TM_NUM_WRED_A_NODE_CURVES-1].next_free_ind =
		(uint16_t)TM_INVAL;

	for (i = 0; i < TM_NUM_WRED_B_NODE_CURVES; i++) {
		ctl->rm_wred_b_node_curves[i].used = RM_FALSE;
		ctl->rm_wred_b_node_curves[i].next_free_ind = i+1;
	}
	ctl->rm_wred_b_node_curves[TM_NUM_WRED_B_NODE_CURVES-1].next_free_ind =
		(uint16_t)TM_INVAL;

	for (j = 0; j < TM_WRED_COS; j++) {
		for (i = 0; i < TM_NUM_WRED_C_NODE_CURVES; i++) {
			ctl->rm_wred_c_node_curves[j][i].used = RM_FALSE;
			ctl->rm_wred_c_node_curves[j][i].next_free_ind = i+1;
		}
		ctl->rm_wred_c_node_curves[j][TM_NUM_WRED_C_NODE_CURVES-1].next_free_ind =
			(uint16_t)TM_INVAL;
	}

	for (i = 0; i < TM_NUM_WRED_PORT_CURVES; i++) {
		ctl->rm_wred_port_curves[i].used = RM_FALSE;
		ctl->rm_wred_port_curves[i].next_free_ind = i+1;
	}
	ctl->rm_wred_port_curves[TM_NUM_WRED_PORT_CURVES-1].next_free_ind =
		(uint16_t)TM_INVAL;

	/* nor used in HX/AX*/
	for (j = 0; j < TM_WRED_COS; j++) {
		for (i = 0; i < TM_NUM_WRED_PORT_CURVES; i++) {
			ctl->rm_wred_port_curves_cos[j][i].used = RM_FALSE;
			ctl->rm_wred_port_curves_cos[j][i].next_free_ind = i+1;
		}
		ctl->rm_wred_port_curves_cos[j][TM_NUM_WRED_PORT_CURVES-1].next_free_ind =
			(uint16_t)TM_INVAL;
	}


	for (i = 0; i < TM_NUM_QUEUE_DROP_PROF; i++) {
		ctl->rm_queue_drop_profiles[i].used = RM_FALSE;
		ctl->rm_queue_drop_profiles[i].next_free_ind = i+1;
	}
	ctl->rm_queue_drop_profiles[TM_NUM_QUEUE_DROP_PROF-1].next_free_ind =
		(uint16_t)TM_INVAL;

	for (i = 0; i < TM_NUM_A_NODE_DROP_PROF; i++) {
		ctl->rm_a_node_drop_profiles[i].used = RM_FALSE;
		ctl->rm_a_node_drop_profiles[i].next_free_ind = i+1;
	}
	ctl->rm_a_node_drop_profiles[TM_NUM_A_NODE_DROP_PROF-1].next_free_ind =
		(uint16_t)TM_INVAL;

	for (i = 0; i < TM_NUM_B_NODE_DROP_PROF; i++) {
		ctl->rm_b_node_drop_profiles[i].used = RM_FALSE;
		ctl->rm_b_node_drop_profiles[i].next_free_ind = i+1;
	}
	ctl->rm_b_node_drop_profiles[TM_NUM_B_NODE_DROP_PROF-1].next_free_ind =
		(uint16_t)TM_INVAL;

	for (j = 0; j < TM_WRED_COS; j++) {
		for (i = 0; i < TM_NUM_C_NODE_DROP_PROF; i++) {
			ctl->rm_c_node_drop_profiles[j][i].used = RM_FALSE;
			ctl->rm_c_node_drop_profiles[j][i].next_free_ind = i+1;
		}
		ctl->rm_c_node_drop_profiles[j][TM_NUM_C_NODE_DROP_PROF-1].next_free_ind =
			(uint16_t)TM_INVAL;
	}

	for (i = 0; i < TM_NUM_PORT_DROP_PROF; i++) {
		ctl->rm_port_drop_profiles[i].used = RM_FALSE;
		ctl->rm_port_drop_profiles[i].next_free_ind = i+1;
	}
	ctl->rm_port_drop_profiles[TM_NUM_PORT_DROP_PROF-1].next_free_ind =
		(uint16_t)TM_INVAL;

	/* not for Hx/AX*/
	for (j = 0; j < TM_WRED_COS; j++) {
		for (i = 0; i < TM_NUM_PORT_DROP_PROF; i++) {
			ctl->rm_port_drop_profiles_cos[j][i].used = RM_FALSE;
			ctl->rm_port_drop_profiles_cos[j][i].next_free_ind = i+1;
		}
		ctl->rm_port_drop_profiles_cos[j][TM_NUM_PORT_DROP_PROF-1].next_free_ind =
			(uint16_t)TM_INVAL;
	}

	/* Initiate counters */
	ctl->rm_queue_cnt = total_queues;
	ctl->rm_a_node_cnt = total_a_nodes;
	ctl->rm_b_node_cnt = total_b_nodes;
	ctl->rm_c_node_cnt = total_c_nodes;
	ctl->rm_port_cnt = total_ports;

	/* Copy total numbers of nodes per level */
	ctl->rm_total_queues = total_queues;
	ctl->rm_total_a_nodes = total_a_nodes;
	ctl->rm_total_b_nodes = total_b_nodes;
	ctl->rm_total_c_nodes = total_c_nodes;
	ctl->rm_total_ports = total_ports;


	/* Initiate free entries arrays */
	for (i = 0; i < RM_MAX_PROFILES; i++)
		ctl->rm_first_free_entry[i] = 0;

	ctl->rm_last_free_entry[RM_WRED_Q_CURVE] = TM_NUM_WRED_QUEUE_CURVES;
	ctl->rm_last_free_entry[RM_WRED_A_CURVE] = TM_NUM_WRED_A_NODE_CURVES;
	ctl->rm_last_free_entry[RM_WRED_B_CURVE] = TM_NUM_WRED_B_NODE_CURVES;
	ctl->rm_last_free_entry[RM_WRED_C_CURVE_COS_0] = TM_NUM_WRED_C_NODE_CURVES;
	ctl->rm_last_free_entry[RM_WRED_C_CURVE_COS_1] = TM_NUM_WRED_C_NODE_CURVES;
	ctl->rm_last_free_entry[RM_WRED_C_CURVE_COS_2] = TM_NUM_WRED_C_NODE_CURVES;
	ctl->rm_last_free_entry[RM_WRED_C_CURVE_COS_3] = TM_NUM_WRED_C_NODE_CURVES;
	ctl->rm_last_free_entry[RM_WRED_C_CURVE_COS_4] = TM_NUM_WRED_C_NODE_CURVES;
	ctl->rm_last_free_entry[RM_WRED_C_CURVE_COS_5] = TM_NUM_WRED_C_NODE_CURVES;
	ctl->rm_last_free_entry[RM_WRED_C_CURVE_COS_6] = TM_NUM_WRED_C_NODE_CURVES;
	ctl->rm_last_free_entry[RM_WRED_C_CURVE_COS_7] = TM_NUM_WRED_C_NODE_CURVES;
	ctl->rm_last_free_entry[RM_WRED_P_CURVE] = TM_NUM_WRED_PORT_CURVES;

	ctl->rm_last_free_entry[RM_WRED_P_CURVE_COS_0] = TM_NUM_WRED_PORT_CURVES;
	ctl->rm_last_free_entry[RM_WRED_P_CURVE_COS_1] = TM_NUM_WRED_PORT_CURVES;
	ctl->rm_last_free_entry[RM_WRED_P_CURVE_COS_2] = TM_NUM_WRED_PORT_CURVES;
	ctl->rm_last_free_entry[RM_WRED_P_CURVE_COS_3] = TM_NUM_WRED_PORT_CURVES;
	ctl->rm_last_free_entry[RM_WRED_P_CURVE_COS_4] = TM_NUM_WRED_PORT_CURVES;
	ctl->rm_last_free_entry[RM_WRED_P_CURVE_COS_5] = TM_NUM_WRED_PORT_CURVES;
	ctl->rm_last_free_entry[RM_WRED_P_CURVE_COS_6] = TM_NUM_WRED_PORT_CURVES;
	ctl->rm_last_free_entry[RM_WRED_P_CURVE_COS_7] = TM_NUM_WRED_PORT_CURVES;

	ctl->rm_last_free_entry[RM_Q_DROP_PRF] = TM_NUM_QUEUE_DROP_PROF;
	ctl->rm_last_free_entry[RM_A_DROP_PRF] = TM_NUM_A_NODE_DROP_PROF;
	ctl->rm_last_free_entry[RM_B_DROP_PRF] = TM_NUM_B_NODE_DROP_PROF;
	ctl->rm_last_free_entry[RM_C_DROP_PRF_COS_0] = TM_NUM_C_NODE_DROP_PROF;
	ctl->rm_last_free_entry[RM_C_DROP_PRF_COS_1] = TM_NUM_C_NODE_DROP_PROF;
	ctl->rm_last_free_entry[RM_C_DROP_PRF_COS_2] = TM_NUM_C_NODE_DROP_PROF;
	ctl->rm_last_free_entry[RM_C_DROP_PRF_COS_3] = TM_NUM_C_NODE_DROP_PROF;
	ctl->rm_last_free_entry[RM_C_DROP_PRF_COS_4] = TM_NUM_C_NODE_DROP_PROF;
	ctl->rm_last_free_entry[RM_C_DROP_PRF_COS_5] = TM_NUM_C_NODE_DROP_PROF;
	ctl->rm_last_free_entry[RM_C_DROP_PRF_COS_6] = TM_NUM_C_NODE_DROP_PROF;
	ctl->rm_last_free_entry[RM_C_DROP_PRF_COS_7] = TM_NUM_C_NODE_DROP_PROF;
	ctl->rm_last_free_entry[RM_P_DROP_PRF] = TM_NUM_PORT_DROP_PROF;

	ctl->rm_last_free_entry[RM_P_DROP_PRF_COS_0] = TM_NUM_PORT_DROP_PROF;
	ctl->rm_last_free_entry[RM_P_DROP_PRF_COS_1] = TM_NUM_PORT_DROP_PROF;
	ctl->rm_last_free_entry[RM_P_DROP_PRF_COS_2] = TM_NUM_PORT_DROP_PROF;
	ctl->rm_last_free_entry[RM_P_DROP_PRF_COS_3] = TM_NUM_PORT_DROP_PROF;
	ctl->rm_last_free_entry[RM_P_DROP_PRF_COS_4] = TM_NUM_PORT_DROP_PROF;
	ctl->rm_last_free_entry[RM_P_DROP_PRF_COS_5] = TM_NUM_PORT_DROP_PROF;
	ctl->rm_last_free_entry[RM_P_DROP_PRF_COS_6] = TM_NUM_PORT_DROP_PROF;
	ctl->rm_last_free_entry[RM_P_DROP_PRF_COS_7] = TM_NUM_PORT_DROP_PROF;

	ctl->rm_free_nodes[RM_Q_LVL] = rm_new_chunk(0, total_queues, NULL);
	ctl->rm_free_nodes[RM_A_LVL] = rm_new_chunk(0, total_a_nodes, NULL);
	ctl->rm_free_nodes[RM_B_LVL] = rm_new_chunk(0, total_b_nodes, NULL);
	ctl->rm_free_nodes[RM_C_LVL] = rm_new_chunk(0, total_c_nodes, NULL);

	*hndl = ctl;

out:
	if ((rc) && (ctl))
		rm_close((rmctl_t)ctl);

	return rc;
}


/**
 */
int rm_close(rmctl_t hndl)
{
	int i;

	DECLARE_RM_HANDLE(ctl, hndl)
	CHECK_RM_HANDLE(ctl)

	ctl->magic = 0;

	/* Free dynamically allocated arrays */
	tm_free(ctl->rm_queue_array);
	tm_free(ctl->rm_a_node_array);
	tm_free(ctl->rm_b_node_array);
	tm_free(ctl->rm_c_node_array);
	tm_free(ctl->rm_port_array);

	tm_free(ctl->rm_wred_queue_curves);
	tm_free(ctl->rm_wred_a_node_curves);
	tm_free(ctl->rm_wred_b_node_curves);
	for (i = 0; i < RM_COS; i++)
		tm_free(ctl->rm_wred_c_node_curves[i]);
	tm_free(ctl->rm_wred_port_curves);
	tm_free(ctl->rm_queue_drop_profiles);
	tm_free(ctl->rm_a_node_drop_profiles);
	tm_free(ctl->rm_b_node_drop_profiles);
	for (i = 0; i < RM_COS; i++)
		tm_free(ctl->rm_wred_port_curves_cos[i]);

	for (i = 0; i < RM_COS; i++)
		tm_free(ctl->rm_c_node_drop_profiles[i]);

	tm_free(ctl->rm_port_drop_profiles);
	for (i = 0; i < RM_COS; i++)
		tm_free(ctl->rm_port_drop_profiles_cos[i]);


	/* Free list of chunks */
	clear_chunk_list(ctl->rm_free_nodes[RM_Q_LVL]);
	clear_chunk_list(ctl->rm_free_nodes[RM_A_LVL]);
	clear_chunk_list(ctl->rm_free_nodes[RM_B_LVL]);
	clear_chunk_list(ctl->rm_free_nodes[RM_C_LVL]);

	/* free rm handle */
	tm_free(ctl);
	return 0;
}

