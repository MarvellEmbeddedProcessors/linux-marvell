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
#define MV_DEV_FLAVOUR_MASK		(0xFF)
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


#define MV_PCI_DEVICE_FLAVOUR_TABLE_ENTRIES(devId)	\
	{ PCI_DEVICE(PCI_VENDOR_ID_MARVELL, devId + 0x00)},\
	{ PCI_DEVICE(PCI_VENDOR_ID_MARVELL, devId + 0x01)},\
	{ PCI_DEVICE(PCI_VENDOR_ID_MARVELL, devId + 0x02)},\
	{ PCI_DEVICE(PCI_VENDOR_ID_MARVELL, devId + 0x03)},\
	{ PCI_DEVICE(PCI_VENDOR_ID_MARVELL, devId + 0x04)},\
	{ PCI_DEVICE(PCI_VENDOR_ID_MARVELL, devId + 0x05)},\
	{ PCI_DEVICE(PCI_VENDOR_ID_MARVELL, devId + 0x06)},\
	{ PCI_DEVICE(PCI_VENDOR_ID_MARVELL, devId + 0x07)},\
	{ PCI_DEVICE(PCI_VENDOR_ID_MARVELL, devId + 0x08)},\
	{ PCI_DEVICE(PCI_VENDOR_ID_MARVELL, devId + 0x09)},\
	{ PCI_DEVICE(PCI_VENDOR_ID_MARVELL, devId + 0x0A)},\
	{ PCI_DEVICE(PCI_VENDOR_ID_MARVELL, devId + 0x0B)},\
	{ PCI_DEVICE(PCI_VENDOR_ID_MARVELL, devId + 0x0C)},\
	{ PCI_DEVICE(PCI_VENDOR_ID_MARVELL, devId + 0x0D)},\
	{ PCI_DEVICE(PCI_VENDOR_ID_MARVELL, devId + 0x0E)},\
	{ PCI_DEVICE(PCI_VENDOR_ID_MARVELL, devId + 0x0F)},\
	{ PCI_DEVICE(PCI_VENDOR_ID_MARVELL, devId + 0x10)},\
	{ PCI_DEVICE(PCI_VENDOR_ID_MARVELL, devId + 0x11)},\
	{ PCI_DEVICE(PCI_VENDOR_ID_MARVELL, devId + 0x12)},\
	{ PCI_DEVICE(PCI_VENDOR_ID_MARVELL, devId + 0x13)},\
	{ PCI_DEVICE(PCI_VENDOR_ID_MARVELL, devId + 0x14)},\
	{ PCI_DEVICE(PCI_VENDOR_ID_MARVELL, devId + 0x15)},\
	{ PCI_DEVICE(PCI_VENDOR_ID_MARVELL, devId + 0x16)},\
	{ PCI_DEVICE(PCI_VENDOR_ID_MARVELL, devId + 0x17)},\
	{ PCI_DEVICE(PCI_VENDOR_ID_MARVELL, devId + 0x18)},\
	{ PCI_DEVICE(PCI_VENDOR_ID_MARVELL, devId + 0x19)},\
	{ PCI_DEVICE(PCI_VENDOR_ID_MARVELL, devId + 0x1A)},\
	{ PCI_DEVICE(PCI_VENDOR_ID_MARVELL, devId + 0x1B)},\
	{ PCI_DEVICE(PCI_VENDOR_ID_MARVELL, devId + 0x1C)},\
	{ PCI_DEVICE(PCI_VENDOR_ID_MARVELL, devId + 0x1D)},\
	{ PCI_DEVICE(PCI_VENDOR_ID_MARVELL, devId + 0x1E)},\
	{ PCI_DEVICE(PCI_VENDOR_ID_MARVELL, devId + 0x1F)},\
	{ PCI_DEVICE(PCI_VENDOR_ID_MARVELL, devId + 0x20)},\
	{ PCI_DEVICE(PCI_VENDOR_ID_MARVELL, devId + 0x21)},\
	{ PCI_DEVICE(PCI_VENDOR_ID_MARVELL, devId + 0x22)},\
	{ PCI_DEVICE(PCI_VENDOR_ID_MARVELL, devId + 0x23)},\
	{ PCI_DEVICE(PCI_VENDOR_ID_MARVELL, devId + 0x24)},\
	{ PCI_DEVICE(PCI_VENDOR_ID_MARVELL, devId + 0x25)},\
	{ PCI_DEVICE(PCI_VENDOR_ID_MARVELL, devId + 0x26)},\
	{ PCI_DEVICE(PCI_VENDOR_ID_MARVELL, devId + 0x27)},\
	{ PCI_DEVICE(PCI_VENDOR_ID_MARVELL, devId + 0x28)},\
	{ PCI_DEVICE(PCI_VENDOR_ID_MARVELL, devId + 0x29)},\
	{ PCI_DEVICE(PCI_VENDOR_ID_MARVELL, devId + 0x2A)},\
	{ PCI_DEVICE(PCI_VENDOR_ID_MARVELL, devId + 0x2B)},\
	{ PCI_DEVICE(PCI_VENDOR_ID_MARVELL, devId + 0x2C)},\
	{ PCI_DEVICE(PCI_VENDOR_ID_MARVELL, devId + 0x2D)},\
	{ PCI_DEVICE(PCI_VENDOR_ID_MARVELL, devId + 0x2E)},\
	{ PCI_DEVICE(PCI_VENDOR_ID_MARVELL, devId + 0x2F)},\
	{ PCI_DEVICE(PCI_VENDOR_ID_MARVELL, devId + 0x30)},\
	{ PCI_DEVICE(PCI_VENDOR_ID_MARVELL, devId + 0x31)},\
	{ PCI_DEVICE(PCI_VENDOR_ID_MARVELL, devId + 0x32)},\
	{ PCI_DEVICE(PCI_VENDOR_ID_MARVELL, devId + 0x33)},\
	{ PCI_DEVICE(PCI_VENDOR_ID_MARVELL, devId + 0x34)},\
	{ PCI_DEVICE(PCI_VENDOR_ID_MARVELL, devId + 0x35)},\
	{ PCI_DEVICE(PCI_VENDOR_ID_MARVELL, devId + 0x36)},\
	{ PCI_DEVICE(PCI_VENDOR_ID_MARVELL, devId + 0x37)},\
	{ PCI_DEVICE(PCI_VENDOR_ID_MARVELL, devId + 0x38)},\
	{ PCI_DEVICE(PCI_VENDOR_ID_MARVELL, devId + 0x39)},\
	{ PCI_DEVICE(PCI_VENDOR_ID_MARVELL, devId + 0x3A)},\
	{ PCI_DEVICE(PCI_VENDOR_ID_MARVELL, devId + 0x3B)},\
	{ PCI_DEVICE(PCI_VENDOR_ID_MARVELL, devId + 0x3C)},\
	{ PCI_DEVICE(PCI_VENDOR_ID_MARVELL, devId + 0x3D)},\
	{ PCI_DEVICE(PCI_VENDOR_ID_MARVELL, devId + 0x3E)},\
	{ PCI_DEVICE(PCI_VENDOR_ID_MARVELL, devId + 0x3F)},\
	{ PCI_DEVICE(PCI_VENDOR_ID_MARVELL, devId + 0x40)},\
	{ PCI_DEVICE(PCI_VENDOR_ID_MARVELL, devId + 0x41)},\
	{ PCI_DEVICE(PCI_VENDOR_ID_MARVELL, devId + 0x42)},\
	{ PCI_DEVICE(PCI_VENDOR_ID_MARVELL, devId + 0x43)},\
	{ PCI_DEVICE(PCI_VENDOR_ID_MARVELL, devId + 0x44)},\
	{ PCI_DEVICE(PCI_VENDOR_ID_MARVELL, devId + 0x45)},\
	{ PCI_DEVICE(PCI_VENDOR_ID_MARVELL, devId + 0x46)},\
	{ PCI_DEVICE(PCI_VENDOR_ID_MARVELL, devId + 0x47)},\
	{ PCI_DEVICE(PCI_VENDOR_ID_MARVELL, devId + 0x48)},\
	{ PCI_DEVICE(PCI_VENDOR_ID_MARVELL, devId + 0x49)},\
	{ PCI_DEVICE(PCI_VENDOR_ID_MARVELL, devId + 0x4A)},\
	{ PCI_DEVICE(PCI_VENDOR_ID_MARVELL, devId + 0x4B)},\
	{ PCI_DEVICE(PCI_VENDOR_ID_MARVELL, devId + 0x4C)},\
	{ PCI_DEVICE(PCI_VENDOR_ID_MARVELL, devId + 0x4D)},\
	{ PCI_DEVICE(PCI_VENDOR_ID_MARVELL, devId + 0x4E)},\
	{ PCI_DEVICE(PCI_VENDOR_ID_MARVELL, devId + 0x4F)},\
	{ PCI_DEVICE(PCI_VENDOR_ID_MARVELL, devId + 0x50)},\
	{ PCI_DEVICE(PCI_VENDOR_ID_MARVELL, devId + 0x51)},\
	{ PCI_DEVICE(PCI_VENDOR_ID_MARVELL, devId + 0x52)},\
	{ PCI_DEVICE(PCI_VENDOR_ID_MARVELL, devId + 0x53)},\
	{ PCI_DEVICE(PCI_VENDOR_ID_MARVELL, devId + 0x54)},\
	{ PCI_DEVICE(PCI_VENDOR_ID_MARVELL, devId + 0x55)},\
	{ PCI_DEVICE(PCI_VENDOR_ID_MARVELL, devId + 0x56)},\
	{ PCI_DEVICE(PCI_VENDOR_ID_MARVELL, devId + 0x57)},\
	{ PCI_DEVICE(PCI_VENDOR_ID_MARVELL, devId + 0x58)},\
	{ PCI_DEVICE(PCI_VENDOR_ID_MARVELL, devId + 0x59)},\
	{ PCI_DEVICE(PCI_VENDOR_ID_MARVELL, devId + 0x5A)},\
	{ PCI_DEVICE(PCI_VENDOR_ID_MARVELL, devId + 0x5B)},\
	{ PCI_DEVICE(PCI_VENDOR_ID_MARVELL, devId + 0x5C)},\
	{ PCI_DEVICE(PCI_VENDOR_ID_MARVELL, devId + 0x5D)},\
	{ PCI_DEVICE(PCI_VENDOR_ID_MARVELL, devId + 0x5E)},\
	{ PCI_DEVICE(PCI_VENDOR_ID_MARVELL, devId + 0x5F)},\
	{ PCI_DEVICE(PCI_VENDOR_ID_MARVELL, devId + 0x60)},\
	{ PCI_DEVICE(PCI_VENDOR_ID_MARVELL, devId + 0x61)},\
	{ PCI_DEVICE(PCI_VENDOR_ID_MARVELL, devId + 0x62)},\
	{ PCI_DEVICE(PCI_VENDOR_ID_MARVELL, devId + 0x63)},\
	{ PCI_DEVICE(PCI_VENDOR_ID_MARVELL, devId + 0x64)},\
	{ PCI_DEVICE(PCI_VENDOR_ID_MARVELL, devId + 0x65)},\
	{ PCI_DEVICE(PCI_VENDOR_ID_MARVELL, devId + 0x66)},\
	{ PCI_DEVICE(PCI_VENDOR_ID_MARVELL, devId + 0x67)},\
	{ PCI_DEVICE(PCI_VENDOR_ID_MARVELL, devId + 0x68)},\
	{ PCI_DEVICE(PCI_VENDOR_ID_MARVELL, devId + 0x69)},\
	{ PCI_DEVICE(PCI_VENDOR_ID_MARVELL, devId + 0x6A)},\
	{ PCI_DEVICE(PCI_VENDOR_ID_MARVELL, devId + 0x6B)},\
	{ PCI_DEVICE(PCI_VENDOR_ID_MARVELL, devId + 0x6C)},\
	{ PCI_DEVICE(PCI_VENDOR_ID_MARVELL, devId + 0x6D)},\
	{ PCI_DEVICE(PCI_VENDOR_ID_MARVELL, devId + 0x6E)},\
	{ PCI_DEVICE(PCI_VENDOR_ID_MARVELL, devId + 0x6F)},\
	{ PCI_DEVICE(PCI_VENDOR_ID_MARVELL, devId + 0x70)},\
	{ PCI_DEVICE(PCI_VENDOR_ID_MARVELL, devId + 0x71)},\
	{ PCI_DEVICE(PCI_VENDOR_ID_MARVELL, devId + 0x72)},\
	{ PCI_DEVICE(PCI_VENDOR_ID_MARVELL, devId + 0x73)},\
	{ PCI_DEVICE(PCI_VENDOR_ID_MARVELL, devId + 0x74)},\
	{ PCI_DEVICE(PCI_VENDOR_ID_MARVELL, devId + 0x75)},\
	{ PCI_DEVICE(PCI_VENDOR_ID_MARVELL, devId + 0x76)},\
	{ PCI_DEVICE(PCI_VENDOR_ID_MARVELL, devId + 0x77)},\
	{ PCI_DEVICE(PCI_VENDOR_ID_MARVELL, devId + 0x78)},\
	{ PCI_DEVICE(PCI_VENDOR_ID_MARVELL, devId + 0x79)},\
	{ PCI_DEVICE(PCI_VENDOR_ID_MARVELL, devId + 0x7A)},\
	{ PCI_DEVICE(PCI_VENDOR_ID_MARVELL, devId + 0x7B)},\
	{ PCI_DEVICE(PCI_VENDOR_ID_MARVELL, devId + 0x7C)},\
	{ PCI_DEVICE(PCI_VENDOR_ID_MARVELL, devId + 0x7D)},\
	{ PCI_DEVICE(PCI_VENDOR_ID_MARVELL, devId + 0x7E)},\
	{ PCI_DEVICE(PCI_VENDOR_ID_MARVELL, devId + 0x7F)},\
	{ PCI_DEVICE(PCI_VENDOR_ID_MARVELL, devId + 0x80)},\
	{ PCI_DEVICE(PCI_VENDOR_ID_MARVELL, devId + 0x81)},\
	{ PCI_DEVICE(PCI_VENDOR_ID_MARVELL, devId + 0x82)},\
	{ PCI_DEVICE(PCI_VENDOR_ID_MARVELL, devId + 0x83)},\
	{ PCI_DEVICE(PCI_VENDOR_ID_MARVELL, devId + 0x84)},\
	{ PCI_DEVICE(PCI_VENDOR_ID_MARVELL, devId + 0x85)},\
	{ PCI_DEVICE(PCI_VENDOR_ID_MARVELL, devId + 0x86)},\
	{ PCI_DEVICE(PCI_VENDOR_ID_MARVELL, devId + 0x87)},\
	{ PCI_DEVICE(PCI_VENDOR_ID_MARVELL, devId + 0x88)},\
	{ PCI_DEVICE(PCI_VENDOR_ID_MARVELL, devId + 0x89)},\
	{ PCI_DEVICE(PCI_VENDOR_ID_MARVELL, devId + 0x8A)},\
	{ PCI_DEVICE(PCI_VENDOR_ID_MARVELL, devId + 0x8B)},\
	{ PCI_DEVICE(PCI_VENDOR_ID_MARVELL, devId + 0x8C)},\
	{ PCI_DEVICE(PCI_VENDOR_ID_MARVELL, devId + 0x8D)},\
	{ PCI_DEVICE(PCI_VENDOR_ID_MARVELL, devId + 0x8E)},\
	{ PCI_DEVICE(PCI_VENDOR_ID_MARVELL, devId + 0x8F)},\
	{ PCI_DEVICE(PCI_VENDOR_ID_MARVELL, devId + 0x90)},\
	{ PCI_DEVICE(PCI_VENDOR_ID_MARVELL, devId + 0x91)},\
	{ PCI_DEVICE(PCI_VENDOR_ID_MARVELL, devId + 0x92)},\
	{ PCI_DEVICE(PCI_VENDOR_ID_MARVELL, devId + 0x93)},\
	{ PCI_DEVICE(PCI_VENDOR_ID_MARVELL, devId + 0x94)},\
	{ PCI_DEVICE(PCI_VENDOR_ID_MARVELL, devId + 0x95)},\
	{ PCI_DEVICE(PCI_VENDOR_ID_MARVELL, devId + 0x96)},\
	{ PCI_DEVICE(PCI_VENDOR_ID_MARVELL, devId + 0x97)},\
	{ PCI_DEVICE(PCI_VENDOR_ID_MARVELL, devId + 0x98)},\
	{ PCI_DEVICE(PCI_VENDOR_ID_MARVELL, devId + 0x99)},\
	{ PCI_DEVICE(PCI_VENDOR_ID_MARVELL, devId + 0x9A)},\
	{ PCI_DEVICE(PCI_VENDOR_ID_MARVELL, devId + 0x9B)},\
	{ PCI_DEVICE(PCI_VENDOR_ID_MARVELL, devId + 0x9C)},\
	{ PCI_DEVICE(PCI_VENDOR_ID_MARVELL, devId + 0x9D)},\
	{ PCI_DEVICE(PCI_VENDOR_ID_MARVELL, devId + 0x9E)},\
	{ PCI_DEVICE(PCI_VENDOR_ID_MARVELL, devId + 0x9F)},\
	{ PCI_DEVICE(PCI_VENDOR_ID_MARVELL, devId + 0xA0)},\
	{ PCI_DEVICE(PCI_VENDOR_ID_MARVELL, devId + 0xA1)},\
	{ PCI_DEVICE(PCI_VENDOR_ID_MARVELL, devId + 0xA2)},\
	{ PCI_DEVICE(PCI_VENDOR_ID_MARVELL, devId + 0xA3)},\
	{ PCI_DEVICE(PCI_VENDOR_ID_MARVELL, devId + 0xA4)},\
	{ PCI_DEVICE(PCI_VENDOR_ID_MARVELL, devId + 0xA5)},\
	{ PCI_DEVICE(PCI_VENDOR_ID_MARVELL, devId + 0xA6)},\
	{ PCI_DEVICE(PCI_VENDOR_ID_MARVELL, devId + 0xA7)},\
	{ PCI_DEVICE(PCI_VENDOR_ID_MARVELL, devId + 0xA8)},\
	{ PCI_DEVICE(PCI_VENDOR_ID_MARVELL, devId + 0xA9)},\
	{ PCI_DEVICE(PCI_VENDOR_ID_MARVELL, devId + 0xAA)},\
	{ PCI_DEVICE(PCI_VENDOR_ID_MARVELL, devId + 0xAB)},\
	{ PCI_DEVICE(PCI_VENDOR_ID_MARVELL, devId + 0xAC)},\
	{ PCI_DEVICE(PCI_VENDOR_ID_MARVELL, devId + 0xAD)},\
	{ PCI_DEVICE(PCI_VENDOR_ID_MARVELL, devId + 0xAE)},\
	{ PCI_DEVICE(PCI_VENDOR_ID_MARVELL, devId + 0xAF)},\
	{ PCI_DEVICE(PCI_VENDOR_ID_MARVELL, devId + 0xB0)},\
	{ PCI_DEVICE(PCI_VENDOR_ID_MARVELL, devId + 0xB1)},\
	{ PCI_DEVICE(PCI_VENDOR_ID_MARVELL, devId + 0xB2)},\
	{ PCI_DEVICE(PCI_VENDOR_ID_MARVELL, devId + 0xB3)},\
	{ PCI_DEVICE(PCI_VENDOR_ID_MARVELL, devId + 0xB4)},\
	{ PCI_DEVICE(PCI_VENDOR_ID_MARVELL, devId + 0xB5)},\
	{ PCI_DEVICE(PCI_VENDOR_ID_MARVELL, devId + 0xB6)},\
	{ PCI_DEVICE(PCI_VENDOR_ID_MARVELL, devId + 0xB7)},\
	{ PCI_DEVICE(PCI_VENDOR_ID_MARVELL, devId + 0xB8)},\
	{ PCI_DEVICE(PCI_VENDOR_ID_MARVELL, devId + 0xB9)},\
	{ PCI_DEVICE(PCI_VENDOR_ID_MARVELL, devId + 0xBA)},\
	{ PCI_DEVICE(PCI_VENDOR_ID_MARVELL, devId + 0xBB)},\
	{ PCI_DEVICE(PCI_VENDOR_ID_MARVELL, devId + 0xBC)},\
	{ PCI_DEVICE(PCI_VENDOR_ID_MARVELL, devId + 0xBD)},\
	{ PCI_DEVICE(PCI_VENDOR_ID_MARVELL, devId + 0xBE)},\
	{ PCI_DEVICE(PCI_VENDOR_ID_MARVELL, devId + 0xBF)},\
	{ PCI_DEVICE(PCI_VENDOR_ID_MARVELL, devId + 0xC0)},\
	{ PCI_DEVICE(PCI_VENDOR_ID_MARVELL, devId + 0xC1)},\
	{ PCI_DEVICE(PCI_VENDOR_ID_MARVELL, devId + 0xC2)},\
	{ PCI_DEVICE(PCI_VENDOR_ID_MARVELL, devId + 0xC3)},\
	{ PCI_DEVICE(PCI_VENDOR_ID_MARVELL, devId + 0xC4)},\
	{ PCI_DEVICE(PCI_VENDOR_ID_MARVELL, devId + 0xC5)},\
	{ PCI_DEVICE(PCI_VENDOR_ID_MARVELL, devId + 0xC6)},\
	{ PCI_DEVICE(PCI_VENDOR_ID_MARVELL, devId + 0xC7)},\
	{ PCI_DEVICE(PCI_VENDOR_ID_MARVELL, devId + 0xC8)},\
	{ PCI_DEVICE(PCI_VENDOR_ID_MARVELL, devId + 0xC9)},\
	{ PCI_DEVICE(PCI_VENDOR_ID_MARVELL, devId + 0xCA)},\
	{ PCI_DEVICE(PCI_VENDOR_ID_MARVELL, devId + 0xCB)},\
	{ PCI_DEVICE(PCI_VENDOR_ID_MARVELL, devId + 0xCC)},\
	{ PCI_DEVICE(PCI_VENDOR_ID_MARVELL, devId + 0xCD)},\
	{ PCI_DEVICE(PCI_VENDOR_ID_MARVELL, devId + 0xCE)},\
	{ PCI_DEVICE(PCI_VENDOR_ID_MARVELL, devId + 0xCF)},\
	{ PCI_DEVICE(PCI_VENDOR_ID_MARVELL, devId + 0xD0)},\
	{ PCI_DEVICE(PCI_VENDOR_ID_MARVELL, devId + 0xD1)},\
	{ PCI_DEVICE(PCI_VENDOR_ID_MARVELL, devId + 0xD2)},\
	{ PCI_DEVICE(PCI_VENDOR_ID_MARVELL, devId + 0xD3)},\
	{ PCI_DEVICE(PCI_VENDOR_ID_MARVELL, devId + 0xD4)},\
	{ PCI_DEVICE(PCI_VENDOR_ID_MARVELL, devId + 0xD5)},\
	{ PCI_DEVICE(PCI_VENDOR_ID_MARVELL, devId + 0xD6)},\
	{ PCI_DEVICE(PCI_VENDOR_ID_MARVELL, devId + 0xD7)},\
	{ PCI_DEVICE(PCI_VENDOR_ID_MARVELL, devId + 0xD8)},\
	{ PCI_DEVICE(PCI_VENDOR_ID_MARVELL, devId + 0xD9)},\
	{ PCI_DEVICE(PCI_VENDOR_ID_MARVELL, devId + 0xDA)},\
	{ PCI_DEVICE(PCI_VENDOR_ID_MARVELL, devId + 0xDB)},\
	{ PCI_DEVICE(PCI_VENDOR_ID_MARVELL, devId + 0xDC)},\
	{ PCI_DEVICE(PCI_VENDOR_ID_MARVELL, devId + 0xDD)},\
	{ PCI_DEVICE(PCI_VENDOR_ID_MARVELL, devId + 0xDE)},\
	{ PCI_DEVICE(PCI_VENDOR_ID_MARVELL, devId + 0xDF)},\
	{ PCI_DEVICE(PCI_VENDOR_ID_MARVELL, devId + 0xE0)},\
	{ PCI_DEVICE(PCI_VENDOR_ID_MARVELL, devId + 0xE1)},\
	{ PCI_DEVICE(PCI_VENDOR_ID_MARVELL, devId + 0xE2)},\
	{ PCI_DEVICE(PCI_VENDOR_ID_MARVELL, devId + 0xE3)},\
	{ PCI_DEVICE(PCI_VENDOR_ID_MARVELL, devId + 0xE4)},\
	{ PCI_DEVICE(PCI_VENDOR_ID_MARVELL, devId + 0xE5)},\
	{ PCI_DEVICE(PCI_VENDOR_ID_MARVELL, devId + 0xE6)},\
	{ PCI_DEVICE(PCI_VENDOR_ID_MARVELL, devId + 0xE7)},\
	{ PCI_DEVICE(PCI_VENDOR_ID_MARVELL, devId + 0xE8)},\
	{ PCI_DEVICE(PCI_VENDOR_ID_MARVELL, devId + 0xE9)},\
	{ PCI_DEVICE(PCI_VENDOR_ID_MARVELL, devId + 0xEA)},\
	{ PCI_DEVICE(PCI_VENDOR_ID_MARVELL, devId + 0xEB)},\
	{ PCI_DEVICE(PCI_VENDOR_ID_MARVELL, devId + 0xEC)},\
	{ PCI_DEVICE(PCI_VENDOR_ID_MARVELL, devId + 0xED)},\
	{ PCI_DEVICE(PCI_VENDOR_ID_MARVELL, devId + 0xEE)},\
	{ PCI_DEVICE(PCI_VENDOR_ID_MARVELL, devId + 0xEF)},\
	{ PCI_DEVICE(PCI_VENDOR_ID_MARVELL, devId + 0xF0)},\
	{ PCI_DEVICE(PCI_VENDOR_ID_MARVELL, devId + 0xF1)},\
	{ PCI_DEVICE(PCI_VENDOR_ID_MARVELL, devId + 0xF2)},\
	{ PCI_DEVICE(PCI_VENDOR_ID_MARVELL, devId + 0xF3)},\
	{ PCI_DEVICE(PCI_VENDOR_ID_MARVELL, devId + 0xF4)},\
	{ PCI_DEVICE(PCI_VENDOR_ID_MARVELL, devId + 0xF5)},\
	{ PCI_DEVICE(PCI_VENDOR_ID_MARVELL, devId + 0xF6)},\
	{ PCI_DEVICE(PCI_VENDOR_ID_MARVELL, devId + 0xF7)},\
	{ PCI_DEVICE(PCI_VENDOR_ID_MARVELL, devId + 0xF8)},\
	{ PCI_DEVICE(PCI_VENDOR_ID_MARVELL, devId + 0xF9)},\
	{ PCI_DEVICE(PCI_VENDOR_ID_MARVELL, devId + 0xFA)},\
	{ PCI_DEVICE(PCI_VENDOR_ID_MARVELL, devId + 0xFB)},\
	{ PCI_DEVICE(PCI_VENDOR_ID_MARVELL, devId + 0xFC)},\
	{ PCI_DEVICE(PCI_VENDOR_ID_MARVELL, devId + 0xFD)},\
	{ PCI_DEVICE(PCI_VENDOR_ID_MARVELL, devId + 0xFE)},\
	{ PCI_DEVICE(PCI_VENDOR_ID_MARVELL, devId + 0xFF)}


#endif /* __MV_PRESTERA_PCI_H */
