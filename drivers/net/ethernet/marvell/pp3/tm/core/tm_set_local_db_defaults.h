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

#ifndef TM_SET_LOCAL_DB_DEFAULTS_H
#define TM_SET_LOCAL_DB_DEFAULTS_H

#include "tm_core_types.h"
#include "rm_internal_types.h"


int set_sw_sched_conf_default(void * hndl);
int set_sw_gen_conf_default(void * hndl);

int	set_sw_drop_profile_default(struct tm_drop_profile *profile,
							uint32_t prof_index);

int set_sw_wred_curve_default(struct tm_wred_curve *curve,
											uint16_t curve_index);
int set_sw_queue_default(struct tm_queue *array,
									   uint32_t queue_ind,
									   struct rmctl *rm);


int set_sw_a_node_default(struct tm_a_node *array,
										uint32_t node_ind,
										struct rmctl *rm);

int set_sw_b_node_default(struct tm_b_node *array,
										uint32_t node_ind,
										struct rmctl *rm);

int set_sw_c_node_default(struct tm_c_node *array,
										uint32_t node_ind,
										struct rmctl *rm);
int set_sw_port_default(struct tm_port *array,
									uint8_t port_ind,
									struct rmctl *rm);
/* initializing default values for eligible functions tables*/
void set_default_node_elig_prio_func_table(struct tm_elig_prio_func_node *func_table);
void set_default_queue_elig_prio_func_table(struct tm_elig_prio_func_queue  *func_table);

#endif   /* TM_SET_LOCAL_DB_DEFAULTS_H */
