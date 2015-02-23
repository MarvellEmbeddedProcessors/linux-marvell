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
* mv_prestera_pltfm.c
*
* DESCRIPTION:
*	functions in kernel mode special for prestera.
*
* DEPENDENCIES:
*
*******************************************************************************/
#include "mvOs.h"
#include "mv_prestera.h"
#include "mv_prestera_pci.h"
#include "mv_pss_api.h"

#ifdef CONFIG_OF
#include <linux/of.h>
#endif

#undef MV_PP_DBG

#ifdef MV_PP_DBG
#define dprintk(a...) printk(a)
#else
#define dprintk(a...)
#endif

#define	DRIVER_NAME	"prestera_device"

/* Switch attr and target id is different for PCI */
#define PP_PCI_ATTR			0xe
#define PP_PCI_TARGETID			0x4
#define PP_PCI_UDID_DATTR		(PP_PCI_ATTR << 4 | PP_PCI_TARGETID)
#define PP_PCI_BA_ATTR			(PP_PCI_ATTR << 8 | PP_PCI_TARGETID)


#define DFX_JTAG_DEVID_STAT		0xF8244

struct presteraPciDev {
	unsigned int deviceNum;
	unsigned int vendorId;
	unsigned int deviceId;
};

static struct presteraPciDev presteraPciDevs[] = {
	{0, PCI_VENDOR_ID_IDT_SWITCH, MV_IDT_SWITCH_DEV_ID_808E},
	{1, PCI_VENDOR_ID_IDT_SWITCH, MV_IDT_SWITCH_DEV_ID_802B},
	{2, PCI_VENDOR_ID_MARVELL, MV_BOBCAT2_DEV_ID},
	{3, PCI_VENDOR_ID_MARVELL, MV_LION2_DEV_ID},
	{4, PCI_VENDOR_ID_MARVELL, MV_ALLEYCAT3_DEV_ID},
	{-1, -1, -1}
};

static const char prestera_drv_name[] = "mvPP";
static void __iomem *inter_regs;
static int gDevId = -1;

/*******************************************************************************
********************************************************************************
********************************************************************************
***
***	Internal Device Configuration Section
***
********************************************************************************
********************************************************************************
*******************************************************************************/

/*******************************************************************************
*	mvInternalDeviceIdSet
*
*
*******************************************************************************/
void mvInternalDevIdSet(unsigned int devId)
{
	gDevId = devId;
}

/*******************************************************************************
*	mvDevIdGet
*
*
*******************************************************************************/
unsigned int mvDevIdGet(void)
{
	return gDevId;
}

#ifndef CONFIG_OF
/*******************************************************************************
*	ppdev_conf_set_pltfm
*
*
*******************************************************************************/
static int ppdev_conf_set_pltfm(void)
{
	struct pp_dev	*ppdev;
	unsigned long	start;
	unsigned long	len;
	int err;

	dprintk("%s\n", __func__);

	ppdev = kmalloc(sizeof(struct pp_dev), GFP_KERNEL);
	if (NULL == ppdev) {
		printk("kmalloc failed\n");
		return -ENOMEM;
	}
	memset(ppdev, 0, sizeof(*ppdev));

	ppdev->devId = mvDevIdGet();
	ppdev->vendorId = MARVELL_VEN_ID;
	ppdev->busNo = 0xFF;/* 0xFF represent internal device for CPSS */
	ppdev->devSel = 0xFF;
	ppdev->funcNo = 0xFF;
	ppdev->on_pci_bus = 0;
	ppdev->irq_data.intVec = IRQ_AURORA_SW_CORE0;

	/* configure the SWITCH register address space */
	/* additional 1MB is reserved for DFX registers */
	start = SWITCH_REGS_PHYS_BASE;
	len = SWITCH_REGS_SIZE + _1M;

	ppdev->ppregs.allocbase = start;
	ppdev->ppregs.allocsize = len;
	ppdev->ppregs.size = len;
	ppdev->ppregs.phys = start;
	ppdev->ppregs.base = (uintptr_t)(SWITCH_REGS_VIRT_BASE);

	/* configure the CPU register address space */
	start = INTER_REGS_PHYS_BASE;
	len = _1M;

	ppdev->config.allocbase = start;
	ppdev->config.allocsize = len;
	ppdev->config.size = len;
	ppdev->config.phys = start;
	ppdev->config.base = (uintptr_t)(INTER_REGS_VIRT_BASE);

	err = ppdev_conf_set(NULL, ppdev);
	if (err)
		return err;

	return 0;
}

/*******************************************************************************
*	prestera_dma_switch_init
*
*
*******************************************************************************/
static void prestera_dma_switch_init(void)
{
	/* open internal switch window for DMA */
	writel(dma_base | PP_BA_ATTR,	SWITCH_REGS_VIRT_BASE | PP_WIN_BA(0));
	writel(PP_UDID_DATTR,		SWITCH_REGS_VIRT_BASE | PP_UDID);
	writel(PP_WIN_SIZE_VAL,		SWITCH_REGS_VIRT_BASE | PP_WIN_SR(0));
	writel(PP_WIN_CTRL_AP,		SWITCH_REGS_VIRT_BASE | PP_WIN_CTRL(0));

	dprintk("%s read pp: 0x%x\n", __func__,
		readl(SWITCH_REGS_VIRT_BASE | PP_WIN_BA(0)));
	dprintk("%s read pp: 0x%x\n", __func__,
		readl(SWITCH_REGS_VIRT_BASE | PP_UDID));
	dprintk("%s read pp: 0x%x\n", __func__,
		readl(SWITCH_REGS_VIRT_BASE | PP_WIN_SR(0)));
	dprintk("%s read pp: 0x%x\n", __func__,
		readl(SWITCH_REGS_VIRT_BASE | PP_WIN_CTRL(0)));
}

/*******************************************************************************
*	prestera_Internal_dev_probe
*
*
*******************************************************************************/
static int prestera_Internal_dev_probe(unsigned int devId)
{
	int err;

	mvInternalDevIdSet(devId);

	/* The device flavor is ignored for AC3/BC2 */
	switch (devId & ~MV_DEV_FLAVOUR_MASK) {

	case MV_BOBCAT2_DEV_ID:
	case MV_ALLEYCAT3_DEV_ID:

		err = ppdev_conf_set_pltfm();
		if (0 != err)
			return err;

		err = prestera_init();
		if (err)
			return err;

		prestera_dma_switch_init();
		break;

	default:
		break;
	}

	printk(KERN_INFO "finish internal dev %x probe\n", devId);

	return 0;
}
#endif

/*******************************************************************************
********************************************************************************
********************************************************************************
***
***	PCI Device Configuration Section
***
********************************************************************************
********************************************************************************
*******************************************************************************/

/*******************************************************************************
*	mv_ppdev_conf_set_pci
*
*
*******************************************************************************/
static int mv_ppdev_conf_set_pci(struct pci_dev *pdev)
{
	struct pp_dev	*ppdev;
	unsigned long	start;
	unsigned long	len;
	void __iomem * const *iomap;
	int err;

	iomap = pcim_iomap_table(pdev);

	ppdev = kmalloc(sizeof(struct pp_dev), GFP_KERNEL);
	if (NULL == ppdev) {
		dev_err(&pdev->dev, "kmalloc ppdev failed\n");
		return -ENOMEM;
	}
	memset(ppdev, 0, sizeof(*ppdev));

	ppdev->devId = pdev->device;
	ppdev->vendorId = MARVELL_VEN_ID;
	ppdev->busNo = pdev->bus->number;
	ppdev->devSel = PCI_SLOT(pdev->devfn);
	ppdev->funcNo = PCI_FUNC(pdev->devfn);
	ppdev->on_pci_bus = 1;
	ppdev->irq_data.intVec = pdev->irq;

	/* Configure the SWITCH register address space */
	/* Additional 1MB is reserved for DFX registers - Bobcat2 / Alleycat3 */
	/* Lion2 does not have DFX */
	start = pci_resource_start(pdev, MV_PCI_BAR_1);
	if (pdev->device != MV_LION2_DEV_ID) /*  MV_BOBCAT2_DEV_ID / MV_ALLEYCAT3_DEV_ID */
		len =  pci_resource_len(pdev, MV_PCI_BAR_1) + _1M;
	else
		len =  pci_resource_len(pdev, MV_PCI_BAR_1);

	ppdev->ppregs.allocbase = start;
	ppdev->ppregs.allocsize = len;
	ppdev->ppregs.size = len;
	ppdev->ppregs.phys = start;
	ppdev->ppregs.base = (unsigned long)iomap[MV_PCI_BAR_1];

	/* Configure the CPU register address space */
	start = pci_resource_start(pdev, MV_PCI_BAR_INTER_REGS);
	len = pci_resource_len(pdev, MV_PCI_BAR_INTER_REGS);

	ppdev->config.allocbase = start;
	ppdev->config.allocsize = len;
	ppdev->config.size = len;
	ppdev->config.phys = start;
	ppdev->config.base = (unsigned long)iomap[MV_PCI_BAR_INTER_REGS];

	err = ppdev_conf_set(pdev, ppdev);
	if (err)
		return err;

	return 0;
}

/*******************************************************************************
*	mv_pci_dma_switch_init
*
*
*******************************************************************************/
static void mv_pci_dma_switch_init(unsigned long switch_reg, struct pci_dev *pdev)
{
	dprintk("%s\n", __func__);

	writel(dma_base | PP_PCI_BA_ATTR, switch_reg | PP_WIN_BA(0));
	writel(PP_PCI_UDID_DATTR,	switch_reg | PP_UDID);
	writel(PP_WIN_SIZE_VAL,		switch_reg | PP_WIN_SR(0));
	writel(PP_WIN_CTRL_AP,		switch_reg | PP_WIN_CTRL(0));

	dprintk("read pp: 0x%x\n", readl(switch_reg | PP_WIN_BA(0)));
	dprintk("read pp: 0x%x\n", readl(switch_reg | PP_UDID));
	dprintk("read pp: 0x%x\n", readl(switch_reg | PP_WIN_SR(0)));
	dprintk("read pp: 0x%x\n", readl(switch_reg | PP_WIN_CTRL(0)));

	/* Debug dma reg - according to old code in
	* arch/arm/mach-armadaxp/pss/hwServices.c
	*/
	writel(0xaaba,	switch_reg | 0x2684);
	dprintk("%s read pp: 0x%x\n", __func__, readl(switch_reg | 0x2684));
}

/*******************************************************************************
*	prestera_pci_dev_config
*
*
*******************************************************************************/
static int prestera_pci_dev_config(struct pci_dev *pdev)
{
	int err;
	void __iomem * const *iomap = NULL;
	void __iomem *switch_reg = NULL;
	unsigned short tmpDevId = pdev->device;

	/* Make sure all AC3/BC2 device flavours are handled in the same way */
	if (((pdev->device & ~MV_DEV_FLAVOUR_MASK) == MV_BOBCAT2_DEV_ID) ||
		((pdev->device & ~MV_DEV_FLAVOUR_MASK) == MV_ALLEYCAT3_DEV_ID))
		tmpDevId &= ~MV_DEV_FLAVOUR_MASK;

	switch (tmpDevId) {

	case MV_IDT_SWITCH_DEV_ID_808E:
	case MV_IDT_SWITCH_DEV_ID_802B:
		bspSmiReadRegLionSpecificSet();
		return 0;

	case MV_BOBCAT2_DEV_ID:
	case MV_ALLEYCAT3_DEV_ID:
	case MV_LION2_DEV_ID:

		iomap = pcim_iomap_table(pdev);
		inter_regs = iomap[MV_PCI_BAR_INTER_REGS];
		switch_reg = iomap[MV_PCI_BAR_1];

		dprintk("inter_regs: %p, bar1: %p\n",
			iomap[MV_PCI_BAR_INTER_REGS], iomap[MV_PCI_BAR_1]);
#ifdef MV_PP_DBG
		if (pdev->device  != MV_LION2_DEV_ID)
			dprintk("bar2: %p\n", iomap[MV_PCI_BAR_2]);
#endif

		break;

	default:
		dprintk("%s: unsupported device\n", __func__);
	}

	err = mv_ppdev_conf_set_pci(pdev);
	if (err)
		return err;

	err = prestera_init();
	if (err)
		return err;

	/* MV_BOBCAT2_DEV_ID /  MV_ALLEYCAT3_DEV_ID */
	if (pdev->device  != MV_LION2_DEV_ID) {
		mv_pci_dma_switch_init((unsigned long)switch_reg, pdev);
		dprintk("DFX(0x%x) test %x and should be ..357\n", iomap[MV_PCI_BAR_2] + DFX_JTAG_DEVID_STAT,
			readl(iomap[MV_PCI_BAR_2] + DFX_JTAG_DEVID_STAT));
	}

	dev_info(&pdev->dev, "%s init completed\n", prestera_drv_name);
	return 0;
}

/*******************************************************************************
*	prestera_pltfm_probe
*
*	Prestera devices probe function
*
*******************************************************************************/
static int prestera_pci_dev_probe(void)
{
	unsigned long type = 0;
	unsigned long instance = 0;
	unsigned long busNo, devSel, funcNo;
	unsigned short devId, vendorId, flavour;

	/*
	**  PCI Configuration Section
	**  ===============
	*/
	prestera_global_init();

	for (type = 0; presteraPciDevs[type].deviceNum != (-1); type++)	{

		devId = presteraPciDevs[type].deviceId;
		vendorId = presteraPciDevs[type].vendorId;

		for (flavour = 0; flavour < MV_DEV_FLAVOUR_MASK; flavour++) {

			instance = 0;
			while (bspPciFindDev(vendorId, devId + flavour, instance, &busNo, &devSel, &funcNo) == 0) {
				dprintk(KERN_INFO "vendorId 0x%x, devId 0x%x, instance 0x%lx\n",
						vendorId, devId + flavour, instance);
				dprintk(KERN_INFO "busNo 0x%lx, devSel 0x%lx, funcNo 0x%lx\n",
						busNo, devSel, funcNo);
				instance++;

				prestera_pci_dev_config(pci_get_bus_and_slot(busNo, PCI_DEVFN(devSel, funcNo)));
			}

			/* devices that support flavours */
			if ((devId != MV_BOBCAT2_DEV_ID) &&
				(devId != MV_ALLEYCAT3_DEV_ID))
				break;
		} /* device flavours */

	} /* table entries */

	return 0;
}

/*******************************************************************************
*	prestera_pltfm_probe
*
*	Prestera devices probe function
*
*******************************************************************************/
static int prestera_pltfm_probe(struct platform_device *pdev)
{
	int err;
#ifndef CONFIG_OF
	unsigned int *pdata;
	unsigned int boardId;
#endif

	/*
	**  PCI Devices  Configuration Section
	**  =====================
	*/
	printk(KERN_INFO "\n==Start PCI Devices scan and configure==\n");
	err = prestera_pci_dev_probe();
	if (0 != err)
		return err;

#ifndef CONFIG_OF
	/*
	**  Internal Device Configuration Section
	**  =======================
	*/
	printk(KERN_INFO "\n==Start Internal Devices scan and configure==\n");
	pdata = (unsigned int *)dev_get_platdata(&pdev->dev);
	if (!pdata) {
		printk(KERN_INFO "No internal device detected\n");
	} else {
		boardId = *pdata;
		printk(KERN_INFO "Internal device 0x%x detected\n", boardId);
		err = prestera_Internal_dev_probe(boardId);
		if (0 != err)
			return err;
	}
#endif

	return 0;
}

/*******************************************************************************
*	prestera_pltfm_cleanup
*
*
*******************************************************************************/
static int prestera_pltfm_cleanup(struct platform_device *pdev)
{
	/*
	 * do nothing, ppdev is freed during char-dev clean-up
	 * (prestera_cleanup)
	 */
	return 0;
}

static struct of_device_id mv_prestera_dt_ids[] = {
	{ .compatible = "marvell,armada-prestera", },
	{},
};
MODULE_DEVICE_TABLE(of, mv_prestera_dt_ids);

static struct platform_driver prestera_driver = {
	.driver = {
		.name	= DRIVER_NAME,
		.owner	= THIS_MODULE,
#ifdef CONFIG_OF
		.of_match_table = of_match_ptr(mv_prestera_dt_ids),
#endif
	},
	.probe		= prestera_pltfm_probe,
	.remove		= prestera_pltfm_cleanup,
};

static int __init prestera_pltfm_init(void)
{
	return platform_driver_register(&prestera_driver);
}
late_initcall(prestera_pltfm_init);

static void __exit prestera_pltfm_exit(void)
{
	platform_driver_unregister(&prestera_driver);
}
module_exit(prestera_pltfm_exit);

MODULE_ALIAS("prestera_device");
MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("device driver for Marvell Prestera family switches");
