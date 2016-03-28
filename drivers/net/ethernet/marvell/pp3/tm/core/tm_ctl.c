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

#include "tm_ctl.h"
#include "tm_set_local_db_defaults.h"
#include "tm_hw_configuration_interface.h"
#include "tm_os_interface.h"
#include "tm_locking_interface.h"
#include "rm_internal_types.h"
#include "rm_ctl.h"
#include "rm_free.h"
#include "rm_status.h"
#include "rm_list.h"
#include "tm_rw_registers_interface.h"
#include "tm_nodes_ctl.h"
#include "tm_drop.h"
#include "tm_nodes_utils.h"
#include "tm_elig_prio_func.h"
#include "tm_errcodes.h"
#include "set_hw_registers.h"
#include "tm_get_gen_param_interface.h"
#include "tm_core_types.h"


int tm_lib_init_hw(tm_handle hndl)
{
	int rc;
	DECLARE_TM_CTL_PTR(ctl, hndl)
	CHECK_TM_CTL_PTR(ctl)

	/* Get general parameters : LAD frequency,  dwrr params  - they are readed from H/W resources */
	rc = tm_get_gen_params(ctl);
	if (rc)
		goto out;

	/* configure default drop registers */
	rc = _tm_config_default_drop_hw((tm_handle)ctl);
	if (rc)
		goto out;

	rc = tm_config_elig_prio_func_table((tm_handle)ctl, 1); /* with H/W update*/
	if (rc)
		goto out;

	/* TBD */
	/*rc = set_hw_disable_ports((tm_handle) ctl, get_tm_port_count());*/
out:
	return rc;
}


/**
 */
int tm_lib_open(const char *cProductName, tm_handle hEnv, tm_handle *htm)
{
	rmctl_t rm = NULL;
	struct tm_ctl *ctl = NULL;
	uint32_t total_queues;
	uint16_t total_a_nodes;
	uint16_t total_b_nodes;
	uint16_t total_c_nodes;
	uint8_t total_ports;
	int rc = 0;
	unsigned int i;
	unsigned int j;


	if ((*htm) != NULL)
	{
		rc = -EINVAL;
		goto out;
	}

	*htm = NULL;

	if (init_tm_hardware_configuration(cProductName))
	{
		rc = TM_CONF_INVALID_PROD_NAME;
		return rc;
	}

	/* Total number of nodes per level */
	total_ports = get_tm_port_count();
	total_c_nodes = get_tm_c_nodes_count();
	total_b_nodes = get_tm_b_nodes_count();
	total_a_nodes = get_tm_a_nodes_count();
	total_queues = get_tm_queues_count();

	/* Allocate handle */
	ctl = tm_malloc(sizeof(*ctl) * 1);
	if (!ctl)
	{
		rc = -ENOMEM;
		goto out;
	}
	tm_memset(ctl, 0, sizeof(*ctl));

	ctl->magic = TM_MAGIC;
	ctl->hEnv=hEnv;

	rc = rm_open(total_ports, total_c_nodes, total_b_nodes, total_a_nodes, total_queues, &rm);
	if (rc)
		goto out;

	ctl->rm = rm;

	if (rc)
		goto out;

	/* allocate global arrays */
	ctl->tm_port_array = tm_malloc(sizeof(struct tm_port) * total_ports);
	if (ctl->tm_port_array == NULL)
	{
		rc = -ENOMEM;
		goto out;
	}

	ctl->tm_c_node_array = tm_malloc(sizeof(struct tm_c_node) * total_c_nodes);
	if (ctl->tm_c_node_array == NULL)
	{
		rc = -ENOMEM;
		goto out;
	}

	ctl->tm_b_node_array = tm_malloc(sizeof(struct tm_b_node) * total_b_nodes);
	if (ctl->tm_b_node_array == NULL)
	{
		rc = -ENOMEM;
		goto out;
	}

	ctl->tm_a_node_array = tm_malloc(sizeof(struct tm_a_node) * total_a_nodes);
	if (ctl->tm_a_node_array == NULL)
	{
		rc = -ENOMEM;
		goto out;
	}

	ctl->tm_queue_array = tm_malloc(sizeof(struct tm_queue) * total_queues);
	if (ctl->tm_queue_array == NULL)
	{
		rc = -ENOMEM;
		goto out;
	}

	ctl->tm_q_lvl_drop_profiles = tm_malloc(sizeof(struct tm_drop_profile) * TM_NUM_QUEUE_DROP_PROF);
	if (ctl->tm_q_lvl_drop_profiles == NULL) {
		rc = -ENOMEM;
		goto out;
	}

	ctl->tm_a_lvl_drop_profiles = tm_malloc(sizeof(struct tm_drop_profile) * TM_NUM_A_NODE_DROP_PROF);
	if (ctl->tm_a_lvl_drop_profiles == NULL)
	{
		rc = -ENOMEM;
		goto out;
	}

	ctl->tm_b_lvl_drop_profiles = tm_malloc(sizeof(struct tm_drop_profile) * TM_NUM_B_NODE_DROP_PROF);
	if (ctl->tm_b_lvl_drop_profiles == NULL)
	{
		rc = -ENOMEM;
		goto out;
	}

	for (i = 0; i < TM_WRED_COS; i++)
	{
		ctl->tm_c_lvl_drop_profiles[i] = tm_malloc(sizeof(struct tm_drop_profile) * TM_NUM_C_NODE_DROP_PROF);
		if (ctl->tm_c_lvl_drop_profiles[i] == NULL)
		{
			rc = -ENOMEM;
			goto out;
		}
	}

	ctl->tm_p_lvl_drop_profiles =
		tm_malloc(sizeof(struct tm_drop_profile) * TM_NUM_PORT_DROP_PROF);
	if (ctl->tm_p_lvl_drop_profiles == NULL) {
		rc = -ENOMEM;
		goto out;
	}

	for (i = 0; i < TM_WRED_COS; i++) {
		ctl->tm_p_lvl_drop_profiles_cos[i] =
			tm_malloc(sizeof(struct tm_drop_profile) * TM_NUM_PORT_DROP_PROF);
		if (ctl->tm_p_lvl_drop_profiles_cos[i] == NULL) {
			rc = -ENOMEM;
			goto out;
		}
	}


	ctl->tm_wred_q_lvl_curves = tm_malloc(sizeof(struct tm_wred_curve) * TM_NUM_WRED_QUEUE_CURVES);
	if (ctl->tm_wred_q_lvl_curves == NULL)
	{
		rc = -ENOMEM;
		goto out;
	}

	ctl->tm_wred_a_lvl_curves = tm_malloc(sizeof(struct tm_wred_curve) * TM_NUM_WRED_A_NODE_CURVES);
	if (ctl->tm_wred_a_lvl_curves == NULL)
	{
		rc = -ENOMEM;
		goto out;
	}

	ctl->tm_wred_b_lvl_curves = tm_malloc(sizeof(struct tm_wred_curve) * TM_NUM_WRED_B_NODE_CURVES);
	if (ctl->tm_wred_b_lvl_curves == NULL)
	{
		rc = -ENOMEM;
		goto out;
	}

	for (i = 0; i < TM_WRED_COS; i++)
	{
		ctl->tm_wred_c_lvl_curves[i] = tm_malloc(sizeof(struct tm_wred_curve) * TM_NUM_WRED_C_NODE_CURVES);
		if (ctl->tm_wred_c_lvl_curves[i] == NULL)
		{
			rc = -ENOMEM;
			goto out;
		}
	}

	ctl->tm_wred_ports_curves =
		tm_malloc(sizeof(struct tm_wred_curve) * TM_NUM_WRED_PORT_CURVES);
	if (ctl->tm_wred_ports_curves == NULL) {
		rc = -ENOMEM;
		goto out;
	}

/* BC2 */
	for (i = 0; i < TM_WRED_COS; i++) {
		ctl->tm_wred_ports_curves_cos[i] =
			tm_malloc(sizeof(struct tm_wred_curve) * TM_NUM_WRED_PORT_CURVES);
		if (ctl->tm_wred_ports_curves_cos[i] == NULL) {
			rc = -ENOMEM;
			goto out;
		}
	}


	ctl->tm_q_lvl_drop_prof_ptr = tm_malloc(sizeof(uint16_t) * total_queues);
	if (ctl->tm_q_lvl_drop_prof_ptr == NULL)
	{
		rc = -ENOMEM;
		goto out;
	}

	ctl->tm_a_lvl_drop_prof_ptr = tm_malloc(sizeof(uint16_t) * total_a_nodes);
	if (ctl->tm_a_lvl_drop_prof_ptr == NULL)
	{
		rc = -ENOMEM;
		goto out;
	}

	ctl->tm_b_lvl_drop_prof_ptr = tm_malloc(sizeof(uint8_t) * total_b_nodes);
	if (ctl->tm_b_lvl_drop_prof_ptr == NULL)
	{
		rc = -ENOMEM;
		goto out;
	}

	for (i = 0; i < TM_WRED_COS; i++)
	{
		ctl->tm_c_lvl_drop_prof_ptr[i] = tm_malloc(sizeof(uint8_t) * total_c_nodes);
		if (ctl->tm_c_lvl_drop_prof_ptr[i] == NULL)
		{
			rc = -ENOMEM;
			goto out;
		}
	}


	ctl->tm_p_lvl_drop_prof_ptr = tm_malloc(sizeof(uint8_t) * total_ports);
	if (ctl->tm_p_lvl_drop_prof_ptr == NULL) {
		rc = -ENOMEM;
		goto out;
	}

	for (i = 0; i < TM_WRED_COS; i++) {
		ctl->tm_p_lvl_drop_prof_ptr_cos[i] =
			tm_malloc(sizeof(uint8_t) * total_ports);
		if (ctl->tm_p_lvl_drop_prof_ptr_cos[i] == NULL) {
			rc = -ENOMEM;
			goto out;
		}
	}

	ctl->tm_q_cos = tm_malloc(sizeof(uint16_t) * total_queues);
	if (ctl->tm_q_cos == NULL) {
		rc = -ENOMEM;
		goto out;
	}
	tm_memset(ctl->tm_q_cos, 0, sizeof(uint16_t) * total_queues);

	ctl->tm_port_sms_attr_pbase = tm_malloc(sizeof(struct port_sms_attr_pbase) * total_ports);
	if (ctl->tm_port_sms_attr_pbase == NULL) {
		rc = -ENOMEM;
		goto out;
	}
	tm_memset(ctl->tm_port_sms_attr_pbase, 0, sizeof(struct port_sms_attr_pbase) * total_ports);

	ctl->tm_port_sms_attr_qmap_pars = tm_malloc(sizeof(struct port_sms_attr_qmap_parsing) * total_ports);
	if (ctl->tm_port_sms_attr_qmap_pars == NULL) {
		rc = -ENOMEM;
		goto out;
	}
	tm_memset(ctl->tm_port_sms_attr_qmap_pars, 0, sizeof(struct port_sms_attr_qmap_parsing) * total_ports);

	/* set to default all the arrays fields */

	for (i = 0; i < total_ports; i++)
		ctl->tm_port_sms_attr_pbase[i].pbase = 0x4;

	for (i = 0; i < total_ports; i++) {
		rc = set_sw_port_default(ctl->tm_port_array, (uint8_t)i, rm);
		if (rc)
			goto out;
	}

	for (i = 0; i < total_c_nodes; i++) {
		rc = set_sw_c_node_default(ctl->tm_c_node_array, i, rm);
		if (rc)
			goto out;
		ctl->tm_c_node_array[i].parent_port = total_ports - 1;
	}

	for (i = 0; i < total_b_nodes; i++) {
		rc = set_sw_b_node_default(ctl->tm_b_node_array, i, rm);
		if (rc)
			goto out;
		ctl->tm_b_node_array[i].parent_c_node = total_c_nodes - 1;
	}

	for (i = 0; i < total_a_nodes; i++) {
		rc = set_sw_a_node_default(ctl->tm_a_node_array, i, rm);
		if (rc)
			goto out;
		ctl->tm_a_node_array[i].parent_b_node = total_b_nodes - 1;
	}

	for (i = 0; i < total_queues; i++) {
		rc = set_sw_queue_default(ctl->tm_queue_array, i, rm);
		if (rc)
			goto out;
		ctl->tm_queue_array[i].parent_a_node = total_a_nodes - 1;
	}

	for (i = 0; i < TM_NUM_QUEUE_DROP_PROF; i++) {
		rc = set_sw_drop_profile_default(ctl->tm_q_lvl_drop_profiles, i);
		if (rc)
			goto out;
	}

	for (i = 0; i < TM_NUM_A_NODE_DROP_PROF; i++) {
		rc = set_sw_drop_profile_default(ctl->tm_a_lvl_drop_profiles, i);
		if (rc)
			goto out;
	}

	for (i = 0; i < TM_NUM_B_NODE_DROP_PROF; i++) {
		rc = set_sw_drop_profile_default(ctl->tm_b_lvl_drop_profiles, i);
		if (rc)
			goto out;
	}

	for (j = 0; j < TM_WRED_COS; j++) {
		for (i = 0; i < TM_NUM_C_NODE_DROP_PROF; i++)
		{
			rc = set_sw_drop_profile_default(ctl->tm_c_lvl_drop_profiles[j], i);
			if (rc)
				goto out;
		}
	}

	for (i = 0; i < TM_NUM_PORT_DROP_PROF; i++) {
		rc = set_sw_drop_profile_default(ctl->tm_p_lvl_drop_profiles, i);
		if (rc)
			goto out;
	}

	for (j = 0; j < TM_WRED_COS; j++) {
		for (i = 0; i < TM_NUM_PORT_DROP_PROF; i++) {
			rc = set_sw_drop_profile_default(ctl->tm_p_lvl_drop_profiles_cos[j], i);
			if (rc)
				goto out;
		}
	}

	for (i = 0; i < TM_NUM_WRED_QUEUE_CURVES; i++) {
		rc = set_sw_wred_curve_default(ctl->tm_wred_q_lvl_curves, (uint16_t)i);
		if (rc)
			goto out;
	}

	for (i = 0; i < TM_NUM_WRED_A_NODE_CURVES; i++) {
		rc = set_sw_wred_curve_default(ctl->tm_wred_a_lvl_curves, (uint16_t)i);
		if (rc)
			goto out;
	}

	for (i = 0; i < TM_NUM_WRED_B_NODE_CURVES; i++) {
		rc = set_sw_wred_curve_default(ctl->tm_wred_b_lvl_curves, (uint16_t)i);
		if (rc)
			goto out;
	}

	for (j = 0; j < TM_WRED_COS; j++) {
		for (i = 0; i < TM_NUM_WRED_C_NODE_CURVES; i++) {
			rc = set_sw_wred_curve_default(ctl->tm_wred_c_lvl_curves[j], (uint16_t)i);
			if (rc)
				goto out;
		}
	}

	for (i = 0; i < TM_NUM_WRED_PORT_CURVES; i++) {
		rc = set_sw_wred_curve_default(ctl->tm_wred_ports_curves, (uint16_t)i);
		if (rc)
			goto out;
	}

	for (j = 0; j < TM_WRED_COS; j++) {
		for (i = 0; i < TM_NUM_WRED_PORT_CURVES; i++) {
			rc = set_sw_wred_curve_default(ctl->tm_wred_ports_curves_cos[j], (uint16_t)i);
			if (rc)
				goto out;
		}
	}

	for (i = 0; i < total_queues; i++)
		ctl->tm_q_lvl_drop_prof_ptr[i] = 0;

	for (i = 0; i < total_a_nodes; i++)
		ctl->tm_a_lvl_drop_prof_ptr[i] = 0;

	for (i = 0; i < total_b_nodes; i++)
		ctl->tm_b_lvl_drop_prof_ptr[i] = 0;

	for (i = 0; i < TM_WRED_COS; i++) {
		for (j = 0; j < total_c_nodes; j++)
			ctl->tm_c_lvl_drop_prof_ptr[i][j] = 0;
		for (j = 0; j < total_ports; j++)
			ctl->tm_p_lvl_drop_prof_ptr_cos[i][j] = 0;
	}

	for (i = 0; i < total_ports; i++)
		ctl->tm_p_lvl_drop_prof_ptr[i] = 0;

	/* Reshuffling changes list */
	ctl->list.next = NULL;

	/* Shaping Profile #0 is reserved in the library and represents no
	 * shaping. If the user application doesn't want to apply shaping
	 * for a node the TM_INF_PROFILE must be used for
	 * shaping profile referece */

	/* Drop Profiles #0 are reserved in the library and represent no drop.
	 * If the user application doesn't want to apply drop for a node
	 * the TM_NO_DROP_PROFILE must be used for drop profile referece */
	/* WRED Curves #0 are reserved in the library and represent traditional
	 * curve with 50% maximal drop probability. */
	/* Create default drop profiles & curves for each level */
	rc = _tm_config_default_drop_sw((tm_handle)ctl);
	if (rc)
		goto out;

	/* scheduling configuration */
	rc = set_sw_sched_conf_default((tm_handle)ctl);
	if (rc)
		goto out;

	/* other scheduler registers */
	rc = set_sw_gen_conf_default((tm_handle)ctl);
	if (rc)
		goto out;

	rc = tm_config_elig_prio_func_table((tm_handle)ctl, 0); /* without H/W update*/
	if (rc)
		goto out;

	/* set the handle */
	*htm = (tm_handle)ctl;

out:
	if ((rc) && (ctl))
		tm_lib_close((tm_handle)ctl);
	return rc;
}


/**
 */
int tm_lib_close(tm_handle hndl)
{
	unsigned int i,j;
	int rc;

	DECLARE_TM_CTL_PTR(ctl, (tm_handle)hndl)
	DECLARE_RM_HANDLE(rm, ctl->rm)
	CHECK_TM_CTL_PTR(ctl)

	/* Free use lists of Drop profiles */
	/* Relevant for Port level only */
	for (i = 0; i < TM_NUM_PORT_DROP_PROF; i++) {
		if (ctl->tm_p_lvl_drop_profiles[i].use_list != NULL) {
			rc = rm_list_delete(rm,
								ctl->tm_p_lvl_drop_profiles[i].use_list);
			if (rc)
				return rc;
		}
	}

	for (i = 0; i < TM_WRED_COS; i++)
		for (j = 0; j < TM_NUM_PORT_DROP_PROF; j++) {
			if (ctl->tm_p_lvl_drop_profiles_cos[i][j].use_list != NULL) {
				rc = rm_list_delete(rm,
									ctl->tm_p_lvl_drop_profiles_cos[i][j].use_list);
				if (rc)
					return rc;
			}
		}
	rm_close(rm);

	/* Clean reshuffling list */
	tm_clean_list(hndl);

	rc = tm_nodes_lock(TM_ENV(ctl));
	if (rc)
		return rc;
	tm_free(ctl->tm_queue_array);
	tm_free(ctl->tm_a_node_array);
	tm_free(ctl->tm_b_node_array);
	tm_free(ctl->tm_c_node_array);
	tm_free(ctl->tm_port_array);
	tm_free(ctl->tm_q_lvl_drop_prof_ptr);
	tm_free(ctl->tm_a_lvl_drop_prof_ptr);
	tm_free(ctl->tm_b_lvl_drop_prof_ptr);
	for (i = 0; i < TM_WRED_COS; i++) {
		tm_free(ctl->tm_c_lvl_drop_prof_ptr[i]);
		tm_free(ctl->tm_p_lvl_drop_prof_ptr_cos[i]);
	}
	tm_free(ctl->tm_p_lvl_drop_prof_ptr);
	tm_free(ctl->tm_q_cos);
	tm_nodes_unlock(TM_ENV(ctl));

	rc = tm_glob_lock(TM_ENV(ctl));
	if (rc)
		return rc;

	tm_free(ctl->tm_q_lvl_drop_profiles);
	tm_free(ctl->tm_a_lvl_drop_profiles);
	tm_free(ctl->tm_b_lvl_drop_profiles);
	for (i = 0; i < TM_WRED_COS; i++) {
		tm_free(ctl->tm_c_lvl_drop_profiles[i]);
		tm_free(ctl->tm_p_lvl_drop_profiles_cos[i]);
	}
	tm_free(ctl->tm_p_lvl_drop_profiles);
	tm_free(ctl->tm_wred_q_lvl_curves);
	tm_free(ctl->tm_wred_a_lvl_curves);
	tm_free(ctl->tm_wred_b_lvl_curves);
	for (i = 0; i < TM_WRED_COS; i++) {
		tm_free(ctl->tm_wred_c_lvl_curves[i]);
		tm_free(ctl->tm_wred_ports_curves_cos[i]);
	}
	tm_free(ctl->tm_wred_ports_curves);
	tm_free(ctl->tm_port_sms_attr_pbase);
	tm_free(ctl->tm_port_sms_attr_qmap_pars);
	tm_glob_unlock(TM_ENV(ctl));


	/* release TM lib handle */
	tm_free(ctl);

	return 0;
}

int tm_lib_init_hw_def(tm_handle hndl)
{
	return set_hw_register_db_default(hndl);
}
