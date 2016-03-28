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

#ifndef __mv_bm_reg_h__
#define __mv_bm_reg_h__

#define BM_UNIT_OFFS								(0x4D0000)

/* Bank 0 Pool Configuration */
#define BM_B0_POOL_CFG_REG(_pid_)						(0x8000 + 0x8 * (_pid_))
#define BM_B0_POOL_CFG_ENABLE_OFFS						0
#define BM_B0_POOL_CFG_ENABLE_MASK    \
		(0x00000001 << BM_B0_POOL_CFG_ENABLE_OFFS)

#define BM_B0_POOL_CFG_QUICK_INIT_OFFS						1
#define BM_B0_POOL_CFG_QUICK_INIT_MASK    \
		(0x00000001 << BM_B0_POOL_CFG_QUICK_INIT_OFFS)

/* AE and AEE */
#define BM_B0_POOL_CFG_AE_THR_OFFS						2
#define BM_B0_POOL_CFG_AE_THR_MASK    \
		(0x00001fff << BM_B0_POOL_CFG_AE_THR_OFFS)

#define BM_B0_POOL_CFG_AAE_THR_OFFS						15
#define BM_B0_POOL_CFG_AAE_THR_MASK    \
		(0x00001fff << BM_B0_POOL_CFG_AAE_THR_OFFS)


/* Bank 0 Pool status */
#define BM_B0_POOL_STATUS_REG(_pid_)						(0x8004 + 0x8 * (_pid_))

/* Bank X [1-4] Pool status */
#define BM_BGP_POOL_STATUS_REG(_bid_, _pid_)	\
							(0x8044 + 0x200*((_bid_) - 1) + 0x8 * (_pid_))

/* fields are relevant for Banks 0-4 */
#define BM_BGP_POOL_STATUS_NEMPTY_OFFS						0
#define BM_BGP_POOL_STATUS_NEMPTY_MASK    \
		(0x00000001 << BM_BGP_POOL_STATUS_NEMPTY_OFFS)

#define BM_BGP_POOL_STATUS_AE_OFFS						1
#define BM_BGP_POOL_STATUS_AE_MASK    \
		(0x00000001 << BM_BGP_POOL_STATUS_AE_OFFS)

#define BM_BGP_POOL_STATUS_AF_OFFS						2
#define BM_BGP_POOL_STATUS_AF_MASK    \
		(0x00000001 << BM_BGP_POOL_STATUS_AF_OFFS)

#define BM_BGP_POOL_STATUS_FILL_BGT_SI_THR_OFFS					3
#define BM_BGP_POOL_STATUS_FILL_BGT_SI_THR_MASK    \
		(0x00000001 << BM_BGP_POOL_STATUS_FILL_BGT_SI_THR_OFFS)


/* Bank X [1-4] Pool Configuration */
#define BM_BGP_POOL_CFG_REG(_bid_, _pid_)	\
							(0x8040 + 0x200*((_bid_) - 1) + 0x8 * (_pid_))
#define BM_BGP_POOL_CFG_ENABLE_OFFS						0
#define BM_BGP_POOL_CFG_ENABLE_MASK    \
		(0x00000001 << BM_BGP_POOL_CFG_ENABLE_OFFS)

#define BM_BGP_POOL_CFG_IN_PAIRS_OFFS						1
#define BM_BGP_POOL_CFG_IN_PAIRS_MASK    \
		(0x00000001 << BM_BGP_POOL_CFG_IN_PAIRS_OFFS)

#define BM_BGP_POOL_CFG_PE_SIZE_OFFS						2
#define BM_BGP_POOL_CFG_PE_SIZE_MASK    \
		(0x00000001 << BM_BGP_POOL_CFG_PE_SIZE_OFFS)

#define BM_BGP_POOL_CFG_QUICK_INIT_OFFS						3
#define BM_BGP_POOL_CFG_QUICK_INIT_MASK    \
		(0x00000001 << BM_BGP_POOL_CFG_QUICK_INIT_OFFS)

#define BM_BGP_POOL_CFG_AE_THR_OFFS						4
#define BM_BGP_POOL_CFG_AE_THR_MASK    \
		(0x00001fff << BM_BGP_POOL_CFG_AE_THR_OFFS)


/* Bank X [0-4] System Recoverable interrupt mask reg */
#define BM_BANK_SYS_REC_INTERRUPT_MASK_REG(_bid_)				(0x90a4 + 0x30 * (_bid_))
/* Bank X [0-4] System Recoverable interrupt cause reg */
#define BM_BANK_SYS_REC_INTERRUPT_CAUSE_REG(_bid_)				(0x90d0 + 0x30 * (_bid_))

/* B0 Internal Debug Nonrecoverable Bank Status */
#define BM_B0_INT_DBG_NREC_STATUS_REGG						(0x9080)
/* Bank X [1-4] Internal Debug Nonrecoverable Bank Status */
#define BM_BGP_INT_DBG_NREC_STATUS_REG(_bid_)					(0x90c0 + 0x30 * ((_bid_) - 1))

/* B0 Internal Debug Nonrecoverable Pool Status */
#define BM_B0_INT_DBG_NREC_POOL_STATUS_RE					(0x9090)

/* Bank X [0-4] System Recoverable Bank D0 Status */
#define BM_BANK_SYS_REC_D0_STATUS_REG(_bid_)					(0x90b0 + 0x30 * (_bid_))

/* Bank X [0-4] System Recoverable Bank D1 Status */
#define BM_BANK_SYS_REC_D1_STATUS_REG(_bid_)					(0x90b4 + 0x30 * (_bid_))

/* interrupts registers */
#define BM_SW_DBG_REC_INT_CAUSE_REG						(0x9190)
#define BM_ERR_INTERRUPT_CAUSE_REG						(0xA000)
#define BM_FUNC_INTERRUPT_CAUSE_REG						(0xA010)
#define BM_ECC_ERR_INTERRUPT_CAUSE_REG						(0xA020)
#define BM_B0_POOL_NEMPTY_INTERRUPT_CAUSE_REG					(0xA040)
#define BM_BGP_POOL_NEMPTY_INTERRUPT_CAUSE_REG(_bid_)				(0xA060 + (((_bid_) - 1) * 0x10))
#define BM_B0_AE_INTERRUPT_CAUSE_REG						(0xA048)
#define BM_B0_AF_INTERRUPT_CAUSE_REG						(0xA050)
#define BM_BGP_AE_INTERRUPT_CAUSE_REG(_bid_)					(0xA0A0 + (((_bid_) - 1) * 0x10))
#define BM_BGP_AF_INTERRUPT_CAUSE_REG(_bid_)					(0xA0E0 + (((_bid_) - 1) * 0x10))

/* System Nonrecoverable Common Debug [0-3] Status register*/
#define BM_SYS_NREC_COMMON_DX_STATUS_REG(i)					(0x91b0 + (i)*4)

/* Common General Configuration */
#define BM_COMMON_GENERAL_CFG_REG						(0x9300)
#define BM_COMMON_GENERAL_CFG_DRM_SI_DECIDE_EXTRA_FILL_OFFS			0
#define BM_COMMON_GENERAL_CFG_DRM_SI_DECIDE_EXTRA_FILL_MASK    \
		(0x000000ff << BM_COMMON_GENERAL_CFG_DRM_SI_DECIDE_EXTRA_FILL_OFFS)

#define BM_COMMON_GENERAL_CFG_DM_VMID_OFFS					8
#define BM_COMMON_GENERAL_CFG_DM_VMID_MASK    \
		(0x000000ff << BM_COMMON_GENERAL_CFG_DM_VMID_OFFS)

#define BM_COMMON_GENERAL_CFG_BM_REQ_RCV_EN_OFFS				16
#define BM_COMMON_GENERAL_CFG_BM_REQ_RCV_EN_MASK    \
		(0x00000001 << BM_COMMON_GENERAL_CFG_BM_REQ_RCV_EN_OFFS)

#define BM_COMMON_GENERAL_CFG_BM_ARBURST_FROM_DMA_AS_IS_OFFS			17
#define BM_COMMON_GENERAL_CFG_BM_ARBURST_FROM_DMA_AS_IS_MASK    \
		(0x00000001 << BM_COMMON_GENERAL_CFG_BM_ARBURST_FROM_DMA_AS_IS_OFFS)

#define BM_COMMON_GENERAL_CFG_BM_AWBURST_FROM_DMA_AS_IS_OFFS			18
#define BM_COMMON_GENERAL_CFG_BM_AWBURST_FROM_DMA_AS_IS_MASK    \
		(0x00000001 << BM_COMMON_GENERAL_CFG_BM_AWBURST_FROM_DMA_AS_IS_OFFS)

#define BM_COMMON_GENERAL_CFG_BM_ARBURST_FROM_PPE_AS_IS_OFFS			19
#define BM_COMMON_GENERAL_CFG_BM_ARBURST_FROM_PPE_AS_IS_MASK    \
		(0x00000001 << BM_COMMON_GENERAL_CFG_BM_ARBURST_FROM_PPE_AS_IS_OFFS)

#define BM_COMMON_GENERAL_CFG_BM_AWBURST_FROM_PPE_AS_IS_OFFS			20
#define BM_COMMON_GENERAL_CFG_BM_AWBURST_FROM_PPE_AS_IS_MASK    \
		(0x00000001 << BM_COMMON_GENERAL_CFG_BM_AWBURST_FROM_PPE_AS_IS_OFFS)

#define BM_COMMON_GENERAL_CFG_BM_ARBURST_FROM_MAC_AS_IS_OFFS			21
#define BM_COMMON_GENERAL_CFG_BM_ARBURST_FROM_MAC_AS_IS_MASK    \
		(0x00000001 << BM_COMMON_GENERAL_CFG_BM_ARBURST_FROM_MAC_AS_IS_OFFS)

#define BM_COMMON_GENERAL_CFG_BM_AWBURST_FROM_MAC_AS_IS_OFFS			22
#define BM_COMMON_GENERAL_CFG_BM_AWBURST_FROM_MAC_AS_IS_MASK    \
		(0x00000001 << BM_COMMON_GENERAL_CFG_BM_AWBURST_FROM_MAC_AS_IS_OFFS)


/* Dram Domain Configuration */
#define BM_DRAM_DOMAIN_CFG_REG							(0x9304)
#define BM_DRAM_DOMAIN_CFG_WR_B0_OFFS						0
#define BM_DRAM_DOMAIN_CFG_WR_B0_MASK    \
		(0x00000003 << BM_DRAM_DOMAIN_CFG_WR_B0_OFFS)

#define BM_DRAM_DOMAIN_CFG_WR_BGP_OFFS						2
#define BM_DRAM_DOMAIN_CFG_WR_BGP_MASK    \
		(0x00000003 << BM_DRAM_DOMAIN_CFG_WR_BGP_OFFS)

#define BM_DRAM_DOMAIN_CFG_RD_B0_OFFS						4
#define BM_DRAM_DOMAIN_CFG_RD_B0_MASK    \
		(0x00000003 << BM_DRAM_DOMAIN_CFG_RD_B0_OFFS)

#define BM_DRAM_DOMAIN_CFG_RD_BGP_OFFS						6
#define BM_DRAM_DOMAIN_CFG_RD_BGP_MASK    \
		(0x00000003 << BM_DRAM_DOMAIN_CFG_RD_BGP_OFFS)


/* Dram Cache Configuration */
/* TODO - remove fildes offstes if not in use*/
#define BM_DRAM_CACHE_CFG_REG							(0x9308)
#define BM_DRAM_CACHE_CFG_WR_B0_OFFS						0
#define BM_DRAM_CACHE_CFG_WR_B0_MASK    \
		(0x0000000f << BM_DRAM_CACHE_CFG_WR_B0_OFFS)

#define BM_DRAM_CACHE_CFG_WR_BGP_OFFS						4
#define BM_DRAM_CACHE_CFG_WR_BGP_MASK    \
		(0x0000000f << BM_DRAM_CACHE_CFG_WR_BGP_OFFS)

#define BM_DRAM_CACHE_CFG_RD_B0_OFFS						8
#define BM_DRAM_CACHE_CFG_RD_B0_MASK    \
		(0x0000000f << BM_DRAM_CACHE_CFG_RD_B0_OFFS)

#define BM_DRAM_CACHE_CFG_RD_BGP_OFFS						12
#define BM_DRAM_CACHE_CFG_RD_BGP_MASK    \
		(0x0000000f << BM_DRAM_CACHE_CFG_RD_BGP_OFFS)


/* Dram Qos Configuration */
#define BM_DRAM_QOS_CFG_REG							(0x930c)
#define BM_DRAM_QOS_CFG_WR_B0_OFFS						0
#define BM_DRAM_QOS_CFG_WR_B0_MASK    \
		(0x00000003 << BM_DRAM_QOS_CFG_WR_B0_OFFS)

#define BM_DRAM_QOS_CFG_WR_BGP_OFFS						2
#define BM_DRAM_QOS_CFG_WR_BGP_MASK    \
		(0x00000003 << BM_DRAM_QOS_CFG_WR_BGP_OFFS)

#define BM_DRAM_QOS_CFG_RD_B0_OFFS						4
#define BM_DRAM_QOS_CFG_RD_B0_MASK    \
		(0x00000003 << BM_DRAM_QOS_CFG_RD_B0_OFFS)

#define BM_DRAM_QOS_CFG_RD_BGP_OFFS						6
#define BM_DRAM_QOS_CFG_RD_BGP_MASK    \
		(0x00000003 << BM_DRAM_QOS_CFG_RD_BGP_OFFS)


/* Bank X [0-4] Bank Request Fifos Status */
#define BM_BANK_REQUEST_FIFOS_STATUS_REG(_bid_)					(0xa200 + 4 * (_bid_))

/* B0 Past Alc Fifos Status */
#define BM_B0_PAST_ALC_FIFOS_STATUS_REG						(0xa220)

/* BANKS 1-4 Past Alc Fifos Status */
#define BM_BGP_PAST_ALC_FIFOS_FILL_STATUS_REG					(0xa224)

/* B0 Release-wrap-ppe Fifos Status */
#define BM_B0_RELEASE_WRAP_PPE_FIFOS_STATUS_REG					(0xa230)

/* Dm Axi Fifos Status */
#define BM_DM_AXI_FIFOS_STATUS_REG						(0xa240)


/* Drm Pending Fifo Status */
#define BM_DRM_PENDING_FIFO_STATUS_REG						(0xa244)
#define BM_DRM_PENDING_FIFO_STATUS_FILL_OFFS					0
#define BM_DRM_PENDING_FIFO_STATUS_FILL_MASK    \
		(0x000000ff << BM_DRM_PENDING_FIFO_STATUS_FILL_OFFS)


/* Dm Axi Write Pending Fifo Status */
#define BM_DM_AXI_WRITE_PENDING_FIFO_STATUS_REG					(0xa248)


/* Bm Idle Status */
#define BM_IDLE_STATUS_REG							(0xa250)
#define BM_IDLE_STATUS_IDLE_OFFS						0
#define BM_IDLE_STATUS_IDLE_MASK    \
		(0x00000001 << BM_IDLE_STATUS_IDLE_OFFS)

#endif /* __mv_bm_reg_h__ */
