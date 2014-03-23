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

#ifndef	__MV_BM_H__
#define	__MV_BM_H__

#include "common/mv_sw_if.h"
#include "common/mv_hw_if.h"

/* Error Codes */
#define BM_WRONG_MEMORY_TYPE           -EINVAL
#define BM_BANK_NOT_IN_RANGE           -EINVAL
#define BM_ATTR_CHANGE_AFTER_BM_ENABLE -EINVAL
#define BM_CHANGE_AFTER_BM_ENABLE      -EINVAL
#define BM_ALIAS_ERROR                 -EINVAL
#define BM_INPUT_NOT_IN_RANGE          -EDOM

/* Input definitions */
#define BM_NUMBER_OF_BANKS_QM			(BM_BANK_QP_MAX - BM_BANK_QP_MIN + 1)
#define BM_NUMBER_OF_BANKS_GP			(BM_BANK_GP_MAX - BM_BANK_GP_MIN + 1)
#define BM_NUMBER_OF_BANKS				(BM_BANK_MAX    - BM_BANK_MIN    + 1)

#define BM_NUMBER_OF_POOLS_QM			(BM_POOL_QM_MAX - BM_POOL_QM_MIN + 1)
#define BM_NUMBER_OF_POOLS_GP			(BM_POOL_GP_MAX - BM_POOL_GP_MIN + 1)
#define BM_NUMBER_OF_POOLS				(BM_POOL_MAX    - BM_POOL_MIN    + 1)

#define BM_NUM_OF_LINE_QM				       512
#define BM_NUM_OF_LINE_GP				      1024

#define GRANULARITY_OF_64_BYTES			        64
#define GRANULARITY_OF_16				        16
/*
#define GRANULARITY_OF_64				        64
*/

#define QM_PE_SIZE_IN_BYTES				         4
#define QM_PE_SIZE_IN_BYTES_IN_DRAM		QM_PE_SIZE_IN_BYTES
#define QM_PE_SIZE_IN_BYTES_IN_CACHE	QM_PE_SIZE_IN_BYTES
#define GP_PE_SIZE_OF_32_BITS_IN_BYTES_IN_DRAM   4
#define GP_PE_SIZE_OF_40_BITS_IN_BYTES_IN_DRAM   8
#define GP_PE_SIZE_IN_BYTES_IN_CACHE	         8
/*
#define PE_SIZE_OF_32_BITS_IN_BYTES		         4
#define PE_SIZE_OF_40_BITS_IN_BYTES		         8
#define GP_PE_SIZE 32
#define PE_SIZE 1 / * for pe = 22 bits or pe = 32 bits * /
#define PE_SIZE 2 / * for pe = 40 bits * /
#define BM_PE_SIZE_IS_4_BYTES			         4
#define BM_PE_SIZE_IS_8_BYTES			         8
#define BM_GRANULARITY_OF_16_PE			(16*PE_SIZE)
*/

#define BM_BUFFER_SIZE_P2				      1024
#define BM_BUFFER_SIZE_P3				        16

/* Default values */
#define BM_PE_SIZE_IS_32_BITS			         1
#define BM_PE_SIZE_IS_40_BITS			         0
#define BM_PE_SIZE_DEF					BM_PE_SIZE_IS_32_BITS	/* 0 - 40 bits, 1 - 32 bits */
#define BM_POOL_PAIR_GP_DEF				         0	/* 0 -   false, 1 -    true */

#define BM_AE_THR_DEF TRUNCATE((num_of_buffers * 1/4), GRANULARITY_OF_16) /*
#define BM_AE_THR_DEF \
	(((num_of_buffers * 1/4) + (GRANULARITY_OF_16-1))&(0xFFFFFFF0))
	Almost empty default threshold is ¼ of num_of_buffers truncated to multiplication of 16,
		otherwise the range is 0 or 16 to num_of_buffers-32 */
#define BM_AF_THR_DEF TRUNCATE((num_of_buffers * 3/4), GRANULARITY_OF_16) /*
#define BM_AF_THR_DEF \
	(((num_of_buffers * 3/4) + (GRANULARITY_OF_16-1))&(0xFFFFFFF0))
	Almost full  default threshold is ¾ of num_of_buffers rounded   to multiplication of 16,
		otherwise the range is 0 or 32 to num_of_buffers-16 */
#define BM_CACHE_VMID_DEF                        0
#define BM_CACHE_ATTR_DEF                        1

#define BM_CACHE_SI_THR_QM_DEF                 448
#define BM_CACHE_SO_THR_QM_DEF                 480
#define BM_CACHE_NUM_OF_BUFFERS_QM_DEF         512

#define BM_CACHE_SI_THR_GP_BIG_DEF             304
#define BM_CACHE_SO_THR_GP_BIG_DEF             336
#define BM_CACHE_NUM_OF_BUFFERS_GP_BIG_DEF     352

#define BM_CACHE_SI_THR_GP_SMALL_DEF            80
#define BM_CACHE_SO_THR_GP_SMALL_DEF            96
#define BM_CACHE_NUM_OF_BUFFERS_GP_SMALL_DEF   112

/* Range Definitions */
#define BM_BANK_QM_MIN				         0
#define BM_BANK_QM_MAX				         0
#define BM_BANK_GP_MIN				         1
#define BM_BANK_GP_MAX				         4
#define BM_BANK_MIN					BM_BANK_QM_MIN
#define BM_BANK_MAX					BM_BANK_GP_MAX

#define BM_POOL_QM_MIN				         0
#define BM_POOL_QM_MAX				         3
#define BM_POOL_GP_MIN				         8
#define BM_POOL_GP_MAX				        35
#define BM_POOL_MIN					BM_POOL_QM_MIN
#define BM_POOL_MAX					BM_POOL_GP_MAX

#define BM_GLOBAL_POOL_IDX_QM_MIN	         0
#define BM_GLOBAL_POOL_IDX_QM_MAX	         3
#define BM_GLOBAL_POOL_IDX_GP_MIN	         4
#define BM_GLOBAL_POOL_IDX_GP_MAX	        31
#define BM_GLOBAL_POOL_IDX_MIN		BM_GLOBAL_POOL_IDX_QM_MIN
#define BM_GLOBAL_POOL_IDX_MAX		BM_GLOBAL_POOL_IDX_GP_MAX

#define BM_PID_LOCAL_QM_MIN			         0
#define BM_PID_LOCAL_QM_MAX			         3
#define BM_PID_LOCAL_GP_MIN			         0
#define BM_PID_LOCAL_GP_MAX			         6
#define BM_PID_LOCAL_MIN			BM_PID_LOCAL_QM_MIN
#define BM_PID_LOCAL_MAX			BM_PID_LOCAL_GP_MAX

#define BM_ACACHE_MIN				0x00000020	/*  32 */
#define BM_ACACHE_MAX				0x0000007F	/* 127 */
#define BM_ADOMAIN_MIN				         0
#define BM_ADOMAIN_MAX				0x00000003	/*   3 */
#define BM_AQOS_MIN					         0
#define BM_AQOS_MAX					0x00000003	/*   3 */

/* #define BM_BUFFERS_MIN				0x00000030 */	/*  48 */
#define BM_NUM_OF_BUFFERS_QM_MIN			0x00000030	/*  48 */
#define BM_NUM_OF_BUFFERS_QM_GPM_MAX		(0x00001400 - 16)	/* 5120 - 16 for P0 & P1 */
#define BM_NUM_OF_BUFFERS_QM_DRAM_MAX		(0x00400000 - 16)	/*   4M - 16 for P2 & P3 */
#define BM_NUM_OF_BUFFERS_QM_MAX \
			MV_MAX(BM_NUM_OF_BUFFERS_QM_GPM_MAX, BM_NUM_OF_BUFFERS_QM_DRAM_MAX)
#define BM_NUM_OF_BUFFERS_GP_MIN			0x00000030	/*  48 */
#define BM_NUM_OF_BUFFERS_GP_MAX			(0x00200000 - 16)	/*  2M - 16 */
/* #define BM_NUM_OF_BUFFERS_MAX				MV_MAX(BM_BUFFERS_QM_MAX, BM_BUFFERS_GP_MAX) */
#define BM_CACHE_NUM_OF_BUFFERS_MIN			0x00000030	/*  48 */
#define BM_CACHE_NUM_OF_BUFFERS_QM_MIN		BM_CACHE_NUM_OF_BUFFERS_MIN
#define BM_CACHE_NUM_OF_BUFFERS_QM_MAX		(0x00000800 - (4-1)*BM_CACHE_NUM_OF_BUFFERS_QM_MIN) /*
	the sum for bank 0 is up to 2048, then for one pool it is 2048-3*48   */
#define BM_CACHE_NUM_OF_BUFFERS_GP_MIN		BM_CACHE_NUM_OF_BUFFERS_MIN
#define BM_CACHE_NUM_OF_BUFFERS_GP_MAX		(0x00000400 - 8)	/* 1024-8 */
/* #define BM_CACHE_BUFFERS_MAX		MV_MAX(BM_CACHE_BUFFERS_QM_MAX, BM_CACHE_BUFFERS_GP_MAX) */
#define BM_FILL_LEVEL_MIN			         0
#define BM_FILL_LEVEL_MAX			num_of_buffers
#define BM_ADDRESS_MIN				         0
#define BM_ADDRESS_MAX				0xFFFFFFFF
#define BM_QUICK_INIT_MIN			         0
#define BM_QUICK_INIT_MAX			0x00000001
#define BM_POOL_PAIR_MIN			         0
#define BM_POOL_PAIR_MAX			0x00000001
#define BM_PE_SIZE_MIN				0x00000001
#define BM_PE_SIZE_MAX				0x00000001
#define BM_VMID_MIN					         0
#define BM_VMID_MAX					0x0000003F	/*  63 */
#define BM_CACHE_VMID_MIN			         0
#define BM_CACHE_VMID_MAX			0x0000003F	/*  63 */
#define BM_CACHE_ATTR_MIN			         0
#define BM_CACHE_ATTR_MAX			0x000000FF	/* 255 */
#define BM_AE_THR_MIN				MV_MIN(0x00000010, num_of_buffers) /*
	16                unless number of buffers is 0 and then it is also 0 */
#define BM_AE_THR_MAX				MV_MAX((num_of_buffers - 0x00000020), 0) /*
	num_of_buffers-32 unless number of buffers is 0 and then it is also 0 */
#define BM_AF_THR_MIN				MV_MIN(0x00000020, num_of_buffers) /*
	32                unless number of buffers is 0 and then it is also 0 */
#define BM_AF_THR_MAX				MV_MAX((num_of_buffers - 0x00000010), 0) /*
	num_of_buffers-16 unless number of buffers is 0 and then it is also 0 */
#define BM_CACHE_SI_THR_MIN			0x00000010	/*  16 */
#define BM_CACHE_SI_THR_MAX			(cache_num_of_buffers - 0x00000010)	/* cache_num_of_buffers - 16 */
#define BM_CACHE_SO_THR_MIN			0x00000018	/*  24 */
#define BM_CACHE_SO_THR_MAX			(cache_num_of_buffers - 0x00000008)	/* cache_num_of_buffers -  8 */
#define BM_CACHE_END_MIN			0x00000020	/*  32 */
#define BM_CACHE_END_MAX			0x0000007F	/* 127 */

#define BM_START_MIN				         0
#define BM_START_MAX				0xFFFFFFFF
#define BM_OFFSET_MIN				         0
#define BM_OFFSET_MAX				0xFFFFFFFF
#define BM_DATA_SIZE_MIN			         0
#define BM_DATA_SIZE_MAX			0xFFFFFFFF
#define BM_DRAM_ADDRESS_LO_MIN		         0
#define BM_DRAM_ADDRESS_LO_MAX		0xFFFFFFFF
#define BM_DRAM_ADDRESS_HI_MIN		         0
#define BM_DRAM_ADDRESS_HI_MAX		0xFFFFFFFF

#define BM_DATA_PTR_MIN				         0
#define BM_DATA_PTR_MAX				0xFFFFFFFF

/*
#define BM_QM_BUFFERS_MIN			48
#define BM_GP_BUFFERS_MIN			48 (but can also be 0)

#define BM_AE_THR_QM_MIN			16
#define BM_AE_THR_GP_MIN			16 (unless number of buffers is 0 and then it is also 0)
#define BM_AE_THR_QM_MAX			(num_of_buffers-32)
#define BM_AE_THR_GP_MAX			(num_of_buffers-32) (unless number of buffers is
										0 and then it is also 0)
#define BM_AF_THR_QM_MIN			32
#define BM_AF_THR_GP_MIN			32 (unless number of buffers is 0 and then it is 0)
#define BM_AF_THR_QM_MAX			(num_of_buffers-16)
#define BM_AF_THR_GP_MAX			(num_of_buffers-16)	(unless number of buffers is
										0 and then it is also 0)

#define BM_QM_BUFFERS_CACHE_MIN		32
#define BM_GP_BUFFERS_CACHE_MIN		32
#define BM_GP_BUFFERS_CACHE_MAX		1024 (if there is one pool then it is (1024-8)=1016

#define BM_QM_SI_THR_MIN			8
#define BM_GP_SI_THR_MIN			8
#define BM_QM_SI_THR_MAX			cache_num_of_buffers - 16
#define BM_GP_SI_THR_MAX			cache_num_of_buffers - 16
#define BM_QM_SO_THR_MIN			24
#define BM_GP_SO_THR_MIN			24
#define BM_QM_SO_THR_MAX			cache_num_of_buffers - 8
#define BM_GP_SO_THR_MAX			cache_num_of_buffers - 8

#define BM_END_MIN					32
*/

/*
#define BM_PID_TO_BANK(_pid)                \
{									        \
	_bid =                                  \
	(((_pid >= 0) && (_pid   <=  7)) ?	0 :	\
	(((_pid >= 8) && (_pid%4 ==  0)) ?	1 :	\
	(((_pid >= 8) && (_pid%4 ==  1)) ?	2 :	\
	(((_pid >= 8) && (_pid%4 ==  2)) ?	3 :	\
	(((_pid >= 8) && (_pid%4 ==  3)) ?	4 :	\
	-1)))))                                 \
	if ((_bid >= 0) && (_bid <= 4))         \
		return _bid;                        \
	else                                    \
		return EDOM;                        \
}
*/

#define BM_PID_TO_BANK(_pid)                \
	(((_pid >= 0) && (_pid   <=  3)) ?	0 :	\
	(((_pid >= 8) && (_pid%4 ==  0)) ?	1 :	\
	(((_pid >= 8) && (_pid%4 ==  1)) ?	2 :	\
	(((_pid >= 8) && (_pid%4 ==  2)) ?	3 :	\
	(((_pid >= 8) && (_pid%4 ==  3)) ?	4 :	\
	-1)))))

#define BM_PID_TO_PID_LOCAL(_pid)                    \
	(((_pid >= 0) && (_pid   <=  3)) ?  _pid       : \
	(((_pid >= 8) && (_pid   <= 35)) ? (_pid-8)>>2 : \
	-1))

#define BM_PID_TO_GLOBAL_POOL_IDX(_pid)              \
	(((_pid >= 0) && (_pid   <=  3)) ?  _pid       : \
	(((_pid >= 8) && (_pid   <= 35)) ? (_pid-4)    : \
	-1))

#define BM_GLOBAL_POOL_IDX_TO_PID(_pid)              \
	(((_pid >= 0) && (_pid   <=  3)) ?  _pid       : \
	(((_pid >= 4) && (_pid   <= 31)) ? (_pid+4)    : \
	-1))

/*
#define BM_MAGIC 0x24051974

#define  DECLARE_BM_CTL_PTR(name, value)	{struct  bm_ctl *name = (struct  bm_ctl *)value; }

#define CHECK_BM_CTL_PTR(ptr)		\
{									\
	if (!ptr)						\
		return -EINVAL;				\
	if (ptr->magic !=  BM_MAGIC)	\
		return -EBADF;				\
}

#define  BM_CTL(name, handle)    {DECLARE_BM_CTL_PTR(name, handle);  CHECK_BM_CTL_PTR(name); }
#define  BM_ENV(var) (var->hEnv)
*/

#define TRUNCATE(_truncated_value, _truncating_value) (_truncated_value - (_truncated_value % _truncating_value))

#define BM_QM_PE_UNITS_TO_BYTES(_num_of_buffers) (_num_of_buffers * 4)
#define BM_GP_PE_UNITS_TO_BYTES(_num_of_buffers, _pe_size)	\
				(((_pe_size) == (40)) ? (_num_of_buffers * 8) : (_num_of_buffers * 4))
/*
#define BM_GP_POOL_DRAM_SIZE_IN_BYTES(_num_of_buffers, _pe_size)
	if (_pe_size==32) (_num_of_buffers * 4)
	if (_pe_size==40) (_num_of_buffers * 8)
#if (_pe_size==32) (_num_of_buffers * 4)
#if (_pe_size==40) (_num_of_buffers * 8)
#define MIN(a,b) (((a)<(b))?(a):(b))

#define CHECK_BM_CTL_PTR(ptr)		\
{									\
	if (!ptr)						\
		return -EINVAL;				\
	if (ptr->magic !=  BM_MAGIC)	\
		return -EBADF;				\
}
*/

/*
typedef void * bm_handle;
*/

/**
 *  Initialize BM module
 *  Return values:
 *		0 - success
 */
int bm_open(void);

/**
 *  Global functions, configures BM attributes for read/write in DRAM
 *  configures all 12 attributes with default values
 *  Return values:
 *		0 - success
 */
int bm_attr_all_pools_def_set(void);

/**
 *  Global functions, configures BM attributes for read/write in DRAM
 *  configures attributes for 4 pools of QM
 *  Return values:
 *		0 - success
 */
int bm_attr_qm_pool_set(u32 arDomain, u32 awDomain, u32 arCache, u32 awCache, u32 arQOS, u32 awQOS);

/**
 *  Global functions, configures BM attributes for read/write in DRAM
 *  configures attributes for general purpose pools (8-35)
 *  Return values:
 *		0 - success
 */
int bm_attr_gp_pool_set(u32 arDomain, u32 awDomain, u32 arCache, u32 awCache, u32 arQOS, u32 awQOS);

/**
 *  Get BM enable status
 *
 *  Return values:
 *		0 - success
 */
int bm_enable_status_get(u32 *bm_req_rcv_en);

/**
 *  Initiates of GPM pools with default values
 *
 *  Return values:
 *		0 - success
 */
int bm_qm_gpm_pools_def_quick_init(u32 num_of_buffers, u32 *qece_base_address, u32 *pl_base_address);

/**
 *  Initiates of DRAM pools with default values
 *
 *  Return values:
 *		0 - success
 */
int bm_qm_dram_pools_def_quick_init(u32 num_of_buffers, u32 *qece_base_address, u32 *pl_base_address);

/**
 *  Initiates QM GPM pools
 *	Configures BM for pool initialization and enables the pool.  Doesn't configure cache parameters.
 *  This function is a super set of several bm function that are listed below
 *  Note: No change can be made to pool after this function is called.
 *  Return values:
 *		0 - success
 */
int bm_qm_gpm_pools_quick_init(u32 num_of_buffers, u32 *qece_base_address,
				u32 *pl_base_address, u32 ae_thr, u32 af_thr,
				u32 cache_vmid, u32 cache_attr, u32 cache_so_thr, u32 cache_si_thr,
				u32 cache_num_of_buffers);

/**
 *  Initiates QM DRAM pools
 *	Configures BM for pool initialization and enables the pool.  Doesn't configure cache parameters.
 *  This function is a super set of several bm function that are listed below
 *  Note: No change can be made to pool after this function is called.
 *  Return values:
 *		0 - success
 */
int bm_qm_dram_pools_quick_init(u32 num_of_buffers, u32 *qece_base_address,
					u32 *pl_base_address, u32 ae_thr, u32 af_thr,
					u32 cache_vmid, u32 cache_attr, u32 cache_so_thr, u32 cache_si_thr,
					u32 cache_num_of_buffers);

/**
 *  Get pool quick init status - to get indication if quick init is completed
 *  and client can start allocate/release from pool
 *  Return values:
 *		0 - success
 */
int bm_pool_quick_init_status_get(
		u32 pool, /* all pools, QM and General purpose. Range 0 to 3 and 8 to 35 */
		u32 *completed); /*	1 - quick init completed,
							0 - still in quick init process */

/**
 *  Basic initialization of general purpose pools with default values
 *  Return values:
 *		0 - success
 */
int bm_gp_pool_def_basic_init(
							u32 pool, /* pool number: general purpose pools 8 to 35 */
							u32 num_of_buffers, /* equal or less
								than num_of_buffers passed when initializing pool */
							u32 *base_address,  /* DRAM base address */
							u32 partition_model);  /* for small partition in cache */

/**
 *  Basic initialization of general purpose pools
 *  Return values:
 *		0 - success
 */
int bm_gp_pool_basic_init(
							u32 pool, /* pool number: general purpose pools 8 to 35 */
							u32 num_of_buffers, /* equal or less
								than num_of_buffers passed when initializing pool */
							u32 *base_address,  /* DRAM base address */
							u32 pe_size, /* PE size can be either 32bits or 40 bits */
							u32 pool_pair, /* Pool_pair is
								either 0 for false or 1 for true */
							u32 ae_thr, /* almost empty threshold for pool */
							u32 af_thr, /* almost full threshold for pool */
							u32 cache_vmid, /* cache_vmid */
							u32 cache_attr, /* cache_attr */
							u32 cache_so_thr, /* cache_so_thr */
							u32 cache_si_thr, /* cache_si_thr */
							u32 cache_num_of_buffers); /* cache_num_of_buffers */

/**
 *  Global enable for BM
 *
 *  Return values:
 *		0 - success
 */
int bm_enable(void);

/**
 *  Global disable for BM
 *
 *  Return values:
 *		0 - success
 */
int bm_disable(void);


/**
 *  gives fill level of pool in DRAM
 *	Return values:
 *		0 - success
 */
int bm_pool_fill_level_get(
					u32 pool, /* pool number: any pool QM and general purpose */
					u32 *fill_level);  /* fill level */

/**
 *  Set BM VMID
 *
 *  Return values:
 *		0 - success
 */
int bm_vmid_set(u32 bm_vmid);

/**
 *  Configure BM registers and allocate memory for pools with default values
 *  Return values:
 *		0 - success
 */
int bm_gp_pool_def_quick_init(u32 pool, u32 num_of_buffers, u32 fill_level,
							u32 *base_address, u32 partition_model);

/**
 *  Configure BM registers and allocate memory for pools
 *  Return values:
 *		0 - success
 */
int bm_gp_pool_quick_init(u32 pool, u32 num_of_buffers, u32 fill_level, u32 *base_address,
					u32 pe_size, u32 pool_pair, u32 ae_thr, u32 af_thr,
					u32 cache_vmid, u32 cache_attr,	u32 cache_so_thr, u32 cache_si_thr,
					u32 cache_num_of_buffers);


/*BM Debug functions*/

/**
 *  Print all global registers
 *  Return values:
 *		0 - success
 */
int bm_global_registers_dump(void);

/**
 *  Print values of all BM pool registers
 *  Return values:
 *		0 - success
 */
int bm_pool_registers_dump(u32 pool);

/**
 *  Print values of all BM bank registers
 *  Return values:
 *		0 - success
 */
int bm_bank_registers_dump(u32 bank);

/**
 *  Print all 512 lines of cache per input bank sram_b0...b4_cache_mem
 *  Return values:
 *		0 - success
 */
int bm_cache_memory_dump(u32 bank);

/**
 *  Read BM idle status
 *
 *  Return values:
 *		0 - success
 */
int bm_idle_status_get(u32 *status);

/**
 *  Get status per pool: almost full, almost empty and pool in cache is not empty
 *
 *  Return values:
 *		0 - success
 */
int bm_pool_status_get(u32 pool, u32 *pool_nempty, u32 *dpool_ae, u32 *dpool_af);

/**
 *  Print several registers status that gives indication why BM is busy
 *  This funciton is useful to call if BM is not reaching idle after a long time.
 *
 *  Return values:
 *		0 - success
 */
int bm_idle_debug(void);

/**
 *  Check error bits If set to 1, and print the error that occurred.
 *  Bit 0 is always an OR of all the other errors.
 *  So bit 0 with value 0 indicates there are no errors.
 *
 *  Return values:
 *		0 - success
 */
int bm_error_dump(void);

/*BM Internal functions*/
/**
 *  Fill memory of pool with PE index
 *	PE index is incrementing value of 1 to num_of_buffers
 *  Mark PE with its location (GPM or DRAM)
 *  Return values:
 *		0 - success
 */
int bm_pool_memory_fill(
						u32 pool, /* pool number: 0 to 3 */
						u32 num_of_buffers, /* equal or less
						than num_of_buffers passed when initializing pool */
						u32 *base_address); /* pool base address.
						same as the address passed when initializing pool */

/**
 *  Configure BM with Fill level of pool in DRAM
 *  Note: Must be called before pool is enabled
 *  Return values:
 *		0 - success
 */
int bm_pool_dram_set(
					u32 pool, /* pool number: any pool QM and general purpose */
					u32 num_of_buffers,  /* number of buffers/PEs in pool*/
					u32 pe_size, /* PE size can be either 32bits or 40 bits */
					u32 *base_address, /* DRAM base address */
					u32 ae_thr, /* almost empty threshold for pool */
					u32 af_thr); /* almost full threshold for pool */

/**
 *  Configure BM with Fill level of pool in DRAM
 *	Supports all pools (QM and general purpose)
 *  Note: Must be called before pool is enabled
 *  Return values:
 *		0 - success
 */
int bm_pool_fill_level_set(
					u32 pool, /* pool number: any pool QM and general purpose */
					u32 num_of_buffers, /* number of buffers/PEs in pool */
					u32 pe_size, /* PE size can be either 32bits or 40 bits */
					u32 quick_init);  /* configures fill level relevant
									  only when pool is in quick init mode */

/**
 *  Enables BM pool
 *
 *  Note: Must be called after all other pool related configuration is complete
 *  Return values:
 *		0 - success
 */
int bm_pool_enable(
					u32 pool, /* pool number: any pool QM and general purpose */
					u32 quick_init); /* 1- enable quick init of pool,
										0 - disable quick init of pool */

/**
 *  Set PE pointer size in general purpose pool
 *
 *  Return values:
 *		0 - success
 */
int bm_gp_pool_pe_size_set(
							u32 pool, /* pool number:
						general purpose pools range: 8 to 35 */
							u32 pe_size); /* PE size can be either 32bits or 40 bits */

/**
 *  Configure if pool is defined to work in pairs
 *
 *  Return values:
 *		0 - success
 */
int bm_gp_pool_pair_set(
					u32 pool, /* pool number: general purpose pools range: 8 to 35 */
					u32 pool_pair); /* Pool_pair is either 0 for false or 1 for true */

/**
 *  Configure pool cache parameters
 *
 *  Note: Must be called before pool is enabled
 *  Return values:
 *		0 - success
 */
int bm_pool_cache_set(
						u32 pool, /* pool number: any pool QM and general purpose.
								  range 0 to 3 and 8 to 35 */
						u32 cache_vmid,
						u32 cache_attr,
						u32 cache_so_thr,
						u32 cache_si_thr,
						u32 cache_num_of_buffers);

/**
 *  Set Pool to disable - TBD
 *
 *  Return values:
 *		0 - success
 */
int bm_pool_disable(u32 pool);

/**
 * access_addr - absolute address: Silicon base + unit base + register offset
 * return register value
 */
/*static INLINE
int mv_pp3_hw_reg_read(u32 access_addr);
*/
/**
 * access_addr - absolute address: Silicon base + unit base + register offset
 * write data to register
 */
/*static INLINE
int mv_pp3_hw_reg_write(u32 access_addr, u32 data);
*/
/**
 *  Read register from BM units
 *
 *  Return values:
 *		0 - success
 */
int bm_register_read(
					u32 base_address, /* register address as appears in CIDER */
					u32 offset,       /* offset from this address */
					u32 wordsNumber,  /* how many words (32bits) to read */
					u32 *dataPtr);    /* returning data. It is the user responsibility
								that dataPtr points to wordsNumber words */

/**
 *  Write register in BM units
 *
 *  Return values:
 *		0 - success
 */
int bm_register_write(
					u32 base_address, /* register address as appears in CIDER */
					u32 offset,       /* offset from this address */
					u32 wordsNumber,  /* how many words (32bits) to read */
					u32 *dataPtr);    /* Data to write. It is the user responsibility
									that dataPtr points to wordsNumber words */


/**
 *  Print per pool several registers which are helpful for advanced debugging
 *
 *  Return values:
 *		0 - success
int bm_per_pool_advanced_debug(u32 pool, u32 line);
 */

/**
 *  Print TBD
 *
 *  Return values:
 *		0 - success
int bm_debug(void);
 */

#endif /* MV_BM_H */
