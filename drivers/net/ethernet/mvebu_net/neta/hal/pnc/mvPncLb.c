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

#include "mvOs.h"
#include "mvCommon.h"

#include "gbe/mvNetaRegs.h"

#include "mvPnc.h"
#include "mvTcam.h"

#ifdef MV_ETH_PNC_LB

void    mvPncLbDump(void)
{
	MV_U32	regVal;
	int i, j, rxq;

	MV_REG_WRITE(MV_PNC_LB_TBL_ACCESS_REG, 0);
	mvOsPrintf("Hash:    rxq    rxq    rxq    rxq\n");
	for (i = 0; i <= MV_PNC_LB_TBL_ADDR_MASK; i++) {
		/* Each read returns 4 hash entries */
		regVal = MV_REG_READ(MV_PNC_LB_TBL_ACCESS_REG);
		/* Extract data */
		regVal = (regVal & MV_PNC_LB_TBL_DATA_MASK) >> MV_PNC_LB_TBL_DATA_OFFS;
		mvOsPrintf("%4d:    ", (i * 4));
		for (j = 0; j < 4; j++) {
			rxq = regVal & 7;
			mvOsPrintf("%3d   ", rxq);
			regVal = regVal >> 3;
		}
		mvOsPrintf("\n");
	}
}

int    mvPncLbRxqSet(int hash, int rxq)
{
	MV_U32 regVal, entry, index;

	entry = (hash / 4) & MV_PNC_LB_TBL_ADDR_MASK;
	index = (hash & 3);

	MV_REG_WRITE(MV_PNC_LB_TBL_ACCESS_REG, entry);
	regVal = MV_REG_READ(MV_PNC_LB_TBL_ACCESS_REG);

	regVal &= ~MV_PNC_LB_TBL_ADDR_MASK;
	regVal |= entry;
	regVal &= ~((7 << (index * 3)) << MV_PNC_LB_TBL_DATA_OFFS);
	regVal |= ((rxq << (index * 3)) << MV_PNC_LB_TBL_DATA_OFFS);
	regVal |= MV_PNC_LB_TBL_WRITE_TRIG_MASK;
	MV_REG_WRITE(MV_PNC_LB_TBL_ACCESS_REG, regVal);

	return 0;
}

int		mvPncLbModeIp4(int mode)
{
	int lb;
	struct tcam_entry te;

	switch (mode) {
	case 0:
		lb = LB_DISABLE_VALUE;
		break;
	case 1:
		lb = LB_2_TUPLE_VALUE;
		break;
	case 2:
	default:
		mvOsPrintf("%s: %d - unexpected mode value\n", __func__, mode);
		return 1;
	}
	tcam_hw_read(&te, TE_IP4_EOF);
	sram_sw_set_load_balance(&te, lb);
	tcam_hw_write(&te, TE_IP4_EOF);

	return 0;
}

int	mvPncLbModeIp6(int mode)
{
	int lb;
	struct tcam_entry te;

	switch (mode) {
	case 0:
		lb = LB_DISABLE_VALUE;
		break;
	case 1:
		lb = LB_2_TUPLE_VALUE;
		break;
	case 2:
	default:
		mvOsPrintf("%s: %d - unexpected mode value\n", __func__, mode);
		return 1;
	}
	tcam_hw_read(&te, TE_IP6_EOF);
	sram_sw_set_load_balance(&te, lb);
	tcam_hw_write(&te, TE_IP6_EOF);

	return 0;
}

int	mvPncLbModeL4(int mode)
{
	int lb;
	struct tcam_entry te;

	switch (mode) {
	case 0:
		lb = LB_DISABLE_VALUE;
		break;
	case 1:
		lb = LB_2_TUPLE_VALUE;
		break;
	case 2:
		lb = LB_4_TUPLE_VALUE;
		break;
	default:
		mvOsPrintf("%s: %d - unexpected mode value\n", __func__, mode);
		return 1;
	}

#ifdef CONFIG_MV_ETH_PNC_L3_FLOW
	mvOsPrintf("%s: Not supported\n", __func__);
	return 1;
#else
	tcam_hw_read(&te, TE_L4_EOF);
	sram_sw_set_load_balance(&te, lb);
	tcam_hw_write(&te, TE_L4_EOF);
	return 0;
#endif /* CONFIG_MV_ETH_PNC_L3_FLOW */
}
#endif /* MV_ETH_PNC_LB */
