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
#ifndef	__mv_qm_h__
#define	__mv_qm_h__

#include "platform/mv_pp3.h"
#include "mv_qm_regs.h"

enum mv_qm_node_type {
	MV_QM_Q_NODE,
	MV_QM_A_NODE,
	MV_QM_B_NODE,
	MV_QM_C_NODE,
	MV_QM_PORT_NODE,
	mv_QM_NODE_INVALID
};

enum mv_qm_thr_profiles {
	QM_THR_PROFILE_EMAC_0 = 0,
	QM_THR_PROFILE_EMAC_1,
	QM_THR_PROFILE_EMAC_2,
	QM_THR_PROFILE_EMAC_3,
	QM_THR_PROFILE_UNUSED_1,
	QM_THR_PROFILE_UNUSED_2,
	QM_THR_PROFILE_HMAC,
	QM_THR_PROFILE_INVALID,
};

enum mv_qm_ql_bp_status {
	QM_BP_XON = 0,
	QM_BP_PAUSE,
	QM_BP_XOFF,
	QM_BP_INVALID,
};

/* QM debug flags */
#define QM_F_DBG_RD_BIT			0
#define QM_F_DBG_WR_BIT			1

#define QM_F_DBG_RD			(1 << QM_F_DBG_RD_BIT)
#define QM_F_DBG_WR			(1 << QM_F_DBG_WR_BIT)

#define QM_QUEUES_NUM			512
#define QM_BPI_GROUPS			64
#define QM_PORTS_NUM			16
#define QM_BP_INVALID_GROUP		0 /* assign all queues to group 0 in default init */

/* fifo depth - set port dequeue fifo size */
#define QM_PPC_DP_FIFO_DEPTH		(0x2 * QM_DQF_PPC_ROW_SIZE)
#define QM_PPC_MNT_FIFO_DEPTH		(0x1 * QM_DQF_PPC_ROW_SIZE)
#define QM_EMAC_10G_FIFO_DEPTH		(0x29A * QM_DQF_MAC_ROW_SIZE)
#define QM_EMAC_1G_FIFO_DEPTH		(0x359 * QM_DQF_MAC_ROW_SIZE)
#define QM_CMAC_IN_FIFO_DEPTH		(0x29A * QM_DQF_MAC_ROW_SIZE)
#define QM_CMAC_LA_FIFO_DEPTH		(0x80 * QM_DQF_MAC_ROW_SIZE)
#define QM_HMAC_FIFO_DEPTH		(0x40 * QM_DQF_MAC_ROW_SIZE)

#define QM_EMAC_FIFO_DEPTH(_port_)	((_port_ == 4) ? QM_EMAC_10G_FIFO_DEPTH : QM_EMAC_1G_FIFO_DEPTH)
#define QM_CMAC_FIFO_DEPTH(_port_)	((_port_ == 10) ? QM_CMAC_IN_FIFO_DEPTH : QM_CMAC_LA_FIFO_DEPTH)

/* credit threshold - relevant for MAC ports only, set when DQF sends indication to the MAC */
#define QM_EMAC_CREDIT_THR		(0x282 * QM_DQF_MAC_ROW_SIZE)
#define QM_CMAC_IN_CREDIT_THR		(0x268 * QM_DQF_MAC_ROW_SIZE)
#define QM_CMAC_LA_CREDIT_THR		(0x68 * QM_DQF_MAC_ROW_SIZE)
#define QM_HMAC_CREDIT_THR		(0x28 * QM_DQF_MAC_ROW_SIZE)
#define QM_CMAC_CREDIT_THR(_port_)	\
			((_port_ == 10) ? QM_CMAC_IN_CREDIT_THR : QM_CMAC_LA_CREDIT_THR)

/* max credit - How many credits needed to start reading DRAM chunk */
#define QM_EMAC_10G_REQ_MAX_CREDIT	(0x7d0)  /* 2000 */
#define QM_EMAC_1G_REQ_MAX_CREDIT	(0x2648) /* 9800 */
#define QM_CMAC_IN_REQ_MAX_CREDIT	(0x7d0)  /* 2000 */

#define QM_EMAC_REQ_MAX_CREDIT(_port_)	\
			((_port_ == 4) ? QM_EMAC_10G_REQ_MAX_CREDIT : QM_EMAC_1G_REQ_MAX_CREDIT)
#define QM_CMAC_REQ_MAX_CREDIT(_port_)	\
			((_port_ == 10) ? QM_CMAC_IN_REQ_MAX_CREDIT : 0)

/* Default pools thresholds */
#define QM_POOL_THR_LOW				0x0000000C
#define QM_POOL_THR_HIGH			0x00000018


/* read/write attributes defenitions */
#define QM_SWF_AWQOS_DEF			0x00000001
#define QM_RDMA_AWQOS_DEF			0x00000001
#define QM_HWF_QE_CE_AWQOS_DEF			0x00000001
#define QM_HWF_SFH_PL_AWQOS_DEF			0x00000001

#define QM_SWF_AWCACHE_DEF			0x0000000B
#define QM_RDMA_AWCACHE_DEF			0x0000000B
#define QM_HWF_QE_CE_AWCACHE_DEF		0x00000003
#define QM_HWF_SFH_PL_AWCACHE_DEF		0x00000003

#define QM_SWF_AWDOMAIN_DEF			0x00000002
#define QM_RDMA_AWDOMAIN_DEF			0x00000002
#define QM_HWF_QE_CE_AWDOMAIN_DEF	        0
#define QM_HWF_SFH_PL_AWDOMAIN_DEF	        0

#define QM_SWF_ARQOS_DEF			0x00000001
#define QM_RDMA_ARQOS_DEF			0x00000001
#define QM_HWF_QE_CE_ARQOS_DEF			0x00000001
#define QM_HWF_SFH_PL_ARQOS_DEF			0x00000001

#define QM_SWF_ARCACHE_DEF			0x0000000B
#define QM_RDMA_ARCACHE_DEF			0x0000000B
#define QM_HWF_QE_CE_ARCACHE_DEF		0x00000003
#define QM_HWF_SFH_PL_ARCACHE_DEF		0x00000003

#define QM_SWF_ARDOMAIN_DEF			0x00000002
#define QM_RDMA_ARDOMAIN_DEF			0x00000002
#define QM_HWF_QE_CE_ARDOMAIN_DEF		0
#define QM_HWF_SFH_PL_ARDOMAIN_DEF		0

/* all SIDs are allocated by pool 0 */
#define QM_POOL0_SID_NUM	QM_SID_MAX
#define QM_POOL1_SID_NUM	0x0

/* reorder unit defenitions */
#define QM_SID_MAX				0x00001000
#define QM_VMID_MAX				0x0000003F
#define QM_CLASS_MAX				0x0000003F
#define QM_SID_POOL_MAX				0x00000001
#define QM_INPUT_PORT_MAX			0x0000011F
#define QM_ATTR_DOMAIN_MAX			0x00000003
#define QM_ATTR_CACHE_MAX			0x0000000F
#define QM_ATTR_QOS_MAX				0x00000003

/* Init registers alias structures */
void qm_init(void __iomem *base);

/* Enable/Disable QM registers read and write dumps */
void qm_dbg_flags(u32 flag, u32 en);

/* Enable/disable tail pointer insertion in the CFH, per EMAC source */
void qm_tail_ptr_mode(int emac, bool enable);

/**
 *  Set base address in Dram for pool
 *
 */
void qm_pfe_base_address_pool_set(struct mv_a40 *pl_base_address, /* Payload DRAM base address */
				  struct mv_a40 *qece_base_address); /* QE/CE DRAM base address */

/**
 *  Enables QM,
 *  Configure DMA with GPM pool thresholds with default values
 *  Return values:
 *		0 - success
 */
void qm_dma_gpm_pools_def_enable(void);

/**
 *  Configure DMA with DRAM pool thresholds with default values
 *  Return values:
 *		0 - success
 */
void qm_dma_dram_pools_def_enable(void);

/**
 *  Set default for QM units for mandatory parameters
 *  Inputs:
 *      ppc_num - number of active PPCs
 *  Return values:
 *		0 - success
 */
int qm_default_set(int ppc_num);

/*
 clear HW configuration
	- disable all queues threshold profile
	- disable all internal back pressure groups
	- disable drop prots
	- dsconnect PPCs from all ports
	- zeroed ports fifo parameters (depth = 0 and base = 0)
	- clear ports credit threshold
*/

void qm_clear_hw_config(void);

/**
 *  Configure DQF for each port which PPC (data or maintenance) handles the packet,
 *  relevant only for PPC port with default values.
 *  Return values:
 *		0 - success
 */
int qm_dqf_port_ppc_map_def_set(void);

/**
 *  Configures QL thresholds
 *  parameters:
 *    profie           - profile number
 *    low, pause, high - thresholds in KB
 *  Return values:
 *		0 - success
 */

void qm_ql_profile_cfg(enum mv_qm_thr_profiles profile, u32 low, u32 pause, u32 high);

int qm_ql_group_bpi_set(int group, int xon_thr, int xoff_thr,
				enum mv_qm_node_type level, int node_id);
int qm_ql_group_bpi_get(int group, int *xon_thr, int *xoff_thr,
				enum mv_qm_node_type *level, int *node_id);

void qm_ql_group_bpi_show(int group);
void qm_ql_group_bpi_show_all(void);

int qm_ql_queue_bpi_group_set(int queue, int group);

/**
 *  Configre PFE to start Flushing Queue. This process takes a while.
 *  Indication for its completion is when Queue is empty
 *  Return values:
 *		0 - success
 */
int qm_queue_flush_start(int queue); /* queue number from 0 to 511 */

/**
 *  Configre PFE to start Flushing Port. This process takes a while.
 *  Indication for its completion is when Port is empty
 *  Return values:
 *		0 - success
 */
int qm_port_flush_start(int port);

/**
 *  Gives queue length and status
 *  Return values:
 *		0 - success
 */
int qm_queue_len_get(int queue, u32 *length, u32 *status);

/**
 *  Get Idle status from DMA
*/
void qm_idle_status_get(u32 *status);

/**
 * Configure REORDER with class command when permission is granted.
 *  Return values:
 *		0 - success
 */
int qm_ru_class_cmd_set(u32 host, u32 host_class, u32 host_sid, u32 cmd);
/*
 *  Check error bits.
 *  If set to 1, and print the error that occurred.
 *  Bit 0 is always an OR of all the other errors.
 *  So bit 0 with value 0 indicates there are no errors.
 *  Return values:
 *		0 - success
*/
void qm_errors_dump(void);

/*
 *  Print all global registers
*/
void qm_global_dump(void);

/*
 *  Print registers per queues 0 to 511
*/
void qm_queue_dump(int queue);


/*
 * Print for the queues that are not empty the following.
 * Run on all queues from 0 to 511.
 * For each queue print hw length indications if qlen, qlen_bp or  qlen_drop are not 0.
*/
void qm_nempty_queue_len_dump(void);

/*
 * Print dqf port parameters:
 *  Return values:
 *		0 - success
*/
void qm_dqf_port_dump(int port);


/* Set emac ingress queue for secret machine
 * inputs
 *      emac - emac number
 *      queue - queue number to stop
*/

void qm_xoff_emac_qnum_set(int emac, int queue);

/* Set hmac ingress queues range for secret machine
 * inputs
 *      first_q - first queue to stop
 *      q_num   - number of queues to stop
*/

void qm_xoff_hmac_qs_set(int first_q, int q_num);

/* Set emac threshold profile to default values
 * inputs
 *      emac - emac number
 *      queue - attached queue to profile
*/
int qm_emac_profile_set(int emac, enum mv_port_mode port_mode, int queue);

/* Set hmac threshold profile to default values
 * inputs
 *      parameters define queues range to attached to hmac profile
 *      first_q - hmac->ppc first queue
 *      q_num   - hmac->ppc queues number
*/

int qm_hmac_profile_set(int first_q, int q_num);

/*
 * Config internal back pressure group
 * Parameters:
 *  group - group number
 *  xon_thr, xoff_thr - low / high thresholds in KB
 *  level - node type (A/B/C)
 *  node_id - node number
*/

int qm_ql_group_bpi_set(int group, int xon_thr, int xoff_thr,
				enum mv_qm_node_type level, int node_id);

/*
 *  Mapping qm queue to internal back pressure group
 *  Return values:
 *		0 - success
 */
int qm_ql_queue_bpi_group_set(int queue, int group);

/*
 * QM sysfs functions
 */
int mv_pp3_qm_sysfs_init(struct kobject *neta_kobj);
int mv_pp3_qm_sysfs_exit(struct kobject *emac_kobj);
void qm_ql_profile_show(enum mv_qm_thr_profiles profile);
const char *mv_qm_node_str(enum mv_qm_node_type level);

#endif /* __mv_qm_h__ */
