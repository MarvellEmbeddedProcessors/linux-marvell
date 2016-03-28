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
#include "gop/pcs/mv_xpcs_regs.h"


/* print value of unit registers */
void mv_xpcs_gl_regs_dump(void)
{
	pr_info("\nXPCS Global registers]\n");
	mv_gop_reg_print("GLOBAL_CFG_0", MV_XPCS_GLOBAL_CFG_0_REG);
	mv_gop_reg_print("GLOBAL_CFG_1", MV_XPCS_GLOBAL_CFG_1_REG);
	mv_gop_reg_print("GLOBAL_FIFO_THR_CFG", MV_XPCS_GLOBAL_FIFO_THR_CFG_REG);
	mv_gop_reg_print("GLOBAL_MAX_IDLE_CNTR", MV_XPCS_GLOBAL_MAX_IDLE_CNTR_REG);
	mv_gop_reg_print("GLOBAL_STATUS", MV_XPCS_GLOBAL_STATUS_REG);
	mv_gop_reg_print("GLOBAL_DESKEW_ERR_CNTR", MV_XPCS_GLOBAL_DESKEW_ERR_CNTR_REG);
	mv_gop_reg_print("TX_PCKTS_CNTR_LSB", MV_XPCS_TX_PCKTS_CNTR_LSB_REG);
	mv_gop_reg_print("TX_PCKTS_CNTR_MSB", MV_XPCS_TX_PCKTS_CNTR_MSB_REG);

}

/* print value of unit registers */
void mv_xpcs_lane_regs_dump(int lane)
{
	pr_info("\nXPCS Lane #%d registers]\n", lane);
	mv_gop_reg_print("LANE_CFG_0", MV_XPCS_LANE_CFG_0_REG(lane));
	mv_gop_reg_print("LANE_CFG_1", MV_XPCS_LANE_CFG_1_REG(lane));
	mv_gop_reg_print("LANE_STATUS", MV_XPCS_LANE_STATUS_REG(lane));
	mv_gop_reg_print("SYMBOL_ERR_CNTR", MV_XPCS_SYMBOL_ERR_CNTR_REG(lane));
	mv_gop_reg_print("DISPARITY_ERR_CNTR", MV_XPCS_DISPARITY_ERR_CNTR_REG(lane));
	mv_gop_reg_print("PRBS_ERR_CNTR", MV_XPCS_PRBS_ERR_CNTR_REG(lane));
	mv_gop_reg_print("RX_PCKTS_CNTR_LSB", MV_XPCS_RX_PCKTS_CNTR_LSB_REG(lane));
	mv_gop_reg_print("RX_PCKTS_CNTR_MSB", MV_XPCS_RX_PCKTS_CNTR_MSB_REG(lane));
	mv_gop_reg_print("RX_BAD_PCKTS_CNTR_LSB", MV_XPCS_RX_BAD_PCKTS_CNTR_LSB_REG(lane));
	mv_gop_reg_print("RX_BAD_PCKTS_CNTR_MSB", MV_XPCS_RX_BAD_PCKTS_CNTR_MSB_REG(lane));
	mv_gop_reg_print("CYCLIC_DATA_0", MV_XPCS_CYCLIC_DATA_0_REG(lane));
	mv_gop_reg_print("CYCLIC_DATA_1", MV_XPCS_CYCLIC_DATA_1_REG(lane));
	mv_gop_reg_print("CYCLIC_DATA_2", MV_XPCS_CYCLIC_DATA_2_REG(lane));
	mv_gop_reg_print("CYCLIC_DATA_3", MV_XPCS_CYCLIC_DATA_3_REG(lane));
}

/* Set PCS to reset or exit from reset */
int mv_xpcs_reset(enum mv_reset reset)
{
	u32 reg_addr;
	u32 val;

	reg_addr = MV_XPCS_GLOBAL_CFG_0_REG;

	/* read - modify - write */
	val = mv_gop_reg_read(reg_addr);
	if (reset == RESET)
		val &= ~MV_XPCS_GLOBAL_CFG_0_PCSRESET_MASK;
	else
		val |= MV_XPCS_GLOBAL_CFG_0_PCSRESET_MASK;
	mv_gop_reg_write(reg_addr, val);

	return 0;
}

/* Set the internal mux's to the required PCS in the PI */
int mv_xpcs_mode(int num_of_lanes)
{
	u32 reg_addr;
	u32 val;
	int lane;

	switch (num_of_lanes) {
	case 1:
		lane = 0;
	break;
	case 2:
		lane = 1;
	break;
	case 4:
		lane = 2;
	break;
	default:
		return -1;
	}

	/* configure XG MAC mode */
	reg_addr = MV_XPCS_GLOBAL_CFG_0_REG;
	val = mv_gop_reg_read(reg_addr);
	val &= ~MV_XPCS_GLOBAL_CFG_0_PCSMODE_MASK;
	MV_U32_SET_FIELD(val, MV_XPCS_GLOBAL_CFG_0_PCSMODE_MASK, 0);
	MV_U32_SET_FIELD(val, MV_XPCS_GLOBAL_CFG_0_LANEACTIVE_MASK, (2 * lane) << MV_XPCS_GLOBAL_CFG_0_LANEACTIVE_OFFS);
	mv_gop_reg_write(reg_addr, val);

	return 0;
}

