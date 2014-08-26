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

#include "mvOs.h"
#include "mvCommon.h"

#include "gbe/mvNetaRegs.h"

#include "mvPnc.h"
#include "mvTcam.h"

#ifdef MV_ETH_PNC_AGING

#define PNC_AGING_CNTRS_ADDR_MASK       (0 << 12)
#define PNC_AGING_GROUPS_ADDR_MASK      (1 << 12)
#define PNC_AGING_SCANNER_ADDR_MASK     (2 << 12)

#define PNC_AGING_CNTR_IDX_ADDR_OFFS    2
#define PNC_AGING_GROUP_ADDR_OFFS       2
#define PNC_AGING_LOG_ADDR_OFFS         5

#define PNC_AGING_CNTR_OFFS             0
#define PNC_AGING_CNTR_MAX              0x3ffffff
#define PNC_AGING_CNTR_MASK             (PNC_AGING_CNTR_MAX << PNC_AGING_CNTR_OFFS)

#define PNC_AGING_GROUP_OFFS            26
#define PNC_AGING_GROUP_ALL_MASK        (0x3 << PNC_AGING_GROUP_OFFS)
#define PNC_AGING_GROUP_MASK(gr)        ((gr) << PNC_AGING_GROUP_OFFS)

#define PNC_AGING_READ_LU_LOG_BIT       28
#define PNC_AGING_READ_LU_LOG_MASK      (1 << PNC_AGING_READ_LU_LOG_BIT)

#define PNC_AGING_READ_MU_LOG_BIT       29
#define PNC_AGING_READ_MU_LOG_MASK      (1 << PNC_AGING_READ_MU_LOG_BIT)

#define PNC_AGING_SKIP_LU_SCAN_BIT      30
#define PNC_AGING_SKIP_LU_SCAN_MASK     (1 << PNC_AGING_SKIP_LU_SCAN_BIT)

#define PNC_AGING_SKIP_MU_SCAN_BIT      31
#define PNC_AGING_SKIP_MU_SCAN_MASK     (1 << PNC_AGING_SKIP_MU_SCAN_BIT)

#define PNC_AGING_LOG_CNTR_IDX_OFFS     0
#define PNC_AGING_LOG_CNTR_IDX_MASK     (0x3FF << PNC_AGING_LOG_CNTR_IDX_OFFS)

#define PNC_AGING_LOG_VALID_BIT         31
#define PNC_AGING_LOG_VALID_MASK        (1 << PNC_AGING_LOG_VALID_BIT)

void    mvPncAgingCntrWrite(int tid, MV_U32 w32)
{
	MV_U32  va;

	WARN_ON_OOR(tid >= MV_PNC_TCAM_SIZE());

	va = (MV_U32)mvPncVirtBase;
	va |= PNC_AGING_ACCESS_MASK;
	va |= PNC_AGING_CNTRS_ADDR_MASK;
	va |= (tid << PNC_AGING_CNTR_IDX_ADDR_OFFS);
/*
	mvOsPrintf("%s: tid=%d, va=0x%x, w32=0x%08x\n",
		__func__, tid, va, w32);
*/
	MV_MEMIO32_WRITE(va, w32);
}


MV_U32  mvPncAgingCntrRead(int tid)
{
	MV_U32  va, w32;

	ERR_ON_OOR(tid >= MV_PNC_TCAM_SIZE());

	va = (MV_U32)mvPncVirtBase;
	va |= PNC_AGING_ACCESS_MASK;
	va |= PNC_AGING_CNTRS_ADDR_MASK;
	va |= (tid << PNC_AGING_CNTR_IDX_ADDR_OFFS);

	w32 = MV_MEMIO32_READ(va);
/*
	mvOsPrintf("%s: tid=%d, va=0x%x, w32=0x%08x\n",
		__func__, tid, va, w32);
*/
	return w32;
}

MV_U32  mvPncAgingGroupCntrRead(int group)
{
	MV_U32  va, w32;

	ERR_ON_OOR(group >= MV_PNC_AGING_MAX_GROUP);

	va = (MV_U32)mvPncVirtBase;
	va |= PNC_AGING_ACCESS_MASK;
	va |= PNC_AGING_GROUPS_ADDR_MASK;
	va |= (group << PNC_AGING_GROUP_ADDR_OFFS);

	w32 = MV_MEMIO32_READ(va);

	return w32;
}

void    mvPncAgingGroupCntrClear(int group)
{
	MV_U32  w32;

	WARN_ON_OOR(group >= MV_PNC_AGING_MAX_GROUP);

	w32 = MV_REG_READ(MV_PNC_AGING_CTRL_REG);
	w32 |= MV_PNC_AGING_GROUP_RESET(group);
	MV_REG_WRITE(MV_PNC_AGING_CTRL_REG, w32);
}

MV_U32  mvPncAgingLogEntryRead(int group, int mostly)
{
	MV_U32  va, w32;

	ERR_ON_OOR(group >= MV_PNC_AGING_MAX_GROUP);

	va = (MV_U32)mvPncVirtBase;
	va |= PNC_AGING_ACCESS_MASK;
	va |= PNC_AGING_SCANNER_ADDR_MASK;
	va |= ((MV_PNC_AGING_MAX_GROUP * mostly + group) << PNC_AGING_LOG_ADDR_OFFS);

	w32 = MV_MEMIO32_READ(va);

	return w32;
}

void    mvPncAgingCntrShow(int tid, MV_32 w32)
{
	mvOsPrintf("[%3d] (%-12s): gr=%d - %10u", tid, tcam_text[tid],
		((w32 & PNC_AGING_GROUP_ALL_MASK) >> PNC_AGING_GROUP_OFFS),
		((w32 & PNC_AGING_CNTR_MASK) >> PNC_AGING_CNTR_OFFS));

	if (w32 & PNC_AGING_READ_LU_LOG_MASK)
		mvOsPrintf(", LU_READ");

	if (w32 & PNC_AGING_READ_MU_LOG_MASK)
		mvOsPrintf(", MU_READ");

	if (w32 & PNC_AGING_SKIP_LU_SCAN_MASK)
		mvOsPrintf(", LU_SKIP");

	if (w32 & PNC_AGING_SKIP_MU_SCAN_MASK)
		mvOsPrintf(", MU_SKIP");

	mvOsPrintf("\n");
}

void    mvPncAgingDump(int all)
{
	int     tid, gr;
	MV_U32  cntrVal;

	mvOsPrintf("TCAM entries Aging counters: %s\n", all ? "ALL" : "Non ZERO");
	for (tid = 0; tid < MV_PNC_TCAM_SIZE(); tid++) {
		cntrVal = mvPncAgingCntrRead(tid);

		if (all || (cntrVal & PNC_AGING_CNTR_MASK))
			mvPncAgingCntrShow(tid, cntrVal);
	}
	mvOsPrintf("Aging Counters Summary per group: \n");
	for (gr = 0; gr < MV_PNC_AGING_MAX_GROUP; gr++)
		mvOsPrintf("group #%d: %10u\n", gr, mvPncAgingGroupCntrRead(gr));
}

static MV_U32  mvPncScannerLog[MV_PNC_TCAM_LINES];
static MV_U32  mvPncAgingCntrs[MV_PNC_TCAM_LINES];

void    mvPncAgingScannerDump(void)
{
	int     i, j, gr;
	MV_U32  w32;

	mvOsPrintf("Scanner LU Log entries for aging counters:\n");
	for (gr = 0; gr < MV_PNC_AGING_MAX_GROUP; gr++) {
		i = 0;
		mvOsPrintf("LU group #%d:\n", gr);
		while (i < MV_PNC_TCAM_SIZE()) {
			w32 = mvPncAgingLogEntryRead(gr, 0);
			if ((w32 & PNC_AGING_LOG_VALID_MASK) == 0)
				break;

			mvOsDelay(20);
			mvPncAgingCntrs[i] = mvPncAgingCntrRead(w32 & PNC_AGING_LOG_CNTR_IDX_MASK);
			mvPncScannerLog[i] = w32;
			i++;
		}
		for (j = 0; j < i; j++) {
			mvOsPrintf("%d: 0x%08x - tid=%u, 0x%08x - cntr=%u\n",
				j, mvPncScannerLog[j], mvPncScannerLog[j] & PNC_AGING_LOG_CNTR_IDX_MASK,
				mvPncAgingCntrs[j],
				(mvPncAgingCntrs[j] & PNC_AGING_CNTR_MASK) >> PNC_AGING_CNTR_OFFS);
		}
	}

	mvOsPrintf("\n");
	mvOsPrintf("Scanner MU Log entries for aging counters:\n");
	for (gr = 0; gr < MV_PNC_AGING_MAX_GROUP; gr++) {
		i = 0;
		mvOsPrintf("MU group #%d:\n", gr);
		while (i < MV_PNC_TCAM_SIZE()) {
			w32 = mvPncAgingLogEntryRead(gr, 1);
			/*mvOsDelay(1);*/
			if ((w32 & PNC_AGING_LOG_VALID_MASK) == 0)
				break;

			mvOsDelay(20);
			mvPncAgingCntrs[i] = mvPncAgingCntrRead(w32 & PNC_AGING_LOG_CNTR_IDX_MASK);
			mvPncScannerLog[i] = w32;
			i++;
		}
		for (j = 0; j < i; j++) {
			mvOsPrintf("%d: 0x%08x - tid=%u, 0x%08x - cntr=%u\n",
				j, mvPncScannerLog[j], mvPncScannerLog[j] & PNC_AGING_LOG_CNTR_IDX_MASK,
				mvPncAgingCntrs[j],
				(mvPncAgingCntrs[j] & PNC_AGING_CNTR_MASK) >> PNC_AGING_CNTR_OFFS);
		}
	}
}

void    mvPncAgingCntrClear(int tid)
{
	MV_U32  w32;

	w32 = mvPncAgingCntrRead(tid);

	w32 &= ~PNC_AGING_CNTR_MASK;
	w32 &= ~(PNC_AGING_READ_LU_LOG_MASK | PNC_AGING_READ_MU_LOG_MASK);

	mvPncAgingCntrWrite(tid, w32);
}

void    mvPncAgingCntrGroupSet(int tid, int gr)
{
	MV_U32  w32;

	w32 = PNC_AGING_GROUP_MASK(gr);

	/*mvOsPrintf("%s: tid=%d, gr=%d, w32=0x%x\n", __FUNCTION__, tid, gr, w32);*/
	mvPncAgingCntrWrite(tid, w32);
}

/* Reset all Aging counters */
void    mvPncAgingReset(void)
{
	int tid, gr;

	for (tid = 0; tid < MV_PNC_TCAM_SIZE(); tid++)
		mvPncAgingCntrClear(tid);

	for (gr = 0; gr < MV_PNC_AGING_MAX_GROUP; gr++)
		mvPncAgingGroupCntrClear(gr);
}
#endif /* MV_ETH_PNC_AGING */
