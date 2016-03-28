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
#include "common/mv_sw_if.h"
#include "gop/mv_gop_if.h"
#include "gop/serdes/mv_serdes_if.h"
#include "gop/serdes/mv_serdes_regs.h"


/* print value of unit registers */
void mv_serdes_lane_regs_dump(int lane)
{
	pr_info("\nSerdes Lane #%d registers]\n", lane);
	mv_gop_reg_print("MV_PP3_SERDES_CFG_0_REG", MV_PP3_SERDES_CFG_0_REG(lane));
	mv_gop_reg_print("MV_PP3_SERDES_CFG_1_REG", MV_PP3_SERDES_CFG_1_REG(lane));
	mv_gop_reg_print("MV_PP3_SERDES_CFG_2_REG", MV_PP3_SERDES_CFG_2_REG(lane));
	mv_gop_reg_print("MV_PP3_SERDES_CFG_3_REG", MV_PP3_SERDES_CFG_3_REG(lane));
	mv_gop_reg_print("MV_PP3_SERDES_MISC_REG", MV_PP3_SERDES_MISC_REG(lane));
}

void mv_serdes_init(int lane, enum sd_media_mode mode)
{
	u32 reg_val;

	/* Media Interface Mode */
	reg_val = mv_gop_reg_read(MV_PP3_SERDES_CFG_0_REG(lane));
	if (mode == MV_RXAUI)
		reg_val |= MV_PP3_SERDES_CFG_0_MEDIA_MODE_MASK;
	else
		reg_val &= ~MV_PP3_SERDES_CFG_0_MEDIA_MODE_MASK;

	/* Pull-Up PLL to StandAlone mode */
	reg_val |= MV_PP3_SERDES_CFG_0_PU_PLL_MASK;
	/* powers up the SD Rx/Tx PLL */
	reg_val |= MV_PP3_SERDES_CFG_0_RX_PLL_MASK;
	reg_val |= MV_PP3_SERDES_CFG_0_TX_PLL_MASK;
	mv_gop_reg_write(MV_PP3_SERDES_CFG_0_REG(lane), reg_val);

	mv_serdes_reset(lane, false, false, false);

	reg_val = 0x17f;
	mv_gop_reg_write(MV_PP3_SERDES_MISC_REG(lane), reg_val);
}

void mv_serdes_reset(int lane, bool analog_reset, bool core_reset, bool digital_reset)
{
	u32 reg_val;

	reg_val = mv_gop_reg_read(MV_PP3_SERDES_CFG_1_REG(lane));
	if (analog_reset)
		reg_val &= ~MV_PP3_SERDES_CFG_1_ANALOG_RESET_MASK;
	else
		reg_val |= MV_PP3_SERDES_CFG_1_ANALOG_RESET_MASK;

	if (core_reset)
		reg_val &= ~MV_PP3_SERDES_CFG_1_CORE_RESET_MASK;
	else
		reg_val |= MV_PP3_SERDES_CFG_1_CORE_RESET_MASK;

	if (digital_reset)
		reg_val &= ~MV_PP3_SERDES_CFG_1_DIGITAL_RESET_MASK;
	else
		reg_val |= MV_PP3_SERDES_CFG_1_DIGITAL_RESET_MASK;

	mv_gop_reg_write(MV_PP3_SERDES_CFG_1_REG(lane), reg_val);
}
