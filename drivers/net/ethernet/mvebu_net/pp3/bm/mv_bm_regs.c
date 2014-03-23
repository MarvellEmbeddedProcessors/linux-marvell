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
#include "bm/mv_bm_regs.h"

struct bm_alias bm;
struct bm_alias bm_reg_size;
struct bm_alias bm_reg_offset;

/*
*/

int bm_reg_address_alias_init(void)
{
	int rc = -BM_ALIAS_ERROR;
	u32 bid;
	u32 siliconBase;

	siliconBase = mv_hw_silicon_base_addr_get();

	bm.base = siliconBase + QM_UNIT_OFFSET + BM_UNIT_OFFSET;	/*0x004D0000*/

	for (bid = 0; bid < 5; bid++) {
		if (bid == 0) {
			bm.b_pool_n_conf[bid]             = bm.base + 0x00008000;
			bm.b_pool_n_st[bid]               = bm.base + 0x00008004;
			bm.b_sys_rec_bank_intr_cause[bid] = bm.base + 0x000090A0;
			bm.b_sys_rec_bank_intr_mask[bid]  = bm.base + 0x000090A4;
			bm.b_sys_rec_bank_d0_st[bid]      = bm.base + 0x000090B0;
			bm.b_sys_rec_bank_d1_st[bid]      = bm.base + 0x000090B4;
		} else if ((bid > 0) && (bid < 5)) {
			bm.b_pool_n_conf[bid]             = bm.base + 0x00008040 + (bid-1)*0x0200;
			bm.b_pool_n_st[bid]               = bm.base + 0x00008044 + (bid-1)*0x0200;
			bm.b_sys_rec_bank_intr_cause[bid] = bm.base + 0x000090D0 + (bid-1)*0x0030;
			bm.b_sys_rec_bank_intr_mask[bid]  = bm.base + 0x000090D4 + (bid-1)*0x0030;
			bm.b_sys_rec_bank_d0_st[bid]      = bm.base + 0x000090E0 + (bid-1)*0x0030;
			bm.b_sys_rec_bank_d1_st[bid]      = bm.base + 0x000090E4 + (bid-1)*0x0030;
		} else {
			rc = -BM_BANK_NOT_IN_RANGE;
			return rc;
		}
	}

	bm.sw_debug_rec_intr_cause    = bm.base + 0x00009190;
	bm.sw_debug_rec_intr_mask     = bm.base + 0x00009194;
	bm.sys_nrec_common_intr_cause = bm.base + 0x000091A0;
	bm.sys_nrec_common_intr_mask  = bm.base + 0x000091A4;
	bm.sys_nrec_common_d0_st      = bm.base + 0x000091B0;
	bm.sys_nrec_common_d1_st      = bm.base + 0x000091B4;
	bm.sys_nrec_common_d2_st      = bm.base + 0x000091B8;
	bm.sys_nrec_common_d3_st      = bm.base + 0x000091BC;
	bm.common_general_conf        = bm.base + 0x00009300;
	bm.dram_domain_conf           = bm.base + 0x00009304;
	bm.dram_cache_conf            = bm.base + 0x00009308;
	bm.dram_qos_conf              = bm.base + 0x0000930C;
	bm.error_intr_cause           = bm.base + 0x0000A000;
	bm.error_intr_mask            = bm.base + 0x0000A004;
	bm.func_intr_cause            = bm.base + 0x0000A010;
	bm.func_intr_mask             = bm.base + 0x0000A014;
	bm.ecc_err_intr_cause         = bm.base + 0x0000A020;
	bm.ecc_err_intr_mask          = bm.base + 0x0000A024;

	for (bid = 0; bid < 5; bid++) {
		if (bid == 0) {
			bm.pool_nempty_intr_cause[bid] = bm.base + 0x0000A040;
			bm.pool_nempty_intr_mask[bid]  = bm.base + 0x0000A044;
			bm.dpool_ae_intr_cause[bid]    = bm.base + 0x0000A048;
			bm.dpool_ae_intr_mask[bid]     = bm.base + 0x0000A04C;
			bm.dpool_af_intr_cause[bid]    = bm.base + 0x0000A050;
			bm.dpool_af_intr_mask[bid]     = bm.base + 0x0000A054;
		} else if ((bid > 0) && (bid < 5)) {
			bm.pool_nempty_intr_cause[bid] = bm.base + 0x0000A060 + (bid-1)*0x0010;
			bm.pool_nempty_intr_mask[bid]  = bm.base + 0x0000A064 + (bid-1)*0x0010;
			bm.dpool_ae_intr_cause[bid]    = bm.base + 0x0000A0A0 + (bid-1)*0x0010;
			bm.dpool_ae_intr_mask[bid]     = bm.base + 0x0000A0A4 + (bid-1)*0x0010;
			bm.dpool_af_intr_cause[bid]    = bm.base + 0x0000A0E0 + (bid-1)*0x0010;
			bm.dpool_af_intr_mask[bid]     = bm.base + 0x0000A0E4 + (bid-1)*0x0010;
		} else {
			rc = -BM_BANK_NOT_IN_RANGE;
			return rc;
		}
	}

	bm.b_bank_req_fifos_st         = bm.base + 0x0000A200;
	bm.b0_past_alc_fifos_st        = bm.base + 0x0000A220;
	bm.bgp_past_alc_fifos_st       = bm.base + 0x0000A224;
	bm.b0_rls_wrp_ppe_fifos_st     = bm.base + 0x0000A230;
	bm.dm_axi_fifos_st             = bm.base + 0x0000A240;
	bm.drm_pend_fifo_st            = bm.base + 0x0000A244;
	bm.dm_axi_wr_pend_fifo_st      = bm.base + 0x0000A248;
	bm.bm_idle_st                  = bm.base + 0x0000A250;

	for (bid = 0; bid < 5; bid++) {
		bm.dpr_c_mng_stat[bid]  = bm.base + 0x00000000 + (bid-0)*0x0400;
		bm.tpr_c_mng_b_dyn[bid] = bm.base + 0x00001400 + (bid-0)*0x0200;
		bm.tpr_ctrs_0_b[bid]    = bm.base + 0x00005000 + (bid-0)*0x0400;
		bm.sram_b_cache[bid]    = bm.base + 0x00010000 + (bid-0)*0x4000;
	}

	bm.dpr_d_mng_ball_stat         = bm.base + 0x00002000;
	bm.tpr_dro_mng_ball_dyn        = bm.base + 0x00004000;
	bm.tpr_drw_mng_ball_dyn        = bm.base + 0x00004800;

	rc = OK;
	return rc;
}

int bm_reg_size_alias_init(void)
{
	int rc = -BM_ALIAS_ERROR;
	u32 bid, word_size_in_bits, byte_size_in_bits = 8;

	word_size_in_bits = 32/byte_size_in_bits;	/* word_size_in_bits = 4 */

	/*memset(&bm_reg_size,0,sizeof(bm_reg_size));*/

	for (bid = 0; bid < 5; bid++) {
		bm_reg_size.b_pool_n_conf[bid]             = 32/byte_size_in_bits/word_size_in_bits;
		bm_reg_size.b_pool_n_st[bid]               = 32/byte_size_in_bits/word_size_in_bits;
		bm_reg_size.b_sys_rec_bank_intr_cause[bid] = 32/byte_size_in_bits/word_size_in_bits;
		bm_reg_size.b_sys_rec_bank_intr_mask[bid]  = 32/byte_size_in_bits/word_size_in_bits;
		bm_reg_size.b_sys_rec_bank_d0_st[bid]      = 32/byte_size_in_bits/word_size_in_bits;
		bm_reg_size.b_sys_rec_bank_d1_st[bid]      = 32/byte_size_in_bits/word_size_in_bits;
	}

	bm_reg_size.sw_debug_rec_intr_cause    = 32/byte_size_in_bits/word_size_in_bits;
	bm_reg_size.sw_debug_rec_intr_mask     = 32/byte_size_in_bits/word_size_in_bits;
	bm_reg_size.sys_nrec_common_intr_cause = 32/byte_size_in_bits/word_size_in_bits;
	bm_reg_size.sys_nrec_common_intr_mask  = 32/byte_size_in_bits/word_size_in_bits;
	bm_reg_size.sys_nrec_common_d0_st      = 32/byte_size_in_bits/word_size_in_bits;
	bm_reg_size.sys_nrec_common_d1_st      = 32/byte_size_in_bits/word_size_in_bits;
	bm_reg_size.sys_nrec_common_d2_st      = 32/byte_size_in_bits/word_size_in_bits;
	bm_reg_size.sys_nrec_common_d3_st      = 32/byte_size_in_bits/word_size_in_bits;
	bm_reg_size.common_general_conf        = 32/byte_size_in_bits/word_size_in_bits;
	bm_reg_size.dram_domain_conf           = 32/byte_size_in_bits/word_size_in_bits;
	bm_reg_size.dram_cache_conf            = 32/byte_size_in_bits/word_size_in_bits;
	bm_reg_size.dram_qos_conf              = 32/byte_size_in_bits/word_size_in_bits;
	bm_reg_size.error_intr_cause           = 32/byte_size_in_bits/word_size_in_bits;
	bm_reg_size.error_intr_mask            = 32/byte_size_in_bits/word_size_in_bits;
	bm_reg_size.func_intr_cause            = 32/byte_size_in_bits/word_size_in_bits;
	bm_reg_size.func_intr_mask             = 32/byte_size_in_bits/word_size_in_bits;
	bm_reg_size.ecc_err_intr_cause         = 32/byte_size_in_bits/word_size_in_bits;
	bm_reg_size.ecc_err_intr_mask          = 32/byte_size_in_bits/word_size_in_bits;

	for (bid = 0; bid < 5; bid++) {
		bm_reg_size.pool_nempty_intr_cause[bid] = 32/byte_size_in_bits/word_size_in_bits;
		bm_reg_size.pool_nempty_intr_mask[bid]  = 32/byte_size_in_bits/word_size_in_bits;
		bm_reg_size.dpool_ae_intr_cause[bid]    = 32/byte_size_in_bits/word_size_in_bits;
		bm_reg_size.dpool_ae_intr_mask[bid]     = 32/byte_size_in_bits/word_size_in_bits;
		bm_reg_size.dpool_af_intr_cause[bid]    = 32/byte_size_in_bits/word_size_in_bits;
		bm_reg_size.dpool_af_intr_mask[bid]     = 32/byte_size_in_bits/word_size_in_bits;
	}

	bm_reg_size.b_bank_req_fifos_st     = 32/byte_size_in_bits/word_size_in_bits;
	bm_reg_size.b0_past_alc_fifos_st    = 32/byte_size_in_bits/word_size_in_bits;
	bm_reg_size.bgp_past_alc_fifos_st   = 32/byte_size_in_bits/word_size_in_bits;
	bm_reg_size.b0_rls_wrp_ppe_fifos_st = 32/byte_size_in_bits/word_size_in_bits;
	bm_reg_size.dm_axi_fifos_st         = 32/byte_size_in_bits/word_size_in_bits;
	bm_reg_size.drm_pend_fifo_st        = 32/byte_size_in_bits/word_size_in_bits;
	bm_reg_size.dm_axi_wr_pend_fifo_st  = 32/byte_size_in_bits/word_size_in_bits;
	bm_reg_size.bm_idle_st              = 32/byte_size_in_bits/word_size_in_bits;

	for (bid = 0; bid < 5; bid++) {
		bm_reg_size.dpr_c_mng_stat[bid]  =  96/byte_size_in_bits/word_size_in_bits;
		bm_reg_size.tpr_c_mng_b_dyn[bid] =  64/byte_size_in_bits/word_size_in_bits;
		bm_reg_size.tpr_ctrs_0_b[bid]    = 128/byte_size_in_bits/word_size_in_bits;
		bm_reg_size.sram_b_cache[bid]    =  64/byte_size_in_bits/word_size_in_bits;
	}

	bm_reg_size.dpr_d_mng_ball_stat  = 160/byte_size_in_bits/word_size_in_bits;
	bm_reg_size.tpr_dro_mng_ball_dyn =  64/byte_size_in_bits/word_size_in_bits;
	bm_reg_size.tpr_drw_mng_ball_dyn =  32/byte_size_in_bits/word_size_in_bits;
	bm_reg_size.sram_b_cache[0]      = 128/byte_size_in_bits/word_size_in_bits;

	rc = OK;
	return rc;
}

int bm_reg_offset_alias_init(void)
{
	int rc = -BM_ALIAS_ERROR;
	u32 bid/*, word_size = 32/8*/;
	/*memset(&bm_reg_offset,0,sizeof(bm_reg_offset));*/

	for (bid = 0; bid < 5; bid++) {
		bm_reg_offset.b_pool_n_conf[bid]             = 8;
		bm_reg_offset.b_pool_n_st[bid]               = 8;
		bm_reg_offset.b_sys_rec_bank_intr_cause[bid] = 0;
		bm_reg_offset.b_sys_rec_bank_intr_mask[bid]  = 0;
		bm_reg_offset.b_sys_rec_bank_d0_st[bid]      = 0;
		bm_reg_offset.b_sys_rec_bank_d1_st[bid]      = 0;
	}

	bm_reg_offset.sw_debug_rec_intr_cause    = 0;
	bm_reg_offset.sw_debug_rec_intr_mask     = 0;
	bm_reg_offset.sys_nrec_common_intr_cause = 0;
	bm_reg_offset.sys_nrec_common_intr_mask  = 0;
	bm_reg_offset.sys_nrec_common_d0_st      = 0;
	bm_reg_offset.sys_nrec_common_d1_st      = 0;
	bm_reg_offset.sys_nrec_common_d2_st      = 0;
	bm_reg_offset.sys_nrec_common_d3_st      = 0;
	bm_reg_offset.common_general_conf        = 0;
	bm_reg_offset.dram_domain_conf           = 0;
	bm_reg_offset.dram_cache_conf            = 0;
	bm_reg_offset.dram_qos_conf              = 0;
	bm_reg_offset.error_intr_cause           = 0;
	bm_reg_offset.error_intr_mask            = 0;
	bm_reg_offset.func_intr_cause            = 0;
	bm_reg_offset.func_intr_mask             = 0;
	bm_reg_offset.ecc_err_intr_cause         = 0;
	bm_reg_offset.ecc_err_intr_mask          = 0;

	for (bid = 0; bid < 5; bid++) {
		bm_reg_offset.pool_nempty_intr_cause[bid] = 0;
		bm_reg_offset.pool_nempty_intr_mask[bid]  = 0;
		bm_reg_offset.dpool_ae_intr_cause[bid]    = 0;
		bm_reg_offset.dpool_ae_intr_mask[bid]     = 0;
		bm_reg_offset.dpool_af_intr_cause[bid]    = 0;
		bm_reg_offset.dpool_af_intr_mask[bid]     = 0;
	}

	bm_reg_offset.b_bank_req_fifos_st     = 4;
	bm_reg_offset.b0_past_alc_fifos_st    = 0;
	bm_reg_offset.bgp_past_alc_fifos_st   = 0;
	bm_reg_offset.b0_rls_wrp_ppe_fifos_st = 0;
	bm_reg_offset.dm_axi_fifos_st         = 0;
	bm_reg_offset.drm_pend_fifo_st        = 0;
	bm_reg_offset.dm_axi_wr_pend_fifo_st  = 0;
	bm_reg_offset.bm_idle_st              = 0;

	for (bid = 0; bid < 5; bid++) {
		bm_reg_offset.dpr_c_mng_stat[bid]  = 16;
		bm_reg_offset.tpr_c_mng_b_dyn[bid] =  8;
		bm_reg_offset.tpr_ctrs_0_b[bid]    = 16;
		bm_reg_offset.sram_b_cache[bid]    =  8;
	}

	bm_reg_offset.dpr_d_mng_ball_stat  = 32;
	bm_reg_offset.tpr_dro_mng_ball_dyn =  8;
	bm_reg_offset.tpr_drw_mng_ball_dyn =  4;
	bm_reg_offset.sram_b_cache[0]      = 16;

	rc = OK;
	return rc;
}

int bm_register_name_get(u32 reg_base_address, u32 reg_offset, char *reg_name)
{
/*	u32 reg_size; */
	int rc = -BM_ALIAS_ERROR;
	u32 pid, bid, pid_local, global_pool_idx, line;

	for (bid = 0; bid < 5; bid++) {
		if (reg_base_address == bm.b_pool_n_conf[bid]) {
			pid_local = reg_offset / bm_reg_offset.b_pool_n_conf[bid];
			/*
			sprintf_s(reg_name, sizeof(reg_name), "b%d_pool_%d_conf", bid, pid_local); */
			sprintf(reg_name, "b%d_pool_%d_conf", bid, pid_local);
			return OK;
		} else if (reg_base_address == bm.b_pool_n_st[bid]) {
			pid_local = reg_offset / bm_reg_offset.b_pool_n_st[bid];
			/*
			sprintf_s(reg_name, sizeof(reg_name), "b%d_pool_%d_st", bid, pid_local); */
			sprintf(reg_name, "b%d_pool_%d_st", bid, pid_local);
			return OK;
		} else if (reg_base_address == bm.b_sys_rec_bank_intr_cause[bid]) {
			/*
			sprintf_s(reg_name, sizeof(reg_name), "b%d_sys_rec_bank_intr_cause", bid); */
			sprintf(reg_name, "b%d_sys_rec_bank_intr_cause", bid);
			return OK;
		} else if (reg_base_address == bm.b_sys_rec_bank_intr_mask[bid]) {
			/*
			sprintf_s(reg_name, sizeof(reg_name), "b%_sys_rec_bank_intr_mask", bid); */
			sprintf(reg_name, "b%d_sys_rec_bank_intr_mask", bid);
			return OK;
		} else if (reg_base_address == bm.b_sys_rec_bank_d0_st[bid]) {
			/*
			sprintf_s(reg_name, sizeof(reg_name), "b%_sys_rec_bank_d0_st", bid); */
			sprintf(reg_name, "b%d_sys_rec_bank_d0_st", bid);
			return OK;
		} else if (reg_base_address == bm.b_sys_rec_bank_d1_st[bid]) {
			/*
			sprintf_s(reg_name, sizeof(reg_name), "b%_sys_rec_bank_d1_st", bid); */
			sprintf(reg_name, "b%d_sys_rec_bank_d1_st", bid);
			return OK;
		}
	}

	if (reg_base_address == bm.sw_debug_rec_intr_cause) {
		sprintf(reg_name, "sw_debug_rec_intr_cause");
		return OK;
	} else if (reg_base_address == bm.sw_debug_rec_intr_mask) {
		sprintf(reg_name, "sw_debug_rec_intr_mask");
		return OK;
	} else if (reg_base_address == bm.sys_nrec_common_intr_cause) {
		sprintf(reg_name, "sys_nrec_common_intr_cause");
		return OK;
	} else if (reg_base_address == bm.sys_nrec_common_intr_mask) {
		sprintf(reg_name, "sys_nrec_common_intr_mask");
		return OK;
	} else if (reg_base_address == bm.sys_nrec_common_d0_st) {
		sprintf(reg_name, "sys_nrec_common_d0_st");
		return OK;
	} else if (reg_base_address == bm.sys_nrec_common_d1_st) {
		sprintf(reg_name, "sys_nrec_common_d1_st");
		return OK;
	} else if (reg_base_address == bm.sys_nrec_common_d2_st) {
		sprintf(reg_name, "sys_nrec_common_d2_st");
		return OK;
	} else if (reg_base_address == bm.sys_nrec_common_d3_st) {
		sprintf(reg_name, "sys_nrec_common_d3_st");
		return OK;
	} else if (reg_base_address == bm.common_general_conf) {
		sprintf(reg_name, "common_general_conf");
		return OK;
	} else if (reg_base_address == bm.dram_domain_conf) {
		sprintf(reg_name, "dram_domain_conf");
		return OK;
	} else if (reg_base_address == bm.dram_cache_conf) {
		sprintf(reg_name, "dram_cache_conf");
		return OK;
	} else if (reg_base_address == bm.dram_qos_conf) {
		sprintf(reg_name, "dram_qos_conf");
		return OK;
	} else if (reg_base_address == bm.error_intr_cause) {
		sprintf(reg_name, "error_intr_cause");
		return OK;
	} else if (reg_base_address == bm.error_intr_mask) {
		sprintf(reg_name, "error_intr_mask");
		return OK;
	} else if (reg_base_address == bm.func_intr_cause) {
		sprintf(reg_name, "func_intr_cause");
		return OK;
	} else if (reg_base_address == bm.func_intr_mask) {
		sprintf(reg_name, "func_intr_mask");
		return OK;
	} else if (reg_base_address == bm.ecc_err_intr_cause) {
		sprintf(reg_name, "ecc_err_intr_cause");
		return OK;
	} else if (reg_base_address == bm.ecc_err_intr_mask) {
		sprintf(reg_name, "ecc_err_intr_mask");
		return OK;
	}

	for (bid = 0; bid < 5; bid++) {
		if (reg_base_address == bm.pool_nempty_intr_cause[bid]) {
			sprintf(reg_name, "b%d_pool_nempty_intr_cause", bid);
			return OK;
		} else if (reg_base_address == bm.pool_nempty_intr_mask[bid]) {
			sprintf(reg_name, "b%d_pool_nempty_intr_mask", bid);
			return OK;
		} else if (reg_base_address == bm.dpool_ae_intr_cause[bid]) {
			sprintf(reg_name, "b%d_dpool_ae_intr_cause", bid);
			return OK;
		} else if (reg_base_address == bm.dpool_ae_intr_mask[bid]) {
			sprintf(reg_name, "b%d_dpool_ae_intr_mask", bid);
			return OK;
		} else if (reg_base_address == bm.dpool_af_intr_cause[bid]) {
			sprintf(reg_name, "b%d_dpool_af_intr_cause", bid);
			return OK;
		} else if (reg_base_address == bm.dpool_af_intr_mask[bid]) {
			sprintf(reg_name, "b%d_dpool_af_intr_mask", bid);
			return OK;
		}
	}

	if (reg_base_address == bm.b_bank_req_fifos_st) {
		bid = reg_offset / bm_reg_offset.b_bank_req_fifos_st;
		sprintf(reg_name, "b%d_bank_req_fifos_st", bid);
		return OK;
	} else if  (reg_base_address == bm.b0_past_alc_fifos_st) {
		sprintf(reg_name, "b0_past_alc_fifos_st");
		return OK;
	} else if  (reg_base_address == bm.bgp_past_alc_fifos_st) {
		sprintf(reg_name, "bgp_past_alc_fifos_st");
		return OK;
	} else if  (reg_base_address == bm.b0_rls_wrp_ppe_fifos_st) {
		sprintf(reg_name, "b0_rls_wrp_ppe_fifos_st");
		return OK;
	} else if  (reg_base_address == bm.dm_axi_fifos_st) {
		sprintf(reg_name, "dm_axi_fifos_st");
		return OK;
	} else if  (reg_base_address == bm.drm_pend_fifo_st) {
		sprintf(reg_name, "drm_pend_fifo_st");
		return OK;
	} else if  (reg_base_address == bm.dm_axi_wr_pend_fifo_st) {
		sprintf(reg_name, "dm_axi_wr_pend_fifo_st");
		return OK;
	} else if  (reg_base_address == bm.bm_idle_st) {
		sprintf(reg_name, "bm_idle_st");
		return OK;
	}

	for (bid = 0; bid < 5; bid++) {
		if  (reg_base_address == bm.dpr_c_mng_stat[bid]) {
			pid_local = reg_offset / bm_reg_offset.dpr_c_mng_stat[bid];
			sprintf(reg_name, "dpr_c_mng_b%d_stat_PID_local_%d", bid, pid_local);
			return OK;
		} else if  (reg_base_address == bm.tpr_c_mng_b_dyn[bid]) {
			pid_local = reg_offset / bm_reg_offset.tpr_c_mng_b_dyn[bid];
			sprintf(reg_name, "tpr_c_mng_b%d_dyn_PID_local_%d", bid, pid_local);
			return OK;
		} else if  (reg_base_address == bm.tpr_ctrs_0_b[bid]) {
			pid_local = reg_offset / bm_reg_offset.tpr_ctrs_0_b[bid];
			sprintf(reg_name, "tpr_ctrs_0_b%d_PID_local_%d", bid, pid_local);
			return OK;
		} else if  (reg_base_address == bm.sram_b_cache[bid]) {
			line = reg_offset / bm_reg_offset.sram_b_cache[bid];
			sprintf(reg_name, "sram_b%d_cache_line_%04d", bid, line);
			return OK;
		}
	}

	if  (reg_base_address == bm.dpr_d_mng_ball_stat) {
		global_pool_idx = reg_offset / bm_reg_offset.dpr_d_mng_ball_stat;
		pid = BM_GLOBAL_POOL_IDX_TO_PID(global_pool_idx);
		sprintf(reg_name, "dpr_d_mng_ball_stat_PID_%02d", pid);
		return OK;
	} else if  (reg_base_address == bm.tpr_dro_mng_ball_dyn) {
		global_pool_idx = reg_offset / bm_reg_offset.tpr_dro_mng_ball_dyn;
		pid = BM_GLOBAL_POOL_IDX_TO_PID(global_pool_idx);
		sprintf(reg_name, "tpr_dro_mng_ball_dyn_PID_%02d", pid);
		return OK;
	} else if  (reg_base_address == bm.tpr_drw_mng_ball_dyn) {
		global_pool_idx = reg_offset / bm_reg_offset.tpr_drw_mng_ball_dyn;
		pid = BM_GLOBAL_POOL_IDX_TO_PID(global_pool_idx);
		sprintf(reg_name, "tpr_drw_mng_ball_dyn_PID_%02d", pid);
		return OK;
	}

	sprintf(reg_name, "unknown");
	return rc;
}

int bm_pid_bid_init()
{
	u32 pid, bid, pid_local, pool, global_pool_idx;
	int rc = -BM_INPUT_NOT_IN_RANGE;

	for (pool = BM_POOL_QM_MIN; pool < BM_POOL_QM_MAX; pool++) {
		if ((pool            <            BM_POOL_QM_MIN) || (pool            >            BM_POOL_QM_MAX))
			return rc;

		pid       = (int)pool;
		if ((pid             <            BM_POOL_QM_MIN) || (pid             >            BM_POOL_QM_MAX))
			return rc;

		bid       = BM_PID_TO_BANK(pid);
		if ((bid             <            BM_BANK_QM_MIN) || (bid             >            BM_BANK_QM_MAX))
			return rc;

		pid_local = BM_PID_TO_PID_LOCAL(pid);
		if ((pid_local       <       BM_PID_LOCAL_QM_MIN) || (pid_local       >       BM_PID_LOCAL_QM_MAX))
			return rc;

		global_pool_idx = BM_PID_TO_GLOBAL_POOL_IDX(pid);
		if ((global_pool_idx < BM_GLOBAL_POOL_IDX_QM_MIN) || (global_pool_idx > BM_GLOBAL_POOL_IDX_QM_MAX))
			return rc;

		pid = BM_GLOBAL_POOL_IDX_TO_PID(global_pool_idx);
		if ((pid             <            BM_POOL_QM_MIN) || (pid             >            BM_POOL_QM_MAX))
			return rc;
	}

	for (pool = BM_POOL_GP_MIN; pool < BM_POOL_GP_MAX; pool++) {
		if ((pool            <            BM_POOL_GP_MIN) || (pool            >            BM_POOL_GP_MAX))
			return rc;

		pid       = (int)pool;
		if ((pid             <            BM_POOL_GP_MIN) || (pid             >            BM_POOL_GP_MAX))
			return rc;

		bid       = BM_PID_TO_BANK(pid);
		if ((bid             <            BM_BANK_GP_MIN) || (bid             >            BM_BANK_GP_MAX))
			return rc;

		pid_local = BM_PID_TO_PID_LOCAL(pid);
		if ((pid_local       <       BM_PID_LOCAL_GP_MIN) || (pid_local       >       BM_PID_LOCAL_GP_MAX))
			return rc;

		global_pool_idx = BM_PID_TO_GLOBAL_POOL_IDX(pid);
		if ((global_pool_idx < BM_GLOBAL_POOL_IDX_GP_MIN) || (global_pool_idx > BM_GLOBAL_POOL_IDX_GP_MAX))
			return rc;

		pid = BM_GLOBAL_POOL_IDX_TO_PID(global_pool_idx);
		if ((pid             <            BM_POOL_GP_MIN) || (pid             >            BM_POOL_GP_MAX))
			return rc;
	}

	rc = OK;
	return rc;
}


/*
*/
