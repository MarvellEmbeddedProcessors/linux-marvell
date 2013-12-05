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

#ifndef __mvBm_h__
#define __mvBm_h__

/* includes */
#include "mvTypes.h"
#include "mvCommon.h"
#include "mvStack.h"
#include "mv802_3.h"

#include "mvBmRegs.h"

typedef struct {
	int valid;
	int longPool;
	int shortPool;
	int longBufNum;
	int shortBufNum;

} MV_BM_CONFIG;

typedef struct {
	int         pool;
	int         capacity;
	int         bufNum;
	int         bufSize;
	MV_U32      *pVirt;
	MV_ULONG    physAddr;
} MV_BM_POOL;

extern MV_U8 *mvBmVirtBase;
/* defines */

/* bits[8-9] of address define pool 0-3 */
#define BM_POOL_ACCESS_OFFS     8

/* INLINE functions */
static INLINE void mvBmPoolPut(int poolId, MV_ULONG bufPhysAddr)
{
	*((MV_ULONG *)((unsigned)mvBmVirtBase | (poolId << BM_POOL_ACCESS_OFFS))) = (MV_ULONG)(MV_32BIT_LE(bufPhysAddr));
}

static INLINE MV_ULONG mvBmPoolGet(int poolId)
{
	MV_U32	bufPhysAddr = *(MV_U32 *)((unsigned)mvBmVirtBase | (poolId << 8));

	return (MV_ULONG)(MV_32BIT_LE(bufPhysAddr));
}

/* prototypes */
MV_STATUS mvBmInit(MV_U8 *virtBase);
void      mvBmRegsInit(void);
void      mvBmConfigSet(MV_U32 mask);
void      mvBmConfigClear(MV_U32 mask);
MV_STATUS mvBmControl(MV_COMMAND cmd);
MV_STATE  mvBmStateGet(void);
void      mvBmPoolTargetSet(int pool, MV_U8 targetId, MV_U8 attr);
void      mvBmPoolEnable(int pool);
void      mvBmPoolDisable(int pool);
MV_BOOL   mvBmPoolIsEnabled(int pool);
MV_STATUS mvBmPoolInit(int pool, void *virtPoolBase, MV_ULONG physPoolBase, int capacity);
MV_STATUS mvBmPoolBufNumUpdate(int pool, int buf_num);
MV_STATUS mvBmPoolBufSizeSet(int pool, int buf_size);
void      mvBmRegs(void);
void      mvBmStatus(void);
void      mvBmPoolDump(int pool, int mode);
void      mvBmPoolPrint(int pool);

#endif /* __mvBm_h__ */
