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

#ifndef __mv_gmac_regs_h__
#define __mv_gmac_regs_h__
/*
#ifdef CONFIG_ARCH_MVEBU
#include "mvNetConfig.h"
#else
#include "mvSysEthConfig.h"
#endif
*/

#define PP3_GMAC_MAX			2
#define GMAC_REG_BASE(_port_)		(0x4000 + (0x1000 * (_port_)))
#define LMS_REG_BASE			(0x0)
#define GMAC_MIB_COUNTERS_REG_BASE	(0x1000)

#define MNG_EXTENDED_GLOBAL_CTRL_REG	(0x3000 + 0x5c)

/****************************************/
/*        MAC Unit Registers            */
/****************************************/

/**** Tri-Speed Ports MAC and CPU Port MAC Configuration Sub-Unit Registers ****/
#define GMAC_CTRL_0_REG				(0x0000)

#define GMAC_PORT_EN_BIT			0
#define GMAC_PORT_EN_MASK			(1 << GMAC_PORT_EN_BIT)

#define GMAC_PORT_TYPE_BIT			1
#define GMAC_PORT_TYPE_MASK			(1 << GMAC_PORT_TYPE_BIT)
#define GMAC_PORT_TYPE_SGMII			(0 << GMAC_PORT_TYPE_BIT)
#define GMAC_PORT_TYPE_1000X			(1 << GMAC_PORT_TYPE_BIT)

#define GMAC_MAX_RX_SIZE_OFFS			2
#define GMAC_MAX_RX_SIZE_MASK			(0x1FFF << GMAC_MAX_RX_SIZE_OFFS)

#define GMAC_GMAC_MIB_CNTR_EN_BIT		15
#define GMAC_GMAC_MIB_CNTR_EN_MASK		(1 << GMAC_GMAC_MIB_CNTR_EN_BIT)
/*-------------------------------------------------------------------------------*/

#define GMAC_CTRL_1_REG				(0x0004)

#define GMAC_PERIODIC_XON_EN_BIT		1
#define GMAC_PERIODIC_XON_EN_MASK		(0x1 << GMAC_PERIODIC_XON_EN_BIT)

#define GMAC_GMII_LB_EN_BIT			5
#define GMAC_GMII_LB_EN_MASK			(1 << GMAC_GMII_LB_EN_BIT)

#define GMAC_PCS_LB_EN_BIT			6
#define GMAC_PCS_LB_EN_MASK			(1 << GMAC_PCS_LB_EN_BIT)

#define GMAC_SA_LOW_OFFS			7
#define GMAC_SA_LOW_MASK			(0xFF << GMAC_SA_LOW_OFFS)
/*-------------------------------------------------------------------------------*/

#define GMAC_CTRL_2_REG				(0x0008)

#define GMAC_INBAND_AN_BIT			0
#define GMAC_INBAND_AN_MASK			(1 << GMAC_INBAND_AN_BIT)

#define GMAC_PCS_ENABLE_BIT			3
#define GMAC_PCS_ENABLE_MASK			(1 << GMAC_PCS_ENABLE_BIT)

#define GMAC_PORT_RGMII_BIT			4
#define GMAC_PORT_RGMII_MASK			(1 << GMAC_PORT_RGMII_BIT)

#define GMAC_PORT_RESET_BIT			6
#define GMAC_PORT_RESET_MASK			(1 << GMAC_PORT_RESET_BIT)
/*-------------------------------------------------------------------------------*/

/**** Port Auto-Negotiation Configuration Sub-Unit Registers ****/
#define GMAC_AN_CTRL_REG			(0x000C)

#define GMAC_FORCE_LINK_FAIL_BIT		0
#define GMAC_FORCE_LINK_FAIL_MASK		(1 << GMAC_FORCE_LINK_FAIL_BIT)

#define GMAC_FORCE_LINK_PASS_BIT		1
#define GMAC_FORCE_LINK_PASS_MASK		(1 << GMAC_FORCE_LINK_PASS_BIT)

#define GMAC_SET_MII_SPEED_100_BIT		5
#define GMAC_SET_MII_SPEED_100_MASK		(1 << GMAC_SET_MII_SPEED_100_BIT)

#define GMAC_SET_GMII_SPEED_1000_BIT		6
#define GMAC_SET_GMII_SPEED_1000_MASK		(1 << GMAC_SET_GMII_SPEED_1000_BIT)

#define GMAC_ENABLE_SPEED_AUTO_NEG_BIT		7
#define GMAC_ENABLE_SPEED_AUTO_NEG_MASK		(1 << GMAC_ENABLE_SPEED_AUTO_NEG_BIT)

#define GMAC_ENABLE_FLOW_CTRL_AUTO_NEG_BIT	11
#define GMAC_ENABLE_FLOW_CTRL_AUTO_NEG_MASK	(1 << GMAC_ENABLE_FLOW_CTRL_AUTO_NEG_BIT)

/* TODO: I keep this bit even though it's not listed in Cider */
#define GMAC_SET_FLOW_CONTROL_BIT		8
#define GMAC_SET_FLOW_CONTROL_MASK		(1 << GMAC_SET_FLOW_CONTROL_BIT)

#define GMAC_FLOW_CONTROL_ADVERTISE_BIT		9
#define GMAC_FLOW_CONTROL_ADVERTISE_MASK	(1 << GMAC_FLOW_CONTROL_ADVERTISE_BIT)

#define GMAC_FLOW_CONTROL_ASYMETRIC_BIT		10
#define GMAC_FLOW_CONTROL_ASYMETRIC_MASK	(1 << GMAC_FLOW_CONTROL_ASYMETRIC_BIT)

#define GMAC_ENABLE_FLOW_CONTROL_AUTO_NEG_BIT	11
#define GMAC_ENABLE_FLOW_CONTROL_AUTO_NEG_MASK	(1 << GMAC_ENABLE_FLOW_CONTROL_AUTO_NEG_BIT)

#define GMAC_SET_FULL_DUPLEX_BIT		12
#define GMAC_SET_FULL_DUPLEX_MASK		(1 << GMAC_SET_FULL_DUPLEX_BIT)

#define GMAC_ENABLE_DUPLEX_AUTO_NEG_BIT		13
#define GMAC_ENABLE_DUPLEX_AUTO_NEG_MASK	(1 << GMAC_ENABLE_DUPLEX_AUTO_NEG_BIT)
/*-------------------------------------------------------------------------------*/

/**** Port Status Sub-Unit Registers ****/
#define GMAC_STATUS_REG				(0x0010)

#define GMAC_LINK_UP_BIT			0
#define GMAC_LINK_UP_MASK			(1 << GMAC_LINK_UP_BIT)

#define GMAC_SPEED_1000_BIT			1
#define GMAC_SPEED_1000_MASK			(1 << GMAC_SPEED_1000_BIT)

#define GMAC_SPEED_100_BIT			2
#define GMAC_SPEED_100_MASK			(1 << GMAC_SPEED_100_BIT)

#define GMAC_FULL_DUPLEX_BIT			3
#define GMAC_FULL_DUPLEX_MASK			(1 << GMAC_FULL_DUPLEX_BIT)

#define GMAC_RX_FLOW_CTRL_ENABLE_BIT		4
#define GMAC_RX_FLOW_CTRL_ENABLE_MASK		(1 << GMAC_RX_FLOW_CTRL_ENABLE_BIT)

#define GMAC_TX_FLOW_CTRL_ENABLE_BIT		5
#define GMAC_TX_FLOW_CTRL_ENABLE_MASK		(1 << GMAC_TX_FLOW_CTRL_ENABLE_BIT)

#define GMAC_RX_FLOW_CTRL_ACTIVE_BIT		6
#define GMAC_RX_FLOW_CTRL_ACTIVE_MASK		(1 << GMAC_RX_FLOW_CTRL_ACTIVE_BIT)

#define GMAC_TX_FLOW_CTRL_ACTIVE_BIT		7
#define GMAC_TX_FLOW_CTRL_ACTIVE_MASK		(1 << GMAC_TX_FLOW_CTRL_ACTIVE_BIT)
/*-------------------------------------------------------------------------------*/

/**** Port Internal Sub-Unit Registers ****/
#define GMAC_PORT_FIFO_CFG_0_REG		(0x0018)

#define GMAC_PORT_FIFO_CFG_1_REG		(0x001C)

#define GMAC_RX_FIFO_MAX_TH_OFFS		0

#define GMAC_TX_FIFO_MIN_TH_OFFS		6
#define GMAC_TX_FIFO_MIN_TH_ALL_MASK		(0x7F << GMAC_TX_FIFO_MIN_TH_OFFS)
#define GMAC_TX_FIFO_MIN_TH_MASK(val)		(((val) << GMAC_TX_FIFO_MIN_TH_OFFS) & GMAC_TX_FIFO_MIN_TH_ALL_MASK)
/*-------------------------------------------------------------------------------*/

/**** Port Interrupt Sub-Unit Registers ****/
#define GMAC_PORT_ISR_CAUSE_REG			(0x0020)

#define GMAC_PORT_ISR_SUM_BIT			0
#define GMAC_PORT_ISR_SUM_MASK			(1 << GMAC_PORT_ISR_SUM_BIT)

#define GMAC_PORT_LINK_CHANGE_BIT		1
#define GMAC_PORT_LINK_CHANGE_MASK		(1 << GMAC_PORT_LINK_CHANGE_BIT)

#define GMAC_PORT_ISR_MASK_REG			(0x0024)

#define GMAC_PORT_ISR_SUM_CAUSE_REG		(0x00A0)
#define GMAC_PORT_ISR_SUM_MASK_REG		(0x00A4)

#define GMAC_PORT_ISR_SUM_INTERN_BIT		0x1
#define GMAC_PORT_ISR_SUM_INTERN_MASK		(1 << GMAC_PORT_ISR_SUM_INTERN_BIT)

/*-------------------------------------------------------------------------------*/

/****************************************/
/*	LMS Unit Registers		*/
/****************************************/

/*
 * PHY Address Ports 0 through 5 Register
 */
#define PHY_ADDR_REG				(LMS_REG_BASE + 0x30)
#define PHY_ADDR_OFFS(port)			(port * 5)
#define PHY_ADDR_MASK(port)			(0x1F << PHY_ADDR_OFFS(port))

/*------------------------------------------------------------------------------
 * PHY Auto-Negotiation Configuration Register0
 */
#define PHY_AN_CFG0_REG				(LMS_REG_BASE + 0x34)
#define PHY_AN_CFG0_STOP_AN_SMI0_BIT		7
#define PHY_AN_CFG0_STOP_AN_SMI0_MASK		(1 << PHY_AN_CFG0_STOP_AN_SMI0_BIT)
#define PHY_AN_EN_OFFS(port)			(port)
#define PHY_AN_EN_MASK(port)			(1 << PHY_AN_EN_OFFS(port))

/*------------------------------------------------------------------------------
 * Interrupt Summary Cause Register
 */
#define ISR_SUM_CAUSE_REG			(LMS_REG_BASE + 0x10)
#define ISR_SUM_LMS_BIT				0
#define ISR_SUM_LMS_MASK			(1 << ISR_SUM_LMS_BIT)

#define ISR_SUM_LMS0_BIT			1
#define ISR_SUM_LMS0_MASK			(1 << ISR_SUM_LMS0_BIT)

#define ISR_SUM_LMS1_BIT			2
#define ISR_SUM_LMS1_MASK			(1 << ISR_SUM_LMS1_BIT)

#define ISR_SUM_LMS2_BIT			3
#define ISR_SUM_LMS2_MASK			(1 << ISR_SUM_LMS2_BIT)

#define ISR_SUM_LMS3_BIT			4
#define ISR_SUM_LMS3_MASK			(1 << ISR_SUM_LMS3_BIT)

#define ISR_SUM_PORTS_BIT			16
#define ISR_SUM_PORTS_MASK			(1 << ISR_SUM_PORTS_BIT)

#define ISR_SUM_PORT0_BIT			17
#define ISR_SUM_PORT0_MASK			(1 << ISR_SUM_PORT0_BIT)

#define ISR_SUM_PORT1_BIT			18
#define ISR_SUM_PORT1_MASK			(1 << ISR_SUM_PORT1_BIT)

#define ISR_SUM_PORT2_BIT			19
#define ISR_SUM_PORT2_MASK			(1 << ISR_SUM_PORT2_BIT)

#define ISR_SUM_MASK_REG			(LMS_REG_BASE + 0x220c)

/*------------------------------------------------------------------------------
 * SMI Management Register
 */
#define SMI_REG(port)				(LMS_REG_BASE + 0x54)

/****************************************/
/*	GMAC_MIB counters		*/
/****************************************/

#define GMAC_MIB_PORT_OFFSET(port)		((port) * 0x400)
#define GMAC_MIB_COUNTERS_BASE(port)		(GMAC_MIB_COUNTERS_REG_BASE + GMAC_MIB_PORT_OFFSET(port))

/* GMAC_MIB Counters register definitions */
#define GMAC_MIB_GOOD_OCTETS_RECEIVED_LOW	0x0
#define GMAC_MIB_GOOD_OCTETS_RECEIVED_HIGH	0x4
#define GMAC_MIB_BAD_OCTETS_RECEIVED		0x8
/* Reserved				0xc */
#define GMAC_MIB_UNICAST_FRAMES_RECEIVED	0x10
#define GMAC_MIB_CRC_ERRORS_SENT		0x14
#define GMAC_MIB_BROADCAST_FRAMES_RECEIVED	0x18
#define GMAC_MIB_MULTICAST_FRAMES_RECEIVED	0x1c
#define GMAC_MIB_FRAMES_64_OCTETS		0x20
#define GMAC_MIB_FRAMES_65_TO_127_OCTETS	0x24
#define GMAC_MIB_FRAMES_128_TO_255_OCTETS	0x28
#define GMAC_MIB_FRAMES_256_TO_511_OCTETS	0x2c
#define GMAC_MIB_FRAMES_512_TO_1023_OCTETS	0x30
#define GMAC_MIB_FRAMES_1024_TO_MAX_OCTETS	0x34
#define GMAC_MIB_GOOD_OCTETS_SENT_LOW		0x38
#define GMAC_MIB_GOOD_OCTETS_SENT_HIGH		0x3c
#define GMAC_MIB_UNICAST_FRAMES_SENT		0x40
/* Reserved					0x44 */
#define GMAC_MIB_MULTICAST_FRAMES_SENT		0x48
#define GMAC_MIB_BROADCAST_FRAMES_SENT		0x4c
/* Reserved					0x50 */
#define GMAC_MIB_FC_SENT			0x54
#define GMAC_MIB_FC_RECEIVED			0x58
#define GMAC_MIB_RX_FIFO_OVERRUN		0x5c
#define GMAC_MIB_UNDERSIZE_RECEIVED		0x60
#define GMAC_MIB_FRAGMENTS_RECEIVED		0x64
#define GMAC_MIB_OVERSIZE_RECEIVED		0x68
#define GMAC_MIB_JABBER_RECEIVED		0x6c
#define GMAC_MIB_MAC_RECEIVE_ERROR		0x70
#define GMAC_MIB_BAD_CRC_EVENT			0x74
#define GMAC_MIB_COLLISION			0x78
#define GMAC_MIB_LATE_COLLISION			0x7c
#endif /* mv_gmac_regs */
