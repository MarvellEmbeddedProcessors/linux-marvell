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

#ifndef __mv_serdes_regs_h__
#define __mv_serdes_regs_h__

/* includes */

/* unit offset */
#define MV_PP3_SERDES_UNIT_OFFSET		0x04000000

#define MV_PP3_SERDES_CFG_0_REG(lane)		(MV_PP3_SERDES_UNIT_OFFSET + 0x00 + (lane * 0x1000))

#define MV_PP3_SERDES_CFG_0_PU_PLL_OFFS		1
#define MV_PP3_SERDES_CFG_0_PU_PLL_MASK		(0x00000001 << MV_PP3_SERDES_CFG_0_PU_PLL_OFFS)
#define MV_PP3_SERDES_CFG_0_RX_PLL_OFFS		11
#define MV_PP3_SERDES_CFG_0_RX_PLL_MASK		(0x00000001 << MV_PP3_SERDES_CFG_0_RX_PLL_OFFS)
#define MV_PP3_SERDES_CFG_0_TX_PLL_OFFS		12
#define MV_PP3_SERDES_CFG_0_TX_PLL_MASK		(0x00000001 << MV_PP3_SERDES_CFG_0_TX_PLL_OFFS)
#define MV_PP3_SERDES_CFG_0_MEDIA_MODE_OFFS	15
#define MV_PP3_SERDES_CFG_0_MEDIA_MODE_MASK	(0x00000001 << MV_PP3_SERDES_CFG_0_MEDIA_MODE_OFFS)

#define MV_PP3_SERDES_CFG_1_REG(lane)		(MV_PP3_SERDES_UNIT_OFFSET + 0x04 + (lane * 0x1000))
#define MV_PP3_SERDES_CFG_1_ANALOG_RESET_OFFS		3
#define MV_PP3_SERDES_CFG_1_ANALOG_RESET_MASK    \
		(0x00000001 << MV_PP3_SERDES_CFG_1_ANALOG_RESET_OFFS)

#define MV_PP3_SERDES_CFG_1_CORE_RESET_OFFS		5
#define MV_PP3_SERDES_CFG_1_CORE_RESET_MASK    \
		(0x00000001 << MV_PP3_SERDES_CFG_1_CORE_RESET_OFFS)

#define MV_PP3_SERDES_CFG_1_DIGITAL_RESET_OFFS		6
#define MV_PP3_SERDES_CFG_1_DIGITAL_RESET_MASK    \
		(0x00000001 << MV_PP3_SERDES_CFG_1_DIGITAL_RESET_OFFS)

#define MV_PP3_SERDES_CFG_1_TX_SYNC_EN_OFFS		7
#define MV_PP3_SERDES_CFG_1_TX_SYNC_EN_MASK    \
		(0x00000001 << MV_PP3_SERDES_CFG_1_TX_SYNC_EN_OFFS)

#define MV_PP3_SERDES_CFG_2_REG(lane)		(MV_PP3_SERDES_UNIT_OFFSET + 0x08 + (lane * 0x1000))
#define MV_PP3_SERDES_CFG_3_REG(lane)		(MV_PP3_SERDES_UNIT_OFFSET + 0x0c + (lane * 0x1000))
#define MV_PP3_SERDES_MISC_REG(lane)		(MV_PP3_SERDES_UNIT_OFFSET + 0x14 + (lane * 0x1000))

#endif /* __mv_serdes_regs_h__ */
