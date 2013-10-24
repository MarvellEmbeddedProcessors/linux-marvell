/*
 * Copyright (C) 2007, 2008, Marvell International Ltd.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms and conditions of the GNU General Public License,
 * version 2, as published by the Free Software Foundation.
 *
 * This program is distributed in the hope it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
 * for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software Foundation,
 * Inc., 51 Franklin St - Fifth Floor, Boston, MA 02110-1301 USA.
 */

#ifndef MV_MEMCPY_H
#define MV_MEMCPY_H

#include <linux/types.h>
#include <linux/io.h>
#include <linux/dmaengine.h>
#include <linux/interrupt.h>

#define USE_TIMER
#define MV_MEMCPY_POOL_SIZE		PAGE_SIZE
#define MV_MEMCPY_SLOT_SIZE		64
#define MV_MEMCPY_THRESHOLD		1

#define MEMCPY_OPERATION_MODE_MEMCPY	2

#define MEMCPY_CURR_DESC(chan)	(chan->base + 0x210 + (chan->idx * 4))
#define MEMCPY_NEXT_DESC(chan)	(chan->base + 0x200 + (chan->idx * 4))
#define MEMCPY_BYTE_COUNT(chan)	(chan->base + 0x220 + (chan->idx * 4))
#define MEMCPY_DEST_POINTER(chan)	(chan->base + 0x2B0 + (chan->idx * 4))
#define MEMCPY_BLOCK_SIZE(chan)	(chan->base + 0x2C0 + (chan->idx * 4))
#define MEMCPY_INIT_VALUE_LOW(chan)	(chan->base + 0x2E0)
#define MEMCPY_INIT_VALUE_HIGH(chan)	(chan->base + 0x2E4)

#define MEMCPY_CONFIG(chan)	(chan->base + 0x10 + (chan->idx * 4))
#define MEMCPY_ACTIVATION(chan)	(chan->base + 0x20 + (chan->idx * 4))
#define MEMCPY_INTR_CAUSE(chan)	(chan->base + 0x30)
#define MEMCPY_INTR_MASK(chan)	(chan->base + 0x40)
#define MEMCPY_ERROR_CAUSE(chan)	(chan->base + 0x50)
#define MEMCPY_ERROR_ADDR(chan)	(chan->base + 0x60)
#define MEMCPY_INTR_MASK_VALUE	0x3F7

#define WINDOW_BASE(w)		(0x250 + ((w) << 2))
#define WINDOW_SIZE(w)		(0x270 + ((w) << 2))
#define WINDOW_REMAP_HIGH(w)	(0x290 + ((w) << 2))
#define WINDOW_BAR_ENABLE(chan)	(0x240 + ((chan) << 2))

/* This structure describes MEMCPY descriptor size 64bytes */
struct mv_memcpy_desc {
	u32 status;		/* descriptor execution status */
	u32 crc32_result;	/* result of CRC-32 calculation */
	u32 desc_command;	/* type of operation to be carried out */
	u32 phy_next_desc;	/* next descriptor address pointer */
	u32 byte_count;		/* size of src/dst blocks in bytes */
	u32 phy_dest_addr;	/* destination block address */
	u32 phy_src_addr[8];	/* source block addresses */
	u32 reserved0;
	u32 reserved1;
};

struct mv_memcpy_chan {
	dma_cookie_t completed_cookie;
	void __iomem *base;
	unsigned int idx;
	dma_addr_t   dma_desc_pool;
	struct mv_memcpy_desc *dma_desc_pool_virt;
	unsigned int next_bank;
	int active_bank;
	unsigned int next_index;
	unsigned int num_descs[2];
	struct {
		struct dma_async_tx_descriptor	async_tx;
		dma_addr_t src_addr;
		dma_addr_t dst_addr;
		size_t     len;
	} buff_info[16];
};

/**
 * struct mv_memcpy_chan - internal representation of a MEMCPY channel
 * @completed_cookie: identifier for the most recently completed operation
 * @mmr_base: memory mapped register base
 * @idx: the index of the memcpy channel
 * @device: parent device
 * @common: common dmaengine channel object members
 * @dma_desc_pool: base of DMA descriptor region (DMA address)
 * @dma_desc_pool_virt: base of DMA descriptor region (CPU address)
 */
struct mv_memcpy_chans {
	struct mv_memcpy_device	*device;
	struct dma_chan         common;
	spinlock_t              lock; /* protects the descriptor slot pool */
	int                     num_channels;
	struct mv_memcpy_chan   chan[4];
	unsigned int            next_chan;
	unsigned int            coalescing;
};

/**
 * struct mv_memcpy_device - internal representation of a MEMCPY device
 * @pdev: Platform device
 * @id: HW MEMCPY Device selector
 * @common: embedded struct dma_device
 * @lock: serializes enqueue/dequeue operations to the descriptors pool
 */
struct mv_memcpy_device {
	struct platform_device  *pdev;
	struct dma_device       common;
	int                     num_engines;
	void __iomem	        *engine_base[2];
};

/* Stores certain registers during suspend to RAM */
struct mv_memcpy_save_regs {
	int memcpy_config;
	int interrupt_mask;
};

#define MV_MEMCPY_MIN_BYTE_COUNT (16)
#define MEMCPY_MAX_BYTE_COUNT    ((16 * 1024 * 1024) - 1)
#define MV_MEMCPY_MAX_BYTE_COUNT MEMCPY_MAX_BYTE_COUNT

#endif
