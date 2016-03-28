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
#include "gop/mac/mv_gmac_regs.h"

/*******************************************************************************
* mv_gpcs_mode_cfg
*
* DESCRIPTION:
	Configure port to working with Gig PCS or don't.
*
* INPUTS:
*       pcs_num   - physical PCS number
*       en        - true to enable PCS
*
* OUTPUTS:
*       None.
*
* RETURNS:
*       0  - on success
*       1  - on error
*
*******************************************************************************/
int mv_gpcs_mode_cfg(int pcs_num, bool en)
{
	u32 val;

	val = mv_gop_reg_read(MV_GMAC_PORT_CTRL2_REG(pcs_num));

	if (en)
		val |= MV_GMAC_PORT_CTRL2_PCS_EN_MASK;
	else
		val &= ~MV_GMAC_PORT_CTRL2_PCS_EN_MASK;

	/* enable / disable PCS on this port */
	mv_gop_reg_write(MV_GMAC_PORT_CTRL2_REG(pcs_num), val);

	return 0;
}

/*******************************************************************************
* mv_gpcs_reset
*
* DESCRIPTION:
*       Set the selected PCS number to reset or exit from reset.
*
* INPUTS:
*       pcs_num    - physical PCS number
*       action    - reset / unreset
*
* OUTPUTS:
*       None.
*
* RETURNS:
*       0  - on success
*       1  - on error
*
*******************************************************************************/
int  mv_gpcs_reset(int pcs_num, enum mv_reset act)
{
	u32 reg_data;

	reg_data = mv_gop_reg_read(MV_GMAC_PORT_CTRL2_REG(pcs_num));
	if (act == RESET)
		MV_U32_SET_FIELD(reg_data, MV_GMAC_PORT_CTRL2_SGMII_MODE_MASK, 0);
	else
		MV_U32_SET_FIELD(reg_data, MV_GMAC_PORT_CTRL2_SGMII_MODE_MASK,
			1 << MV_GMAC_PORT_CTRL2_SGMII_MODE_OFFS);

	mv_gop_reg_write(MV_GMAC_PORT_CTRL2_REG(pcs_num), reg_data);
	return 0;
}
