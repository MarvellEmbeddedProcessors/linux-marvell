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
********************************************************************************
* mv_prestera_pci.h
*
* DESCRIPTION:
*       Includes defines and structures needed by the PCI PP device driver
*
* DEPENDENCIES:
*       None.
*
*******************************************************************************/
#ifndef __MV_PRESTERA_PCI_H
#define __MV_PRESTERA_PCI_H

/* PCI Devices definition */
#define MARVELL_VEN_ID			(0x11AB)
#define MV_BOBCAT2_DEV_ID		(0xFC00)
#define MV_LION2_DEV_ID			(0x8000)
#define MV_ALLEYCAT3_DEV_ID		(0xF400)
#define PCI_VENDOR_ID_IDT_SWITCH	(0x111D)
#define MV_IDT_SWITCH_DEV_ID_808E	(0x808E)
#define MV_IDT_SWITCH_DEV_ID_802B	(0x802B)

/* General definition */
#define ENABLE		(1)
#define DISABLE		(0)
#define TBL_TERM	0xFF

/* PCI BAR definition */
#define PEX_0		(0)
#define BAR_1		(1)
#define BAR_2		(2)

#define PEX_IRQ_EN			BIT(28)
#define PEX_IRQ_EP			BIT(31)

#define PXBCR_BAR_EN			(0x00000001)
#define PXBCR_BAR_SIZE_OFFS		(16)
#define PXBCR_BAR_SIZE_MASK		(0xFFFF << PXBCR_BAR_SIZE_OFFS)
#define PXBCR_BAR_SIZE_ALIGNMENT	(0x10000)

#define SIZE_TO_BAR_REG(size)	((((size) / PXBCR_BAR_SIZE_ALIGNMENT) - 1) << PXBCR_BAR_SIZE_OFFS)

#define PXWRR_REMAP_EN				(BIT(0))
#define PXWCR_WIN_EN				(BIT(0)) /* Window Enable.*/
#define PXWCR_WIN_BAR_MAP_OFFS			(1)    /* Mapping to BAR.*/
#define PXWCR_WIN_BAR_MAP_MASK			(BIT(1))
#define PXWCR_WIN_BAR_MAP_BAR1			(0 << PXWCR_WIN_BAR_MAP_OFFS)
#define PXWCR_WIN_BAR_MAP_BAR2			(1 << PXWCR_WIN_BAR_MAP_OFFS)
#define PXWCR_TARGET_OFFS			(4)  /*Unit ID */
#define PXWCR_TARGET_MASK			(0xf << PXWCR_TARGET_OFFS)
#define PXWCR_ATTRIB_OFFS			(8)  /*Target attributes */

#define PEX_WIN0_3_CTRL_REG(pexIf, winNum)	(0x41820 + (winNum) * 0x10 - (pexIf) * 0x10000)
#define PEX_WIN0_3_BASE_REG(pexIf, winNum)	(0x41824 + (winNum) * 0x10 - (pexIf) * 0x10000)
#define PEX_WIN0_3_REMAP_REG(pexIf, winNum)	(0x4182C + (winNum) * 0x10 - (pexIf) * 0x10000)
#define PEX_WIN4_5_CTRL_REG(pexIf, winNum)	(0x41860 + (winNum - 4) * 0x20 - (pexIf) * 0x10000)
#define PEX_WIN4_5_BASE_REG(pexIf, winNum)	(0x41864 + (winNum - 4) * 0x20 - (pexIf) * 0x10000)
#define PEX_WIN4_5_REMAP_REG(pexIf, winNum)	(0x4186C + (winNum - 4) * 0x20 - (pexIf) * 0x10000)
#define PEX_WIN4_5_REMAP_HIGH_REG(pexIf, winNum)	(0x41870 + (winNum - 4) * 0x20 - (pexIf) * 0x10000)

/* IDT Switch definition */
#define MAX_NUM_OF_IDT_SWITCH	(2)
#define MAX_NUM_OF_PP	(4)
#define MV_IDT_SWITCH_MAX_NUM_OF_PORT	(MAX_NUM_OF_PP)

#define MV_IDT_SWITCH_PCI_MEM_BASE_REG		(0x20)
#define MV_IDT_SWITCH_PCI_MEM_BASE_REG_MASK	(0xFFFF0000)
#define MV_IDT_SWITCH_PCI_MEM_BASE_REG_SHIFT	(16)
#define MV_IDT_SWITCH_PCI_LINK_STATUS_REG	(0x52)
#define MV_IDT_SWITCH_PCI_LINK_STATUS_REG_ACTIVE_LINK	(0x2000)

/* IDT Switch structure */
struct idtSwitchConfig {
	unsigned int ppIdtSwitchUsBusNum;
	unsigned int ppIdtSwitchDsBusNum;
	unsigned int numOfPpInstances;

	struct {
		unsigned long startAddr[MV_IDT_SWITCH_MAX_NUM_OF_PORT];
		unsigned long endAddr[MV_IDT_SWITCH_MAX_NUM_OF_PORT];
		unsigned int ppBusNumArray[MV_IDT_SWITCH_MAX_NUM_OF_PORT];
		unsigned int ppIdtSwitchDsPpDevices[MV_IDT_SWITCH_MAX_NUM_OF_PORT];
	} idtSwPortCfg;
};

/* PCI BAR enumeration */
enum {
	MV_PCI_BAR_INTER_REGS	= 0,
	MV_PCI_BAR_1		= 2,
	MV_PCI_BAR_2		= 4,
};

struct pci_decoding_window {
	uint8_t win_num;
	uint8_t win_bar_map;
	int base_offset;	/* offset from BAR base */
	int size;		/* have to be 64KB granularity */
	int remap;
	uint8_t target_id;
	uint8_t attr;
	uint8_t enable;
};

/* PCI BAR macro */
#define MV_PEX_IF_REGS_OFFSET(pexIf)	(pexIf < 8 ? (0x40000 + ((pexIf) / 4) * 0x40000 + ((pexIf) % 4) * 0x4000) \
										 : (0x42000 + ((pexIf) % 8) * 0x40000))
#define MV_PEX_IF_REGS_BASE(unit)	(MV_PEX_IF_REGS_OFFSET(unit))
#define PEX_BAR_CTRL_REG(pexIf, bar)	(MV_PEX_IF_REGS_BASE(pexIf) + 0x1804 + (bar-1)*4)

/* For msys internal irq */
#define MV_MBUS_REGS_OFFSET			0x20000
#define MV_CPUIF_SHARED_REGS_BASE		MV_MBUS_REGS_OFFSET
#define CPU_INT_SOURCE_CONTROL_REG(i)		(MV_CPUIF_SHARED_REGS_BASE + 0xB00 + (i * 0x4))

#define CPU_INT_CLEAR_MASK_OFFS			0xBC
#define MV_CPUIF_LOCAL_REGS_OFFSET		0x21000
#define CPU_INT_CLEAR_MASK_LOCAL_REG	(MV_CPUIF_LOCAL_REGS_OFFSET + CPU_INT_CLEAR_MASK_OFFS)

#define MSYS_CAUSE_VEC1_REG_OFFS		0x20904
#define CPU_INT_SOURCE_CONTROL_IRQ_OFFS		28

#endif /* __MV_PRESTERA_PCI_H */
