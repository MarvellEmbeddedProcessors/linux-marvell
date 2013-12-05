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

#include "mvPp2ClsMcHw.h"

MC_SHADOW_ENTRY  mvMcShadowTbl[MV_PP2_MC_TBL_SIZE];

/******************************************************************************
 * Common utilities
 ******************************************************************************/
/*
static void mvPp2McShadowSet(int index, int next)
{
	mvMcShadowTbl[index].valid = 1;
	mvMcShadowTbl[index].next = next;
}
*/
/*-------------------------------------------------------------------------------*/
/*
static void mvPp2McShadowClear(int index)
{
	mvMcShadowTbl[index].valid = 0;
}
*/
/*-------------------------------------------------------------------------------*/
/*
static void mvPp2McShadowClearAll(void)
{
	int index;

	for (index = 0; index < MV_PP2_MC_TBL_SIZE; index++)
		mvMcShadowTbl[index].valid = 0;
}
*/
/*-------------------------------------------------------------------------------*/
/*
int mvPp2McFirstFreeGet(void)
{
	int index;

	Go through the all entires from first to last
	for (index = 0; index < MV_PP2_MC_TBL_SIZE; index++) {
		if (!mvMcShadowTbl[index].valid)
			break;
	}
	return index;
}
*/
/*-------------------------------------------------------------------------------*/
int	mvPp2McHwWrite(MV_PP2_MC_ENTRY *mc, int index)
{
	PTR_VALIDATE(mc);

	POS_RANGE_VALIDATE(index, MV_PP2_MC_TBL_SIZE - 1);

	mc->index = index;

	/* write index */
	mvPp2WrReg(MV_PP2_MC_INDEX_REG, mc->index);

	/* write data */
	mvPp2WrReg(MV_PP2_MC_DATA1_REG, mc->sram.regs.data1);
	mvPp2WrReg(MV_PP2_MC_DATA2_REG, mc->sram.regs.data2);
	mvPp2WrReg(MV_PP2_MC_DATA3_REG, mc->sram.regs.data3);

	/*
	update shadow
	next = ((mc->sram.regs.data3 & MV_PP2_MC_DATA3_NEXT_MASK) >> MV_PP2_MC_DATA3_NEXT);
	mvPp2McShadowSet(mc->index, next);
	*/

	return MV_OK;
}

/*-------------------------------------------------------------------------------*/
int	mvPp2McHwRead(MV_PP2_MC_ENTRY *mc, int index)
{
	PTR_VALIDATE(mc);

	POS_RANGE_VALIDATE(index, MV_PP2_MC_TBL_SIZE - 1);

	mc->index = index;

	/* write index */
	mvPp2WrReg(MV_PP2_MC_INDEX_REG, mc->index);

	/* read data */
	mc->sram.regs.data1 = mvPp2RdReg(MV_PP2_MC_DATA1_REG);
	mc->sram.regs.data2 = mvPp2RdReg(MV_PP2_MC_DATA2_REG);
	mc->sram.regs.data3 = mvPp2RdReg(MV_PP2_MC_DATA3_REG);

	return MV_OK;
}

/*-------------------------------------------------------------------------------*/
int	mvPp2McSwDump(MV_PP2_MC_ENTRY *mc)
{
	mvOsPrintf("INDEX:0x%2x\t", mc->index);

	mvOsPrintf("IPTR:0x%1x\t",
			(mc->sram.regs.data1 >> MV_PP2_MC_DATA1_IPTR) & ACT_HWF_ATTR_IPTR_MAX);

	mvOsPrintf("DPTR:0x%1x\t",
			(mc->sram.regs.data1 >> MV_PP2_MC_DATA1_DPTR) & ACT_HWF_ATTR_DPTR_MAX);

	if (mc->sram.regs.data2 &  MV_PP2_MC_DATA2_GEM_ID_EN)
		mvOsPrintf("GPID:0x%3x\t", (mc->sram.regs.data2 >> MV_PP2_MC_DATA2_GEM_ID) & ACT_QOS_ATTR_GEM_ID_MAX);
	else
		mvOsPrintf("GPID:INV\t");

	if (mc->sram.regs.data2 &  MV_PP2_MC_DATA2_DSCP_EN)
		mvOsPrintf("DSCP:0x%1x\t", (mc->sram.regs.data2 >> MV_PP2_MC_DATA2_DSCP) & ACT_QOS_ATTR_DSCP_MAX);
	else
		mvOsPrintf("DSCP:INV\t");

	if (mc->sram.regs.data2 &  MV_PP2_MC_DATA2_PRI_EN)
		mvOsPrintf("PRI:0x%1x \t", (mc->sram.regs.data2 >> MV_PP2_MC_DATA2_PRI) & ACT_QOS_ATTR_PRI_MAX);
	else
		mvOsPrintf("DSCP:INV\t");

	mvOsPrintf("QUEUE:0x%2x\t", (mc->sram.regs.data3 >> MV_PP2_MC_DATA3_QUEUE) & 0xFF);/*TODO use gbe define*/

	if (mc->sram.regs.data3 & MV_PP2_MC_DATA3_HWF_EN)
		mvOsPrintf("HW_FWD:ENABLE\t");

	else
		mvOsPrintf("HW_FWD:DISABLE\t");

	mvOsPrintf("NEXT:0x%2x\t", (mc->sram.regs.data3 >> MV_PP2_MC_DATA3_NEXT) & MV_PP2_MC_INDEX_MAX);

	mvOsPrintf("\n");

	return MV_OK;
}

/*-------------------------------------------------------------------------------*/
int	mvPp2McHwDump(void)
{
	int index;
	MV_PP2_MC_ENTRY mc;

	for (index = 0; index < MV_PP2_MC_TBL_SIZE; index++) {
		mc.index = index;
		mvPp2McHwRead(&mc, index);
		mvPp2McSwDump(&mc);
		mvOsPrintf("-------------------------------------------------------------------------");
		mvOsPrintf("-------------------------------------------------------------------------\n");

	}

	return MV_OK;
}

/*-------------------------------------------------------------------------------*/
void	mvPp2McSwClear(MV_PP2_MC_ENTRY *mc)
{
	memset(mc, 0, sizeof(MV_PP2_MC_ENTRY));
}

/*-------------------------------------------------------------------------------*/
void	mvPp2McHwClearAll(void)
{
	int index;
	MV_PP2_MC_ENTRY mc;

	mvPp2McSwClear(&mc);

	for (index = 0; index < MV_PP2_MC_TBL_SIZE; index++)
		mvPp2McHwWrite(&mc, index);

}
/*-------------------------------------------------------------------------------*/

int	mvPp2McSwModSet(MV_PP2_MC_ENTRY *mc, int data_ptr, int instr_offs)
{
	PTR_VALIDATE(mc);
	POS_RANGE_VALIDATE(data_ptr, ACT_HWF_ATTR_DPTR_MAX);
	POS_RANGE_VALIDATE(instr_offs, ACT_HWF_ATTR_IPTR_MAX);

	mc->sram.regs.data1 &= ~(ACT_HWF_ATTR_DPTR_MAX << MV_PP2_MC_DATA1_DPTR);
	mc->sram.regs.data1 |= (data_ptr << MV_PP2_MC_DATA1_DPTR);

	mc->sram.regs.data1 &= ~(ACT_HWF_ATTR_IPTR_MAX << MV_PP2_MC_DATA1_IPTR);
	mc->sram.regs.data1 |= (instr_offs << MV_PP2_MC_DATA1_IPTR);

	return MV_OK;
}
/*-------------------------------------------------------------------------------*/
int	mvPp2McSwGpidSet(MV_PP2_MC_ENTRY *mc, int gpid, int enable)
{
	PTR_VALIDATE(mc);
	POS_RANGE_VALIDATE(gpid, ACT_QOS_ATTR_GEM_ID_MAX);
	POS_RANGE_VALIDATE(enable, 1);
	if (enable) {
		mc->sram.regs.data2 &= ~(ACT_QOS_ATTR_GEM_ID_MAX << MV_PP2_MC_DATA2_GEM_ID);
		mc->sram.regs.data2 |= (gpid << MV_PP2_MC_DATA2_GEM_ID);
		mc->sram.regs.data2 |= MV_PP2_MC_DATA2_GEM_ID_EN;

	} else
		mc->sram.regs.data2 &= ~MV_PP2_MC_DATA2_GEM_ID_EN;

	return MV_OK;

}
/*-------------------------------------------------------------------------------*/
int	mvPp2McSwDscpSet(MV_PP2_MC_ENTRY *mc, int dscp, int enable)
{
	PTR_VALIDATE(mc);
	POS_RANGE_VALIDATE(dscp, ACT_QOS_ATTR_DSCP_MAX);
	POS_RANGE_VALIDATE(enable, 1);
	if (enable) {
		mc->sram.regs.data2 &= ~(ACT_QOS_ATTR_DSCP_MAX << MV_PP2_MC_DATA2_DSCP);
		mc->sram.regs.data2 |= (dscp << MV_PP2_MC_DATA2_DSCP);
		mc->sram.regs.data2 |= MV_PP2_MC_DATA2_DSCP_EN;

	} else
		mc->sram.regs.data2 &= MV_PP2_MC_DATA2_DSCP_EN;

	return MV_OK;

}
/*-------------------------------------------------------------------------------*/
int	mvPp2McSwPrioSet(MV_PP2_MC_ENTRY *mc, int prio, int enable)
{
	PTR_VALIDATE(mc);
	POS_RANGE_VALIDATE(prio, ACT_QOS_ATTR_PRI_MAX);
	POS_RANGE_VALIDATE(enable, 1);
	if (enable) {
		mc->sram.regs.data2 &= ~(ACT_QOS_ATTR_PRI_MAX << MV_PP2_MC_DATA2_PRI);
		mc->sram.regs.data2 |= (prio << MV_PP2_MC_DATA2_PRI);
		mc->sram.regs.data2 |= MV_PP2_MC_DATA2_PRI_EN;

	} else
		mc->sram.regs.data2 &= ~MV_PP2_MC_DATA2_PRI_EN;

	return MV_OK;

}
/*-------------------------------------------------------------------------------*/
int	mvPp2McSwQueueSet(MV_PP2_MC_ENTRY *mc, int q)
{
	PTR_VALIDATE(mc);
	POS_RANGE_VALIDATE(q, 0xFF);/*TODO use gbe define*/

	mc->sram.regs.data3 &= ~(0xFF << MV_PP2_MC_DATA3_QUEUE);/*TODO use gbe define*/
	mc->sram.regs.data3 |= (q << MV_PP2_MC_DATA3_QUEUE);

	return MV_OK;
}
/*-------------------------------------------------------------------------------*/
int	mvPp2McSwForwardEn(MV_PP2_MC_ENTRY *mc, int enable)
{
	PTR_VALIDATE(mc);
	POS_RANGE_VALIDATE(enable, 1);

	if (enable)
		mc->sram.regs.data3 |= MV_PP2_MC_DATA3_HWF_EN;
	else
		mc->sram.regs.data3 &= ~MV_PP2_MC_DATA3_HWF_EN;

	return MV_OK;

}
/*-------------------------------------------------------------------------------*/

int	mvPp2McSwNext(MV_PP2_MC_ENTRY *mc, int next)
{
	PTR_VALIDATE(mc);
	/* if next = -1 last link */
	DECIMAL_RANGE_VALIDATE(next, -1, MV_PP2_MC_INDEX_MAX);

	mc->sram.regs.data3 &= ~(MV_PP2_MC_INDEX_MAX << MV_PP2_MC_DATA3_NEXT);
	mc->sram.regs.data3 |= (next << MV_PP2_MC_DATA3_NEXT);

	return MV_OK;

}
/*-------------------------------------------------------------------------------*/
