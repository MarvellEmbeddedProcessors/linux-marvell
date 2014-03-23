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
LOSS OF USE, DATA, OR PROFITS;OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

*******************************************************************************/

#ifndef	__MV_QM_H__
#define	__MV_QM_H__

#include "common/mv_sw_if.h"
#include "common/mv_hw_if.h"

/* Error Codes */
#define QM_WRONG_MEMORY_TYPE           -EINVAL
#define QM_ALIAS_ERROR                 -EINVAL
#define QM_INPUT_NOT_IN_RANGE          -EINVAL

/* Input definitions*/
#define GPM_MEMORY_TYPE				 0
#define DRAM_MEMORY_TYPE			 1
#define GRANULARITY_OF_16_BYTES		16

#define QM_QUEUE_PROFILE_INVALID			0x00000000	/*    0 */
#define QM_QUEUE_PROFILE_0					0x00000001	/*    1 */
#define QM_QUEUE_PROFILE_1					0x00000002	/*    2 */
#define QM_QUEUE_PROFILE_2					0x00000003	/*    3 */
#define QM_QUEUE_PROFILE_3					0x00000004	/*    4 */
#define QM_QUEUE_PROFILE_4					0x00000005	/*    5 */
#define QM_QUEUE_PROFILE_5					0x00000006	/*    6 */
#define QM_QUEUE_PROFILE_6					0x00000007	/*    7 */


/* Default values */
#define QM_THR_HI_DEF			0x0000000C	/*  12 */
#define QM_THR_LO_DEF			0x00000018	/*  24 */
#define QM_GPM_QE_THR_HI_DEF	QM_THR_HI_DEF
#define QM_GPM_QE_THR_LO_DEF	QM_THR_LO_DEF
#define QM_GPM_PL_THR_HI_DEF	QM_THR_HI_DEF
#define QM_GPM_PL_THR_LO_DEF	QM_THR_LO_DEF
#define QM_DRAM_QE_THR_HI_DEF	QM_THR_HI_DEF
#define QM_DRAM_QE_THR_LO_DEF	QM_THR_LO_DEF
#define QM_DRAM_PL_THR_HI_DEF	QM_THR_HI_DEF
#define QM_DRAM_PL_THR_LO_DEF	QM_THR_LO_DEF

#define QM_POOL0_SID_NUM_DEF	QM_POOL0_SID_NUM_MAX
#define QM_POOL1_SID_NUM_DEF	QM_POOL1_SID_NUM_MIN

#define QM_CLASS_ARR_CMAC_EMAC_DEF	input_port	/*    0 */
#define QM_CLASS_ARR_HMAC_DEF		0x00000008	/*    8 */
#define QM_CLASS_ARR_PPC_DEF		0x00000009	/*    9 */
#define QM_PORT_ARR_DEF				         0	/*    0 */
#define QM_ARRAYS_SIZE_DEF			0x0000005A	/*   90 */

#define QM_PORT_DEPTH_ARR_PPC0_DEF		    (2 * QM_SIZE_OF_PORT_DEPTH_ARR_PPC_IN_BYTES)	/* 2*144B */
#define QM_PORT_DEPTH_ARR_PPC1_DEF		    (1 * QM_SIZE_OF_PORT_DEPTH_ARR_PPC_IN_BYTES)	/* 1*144B */
#define QM_PORT_DEPTH_ARR_EMAC_DEF		  (160 * QM_SIZE_OF_PORT_DEPTH_ARR_MAC_IN_BYTES)	/*  2560B */
#define QM_PORT_DEPTH_ARR_CMAC0_DEF		  (160 * QM_SIZE_OF_PORT_DEPTH_ARR_MAC_IN_BYTES)	/*  2560B */
#define QM_PORT_DEPTH_ARR_CMAC1_DEF		   (32 * QM_SIZE_OF_PORT_DEPTH_ARR_MAC_IN_BYTES)	/*   512B */
#define QM_PORT_DEPTH_ARR_HMAC_DEF		   (32 * QM_SIZE_OF_PORT_DEPTH_ARR_MAC_IN_BYTES)	/*   512B */

#define QM_PORT_CREDIT_THR_ARR_EMAC_DEF		  (152 * QM_SIZE_OF_PORT_DEPTH_ARR_MAC_IN_BYTES)	/*  2432B */
#define QM_PORT_CREDIT_THR_ARR_CMAC0_DEF	  (152 * QM_SIZE_OF_PORT_DEPTH_ARR_MAC_IN_BYTES)	/*  2432B */
#define QM_PORT_CREDIT_THR_ARR_CMAC1_DEF	   (24 * QM_SIZE_OF_PORT_DEPTH_ARR_MAC_IN_BYTES)	/*   384B */
#define QM_PORT_CREDIT_THR_ARR_HMAC_DEF		   (24 * QM_SIZE_OF_PORT_DEPTH_ARR_MAC_IN_BYTES)	/*   384B */

#define QM_PORT_PPC_ARR_PPC0_DEF	    1	/* packets are processed by data        PPC */
#define QM_PORT_PPC_ARR_PPC1_DEF	    2	/* packets are processed by maintenance PPC */

#define QM_SWF_AWQOS_DEF			0x00000001	/*    1 */
#define QM_RDMA_AWQOS_DEF			0x00000001	/*    1 */
#define QM_HWF_QE_CE_AWQOS_DEF		0x00000001	/*    1 */
#define QM_HWF_SFH_PL_AWQOS_DEF		0x00000001	/*    1 */

#define QM_SWF_AWCACHE_DEF			0x0000000B	/*   11 */
#define QM_RDMA_AWCACHE_DEF			0x0000000B	/*   11 */
#define QM_HWF_QE_CE_AWCACHE_DEF	0x00000003	/*    3 */
#define QM_HWF_SFH_PL_AWCACHE_DEF	0x00000003	/*    3 */

#define QM_SWF_AWDOMAIN_DEF			0x00000002	/*    2 */
#define QM_RDMA_AWDOMAIN_DEF		0x00000002	/*    2 */
#define QM_HWF_QE_CE_AWDOMAIN_DEF	         0	/*    0 */
#define QM_HWF_SFH_PL_AWDOMAIN_DEF	         0	/*    0 */

#define QM_SWF_ARQOS_DEF			0x00000001	/*    1 */
#define QM_RDMA_ARQOS_DEF			0x00000001	/*    1 */
#define QM_HWF_QE_CE_ARQOS_DEF		0x00000001	/*    1 */
#define QM_HWF_SFH_PL_ARQOS_DEF		0x00000001	/*    1 */

#define QM_SWF_ARCACHE_DEF			0x0000000B	/*   11 */
#define QM_RDMA_ARCACHE_DEF			0x0000000B	/*   11 */
#define QM_HWF_QE_CE_ARCACHE_DEF	0x00000003	/*    3 */
#define QM_HWF_SFH_PL_ARCACHE_DEF	0x00000003	/*    3 */

#define QM_SWF_ARDOMAIN_DEF			0x00000002	/*    2 */
#define QM_RDMA_ARDOMAIN_DEF		0x00000002	/*    2 */
#define QM_HWF_QE_CE_ARDOMAIN_DEF	         0	/*    0 */
#define QM_HWF_SFH_PL_ARDOMAIN_DEF	         0	/*    0 */

/* Range Definitions */
#define QM_QUEUE_MIN			         0	/*    0 */
#define QM_QUEUE_MAX			0x00000200	/*  512 */
#define QM_PORT_MIN				         0	/*    0 */
#define QM_PORT_MAX				0x0000000F	/*   15 */
#define QM_PORT_PPC_MIN			QM_PORT_MIN
#define QM_PORT_PPC_MAX			0x00000002	/*    2 */
#define QM_PORT_MAC_MIN			0x00000003	/*    3 */
#define QM_PORT_MAC_MAX			0x0000000A	/*   10 */
#define QM_QUEUE_PROFILE_MIN	         1	/*    1 */
#define QM_QUEUE_PROFILE_MAX	0x00000007	/*    7 */

#define QM_PPC_MIN				         0
#define QM_PPC_MAX				0x7FFFFFFF
#define QM_BASE_MIN				         0
#define QM_BASE_MAX				0x7FFFFFFF
#define QM_SIZE_MIN				         0
#define QM_SIZE_MAX				0x7FFFFFFF

#define QM_VMID_MIN				         0	/*   0 */
#define QM_VMID_MAX				0x0000003F	/*  63 */
#define QM_THR_MIN				         0	/*   0 */
#define QM_THR_MAX				0x00000020	/*  32 */
#define QM_GPM_QE_THR_HI_MIN	QM_THR_MIN
#define QM_GPM_QE_THR_HI_MAX	QM_THR_MAX
#define QM_GPM_QE_THR_LO_MIN	QM_THR_MIN
#define QM_GPM_QE_THR_LO_MAX	QM_THR_MAX
#define QM_GPM_PL_THR_HI_MIN	QM_THR_MIN
#define QM_GPM_PL_THR_HI_MAX	QM_THR_MAX
#define QM_GPM_PL_THR_LO_MIN	QM_THR_MIN
#define QM_GPM_PL_THR_LO_MAX	QM_THR_MAX
#define QM_DRAM_QE_THR_HI_MIN	QM_THR_MIN
#define QM_DRAM_QE_THR_HI_MAX	QM_THR_MAX
#define QM_DRAM_QE_THR_LO_MIN	QM_THR_MIN
#define QM_DRAM_QE_THR_LO_MAX	QM_THR_MAX
#define QM_DRAM_PL_THR_HI_MIN	QM_THR_MIN
#define QM_DRAM_PL_THR_HI_MAX	QM_THR_MAX
#define QM_DRAM_PL_THR_LO_MIN	QM_THR_MIN
#define QM_DRAM_PL_THR_LO_MAX	QM_THR_MAX
#define QM_POOL_SID_NUM_MIN		         0
#define QM_POOL_SID_NUM_MAX		0x00001000	/* 4096 */

#define QM_POOL0_SID_NUM_MIN	QM_POOL_SID_NUM_MIN
#define QM_POOL0_SID_NUM_MAX	QM_POOL_SID_NUM_MAX
#define QM_POOL1_SID_NUM_MIN	QM_POOL_SID_NUM_MIN
#define QM_POOL1_SID_NUM_MAX	QM_POOL_SID_NUM_MAX

#define QM_CLASS_ARR_MIN		         0	/*    0 */
#define QM_CLASS_ARR_MAX		0x00000009	/*    9 */
#define QM_PORT_ARR_MIN			         0	/*    0 */
#define QM_PORT_ARR_MAX			0x00000001	/*    1 */
#define QM_ARRAYS_SIZE_MIN		0x0000005A	/*   90 */
#define QM_ARRAYS_SIZE_MAX		0x00000120	/*  288 */

#define QM_INPUT_PORT_CMAC_EMAC_MIN	         0	/*    0 */
#define QM_INPUT_PORT_CMAC_EMAC_MAX	0x00000007	/*    7 */
#define QM_INPUT_PORT_HMAC_MIN		0x00000008	/*    8 */
#define QM_INPUT_PORT_HMAC_MAX		0x00000047	/*   71 */
#define QM_INPUT_PORT_PPC_MIN		0x00000048	/*   72 */
#define QM_INPUT_PORT_PPC_MAX		0x00000059	/*   89 */

#define QM_MEMORY_TYPE_MIN		         0
#define QM_MEMORY_TYPE_MAX		         1

#define QM_SIZE_OF_PORT_DEPTH_ARR_PPC_IN_BYTES		0x00000090	/*  144 */
#define QM_SIZE_OF_PORT_DEPTH_ARR_MAC_IN_BYTES		0x00000010	/*   16 */
#define QM_SIZE_OF_PORT_CREDIT_THR_ARR_MAC_IN_BYTES	0x00000010	/*   16 */

#define QM_PORT_DEPTH_ARR_MIN			     0	/*     0 */
#define QM_PORT_DEPTH_ARR_SUM_MAX	0x00004000	/* 16384 */

#define QM_PORT_CREDIT_THR_ARR_MIN	GRANULARITY_OF_16_BYTES	/*     16 */
#define QM_PORT_CREDIT_THR_ARR_MAX	(data_fifo_depth_p - 8 * GRANULARITY_OF_16_BYTES)	/* 8 * 16 */

#define QM_SWF_ARDOMAIN_MIN				         0	/*    0 */
#define QM_SWF_ARDOMAIN_MAX				0x00000003	/*    3 */
#define QM_SWF_ARCACHE_MIN				         0	/*    0 */
#define QM_SWF_ARCACHE_MAX				0x0000000F	/*   15 */
#define QM_SWF_ARQOS_MIN					     0	/*    0 */
#define QM_SWF_ARQOS_MAX				0x00000003	/*    3 */

#define QM_RDMA_ARDOMAIN_MIN			         0	/*    0 */
#define QM_RDMA_ARDOMAIN_MAX			0x00000003	/*    3 */
#define QM_RDMA_ARCACHE_MIN				         0	/*    0 */
#define QM_RDMA_ARCACHE_MAX				0x0000000F	/*   15 */
#define QM_RDMA_ARQOS_MIN					     0	/*    0 */
#define QM_RDMA_ARQOS_MAX				0x00000003	/*    3 */

#define QM_HWF_QE_CE_ARDOMAIN_MIN		         0	/*    0 */
#define QM_HWF_QE_CE_ARDOMAIN_MAX		0x00000003	/*    3 */
#define QM_HWF_QE_CE_ARCACHE_MIN		         0	/*    0 */
#define QM_HWF_QE_CE_ARCACHE_MAX		0x0000000F	/*   15 */
#define QM_HWF_QE_CE_ARQOS_MIN				     0	/*    0 */
#define QM_HWF_QE_CE_ARQOS_MAX			0x00000003	/*    3 */

#define QM_HWF_SFH_PL_ARQOS_MIN			         0	/*    0 */
#define QM_HWF_SFH_PL_ARQOS_MAX			0x00000003	/*    3 */
#define QM_HWF_SFH_PL_ARCACHE_MIN		         0	/*    0 */
#define QM_HWF_SFH_PL_ARCACHE_MAX		0x0000000F	/*   15 */
#define QM_HWF_SFH_PL_ARDOMAIN_MIN			     0	/*    0 */
#define QM_HWF_SFH_PL_ARDOMAIN_MAX		0x00000003	/*    3 */

#define QM_SWF_AWDOMAIN_MIN				         0	/*    0 */
#define QM_SWF_AWDOMAIN_MAX				0x00000003	/*    3 */
#define QM_SWF_AWCACHE_MIN				         0	/*    0 */
#define QM_SWF_AWCACHE_MAX				0x0000000F	/*   15 */
#define QM_SWF_AWQOS_MIN					     0	/*    0 */
#define QM_SWF_AWQOS_MAX				0x00000003	/*    3 */

#define QM_RDMA_AWDOMAIN_MIN			         0	/*    0 */
#define QM_RDMA_AWDOMAIN_MAX			0x00000003	/*    3 */
#define QM_RDMA_AWCACHE_MIN				         0	/*    0 */
#define QM_RDMA_AWCACHE_MAX				0x0000000F	/*   15 */
#define QM_RDMA_AWQOS_MIN					     0	/*    0 */
#define QM_RDMA_AWQOS_MAX				0x00000003	/*    3 */

#define QM_HWF_QE_CE_AWDOMAIN_MIN		         0	/*    0 */
#define QM_HWF_QE_CE_AWDOMAIN_MAX		0x00000003	/*    3 */
#define QM_HWF_QE_CE_AWCACHE_MIN		         0	/*    0 */
#define QM_HWF_QE_CE_AWCACHE_MAX		0x0000000F	/*   15 */
#define QM_HWF_QE_CE_AWQOS_MIN				     0	/*    0 */
#define QM_HWF_QE_CE_AWQOS_MAX			0x00000003	/*    3 */

#define QM_HWF_SFH_PL_AWQOS_MIN			         0	/*    0 */
#define QM_HWF_SFH_PL_AWQOS_MAX			0x00000003	/*    3 */
#define QM_HWF_SFH_PL_AWCACHE_MIN		         0	/*    0 */
#define QM_HWF_SFH_PL_AWCACHE_MAX		0x0000000F	/*   15 */
#define QM_HWF_SFH_PL_AWDOMAIN_MIN			     0	/*    0 */
#define QM_HWF_SFH_PL_AWDOMAIN_MAX		0x00000003	/*    3 */

#define QM_LOW_THRESHOLD_MIN				     0
#define QM_LOW_THRESHOLD_MAX			0x7FFFFFFF
#define QM_PAUSE_THRESHOLD_MIN			         0
#define QM_PAUSE_THRESHOLD_MAX			0x7FFFFFFF
#define QM_HIGH_THRESHOLD_MIN			         0
#define QM_HIGH_THRESHOLD_MAX			0x7FFFFFFF
#define QM_TRAFFIC_SOURCE_MIN			         0
#define QM_TRAFFIC_SOURCE_MAX			0x7FFFFFFF
#define QM_HOST_MIN				         0
#define QM_HOST_MAX				         1
#define QM_REORDER_CLASS_MIN	         0
#define QM_REORDER_CLASS_MAX	        64
#define QM_SID_MIN				         0
#define QM_SID_MAX				0x7FFFFFFF
#define QM_CMD_MIN				         0
#define QM_CMD_MAX				0x7FFFFFFF

/* typedef void *      qm_handle;*/

/**
 *
 *  Return values:
 *		0 - success
 */
int qm_open(void);

/**
 *
 *  Return values:
 *		0 - success
 */
int qm_close(void);

/**
 *
 *  Return values:
 *		0 - success
 */
int qm_restart(void);

/**
 *  Set base address in Dram for pool
 *
 *  Return values:
 *		0 - success
 */
int qm_pfe_base_address_pool_set(
							u32 *pl_base_address, /* Payload DRAM base address */
							u32 *qece_base_address);/* QE/CE DRAM base address */

/**
 *  Enables QM,
 *  Configure DMA with GPM pool thresholds with default values
 *  Return values:
 *		0 - success
 */
int qm_dma_gpm_pools_def_enable(void);

/**
 *  Enables QM,
 *  Configure DMA with GPM pool thresholds
 *  Return values:
 *		0 - success
 */
int qm_dma_gpm_pools_enable(
				u32 qece_thr_hi, /* GPM qe pool (pool 0) high 32bits threshold */
				u32 qece_thr_lo, /* GPM qe pool (pool 0) low 32bits threshold */
				u32 pl_thr_hi, /* GPM payload pool (pool 1) hi 32bits threshold */
				u32 pl_thr_lo);/* GPM payload pool (pool 1) low 32bits threshold */

/**
 *  Configure DMA with DRAM pool thresholds with default values
 *  Return values:
 *		0 - success
 */
int qm_dma_dram_pools_def_enable(void);

/**
 *  Configure DMA with DRAM pool thresholds
 *  Return values:
 *		0 - success
 */
int qm_dma_dram_pools_enable(
				u32 qece_thr_hi, /* DRAM qe pool (pool 2) high 32bits threshold */
				u32 qece_thr_lo, /* DRAM qe pool (pool 2) low 32bits threshold */
				u32 pl_thr_hi, /* DRAM payload pool (pool 3) hi 32bits threshold */
				u32 pl_thr_lo);/* DRAM payload pool (pool 3) low 32bits threshold */

/**
 *  Configures for each queue in DMA if the queue resides in GPM or in DRAM
 *  Return values:
 *		0 - success
 */
int qm_dma_queue_memory_type_set(
				u32 queue, /* Queue number 0 to 511 */
				u32 memory_type);/* Memory type 0 - for DRAM 1 - for GPM */

/**
 *  Disable prefetching of BM from DMA and PFE - TBD
 *  Return values:
 *		0 - success
int qm_disable(void);
TBD - ask yuval peleg defined bits to stops and bit to check if it is stopped
*/

/**
 *  verify if there is any Queue (0 to 511),
 *  that has a queue length larger than 0
 *  Return values:
 *		0 - success
 */
int qm_packets_in_queues(
				u32 *status);

/**
 *  Set default for QM units for mandatory parameters
 *  Return values:
 *		0 - success
 */
int qm_default_set(void);

/**
 *  Set SID number for each pool (0 and 1) in REORDER unit with default values
 *  Return values:
 *		0 - success
 */
int qm_ru_pool_sid_number_def_set(void);

/**
 *  Set SID number for each pool (0 and 1) in REORDER unit
 *  Return values:
 *		0 - success
 */
int qm_ru_pool_sid_number_set(
				u32 pool0_sid_num, /* SID number fo pool 0. Total number of SID is 4k */
				u32 pool1_sid_num);/* SID number fo pool 1. Total number of SID is 4k */

/**
 * Configure REORDER with class command when permission is granted with default values.
 *  Return values:
 *		0 - success
 */
int qm_ru_port_to_class_def_set(void);

/**
 * Configure REORDER with class command when permission is granted.
 *  Return values:
 *		0 - success
 */
int qm_ru_port_to_class_set(
				u32 *port_class_arr, /* class number in reorder unit. 0 to 63 */
				u32 *port_pool_arr, /* holds pool  values which are either 0 or 1 */
				u32 input_port);/* input port that arrive with the packet. 0 to 287 */
/*
				u32 reorder_class, / * class number in reorder unit. 0 to 63 * /
				u32 input_port, / * input port that arrive with the packet. 0 to 287 * /
*/

/**
 *  Configure DQF fifo base and depth thresholds with default values
 *  Return values:
 *		0 - success
 */
int qm_dqf_port_data_fifo_def_set(void);

/**
 *  Configure DQF fifo base and depth thresholds
 *  Return values:
 *		0 - success
 */
int qm_dqf_port_data_fifo_set(
				u32 *port_depth_arr);/* holds depth in Bytes for ports 0 to 15 */

/**
 *  Configure DQF fifo credit thresholds with default values
 *  Return values:
 *		0 - success
 */
int qm_dqf_port_credit_thr_def_set(void);

/**
 *  Configure DQF fifo credit thresholds
 *  Return values:
 *		0 - success
 */
int qm_dqf_port_credit_thr_set(
				u32 *port_credit_thr_arr);/* Configures credits thresholds for xMac input ports */

/**
 *  Configure DQF for each port which PPC (data or maintenance) handles the packet,
 *  relevant only for PPC port with default values.
 *  Return values:
 *		0 - success
 */
int qm_dqf_port_ppc_map_def_set(void);

/**
 *  Configure DQF for each port which PPC (data or maintenance) handles the packet,
 *  relevant only for PPC port.
 *  Return values:
 *		0 - success
 */
int qm_dqf_port_ppc_map_set(
				u32 *port_ppc_arr, /* holds indication which PPC process packets from this port */
				u32 port);/* input ports */

/**
 *  Configure DMA QOS write attributes with default values.
 *  Return values:
 *		0 - success
 */
int qm_dma_qos_attr_def_set(void);

/**
 *  Configure DMA QOS write attributes.
 *  Return values:
 *		0 - success
 */
int qm_dma_qos_attr_set(
				u32 swf_awqos,
				u32 rdma_awqos,
				u32 hwf_qe_ce_awqos,
				u32 hwf_sfh_pl_awqos);

/**
 *  Configure DMA CACHE write attributes with default values.
 *  Return values:
 *		0 - success
 */
int qm_dma_cache_attr_def_set(void);

/**
 *  Configure DMA CACHE write attributes.
 *  Return values:
 *		0 - success
 */
int qm_dma_cache_attr_set(
				u32 swf_awcache,
				u32 rdma_awcache,
				u32 hwf_qe_ce_awcache,
				u32 hwf_sfh_pl_awcache);

/**
 *  Configure DMA DOMAIN write attributes with default values.
 *  Return values:
 *		0 - success
 */
int qm_dma_domain_attr_def_set(void);

/**
 *  Configure DMA DOMAIN write attributes.
 *  Return values:
 *		0 - success
 */
int qm_dma_domain_attr_set(
				u32 swf_awdomain,
				u32 rdma_awdomain,
				u32 hwf_qe_ce_awdomain,
				u32 hwf_sfh_pl_awdomain);

/**
 *  Configure PFE QOS read attributes with default values.
 *  Return values:
 *		0 - success
 */
int qm_pfe_qos_attr_def_set(void);

/**
 *  Configure PFE QOS read attributes.
 *  Return values:
 *		0 - success
 */
int qm_pfe_qos_attr_set(
				u32 swf_arqos,
				u32 rdma_arqos,
				u32 hwf_qe_ce_arqos,
				u32 hwf_sfh_pl_arqos);

/**
 *  Configure PFE CACHE read attributes with default values.
 *  Return values:
 *		0 - success
 */
int qm_pfe_cache_attr_def_set(void);

/**
 *  Configure PFE CACHE read attributes.
 *  Return values:
 *		0 - success
 */
int qm_pfe_cache_attr_set(
				u32 swf_arcache,
				u32 rdma_arcache,
				u32 hwf_qe_ce_arcache,
				u32 hwf_sfh_pl_arcache);

/**
 *  Configure PFE DOMAIN read attributes with default values.
 *  Return values:
 *		0 - success
 */
int qm_pfe_domain_attr_def_set(void);

/**
 *  Configure PFE DOMAIN read attributes.
 *  Return values:
 *		0 - success
 */
int qm_pfe_domain_attr_set(
				u32 swf_ardomain,
				u32 rdma_ardomain,
				u32 hwf_qe_ce_ardomain,
				u32 hwf_sfh_pl_ardomain);

/**
 *  Configures per queue threshold profile with default values.
 *  Return values:
 *		0 - success
 */
int qm_ql_q_profile_def_set(void);

/**
 *  Configures per queue threshold profile.
 *  Return values:
 *		0 - success
 */
int qm_ql_q_profile_set(
				u32 queue_profile,
				u32 queue);

/**
 *  Configures QL threshold for pause, on (low) and off (high)
 *  and the to whom to send it (source) with default values.
 *  Return values:
 *		0 - success
 */
int qm_ql_thr_def_set(void);

/**
 *  Configures QL threshold for pause, on (low) and off (high)
 *  and the to whom to send it (source).
 *  Return values:
 *		0 - success
 */
int qm_ql_thr_set(
				u32 low_threshold,
				u32 pause_threshold,
				u32 high_threshold,
				u32 traffic_source,
				u32 queue_profile);


/**
 *  Configure DMA write attributes for software forwarding mode
 *  Return values:
 *		0 - success
 */
int qm_axi_swf_write_attr_set(
						u32 qos,
						u32 cache,
						u32 domain);

/**
 *  Configure DMA write attributes for rdma mode
 *  Return values:
 *		0 - success
 */
int qm_axi_rdma_write_attr_set(
						u32 qos,
						u32 cache,
						u32 domain);

/**
 *  Configure DMA write attributes for hardware forwarding mode for qe/ce
 *  Return values:
 *		0 - success
 */
int qm_axi_hwf_qece_write_attr_set(
						u32 qos,
						u32 cache,
						u32 domain);

/**
 *  Configure DMA write attributes for hardware forwarding mode for payload
 *  Return values:
 *		0 - success
 */
int qm_axi_hwf_pyl_write_attr_set(
						u32 qos,
						u32 cache,
						u32 domain);


/**
 *  Initiate QM DRAM pools
 *	Configures BM for pool initialization and enable the pool.  Doesn't configure cache parameters.
 *  This function is a super set of several bm function that are listed below
 *  Note: No change can be made to pool after this function is called.
 *  Return values:
 *		0 - success
 */
int qm_dram_qm_pool_quick_init(
		u32 pool, /* pool number: 2 or 3 */
		u32 num_of_buffers,  /* number of buffers/PEs in pool max pool = 2M x 8B*/
		u32 base_address_hi, /* hi part of DRAM base address */
		u32 base_address_lo, /* low part of DRAM base address */
		u32 ae_thr, /* almost empty threshold for pool */
		u32 af_thr);/* almost full threshold for pool */

/**
 *  Enables DMA and PFE by configuring the registers that represents GPM and DRAM thresholds
 *  Note: BM is not enabled from here since BM should be enabled the last
 *  Return values:
 *		0 - success
 */
int qm_enable(
			u32 gpm_qe_thr_hi,
			u32 gpm_qe_thr_lo,
			u32 gpm_pl_thr_hi,
			u32 gpm_pl_thr_lo,
			u32 dram_qe_thr_hi,
			u32 dram_qe_thr_lo,
			u32 dram_pl_thr_hi,
			u32 dram_pl_thr_lo);


/**
 *  Get Idle status from DMA
 *  Return values:
 *		0 - success
 */
int qm_idle_status_get(
			u32 *dma_status);/* DMA status is output to the called */

/**
 *  Set VMID in DMA and PFE
 *  Return values:
 *		0 - success
 */
int qm_vmid_set(u32 qm_vmid);/* VMID value for DAM and PFE */
int dma_vmid_set(u32 qm_vmid);/* VMID value for DAM and PFE */
int pfe_vmid_set(u32 qm_vmid);/* VMID value for DAM and PFE */


/**
 *  Interupts handling - TBD
 *  Return values:
 *		0 - success
 */
int qm_inter_read(
					u32 group,
					u32 *dataPtr);

/**
 *  Interupts handling - TBD
 *  Return values:
 *		0 - success
 */
int qm_inter_clean(
					u32 group);

/**
 *  Interupts handling - TBD
 *  Return values:
 *		0 - success
 */
int qm_inter_mask(
					u32 group,
					u32 mask);

/**
 *  Errors handling - TBD
 *  Return values:
 *		0 - success
 */
int qm_error_read(
					u32 group,
					u32 *dataPtr);

/**
 *  Errors handling - TBD
 *  Return values:
 *		0 - success
 */
int qm_error_clean(
					u32 group);

/**
 *  Errors handling - TBD
 *  Return values:
 *		0 - success
 */
int qm_error_mask(
					u32 group,
					u32 mask);

/**
 *  Dump registers values from all modules apart of BM
 *  Return values:
 *		0 - success
 */
int qm_debug_dump_registers(void);

/**
 *  Configre PFE to start Flushing Queue. This process takes a while.
 *  Indication for its completion is when Queue is empty
 *  Return values:
 *		0 - success
 */
int qm_queue_flush_start(
						u32 queue);/* queue number from 0 to 511 */

/**
 *  Configure PFE to stop Flushing Queue.
 *  Return values:
 *		0 - success
 */
int qm_queue_flush_stop(u32 queue);

/**
 *  Configre PFE to start Flushing Port. This process takes a while.
 *  Indication for its completion is when Port is empty
 *  Return values:
 *		0 - success
 */
int qm_port_flush_start(
							u32 port);/* port number from 0 to 15 */

/**
 *  Configure PFE to stop Flushing Port.
 *  Return values:
 *		0 - success
 */
int qm_port_flush_stop(
						u32 port);/* port number from 0 to 15 */

/**
 *  Get from DQF read and write pointers for specific port
 *  Return values:
 *		0 - success
 */
int qm_port_fifo_ptr_get(
						u32 port, /* port number 0 to 15 */
						u32 *read,
						u32 *write);

/**
 *  Configures QL thresholds for EMAC and HMAC (configured per source )
 *  Return values:
 *		0 - success
 */
int qm_ql_source_thr_set(
				u32 low, /* low threshold: beneath it will turn ON*/
				u32 pause, /* pause threshold above it will send PAUSE*/
				u32 high, /* high threshold above it will turn OFF*/
				u32 source);/* source 0 to 6 */

/**
 *  Get head of class from REORDER unit
 *  Return values:
 *		0 - success
 */
int qm_class_head_get(
					u32 reorder_class, /* class number in reorder unit. 0 to 63 */
					u32 *head);/* output to caller */

/**
 * Configure REORDER with class command when permission is granted.
 *  Return values:
 *		0 - success
 */
int qm_class_cmd_set(
					u32 host,
					u32 reorder_class, /* class number in reorder unit. 0 to 63 */
					u32 sid, /* sid is in the range 0 to 4k */
					u32 cmd);/* cmd is either update or release */

/**
 *  Write QM register
 *  Return values:
 *		0 - success
 */
int qm_register_write(u32 base_address, u32 offset, u32 wordsNumber, u32 *dataPtr);

/**
 *  Read QM register
 *  Return values:
 *		0 - success
 */
int qm_register_read(u32 base_address, u32 offset, u32 wordsNumber, u32 *dataPtr);


#endif /* MV_QM_H */
