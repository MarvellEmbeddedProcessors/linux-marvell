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


#ifndef __INCmvEthRegsh
#define __INCmvEthRegsh

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#include "mvNetaRegs.h"


#define ETH_MAX_DECODE_WIN              6
#define ETH_MAX_HIGH_ADDR_REMAP_WIN     4

/****************************************/
/*        Ethernet Unit Registers       */
/****************************************/
#ifdef CONFIG_OF
extern int port_vbase[MV_ETH_MAX_PORTS];

#define ETH_REG_BASE(port)                  port_vbase[port]
#else /* CONFIG_OF */
#define ETH_REG_BASE(port)                  MV_ETH_REGS_BASE(port)
#endif /* CONFIG_OF */

#define ETH_PHY_ADDR_REG(port)              (ETH_REG_BASE(port) + 0x2000)
#define ETH_SMI_REG(port)                   (ETH_REG_BASE(port) + 0x2004)
#define ETH_UNIT_DEF_ADDR_REG(port)         (ETH_REG_BASE(port) + 0x2008)
#define ETH_UNIT_DEF_ID_REG(port)           (ETH_REG_BASE(port) + 0x200c)
#define ETH_UNIT_RESERVED(port)             (ETH_REG_BASE(port) + 0x2014)
#define ETH_UNIT_INTR_CAUSE_REG(port)       (ETH_REG_BASE(port) + 0x2080)
#define ETH_UNIT_INTR_MASK_REG(port)        (ETH_REG_BASE(port) + 0x2084)

#define ETH_UNIT_ERROR_ADDR_REG(port)       (ETH_REG_BASE(port) + 0x2094)
#define ETH_UNIT_INT_ADDR_ERROR_REG(port)   (ETH_REG_BASE(port) + 0x2098)

/* Ethernet Unit Control (EUC) register */
#define ETH_UNIT_CONTROL_REG(port)          (ETH_REG_BASE(port) + 0x20B0)

#define ETH_PHY_POLLING_ENABLE_BIT          1
#define ETH_PHY_POLLING_ENABLE_MASK        (1 << ETH_PHY_POLLING_ENABLE_BIT)

#define ETH_UNIT_PORT_RESET_BIT             24
#define ETH_UNIT_PORT_RESET_MASK            (1 << ETH_UNIT_PORT_RESET_BIT)
/*-----------------------------------------------------------------------------------------------*/

/**** Address decode registers ****/

#define ETH_WIN_BASE_REG(port, win)         (ETH_REG_BASE(port) + 0x2200 + ((win) << 3))
#define ETH_WIN_SIZE_REG(port, win)         (ETH_REG_BASE(port) + 0x2204 + ((win) << 3))
#define ETH_WIN_REMAP_REG(port, win)        (ETH_REG_BASE(port) + 0x2280 + ((win) << 2))
#define ETH_BASE_ADDR_ENABLE_REG(port)      (ETH_REG_BASE(port) + 0x2290)
#define ETH_ACCESS_PROTECT_REG(port)        (ETH_REG_BASE(port) + 0x2294)

/* The target associated with this window*/
#define ETH_WIN_TARGET_OFFS                 0
#define ETH_WIN_TARGET_MASK                 (0xf << ETH_WIN_TARGET_OFFS)
/* The target attributes associated with window */
#define ETH_WIN_ATTR_OFFS                   8
#define ETH_WIN_ATTR_MASK                   (0xff << ETH_WIN_ATTR_OFFS)

/* The Base address associated with window */
#define ETH_WIN_BASE_OFFS		            16
#define ETH_WIN_BASE_MASK		            (0xFFFF << ETH_WIN_BASE_OFFS)

#define ETH_WIN_SIZE_OFFS		            16
#define ETH_WIN_SIZE_MASK		            (0xFFFF << ETH_WIN_SIZE_OFFS)

 /* Ethernet Port Access Protect Register (EPAPR) */
#define ETH_PROT_NO_ACCESS                  0
#define ETH_PROT_READ_ONLY                  1
#define ETH_PROT_FULL_ACCESS                3
#define ETH_PROT_WIN_OFFS(winNum)           (2 * (winNum))
#define ETH_PROT_WIN_MASK(winNum)           (0x3 << ETH_PROT_WIN_OFFS(winNum))
/*-----------------------------------------------------------------------------------------------*/


/***** Port Configuration reg (PxCR) *****/
#define ETH_PORT_CONFIG_REG(port)           (ETH_REG_BASE(port) + 0x2400)

#define ETH_UNICAST_PROMISCUOUS_MODE_BIT    0
#define ETH_UNICAST_PROMISCUOUS_MODE_MASK   (1 << ETH_UNICAST_PROMISCUOUS_MODE_BIT)

#define ETH_DEF_RX_QUEUE_OFFSET             1
#define ETH_DEF_RX_QUEUE_ALL_MASK           (0x7 << ETH_DEF_RX_QUEUE_OFFSET)
#define ETH_DEF_RX_QUEUE_MASK(queue)        ((queue) << ETH_DEF_RX_QUEUE_OFFSET)

#define ETH_DEF_RX_ARP_QUEUE_OFFSET         4
#define ETH_DEF_RX_ARP_QUEUE_ALL_MASK       (0x7 << ETH_DEF_RX_ARP_QUEUE_OFFSET)
#define ETH_DEF_RX_ARP_QUEUE_MASK(queue)    ((queue) << ETH_DEF_RX_ARP_QUEUE_OFFSET)

#define ETH_REJECT_NOT_IP_ARP_BCAST_BIT     7
#define ETH_REJECT_NOT_IP_ARP_BCAST_MASK    (1 << ETH_REJECT_NOT_IP_ARP_BCAST_BIT)

#define ETH_REJECT_IP_BCAST_BIT             8
#define ETH_REJECT_IP_BCAST_MASK            (1 << ETH_REJECT_IP_BCAST_BIT)

#define ETH_REJECT_ARP_BCAST_BIT            9
#define ETH_REJECT_ARP_BCAST_MASK           (1 << ETH_REJECT_ARP_BCAST_BIT)

#define ETH_TX_NO_SET_ERROR_SUMMARY_BIT     12
#define ETH_TX_NO_SET_ERROR_SUMMARY_MASK    (1 << ETH_TX_NO_SET_ERROR_SUMMARY_BIT)

#define ETH_CAPTURE_TCP_FRAMES_ENABLE_BIT   14
#define ETH_CAPTURE_TCP_FRAMES_ENABLE_MASK  (1 << ETH_CAPTURE_TCP_FRAMES_ENABLE_BIT)

#define ETH_CAPTURE_UDP_FRAMES_ENABLE_BIT   15
#define ETH_CAPTURE_UDP_FRAMES_ENABLE_MASK  (1 << ETH_CAPTURE_UDP_FRAMES_ENABLE_BIT)

#define ETH_DEF_RX_TCP_QUEUE_OFFSET         16
#define ETH_DEF_RX_TCP_QUEUE_ALL_MASK       (0x7 << ETH_DEF_RX_TCP_QUEUE_OFFSET)
#define ETH_DEF_RX_TCP_QUEUE_MASK(queue)    ((queue) << ETH_DEF_RX_TCP_QUEUE_OFFSET)

#define ETH_DEF_RX_UDP_QUEUE_OFFSET         19
#define ETH_DEF_RX_UDP_QUEUE_ALL_MASK       (0x7 << ETH_DEF_RX_UDP_QUEUE_OFFSET)
#define ETH_DEF_RX_UDP_QUEUE_MASK(queue)    ((queue) << ETH_DEF_RX_UDP_QUEUE_OFFSET)

#define ETH_DEF_RX_BPDU_QUEUE_OFFSET        22
#define ETH_DEF_RX_BPDU_QUEUE_ALL_MASK      (0x7 << ETH_DEF_RX_BPDU_QUEUE_OFFSET)
#define ETH_DEF_RX_BPDU_QUEUE_MASK(queue)   ((queue) << ETH_DEF_RX_BPDU_QUEUE_OFFSET)

#define ETH_RX_CHECKSUM_MODE_OFFSET         25
#define ETH_RX_CHECKSUM_NO_PSEUDO_HDR       (0 << ETH_RX_CHECKSUM_MODE_OFFSET)
#define ETH_RX_CHECKSUM_WITH_PSEUDO_HDR     (1 << ETH_RX_CHECKSUM_MODE_OFFSET)
/*-----------------------------------------------------------------------------------------------*/

/***** Port Configuration Extend reg (PxCXR) *****/
#define ETH_PORT_CONFIG_EXTEND_REG(port)    (ETH_REG_BASE(port) + 0x2404)

#define ETH_CAPTURE_SPAN_BPDU_ENABLE_BIT    1
#define ETH_CAPTURE_SPAN_BPDU_ENABLE_MASK   (1 << ETH_CAPTURE_SPAN_BPDU_ENABLE_BIT)

#define ETH_TX_DISABLE_GEN_CRC_BIT          3
#define ETH_TX_DISABLE_GEN_CRC_MASK         (1 << ETH_TX_DISABLE_GEN_CRC_BIT)
/*-----------------------------------------------------------------------------------------------*/

#define ETH_VLAN_ETHER_TYPE_REG(port)       (ETH_REG_BASE(port) + 0x2410)
#define ETH_MAC_ADDR_LOW_REG(port)          (ETH_REG_BASE(port) + 0x2414)
#define ETH_MAC_ADDR_HIGH_REG(port)         (ETH_REG_BASE(port) + 0x2418)


/***** Port Sdma Configuration reg (SDCR) *****/
#define ETH_SDMA_CONFIG_REG(port)           (ETH_REG_BASE(port) + 0x241c)

#define ETH_RX_FRAME_INTERRUPT_BIT          0
#define ETH_RX_FRAME_INTERRUPT_MASK         (1 << ETH_RX_FRAME_INTERRUPT_BIT)

#define ETH_BURST_SIZE_1_64BIT_VALUE        0
#define ETH_BURST_SIZE_2_64BIT_VALUE        1
#define ETH_BURST_SIZE_4_64BIT_VALUE        2
#define ETH_BURST_SIZE_8_64BIT_VALUE        3
#define ETH_BURST_SIZE_16_64BIT_VALUE       4

#define ETH_RX_BURST_SIZE_OFFSET            1
#define ETH_RX_BURST_SIZE_ALL_MASK          (0x7 << ETH_RX_BURST_SIZE_OFFSET)
#define ETH_RX_BURST_SIZE_MASK(burst)       ((burst) << ETH_RX_BURST_SIZE_OFFSET)

#define ETH_RX_NO_DATA_SWAP_BIT             4
#define ETH_RX_NO_DATA_SWAP_MASK            (1 << ETH_RX_NO_DATA_SWAP_BIT)
#define ETH_RX_DATA_SWAP_MASK               (0 << ETH_RX_NO_DATA_SWAP_BIT)

#define ETH_TX_NO_DATA_SWAP_BIT             5
#define ETH_TX_NO_DATA_SWAP_MASK            (1 << ETH_TX_NO_DATA_SWAP_BIT)
#define ETH_TX_DATA_SWAP_MASK               (0 << ETH_TX_NO_DATA_SWAP_BIT)

#define ETH_DESC_SWAP_BIT                   6
#define ETH_DESC_SWAP_MASK                  (1 << ETH_DESC_SWAP_BIT)
#define ETH_NO_DESC_SWAP_MASK               (0 << ETH_DESC_SWAP_BIT)

#define ETH_TX_BURST_SIZE_OFFSET            22
#define ETH_TX_BURST_SIZE_ALL_MASK          (0x7 << ETH_TX_BURST_SIZE_OFFSET)
#define ETH_TX_BURST_SIZE_MASK(burst)       ((burst) << ETH_TX_BURST_SIZE_OFFSET)
/*-----------------------------------------------------------------------------------------------*/

#define ETH_DIFF_SERV_PRIO_REG(port, code)  (ETH_REG_BASE(port) + 0x2420  + ((code) << 2))

/* Port Serial Control0 register (PSC0) */
#define ETH_PORT_SERIAL_CTRL_REG(port)      (ETH_REG_BASE(port) + 0x243c)

#define ETH_TX_FC_MODE_OFFSET               5
#define ETH_TX_FC_MODE_MASK                 (3 << ETH_TX_FC_MODE_OFFSET)
#define ETH_TX_FC_NO_PAUSE                  (0 << ETH_TX_FC_MODE_OFFSET)
#define ETH_TX_FC_SEND_PAUSE                (1 << ETH_TX_FC_MODE_OFFSET)

#define ETH_TX_BP_MODE_OFFSET               7
#define ETH_TX_BP_MODE_MASK                 (3 << ETH_TX_BP_MODE_OFFSET)
#define ETH_TX_BP_NO_JAM                    (0 << ETH_TX_BP_MODE_OFFSET)
#define ETH_TX_BP_SEND_JAM                  (1 << ETH_TX_BP_MODE_OFFSET)

#define ETH_RETRANSMIT_FOREVER_BIT          11
#define ETH_RETRANSMIT_FOREVER_MASK         (1 << ETH_RETRANSMIT_FOREVER_BIT)

#define ETH_DTE_ADVERT_BIT                  14
#define ETH_DTE_ADVERT_MASK                 (1 << ETH_DTE_ADVERT_BIT)

/* Other bits are different for new GMAC and old GMAC modules */
#ifdef MV_ETH_GMAC_NEW

#define ETH_IGNORE_RX_ERR_BIT               28
#define ETH_IGNORE_RX_ERR_MASK              (1 << ETH_IGNORE_RX_ERR_BIT)

#define ETH_IGNORE_COL_BIT                  29
#define ETH_IGNORE_COL_MASK                 (1 << ETH_IGNORE_COL_BIT)

#define ETH_IGNORE_CARRIER_SENSE_BIT        30
#define ETH_IGNORE_CARRIER_SENSE_MASK       (1 << ETH_IGNORE_CARRIER_SENSE_BIT)

#else /* Old GMAC */

#define ETH_PORT_ENABLE_BIT                 0
#define ETH_PORT_ENABLE_MASK                (1 << ETH_PORT_ENABLE_BIT)

#define ETH_FORCE_LINK_PASS_BIT             1
#define ETH_FORCE_LINK_PASS_MASK            (1 << ETH_FORCE_LINK_PASS_BIT)

#define ETH_DISABLE_DUPLEX_AUTO_NEG_BIT     2
#define ETH_DISABLE_DUPLEX_AUTO_NEG_MASK    (1 << ETH_DISABLE_DUPLEX_AUTO_NEG_BIT)

#define ETH_DISABLE_FC_AUTO_NEG_BIT         3
#define ETH_DISABLE_FC_AUTO_NEG_MASK        (1 << ETH_DISABLE_FC_AUTO_NEG_BIT)

#define ETH_ADVERTISE_SYM_FC_BIT            4
#define ETH_ADVERTISE_SYM_FC_MASK           (1 << ETH_ADVERTISE_SYM_FC_BIT)

#define ETH_DO_NOT_FORCE_LINK_FAIL_BIT      10
#define ETH_DO_NOT_FORCE_LINK_FAIL_MASK     (1 << ETH_DO_NOT_FORCE_LINK_FAIL_BIT)

#define ETH_DISABLE_SPEED_AUTO_NEG_BIT      13
#define ETH_DISABLE_SPEED_AUTO_NEG_MASK     (1 << ETH_DISABLE_SPEED_AUTO_NEG_BIT)

#define ETH_MAX_RX_PACKET_SIZE_OFFSET       17
#define ETH_MAX_RX_PACKET_SIZE_MASK         (7 << ETH_MAX_RX_PACKET_SIZE_OFFSET)
#define ETH_MAX_RX_PACKET_1518BYTE          (0 << ETH_MAX_RX_PACKET_SIZE_OFFSET)
#define ETH_MAX_RX_PACKET_1522BYTE          (1 << ETH_MAX_RX_PACKET_SIZE_OFFSET)
#define ETH_MAX_RX_PACKET_1552BYTE          (2 << ETH_MAX_RX_PACKET_SIZE_OFFSET)
#define ETH_MAX_RX_PACKET_9022BYTE          (3 << ETH_MAX_RX_PACKET_SIZE_OFFSET)
#define ETH_MAX_RX_PACKET_9192BYTE          (4 << ETH_MAX_RX_PACKET_SIZE_OFFSET)
#define ETH_MAX_RX_PACKET_9700BYTE          (5 << ETH_MAX_RX_PACKET_SIZE_OFFSET)

#define ETH_SET_FULL_DUPLEX_BIT             21
#define ETH_SET_FULL_DUPLEX_MASK            (1 << ETH_SET_FULL_DUPLEX_BIT)

#define ETH_SET_FLOW_CTRL_BIT               22
#define ETH_SET_FLOW_CTRL_MASK              (1 << ETH_SET_FLOW_CTRL_BIT)

#define ETH_SET_GMII_SPEED_1000_BIT         23
#define ETH_SET_GMII_SPEED_1000_MASK        (1 << ETH_SET_GMII_SPEED_1000_BIT)

#define ETH_SET_MII_SPEED_100_BIT           24
#define ETH_SET_MII_SPEED_100_MASK          (1 << ETH_SET_MII_SPEED_100_BIT)

#endif /* MV_ETH_GMAC_NEW */
/*-----------------------------------------------------------------------------------------------*/

#define ETH_VLAN_TAG_TO_PRIO_REG(port)      (ETH_REG_BASE(port) + 0x2440)

/* Ethernet Type Priority register */
#define ETH_TYPE_PRIO_REG(port)             (ETH_REG_BASE(port) + 0x24BC)

#define ETH_TYPE_PRIO_ENABLE_BIT            0
#define ETH_TYPE_PRIO_FORCE_BIT             1

#define ETH_TYPE_PRIO_RXQ_OFFS              2
#define ETH_TYPE_PRIO_RXQ_ALL_MASK          (0x7 << ETH_TYPE_PRIO_RXQ_OFFS)
#define ETH_TYPE_PRIO_RXQ_MASK(rxq)         ((rxq) << ETH_TYPE_PRIO_RXQ_OFFS)

#define ETH_TYPE_PRIO_VALUE_OFFS            5
#define ETH_TYPE_PRIO_VALUE_ALL_MASK        (0xFFFF << ETH_TYPE_PRIO_VALUE_OFFS)
#define ETH_TYPE_PRIO_VALUE_MASK(type)      (type << ETH_TYPE_PRIO_VALUE_OFFS)

#define ETH_FORCE_UNICAST_BIT               21
#define ETH_FORCE_UNICAST_MASK              (1 << ETH_FORCE_UNICAST_BIT)
/*-----------------------------------------------------------------------------------------------*/

/***** Ethernet Port Status reg (PSR) *****/
#define ETH_PORT_STATUS_REG(port)           (ETH_REG_BASE(port) + 0x2444)

/* Other bits are different for new GMAC and old GMAC modules */
#ifdef MV_ETH_GMAC_NEW

#define ETH_TX_IN_PROGRESS_OFFS             0
#define ETH_TX_IN_PROGRESS_MASK(txp)        (1 << ((txp) + ETH_TX_IN_PROGRESS_OFFS))
#define ETH_TX_IN_PROGRESS_ALL_MASK         (0xFF << ETH_TX_IN_PROGRESS_OFFS)

#define ETH_TX_FIFO_EMPTY_OFFS              8
#define ETH_TX_FIFO_EMPTY_MASK(txp)         (1 << ((txp) + ETH_TX_FIFO_EMPTY_OFFS))
#define ETH_TX_FIFO_EMPTY_ALL_MASK          (0xFF << ETH_TX_FIFO_EMPTY_OFFS)

#define ETH_RX_FIFO_EMPTY_BIT               16
#define ETH_RX_FIFO_EMPTY_MASK              (1 << ETH_RX_FIFO_EMPTY_BIT)

#else /* Old GMAC */

#define ETH_LINK_UP_BIT                     1
#define ETH_LINK_UP_MASK                    (1 << ETH_LINK_UP_BIT)

#define ETH_FULL_DUPLEX_BIT                 2
#define ETH_FULL_DUPLEX_MASK                (1 << ETH_FULL_DUPLEX_BIT)

#define ETH_FLOW_CTRL_ENABLED_BIT        	3
#define ETH_FLOW_CTRL_ENABLED_MASK       	(1 << ETH_FLOW_CTRL_ENABLED_BIT)

#define ETH_GMII_SPEED_1000_BIT             4
#define ETH_GMII_SPEED_1000_MASK            (1 << ETH_GMII_SPEED_1000_BIT)

#define ETH_MII_SPEED_100_BIT               5
#define ETH_MII_SPEED_100_MASK              (1 << ETH_MII_SPEED_100_BIT)

#define ETH_TX_IN_PROGRESS_BIT              7
#define ETH_TX_IN_PROGRESS_MASK             (1 << ETH_TX_IN_PROGRESS_BIT)

#define ETH_TX_FIFO_EMPTY_BIT               10
#define ETH_TX_FIFO_EMPTY_MASK              (1 << ETH_TX_FIFO_EMPTY_BIT)

#define ETH_RX_FIFO_EMPTY_BIT               12
#define ETH_RX_FIFO_EMPTY_MASK              (1 << ETH_RX_FIFO_EMPTY_BIT)

#define PON_TX_IN_PROGRESS_OFFS             0
#define PON_TX_IN_PROGRESS_MASK(txp)        (1 << ((txp) + PON_TX_IN_PROGRESS_OFFS))
#define PON_TX_IN_PROGRESS_ALL_MASK         (0xFF << PON_TX_IN_PROGRESS_OFFS)

#define PON_TX_FIFO_EMPTY_OFFS              8
#define PON_TX_FIFO_EMPTY_MASK(txp)         (1 << ((txp) + PON_TX_FIFO_EMPTY_OFFS))
#define PON_TX_FIFO_EMPTY_ALL_MASK          (0xFF << PON_TX_FIFO_EMPTY_OFFS)

#endif /* MV_ETH_GMAC_NEW */
/*-----------------------------------------------------------------------------------------------*/


/***** Transmit Queue Command (TxQC) register *****/
#define ETH_TX_QUEUE_COMMAND_REG(p, txp)    (NETA_TX_REG_BASE((p), (txp)) + 0x0048)

#define ETH_TXQ_ENABLE_OFFSET               0
#define ETH_TXQ_ENABLE_MASK                 (0x000000FF << ETH_TXQ_ENABLE_OFFSET)

#define ETH_TXQ_DISABLE_OFFSET              8
#define ETH_TXQ_DISABLE_MASK                (0x000000FF << ETH_TXQ_DISABLE_OFFSET)
/*-----------------------------------------------------------------------------------------------*/

/* Marvell Header Register */
#define ETH_PORT_MARVELL_HEADER_REG(port)   (ETH_REG_BASE(port) + 0x2454)

#define ETH_MH_EN_BIT                       0
#define ETH_MH_EN_MASK                      (1 << ETH_MH_EN_BIT)

#define ETH_DSA_EN_OFFS                     10
#define ETH_DSA_EN_MASK                     (3 << ETH_DSA_EN_OFFS)
#define ETH_DSA_MASK                        (1 << ETH_DSA_EN_OFFS)
#define ETH_DSA_EXT_MASK                    (2 << ETH_DSA_EN_OFFS)
/*-----------------------------------------------------------------------------------------------*/

/* Ethernet Cause Register */
#define ETH_INTR_CAUSE_REG(port)            (ETH_REG_BASE(port) + 0x2460)

#define ETH_CAUSE_RX_READY_SUM_BIT          0
#define ETH_CAUSE_EXTEND_BIT                1

#define ETH_CAUSE_RX_READY_OFFSET           2
#define ETH_CAUSE_RX_READY_BIT(queue)       (ETH_CAUSE_RX_READY_OFFSET + (queue))
#define ETH_CAUSE_RX_READY_MASK(queue)      (1 << (ETH_CAUSE_RX_READY_BIT(queue)))

#define ETH_CAUSE_RX_ERROR_SUM_BIT          10
#define ETH_CAUSE_RX_ERROR_OFFSET           11
#define ETH_CAUSE_RX_ERROR_BIT(queue)       (ETH_CAUSE_RX_ERROR_OFFSET + (queue))
#define ETH_CAUSE_RX_ERROR_MASK(queue)      (1 << (ETH_CAUSE_RX_ERROR_BIT(queue)))

#define ETH_CAUSE_TX_END_BIT                19
#define ETH_CAUSE_SUM_BIT                   31
/*-----------------------------------------------------------------------------------------------*/

/* Ethernet Cause Extended Register */
#define ETH_INTR_CAUSE_EXT_REG(port)        (ETH_REG_BASE(port) + 0x2464)

#define ETH_CAUSE_TX_BUF_OFFSET             0
#define ETH_CAUSE_TX_BUF_BIT(queue)         (ETH_CAUSE_TX_BUF_OFFSET + (queue))
#define ETH_CAUSE_TX_BUF_MASK(queue)        (1 << (ETH_CAUSE_TX_BUF_BIT(queue)))

#define ETH_CAUSE_TX_ERROR_OFFSET           8
#define ETH_CAUSE_TX_ERROR_BIT(queue)       (ETH_CAUSE_TX_ERROR_OFFSET + (queue))
#define ETH_CAUSE_TX_ERROR_MASK(queue)      (1 << (ETH_CAUSE_TX_ERROR_BIT(queue)))

#define ETH_CAUSE_PHY_STATUS_CHANGE_BIT     16
#define ETH_CAUSE_RX_OVERRUN_BIT            18
#define ETH_CAUSE_TX_UNDERRUN_BIT           19
#define ETH_CAUSE_LINK_STATE_CHANGE_BIT     20
#define ETH_CAUSE_INTERNAL_ADDR_ERR_BIT     23
#define ETH_CAUSE_EXTEND_SUM_BIT            31
/*-----------------------------------------------------------------------------------------------*/

#define ETH_INTR_MASK_REG(port)             (ETH_REG_BASE(port) + 0x2468)
#define ETH_INTR_MASK_EXT_REG(port)         (ETH_REG_BASE(port) + 0x246c)
#define ETH_RX_MINIMAL_FRAME_SIZE_REG(port) (ETH_REG_BASE(port) + 0x247c)
#define ETH_RX_DISCARD_PKTS_CNTR_REG(port)  (ETH_REG_BASE(port) + 0x2484)
#define ETH_RX_OVERRUN_PKTS_CNTR_REG(port)  (ETH_REG_BASE(port) + 0x2488)
#define ETH_INTERNAL_ADDR_ERROR_REG(port)   (ETH_REG_BASE(port) + 0x2494)

/***** Receive Queue Command (RxQC) register *****/
#define ETH_RX_QUEUE_COMMAND_REG(port)      (ETH_REG_BASE(port) + 0x2680)

#define ETH_RXQ_ENABLE_OFFSET               0
#define ETH_RXQ_ENABLE_MASK                 (0x000000FF << ETH_RXQ_ENABLE_OFFSET)

#define ETH_RXQ_DISABLE_OFFSET              8
#define ETH_RXQ_DISABLE_MASK                (0x000000FF << ETH_RXQ_DISABLE_OFFSET)
/*-----------------------------------------------------------------------------------------------*/

#define ETH_MIB_COUNTERS_BASE(port, txp)    (ETH_REG_BASE(port) + 0x3000 + ((txp) * 0x80))
#define ETH_DA_FILTER_SPEC_MCAST_BASE(port) (ETH_REG_BASE(port) + 0x3400)
#define ETH_DA_FILTER_OTH_MCAST_BASE(port)  (ETH_REG_BASE(port) + 0x3500)
#define ETH_DA_FILTER_UCAST_BASE(port)      (ETH_REG_BASE(port) + 0x3600)

/* Phy address register definitions */
#define ETH_PHY_ADDR_OFFS          0
#define ETH_PHY_ADDR_MASK          (0x1f << ETH_PHY_ADDR_OFFS)

/* MIB Counters register definitions */
#define ETH_MIB_GOOD_OCTETS_RECEIVED_LOW    0x0
#define ETH_MIB_GOOD_OCTETS_RECEIVED_HIGH   0x4
#define ETH_MIB_BAD_OCTETS_RECEIVED         0x8
#define ETH_MIB_INTERNAL_MAC_TRANSMIT_ERR   0xc
#define ETH_MIB_GOOD_FRAMES_RECEIVED        0x10
#define ETH_MIB_BAD_FRAMES_RECEIVED         0x14
#define ETH_MIB_BROADCAST_FRAMES_RECEIVED   0x18
#define ETH_MIB_MULTICAST_FRAMES_RECEIVED   0x1c
#define ETH_MIB_FRAMES_64_OCTETS            0x20
#define ETH_MIB_FRAMES_65_TO_127_OCTETS     0x24
#define ETH_MIB_FRAMES_128_TO_255_OCTETS    0x28
#define ETH_MIB_FRAMES_256_TO_511_OCTETS    0x2c
#define ETH_MIB_FRAMES_512_TO_1023_OCTETS   0x30
#define ETH_MIB_FRAMES_1024_TO_MAX_OCTETS   0x34
#define ETH_MIB_GOOD_OCTETS_SENT_LOW        0x38
#define ETH_MIB_GOOD_OCTETS_SENT_HIGH       0x3c
#define ETH_MIB_GOOD_FRAMES_SENT            0x40
#define ETH_MIB_EXCESSIVE_COLLISION         0x44
#define ETH_MIB_MULTICAST_FRAMES_SENT       0x48
#define ETH_MIB_BROADCAST_FRAMES_SENT       0x4c
#define ETH_MIB_UNREC_MAC_CONTROL_RECEIVED  0x50
#define ETH_MIB_FC_SENT                     0x54
#define ETH_MIB_GOOD_FC_RECEIVED            0x58
#define ETH_MIB_BAD_FC_RECEIVED             0x5c
#define ETH_MIB_UNDERSIZE_RECEIVED          0x60
#define ETH_MIB_FRAGMENTS_RECEIVED          0x64
#define ETH_MIB_OVERSIZE_RECEIVED           0x68
#define ETH_MIB_JABBER_RECEIVED             0x6c
#define ETH_MIB_MAC_RECEIVE_ERROR           0x70
#define ETH_MIB_BAD_CRC_EVENT               0x74
#define ETH_MIB_COLLISION                   0x78
#define ETH_MIB_LATE_COLLISION              0x7c


#ifndef MV_ETH_GMAC_NEW
/*****************************************************/
/*        Registers are not exist in new GMAC    */
/*****************************************************/
#define ETH_MII_SERIAL_PARAM_REG(port)      (ETH_REG_BASE(port) + 0x2408)
#define ETH_GMII_SERIAL_PARAM_REG(port)     (ETH_REG_BASE(port) + 0x240c)

/* Port Serial Control1 (PSC1) */
#define ETH_PORT_SERIAL_CTRL_1_REG(port)    (ETH_REG_BASE(port) + 0x244c)
#define ETH_PSC_ENABLE_BIT                  2
#define ETH_PSC_ENABLE_MASK                 (1 << ETH_PSC_ENABLE_BIT)

#define ETH_RGMII_ENABLE_BIT                3
#define ETH_RGMII_ENABLE_MASK               (1 << ETH_RGMII_ENABLE_BIT)

#define ETH_PORT_RESET_BIT                  4
#define ETH_PORT_RESET_MASK                 (1 << ETH_PORT_RESET_BIT)

#define ETH_INBAND_AUTO_NEG_ENABLE_BIT      6
#define ETH_INBAND_AUTO_NEG_ENABLE_MASK     (1 << ETH_INBAND_AUTO_NEG_ENABLE_BIT)

#define ETH_INBAND_AUTO_NEG_BYPASS_BIT      7
#define ETH_INBAND_AUTO_NEG_BYPASS_MASK     (1 << ETH_INBAND_AUTO_NEG_BYPASS_BIT)

#define ETH_INBAND_AUTO_NEG_START_BIT       8
#define ETH_INBAND_AUTO_NEG_START_MASK      (1 << ETH_INBAND_AUTO_NEG_START_BIT)

#define ETH_PORT_TYPE_BIT                   11
#define ETH_PORT_TYPE_1000BasedX_MASK       (1 << ETH_PORT_TYPE_BIT)

#define ETH_SGMII_MODE_BIT                  12
#define ETH_1000BaseX_MODE_MASK             (0 << ETH_SGMII_MODE_BIT)
#define ETH_SGMII_MODE_MASK                 (1 << ETH_SGMII_MODE_BIT)

#define ETH_MGMII_MODE_BIT                  13

#define ETH_EN_MII_ODD_PRE_BIT		        22
#define ETH_EN_MII_ODD_PRE_MASK		        (1 << ETH_EN_MII_ODD_PRE_BIT)
/*-----------------------------------------------------------------------------------------------*/

/* Ethernet Port Status1 (PS1) */
#define ETH_PORT_STATUS_1_REG(port)         (ETH_REG_BASE(port) + 0x2450)
#define ETH_AUTO_NEG_DONE_BIT               4
#define ETH_AUTO_NEG_DONE_MASK              (1 << ETH_AUTO_NEG_DONE_BIT)
/*-----------------------------------------------------------------------------------------------*/

#define ETH_PORT_FIFO_PARAMS_REG(port)      (ETH_REG_BASE(port) + 0x2458)

#endif /* MV_ETH_GMAC_NEW */

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __INCmvEthRegsh */
