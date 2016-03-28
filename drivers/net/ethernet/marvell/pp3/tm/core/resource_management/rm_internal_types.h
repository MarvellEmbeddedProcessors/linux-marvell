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

#ifndef RM_INTERNAL_TYPES_H
#define RM_INTERNAL_TYPES_H

#include "tm/core/tm_core_types.h"

/*nesessary fo RM_HANDLE macro*/
/** Magic number used to determine struct rmctl_t validity.
 */
#define RM_MAGIC 0x1EDA5D


/* following macro declares and checks validity of rmctl */
#define DECLARE_RM_HANDLE(handle, value)	struct rmctl *handle = (struct rmctl *)value;

#define CHECK_RM_HANDLE(handle)	\
{ \
	if (!handle) \
		return -EINVAL; \
	if (handle->magic != RM_MAGIC) \
		return -EBADF; \
}

#define RM_COS   8

#define chunk_ptr   struct rm_chunk*

/**
 */
enum rm_prf_level {
	RM_WRED_Q_CURVE = 0,
	RM_WRED_A_CURVE,
	RM_WRED_B_CURVE,
	RM_WRED_C_CURVE_COS_0,
	RM_WRED_C_CURVE_COS_1,
	RM_WRED_C_CURVE_COS_2,
	RM_WRED_C_CURVE_COS_3,
	RM_WRED_C_CURVE_COS_4,
	RM_WRED_C_CURVE_COS_5,
	RM_WRED_C_CURVE_COS_6,
	RM_WRED_C_CURVE_COS_7,
	RM_WRED_P_CURVE,
	RM_WRED_P_CURVE_COS_0,
	RM_WRED_P_CURVE_COS_1,
	RM_WRED_P_CURVE_COS_2,
	RM_WRED_P_CURVE_COS_3,
	RM_WRED_P_CURVE_COS_4,
	RM_WRED_P_CURVE_COS_5,
	RM_WRED_P_CURVE_COS_6,
	RM_WRED_P_CURVE_COS_7,
	/*----*/
	RM_Q_DROP_PRF,
	RM_A_DROP_PRF,
	RM_B_DROP_PRF,
	RM_C_DROP_PRF_COS_0,
	RM_C_DROP_PRF_COS_1,
	RM_C_DROP_PRF_COS_2,
	RM_C_DROP_PRF_COS_3,
	RM_C_DROP_PRF_COS_4,
	RM_C_DROP_PRF_COS_5,
	RM_C_DROP_PRF_COS_6,
	RM_C_DROP_PRF_COS_7,
	RM_P_DROP_PRF,
	RM_P_DROP_PRF_COS_0,
	RM_P_DROP_PRF_COS_1,
	RM_P_DROP_PRF_COS_2,
	RM_P_DROP_PRF_COS_3,
	RM_P_DROP_PRF_COS_4,
	RM_P_DROP_PRF_COS_5,
	RM_P_DROP_PRF_COS_6,
	RM_P_DROP_PRF_COS_7,
	RM_MAX_PROFILES
};




/** RM node data structure.
 */
struct rm_node {
	uint8_t   used:1;
	uint16_t  parent_ind:14;
	uint16_t  next_free_ind;
	/* Children */
	uint32_t cnt;
	uint16_t  first_child;   /* Head */
	uint16_t  last_child;    /* Tail */
} __ATTRIBUTE_PACKED__;


/** RM entry data structure.
 */
struct rm_entry {
	uint8_t   used:1;
	uint32_t  next_free_ind;
} __ATTRIBUTE_PACKED__;


/** RM chunk data structure.
 */
struct rm_chunk {
	uint32_t  index;
	uint32_t  size;
	struct rm_chunk *next_free;
};


/** Resource Manger handle struct.
 */
struct rmctl {
	uint32_t magic;
	/* RM node arrays */
	struct rm_node *rm_queue_array;
	struct rm_node *rm_a_node_array;
	struct rm_node *rm_b_node_array;
	struct rm_node *rm_c_node_array;
	struct rm_node *rm_port_array;

	/* Statistics - total num of free nodes per level */
	uint8_t  rm_port_cnt;
	uint16_t rm_c_node_cnt;
	uint16_t rm_b_node_cnt;
	uint16_t rm_a_node_cnt;
	uint32_t rm_queue_cnt;

	/* RM entry arrays */
	struct rm_entry *rm_wred_queue_curves;
	struct rm_entry *rm_wred_a_node_curves;
	struct rm_entry *rm_wred_b_node_curves;
	struct rm_entry *rm_wred_c_node_curves[RM_COS];
	struct rm_entry *rm_wred_port_curves;
	struct rm_entry *rm_wred_port_curves_cos[RM_COS];
/*----*/
	struct rm_entry *rm_queue_drop_profiles;
	struct rm_entry *rm_a_node_drop_profiles;
	struct rm_entry *rm_b_node_drop_profiles;
	struct rm_entry *rm_c_node_drop_profiles[RM_COS];
	struct rm_entry *rm_port_drop_profiles;
	struct rm_entry *rm_port_drop_profiles_cos[RM_COS];
/*----*/

	uint32_t rm_first_free_entry[RM_MAX_PROFILES]; /* Heads */
	uint32_t rm_last_free_entry[RM_MAX_PROFILES];  /* Tails */

	/* Total number of nodes within each level */
	uint8_t  rm_total_ports;   /**< total ports number */
	uint16_t rm_total_c_nodes; /**< total c_nodes number */
	uint16_t rm_total_b_nodes; /**< total b_nodes number */
	uint16_t rm_total_a_nodes; /**< total a_nodes number */
	uint32_t rm_total_queues;  /**< total queues number */

	/* Internal DB */
	chunk_ptr rm_free_nodes[4]; /* from RM_Q_LVL to RM_C_LVL, RM_P_LVL not used inside RM */
};


#endif   /* RM_INTERNAL_TYPES_H */
