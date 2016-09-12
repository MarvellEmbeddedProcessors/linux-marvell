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
#ifndef	__mv_bm_h__
#define	__mv_bm_h__

#include "common/mv_sw_if.h"

/* QM debug flags */
#define BM_F_DBG_RD_BIT			0
#define BM_F_DBG_WR_BIT			1

#define BM_F_DBG_RD			(1 << BM_F_DBG_RD_BIT)
#define BM_F_DBG_WR			(1 << BM_F_DBG_WR_BIT)

/*----------------------- cache bank memory HW definitions -----------------------*/

#define BM_QM_CACHE_LINES			512
#define BM_GP_CACHE_LINES			1024 /* 1024 in each bank */
#define BM_GP_CACHE_LINE_BYTES			8
#define BM_QM_CACHE_LINE_BYTES			16
#define BM_CACHE_LINE_BYTES(pool)		((pool >= BM_GP_POOL_MIN) ? \
						BM_GP_CACHE_LINE_BYTES : BM_QM_CACHE_LINE_BYTES)
/*
 * size of PE is always 32 bits for QM pools (0 to 3)
 * but can be either 32bits or 40 bits for GP pools (8 to 35)
 */

#define BM_QM_CACHE_PE_BYTES			4
#define BM_GP_CACHE_PE_BYTES			8
#define BM_CACHE_PE_BYTES(pool)			((pool >= BM_GP_POOL_MIN) ? \
							BM_GP_CACHE_PE_BYTES : BM_QM_CACHE_PE_BYTES)
#define BM_PES_IN_CACHE_LINE(pool)		(BM_CACHE_LINE_BYTES(pool) / BM_CACHE_PE_BYTES(pool))

#define BM_BANK_MIN				0
#define BM_BANK_MAX				4
#define BM_QM_BANK_NUM				1
#define BM_GP_BANK_NUM				4
#define BM_BANK_NUM				(BM_QM_BANK_NUM + BM_GP_BANK_NUM)


#define BM_QM_BANK_POOLS_NUM			(BM_QM_GPM_POOLS_NUM + BM_QM_DRAM_POOLS_NUM)
#define BM_GP_BANK_POOLS_NUM			7
#define BM_BANK_POOLS_NUM(bank)			((bank == 0) ? BM_QM_BANK_POOLS_NUM : BM_GP_BANK_POOLS_NUM)

#ifdef CONFIG_MV_PP3_FPGA
/* FPGA */
#define BM_QM_GPM_POOL_CAPACITY			512
#define BM_QM_CACHE_BUF_NUM			(512 - 16)
#define BM_QM_CACHE_SI				80
#define BM_QM_CACHE_SO				112
#else /* CONFIG_MV_PP3_FPGA */

#define BM_QM_GPM_POOL_CAPACITY			6400
#define BM_QM_CACHE_BUF_NUM			512
#define BM_QM_CACHE_SI				448
#define BM_QM_CACHE_SO				480
#endif /* !CONFIG_MV_PP3_FPGA */

#define BM_QM_DRAM_POOL_CAPACITY		512

/*------------------------ cache memory thresholds ------------------------*/

/* small partition*/
#define BM_GP_CACHE_BUF_NUM			144
#define BM_GP_CACHE_SI				(BM_GP_CACHE_BUF_NUM - 32)
#define BM_GP_CACHE_SO				(BM_GP_CACHE_BUF_NUM - 16)

/*--------------------------- dram definitions ---------------------------*/

#define BM_DRAM_LINE_BYTES			64

#ifdef CONFIG_MV_PP3_FPGA
#define BM_DRAM_AE(_buf_num_)			80
#define BM_DRAM_AF(_buf_num_)			112
#else /* CONFIG_MV_PP3_FPGA */
#define BM_DRAM_AE(_buf_num_)			(((_buf_num_ * 1/8) > 16) ? MV_ALIGN_DOWN(_buf_num_/8, 16) : 16)
#define BM_DRAM_AF(_buf_num_)			MV_ALIGN_DOWN((_buf_num_ * 3/4), 16)
#endif /* !CONFIG_MV_PP3_FPGA */


/*--------------------------- pools definitions --------------------------*/
#define BM_TOT_AE				300 /* total almost empty threshold */
#define BM_TOT_AAE				400 /* total almost almost empty threshold */

#define BM_QM_GPM_POOL_0			0
#define BM_QM_GPM_POOL_1			1
#define BM_QM_GPM_POOLS_NUM			(1 + BM_QM_GPM_POOL_1 - BM_QM_GPM_POOL_0)

#define BM_QM_DRAM_POOL_0			2
#define BM_QM_DRAM_POOL_1			3
#define BM_QM_DRAM_POOLS_NUM			(1 + BM_QM_DRAM_POOL_1 - BM_QM_DRAM_POOL_0)

#define BM_GP_POOL_MIN				8
#define BM_GP_POOL_MAX				35
#define BM_GP_POOLS_NUM				(BM_GP_BANK_POOLS_NUM * BM_GP_BANK_NUM)

#define BM_POOLS_NUM				(BM_GP_POOL_MAX + 1)

#define BM_IS_QM_GPM_POOL(pool)			(pool < BM_QM_GPM_POOL_0 || pool > BM_QM_GPM_POOL_1)

/* pool 2 buffer size is 1024, pool 3 buffer size is 16 */
#define BM_QM_DRAM_POOL_BUF_SIZE(pool)		((pool == 2) ? 1024 : 16)



/*--------------------------- global definitions --------------------------*/

/*
 * domain read/write for any pool in the range 0 to 3 (2 bits)
 * Cache read/write for any pool in the range 0 to 15 (4 bits)
 * QOS read/write for any pool in the range 0 to 3 (2 bits)
*/

#define BM_AWCACHE				0x7
#define BM_ARCACHE				0xB
#define BM_ADOMAIN				0x2
#define BM_ARQOS				0x1
#define BM_AWQOS				0x0
#define BM_VMID					0x0
#define BM_CACHE_ATTR				0x1

/*--------------------------------- APIs -----------------------------------*/

/* init bm base addres */
void mv_pp3_bm_init(void __iomem *base);

/* Enable/Disable BM registers read and write dumps */
void bm_dbg_flags(u32 flag, u32 en);

int bm_gp_pid_validation(int pool);

/**
 *  Global functions, configures BM attributes for read/write in DRAM
 *  configures all 12 attributes with default values
 *  Return values:
 *		0 - success
 */
void bm_attr_all_pools_def_set(void);

/**
 *  Initiates of GPM pools with default values
 *
 *  Return values:
 *		0 - success
 */
int bm_qm_gpm_pools_def_quick_init(int buf_num, struct mv_a40 *p0_base, struct mv_a40 *p1_base);

/**
 *  Initiates of DRAM pools with default values
 *
 *  Return values:
 *		0 - success
 */
int bm_qm_dram_pools_def_quick_init(struct device *dev, int buf_num, struct mv_a40 *p0_base, struct mv_a40 *p1_base);

/**
 *  Basic initialization of general purpose pools with default values
 *  enable pair mode, disable quick init
 *  parameters:
 *   pool - general purpose pools 8 to 35
 *   buf_num - number of buffers
 *   base_address - DRAM base address
 *  Return values:
 *		0 - success
 */
int bm_gp_pool_def_basic_init(int pool, int buf_num, struct mv_a40 *base_address);

/**
 *  Global enable for BM
 *
 */
void bm_enable(void);

/**
 *  Global disable for BM
 */
void bm_disable(void);


/**
 *  Print all global registers
 *  Return values:
 *		0 - success
 */
void bm_global_registers_dump(void);

/**
 *  Print values of all BM pool registers
 *  Return values:
 *		0 - success
 */
void bm_pool_registers_dump(int pool);

/**
 *  Print parsed values of all BM pool registers
 */
void bm_pool_registers_parse(int pool);


/**
 *  Print values of all BM bank registers
 */
void bm_bank_registers_dump(int bank);

/**
 *  Print all 512 lines of cache per input bank sram_b0...b4_cache_mem
 *  Return values:
 *		0 - success
 */
void bm_bank_cache_dump(int bank);

/**
 *  Print several registers status that gives indication why BM is busy
 *  This funciton is useful to call if BM is not reaching idle after a long time.
 *
 *  Return values:
 *		0 - success
 */
void bm_idle_status_dump(void);

/**
 *  Check error bits If set to 1, and print the error that occurred.
 *  Bit 0 is always an OR of all the other errors.
 *  So bit 0 with value 0 indicates there are no errors.
 *
 *  Return values:
 *		0 - success
 */
void bm_error_dump(void);

/**
 *  Enables BM pool
 *
 *  Note: Must be called after all other pool related configuration is complete
 *  Return values:
 *		0 - success
 */
int bm_pool_enable(int pool);
/**
 *  Set Pool to disable - TBD
 *
 *  Return values:
 *		0 - success
 */
int bm_pool_disable(int pool);


/**
 *  Print pool status
 *
 *  Return values:
 *		0 - success
 */

void bm_pool_status_dump(int pool);

int bm_bank0_pool_check(int pool, u32 *pool_base);

int bm_pool_dump(int pool, int mode, u32 *pool_base, int capacity);
/*
 BM sysFS function
*/

int mv_pp3_bm_sysfs_init(struct kobject *dev_kobj);
int mv_pp3_bm_sysfs_exit(struct kobject *dev_kobj);

int mv_pp3_bm_debug_sysfs_init(struct kobject *pp3_bm_kobj);
int mv_pp3_bm_debug_sysfs_exit(struct kobject *pp3_bm_kobj);

#endif /* __mv_bm_h__ */
