 /*
 * ***************************************************************************
 * Copyright (C) 2016 Marvell International Ltd.
 * ***************************************************************************
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the Free
 * Software Foundation, either version 2 of the License, or any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 * ***************************************************************************
 */

#include <linux/kernel.h>
#include <linux/version.h>
#include <linux/netdevice.h>
#include <linux/etherdevice.h>
#include <linux/platform_device.h>
#include <linux/skbuff.h>
#include <linux/inetdevice.h>
#include <linux/mbus.h>
#include <linux/module.h>
#include <linux/interrupt.h>
#include <linux/irq.h>
#include <linux/cpumask.h>
#include <linux/kallsyms.h>
#include <linux/of.h>
#include <linux/of_irq.h>
#include <linux/of_mdio.h>
#include <linux/of_net.h>
#include <linux/of_address.h>
#include <linux/of_address.h>
#include <linux/of_device.h>
#include <linux/phy/phy.h>
#include <linux/if_vlan.h>
#include <linux/cpu.h>
#include <linux/phy.h>
#include <linux/clk.h>
#include <linux/hrtimer.h>
#include <linux/ktime.h>
#include <uapi/linux/ppp_defs.h>
#include <net/ip.h>
#include <net/ipv6.h>
#include <net/busy_poll.h>
#include <asm/cacheflush.h>
#include <linux/dma-mapping.h>
#include <linux/cma.h>
#include <linux/dma-contiguous.h>
#include <linux/of_reserved_mem.h>

#include <dt-bindings/phy/phy-comphy-mvebu.h>

#include "mv_pp2x.h"
#include "mv_pp2x_hw.h"
#include "mv_gop110_hw.h"

#if defined(CONFIG_NETMAP) || defined(CONFIG_NETMAP_MODULE)
#include <if_mv_pp2x_netmap.h>
#endif

#ifdef CONFIG_MV_PTP_SERVICE
/* inline PTP procedures */
#include <mv_pp2x_ptp_hook.c>
/* non-inline init/config */
#include <mv_ptp_if.h>
#include <mv_ptp_service.h>
#include <mv_pp2x_ptp_init.h>
#endif

#define MVPP2_SKB_TEST_SIZE 64
#define MVPP2_ADDRESS 0xf2000000
#define CPN110_ADDRESS_SPACE_SIZE (16 * 1024 * 1024)

#define UIO_PP2_STRING  "uio_pp_%d"
#define UIO_PORT_STRING "uio_pp_port_%d:%d"

/* Declaractions */
#if defined(CONFIG_NETMAP) || defined(CONFIG_NETMAP_MODULE)
u8 mv_pp2x_num_cos_queues = 1;
#else
u8 mv_pp2x_num_cos_queues = 4;
#endif

u8 mv_pp2x_rx_count = MVPP2_DEFAULT_RX_COUNT;

static u8 mv_pp2x_queue_mode = MVPP2_QDIST_SINGLE_MODE;
static u8 mv_pp2x_used_addr_spaces;
static u8 rss_mode = MVPP2_RSS_5T; /* Set 5-tuple default RSS mode */
static u8 default_cpu;
static u8 cos_classifer;
static u32 pri_map = 0x3210; /* As default, cos0--rxq0, cos1--rxq1,
			      * cos2--rxq2, cos3--rxq3
			      */
static u8 default_cos = 3; /* As default, non-IP packet has highest CoS value */
static u16 rx_queue_size = MVPP2_MAX_RXD;
static u16 tx_queue_size = MVPP2_MAX_TXD;
static u16 buffer_scaling = 100;
static u32 port_cpu_bind_map;
static u8 first_bm_pool;
static u8 first_log_rxq_queue;
static u8 uc_filter_max = 4;
static u16 stats_delay_msec = STATS_DELAY;
static u16 stats_delay;
static int auto_cell_index;

u32 debug_param;

struct mv_pp2x_pool_attributes mv_pp2x_pools[] = {
	{
		.description =  "short", /* pkt_size=MVPP2_BM_SHORT_PKT_SIZE */
		.buf_num     =  MVPP2_BM_SHORT_BUF_NUM,
	},
	{
		.description =  "long", /* pkt_size=MVPP2_BM_LONG_PKT_SIZE */
		.buf_num     =  MVPP2_BM_LONG_BUF_NUM,
	},
	{
		.description =	"jumbo", /* pkt_size=MVPP2_BM_JUMBO_PKT_SIZE */
		.buf_num     =  MVPP2_BM_JUMBO_BUF_NUM,
	}
};

module_param_named(num_cos_queues, mv_pp2x_num_cos_queues, byte, S_IRUGO);
MODULE_PARM_DESC(num_cos_queues, "Set number of cos_queues (1-8), def=4");

module_param_named(queue_mode, mv_pp2x_queue_mode, byte, S_IRUGO);
MODULE_PARM_DESC(queue_mode, "Set queue_mode (single=0, multi=1)");

module_param(rss_mode, byte, S_IRUGO);
MODULE_PARM_DESC(rss_mode, "Set rss_mode (2T=0, 5T=1)");

module_param(default_cpu, byte, S_IRUGO);
MODULE_PARM_DESC(default_cpu, "Set default CPU for non RSS frames");

module_param(cos_classifer, byte, S_IRUGO);
MODULE_PARM_DESC(cos_classifer,
		 "Cos Classifier (vlan_pri=0, dscp=1, vlan_dscp=2, dscp_vlan=3)");

module_param(pri_map, uint, S_IRUGO);
MODULE_PARM_DESC(pri_map, "Set priority_map, nibble for each cos.");

module_param(default_cos, byte, S_IRUGO);
MODULE_PARM_DESC(default_cos,
		 "Set default cos value(0-7) for unclassified traffic");

module_param(rx_queue_size, ushort, S_IRUGO);
MODULE_PARM_DESC(rx_queue_size, "Rx queue size");

module_param(tx_queue_size, ushort, S_IRUGO);
MODULE_PARM_DESC(tx_queue_size, "Tx queue size");

module_param(buffer_scaling, ushort, S_IRUGO);
MODULE_PARM_DESC(buffer_scaling, "Buffer scaling (TBD)");

module_param(uc_filter_max, byte, S_IRUGO);
MODULE_PARM_DESC(uc_filter_max,
		 "Set unicast filter max size, it is multiple of 4. def=4");

module_param(debug_param, uint, S_IRUGO);
MODULE_PARM_DESC(debug_param,
		 "Ad-hoc parameter, which can be used for various debug operations.");

module_param(stats_delay_msec, ushort, S_IRUGO);
MODULE_PARM_DESC(stats_delay_msec, "Set statistic delay in msec, def=250");

module_param_named(short_pool, mv_pp2x_pools[MVPP2_BM_SWF_SHORT_POOL].buf_num, uint, S_IRUGO);
MODULE_PARM_DESC(short_pool, "Short pool size (0-8192), def=2048");

module_param_named(long_pool, mv_pp2x_pools[MVPP2_BM_SWF_LONG_POOL].buf_num, uint, S_IRUGO);
MODULE_PARM_DESC(long_pool, "Long pool size (0-8192), def=1024");

module_param_named(jumbo_pool, mv_pp2x_pools[MVPP2_BM_SWF_JUMBO_POOL].buf_num, uint, S_IRUGO);
MODULE_PARM_DESC(jumbo_pool, "Jumbo pool size (0-8192), def=512");

module_param_named(rx_count, mv_pp2x_rx_count, byte, S_IRUGO);
MODULE_PARM_DESC(rx_count, "Set rx_count 8 or 16, def = 8. Parameter configure number of HOT CPU's.");

/*TODO:  Below module_params will not go to ML. Created for testing. */

module_param(port_cpu_bind_map, uint, S_IRUGO);
MODULE_PARM_DESC(port_cpu_bind_map,
		 "Set default port-to-cpu binding, nibble for each port. Relevant when queue_mode=multi-mode & rss is disabled");

module_param(first_bm_pool, byte, S_IRUGO);
MODULE_PARM_DESC(first_bm_pool, "First used buffer pool (0-11)");

module_param(first_log_rxq_queue, byte, S_IRUGO);
MODULE_PARM_DESC(first_log_rxq_queue, "First logical rx_queue (0-31)");

void set_device_base_address(struct net_device *dev)
{
	struct mv_pp2x_port *port = netdev_priv(dev);

	dev->mem_start = (unsigned long)port->priv->hw.phys_addr_start;
	dev->mem_end = (unsigned long)port->priv->hw.phys_addr_end;
}

/* Number of RXQs used by single port */
static int mv_pp2x_rxq_number;
/* Number of TXQs used by single port */
static int mv_pp2x_txq_number;

static inline int mv_pp2x_txq_count(struct mv_pp2x_txq_pcpu *txq_pcpu)
{
	int index_modulo = (txq_pcpu->txq_put_index - txq_pcpu->txq_get_index +
				txq_pcpu->size) % txq_pcpu->size;

	return index_modulo;
}

static inline int mv_pp2x_txq_free_count(struct mv_pp2x_txq_pcpu *txq_pcpu)
{
	int index_modulo = (txq_pcpu->txq_get_index - txq_pcpu->txq_put_index +
				txq_pcpu->size) % txq_pcpu->size;

	if (unlikely(index_modulo == 0))
		return txq_pcpu->size;

	return index_modulo;
}

static void mv_pp2x_txq_inc_get(struct mv_pp2x_txq_pcpu *txq_pcpu)
{
	if (unlikely(txq_pcpu->txq_get_index == txq_pcpu->size - 1))
		txq_pcpu->txq_get_index = 0;
	else
		txq_pcpu->txq_get_index++;
}

void mv_pp2x_txq_inc_error(struct mv_pp2x_txq_pcpu *txq_pcpu, int num)
{
	for (; num > 0; num--) {
		if (unlikely(txq_pcpu->txq_put_index < 1))
			txq_pcpu->txq_put_index = txq_pcpu->size - 1;
		else
			txq_pcpu->txq_put_index--;
		txq_pcpu->tx_skb[txq_pcpu->txq_put_index] = 0;
		txq_pcpu->data_size[txq_pcpu->txq_put_index] = 0;
		txq_pcpu->tx_buffs[txq_pcpu->txq_put_index] = 0;
	}
}

void mv_pp2x_txq_inc_put(enum mvppv2_version pp2_ver,
			 struct mv_pp2x_txq_pcpu *txq_pcpu,
			 struct sk_buff *skb,
			 struct mv_pp2x_tx_desc *tx_desc)
{
	txq_pcpu->tx_skb[txq_pcpu->txq_put_index] = skb;
	txq_pcpu->data_size[txq_pcpu->txq_put_index] = tx_desc->data_size;
	txq_pcpu->tx_buffs[txq_pcpu->txq_put_index] =
				mv_pp2x_txdesc_phys_addr_get(pp2_ver, tx_desc);
	if (unlikely(txq_pcpu->txq_put_index == txq_pcpu->size - 1))
		txq_pcpu->txq_put_index = 0;
	else
		txq_pcpu->txq_put_index++;
#if defined(__BIG_ENDIAN)
	if (pp2_ver == PPV21)
		mv_pp21_tx_desc_swap(tx_desc);
	else
		mv_pp22_tx_desc_swap(tx_desc);
#endif /* __BIG_ENDIAN */
}

static void mv_pp2x_txq_dec_put(struct mv_pp2x_txq_pcpu *txq_pcpu)
{
	if (unlikely(txq_pcpu->txq_put_index == 0))
		txq_pcpu->txq_put_index = txq_pcpu->size - 1;
	else
		txq_pcpu->txq_put_index--;
}

static void mv_pp2x_extra_pool_inc(struct mv_pp2x_ext_buf_pool *ext_buf_pool)
{
	if (unlikely(ext_buf_pool->buf_pool_next_free == ext_buf_pool->buf_pool_size - 1))
		ext_buf_pool->buf_pool_next_free = 0;
	else
		ext_buf_pool->buf_pool_next_free++;
}

static void mv_pp2x_skb_pool_inc(struct mv_pp2x_skb_pool *skb_pool)
{
	if (unlikely(skb_pool->skb_pool_next_free == skb_pool->skb_pool_size - 1))
		skb_pool->skb_pool_next_free = 0;
	else
		skb_pool->skb_pool_next_free++;
}

static u8 mv_pp2x_first_pool_get(struct mv_pp2x *priv)
{
	return priv->pp2_cfg.first_bm_pool;
}

static int mv_pp2x_pool_pkt_size_get(enum mv_pp2x_bm_pool_log_num  log_id)
{
	return mv_pp2x_pools[log_id].pkt_size;
}

static int mv_pp2x_pool_buf_num_get(enum mv_pp2x_bm_pool_log_num  log_id)
{
	return mv_pp2x_pools[log_id].buf_num;
}

char *mv_pp2x_pool_description_get(enum mv_pp2x_bm_pool_log_num  log_id)
{
	return mv_pp2x_pools[log_id].description;
}
EXPORT_SYMBOL(mv_pp2x_pool_description_get);

/* Buffer Manager configuration routines */
static void *mv_pp2x_frag_alloc(const struct mv_pp2x_bm_pool *pool)
{
	if (likely(pool->frag_size <= PAGE_SIZE))
		return netdev_alloc_frag(pool->frag_size);
	else
		return kmalloc(pool->frag_size, GFP_ATOMIC);
}

static void mv_pp2x_frag_free(const struct mv_pp2x_bm_pool *pool, void *data)
{
	if (likely(pool->frag_size <= PAGE_SIZE))
		skb_free_frag(data);
	else
		kfree(data);
}

static int mv_pp2x_rx_refill_new(struct mv_pp2x_port *port,
				 struct mv_pp2x_bm_pool *bm_pool,
			   u32 pool, int is_recycle, int cpu)
{
	dma_addr_t phys_addr;
	void *data;
	struct mv_pp2x_cp_pcpu *cp_pcpu = port->priv->pcpu[cpu];

	/* BM pool is refilled only if number of used buffers is bellow
	 * refill threshold. Number of used buffers could be decremented
	 * by recycling mechanism.
	 */
	if (is_recycle &&
	    (cp_pcpu->in_use[bm_pool->id] < bm_pool->in_use_thresh))
		return 0;

	data = mv_pp2x_frag_alloc(bm_pool);
	if (!data)
		return -ENOMEM;

	phys_addr = dma_map_single(port->dev->dev.parent, data,
				   MVPP2_RX_BUF_SIZE(bm_pool->pkt_size),
				   DMA_FROM_DEVICE);

	if (unlikely(dma_mapping_error(port->dev->dev.parent, phys_addr))) {
		mv_pp2x_frag_free(bm_pool, data);
		return -ENOMEM;
	}

	mv_pp2x_pool_refill(port->priv, pool, phys_addr, cpu);

	/* Decrement only if refill called from RX */
	if (is_recycle)
		cp_pcpu->in_use[bm_pool->id]--;

	return 0;
}

/* Create pool */
static int mv_pp2x_bm_pool_create(struct device *dev,
				  struct mv_pp2x_hw *hw,
					 struct mv_pp2x_bm_pool *bm_pool,
					 int size, int pkt_size)
{
	int size_bytes;

	/* Driver enforces size= x16 both for PPv21 and for PPv22, even though
	 *    PPv22 HW allows size= x8
	 */
	if (!IS_ALIGNED(size, (1 << MVPP21_BM_POOL_SIZE_OFFSET)))
		return -EINVAL;

	/*YuvalC: Two pointers per buffer, existing bug fixed. */
	size_bytes = 2 * sizeof(uintptr_t) * size;
	bm_pool->virt_addr = dma_alloc_coherent(dev, size_bytes,
						&bm_pool->phys_addr,
						GFP_KERNEL);
	if (!bm_pool->virt_addr)
		return -ENOMEM;

	if (!IS_ALIGNED((uintptr_t)bm_pool->virt_addr,
			MVPP2_BM_POOL_PTR_ALIGN)) {
		dma_free_coherent(dev, size_bytes, bm_pool->virt_addr,
				  bm_pool->phys_addr);
		dev_err(dev, "BM pool %d is not %d bytes aligned\n",
			bm_pool->id, MVPP2_BM_POOL_PTR_ALIGN);
		return -ENOMEM;
	}

	mv_pp2x_bm_hw_pool_create(hw, bm_pool->id, bm_pool->phys_addr, size);

	bm_pool->size = size;
	bm_pool->pkt_size = pkt_size;
	bm_pool->frag_size = SKB_DATA_ALIGN(MVPP2_RX_BUF_SIZE(
				bm_pool->pkt_size)) + MVPP2_SKB_SHINFO_SIZE;
	bm_pool->buf_num = 0;
	mv_pp2x_bm_pool_bufsize_set(hw, bm_pool,
				    MVPP2_RX_BUF_SIZE(bm_pool->pkt_size));

	return 0;
}

void mv_pp2x_bm_bufs_free(struct device *dev, struct mv_pp2x *priv,
			  struct mv_pp2x_bm_pool *bm_pool, int buf_num)
{
	int i;

	if (buf_num > bm_pool->buf_num) {
		WARN(1, "Pool does not have so many bufs pool(%d) bufs(%d)\n",
		     bm_pool->id, buf_num);
		buf_num = bm_pool->buf_num;
	}
	for (i = 0; i < buf_num; i++) {
		u8 *virt_addr;
		dma_addr_t phys_addr;

		/* Get buffer virtual address (indirect access) */
		phys_addr = mv_pp2x_bm_phys_addr_get(&priv->hw, bm_pool->id);
		if (!phys_addr)
			break;
		if (!bm_pool->external_pool) {
			dma_unmap_single(dev, phys_addr,
					 MVPP2_RX_BUF_SIZE(bm_pool->pkt_size), DMA_TO_DEVICE);
			virt_addr = phys_to_virt(dma_to_phys(dev, phys_addr));
			mv_pp2x_frag_free(bm_pool, virt_addr);
		}
	}

	/* Update BM driver with number of buffers removed from pool */
	bm_pool->buf_num -= i;
}

/* Cleanup pool */
int mv_pp2x_bm_pool_destroy(struct device *dev, struct mv_pp2x *priv,
			    struct mv_pp2x_bm_pool *bm_pool)
{
	u32 val;
	int size_bytes, buf_num;

	buf_num = mv_pp2x_check_hw_buf_num(priv, bm_pool);

	mv_pp2x_bm_bufs_free(dev, priv, bm_pool, buf_num);

	/* Check buffer counters after free */
	buf_num = mv_pp2x_check_hw_buf_num(priv, bm_pool);

	if (buf_num) {
		WARN(1, "cannot free all buffers in pool %d, buf_num left %d\n",
		     bm_pool->id,
		     bm_pool->buf_num);
		return 0;
	}

	val = mv_pp2x_read(&priv->hw, MVPP2_BM_POOL_CTRL_REG(bm_pool->id));
	val |= MVPP2_BM_STOP_MASK;
	mv_pp2x_write(&priv->hw, MVPP2_BM_POOL_CTRL_REG(bm_pool->id), val);

	size_bytes = 2 * sizeof(uintptr_t) * bm_pool->size;
	dma_free_coherent(dev, size_bytes, bm_pool->virt_addr,
			  bm_pool->phys_addr);
	mv_pp2x_bm_pool_bufsize_set(&priv->hw, bm_pool, 0);
	priv->num_pools--;
	return 0;
}

int mv_pp2x_bm_pool_ext_add(struct device *dev, struct mv_pp2x *priv,
			    u32 *pool_num, u32 pkt_size)
{
	int err, size, enabled;
	u8 first_pool = mv_pp2x_first_pool_get(priv);
	u32 pool = priv->num_pools;
	struct mv_pp2x_bm_pool *bm_pool;
	struct mv_pp2x_hw *hw = &priv->hw;

	if ((priv->num_pools + 1) > MVPP2_BM_POOLS_MAX_ALLOC_NUM) {
		dev_err(dev, "Unable to add pool. Max BM pool alloc reached %d\n",
			priv->num_pools + 1);
		return -ENOMEM;
	}

	/* Check if pool is already active. Ignore request */
	enabled = mv_pp2x_read(hw, MVPP2_BM_POOL_CTRL_REG(pool)) &
		MVPP2_BM_STATE_MASK;

	if (enabled) {
		dev_info(dev, "%s pool %d already enabled. Ignoring request\n",
			 __func__, pool);
		return 0;
	}

	/* Mask BM interrupts */
	mv_pp2x_write(&priv->hw, MVPP2_BM_INTR_MASK_REG(first_pool +
		      pool), 0);
	/* Clear BM cause register */
	mv_pp2x_write(&priv->hw, MVPP2_BM_INTR_CAUSE_REG(first_pool +
		      pool), 0);

	/* Create all pools with maximum size */
	size = MVPP2_BM_POOL_SIZE_MAX;
	bm_pool = &priv->bm_pools[pool];
	bm_pool->log_id = pool;
	bm_pool->id = first_pool + pool;
	bm_pool->external_pool = true;
	err = mv_pp2x_bm_pool_create(dev, hw, bm_pool, size, pkt_size);
	if (err)
		return err;

	*pool_num = pool;
	priv->num_pools++;
	return 0;
}

static int mv_pp2x_bm_pools_init(struct platform_device *pdev,
				 struct mv_pp2x *priv,
				       u8 first_pool, u8 num_pools)
{
	int i, err, size;
	struct mv_pp2x_bm_pool *bm_pool;
	struct mv_pp2x_hw *hw = &priv->hw;

	/* Create all pools with maximum size */
	size = MVPP2_BM_POOL_SIZE_MAX;
	for (i = 0; i < num_pools; i++) {
		bm_pool = &priv->bm_pools[i];
		bm_pool->log_id = i;
		bm_pool->id = first_pool + i;
		bm_pool->external_pool = false;
		err = mv_pp2x_bm_pool_create(&pdev->dev, hw, bm_pool, size,
					     mv_pp2x_pool_pkt_size_get(bm_pool->log_id));
		if (err)
			goto err_unroll_pools;
	}
	priv->num_pools = num_pools;
	return 0;

err_unroll_pools:
	dev_err(&pdev->dev, "failed to create BM pool %d, size %d\n", i, size);
	for (i = i - 1; i >= 0; i--)
		mv_pp2x_bm_pool_destroy(&pdev->dev, priv, &priv->bm_pools[i]);
	return err;
}

static int mv_pp2x_bm_init(struct platform_device *pdev, struct mv_pp2x *priv)
{
	int i, err, cpu;
	u8 first_pool = mv_pp2x_first_pool_get(priv);
	u8 num_pools = MVPP2_BM_SWF_NUM_POOLS;

	for (i = first_pool; i < (first_pool + num_pools); i++) {
		/* Mask BM all interrupts */
		mv_pp2x_write(&priv->hw, MVPP2_BM_INTR_MASK_REG(i), 0);
		/* Clear BM cause register */
		mv_pp2x_write(&priv->hw, MVPP2_BM_INTR_CAUSE_REG(i), 0);
	}

	/* Allocate and initialize BM pools */
	priv->bm_pools = devm_kcalloc(&pdev->dev, MVPP2_BM_POOLS_NUM,
				      sizeof(struct mv_pp2x_bm_pool),
				      GFP_KERNEL);
	if (!priv->bm_pools)
		return -ENOMEM;

	/* On PPV22 high virtual and physical address buffer manager register should be
	 * initialized to 0 to avoid writing to the random addresses an 32 Bit systems.
	 */
	if (priv->pp2_version == PPV22) {
		for (cpu = 0; cpu < mv_pp2x_used_addr_spaces; cpu++) {
			/* Reset the BM virtual and physical address high register */
			mv_pp2x_relaxed_write(&priv->hw, MVPP22_BM_PHY_VIRT_HIGH_RLS_REG,
					      0, cpu);
		}
	}

	err = mv_pp2x_bm_pools_init(pdev, priv, first_pool, num_pools);
	if (err < 0)
		return err;
	return 0;
}

/* Allocate buffers for the pool */
int mv_pp2x_bm_bufs_add(struct mv_pp2x_port *port,
			struct mv_pp2x_bm_pool *bm_pool, int buf_num)
{
	int i, buf_size, total_size;

	buf_size = MVPP2_RX_BUF_SIZE(bm_pool->pkt_size);
	total_size = MVPP2_RX_TOTAL_SIZE(buf_size);

	if (buf_num < 0 ||
	    (buf_num + bm_pool->buf_num > bm_pool->size)) {
		netdev_err(port->dev,
			   "cannot allocate %d buffers for pool %d\n",
			   buf_num, bm_pool->id);
		return 0;
	}

	for (i = 0; i < buf_num; i++)
		mv_pp2x_rx_refill_new(port, bm_pool, (u32)bm_pool->id, 0, 0);

	/* Update BM driver with number of buffers added to pool */
	bm_pool->buf_num += i;
	bm_pool->in_use_thresh = bm_pool->buf_num / MVPP2_BM_PER_CPU_THRESHOLD(num_present_cpus());

	netdev_dbg(port->dev,
		   "%s pool %d: pkt_size=%4d, buf_size=%4d, total_size=%4d\n",
		   mv_pp2x_pool_description_get(bm_pool->log_id),
		   bm_pool->id, bm_pool->pkt_size, buf_size, total_size);

	netdev_dbg(port->dev,
		   "%s pool %d: %d of %d buffers added\n",
		   mv_pp2x_pool_description_get(bm_pool->log_id),
		   bm_pool->id, i, buf_num);
	return i;
}

static int mv_pp2x_bm_buf_calc(enum mv_pp2x_bm_pool_log_num log_pool,
			       u32 port_map)
{
	/*TODO: Code algo based  on
	 * port_map/num_rx_queues/num_tx_queues/queue_sizes
	 */
	int num_ports = hweight_long(port_map);

	return(num_ports * mv_pp2x_pool_buf_num_get(log_pool));
}

/* Notify the driver that BM pool is being used as specific type and return the
 * pool pointer on success
 */

static struct mv_pp2x_bm_pool *mv_pp2x_bm_pool_use_internal(
	struct mv_pp2x_port *port, enum mv_pp2x_bm_pool_log_num log_pool,
	bool add_port)
{
	int pkts_num, add_num, num;
	struct mv_pp2x_bm_pool *pool = &port->priv->bm_pools[log_pool];

	if (log_pool < MVPP2_BM_SWF_SHORT_POOL ||
	    log_pool > MVPP2_BM_SWF_JUMBO_POOL) {
		netdev_err(port->dev, "pool does not exist\n");
		return NULL;
	}

	if (add_port) {
		pkts_num = mv_pp2x_bm_buf_calc(log_pool,
					       pool->port_map |
					       (1 << port->id));
	} else {
		pkts_num = mv_pp2x_bm_buf_calc(log_pool,
					       pool->port_map &
					       ~(1 << port->id));
	}

	add_num = pkts_num - pool->buf_num;

	/* Allocate buffers for this pool */
	if (add_num > 0) {
		num = mv_pp2x_bm_bufs_add(port, pool, add_num);
		if (num != add_num) {
			WARN(1, "pool %d: %d of %d allocated\n",
			     pool->id, num, pkts_num);
			/* We need to undo the bufs_add() allocations */
			return NULL;
		}
	} else if (add_num < 0) {
		mv_pp2x_bm_bufs_free(port->dev->dev.parent, port->priv, pool, -add_num);
	}

	return pool;
}

static struct mv_pp2x_bm_pool *mv_pp2x_bm_pool_use(
			struct mv_pp2x_port *port,
			enum mv_pp2x_bm_pool_log_num log_pool)
{
	return mv_pp2x_bm_pool_use_internal(port, log_pool, true);
}

static struct mv_pp2x_bm_pool *mv_pp2x_bm_pool_stop_use(
			struct mv_pp2x_port *port,
			enum mv_pp2x_bm_pool_log_num log_pool)
{
	return mv_pp2x_bm_pool_use_internal(port, log_pool, false);
}

int mv_pp2x_swf_bm_pool_assign(struct mv_pp2x_port *port, u32 rxq,
			       u32 long_id, u32 short_id)
{
	struct mv_pp2x_hw *hw = &port->priv->hw;

	if (rxq >= port->num_rx_queues)
		return -ENOMEM;

	port->priv->pp2xdata->mv_pp2x_rxq_long_pool_set(hw,
		port->rxqs[rxq]->id, long_id);
	port->priv->pp2xdata->mv_pp2x_rxq_short_pool_set(hw,
		port->rxqs[rxq]->id, short_id);
	return 0;
}

/* Initialize pools for swf */
static int mv_pp2x_swf_bm_pool_init(struct mv_pp2x_port *port)
{
	int rxq;
	enum mv_pp2x_bm_pool_log_num long_log_pool, short_log_pool;
	struct mv_pp2x_hw *hw = &port->priv->hw;

	/* If port pkt_size is higher than 1518B:
	* HW Long pool - SW Jumbo pool, HW Short pool - SW Short pool
	* esle: HW Long pool - SW Long pool, HW Short pool - SW Short pool
	*/
	if (port->pkt_size > MVPP2_BM_LONG_PKT_SIZE) {
		long_log_pool = MVPP2_BM_SWF_JUMBO_POOL;
		short_log_pool = MVPP2_BM_SWF_LONG_POOL;
	} else {
		long_log_pool = MVPP2_BM_SWF_LONG_POOL;
		short_log_pool = MVPP2_BM_SWF_SHORT_POOL;
	}

	if (!port->pool_long) {
		port->pool_long =
		       mv_pp2x_bm_pool_use(port, long_log_pool);
		if (!port->pool_long)
			return -ENOMEM;
		port->pool_long->port_map |= (1 << port->id);

		for (rxq = 0; rxq < port->num_rx_queues; rxq++) {
			port->priv->pp2xdata->mv_pp2x_rxq_long_pool_set(hw,
				port->rxqs[rxq]->id, port->pool_long->id);
		}
	}

	if (!port->pool_short) {
		port->pool_short =
			mv_pp2x_bm_pool_use(port, short_log_pool);
		if (!port->pool_short)
			return -ENOMEM;

		port->pool_short->port_map |= (1 << port->id);

		for (rxq = 0; rxq < port->num_rx_queues; rxq++)
			port->priv->pp2xdata->mv_pp2x_rxq_short_pool_set(hw,
			port->rxqs[rxq]->id, port->pool_short->id);
	}

	return 0;
}

static int mv_pp2x_bm_update_mtu(struct net_device *dev, int mtu)
{
	struct mv_pp2x_port *port = netdev_priv(dev);
	struct mv_pp2x_bm_pool *old_long_port_pool = port->pool_long;
	struct mv_pp2x_bm_pool *old_short_port_pool = port->pool_short;
	struct mv_pp2x_hw *hw = &port->priv->hw;
	enum mv_pp2x_bm_pool_log_num new_long_pool, old_long_pool;
	enum mv_pp2x_bm_pool_log_num new_short_pool, old_short_pool;
	int rxq;
	int pkt_size = MVPP2_RX_PKT_SIZE(mtu);

	old_long_pool = old_long_port_pool->log_id;

	/* If port MTU is higher than 1518B:
	* HW Long pool - SW Jumbo pool, HW Short pool - SW Short pool
	* esle: HW Long pool - SW Long pool, HW Short pool - SW Short pool
	*/
	if (pkt_size > MVPP2_BM_LONG_PKT_SIZE) {
		new_long_pool = MVPP2_BM_SWF_JUMBO_POOL;
		new_short_pool = MVPP2_BM_SWF_LONG_POOL;
	} else {
		new_long_pool = MVPP2_BM_SWF_LONG_POOL;
		new_short_pool = MVPP2_BM_SWF_SHORT_POOL;
	}

	if (new_long_pool != old_long_pool) {
		/* Remove port from old short&long pool */
		mv_pp2x_bm_pool_stop_use(port, old_long_pool);
		old_long_port_pool->port_map &= ~(1 << port->id);

		mv_pp2x_bm_pool_stop_use(port, old_short_pool);
		old_short_port_pool->port_map &= ~(1 << port->id);

		/* Add port to new short&long pool */
		port->pool_long = mv_pp2x_bm_pool_use(port, new_long_pool);
		if (!port->pool_long)
			return -ENOMEM;
		port->pool_long->port_map |= (1 << port->id);
		for (rxq = 0; rxq < port->num_rx_queues; rxq++)
			port->priv->pp2xdata->mv_pp2x_rxq_long_pool_set(hw,
			port->rxqs[rxq]->id, port->pool_long->id);

		port->pool_short = mv_pp2x_bm_pool_use(port, new_short_pool);
		if (!port->pool_short)
			return -ENOMEM;
		port->pool_short->port_map |= (1 << port->id);
		for (rxq = 0; rxq < port->num_rx_queues; rxq++)
			port->priv->pp2xdata->mv_pp2x_rxq_short_pool_set(hw,
			port->rxqs[rxq]->id, port->pool_short->id);

		/* Update L4 checksum when jumbo enable/disable on port */
		if (new_long_pool == MVPP2_BM_SWF_JUMBO_POOL) {
			if (port->id != port->priv->l4_chksum_jumbo_port) {
				dev->features &=
					~(NETIF_F_IP_CSUM | NETIF_F_IPV6_CSUM);
				dev->hw_features &=
					~(NETIF_F_IP_CSUM | NETIF_F_IPV6_CSUM);
				}
		} else {
			dev->features |=
				(NETIF_F_IP_CSUM | NETIF_F_IPV6_CSUM);
			dev->hw_features |=
				(NETIF_F_IP_CSUM | NETIF_F_IPV6_CSUM);
		}
	}

	dev->mtu = mtu;

	dev->wanted_features = dev->features;
	netdev_update_features(dev);
	return 0;
}

/* Set defaults to the MVPP2 port */
static void mv_pp2x_defaults_set(struct mv_pp2x_port *port)
{
	int tx_port_num, val, queue, ptxq, lrxq;
	struct mv_pp2x_hw *hw = &port->priv->hw;

	/* Configure port to loopback if needed */
	if (port->flags & MVPP2_F_LOOPBACK) {
		if (port->priv->pp2_version == PPV21)
			mv_pp21_port_loopback_set(port);
	}

	/* Disable Legacy WRR, Disable EJP, Release from reset */
	tx_port_num = mv_pp2x_egress_port(port);
	mv_pp2x_write(hw, MVPP2_TXP_SCHED_PORT_INDEX_REG,
		      tx_port_num);
	mv_pp2x_write(hw, MVPP2_TXP_SCHED_CMD_1_REG, 0);

	/* Close bandwidth for all queues */
	for (queue = 0; queue < MVPP2_MAX_TXQ; queue++) {
		ptxq = mv_pp2x_txq_phys(port->id, queue);
		mv_pp2x_write(hw,
			      MVPP2_TXQ_SCHED_TOKEN_CNTR_REG(ptxq), 0);
	}

	/* Set refill period to 1 usec, refill tokens
	 * and bucket size to maximum
	 */
	mv_pp2x_write(hw, MVPP2_TXP_SCHED_PERIOD_REG,
		      hw->tclk / USEC_PER_SEC);
	val = mv_pp2x_read(hw, MVPP2_TXP_SCHED_REFILL_REG);
	val &= ~MVPP2_TXP_REFILL_PERIOD_ALL_MASK;
	val |= MVPP2_TXP_REFILL_PERIOD_MASK(1);
	val |= MVPP2_TXP_REFILL_TOKENS_ALL_MASK;
	mv_pp2x_write(hw, MVPP2_TXP_SCHED_REFILL_REG, val);
	val = MVPP2_TXP_TOKEN_SIZE_MAX;
	mv_pp2x_write(hw, MVPP2_TXP_SCHED_TOKEN_SIZE_REG, val);

	/* Set MaximumLowLatencyPacketSize value to 256 */
	mv_pp2x_write(hw, MVPP2_RX_CTRL_REG(port->id),
		      MVPP2_RX_USE_PSEUDO_FOR_CSUM_MASK |
		      MVPP2_RX_LOW_LATENCY_PKT_SIZE(256));

	/* Disable Rx cache snoop */
	for (lrxq = 0; lrxq < port->num_rx_queues; lrxq++) {
		queue = port->rxqs[lrxq]->id;
		val = mv_pp2x_read(hw, MVPP2_RXQ_CONFIG_REG(queue));
		val |= MVPP2_SNOOP_PKT_SIZE_MASK;
		val |= MVPP2_SNOOP_BUF_HDR_MASK;
		mv_pp2x_write(hw, MVPP2_RXQ_CONFIG_REG(queue), val);
	}

	/* Enable classifier high queue in forwarding port control*/
	val = mv_pp2x_read(hw, MVPP2_CLS_SWFWD_PCTRL_REG);
	val &= ~(MVPP2_CLS_SWFWD_PCTRL_MASK(port->id));
	mv_pp2x_write(hw, MVPP2_CLS_SWFWD_PCTRL_REG, val);

	/* At default, mask all interrupts to all present cpus */
	mv_pp2x_port_interrupts_disable(port);
}

static inline void *mv_pp2_extra_pool_get(struct mv_pp2x_port *port, int address_space)
{
	void *ext_buf;
	struct mv_pp2x_port_pcpu *port_pcpu = port->pcpu[address_space];
	struct mv_pp2x_ext_buf_struct *ext_buf_struct;

	if (!list_empty(&port_pcpu->ext_buf_port_list)) {
		ext_buf_struct = list_last_entry(&port_pcpu->ext_buf_port_list,
						 struct mv_pp2x_ext_buf_struct, ext_buf_list);
		list_del(&ext_buf_struct->ext_buf_list);
		port_pcpu->ext_buf_pool->buf_pool_in_use--;

		ext_buf = ext_buf_struct->ext_buf_data;

	} else {
		ext_buf = kmalloc(MVPP2_EXTRA_BUF_SIZE, GFP_ATOMIC);
	}

	return ext_buf;
}

static inline int mv_pp2_extra_pool_put(struct mv_pp2x_port *port, void *ext_buf,
					int cpu)
{
	struct mv_pp2x_port_pcpu *port_pcpu = port->pcpu[cpu];
	struct mv_pp2x_ext_buf_struct *ext_buf_struct;

	if (port_pcpu->ext_buf_pool->buf_pool_in_use >= port_pcpu->ext_buf_pool->buf_pool_size) {
		kfree(ext_buf);
		return 1;
	}
	port_pcpu->ext_buf_pool->buf_pool_in_use++;

	ext_buf_struct =
		&port_pcpu->ext_buf_pool->ext_buf_struct[port_pcpu->ext_buf_pool->buf_pool_next_free];
	mv_pp2x_extra_pool_inc(port_pcpu->ext_buf_pool);
	ext_buf_struct->ext_buf_data = ext_buf;

	list_add(&ext_buf_struct->ext_buf_list,
		 &port_pcpu->ext_buf_port_list);

	return 0;
}

static inline struct sk_buff *mv_pp2_skb_pool_get(struct mv_pp2x_port *port, int address_space)
{
	struct sk_buff *skb;
	struct mv_pp2x_cp_pcpu *cp_pcpu = port->priv->pcpu[address_space];
	struct mv_pp2x_skb_struct *skb_struct;

	if (!list_empty(&cp_pcpu->skb_port_list)) {
		skb_struct = list_last_entry(&cp_pcpu->skb_port_list,
					     struct mv_pp2x_skb_struct, skb_list);
		list_del(&skb_struct->skb_list);
		cp_pcpu->skb_pool->skb_pool_in_use--;

		skb = skb_struct->skb;

	} else {
		skb = NULL;
	}

	return skb;
}

static inline int mv_pp2_skb_pool_put(struct mv_pp2x_port *port, struct sk_buff *skb,
				      int cpu)
{
	struct mv_pp2x_cp_pcpu *cp_pcpu = port->priv->pcpu[cpu];
	struct mv_pp2x_skb_struct *skb_struct;

	if (cp_pcpu->skb_pool->skb_pool_in_use >= cp_pcpu->skb_pool->skb_pool_size) {
		dev_kfree_skb_any(skb);
		return 1;
	}
	cp_pcpu->skb_pool->skb_pool_in_use++;

	skb_struct =
		&cp_pcpu->skb_pool->skb_struct[cp_pcpu->skb_pool->skb_pool_next_free];
	mv_pp2x_skb_pool_inc(cp_pcpu->skb_pool);

	skb_struct->skb = skb;

	list_add(&skb_struct->skb_list,
		 &cp_pcpu->skb_port_list);

	return 0;
}

/* Check if there are enough reserved descriptors for transmission.
 * If not, request chunk of reserved descriptors and check again.
 */
int mv_pp2x_txq_reserved_desc_num_proc(
					struct mv_pp2x *priv,
					struct mv_pp2x_tx_queue *txq,
					struct mv_pp2x_txq_pcpu *txq_pcpu,
					int num, int cpu)
{
	int req;

	if (likely(txq_pcpu->reserved_num >= num))
		return 0;

	/* Not enough descriptors reserved! Update the reserved descriptor
	 * count and check again.
	 */

	/* Entire txq_size is used for SWF . Must be changed when HWF
	 * is implemented.
	 * There will always be at least one CHUNK available
	 */

	req = MVPP2_CPU_DESC_CHUNK;

	txq_pcpu->reserved_num += mv_pp2x_txq_alloc_reserved_desc(priv, txq,
							req, cpu);

	/* OK, the descriptor cound has been updated: check again. */
	if (unlikely(txq_pcpu->reserved_num < num))
		return -ENOMEM;

	return 0;
}

int mv_pp2x_tso_txq_reserved_desc_num_proc(
					struct mv_pp2x *priv,
					struct mv_pp2x_tx_queue *txq,
					struct mv_pp2x_txq_pcpu *txq_pcpu,
					int num, int cpu)
{
	int req, cpu_desc, desc_count;

	if (txq_pcpu->reserved_num >= num)
		return 0;

	/* Not enough descriptors reserved! Update the reserved descriptor
	 * count and check again.
	 */

	/* Entire txq_size is used for SWF . Must be changed when HWF
	 * is implemented.
	 * There will always be at least one CHUNK available
	 */

	req = MVPP2_CPU_DESC_CHUNK;

	if (unlikely((num - txq_pcpu->reserved_num) > req)) {
		req = num - txq_pcpu->reserved_num;
		desc_count = 0;
		/* Compute total of used descriptors */
		for (cpu_desc = 0; cpu_desc < mv_pp2x_used_addr_spaces; cpu_desc++) {
			int txq_count;
			struct mv_pp2x_txq_pcpu *txq_pcpu_aux;

			txq_pcpu_aux = &txq->pcpu[cpu_desc];
			txq_count = mv_pp2x_txq_count(txq_pcpu_aux);
			desc_count += txq_count;
		}
		desc_count += req;

		if (unlikely(desc_count > txq->size))
			return -ENOMEM;
	}

	txq_pcpu->reserved_num += mv_pp2x_txq_alloc_reserved_desc(priv, txq,
							req, cpu);

	/* OK, the descriptor cound has been updated: check again. */
	if (unlikely(txq_pcpu->reserved_num < num))
		return -ENOMEM;

	return 0;
}

/* Release the last allocated Tx descriptor. Useful to handle DMA
 * mapping failures in the Tx path.
 */

/* Free Tx queue skbuffs */
static void mv_pp2x_txq_bufs_free(struct mv_pp2x_port *port,
				  struct mv_pp2x_txq_pcpu *txq_pcpu,
				  int num)
{
	int i;

	for (i = 0; i < num; i++) {
		dma_addr_t buf_phys_addr =
				    txq_pcpu->tx_buffs[txq_pcpu->txq_get_index];
		uintptr_t skb = (uintptr_t)txq_pcpu->tx_skb[txq_pcpu->txq_get_index];
		int data_size = txq_pcpu->data_size[txq_pcpu->txq_get_index];
		struct sk_buff *skb_rec;

		txq_pcpu->tx_buffs[txq_pcpu->txq_get_index] = 0;
		txq_pcpu->tx_skb[txq_pcpu->txq_get_index] = 0;
		txq_pcpu->data_size[txq_pcpu->txq_get_index] = 0;

		if (skb & MVPP2_ETH_SHADOW_EXT) {
			/* Refill TSO external pool */
			skb &= ~MVPP2_ETH_SHADOW_EXT;
			mv_pp2_extra_pool_put(port, (void *)skb, txq_pcpu->cpu);
			mv_pp2x_txq_inc_get(txq_pcpu);
			dma_unmap_single(port->dev->dev.parent, buf_phys_addr,
					 data_size, DMA_TO_DEVICE);
			continue;
		} else if (skb & MVPP2_ETH_SHADOW_REC) {
			/* Release skb without data buffer, if data buffer were marked as
			 * recycled in TX routine.
			 */
			struct mv_pp2x_bm_pool *bm_pool;
			struct mv_pp2x_cp_pcpu *cp_pcpu = port->priv->pcpu[txq_pcpu->cpu];

			skb &= ~MVPP2_ETH_SHADOW_REC;
			skb_rec = (struct sk_buff *)skb;
			bm_pool = &port->priv->bm_pools[MVPP2X_SKB_BPID_GET(skb_rec)];
			cp_pcpu->in_use[bm_pool->id]--;
			/* Do not release buffer of recycled skb */
			if (!skb_irq_freeable(skb_rec)) {
				skb_rec->head = NULL;
				dev_kfree_skb_any(skb_rec);
			} else {
				skb_rec->head = NULL;
				mv_pp2_skb_pool_put(port, skb_rec, txq_pcpu->cpu);
			}
			mv_pp2x_txq_inc_get(txq_pcpu);
			continue;
		}

		dma_unmap_single(port->dev->dev.parent, buf_phys_addr,
				 data_size, DMA_TO_DEVICE);

		if (skb & MVPP2_ETH_SHADOW_SKB) {
			skb &= ~MVPP2_ETH_SHADOW_SKB;
			dev_kfree_skb_any((struct sk_buff *)skb);
		}
		mv_pp2x_txq_inc_get(txq_pcpu);
	}
}

static void mv_pp2x_txq_buf_free(struct mv_pp2x_port *port, uintptr_t skb,
				 dma_addr_t  buf_phys_addr, int data_size,
				 int cpu)
{
	dma_unmap_single(port->dev->dev.parent, buf_phys_addr,
			 data_size, DMA_TO_DEVICE);

	if (skb & MVPP2_ETH_SHADOW_EXT) {
		skb &= ~MVPP2_ETH_SHADOW_EXT;
		mv_pp2_extra_pool_put(port, (void *)skb, cpu);
		return;
	}

	if (skb & MVPP2_ETH_SHADOW_SKB) {
		skb &= ~MVPP2_ETH_SHADOW_SKB;
		dev_kfree_skb_any((struct sk_buff *)skb);
	}
}

/* Handle end of transmission */
static void mv_pp2x_txq_done(struct mv_pp2x_port *port,
			     struct mv_pp2x_tx_queue *txq,
				   struct mv_pp2x_txq_pcpu *txq_pcpu)
{
	struct netdev_queue *nq = netdev_get_tx_queue(port->dev, (txq->log_id +
						     (txq_pcpu->cpu * mv_pp2x_txq_number)));
	int tx_done;

#ifdef DEV_NETMAP
	if (port->flags & MVPP2_F_IFCAP_NETMAP) {
		if (netmap_tx_irq(port->dev, 0))
			return; /* cleaned ok */
	}
#endif /* DEV_NETMAP */

	tx_done = mv_pp2x_txq_sent_desc_proc(port, txq_pcpu->cpu, txq->id);
	if (!tx_done)
		return;

	mv_pp2x_txq_bufs_free(port, txq_pcpu, tx_done);

	if (netif_tx_queue_stopped(nq))
		if (mv_pp2x_txq_free_count(txq_pcpu) >= port->txq_stop_limit)
			netif_tx_wake_queue(nq);
}

static unsigned int mv_pp2x_tx_done(struct mv_pp2x_port *port, u32 cause,
				    int address_space)
{
	struct mv_pp2x_tx_queue *txq;
	struct mv_pp2x_txq_pcpu *txq_pcpu;
	unsigned int tx_todo = 0;

	while (cause) {
		int txq_count;

		txq = mv_pp2x_get_tx_queue(port, cause);
		if (!txq)
			break;

		txq_pcpu = &txq->pcpu[address_space];
		txq_count = mv_pp2x_txq_count(txq_pcpu);

		if (txq_count) {
			mv_pp2x_txq_done(port, txq, txq_pcpu);
			/*Recalc after tx_done*/
			txq_count = mv_pp2x_txq_count(txq_pcpu);

			tx_todo += txq_count;
		}

		cause &= ~(1 << txq->log_id);
	}
	return tx_todo;
}

/* Rx/Tx queue initialization/cleanup methods */

/* Allocate and initialize descriptors for aggr TXQ */
static int mv_pp2x_aggr_txq_init(struct platform_device *pdev,
				 struct mv_pp2x_aggr_tx_queue *aggr_txq,
				      int desc_num, int cpu,
				      struct mv_pp2x *priv)
{
	struct mv_pp2x_hw *hw = &priv->hw;
	dma_addr_t first_desc_phy;

	/* Allocate memory for TX descriptors, ensure it can be 512B aligned. */
	aggr_txq->desc_mem = dma_zalloc_coherent(&pdev->dev,
		MVPP2_DESCQ_MEM_SIZE(desc_num),
		&aggr_txq->descs_phys, GFP_KERNEL);
	if (!aggr_txq->desc_mem)
		return -ENOMEM;

	aggr_txq->first_desc = (struct mv_pp2x_tx_desc *)
			MVPP2_DESCQ_MEM_ALIGN((uintptr_t)aggr_txq->desc_mem);
	first_desc_phy = MVPP2_DESCQ_MEM_ALIGN(aggr_txq->descs_phys);

	pr_debug("first_desc=%p, desc_mem=%p\n",
		 aggr_txq->desc_mem, aggr_txq->first_desc);

	aggr_txq->last_desc = aggr_txq->size - 1;

	/* Aggr TXQ no reset WA */
	aggr_txq->next_desc_to_proc = mv_pp2x_read(hw,
						MVPP2_AGGR_TXQ_INDEX_REG(cpu));

	/* Set Tx descriptors queue starting address
	 * indirect access
	 */
	mv_pp2x_write(hw, MVPP2_AGGR_TXQ_DESC_ADDR_REG(cpu),
		      first_desc_phy >>
		      priv->pp2xdata->hw.desc_queue_addr_shift);
	mv_pp2x_write(hw, MVPP2_AGGR_TXQ_DESC_SIZE_REG(cpu), desc_num);

	return 0;
}

/* Create a specified Rx queue */
static int mv_pp2x_rxq_init(struct mv_pp2x_port *port,
			    struct mv_pp2x_rx_queue *rxq)
{
	struct mv_pp2x_hw *hw = &port->priv->hw;
	dma_addr_t first_desc_phy;

	rxq->size = port->rx_ring_size;

	/* Allocate memory for RX descriptors */
	rxq->desc_mem = dma_alloc_coherent(port->dev->dev.parent,
					   MVPP2_DESCQ_MEM_SIZE(rxq->size),
					   &rxq->descs_phys, GFP_KERNEL);
	if (!rxq->desc_mem)
		return -ENOMEM;

	rxq->first_desc = (struct mv_pp2x_rx_desc *)
		MVPP2_DESCQ_MEM_ALIGN((uintptr_t)rxq->desc_mem);
	first_desc_phy  = MVPP2_DESCQ_MEM_ALIGN(rxq->descs_phys);

	rxq->last_desc = rxq->size - 1;

	/* Zero occupied and non-occupied counters - direct access */
	mv_pp2x_write(hw, MVPP2_RXQ_STATUS_REG(rxq->id), 0);

	/* Set Rx descriptors queue starting address - indirect access */
	mv_pp2x_write(hw, MVPP2_RXQ_NUM_REG, rxq->id);

	mv_pp2x_write(hw, MVPP2_RXQ_DESC_ADDR_REG, (first_desc_phy >>
		      port->priv->pp2xdata->hw.desc_queue_addr_shift));
	mv_pp2x_write(hw, MVPP2_RXQ_DESC_SIZE_REG, rxq->size);
	mv_pp2x_write(hw, MVPP2_RXQ_INDEX_REG, 0);

	/* Set Offset */
	mv_pp2x_rxq_offset_set(port, rxq->id, NET_SKB_PAD);

	/* Set coalescing pkts and time */
	mv_pp2x_rx_pkts_coal_set(port, rxq);
	mv_pp2x_rx_time_coal_set(port, rxq);

	/* Add number of descriptors ready for receiving packets */
	mv_pp2x_rxq_status_update(port, rxq->id, 0, rxq->size);

	return 0;
}

/* Push packets received by the RXQ to BM pool */
static void mv_pp2x_rxq_drop_pkts(struct mv_pp2x_port *port,
				  struct mv_pp2x_rx_queue *rxq)
{
	int rx_received, i, cpu;
	u8 *buf_cookie;
	dma_addr_t buf_phys_addr;
	struct mv_pp2x_bm_pool *bm_pool;
	struct mv_pp2x_cp_pcpu *cp_pcpu;

	preempt_disable();
	rx_received = mv_pp2x_rxq_received(port, rxq->id);
	preempt_enable();
	if (!rx_received)
		return;

	cpu = get_cpu();
	cp_pcpu = port->priv->pcpu[cpu];
	for (i = 0; i < rx_received; i++) {
		struct mv_pp2x_rx_desc *rx_desc =
			mv_pp2x_rxq_next_desc_get(rxq);

#if defined(__BIG_ENDIAN)
		if (port->priv->pp2_version == PPV21)
			mv_pp21_rx_desc_swap(rx_desc);
		else
			mv_pp22_rx_desc_swap(rx_desc);
#endif /* __BIG_ENDIAN */

		bm_pool = &port->priv->bm_pools[MVPP2_RX_DESC_POOL(rx_desc)];

		if (port->priv->pp2_version == PPV21)
			buf_phys_addr = mv_pp21_rxdesc_phys_addr_get(rx_desc);
		else
			buf_phys_addr = mv_pp22_rxdesc_phys_addr_get(rx_desc);

		buf_cookie = phys_to_virt(dma_to_phys(port->dev->dev.parent, buf_phys_addr));

		mv_pp2x_pool_refill(port->priv, MVPP2_RX_DESC_POOL(rx_desc),
				    buf_phys_addr, cpu);
		cp_pcpu->in_use[bm_pool->id]--;
	}
	put_cpu();
	mv_pp2x_rxq_status_update(port, rxq->id, rx_received, rx_received);
}

/* Cleanup Rx queue */
static void mv_pp2x_rxq_deinit(struct mv_pp2x_port *port,
			       struct mv_pp2x_rx_queue *rxq)
{
	struct mv_pp2x_hw *hw = &port->priv->hw;

	mv_pp2x_rxq_drop_pkts(port, rxq);

	if (rxq->desc_mem)
		dma_free_coherent(port->dev->dev.parent,
				  MVPP2_DESCQ_MEM_SIZE(rxq->size),
				  rxq->desc_mem,
				  rxq->descs_phys);

	rxq->first_desc		= NULL;
	rxq->desc_mem		= NULL;
	rxq->last_desc		= 0;
	rxq->next_desc_to_proc  = 0;
	rxq->descs_phys         = 0;

	/* Clear Rx descriptors queue starting address and size;
	 * free descriptor number
	 */
	mv_pp2x_write(hw, MVPP2_RXQ_STATUS_REG(rxq->id), 0);
	mv_pp2x_write(hw, MVPP2_RXQ_NUM_REG, rxq->id);
	mv_pp2x_write(hw, MVPP2_RXQ_DESC_ADDR_REG, 0);
	mv_pp2x_write(hw, MVPP2_RXQ_DESC_SIZE_REG, 0);
}

/* Create and initialize a Tx queue */
static int mv_pp2x_txq_init(struct mv_pp2x_port *port,
			    struct mv_pp2x_tx_queue *txq)
{
	u32 val;
	int cpu, desc, desc_per_txq, tx_port_num;
	struct mv_pp2x_hw *hw = &port->priv->hw;
	struct mv_pp2x_txq_pcpu *txq_pcpu;
	dma_addr_t first_desc_phy;

	txq->size = port->tx_ring_size;

	/* Allocate memory for Tx descriptors */
	txq->desc_mem = dma_alloc_coherent(port->dev->dev.parent,
					   MVPP2_DESCQ_MEM_SIZE(txq->size),
					   &txq->descs_phys, GFP_KERNEL);
	if (!txq->desc_mem)
		return -ENOMEM;

	txq->first_desc = (struct mv_pp2x_tx_desc *)
		MVPP2_DESCQ_MEM_ALIGN((uintptr_t)txq->desc_mem);
	first_desc_phy  = MVPP2_DESCQ_MEM_ALIGN(txq->descs_phys);

	txq->last_desc = txq->size - 1;

	/* Set Tx descriptors queue starting address - indirect access */
	mv_pp2x_write(hw, MVPP2_TXQ_NUM_REG, txq->id);
	mv_pp2x_write(hw, MVPP2_TXQ_DESC_ADDR_LOW_REG,
		      first_desc_phy >> MVPP2_TXQ_DESC_ADDR_LOW_SHIFT);
	mv_pp2x_write(hw, MVPP2_TXQ_DESC_SIZE_REG,
		      txq->size & MVPP2_TXQ_DESC_SIZE_MASK);
	mv_pp2x_write(hw, MVPP2_TXQ_INDEX_REG, 0);
	mv_pp2x_write(hw, MVPP2_TXQ_RSVD_CLR_REG,
		      txq->id << MVPP2_TXQ_RSVD_CLR_OFFSET);
	val = mv_pp2x_read(hw, MVPP2_TXQ_PENDING_REG);
	val &= ~MVPP2_TXQ_PENDING_MASK;
	mv_pp2x_write(hw, MVPP2_TXQ_PENDING_REG, val);

	/* Calculate base address in prefetch buffer. We reserve 16 descriptors
	 * for each existing TXQ.
	 * TCONTS for PON port must be continuous from 0 to MVPP2_MAX_TCONT
	 * GBE ports assumed to be continious from 0 to MVPP2_MAX_PORTS
	 */
	desc_per_txq = 16;
	desc = (port->id * MVPP2_MAX_TXQ * desc_per_txq) +
	       (txq->log_id * desc_per_txq);

	mv_pp2x_write(hw, MVPP2_TXQ_PREF_BUF_REG,
		      MVPP2_PREF_BUF_PTR(desc) | MVPP2_PREF_BUF_SIZE_16 |
		      MVPP2_PREF_BUF_THRESH(desc_per_txq / 2));

	/* WRR / EJP configuration - indirect access */
	tx_port_num = mv_pp2x_egress_port(port);
	mv_pp2x_write(hw, MVPP2_TXP_SCHED_PORT_INDEX_REG, tx_port_num);

	val = mv_pp2x_read(hw, MVPP2_TXQ_SCHED_REFILL_REG(txq->log_id));
	val &= ~MVPP2_TXQ_REFILL_PERIOD_ALL_MASK;
	val |= MVPP2_TXQ_REFILL_PERIOD_MASK(1);
	val |= MVPP2_TXQ_REFILL_TOKENS_ALL_MASK;
	mv_pp2x_write(hw, MVPP2_TXQ_SCHED_REFILL_REG(txq->log_id), val);

	val = MVPP2_TXQ_TOKEN_SIZE_MAX;
	mv_pp2x_write(hw, MVPP2_TXQ_SCHED_TOKEN_SIZE_REG(txq->log_id),
		      val);

	for (cpu = 0; cpu < mv_pp2x_used_addr_spaces; cpu++) {
		txq_pcpu = &txq->pcpu[cpu];
		txq_pcpu->size = txq->size;
		txq_pcpu->tx_skb = kcalloc(txq_pcpu->size,
					   sizeof(*txq_pcpu->tx_skb),
					   GFP_KERNEL);
		if (!txq_pcpu->tx_skb)
			goto error;

		txq_pcpu->tx_buffs = kcalloc(txq_pcpu->size,
					     sizeof(dma_addr_t), GFP_KERNEL);
		if (!txq_pcpu->tx_buffs)
			goto error;

		txq_pcpu->data_size = kcalloc(txq_pcpu->size,
						sizeof(int), GFP_KERNEL);
		if (!txq_pcpu->data_size)
			goto error;

		txq_pcpu->reserved_num = 0;
		txq_pcpu->txq_put_index = 0;
		txq_pcpu->txq_get_index = 0;
	}

	return 0;

error:
	for (cpu = 0; cpu < mv_pp2x_used_addr_spaces; cpu++) {
		txq_pcpu = &txq->pcpu[cpu];
		kfree(txq_pcpu->tx_skb);
		kfree(txq_pcpu->tx_buffs);
		kfree(txq_pcpu->data_size);
	}

	dma_free_coherent(port->dev->dev.parent,
			  MVPP2_DESCQ_MEM_SIZE(txq->size),
			  txq->first_desc, txq->descs_phys);

	return -ENOMEM;
}

/* Free allocated TXQ resources */
static void mv_pp2x_txq_deinit(struct mv_pp2x_port *port,
			       struct mv_pp2x_tx_queue *txq)
{
	struct mv_pp2x_txq_pcpu *txq_pcpu;
	struct mv_pp2x_hw *hw = &port->priv->hw;
	int cpu;

	for (cpu = 0; cpu < mv_pp2x_used_addr_spaces; cpu++) {
		preempt_disable();
		txq_pcpu = &txq->pcpu[cpu];
		kfree(txq_pcpu->tx_skb);
		kfree(txq_pcpu->tx_buffs);
		kfree(txq_pcpu->data_size);
		preempt_enable();
	}

	if (txq->desc_mem)
		dma_free_coherent(port->dev->dev.parent,
				  MVPP2_DESCQ_MEM_SIZE(txq->size),
				  txq->desc_mem, txq->descs_phys);

	txq->desc_mem		= NULL;
	txq->first_desc		= NULL;
	txq->last_desc		= 0;
	txq->next_desc_to_proc	= 0;
	txq->descs_phys		= 0;

	/* Set minimum bandwidth for disabled TXQs */
	mv_pp2x_write(hw, MVPP2_TXQ_SCHED_TOKEN_CNTR_REG(txq->id), 0);

	/* Set Tx descriptors queue starting address and size */
	mv_pp2x_write(hw, MVPP2_TXQ_NUM_REG, txq->id);
	mv_pp2x_write(hw, MVPP2_TXQ_DESC_ADDR_LOW_REG, 0);
	mv_pp2x_write(hw, MVPP2_TXQ_DESC_SIZE_REG, 0);
}

/* Cleanup Tx ports */
static void mv_pp2x_txq_clean(struct mv_pp2x_port *port,
			      struct mv_pp2x_tx_queue *txq)
{
	struct mv_pp2x_txq_pcpu *txq_pcpu;
	int delay, pending, cpu;
	u32 val;
	bool egress_en = false;
	struct mv_pp2x_hw *hw = &port->priv->hw;
	int tx_port_num = mv_pp2x_egress_port(port);

	mv_pp2x_write(hw, MVPP2_TXQ_NUM_REG, txq->id);
	val = mv_pp2x_read(hw, MVPP2_TXQ_PREF_BUF_REG);
	val |= MVPP2_TXQ_DRAIN_EN_MASK;
	mv_pp2x_write(hw, MVPP2_TXQ_PREF_BUF_REG, val);

	/* The napi queue has been stopped so wait for all packets
	 * to be transmitted.
	 */

	/* Enable egress queue in order to allow releasing all packets*/
	mv_pp2x_write(hw, MVPP2_TXP_SCHED_PORT_INDEX_REG, tx_port_num);
	val = mv_pp2x_read(hw, MVPP2_TXP_SCHED_Q_CMD_REG);
	if (!(val & (1 << txq->log_id))) {
		val |= 1 << txq->log_id;
		mv_pp2x_write(hw, MVPP2_TXP_SCHED_Q_CMD_REG, val);
		egress_en = true;
	}

	delay = 0;
	do {
		if (delay >= MVPP2_TX_PENDING_TIMEOUT_MSEC) {
			netdev_warn(port->dev,
				    "port %d: cleaning queue %d timed out\n",
				    port->id, txq->log_id);
			break;
		}
		mdelay(1);
		delay++;

		pending = mv_pp2x_txq_pend_desc_num_get(port, txq);
	} while (pending);

	/* Disable egress queue */
	if (egress_en) {
		mv_pp2x_write(hw, MVPP2_TXP_SCHED_PORT_INDEX_REG, tx_port_num);
		val = (mv_pp2x_read(hw, MVPP2_TXP_SCHED_Q_CMD_REG)) &
			    MVPP2_TXP_SCHED_ENQ_MASK;
		val |= 1 << txq->log_id;
		mv_pp2x_write(hw, MVPP2_TXP_SCHED_Q_CMD_REG,
			      (val << MVPP2_TXP_SCHED_DISQ_OFFSET));
		egress_en = false;
	}

	val &= ~MVPP2_TXQ_DRAIN_EN_MASK;
	mv_pp2x_write(hw, MVPP2_TXQ_PREF_BUF_REG, val);

	for (cpu = 0; cpu < mv_pp2x_used_addr_spaces; cpu++) {
		int txq_count;

		preempt_disable();
		txq_pcpu = &txq->pcpu[cpu];

		/* Release all packets */
		txq_count = mv_pp2x_txq_count(txq_pcpu);
		mv_pp2x_txq_bufs_free(port, txq_pcpu, txq_count);

		/* Reset queue */
		txq_pcpu->txq_put_index = 0;
		txq_pcpu->txq_get_index = 0;
		preempt_enable();
	}
}

/* Cleanup all Tx queues */
void mv_pp2x_cleanup_txqs(struct mv_pp2x_port *port)
{
	struct mv_pp2x_tx_queue *txq;
	int queue, cpu;
	u32 val;
	struct mv_pp2x_hw *hw = &port->priv->hw;

	val = mv_pp2x_read(hw, MVPP2_TX_PORT_FLUSH_REG);

	/* Reset Tx ports and delete Tx queues */
	val |= MVPP2_TX_PORT_FLUSH_MASK(port->id);
	mv_pp2x_write(hw, MVPP2_TX_PORT_FLUSH_REG, val);

	for (queue = 0; queue < port->num_tx_queues; queue++) {
		txq = port->txqs[queue];
		mv_pp2x_txq_clean(port, txq);
		mv_pp2x_txq_deinit(port, txq);
	}

	/* Mvpp21 and Mvpp22 has different per cpu register access.
	* Mvpp21 - to access CPUx should run on CPUx
	* Mvpp22 - CPUy can access CPUx from CPUx address space
	* If added to support CPU hot plug feature supported only by Mvpp22
	*/
	if (port->priv->pp2_version == PPV22)
		for (cpu = 0; cpu < mv_pp2x_used_addr_spaces; cpu++)
			for (queue = 0; queue < port->num_tx_queues; queue++)
				mv_pp2x_txq_sent_desc_proc(port, cpu, port->txqs[queue]->id);
	else
		on_each_cpu(mv_pp2x_txq_sent_counter_clear, port, 1);

	val &= ~MVPP2_TX_PORT_FLUSH_MASK(port->id);
	mv_pp2x_write(hw, MVPP2_TX_PORT_FLUSH_REG, val);
}

/* Cleanup all Rx queues */
void mv_pp2x_cleanup_rxqs(struct mv_pp2x_port *port)
{
	int queue;

	for (queue = 0; queue < port->num_rx_queues; queue++)
		mv_pp2x_rxq_deinit(port, port->rxqs[queue]);
}

/* Init all Rx queues for port */
int mv_pp2x_setup_rxqs(struct mv_pp2x_port *port)
{
	int queue, err;

	for (queue = 0; queue < port->num_rx_queues; queue++) {
		err = mv_pp2x_rxq_init(port, port->rxqs[queue]);
		if (err)
			goto err_cleanup;
	}
	return 0;

err_cleanup:
	mv_pp2x_cleanup_rxqs(port);
	return err;
}

/* Init all tx queues for port */
int mv_pp2x_setup_txqs(struct mv_pp2x_port *port)
{
	struct mv_pp2x_tx_queue *txq;
	int queue, err, cpu;

	for (queue = 0; queue < port->num_tx_queues; queue++) {
		txq = port->txqs[queue];
		err = mv_pp2x_txq_init(port, txq);
		if (err)
			goto err_cleanup;
	}
	if (port->interrupt_tx_done) {
		mv_pp2x_tx_done_time_coal_set(port, port->tx_time_coal);
		mv_pp2x_tx_done_pkts_coal_set_all(port);
	}

	/* Mvpp21 and Mvpp22 has different per cpu register access.
	* Mvpp21 - to access CPUx should run on CPUx
	* Mvpp22 - CPUy can access CPUx from CPUx address space
	* If added to support CPU hot plug feature supported only by Mvpp22
	*/

	if (port->priv->pp2_version == PPV22)
		for (cpu = 0; cpu < mv_pp2x_used_addr_spaces; cpu++)
			for (queue = 0; queue < port->num_tx_queues; queue++)
				mv_pp2x_txq_sent_desc_proc(port, cpu, port->txqs[queue]->id);
	else
		on_each_cpu(mv_pp2x_txq_sent_counter_clear, port, 1);

	return 0;

err_cleanup:
	mv_pp2x_cleanup_txqs(port);
	return err;
}

void mv_pp2x_cleanup_irqs(struct mv_pp2x_port *port)
{
	int qvec;

	/* Rx/TX irq's */
	for (qvec = 0; qvec < port->num_qvector; qvec++) {
		irq_set_affinity_hint(port->q_vector[qvec].irq, NULL);
		free_irq(port->q_vector[qvec].irq, &port->q_vector[qvec]);
	}

	/* Link irq */
	if (port->priv->pp2_version == PPV22)
		free_irq(port->mac_data.link_irq, port);
}

void mv_pp2x_interrupts_set_mask(struct queue_vector *q_vec, u32 queue_mask)
{
	struct mv_pp2x_port *port = q_vec->parent;

	mv_pp22_thread_write(&port->priv->hw, q_vec->sw_thread_id, MVPP2_ISR_RX_TX_CAUSE_MASK_REG(port->id),
			     queue_mask);
}

void mv_pp2x_tx_done_pkts_coal_set_all(struct mv_pp2x_port *port)
{
	int i;

	for (i = 0; i < mv_pp2x_used_addr_spaces; i++)
		mv_pp2x_tx_done_pkts_coal_set(port, i);
}

/**
 * \brief wrapper for calling napi_schedule
 * @param param parameters to pass to napi_schedule
 *
 * Used when scheduling on different CPUs
 */
static void napi_schedule_wrapper(void *param)
{
	struct napi_struct *napi = param;

	napi_schedule(napi);
}

/* The callback for per-q_vector interrupt */
static irqreturn_t mv_pp2x_isr(int irq, void *dev_id)
{
	struct queue_vector *q_vec = (struct queue_vector *)dev_id;

	/* In single mode only first sub queue vector do RX and TX done interrupt routine
	 * so its save to disable Interrupt.
	 * In multi queue mode only if CPU's share same interrupt, cause mask related to this
	 * CPU queues and NAPI scheduled on this CPU.
	 */
	if (!q_vec->num_of_sub_vectors  || mv_pp2x_queue_mode == MVPP2_QDIST_SINGLE_MODE) {
		mv_pp2x_qvector_interrupt_disable(q_vec);
		napi_schedule(&q_vec->sub_vec[0]->napi);
	} else {
		u32 cause_rx_tx;
		int i;
		unsigned long flags;
		struct mv_pp2x_port *port = q_vec->parent;

		/* Check interrupt cause and which CPU should handle NAPI */
		cause_rx_tx = mv_pp22_thread_relaxed_read(&port->priv->hw, q_vec->sw_thread_id,
							  MVPP2_ISR_RX_TX_CAUSE_REG(port->id));
		for (i = 0; i < q_vec->num_of_sub_vectors; i++) {
			/* Find which sub queue vector should handle Interrupt cause and
			 * remove this cause from queue vector cause.
			 * It's done to protect scheduling of NAPI twice due to same cause.
			 */
			if ((cause_rx_tx & q_vec->sub_vec[i]->queue_mask) &&
			    (q_vec->queue_mask & q_vec->sub_vec[i]->queue_mask)) {
				spin_lock_irqsave(&q_vec->hw_mask_lock, flags);
				/* HW_MASK shadow kept in SW, faster for Read-Modify-Write */
				q_vec->queue_mask &= (~q_vec->sub_vec[i]->queue_mask);
				mv_pp2x_interrupts_set_mask(q_vec, q_vec->queue_mask);
				spin_unlock_irqrestore(&q_vec->hw_mask_lock, flags);
			} else {
					continue;
			}

			if (q_vec->sub_vec[i]->cpu_id == smp_processor_id()) {
				napi_schedule(&q_vec->sub_vec[i]->napi);
			} else {
				struct call_single_data *csd = &q_vec->sub_vec[i]->csd;

				csd->func = napi_schedule_wrapper;
				csd->info = &q_vec->sub_vec[i]->napi;
				csd->flags = 0;
				smp_call_function_single_async(q_vec->sub_vec[i]->cpu_id, csd);
			}
		}
	}

	return IRQ_HANDLED;
}

static irqreturn_t mv_pp2_link_change_isr(int irq, void *data)
{
	struct mv_pp2x_port *port = (struct mv_pp2x_port *)data;

	pr_debug("%s cpu_id(%d) irq(%d) pp_port(%d)\n", __func__,
		 smp_processor_id(), irq, port->id);
	if (port->priv->pp2_version == PPV22) {
		/* mask all events from this mac */
		mv_gop110_port_events_mask(&port->priv->hw.gop, &port->mac_data);
		/* read cause register to clear event */
		mv_gop110_port_events_clear(&port->priv->hw.gop, &port->mac_data);
	}

	if (!port->mac_data.phy_dev)
		tasklet_schedule(&port->link_change_tasklet);

	return IRQ_HANDLED;
}

/* Routine configure irq's.
 * Exist 3 type of irq's - MVPP2_PRIVATE, MVPP2_TX_SHARED and MVPP2_RX_SHARED
 * MVPP2_PRIVATE - in multi queue mode used for TX done and RX
 * CPU that use MVPP2_PRIVATE in multi queue mode called hot CPU's
 * in single queue mode used only for TX done
 * If there are 8 or less system, each cpu will has his own Interrupt.
 * If there are more than 8 CPU's, number of CPU's will share same interrupt.
 * MVPP2_RX_SHARED - exist only in single queue mode. In this mode all cpumask
 * used. Interrupt could be handled by any CPU. RX routine always handled by single interrupt
 * and single thread.
 * MVPP2_TX_SHARED - exist only in multi queue mode. This interrupt handle TX done of cold CPU's
 * IRQ balancing allowed only for single queue mode with in system with 8 or less CPU's.
 */
int mv_pp2x_setup_irqs(struct net_device *dev, struct mv_pp2x_port *port)
{
	int qvec_id, cpu, err;
	struct queue_vector *qvec;

	/* Rx/TX irq's */
	for (qvec_id = 0; qvec_id < port->num_qvector; qvec_id++) {
		qvec = &port->q_vector[qvec_id];
		if (!qvec->irq)
			continue;
		err = request_irq(qvec->irq, mv_pp2x_isr, 0,
				  qvec->irq_name, qvec);
		pr_debug("%s interrupt request\n", qvec->irq_name);
		if (qvec->qv_type == MVPP2_PRIVATE) {
			if (!qvec->num_of_sub_vectors) {
				cpu = QV_THR_2_MASTER_AP_CPU(qvec->ap_id, qvec->sw_thread_id);
				irq_set_affinity_hint(qvec->irq, cpumask_of(cpu));
			} else {
				cpumask_t cpus;

				cpumask_clear(&cpus);
				if (port->priv->pp2_cfg.queue_mode ==
							MVPP2_QDIST_MULTI_MODE)
					for (cpu = 0; cpu < qvec->num_of_sub_vectors; cpu++)
						cpumask_set_cpu(qvec->sub_vec[cpu]->cpu_id, &cpus);
				else
					for (cpu = 0; cpu < qvec->num_of_sub_vectors; cpu++)
						cpumask_set_cpu(QV_THR_2_MASTER_AP_CPU(qvec->ap_id, cpu), &cpus);
				irq_set_affinity_hint(qvec->irq, &cpus);
			}
			if ((port->priv->pp2_cfg.queue_mode ==
				MVPP2_QDIST_MULTI_MODE) || qvec->num_of_sub_vectors)
				irq_set_status_flags(qvec->irq, IRQ_NO_BALANCING);
		} else if (qvec->qv_type == MVPP2_TX_SHARED) {
			cpumask_t cpus;

			cpumask_clear(&cpus);
			for (cpu = port->priv->rx_count; cpu < num_active_cpus(); cpu++)
				cpumask_set_cpu(cpu, &cpus);
			irq_set_affinity_hint(qvec->irq, &cpus);
			irq_set_status_flags(qvec->irq, IRQ_NO_BALANCING);
		}
		if (err) {
			netdev_err(dev, "cannot request IRQ %d\n",
				   qvec->irq);
			goto err_cleanup;
		}
	}
	/* Link irq */
	if (port->mac_data.link_irq != MVPP2_NO_LINK_IRQ) {
		pr_debug("%s interrupt request\n", port->mac_data.irq_name);
		err = request_irq(port->mac_data.link_irq,
				  mv_pp2_link_change_isr, 0,
				  port->mac_data.irq_name, port);
		if (err) {
			netdev_err(dev, "cannot request IRQ %d\n",
				   port->mac_data.link_irq);
			goto err_cleanup;
		}
		if (!dev->irq)
			dev->irq = port->mac_data.link_irq;
	}
	return 0;
err_cleanup:
	mv_pp2x_cleanup_irqs(port);
	return err;
}

/* Adjust link */

/* Called from link_tasklet */
static void mv_pp22_dev_link_event(struct net_device *dev)
{
	bool link_is_up;

	struct mv_pp2x_port *port = netdev_priv(dev);
	struct gop_hw *gop = &port->priv->hw.gop;

	if (port->priv->pp2_version == PPV21)
		return;

	/* Check Link status on ethernet port */
	link_is_up = mv_gop110_port_is_link_up(gop, &port->mac_data);

	if (link_is_up) {
		if (netif_carrier_ok(dev))
			return;
		netif_carrier_on(dev);
		if (!(port->flags & MVPP2_F_IF_MUSDK))
			netif_tx_wake_all_queues(dev);
		netdev_info(dev, "link up\n");
		port->mac_data.flags |= MV_EMAC_F_LINK_UP;
	} else {
		if (!netif_carrier_ok(dev))
			return;
		netif_carrier_off(dev);
		netif_tx_stop_all_queues(dev);
		netdev_info(dev, "link down\n");
		port->mac_data.flags &= ~MV_EMAC_F_LINK_UP;
	}
}

/* Called from phy_lib */
static void mv_pp21_link_event(struct net_device *dev)
{
	struct mv_pp2x_port *port = netdev_priv(dev);
	struct phy_device *phydev = port->mac_data.phy_dev;
	int status_change = 0;
	u32 val;

	if (!phydev)
		return;

	if (phydev->link) {
		if ((port->mac_data.speed != phydev->speed) ||
		    (port->mac_data.duplex != phydev->duplex)) {
			u32 val;

			val = readl(port->base + MVPP2_GMAC_AUTONEG_CONFIG);
			val &= ~(MVPP2_GMAC_CONFIG_MII_SPEED |
				 MVPP2_GMAC_CONFIG_GMII_SPEED |
				 MVPP2_GMAC_CONFIG_FULL_DUPLEX |
				 MVPP2_GMAC_AN_SPEED_EN |
				 MVPP2_GMAC_AN_DUPLEX_EN);

			if (phydev->duplex)
				val |= MVPP2_GMAC_CONFIG_FULL_DUPLEX;

			if (phydev->speed == SPEED_1000)
				val |= MVPP2_GMAC_CONFIG_GMII_SPEED;
			else if (phydev->speed == SPEED_100)
				val |= MVPP2_GMAC_CONFIG_MII_SPEED;

			writel(val, port->base + MVPP2_GMAC_AUTONEG_CONFIG);

			port->mac_data.duplex = phydev->duplex;
			port->mac_data.speed  = phydev->speed;
		}
	}

	if (phydev->link != port->mac_data.link) {
		if (!phydev->link) {
			port->mac_data.duplex = -1;
			port->mac_data.speed = 0;
		}

		port->mac_data.link = phydev->link;
		status_change = 1;
	}

	if (status_change) {
		if (phydev->link) {
			val = readl(port->base + MVPP2_GMAC_AUTONEG_CONFIG);
			val |= (MVPP2_GMAC_FORCE_LINK_PASS |
				MVPP2_GMAC_FORCE_LINK_DOWN);
			writel(val, port->base + MVPP2_GMAC_AUTONEG_CONFIG);
			mv_pp2x_egress_enable(port);
			mv_pp2x_ingress_enable(port);
		} else {
			mv_pp2x_ingress_disable(port);
			mv_pp2x_egress_disable(port);
		}
	}
}

static void mv_pp22_link_event(struct net_device *dev)
{
	struct mv_pp2x_port *port = netdev_priv(dev);
	struct phy_device *phydev = port->mac_data.phy_dev;
	int status_change = 0;

	if (!phydev)
		return;

	if (phydev->link) {
		if ((port->mac_data.speed != phydev->speed) ||
		    (port->mac_data.duplex != phydev->duplex)) {
			if (port->comphy)
				mv_gop110_update_comphy(port, phydev->speed);
			port->mac_data.duplex = phydev->duplex;
			port->mac_data.speed  = phydev->speed;
		}
		port->mac_data.flags |= MV_EMAC_F_LINK_UP;
	}

	if (phydev->link != port->mac_data.link) {
		if (!phydev->link) {
			port->mac_data.duplex = -1;
			port->mac_data.speed = 0;
		}

		port->mac_data.link = phydev->link;
		status_change = 1;
	}

	if (status_change) {
		if (phydev->link) {
			mv_gop110_port_events_mask(&port->priv->hw.gop,
						   &port->mac_data);
			mv_gop110_port_enable(&port->priv->hw.gop,
					      &port->mac_data, port->comphy);
			mv_pp2x_egress_enable(port);
			mv_pp2x_ingress_enable(port);
			netif_carrier_on(dev);
			if (!(port->flags & MVPP2_F_IF_MUSDK))
				netif_tx_wake_all_queues(dev);
			mv_gop110_port_events_unmask(&port->priv->hw.gop,
						     &port->mac_data);
			port->mac_data.flags |= MV_EMAC_F_LINK_UP;
			netdev_info(dev, "link up\n");
		} else {
			mv_pp2x_ingress_disable(port);
			mv_pp2x_egress_disable(port);
			mv_gop110_port_events_mask(&port->priv->hw.gop,
						   &port->mac_data);
			mv_gop110_port_disable(&port->priv->hw.gop,
					       &port->mac_data, port->comphy);
			netif_carrier_off(dev);
			netif_tx_stop_all_queues(dev);
			port->mac_data.flags &= ~MV_EMAC_F_LINK_UP;
			netdev_info(dev, "link down\n");
		}
	}
}

void mv_pp2_link_change_tasklet(unsigned long data)
{
	struct net_device *dev = (struct net_device *)data;
	struct mv_pp2x_port *port = netdev_priv(dev);

	mv_pp22_dev_link_event(dev);

	/* Unmask interrupt */
	mv_gop110_port_events_unmask(&port->priv->hw.gop, &port->mac_data);
}

static void mv_pp2x_timer_set(struct mv_pp2x_port_pcpu *port_pcpu)
{
	ktime_t interval;

	if (!port_pcpu->timer_scheduled) {
		port_pcpu->timer_scheduled = true;
		interval = ktime_set(0, MVPP2_TXDONE_HRTIMER_PERIOD_NS);
		hrtimer_start(&port_pcpu->tx_done_timer, interval,
			      HRTIMER_MODE_REL_PINNED);
	}
}

/* Set transmit TX timer */
static void mv_pp2x_tx_timer_set(struct mv_pp2x_cp_pcpu *cp_pcpu)
{
	ktime_t interval;

	if (!cp_pcpu->tx_timer_scheduled) {
		cp_pcpu->tx_timer_scheduled = true;
		interval = ktime_set(0, MVPP2_TX_HRTIMER_PERIOD_NS);
		hrtimer_start(&cp_pcpu->tx_timer, interval,
			      HRTIMER_MODE_REL_PINNED);
	}
}

/* Cancel transmit TX timer */
static inline void mv_pp2x_tx_timer_kill(struct mv_pp2x_cp_pcpu *cp_pcpu)
{
	if (cp_pcpu->tx_timer_scheduled) {
		cp_pcpu->tx_timer_scheduled = false;
		hrtimer_cancel(&cp_pcpu->tx_timer);
	}
}

/* Singe mode: address_space equal to current CPU/number of AP's in system
 * For example if its dual AP(16 CPU's) and current CPU = 13. address_space 6 would be used
 * Multi mode: address_space for HOT CPU's equal to current CPU
 * All cold CPU's share same address space for TX.
 * So for cold CPU's address space equal to last address space used by HOT CPU's + 1.
 * For example if its dual AP(16 CPU's), # of HOT CPU's = 8 and current CPU = 13.
 * CPU would be indicated as cold_cpu=true and address_space = 8.
 * Single resource mode use only first address space.
 */
static int mv_pp2x_check_address_space(struct mv_pp2x *priv, int cpu, bool *cold_cpu)
{
	int address_space;

	if (mv_pp2x_queue_mode == MVPP2_SINGLE_RESOURCE_MODE) {
		address_space = 0;
	} else if (mv_pp2x_queue_mode == MVPP2_QDIST_SINGLE_MODE) {
		address_space = QV_CPU_2_ADR_SP(cpu, priv->pp2_cfg.num_of_ap);
	} else if (cpu < priv->rx_count) {
		address_space = QV_CPU_2_ADR_SP(cpu, (priv->rx_count / MVPP2_MAX_CPUS_IN_AP));
	} else {
		address_space = priv->rx_count;
		if (cold_cpu)
			*cold_cpu = true;
	}

	return address_space;
}

static void mv_pp2x_tx_proc_cb(unsigned long data)
{
	struct net_device *dev = (struct net_device *)data;
	struct mv_pp2x_port *port = netdev_priv(dev);
	struct mv_pp2x_port_pcpu *port_pcpu;
	unsigned int tx_todo, cause, cpu = smp_processor_id();
	u8 address_space;

	address_space = mv_pp2x_check_address_space(port->priv, cpu, NULL);

	port_pcpu = port->pcpu[address_space];

	if (!netif_running(dev))
		return;
	port_pcpu->timer_scheduled = false;

	/* Process all the Tx queues */
	cause = (1 << mv_pp2x_txq_number) - 1;
	tx_todo = mv_pp2x_tx_done(port, cause, address_space);

	/* Set the timer in case not all the packets were processed */
	if (tx_todo)
		mv_pp2x_timer_set(port_pcpu);
}

/* Update HW with number of aggregated Tx descriptors to be sent */
void mv_pp2x_aggr_txq_pend_desc_add(struct mv_pp2x_port *port, int pending, int address_space)
{
	/* aggregated access - relevant TXQ number is written in TX desc */
	mv_pp22_thread_write(&port->priv->hw, address_space, MVPP2_AGGR_TXQ_UPDATE_REG, pending);
}

/* TX to HW the pendings in aggregated TXQ; kill deferring TX hrtimer */
static inline void mv_pp2x_aggr_txq_pend_send(struct mv_pp2x_port *port,
					      struct mv_pp2x_cp_pcpu *cp_pcpu,
					      struct mv_pp2x_aggr_tx_queue *aggr_txq, int address_space)
{
	mv_pp2x_tx_timer_kill(cp_pcpu);
	aggr_txq->hw_count += aggr_txq->sw_count;
	mv_pp2x_aggr_txq_pend_desc_add(port, aggr_txq->sw_count, address_space);
	aggr_txq->sw_count = 0;
}

/* Tasklet transmit procedure */
static void mv_pp2x_tx_send_proc_cb(unsigned long data)
{
	struct mv_pp2x *priv = (struct mv_pp2x *)data;
	struct mv_pp2x_aggr_tx_queue *aggr_txq;
	struct mv_pp2x_cp_pcpu *cp_pcpu;
	int cpu = smp_processor_id();
	unsigned long flags;
	u8 address_space;
	bool cold_cpu = false;

	address_space = mv_pp2x_check_address_space(priv, cpu, &cold_cpu);

	cp_pcpu = priv->pcpu[address_space];
	cp_pcpu->tx_timer_scheduled = false;

	aggr_txq = &priv->aggr_txqs[cpu];

	if (likely(aggr_txq->sw_count > 0)) {
		/* Check if current mode require Aggregate transmit lock or its cold CPU.
		 * All cold CPU share same address space and we should protect Aggregate transmit update
		 * procedure.
		 */
		if (priv->pp2_cfg.spinlocks_bitmap & MV_AGGR_QUEUE_LOCK || cold_cpu)
			spin_lock_irqsave(&aggr_txq->spinlock, flags);

		aggr_txq->hw_count += aggr_txq->sw_count;

		mv_pp22_thread_write(&priv->hw, address_space, MVPP2_AGGR_TXQ_UPDATE_REG, aggr_txq->sw_count);
		aggr_txq->sw_count = 0;

		if (priv->pp2_cfg.spinlocks_bitmap & MV_AGGR_QUEUE_LOCK || cold_cpu)
			spin_unlock_irqrestore(&aggr_txq->spinlock, flags);
	}
}

static enum hrtimer_restart mv_pp2x_hr_timer_cb(struct hrtimer *timer)
{
	struct mv_pp2x_port_pcpu *port_pcpu = container_of(timer,
			 struct mv_pp2x_port_pcpu, tx_done_timer);

	tasklet_schedule(&port_pcpu->tx_done_tasklet);

	return HRTIMER_NORESTART;
}

static enum hrtimer_restart mv_pp2x_tx_hr_timer_cb(struct hrtimer *timer)
{
	struct mv_pp2x_cp_pcpu *cp_pcpu = container_of(timer,
			 struct mv_pp2x_cp_pcpu, tx_timer);

	tasklet_schedule(&cp_pcpu->tx_tasklet);

	return HRTIMER_NORESTART;
}

/* The function calculate the width, such as cpu width, cos queue width */
static void mv_pp2x_width_calc(struct mv_pp2x_port *port, u32 *cpu_width,
			       u32 *cos_width, u32 *port_rxq_width)
{
	struct mv_pp2x *pp2 = port->priv;
	int hot_cpus = num_online_cpus();

	if (hot_cpus > port->priv->rx_count)
		hot_cpus = port->priv->rx_count;

	if (pp2) {
		/* Calculate CPU width */
		if (cpu_width)
			*cpu_width = ilog2(roundup_pow_of_two(hot_cpus));
		/* Calculate cos queue width */
		if (cos_width)
			*cos_width = ilog2(roundup_pow_of_two(port->cos_cfg.num_cos_queues));
		/* Calculate rx queue width on the port */
		if (port_rxq_width)
			*port_rxq_width = ilog2(roundup_pow_of_two(pp2->pp2xdata->pp2x_max_port_rxqs));
	}
}

int mv_pp2x_update_flow_info(struct mv_pp2x_hw *hw)
{
	struct mv_pp2x_cls_flow_info *flow_info;
	struct mv_pp2x_cls_lookup_entry le;
	struct mv_pp2x_cls_flow_entry fe;
	int flow_index, lkp_type, prio, is_last, engine, update_rss2;
	int i, j, err;

	for (i = 0; i < (MVPP2_PRS_FL_LAST - MVPP2_PRS_FL_START); i++) {
		is_last = 0;
		update_rss2 = 0;
		flow_info = &hw->cls_shadow->flow_info[i];
		mv_pp2x_cls_lookup_read(hw, MVPP2_PRS_FL_START + i, 0, &le);
		err = mv_pp2x_cls_sw_lkp_flow_get(&le, &flow_index);
		if (err)
			return err;
		for (j = 0; is_last == 0; j++) {
			mv_pp2x_cls_flow_read(hw, flow_index + j, &fe);
			err = mv_pp2x_cls_sw_flow_engine_get(&fe, &engine,
							     &is_last);
			if (err)
				return err;
			err = mv_pp2x_cls_sw_flow_extra_get(&fe, &lkp_type,
							    &prio);
			if (err)
				return err;
			if (lkp_type == MVPP2_CLS_LKP_DEFAULT) {
				flow_info->flow_entry_dflt = flow_index + j;
			} else if (lkp_type == MVPP2_CLS_LKP_VLAN_PRI) {
				flow_info->flow_entry_vlan = flow_index + j;
			} else if (lkp_type == MVPP2_CLS_LKP_DSCP_PRI) {
				flow_info->flow_entry_dscp = flow_index + j;
			} else if (lkp_type == MVPP2_CLS_LKP_HASH) {
				if (!update_rss2) {
					flow_info->flow_entry_rss1 =
								flow_index + j;
					update_rss2 = 1;
				} else {
					flow_info->flow_entry_rss2 =
								flow_index + j;
				}
			}
		}
	}

	return 0;
}

/* CoS API */

/* mv_pp2x_cos_classifier_set
*  -- The API supplies interface to config cos classifier:
*     0: cos based on vlan pri;
*     1: cos based on dscp;
*     2: cos based on vlan for tagged packets,
*		and based on dscp for untagged IP packets;
*     3: cos based on dscp for IP packets, and based on vlan for non-IP packets;
*/
int mv_pp2x_cos_classifier_set(struct mv_pp2x_port *port,
			       enum mv_pp2x_cos_classifier cos_mode)
{
	int index, flow_idx, lkpid;
	int data[MVPP2_LKP_PTR_NUM];
	struct mv_pp2x_hw *hw = &port->priv->hw;
	struct mv_pp2x_cls_flow_info *flow_info;

	for (index = 0; index < (MVPP2_PRS_FL_LAST - MVPP2_PRS_FL_START);
		index++) {
		int i, j;

		flow_info = &hw->cls_shadow->flow_info[index];
		/* Init data[] as invalid value */
		for (i = 0; i < MVPP2_LKP_PTR_NUM; i++)
			data[i] = MVPP2_FLOW_TBL_SIZE;
		lkpid = index + MVPP2_PRS_FL_START;
		/* Prepare a temp table for the lkpid */
		mv_pp2x_cls_flow_tbl_temp_copy(hw, lkpid, &flow_idx);
		/* Update lookup table to temp flow table */
		mv_pp2x_cls_lkp_flow_set(hw, lkpid, 0, flow_idx);
		mv_pp2x_cls_lkp_flow_set(hw, lkpid, 1, flow_idx);
		/* Update original flow table */
		/* First, remove the port from original table */
		j = 0;
		if (flow_info->flow_entry_dflt)
			data[j++] = flow_info->flow_entry_dflt;

		if (flow_info->flow_entry_vlan)
			data[j++] = flow_info->flow_entry_vlan;

		if (flow_info->flow_entry_dscp)
			data[j++] = flow_info->flow_entry_dscp;

		for (i = 0; i < j; i++) {
			if (data[i] != MVPP2_FLOW_TBL_SIZE)
				mv_pp2x_cls_flow_port_del(hw,
							  data[i],
							  port->id);
		}

		/* Second, update the port in original table */
		if (mv_pp2x_prs_flow_id_attr_get(lkpid) &
			MVPP2_PRS_FL_ATTR_VLAN_BIT) {
			if (cos_mode == MVPP2_COS_CLS_VLAN ||
			    cos_mode == MVPP2_COS_CLS_VLAN_DSCP ||
			    (cos_mode == MVPP2_COS_CLS_DSCP_VLAN &&
				lkpid == MVPP2_PRS_FL_NON_IP_TAG))
				mv_pp2x_cls_flow_port_add(hw,
							  flow_info->flow_entry_vlan, port->id);
			/* Hanlde NON-IP tagged packet */
			else if (cos_mode == MVPP2_COS_CLS_DSCP &&
				 lkpid == MVPP2_PRS_FL_NON_IP_TAG)
				mv_pp2x_cls_flow_port_add(hw,
							  flow_info->flow_entry_dflt, port->id);
			else if (cos_mode == MVPP2_COS_CLS_DSCP ||
				 cos_mode == MVPP2_COS_CLS_DSCP_VLAN)
				mv_pp2x_cls_flow_port_add(hw,
							  flow_info->flow_entry_dscp, port->id);
		} else {
			if (lkpid == MVPP2_PRS_FL_NON_IP_UNTAG ||
			    cos_mode == MVPP2_COS_CLS_VLAN)
				mv_pp2x_cls_flow_port_add(hw,
							  flow_info->flow_entry_dflt, port->id);
			else if (cos_mode == MVPP2_COS_CLS_DSCP ||
				 cos_mode == MVPP2_COS_CLS_VLAN_DSCP ||
				 cos_mode == MVPP2_COS_CLS_DSCP_VLAN)
				mv_pp2x_cls_flow_port_add(hw,
							  flow_info->flow_entry_dscp, port->id);
		}
		/* Restore lookup table */
		flow_idx = data[0];
		if (flow_info->flow_entry_rss1)
			flow_idx = min_t(int, flow_info->flow_entry_rss1, flow_idx);
		if (flow_info->flow_entry_rss2)
			flow_idx = min_t(int, flow_info->flow_entry_rss2, flow_idx);

		for (i = 0; i < j; i++) {
			if (flow_idx > data[i])
				flow_idx = data[i];
		}
		mv_pp2x_cls_lkp_flow_set(hw, lkpid, 0, flow_idx);
		mv_pp2x_cls_lkp_flow_set(hw, lkpid, 1, flow_idx);
	}

	/* Update it in priv */
	port->cos_cfg.cos_classifier = cos_mode;

	return 0;
}
EXPORT_SYMBOL(mv_pp2x_cos_classifier_set);

/* mv_pp2x_cos_pri_map_set
*  -- Set priority_map per port, nibble for each cos value(0~7).
*/
int mv_pp2x_cos_pri_map_set(struct mv_pp2x_port *port, int cos_pri_map)
{
	int ret, prev_pri_map;
	u8 bound_cpu_first_rxq;

	if (port->cos_cfg.pri_map == cos_pri_map)
		return 0;

	prev_pri_map = port->cos_cfg.pri_map;
	port->cos_cfg.pri_map = cos_pri_map;

	/* Update C2 rules with nre pri_map */
	bound_cpu_first_rxq  = mv_pp2x_bound_cpu_first_rxq_calc(port);
	ret = mv_pp2x_cls_c2_rule_set(port, bound_cpu_first_rxq);
	if (ret) {
		port->cos_cfg.pri_map = prev_pri_map;
		return ret;
	}

	return 0;
}
EXPORT_SYMBOL(mv_pp2x_cos_pri_map_set);

/* mv_pp2x_cos_default_value_set
*  -- Set default cos value for untagged or non-IP packets per port.
*/
int mv_pp2x_cos_default_value_set(struct mv_pp2x_port *port, int cos_value)
{
	int ret, prev_cos_value;
	u8 bound_cpu_first_rxq;

	if (port->cos_cfg.default_cos == cos_value)
		return 0;

	prev_cos_value = port->cos_cfg.default_cos;
	port->cos_cfg.default_cos = cos_value;

	/* Update C2 rules with the pri_map */
	bound_cpu_first_rxq  = mv_pp2x_bound_cpu_first_rxq_calc(port);
	ret = mv_pp2x_cls_c2_rule_set(port, bound_cpu_first_rxq);
	if (ret) {
		port->cos_cfg.default_cos = prev_cos_value;
		return ret;
	}

	return 0;
}
EXPORT_SYMBOL(mv_pp2x_cos_default_value_set);

/* RSS API */

/* Translate CPU sequence number to real CPU ID */
static int mv_pp22_cpu_id_from_indir_tbl_get(struct mv_pp2x *pp2,
					     int cpu_seq, u32 *cpu_id)
{
	int i;
	int seq = 0;
	unsigned long hot_cpu_mask = 0;

	bitmap_set(&hot_cpu_mask, 0, min_t(u32, num_online_cpus(), mv_pp2x_rx_count));

	if (!pp2 || !cpu_id || cpu_seq >= 16)
		return -EINVAL;

	for (i = 0; i < 16; i++) {
		if (hot_cpu_mask & (1 << i)) {
			if (seq == cpu_seq) {
				*cpu_id = i;
				return 0;
			}
			seq++;
		}
	}

	return -1;
}

/* mv_pp22_rss_rxfh_indir_set
*  -- The API set the RSS table according to CPU weight from ethtool
*/
int mv_pp22_rss_rxfh_indir_set(struct mv_pp2x_port *port)
{
	struct mv_pp22_rss_entry rss_entry;
	int rss_tbl, entry_idx;
	u32 cos_width = 0, cpu_width = 0, cpu_id = 0;
	int rss_tbl_needed = port->cos_cfg.num_cos_queues;

	if (port->priv->pp2_cfg.queue_mode != MVPP2_QDIST_MULTI_MODE)
		return -1;

	memset(&rss_entry, 0, sizeof(struct mv_pp22_rss_entry));

	/* Calculate cpu and cos width */
	mv_pp2x_width_calc(port, &cpu_width, &cos_width, NULL);

	rss_entry.u.entry.width = cos_width + cpu_width;

	rss_entry.sel = MVPP22_RSS_ACCESS_TBL;

	for (rss_tbl = 0; rss_tbl < rss_tbl_needed; rss_tbl++) {
		for (entry_idx = 0; entry_idx < MVPP22_RSS_TBL_LINE_NUM; entry_idx++) {
			rss_entry.u.entry.tbl_id = rss_tbl;
			rss_entry.u.entry.tbl_line = entry_idx;
			if (mv_pp22_cpu_id_from_indir_tbl_get(port->priv,
							      port->priv->rx_indir_table[entry_idx], &cpu_id))
				return -1;
			/* Value of rss_tbl equals to cos queue */
			rss_entry.u.entry.rxq = (cpu_id << cos_width) | rss_tbl;
			if (mv_pp22_rss_tbl_entry_set(&port->priv->hw, &rss_entry))
				return -1;
		}
	}

	return 0;
}

/* mv_pp22_rss_enable_set
*  -- The API enable or disable RSS on the port
*/
void mv_pp22_rss_enable(struct mv_pp2x_port *port, bool en)
{
	u8 bound_cpu_first_rxq;

	if (port->rss_cfg.rss_en == en)
		return;

	bound_cpu_first_rxq  = mv_pp2x_bound_cpu_first_rxq_calc(port);

	if (port->priv->pp2_cfg.queue_mode == MVPP2_QDIST_MULTI_MODE) {
		mv_pp22_rss_c2_enable(port, en);
		port->rss_cfg.rss_en = en;
		if (en) {
			if (mv_pp22_rss_default_cpu_set(port,
							port->rss_cfg.dflt_cpu)) {
				netdev_err(port->dev,
					   "cannot set rss default cpu on port(%d)\n",
				port->id);
				port->rss_cfg.rss_en = 0;
			}
		} else {
			if (mv_pp2x_cls_c2_rule_set(port,
						    bound_cpu_first_rxq)) {
				netdev_err(port->dev,
					   "cannot set c2 and qos table on port(%d)\n",
				port->id);
				port->rss_cfg.rss_en = 1;
			}
		}
	}
}

/* mv_pp2x_rss_mode_set
*  -- The API to update RSS hash mode for non-fragemnt UDP packet per port.
*/
int mv_pp22_rss_mode_set(struct mv_pp2x_port *port, int rss_mode)
{
	int index, flow_idx, flow_idx_rss, lkpid, lkpid_attr;
	int data[3];
	struct mv_pp2x_hw *hw = &port->priv->hw;
	struct mv_pp2x_cls_flow_info *flow_info;
	int err;

	err = mv_pp2x_update_flow_info(hw);
	if (err) {
		netdev_err(port->dev, "cannot update flow info\n");
		return err;
	}

	if (port->priv->pp2_cfg.queue_mode != MVPP2_QDIST_MULTI_MODE)
		return -1;

	if (rss_mode != MVPP2_RSS_2T &&
	    rss_mode != MVPP2_RSS_5T) {
		pr_err("Invalid rss mode:%d\n", rss_mode);
		return -EINVAL;
	}

	for (index = 0; index < (MVPP2_PRS_FL_LAST - MVPP2_PRS_FL_START);
		index++) {
		flow_info = &hw->cls_shadow->flow_info[index];
		data[0] = MVPP2_FLOW_TBL_SIZE;
		data[1] = MVPP2_FLOW_TBL_SIZE;
		data[2] = MVPP2_FLOW_TBL_SIZE;
		lkpid = index + MVPP2_PRS_FL_START;
		/* Get lookup ID attribute */
		lkpid_attr = mv_pp2x_prs_flow_id_attr_get(lkpid);
		/* Only non-frag TCP & UDP can set rss mode */
		if ((lkpid_attr & (MVPP2_PRS_FL_ATTR_TCP_BIT | MVPP2_PRS_FL_ATTR_UDP_BIT)) &&
		    !(lkpid_attr & MVPP2_PRS_FL_ATTR_FRAG_BIT)) {
			/* Prepare a temp table for the lkpid */
			mv_pp2x_cls_flow_tbl_temp_copy(hw, lkpid, &flow_idx);
			/* Update lookup table to temp flow table */
			mv_pp2x_cls_lkp_flow_set(hw, lkpid, 0, flow_idx);
			mv_pp2x_cls_lkp_flow_set(hw, lkpid, 1, flow_idx);
			/* Update original flow table */
			/* First, remove the port from original table */
			mv_pp2x_cls_flow_port_del(hw,
						  flow_info->flow_entry_rss1, port->id);
			mv_pp2x_cls_flow_port_del(hw,
						  flow_info->flow_entry_rss2, port->id);
			if (flow_info->flow_entry_dflt)
				data[0] = flow_info->flow_entry_dflt;
			if (flow_info->flow_entry_vlan)
				data[1] = flow_info->flow_entry_vlan;
			if (flow_info->flow_entry_dscp)
				data[2] = flow_info->flow_entry_dscp;
			/* Second, update port in original table with rss_mode*/
			if (rss_mode == MVPP2_RSS_2T)
				flow_idx_rss = flow_info->flow_entry_rss1;
			else
				flow_idx_rss = flow_info->flow_entry_rss2;
			mv_pp2x_cls_flow_port_add(hw, flow_idx_rss, port->id);

			/*Find the ptr of flow table, the min flow index */
			flow_idx_rss = min(flow_info->flow_entry_rss1,
					   flow_info->flow_entry_rss2);
			flow_idx = min(min(data[0], data[1]), min(data[2],
								  flow_idx_rss));
			/*Third, restore lookup table */
			mv_pp2x_cls_lkp_flow_set(hw, lkpid, 0, flow_idx);
			mv_pp2x_cls_lkp_flow_set(hw, lkpid, 1, flow_idx);
		} else
			if (flow_info->flow_entry_rss1) {
				flow_idx_rss = flow_info->flow_entry_rss1;
				mv_pp2x_cls_flow_port_add(hw, flow_idx_rss,
							  port->id);
		}
	}
	/* Record it in priv */
	port->rss_cfg.rss_mode = rss_mode;

	return 0;
}
EXPORT_SYMBOL(mv_pp22_rss_mode_set);

/* mv_pp22_rss_default_cpu_set
*  -- The API to update the default CPU to handle the non-IP packets.
*/
int mv_pp22_rss_default_cpu_set(struct mv_pp2x_port *port, int default_cpu)
{
	u8 index, queue, q_cpu_mask;
	u32 cpu_width = 0, cos_width = 0;
	struct mv_pp2x_hw *hw = &port->priv->hw;

	if (port->priv->pp2_cfg.queue_mode != MVPP2_QDIST_MULTI_MODE)
		return -1;

	if (!(*cpumask_bits(cpu_online_mask) & (1 << default_cpu))) {
		pr_err("Invalid default cpu id %d\n", default_cpu);
		return -EINVAL;
	}

	/* Calculate width */
	mv_pp2x_width_calc(port, &cpu_width, &cos_width, NULL);
	q_cpu_mask = (1 << cpu_width) - 1;

	/* Update LSB[cpu_width + cos_width - 1 : cos_width]
	 * of queue (queue high and low) on c2 rule.
	 */
	index = hw->c2_shadow->rule_idx_info[port->id].default_rule_idx;
	queue = mv_pp2x_cls_c2_rule_queue_get(hw, index);
	queue &= ~(q_cpu_mask << cos_width);
	queue |= (default_cpu << cos_width);
	mv_pp2x_cls_c2_rule_queue_set(hw, index, queue);

	/* Update LSB[cpu_width + cos_width - 1 : cos_width]
	 * of queue on pbit table, table id equals to port id
	 */
	for (index = 0; index < MVPP2_QOS_TBL_LINE_NUM_PRI; index++) {
		queue = mv_pp2x_cls_c2_pbit_tbl_queue_get(hw, port->id, index);
		queue &= ~(q_cpu_mask << cos_width);
		queue |= (default_cpu << cos_width);
		mv_pp2x_cls_c2_pbit_tbl_queue_set(hw, port->id, index, queue);
	}

	/* Update default cpu in cfg */
	port->rss_cfg.dflt_cpu = default_cpu;

	return 0;
}
EXPORT_SYMBOL(mv_pp22_rss_default_cpu_set);

/* Main RX/TX processing routines */

/* Handle tx checksum */
static u32 mv_pp2x_skb_tx_csum(struct mv_pp2x_port *port, struct sk_buff *skb)
{
	if (skb->ip_summed == CHECKSUM_PARTIAL) {
		int ip_hdr_len = 0;
		u8 l4_proto;

		if (skb->protocol == htons(ETH_P_IP)) {
			struct iphdr *ip4h = ip_hdr(skb);

			/* Calculate IPv4 checksum and L4 checksum */
			ip_hdr_len = ip4h->ihl;
			l4_proto = ip4h->protocol;
		} else if (skb->protocol == htons(ETH_P_IPV6)) {
			struct ipv6hdr *ip6h = ipv6_hdr(skb);

			/* Read l4_protocol from one of IPv6 extra headers */
			if (skb_network_header_len(skb) > 0)
				ip_hdr_len = (skb_network_header_len(skb) >> 2);
			l4_proto = ip6h->nexthdr;
		} else {
			return MVPP2_TXD_L4_CSUM_NOT;
		}

		return mv_pp2x_txq_desc_csum(skb_network_offset(skb),
				ntohs(skb->protocol), ip_hdr_len, l4_proto);
	}

	return MVPP2_TXD_L4_CSUM_NOT | MVPP2_TXD_IP_CSUM_DISABLE;
}

static void mv_pp2x_buff_hdr_rx(struct mv_pp2x_port *port,
				struct mv_pp2x_rx_desc *rx_desc, int cpu)
{
	struct mv_pp2x_buff_hdr *buff_hdr;
	struct sk_buff *skb;
	u32 rx_status = rx_desc->status;
	u32 buff_phys_addr;
	u32 buff_virt_addr;
	u32 buff_phys_addr_next;
	u32 buff_virt_addr_next;
	int mc_id;
	int pool_id;

	pool_id = (rx_status & MVPP2_RXD_BM_POOL_ID_MASK) >>
		   MVPP2_RXD_BM_POOL_ID_OFFS;
	/*TODO : YuvalC, this is just workaround to compile.
	* Need to handle mv_pp2x_buff_hdr_rx().
	*/
	buff_phys_addr = rx_desc->u.pp21.buf_phys_addr;
	buff_virt_addr = rx_desc->u.pp21.buf_cookie;

	do {
		skb = (struct sk_buff *)(u64)buff_virt_addr;
		buff_hdr = (struct mv_pp2x_buff_hdr *)skb->head;

		mc_id = MVPP2_B_HDR_INFO_MC_ID(buff_hdr->info);

		buff_phys_addr_next = buff_hdr->next_buff_phys_addr;
		buff_virt_addr_next = buff_hdr->next_buff_virt_addr;

		/* Release buffer */
		mv_pp2x_bm_pool_mc_put(port, pool_id, buff_phys_addr,
				       buff_virt_addr, mc_id, cpu);

		buff_phys_addr = buff_phys_addr_next;
		buff_virt_addr = buff_virt_addr_next;

	} while (!MVPP2_B_HDR_INFO_IS_LAST(buff_hdr->info));
}

static void mv_pp2x_set_skb_hash(struct mv_pp2x_rx_desc *rx_desc, u32 rx_status,
				 struct sk_buff *skb)
{
	/* Store unique Marvell hash to indicate recycled skb.
	*  Hash is used as additional to skb->cb protection
	*  for recycling. skb->hash unused in xmit procedure since
	*  .ndo_select_queue callback implemented in driver.
	*/
	if ((rx_status & MVPP2_RXD_L4_UDP) || (rx_status & MVPP2_RXD_L4_TCP))
		skb_set_hash(skb, MVPP2_UNIQUE_HASH, PKT_HASH_TYPE_L4);
	else
		skb_set_hash(skb, MVPP2_UNIQUE_HASH, PKT_HASH_TYPE_L3);
}

/* Function is similar to build_skb routine without sbk kmem_cache_alloc */
static void mv_pp2x_build_skb(struct sk_buff *skb, unsigned char *data,
			      unsigned int frag_size)
{
	struct skb_shared_info *shinfo;
	unsigned int size = frag_size ? : ksize(data);

	size -= SKB_DATA_ALIGN(sizeof(struct skb_shared_info));

	memset(skb, 0, offsetof(struct sk_buff, tail));
	skb->truesize = SKB_TRUESIZE(size);
	atomic_set(&skb->users, 1);
	skb->head = data;
	skb->data = data;
	skb_reset_tail_pointer(skb);
	skb->end = skb->tail + size;
	skb->mac_header = (typeof(skb->mac_header))~0U;
	skb->transport_header = (typeof(skb->transport_header))~0U;

	/* make sure we initialize shinfo sequentially */
	shinfo = skb_shinfo(skb);
	memset(shinfo, 0, offsetof(struct skb_shared_info, dataref));
	atomic_set(&shinfo->dataref, 1);
	kmemcheck_annotate_variable(shinfo->destructor_arg);

	if (skb && frag_size) {
		skb->head_frag = 1;
		if (page_is_pfmemalloc(virt_to_head_page(data)))
			skb->pfmemalloc = 1;
	}
}

/* Main rx processing */
static int mv_pp2x_rx(struct mv_pp2x_port *port, struct napi_struct *napi,
		      int rx_todo, struct mv_pp2x_rx_queue *rxq, int address_space)
{
	struct net_device *dev = port->dev;
	int rx_received, rx_filled, i;
	u32 rcvd_pkts = 0;
	u32 rcvd_bytes = 0;
	u32 refill_array[MVPP2_BM_POOLS_NUM] = {0};
	u8  num_pool = MVPP2_BM_SWF_NUM_POOLS;
	u8  first_bm_pool = port->priv->pp2_cfg.first_bm_pool;
	int cpu = smp_processor_id();
	struct mv_pp2x_cp_pcpu *cp_pcpu = port->priv->pcpu[address_space];

#ifdef DEV_NETMAP
		if (port->flags & MVPP2_F_IFCAP_NETMAP) {
			int netmap_done = 0;

			if (netmap_rx_irq(port->dev, rxq->log_id, &netmap_done))
				return netmap_done;
		/* Netmap implementation includes all queues in i/f.*/
		return 1;
		}
#endif /* DEV_NETMAP */

	/* Get number of received packets and clamp the to-do */
	rx_received = mv_pp2x_rxq_received(port, rxq->id);

	if (rx_todo > rx_received)
		rx_todo = rx_received;

	rx_filled = 0;
	for (i = 0; i < rx_todo; i++) {
		struct mv_pp2x_rx_desc *rx_desc =
			mv_pp2x_rxq_next_desc_get(rxq);
		struct mv_pp2x_bm_pool *bm_pool;
		struct sk_buff *skb;
		u32 rx_status, pool;
		int rx_bytes;
		dma_addr_t buf_phys_addr;
		unsigned char *data;

#if defined(__BIG_ENDIAN)
		if (port->priv->pp2_version == PPV21)
			mv_pp21_rx_desc_swap(rx_desc);
		else
			mv_pp22_rx_desc_swap(rx_desc);
#endif /* __BIG_ENDIAN */

		rx_filled++;
		rx_status = rx_desc->status;
		rx_bytes = rx_desc->data_size - MVPP2_MH_SIZE;

		pool = MVPP2_RX_DESC_POOL(rx_desc);
		bm_pool = &port->priv->bm_pools[pool - first_bm_pool];
		/* Check if buffer header is used */
		if (unlikely(rx_status & MVPP2_RXD_BUF_HDR)) {
			mv_pp2x_buff_hdr_rx(port, rx_desc, cpu);
			continue;
		}

		if (port->priv->pp2_version == PPV21)
			buf_phys_addr = mv_pp21_rxdesc_phys_addr_get(rx_desc);
		else
			buf_phys_addr = mv_pp22_rxdesc_phys_addr_get(rx_desc);

		data = phys_to_virt(dma_to_phys(port->dev->dev.parent, buf_phys_addr));

		/* Prefetch 128B packet_header */
		prefetch(data + NET_SKB_PAD);
		dma_sync_single_for_cpu(dev->dev.parent, buf_phys_addr,
					MVPP2_RX_BUF_SIZE(rx_desc->data_size),
					DMA_FROM_DEVICE);

		/* In case of an error, release the requested buffer pointer
		 * to the Buffer Manager. This request process is controlled
		 * by the hardware, and the information about the buffer is
		 * comprised by the RX descriptor.
		 */
		if (unlikely(rx_status & MVPP2_RXD_ERR_SUMMARY)) {
			netdev_warn(port->dev, "MVPP2_RXD_ERR_SUMMARY\n");
err_drop_frame:
			dev->stats.rx_errors++;
			mv_pp2x_rx_error(port, rx_desc);
			mv_pp2x_pool_refill(port->priv, pool, buf_phys_addr, address_space);
			continue;
		}
		/* Try to get skb from CP skb pool
		*  If get func return skb -> use mv_pp2x_build_skb to reset skb
		*  else -> use regular build_skb callback
		*/
		skb = mv_pp2_skb_pool_get(port, address_space);

		if (skb)
			mv_pp2x_build_skb(skb, data, bm_pool->frag_size > PAGE_SIZE ? 0 :
				bm_pool->frag_size);
		else
			skb = build_skb(data, bm_pool->frag_size > PAGE_SIZE ? 0 :
				bm_pool->frag_size);

		if (unlikely(!skb)) {
			netdev_warn(port->dev, "skb build failed\n");
			goto err_drop_frame;
		}

		dma_unmap_single(dev->dev.parent, buf_phys_addr,
				 MVPP2_RX_BUF_SIZE(bm_pool->pkt_size),
				 DMA_FROM_DEVICE);
		refill_array[bm_pool->log_id]++;
		cp_pcpu->in_use[bm_pool->id]++;

#ifdef MVPP2_VERBOSE
		mv_pp2x_skb_dump(skb, rx_desc->data_size, 4);
#endif

		rcvd_pkts++;
		rcvd_bytes += rx_bytes;
		skb_reserve(skb, MVPP2_MH_SIZE + NET_SKB_PAD);
#ifdef CONFIG_MV_PTP_SERVICE
		/* If packet is PTP fetch timestamp info and built into packet data */
		mv_pp2_is_pkt_ptp_rx_proc(port, rx_desc, rx_bytes, skb->data, rcvd_pkts);
#endif
		skb_put(skb, rx_bytes);
		skb->protocol = eth_type_trans(skb, dev);

		if (likely(dev->features & NETIF_F_RXCSUM))
			mv_pp2x_rx_csum(port, rx_status, skb);
		/* Store skb magic id sequence for recycling  */
		MVPP2X_SKB_MAGIC_BPID_SET(skb, (MVPP2X_SKB_MAGIC(skb) |
					(port->priv->pp2_cfg.cell_index << 4) |
							pool));

		skb_record_rx_queue(skb, (u16)rxq->log_id);
		mv_pp2x_set_skb_hash(rx_desc, rx_status, skb);
		skb_mark_napi_id(skb, napi);

		napi_gro_receive(napi, skb);
	}

	/* Refill pool */
	for (i = 0; i < num_pool; i++) {
		int err;
		struct mv_pp2x_bm_pool *refill_bm_pool;

		if (!refill_array[i])
			continue;

		refill_bm_pool = &port->priv->bm_pools[i];
		while (likely(refill_array[i]--)) {
			err = mv_pp2x_rx_refill_new(port, refill_bm_pool,
						    refill_bm_pool->id, port->priv->pp2_cfg.recycling, address_space);
			if (unlikely(err)) {
				netdev_err(port->dev, "failed to refill BM pools\n");
				refill_array[i]++;
				rx_filled -= refill_array[i];
				break;
			}
		}
	}

	if (likely(rcvd_pkts)) {
		struct mv_pp2x_pcpu_stats *stats = this_cpu_ptr(port->stats);

		u64_stats_update_begin(&stats->syncp);
		stats->rx_packets += rcvd_pkts;
		stats->rx_bytes   += rcvd_bytes;
		u64_stats_update_end(&stats->syncp);
	}

	/* Update Rx queue management counters */

	mv_pp2x_rxq_status_update(port, rxq->id, rx_todo, rx_filled);

	return rx_todo;
}

static inline void tx_desc_unmap_put(struct device *dev,
				     struct mv_pp2x_tx_queue *txq, struct mv_pp2x_tx_desc *desc)
{
	dma_addr_t buf_phys_addr;

	buf_phys_addr = mv_pp2x_txdesc_phys_addr_get(
		((struct mv_pp2x *)(dev_get_drvdata(dev)))->pp2_version, desc);
	dma_unmap_single(dev, buf_phys_addr,
			 desc->data_size, DMA_TO_DEVICE);
	mv_pp2x_txq_desc_put(txq);
}

/* Handle tx fragmentation processing */
static int mv_pp2x_tx_frag_process(struct mv_pp2x_port *port,
				   struct sk_buff *skb, struct mv_pp2x_aggr_tx_queue *aggr_txq,
	 struct mv_pp2x_tx_queue *txq, struct mv_pp2x_txq_pcpu *txq_pcpu)
{
	struct mv_pp2x_tx_desc *tx_desc;
	int i;
	dma_addr_t buf_phys_addr;

	for (i = 0; i < skb_shinfo(skb)->nr_frags; i++) {
		skb_frag_t *frag = &skb_shinfo(skb)->frags[i];
		void *addr = page_address(frag->page.p) + frag->page_offset;

		tx_desc = mv_pp2x_txq_next_desc_get(aggr_txq);
		tx_desc->phys_txq = txq->id;
		tx_desc->data_size = frag->size;

		buf_phys_addr = dma_map_single(port->dev->dev.parent, addr,
					       tx_desc->data_size,
					       DMA_TO_DEVICE);
		if (dma_mapping_error(port->dev->dev.parent, buf_phys_addr)) {
			mv_pp2x_txq_desc_put(txq);
			goto error;
		}
		tx_desc->packet_offset = buf_phys_addr & MVPP2_TX_DESC_DATA_OFFSET;
		mv_pp2x_txdesc_phys_addr_set(port->priv->pp2_version,
					     buf_phys_addr & ~MVPP2_TX_DESC_DATA_OFFSET, tx_desc);

		if (i == (skb_shinfo(skb)->nr_frags - 1)) {
			/* Last descriptor */
			tx_desc->command = MVPP2_TXD_L_DESC;
			mv_pp2x_txq_inc_put(port->priv->pp2_version, txq_pcpu,
					    (struct sk_buff *)((uintptr_t)skb | MVPP2_ETH_SHADOW_SKB),
				tx_desc);

		} else {
			/* Descriptor in the middle: Not First, Not Last */
			tx_desc->command = 0;
			mv_pp2x_txq_inc_put(port->priv->pp2_version,
					    txq_pcpu, NULL, tx_desc);
		}
	}

	return 0;

error:
	/* Release all descriptors that were used to map fragments of
	 * this packet, as well as the corresponding DMA mappings
	 */
	 mv_pp2x_txq_inc_error(txq_pcpu, i);

	for (i = i - 1; i >= 0; i--) {
		tx_desc = txq->first_desc + i;
		tx_desc_unmap_put(port->dev->dev.parent, txq, tx_desc);
	}

	return -ENOMEM;
}

static inline void mv_pp2x_tx_done_post_proc(struct mv_pp2x_tx_queue *txq,
					     struct mv_pp2x_txq_pcpu *txq_pcpu, struct mv_pp2x_port *port,
					     int frags, int address_space)
{
	int txq_count = mv_pp2x_txq_count(txq_pcpu);

	/* Finalize TX processing */
	if (txq_count >= txq->pkts_coal)
		mv_pp2x_txq_done(port, txq, txq_pcpu);

	/* Recalc after tx_done */
	txq_count = mv_pp2x_txq_count(txq_pcpu);

	/* Set the timer in case not all frags were processed */
	if (txq_count <= frags && txq_count > 0) {
		struct mv_pp2x_port_pcpu *port_pcpu = port->pcpu[address_space];

		mv_pp2x_timer_set(port_pcpu);
	}
}

/* Validate TSO */
static inline int mv_pp2_tso_validate(struct sk_buff *skb, struct net_device *dev)
{
	if (!(dev->features & NETIF_F_TSO)) {
		pr_err("skb_is_gso(skb) returns true but features is not NETIF_F_TSO\n");
		return 1;
	}
	if (skb_shinfo(skb)->frag_list) {
		pr_err("frag_list is not null\n");
		return 1;
	}
	if (skb_shinfo(skb)->gso_segs == 1) {
		pr_err("Only one TSO segment\n");
		return 1;
	}
	if (skb->len <= skb_shinfo(skb)->gso_size) {
		pr_err("total_len (%d) less than gso_size (%d)\n",
		       skb->len, skb_shinfo(skb)->gso_size);
		return 1;
	}
	if ((htons(ETH_P_IP) != skb->protocol) || (ip_hdr(skb)->protocol != IPPROTO_TCP) ||
	    (!tcp_hdr(skb))) {
		pr_err("Protocol is not TCP over IP\n");
		return 1;
	}

	return 0;
}

static inline int mv_pp2_tso_build_hdr_desc(struct mv_pp2x_tx_desc *tx_desc,
					    u8 *data, struct mv_pp2x_port *port,
					    struct sk_buff *skb,
					    struct mv_pp2x_txq_pcpu *txq_pcpu,
					    u16 *mh, int hdr_len, int size,
					    u32 tcp_seq, u16 ip_id, int left_len)
{
	struct iphdr *iph;
	struct tcphdr *tcph;
	u8 *mac;
	dma_addr_t buf_phys_addr;
	int mac_hdr_len = skb_network_offset(skb);
	u8 *data_orig = data;

	/* Reserve 2 bytes for IP header alignment */
	mac = data + MVPP2_MH_SIZE;
	iph = (struct iphdr *)(mac + mac_hdr_len);

	memcpy(mac, skb->data, hdr_len);

	if (iph) {
		iph->id = htons(ip_id);
		iph->tot_len = htons(size + hdr_len - mac_hdr_len);
	}

	tcph = (struct tcphdr *)(mac + skb_transport_offset(skb));
	tcph->seq = htonl(tcp_seq);

	if (left_len) {
		/* Clear all special flags for not last packet */
		tcph->psh = 0;
		tcph->fin = 0;
		tcph->rst = 0;
	}

	if (mh) {
		/* Start tarnsmit from MH - add 2 bytes to size */
		*((u16 *)data) = *mh;
		/* increment ip_offset field in TX descriptor by 2 bytes */
		mac_hdr_len += MVPP2_MH_SIZE;
		hdr_len += MVPP2_MH_SIZE;
	} else {
		/* Start transmit from MAC */
		data = mac;
	}

	tx_desc->data_size = hdr_len;
	tx_desc->command = mv_pp2x_txq_desc_csum(mac_hdr_len, ntohs(skb->protocol),
				((u8 *)tcph - (u8 *)iph) >> 2, IPPROTO_TCP);
	tx_desc->command |= MVPP2_TXD_F_DESC;

	buf_phys_addr = dma_map_single(port->dev->dev.parent, data,
				       tx_desc->data_size, DMA_TO_DEVICE);

	if (unlikely(dma_mapping_error(port->dev->dev.parent, buf_phys_addr))) {
		pr_err("%s dma_map_single error\n", __func__);
		return -1;
	}

	tx_desc->packet_offset = buf_phys_addr & MVPP2_TX_DESC_DATA_OFFSET;

	mv_pp2x_txdesc_phys_addr_set(port->priv->pp2_version,
				     buf_phys_addr & ~MVPP2_TX_DESC_DATA_OFFSET, tx_desc);

	mv_pp2x_txq_inc_put(port->priv->pp2_version,
			    txq_pcpu,
				(struct sk_buff *)((uintptr_t)data_orig |
				MVPP2_ETH_SHADOW_EXT), tx_desc);

	return hdr_len;
}

static inline int mv_pp2_tso_build_data_desc(struct mv_pp2x_port *port,
					     struct mv_pp2x_tx_desc *tx_desc,
					     struct sk_buff *skb,
					     struct mv_pp2x_txq_pcpu *txq_pcpu,
					     char *frag_ptr, int frag_size,
					     int data_left, int total_left)
{
	dma_addr_t buf_phys_addr;
	int size;
	uintptr_t val = 0;

	size = min(frag_size, data_left);

	tx_desc->data_size = size;

	buf_phys_addr = dma_map_single(port->dev->dev.parent, frag_ptr,
				       size, DMA_TO_DEVICE);
	if (unlikely(dma_mapping_error(port->dev->dev.parent, buf_phys_addr))) {
		pr_err("%s dma_map_single error\n", __func__);
		return -1;
	}

	tx_desc->packet_offset = buf_phys_addr & MVPP2_TX_DESC_DATA_OFFSET;

	mv_pp2x_txdesc_phys_addr_set(port->priv->pp2_version,
				     buf_phys_addr & ~MVPP2_TX_DESC_DATA_OFFSET, tx_desc);

	tx_desc->command = 0;

	if (size == data_left) {
		/* last descriptor in the TCP packet */
		tx_desc->command = MVPP2_TXD_L_DESC;

		if (total_left == 0) {
			/* last descriptor in SKB */
			val = ((uintptr_t)skb | MVPP2_ETH_SHADOW_SKB);
		}
	}
	mv_pp2x_txq_inc_put(port->priv->pp2_version, txq_pcpu,
			    (struct sk_buff *)(val), tx_desc);

	return size;
}

/* send tso packet */
static inline int mv_pp2_tx_tso(struct sk_buff *skb, struct net_device *dev,
				struct mv_pp2x_tx_queue *txq,
			 struct mv_pp2x_aggr_tx_queue *aggr_txq, int address_space)
{
	int frag = 0, i;
	int total_len, hdr_len, size, frag_size, data_left;
	int total_desc_num, total_bytes = 0, max_desc_num = 0;
	char *frag_ptr;
	struct mv_pp2x_tx_desc *tx_desc;
	struct mv_pp2x_port *port = netdev_priv(dev);
	struct mv_pp2x_txq_pcpu *txq_pcpu = &txq->pcpu[address_space];
	u16 ip_id, *mh = NULL;
	u32 tcp_seq = 0;
	skb_frag_t *skb_frag_ptr;
	const struct tcphdr *th = tcp_hdr(skb);
	struct mv_pp2x_cp_pcpu *cp_pcpu = port->priv->pcpu[address_space];

	if (unlikely(mv_pp2_tso_validate(skb, dev)))
		return 0;

	/* Calculate expected number of TX descriptors */
	max_desc_num = skb_shinfo(skb)->gso_segs * 2 + skb_shinfo(skb)->nr_frags;

	/* Check number of available descriptors */
	if (unlikely(mv_pp2x_aggr_desc_num_check(port->priv, aggr_txq, max_desc_num, address_space) ||
		     mv_pp2x_tso_txq_reserved_desc_num_proc(port->priv, txq,
							    txq_pcpu, max_desc_num, address_space))) {
		return 0;
	}

	if (unlikely(max_desc_num > port->txq_stop_limit))
		if (likely(mv_pp2x_txq_free_count(txq_pcpu) < max_desc_num))
			return 0;

	total_len = skb->len;
	hdr_len = (skb_transport_offset(skb) + tcp_hdrlen(skb));

	total_len -= hdr_len;
	ip_id = ntohs(ip_hdr(skb)->id);
	tcp_seq = ntohl(th->seq);

	frag_size = skb_headlen(skb);
	frag_ptr = skb->data;

	if (unlikely(frag_size < hdr_len)) {
		pr_err("frag_size=%d, hdr_len=%d\n", frag_size, hdr_len);
		return 0;
	}

	/* Skip header - we'll add header in another buffer (from extra pool) */
	frag_size -= hdr_len;
	frag_ptr += hdr_len;

	/* A special case where the first skb's frag contains only the packet's header */
	if (frag_size == 0) {
		skb_frag_ptr = &skb_shinfo(skb)->frags[frag];

		/* Move to next segment */
		frag_size = skb_frag_ptr->size;
		frag_ptr = page_address(skb_frag_ptr->page.p) + skb_frag_ptr->page_offset;
		frag++;
	}
	total_desc_num = 0;

	/* Each iteration - create new TCP segment */
	while (likely(total_len > 0)) {
		u8 *data;

		data_left = min((int)(skb_shinfo(skb)->gso_size), total_len);

		/* Sanity check */
		if (unlikely(total_desc_num >= max_desc_num)) {
			pr_err("%s: Used TX descriptors number %d is larger than allocated %d\n",
			       __func__, total_desc_num, max_desc_num);
			goto out_no_tx_desc;
		}

		data = mv_pp2_extra_pool_get(port, address_space);
		if (unlikely(!data)) {
			pr_err("Can't allocate extra buffer for TSO\n");
			goto out_no_tx_desc;
		}
		tx_desc = mv_pp2x_txq_next_desc_get(aggr_txq);
		tx_desc->phys_txq = txq->id;

		total_len -= data_left;

		/* prepare packet headers: MAC + IP + TCP */
		size = mv_pp2_tso_build_hdr_desc(tx_desc, data, port, skb,
						 txq_pcpu, mh, hdr_len,
						 data_left, tcp_seq, ip_id,
						 total_len);
		if (unlikely(size < 0))
			goto out_no_tx_desc;
		total_desc_num++;

		total_bytes += size;

		/* Update packet's IP ID */
		ip_id++;

		while (likely(data_left > 0)) {
			/* Sanity check */
			if (unlikely(total_desc_num >= max_desc_num)) {
				pr_err("%s: Used TX descriptors number %d is larger than allocated %d\n",
				       __func__, total_desc_num, max_desc_num);
				goto out_no_tx_desc;
			}
			tx_desc = mv_pp2x_txq_next_desc_get(aggr_txq);
			tx_desc->phys_txq = txq->id;

			size = mv_pp2_tso_build_data_desc(port, tx_desc, skb, txq_pcpu,
							  frag_ptr, frag_size, data_left, total_len);

			if (unlikely(size < 0))
				goto out_no_tx_desc;

			total_desc_num++;
			total_bytes += size;
			data_left -= size;

			/* Update TCP sequence number */
			tcp_seq += size;

			/* Update frag size, and offset */
			frag_size -= size;
			frag_ptr += size;

			if ((frag_size == 0) && (frag < skb_shinfo(skb)->nr_frags)) {
				skb_frag_ptr = &skb_shinfo(skb)->frags[frag];

				/* Move to next segment */
				frag_size = skb_frag_ptr->size;
				frag_ptr = page_address(skb_frag_ptr->page.p) + skb_frag_ptr->page_offset;
				frag++;
			}
		}
	}

	aggr_txq->sw_count += total_desc_num;

	if (!skb->xmit_more)
		/* Transmit TCP segment with bulked descriptors and cancel tx hr timer if exist */
		mv_pp2x_aggr_txq_pend_send(port, cp_pcpu, aggr_txq, address_space);

	txq_pcpu->reserved_num -= total_desc_num;

	return total_desc_num;

out_no_tx_desc:
	/* No enough memory for packet header - rollback */
	pr_err("%s: No TX descriptors - rollback %d, txq_count=%d, nr_frags=%d, skb=%p, len=%d, gso_segs=%d\n",
	       __func__, total_desc_num, (aggr_txq->hw_count + aggr_txq->sw_count), skb_shinfo(skb)->nr_frags,
			skb, skb->len, skb_shinfo(skb)->gso_segs);

	for (i = 0; i < total_desc_num; i++) {
		struct sk_buff *shadow_skb;
		dma_addr_t  shadow_buf;
		int data_size;

		mv_pp2x_txq_dec_put(txq_pcpu);

		shadow_skb = txq_pcpu->tx_skb[txq_pcpu->txq_put_index];
		shadow_buf = txq_pcpu->tx_buffs[txq_pcpu->txq_put_index];
		data_size = txq_pcpu->data_size[txq_pcpu->txq_put_index];

		mv_pp2x_txq_buf_free(port, (uintptr_t)shadow_skb, shadow_buf,
				     data_size, address_space);

		mv_pp2x_txq_prev_desc_get(aggr_txq);
	}

	return 0;
}

/* Routine to check if skb recyclable. Data buffer cannot be recycled if:
 * 1. skb is cloned, shared, nonlinear, zero copied.
 * 2. skb with not align data buffer.
 * 3. IRQs are disabled.
 */
static inline bool mv_pp2x_skb_is_recycleable(const struct sk_buff *skb, int skb_size)
{
	if (unlikely(irqs_disabled()))
		return false;

	if (unlikely(skb_shinfo(skb)->tx_flags & SKBTX_DEV_ZEROCOPY))
		return false;

	if (unlikely(skb_is_nonlinear(skb) || skb->fclone != SKB_FCLONE_UNAVAILABLE))
		return false;

	skb_size = SKB_DATA_ALIGN(skb_size + NET_SKB_PAD);
	if (unlikely(skb_end_pointer(skb) - skb->head < skb_size))
		return false;

	if (unlikely(skb_shared(skb) || skb_cloned(skb)))
		return false;

	return true;
}

/* Routine to get BM pool from skb cb */
static inline struct mv_pp2x_bm_pool *mv_pp2x_skb_recycle_get_pool(struct mv_pp2x *priv, struct sk_buff *skb)
{
	if (MVPP2X_SKB_RECYCLE_MAGIC_IS_OK(skb))
		return &priv->bm_pools[MVPP2X_SKB_BPID_GET(skb)];
	else
		return NULL;
}

/* Routine:
 * 1. Check that it's save to recycle skb by mv_pp2x_skb_is_recycleable routine
 * 2. Check if MVPP2 unique hash were set in RX routine.
 * 3. Check that recycle is on same CPN.
 * 4. Test skb->cb magic and return BM pool ID if its pass all criterions.
 * Otherwise -1 returned.
 */
static inline int mv_pp2x_skb_recycle_check(struct mv_pp2x *priv, struct sk_buff *skb, struct mv_pp2x_cp_pcpu *cp_pcpu)
{
	struct mv_pp2x_bm_pool *bm_pool;

	if ((skb->hash == MVPP2_UNIQUE_HASH) && (MVPP2X_SKB_PP2_CELL_GET(skb) == priv->pp2_cfg.cell_index) &&
	    priv->pp2_cfg.recycling) {
		bm_pool = mv_pp2x_skb_recycle_get_pool(priv, skb);
		if (bm_pool)
			if (mv_pp2x_skb_is_recycleable(skb, bm_pool->pkt_size) && (cp_pcpu->in_use[bm_pool->id] > 0))
				return bm_pool->id;
	}

	return -1;
}

/* Main tx processing */
static int mv_pp2x_tx(struct sk_buff *skb, struct net_device *dev)
{
	struct mv_pp2x_port *port = netdev_priv(dev);
	struct mv_pp2x_tx_queue *txq;
	struct mv_pp2x_aggr_tx_queue *aggr_txq;
	struct mv_pp2x_txq_pcpu *txq_pcpu;
	struct netdev_queue *nq;
	struct mv_pp2x_tx_desc *tx_desc;
	dma_addr_t buf_phys_addr;
	int frags = 0, pool_id;
	u16 txq_id;
	u32 tx_cmd, cpu = smp_processor_id();
	int address_space;
	struct mv_pp2x_cp_pcpu *cp_pcpu;
	u8 recycling;
	unsigned long flags;
	bool cold_cpu = false;

	address_space = mv_pp2x_check_address_space(port->priv, cpu, &cold_cpu);

	cp_pcpu = port->priv->pcpu[address_space];

	/* Set relevant physical TxQ and Linux netdev queue */
	txq_id = skb_get_queue_mapping(skb) % mv_pp2x_txq_number;
	txq = port->txqs[txq_id];
	txq_pcpu = &txq->pcpu[address_space];
	aggr_txq = &port->priv->aggr_txqs[address_space];

	/* Check if current mode require Aggregate transmit lock or its cold CPU.
	 * All cold CPU share same address space and we should protect Aggregate transmit update
	 * procedure.
	 */
	if ((port->priv->pp2_cfg.spinlocks_bitmap & MV_AGGR_QUEUE_LOCK) || cold_cpu)
		spin_lock_irqsave(&aggr_txq->spinlock, flags);

	/* Prevent shadow_q override, stop tx_queue until tx_done is called*/
	if (unlikely(mv_pp2x_txq_free_count(txq_pcpu) < port->txq_stop_limit)) {
		if ((txq->log_id + (address_space * mv_pp2x_txq_number)) == skb_get_queue_mapping(skb)) {
			nq = netdev_get_tx_queue(dev, skb_get_queue_mapping(skb));
			netif_tx_stop_queue(nq);
		}
		frags = 0;
		goto out;
	}

	/* GSO/TSO */
	if (skb_is_gso(skb)) {
		frags = mv_pp2_tx_tso(skb, dev, txq, aggr_txq, address_space);
		goto out;
	}

	frags = skb_shinfo(skb)->nr_frags + 1;
	pr_debug("txq_id=%d, frags=%d\n", txq_id, frags);

	/* Check number of available descriptors */
	if (unlikely(mv_pp2x_aggr_desc_num_check(port->priv, aggr_txq, frags, address_space) ||
		     mv_pp2x_txq_reserved_desc_num_proc(port->priv, txq,
							txq_pcpu, frags, address_space))) {
		frags = 0;
		goto out;
	}

	/* Get a descriptor for the first part of the packet */
	tx_desc = mv_pp2x_txq_next_desc_get(aggr_txq);
	tx_desc->phys_txq = txq->id;
	tx_desc->data_size = skb_headlen(skb);
	pr_debug(
		"tx_desc=%p, cmd(0x%x), pkt_offset(%d), phys_txq=%d, data_size=%d\n"
		"rsrvd_hw_cmd1(0x%llx)\n"
		"buf_phys_addr_hw_cmd2(0x%llx)\n"
		"buf_cookie_bm_qset_hw_cmd3(0x%llx)\n"
		"skb_len=%d, skb_data_len=%d\n",
		tx_desc, tx_desc->command, tx_desc->packet_offset,
		tx_desc->phys_txq, tx_desc->data_size,
		tx_desc->u.pp22.rsrvd_hw_cmd1,
		tx_desc->u.pp22.buf_phys_addr_hw_cmd2,
		tx_desc->u.pp22.buf_cookie_bm_qset_hw_cmd3,
		skb->len, skb->data_len);
#ifdef MVPP2_VERBOSE
		mv_pp2x_skb_dump(skb, tx_desc->data_size, 4);
#endif
	/* Mandatory tx_desc fields are always initialized, but
	 * service-specific fields are NOT cleared => old tx_desc impacts
	 * new TX by irrelevant data inside. Clear these fields before usage.
	 * The clear shold be done in an asm-optimized way.
	 * Bit-field and Byte access generates in assembler some additional asm commands.
	 * But Write32bitZero only to required fields is most effective.
	 */
#ifdef CONFIG_MV_PTP_SERVICE
	/* Clearing the following fields:
	 * u32[2]: 0x08: L4iChk , DSATTag, DP, GEM_port_id/PTP_Descriptor[11:0]
	 * u32[5]: 0x14: Buffer_physical_pointer ,PTP Descriptor
	 */
	*((u32 *)tx_desc + 2) = 0;
	*((u32 *)tx_desc + 5) = 0;
#endif

	buf_phys_addr = dma_map_single(dev->dev.parent, skb->data,
				       tx_desc->data_size, DMA_TO_DEVICE);
	if (unlikely(dma_mapping_error(dev->dev.parent, buf_phys_addr))) {
		mv_pp2x_txq_desc_put(txq);
		frags = 0;
		goto out;
	}
	pr_debug("buf_phys_addr=%x\n", (unsigned int)buf_phys_addr);

	tx_desc->packet_offset = buf_phys_addr & MVPP2_TX_DESC_DATA_OFFSET;
	mv_pp2x_txdesc_phys_addr_set(port->priv->pp2_version,
				     buf_phys_addr & ~MVPP2_TX_DESC_DATA_OFFSET, tx_desc);

	tx_cmd = mv_pp2x_skb_tx_csum(port, skb);

	if (frags == 1) {
		/* First and Last descriptor */
		/* Check if skb should be recycled */
		pool_id = mv_pp2x_skb_recycle_check(port->priv, skb, cp_pcpu);
		/* If pool ID provided -> packet should be recycled.
		*  Set recycled field in TX descriptor and add skb recycle shadow.
		*/
		if (pool_id > -1) {
			tx_cmd |= MVPP2_TXD_BUF_MOD;
			tx_cmd |= ((pool_id << MVPP2_RXD_BM_POOL_ID_OFFS) & MVPP2_RXD_BM_POOL_ID_MASK);
			recycling = MVPP2_ETH_SHADOW_REC;
		} else {
			recycling = MVPP2_ETH_SHADOW_SKB;
		}

		tx_cmd |= MVPP2_TXD_F_DESC | MVPP2_TXD_L_DESC;
		tx_desc->command = tx_cmd;
		mv_pp2x_txq_inc_put(port->priv->pp2_version,
				    txq_pcpu, (struct sk_buff *)((uintptr_t)skb |
					recycling), tx_desc);
	} else {
		/* First but not Last */
		tx_cmd |= MVPP2_TXD_F_DESC | MVPP2_TXD_PADDING_DISABLE;
		tx_desc->command = tx_cmd;
		mv_pp2x_txq_inc_put(port->priv->pp2_version,
				    txq_pcpu, NULL, tx_desc);

		/* Continue with other skb fragments */
		if (unlikely(mv_pp2x_tx_frag_process(port, skb, aggr_txq, txq, txq_pcpu))) {
			mv_pp2x_txq_inc_error(txq_pcpu, 1);
			tx_desc_unmap_put(port->dev->dev.parent, txq, tx_desc);
			frags = 0;
			goto out;
		}
	}
	txq_pcpu->reserved_num -= frags;
	aggr_txq->sw_count += frags;

#ifdef CONFIG_MV_PTP_SERVICE
	/* If packet is PTP add Time-Stamp request into the tx_desc */
	mv_pp2_is_pkt_ptp_tx_proc(port, tx_desc, skb);
#endif

	/* Start 50 microseconds timer to transmit */
	if (!skb->xmit_more) {
		if (skb->hash == MVPP2_UNIQUE_HASH && port->priv->pp2_cfg.recycling)
			mv_pp2x_tx_timer_set(cp_pcpu);
		else
			mv_pp2x_aggr_txq_pend_send(port, cp_pcpu, aggr_txq, address_space);
	}

out:
	if (likely(frags > 0)) {
		struct mv_pp2x_pcpu_stats *stats = this_cpu_ptr(port->stats);

		u64_stats_update_begin(&stats->syncp);
		stats->tx_packets++;
		stats->tx_bytes += skb->len;
		u64_stats_update_end(&stats->syncp);
	} else {
		/* Transmit bulked descriptors*/
		if (aggr_txq->sw_count > 0)
			mv_pp2x_aggr_txq_pend_send(port, cp_pcpu, aggr_txq, address_space);
		dev->stats.tx_dropped++;
		dev_kfree_skb_any(skb);
	}
	/* PPV22 TX Post-Processing */

#ifdef DEV_NETMAP
	/* Don't check tx done for ports working in Netmap mode */
	if ((port->flags & MVPP2_F_IFCAP_NETMAP))
		return NETDEV_TX_OK;
#endif
	if (!port->interrupt_tx_done)
		mv_pp2x_tx_done_post_proc(txq, txq_pcpu, port, frags, address_space);

	if ((port->priv->pp2_cfg.spinlocks_bitmap & MV_AGGR_QUEUE_LOCK)  || cold_cpu)
		spin_unlock_irqrestore(&aggr_txq->spinlock, flags);

	return NETDEV_TX_OK;
}

static inline void mv_pp21_cause_misc_handle(struct mv_pp2x_port *port,
					     struct mv_pp2x_hw *hw, u32 cause_rx_tx)
{
	u32 cause_misc = cause_rx_tx & MVPP2_CAUSE_MISC_SUM_MASK;

	if (cause_misc) {
		mv_pp2x_cause_error(port->dev, cause_misc);

		/* Clear the cause register */
		mv_pp2x_write(hw, MVPP2_ISR_MISC_CAUSE_REG, 0);
		mv_pp2x_write(hw, MVPP2_ISR_RX_TX_CAUSE_REG(port->id),
			      cause_rx_tx & ~MVPP2_CAUSE_MISC_SUM_MASK);
	}
}

static inline void mv_pp22_cause_misc_handle(struct mv_pp2x_port *port,
					     struct mv_pp2x_hw *hw, u32 cause_rx_tx, u16 sw_thread_id)
{
	u32 cause_misc = cause_rx_tx & MVPP2_CAUSE_MISC_SUM_MASK;

	if (cause_misc) {
		mv_pp2x_cause_error(port->dev, cause_misc);

		/* Clear the cause register */
		mv_pp22_thread_write(hw, sw_thread_id, MVPP2_ISR_MISC_CAUSE_REG, 0);
		mv_pp22_thread_write(hw, sw_thread_id, MVPP2_ISR_RX_TX_CAUSE_REG(port->id),
				     cause_rx_tx & ~MVPP2_CAUSE_MISC_SUM_MASK);
	}
}

static inline int mv_pp2x_cause_rx_handle(struct mv_pp2x_port *port,
					  struct queue_vector *q_vec, struct sub_queue_vector *sub_q_vec,
					  struct napi_struct *napi, int budget, u32 cause_rx)
{
	int rx_done = 0, count = 0;
	struct mv_pp2x_rx_queue *rxq;

	while (cause_rx && budget > 0) {
		rxq = mv_pp2x_get_rx_queue(port, cause_rx);
		if (!rxq)
			break;

		count = mv_pp2x_rx(port, napi, budget, rxq, q_vec->sw_thread_id);
		rx_done += count;
		budget -= count;
		if (budget > 0) {
			/* Clear the bit associated to this Rx queue
			 * so that next iteration will continue from
			 * the next Rx queue.
			 */
			cause_rx &= ~(1 << rxq->log_id);
		}
	}

#ifdef DEV_NETMAP
	if ((port->flags & MVPP2_F_IFCAP_NETMAP)) {
		napi_complete(napi);
		sub_q_vec->pending_cause_rx = 0;
		return rx_done;
	}
#endif

	if (budget > 0) {
		cause_rx = 0;
		napi_complete(napi);
		if (!q_vec->num_of_sub_vectors  || mv_pp2x_queue_mode == MVPP2_QDIST_SINGLE_MODE) {
			mv_pp2x_qvector_interrupt_enable(q_vec);
		} else {
			unsigned long flags;

			spin_lock_irqsave(&q_vec->hw_mask_lock, flags);
			q_vec->queue_mask |= sub_q_vec->queue_mask;
			mv_pp2x_interrupts_set_mask(q_vec, q_vec->queue_mask);
			spin_unlock_irqrestore(&q_vec->hw_mask_lock, flags);
		}
	}
	sub_q_vec->pending_cause_rx = cause_rx;

	return rx_done;
}

static int mv_pp21_poll(struct napi_struct *napi, int budget)
{
	u32 cause_rx_tx, cause_rx;
	int rx_done = 0;
	struct mv_pp2x_port *port = netdev_priv(napi->dev);
	struct mv_pp2x_hw *hw = &port->priv->hw;
	struct queue_vector *q_vec = container_of(napi,
			struct queue_vector, napi);

	/* Rx/Tx cause register
	 *
	 * Bits 0-15: each bit indicates received packets on the Rx queue
	 * (bit 0 is for Rx queue 0).
	 *
	 * Bits 16-23: each bit indicates transmitted packets on the Tx queue
	 * (bit 16 is for Tx queue 0).
	 *
	 * Each CPU has its own Rx/Tx cause register
	 */
	cause_rx_tx = mv_pp2x_read(hw, MVPP2_ISR_RX_TX_CAUSE_REG(port->id));

	cause_rx_tx &= ~MVPP2_CAUSE_TXQ_OCCUP_DESC_ALL_MASK;

	/*Process misc errors */
	mv_pp21_cause_misc_handle(port, hw, cause_rx_tx);

	/* Process RX packets */
	cause_rx = cause_rx_tx & MVPP2_CAUSE_RXQ_OCCUP_DESC_ALL_MASK;
	cause_rx |= q_vec->pending_cause_rx;
	/* FIXME: Mvpp21 support */
	rx_done = mv_pp2x_cause_rx_handle(port, q_vec, (struct sub_queue_vector *)NULL, napi, budget, cause_rx);

	return rx_done;
}

static int mv_pp22_poll(struct napi_struct *napi, int budget)
{
	u32 cause_rx_tx, cause_rx, cause_tx;
	int rx_done = 0;
	struct mv_pp2x_port *port = netdev_priv(napi->dev);
	struct mv_pp2x_hw *hw = &port->priv->hw;

	struct sub_queue_vector *sub_q_vec = container_of(napi,
			struct sub_queue_vector, napi);

	struct queue_vector *q_vec = sub_q_vec->parent;

	/* Rx/Tx cause register
	 * Each CPU has its own Tx cause register
	 */

	/*The read is in the q_vector's sw_thread_id  address_space */
	cause_rx_tx = mv_pp22_thread_relaxed_read(hw, q_vec->sw_thread_id,
						  MVPP2_ISR_RX_TX_CAUSE_REG(port->id));
	pr_debug("%s port_id(%d), cpuId(%d), sw_thread_id(%d), isr_tx_rx(0x%x)\n",
		 __func__, port->id, q_vec->sw_thread_id, q_vec->sw_thread_id, cause_rx_tx);

	/*Process misc errors */
	cause_rx_tx &= sub_q_vec->queue_mask;
	mv_pp22_cause_misc_handle(port, hw, cause_rx_tx, q_vec->sw_thread_id);
	/* Release TX descriptors */
	cause_tx = (cause_rx_tx & MVPP2_CAUSE_TXQ_OCCUP_DESC_ALL_MASK) >>
			MVPP2_CAUSE_TXQ_OCCUP_DESC_ALL_OFFSET;
	if (cause_tx)
		mv_pp2x_tx_done(port, cause_tx, q_vec->sw_thread_id);

	/* Process RX packets */
	cause_rx = cause_rx_tx & MVPP2_CAUSE_RXQ_OCCUP_DESC_ALL_MASK;
	/*Convert queues from subgroup-relative to port-relative */
	cause_rx <<= q_vec->first_rx_queue;
	cause_rx |= sub_q_vec->pending_cause_rx;

	rx_done = mv_pp2x_cause_rx_handle(port, q_vec, sub_q_vec, napi, budget, cause_rx);

	return rx_done;
}

void mv_pp2x_port_napi_enable(struct mv_pp2x_port *port)
{
	int i, sub_vec_id;
	struct queue_vector *qvec;

	for (i = 0; i < port->num_qvector; i++) {
		qvec = &port->q_vector[i];
		if (!qvec->num_of_sub_vectors  || mv_pp2x_queue_mode == MVPP2_QDIST_SINGLE_MODE)
			napi_enable(&qvec->sub_vec[0]->napi);
		else
			for (sub_vec_id = 0; sub_vec_id < qvec->num_of_sub_vectors; sub_vec_id++)
				napi_enable(&qvec->sub_vec[sub_vec_id]->napi);
	}
}

void mv_pp2x_port_napi_disable(struct mv_pp2x_port *port)
{
	int i, sub_vec_id;
	struct queue_vector *qvec;

	for (i = 0; i < port->num_qvector; i++) {
		qvec = &port->q_vector[i];
		if (!qvec->num_of_sub_vectors  || mv_pp2x_queue_mode == MVPP2_QDIST_SINGLE_MODE)
			napi_disable(&qvec->sub_vec[0]->napi);
		else
			for (sub_vec_id = 0; sub_vec_id < qvec->num_of_sub_vectors; sub_vec_id++)
				napi_disable(&qvec->sub_vec[sub_vec_id]->napi);
	}
}

static void mv_pp2x_port_napi_remove(struct mv_pp2x_port *port)
{
	int i, sub_vec_id;
	struct queue_vector *qvec;

	for (i = 0; i < port->num_qvector; i++) {
		qvec = &port->q_vector[i];
		if (!qvec->num_of_sub_vectors || mv_pp2x_queue_mode == MVPP2_QDIST_SINGLE_MODE) {
			napi_hash_del(&qvec->sub_vec[0]->napi);
			netif_napi_del(&qvec->sub_vec[0]->napi);
		} else {
			for (sub_vec_id = 0; sub_vec_id < qvec->num_of_sub_vectors; sub_vec_id++) {
				napi_hash_del(&qvec->sub_vec[sub_vec_id]->napi);
				netif_napi_del(&qvec->sub_vec[sub_vec_id]->napi);
			}
		}
	}
}

static void mv_pp2x_port_irqs_dispose_mapping(struct mv_pp2x_port *port)
{
	int i;

	for (i = 0; i < port->num_irqs; i++)
		irq_dispose_mapping(port->of_irqs[i]);
}

static void mv_serdes_port_init(struct mv_pp2x_port *port)
{
	int mode;

	switch (port->mac_data.phy_mode) {
	case PHY_INTERFACE_MODE_RGMII:
		break;
	case PHY_INTERFACE_MODE_SGMII:
	case PHY_INTERFACE_MODE_QSGMII:
	case PHY_INTERFACE_MODE_1000BASEX:
		if (port->mac_data.flags & MV_EMAC_F_SGMII2_5)
			mode = COMPHY_DEF(COMPHY_HS_SGMII_MODE, port->id,
					  COMPHY_SPEED_3_125G, COMPHY_POLARITY_NO_INVERT);
		else
			mode = COMPHY_DEF(COMPHY_SGMII_MODE, port->id,
					  COMPHY_SPEED_1_25G, COMPHY_POLARITY_NO_INVERT);
		phy_set_mode(port->comphy, mode);
	break;
	case PHY_INTERFACE_MODE_XAUI:
	case PHY_INTERFACE_MODE_RXAUI:
		mode = COMPHY_DEF(COMPHY_RXAUI_MODE, port->id,
				  COMPHY_SPEED_10_3125G, COMPHY_POLARITY_NO_INVERT);
		phy_set_mode(port->comphy, mode);
	break;
	case PHY_INTERFACE_MODE_KR:
	case PHY_INTERFACE_MODE_SFI:
		if (port->mac_data.flags & MV_EMAC_F_5G)
			mode = COMPHY_DEF(COMPHY_SFI_MODE, port->id,
					  COMPHY_SPEED_5_15625G, COMPHY_POLARITY_NO_INVERT);
		else
			mode = COMPHY_DEF(COMPHY_SFI_MODE, port->id,
					  COMPHY_SPEED_10_3125G, COMPHY_POLARITY_NO_INVERT);
		phy_set_mode(port->comphy, mode);
	break;
	case PHY_INTERFACE_MODE_XFI:
		if (port->mac_data.flags & MV_EMAC_F_5G)
			mode = COMPHY_DEF(COMPHY_XFI_MODE, port->id,
					  COMPHY_SPEED_5_15625G, COMPHY_POLARITY_NO_INVERT);
		else
			mode = COMPHY_DEF(COMPHY_XFI_MODE, port->id,
					  COMPHY_SPEED_10_3125G, COMPHY_POLARITY_NO_INVERT);

		phy_set_mode(port->comphy, mode);
	break;
	default:
		pr_err("%s: Wrong port mode (%d)", __func__, port->mac_data.phy_mode);
	}
}

int mvcpn110_mac_hw_init(struct mv_pp2x_port *port)
{
	struct gop_hw *gop = &port->priv->hw.gop;
	struct mv_mac_data *mac = &port->mac_data;
	int gop_port = mac->gop_index;

	if (mac->flags & MV_EMAC_F_INIT)
		return 0;

	/* configure port PHY address */
	mv_gop110_smi_phy_addr_cfg(gop, gop_port, mac->phy_addr);

	if (port->comphy)
		mv_serdes_port_init(port);

	mv_gop110_port_init(gop, mac);

	if (mac->force_link)
		mv_gop110_fl_cfg(gop, mac);

	mac->flags |= MV_EMAC_F_INIT;

	return 0;
}

/* Set hw internals when starting port */
void mv_pp2x_start_dev(struct mv_pp2x_port *port)
{
	struct gop_hw *gop = &port->priv->hw.gop;
	struct mv_mac_data *mac = &port->mac_data;
	int mac_num = port->mac_data.gop_index;
#ifdef DEV_NETMAP
	if (port->flags & MVPP2_F_IFCAP_NETMAP) {
		if (mv_pp2x_netmap_rxq_init_buffers(port))
			pr_debug("%s: Netmap rxq_init_buffers done\n",
				 __func__);
		if (mv_pp2x_netmap_txq_init_buffers(port))
			pr_debug("%s: Netmap txq_init_buffers done\n",
				 __func__);
	}
#endif /* DEV_NETMAP */
	if (port->priv->pp2_version == PPV21) {
		mv_pp21_gmac_max_rx_size_set(port);
	} else {
		switch (mac->phy_mode) {
		case PHY_INTERFACE_MODE_RGMII:
		case PHY_INTERFACE_MODE_SGMII:
		case PHY_INTERFACE_MODE_QSGMII:
		case PHY_INTERFACE_MODE_1000BASEX:
			mv_gop110_gmac_max_rx_size_set(gop, mac_num,
						       port->pkt_size);
		break;
		case PHY_INTERFACE_MODE_XAUI:
		case PHY_INTERFACE_MODE_RXAUI:
		case PHY_INTERFACE_MODE_KR:
		case PHY_INTERFACE_MODE_SFI:
		case PHY_INTERFACE_MODE_XFI:
			mv_gop110_xlg_mac_max_rx_size_set(gop,
							  mac_num, port->pkt_size);
		break;
		default:
		break;
		}
	}
	mv_pp2x_txp_max_tx_size_set(port);

	mv_pp2x_port_napi_enable(port);

	/* Enable RX/TX interrupts on all CPUs */
	mv_pp2x_port_interrupts_enable(port);

	if (port->comphy) {
		mv_gop110_port_disable(gop, mac, port->comphy);
		phy_power_on(port->comphy);
		}

	if (port->priv->pp2_version == PPV21) {
		mv_pp21_port_enable(port);
	} else {
		if (!(port->flags & MVPP2_F_LOOPBACK)) {
			mv_gop110_port_events_mask(gop, mac);
			mv_gop110_port_enable(gop, mac, port->comphy);
		}
	}

	if (port->mac_data.phy_dev) {
		phy_start(port->mac_data.phy_dev);
	} else {
		if (!(port->flags & MVPP2_F_LOOPBACK)) {
			mv_pp22_dev_link_event(port->dev);
			tasklet_init(&port->link_change_tasklet, mv_pp2_link_change_tasklet,
				     (unsigned long)(port->dev));
		}
	}

	if (port->mac_data.phy_dev && !(port->flags & MVPP2_F_IF_MUSDK))
		netif_tx_start_all_queues(port->dev);

	mv_pp2x_egress_enable(port);
	mv_pp2x_ingress_enable(port);
	/* Unmask link_event */
	if (port->priv->pp2_version == PPV22 && !(port->flags & MVPP2_F_LOOPBACK)) {
		mv_gop110_port_events_unmask(gop, mac);
		port->mac_data.flags |= MV_EMAC_F_PORT_UP;
	}
}

/* Drain pending packets */
static void mv_pp2x_send_pend_aggr_txq(void *arg)
{
	struct mv_pp2x_port *port = arg;
	struct mv_pp2x_aggr_tx_queue *aggr_txq;
	int txq_id, address_space;
	struct mv_pp2x_tx_queue *txq;
	struct mv_pp2x_txq_pcpu *txq_pcpu;
	struct mv_pp2x_cp_pcpu *cp_pcpu;
	bool free_aggr = false;

	/* To avoid scheduling of pending packets on same address_space twice:
	 * Single mode only fist of all vectors related to this sub vec will drain pending packets
	 * Milti mode: hot CPU's - only fist of all vectors related to this sub vec will drain pending packets
	 * cold CPU's - only fist cold CPU will drain pending packets.
	 * Single resource mode: Only CPU 0 will drain pending packets.
	 */
	if (mv_pp2x_queue_mode == MVPP2_SINGLE_RESOURCE_MODE) {
		/* Only CPU0 will drain aggregated queue */
		if (smp_processor_id())
			return;
		address_space = 0;
	} else if (mv_pp2x_queue_mode == MVPP2_QDIST_SINGLE_MODE) {
		address_space = QV_CPU_2_ADR_SP(smp_processor_id(), port->priv->pp2_cfg.num_of_ap);
		if ((address_space * port->priv->pp2_cfg.num_of_ap) != smp_processor_id())
			return;
	} else if (smp_processor_id() < port->priv->rx_count) {
		address_space = QV_CPU_2_ADR_SP(smp_processor_id(), (port->priv->rx_count / MVPP2_MAX_CPUS_IN_AP));
		if ((address_space * (port->priv->rx_count / MVPP2_MAX_CPUS_IN_AP)) != smp_processor_id())
			return;
	} else {
		/* Only first cold CPU will drain aggregated queue */
		address_space = port->priv->rx_count;
		if (smp_processor_id() != address_space)
			return;
	}

	aggr_txq = &port->priv->aggr_txqs[address_space];

	for (txq_id = 0; txq_id < port->num_tx_queues; txq_id++) {
		txq = port->txqs[txq_id];
		txq_pcpu = &txq->pcpu[address_space];
		if (mv_pp2x_txq_free_count(txq_pcpu) != txq->size) {
			free_aggr = true;
			break;
		}
	}

	if (!aggr_txq->sw_count || !free_aggr)
		return; /* no pendings */

	/* Schedule Drain over the same tasklet-context
	 * which the regular TX is using (refer mv_pp2x_tx_send_proc_cb).
	 * So the Drain from the stop_dev and TX are unpreemptive and correct.
	 */
	cp_pcpu = port->priv->pcpu[address_space];
	tasklet_schedule(&cp_pcpu->tx_tasklet);
}

/* Set hw internals when stopping port */
void mv_pp2x_stop_dev(struct mv_pp2x_port *port)
{
	struct gop_hw *gop = &port->priv->hw.gop;
	struct mv_mac_data *mac = &port->mac_data;

	/* Stop new packets arriving from RX-interrupts and Linux-TX */
	mv_pp2x_ingress_disable(port);
	netif_carrier_off(port->dev);
	msleep(20);

	/* Drain pending aggregated TXQ on all CPUs */
	on_each_cpu(mv_pp2x_send_pend_aggr_txq, port, 1);
	msleep(200); /* yield and wait for tx-tasklet and HW idle */

	/* Disable interrupts on all CPUs */
	mv_pp2x_port_interrupts_disable(port);

	mv_pp2x_port_napi_disable(port);
	netif_tx_stop_all_queues(port->dev);
	mv_pp2x_egress_disable(port);

	if (port->comphy)
		phy_power_off(port->comphy);

	if (port->priv->pp2_version == PPV21) {
		mv_pp21_port_disable(port);
	} else {
		if (!(port->flags & MVPP2_F_LOOPBACK)) {
			mv_gop110_port_events_mask(gop, mac);
			mv_gop110_port_disable(gop, mac, port->comphy);
			port->mac_data.flags &= ~MV_EMAC_F_LINK_UP;
			port->mac_data.flags &= ~MV_EMAC_F_PORT_UP;
		}
	}

	if (port->mac_data.phy_dev)
		phy_stop(port->mac_data.phy_dev);
	else
		if (!(port->flags & MVPP2_F_LOOPBACK))
			tasklet_kill(&port->link_change_tasklet);
}

/* Return positive if MTU is valid */
static int mv_pp2x_check_mtu_valid(struct net_device *dev, int mtu)
{
	if (mtu < 68) {
		netdev_err(dev, "cannot change mtu to less than 68\n");
		return -EINVAL;
	}

	if (MVPP2_RX_PKT_SIZE(mtu) > MVPP2_BM_JUMBO_PKT_SIZE) {
		netdev_info(dev, "illegal MTU value %d, round to 9704\n", mtu);
		mtu = MVPP2_RX_MTU_SIZE(MVPP2_BM_JUMBO_PKT_SIZE);
	}

	return mtu;
}

int mv_pp2x_check_ringparam_valid(struct net_device *dev,
				  struct ethtool_ringparam *ring)
{
	u16 new_rx_pending = ring->rx_pending;
	u16 new_tx_pending = ring->tx_pending;

	if (ring->rx_pending == 0 || ring->tx_pending == 0)
		return -EINVAL;

	if (ring->rx_pending > MVPP2_MAX_RXD)
		new_rx_pending = MVPP2_MAX_RXD;
	else if (!IS_ALIGNED(ring->rx_pending, 16))
		new_rx_pending = ALIGN(ring->rx_pending, 16);

	if (ring->tx_pending > MVPP2_MAX_TXD)
		new_tx_pending = MVPP2_MAX_TXD;
	else if (!IS_ALIGNED(ring->tx_pending, 32))
		new_tx_pending = ALIGN(ring->tx_pending, 32);

	if (ring->rx_pending != new_rx_pending) {
		netdev_info(dev, "illegal Rx ring size value %d, round to %d\n",
			    ring->rx_pending, new_rx_pending);
		ring->rx_pending = new_rx_pending;
	}

	if (ring->tx_pending != new_tx_pending) {
		netdev_info(dev, "illegal Tx ring size value %d, round to %d\n",
			    ring->tx_pending, new_tx_pending);
		ring->tx_pending = new_tx_pending;
	}

	return 0;
}

void mv_pp2x_check_queue_size_valid(struct mv_pp2x_port *port)
{
	u16 tx_queue_size = port->tx_ring_size;
	u16 rx_queue_size = port->rx_ring_size;

	if (tx_queue_size == 0)
		port->tx_ring_size = MVPP2_MAX_TXD;

	if (rx_queue_size == 0)
		port->rx_ring_size = MVPP2_MAX_RXD;

	if (port->tx_ring_size > MVPP2_MAX_TXD)
		port->tx_ring_size = MVPP2_MAX_TXD;
	else if (!IS_ALIGNED(port->tx_ring_size, 16))
		port->tx_ring_size = ALIGN(port->tx_ring_size, 16);

	if (port->rx_ring_size > MVPP2_MAX_RXD)
		port->rx_ring_size = MVPP2_MAX_RXD;
	else if (!IS_ALIGNED(port->rx_ring_size, 16))
		port->rx_ring_size = ALIGN(port->rx_ring_size, 16);

	if (tx_queue_size != port->tx_ring_size)
		pr_err("illegal Tx queue size value %d, round to %d\n",
		       tx_queue_size, port->tx_ring_size);

	if (rx_queue_size != port->rx_ring_size)
		pr_err("illegal Rx queue size value %d, round to %d\n",
		       rx_queue_size, port->rx_ring_size);
}

static int mv_pp2x_phy_connect(struct mv_pp2x_port *port)
{
	struct phy_device *phy_dev;
	void (*fp)(struct net_device *);

	if (port->priv->pp2_version == PPV21)
		fp = mv_pp21_link_event;
	else
		fp = mv_pp22_link_event;

	phy_dev = of_phy_connect(port->dev, port->mac_data.phy_node,
				 fp, 0, port->mac_data.phy_mode);
	if (!phy_dev) {
		dev_err(port->dev->dev.parent, "port ID: %d cannot connect to phy\n", port->id);
		return -ENODEV;
	}
	phy_dev->supported &= PHY_GBIT_FEATURES;
	phy_dev->advertising = phy_dev->supported;

	port->mac_data.phy_dev = phy_dev;
	port->mac_data.link    = 0;
	port->mac_data.duplex  = 0;
	port->mac_data.speed   = 0;

	return 0;
}

static void mv_pp2x_phy_disconnect(struct mv_pp2x_port *port)
{
	if (port->mac_data.phy_dev) {
		phy_disconnect(port->mac_data.phy_dev);
		port->mac_data.phy_dev = NULL;
	}
}

int mv_pp2x_open_cls(struct net_device *dev)
{
	struct mv_pp2x_port *port = netdev_priv(dev);
	unsigned char mac_bcast[ETH_ALEN] = {
			0xff, 0xff, 0xff, 0xff, 0xff, 0xff };
	struct mv_pp2x_hw *hw = &port->priv->hw;
	int err;
	u32 cpu_width = 0, cos_width = 0, port_rxq_width = 0;
	u8 bound_cpu_first_rxq;

	/* Calculate width */
	mv_pp2x_width_calc(port, &cpu_width, &cos_width, &port_rxq_width);
	if (cpu_width + cos_width > port_rxq_width) {
		err = -1;
		netdev_err(dev, "cpu or cos queue width invalid\n");
		return err;
	}

	err = mv_pp2x_prs_mac_da_accept(port, mac_bcast, true);
	if (err) {
		netdev_err(dev, "mv_pp2x_prs_mac_da_accept BC failed\n");
		return err;
	}

	err = mv_pp2x_prs_mac_da_accept(port, dev->dev_addr, true);
	if (err) {
		netdev_err(dev, "mv_pp2x_prs_mac_da_accept M2M failed\n");
		return err;
	}
	err = mv_pp2x_prs_tag_mode_set(hw, port->id, MVPP2_TAG_TYPE_MH);

	if (err) {
		netdev_err(dev, "mv_pp2x_prs_tag_mode_set failed\n");
		return err;
	}

	err = mv_pp2x_prs_def_flow(port);
	if (err) {
		netdev_err(dev, "mv_pp2x_prs_def_flow failed\n");
		return err;
	}

	err = mv_pp2x_prs_flow_set(port);
	if (err) {
		netdev_err(dev, "mv_pp2x_prs_flow_set failed\n");
		return err;
	}

	/* For musdk ports do not updated classifier/rss, only parser. */
	if (port->flags & MVPP2_F_IF_MUSDK)
		return 0;

	err = mv_pp2x_update_flow_info(hw);
	if (err) {
		netdev_err(port->dev, "cannot update flow info\n");
		return err;
	}

	/* Set CoS classifier */
	err = mv_pp2x_cos_classifier_set(port, cos_classifer);
	if (err) {
		netdev_err(port->dev, "cannot set cos classifier\n");
		return err;
	}

	/* Init C2 rules */
	bound_cpu_first_rxq  = mv_pp2x_bound_cpu_first_rxq_calc(port);
	err = mv_pp2x_cls_c2_rule_set(port, bound_cpu_first_rxq);
	if (err) {
		netdev_err(port->dev, "cannot init C2 rules\n");
		return err;
	}

	if (port->priv->pp2_version == PPV21)
		return 0;

	/* Assign rss table for rxq belong to this port */
	err = mv_pp22_rss_rxq_set(port, cos_width);
	if (err) {
		netdev_err(port->dev, "cannot allocate rss table for rxq\n");
		return err;
	}

	/* RSS related config */
	if (port->priv->pp2_cfg.queue_mode == MVPP2_QDIST_MULTI_MODE) {
		/* Set RSS mode */
		err = mv_pp22_rss_mode_set(port, port->rss_cfg.rss_mode);
		if (err) {
			netdev_err(port->dev, "cannot set rss mode\n");
			return err;
		}

		/* Init RSS table */
		err = mv_pp22_rss_rxfh_indir_set(port);
		if (err) {
			netdev_err(port->dev, "cannot init rss rxfh indir\n");
			return err;
		}

		/* Set rss default CPU only when rss enabled */
		if (port->rss_cfg.rss_en) {
			err = mv_pp22_rss_default_cpu_set(port,
							  port->rss_cfg.dflt_cpu);
			if (err) {
				netdev_err(port->dev, "cannot set rss default cpu\n");
				return err;
			}
		}
	}

	return 0;
}

/* Mask the current CPU's Rx/Tx interrupts */
static inline void mv_pp21_interrupts_mask(void *arg)
{
	struct mv_pp2x_port *port = arg;

	mv_pp2x_write(&port->priv->hw, MVPP2_ISR_RX_TX_MASK_REG(port->id), 0);
}

/* Unmask the current CPU's Rx/Tx interrupts */
static inline void mv_pp21_interrupts_unmask(void *arg)
{
	struct mv_pp2x_port *port = arg;
	u32 val;

	val = MVPP2_CAUSE_MISC_SUM_MASK | MVPP2_CAUSE_RXQ_OCCUP_DESC_ALL_MASK;
	/* Don't unmask Tx done interrupts for ports working in Netmap mode*/
	if (!(port->flags & MVPP2_F_IFCAP_NETMAP) && port->priv->pp2xdata->interrupt_tx_done)
		val |= MVPP2_CAUSE_TXQ_OCCUP_DESC_ALL_MASK;

	mv_pp2x_write(&port->priv->hw, MVPP2_ISR_RX_TX_MASK_REG(port->id), val);
}

/* Mask the private Rx/Tx interrupts */
static inline void mv_pp22_private_interrupt_mask(struct mv_pp2x_port *port, int address_space)
{
	mv_pp2x_relaxed_write(&port->priv->hw, MVPP2_ISR_RX_TX_MASK_REG(port->id), 0, address_space);
}

/* Unmask he private Rx/Tx interrupts */
static inline void mv_pp22_private_interrupt_unmask(struct mv_pp2x_port *port, int address_space)
{
	u32 val;

	val = MVPP2_CAUSE_MISC_SUM_MASK | MVPP2_CAUSE_RXQ_OCCUP_DESC_ALL_MASK;
	/* Don't unmask Tx done interrupts for ports working in Netmap mode*/
	if (!(port->flags & MVPP2_F_IFCAP_NETMAP) && port->priv->pp2xdata->interrupt_tx_done)
		val |= MVPP2_CAUSE_TXQ_OCCUP_DESC_ALL_MASK;

	mv_pp2x_relaxed_write(&port->priv->hw, MVPP2_ISR_RX_TX_MASK_REG(port->id), val, address_space);
}

/* Mask shared and private Rx/Tx interrupts */
static inline void mv_pp22_interrupts_mask(struct mv_pp2x_port *port)
{
	struct queue_vector *q_vec = &port->q_vector[0];
	int i;

	for (i = 0; i < port->num_qvector; i++) {
		if (q_vec[i].qv_type == MVPP2_RX_SHARED)
			mv_pp22_thread_write(&port->priv->hw, q_vec[i].sw_thread_id,
					     MVPP2_ISR_RX_TX_MASK_REG(port->id), 0);
		else
			mv_pp22_private_interrupt_mask(port, q_vec[i].sw_thread_id);
	}
}

/* Unmask shared and private Rx/Tx interrupts */
static inline void mv_pp22_interrupts_unmask(struct mv_pp2x_port *port)
{
	struct queue_vector *q_vec = &port->q_vector[0];
	int i;

	for (i = 0; i < port->num_qvector; i++) {
		if (q_vec[i].qv_type == MVPP2_RX_SHARED)
			mv_pp22_thread_write(&port->priv->hw, q_vec[i].sw_thread_id,
					     MVPP2_ISR_RX_TX_MASK_REG(port->id), MVPP2_CAUSE_RXQ_OCCUP_DESC_ALL_MASK);
		else
			mv_pp22_private_interrupt_unmask(port, q_vec[i].sw_thread_id);
	}
}

int mv_pp2x_open(struct net_device *dev)
{
	struct mv_pp2x_port *port = netdev_priv(dev);
	int err;

	set_device_base_address(dev);

	/* Allocate the Rx/Tx queues */
	err = mv_pp2x_setup_rxqs(port);
	if (err) {
		netdev_err(port->dev, "cannot allocate Rx queues\n");
		return err;
	}
	err = mv_pp2x_setup_txqs(port);
	if (err) {
		netdev_err(port->dev, "cannot allocate Tx queues\n");
		goto err_cleanup_rxqs;
	}
	err = mv_pp2x_setup_irqs(dev, port);
	if (err) {
		netdev_err(port->dev, "cannot allocate irq's\n");
		goto err_cleanup_txqs;
	}

	/* Only Mvpp22 support hot plug feature */
	if (port->priv->pp2_version == PPV22  && !(port->flags & (MVPP2_F_IF_MUSDK | MVPP2_F_LOOPBACK))) {
		register_hotcpu_notifier(&port->port_hotplug_nb);
		port->port_hotplugged = true;
	}

	/* In default link is down */
	netif_carrier_off(port->dev);

	if (!(port->flags & MVPP2_F_IF_MUSDK)) {
		/* Unmask interrupts on all CPUs */
		if (port->priv->pp2_version == PPV21)
			on_each_cpu(mv_pp21_interrupts_unmask, port, 1);
		else
			/* Unmask shared and private interrupts */
			mv_pp22_interrupts_unmask(port);

		/* Port is init in uboot */
	}
	if ((port->priv->pp2_version == PPV22) && !(port->flags & MVPP2_F_LOOPBACK))
		mvcpn110_mac_hw_init(port);
	mv_pp2x_start_dev(port);

	/* Before rxq and port init, all ingress packets should be blocked
	 *  in classifier
	 */
	err = mv_pp2x_open_cls(dev);
	if (err)
		goto err_free_all;

	return 0;

err_free_all:
	mv_pp2x_cleanup_irqs(port);
err_cleanup_txqs:
	mv_pp2x_cleanup_txqs(port);
err_cleanup_rxqs:
	mv_pp2x_cleanup_rxqs(port);
	return err;
}

int mv_pp2x_stop(struct net_device *dev)
{
	struct mv_pp2x_port *port = netdev_priv(dev);
	struct mv_pp2x_port_pcpu *port_pcpu;
	int cpu;

	mv_pp2x_stop_dev(port);

	if (!(port->flags & MVPP2_F_IF_MUSDK)) {
		/* Mask interrupts on all CPUs */
	if (port->priv->pp2_version == PPV21)
		on_each_cpu(mv_pp21_interrupts_mask, port, 1);
	else
		/* Mask shared and private interrupts */
		mv_pp22_interrupts_mask(port);
	}
	mv_pp2x_cleanup_irqs(port);

	if (port->port_hotplugged)
		unregister_hotcpu_notifier(&port->port_hotplug_nb);

	/* Cancel tx timers in case Tx done interrupts are disabled and if port is not in Netmap mode */
	for (cpu = 0; cpu < mv_pp2x_used_addr_spaces; cpu++) {
		port_pcpu = port->pcpu[cpu];

		if (port_pcpu->tx_done_timer.function) {
			hrtimer_cancel(&port_pcpu->tx_done_timer);
			port_pcpu->timer_scheduled = false;
		}
		if (port_pcpu->tx_done_tasklet.func)
			tasklet_kill(&port_pcpu->tx_done_tasklet);
	}

	mv_pp2x_cleanup_rxqs(port);
	mv_pp2x_cleanup_txqs(port);

	return 0;
}

static void mv_pp2x_set_rx_promisc(struct mv_pp2x_port *port)
{
	struct mv_pp2x_hw *hw = &port->priv->hw;
	int id = port->id;

	/* Enter promisc mode */
	/* Accept all: Multicast + Unicast */
	mv_pp2x_prs_mac_uc_promisc_set(hw, id, true);
	mv_pp2x_prs_mac_mc_promisc_set(hw, id, true);
	/* Remove all port->id's mcast enries */
	mv_pp2x_prs_mac_entry_del(port, MVPP2_PRS_MAC_MC, MVPP2_DEL_MAC_ALL);
	/* Remove all port->id's ucast enries except M2M entry */
	mv_pp2x_prs_mac_entry_del(port, MVPP2_PRS_MAC_UC, MVPP2_DEL_MAC_ALL);
}

static void mv_pp2x_set_rx_allmulti(struct mv_pp2x_port *port)
{
	struct mv_pp2x_hw *hw = &port->priv->hw;
	int id = port->id;

	/* Accept all multicast */
	mv_pp2x_prs_mac_mc_promisc_set(hw, id, true);
	/* Remove all multicast filter entries from parser */
	mv_pp2x_prs_mac_entry_del(port, MVPP2_PRS_MAC_MC, MVPP2_DEL_MAC_ALL);
}

static void mv_pp2x_set_rx_uc_multi(struct mv_pp2x_port *port)
{
	struct mv_pp2x_hw *hw = &port->priv->hw;
	int id = port->id;

	/* Accept all unicast */
	mv_pp2x_prs_mac_uc_promisc_set(hw, id, true);
	/* Remove all unicast filter entries from parser */
	mv_pp2x_prs_mac_entry_del(port, MVPP2_PRS_MAC_UC, MVPP2_DEL_MAC_ALL);
}

/* register unicast and multicast addresses */
static void mv_pp2x_set_rx_mode(struct net_device *dev)
{
	struct mv_pp2x_port *port = netdev_priv(dev);
	struct netdev_hw_addr *ha;
	struct mv_pp2x_hw *hw = &port->priv->hw;
	int id = port->id;
	int err;

	if (dev->flags & IFF_PROMISC) {
		mv_pp2x_set_rx_promisc(port);
	} else {
		/* Put dev into UC promisc if MAC num greater than uc filter max */
		if (netdev_uc_count(dev) > port->priv->pp2_cfg.uc_filter_max) {
			mv_pp2x_set_rx_uc_multi(port);
		} else {
			/* Remove old enries not in uc list except M2M entry */
			mv_pp2x_prs_mac_entry_del(port,
						  MVPP2_PRS_MAC_UC,
						  MVPP2_DEL_MAC_NOT_IN_LIST);
			/* Add all entries into to uc mac addr filter list */
			netdev_for_each_uc_addr(ha, dev) {
				err = mv_pp2x_prs_mac_da_accept(port,
								ha->addr, true);
				if (err)
					netdev_err(dev,
						   "[%2x:%2x:%2x:%2x:%2x:%x]add fail\n",
						   ha->addr[0], ha->addr[1],
						   ha->addr[2], ha->addr[3],
						   ha->addr[4], ha->addr[5]);
			}
			/* Leave promisc mode */
			mv_pp2x_prs_mac_uc_promisc_set(hw, id, false);
		}

		if (dev->flags & IFF_ALLMULTI) {
			mv_pp2x_set_rx_allmulti(port);
		} else {
			/* Put dev allmulti if MAC num exceeds mc filter max */
			if (netdev_mc_count(dev) >
					port->priv->pp2_cfg.mc_filter_max) {
				mv_pp2x_set_rx_allmulti(port);
				return;
			}
			/* Remove old mcast entries not in mc list */
			mv_pp2x_prs_mac_entry_del(port,
						  MVPP2_PRS_MAC_MC,
						  MVPP2_DEL_MAC_NOT_IN_LIST);
			/* Add all entries into to mc mac filter list */
			if (!netdev_mc_empty(dev)) {
				netdev_for_each_mc_addr(ha, dev) {
					err =
					mv_pp2x_prs_mac_da_accept(port,
								  ha->addr,
								  true);
					if (err)
						netdev_err(dev,
							   "MAC[%2x:%2x:%2x:%2x:%2x:%2x] add failed\n",
						ha->addr[0], ha->addr[1],
						ha->addr[2], ha->addr[3],
						ha->addr[4], ha->addr[5]);
				}
			}
			/* Reject other MC mac entries */
			mv_pp2x_prs_mac_mc_promisc_set(hw, id, false);
		}
	}
}

static int mv_pp2x_set_mac_address(struct net_device *dev, void *p)
{
	const struct sockaddr *addr = p;
	int err;

	if (!is_valid_ether_addr(addr->sa_data)) {
		err = -EADDRNOTAVAIL;
		goto error;
	}

	err = mv_pp2x_prs_update_mac_da(dev, addr->sa_data);
	if (!err)
		return 0;
	/* Reconfigure parser to accept the original MAC address */
	err = mv_pp2x_prs_update_mac_da(dev, dev->dev_addr);

error:
	netdev_err(dev, "fail to change MAC address\n");
	return err;
}

static int mv_pp2x_change_mtu(struct net_device *dev, int mtu)
{
	struct mv_pp2x_port *port = netdev_priv(dev);
	int err;

#ifdef DEV_NETMAP
	if (port->flags & MVPP2_F_IFCAP_NETMAP) {
		netdev_err(dev, "MTU can not be modified for port configured to Netmap mode\n");
		return -EPERM;
	}
#endif
	if (port->flags & MVPP2_F_IF_MUSDK) {
		netdev_err(dev, "MTU can not be modified for port in MUSDK mode\n");
		return -EPERM;
	}

	mtu = mv_pp2x_check_mtu_valid(dev, mtu);
	if (mtu < 0) {
		err = mtu;
		goto error;
	}

	if (!netif_running(dev)) {
		err = mv_pp2x_bm_update_mtu(dev, mtu);
		if (!err) {
			port->pkt_size =  MVPP2_RX_PKT_SIZE(mtu);
			return 0;
		}

		/* Reconfigure BM to the original MTU */
		err = mv_pp2x_bm_update_mtu(dev, dev->mtu);
		goto error;
	}

	mv_pp2x_stop_dev(port);

	err = mv_pp2x_bm_update_mtu(dev, mtu);
	if (!err) {
		port->pkt_size =  MVPP2_RX_PKT_SIZE(mtu);
		goto out_start;
	}

	/* Reconfigure BM to the original MTU */
	err = mv_pp2x_bm_update_mtu(dev, dev->mtu);
	if (err)
		goto error;

out_start:
	mv_pp2x_start_dev(port);
	return 0;

error:
	netdev_err(dev, "fail to change MTU\n");
	return err;
}

static int mv_pp2x_rx_add_vid(struct net_device *dev, u16 proto, u16 vid)
{
	int err;

	if (vid >= VLAN_N_VID)
		return -EINVAL;

	err = mv_pp2x_prs_vid_entry_accept(dev, proto, vid, true);
	return err;
}

static int mv_pp2x_rx_kill_vid(struct net_device *dev, u16 proto, u16 vid)
{
	int err;

	if (vid >= VLAN_N_VID)
		return -EINVAL;

	err = mv_pp2x_prs_vid_entry_accept(dev, proto, vid, false);
	return err;
}

static struct rtnl_link_stats64 *
mv_pp2x_get_stats64(struct net_device *dev, struct rtnl_link_stats64 *stats)
{
	struct mv_pp2x_port *port = netdev_priv(dev);
	unsigned int start;
	int cpu;

	for_each_online_cpu(cpu) {
		struct mv_pp2x_pcpu_stats *cpu_stats;
		u64 rx_packets;
		u64 rx_bytes;
		u64 tx_packets;
		u64 tx_bytes;

		cpu_stats = per_cpu_ptr(port->stats, cpu);
		do {
			start = u64_stats_fetch_begin_irq(&cpu_stats->syncp);
			rx_packets = cpu_stats->rx_packets;
			rx_bytes   = cpu_stats->rx_bytes;
			tx_packets = cpu_stats->tx_packets;
			tx_bytes   = cpu_stats->tx_bytes;
		} while (u64_stats_fetch_retry_irq(&cpu_stats->syncp, start));

		stats->rx_packets += rx_packets;
		stats->rx_bytes   += rx_bytes;
		stats->tx_packets += tx_packets;
		stats->tx_bytes   += tx_bytes;
	}

	stats->rx_errors	= dev->stats.rx_errors;
	stats->rx_dropped	= dev->stats.rx_dropped;
	stats->tx_dropped	= dev->stats.tx_dropped;

	return stats;
}

static int mv_pp2x_ioctl(struct net_device *dev, struct ifreq *ifr, int cmd)
{
	struct mv_pp2x_port *port = netdev_priv(dev);
	int ret = 0;

	if (!port->mac_data.phy_dev)
		return -ENOTSUPP;
	ret = phy_mii_ioctl(port->mac_data.phy_dev, ifr, cmd);
	if (!ret)
		mv_pp21_link_event(dev);
	return ret;
}

/*of_irq_count is not exported */
int mv_pp2x_of_irq_count(struct device_node *dev)
{
	struct of_phandle_args irq;
	int nr = 0;

	while (of_irq_parse_one(dev, nr, &irq) == 0)
		nr++;

	return nr;
}

/* Currently only support LK-3.18 and above, no back support */
static int mv_pp2x_netdev_set_features(struct net_device *dev,
				       netdev_features_t features)
{
	netdev_features_t changed = dev->features ^ features;
	struct mv_pp2x_port *port = netdev_priv(dev);

	/* dev->features is not changed */
	if (!changed)
		return 0;

	if (changed & NETIF_F_RXHASH) {
		if (port->flags & MVPP2_F_IF_MUSDK) {
			netdev_err(dev, "Hashing can not be modified for port in MUSDK mode\n");
			return -EPERM;
		}
		if (features & NETIF_F_RXHASH) {
			/* Enable RSS */
			mv_pp22_rss_enable(port, true);
		} else {
			/* Disable RSS */
			mv_pp22_rss_enable(port, false);
		}
	}

	if (changed & NETIF_F_TSO) {
		if (features & NETIF_F_TSO)
			port->txq_stop_limit = TSO_TXQ_LIMIT;
		else
			port->txq_stop_limit = TXQ_LIMIT;
	}

	dev->features = features;

	return 0;
}

u16 mv_pp2x_select_queue(struct net_device *dev, struct sk_buff *skb,
			 void *accel_priv, select_queue_fallback_t fallback)

{
	/* If packet in coming from Rx -> RxQ = TxQ, callback function used for packets from CPU Tx */
	if (skb->queue_mapping)
		return ((skb->queue_mapping - 1) % mv_pp2x_txq_number) + (smp_processor_id() * mv_pp2x_txq_number);
	else
		return mv_pp2x_txq_number * fallback(dev, skb);
}

/* Device ops */
static const struct net_device_ops mv_pp2x_netdev_ops = {
	.ndo_open		= mv_pp2x_open,
	.ndo_stop		= mv_pp2x_stop,
	.ndo_start_xmit		= mv_pp2x_tx,
	.ndo_select_queue	= mv_pp2x_select_queue,
	.ndo_set_rx_mode	= mv_pp2x_set_rx_mode,
	.ndo_set_mac_address	= mv_pp2x_set_mac_address,
	.ndo_change_mtu		= mv_pp2x_change_mtu,
	.ndo_get_stats64	= mv_pp2x_get_stats64,
	.ndo_do_ioctl		= mv_pp2x_ioctl,
	.ndo_set_features	= mv_pp2x_netdev_set_features,
	.ndo_vlan_rx_add_vid	= mv_pp2x_rx_add_vid,
	.ndo_vlan_rx_kill_vid	= mv_pp2x_rx_kill_vid,
};

/* Driver initialization */

static void mv_pp21_port_power_up(struct mv_pp2x_port *port)
{
	mv_pp21_port_mii_set(port);
	mv_pp21_port_periodic_xon_disable(port);
	mv_pp21_port_fc_adv_enable(port);
	mv_pp21_port_reset(port);
}

static int  mv_pp2x_port_txqs_init(struct device *dev,
				   struct mv_pp2x_port *port)
{
	int queue, cpu;
	struct mv_pp2x_txq_pcpu *txq_pcpu;

	port->tx_time_coal = MVPP2_TXDONE_COAL_USEC;

	for (queue = 0; queue < port->num_tx_queues; queue++) {
		int queue_phy_id = mv_pp2x_txq_phys(port->id, queue);
		struct mv_pp2x_tx_queue *txq;

		txq = devm_kzalloc(dev, sizeof(*txq), GFP_KERNEL);
		if (!txq)
			return -ENOMEM;

		txq->pcpu = devm_kcalloc(dev, mv_pp2x_used_addr_spaces,
				       sizeof(struct mv_pp2x_txq_pcpu),
				       GFP_KERNEL);
		if (!txq->pcpu)
			return(-ENOMEM);

		txq->id = queue_phy_id;
		txq->log_id = queue;
		txq->pkts_coal = MVPP2_TXDONE_COAL_PKTS;

		for (cpu = 0; cpu < mv_pp2x_used_addr_spaces; cpu++) {
			txq_pcpu = &txq->pcpu[cpu];
			txq_pcpu->cpu = cpu;
		}

		port->txqs[queue] = txq;
	}

	return 0;
}

static int  mv_pp2x_port_rxqs_init(struct device *dev,
				   struct mv_pp2x_port *port)
{
	int queue;

	/* Allocate and initialize Rx queue for this port */
	for (queue = 0; queue < port->num_rx_queues; queue++) {
		struct mv_pp2x_rx_queue *rxq;

		/* Map physical Rx queue to port's logical Rx queue */
		rxq = devm_kzalloc(dev, sizeof(*rxq), GFP_KERNEL);
		if (!rxq)
			return(-ENOMEM);
		/* Map this Rx queue to a physical queue */
		rxq->id = port->first_rxq + queue;
		rxq->port = port->id;
		rxq->log_id = queue;

		port->rxqs[queue] = rxq;
	}

	return 0;
}

static void mv_pp21_port_queue_vectors_init(struct mv_pp2x_port *port)
{
	struct queue_vector *q_vec = &port->q_vector[0];

	q_vec[0].first_rx_queue = 0;
	q_vec[0].num_rx_queues = port->num_rx_queues;
	q_vec[0].parent = port;
	q_vec[0].pending_cause_rx = 0;
	q_vec[0].qv_type = MVPP2_RX_SHARED;
	q_vec[0].sw_thread_id = 0;
	q_vec[0].sw_thread_mask = *cpumask_bits(cpu_online_mask);
	q_vec[0].irq = port->of_irqs[0];
	netif_napi_add(port->dev, &q_vec[0].napi, mv_pp21_poll,
		       NAPI_POLL_WEIGHT);

	port->num_qvector = 1;
}

/* Routine calculate number of private queue vectors */
static int mv_pp22_get_num_private_queue_vectors(struct mv_pp2x_port *port)
{
	if (num_active_cpus() > MVPP2_MAX_CPUS_IN_AP || num_active_cpus() > port->priv->rx_count)
		return min_t(u32, MVPP2_MAX_CPUS_IN_AP, port->priv->rx_count);

	return num_active_cpus();
}

/* Routine allocate hif(address space) for Kernel */
static void mv_pp22_allocate_hif_for_kernel(struct mv_pp2x_hw *hw, int hif_num)
{
	int val;

	val = mv_pp2x_read(hw, MVPP22_HIF_ALLOCATION_REG);
	val |= BIT(hif_num);
	mv_pp2x_write(hw, MVPP22_HIF_ALLOCATION_REG, val);
}

/* Routine calculate number of used sub_vector */
static struct sub_queue_vector *mv_pp22_allocate_sub_vector(struct mv_pp2x_port *port, struct queue_vector *q_vec,
							    u32 queue_mask)
{
	struct sub_queue_vector *sub_q_vec;

	sub_q_vec = devm_kzalloc(port->dev->dev.parent, sizeof(*sub_q_vec), GFP_KERNEL);
	netif_napi_add(port->dev, &sub_q_vec->napi, mv_pp22_poll, NAPI_POLL_WEIGHT);
	napi_hash_add(&sub_q_vec->napi);
	sub_q_vec->parent = q_vec;
	sub_q_vec->queue_mask = queue_mask;

	return sub_q_vec;
}

/* Routine calculate number of used sub_vector */
static int mv_pp22_calculate_num_sub_vector(struct mv_pp2x_port *port)
{
	int num_of_sub_vectors = 0, cold_cpu_address_spaces = 0;
	int num_rx_queues = mv_pp2x_num_cos_queues * num_of_sub_vectors;

	/* Logic: If system has more than 8 CPU's, more than 1 of sub queue vector should be used
	 * for same interrupt and address space, number of sub queue vector = # of active CPU's / 8.
	 * Procedure also check if in multi queue mode exist cold CPU's.
	 */
	if (num_active_cpus() > MVPP2_MAX_CPUS_IN_AP) {
		num_of_sub_vectors = num_active_cpus() / MVPP2_MAX_CPUS_IN_AP;
		if (port->priv->rx_count < num_active_cpus() && mv_pp2x_queue_mode == MVPP2_QDIST_MULTI_MODE) {
			num_of_sub_vectors = ((port->priv->rx_count / MVPP2_MAX_CPUS_IN_AP) > 1) ?
				(port->priv->rx_count / MVPP2_MAX_CPUS_IN_AP) : 0;
			cold_cpu_address_spaces = port->priv->other_count;
			if (num_rx_queues > MVPP22_MAX_NUM_RXQ)
				WARN(1, "Number of requested RXQ's greater than amount of phys RXQ's\n");
		} else {
			port->priv->other_count = 0;
		}
	}

	port->priv->other_count = cold_cpu_address_spaces;

	return num_of_sub_vectors;
}

/* This procedure allocated queue vectors and sub queue vectors per driver configurations.
 * For multi queue mode:
 * If more than 8 CPU's in system and # of HOT CPU's above 8. More than one sub vector would be
 * allocated per queue vector. Also for Cold CPU's MVPP2_TX_SHARED interrupt would be allocated.
 * For example: If system has 16 CPU's and 8 Hot CPU's, then there would be 8 cold CPU's.
 * For single queue mode:
 * If more than 8 CPU's in system -> More than one sub vector would be allocated per queue vector
 * for TX done procedure. All RX will be handled by same MVPP2_RX_SHARED interrupt and on address space.
 * Single resource mode: Single queue vector, address space and MVPP2_PRIVATE Interrupt allocated.
 */
static void mv_pp22_queue_vectors_init(struct mv_pp2x_port *port)
{
	int address_space = 0, num_private_q_vec, num_of_sub_vectors, i, num_rx_queues;
	int sw_thread_index = 0, irq_index = 0, cpu_ap_ofset = 0;
	struct queue_vector *q_vec;
	struct sub_queue_vector *sub_q_vec;
	unsigned long queue_mask = 0;

	bitmap_set(&queue_mask, 0, mv_pp2x_num_cos_queues);

	port->q_vector = devm_kcalloc(port->dev->dev.parent, MVPP2_MAX_ADDR_SPACES,
		sizeof(struct queue_vector), GFP_KERNEL);
	q_vec = port->q_vector;

	/* For single resource mode skip sub vector allocation.
	 * In this mode single queue vector, address space and Interrupt used.
	 */
	if (mv_pp2x_queue_mode == MVPP2_SINGLE_RESOURCE_MODE)
		goto last_queue_vector;

	/* Each cpu has queue_vector for private tx_done counters and/or
	 * private rx_queues
	 */
	num_private_q_vec = mv_pp22_get_num_private_queue_vectors(port);

	/* Calculate number of sub queue vectors */
	num_of_sub_vectors = mv_pp22_calculate_num_sub_vector(port);

	for (address_space = 0; address_space < num_private_q_vec; address_space++) {
		q_vec[address_space].parent = port;
		q_vec[address_space].qv_type = MVPP2_PRIVATE;
		q_vec[address_space].sw_thread_id = sw_thread_index++;
		mv_pp22_allocate_hif_for_kernel(&port->priv->hw, q_vec[address_space].sw_thread_id);
		q_vec[address_space].sw_thread_mask = (1 << q_vec[address_space].sw_thread_id);
		q_vec[address_space].pending_cause_rx = 0;
		q_vec[address_space].num_of_sub_vectors = num_of_sub_vectors;
		/* TODO: AP ID should be static varible */
		q_vec[address_space].ap_id = 0;
		q_vec[address_space].queue_mask = 0;
		q_vec[address_space].cold_cpu = false;
		if (port->interrupt_tx_done || mv_pp2x_queue_mode == MVPP2_QDIST_MULTI_MODE)
			q_vec[address_space].irq = port->of_irqs[irq_index++];

		if (mv_pp2x_queue_mode == MVPP2_QDIST_MULTI_MODE) {
			/* Check if there singe CPU per queue vector or # of sub vectors should be allocated
			 * for one queue vector.
			 */
			if (!num_of_sub_vectors) {
				q_vec[address_space].num_rx_queues = mv_pp2x_num_cos_queues;
				q_vec[address_space].first_rx_queue = address_space * mv_pp2x_num_cos_queues;

				q_vec[address_space].sub_vec = devm_kcalloc(port->dev->dev.parent, 1,
					sizeof(*q_vec[address_space].sub_vec), GFP_KERNEL);
				sub_q_vec = mv_pp22_allocate_sub_vector(port, &q_vec[address_space], queue_mask);
				sub_q_vec->queue_mask |= (queue_mask << MVPP2_CAUSE_TXQ_OCCUP_DESC_ALL_OFFSET);
				q_vec[address_space].queue_mask = sub_q_vec->queue_mask;
				q_vec[address_space].sub_vec[0] = sub_q_vec;
			} else {
				/* Calculate CPU AP offset. AP0: CPU 0-7, AP1 CPU 8-15 and etc */
				cpu_ap_ofset = q_vec[address_space].ap_id * MVPP2_MAX_CPUS_IN_AP;
				num_rx_queues = mv_pp2x_num_cos_queues * num_of_sub_vectors;
				q_vec[address_space].sub_vec = devm_kcalloc(port->dev->dev.parent, num_of_sub_vectors,
					sizeof(*q_vec[address_space].sub_vec), GFP_KERNEL);
				for (i = 0; i < num_of_sub_vectors; i++) {
					sub_q_vec = mv_pp22_allocate_sub_vector(port, &q_vec[address_space],
										(queue_mask) <<
										(i * mv_pp2x_num_cos_queues));
					sub_q_vec->cpu_id = (address_space + cpu_ap_ofset) * num_of_sub_vectors + i;
					q_vec[address_space].queue_mask |= sub_q_vec->queue_mask;
					/* Only first sub_q_vec will handle TX done */
					if (i == 0) {
						sub_q_vec->queue_mask |=
							(queue_mask << MVPP2_CAUSE_TXQ_OCCUP_DESC_ALL_OFFSET);
						q_vec[address_space].queue_mask |=
							(queue_mask << MVPP2_CAUSE_TXQ_OCCUP_DESC_ALL_OFFSET);
					}
					q_vec[address_space].sub_vec[i] = sub_q_vec;
				}
				q_vec[address_space].num_rx_queues = num_rx_queues;
				q_vec[address_space].first_rx_queue = address_space * num_rx_queues;
			}
		} else {
			/* For single queue mode only sub vector will handle TX done for all CPU's in queue vector */
			q_vec[address_space].first_rx_queue = 0;
			q_vec[address_space].num_rx_queues = 0;
			q_vec[address_space].sub_vec = devm_kcalloc(port->dev->dev.parent, 1,
								    sizeof(*q_vec[address_space].sub_vec), GFP_KERNEL);
			q_vec[address_space].queue_mask = (queue_mask << MVPP2_CAUSE_TXQ_OCCUP_DESC_ALL_OFFSET);
			sub_q_vec = mv_pp22_allocate_sub_vector(port, &q_vec[address_space],
								q_vec[address_space].queue_mask);
			q_vec[address_space].sub_vec[0] = sub_q_vec;
		}
		port->num_qvector++;
	}

last_queue_vector:
	if (mv_pp2x_queue_mode == MVPP2_QDIST_MULTI_MODE) {
		/* Additional queue_vector's for TX Shared queue vectors */
		for (i = address_space; i < (num_private_q_vec + port->priv->other_count); i++) {
			q_vec[address_space].parent = port;
			q_vec[address_space].qv_type = MVPP2_TX_SHARED;
			q_vec[address_space].sw_thread_id = sw_thread_index++;
			mv_pp22_allocate_hif_for_kernel(&port->priv->hw, q_vec[address_space].sw_thread_id);
			q_vec[address_space].sw_thread_mask = (1 << q_vec[address_space].sw_thread_id);
			q_vec[address_space].pending_cause_rx = 0;
			q_vec[address_space].num_of_sub_vectors = num_of_sub_vectors;
			/* TODO: AP ID should be static varible */
			q_vec[address_space].ap_id = 0;
			q_vec[address_space].first_rx_queue = 0;
			q_vec[address_space].num_rx_queues = 0;
			q_vec[address_space].irq = port->of_irqs[irq_index];
			q_vec[address_space].sub_vec = devm_kcalloc(port->dev->dev.parent, 1,
								    sizeof(*q_vec[address_space].sub_vec), GFP_KERNEL);
			q_vec[address_space].queue_mask = (queue_mask << MVPP2_CAUSE_TXQ_OCCUP_DESC_ALL_OFFSET);
			sub_q_vec = mv_pp22_allocate_sub_vector(port, &q_vec[address_space],
								q_vec[address_space].queue_mask);
			q_vec[address_space].sub_vec[0] = sub_q_vec;
			q_vec[address_space].cold_cpu = true;
			port->num_qvector++;
		}
	} else {
		/* Additional queue_vector for Shared RX or Single resource mode */
		q_vec[address_space].parent = port;
		q_vec[address_space].sw_thread_id = irq_index;
		mv_pp22_allocate_hif_for_kernel(&port->priv->hw, q_vec[address_space].sw_thread_id);
		q_vec[address_space].pending_cause_rx = 0;
		q_vec[address_space].sub_vec = devm_kcalloc(port->dev->dev.parent, 1,
								sizeof(*q_vec[address_space].sub_vec), GFP_KERNEL);
		q_vec[address_space].sw_thread_mask = (1 << q_vec[address_space].sw_thread_id);
		q_vec[address_space].irq = port->of_irqs[irq_index];

		q_vec[address_space].qv_type = MVPP2_RX_SHARED;
		/* Add TX done queue mask for single resource mode(should handle both TX done and RX)
		 * and set MVPP2_PRIVATE interrupt type(used by singe CPU 0)
		 */
		if (mv_pp2x_queue_mode == MVPP2_QDIST_SINGLE_MODE) {
			q_vec[address_space].qv_type = MVPP2_RX_SHARED;
		} else {
			q_vec[address_space].qv_type = MVPP2_PRIVATE;
			queue_mask |= (queue_mask << MVPP2_CAUSE_TXQ_OCCUP_DESC_ALL_OFFSET);
		}
		sub_q_vec = mv_pp22_allocate_sub_vector(port, &q_vec[address_space], queue_mask);
		q_vec[address_space].queue_mask = sub_q_vec->queue_mask;
		q_vec[address_space].sub_vec[0] = sub_q_vec;
		q_vec[address_space].first_rx_queue = 0;
		q_vec[address_space].num_rx_queues = port->num_rx_queues;
		q_vec[address_space].cold_cpu = false;
		q_vec[address_space].ap_id = 0;
		port->num_qvector++;
	}
}

static void mv_pp2x_port_irq_names_update(struct mv_pp2x_port *port)
{
	int i, cpu;
	struct queue_vector *q_vec = &port->q_vector[0];
	char str_common[32];
	struct net_device  *net_dev = port->dev;
	struct device *parent_dev;

	parent_dev = net_dev->dev.parent;

	snprintf(str_common, sizeof(str_common), "%s.%s",
		 dev_name(parent_dev), net_dev->name);

	if (port->priv->pp2_version == PPV21) {
		snprintf(q_vec[0].irq_name, IRQ_NAME_SIZE, "%s", str_common);
		return;
	}

	for (i = 0; i < port->num_qvector; i++) {
		if (!q_vec[i].irq)
			continue;
		if (q_vec[i].qv_type == MVPP2_PRIVATE) {
			if (!q_vec[i].num_of_sub_vectors) {
				cpu = QV_THR_2_MASTER_AP_CPU(q_vec[i].ap_id, q_vec[i].sw_thread_id);
				snprintf(q_vec[i].irq_name, IRQ_NAME_SIZE, "%s.%s%d",
					 str_common, "cpu", cpu);
			} else {
				snprintf(q_vec[i].irq_name, IRQ_NAME_SIZE, "%s.%s_%d",
					 str_common, "sw_thread", q_vec[i].sw_thread_id);
			}
		} else if (q_vec[i].qv_type == MVPP2_TX_SHARED) {
			snprintf(q_vec[i].irq_name, IRQ_NAME_SIZE, "%s.%s",
				 str_common, "tx_shared");
		} else {
			snprintf(q_vec[i].irq_name, IRQ_NAME_SIZE, "%s.%s",
				 str_common, "rx_shared");
		}
	}
	snprintf(port->mac_data.irq_name, IRQ_NAME_SIZE, "%s.%s", str_common,
		 "link");
}

static void mv_pp21x_port_isr_rx_group_cfg(struct mv_pp2x_port *port)
{
	mv_pp21_isr_rx_group_write(&port->priv->hw, port->id,
				   port->num_rx_queues);
}

static void mv_pp22_port_isr_rx_group_cfg(struct mv_pp2x_port *port)
{
	int i;
/*	u8 cur_rx_queue; */
	struct mv_pp2x_hw *hw = &port->priv->hw;

	for (i = 0; i < port->num_qvector; i++) {
		if (port->q_vector[i].num_rx_queues != 0) {
			mv_pp22_isr_rx_group_write(hw, port->id,
						   port->q_vector[i].sw_thread_id,
				port->q_vector[i].first_rx_queue,
				port->q_vector[i].num_rx_queues);
		}
	}
}

static int mv_pp2_init_emac_data(struct mv_pp2x_port *port,
				 struct device_node *emac_node)
{
	struct device_node *fixed_link_node, *phy_node;
	int phy_mode;
	u32 speed, id;

	if (of_property_read_u32(emac_node, "port-id", &id))
		return -EINVAL;

	port->mac_data.gop_index = id;
	port->mac_data.link_irq = irq_of_parse_and_map(emac_node, 0);

	phy_mode = of_get_phy_mode(emac_node);

	if (of_phy_is_fixed_link(emac_node)) {
		port->mac_data.force_link = true;
		port->mac_data.link = true;
		fixed_link_node = of_get_child_by_name(emac_node, "fixed-link");
		port->mac_data.duplex = of_property_read_bool(fixed_link_node,
				"full-duplex");
		if (port->mac_data.speed == SPEED_2500)
			port->mac_data.flags |= MV_EMAC_F_SGMII2_5;
		if (of_property_read_u32(fixed_link_node, "speed",
					 &port->mac_data.speed))
			return -EINVAL;
	} else {
		port->mac_data.force_link = false;

		switch (phy_mode) {
		case PHY_INTERFACE_MODE_SGMII:
		case PHY_INTERFACE_MODE_1000BASEX:
			speed = 0;
			/* check phy speed */
			of_property_read_u32(emac_node, "phy-speed", &speed);
			switch (speed) {
			case SPEED_1000:
				port->mac_data.speed = SPEED_1000; /* sgmii */
				break;
			case SPEED_2500:
				port->mac_data.speed = SPEED_2500; /* sgmii */
				port->mac_data.flags |= MV_EMAC_F_SGMII2_5;
				break;
			default:
				port->mac_data.speed = SPEED_1000; /* sgmii */
			}
			break;
		case PHY_INTERFACE_MODE_RXAUI:
			break;
		case PHY_INTERFACE_MODE_QSGMII:
			break;
		case PHY_INTERFACE_MODE_RGMII:
			break;
		case PHY_INTERFACE_MODE_KR:
		case PHY_INTERFACE_MODE_SFI:
		case PHY_INTERFACE_MODE_XFI:
			speed = 0;
			/* check phy speed */
			of_property_read_u32(emac_node, "phy-speed", &speed);
			switch (speed) {
			case SPEED_10000:
				port->mac_data.speed = SPEED_10000;
				break;
			case SPEED_5000:
				port->mac_data.speed = SPEED_5000;
				port->mac_data.flags |= MV_EMAC_F_5G;
				break;
			default:
				port->mac_data.speed = SPEED_10000;
			}
			break;

		default:
			netdev_err(port->dev, "%s: incorrect phy-mode\n", __func__);
			return -1;
		}
	}
	port->mac_data.phy_mode = phy_mode;
	pr_info("gop_mac(%d), phy_mode(%d)=%s phy-speed=%d\n", id,  phy_mode,
		phy_modes(phy_mode), port->mac_data.speed);

	phy_node = of_parse_phandle(emac_node, "phy", 0);
	if (phy_node) {
		port->mac_data.phy_node = phy_node;
		if (of_property_read_u32(phy_node, "reg",
					 &port->mac_data.phy_addr))
			netdev_err(port->dev, "%s: NO PHY address on emac %d\n",
				   __func__, port->mac_data.gop_index);

		pr_debug("gop_mac(%d), phy_reg(%d)\n", id,
			 port->mac_data.phy_addr);
	} else {
		pr_debug("No PHY NODE on emac %d\n", id);
	}
return 0;
}

static u32 mvp_pp2x_gop110_netc_cfg_create(struct mv_pp2x *priv)
{
	u32 val = 0;
	int i;
	struct mv_pp2x_port *port;
	struct mv_mac_data *mac;

	for (i = 0; i < priv->num_ports; i++) {
		port = priv->port_list[i];
		mac = &port->mac_data;
		if (mac->gop_index == 0) {
			if (mac->phy_mode == PHY_INTERFACE_MODE_XAUI)
				val |= MV_NETC_GE_MAC0_XAUI;
			else if (mac->phy_mode == PHY_INTERFACE_MODE_RXAUI)
				val |= MV_NETC_GE_MAC0_RXAUI_L23;
		}
		if (mac->gop_index == 2) {
			if (mac->phy_mode == PHY_INTERFACE_MODE_SGMII ||
			    mac->phy_mode == PHY_INTERFACE_MODE_1000BASEX)
				val |= MV_NETC_GE_MAC2_SGMII;
		}
		if (mac->gop_index == 3) {
			if (mac->phy_mode == PHY_INTERFACE_MODE_SGMII ||
			    mac->phy_mode == PHY_INTERFACE_MODE_1000BASEX)
				val |= MV_NETC_GE_MAC3_SGMII;
			else if (mac->phy_mode == PHY_INTERFACE_MODE_RGMII)
				val |= MV_NETC_GE_MAC3_RGMII;
		}
	}
	return val;
}

static void mv_pp2x_get_port_stats(struct mv_pp2x_port *port)
{
	bool link_is_up;
	struct mv_mac_data *mac = &port->mac_data;
	struct gop_hw *gop = &port->priv->hw.gop;
	int gop_port = mac->gop_index;
	struct gop_stat	*gop_statistics = &mac->gop_statistics;

	if (port->priv->pp2_version == PPV21)
		return;
	if (!(port->flags & MVPP2_F_LOOPBACK)) {
		link_is_up = mv_gop110_port_is_link_up(gop, &port->mac_data);
		if (link_is_up) {
			mv_gop110_mib_counters_stat_update(gop, gop_port, gop_statistics);
			mv_pp2x_counters_stat_update(port, gop_statistics);
		}
	}
}

static void mv_pp2x_get_device_stats(struct work_struct *work)
{
	struct delayed_work *delay = to_delayed_work(work);
	struct mv_pp2x *priv = container_of(delay, struct mv_pp2x,
						 stats_task);
	int i;

	for (i = 0; i < priv->num_ports; i++) {
		if (priv->port_list[i])
			mv_pp2x_get_port_stats(priv->port_list[i]);
	}

	queue_delayed_work(priv->workqueue, &priv->stats_task, stats_delay);
}

/* Initialize port HW */
static int mv_pp2x_port_hw_init(struct mv_pp2x_port *port)
{
	struct mv_pp2x *priv = port->priv;
	struct gop_hw *gop = &port->priv->hw.gop;
	struct mv_mac_data *mac = &port->mac_data;
	int err = 0;

	/* Disable port */
	mv_pp2x_egress_disable(port);

	if (port->priv->pp2_version == PPV21)
		mv_pp21_port_disable(port);
	else
		if (!(port->flags & MVPP2_F_LOOPBACK))
			mv_gop110_port_disable(gop, mac, port->comphy);

	/* Configure Rx queue group interrupt for this port */
	priv->pp2xdata->mv_pp2x_port_isr_rx_group_cfg(port);

	mv_pp2x_ingress_disable(port);

	/* Port default configuration */
	mv_pp2x_defaults_set(port);

	/* Port's classifier configuration */
	mv_pp2x_cls_oversize_rxq_set(port);
	mv_pp2x_cls_port_config(port);

	/* Initialize pools for swf */
	err = mv_pp2x_swf_bm_pool_init(port);

	return err;
}

/* Initialize port */
static int mv_pp2x_port_init(struct mv_pp2x_port *port)
{
	struct device *dev = port->dev->dev.parent;
	int queue, err;

	/* Allocate queues */
	port->txqs = devm_kcalloc(dev, port->num_tx_queues, sizeof(*port->txqs),
				  GFP_KERNEL);
	if (!port->txqs)
		return -ENOMEM;
	port->rxqs = devm_kcalloc(dev, port->num_rx_queues, sizeof(*port->rxqs),
				  GFP_KERNEL);
	if (!port->rxqs)
		return -ENOMEM;

	/* Associate physical Tx queues to port and initialize.  */
	err = mv_pp2x_port_txqs_init(dev, port);

	if (err)
		goto out;

	/* Associate physical Rx queues to port and initialize.  */
	err = mv_pp2x_port_rxqs_init(dev, port);
	if (err)
		goto out;

	/* Create Rx descriptor rings */
	for (queue = 0; queue < port->num_rx_queues; queue++) {
		struct mv_pp2x_rx_queue *rxq = port->rxqs[queue];

		rxq->size = port->rx_ring_size;
		rxq->pkts_coal = MVPP2_RX_COAL_PKTS;
		rxq->time_coal = MVPP2_RX_COAL_USEC;
	}

	/* Provide an initial Rx packet size */
	port->pkt_size = MVPP2_RX_PKT_SIZE(port->dev->mtu);

	/* Configure queue_vectors */
	port->priv->pp2xdata->mv_pp2x_port_queue_vectors_init(port);

	err = mv_pp2x_port_hw_init(port);
	if (err)
		goto out;
	return 0;

out:
	mv_pp2x_port_napi_remove(port);

	return err;
}

static void mv_pp2x_port_init_config(struct mv_pp2x_port *port)
{
	/* CoS init config */
	port->cos_cfg.cos_classifier = cos_classifer;
	port->cos_cfg.default_cos = default_cos;
	port->cos_cfg.num_cos_queues = mv_pp2x_num_cos_queues;
	port->cos_cfg.pri_map = pri_map;

	/* RSS init config */
	port->rss_cfg.dflt_cpu = default_cpu;
	/* RSS is disabled as default, it can be update when running */
	port->rss_cfg.rss_en = 0;
	port->rss_cfg.rss_mode = rss_mode;
	port->priv->rx_count = mv_pp2x_rx_count;
	port->priv->other_count = MVPP2_DEFAULT_OTHER_COUNT;
}

/* Routine called by port CPU hot plug notifier. If port up callback set irq affinity for private interrupts,
*  unmask private interrupt, set packet coalescing and clear counters.
*/
static int mv_pp2x_port_cpu_callback(struct notifier_block *nfb,
				     unsigned long action, void *hcpu)
{
	unsigned int cpu = (unsigned long)hcpu;
	int qvec_id;
	struct queue_vector *qvec;
	struct mv_pp2x_port *port = container_of(nfb, struct mv_pp2x_port, port_hotplug_nb);
	struct mv_pp2x_aggr_tx_queue *aggr_txq;
	struct mv_pp2x_cp_pcpu *cp_pcpu;

	switch (action) {
	case CPU_ONLINE:
	case CPU_ONLINE_FROZEN:
		for (qvec_id = 0; qvec_id < port->num_qvector; qvec_id++) {
			qvec = &port->q_vector[qvec_id];
			if (!qvec->irq)
				continue;
			if (qvec->qv_type == MVPP2_PRIVATE &&
			    (QV_THR_2_MASTER_AP_CPU(qvec->ap_id, qvec->sw_thread_id) == cpu)) {
				irq_set_affinity_hint(qvec->irq, cpumask_of(cpu));
				mv_pp22_private_interrupt_unmask(port, cpu);
				if (port->priv->pp2_cfg.queue_mode == MVPP2_QDIST_MULTI_MODE)
					irq_set_status_flags(qvec->irq, IRQ_NO_BALANCING);
				/* FIXME: A810 support*/
			}
		}
		if (port->interrupt_tx_done)
			mv_pp2x_tx_done_pkts_coal_set(port, cpu);
		break;
	case CPU_DEAD:
	case CPU_DEAD_FROZEN:
		cp_pcpu = port->priv->pcpu[cpu];
		aggr_txq = &port->priv->aggr_txqs[cpu];
		mv_pp2x_tx_timer_kill(cp_pcpu);
		aggr_txq->hw_count += aggr_txq->sw_count;
		mv_pp22_thread_write(&port->priv->hw, cpu, MVPP2_AGGR_TXQ_UPDATE_REG, aggr_txq->sw_count);
		aggr_txq->sw_count = 0;
	}

	return NOTIFY_OK;
}

static int mv_pp22_uio_mem_map(struct mv_pp2x_uio *pp2x_uio, struct resource *res)
{
	struct uio_mem	*uio_mem = &pp2x_uio->u_info.mem[pp2x_uio->num_maps];

	if (pp2x_uio->num_maps >= MAX_UIO_MAPS) {
		pr_err("Too many UIO maps requests\n");
		return -ENOSPC;
	}

	uio_mem->memtype = UIO_MEM_PHYS;
	uio_mem->addr = res->start & PAGE_MASK;
	uio_mem->size = PAGE_ALIGN(resource_size(res));
	uio_mem->name = res->name;

	pp2x_uio->num_maps++;

	pr_debug("uio: addr(%llx) size(%llx) name(%s) map_num(%d)\n",
		 uio_mem->addr, uio_mem->size, uio_mem->name,
		 pp2x_uio->num_maps);

	return 0;
}

int mv_pp2x_port_musdk_set(void *netdev_priv)
{
	struct mv_pp2x_port *port = netdev_priv;
	bool was_running = false;

	if (port->flags & MVPP2_F_IF_MUSDK)
		return 0;

	pr_debug("mv_pp2_uio_open %s\n", port->dev->name);

	/* Close device before updates */
	if (netif_running(port->dev)) {
		rtnl_lock();
		dev_close(port->dev);
		was_running = true;
	}
	/* Backup num configured entities */
	port->cfg_num_qvector   = port->num_qvector;
	port->cfg_num_rx_queues = port->num_rx_queues;
	port->cfg_num_tx_queues = port->num_tx_queues;

	/* Set num configured entities to 0 */
	port->num_qvector = 0;
	port->num_rx_queues = 0;
	port->num_tx_queues = 0;

	port->flags |= MVPP2_F_IF_MUSDK;
	if (was_running) {
		dev_open(port->dev);
		rtnl_unlock();
	}
	return 0;
}
EXPORT_SYMBOL(mv_pp2x_port_musdk_set);

int mv_pp2x_port_musdk_clear(void *netdev_priv)
{
	struct mv_pp2x_port *port = netdev_priv;

	if (!(port->flags & MVPP2_F_IF_MUSDK))
		return 0;

	pr_debug("mv_pp2_uio_release %s\n", port->dev->name);

	/* Close device before updates */
	if (netif_running(port->dev)) {
		rtnl_lock();
		dev_close(port->dev);
		rtnl_unlock();
	}

	/* Set num configured entities back to configured values */
	port->num_qvector   = port->cfg_num_qvector;
	port->num_rx_queues = port->cfg_num_rx_queues;
	port->num_tx_queues = port->cfg_num_tx_queues;

	port->flags &= ~MVPP2_F_IF_MUSDK;
	return 0;
}
EXPORT_SYMBOL(mv_pp2x_port_musdk_clear);

static int mv_pp2_uio_open(struct uio_info *info, struct inode *inode)
{
	int err;
	struct mv_pp2x_port *port = info->priv;

	err = mv_pp2x_port_musdk_set(port);
	return err;
}

static int mv_pp2_uio_release(struct uio_info *info, struct inode *inode)
{
	int err;
	struct mv_pp2x_port *port = info->priv;

	err = mv_pp2x_port_musdk_clear(port);
	return err;
}

/* Ports initialization */
static int mv_pp2x_port_probe(struct platform_device *pdev,
			      struct device_node *port_node,
			    struct mv_pp2x *priv)
{
	struct device_node *emac_node = NULL;
	struct device_node *phy_node = NULL;
	struct mv_pp2x_port *port;
	struct mv_pp2x_port_pcpu *port_pcpu;
	struct net_device *dev;
	struct resource *res;
	const char *dt_mac_addr = NULL;
	const char *mac_from;
	char hw_mac_addr[ETH_ALEN] = {0};
	u32 id;
	int features, err = 0, i, cpu;
	int priv_common_regs_num = 2;
	struct mv_pp2x_ext_buf_struct *ext_buf_struct;
	unsigned int *port_irqs;
	int port_num_irq;
	int phy_mode;
	struct phy *comphy = NULL;
	const char *musdk_status;
	int statlen;

	if (of_property_read_bool(port_node, "marvell,loopback")) {
		dev = alloc_netdev_mqs(sizeof(struct mv_pp2x_port), "pp2_lpbk%d", NET_NAME_UNKNOWN,
				       ether_setup, mv_pp2x_txq_number * num_active_cpus(), mv_pp2x_rxq_number);
	} else {
		dev = alloc_etherdev_mqs(sizeof(struct mv_pp2x_port),
					 mv_pp2x_txq_number * num_active_cpus(), mv_pp2x_rxq_number);
	}
	if (!dev)
		return -ENOMEM;

	/* Setup XPS mapping */
	for (cpu = 0; cpu < num_online_cpus(); cpu++)
		netif_set_xps_queue(dev, cpumask_of(cpu), cpu);

	/*Connect entities */
	port = netdev_priv(dev);
	port->dev = dev;
	SET_NETDEV_DEV(dev, &pdev->dev);
	port->priv = priv;
	port->flags = 0;
	port->interrupt_tx_done = port->priv->pp2xdata->interrupt_tx_done;

	musdk_status = of_get_property(port_node, "musdk-status", &statlen);

	mv_pp2x_port_init_config(port);

	if (of_property_read_u32(port_node, "port-id", &id)) {
		err = -EINVAL;
		dev_err(&pdev->dev, "missing port-id value\n");
		goto err_free_netdev;
	}
	port->id = id;

	if (priv->pp2_version == PPV21) {
		phy_node = of_parse_phandle(port_node, "phy", 0);
		if (!phy_node) {
			dev_err(&pdev->dev, "missing phy\n");
			err = -ENODEV;
			goto err_free_netdev;
		}

		phy_mode = of_get_phy_mode(port_node);
		if (phy_mode < 0) {
			dev_err(&pdev->dev, "incorrect phy mode\n");
			err = phy_mode;
			goto err_free_netdev;
		}
		port->mac_data.phy_mode = phy_mode;
		port->mac_data.phy_node = phy_node;
		emac_node = port_node;
	} else {
		if (of_property_read_bool(port_node, "marvell,loopback"))
			port->flags |= MVPP2_F_LOOPBACK;

		if (!(port->flags & MVPP2_F_LOOPBACK)) {
			emac_node = of_parse_phandle(port_node, "emac-data", 0);
			if (!emac_node) {
				dev_err(&pdev->dev, "missing emac-data\n");
				err = -EINVAL;
				goto err_free_netdev;
			}
			/* Init emac_data, includes link interrupt */
			if (mv_pp2_init_emac_data(port, emac_node))
				goto err_free_netdev;

			comphy = devm_of_phy_get(&pdev->dev, emac_node, "comphy");

			if (!IS_ERR(comphy))
				port->comphy = comphy;
		} else {
			port->mac_data.link_irq = MVPP2_NO_LINK_IRQ;
		}
	}

	if (port->mac_data.phy_node) {
		err = mv_pp2x_phy_connect(port);
		if (err < 0)
			goto err_free_netdev;
	}

	/* get MAC address */
	if (emac_node)
		dt_mac_addr = of_get_mac_address(emac_node);

	if (dt_mac_addr && is_valid_ether_addr(dt_mac_addr)) {
		mac_from = "device tree";
		ether_addr_copy(dev->dev_addr, dt_mac_addr);
	} else {
		if (priv->pp2_version == PPV21)
			mv_pp21_get_mac_address(port, hw_mac_addr);
		if (is_valid_ether_addr(hw_mac_addr)) {
			mac_from = "hardware";
			ether_addr_copy(dev->dev_addr, hw_mac_addr);
		} else {
			mac_from = "random";
			eth_hw_addr_random(dev);
		}
	}
	pr_info("mac_addr %x:%x:%x:%x:%x:%x",
		dev->dev_addr[0],
			dev->dev_addr[1], dev->dev_addr[2], dev->dev_addr[3],
			dev->dev_addr[4], dev->dev_addr[5]);

	/* Tx/Rx Interrupt */
	port_num_irq = mv_pp2x_of_irq_count(port_node);
	if (port_num_irq != priv->pp2xdata->num_port_irq) {
		dev_err(&pdev->dev,
			"port(%d)-number of irq's doesn't match hw\n", id);
		goto err_free_netdev;
	}
	port_irqs = devm_kcalloc(&pdev->dev, port_num_irq,
				 sizeof(u32), GFP_KERNEL);
	port->of_irqs = port_irqs;
	port->num_irqs = 0;
	for (i = 0; i < port_num_irq; i++) {
		port_irqs[i] = irq_of_parse_and_map(port_node, i);
		if (port_irqs[i] == 0) {
			dev_err(&pdev->dev,
				"Fail to parse port(%d), irq(%d)\n", id, i);
			err = -EINVAL;
			goto err_free_irq;
		}
		port->num_irqs++;
	}

	dev->tx_queue_len = tx_queue_size;
	dev->watchdog_timeo = 5 * HZ;

	port->num_tx_queues = mv_pp2x_txq_number;
	port->num_rx_queues = mv_pp2x_rxq_number;
	dev->netdev_ops = &mv_pp2x_netdev_ops;
	mv_pp2x_set_ethtool_ops(dev);

	if (priv->pp2_version == PPV21)
		port->first_rxq = (port->id) * mv_pp2x_rxq_number +
			first_log_rxq_queue;
	else
		port->first_rxq = (port->id) * priv->pp2xdata->pp2x_max_port_rxqs +
			first_log_rxq_queue;

	if (priv->pp2_version == PPV21) {
		res = platform_get_resource(pdev, IORESOURCE_MEM,
					    priv_common_regs_num + id);
		port->base = devm_ioremap_resource(&pdev->dev, res);
		if (IS_ERR(port->base)) {
			err = PTR_ERR(port->base);
			goto err_free_irq;
		}
	}

	/* Alloc per-cpu stats */
	port->stats = netdev_alloc_pcpu_stats(struct mv_pp2x_pcpu_stats);
	if (!port->stats) {
		err = -ENOMEM;
		goto err_free_irq;
	}

	port->tx_ring_size = tx_queue_size;
	port->rx_ring_size = rx_queue_size;

	mv_pp2x_check_queue_size_valid(port);

	err = mv_pp2x_port_init(port);
	if (err < 0) {
		dev_err(&pdev->dev, "failed to init port %d\n", id);
		goto err_free_stats;
	}
	if (port->priv->pp2_version == PPV21)
		mv_pp21_port_power_up(port);

	port->pcpu = devm_kcalloc(&pdev->dev, MVPP2_MAX_ADDR_SPACES, sizeof(*port->pcpu),
								GFP_KERNEL);
	if (!port->pcpu) {
		err = -ENOMEM;
		goto err_free_stats;
	}

	for (cpu = 0; cpu < mv_pp2x_used_addr_spaces; cpu++) {
		port_pcpu = devm_kcalloc(&pdev->dev, 1, sizeof(*port_pcpu), GFP_KERNEL);
		port->pcpu[cpu] = port_pcpu;
		if (((port->flags & MVPP2_F_LOOPBACK)) && port->interrupt_tx_done)
			continue;
		hrtimer_init(&port_pcpu->tx_done_timer, CLOCK_MONOTONIC,
			     HRTIMER_MODE_REL_PINNED);
		port_pcpu->tx_done_timer.function = mv_pp2x_hr_timer_cb;
		port_pcpu->timer_scheduled = false;

		tasklet_init(&port_pcpu->tx_done_tasklet,
			     mv_pp2x_tx_proc_cb, (unsigned long)dev);
	}

	/* Init pool of external buffers for TSO, fragmentation, etc */
	for (cpu = 0; cpu < mv_pp2x_used_addr_spaces; cpu++) {
		port_pcpu = port->pcpu[cpu];
		port_pcpu->ext_buf_size = MVPP2_EXTRA_BUF_SIZE;

		INIT_LIST_HEAD(&port_pcpu->ext_buf_port_list);
		port_pcpu->ext_buf_pool = devm_kzalloc(port->dev->dev.parent,
			sizeof(struct mv_pp2x_ext_buf_pool), GFP_ATOMIC);
		port_pcpu->ext_buf_pool->buf_pool_size = MVPP2_EXTRA_BUF_NUM;
		port_pcpu->ext_buf_pool->ext_buf_struct =
			devm_kzalloc(port->dev->dev.parent,
				     sizeof(*ext_buf_struct) * MVPP2_EXTRA_BUF_NUM, GFP_ATOMIC);

		for (i = 0; i < MVPP2_EXTRA_BUF_NUM; i++) {
			u8 *ext_buf = kmalloc(MVPP2_EXTRA_BUF_SIZE, GFP_ATOMIC);

			port_pcpu->ext_buf_pool->ext_buf_struct[i].ext_buf_data = ext_buf;
			if (!ext_buf) {
				pr_warn("\to %s Warning: %d of %d extra buffers allocated\n",
					__func__, i, MVPP2_EXTRA_BUF_NUM);
				break;
			}
			list_add(&port_pcpu->ext_buf_pool->ext_buf_struct[i].ext_buf_list,
				 &port_pcpu->ext_buf_port_list);
			mv_pp2x_extra_pool_inc(port_pcpu->ext_buf_pool);
			port_pcpu->ext_buf_pool->buf_pool_in_use++;
		}
	}

	features = NETIF_F_SG;
	dev->features = features | NETIF_F_RXCSUM | NETIF_F_IP_CSUM |
			NETIF_F_IPV6_CSUM | NETIF_F_TSO;
	dev->hw_features |= features | NETIF_F_RXCSUM | NETIF_F_GRO |
			NETIF_F_IP_CSUM | NETIF_F_IPV6_CSUM | NETIF_F_TSO;

	/* Only when multi queue mode, rxhash is supported */
	if (mv_pp2x_queue_mode)
		dev->hw_features |= NETIF_F_RXHASH;

	if (dev->features & NETIF_F_TSO)
		port->txq_stop_limit = TSO_TXQ_LIMIT;
	else
		port->txq_stop_limit = TXQ_LIMIT;

	dev->vlan_features |= features;

	/* Add support for VLAN filtering */
	dev->features |= NETIF_F_HW_VLAN_CTAG_FILTER;

	dev->priv_flags |= IFF_UNICAST_FLT;

	err = register_netdev(dev);
	if (err < 0) {
		dev_err(&pdev->dev, "failed to register netdev\n");
		goto err_free_stats;
	}

	/* Register uio_device */
	if (priv->pp2_version == PPV22) {
		port->uio.u_info.name = kasprintf(GFP_KERNEL, UIO_PORT_STRING,
						  priv->pp2_cfg.cell_index, port->id);
		port->uio.u_info.version = "0.1";
		port->uio.u_info.priv = port;
		port->uio.u_info.open = mv_pp2_uio_open;
		port->uio.u_info.release = mv_pp2_uio_release;

		err = uio_register_device(&dev->dev, &port->uio.u_info);
		if (err) {
			dev_err(&dev->dev, "Failed to register uio device\n");
			goto err_unreg_netdev;
		}
	}

	/* Clear MIB and mvpp2 counters statistic */
	mv_gop110_mib_counters_clear(&port->priv->hw.gop, port->mac_data.gop_index);
	mv_pp2x_counters_stat_clear(port);

	mv_pp2x_port_irq_names_update(port);

	if (priv->pp2_version == PPV22)
		port->port_hotplug_nb.notifier_call = mv_pp2x_port_cpu_callback;

	netdev_info(dev, "Using %s mac address %pM\n", mac_from, dev->dev_addr);

	priv->port_list[priv->num_ports] = port;
	priv->num_ports++;

#ifdef DEV_NETMAP
	mv_pp2x_netmap_attach(port);
#endif /* DEV_NETMAP */
#ifdef CONFIG_MV_PTP_SERVICE
	mv_pp2x_ptp_init(pdev, port, id);
#endif

	/* Populate network device of_node */
	dev->dev.of_node = port_node;

	return 0;
	dev_err(&pdev->dev, "%s failed for port_id(%d)\n", __func__, id);

err_unreg_netdev:
	unregister_netdev(dev);
err_free_stats:
	free_percpu(port->stats);
err_free_irq:
	mv_pp2x_port_irqs_dispose_mapping(port);
err_free_netdev:
	free_netdev(dev);
	return err;
}

/* Ports removal routine */
static void mv_pp2x_port_remove(struct mv_pp2x_port *port)
{
#ifdef DEV_NETMAP
	netmap_detach(port->dev);
#endif /* DEV_NETMAP */

	if (port->priv->pp2_version == PPV22) {
		uio_unregister_device(&port->uio.u_info);
		kfree(port->uio.u_info.name);
	}

	mv_pp2x_port_napi_remove(port);
	unregister_netdev(port->dev);

	if (port->mac_data.phy_node)
		mv_pp2x_phy_disconnect(port); /* do after unregister netdev */

	free_percpu(port->stats);
	mv_pp2x_port_irqs_dispose_mapping(port);
	free_netdev(port->dev);
}

/* Initialize decoding windows */
static void mv_pp2x_conf_mbus_windows(const struct mbus_dram_target_info *dram,
				      struct mv_pp2x_hw *hw)
{
	u32 win_enable;
	int i;

	for (i = 0; i < 6; i++) {
		mv_pp2x_write(hw, MVPP2_WIN_BASE(i), 0);
		mv_pp2x_write(hw, MVPP2_WIN_SIZE(i), 0);

		if (i < 4)
			mv_pp2x_write(hw, MVPP2_WIN_REMAP(i), 0);
	}

	win_enable = 0;

	for (i = 0; i < dram->num_cs; i++) {
		const struct mbus_dram_window *cs = dram->cs + i;

		mv_pp2x_write(hw, MVPP2_WIN_BASE(i),
			      (cs->base & 0xffff0000) | (cs->mbus_attr << 8) |
			    dram->mbus_dram_target_id);

		mv_pp2x_write(hw, MVPP2_WIN_SIZE(i),
			      (cs->size - 1) & 0xffff0000);

		win_enable |= (1 << i);
	}

	mv_pp2x_write(hw, MVPP2_BASE_ADDR_ENABLE, win_enable);
}

/* Initialize network controller common part HW */
static int mv_pp2x_init(struct platform_device *pdev, struct mv_pp2x *priv)
{
	int err, i, cpu;
	int last_log_rx_queue;
	u32 val;
	const struct mbus_dram_target_info *dram_target_info;
	u8 pp2_ver = priv->pp2xdata->pp2x_ver;
	struct mv_pp2x_hw *hw = &priv->hw;

	/* Set statistic delay */
	stats_delay = (stats_delay_msec * HZ) / 1000;

	/* Checks for hardware constraints */
	if (mv_pp2x_num_cos_queues <= 0 || mv_pp2x_num_cos_queues > MVPP2_MAX_TXQ) {
		dev_err(&pdev->dev, "Illegal num_cos_queue parameter\n");
		return -EINVAL;
	}

	last_log_rx_queue = first_log_rxq_queue + mv_pp2x_rxq_number;
	if (last_log_rx_queue > priv->pp2xdata->pp2x_max_port_rxqs) {
		dev_err(&pdev->dev, "too high num_cos_queue parameter\n");
		return -EINVAL;
	}

	/*TODO: YuvalC, replace this with a per-pp2x validation function. */
	if ((pp2_ver == PPV21) && (mv_pp2x_rxq_number % 4)) {
		dev_err(&pdev->dev, "invalid num_cos_queue parameter\n");
		return -EINVAL;
	}

	if (mv_pp2x_txq_number > MVPP2_MAX_TXQ) {
		dev_err(&pdev->dev, "invalid num_cos_queue parameter\n");
		return -EINVAL;
	}

	/* MBUS windows configuration */
	dram_target_info = mv_mbus_dram_info();
	if (dram_target_info)
		mv_pp2x_conf_mbus_windows(dram_target_info, hw);

	if (priv->pp2_version == PPV22) {
		mv_pp2x_write(hw, MVPP22_BM_PHY_VIRT_HIGH_RLS_REG, 0x0);
		/*AXI Bridge Configuration */
		/* BM */
		mv_pp2x_write(hw, MVPP22_AXI_BM_WR_ATTR_REG,
			      MVPP22_AXI_ATTR_HW_COH_WRITE);
		mv_pp2x_write(hw, MVPP22_AXI_BM_RD_ATTR_REG,
			      MVPP22_AXI_ATTR_HW_COH_READ);

		/* Descriptors */
		mv_pp2x_write(hw, MVPP22_AXI_AGGRQ_DESCR_RD_ATTR_REG,
			      MVPP22_AXI_ATTR_HW_COH_READ);
		mv_pp2x_write(hw, MVPP22_AXI_TXQ_DESCR_WR_ATTR_REG,
			      MVPP22_AXI_ATTR_HW_COH_WRITE);
		mv_pp2x_write(hw, MVPP22_AXI_TXQ_DESCR_RD_ATTR_REG,
			      MVPP22_AXI_ATTR_HW_COH_READ);
		mv_pp2x_write(hw, MVPP22_AXI_RXQ_DESCR_WR_ATTR_REG,
			      MVPP22_AXI_ATTR_HW_COH_WRITE);

		/* Buffer Data */
		mv_pp2x_write(hw, MVPP22_AXI_TX_DATA_RD_ATTR_REG,
			      MVPP22_AXI_ATTR_HW_COH_READ);
		mv_pp2x_write(hw, MVPP22_AXI_RX_DATA_WR_ATTR_REG,
			      MVPP22_AXI_ATTR_HW_COH_WRITE);

		val = MVPP22_AXI_CODE_CACHE_NON_CACHE << MVPP22_AXI_CODE_CACHE_OFFS;
		val |= MVPP22_AXI_CODE_DOMAIN_SYSTEM << MVPP22_AXI_CODE_DOMAIN_OFFS;
		mv_pp2x_write(hw, MVPP22_AXI_RD_NORMAL_CODE_REG, val);
		mv_pp2x_write(hw, MVPP22_AXI_WR_NORMAL_CODE_REG, val);

		val = MVPP22_AXI_CODE_CACHE_RD_CACHE << MVPP22_AXI_CODE_CACHE_OFFS;
		val |= MVPP22_AXI_CODE_DOMAIN_OUTER_DOM << MVPP22_AXI_CODE_DOMAIN_OFFS;

		mv_pp2x_write(hw, MVPP22_AXI_RD_SNOOP_CODE_REG, val);

		val = MVPP22_AXI_CODE_CACHE_WR_CACHE << MVPP22_AXI_CODE_CACHE_OFFS;
		val |= MVPP22_AXI_CODE_DOMAIN_OUTER_DOM << MVPP22_AXI_CODE_DOMAIN_OFFS;

		mv_pp2x_write(hw, MVPP22_AXI_WR_SNOOP_CODE_REG, val);
	}

	/* Disable HW PHY polling */
	if (priv->pp2_version == PPV21) {
		val = readl(hw->lms_base + MVPP2_PHY_AN_CFG0_REG);
		val |= MVPP2_PHY_AN_STOP_SMI0_MASK;
		writel(val, hw->lms_base + MVPP2_PHY_AN_CFG0_REG);
		writel(MVPP2_EXT_GLOBAL_CTRL_DEFAULT,
		       hw->lms_base + MVPP2_MNG_EXTENDED_GLOBAL_CTRL_REG);
	}

	/* Allocate and initialize aggregated TXQs
	 * The aggr_txqs area should be aligned onto cache-line-size.
	 * So allocate cache-line-size more than needed, round-up the pointer
	 * but keep the offset between aligned and original pointers
	 * for further usage in free(aligned - offset).
	 * (offset is used instead of origin-ptr since it is more compact)
	 */
	val = sizeof(struct mv_pp2x_aggr_tx_queue) * mv_pp2x_used_addr_spaces +
		MVPP2_CACHE_LINE_SIZE;
	priv->aggr_txqs = devm_kcalloc(&pdev->dev, 1, val, GFP_KERNEL);
	if (!priv->aggr_txqs)
		return -ENOMEM;
	val = (dma_addr_t)priv->aggr_txqs & MVPP2_CACHE_LINE_MASK;
	if (!val) {
		priv->aggr_txqs_align_offs = 0;
	} else {
		priv->aggr_txqs_align_offs = sizeof(struct mv_pp2x_aggr_tx_queue) - val;
		priv->aggr_txqs = (void *)((u8 *)priv->aggr_txqs +
			priv->aggr_txqs_align_offs);
	}

	priv->num_aggr_qs = mv_pp2x_used_addr_spaces;

	i = 0;

	for (cpu = 0; cpu < mv_pp2x_used_addr_spaces; cpu++) {
		priv->aggr_txqs[i].id = cpu;
		priv->aggr_txqs[i].size = MVPP2_AGGR_TXQ_SIZE;

		err = mv_pp2x_aggr_txq_init(pdev, &priv->aggr_txqs[i],
					    MVPP2_AGGR_TXQ_SIZE, i, priv);
		if (err < 0)
			return err;
		i++;
	}

	/* Rx Fifo Init is done only in uboot */

	/* Set cache snoop when transmiting packets */
	mv_pp2x_write(hw, MVPP2_TX_SNOOP_REG, 0x1);

	/* Buffer Manager initialization */
	err = mv_pp2x_bm_init(pdev, priv);
	if (err < 0)
		return err;

	/* Parser flow id attribute tbl init */
	mv_pp2x_prs_flow_id_attr_init();

	/* Parser default initialization */
	err = mv_pp2x_prs_default_init(pdev, hw);
	if (err < 0)
		return err;

	/* Classifier default initialization */
	err = mv_pp2x_cls_init(pdev, hw);
	if (err < 0)
		return err;

	/* Classifier engine2 initialization */
	err = mv_pp2x_c2_init(pdev, hw);
	if (err < 0)
		return err;

	if (pp2_ver == PPV22) {
		for (i = 0; i < 128; i++) {
			val = mv_pp2x_read(hw, MVPP2_RXQ_CONFIG_REG(i));
			val |= MVPP2_RXQ_DISABLE_MASK;
			mv_pp2x_write(hw, MVPP2_RXQ_CONFIG_REG(i), val);
		}
	}

	return 0;
}

static struct mv_pp2x_platform_data pp21_pdata = {
	.pp2x_ver = PPV21,
	.pp2x_max_port_rxqs = 8,
	.mv_pp2x_rxq_short_pool_set = mv_pp21_rxq_short_pool_set,
	.mv_pp2x_rxq_long_pool_set = mv_pp21_rxq_long_pool_set,
	.multi_addr_space = false,
	.interrupt_tx_done = false,
	.multi_hw_instance = false,
	.mv_pp2x_port_queue_vectors_init = mv_pp21_port_queue_vectors_init,
	.mv_pp2x_port_isr_rx_group_cfg = mv_pp21x_port_isr_rx_group_cfg,
	.num_port_irq = 1,
	.hw.desc_queue_addr_shift = MVPP21_DESC_ADDR_SHIFT,
};

static struct mv_pp2x_platform_data pp22_pdata = {
	.pp2x_ver = PPV22,
	.pp2x_max_port_rxqs = 32,
	.mv_pp2x_rxq_short_pool_set = mv_pp22_rxq_short_pool_set,
	.mv_pp2x_rxq_long_pool_set = mv_pp22_rxq_long_pool_set,
	.multi_addr_space = true,
	.interrupt_tx_done = true,
	.multi_hw_instance = true,
	.mv_pp2x_port_queue_vectors_init = mv_pp22_queue_vectors_init,
	.mv_pp2x_port_isr_rx_group_cfg = mv_pp22_port_isr_rx_group_cfg,
	.num_port_irq = 9,
	.hw.desc_queue_addr_shift = MVPP22_DESC_ADDR_SHIFT,
};

static const struct of_device_id mv_pp2x_match_tbl[] = {
		{
			.compatible = "marvell,armada-375-pp2",
			.data = &pp21_pdata,
		},
		{
			.compatible = "marvell,mv-pp22",
			.data = &pp22_pdata,
		},
	{ }
};

static int mv_pp2x_init_config(struct mv_pp2x_param_config *pp2_cfg,
			       u32 cell_index)
{
	pp2_cfg->cell_index = cell_index;
	pp2_cfg->first_bm_pool = first_bm_pool;
	pp2_cfg->first_sw_thread = 0;
	pp2_cfg->first_log_rxq = first_log_rxq_queue;
	pp2_cfg->queue_mode = mv_pp2x_queue_mode;
	pp2_cfg->rx_cpu_map = port_cpu_bind_map;
	pp2_cfg->uc_filter_max = uc_filter_max;
	pp2_cfg->recycling = true;
	pp2_cfg->mc_filter_max = MVPP2_PRS_MAC_UC_MC_FILT_MAX - uc_filter_max;
	/* FIXME: Hardcoded no locks for MVPP2_QDIST_MULTI_MODE, to run 2APhase, need to change define.
	 * In 2APhase ethtool would be added.
	 * Add locks required during data path.
	 * In single queue mode aggregated queue should be locked if more than one AP's exist.
	 * Multi AP system has more than 8 CPU's, and mvpp22 HW has only 9 address spaces(9 physical aggregated queues)
	 * In multi queue mode aggregated queue and BM refill should be locked if rx_count less than number
	 * of address spaces - 1.
	 * Same as for physical aggregated queues, mvpp22 HW has only 9 copy's of buffer manager refill register.
	 * So in this case BM refill procedure should be protected.
	 * In single resource mode aggregated queue always should be locked, since single address spaces used by kernel.
	 * Singe thread deal with RX, so there is no need in BM lock.
	 */
	if (((num_active_cpus() > MVPP2_MAX_CPUS_IN_AP) && ((pp2_cfg->queue_mode == MVPP2_QDIST_SINGLE_MODE) ||
							    (mv_pp2x_rx_count > (MVPP2_MAX_ADDR_SPACES - 1)))) ||
							    (pp2_cfg->queue_mode == MVPP2_SINGLE_RESOURCE_MODE)) {
		pp2_cfg->spinlocks_bitmap |= MV_AGGR_QUEUE_LOCK;

		if (pp2_cfg->queue_mode == MVPP2_QDIST_MULTI_MODE) {
			pp2_cfg->spinlocks_bitmap |= MV_BM_LOCK;
			pp2_cfg->recycling = false;
		}
	}

	/* Calculate number of AP in system */
	pp2_cfg->num_of_ap = ((num_present_cpus() / MVPP2_MAX_CPUS_IN_AP) > 1) ?
						(num_present_cpus() / MVPP2_MAX_CPUS_IN_AP) : 1;

	return 0;
}

static void mv_pp22_init_rxfhindir(struct mv_pp2x *pp2)
{
	int i;
	int hot_cpus = num_online_cpus();

	if (hot_cpus > mv_pp2x_rx_count)
		hot_cpus = mv_pp2x_rx_count;

	for (i = 0; i < MVPP22_RSS_TBL_LINE_NUM; i++)
		pp2->rx_indir_table[i] = i % hot_cpus;
}

static int mv_pp2x_platform_data_get(struct platform_device *pdev,
				     struct mv_pp2x *priv,	u32 *cell_index, int *port_count)
{
	struct mv_pp2x_hw *hw = &priv->hw;
	struct mv_pp2x_uio *uio = &priv->uio;
	static bool cell_index_dts_flag;
	const struct of_device_id *match;
	struct device_node *dn = pdev->dev.of_node;
	struct resource *res;
	resource_size_t mspg_base, mspg_end;
	int	err;

	match = of_match_node(mv_pp2x_match_tbl, dn);
	if (!match)
		return -ENODEV;

	priv->pp2xdata = (struct mv_pp2x_platform_data *)match->data;

	if (of_property_read_u32(dn, "cell-index", cell_index)) {
		*cell_index = auto_cell_index;
		auto_cell_index++;
	} else {
		cell_index_dts_flag = true;
	}
	/* If cell_index is set by dts, and any previous device was set by
	 * auto_index, then fail.
	 */
	if (auto_cell_index && cell_index_dts_flag)
		return -ENXIO;
	/* PPV2 Address Space */
	if (priv->pp2xdata->pp2x_ver == PPV21) {
		res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
		hw->phys_addr_start = res->start;
		hw->phys_addr_end = res->end;
		hw->base = devm_ioremap_resource(&pdev->dev, res);
		if (IS_ERR(hw->base))
			return PTR_ERR(hw->base);
		res = platform_get_resource(pdev, IORESOURCE_MEM, 1);
		hw->lms_base = devm_ioremap_resource(&pdev->dev, res);
		if (IS_ERR(hw->lms_base))
			return PTR_ERR(hw->lms_base);
	} else {
		res = platform_get_resource_byname(pdev, IORESOURCE_MEM, "pp");
		hw->phys_addr_start = res->start;
		hw->phys_addr_end = res->end;
		hw->base = devm_ioremap_resource(&pdev->dev, res);
		if (IS_ERR(hw->base))
			return PTR_ERR(hw->base);
		/* Map "pp" to uio */
		err = mv_pp22_uio_mem_map(uio, res);
		if (err)
			return err;

		/* xmib */
		res = platform_get_resource_byname(pdev,
						   IORESOURCE_MEM, "xmib");
		hw->gop.gop_110.xmib.base =
			devm_ioremap_resource(&pdev->dev, res);
		if (IS_ERR(hw->gop.gop_110.xmib.base))
			return PTR_ERR(hw->gop.gop_110.xmib.base);
		hw->gop.gop_110.xmib.obj_size = 0x0100;

		/* skipped led */

		/* smi */
		res = platform_get_resource_byname(pdev,
						   IORESOURCE_MEM, "smi");
		hw->gop.gop_110.smi_base =
			devm_ioremap_resource(&pdev->dev, res);
		if (IS_ERR(hw->gop.gop_110.smi_base))
			return PTR_ERR(hw->gop.gop_110.smi_base);

		/* rfu1 */
		res = platform_get_resource_byname(pdev,
						   IORESOURCE_MEM, "rfu1");
		hw->gop.gop_110.rfu1_base =
			devm_ioremap_resource(&pdev->dev, res);
		if (IS_ERR(hw->gop.gop_110.rfu1_base))
			return PTR_ERR(hw->gop.gop_110.rfu1_base);

		/* skipped tai */

		/* xsmi  */
		res = platform_get_resource_byname(pdev,
						   IORESOURCE_MEM, "xsmi");
		hw->gop.gop_110.xsmi_base =
			devm_ioremap_resource(&pdev->dev, res);
		if (IS_ERR(hw->gop.gop_110.xsmi_base))
			return PTR_ERR(hw->gop.gop_110.xsmi_base);

		/* MSPG - base register */
		res = platform_get_resource_byname(pdev,
						   IORESOURCE_MEM, "mspg");
		hw->gop.gop_110.mspg_base =
			devm_ioremap_resource(&pdev->dev, res);
		if (IS_ERR(hw->gop.gop_110.mspg_base))
			return PTR_ERR(hw->gop.gop_110.mspg_base);
		mspg_base = res->start;
		mspg_end  = res->end;

		/* Map "mspg" to uio */
		err = mv_pp22_uio_mem_map(uio, res);
		if (err)
			return err;

		/* xpcs */
		res = platform_get_resource_byname(pdev,
						   IORESOURCE_MEM, "xpcs");
		if ((res->start <= mspg_base) || (res->end >= mspg_end))
			return -ENXIO;
		hw->gop.gop_110.xpcs_base =
			(void *)(hw->gop.gop_110.mspg_base +
				(res->start - mspg_base));

		hw->gop.gop_110.ptp.base =
			(void *)(hw->gop.gop_110.mspg_base + 0x0800);
		hw->gop.gop_110.ptp.obj_size = 0x1000;
		/* MSPG - gmac */
		res = platform_get_resource_byname(pdev,
						   IORESOURCE_MEM, "gmac");
		if ((res->start <= mspg_base) || (res->end >= mspg_end))
			return -ENXIO;
		hw->gop.gop_110.gmac.base =
			(void *)(hw->gop.gop_110.mspg_base +
			(res->start - mspg_base));
		hw->gop.gop_110.gmac.obj_size = 0x1000;

		/* FCA - flow control*/
		res = platform_get_resource_byname(pdev,
						   IORESOURCE_MEM, "fca");
		if ((res->start <= mspg_base) || (res->end >= mspg_end))
			return -ENXIO;
		hw->gop.gop_110.fca.base =
			(void *)(hw->gop.gop_110.mspg_base +
			(res->start - mspg_base));
		hw->gop.gop_110.fca.obj_size = 0x1000;

		/* MSPG - xlg */
		res = platform_get_resource_byname(pdev,
						   IORESOURCE_MEM, "xlg");
		if ((res->start <= mspg_base) || (res->end >= mspg_end))
			return -ENXIO;
		hw->gop.gop_110.xlg_mac.base =
			(void *)(hw->gop.gop_110.mspg_base +
			(res->start - mspg_base));
		hw->gop.gop_110.xlg_mac.obj_size = 0x1000;

		/* Jumbo L4_checksum port */
		if (of_property_read_u32(dn, "l4_chksum_jumbo_port",
					 &priv->l4_chksum_jumbo_port))
			/* Init as a invalid value */
			priv->l4_chksum_jumbo_port = MVPP2_MAX_PORTS;

		hw->gop_core_clk = devm_clk_get(&pdev->dev, "gop_core_clk");
		if (IS_ERR(hw->gop_core_clk))
			return PTR_ERR(hw->gop_core_clk);
		err = clk_prepare_enable(hw->gop_core_clk);
		if (err < 0)
			return err;

		hw->mg_core_clk = devm_clk_get(&pdev->dev, "mg_core_clk");
		if (IS_ERR(hw->mg_clk))
			return PTR_ERR(hw->mg_core_clk);
		err = clk_prepare_enable(hw->mg_core_clk);
		if (err < 0)
			return err;

		hw->mg_clk = devm_clk_get(&pdev->dev, "mg_clk");
		if (IS_ERR(hw->mg_clk))
			return PTR_ERR(hw->mg_clk);
		err = clk_prepare_enable(hw->mg_clk);
		if (err < 0)
			return err;
	}

	hw->gop_clk = devm_clk_get(&pdev->dev, "gop_clk");
	if (IS_ERR(hw->gop_clk))
		return PTR_ERR(hw->gop_clk);
	err = clk_prepare_enable(hw->gop_clk);
	if (err < 0)
		return err;

	hw->pp_clk = devm_clk_get(&pdev->dev, "pp_clk");
	if (IS_ERR(hw->pp_clk))
		return PTR_ERR(hw->pp_clk);
	err = clk_prepare_enable(hw->pp_clk);
	if (err < 0)
		return err;

	/* Get system's tclk rate */
	hw->tclk = clk_get_rate(hw->pp_clk);

	*port_count = of_get_available_child_count(dn);
	if (*port_count == 0) {
		dev_err(&pdev->dev, "no ports enabled\n");
		return -ENODEV;
	}
	return 0;
}

/* Initialize Rx FIFO's */
static void mv_pp22_rx_fifo_init(struct mv_pp2x *priv)
{
	int port;

	/* Port 0 maximum speed -10Gb/s port - required by spec RX FIFO size 32KB
	*   Port 1 maximum speed -2.5Gb/s port -required by spec RX FIFO size 8KB
	*   Port 2 maximum speed -1Gb/s port - required by spec RX FIFO size 4KB
	*   Port 3 LoopBack port -required by spec RX FIFO size 4KB
	*/
	for (port = 0; port < MVPP2_MAX_PORTS; port++) {
		if (port == 0) {
			mv_pp2x_write(&priv->hw, MVPP2_RX_DATA_FIFO_SIZE_REG(port),
				      MVPP2_RX_FIFO_DATA_SIZE_32KB);
			mv_pp2x_write(&priv->hw, MVPP2_RX_ATTR_FIFO_SIZE_REG(port),
				      MVPP2_RX_FIFO_ATTR_SIZE_32KB);
		} else if (port == 1) {
			mv_pp2x_write(&priv->hw, MVPP2_RX_DATA_FIFO_SIZE_REG(port),
				      MVPP2_RX_FIFO_DATA_SIZE_8KB);
			mv_pp2x_write(&priv->hw, MVPP2_RX_ATTR_FIFO_SIZE_REG(port),
				      MVPP2_RX_FIFO_ATTR_SIZE_8KB);
		} else {
			mv_pp2x_write(&priv->hw, MVPP2_RX_DATA_FIFO_SIZE_REG(port),
				      MVPP2_RX_FIFO_DATA_SIZE_4KB);
			mv_pp2x_write(&priv->hw, MVPP2_RX_ATTR_FIFO_SIZE_REG(port),
				      MVPP2_RX_FIFO_ATTR_SIZE_4KB);
		}
	}

	mv_pp2x_write(&priv->hw, MVPP2_RX_MIN_PKT_SIZE_REG,
		      MVPP2_RX_FIFO_PORT_MIN_PKT);
	mv_pp2x_write(&priv->hw, MVPP2_RX_FIFO_INIT_REG, 0x1);
}

/* Initialize Tx FIFO's */
static void mv_pp22_tx_fifo_init(struct mv_pp2x *priv)
{
	int i;

	/* Check l4_chksum_jumbo_port */
	if (priv->l4_chksum_jumbo_port < MVPP2_MAX_PORTS) {
		for (i = 0; i < priv->num_ports; i++) {
			if (priv->port_list[i]->id ==
				priv->l4_chksum_jumbo_port)
				break;
		}
		if (i == priv->num_ports)
			WARN(1, "Unavailable l4_chksum_jumbo_port %d\n",
			     priv->l4_chksum_jumbo_port);
	} else {
		/* Find port with highest speed, allocate extra FIFO to it */
		for (i = 0; i < priv->num_ports; i++) {
			phy_interface_t phy_mode =
					priv->port_list[i]->mac_data.phy_mode;

			if ((phy_mode == PHY_INTERFACE_MODE_XAUI) ||
			    (phy_mode == PHY_INTERFACE_MODE_RXAUI) ||
			    (phy_mode == PHY_INTERFACE_MODE_KR) ||
			    (phy_mode == PHY_INTERFACE_MODE_SFI) ||
			    (phy_mode == PHY_INTERFACE_MODE_XFI)) {
				/* Record l4_chksum_jumbo_port */
				priv->l4_chksum_jumbo_port =
							priv->port_list[i]->id;
				break;
			} else if (priv->port_list[i]->mac_data.speed == 2500 &&
				   (priv->l4_chksum_jumbo_port ==
							MVPP2_MAX_PORTS)) {
				/* Only first 2.5G port may get extra FIFO */
				priv->l4_chksum_jumbo_port =
							priv->port_list[i]->id;
			}
		}
		/* First 1G port in list get extra FIFO */
		if (priv->l4_chksum_jumbo_port == MVPP2_MAX_PORTS)
			priv->l4_chksum_jumbo_port = priv->port_list[0]->id;
	}

	/* Set FIFO according to l4_chksum_jumbo_port.
	*  10KB for l4_chksum_jumbo_port and 3KB for other ports.
	*  TX FIFO should be set for all ports, even if port not initialized.
	*/
	for (i = 0; i < MVPP2_MAX_PORTS; i++) {
		if (i != priv->l4_chksum_jumbo_port) {
			mv_pp2x_tx_fifo_size_set(&priv->hw, i,
						 MVPP2_TX_FIFO_DATA_SIZE_3KB);
			mv_pp2x_tx_fifo_threshold_set(&priv->hw, i,
						      MVPP2_TX_FIFO_THRESHOLD_3KB);
			}
	}
	mv_pp2x_tx_fifo_size_set(&priv->hw,
				 priv->l4_chksum_jumbo_port,
			    MVPP2_TX_FIFO_DATA_SIZE_10KB);
	mv_pp2x_tx_fifo_threshold_set(&priv->hw,
				      priv->l4_chksum_jumbo_port,
			    MVPP2_TX_FIFO_THRESHOLD_10KB);
}

/* Initialize Rx FIFO's */
static void mvpp21_rx_fifo_init(struct mv_pp2x *priv)
{
	int port;

	for (port = 0; port < MVPP2_MAX_PORTS; port++) {
		mv_pp2x_write(&priv->hw, MVPP2_RX_DATA_FIFO_SIZE_REG(port),
			      MVPP2_RX_FIFO_PORT_DATA_SIZE);
		mv_pp2x_write(&priv->hw, MVPP2_RX_ATTR_FIFO_SIZE_REG(port),
			      MVPP2_RX_FIFO_PORT_ATTR_SIZE);
	}

	mv_pp2x_write(&priv->hw, MVPP2_RX_MIN_PKT_SIZE_REG,
		      MVPP2_RX_FIFO_PORT_MIN_PKT);
	mv_pp2x_write(&priv->hw, MVPP2_RX_FIFO_INIT_REG, 0x1);
}

/* Initialize Tx FIFO's */
static void mvpp21_tx_fifo_init(struct mv_pp2x *priv)
{
	int val;

	/* Update TX FIFO MIN Threshold */
	val = mv_pp2x_read(&priv->hw, MVPP2_GMAC_PORT_FIFO_CFG_1_REG);
	val &= ~MVPP2_GMAC_TX_FIFO_MIN_TH_ALL_MASK;
	/* Min. TX threshold must be less than minimal packet length */
	val |= MVPP2_GMAC_TX_FIFO_MIN_TH_MASK(64 - 4 - 2);
	mv_pp2x_write(&priv->hw, MVPP2_GMAC_PORT_FIFO_CFG_1_REG, val);
}

static void mv_pp21_fifo_init(struct mv_pp2x *priv)
{
	mvpp21_rx_fifo_init(priv);
	mvpp21_tx_fifo_init(priv);
}

void mv_pp22_set_net_comp(struct mv_pp2x *priv)
{
	u32 net_comp_config;

	net_comp_config = mvp_pp2x_gop110_netc_cfg_create(priv);
	mv_gop110_netc_init(&priv->hw.gop, net_comp_config, MV_NETC_FIRST_PHASE);
	mv_gop110_netc_init(&priv->hw.gop, net_comp_config, MV_NETC_SECOND_PHASE);
}

/* Routine called by CP CPU hot plug notifier. Callback reconfigure RSS RX flow hash indir'n table */
static int mv_pp2x_cp_cpu_callback(struct notifier_block *nfb,
				   unsigned long action, void *hcpu)
{
	struct mv_pp2x *priv = container_of(nfb, struct mv_pp2x, cp_hotplug_nb);
	unsigned int cpu = (unsigned long)hcpu;
	int i;
	struct mv_pp2x_port *port;

	/* RSS rebalanced to equal */
	mv_pp22_init_rxfhindir(priv);

	switch (action) {
	case CPU_DEAD:
	case CPU_DEAD_FROZEN:
		/* If default CPU is down, CPU0 will be default CPU */
		for (i = 0; i < priv->num_ports; i++) {
			port = priv->port_list[i];
			if (port && (cpu == port->rss_cfg.dflt_cpu)) {
				port->rss_cfg.dflt_cpu = 0;
				if (port->rss_cfg.rss_en && netif_running(port->dev))
					mv_pp22_rss_default_cpu_set(port,
								    port->rss_cfg.dflt_cpu);
			}
		}
	}

	return NOTIFY_OK;
}

static int mv_pp2x_probe(struct platform_device *pdev)
{
	struct mv_pp2x *priv;
	struct mv_pp2x_hw *hw;
	int port_count = 0, cpu;
	int i, err;
	u32 cell_index = 0;
	struct device_node *dn = pdev->dev.of_node;
	struct device_node *port_node;
	struct mv_pp2x_cp_pcpu *cp_pcpu;
	bool probe_defer = false;
	phys_addr_t cma_addr;

	priv = devm_kzalloc(&pdev->dev, sizeof(struct mv_pp2x), GFP_KERNEL);
	if (!priv)
		return -ENOMEM;
	hw = &priv->hw;
	priv->pdev = pdev;

	err = mv_pp2x_platform_data_get(pdev, priv, &cell_index, &port_count);
	if (err) {
		if (err != -EPROBE_DEFER) {
			dev_err(&pdev->dev, "mvpp2: platform_data get failed\n");
		} else {
			/* Rollback auto_cell_index if it was used */
			if (auto_cell_index)
				auto_cell_index--;
			probe_defer = true;
		}
		goto err_clk;
	}
	priv->pp2_version = priv->pp2xdata->pp2x_ver;
	/* Retrieve dts defined cma-area (shared-dma-pool).
	 * Enables allocating cma_memory on DRAM that is physically close to the PPV2 device.
	 */
	of_reserved_mem_device_init(&pdev->dev);
	cma_addr = cma_get_base(dev_get_cma_area(&pdev->dev));
	dev_info(&pdev->dev, "cma_area base %pa size %ld MB\n", &cma_addr,
		 cma_get_size(dev_get_cma_area(&pdev->dev)) / SZ_1M);

	mv_pp2x_used_addr_spaces = (mv_pp2x_queue_mode == MVPP2_SINGLE_RESOURCE_MODE) ? 1 : MVPP2_MAX_ADDR_SPACES;

	/* DMA Configruation */
	if (priv->pp2_version == PPV22) {
		pdev->dev.dma_mask = kmalloc(sizeof(*pdev->dev.dma_mask), GFP_KERNEL);
		err = dma_set_mask(&pdev->dev, DMA_BIT_MASK(40));
		if (err == 0)
			dma_set_coherent_mask(&pdev->dev, DMA_BIT_MASK(40));
		if (err) {
			dev_err(&pdev->dev, "mvpp2: cannot set dma_mask\n");
			goto err_clk;
		}
	}

	/* Save cpu_present_mask + populate the per_cpu address space */
	for (i = 0; i < MVPP2_MAX_ADDR_SPACES; i++) {
		hw->cpu_base[i] = hw->base;
		if (priv->pp2xdata->multi_addr_space)
			hw->cpu_base[i] +=
				i * MVPP2_ADDR_SPACE_SIZE;
	}

	/* Init PP2 Configuration */
	err = mv_pp2x_init_config(&priv->pp2_cfg, cell_index);
	priv->hw.mv_pp2x_no_single_mode = (mv_pp2x_queue_mode == MVPP2_SINGLE_RESOURCE_MODE) ? 0 : 1;

	if (err < 0) {
		dev_err(&pdev->dev, "invalid driver parameters configured\n");
		goto err_clk;
	}

	/* Initialize network controller */
	err = mv_pp2x_init(pdev, priv);
	if (err < 0) {
		dev_err(&pdev->dev, "failed to initialize controller\n");
		goto err_clk;
	}

	priv->port_list = devm_kcalloc(&pdev->dev, port_count,
				      sizeof(struct mv_pp2x_port *),
				      GFP_KERNEL);
	if (!priv->port_list) {
		err = -ENOMEM;
		goto err_clk;
	}

	priv->pcpu = devm_kcalloc(&pdev->dev, MVPP2_MAX_ADDR_SPACES, sizeof(*priv->pcpu),
							GFP_KERNEL);
	if (!priv->pcpu) {
		err = -ENOMEM;
		goto  err_clk;
	}

	/* Init per CPU CP skb list for skb recycling */
	for (cpu = 0; cpu < mv_pp2x_used_addr_spaces; cpu++) {
		cp_pcpu = devm_kcalloc(&pdev->dev, 1, sizeof(*cp_pcpu), GFP_KERNEL);
		priv->pcpu[cpu] = cp_pcpu;

		INIT_LIST_HEAD(&cp_pcpu->skb_port_list);
		cp_pcpu->skb_pool = devm_kzalloc(&pdev->dev,
			sizeof(struct mv_pp2x_skb_pool), GFP_ATOMIC);
		cp_pcpu->skb_pool->skb_pool_size = MVPP2_SKB_NUM;
		cp_pcpu->skb_pool->skb_struct =
			devm_kzalloc(&pdev->dev,
				     sizeof(struct mv_pp2x_skb_struct) * MVPP2_SKB_NUM, GFP_ATOMIC);
	}

	/* Init PP22 rxfhindir table evenly in probe */
	if (priv->pp2_version == PPV22) {
		mv_pp22_init_rxfhindir(priv);
		if (mv_pp2x_queue_mode == MVPP2_QDIST_MULTI_MODE)
			priv->num_rss_tables = mv_pp2x_num_cos_queues;
		else
			priv->num_rss_tables = 0;
	}

	if (priv->pp2_version == PPV22) {
		priv->uio.u_info.name = kasprintf(GFP_KERNEL, UIO_PP2_STRING, cell_index);
		pr_debug("mv_pp2x_probe : %s\n", priv->uio.u_info.name);
		priv->uio.u_info.version = "0.1";
		err = uio_register_device(&pdev->dev, &priv->uio.u_info);
		if (err) {
			dev_err(&pdev->dev, "Failed to register uio device\n");
			goto err_clk;
		}
	}

	/* Initialize ports */
	for_each_available_child_of_node(dn, port_node) {
		err = mv_pp2x_port_probe(pdev, port_node, priv);
		if (err < 0)
			goto err_uio;
	}
	/* Init hrtimer for tx transmit procedure.
	 * Instead of reg_write atfer each xmit callback, 50 microsecond
	 * hrtimer would be started. Hrtimer will reduce amount of accesses
	 * to transmit register and dmb() influence on network performance.
	 */
	for (cpu = 0; cpu < mv_pp2x_used_addr_spaces; cpu++) {
		cp_pcpu = priv->pcpu[cpu];

		hrtimer_init(&cp_pcpu->tx_timer, CLOCK_MONOTONIC,
			     HRTIMER_MODE_REL_PINNED);
		cp_pcpu->tx_timer.function = mv_pp2x_tx_hr_timer_cb;
		cp_pcpu->tx_timer_scheduled = false;

		tasklet_init(&cp_pcpu->tx_tasklet,
			     mv_pp2x_tx_send_proc_cb, (unsigned long)priv);
	}

	if (priv->pp2_version == PPV22) {
		/* Init tx&rx fifo for each port */
		mv_pp22_tx_fifo_init(priv);
		mv_pp22_rx_fifo_init(priv);
		mv_pp22_set_net_comp(priv);
	} else {
		mv_pp21_fifo_init(priv);
	}

	platform_set_drvdata(pdev, priv);

	priv->workqueue = create_singlethread_workqueue("mv_pp2x");

	if (!priv->workqueue) {
		err = -ENOMEM;
		goto err_uio;
	}

	/* Only Mvpp22 support hot plug feature */
	if (priv->pp2_version == PPV22 && mv_pp2x_queue_mode == MVPP2_QDIST_MULTI_MODE) {
		priv->cp_hotplug_nb.notifier_call = mv_pp2x_cp_cpu_callback;
		register_hotcpu_notifier(&priv->cp_hotplug_nb);
	}
	INIT_DELAYED_WORK(&priv->stats_task, mv_pp2x_get_device_stats);

	queue_delayed_work(priv->workqueue, &priv->stats_task, stats_delay);
	pr_debug("Platform Device Name : %s\n", kobject_name(&pdev->dev.kobj));
	return 0;
err_uio:
	if (priv->pp2_version == PPV22)
		uio_unregister_device(&priv->uio.u_info);
err_clk:
	clk_disable_unprepare(hw->gop_clk);
	clk_disable_unprepare(hw->pp_clk);
	if (priv->pp2_version == PPV22)  {
		clk_disable_unprepare(hw->gop_core_clk);
		clk_disable_unprepare(hw->mg_clk);
		clk_disable_unprepare(hw->mg_core_clk);
		kfree(priv->uio.u_info.name);
	}
	if (probe_defer)
		devm_kfree(&pdev->dev, priv);
	return err;
}

static int mv_pp2x_remove(struct platform_device *pdev)
{
	struct mv_pp2x *priv = platform_get_drvdata(pdev);
	struct mv_pp2x_hw *hw = &priv->hw;
	int i, num_of_ports, num_of_pools;
	struct mv_pp2x_cp_pcpu *cp_pcpu;

	if (priv->pp2_version == PPV22 && mv_pp2x_queue_mode == MVPP2_QDIST_MULTI_MODE)
		unregister_hotcpu_notifier(&priv->cp_hotplug_nb);

	if (priv->pp2_version == PPV22) {
		uio_unregister_device(&priv->uio.u_info);
		kfree(priv->uio.u_info.name);
	}
	cancel_delayed_work(&priv->stats_task);
	flush_workqueue(priv->workqueue);
	destroy_workqueue(priv->workqueue);

	for (i = 0; i < mv_pp2x_used_addr_spaces; i++) {
		cp_pcpu = priv->pcpu[i];
		tasklet_kill(&cp_pcpu->tx_tasklet);
	}

	num_of_ports = priv->num_ports;

	for (i = 0; i < num_of_ports; i++) {
		if (priv->port_list[i])
			mv_pp2x_port_remove(priv->port_list[i]);
		priv->num_ports--;
	}

	num_of_pools = priv->num_pools;

	for (i = 0; i < num_of_pools; i++) {
		struct mv_pp2x_bm_pool *bm_pool = &priv->bm_pools[i];

		mv_pp2x_bm_pool_destroy(&pdev->dev, priv, bm_pool);
	}

	for (i = 0; i < mv_pp2x_used_addr_spaces; i++) {
		struct mv_pp2x_aggr_tx_queue *aggr_txq = &priv->aggr_txqs[i];

		dma_free_coherent(&pdev->dev,
				  MVPP2_DESCQ_MEM_SIZE(aggr_txq->size),
				  aggr_txq->desc_mem,
				  aggr_txq->descs_phys);
	}

	kfree(pdev->dev.dma_mask);
	pdev->dev.dma_mask = &pdev->dev.coherent_dma_mask;

	clk_disable_unprepare(hw->pp_clk);
	clk_disable_unprepare(hw->gop_clk);

	return 0;
}

#ifdef CONFIG_PM_SLEEP
/* Ports initialization after suspend */
static int mv_pp2x_port_resume(struct platform_device *pdev,
			       struct device_node *port_node,
			       struct mv_pp2x *priv, struct mv_pp2x_port *port)
{
	if (port->mac_data.phy_node)
		mv_pp2x_phy_connect(port);

	mv_pp2x_port_hw_init(port);

	return 0;
}

/* Routine configure mvpp2x HW after suspend */
static int mv_pp2x_probe_after_suspend(struct device *dev)
{
	struct mv_pp2x *priv = dev_get_drvdata(dev);
	int i, err;
	struct platform_device *pdev = priv->pdev;
	struct device_node *dn = pdev->dev.of_node;
	struct device_node *port_node;

	/* Resume network controller */
	err = mv_pp2x_init(pdev, priv);
	if (err < 0) {
		dev_err(&pdev->dev, "failed to resume controller\n");
		return err;
	}

	i = 0;
	/* Resume ports */
	for_each_available_child_of_node(dn, port_node) {
		mv_pp2x_port_resume(pdev, port_node, priv, priv->port_list[i]);
		i++;
	}

	if (priv->pp2_version == PPV22) {
		/* Init tx&rx fifo for each port */
		mv_pp22_tx_fifo_init(priv);
		mv_pp22_rx_fifo_init(priv);
		mv_pp22_set_net_comp(priv);
	} else {
		mv_pp21_fifo_init(priv);
	}

	return 0;
}

/* Routine suspend to RAM CP */
static int mvpp2x_suspend(struct device *dev)
{
	struct mv_pp2x *priv = dev_get_drvdata(dev);
	int i, num_of_ports, num_of_pools;
	struct platform_device *pdev = priv->pdev;

	num_of_ports = priv->num_ports;
	for (i = 0; i < num_of_ports; i++) {
		struct mv_mac_data *mac = &priv->port_list[i]->mac_data;
		/* Stop interface if port is up */
		if (netif_running(priv->port_list[i]->dev))
			mv_pp2x_stop(priv->port_list[i]->dev);

		if (mac->phy_node)
			mv_pp2x_phy_disconnect(priv->port_list[i]);

		/* Null all pointers to BM pools, pool's will be reconfigured in resume procedure */
		priv->port_list[i]->pool_short = NULL;
		priv->port_list[i]->pool_long = NULL;
		/* Missing flag will trigger GoP reconfiguration in resume routine */
		mac->flags &= ~MV_EMAC_F_INIT;
	}

	for (i = 0; i < mv_pp2x_used_addr_spaces; i++) {
		struct mv_pp2x_aggr_tx_queue *aggr_txq = &priv->aggr_txqs[i];

		dma_free_coherent(&pdev->dev,
				  MVPP2_DESCQ_MEM_SIZE(aggr_txq->size),
				  aggr_txq->desc_mem, aggr_txq->descs_phys);
	}

	devm_kfree(&pdev->dev, priv->hw.prs_shadow);
	devm_kfree(&pdev->dev, priv->hw.cls_shadow);
	devm_kfree(&pdev->dev, priv->hw.c2_shadow);
	/* aggr_txqs is aligned round-up; restore the original by -offset */
	devm_kfree(&pdev->dev, ((u8 *)priv->aggr_txqs - priv->aggr_txqs_align_offs));

	num_of_pools = priv->num_pools;
	for (i = 0; i < num_of_pools; i++) {
		struct mv_pp2x_bm_pool *bm_pool = &priv->bm_pools[i];

		mv_pp2x_bm_pool_destroy(dev, priv, bm_pool);
	}

	devm_kfree(&pdev->dev, priv->bm_pools);

	return 0;
}

/* Routine resume CP after S2RAM */
static int mvpp2x_resume(struct device *dev)
{
	struct mv_pp2x *priv;
	int i, num_of_ports, err;

	err = mv_pp2x_probe_after_suspend(dev);
	if (err < 0)
		return err;

	priv = dev_get_drvdata(dev);
	num_of_ports = priv->num_ports;

	for (i = 0; i < num_of_ports; i++) {
		if (priv->port_list[i]->comphy)
			phy_init(priv->port_list[i]->comphy);
		/* Start interface if port was up before suspend */
		if (netif_running(priv->port_list[i]->dev))
			mv_pp2x_open(priv->port_list[i]->dev);

		if (priv->port_list[i]->dev->flags & IFF_PROMISC)
			mv_pp2x_set_rx_promisc(priv->port_list[i]);
		else if (priv->port_list[i]->dev->flags & IFF_ALLMULTI)
			mv_pp2x_set_rx_allmulti(priv->port_list[i]);
	}

	return 0;
}

static const struct dev_pm_ops mv_pp2x_pm_ops = {
	.suspend = mvpp2x_suspend,
	.resume = mvpp2x_resume,
};
#endif /* CONFIG_PM_SLEEP */

MODULE_DEVICE_TABLE(of, mv_pp2x_match_tbl);

static struct platform_driver mv_pp2x_driver = {
	.probe = mv_pp2x_probe,
	.remove = mv_pp2x_remove,
	.driver = {
		.name = MVPP2_DRIVER_NAME,
		.of_match_table = mv_pp2x_match_tbl,
#ifdef CONFIG_PM_SLEEP
		.pm = &mv_pp2x_pm_ops,
#endif
	},
};

static int mv_pp2x_rxq_number_get(void)
{
	int rx_queue_num;

	if (mv_pp2x_queue_mode == MVPP2_QDIST_MULTI_MODE)
		if (mv_pp2x_num_cos_queues * num_active_cpus() > MVPP22_MAX_NUM_RXQ)
			rx_queue_num = MVPP22_MAX_NUM_RXQ;
		else
			rx_queue_num = mv_pp2x_num_cos_queues * mv_pp2x_rx_count;
	else
		rx_queue_num = mv_pp2x_num_cos_queues;

	return rx_queue_num;
}

static int __init mpp2_module_init(void)
{
	int ret = 0;

	mv_pp2x_rxq_number = mv_pp2x_rxq_number_get();
	mv_pp2x_txq_number = mv_pp2x_num_cos_queues;

	/* Compiler does not allow below Init in structure definition */
	mv_pp2x_pools[MVPP2_BM_SWF_SHORT_POOL].pkt_size =
		MVPP2_BM_SHORT_PKT_SIZE;
	mv_pp2x_pools[MVPP2_BM_SWF_LONG_POOL].pkt_size =
		MVPP2_BM_LONG_PKT_SIZE;
	mv_pp2x_pools[MVPP2_BM_SWF_JUMBO_POOL].pkt_size =
		MVPP2_BM_JUMBO_PKT_SIZE;

	ret = platform_driver_register(&mv_pp2x_driver);

	return ret;
}

static void __exit mpp2_module_exit(void)
{
	platform_driver_unregister(&mv_pp2x_driver);
}

module_init(mpp2_module_init);
module_exit(mpp2_module_exit);

MODULE_DESCRIPTION("Marvell PPv2x Ethernet Driver - www.marvell.com");
MODULE_AUTHOR("Marvell");
MODULE_LICENSE("GPL v2");
