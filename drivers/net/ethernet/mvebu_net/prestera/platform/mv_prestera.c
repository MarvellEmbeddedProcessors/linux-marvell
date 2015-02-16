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
* mv_prestera.c
*
* DESCRIPTION:
*	functions in kernel mode special for prestera.
*
* DEPENDENCIES:
*
* COMMENTS:
*   Please note: this file is shared for:
*       axp_lsp_3.4.69
*       msys_lsp_3_4
*       msys_lsp_2_6_32
*
*******************************************************************************/
#include "mvOs.h"
#include "mv_prestera_glob.h"
#include "mv_prestera_smi.h"
#include "mv_prestera_smi_glob.h"
#include "mv_prestera_irq.h"
#include "mv_prestera.h"
#include "mv_pss_api.h"
#include "mv_prestera_pci.h"
#ifdef PRESTERA_PP_DRIVER
#include "mv_prestera_pp_driver_glob.h"
#endif

#if defined(CONFIG_MIPS) && defined(CONFIG_64BIT)
#define MIPS64_CPU
#endif

#if defined(INTEL64_CPU) || defined(EP3041A) || \
	 (defined(CONFIG_MIPS) && defined(CONFIG_64BIT))
#define EXT_ARCH_64_CPU
#endif

#if defined(GDA8548) || defined(EP3041A)
#define consistent_sync(x...)
#endif

#if defined(MIPS64_CPU) || defined(INTEL64_CPU)
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 27)
#define consistent_sync(x...)
#endif
#endif

#if LINUX_VERSION_CODE >= KERNEL_VERSION(3, 0, 6)
#define consistent_sync(x...)
#endif

#undef MV_DEBUG

/* defines  */
#ifdef MV_DEBUG
#define dprintk(a...) printk(a)
#else
#define dprintk(a...)
#endif

#define MAX_SUPPORTED_INTR	(4)

#define PP_MAPPINGS_MAX             64

struct prvPciDeviceQuirks quirks[] = {
	{MV_LION2_DEV_ID, PCI_DEV_PEX_EN, PCI_DEV_LION_CONFIG_OFFSET, PCI_DEV_DFX_DIS, {0, 3, 2, 1, 0, 0, 0, 0} },
	{MV_BOBCAT2_DEV_ID, PCI_DEV_PEX_EN, PCI_DEV_BC2_CONFIG_OFFSET, PCI_DEV_DFX_EN, {0, 0, 0, 0, 0, 0, 0, 0} },
	{MV_ALLEYCAT3_DEV_ID, PCI_DEV_PEX_EN, PCI_DEV_AC3_CONFIG_OFFSET, PCI_DEV_DFX_EN, {0, 1, 0, 0, 0, 0, 0, 0} },
	{-1, -1, -1, -1}
};

/* local variables and variables */
static int			dev_open_nr;
static int			dev_init_done;
static struct prestera_device	*prestera_dev;
static int			prestera_major = PRESTERA_MAJOR;
static struct cdev		prestera_cdev;
static int			prestera_ppdev;
static const char		*prestera_dev_name = "mvPP";
static uintptr_t		pci_regs_vma_start = CPSS_SWITCH_VIRT_ADDR;
static uintptr_t		pci_regs_vma_end = CPSS_SWITCH_VIRT_ADDR;
static uintptr_t		pci_dfx_vma_start = CPSS_DFX_VIRT_ADDR;
static uintptr_t		pci_dfx_vma_end = CPSS_DFX_VIRT_ADDR;
static uintptr_t		pci_conf_vma_start = CPSS_CPU_VIRT_ADDR;
static uintptr_t		pci_conf_vma_end = CPSS_CPU_VIRT_ADDR;
static uintptr_t		dma_base_vma_start = CPSS_DMA_VIRT_ADDR;
static uintptr_t		dma_base_vma_end = CPSS_DMA_VIRT_ADDR + 2 * _1M;
static struct pp_dev		*ppdevs[PRV_MAX_PP_DEVICES];
static int			founddevs;
static unsigned int		dma_len;
unsigned long			dma_base;
static void			*dma_area;
static void			*dma_tmp_virt;
static dma_addr_t		dma_tmp_phys;
/* info for mmap */
static struct Mmap_Info_stc mmapInfoArr[PP_MAPPINGS_MAX];
static int                  mmapInfoArrSize ;
#define M mmapInfoArr[mmapInfoArrSize]

#ifdef EXT_ARCH_64_CPU
int bspAdv64Malloc32bit; /* in bssBspApis.c */
#endif

/************************************************************************
 *
 * get_founddev: retrieve number of detected devices
 *
 */
unsigned int get_founddev(void)
{
	return founddevs;
}

/************************************************************************
 *
 * prestera_global_init: init global parameters
 *
 */
int prestera_global_init(void)
{
	mmapInfoArrSize = 0;
	founddevs = 0;
	return 0;
}

/************************************************************************
 *
 * get_quirks: retrive quirks instance according to board type
 *
 */
static int32_t get_quirks(unsigned short devId)
{
	uint32_t index = 0;

	while (quirks[index].pciId != (-1)) {
		if (devId == quirks[index].pciId)
			return index;
		index++;
	}
	return -1;
}

/************************************************************************
 *
 * prestera_mapped_virt2phys: convert userspace address to physical
 * Only for mmaped areas
 *
 */
static mv_phys_addr_t prestera_mapped_virt2phys(unsigned long address)
{
	if (address >= dma_base_vma_start && address < dma_base_vma_end) {
		address -= dma_base_vma_start;
		address += dma_base;
		return (mv_phys_addr_t)address;
	}
	if (address >= pci_regs_vma_start && address < pci_regs_vma_end) {
		struct pp_dev   *dev;
		int             i;

		for (i = 0; i < founddevs; i++) {
			dev = ppdevs[i];
			if (address >= dev->ppregs.mmapbase && address < dev->ppregs.mmapbase + dev->ppregs.mmapsize) {
				address -= dev->ppregs.mmapbase;
				return dev->ppregs.phys + address;
			}
		}
		/* should never happen? */
			return 0;
	}

	/* default */
	return 0;
}


/************************************************************************
 *
 * prestera_DmaRead: bspDmaRead() wrapper
 */
static int prestera_dma_read(unsigned long address,
			     unsigned long length,
			     unsigned long burstLimit,
			     unsigned long buffer)
{
	unsigned long bufferPhys;
	unsigned long tmpLength;
	mv_phys_addr_t phys;

	dprintk("%s(address=0x%lx, length=0x%lx, burstLimit=0x%lx, buffer=0x%lx)\n",
			__func__,
			(unsigned long)(address),
			(unsigned long)(length),
			(unsigned long)burstLimit,
			(unsigned long)buffer);

	phys = prestera_mapped_virt2phys(address);
	if (!phys)
		return -EFAULT;

	bufferPhys = prestera_mapped_virt2phys(buffer);
	if (bufferPhys)
		return bspDmaRead(phys, length, burstLimit, (unsigned long *)bufferPhys);

	/* use dma_tmp buffer */
	while (length > 0) {
		tmpLength = (length > (PAGE_SIZE / 4)) ? PAGE_SIZE / 4 : length;

		if (bspDmaRead(phys, tmpLength, burstLimit, (unsigned long *)dma_tmp_phys))
			return -EFAULT;

		length -= tmpLength;
		tmpLength *= 4;
		if (copy_to_user((void *)buffer, dma_tmp_virt, tmpLength))
			return -EFAULT;

		phys += tmpLength;
		buffer += tmpLength;
	}
	return 0;
}

/************************************************************************
 *
 * prestera_DmaWrite: bspDmaRead() wrapper
 */
static int prestera_dma_write(unsigned long address,
				unsigned long length,
				unsigned long burstLimit,
				unsigned long buffer)
{
	unsigned long bufferPhys;
	unsigned long tmpLength;
	mv_phys_addr_t phys;

	dprintk("%s(address=0x%lx, length=0x%lx, burstLimit=0x%lx, buffer=0x%lx)\n",
			__func__,
			(unsigned long)(address),
			(unsigned long)(length),
			(unsigned long)burstLimit,
			(unsigned long)buffer);

	phys = prestera_mapped_virt2phys(address);
	if (!phys)
		return -EFAULT;

	bufferPhys = prestera_mapped_virt2phys(buffer);
	if (bufferPhys)
		return bspDmaWrite(phys, (unsigned long *)bufferPhys, length, burstLimit);
	/* use dma_tmp buffer */
	while (length > 0) {
		tmpLength = (length > (PAGE_SIZE / 4)) ? PAGE_SIZE / 4 : length;

		if (copy_from_user(dma_tmp_virt, (void *)buffer, tmpLength * 4))
			return -EFAULT;

		if (bspDmaWrite(phys, (unsigned long *)dma_tmp_phys, tmpLength, burstLimit))
			return -EFAULT;

		length -= tmpLength;
		tmpLength *= 4;
		phys += tmpLength;
		buffer += tmpLength;
	}
	return 0;
}

static loff_t prestera_lseek(struct file *filp, loff_t off, int whence)
{
	struct prestera_device  *dev;
	loff_t                  newpos;

	dev = (struct prestera_device *)filp->private_data;

	dprintk("%s(whence=0x%lx, off=0x%lx)\n",
			__func__,
			(unsigned long)(whence),
			(unsigned long)(off));

	switch (whence) {
	case 0: /* SEEK_SET */
		newpos = off;
		break;

	case 1: /* SEEK_CUR */
		newpos = filp->f_pos + off;
		break;

	case 2: /* SEEK_END */
		newpos = dev->size + off;
		break;

	default: /* can't happend */
		printk(KERN_ERR "%s whence %d ERROR\n", __func__, whence);
		return -EINVAL;
	}
	if (newpos < 0)
		return -EINVAL;

	if (newpos >= dev->size)
		return -EINVAL;

	filp->f_pos = newpos;
	return newpos;
}

/*
 * find device index
 * return -1 if not found
 */
static int find_pp_device(uint32_t busNo, uint32_t devSel, uint32_t funcNo)
{
	int             i;
	struct pp_dev   *pp;
	for (i = 0; i < founddevs; i++) {
		pp = ppdevs[i];
		if (pp->busNo != busNo ||
				pp->devSel != devSel ||
				pp->funcNo != funcNo)
			continue;
		/* found */
		return i;
	}
	return -1; /* not found */
}

/*
 * configure virtual addresses
 */
static void ppdev_set_vma(struct pp_dev *dev, int devIdx, unsigned long regsSize)
{
	if (dev->ppregs.mmapbase != 0)
		return; /* already configured */

	/* allocate virtual addresses */
	dev->ppregs.mmapbase = pci_regs_vma_end;
	dev->ppregs.mmapsize = regsSize;
	pci_regs_vma_end += dev->ppregs.mmapsize;
	M.map_type = MMAP_INFO_TYPE_PP_REGS_E;
	M.index  = devIdx;
	M.addr   = dev->ppregs.mmapbase;
	M.length = dev->ppregs.mmapsize;
	M.offset = 0;
	mmapInfoArrSize++;

	dev->config.mmapbase = pci_conf_vma_end;
	dev->config.mmapsize = dev->config.size;
	pci_conf_vma_end += dev->config.mmapsize;
	M.map_type = MMAP_INFO_TYPE_PP_CONF_E;
	M.index  = devIdx;
	M.addr   = dev->config.mmapbase;
	M.length = dev->config.mmapsize;
	M.offset = dev->config.mmapoffset;
	mmapInfoArrSize++;

	if (dev->dfx.phys != 0) {
		dev->dfx.mmapbase = pci_dfx_vma_end;
		dev->dfx.mmapsize = dev->dfx.size;
		pci_dfx_vma_end += dev->dfx.mmapsize;
		M.map_type = MMAP_INFO_TYPE_PP_DFX_E;
		M.index  = devIdx;
		M.addr   = dev->dfx.mmapbase;
		M.length = dev->dfx.mmapsize;
		M.offset = 0;
		mmapInfoArrSize++;
	}
}

#ifdef PRESTERA_PP_DRIVER
/* ppDriver */
static int presteraPpDriver_ioctl(unsigned int cmd, unsigned long arg)
{
	struct pp_dev *dev;

	if (cmd == PRESTERA_PP_DRIVER_IO) {
		struct mvPpDrvDriverIo_STC io;

		if (copy_from_user(&io, (struct mvPpDrvDriverIo_STC *)arg, sizeof(io))) {
			printk(KERN_ERR "IOCTL: FAULT\n");
			return -EFAULT;
		}
		if (io.id >= founddevs)
			return -EFAULT;
		dev = ppdevs[io.id];
		if (dev->ppdriver)
			return dev->ppdriver(dev->ppdriverData, &io);
		return -EFAULT;
	}
	if (cmd == PRESTERA_PP_DRIVER_OPEN) {
		struct mvPpDrvDriverOpen_STC info;
		int i;
		if (copy_from_user(&info, (struct mvPpDrvDriverOpen_STC *)arg, sizeof(info))) {
			printk(KERN_ERR "IOCTL: FAULT\n");
			return -EFAULT;
		}

		i = find_pp_device(info.busNo, info.devSel, info.funcNo);
		if (i < 0) {
			/* device not found */
			return -EFAULT;
		}
		dev = ppdevs[i];

		if (dev->ppdriver != NULL) {
			if ((int)info.type != dev->ppdriverType) {
				/* driver doesn't match */
				return -EFAULT;
			}
			/* driver is the same, skip */
		} else {
			switch (info.type) {
			case mvPpDrvDriverType_Pci_E:
				presteraPpDriverPciPexCreate(dev);
				break;
			case mvPpDrvDriverType_PciHalf_E:
				presteraPpDriverPciPexHalfCreate(dev);
				break;
			case mvPpDrvDriverType_PexMbus_E:
				presteraPpDriverPexMbusCreate(dev);
				break;
			}
			if (dev->ppdriver == NULL) {
				/* failed to initialize, return error */
				return -EFAULT;
			}
		}
		info.id = i;

		if (copy_to_user((struct mvPpDrvDriverOpen_STC *)arg, &info, sizeof(info))) {
			printk(KERN_ERR "IOCTL: FAULT\n");
			return -EFAULT;
		}
	}
	return 0;
}
#endif /* defined(PRESTERA_PP_DRIVER) */

#ifdef MV_DEBUG
static void ioctl_cmd_pr(unsigned int cmd)
{
	char *dir;

	static const char * const prestera_ioctls[] = {
		[_IOC_NR(PRESTERA_IOC_HWRESET)] = "HW_RESET",
		[_IOC_NR(PRESTERA_IOC_INTCONNECT)] = "INT_CONNECT",
		[_IOC_NR(PRESTERA_IOC_INTENABLE)] = "INT_ENABLE",
		[_IOC_NR(PRESTERA_IOC_INTDISABLE)] = "INT_DISABLE",
		[_IOC_NR(PRESTERA_IOC_WAIT)] = "WAIT",
		[_IOC_NR(PRESTERA_IOC_FIND_DEV)] = "FIND_DEV",
		[_IOC_NR(PRESTERA_IOC_PCICONFIGWRITEREG)] = "PCI_CONFIG_WRITE_REG",
		[_IOC_NR(PRESTERA_IOC_PCICONFIGREADREG)] = "PCI_CONFIG_READ_REG",
		[_IOC_NR(PRESTERA_IOC_GETINTVEC)] = "GET_INT_VEC",
		[_IOC_NR(PRESTERA_IOC_FLUSH)] = "FLUSH",
		[_IOC_NR(PRESTERA_IOC_INVALIDATE)] = "INVALIDATE",
		[_IOC_NR(PRESTERA_IOC_GETBASEADDR)] = "GET_BASE_ADDR",
		[_IOC_NR(PRESTERA_IOC_DMAWRITE)] = "DMA_WRITE",
		[_IOC_NR(PRESTERA_IOC_DMAREAD)] = "DMA_READ",
		[_IOC_NR(PRESTERA_IOC_GETDMASIZE)] = "GET_DMA_SIZE",
		[_IOC_NR(PRESTERA_IOC_TWSIINITDRV)] = "TWSI_INIT_DRV",
		[_IOC_NR(PRESTERA_IOC_TWSIWAITNOBUSY)] = "TWSI_WAIT_NO_BUSY",
		[_IOC_NR(PRESTERA_IOC_TWSIWRITE)] = "TWSI_WRITE",
		[_IOC_NR(PRESTERA_IOC_TWSIREAD)] = "TWSI_READ",
		[_IOC_NR(PRESTERA_IOC_GETMAPPING)] = "GET_MAPPING",
	};
	#define PRESTERA_IOCTLS ARRAY_SIZE(prestera_ioctls)

	switch (_IOC_DIR(cmd)) {
	case _IOC_NONE:
		dir = "--";
		break;
	case _IOC_READ:
		dir = "r-";
		break;
	case _IOC_WRITE:
		dir = "-w";
		break;
	case _IOC_READ | _IOC_WRITE:
		dir = "rw";
		break;
	default:
		dir = "*ERR*"; break;
	}
	printk(KERN_ERR "got ioctl '%c', dir=%s, #%d (0x%08x) ",
		_IOC_TYPE(cmd), dir, _IOC_NR(cmd), cmd);

	if (_IOC_NR(cmd) < PRESTERA_IOCTLS)
		printk("%s\n", prestera_ioctls[_IOC_NR(cmd)]);
}
#endif

/************************************************************************
 *
 * prestera_ioctl: The device ioctl() implementation
 */
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 11)
#ifndef HAVE_UNLOCKED_IOCTL
#define HAVE_UNLOCKED_IOCTL 1
#endif
#endif
#ifdef HAVE_UNLOCKED_IOCTL
static long prestera_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
#else
static int prestera_ioctl(struct inode *inode, struct file *filp, unsigned int cmd, unsigned long arg)
#endif
{
	struct pp_dev			*dev = NULL;
	struct GT_PCI_Dev_STC		gtDev;
	struct GT_Intr2Vec		int2vec;
	struct GT_VectorCookie_STC	vector_cookie;
	struct PciConfigReg_STC		pciConfReg;
	struct GT_RANGE_STC		range;
	struct intData			*intData;
	int				i;
	struct GT_DmaReadWrite_STC	dmaRWparams;
	struct GT_TwsiReadWrite_STC	twsiRWparams;
	mv_kmod_size_t			temp_len;
	unsigned long			intrline;
	struct GT_PCI_Mapping_STC	mapping;
	mv_phys_addr_t			ret;
	struct GT_PCI_MMAP_INFO_STC	mInfo;
	struct GT_PCI_VMA_ADDRESSES_STC	vmaInfo;


#ifdef PRESTERA_PP_DRIVER
	if (_IOC_TYPE(cmd) == PRESTERA_PP_DRIVER_IOC_MAGIC)
		return presteraPpDriver_ioctl(cmd, arg);
#endif /* defined(PRESTERA_PP_DRIVER) */
	if (_IOC_TYPE(cmd) == PRESTERA_SMI_IOC_MAGIC)
		return prestera_smi_ioctl(cmd, arg);

	/* don't even decode wrong cmds: better returning  ENOTTY than EFAULT */
	if (_IOC_TYPE(cmd) != PRESTERA_IOC_MAGIC) {
		printk(KERN_ERR "wrong ioctl magic key\n");
		return -ENOTTY;
	}

#ifdef MV_DEBUG
	if (cmd != PRESTERA_IOC_WAIT)
		ioctl_cmd_pr(cmd);
#endif

	switch (cmd) {
	case PRESTERA_IOC_DMAWRITE:
		if (copy_from_user(&dmaRWparams, (struct GT_DmaReadWrite_STC *)arg, sizeof(dmaRWparams))) {
			printk(KERN_ERR "copy_from_user failed\n");
			return -EFAULT;
		}
		return prestera_dma_write(dmaRWparams.address,
						dmaRWparams.length,
						dmaRWparams.burstLimit,
						(unsigned long)dmaRWparams.buffer);

	case PRESTERA_IOC_DMAREAD:
		if (copy_from_user(&dmaRWparams, (struct GT_DmaReadWrite_STC *)arg, sizeof(dmaRWparams))) {
			printk(KERN_ERR "copy_from_user failed\n");
			return -EFAULT;
		}
		return prestera_dma_read(dmaRWparams.address,
						dmaRWparams.length,
						dmaRWparams.burstLimit,
						(unsigned long)dmaRWparams.buffer);

	case PRESTERA_IOC_HWRESET:
		printk(KERN_INFO "got PRESTERA_IOC_HWRESET, do nothing\n");
		break;

	case PRESTERA_IOC_INTCONNECT:
		/* read and parse user data structure */
		if (copy_from_user(&vector_cookie, (struct GT_VectorCookie_STC *)arg,
					sizeof(vector_cookie))) {
			printk(KERN_ERR "copy_from_user failed\n");
			return -EFAULT;
		}

		/* Find ppdevs associated with requested irq nr */
		for (i = 0; i < founddevs; i++) {
			if (ppdevs[i]->irq_data.intVec == vector_cookie.vector)
				break;
		}
		if (i == founddevs)
			return -ENODEV;

		if (prestera_int_connect(ppdevs[i], 0, &intData)) {
			printk(KERN_ERR "prestera_int_connect failed\n");
			return -EFAULT;
		}
		vector_cookie.cookie = (mv_kmod_uintptr_t)((uintptr_t)intData);

		dprintk("PRESTERA_IOC_INTCONNECT interrupt %lx\n", vector_cookie.vector);

		/* USER READS */
		if (copy_to_user((struct GT_VectorCookie_STC *)arg, &vector_cookie,
				 sizeof(vector_cookie))) {
			printk(KERN_ERR "copy_to_user failed\n");
			return -EFAULT;
		}
		break;

	case PRESTERA_IOC_INTENABLE:
		/* clear the mask reg on device 0x10 */
		if (arg > 64)
			send_sig_info(SIGSTOP, (struct siginfo *)1, current);
		enable_irq(arg);
		break;

	case PRESTERA_IOC_INTDISABLE:
		disable_irq(arg);
		break;

	case PRESTERA_IOC_WAIT:
		/* cookie */
		intData = (struct intData *)arg;

		/* enable the interrupt vector */
		enable_irq(intData->intVec);

		if (down_interruptible(&intData->sem)) {
			/* to avoid unbalanced irq warning when suspended by gdb */
			disable_irq(intData->intVec);
			return -ERESTARTSYS;
		}
		break;

	case PRESTERA_IOC_FIND_DEV:
		/* read and parse user data structure */
		if (copy_from_user(&gtDev, (struct GT_PCI_Dev_STC *) arg, sizeof(gtDev))) {
			printk(KERN_ERR "copy_from_user failed\n");
			return -EFAULT;
		}

		for (i = 0; i < founddevs; i++) {
			dev = ppdevs[i];
			if ((gtDev.vendorId == dev->vendorId) &&
				(gtDev.devId == dev->devId) &&
				(gtDev.instance == dev->instance))
				break;
		}
		if (i == founddevs)
			return -ENODEV;

		/* Found */
		gtDev.busNo = dev->busNo;
		gtDev.devSel = dev->devSel;
		gtDev.funcNo = dev->funcNo;

		dprintk("PCI_FIND_DEV: pci? %d bus# %ld devSel %ld func# %ld vendId 0x%X devId 0x%X inst %lx\n",
				dev->on_pci_bus, gtDev.busNo, gtDev.devSel,
				gtDev.funcNo, gtDev.vendorId, gtDev.devId, gtDev.instance);

		/* READ */
		if (copy_to_user((struct GT_PCI_Dev_STC *)arg, &gtDev, sizeof(gtDev))) {
			printk(KERN_ERR "copy_from_user failed\n");
			return -EFAULT;
		}
		break;

	case PRESTERA_IOC_GETMAPPING:
		/* read and parse user data structure */
		if (copy_from_user(&mapping, (struct GT_PCI_Mapping_STC *)arg,
					sizeof(mapping))) {
			printk(KERN_ERR "copy_from_user failed\n");
			return -EFAULT;
		}

		dprintk("Search (mapping) bus %x, devSel %x, func%x\n",
				mapping.busNo, mapping.devSel, mapping.funcNo);

		i = find_pp_device(mapping.busNo, mapping.devSel, mapping.funcNo);
		if (i < 0) {
			/* not found */
			return -ENODEV;
		}
		dev = ppdevs[i];
		/* configure virtual addresses if not configured yet */
		ppdev_set_vma(dev, i, mapping.regsSize);

		mapping.mapConfig.addr = dev->config.mmapbase;
		mapping.mapConfig.length = (size_t)(dev->config.mmapsize);
		mapping.mapConfig.offset = (size_t)(dev->config.mmapoffset);
		mapping.mapRegs.addr = dev->ppregs.mmapbase;
		mapping.mapRegs.length = (size_t)(dev->ppregs.mmapsize);
		mapping.mapRegs.offset = (size_t)(dev->ppregs.mmapoffset);
		mapping.mapDfx.addr = dev->dfx.mmapbase;
		mapping.mapDfx.length = (size_t)(dev->dfx.mmapsize);
		mapping.mapDfx.offset = (size_t)(dev->dfx.mmapoffset);

		if (copy_to_user((struct GT_PCI_Mapping_STC *)arg, &mapping, sizeof(mapping))) {
			printk(KERN_ERR "copy_to_user failed\n");
			return -EFAULT;
		}
		break;

	case PRESTERA_IOC_PCICONFIGWRITEREG:
		/* read and parse user data structure */
		if (copy_from_user(&pciConfReg, (struct PciConfigReg_STC *)arg, sizeof(pciConfReg))) {
			printk(KERN_ERR "copy_from_user failed\n");
			return -EFAULT;
		}

		for (i = 0; i < founddevs; i++) {
			dev = ppdevs[i];
			if ((pciConfReg.busNo == dev->busNo) &&
				(pciConfReg.devSel == dev->devSel) &&
				(pciConfReg.funcNo == dev->funcNo))
				break;
		}
		if (i == founddevs)
			return -ENODEV;

		if (dev->on_pci_bus) {
			if (bspPciConfigWriteReg(pciConfReg.busNo, pciConfReg.devSel,
				pciConfReg.funcNo, pciConfReg.regAddr,
				pciConfReg.data) != MV_OK) {
				printk(KERN_ERR "bspPciConfigWriteReg failed\n");
				return -EFAULT;
			}
		}
		break;

	case PRESTERA_IOC_PCICONFIGREADREG:
		/* read and parse user data structure */
		if (copy_from_user(&pciConfReg, (struct PciConfigReg_STC *) arg, sizeof(pciConfReg))) {
			printk(KERN_ERR "copy_from_user failed\n");
			return -EFAULT;
		}

		for (i = 0; i < founddevs; i++) {
			dev = ppdevs[i];
			if ((pciConfReg.busNo == dev->busNo) &&
				(pciConfReg.devSel == dev->devSel) &&
				(pciConfReg.funcNo == dev->funcNo))
				break;
		}

		if (i == founddevs)
			return -ENODEV;

		pciConfReg.data = 0;

		if (dev->on_pci_bus) {
			unsigned long data;
			if (bspPciConfigReadReg(pciConfReg.busNo,
							pciConfReg.devSel,
							pciConfReg.funcNo,
							pciConfReg.regAddr,
							&data) != MV_OK) {
				printk(KERN_ERR "bspPciConfigReadReg failed\n");
				return -EFAULT;
			}
			pciConfReg.data = (uint32_t)data;
		}

		if (copy_to_user((struct PciConfigReg_STC *)arg, &pciConfReg, sizeof(pciConfReg))) {
			printk(KERN_ERR "copy_to_user failed\n");
			return -EFAULT;
		}

		break;

	case PRESTERA_IOC_GETINTVEC:
		if (copy_from_user(&int2vec, (struct GT_Intr2Vec *)arg, sizeof(int2vec))) {
			printk(KERN_ERR "copy_from_user failed\n");
			return -EFAULT;
		}

		i = get_quirks(prestera_ppdev);
		if (i  == (-1)) {
			printk(KERN_ERR "get_quirks failed for interrupt get\n");
			return -EFAULT;
		}
		if (int2vec.intrLine > MAX_SUPPORTED_INTR) {
			printk(KERN_ERR "intrLine %d is out of range\n", (int)int2vec.intrLine);
			return -EFAULT;
		}
		intrline = quirks[i].interruptMap[int2vec.intrLine];

		dprintk("PRESTERA_IOC_GETINTVEC input line %ld output line %ld\n", int2vec.intrLine, intrline);

		if (MV_OK != bspPciGetIntVec(intrline, (void *)&int2vec.vector)) {
			printk(KERN_ERR "bspPciGetIntVec failed\n");
			return -EFAULT;
		}

		if (copy_to_user((struct GT_Intr2Vec *)arg, &int2vec, sizeof(int2vec))) {
			printk(KERN_ERR "copy_to_user failed\n");
			return -EFAULT;
		}
		break;

	case PRESTERA_IOC_FLUSH:
		/* read and parse user data structure */
		if (copy_from_user(&range, (struct GT_RANGE_STC *)arg, sizeof(range))) {
			printk(KERN_ERR "copy_from_user failed\n");
			return -EFAULT;
		}
#if LINUX_VERSION_CODE >= KERNEL_VERSION(3, 4, 69) || defined(CONFIG_ARCH_MSYS)
		pci_map_single(NULL, (void *)range.address, range.length, PCI_DMA_TODEVICE);
#else
		consistent_sync((void *)range.address, range.length, PCI_DMA_TODEVICE);
#endif
		break;

	case PRESTERA_IOC_INVALIDATE:
		/* read and parse user data structure */
		if (copy_from_user(&range, (struct GT_RANGE_STC *)arg, sizeof(range))) {
			printk(KERN_ERR "copy_from_user failed\n");
			return -EFAULT;
		}
#if LINUX_VERSION_CODE >= KERNEL_VERSION(3, 4, 69) || defined(CONFIG_ARCH_MSYS)
		pci_map_single(NULL, (void *)range.address, range.length, PCI_DMA_FROMDEVICE);
#else
		consistent_sync((void *)range.address, range.length, PCI_DMA_FROMDEVICE);
#endif

		break;

	case PRESTERA_IOC_GETBASEADDR:
		ret = (mv_phys_addr_t)dma_base;
		if (copy_to_user((void *)arg, &ret, sizeof(ret))) {
			printk(KERN_ERR "copy_to_user failed\n");
			return -EFAULT;
		}
		break;

	case PRESTERA_IOC_GETDMASIZE:
		temp_len = dma_len;

#ifdef EXT_ARCH_64_CPU
		if (bspAdv64Malloc32bit == 1)
			temp_len |= 0x80000000;
#endif
		if (copy_to_user((void *)arg, &temp_len, sizeof(temp_len))) {
			printk(KERN_ERR "copy_to_user failed\n");
			return -EFAULT;
		}
		break;

	case PRESTERA_IOC_TWSIINITDRV:
		if (bspTwsiInitDriver() != MV_OK) {
			printk(KERN_ERR "bspTwsiInitDriver failed\n");
			return -EFAULT;
		}
		break;

	case PRESTERA_IOC_TWSIWAITNOBUSY:
		if (bspTwsiWaitNotBusy() != MV_OK) {
			printk(KERN_ERR "bspTwsiWaitNotBusy failed\n");
			return -EFAULT;
		}
		break;

	case PRESTERA_IOC_TWSIWRITE:
		/* read and parse user data structure */
		if (copy_from_user(&twsiRWparams, (struct GT_TwsiReadWrite_STC *)arg,
				   sizeof(twsiRWparams))) {
			printk(KERN_ERR "copy_from_user failed\n");
			return -EFAULT;
		}
		if (bspTwsiMasterWriteTrans(twsiRWparams.devId, twsiRWparams.pData,
						twsiRWparams.len, twsiRWparams.stop) != MV_OK) {
			printk(KERN_ERR "bspTwsiMasterWriteTrans failed\n");
			return -EFAULT;
		}

		break;

	case PRESTERA_IOC_TWSIREAD:
		/* read and parse user data structure */
		if (copy_from_user(&twsiRWparams, (struct GT_TwsiReadWrite_STC *)arg,
				   sizeof(twsiRWparams))) {
			printk(KERN_ERR "copy_from_user failed\n");
			return -EFAULT;
		}
		if (bspTwsiMasterReadTrans(twsiRWparams.devId, twsiRWparams.pData,
					    twsiRWparams.len, twsiRWparams.stop) != MV_OK) {
			printk(KERN_ERR "bspTwsiMasterReadTrans failed\n");
			return -EFAULT;
		}
		if (copy_to_user((struct GT_TwsiReadWrite_STC *)arg, &twsiRWparams,
				 sizeof(twsiRWparams))) {
			printk(KERN_ERR "copy_to_user failed\n");
			return -EFAULT;
		}

		break;

	case PRESTERA_IOC_ISFIRSTCLIENT:
		if (dev_open_nr == 1)
			return 0;
		return 1;

	case PRESTERA_IOC_GETMMAPINFO:
		/* read and parse user data structure */
		if (copy_from_user(&mInfo, (struct GT_PCI_MMAP_INFO_STC *) arg, sizeof(mInfo))) {
			printk(KERN_ERR "copy_from_user failed\n");
			return -EFAULT;
		}
		if (mInfo.index < 0 || mInfo.index >= mmapInfoArrSize) {
			/* out of range */
			return -EFAULT;
		}
		mInfo.addr = (mv_kmod_uintptr_t)mmapInfoArr[mInfo.index].addr;
		mInfo.length = (mv_kmod_size_t)mmapInfoArr[mInfo.index].length;
		mInfo.offset = (mv_kmod_size_t)mmapInfoArr[mInfo.index].offset;
		if (copy_to_user((struct GT_PCI_MMAP_INFO_STC *)arg, &mInfo, sizeof(mInfo))) {
			printk(KERN_ERR "copy_to_user failed\n");
			return -EFAULT;
		}
		break;

	case PRESTERA_IOC_GETVMA:
		memset(&vmaInfo, 0, sizeof(vmaInfo));
		vmaInfo.dmaBase = (mv_kmod_uintptr_t)dma_base_vma_start;
		vmaInfo.ppConfigBase = (mv_kmod_uintptr_t)pci_conf_vma_start;
		vmaInfo.ppRegsBase = (mv_kmod_uintptr_t)pci_regs_vma_start;
		vmaInfo.ppDfxBase = (mv_kmod_uintptr_t)pci_dfx_vma_start;
		if (copy_to_user((struct GT_PCI_VMA_ADDRESSES_STC *)arg, &vmaInfo, sizeof(vmaInfo))) {
			printk(KERN_ERR "copy_to_user failed\n");
			return -EFAULT;
		}
		break;

	default:
		printk(KERN_WARNING "Unknown ioctl (%d)\n", _IOC_NR(cmd));
		return -EFAULT;
	}
	return 0;
}

/*
 * open and close: just keep track of how many times the device is
 * mapped, to avoid releasing it.
 */

void prestera_vma_open(struct vm_area_struct *vma)
{
	dev_open_nr++;
}

void prestera_vma_close(struct vm_area_struct *vma)
{
	dev_open_nr--;
}

struct vm_operations_struct prestera_vm_ops = {
	.open	= prestera_vma_open,
	.close	= prestera_vma_close,
};



/************************************************************************
 *
 * prestera_do_mmap: Map physical address to userspace
 */
static int prestera_do_mmap(struct vm_area_struct *vma,
		unsigned long phys,
		unsigned long pageSize)
{
	int             rc;

	/* bind the prestera_vm_ops */
	vma->vm_ops = &prestera_vm_ops;

	/* VM_IO for I/O memory */
	vma->vm_flags |= VM_IO;

	/* disable caching on mapped memory */
	vma->vm_page_prot = pgprot_noncached(vma->vm_page_prot);
	vma->vm_private_data = prestera_dev;

	vma->vm_pgoff = phys >> PAGE_SHIFT;

	/* fix case when pageSize < length_param_of_mmap */
	vma->vm_end = vma->vm_start + pageSize;


	dprintk(KERN_INFO "%s: remap_pfn_range(0x%lx, 0x%lx, 0x%lx, 0x%lx)\n",
			__func__, (unsigned long)(vma->vm_start),
			(unsigned long)(phys >> PAGE_SHIFT),
			(unsigned long)pageSize,
			(unsigned long)vma->vm_page_prot);

	rc = remap_pfn_range(vma,
			vma->vm_start,
			phys >> PAGE_SHIFT,
			pageSize,
			vma->vm_page_prot);
	if (rc) {
		printk(KERN_INFO "remap_pfn_range(0x%lx) failed (rc=%d)\n", vma->vm_start, rc);
		return 1;
	}

	prestera_vma_open(vma);

	return 0;
}

/************************************************************************
 *
 * prestera_mmap_dyn: map to dynamic address (single process only
 *
 * Key is vma->vm_pgoff where bit:(31-PAGE_SHIFT) == 1
 */
static int prestera_mmap_dyn(struct file *file, struct vm_area_struct *vma)
{
	uint32_t busNo, devSel, funcNo, barNo;
	struct pp_dev  *ppdev;
	unsigned long   phys;
	unsigned long   pageSize = 0;
	int i;

	busNo = (vma->vm_pgoff >> 10) & 0xff;
	devSel = (vma->vm_pgoff >> 5) & 0x1f;
	funcNo = (vma->vm_pgoff >> 2) & 0x07;
	barNo = vma->vm_pgoff & 0x03;
	if (busNo == 0xff && devSel == 0x1f && funcNo == 0x07) {
		/* xCat */
		devSel = 0xff;
		funcNo = 0xff;
	}
	i = find_pp_device(busNo, devSel, funcNo);
	if (i < 0) {
		printk(KERN_ERR "mvPP device 0x%02x:%02x.%x not found\n",
				busNo, devSel, funcNo);
		return -ENXIO;
	}
	ppdev = ppdevs[i];

	pageSize = vma->vm_end - vma->vm_start;

	switch (barNo) {
	case 0: /* config */
		if (pageSize < ppdev->config.size + ppdev->config.mmapoffset) {
			printk(KERN_ERR "No enough address space for config: 0x%lx, required 0x%lx\n",
					(unsigned long)pageSize,
					(unsigned long)(ppdev->config.size + ppdev->config.mmapoffset));
			return -ENXIO;
		}
		phys = ppdev->config.phys;
		pageSize = ppdev->config.size;
		ppdev->config.mmapbase = vma->vm_start;
		vma->vm_start += ppdev->config.mmapoffset;
		ppdev->config.mmapsize = pageSize;
		break;
	case 1: /* regs */
		phys = ppdev->ppregs.phys;
		ppdev->ppregs.mmapbase = vma->vm_start;
		ppdev->ppregs.mmapsize = pageSize;
		break;
	case 2: /* dfx */
		if (!ppdev->dfx.phys)
			return -ENXIO; /* ignore */
		if (pageSize < ppdev->dfx.size) {
			printk(KERN_ERR "No enough address space for dfx: 0x%lx, required 0x%lx\n",
					(unsigned long)pageSize,
					(unsigned long)ppdev->dfx.size);
			return -ENXIO;
		}
		phys = ppdev->dfx.phys;
		pageSize = ppdev->dfx.size;
		ppdev->dfx.mmapbase = vma->vm_start;
		ppdev->dfx.mmapsize = pageSize;
		break;
	default:
		printk(KERN_ERR "bad bar: %d\n", barNo);
		return -ENXIO;
	}

	return prestera_do_mmap(vma, phys, pageSize);
}

/************************************************************************
 *
 * prestera_mmap: The device mmap() implementation
 */
static int prestera_mmap(struct file *file, struct vm_area_struct *vma)
{
	unsigned long	phys = 0;
	unsigned long	pageSize = 0;
	struct pp_dev	*ppdev;
	int		i;

	if ((vma->vm_pgoff & (1<<(31-PAGE_SHIFT))) != 0)
		return prestera_mmap_dyn(file, vma);

	if (((vma->vm_pgoff) << PAGE_SHIFT) & (PAGE_SIZE - 1)) {
		/* need aligned offsets */
		printk(KERN_ERR "prestera_mmap offset not aligned\n");
		return -ENXIO;
	}

#define D mmapInfoArr[i]
	/* search for mapping */
	for (i = 0; i < mmapInfoArrSize; i++) {
		if (vma->vm_start == D.addr + D.offset)
			break;
	}
	if (i == mmapInfoArrSize) {
		printk(KERN_ERR "unknown range (0%0x)\n", (int)vma->vm_start);
		return 1;
	}
	ppdev = ppdevs[D.index];

	pageSize = vma->vm_end - vma->vm_start;

	switch (D.map_type) {
	case MMAP_INFO_TYPE_DMA_E:
			phys = dma_base;
			pageSize = dma_len;

			break;
	case MMAP_INFO_TYPE_PP_CONF_E:
			phys = ppdev->config.phys;
			pageSize = ppdev->config.mmapsize;

			if (pageSize > vma->vm_end - vma->vm_start)
				pageSize = vma->vm_end - vma->vm_start;

			break;

	case MMAP_INFO_TYPE_PP_REGS_E:
		phys = ppdev->ppregs.phys;
		pageSize = ppdev->ppregs.mmapsize;
		break;

	case MMAP_INFO_TYPE_PP_DFX_E:
		phys = ppdev->dfx.phys;
		pageSize = ppdev->dfx.mmapsize;
		break;

	default:
		printk(KERN_WARNING "Unknown map_type\n");
		return -EFAULT;
	}

	return prestera_do_mmap(vma, phys, pageSize);
}

/************************************************************************
 *
 * prestera_open: The device open() implementation
 */
static int prestera_open(struct inode *inode, struct file *filp)
{
	if (down_interruptible(&prestera_dev->sem))
		return -ERESTARTSYS;


	if (!dev_init_done) {
		up(&prestera_dev->sem);
		return -EIO;
	}

#ifndef SHARED_MEMORY
	/* Avoid single-usage restriction for shared memory:
	 * device should be accessible for multiple clients. */
	if (dev_open_nr) {
		up(&prestera_dev->sem);
		return -EBUSY;
	}
#endif

	filp->private_data = prestera_dev;

	dev_open_nr++;
	up(&prestera_dev->sem);

	printk(KERN_INFO "%s opened\n", prestera_dev_name);

	return 0;
}

/************************************************************************
 *
 * prestera_release: The device close() implementation
 */
static int prestera_release(struct inode *inode, struct file *file)
{
	dev_open_nr--;

	if (dev_open_nr == 0)
		prestera_int_cleanup();

	printk(KERN_DEBUG "%s released\n", prestera_dev_name);

	return 0;
}

/************************************************************************
 *
 * proc read data rooutine
 */
static inline u32 dbgread_u32(void *addr)
{
#if defined EXT_ARCH_64_CPU || defined CONFIG_ARM
	return readl((void *)addr);
#else
	return *(u32 *)addr;
#endif
}

struct dumpregs_stc {
	const char *nm;
	unsigned start;
	unsigned end;
};

#ifdef CONFIG_OF
static int dumpregs(struct seq_file *m, struct mem_region *mem, struct dumpregs_stc *r)
{
	void *basePtr = (void *)mem->base;
	if (basePtr == NULL)
		basePtr = ioremap_nocache(mem->phys, PAGE_SIZE);
	for (; r->nm; r++) {
		int i, n;
		uintptr_t address;
		uint32_t  value;

		address = (uintptr_t)basePtr + (uintptr_t)r->start;
		for (i = r->start, n = 0; i <= r->end; i += 4) {
			value = dbgread_u32((void *)address);
			if (n == 0)
				seq_printf(m, "\t%s(0x%04x):", r->nm, i);
			seq_printf(m, " %08x", le32_to_cpu(value));
			n++;
			if (n == 4) {
				n = 0;
				if (i+4 <= r->end)
					seq_putc(m, '\n');
			}
			address += 4;
		}
		seq_putc(m, '\n');
	}

	if (mem->base == 0)
		iounmap(basePtr);
	return 0;
}
#else
static int dumpregs(char *page, int len, struct mem_region *mem, struct dumpregs_stc *r)
{
	void *basePtr = (void *)mem->base;
	if (basePtr == NULL)
		basePtr = ioremap_nocache(mem->phys, PAGE_SIZE);
	for (; r->nm; r++) {
		int i, n;
		uintptr_t address;
		uint32_t  value;

		address = (uintptr_t)basePtr + (uintptr_t)r->start;
		for (i = r->start, n = 0; i <= r->end; i += 4) {
			value = dbgread_u32((void *)address);
			if (n == 0)
				len += sprintf(page+len, "\t%s(0x%04x):", r->nm, i);
			len += sprintf(page+len, " %08x", le32_to_cpu(value));
			n++;
			if (n == 4) {
				n = 0;
				if (i+4 <= r->end)
					page[len++] = '\n';
			}
			address += 4;
		}
		page[len++] = '\n';
	}

	if (mem->base == 0)
		iounmap(basePtr);
	return len;
}
#endif

static struct dumpregs_stc ppConf[] = {
	{ "\toff",  0,   0x10 },
#if 0
	/* No idea what are these registers,
	 * not found in Lion2, Bobcat2 specs
	 */
	{ "MASK", 0x118, 0x118 },
	{ "MASK", 0x114, 0x114 }, /*cause */
#endif
	{ NULL, 0, 0 }
};

static struct dumpregs_stc ppRegs[] = {
	{ "\toff",         0,         0 },
	{ "\toff",      0x4c,      0x4c },
	{ "\toff",      0x50,      0x50 },
#if 0
	{ "\toff", 0x1000000, 0x1000000 },
#ifndef EXT_ARCH_64_CPU
	{ "\toff", 0x2000000, 0x2000000 },
	{ "\toff", 0x3000000, 0x3000000 },
#endif
#endif
	{ NULL, 0, 0 }
};

#ifndef CONFIG_OF
int prestera_read_proc_mem(char		*page,
			   char		**start,
			   off_t	offset,
			   int		count,
			   int		*eof,
			   void		*data)
{
	int		len;
	struct pp_dev	*ppdev;
	int             i;

	len = 0;

	len += sprintf(page + len, "%s major # %d\n", prestera_dev_name, prestera_major);
	len += sprintf(page + len, "short_bh_count %d\n", prestera_int_bh_cnt_get());
	len += sprintf(page + len, "rx_DSR %d\n", prestera_smi_eth_port_rx_dsr_cnt());
	len += sprintf(page + len, "tx_DSR %d\n", prestera_smi_eth_port_tx_dsr_cnt());

	len += sprintf(page + len, "DMA area: 0x%lx(virt), base: 0x%lx(phys), len: 0x%x\n",
			(unsigned long)dma_area, dma_base, dma_len);

	for (i = 0; i < founddevs; i++) {
		ppdev = ppdevs[i];

		len += sprintf(page + len, "Device %d\n", i);
		len += sprintf(page+len, "\tPCI %02x:%02x.%x  vendor:dev=%04x:%04x\n",
				(unsigned)ppdev->busNo, (unsigned)ppdev->devSel, (unsigned)ppdev->funcNo,
				ppdev->vendorId, ppdev->devId);

		len += sprintf(page + len, "\tconfig 0x%lx(user virt), phys: 0x%lx, len: 0x%lx\n",
				ppdev->config.mmapbase, ppdev->config.phys, ppdev->config.size);

		len = dumpregs(page, len, &(ppdev->config), ppConf);

		len += sprintf(page + len, "\tppregs 0x%lx(user virt), phys: 0x%lx, len: 0x%lx\n",
				ppdev->ppregs.mmapbase, ppdev->ppregs.phys, ppdev->ppregs.size);

		len = dumpregs(page, len, &(ppdev->ppregs), ppRegs);
	}

	*eof = 1;

	return len;
}
#endif

static const struct file_operations prestera_fops = {
	.llseek			= prestera_lseek,
	.read			= prestera_smi_read,
	.write			= prestera_smi_write,
#ifdef HAVE_UNLOCKED_IOCTL
	.unlocked_ioctl		= prestera_ioctl,
#else
	.ioctl  = prestera_ioctl,
#endif
	.mmap			= prestera_mmap,
	.open			= prestera_open,
	.release		= prestera_release
};

#ifdef PRESTERA_SYSCALLS
/************************************************************************
*
* syscall entries for fast calls
*
************************************************************************/
/* fast call to prestera_ioctl() */
asmlinkage long sys_prestera_ctl(unsigned int cmd, unsigned long arg)
{
#ifdef HAVE_UNLOCKED_IOCTL
	return prestera_ioctl(NULL, cmd, arg);
#else
	return prestera_ioctl(NULL, NULL, cmd, arg);
#endif
}

#define OWN_SYSCALLS 1

#ifdef __NR_SYSCALL_BASE
#  define __SYSCALL_TABLE_INDEX(name) (__NR_ ## name - __NR_SYSCALL_BASE)
#else
#  define __SYSCALL_TABLE_INDEX(name) (__NR_ ## name)
#endif

#define __TBL_ENTRY(name) { __SYSCALL_TABLE_INDEX(name), (long)sys_ ## name, 0 }
static struct {
	int entry_number;
	long own_entry;
	long saved_entry;
} prestera_syscall[OWN_SYSCALLS] = {
	__TBL_ENTRY(prestera_ctl)
};
#undef  __TBL_ENTRY

/*******************************************************************************
* prestera_syscall_init
*
* DESCRIPTION:
*       None
*
* COMMENTS:
*       None
*
*******************************************************************************/
static int prestera_syscall_init(void)
{
	int	k;
	long	*syscall_tbl;

	syscall_tbl = (long *)kallsyms_lookup_name("sys_call_table");

	if (syscall_tbl == NULL) {
		printk(KERN_ALERT "%s failed to get address of sys_call_table\n", __func__);
		return -EFAULT;
	}

	for (k = 0; k < OWN_SYSCALLS; k++) {
		prestera_syscall[k].saved_entry = syscall_tbl[prestera_syscall[k].entry_number];
		syscall_tbl[prestera_syscall[k].entry_number] = prestera_syscall[k].own_entry;
	}
	return 0;
}

/*******************************************************************************
* prestera_RestoreSyscalls
*
* DESCRIPTION:
*       Restore original syscall entries.
*
* INPUTS:
*       None
*
* OUTPUTS:
*       None
*
* RETURNS:
*       None
*
* COMMENTS:
*       None
*
*******************************************************************************/
static int prestera_syscall_restore(void)
{
	int	k;
	long	*syscall_tbl;

	syscall_tbl = (long *)kallsyms_lookup_name("sys_call_table");

	if (syscall_tbl == NULL) {
		printk(KERN_ALERT "%s failed to get address of sys_call_table\n", __func__);
		return -EFAULT;
	}

	for (k = 0; k < OWN_SYSCALLS; k++) {
		if (prestera_syscall[k].saved_entry)
			syscall_tbl[prestera_syscall[k].entry_number] = prestera_syscall[k].saved_entry;
	}
	return 0;
}

#endif /* PRESTERA_SYSCALLS */



static int prestera_dma_init(void)
{
	dma_len  = _2M;
	dma_area = dma_alloc_coherent(NULL, dma_len, (dma_addr_t *)&dma_base,
				      GFP_DMA | GFP_KERNEL);

	if (!dma_area) {
		printk(KERN_ERR "dma_alloc_coherent() dma_area failed\n");
		return -ENOMEM;
	}

	dprintk("DMA - dma_area: %p(v), dma_base: 0x%lx(p), dma_len: 0x%x\n",
			dma_area, dma_base, dma_len);

	/* allocate temp area for bspDma operations */
	dma_tmp_virt = dma_alloc_coherent(NULL, PAGE_SIZE, &dma_tmp_phys,
					  GFP_DMA | GFP_KERNEL);

	if (!dma_tmp_virt) {
		printk(KERN_ERR "dma_alloc_coherent() failed\n");
		return -ENOMEM;
	}

	M.map_type = MMAP_INFO_TYPE_DMA_E;
	M.addr = dma_base_vma_start;
	dma_base_vma_end = dma_base_vma_start + dma_len;
	M.length = dma_len;
	M.offset = 0;
	mmapInfoArrSize++;

	return 0;
}

/************************************************************************
 *
 * prestera_cleanup:
 */
static void prestera_cleanup(void)
{
	int		i;
	struct pp_dev	*ppdev;

	dev_init_done = 0;

	prestera_int_cleanup();

	for (i = 0; i < founddevs; i++) {
		ppdev = ppdevs[i];
#ifdef PRESTERA_PP_DRIVER
		if (ppdev->ppdriver) /* destroy driver */
			ppdev->ppdriver(ppdev->ppdriverData, NULL);
#endif /* PRESTERA_PP_DRIVER */
		kfree(ppdev);
	}
	founddevs = 0;

	if (dma_tmp_virt) {
		dma_free_coherent(NULL, PAGE_SIZE, dma_tmp_virt, dma_tmp_phys);
		dma_tmp_virt = NULL;
	}

	if (dma_area) {
		dma_free_coherent(NULL, dma_len, (dma_addr_t *)&dma_base,
				  GFP_DMA | GFP_KERNEL);
		dma_area = NULL;
	}

#ifdef PRESTERA_SYSCALLS
	prestera_syscall_restore();
#endif
	remove_proc_entry(prestera_dev_name, NULL);

	cdev_del(&prestera_cdev);

	unregister_chrdev_region(MKDEV(prestera_major, 0), 1);
}

static uint32_t get_instance(unsigned short vendorId, unsigned short devId)
{
	static uint32_t bobcat2_instance;
	static uint32_t lion2_instance;
	static uint32_t ac3_instance;
	uint32_t instance = 0;

	switch (devId) {
	case MV_BOBCAT2_DEV_ID: {
		instance = bobcat2_instance;
		bobcat2_instance++;
		break;
		}
	case MV_LION2_DEV_ID:{
		instance = lion2_instance;
		lion2_instance++;
		break;
		}
	case MV_ALLEYCAT3_DEV_ID:{
		instance = ac3_instance;
		ac3_instance++;
		break;
		}
	default:
		instance = -1;
	}
	return instance;
}

/*
 * XXX: probably not need to distinguish instance of bobcat, lion etc, maybe
 * it will be enough for CPSS to remove instance variable and assign founddevs
 * to ppdev->instance - will be check during multi-switch configuration tests
 */
int ppdev_conf_set(struct pci_dev *pdev, struct pp_dev *ppdev)
{
	uint32_t instance;
	int32_t quirksInstance;
	int start, len;

	dprintk("%s\n", __func__);

	instance = get_instance(ppdev->vendorId, ppdev->devId);
	if (instance == (-1)) {
		printk(KERN_ERR "%s: instance get failed\n", __func__);
		return -ENODEV;
	}

	ppdev->instance = instance;

	quirksInstance = get_quirks(ppdev->devId);
	if (quirksInstance == (-1)) {
		printk(KERN_ERR "%s: quirksInstance get failed\n", __func__);
		return -ENODEV;
	}

	if (quirks[quirksInstance].hasDfx == PCI_DEV_DFX_EN) {
		/* Has DFX */
		ppdev->dfx.phys = ppdev->ppregs.phys + _64M;
		ppdev->dfx.size = _1M;
		if (ppdev->ppregs.base != 0)
			ppdev->dfx.base = ppdev->ppregs.base + _64M;
	}

	if (quirks[quirksInstance].configOffset != 0) {
		if (ppdev->config.allocsize > quirks[quirksInstance].configOffset) {
			start = ppdev->config.phys;
			len    = ppdev->config.size;

			start += quirks[quirksInstance].configOffset;
			len   -= quirks[quirksInstance].configOffset;
			if (ppdev->config.base)
				ppdev->config.base += quirks[quirksInstance].configOffset;

			ppdev->config.phys = start;
			ppdev->config.size = len;

			ppdev->config.mmapoffset = quirks[quirksInstance].configOffset;
		}
	} else {
			/* INTER_REGS address space mapping */
			/* ppdev->config.mmapoffset = 0; */
	}

	/* Only for debug purpose */
	dprintk("%s:pdev: devId 0x%x, vendorId 0x%x, instance 0x%lx\n",
			__func__, ppdev->devId, ppdev->vendorId, ppdev->instance);
	dprintk("pdev: busNo 0x%lx, devSel 0x%lx, funcNo 0x%lx\n",
			ppdev->busNo, ppdev->devSel, ppdev->funcNo);
	dprintk("ppregs:  allocbase 0x%lx, allocsize 0x%lx, size 0x%lx\n",
			ppdev->ppregs.allocbase, ppdev->ppregs.allocsize, ppdev->ppregs.size);
	dprintk("ppregs: phys 0x%lx, mmapoffset 0x%x\n",
			ppdev->ppregs.phys, ppdev->ppregs.mmapoffset);
	dprintk("config: phys 0x%lx, mmapoffset 0x%x\n",
			ppdev->config.phys, ppdev->config.mmapoffset);

	/* Add device to ppdevs */
	ppdevs[founddevs++] = ppdev;
	dprintk("num of devices %d\n", founddevs);

	prestera_ppdev = quirks[quirksInstance].pciId;

	return 0;
}
#ifdef CONFIG_OF
static int proc_status_show(struct seq_file *m, void *v)
{
	int		i;
	struct pp_dev	*ppdev;

	seq_printf(m, "%s major # %d\n", prestera_dev_name, prestera_major);
	seq_printf(m, "short_bh_count %d\n", prestera_int_bh_cnt_get());
	seq_printf(m, "rx_DSR %d\n", prestera_smi_eth_port_rx_dsr_cnt());
	seq_printf(m, "tx_DSR %d\n", prestera_smi_eth_port_tx_dsr_cnt());

	seq_printf(m, "DMA area: 0x%lx(virt), base: 0x%lx(phys), len: 0x%x\n",
			(unsigned long)dma_area, dma_base, dma_len);

	for (i = 0; i < founddevs; i++) {
		ppdev = ppdevs[i];

		seq_printf(m, "Device %d\n", i);
		seq_printf(m, "\tPCI %02x:%02x.%x  vendor:dev=%04x:%04x\n",
				(unsigned)ppdev->busNo, (unsigned)ppdev->devSel, (unsigned)ppdev->funcNo,
				ppdev->vendorId, ppdev->devId);

		seq_printf(m, "\tconfig 0x%lx(user virt), phys: 0x%lx, len: 0x%lx\n",
				ppdev->config.mmapbase, ppdev->config.phys, ppdev->config.size);

		dumpregs(m, &(ppdev->config), ppConf);

		seq_printf(m, "\tppregs 0x%lx(user virt), phys: 0x%lx, len: 0x%lx\n",
				ppdev->ppregs.mmapbase, ppdev->ppregs.phys, ppdev->ppregs.size);

		dumpregs(m, &(ppdev->ppregs), ppRegs);
	}

	return 0;
}

static int proc_status_open(struct inode *inode, struct file *file)
{
	return single_open(file, proc_status_show, PDE_DATA(inode));
}

static const struct file_operations prestera_read_proc_operations = {
	.open		= proc_status_open,
	.read		= seq_read,
	.llseek		= seq_lseek,
	.release	= seq_release,
};

#endif

/************************************************************************
 *
 * prestera_init:
 */
int prestera_init(void)
{
	int rc = 0;

	dprintk("%s\n", __func__);

	/* If already initialized skip it */
	if (dev_init_done == 1) {
		dprintk("%s: already initialized\n", __func__);
		return 0;
	}

	/* init static vars */
	dev_open_nr = 0;
	dma_tmp_virt = NULL;
	dma_base = 0;
	dma_len = 0;
	dma_area = NULL;

	/* first thing register the device at OS */

	/* Register your major. */
	rc = register_chrdev_region(MKDEV(prestera_major, 0), 1, prestera_dev_name);
	if (rc < 0) {
		printk(KERN_ERR "%s: register_chrdev_region err= %d\n", __func__, rc);
		return rc;
	}

	cdev_init(&prestera_cdev, &prestera_fops);

	prestera_cdev.owner = THIS_MODULE;

	rc = cdev_add(&prestera_cdev, MKDEV(prestera_major, 0), 1);
	if (rc) {
		unregister_chrdev_region(MKDEV(prestera_major, 0), 1);
		printk(KERN_ERR "%s: cdev_add err= %d\n", __func__, rc);
		return rc;
	}

	prestera_dev = kmalloc(sizeof(struct prestera_device), GFP_KERNEL);
	if (!prestera_dev) {
		printk(KERN_ERR "prestera_dev: Failed allocating memory for device\n");
		rc = -ENOMEM;
		goto fail;
	}

#ifdef PRESTERA_SYSCALLS
	rc = prestera_syscall_init();
	if (0 != rc)
		goto fail;
#endif

	/* create /proc entry */
#ifdef CONFIG_OF
	if (!proc_create(prestera_dev_name, S_IRUGO, NULL, &prestera_read_proc_operations))
		return -ENOMEM;
#else
	create_proc_read_entry(prestera_dev_name, 0, NULL, prestera_read_proc_mem, NULL);
#endif

	/* initialize the device main semaphore */
	sema_init(&prestera_dev->sem, 1);

	prestera_int_init();

	rc = prestera_smi_init();
	if (0 != rc)
		goto fail;

	rc = prestera_dma_init();
	if (0 != rc)
		goto fail;

	dev_init_done = 1;

	printk(KERN_INFO "%s driver initialized\n", prestera_dev_name);

	return 0;

fail:
	prestera_cleanup();

	printk(KERN_ERR "%s driver init failed, rc=%d\n", prestera_dev_name, rc);

	return rc;
}

module_param(prestera_major, int, S_IRUGO);

