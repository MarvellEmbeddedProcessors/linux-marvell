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

#ifndef __mv_bm_mem_h__
#define __mv_bm_mem_h__

/* SRAM_B0_CACHE Table */
#define BM_SRAM_B0_CACHE_TBL_ENTRY(i)				(0x4E0000 + i*16)
#define BM_SRAM_B0_CACHE_TBL_ENTRY_SIZE				(128)
#define BM_SRAM_B0_CACHE_TBL_ENTRY_WORDS			\
		(MV_ALIGN_UP(BM_SRAM_B0_CACHE_TBL_ENTRY_SIZE, 32) / 32)
#define BM_SRAM_B0_CACHE_DATA_3_OFFS				96
#define BM_SRAM_B0_CACHE_DATA_3_BITS				22
#define BM_SRAM_B0_CACHE_DATA_2_OFFS				64
#define BM_SRAM_B0_CACHE_DATA_2_BITS				22
#define BM_SRAM_B0_CACHE_DATA_1_OFFS				32
#define BM_SRAM_B0_CACHE_DATA_1_BITS				22
#define BM_SRAM_B0_CACHE_DATA_0_OFFS				0
#define BM_SRAM_B0_CACHE_DATA_0_BITS				22

/* SRAM_bank 1-4 CACHE Table */
#define BM_SRAM_BGP_CACHE_TBL_ENTRY(bid, i)			(0x4E0000 + (bid*0x4000) + i*8)
#define BM_SRAM_BGP_CACHE_TBL_ENTRY_SIZE			(40)
#define BM_SRAM_BGP_CACHE_TBL_ENTRY_WORDS			\
		(MV_ALIGN_UP(BM_SRAM_BGP_CACHE_TBL_ENTRY_SIZE, 32) / 32)
#define BM_SRAM_BGP_CACHE_DATA_OFFS				0
#define BM_SRAM_BGP_CACHE_DATA_BITS				40


/* dpr c mng bank [0-4] stat table */
#define BM_DPR_C_MNG_BANK_STAT_TBL_ENTRY(bid, i)		(0x4D0000 + (bid*0x400) + i*16)
#define BM_DPR_C_MNG_BANK_STAT_TBL_ENTRY_SIZE			(96)
#define BM_DPR_C_MNG_BANK_STAT_TBL_ENTRY_WORDS			\
		(MV_ALIGN_UP(BM_DPR_C_MNG_BANK_STAT_TBL_ENTRY_SIZE, 32) / 32)
#define BM_DPR_C_MNG_BANK_STAT_CACHE_START_OFFS			0
#define BM_DPR_C_MNG_BANK_STAT_CACHE_START_BITS			7
#define BM_DPR_C_MNG_BANK_STAT_CACHE_SI_THR_OFFS		32
#define BM_DPR_C_MNG_BANK_STAT_CACHE_SI_THR_BITS		11
#define BM_DPR_C_MNG_BANK_STAT_CACHE_END_OFFS			16
#define BM_DPR_C_MNG_BANK_STAT_CACHE_END_BITS			7
#define BM_DPR_C_MNG_BANK_STAT_CACHE_SO_THR_OFFS		48
#define BM_DPR_C_MNG_BANK_STAT_CACHE_SO_THR_BITS		11
#define BM_DPR_C_MNG_BANK_STAT_CACHE_ATTR_OFFS			64
#define BM_DPR_C_MNG_BANK_STAT_CACHE_ATTR_BITS			8
#define BM_DPR_C_MNG_BANK_STAT_CACHE_VMID_OFFS			80
#define BM_DPR_C_MNG_BANK_STAT_CACHE_VMID_BITS			8

/* tpr c mng bank [0-4] dyn table */
#define BM_TPR_C_MNG_BANK_DYN_TBL_ENTRY(bid, i)			(0x4D1400 + (0x200*bid) + i*8)
#define BM_TPR_C_MNG_BANK_DYN_TBL_ENTRY_SIZE			(64)
#define BM_TPR_C_MNG_BANK_DYN_TBL_ENTRY_WORDS			\
		(MV_ALIGN_UP(BM_TPR_C_MNG_BANK_DYN_TBL_ENTRY_SIZE, 32) / 32)
#define BM_TPR_C_MNG_BANK_DYN_CACHE_FILL_MIN_OFFS		0
#define BM_TPR_C_MNG_BANK_DYN_CACHE_FILL_MIN_BITS		10
#define BM_TPR_C_MNG_BANK_DYN_CACHE_RD_PTR_OFFS			32
#define BM_TPR_C_MNG_BANK_DYN_CACHE_RD_PTR_BITS			10
#define BM_TPR_C_MNG_BANK_DYN_CACHE_FILL_MAX_OFFS		16
#define BM_TPR_C_MNG_BANK_DYN_CACHE_FILL_MAX_BITS		10
#define BM_TPR_C_MNG_BANK_DYN_CACHE_WR_PTR_OFFS			48
#define BM_TPR_C_MNG_BANK_DYN_CACHE_WR_PTR_BITS			10

/* DPR_D_MNG_BALL_STAT Table */
#define BM_DPR_D_MNG_BALL_STAT_TBL_ENTRY(i)			(0x4D2000 + i*32)
#define BM_DPR_D_MNG_BALL_STAT_TBL_ENTRY_SIZE			(160)
#define BM_DPR_D_MNG_BALL_STAT_TBL_ENTRY_WORDS		        \
		(MV_ALIGN_UP(BM_DPR_D_MNG_BALL_STAT_TBL_ENTRY_SIZE, 32) / 32)
#define BM_DPR_D_MNG_BALL_STAT_DRAM_AE_THR_OFFS			0
#define BM_DPR_D_MNG_BALL_STAT_DRAM_AE_THR_BITS			18
#define BM_DPR_D_MNG_BALL_STAT_DRAM_AF_THR_OFFS			32
#define BM_DPR_D_MNG_BALL_STAT_DRAM_AF_THR_BITS			18
#define BM_DPR_D_MNG_BALL_STAT_DRAM_START_LSB_OFFS		64
#define BM_DPR_D_MNG_BALL_STAT_DRAM_START_LSB_BITS		32
#define BM_DPR_D_MNG_BALL_STAT_DRAM_START_MSB_OFFS		96
#define BM_DPR_D_MNG_BALL_STAT_DRAM_START_MSB_BITS		8
#define BM_DPR_D_MNG_BALL_STAT_DRAM_SIZE_OFFS                  128
#define BM_DPR_D_MNG_BALL_STAT_DRAM_SIZE_BITS                  18



/* TPR_DRO_MNG_BALL_DYN Table */
#define BM_TPR_DRO_MNG_BALL_DYN_TBL_ENTRY(i)			(0x4D4000 + i*8)
#define BM_TPR_DRO_MNG_BALL_DYN_TBL_ENTRY_SIZE			(64)
#define BM_TPR_DRO_MNG_BALL_DYN_TBL_ENTRY_WORDS			\
		(MV_ALIGN_UP(BM_TPR_DRO_MNG_BALL_DYN_TBL_ENTRY_SIZE, 32) / 32)
#define BM_TPR_DRO_MNG_BALL_DYN_DRAM_RD_PTR_OFFS		0
#define BM_TPR_DRO_MNG_BALL_DYN_DRAM_RD_PTR_BITS		21
#define BM_TPR_DRO_MNG_BALL_DYN_DRAM_WR_PTR_OFFS		32
#define BM_TPR_DRO_MNG_BALL_DYN_DRAM_WR_PTR_BITS		21

/* TPR_DRW_MNG_BALL_DYN Table */
#define BM_TPR_DRW_MNG_BALL_DYN_TBL_ENTRY(i)			(0x4D4800 + i*4)
#define BM_TPR_DRW_MNG_BALL_DYN_TBL_ENTRY_SIZE			(32)
#define BM_TPR_DRW_MNG_BALL_DYN_TBL_ENTRY_WORDS			\
		(MV_ALIGN_UP(BM_TPR_DRW_MNG_BALL_DYN_TBL_ENTRY_SIZE, 32) / 32)
#define BM_TPR_DRW_MNG_BALL_DYN_DRAM_FILL_OFFS			0
#define BM_TPR_DRW_MNG_BALL_DYN_DRAM_FILL_BITS			21

/* tpr cntrs bnak [0-4] table */
#define BM_TPR_CTRS_BANK_TBL_ENTRY(bid, pid)			(0x4D5000 + (0x400*bid) + (pid*16))
#define BM_TPR_CTRS_BANK_TBL_ENTRY_SIZE				(128)
#define BM_TPR_CTRS_BANK_TBL_ENTRY_WORDS			\
		(MV_ALIGN_UP(BM_TPR_CTRS_BANK_TBL_ENTRY_SIZE, 32) / 32)
#define BM_TPR_CTRS_BANK_DELAYED_RELEASES_CTR_OFFS		0
#define BM_TPR_CTRS_BANK_DELAYED_RELEASES_CTR_BITS		32
#define BM_TPR_CTRS_BANK_RELEASED_PES_CTR_OFFS			64
#define BM_TPR_CTRS_BANK_RELEASED_PES_CTR_BITS			32
#define BM_TPR_CTRS_BANK_FAILED_ALLOCS_CTR_OFFS			32
#define BM_TPR_CTRS_BANK_FAILED_ALLOCS_CTR_BITS			32
#define BM_TPR_CTRS_BANK_ALLOCATED_PES_CTR_OFFS			96
#define BM_TPR_CTRS_BANK_ALLOCATED_PES_CTR_BITS			32

#endif /* __mv_bm_mem_h__ */
