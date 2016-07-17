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

#ifndef __mv_hw_if_h__
#define __mv_hw_if_h__

#include <linux/kernel.h>
#include <linux/io.h>
#include <linux/errno.h>
#include <linux/interrupt.h>
#include <linux/platform_device.h>
#include <linux/dma-mapping.h>

extern bool coherency_hard_mode;
/*
	access_addr - absolute address: Silicon base + unit base + table base + entry offset
	words_num   - number of words (word = 32 bits) to read
	data_ptr    - pointer to an array containing the read data
*/
static inline void mv_pp3_hw_read(void __iomem *access_addr, int words_num, u32 *data_ptr)
{
	int i;

	for (i = 0; i < words_num; i++)
		data_ptr[i] = readl(access_addr + (4 * i));
}

/*
	access_addr - absolute address: Silicon base + unit base + table base + entry offset
	words_num   - number of words (word = 32 bits) to write
	data_ptr    - pointer to an array containing the write data
*/
static inline void mv_pp3_hw_write(void __iomem *access_addr, int words_num, u32 *data_ptr)
{
	int i;

	for (i = 0; i < words_num; i++)
		writel(data_ptr[i], access_addr + (4 * i));

	/* WA for MBUS issue occurs when read interrupts multiple write transactions */
	/* Occurs for write tables with line width more than 32 bits */
	if (words_num > 1)
		readl(access_addr + 4 * (words_num - 1));
}

/*
	access_addr - absolute address: Silicon base + unit base + register offset
	return register value
*/
static inline u32 mv_pp3_hw_reg_read(void __iomem *access_addr)
{
	return readl(access_addr);
}

/*
	access_addr - absolute address: Silicon base + unit base + register offset
	write data to register
*/
static inline void mv_pp3_hw_reg_write(void __iomem *access_addr, u32 data)
{
#ifdef PP3_DEBUG
	pr_info("\nwrite reg %p, data 0x%x", access_addr, data);
#endif
	writel(data, access_addr);
}

/* Cache coherency functions */
static inline void mv_pp3_os_cache_io_sync(void *handle)
{
#ifndef CONFIG_MV_PP3_COHERENCY_HARD_MODE_ONLY
	if (likely(coherency_hard_mode))
#endif
		dma_sync_single_for_cpu(handle, (dma_addr_t) NULL,
			(size_t) NULL, DMA_FROM_DEVICE);
}

static inline dma_addr_t mv_pp3_os_dma_map_single(struct device *dev, void *addr, size_t size, int direction)
{
#ifndef CONFIG_MV_PP3_COHERENCY_HARD_MODE_ONLY
	if (unlikely(!coherency_hard_mode))
		return dma_map_single(dev, addr, size, direction);
#endif
	return virt_to_phys(addr);
}

static inline dma_addr_t mv_pp3_os_dma_map_page(struct device *dev, struct page *page, int offset,
						size_t size, int direction)
{
#ifndef CONFIG_MV_PP3_COHERENCY_HARD_MODE_ONLY
	if (unlikely(!coherency_hard_mode))
		return dma_map_page(dev, page, offset, size, direction);
#endif
	return pfn_to_dma(dev, page_to_pfn(page)) + offset;
}

#endif /* __mv_hw_if_h__ */
