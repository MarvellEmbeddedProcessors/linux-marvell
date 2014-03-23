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

#ifndef	__MV_BM_REGS_H__
#define	__MV_BM_REGS_H__

#include "bm/mv_bm.h"

#define BM_UNIT_OFFSET		0x0D0000
#define QM_UNIT_OFFSET		0x400000

/*********************************/
/* Internal Databases Structures */
/*********************************/

/** Global arrays structures definitions */

/*
typedef void * bm_handle;
*/

/*
struct bm_dpr_c_mng_stat {
    u32 bank;
    u32 cache_start;
    u32 cache_end;
    u32 cache_si_thr;
    u32 cache_so_thr;
    u32 cache_attr;
    u32 cache_vmid;
} __ATTRIBUTE_PACKED__;
*/

struct bm_c_mng_stat_data {
	u32 cache_start:7;		/* byte[ 0-11],bit[ 0- 7]	0x0 */
	u32 _reserved_1:9;		/* byte[ 0-11],bit[ 8-15] */
	u32 cache_end:7;		/* byte[ 0-11],bit[16-22]	0x0 */
	u32 _reserved_2:9;		/* byte[ 0-11],bit[23-31] */
	u32 cache_si_thr:10;	/* byte[ 0-11],bit[32-41]	0x0 */
	u32 _reserved_3:6;		/* byte[ 0-11],bit[42-47] */
	u32 cache_so_thr:10;	/* byte[ 0-11],bit[48-57]	0x0 */
	u32 _reserved_4:6;		/* byte[ 0-11],bit[58-63] */
	u32 cache_attr:8;		/* byte[ 0-11],bit[64-71]	0x0 */
	u32 _reserved_5:8;		/* byte[ 0-11],bit[72-79] */
	u32 cache_vmid:8;		/* byte[ 0-11],bit[80-87]	0x0 */
	u32 _reserved_6:8;		/* byte[ 0-11],bit[88-95] */
} __ATTRIBUTE_PACKED__;

/*
struct bm_dpr_c_mng_stat {
	struct bm_c_mng_stat_data reg_c_mng_stat_data[5][31];	/ * byte[ 0- 0x3FFF] * /
} __ATTRIBUTE_PACKED__;

struct bm_tpr_c_mng_dyn {
	u32 cache_fill_min;
	u32 cache_fill_max;
	u32 cache_rd_ptr;
	u32 cache_wr_ptr;
} __ATTRIBUTE_PACKED__;
*/

struct bm_c_mng_dyn_data {
	u32 cache_fill_min:10;		/* byte[ 0- 7],bit[ 0- 9]	0x0 */
	u32 _reserved_1:6;			/* byte[ 0- 7],bit[10-15] */
	u32 cache_fill_max:10;		/* byte[ 0- 7],bit[16-25]	0x0 */
	u32 _reserved_2:6;			/* byte[ 0- 7],bit[25-31] */
	u32 cache_rd_ptr:10;		/* byte[ 0- 7],bit[32-41]	0x0 */
	u32 _reserved_3:6;			/* byte[ 0- 7],bit[42-47] */
	u32 cache_wr_ptr:10;		/* byte[ 0- 7],bit[48-57]	0x0 */
	u32 _reserved_4:6;			/* byte[ 0- 7],bit[58-63] */
} __ATTRIBUTE_PACKED__;

/*
struct bm_tpr_c_mng_dyn {
	struct bm_c_mng_dyn_data reg_c_mng_dyn_data[5][31];	/ * byte[ 0- 0x3FFF] * /
} __ATTRIBUTE_PACKED__;
*/

struct bm_d_mng_ball_stat_data {
	u32 dram_ae_thr:18;			/* byte[ 0-20],bit[ 0-17]	0x0 */
	u32 _reserved_1:14;			/* byte[ 0-20],bit[18-31] */
	u32 dram_af_thr:18;			/* byte[ 0-20],bit[32-49]	0x0 */
	u32 _reserved_2:14;			/* byte[ 0-20],bit[50-63] */
/*	int64_t dram_start:40;		 byte[ 0-20],bit[ 64-103]	0x0 */
	u32 dram_start_lo:32;		/* byte[ 0-20],bit[ 64- 95]	0x0 */
	u32 dram_start_hi:8;		/* byte[ 0-20],bit[ 97-103]	0x0 */
	u32 _reserved_3:24;			/* byte[ 0-20],bit[104-127] */
	u32 dram_size:18;			/* byte[ 0-20],bit[128-145]	0x0 */
	u32 _reserved_4:14;			/* byte[ 0-20],bit[146-159] */
} __ATTRIBUTE_PACKED__;

/*
struct bm_dpr_d_mng_ball_stat {
	struct bm_d_mng_ball_stat_data reg_d_mng_ball_stat_data[POOL_NUM];	*//* byte[ 0- 0x3FFF] 128+4 *//*
} __ATTRIBUTE_PACKED__;
*/

struct bm_tpr_dro_mng_ball_dyn_data {
	u32 dram_rd_ptr:21;		/* byte[ 0- 7],bit[ 0-20]	0x0 */
	u32 _reserved_1:11;		/* byte[ 0- 7],bit[21-31] */
	u32 dram_wr_ptr:21;		/* byte[ 0- 7],bit[32-52]	0x0 */
	u32 _reserved_2:11;		/* byte[ 0- 7],bit[53-63] */
} __ATTRIBUTE_PACKED__;

/*
struct bm_tpr_dro_mng_ball_dyn {
	struct bm_tpr_dro_mng_ball_dyn_data reg_tpr_dro_mng_ball_dyn_data[POOL_NUM];	*//* byte[ 0- 0x3FFF] *//*
} __ATTRIBUTE_PACKED__;
*/
struct bm_tpr_drw_mng_ball_dyn_data {
	u32 dram_fill:21;		/* byte[ 0- 3],bit[ 0-20]	0x0 */
	u32 _reserved_1:11;		/* byte[ 0- 3],bit[21-31] */
} __ATTRIBUTE_PACKED__;

/*
struct bm_tpr_drw_mng_ball_dyn {
	struct bm_tpr_drw_mng_ball_dyn_data reg_tpr_drw_mng_ball_dyn_data[POOL_NUM];	*//* byte[ 0- 0x3FFF] *//*
} __ATTRIBUTE_PACKED__;
*/
struct bm_tpr_ctrs_0_data {
	u32 delayed_releases_ctr:32;	/* byte[ 0-15],bit[ 0-31]	0x0 */
	u32 failed_allocs_ctr:32;		/* byte[ 0-15],bit[32-63]	0x0 */
	u32 released_pes_ctr:32;		/* byte[ 0-15],bit[64-95]	0x0 */
	u32 allocated_pes_ctr:32;		/* byte[ 0-15],bit[96-127]	0x0 */
} __ATTRIBUTE_PACKED__;

/*
struct bm_tpr_ctrs_0 {
	struct bm_tpr_ctrs_0_data reg_tpr_ctrs_0_data[5][31];	*//* byte[ 0- 0x3FFF] *//*
} __ATTRIBUTE_PACKED__;
*/

/*
struct bm_sram_cache{
	u32 cache_b0_data_0;
	u32 cache_b0_data_1;
	u32 cache_b0_data_2;
	u32 cache_b0_data_3;
	u32 cache_bgp_data;
} __ATTRIBUTE_PACKED__;
*/
struct bm_cache_b0_mem_data {
	u32 cache_b0_data_0:22;			/* byte[ 0- 3],bit[  0- 21]	0x0 */
	u32 _reserved_1:10;				/* byte[ 0- 3],bit[ 22- 31] */
	u32 cache_b0_data_1:22;			/* byte[ 4- 7],bit[ 32- 53]	0x0 */
	u32 _reserved_2:10;				/* byte[ 4- 7],bit[ 54- 63] */
	u32 cache_b0_data_2:22;			/* byte[ 8-11],bit[ 64- 85]	0x0 */
	u32 _reserved_3:10;				/* byte[ 8-11],bit[ 86- 95] */
	u32 cache_b0_data_3:22;			/* byte[12-15],bit[ 96-117]	0x0 */
	u32 _reserved_4:10;				/* byte[12-15],bit[118-127] */
} __ATTRIBUTE_PACKED__;

struct bm_cache_bgp_data {
/*	uint64_t cache_bgp_data:40; */	/* byte[ 0- 7],bit[ 0-39]	0x0 */
	u32 cache_bgp_data_lo:32;		/* byte[ 0- 7],bit[ 0-31]	0x0 */
	u32 cache_bgp_data_hi:8;		/* byte[ 0- 7],bit[32-39]	0x0 */
	u32 _reserved:24;				/* byte[ 0- 7],bit[40-63] */
} __ATTRIBUTE_PACKED__;

/*
struct bm_sram_cache {
	struct bm_cache_b0_mem_data reg_cache_b0_data[31];	*//* byte[ 0- 0x3FFF] *//*
	struct bm_cache_bgp_data    reg_cache_b1_data[31];	*//* byte[ 0- 0x3FFF] *//*
	struct bm_cache_bgp_data    reg_cache_b2_data[31];	*//* byte[ 0- 0x3FFF] *//*
	struct bm_cache_bgp_data    reg_cache_b3_data[31];	*//* byte[ 0- 0x3FFF] *//*
	struct bm_cache_bgp_data    reg_cache_b4_data[31];	*//* byte[ 0- 0x3FFF] *//*
} __ATTRIBUTE_PACKED__;
*/

struct bm_pool_conf_b0 {
	u32 pool_enable:1;				/* byte[ 0- 3],bit[ 0- 0] */
	u32 pool_quick_init:1;			/* byte[ 0- 3],bit[ 1- 1] */
	u32 _reserved:30;				/* byte[ 0- 3],bit[ 2-31] */
} __ATTRIBUTE_PACKED__;

struct bm_pool_conf_bgp {
	u32 pool_enable:1;				/* byte[ 0- 3],bit[ 0- 0] */
	u32 pool_in_pairs:1;			/* byte[ 0- 3],bit[ 1- 1] */
	u32 pe_size:1;					/* byte[ 0- 3],bit[ 2- 2] */
	u32 pool_quick_init:1;			/* byte[ 0- 3],bit[ 3- 3] */
	u32 _reserved:28;				/* byte[ 0- 3],bit[ 4-31] */
} __ATTRIBUTE_PACKED__;

struct bm_pool_st {
	u32 pool_nempty_st:1;			/* byte[ 0- 3],bit[ 0- 0] */
	u32 dpool_ae_st:1;				/* byte[ 0- 3],bit[ 1- 1] */
	u32 dpool_af_st:1;				/* byte[ 0- 3],bit[ 2- 2] */
	u32 pool_fill_bgt_si_thr_st:1;	/* byte[ 0- 3],bit[ 3- 3] */
	u32 _reserved:28;				/* byte[ 0- 3],bit[ 4-31] */
} __ATTRIBUTE_PACKED__;

#ifdef my_HIDE
struct BM_pool {
	struct BM_pool_conf_b0  reg_b0_pool_conf[31];
	struct BM_pool_conf_bgp reg_bgp_pool_conf[31];
	struct BM_pool_st       reg_pool_st[31];
} __ATTRIBUTE_PACKED__;

struct bm_pool_conf {
	u32 pool_enable:1;				/* byte[ 0- 3],bit[ 0- 0]	0x0 */
	u32 pool_in_pairs:1;			/* byte[ 0- 3],bit[ 1- 1]	0x0 */
	u32 PE_size:1;					/* byte[ 0- 3],bit[ 2- 2]	0x1 */
	u32 pool_quick_init:1;			/* byte[ 0- 3],bit[ 3- 3]	0x1 */
	u32 _reserved:28;				/* byte[ 0- 3],bit[ 4-31] */
} __ATTRIBUTE_PACKED__;

struct bm_pool_st {
	u32 pool_nempty_st:1;			/* byte[ 0- 3],bit[ 0- 0]	0x0 */
	u32 dpool_ae_st:1;				/* byte[ 0- 3],bit[ 1- 1]	0x0 */
	u32 dpool_af_st:1;				/* byte[ 0- 3],bit[ 2- 2]	0x0 */
	u32 pool_fill_bgt_si_thr_st:1;	/* byte[ 0- 3],bit[ 3- 3]	0x0 */
	u32 _reserved:28;				/* byte[ 0- 3],bit[ 4-31] */
} __ATTRIBUTE_PACKED__;
#endif

struct bm_b0_internal_dbg_nrec_bank_st {
	u32 alc_fifo_of_st:1;			/* byte[ 0- 3],bit[ 0- 0]	0x0 */
	u32 alc_fifo_uf_st:1;			/* byte[ 0- 3],bit[ 1- 1]	0x0 */
	u32 alc_resp_fifo_of_st:1;		/* byte[ 0- 3],bit[ 2- 2]	0x0 */
	u32 alc_resp_fifo_uf_st:1;		/* byte[ 0- 3],bit[ 3- 3]	0x0 */
	u32 past_alc_fifo_of_st:1;		/* byte[ 0- 3],bit[ 4- 4]	0x0 */
	u32 past_alc_fifo_uf_st:1;		/* byte[ 0- 3],bit[ 5- 5]	0x0 */
	u32 past_alc_ppe_fifo_of_st:1;	/* byte[ 0- 3],bit[ 6- 6]	0x0 */
	u32 past_alc_ppe_fifo_uf_st:1;	/* byte[ 0- 3],bit[ 7- 7]	0x0 */
	u32 rls_fifo_of_st:1;			/* byte[ 0- 3],bit[ 8- 8]	0x0 */
	u32 rls_fifo_uf_st:1;			/* byte[ 0- 3],bit[ 9- 9]	0x0 */
	u32 si_fifo_of_st:1;			/* byte[ 0- 3],bit[10-10]	0x0 */
	u32 si_fifo_uf_st:1;			/* byte[ 0- 3],bit[11-11]	0x0 */
	u32 si_size_viol_in_pipe_st:1;	/* byte[ 0- 3],bit[12-12]	0x0 */
	u32 so_fifo_of_st:1;			/* byte[ 0- 3],bit[13-13]	0x0 */
	u32 so_fifo_uf_st:1;			/* byte[ 0- 3],bit[14-14]	0x0 */
	u32 so_size_viol_in_pipe_st:1;	/* byte[ 0- 3],bit[15-15]	0x0 */
	u32 dram_w_fifo_of_st:1;		/* byte[ 0- 3],bit[16-16]	0x0 */
	u32 dram_w_fifo_uf_st:1;		/* byte[ 0- 3],bit[17-17]	0x0 */
	u32 _reserved:14;				/* byte[ 0- 3],bit[18-31] */
} __ATTRIBUTE_PACKED__;

struct bm_b0_internal_dbg_nrec_pool_st {
	u32 b0_alc_resp_ppe_fifos_of_st:4;	/* byte[ 0- 3],bit[ 0- 3]	0x0 */
	u32 _reserved_1:4;					/* byte[ 0- 3],bit[ 4- 7] */
	u32 b0_alc_resp_ppe_fifos_uf_st:4;	/* byte[ 0- 3],bit[ 8-11]	0x0 */
	u32 _reserved_2:4;					/* byte[ 0- 3],bit[12-15] */
	u32 b0_rls_wrp_ppe_fifos_of_st:4;	/* byte[ 0- 3],bit[16-19]	0x0 */
	u32 _reserved_3:4;					/* byte[ 0- 3],bit[20-23] */
	u32 b0_rls_wrp_ppe_fifos_uf_st:4;	/* byte[ 0- 3],bit[24-27]	0x0 */
	u32 _reserved_4:4;					/* byte[ 0- 3],bit[28-31] */
} __ATTRIBUTE_PACKED__;

struct bm_bgp_internal_dbg_nrec_bank_st {
	u32 alc_fifo_of_st:1;				/* byte[ 0- 3],bit[ 0- 0]	0x0 */
	u32 alc_fifo_uf_st:1;				/* byte[ 0- 3],bit[ 1- 1]	0x0 */
	u32 alc_resp_fifo_of_st:1;			/* byte[ 0- 3],bit[ 2- 2]	0x0 */
	u32 alc_resp_fifo_uf_st:1;			/* byte[ 0- 3],bit[ 3- 3]	0x0 */
	u32 past_alc_fifo_of_st:1;			/* byte[ 0- 3],bit[ 4- 4]	0x0 */
	u32 past_alc_fifo_uf_st:1;			/* byte[ 0- 3],bit[ 5- 5]	0x0 */
	u32 rls_fifo_of_st:1;				/* byte[ 0- 3],bit[ 6- 6]	0x0 */
	u32 rls_fifo_uf_st:1;				/* byte[ 0- 3],bit[ 7- 7]	0x0 */
	u32 si_fifo_of_st:1;				/* byte[ 0- 3],bit[ 8- 8]	0x0 */
	u32 si_fifo_uf_st:1;				/* byte[ 0- 3],bit[ 9- 9]	0x0 */
	u32 si_size_viol_in_pipe_st:1;		/* byte[ 0- 3],bit[10-10]	0x0 */
	u32 so_fifo_of_st:1;				/* byte[ 0- 3],bit[11-11]	0x0 */
	u32 so_fifo_uf_st:1;				/* byte[ 0- 3],bit[12-12]	0x0 */
	u32 so_size_viol_in_pipe_st:1;		/* byte[ 0- 3],bit[13-13]	0x0 */
	u32 dram_w_fifo_of_st:1;			/* byte[ 0- 3],bit[14-14]	0x0 */
	u32 dram_w_fifo_uf_st:1;			/* byte[ 0- 3],bit[15-15]	0x0 */
	u32 b_dwm_in_32b_pend_no_2nd_st:1;	/* byte[ 0- 3],bit[16-16]	0x0 */
	u32 _reserved:15;					/* byte[ 0- 3],bit[17-31] */
} __ATTRIBUTE_PACKED__;

struct bm_b_sys_rec_bank_intr_cause {
	u32 sys_rec_bank_intr_cause_sum:1;	/* byte[ 0- 3],bit[ 0- 0]	0x0 */
	u32 alc_vmid_mis_s:1;				/* byte[ 0- 3],bit[ 1- 1]	0x0 */
	u32 rls_vmid_mis_s:1;				/* byte[ 0- 3],bit[ 2- 2]	0x0 */
	u32 alc_dis_pool_s:1;				/* byte[ 0- 3],bit[ 3- 3]	0x0 */
	u32 rls_dis_pool_s:1;				/* byte[ 0- 3],bit[ 4- 4]	0x0 */
	u32 _reserved:27;					/* byte[ 0- 3],bit[ 5-31] */
} __ATTRIBUTE_PACKED__;

struct bm_sys_rec_bank_intr_mask {
	u32 _reserved_1:1;					/* byte[ 0- 3],bit[ 0- 0] */
	u32 alc_vmid_mis_m:1;				/* byte[ 0- 3],bit[ 1- 1]	0x0 */
	u32 rls_vmid_mis_m:1;				/* byte[ 0- 3],bit[ 2- 2]	0x0 */
	u32 alc_dis_pool_m:1;				/* byte[ 0- 3],bit[ 3- 3]	0x0 */
	u32 rls_dis_pool_m:1;				/* byte[ 0- 3],bit[ 4- 4]	0x0 */
	u32 _reserved_2:27;					/* byte[ 0- 3],bit[ 5-31] */
} __ATTRIBUTE_PACKED__;

struct bm_b_sys_rec_bank_d0_st {
	u32 b_last_vmid_mis_alc_vmid_st:8;	/* byte[ 0- 3],bit[ 0- 7]	0x0 */
	u32 b_last_vmid_mis_rls_vmid_st:8;	/* byte[ 0- 3],bit[ 8-15]	0x0 */
	u32 b_last_vmid_mis_alc_pid_st:8;	/* byte[ 0- 3],bit[16-23]	0x0 */
	u32 b_last_vmid_mis_rls_pid_st:8;	/* byte[ 0- 3],bit[24-31]	0x0 */
} __ATTRIBUTE_PACKED__;

struct bm_b_sys_rec_bank_d1_st {
	u32 b_last_vmid_mis_alc_src_st:2;	/* byte[ 0- 3],bit[ 0- 1]	0x0 */
	u32 b_last_vmid_mis_rls_src_st:2;	/* byte[ 0- 3],bit[ 2- 3]	0x0 */
	u32 b_last_alc_dis_pool_src_st:2;	/* byte[ 0- 3],bit[ 4- 5]	0x0 */
	u32 b_last_rls_dis_pool_src_st:2;	/* byte[ 0- 3],bit[ 6- 7]	0x0 */
	u32 b_last_alc_dis_pool_pid_st:8;	/* byte[ 0- 3],bit[ 8-15]	0x0 */
	u32 b_last_rls_dis_pool_pid_st:8;	/* byte[ 0- 3],bit[16-23]	0x0 */
	u32 _reserved:8;					/* byte[ 0- 3],bit[24-31] */
} __ATTRIBUTE_PACKED__;

#ifdef my_HIDE
struct bm_bgp_sys_rec_bank_d0_st {
	u32 b_last_vmid_mis_alc_vmid_st:8;	/* byte[ 0- 3],bit[ 0- 7]	0x0 */
	u32 b_last_vmid_mis_rls_vmid_st:8;	/* byte[ 0- 3],bit[ 8-15]	0x0 */
	u32 b_last_vmid_mis_alc_pid_st:8;	/* byte[ 0- 3],bit[16-23]	0x0 */
	u32 b_last_vmid_mis_rls_pid_st:8;	/* byte[ 0- 3],bit[24-31]	0x0 */
} __ATTRIBUTE_PACKED__;

struct bm_bgp_sys_rec_bank_d1_st {
	u32 b_last_vmid_mis_alc_src_st:2;	/* byte[ 0- 3],bit[ 0- 1]	0x0 */
	u32 b_last_vmid_mis_rls_src_st:2;	/* byte[ 0- 3],bit[ 2- 3]	0x0 */
	u32 b_last_alc_dis_pool_src_st:2;	/* byte[ 0- 3],bit[ 4- 5]	0x0 */
	u32 b_last_rls_dis_pool_src_st:2;	/* byte[ 0- 3],bit[ 6- 7]	0x0 */
	u32 b_last_alc_dis_pool_pid_st:8;	/* byte[ 0- 3],bit[ 8-15]	0x0 */
	u32 b_last_rls_dis_pool_pid_st:8;	/* byte[ 0- 3],bit[16-23]	0x0 */
	u32 _reserved:8;					/* byte[ 0- 3],bit[24-31] */
} __ATTRIBUTE_PACKED__;
#endif

struct bm_b_pool_nempty_intr_cause {
	u32 b_pool_nempty_intr_sum:1;		/* byte[ 0- 3],bit[ 0- 0]	0x0 */
	u32 b_pool_0_nempty_s:1;			/* byte[ 0- 3],bit[ 1- 1]	0x0 */
	u32 b_pool_1_nempty_s:1;			/* byte[ 0- 3],bit[ 2- 2]	0x0 */
	u32 b_pool_2_nempty_s:1;			/* byte[ 0- 3],bit[ 3- 3]	0x0 */
	u32 b_pool_3_nempty_s:1;			/* byte[ 0- 3],bit[ 4- 4]	0x0 */
	u32 b_pool_4_nempty_s:1;			/* byte[ 0- 3],bit[ 5- 5]	0x0 */
	u32 b_pool_5_nempty_s:1;			/* byte[ 0- 3],bit[ 6- 6]	0x0 */
	u32 b_pool_6_nempty_s:1;			/* byte[ 0- 3],bit[ 7- 7]	0x0 */
	u32 _reserved:8;					/* byte[ 0- 3],bit[ 8-31] */
} __ATTRIBUTE_PACKED__;

/*
struct bm_pool_nempty_intr_mask {
	u32 _reserved_1:1;					*//* byte[ 0- 3],bit[ 0- 0] *//*
	u32 nempty_m[31];					*//* byte[ 0- 3],bit[ 1-31]	0x0 *//*
} __ATTRIBUTE_PACKED__;
*/


struct bm_b_dpool_ae_intr_cause {
	u32 b_dpool_ae_intr_sum:1;			/* byte[ 0- 3],bit[ 0- 0]	0x0 */
	u32 b_dpool_0_ae_s:1;				/* byte[ 0- 3],bit[ 1- 1]	0x0 */
	u32 b_dpool_1_ae_s:1;				/* byte[ 0- 3],bit[ 2- 2]	0x0 */
	u32 b_dpool_2_ae_s:1;				/* byte[ 0- 3],bit[ 3- 3]	0x0 */
	u32 b_dpool_3_ae_s:1;				/* byte[ 0- 3],bit[ 4- 4]	0x0 */
	u32 b_dpool_4_ae_s:1;				/* byte[ 0- 3],bit[ 5- 5]	0x0 */
	u32 b_dpool_5_ae_s:1;				/* byte[ 0- 3],bit[ 6- 6]	0x0 */
	u32 b_dpool_6_ae_s:1;				/* byte[ 0- 3],bit[ 7- 7]	0x0 */
	u32 _reserved:8;					/* byte[ 0- 3],bit[ 8-31] */
} __ATTRIBUTE_PACKED__;

/*
struct bm_dpool_ae_intr_mask {
	u32 _reserved_1:1;				*//* byte[ 0- 3],bit[ 0- 0] *//*
	u32 ae_m[31];					*//* byte[ 0- 3],bit[ 1-31]	0x0 *//*
} __ATTRIBUTE_PACKED__;
*/

struct bm_b_dpool_af_intr_cause {
	u32 b_dpool_af_intr_sum:1;			/* byte[ 0- 3],bit[ 0- 0]	0x0 */
	u32 b_dpool_0_af_s:1;				/* byte[ 0- 3],bit[ 1- 1]	0x0 */
	u32 b_dpool_1_af_s:1;				/* byte[ 0- 3],bit[ 2- 2]	0x0 */
	u32 b_dpool_2_af_s:1;				/* byte[ 0- 3],bit[ 3- 3]	0x0 */
	u32 b_dpool_3_af_s:1;				/* byte[ 0- 3],bit[ 4- 4]	0x0 */
	u32 b_dpool_4_af_s:1;				/* byte[ 0- 3],bit[ 5- 5]	0x0 */
	u32 b_dpool_5_af_s:1;				/* byte[ 0- 3],bit[ 6- 6]	0x0 */
	u32 b_dpool_6_af_s:1;				/* byte[ 0- 3],bit[ 7- 7]	0x0 */
	u32 _reserved:8;					/* byte[ 0- 3],bit[ 8-31] */
} __ATTRIBUTE_PACKED__;

/*
struct bm_dpool_af_intr_mask {
	u32 _reserved_1:1;				*//* byte[ 0- 3],bit[ 0- 0] *//*
	u32 af_m[31];					*//* byte[ 0- 3],bit[ 1-31]	0x0 *//*
} __ATTRIBUTE_PACKED__;
*/
struct bm_b_bank_req_fifos_st {
	u32 b_alc_fifo_fill_st:8;			/* byte[ 0- 3],bit[ 0- 7]	0x0 */
	u32 b_rls_fifo_fill_st:8;			/* byte[ 0- 3],bit[ 8-15]	0x0 */
	u32 b_so_fifo_fill_st:8;			/* byte[ 0- 3],bit[16-23]	0x0 */
	u32 b_si_fifo_fill_st:8;			/* byte[ 0- 3],bit[24-31]	0x0 */
} __ATTRIBUTE_PACKED__;

struct bm_b0_past_alc_fifos_st {
	u32 b0_past_alc_fifo_fill_st:8;		/* byte[ 0- 3],bit[ 0- 7]	0x0 */
	u32 b0_past_alc_ppe_fifo_fill_st:8;	/* byte[ 0- 3],bit[ 8-15]	0x0 */
	u32 _reserved:16;					/* byte[ 0- 3],bit[16-31]	    */
} __ATTRIBUTE_PACKED__;

struct bm_bgp_past_alc_fifos_st {
	u32 b1_past_alc_fifo_fill_st:8;		/* byte[ 0- 3],bit[ 0- 7]	0x0 */
	u32 b2_past_alc_fifo_fill_st:8;		/* byte[ 0- 3],bit[ 8-15]	0x0 */
	u32 b3_past_alc_fifo_fill_st:8;		/* byte[ 0- 3],bit[16-23]	0x0 */
	u32 b4_past_alc_fifo_fill_st:8;		/* byte[ 0- 3],bit[24-31]	0x0 */
} __ATTRIBUTE_PACKED__;

struct bm_b0_rls_wrp_ppe_fifos_st {
	u32 rls_wrp_ppe_fifo_0_fill_st:8;	/* byte[ 0- 3],bit[ 0- 7]	0x0 */
	u32 rls_wrp_ppe_fifo_1_fill_st:8;	/* byte[ 0- 3],bit[ 8-15]	0x0 */
	u32 rls_wrp_ppe_fifo_2_fill_st:8;	/* byte[ 0- 3],bit[16-23]	0x0 */
	u32 rls_wrp_ppe_fifo_3_fill_st:8;	/* byte[ 0- 3],bit[24-31]	0x0 */
} __ATTRIBUTE_PACKED__;

/*
struct bm_bank_profile {
	struct bm_dpr_c_mng_stat                reg_dpr_c_mng_stat[31];/
	struct bm_tpr_c_mng_dyn                 reg_tpr_c_mng_dyn[31];
	struct bm_tpr_ctrs_0                    reg_tpr_ctrs_0[31];
	struct bm_sram_cache                    reg_sram_cache[31];
	struct bm_pool_conf                     reg_pool_conf[31];
	struct bm_pool_st                       reg_pool_st[31];
	struct bm_b0_internal_dbg_nrec_bank_st  reg_b0_internal_dbg_nrec_bank_st;
	struct bm_b0_internal_dbg_nrec_pool_st  reg_b0_internal_dbg_nrec_pool_st;
	struct bm_bgp_internal_dbg_nrec_bank_st reg_bgp_internal_dbg_nrec_bank_st;
	struct bm_sys_rec_bank_intr_cause       reg_sys_rec_bank_intr_cause;
	struct bm_sys_rec_bank_intr_mask        reg_sys_rec_bank_intr_mask;
	struct bm_sys_rec_bank_d0_st            reg_sys_rec_bank_d0_st;
	struct bm_sys_rec_bank_d1_st            reg_sys_rec_bank_d1_st;
	struct bm_pool_nempty_intr_cause        reg_pool_nempty_intr_cause;
	struct bm_pool_nempty_intr_mask         reg_pool_nempty_intr_mask;
	struct bm_dpool_ae_intr_cause           reg_dpool_ae_intr_cause;
	struct bm_dpool_ae_intr_mask            reg_dpool_ae_intr_mask;
	struct bm_dpool_af_intr_cause           reg_dpool_af_intr_cause;
	struct bm_dpool_af_intr_mask            reg_dpool_af_intr_mask;
	struct bm_bank_req_fifos_st             reg_req_fifos_fill_st;
} __ATTRIBUTE_PACKED__;
*/
struct bm_internal_dbg_nrec_common_st {
	u32 aggr_0_fifo_of_st:1;			/* byte[ 0- 3],bit[ 0- 0]	0x0 */
	u32 aggr_1_fifo_of_st:1;			/* byte[ 0- 3],bit[ 1- 1]	0x0 */
	u32 aggr_2_fifo_of_st:1;			/* byte[ 0- 3],bit[ 2- 2]	0x0 */
	u32 aggr_0_fifo_uf_st:1;			/* byte[ 0- 3],bit[ 3- 3]	0x0 */
	u32 aggr_1_fifo_uf_st:1;			/* byte[ 0- 3],bit[ 4- 4]	0x0 */
	u32 aggr_2_fifo_uf_st:1;			/* byte[ 0- 3],bit[ 5- 5]	0x0 */
	u32 rams_ctl_d_mng_rd_coll_st:1;	/* byte[ 0- 3],bit[ 6- 6]	0x0 */
	u32 rams_ctl_d_mng_wr_coll_st:1;	/* byte[ 0- 3],bit[ 7- 7]	0x0 */
	u32 dram_r_pend_fifo_of_st:1;		/* byte[ 0- 3],bit[ 8- 8]	0x0 */
	u32 dram_r_pend_fifo_uf_st:1;		/* byte[ 0- 3],bit[ 9- 9]	0x0 */
	u32 dram_axi_wd_fifo_of_st:1;		/* byte[ 0- 3],bit[10-10]	0x0 */
	u32 dram_axi_wd_fifo_uf_st:1;		/* byte[ 0- 3],bit[11-11]	0x0 */
	u32 dram_axi_wa_fifo_of_st:1;		/* byte[ 0- 3],bit[12-12]	0x0 */
	u32 dram_axi_wa_fifo_uf_st:1;		/* byte[ 0- 3],bit[13-13]	0x0 */
	u32 dram_axi_rd_fifo_of_st:1;		/* byte[ 0- 3],bit[14-14]	0x0 */
	u32 dram_axi_rd_fifo_uf_st:1;		/* byte[ 0- 3],bit[15-15]	0x0 */
	u32 dram_axi_ra_fifo_of_st:1;		/* byte[ 0- 3],bit[16-10]	0x0 */
	u32 dram_axi_ra_fifo_uf_st:1;		/* byte[ 0- 3],bit[17-17]	0x0 */
	u32 _reserved:14;					/* byte[ 0- 3],bit[18-31]	0x0 */
} __ATTRIBUTE_PACKED__;

struct bm_sw_debug_rec_intr_cause {
	u32 sw_debug_rec_intr_cause_sum:1;	/* byte[ 0- 3],bit[ 0- 0]	0x0 */
	u32 rams_ctl_sw_wr_c_s:1;			/* byte[ 0- 3],bit[ 1- 1]	0x0 */
	u32 rams_ctl_sw_wr_c_dyn_s:1;		/* byte[ 0- 3],bit[ 2- 2]	0x0 */
	u32 rams_ctl_sw_wr_d_dro_s:1;		/* byte[ 0- 3],bit[ 3- 3]	0x0 */
	u32 qm_bm_rf_err_s:1;				/* byte[ 0- 3],bit[ 4- 5]	0x0 */
	u32 _reserved:27;					/* byte[ 0- 3],bit[ 5-31] */
} __ATTRIBUTE_PACKED__;

struct bm_sw_debug_rec_intr_mask {
	u32 _reserved_1:1;					/* byte[ 0- 3],bit[ 0- 0] */
	u32 rams_ctl_sw_wr_c_m:1;			/* byte[ 0- 3],bit[ 1- 1]	0x0 */
	u32 rams_ctl_sw_wr_c_dyn_m:1;		/* byte[ 0- 3],bit[ 2- 2]	0x0 */
	u32 rams_ctl_sw_wr_d_dro_m:1;		/* byte[ 0- 3],bit[ 3- 3]	0x0 */
	u32 qm_bm_rf_err_m:1;				/* byte[ 0- 3],bit[ 4- 5]	0x0 */
	u32 _reserved_2:27;					/* byte[ 0- 3],bit[ 5-31] */
} __ATTRIBUTE_PACKED__;

struct bm_sys_nrec_common_intr_cause {
	u32 sys_nrec_common_intr_cause_sum:1;	/* byte[ 0- 3],bit[ 0- 0] */
	u32 qm_alc_pairs_viol_s:1;				/* byte[ 0- 3],bit[ 1- 1]	0x0 */
	u32 qm_rls_pairs_viol_s:1;				/* byte[ 0- 3],bit[ 2- 2]	0x0 */
	u32 ppe_alc_pairs_viol_s:1;				/* byte[ 0- 3],bit[ 3- 3]	0x0 */
	u32 ppe_rls_pairs_viol_s:1;				/* byte[ 0- 3],bit[ 4- 4]	0x0 */
	u32 ppe_alc_blen_viol_s:1;				/* byte[ 0- 3],bit[ 5- 5]	0x0 */
	u32 ppe_rls_blen_viol_s:1;				/* byte[ 0- 3],bit[ 6- 6]	0x0 */
	u32 mac_alc_pairs_viol_s:1;				/* byte[ 0- 3],bit[ 7- 7]	0x0 */
	u32 mac_rls_pairs_viol_s:1;				/* byte[ 0- 3],bit[ 8- 8]	0x0 */
	u32 mac_alc_pid_viol_s:1;				/* byte[ 0- 3],bit[ 9- 9]	0x0 */
	u32 mac_rls_pid_viol_s:1;				/* byte[ 0- 3],bit[10-10]	0x0 */
	u32 drm_dram_err_s:1;					/* byte[ 0- 3],bit[11-11]	0x0 */
	u32 dwm_dram_err_s:1;					/* byte[ 0- 3],bit[12-12]	0x0 */
	u32 dwm_fail_so_dram_fill_s:1;			/* byte[ 0- 3],bit[13-13]	0x0 */
	u32 _reserved:18;						/* byte[ 0- 3],bit[14-31] */
} __ATTRIBUTE_PACKED__;

struct bm_sys_nrec_common_intr_mask {
	u32 _reserved_0:1;						/* byte[ 0- 3],bit[ 0- 0] */
	u32 qm_alc_pairs_viol_m:1;				/* byte[ 0- 3],bit[ 1- 1]	0x0 */
	u32 qm_rls_pairs_viol_m:1;				/* byte[ 0- 3],bit[ 2- 2]	0x0 */
	u32 ppe_alc_pairs_viol_m:1;				/* byte[ 0- 3],bit[ 3- 3]	0x0 */
	u32 ppe_rls_pairs_viol_m:1;				/* byte[ 0- 3],bit[ 4- 4]	0x0 */
	u32 ppe_alc_blen_viol_m:1;				/* byte[ 0- 3],bit[ 5- 5]	0x0 */
	u32 ppe_rls_blen_viol_m:1;				/* byte[ 0- 3],bit[ 6- 6]	0x0 */
	u32 mac_alc_pairs_viol_m:1;				/* byte[ 0- 3],bit[ 7- 7]	0x0 */
	u32 mac_rls_pairs_viol_m:1;				/* byte[ 0- 3],bit[ 8- 8]	0x0 */
	u32 mac_alc_pid_viol_m:1;				/* byte[ 0- 3],bit[ 9- 9]	0x0 */
	u32 mac_rls_pid_viol_m:1;				/* byte[ 0- 3],bit[10-10]	0x0 */
	u32 drm_dram_err_m:1;					/* byte[ 0- 3],bit[11-11]	0x0 */
	u32 dwm_dram_err_m:1;					/* byte[ 0- 3],bit[12-12]	0x0 */
	u32 dwm_fail_so_dram_fill_m:1;			/* byte[ 0- 3],bit[13-13]	0x0 */
	u32 _reserved_1:18;						/* byte[ 0- 3],bit[14-31] */
} __ATTRIBUTE_PACKED__;

struct bm_sys_nrec_common_d0_st {
	u32 qm_last_alc_viol_pid_st:8;		/* byte[ 0- 3],bit[ 0- 7]	0x0 */
	u32 qm_last_rls_viol_pid_st:8;		/* byte[ 0- 3],bit[ 8-15]	0x0 */
	u32 ppe_last_alc_viol_pid_st:8;		/* byte[ 0- 3],bit[16-23]	0x0 */
	u32 ppe_last_rls_viol_pid_st:8;		/* byte[ 0- 3],bit[24-31]	0x0 */
} __ATTRIBUTE_PACKED__;

struct bm_sys_nrec_common_d1_st {
	u32 mac_last_alc_viol_pid_st:8;		/* byte[ 0- 3],bit[ 0- 7]	0x0 */
	u32 mac_last_rls_viol_pid_st:8;		/* byte[ 0- 3],bit[ 8-15]	0x0 */
	u32 drm_last_dram_err_pid_st:8;		/* byte[ 0- 3],bit[16-23]	0x0 */
	u32 dwm_last_fail_so_pid_st:8;		/* byte[ 0- 3],bit[24-31]	0x0 */
} __ATTRIBUTE_PACKED__;

struct bm_sys_nrec_common_d2_st {
	u32 qm_last_alc_viol_blen_st:8;		/* byte[ 0- 3],bit[ 0- 7]	0x0 */
	u32 qm_last_rls_viol_blen_st:8;		/* byte[ 0- 3],bit[ 8-15]	0x0 */
	u32 ppe_last_alc_viol_blen_st:8;	/* byte[ 0- 3],bit[16-23]	0x0 */
	u32 ppe_last_rls_viol_blen_st:8;	/* byte[ 0- 3],bit[24-31]	0x0 */
} __ATTRIBUTE_PACKED__;

struct bm_sys_nrec_common_d3_st {
	u32 mac_last_alc_viol_blen_st:8;	/* byte[ 0- 3],bit[ 0- 7]	0x0 */
	u32 mac_last_rls_viol_blen_st:8;	/* byte[ 0- 3],bit[ 8-15]	0x0 */
	u32 _reserved:15;					/* byte[ 0- 3],bit[16-31] */
} __ATTRIBUTE_PACKED__;

struct bm_common_general_conf {
	u32 drm_si_decide_extra_fill:8;		/* byte[ 0- 3],bit[ 0- 7]	0x0 */
	u32 dm_vmid:8;						/* byte[ 0- 3],bit[ 8-15]	0x3 */
	u32 bm_req_rcv_en:1;				/* byte[ 0- 3],bit[16-16]	0x0 */
	u32 _reserved:15;					/* byte[ 0- 3],bit[17-31] */
} __ATTRIBUTE_PACKED__;

struct bm_dram_domain_conf {
	u32 dwm_awdomain_b0:2;				/* byte[ 0- 3],bit[ 0- 1]	0x1 */
	u32 dwm_awdomain_bgp:2;				/* byte[ 0- 3],bit[ 2- 3]	0x2 */
	u32 drm_ardomain_b0:2;				/* byte[ 0- 3],bit[ 4- 5]	0x1 */
	u32 drm_ardomain_bgp:2;				/* byte[ 0- 3],bit[ 6- 7]	0x2 */
	u32 _reserved:24;					/* byte[ 0- 3],bit[ 8-31] */
} __ATTRIBUTE_PACKED__;

struct bm_dram_cache_conf {
	u32 dwm_awcache_b0:4;				/* byte[ 0- 3],bit[ 0- 3]	0x1 */
	u32 dwm_awcache_bgp:4;				/* byte[ 0- 3],bit[ 4- 7]	0x2 */
	u32 drm_arcache_b0:4;				/* byte[ 0- 3],bit[ 8-11]	0x1 */
	u32 drm_arcache_bgp:4;				/* byte[ 0- 3],bit[12-15]	0x2 */
	u32 _reserved:16;					/* byte[ 0- 3],bit[16-31] */
} __ATTRIBUTE_PACKED__;

struct bm_dram_qos_conf {
	u32 dwm_awqos_b0:2;					/* byte[ 0- 3],bit[ 0- 1]	0x1 */
	u32 dwm_awqos_bgp:2;				/* byte[ 0- 3],bit[ 2- 3]	0x2 */
	u32 drm_arqos_b0:2;					/* byte[ 0- 3],bit[ 4- 5]	0x1 */
	u32 drm_arqos_bgp:2;				/* byte[ 0- 3],bit[ 6- 7]	0x2 */
	u32 _reserved:24;					/* byte[ 0- 3],bit[ 8-31] */
} __ATTRIBUTE_PACKED__;

struct bm_error_intr_cause {
	u32 qm_bm_err_intr_sum:1;		/* byte[ 0- 3],bit[ 0- 0]	0x0 */
	u32 ecc_err_intr_s:1;			/* byte[ 0- 3],bit[ 1- 1]	0x0 */
	u32 b0_sys_events_rec_bank_s:1;	/* byte[ 0- 3],bit[ 2- 2]	0x0 */
	u32 b1_sys_events_rec_bank_s:1;	/* byte[ 0- 3],bit[ 3- 3]	0x0 */
	u32 b2_sys_events_rec_bank_s:1;	/* byte[ 0- 3],bit[ 4- 4]	0x0 */
	u32 b3_sys_events_rec_bank_s:1;	/* byte[ 0- 3],bit[ 5- 5]	0x0 */
	u32 b4_sys_events_rec_bank_s:1;	/* byte[ 0- 3],bit[ 6- 6]	0x0 */
	u32 sw_debug_events_rec_s:1;	/* byte[ 0- 3],bit[ 7- 7]	0x0 */
	u32 sys_events_nrec_common_s:1;	/* byte[ 0- 3],bit[ 8- 8]	0x0 */
	u32 _reserved:23;				/* byte[ 0- 3],bit[ 9-31] */
} __ATTRIBUTE_PACKED__;

struct bm_error_intr_mask {
	u32 ecc_err_intr_m:1;			/* byte[ 0- 3],bit[ 1- 1]	0x0 */
	u32 b0_sys_events_rec_bank_m:1;	/* byte[ 0- 3],bit[ 2- 2]	0x0 */
	u32 b1_sys_events_rec_bank_m:1;	/* byte[ 0- 3],bit[ 3- 3]	0x0 */
	u32 b2_sys_events_rec_bank_m:1;	/* byte[ 0- 3],bit[ 4- 4]	0x0 */
	u32 b3_sys_events_rec_bank_m:1;	/* byte[ 0- 3],bit[ 5- 5]	0x0 */
	u32 b4_sys_events_rec_bank_m:1;	/* byte[ 0- 3],bit[ 6- 6]	0x0 */
	u32 sw_debug_events_rec_m:1;	/* byte[ 0- 3],bit[ 7- 7]	0x0 */
	u32 sys_events_nrec_common_m:1;	/* byte[ 0- 3],bit[ 8- 8]	0x0 */
} __ATTRIBUTE_PACKED__;

struct bm_func_intr_cause {
	u32 qm_bm_func_intr_sum:1;		/* byte[ 0- 3],bit[ 0- 0]	0x0 */
	u32 b0_pool_nempty_intr_s:1;	/* byte[ 0- 3],bit[ 1- 1]	0x0 */
	u32 b0_dpool_ae_intr_s:1;		/* byte[ 0- 3],bit[ 2- 2]	0x0 */
	u32 b0_dpool_af_intr_s:1;		/* byte[ 0- 3],bit[ 3- 3]	0x0 */
	u32 b1_pool_nempty_intr_s:1;	/* byte[ 0- 3],bit[ 4- 4]	0x0 */
	u32 b1_dpool_ae_intr_s:1;		/* byte[ 0- 3],bit[ 5- 5]	0x0 */
	u32 b1_dpool_af_intr_s:1;		/* byte[ 0- 3],bit[ 6- 6]	0x0 */
	u32 b2_pool_nempty_intr_s:1;	/* byte[ 0- 3],bit[ 7- 7]	0x0 */
	u32 b2_dpool_ae_intr_s:1;		/* byte[ 0- 3],bit[ 8- 8]	0x0 */
	u32 b2_dpool_af_intr_s:1;		/* byte[ 0- 3],bit[ 9- 9]	0x0 */
	u32 b3_pool_nempty_intr_s:1;	/* byte[ 0- 3],bit[10-10]	0x0 */
	u32 b3_dpool_ae_intr_s:1;		/* byte[ 0- 3],bit[11-11]	0x0 */
	u32 b3_dpool_af_intr_s:1;		/* byte[ 0- 3],bit[12-12]	0x0 */
	u32 b4_pool_nempty_intr_s:1;	/* byte[ 0- 3],bit[13-13]	0x0 */
	u32 b4_dpool_ae_intr_s:1;		/* byte[ 0- 3],bit[14-14]	0x0 */
	u32 b4_dpool_af_intr_s:1;		/* byte[ 0- 3],bit[15-15]	0x0 */
} __ATTRIBUTE_PACKED__;

struct bm_func_intr_mask {
	u32 b0_pool_nempty_intr_m:1;	/* byte[ 0- 3],bit[ 1- 1]	0x0 */
	u32 b0_dpool_ae_intr_m:1;		/* byte[ 0- 3],bit[ 2- 2]	0x0 */
	u32 b0_dpool_af_intr_m:1;		/* byte[ 0- 3],bit[ 3- 3]	0x0 */
	u32 b1_pool_nempty_intr_m:1;	/* byte[ 0- 3],bit[ 4- 4]	0x0 */
	u32 b1_dpool_ae_intr_m:1;		/* byte[ 0- 3],bit[ 5- 5]	0x0 */
	u32 b1_dpool_af_intr_m:1;		/* byte[ 0- 3],bit[ 6- 6]	0x0 */
	u32 b2_pool_nempty_intr_m:1;	/* byte[ 0- 3],bit[ 7- 7]	0x0 */
	u32 b2_dpool_ae_intr_m:1;		/* byte[ 0- 3],bit[ 8- 8]	0x0 */
	u32 b2_dpool_af_intr_m:1;		/* byte[ 0- 3],bit[ 9- 9]	0x0 */
	u32 b3_pool_nempty_intr_m:1;	/* byte[ 0- 3],bit[10-10]	0x0 */
	u32 b3_dpool_ae_intr_m:1;		/* byte[ 0- 3],bit[11-11]	0x0 */
	u32 b3_dpool_af_intr_m:1;		/* byte[ 0- 3],bit[12-12]	0x0 */
	u32 b4_pool_nempty_intr_m:1;	/* byte[ 0- 3],bit[13-13]	0x0 */
	u32 b4_dpool_ae_intr_m:1;		/* byte[ 0- 3],bit[14-14]	0x0 */
	u32 b4_dpool_af_intr_m:1;		/* byte[ 0- 3],bit[15-15]	0x0 */
} __ATTRIBUTE_PACKED__;

struct bm_ecc_err_intr_cause {
	u32 ecc_err_intr_sum:1;					/* byte[ 0- 3],bit[ 0- 0]	0x0 */
	u32 dpr_c_mng_b0_stat_ser_err_1_s:1;	/* byte[ 0- 3],bit[ 1- 1]	0x0 */
	u32 dpr_c_mng_b0_stat_ser_err_2_s:1;	/* byte[ 0- 3],bit[ 2- 2]	0x0 */
	u32 dpr_c_mng_b1_stat_ser_err_1_s:1;	/* byte[ 0- 3],bit[ 3- 3]	0x0 */
	u32 dpr_c_mng_b1_stat_ser_err_2_s:1;	/* byte[ 0- 3],bit[ 4- 4]	0x0 */
	u32 dpr_c_mng_b2_stat_ser_err_1_s:1;	/* byte[ 0- 3],bit[ 5- 5]	0x0 */
	u32 dpr_c_mng_b2_stat_ser_err_2_s:1;	/* byte[ 0- 3],bit[ 6- 6]	0x0 */
	u32 dpr_c_mng_b3_stat_ser_err_1_s:1;	/* byte[ 0- 3],bit[ 7- 7]	0x0 */
	u32 dpr_c_mng_b3_stat_ser_err_2_s:1;	/* byte[ 0- 3],bit[ 8- 8]	0x0 */
	u32 dpr_c_mng_b4_stat_ser_err_1_s:1;	/* byte[ 0- 3],bit[ 9- 9]	0x0 */
	u32 dpr_c_mng_b4_stat_ser_err_2_s:1;	/* byte[ 0- 3],bit[10-10]	0x0 */
	u32 dpr_d_mng_ball_stat_ser_err_1_s:1;	/* byte[ 0- 3],bit[11-11]	0x0 */
	u32 dpr_d_mng_ball_stat_ser_err_2_s:1;	/* byte[ 0- 3],bit[12-12]	0x0 */
	u32 sram_b0_cache_ser_err_s:1;			/* byte[ 0- 3],bit[13-13]	0x0 */
	u32 sram_b1_cache_ser_err_s:1;			/* byte[ 0- 3],bit[14-14]	0x0 */
	u32 sram_b2_cache_ser_err_s:1;			/* byte[ 0- 3],bit[15-15]	0x0 */
	u32 sram_b3_cache_ser_err_s:1;			/* byte[ 0- 3],bit[16-10]	0x0 */
	u32 sram_b4_cache_ser_err_s:1;			/* byte[ 0- 3],bit[17-17]	0x0 */
	u32 tpr_c_mng_b0_dyn_ser_err_s:1;		/* byte[ 0- 3],bit[18-18]	0x0 */
	u32 tpr_c_mng_b1_dyn_ser_err_s:1;		/* byte[ 0- 3],bit[19-19]	0x0 */
	u32 tpr_c_mng_b2_dyn_ser_err_s:1;		/* byte[ 0- 3],bit[20-20]	0x0 */
	u32 tpr_c_mng_b3_dyn_ser_err_s:1;		/* byte[ 0- 3],bit[21-21]	0x0 */
	u32 tpr_c_mng_b4_dyn_ser_err_s:1;		/* byte[ 0- 3],bit[22-22]	0x0 */
	u32 tpr_dro_mng_ball_dyn_ser_err_s:1;	/* byte[ 0- 3],bit[23-23]	0x0 */
	u32 tpr_drw_mng_ball_dyn_ser_err_s:1;	/* byte[ 0- 3],bit[24-24]	0x0 */
} __ATTRIBUTE_PACKED__;

struct bm_ecc_err_intr_mask {
	u32 dpr_c_mng_b0_stat_ser_err_1_m:1;	/* byte[ 0- 3],bit[ 1- 1]	0x0 */
	u32 dpr_c_mng_b0_stat_ser_err_2_m:1;	/* byte[ 0- 3],bit[ 2- 2]	0x0 */
	u32 dpr_c_mng_b1_stat_ser_err_1_m:1;	/* byte[ 0- 3],bit[ 3- 3]	0x0 */
	u32 dpr_c_mng_b1_stat_ser_err_2_m:1;	/* byte[ 0- 3],bit[ 4- 4]	0x0 */
	u32 dpr_c_mng_b2_stat_ser_err_1_m:1;	/* byte[ 0- 3],bit[ 5- 5]	0x0 */
	u32 dpr_c_mng_b2_stat_ser_err_2_m:1;	/* byte[ 0- 3],bit[ 6- 6]	0x0 */
	u32 dpr_c_mng_b3_stat_ser_err_1_m:1;	/* byte[ 0- 3],bit[ 7- 7]	0x0 */
	u32 dpr_c_mng_b3_stat_ser_err_2_m:1;	/* byte[ 0- 3],bit[ 8- 8]	0x0 */
	u32 dpr_c_mng_b4_stat_ser_err_1_m:1;	/* byte[ 0- 3],bit[ 9- 9]	0x0 */
	u32 dpr_c_mng_b4_stat_ser_err_2_m:1;	/* byte[ 0- 3],bit[10-10]	0x0 */
	u32 dpr_d_mng_ball_stat_ser_err_1_m:1;	/* byte[ 0- 3],bit[11-11]	0x0 */
	u32 dpr_d_mng_ball_stat_ser_err_2_m:1;	/* byte[ 0- 3],bit[12-12]	0x0 */
	u32 sram_b0_cache_ser_err_m:1;			/* byte[ 0- 3],bit[13-13]	0x0 */
	u32 sram_b1_cache_ser_err_m:1;			/* byte[ 0- 3],bit[14-14]	0x0 */
	u32 sram_b2_cache_ser_err_m:1;			/* byte[ 0- 3],bit[15-15]	0x0 */
	u32 sram_b3_cache_ser_err_m:1;			/* byte[ 0- 3],bit[16-10]	0x0 */
	u32 sram_b4_cache_ser_err_m:1;			/* byte[ 0- 3],bit[17-17]	0x0 */
	u32 tpr_c_mng_b0_dyn_ser_err_m:1;		/* byte[ 0- 3],bit[18-18]	0x0 */
	u32 tpr_c_mng_b1_dyn_ser_err_m:1;		/* byte[ 0- 3],bit[19-19]	0x0 */
	u32 tpr_c_mng_b2_dyn_ser_err_m:1;		/* byte[ 0- 3],bit[20-20]	0x0 */
	u32 tpr_c_mng_b3_dyn_ser_err_m:1;		/* byte[ 0- 3],bit[21-21]	0x0 */
	u32 tpr_c_mng_b4_dyn_ser_err_m:1;		/* byte[ 0- 3],bit[22-22]	0x0 */
	u32 tpr_dro_mng_ball_dyn_ser_err_m:1;	/* byte[ 0- 3],bit[23-23]	0x0 */
	u32 tpr_drw_mng_ball_dyn_ser_err_m:1;	/* byte[ 0- 3],bit[24-24]	0x0 */
} __ATTRIBUTE_PACKED__;

struct bm_dm_axi_fifos_st {
	u32 dwm_waddr_fifo_fill_st:8;		/* byte[ 0- 3],bit[ 0- 7]	0x0 */
	u32 dwm_wdata_fifo_fill_st:8;		/* byte[ 0- 3],bit[ 8-15]	0x0 */
	u32 drm_raddr_fifo_fill_st:8;		/* byte[ 0- 3],bit[16-23]	0x0 */
	u32 drm_rdata_fifo_fill_st:8;		/* byte[ 0- 3],bit[24-31]	0x0 */
} __ATTRIBUTE_PACKED__;

struct bm_drm_pend_fifo_st {
	u32 drm_pend_fifo_fill_st:8;			/* byte[ 0- 3],bit[ 0- 7]	0x0 */
	u32 _reserved:24;						/* byte[ 0- 3],bit[ 8-31]	    */
} __ATTRIBUTE_PACKED__;

struct bm_dm_axi_wr_pend_fifo_st {
	u32 axi_wr_pend_fifo_fill_st:8;			/* byte[ 0- 3],bit[ 0- 7]	0x0 */
	u32 _reserved:24;						/* byte[ 0- 3],bit[ 8-31]	    */
} __ATTRIBUTE_PACKED__;

struct bm_bm_idle_st {
	u32 bm_idle_st:1;						/* byte[ 0- 3],bit[ 0- 0]	0x0 */
	u32 _reserved_1:31;						/* byte[ 0- 3],bit[ 1-31] */
} __ATTRIBUTE_PACKED__;

/*extern struct bm_ctl BM_default, BM_profile_1;  */

/*
#define bm_ctl qm_bm_profile
struct bm_profile {
	u32                                    address_base;
	struct bm_dpr_c_mng_stat               tab_dpr_c_mng_stat;
	struct bm_tpr_c_mng_dyn                tab_tpr_c_mng_dyn;
	struct bm_dpr_d_mng_ball_stat          tab_dpr_d_mng_ball_stat;
	struct bm_tpr_dro_mng_ball_dyn         tab_tpr_dro_mng_ball_dyn;
	struct bm_tpr_drw_mng_ball_dyn         tab_tpr_drw_mng_ball_dyn;
	struct bm_tpr_ctrs_0                   tab_tpr_ctrs_0;
	struct bm_sram_cache                   tab_sram_cache;
	struct bm_bank_profile                 vec_bank[5];
	struct bm_b0_past_alc_fifos_st         reg_b0_past_alc_fifos_st;
	struct bm_bgp_past_alc_fifos_st        reg_bgp_past_alc_fifos_st;
	struct bm_b0_rls_wrp_ppe_fifos_st      reg_b0_rls_wrp_ppe_fifos_st;
	struct bm_internal_dbg_nrec_common_st  reg_internal_dbg_nrec_common_st;
	struct bm_sw_debug_rec_intr_cause      reg_sw_debug_rec_intr_cause;
	struct bm_sw_debug_rec_intr_mask       reg_sw_debug_rec_intr_mask;
	struct bm_sys_nrec_common_intr_cause   reg_sys_nrec_common_intr_cause;
	struct bm_sys_nrec_common_intr_mask    reg_sys_nrec_common_intr_mask;
	struct bm_sys_nrec_common_d0_st        reg_sys_nrec_common_d0_st;
	struct bm_sys_nrec_common_d1_st        reg_sys_nrec_common_d1_st;
	struct bm_common_general_conf          reg_common_general_conf;
	struct bm_dram_domain_conf             reg_dram_domain_conf;
	struct bm_dram_cache_conf              reg_dram_cache_conf;
	struct bm_dram_qos_conf                reg_dram_qos_conf;
	struct bm_error_intr_cause             reg_error_intr_cause;
	struct bm_error_intr_mask              reg_error_intr_mask;
	struct bm_func_intr_cause              reg_func_intr_cause;
	struct bm_func_intr_mask               reg_func_intr_mask;
	struct bm_ecc_err_intr_cause           reg_ecc_err_intr_cause;
	struct bm_ecc_err_intr_mask            reg_ecc_err_intr_mask;
	struct bm_dm_axi_fifos_st              reg_dm_axi_fifos_st;
	struct bm_drm_pend_fifo_st             reg_drm_pend_fifo_st;
	struct bm_dm_axi_wr_pend_fifo_st       reg_dm_axi_wr_pend_fifo_st;
	struct bm_bm_idle_st                   reg_bm_idle_st;
	/ * environment * /
	bm_handle hEnv;
	u32    magic;
} __ATTRIBUTE_PACKED__;
*/

/* SW structures  */
struct dram_ctrl_profile {
	u32 dwm_awdomain_b0;
	u32 dwm_awdomain_bgp;
	u32 drm_ardomain_b0;
	u32 drm_ardomain_bgp;
	u32 dwm_awcache_b0;
	u32 dwm_awcache_bgp;
	u32 drm_arcache_b0;
	u32 drm_arcache_bgp;
	u32 dwm_awqos_b0;
	u32 dwm_awqos_bgp;
	u32 drm_arqos_b0;
	u32 drm_arqos_bgp;
} __ATTRIBUTE_PACKED__;

/*
struct pool_buffer_bank_profile {
	struct bm_pool_conf pool_conf[31];
	struct bm_pool_st   pool_st[31];
} __ATTRIBUTE_PACKED__;

struct pool_buffer_profile {
	struct pool_buffer_bank_profile bank[5];
} __ATTRIBUTE_PACKED__;

struct sram_cache_bank_profile {
	struct bm_sram_cache sram_cache[31];
} __ATTRIBUTE_PACKED__;

struct sram_cache_profile {
	struct sram_cache_bank_profile bank[5];
} __ATTRIBUTE_PACKED__;

struct mask_profile {
	u32 XXXXX;
} __ATTRIBUTE_PACKED__;
*/

/* registers addresses definitons */

struct bm_alias {
	u32 base;
	u32 b_pool_n_conf[BM_NUMBER_OF_BANKS];
	u32 b_pool_n_st[BM_NUMBER_OF_BANKS];
	u32 b_sys_rec_bank_intr_cause[BM_NUMBER_OF_BANKS];
	u32 b_sys_rec_bank_intr_mask[BM_NUMBER_OF_BANKS];
	u32 b_sys_rec_bank_d0_st[BM_NUMBER_OF_BANKS];
	u32 b_sys_rec_bank_d1_st[BM_NUMBER_OF_BANKS];
	u32 sw_debug_rec_intr_cause;
	u32 sw_debug_rec_intr_mask;
	u32 sys_nrec_common_intr_cause;
	u32 sys_nrec_common_intr_mask;
	u32 sys_nrec_common_d0_st;
	u32 sys_nrec_common_d1_st;
	u32 sys_nrec_common_d2_st;
	u32 sys_nrec_common_d3_st;
	u32 common_general_conf;
	u32 dram_domain_conf;
	u32 dram_cache_conf;
	u32 dram_qos_conf;
	u32 error_intr_cause;
	u32 error_intr_mask;
	u32 func_intr_cause;
	u32 func_intr_mask;
	u32 ecc_err_intr_cause;
	u32 ecc_err_intr_mask;
	u32 pool_nempty_intr_cause[BM_NUMBER_OF_BANKS];
	u32 pool_nempty_intr_mask[BM_NUMBER_OF_BANKS];
	u32 dpool_ae_intr_cause[BM_NUMBER_OF_BANKS];
	u32 dpool_ae_intr_mask[BM_NUMBER_OF_BANKS];
	u32 dpool_af_intr_cause[BM_NUMBER_OF_BANKS];
	u32 dpool_af_intr_mask[BM_NUMBER_OF_BANKS];
	u32 b_bank_req_fifos_st;
	u32 b0_past_alc_fifos_st;
	u32 bgp_past_alc_fifos_st;
	u32 b0_rls_wrp_ppe_fifos_st;
	u32 dm_axi_fifos_st;
	u32 drm_pend_fifo_st;
	u32 dm_axi_wr_pend_fifo_st;
	u32 bm_idle_st;
	u32 dpr_c_mng_stat[BM_NUMBER_OF_BANKS];
	u32 tpr_c_mng_b_dyn[BM_NUMBER_OF_BANKS];
	u32 dpr_d_mng_ball_stat;
	u32 tpr_dro_mng_ball_dyn;
	u32 tpr_drw_mng_ball_dyn;
	u32 tpr_ctrs_0_b[BM_NUMBER_OF_BANKS];
	u32 sram_b_cache[BM_NUMBER_OF_BANKS];
} __ATTRIBUTE_PACKED__;


extern struct bm_alias bm;
extern struct bm_alias bm_reg_size;
extern struct bm_alias bm_reg_offset;

int bm_reg_address_alias_init(void);
int bm_reg_size_alias_init(void);
int bm_reg_offset_alias_init(void);
int bm_pid_bid_init(void);
int bm_register_name_get(u32 reg_base_address, u32 reg_offset, char *reg_name);

#endif	/*__MV_BM_REGS_H__*/
