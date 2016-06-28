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

#include <linux/phy.h>
#include <linux/clk.h>
#include <linux/hrtimer.h>
#include <linux/ktime.h>
#include <uapi/linux/ppp_defs.h>
#include <net/ip.h>
#include <net/ipv6.h>
#include <asm/cacheflush.h>
#include <linux/dma-mapping.h>

#include "mv_pp2x.h"
#include "mv_pp2x_hw.h"
#include "mv_gop110_hw.h"
#include "mv_pp2x_debug.h"

#if defined(CONFIG_NETMAP) || defined(CONFIG_NETMAP_MODULE)
#include <if_mv_pp2x_netmap.h>
#endif

#define MVPP2_SKB_TEST_SIZE 64
#define MVPP2_ADDRESS 0xf2000000
#define CPN110_ADDRESS_SPACE_SIZE (16*1024*1024)

/* Declaractions */
u8 mv_pp2x_num_cos_queues = 4;
static u8 mv_pp2x_queue_mode = MVPP2_QDIST_SINGLE_MODE;
static u8 rss_mode;
static u8 default_cpu;
static u8 cos_classifer;
static u32 pri_map = 0x3210; /* As default, cos0--rxq0, cos1--rxq1,
			      * cos2--rxq2, cos3--rxq3
			      */
static u8 default_cos = 3; /* As default, non-IP packet has the
			    * highest CoS value
			    */
static bool jumbo_pool;
static u16 rx_queue_size = MVPP2_MAX_RXD;
static u16 tx_queue_size = MVPP2_MAX_TXD;
static u16 buffer_scaling = 100;
static u32 port_cpu_bind_map;
static u8 first_bm_pool;
static u8 first_addr_space;
static u8 first_log_rxq_queue;

u32 debug_param;

#if defined(CONFIG_MV_PP2_POLLING)
#define MV_PP2_FPGA_PERODIC_TIME 100
#endif

#if defined(CONFIG_MV_PP2_POLLING)
struct timer_list cpu_poll_timer;
static int cpu_poll_timer_ref_cnt;
static void mv_pp22_cpu_timer_callback(unsigned long data);
#endif

module_param_named(num_cos_queues, mv_pp2x_num_cos_queues, byte, S_IRUGO);
MODULE_PARM_DESC(num_cos_queues, "Set number of cos_queues (1-8), def=4");

module_param_named(queue_mode, mv_pp2x_queue_mode, byte, S_IRUGO);
MODULE_PARM_DESC(queue_mode, "Set queue_mode (single=0, multi=1)");

module_param(rss_mode, byte, S_IRUGO);
MODULE_PARM_DESC(rss_mode, "Set rss_mode (UDP_2T=0, UDP_5T=1)");

module_param(default_cpu, byte, S_IRUGO);
MODULE_PARM_DESC(default_cpu, "Set default CPU for non RSS frames");

module_param(cos_classifer, byte, S_IRUGO);
MODULE_PARM_DESC(cos_classifer,
	"Cos Classifier (vlan_pri=0, dscp=1, vlan_dscp=2, dscp_vlan=3)");

module_param(pri_map, uint, S_IRUGO);
MODULE_PARM_DESC(pri_map, "Set priority_map, nibble for each cos.");

module_param(default_cos, byte, S_IRUGO);
MODULE_PARM_DESC(default_cos, "Set default cos (0-(num_cose_queues-1)).");

module_param(jumbo_pool, bool, S_IRUGO);
MODULE_PARM_DESC(jumbo_pool, "no_jumbo_support(0), jumbo_support(1)");

module_param(rx_queue_size, ushort, S_IRUGO);
MODULE_PARM_DESC(rx_queue_size, "Rx queue size");

module_param(tx_queue_size, ushort, S_IRUGO);
MODULE_PARM_DESC(tx_queue_size, "Tx queue size");

module_param(buffer_scaling, ushort, S_IRUGO);
MODULE_PARM_DESC(buffer_scaling, "Buffer scaling (TBD)");

module_param(debug_param, uint, S_IRUGO);
MODULE_PARM_DESC(debug_param,
	"Ad-hoc parameter, which can be used for various debug operations.");

/*TODO:  Below module_params will not go to ML. Created for testing. */

module_param(port_cpu_bind_map, uint, S_IRUGO);
MODULE_PARM_DESC(port_cpu_bind_map,
	"Set default port-to-cpu binding, nibble for each port. Relevant when queue_mode=multi-mode & rss is disabled");

module_param(first_bm_pool, byte, S_IRUGO);
MODULE_PARM_DESC(first_bm_pool, "First used buffer pool (0-11)");

module_param(first_addr_space, byte, S_IRUGO);
MODULE_PARM_DESC(first_addr_space, "First used PPV22 address space (0-8)");

module_param(first_log_rxq_queue, byte, S_IRUGO);
MODULE_PARM_DESC(first_log_rxq_queue, "First logical rx_queue (0-31)");

/* Number of RXQs used by single port */
static int mv_pp2x_rxq_number;
/* Number of TXQs used by single port */
static int mv_pp2x_txq_number;

struct mv_pp2x_pool_attributes mv_pp2x_pools[] = {
	{
		.description =  "short",
		/*.pkt_size    =	MVPP2_BM_SHORT_PKT_SIZE, */
		.buf_num     =  MVPP2_BM_SHORT_BUF_NUM,
	},
	{
		.description =  "long",
		/*.pkt_size    =	MVPP2_BM_LONG_PKT_SIZE,*/
		.buf_num     =  MVPP2_BM_LONG_BUF_NUM,
	},
	{
		.description =	"jumbo",
		/*.pkt_size    =	MVPP2_BM_JUMBO_PKT_SIZE, */
		.buf_num     =  MVPP2_BM_JUMBO_BUF_NUM,
	}
};

static inline int mv_pp2x_txq_count(struct mv_pp2x_txq_pcpu *txq_pcpu)
{

	int index_diff = txq_pcpu->txq_put_index - txq_pcpu->txq_get_index;

	return(index_diff % txq_pcpu->size);
}

static inline int mv_pp2x_txq_free_count(struct mv_pp2x_txq_pcpu *txq_pcpu)
{

	int index_diff = txq_pcpu->txq_get_index - txq_pcpu->txq_put_index;

	return(index_diff % txq_pcpu->size);
}

static void mv_pp2x_txq_inc_get(struct mv_pp2x_txq_pcpu *txq_pcpu)
{
	txq_pcpu->txq_get_index++;
	if (txq_pcpu->txq_get_index == txq_pcpu->size)
		txq_pcpu->txq_get_index = 0;
}

void mv_pp2x_txq_inc_put(enum mvppv2_version pp2_ver,
			 struct mv_pp2x_txq_pcpu *txq_pcpu,
			 struct sk_buff *skb,
			 struct mv_pp2x_tx_desc *tx_desc)
{
	txq_pcpu->tx_skb[txq_pcpu->txq_put_index] = skb;
	if (skb)
		txq_pcpu->tx_buffs[txq_pcpu->txq_put_index] =
				mv_pp2x_txdesc_phys_addr_get(pp2_ver, tx_desc);
	txq_pcpu->txq_put_index++;
	if (txq_pcpu->txq_put_index == txq_pcpu->size)
		txq_pcpu->txq_put_index = 0;
#if defined(__BIG_ENDIAN)
	if (pp2_ver == PPV21)
		mv_pp21_tx_desc_swap(tx_desc);
	else
		mv_pp22_tx_desc_swap(tx_desc);
#endif /* __BIG_ENDIAN */
}

static inline u8 mv_pp2x_first_pool_get(struct mv_pp2x *priv)
{
	return priv->pp2_cfg.first_bm_pool;
}

static inline u8 mv_pp2x_kernel_num_pools_get(struct mv_pp2x *priv)
{
	return((priv->pp2_cfg.jumbo_pool == true) ? 3 : 2);
}

static inline u8 mv_pp2x_last_pool_get(struct mv_pp2x *priv)
{
	return(mv_pp2x_first_pool_get(priv) + priv->num_pools);
}

static inline int mv_pp2x_pool_pkt_size_get(
		enum mv_pp2x_bm_pool_log_num  log_id)
{
	return mv_pp2x_pools[log_id].pkt_size;
}

static inline int mv_pp2x_pool_buf_num_get(
		enum mv_pp2x_bm_pool_log_num  log_id)
{
	return mv_pp2x_pools[log_id].buf_num;
}

static inline const char *
	mv_pp2x_pool_description_get(
		enum mv_pp2x_bm_pool_log_num  log_id)
{
	return mv_pp2x_pools[log_id].description;
}

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
			   u32 pool, int is_recycle)
{
	dma_addr_t phys_addr;
	void *data;

	if (is_recycle &&
	    (atomic_read(&bm_pool->in_use) < bm_pool->in_use_thresh))
		return 0;

	data = mv_pp2x_frag_alloc(bm_pool);
	if (!data)
		return -ENOMEM;
#ifdef CONFIG_64BIT
	if (unlikely(bm_pool->data_high != ((u64)data & 0xffffffff00000000)))
		return -ENOMEM;
#endif

	phys_addr = dma_map_single(port->dev->dev.parent, data,
				   MVPP2_RX_BUF_SIZE(bm_pool->pkt_size),
				   DMA_FROM_DEVICE);
	if (unlikely(dma_mapping_error(port->dev->dev.parent, phys_addr))) {
		mv_pp2x_frag_free(bm_pool, data);
		return -ENOMEM;
	}

	mv_pp2x_pool_refill(port->priv, pool, phys_addr, (u8 *)data);
	atomic_dec(&bm_pool->in_use);
	return 0;
}

/* Create pool */
static int mv_pp2x_bm_pool_create(struct device *dev,
					 struct mv_pp2x_hw *hw,
					 struct mv_pp2x_bm_pool *bm_pool,
					 int size)
{
	int size_bytes;

	/* Driver enforces size= x16 both for PPv21 and for PPv22, even though
	 *    PPv22 HW allows size= x8
	 */
	if (!IS_ALIGNED(size, (1<<MVPP21_BM_POOL_SIZE_OFFSET)))
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
	bm_pool->pkt_size = mv_pp2x_pool_pkt_size_get(bm_pool->log_id);
	bm_pool->frag_size = SKB_DATA_ALIGN(MVPP2_RX_BUF_SIZE(
				bm_pool->pkt_size)) + MVPP2_SKB_SHINFO_SIZE;
	bm_pool->buf_num = 0;
	mv_pp2x_bm_pool_bufsize_set(hw, bm_pool,
				    MVPP2_RX_BUF_SIZE(bm_pool->pkt_size));
	atomic_set(&bm_pool->in_use, 0);
#ifdef CONFIG_64BIT
{
	void *data_tmp;

	data_tmp = mv_pp2x_frag_alloc(bm_pool);
	if (data_tmp) {
		bm_pool->data_high = (u64)data_tmp & 0xffffffff00000000;
		mv_pp2x_frag_free(bm_pool, data_tmp);
	}
}
#endif

	return 0;
}

void mv_pp2x_bm_bufs_free(struct mv_pp2x *priv, struct mv_pp2x_bm_pool *bm_pool,
				int buf_num, bool is_skb)
{
	int i;

	if (buf_num > bm_pool->buf_num) {
		WARN(1, "Pool does not have so many bufs pool(%d) bufs(%d)\n",
		     bm_pool->id, buf_num);
		buf_num = bm_pool->buf_num;

	}
	for (i = 0; i < buf_num; i++) {
		struct sk_buff *vaddr;

		/* Get buffer virtual address (indirect access) */
		vaddr = mv_pp2x_bm_virt_addr_get(&priv->hw, bm_pool->id);
		if (!vaddr)
			break;
		if (is_skb) {
#ifdef CONFIG_64BIT
			dev_kfree_skb_any((struct sk_buff *)
				(priv->pp2xdata->skb_base_addr |
				(uintptr_t)vaddr));
#else
			dev_kfree_skb_any(vaddr);
#endif
		}
	}

	/* Update BM driver with number of buffers removed from pool */
	bm_pool->buf_num -= i;
}

/* Cleanup pool */
int mv_pp2x_bm_pool_destroy(struct device *dev, struct mv_pp2x *priv,
			   struct mv_pp2x_bm_pool *bm_pool, bool is_skb)
{
	u32 val;
	int size_bytes;

	mv_pp2x_bm_bufs_free(priv, bm_pool, bm_pool->buf_num, is_skb);
	if (bm_pool->buf_num) {
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

int mv_pp2x_bm_pool_add(struct device *dev, struct mv_pp2x *priv, u32 *pool_num,
	u32 pkt_size)
{
	int err, size, enabled;
	u8 first_pool = mv_pp2x_first_pool_get(priv);
	u32 pool = priv->num_pools;
	struct mv_pp2x_bm_pool *bm_pool;
	struct mv_pp2x_hw *hw = &priv->hw;

	if ((priv->num_pools + 1) >= MVPP2_BM_POOLS_MAX_ALLOC_NUM) {
		DBG_MSG("Unable to add pool. Max BM pool alloc reached %d\n",
			priv->num_pools + 1);
		return -ENOMEM;
	}

	/* Check if pool is already active. Ignore request */
	enabled = mv_pp2x_read(hw, MVPP2_BM_POOL_CTRL_REG(pool)) &
		MVPP2_BM_STATE_MASK;

	if (enabled) {
		DBG_MSG("%s pool %d already enabled. Ignoring request\n",
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
	mv_pp2x_pools[pool].pkt_size = pkt_size;
		err = mv_pp2x_bm_pool_create(dev, hw, bm_pool, size);
		if (err)
			return err;

	*pool_num = pool;
	priv->num_pools++;
	DBG_MSG("log_id %d, id %d, pool_num %d, num_pools %d\n",
		bm_pool->log_id, bm_pool->id, *pool_num, priv->num_pools);

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
		err = mv_pp2x_bm_pool_create(&pdev->dev, hw, bm_pool, size);
		if (err)
			goto err_unroll_pools;
	}
	priv->num_pools = num_pools;
	return 0;

err_unroll_pools:
	dev_err(&pdev->dev, "failed to create BM pool %d, size %d\n", i, size);
	for (i = i - 1; i >= 0; i--)
		mv_pp2x_bm_pool_destroy(&pdev->dev, priv, &priv->bm_pools[i],
					true);
		return err;
}

static int mv_pp2x_bm_init(struct platform_device *pdev, struct mv_pp2x *priv)
{
	int i, err;
	u8 first_pool = mv_pp2x_first_pool_get(priv);
	u8 num_pools = mv_pp2x_kernel_num_pools_get(priv);

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

	MVPP2_PRINT_VAR(buf_num);

	for (i = 0; i < buf_num; i++)
		mv_pp2x_rx_refill_new(port, bm_pool, (u32)bm_pool->id, 0);

	/* Update BM driver with number of buffers added to pool */
	bm_pool->buf_num += i;
	bm_pool->in_use_thresh = bm_pool->buf_num / 4;

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
	MVPP2_PRINT_VAR(pkts_num);
	} else {
		pkts_num = mv_pp2x_bm_buf_calc(log_pool,
					       pool->port_map &
					       ~(1 << port->id));
	}

	add_num = pkts_num - pool->buf_num;
	MVPP2_PRINT_VAR(add_num);

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
		mv_pp2x_bm_bufs_free(port->priv, pool, -add_num, true);
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
	struct mv_pp2x_hw *hw = &(port->priv->hw);

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
	struct mv_pp2x_hw *hw = &(port->priv->hw);

	if (!port->pool_long) {
		port->pool_long =
		       mv_pp2x_bm_pool_use(port, MVPP2_BM_SWF_LONG_POOL);
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
			mv_pp2x_bm_pool_use(port, MVPP2_BM_SWF_SHORT_POOL);
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
	struct mv_pp2x_bm_pool *old_port_pool = port->pool_long;
	struct mv_pp2x_hw *hw = &port->priv->hw;
	enum mv_pp2x_bm_pool_log_num new_log_pool;
	enum mv_pp2x_bm_pool_log_num old_log_pool = old_port_pool->log_id;
	int rxq;
	int pkt_size = MVPP2_RX_PKT_SIZE(mtu);

	if (pkt_size > MVPP2_BM_LONG_PKT_SIZE)
		new_log_pool = MVPP2_BM_SWF_JUMBO_POOL;
	else
		new_log_pool = MVPP2_BM_SWF_LONG_POOL;

	if (new_log_pool != old_log_pool) {
		/* Add port to new pool */
		port->pool_long = mv_pp2x_bm_pool_use(port, new_log_pool);
		if (!port->pool_long)
			return -ENOMEM;
		port->pool_long->port_map |= (1 << port->id);
		for (rxq = 0; rxq < port->num_rx_queues; rxq++)
			port->priv->pp2xdata->mv_pp2x_rxq_long_pool_set(hw,
			port->rxqs[rxq]->id, port->pool_long->id);

		/* Remove port from old pool */
		mv_pp2x_bm_pool_stop_use(port, old_log_pool);
		old_port_pool->port_map &= ~(1 << port->id);
	}

	dev->mtu = mtu;
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
		if (is_device_dma_coherent(port->dev->dev.parent)) {
			val |= MVPP2_SNOOP_PKT_SIZE_MASK;
			val |= MVPP2_SNOOP_BUF_HDR_MASK;
		} else {
			val &= ~MVPP2_SNOOP_PKT_SIZE_MASK;
			val &= ~MVPP2_SNOOP_BUF_HDR_MASK;
		}
		mv_pp2x_write(hw, MVPP2_RXQ_CONFIG_REG(queue), val);
	}

	/* At default, mask all interrupts to all present cpus */
	mv_pp2x_port_interrupts_disable(port);
}

/* Check if there are enough reserved descriptors for transmission.
 * If not, request chunk of reserved descriptors and check again.
 */
int mv_pp2x_txq_reserved_desc_num_proc(
					struct mv_pp2x *priv,
					struct mv_pp2x_tx_queue *txq,
					struct mv_pp2x_txq_pcpu *txq_pcpu,
					int num)
{
	int req, cpu, desc_count;

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

	if ((num - txq_pcpu->reserved_num) > req) {
		req = num - txq_pcpu->reserved_num;
		desc_count = 0;
		/* Compute total of used descriptors */
		for_each_online_cpu(cpu) {
			int txq_count;
			struct mv_pp2x_txq_pcpu *txq_pcpu_aux;

			txq_pcpu_aux = per_cpu_ptr(txq->pcpu, cpu);
			txq_count = mv_pp2x_txq_count(txq_pcpu_aux);
			desc_count += txq_count;

		}
		desc_count += req;

		if (desc_count > txq->size)
			return -ENOMEM;
	}

	txq_pcpu->reserved_num += mv_pp2x_txq_alloc_reserved_desc(priv, txq,
								  req);

	/* OK, the descriptor cound has been updated: check again. */
	if (txq_pcpu->reserved_num < num)
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
		struct sk_buff *skb = txq_pcpu->tx_skb[txq_pcpu->txq_get_index];

		mv_pp2x_txq_inc_get(txq_pcpu);

		if (!skb)
			continue;

		dma_unmap_single(port->dev->dev.parent, buf_phys_addr,
				 skb_headlen(skb), DMA_TO_DEVICE);
		dev_kfree_skb_any(skb);
	}
}

/* Handle end of transmission */
static void mv_pp2x_txq_done(struct mv_pp2x_port *port,
				   struct mv_pp2x_tx_queue *txq,
				   struct mv_pp2x_txq_pcpu *txq_pcpu)
{
	struct netdev_queue *nq = netdev_get_tx_queue(port->dev, txq->log_id);
	int tx_done;


#ifdef DEV_NETMAP
	if (port->flags & MVPP2_F_IFCAP_NETMAP) {
		if (netmap_tx_irq(port->dev, 0))
			return; /* cleaned ok */
	}
#endif /* DEV_NETMAP */

	tx_done = mv_pp2x_txq_sent_desc_proc(port,
					     QV_CPU_2_THR(txq_pcpu->cpu),
					     txq->id);
	if (!tx_done)
		return;

	mv_pp2x_txq_bufs_free(port, txq_pcpu, tx_done);

		if (netif_tx_queue_stopped(nq))
		if (mv_pp2x_txq_free_count(txq_pcpu) >= (MAX_SKB_FRAGS + 2))
			netif_tx_wake_queue(nq);
}

static unsigned int mv_pp2x_tx_done(struct mv_pp2x_port *port, u32 cause,
					    int cpu)
{
	struct mv_pp2x_tx_queue *txq;
	struct mv_pp2x_txq_pcpu *txq_pcpu;
	unsigned int tx_todo = 0;

	while (cause) {
		int txq_count;

		txq = mv_pp2x_get_tx_queue(port, cause);
		if (!txq)
			break;

		txq_pcpu = per_cpu_ptr(txq->pcpu, cpu);
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
	aggr_txq->desc_mem = dma_alloc_coherent(&pdev->dev,
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
	mv_pp2x_rx_pkts_coal_set(port, rxq, rxq->pkts_coal);
	mv_pp2x_rx_time_coal_set(port, rxq, rxq->time_coal);

	/* Add number of descriptors ready for receiving packets */
	mv_pp2x_rxq_status_update(port, rxq->id, 0, rxq->size);

	return 0;
}

/* Push packets received by the RXQ to BM pool */
static void mv_pp2x_rxq_drop_pkts(struct mv_pp2x_port *port,
					struct mv_pp2x_rx_queue *rxq)
{
	int rx_received, i;
	u8 *buf_cookie;
	dma_addr_t buf_phys_addr;

	rx_received = mv_pp2x_rxq_received(port, rxq->id);
	if (!rx_received)
		return;

	for (i = 0; i < rx_received; i++) {
		struct mv_pp2x_rx_desc *rx_desc =
			mv_pp2x_rxq_next_desc_get(rxq);

		if (port->priv->pp2_version == PPV21) {
			buf_cookie = mv_pp21_rxdesc_cookie_get(rx_desc);
			buf_phys_addr = mv_pp21_rxdesc_phys_addr_get(rx_desc);
		} else {
			buf_cookie = mv_pp22_rxdesc_cookie_get(rx_desc);
			buf_phys_addr = mv_pp22_rxdesc_phys_addr_get(rx_desc);
		}
		mv_pp2x_pool_refill(port->priv, MVPP2_RX_DESC_POOL(rx_desc),
			buf_phys_addr, buf_cookie);
	}
	mv_pp2x_rxq_status_update(port, rxq->id, rx_received, rx_received);
}

/* Cleanup Rx queue */
static void mv_pp2x_rxq_deinit(struct mv_pp2x_port *port,
				   struct mv_pp2x_rx_queue *rxq)
{
	struct mv_pp2x_hw *hw = &(port->priv->hw);

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
	struct mv_pp2x_hw *hw = &(port->priv->hw);
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
		      MVPP2_PREF_BUF_THRESH(desc_per_txq/2));

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

	for_each_online_cpu(cpu) {
		txq_pcpu = per_cpu_ptr(txq->pcpu, cpu);
		txq_pcpu->size = txq->size;
		txq_pcpu->tx_skb = kmalloc(txq_pcpu->size *
					   sizeof(*txq_pcpu->tx_skb),
					   GFP_KERNEL);
		if (!txq_pcpu->tx_skb)
			goto error;

		txq_pcpu->tx_buffs = kmalloc(txq_pcpu->size *
					     sizeof(dma_addr_t), GFP_KERNEL);
		if (!txq_pcpu->tx_buffs)
			goto error;

		txq_pcpu->count = 0;
		txq_pcpu->reserved_num = 0;
		txq_pcpu->txq_put_index = 0;
		txq_pcpu->txq_get_index = 0;
	}

	return 0;

error:
	for_each_online_cpu(cpu) {
		txq_pcpu = per_cpu_ptr(txq->pcpu, cpu);
		kfree(txq_pcpu->tx_skb);
		kfree(txq_pcpu->tx_buffs);
	}

	dma_free_coherent(port->dev->dev.parent,
			  txq->size * MVPP2_DESC_ALIGNED_SIZE,
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

	for_each_online_cpu(cpu) {
		txq_pcpu = per_cpu_ptr(txq->pcpu, cpu);
		kfree(txq_pcpu->tx_skb);
		kfree(txq_pcpu->tx_buffs);
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

	for_each_online_cpu(cpu) {
		int txq_count;

		txq_pcpu = per_cpu_ptr(txq->pcpu, cpu);

		/* Release all packets */
		txq_count = mv_pp2x_txq_count(txq_pcpu);
		mv_pp2x_txq_bufs_free(port, txq_pcpu, txq_count);

		/* Reset queue */
		txq_pcpu->txq_put_index = 0;
		txq_pcpu->txq_get_index = 0;
	}
}

/* Cleanup all Tx queues */
void mv_pp2x_cleanup_txqs(struct mv_pp2x_port *port)
{
	struct mv_pp2x_tx_queue *txq;
	int queue;
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
	int queue, err;

	for (queue = 0; queue < port->num_tx_queues; queue++) {
		txq = port->txqs[queue];
		err = mv_pp2x_txq_init(port, txq);
		if (err)
			goto err_cleanup;
	}
	if (port->priv->pp2xdata->interrupt_tx_done == true) {
		mv_pp2x_tx_done_time_coal_set(port, port->tx_time_coal);
		on_each_cpu(mv_pp2x_tx_done_pkts_coal_set, port, 1);
	}
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
	free_irq(port->mac_data.link_irq, port);
}

/* The callback for per-q_vector interrupt */
static irqreturn_t mv_pp2x_isr(int irq, void *dev_id)
{
	struct queue_vector *q_vec = (struct queue_vector *)dev_id;

	mv_pp2x_qvector_interrupt_disable(q_vec);
	pr_debug("%s cpu_id(%d) port_id(%d) q_vec(%d), qv_type(%d)\n",
		__func__, smp_processor_id(), q_vec->parent->id,
		(int)(q_vec-q_vec->parent->q_vector), q_vec->qv_type);
	napi_schedule(&q_vec->napi);

	return IRQ_HANDLED;
}

static irqreturn_t mv_pp2_link_change_isr(int irq, void *data)
{
	struct mv_pp2x_port *port = (struct mv_pp2x_port *)data;

	pr_debug("%s cpu_id(%d) irq(%d) pp_port(%d)\n", __func__,
		smp_processor_id(), irq, port->id);
	/* mask all events from this mac */
	mv_gop110_port_events_mask(&port->priv->hw.gop, &port->mac_data);
	/* read cause register to clear event */
	mv_gop110_port_events_clear(&port->priv->hw.gop, &port->mac_data);

	tasklet_schedule(&port->link_change_tasklet);

	return IRQ_HANDLED;

}

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
			cpu = QV_THR_2_CPU(qvec->sw_thread_id);
			irq_set_affinity_hint(qvec->irq, cpumask_of(cpu));
			if (port->priv->pp2_cfg.queue_mode ==
				MVPP2_QDIST_MULTI_MODE)
				irq_set_status_flags(qvec->irq,
						     IRQ_NO_BALANCING);
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

	/* Check Link status on ethernet port */
	link_is_up = mv_gop110_port_is_link_up(gop, &port->mac_data);


	if (link_is_up) {
		if (netif_carrier_ok(dev))
			return;

		netif_carrier_on(dev);
		netif_tx_wake_all_queues(dev);
		pr_info("%s: link up\n", dev->name);
		port->mac_data.flags |= MV_EMAC_F_LINK_UP;
	} else {
		if (!netif_carrier_ok(dev))
			return;
		netif_carrier_off(dev);
		netif_tx_stop_all_queues(dev);
		pr_info("%s: link down\n", dev->name);
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
		phy_print_status(phydev);
	}
}

void mv_pp2_link_change_tasklet(unsigned long data)
{
	struct net_device *dev = (struct net_device *)data;
#if !defined(CONFIG_MV_PP2_POLLING)
	struct mv_pp2x_port *port = netdev_priv(dev);
#endif

	mv_pp22_dev_link_event(dev);

#if !defined(CONFIG_MV_PP2_POLLING)
	/* Unmask interrupt */
	mv_gop110_port_events_unmask(&port->priv->hw.gop, &port->mac_data);
#endif
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

static void mv_pp2x_tx_proc_cb(unsigned long data)
{
	struct net_device *dev = (struct net_device *)data;
	struct mv_pp2x_port *port = netdev_priv(dev);
	struct mv_pp2x_port_pcpu *port_pcpu = this_cpu_ptr(port->pcpu);
	unsigned int tx_todo, cause;

	if (!netif_running(dev))
		return;
	port_pcpu->timer_scheduled = false;

	/* Process all the Tx queues */
	cause = (1 << mv_pp2x_txq_number) - 1;
	tx_todo = mv_pp2x_tx_done(port, cause, smp_processor_id());

	/* Set the timer in case not all the packets were processed */
	if (tx_todo)
		mv_pp2x_timer_set(port_pcpu);
}

static enum hrtimer_restart mv_pp2x_hr_timer_cb(struct hrtimer *timer)
{
	struct mv_pp2x_port_pcpu *port_pcpu = container_of(timer,
			 struct mv_pp2x_port_pcpu, tx_done_timer);

	tasklet_schedule(&port_pcpu->tx_done_tasklet);

	return HRTIMER_NORESTART;
}

/* The function get the number of cpu online */
static inline int mv_pp2x_num_online_cpu_get(struct mv_pp2x *pp2)
{
	u8 num_online_cpus = 0;
	u16 x = pp2->cpu_map;

	while (x) {
		x &= (x - 1);
		num_online_cpus++;
	}

	return num_online_cpus;
}

/* The function calculate the width, such as cpu width, cos queue width */
static inline void mv_pp2x_width_calc(struct mv_pp2x *pp2, u32 *cpu_width,
				u32 *cos_width, u32 *port_rxq_width)
{
	if (pp2) {
		/* Calculate CPU width */
		if (cpu_width)
			*cpu_width = ilog2(roundup_pow_of_two(
				mv_pp2x_num_online_cpu_get(pp2)));
		/* Calculate cos queue width */
		if (cos_width)
			*cos_width = ilog2(roundup_pow_of_two(
				pp2->pp2_cfg.cos_cfg.num_cos_queues));
		/* Calculate rx queue width on the port */
		if (port_rxq_width)
			*port_rxq_width = ilog2(roundup_pow_of_two(
				pp2->pp2xdata->pp2x_max_port_rxqs));
	}
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
	int data[3];
	struct mv_pp2x_hw *hw = &(port->priv->hw);
	struct mv_pp2x_cls_flow_info *flow_info;

	for (index = 0; index < (MVPP2_PRS_FL_LAST - MVPP2_PRS_FL_START);
		index++) {
		flow_info = &(hw->cls_shadow->flow_info[index]);
		data[0] = MVPP2_FLOW_TBL_SIZE;
		data[1] = MVPP2_FLOW_TBL_SIZE;
		data[2] = MVPP2_FLOW_TBL_SIZE;
		lkpid = index + MVPP2_PRS_FL_START;
		/* Prepare a temp table for the lkpid */
		mv_pp2x_cls_flow_tbl_temp_copy(hw, lkpid, &flow_idx);
		/* Update lookup table to temp flow table */
		mv_pp2x_cls_lkp_flow_set(hw, lkpid, 0, flow_idx);
		mv_pp2x_cls_lkp_flow_set(hw, lkpid, 1, flow_idx);
		/* Update original flow table */
		/* First, remove the port from original table */
		if (flow_info->flow_entry_dflt) {
			mv_pp2x_cls_flow_port_del(hw,
			flow_info->flow_entry_dflt,
			port->id);
			data[0] =
			flow_info->flow_entry_dflt;
		}
		if (flow_info->flow_entry_vlan) {
			mv_pp2x_cls_flow_port_del(hw,
				flow_info->flow_entry_vlan, port->id);
			data[1] = flow_info->flow_entry_vlan;
		}
		if (flow_info->flow_entry_dscp) {
			mv_pp2x_cls_flow_port_del(hw,
				flow_info->flow_entry_dscp, port->id);
			data[2] = flow_info->flow_entry_dscp;
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
		flow_idx = min(data[0], min(data[1], data[2]));
		mv_pp2x_cls_lkp_flow_set(hw, lkpid, 0, flow_idx);
		mv_pp2x_cls_lkp_flow_set(hw, lkpid, 1, flow_idx);
	}

	/* Update it in priv */
	port->priv->pp2_cfg.cos_cfg.cos_classifier = cos_mode;

	return 0;
}

/* mv_pp2x_cos_classifier_get
*  -- Get the cos classifier on the port.
*/
int mv_pp2x_cos_classifier_get(struct mv_pp2x_port *port)
{
	return port->priv->pp2_cfg.cos_cfg.cos_classifier;
}

/* mv_pp2x_cos_pri_map_set
*  -- Set priority_map per port, nibble for each cos value(0~7).
*/
int mv_pp2x_cos_pri_map_set(struct mv_pp2x_port *port, int cos_pri_map)
{
	int ret, prev_pri_map;
	u8 bound_cpu_first_rxq;


	if (port->priv->pp2_cfg.cos_cfg.pri_map == cos_pri_map)
		return 0;

	prev_pri_map = port->priv->pp2_cfg.cos_cfg.pri_map;
	port->priv->pp2_cfg.cos_cfg.pri_map = cos_pri_map;


	/* Update C2 rules with nre pri_map */
	bound_cpu_first_rxq  = mv_pp2x_bound_cpu_first_rxq_calc(port);
	ret = mv_pp2x_cls_c2_rule_set(port, bound_cpu_first_rxq);
	if (ret) {
		port->priv->pp2_cfg.cos_cfg.pri_map = prev_pri_map;
		return ret;
	}

	return 0;
}

/* mv_pp2x_cos_pri_map_get
*  -- Get priority_map on the port.
*/
int mv_pp2x_cos_pri_map_get(struct mv_pp2x_port *port)
{
	return port->priv->pp2_cfg.cos_cfg.pri_map;
}

/* mv_pp2x_cos_default_value_set
*  -- Set default cos value for untagged or non-IP packets per port.
*/
int mv_pp2x_cos_default_value_set(struct mv_pp2x_port *port, int cos_value)
{
	int ret, prev_cos_value;
	u8 bound_cpu_first_rxq;

	if (port->priv->pp2_cfg.cos_cfg.default_cos == cos_value)
		return 0;

	prev_cos_value = port->priv->pp2_cfg.cos_cfg.default_cos;
	port->priv->pp2_cfg.cos_cfg.default_cos = cos_value;

	/* Update C2 rules with the pri_map */
	bound_cpu_first_rxq  = mv_pp2x_bound_cpu_first_rxq_calc(port);
	ret = mv_pp2x_cls_c2_rule_set(port, bound_cpu_first_rxq);
	if (ret) {
		port->priv->pp2_cfg.cos_cfg.default_cos = prev_cos_value;
		return ret;
	}

	return 0;
}

/* mv_pp2x_cos_default_value_get
*  -- Get default cos value for untagged or non-IP packets on the port.
*/
int mv_pp2x_cos_default_value_get(struct mv_pp2x_port *port)
{
	return port->priv->pp2_cfg.cos_cfg.default_cos;
}

/* RSS API */

/* Translate CPU sequence number to real CPU ID */
static inline int mv_pp22_cpu_id_from_indir_tbl_get(struct mv_pp2x *pp2,
					int cpu_seq, u32 *cpu_id)
{
	int i;
	int seq = 0;

	if (!pp2 || !cpu_id || cpu_seq >= 16)
		return -EINVAL;

	for (i = 0; i < 16; i++) {
		if (pp2->cpu_map & (1 << i)) {
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
	int rss_tbl_needed = port->priv->pp2_cfg.cos_cfg.num_cos_queues;

	if (port->priv->pp2_cfg.queue_mode == MVPP2_QDIST_SINGLE_MODE)
		return -1;

	memset(&rss_entry, 0, sizeof(struct mv_pp22_rss_entry));

	if (!port->priv->cpu_map)
		return -1;

	/* Calculate cpu and cos width */
	mv_pp2x_width_calc(port->priv, &cpu_width, &cos_width, NULL);

	rss_entry.u.entry.width = cos_width + cpu_width;

	rss_entry.sel = MVPP22_RSS_ACCESS_TBL;

	for (rss_tbl = 0; rss_tbl < rss_tbl_needed; rss_tbl++) {
		for (entry_idx = 0; entry_idx < MVPP22_RSS_TBL_LINE_NUM;
			entry_idx++) {
			rss_entry.u.entry.tbl_id = rss_tbl;
			rss_entry.u.entry.tbl_line = entry_idx;
			if (mv_pp22_cpu_id_from_indir_tbl_get(port->priv,
			     port->priv->rx_indir_table[entry_idx],
			     &cpu_id))
				return -1;
			/* Value of rss_tbl equals to cos queue */
			rss_entry.u.entry.rxq = (cpu_id << cos_width) |
				rss_tbl;
			if (mv_pp22_rss_tbl_entry_set(
				&port->priv->hw, &rss_entry))
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


	if (port->priv->pp2_cfg.rss_cfg.rss_en == en)
		return;

	bound_cpu_first_rxq  = mv_pp2x_bound_cpu_first_rxq_calc(port);

	if (port->priv->pp2_cfg.queue_mode == MVPP2_QDIST_MULTI_MODE) {
		mv_pp22_rss_c2_enable(port, en);
		if (en) {
			if (mv_pp22_rss_default_cpu_set(port,
				port->priv->pp2_cfg.rss_cfg.dflt_cpu))
				netdev_err(port->dev,
				"cannot set rss default cpu on port(%d)\n",
				port->id);
			else
				port->priv->pp2_cfg.rss_cfg.rss_en = 1;
		} else {
			if (mv_pp2x_cls_c2_rule_set(port, bound_cpu_first_rxq))
				netdev_err(port->dev,
				"cannot set c2 and qos table on port(%d)\n",
				port->id);
			else
				port->priv->pp2_cfg.rss_cfg.rss_en = 0;
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
	struct mv_pp2x_hw *hw = &(port->priv->hw);
	struct mv_pp2x_cls_flow_info *flow_info;

	if (port->priv->pp2_cfg.queue_mode == MVPP2_QDIST_SINGLE_MODE)
		return -1;

	for (index = 0; index < (MVPP2_PRS_FL_LAST - MVPP2_PRS_FL_START);
		index++) {
		flow_info = &(hw->cls_shadow->flow_info[index]);
		data[0] = MVPP2_FLOW_TBL_SIZE;
		data[1] = MVPP2_FLOW_TBL_SIZE;
		data[2] = MVPP2_FLOW_TBL_SIZE;
		lkpid = index + MVPP2_PRS_FL_START;
		/* Get lookup ID attribute */
		lkpid_attr = mv_pp2x_prs_flow_id_attr_get(lkpid);
		/* Only non-frag UDP can set rss mode */
		if ((lkpid_attr & MVPP2_PRS_FL_ATTR_UDP_BIT) &&
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
			if (rss_mode == MVPP2_RSS_NF_UDP_2T)
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
	port->priv->pp2_cfg.rss_cfg.rss_mode = rss_mode;

	return 0;
}

/* mv_pp22_rss_default_cpu_set
*  -- The API to update the default CPU to handle the non-IP packets.
*/
int mv_pp22_rss_default_cpu_set(struct mv_pp2x_port *port, int default_cpu)
{
	u8 index, queue, q_cpu_mask;
	u32 cpu_width = 0, cos_width = 0;
	struct mv_pp2x_hw *hw = &(port->priv->hw);

	if (port->priv->pp2_cfg.queue_mode == MVPP2_QDIST_SINGLE_MODE)
		return -1;

	/* Calculate width */
	mv_pp2x_width_calc(port->priv, &cpu_width, &cos_width, NULL);
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
	port->priv->pp2_cfg.rss_cfg.dflt_cpu = default_cpu;

	return 0;
}

/* Main RX/TX processing routines */
#if 0

/* Reuse skb if possible, or allocate a new skb and add it to BM pool */
static int mv_pp2x_rx_refill(struct mv_pp2x_port *port,
			   struct mv_pp2x_bm_pool *bm_pool,
			   u32 pool, int is_recycle)
{
	struct sk_buff *skb;
	dma_addr_t phys_addr;

	if (is_recycle &&
	    (atomic_read(&bm_pool->in_use) < bm_pool->in_use_thresh))
		return 0;

	/* No recycle or too many buffers are in use, so allocate a new skb */
	skb = mv_pp2x_skb_alloc(port, bm_pool, &phys_addr, GFP_ATOMIC);
	if (!skb)
		return -ENOMEM;

	mv_pp2x_pool_refill(port->priv, pool, phys_addr, skb);
	atomic_dec(&bm_pool->in_use);
	return 0;
}
#endif

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
			      struct mv_pp2x_rx_desc *rx_desc)
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
				     buff_virt_addr, mc_id);

		buff_phys_addr = buff_phys_addr_next;
		buff_virt_addr = buff_virt_addr_next;

	} while (!MVPP2_B_HDR_INFO_IS_LAST(buff_hdr->info));
}

/* Main rx processing */
static int mv_pp2x_rx(struct mv_pp2x_port *port, struct napi_struct *napi,
			int rx_todo, struct mv_pp2x_rx_queue *rxq)
{
	struct net_device *dev = port->dev;
	int rx_received, rx_filled, i;
	u32 rcvd_pkts = 0;
	u32 rcvd_bytes = 0;
	u32 refill_array[MVPP2_BM_POOLS_NUM] = {0};
	u8  first_bm_pool = port->priv->pp2_cfg.first_bm_pool;
	u8  num_pool = mv_pp2x_kernel_num_pools_get(port->priv);

#ifdef DEV_NETMAP
		if (port->flags & MVPP2_F_IFCAP_NETMAP) {
			int netmap_done = 0;

			if (netmap_rx_irq(port->dev, 0, &netmap_done))
				return netmap_done;
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
		if (rx_status & MVPP2_RXD_BUF_HDR) {
			mv_pp2x_buff_hdr_rx(port, rx_desc);
			continue;
		}

		if (port->priv->pp2_version == PPV21) {
			data = mv_pp21_rxdesc_cookie_get(rx_desc);
			buf_phys_addr = mv_pp21_rxdesc_phys_addr_get(rx_desc);
		} else {
			data = mv_pp22_rxdesc_cookie_get(rx_desc);
			buf_phys_addr = mv_pp22_rxdesc_phys_addr_get(rx_desc);
		}

#ifdef CONFIG_64BIT
		data = (unsigned char *)((uintptr_t)data | bm_pool->data_high);
#endif

#ifdef MVPP2_VERBOSE
		mv_pp2x_skb_dump(skb, rx_desc->data_size, 4);
#endif
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
		if (rx_status & MVPP2_RXD_ERR_SUMMARY) {
			pr_err("MVPP2_RXD_ERR_SUMMARY\n");
err_drop_frame:
			dev->stats.rx_errors++;
			mv_pp2x_rx_error(port, rx_desc);
			mv_pp2x_pool_refill(port->priv, pool, buf_phys_addr,
				data);
			continue;
		}

		skb = build_skb(data, bm_pool->frag_size > PAGE_SIZE ? 0 :
				bm_pool->frag_size);
		if (!skb) {
			pr_err("skb build failed\n");
			goto err_drop_frame;
		}

		dma_unmap_single(dev->dev.parent, buf_phys_addr,
				 MVPP2_RX_BUF_SIZE(bm_pool->pkt_size),
				 DMA_FROM_DEVICE);

		atomic_inc(&bm_pool->in_use);
		refill_array[bm_pool->log_id]++;

#ifdef MVPP2_VERBOSE
		mv_pp2x_skb_dump(skb, rx_desc->data_size, 4);
#endif
#if 0
		dma_sync_single_for_cpu(dev->dev.parent, buf_phys_addr,
					MVPP2_RX_BUF_SIZE(rx_desc->data_size),
					DMA_FROM_DEVICE);
#endif
		rcvd_pkts++;
		rcvd_bytes += rx_bytes;
		skb_reserve(skb, MVPP2_MH_SIZE+NET_SKB_PAD);
		skb_put(skb, rx_bytes);
		skb->protocol = eth_type_trans(skb, dev);
		mv_pp2x_rx_csum(port, rx_status, skb);

		napi_gro_receive(napi, skb);
	}

	/* Refill pool */
	for (i = 0; i < num_pool; i++) {
		int err;
		struct mv_pp2x_bm_pool *refill_bm_pool;

		if (!refill_array[i])
			continue;

		refill_bm_pool = &port->priv->bm_pools[i];
		while (refill_array[i]--) {
			err = mv_pp2x_rx_refill_new(port, refill_bm_pool,
				refill_bm_pool->log_id + first_bm_pool, 0);
			if (err) {
				netdev_err(port->dev, "failed to refill BM pools\n");
				refill_array[i]++;
				rx_filled -= refill_array[i];
				break;
			}
		}
	}

	if (rcvd_pkts) {
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
	 struct mv_pp2x_tx_queue *txq)
{
	struct mv_pp2x_txq_pcpu *txq_pcpu = this_cpu_ptr(txq->pcpu);
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
		tx_desc->packet_offset = buf_phys_addr & MVPP2_TX_DESC_ALIGN;
		mv_pp2x_txdesc_phys_addr_set(port->priv->pp2_version,
			buf_phys_addr & ~MVPP2_TX_DESC_ALIGN, tx_desc);

		if (i == (skb_shinfo(skb)->nr_frags - 1)) {
			/* Last descriptor */
			tx_desc->command = MVPP2_TXD_L_DESC;
			mv_pp2x_txq_inc_put(port->priv->pp2_version,
				txq_pcpu, skb, tx_desc);
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
	for (i = i - 1; i >= 0; i--) {
		tx_desc = txq->first_desc + i;
		tx_desc_unmap_put(port->dev->dev.parent, txq, tx_desc);
	}

	return -ENOMEM;
}

static inline void mv_pp2x_tx_done_post_proc(struct mv_pp2x_tx_queue *txq,
	struct mv_pp2x_txq_pcpu *txq_pcpu, struct mv_pp2x_port *port, int frags)
{
	int txq_count = mv_pp2x_txq_count(txq_pcpu);

	/* Finalize TX processing */
	if (txq_count >= txq->pkts_coal)
		mv_pp2x_txq_done(port, txq, txq_pcpu);

	/* Recalc after tx_done */
	txq_count = mv_pp2x_txq_count(txq_pcpu);

	/* Set the timer in case not all frags were processed */
	if (txq_count <= frags && txq_count > 0) {
		struct mv_pp2x_port_pcpu *port_pcpu = this_cpu_ptr(port->pcpu);

		mv_pp2x_timer_set(port_pcpu);
	}
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
	int frags = 0;
	u16 txq_id;
	u32 tx_cmd;

	txq_id = 0/*skb_get_queue_mapping(skb)*/;
	nq = netdev_get_tx_queue(dev, txq_id);
	txq = port->txqs[txq_id];
	txq_pcpu = this_cpu_ptr(txq->pcpu);
	aggr_txq = &port->priv->aggr_txqs[smp_processor_id()];

	frags = skb_shinfo(skb)->nr_frags + 1;
	pr_debug("txq_id=%d, frags=%d\n", txq_id, frags);

	/* Check number of available descriptors */
	if (mv_pp2x_aggr_desc_num_check(port->priv, aggr_txq, frags) ||
	    mv_pp2x_txq_reserved_desc_num_proc(port->priv, txq,
					     txq_pcpu, frags)) {
		frags = 0;
		MVPP2_PRINT_LINE();
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

	buf_phys_addr = dma_map_single(dev->dev.parent, skb->data,
				       tx_desc->data_size, DMA_TO_DEVICE);
	if (unlikely(dma_mapping_error(dev->dev.parent, buf_phys_addr))) {
		mv_pp2x_txq_desc_put(txq);
		frags = 0;
		MVPP2_PRINT_LINE();
		goto out;
	}
	pr_debug("buf_phys_addr=%x\n", (unsigned int)buf_phys_addr);

	tx_desc->packet_offset = buf_phys_addr & MVPP2_TX_DESC_ALIGN;
	mv_pp2x_txdesc_phys_addr_set(port->priv->pp2_version,
		buf_phys_addr & ~MVPP2_TX_DESC_ALIGN, tx_desc);

	tx_cmd = mv_pp2x_skb_tx_csum(port, skb);

	if (frags == 1) {
		/* First and Last descriptor */

		tx_cmd |= MVPP2_TXD_F_DESC | MVPP2_TXD_L_DESC;
		tx_desc->command = tx_cmd;
		mv_pp2x_txq_inc_put(port->priv->pp2_version,
			txq_pcpu, skb, tx_desc);
	} else {
		/* First but not Last */
		MVPP2_PRINT_LINE();
		tx_cmd |= MVPP2_TXD_F_DESC | MVPP2_TXD_PADDING_DISABLE;
		tx_desc->command = tx_cmd;
		mv_pp2x_txq_inc_put(port->priv->pp2_version,
			txq_pcpu, NULL, tx_desc);

		/* Continue with other skb fragments */
		if (mv_pp2x_tx_frag_process(port, skb, aggr_txq, txq)) {
			MVPP2_PRINT_LINE();
			tx_desc_unmap_put(port->dev->dev.parent, txq, tx_desc);
			frags = 0;
			goto out;
		}
	}
	txq_pcpu->reserved_num -= frags;
	aggr_txq->count += frags;
	aggr_txq->xmit_bulk += frags;

	/* Prevent shadow_q override, stop tx_queue until tx_done is called*/

	if (mv_pp2x_txq_free_count(txq_pcpu) < (MAX_SKB_FRAGS + 2)) {
		MVPP2_PRINT_LINE();
		netif_tx_stop_queue(nq);
	}

	/* Enable transmit */
	if (!skb->xmit_more || netif_xmit_stopped(nq)) {
		mv_pp2x_aggr_txq_pend_desc_add(port, aggr_txq->xmit_bulk);
		aggr_txq->xmit_bulk = 0;
	}
out:
	if (frags > 0) {
		struct mv_pp2x_pcpu_stats *stats = this_cpu_ptr(port->stats);

		u64_stats_update_begin(&stats->syncp);
		stats->tx_packets++;
		stats->tx_bytes += skb->len;
		u64_stats_update_end(&stats->syncp);
	} else {
		MVPP2_PRINT_LINE();

		dev->stats.tx_dropped++;
		dev_kfree_skb_any(skb);
	}
	/* PPV21 TX Post-Processing */

	if (port->priv->pp2xdata->interrupt_tx_done == false && frags > 0)
		mv_pp2x_tx_done_post_proc(txq, txq_pcpu, port, frags);

	return NETDEV_TX_OK;
}
static inline void mv_pp2x_cause_misc_handle(struct mv_pp2x_port *port,
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

static inline int mv_pp2x_cause_rx_handle(struct mv_pp2x_port *port,
		struct queue_vector *q_vec, struct napi_struct *napi,
		int budget, u32 cause_rx)
{
	int rx_done = 0, count = 0;
	struct mv_pp2x_rx_queue *rxq;

	while (cause_rx && budget > 0) {
		rxq = mv_pp2x_get_rx_queue(port, cause_rx);
		if (!rxq)
			break;

		count = mv_pp2x_rx(port, &q_vec->napi, budget, rxq);
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
		u32 state;

		state = mv_pp2x_qvector_interrupt_state_get(q_vec);
		if (state)
			mv_pp2x_qvector_interrupt_disable(q_vec);
		cause_rx = 0;
		napi_complete(napi);
		if (state)
			mv_pp2x_qvector_interrupt_enable(q_vec);
		q_vec->pending_cause_rx = cause_rx;
		return rx_done;
	}
#endif

	if (budget > 0) {
		cause_rx = 0;
		napi_complete(napi);
		mv_pp2x_qvector_interrupt_enable(q_vec);
	}
	q_vec->pending_cause_rx = cause_rx;

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
	mv_pp2x_cause_misc_handle(port, hw, cause_rx_tx);

	/* Process RX packets */
	cause_rx = cause_rx_tx & MVPP2_CAUSE_RXQ_OCCUP_DESC_ALL_MASK;
	cause_rx |= q_vec->pending_cause_rx;

	rx_done = mv_pp2x_cause_rx_handle(port, q_vec, napi, budget, cause_rx);

	return rx_done;
}

static int mv_pp22_poll(struct napi_struct *napi, int budget)
{
	u32 cause_rx_tx, cause_rx, cause_tx;
	int rx_done = 0, txq_cpu;
	struct mv_pp2x_port *port = netdev_priv(napi->dev);
	struct mv_pp2x_hw *hw = &port->priv->hw;
	struct queue_vector *q_vec = container_of(napi,
			struct queue_vector, napi);

	/* Rx/Tx cause register
	 * Each CPU has its own Tx cause register
	 */

	/*The read is in the q_vector's sw_thread_id  address_space */
	cause_rx_tx = mv_pp22_thread_relaxed_read(hw, q_vec->sw_thread_id,
			MVPP2_ISR_RX_TX_CAUSE_REG(port->id));
	pr_debug("%s port_id(%d), q_vec(%d), cpuId(%d), sw_thread_id(%d), isr_tx_rx(0x%x)\n",
		__func__, port->id, (int)(q_vec - port->q_vector),
		smp_processor_id(), q_vec->sw_thread_id, cause_rx_tx);

	/*Process misc errors */
	mv_pp2x_cause_misc_handle(port, hw, cause_rx_tx);

	/* Release TX descriptors */
	cause_tx = (cause_rx_tx & MVPP2_CAUSE_TXQ_OCCUP_DESC_ALL_MASK) >>
			MVPP2_CAUSE_TXQ_OCCUP_DESC_ALL_OFFSET;
	if (cause_tx) {
		txq_cpu = QV_THR_2_CPU(q_vec->sw_thread_id);
		mv_pp2x_tx_done(port, cause_tx, txq_cpu);
	}

	/* Process RX packets */
	cause_rx = cause_rx_tx & MVPP2_CAUSE_RXQ_OCCUP_DESC_ALL_MASK;
	/*Convert queues from subgroup-relative to port-relative */
	cause_rx <<= q_vec->first_rx_queue;
	cause_rx |= q_vec->pending_cause_rx;

	rx_done = mv_pp2x_cause_rx_handle(port, q_vec, napi, budget, cause_rx);

	return rx_done;
}

void mv_pp2x_port_napi_enable(struct mv_pp2x_port *port)
{
	int i;

	for (i = 0; i < port->num_qvector; i++)
		napi_enable(&port->q_vector[i].napi);
}

void mv_pp2x_port_napi_disable(struct mv_pp2x_port *port)
{
	int i;

	for (i = 0; i < port->num_qvector; i++)
		napi_disable(&port->q_vector[i].napi);
}

static inline void mv_pp2x_port_irqs_dispose_mapping(struct mv_pp2x_port *port)
{
	int i;

	for (i = 0; i < port->num_irqs; i++)
		irq_dispose_mapping(port->of_irqs[i]);
}

static int mvcpn110_mac_hw_init(struct mv_pp2x_port *port)
{

	struct gop_hw *gop = &port->priv->hw.gop;
	struct mv_mac_data *mac = &port->mac_data;
	int gop_port = mac->gop_index;

	if (mac->flags & MV_EMAC_F_INIT)
		return 0;

	/* configure port PHY address */
	mv_gop110_smi_phy_addr_cfg(gop, gop_port, mac->phy_addr);

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
	if (port->priv->pp2_version == PPV21)
		mv_pp21_gmac_max_rx_size_set(port);
	else {
		switch (mac->phy_mode) {
		case PHY_INTERFACE_MODE_RGMII:
		case PHY_INTERFACE_MODE_SGMII:
		case PHY_INTERFACE_MODE_QSGMII:
			mv_gop110_gmac_max_rx_size_set(gop, mac_num,
					port->pkt_size);
		break;
		case PHY_INTERFACE_MODE_XAUI:
		case PHY_INTERFACE_MODE_RXAUI:
		case PHY_INTERFACE_MODE_KR:
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
#if !defined(CONFIG_MV_PP2_POLLING)
	mv_pp2x_port_interrupts_enable(port);
#endif

	if (port->priv->pp2_version == PPV21) {
		mv_pp21_port_enable(port);
	} else {
		mv_gop110_port_events_mask(gop, mac);
		mv_gop110_port_enable(gop, mac);
	}
	if (port->mac_data.phy_dev)
		phy_start(port->mac_data.phy_dev);
	else
		mv_pp22_dev_link_event(port->dev);

	tasklet_init(&port->link_change_tasklet, mv_pp2_link_change_tasklet,
		(unsigned long)(port->dev));

	mv_pp2x_egress_enable(port);
	mv_pp2x_ingress_enable(port);
	MVPP2_PRINT_VAR(mac->phy_mode);
	/* Unmask link_event */
#if !defined(CONFIG_MV_PP2_POLLING)
	if (port->priv->pp2_version == PPV22)
		mv_gop110_port_events_unmask(gop, mac);
#endif
}

/* Set hw internals when stopping port */
void mv_pp2x_stop_dev(struct mv_pp2x_port *port)
{
	struct gop_hw *gop = &port->priv->hw.gop;
	struct mv_mac_data *mac = &port->mac_data;

	/* Stop new packets from arriving to RXQs */
	mv_pp2x_ingress_disable(port);

	mdelay(10);

	/* Disable interrupts on all CPUs */
	mv_pp2x_port_interrupts_disable(port);

	mv_pp2x_port_napi_disable(port);

	netif_carrier_off(port->dev);
	netif_tx_stop_all_queues(port->dev);

	mv_pp2x_egress_disable(port);
	if (port->priv->pp2_version == PPV21) {
		mv_pp21_port_disable(port);
	} else {
		mv_gop110_port_events_mask(gop, mac);
		mv_gop110_port_disable(gop, mac);
		port->mac_data.flags &= ~MV_EMAC_F_LINK_UP;
		tasklet_kill(&port->link_change_tasklet);
	}

	if (port->mac_data.phy_dev)
		phy_stop(port->mac_data.phy_dev);
}

/* Return positive if MTU is valid */
static inline int mv_pp2x_check_mtu_valid(struct net_device *dev, int mtu)
{
	if (mtu < 68) {
		netdev_err(dev, "cannot change mtu to less than 68\n");
		return -EINVAL;
	}
	if (MVPP2_RX_PKT_SIZE(mtu) > MVPP2_BM_LONG_PKT_SIZE &&
		jumbo_pool == false) {
		netdev_err(dev, "jumbo packet not supported (%d)\n", mtu);
		return -EINVAL;
	}

	/* 9676 == 9700 - 20 and rounding to 8 */
	if (mtu > 9676) {
		netdev_info(dev, "illegal MTU value %d, round to 9676\n", mtu);
		mtu = 9676;
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

static int mv_pp2x_phy_connect(struct mv_pp2x_port *port)
{
	struct phy_device *phy_dev;

	phy_dev = of_phy_connect(port->dev, port->mac_data.phy_node,
		mv_pp21_link_event, 0, port->mac_data.phy_mode);
	if (!phy_dev) {
		netdev_err(port->dev, "cannot connect to phy\n");
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
	struct mv_pp2x_hw *hw = &(port->priv->hw);
	int err;
	u32 cpu_width = 0, cos_width = 0, port_rxq_width = 0;
	u8 bound_cpu_first_rxq;

	/* Calculate width */
	mv_pp2x_width_calc(port->priv, &cpu_width, &cos_width, &port_rxq_width);
	if (cpu_width + cos_width > port_rxq_width) {
		err = -1;
		netdev_err(dev, "cpu or cos queue width invalid\n");
		return err;
	}

	err = mv_pp2x_prs_mac_da_accept(hw, port->id, mac_bcast, true);
	if (err) {
		netdev_err(dev, "mv_pp2x_prs_mac_da_accept BC failed\n");
		return err;
	}

	err = mv_pp2x_prs_mac_da_accept(hw, port->id,
				      dev->dev_addr, true);
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

	/* Assign rss table for rxq belong to this port */
	err = mv_pp22_rss_rxq_set(port, cos_width);
	if (err) {
		netdev_err(port->dev, "cannot allocate rss table for rxq\n");
		return err;
	}

	/* RSS related config */
	if (port->priv->pp2_cfg.queue_mode == MVPP2_QDIST_MULTI_MODE) {
		/* Set RSS mode */
		err = mv_pp22_rss_mode_set(port,
			port->priv->pp2_cfg.rss_cfg.rss_mode);
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
		if (port->priv->pp2_cfg.rss_cfg.rss_en) {
			err = mv_pp22_rss_default_cpu_set(port,
					port->priv->pp2_cfg.rss_cfg.dflt_cpu);
			if (err) {
				netdev_err(port->dev, "cannot set rss default cpu\n");
				return err;
			}
		}

	}

	return 0;
}

int mv_pp2x_open(struct net_device *dev)
{
	struct mv_pp2x_port *port = netdev_priv(dev);
	int err;

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

#if !defined(CONFIG_MV_PP2_POLLING)
	err = mv_pp2x_setup_irqs(dev, port);
	if (err) {
		netdev_err(port->dev, "cannot allocate irq's\n");
		goto err_cleanup_txqs;
	}
#endif
	/* In default link is down */
	netif_carrier_off(port->dev);

	/*FIXME: Should check which gop_version
	 * (better, gop_version_attr: support_phy_connect), not the pp_version
	 */
	if (port->priv->pp2_version == PPV21) {
		err = mv_pp2x_phy_connect(port);
		if (err < 0)
			goto err_free_irq;
	}

#if !defined(CONFIG_MV_PP2_POLLING)

	/* Unmask interrupts on all CPUs */
	on_each_cpu(mv_pp2x_interrupts_unmask, port, 1);

	/* Unmask shared interrupts */
	mv_pp2x_shared_thread_interrupts_unmask(port);
#endif

#if defined(CONFIG_MV_PP2_POLLING)
	if (cpu_poll_timer_ref_cnt == 0) {
		cpu_poll_timer.expires  =
		jiffies + msecs_to_jiffies(MV_PP2_FPGA_PERODIC_TIME*100);
		add_timer(&cpu_poll_timer);
		cpu_poll_timer_ref_cnt++;
	}
#endif
	/* Port is init in uboot */

	if (port->priv->pp2_version == PPV22)
		mvcpn110_mac_hw_init(port);
	mv_pp2x_start_dev(port);

	/* Before rxq and port init, all ingress packets should be blocked
	 *  in classifier
	 */
	err = mv_pp2x_open_cls(dev);
	if (err)
		goto err_free_all;

	MVPP2_PRINT_2LINE();
	return 0;

err_free_all:
err_free_irq:
	mv_pp2x_cleanup_irqs(port);
#if !defined(CONFIG_MV_PP2_POLLING)
err_cleanup_txqs:
	mv_pp2x_cleanup_txqs(port);
	MVPP2_PRINT_2LINE();
#endif
err_cleanup_rxqs:
	mv_pp2x_cleanup_rxqs(port);
	MVPP2_PRINT_2LINE();
	return err;
}

int mv_pp2x_stop(struct net_device *dev)
{
	struct mv_pp2x_port *port = netdev_priv(dev);
	struct mv_pp2x_port_pcpu *port_pcpu;
	int cpu;

#if defined(CONFIG_MV_PP2_POLLING)
	cpu_poll_timer_ref_cnt--;
	if (cpu_poll_timer_ref_cnt == 0)
		del_timer_sync(&cpu_poll_timer);
#endif
	mv_pp2x_stop_dev(port);

	if (port->priv->pp2_version == PPV21)
		mv_pp2x_phy_disconnect(port);

#if !defined(CONFIG_MV_PP2_POLLING)
	/* Mask interrupts on all CPUs */
	on_each_cpu(mv_pp2x_interrupts_mask, port, 1);

	/* Mask shared interrupts */
	mv_pp2x_shared_thread_interrupts_mask(port);
	mv_pp2x_cleanup_irqs(port);
#endif
	if (port->priv->pp2xdata->interrupt_tx_done == false) {
		for_each_online_cpu(cpu) {
			port_pcpu = per_cpu_ptr(port->pcpu, cpu);
			hrtimer_cancel(&port_pcpu->tx_done_timer);
			port_pcpu->timer_scheduled = false;
			tasklet_kill(&port_pcpu->tx_done_tasklet);
		}
	}

	mv_pp2x_cleanup_rxqs(port);
	mv_pp2x_cleanup_txqs(port);

	return 0;
}

/* register unicast and multicast addresses */
static void mv_pp2x_set_rx_mode(struct net_device *dev)
{
	struct mv_pp2x_port *port = netdev_priv(dev);
	struct netdev_hw_addr *ha;
	struct mv_pp2x_hw *hw = &(port->priv->hw);
	int id = port->id;
	int err;

	if (dev->flags & IFF_PROMISC) {
		/* Accept all: Multicast + Unicast */
		mv_pp2x_prs_mac_multi_set(hw, id, MVPP2_PE_MAC_MC_ALL, true);
		mv_pp2x_prs_mac_multi_set(hw, id, MVPP2_PE_MAC_MC_IP6, true);
		/* Remove all port->id's mcast enries */
		mv_pp2x_prs_mcast_del_all(hw, id);
		/* Enter promisc mode */
		mv_pp2x_prs_mac_promisc_set(hw, id, true);
	} else {
		if (dev->flags & IFF_ALLMULTI) {
			/* Accept all multicast */
			mv_pp2x_prs_mac_multi_set(hw, id,
						  MVPP2_PE_MAC_MC_ALL, true);
			mv_pp2x_prs_mac_multi_set(hw, id,
						  MVPP2_PE_MAC_MC_IP6, true);
			/* Remove all port->id's mcast enries */
			mv_pp2x_prs_mcast_del_all(hw, id);
		} else {
			/* Accept only initialized multicast */
			if (!netdev_mc_empty(dev)) {
				netdev_for_each_mc_addr(ha, dev) {
					err = mv_pp2x_prs_mac_da_accept(hw,
							id, ha->addr, true);
					if (err)
						netdev_err(dev,
						"MAC[%2x:%2x:%2x:%2x:%2x:%2x] add failed\n",
						ha->addr[0], ha->addr[1],
						ha->addr[2], ha->addr[3],
						ha->addr[4], ha->addr[5]);
				}
			}
			mv_pp2x_prs_mac_multi_set(hw, id,
						  MVPP2_PE_MAC_MC_ALL, false);
			mv_pp2x_prs_mac_multi_set(hw, id,
						  MVPP2_PE_MAC_MC_IP6, false);
		}
		/* Leave promisc mode */
		mv_pp2x_prs_mac_promisc_set(hw, id, false);
	}
}

static int mv_pp2x_set_mac_address(struct net_device *dev, void *p)
{
	struct mv_pp2x_port *port = netdev_priv(dev);
	const struct sockaddr *addr = p;
	int err;

	if (!is_valid_ether_addr(addr->sa_data)) {
		err = -EADDRNOTAVAIL;
		goto error;
	}

	if (!netif_running(dev)) {
		err = mv_pp2x_prs_update_mac_da(dev, addr->sa_data);
		if (!err)
			return 0;
		/* Reconfigure parser to accept the original MAC address */
		err = mv_pp2x_prs_update_mac_da(dev, dev->dev_addr);
		goto error;
	}

	mv_pp2x_stop_dev(port);

	err = mv_pp2x_prs_update_mac_da(dev, addr->sa_data);
	if (!err)
		goto out_start;

	/* Reconfigure parser accept the original MAC address */
	err = mv_pp2x_prs_update_mac_da(dev, dev->dev_addr);
	if (err)
		goto error;
out_start:
	mv_pp2x_start_dev(port);
	return 0;

error:
	netdev_err(dev, "fail to change MAC address\n");
	return err;
}

static int mv_pp2x_change_mtu(struct net_device *dev, int mtu)
{
	struct mv_pp2x_port *port = netdev_priv(dev);
	int err;

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

static struct rtnl_link_stats64 *
mv_pp2x_get_stats64(struct net_device *dev, struct rtnl_link_stats64 *stats)
{
	struct mv_pp2x_port *port = netdev_priv(dev);
	unsigned int start;
	int cpu;

	for_each_possible_cpu(cpu) {
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
		if (features & NETIF_F_RXHASH) {
			/* Enable RSS */
			mv_pp22_rss_enable(port, true);
		} else {
			/* Disable RSS */
			mv_pp22_rss_enable(port, false);
		}
	}

	dev->features = features;

	return 0;
}

/* Device ops */

static const struct net_device_ops mv_pp2x_netdev_ops = {
	.ndo_open		= mv_pp2x_open,
	.ndo_stop		= mv_pp2x_stop,
	.ndo_start_xmit		= mv_pp2x_tx,
	.ndo_set_rx_mode	= mv_pp2x_set_rx_mode,
	.ndo_set_mac_address	= mv_pp2x_set_mac_address,
	.ndo_change_mtu		= mv_pp2x_change_mtu,
	.ndo_get_stats64	= mv_pp2x_get_stats64,
	.ndo_do_ioctl		= mv_pp2x_ioctl,
	.ndo_set_features	= mv_pp2x_netdev_set_features,
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

		txq->pcpu = alloc_percpu(struct mv_pp2x_txq_pcpu);
		if (!txq->pcpu)
			return(-ENOMEM);

		txq->id = queue_phy_id;
		txq->log_id = queue;
		txq->pkts_coal = MVPP2_TXDONE_COAL_PKTS;

		for_each_online_cpu(cpu) {
			txq_pcpu = per_cpu_ptr(txq->pcpu, cpu);
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
	q_vec[0].qv_type = MVPP2_SHARED;
	q_vec[0].sw_thread_id = 0;
	q_vec[0].sw_thread_mask = port->priv->cpu_map;
	q_vec[0].irq = port->of_irqs[0];
	netif_napi_add(port->dev, &q_vec[0].napi, mv_pp21_poll,
		NAPI_POLL_WEIGHT);

	port->num_qvector = 1;
}

static int mv_pp2_num_cpu_irqs(struct mv_pp2x_port *port)
{
	int cpu_avail_irq;

	cpu_avail_irq = port->num_irqs -
		((mv_pp2x_queue_mode == MVPP2_QDIST_SINGLE_MODE) ? 1 : 0);
	if (cpu_avail_irq < 0)
		return 0;
	return min((num_active_cpus()), ((unsigned int)cpu_avail_irq));
}

static void mv_pp22_queue_vectors_init(struct mv_pp2x_port *port)
{
	int cpu;
	int sw_thread_index = first_addr_space, irq_index = first_addr_space;
	struct queue_vector *q_vec = &port->q_vector[0];
	struct net_device  *net_dev = port->dev;

	/* Each cpu has queue_vector for private tx_done counters and/or
	 *  private rx_queues
	 */
	for (cpu = 0; cpu < num_active_cpus(); cpu++) {
		q_vec[cpu].parent = port;
		q_vec[cpu].qv_type = MVPP2_PRIVATE;
		q_vec[cpu].sw_thread_id = sw_thread_index++;
		q_vec[cpu].sw_thread_mask = (1<<q_vec[cpu].sw_thread_id);
		q_vec[cpu].pending_cause_rx = 0;
#if !defined(CONFIG_MV_PP2_POLLING)
		if (port->priv->pp2xdata->interrupt_tx_done == true ||
		    mv_pp2x_queue_mode == MVPP2_QDIST_MULTI_MODE)
			q_vec[cpu].irq = port->of_irqs[irq_index++];
#endif
		netif_napi_add(net_dev, &q_vec[cpu].napi, mv_pp22_poll,
			NAPI_POLL_WEIGHT);
		if (mv_pp2x_queue_mode == MVPP2_QDIST_MULTI_MODE) {
			q_vec[cpu].num_rx_queues = mv_pp2x_num_cos_queues;
			q_vec[cpu].first_rx_queue = cpu*mv_pp2x_num_cos_queues;
		} else {
			q_vec[cpu].first_rx_queue = 0;
			q_vec[cpu].num_rx_queues = 0;
		}
		port->num_qvector++;
	}
	/*Additional queue_vector for Shared RX */
	if (mv_pp2x_queue_mode == MVPP2_QDIST_SINGLE_MODE) {
		q_vec[cpu].parent = port;
		q_vec[cpu].qv_type = MVPP2_SHARED;
		q_vec[cpu].sw_thread_id = irq_index;
		q_vec[cpu].sw_thread_mask = (1<<q_vec[cpu].sw_thread_id);
		q_vec[cpu].pending_cause_rx = 0;
#if !defined(CONFIG_MV_PP2_POLLING)
		q_vec[cpu].irq = port->of_irqs[irq_index];
#endif
		netif_napi_add(net_dev, &q_vec[cpu].napi, mv_pp22_poll,
			NAPI_POLL_WEIGHT);
		q_vec[cpu].first_rx_queue = 0;
		q_vec[cpu].num_rx_queues = port->num_rx_queues;

		port->num_qvector++;
	}
}

static void mv_pp22_port_irq_names_update(struct mv_pp2x_port *port)
{
	int i, cpu;
	struct queue_vector *q_vec = &port->q_vector[0];
	char str_common[32];
	struct net_device  *net_dev = port->dev;
	struct device *parent_dev;

	parent_dev = net_dev->dev.parent;

	snprintf(str_common, sizeof(str_common), "%s.%s",
		dev_name(parent_dev), net_dev->name);

	for (i = 0; i < port->num_qvector; i++) {
		if (!q_vec[i].irq)
			continue;
		if (q_vec[i].qv_type == MVPP2_PRIVATE) {
			cpu = QV_THR_2_CPU(q_vec[i].sw_thread_id);
			snprintf(q_vec[i].irq_name, IRQ_NAME_SIZE, "%s.%s%d",
				str_common, "cpu", cpu);
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
	struct mv_pp2x_hw *hw = &(port->priv->hw);

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
#if !defined(CONFIG_MV_PP2_POLLING)
	port->mac_data.link_irq = irq_of_parse_and_map(emac_node, 0);
#endif

	phy_mode = of_get_phy_mode(emac_node);

	if (of_phy_is_fixed_link(emac_node)) {
		port->mac_data.force_link = true;
		port->mac_data.link = true;
		fixed_link_node = of_get_child_by_name(emac_node, "fixed-link");
		port->mac_data.duplex = of_property_read_bool(fixed_link_node,
				"full-duplex");
		if (of_property_read_u32(fixed_link_node, "speed",
				&port->mac_data.speed))
			return -EINVAL;
	} else {
		port->mac_data.force_link = false;

		switch (phy_mode) {
		case PHY_INTERFACE_MODE_SGMII:
			speed = 0;
			/* check phy speed */
			of_property_read_u32(emac_node, "phy-speed", &speed);
			switch (speed) {
			case 1000:
				port->mac_data.speed = 1000; /* sgmii */
				break;
			case 2500:
				port->mac_data.speed = 2500; /* sgmii */
				port->mac_data.flags |= MV_EMAC_F_SGMII2_5;
				break;
			default:
				port->mac_data.speed = 1000; /* sgmii */
			}
			break;
		case PHY_INTERFACE_MODE_RXAUI:
			break;
		case PHY_INTERFACE_MODE_QSGMII:
			break;
		case PHY_INTERFACE_MODE_RGMII:
			break;
		case PHY_INTERFACE_MODE_KR:
			break;

		default:
			pr_err("%s: incorrect phy-mode\n", __func__);
			return -1;
		}
	}
	port->mac_data.phy_mode = phy_mode;
	pr_debug("gop_mac(%d), phy_mode(%d) (%s)\n", id,  phy_mode,
		phy_modes(phy_mode));
	pr_debug("gop_mac(%d), phy_speed(%d)\n", id,  port->mac_data.speed);


	phy_node = of_parse_phandle(emac_node, "phy", 0);
	if (phy_node) {
		port->mac_data.phy_node = phy_node;
		if (of_property_read_u32(phy_node, "reg",
		    &port->mac_data.phy_addr))
			pr_err("%s: NO PHY address on emac %d\n", __func__,
			       port->mac_data.gop_index);

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
			if (mac->phy_mode == PHY_INTERFACE_MODE_SGMII)
				val |= MV_NETC_GE_MAC2_SGMII;
		}
		if (mac->gop_index == 3) {
			if (mac->phy_mode == PHY_INTERFACE_MODE_SGMII)
				val |= MV_NETC_GE_MAC3_SGMII;
			else if (mac->phy_mode == PHY_INTERFACE_MODE_RGMII)
				val |= MV_NETC_GE_MAC3_RGMII;
		}
	}
	return val;
}

/* Initialize port HW */
static int mv_pp2x_port_init(struct mv_pp2x_port *port)
{
	struct device *dev = port->dev->dev.parent;
	struct mv_pp2x *priv = port->priv;
	struct gop_hw *gop = &port->priv->hw.gop;
	struct mv_mac_data *mac = &port->mac_data;
	int queue, err;

	/* Disable port */
	mv_pp2x_egress_disable(port);

	if (port->priv->pp2_version == PPV21)
		mv_pp21_port_disable(port);
	else
		mv_gop110_port_disable(gop, mac);

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
		goto err_free_percpu;

	/* Associate physical Rx queues to port and initialize.  */
	err = mv_pp2x_port_rxqs_init(dev, port);

	if (err)
		goto err_free_percpu;

	/* Configure queue_vectors */
	priv->pp2xdata->mv_pp2x_port_queue_vectors_init(port);

	/* Configure Rx queue group interrupt for this port */
	priv->pp2xdata->mv_pp2x_port_isr_rx_group_cfg(port);

	/* Create Rx descriptor rings */
	for (queue = 0; queue < port->num_rx_queues; queue++) {
		struct mv_pp2x_rx_queue *rxq = port->rxqs[queue];

		rxq->size = port->rx_ring_size;
		rxq->pkts_coal = MVPP2_RX_COAL_PKTS;
		rxq->time_coal = MVPP2_RX_COAL_USEC;
	}

	mv_pp2x_ingress_disable(port);

	/* Port default configuration */
	mv_pp2x_defaults_set(port);

	/* Port's classifier configuration */
	mv_pp2x_cls_oversize_rxq_set(port);
	mv_pp2x_cls_port_config(port);

	/* Provide an initial Rx packet size */
	port->pkt_size = MVPP2_RX_PKT_SIZE(port->dev->mtu);

	/* Initialize pools for swf */
	err = mv_pp2x_swf_bm_pool_init(port);
	if (err)
		goto err_free_percpu;
	return 0;

err_free_percpu:
	for (queue = 0; queue < port->num_tx_queues; queue++) {
		if (!port->txqs[queue])
			continue;
		free_percpu(port->txqs[queue]->pcpu);
	}
	return err;
}

/* Ports initialization */
static int mv_pp2x_port_probe(struct platform_device *pdev,
			    struct device_node *port_node,
			    struct mv_pp2x *priv)
{
	struct device_node *emac_node;
	struct mv_pp2x_port *port;
	struct mv_pp2x_port_pcpu *port_pcpu;
	struct net_device *dev;
	struct resource *res;
	const char *dt_mac_addr;
	const char *mac_from;
	char hw_mac_addr[ETH_ALEN];
	u32 id;
	int features, err = 0, i, cpu;
	int priv_common_regs_num = 2;
#if !defined(CONFIG_MV_PP2_POLLING)
	unsigned int *port_irqs;
	int port_num_irq;
#endif
	dev = alloc_etherdev_mqs(sizeof(struct mv_pp2x_port),
		mv_pp2x_txq_number, mv_pp2x_rxq_number);
	if (!dev)
		return -ENOMEM;

	/*Connect entities */
	port = netdev_priv(dev);
	port->dev = dev;
	SET_NETDEV_DEV(dev, &pdev->dev);
	port->priv = priv;

	if (of_property_read_u32(port_node, "port-id", &id)) {
		err = -EINVAL;
		dev_err(&pdev->dev, "missing port-id value\n");
		goto err_free_netdev;
	}
	port->id = id;

	emac_node = of_parse_phandle(port_node, "emac-data", 0);
	if (!emac_node) {
		dev_err(&pdev->dev, "missing emac-data\n");
		err = -EINVAL;
		goto err_free_netdev;
	}
	/* Init emac_data, includes link interrupt */
	if (mv_pp2_init_emac_data(port, emac_node))
		goto err_free_netdev;

	/* get MAC address */
	dt_mac_addr = of_get_mac_address(emac_node);
	if (dt_mac_addr && is_valid_ether_addr(dt_mac_addr)) {
		mac_from = "device tree";
		ether_addr_copy(dev->dev_addr, dt_mac_addr);
		pr_debug("gop_index(%d), mac_addr %x:%x:%x:%x:%x:%x",
			port->mac_data.gop_index, dev->dev_addr[0],
			dev->dev_addr[1], dev->dev_addr[2], dev->dev_addr[3],
			dev->dev_addr[4], dev->dev_addr[5]);
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

	/* Tx/Rx Interrupt */
#if !defined(CONFIG_MV_PP2_POLLING)
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
	if (port_num_irq > PPV2_MAX_NUM_IRQ)
		port_num_irq = PPV2_MAX_NUM_IRQ;
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
#endif

	/*FIXME, full handling loopback */
	if (of_property_read_bool(port_node, "marvell,loopback"))
		port->flags |= MVPP2_F_LOOPBACK;

	port->num_tx_queues = mv_pp2x_txq_number;
	port->num_rx_queues = mv_pp2x_rxq_number;
	dev->tx_queue_len = MVPP2_MAX_TXD;
	dev->watchdog_timeo = 5 * HZ;
	dev->netdev_ops = &mv_pp2x_netdev_ops;
	mv_pp2x_set_ethtool_ops(dev);

	/*YuvalC: Port first_rxq relative to port->id, not dependent on board
	 * topology, i.e. not dynamically allocated
	 */
	port->first_rxq = (port->id)*(priv->pp2xdata->pp2x_max_port_rxqs) +
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

	if (mv_pp2_num_cpu_irqs(port) < num_active_cpus() &&
	    port->priv->pp2xdata->interrupt_tx_done == true) {
		port->priv->pp2xdata->interrupt_tx_done = false;
		pr_info("mvpp2x: interrupt_tx_done override to false\n");
	}

	err = mv_pp2x_port_init(port);
	if (err < 0) {
		dev_err(&pdev->dev, "failed to init port %d\n", id);
		goto err_free_stats;
	}
	if (port->priv->pp2_version == PPV21)
		mv_pp21_port_power_up(port);

	port->pcpu = alloc_percpu(struct mv_pp2x_port_pcpu);
	if (!port->pcpu) {
		err = -ENOMEM;
		goto err_free_txq_pcpu;
	}
	if (port->priv->pp2xdata->interrupt_tx_done == false) {
		for_each_online_cpu(cpu) {
			port_pcpu = per_cpu_ptr(port->pcpu, cpu);

			hrtimer_init(&port_pcpu->tx_done_timer, CLOCK_MONOTONIC,
				     HRTIMER_MODE_REL_PINNED);
			port_pcpu->tx_done_timer.function = mv_pp2x_hr_timer_cb;
			port_pcpu->timer_scheduled = false;

			tasklet_init(&port_pcpu->tx_done_tasklet,
				mv_pp2x_tx_proc_cb, (unsigned long)dev);
		}
	}
	features = NETIF_F_SG;
	dev->features = features | NETIF_F_RXCSUM | NETIF_F_IP_CSUM |
				NETIF_F_IPV6_CSUM;
	dev->hw_features |= features | NETIF_F_RXCSUM | NETIF_F_GRO |
				NETIF_F_IP_CSUM | NETIF_F_IPV6_CSUM;
	/* Only when multi queue mode, rxhash is supported */
	if (mv_pp2x_queue_mode)
		dev->hw_features |= NETIF_F_RXHASH;
	dev->vlan_features |= features;
	err = register_netdev(dev);
	if (err < 0) {
		dev_err(&pdev->dev, "failed to register netdev\n");
		goto err_free_port_pcpu;
	}
	mv_pp22_port_irq_names_update(port);

	netdev_info(dev, "Using %s mac address %pM\n", mac_from, dev->dev_addr);

	priv->port_list[priv->num_ports] = port;
	priv->num_ports++;

#ifdef DEV_NETMAP
	mv_pp2x_netmap_attach(port);
#endif /* DEV_NETMAP */

	return 0;
	dev_err(&pdev->dev, "%s failed for port_id(%d)\n", __func__, id);

err_free_port_pcpu:
	free_percpu(port->pcpu);
err_free_txq_pcpu:
	for (i = 0; i < mv_pp2x_txq_number; i++)
		free_percpu(port->txqs[i]->pcpu);
err_free_stats:
	free_percpu(port->stats);
err_free_irq:
#if !defined(CONFIG_MV_PP2_POLLING)
	mv_pp2x_port_irqs_dispose_mapping(port);
#endif
err_free_netdev:
	free_netdev(dev);
	return err;
}

/* Ports removal routine */
static void mv_pp2x_port_remove(struct mv_pp2x_port *port)
{
	int i;
#ifdef DEV_NETMAP
	netmap_detach(port->dev);
#endif /* DEV_NETMAP */

	unregister_netdev(port->dev);
	free_percpu(port->pcpu);
	free_percpu(port->stats);
	for (i = 0; i < port->num_tx_queues; i++)
		free_percpu(port->txqs[i]->pcpu);
#if !defined(CONFIG_MV_PP2_POLLING)
	mv_pp2x_port_irqs_dispose_mapping(port);
#endif
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

	/* Checks for hardware constraints */
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

	mv_pp2x_write(hw, MVPP22_BM_PHY_VIRT_HIGH_RLS_REG, 0x0);

	/*AXI Bridge Configuration */

	if (is_device_dma_coherent(&pdev->dev)) {

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
	} else {

		/* BM */
		mv_pp2x_write(hw, MVPP22_AXI_BM_WR_ATTR_REG,
			MVPP22_AXI_ATTR_NON_CACHE);
		mv_pp2x_write(hw, MVPP22_AXI_BM_RD_ATTR_REG,
			MVPP22_AXI_ATTR_NON_CACHE);

		/* Descriptors */
		mv_pp2x_write(hw, MVPP22_AXI_AGGRQ_DESCR_RD_ATTR_REG,
			MVPP22_AXI_ATTR_NON_CACHE);
		mv_pp2x_write(hw, MVPP22_AXI_TXQ_DESCR_WR_ATTR_REG,
			MVPP22_AXI_ATTR_NON_CACHE);
		mv_pp2x_write(hw, MVPP22_AXI_TXQ_DESCR_RD_ATTR_REG,
			MVPP22_AXI_ATTR_NON_CACHE);
		mv_pp2x_write(hw, MVPP22_AXI_RXQ_DESCR_WR_ATTR_REG,
			MVPP22_AXI_ATTR_NON_CACHE);

		/* Buffer Data */
		mv_pp2x_write(hw, MVPP22_AXI_RX_DATA_WR_ATTR_REG,
			MVPP22_AXI_ATTR_SW_COH_WRITE);
		mv_pp2x_write(hw, MVPP22_AXI_TX_DATA_RD_ATTR_REG,
			MVPP22_AXI_ATTR_SW_COH_READ);
	}

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

	/* Disable HW PHY polling */
	if (priv->pp2_version == PPV21) {
		val = readl(hw->lms_base + MVPP2_PHY_AN_CFG0_REG);
		val |= MVPP2_PHY_AN_STOP_SMI0_MASK;
		writel(val, hw->lms_base + MVPP2_PHY_AN_CFG0_REG);
		writel(MVPP2_EXT_GLOBAL_CTRL_DEFAULT,
			hw->lms_base + MVPP2_MNG_EXTENDED_GLOBAL_CTRL_REG);
	}

	/* Allocate and initialize aggregated TXQs */
	priv->aggr_txqs = devm_kcalloc(&pdev->dev, num_active_cpus(),
				       sizeof(struct mv_pp2x_aggr_tx_queue),
				       GFP_KERNEL);

	if (!priv->aggr_txqs)
		return -ENOMEM;
	priv->num_aggr_qs = num_active_cpus();

	i = 0;
	for_each_online_cpu(cpu) {
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
	if (is_device_dma_coherent(&pdev->dev))
		mv_pp2x_write(hw, MVPP2_TX_SNOOP_REG, 0x1);
	else
		mv_pp2x_write(hw, MVPP2_TX_SNOOP_REG, 0x0);

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
#ifdef CONFIG_64BIT
	.skb_base_addr = 0,
	.skb_base_mask = DMA_BIT_MASK(32),
#endif
};

static struct mv_pp2x_platform_data pp22_pdata = {
	.pp2x_ver = PPV22,
	.pp2x_max_port_rxqs = 32,
	.mv_pp2x_rxq_short_pool_set = mv_pp22_rxq_short_pool_set,
	.mv_pp2x_rxq_long_pool_set = mv_pp22_rxq_long_pool_set,
	.multi_addr_space = true,
#ifdef CONFIG_MV_PP2_POLLING
	.interrupt_tx_done = false,
#else
	.interrupt_tx_done = true, /*temp. value*/
#endif
	.multi_hw_instance = true,
	.mv_pp2x_port_queue_vectors_init = mv_pp22_queue_vectors_init,
	.mv_pp2x_port_isr_rx_group_cfg = mv_pp22_port_isr_rx_group_cfg,
	.num_port_irq = 5,
	.hw.desc_queue_addr_shift = MVPP22_DESC_ADDR_SHIFT,
	.skb_base_addr = 0,
	.skb_base_mask = DMA_BIT_MASK(32),
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

static void mv_pp2x_init_config(struct mv_pp2x_param_config *pp2_cfg,
	u32 cell_index)
{
	pp2_cfg->cell_index = cell_index;
	pp2_cfg->first_bm_pool = first_bm_pool;
	pp2_cfg->first_sw_thread = first_addr_space;
	pp2_cfg->first_log_rxq = first_log_rxq_queue;
	pp2_cfg->jumbo_pool = jumbo_pool;
	pp2_cfg->queue_mode = mv_pp2x_queue_mode;

	pp2_cfg->cos_cfg.cos_classifier = cos_classifer;
	pp2_cfg->cos_cfg.default_cos = default_cos;
	pp2_cfg->cos_cfg.num_cos_queues = mv_pp2x_num_cos_queues;
	pp2_cfg->cos_cfg.pri_map = pri_map;

	pp2_cfg->rss_cfg.dflt_cpu = default_cpu;
	/* RSS is disabled as default, which can be update in running time */
	pp2_cfg->rss_cfg.rss_en = 0;
	pp2_cfg->rss_cfg.rss_mode = rss_mode;

	pp2_cfg->rx_cpu_map = port_cpu_bind_map;
}

static void mv_pp2x_init_rxfhindir(struct mv_pp2x *pp2)
{
	int i;
	int online_cpus = mv_pp2x_num_online_cpu_get(pp2);

	if (!online_cpus)
		return;

	for (i = 0; i < MVPP22_RSS_TBL_LINE_NUM; i++)
		pp2->rx_indir_table[i] = i%online_cpus;
}

void mv_pp2x_pp2_basic_print(struct platform_device *pdev, struct mv_pp2x *priv)
{
	DBG_MSG("%s\n", __func__);

	DBG_MSG("num_present_cpus(%d) num_act_cpus(%d) num_online_cpus(%d)\n",
		num_present_cpus(), num_active_cpus(), num_online_cpus());
	DBG_MSG("cpu_map(%x)\n", priv->cpu_map);

	DBG_MSG("pdev->name(%s) pdev->id(%d)\n", pdev->name, pdev->id);
	DBG_MSG("dev.init_name(%s) dev.id(%d)\n",
		pdev->dev.init_name, pdev->dev.id);
	DBG_MSG("dev.kobj.name(%s)\n", pdev->dev.kobj.name);
	DBG_MSG("dev->bus.name(%s) pdev.dev->bus.dev_name(%s)\n",
		pdev->dev.bus->name, pdev->dev.bus->dev_name);

	DBG_MSG("Device dma_coherent(%d)\n", pdev->dev.archdata.dma_coherent);

	DBG_MSG("pp2_ver(%d)\n", priv->pp2_version);
	DBG_MSG("queue_mode(%d)\n", priv->pp2_cfg.queue_mode);
	DBG_MSG("first_bm_pool(%d) jumbo_pool(%d)\n",
		priv->pp2_cfg.first_bm_pool, priv->pp2_cfg.jumbo_pool);
	DBG_MSG("cell_index(%d) num_ports(%d)\n",
		priv->pp2_cfg.cell_index, priv->num_ports);
#ifdef CONFIG_64BIT
	DBG_MSG("skb_base_addr(%p)\n", (void *)priv->pp2xdata->skb_base_addr);
#endif
	DBG_MSG("hw->base(%p)\n", priv->hw.base);
	if (priv->pp2_version == PPV22) {
		DBG_MSG("gop_addr: gmac(%p) xlg(%p) serdes(%p)\n",
			priv->hw.gop.gop_110.gmac.base,
			priv->hw.gop.gop_110.xlg_mac.base,
			priv->hw.gop.gop_110.serdes.base);
		DBG_MSG("gop_addr: xmib(%p) smi(%p) xsmi(%p)\n",
			priv->hw.gop.gop_110.xmib.base,
			priv->hw.gop.gop_110.smi_base,
			priv->hw.gop.gop_110.xsmi_base);
		DBG_MSG("gop_addr: mspg(%p) xpcs(%p) ptp(%p)\n",
			priv->hw.gop.gop_110.mspg_base,
			priv->hw.gop.gop_110.xpcs_base,
			priv->hw.gop.gop_110.ptp.base);
		DBG_MSG("gop_addr: rfu1(%p)\n",
			priv->hw.gop.gop_110.rfu1_base);
	}
}
EXPORT_SYMBOL(mv_pp2x_pp2_basic_print);

void mv_pp2x_pp2_port_print(struct mv_pp2x_port *port)
{
	int i;

	DBG_MSG("%s port_id(%d)\n", __func__, port->id);
	DBG_MSG("\t ifname(%s)\n", port->dev->name);
	DBG_MSG("\t first_rxq(%d)\n", port->first_rxq);
	DBG_MSG("\t num_irqs(%d)\n", port->num_irqs);
	for (i = 0; i < port->num_irqs; i++)
		DBG_MSG("\t\t irq%d(%d)\n", i, port->of_irqs[i]);
	DBG_MSG("\t pkt_size(%d)\n", port->pkt_size);
	DBG_MSG("\t flags(%lx)\n", port->flags);
	DBG_MSG("\t tx_ring_size(%d)\n", port->tx_ring_size);
	DBG_MSG("\t rx_ring_size(%d)\n", port->rx_ring_size);
	DBG_MSG("\t time_coal(%d)\n", port->tx_time_coal);
	DBG_MSG("\t pool_long(%p)\n", port->pool_long);
	DBG_MSG("\t pool_short(%p)\n", port->pool_short);
	DBG_MSG("\t first_rxq(%d)\n", port->first_rxq);
	DBG_MSG("\t num_rx_queues(%d)\n", port->num_rx_queues);
	DBG_MSG("\t num_tx_queues(%d)\n", port->num_tx_queues);
	DBG_MSG("\t num_qvector(%d)\n", port->num_qvector);

	for (i = 0; i < port->num_qvector; i++) {
		DBG_MSG("\t qvector_index(%d)\n", i);
#if !defined(CONFIG_MV_PP2_POLLING)
		DBG_MSG("\t\t irq(%d) irq_name:%s\n",
			port->q_vector[i].irq, port->q_vector[i].irq_name);
#endif
		DBG_MSG("\t\t qv_type(%d)\n",
			port->q_vector[i].qv_type);
		DBG_MSG("\t\t sw_thread_id	(%d)\n",
			port->q_vector[i].sw_thread_id);
		DBG_MSG("\t\t sw_thread_mask(%d)\n",
			port->q_vector[i].sw_thread_mask);
		DBG_MSG("\t\t first_rx_queue(%d)\n",
			port->q_vector[i].first_rx_queue);
		DBG_MSG("\t\t num_rx_queues(%d)\n",
			port->q_vector[i].num_rx_queues);
		DBG_MSG("\t\t pending_cause_rx(%d)\n",
			port->q_vector[i].pending_cause_rx);
	}
	DBG_MSG("\t GOP ind(%d) phy_mode(%d) phy_addr(%d)\n",
		port->mac_data.gop_index, port->mac_data.phy_mode,
		port->mac_data.phy_addr);
	DBG_MSG("\t GOP force_link(%d) autoneg(%d) duplex(%d) speed(%d)\n",
		port->mac_data.force_link, port->mac_data.autoneg,
		port->mac_data.duplex, port->mac_data.speed);
#if !defined(CONFIG_MV_PP2_POLLING)
	DBG_MSG("\t GOP link_irq(%d) irq_name:%s\n", port->mac_data.link_irq,
		port->mac_data.irq_name);
#endif
	DBG_MSG("\t GOP phy_dev(%p) phy_node(%p)\n", port->mac_data.phy_dev,
		port->mac_data.phy_node);

}
EXPORT_SYMBOL(mv_pp2x_pp2_port_print);

void mv_pp2x_pp2_ports_print(struct mv_pp2x *priv)
{
	int i;
	struct mv_pp2x_port *port;

	for (i = 0; i < priv->num_ports; i++) {
		if (priv->port_list[i] == NULL) {
			pr_emerg("\t port_list[%d]= NULL!\n", i);
			continue;
		}
		port = priv->port_list[i];
		mv_pp2x_pp2_port_print(port);
	}
}
EXPORT_SYMBOL(mv_pp2x_pp2_ports_print);

static int mv_pp2x_platform_data_get(struct platform_device *pdev,
		struct mv_pp2x *priv,	u32 *cell_index, int *port_count)
{
	struct mv_pp2x_hw *hw = &priv->hw;
	static int auto_cell_index;
	static bool cell_index_dts_flag;
	const struct of_device_id *match;
	struct device_node *dn = pdev->dev.of_node;
	struct resource *res;
	resource_size_t mspg_base, mspg_end;
	u32	err;

	match = of_match_node(mv_pp2x_match_tbl, dn);
	if (!match)
		return -ENODEV;

	priv->pp2xdata = (struct mv_pp2x_platform_data *) match->data;

	if (of_property_read_u32(dn, "cell-index", cell_index)) {
		*cell_index = auto_cell_index;
		auto_cell_index++;
	}

	else
		cell_index_dts_flag = true;

	if (auto_cell_index && cell_index_dts_flag)
		return -ENXIO;

	MVPP2_PRINT_VAR(*cell_index);

	/* PPV2 Address Space */
	res = platform_get_resource_byname(pdev, IORESOURCE_MEM, "pp");
	hw->base = devm_ioremap_resource(&pdev->dev, res);
	if (IS_ERR(hw->base))
		return PTR_ERR(hw->base);
	MVPP2_PRINT_2LINE();

	if (priv->pp2xdata->pp2x_ver == PPV21) {
		res = platform_get_resource_byname(pdev,
			IORESOURCE_MEM, "lms");
		hw->lms_base = devm_ioremap_resource(&pdev->dev, res);
		if (IS_ERR(hw->lms_base))
			return PTR_ERR(hw->lms_base);
	} else {
		/* serdes */
		res = platform_get_resource_byname(pdev,
			IORESOURCE_MEM, "serdes");
		hw->gop.gop_110.serdes.base =
			devm_ioremap_resource(&pdev->dev, res);
		if (IS_ERR(hw->gop.gop_110.serdes.base))
			return PTR_ERR(hw->gop.gop_110.serdes.base);
		hw->gop.gop_110.serdes.obj_size = 0x1000;

		MVPP2_PRINT_2LINE();

		/* xmib */
		res = platform_get_resource_byname(pdev,
			IORESOURCE_MEM, "xmib");
		hw->gop.gop_110.xmib.base =
			devm_ioremap_resource(&pdev->dev, res);
		if (IS_ERR(hw->gop.gop_110.xmib.base))
			return PTR_ERR(hw->gop.gop_110.xmib.base);
		hw->gop.gop_110.xmib.obj_size = 0x0100;

		MVPP2_PRINT_2LINE();

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

		MVPP2_PRINT_2LINE();

		/* skipped tai */

		/* xsmi  */
		res = platform_get_resource_byname(pdev,
			IORESOURCE_MEM, "xsmi");
		hw->gop.gop_110.xsmi_base =
			devm_ioremap_resource(&pdev->dev, res);
		if (IS_ERR(hw->gop.gop_110.xsmi_base))
			return PTR_ERR(hw->gop.gop_110.xsmi_base);

		MVPP2_PRINT_2LINE();

		/* MSPG - base register */
		res = platform_get_resource_byname(pdev,
			IORESOURCE_MEM, "mspg");
		hw->gop.gop_110.mspg_base =
			devm_ioremap_resource(&pdev->dev, res);
		if (IS_ERR(hw->gop.gop_110.mspg_base))
			return PTR_ERR(hw->gop.gop_110.mspg_base);
		mspg_base = res->start;
		mspg_end  = res->end;

		MVPP2_PRINT_2LINE();

		/* xpcs */
		res = platform_get_resource_byname(pdev,
			IORESOURCE_MEM, "xpcs");
		if ((res->start <= mspg_base) || (res->end >= mspg_end))
			return -ENXIO;
		hw->gop.gop_110.xpcs_base =
			(void *)(hw->gop.gop_110.mspg_base +
				(res->start-mspg_base));

		MVPP2_PRINT_2LINE();

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
			(res->start-mspg_base));
		hw->gop.gop_110.gmac.obj_size = 0x1000;

		MVPP2_PRINT_2LINE();

		/* MSPG - xlg */
		res = platform_get_resource_byname(pdev,
			IORESOURCE_MEM, "xlg");
		if ((res->start <= mspg_base) || (res->end >= mspg_end))
			return -ENXIO;
		hw->gop.gop_110.xlg_mac.base =
			(void *)(hw->gop.gop_110.mspg_base +
			(res->start-mspg_base));
		hw->gop.gop_110.xlg_mac.obj_size = 0x1000;

		MVPP2_PRINT_2LINE();
	}

	hw->gop_core_clk = devm_clk_get(&pdev->dev, "gop_core_clk");
	if (IS_ERR(hw->gop_core_clk))
		return PTR_ERR(hw->gop_core_clk);
	err = clk_prepare_enable(hw->gop_core_clk);
	if (err < 0)
		return err;

	hw->gop_clk = devm_clk_get(&pdev->dev, "gop_clk");
	if (IS_ERR(hw->gop_clk))
		return PTR_ERR(hw->gop_clk);
	err = clk_prepare_enable(hw->gop_clk);
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
	hw->pp_clk = devm_clk_get(&pdev->dev, "pp_clk");
	if (IS_ERR(hw->pp_clk))
		return PTR_ERR(hw->pp_clk);
	err = clk_prepare_enable(hw->pp_clk);
	if (err < 0)
		return err;

	/* Get system's tclk rate */
	hw->tclk = clk_get_rate(hw->pp_clk);
	MVPP2_PRINT_VAR(hw->tclk);

	*port_count = of_get_available_child_count(dn);
	MVPP2_PRINT_VAR(*port_count);
	if (*port_count == 0) {
		dev_err(&pdev->dev, "no ports enabled\n");
		err = -ENODEV;
	}
	return 0;
}

static int mv_pp2x_probe(struct platform_device *pdev)
{
	struct mv_pp2x *priv;
	struct mv_pp2x_hw *hw;
	int port_count = 0, cpu;
	int i, err;
	u16 cpu_map;
	u32 cell_index = 0;
	u32 net_comp_config;
	struct device_node *dn = pdev->dev.of_node;
	struct device_node *port_node;

	priv = devm_kzalloc(&pdev->dev, sizeof(struct mv_pp2x), GFP_KERNEL);
	if (!priv)
		return -ENOMEM;
	hw = &priv->hw;

	err = mv_pp2x_platform_data_get(pdev, priv, &cell_index, &port_count);
	if (err) {
		pr_crit("mvpp2: platform_data get failed\n");
		goto err_clk;
	}
	MVPP2_PRINT_2LINE();

	priv->pp2_version = priv->pp2xdata->pp2x_ver;

	/* DMA Configruation */
	err = dma_set_mask_and_coherent(&pdev->dev, DMA_BIT_MASK(32));
	if (err) {
		pr_crit("mvpp2: cannot set dma_mask\n");
		goto err_clk;
	}

#ifdef CONFIG_64BIT
{
	/* Set skb_base_address (MSB bits) */
	struct sk_buff *skb;

	if (priv->pp2xdata->skb_base_addr == 0) {
		skb = alloc_skb(MVPP2_SKB_TEST_SIZE, GFP_KERNEL);
		MVPP2_PRINT_VAR(skb);
		if (!skb) {
			err = ENOMEM;
			goto err_clk;
		}
		priv->pp2xdata->skb_base_addr =
			(uintptr_t)skb & ~(priv->pp2xdata->skb_base_mask);
		kfree_skb(skb);
	}
}
#endif

	/* Save cpu_present_mask + populate the per_cpu address space */
	cpu_map = 0;
	i = 0;

	for_each_online_cpu(cpu) {
		cpu_map |= (1<<cpu);
		hw->cpu_base[cpu] = hw->base;
		if (priv->pp2xdata->multi_addr_space) {
			hw->cpu_base[cpu] +=
				(first_addr_space + i)*MVPP2_ADDR_SPACE_SIZE;
			i++;
		}
	}
	priv->cpu_map = cpu_map;

	/*Init PP2 Configuration */
	mv_pp2x_init_config(&priv->pp2_cfg, cell_index);

	/* Initialize network controller */
	err = mv_pp2x_init(pdev, priv);
	if (err < 0) {
		dev_err(&pdev->dev, "failed to initialize controller\n");
		goto err_clk;
	}

	/* smi init */
	if (priv->pp2_version == PPV21)
		mv_gop110_smi_init(&hw->gop);

	priv->port_list = devm_kcalloc(&pdev->dev, port_count,
				      sizeof(struct mv_pp2x_port *),
				      GFP_KERNEL);
	if (!priv->port_list) {
		err = -ENOMEM;
		goto err_clk;
	}

	/* Init PP22 rxfhindir table evenly in probe */
	mv_pp2x_init_rxfhindir(priv);

	/* Initialize ports */
	for_each_available_child_of_node(dn, port_node) {
		err = mv_pp2x_port_probe(pdev, port_node, priv);
		if (err < 0)
			goto err_clk;
	}
	net_comp_config = mvp_pp2x_gop110_netc_cfg_create(priv);
	mv_gop110_netc_init(&priv->hw.gop, net_comp_config,
				MV_NETC_FIRST_PHASE);
	mv_gop110_netc_init(&priv->hw.gop, net_comp_config,
				MV_NETC_SECOND_PHASE);

#if defined(CONFIG_MV_PP2_POLLING)
	init_timer(&cpu_poll_timer);
	cpu_poll_timer.function = mv_pp22_cpu_timer_callback;
	cpu_poll_timer.data     = (unsigned long)pdev;
#endif

	platform_set_drvdata(pdev, priv);
	pr_debug("Platform Device Name : %s\n", kobject_name(&pdev->dev.kobj));
	return 0;

err_clk:
	clk_disable_unprepare(hw->gop_clk);
	clk_disable_unprepare(hw->pp_clk);
	clk_disable_unprepare(hw->gop_clk);
	clk_disable_unprepare(hw->gop_core_clk);
	clk_disable_unprepare(hw->mg_clk);
	clk_disable_unprepare(hw->mg_core_clk);
	return err;
}

static int mv_pp2x_remove(struct platform_device *pdev)
{
	struct mv_pp2x *priv = platform_get_drvdata(pdev);
	struct mv_pp2x_hw *hw = &priv->hw;
	int i;

	for (i = 0; i < priv->num_ports; i++) {
		if (priv->port_list[i])
			mv_pp2x_port_remove(priv->port_list[i]);
		priv->num_ports--;
	}

	for (i = 0; i < priv->num_pools; i++) {
		struct mv_pp2x_bm_pool *bm_pool = &priv->bm_pools[i];

		mv_pp2x_bm_pool_destroy(&pdev->dev, priv, bm_pool, true);
	}

	for_each_online_cpu(i) {
		struct mv_pp2x_aggr_tx_queue *aggr_txq = &priv->aggr_txqs[i];

		dma_free_coherent(&pdev->dev,
				  MVPP2_DESCQ_MEM_SIZE(aggr_txq->size),
				  aggr_txq->desc_mem,
				  aggr_txq->descs_phys);
	}

	clk_disable_unprepare(hw->pp_clk);
	clk_disable_unprepare(hw->gop_clk);

	return 0;
}

MODULE_DEVICE_TABLE(of, mv_pp2x_match_tbl);

static struct platform_driver mv_pp2x_driver = {
	.probe = mv_pp2x_probe,
	.remove = mv_pp2x_remove,
	.driver = {
		.name = MVPP2_DRIVER_NAME,
		.of_match_table = mv_pp2x_match_tbl,
	},
};

static int mv_pp2x_rxq_number_get(void)
{
	int rx_queue_num;

	if (mv_pp2x_queue_mode == MVPP2_QDIST_SINGLE_MODE)
		rx_queue_num = mv_pp2x_num_cos_queues;
	else
		rx_queue_num = mv_pp2x_num_cos_queues * num_active_cpus();

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

#if defined(CONFIG_MV_PP2_POLLING)
static void mv_pp22_cpu_timer_callback(unsigned long data)
{
	struct platform_device *pdev = (struct platform_device *)data;
	struct mv_pp2x *priv = platform_get_drvdata(pdev);
	int i = 0, j, err;
	struct mv_pp2x_port *port;
	u32 timeout;

	/* Check link_change for initialized ports */
	for (i = 0 ; i < priv->num_ports; i++) {
		port = priv->port_list[i];
		if (port && port->link_change_tasklet.func)
			tasklet_schedule(&port->link_change_tasklet);
	}

	/* Schedule napi for ports with link_up. */
	for (i = 0 ; i < priv->num_ports; i++) {
		port = priv->port_list[i];
		if (port && netif_carrier_ok(port->dev)) {
			for (j = 0 ; j < num_active_cpus(); j++) {
				if (j == smp_processor_id()) {
					napi_schedule(&port->q_vector[j].napi);
				} else {
					err = smp_call_function_single(j,
						(smp_call_func_t)napi_schedule,
						&port->q_vector[j].napi, 1);
					if (err)
						pr_crit("napi_schedule error: %s\n",
							__func__);
				}
			}
		} else if (port) {
			pr_debug("mvpp2(%d): port=%p netif_carrier_ok=%d\n",
				__LINE__, port,
				netif_carrier_ok(port->dev));
		} else
			pr_debug("mvpp2(%d): mv_pp22_cpu_timer_callback. PORT NULL !!!!\n",
				__LINE__);
	}

	timeout = MV_PP2_FPGA_PERODIC_TIME;
	mod_timer(&cpu_poll_timer, jiffies + msecs_to_jiffies(timeout));
}

#endif
module_init(mpp2_module_init);
module_exit(mpp2_module_exit);

MODULE_DESCRIPTION("Marvell PPv2x Ethernet Driver - www.marvell.com");
MODULE_AUTHOR("Marvell");
MODULE_LICENSE("GPL v2");
