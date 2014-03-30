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
Marvell Commercial License Option

If you received this File from Marvell and you have entered into a commercial
license agreement (a "Commercial License") with Marvell, the File is licensed
to you under the terms of the applicable Commercial License.

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
********************************************************************************
Marvell BSD License Option

If you received this File from Marvell, you may opt to use, redistribute and/or
modify this File under the following licensing terms.
Redistribution and use in source and binary forms, with or without modification,
are permitted provided that the following conditions are met:

    *   Redistributions of source code must retain the above copyright notice,
	this list of conditions and the following disclaimer.

    *   Redistributions in binary form must reproduce the above copyright
	notice, this list of conditions and the following disclaimer in the
	documentation and/or other materials provided with the distribution.

    *   Neither the name of Marvell nor the names of its contributors may be
	used to endorse or promote products derived from this software without
	specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

*******************************************************************************/

/* includes */
#include "common/mv_sw_if.h"
#include "common/mv_hw_if.h"
#include "qm/mv_qm.h"
#include "qm/mv_qm_regs.h"

/**
 */
int qm_open(void)
{
	int rc;

	rc = qm_reg_address_alias_init();
	if (rc != OK)
		return rc;
	rc = qm_reg_size_alias_init();
	if (rc != OK)
		return rc;
	rc = qm_reg_offset_alias_init();
/*
	if (rc != OK)
		return rc;
	rc = qm_pid_bid_init();
*/
	return rc;
}

/**
 */
int qm_close(void)
{
	int rc = OK;

	return rc;
}

/**
 */
int qm_restart(void)
{
	int rc = OK;

	return rc;
}

int qm_pfe_base_address_pool_set(u32 *qece_base_address, u32 *pyld_base_address)
{
	int rc = -QM_INPUT_NOT_IN_RANGE;
	u32 reg_base_address, reg_size, reg_offset;

	struct pfe_qece_dram_base_address_hi         reg_qece_dram_base_address_hi;
	struct pfe_qece_dram_base_address_lo         reg_qece_dram_base_address_lo;
	struct pfe_pyld_dram_base_address_hi         reg_pyld_dram_base_address_hi;
	struct pfe_pyld_dram_base_address_lo         reg_pyld_dram_base_address_lo;

	reg_base_address =      qm.pfe.qece_dram_base_address_hi;
	reg_size   =   qm_reg_size.pfe.qece_dram_base_address_hi;
	reg_offset = qm_reg_offset.pfe.qece_dram_base_address_hi * 0;

	rc = qm_register_read(reg_base_address, reg_offset, reg_size, (u32 *)&reg_qece_dram_base_address_hi);
	if (rc != OK)
		return rc;

	reg_qece_dram_base_address_hi.qece_dram_base_address_hi	 = ((struct mv_word40 *)qece_base_address)->hi;
	rc = qm_register_write(reg_base_address, reg_offset, reg_size, (u32 *)&reg_qece_dram_base_address_hi);
	if (rc != OK)
		return rc;

	reg_base_address =      qm.pfe.qece_dram_base_address_lo;
	reg_size   =   qm_reg_size.pfe.qece_dram_base_address_lo;
	reg_offset = qm_reg_offset.pfe.qece_dram_base_address_lo * 0;

	rc = qm_register_read(reg_base_address, reg_offset, reg_size, (u32 *)&reg_qece_dram_base_address_lo);
	if (rc != OK)
		return rc;

	reg_qece_dram_base_address_lo.qece_dram_base_address_low = ((struct mv_word40 *)qece_base_address)->lo;
	rc = qm_register_write(reg_base_address, reg_offset, reg_size, (u32 *)&reg_qece_dram_base_address_lo);
	if (rc != OK)
		return rc;

	reg_base_address =      qm.pfe.pyld_dram_base_address_hi;
	reg_size   =   qm_reg_size.pfe.pyld_dram_base_address_hi;
	reg_offset = qm_reg_offset.pfe.pyld_dram_base_address_hi * 0;

	rc = qm_register_read(reg_base_address, reg_offset, reg_size, (u32 *)&reg_pyld_dram_base_address_hi);
	if (rc != OK)
		return rc;
	reg_pyld_dram_base_address_hi.pyld_dram_base_address_hi	 = ((struct mv_word40 *)pyld_base_address)->hi;
	rc = qm_register_write(reg_base_address, reg_offset, reg_size, (u32 *)&reg_pyld_dram_base_address_hi);
	if (rc != OK)
		return rc;

	reg_base_address =      qm.pfe.pyld_dram_base_address_lo;
	reg_size   =   qm_reg_size.pfe.pyld_dram_base_address_lo;
	reg_offset = qm_reg_offset.pfe.pyld_dram_base_address_lo * 0;

	rc = qm_register_read(reg_base_address, reg_offset, reg_size, (u32 *)&reg_pyld_dram_base_address_lo);
	if (rc != OK)
		return rc;

	reg_pyld_dram_base_address_lo.pyld_dram_base_address_low = ((struct mv_word40 *)pyld_base_address)->lo;
	rc = qm_register_write(reg_base_address, reg_offset, reg_size, (u32 *)&reg_pyld_dram_base_address_lo);

	return rc;
}

/*int qm_enable(u32 qe_thr_hi, u32 qe_thr_lo, u32 pl_thr_hi, u32 pl_thr_lo)
int qm_enable(u32 gpm_qe_thr_hi, u32 gpm_qe_thr_lo, u32 gpm_pl_thr_hi, u32 gpm_pl_thr_lo,
			u32 dram_qe_thr_hi, u32 dram_qe_thr_lo, u32 dram_pl_thr_hi, u32 dram_pl_thr_lo)
{
	int rc = -QM_INPUT_NOT_IN_RANGE;
	u32 reg_base_address, reg_size, reg_offset;

	if ((gpm_qe_thr_hi  <  QM_GPM_QE_THR_HI_MIN) || (gpm_qe_thr_hi  >  QM_GPM_QE_THR_HI_MAX)) return rc;
	if ((gpm_qe_thr_lo  <  QM_GPM_QE_THR_LO_MIN) || (gpm_qe_thr_lo  >  QM_GPM_QE_THR_LO_MAX)) return rc;
	if ((gpm_pl_thr_hi  <  QM_GPM_PL_THR_HI_MIN) || (gpm_pl_thr_hi  >  QM_GPM_PL_THR_HI_MAX)) return rc;
	if ((gpm_pl_thr_lo  <  QM_GPM_PL_THR_LO_MIN) || (gpm_pl_thr_lo  >  QM_GPM_PL_THR_LO_MAX)) return rc;
	if ((dram_qe_thr_hi < QM_DRAM_QE_THR_HI_MIN) || (dram_qe_thr_hi > QM_DRAM_QE_THR_HI_MAX)) return rc;
	if ((dram_qe_thr_lo < QM_DRAM_QE_THR_LO_MIN) || (dram_qe_thr_lo > QM_DRAM_QE_THR_LO_MAX)) return rc;
	if ((dram_pl_thr_hi < QM_DRAM_PL_THR_HI_MIN) || (dram_pl_thr_hi > QM_DRAM_PL_THR_HI_MAX)) return rc;
	if ((dram_pl_thr_lo < QM_DRAM_PL_THR_LO_MIN) || (dram_pl_thr_lo > QM_DRAM_PL_THR_LO_MAX)) return rc;

	rc =  qm_gpm_pool_thr_set(gpm_qe_thr_hi,  gpm_qe_thr_lo,  gpm_pl_thr_hi,  gpm_pl_thr_lo); if (rc) return rc;
	rc = qm_dram_pool_thr_set(dram_qe_thr_hi, dram_qe_thr_lo, dram_pl_thr_hi, dram_pl_thr_lo); if (rc) return rc;

	return rc;
}
*/

int qm_dma_gpm_pools_def_enable(void)
{
	int rc = OK;
	u32 qece_thr_hi, qece_thr_lo, pl_thr_hi, pl_thr_lo;

	qece_thr_hi = QM_GPM_QE_THR_HI_DEF;
	qece_thr_lo = QM_GPM_QE_THR_LO_DEF;
	pl_thr_hi   = QM_GPM_PL_THR_HI_DEF;
	pl_thr_lo   = QM_GPM_PL_THR_LO_DEF;

	rc = qm_dma_gpm_pools_enable(qece_thr_hi, qece_thr_lo, pl_thr_hi, pl_thr_lo);
	return rc;
}

int qm_dma_gpm_pools_enable(u32 qece_thr_hi, u32 qece_thr_lo, u32 pl_thr_hi, u32 pl_thr_lo)
{
	int rc = -QM_INPUT_NOT_IN_RANGE;
	struct dma_gpm_thresholds reg_gpm_thresholds;	/* RW */
	u32 reg_base_address, reg_size, reg_offset;

	if ((qece_thr_hi < QM_GPM_QE_THR_HI_MIN) || (qece_thr_hi > QM_GPM_QE_THR_HI_MAX))
		return rc;
	if ((qece_thr_lo < QM_GPM_QE_THR_LO_MIN) || (qece_thr_lo > QM_GPM_QE_THR_LO_MAX))
		return rc;
	if ((pl_thr_hi   < QM_GPM_PL_THR_HI_MIN) || (pl_thr_hi   > QM_GPM_PL_THR_HI_MAX))
		return rc;
	if ((pl_thr_lo   < QM_GPM_PL_THR_LO_MIN) || (pl_thr_lo   > QM_GPM_PL_THR_LO_MAX))
		return rc;

	reg_base_address =      qm.dma.gpm_thresholds;
	reg_size   =   qm_reg_size.dma.gpm_thresholds;
	reg_offset = qm_reg_offset.dma.gpm_thresholds * 0;

	rc = qm_register_read(reg_base_address, reg_offset, reg_size, (u32 *)&reg_gpm_thresholds);
	if (rc != OK)
		return rc;
	reg_gpm_thresholds.gpm_qe_pool_low_bp  = qece_thr_lo;	/* qe_thr & 0x00FFFFFFFF; */
	reg_gpm_thresholds.gpm_qe_pool_high_bp = qece_thr_hi;	/* qe_thr >> 32; */
	reg_gpm_thresholds.gpm_pl_pool_low_bp  =   pl_thr_lo;	/* pl_thr & 0x00FFFFFFFF; */
	reg_gpm_thresholds.gpm_pl_pool_high_bp =   pl_thr_hi;	/* pl_thr >> 32; */
	rc = qm_register_write(reg_base_address, reg_offset, reg_size, (u32 *)&reg_gpm_thresholds);

	return rc;
}

int qm_dma_dram_pools_def_enable(void)
{
	int rc = OK;
	u32 qece_thr_hi, qece_thr_lo, pl_thr_hi, pl_thr_lo;

	qece_thr_hi = QM_DRAM_QE_THR_HI_DEF;
	qece_thr_lo = QM_DRAM_QE_THR_LO_DEF;
	pl_thr_hi   = QM_DRAM_PL_THR_HI_DEF;
	pl_thr_lo   = QM_DRAM_PL_THR_LO_DEF;

	rc = qm_dma_dram_pools_enable(qece_thr_hi, qece_thr_lo, pl_thr_hi, pl_thr_lo);
	return rc;
}

int qm_dma_dram_pools_enable(u32 qece_thr_hi, u32 qece_thr_lo, u32 pl_thr_hi, u32 pl_thr_lo)
{
	int rc = -QM_INPUT_NOT_IN_RANGE;
	struct dma_dram_thresholds reg_dram_thresholds;	/* RW */
	u32 reg_base_address, reg_size, reg_offset;

	if ((qece_thr_hi < QM_GPM_QE_THR_HI_MIN) || (qece_thr_hi > QM_GPM_QE_THR_HI_MAX))
		return rc;
	if ((qece_thr_lo < QM_GPM_QE_THR_LO_MIN) || (qece_thr_lo > QM_GPM_QE_THR_LO_MAX))
		return rc;
	if ((pl_thr_hi   < QM_GPM_PL_THR_HI_MIN) || (pl_thr_hi   > QM_GPM_PL_THR_HI_MAX))
		return rc;
	if ((pl_thr_lo   < QM_GPM_PL_THR_LO_MIN) || (pl_thr_lo   > QM_GPM_PL_THR_LO_MAX))
		return rc;

	reg_base_address =      qm.dma.dram_thresholds;
	reg_size   =   qm_reg_size.dma.dram_thresholds;
	reg_offset = qm_reg_offset.dma.dram_thresholds * 0;

	rc = qm_register_read(reg_base_address, reg_offset, reg_size, (u32 *)&reg_dram_thresholds);
	if (rc != OK)
		return rc;
	reg_dram_thresholds.dram_qe_pool_low_bp  = qece_thr_lo;	/* qe_thr & 0x00FFFFFFFF; */
	reg_dram_thresholds.dram_qe_pool_high_bp = qece_thr_hi;	/* qe_thr >> 32; */
	reg_dram_thresholds.dram_pl_pool_low_bp  =   pl_thr_lo;	/* pl_thr & 0x00FFFFFFFF; */
	reg_dram_thresholds.dram_pl_pool_high_bp =   pl_thr_hi;	/* pl_thr >> 32; */
	rc = qm_register_write(reg_base_address, reg_offset, reg_size, (u32 *)&reg_dram_thresholds);
	return rc;
}

int qm_dma_queue_memory_type_set(u32 queue, u32 memory_type)
{
	int rc = -QM_INPUT_NOT_IN_RANGE;
	struct dma_q_memory_allocation        reg_q_memory_allocation;
	u32 reg_base_address, reg_size, reg_offset;

	if ((queue       <       QM_QUEUE_MIN) || (queue       >       QM_QUEUE_MAX))
		return rc;
	if ((memory_type < QM_MEMORY_TYPE_MIN) || (memory_type > QM_MEMORY_TYPE_MAX))
		return rc;

	reg_base_address =      qm.dma.Q_memory_allocation;
	reg_size   =   qm_reg_size.dma.Q_memory_allocation;
	reg_offset = qm_reg_offset.dma.Q_memory_allocation * (queue/32);

	rc = qm_register_read(reg_base_address, reg_offset, reg_size, (u32 *)&reg_q_memory_allocation);
	if (rc != OK)
		return rc;

	switch (memory_type) {
	case GPM_MEMORY_TYPE:
		reg_q_memory_allocation.q_memory &= ~(0x00000001 << (queue%32));
		break;
	case DRAM_MEMORY_TYPE:
		reg_q_memory_allocation.q_memory |=  (0x00000001 << (queue%32));
		break;
	default:
		rc = -QM_WRONG_MEMORY_TYPE;
		return rc;
	}
	rc = qm_register_write(reg_base_address, reg_offset, reg_size, (u32 *)&reg_q_memory_allocation);
	return rc;
}

/*
int qm_disable(void)
TBD  ask yuval peleg defined bits to stops and bit to check if it is stopped{
	int rc = !OK;
	int reg_size;

	if (rc = dma_gpm_pool_thr_set(0, 0, 0, 0))
		return rc;
	if (rc = dma_dram_pool_thr_set(0, 0, 0, 0))

	return rc;
}
*/

int qm_packets_in_queues(u32 *status)
{
	int rc = -QM_INPUT_NOT_IN_RANGE;
	int queue;
	struct ql_qlen reg_qlen;
	u32 reg_base_address, reg_size, reg_offset;

	*status = 0;

	for (queue = 0; queue < QM_QUEUE_MAX; queue++) {
		reg_base_address =      qm.ql.qlen;
		reg_size   =   qm_reg_size.ql.qlen;
		reg_offset = qm_reg_offset.ql.qlen * queue;

		rc = qm_register_read(reg_base_address, reg_offset, reg_size, (u32 *)&reg_qlen);
		if (rc != OK)
			return rc;
		if (reg_qlen.reg_ql_entry.ql > 0)
			*status = 1;
	}

	rc = OK;
	return rc;
}

int qm_default_set(void)
{
	int rc = !OK;

	rc = qm_ru_port_to_class_def_set();
	if (rc != OK)
		return rc;
	rc = qm_ru_pool_sid_number_def_set();
	if (rc != OK)
		return rc;
	rc = qm_dqf_port_data_fifo_def_set();
	if (rc != OK)
		return rc;
	rc = qm_dqf_port_credit_thr_def_set();
	if (rc != OK)
		return rc;
	rc = qm_dqf_port_ppc_map_def_set();
	if (rc != OK)
		return rc;
	rc = qm_dma_qos_attr_def_set();
	if (rc != OK)
		return rc;
	rc = qm_dma_domain_attr_def_set();
	if (rc != OK)
		return rc;
	rc = qm_dma_cache_attr_def_set();
	if (rc != OK)
		return rc;
	rc = qm_pfe_qos_attr_def_set();
	if (rc != OK)
		return rc;
	rc = qm_pfe_domain_attr_def_set();
	if (rc != OK)
		return rc;
	rc = qm_pfe_cache_attr_def_set();
	if (rc != OK)
		return rc;
	rc = qm_ql_thr_def_set();
	if (rc != OK)
		return rc;
	rc = qm_ql_q_profile_def_set();
	if (rc != OK)
		return rc;

	return rc;
}

int qm_ru_pool_sid_number_def_set(void)
{
	int rc = !OK;
	u32 pool0_sid_num, pool1_sid_num;

	pool0_sid_num = QM_POOL0_SID_NUM_DEF;
	pool1_sid_num = QM_POOL1_SID_NUM_DEF;

	rc = qm_ru_pool_sid_number_set(pool0_sid_num, pool1_sid_num);
	return rc;
}

int qm_ru_pool_sid_number_set(u32 pool0_sid_num, u32 pool1_sid_num)
{
	int rc = -QM_INPUT_NOT_IN_RANGE;
	struct reorder_ru_pool         reg_ru_pool;
	u32 reg_base_address, reg_size, reg_offset;

	if ((pool0_sid_num < QM_POOL0_SID_NUM_MIN) || (pool0_sid_num > QM_POOL0_SID_NUM_MAX))
		return rc;
	if ((pool1_sid_num < QM_POOL1_SID_NUM_MIN) || (pool1_sid_num > QM_POOL1_SID_NUM_MAX))
		return rc;

	reg_base_address =      qm.reorder.ru_pool;
	reg_size   =   qm_reg_size.reorder.ru_pool;
	reg_offset = qm_reg_offset.reorder.ru_pool * 0;

	rc = qm_register_read(reg_base_address, reg_offset, reg_size, (u32 *)&reg_ru_pool);
	if (rc != OK)
		return rc;
	reg_ru_pool.sid_limit   = pool0_sid_num;
	rc = qm_register_write(reg_base_address, reg_offset, reg_size, (u32 *)&reg_ru_pool);
	if (rc != OK)
		return rc;

	reg_base_address =      qm.reorder.ru_pool;
	reg_size   =   qm_reg_size.reorder.ru_pool;
	reg_offset = qm_reg_offset.reorder.ru_pool * 1;

	rc = qm_register_read(reg_base_address, reg_offset, reg_size, (u32 *)&reg_ru_pool);
	if (rc != OK)
		return rc;
	reg_ru_pool.sid_limit   = pool1_sid_num;
	rc = qm_register_write(reg_base_address, reg_offset, reg_size, (u32 *)&reg_ru_pool);
	return rc;
}

int qm_ru_port_to_class_def_set(void)
{
	int rc = !OK;
	u32 port_class_arr, port_pool_arr, arrays_size, input_port;

	port_pool_arr  = QM_PORT_ARR_DEF;
	arrays_size    = QM_ARRAYS_SIZE_DEF;

	/* cMac and eMac */
	for (input_port = QM_INPUT_PORT_CMAC_EMAC_MIN; input_port <= QM_INPUT_PORT_CMAC_EMAC_MAX; input_port++) {
		port_class_arr = QM_CLASS_ARR_CMAC_EMAC_DEF;
		rc = qm_ru_port_to_class_set(&port_class_arr, &port_pool_arr, input_port);
		if (rc != OK)
			return rc;
	}

	/* hMac */
	for (input_port = QM_INPUT_PORT_HMAC_MIN; input_port <= QM_INPUT_PORT_HMAC_MAX; input_port++) {
		port_class_arr = QM_CLASS_ARR_HMAC_DEF;
		rc = qm_ru_port_to_class_set(&port_class_arr, &port_pool_arr, input_port);
		if (rc != OK)
			return rc;
	}

	/* PPC */
	for (input_port = QM_INPUT_PORT_PPC_MIN; input_port <= QM_INPUT_PORT_PPC_MAX; input_port++) {
		port_class_arr = QM_CLASS_ARR_PPC_DEF;
		rc = qm_ru_port_to_class_set(&port_class_arr, &port_pool_arr, input_port);
		if (rc != OK)
			return rc;
	}

	rc = OK;
	return rc;
}
/*
port_class_arr holds class values which are in the range 0 to 63.
Default values are:
for input port  0 to  7 (cMac and eMac) class is 0 to 7 (respectively).
For input port  8 to 71 (hMac) class is 8.
For Input port 72 to 89  (PPC) class is 9.
Port input 90 to 287 are not used so disregard their value.
Classes 10 to 63 are left unused for future use.

port_pool_arr holds pool  values which are either 0 or 1 (default values are pool 0 for all input ports)

arrays_size holds the size of each array. Size can be a value from 90
(current implementation to 288 (since input port is 0 to 287)
*/

int qm_ru_port_to_class_set(u32 *port_class_arr, u32 *port_pool_arr, u32 input_port)
{
	int rc = -QM_INPUT_NOT_IN_RANGE;
/*	struct reorder_ru_pool       reg_ru_pool;*/
	u32 reg_base_address, reg_size, reg_offset;
	struct reorder_ru_port2class reg_ru_port2class;

	if ((*port_class_arr < QM_CLASS_ARR_MIN)   || (*port_class_arr >   QM_CLASS_ARR_MAX))
		return rc;
	if ((*port_pool_arr  < QM_PORT_ARR_MIN)    || (*port_pool_arr  >    QM_PORT_ARR_MAX))
		return rc;
	if ((input_port      < QM_ARRAYS_SIZE_MIN) || (input_port      > QM_ARRAYS_SIZE_MAX))
		return rc;

	reg_base_address =      qm.reorder.ru_port2class;
	reg_size   =   qm_reg_size.reorder.ru_port2class;
	reg_offset = qm_reg_offset.reorder.ru_port2class * input_port; /* ??? */

	rc = qm_register_read(reg_base_address, reg_offset, reg_size, (u32 *)&reg_ru_port2class);
	if (rc != OK)
		return rc;
	reg_ru_port2class.ru_class = *port_class_arr;
	reg_ru_port2class.ru_pool  = *port_pool_arr;
	rc = qm_register_write(reg_base_address, reg_offset, reg_size, (u32 *)&reg_ru_port2class);
	return rc;
}

int qm_dqf_port_data_fifo_def_set(void)
{
	int rc = !OK;
	u32 port_depth_arr[QM_PORT_MAX + 1];

	port_depth_arr[0]  = QM_PORT_DEPTH_ARR_PPC0_DEF;	/* 2*144B for PPC0		*/
	port_depth_arr[1]  = QM_PORT_DEPTH_ARR_PPC1_DEF;	/* 1*144B for PPC1-mnt0	*/
	port_depth_arr[2]  = QM_PORT_DEPTH_ARR_PPC1_DEF;	/* 1*144B for PPC1-mnt1	*/
	port_depth_arr[3]  = QM_PORT_DEPTH_ARR_EMAC_DEF;	/*  2560B for eMac0		*/
	port_depth_arr[4]  = QM_PORT_DEPTH_ARR_EMAC_DEF;	/*  2560B for eMac1		*/
	port_depth_arr[5]  = QM_PORT_DEPTH_ARR_EMAC_DEF;	/*  2560B for eMac2		*/
	port_depth_arr[6]  = QM_PORT_DEPTH_ARR_EMAC_DEF;	/*  2560B for eMac3		*/
	port_depth_arr[7]  = QM_PORT_DEPTH_ARR_EMAC_DEF;	/*  2560B for eMac4		*/
	port_depth_arr[8]  = QM_PORT_DEPTH_ARR_CMAC0_DEF;	/*  2560B for cMac0		*/
	port_depth_arr[9]  = QM_PORT_DEPTH_ARR_CMAC1_DEF;	/*   512B for cMac1		*/
	port_depth_arr[10] = QM_PORT_DEPTH_ARR_HMAC_DEF;	/*   512B for hMac		*/
	port_depth_arr[11] = 0;
	port_depth_arr[12] = 0;
	port_depth_arr[13] = 0;
	port_depth_arr[14] = 0;
	port_depth_arr[15] = 0;

	rc = qm_dqf_port_data_fifo_set(port_depth_arr);
	return rc;
}

int qm_dqf_port_data_fifo_set(u32 *port_depth_arr)
{
	int rc = -QM_INPUT_NOT_IN_RANGE;
	struct dqf_Data_FIFO_params_p          reg_Data_FIFO_params_p;
	u32 reg_base_address, reg_size, reg_offset;
	u32 port, port_depth_arr_sum = 0, data_fifo_ppc_counter = 0, data_fifo_mac_counter = 0;

	for (port = QM_PORT_MIN; port <= QM_PORT_MAX; port++) {
		if ((port_depth_arr[port] % GRANULARITY_OF_16_BYTES) != 0)
			return rc;
		port_depth_arr_sum = port_depth_arr_sum + port_depth_arr[port];
		if ((port_depth_arr[port] < QM_PORT_DEPTH_ARR_MIN) || (port_depth_arr_sum > QM_PORT_DEPTH_ARR_SUM_MAX))
			return rc;
	}

	for (port = QM_PORT_MIN; port <= QM_PORT_MAX; port++) {
		reg_base_address =      qm.dqf.Data_FIFO_params_p;
		reg_size   =   qm_reg_size.dqf.Data_FIFO_params_p;
		reg_offset = qm_reg_offset.dqf.Data_FIFO_params_p * port;

		rc = qm_register_read(reg_base_address, reg_offset, reg_size, (u32 *)&reg_Data_FIFO_params_p);
		if (rc != OK)
			return rc;

		switch (port) {
		case 0:
			reg_Data_FIFO_params_p.data_fifo_base_p   = data_fifo_ppc_counter;	/*      0 */
			data_fifo_ppc_counter += (port_depth_arr[port] / QM_SIZE_OF_PORT_DEPTH_ARR_PPC_IN_BYTES);
			reg_Data_FIFO_params_p.data_fifo_depth_p  = data_fifo_ppc_counter;	/*      2 */
			break;
		case 1:
			reg_Data_FIFO_params_p.data_fifo_base_p   = data_fifo_ppc_counter;	/*      2 */
			data_fifo_ppc_counter += (port_depth_arr[port] / QM_SIZE_OF_PORT_DEPTH_ARR_PPC_IN_BYTES);
			reg_Data_FIFO_params_p.data_fifo_depth_p  = data_fifo_ppc_counter;	/*      1 */
			break;
		case 2:
			reg_Data_FIFO_params_p.data_fifo_base_p   = data_fifo_ppc_counter;	/*      3 */
			data_fifo_ppc_counter += (port_depth_arr[port] / QM_SIZE_OF_PORT_DEPTH_ARR_PPC_IN_BYTES);
			reg_Data_FIFO_params_p.data_fifo_depth_p  = data_fifo_ppc_counter;	/*      1 */
			break;
		case 3:
			reg_Data_FIFO_params_p.data_fifo_base_p   = data_fifo_mac_counter;	/*      0 */
			data_fifo_mac_counter += (port_depth_arr[port] / QM_SIZE_OF_PORT_DEPTH_ARR_MAC_IN_BYTES);
			reg_Data_FIFO_params_p.data_fifo_depth_p  = data_fifo_mac_counter;	/* 0x00A0 */
			break;
		case 4:
			reg_Data_FIFO_params_p.data_fifo_base_p   = data_fifo_mac_counter;	/* 0x00A0 */
			data_fifo_mac_counter += (port_depth_arr[port] / QM_SIZE_OF_PORT_DEPTH_ARR_MAC_IN_BYTES);
			reg_Data_FIFO_params_p.data_fifo_depth_p  = data_fifo_mac_counter;	/* 0x0140 */
			break;
		case 5:
			reg_Data_FIFO_params_p.data_fifo_base_p   = data_fifo_mac_counter;	/* 0x0140 */
			data_fifo_mac_counter += (port_depth_arr[port] / QM_SIZE_OF_PORT_DEPTH_ARR_MAC_IN_BYTES);
			reg_Data_FIFO_params_p.data_fifo_depth_p  = data_fifo_mac_counter;	/* 0x01E0 */
			break;
		case 6:
			reg_Data_FIFO_params_p.data_fifo_base_p   = data_fifo_mac_counter;	/* 0x01E0 */
			data_fifo_mac_counter += (port_depth_arr[port] / QM_SIZE_OF_PORT_DEPTH_ARR_MAC_IN_BYTES);
			reg_Data_FIFO_params_p.data_fifo_depth_p  = data_fifo_mac_counter;	/* 0x0280 */
			break;
		case 7:
			reg_Data_FIFO_params_p.data_fifo_base_p   = data_fifo_mac_counter;	/* 0x0280 */
			data_fifo_mac_counter += (port_depth_arr[port] / QM_SIZE_OF_PORT_DEPTH_ARR_MAC_IN_BYTES);
			reg_Data_FIFO_params_p.data_fifo_depth_p  = data_fifo_mac_counter;	/* 0x0320 */
			break;
		case 8:
			reg_Data_FIFO_params_p.data_fifo_base_p   = data_fifo_mac_counter;	/* 0x0320 */
			data_fifo_mac_counter += (port_depth_arr[port] / QM_SIZE_OF_PORT_DEPTH_ARR_MAC_IN_BYTES);
			reg_Data_FIFO_params_p.data_fifo_depth_p  = data_fifo_mac_counter;	/* 0x03C0 */
			break;
		case 9:
			reg_Data_FIFO_params_p.data_fifo_base_p   = data_fifo_mac_counter;	/* 0x03C0 */
			data_fifo_mac_counter += (port_depth_arr[port] / QM_SIZE_OF_PORT_DEPTH_ARR_MAC_IN_BYTES);
			reg_Data_FIFO_params_p.data_fifo_depth_p  = data_fifo_mac_counter;	/* 0x03E0 */
			break;
		case 10:
			reg_Data_FIFO_params_p.data_fifo_base_p   = data_fifo_mac_counter;	/* 0x03E0 */
			data_fifo_mac_counter += (port_depth_arr[port] / QM_SIZE_OF_PORT_DEPTH_ARR_MAC_IN_BYTES);
			reg_Data_FIFO_params_p.data_fifo_depth_p  = data_fifo_mac_counter;	/* 0x0400 */
			break;
		default:
			return rc;
		}
		rc = qm_register_write(reg_base_address, reg_offset, reg_size, (u32 *)&reg_Data_FIFO_params_p);
		if (rc != OK)
			return rc;
	}

	rc = OK;
	return rc;
}

int qm_dqf_port_credit_thr_def_set(void)
/*int qm_ru_port_to_class_def_set()*/
{
	int rc = !OK;
	u32 port_credit_thr_arr[QM_PORT_MAX + 1];

	port_credit_thr_arr[0]  = 0;
	port_credit_thr_arr[1]  = 0;
	port_credit_thr_arr[2]  = 0;
	port_credit_thr_arr[3]  = QM_PORT_CREDIT_THR_ARR_EMAC_DEF;	/*  2432B for eMac0		*/
	port_credit_thr_arr[4]  = QM_PORT_CREDIT_THR_ARR_EMAC_DEF;	/*  2432B for eMac1		*/
	port_credit_thr_arr[5]  = QM_PORT_CREDIT_THR_ARR_EMAC_DEF;	/*  2432B for eMac2		*/
	port_credit_thr_arr[6]  = QM_PORT_CREDIT_THR_ARR_EMAC_DEF;	/*  2432B for eMac3		*/
	port_credit_thr_arr[7]  = QM_PORT_CREDIT_THR_ARR_EMAC_DEF;	/*  2432B for eMac4		*/
	port_credit_thr_arr[8]  = QM_PORT_CREDIT_THR_ARR_CMAC0_DEF;	/*  2432B for cMac0		*/
	port_credit_thr_arr[9]  = QM_PORT_CREDIT_THR_ARR_CMAC1_DEF;	/*   384B for cMac1		*/
	port_credit_thr_arr[10] = QM_PORT_CREDIT_THR_ARR_HMAC_DEF;	/*   384B for hMac		*/
	port_credit_thr_arr[11] = 0;
	port_credit_thr_arr[12] = 0;
	port_credit_thr_arr[13] = 0;
	port_credit_thr_arr[14] = 0;
	port_credit_thr_arr[15] = 0;

	rc = qm_dqf_port_credit_thr_set(port_credit_thr_arr);
	return rc;
}

int qm_dqf_port_credit_thr_set(u32 *port_credit_thr_arr)
{
	int rc = -QM_INPUT_NOT_IN_RANGE;
	struct dqf_Data_FIFO_params_p          reg_Data_FIFO_params_p;
	struct dqf_Credit_Threshold_p          reg_Credit_Threshold_p;
	u32 reg_base_address, reg_size, reg_offset;
	u32 port, data_fifo_depth_p;

	for (port = QM_PORT_MAC_MIN; port <= QM_PORT_MAC_MAX; port++) {
		reg_base_address =      qm.dqf.Data_FIFO_params_p;
		reg_size   =   qm_reg_size.dqf.Data_FIFO_params_p;
		reg_offset = qm_reg_offset.dqf.Data_FIFO_params_p * port;

		rc = qm_register_read(reg_base_address, reg_offset, reg_size, (u32 *)&reg_Data_FIFO_params_p);
		if (rc != OK)
			return rc;
		data_fifo_depth_p = reg_Data_FIFO_params_p.data_fifo_depth_p;

		if ((port_credit_thr_arr[port] % GRANULARITY_OF_16_BYTES) != 0)
			return rc;
		if ((port_credit_thr_arr[port] < QM_PORT_CREDIT_THR_ARR_MIN) ||
			(port_credit_thr_arr[port] > QM_PORT_CREDIT_THR_ARR_MAX))
			return rc;
	}

	for (port = QM_PORT_MAC_MIN; port <= QM_PORT_MAC_MAX; port++) {
		reg_base_address =      qm.dqf.Credit_Threshold_p;
		reg_size   =   qm_reg_size.dqf.Credit_Threshold_p;
		reg_offset = qm_reg_offset.dqf.Credit_Threshold_p * port;

		rc = qm_register_read(reg_base_address, reg_offset, reg_size, (u32 *)&reg_Credit_Threshold_p);
		if (rc != OK)
			return rc;

		reg_Credit_Threshold_p.Credit_Threshold_p   = port_credit_thr_arr[port];

		rc = qm_register_write(reg_base_address, reg_offset, reg_size, (u32 *)&reg_Credit_Threshold_p);
		if (rc != OK)
			return rc;
	}

	rc = OK;
	return rc;
}

int qm_dqf_port_ppc_map_def_set(void)
{
	int rc = !OK;
	u32 port_ppc_arr[QM_PORT_MAX + 1];
	u32 port;

	port_ppc_arr[0]  = QM_PORT_PPC_ARR_PPC0_DEF;	/* 1 for PPC0      */
	port_ppc_arr[1]  = QM_PORT_PPC_ARR_PPC1_DEF;	/* 2 for PPC1-mnt0 */
	port_ppc_arr[2]  = QM_PORT_PPC_ARR_PPC1_DEF;	/* 2 for PPC1-mnt1 */
	port_ppc_arr[3]  = 0;
	port_ppc_arr[4]  = 0;
	port_ppc_arr[5]  = 0;
	port_ppc_arr[6]  = 0;
	port_ppc_arr[7]  = 0;
	port_ppc_arr[8]  = 0;
	port_ppc_arr[9]  = 0;
	port_ppc_arr[10] = 0;
	port_ppc_arr[11] = 0;
	port_ppc_arr[12] = 0;
	port_ppc_arr[13] = 0;
	port_ppc_arr[14] = 0;
	port_ppc_arr[15] = 0;

	/* PPC */
	for (port = QM_PORT_PPC_MIN; port <= QM_PORT_PPC_MAX; port++) {
		rc = qm_dqf_port_ppc_map_set(&port_ppc_arr[port], port);
		if (rc != OK)
			return rc;
	}

	rc = OK;
	return rc;
}

int qm_dqf_port_ppc_map_set(u32 *port_ppc, u32 port)
{
	int rc = -QM_INPUT_NOT_IN_RANGE;
	struct dqf_PPC_port_map_p              reg_PPC_port_map_p;
	u32 reg_base_address, reg_size, reg_offset;

	if ((port          < QM_PORT_PPC_MIN) || (port          >  QM_PORT_PPC_MAX))
		return rc;

	reg_base_address =      qm.dqf.PPC_port_map_p;
	reg_size   =   qm_reg_size.dqf.PPC_port_map_p;
	reg_offset = qm_reg_offset.dqf.PPC_port_map_p * port;

	rc = qm_register_read(reg_base_address, reg_offset, reg_size, (u32 *)&reg_PPC_port_map_p);
	if (rc != OK)
		return rc;
	reg_PPC_port_map_p.ppc_port_map_p = *port_ppc;
	rc = qm_register_write(reg_base_address, reg_offset, reg_size, (u32 *)&reg_PPC_port_map_p);
	return rc;
}

int qm_dma_qos_attr_def_set(void)
{
	int rc = !OK;
	u32 swf_awqos, rdma_awqos, hwf_qe_ce_awqos, hwf_sfh_pl_awqos;

	swf_awqos        = QM_SWF_AWQOS_DEF;		/* 1 for QM_SWF_AWQOS_DEF        */
	rdma_awqos       = QM_RDMA_AWQOS_DEF;		/* 1 for QM_RDMA_AWQOS_DEF       */
	hwf_qe_ce_awqos  = QM_HWF_QE_CE_AWQOS_DEF;	/* 1 for QM_HWF_QE_CE_AWQOS_DEF  */
	hwf_sfh_pl_awqos = QM_HWF_SFH_PL_AWQOS_DEF;	/* 1 for QM_HWF_SFH_PL_AWQOS_DEF */

	rc = qm_dma_qos_attr_set(swf_awqos, rdma_awqos, hwf_qe_ce_awqos, hwf_sfh_pl_awqos);
	return rc;
}

/*
int qm_axi_swf_write_attr_set(u32 qos,  u32 cache, u32 domain)
int qm_axi_rdma_write_attr_set(u32 qos, u32 cache, u32 domain)
int qm_axi_hwf_qece_write_attr_set(u32 qos, u32 cache, u32 domain)
int qm_axi_hwf_pyl_write_attr_set(u32 qos, u32 cache, u32 domain)
*/
int qm_dma_qos_attr_set(u32 swf_awqos, u32 rdma_awqos, u32 hwf_qe_ce_awqos, u32 hwf_sfh_pl_awqos)
{
	int rc = -QM_INPUT_NOT_IN_RANGE;
	struct dma_AXI_write_attributes_for_swf_mode  reg_AXI_write_attributes_for_swf_mode;
	struct dma_AXI_write_attributes_for_rdma_mode reg_AXI_write_attributes_for_rdma_mode;
	struct dma_AXI_write_attributes_for_hwf_qece  reg_AXI_write_attributes_for_hwf_qece;
	struct dma_AXI_write_attributes_for_hwf_pyld  reg_AXI_write_attributes_for_hwf_pyld;
	u32 reg_base_address, reg_size, reg_offset;

	if ((swf_awqos       <         QM_SWF_AWQOS_MIN) || (swf_awqos        >        QM_SWF_AWQOS_MAX))
		return rc;
	if ((rdma_awqos      <        QM_RDMA_AWQOS_MIN) || (rdma_awqos       >       QM_RDMA_AWQOS_MAX))
		return rc;
	if ((hwf_qe_ce_awqos <   QM_HWF_QE_CE_AWQOS_MIN) || (hwf_qe_ce_awqos  >  QM_HWF_QE_CE_AWQOS_MAX))
		return rc;
	if ((hwf_sfh_pl_awqos < QM_HWF_SFH_PL_AWQOS_MIN) || (hwf_sfh_pl_awqos > QM_HWF_SFH_PL_AWQOS_MAX))
		return rc;

	reg_base_address =      qm.dma.AXI_write_attributes_for_swf_mode;
	reg_size   =   qm_reg_size.dma.AXI_write_attributes_for_swf_mode;
	reg_offset = qm_reg_offset.dma.AXI_write_attributes_for_swf_mode * 0;

	rc = qm_register_read(reg_base_address, reg_offset, reg_size, (u32 *)&reg_AXI_write_attributes_for_swf_mode);
	if (rc != OK)
		return rc;
	reg_AXI_write_attributes_for_swf_mode.swf_awqos    = swf_awqos;
	rc = qm_register_write(reg_base_address, reg_offset, reg_size, (u32 *)&reg_AXI_write_attributes_for_swf_mode);
	if (rc != OK)
		return rc;

	reg_base_address =      qm.dma.AXI_write_attributes_for_rdma_mode;
	reg_size   =   qm_reg_size.dma.AXI_write_attributes_for_rdma_mode;
	reg_offset = qm_reg_offset.dma.AXI_write_attributes_for_rdma_mode * 0;

	rc = qm_register_read(reg_base_address, reg_offset, reg_size, (u32 *)&reg_AXI_write_attributes_for_rdma_mode);
	if (rc != OK)
		return rc;
	reg_AXI_write_attributes_for_rdma_mode.rdma_awqos    = rdma_awqos;
	rc = qm_register_write(reg_base_address, reg_offset, reg_size, (u32 *)&reg_AXI_write_attributes_for_rdma_mode);
	if (rc != OK)
		return rc;

	reg_base_address =      qm.dma.AXI_write_attributes_for_hwf_qece;
	reg_size   =   qm_reg_size.dma.AXI_write_attributes_for_hwf_qece;
	reg_offset = qm_reg_offset.dma.AXI_write_attributes_for_hwf_qece * 0;

	rc = qm_register_read(reg_base_address, reg_offset, reg_size, (u32 *)&reg_AXI_write_attributes_for_hwf_qece);
	if (rc != OK)
		return rc;
	reg_AXI_write_attributes_for_hwf_qece.qece_awqos    = hwf_qe_ce_awqos;
	rc = qm_register_write(reg_base_address, reg_offset, reg_size, (u32 *)&reg_AXI_write_attributes_for_hwf_qece);
	if (rc != OK)
		return rc;

	reg_base_address =      qm.dma.AXI_write_attributes_for_hwf_pyld;
	reg_size   =   qm_reg_size.dma.AXI_write_attributes_for_hwf_pyld;
	reg_offset = qm_reg_offset.dma.AXI_write_attributes_for_hwf_pyld * 0;

	rc = qm_register_read(reg_base_address, reg_offset, reg_size, (u32 *)&reg_AXI_write_attributes_for_hwf_pyld);
	if (rc != OK)
		return rc;
	reg_AXI_write_attributes_for_hwf_pyld.pyld_awqos    = hwf_sfh_pl_awqos;
	rc = qm_register_write(reg_base_address, reg_offset, reg_size, (u32 *)&reg_AXI_write_attributes_for_hwf_pyld);
	return rc;
}

int qm_dma_cache_attr_def_set(void)
{
	int rc = !OK;
	u32 swf_awcache, rdma_awcache, hwf_qe_ce_awcache, hwf_sfh_pl_awcache;

	swf_awcache        = QM_SWF_AWCACHE_DEF;		/* 11 for QM_SWF_AWQOS_DEF        */
	rdma_awcache       = QM_RDMA_AWCACHE_DEF;		/* 11 for QM_RDMA_AWQOS_DEF       */
	hwf_qe_ce_awcache  = QM_HWF_QE_CE_AWCACHE_DEF;	/*  3 for QM_HWF_QE_CE_AWQOS_DEF  */
	hwf_sfh_pl_awcache = QM_HWF_SFH_PL_AWCACHE_DEF;	/*  3 for QM_HWF_SFH_PL_AWQOS_DEF */

	rc = qm_dma_qos_attr_set(swf_awcache, rdma_awcache, hwf_qe_ce_awcache, hwf_sfh_pl_awcache);
	return rc;
}

int qm_dma_cache_attr_set(u32 swf_awcache, u32 rdma_awcache, u32 hwf_qe_ce_awcache, u32 hwf_sfh_pl_awcache)
{
	int rc = -QM_INPUT_NOT_IN_RANGE;
	struct dma_AXI_write_attributes_for_swf_mode  reg_AXI_write_attributes_for_swf_mode;
	struct dma_AXI_write_attributes_for_rdma_mode reg_AXI_write_attributes_for_rdma_mode;
	struct dma_AXI_write_attributes_for_hwf_qece  reg_AXI_write_attributes_for_hwf_qece;
	struct dma_AXI_write_attributes_for_hwf_pyld  reg_AXI_write_attributes_for_hwf_pyld;
	u32 reg_base_address, reg_size, reg_offset;

	if ((swf_awcache       <         QM_SWF_AWCACHE_MIN) || (swf_awcache        >        QM_SWF_AWCACHE_MAX))
		return rc;
	if ((rdma_awcache      <        QM_RDMA_AWCACHE_MIN) || (rdma_awcache       >       QM_RDMA_AWCACHE_MAX))
		return rc;
	if ((hwf_qe_ce_awcache <   QM_HWF_QE_CE_AWCACHE_MIN) || (hwf_qe_ce_awcache  >  QM_HWF_QE_CE_AWCACHE_MAX))
		return rc;
	if ((hwf_sfh_pl_awcache < QM_HWF_SFH_PL_AWCACHE_MIN) || (hwf_sfh_pl_awcache > QM_HWF_SFH_PL_AWCACHE_MAX))
		return rc;

	reg_base_address =      qm.dma.AXI_write_attributes_for_swf_mode;
	reg_size   =   qm_reg_size.dma.AXI_write_attributes_for_swf_mode;
	reg_offset = qm_reg_offset.dma.AXI_write_attributes_for_swf_mode * 0;

	rc = qm_register_read(reg_base_address, reg_offset, reg_size, (u32 *)&reg_AXI_write_attributes_for_swf_mode);
	if (rc != OK)
		return rc;
	reg_AXI_write_attributes_for_swf_mode.swf_awcache  = swf_awcache;
	rc = qm_register_write(reg_base_address, reg_offset, reg_size, (u32 *)&reg_AXI_write_attributes_for_swf_mode);
	if (rc != OK)
		return rc;

	reg_base_address =      qm.dma.AXI_write_attributes_for_rdma_mode;
	reg_size   =   qm_reg_size.dma.AXI_write_attributes_for_rdma_mode;
	reg_offset = qm_reg_offset.dma.AXI_write_attributes_for_rdma_mode * 0;

	rc = qm_register_read(reg_base_address, reg_offset, reg_size, (u32 *)&reg_AXI_write_attributes_for_rdma_mode);
	if (rc != OK)
		return rc;
	reg_AXI_write_attributes_for_rdma_mode.rdma_awcache  = rdma_awcache;
	rc = qm_register_write(reg_base_address, reg_offset, reg_size, (u32 *)&reg_AXI_write_attributes_for_rdma_mode);
	if (rc != OK)
		return rc;

	reg_base_address =      qm.dma.AXI_write_attributes_for_hwf_qece;
	reg_size   =   qm_reg_size.dma.AXI_write_attributes_for_hwf_qece;
	reg_offset = qm_reg_offset.dma.AXI_write_attributes_for_hwf_qece * 0;

	rc = qm_register_read(reg_base_address, reg_offset, reg_size, (u32 *)&reg_AXI_write_attributes_for_hwf_qece);
	if (rc != OK)
		return rc;
	reg_AXI_write_attributes_for_hwf_qece.qece_awcache  = hwf_qe_ce_awcache;
	rc = qm_register_write(reg_base_address, reg_offset, reg_size, (u32 *)&reg_AXI_write_attributes_for_hwf_qece);
	if (rc != OK)
		return rc;

	reg_base_address =      qm.dma.AXI_write_attributes_for_hwf_pyld;
	reg_size   =   qm_reg_size.dma.AXI_write_attributes_for_hwf_pyld;
	reg_offset = qm_reg_offset.dma.AXI_write_attributes_for_hwf_pyld * 0;

	rc = qm_register_read(reg_base_address, reg_offset, reg_size, (u32 *)&reg_AXI_write_attributes_for_hwf_pyld);
	if (rc != OK)
		return rc;
	reg_AXI_write_attributes_for_hwf_pyld.pyld_awcache  = hwf_sfh_pl_awcache;
	rc = qm_register_write(reg_base_address, reg_offset, reg_size, (u32 *)&reg_AXI_write_attributes_for_hwf_pyld);
	return rc;
}

int qm_dma_domain_attr_def_set(void)
{
	int rc = !OK;
	u32 swf_awdomain, rdma_awdomain, hwf_qe_ce_awdomain, hwf_sfh_pl_awdomain;

	swf_awdomain        = QM_SWF_AWDOMAIN_DEF;			/* 2 for QM_SWF_AWDOMAIN_DEF        */
	rdma_awdomain       = QM_RDMA_AWDOMAIN_DEF;			/* 2 for QM_RDMA_AWDOMAIN_DEF       */
	hwf_qe_ce_awdomain  = QM_HWF_QE_CE_AWDOMAIN_DEF;	/* 0 for QM_HWF_QE_CE_AWDOMAIN_DEF  */
	hwf_sfh_pl_awdomain = QM_HWF_SFH_PL_AWDOMAIN_DEF;	/* 0 for QM_HWF_SFH_PL_AWDOMAIN_DEF */

	rc = qm_dma_qos_attr_set(swf_awdomain, rdma_awdomain, hwf_qe_ce_awdomain, hwf_sfh_pl_awdomain);
	return rc;
}

int qm_dma_domain_attr_set(u32 swf_awdomain, u32 rdma_awdomain, u32 hwf_qe_ce_awdomain, u32 hwf_sfh_pl_awdomain)
{
	int rc = -QM_INPUT_NOT_IN_RANGE;
	struct dma_AXI_write_attributes_for_swf_mode  reg_AXI_write_attributes_for_swf_mode;
	struct dma_AXI_write_attributes_for_rdma_mode reg_AXI_write_attributes_for_rdma_mode;
	struct dma_AXI_write_attributes_for_hwf_qece  reg_AXI_write_attributes_for_hwf_qece;
	struct dma_AXI_write_attributes_for_hwf_pyld  reg_AXI_write_attributes_for_hwf_pyld;
	u32 reg_base_address, reg_size, reg_offset;

	if ((swf_awdomain       <         QM_SWF_AWDOMAIN_MIN) || (swf_awdomain        >        QM_SWF_AWDOMAIN_MAX))
		return rc;
	if ((rdma_awdomain      <        QM_RDMA_AWDOMAIN_MIN) || (rdma_awdomain       >       QM_RDMA_AWDOMAIN_MAX))
		return rc;
	if ((hwf_qe_ce_awdomain <   QM_HWF_QE_CE_AWDOMAIN_MIN) || (hwf_qe_ce_awdomain  >  QM_HWF_QE_CE_AWDOMAIN_MAX))
		return rc;
	if ((hwf_sfh_pl_awdomain < QM_HWF_SFH_PL_AWDOMAIN_MIN) || (hwf_sfh_pl_awdomain > QM_HWF_SFH_PL_AWDOMAIN_MAX))
		return rc;

	reg_base_address =      qm.dma.AXI_write_attributes_for_swf_mode;
	reg_size   =   qm_reg_size.dma.AXI_write_attributes_for_swf_mode;
	reg_offset = qm_reg_offset.dma.AXI_write_attributes_for_swf_mode * 0;

	rc = qm_register_read(reg_base_address, reg_offset, reg_size, (u32 *)&reg_AXI_write_attributes_for_swf_mode);
	if (rc != OK)
		return rc;
	reg_AXI_write_attributes_for_swf_mode.swf_awdomain = swf_awdomain;
	rc = qm_register_write(reg_base_address, reg_offset, reg_size, (u32 *)&reg_AXI_write_attributes_for_swf_mode);
	if (rc != OK)
		return rc;

	reg_base_address =      qm.dma.AXI_write_attributes_for_rdma_mode;
	reg_size   =   qm_reg_size.dma.AXI_write_attributes_for_rdma_mode;
	reg_offset = qm_reg_offset.dma.AXI_write_attributes_for_rdma_mode * 0;

	rc = qm_register_read(reg_base_address, reg_offset, reg_size, (u32 *)&reg_AXI_write_attributes_for_rdma_mode);
	if (rc != OK)
		return rc;
	reg_AXI_write_attributes_for_rdma_mode.rdma_awdomain = rdma_awdomain;
	rc = qm_register_write(reg_base_address, reg_offset, reg_size, (u32 *)&reg_AXI_write_attributes_for_rdma_mode);
	if (rc != OK)
		return rc;

	reg_base_address =      qm.dma.AXI_write_attributes_for_hwf_qece;
	reg_size   =   qm_reg_size.dma.AXI_write_attributes_for_hwf_qece;
	reg_offset = qm_reg_offset.dma.AXI_write_attributes_for_hwf_qece * 0;

	rc = qm_register_read(reg_base_address, reg_offset, reg_size, (u32 *)&reg_AXI_write_attributes_for_hwf_qece);
	if (rc != OK)
		return rc;
	reg_AXI_write_attributes_for_hwf_qece.qece_awdomain = hwf_qe_ce_awdomain;
	rc = qm_register_write(reg_base_address, reg_offset, reg_size, (u32 *)&reg_AXI_write_attributes_for_hwf_qece);
	if (rc != OK)
		return rc;

	reg_base_address =      qm.dma.AXI_write_attributes_for_hwf_pyld;
	reg_size   =   qm_reg_size.dma.AXI_write_attributes_for_hwf_pyld;
	reg_offset = qm_reg_offset.dma.AXI_write_attributes_for_hwf_pyld * 0;

	rc = qm_register_read(reg_base_address, reg_offset, reg_size, (u32 *)&reg_AXI_write_attributes_for_hwf_pyld);
	if (rc != OK)
		return rc;
	reg_AXI_write_attributes_for_hwf_pyld.pyld_awdomain = hwf_sfh_pl_awdomain;
	rc = qm_register_write(reg_base_address, reg_offset, reg_size, (u32 *)&reg_AXI_write_attributes_for_hwf_pyld);
	return rc;
}

int qm_pfe_qos_attr_def_set(void)
{
	int rc = !OK;
	u32 swf_arqos, rdma_arqos, hwf_qe_ce_arqos, hwf_sfh_pl_arqos;

	swf_arqos        = QM_SWF_ARQOS_DEF;		/* 1 for QM_SWF_ARQOS_DEF        */
	rdma_arqos       = QM_RDMA_ARQOS_DEF;		/* 1 for QM_RDMA_ARQOS_DEF       */
	hwf_qe_ce_arqos  = QM_HWF_QE_CE_ARQOS_DEF;	/* 1 for QM_HWF_QE_CE_ARQOS_DEF  */
	hwf_sfh_pl_arqos = QM_HWF_SFH_PL_ARQOS_DEF;	/* 1 for QM_HWF_SFH_PL_ARQOS_DEF */

	rc = qm_dma_qos_attr_set(swf_arqos, rdma_arqos, hwf_qe_ce_arqos, hwf_sfh_pl_arqos);
	return rc;
}

/*
int qm_axi_swf_read_attr_set(u32 qos, u32 cache, u32 domain)
int qm_axi_rdma_read_attr_set(u32 qos, u32 cache, u32 domain)
int qm_axi_hwf_qece_read_attr_set(u32 qos, u32 cache, u32 domain)
int qm_axi_hwf_pyld_read_attr_set(u32 qos, u32 cache, u32 domain)
*/
int qm_pfe_qos_attr_set(u32 swf_arqos, u32 rdma_arqos, u32 hwf_qe_ce_arqos, u32 hwf_sfh_pl_arqos)
{
	int rc = -QM_INPUT_NOT_IN_RANGE;
	struct pfe_AXI_read_attributes_for_swf_mode  reg_AXI_read_attributes_for_swf_mode;
	struct pfe_AXI_read_attributes_for_rdma_mode reg_AXI_read_attributes_for_rdma_mode;
	struct pfe_AXI_read_attributes_for_hwf_qece  reg_AXI_read_attributes_for_hwf_qece;
	struct pfe_AXI_read_attributes_for_hwf_pyld  reg_AXI_read_attributes_for_hwf_pyld;
	u32 reg_base_address, reg_size, reg_offset;

	if ((swf_arqos       <         QM_SWF_ARQOS_MIN) || (swf_arqos        >        QM_SWF_ARQOS_MAX))
		return rc;
	if ((rdma_arqos      <        QM_RDMA_ARQOS_MIN) || (rdma_arqos       >       QM_RDMA_ARQOS_MAX))
		return rc;
	if ((hwf_qe_ce_arqos <   QM_HWF_QE_CE_ARQOS_MIN) || (hwf_qe_ce_arqos  >  QM_HWF_QE_CE_ARQOS_MAX))
		return rc;
	if ((hwf_sfh_pl_arqos < QM_HWF_SFH_PL_ARQOS_MIN) || (hwf_sfh_pl_arqos > QM_HWF_SFH_PL_ARQOS_MAX))
		return rc;

	reg_base_address =      qm.pfe.AXI_read_attributes_for_swf_mode;
	reg_size   =   qm_reg_size.pfe.AXI_read_attributes_for_swf_mode;
	reg_offset = qm_reg_offset.pfe.AXI_read_attributes_for_swf_mode * 0;

	rc = qm_register_read(reg_base_address, reg_offset, reg_size, (u32 *)&reg_AXI_read_attributes_for_swf_mode);
	if (rc != OK)
		return rc;
	reg_AXI_read_attributes_for_swf_mode.swf_arqos    = swf_arqos;
	rc = qm_register_write(reg_base_address, reg_offset, reg_size, (u32 *)&reg_AXI_read_attributes_for_swf_mode);
	if (rc != OK)
		return rc;

	reg_base_address =      qm.pfe.AXI_read_attributes_for_rdma_mode;
	reg_size   =   qm_reg_size.pfe.AXI_read_attributes_for_rdma_mode;
	reg_offset = qm_reg_offset.pfe.AXI_read_attributes_for_rdma_mode * 0;

	rc = qm_register_read(reg_base_address, reg_offset, reg_size, (u32 *)&reg_AXI_read_attributes_for_rdma_mode);
	if (rc != OK)
		return rc;
	reg_AXI_read_attributes_for_rdma_mode.rdma_arqos    = rdma_arqos;
	rc = qm_register_write(reg_base_address, reg_offset, reg_size, (u32 *)&reg_AXI_read_attributes_for_rdma_mode);
	if (rc != OK)
		return rc;

	reg_base_address =      qm.pfe.AXI_read_attributes_for_hwf_qece;
	reg_size   =   qm_reg_size.pfe.AXI_read_attributes_for_hwf_qece;
	reg_offset = qm_reg_offset.pfe.AXI_read_attributes_for_hwf_qece * 0;

	rc = qm_register_read(reg_base_address, reg_offset, reg_size, (u32 *)&reg_AXI_read_attributes_for_hwf_qece);
	if (rc != OK)
		return rc;
	reg_AXI_read_attributes_for_hwf_qece.qece_arqos    = hwf_qe_ce_arqos;
	rc = qm_register_write(reg_base_address, reg_offset, reg_size, (u32 *)&reg_AXI_read_attributes_for_hwf_qece);
	if (rc != OK)
		return rc;

	reg_base_address =      qm.pfe.AXI_read_attributes_for_hwf_pyld;
	reg_size   =   qm_reg_size.pfe.AXI_read_attributes_for_hwf_pyld;
	reg_offset = qm_reg_offset.pfe.AXI_read_attributes_for_hwf_pyld * 0;

	rc = qm_register_read(reg_base_address, reg_offset, reg_size, (u32 *)&reg_AXI_read_attributes_for_hwf_pyld);
	if (rc != OK)
		return rc;
	reg_AXI_read_attributes_for_hwf_pyld.pyld_arqos    = hwf_sfh_pl_arqos;
	rc = qm_register_write(reg_base_address, reg_offset, reg_size, (u32 *)&reg_AXI_read_attributes_for_hwf_pyld);
	return rc;
}

int qm_pfe_cache_attr_def_set(void)
{
	int rc = !OK;
	u32 swf_arcache, rdma_arcache, hwf_qe_ce_arcache, hwf_sfh_pl_arcache;

	swf_arcache        = QM_SWF_ARCACHE_DEF;		/* 1 for QM_SWF_ARCACHE_DEF	       */
	rdma_arcache       = QM_RDMA_ARCACHE_DEF;		/* 1 for QM_RDMA_ARCACHE_DEF       */
	hwf_qe_ce_arcache  = QM_HWF_QE_CE_ARCACHE_DEF;	/* 1 for QM_HWF_QE_CE_ARCACHE_DEF  */
	hwf_sfh_pl_arcache = QM_HWF_SFH_PL_ARCACHE_DEF;	/* 1 for QM_HWF_SFH_PL_ARCACHE_DEF */

	rc = qm_pfe_cache_attr_set(swf_arcache, rdma_arcache, hwf_qe_ce_arcache, hwf_sfh_pl_arcache);
	return rc;
}

int qm_pfe_cache_attr_set(u32 swf_arcache, u32 rdma_arcache, u32 hwf_qe_ce_arcache, u32 hwf_sfh_pl_arcache)
{
	int rc = -QM_INPUT_NOT_IN_RANGE;
	struct pfe_AXI_read_attributes_for_swf_mode  reg_AXI_read_attributes_for_swf_mode;
	struct pfe_AXI_read_attributes_for_rdma_mode reg_AXI_read_attributes_for_rdma_mode;
	struct pfe_AXI_read_attributes_for_hwf_qece  reg_AXI_read_attributes_for_hwf_qece;
	struct pfe_AXI_read_attributes_for_hwf_pyld  reg_AXI_read_attributes_for_hwf_pyld;
	u32 reg_base_address, reg_size, reg_offset;

	if ((swf_arcache       <         QM_SWF_ARCACHE_MIN) || (swf_arcache        >        QM_SWF_ARCACHE_MAX))
		return rc;
	if ((rdma_arcache      <        QM_RDMA_ARCACHE_MIN) || (rdma_arcache       >       QM_RDMA_ARCACHE_MAX))
		return rc;
	if ((hwf_qe_ce_arcache <   QM_HWF_QE_CE_ARCACHE_MIN) || (hwf_qe_ce_arcache  >  QM_HWF_QE_CE_ARCACHE_MAX))
		return rc;
	if ((hwf_sfh_pl_arcache < QM_HWF_SFH_PL_ARCACHE_MIN) || (hwf_sfh_pl_arcache > QM_HWF_SFH_PL_ARCACHE_MAX))
		return rc;

	reg_base_address =      qm.pfe.AXI_read_attributes_for_swf_mode;
	reg_size   =   qm_reg_size.pfe.AXI_read_attributes_for_swf_mode;
	reg_offset = qm_reg_offset.pfe.AXI_read_attributes_for_swf_mode * 0;

	rc = qm_register_read(reg_base_address, reg_offset, reg_size, (u32 *)&reg_AXI_read_attributes_for_swf_mode);
	if (rc != OK)
		return rc;
	reg_AXI_read_attributes_for_swf_mode.swf_arcache    = swf_arcache;
	rc = qm_register_write(reg_base_address, reg_offset, reg_size, (u32 *)&reg_AXI_read_attributes_for_swf_mode);
	if (rc != OK)
		return rc;

	reg_base_address =      qm.pfe.AXI_read_attributes_for_rdma_mode;
	reg_size   =   qm_reg_size.pfe.AXI_read_attributes_for_rdma_mode;
	reg_offset = qm_reg_offset.pfe.AXI_read_attributes_for_rdma_mode * 0;

	rc = qm_register_read(reg_base_address, reg_offset, reg_size, (u32 *)&reg_AXI_read_attributes_for_rdma_mode);
	if (rc != OK)
		return rc;
	reg_AXI_read_attributes_for_rdma_mode.rdma_arcache    = rdma_arcache;
	rc = qm_register_write(reg_base_address, reg_offset, reg_size, (u32 *)&reg_AXI_read_attributes_for_rdma_mode);
	if (rc != OK)
		return rc;

	reg_base_address =      qm.pfe.AXI_read_attributes_for_hwf_qece;
	reg_size   =   qm_reg_size.pfe.AXI_read_attributes_for_hwf_qece;
	reg_offset = qm_reg_offset.pfe.AXI_read_attributes_for_hwf_qece * 0;

	rc = qm_register_read(reg_base_address, reg_offset, reg_size, (u32 *)&reg_AXI_read_attributes_for_hwf_qece);
	if (rc != OK)
		return rc;
	reg_AXI_read_attributes_for_hwf_qece.qece_arcache    = hwf_qe_ce_arcache;
	rc = qm_register_write(reg_base_address, reg_offset, reg_size, (u32 *)&reg_AXI_read_attributes_for_hwf_qece);
	if (rc != OK)
		return rc;

	reg_base_address =      qm.pfe.AXI_read_attributes_for_hwf_pyld;
	reg_size   =   qm_reg_size.pfe.AXI_read_attributes_for_hwf_pyld;
	reg_offset = qm_reg_offset.pfe.AXI_read_attributes_for_hwf_pyld * 0;

	rc = qm_register_read(reg_base_address, reg_offset, reg_size, (u32 *)&reg_AXI_read_attributes_for_hwf_pyld);
	if (rc != OK)
		return rc;
	reg_AXI_read_attributes_for_hwf_pyld.pyld_arcache    = hwf_sfh_pl_arcache;
	rc = qm_register_write(reg_base_address, reg_offset, reg_size, (u32 *)&reg_AXI_read_attributes_for_hwf_pyld);
	return rc;
}

int qm_pfe_domain_attr_def_set(void)
{
	int rc = !OK;
	u32 swf_ardomain, rdma_ardomain, hwf_qe_ce_ardomain, hwf_sfh_pl_ardomain;

	swf_ardomain        = QM_SWF_ARDOMAIN_DEF;			/* 1 for QM_SWF_ARDOMAIN_DEF        */
	rdma_ardomain       = QM_RDMA_ARDOMAIN_DEF;			/* 1 for QM_RDMA_ARDOMAIN_DEF       */
	hwf_qe_ce_ardomain  = QM_HWF_QE_CE_ARDOMAIN_DEF;	/* 1 for QM_HWF_QE_CE_ARDOMAIN_DEF  */
	hwf_sfh_pl_ardomain = QM_HWF_SFH_PL_ARDOMAIN_DEF;	/* 1 for QM_HWF_SFH_PL_ARDOMAIN_DEF */

	rc = qm_pfe_domain_attr_set(swf_ardomain, rdma_ardomain, hwf_qe_ce_ardomain, hwf_sfh_pl_ardomain);
	return rc;
}

int qm_pfe_domain_attr_set(u32 swf_ardomain, u32 rdma_ardomain, u32 hwf_qe_ce_ardomain, u32 hwf_sfh_pl_ardomain)
{
	int rc = -QM_INPUT_NOT_IN_RANGE;
	struct pfe_AXI_read_attributes_for_swf_mode  reg_AXI_read_attributes_for_swf_mode;
	struct pfe_AXI_read_attributes_for_rdma_mode reg_AXI_read_attributes_for_rdma_mode;
	struct pfe_AXI_read_attributes_for_hwf_qece  reg_AXI_read_attributes_for_hwf_qece;
	struct pfe_AXI_read_attributes_for_hwf_pyld  reg_AXI_read_attributes_for_hwf_pyld;
	u32 reg_base_address, reg_size, reg_offset;

	if ((swf_ardomain       <         QM_SWF_ARDOMAIN_MIN) || (swf_ardomain        >        QM_SWF_ARDOMAIN_MAX))
		return rc;
	if ((rdma_ardomain      <        QM_RDMA_ARDOMAIN_MIN) || (rdma_ardomain       >       QM_RDMA_ARDOMAIN_MAX))
		return rc;
	if ((hwf_qe_ce_ardomain <   QM_HWF_QE_CE_ARDOMAIN_MIN) || (hwf_qe_ce_ardomain  >  QM_HWF_QE_CE_ARDOMAIN_MAX))
		return rc;
	if ((hwf_sfh_pl_ardomain < QM_HWF_SFH_PL_ARDOMAIN_MIN) || (hwf_sfh_pl_ardomain > QM_HWF_SFH_PL_ARDOMAIN_MAX))
		return rc;

	reg_base_address =      qm.pfe.AXI_read_attributes_for_swf_mode;
	reg_size   =   qm_reg_size.pfe.AXI_read_attributes_for_swf_mode;
	reg_offset = qm_reg_offset.pfe.AXI_read_attributes_for_swf_mode * 0;

	rc = qm_register_read(reg_base_address, reg_offset, reg_size, (u32 *)&reg_AXI_read_attributes_for_swf_mode);
	if (rc != OK)
		return rc;
	reg_AXI_read_attributes_for_swf_mode.swf_ardomain    = swf_ardomain;
	rc = qm_register_write(reg_base_address, reg_offset, reg_size, (u32 *)&reg_AXI_read_attributes_for_swf_mode);
	if (rc != OK)
		return rc;

	reg_base_address =      qm.pfe.AXI_read_attributes_for_rdma_mode;
	reg_size   =   qm_reg_size.pfe.AXI_read_attributes_for_rdma_mode;
	reg_offset = qm_reg_offset.pfe.AXI_read_attributes_for_rdma_mode * 0;

	rc = qm_register_read(reg_base_address, reg_offset, reg_size, (u32 *)&reg_AXI_read_attributes_for_rdma_mode);
	if (rc != OK)
		return rc;
	reg_AXI_read_attributes_for_rdma_mode.rdma_ardomain    = rdma_ardomain;
	rc = qm_register_write(reg_base_address, reg_offset, reg_size, (u32 *)&reg_AXI_read_attributes_for_rdma_mode);
	if (rc != OK)
		return rc;

	reg_base_address =      qm.pfe.AXI_read_attributes_for_hwf_qece;
	reg_size   =   qm_reg_size.pfe.AXI_read_attributes_for_hwf_qece;
	reg_offset = qm_reg_offset.pfe.AXI_read_attributes_for_hwf_qece * 0;

	rc = qm_register_read(reg_base_address, reg_offset, reg_size, (u32 *)&reg_AXI_read_attributes_for_hwf_qece);
	if (rc != OK)
		return rc;
	reg_AXI_read_attributes_for_hwf_qece.qece_ardomain    = hwf_qe_ce_ardomain;
	rc = qm_register_write(reg_base_address, reg_offset, reg_size, (u32 *)&reg_AXI_read_attributes_for_hwf_qece);
	if (rc != OK)
		return rc;

	reg_base_address =      qm.pfe.AXI_read_attributes_for_hwf_pyld;
	reg_size   =   qm_reg_size.pfe.AXI_read_attributes_for_hwf_pyld;
	reg_offset = qm_reg_offset.pfe.AXI_read_attributes_for_hwf_pyld * 0;

	rc = qm_register_read(reg_base_address, reg_offset, reg_size, (u32 *)&reg_AXI_read_attributes_for_hwf_pyld);
	if (rc != OK)
		return rc;
	reg_AXI_read_attributes_for_hwf_pyld.pyld_ardomain    = hwf_sfh_pl_ardomain;
	rc = qm_register_write(reg_base_address, reg_offset, reg_size, (u32 *)&reg_AXI_read_attributes_for_hwf_pyld);
	return rc;
}

int qm_ql_q_profile_def_set(void)
{
	int rc = !OK;
	u32 queue_profile, queue;

	for (queue = 0; queue <= 191; queue++) {
		queue_profile = QM_QUEUE_PROFILE_0;
		rc = qm_ql_q_profile_set(queue_profile, queue);
		if (rc != OK)
			return rc;
	}
	for (queue = 192; queue <= 223; queue++) {
		queue_profile = QM_QUEUE_PROFILE_1;
		rc = qm_ql_q_profile_set(queue_profile, queue);
		if (rc != OK)
			return rc;
	}
	for (queue = 224; queue <= 255; queue++) {
		queue_profile = QM_QUEUE_PROFILE_3;
		rc = qm_ql_q_profile_set(queue_profile, queue);
		if (rc != OK)
			return rc;
	}
	for (queue = 256; queue <= 287; queue++) {
		queue_profile = QM_QUEUE_PROFILE_4;
		rc = qm_ql_q_profile_set(queue_profile, queue);
		if (rc != OK)
			return rc;
	}
	for (queue = 288; queue <= 319; queue++) {
		queue_profile = QM_QUEUE_PROFILE_5;
		rc = qm_ql_q_profile_set(queue_profile, queue);
		if (rc != OK)
			return rc;
	}
	for (queue = 320; queue <= 415; queue++) {
		queue_profile = QM_QUEUE_PROFILE_0;
		rc = qm_ql_q_profile_set(queue_profile, queue);
		if (rc != OK)
			return rc;
	}
	for (queue = 416; queue <= 447; queue++) {
		queue_profile = QM_QUEUE_PROFILE_6;
		rc = qm_ql_q_profile_set(queue_profile, queue);
		if (rc != OK)
			return rc;
	}
	for (queue = 448; queue <= 511; queue++) {
		queue_profile = QM_QUEUE_PROFILE_0;
		rc = qm_ql_q_profile_set(queue_profile, queue);
		if (rc != OK)
			return rc;
	}

	rc = OK;
	return rc;
}

int qm_ql_q_profile_set(u32 queue_profile, u32 queue)
{
	int rc = -QM_INPUT_NOT_IN_RANGE;
	struct ql_qptr_entry reg_qptr_entry;
	u32 reg_base_address, reg_size, reg_offset;

	if ((queue_profile < QM_QUEUE_PROFILE_MIN) || (queue_profile >  QM_QUEUE_PROFILE_MAX))
		return rc;
	if ((queue         <         QM_QUEUE_MIN) || (queue         >          QM_QUEUE_MAX))
		return rc;

	reg_base_address =      qm.ql.qptr;
	reg_size   =   qm_reg_size.ql.qptr;
	reg_offset = qm_reg_offset.ql.qptr * queue/8;

	rc = qm_register_read(reg_base_address, reg_offset, reg_size, (u32 *)&reg_qptr_entry);
	if (rc != OK)
		return rc;

	switch (queue % 8) {
	case 0:
		reg_qptr_entry.qptr0 = queue_profile;
		break;
	case 1:
		reg_qptr_entry.qptr1 = queue_profile;
		break;
	case 2:
		reg_qptr_entry.qptr2 = queue_profile;
		break;
	case 3:
		reg_qptr_entry.qptr3 = queue_profile;
		break;
	case 4:
		reg_qptr_entry.qptr4 = queue_profile;
		break;
	case 5:
		reg_qptr_entry.qptr5 = queue_profile;
		break;
	case 6:
		reg_qptr_entry.qptr6 = queue_profile;
		break;
	case 7:
		reg_qptr_entry.qptr7 = queue_profile;
		break;
	default:
		return rc;
	}

	rc = qm_register_write(reg_base_address, reg_offset, reg_size, (u32 *)&reg_qptr_entry);
	return rc;
}

int qm_ql_thr_def_set(void)
{
	int rc = !OK;
	u32 low_threshold, pause_threshold, high_threshold, traffic_source;

/*
#define QM_LOW_THRESHOLD_DEF	QM_THR_HI_DEF
#define QM_QUEUE_PROFILE_INVALID			0x00000000	/ *    0 * /
*/
#define QM_QUEUE_PROFILE_0_LOW_THRESHOLD_DEF					0x0004B000	/* 300.0KB */
#define QM_QUEUE_PROFILE_1_LOW_THRESHOLD_DEF					0x00007800	/*  30.0KB */
#define QM_QUEUE_PROFILE_2_LOW_THRESHOLD_DEF					0x00007800	/*  30.0KB */
#define QM_QUEUE_PROFILE_3_LOW_THRESHOLD_DEF					0x00003C00	/*  15.0KB */
#define QM_QUEUE_PROFILE_4_LOW_THRESHOLD_DEF					0x00003C00	/*  15.0KB */
#define QM_QUEUE_PROFILE_5_LOW_THRESHOLD_DEF					0x00000200	/*   0.5KB */
#define QM_QUEUE_PROFILE_6_LOW_THRESHOLD_DEF					0x00000600	/*   1.5KB */

#define QM_QUEUE_PROFILE_0_PAUSE_THRESHOLD_DEF					0xFFFFFF	/* -1 */
#define QM_QUEUE_PROFILE_1_PAUSE_THRESHOLD_DEF					0xFFFFFF	/* 0xFFFFFF */
#define QM_QUEUE_PROFILE_2_PAUSE_THRESHOLD_DEF					0xFFFFFF	/* 0xFFFFFF */
#define QM_QUEUE_PROFILE_3_PAUSE_THRESHOLD_DEF					0xFFFFFF	/* 0xFFFFFF */
#define QM_QUEUE_PROFILE_4_PAUSE_THRESHOLD_DEF					0xFFFFFF	/* 0xFFFFFF */
#define QM_QUEUE_PROFILE_5_PAUSE_THRESHOLD_DEF					0xFFFFFF	/* 0xFFFFFF */
#define QM_QUEUE_PROFILE_6_PAUSE_THRESHOLD_DEF					0xFFFFFF	/* 0xFFFFFF */

#define QM_QUEUE_PROFILE_0_HIGH_THRESHOLD_DEF					0x00064000	/* 400.0KB */
#define QM_QUEUE_PROFILE_1_HIGH_THRESHOLD_DEF					0x0000A000	/*  40.0KB */
#define QM_QUEUE_PROFILE_2_HIGH_THRESHOLD_DEF					0x0000A000	/*  40.0KB */
#define QM_QUEUE_PROFILE_3_HIGH_THRESHOLD_DEF					0x00005000	/*  20.0KB */
#define QM_QUEUE_PROFILE_4_HIGH_THRESHOLD_DEF					0x00005000	/*  20.0KB */
#define QM_QUEUE_PROFILE_5_HIGH_THRESHOLD_DEF					0x00000400	/*   1.0KB */
#define QM_QUEUE_PROFILE_6_HIGH_THRESHOLD_DEF					0x00000800	/*   2.0KB */

#define QM_QUEUE_PROFILE_0_TRAFFIC_SOURCE_DEF					         0	/*    0 */
#define QM_QUEUE_PROFILE_1_TRAFFIC_SOURCE_DEF					         0	/*    0 */
#define QM_QUEUE_PROFILE_2_TRAFFIC_SOURCE_DEF					0x00000001	/*    1 */
#define QM_QUEUE_PROFILE_3_TRAFFIC_SOURCE_DEF					0x00000002	/*    2 */
#define QM_QUEUE_PROFILE_4_TRAFFIC_SOURCE_DEF					0x00000003	/*    3 */
#define QM_QUEUE_PROFILE_5_TRAFFIC_SOURCE_DEF					0x00000004	/*    4 */
#define QM_QUEUE_PROFILE_6_TRAFFIC_SOURCE_DEF					0x00000004	/*    4 */

	/* Profile 0 high 400KB low 300KB pause 0xFFFFFF source 0 (emac0-10G) */
	low_threshold	= QM_QUEUE_PROFILE_0_LOW_THRESHOLD_DEF;
	pause_threshold	= QM_QUEUE_PROFILE_0_PAUSE_THRESHOLD_DEF;
	high_threshold	= QM_QUEUE_PROFILE_0_HIGH_THRESHOLD_DEF;
	traffic_source	= QM_QUEUE_PROFILE_0_TRAFFIC_SOURCE_DEF;
	rc = qm_ql_thr_set(low_threshold, pause_threshold, high_threshold, traffic_source, QM_QUEUE_PROFILE_0);
	if (rc != OK)
		return rc;

	/* Profile 1 high 40KB low 30KB pause 0xFFFFFF source 0 (emac0-1G) */
	low_threshold	= QM_QUEUE_PROFILE_1_LOW_THRESHOLD_DEF;
	pause_threshold	= QM_QUEUE_PROFILE_1_PAUSE_THRESHOLD_DEF;
	high_threshold	= QM_QUEUE_PROFILE_1_HIGH_THRESHOLD_DEF;
	traffic_source	= QM_QUEUE_PROFILE_1_TRAFFIC_SOURCE_DEF;
	rc = qm_ql_thr_set(low_threshold, pause_threshold, high_threshold, traffic_source, QM_QUEUE_PROFILE_1);
	if (rc != OK)
		return rc;

	/* Profile 2 high 40KB low 30KB pause 0xFFFFFF source 1 (emac1-1G) */
	low_threshold	= QM_QUEUE_PROFILE_2_LOW_THRESHOLD_DEF;
	pause_threshold	= QM_QUEUE_PROFILE_2_PAUSE_THRESHOLD_DEF;
	high_threshold	= QM_QUEUE_PROFILE_2_HIGH_THRESHOLD_DEF;
	traffic_source	= QM_QUEUE_PROFILE_2_TRAFFIC_SOURCE_DEF;
	rc = qm_ql_thr_set(low_threshold, pause_threshold, high_threshold, traffic_source, QM_QUEUE_PROFILE_2);
	if (rc != OK)
		return rc;

	/* Profile 3 high 20KB low 15KB pause 0xFFFFFF source 2 (emac2-1G) */
	low_threshold	= QM_QUEUE_PROFILE_3_LOW_THRESHOLD_DEF;
	pause_threshold	= QM_QUEUE_PROFILE_3_PAUSE_THRESHOLD_DEF;
	high_threshold	= QM_QUEUE_PROFILE_3_HIGH_THRESHOLD_DEF;
	traffic_source	= QM_QUEUE_PROFILE_3_TRAFFIC_SOURCE_DEF;
	rc = qm_ql_thr_set(low_threshold, pause_threshold, high_threshold, traffic_source, QM_QUEUE_PROFILE_3);
	if (rc != OK)
		return rc;

	/* Profile 4 high 20KB low 15KB pause 0xFFFFFF source 3 (emac3-1G) */
	low_threshold	= QM_QUEUE_PROFILE_4_LOW_THRESHOLD_DEF;
	pause_threshold	= QM_QUEUE_PROFILE_4_PAUSE_THRESHOLD_DEF;
	high_threshold	= QM_QUEUE_PROFILE_4_HIGH_THRESHOLD_DEF;
	traffic_source	= QM_QUEUE_PROFILE_4_TRAFFIC_SOURCE_DEF;
	rc = qm_ql_thr_set(low_threshold, pause_threshold, high_threshold, traffic_source, QM_QUEUE_PROFILE_4);
	if (rc != OK)
		return rc;

	/* Profile 5 high  1KB low 0.5KB pause 0xFFFFFF source 4 (hmac-1K) */
	low_threshold	= QM_QUEUE_PROFILE_5_LOW_THRESHOLD_DEF;
	pause_threshold	= QM_QUEUE_PROFILE_5_PAUSE_THRESHOLD_DEF;
	high_threshold	= QM_QUEUE_PROFILE_5_HIGH_THRESHOLD_DEF;
	traffic_source	= QM_QUEUE_PROFILE_5_TRAFFIC_SOURCE_DEF;
	rc = qm_ql_thr_set(low_threshold, pause_threshold, high_threshold, traffic_source, QM_QUEUE_PROFILE_5);
	if (rc != OK)
		return rc;

	/* Profile 6  high  2KB low 1.5KB pause 0xFFFFFF source 4 (hmac-2K) */
	low_threshold	= QM_QUEUE_PROFILE_6_LOW_THRESHOLD_DEF;
	pause_threshold	= QM_QUEUE_PROFILE_6_PAUSE_THRESHOLD_DEF;
	high_threshold	= QM_QUEUE_PROFILE_6_HIGH_THRESHOLD_DEF;
	traffic_source	= QM_QUEUE_PROFILE_6_TRAFFIC_SOURCE_DEF;
	rc = qm_ql_thr_set(low_threshold, pause_threshold, high_threshold, traffic_source, QM_QUEUE_PROFILE_6);
	return rc;
}

int qm_ql_thr_set(u32 low_threshold, u32 pause_threshold, u32 high_threshold, u32 traffic_source, u32 queue_profile)
{
	int rc = -QM_INPUT_NOT_IN_RANGE;
	struct ql_low_threshold		reg_low_threshold;
	struct ql_pause_threshold	reg_pause_threshold;
	struct ql_high_threshold	reg_high_threshold;
	struct ql_traffic_source	reg_traffic_source;
	u32 reg_base_address, reg_size, reg_offset;

	if ((queue_profile   <   QM_QUEUE_PROFILE_MIN) || (queue_profile   >   QM_QUEUE_PROFILE_MAX))
		return rc;
	if ((low_threshold   <   QM_LOW_THRESHOLD_MIN) || (low_threshold   >   QM_LOW_THRESHOLD_MAX))
		return rc;
	if ((pause_threshold < QM_PAUSE_THRESHOLD_MIN) || (pause_threshold > QM_PAUSE_THRESHOLD_MAX))
		return rc;
	if ((high_threshold  <  QM_HIGH_THRESHOLD_MIN) || (high_threshold  >  QM_HIGH_THRESHOLD_MAX))
		return rc;
	if ((traffic_source  <  QM_TRAFFIC_SOURCE_MIN) || (traffic_source  >  QM_TRAFFIC_SOURCE_MAX))
		return rc;

	reg_base_address =      qm.ql.low_threshold;
	reg_size   =   qm_reg_size.ql.low_threshold;
	reg_offset = qm_reg_offset.ql.low_threshold * (queue_profile - 1);

	rc = qm_register_read(reg_base_address, reg_offset, reg_size, (u32 *)&reg_low_threshold);
	if (rc != OK)
		return rc;
	reg_low_threshold.low_threshold   = low_threshold;
	rc = qm_register_write(reg_base_address, reg_offset, reg_size, (u32 *)&reg_low_threshold);
	if (rc != OK)
		return rc;

	reg_base_address =      qm.ql.pause_threshold;
	reg_size   =   qm_reg_size.ql.pause_threshold;
	reg_offset = qm_reg_offset.ql.pause_threshold * (queue_profile - 1);

	rc = qm_register_read(reg_base_address, reg_offset, reg_size, (u32 *)&reg_pause_threshold);
	if (rc != OK)
		return rc;
	reg_pause_threshold.pause_threshold = pause_threshold;
	rc = qm_register_write(reg_base_address, reg_offset, reg_size, (u32 *)&reg_pause_threshold);
	if (rc != OK)
		return rc;

	reg_base_address =      qm.ql.high_threshold;
	reg_size   =   qm_reg_size.ql.high_threshold;
	reg_offset = qm_reg_offset.ql.high_threshold * (queue_profile - 1);

	rc = qm_register_read(reg_base_address, reg_offset, reg_size, (u32 *)&reg_high_threshold);
	if (rc != OK)
		return rc;
	reg_high_threshold.high_threshold = high_threshold;
	rc = qm_register_write(reg_base_address, reg_offset, reg_size, (u32 *)&reg_high_threshold);
	if (rc != OK)
		return rc;

	reg_base_address =      qm.ql.traffic_source;
	reg_size   =   qm_reg_size.ql.traffic_source;
	reg_offset = qm_reg_offset.ql.traffic_source * (queue_profile - 1);

	rc = qm_register_read(reg_base_address, reg_offset, reg_size, (u32 *)&reg_traffic_source);
	if (rc != OK)
		return rc;
	reg_traffic_source.traffic_source = traffic_source;
	rc = qm_register_write(reg_base_address, reg_offset, reg_size, (u32 *)&reg_traffic_source);
	if (rc != OK)
		return rc;

	return rc;
}

int qm_vmid_set(u32 qm_vmid)
{
	int rc = -QM_INPUT_NOT_IN_RANGE;

	if ((qm_vmid < QM_VMID_MIN) || (qm_vmid > QM_VMID_MAX))
		return rc;

	/*rc = bm_vmid_set( &ctl, bm_vmid); if (rc) return rc; */
	rc = dma_vmid_set(qm_vmid);
	if (rc != OK)
		return rc;

	rc = pfe_vmid_set(qm_vmid);
	if (rc != OK)
		return rc;

	return rc;
}

int dma_vmid_set(u32 qm_vmid)
{
	int rc = -QM_INPUT_NOT_IN_RANGE;
	struct dma_DRAM_VMID				  reg_DRAM_VMID;
	u32 reg_base_address, reg_size, reg_offset;

	if ((qm_vmid < QM_VMID_MIN) || (qm_vmid > QM_VMID_MAX))
		return rc;

	reg_base_address =      qm.dma.DRAM_VMID;
	reg_size   =   qm_reg_size.dma.DRAM_VMID;
	reg_offset = qm_reg_offset.dma.DRAM_VMID * 0;

	rc = qm_register_read(reg_base_address, reg_offset, reg_size, (u32 *)&reg_DRAM_VMID);
	if (rc != OK)
		return rc;
	reg_DRAM_VMID.dram_vmid = qm_vmid;
	rc = qm_register_write(reg_base_address, reg_offset, reg_size, (u32 *)&reg_DRAM_VMID);
	if (rc != OK)
		return rc;

	return rc;
}

int pfe_vmid_set(u32 qm_vmid)
{
	int rc = -QM_INPUT_NOT_IN_RANGE;
	struct pfe_QM_VMID     reg_QM_VMID;
	u32 reg_base_address, reg_size, reg_offset;

	if ((qm_vmid < QM_VMID_MIN) || (qm_vmid > QM_VMID_MAX))
		return rc;

	reg_base_address =      qm.pfe.QM_VMID;
	reg_size   =   qm_reg_size.pfe.QM_VMID;
	reg_offset = qm_reg_offset.pfe.QM_VMID * 0;

	rc = qm_register_read(reg_base_address, reg_offset, reg_size, (u32 *)&reg_QM_VMID);
	if (rc != OK)
		return rc;
	reg_QM_VMID.VMID = qm_vmid;
	rc = qm_register_write(reg_base_address, reg_offset, reg_size, (u32 *)&reg_QM_VMID);
	if (rc != OK)
		return rc;

	return rc;
}

int qm_queue_flush_start(u32 queue)
{
	int rc = -QM_INPUT_NOT_IN_RANGE;
	struct pfe_queue_flush reg_queue_flush;
	u32 reg_base_address, reg_size, reg_offset;

	if ((queue   <   QM_QUEUE_MIN) || (queue   >   QM_QUEUE_MAX))
		return rc;

	reg_base_address =      qm.pfe.queue_flush;
	reg_size   =   qm_reg_size.pfe.queue_flush;
	reg_offset = qm_reg_offset.pfe.queue_flush * (queue/32);

	rc = qm_register_read(reg_base_address, reg_offset, reg_size, (u32 *)&reg_queue_flush);
	if (rc != OK)
		return rc;
	reg_queue_flush.queue_flush_bit_per_q |=  (0x00000001 << ((reg_queue_flush.queue_flush_bit_per_q)%32));
	rc = qm_register_write(reg_base_address, reg_offset, reg_size, (u32 *)&reg_queue_flush);
	if (rc != OK)
		return rc;

	return rc;
}

int qm_port_flush_start(u32 port)
{
	int rc = -QM_INPUT_NOT_IN_RANGE;
	int pid;
	struct pfe_port_flush reg_port_flush;
	u32 reg_base_address, reg_size, reg_offset;

	if ((port            <    QM_PORT_MIN) || (port            >    QM_PORT_MAX))
		return rc;

	pid = port;

	reg_base_address =      qm.pfe.port_flush;
	reg_size   =   qm_reg_size.pfe.port_flush;
	reg_offset = qm_reg_offset.pfe.port_flush * 0;

	rc = qm_register_read(reg_base_address, reg_offset, reg_size, (u32 *)&reg_port_flush);
	if (rc != OK)
		return rc;
	reg_port_flush.port_flush |=  (0x00000001 << pid);
	rc = qm_register_write(reg_base_address, reg_offset, reg_size, (u32 *)&reg_port_flush);
	if (rc != OK)
		return rc;

	return rc;
}

int ql_queue_length_get(u32 queue, u32 *length, u32 *status)
{
	int rc = -QM_INPUT_NOT_IN_RANGE;
	u32 reg_base_address, reg_size, reg_offset;
	struct ql_qlen     reg_qlen;

	if ((queue   <   QM_QUEUE_MIN) || (queue   >   QM_QUEUE_MAX))
		return rc;

	reg_base_address =      qm.ql.qlen;
	reg_size   =   qm_reg_size.ql.qlen;
	reg_offset = qm_reg_offset.ql.qlen * queue;

	rc = qm_register_read(reg_base_address, reg_offset, reg_size, (u32 *)&reg_qlen);
	if (rc != OK)
		return rc;
	*length = reg_qlen.reg_ql_entry.ql;
	*status = reg_qlen.reg_ql_entry.qstatus;

	return rc;
}

int qm_idle_status_get(u32 *status)
{
	int rc = -QM_INPUT_NOT_IN_RANGE;
	struct dma_idle_status reg_idle_status;
	u32 reg_base_address, reg_size, reg_offset;

	reg_base_address =      qm.dma.idle_status;
	reg_size   =   qm_reg_size.dma.idle_status;
	reg_offset = qm_reg_offset.dma.idle_status * 0;

	pr_info("\n-------------- Read DMA idle status -----------");

	rc = qm_register_read(reg_base_address, reg_offset, reg_size, (u32 *)&reg_idle_status);
	if (rc != OK)
		return rc;
	*status = *(u32 *)&reg_idle_status;

	pr_info("\n");

	pr_info("\t idle_status.gpm_pl_cache_is_empty  = 0x%08X\n",
				reg_idle_status.gpm_pl_cache_is_empty);
	pr_info("\t idle_status.gpm_pl_cache_is_full  = 0x%08X\n",
				reg_idle_status.gpm_pl_cache_is_full);
	pr_info("\t idle_status.gpm_qe_cache_is_empty  = 0x%08X\n",
				reg_idle_status.gpm_qe_cache_is_empty);
	pr_info("\t idle_status.gpm_qe_cache_is_full  = 0x%08X\n",
				reg_idle_status.gpm_qe_cache_is_full);
	pr_info("\t idle_status.dram_pl_cache_is_empty  = 0x%08X\n",
				reg_idle_status.dram_pl_cache_is_empty);
	pr_info("\t idle_status.dram_pl_cache_is_full  = 0x%08X\n",
				reg_idle_status.dram_pl_cache_is_full);
	pr_info("\t idle_status.dram_qe_cache_is_empty  = 0x%08X\n",
				reg_idle_status.dram_qe_cache_is_empty);
	pr_info("\t idle_status.dram_qe_cache_is_full  = 0x%08X\n",
				reg_idle_status.dram_qe_cache_is_full);
	pr_info("\t idle_status.dram_fifo_is_empty  = 0x%08X\n",
				reg_idle_status.dram_fifo_is_empty);
	pr_info("\t idle_status.mac_axi_enq_channel_is_empty  = 0x%08X\n",
				reg_idle_status.mac_axi_enq_channel_is_empty);
	pr_info("\t idle_status.NSS_axi_enq_channel_is_empty  = 0x%08X\n",
				reg_idle_status.NSS_axi_enq_channel_is_empty);
	pr_info("\t idle_status.gpm_ppe_read_fifo_is_empty  = 0x%08X\n",
				reg_idle_status.gpm_ppe_read_fifo_is_empty);
	pr_info("\t idle_status.ppe_gpm_pl_write_fifo_is_empty  = 0x%08X\n",
				reg_idle_status.ppe_gpm_pl_write_fifo_is_empty);
	pr_info("\t idle_status.ppe_gpm_qe_write_fifo_is_empty  = 0x%08X\n",
				reg_idle_status.ppe_gpm_qe_write_fifo_is_empty);
	pr_info("\t idle_status.ppe_ru_read_fifo_is_empty  = 0x%08X\n",
				reg_idle_status.ppe_ru_read_fifo_is_empty);
	pr_info("\t idle_status.ppe_ru_write_fifo_is_empty  = 0x%08X\n",
				reg_idle_status.ppe_ru_write_fifo_is_empty);
	pr_info("\t idle_status.ru_ppe_read_fifo_is_empty  = 0x%08X\n",
				reg_idle_status.ru_ppe_read_fifo_is_empty);
	pr_info("\t idle_status.dram_fifo_fsm_state_is_idle  = 0x%08X\n",
				reg_idle_status.dram_fifo_fsm_state_is_idle);
	pr_info("\t idle_status.qeram_init_fsm_state_is_idle  = 0x%08X\n",
				reg_idle_status.qeram_init_fsm_state_is_idle);

/*
	rc = qm_register_read(reg_base_address, reg_offset, reg_size, (u32 *)status);
	if (rc != OK)
		return rc;
*/
	return rc;
}

/*
int reorder_class_cmd_set(u32 host, u32 class, u32 sid, u32 cmd)
int	qm_ru_class_cmd_set(u32 host, u32 class, , u32 sid, u32 cmd)
 TBD (Alon Eldar)
int qm_class_cmd_set(u32 host, u32 reorder_class, u32 sid, u32 cmd)
*/
int	qm_ru_class_cmd_set(u32 host, u32 host_class, u32 host_sid, u32 cmd)
{
	int rc = -QM_INPUT_NOT_IN_RANGE;
	struct reorder_ru_host_cmd        reg_ru_host_cmd;
	struct reorder_ru_task_permission reg_ru_task_permission;
	u32 reg_base_address, reg_size, reg_offset;

	if ((host       <          QM_HOST_MIN) || (host       >          QM_HOST_MAX))
		return rc; /* Host: cpu 0 or cpu 1 (how does neta knows who It is  ask DIma?) */
	if ((host_class < QM_REORDER_CLASS_MIN) || (host_class > QM_REORDER_CLASS_MAX))
		return rc;
	if ((host_sid   <           QM_SID_MIN) || (host_sid   >           QM_SID_MAX))
		return rc;
	if ((cmd        <           QM_CMD_MIN) || (cmd        >           QM_CMD_MAX))
		return rc;

	reg_base_address =      qm.reorder.ru_task_permission;
	reg_size   =   qm_reg_size.reorder.ru_task_permission;
	reg_offset = qm_reg_offset.reorder.ru_task_permission * host;

	rc = qm_register_read(reg_base_address, reg_offset, reg_size, (u32 *)&reg_ru_task_permission);
	if (rc != OK)
		return rc;

	if (reg_ru_task_permission.ru_host_permitted == 1) {
		reg_base_address =      qm.reorder.ru_host_cmd;
		reg_size   =   qm_reg_size.reorder.ru_host_cmd;
		reg_offset = qm_reg_offset.reorder.ru_host_cmd * host;

		rc = qm_register_read(reg_base_address, reg_offset, reg_size, (u32 *)&reg_ru_host_cmd);
		if (rc != OK)
			return rc;
		reg_ru_host_cmd.ru_host_sid   = host_sid;
		reg_ru_host_cmd.ru_host_class = host_class;
/*		reg_ru_host_cmd.ru_host_task  = XXX;
		reg_ru_host_cmd.ru_host_exec  = XXX;
*/
		rc = qm_register_write(reg_base_address, reg_offset, reg_size, (u32 *)&reg_ru_host_cmd);
		if (rc != OK)
			return rc;
	} else {
		pr_info("\t reg_ru_task_permission.ru_host_permitted = 0x%08X\n",
				reg_ru_task_permission.ru_host_permitted);

	}

	return rc;
}



/*	rc = qm_register_read(qm.ql.mem2mg_resp_data_ll, (u32 *)&reg_mem2mg_resp_data_ll); if (rc) return rc;*/


#define	COMPLETE_HW_WRITE

#ifdef my_RW_DEBUG_UNITEST
int qm_register_read(u32 base_address, u32 offset, u32 wordsNumber, u32 *dataPtr)
{
	int rc = OK;
	u32 *temp;
	u32 i;

/*	In the future we can also add printing of the fields of the register */
/*	pr_info(" DUMMY_PRINT  read by function <%s>,  result = 0x%08X\n", __func__, *(u32 *)dataPtr);

	bm_register_name_get(base_address, offset, reg_name);
	pr_info("[QM]  READ_REG add = 0x%08X : name = %s : value =", base_address, reg_name);
*/
	temp = dataPtr;
	for (i = 0; i < wordsNumber; i++) {
		/*
		pr_info(" 0x%08X", *(u32 *)temp);*/
		*(u32 *)temp = 0;
		temp++;
	}
	pr_info("\n");
/*	return OK;	 */
/*
	rc = mv_pp3_hw_read(base_address+offset, wordsNumber, dataPtr);
	if (rc != OK) {
		pr_info(" Not Available\n");
		return rc;
	}
*/
/*	if (rc != OK)
		return rc;*/

	COMPLETE_HW_WRITE
	rc = OK;
	return rc;
}

int qm_register_write(u32 base_address, u32 offset, u32 wordsNumber, u32 *dataPtr)
{
	char reg_name[50];
	int rc = OK;
	u32 *temp;
	u32 i;

	bm_register_name_get(base_address, offset, reg_name);
	pr_info("[QM] WROTE_REG add = 0x%08X : name = %s : value =", base_address, reg_name);
	temp = dataPtr;
	for (i = 0; i < wordsNumber; i++) {
		pr_info(" 0x%08X", *(u32 *)temp);
		temp++;
	}
	pr_info("\n");

/*	pr_info(" DUMMY_PRINT, result=%d\n", *(u32 *)dataPtr);*/
/*	pr_info(" DUMMY_PRINT, result = 0x%08X\n", *(u32 *)dataPtr);*/
/*	pr_info(" DUMMY_PRINT write by function <%s>, result = 0x%08X\n", __func__, *(u32 *)dataPtr);*/

/*	return OK;	 */
/*
	rc = mv_pp3_hw_write(base_address+offset, wordsNumber, dataPtr);
	if (rc != OK)
		return rc;
*/

	COMPLETE_HW_WRITE
	return rc;
}
#else
int qm_register_read(u32 base_address, u32 offset, u32 wordsNumber, u32 *dataPtr)
{
	int rc = OK;

	mv_pp3_hw_read(base_address+offset, wordsNumber, dataPtr);
	if (rc != OK)
		return rc;

	COMPLETE_HW_WRITE
	return rc;
}

int qm_register_write(u32 base_address, u32 offset, u32 wordsNumber, u32 *dataPtr)
{
	int rc = OK;

	mv_pp3_hw_write(base_address+offset, wordsNumber, dataPtr);
	if (rc != OK)
		return rc;

	COMPLETE_HW_WRITE
	return rc;
}
#endif

void qm_register_register_fields_print(u32 base_address, u32 value)
{

}
