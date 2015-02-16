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
* mv_prestera.h
*
* DESCRIPTION:
*       Includes defines and structures needed by the PP device driver
*
* DEPENDENCIES:
*       None.
*
* COMMENTS:
*   Please note: this file is shared for:
*       axp_lsp_3.4.69
*       msys_lsp_3_4
*
*******************************************************************************/
#ifndef __MV_PRESTERA
#define __MV_PRESTERA

#define PRESTERA_PP_DRIVER

#include <linux/version.h>
#include "mv_prestera_glob.h"

#ifdef __BIG_ENDIAN
#define CPU_BE
#else
#define CPU_LE
#endif

#ifndef PRESTERA_MAJOR
#define PRESTERA_MAJOR	244	/* major number */
#endif

/* CPSS configurations */
#define CPSS_DMA_VIRT_ADDR	0x19000000
#define CPSS_SWITCH_VIRT_ADDR	0x20000000
#define CPSS_DFX_VIRT_ADDR		0x1b000000
#define CPSS_CPU_VIRT_ADDR	0x19400000
#define CPSS_VIRT_ADDR_MASK	0xf0000000

/* General definition */
#define ENABLE	(1)
#define DISABLE	(0)
#define _1M	(0x00100000)
#define _2M	(0x00200000)
#define _64M	(0x04000000)

/* PCI Devices definition */
#define MARVELL_VEN_ID			(0x11AB)
#define MV_BOBCAT2_DEV_ID		(0xFC00)
#define MV_LION2_DEV_ID			(0x8000)
#define MV_ALLEYCAT3_DEV_ID		(0xF400)
#define PCI_VENDOR_ID_IDT_SWITCH	(0x111D)
#define MV_IDT_SWITCH_DEV_ID_808E	(0x808E)
#define MV_IDT_SWITCH_DEV_ID_802B	(0x802B)

#define PCI_DEV_LION_CONFIG_OFFSET	0x70000
#define PCI_DEV_BC2_CONFIG_OFFSET	0x0
#define PCI_DEV_AC3_CONFIG_OFFSET	0x0
#define PCI_DEV_PEX_EN	1
#define PCI_DEV_DFX_EN	1
#define PCI_DEV_DFX_DIS	0

#define PCI_DEV_INTR_MAX_NUM	8
#define PCI_DEV_DEF_INTR_NUM	0
#define PCI_DEV_LION2_PP_1_INTR_NUM	3
#define PCI_DEV_LION2_PP_2_INTR_NUM	2
#define PCI_DEV_LION2_PP_3_INTR_NUM	1
#define PCI_DEV_LION2_PP_4_INTR_NUM	0

/* PCI BAR definition */
#define PEX_0	(0)
#define BAR_1	(1)
#define BAR_2	(2)

/* Bobcat2 Customer Boards */
#define BC2_CUSTOMER_BOARD_ID_BASE      0x0
#define BC2_CUSTOMER_BOARD_ID0          (BC2_CUSTOMER_BOARD_ID_BASE + 0)
#define BC2_CUSTOMER_BOARD_ID1          (BC2_CUSTOMER_BOARD_ID_BASE + 1)
#define BC2_CUSTOMER_MAX_BOARD_ID       (BC2_CUSTOMER_BOARD_ID_BASE + 2)
#define BC2_CUSTOMER_BOARD_NUM          (BC2_CUSTOMER_MAX_BOARD_ID - BC2_CUSTOMER_BOARD_ID_BASE)

/* Bobcat2 Marvell boards */
#define BC2_MARVELL_BOARD_ID_BASE       0x10
#define DB_DX_BC2_ID                            (BC2_MARVELL_BOARD_ID_BASE + 0)
#define RD_DX_BC2_ID                            (BC2_MARVELL_BOARD_ID_BASE + 1)
#define RD_MTL_BC2                                      (BC2_MARVELL_BOARD_ID_BASE + 2)
#define BC2_MARVELL_MAX_BOARD_ID        (BC2_MARVELL_BOARD_ID_BASE + 3)
#define BC2_MARVELL_BOARD_NUM           (BC2_MARVELL_MAX_BOARD_ID - BC2_MARVELL_BOARD_ID_BASE)

/* AlleyCat3 Customer Boards */
#define AC3_CUSTOMER_BOARD_ID_BASE      0x20
#define AC3_CUSTOMER_BOARD_ID0          (AC3_CUSTOMER_BOARD_ID_BASE + 0)
#define AC3_CUSTOMER_BOARD_ID1          (AC3_CUSTOMER_BOARD_ID_BASE + 1)
#define AC3_CUSTOMER_MAX_BOARD_ID       (AC3_CUSTOMER_BOARD_ID_BASE + 2)
#define AC3_CUSTOMER_BOARD_NUM          (AC3_CUSTOMER_MAX_BOARD_ID - AC3_CUSTOMER_BOARD_ID_BASE)

/* AlleyCat3 Marvell boards */
#define AC3_MARVELL_BOARD_ID_BASE       0x30
#define DB_AC3_ID                                       (AC3_MARVELL_BOARD_ID_BASE + 0)
#define AC3_MARVELL_MAX_BOARD_ID        (AC3_MARVELL_BOARD_ID_BASE + 1)
#define AC3_MARVELL_BOARD_NUM           (AC3_MARVELL_MAX_BOARD_ID - AC3_MARVELL_BOARD_ID_BASE)

#define PRV_MAX_PP_DEVICES	10

/* Switch registers & reg values */
#define PP_UDID				(0x00000204)		/* Unit default ID reg */
#define PP_WIN_BA(n)			(0x0000020c + (8*n))	/* base address reg */
#define PP_WIN_SR(n)			(0x00000210 + (8*n))	/* base window size reg */
#define PP_WIN_CTRL(n)			(0x00000254 + (4*n))	/* window control reg */

#define PP_ATTR				0x10
#define PP_UDID_DATTR			(PP_ATTR << 4)
#define PP_BA_ATTR			(PP_ATTR << 8)
#define PP_WIN_MAX_SIZE			0xFFFF
#define PP_WIN_SIZE_OFF			16
#define PP_WIN_SIZE_VAL			(PP_WIN_MAX_SIZE << PP_WIN_SIZE_OFF)
#define PP_WIN_CTRL_RW			0x3
#define PP_WIN_CTRL_AP			(PP_WIN_CTRL_RW << 1)

#define IRQ_AURORA_SW_CORE0		33

struct intData {
	unsigned long		intVec;		/* The interrupt vector we bind to */
	struct semaphore	sem;		/* The semaphore on which the user wait for */
	struct tasklet_struct	*tasklet;	/* The tasklet - need it for cleanup */
};

struct prestera_device {
	struct semaphore sem;		/* Mutual exclusion semaphore */
	loff_t size;			/* prestera mem size */
};

struct mem_region {
	mv_phys_addr_t	phys;
	unsigned long	allocbase;
	unsigned long	size;
	unsigned long	allocsize;
	uintptr_t	base;
	uintptr_t	mmapbase;
	uintptr_t	mmapsize;
	size_t		mmapoffset;
};

struct Mmap_Info_stc {
	enum {
		MMAP_INFO_TYPE_DMA_E,
		MMAP_INFO_TYPE_PP_CONF_E,
		MMAP_INFO_TYPE_PP_REGS_E,
		MMAP_INFO_TYPE_PP_DFX_E
	}			map_type;
	int			index;
	uintptr_t	addr;
	size_t		length;
	size_t		offset;
};

#ifdef PRESTERA_PP_DRIVER
typedef int (*PP_DRIVER_FUNC)(void *drv, void *io);
#endif
struct pp_dev {
	unsigned short		devId;
	unsigned short		vendorId;
	unsigned short		on_pci_bus;
	unsigned long		instance;
	unsigned long		busNo;
	unsigned long		devSel;
	unsigned long		funcNo;
	struct mem_region	config;		/* Configuration space */
	struct mem_region	ppregs;		/* PP registers space */
	struct intData		irq_data;
	struct mem_region	dfx;        /* DFX space */

#ifdef PRESTERA_PP_DRIVER
	PP_DRIVER_FUNC ppdriver;
	int            ppdriverType;
	void          *ppdriverData;
#endif
};
#ifdef PRESTERA_PP_DRIVER
int presteraPpDriverPciPexCreate(struct pp_dev *dev);
int presteraPpDriverPciPexHalfCreate(struct pp_dev *dev);
int presteraPpDriverPexMbusCreate(struct pp_dev *dev);
#endif

/* Register offset definition struct */
struct prvPciDeviceQuirks {
	unsigned int	pciId;
	unsigned int	isPex;
	unsigned int	configOffset;
	unsigned int	hasDfx;
	unsigned int	interruptMap[8];
};

int prestera_init(void);
int prestera_global_init(void);
int ppdev_conf_set(struct pci_dev *pdev, struct pp_dev *ppdev);
unsigned int mvDevIdGet(void);
unsigned int get_founddev(void);
extern unsigned long		dma_base;

static inline uint32_t read_u32(uintptr_t addr)
{
	uint32_t data;
	data = __raw_readl(addr); /* also converted from LE */
	return le32_to_cpu(data);
}

static inline void  write_u32(uint32_t data, uintptr_t addr)
{
	data = cpu_to_le32(data);
	__raw_writel(data, addr);
}

#endif /* __MV_PRESTERA */
