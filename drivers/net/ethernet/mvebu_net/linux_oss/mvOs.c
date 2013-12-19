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

/*******************************************************************************
* mvOsCpuArchLib.c - Marvell CPU architecture library
*
* DESCRIPTION:
*       This library introduce Marvell API for OS dependent CPU architecture
*       APIs. This library introduce single CPU architecture services APKI
*       cross OS.
*
* DEPENDENCIES:
*       None.
*
*******************************************************************************/

/* includes */
#include "mvOs.h"

static MV_U32 read_p15_c0(void);
static MV_U32 read_p15_c1(void);

/* defines  */
#define ARM_ID_REVISION_OFFS	0
#define ARM_ID_REVISION_MASK	(0xf << ARM_ID_REVISION_OFFS)

#define ARM_ID_PART_NUM_OFFS	4
#define ARM_ID_PART_NUM_MASK	(0xfff << ARM_ID_PART_NUM_OFFS)

#define ARM_ID_ARCH_OFFS	16
#define ARM_ID_ARCH_MASK	(0xf << ARM_ID_ARCH_OFFS)

#define ARM_ID_VAR_OFFS		20
#define ARM_ID_VAR_MASK		(0xf << ARM_ID_VAR_OFFS)

#define ARM_ID_ASCII_OFFS	24
#define ARM_ID_ASCII_MASK	(0xff << ARM_ID_ASCII_OFFS)

#define ARM_FEATURE_THUMBEE_OFFS	12
#define ARM_FEATURE_THUMBEE_MASK	(0xf << ARM_FEATURE_THUMBEE_OFFS)


void *mvOsIoCachedMalloc(void *osHandle, MV_U32 size, MV_ULONG *pPhyAddr,
			  MV_U32 *memHandle)
{
	void *p = kmalloc(size, GFP_ATOMIC);
	dma_addr_t dma_addr;
	dma_addr = pci_map_single(osHandle, p, 0, PCI_DMA_BIDIRECTIONAL);
	*pPhyAddr = (MV_ULONG)(dma_addr & 0xFFFFFFFF);
	return p;
}
void *mvOsIoUncachedMalloc(void *osHandle, MV_U32 size, MV_ULONG *pPhyAddr,
			    MV_U32 *memHandle)
{
	dma_addr_t dma_addr;
	void *ptr = pci_alloc_consistent(osHandle, size, &dma_addr);
	*pPhyAddr = (MV_ULONG)(dma_addr & 0xFFFFFFFF);
	return ptr;
}

void mvOsIoUncachedFree(void *osHandle, MV_U32 size, MV_ULONG phyAddr, void *pVirtAddr,
			 MV_U32 memHandle)
{
	pci_free_consistent(osHandle, size, pVirtAddr, (dma_addr_t)phyAddr);
}

void mvOsIoCachedFree(void *osHandle, MV_U32 size, MV_ULONG phyAddr, void *pVirtAddr,
		       MV_U32 memHandle)
{
	return kfree(pVirtAddr);
}

int mvOsRand(void)
{
	int rand;
	get_random_bytes(&rand, sizeof(rand));
	return rand;
}

/*******************************************************************************
* mvOsCpuVerGet() -
*
* DESCRIPTION:
*
* INPUT:
*       None.
*
* OUTPUT:
*       None.
*
* RETURN:
*       32bit CPU Revision
*
*******************************************************************************/
MV_U32 mvOsCpuRevGet(MV_VOID)
{
	return (read_p15_c0() & ARM_ID_REVISION_MASK) >> ARM_ID_REVISION_OFFS;
}
/*******************************************************************************
* mvOsCpuPartGet() -
*
* DESCRIPTION:
*
* INPUT:
*       None.
*
* OUTPUT:
*       None.
*
* RETURN:
*       32bit CPU Part number
*
*******************************************************************************/
MV_U32 mvOsCpuPartGet(MV_VOID)
{
	return (read_p15_c0() & ARM_ID_PART_NUM_MASK) >> ARM_ID_PART_NUM_OFFS;
}
/*******************************************************************************
* mvOsCpuArchGet() -
*
* DESCRIPTION:
*
* INPUT:
*       None.
*
* OUTPUT:
*       None.
*
* RETURN:
*       32bit CPU Architicture number
*
*******************************************************************************/
MV_U32 mvOsCpuArchGet(MV_VOID)
{
	return (read_p15_c0() & ARM_ID_ARCH_MASK) >> ARM_ID_ARCH_OFFS;
}
/*******************************************************************************
* mvOsCpuVarGet() -
*
* DESCRIPTION:
*
* INPUT:
*       None.
*
* OUTPUT:
*       None.
*
* RETURN:
*       32bit CPU Variant number
*
*******************************************************************************/
MV_U32 mvOsCpuVarGet(MV_VOID)
{
	return (read_p15_c0() & ARM_ID_VAR_MASK) >> ARM_ID_VAR_OFFS;
}
/*******************************************************************************
* mvOsCpuAsciiGet() -
*
* DESCRIPTION:
*
* INPUT:
*       None.
*
* OUTPUT:
*       None.
*
* RETURN:
*       32bit CPU Variant number
*
*******************************************************************************/
MV_U32 mvOsCpuAsciiGet(MV_VOID)
{
	return (read_p15_c0() & ARM_ID_ASCII_MASK) >> ARM_ID_ASCII_OFFS;
}

/*******************************************************************************
* mvOsCpuThumbEEGet() -
*
* DESCRIPTION:
*
* INPUT:
*       None.
*
* OUTPUT:
*       None.
*
* RETURN:
*       32bit CPU Variant number
*
*******************************************************************************/
MV_U32 mvOsCpuThumbEEGet(MV_VOID)
{
	return (read_p15_c1() & ARM_FEATURE_THUMBEE_MASK) >> ARM_FEATURE_THUMBEE_OFFS;
}

/*
static unsigned long read_p15_c0 (void)
*/
/* read co-processor 15, register #0 (ID register) */
static MV_U32 read_p15_c0 (void)
{
	MV_U32 value;

	__asm__ __volatile__(
		"mrc	p15, 0, %0, c0, c0, 0   @ read control reg\n"
		: "=r" (value)
		:
		: "memory");

	return value;
}

/* read co-processor 15, register #1 (Feature 0) */
static MV_U32 read_p15_c1 (void)
{
	MV_U32 value;

	__asm__ __volatile__(
						 "mrc	p15, 0, %0, c0, c1, 0   @ read feature0 reg\n"
	: "=r" (value)
	:
	: "memory");

	return value;
}
