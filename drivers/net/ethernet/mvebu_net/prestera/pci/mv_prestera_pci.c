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
* mv_prestera_pci.c
*
* DESCRIPTION:
*	functions in kernel mode special for pci prestera.
*
* DEPENDENCIES:
*
*******************************************************************************/
#include <linux/pci.h>
#include <linux/module.h>
#include <linux/mm.h>
#include <linux/slab.h>
#include <linux/semaphore.h>
#include <linux/platform_device.h>
#include <pci/pci.h>
#include "mv_prestera_pci.h"
#include "mv_sysmap_pci.h"

#ifdef CONFIG_OF
#include "mach/mvCommon.h"
#else
#include "common/mvCommon.h"
#endif

#ifdef CONFIG_MV_INCLUDE_SERVICECPU
#include "mv_servicecpu/servicecpu.h"
#endif
#ifdef CONFIG_MV_INCLUDE_DRAGONITE_XCAT
#include "mv_drivers_lsp/mv_dragonite/dragonite_xcat.h"
#endif

/* Macro definitions */
#undef MV_PP_PCI_DBG
#undef MV_PP_IDT_DBG

#ifdef MV_PP_PCI_DBG
#define dprintk(a...) printk(a)
#else
#define dprintk(a...)
#endif

#define SWITCH_TARGET_ID		0x3
#define DFX_TARGET_ID			0x8
#define RAM_TARGET_ID			0x0
#define SHARED_RAM_ATTR_ID		0x3E
#define DRAGONITE_TARGET_ID		0xa

/* DRAGONTIE */
#define DRAGONITE_CTRL_REG			0x1c
#define DRAGONITE_POE_CAUSE_IRQ_REG		0x64
#define DRAGONITE_POE_MASK_IRQ_REG		0x68
#define DRAGONITE_HOST2POE_IRQ_REG		0x6c
#define DRAGONITE_DEBUGGER_REG			0xF8290

#ifdef MV_PP_IDT_DBG
#define idtprintk(a...) printk(a)
#else
#define idtprintk(a...)
#endif

/* Global variables */
static const char prestera_drv_name[] = "mvPP_PCI";
static void __iomem *inter_regs;
static struct idtSwitchConfig idtSwCfg[MAX_NUM_OF_IDT_SWITCH];

static struct pci_decoding_window ac3_pci_sysmap[] = {

	/*win_num	bar_num			offset		size(n*64KB)	remap
	 *				target_id			attr	status*/
	/* BAR 1*/
	{0,		PXWCR_WIN_BAR_MAP_BAR1,	0x0,		_64M,		0x0,
					SWITCH_TARGET_ID,		0x0,	ENABLE},

	/* BAR 2*/
	{DFXW,		PXWCR_WIN_BAR_MAP_BAR2,	DFX_BASE,	DFX_SIZE,	0x0,
					DFX_TARGET_ID,			0x0,	ENABLE},

	{SCPUW,		PXWCR_WIN_BAR_MAP_BAR2,	SCPU_BASE,	SCPU_SIZE,	0xFFF80000,
					RAM_TARGET_ID,	SHARED_RAM_ATTR_ID,	ENABLE},

	{DITCMW,	PXWCR_WIN_BAR_MAP_BAR2, ITCM_BASE,	ITCM_SIZE,	0x0,
					DRAGONITE_TARGET_ID,		0x0,	ENABLE},

	{DDTCMW,	PXWCR_WIN_BAR_MAP_BAR2,	DTCM_BASE,	DTCM_SIZE,	DRAGONITE_DTCM_OFFSET,
					DRAGONITE_TARGET_ID,		0x0,	ENABLE},

	{0xff,		PXWCR_WIN_BAR_MAP_BAR2,	0x0,		0,		0x0,
					0,				0x0,	TBL_TERM},
};

static struct pci_decoding_window bc2_pci_sysmap[] = {

	/*win_num	bar_num			offset		size(n*64KB)	remap
	 *				target_id			attr	status*/
	/* BAR 1*/
	{0,		PXWCR_WIN_BAR_MAP_BAR1,	0x0,		_64M,		0x0,
					SWITCH_TARGET_ID,		0x0,	ENABLE},

	/* BAR 2*/
	{DFXW,		PXWCR_WIN_BAR_MAP_BAR2,	DFX_BASE,	DFX_SIZE,	0x0,
					DFX_TARGET_ID,			0x0,	ENABLE},

	{SCPUW,		PXWCR_WIN_BAR_MAP_BAR2,	SCPU_BASE,	SCPU_SIZE,	0xFFF80000,
					RAM_TARGET_ID,	SHARED_RAM_ATTR_ID,	ENABLE},

	{0xff,		PXWCR_WIN_BAR_MAP_BAR2,	0x0,		0,		0x0,
					0,				0x0,	TBL_TERM},
};


static struct pci_decoding_window *get_pci_sysmap(struct pci_dev *pdev)
{
	switch (pdev->device) {

	case MV_BOBCAT2_DEV_ID:
		return bc2_pci_sysmap;
	case MV_ALLEYCAT3_DEV_ID:
		return ac3_pci_sysmap;
	default:
		return NULL;
	}
}

/*******************************************************************************
*	mv_set_pex_bars
*
*
*******************************************************************************/
static void mv_set_pex_bars(uint8_t pex_nr, uint8_t bar_nr, int enable)
{
	int val;

	dprintk("%s: pex_nr %d, bar_nr %d, enable:%d\n", __func__, pex_nr,
		bar_nr, enable);

	val = readl(inter_regs + PEX_BAR_CTRL_REG(pex_nr, bar_nr));

	if (enable == ENABLE)
		writel(val | PXBCR_BAR_EN,
			inter_regs + PEX_BAR_CTRL_REG(pex_nr, bar_nr));
	else
		writel(val & ~(PXBCR_BAR_EN),
			inter_regs + PEX_BAR_CTRL_REG(pex_nr, bar_nr));
}

/*******************************************************************************
*	mv_resize_bar
*
*
*******************************************************************************/
static void mv_resize_bar(uint8_t pex_nr, uint8_t bar_nr, uint32_t bar_size)
{
	/* Disable BAR before reconfiguration */
	mv_set_pex_bars(pex_nr, bar_nr, DISABLE);

	/* Resize */
	writel(bar_size, inter_regs + PEX_BAR_CTRL_REG(pex_nr, bar_nr));
	dprintk("PEX_BAR_CTRL_REG(%d, %d) = 0x%x\n", pex_nr, bar_nr,
		readl(inter_regs + PEX_BAR_CTRL_REG(pex_nr, bar_nr)));

	/* Enable BAR */
	mv_set_pex_bars(pex_nr, bar_nr, ENABLE);
}

/*******************************************************************************
*	mv_read_and_assign_bars
*
*
*******************************************************************************/
static int mv_read_and_assign_bars(struct pci_dev *pdev, int resno)
{
	struct resource *res = pdev->resource + resno;
	int reg, err;

	dprintk("before reassign: r_start 0x%x, r_end: 0x%x, r_flags 0x%lx\n",
		res->start, res->end, res->flags);

	reg = PCI_BASE_ADDRESS_0 + (resno << 2);
	__pci_read_base(pdev, pci_bar_unknown, res, reg);
	err = pci_assign_resource(pdev, resno);

	dprintk("after reassign: r_start 0x%x, r_end: 0x%x, r_flags 0x%lx\n",
		res->start, res->end, res->flags);

	return err;
}

/*******************************************************************************
*	mv_configure_win_bar
*
*
*******************************************************************************/
static int mv_configure_win_bar(struct pci_decoding_window *win_map, struct pci_dev *pdev)
{
	uint8_t target, bar_nr;
	int io_base_bar[2], base_addr, val, win_ctrl_reg, win_base_reg, win_remap_reg;

	io_base_bar[0] = pci_resource_start(pdev, MV_PCI_BAR_1);
	io_base_bar[1] = pci_resource_start(pdev, MV_PCI_BAR_2);

	for (target = 0; win_map[target].enable != TBL_TERM; target++) {
		if (win_map[target].enable != ENABLE)
			continue;

		val = (SIZE_TO_BAR_REG(win_map[target].size) |
		      (win_map[target].target_id << PXWCR_TARGET_OFFS) |
		      (win_map[target].attr << PXWCR_ATTRIB_OFFS) |
		      win_map[target].win_bar_map | PXWCR_WIN_EN);

		dprintk("targ size 0x%x, size reg 0x%x, val 0x%x\n", win_map[target].size,
			SIZE_TO_BAR_REG(win_map[target].size), val);

		bar_nr = win_map[target].win_bar_map >> PXWCR_WIN_BAR_MAP_OFFS;
		base_addr = io_base_bar[bar_nr] + win_map[target].base_offset;

		switch (win_map[target].win_num) {
		case 0:
		case 1:
		case 2:
		case 3:
			win_ctrl_reg = PEX_WIN0_3_CTRL_REG(PEX_0, win_map[target].win_num);
			win_base_reg = PEX_WIN0_3_BASE_REG(PEX_0, win_map[target].win_num);
			win_remap_reg = PEX_WIN0_3_REMAP_REG(PEX_0, win_map[target].win_num);
			break;
		case 4:
		case 5:
			win_ctrl_reg = PEX_WIN4_5_CTRL_REG(PEX_0, win_map[target].win_num);
			win_base_reg = PEX_WIN4_5_BASE_REG(PEX_0, win_map[target].win_num);
			win_remap_reg = PEX_WIN4_5_REMAP_REG(PEX_0, win_map[target].win_num);
			break;
		default:
			dev_err(&pdev->dev, "Not supported decoding window\n");
			return -ENODEV;
		}

		dprintk("pex_win %d for bar%d = 0x%x\n", win_map[target].win_num,
			bar_nr+1, readl(inter_regs + win_ctrl_reg));

		writel(base_addr, inter_regs + win_base_reg);
		writel(win_map[target].remap | PXWRR_REMAP_EN, inter_regs + win_remap_reg);
		writel(val, inter_regs + win_ctrl_reg);

		dprintk("BAR%d: pex_win_ctrl = 0x%x, pex_win_base 0 = 0x%x\n",
			bar_nr+1, readl(inter_regs + win_ctrl_reg), readl(inter_regs + win_base_reg));
	}

	return 0;
}

static void mv_release_pci_resources(struct pci_dev *pdev)
{
	int i;

	/* Release all resources which were assigned */
	for (i = 0; i < PCI_STD_RESOURCE_END; i++) {
		struct resource *res = pdev->resource + i;
		if (res->parent)
			release_resource(res);
	}
}

static int mv_calc_bar_size(struct pci_decoding_window *win_map, uint8_t bar)
{
	uint8_t target, bar_nr;
	int size = 0;

	for (target = 0; win_map[target].enable != TBL_TERM; target++) {
		if (win_map[target].enable != ENABLE)
			continue;

		bar_nr = (win_map[target].win_bar_map >> PXWCR_WIN_BAR_MAP_OFFS) + 1;
		if (bar_nr != bar)
			continue;

		size += win_map[target].size;
	}

	/* Round up to next power of 2 if needed */
	if (!MV_IS_POWER_OF_2(size))
		size = (1 << (mvLog2(size) + 1));

	dprintk("%s: calculated bar size %d\n", __func__, size);
	return size;

}

/*******************************************************************************
*	mv_reconfig_bars
*
*	PCI device BAR Re-configuration
*
*******************************************************************************/
static int mv_reconfig_bars(struct pci_dev *pdev, struct pci_decoding_window *prestera_sysmap_bar)
{
	int i, err, size;

	/* For some configurations BAR1 or BAR2 occupied whole pci address space
	 * and BAR0 which is needed to reconfigure other bars is not assigned
	 * and therefore unreachable. If BAR0 is not assigned, assigned it to
	 * be able to resize BAR1 and BAR2
	 */
	if (!pdev->resource->parent) {
		mv_release_pci_resources(pdev);
		err = mv_read_and_assign_bars(pdev, MV_PCI_BAR_INTER_REGS);
		if (err != 0)
			return err;
	}

	inter_regs = pci_iomap(pdev, MV_PCI_BAR_INTER_REGS, _1M);
	if (inter_regs == NULL) {
		dev_err(&pdev->dev, "failed to remap registers\n");
		return -ENODEV;
	}
	dprintk("%s: inter_regs 0x%p\n", __func__, inter_regs);

	/* Resize BAR1 (64MB for SWITCH) */
	size = mv_calc_bar_size(prestera_sysmap_bar, BAR_1);
	mv_resize_bar(PEX_0, BAR_1, SIZE_TO_BAR_REG(size));

	/* Resize BAR2 (1MB for DFX and Dragonite) */
	size = mv_calc_bar_size(prestera_sysmap_bar, BAR_2);
	mv_resize_bar(PEX_0, BAR_2, SIZE_TO_BAR_REG(size));

	/* Unmap inter_regs - will be mapped again after BAR reassignment */
	iounmap(inter_regs);

	mv_release_pci_resources(pdev);

	/*
	 * Now when all PCI BARs are reconfigured, read them again and reassign
	 * resources
	 */
	err = mv_read_and_assign_bars(pdev, MV_PCI_BAR_1);
	if (err != 0)
		return err;

	err = mv_read_and_assign_bars(pdev, MV_PCI_BAR_2);
	if (err != 0)
		return err;

	err = mv_read_and_assign_bars(pdev, MV_PCI_BAR_INTER_REGS);

	inter_regs = pci_iomap(pdev, MV_PCI_BAR_INTER_REGS, _1M);
	if (inter_regs == NULL) {
		dev_err(&pdev->dev, "failed to remap registers\n");
		return -ENODEV;
	}
	dprintk("%s: inter_regs 0x%p\n", __func__, inter_regs);

	/* Disable all windows which point to BAR1 and BAR2 */
	for (i = 0; i < 6; i++) {
		if (i < 4) {
			writel(0, inter_regs + PEX_WIN0_3_CTRL_REG(PEX_0, i));
			writel(0, inter_regs + PEX_WIN0_3_BASE_REG(PEX_0, i));
			writel(0, inter_regs + PEX_WIN0_3_REMAP_REG(PEX_0, i));
		} else {
			writel(0, inter_regs + PEX_WIN4_5_CTRL_REG(PEX_0, i));
			writel(0, inter_regs + PEX_WIN4_5_BASE_REG(PEX_0, i));
			writel(0, inter_regs + PEX_WIN4_5_REMAP_REG(PEX_0, i));
		}
	}

	err = mv_configure_win_bar(prestera_sysmap_bar, pdev);
	if (err)
		return err;

	dprintk("%s: decoding win for BAR1 and BAR2 configured\n", __func__);

	return err;
}

/*******************************************************************************
*	mv_calc_device_address_range
*
*	Scan device bars and calc address range
*
*******************************************************************************/
static void mv_calc_device_address_range(struct pci_dev *dev,  unsigned int devInstance, int index)
{
	unsigned int i = 0;
	unsigned long startAddr, endAddr;
	unsigned long tempStartAddr, tempEndAddr;

	startAddr = pci_resource_start(dev, i);
	endAddr = pci_resource_end(dev, i);

	for (i = 2; i < PCI_STD_RESOURCE_END; i += 2) {
		tempStartAddr = pci_resource_start(dev, i);
		tempEndAddr = pci_resource_end(dev, i);

		if ((tempStartAddr != 0) && (tempEndAddr != 0)) {
			if (tempStartAddr < startAddr)
				startAddr = tempStartAddr;
			if (tempEndAddr > endAddr)
				endAddr = tempEndAddr;
		}
	}

	idtSwCfg[index].idtSwPortCfg.startAddr[devInstance] = startAddr;
	idtSwCfg[index].idtSwPortCfg.endAddr[devInstance] = endAddr;
}

/*******************************************************************************
*	mv_find_pci_dev_pp_instances
*
*	Scan for PCI devices
*	Search for first instance if IDT Switch - defined as Uplink Bus
*	Search for second instance if IDT Switch - defined as Downlink Bus
*	Search for Marvell devices - count and save for further processing
*
*******************************************************************************/
static void mv_find_pci_dev_pp_instances(void)
{
	struct pci_dev *dev = NULL;
	int index;

	memset(&(idtSwCfg[0]), 0, ((sizeof(struct idtSwitchConfig)) * MAX_NUM_OF_IDT_SWITCH));
	for (index = 0; index < MAX_NUM_OF_IDT_SWITCH; index++) {
		idtSwCfg[index].ppIdtSwitchUsBusNum = 0xFF;
		idtSwCfg[index].ppIdtSwitchDsBusNum = 0xFF;
	}

	while ((dev = pci_get_device(PCI_ANY_ID, PCI_ANY_ID, dev)) != NULL) {
		if (dev->vendor == PCI_VENDOR_ID_IDT_SWITCH) {
			for (index = 0; index < MAX_NUM_OF_IDT_SWITCH; index++) {
				if (idtSwCfg[index].ppIdtSwitchUsBusNum == 0xFF) {
					idtSwCfg[index].ppIdtSwitchUsBusNum = dev->bus->number;
					idtprintk("idt -%d- us bus number %d\n", index, dev->bus->number);
					break;
				}

				if (idtSwCfg[index].ppIdtSwitchDsBusNum == 0xFF) {
					idtSwCfg[index].ppIdtSwitchDsBusNum = dev->bus->number;
					idtprintk("idt -%d- ds bus number %d\n", index, dev->bus->number);
					break;
				}

				if ((idtSwCfg[index].ppIdtSwitchUsBusNum == dev->bus->number) ||
					(idtSwCfg[index].ppIdtSwitchDsBusNum == dev->bus->number))
					break;
			}
		} else if (dev->vendor == MARVELL_VEN_ID) {
			if (dev->bus->parent != NULL) {
				idtprintk("mrvl dev, bus number %d, parent %d\n",
					dev->bus->number,  dev->bus->parent->number);

				for (index = 0; index < MAX_NUM_OF_IDT_SWITCH; index++)
					if (idtSwCfg[index].ppIdtSwitchDsBusNum == dev->bus->parent->number)
						break;

				mv_calc_device_address_range(dev, idtSwCfg[index].numOfPpInstances, index);
				idtSwCfg[index].idtSwPortCfg.ppBusNumArray[idtSwCfg[index].numOfPpInstances] =
												dev->bus->number;
				idtSwCfg[index].numOfPpInstances++;
			}
		}
	}
}

/*******************************************************************************
*	mv_discover_active_pp_instances
*
*	Locate the active downlink ports connected to IDT switch
*	The idt switch downlink ports are 2 - (2 + ppInstance).
*	ports 0 - 1 are for the IDT switch, define which idt port are connected to the pps
*
*******************************************************************************/
static void mv_discover_active_pp_instances(void)
{
	int i, num;
	struct pci_dev *dev = NULL;
	unsigned short pciLinkStatusRegVal;
	int index;

	for (index = 0; index < MAX_NUM_OF_IDT_SWITCH; index++) {
		for (num = 0, i = 0; num < idtSwCfg[index].numOfPpInstances; i++) {
			dev = pci_get_bus_and_slot(idtSwCfg[index].ppIdtSwitchDsBusNum, PCI_DEVFN(i + 2, 0));
			if (dev != NULL) {
				pci_read_config_word(dev, (int)MV_IDT_SWITCH_PCI_LINK_STATUS_REG, &pciLinkStatusRegVal);
				if (pciLinkStatusRegVal & MV_IDT_SWITCH_PCI_LINK_STATUS_REG_ACTIVE_LINK)
					idtSwCfg[index].idtSwPortCfg.ppIdtSwitchDsPpDevices[num++] = i + 2;
			}
		}
	}
}

/*******************************************************************************
*	mv_configure_idt_switch_addr_range
*
*
*******************************************************************************/
static void mv_configure_idt_switch_addr_range(void)
{
	int i;
	struct pci_dev *dev = NULL;
	int index;

	for (index = 0; index < MAX_NUM_OF_IDT_SWITCH; index++) {
		for (i = 0; i < idtSwCfg[index].numOfPpInstances; i++) {
			dev = pci_get_bus_and_slot(idtSwCfg[index].ppIdtSwitchDsBusNum,
						  PCI_DEVFN(idtSwCfg[index].idtSwPortCfg.ppIdtSwitchDsPpDevices[i], 0));
			if (dev != NULL) {
				pci_write_config_dword(dev, MV_IDT_SWITCH_PCI_MEM_BASE_REG,
							((idtSwCfg[index].idtSwPortCfg.endAddr[i] &
							MV_IDT_SWITCH_PCI_MEM_BASE_REG_MASK) |
							(((idtSwCfg[index].idtSwPortCfg.startAddr[i]) &
							MV_IDT_SWITCH_PCI_MEM_BASE_REG_MASK) >> 16)));
			}
		}

		if (idtSwCfg[index].numOfPpInstances > 0) {
			dev = pci_get_bus_and_slot(idtSwCfg[index].ppIdtSwitchUsBusNum, PCI_DEVFN(0, 0));
			pci_write_config_dword(dev, MV_IDT_SWITCH_PCI_MEM_BASE_REG,
				((idtSwCfg[index].idtSwPortCfg.endAddr[idtSwCfg[index].numOfPpInstances - 1] &
				MV_IDT_SWITCH_PCI_MEM_BASE_REG_MASK) |
				(((idtSwCfg[index].idtSwPortCfg.startAddr[0]) &
				MV_IDT_SWITCH_PCI_MEM_BASE_REG_MASK) >> MV_IDT_SWITCH_PCI_MEM_BASE_REG_SHIFT)));
		}
	}
}

/*******************************************************************************
*	mv_print_idt_switch_configuration
*
*
*******************************************************************************/
static void mv_print_idt_switch_configuration(void)
{
	int i;
	unsigned int pciBaseAddr;
	struct pci_dev *dev = NULL;
	int index;

	for (index = 0; index < MAX_NUM_OF_IDT_SWITCH; index++) {
		if (idtSwCfg[index].numOfPpInstances > 0) {
			idtprintk("\n");
			idtprintk("PEX IDT Switch Configuration\n");
			idtprintk("============================\n");
			idtprintk("                  %02x:00.0 (IDT Uplink port)\n",
				idtSwCfg[index].ppIdtSwitchUsBusNum);
			idtprintk("                     |\n");
			idtprintk("---------------------|---------------------\n");
			idtprintk("%02x:%02x.0     %02x:%02x.0     %02x:%02x.0     %02x:%02x.0 (IDT Downlink ports)\n",
				idtSwCfg[index].ppIdtSwitchDsBusNum,
				idtSwCfg[index].idtSwPortCfg.ppIdtSwitchDsPpDevices[0],
				idtSwCfg[index].ppIdtSwitchDsBusNum,
				idtSwCfg[index].idtSwPortCfg.ppIdtSwitchDsPpDevices[1],
				idtSwCfg[index].ppIdtSwitchDsBusNum,
				idtSwCfg[index].idtSwPortCfg.ppIdtSwitchDsPpDevices[2],
				idtSwCfg[index].ppIdtSwitchDsBusNum,
				idtSwCfg[index].idtSwPortCfg.ppIdtSwitchDsPpDevices[3]);
			idtprintk("%02x:00.0     %02x:00.0     %02x:00.0     %02x:00.0 (Marvell Devices)\n",
				idtSwCfg[index].idtSwPortCfg.ppBusNumArray[0],
				idtSwCfg[index].idtSwPortCfg.ppBusNumArray[1],
				idtSwCfg[index].idtSwPortCfg.ppBusNumArray[2],
				idtSwCfg[index].idtSwPortCfg.ppBusNumArray[3]);
			idtprintk("\n");
		}
	}

	for (index = 0; index < MAX_NUM_OF_IDT_SWITCH; index++) {
		for (i = 0; i < idtSwCfg[index].numOfPpInstances; i++) {
			dev = pci_get_bus_and_slot(idtSwCfg[index].ppIdtSwitchDsBusNum,
				PCI_DEVFN(idtSwCfg[index].idtSwPortCfg.ppIdtSwitchDsPpDevices[i], 0));
			if (dev != NULL) {
				pci_read_config_dword(dev, (int)MV_IDT_SWITCH_PCI_MEM_BASE_REG, &pciBaseAddr);
				idtprintk("PEX IDT Downlink port [bus 0x%x device 0x%x] Base address 0x%08x\n",
					idtSwCfg[index].ppIdtSwitchDsBusNum,
					idtSwCfg[index].idtSwPortCfg.ppIdtSwitchDsPpDevices[i], pciBaseAddr);
			}
		}

		if (idtSwCfg[index].numOfPpInstances > 0) {
			dev = pci_get_bus_and_slot(idtSwCfg[index].ppIdtSwitchUsBusNum, PCI_DEVFN(0, 0));
			pci_read_config_dword(dev, (int)MV_IDT_SWITCH_PCI_MEM_BASE_REG, &pciBaseAddr);
			idtprintk("PEX IDT Uplink port   [bus 0x%x device 0x%x] Base address 0x%08x\n\n",
				idtSwCfg[index].ppIdtSwitchUsBusNum, 0, pciBaseAddr);
		}
	}
	idtprintk("\n");
}

/*******************************************************************************
*	mv_reconfig_idt_switch
*
*
*******************************************************************************/
void mv_reconfig_idt_switch(void)
{
	/* Scan for PCI devices - Marvell & IDT Switch */
	mv_find_pci_dev_pp_instances();

	/* Discover Marvell devices connected to IDT Switch */
	mv_discover_active_pp_instances();

	/* Configure IDT Switch address range */
	mv_configure_idt_switch_addr_range();

	/* Print configuration */
	mv_print_idt_switch_configuration();
}

#ifdef CONFIG_MV_INCLUDE_DRAGONITE_XCAT
int __init mv_msys_dragonite_init(struct pci_dev *pdev, struct pci_decoding_window *prestera_sysmap_bar)
{
	void __iomem * const *iomap = NULL;
	void __iomem *switch_reg, *dfx_reg;
	struct resource *dragonite_resources;
	struct dragonite_info *dragonite_pci_data;
	struct platform_device *mv_dragonite_dev;
	const int msys_interregs_phys_base =
				pci_resource_start(pdev, MV_PCI_BAR_INTER_REGS);
	const int itcm_phys = pci_resource_start(pdev, MV_PCI_BAR_2) +
					prestera_sysmap_bar[DITCMW].base_offset;
	const int dtcm_phys = pci_resource_start(pdev, MV_PCI_BAR_2) +
					prestera_sysmap_bar[DDTCMW].base_offset;

	dragonite_resources = devm_kzalloc(&pdev->dev, 4 * sizeof(struct resource), GFP_KERNEL);
	if (!dragonite_resources)
		return -ENOMEM;

	dragonite_pci_data = devm_kzalloc(&pdev->dev, sizeof(struct dragonite_info), GFP_KERNEL);
	if (!dragonite_pci_data)
		return -ENOMEM;

	mv_dragonite_dev =  devm_kzalloc(&pdev->dev, sizeof(struct platform_device), GFP_KERNEL);
	if (!mv_dragonite_dev)
		return -ENOMEM;

	dev_dbg(&pdev->dev, "Dragonite init...\n");

	iomap = pcim_iomap_table(pdev);
	inter_regs = iomap[MV_PCI_BAR_INTER_REGS];
	switch_reg = iomap[MV_PCI_BAR_1];
	dfx_reg = iomap[MV_PCI_BAR_2];

	dragonite_resources[0].start	= itcm_phys;
	dragonite_resources[0].end	= itcm_phys + ITCM_SIZE - 1;
	dragonite_resources[0].flags	= IORESOURCE_MEM;

	dragonite_resources[1].start	= dtcm_phys;
	dragonite_resources[1].end	= dtcm_phys + DTCM_SIZE - 1;
	dragonite_resources[1].flags	= IORESOURCE_MEM;

	dragonite_resources[2].start	= msys_interregs_phys_base;
	dragonite_resources[2].end	= msys_interregs_phys_base + _1M - 1;
	dragonite_resources[2].flags	= IORESOURCE_MEM;

	dragonite_resources[3].start	= pdev->irq;
	dragonite_resources[3].end	= pdev->irq;
	dragonite_resources[3].flags	= IORESOURCE_IRQ;

	dragonite_pci_data->ctrl_reg	= (void *)(switch_reg + DRAGONITE_CTRL_REG);
	dragonite_pci_data->jtag_reg	= (void *)(dfx_reg + DRAGONITE_DEBUGGER_REG);
	dragonite_pci_data->poe_cause_irq_reg = (void *)(switch_reg + DRAGONITE_POE_CAUSE_IRQ_REG);
	dragonite_pci_data->poe_mask_irq_reg = (void *)(switch_reg + DRAGONITE_POE_MASK_IRQ_REG);
	dragonite_pci_data->host2poe_irq_reg = (void *)(switch_reg + DRAGONITE_HOST2POE_IRQ_REG);

	mv_dragonite_dev->name		= "dragonite_xcat";
	mv_dragonite_dev->id		= -1;
	mv_dragonite_dev->dev.platform_data = dragonite_pci_data;
	mv_dragonite_dev->num_resources	= 4;
	mv_dragonite_dev->resource	= dragonite_resources;

	platform_device_register(mv_dragonite_dev);

	return 0;
}
#endif

/*******************************************************************************
*	prestera_pci_probe
*
*	PCI device probe function
*
*******************************************************************************/
static int prestera_pci_probe(struct pci_dev *pdev, const struct pci_device_id *ent)
{
	int err;
	static int pexSwitchConfigure;
	void __iomem * const *iomap;
	struct pci_decoding_window *prestera_sysmap_bar;

	switch (pdev->device) {

	case MV_IDT_SWITCH_DEV_ID_808E:
	case MV_IDT_SWITCH_DEV_ID_802B:
		if (pexSwitchConfigure == 0) {
			mv_reconfig_idt_switch();
			pexSwitchConfigure++;
		}
		return 0;

	case MV_BOBCAT2_DEV_ID:
	case MV_ALLEYCAT3_DEV_ID:

		prestera_sysmap_bar = get_pci_sysmap(pdev);
		if (!prestera_sysmap_bar)
			return -ENXIO;

		err = pcim_enable_device(pdev);
		if (err)
			return err;

		/*
		* Reconfigure and reassign bars
		*  BAR0: 1MB  for INTER REGS (fixed size, no configuration needed)
		*  BAR1: 64MB for SWITCH REGS
		*  BAR2: 1MB  for DFX REGS
		*/
		err = mv_reconfig_bars(pdev, prestera_sysmap_bar);
		if (err != 0)
			return err;

		err = pcim_iomap_regions(pdev, (1 << MV_PCI_BAR_INTER_REGS) |
					(1 << MV_PCI_BAR_1) |
					(1 << MV_PCI_BAR_2), prestera_drv_name);
		if (err)
			return err;
#ifdef CONFIG_MV_INCLUDE_DRAGONITE_XCAT
		if (pdev->device == MV_ALLEYCAT3_DEV_ID)
			err = mv_msys_dragonite_init(pdev, prestera_sysmap_bar);
			if (err)
				return err;
#endif

#ifdef CONFIG_MV_INCLUDE_SERVICECPU
		iomap = pcim_iomap_table(pdev);
		servicecpu_data.inter_regs_base = iomap[MV_PCI_BAR_INTER_REGS];
		servicecpu_data.pci_win_size = prestera_sysmap_bar[SCPUW].size;
		servicecpu_data.pci_win_virt_base =
			iomap[MV_PCI_BAR_2] + prestera_sysmap_bar[SCPUW].base_offset;
		servicecpu_data.pci_win_phys_base = (void *)(pci_resource_start(pdev,
			MV_PCI_BAR_2) + prestera_sysmap_bar[SCPUW].base_offset);
		servicecpu_init();
#endif
		break;

	case MV_LION2_DEV_ID:

		err = pcim_enable_device(pdev);
		if (err)
			return err;

		err = pcim_iomap_regions(pdev, ((1 << MV_PCI_BAR_INTER_REGS) |
						(1 << MV_PCI_BAR_1)),
						prestera_drv_name);
		if (err)
			return err;

		break;

	default:
		 dprintk("%s: unsupported device\n", __func__);
	}

	dev_info(&pdev->dev, "%s init completed\n", prestera_drv_name);
	return 0;
}

static void prestera_pci_remove(struct pci_dev *pdev)
{
	dprintk("%s\n", __func__);

#ifdef CONFIG_MV_INCLUDE_SERVICECPU
		servicecpu_deinit();
#endif
}

static DEFINE_PCI_DEVICE_TABLE(prestera_pci_tbl) = {
	/* Marvell */
	{ PCI_DEVICE(PCI_VENDOR_ID_IDT_SWITCH, MV_IDT_SWITCH_DEV_ID_808E)},
	{ PCI_DEVICE(PCI_VENDOR_ID_IDT_SWITCH, MV_IDT_SWITCH_DEV_ID_802B)},
	{ PCI_DEVICE(PCI_VENDOR_ID_MARVELL, MV_BOBCAT2_DEV_ID)},
	{ PCI_DEVICE(PCI_VENDOR_ID_MARVELL, MV_LION2_DEV_ID)},
	{ PCI_DEVICE(PCI_VENDOR_ID_MARVELL, MV_ALLEYCAT3_DEV_ID)},
	{}
};

static struct pci_driver prestera_pci_driver = {
	.name			= prestera_drv_name,
	.id_table		= prestera_pci_tbl,
	.probe			= prestera_pci_probe,
	.remove			= prestera_pci_remove,
};

static int __init prestera_pci_init(void)
{
	return pci_register_driver(&prestera_pci_driver);
}

static void __exit prestera_pci_cleanup(void)
{
	pci_unregister_driver(&prestera_pci_driver);
}

MODULE_AUTHOR("Grzegorz Jaszczyk <jaz@semihalf.com>");
MODULE_DESCRIPTION("pci device driver for Marvell Prestera family switches");
MODULE_LICENSE("GPL");

module_init(prestera_pci_init);
module_exit(prestera_pci_cleanup);
