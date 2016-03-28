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
#ifndef	__mv_qm_regs_h__
#define	__mv_qm_regs_h__

#include "qm/mv_qm.h"

#define      QL_UNIT_OFFSET 0x00400000
#define     PFE_UNIT_OFFSET 0x00410000
#define     DQF_UNIT_OFFSET 0x00420000
#define     DMA_UNIT_OFFSET 0x00430000
#define REORDER_UNIT_OFFSET 0x00500000
#define     GPM_UNIT_OFFSET 0x00600000


/* --------------------------------------- */
/*     registers global defenitions        */
/* --------------------------------------- */

/* DQF row size in bytes*/
#define QM_DQF_PPC_ROW_SIZE		144
#define QM_DQF_MAC_ROW_SIZE		16

/* DMA degenitions */
#define QM_TAIL_PTR_CMAC_BIT		0x6

/* DMA */
struct dma_q_memory_allocation {
	int q_memory:32;					/* byte[ 0- 3] ,bit[ 0-31] */
} __ATTRIBUTE_PACKED__;

struct dma_gpm_thresholds {
	int gpm_pl_pool_low_bp:6;			/* byte[ 0- 3] ,bit[ 0- 5] */
	int _reserved_1:2;					/* byte[ 0- 3] ,bit[ 6- 7] */
	int gpm_pl_pool_high_bp:6;			/* byte[ 0- 3] ,bit[ 8-13] */
	int _reserved_2:2;					/* byte[ 0- 3] ,bit[14-15] */
	int gpm_qe_pool_low_bp:6;			/* byte[ 0- 3] ,bit[16-21] */
	int _reserved_3:2;					/* byte[ 0- 3] ,bit[22-23] */
	int gpm_qe_pool_high_bp:6;			/* byte[ 0- 3] ,bit[24-29] */
	int _reserved_4:2;					/* byte[ 0- 3] ,bit[30-31] */
} __ATTRIBUTE_PACKED__;

struct dma_dram_thresholds {
	int dram_pl_pool_low_bp:6;			/* byte[ 0- 3] ,bit[ 0- 5] */
	int _reserved_1:2;					/* byte[ 0- 3] ,bit[ 6- 7] */
	int dram_pl_pool_high_bp:6;			/* byte[ 0- 3] ,bit[ 8-13] */
	int _reserved_2:2;					/* byte[ 0- 3] ,bit[14-15] */
	int dram_qe_pool_low_bp:6;			/* byte[ 0- 3] ,bit[16-21] */
	int _reserved_3:2;					/* byte[ 0- 3] ,bit[22-23] */
	int dram_qe_pool_high_bp:6;			/* byte[ 0- 3] ,bit[24-29] */
	int _reserved_4:2;					/* byte[ 0- 3] ,bit[30-31] */
} __ATTRIBUTE_PACKED__;

struct dma_AXI_write_attributes_for_swf_mode {
	int swf_awdomain:2;					/* byte[ 0- 3] ,bit[ 0- 1] */
	int swf_awcache:4;					/* byte[ 0- 3] ,bit[ 2- 5] */
	int swf_awqos:2;					/* byte[ 0- 3] ,bit[ 6- 7] */
	int _reserved:24;					/* byte[ 0- 3] ,bit[ 8-31] */
} __ATTRIBUTE_PACKED__;

struct dma_AXI_write_attributes_for_rdma_mode {
	int rdma_awdomain:2;				/* byte[ 0- 3] ,bit[ 0- 1] */
	int rdma_awcache:4;					/* byte[ 0- 3] ,bit[ 2- 5] */
	int rdma_awqos:2;					/* byte[ 0- 3] ,bit[ 6- 7] */
	int _reserved:24;					/* byte[ 0- 3] ,bit[ 8-31] */
} __ATTRIBUTE_PACKED__;

struct dma_AXI_write_attributes_for_hwf_qece {
	int qece_awdomain:2;				/* byte[ 0- 3] ,bit[ 0- 1] */
	int qece_awcache:4;					/* byte[ 0- 3] ,bit[ 2- 5] */
	int qece_awqos:2;					/* byte[ 0- 3] ,bit[ 6- 7] */
	int _reserved:24;					/* byte[ 0- 3] ,bit[ 8-31] */
} __ATTRIBUTE_PACKED__;

struct dma_AXI_write_attributes_for_hwf_pyld {
	int pyld_awdomain:2;				/* byte[ 0- 3] ,bit[ 0- 1] */
	int pyld_awcache:4;					/* byte[ 0- 3] ,bit[ 2- 5] */
	int pyld_awqos:2;					/* byte[ 0- 3] ,bit[ 6- 7] */
	int _reserved:24;					/* byte[ 0- 3] ,bit[ 8-31] */
} __ATTRIBUTE_PACKED__;

struct dma_DRAM_VMID {
	int dram_vmid:8;					/* byte[ 0- 3] ,bit[ 0- 7] */
	int _reserved:24;					/* byte[ 0- 3] ,bit[ 8-31] */
} __ATTRIBUTE_PACKED__;

struct dma_idle_status {
	int gpm_pl_cache_is_empty:1;			/* byte[ 0- 3] ,bit[ 0- 0] */
	int gpm_pl_cache_is_full:1;				/* byte[ 0- 3] ,bit[ 1- 1] */
	int gpm_qe_cache_is_empty:1;			/* byte[ 0- 3] ,bit[ 2- 2] */
	int gpm_qe_cache_is_full:1;				/* byte[ 0- 3] ,bit[ 3- 3] */
	int dram_pl_cache_is_empty:1;			/* byte[ 0- 3] ,bit[ 4- 4] */
	int dram_pl_cache_is_full:1;			/* byte[ 0- 3] ,bit[ 5- 5] */
	int dram_qe_cache_is_empty:1;			/* byte[ 0- 3] ,bit[ 6- 6] */
	int dram_qe_cache_is_full:1;			/* byte[ 0- 3] ,bit[ 7- 7] */
	int dram_fifo_is_empty:1;				/* byte[ 0- 3] ,bit[ 8- 8] */
	int mac_axi_enq_channel_is_empty:1;		/* byte[ 0- 3] ,bit[ 9- 9] */
	int NSS_axi_enq_channel_is_empty:1;		/* byte[ 0- 3] ,bit[10-10] */
	int gpm_ppe_read_fifo_is_empty:1;		/* byte[ 0- 3] ,bit[11-11] */
	int ppe_gpm_pl_write_fifo_is_empty:1;	/* byte[ 0- 3] ,bit[12-12] */
	int ppe_gpm_qe_write_fifo_is_empty:1;	/* byte[ 0- 3] ,bit[13-13] */
	int ppe_ru_read_fifo_is_empty:1;		/* byte[ 0- 3] ,bit[14-14] */
	int ppe_ru_write_fifo_is_empty:1;		/* byte[ 0- 3] ,bit[15-15] */
	int ru_ppe_read_fifo_is_empty:1;		/* byte[ 0- 3] ,bit[16-16] */
	int dram_fifo_fsm_state_is_idle:1;		/* byte[ 0- 3] ,bit[17-17] */
	int qeram_init_fsm_state_is_idle:1;		/* byte[ 0- 3] ,bit[18-18] */
	int _reserved:13;						/* byte[ 0- 3] ,bit[19-31] */
} __ATTRIBUTE_PACKED__;

struct dma_ecc_error_cause {
	int qm_dma_ecc_interrupt:1;				/* byte[ 0- 3] ,bit[ 0- 0] */
	int ceram_mac_ecc_error:1;				/* byte[ 0- 3] ,bit[ 1- 1] */
	int ceram_ppe_ecc_error:1;				/* byte[ 0- 3] ,bit[ 2- 2] */
	int gpm_pl_ecc_error:1;					/* byte[ 0- 3] ,bit[ 3- 3] */
	int gpm_qe_ecc_error:1;					/* byte[ 0- 3] ,bit[ 4- 4] */
	int qeram_ecc_error:1;					/* byte[ 0- 3] ,bit[ 5- 5] */
	int dram_fifo_ecc_error:1;				/* byte[ 0- 3] ,bit[ 6- 6] */
	int _reserved:25;						/* byte[ 0- 3] ,bit[ 7-31] */
} __ATTRIBUTE_PACKED__;

struct dma_ecc_error_mask {
	int _reserved_1:1;						/* byte[ 0- 3] ,bit[ 0- 0] */
	int ceram_mac_ecc_error_mask:1;			/* byte[ 0- 3] ,bit[ 1- 1] */
	int ceram_ppe_ecc_error_mask:1;			/* byte[ 0- 3] ,bit[ 2- 2] */
	int gpm_pl_ecc_error_mask:1;			/* byte[ 0- 3] ,bit[ 3- 3] */
	int gpm_qe_ecc_error_mask:1;			/* byte[ 0- 3] ,bit[ 4- 4] */
	int qeram_ecc_error_mask:1;				/* byte[ 0- 3] ,bit[ 5- 5] */
	int dram_fifo_ecc_error_mask:1;			/* byte[ 0- 3] ,bit[ 6- 6] */
	int _reserved_2:25;						/* byte[ 0- 3] ,bit[ 7-31] */
} __ATTRIBUTE_PACKED__;

struct dma_internal_error_cause {
	int qm_dma_internal_error_interrupt:1;			/* byte[ 0- 3] ,bit[ 0- 0] */
	int reg_file_error:1;							/* byte[ 0- 3] ,bit[ 1- 1] */
	int dram_response_error:1;						/* byte[ 0- 3] ,bit[ 2- 2] */
	int mac_enq_with_wrong_source_id_error:1;		/* byte[ 0- 3] ,bit[ 3- 3] */
	int ppe_enq_with_wrong_source_id_error:1;		/* byte[ 0- 3] ,bit[ 4- 4] */
	int _reserved:27;								/* byte[ 0- 3] ,bit[ 5-31] */
} __ATTRIBUTE_PACKED__;

/* HW structures*/
/* QL */
struct ql_low_threshold {
	int low_threshold:16;					/* byte[ 0- 3] ,bit[ 0-15] */
	int _reserved:16;						/* byte[ 0- 3] ,bit[16-31] */
} __ATTRIBUTE_PACKED__;

struct ql_pause_threshold {
	int pause_threshold:16;					/* byte[ 0- 3] ,bit[ 0-15] */
	int _reserved:16;						/* byte[ 0- 3] ,bit[16-31] */
} __ATTRIBUTE_PACKED__;

struct ql_high_threshold {
	int high_threshold:16;					/* byte[ 0- 3] ,bit[ 0-15] */
	int _reserved:16;						/* byte[ 0- 3] ,bit[16-31] */
} __ATTRIBUTE_PACKED__;

struct ql_traffic_source {
	int traffic_source:3;					/* byte[ 0- 3] ,bit[ 0- 2] */
	int _reserved:29;						/* byte[ 0- 3] ,bit[ 3-31] */
} __ATTRIBUTE_PACKED__;

struct ql_ecc_error_cause {
	int qm_ql_ecc_interrupt:1;				/* byte[ 0- 3] ,bit[ 0- 0] */
	int qptr_ecc_error:1;					/* byte[ 0- 3] ,bit[ 1- 1] */
	int qlen_ecc_error:1;					/* byte[ 0- 3] ,bit[ 2- 2] */
	int _reserved:29;						/* byte[ 0- 3] ,bit[ 3-31] */
} __ATTRIBUTE_PACKED__;

struct ql_ecc_error_mask {
	int _reserved_0:1;						/* byte[ 0- 3] ,bit[ 0- 0] */
	int qptr_ecc_error_mask:1;				/* byte[ 0- 3] ,bit[ 1- 1] */
	int qlen_ecc_error_mask:1;				/* byte[ 0- 3] ,bit[ 2- 2] */
	int _reserved_1:29;						/* byte[ 0- 3] ,bit[ 3-31] */
} __ATTRIBUTE_PACKED__;

struct ql_internal_error_cause {
	int qm_ql_internal_error_interrupt:1;	/* byte[ 0- 3] ,bit[ 0- 0] */
	int reg_file_error:1;					/* byte[ 0- 3] ,bit[ 1- 1] */
	int _reserved:30;						/* byte[ 0- 3] ,bit[ 2-31] */
} __ATTRIBUTE_PACKED__;

struct ql_internal_error_mask {
	int _reserved_0:1;						/* byte[ 0- 3] ,bit[ 0- 0] */
	int reg_file_error_mask:1;				/* byte[ 0- 3] ,bit[ 1- 1] */
	int _reserved:30;						/* byte[ 0- 3] ,bit[ 2-31] */
} __ATTRIBUTE_PACKED__;

struct ql_nss_general_purpose {
	int nss_general_purpose:32;				/* byte[ 0- 3] ,bit[ 0-31] */
} __ATTRIBUTE_PACKED__;

struct ql_qptr_entry {
	int qptr0:3;							/* byte[ 0- 3] ,bit[ 0- 2] */
	int qptr1:3;							/* byte[ 0- 3] ,bit[ 3- 5] */
	int qptr2:3;							/* byte[ 0- 3] ,bit[ 6- 8] */
	int qptr3:3;							/* byte[ 0- 3] ,bit[ 9-11] */
	int qptr4:3;							/* byte[ 0- 3] ,bit[12-14] */
	int qptr5:3;							/* byte[ 0- 3] ,bit[15-17] */
	int qptr6:3;							/* byte[ 0- 3] ,bit[18-20] */
	int qptr7:3;							/* byte[ 0- 3] ,bit[21-23] */
	int _reserved:8;						/* byte[ 0- 3] ,bit[24-31] */
} __ATTRIBUTE_PACKED__;

struct ql_qptr {
	struct ql_qptr_entry reg_qptr_entry;
} __ATTRIBUTE_PACKED__;

struct ql_rule_bpi_entry {
	int  xon_thr:16;						/* byte[ 0- 1] ,bit[ 0- 15] */
	int xoff_thr:16;						/* byte[ 2- 3] ,bit[ 16- 31] */
	int target_lvl:2;						/* byte[ 4- 4] ,bit[ 32- 33] */
	int target_node:7;						/* byte[ 4- 5] ,bit[ 34- 40] */
	int _reserved:23;
} __ATTRIBUTE_PACKED__;

struct ql_rule_bpi {
	struct ql_rule_bpi_entry reg_rule_bpi_entry;
} __ATTRIBUTE_PACKED__;




struct ql_ql_entry {
	int ql:24;							/* byte[ 0- 3] ,bit[ 0-23] */
	int qstatus:2;							/* byte[ 0- 3] ,bit[24-25] */
	int _reserved:6;						/* byte[ 0- 3] ,bit[26-31] */
} __ATTRIBUTE_PACKED__;

struct ql_qlen {
	struct ql_ql_entry reg_ql_entry;
} __ATTRIBUTE_PACKED__;

struct ql_bp_qlen_entry {
	int ql:16;							/* byte[ 0- 1] ,bit[ 0-15] */
	int qstatus:2;							/* byte[ 2] ,bit[16-17] */
	int _reserved:14;						/* byte[ 2-3] ,bit[18-31] */
} __ATTRIBUTE_PACKED__;

struct ql_bp_qlen {
	struct ql_bp_qlen_entry reg_ql_bp_qlen_entry;
} __ATTRIBUTE_PACKED__;

struct ql_drop_qlen_entry {
	int ql:16;							/* byte[ 0- 1] ,bit[ 0-15] */
	int _reserved:16;						/* byte[ 2-3] ,bit[16-31] */
} __ATTRIBUTE_PACKED__;

struct ql_drop_qlen {
	struct ql_drop_qlen_entry reg_ql_drop_qlen_entry;
} __ATTRIBUTE_PACKED__;

/* PFE */
struct pfe_qece_dram_base_address_hi {
	int qece_dram_base_address_hi:8;		/* byte[ 0- 3] ,bit[ 0- 7] */
	int _reserved:24;						/* byte[ 0- 3] ,bit[ 8-31] */
} __ATTRIBUTE_PACKED__;

struct pfe_pyld_dram_base_address_hi {
	int pyld_dram_base_address_hi:8;		/* byte[ 0- 3] ,bit[ 0- 7] */
	int _reserved:24;						/* byte[ 0- 3] ,bit[ 8-31] */
} __ATTRIBUTE_PACKED__;

struct pfe_qece_dram_base_address_lo {
	int qece_dram_base_address_low:32;		/* byte[ 0- 3] ,bit[ 0-31] */
} __ATTRIBUTE_PACKED__;

struct pfe_pyld_dram_base_address_lo {
	int pyld_dram_base_address_low:32;		/* byte[ 0- 3] ,bit[ 0-31] */
} __ATTRIBUTE_PACKED__;

struct pfe_QM_VMID {
	int VMID:8;								/* byte[ 0- 3] ,bit[ 0- 7] */
	int _reserved:24;						/* byte[ 0- 3] ,bit[ 8-31] */
} __ATTRIBUTE_PACKED__;

/*
struct pfe_port_ppe {
	int port_ppe:16;						*//* byte[ 0- 3] ,bit[ 0-15] *//*
	int _reserved:16;						*//* byte[ 0- 3] ,bit[16-31] *//*
} __ATTRIBUTE_PACKED__;
*/
struct pfe_port_flush {
	int port_flush:16;						/* byte[ 0- 3] ,bit[ 0-15] */
	int _reserved:16;						/* byte[ 0- 3] ,bit[16-31] */
} __ATTRIBUTE_PACKED__;

struct pfe_AXI_read_attributes_for_swf_mode {
	int swf_ardomain:2;						/* byte[ 0- 3] ,bit[ 0- 1] */
	int swf_arcache:4;						/* byte[ 0- 3] ,bit[ 2- 5] */
	int swf_arqos:2;						/* byte[ 0- 3] ,bit[ 6- 7] */
	int _reserved:24;						/* byte[ 0- 3] ,bit[ 8-31] */
} __ATTRIBUTE_PACKED__;

struct pfe_AXI_read_attributes_for_rdma_mode {
	int rdma_ardomain:2;					/* byte[ 0- 3] ,bit[ 0- 1] */
	int rdma_arcache:4;						/* byte[ 0- 3] ,bit[ 2- 5] */
	int rdma_arqos:2;						/* byte[ 0- 3] ,bit[ 6- 7] */
	int _reserved:24;						/* byte[ 0- 3] ,bit[ 8-31] */
} __ATTRIBUTE_PACKED__;

struct pfe_AXI_read_attributes_for_hwf_qece {
	int qece_ardomain:2;					/* byte[ 0- 3] ,bit[ 0- 1] */
	int qece_arcache:4;						/* byte[ 0- 3] ,bit[ 2- 5] */
	int qece_arqos:2;						/* byte[ 0- 3] ,bit[ 6- 7] */
	int _reserved:24;						/* byte[ 0- 3] ,bit[ 8-31] */
} __ATTRIBUTE_PACKED__;

struct pfe_AXI_read_attributes_for_hwf_pyld {
	int pyld_ardomain:2;					/* byte[ 0- 3] ,bit[ 0- 1] */
	int pyld_arcache:4;						/* byte[ 0- 3] ,bit[ 2- 5] */
	int pyld_arqos:2;						/* byte[ 0- 3] ,bit[ 6- 7] */
	int _reserved:24;						/* byte[ 0- 3] ,bit[ 8-31] */
} __ATTRIBUTE_PACKED__;

struct pfe_ecc_error_cause {
	int qm_pfe_ecc_interrupt:1;				/* byte[ 0- 3] ,bit[ 0- 0] */
	int qflush_ecc_error:1;					/* byte[ 0- 3] ,bit[ 1- 1] */
	int qece_ecc_error:1;					/* byte[ 0- 3] ,bit[ 2- 2] */
	int macsdata_ecc_error:1;				/* byte[ 0- 3] ,bit[ 3- 3] */
	int _reserved:28;						/* byte[ 0- 3] ,bit[ 8-31] */
} __ATTRIBUTE_PACKED__;

struct pfe_ecc_error_mask {
	int _reserved_1:1;						/* byte[ 0- 3] ,bit[ 0- 0] */
	int qflush_ecc_error_mask:1;			/* byte[ 0- 3] ,bit[ 1- 1] */
	int qece_ecc_error_mask:1;				/* byte[ 0- 3] ,bit[ 2- 2] */
	int macsdata_ecc_error_mask:1;			/* byte[ 0- 3] ,bit[ 3- 3] */
	int _reserved_2:28;						/* byte[ 0- 3] ,bit[ 4-31] */
} __ATTRIBUTE_PACKED__;

struct pfe_internal_error_cause {
	int qm_pfe_internal_error_interrupt:1;			/* byte[ 0- 3] ,bit[ 0- 0] */
	int reg_file_error:1;							/* byte[ 0- 3] ,bit[ 1- 1] */
	int dram_response_error:1;						/* byte[ 0- 3] ,bit[ 2- 2] */
	int last_port_not_last_queue_error:1;			/* byte[ 0- 3] ,bit[ 3- 3] */
	int gpm_cl_error:1;								/* byte[ 0- 3] ,bit[ 4- 4] */
	int deq_mode_error:1;							/* byte[ 0- 3] ,bit[ 5- 5] */
	int no_descriptor_mode_for_dram_q_error:1;		/* byte[ 0- 3] ,bit[ 6- 6] */
	int pckt_len_grtr_cfh_len_plus_descr_error:1;	/* byte[ 0- 3] ,bit[ 7- 7] */
	int _reserved:24;								/* byte[ 0- 3] ,bit[ 8-31] */
} __ATTRIBUTE_PACKED__;

struct pfe_internal_error_mask {
	int _reserved_1:1;                                  /* byte[ 0- 3] ,bit[ 0- 0] */
	int reg_file_error_mask:1;							/* byte[ 0- 3] ,bit[ 1- 1] */
	int dram_response_error_mask:1;						/* byte[ 0- 3] ,bit[ 2- 2] */
	int last_port_not_last_queue_error_mask:1;			/* byte[ 0- 3] ,bit[ 3- 3] */
	int gpm_cl_error_mask:1;							/* byte[ 0- 3] ,bit[ 4- 4] */
	int deq_mode_error_mask:1;							/* byte[ 0- 3] ,bit[ 5- 5] */
	int no_descriptor_mode_for_dram_q_error_mask:1;		/* byte[ 0- 3] ,bit[ 6- 6] */
	int pckt_len_grtr_cfh_len_plus_descr_error_mask:1;	/* byte[ 0- 3] ,bit[ 7- 7] */
	int _reserved_2:24;                                 /* byte[ 0- 3] ,bit[ 8-31] */
} __ATTRIBUTE_PACKED__;

struct pfe_idle_status {
	int axi_outstanding_fifo_empty:1;		/* byte[ 0- 3] ,bit[ 0-31] */
	int dram_to_macs_fifo_empty:1;			/* byte[ 0- 3] ,bit[ 2- 2] */
	int bm_release_fifo_empty:1;			/* byte[ 0- 3] ,bit[ 3- 3] */
	int _reserved:29;						/* byte[ 0- 3] ,bit[ 4-31] */
} __ATTRIBUTE_PACKED__;

struct pfe_queue_flush {
	int queue_flush_bit_per_q:32;			/* byte[ 0- 3] ,bit[ 0-31] */
} __ATTRIBUTE_PACKED__;

struct pfe_queue_qece {
/*	uint128_t qece:128;						 byte[ 0-15] ,bit[ 0-127] */
	int qece_1:32;							/* byte[ 0-15] ,bit[ 0-31] */
	int qece_2:32;							/* byte[ 0-15] ,bit[32-63] */
	int qece_3:32;							/* byte[ 0-15] ,bit[64-95] */
	int qece_4:32;							/* byte[ 0-15] ,bit[96-127] */
/*	int _reserved:32;						 byte[ 0- 3] ,bit[ 0-31]  DUMMY*/
} __ATTRIBUTE_PACKED__;



/* REORDER */
struct reorder_ru_pool {
	int sid_limit:32;					/* byte[ 0- 3] ,bit[ 0-31] */
} __ATTRIBUTE_PACKED__;

struct reorder_ru_class_head {
	int ru_class_head:12;				/* byte[ 0- 3] ,bit[ 0-11] */
	int _reserved:20;					/* byte[ 0- 3] ,bit[12-31] */
} __ATTRIBUTE_PACKED__;

struct reorder_ru_ser_error_cause {
	int ru_ser_error_interrupt:1;		/* byte[ 0- 3] ,bit[ 0- 0] */
	int ru_qe_ser_error:1;				/* byte[ 0- 3] ,bit[ 1- 1] */
	int ru_class_ser_error:1;			/* byte[ 0- 3] ,bit[ 2- 2] */
	int ru_tasks_ser_error:1;			/* byte[ 0- 3] ,bit[ 3- 3] */
	int ru_ptr2next_ser_error:1;		/* byte[ 0- 3] ,bit[ 4- 4] */
	int ru_sid_fifo_ser_error:1;		/* byte[ 0- 3] ,bit[ 5- 5] */
	int ru_port2class_ser_error:1;		/* byte[ 0- 3] ,bit[ 6- 6] */
	int ru_rf_error:1;					/* byte[ 0- 3] ,bit[ 7- 7] */
	int _reserved:24;					/* byte[ 0- 3] ,bit[ 8-31] */
} __ATTRIBUTE_PACKED__;

struct reorder_ru_ser_error_mask {
	int ru_ser_error_mask:1;			/* byte[ 0- 3] ,bit[ 0- 0] */
	int ru_qe_ser_error_mask:1;			/* byte[ 0- 3] ,bit[ 1- 1] */
	int ru_class_ser_error_mask:1;		/* byte[ 0- 3] ,bit[ 2- 2] */
	int ru_tasks_ser_error_mask:1;		/* byte[ 0- 3] ,bit[ 3- 3] */
	int ru_ptr2next_ser_error_mask:1;	/* byte[ 0- 3] ,bit[ 4- 4] */
	int ru_sid_fifo_ser_error_mask:1;	/* byte[ 0- 3] ,bit[ 5- 5] */
	int ru_port2class_ser_error_mask:1;	/* byte[ 0- 3] ,bit[ 6- 6] */
	int ru_rf_error_mask:1;				/* byte[ 0- 3] ,bit[ 7- 7] */
	int _reserved:24;					/* byte[ 0- 3] ,bit[ 8-31] */
} __ATTRIBUTE_PACKED__;

struct reorder_ru_host_cmd {
	int ru_host_sid:12;					/* byte[ 0- 3] ,bit[ 0-11] */
	int _reserved_1:4;					/* byte[ 0- 3] ,bit[12-15] */
	int ru_host_class:7;				/* byte[ 0- 3] ,bit[16-22] */
	int _reserved_2:1;					/* byte[ 0- 3] ,bit[23-23] */
	int ru_host_task:3;					/* byte[ 0- 3] ,bit[24-26] */
	int _reserved_3:4;					/* byte[ 0- 3] ,bit[27-30] */
	int ru_host_exec:1;					/* byte[ 0- 3] ,bit[31-31] */
} __ATTRIBUTE_PACKED__;

struct reorder_ru_task_permission {
	int _reserved_1:31;					/* byte[ 0- 3] ,bit[ 0-30] */
	int ru_host_permitted:1;			/* byte[ 0- 3] ,bit[31-31] */
} __ATTRIBUTE_PACKED__;

struct reorder_ru_port2class {
	int ru_class:6;						/* byte[ 0- 3] ,bit[ 0- 5] */
	int ru_pool:1;						/* byte[ 0- 3] ,bit[ 6- 6] */
	int _reserved_1:25;					/* byte[ 0- 3] ,bit[ 7-31] */
} __ATTRIBUTE_PACKED__;


/* GPM */
struct gpm_gpm_pl {
/*	int gpm_pl:512;						 byte[ 0-63] ,bit[  0-511] */
	int gpm_pl_00:32;					/* byte[ 0-63] ,bit[  0- 31] */
	int gpm_pl_01:32;					/* byte[ 0-63] ,bit[ 32- 63] */
	int gpm_pl_02:32;					/* byte[ 0-63] ,bit[ 64- 95] */
	int gpm_pl_03:32;					/* byte[ 0-63] ,bit[ 96-127] */
	int gpm_pl_04:32;					/* byte[ 0-63] ,bit[128-159] */
	int gpm_pl_05:32;					/* byte[ 0-63] ,bit[160-191] */
	int gpm_pl_06:32;					/* byte[ 0-63] ,bit[192-223] */
	int gpm_pl_07:32;					/* byte[ 0-63] ,bit[224-255] */
	int gpm_pl_08:32;					/* byte[ 0-63] ,bit[256-287] */
	int gpm_pl_09:32;					/* byte[ 0-63] ,bit[288-319] */
	int gpm_pl_10:32;					/* byte[ 0-63] ,bit[320-351] */
	int gpm_pl_11:32;					/* byte[ 0-63] ,bit[352-383] */
	int gpm_pl_12:32;					/* byte[ 0-63] ,bit[384-415] */
	int gpm_pl_13:32;					/* byte[ 0-63] ,bit[416-447] */
	int gpm_pl_14:32;					/* byte[ 0-63] ,bit[448-479] */
	int gpm_pl_15:32;					/* byte[ 0-63] ,bit[480-511] */
} __ATTRIBUTE_PACKED__;

struct gpm_gpm_qe {
/*	int gpm_qe:128;						 byte[ 0- 7] ,bit[  0-128] */
	int gpm_qe_00:32;					/* byte[ 0- 7] ,bit[  0- 31] */
	int gpm_qe_01:32;					/* byte[ 0- 7] ,bit[ 32- 63] */
	int gpm_qe_02:32;					/* byte[ 0- 7] ,bit[ 64- 95] */
	int gpm_qe_03:32;					/* byte[ 0- 7] ,bit[ 96-127] */
} __ATTRIBUTE_PACKED__;



/* DQF */
struct dqf_Data_FIFO_params_p {
	int data_fifo_base_p:12;			/* byte[ 0- 3] ,bit[ 0- 11] */
	int _reserved_1:4;					/* byte[ 0- 3] ,bit[12-15] */
	int data_fifo_depth_p:12;			/* byte[ 0- 3] ,bit[16-27] */
	int _reserved_2:4;					/* byte[ 0- 3] ,bit[28-31] */
} __ATTRIBUTE_PACKED__;

struct dqf_Credit_Threshold_p {
	int Credit_Threshold_p:12;			/* byte[ 0- 3] ,bit[ 0- 11] */
	int _reserved_1:20;					/* byte[ 0- 3] ,bit[12-31] */
} __ATTRIBUTE_PACKED__;

struct dqf_PPC_port_map_p {
	int ppc_port_map_p:3;				/* byte[ 0- 3] ,bit[ 0- 2] */
	int _reserved_1:29;					/* byte[ 0- 3] ,bit[ 3-31] */
} __ATTRIBUTE_PACKED__;

struct dqf_data_fifo_pointers_p {
	int data_fifo_wr_ptr_p:13;			/* byte[ 0- 3] ,bit[ 0-12] */
	int _reserved_1:3;					/* byte[ 0- 3] ,bit[13-15] */
	int data_fifo_rd_ptr_p:13;			/* byte[ 0- 3] ,bit[16-28] */
	int _reserved_2:3;					/* byte[ 0- 3] ,bit[29-31] */
} __ATTRIBUTE_PACKED__;

struct dqf_mg2mem_req_addr_ctrl {
	int mg2mem_req_addr:12;				/* byte[ 0- 3] ,bit[ 0- 11] */
	int mg2mem_req_mem_sel:1;			/* byte[ 0- 3] ,bit[12-12] */
	int _reserved_1:19;					/* byte[ 0- 3] ,bit[13-31] */
} __ATTRIBUTE_PACKED__;

struct dqf_dqf_intr_cause {
	int dqf_intr_sum:1;					/* byte[ 0- 3] ,bit[ 0- 0] */
	int dqf_ser_sum:1;					/* byte[ 0- 3] ,bit[ 1- 1] */
	int write_to_full_err_sum:1;		/* byte[ 0- 3] ,bit[ 2- 2] */
	int read_from_empty_err_sum:1;		/* byte[ 0- 3] ,bit[ 3- 3] */
	int wrong_axi_rd_err_sum:1;			/* byte[ 0- 3] ,bit[ 4- 4] */
	int dqf_cs_calc_err:1;				/* byte[ 0- 3] ,bit[ 5- 5] */
	int dqf_cs_inp_ctrl_err:1;			/* byte[ 0- 3] ,bit[ 6- 6] */
	int dqf_rf_error:1;					/* byte[ 0- 3] ,bit[ 7- 7] */
	int _reserved_1:24;					/* byte[ 0- 3] ,bit[ 8-31] */
} __ATTRIBUTE_PACKED__;

struct dqf_dqf_intr_mask {
	int _reserved_1:1;						/* byte[ 0- 3] ,bit[ 0- 0] */
	int dqf_ser_sum_mask:1;					/* byte[ 0- 3] ,bit[ 1- 1] */
	int write_to_full_error_sum_mask:1;		/* byte[ 0- 3] ,bit[ 2- 2] */
	int read_from_empty_error_sum_mask:1;	/* byte[ 0- 3] ,bit[ 3- 3] */
	int wrong_axi_rd_error_sum_mask:1;		/* byte[ 0- 3] ,bit[ 4- 4] */
	int dqf_cs_calc_err_mask:1;				/* byte[ 0- 3] ,bit[ 5- 5] */
	int dqf_cs_inp_ctrl_err_mask:1;			/* byte[ 0- 3] ,bit[ 6- 6] */
	int dqf_rf_error_mask:1;				/* byte[ 0- 3] ,bit[ 7- 7] */
	int _reserved_2:24;						/* byte[ 0- 3] ,bit[ 8-31] */
} __ATTRIBUTE_PACKED__;

struct dqf_misc_error_intr_cause {
	int misc_intr_sum:1;					/* byte[ 0- 3] ,bit[ 0- 0] */
	int dqf_cs_calc_err:1;					/* byte[ 0- 3] ,bit[ 1- 1] */
	int dqf_cs_inp_ctrl_err:1;				/* byte[ 0- 3] ,bit[ 2- 2] */
	int dqf_rf_error:1;						/* byte[ 0- 3] ,bit[ 3- 3] */
	int _reserved_1:28;						/* byte[ 0- 3] ,bit[ 4-31] */
} __ATTRIBUTE_PACKED__;

struct dqf_misc_error_intr_mask {
	int _reserved_1:1;						/* byte[ 0- 3] ,bit[ 0- 0] */
	int dqf_cs_calc_err_mask:1;				/* byte[ 0- 3] ,bit[ 1- 1] */
	int dqf_cs_inp_ctrl_err_mask:1;			/* byte[ 0- 3] ,bit[ 2- 2] */
	int dqf_rf_error_mask:1;				/* byte[ 0- 3] ,bit[ 3- 3] */
	int _reserved_2:28;						/* byte[ 0- 3] ,bit[ 4-31] */
} __ATTRIBUTE_PACKED__;

struct dqf_dqf_ser_summary_intr_cause {
	int ser_summary_intr_sum:1;				/* byte[ 0- 3] ,bit[ 0- 0] */
	int ppe_data_ser_error_0:1;				/* byte[ 0- 3] ,bit[ 1- 1] */
	int ppe_data_ser_error_1:1;				/* byte[ 0- 3] ,bit[ 2- 2] */
	int ppe_data_ser_error_2:1;				/* byte[ 0- 3] ,bit[ 3- 3] */
	int ppe_data_ser_error_3:1;				/* byte[ 0- 3] ,bit[ 4- 4] */
	int ppe_data_ser_error_4:1;				/* byte[ 0- 3] ,bit[ 5- 5] */
	int ppe_data_ser_error_5:1;				/* byte[ 0- 3] ,bit[ 6- 6] */
	int ppe_data_ser_error_6:1;				/* byte[ 0- 3] ,bit[ 7- 7] */
	int ppe_data_ser_error_7:1;				/* byte[ 0- 3] ,bit[ 8- 8] */
	int ppe_data_ser_error_8:1;				/* byte[ 0- 3] ,bit[ 9- 9] */
	int macs_csptr_ser_error_0:1;			/* byte[ 0- 3] ,bit[10-10] */
	int macs_csptr_ser_error_1:1;			/* byte[ 0- 3] ,bit[11-11] */
	int macs_csres_ser_error_0:1;			/* byte[ 0- 3] ,bit[12-12] */
	int macs_csres_ser_error_1:1;			/* byte[ 0- 3] ,bit[13-13] */
	int macs_data_ser_error:1;				/* byte[ 0- 3] ,bit[14-14] */
	int macs_desc_ser_error:1;				/* byte[ 0- 3] ,bit[15-15] */
	int macs_d2cs_ser_error:1;				/* byte[ 0- 3] ,bit[16-16] */
	int _reserved_1:15;						/* byte[ 0- 3] ,bit[17-31] */
} __ATTRIBUTE_PACKED__;

struct dqf_dqf_ser_summary_intr_mask {
	int _reserved_1:1;						/* byte[ 0- 3] ,bit[ 0- 0] */
	int ppe_data_ser_error_0_mask:1;		/* byte[ 0- 3] ,bit[ 1- 1] */
	int ppe_data_ser_error_1_mask:1;		/* byte[ 0- 3] ,bit[ 2- 2] */
	int ppe_data_ser_error_2_mask:1;		/* byte[ 0- 3] ,bit[ 3- 3] */
	int ppe_data_ser_error_3_mask:1;		/* byte[ 0- 3] ,bit[ 4- 4] */
	int ppe_data_ser_error_4_mask:1;		/* byte[ 0- 3] ,bit[ 5- 5] */
	int ppe_data_ser_error_5_mask:1;		/* byte[ 0- 3] ,bit[ 6- 6] */
	int ppe_data_ser_error_6_mask:1;		/* byte[ 0- 3] ,bit[ 7- 7] */
	int ppe_data_ser_error_7_mask:1;		/* byte[ 0- 3] ,bit[ 8- 8] */
	int ppe_data_ser_error_8_mask:1;		/* byte[ 0- 3] ,bit[ 9- 9] */
	int macs_csptr_ser_error_0_mask:1;		/* byte[ 0- 3] ,bit[10-10] */
	int macs_csptr_ser_error_1_mask:1;		/* byte[ 0- 3] ,bit[11-11] */
	int macs_csres_ser_error_0_mask:1;		/* byte[ 0- 3] ,bit[12-12] */
	int macs_csres_ser_error_1_mask:1;		/* byte[ 0- 3] ,bit[13-13] */
	int macs_data_ser_error_mask:1;			/* byte[ 0- 3] ,bit[14-14] */
	int macs_desc_ser_error_mask:1;			/* byte[ 0- 3] ,bit[15-15] */
	int macs_d2cs_ser_error_mask:1;			/* byte[ 0- 3] ,bit[16-16] */
	int _reserved_2:15;						/* byte[ 0- 3] ,bit[17-31] */
} __ATTRIBUTE_PACKED__;

struct dqf_write_to_full_error_intr_cause {
	int write_to_full_intr_sum:1;			/* byte[ 0- 3] ,bit[ 0- 0] */
	int write_to_full_error_p0:1;			/* byte[ 0- 3] ,bit[ 1- 1] */
	int write_to_full_error_p1:1;			/* byte[ 0- 3] ,bit[ 2- 2] */
	int write_to_full_error_p2:1;			/* byte[ 0- 3] ,bit[ 3- 3] */
	int write_to_full_error_p3:1;			/* byte[ 0- 3] ,bit[ 4- 4] */
	int write_to_full_error_p4:1;			/* byte[ 0- 3] ,bit[ 5- 5] */
	int write_to_full_error_p5:1;			/* byte[ 0- 3] ,bit[ 6- 6] */
	int write_to_full_error_p6:1;			/* byte[ 0- 3] ,bit[ 7- 7] */
	int write_to_full_error_p7:1;			/* byte[ 0- 3] ,bit[ 8- 8] */
	int write_to_full_error_p8:1;			/* byte[ 0- 3] ,bit[ 9- 9] */
	int write_to_full_error_p9:1;			/* byte[ 0- 3] ,bit[10-10] */
	int write_to_full_error_p10:1;			/* byte[ 0- 3] ,bit[11-11] */
	int write_to_full_error_p11:1;			/* byte[ 0- 3] ,bit[12-12] */
	int write_to_full_error_p12:1;			/* byte[ 0- 3] ,bit[13-13] */
	int write_to_full_error_p13:1;			/* byte[ 0- 3] ,bit[14-14] */
	int write_to_full_error_p14:1;			/* byte[ 0- 3] ,bit[15-15] */
	int write_to_full_error_p15:1;			/* byte[ 0- 3] ,bit[16-16] */
	int _reserved_1:15;						/* byte[ 0- 3] ,bit[17-31] */
} __ATTRIBUTE_PACKED__;

struct dqf_write_to_full_error_intr_mask {
	int _reserved_1:1;						/* byte[ 0- 3] ,bit[ 0- 0] */
	int write_to_full_error_mask_p0:1;		/* byte[ 0- 3] ,bit[ 1- 1] */
	int write_to_full_error_mask_p1:1;		/* byte[ 0- 3] ,bit[ 2- 2] */
	int write_to_full_error_mask_p2:1;		/* byte[ 0- 3] ,bit[ 3- 3] */
	int write_to_full_error_mask_p3:1;		/* byte[ 0- 3] ,bit[ 4- 4] */
	int write_to_full_error_mask_p4:1;		/* byte[ 0- 3] ,bit[ 5- 5] */
	int write_to_full_error_mask_p5:1;		/* byte[ 0- 3] ,bit[ 6- 6] */
	int write_to_full_error_mask_p6:1;		/* byte[ 0- 3] ,bit[ 7- 7] */
	int write_to_full_error_mask_p7:1;		/* byte[ 0- 3] ,bit[ 8- 8] */
	int write_to_full_error_mask_p8:1;		/* byte[ 0- 3] ,bit[ 9- 9] */
	int write_to_full_error_mask_p9:1;		/* byte[ 0- 3] ,bit[10-10] */
	int write_to_full_error_mask_p10:1;		/* byte[ 0- 3] ,bit[11-11] */
	int write_to_full_error_mask_p11:1;		/* byte[ 0- 3] ,bit[12-12] */
	int write_to_full_error_mask_p12:1;		/* byte[ 0- 3] ,bit[13-13] */
	int write_to_full_error_mask_p13:1;		/* byte[ 0- 3] ,bit[14-14] */
	int write_to_full_error_mask_p14:1;		/* byte[ 0- 3] ,bit[15-15] */
	int write_to_full_error_mask_p15:1;		/* byte[ 0- 3] ,bit[16-16] */
	int _reserved_2:15;						/* byte[ 0- 3] ,bit[17-31] */
} __ATTRIBUTE_PACKED__;

struct dqf_read_from_empty_error_intr_cause {
	int read_from_empty_intr_sum:1;			/* byte[ 0- 3] ,bit[ 0- 0] */
	int read_from_empty_error_p0:1;			/* byte[ 0- 3] ,bit[ 1- 1] */
	int read_from_empty_error_p1:1;			/* byte[ 0- 3] ,bit[ 2- 2] */
	int read_from_empty_error_p2:1;			/* byte[ 0- 3] ,bit[ 3- 3] */
	int read_from_empty_error_p3:1;			/* byte[ 0- 3] ,bit[ 4- 4] */
	int read_from_empty_error_p4:1;			/* byte[ 0- 3] ,bit[ 5- 5] */
	int read_from_empty_error_p5:1;			/* byte[ 0- 3] ,bit[ 6- 6] */
	int read_from_empty_error_p6:1;			/* byte[ 0- 3] ,bit[ 7- 7] */
	int read_from_empty_error_p7:1;			/* byte[ 0- 3] ,bit[ 8- 8] */
	int read_from_empty_error_p8:1;			/* byte[ 0- 3] ,bit[ 9- 9] */
	int read_from_empty_error_p9:1;			/* byte[ 0- 3] ,bit[10-10] */
	int read_from_empty_error_p10:1;		/* byte[ 0- 3] ,bit[11-11] */
	int read_from_empty_error_p11:1;		/* byte[ 0- 3] ,bit[12-12] */
	int read_from_empty_error_p12:1;		/* byte[ 0- 3] ,bit[13-13] */
	int read_from_empty_error_p13:1;		/* byte[ 0- 3] ,bit[14-14] */
	int read_from_empty_error_p14:1;		/* byte[ 0- 3] ,bit[15-15] */
	int read_from_empty_error_p15:1;		/* byte[ 0- 3] ,bit[16-16] */
	int _reserved_1:15;						/* byte[ 0- 3] ,bit[17-31] */
} __ATTRIBUTE_PACKED__;

struct dqf_read_from_empty_error_intr_mask {
	int _reserved_1:1;						/* byte[ 0- 3] ,bit[ 0- 0] */
	int read_from_empty_error_mask_p0:1;	/* byte[ 0- 3] ,bit[ 1- 1] */
	int read_from_empty_error_mask_p1:1;	/* byte[ 0- 3] ,bit[ 2- 2] */
	int read_from_empty_error_mask_p2:1;	/* byte[ 0- 3] ,bit[ 3- 3] */
	int read_from_empty_error_mask_p3:1;	/* byte[ 0- 3] ,bit[ 4- 4] */
	int read_from_empty_error_mask_p4:1;	/* byte[ 0- 3] ,bit[ 5- 5] */
	int read_from_empty_error_mask_p5:1;	/* byte[ 0- 3] ,bit[ 6- 6] */
	int read_from_empty_error_mask_p6:1;	/* byte[ 0- 3] ,bit[ 7- 7] */
	int read_from_empty_error_mask_p7:1;	/* byte[ 0- 3] ,bit[ 8- 8] */
	int read_from_empty_error_mask_p8:1;	/* byte[ 0- 3] ,bit[ 9- 9] */
	int read_from_empty_error_mask_p9:1;	/* byte[ 0- 3] ,bit[10-10] */
	int read_from_empty_error_mask_p10:1;	/* byte[ 0- 3] ,bit[11-11] */
	int read_from_empty_error_mask_p11:1;	/* byte[ 0- 3] ,bit[12-12] */
	int read_from_empty_error_mask_p12:1;	/* byte[ 0- 3] ,bit[13-13] */
	int read_from_empty_error_mask_p13:1;	/* byte[ 0- 3] ,bit[14-14] */
	int read_from_empty_error_mask_p14:1;	/* byte[ 0- 3] ,bit[15-15] */
	int read_from_empty_error_mask_p15:1;	/* byte[ 0- 3] ,bit[16-16] */
	int _reserved_2:15;						/* byte[ 0- 3] ,bit[17-31] */
} __ATTRIBUTE_PACKED__;

struct dqf_wrong_axi_rd_error_intr_cause {
	int wrong_axi_rd_intr_sum:1;			/* byte[ 0- 3] ,bit[ 0- 0] */
	int wrong_axi_rd_error_p0:1;			/* byte[ 0- 3] ,bit[ 1- 1] */
	int wrong_axi_rd_error_p1:1;			/* byte[ 0- 3] ,bit[ 2- 2] */
	int wrong_axi_rd_error_p2:1;			/* byte[ 0- 3] ,bit[ 3- 3] */
	int wrong_axi_rd_error_p3:1;			/* byte[ 0- 3] ,bit[ 4- 4] */
	int wrong_axi_rd_error_p4:1;			/* byte[ 0- 3] ,bit[ 5- 5] */
	int wrong_axi_rd_error_p5:1;			/* byte[ 0- 3] ,bit[ 6- 6] */
	int wrong_axi_rd_error_p6:1;			/* byte[ 0- 3] ,bit[ 7- 7] */
	int wrong_axi_rd_error_p7:1;			/* byte[ 0- 3] ,bit[ 8- 8] */
	int wrong_axi_rd_error_p8:1;			/* byte[ 0- 3] ,bit[ 9- 9] */
	int wrong_axi_rd_error_p9:1;			/* byte[ 0- 3] ,bit[10-10] */
	int wrong_axi_rd_error_p10:1;			/* byte[ 0- 3] ,bit[11-11] */
	int wrong_axi_rd_error_p11:1;			/* byte[ 0- 3] ,bit[12-12] */
	int wrong_axi_rd_error_p12:1;			/* byte[ 0- 3] ,bit[13-13] */
	int wrong_axi_rd_error_p13:1;			/* byte[ 0- 3] ,bit[14-14] */
	int wrong_axi_rd_error_p14:1;			/* byte[ 0- 3] ,bit[15-15] */
	int wrong_axi_rd_error_p15:1;			/* byte[ 0- 3] ,bit[16-16] */
	int _reserved_1:15;						/* byte[ 0- 3] ,bit[17-31] */
} __ATTRIBUTE_PACKED__;

struct dqf_wrong_axi_rd_error_intr_mask {
	int _reserved_1:1;						/* byte[ 0- 3] ,bit[ 0- 0] */
	int wrong_axi_rd_error_mask_p0:1;		/* byte[ 0- 3] ,bit[ 1- 1] */
	int wrong_axi_rd_error_mask_p1:1;		/* byte[ 0- 3] ,bit[ 2- 2] */
	int wrong_axi_rd_error_mask_p2:1;		/* byte[ 0- 3] ,bit[ 3- 3] */
	int wrong_axi_rd_error_mask_p3:1;		/* byte[ 0- 3] ,bit[ 4- 4] */
	int wrong_axi_rd_error_mask_p4:1;		/* byte[ 0- 3] ,bit[ 5- 5] */
	int wrong_axi_rd_error_mask_p5:1;		/* byte[ 0- 3] ,bit[ 6- 6] */
	int wrong_axi_rd_error_mask_p6:1;		/* byte[ 0- 3] ,bit[ 7- 7] */
	int wrong_axi_rd_error_mask_p7:1;		/* byte[ 0- 3] ,bit[ 8- 8] */
	int wrong_axi_rd_error_mask_p8:1;		/* byte[ 0- 3] ,bit[ 9- 9] */
	int wrong_axi_rd_error_mask_p9:1;		/* byte[ 0- 3] ,bit[10-10] */
	int wrong_axi_rd_error_mask_p10:1;		/* byte[ 0- 3] ,bit[11-11] */
	int wrong_axi_rd_error_mask_p11:1;		/* byte[ 0- 3] ,bit[12-12] */
	int wrong_axi_rd_error_mask_p12:1;		/* byte[ 0- 3] ,bit[13-13] */
	int wrong_axi_rd_error_mask_p13:1;		/* byte[ 0- 3] ,bit[14-14] */
	int wrong_axi_rd_error_mask_p14:1;		/* byte[ 0- 3] ,bit[15-15] */
	int wrong_axi_rd_error_mask_p15:1;		/* byte[ 0- 3] ,bit[16-16] */
	int _reserved_2:15;						/* byte[ 0- 3] ,bit[17-31] */
} __ATTRIBUTE_PACKED__;

struct dqf_mem2mg_resp_status {
	int mem2mg_resp_ready:1;			/* byte[ 0- 3] ,bit[ 0- 0] */
	int mem2mg_resp_sop:1;				/* byte[ 0- 3] ,bit[ 1- 1] */
	int mem2mg_resp_eop:1;				/* byte[ 0- 3] ,bit[ 2- 2] */
	int _reserved_1:29;					/* byte[ 0- 3] ,bit[ 3-31] */
} __ATTRIBUTE_PACKED__;

struct dqf_mem2mg_resp_data_hh {
	int mem2mg_resp_data_hh:32;			/* byte[ 0- 3] ,bit[ 0-31] */
} __ATTRIBUTE_PACKED__;

struct dqf_mem2mg_resp_data_hl {
	int mem2mg_resp_data_hl:32;			/* byte[ 0- 3] ,bit[ 0-31] */
} __ATTRIBUTE_PACKED__;

struct dqf_mem2mg_resp_data_lh {
	int mem2mg_resp_data_lh:32;			/* byte[ 0- 3] ,bit[ 0-31] */
} __ATTRIBUTE_PACKED__;

struct dqf_mem2mg_resp_data_ll {
	int mem2mg_resp_data_ll:32;			/* byte[ 0- 3] ,bit[ 0-31] */
} __ATTRIBUTE_PACKED__;

struct l3_result {
	int l3_res:16;						/* byte[ 0- 3] ,bit[ 0-15] */
	int _reserved_1:16;					/* byte[ 0- 3] ,bit[16-31] */
} __ATTRIBUTE_PACKED__;

struct dqf_dqf_macs_l3_res {
	struct l3_result reg_l3_result;
} __ATTRIBUTE_PACKED__;

struct l4_result {
	int l3_res:16;						/* byte[ 0- 3] ,bit[ 0-15] */
	int _reserved_1:16;					/* byte[ 0- 3] ,bit[16-31] */
} __ATTRIBUTE_PACKED__;

struct dqf_dqf_macs_l4_res {
	struct l4_result reg_l4_result;
} __ATTRIBUTE_PACKED__;

struct l3_pointer {
	int l3_ptr:16;						/* byte[ 0- 3] ,bit[ 0-15] */
	int _reserved_1:16;					/* byte[ 0- 3] ,bit[16-31] */
} __ATTRIBUTE_PACKED__;

struct dqf_dqf_macs_l3_ptr {
	struct l3_pointer reg_l3_pointer;
} __ATTRIBUTE_PACKED__;

struct l4_pointer {
	int l4_ptr:16;						/* byte[ 0- 3] ,bit[ 0-15] */
	int _reserved_1:16;					/* byte[ 0- 3] ,bit[16-31] */
} __ATTRIBUTE_PACKED__;

struct dqf_dqf_macs_l4_ptr {
	struct l4_pointer reg_l4_pointer;
} __ATTRIBUTE_PACKED__;

struct desc {
	int macs_desc_1:32;					/* byte[ 0- 7] ,bit[ 0-31] */
	int macs_desc_2:2;					/* byte[ 0- 7] ,bit[32-33] */
	int _reserved_1:30;					/* byte[ 0- 7] ,bit[34-63] */
} __ATTRIBUTE_PACKED__;

struct dqf_dqf_macs_desc {
	struct desc reg_desc;
} __ATTRIBUTE_PACKED__;



/* QM General */


struct qm_alias {

	struct {
		int base;
		int qptr;
		int qmap_port;
		int qmap_group;
		int rule_bpi;
		int low_threshold;
		int pause_threshold;
		int high_threshold;
		int traffic_source;
		int ECC_error_cause;
		int ECC_error_mask;
		int Internal_error_cause;
		int internal_error_mask;
		int nss_general_purpose;
		int qlen;
		int bp_qlen;
		int drop_qlen;
		int xoff_when_bm_empty_en;
		int xoff_mac_qnum;
		int xoff_hmac_qs;
	} ql;

	struct {
		int base;
		int qece_dram_base_address_hi;
		int pyld_dram_base_address_hi;
		int qece_dram_base_address_lo;
		int pyld_dram_base_address_lo;
		int QM_VMID;
		int port_flush;
		int AXI_read_attributes_for_swf_mode;
		int AXI_read_attributes_for_rdma_mode;
		int AXI_read_attributes_for_hwf_qece;
		int AXI_read_attributes_for_hwf_pyld;
		int max_credit_for_new_dram_req;
		int ecc_error_cause;
		int ecc_error_mask;
		int internal_error_cause;
		int internal_error_mask;
		int idle_status;
		int queue_flush;
		int queue_qece;
	} pfe;

	struct {
		int base;
		int Data_FIFO_params_p;
		int Credit_Threshold_p;
		int PPC_port_map_p;
		int data_fifo_pointers_p;
		int dqf_itnr_cause;
		int dqf_itnr_mask;
		int misc_error_intr_cause;
		int misc_error_intr_mask;
		int dqf_ser_summary_intr_cause;
		int dqf_ser_summary_intr_mask;
		int write_to_full_error_intr_cause;
		int write_to_full_error_intr_mask;
		int read_from_empty_error_intr_cause;
		int read_from_empty_error_intr_mask;
		int wrong_axi_rd_error_intr_cause;
		int wrong_axi_rd_error_intr_mask;
		int mg2mem_req_addr_ctrl;
		int mem2mg_resp_status;
		int mem2mg_resp_data_hh;
		int mem2mg_resp_data_hl;
		int mem2mg_resp_data_lh;
		int mem2mg_resp_data_ll;
		int dqf_macs_l3_res;
		int dqf_macs_l4_res;
		int dqf_macs_l3_ptr;
		int dqf_macs_l4_ptr;
		int dqf_macs_desc;
	} dqf;

	struct {
		int base;
		int Q_memory_allocation;
		int gpm_thresholds;
		int dram_thresholds;
		int AXI_write_attributes_for_swf_mode;
		int AXI_write_attributes_for_rdma_mode;
		int AXI_write_attributes_for_hwf_qece;
		int AXI_write_attributes_for_hwf_pyld;
		int DRAM_VMID;
		int tail_pointer_en;
		int idle_status;
		int ecc_error_cause;
		int ecc_error_mask;
		int internal_error_cause;
		int internal_error_mask;
		int ceram_mac;
		int ceram_ppe;
		int qeram;
		int dram_fifo;
	} dma;

	struct {
		int base;
		int ErrStus;
	} sched;

	struct {
		int base;
		int DrpErrStus;
		int DrpFirstExc;
		int DrpErrCnt;
		int DrpExcCnt;
		int DrpExcMask;
		int DrpId;
		int DrpForceErr;
		int WREDDropProbMode;
		int WREDMaxProbModePerColor;
		int DPSource;
		int RespLocalDPSel;
		int Drp_Decision_to_Query_debug;
		int Drp_Decision_hierarchy_to_Query_debug;
		int TMtoTMPktGenQuantum;
		int TMtoTMDPCoSSel;
		int AgingUpdEnable;
		int PortInstAndAvgQueueLength;
		int DrpEccConfig;
		int DrpEccMemParams;
	} drop;

	struct {
		int base;
		int ru_qe;
		int ru_class;
		int ru_tasks;
		int ru_ptr2next;
		int ru_sid_fifo;
		int ru_port2class;
		int ru_pool;
		int ru_class_head;
		int ru_ser_error_cause;
		int ru_ser_error_mask;
		int ru_host_cmd;
		int ru_task_permission;
	} reorder;

};	/* QM; */

extern struct qm_alias qm;
extern struct qm_alias qm_reg_size;
extern struct qm_alias qm_reg_offset;

void qm_reg_address_alias_init(u32 silicon_base);
void  qm_reg_size_alias_init(void);
void qm_reg_offset_alias_init(void);

#endif   /* __mv_qm_regs_h__ */

