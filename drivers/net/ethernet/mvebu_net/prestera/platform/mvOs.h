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
*******************************************************************************/
#ifndef _MV_OS_LNX_H_
#define _MV_OS_LNX_H_

#include <linux/uaccess.h>
#include <linux/unistd.h>
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/cdev.h>
#include <linux/proc_fs.h>
#include <linux/irq.h>
#include <linux/interrupt.h>
#include <linux/dma-mapping.h>
#include <linux/version.h>
#include <linux/sched.h>
#include <linux/syscalls.h>
#include <linux/signal.h>
#include <linux/kallsyms.h>
#include <linux/stddef.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/errno.h>
#include <linux/reboot.h>
#include <linux/pci.h>
#include <linux/kdev_t.h>
#include <linux/major.h>
#include <linux/blkdev.h>
#include <linux/console.h>
#include <linux/delay.h>
#include <linux/seq_file.h>
#include <linux/string.h>
#include <linux/slab.h>
#include <linux/kernel.h>
#include <linux/string.h>
#include <linux/slab.h>
#include <linux/mm.h>
#include <linux/random.h>
#include <linux/semaphore.h>
#include <linux/time.h>
#include <linux/sysctl.h>
#include <pci/pci.h>
#include "mvTypes.h"

/* Endianess macros.                                                        */
#if defined(MV_CPU_LE)
#define MV_16BIT_LE(X)  (X)
#define MV_32BIT_LE(X)  (X)
#define MV_64BIT_LE(X)  (X)
#define MV_16BIT_BE(X)  MV_BYTE_SWAP_16BIT(X)
#define MV_32BIT_BE(X)  MV_BYTE_SWAP_32BIT(X)
#define MV_64BIT_BE(X)  MV_BYTE_SWAP_64BIT(X)
#elif defined(MV_CPU_BE)
#define MV_16BIT_LE(X)  MV_BYTE_SWAP_16BIT(X)
#define MV_32BIT_LE(X)  MV_BYTE_SWAP_32BIT(X)
#define MV_64BIT_LE(X)  MV_BYTE_SWAP_64BIT(X)
#define MV_16BIT_BE(X)  (X)
#define MV_32BIT_BE(X)  (X)
#define MV_64BIT_BE(X)  (X)
#else
#error "CPU endianess isn't defined!\n"
#endif

#define MV_ETH_BASE_ADDR			(0x70000)
#define MV_ETH_REGS_OFFSET(port)		(MV_ETH_BASE_ADDR + (port) * 0x4000)
#define MV_ETH_REGS_BASE(p)			MV_ETH_REGS_OFFSET(p)
#define ETH_SMI_REG(port)			(MV_ETH_REGS_BASE(port) + 0x004)

#define MV_32BIT_LE_FAST(val) \
				MV_32BIT_LE(val)

#define MV_MEMIO32_READ(addr) \
				((*((unsigned int *)(addr))))

#define MV_MEMIO32_WRITE(addr, data) \
				((*((unsigned int *)(addr))) = ((unsigned int)(data)))

#define MV_MEMIO_LE32_WRITE(addr, data) \
				MV_MEMIO32_WRITE(addr, MV_32BIT_LE_FAST(data))

static inline unsigned int MV_MEMIO_LE32_READ(unsigned int addr)
{
	unsigned int data;

	data = (unsigned int)MV_MEMIO32_READ(addr);

	return (unsigned int)MV_32BIT_LE_FAST(data);
}

#define MV_REG_READ(offset) \
				(MV_MEMIO_LE32_READ(INTER_REGS_VIRT_BASE | (offset)))

#define MV_REG_WRITE(offset, val) \
				MV_MEMIO_LE32_WRITE((INTER_REGS_VIRT_BASE | (offset)), (val))

#endif /* _MV_OS_LNX_H_ */
