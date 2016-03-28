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

#ifndef MV_TM_SCHEME__H
#define MV_TM_SCHEME__H

#include "common/mv_sw_if.h"
#include "tm/mv_tm.h"

enum mv_tm_source_type {
	SOURCE_PPC_DP,  /* PP Data Path ports */
	SOURCE_PPC_MNT, /* PP Control ports */
	SOURCE_EMAC,    /* EMAC ports */
	SOURCE_CMAC,    /* CMAC ports */
	SOURCE_HMAC,    /* HMAC ports */
	SOURCE_DROP,    /* Drop ports */
	SOURCE_LAST
};

/* Return number of ports for given type.
   Example: return 2 for tm_scheme_type_ports_num(SOURCE_PPC_MNT).
   Return -1 for invalid type. */
int mv_tm_scheme_type_ports_num(enum mv_tm_source_type type);

/* Translate source type and id to port id.
   Example: return 7 for tm_scheme_source_to_port(SOURCE_EMAC, 3) for A0.
   Return -1 if source id is invalid. */
int mv_tm_scheme_source_to_port(enum mv_tm_source_type type, int id);

/* Translate port id to source type and source id, return -1 if port id is invalid. */
int mv_tm_scheme_port_to_source(int port_id, int *source_id);

/* Return total number of ports. */
int mv_tm_scheme_ports_num(void);

/* Return base and number of A nodes that attached to port number port_id.
   Return -1 if port_id is invalid or not in use. */
int mv_tm_scheme_port_a_node_get(int port_id, int *base, int *num);

/* Return base and number of B nodes that attached to port number port_id.
   Return -1 if port_id is invalid or not in use. */
int mv_tm_scheme_port_b_nodes_get(int port_id, int *base, int *num);

/* Return base and number of C nodes that attached to port number port_id.
   Return -1 if port_id is invalid or not in use. */
int mv_tm_scheme_port_c_nodes_get(int port_id, int *base, int *num);


/* Return base and number of B nodes that attached to Cnode number cnode_id.
   Return -1 if port_id is invalid or not in use. */
int mv_tm_scheme_c_node_b_nodes_get(int bnode_id, int *base, int *num);

/* Return base and number of A nodes that attached to B node number bnode_id.
   Return -1 if port_id is invalid or not in use. */
int mv_tm_scheme_b_node_a_nodes_get(int bnode_id, int *base, int *num);

/* Return base and number of queues that attached to A node number anode_id.
   Return -1 if anode_id is invalid or not in use. */
int mv_tm_scheme_a_node_queues_get(int anode_id, int *base, int *num);

int mv_tm_scheme_sub_nodes_get(enum tm_level, int node_id, int *base, int *num);
int mv_tm_scheme_parent_node_get(enum tm_level level, int node_id, int *parent);

/* Retrun A node, B node, C node and port number that attached to queue number q_id.
   Return -1 if q_id is invalid or not in use. */
int mv_tm_scheme_queue_path_get(int q_id, int *anode, int *bnode, int *cnode, int *port);

#endif /* MV_TM_SCHEME__H */
