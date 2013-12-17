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

#ifndef __MV_PP2_GBE_REGS_H__
#define __MV_PP2_GBE_REGS_H__

#ifdef CONFIG_ARCH_MVEBU
#include "mvNetConfig.h"
#else
#include "mvSysEthConfig.h"
#endif

/************************** PPv2 HW Configuration ***********************/

/************************** TX General Registers ******************************/
#define MV_PP2_TX_SNOOP_REG			(MV_PP2_REG_BASE + 0x8800)
#define MV_PP2_TX_FIFO_THRESH_REG		(MV_PP2_REG_BASE + 0x8804)

/* Indirect access */
#define MV_PP2_TX_PKT_LEN_IDX_REG		(MV_PP2_REG_BASE + 0x8808)
#define MV_PP2_TX_PKT_LEN_CHANGE_REG		(MV_PP2_REG_BASE + 0x880C)

#define MV_PP2_TX_PORT_FLUSH_REG		(MV_PP2_REG_BASE + 0x8810)

#define MV_PP2_TX_PORT_FLUSH_OFFS		0
#define MV_PP2_TX_PORT_FLUSH_BITS		7
#define MV_PP2_TX_PORT_FLUSH_ALL_MASK		(((1 << MV_PP2_TX_PORT_FLUSH_BITS) - 1) << MV_PP2_TX_PORT_FLUSH_OFFS)
#define MV_PP2_TX_PORT_FLUSH_MASK(p)		((1 << (p)) << MV_PP2_TX_PORT_FLUSH_OFFS)

/* Registers per egress port */
#define MV_PP2_TXP_BAD_CRC_CNTR_REG(txp)	(MV_PP2_REG_BASE + 0x8900)
#define MV_PP2_TXP_DROP_CNTR_REG(txp)		(MV_PP2_REG_BASE + 0x8980)
#define MV_PP2_TXP_DEQUEUE_THRESH_REG(txp)	(MV_PP2_REG_BASE + 0x88A0)

/************************** RX Fifo Registers ******************************/
#define MV_PP2_RX_DATA_FIFO_SIZE_REG(port)	(MV_PP2_REG_BASE + 0x00 + 4 * (port))
#define MV_PP2_RX_ATTR_FIFO_SIZE_REG(port)	(MV_PP2_REG_BASE + 0x20 + 4 * (port))
#define MV_PP2_RX_MIN_PKT_SIZE_REG		(MV_PP2_REG_BASE + 0x60)
#define MV_PP2_RX_FIFO_INIT_REG			(MV_PP2_REG_BASE + 0x64)

/************************** Top Reg file ******************************/
#define MV_PP2_MH_REG(port)			(MV_PP2_REG_BASE + 0x5040 + 4 * (port))

#define MV_PP2_MH_EN_OFFS			0
#define MV_PP2_MH_EN_MASK			(1 << MV_PP2_MH_EN_OFFS)

#define MV_PP2_DSA_EN_OFFS			0
#define MV_PP2_DSA_EN_MASK			(0x3 << MV_PP2_DSA_EN_OFFS)
#define MV_PP2_DSA_DISABLE			0
#define MV_PP2_DSA_NON_EXTENDED			(0x1 << MV_PP2_DSA_EN_OFFS)
#define MV_PP2_DSA_EXTENDED			(0x2 << MV_PP2_DSA_EN_OFFS)

/************************** RX DMA Top Registers ******************************/
#define MV_PP2_RX_CTRL_REG(port)		(MV_PP2_REG_BASE + 0x140 + 4 * (port))

#define MV_PP2_POOL_BUF_SIZE_REG(pool)		(MV_PP2_REG_BASE + 0x180 + 4 * (pool))

#define MV_PP2_POOL_BUF_SIZE_OFFSET		5
#define MV_PP2_POOL_BUF_SIZE_MASK		(0xFFFE)
/*-------------------------------------------------------------------------------*/

#ifdef CONFIG_MV_ETH_PP2_1 /* PPv2.1 - A0 */

#define MV_PP2_RX_STATUS			(MV_PP2_REG_BASE + 0x174)

#define MV_PP2_DISABLE_IN_PROG_OFFS		0
#define MV_PP2_DISABLE_IN_PROG_MASK		(0x1 << MV_PP2_DISABLE_IN_PROG_OFFS)
/*-------------------------------------------------------------------------------*/

#define MV_PP2_RXQ_CONFIG_REG(rxq)		(MV_PP2_REG_BASE + 0x800 + 4 * (rxq))

#define MV_PP2_SNOOP_PKT_SIZE_OFFS		0
#define MV_PP2_SNOOP_PKT_SIZE_MASK		(0x1FF << MV_PP2_SNOOP_PKT_SIZE_OFFS)

#define MV_PP2_SNOOP_BUF_HDR_OFFS		9
#define MV_PP2_SNOOP_BUF_HDR_MASK		(0x1 << MV_PP2_SNOOP_BUF_HDR_OFFS)

#define MV_PP2_L2_DEPOSIT_PKT_SIZE_OFFS		12
#define MV_PP2_L2_DEPOSIT_PKT_SIZE_MASK		(0xF << MV_PP2_L2_DEPOSIT_PKT_SIZE_OFFS)

#define MV_PP2_L2_DEPOSIT_BUF_HDR_OFFS		16
#define MV_PP2_L2_DEPOSIT_BUF_HDR_MASK		(0x1 << MV_PP2_L2_DEPOSIT_BUF_HDR_OFFS)

#define MV_PP2_RXQ_POOL_SHORT_OFFS		20
#define MV_PP2_RXQ_POOL_SHORT_MASK		(0x7 << MV_PP2_RXQ_POOL_SHORT_OFFS)

#define MV_PP2_RXQ_POOL_LONG_OFFS		24
#define MV_PP2_RXQ_POOL_LONG_MASK		(0x7 << MV_PP2_RXQ_POOL_LONG_OFFS)

#define MV_PP2_RXQ_PACKET_OFFSET_OFFS		28
#define MV_PP2_RXQ_PACKET_OFFSET_MASK		(0x7 << MV_PP2_RXQ_PACKET_OFFSET_OFFS)

#define MV_PP2_RXQ_DISABLE_BIT			31
#define MV_PP2_RXQ_DISABLE_MASK			(0x1 << MV_PP2_RXQ_DISABLE_BIT)
/*-------------------------------------------------------------------------------*/

#define MV_PP2_HWF_TXQ_CONFIG_REG(txq)		(MV_PP2_REG_BASE + 0xc00 + 4 * (txq))

#define MV_PP2_HWF_TXQ_POOL_SHORT_OFFS		0
#define MV_PP2_HWF_TXQ_POOL_SHORT_MASK		(0x7 << MV_PP2_HWF_TXQ_POOL_SHORT_OFFS)

#define MV_PP2_HWF_TXQ_POOL_LONG_OFFS		4
#define MV_PP2_HWF_TXQ_POOL_LONG_MASK		(0x7 << MV_PP2_HWF_TXQ_POOL_LONG_OFFS)

#define MV_PP2_HWF_TXQ_DISABLE_BIT              31
#define MV_PP2_HWF_TXQ_DISABLE_MASK             (0x1 << MV_PP2_HWF_TXQ_DISABLE_BIT)
/*-------------------------------------------------------------------------------*/

#else /* PPv2 - Z1 */

#define MV_PP2_V0_RXQ_SNOOP_REG(rxq)		(MV_PP2_REG_BASE + 0x800 + 4 * (rxq))

#define MV_PP2_V0_SNOOP_PKT_SIZE_OFFS		5
#define MV_PP2_V0_SNOOP_PKT_SIZE_MASK		(0x1FF << MV_PP2_V0_SNOOP_PKT_SIZE_OFFS)

#define MV_PP2_V0_SNOOP_BUF_HDR_OFFS		14
#define MV_PP2_V0_SNOOP_BUF_HDR_MASK		(0x1 << MV_PP2_V0_SNOOP_BUF_HDR_OFFS)

#define MV_PP2_V0_L2_DEPOSIT_PKT_SIZE_OFFS	21
#define MV_PP2_V0_L2_DEPOSIT_PKT_SIZE_MASK	(0xF << MV_PP2_V0_L2_DEPOSIT_PKT_SIZE_OFFS)

#define MV_PP2_V0_L2_DEPOSIT_BUF_HDR_OFFS	25
#define MV_PP2_V0_L2_DEPOSIT_BUF_HDR_MASK	(0x1 << MV_PP2_V0_L2_DEPOSIT_BUF_HDR_OFFS)
/*-------------------------------------------------------------------------------*/

#define MV_PP2_V0_RXQ_CONFIG_REG(rxq)		(MV_PP2_REG_BASE + 0xc00 + 4 * (rxq))

#define MV_PP2_V0_RXQ_POOL_SHORT_OFFS		0
#define MV_PP2_V0_RXQ_POOL_SHORT_MASK		(0x7 << MV_PP2_V0_RXQ_POOL_SHORT_OFFS)
#define MV_PP2_V0_RXQ_POOL_LONG_OFFS		8
#define MV_PP2_V0_RXQ_POOL_LONG_MASK		(0x7 << MV_PP2_V0_RXQ_POOL_LONG_OFFS)
#define MV_PP2_V0_RXQ_PACKET_OFFSET_OFFS	17
#define MV_PP2_V0_RXQ_PACKET_OFFSET_MASK	(0xFF << MV_PP2_V0_RXQ_PACKET_OFFSET_OFFS)
/*-------------------------------------------------------------------------------*/

#define MV_PP2_V0_PORT_HWF_CONFIG_REG(port)	(MV_PP2_REG_BASE + 0x120 + 4 * (port))

#define MV_PP2_V0_PORT_HWF_POOL_SHORT_OFFS	0
#define MV_PP2_V0_PORT_HWF_POOL_SHORT_MASK	(0x7 << MV_PP2_V0_PORT_HWF_POOL_SHORT_OFFS)
#define MV_PP2_V0_PORT_HWF_POOL_LONG_OFFS	8
#define MV_PP2_V0_PORT_HWF_POOL_LONG_MASK	(0x7 << MV_PP2_V0_PORT_HWF_POOL_LONG_OFFS)
/*-------------------------------------------------------------------------------*/

#endif /* PPv2 - Z1 / PPv2.1 - A0 */

#define MV_PP2_RX_GEMPID_SRC_OFFS		8
#define MV_PP2_RX_GEMPID_SRC_MASK		(0x7 << MV_PP2_RX_GEMPID_SRC_OFFS)

#define MV_PP2_RX_LOW_LATENCY_PKT_SIZE_OFFS	16
#define MV_PP2_RX_LOW_LATENCY_PKT_SIZE_BITS	12
#define MV_PP2_RX_LOW_LATENCY_PKT_SIZE_MAX	((1 << MV_PP2_RX_LOW_LATENCY_PKT_SIZE_BITS) - 1)
#define MV_PP2_RX_LOW_LATENCY_PKT_SIZE_MASK(s)	(((s) & MV_PP2_RX_LOW_LATENCY_PKT_SIZE_MAX) << \
							MV_PP2_RX_LOW_LATENCY_PKT_SIZE_OFFS)

#define MV_PP2_RX_DROP_ON_CSUM_ERR_BIT		30
#define MV_PP2_RX_DROP_ON_CSUM_ERR_MASK		(1 << MV_PP2_RX_DROP_ON_CSUM_ERR_BIT)

#define MV_PP2_RX_USE_PSEUDO_FOR_CSUM_BIT	31
#define MV_PP2_RX_USE_PSEUDO_FOR_CSUM_MASK      (1 << MV_PP2_RX_USE_PSEUDO_FOR_CSUM_BIT)
/*-------------------------------------------------------------------------------*/

/************************** Descriptor Manager Top Registers ******************************/

#define MV_PP2_RXQ_NUM_REG			(MV_PP2_REG_BASE + 0x2040)

#define MV_PP2_RXQ_NUM_OFFSET			0
#define MV_PP2_RXQ_NUM_MASK			(0xFF << MV_PP2_RXQ_NUM_OFFSET)
/*-------------------------------------------------------------------------------*/
#define MV_PP2_RXQ_DESC_ADDR_REG		(MV_PP2_REG_BASE + 0x2044)

#define MV_PP2_RXQ_DESC_SIZE_REG		(MV_PP2_REG_BASE + 0x2048)

#define MV_PP2_RXQ_DESC_SIZE_OFFSET		4
#define MV_PP2_RXQ_DESC_SIZE_MASK		(0x3FF << MV_PP2_RXQ_DESC_SIZE_OFFSET)

#define MV_PP2_RXQ_L2_DEPOSIT_OFFSET		16
#define MV_PP2_RXQ_L2_DEPOSIT_MASK		(0x1 << MV_PP2_RXQ_L2_DEPOSIT_OFFSET)
/*-------------------------------------------------------------------------------*/
#define MV_PP2_RXQ_STATUS_UPDATE_REG(rxq)	(MV_PP2_REG_BASE + 0x3000 + 4 * (rxq))

#define MV_PP2_RXQ_NUM_PROCESSED_OFFSET	0
#define MV_PP2_RXQ_NUM_PROCESSED_MASK		(0x3FFF << MV_PP2_RXQ_NUM_PROCESSED_OFFSET)
#define MV_PP2_RXQ_NUM_NEW_OFFSET		16
#define MV_PP2_RXQ_NUM_NEW_MASK			(0x3FFF << MV_PP2_RXQ_NUM_NEW_OFFSET)
/*-------------------------------------------------------------------------------*/
#define MV_PP2_RXQ_STATUS_REG(rxq)		(MV_PP2_REG_BASE + 0x3400 + 4 * (rxq))

#define MV_PP2_RXQ_OCCUPIED_OFFSET		0
#define MV_PP2_RXQ_OCCUPIED_MASK		(0x3FFF << MV_PP2_RXQ_OCCUPIED_OFFSET)
#define MV_PP2_RXQ_NON_OCCUPIED_OFFSET		16
#define MV_PP2_RXQ_NON_OCCUPIED_MASK		(0x3FFF << MV_PP2_RXQ_NON_OCCUPIED_OFFSET)
/*-------------------------------------------------------------------------------*/
#define MV_PP2_RXQ_THRESH_REG			(MV_PP2_REG_BASE + 0x204c)

#define MV_PP2_OCCUPIED_THRESH_OFFSET		0
#define MV_PP2_OCCUPIED_THRESH_MASK		(0x3FFF << MV_PP2_OCCUPIED_THRESH_OFFSET)

#define MV_PP2_NON_OCCUPIED_THRESH_OFFSET	16
#define MV_PP2_NON_OCCUPIED_THRESH_MASK		(0x3FFF << MV_PP2_NON_OCCUPIED_THRESH_OFFSET)
/*-------------------------------------------------------------------------------*/

#define MV_PP2_RXQ_INDEX_REG			(MV_PP2_REG_BASE + 0x2050)
/*-------------------------------------------------------------------------------*/

/*-------------------------------------------------------------------------------*/
#define MV_PP2_TXQ_NUM_REG			(MV_PP2_REG_BASE + 0x2080)

#define MV_PP2_TXQ_NUM_OFFSET			0
#define MV_PP2_TXQ_NUM_MASK			(0xFF << MV_PP2_RXQ_NUM_OFFSET)
/*-------------------------------------------------------------------------------*/
#define MV_PP2_TXQ_DESC_ADDR_REG		(MV_PP2_REG_BASE + 0x2084)

#define MV_PP2_TXQ_DESC_SIZE_REG		(MV_PP2_REG_BASE + 0x2088)

#define MV_PP2_TXQ_DESC_SIZE_OFFSET		4
#define MV_PP2_TXQ_DESC_SIZE_MASK		(0x3FF << MV_PP2_TXQ_DESC_SIZE_OFFSET)
/*-------------------------------------------------------------------------------*/
#define MV_PP2_TXQ_DESC_HWF_SIZE_REG		(MV_PP2_REG_BASE + 0x208c)

#define MV_PP2_TXQ_DESC_HWF_SIZE_OFFSET		4
#define MV_PP2_TXQ_DESC_HWF_SIZE_MASK		(0x3FF << MV_PP2_TXQ_DESC_HWF_SIZE_OFFSET)
/*-------------------------------------------------------------------------------*/

/* Aggregated (per CPU) TXQ - WO */
#define MV_PP2_AGGR_TXQ_UPDATE_REG		(MV_PP2_REG_BASE + 0x2090)
/*-------------------------------------------------------------------------------*/

/* Each CPU has own copy */
#define MV_PP2_TXQ_THRESH_REG			(MV_PP2_REG_BASE + 0x2094)

#define MV_PP2_TRANSMITTED_THRESH_OFFSET	16
#define MV_PP2_TRANSMITTED_THRESH_MASK		(0x3FFF << MV_PP2_TRANSMITTED_THRESH_OFFSET)
/*-------------------------------------------------------------------------------*/

#define MV_PP2_TXQ_INDEX_REG			(MV_PP2_REG_BASE + 0x2098)
/*-------------------------------------------------------------------------------*/

#define MV_PP2_TXQ_PREF_BUF_REG			(MV_PP2_REG_BASE + 0x209c)

#define MV_PP2_PREF_BUF_PTR_OFFS		0
#define MV_PP2_PREF_BUF_PTR_MASK		(0xFFF << MV_PP2_PREF_BUF_PTR_OFFS)
#define MV_PP2_PREF_BUF_PTR(desc)		(((desc) << MV_PP2_PREF_BUF_PTR_OFFS) & MV_PP2_PREF_BUF_PTR_MASK)

#define MV_PP2_PREF_BUF_SIZE_OFFS		12
#define MV_PP2_PREF_BUF_SIZE_MASK		(0x7 << MV_PP2_PREF_BUF_SIZE_OFFS)
#define MV_PP2_PREF_BUF_SIZE_NONE		(0 << MV_PP2_PREF_BUF_SIZE_OFFS)
#define MV_PP2_PREF_BUF_SIZE_1			(1 << MV_PP2_PREF_BUF_SIZE_OFFS)
#define MV_PP2_PREF_BUF_SIZE_2			(2 << MV_PP2_PREF_BUF_SIZE_OFFS)
#define MV_PP2_PREF_BUF_SIZE_4			(3 << MV_PP2_PREF_BUF_SIZE_OFFS)
#define MV_PP2_PREF_BUF_SIZE_8			(4 << MV_PP2_PREF_BUF_SIZE_OFFS)
#define MV_PP2_PREF_BUF_SIZE_16			(5 << MV_PP2_PREF_BUF_SIZE_OFFS)
#define MV_PP2_PREF_BUF_SIZE_32			(6 << MV_PP2_PREF_BUF_SIZE_OFFS)
#define MV_PP2_PREF_BUF_SIZE_64			(7 << MV_PP2_PREF_BUF_SIZE_OFFS)

#define MV_PP2_PREF_BUF_THRESH_OFFS		17
#define MV_PP2_PREF_BUF_THRESH_MASK		(0xF << MV_PP2_PREF_BUF_THRESH_OFFS)
#define MV_PP2_PREF_BUF_THRESH(val)		((val) << MV_PP2_PREF_BUF_THRESH_OFFS)

/* new field for PPV2.1 - A0 only */
#define MV_PP2_TXQ_DRAIN_EN_BIT			31
#define MV_PP2_TXQ_DRAIN_EN_MASK		(1 << MV_PP2_TXQ_DRAIN_EN_BIT)
/*-------------------------------------------------------------------------------*/

#define MV_PP2_TXQ_PENDING_REG			(MV_PP2_REG_BASE + 0x20a0)

#define MV_PP2_TXQ_PENDING_OFFSET		0
#define MV_PP2_TXQ_PENDING_MASK			(0x3FFF << MV_PP2_TXQ_PENDING_OFFSET)

/*
   ppv2.1 field MV_PP2_TXQ_HWF_PENDING_OFFSET changed to MV_PP2_TXQ_RESERVED_DESC_OFFSET
   MAS 3.16
*/
#define MV_PP2_TXQ_HWF_PENDING_OFFSET		16
#define MV_PP2_TXQ_HWF_PENDING_MASK		(0x3FFF << MV_PP2_TXQ_HWF_PENDING_OFFSET)

#define MV_PP2_TXQ_RSVD_DESC_OFFSET		16
#define MV_PP2_TXQ_RSVD_DESC_MASK		(0x3FFF << MV_PP2_TXQ_RSVD_DESC_MASK)

/*-------------------------------------------------------------------------------*/

#define MV_PP2_TXQ_INT_STATUS_REG		(MV_PP2_REG_BASE + 0x20a4)
/*-------------------------------------------------------------------------------*/
/*
   ppv2.1- new register 0x20b0, not exist ip ppv2.0
   MAS 3.16
*/
#define MV_PP2_TXQ_RSVD_REQ_REG			(MV_PP2_REG_BASE + 0x20b0)

#define MV_PP2_TXQ_RSVD_REQ_DESC_OFFSET		0
#define MV_PP2_TXQ_RSVD_REQ_DESC_MASK		(0x3FFF << MV_PP2_TXQ_RSVD_REQ_DESC_OFFSET)

#define MV_PP2_TXQ_RSVD_REQ_Q_OFFSET		16
#define MV_PP2_TXQ_RSVD_REQ_Q_MASK		(0xFF << MV_PP2_TXQ_RSVD_REQ_Q_OFFSET)
/*-------------------------------------------------------------------------------*/
/*
   ppv2.1- new register 0x20b4, not exist ip ppv2.0
   MAS 3.16
*/
#define MV_PP2_TXQ_RSVD_RSLT_REG		(MV_PP2_REG_BASE + 0x20b4)

#define MV_PP2_TXQ_RSVD_RSLT_OFFSET		0
#define MV_PP2_TXQ_RSVD_RSLT_MASK		(0x3FFF << MV_PP2_TXQ_RSVD_RSLT_OFFSET)

/*-------------------------------------------------------------------------------*/
/*
   ppv2.1- new register 0x20b8, not exist ip ppv2.0
   MAS 3.22
*/
#define MV_PP2_TXQ_RSVD_CLR_REG			(MV_PP2_REG_BASE + 0x20b8)

#define MV_PP2_TXQ_RSVD_CLR_OFFSET		16
#define MV_PP2_TXQ_RSVD_CLR_MASK		(0xFF << MV_PP2_TXQ_RSVD_CLR_OFFSET)
/*-------------------------------------------------------------------------------*/

/* Direct access - per TXQ, per CPU */
#define MV_PP2_TXQ_SENT_REG(txq)		(MV_PP2_REG_BASE + 0x3c00 + 4 * (txq))

#define MV_PP2_TRANSMITTED_COUNT_OFFSET	16
#define MV_PP2_TRANSMITTED_COUNT_MASK		(0x3FFF << MV_PP2_TRANSMITTED_COUNT_OFFSET)
/*-------------------------------------------------------------------------------*/

#define MV_PP2_AGGR_TXQ_DESC_ADDR_REG(cpu)	(MV_PP2_REG_BASE + 0x2100 + 4 * (cpu))

#define MV_PP2_AGGR_TXQ_DESC_SIZE_REG(cpu)	(MV_PP2_REG_BASE + 0x2140 + 4 * (cpu))
#define MV_PP2_AGGR_TXQ_DESC_SIZE_OFFSET	4
#define MV_PP2_AGGR_TXQ_DESC_SIZE_MASK		(0x3FF << MV_PP2_AGGR_TXQ_DESC_SIZE_OFFSET)
/*-------------------------------------------------------------------------------*/
#define MV_PP2_AGGR_TXQ_STATUS_REG(cpu)		(MV_PP2_REG_BASE + 0x2180 + 4 * (cpu))

#define MV_PP2_AGGR_TXQ_PENDING_OFFSET		0
#define MV_PP2_AGGR_TXQ_PENDING_MASK		(0x3FFF << MV_PP2_AGGR_TXQ_PENDING_OFFSET)
/*-------------------------------------------------------------------------------*/

#define MV_PP2_AGGR_TXQ_INDEX_REG(cpu)		(MV_PP2_REG_BASE + 0x21c0 + 4 * (cpu))
/*-------------------------------------------------------------------------------*/

/* Registers for HWF to SWF switching */
#define MV_PP2_FWD_SWITCH_FLOW_ID_REG		(MV_PP2_REG_BASE + 0x2200)

#define MV_PP2_FWD_SWITCH_CTRL_REG		(MV_PP2_REG_BASE + 0x2204)

#define MV_PP2_FWD_SWITCH_TXQ_OFFS		0
#define MV_PP2_FWD_SWITCH_TXQ_MAX		255
#define MV_PP2_FWD_SWITCH_TXQ_MASK              (255 << MV_PP2_FWD_SWITCH_TXQ_OFFS)
#define MV_PP2_FWD_SWITCH_TXQ_VAL(txq)		(((txq) << MV_PP2_FWD_SWITCH_TXQ_OFFS) & \
							MV_PP2_FWD_SWITCH_TXQ_MASK)

#define MV_PP2_FWD_SWITCH_RXQ_OFFS		8
#define MV_PP2_FWD_SWITCH_RXQ_MAX		255
#define MV_PP2_FWD_SWITCH_RXQ_MASK              (255 << MV_PP2_FWD_SWITCH_RXQ_OFFS)
#define MV_PP2_FWD_SWITCH_RXQ_VAL(rxq)		(((rxq) << MV_PP2_FWD_SWITCH_RXQ_OFFS) & \
							MV_PP2_FWD_SWITCH_RXQ_MASK)

#define MV_PP2_FWD_SWITCH_TIMEOUT_OFFS		16
#define MV_PP2_FWD_SWITCH_TIMEOUT_BITS		10
#define MV_PP2_FWD_SWITCH_TIMEOUT_MAX		((1 << MV_PP2_FWD_SWITCH_TIMEOUT_BITS) - 1)
#define MV_PP2_FWD_SWITCH_TIMEOUT_MASK          (MV_PP2_FWD_SWITCH_TIMEOUT_MAX << MV_PP2_FWD_SWITCH_TIMEOUT_OFFS)
#define MV_PP2_FWD_SWITCH_TIMEOUT_VAL(time)	(((time) << MV_PP2_FWD_SWITCH_TIMEOUT_OFFS) & \
							MV_PP2_FWD_SWITCH_TIMEOUT_MASK)

#define MV_PP2_FWD_SWITCH_STATUS_REG		(MV_PP2_REG_BASE + 0x2208)

#define MV_PP2_FWD_SWITCH_STATE_OFFS		0
#define MV_PP2_FWD_SWITCH_STATE_MASK		(0x7 << MV_PP2_FWD_SWITCH_STATE_OFFS)

#define MV_PP2_FWD_SWITCH_STATUS_OFFS		4
#define MV_PP2_FWD_SWITCH_STATUS_MASK		(0x3 << MV_PP2_FWD_SWITCH_STATUS_OFFS)

#define MV_PP2_FWD_SWITCH_TIMER_OFFS		16
#define MV_PP2_FWD_SWITCH_TIMER_BITS		10
#define MV_PP2_FWD_SWITCH_TIMER_MAX		((1 << MV_PP2_FWD_SWITCH_TIMER_BITS) - 1)
#define MV_PP2_FWD_SWITCH_TIMER_MASK		(MV_PP2_FWD_SWITCH_TIMER_MAX << MV_PP2_FWD_SWITCH_TIMER_OFFS)
/*-------------------------------------------------------------------------------*/

/* Unused registers */
#define MV_PP2_INTERNAL_BUF_CTRL_REG		(MV_PP2_REG_BASE + 0x2220)

/* No CPU access to Physcal TXQ descriptors, so Snoop doesn't needed */
#define MV_PP2_TX_DESC_SNOOP_REG		(MV_PP2_REG_BASE + 0x2224)
/*-------------------------------------------------------------------------------*/


/************************** Interrupt Cause and Mask registers ******************/
#define MV_PP2_ISR_RX_THRESHOLD_REG(port)	(MV_PP2_REG_BASE + 0x5200 + 4 * (port))

#define MV_PP2_ISR_RXQ_GROUP_REG(port)		(MV_PP2_REG_BASE + 0x5400 + 4 * (port))

#define MV_PP2_ISR_ENABLE_REG(port)		(MV_PP2_REG_BASE + 0x5420 + 4 * (port))

#define MV_PP2_ISR_ENABLE_INTERRUPT_OFFS	0
#define MV_PP2_ISR_ENABLE_INTERRUPT_MASK	0xFFFF
#define MV_PP2_ISR_ENABLE_INTERRUPT(cpuMask)	(((cpuMask) << MV_PP2_ISR_ENABLE_INTERRUPT_OFFS)\
							& MV_PP2_ISR_ENABLE_INTERRUPT_MASK)

#define MV_PP2_ISR_DISABLE_INTERRUPT_OFFS	16
#define MV_PP2_ISR_DISABLE_INTERRUPT_MASK	(0xFFFF << MV_PP2_ISR_DISABLE_INTERRUPT_OFFS)
#define MV_PP2_ISR_DISABLE_INTERRUPT(cpuMask)	(((cpuMask) << MV_PP2_ISR_DISABLE_INTERRUPT_OFFS)\
							& MV_PP2_ISR_DISABLE_INTERRUPT_MASK)


#define MV_PP2_ISR_RX_TX_CAUSE_REG(port)	(MV_PP2_REG_BASE + 0x5480 + 4 * (port))
#define MV_PP2_ISR_RX_TX_MASK_REG(port)		(MV_PP2_REG_BASE + 0x54a0 + 4 * (port))

#define MV_PP2_CAUSE_RXQ_OCCUP_DESC_OFFS      	0
#define MV_PP2_CAUSE_RXQ_OCCUP_DESC_BIT(q)    	(MV_PP2_CAUSE_RXQ_OCCUP_DESC_OFFS + (q))
#define MV_PP2_CAUSE_RXQ_OCCUP_DESC_ALL_MASK  	(0xFFFF << MV_PP2_CAUSE_RXQ_OCCUP_DESC_OFFS)
#define MV_PP2_CAUSE_RXQ_OCCUP_DESC_MASK(q)   	(1 << (MV_PP2_CAUSE_RXQ_OCCUP_DESC_BIT(q)))

#define MV_PP2_CAUSE_TXQ_OCCUP_DESC_OFFS       	16
#define MV_PP2_CAUSE_TXQ_OCCUP_DESC_BIT(q)     	(MV_PP2_CAUSE_TXQ_OCCUP_DESC_OFFS + (q))
#define MV_PP2_CAUSE_TXQ_OCCUP_DESC_ALL_MASK   	(0xFF << MV_PP2_CAUSE_TXQ_OCCUP_DESC_OFFS)
#define MV_PP2_CAUSE_TXQ_OCCUP_DESC_MASK(q)    	(1 << (MV_PP2_CAUSE_TXQ_SENT_DESC_BIT(q)))

#define MV_PP2_CAUSE_RX_FIFO_OVERRUN_BIT        24
#define MV_PP2_CAUSE_RX_FIFO_OVERRUN_MASK      	(1 << MV_PP2_CAUSE_RX_FIFO_OVERRUN_BIT)

#define MV_PP2_CAUSE_FCS_ERR_BIT           	25
#define MV_PP2_CAUSE_FCS_ERR_MASK          	(1 << MV_PP2_CAUSE_FCS_ERR_BIT)

#define MV_PP2_CAUSE_TX_FIFO_UNDERRUN_BIT       26
#define MV_PP2_CAUSE_TX_FIFO_UNDERRUN_MASK     	(1 << MV_PP2_CAUSE_TX_FIFO_UNDERRUN_BIT)

#define MV_PP2_CAUSE_TX_EXCEPTION_SUM_BIT     	29
#define MV_PP2_CAUSE_TX_EXCEPTION_SUM_MASK     	(1 << MV_PP2_CAUSE_TX_EXCEPTION_SUM_BIT)

#define MV_PP2_CAUSE_RX_EXCEPTION_SUM_BIT     	30
#define MV_PP2_CAUSE_RX_EXCEPTION_SUM_MASK     	(1 << MV_PP2_CAUSE_RX_EXCEPTION_SUM_BIT)

#define MV_PP2_CAUSE_MISC_SUM_BIT     		31
#define MV_PP2_CAUSE_MISC_SUM_MASK     		(1 << MV_PP2_CAUSE_MISC_SUM_BIT)

#define MV_PP2_CAUSE_MISC_ERR_SUM_MASK		(0xE7000000)


#define MV_PP2_ISR_PON_RX_TX_CAUSE_REG			(MV_PP2_REG_BASE + 0x549c)
#define MV_PP2_ISR_PON_RX_TX_MASK_REG			(MV_PP2_REG_BASE + 0x54bc)

#define MV_PP2_PON_CAUSE_RXQ_OCCUP_DESC_OFFS      	0
#define MV_PP2_PON_CAUSE_RXQ_OCCUP_DESC_BIT(q)    	(MV_PP2_PON_CAUSE_RXQ_OCCUP_DESC_OFFS + (q))
#define MV_PP2_PON_CAUSE_RXQ_OCCUP_DESC_ALL_MASK  	(0xFFFF << MV_PP2_PON_CAUSE_RXQ_OCCUP_DESC_OFFS)
#define MV_PP2_PON_CAUSE_RXQ_OCCUP_DESC_MASK(q)   	(1 << (MV_PP2_PON_CAUSE_RXQ_OCCUP_DESC_BIT(q)))

#define MV_PP2_PON_CAUSE_RX_FIFO_OVERRUN_BIT        	16
#define MV_PP2_PON_CAUSE_RX_FIFO_OVERRUN_MASK      	(1 << MV_PP2_CAUSE_RX_FIFO_OVERRUN_BIT)

#define MV_PP2_PON_CAUSE_FCS_ERR_BIT           		17
#define MV_PP2_PON_CAUSE_FCS_ERR_MASK          		(1 << MV_PP2_PON_CAUSE_FCS_ERR_BIT)

#define MV_PP2_PON_BYTE_COUNT_ERR_BIT          		18
#define MV_PP2_PON_BYTE_COUNT_ERR_MASK         		(1 << MV_PP2_PON_BYTE_COUNT_ERR_BIT)

#define MV_PP2_PON_CAUSE_TX_FIFO_UNDERRUN_BIT       	21
#define MV_PP2_PON_CAUSE_TX_FIFO_UNDERRUN_MASK     	(1 << MV_PP2_PON_CAUSE_TX_FIFO_UNDERRUN_BIT)

#define MV_PP2_PON_CAUSE_TXP_OCCUP_DESC_OFFS       	22
#define MV_PP2_PON_CAUSE_TXP_OCCUP_DESC_ALL_MASK   	(0xFF << MV_PP2_PON_CAUSE_TXP_OCCUP_DESC_OFFS)

#define MV_PP2_PON_CAUSE_TX_EXCEPTION_SUM_BIT     	29
#define MV_PP2_PON_CAUSE_TX_EXCEPTION_SUM_MASK     	(1 << MV_PP2_PON_CAUSE_TX_EXCEPTION_SUM_BIT)

#define MV_PP2_PON_CAUSE_RX_EXCEPTION_SUM_BIT     	30
#define MV_PP2_PON_CAUSE_RX_EXCEPTION_SUM_MASK     	(1 << MV_PP2_PON_CAUSE_RX_EXCEPTION_SUM_BIT)

#define MV_PP2_PON_CAUSE_MISC_SUM_BIT     		31
#define MV_PP2_PON_CAUSE_MISC_SUM_MASK     		(1 << MV_PP2_PON_CAUSE_MISC_SUM_BIT)

#define MV_PP2_PON_CAUSE_MISC_ERR_SUM_MASK		(0xC0270000)


/* TCONT Cause registers 54c0 - 54cc */
/* TCONT Mask registers 54d0 - 54dc */
#define MV_PP2_ISR_RX_ERR_CAUSE_REG(port)	(MV_PP2_REG_BASE + 0x5500 + 4 * (port))
#define MV_PP2_ISR_RX_ERR_MASK_REG(port)	(MV_PP2_REG_BASE + 0x5520 + 4 * (port))
#define MV_PP2_ISR_TX_ERR_CAUSE_REG(port)	(MV_PP2_REG_BASE + 0x5500 + 4 * (port))
#define MV_PP2_ISR_TX_ERR_MASK_REG(port)	(MV_PP2_REG_BASE + 0x5520 + 4 * (port))
/* TCONT TX exception Cause registers 5580 - 558c */
/* TCONT TX exception Mask registers 5590 - 559c */
#define MV_PP2_ISR_PON_TX_UNDR_CAUSE_REG	(MV_PP2_REG_BASE + 0x55a0)
#define MV_PP2_ISR_PON_TX_UNDR_MASK_REG		(MV_PP2_REG_BASE + 0x55a4)

#define MV_PP2_ISR_MISC_CAUSE_REG			(MV_PP2_REG_BASE + 0x55b0)
#define MV_PP2_ISR_MISC_MASK_REG			(MV_PP2_REG_BASE + 0x55b4)
/*-------------------------------------------------------------------------------*/

#define MV_PP2_OVERRUN_DROP_REG(port)		(MV_PP2_REG_BASE + 0x7000 + 4 * (port))
#define MV_PP2_CLS_DROP_REG(port)			(MV_PP2_REG_BASE + 0x7020 + 4 * (port))

/******************************** Port Drop counters ppv2.0*****************************/

#define MV_PP2_V0_POLICER_DROP_REG(plcr)		(MV_PP2_REG_BASE + 0x7040 + 4 * (plcr))
#define MV_PP2_V0_TX_EARLY_DROP_REG(eport)		(MV_PP2_REG_BASE + 0x7080 + 4 * (eport))
#define MV_PP2_V0_TX_DESC_DROP_REG(eport)		(MV_PP2_REG_BASE + 0x7100 + 4 * (eport))
#define MV_PP2_V0_RX_EARLY_DROP_REG(rxq)		(MV_PP2_REG_BASE + 0x7200 + 4 * (rxq))
#define MV_PP2_V0_RX_DESC_DROP_REG(rxq)			(MV_PP2_REG_BASE + 0x7400 + 4 * (rxq))

/************************************ counters ppv2.1 **********************************/


#define MV_PP2_V1_CNT_IDX_REG				(MV_PP2_REG_BASE + 0x7040)
/* TX counters index */
#define TX_CNT_IDX_TXP					3
#define TX_CNT_IDX_TXQ					0

#define TX_CNT_IDX(port, txp, txq)			((MV_PPV2_TXP_PHYS(port, txp) << 3) | (txq))

#define MV_PP2_V1_TX_DESC_ENQ_REG			(MV_PP2_REG_BASE + 0x7100)
#define MV_PP2_V1_TX_DESC_ENQ_TO_DRAM_REG		(MV_PP2_REG_BASE + 0x7104)
#define MV_PP2_V1_TX_BUF_ENQ_TO_DRAM_REG		(MV_PP2_REG_BASE + 0x7108)
#define MV_PP2_V1_TX_DESC_HWF_ENQ_REG			(MV_PP2_REG_BASE + 0x710c)
#define MV_PP2_V1_TX_PKT_DQ_REG				(MV_PP2_REG_BASE + 0x7130)
#define MV_PP2_V1_TX_PKT_FULLQ_DROP_REG			(MV_PP2_REG_BASE + 0x7200)
#define MV_PP2_V1_TX_PKT_EARLY_DROP_REG			(MV_PP2_REG_BASE + 0x7204)
#define MV_PP2_V1_TX_PKT_BM_DROP_REG			(MV_PP2_REG_BASE + 0x7208)
#define MV_PP2_V1_TX_PKT_BM_MC_DROP_REG			(MV_PP2_REG_BASE + 0x720c)

#define MV_PP2_V1_RX_PKT_FULLQ_DROP_REG			(MV_PP2_REG_BASE + 0x7220)
#define MV_PP2_V1_RX_PKT_EARLY_DROP_REG			(MV_PP2_REG_BASE + 0x7224)
#define MV_PP2_V1_RX_PKT_BM_DROP_REG			(MV_PP2_REG_BASE + 0x7228)
#define MV_PP2_V1_RX_DESC_ENQ_REG			(MV_PP2_REG_BASE + 0x7120)

#define MV_PP2_V1_OVERFLOW_MC_DROP_REG			(MV_PP2_REG_BASE + 0x770c)




/*-------------------------------------------------------------------------------*/


/************************** TX Scheduler Registers ******************************/
/* Indirect access */
#define MV_PP2_TXP_SCHED_PORT_INDEX_REG		(MV_PP2_REG_BASE + 0x8000)

#define MV_PP2_TXP_SCHED_Q_CMD_REG		(MV_PP2_REG_BASE + 0x8004)

#define MV_PP2_TXP_SCHED_ENQ_OFFSET		0
#define MV_PP2_TXP_SCHED_ENQ_MASK		(0xFF << MV_PP2_TXP_SCHED_ENQ_OFFSET)
#define MV_PP2_TXP_SCHED_DISQ_OFFSET		8
#define MV_PP2_TXP_SCHED_DISQ_MASK		(0xFF << MV_PP2_TXP_SCHED_DISQ_OFFSET)
/*-----------------------------------------------------------------------------------------------*/

#define MV_PP2_TXP_SCHED_CMD_1_REG		(MV_PP2_REG_BASE + 0x8010)

#define MV_PP2_TXP_SCHED_RESET_BIT		0
#define MV_PP2_TXP_SCHED_RESET_MASK		(1 << MV_PP2_TXP_SCHED_RESET_BIT)

#define MV_PP2_TXP_SCHED_PTP_SYNC_BIT		1
#define MV_PP2_TXP_SCHED_PTP_SYNC_MASK		(1 << MV_PP2_TXP_SCHED_PTP_SYNC_BIT)

#define MV_PP2_TXP_SCHED_EJP_ENABLE_BIT		2
#define MV_PP2_TXP_SCHED_EJP_ENABLE_MASK	(1 << MV_PP2_TXP_SCHED_EJP_ENABLE_BIT)
/*-----------------------------------------------------------------------------------------------*/

/* Transmit Queue Fixed Priority Configuration (TQFPC) */
#define MV_PP2_TXP_SCHED_FIXED_PRIO_REG		(MV_PP2_REG_BASE + 0x8014)

#define MV_PP2_TXP_FIXED_PRIO_OFFS          	0
#define MV_PP2_TXP_FIXED_PRIO_MASK          	(0xFF << MV_PP2_TX_FIXED_PRIO_OFFS)
/*-----------------------------------------------------------------------------------------------*/

/* Basic Refill No of Clocks (BRC) */
#define MV_PP2_TXP_SCHED_PERIOD_REG		(MV_PP2_REG_BASE + 0x8018)

#define MV_PP2_TXP_REFILL_CLOCKS_OFFS       	0
#define MV_PP2_TXP_REFILL_CLOCKS_MIN        	16
#define MV_PP2_TXP_REFILL_CLOCKS_MASK       	(0xFFFF << MV_PP2_TXP_REFILL_CLOCKS_OFFS)
/*-----------------------------------------------------------------------------------------------*/

/* Port Maximum Transmit Unit (PMTU) */
#define MV_PP2_TXP_SCHED_MTU_REG		(MV_PP2_REG_BASE + 0x801c)
#define MV_PP2_TXP_MTU_OFFS			0
#define MV_PP2_TXP_MTU_MAX			0x7FFFF
#define MV_PP2_TXP_MTU_ALL_MASK			(MV_PP2_TXP_MTU_MAX << MV_PP2_TXP_MTU_OFFS)
#define MV_PP2_TXP_MTU_MASK(mtu)		((mtu) << MV_PP2_TXP_MTU_OFFS)
/*-----------------------------------------------------------------------------------------------*/

/* Port Bucket Refill (PRefill) */
#define MV_PP2_TXP_SCHED_REFILL_REG		(MV_PP2_REG_BASE + 0x8020)
#define MV_PP2_TXP_REFILL_TOKENS_OFFS		0
#define MV_PP2_TXP_REFILL_TOKENS_MAX		0x7FFFF
#define MV_PP2_TXP_REFILL_TOKENS_ALL_MASK	(MV_PP2_TXP_REFILL_TOKENS_MAX << MV_PP2_TXP_REFILL_TOKENS_OFFS)
#define MV_PP2_TXP_REFILL_TOKENS_MASK(val)	((val) << MV_PP2_TXP_REFILL_TOKENS_OFFS)

#define MV_PP2_TXP_REFILL_PERIOD_OFFS       	20
#define MV_PP2_TXP_REFILL_PERIOD_MAX        	0x3FF
#define MV_PP2_TXP_REFILL_PERIOD_ALL_MASK   	(MV_PP2_TXP_REFILL_PERIOD_MAX << MV_PP2_TXP_REFILL_PERIOD_OFFS)
#define MV_PP2_TXP_REFILL_PERIOD_MASK(val)  	((val) << MV_PP2_TXP_REFILL_PERIOD_OFFS)
/*-----------------------------------------------------------------------------------------------*/

/* Port Maximum Token Bucket Size (PMTBS) */
#define MV_PP2_TXP_SCHED_TOKEN_SIZE_REG		(MV_PP2_REG_BASE + 0x8024)
#define MV_PP2_TXP_TOKEN_SIZE_MAX           	0xFFFFFFFF
/*-----------------------------------------------------------------------------------------------*/

/* Port Token Bucket Counter (PMTBS) */
#define MV_PP2_TXP_SCHED_TOKEN_CNTR_REG		(MV_PP2_REG_BASE + 0x8028)
#define MV_PP2_TXP_TOKEN_CNTR_MAX		0xFFFFFFFF
/*-----------------------------------------------------------------------------------------------*/

/* Queue Bucket Refill (QRefill) */
#define MV_PP2_TXQ_SCHED_REFILL_REG(q)		(MV_PP2_REG_BASE + 0x8040 + ((q) << 2))

#define MV_PP2_TXQ_REFILL_TOKENS_OFFS		0
#define MV_PP2_TXQ_REFILL_TOKENS_MAX		0x7FFFF
#define MV_PP2_TXQ_REFILL_TOKENS_ALL_MASK	(MV_PP2_TXQ_REFILL_TOKENS_MAX << MV_PP2_TXQ_REFILL_TOKENS_OFFS)
#define MV_PP2_TXQ_REFILL_TOKENS_MASK(val)	((val) << MV_PP2_TXQ_REFILL_TOKENS_OFFS)

#define MV_PP2_TXQ_REFILL_PERIOD_OFFS		20
#define MV_PP2_TXQ_REFILL_PERIOD_MAX		0x3FF
#define MV_PP2_TXQ_REFILL_PERIOD_ALL_MASK	(MV_PP2_TXQ_REFILL_PERIOD_MAX << MV_PP2_TXQ_REFILL_PERIOD_OFFS)
#define MV_PP2_TXQ_REFILL_PERIOD_MASK(val)	((val) << MV_PP2_TXQ_REFILL_PERIOD_OFFS)
/*-----------------------------------------------------------------------------------------------*/

/* Queue Maximum Token Bucket Size (QMTBS) */
#define MV_PP2_TXQ_SCHED_TOKEN_SIZE_REG(q)	(MV_PP2_REG_BASE + 0x8060 + ((q) << 2))
#define MV_PP2_TXQ_TOKEN_SIZE_MAX		0x7FFFFFFF
/*-----------------------------------------------------------------------------------------------*/

/* Queue Token Bucket Counter (PMTBS) */
#define MV_PP2_TXQ_SCHED_TOKEN_CNTR_REG(q)	(MV_PP2_REG_BASE + 0x8080 + ((q) << 2))
#define MV_PP2_TXQ_TOKEN_CNTR_MAX		0xFFFFFFFF
/*-----------------------------------------------------------------------------------------------*/

/* Transmit Queue Arbiter Configuration (TQxAC) */
#define MV_PP2_TXQ_SCHED_WRR_REG(q)		(MV_PP2_REG_BASE + 0x80A0 + ((q) << 2))

#define MV_PP2_TXQ_WRR_WEIGHT_OFFS		0
#define MV_PP2_TXQ_WRR_WEIGHT_MAX		0xFF
#define MV_PP2_TXQ_WRR_WEIGHT_ALL_MASK		(MV_PP2_TXQ_WRR_WEIGHT_MAX << MV_PP2_TXQ_WRR_WEIGHT_OFFS)
#define MV_PP2_TXQ_WRR_WEIGHT_MASK(weigth)	((weigth) << MV_PP2_TXQ_WRR_WEIGHT_OFFS)

#define MV_PP2_TXQ_WRR_BYTE_COUNT_OFFS		8
#define MV_PP2_TXQ_WRR_BYTE_COUNT_MASK		(0x3FFFF << MV_PP2_TXQ_WRR_BYTE_COUNT_OFFS)

/************************** PPv2 HW defines ******************************/
#define MV_PP2_RX_FIFO_PORT_DATA_SIZE		0x2000
#define MV_PP2_RX_FIFO_PORT_ATTR_SIZE		0x80
#define MV_PP2_RX_FIFO_PORT_MIN_PKT		0x80

#define MV_PP2_MAX_PORTS			8 	/* Maximum number of ports supported by PPv2 HW */
#define MV_PP2_MAX_RXQS_TOTAL			256	/* Maximum number of RXQs supported by PPv2 HW for all ports */

#define MV_PP2_DESC_ALIGNED_SIZE		32
#define MV_PP2_DESC_Q_ALIGN			512
/************************** RX/TX Descriptor defines and inlines ******************************/
/* RXQ */
typedef struct pp2_rx_desc {
	MV_U32 status;
	MV_U16 parserInfo;
	MV_U16 dataSize;
	MV_U32 bufPhysAddr;
	MV_U32 bufCookie;
	MV_U16 gemPortIdPktColor;
	MV_U16 csumL4;
	MV_U8  bmQset;
	MV_U8  reserved;
	MV_U16 classifyInfo;
	MV_U32 flowId;
	MV_U32 reserved2;
} PP2_RX_DESC;

/* Bits of "status" field */
#define PP2_RX_L3_OFFSET_OFFS			0
#define PP2_RX_L3_OFFSET_MASK			(0x7F << PP2_RX_L3_OFFSET_OFFS)

#define PP2_RX_IP_HLEN_OFFS			8
#define PP2_RX_IP_HLEN_MASK			(0x1F << PP2_RX_IP_HLEN_OFFS)

#define PP2_RX_ERR_CODE_OFFS			13
#define PP2_RX_ERR_CODE_MASK			(3 << PP2_RX_ERR_CODE_OFFS)
#define PP2_RX_ERR_CRC				(0 << PP2_RX_ERR_CODE_OFFS)
#define PP2_RX_ERR_OVERRUN			(1 << PP2_RX_ERR_CODE_OFFS)
#define PP2_RX_RESERVED				(2 << PP2_RX_ERR_CODE_OFFS)
#define PP2_RX_ERR_RESOURCE			(3 << PP2_RX_ERR_CODE_OFFS)

#define PP2_RX_ES_BIT				15
#define PP2_RX_ES_MASK				(1 << PP2_RX_ES_BIT)

#define PP2_RX_BM_POOL_ID_OFFS			16
#define PP2_RX_BM_POOL_ALL_MASK			(0x7 << PP2_RX_BM_POOL_ID_OFFS)
#define PP2_RX_BM_POOL_ID_MASK(pool)		((pool) << PP2_RX_BM_POOL_ID_OFFS)

#define PP2_RX_HWF_SYNC_BIT			21
#define PP2_RX_HWF_SYNC_MASK			(1 << PP2_RX_HWF_SYNC_BIT)

#define PP2_RX_L4_CHK_OK_BIT			22
#define PP2_RX_L4_CHK_OK_MASK			(1 << PP2_RX_L4_CHK_OK_BIT)

#define PP2_RX_IP_FRAG_BIT			23
#define PP2_RX_IP_FRAG_MASK			(1 << PP2_RX_IP_FRAG_BIT)

#define PP2_RX_IP4_HEADER_ERR_BIT		24
#define PP2_RX_IP4_HEADER_ERR_MASK		(1 << PP2_RX_IP4_HEADER_ERR_BIT)

#define PP2_RX_L4_OFFS				25
#define PP2_RX_L4_MASK				(7 << PP2_RX_L4_OFFS)
/* Value 0 - N/A, 3-7 - User Defined */
#define PP2_RX_L4_TCP				(1 << PP2_RX_L4_OFFS)
#define PP2_RX_L4_UDP				(2 << PP2_RX_L4_OFFS)

#define PP2_RX_L3_OFFS				28
#define PP2_RX_L3_MASK				(7 << PP2_RX_L3_OFFS)
/* Value 0 - N/A, 6-7 - User Defined */
#define PP2_RX_L3_IP4				(1 << PP2_RX_L3_OFFS)
#define PP2_RX_L3_IP4_OPT			(2 << PP2_RX_L3_OFFS)
#define PP2_RX_L3_IP4_OTHER			(3 << PP2_RX_L3_OFFS)
#define PP2_RX_L3_IP6				(4 << PP2_RX_L3_OFFS)
#define PP2_RX_L3_IP6_EXT			(5 << PP2_RX_L3_OFFS)

#define PP2_RX_BUF_HDR_BIT			31
#define PP2_RX_BUF_HDR_MASK			(1 << PP2_RX_BUF_HDR_BIT)

/* status field MACROs */
#define PP2_RX_L3_IS_IP4(status)		(((status) & PP2_RX_L3_MASK) == PP2_RX_L3_IP4)
#define PP2_RX_L3_IS_IP4_OPT(status)		(((status) & PP2_RX_L3_MASK) == PP2_RX_L3_IP4_OPT)
#define PP2_RX_L3_IS_IP4_OTHER(status)		(((status) & PP2_RX_L3_MASK) == PP2_RX_L3_IP4_OTHER)
#define PP2_RX_L3_IS_IP6(status)		(((status) & PP2_RX_L3_MASK) == PP2_RX_L3_IP6)
#define PP2_RX_L3_IS_IP6_EXT(status)		(((status) & PP2_RX_L3_MASK) == PP2_RX_L3_IP6_EXT)
#define PP2_RX_L4_IS_UDP(status)		(((status) & PP2_RX_L4_MASK) == PP2_RX_L4_UDP)
#define PP2_RX_L4_IS_TCP(status)		(((status) & PP2_RX_L4_MASK) == PP2_RX_L4_TCP)
#define PP2_RX_IP4_HDR_ERR(status)		((status) & PP2_RX_IP4_HEADER_ERR_MASK)
#define PP2_RX_IP4_FRG(status)			((status) & PP2_RX_IP_FRAG_MASK)
#define PP2_RX_L4_CHK_OK(status)		((status) & PP2_RX_L4_CHK_OK_MASK)

/* Sub fields of "parserInfo" field */
#define PP2_RX_LKP_ID_OFFS			0
#define PP2_RX_LKP_ID_BITS			6
#define PP2_RX_LKP_ID_MASK			(((1 << PP2_RX_LKP_ID_BITS) - 1) << PP2_RX_LKP_ID_OFFS)

#define PP2_RX_CPU_CODE_OFFS			6
#define PP2_RX_CPU_CODE_BITS			3
#define PP2_RX_CPU_CODE_MASK			(((1 << PP2_RX_CPU_CODE_BITS) - 1) << PP2_RX_CPU_CODE_OFFS)

#define PP2_RX_PPPOE_BIT			9
#define PP2_RX_PPPOE_MASK			(1 << PP2_RX_PPPOE_BIT)

#define PP2_RX_L3_CAST_OFFS			10
#define PP2_RX_L3_CAST_BITS			2
#define PP2_RX_L3_CAST_MASK			(((1 << PP2_RX_L3_CAST_BITS) - 1) << PP2_RX_L3_CAST_OFFS)

#define PP2_RX_L2_CAST_OFFS			12
#define PP2_RX_L2_CAST_BITS			2
#define PP2_RX_L2_CAST_MASK			(((1 << PP2_RX_L2_CAST_BITS) - 1) << PP2_RX_L2_CAST_OFFS)

#define PP2_RX_VLAN_INFO_OFFS			14
#define PP2_RX_VLAN_INFO_BITS			2
#define PP2_RX_VLAN_INFO_MASK			(((1 << PP2_RX_VLAN_INFO_BITS) - 1) << PP2_RX_VLAN_INFO_OFFS)

/* Bits of "bmQset" field */
#define PP2_RX_BUFF_QSET_NUM_OFFS		0
#define PP2_RX_BUFF_QSET_NUM_MASK		(0x7f << PP2_RX_BUFF_QSET_NUM_OFFS)

#define PP2_RX_BUFF_TYPE_OFFS			7
#define PP2_RX_BUFF_TYPE_MASK			(0x1 << PP2_RX_BUFF_TYPE_OFFS)
/*-------------------------------------------------------------------------------*/

/* TXQ */
typedef struct pp2_tx_desc {
	MV_U32 command;
	MV_U8  pktOffset;
	MV_U8  physTxq;
	MV_U16 dataSize;
	MV_U32 bufPhysAddr;
	MV_U32 bufCookie;
	MV_U32 hwCmd[3];
	MV_U32 reserved;
} PP2_TX_DESC;

/* Bits of "command" field */
#define PP2_TX_L3_OFFSET_OFFS			0
#define PP2_TX_L3_OFFSET_MASK			(0x7F << PP2_TX_L3_OFFSET_OFFS)

#define PP2_TX_BUF_RELEASE_MODE_BIT		7
#define PP2_TX_BUF_RELEASE_MODE_MASK		(1 << PP2_TX_BUF_RELEASE_MODE_BIT)

#define PP2_TX_IP_HLEN_OFFS			8
#define PP2_TX_IP_HLEN_MASK			(0x1F << PP2_TX_IP_HLEN_OFFS)

#define PP2_TX_L4_CSUM_OFFS			13
#define PP2_TX_L4_CSUM_MASK			(3 << PP2_TX_L4_CSUM_OFFS)
#define PP2_TX_L4_CSUM				(0 << PP2_TX_L4_CSUM_OFFS)
#define PP2_TX_L4_CSUM_FRG			(1 << PP2_TX_L4_CSUM_OFFS)
#define PP2_TX_L4_CSUM_NOT			(2 << PP2_TX_L4_CSUM_OFFS)

#define PP2_TX_IP_CSUM_DISABLE_BIT		15
#define PP2_TX_IP_CSUM_DISABLE_MASK		(1 << PP2_TX_IP_CSUM_DISABLE_BIT)

/* 3 bits: 16..18 */
#define PP2_TX_POOL_INDEX_OFFS			16
#define PP2_TX_POOL_INDEX_MASK			(7 << PP2_TX_POOL_INDEX_OFFS)

/* bit 19 - Reserved */

#define PP2_TX_PKT_OFFS_9_BIT			20
#define PP2_TX_PKT_OFFS_9_MASK			(1 << PP2_TX__PKT_OFFS_9_BIT)

#define PP2_TX_HWF_SYNC_BIT			21
#define PP2_TX_HWF_SYNC_MASK			(1 << PP2_TX_HWF_SYNC_BIT)

#define PP2_TX_HWF_BIT				22
#define PP2_TX_HWF_MASK				(1 << PP2_TX_HWF_BIT)

#define PP2_TX_PADDING_DISABLE_BIT		23
#define PP2_TX_PADDING_DISABLE_MASK		(1 << PP2_TX_PADDING_DISABLE_BIT)

#define PP2_TX_L4_OFFS				24
#define PP2_TX_L4_TCP				(0 << PP2_TX_L4_OFFS)
#define PP2_TX_L4_UDP				(1 << PP2_TX_L4_OFFS)

#define PP2_TX_L3_OFFS				26
#define PP2_TX_L3_IP4				(0 << PP2_TX_L3_OFFS)
#define PP2_TX_L3_IP6				(1 << PP2_TX_L3_OFFS)

#define PP2_TX_L_DESC_BIT			28
#define PP2_TX_L_DESC_MASK			(1 << PP2_TX_L_DESC_BIT)

#define PP2_TX_F_DESC_BIT			29
#define PP2_TX_F_DESC_MASK			(1 << PP2_TX_F_DESC_BIT)

#define PP2_TX_DESC_FRMT_BIT			30
#define PP2_TX_DESC_FRMT_MASK			(1 << PP2_TX_DESC_FRMT_BIT)
#define PP2_TX_DESC_PER_BUF			(0 << PP2_TX_DESC_FRMT_BIT)
#define PP2_TX_DESC_PER_PKT			(1 << PP2_TX_DESC_FRMT_BIT)

#define PP2_TX_BUF_HDR_BIT			31
#define PP2_TX_BUF_HDR_MASK			(1 << PP2_TX_BUF_HDR_BIT)

/* Bits of "hwCmd[0]" field - offset 0x10 */
#define PP2_TX_GEMPID_OFFS			0
#define PP2_TX_GEMPID_BITS			12
#define PP2_TX_GEMPID_ALL_MASK			(((1 << PP2_TX_GEMPID_BITS) - 1) << PP2_TX_GEMPID_OFFS)
#define PP2_TX_GEMPID_MASK(gpid)		(((gpid) & PP2_TX_GEMPID_ALL_MASK) << PP2_TX_GEMPID_OFFS)

#define PP2_TX_COLOR_OFFS			12
#define PP2_TX_COLOR_ALL_MASK			(0x3 << PP2_TX_COLOR_OFFS)
#define PP2_TX_COLOR_GREEN			0
#define PP2_TX_COLOR_YELLOW			1
#define PP2_TX_COLOR_MASK(col)			(((col) & PP2_TX_COLOR_ALL_MASK) << PP2_TX_COLOR_OFFS)

#define PP2_TX_DSA_OFFS				14
#define PP2_TX_DSA_ALL_MASK			(0x3 << PP2_TX_DSA_OFFS)
#define PP2_TX_DSA_NONE				0
#define PP2_TX_DSA_TAG				1
#define PP2_TX_EDSA_TAG				2
#define PP2_TX_DSA_MASK(dsa)			(((dsa) & PP2_TX_DSA_ALL_MASK) << PP2_TX_DSA_OFFS)

#define PP2_TX_L4_CSUM_INIT_OFFS		16
#define PP2_TX_L4_CSUM_INIT_MASK		(0xffff << PP2_TX_L4_CSUM_INIT_OFFS)

/* Bits of "hwCmd[1]" field - offset 0x14 */

#define PP2_TX_MOD_QSET_OFFS			0
#define PP2_TX_MOD_QSET_BITS			7
#define PP2_TX_MOD_QSET_MASK			(((1 << PP2_TX_MOD_QSET_BITS) - 1) << PP2_TX_MOD_QSET_OFFS)

#define PP2_TX_MOD_GRNTD_BIT			7
#define PP2_TX_MOD_GRNTD_MASK			(1 <<  PP2_TX_MOD_GRNTD_BIT)

/* bits 8..15 are reserved */

#define PP2_TX_MOD_DSCP_OFFS			16
#define PP2_TX_MOD_DSCP_BITS			6
#define PP2_TX_MOD_DSCP_MASK			(((1 << PP2_TX_MOD_DSCP_BITS) - 1) << PP2_TX_MOD_DSCP_OFFS)

#define PP2_TX_MOD_PRIO_OFFS			22
#define PP2_TX_MOD_PRIO_BITS			3
#define PP2_TX_MOD_PRIO_MASK			(((1 << PP2_TX_MOD_PRIO_BITS) - 1) << PP2_TX_MOD_PRIO_OFFS)

#define PP2_TX_MOD_DSCP_EN_BIT			25
#define PP2_TX_MOD_DSCP_EN_MASK			(1 << PP2_TX_MOD_DSCP_EN_BIT)

#define PP2_TX_MOD_PRIO_EN_BIT			26
#define PP2_TX_MOD_PRIO_EN_MASK			(1 << PP2_TX_MOD_PRIO_EN_BIT)

#define PP2_TX_MOD_GEMPID_EN_BIT		27
#define PP2_TX_MOD_GEMPID_EN_MASK		(1 << PP2_TX_MOD_GEMPID_EN_BIT)

/* Bits of "hwCmd[2]" field - offset 0x18 */
#define PP2_TX_PME_DPTR_OFFS			0
#define PP2_TX_PME_DPTR_ALL_MASK		(0xffff << PP2_TX_PME_DPTR_OFFS)
#define PP2_TX_PME_DPTR_MASK(val)		(((val) & PP2_TX_PME_DPTR_ALL_MASK) << PP2_TX_PME_DPTR_OFFS)

#define PP2_TX_PME_IPTR_OFFS			16
#define PP2_TX_PME_IPTR_ALL_MASK		(0xff << PP2_TX_PME_IPTR_OFFS)
#define PP2_TX_PME_IPTR_MASK(val)		(((val) & PP2_TX_PME_IPTR_ALL_MASK) << PP2_TX_PME_IPTR_OFFS)

/* Bit 24 - HWF_IDB is for HWF usage only */

#define PP2_TX_GEM_OEM_BIT			25
#define PP2_TX_GEM_OEM_MASK			(1 << PP2_TX_GEM_OEM_BIT)

/* Bit 26 - ERROR_SUM is for HWF usage only */

#define PP2_TX_PON_FEC_BIT			27
#define PP2_TX_PON_FEC_MASK			(1 << PP2_TX_PON_FEC_BIT)

#define PP2_TX_CPU_MAP_OFFS			28
#define PP2_TX_CPU_MAP_BITS			4
#define PP2_TX_CPU_MAP_MASK			(((1 << PP2_TX_CPU_MAP_BITS) - 1) << PP2_TX_CPU_MAP_OFFS)


/************************** Buffer Header defines ******************************/
typedef struct pp2_buff_hdr {
	MV_U32 nextBuffPhysAddr;
	MV_U32 nextBuffVirtAddr;
	MV_U16 byteCount;
	MV_U16 info;
	MV_U8  bmQset;
} PP2_BUFF_HDR;



/* info bits */
#define PP2_BUFF_HDR_INFO_MC_ID_OFFS		0
#define PP2_BUFF_HDR_INFO_MC_ID_MASK		(0xfff << PP2_BUFF_HDR_INFO_MC_ID_OFFS)
#define PP2_BUFF_HDR_INFO_MC_ID(info)		((info & PP2_BUFF_HDR_INFO_MC_ID_MASK) >> PP2_BUFF_HDR_INFO_MC_ID_OFFS)

#define PP2_BUFF_HDR_INFO_LAST_OFFS		12
#define PP2_BUFF_HDR_INFO_LAST_MASK		(0x1 << PP2_BUFF_HDR_INFO_LAST_OFFS)
#define PP2_BUFF_HDR_INFO_IS_LAST(info)		((info & PP2_BUFF_HDR_INFO_LAST_MASK) >> PP2_BUFF_HDR_INFO_LAST_OFFS)

/* bmQset bits */
#define PP2_BUFF_HDR_BM_QSET_NUM_OFFS		0
#define PP2_BUFF_HDR_BM_QSET_NUM_MASK		(0x7f << PP2_BUFF_HDR_BM_QSET_NUM_OFFS)

#define PP2_BUFF_HDR_BM_QSET_TYPE_OFFS		7
#define PP2_BUFF_HDR_BM_QSET_TYPE_MASK		(0x1 << PP2_BUFF_HDR_BM_QSET_TYPE_OFFS)

/************************** Ethernet misc ******************************/

#define ETH_MAX_DECODE_WIN			6
#define ETH_MAX_HIGH_ADDR_REMAP_WIN		4

/**** Address decode registers ****/

#define ETH_WIN_BASE_REG(win)			(MV_PP2_REG_BASE + 0x4000 + ((win) << 2))
#define ETH_WIN_SIZE_REG(win)			(MV_PP2_REG_BASE + 0x4020 + ((win) << 2))
#define ETH_WIN_REMAP_REG(win)			(MV_PP2_REG_BASE + 0x4040 + ((win) << 2))
#define ETH_BASE_ADDR_ENABLE_REG		(MV_PP2_REG_BASE + 0x4060)

/* The target associated with this window*/
#define ETH_WIN_TARGET_OFFS			0
#define ETH_WIN_TARGET_MASK			(0xf << ETH_WIN_TARGET_OFFS)
/* The target attributes associated with window */
#define ETH_WIN_ATTR_OFFS			8
#define ETH_WIN_ATTR_MASK			(0xff << ETH_WIN_ATTR_OFFS)

/* The Base address associated with window */
#define ETH_WIN_BASE_OFFS			16
#define ETH_WIN_BASE_MASK			(0xFFFF << ETH_WIN_BASE_OFFS)

#define ETH_WIN_SIZE_OFFS			16
#define ETH_WIN_SIZE_MASK			(0xFFFF << ETH_WIN_SIZE_OFFS)
/*-----------------------------------------------------------------------------------------------*/

#define ETH_TARGET_DEF_ADDR_REG			(MV_PP2_REG_BASE + 0x4064)
#define ETH_TARGET_DEF_ID_REG			(MV_PP2_REG_BASE + 0x4068)
/*-----------------------------------------------------------------------------------------------*/

#endif /* __MV_PP2_GBE_REGS_H__ */
