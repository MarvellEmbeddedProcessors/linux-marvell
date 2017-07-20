/*
 * Copyright (C) 2015-2016 Marvell International Ltd.

 * This program is free software: you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation, either version 2 of the
 * License, or any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 */

#include <linux/clk.h>
#include <linux/dma-mapping.h>
#include <linux/interrupt.h>
#include <linux/io.h>
#include <linux/module.h>
#include <linux/msi.h>
#include <linux/of.h>
#include <linux/of_irq.h>
#include <linux/platform_device.h>
#include <linux/spinlock.h>

#include "dmaengine.h"

/* DMA Engine Registers */
#define DMA_DESQ_BALR_OFF	0x000
#define DMA_DESQ_BAHR_OFF	0x004
#define DMA_DESQ_SIZE_OFF	0x008
#define DMA_DESQ_DONE_OFF	0x00C
#define   DMA_DESQ_DONE_PENDING_MASK		0x7FFF
#define   DMA_DESQ_DONE_PENDING_SHIFT		0
#define   DMA_DESQ_DONE_READ_PTR_MASK		0x1FFF
#define   DMA_DESQ_DONE_READ_PTR_SHIFT		16
#define DMA_DESQ_ARATTR_OFF 	0x010
#define   DMA_DESQ_ATTR_CACHE_MASK		0x3F3F
#define   DMA_DESQ_ATTR_OUTER_SHAREABLE		0x202
#define   DMA_DESQ_ATTR_CACHEABLE		0x3C3C
#define DMA_IMSG_CDAT_OFF	0x014
#define DMA_IMSG_THRD_OFF	0x018
#define   DMA_IMSG_THRD_MASK			0x7FFF
#define   DMA_IMSG_THRD_SHIFT			0x0
#define   DMA_IMSG_TIMER_MASK			0x1
#define   DMA_IMSG_TIMER_SHIFT			18
#define DMA_DESQ_AWATTR_OFF	0x01C
  /* Same flags as DMA_DESQ_ARATTR_OFF */
#define DMA_DESQ_ALLOC_OFF	0x04C
#define   DMA_DESQ_ALLOC_WRPTR_MASK		0xFFFF
#define   DMA_DESQ_ALLOC_WRPTR_SHIFT		16
#define DMA_IMSG_BALR_OFF	0x050
#define DMA_IMSG_BAHR_OFF	0x054
#define DMA_DESQ_CTRL_OFF	0x100
#define	  DMA_DESQ_CTRL_32B			1
#define   DMA_DESQ_CTRL_128B			7
#define DMA_DESQ_STOP_OFF	0x800
#define DMA_DESQ_DEALLOC_OFF	0x804
#define DMA_DESQ_ADD_OFF	0x808
#define DMA_IMSG_TMOT		0x810
#define   DMA_IMSG_TIMER_THRD_MASK		0x1FFF
#define   DMA_IMSG_TIMER_THRD_SHIFT		0x0

/* XOR Global registers */
#define GLOB_BW_CTRL		0x4
#define   GLOB_BW_CTRL_NUM_OSTD_RD_SHIFT	0
#define   GLOB_BW_CTRL_NUM_OSTD_RD_VAL		64
#define   GLOB_BW_CTRL_NUM_OSTD_WR_SHIFT	8
#define   GLOB_BW_CTRL_NUM_OSTD_WR_VAL		8
#define   GLOB_BW_CTRL_RD_BURST_LEN_SHIFT	12
#define   GLOB_BW_CTRL_RD_BURST_LEN_VAL		4
#define   GLOB_BW_CTRL_WR_BURST_LEN_SHIFT	16
#define   GLOB_BW_CTRL_WR_BURST_LEN_VAL      	4
#define GLOB_PAUSE		0x014
#define   GLOB_PAUSE_AXI_TIME_DIS_VAL		0x8
#define GLOB_SYS_INT_CAUSE	0x200
#define GLOB_SYS_INT_MASK	0x204
#define GLOB_MEM_INT_CAUSE	0x220
#define GLOB_MEM_INT_MASK	0x224

#define MV_XOR_V2_MIN_DESC_SIZE		32
#define MV_XOR_V2_EXT_DESC_SIZE		128

#define MV_XOR_V2_DESC_RESERVED_SIZE	12
#define MV_XOR_V2_DESC_BUFF_D_ADDR_SIZE	12

#define MV_XOR_V2_CMD_LINE_NUM_MAX_D_BUF 8

/* descriptors queue size */
#define MV_XOR_V2_MAX_DESC_NUM	1024

/* Threshold values for descriptors and timeout
** got this values after runnig benchmarks with RAID5
*/
#define MV_XOR_DONE_IMSG_THRD	0x14
#define MV_XOR_TIMER_THRD	0xB0

/**
 * struct mv_xor_v2_descriptor - DMA HW descriptor
 * @desc_id: used by S/W and is not affected by H/W.
 * @flags: error and status flags
 * @crc32_result: CRC32 calculation result
 * @desc_ctrl: operation mode and control flags
 * @buff_size: amount of bytes to be processed
 * @fill_pattern_src_addr: Source-Address, or Q-Buffer-Address
 * @fill_pattern_dst_addr: Destination-Address, or P-Buffer-Address
 * @data_buff_addr: Source (and might be RAID6 destination)
 * addresses of data buffers in RAID5 and RAID6
 * @reserved: reserved
 */
struct mv_xor_v2_descriptor {
	u16 desc_id;
	u16 flags;
	u32 crc32_result;
	u32 desc_ctrl;

	/* Definitions for desc_ctrl */
#define DESC_NUM_ACTIVE_D_BUF_SHIFT 	22
#define DESC_OP_MODE_SHIFT  		28
#define DESC_OP_MODE_NOP 		0	/* Idle operation */
#define DESC_OP_MODE_MEMCPY		1	/* Pure-DMA operation */
#define DESC_OP_MODE_MEMSET		2	/* Mem-Fill operation */
#define DESC_OP_MODE_MEMINIT		3	/* Mem-Init operation */
#define DESC_OP_MODE_MEM_COMPARE	4	/* Mem-Compare operation */
#define DESC_OP_MODE_CRC32		5	/* CRC32 calculation */
#define DESC_OP_MODE_XOR		6	/* RAID5 (XOR) operation */
#define DESC_OP_MODE_RAID6		7	/* RAID6 P&Q-generation */
#define DESC_OP_MODE_RAID6_REC		8	/* RAID6 Recovery */
#define DESC_Q_BUFFER_ENABLE		BIT(16)
#define DESC_P_BUFFER_ENABLE		BIT(17)
#define DESC_IOD			BIT(27)

	u32 buff_size;
	u32 fill_pattern_src_addr[2];
	u32 fill_pattern_dst_addr[2];
	u32 data_buff_addr[MV_XOR_V2_DESC_BUFF_D_ADDR_SIZE];
	u32 reserved[MV_XOR_V2_DESC_RESERVED_SIZE];
};

/**
 * struct mv_xor_v2_device - implements a xor device
 * @sw_ll_lock: serializes enqueue/dequeue operations to the sw
 * descriptors pool
 * @push_lock: serializes enqueue operations to the DESCQ
 * @dma_base: memory mapped DMA register base
 * @glob_base: memory mapped global register base
 * @irq_tasklet:
 * @free_sw_desc: linked list of free SW descriptors
 * @alloc_sw_desc: linked list of allocated SW descriptors
 * @complete_sw_desc: linked list of completed SW descriptors
 * @dmadev: dma device
 * @dmachan: dma channel
 * @hw_desq: HW descriptors queue
 * @hw_desq_virt: virtual address of DESCQ
 * @sw_desq: SW descriptors queue
 * @desc_size: HW descriptor size
 * @npendings: number of pending descriptors (for which tx_submit has
 * been called, but not yet issue_pending)
 * @hw_queue_idx: index of next HW descriptor to push to the queue
*/
struct mv_xor_v2_device {
	spinlock_t sw_ll_lock;
	spinlock_t push_lock;
	void __iomem *dma_base;
	void __iomem *glob_base;
	struct clk *clk;
	struct tasklet_struct irq_tasklet;
	struct list_head free_sw_desc;
	struct list_head alloc_sw_desc;
	struct list_head complete_sw_desc;
	struct dma_device dmadev;
	struct dma_chan	dmachan;
	dma_addr_t hw_desq;
	struct mv_xor_v2_descriptor *hw_desq_virt;
	struct mv_xor_v2_sw_desc *sw_desq;
	int desc_size;
	unsigned int npendings;
	int hw_queue_idx;
};

/**
 * struct mv_xor_v2_sw_desc - implements a xor SW descriptor
 * @idx: descriptor index
 * @async_tx: support for the async_tx api
 * @hw_desc: assosiated HW descriptor
 * @node: node of the SW descriprots (free_sw_desc or alloc_sw_desc or complete_sw_desc)
*/
struct mv_xor_v2_sw_desc {
	int idx;
	struct dma_async_tx_descriptor async_tx;
	struct mv_xor_v2_descriptor hw_desc;
	struct list_head node;
};

/*
 * Fill the data buffers to a HW descriptor
 */
static void mv_xor_v2_set_data_buffers(struct mv_xor_v2_device *xor_dev,
					struct mv_xor_v2_descriptor *desc,
					dma_addr_t src, int index)
{
	int arr_index = ((index >> 1) * 3);

	/*
	 * fill the buffer's addresses to the descriptor
	 * The format of the buffers address for 2 sequential buffers X and X+1:
	 *  First word: Buffer-DX-Address-Low[31:0]
	 *  Second word:Buffer-DX+1-Address-Low[31:0]
	 *  Third word: DX+1-Buffer-Address-High[47:32] [31:16]
	 *		DX-Buffer-Address-High[47:32] [15:0]
	 */
	if ((index & 0x1) == 0) {
		desc->data_buff_addr[arr_index] = lower_32_bits(src);

		/* Clear lower 16-bits */
		desc->data_buff_addr[arr_index + 2] &= ~0xFFFF;

		/* Set them if we have a 64 bits DMA address */
		if (IS_ENABLED(CONFIG_ARCH_DMA_ADDR_T_64BIT))
			desc->data_buff_addr[arr_index + 2] |=
				upper_32_bits(src) & 0xFFFF;
	} else {
		desc->data_buff_addr[arr_index + 1] =
			lower_32_bits(src);

		/* Clear upper 16-bits */
		desc->data_buff_addr[arr_index + 2] &= ~0xFFFF0000;

		/* Set them if we have a 64 bits DMA address */
		if (IS_ENABLED(CONFIG_ARCH_DMA_ADDR_T_64BIT))
			desc->data_buff_addr[arr_index + 2] |=
				(upper_32_bits(src) & 0xFFFF) << 16;
	}
}

/*
 * notify the engine of new descriptors, and update the available index.
 */
static void mv_xor_v2_add_desc_to_desq(struct mv_xor_v2_device *xor_dev,
				       int num_of_desc)
{
	/* write the number of new descriptors in the DESQ. */
	writel(num_of_desc, xor_dev->dma_base + DMA_DESQ_ADD_OFF);
}

/*
 * free HW descriptors
 */
static void mv_xor_v2_free_desc_from_desq(struct mv_xor_v2_device *xor_dev,
					  int num_of_desc)
{
	/* write the number of new descriptors in the DESQ. */
	writel(num_of_desc, xor_dev->dma_base + DMA_DESQ_DEALLOC_OFF);
}

/*
 * Set descriptor size
 * Return the HW descriptor size in bytes
 */
static int mv_xor_v2_set_desc_size(struct mv_xor_v2_device *xor_dev)
{
	writel(DMA_DESQ_CTRL_128B,
	       xor_dev->dma_base + DMA_DESQ_CTRL_OFF);

	return MV_XOR_V2_EXT_DESC_SIZE;
}

/*
 * Allocate resources for a channel
 */
static int mv_xor_v2_alloc_chan_resources(struct dma_chan *chan)
{
	/* nothing to be done here */
	return 0;
}

/*
 * Free resources of a channel
 */
void mv_xor_v2_free_chan_resources(struct dma_chan *chan)
{
	/* nothing to be done here */
}

/*
 * Set the IMSG threshold
 */
static inline
void mv_xor_v2_enable_imsg_thrd(struct mv_xor_v2_device *xor_dev)
{
	u32 reg;

	reg = readl(xor_dev->dma_base + DMA_IMSG_THRD_OFF);
	/* Configure threshold for IMSG */
	reg &= (~DMA_IMSG_THRD_MASK << DMA_IMSG_THRD_SHIFT);
	reg |= (MV_XOR_DONE_IMSG_THRD << DMA_IMSG_THRD_SHIFT);
	/* Enable IMSG-timer */
	reg &= ~(DMA_IMSG_TIMER_MASK << DMA_IMSG_TIMER_SHIFT);
	reg |= (0x1 << DMA_IMSG_TIMER_SHIFT);
	writel(reg, xor_dev->dma_base + DMA_IMSG_THRD_OFF);

	/* Configure Timer Threshold */
	reg = readl(xor_dev->dma_base + DMA_IMSG_TMOT);
	reg &= (~DMA_IMSG_TIMER_THRD_MASK << DMA_IMSG_TIMER_THRD_SHIFT);
	reg |= (MV_XOR_TIMER_THRD << DMA_IMSG_TIMER_THRD_SHIFT);
	writel(reg, xor_dev->dma_base + DMA_IMSG_TMOT);
}

static irqreturn_t mv_xor_v2_interrupt_handler(int irq, void *data)
{
	struct mv_xor_v2_device *xor_dev = data;

	/* schedule a tasklet to handle descriptors callbacks */
	tasklet_schedule(&xor_dev->irq_tasklet);

	return IRQ_HANDLED;
}

/*
 * submit a descriptor to the DMA engine
 */
static dma_cookie_t
mv_xor_v2_tx_submit(struct dma_async_tx_descriptor *tx)
{
	void *dest_hw_desc;
	dma_cookie_t cookie;
	struct mv_xor_v2_sw_desc *sw_desc =
		container_of(tx, struct mv_xor_v2_sw_desc, async_tx);
	struct mv_xor_v2_device *xor_dev =
		container_of(tx->chan, struct mv_xor_v2_device, dmachan);

	dev_dbg(xor_dev->dmadev.dev,
		"%s sw_desc %p: async_tx %p\n",
		__func__, sw_desc, &sw_desc->async_tx);

	/* lock enqueue DESCQ */
	spin_lock_bh(&xor_dev->push_lock);
	/* assign coookie */
	cookie = dma_cookie_assign(tx);

	/* copy the HW descriptor from the SW descriptor to the DESQ */
	dest_hw_desc = ((void *)xor_dev->hw_desq_virt +
			(xor_dev->desc_size * xor_dev->hw_queue_idx));
	/*
	 * Increase the push index for the HW queue and check if reach
	 * the end of HW buffer
	 */
	if (++xor_dev->hw_queue_idx >= MV_XOR_V2_MAX_DESC_NUM)
		xor_dev->hw_queue_idx = 0;

	memcpy(dest_hw_desc, &sw_desc->hw_desc, xor_dev->desc_size);

	xor_dev->npendings++;

	/* unlock enqueue DESCQ */
	spin_unlock_bh(&xor_dev->push_lock);

	return cookie;
}

/*
 * Prepare a SW descriptor
 */
static struct mv_xor_v2_sw_desc	*
mv_xor_v2_prep_sw_desc(struct mv_xor_v2_device *xor_dev)
{
	struct mv_xor_v2_sw_desc *sw_desc;

	/* Lock the channel */
	spin_lock_bh(&xor_dev->sw_ll_lock);

	if (!list_empty(&xor_dev->free_sw_desc)) {
		/* get a free SW descriptor from the free list */
		sw_desc = list_first_entry(&xor_dev->free_sw_desc,
					   struct mv_xor_v2_sw_desc, node);
		/* move the node from free list to allocated list */
		list_move_tail(&sw_desc->node, &xor_dev->alloc_sw_desc);

		/* Release the channel */
		spin_unlock_bh(&xor_dev->sw_ll_lock);

		/* set the async tx descriptor */
		dma_async_tx_descriptor_init(&sw_desc->async_tx, &xor_dev->dmachan);
		sw_desc->async_tx.tx_submit = mv_xor_v2_tx_submit;
		async_tx_ack(&sw_desc->async_tx);

		return sw_desc;
	}

	/* Release the channel */
	spin_unlock_bh(&xor_dev->sw_ll_lock);

	/* schedule tasklet to free some descriptors */
	tasklet_schedule(&xor_dev->irq_tasklet);

	/* no free descriptors */
	return NULL;
}

/*
 * Prepare a HW descriptor for a memcpy operation
 */
static struct dma_async_tx_descriptor *
mv_xor_v2_prep_dma_memcpy(struct dma_chan *chan, dma_addr_t dest, dma_addr_t src,
			  size_t len, unsigned long flags)
{
	struct mv_xor_v2_sw_desc	*sw_desc;
	struct mv_xor_v2_descriptor	*hw_descriptor;
	struct mv_xor_v2_device		*xor_dev;

	xor_dev = container_of(chan, struct mv_xor_v2_device, dmachan);

	dev_dbg(xor_dev->dmadev.dev,
		"%s len: %zu src %pad dest %pad flags: %ld\n",
		__func__, len, &src, &dest, flags);

	sw_desc = mv_xor_v2_prep_sw_desc(xor_dev);

	/* If there's no empty discriptor, return NULL to dma stack */
	if (!sw_desc)
		return NULL;

	sw_desc->async_tx.flags = flags;

	/* set the HW descriptor */
	hw_descriptor = &sw_desc->hw_desc;

	/* save the SW descriptor ID to restore when operation is done */
	hw_descriptor->desc_id = sw_desc->idx;

	/* Set the MEMCPY control word */
	hw_descriptor->desc_ctrl =
		DESC_OP_MODE_MEMCPY << DESC_OP_MODE_SHIFT;

	if (flags & DMA_PREP_INTERRUPT)
		hw_descriptor->desc_ctrl |= DESC_IOD;

	/* Set source address */
	hw_descriptor->fill_pattern_src_addr[0] = lower_32_bits(src);

	if (IS_ENABLED(CONFIG_ARCH_DMA_ADDR_T_64BIT))
		hw_descriptor->fill_pattern_src_addr[1] =
			upper_32_bits(src) & 0xFFFF;
	else
		hw_descriptor->fill_pattern_src_addr[1] = 0;

	/* Set Destination address */
	hw_descriptor->fill_pattern_dst_addr[0] = lower_32_bits(dest);

	if (IS_ENABLED(CONFIG_ARCH_DMA_ADDR_T_64BIT))
		hw_descriptor->fill_pattern_dst_addr[1] =
			upper_32_bits(dest) & 0xFFFF;
	else
		hw_descriptor->fill_pattern_dst_addr[1] = 0;

	/* Set buffers size */
	hw_descriptor->buff_size = len;

	/* return the async tx descriptor */
	return &sw_desc->async_tx;
}

/*
 * Prepare a HW descriptor for a XOR operation
 */
static struct dma_async_tx_descriptor *
mv_xor_v2_prep_dma_xor(struct dma_chan *chan, dma_addr_t dest, dma_addr_t *src,
		       unsigned int src_cnt, size_t len, unsigned long flags)
{
	struct mv_xor_v2_sw_desc	*sw_desc;
	struct mv_xor_v2_descriptor	*hw_descriptor;
	struct mv_xor_v2_device		*xor_dev;
	int i;

	WARN_ON(src_cnt > MV_XOR_V2_CMD_LINE_NUM_MAX_D_BUF || src_cnt < 1);

	xor_dev = container_of(chan, struct mv_xor_v2_device, dmachan);

	dev_dbg(xor_dev->dmadev.dev,
		"%s src_cnt: %d len: %zu dest %pad flags: %ld\n",
		__func__, src_cnt, len, &dest, flags);

	WARN_ON(xor_dev->desc_size != MV_XOR_V2_EXT_DESC_SIZE);

	sw_desc = mv_xor_v2_prep_sw_desc(xor_dev);

	/* If there's no empty discriptor, return NULL to dma stack */
	if (!sw_desc)
		return NULL;

	sw_desc->async_tx.flags = flags;

	/* set the HW descriptor */
	hw_descriptor = &sw_desc->hw_desc;

	/* save the SW descriptor ID to restore when operation is done */
	hw_descriptor->desc_id = sw_desc->idx;

	/* Set the XOR control word */
	hw_descriptor->desc_ctrl =
		DESC_OP_MODE_XOR << DESC_OP_MODE_SHIFT;
	hw_descriptor->desc_ctrl |= DESC_P_BUFFER_ENABLE;

	if (flags & DMA_PREP_INTERRUPT)
		hw_descriptor->desc_ctrl |= DESC_IOD;

	/* Set the data buffers */
	for (i = 0; i < src_cnt; i++)
		mv_xor_v2_set_data_buffers(xor_dev, hw_descriptor, src[i], i);

	hw_descriptor->desc_ctrl |=
		src_cnt << DESC_NUM_ACTIVE_D_BUF_SHIFT;

	/* Set Destination address */
	hw_descriptor->fill_pattern_dst_addr[0] = lower_32_bits(dest);
	if (IS_ENABLED(CONFIG_ARCH_DMA_ADDR_T_64BIT))
		hw_descriptor->fill_pattern_dst_addr[1] =
			upper_32_bits(dest) & 0xFFFF;
	else
		hw_descriptor->fill_pattern_dst_addr[1] = 0;

	/* Set buffers size */
	hw_descriptor->buff_size = len;

	/* return the async tx descriptor */
	return &sw_desc->async_tx;
}

/*
 * Prepare a HW descriptor for a XOR operation
 */
static struct dma_async_tx_descriptor *
mv_xor_v2_prep_dma_pq(struct dma_chan *chan, dma_addr_t *dest, dma_addr_t *src,
			unsigned int src_cnt, const unsigned char *scf,
			size_t len, unsigned long flags)
{
	struct mv_xor_v2_sw_desc	*sw_desc;
	struct mv_xor_v2_descriptor	*hw_descriptor;
	struct mv_xor_v2_device		*xor_dev;
	int i;

	WARN_ON(src_cnt > MV_XOR_V2_CMD_LINE_NUM_MAX_D_BUF || src_cnt < 1);

	xor_dev = container_of(chan, struct mv_xor_v2_device, dmachan);

	dev_dbg(xor_dev->dmadev.dev,
		"%s src_cnt: %d len: %zu dest %pad flags: %ld\n",
		__func__, src_cnt, len, &dest, flags);

	WARN_ON(xor_dev->desc_size != MV_XOR_V2_EXT_DESC_SIZE);

	/*
	 * Because of limitation in our XORv2 unit, XORv2 do not
	 * support mult and sum product requests. If Raid6 have double
	 * parity devices the scf array will include multiple and sum
	 * parameters for the equations, but in XORv2 unit does not
	 * support thus operations.
	 * return NULL to DMA stack, to let the CPU do the mult
	 * and SUM operations
	 */
	if ((flags & DMA_PREP_PQ_MULT) || (flags & DMA_PREP_PQ_SUM_PRODUCT))
		return NULL;

	sw_desc = mv_xor_v2_prep_sw_desc(xor_dev);

	/* If there's no empty discriptor, return NULL to dma stack */
	if (!sw_desc)
		return NULL;

	sw_desc->async_tx.flags = flags;

	/* set the HW descriptor */
	hw_descriptor = &sw_desc->hw_desc;

	/* save the SW descriptor ID to restore when operation is done */
	hw_descriptor->desc_id = sw_desc->idx;

	/* Set the XOR control word */
	hw_descriptor->desc_ctrl =
		DESC_OP_MODE_RAID6 << DESC_OP_MODE_SHIFT;

	if (flags & DMA_PREP_INTERRUPT)
		hw_descriptor->desc_ctrl |= DESC_IOD;

	hw_descriptor->desc_ctrl |=
		src_cnt << DESC_NUM_ACTIVE_D_BUF_SHIFT;

	if (!(flags & DMA_PREP_PQ_DISABLE_Q)) {
		hw_descriptor->desc_ctrl |= DESC_Q_BUFFER_ENABLE;

		/* Set Q-Buffer-Address */
		hw_descriptor->fill_pattern_src_addr[0] = lower_32_bits(dest[1]);
		if (IS_ENABLED(CONFIG_ARCH_DMA_ADDR_T_64BIT))
			hw_descriptor->fill_pattern_src_addr[1] =
				upper_32_bits(dest[1]) & 0xFFFF;
		else
			hw_descriptor->fill_pattern_src_addr[1] = 0;
	}

	if (!(flags & DMA_PREP_PQ_DISABLE_P)) {
		hw_descriptor->desc_ctrl |= DESC_P_BUFFER_ENABLE;

		/* Set P-Buffer-Address */
		hw_descriptor->fill_pattern_dst_addr[0] = lower_32_bits(dest[0]);
		if (IS_ENABLED(CONFIG_ARCH_DMA_ADDR_T_64BIT))
			hw_descriptor->fill_pattern_dst_addr[1] =
				upper_32_bits(dest[0]) & 0xFFFF;
		else
			hw_descriptor->fill_pattern_dst_addr[1] = 0;
	}

	/* Set the data buffers */
	for (i = 0; i < src_cnt; i++)
		mv_xor_v2_set_data_buffers(xor_dev, hw_descriptor, src[i], i);

	/* Set buffers size */
	hw_descriptor->buff_size = len;

	/* return the async tx descriptor */
	return &sw_desc->async_tx;
}

/*
 * Prepare a HW descriptor for interrupt operation.
 */
static struct dma_async_tx_descriptor *
mv_xor_v2_prep_dma_interrupt(struct dma_chan *chan, unsigned long flags)
{
	struct mv_xor_v2_sw_desc	*sw_desc;
	struct mv_xor_v2_descriptor	*hw_descriptor;
	struct mv_xor_v2_device		*xor_dev;

	xor_dev = container_of(chan, struct mv_xor_v2_device, dmachan);

	sw_desc = mv_xor_v2_prep_sw_desc(xor_dev);

	/* If there's no empty discriptor, return NULL to dma stack */
	if (!sw_desc)
		return NULL;

	/* set the HW descriptor */
	hw_descriptor = &sw_desc->hw_desc;

	/* save the SW descriptor ID to restore when operation is done */
	hw_descriptor->desc_id = sw_desc->idx;

	/* Set the INTERRUPT control word */
	hw_descriptor->desc_ctrl =
		DESC_OP_MODE_NOP << DESC_OP_MODE_SHIFT;
	hw_descriptor->desc_ctrl |= DESC_IOD;

	/* return the async tx descriptor */
	return &sw_desc->async_tx;
}

/*
 * poll for a transaction completion
 */
static enum dma_status mv_xor_v2_tx_status(struct dma_chan *chan,
		dma_cookie_t cookie, struct dma_tx_state *txstate)
{
	/* return the transaction status */
	return dma_cookie_status(chan, cookie, txstate);
}

/*
 * issue all the pending descriptors (update the DMA Engine with
 * the ready descriptors)
 */
static void mv_xor_v2_issue_pending(struct dma_chan *chan)
{
	struct mv_xor_v2_device *xor_dev =
		container_of(chan, struct mv_xor_v2_device, dmachan);

	/* Lock the channel */
	spin_lock_bh(&xor_dev->push_lock);
	if (xor_dev->npendings > 0) {
		mv_xor_v2_add_desc_to_desq(xor_dev, xor_dev->npendings);
		xor_dev->npendings = 0;
	}
	/* Release the channel */
	spin_unlock_bh(&xor_dev->push_lock);
}

static inline
int mv_xor_v2_get_pending_params(struct mv_xor_v2_device *xor_dev,
				 int *pending_ptr)
{
	u32 reg;

	reg = readl(xor_dev->dma_base + DMA_DESQ_DONE_OFF);

	/* get the next pending descriptor index */
	*pending_ptr = ((reg >> DMA_DESQ_DONE_READ_PTR_SHIFT) &
			DMA_DESQ_DONE_READ_PTR_MASK);

	/* get the number of descriptors pending handle */
	return ((reg >> DMA_DESQ_DONE_PENDING_SHIFT) &
		DMA_DESQ_DONE_PENDING_MASK);
}

/*
 * handle the descriptors after HW process
 */
static void mv_xor_v2_tasklet(unsigned long data)
{
	struct mv_xor_v2_device *xor_dev = (struct mv_xor_v2_device *) data;
	int pending_ptr, num_of_pending, i;
	struct mv_xor_v2_descriptor *next_pending_hw_desc = NULL;
	struct mv_xor_v2_sw_desc *next_pending_sw_desc = NULL;
	struct mv_xor_v2_sw_desc *iter, *_iter;

	dev_dbg(xor_dev->dmadev.dev, "%s %d\n", __func__, __LINE__);

	/* Lock the channel */
	spin_lock_bh(&xor_dev->sw_ll_lock);
	/* free the compeleted descriptors (check the ctrl ack flag) */
	list_for_each_entry_safe(iter, _iter, &xor_dev->complete_sw_desc,
				 node) {
		if (async_tx_test_ack(&iter->async_tx))
			list_move_tail(&iter->node, &xor_dev->free_sw_desc);
	}
	/* Release the channel */
	spin_unlock_bh(&xor_dev->sw_ll_lock);

	/* get the pending descriptors parameters */
	num_of_pending = mv_xor_v2_get_pending_params(xor_dev, &pending_ptr);

	/* loop over free descriptors */
	for (i = 0; i < num_of_pending; i++) {

		if (pending_ptr >= MV_XOR_V2_MAX_DESC_NUM)
			pending_ptr = 0;

		next_pending_hw_desc = (struct mv_xor_v2_descriptor *)
			((void *)xor_dev->hw_desq_virt +
			 (xor_dev->desc_size * (pending_ptr)));

		/* get the SW descriptor related to the HW descriptor */
		next_pending_sw_desc =
			&xor_dev->sw_desq[next_pending_hw_desc->desc_id];

		/* call the callback */
		if (next_pending_sw_desc->async_tx.cookie > 0) {
			/*
			 * update the channel's completed cookie - no
			 * lock is required the IMSG threshold provide
			 * the locking
			 */
			dma_cookie_complete(&next_pending_sw_desc->async_tx);

			if (next_pending_sw_desc->async_tx.callback)
				next_pending_sw_desc->async_tx.callback(
				next_pending_sw_desc->async_tx.callback_param);

			dma_descriptor_unmap(&next_pending_sw_desc->async_tx);
		}

		dma_run_dependencies(&next_pending_sw_desc->async_tx);

		/* Lock the channel */
		spin_lock_bh(&xor_dev->sw_ll_lock);

		/*
		 * add the SW descriptor to the free/completed descriptors list
		 * depends on ctrl ack flag
		 */
		if (!async_tx_test_ack(&next_pending_sw_desc->async_tx))
			list_move_tail(&next_pending_sw_desc->node, &xor_dev->complete_sw_desc);
		else
			list_move_tail(&next_pending_sw_desc->node, &xor_dev->free_sw_desc);


		/* Release the channel */
		spin_unlock_bh(&xor_dev->sw_ll_lock);

		/* increment the next descriptor */
		pending_ptr++;
	}

	if (num_of_pending != 0) {
		/* free the descriptors */
		mv_xor_v2_free_desc_from_desq(xor_dev, num_of_pending);
	}
}

/*
 *	Set DMA Interrupt-message (IMSG) parameters
 */
static void mv_xor_v2_set_msi_msg(struct msi_desc *desc, struct msi_msg *msg)
{
	struct mv_xor_v2_device *xor_dev = dev_get_drvdata(desc->dev);

	writel(msg->address_lo, xor_dev->dma_base + DMA_IMSG_BALR_OFF);
	writel(msg->address_hi & 0xFFFF, xor_dev->dma_base + DMA_IMSG_BAHR_OFF);
	writel(msg->data, xor_dev->dma_base + DMA_IMSG_CDAT_OFF);
}

static int mv_xor_v2_descq_init(struct mv_xor_v2_device *xor_dev)
{
	u32 reg;

	/* write the DESQ size to the DMA engine */
	writel(MV_XOR_V2_MAX_DESC_NUM, xor_dev->dma_base + DMA_DESQ_SIZE_OFF);

	/* write the DESQ address to the DMA enngine*/
	writel(xor_dev->hw_desq & 0xFFFFFFFF,
	       xor_dev->dma_base + DMA_DESQ_BALR_OFF);
	writel((xor_dev->hw_desq & 0xFFFF00000000) >> 32,
	       xor_dev->dma_base + DMA_DESQ_BAHR_OFF);

	/*
	 * This is a temporary solution, until we activate the
	 * SMMU. Set the attributes for reading & writing data buffers
	 * & descriptors to:
	 *
	 *  - OuterShareable - Snoops will be performed on CPU caches
	 *  - Enable cacheable - Bufferable, Modifiable, Other Allocate
	 *    and Allocate
	 */
	reg = readl(xor_dev->dma_base + DMA_DESQ_ARATTR_OFF);
	reg &= ~DMA_DESQ_ATTR_CACHE_MASK;
	reg |= DMA_DESQ_ATTR_OUTER_SHAREABLE | DMA_DESQ_ATTR_CACHEABLE;
	writel(reg, xor_dev->dma_base + DMA_DESQ_ARATTR_OFF);

	reg = readl(xor_dev->dma_base + DMA_DESQ_AWATTR_OFF);
	reg &= ~DMA_DESQ_ATTR_CACHE_MASK;
	reg |= DMA_DESQ_ATTR_OUTER_SHAREABLE | DMA_DESQ_ATTR_CACHEABLE;
	writel(reg, xor_dev->dma_base + DMA_DESQ_AWATTR_OFF);

	/* BW CTRL - set values to optimize the XOR performance:
	 *
	 *  - Set WrBurstLen & RdBurstLen - the unit will issue
	 *    maximum of 256B write/read transactions.
	 * -  Limit the number of outstanding write & read data
	 *    (OBB/IBB) requests to the maximal value.
	*/
	reg = (GLOB_BW_CTRL_NUM_OSTD_RD_VAL << GLOB_BW_CTRL_NUM_OSTD_RD_SHIFT) |
		(GLOB_BW_CTRL_NUM_OSTD_WR_VAL << GLOB_BW_CTRL_NUM_OSTD_WR_SHIFT) |
		(GLOB_BW_CTRL_RD_BURST_LEN_VAL << GLOB_BW_CTRL_RD_BURST_LEN_SHIFT) |
		(GLOB_BW_CTRL_WR_BURST_LEN_VAL << GLOB_BW_CTRL_WR_BURST_LEN_SHIFT);
	writel(reg, xor_dev->glob_base + GLOB_BW_CTRL);

	/* Disable the AXI timer feature */
	reg = readl(xor_dev->glob_base + GLOB_PAUSE);
	reg |= GLOB_PAUSE_AXI_TIME_DIS_VAL;
	writel(reg, xor_dev->glob_base + GLOB_PAUSE);

	/* enable the DMA engine */
	writel(0, xor_dev->dma_base + DMA_DESQ_STOP_OFF);

	return 0;
}

static int mv_xor_v2_suspend(struct platform_device *dev, pm_message_t state)
{
	struct mv_xor_v2_device *xor_dev = platform_get_drvdata(dev);

	dev_dbg(xor_dev->dmadev.dev, "%s %d\n", __func__, __LINE__);

	/* Set this bit to disable to stop the XOR unit. */
	writel(0x1, xor_dev->dma_base + DMA_DESQ_STOP_OFF);

	return 0;
}

static int mv_xor_v2_resume(struct platform_device *dev)
{
	struct mv_xor_v2_device *xor_dev = platform_get_drvdata(dev);

	dev_dbg(xor_dev->dmadev.dev, "%s %d\n", __func__, __LINE__);

	/* Configure the descriptor size */
	mv_xor_v2_set_desc_size(xor_dev);
	/* Configure the message threshold */
	mv_xor_v2_enable_imsg_thrd(xor_dev);
	/* Init the descriptors in the engine */
	mv_xor_v2_descq_init(xor_dev);

	return 0;
}

static int mv_xor_v2_probe(struct platform_device *pdev)
{
	struct mv_xor_v2_device *xor_dev;
	struct resource *res;
	int i, ret = 0;
	struct dma_device *dma_dev;
	struct mv_xor_v2_sw_desc *sw_desc;
	struct msi_desc *msi_desc;
	struct device *dev = &pdev->dev;
	u32 dma_bus_width;

	dev_notice(&pdev->dev, "Marvell Version 2 XOR driver\n");

	xor_dev = devm_kzalloc(&pdev->dev, sizeof(*xor_dev), GFP_KERNEL);
	if (!xor_dev)
		return -ENOMEM;

	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	xor_dev->dma_base = devm_ioremap_resource(&pdev->dev, res);
	if (IS_ERR(xor_dev->dma_base))
		return PTR_ERR(xor_dev->dma_base);

	res = platform_get_resource(pdev, IORESOURCE_MEM, 1);
	xor_dev->glob_base = devm_ioremap_resource(&pdev->dev, res);
	if (IS_ERR(xor_dev->glob_base))
		return PTR_ERR(xor_dev->glob_base);

	platform_set_drvdata(pdev, xor_dev);

	xor_dev->clk = devm_clk_get(&pdev->dev, NULL);
	if (!IS_ERR(xor_dev->clk)) {
		ret = clk_prepare_enable(xor_dev->clk);
		if (ret)
			return ret;
	} else if (PTR_ERR(xor_dev->clk) == -EPROBE_DEFER) {
		return -EPROBE_DEFER;
	} else {
		dev_err(dev, "Failed to enable XOR clock.\n");
		return PTR_ERR(xor_dev->clk);
	}

	ret = platform_msi_domain_alloc_irqs(&pdev->dev, 1,
					     mv_xor_v2_set_msi_msg);
	if (ret)
		goto disable_clk;

	/* Read the DMA bus width from the device-tree and configure it */
	ret = of_property_read_u32(pdev->dev.of_node, "dma-bus-width",
				   &dma_bus_width);
	if (ret) {
		dev_err(&pdev->dev, "Failed to read dma-bus-width property\n");
		goto disable_clk;
	}

	ret = dma_set_mask_and_coherent(&pdev->dev, DMA_BIT_MASK(dma_bus_width));
	if (ret)
		goto disable_clk;

	msi_desc = first_msi_entry(&pdev->dev);
	if (!msi_desc)
		goto free_msi_irqs;

	ret = devm_request_irq(&pdev->dev, msi_desc->irq,
			       mv_xor_v2_interrupt_handler, 0,
			       dev_name(&pdev->dev), xor_dev);
	if (ret)
		goto free_msi_irqs;

	tasklet_init(&xor_dev->irq_tasklet, mv_xor_v2_tasklet,
		     (unsigned long) xor_dev);

	xor_dev->desc_size = mv_xor_v2_set_desc_size(xor_dev);
	xor_dev->npendings = 0;
	xor_dev->hw_queue_idx = 0;

	dma_cookie_init(&xor_dev->dmachan);

	/*
	 * allocate coherent memory for hardware descriptors
	 * note: writecombine gives slightly better performance, but
	 * requires that we explicitly flush the writes
	 */
	xor_dev->hw_desq_virt =
		dma_alloc_coherent(&pdev->dev,
				   xor_dev->desc_size * MV_XOR_V2_MAX_DESC_NUM,
				   &xor_dev->hw_desq, GFP_KERNEL);
	if (!xor_dev->hw_desq_virt) {
		ret = -ENOMEM;
		goto free_msi_irqs;
	}

	/* alloc memory for the SW descriptors */
	xor_dev->sw_desq = devm_kzalloc(&pdev->dev, sizeof(*sw_desc) *
					MV_XOR_V2_MAX_DESC_NUM, GFP_KERNEL);
	if (!xor_dev->sw_desq) {
		ret = -ENOMEM;
		goto free_hw_desq;
	}

	/* init the channel locks */
	spin_lock_init(&xor_dev->sw_ll_lock);
	spin_lock_init(&xor_dev->push_lock);

	/* init the SW descriptors lists */
	INIT_LIST_HEAD(&xor_dev->free_sw_desc);
	INIT_LIST_HEAD(&xor_dev->complete_sw_desc);
	INIT_LIST_HEAD(&xor_dev->alloc_sw_desc);

	/* add all SW descriptors to the free list */
	for (i = 0; i < MV_XOR_V2_MAX_DESC_NUM; i++) {
		xor_dev->sw_desq[i].idx = i;
		list_add(&xor_dev->sw_desq[i].node,
			 &xor_dev->free_sw_desc);
	}

	dma_dev = &xor_dev->dmadev;

	/* set DMA capabilities */
	dma_cap_zero(dma_dev->cap_mask);
	dma_cap_set(DMA_MEMCPY, dma_dev->cap_mask);
	dma_cap_set(DMA_XOR, dma_dev->cap_mask);
	dma_cap_set(DMA_INTERRUPT, dma_dev->cap_mask);
	dma_cap_set(DMA_PQ, dma_dev->cap_mask);

	/* init dma link list */
	INIT_LIST_HEAD(&dma_dev->channels);

	/* set base routines */
	dma_dev->device_alloc_chan_resources =
		mv_xor_v2_alloc_chan_resources;
	dma_dev->device_free_chan_resources =
		mv_xor_v2_free_chan_resources;
	dma_dev->device_tx_status = mv_xor_v2_tx_status;
	dma_dev->device_issue_pending = mv_xor_v2_issue_pending;
	dma_dev->dev = &pdev->dev;

	dma_dev->device_prep_dma_memcpy = mv_xor_v2_prep_dma_memcpy;
	dma_dev->device_prep_dma_interrupt = mv_xor_v2_prep_dma_interrupt;
	dma_dev->max_xor = MV_XOR_V2_CMD_LINE_NUM_MAX_D_BUF;
	dma_dev->device_prep_dma_xor = mv_xor_v2_prep_dma_xor;
	dma_set_maxpq(dma_dev, MV_XOR_V2_CMD_LINE_NUM_MAX_D_BUF, 0);
	dma_dev->device_prep_dma_pq = mv_xor_v2_prep_dma_pq;

	xor_dev->dmachan.device = dma_dev;

	list_add_tail(&xor_dev->dmachan.device_node,
		      &dma_dev->channels);

	mv_xor_v2_enable_imsg_thrd(xor_dev);

	mv_xor_v2_descq_init(xor_dev);

	ret = dma_async_device_register(dma_dev);
	if (ret)
		goto free_hw_desq;

	return 0;

free_hw_desq:
	dma_free_coherent(&pdev->dev,
			  xor_dev->desc_size * MV_XOR_V2_MAX_DESC_NUM,
			  xor_dev->hw_desq_virt, xor_dev->hw_desq);
free_msi_irqs:
	platform_msi_domain_free_irqs(&pdev->dev);
disable_clk:
	if (!IS_ERR(xor_dev->clk))
		clk_disable_unprepare(xor_dev->clk);
	return ret;
}

static int mv_xor_v2_remove(struct platform_device *pdev)
{
	struct mv_xor_v2_device *xor_dev = platform_get_drvdata(pdev);

	dma_async_device_unregister(&xor_dev->dmadev);

	dma_free_coherent(&pdev->dev,
			  xor_dev->desc_size * MV_XOR_V2_MAX_DESC_NUM,
			  xor_dev->hw_desq_virt, xor_dev->hw_desq);

	platform_msi_domain_free_irqs(&pdev->dev);

	return 0;
}

#ifdef CONFIG_OF
static const struct of_device_id mv_xor_v2_dt_ids[] = {
	{ .compatible = "marvell,mv-xor-v2", },
	{},
};

MODULE_DEVICE_TABLE(of, mv_xor_v2_dt_ids);
#endif

static struct platform_driver mv_xor_v2_driver = {
	.probe		= mv_xor_v2_probe,
	.suspend	= mv_xor_v2_suspend,
	.resume		= mv_xor_v2_resume,
	.remove		= mv_xor_v2_remove,
	.driver		= {
		.owner	= THIS_MODULE,
		.name	= "mv_xor_v2",
		.of_match_table = of_match_ptr(mv_xor_v2_dt_ids),
	},
};

module_platform_driver(mv_xor_v2_driver);

MODULE_DESCRIPTION("DMA engine driver for Marvell's Version 2 of XOR engine");
MODULE_LICENSE("GPL");

