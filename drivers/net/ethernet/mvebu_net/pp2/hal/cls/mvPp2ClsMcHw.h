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

#ifndef __MV_CLS_MC_HW_H__
#define __MV_CLS_MC_HW_H__

#include "mvPp2ClsActHw.h"
#include "../common/mvPp2ErrCode.h"
#include "../common/mvPp2Common.h"
#include "../gbe/mvPp2GbeRegs.h"

/*-------------------------------------------------------------------------------*/
/*			Multicast table Top Registers	    			 */
/*-------------------------------------------------------------------------------*/
#define MV_PP2_MC_INDEX_REG			(MV_PP2_REG_BASE + 0x160)
#define MV_PP2_MC_INDEX_MAX			ACT_DUP_FID_MAX
/*-------------------------------------------------------------------------------*/

#define MV_PP2_MC_DATA1_REG			(MV_PP2_REG_BASE + 0x164)
#define	MV_PP2_MC_DATA1_DPTR			1
#define	MV_PP2_MC_DATA1_IPTR			16
/*-------------------------------------------------------------------------------*/

#define MV_PP2_MC_DATA2_REG			(MV_PP2_REG_BASE + 0x168)
#define MV_PP2_MC_DATA2_GEM_ID			0
#define MV_PP2_MC_DATA2_PRI			12
#define MV_PP2_MC_DATA2_DSCP			15
#define MV_PP2_MC_DATA2_GEM_ID_EN		(1 << 21)
#define MV_PP2_MC_DATA2_PRI_EN			(1 << 22)
#define MV_PP2_MC_DATA2_DSCP_EN			(1 << 23)
/*-------------------------------------------------------------------------------*/

#define MV_PP2_MC_DATA3_REG			(MV_PP2_REG_BASE + 0x16C)

#define MV_PP2_MC_DATA3_QUEUE			0

#define MV_PP2_MC_DATA3_HWF_EN			(1 << 8)

#define MV_PP2_MC_DATA3_NEXT			16
#define MV_PP2_MC_DATA3_NEXT_MASK		(MV_PP2_MC_INDEX_MAX << MV_PP2_MC_DATA3_NEXT)


typedef struct {
	int             valid;
	int		next;
} MC_SHADOW_ENTRY;

#define LAST 	(-1)
/*-------------------------------------------------------------------------------*/
/*			Multicast table Public APIs				 */
/*-------------------------------------------------------------------------------*/
#define MV_PP2_MC_TBL_SIZE		256
#define MV_PP2_MC_WORDS			3


typedef struct mvPp2McEntry {
	unsigned int index;
	union {
		MV_U32 words[MV_PP2_MC_WORDS];
		struct {
			MV_U32 data1;/* 0x164 */
			MV_U32 data2;/* 0x168 */
			MV_U32 data3;/* 0x16c */
		} regs;
	} sram;
} MV_PP2_MC_ENTRY;
/*
int	mvPp2McFirstFreeGet(void)
*/

int	mvPp2McHwWrite(MV_PP2_MC_ENTRY *mc, int index);
int	mvPp2McHwRead(MV_PP2_MC_ENTRY *mc, int index);
int	mvPp2McSwDump(MV_PP2_MC_ENTRY *mc);
int	mvPp2McHwDump(void);
void	mvPp2McSwClear(MV_PP2_MC_ENTRY *mc);
void	mvPp2McHwClearAll(void);


int	mvPp2McSwModSet(MV_PP2_MC_ENTRY *mc, int data_ptr, int instr_offs);
int	mvPp2McSwGpidSet(MV_PP2_MC_ENTRY *mc, int gpid, int enable);
int	mvPp2McSwDscpSet(MV_PP2_MC_ENTRY *mc, int dscp, int enable);
int	mvPp2McSwPrioSet(MV_PP2_MC_ENTRY *mc, int prio, int enable);
int	mvPp2McSwQueueSet(MV_PP2_MC_ENTRY *mc, int q);
int	mvPp2McSwForwardEn(MV_PP2_MC_ENTRY *mc, int enable);
int	mvPp2McSwNext(MV_PP2_MC_ENTRY *mc, int next);


#endif /*__MV_CLS_MC_HW_H__ */

