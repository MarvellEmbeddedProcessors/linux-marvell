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

#ifndef SET_HW_REGISTERS_H
#define SET_HW_REGISTERS_H

#include "tm_core_types.h"

int set_hw_fixed_port_periodic_scheme(tm_handle hndl);
int set_hw_fixed_c_level_periodic_scheme(tm_handle hndl);
int set_hw_fixed_b_level_periodic_scheme(tm_handle hndl);
int set_hw_fixed_a_level_periodic_scheme(tm_handle hndl);
int set_hw_fixed_queue_periodic_scheme(tm_handle hndl);

int set_hw_fixed_port_shaping_status(tm_handle hndl, uint8_t shaping_status);

int set_hw_fixed_shaping_status(tm_handle hndl, enum tm_level level);

int set_hw_dwrr_limit(tm_handle hndl);

int set_hw_gen_conf(tm_handle hndl);

int set_hw_max_dp_mode(tm_handle hndl);

int set_hw_queues_wred_curve(tm_handle hndl, uint8_t curve_ind);
int set_hw_queues_default_wred_curve(tm_handle hndl, uint8_t *prob_array);

int set_hw_a_nodes_wred_curve(tm_handle hndl, uint8_t curve_ind);
int set_hw_a_nodes_default_wred_curve(tm_handle hndl, uint8_t *prob_array);

int set_hw_b_nodes_wred_curve(tm_handle hndl, uint8_t curve_ind);
int set_hw_b_nodes_default_wred_curve(tm_handle hndl, uint8_t *prob_array);

int set_hw_c_nodes_wred_curve(tm_handle hndl, uint8_t cos, uint8_t curve_ind);
int set_hw_c_nodes_default_wred_curve(tm_handle hndl, uint8_t cos, uint8_t *prob_array);

int set_hw_ports_wred_curve(tm_handle hndl, uint8_t curve_ind);
int set_hw_ports_default_wred_curve(tm_handle hndl, uint8_t *prob_array);

int set_hw_ports_wred_curve_cos(tm_handle hndl, uint8_t cos, uint8_t curve_ind);
int set_hw_ports_default_wred_curve_cos(tm_handle hndl, uint8_t cos, uint8_t *prob_array);

int set_hw_queue_drop_profile(tm_handle hndl, uint32_t prof_ind);
int set_hw_queue_default_drop_profile(tm_handle hndl, struct tm_drop_profile *profile);

int set_hw_a_nodes_drop_profile(tm_handle hndl, uint32_t prof_ind);
int set_hw_a_nodes_default_drop_profile(tm_handle hndl, struct tm_drop_profile *profile);

int set_hw_b_nodes_drop_profile(tm_handle hndl, uint32_t prof_ind);
int set_hw_b_nodes_default_drop_profile(tm_handle hndl, struct tm_drop_profile *profile);

int set_hw_c_nodes_drop_profile(tm_handle hndl, uint8_t cos, uint32_t prof_ind);
int set_hw_c_nodes_default_drop_profile(tm_handle hndl, struct tm_drop_profile *profile, uint8_t cos);

int set_hw_ports_drop_profile(tm_handle hndl, uint32_t prof_ind, uint8_t port_ind);
int set_hw_ports_default_drop_profile(tm_handle hndl, struct tm_drop_profile *profile, uint8_t port_ind);

int set_hw_ports_drop_profile_cos(tm_handle hndl, uint8_t cos, uint32_t prof_ind, uint8_t port_ind);
int set_hw_ports_default_drop_profile_cos(tm_handle hndl,  struct tm_drop_profile *profile,
	uint8_t cos, uint8_t port_ind);

int set_hw_shaping_profile(tm_handle hndl,
								  enum tm_level level,
								  uint32_t node_ind,
								  uint32_t prof_ind);

int set_hw_drop_aqm_mode(tm_handle hndl);

int set_hw_drop_color_num(tm_handle hndl);

int set_hw_tm2tm_aqm_mode(tm_handle hndl);

int set_hw_periodic_scheme(tm_handle hndl);

int set_hw_map(tm_handle hndl, enum tm_level lvl, uint32_t index);

int set_hw_queue(tm_handle hndl, uint32_t index);
int get_hw_queue(tm_handle hndl, uint32_t index, struct tm_queue *queue);

int set_hw_a_node(tm_handle hndl, uint32_t index);
int get_hw_a_node(tm_handle hndl, uint32_t index, struct tm_a_node *node);

int set_hw_b_node(tm_handle hndl, uint32_t index);
int get_hw_b_node(tm_handle hndl, uint32_t index, struct tm_b_node *node);

int set_hw_c_node(tm_handle hndl, uint32_t index);
int get_hw_c_node(tm_handle hndl, uint32_t index, struct tm_c_node *node);

int set_hw_queue_elig_prio_func_ptr(tm_handle hndl, uint32_t ind);
int set_hw_a_node_elig_prio_func_ptr(tm_handle hndl, uint32_t ind);
int set_hw_b_node_elig_prio_func_ptr(tm_handle hndl, uint32_t ind);
int set_hw_c_node_elig_prio_func_ptr(tm_handle hndl, uint32_t ind);
int set_hw_port_elig_prio_func_ptr(tm_handle hndl, uint8_t port_ind);

/* in following functions  NULL value of pcbs/pebs   will cause to set cbs/ebs system default value */
int set_hw_queue_shaping_ex(tm_handle hndl, uint32_t index,
							uint32_t cbw, uint32_t ebw, uint32_t *pcbs, uint32_t *pebs);
int set_hw_queue_shaping_def(tm_handle hndl, uint32_t index);
int get_hw_queue_shaping(tm_handle hndl, uint32_t index,
							uint32_t *pcbw, uint32_t *pebw, uint32_t *pcbs, uint32_t *pebs);

int set_hw_a_node_shaping_ex(tm_handle hndl, uint32_t index,
							uint32_t cbw, uint32_t ebw, uint32_t *pcbs, uint32_t *pebs);
int set_hw_a_node_shaping_def(tm_handle hndl, uint32_t index);
int get_hw_a_node_shaping(tm_handle hndl, uint32_t index,
							uint32_t *pcbw, uint32_t *pebw, uint32_t *pcbs, uint32_t *pebs);

int set_hw_b_node_shaping_ex(tm_handle hndl, uint32_t index,
							uint32_t cbw, uint32_t ebw, uint32_t *pcbs, uint32_t *pebs);
int set_hw_b_node_shaping_def(tm_handle hndl, uint32_t index);
int get_hw_b_node_shaping(tm_handle hndl, uint32_t index,
							uint32_t *pcbw, uint32_t *pebw, uint32_t *pcbs, uint32_t *pebs);

int set_hw_c_node_shaping_ex(tm_handle hndl, uint32_t index,
							uint32_t cbw, uint32_t ebw, uint32_t *pcbs, uint32_t *pebs);
int set_hw_c_node_shaping_def(tm_handle hndl, uint32_t index);
int get_hw_c_node_shaping(tm_handle hndl, uint32_t index,
							uint32_t *pcbw, uint32_t *pebw, uint32_t *pcbs, uint32_t *pebs);

int set_hw_port_shaping_ex(tm_handle hndl, uint8_t index,
							uint32_t cbw, uint32_t ebw, uint32_t *pcbs, uint32_t *pebs);
int set_hw_port_shaping_def(tm_handle hndl, uint8_t index);
int get_hw_port_shaping(tm_handle hndl, uint32_t index,
						uint32_t *pcbw, uint32_t *pebw, uint32_t *pcbs, uint32_t *pebs);

int set_hw_port_scheduling(tm_handle hndl, uint8_t port_ind);

int set_hw_port_drop(tm_handle hndl, uint8_t port_ind);
int set_hw_port_drop_cos(tm_handle hndl, uint8_t port_ind, uint8_t cos);

int set_hw_port(tm_handle hndl, uint8_t index);
int get_hw_port(tm_handle hndl, uint8_t index, struct tm_port *port);

int set_hw_tree_deq_status(tm_handle hndl);

#ifdef MV_QMTM_NSS_A0
int set_hw_tree_dwrr_priority(tm_handle hndl);
#endif

int set_hw_deq_status(tm_handle hndl, enum tm_level lvl, uint32_t index);

int set_hw_disable_ports(tm_handle hndl, uint32_t total_ports);



int set_hw_q_elig_prio_func_entry(tm_handle hndl, uint16_t func_offset);
int set_hw_a_lvl_elig_prio_func_entry(tm_handle hndl, uint16_t func_offset);
int set_hw_b_lvl_elig_prio_func_entry(tm_handle hndl, uint16_t func_offset);
int set_hw_c_lvl_elig_prio_func_entry(tm_handle hndl, uint16_t func_offset);
int set_hw_p_lvl_elig_prio_func_entry(tm_handle hndl, uint16_t func_offset);

int set_hw_elig_prio_func_tbl_q_level(tm_handle hndl);
int set_hw_elig_prio_func_tbl_a_level(tm_handle hndl);
int set_hw_elig_prio_func_tbl_b_level(tm_handle hndl);
int set_hw_elig_prio_func_tbl_c_level(tm_handle hndl);
int set_hw_elig_prio_func_tbl_p_level(tm_handle hndl);



int set_hw_port_deficit_clear(tm_handle hndl, uint8_t index);

int set_hw_c_node_deficit_clear(tm_handle hndl, uint32_t index);

int set_hw_b_node_deficit_clear(tm_handle hndl, uint32_t index);

int set_hw_a_node_deficit_clear(tm_handle hndl, uint32_t index);

int set_hw_queue_deficit_clear(tm_handle hndl, uint32_t index);

int set_hw_dp_remote_resp(tm_handle hndl, enum tm2tm_channel remote_lvl);

int set_hw_dp_local_resp(tm_handle hndl, uint8_t port_dp, enum tm_level local_lvl);

int set_hw_port_sms_attr_pbase(tm_handle hndl, uint8_t index);

int set_hw_port_sms_attr_qmap_pars(tm_handle hndl, uint8_t index);

int set_hw_dp_source(tm_handle hndl);

int set_hw_queue_cos(tm_handle hndl, uint32_t index);

int set_hw_register_db_default(tm_handle hndl);


int get_hw_queue_elig_prio_func_ptr(tm_handle hndl, uint32_t ind, uint8_t *pfunc);
int get_hw_a_node_elig_prio_func_ptr(tm_handle hndl, uint32_t ind, uint8_t *pfunc);
int get_hw_b_node_elig_prio_func_ptr(tm_handle hndl, uint32_t ind, uint8_t *pfunc);
int get_hw_c_node_elig_prio_func_ptr(tm_handle hndl, uint32_t ind, uint8_t *pfunc);
int get_hw_port_elig_prio_func_ptr(tm_handle hndl, uint32_t ind, uint8_t *pfunc);

int get_hw_elig_prio_func(tm_handle hndl, enum tm_level level, uint16_t func_offset, uint16_t *table);


int get_hw_port_status(tm_handle hndl,
					 uint8_t index,
					 struct tm_port_status *tm_status);

int get_hw_c_node_status(tm_handle hndl,
					   uint32_t index,
					   struct tm_node_status *tm_status);

int get_hw_b_node_status(tm_handle hndl,
					   uint32_t index,
					   struct tm_node_status *tm_status);

int get_hw_a_node_status(tm_handle hndl,
					   uint32_t index,
					   struct tm_node_status *tm_status);

int get_hw_queue_status(tm_handle hndl,
					   uint32_t index,
					   struct tm_node_status *tm_status);

int get_hw_queue_length(tm_handle hndl,
					  enum tm_level level,
					  uint32_t index,
					  uint32_t *av_q_length);

int get_hw_sched_errors(tm_handle hndl, struct tm_error_info *info);

int get_hw_drop_errors(tm_handle hndl, struct tm_error_info *info);

int tm_dump_port_hw(tm_handle hndl, uint32_t portIndex);

int set_hw_elig_per_queue_range(tm_handle hndl, uint32_t startInd, uint32_t endInd, uint8_t elig);

int check_hw_drop_path(tm_handle hndl, uint32_t timeout, uint8_t full_path);

int show_hw_elig_prio_func(tm_handle hndl, enum tm_level level, uint16_t func_offset);



int set_hw_queue_map_directly(tm_handle hndl, uint32_t queue_index, uint32_t parent);

int set_hw_a_node_map_directly(tm_handle hndl,
				 uint32_t a_node_index,
				 uint32_t parent,
				 uint32_t first_child,
				 uint32_t last_child);


#endif   /* SET_HW_REGISTERS_H */
