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

#include "mv_tm_scheme.h"
#include "tm_core_types.h"
#include "rm_status.h"

static uint8_t source_num[SOURCE_LAST] = {2, 2, 4, 2, 1, 2};

static uint8_t type_base[SOURCE_LAST] = {TM_A0_PORT_PPC0_0,
						TM_A0_PORT_PPC1_MNT0,
						TM_A0_PORT_EMAC0,
						TM_A0_PORT_CMAC_IN,
						TM_A0_PORT_HMAC,
						TM_A0_PORT_DROP0};

static uint8_t port_type[MV_TM_MAX_PORTS] = {SOURCE_PPC_DP,
						SOURCE_PPC_DP,
						SOURCE_PPC_MNT,
						SOURCE_PPC_MNT,
						SOURCE_EMAC,
						SOURCE_EMAC,
						SOURCE_EMAC,
						SOURCE_EMAC,
						SOURCE_LAST, /* EMAC 4 not in use */
						SOURCE_LAST, /* EMAC LB not in use */
						SOURCE_CMAC,
						SOURCE_CMAC,
						SOURCE_HMAC,
						SOURCE_LAST,
						SOURCE_DROP,
						SOURCE_DROP};

/* Return number of ports for given type.
   Example: return 2 for tm_scheme_type_ports_num(SOURCE_PPC_MNT).
   Return -1 for invalid type. */
int mv_tm_scheme_type_ports_num(enum mv_tm_source_type type)
{
	TM_WRAPPER_BEGIN(qmtm_hndl, ctl, henv);

	if (type >= SOURCE_LAST)
		rc = -1;
	else
		rc = source_num[type];
	if (rc < 0)
		pr_info("mv_tm_scheme_type_ports_num error: %d\n", rc);

	TM_WRAPPER_END(qmtm_hndl);
}
EXPORT_SYMBOL(mv_tm_scheme_type_ports_num);

/* Translate source type and id to port id.
   Example: return 7 for tm_scheme_source_to_port(SOURCE_EMAC, 3) for A0.
   Return -1 if source id is invalid. */
int mv_tm_scheme_source_to_port(enum mv_tm_source_type type, int id)
{
	TM_WRAPPER_BEGIN(qmtm_hndl, ctl, henv);

	if ((type >= SOURCE_LAST) || (id >= source_num[type]))
		rc = -1;
	else
		rc = type_base[type] + id;
	if (rc < 0)
		pr_info("mv_tm_scheme_source_to_port error: %d\n", rc);

	TM_WRAPPER_END(qmtm_hndl);
}
EXPORT_SYMBOL(mv_tm_scheme_source_to_port);

/* Translate port id to source type and source id, return -1 if port id is invalid. */
int mv_tm_scheme_port_to_source(int port_id, int *source_id)
{
	TM_WRAPPER_BEGIN(qmtm_hndl, ctl, henv);

	if (port_id >= MV_TM_MAX_PORTS) {
		rc = -1;
		goto out;
	}

	rc = port_type[port_id];

	if (source_id)
		*source_id = port_id - type_base[rc];
out:
	if (rc < 0)
		pr_info("mv_tm_scheme_port_to_source error: %d\n", rc);
	TM_WRAPPER_END(qmtm_hndl);
}
EXPORT_SYMBOL(mv_tm_scheme_port_to_source);

/* Return total number of ports. */
int mv_tm_scheme_ports_num(void)
{
	TM_WRAPPER_BEGIN(qmtm_hndl, ctl, henv);

	rc = MV_TM_MAX_PORTS;

	TM_WRAPPER_END(qmtm_hndl);
}
EXPORT_SYMBOL(mv_tm_scheme_ports_num);

/* Return base and number of A nodes that attached to port number port_id.
   Return -1 if port_id is invalid or not in use. */
int mv_tm_scheme_port_a_node_get(int port_id, int *base, int *num)
{
	int i, j;
	int sum = 0;
	uint8_t status;

	TM_WRAPPER_BEGIN(qmtm_hndl, ctl, henv);

	/* check that port is legal and in use */
	rc = rm_node_status(ctl->rm, P_LEVEL, port_id, &status);
	if ((rc) || (status != RM_TRUE)) {
		rc = -1;
		goto out;
	}

	for (i = ctl->tm_port_array[port_id].first_child_c_node;
		i <= ctl->tm_port_array[port_id].last_child_c_node; i++) {

		rc = rm_node_status(ctl->rm, C_LEVEL, i, &status);
		if ((rc) || (status != RM_TRUE)) {
			rc = -1;
			goto out;
		}

		for (j = ctl->tm_c_node_array[i].first_child_b_node;
			j <= ctl->tm_c_node_array[i].last_child_b_node; j++) {
				rc = rm_node_status(ctl->rm, B_LEVEL, j, &status);
				if ((rc) || (status != RM_TRUE)) {
					rc = -1;
					goto out;
				}

			/* static configuration*/
			sum += (ctl->tm_b_node_array[j].last_child_a_node -
				ctl->tm_b_node_array[j].first_child_a_node + 1);
		}
	}
	*num = sum;

	i = ctl->tm_port_array[port_id].first_child_c_node;
	j = ctl->tm_c_node_array[i].first_child_b_node;
	*base = ctl->tm_b_node_array[j].first_child_a_node;
out:
	if (rc < 0)
		pr_info("mv_tm_scheme_port_a_node_get error: %d\n", rc);
	TM_WRAPPER_END(qmtm_hndl);
}
EXPORT_SYMBOL(mv_tm_scheme_port_a_node_get);

/* Return base and number of B nodes that attached to port number port_id.
   Return -1 if port_id is invalid or not in use. */
int mv_tm_scheme_port_b_nodes_get(int port_id, int *base, int *num)
{
	int i;
	int sum = 0;
	uint8_t status;

	TM_WRAPPER_BEGIN(qmtm_hndl, ctl, henv);

	/* check that port is legal and in use */
	rc = rm_node_status(ctl->rm, P_LEVEL, port_id, &status);
	if ((rc) || (status != RM_TRUE)) {
		rc = -1;
		goto out;
	}

	for (i = ctl->tm_port_array[port_id].first_child_c_node;
		i <= ctl->tm_port_array[port_id].last_child_c_node; i++) {

		rc = rm_node_status(ctl->rm, C_LEVEL, i, &status);
		if ((rc) || (status != RM_TRUE)) {
			rc = -1;
			goto out;
		}
		/* static configuration*/
		sum += (ctl->tm_c_node_array[i].last_child_b_node -
			ctl->tm_c_node_array[i].first_child_b_node + 1);
	}
	*num = sum;

	i = ctl->tm_port_array[port_id].first_child_c_node;
	*base = ctl->tm_c_node_array[i].first_child_b_node;
out:
	if (rc < 0)
		pr_info("mv_tm_scheme_port_b_nodes_get error: %d\n", rc);
	TM_WRAPPER_END(qmtm_hndl);
}
EXPORT_SYMBOL(mv_tm_scheme_port_b_nodes_get);

/* Return base and number of C nodes that attached to port number port_id.
   Return -1 if port_id is invalid or not in use. */
int mv_tm_scheme_port_c_nodes_get(int port_id, int *base, int *num)
{
	uint8_t status;

	TM_WRAPPER_BEGIN(qmtm_hndl, ctl, henv);

	/* check that port is legal and in use */
	rc = rm_node_status(ctl->rm, P_LEVEL, port_id, &status);
	if ((rc) || (status != RM_TRUE)) {
		rc = -1;
		goto out;
	}
	if (base)
		*base = ctl->tm_port_array[port_id].first_child_c_node;
	if (num)
		*num = ctl->tm_port_array[port_id].last_child_c_node -
			ctl->tm_port_array[port_id].first_child_c_node + 1;
out:
	if (rc < 0)
		pr_info("mv_tm_scheme_port_c_nodes_get error: %d\n", rc);
	TM_WRAPPER_END(qmtm_hndl);
}
EXPORT_SYMBOL(mv_tm_scheme_port_c_nodes_get);


/* Return base and number of B nodes that attached to C node number cnode_id.
   Return -1 if cnode_id is invalid or not in use. */
int mv_tm_scheme_c_node_b_nodes_get(int cnode_id, int *base, int *num)
{
	uint8_t status;

	TM_WRAPPER_BEGIN(qmtm_hndl, ctl, henv);

	/* check that port is legal and in use */
	rc = rm_node_status(ctl->rm, C_LEVEL, cnode_id, &status);
	if ((rc) || (status != RM_TRUE)) {
		rc = -1;
		goto out;
	}
	if (base)
		*base = ctl->tm_c_node_array[cnode_id].first_child_b_node;
	if (num)
		*num = ctl->tm_c_node_array[cnode_id].last_child_b_node -
			ctl->tm_c_node_array[cnode_id].first_child_b_node + 1;
out:
	if (rc < 0)
		pr_info("mv_tm_scheme_c_node_b_nodes_get error: %d\n", rc);
	TM_WRAPPER_END(qmtm_hndl);
}
EXPORT_SYMBOL(mv_tm_scheme_c_node_b_nodes_get);



/* Return base and number of queues that attached to A node number anode_id.
   Return -1 if anode_id is invalid or not in use. */
int mv_tm_scheme_b_node_a_nodes_get(int bnode_id, int *base, int *num)
{
	uint8_t status;

	TM_WRAPPER_BEGIN(qmtm_hndl, ctl, henv);

	rc = rm_node_status(ctl->rm, B_LEVEL, bnode_id, &status);
	if ((rc) || (status != RM_TRUE)) {
		rc = -1;
		goto out;
	}

	if (base)
		*base = ctl->tm_b_node_array[bnode_id].first_child_a_node;
	if (num)
		*num = ctl->tm_b_node_array[bnode_id].last_child_a_node -
			ctl->tm_b_node_array[bnode_id].first_child_a_node + 1;
out:
	if (rc < 0)
		pr_info("mv_tm_scheme_b_node_a_nodes_get error: %d\n", rc);
	TM_WRAPPER_END(qmtm_hndl);
}
EXPORT_SYMBOL(mv_tm_scheme_b_node_a_nodes_get);

/* Return base and number of queues that attached to A node number anode_id.
   Return -1 if anode_id is invalid or not in use. */
int mv_tm_scheme_a_node_queues_get(int anode_id, int *base, int *num)
{
	uint8_t status;

	TM_WRAPPER_BEGIN(qmtm_hndl, ctl, henv);

	rc = rm_node_status(ctl->rm, A_LEVEL, anode_id, &status);
	if ((rc) || (status != RM_TRUE)) {
		rc = -1;
		goto out;
	}

	if (base)
		*base = ctl->tm_a_node_array[anode_id].first_child_queue;
	if (num)
		*num = ctl->tm_a_node_array[anode_id].last_child_queue -
			ctl->tm_a_node_array[anode_id].first_child_queue + 1;
out:
	if (rc < 0)
		pr_info("mv_tm_scheme_a_node_queues_get error: %d\n", rc);
	TM_WRAPPER_END(qmtm_hndl);
}
EXPORT_SYMBOL(mv_tm_scheme_a_node_queues_get);


/* Retrun A node, B node, C node and port number that attached to queue number q_id.
   Return -1 if q_id is invalid or not in use. */
int mv_tm_scheme_queue_path_get(int q_id, int *anode, int *bnode, int *cnode, int *port)
{
	uint8_t status;

	int a_node, b_node, c_node, p;

	TM_WRAPPER_BEGIN(qmtm_hndl, ctl, henv);

	rc = rm_node_status(ctl->rm, Q_LEVEL, q_id, &status);
	if ((rc) || (status != RM_TRUE)) {
		rc = -1;
		goto out;
	}

	a_node = ctl->tm_queue_array[q_id].parent_a_node;
	b_node = ctl->tm_a_node_array[a_node].parent_b_node;
	c_node = ctl->tm_b_node_array[b_node].parent_c_node;
	p = ctl->tm_c_node_array[c_node].parent_port;

	if (anode)
		*anode = a_node;
	if (bnode)
		*bnode = b_node;
	if (cnode)
		*cnode = c_node;
	if (port)
		*port = p;
out:
	if (rc < 0)
		pr_info("mv_tm_scheme_queue_path_get error: %d\n", rc);

	TM_WRAPPER_END(qmtm_hndl);
}
EXPORT_SYMBOL(mv_tm_scheme_queue_path_get);


int mv_tm_scheme_sub_nodes_get(enum tm_level level, int node_id, int *base, int *num)
{
	TM_WRAPPER_BEGIN(qmtm_hndl, ctl, henv);

	if (!base || !num) {
		rc = -1;
		goto out;
	}

	switch (level) {
	case A_LEVEL:		/**< A-nodes Level */
		rc = mv_tm_scheme_a_node_queues_get(node_id, base, num);
		break;
	case B_LEVEL:		/**< B-nodes Level */
		rc = mv_tm_scheme_b_node_a_nodes_get(node_id, base, num);
		break;
	case C_LEVEL:		/**< C-nodes Level */
		rc = mv_tm_scheme_c_node_b_nodes_get(node_id, base, num);
		break;
	case P_LEVEL:		/**< Ports Level */
		rc = mv_tm_scheme_port_c_nodes_get(node_id, base, num);
		break;
	default:
		rc = -1;
		break;
	}
out:
	if (rc < 0)
		pr_info("%s error: %d\n", __func__, rc);
	TM_WRAPPER_END(qmtm_hndl);
}
EXPORT_SYMBOL(mv_tm_scheme_sub_nodes_get);


int mv_tm_scheme_parent_node_get(enum tm_level level, int node_id, int *parent)
{
	uint8_t status;

	TM_WRAPPER_BEGIN(qmtm_hndl, ctl, henv);

	/* check that node_id is legal */
	rc = rm_node_status(ctl->rm, level, node_id, &status);
	if ((rc) || (status != RM_TRUE)) {
		pr_err("%s: Failed - TM level #%d, Node #%d is invalid\n", __func__, level, node_id);
		rc = -1;
		goto out;
	}
	if (!parent) {
		rc = -1;
		pr_err("%s: Failed - parent is NULL\n", __func__);
		goto out;
	}

	switch (level) {
	case Q_LEVEL:		/**< Q-nodes Level */
		*parent = ctl->tm_queue_array[node_id].parent_a_node;
		break;
	case A_LEVEL:		/**< A-nodes Level */
		*parent = ctl->tm_a_node_array[node_id].parent_b_node;
		break;
	case B_LEVEL:		/**< B-nodes Level */
		*parent = ctl->tm_b_node_array[node_id].parent_c_node;
		break;
	case C_LEVEL:		/**< C-nodes Level */
		*parent = ctl->tm_c_node_array[node_id].parent_port;
		break;
	case P_LEVEL:		/**< Ports Level */
		pr_info("No parents for port level\n");
		rc = -1;
		break;
	default:
		pr_err("%s: Failed - Unexpected TM level %d\n", __func__, level);
		rc = -1;
		break;
	}
out:
	TM_WRAPPER_END(qmtm_hndl);
}
EXPORT_SYMBOL(mv_tm_scheme_parent_node_get);

