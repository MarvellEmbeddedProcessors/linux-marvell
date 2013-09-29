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


#ifndef __mvNetaRegs_h__
#define __mvNetaRegs_h__

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#if !defined(CONFIG_OF)
#include "mvSysEthConfig.h"
#endif

#define NETA_REG_BASE(port) 				MV_ETH_REGS_BASE(port)


/************************** NETA TX Registers ******************************/

#ifdef CONFIG_MV_PON
#define NETA_TX_REG_BASE(p, txp)  	(MV_PON_PORT(p) ? \
					(MV_PON_REG_BASE + 0x4000 + ((txp) >> 1) * 0x2000 + ((txp) & 0x1) * 0x400) :  \
					(NETA_REG_BASE(p) + 0x2400))
#else
#define NETA_TX_REG_BASE(p, txp)   	(NETA_REG_BASE(p) + 0x2400)
#endif /* CONFIG_MV_PON */

/************************** NETA RX Registers ******************************/

/* PxRXyC: Port RX queues Configuration Register */
#define NETA_RXQ_CONFIG_REG(p, q)        	(NETA_REG_BASE(p) + 0x1400 + ((q) << 2))

#define NETA_RXQ_HW_BUF_ALLOC_BIT           0
#define NETA_RXQ_HW_BUF_ALLOC_MASK          (1 << NETA_RXQ_HW_BUF_ALLOC_BIT)

#define NETA_RXQ_SHORT_POOL_ID_OFFS         4
#define NETA_RXQ_SHORT_POOL_ID_MASK         (0x3 << NETA_RXQ_SHORT_POOL_ID_OFFS)

#define NETA_RXQ_LONG_POOL_ID_OFFS          6
#define NETA_RXQ_LONG_POOL_ID_MASK          (0x3 << NETA_RXQ_LONG_POOL_ID_OFFS)

#define NETA_RXQ_PACKET_OFFSET_OFFS         8
#define NETA_RXQ_PACKET_OFFSET_ALL_MASK     (0xF << NETA_RXQ_PACKET_OFFSET_OFFS)
#define NETA_RXQ_PACKET_OFFSET_MASK(offs)   ((offs) << NETA_RXQ_PACKET_OFFSET_OFFS)


#define NETA_RXQ_INTR_ENABLE_BIT            15
#define NETA_RXQ_INTR_ENABLE_MASK           (0x1 << NETA_RXQ_INTR_ENABLE_BIT)

/* ????? What about PREFETCH commands 1, 2, 3 */
#define NETA_RXQ_PREFETCH_MODE_BIT          16
#define NETA_RXQ_PREFETCH_PNC               (0 << NETA_RXQ_PREFETCH_MODE_BIT)
#define NETA_RXQ_PREFETCH_CMD_0             (1 << NETA_RXQ_PREFETCH_MODE_BIT)
/*-------------------------------------------------------------------------------*/

#define NETA_RXQ_SNOOP_REG(p, q)            (NETA_REG_BASE(p) + 0x1420 + ((q) << 2))

#define NETA_RXQ_SNOOP_BYTES_OFFS           0
#define NETA_RXQ_SNOOP_BYTES_MASK           (0x3FFF << NETA_RXQ_SNOOP_BYTES_OFFS)

#define NETA_RXQ_L2_DEPOSIT_BYTES_OFFS      16
#define NETA_RXQ_L2_DEPOSIT_BYTES_MASK      (0x3FFF << NETA_RXQ_L2_DEPOSIT_BYTES_OFFS)


#define NETA_RXQ_PREFETCH_01_REG(p, q)      (NETA_REG_BASE(p) + 0x1440 + ((q) << 2))
#define NETA_RXQ_PREFETCH_23_REG(p, q)      (NETA_REG_BASE(p) + 0x1460 + ((q) << 2))

#define NETA_RXQ_PREFETCH_CMD_OFFS(cmd)     (((cmd) & 1) ? 16 : 0)
#define NETA_RXQ_PREFETCH_CMD_MASK(cmd)     (0xFFFF << NETA_RXQ_PREFETCH_CMD_OFFS(cmd))
/*-------------------------------------------------------------------------------*/

#define NETA_RXQ_BASE_ADDR_REG(p, q)        (NETA_REG_BASE(p) + 0x1480 + ((q) << 2))
#define NETA_RXQ_SIZE_REG(p, q)             (NETA_REG_BASE(p) + 0x14A0 + ((q) << 2))

#define NETA_RXQ_DESC_NUM_OFFS              0
#define NETA_RXQ_DESC_NUM_MASK              (0x3FFF << NETA_RXQ_DESC_NUM_OFFS)

#define NETA_RXQ_BUF_SIZE_OFFS              19
#define NETA_RXQ_BUF_SIZE_MASK              (0x1FFF << NETA_RXQ_BUF_SIZE_OFFS)
/*-------------------------------------------------------------------------------*/

#define NETA_RXQ_THRESHOLD_REG(p, q)        (NETA_REG_BASE(p) + 0x14C0 + ((q) << 2))
#define NETA_RXQ_STATUS_REG(p, q)           (NETA_REG_BASE(p) + 0x14E0 + ((q) << 2))

#define NETA_RXQ_OCCUPIED_DESC_OFFS         0
#define NETA_RXQ_OCCUPIED_DESC_ALL_MASK     (0x3FFF << NETA_RXQ_OCCUPIED_DESC_OFFS)
#define NETA_RXQ_OCCUPIED_DESC_MASK(val)    ((val) << NETA_RXQ_OCCUPIED_DESC_OFFS)

#define NETA_RXQ_NON_OCCUPIED_DESC_OFFS     16
#define NETA_RXQ_NON_OCCUPIED_DESC_ALL_MASK (0x3FFF << NETA_RXQ_NON_OCCUPIED_DESC_OFFS)
#define NETA_RXQ_NON_OCCUPIED_DESC_MASK(v)  ((v) << NETA_RXQ_NON_OCCUPIED_DESC_OFFS)
/*-------------------------------------------------------------------------------*/

#define NETA_RXQ_STATUS_UPDATE_REG(p, q)    (NETA_REG_BASE(p) + 0x1500 + ((q) << 2))

/* Decrement OCCUPIED Descriptors counter */
#define NETA_RXQ_DEC_OCCUPIED_OFFS          0
#define NETA_RXQ_DEC_OCCUPIED_MASK          (0xFF << NETA_RXQ_DEC_OCCUPIED_OFFS)

/* Increment NON_OCCUPIED Descriptors counter */
#define NETA_RXQ_ADD_NON_OCCUPIED_OFFS      16
#define NETA_RXQ_ADD_NON_OCCUPIED_MASK      (0xFF << NETA_RXQ_ADD_NON_OCCUPIED_OFFS)
/*-------------------------------------------------------------------------------*/

/* Port RX queues Descriptors Index Register (a register per RX Queue) */
#define NETA_RXQ_INDEX_REG(p, q)            (NETA_REG_BASE(p) + 0x1520 + ((q) << 2))

#define NETA_RXQ_NEXT_DESC_INDEX_OFFS       0
#define NETA_RXQ_NEXT_DESC_INDEX_MASK       (0x3FFF << NETA_RXQ_NEXT_DESC_INDEX_OFFS)
/*-------------------------------------------------------------------------------*/

/* Port Pool-N Buffer Size Register - 8 bytes alignment */
#define NETA_POOL_BUF_SIZE_REG(p, pool)     (NETA_REG_BASE(p) + 0x1700 + ((pool) << 2))
#define NETA_POOL_BUF_SIZE_ALIGN            8
#define NETA_POOL_BUF_SIZE_OFFS             3
#define NETA_POOL_BUF_SIZE_MASK             (0x1FFF << NETA_POOL_BUF_SIZE_OFFS)
/*-------------------------------------------------------------------------------*/

/* Port RX Flow Control register */
#define NETA_FLOW_CONTROL_REG(p)            (NETA_REG_BASE(p) + 0x1710)

#define NETA_PRIO_PAUSE_PKT_GEN_BIT         0
#define NETA_PRIO_PAUSE_PKT_GEN_GIGA        (0 << NETA_PRIO_PAUSE_PKT_GEN_BIT)
#define NETA_PRIO_PAUSE_PKT_GEN_CPU         (1 << NETA_PRIO_PAUSE_PKT_GEN_BIT)

#define NETA_PRIO_TX_PAUSE_BIT              1
#define NETA_PRIO_TX_PAUSE_GIGA             (0 << NETA_PRIO_TX_PAUSE_BIT)
#define NETA_PRIO_TX_PAUSE_CPU              (1 << NETA_PRIO_TX_PAUSE_BIT)
/*-------------------------------------------------------------------------------*/

/* Port TX pause control register */
#define NETA_TX_PAUSE_REG(p)                (NETA_REG_BASE(p) + 0x1714)

/* ????? One register for all TXQs - problem for multi-core */
#define NETA_TXQ_PAUSE_ENABLE_OFFS          0
#define NETA_TXQ_PAUSE_ENABLE_ALL_MASK      (0xFF << NETA_TXQ_PAUSE_ENABLE_OFFS)
#define NETA_TXQ_PAUSE_ENABLE_MASK(q)       ((1 << q) << NETA_TXQ_PAUSE_ENABLE_OFFS)
/*-------------------------------------------------------------------------------*/

/* Port Flow Control generation control register */
#define NETA_FC_GEN_REG(p)                  (NETA_REG_BASE(p) + 0x1718)

#define NETA_PAUSE_PKT_GEN_DATA_BIT         0
#define NETA_PAUSE_PKT_GEN_DATA_OFF         (0 << NETA_PAUSE_PKT_GEN_DATA_BIT)
#define NETA_PAUSE_PKT_GEN_DATA_ON          (1 << NETA_PAUSE_PKT_GEN_DATA_BIT)

#define NETA_TXQ_PAUSE_PKT_GEN_OFFS         4
#define NETA_TXQ_PAUSE_PKT_GEN_ALL_MASK     (0x7 << NETA_TXQ_PAUSE_PKT_GEN_OFFS)
#define NETA_TXQ_PAUSE_PKT_GEN_MASK(q)      ((1 << q) << NETA_TXQ_PAUSE_PKT_GEN_OFFS)

#define NETA_RX_DEBUG_REG(p)                (NETA_REG_BASE(p) + 0x17f0)
/* RXQ memory dump: offset = 1c00 - 1cbc */

/* PxRXINIT: Port RX Initialization Register */
#define NETA_PORT_RX_RESET_REG(p)           (NETA_REG_BASE(p) + 0x1cc0)

#define NETA_PORT_RX_DMA_RESET_BIT          0
#define NETA_PORT_RX_DMA_RESET_MASK         (1 << NETA_PORT_RX_DMA_RESET_BIT)
/*-------------------------------------------------------------------------------*/


#define NETA_HWF_RX_CTRL_REG(p)             (NETA_REG_BASE(p) + 0x1d00)

#define NETA_COLOR_SRC_SEL_BIT				0
#define NETA_COLOR_SRC_SEL_MASK				(1 << NETA_COLOR_SRC_SEL_BIT)

#define NETA_GEM_PID_SRC_SEL_OFFS           4
#define NETA_GEM_PID_SRC_SEL_MASK           (7 << NETA_GEM_PID_SRC_SEL_OFFS)
#define NETA_GEM_PID_SRC_GPON_HDR           (0 << NETA_GEM_PID_SRC_SEL_OFFS)
#define NETA_GEM_PID_SRC_EXT_DSA_TAG        (1 << NETA_GEM_PID_SRC_SEL_OFFS)
#define NETA_GEM_PID_SRC_FLOW_ID            (2 << NETA_GEM_PID_SRC_SEL_OFFS)
#define NETA_GEM_PID_SRC_DSA_TAG            (3 << NETA_GEM_PID_SRC_SEL_OFFS)
#define NETA_GEM_PID_SRC_ZERO               (4 << NETA_GEM_PID_SRC_SEL_OFFS)

#define NETA_TXQ_SRC_SEL_BIT                8
#define NETA_TXQ_SRC_SEL_MASK               (1 << NETA_TXQ_SRC_SEL_BIT)
#define NETA_TXQ_SRC_FLOW_ID                (0 << NETA_TXQ_SRC_SEL_BIT)
#define NETA_TXQ_SRC_RES_INFO               (1 << NETA_TXQ_SRC_SEL_BIT)

#ifdef MV_ETH_PMT_NEW

#define NETA_MH_SEL_OFFS                    12
#define NETA_MH_SEL_MASK                    (0xF << NETA_MH_SEL_OFFS)
#define NETA_MH_DONT_CHANGE                 (0 << NETA_MH_SEL_OFFS)
#define NETA_MH_REPLACE_GPON_HDR            (1 << NETA_MH_SEL_OFFS)
#define NETA_MH_REPLACE_MH_REG(r)           (((r) + 1) << NETA_MH_SEL_OFFS)

#define NETA_MH_SRC_PNC_BIT                 16
#define NETA_MH_SRC_PNC_MASK                (1 << NETA_MH_SRC_PNC_BIT)

#define NETA_HWF_ENABLE_BIT                 17
#define NETA_HWF_ENABLE_MASK                (1 << NETA_HWF_ENABLE_BIT)

#else

#define NETA_MH_SEL_OFFS                    12
#define NETA_MH_SEL_MASK                    (0x7 << NETA_MH_SEL_OFFS)
#define NETA_MH_DONT_CHANGE                 (0 << NETA_MH_SEL_OFFS)
#define NETA_MH_REPLACE_GPON_HDR            (1 << NETA_MH_SEL_OFFS)
#define NETA_MH_REPLACE_MH_REG(r)           (((r) + 1) << NETA_MH_SEL_OFFS)

#define NETA_MH_SRC_PNC_BIT                 15
#define NETA_MH_SRC_PNC_MASK                (1 << NETA_MH_SRC_PNC_BIT)

#define NETA_HWF_ENABLE_BIT                 16
#define NETA_HWF_ENABLE_MASK                (1 << NETA_HWF_ENABLE_BIT)

#endif /* MV_ETH_PMT_NEW */

#define NETA_HWF_SHORT_POOL_OFFS            20
#define NETA_HWF_SHORT_POOL_MASK            (3 << NETA_HWF_SHORT_POOL_OFFS)
#define NETA_HWF_SHORT_POOL_ID(pool)        ((pool) << NETA_HWF_SHORT_POOL_OFFS)

#define NETA_HWF_LONG_POOL_OFFS             22
#define NETA_HWF_LONG_POOL_MASK             (3 << NETA_HWF_LONG_POOL_OFFS)
#define NETA_HWF_LONG_POOL_ID(pool)         ((pool) << NETA_HWF_LONG_POOL_OFFS)
/*-----------------------------------------------------------------------------------*/

#define NETA_HWF_RX_THRESH_REG(p)           (NETA_REG_BASE(p) + 0x1d04)

#ifdef MV_ETH_PMT_NEW

#define NETA_HWF_RX_FIFO_WORDS_OFFS         0
#define NETA_HWF_RX_FIFO_WORDS_MASK         (0x3FF << NETA_HWF_RX_FIFO_WORDS_OFFS)

#define NETA_HWF_RX_FIFO_PKTS_OFFS          16
#define NETA_HWF_RX_FIFO_PKTS_MASK          (0x7F << NETA_HWF_RX_FIFO_PKTS_OFFS)

#else

#define NETA_HWF_RX_FIFO_WORDS_OFFS         0
#define NETA_HWF_RX_FIFO_WORDS_MASK         (0xFF << NETA_HWF_RX_FIFO_WORDS_OFFS)

#define NETA_HWF_RX_FIFO_PKTS_OFFS          8
#define NETA_HWF_RX_FIFO_PKTS_MASK          (0x1F << NETA_HWF_RX_FIFO_PKTS_OFFS)

#endif /* MV_ETH_PMT_NEW */
/*-----------------------------------------------------------------------------------*/


#define NETA_HWF_TXP_CFG_REG(p, txp)        (NETA_REG_BASE(p) + 0x1d10 + ((txp) >> 1) * 4)

#define NETA_TXP_BASE_ADDR_OFFS(txp)        (((txp) & 0x1) ? 18 : 2)
#define NETA_TXP_BASE_ADDR_MASK(txp)        (0xFFFF << NETA_TXP_BASE_ADDR_OFFS(txp))
/*-----------------------------------------------------------------------------------*/

#define NETA_HWF_TX_PTR_REG(p)              (NETA_REG_BASE(p) + 0x1d30)

#define NETA_HWF_TX_PORT_OFFS               11
#define NETA_HWF_TX_PORT_ALL_MASK           (0xF << NETA_HWF_TX_PORT_OFFS)
#define NETA_HWF_TX_PORT_MASK(txp)          ((txp) << NETA_HWF_TX_PORT_OFFS)

#define NETA_HWF_TXQ_OFFS                   8
#define NETA_HWF_TXQ_ALL_MASK               (0x7 << NETA_HWF_TXQ_OFFS)
#define NETA_HWF_TXQ_MASK(txq)              ((txq) << NETA_HWF_TXQ_OFFS)

#define NETA_HWF_REG_OFFS                   0
#define NETA_HWF_REG_ALL_MASK               (0x7 << NETA_HWF_REG_OFFS)
#define NETA_HWF_REG_MASK(reg)              ((reg) << NETA_HWF_REG_OFFS)
/*-----------------------------------------------------------------------------------*/

#define NETA_HWF_DROP_TH_REG(p)             (NETA_REG_BASE(p) + 0x1d40)

#define NETA_YELLOW_DROP_THRESH_OFFS        0
#define NETA_YELLOW_DROP_THRESH_MASK        (0x3fff << NETA_YELLOW_DROP_THRESH_OFFS)

#define NETA_YELLOW_DROP_RND_GEN_OFFS       16
#define NETA_YELLOW_DROP_RND_GEN_MASK       (0xf << NETA_YELLOW_DROP_RND_GEN_OFFS)
/*-----------------------------------------------------------------------------------*/

#define NETA_HWF_TXQ_BASE_REG(p)            (NETA_REG_BASE(p) + 0x1d44)
/*-----------------------------------------------------------------------------------*/

#define NETA_HWF_TXQ_SIZE_REG(p)            (NETA_REG_BASE(p) + 0x1d48)

#define NETA_HWF_TXQ_SIZE_OFFS              0
#define NETA_HWF_TXQ_SIZE_MASK              (0x3fff << NETA_HWF_TXQ_SIZE_OFFS)
/*-----------------------------------------------------------------------------------*/

#define NETA_HWF_TXQ_ENABLE_REG(p)          (NETA_REG_BASE(p) + 0x1d4c)

#define NETA_HWF_TXQ_ENABLE_BIT             0
#define NETA_HWF_TXQ_ENABLE_MASK            (1 << NETA_HWF_TXQ_ENABLE_BIT)
/*-----------------------------------------------------------------------------------*/

#define NETA_HWF_ACCEPTED_CNTR(p)           (NETA_REG_BASE(p) + 0x1d50)
#define NETA_HWF_YELLOW_DROP_CNTR(p)        (NETA_REG_BASE(p) + 0x1d54)
#define NETA_HWF_GREEN_DROP_CNTR(p)         (NETA_REG_BASE(p) + 0x1d58)
#define NETA_HWF_THRESH_DROP_CNTR(p)        (NETA_REG_BASE(p) + 0x1d5c)

#define NETA_HWF_MEMORY_REG(p)				(NETA_REG_BASE(p) + 0x1d60)

/* Hardware Forwarding TX access gap register */
#define NETA_HWF_TX_GAP_REG(p)				(NETA_REG_BASE(p) + 0x1d6C)

#define NETA_HWF_SMALL_TX_GAP_BIT			0
#define NETA_HWF_SMALL_TX_GAP_MASK			(1 << NETA_HWF_SMALL_TX_GAP_BIT)
/*-----------------------------------------------------------------------------------*/



/**************************** NETA General Registers ***********************/

/* Cross Bar registers per Giga Unit */
#define NETA_MBUS_RETRY_REG(p)              (NETA_REG_BASE((p) & ~0x1) + 0x2010)

#define NETA_MBUS_RETRY_DISABLE_BIT			16
#define NETA_MBUS_RETRY_DISABLE_MASK		(1 << NETA_MBUS_RETRY_DISABLE_BIT)

#define NETA_MBUS_RETRY_CYCLES_OFFS			0
#define NETA_MBUS_RETRY_CYCLES_MASK			(0xFF << NETA_MBUS_RETRY_CYCLES_OFFS)
#define NETA_MBUS_RETRY_CYCLES(val)			((val) << NETA_MBUS_RETRY_CYCLES_OFFS)
/*-------------------------------------------------------------------------------*/

#define NETA_MBUS_ARBITER_REG(p)            (NETA_REG_BASE((p) & ~0x1) + 0x20C0)
/*-------------------------------------------------------------------------------*/

/* PACC - Port Acceleration Register */
#define NETA_ACC_MODE_REG(p)                (NETA_REG_BASE(p) + 0x2500)

#define NETA_ACC_MODE_OFFS                  0
#define NETA_ACC_MODE_ALL_MASK              (7 << NETA_ACC_MODE_OFFS)
#define NETA_ACC_MODE_MASK(mode)            ((mode) << NETA_ACC_MODE_OFFS)
#define NETA_ACC_MODE_LEGACY                0
#define NETA_ACC_MODE_EXT                   1
#define NETA_ACC_MODE_EXT_BMU               2
#define NETA_ACC_MODE_EXT_PNC               3
#define NETA_ACC_MODE_EXT_PNC_BMU           4
/*-------------------------------------------------------------------------------*/

#define NETA_BM_ADDR_REG(p)                 (NETA_REG_BASE(p) + 0x2504)
/*-------------------------------------------------------------------------------*/

/* RXQs and TXQs to CPU mapping */
#define NETA_MAX_CPU_REGS                   4
#define NETA_CPU_MAP_REG(p, cpu)            (NETA_REG_BASE(p) + 0x2540 + ((cpu) << 2))

#define NETA_CPU_RXQ_ACCESS_OFFS            0
#define NETA_CPU_RXQ_ACCESS_ALL_MASK        (0xFF << NETA_CPU_RXQ_ACCESS_OFFS)
#define NETA_CPU_RXQ_ACCESS_MASK(q)         (1 << (NETA_CPU_RXQ_ACCESS_OFFS + (q)))

#define NETA_CPU_TXQ_ACCESS_OFFS            8
#define NETA_CPU_TXQ_ACCESS_ALL_MASK        (0xFF << NETA_CPU_TXQ_ACCESS_OFFS)
#define NETA_CPU_TXQ_ACCESS_MASK(q)         (1 << (NETA_CPU_TXQ_ACCESS_OFFS + (q)))
/*-------------------------------------------------------------------------------*/

/* Interrupt coalescing mechanism */
#define NETA_RXQ_INTR_TIME_COAL_REG(p, q)   (NETA_REG_BASE(p) + 0x2580 + ((q) << 2))

/* Exception Interrupt Port/Queue Cause register */
#define NETA_INTR_NEW_CAUSE_REG(p)          (NETA_REG_BASE(p) + 0x25A0)
#define NETA_INTR_NEW_MASK_REG(p)           (NETA_REG_BASE(p) + 0x25A4)

#ifdef CONFIG_MV_PON
#   define GPON_CAUSE_TXQ_SENT_SUM_OFFS     0
#   define GPON_CAUSE_TXQ_SENT_SUM_MASK     (3 << GPON_CAUSE_TXQ_SENT_SUM_OFFS)
#endif /* CONFIG_MV_PON */

#define NETA_CAUSE_TXQ_SENT_DESC_OFFS       0
#define NETA_CAUSE_TXQ_SENT_DESC_BIT(q)     (NETA_CAUSE_TXQ_SENT_DESC_OFFS + (q))
#define NETA_CAUSE_TXQ_SENT_DESC_ALL_MASK   (0xFF << NETA_CAUSE_TXQ_SENT_DESC_OFFS)
#define NETA_CAUSE_TXQ_SENT_DESC_MASK(q)    (1 << (NETA_CAUSE_TXQ_SENT_DESC_BIT(q)))

#define NETA_CAUSE_RXQ_OCCUP_DESC_OFFS      8
#define NETA_CAUSE_RXQ_OCCUP_DESC_BIT(q)    (NETA_CAUSE_RXQ_OCCUP_DESC_OFFS + (q))
#define NETA_CAUSE_RXQ_OCCUP_DESC_ALL_MASK  (0xFF << NETA_CAUSE_RXQ_OCCUP_DESC_OFFS)
#define NETA_CAUSE_RXQ_OCCUP_DESC_MASK(q)   (1 << (NETA_CAUSE_RXQ_OCCUP_DESC_BIT(q)))

#define NETA_CAUSE_RXQ_FREE_DESC_OFFS       16
#define NETA_CAUSE_RXQ_FREE_DESC_BIT(q)     (NETA_CAUSE_RXQ_FREE_DESC_OFFS + (q))
#define NETA_CAUSE_RXQ_FREE_DESC_MASK(q)    (1 << (NETA_CAUSE_RXQ_FREE_DESC_BIT(q)))

#define NETA_CAUSE_OLD_REG_SUM_BIT          29
#define NETA_CAUSE_OLD_REG_SUM_MASK         (1 << NETA_CAUSE_OLD_REG_SUM_BIT)

#define NETA_CAUSE_TX_ERR_SUM_BIT           30
#define NETA_CAUSE_TX_ERR_SUM_MASK          (1 << NETA_CAUSE_TX_ERR_SUM_BIT)

#define NETA_CAUSE_MISC_SUM_BIT             31
#define NETA_CAUSE_MISC_SUM_MASK            (1 << NETA_CAUSE_MISC_SUM_BIT)

#define NETA_CAUSE_TXQ_SENT_DESC_TXP_SUM    4 /* How many Tx ports are summarized by each bit */
/*-------------------------------------------------------------------------------*/

/* Data Path Port/Queue Cause Register */
#define NETA_INTR_OLD_CAUSE_REG(p)          (NETA_REG_BASE(p) + 0x25A8)
#define NETA_INTR_OLD_MASK_REG(p)           (NETA_REG_BASE(p) + 0x25AC)

#ifdef CONFIG_MV_PON
#   define GPON_CAUSE_TXQ_BUF_OFFS          0
#   define GPON_CAUSE_TXQ_BUF_MASK          (3 << GPON_CAUSE_TXQ_BUF_OFFS)
#endif /* CONFIG_MV_PON */

#define NETA_CAUSE_TXQ_BUF_OFFS             0
#define NETA_CAUSE_TXQ_BUF_BIT(q)           (NETA_CAUSE_TXQ_BUF_OFFS + (q))
#define NETA_CAUSE_TXQ_BUF_ALL_MASK         (0xFF << NETA_CAUSE_TXQ_BUF_OFFS)
#define NETA_CAUSE_TXQ_BUF_MASK(q)          (1 << (NETA_CAUSE_TXQ_BUF_BIT(q)))

#define NETA_CAUSE_RXQ_PKT_OFFS             8
#define NETA_CAUSE_RXQ_PKT_BIT(q)           (NETA_CAUSE_RXQ_PKT_OFFS + (q))
#define NETA_CAUSE_RXQ_PKT_ALL_MASK         (0xFF << NETA_CAUSE_RXQ_PKT_OFFS)
#define NETA_CAUSE_RXQ_PKT_MASK(q)          (1 << (NETA_CAUSE_RXQ_PKT_BIT(q)))

#define NETA_CAUSE_RXQ_ERROR_OFFS           16
#define NETA_CAUSE_RXQ_ERROR_BIT(q)         (NETA_CAUSE_RXQ_ERROR_OFFS + (q))
#define NETA_CAUSE_RXQ_ERROR_ALL_MASK       (0xFF << NETA_CAUSE_RXQ_ERROR_OFFS)
#define NETA_CAUSE_RXQ_ERROR_MASK(q)        (1 << (NETA_CAUSE_RXQ_ERROR_BIT(q)))

#define NETA_CAUSE_NEW_REG_SUM_BIT          29
#define NETA_CAUSE_NEW_REG_SUM_MASK         (1 << NETA_CAUSE_NEW_REG_SUM_BIT)
/*-------------------------------------------------------------------------------*/

/* Misc Port Cause Register */
#define NETA_INTR_MISC_CAUSE_REG(p)         (NETA_REG_BASE(p) + 0x25B0)
#define NETA_INTR_MISC_MASK_REG(p)          (NETA_REG_BASE(p) + 0x25B4)

#define NETA_CAUSE_PHY_STATUS_CHANGE_BIT    0
#define NETA_CAUSE_PHY_STATUS_CHANGE_MASK   (1 << NETA_CAUSE_PHY_STATUS_CHANGE_BIT)

#define NETA_CAUSE_LINK_CHANGE_BIT          1
#define NETA_CAUSE_LINK_CHANGE_MASK         (1 << NETA_CAUSE_LINK_CHANGE_BIT)

#define NETA_CAUSE_PTP_BIT                  4

#define NETA_CAUSE_INTERNAL_ADDR_ERR_BIT    7
#define NETA_CAUSE_RX_OVERRUN_BIT           8
#define NETA_CAUSE_RX_CRC_ERROR_BIT         9
#define NETA_CAUSE_RX_LARGE_PKT_BIT         10
#define NETA_CAUSE_TX_UNDERUN_BIT           11
#define NETA_CAUSE_PRBS_ERR_BIT             12
#define NETA_CAUSE_PSC_SYNC_CHANGE_BIT      13
#define NETA_CAUSE_SERDES_SYNC_ERR_BIT      14

#define NETA_CAUSE_BMU_ALLOC_ERR_OFFS       16
#define NETA_CAUSE_BMU_ALLOC_ERR_ALL_MASK   (0xF << NETA_CAUSE_BMU_ALLOC_ERR_OFFS)
#define NETA_CAUSE_BMU_ALLOC_ERR_MASK(pool) (1 << (NETA_CAUSE_BMU_ALLOC_ERR_OFFS + (pool)))

#define NETA_CAUSE_TXQ_ERROR_OFFS           24
#define NETA_CAUSE_TXQ_ERROR_BIT(q)         (NETA_CAUSE_TXQ_ERROR_OFFS + (q))
#define NETA_CAUSE_TXQ_ERROR_ALL_MASK       (0xFF << NETA_CAUSE_TXQ_ERROR_OFFS)
#define NETA_CAUSE_TXQ_ERROR_MASK(q)        (1 << (NETA_CAUSE_TXQ_ERROR_BIT(q)))

#ifdef CONFIG_MV_PON
#   define GPON_CAUSE_TXQ_ERROR_OFFS        24
#   define GPON_CAUSE_TXQ_ERROR_MASK        (0x3 << GPON_CAUSE_TXQ_ERROR_OFFS)
#endif /* CONFIG_MV_PON */
/*-------------------------------------------------------------------------------*/

/* one register for all queues - problem for Multi-Core */
#define NETA_INTR_ENABLE_REG(p)             (NETA_REG_BASE(p) + 0x25B8)

#define NETA_RXQ_PKT_INTR_ENABLE_OFFS       0
#define NETA_RXQ_PKT_INTR_ENABLE_ALL_MASK   (0xFF << NETA_RXQ_PKT_INTR_ENABLE_OFFS)
#define NETA_RXQ_PKT_INTR_ENABLE_MASK(q)    ((1 << NETA_RXQ_PKT_INTR_ENABLE_OFFS + (q))

#define NETA_TXQ_PKT_INTR_ENABLE_OFFS       8
#define NETA_TXQ_PKT_INTR_ENABLE_ALL_MASK   (0xFF << NETA_TXQ_PKT_INTR_ENABLE_OFFS)
#define NETA_TXQ_PKT_INTR_ENABLE_MASK(q)    ((1 << NETA_TXQ_PKT_INTR_ENABLE_OFFS + (q))
/*-------------------------------------------------------------------------------*/

#define NETA_VERSION_REG(p)                 (NETA_REG_BASE(p) + 0x25BC)

#define NETA_VERSION_OFFS                   0
#define NETA_VERSION_MASK                   (0xFF << NETA_VERSION_OFFS)

/* Serdes registres: 0x72E00-0x72FFC */

#ifdef CONFIG_MV_PON
/* Extra registers for GPON port only */
#   define GPON_TXQ_INTR_ENABLE_REG(txq)    (MV_PON_REG_BASE + 0x0480 +  (txq / 32) * 4)
#   define GPON_TXQ_INTR_NEW_CAUSE_REG(txq) (MV_PON_REG_BASE + 0x0500 +  (txq / 32) * 8)
#   define GPON_TXQ_INTR_NEW_MASK_REG(txq)  (MV_PON_REG_BASE + 0x0504 +  (txq / 32) * 8)
#   define GPON_TXQ_INTR_OLD_CAUSE_REG(txq) (MV_PON_REG_BASE + 0x0540 +  (txq / 32) * 8)
#   define GPON_TXQ_INTR_OLD_MASK_REG(txq)  (MV_PON_REG_BASE + 0x0544 +  (txq / 32) * 8)
#   define GPON_TXQ_INTR_ERR_CAUSE_REG(txq) (MV_PON_REG_BASE + 0x0580 +  (txq / 32) * 8)
#   define GPON_TXQ_INTR_ERR_MASK_REG(txq)  (MV_PON_REG_BASE + 0x0584 +  (txq / 32) * 8)
#endif /* CONFIG_MV_PON */
/*-------------------------------------------------------------------------------*/

#ifdef MV_ETH_GMAC_NEW

/******* New GigE MAC registers *******/
#define NETA_GMAC_CTRL_0_REG(p)             (NETA_REG_BASE(p) + 0x2C00)

#define NETA_GMAC_PORT_EN_BIT               0
#define NETA_GMAC_PORT_EN_MASK              (1 << NETA_GMAC_PORT_EN_BIT)

#define NETA_GMAC_PORT_TYPE_BIT             1
#define NETA_GMAC_PORT_TYPE_MASK            (1 << NETA_GMAC_PORT_TYPE_BIT)
#define NETA_GMAC_PORT_TYPE_SGMII           (0 << NETA_GMAC_PORT_TYPE_BIT)
#define NETA_GMAC_PORT_TYPE_1000X           (1 << NETA_GMAC_PORT_TYPE_BIT)

#define NETA_GMAC_MAX_RX_SIZE_OFFS          2
#define NETA_GMAC_MAX_RX_SIZE_MASK          (0x1FFF << NETA_GMAC_MAX_RX_SIZE_OFFS)

#define NETA_GMAC_MIB_CNTR_EN_BIT           15
#define NETA_GMAC_MIB_CNTR_EN_MASK          (1 << NETA_GMAC_MIB_CNTR_EN_BIT)
/*-------------------------------------------------------------------------------*/

#define NETA_GMAC_CTRL_1_REG(p)             (NETA_REG_BASE(p) + 0x2C04)

#define NETA_GMAC_CTRL_2_REG(p)             (NETA_REG_BASE(p) + 0x2C08)

#define NETA_GMAC_PSC_ENABLE_BIT            3
#define NETA_GMAC_PSC_ENABLE_MASK           (1 << NETA_GMAC_PSC_ENABLE_BIT)

#define NETA_GMAC_PORT_RGMII_BIT            4
#define NETA_GMAC_PORT_RGMII_MASK           (1 << NETA_GMAC_PORT_RGMII_BIT)

#define NETA_GMAC_PORT_RESET_BIT            6
#define NETA_GMAC_PORT_RESET_MASK           (1 << NETA_GMAC_PORT_RESET_BIT)
/*-------------------------------------------------------------------------------*/

#define NETA_GMAC_AN_CTRL_REG(p)                (NETA_REG_BASE(p) + 0x2C0C)

#define NETA_FORCE_LINK_FAIL_BIT                0
#define NETA_FORCE_LINK_FAIL_MASK               (1 << NETA_FORCE_LINK_FAIL_BIT)

#define NETA_FORCE_LINK_PASS_BIT                1
#define NETA_FORCE_LINK_PASS_MASK               (1 << NETA_FORCE_LINK_PASS_BIT)

#define NETA_SET_MII_SPEED_100_BIT              5
#define NETA_SET_MII_SPEED_100_MASK             (1 << NETA_SET_MII_SPEED_100_BIT)

#define NETA_SET_GMII_SPEED_1000_BIT            6
#define NETA_SET_GMII_SPEED_1000_MASK           (1 << NETA_SET_GMII_SPEED_1000_BIT)

#define NETA_ENABLE_SPEED_AUTO_NEG_BIT          7
#define NETA_ENABLE_SPEED_AUTO_NEG_MASK         (1 << NETA_ENABLE_SPEED_AUTO_NEG_BIT)

#define NETA_SET_FLOW_CONTROL_BIT               8
#define NETA_SET_FLOW_CONTROL_MASK              (1 << NETA_SET_FLOW_CONTROL_BIT)

#define NETA_FLOW_CONTROL_ADVERTISE_BIT         9
#define NETA_FLOW_CONTROL_ADVERTISE_MASK        (1 << NETA_FLOW_CONTROL_ADVERTISE_BIT)

#define NETA_FLOW_CONTROL_ASYMETRIC_BIT         10
#define NETA_FLOW_CONTROL_ASYMETRIC_MASK        (1 << NETA_FLOW_CONTROL_ASYMETRIC_BIT)

#define NETA_ENABLE_FLOW_CONTROL_AUTO_NEG_BIT   11
#define NETA_ENABLE_FLOW_CONTROL_AUTO_NEG_MASK  (1 << NETA_ENABLE_FLOW_CONTROL_AUTO_NEG_BIT)

#define NETA_SET_FULL_DUPLEX_BIT                12
#define NETA_SET_FULL_DUPLEX_MASK               (1 << NETA_SET_FULL_DUPLEX_BIT)

#define NETA_ENABLE_DUPLEX_AUTO_NEG_BIT         13
#define NETA_ENABLE_DUPLEX_AUTO_NEG_MASK        (1 << NETA_ENABLE_DUPLEX_AUTO_NEG_BIT)

/*-------------------------------------------------------------------------------*/

#define NETA_GMAC_STATUS_REG(p)                 (NETA_REG_BASE(p) + 0x2C10)

#define NETA_GMAC_LINK_UP_BIT               0
#define NETA_GMAC_LINK_UP_MASK              (1 << NETA_GMAC_LINK_UP_BIT)

#define NETA_GMAC_SPEED_1000_BIT            1
#define NETA_GMAC_SPEED_1000_MASK           (1 << NETA_GMAC_SPEED_1000_BIT)

#define NETA_GMAC_SPEED_100_BIT             2
#define NETA_GMAC_SPEED_100_MASK            (1 << NETA_GMAC_SPEED_100_BIT)

#define NETA_GMAC_FULL_DUPLEX_BIT           3
#define NETA_GMAC_FULL_DUPLEX_MASK          (1 << NETA_GMAC_FULL_DUPLEX_BIT)

#define NETA_RX_FLOW_CTRL_ENABLE_BIT        4
#define NETA_RX_FLOW_CTRL_ENABLE_MASK       (1 << NETA_RX_FLOW_CTRL_ENABLE_BIT)

#define NETA_TX_FLOW_CTRL_ENABLE_BIT        5
#define NETA_TX_FLOW_CTRL_ENABLE_MASK       (1 << NETA_TX_FLOW_CTRL_ENABLE_BIT)

#define NETA_RX_FLOW_CTRL_ACTIVE_BIT        6
#define NETA_RX_FLOW_CTRL_ACTIVE_MASK       (1 << NETA_RX_FLOW_CTRL_ACTIVE_BIT)

#define NETA_TX_FLOW_CTRL_ACTIVE_BIT        7
#define NETA_TX_FLOW_CTRL_ACTIVE_MASK       (1 << NETA_TX_FLOW_CTRL_ACTIVE_BIT)
/*-------------------------------------------------------------------------------*/

#define NETA_GMAC_SERIAL_REG(p)             (NETA_REG_BASE(p) + 0x2C14)

#define NETA_GMAC_FIFO_PARAM_0_REG(p)       (NETA_REG_BASE(p) + 0x2C18)
#define NETA_GMAC_FIFO_PARAM_1_REG(p)       (NETA_REG_BASE(p) + 0x2C1C)

#define NETA_GMAC_CAUSE_REG(p)              (NETA_REG_BASE(p) + 0x2C20)
#define NETA_GMAC_MASK_REG(p)               (NETA_REG_BASE(p) + 0x2C24)

#define NETA_GMAC_SERDES_CFG_0_REG(p)       (NETA_REG_BASE(p) + 0x2C28)
#define NETA_GMAC_SERDES_CFG_1_REG(p)       (NETA_REG_BASE(p) + 0x2C2C)
#define NETA_GMAC_SERDES_CFG_2_REG(p)       (NETA_REG_BASE(p) + 0x2C30)
#define NETA_GMAC_SERDES_CFG_3_REG(p)       (NETA_REG_BASE(p) + 0x2C34)

#define NETA_GMAC_PRBS_STATUS_REG(p)        (NETA_REG_BASE(p) + 0x2C38)
#define NETA_GMAC_PRBS_ERR_CNTR_REG(p)      (NETA_REG_BASE(p) + 0x2C3C)

#define NETA_GMAC_STATUS_1_REG(p)           (NETA_REG_BASE(p) + 0x2C40)

#define NETA_GMAC_MIB_CTRL_REG(p)           (NETA_REG_BASE(p) + 0x2C44)
#define NETA_GMAC_CTRL_3_REG(p)             (NETA_REG_BASE(p) + 0x2C48)

#define NETA_GMAC_QSGMII_REG(p)             (NETA_REG_BASE(p) + 0x2C4C)
#define NETA_GMAC_QSGMII_STATUS_REG(p)      (NETA_REG_BASE(p) + 0x2C50)
#define NETA_GMAC_QSGMII_ERR_CNTR_REG(p)    (NETA_REG_BASE(p) + 0x2C54)

/* 8 FC Timer registers: 0x2c58 .. 0x2c74 */
#define NETA_GMAC_FC_TIMER_REG(p, r)        (NETA_REG_BASE(p) + 0x2C58 + ((r) << 2))

/* 4 DSA Tag registers: 0x2c78 .. 0x2c84 */
#define NETA_GMAC_DSA_TAG_REG(p, r)         (NETA_REG_BASE(p) + 0x2C78 + ((r) << 2))

#define NETA_GMAC_FC_WIN_0_REG(p)           (NETA_REG_BASE(p) + 0x2C88)
#define NETA_GMAC_FC_WIN_1_REG(p)           (NETA_REG_BASE(p) + 0x2C8C)

#define NETA_GMAC_CTRL_4_REG(p)             (NETA_REG_BASE(p) + 0x2C90)

#define NETA_GMAC_SERIAL_1_REG(p)           (NETA_REG_BASE(p) + 0x2C94)

#define NETA_LOW_POWER_CTRL_0_REG(p)        (NETA_REG_BASE(p) + 0x2CC0)

/* Low Power Idle Control #1 register */
#define NETA_LOW_POWER_CTRL_1_REG(p)        (NETA_REG_BASE(p) + 0x2CC4)

#define NETA_LPI_REQUEST_EN_BIT             0
#define NETA_LPI_REQUEST_EN_MASK            (1 << NETA_LPI_REQUEST_EN_BIT)

#define NETA_LPI_REQUEST_FORCE_BIT          1
#define NETA_LPI_REQUEST_FORCE_MASK         (1 << NETA_LPI_REQUEST_FORCE_BIT)

#define NETA_LPI_MANUAL_MODE_BIT            2
#define NETA_LPI_MANUAL_MODE_MASK           (1 << NETA_LPI_MANUAL_MODE_BIT)
/*-------------------------------------------------------------------------------*/

#define NETA_LOW_POWER_CTRL_2_REG(p)        (NETA_REG_BASE(p) + 0x2CC8)
#define NETA_LOW_POWER_STATUS_REG(p)        (NETA_REG_BASE(p) + 0x2CCC)
#define NETA_LOW_POWER_CNTR_REG(p)          (NETA_REG_BASE(p) + 0x2CD0)

#endif /* MV_ETH_GMAC_NEW */


#ifdef MV_PON_MIB_SUPPORT
/* Special registers for PON MIB support */
#define NETA_PON_MIB_MAX_GEM_PID            32
#define NETA_PON_MIB_RX_CTRL_REG(idx)       (MV_PON_REG_BASE + 0x3800 + ((idx) << 2))
#define NETA_PON_MIB_RX_DEF_REG             (MV_PON_REG_BASE + 0x3880)

#define NETA_PON_MIB_RX_GEM_PID_OFFS        0
#define NETA_PON_MIB_RX_GEM_PID_ALL_MASK    (0xFFF << NETA_PON_MIB_RX_GEM_PID_OFFS)
#define NETA_PON_MIB_RX_GEM_PID_MASK(pid)   ((pid) << NETA_PON_MIB_RX_GEM_PID_OFFS)

#define NETA_PON_MIB_RX_MIB_NO_OFFS         12
#define NETA_PON_MIB_RX_MIB_NO_MASK         (0x7 << NETA_PON_MIB_RX_MIB_NO_OFFS)
#define NETA_PON_MIB_RX_MIB_NO(mib)         ((mib) << NETA_PON_MIB_RX_MIB_NO_OFFS)

#define NETA_PON_MIB_RX_VALID_BIT           15
#define NETA_PON_MIB_RX_VALID_MASK          (1 << NETA_PON_MIB_RX_VALID_BIT)

#endif /* MV_PON_MIB_SUPPORT */

/******************************** NETA TX Registers *****************************/

#define NETA_TXQ_BASE_ADDR_REG(p, txp, q)   (NETA_TX_REG_BASE((p), (txp)) + 0x1800 + ((q) << 2))

#define NETA_TXQ_SIZE_REG(p, txp, q)        (NETA_TX_REG_BASE((p), (txp)) + 0x1820 + ((q) << 2))

#define NETA_TXQ_DESC_NUM_OFFS              0
#define NETA_TXQ_DESC_NUM_ALL_MASK          (0x3FFF << NETA_TXQ_DESC_NUM_OFFS)
#define NETA_TXQ_DESC_NUM_MASK(size)        ((size) << NETA_TXQ_DESC_NUM_OFFS)

#define NETA_TXQ_SENT_DESC_TRESH_OFFS       16
#define NETA_TXQ_SENT_DESC_TRESH_ALL_MASK   (0x3FFF << NETA_TXQ_SENT_DESC_TRESH_OFFS)
#define NETA_TXQ_SENT_DESC_TRESH_MASK(coal) ((coal) << NETA_TXQ_SENT_DESC_TRESH_OFFS)

#define NETA_TXQ_STATUS_REG(p, txp, q)      (NETA_TX_REG_BASE((p), (txp)) + 0x1840 + ((q) << 2))

#define NETA_TXQ_PENDING_DESC_OFFS          0
#define NETA_TXQ_PENDING_DESC_MASK          (0x3FFF << NETA_TXQ_PENDING_DESC_OFFS)

#define NETA_TXQ_SENT_DESC_OFFS             16
#define NETA_TXQ_SENT_DESC_MASK             (0x3FFF << NETA_TXQ_SENT_DESC_OFFS)

#define NETA_TXQ_UPDATE_REG(p, txp, q)      (NETA_TX_REG_BASE((p), (txp)) + 0x1860 + ((q) << 2))

#define NETA_TXQ_ADD_PENDING_OFFS           0
#define NETA_TXQ_ADD_PENDING_MASK           (0xFF << NETA_TXQ_ADD_PENDING_OFFS)

#define NETA_TXQ_DEC_SENT_OFFS              16
#define NETA_TXQ_DEC_SENT_MASK              (0xFF << NETA_TXQ_DEC_SENT_OFFS)

#define NETA_TXQ_INDEX_REG(p, txp, q)       (NETA_TX_REG_BASE((p), (txp)) + 0x1880 + ((q) << 2))

#define NETA_TXQ_NEXT_DESC_INDEX_OFFS       0
#define NETA_TXQ_NEXT_DESC_INDEX_MASK       (0x3FFF << NETA_TXQ_NEXT_DESC_INDEX_OFFS)

#define NETA_TXQ_SENT_DESC_REG(p, txp, q)   (NETA_TX_REG_BASE((p), (txp)) + 0x18A0 + ((q) << 2))
/* Use NETA_TXQ_SENT_DESC_OFFS and NETA_TXQ_SENT_DESC_MASK */

#ifdef MV_ETH_PMT_NEW
#define NETA_TX_BAD_FCS_CNTR_REG(p, txp)    (NETA_TX_REG_BASE((p), (txp)) + 0x18C0)
#define NETA_TX_DROP_CNTR_REG(p, txp)       (NETA_TX_REG_BASE((p), (txp)) + 0x18C4)
#endif /* MV_ETH_PMT_NEW */

#define NETA_PORT_TX_RESET_REG(p, txp)      (NETA_TX_REG_BASE((p), (txp)) + 0x18F0)

#define NETA_PORT_TX_DMA_RESET_BIT          0
#define NETA_PORT_TX_DMA_RESET_MASK         (1 << NETA_PORT_TX_DMA_RESET_BIT)

#ifdef MV_ETH_PMT_NEW

#define NETA_TX_ADD_BYTES_REG(p, txp)       (NETA_TX_REG_BASE((p), (txp)) + 0x18FC)

#define NETA_TX_NEW_BYTES_OFFS              0
#define NETA_TX_NEW_BYTES_ALL_MASK          (0xFFFF << NETA_TX_NEW_BYTES_OFFS)
#define NETA_TX_NEW_BYTES_MASK(bytes)       ((bytes) << NETA_TX_NEW_BYTES_OFFS)

#define NETA_TX_NEW_BYTES_TXQ_OFFS          28
#define NETA_TX_NEW_BYTES_TXQ_MASK(txq)     ((txq) << NETA_TX_NEW_BYTES_TXQ_OFFS)

#define NETA_TX_NEW_BYTES_COLOR_BIT         31
#define NETA_TX_NEW_BYTES_COLOR_GREEN       (0 << NETA_TX_NEW_BYTES_COLOR_BIT)
#define NETA_TX_NEW_BYTES_COLOR_YELLOW      (1 << NETA_TX_NEW_BYTES_COLOR_BIT)
/*-------------------------------------------------------------------------------*/

#define NETA_TXQ_NEW_BYTES_REG(p, txp, txq) (NETA_TX_REG_BASE((p), (txp)) + 0x1900 + ((txq) << 2))

#define NETA_TX_MAX_MH_REGS                 15
#define NETA_TX_MH_REG(p, txp, idx)         (NETA_TX_REG_BASE((p), (txp)) + 0x1944 + ((idx) << 2))

/*************** Packet Modification Registers *******************/
#define NETA_TX_PMT_ACCESS_REG(p)           (NETA_TX_REG_BASE((p), 0) + 0x1980)
#define NETA_TX_PMT_FIFO_THRESH_REG(p)      (NETA_TX_REG_BASE((p), 0) + 0x1984)
#define NETA_TX_PMT_MTU_REG(p)              (NETA_TX_REG_BASE((p), 0) + 0x1988)

#define NETA_TX_PMT_MAX_ETHER_TYPES         4
#define NETA_TX_PMT_ETHER_TYPE_REG(p, i)    (NETA_TX_REG_BASE((p), 0) + 0x1990 + ((i) << 2))

#define NETA_TX_PMT_DEF_VLAN_CFG_REG(p)     (NETA_TX_REG_BASE((p), 0) + 0x19a0)
#define NETA_TX_PMT_DEF_DSA_1_CFG_REG(p)    (NETA_TX_REG_BASE((p), 0) + 0x19a4)
#define NETA_TX_PMT_DEF_DSA_2_CFG_REG(p)    (NETA_TX_REG_BASE((p), 0) + 0x19a8)
#define NETA_TX_PMT_DEF_DSA_SRC_DEV_REG(p)  (NETA_TX_REG_BASE((p), 0) + 0x19ac)

#define NETA_TX_PMT_TTL_ZERO_FRWD_REG(p)    (NETA_TX_REG_BASE((p), 0) + 0x19b0)
#define NETA_TX_PMT_TTL_ZERO_CNTR_REG(p)    (NETA_TX_REG_BASE((p), 0) + 0x19b4)

#define NETA_TX_PMT_PPPOE_TYPE_REG(p)       (NETA_TX_REG_BASE((p), 0) + 0x19c0)
#define NETA_TX_PMT_PPPOE_DATA_REG(p)       (NETA_TX_REG_BASE((p), 0) + 0x19c4)
#define NETA_TX_PMT_PPPOE_LEN_REG(p)        (NETA_TX_REG_BASE((p), 0) + 0x19c8)
#define NETA_TX_PMT_PPPOE_PROTO_REG(p)      (NETA_TX_REG_BASE((p), 0) + 0x19cc)

#define NETA_TX_PMT_CONFIG_REG(p)           (NETA_TX_REG_BASE((p), 0) + 0x19d0)
#define NETA_TX_PMT_STATUS_1_REG(p)         (NETA_TX_REG_BASE((p), 0) + 0x19d4)
#define NETA_TX_PMT_STATUS_2_REG(p)         (NETA_TX_REG_BASE((p), 0) + 0x19d8)

#else

#define NETA_TX_COLOR_ADD_BYTES_REG(p, txp) (NETA_TX_REG_BASE((p), (txp)) + 0x1900)
#define NETA_TX_GREEN_BYTES_REG(p, txp)     (NETA_TX_REG_BASE((p), (txp)) + 0x1908)
#define NETA_TX_YELLOW_BYTES_REG(p, txp)    (NETA_TX_REG_BASE((p), (txp)) + 0x190c)

#define NETA_TX_MAX_MH_REGS                 6
#define NETA_TX_MH_REG(p, txp, idx)         (NETA_TX_REG_BASE((p), (txp)) + 0x1910 + ((idx) << 2))

#define NETA_TX_DSA_SRC_DEV_REG(p, txp)     (NETA_TX_REG_BASE((p), (txp)) + 0x192C)

#define NETA_TX_MAX_ETH_TYPE_REGS           4
#define NETA_TX_ETH_TYPE_REG(p, txp, idx)   (NETA_TX_REG_BASE((p), (txp)) + 0x1930 + ((idx) << 2))

#define NETA_TX_PMT_SIZE		256
#define NETA_TX_PMT_W0_MASK		0xFFFF
#define NETA_TX_PMT_W1_MASK		0xFFFFFFFF
#define NETA_TX_PMT_W2_MASK		0x7FFFFFF
#define NETA_TX_PMT_REG(p)		(NETA_TX_REG_BASE(p, 0) + 0x1940)
#define NETA_TX_PMT_W0_REG(p)		(NETA_TX_REG_BASE(p, 0) + 0x1944)
#define NETA_TX_PMT_W1_REG(p)		(NETA_TX_REG_BASE(p, 0) + 0x1948)
#define NETA_TX_PMT_W2_REG(p)		(NETA_TX_REG_BASE(p, 0) + 0x194c)

#endif /* MV_ETH_PMT_NEW */

/*********************** New TX WRR EJP Registers ********************************/

#define NETA_TX_CMD_1_REG(p, txp)           (NETA_TX_REG_BASE((p), (txp)) + 0x1a00)

#define NETA_TX_EJP_RESET_BIT               0
#define NETA_TX_EJP_RESET_MASK              (1 << NETA_TX_EJP_RESET_BIT)

#define NETA_TX_PTP_SYNC_BIT                1
#define NETA_TX_PTP_SYNC_MASK               (1 << NETA_TX_EJP_RESET_BIT)

#define NETA_TX_EJP_ENABLE_BIT              2
#define NETA_TX_EJP_ENABLE_MASK             (1 << NETA_TX_EJP_ENABLE_BIT)

#define NETA_TX_LEGACY_WRR_BIT              3
#define NETA_TX_LEGACY_WRR_MASK             (1 << NETA_TX_LEGACY_WRR_BIT)
/*-----------------------------------------------------------------------------------------------*/

/* Transmit Queue Fixed Priority Configuration (TQFPC) */
#define NETA_TX_FIXED_PRIO_CFG_REG(p, txp)  (NETA_TX_REG_BASE((p), (txp)) + 0x1a04)

#define NETA_TX_FIXED_PRIO_OFFS             0
#define NETA_TX_FIXED_PRIO_MASK             (0xFF << NETA_TX_FIXED_PRIO_OFFS)
/*-----------------------------------------------------------------------------------------------*/

/* Basic Refill No of Clocks (BRC) */
#define NETA_TX_REFILL_PERIOD_REG(p, txp)   (NETA_TX_REG_BASE((p), (txp)) + 0x1a08)

#define NETA_TX_REFILL_CLOCKS_OFFS          0
#define NETA_TX_REFILL_CLOCKS_MIN           16
#define NETA_TX_REFILL_CLOCKS_MASK          (0xFFFF << NETA_TX_REFILL_RATE_CLOCKS_MASK)
/*-----------------------------------------------------------------------------------------------*/

/* Port Maximum Transmit Unit (PMTU) */
#define NETA_TXP_MTU_REG(p, txp)            (NETA_TX_REG_BASE((p), (txp)) + 0x1a0c)

#define NETA_TXP_MTU_OFFS                   0
#define NETA_TXP_MTU_MAX                    0x3FFFF
#define NETA_TXP_MTU_ALL_MASK               (NETA_TXP_MTU_MAX << NETA_TXP_MTU_OFFS)
#define NETA_TXP_MTU_MASK(mtu)              ((mtu) << NETA_TXP_MTU_OFFS)
/*-----------------------------------------------------------------------------------------------*/

/* Port Bucket Refill (PRefill) */
#define NETA_TXP_REFILL_REG(p, txp)         (NETA_TX_REG_BASE((p), (txp)) + 0x1a10)

#define NETA_TXP_REFILL_TOKENS_OFFS         0
#define NETA_TXP_REFILL_TOKENS_MAX          0x7FFFF
#define NETA_TXP_REFILL_TOKENS_ALL_MASK     (NETA_TXP_REFILL_TOKENS_MAX << NETA_TXP_REFILL_TOKENS_OFFS)
#define NETA_TXP_REFILL_TOKENS_MASK(val)    ((val) << NETA_TXP_REFILL_TOKENS_OFFS)

#define NETA_TXP_REFILL_PERIOD_OFFS         20
#define NETA_TXP_REFILL_PERIOD_MAX          0x3FF
#define NETA_TXP_REFILL_PERIOD_ALL_MASK     (NETA_TXP_REFILL_PERIOD_MAX << NETA_TXP_REFILL_PERIOD_OFFS)
#define NETA_TXP_REFILL_PERIOD_MASK(val)    ((val) << NETA_TXP_REFILL_PERIOD_OFFS)
/*-----------------------------------------------------------------------------------------------*/

/* Port Maximum Token Bucket Size (PMTBS) */
#define NETA_TXP_TOKEN_SIZE_REG(p, txp)     (NETA_TX_REG_BASE((p), (txp)) + 0x1a14)
#define NETA_TXP_TOKEN_SIZE_MAX             0xFFFFFFFF
/*-----------------------------------------------------------------------------------------------*/

/* Port Token Bucket Counter (PMTBS) */
#define NETA_TXP_TOKEN_CNTR_REG(p, txp)     (NETA_TX_REG_BASE((p), (txp)) + 0x1a18)
#define NETA_TXP_TOKEN_CNTR_MAX             0xFFFFFFFF
/*-----------------------------------------------------------------------------------------------*/

/* Queue Bucket Refill (QRefill) */
#define NETA_TXQ_REFILL_REG(p, txp, q)      (NETA_TX_REG_BASE((p), (txp)) + 0x1a20 + ((q) << 2))

#define NETA_TXQ_REFILL_TOKENS_OFFS         0
#define NETA_TXQ_REFILL_TOKENS_MAX          0x7FFFF
#define NETA_TXQ_REFILL_TOKENS_ALL_MASK     (NETA_TXQ_REFILL_TOKENS_MAX << NETA_TXQ_REFILL_TOKENS_OFFS)
#define NETA_TXQ_REFILL_TOKENS_MASK(val)    ((val) << NETA_TXQ_REFILL_TOKENS_OFFS)

#define NETA_TXQ_REFILL_PERIOD_OFFS         20
#define NETA_TXQ_REFILL_PERIOD_MAX          0x3FF
#define NETA_TXQ_REFILL_PERIOD_ALL_MASK     (NETA_TXQ_REFILL_PERIOD_MAX << NETA_TXQ_REFILL_PERIOD_OFFS)
#define NETA_TXQ_REFILL_PERIOD_MASK(val)    ((val) << NETA_TXQ_REFILL_PERIOD_OFFS)
/*-----------------------------------------------------------------------------------------------*/

/* Queue Maximum Token Bucket Size (QMTBS) */
#define NETA_TXQ_TOKEN_SIZE_REG(p, txp, q)  (NETA_TX_REG_BASE((p), (txp)) + 0x1a40 + ((q) << 2))
#define NETA_TXQ_TOKEN_SIZE_MAX             0x7FFFFFFF
/*-----------------------------------------------------------------------------------------------*/

/* Queue Token Bucket Counter (PMTBS) */
#define NETA_TXQ_TOKEN_CNTR_REG(p, txp, q)  (NETA_TX_REG_BASE((p), (txp)) + 0x1a60 + ((q) << 2))
#define NETA_TXQ_TOKEN_CNTR_MAX             0xFFFFFFFF
/*-----------------------------------------------------------------------------------------------*/

/* Transmit Queue Arbiter Configuration (TQxAC) */
#define NETA_TXQ_WRR_ARBITER_REG(p, txp, q) (NETA_TX_REG_BASE((p), (txp)) + 0x1a80 + ((q) << 2))

#define NETA_TXQ_WRR_WEIGHT_OFFS            0
#define NETA_TXQ_WRR_WEIGHT_MAX             0xFF
#define NETA_TXQ_WRR_WEIGHT_ALL_MASK        (NETA_TXQ_WRR_WEIGHT_MAX << NETA_TXQ_WRR_WEIGHT_OFFS)
#define NETA_TXQ_WRR_WEIGHT_MASK(weigth)    ((weigth) << NETA_TXQ_WRR_WEIGHT_OFFS)

#define NETA_TXQ_WRR_BYTE_COUNT_OFFS        8
#define NETA_TXQ_WRR_BYTE_COUNT_MASK        (0x3FFFF << NETA_TXQ_WRR_BYTE_COUNT_OFFS)
/*-----------------------------------------------------------------------------------------------*/

/* Transmission Queue IPG (TQxIPG) */
#define NETA_TXQ_EJP_IPG_REG(p, txp, q)     (NETA_TX_REG_BASE((p), (txp)) + 0x1aa0 + ((q) << 2))

#define NETA_TXQ_EJP_IPG_OFFS               0
#define NETA_TXQ_EJP_IPG_MASK               (0x3FFF << NETA_TXQ_EJP_IPG_OFFS)
/*-----------------------------------------------------------------------------------------------*/

#define NETA_TXP_EJP_HI_LO_REG(p, txp)      (NETA_TX_REG_BASE((p), (txp)) + 0x1ab0)
#define NETA_TXP_EJP_HI_ASYNC_REG(p, txp)   (NETA_TX_REG_BASE((p), (txp)) + 0x1ab4)
#define NETA_TXP_EJP_LO_ASYNC_REG(p, txp)   (NETA_TX_REG_BASE((p), (txp)) + 0x1ab8)
#define NETA_TXP_EJP_SPEED_REG(p, txp)      (NETA_TX_REG_BASE((p), (txp)) + 0x1abc)
/*-----------------------------------------------------------------------------------------------*/

/******************** NETA RX EXTENDED DESCRIPTOR ********************************/

#define NETA_DESC_ALIGNED_SIZE	            32

#if defined(MV_CPU_BE) && !defined(CONFIG_MV_ETH_BE_WA)

typedef struct neta_rx_desc {
	MV_U16  dataSize;
	MV_U16  pncInfo;
	MV_U32  status;
	MV_U32  pncFlowId;
	MV_U32  bufPhysAddr;
	MV_U16  csumL4;
	MV_U16  prefetchCmd;
	MV_U32  bufCookie;
	MV_U32  hw_cmd;
	MV_U32  pncExtra;
} NETA_RX_DESC;

#else

typedef struct neta_rx_desc {
	MV_U32  status;
	MV_U16  pncInfo;
	MV_U16  dataSize;
	MV_U32  bufPhysAddr;
	MV_U32  pncFlowId;
	MV_U32  bufCookie;
	MV_U16  prefetchCmd;
	MV_U16  csumL4;
	MV_U32  pncExtra;
	MV_U32  hw_cmd;
} NETA_RX_DESC;

#endif /* MV_CPU_BE && !CONFIG_MV_ETH_BE_WA */

/* "status" word fileds definition */
#define NETA_RX_L3_OFFSET_OFFS              0
#define NETA_RX_L3_OFFSET_MASK              (0x7F << NETA_RX_L3_OFFSET_OFFS)

#define NETA_RX_IP_HLEN_OFFS                8
#define NETA_RX_IP_HLEN_MASK                (0x1F << NETA_RX_IP_HLEN_OFFS)

#define NETA_RX_BM_POOL_ID_OFFS             13
#define NETA_RX_BM_POOL_ALL_MASK            (0x3 << NETA_RX_BM_POOL_ID_OFFS)
#define NETA_RX_BM_POOL_ID_MASK(pool)       ((pool) << NETA_RX_BM_POOL_ID_OFFS)

#define NETA_RX_ES_BIT                      16
#define NETA_RX_ES_MASK                     (1 << NETA_RX_ES_BIT)

#define NETA_RX_ERR_CODE_OFFS               17
#define NETA_RX_ERR_CODE_MASK               (3 << NETA_RX_ERR_CODE_OFFS)
#define NETA_RX_ERR_CRC                     (0 << NETA_RX_ERR_CODE_OFFS)
#define NETA_RX_ERR_OVERRUN                 (1 << NETA_RX_ERR_CODE_OFFS)
#define NETA_RX_ERR_LEN                     (2 << NETA_RX_ERR_CODE_OFFS)
#define NETA_RX_ERR_RESOURCE                (3 << NETA_RX_ERR_CODE_OFFS)

#define NETA_RX_F_DESC_BIT                  26
#define NETA_RX_F_DESC_MASK                 (1 << NETA_RX_F_DESC_BIT)

#define NETA_RX_L_DESC_BIT                  27
#define NETA_RX_L_DESC_MASK                 (1 << NETA_RX_L_DESC_BIT)

#define NETA_RX_L4_CSUM_OK_BIT              30
#define NETA_RX_L4_CSUM_OK_MASK             (1 << NETA_RX_L4_CSUM_OK_BIT)

#define NETA_RX_IP4_FRAG_BIT                31
#define NETA_RX_IP4_FRAG_MASK               (1 << NETA_RX_IP4_FRAG_BIT)

#ifdef CONFIG_MV_ETH_PNC

#define NETA_RX_L3_OFFS                     24
#define NETA_RX_L3_MASK                     (3 << NETA_RX_L3_OFFS)
#define NETA_RX_L3_UN                       (0 << NETA_RX_L3_OFFS)
#define NETA_RX_L3_IP6                      (1 << NETA_RX_L3_OFFS)
#define NETA_RX_L3_IP4                      (2 << NETA_RX_L3_OFFS)
#define NETA_RX_L3_IP4_ERR                  (3 << NETA_RX_L3_OFFS)

#define NETA_RX_L4_OFFS                     28
#define NETA_RX_L4_MASK                     (3 << NETA_RX_L4_OFFS)
#define NETA_RX_L4_TCP                      (0 << NETA_RX_L4_OFFS)
#define NETA_RX_L4_UDP                      (1 << NETA_RX_L4_OFFS)
#define NETA_RX_L4_OTHER                    (2 << NETA_RX_L4_OFFS)

/* Bits of "pncExtra" field */
#define NETA_RX_PNC_ENABLED_BIT             0
#define NETA_RX_PNC_ENABLED_MASK            (1 << NETA_RX_PNC_ENABLED_BIT)

#define NETA_RX_PNC_LOOPS_OFFS              1
#define NETA_RX_PNC_LOOPS_MASK              (0xF << NETA_RX_PNC_LOOPS_OFFS)

#define NETA_PNC_STATUS_OFFS                5
#define NETA_PNC_STATUS_MASK                (3 << NETA_PNC_STATUS_OFFS)

#define NETA_PNC_RI_EXTRA_OFFS              16
#define NETA_PNC_RI_EXTRA_MASK              (0xFFF << NETA_PNC_RI_EXTRA_OFFS)
/*---------------------------------------------------------------------------*/

#else

#define ETH_RX_VLAN_TAGGED_FRAME_BIT        19
#define ETH_RX_VLAN_TAGGED_FRAME_MASK       (1 << ETH_RX_VLAN_TAGGED_FRAME_BIT)

#define ETH_RX_BPDU_FRAME_BIT               20
#define ETH_RX_BPDU_FRAME_MASK              (1 << ETH_RX_BPDU_FRAME_BIT)

#define ETH_RX_L4_TYPE_OFFSET               21
#define ETH_RX_L4_TYPE_MASK                 (3 << ETH_RX_L4_TYPE_OFFSET)
#define ETH_RX_L4_TCP_TYPE                  (0 << ETH_RX_L4_TYPE_OFFSET)
#define ETH_RX_L4_UDP_TYPE                  (1 << ETH_RX_L4_TYPE_OFFSET)
#define ETH_RX_L4_OTHER_TYPE                (2 << ETH_RX_L4_TYPE_OFFSET)

#define ETH_RX_NOT_LLC_SNAP_FORMAT_BIT      23
#define ETH_RX_NOT_LLC_SNAP_FORMAT_MASK     (1 << ETH_RX_NOT_LLC_SNAP_FORMAT_BIT)

#ifdef MV_ETH_LEGACY_PARSER_IPV6

#define NETA_RX_L3_OFFS                     24
#define NETA_RX_L3_MASK                     (3 << NETA_RX_L3_OFFS)
#define NETA_RX_L3_UN                       (0 << NETA_RX_L3_OFFS)
#define NETA_RX_L3_IP6                      (2 << NETA_RX_L3_OFFS)
#define NETA_RX_L3_IP4                      (3 << NETA_RX_L3_OFFS)
#define NETA_RX_L3_IP4_ERR                  (1 << NETA_RX_L3_OFFS)

#else

#define ETH_RX_IP_FRAME_TYPE_BIT            24
#define ETH_RX_IP_FRAME_TYPE_MASK           (1 << ETH_RX_IP_FRAME_TYPE_BIT)

#define ETH_RX_IP_HEADER_OK_BIT             25
#define ETH_RX_IP_HEADER_OK_MASK            (1 << ETH_RX_IP_HEADER_OK_BIT)

#endif /* MV_ETH_LEGACY_PARSER_IPV6 */

#define ETH_RX_UNKNOWN_DA_BIT               28
#define ETH_RX_UNKNOWN_DA_MASK              (1 << ETH_RX_UNKNOWN_DA_BIT)

#endif /* CONFIG_MV_ETH_PNC */
/*---------------------------------------------------------------------------*/


/* Bit map of "hw_cmd" field */
#define NETA_RX_COLOR_BIT                   3
#define NETA_RX_COLOR_MASK                  (1 << NETA_RX_COLOR_BIT)
#define NETA_RX_COLOR_GREEN                 (0 << NETA_RX_COLOR_BIT)
#define NETA_RX_COLOR_YELLOW                (1 << NETA_RX_COLOR_BIT)

#define NETA_RX_DSA_OFFS                    4
#define NETA_RX_DSA_MASK                    (3 << NETA_RX_DSA_OFFS)
#define NETA_RX_DSA_NONE                    (0 << NETA_RX_DSA_OFFS)
#define NETA_RX_DSA                         (1 << NETA_RX_DSA_OFFS)
#define NETA_RX_DSA_E                       (2 << NETA_RX_DSA_OFFS)

#define NETA_RX_GEM_PID_OFFS                8
#define NETA_RX_GEM_PID_MASK                (0xFFF << NETA_RX_GEM_PID_OFFS)
/*---------------------------------------------------------------------------*/


/******************** NETA TX EXTENDED DESCRIPTOR ********************************/

#if defined(MV_CPU_BE) && !defined(CONFIG_MV_ETH_BE_WA)

typedef struct neta_tx_desc {
	MV_U16  dataSize;
	MV_U16  csumL4;
	MV_U32  command;
	MV_U32  hw_cmd;
	MV_U32  bufPhysAddr;
	MV_U32  reserved[4];
} NETA_TX_DESC;

#else

typedef struct neta_tx_desc {
	MV_U32  command;
	MV_U16  csumL4;
	MV_U16  dataSize;
	MV_U32  bufPhysAddr;
	MV_U32  hw_cmd;
	MV_U32  reserved[4];
} NETA_TX_DESC;

#endif /* MV_CPU_BE && !CONFIG_MV_ETH_BE_WA */

/* "command" word fileds definition */
#define NETA_TX_L3_OFFSET_OFFS              0
#define NETA_TX_L3_OFFSET_MASK              (0x7F << NETA_TX_L3_OFFSET_OFFS)

#define NETA_TX_GEM_OEM_BIT                 7
#define NETA_TX_GEM_OEM_MASK                (1 << NETA_TX_GEM_OEM_BIT)

#define NETA_TX_IP_HLEN_OFFS                8
#define NETA_TX_IP_HLEN_MASK                (0x1F << NETA_TX_IP_HLEN_OFFS)

#define NETA_TX_BM_POOL_ID_OFFS             13
#define NETA_TX_BM_POOL_ID_ALL_MASK         (0x3 << NETA_TX_BM_POOL_ID_OFFS)
#define NETA_TX_BM_POOL_ID_MASK(pool)       ((pool) << NETA_TX_BM_POOL_ID_OFFS)

#define NETA_TX_HWF_BIT                     15
#define NETA_TX_HWF_MASK                    (1 << NETA_TX_HWF_BIT)

#define NETA_TX_L4_BIT                      16
#define NETA_TX_L4_TCP                      (0 << NETA_TX_L4_BIT)
#define NETA_TX_L4_UDP                      (1 << NETA_TX_L4_BIT)

#define NETA_TX_L3_BIT                      17
#define NETA_TX_L3_IP4                      (0 << NETA_TX_L3_BIT)
#define NETA_TX_L3_IP6                      (1 << NETA_TX_L3_BIT)

#define NETA_TX_IP_CSUM_BIT                 18
#define NETA_TX_IP_CSUM_MASK                (1 << NETA_TX_IP_CSUM_BIT)

#define NETA_TX_Z_PAD_BIT                   19
#define NETA_TX_Z_PAD_MASK                  (1 << NETA_TX_Z_PAD_BIT)

#define NETA_TX_L_DESC_BIT                  20
#define NETA_TX_L_DESC_MASK                 (1 << NETA_TX_L_DESC_BIT)

#define NETA_TX_F_DESC_BIT                  21
#define NETA_TX_F_DESC_MASK                 (1 << NETA_TX_F_DESC_BIT)

#define NETA_TX_BM_ENABLE_BIT               22
#define NETA_TX_BM_ENABLE_MASK              (1 << NETA_TX_BM_ENABLE_BIT)


#define NETA_TX_PKT_OFFSET_OFFS             23
#define NETA_TX_PKT_OFFSET_MAX				0x7F
#define NETA_TX_PKT_OFFSET_ALL_MASK         (NETA_TX_PKT_OFFSET_MAX << NETA_TX_PKT_OFFSET_OFFS)
#define NETA_TX_PKT_OFFSET_MASK(offset)     (((offset) << NETA_TX_PKT_OFFSET_OFFS) & NETA_TX_PKT_OFFSET_ALL_MASK)

#define NETA_TX_L4_CSUM_BIT                 30
#define NETA_TX_L4_CSUM_MASK                (3 << NETA_TX_L4_CSUM_BIT)
#define NETA_TX_L4_CSUM_PART                (0 << NETA_TX_L4_CSUM_BIT)
#define NETA_TX_L4_CSUM_FULL                (1 << NETA_TX_L4_CSUM_BIT)
#define NETA_TX_L4_CSUM_NOT                 (2 << NETA_TX_L4_CSUM_BIT)

#define NETA_TX_FLZ_DESC_MASK               (NETA_TX_F_DESC_MASK | NETA_TX_L_DESC_MASK | NETA_TX_Z_PAD_MASK)
/*-------------------------------------------------------------------------------*/

/* "hw_cmd" field definition */
#define NETA_TX_ES_BIT                      0
#define NETA_TX_ES_MASK                     (1 << NETA_TX_ES_BIT)

#define NETA_TX_ERR_CODE_OFFS               1
#define NETA_TX_ERR_CODE_MASK               (3 << NETA_TX_ERR_CODE_OFFS)
#define NETA_TX_ERR_LATE_COLLISION          (0 << NETA_TX_ERR_CODE_OFFS)
#define NETA_TX_ERR_UNDERRUN                (1 << NETA_TX_ERR_CODE_OFFS)
#define NETA_TX_ERR_EXCE_COLLISION          (2 << NETA_RX_ERR_CODE_OFFS)

#define NETA_TX_COLOR_BIT                   3
#define NETA_TX_COLOR_GREEN                 (0 << NETA_TX_COLOR_BIT)
#define NETA_TX_COLOR_YELLOW                (1 << NETA_TX_COLOR_BIT)

#define NETA_TX_MH_SEL_OFFS                 4

#ifdef MV_ETH_PMT_NEW
#define NETA_TX_MH_SEL_MASK                 (0xF << NETA_TX_MH_SEL_OFFS)
#else
#define NETA_TX_MH_SEL_MASK                 (0x7 << NETA_TX_MH_SEL_OFFS)
#endif /* MV_ETH_PMT_NEW */

#define NETA_TX_MH_UNCHANGE                 (0 << NETA_TX_MH_SEL_OFFS)

#define NETA_TX_GEM_PID_OFFS                8
#define NETA_TX_GEM_PID_MASK                (0xFFF << NETA_TX_GEM_PID_OFFS)

#define NETA_TX_MOD_CMD_OFFS                20
#define NETA_TX_MOD_CMD_MASK                (0x3FF << NETA_TX_MOD_CMD_OFFS)

#define NETA_TX_DSA_OFFS                    30
#define NETA_TX_DSA_MASK                    (3 << NETA_TX_DSA_OFFS)
#define NETA_TX_DSA_NONE                    (0 << NETA_TX_DSA_OFFS)
#define NETA_TX_DSA                         (1 << NETA_TX_DSA_OFFS) /* normal dsa */
#define NETA_TX_DSA_E                       (2 << NETA_TX_DSA_OFFS) /* extended dsa */
/*-------------------------------------------------------------------------------*/


#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __mvNetaRegs_h__ */
