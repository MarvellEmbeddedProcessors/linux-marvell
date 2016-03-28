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

#ifndef __mv_pp3_config_h__
#define __mv_pp3_config_h__

#include "common/mv_sw_if.h"
#include "mv_pp3.h"

#define MV_PP3_MSG_BUFF_SIZE	4096

#define MV_PP3_HMAC_TO_PPC_QUEUE_BASE	(44)
#define MV_PP3_HMAC_MSG_FRAME		(3)

#define MV_PP3_SW_IRQ_OFF(_frame_, _group_)		(((_frame_) * MV_A390_GIC_INT_GROUPS_NUM) + (_group_))
#define MV_PP3_SW_IRQ_2_HFRAME(_irq_)			((_irq_) / MV_A390_GIC_INT_GROUPS_NUM)
#define MV_PP3_SW_IRQ_2_GROUP(_irq_)			((_irq_) % MV_A390_GIC_INT_GROUPS_NUM)

/* Maximum packet size for TX checksum offload - 10 KBytes */
#define MV_PP3_TX_CSUM_MAX_SIZE		(10 * 1024)

/* HW RX queue allocation mode: */
/* - 1 SW -> 4 HW queues        */
/* - 1 SW -> 1 HW queue         */
enum mv_hwq_alloc_mode {

	MV_1SWQ_4HWQ,
	MV_1SWQ_1HWQ
};

/* list of system clients */
enum mv_pp3_cfg_clients {

	PP3_PPC0_DP,
	PP3_EMAC0,
	PP3_EMAC1,
	PP3_EMAC2,
	PP3_EMAC3,
	PP3_CMAC_IN,
	PP3_CMAC_LA,
	PP3_HMAC_RX,

	PP3_CLIENTS_NUM
};

struct mv_pp3_tm_node {
	int level;
	bool busy;
	int node_id;
	int first_ch;
	int num_of_ch;
	int first_q;
	int num_of_q;
	int first_free_q;
	struct mv_pp3_tm_node *sub_nodes;
};

/* Hold QM/TM queues allocation */
struct mv_pp3_hwq_cfg {
	int qm_port_id;
	struct mv_pp3_tm_node port_nodes;
};

void mv_pp3_configurator_init(struct mv_pp3 *priv);
int mv_pp3_cfg_dp_reserve_rxq(int id, int if_num, int cpu, int q_num);
int mv_pp3_cfg_dp_reserve_txq(int id, int if_num, int cpu, int q_num);
void mv_pp3_configurator_close(void);
void mv_pp3_cfg_rx_irq_base_init(int irq_base);
int mv_pp3_cfg_hwq_info_set(enum mv_pp3_cfg_clients cl, struct mv_pp3_hwq_cfg *cfg);
int mv_pp3_cfg_hmac_rx_anode_get(int hwq_base, int *anode_num);
int mv_pp3_cfg_hmac_tx_anode_get(int hwq_base, int *anode_num);
int mv_pp3_cfg_emac_anode_get(int emac, int hwq_base, int *anode_num);
int mv_pp3_cfg_hmac_pnode_get(enum mv_pp3_cfg_clients client);
int mv_pp3_cfg_rx_bp_node_get(int hwq_base, int *node_type, int *node_num);
void mv_pp3_rx_hwq_alloc_mode_change(enum mv_hwq_alloc_mode mode);

/* get ingress virtual queues mapping array
Outputs:
	ingress_vq_map	- Array of virtual ingress queues priority
	size		- Array size
*/
enum mv_priority_type *mv_pp3_nic_cfg_ingress_vq_map_get(int *size);

/* get messenger bm queue parameters (frame, queue, size)
Outputs:
	frame	- HMAC frame number
	queue	- HMAC queue number
	size	- max number of CFH messages in HMAC queue
	group	- queue interrupt group
	irq_off	- IRQ offset to connect queue ISR
*/
int mv_pp3_cfg_msg_bmq_params_get(int *frame, int *queue, int *size, int *group, int *irq_off);

/* get channel HMAC SW parameters (free frame & queue & interrupt group)
Inputs:
	chan_num - channel ID
Outputs:
	frame	- HMAC frame number
	queue	- HMAC queue number
	size	- max number of CFH messages in HMAC queue
	group	- queue interrupt group
	irq_off	- IRQ offset to connect queue ISR
*/
int mv_pp3_cfg_chan_sw_params_get(int chan_num, int *frame, int *queue, int *group, int *irq_off);

/* get channel QM HW q number, messenger BM pool ID
Inputs:
	chan_num - channel ID
Outputs:
	hwq_rx	- RX QM queue number
	hwq_tx	- TX QM queue number
	pool_id	- BM pool ID
	b_hr	- buffer header
*/
int mv_pp3_cfg_chan_hw_params_get(int chan_num, unsigned short *hwq_rx, unsigned char *hwq_tx);

/* get data path frames bitmap per cpu */
int mv_pp3_cfg_dp_cpu_frames(int cpu, int *frame);

/* get free buffers pool */
int mv_pp3_cfg_dp_gen_pool_id(int *pool_id);

/* get free internal back pressure group */
int mv_pp3_cfg_dp_gen_bp_group(int *group_id);

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
int mv_pp3_cfg_dp_bmq_params_get(int cpu, int *frame, int *queue, int *group);

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

int mv_pp3_cfg_dp_rxq_params_get(int cpu, int *frame, int *queue, int *hwq_base, int *hwq_num);


/* get free IRQ group number for RX proces
Inputs:
	frame   - HMAC frame number
Outputs:
	group     - group number
*/

int mv_pp3_cfg_dp_gen_irq_group(int frame, int cpu, int *group);
/* get IRQ number for RX proces
Inputs:
	frame   - HMAC frame number
	irq_group     - group number
Outputs:
	Irq number
*/
int mv_pp3_cfg_rx_irq_get(int frame, int irq_group);

/* get cpu assigned to <frame, group> for RX process

Inputs:
	frame   - HMAC frame number
	group   - HMAC group number
Outputs:
	cpu     - cpu ID
*/
int mv_pp3_cfg_group2cpu_get(int frame, int group);

/* get data path TX queue parameters for NIC mode
Inputs:
	cpu	- CPU number
Outputs:
	frame	- HMAC frame number
	queue	- free HMAC RX queue number
	hwq	-  HMAC->PPC QM f queue number
	emacs	- QM first queue number used by EMAC
*/
int mv_pp3_cfg_dp_txq_params_get(int vp, int cpu, int *frame, int *queue, int *hwq);

/* get data path queue HW parameters
Inputs:
	vp	- vport number
Outputs:
	qmq	- QM first queue number used by EMAC
	qmq_num - number of hw queue for PPC->EMAC traffic
*/
int mv_pp3_cfg_dp_emac_params_get(int emac, int *qmq);

/* get EMAC deq port num and enq queue num
Inputs:
	emac	- EMAC number
Outputs:
	qmp	- EMAC deq port num
	qmq	- EMAC enq queue num
*/
int mv_pp3_cfg_emac_qm_params_get(int emac, int *qmp, int *qmq);


/* get total data path TX queues

Outputs:
	hw_txq	- first QM (HMAC to PPC) TX queue number
	hwq_num	- number of (HMAC to PPC) TX queues
*/

int mv_pp3_cfg_dp_hw_txq_get(int *hw_txq, int *hwq_num);

#endif /* __mv_pp3_config_h__ */
