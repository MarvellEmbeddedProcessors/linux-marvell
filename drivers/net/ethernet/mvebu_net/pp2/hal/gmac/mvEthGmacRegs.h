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

#ifndef __mvEthGmacRegs_h__
#define __mvEthGmacRegs_h__

#ifdef CONFIG_ARCH_MVEBU
#include "mvNetConfig.h"
#else
#include "mvSysEthConfig.h"
#endif

#define ETH_MNG_EXTENDED_GLOBAL_CTRL_REG   (GOP_MNG_REG_BASE + 0x5c)

#define ETH_REG_BASE(port)                 GOP_REG_BASE(port)

/****************************************/
/*        MAC Unit Registers            */
/****************************************/

/**** Tri-Speed Ports MAC and CPU Port MAC Configuration Sub-Unit Registers ****/
#define ETH_GMAC_CTRL_0_REG(p)             (ETH_REG_BASE(p) + 0x0)

#define ETH_GMAC_PORT_EN_BIT               0
#define ETH_GMAC_PORT_EN_MASK              (1 << ETH_GMAC_PORT_EN_BIT)

#define ETH_GMAC_PORT_TYPE_BIT             1
#define ETH_GMAC_PORT_TYPE_MASK            (1 << ETH_GMAC_PORT_TYPE_BIT)
#define ETH_GMAC_PORT_TYPE_SGMII           (0 << ETH_GMAC_PORT_TYPE_BIT)
#define ETH_GMAC_PORT_TYPE_1000X           (1 << ETH_GMAC_PORT_TYPE_BIT)

#define ETH_GMAC_MAX_RX_SIZE_OFFS          2
#define ETH_GMAC_MAX_RX_SIZE_MASK          (0x1FFF << ETH_GMAC_MAX_RX_SIZE_OFFS)

#define ETH_GMAC_MIB_CNTR_EN_BIT           15
#define ETH_GMAC_MIB_CNTR_EN_MASK          (1 << ETH_GMAC_MIB_CNTR_EN_BIT)
/*-------------------------------------------------------------------------------*/

#define ETH_GMAC_CTRL_1_REG(p)             (ETH_REG_BASE(p) + 0x4)

#define ETH_GMAC_PERIODIC_XON_EN_BIT       1
#define ETH_GMAC_PERIODIC_XON_EN_MASK      (0x1 << ETH_GMAC_PERIODIC_XON_EN_BIT)

#define ETH_GMAC_GMII_LB_EN_BIT            5
#define ETH_GMAC_GMII_LB_EN_MASK           (1 << ETH_GMAC_GMII_LB_EN_BIT)

#define ETH_GMAC_PCS_LB_EN_BIT             6
#define ETH_GMAC_PCS_LB_EN_MASK            (1 << ETH_GMAC_PCS_LB_EN_BIT)

#define ETH_GMAC_SA_LOW_OFFS               7
#define ETH_GMAC_SA_LOW_MASK               (0xFF << ETH_GMAC_SA_LOW_OFFS)
/*-------------------------------------------------------------------------------*/

#define ETH_GMAC_CTRL_2_REG(p)             (ETH_REG_BASE(p) + 0x8)

#define ETH_GMAC_INBAND_AN_BIT            0
#define ETH_GMAC_INBAND_AN_MASK           (1 << ETH_GMAC_INBAND_AN_BIT)

#define ETH_GMAC_PCS_ENABLE_BIT            3
#define ETH_GMAC_PCS_ENABLE_MASK           (1 << ETH_GMAC_PCS_ENABLE_BIT)

#define ETH_GMAC_PORT_RGMII_BIT            4
#define ETH_GMAC_PORT_RGMII_MASK           (1 << ETH_GMAC_PORT_RGMII_BIT)

#define ETH_GMAC_PORT_RESET_BIT            6
#define ETH_GMAC_PORT_RESET_MASK           (1 << ETH_GMAC_PORT_RESET_BIT)
/*-------------------------------------------------------------------------------*/

/**** Port Auto-Negotiation Configuration Sub-Unit Registers ****/
#define ETH_GMAC_AN_CTRL_REG(p)                (ETH_REG_BASE(p) + 0xC)

#define ETH_FORCE_LINK_FAIL_BIT                0
#define ETH_FORCE_LINK_FAIL_MASK               (1 << ETH_FORCE_LINK_FAIL_BIT)

#define ETH_FORCE_LINK_PASS_BIT                1
#define ETH_FORCE_LINK_PASS_MASK               (1 << ETH_FORCE_LINK_PASS_BIT)

#define ETH_SET_MII_SPEED_100_BIT              5
#define ETH_SET_MII_SPEED_100_MASK             (1 << ETH_SET_MII_SPEED_100_BIT)

#define ETH_SET_GMII_SPEED_1000_BIT            6
#define ETH_SET_GMII_SPEED_1000_MASK           (1 << ETH_SET_GMII_SPEED_1000_BIT)

#define ETH_ENABLE_SPEED_AUTO_NEG_BIT          7
#define ETH_ENABLE_SPEED_AUTO_NEG_MASK         (1 << ETH_ENABLE_SPEED_AUTO_NEG_BIT)

#define ETH_ENABLE_FLOW_CTRL_AUTO_NEG_BIT      11
#define ETH_ENABLE_FLOW_CTRL_AUTO_NEG_MASK     (1 << ETH_ENABLE_FLOW_CTRL_AUTO_NEG_BIT)

/* TODO: I keep this bit even though it's not listed in Cider */
#define ETH_SET_FLOW_CONTROL_BIT               8
#define ETH_SET_FLOW_CONTROL_MASK              (1 << ETH_SET_FLOW_CONTROL_BIT)

#define ETH_FLOW_CONTROL_ADVERTISE_BIT         9
#define ETH_FLOW_CONTROL_ADVERTISE_MASK        (1 << ETH_FLOW_CONTROL_ADVERTISE_BIT)

#define ETH_FLOW_CONTROL_ASYMETRIC_BIT         10
#define ETH_FLOW_CONTROL_ASYMETRIC_MASK        (1 << ETH_FLOW_CONTROL_ASYMETRIC_BIT)

#define ETH_ENABLE_FLOW_CONTROL_AUTO_NEG_BIT   11
#define ETH_ENABLE_FLOW_CONTROL_AUTO_NEG_MASK  (1 << ETH_ENABLE_FLOW_CONTROL_AUTO_NEG_BIT)

#define ETH_SET_FULL_DUPLEX_BIT                12
#define ETH_SET_FULL_DUPLEX_MASK               (1 << ETH_SET_FULL_DUPLEX_BIT)

#define ETH_ENABLE_DUPLEX_AUTO_NEG_BIT         13
#define ETH_ENABLE_DUPLEX_AUTO_NEG_MASK        (1 << ETH_ENABLE_DUPLEX_AUTO_NEG_BIT)
/*-------------------------------------------------------------------------------*/

/**** Port Status Sub-Unit Registers ****/
#define ETH_GMAC_STATUS_REG(p)             (ETH_REG_BASE(p) + 0x10)

#define ETH_GMAC_LINK_UP_BIT               0
#define ETH_GMAC_LINK_UP_MASK              (1 << ETH_GMAC_LINK_UP_BIT)

#define ETH_GMAC_SPEED_1000_BIT            1
#define ETH_GMAC_SPEED_1000_MASK           (1 << ETH_GMAC_SPEED_1000_BIT)

#define ETH_GMAC_SPEED_100_BIT             2
#define ETH_GMAC_SPEED_100_MASK            (1 << ETH_GMAC_SPEED_100_BIT)

#define ETH_GMAC_FULL_DUPLEX_BIT           3
#define ETH_GMAC_FULL_DUPLEX_MASK          (1 << ETH_GMAC_FULL_DUPLEX_BIT)

#define ETH_RX_FLOW_CTRL_ENABLE_BIT        4
#define ETH_RX_FLOW_CTRL_ENABLE_MASK       (1 << ETH_RX_FLOW_CTRL_ENABLE_BIT)

#define ETH_TX_FLOW_CTRL_ENABLE_BIT        5
#define ETH_TX_FLOW_CTRL_ENABLE_MASK       (1 << ETH_TX_FLOW_CTRL_ENABLE_BIT)

#define ETH_RX_FLOW_CTRL_ACTIVE_BIT        6
#define ETH_RX_FLOW_CTRL_ACTIVE_MASK       (1 << ETH_RX_FLOW_CTRL_ACTIVE_BIT)

#define ETH_TX_FLOW_CTRL_ACTIVE_BIT        7
#define ETH_TX_FLOW_CTRL_ACTIVE_MASK       (1 << ETH_TX_FLOW_CTRL_ACTIVE_BIT)
/*-------------------------------------------------------------------------------*/

/**** Port Internal Sub-Unit Registers ****/
#define GMAC_PORT_FIFO_CFG_0_REG(p)        (ETH_REG_BASE(p) + 0x18)

#define GMAC_PORT_FIFO_CFG_1_REG(p)        (ETH_REG_BASE(p) + 0x1C)

#define GMAC_RX_FIFO_MAX_TH_OFFS           0

#define GMAC_TX_FIFO_MIN_TH_OFFS           6
#define GMAC_TX_FIFO_MIN_TH_ALL_MASK       (0x7F << GMAC_TX_FIFO_MIN_TH_OFFS)
#define GMAC_TX_FIFO_MIN_TH_MASK(val)      (((val) << GMAC_TX_FIFO_MIN_TH_OFFS) & GMAC_TX_FIFO_MIN_TH_ALL_MASK)
/*-------------------------------------------------------------------------------*/

/**** Port Interrupt Sub-Unit Registers ****/
#define ETH_PORT_ISR_CAUSE_REG(p)		(ETH_REG_BASE(p) + 0x20)

#define ETH_PORT_ISR_SUM_BIT			0
#define ETH_PORT_ISR_SUM_MASK			(1 << ETH_PORT_ISR_SUM_BIT)

#define ETH_PORT_LINK_CHANGE_BIT		1
#define ETH_PORT_LINK_CHANGE_MASK		(1 << ETH_PORT_LINK_CHANGE_BIT)

#define ETH_PORT_ISR_MASK_REG(p)		(ETH_REG_BASE(p) + 0x24)

#define ETH_PORT_ISR_SUM_CAUSE_REG(p)		(ETH_REG_BASE(p) + 0xA0)
#define ETH_PORT_ISR_SUM_MASK_REG(p)		(ETH_REG_BASE(p) + 0xA4)

#define ETH_PORT_ISR_SUM_INTERN_BIT		0x1
#define ETH_PORT_ISR_SUM_INTERN_MASK		(1 << ETH_PORT_ISR_SUM_INTERN_BIT)
/*-------------------------------------------------------------------------------*/

/**** Port MIB Counters Control register ****/
#define ETH_GMAC_MIB_CTRL_REG(p)		(ETH_REG_BASE(p) + 0x44)
/*-------------------------------------------------------------------------------*/

/**** Port MAC Control register #3 ****/
#define ETH_GMAC_CTRL_3_REG(p)			(ETH_REG_BASE(p) + 0x48)
/*-------------------------------------------------------------------------------*/

/**** CCFC Port Speed Timer register ****/
#define ETH_GMAC_SPEED_TIMER_REG(p)		(ETH_REG_BASE(p) + 0x58)
/*-------------------------------------------------------------------------------*/

/**** Port MAC Control register #4 ****/
#define ETH_GMAC_CTRL_4_REG(p)			(ETH_REG_BASE(p) + 0x90)

#define ETH_GMAC_MH_ENABLE_BIT			9
#define ETH_GMAC_MH_ENABLE_MASK			(1 << ETH_GMAC_MH_ENABLE_BIT)
/*-------------------------------------------------------------------------------*/

/****************************************/
/*        LMS Unit Registers       	*/
/****************************************/

/*
 * PHY Address Ports 0 through 5 Register
 */
#define ETH_PHY_ADDR_REG		(LMS_REG_BASE + 0x30)
#define ETH_PHY_ADDR_OFFS(port)		(port * 5)
#define ETH_PHY_ADDR_MASK(port)		(0x1F << ETH_PHY_ADDR_OFFS(port))

/*------------------------------------------------------------------------------
 * PHY Auto-Negotiation Configuration Register0
 */
#define ETH_PHY_AN_CFG0_REG			(LMS_REG_BASE + 0x34)
#define ETH_PHY_AN_CFG0_STOP_AN_SMI0_BIT	7
#define ETH_PHY_AN_CFG0_STOP_AN_SMI0_MASK	(1 << ETH_PHY_AN_CFG0_STOP_AN_SMI0_BIT)
#define ETH_PHY_AN_EN_OFFS(port)		(port)
#define ETH_PHY_AN_EN_MASK(port)		(1 << ETH_PHY_AN_EN_OFFS(port))

/*------------------------------------------------------------------------------
 * Interrupt Summary Cause Register
 */
#define ETH_ISR_SUM_CAUSE_REG		(LMS_REG_BASE + 0x10)
#define ETH_ISR_SUM_LMS_BIT		0
#define ETH_ISR_SUM_LMS_MASK		(1 << ETH_ISR_SUM_LMS_BIT)

#define ETH_ISR_SUM_LMS0_BIT		1
#define ETH_ISR_SUM_LMS0_MASK		(1 << ETH_ISR_SUM_LMS0_BIT)

#define ETH_ISR_SUM_LMS1_BIT		2
#define ETH_ISR_SUM_LMS1_MASK		(1 << ETH_ISR_SUM_LMS1_BIT)

#define ETH_ISR_SUM_LMS2_BIT		3
#define ETH_ISR_SUM_LMS2_MASK		(1 << ETH_ISR_SUM_LMS2_BIT)

#define ETH_ISR_SUM_LMS3_BIT		4
#define ETH_ISR_SUM_LMS3_MASK		(1 << ETH_ISR_SUM_LMS3_BIT)

#define ETH_ISR_SUM_PORTS_BIT		16
#define ETH_ISR_SUM_PORTS_MASK		(1 << ETH_ISR_SUM_PORTS_BIT)

#define ETH_ISR_SUM_PORT0_BIT		17
#define ETH_ISR_SUM_PORT0_MASK		(1 << ETH_ISR_SUM_PORT0_BIT)

#define ETH_ISR_SUM_PORT1_BIT		18
#define ETH_ISR_SUM_PORT1_MASK		(1 << ETH_ISR_SUM_PORT1_BIT)

#define ETH_ISR_SUM_PORT2_BIT		19
#define ETH_ISR_SUM_PORT2_MASK		(1 << ETH_ISR_SUM_PORT2_BIT)

#define ETH_ISR_SUM_PORT_MASK(p)	(1 << (ETH_ISR_SUM_PORT0_BIT + p))

#define ETH_ISR_SUM_MASK_REG		(LMS_REG_BASE + 0x220c)

/*------------------------------------------------------------------------------
 * SMI Management Register
 */
#define ETH_SMI_REG(port)		(LMS_REG_BASE + 0x54)

/****************************************/
/*        MIB counters		       	*/
/****************************************/
#define ETH_MIB_PORT_OFFSET(port)	    ((port >> 1) * 0x400 + (port) * 0x400)
#define ETH_MIB_COUNTERS_BASE(port)    (MIB_COUNTERS_REG_BASE + ETH_MIB_PORT_OFFSET(port))

/* MIB Counters register definitions */
#define ETH_MIB_GOOD_OCTETS_RECEIVED_LOW    0x0
#define ETH_MIB_GOOD_OCTETS_RECEIVED_HIGH   0x4
#define ETH_MIB_BAD_OCTETS_RECEIVED         0x8
/* Reserved                                 0xc */
#define ETH_MIB_UNICAST_FRAMES_RECEIVED     0x10
#define ETH_MIB_CRC_ERRORS_SENT             0x14
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
#define ETH_MIB_UNICAST_FRAMES_SENT         0x40
/* Reserved                                 0x44 */
#define ETH_MIB_MULTICAST_FRAMES_SENT       0x48
#define ETH_MIB_BROADCAST_FRAMES_SENT       0x4c
/* Reserved                                 0x50 */
#define ETH_MIB_FC_SENT                     0x54
#define ETH_MIB_FC_RECEIVED                 0x58
#define ETH_MIB_RX_FIFO_OVERRUN             0x5c
#define ETH_MIB_UNDERSIZE_RECEIVED          0x60
#define ETH_MIB_FRAGMENTS_RECEIVED          0x64
#define ETH_MIB_OVERSIZE_RECEIVED           0x68
#define ETH_MIB_JABBER_RECEIVED             0x6c
#define ETH_MIB_MAC_RECEIVE_ERROR           0x70
#define ETH_MIB_BAD_CRC_EVENT               0x74
#define ETH_MIB_COLLISION                   0x78
#define ETH_MIB_LATE_COLLISION              0x7c
#endif /* mvEthGmacRegs */
