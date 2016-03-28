/*
* ***************************************************************************
* Copyright (C) 2015 Marvell International Ltd.
* ***************************************************************************
* This program is free software: you can redistribute it and/or modify it
* under the terms of the GNU General Public License as published by the Free
* Software Foundation, either version 2 of the License, or any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program.  If not, see <http://www.gnu.org/licenses/>.
* ***************************************************************************
*/

#include "mv_pp3.h"
#include "mv_pp3_config.h"
#include "bm/mv_bm.h"
#include "hmac/mv_hmac.h"
#include "emac/mv_emac.h"
#include <net/gnss/mv_nss_defs.h>
#include "tm_defs.h"
#include "mv_tm_scheme.h"
#include "a390_gic_odmi_if.h"

/* store resource allocation per interface */
struct mv_pp3_net_if_cfg {
	bool   first_init_done;
	u32    num_of_sw_q[MV_PP3_CPU_NUM];		/* reserved for this interface on one cpu */
	struct mv_pp3_tm_node *anode[MV_PP3_HFRM_Q_NUM];/* A node reserved for sw queue */
	struct mv_pp3_tm_node *tx_bnode;
	struct mv_pp3_tm_node *rx_anode;
	int if_frame;					/* HMAC frame reserved for this inteface */
	int if_first_sw_q;				/* HMAC first queue reserved for interace */
};

struct mv_pp3_cpu_cfg {
	u32 sw_free_rxq[MV_PP3_HFRM_NUM];	/* free HMAC RX SW queue number per frame */
	u32 sw_free_txq[MV_PP3_HFRM_NUM];	/* free HMAC TX SW queue number per frame */
	u32 free_irq_group[MV_PP3_HFRM_NUM];	/* free HMAC IRQ group number per frame */
	u32 buffer_pool_id;			/* free GP pool number */
	u32 bp_group_id;			/* internal back pressure group */
	u32 irq_rx_base;			/* ODMI Interrupt Vector base number */
	int frame_vip_vport[MV_PP3_HFRM_NUM];
	bool vip_rx_if[MV_PP3_HFRM_NUM][MV_PP3_CPU_NUM];
	bool vip_tx_if[MV_PP3_HFRM_NUM][MV_PP3_CPU_NUM];
};

static struct mv_pp3_hwq_cfg    mv_pp3_cfg_clients_info[PP3_CLIENTS_NUM];
static struct mv_pp3_net_if_cfg mv_pp3_if_rxq_resources[MV_PP3_INTERNAL_CPU_PORT_NUM];
static struct mv_pp3_net_if_cfg mv_pp3_if_txq_resources[MV_PP3_INTERNAL_CPU_PORT_NUM];
static struct mv_pp3_cpu_cfg    mv_pp3_cfg_sw_info;

static void mv_pp3_port_subtree_build(struct mv_pp3_tm_node *level_ptr, enum tm_level level, int node_id);
static void mv_pp3_cfg_subtree_free(struct mv_pp3_tm_node *node);
#ifdef PP3_DEBUG
static void mv_pp3_port_subtree_print(struct mv_pp3_tm_node *node);
#endif



int mv_pp3_cfg_rx_irq_get(int id, int irq_group)
{
	int frame = mv_pp3_if_rxq_resources[id].if_frame;

	return mv_pp3_cfg_sw_info.irq_rx_base + MV_PP3_SW_IRQ_OFF(frame, irq_group);
}

void mv_pp3_configurator_init(struct mv_pp3 *priv)
{
	int i;
	int qm_ports[PP3_CLIENTS_NUM] = {TM_A0_PORT_PPC0_0, TM_A0_PORT_EMAC0, TM_A0_PORT_EMAC1,
					 TM_A0_PORT_EMAC2, TM_A0_PORT_EMAC3, TM_A0_PORT_CMAC_IN,
					 TM_A0_PORT_CMAC_LA, TM_A0_PORT_HMAC};

	memset(mv_pp3_cfg_clients_info, 0, sizeof(mv_pp3_cfg_clients_info));
	memset(&mv_pp3_cfg_sw_info, 0, sizeof(mv_pp3_cfg_sw_info));
	memset(mv_pp3_if_rxq_resources, 0, sizeof(mv_pp3_if_rxq_resources));
	memset(mv_pp3_if_txq_resources, 0, sizeof(mv_pp3_if_txq_resources));

	mv_pp3_cfg_sw_info.irq_rx_base = priv->irq_base;
	/* first GP pool number */
	mv_pp3_cfg_sw_info.buffer_pool_id = BM_GP_POOL_MIN;
	/* first internal BP group */
	mv_pp3_cfg_sw_info.bp_group_id = 1;

	for (i = 0; i < MV_PP3_HFRM_NUM; i++)
		mv_pp3_cfg_sw_info.frame_vip_vport[i] = -1;

	/* init configurator clients */
	for (i = 0; i < PP3_CLIENTS_NUM; i++) {
		mv_pp3_cfg_clients_info[i].qm_port_id = qm_ports[i];
		mv_pp3_port_subtree_build(&mv_pp3_cfg_clients_info[i].port_nodes, C_LEVEL, qm_ports[i]);
		mv_pp3_cfg_hwq_info_set(i, &mv_pp3_cfg_clients_info[i]);
	}

#ifdef PP3_DEBUG
	/* print clients info */
	for (i = 0; i < PP3_CLIENTS_NUM; i++) {
		struct mv_pp3_tm_node *tmp = &mv_pp3_cfg_clients_info[i].port_nodes;
		pr_info("\nPort %d:", mv_pp3_cfg_clients_info[i].qm_port_id);
		pr_info("\t\tnode %d: first = %d, first_free = %d (%d), subnodes number = %d, queues = %d\n",
		tmp->node_id, tmp->first_ch, tmp->first_free_q, tmp->first_q, tmp->num_of_ch, tmp->num_of_q);
		mv_pp3_port_subtree_print(&mv_pp3_cfg_clients_info[i].port_nodes);
	}
#endif
}

void mv_pp3_configurator_close(void)
{
	int i;

	/* free configurator clients tree */
	for (i = 0; i < PP3_CLIENTS_NUM; i++) {
		if (!mv_pp3_cfg_clients_info[i].port_nodes.sub_nodes)
			continue;

		mv_pp3_cfg_subtree_free(&mv_pp3_cfg_clients_info[i].port_nodes);
	}
}

int mv_pp3_cfg_hwq_info_set(enum mv_pp3_cfg_clients cl, struct mv_pp3_hwq_cfg *cfg)
{
	struct mv_pp3_tm_node *cnode, *bnode, *anode;
	int i, j;

	if (cl >= PP3_CLIENTS_NUM)
		return -1;

	switch (cl) {
	case PP3_PPC0_DP:
		/* configure TX tree for better usage */
		cnode = &cfg->port_nodes;
		for (i = 0; i < cnode->num_of_ch; i++) {
			bnode = &cnode->sub_nodes[i];
			/* set on B level number of first free queue */
			bnode->first_q = bnode->sub_nodes[0].first_ch;
			bnode->first_free_q = bnode->first_q;
			for (j = 0; j < bnode->num_of_ch; j++) {
				anode = &bnode->sub_nodes[j];
				/* set on A level number of first free queue */
				anode->first_q = anode->first_ch;
				anode->first_free_q = anode->first_q;
			}
			if (i == 0) {
				cnode->first_q = bnode->first_q;
				cnode->first_free_q = bnode->first_free_q;
			}
		}
	break;
	case PP3_HMAC_RX:
		cnode = &cfg->port_nodes;
		for (i = 0; i < cnode->num_of_ch; i++) {
			bnode = &cnode->sub_nodes[i];
			bnode->first_q = bnode->sub_nodes[0].first_ch;
			bnode->first_free_q = bnode->first_q;
			for (j = 0; j < bnode->num_of_ch; j++) {
				anode = &bnode->sub_nodes[j];
				/* set on A level number of first free queue */
				anode->first_q = anode->first_ch;
				anode->first_free_q = anode->first_q;
			}
			if (i == 0) {
				cnode->first_q = bnode->first_q;
				cnode->first_free_q = bnode->first_free_q;
			}
		}
	break;
	case PP3_EMAC0:
	case PP3_EMAC1:
	case PP3_EMAC2:
	case PP3_EMAC3:
		cnode = &cfg->port_nodes;
		if (cnode->num_of_ch) {
			bnode = &cnode->sub_nodes[0];
			cnode->first_q = bnode->sub_nodes[0].first_ch;
			cnode->first_free_q = cnode->first_q;
		}
		break;
	default:
		return -1;
	}

	return 0;
}

int mv_pp3_cfg_rx_bp_node_get(int hwq_base, int *node_type, int *node_num)
{
	if ((node_num == NULL) || (node_type == NULL))
		return -1;

	/* each SWQ is connected to 1 HW queue */
	*node_num = hwq_base;
	*node_type = MV_QM_Q_NODE;

	return 0;
}

static int pp3_cfg_anode_get(struct mv_pp3_tm_node *cnode, int hwq_base, int *anode_num)
{
	struct mv_pp3_tm_node *bnode, *anode;
	int i, j, k;

	for (i = 0; i < cnode->num_of_ch; i++) {
		bnode = &cnode->sub_nodes[i];
		for (j = 0; j < bnode->num_of_ch; j++) {
			anode = &bnode->sub_nodes[j];
			for (k = 0; k < anode->num_of_ch; k++)
				if ((anode->first_ch <= hwq_base) &&
				   ((anode->first_ch + anode->num_of_ch) > hwq_base)) {
					*anode_num = anode->node_id;
					return 0;
				}
		}
	}
	return -1;
}

int mv_pp3_cfg_hmac_rx_anode_get(int hwq_base, int *anode_num)
{
	if ((anode_num == NULL) || (!mv_pp3_cfg_clients_info[PP3_HMAC_RX].port_nodes.sub_nodes))
		return -1;

	return pp3_cfg_anode_get(&mv_pp3_cfg_clients_info[PP3_HMAC_RX].port_nodes, hwq_base, anode_num);
}

int mv_pp3_cfg_hmac_tx_anode_get(int hwq_base, int *anode_num)
{
	if ((anode_num == NULL) || (!mv_pp3_cfg_clients_info[PP3_PPC0_DP].port_nodes.sub_nodes))
		return -1;

	return pp3_cfg_anode_get(&mv_pp3_cfg_clients_info[PP3_PPC0_DP].port_nodes, hwq_base, anode_num);
}

int mv_pp3_cfg_emac_anode_get(int emac, int hwq_base, int *anode_num)
{
	if ((anode_num == NULL) || (!mv_pp3_cfg_clients_info[PP3_EMAC0 + emac].port_nodes.sub_nodes))
		return -1;

	return pp3_cfg_anode_get(&mv_pp3_cfg_clients_info[PP3_EMAC0 + emac].port_nodes, hwq_base, anode_num);
}

/* get channel HMAC SW parameters (free frame & queue & interrupt group)
Inputs:
	chan_num - channel ID
Outputs:
	frame	- HMAC frame number
	queue	- HMAC queue number
	group	- HMAC queue interrupt group
	irq_off	- IRQ offset to connect queue ISR
*/
int mv_pp3_cfg_chan_sw_params_get(int chan_num, int *frame, int *queue, int *group, int *irq_off)
{
	if ((frame == NULL) || (queue == NULL) || (group == NULL) || (irq_off == NULL))
		return -1;

	if (((mv_pp3_cfg_sw_info.sw_free_rxq[MV_PP3_HMAC_MSG_FRAME] + 1) > MV_PP3_HFRM_Q_NUM) ||
		(mv_pp3_cfg_sw_info.free_irq_group[MV_PP3_HMAC_MSG_FRAME] >= MV_A390_GIC_INT_GROUPS_NUM))
		return -1;

	*frame = MV_PP3_HMAC_MSG_FRAME;
	*queue = mv_pp3_cfg_sw_info.sw_free_rxq[MV_PP3_HMAC_MSG_FRAME]++;
	mv_pp3_cfg_sw_info.sw_free_txq[MV_PP3_HMAC_MSG_FRAME]++;

	*group = mv_pp3_cfg_sw_info.free_irq_group[MV_PP3_HMAC_MSG_FRAME]++;
	*irq_off = mv_pp3_cfg_sw_info.irq_rx_base + *frame * MV_A390_GIC_INT_GROUPS_NUM + *group;

	return 0;
}

/* get channel QM HW q number, messenger BM pool ID
Inputs:
	chan_num - channel ID
Outputs:
	hwq_rx	- RX QM queue number
	hwq_tx	- TX QM queue number
*/
int mv_pp3_cfg_chan_hw_params_get(int chan_num, unsigned short *hwq_rx, unsigned char *hwq_tx)
{
	struct mv_pp3_tm_node *cnode, *bnode, *anode;
	bool found = false;
	int i, j;

	if ((hwq_rx == NULL) || (hwq_tx == NULL))
		return -1;

	/* on Rx: 4 queues connected to last A node are used for messenger */
	cnode = &mv_pp3_cfg_clients_info[PP3_HMAC_RX].port_nodes;
	bnode = &cnode->sub_nodes[cnode->num_of_ch - 1];		/* last B node */
	for (i = 1; (i <= 4) && !found; i++) {
		anode = &bnode->sub_nodes[bnode->num_of_ch - i];
		if (((anode->first_free_q - anode->first_q) + 1) <= anode->num_of_q) {
			found = true;
			*hwq_rx = anode->first_free_q++;
			anode->busy = true;
		}
	}
	if (!found)
		return -1;

	/* on TX: queues start from MV_PP3_HMAC_TO_PPC_QUEUE_BASE are used for messenger */
	/* mark coresponded B node as busy */
	found = false;
	cnode = &mv_pp3_cfg_clients_info[PP3_PPC0_DP].port_nodes;
	for (i = 0; (i < cnode->num_of_ch) && (!found); i++) {
		bnode = &cnode->sub_nodes[i];
		if ((bnode->first_q + bnode->num_of_q) >= MV_PP3_HMAC_TO_PPC_QUEUE_BASE) {
			for (j = 0; j < bnode->num_of_ch; j++) {
				anode = &bnode->sub_nodes[j];
				if ((anode->first_q >= MV_PP3_HMAC_TO_PPC_QUEUE_BASE) &&
				   (((anode->first_free_q - anode->first_q) + 1) <= anode->num_of_q)) {
					*hwq_tx = anode->first_free_q++;
					found = true;
					anode->busy = true;
				}
			}
		}
	}
	if (!found)
		return -1;

	return 0;
}

/* get free internal back pressure group id */
int mv_pp3_cfg_dp_gen_bp_group(int *group_id)
{
	if (group_id == NULL)
		return -1;

	if (mv_pp3_cfg_sw_info.bp_group_id >= MV_PP3_QM_BP_RULES_NUM)
		return -1;

	*group_id = mv_pp3_cfg_sw_info.bp_group_id++;
	return 0;
}

/* get free buffers pool */
int mv_pp3_cfg_dp_gen_pool_id(int *pool_id)
{
	if (pool_id == NULL)
		return -1;

	if (mv_pp3_cfg_sw_info.buffer_pool_id >= BM_GP_POOL_MAX)
		return -1;

	*pool_id = mv_pp3_cfg_sw_info.buffer_pool_id++;
	return 0;
}


/* get frame and queue number in order to manage bm pools per cpu
Inputs:
	cpu	- CPU number
Outputs:
	frame	- HMAC frame number
	queue	- HMAC queue number
	size	- max number of CFH messages in HMAC queue
	group	- queue interrupt group
	irq_off	- IRQ offset to connect queue ISR
*/
int mv_pp3_cfg_dp_bmq_params_get(int cpu, int *frame, int *queue, int *group)
{

	if (((mv_pp3_cfg_sw_info.sw_free_rxq[MV_PP3_HMAC_MSG_FRAME] + 1) > MV_PP3_HFRM_Q_NUM) ||
	   ((mv_pp3_cfg_sw_info.free_irq_group[MV_PP3_HMAC_MSG_FRAME] + 1) > MV_A390_GIC_INT_GROUPS_NUM))
		return -1;

	if (frame)
		*frame = MV_PP3_HMAC_MSG_FRAME;
	if (queue) {
		*queue = mv_pp3_cfg_sw_info.sw_free_rxq[MV_PP3_HMAC_MSG_FRAME]++;
		mv_pp3_cfg_sw_info.sw_free_txq[MV_PP3_HMAC_MSG_FRAME]++;
	}
	if (group)
		*group = mv_pp3_cfg_sw_info.free_irq_group[MV_PP3_HMAC_MSG_FRAME]++;

	return 0;
}

static int mv_pp3_cfg_hw_rxq_single_hwq_get(int if_num, int *hw_rxq)
{
	struct mv_pp3_tm_node *node;

	node = mv_pp3_if_rxq_resources[if_num].rx_anode;
	if (node->num_of_q > (node->first_free_q - node->first_q)) {
		*hw_rxq = node->first_free_q;
		node->first_free_q++;
		return 0;
	}

	return -1;
}

/* get data path TX queue parameters

Connect one SW queue to the one A node.
If SW queues duplicated per cpu, the same SW queues connected to the same A node.

Inputs:
	if_num	- emac number
	cpu	- CPU number
Outputs:
	hw_txq	- QM TX queue number
*/
static int mv_pp3_cfg_hw_txq_params_get(int id, int cpu, int *hw_txq)
{
	struct mv_pp3_tm_node *bnode, *anode;
	u32 curr_q_num;
	int i;
	int if_num = id - cpu;

	bnode = mv_pp3_if_txq_resources[if_num].tx_bnode;

	if (mv_pp3_if_txq_resources[if_num].first_init_done == false) {
		/* take first free anode */
		for (i = 0; i < bnode->num_of_ch; i++) {
			anode = &bnode->sub_nodes[i];
			if (anode->busy)
				continue;

			anode->busy = true;
			*hw_txq = anode->first_free_q++;
			mv_pp3_if_txq_resources[if_num].anode[0] = anode;
			mv_pp3_if_txq_resources[if_num].num_of_sw_q[cpu] = 1;
			mv_pp3_if_txq_resources[if_num].first_init_done = true;
			return 0;
		}
	}

	/* number of SW queues already reserved on interface / cpu */
	curr_q_num = mv_pp3_if_txq_resources[if_num].num_of_sw_q[cpu];
	if (curr_q_num > 0) {
		if (mv_pp3_if_txq_resources[if_num].anode[curr_q_num]) {
			anode = mv_pp3_if_txq_resources[if_num].anode[curr_q_num];
			*hw_txq = anode->first_free_q++;
		} else {
			/* next SW queue on cpu takes next free A node */
			for (i = 0; i < bnode->num_of_ch; i++) {
				anode = &bnode->sub_nodes[i];
				if (anode->busy)
					continue;

				anode->busy = true;
				*hw_txq = anode->first_free_q++;
				mv_pp3_if_txq_resources[if_num].anode[curr_q_num] = anode;
				break;
			}
		}
		mv_pp3_if_txq_resources[if_num].num_of_sw_q[cpu]++;
	} else {
		/* first SW queue on cpu, but not on interface takes A node allocated before */
		anode = mv_pp3_if_txq_resources[if_num].anode[curr_q_num];
		*hw_txq = anode->first_free_q++;
		mv_pp3_if_txq_resources[if_num].num_of_sw_q[cpu]++;
	}

	return 0;
}

/* get data path RX queue parameters for NIC mode
Inputs:
	emac	- emac number
	cpu	- CPU number
Outputs:
	frame	- HMAC frame number
	sw_rxq	- HMAC RX queue number
	hw_rxq	- QM RX queue number
	hwq_num - number of RX hw queue per one sw queue
*/
int mv_pp3_cfg_dp_rxq_params_get(int if_num, int *frame, int *sw_rxq, int *hw_rxq, int *hwq_num)
{
	if ((frame == NULL) || (sw_rxq == NULL) || (hw_rxq == NULL) || (hwq_num == NULL)) {
		pr_err("%s: Invalid input parameters\n", __func__);
		return -1;
	}

	if (!mv_pp3_if_rxq_resources[if_num].rx_anode) {
		pr_info("%s: failed for cpu_vp = %d, cpu %d\n", __func__, if_num, if_num % MV_PP3_CPU_NUM);
		return -1;
	}

	*frame = mv_pp3_if_rxq_resources[if_num].if_frame;
	*sw_rxq = mv_pp3_if_rxq_resources[if_num].if_first_sw_q++;

	*hwq_num = 1;
	mv_pp3_cfg_hw_rxq_single_hwq_get(if_num, hw_rxq);

	return 0;
}

/* get data path TX queue parameters for NIC mode
Inputs:
	emac	- emac number
	cpu	- CPU number
Outputs:
	frame	- HMAC frame number
	sw_txq	- HMAC TX queue number
	hw_txq	- QM TX queue number
*/

int mv_pp3_cfg_dp_txq_params_get(int vp, int cpu, int *frame, int *sw_txq, int *hw_txq)
{
	if ((frame == NULL) || (sw_txq == NULL) || (hw_txq == NULL))
		return -1;

	*frame = mv_pp3_if_txq_resources[vp].if_frame;
	*sw_txq = mv_pp3_if_txq_resources[vp].if_first_sw_q++;

	mv_pp3_cfg_hw_txq_params_get(vp, cpu, hw_txq);
	return 0;
}

/* get total data path TX queues base and number

Outputs:
	hw_txq	- first QM (HMAC to PPC) TX queue number
	hwq_num	- number of (HMAC to PPC) TX queues
*/
int mv_pp3_cfg_dp_hw_txq_get(int *hw_txq, int *hwq_num)
{
	if ((hw_txq == NULL) || (hwq_num == NULL))
		return -1;

	*hw_txq = MV_PP3_HMAC_TO_PPC_QUEUE_BASE;
	*hwq_num = mv_pp3_cfg_clients_info[PP3_PPC0_DP].port_nodes.num_of_q - MV_PP3_HMAC_TO_PPC_QUEUE_BASE;

	return 0;
}

/* get data path queue HW parameters
Inputs:
	emac	- emac number
Outputs:
	qmq	- QM first queue number used by EMAC
*/
int mv_pp3_cfg_dp_emac_params_get(int emac, int *qmq)
{
	if ((qmq == NULL) || (emac >= MV_PP3_EMAC_NUM))
		return -1;

	*qmq = mv_pp3_cfg_clients_info[PP3_EMAC0 + emac].port_nodes.first_free_q;
	mv_pp3_cfg_clients_info[PP3_EMAC0 + emac].port_nodes.first_free_q++;
	return 0;
}

/* get EMAC deq port num and enq queue num
Inputs:
	emac	- EMAC number
Outputs:
	qmp	- EMAC deq port num
	qmq	- EMAC enq queue num
*/

int mv_pp3_cfg_emac_qm_params_get(int emac, int *qmp, int *qmq)
{
	if (emac >= MV_PP3_EMAC_NUM)
		return -1;

	/* EMAC->PPC QM queue */
	if (qmq)
		*qmq = mv_pp3_cfg_clients_info[PP3_PPC0_DP].port_nodes.first_q + 1 + emac * 4;
	if (qmp)
		*qmp = mv_pp3_cfg_clients_info[PP3_EMAC0 + emac].qm_port_id;

	return 0;
}

/* get data path IRQ number for RX process
Inputs:
	frame	  - frame number
Outputs:
	group     - frame group number
*/
int mv_pp3_cfg_dp_gen_irq_group(int id, int cpu, int *group)
{
	if (group == NULL)
		return -1;

	if (mv_pp3_cfg_sw_info.free_irq_group[mv_pp3_if_rxq_resources[id].if_frame] >= MV_A390_GIC_INT_GROUPS_NUM) {
		pr_err("No free irq group for frame %d\n", mv_pp3_if_rxq_resources[id].if_frame);
		return -1;
	}

	/* skip two first multiple groups */
	*group = mv_pp3_cfg_sw_info.free_irq_group[mv_pp3_if_rxq_resources[id].if_frame];
	mv_pp3_cfg_sw_info.free_irq_group[mv_pp3_if_rxq_resources[id].if_frame]++;
	return 0;
}

int mv_pp3_cfg_hmac_pnode_get(enum mv_pp3_cfg_clients client)
{
	return mv_pp3_cfg_clients_info[client].qm_port_id;
}

void mv_pp3_port_subtree_build(struct mv_pp3_tm_node *level_ptr, enum tm_level level, int node_id)
{
	struct mv_pp3_tm_node *tmp_ptr;
	int i;

	mv_tm_scheme_sub_nodes_get(level, node_id, &level_ptr->first_ch, &level_ptr->num_of_ch);
	level_ptr->level = level;
	/*pr_info("%s: node %d (%p): level %d, first = %d, number = %d\n", __func__,
		node_id, level_ptr, level, level_ptr->first_ch, level_ptr->num_of_ch);*/
	if (level == A_LEVEL) {
		level_ptr->num_of_q = level_ptr->num_of_ch;
		level_ptr->first_free_q = level_ptr->first_ch;
		return;
	}

	level_ptr->sub_nodes = kzalloc(level_ptr->num_of_ch * sizeof(struct mv_pp3_tm_node), GFP_KERNEL);

	for (i = 0; i < level_ptr->num_of_ch; i++) {
		tmp_ptr = &level_ptr->sub_nodes[i];
		tmp_ptr->node_id = level_ptr->first_ch + i;
		mv_pp3_port_subtree_build(tmp_ptr, level - 1, tmp_ptr->node_id);
		level_ptr->num_of_q += tmp_ptr->num_of_q;
	}

	return;
}

#ifdef PP3_DEBUG
static void mv_pp3_port_subtree_print(struct mv_pp3_tm_node *node)
{
	struct mv_pp3_tm_node *tmp;
	int i;

	if (node->level == A_LEVEL)
		return;

	for (i = 0; i < node->num_of_ch; i++) {
		tmp = &node->sub_nodes[i];
		pr_info("\t\tnode %d: first = %d, first_free = %d (%d), subnodes number = %d, queues = %d\n",
		tmp->node_id, tmp->first_ch, tmp->first_free_q, tmp->first_q, tmp->num_of_ch, tmp->num_of_q);
		mv_pp3_port_subtree_print(tmp);
	}

	return;
}
#endif /* PP3_DEBUG */

static void mv_pp3_cfg_sort(u32 *queues, int *arr)
{
	int tmp, i, j;

	for (i = 0; i < MV_PP3_HFRM_NUM; i++)
		arr[i] = i;

	for (i = 0; i < (MV_PP3_HFRM_NUM - 1); i++) {
		for (j = 0; j < MV_PP3_HFRM_NUM - 1 - i; j++) {
			if (queues[arr[j]] > queues[arr[j+1]]) {
				tmp = arr[j+1];
				arr[j+1] = arr[j];
				arr[j] = tmp;
			}
		}
	}
}

/* reserve data path RX queues for future allocation by specified interface
 * reserve A node with queues number >= virtual queues number
Inputs:
	id	- unique request ID (interface number)
	cpu	- cpu number
	q_num	- number of queues allocated per request (interface)
Outputs:
	hw_rxq	- first QM queue number
*/
int mv_pp3_cfg_dp_reserve_rxq(int id, int if_num, int cpu, int q_num)
{
	struct mv_pp3_tm_node *cnode, *bnode, *anode;
	int i, j, anode_q_num;
	int frame = 0;
	int ind_arr[MV_PP3_HFRM_NUM];
	bool found;

	if (id >= MV_PP3_INTERNAL_CPU_PORT_MAX) {
		pr_info("%s: cannot reserved RX resources for interface %d\n", __func__, id);
		return -1;
	}
	if (mv_pp3_if_rxq_resources[id].rx_anode) {
		pr_info("%s: already reserved RX resources for interface %d\n", __func__, id);
		return -1;
	}

	/* look for free SW queues belong to one frame */
	found = false;
	for (i = 0; (i < MV_PP3_HFRM_NUM) && !found; i++) {
		if (mv_pp3_cfg_sw_info.frame_vip_vport[i] == if_num) {
			found = true;
			frame = i;
		}
	}
	if (!found) {
		/* sort all frames per free RXQs number */
		mv_pp3_cfg_sort(mv_pp3_cfg_sw_info.sw_free_rxq, ind_arr);
		/* look for free SW queues belong to one frame */
		for (i = 0; (i < MV_PP3_HFRM_NUM) && !found; i++) {
			if ((mv_pp3_cfg_sw_info.sw_free_rxq[ind_arr[i]] + q_num) > MV_PP3_HFRM_Q_NUM)
				continue;

			/* each emac interface placed in different frame */
			if ((if_num < MV_PP3_EMAC_VP_NUM) && (mv_pp3_cfg_sw_info.vip_rx_if[ind_arr[i]][cpu]))
				continue;

			if (mv_pp3_cfg_sw_info.free_irq_group[ind_arr[i]] >= MV_A390_GIC_INT_GROUPS_NUM)
				continue;

			frame = ind_arr[i];
			found = true;
		}
	}

	if (!found) {
		pr_info("%s: cannot reserved SW RX resources for interface %d\n", __func__, id);
		return -1;
	}

	mv_pp3_if_rxq_resources[id].if_frame = frame;
	mv_pp3_if_rxq_resources[id].if_first_sw_q = mv_pp3_cfg_sw_info.sw_free_rxq[frame];
	mv_pp3_cfg_sw_info.sw_free_rxq[frame] += q_num;
	mv_pp3_cfg_sw_info.frame_vip_vport[frame] = if_num;
	if (if_num < MV_PP3_EMAC_VP_NUM)
		mv_pp3_cfg_sw_info.vip_rx_if[frame][cpu] = true;

	found = false;
	/* look for free A node with queues number >= q_num */
	anode_q_num = 0xFFFF;
	cnode = &mv_pp3_cfg_clients_info[PP3_HMAC_RX].port_nodes;
	for (i = 0; i < cnode->num_of_ch; i++) {
		bnode = &cnode->sub_nodes[i];
		for (j = 0; j < bnode->num_of_ch; j++) {
			anode = &bnode->sub_nodes[j];
			if (!anode->busy) {
				/* check how many queues it has */
				if ((anode->num_of_q >= q_num) && (anode_q_num > anode->num_of_q)) {
						mv_pp3_if_rxq_resources[id].rx_anode = anode;
						anode_q_num = anode->num_of_q;
						found = true;
					}
			}
		}
	}
	/* mark A node as busy */
	cnode = &mv_pp3_cfg_clients_info[PP3_HMAC_RX].port_nodes;
	for (i = 0; (i < cnode->num_of_ch) && found; i++) {
		bnode = &cnode->sub_nodes[i];
		for (j = 0; j < bnode->num_of_ch; j++) {
			anode = &bnode->sub_nodes[j];
			if (mv_pp3_if_rxq_resources[id].rx_anode->node_id == anode->node_id) {
				anode->busy = true;
				return 0;
			}
		}
	}
	return -1;
}

/* reserve data path TX queues for future allocation by specified interface
 * reserve B node with A nodes number >= virtual queues number
 * one interface (with all internal cpu vp) must belong to one B node
Inputs:
	id	- unique request ID (interface number)
	cpu	- cpu number
	q_num	- number of queues allocated per request (interface)
Outputs:
	hw_rxq	- first QM queue number
*/
int mv_pp3_cfg_dp_reserve_txq(int id, int if_num, int cpu, int q_num)
{
	struct mv_pp3_tm_node *cnode, *bnode, *anode;
	int i, j;
	int frame = 0;
	int ind_arr[MV_PP3_HFRM_NUM];
	bool found;

	if (id >= MV_PP3_INTERNAL_CPU_PORT_MAX) {
		pr_info("%s: cannot reserved TX resources for emac %d\n", __func__, id);
		return -1;
	}

	/* look for free SW queues belong to one frame */
	found = false;
	for (i = 0; (i < MV_PP3_HFRM_NUM) && !found; i++) {
		if (mv_pp3_cfg_sw_info.frame_vip_vport[i] == if_num) {
			found = true;
			frame = i;
		}
	}
	if (!found) {
		/* sort all frames per free RXQs number */
		mv_pp3_cfg_sort(mv_pp3_cfg_sw_info.sw_free_txq, ind_arr);

		for (i = 0; (i < MV_PP3_HFRM_NUM) && !found; i++) {
			if ((mv_pp3_cfg_sw_info.sw_free_txq[ind_arr[i]] + q_num) > MV_PP3_HFRM_Q_NUM)
				continue;

			/* each emac interface placed in different frame */
			if ((if_num < MV_PP3_EMAC_VP_NUM) && (mv_pp3_cfg_sw_info.vip_tx_if[ind_arr[i]][cpu]))
				continue;

			if (mv_pp3_cfg_sw_info.free_irq_group[ind_arr[i]] >= MV_A390_GIC_INT_GROUPS_NUM)
				continue;

			frame = ind_arr[i];
			found = true;
		}
	}
	if (!found) {
		pr_info("%s: cannot reserved SW TX resources for interface %d\n", __func__, id);
		return -1;
	}

	mv_pp3_if_txq_resources[id].if_frame = frame;
	mv_pp3_if_txq_resources[id].if_first_sw_q = mv_pp3_cfg_sw_info.sw_free_txq[frame];
	mv_pp3_cfg_sw_info.sw_free_txq[frame] += q_num;
	mv_pp3_cfg_sw_info.frame_vip_vport[frame] = if_num;
	if (if_num < MV_PP3_EMAC_VP_NUM)
		mv_pp3_cfg_sw_info.vip_tx_if[frame][cpu] = true;

	/* one B node per interface */
	if (mv_pp3_if_txq_resources[id].tx_bnode)
		return 0;

	if (mv_pp3_if_txq_resources[id - cpu].tx_bnode) {
		/* use the same B node for next interface */
		bnode = mv_pp3_if_txq_resources[id - cpu].tx_bnode;
		mv_pp3_if_txq_resources[id].tx_bnode = bnode;
		return 0;
	}

	found = false;
	/* look for first B node with free A nodes number >= q_num */
	cnode = &mv_pp3_cfg_clients_info[PP3_PPC0_DP].port_nodes;
	for (i = 0; (i < cnode->num_of_ch); i++) {
		bnode = &cnode->sub_nodes[i];
		if (bnode->first_q >= MV_PP3_HMAC_TO_PPC_QUEUE_BASE + 4) {
			for (j = 0; j < bnode->num_of_ch; j++) {
				anode = &bnode->sub_nodes[j];
				if ((!anode->busy) && (anode->num_of_q >= q_num)) {
					mv_pp3_if_txq_resources[id].tx_bnode = bnode;
					bnode->busy = true;
					return 0;
				}
			}
		}
	}

	return -1;
}

static void mv_pp3_cfg_subtree_free(struct mv_pp3_tm_node *node)
{
	struct mv_pp3_tm_node *anode, *bnode;
	int i;

	for (i = 0; i < node->num_of_ch; i++) {
		bnode = &node->sub_nodes[i];
		anode = &bnode->sub_nodes[0];
		kfree(anode);
	}
	bnode = &node->sub_nodes[0];
	kfree(bnode);

	return;
}
