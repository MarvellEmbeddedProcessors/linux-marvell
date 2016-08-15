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
/* includes */
#include "common/mv_sw_if.h"
#include "common/mv_hw_if.h"
#include "qm/mv_qm.h"
#include "qm/mv_qm_regs.h"
#include "tm/wrappers/mv_tm_scheme.h"

static int ppc_port_fifo_base;
static int mac_port_fifo_base;
static int qm_regs_debug_flags;


void qm_dbg_flags(u32 flag, u32 en)
{
	u32 bit_flag;

	bit_flag = (fls(flag) - 1);

	if (en)
		qm_regs_debug_flags |= (1 << bit_flag);
	else
		qm_regs_debug_flags &= ~(1 << bit_flag);

	return;
}

static void qm_register_read(u32 base_address, u32 offset, u32 wordsNumber, u32 *dataPtr)
{

	mv_pp3_hw_read((void __iomem *)base_address + offset, wordsNumber, dataPtr);

	if (qm_regs_debug_flags & QM_F_DBG_RD) {
		int i;

		for (i = 0; i < wordsNumber; i++)
			pr_info("QM READ : 0x%08x = 0x%08x\n", base_address+offset + (4 * i), dataPtr[i]);
	}
}

static void qm_register_write(u32 base_address, u32 offset, u32 wordsNumber, u32 *dataPtr)
{

	mv_pp3_hw_write((void __iomem *)base_address + offset, wordsNumber, dataPtr);

	if (qm_regs_debug_flags & QM_F_DBG_WR) {
		int i;

		for (i = 0; i < wordsNumber; i++)
			pr_info("QM WRITE: 0x%08x = 0x%08x\n", base_address+offset + (4 * i), dataPtr[i]);
	}
}

static void mv_pp3_qm_reg_print(char *reg_name, u32 reg, u32 val)
{
	pr_info("  %-32s: 0x%04x = 0x%08x\n", reg_name, reg, val);
}

/* send XOFF to macs when BM-almost-empty is asserted */
static void qm_xoff_to_macs_enable(void)
{
	u32 val = 1;

	qm_register_write(qm.ql.base + 0x0000600, 0, 1, &val);
}

/* set hmac ingress queues for secret machine stop/start */
void qm_xoff_hmac_qs_set(int first_q, int q_num)
{
	u32 val = first_q | ((first_q + q_num) << 16);
	qm_register_write(qm.ql.xoff_hmac_qs, 0, 1, &val);
}

/* set emac ingress queue for secret machine stop/start */
void qm_xoff_emac_qnum_set(int emac, int queue)
{
	qm_register_write(qm.ql.xoff_mac_qnum, emac * 4, 1, &queue);
}


/* Enable/disable tail pointer insertion in the CFH, per EMAC source */
void qm_tail_ptr_mode(int emac, bool enable)
{
	u32 reg_val;

	qm_register_read(qm.dma.tail_pointer_en, 0, 1, &reg_val);

	if (enable)
		reg_val |= 1 << emac;
	else
		reg_val &= ~(1 << emac);

	qm_register_write(qm.dma.tail_pointer_en, 0, 1, &reg_val);
}

/* Data FIFO port 15 parameters - depth and base offset */
void qm_data_fifo_drop_port_cfg(void)
{
	u32 val = 0X1020000;
	int reg_base_address =      qm.dqf.Data_FIFO_params_p;
	int reg_size   =   qm_reg_size.dqf.Data_FIFO_params_p;
	int reg_offset = qm_reg_offset.dqf.Data_FIFO_params_p * 15;

	qm_register_write(reg_base_address, reg_offset, reg_size, &val);
}

/* init regisers base, offset and size */
void qm_init(void __iomem *base)
{
	qm_reg_address_alias_init((u32)base);
	qm_reg_size_alias_init();
	qm_reg_offset_alias_init();
}


void qm_pfe_base_address_pool_set(struct mv_a40 *qece_base_address, struct mv_a40 *pyld_base_address)
{
	u32 reg_base_address, reg_size, reg_offset;

	struct pfe_qece_dram_base_address_hi         reg_qece_dram_base_address_hi;
	struct pfe_qece_dram_base_address_lo         reg_qece_dram_base_address_lo;
	struct pfe_pyld_dram_base_address_hi         reg_pyld_dram_base_address_hi;
	struct pfe_pyld_dram_base_address_lo         reg_pyld_dram_base_address_lo;

	if (qece_base_address) {
		reg_base_address =      qm.pfe.qece_dram_base_address_hi;
		reg_size   =   qm_reg_size.pfe.qece_dram_base_address_hi;
		reg_offset = qm_reg_offset.pfe.qece_dram_base_address_hi * 0;

		qm_register_read(reg_base_address, reg_offset, reg_size, (u32 *)&reg_qece_dram_base_address_hi);
		reg_qece_dram_base_address_hi.qece_dram_base_address_hi	= qece_base_address->dma_msb;
		qm_register_write(reg_base_address, reg_offset, reg_size, (u32 *)&reg_qece_dram_base_address_hi);

		reg_base_address =      qm.pfe.qece_dram_base_address_lo;
		reg_size   =   qm_reg_size.pfe.qece_dram_base_address_lo;
		reg_offset = qm_reg_offset.pfe.qece_dram_base_address_lo * 0;

		qm_register_read(reg_base_address, reg_offset, reg_size, (u32 *)&reg_qece_dram_base_address_lo);

		reg_qece_dram_base_address_lo.qece_dram_base_address_low = qece_base_address->dma_lsb;
		qm_register_write(reg_base_address, reg_offset, reg_size, (u32 *)&reg_qece_dram_base_address_lo);
	}

	if (pyld_base_address) {
		reg_base_address =      qm.pfe.pyld_dram_base_address_hi;
		reg_size   =   qm_reg_size.pfe.pyld_dram_base_address_hi;
		reg_offset = qm_reg_offset.pfe.pyld_dram_base_address_hi * 0;

		qm_register_read(reg_base_address, reg_offset, reg_size, (u32 *)&reg_pyld_dram_base_address_hi);

		reg_pyld_dram_base_address_hi.pyld_dram_base_address_hi	 = pyld_base_address->dma_msb;
		qm_register_write(reg_base_address, reg_offset, reg_size, (u32 *)&reg_pyld_dram_base_address_hi);

		reg_base_address =      qm.pfe.pyld_dram_base_address_lo;
		reg_size   =   qm_reg_size.pfe.pyld_dram_base_address_lo;
		reg_offset = qm_reg_offset.pfe.pyld_dram_base_address_lo * 0;

		qm_register_read(reg_base_address, reg_offset, reg_size, (u32 *)&reg_pyld_dram_base_address_lo);

		reg_pyld_dram_base_address_lo.pyld_dram_base_address_low = pyld_base_address->dma_lsb;
		qm_register_write(reg_base_address, reg_offset, reg_size, (u32 *)&reg_pyld_dram_base_address_lo);
	}
}

/* Configure DMA with GPM pool thresholds, Enable QM */
static void qm_dma_gpm_pools_enable(u32 qece_thr_hi, u32 qece_thr_lo, u32 pl_thr_hi, u32 pl_thr_lo)
{
	struct dma_gpm_thresholds reg_gpm_thresholds;
	u32 reg_base_address, reg_size, reg_offset;

	reg_base_address =      qm.dma.gpm_thresholds;
	reg_size   =   qm_reg_size.dma.gpm_thresholds;
	reg_offset = qm_reg_offset.dma.gpm_thresholds * 0;

	qm_register_read(reg_base_address, reg_offset, reg_size, (u32 *)&reg_gpm_thresholds);

	reg_gpm_thresholds.gpm_qe_pool_low_bp  = qece_thr_lo;	/* qe_thr & 0x00FFFFFFFF; */
	reg_gpm_thresholds.gpm_qe_pool_high_bp = qece_thr_hi;	/* qe_thr >> 32; */
	reg_gpm_thresholds.gpm_pl_pool_low_bp  =   pl_thr_lo;	/* pl_thr & 0x00FFFFFFFF; */
	reg_gpm_thresholds.gpm_pl_pool_high_bp =   pl_thr_hi;	/* pl_thr >> 32; */
	qm_register_write(reg_base_address, reg_offset, reg_size, (u32 *)&reg_gpm_thresholds);
}

void qm_dma_gpm_pools_def_enable(void)
{
	/* set gpm qece and pl pools (0 and 1) thresholds */
	qm_dma_gpm_pools_enable(QM_POOL_THR_HIGH, QM_POOL_THR_LOW,
					QM_POOL_THR_HIGH, QM_POOL_THR_LOW);
}

/* Configure DMA with DRAM pool thresholds */
static void qm_dma_dram_pools_enable(u32 qece_thr_hi, u32 qece_thr_lo, u32 pl_thr_hi, u32 pl_thr_lo)
{
	struct dma_dram_thresholds reg_dram_thresholds;
	u32 reg_base_address, reg_size, reg_offset;

	reg_base_address =      qm.dma.dram_thresholds;
	reg_size   =   qm_reg_size.dma.dram_thresholds;
	reg_offset = qm_reg_offset.dma.dram_thresholds * 0;

	qm_register_read(reg_base_address, reg_offset, reg_size, (u32 *)&reg_dram_thresholds);
	reg_dram_thresholds.dram_qe_pool_low_bp  = qece_thr_lo;	/* qe_thr & 0x00FFFFFFFF; */
	reg_dram_thresholds.dram_qe_pool_high_bp = qece_thr_hi;	/* qe_thr >> 32; */
	reg_dram_thresholds.dram_pl_pool_low_bp  =   pl_thr_lo;	/* pl_thr & 0x00FFFFFFFF; */
	reg_dram_thresholds.dram_pl_pool_high_bp =   pl_thr_hi;	/* pl_thr >> 32; */
	qm_register_write(reg_base_address, reg_offset, reg_size, (u32 *)&reg_dram_thresholds);
}


void qm_dma_dram_pools_def_enable(void)
{
	/* set gpm qece and pl pools (2 and 3) thresholds */
	qm_dma_dram_pools_enable(QM_POOL_THR_HIGH, QM_POOL_THR_LOW,
					QM_POOL_THR_HIGH, QM_POOL_THR_LOW);
}

/*
qm_dqf_port_data_fifo_params_set:
	description:
		set port FIFO base and offset
	parameters:
		port  - QM port number
		base  - base offset in bytes
		depth - FIFO depth in bytes
*/

static int qm_dqf_port_data_fifo_params_set(int port, int base, int depth)
{
	struct dqf_Data_FIFO_params_p reg_data_fifo_params;
	u32 reg_base_address, reg_size, reg_offset;

	if (port <= 3) {
		/* rows of 144B */
		reg_data_fifo_params.data_fifo_depth_p  =
			depth / QM_DQF_PPC_ROW_SIZE;
		reg_data_fifo_params.data_fifo_base_p =
			base / QM_DQF_PPC_ROW_SIZE;
	} else {
		/* rows of 16B */
		reg_data_fifo_params.data_fifo_depth_p  =
			depth/QM_DQF_MAC_ROW_SIZE;
		reg_data_fifo_params.data_fifo_base_p   =
			base / QM_DQF_MAC_ROW_SIZE;
	}


	reg_base_address =      qm.dqf.Data_FIFO_params_p;
	reg_size   =   qm_reg_size.dqf.Data_FIFO_params_p;
	reg_offset = qm_reg_offset.dqf.Data_FIFO_params_p * port;

	qm_register_write(qm.dqf.Data_FIFO_params_p,
			  qm_reg_offset.dqf.Data_FIFO_params_p * port,
			  qm_reg_size.dqf.Data_FIFO_params_p,
			  (u32 *)&reg_data_fifo_params);

	return MV_OK;
}

/*
qm_dqf_all_ports_credit_thr_set:
	description:
		set Credit Threshold per port
	parameters:
		port   - QM port number
		credit - Credit Threshold in bytes
*/
static int qm_dqf_port_credit_thr_set(int port, int credit)
{
	struct dqf_Credit_Threshold_p          reg_credit_threshold_p;

	if (port >= QM_PORTS_NUM) {
		pr_err("Invalid port number %d\n", port);
		return -EINVAL;
	}

	memset(&reg_credit_threshold_p, 0, sizeof(struct dqf_Credit_Threshold_p));

	reg_credit_threshold_p.Credit_Threshold_p   = credit / 16;

	qm_register_write(qm.dqf.Credit_Threshold_p,
			  qm_reg_offset.dqf.Credit_Threshold_p * port,
			  qm_reg_size.dqf.Credit_Threshold_p, (u32 *)&reg_credit_threshold_p);
	return MV_OK;
}

/**
*  Configure DQF for each port which PPC (data or maintenance) handles the packet,
*  relevant only for PPC port.
*  Return values:
*		0 - success
*/
static int qm_dqf_port_ppc_map_set(int port, u32 ppc_bitmap)
{
	struct dqf_PPC_port_map_p reg_PPC_port_map_p;
	u32 reg_base_address, reg_size, reg_offset;

	if (port >= QM_PORTS_NUM) {
		pr_err("Invalid port number %d\n", port);
		return -EINVAL;
	}

	memset(&reg_PPC_port_map_p, 0, sizeof(struct dqf_PPC_port_map_p));

	reg_base_address =      qm.dqf.PPC_port_map_p;
	reg_size   =   qm_reg_size.dqf.PPC_port_map_p;
	reg_offset = qm_reg_offset.dqf.PPC_port_map_p * port;

	reg_PPC_port_map_p.ppc_port_map_p = ppc_bitmap;

	qm_register_write(reg_base_address, reg_offset, reg_size, (u32 *)&reg_PPC_port_map_p);

	return MV_OK;
}

static int qm_ql_q_profile_set(int queue, enum mv_qm_thr_profiles profile)
{
	u32 reg_offset, queue_offset, reg_qptr_entry;

	if (queue >= QM_QUEUES_NUM) {
		pr_err("Invalid queue number %d\n", queue);
		return -EINVAL;
	}

	reg_offset = qm_reg_offset.ql.qptr * (queue/8);
	qm_register_read(qm.ql.qptr, reg_offset, qm_reg_size.ql.qptr, &reg_qptr_entry);

	/* each queue is 3 bits, 8 queues in u32 */
	queue_offset = (queue % 8) * 3;
	/* clear old value */
	reg_qptr_entry &= ~(0x7 << queue_offset);

	if (profile != QM_THR_PROFILE_INVALID)
		reg_qptr_entry |= ((profile + 1) << queue_offset);

	qm_register_write(qm.ql.qptr, reg_offset, qm_reg_size.ql.qptr, &reg_qptr_entry);

	return MV_OK;
}

/*
 description:
	return queue thersholds profile number
	inputs:
		qeueue  - queue number
*/
static int qm_ql_q_profile_get(int queue, enum mv_qm_thr_profiles *profile)
{
	u32 reg_offset, queue_offset, reg_qptr_entry;

	if (!profile) {
		pr_err("input pointer is NULL");
		return -EINVAL;
	}

	if (queue >= QM_QUEUES_NUM) {
		pr_err("Invalid queue number %d\n", queue);
		return -EINVAL;
	}

	reg_offset = qm_reg_offset.ql.qptr * (queue/8);

	qm_register_read(qm.ql.qptr, reg_offset, qm_reg_size.ql.qptr, &reg_qptr_entry);

	/* each queue is 3 bits, 8 queues in u32 */
	queue_offset = (queue % 8) * 3;

	reg_qptr_entry &= 0x7 << queue_offset;

	*profile = reg_qptr_entry >> queue_offset;

	return MV_OK;
}

int qm_emac_profile_set(int emac, enum mv_port_mode port_mode, int queue)
{
	int ret_val;

	if ((port_mode == MV_PORT_XAUI) || (port_mode == MV_PORT_RXAUI))
		/* 10 Giga port thresholds */
		qm_ql_profile_cfg(emac, MV_PP3_10G_LOW_THR, MV_PP3_10G_PAUSE_THR, MV_PP3_10G_HIGH_THR);

	else  if (port_mode == MV_PORT_SGMII2_5)
		/* 2.5 Giga port thresholds */
		qm_ql_profile_cfg(emac, MV_PP3_2_5G_LOW_THR, MV_PP3_2_5G_PAUSE_THR, MV_PP3_2_5G_HIGH_THR);
	else
		/* 1 Giga port thresholds */
		qm_ql_profile_cfg(emac, MV_PP3_1G_LOW_THR, MV_PP3_1G_PAUSE_THR, MV_PP3_1G_HIGH_THR);

	/* connect queue to profile */
	ret_val = qm_ql_q_profile_set(queue, emac);

	if (ret_val < 0) {
		pr_err("%s: mapping queue %d to emac %d profile failed\n",
				__func__, queue, emac);
		return ret_val;
	}

	return MV_OK;
}


int qm_hmac_profile_set(int first_q, int q_num)
{
	int ret_val, queue, last_q;

	qm_ql_profile_cfg(QM_THR_PROFILE_HMAC, MV_PP3_HMAC_LOW_THR,
			  MV_PP3_HMAC_PAUSE_THR, MV_PP3_HMAC_HIGH_THR);

	last_q = q_num + first_q;

	for (queue = first_q; queue < last_q; queue++) {
		ret_val = qm_ql_q_profile_set(queue, QM_THR_PROFILE_HMAC);

		if (ret_val < 0) {
			pr_err("%s: mapping queue %d to hmac profile (%d) failed\n",
					__func__, queue, QM_THR_PROFILE_HMAC);
			return ret_val;
		}
	}

	return MV_OK;
}

/* mapping queue to port for drop recommendation */
static int qm_ql_queue_port_set(int queue, int port)
{
	u32 reg_offset, queue_offset, reg_val;

	if (queue >= QM_QUEUES_NUM) {
		pr_err("Invalid queue number %d\n", queue);
		return -EINVAL;
	}

	if (port >= QM_PORTS_NUM) {
		pr_err("Invalid port number %d\n", port);
		return -EINVAL;
	}

	reg_offset = qm_reg_offset.ql.qmap_port * (queue/4);
	qm_register_read(qm.ql.qmap_port, reg_offset, qm_reg_size.ql.qmap_port, &reg_val);

	/* each queue is 6 bits + 2 reserved */
	queue_offset = (queue % 4) * 8;

	/* clear old value */
	reg_val &= ~(0xF << queue_offset);

	reg_val |= (port << queue_offset);

	qm_register_write(qm.ql.qmap_port, reg_offset, qm_reg_size.ql.qmap_port, &reg_val);

	return MV_OK;
}

/* mapping queue to internal back pressure group */
int qm_ql_queue_bpi_group_set(int queue, int group)
{
	u32 reg_offset, queue_offset, reg_val;

	if (queue >= QM_QUEUES_NUM) {
		pr_err("Invalid queue number %d\n", queue);
		return -EINVAL;
	}

	if (group >= MV_PP3_QM_BP_RULES_NUM) {
		pr_err("Invalid group number %d\n", group);
		return -EINVAL;
	}

	reg_offset = qm_reg_offset.ql.qmap_group * (queue/4);
	qm_register_read(qm.ql.qmap_group, reg_offset, qm_reg_size.ql.qmap_group, &reg_val);

	/* each queue is 6 bits + 2 reserved */
	queue_offset = (queue % 4) * 8;

	/* clear old value */
	reg_val &= ~(0x3F << queue_offset);

	reg_val |= (group << queue_offset);

	qm_register_write(qm.ql.qmap_group, reg_offset, qm_reg_size.ql.qmap_group, &reg_val);

	return MV_OK;
}
/* get queue internal back pressure group */
static int qm_ql_queue_bpi_group_get(int queue, int *group)
{
	u32 reg_offset, queue_offset, reg_val;

	if (queue >= QM_QUEUES_NUM) {
		pr_err("Invalid queue number %d\n", queue);
		return -EINVAL;
	}

	reg_offset = qm_reg_offset.ql.qmap_group * (queue/4);
	qm_register_read(qm.ql.qmap_group, reg_offset, qm_reg_size.ql.qmap_group, &reg_val);

	/* each queue is 6 bits + 2 reserved */
	queue_offset = (queue % 4) * 8;

	/* clear old value */
	reg_val &= 0x3F << queue_offset;

	*group = reg_val >> queue_offset;

	return MV_OK;
}


const char *mv_qm_node_str(enum mv_qm_node_type level)
{
	const char *str;

	switch (level) {
	case MV_QM_Q_NODE:
		str = "Q-NODE";
		break;
	case MV_QM_A_NODE:
		str = "A-NODE";
		break;
	case MV_QM_B_NODE:
		str = "B-NODE";
		break;
	case MV_QM_C_NODE:
		str = "C-NODE";
		break;
	case MV_QM_PORT_NODE:
		str = "P-NODE";
		break;
	default:
		str = "Unknown";
	}
	return str;
}

/* config internal back pressure group */
int qm_ql_group_bpi_set(int group, int xon_thr, int xoff_thr, enum mv_qm_node_type level, int node_id)
{
	struct ql_rule_bpi_entry reg_rule_bpi_entry;

	memset(&reg_rule_bpi_entry, 0, sizeof(struct ql_rule_bpi_entry));

	if ((level != MV_QM_A_NODE) && (level != MV_QM_B_NODE) && (level != MV_QM_C_NODE)) {
		pr_err("%s: invalid node type %s (%d)\n", __func__, mv_qm_node_str(level), level);
		return -EINVAL;
	}

	/* TODO inputs validation */

	reg_rule_bpi_entry.xon_thr = (xon_thr * 1024) / MV_PP3_QM_UNITS;
	reg_rule_bpi_entry.xoff_thr = (xoff_thr * 1024) / MV_PP3_QM_UNITS;

	reg_rule_bpi_entry.target_lvl = level;
	reg_rule_bpi_entry.target_node = node_id;

	qm_register_write(qm.ql.rule_bpi, qm_reg_offset.ql.rule_bpi * group,
				qm_reg_size.ql.rule_bpi, (u32 *)&reg_rule_bpi_entry);

	return MV_OK;
}

int qm_ql_group_bpi_get(int group, int *xon_thr, int *xoff_thr, enum mv_qm_node_type *level, int *node_id)
{
	struct ql_rule_bpi_entry reg_rule_bpi_entry;

	if (group >= MV_PP3_QM_BP_RULES_NUM) {
		pr_err("Invalid group number %d\n", group);
		return -EINVAL;
	}

	qm_register_read(qm.ql.rule_bpi, qm_reg_offset.ql.rule_bpi * group,
			qm_reg_size.ql.rule_bpi, (u32 *)&reg_rule_bpi_entry);
	if (xon_thr)
		*xon_thr = (reg_rule_bpi_entry.xon_thr * MV_PP3_QM_UNITS / 1024);
	if (xoff_thr)
		*xoff_thr = (reg_rule_bpi_entry.xoff_thr * MV_PP3_QM_UNITS / 1024);
	if (level)
		*level = reg_rule_bpi_entry.target_lvl & 0x3;
	if (node_id)
		*node_id = reg_rule_bpi_entry.target_node & 0x7F;

	return 0;
}

static int qm_ql_rule_bpi_disable(int group)
{
	/* disable group back pressure - set high xon_thr and xoff_thr */
	return qm_ql_group_bpi_set(group, 0x1FFF, 0x1FFF, MV_QM_A_NODE, 0);
}

void qm_ql_group_bpi_show_all(void)
{
	int group;
	struct ql_rule_bpi_entry reg_rule_bpi_entry;

	for (group = 0; group < QM_BPI_GROUPS; group++) {
		qm_register_read(qm.ql.rule_bpi, qm_reg_offset.ql.rule_bpi * group,
				qm_reg_size.ql.rule_bpi, (u32 *)&reg_rule_bpi_entry);

		/* print only enabled groups */
		if (reg_rule_bpi_entry.xon_thr < reg_rule_bpi_entry.xoff_thr)
			qm_ql_group_bpi_show(group);
	}
}

void qm_ql_group_bpi_show(int group)
{
	struct ql_rule_bpi_entry reg_rule_bpi_entry;
	const char *node_name;
	int queue, q_group, count = 0;

	qm_register_read(qm.ql.rule_bpi, qm_reg_offset.ql.rule_bpi * group,
				qm_reg_size.ql.rule_bpi, (u32 *)&reg_rule_bpi_entry);

	node_name = mv_qm_node_str(reg_rule_bpi_entry.target_lvl & 0x3);

	pr_info("\n-------------- QM QL BPI group %d -----------", group);
	pr_info("Xon  threshold [KB]  :  %d\n", (reg_rule_bpi_entry.xon_thr * MV_PP3_QM_UNITS) / 1024);
	pr_info("Xoff threshold [KB]  :  %d\n", (reg_rule_bpi_entry.xoff_thr * MV_PP3_QM_UNITS) / 1024);
	pr_info("Node                 :  %s %d\n", node_name, reg_rule_bpi_entry.target_node & 0x7F);

	pr_info("Connected queues list:");
	for (queue = 0; queue < QM_QUEUES_NUM; queue++) {
		qm_ql_queue_bpi_group_get(queue, &q_group);

		if (q_group == group) {
			pr_cont(" %3d", queue);
			count++;

			if ((count % 25) == 0)
				pr_info("\n                      ");
		}
	}

	pr_info("\n");
}

/* set thresholds profile
	parameters:
		profie - profile number
		low, pause, high - thresholds in KB
		source - traffic source to stop
*/
void qm_ql_profile_cfg(enum mv_qm_thr_profiles profile, u32 low, u32 pause, u32 high)
{
	struct ql_low_threshold		reg_low_threshold;
	struct ql_pause_threshold	reg_pause_threshold;
	struct ql_high_threshold	reg_high_threshold;
	struct ql_traffic_source	reg_traffic_source;
	u32 reg_base_address, reg_size, reg_offset;

	memset(&reg_low_threshold, 0, sizeof(struct ql_low_threshold));
	memset(&reg_pause_threshold, 0, sizeof(struct ql_pause_threshold));
	memset(&reg_high_threshold, 0, sizeof(struct ql_high_threshold));
	memset(&reg_traffic_source, 0, sizeof(struct ql_traffic_source));

	reg_base_address =      qm.ql.low_threshold;
	reg_size   =   qm_reg_size.ql.low_threshold;
	reg_offset = qm_reg_offset.ql.low_threshold * profile;
	reg_low_threshold.low_threshold   = (low * 1024) / MV_PP3_QM_UNITS;
	qm_register_write(reg_base_address, reg_offset, reg_size, (u32 *)&reg_low_threshold);

	reg_base_address =      qm.ql.pause_threshold;
	reg_size   =   qm_reg_size.ql.pause_threshold;
	reg_offset = qm_reg_offset.ql.pause_threshold * profile;
	reg_pause_threshold.pause_threshold = (pause * 1024) / MV_PP3_QM_UNITS;
	qm_register_write(reg_base_address, reg_offset, reg_size, (u32 *)&reg_pause_threshold);

	reg_base_address =      qm.ql.high_threshold;
	reg_size   =   qm_reg_size.ql.high_threshold;
	reg_offset = qm_reg_offset.ql.high_threshold * profile;
	reg_high_threshold.high_threshold = (high * 1024) / MV_PP3_QM_UNITS;
	qm_register_write(reg_base_address, reg_offset, reg_size, (u32 *)&reg_high_threshold);

	reg_base_address =      qm.ql.traffic_source;
	reg_size   =   qm_reg_size.ql.traffic_source;
	reg_offset = qm_reg_offset.ql.traffic_source * profile;
	reg_traffic_source.traffic_source = profile;
	qm_register_write(reg_base_address, reg_offset, reg_size, (u32 *)&reg_traffic_source);
}

int qm_queue_flush_start(int queue)
{
	struct pfe_queue_flush reg_queue_flush;
	u32 reg_base_address, reg_size, reg_offset;

	if (queue >= QM_QUEUES_NUM) {
		pr_err("Invalid queue number %d\n", queue);
		return -EINVAL;
	}

	reg_base_address =      qm.pfe.queue_flush;
	reg_size   =   qm_reg_size.pfe.queue_flush;
	reg_offset = qm_reg_offset.pfe.queue_flush * (queue/32);

	qm_register_read(reg_base_address, reg_offset, reg_size, (u32 *)&reg_queue_flush);
	reg_queue_flush.queue_flush_bit_per_q |=  (0x00000001 << ((reg_queue_flush.queue_flush_bit_per_q)%32));
	qm_register_write(reg_base_address, reg_offset, reg_size, (u32 *)&reg_queue_flush);

	return MV_OK;
}

static int qm_port_max_credit_request_set(int port, int credit)
{
	if (port >= QM_PORTS_NUM) {
		pr_err("Invalid port number %d\n", port);
		return -EINVAL;
	}

	qm_register_write(qm.pfe.max_credit_for_new_dram_req, port * 4, 1, &credit);

	return MV_OK;
}

static int qm_port_drop_mode_set(u32 port, bool enable)
{
	struct pfe_port_flush reg_port_flush;
	u32 reg_base_address, reg_size, reg_offset;

	if (port >= QM_PORTS_NUM) {
		pr_err("Invalid port number %d\n", port);
		return -EINVAL;
	}

	reg_base_address =      qm.pfe.port_flush;
	reg_size   =   qm_reg_size.pfe.port_flush;
	reg_offset = qm_reg_offset.pfe.port_flush * 0;

	qm_register_read(reg_base_address, reg_offset, reg_size, (u32 *)&reg_port_flush);

	if (enable)
		reg_port_flush.port_flush |=  (0x00000001 << port);
	else
		reg_port_flush.port_flush &= ~(0x00000001 << port);

	qm_register_write(reg_base_address, reg_offset, reg_size, (u32 *)&reg_port_flush);

	return MV_OK;
}

int ql_queue_length_get(int queue, u32 *length, u32 *status)
{
	u32 reg_base_address, reg_size, reg_offset;
	struct ql_qlen     reg_qlen;

	if (queue >= QM_QUEUES_NUM) {
		pr_err("Invalid queue number %d\n", queue);
		return -EINVAL;
	}

	reg_base_address =      qm.ql.qlen;
	reg_size   =   qm_reg_size.ql.qlen;
	reg_offset = qm_reg_offset.ql.qlen * queue;

	qm_register_read(reg_base_address, reg_offset, reg_size, (u32 *)&reg_qlen);

	*length = reg_qlen.reg_ql_entry.ql;
	*status = reg_qlen.reg_ql_entry.qstatus;

	return MV_OK;
}

void qm_idle_status_get(u32 *status)
{
	struct dma_idle_status reg_idle_status;
	u32 reg_base_address, reg_size, reg_offset;

	reg_base_address =      qm.dma.idle_status;
	reg_size   =   qm_reg_size.dma.idle_status;
	reg_offset = qm_reg_offset.dma.idle_status * 0;

	pr_info("\n-------------- Read DMA idle status -----------");

	qm_register_read(reg_base_address, reg_offset, reg_size, (u32 *)&reg_idle_status);

	*status = *(u32 *)&reg_idle_status;

	pr_info("\n");

	pr_info("\t idle_status.gpm_pl_cache_is_empty  = %d\n",
				((reg_idle_status.gpm_pl_cache_is_empty == 0) ? 0 : 1));
	pr_info("\t idle_status.gpm_pl_cache_is_full  = %d\n",
				((reg_idle_status.gpm_pl_cache_is_full == 0) ? 0 : 1));
	pr_info("\t idle_status.gpm_qe_cache_is_empty  = %d\n",
				((reg_idle_status.gpm_qe_cache_is_empty == 0) ? 0 : 1));
	pr_info("\t idle_status.gpm_qe_cache_is_full  = %d\n",
				((reg_idle_status.gpm_qe_cache_is_full == 0) ? 0 : 1));
	pr_info("\t idle_status.dram_pl_cache_is_empty  = %d\n",
				((reg_idle_status.dram_pl_cache_is_empty == 0) ? 0 : 1));
	pr_info("\t idle_status.dram_pl_cache_is_full  = %d\n",
				((reg_idle_status.dram_pl_cache_is_full == 0) ? 0 : 1));
	pr_info("\t idle_status.dram_qe_cache_is_empty  = %d\n",
				((reg_idle_status.dram_qe_cache_is_empty == 0) ? 0 : 1));
	pr_info("\t idle_status.dram_qe_cache_is_full  = %d\n",
				((reg_idle_status.dram_qe_cache_is_full == 0) ? 0 : 1));
	pr_info("\t idle_status.dram_fifo_is_empty  = %d\n",
				((reg_idle_status.dram_fifo_is_empty == 0) ? 0 : 1));
	pr_info("\t idle_status.mac_axi_enq_channel_is_empty  = %d\n",
				((reg_idle_status.mac_axi_enq_channel_is_empty == 0) ? 0 : 1));
	pr_info("\t idle_status.NSS_axi_enq_channel_is_empty  = %d\n",
				((reg_idle_status.NSS_axi_enq_channel_is_empty == 0) ? 0 : 1));
	pr_info("\t idle_status.gpm_ppe_read_fifo_is_empty  = %d\n",
				((reg_idle_status.gpm_ppe_read_fifo_is_empty == 0) ? 0 : 1));
	pr_info("\t idle_status.ppe_gpm_pl_write_fifo_is_empty  = %d\n",
				((reg_idle_status.ppe_gpm_pl_write_fifo_is_empty == 0) ? 0 : 1));
	pr_info("\t idle_status.ppe_gpm_qe_write_fifo_is_empty  = %d\n",
				((reg_idle_status.ppe_gpm_qe_write_fifo_is_empty == 0) ? 0 : 1));
	pr_info("\t idle_status.ppe_ru_read_fifo_is_empty  = %d\n",
				((reg_idle_status.ppe_ru_read_fifo_is_empty == 0) ? 0 : 1));
	pr_info("\t idle_status.ppe_ru_write_fifo_is_empty  = %d\n",
				((reg_idle_status.ppe_ru_write_fifo_is_empty == 0) ? 0 : 1));
	pr_info("\t idle_status.ru_ppe_read_fifo_is_empty  = %d\n",
				((reg_idle_status.ru_ppe_read_fifo_is_empty == 0) ? 0 : 1));
	pr_info("\t idle_status.dram_fifo_fsm_state_is_idle  = %d\n",
				((reg_idle_status.dram_fifo_fsm_state_is_idle == 0) ? 0 : 1));
	pr_info("\t idle_status.qeram_init_fsm_state_is_idle  = %d\n",
				((reg_idle_status.qeram_init_fsm_state_is_idle == 0) ? 0 : 1));
}

/* QM Debug functions */
void qm_errors_dump(void)
{
	u32 reg_base_address, reg_size, reg_offset, reg_data;

	pr_info("\n-------------- QL errors dump (0x%x) -----------\n", qm.ql.base);

	reg_base_address =      qm.ql.ECC_error_cause;
	reg_size   =   qm_reg_size.ql.ECC_error_cause;
	reg_offset = qm_reg_offset.ql.ECC_error_cause * 0;

	qm_register_read(reg_base_address, reg_offset, reg_size, &reg_data);
	mv_pp3_qm_reg_print("ECC ERROR CAUSE", (reg_base_address + reg_offset) & 0xFFFF, reg_data);

	reg_base_address =      qm.ql.Internal_error_cause;
	reg_size   =   qm_reg_size.ql.Internal_error_cause;
	reg_offset = qm_reg_offset.ql.Internal_error_cause * 0;

	qm_register_read(reg_base_address, reg_offset, reg_size, &reg_data);
	mv_pp3_qm_reg_print("INTERNAL ERROR CAUSE", (reg_base_address + reg_offset) & 0xFFFF, reg_data);

	pr_info("\n-------------- PFE errors dump (0x%x) -----------\n", qm.pfe.base);

	reg_base_address =      qm.pfe.ecc_error_cause;
	reg_size   =   qm_reg_size.pfe.ecc_error_cause;
	reg_offset = qm_reg_offset.pfe.ecc_error_cause * 0;

	qm_register_read(reg_base_address, reg_offset, reg_size, &reg_data);
	mv_pp3_qm_reg_print("ECC ERROR CAUSE", (reg_base_address + reg_offset) & 0xFFFF, reg_data);

	reg_base_address =      qm.pfe.internal_error_cause;
	reg_size   =   qm_reg_size.pfe.internal_error_cause;
	reg_offset = qm_reg_offset.pfe.internal_error_cause * 0;

	qm_register_read(reg_base_address, reg_offset, reg_size, &reg_data);
	mv_pp3_qm_reg_print("INTERNAL ERROR CAUSE", (reg_base_address + reg_offset) & 0xFFFF, reg_data);

	pr_info("\n-------------- DMA errors dump (0x%x) -----------\n", qm.dma.base);

	reg_base_address =      qm.dma.ecc_error_cause;
	reg_size   =   qm_reg_size.dma.ecc_error_cause;
	reg_offset = qm_reg_offset.dma.ecc_error_cause * 0;

	qm_register_read(reg_base_address, reg_offset, reg_size, &reg_data);
	mv_pp3_qm_reg_print("ECC ERROR CAUSE", (reg_base_address + reg_offset) & 0xFFFF, reg_data);

	reg_base_address =      qm.dma.internal_error_cause;
	reg_size   =   qm_reg_size.dma.internal_error_cause;
	reg_offset = qm_reg_offset.dma.internal_error_cause * 0;

	qm_register_read(reg_base_address, reg_offset, reg_size, &reg_data);
	mv_pp3_qm_reg_print("INTERNAL ERROR CAUSE", (reg_base_address + reg_offset) & 0xFFFF, reg_data);

	pr_info("\n-------------- DQF errors dump (0x%x) -----------\n", qm.dqf.base);

	reg_base_address =      qm.dqf.dqf_itnr_cause;
	reg_size   =   qm_reg_size.dqf.dqf_itnr_cause;
	reg_offset = qm_reg_offset.dqf.dqf_itnr_cause * 0;

	qm_register_read(reg_base_address, reg_offset, reg_size, &reg_data);
	mv_pp3_qm_reg_print("DQF INTERRUPT CAUSE", (reg_base_address + reg_offset) & 0xFFFF, reg_data);

	reg_base_address =      qm.dqf.dqf_ser_summary_intr_cause;
	reg_size   =   qm_reg_size.dqf.dqf_ser_summary_intr_cause;
	reg_offset = qm_reg_offset.dqf.dqf_ser_summary_intr_cause * 0;

	qm_register_read(reg_base_address, reg_offset, reg_size, &reg_data);
	mv_pp3_qm_reg_print("INTERNAL SER SUMMARY CAUSE", (reg_base_address + reg_offset) & 0xFFFF, reg_data);

	reg_base_address =      qm.dqf.write_to_full_error_intr_cause;
	reg_size   =   qm_reg_size.dqf.write_to_full_error_intr_cause;
	reg_offset = qm_reg_offset.dqf.write_to_full_error_intr_cause * 0;

	qm_register_read(reg_base_address, reg_offset, reg_size, &reg_data);
	mv_pp3_qm_reg_print("WRITE TO FULL ERROR CAUSE", (reg_base_address + reg_offset) & 0xFFFF, reg_data);

	reg_base_address =      qm.dqf.read_from_empty_error_intr_cause;
	reg_size   =   qm_reg_size.dqf.read_from_empty_error_intr_cause;
	reg_offset = qm_reg_offset.dqf.read_from_empty_error_intr_cause * 0;

	qm_register_read(reg_base_address, reg_offset, reg_size, &reg_data);
	mv_pp3_qm_reg_print("READ FROM EMPTY ERROR CAUSE", (reg_base_address + reg_offset) & 0xFFFF, reg_data);

	reg_base_address =      qm.dqf.wrong_axi_rd_error_intr_cause;
	reg_size   =   qm_reg_size.dqf.wrong_axi_rd_error_intr_cause;
	reg_offset = qm_reg_offset.dqf.wrong_axi_rd_error_intr_cause * 0;

	qm_register_read(reg_base_address, reg_offset, reg_size, &reg_data);
	mv_pp3_qm_reg_print("WRONG AXI RD ERROR CAUSE", (reg_base_address + reg_offset) & 0xFFFF, reg_data);

	pr_info("\n-------------- REORDER errors dump (0x%x) -----------\n", qm.reorder.base);

	reg_base_address =      qm.reorder.ru_ser_error_cause;
	reg_size   =   qm_reg_size.reorder.ru_ser_error_cause;
	reg_offset = qm_reg_offset.reorder.ru_ser_error_cause * 0;

	qm_register_read(reg_base_address, reg_offset, reg_size, &reg_data);
	mv_pp3_qm_reg_print("RU SER ERROR CAUSE", (reg_base_address + reg_offset) & 0xFFFF, reg_data);
}

void qm_global_dump(void)
{
	u32 reg_data;
	u32 reg_base_address, reg_size, reg_offset;
	u32 t;

	pr_info("\n-------------- QL global registers dump (0x%x)-----------\n", qm.ql.base);

	for (t = 0; t < QM_THR_PROFILE_INVALID; t++) {

		pr_info("\n-------------- Profile %d ---------------\n", t);
		reg_base_address =      qm.ql.low_threshold;
		reg_size   =   qm_reg_size.ql.low_threshold;
		reg_offset = qm_reg_offset.ql.low_threshold * t;
		qm_register_read(reg_base_address, reg_offset, reg_size, (u32 *)&reg_data);
		mv_pp3_qm_reg_print("LOW THRESHOLD", (reg_base_address + reg_offset) & 0xFFFF, reg_data);

		reg_base_address =      qm.ql.pause_threshold;
		reg_size   =   qm_reg_size.ql.pause_threshold;
		reg_offset = qm_reg_offset.ql.pause_threshold * t;
		qm_register_read(reg_base_address, reg_offset, reg_size, &reg_data);
		mv_pp3_qm_reg_print("PAUSE THRESHOLD", (reg_base_address + reg_offset) & 0xFFFF, reg_data);

		reg_base_address =      qm.ql.high_threshold;
		reg_size   =   qm_reg_size.ql.high_threshold;
		reg_offset = qm_reg_offset.ql.high_threshold * t;
		qm_register_read(reg_base_address, reg_offset, reg_size, &reg_data);
		mv_pp3_qm_reg_print("HIGH THRESHOLD", (reg_base_address + reg_offset) & 0xFFFF, reg_data);

		reg_base_address =      qm.ql.traffic_source;
		reg_size   =   qm_reg_size.ql.traffic_source;
		reg_offset = qm_reg_offset.ql.traffic_source * t;
		qm_register_read(reg_base_address, reg_offset, reg_size, &reg_data);
		mv_pp3_qm_reg_print("TRAFFIC SOURCE", (reg_base_address + reg_offset) & 0xFFFF, reg_data);
	}

	pr_info("\n-------------- PFE global registers dump (0x%x)-----------\n", qm.pfe.base);

	reg_base_address =      qm.pfe.qece_dram_base_address_hi;
	reg_size   =   qm_reg_size.pfe.qece_dram_base_address_hi;
	reg_offset = qm_reg_offset.pfe.qece_dram_base_address_hi * 0;
	qm_register_read(reg_base_address, reg_offset, reg_size, &reg_data);
	mv_pp3_qm_reg_print("QE_CE DRAM BASE HIGH", (reg_base_address + reg_offset) & 0xFFFF, reg_data);

	reg_base_address =      qm.pfe.pyld_dram_base_address_hi;
	reg_size   =   qm_reg_size.pfe.pyld_dram_base_address_hi;
	reg_offset = qm_reg_offset.pfe.pyld_dram_base_address_hi * 0;
	qm_register_read(reg_base_address, reg_offset, reg_size, &reg_data);
	mv_pp3_qm_reg_print("PAYLOAD DRAM BASE HIGH", (reg_base_address + reg_offset) & 0xFFFF, reg_data);

	reg_base_address =      qm.pfe.qece_dram_base_address_lo;
	reg_size   =   qm_reg_size.pfe.qece_dram_base_address_lo;
	reg_offset = qm_reg_offset.pfe.qece_dram_base_address_lo * 0;
	qm_register_read(reg_base_address, reg_offset, reg_size, &reg_data);
	mv_pp3_qm_reg_print("QE_CE DRAM BASE LOW", (reg_base_address + reg_offset) & 0xFFFF, reg_data);

	reg_base_address =      qm.pfe.pyld_dram_base_address_lo;
	reg_size   =   qm_reg_size.pfe.pyld_dram_base_address_lo;
	reg_offset = qm_reg_offset.pfe.pyld_dram_base_address_lo * 0;
	qm_register_read(reg_base_address, reg_offset, reg_size, &reg_data);
	mv_pp3_qm_reg_print("PAYLOAD DRAM BASE LOW", (reg_base_address + reg_offset) & 0xFFFF, reg_data);

	reg_base_address =      qm.pfe.QM_VMID;
	reg_size   =   qm_reg_size.pfe.QM_VMID;
	reg_offset = qm_reg_offset.pfe.QM_VMID * 0;
	qm_register_read(reg_base_address, reg_offset, reg_size, &reg_data);
	mv_pp3_qm_reg_print("QM VMID", (reg_base_address + reg_offset) & 0xFFFF, reg_data);

	reg_base_address =      qm.pfe.port_flush;
	reg_size   =   qm_reg_size.pfe.port_flush;
	reg_offset = qm_reg_offset.pfe.port_flush * 0;
	qm_register_read(reg_base_address, reg_offset, reg_size, &reg_data);
	mv_pp3_qm_reg_print("PORT FLUSH", (reg_base_address + reg_offset) & 0xFFFF, reg_data);

	reg_base_address =      qm.pfe.AXI_read_attributes_for_swf_mode;
	reg_size   =   qm_reg_size.pfe.AXI_read_attributes_for_swf_mode;
	reg_offset = qm_reg_offset.pfe.AXI_read_attributes_for_swf_mode * 0;

	qm_register_read(reg_base_address, reg_offset, reg_size, &reg_data);
	mv_pp3_qm_reg_print("AXI READ ATTR SWF MODE", (reg_base_address + reg_offset) & 0xFFFF, reg_data);

	reg_base_address =      qm.pfe.AXI_read_attributes_for_rdma_mode;
	reg_size   =   qm_reg_size.pfe.AXI_read_attributes_for_rdma_mode;
	reg_offset = qm_reg_offset.pfe.AXI_read_attributes_for_rdma_mode * 0;

	qm_register_read(reg_base_address, reg_offset, reg_size, &reg_data);
	mv_pp3_qm_reg_print("AXI READ ATTR RDMA MODE", (reg_base_address + reg_offset) & 0xFFFF, reg_data);

	reg_base_address =      qm.pfe.AXI_read_attributes_for_hwf_qece;
	reg_size   =   qm_reg_size.pfe.AXI_read_attributes_for_hwf_qece;
	reg_offset = qm_reg_offset.pfe.AXI_read_attributes_for_hwf_qece * 0;

	qm_register_read(reg_base_address, reg_offset, reg_size, &reg_data);
	mv_pp3_qm_reg_print("AXI READ ATTR HWF MODE QECE", (reg_base_address + reg_offset) & 0xFFFF, reg_data);

	reg_base_address =      qm.pfe.AXI_read_attributes_for_hwf_pyld;
	reg_size   =   qm_reg_size.pfe.AXI_read_attributes_for_hwf_pyld;
	reg_offset = qm_reg_offset.pfe.AXI_read_attributes_for_hwf_pyld * 0;

	qm_register_read(reg_base_address, reg_offset, reg_size, &reg_data);
	mv_pp3_qm_reg_print("AXI READ ATTR HWF MODE PYLD", (reg_base_address + reg_offset) & 0xFFFF, reg_data);

	pr_info("\n-------------- DMA global registers dump (0x%x) -----------\n", qm.dma.base);

	reg_base_address =      qm.dma.gpm_thresholds;
	reg_size   =   qm_reg_size.dma.gpm_thresholds;
	reg_offset = qm_reg_offset.dma.gpm_thresholds * 0;

	qm_register_read(reg_base_address, reg_offset, reg_size, &reg_data);
	mv_pp3_qm_reg_print("GPM THRESHOLD", (reg_base_address + reg_offset) & 0xFFFF, reg_data);

	reg_base_address =      qm.dma.dram_thresholds;
	reg_size   =   qm_reg_size.dma.dram_thresholds;
	reg_offset = qm_reg_offset.dma.dram_thresholds * 0;

	qm_register_read(reg_base_address, reg_offset, reg_size, &reg_data);
	mv_pp3_qm_reg_print("DRAM THRESHOLD", (reg_base_address + reg_offset) & 0xFFFF, reg_data);

	reg_base_address =      qm.dma.AXI_write_attributes_for_swf_mode;
	reg_size   =   qm_reg_size.dma.AXI_write_attributes_for_swf_mode;
	reg_offset = qm_reg_offset.dma.AXI_write_attributes_for_swf_mode * 0;

	qm_register_read(reg_base_address, reg_offset, reg_size, &reg_data);
	mv_pp3_qm_reg_print("AXI WRITE ATTR SWF MODE", (reg_base_address + reg_offset) & 0xFFFF, reg_data);

	reg_base_address =      qm.dma.AXI_write_attributes_for_rdma_mode;
	reg_size   =   qm_reg_size.dma.AXI_write_attributes_for_rdma_mode;
	reg_offset = qm_reg_offset.dma.AXI_write_attributes_for_rdma_mode * 0;

	qm_register_read(reg_base_address, reg_offset, reg_size, &reg_data);
	mv_pp3_qm_reg_print("AXI READ ATTR RDMA MODE", (reg_base_address + reg_offset) & 0xFFFF, reg_data);

	reg_base_address =      qm.dma.AXI_write_attributes_for_hwf_qece;
	reg_size   =   qm_reg_size.dma.AXI_write_attributes_for_hwf_qece;
	reg_offset = qm_reg_offset.dma.AXI_write_attributes_for_hwf_qece * 0;

	qm_register_read(reg_base_address, reg_offset, reg_size, &reg_data);
	mv_pp3_qm_reg_print("AXI READ ATTR HWF MODE QECE", (reg_base_address + reg_offset) & 0xFFFF, reg_data);

	reg_base_address =      qm.dma.AXI_write_attributes_for_hwf_pyld;
	reg_size   =   qm_reg_size.dma.AXI_write_attributes_for_hwf_pyld;
	reg_offset = qm_reg_offset.dma.AXI_write_attributes_for_hwf_pyld * 0;

	qm_register_read(reg_base_address, reg_offset, reg_size, &reg_data);
	mv_pp3_qm_reg_print("AXI READ ATTR HWF MODE PYLD", (reg_base_address + reg_offset) & 0xFFFF, reg_data);

	reg_base_address =      qm.dma.DRAM_VMID;
	reg_size   =   qm_reg_size.dma.DRAM_VMID;
	reg_offset = qm_reg_offset.dma.DRAM_VMID * 0;

	qm_register_read(reg_base_address, reg_offset, reg_size, &reg_data);
	mv_pp3_qm_reg_print("DRAM VMID", (reg_base_address + reg_offset) & 0xFFFF, reg_data);

	reg_base_address =      qm.dma.idle_status;
	reg_size   =   qm_reg_size.dma.idle_status;
	reg_offset = qm_reg_offset.dma.idle_status * 0;

	qm_register_read(reg_base_address, reg_offset, reg_size, &reg_data);
	mv_pp3_qm_reg_print("IDLE STATUS", (reg_base_address + reg_offset) & 0xFFFF, reg_data);

	pr_info("\n-------------- REORDER global registers dump (0x%x) -----------\n", qm.reorder.base);

	reg_base_address =      qm.reorder.ru_pool;
	reg_size   =   qm_reg_size.reorder.ru_pool;
	reg_offset = qm_reg_offset.reorder.ru_pool * 0;

	qm_register_read(reg_base_address, reg_offset, reg_size, &reg_data);
	mv_pp3_qm_reg_print("RU POOL 0", (reg_base_address + reg_offset) & 0xFFFF, reg_data);

	reg_base_address =      qm.reorder.ru_pool;
	reg_size   =   qm_reg_size.reorder.ru_pool;
	reg_offset = qm_reg_offset.reorder.ru_pool * 1;

	qm_register_read(reg_base_address, reg_offset, reg_size, &reg_data);
	mv_pp3_qm_reg_print("RU POOL 1", (reg_base_address + reg_offset) & 0xFFFF, reg_data);

	for (t = 0; t <= QM_CLASS_MAX; t++) {
		char r_name[20];
		reg_base_address =      qm.reorder.ru_class_head;
		reg_size   =   qm_reg_size.reorder.ru_class_head;
		reg_offset = qm_reg_offset.reorder.ru_class_head * t;

		qm_register_read(reg_base_address, reg_offset, reg_size, &reg_data);
		sprintf(r_name, "RU CLASS HEAD %d", t);
		mv_pp3_qm_reg_print(r_name, (reg_base_address + reg_offset) & 0xFFFF, reg_data);
	}
}

void qm_queue_dump(int queue)
{
	u32 reg_data;
	u32 reg_base_address, reg_size, reg_offset;

	if (queue >= QM_QUEUES_NUM) {
		pr_err("Invalid queue number %d\n", queue);
		return;
	}

	pr_info("\n-------------- QM queue dump for queue %d -----------\n", queue);
	pr_info("\n-------------- QL queue registers -----------\n");

	reg_base_address =      qm.ql.qptr;
	reg_size   =   qm_reg_size.ql.qptr;
	reg_offset = qm_reg_offset.ql.qptr * queue/8;

	qm_register_read(reg_base_address, reg_offset, reg_size, &reg_data);
	reg_data = (reg_data  >> (queue % 8) & 7);
	mv_pp3_qm_reg_print("QUEUE PTR", (reg_base_address + reg_offset) & 0xFFFF, reg_data);

	reg_base_address =      qm.ql.qlen;
	reg_size   =   qm_reg_size.ql.qlen;
	reg_offset = qm_reg_offset.ql.qlen * queue;

	qm_register_read(reg_base_address, reg_offset, reg_size, (u32 *)&reg_data);
	mv_pp3_qm_reg_print("QUEUE LENGTH AND STATUS", (reg_base_address + reg_offset) & 0xFFFF, reg_data);

	pr_info("\n-------------- PFE queue dump -----------\n");

	reg_base_address =      qm.pfe.queue_flush;
	reg_size   =   qm_reg_size.pfe.queue_flush;
	reg_offset = qm_reg_offset.pfe.queue_flush * (queue/32); /* each queue is represented by one bit*/

	qm_register_read(reg_base_address, reg_offset, reg_size, &reg_data);
	reg_data = (reg_data  >> (queue % 32) & 1);
	mv_pp3_qm_reg_print("QUEUE FLUSH", (reg_base_address + reg_offset) & 0xFFFF, reg_data);

	pr_info("\n-------------- DMA queue dump -----------\n");

	reg_base_address =      qm.dma.Q_memory_allocation;
	reg_size   =   qm_reg_size.dma.Q_memory_allocation;
	reg_offset = qm_reg_offset.dma.Q_memory_allocation * (queue/32); /* each queue is represented by one bit*/

	qm_register_read(reg_base_address, reg_offset, reg_size, &reg_data);
	reg_data = (reg_data  >> (queue % 32) & 1);
	mv_pp3_qm_reg_print("QUEUE MEM ALLOCATION", (reg_base_address + reg_offset) & 0xFFFF, reg_data);
}

static const char *qm_ql_bp_status_str(enum mv_qm_ql_bp_status status)
{
	const char *str;

	switch (status) {
	case QM_BP_XON:
		str = "XON";
		break;
	case QM_BP_PAUSE:
		str = "PAUSE";
		break;
	case QM_BP_XOFF:
		str = "XOFF";
		break;
	default:
		str = "Unknown";
	}
	return str;
}

struct qm_queue_len_info {
	int queue;
	struct ql_qlen       reg_qlen;
	struct ql_bp_qlen    reg_ql_bp_qlen;
	struct ql_drop_qlen  reg_ql_drop_qlen;
};

void qm_nempty_queue_len_dump(void)
{
	int queue, nempty = 0;
	bool dump;
	char bp_str[10];
	struct qm_queue_len_info *qlen_array, *qlen_info;

	qlen_array = kzalloc(QM_QUEUES_NUM * sizeof(struct qm_queue_len_info), GFP_KERNEL);
	if (qlen_array == NULL) {
		pr_err("%s: Can't allocate %d bytes. first=%d\n", __func__,
			QM_QUEUES_NUM * sizeof(struct qm_queue_len_info), 0);
		return;
	}
	/* First of all read queues registers from QM without print */
	for (queue = 0; queue < QM_QUEUES_NUM; queue++) {
		qlen_info = &qlen_array[nempty];

		qm_register_read(qm.ql.qlen, qm_reg_offset.ql.qlen * queue,
				qm_reg_size.ql.qlen, (u32 *)&qlen_info->reg_qlen);

		qm_register_read(qm.ql.bp_qlen, qm_reg_offset.ql.bp_qlen * queue,
				qm_reg_size.ql.bp_qlen, (u32 *)&qlen_info->reg_ql_bp_qlen);

		qm_register_read(qm.ql.drop_qlen, qm_reg_offset.ql.drop_qlen * queue,
				qm_reg_size.ql.drop_qlen, (u32 *)&qlen_info->reg_ql_drop_qlen);

		dump =  qlen_info->reg_qlen.reg_ql_entry.ql ||
			qlen_info->reg_ql_bp_qlen.reg_ql_bp_qlen_entry.ql ||
			qlen_info->reg_ql_drop_qlen.reg_ql_drop_qlen_entry.ql;

		if (dump) {
			qlen_info->queue = queue;
			nempty++;
		}
	}
	pr_info("\n");
	pr_info("Queue | length[16B] | bp_len[128B] (mode)   | drop_len[128B]\n");

	for (queue = 0; queue < nempty; queue++) {
		qlen_info = &qlen_array[queue];

		sprintf(bp_str, "(%s)",
			qm_ql_bp_status_str(qlen_info->reg_ql_bp_qlen.reg_ql_bp_qlen_entry.qstatus & 0x3));
		pr_info("%3d   | 0x%08X  | 0x%08X %-9s  | 0x%08X\n",
			qlen_info->queue, qlen_info->reg_qlen.reg_ql_entry.ql,
			qlen_info->reg_ql_bp_qlen.reg_ql_bp_qlen_entry.ql, bp_str,
			qlen_info->reg_ql_drop_qlen.reg_ql_drop_qlen_entry.ql);
	}
	kfree(qlen_array);
}

void qm_dqf_port_dump(int port)
{
	u32 reg_base_address, reg_size, reg_offset;
	u32 reg_data;

	if (port >= QM_PORTS_NUM) {
		pr_err("Invalid port number %d\n", port);
		return;
	}

	pr_info("\n-------------- DQF port dump for port %d -----------\n", port);

	reg_base_address =      qm.dqf.Data_FIFO_params_p;
	reg_size   =   qm_reg_size.dqf.Data_FIFO_params_p;
	reg_offset = qm_reg_offset.dqf.Data_FIFO_params_p * port;

	qm_register_read(reg_base_address, reg_offset, reg_size, &reg_data);
	mv_pp3_qm_reg_print("DATA FIFO PARAMS", (reg_base_address + reg_offset) & 0xFFFF, reg_data);

	reg_base_address =      qm.dqf.Credit_Threshold_p;
	reg_size   =   qm_reg_size.dqf.Credit_Threshold_p;
	reg_offset = qm_reg_offset.dqf.Credit_Threshold_p * port;

	qm_register_read(reg_base_address, reg_offset, reg_size, &reg_data);
	mv_pp3_qm_reg_print("CREDIT THRESHOLD", (reg_base_address + reg_offset) & 0xFFFF, reg_data);

	reg_base_address =      qm.dqf.PPC_port_map_p;
	reg_size   =   qm_reg_size.dqf.PPC_port_map_p;
	reg_offset = qm_reg_offset.dqf.PPC_port_map_p * port;

	qm_register_read(reg_base_address, reg_offset, reg_size, &reg_data);
	mv_pp3_qm_reg_print("PPC_PORT_MAP", (reg_base_address + reg_offset) & 0xFFFF, reg_data);

	reg_base_address =      qm.dqf.data_fifo_pointers_p;
	reg_size   =   qm_reg_size.dqf.data_fifo_pointers_p;
	reg_offset = qm_reg_offset.dqf.data_fifo_pointers_p * port;

	qm_register_read(reg_base_address, reg_offset, reg_size, &reg_data);
	mv_pp3_qm_reg_print("DATA FIFO PTRS", (reg_base_address + reg_offset) & 0xFFFF, reg_data);
}

static const char *qm_ql_profile_source_str(enum mv_qm_thr_profiles profile)
{
	const char *source_str;

	switch (profile) {
	case QM_THR_PROFILE_EMAC_0:
		source_str = "EMAC0";
		break;
	case QM_THR_PROFILE_EMAC_1:
		source_str = "EMAC1";
		break;
	case QM_THR_PROFILE_EMAC_2:
		source_str = "EMAC2";
		break;
	case QM_THR_PROFILE_EMAC_3:
		source_str = "EMAC3";
		break;
	case QM_THR_PROFILE_HMAC:
		source_str = "HMAC";
		break;
	default:
		source_str = "Unknown";
	}
	return source_str;
}

void qm_ql_profile_show(enum mv_qm_thr_profiles profile)
{
	enum mv_qm_thr_profiles q_profile;
	struct ql_low_threshold		reg_low_threshold;
	struct ql_pause_threshold	reg_pause_threshold;
	struct ql_high_threshold	reg_high_threshold;
	u32 reg_offset, reg_data;
	int queue, count = 0;

	reg_offset = qm_reg_offset.ql.low_threshold * profile;
	qm_register_read(qm.ql.low_threshold, reg_offset, 1, (u32 *)&reg_low_threshold);

	reg_offset = qm_reg_offset.ql.pause_threshold * profile;
	qm_register_read(qm.ql.pause_threshold, reg_offset, 1, (u32 *)&reg_pause_threshold);

	reg_offset = qm_reg_offset.ql.high_threshold * profile;
	qm_register_read(qm.ql.high_threshold, reg_offset, 1, (u32 *)&reg_high_threshold);

	reg_offset = qm_reg_offset.ql.traffic_source * profile;
	qm_register_read(qm.ql.traffic_source, reg_offset, 1, &reg_data);

	pr_info("\n-------------- QM QL profile %d -----------", profile);
	pr_info("Low threshold [KB]      : %d\n", (reg_low_threshold.low_threshold * MV_PP3_QM_UNITS) / 1024);
	pr_info("Pause threshold [KB]    : %d\n", (reg_pause_threshold.pause_threshold * MV_PP3_QM_UNITS) / 1024);
	pr_info("High threshold [KB]     : %d\n", (reg_high_threshold.high_threshold * MV_PP3_QM_UNITS) / 1024);
	pr_info("Traffic source          : %s (%d)\n", qm_ql_profile_source_str(reg_data), reg_data);

	pr_info("Connected queues list   :\n");
	for (queue = 0; queue < QM_QUEUES_NUM; queue++) {
		qm_ql_q_profile_get(queue, &q_profile);

		if (q_profile == (profile + 1)) {
			pr_cont(" %3d", queue);
			count++;

			if ((count % 8) == 0)
				pr_info("\n");
		}
	}

	pr_info("\n");
}

/* global HW work around */
static void qm_hw_internal_regs_set(void)
{	u32 val;

	/* DQF port 14 data fifo prameters registers use for global WA */
	val = 0x200000;
	qm_register_write(qm.dqf.base + 0x38, 0, 1, &val);

	/* DQF port 15 data fifo prameters registers use for global WA */
	val = 0x4030000;
	qm_register_write(qm.dqf.base + 0x3C, 0, 1, &val);
}


/*
 clear HW configuration
	- disable all queues threshold profile
	- disable all backpressure profiles
	- disable drop prots
	- dsconnect PPCs from all ports
	- zeroed ports fifo parameters (depth = 0 and base = 0)
	- clear ports credit threshold
*/
void qm_clear_hw_config(void)
{
	int i;
	/*
	 disable for all queues
		1. threshold profiles
		2. assign internal back pressure to group 0 - not in use
	*/
	for (i = 0; i < QM_QUEUES_NUM;  i++) {
		qm_ql_q_profile_set(i, QM_THR_PROFILE_INVALID);
		qm_ql_queue_bpi_group_set(i, QM_BP_INVALID_GROUP);
	}

	/* disable back pressure groups */
	for (i = 0; i < QM_BPI_GROUPS; i++)
		qm_ql_rule_bpi_disable(i);

	/* clear mac ingress hw queue number to stop */
	/* TODO - define MAC number */
	for (i = 0; i < 7; i++) {
		qm_xoff_emac_qnum_set(i, 0);
		qm_tail_ptr_mode(i, false);
	}

	qm_xoff_hmac_qs_set(0, 0);

	for (i = 0; i < QM_PORTS_NUM; i++) {
		/* disable drop mode for all prots */
		qm_port_drop_mode_set(i, false);

		/* disconnect PPCs from ports */
		qm_dqf_port_ppc_map_set(i, 0);

		/* clear port dequeue fifo parameters */
		qm_dqf_port_data_fifo_params_set(i, 0, 0);

		/* clear port dequeue fifo credit threshold */
		qm_dqf_port_credit_thr_set(i, 0);

		/* clear port max credit request */
		qm_port_max_credit_request_set(i, 0);
	}

	/* clear data fifo base */
	mac_port_fifo_base = 0;
	ppc_port_fifo_base = 0;
}

/*
qm_port_default_set:
	description: config port parameters
		- drop ports
		- dequeue fifo base and depth
		- dequeue fifo credit threshold
		- how many credits needed to start reading DRAM chunk
*/

static int qm_port_default_set(int port, enum mv_tm_source_type type)
{
	switch (type) {

	case SOURCE_DROP:
		/* DQF do not aware to ports 13-15.
		   ports 13-15 registers used as NSS general debug registers */
		qm_port_drop_mode_set(port, true);
		break;
	case SOURCE_PPC_DP:
		qm_dqf_port_data_fifo_params_set(port, ppc_port_fifo_base, QM_PPC_DP_FIFO_DEPTH);
		ppc_port_fifo_base += QM_PPC_DP_FIFO_DEPTH;
		break;
	case SOURCE_PPC_MNT:
		qm_dqf_port_data_fifo_params_set(port, ppc_port_fifo_base, QM_PPC_MNT_FIFO_DEPTH);
		ppc_port_fifo_base += QM_PPC_MNT_FIFO_DEPTH;
		break;
	case SOURCE_EMAC:
		qm_dqf_port_data_fifo_params_set(port, mac_port_fifo_base, QM_EMAC_FIFO_DEPTH(port));
		mac_port_fifo_base += QM_EMAC_FIFO_DEPTH(port);
		qm_dqf_port_credit_thr_set(port, QM_EMAC_CREDIT_THR);
		qm_port_max_credit_request_set(port, QM_EMAC_REQ_MAX_CREDIT(port));
		break;
	case SOURCE_CMAC:
		qm_dqf_port_data_fifo_params_set(port, mac_port_fifo_base, QM_CMAC_FIFO_DEPTH(port));
		mac_port_fifo_base += QM_CMAC_FIFO_DEPTH(port);
		qm_dqf_port_credit_thr_set(port, QM_CMAC_CREDIT_THR(port));
		qm_port_max_credit_request_set(port, QM_CMAC_REQ_MAX_CREDIT(port));
		break;
	case SOURCE_HMAC:
		qm_dqf_port_data_fifo_params_set(port, mac_port_fifo_base, QM_HMAC_FIFO_DEPTH);
		mac_port_fifo_base += QM_HMAC_FIFO_DEPTH;
		qm_dqf_port_credit_thr_set(port, QM_HMAC_CREDIT_THR);
		break;
	default:
		pr_err("%s: invalid port number %d\n", __func__, port);
		return -EINVAL;
	}

	return 0;
}

int qm_ru_port_to_class_set(int port, int class, int sid_pool)
{
	u32 port2class_entry;

	if (port > QM_INPUT_PORT_MAX) {
		pr_err("%s: Invalid port number %d\n",  __func__, port);
		return -EINVAL;
	}

	if (class > QM_CLASS_MAX) {
		pr_err("%s: Invalid class number %d\n", __func__, class);
		return -EINVAL;
	}

	if (sid_pool > QM_SID_POOL_MAX) {
		pr_err("%s: Invalid pool number %d\n",  __func__, sid_pool);
		return -EINVAL;
	}

	port2class_entry = class | (sid_pool << 6);
	qm_register_write(qm.reorder.ru_port2class, port * 4, 1, &port2class_entry);
	return 0;
}

/*
 qm_ru_port_to_class_default_set
	description: default port to class table configuration
 -------------------------------------
 |SOURCE | source port | class range |
 -------------------------------------
 | EMAC  |   0 - 5     |   0 - 5     |
 -------------------------------------
 | CMAC  |   6 - 7     |   6 - 7     |
 -------------------------------------
 | HMAC  |   8 - 71    |   8 - 47    |  Wrap around every 40
 -------------------------------------
 | FW    |   72 - 119  |   48 - 63   |  Wrap around every 16
 -------------------------------------
*/

static int qm_ru_port_to_class_default_set(void)
{
	int port, class;

	/* EMACs - one class per EMAC*/
	class = 0;
	for (port = 0; port < 6; port++) {
		if (qm_ru_port_to_class_set(port, class, 0) < 0)
			goto err;
		class++;
	}

	/* OCMAC one class per direction */
	class = 6;
	for (port = 6; port < 8; port++) {
		if (qm_ru_port_to_class_set(port, class, 0) < 0)
			goto err;
		class++;
	}

	class = 8;
	for (port = 8; port < 72; port++) {
		if (qm_ru_port_to_class_set(port, class + ((port - 8) % 40), 0) < 0)
			goto err;
	}

	class += 40;
	for (port = 72; port < 120; port++) {
		if (qm_ru_port_to_class_set(port, class + ((port - 72) % 16), 0) < 0)
			goto err;
	}

	return 0;
err:
	pr_err("%s: failed to config reorder unit\n", __func__);
	return -1;
}

/*
qm_default_set:
	description: set defaults
	parameters
		ppc_num - number of active PPCs
*/
int qm_default_set(int ppc_num)
{
	int i, ports, port_id;
	enum mv_tm_source_type type;

	/* hw WA */
	qm_hw_internal_regs_set();

	/* secret machine enable*/
	qm_xoff_to_macs_enable();

	/* Enable tail pointer insertion in CMAC CFH*/
	qm_tail_ptr_mode(QM_TAIL_PTR_CMAC_BIT, true);

	/* mapping queue to port for QM drop recommendation */
	for (i = 0; i < QM_QUEUES_NUM;  i++)
		if (mv_tm_scheme_queue_path_get(i, NULL, NULL, NULL, &port_id) == 0)
			qm_ql_queue_port_set(i, port_id);
		else
			pr_warn("%s: cannot get port id of q#%d from TM api\n", __func__, i);


	/* config ports */
	ports = mv_tm_scheme_ports_num();

	for (i = 0; i < ports; i++) {
		/* tm api return SOURCE_LAST for unused ports */
		type = mv_tm_scheme_port_to_source(i, NULL);
		if (type != SOURCE_LAST)
			if (qm_port_default_set(i, type) < 0)
				return -1;
	}

	/* Set all active PPCs to process packets from scheduler port 0 and 1 */
	port_id = mv_tm_scheme_source_to_port(SOURCE_PPC_DP, 0);
	qm_dqf_port_ppc_map_set(port_id, (1 << ppc_num) - 1);

	port_id = mv_tm_scheme_source_to_port(SOURCE_PPC_DP, 1);
	qm_dqf_port_ppc_map_set(port_id, (1 << ppc_num) - 1);

	if (qm_ru_port_to_class_default_set() < 0)
		return -1;

	return 0;
}
