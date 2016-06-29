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

#ifndef _MVPP2_HW_H_
#define _MVPP2_HW_H_

#include <linux/kernel.h>
#include <linux/skbuff.h>
#include <linux/printk.h>

#include <linux/platform_device.h>

#if 1
void mv_pp2x_write(struct mv_pp2x_hw *hw, u32 offset, u32 data);
u32 mv_pp2x_read(struct mv_pp2x_hw *hw, u32 offset);

#else
static inline void mv_pp2x_write(struct mv_pp2x_hw *hw, u32 offset, u32 data)
{
#if 1
	if (smp_processor_id() != 0)
		pr_emerg_once("Received mv_pp2x_write(%d) from CPU1 !!\n",
			offset);
#endif
	pr_debug("mv_pp2x_write(%p)\n", hw->cpu_base[smp_processor_id()] +
		offset);
	writel(data, hw->cpu_base[smp_processor_id()] + offset);
}

static inline u32 mv_pp2x_read(struct mv_pp2x_hw *hw, u32 offset)
{
#if 1
	if (smp_processor_id() != 0)
		pr_emerg_once("Received mv_pp2x_read(%d) from CPU1 !!\n",
			offset);
#endif
	pr_debug("mv_pp2x_read(%p)\n", hw->cpu_base[smp_processor_id()] +
		offset);

	return readl(hw->cpu_base[smp_processor_id()] + offset);
}
#endif

void mv_pp2x_relaxed_write(struct mv_pp2x_hw *hw, u32 offset, u32 data);
u32 mv_pp2x_relaxed_read(struct mv_pp2x_hw *hw, u32 offset);



static inline void mv_pp22_thread_write(struct mv_pp2x_hw *hw, u32 sw_thread,
					     u32 offset, u32 data)
{
	writel(data, hw->base + sw_thread*MVPP2_ADDR_SPACE_SIZE + offset);
}

static inline u32 mv_pp22_thread_read(struct mv_pp2x_hw *hw, u32 sw_thread,
					    u32 offset)
{
	return readl(hw->base + sw_thread*MVPP2_ADDR_SPACE_SIZE + offset);
}

static inline void mv_pp22_thread_relaxed_write(struct mv_pp2x_hw *hw,
						u32 sw_thread,
						u32 offset, u32 data)
{
	writel_relaxed(data, hw->base + sw_thread*MVPP2_ADDR_SPACE_SIZE + offset);
}

static inline u32 mv_pp22_thread_relaxed_read(struct mv_pp2x_hw *hw,
					      u32 sw_thread,
					      u32 offset)
{
	return readl_relaxed(hw->base + sw_thread*MVPP2_ADDR_SPACE_SIZE + offset);
}

static inline void mv_pp21_isr_rx_group_write(struct mv_pp2x_hw *hw, int port,
						    int num_rx_queues)
{
	mv_pp2x_write(hw, MVPP21_ISR_RXQ_GROUP_REG(port), num_rx_queues);
}

static inline void mv_pp22_isr_rx_group_write(struct mv_pp2x_hw *hw, int port,
						    int sub_group,
						    int start_queue,
						    int num_rx_queues)
{
	int val;

	val = (port << MVPP22_ISR_RXQ_GROUP_INDEX_GROUP_OFFSET) | sub_group;
	mv_pp2x_write(hw, MVPP22_ISR_RXQ_GROUP_INDEX_REG, val);
	val = (num_rx_queues << MVPP22_ISR_RXQ_SUB_GROUP_SIZE_OFFSET) |
		start_queue;
	mv_pp2x_write(hw, MVPP22_ISR_RXQ_SUB_GROUP_CONFIG_REG, val);

}

/* Get number of physical egress port */
static inline int mv_pp2x_egress_port(struct mv_pp2x_port *port)
{
	return MVPP2_MAX_TCONT + port->id;
}

/* Get number of physical TXQ */
static inline int mv_pp2x_txq_phys(int port, int txq)
{
	return (MVPP2_MAX_TCONT + port) * MVPP2_MAX_TXQ + txq;
}

/* Rx descriptors helper methods */

/* Get number of Rx descriptors occupied by received packets */
static inline int mv_pp2x_rxq_received(struct mv_pp2x_port *port, int rxq_id)
{
	u32 val = mv_pp2x_relaxed_read(&port->priv->hw, MVPP2_RXQ_STATUS_REG(rxq_id));

	return val & MVPP2_RXQ_OCCUPIED_MASK;
}

/* Get number of Rx descriptors occupied by received packets */
static inline int mv_pp2x_rxq_free(struct mv_pp2x_port *port, int rxq_id)
{
	u32 val = mv_pp2x_read(&port->priv->hw, MVPP2_RXQ_STATUS_REG(rxq_id));

	return (val & (MVPP2_RXQ_NON_OCCUPIED_MASK >>
		MVPP2_RXQ_NON_OCCUPIED_OFFSET));
}

/* Update Rx queue status with the number of occupied and available
 * Rx descriptor slots.
 */
static inline void mv_pp2x_rxq_status_update(struct mv_pp2x_port *port,
						    int rxq_id,
						    int used_count,
						    int free_count)
{
	/* Decrement the number of used descriptors and increment count
	 * increment the number of free descriptors.
	 */
	u32 val = used_count | (free_count << MVPP2_RXQ_NUM_NEW_OFFSET);

	mv_pp2x_write(&port->priv->hw,
		      MVPP2_RXQ_STATUS_UPDATE_REG(rxq_id), val);
}

/* Get pointer to next RX descriptor to be processed by SW */
static inline struct mv_pp2x_rx_desc *
mv_pp2x_rxq_next_desc_get(struct mv_pp2x_rx_queue *rxq)
{
	int rx_desc = rxq->next_desc_to_proc;

	rxq->next_desc_to_proc = MVPP2_QUEUE_NEXT_DESC(rxq, rx_desc);
	/* For uneven descriptors, fetch next two descriptors (2*32B) */
	if (rx_desc & 0x1)
		prefetch(rxq->first_desc + rxq->next_desc_to_proc);
	return (rxq->first_desc + rx_desc);
}

/* Mask the current CPU's Rx/Tx interrupts */
static inline void mv_pp2x_interrupts_mask(void *arg)
{
	struct mv_pp2x_port *port = arg;

	mv_pp2x_write(&(port->priv->hw), MVPP2_ISR_RX_TX_MASK_REG(port->id), 0);
}

/* Unmask the current CPU's Rx/Tx interrupts */
static inline void mv_pp2x_interrupts_unmask(void *arg)
{
	struct mv_pp2x_port *port = arg;
	u32 val;

	val = MVPP2_CAUSE_MISC_SUM_MASK | MVPP2_CAUSE_RXQ_OCCUP_DESC_ALL_MASK;
	if (port->priv->pp2xdata->interrupt_tx_done == true)
		val |= MVPP2_CAUSE_TXQ_OCCUP_DESC_ALL_MASK;

	mv_pp2x_write(&(port->priv->hw),
		      MVPP2_ISR_RX_TX_MASK_REG(port->id), val);
}

static inline void mv_pp2x_shared_thread_interrupts_mask(
		struct mv_pp2x_port *port)
{
	struct queue_vector *q_vec = &port->q_vector[0];
	int i;

	if (port->priv->pp2xdata->multi_addr_space == false)
		return;

	for (i = 0; i < port->num_qvector; i++) {
		if (q_vec[i].qv_type == MVPP2_SHARED)
			mv_pp22_thread_write(&port->priv->hw,
					     q_vec[i].sw_thread_id,
					    MVPP2_ISR_RX_TX_MASK_REG(port->id),
					    0);
	}
}

/* Unmask the shared CPU's Rx interrupts */
static inline void mv_pp2x_shared_thread_interrupts_unmask(
		struct mv_pp2x_port *port)
{
	struct queue_vector *q_vec = &port->q_vector[0];
	int i;

	if (port->priv->pp2xdata->multi_addr_space == false)
		return;

	for (i = 0; i < port->num_qvector; i++) {
		if (q_vec[i].qv_type == MVPP2_SHARED)
			mv_pp22_thread_write(&port->priv->hw,
					     q_vec[i].sw_thread_id,
					    MVPP2_ISR_RX_TX_MASK_REG(port->id),
					  MVPP2_CAUSE_RXQ_OCCUP_DESC_ALL_MASK);
	}
}

static inline struct mv_pp2x_rx_queue *mv_pp2x_get_rx_queue(
		struct mv_pp2x_port *port, u32 cause)
{
	int rx_queue = fls(cause) - 1;

	return port->rxqs[rx_queue];
}

static inline struct mv_pp2x_tx_queue *mv_pp2x_get_tx_queue(
		struct mv_pp2x_port *port, u32 cause)
{
	int tx_queue = fls(cause) - 1;

	return port->txqs[tx_queue];
}

static inline u8 *mv_pp2x_bm_virt_addr_get(struct mv_pp2x_hw *hw, u32 pool)
{
	uintptr_t val = 0;

	mv_pp2x_read(hw, MVPP2_BM_PHY_ALLOC_REG(pool));
/*TODO: Validate this is  correct CONFIG_XXX for (sk_buff *),
 * it is a kmem_cache address (YuvalC).
 */
#ifdef CONFIG_PHYS_ADDR_T_64BIT
	val = mv_pp2x_read(hw, MVPP22_BM_PHY_VIRT_HIGH_ALLOC_REG);
	val &= MVPP22_BM_VIRT_HIGH_ALLOC_MASK;
	val <<= (32 - MVPP22_BM_VIRT_HIGH_ALLOC_OFFSET);
#endif
	val |= mv_pp2x_read(hw, MVPP2_BM_VIRT_ALLOC_REG);
	/* TODO: Remove it when 40-bit supported */
	val &= 0xffffffff;

	return((u8 *)val);
}

static inline void mv_pp2x_bm_hw_pool_create(struct mv_pp2x_hw *hw,
						      u32 pool,
						      dma_addr_t pool_addr,
						      int size)
{
	u32 val;

	mv_pp2x_write(hw, MVPP2_BM_POOL_BASE_ADDR_REG(pool),
		      lower_32_bits(pool_addr));
#ifdef CONFIG_ARCH_DMA_ADDR_T_64BIT
	mv_pp2x_write(hw, MVPP22_BM_POOL_BASE_ADDR_HIGH_REG,
		      (upper_32_bits(pool_addr) & MVPP22_ADDR_HIGH_MASK));
#endif
	mv_pp2x_write(hw, MVPP2_BM_POOL_SIZE_REG(pool), size);

	val = mv_pp2x_read(hw, MVPP2_BM_POOL_CTRL_REG(pool));
	val |= MVPP2_BM_START_MASK;
	mv_pp2x_write(hw, MVPP2_BM_POOL_CTRL_REG(pool), val);
}

/* Release buffer to BM */
static inline void mv_pp2x_bm_pool_put(struct mv_pp2x_hw *hw, u32 pool,
					      dma_addr_t buf_phys_addr,
					      u8 *buf_virt_addr)
{

	mv_pp2x_relaxed_write(hw, MVPP2_BM_VIRT_RLS_REG,
			      lower_32_bits((uintptr_t)buf_virt_addr));
	mv_pp2x_relaxed_write(hw, MVPP2_BM_PHY_RLS_REG(pool),
		      lower_32_bits(buf_phys_addr));

}

/* Release multicast buffer */
static inline void mv_pp2x_bm_pool_mc_put(struct mv_pp2x_port *port, int pool,
						   u32 buf_phys_addr,
						   u32 buf_virt_addr,
						   int mc_id)
{
	u32 val = 0;

	val |= (mc_id & MVPP21_BM_MC_ID_MASK);
	mv_pp2x_write(&(port->priv->hw), MVPP21_BM_MC_RLS_REG, val);
	/*TODO : YuvalC, this is just workaround to compile.
	 * Need to handle mv_pp2x_buff_hdr_rx().
	 */
	mv_pp2x_bm_pool_put(&(port->priv->hw), pool,
			    (dma_addr_t)(buf_phys_addr |
			    MVPP2_BM_PHY_RLS_MC_BUFF_MASK),
			    (u8 *)(u64)(buf_virt_addr));
}

static inline void mv_pp2x_port_interrupts_enable(struct mv_pp2x_port *port)
{
	int sw_thread_mask = 0, i;
	struct queue_vector *q_vec = &port->q_vector[0];

	for (i = 0; i < port->num_qvector; i++)
		sw_thread_mask |= q_vec[i].sw_thread_mask;
#if !defined(CONFIG_MV_PP2_POLLING)
	mv_pp2x_write(&port->priv->hw, MVPP2_ISR_ENABLE_REG(port->id),
		      MVPP2_ISR_ENABLE_INTERRUPT(sw_thread_mask));
#endif
}

static inline void mv_pp2x_port_interrupts_disable(struct mv_pp2x_port *port)
{
	int sw_thread_mask = 0, i;
	struct queue_vector *q_vec = &port->q_vector[0];

	for (i = 0; i < port->num_qvector; i++)
		sw_thread_mask |= q_vec[i].sw_thread_mask;

	mv_pp2x_write(&port->priv->hw, MVPP2_ISR_ENABLE_REG(port->id),
		      MVPP2_ISR_DISABLE_INTERRUPT(sw_thread_mask));
}


static inline void mv_pp2x_qvector_interrupt_enable(struct queue_vector *q_vec)
{
#if !defined(CONFIG_MV_PP2_POLLING)
	struct mv_pp2x_port *port = q_vec->parent;

	mv_pp2x_write(&port->priv->hw, MVPP2_ISR_ENABLE_REG(port->id),
		      MVPP2_ISR_ENABLE_INTERRUPT(q_vec->sw_thread_mask));
#endif
}

static inline void mv_pp2x_qvector_interrupt_disable(struct queue_vector *q_vec)
{
	struct mv_pp2x_port *port = q_vec->parent;

	mv_pp2x_write(&port->priv->hw, MVPP2_ISR_ENABLE_REG(port->id),
		      MVPP2_ISR_DISABLE_INTERRUPT(q_vec->sw_thread_mask));

}

static inline u32 mv_pp2x_qvector_interrupt_state_get(struct queue_vector
						       *q_vec)
{
	struct mv_pp2x_port *port = q_vec->parent;
	u32 state;

	state = mv_pp2x_read(&port->priv->hw, MVPP2_ISR_ENABLE_REG(port->id));
	state &= MVPP2_ISR_ENABLE_INTERRUPT(q_vec->sw_thread_mask);
	return state;
}

static inline int mv_pp2x_txq_sent_desc_proc(struct mv_pp2x_port *port,
					     int sw_thread,
					     u8 txq_id)
{
	u32 val;

	/* Reading status reg resets transmitted descriptor counter */
	if (port->priv->pp2_version == PPV21) {
		val = mv_pp22_thread_relaxed_read(&port->priv->hw,
							sw_thread,
							MVPP21_TXQ_SENT_REG(txq_id));
		return (val & MVPP21_TRANSMITTED_COUNT_MASK) >>
			MVPP21_TRANSMITTED_COUNT_OFFSET;
		}
	else {
		val = mv_pp22_thread_relaxed_read(&port->priv->hw,
						  sw_thread,
						  MVPP22_TXQ_SENT_REG(txq_id));
		return (val & MVPP22_TRANSMITTED_COUNT_MASK) >>
			MVPP22_TRANSMITTED_COUNT_OFFSET;
		}

}

static inline void mv_pp2x_txq_desc_put(struct mv_pp2x_tx_queue *txq)
{
	if (txq->next_desc_to_proc == 0)
		txq->next_desc_to_proc = txq->last_desc - 1;
	else
		txq->next_desc_to_proc--;
}


static inline void mv_pp2x_txq_sent_counter_clear(void *arg)
{
	struct mv_pp2x_port *port = arg;
	int queue;

	for (queue = 0; queue < port->num_tx_queues; queue++) {
		int id = port->txqs[queue]->id;

		if (port->priv->pp2_version == PPV21)
			mv_pp2x_read(&(port->priv->hw),
				     MVPP21_TXQ_SENT_REG(id));
		else
			mv_pp2x_read(&(port->priv->hw),
				     MVPP22_TXQ_SENT_REG(id));
	}
}

static inline u8 *mv_pp21_rxdesc_cookie_get(
		struct mv_pp2x_rx_desc *rx_desc)
{
	return((u8 *)((uintptr_t)rx_desc->u.pp21.buf_cookie));
}

static inline dma_addr_t mv_pp21_rxdesc_phys_addr_get(
		struct mv_pp2x_rx_desc *rx_desc)
{
	return (dma_addr_t)rx_desc->u.pp21.buf_phys_addr;
}

/*YuvalC: Below functions are intended to support both aarch64 & aarch32 */
static inline u8 *mv_pp22_rxdesc_cookie_get(
		struct mv_pp2x_rx_desc *rx_desc)
{
	return((u8 *)((uintptr_t)
		(rx_desc->u.pp22.buf_cookie_bm_qset_cls_info &
		DMA_BIT_MASK(40))));
}

static inline dma_addr_t mv_pp22_rxdesc_phys_addr_get(
		struct mv_pp2x_rx_desc *rx_desc)
{
	return((dma_addr_t)
		(rx_desc->u.pp22.buf_phys_addr_key_hash &
		DMA_BIT_MASK(32)));
}

static inline struct sk_buff *mv_pp21_txdesc_cookie_get(
		struct mv_pp2x_tx_desc *tx_desc)
{
	return((struct sk_buff *)((uintptr_t)tx_desc->u.pp21.buf_cookie));
}

static inline dma_addr_t mv_pp21_txdesc_phys_addr_get(
		struct mv_pp2x_tx_desc *tx_desc)
{
	return (dma_addr_t)tx_desc->u.pp21.buf_phys_addr;
}

static inline struct sk_buff *mv_pp22_txdesc_cookie_get(
		struct mv_pp2x_tx_desc *tx_desc)
{
	return((struct sk_buff *)((uintptr_t)
		(tx_desc->u.pp22.buf_cookie_bm_qset_hw_cmd3 &
		DMA_BIT_MASK(40))));
}

static inline dma_addr_t mv_pp22_txdesc_phys_addr_get(
		struct mv_pp2x_tx_desc *tx_desc)
{
	return((dma_addr_t)
		(tx_desc->u.pp22.buf_phys_addr_hw_cmd2 & DMA_BIT_MASK(40)));
}

static inline dma_addr_t mv_pp2x_txdesc_phys_addr_get(
	enum mvppv2_version pp2_ver, struct mv_pp2x_tx_desc *tx_desc)
{
	if (pp2_ver == PPV21)
		return mv_pp21_txdesc_phys_addr_get(tx_desc);

	return mv_pp22_txdesc_phys_addr_get(tx_desc);
}


static inline void mv_pp21_txdesc_phys_addr_set(dma_addr_t phys_addr,
	struct mv_pp2x_tx_desc *tx_desc)
{
	tx_desc->u.pp21.buf_phys_addr = phys_addr;
}

static inline void mv_pp22_txdesc_phys_addr_set(dma_addr_t phys_addr,
	struct mv_pp2x_tx_desc *tx_desc)
{
	u64 *buf_phys_addr_p = &tx_desc->u.pp22.buf_phys_addr_hw_cmd2;

#ifdef CONFIG_ARCH_DMA_ADDR_T_64BIT
	*buf_phys_addr_p &= ~(DMA_BIT_MASK(40));
	*buf_phys_addr_p |= phys_addr & DMA_BIT_MASK(40);
#else
	*((dma_addr_t *)buf_phys_addr_p) = phys_addr;
	*((u8 *)buf_phys_addr_p + sizeof(dma_addr_t)) = 0; /*5th byte*/

	/*pr_crit("phys_addr=%x, buf_phys_addr_hw_cmd2=%d\n",
	* phys_addr, tx_desc->u.pp22.buf_phys_addr_hw_cmd2);
	*/
#endif
}

static inline void mv_pp2x_txdesc_phys_addr_set(enum mvppv2_version pp2_ver,
	dma_addr_t phys_addr, struct mv_pp2x_tx_desc *tx_desc)
{
	if (pp2_ver == PPV21)
		mv_pp21_txdesc_phys_addr_set(phys_addr, tx_desc);
	else
		mv_pp22_txdesc_phys_addr_set(phys_addr, tx_desc);
}

int mv_pp2x_ptr_validate(const void *ptr);
int mv_pp2x_range_validate(int value, int min, int max);

int mv_pp2x_prs_hw_read(struct mv_pp2x_hw *hw, struct mv_pp2x_prs_entry *pe);

int mv_pp2x_prs_default_init(struct platform_device *pdev,
				  struct mv_pp2x_hw *hw);
void mv_pp2x_prs_mac_promisc_set(struct mv_pp2x_hw *hw, int port, bool add);
void mv_pp2x_prs_mac_multi_set(struct mv_pp2x_hw *hw, int port, int index,
				      bool add);
int mv_pp2x_prs_mac_da_accept(struct mv_pp2x_hw *hw, int port,
				      const u8 *da, bool add);
int mv_pp2x_prs_def_flow(struct mv_pp2x_port *port);
int mv_pp2x_prs_flow_set(struct mv_pp2x_port *port);
void mv_pp2x_prs_mcast_del_all(struct mv_pp2x_hw *hw, int port);
int mv_pp2x_prs_tag_mode_set(struct mv_pp2x_hw *hw, int port, int type);
int mv_pp2x_prs_update_mac_da(struct net_device *dev, const u8 *da);
void mv_pp2x_prs_flow_id_attr_init(void);
int mv_pp2x_prs_flow_id_attr_get(int flow_id);

int mv_pp2x_cls_init(struct platform_device *pdev, struct mv_pp2x_hw *hw);
void mv_pp2x_cls_port_config(struct mv_pp2x_port *port);
void mv_pp2x_cls_config(struct mv_pp2x_hw *hw);
void mv_pp2x_cls_oversize_rxq_set(struct mv_pp2x_port *port);
void mv_pp2x_cls_lookup_read(struct mv_pp2x_hw *hw, int lkpid, int way,
				   struct mv_pp2x_cls_lookup_entry *le);
void mv_pp2x_cls_flow_tbl_temp_copy(struct mv_pp2x_hw *hw, int lkpid,
					    int *temp_flow_idx);
void mv_pp2x_cls_lkp_flow_set(struct mv_pp2x_hw *hw, int lkpid, int way,
				    int flow_idx);
void mv_pp2x_cls_flow_port_add(struct mv_pp2x_hw *hw, int index, int port_id);
void mv_pp2x_cls_flow_port_del(struct mv_pp2x_hw *hw, int index, int port_id);

void mv_pp2x_txp_max_tx_size_set(struct mv_pp2x_port *port);
void mv_pp2x_tx_done_time_coal_set(struct mv_pp2x_port *port, u32 usec);
void mv_pp21_gmac_max_rx_size_set(struct mv_pp2x_port *port);

int mv_pp2x_txq_pend_desc_num_get(struct mv_pp2x_port *port,
					    struct mv_pp2x_tx_queue *txq);
u32 mv_pp2x_txq_desc_csum(int l3_offs, int l3_proto,
				  int ip_hdr_len, int l4_proto);
struct mv_pp2x_tx_desc *mv_pp2x_txq_next_desc_get(
		struct mv_pp2x_aggr_tx_queue *aggr_txq);
int mv_pp2x_txq_alloc_reserved_desc(struct mv_pp2x *priv,
					    struct mv_pp2x_tx_queue *txq,
					    int num);
void mv_pp2x_aggr_txq_pend_desc_add(struct mv_pp2x_port *port, int pending);
int mv_pp2x_aggr_desc_num_read(struct mv_pp2x *priv, int cpu);
int mv_pp2x_aggr_desc_num_check(struct mv_pp2x *priv,
					struct mv_pp2x_aggr_tx_queue *aggr_txq,
					int num);
void mv_pp2x_rxq_offset_set(struct mv_pp2x_port *port,
				 int prxq, int offset);
void mv_pp2x_bm_pool_bufsize_set(struct mv_pp2x_hw *hw,
					 struct mv_pp2x_bm_pool *bm_pool,
					 int buf_size);
void mv_pp2x_pool_refill(struct mv_pp2x *priv, u32 pool,
			    dma_addr_t phys_addr, u8 *cookie);

void mv_pp21_rxq_long_pool_set(struct mv_pp2x_hw *hw,
				     int prxq, int long_pool);
void mv_pp21_rxq_short_pool_set(struct mv_pp2x_hw *hw,
				     int prxq, int short_pool);

void mv_pp22_rxq_long_pool_set(struct mv_pp2x_hw *hw,
				     int prxq, int long_pool);
void mv_pp22_rxq_short_pool_set(struct mv_pp2x_hw *hw,
				     int prxq, int short_pool);

void mv_pp21_port_mii_set(struct mv_pp2x_port *port);
void mv_pp21_port_fc_adv_enable(struct mv_pp2x_port *port);
void mv_pp21_port_enable(struct mv_pp2x_port *port);
void mv_pp21_port_disable(struct mv_pp2x_port *port);

void mv_pp2x_ingress_enable(struct mv_pp2x_port *port);
void mv_pp2x_ingress_disable(struct mv_pp2x_port *port);
void mv_pp2x_egress_enable(struct mv_pp2x_port *port);
void mv_pp2x_egress_disable(struct mv_pp2x_port *port);

void mv_pp21_port_periodic_xon_disable(struct mv_pp2x_port *port);
void mv_pp21_port_loopback_set(struct mv_pp2x_port *port);
void mv_pp21_port_reset(struct mv_pp2x_port *port);

void mv_pp2x_rx_pkts_coal_set(struct mv_pp2x_port *port,
				    struct mv_pp2x_rx_queue *rxq, u32 pkts);
void mv_pp2x_rx_time_coal_set(struct mv_pp2x_port *port,
				   struct mv_pp2x_rx_queue *rxq, u32 usec);
void mv_pp2x_tx_done_pkts_coal_set(void *arg);
void mv_pp2x_cause_error(struct net_device *dev, int cause);
void mv_pp2x_rx_error(struct mv_pp2x_port *port,
			  struct mv_pp2x_rx_desc *rx_desc);
void mv_pp2x_rx_csum(struct mv_pp2x_port *port, u32 status,
			   struct sk_buff *skb);
void mv_pp21_get_mac_address(struct mv_pp2x_port *port, unsigned char *addr);

int mv_pp2x_c2_init(struct platform_device *pdev, struct mv_pp2x_hw *hw);

int mv_pp2x_prs_sw_sram_shift_set(struct mv_pp2x_prs_entry *pe, int shift,
					  unsigned int op);
int mv_pp2x_prs_sw_sram_shift_get(struct mv_pp2x_prs_entry *pe, int *shift);
int mv_pp2x_prs_sw_sram_next_lu_get(struct mv_pp2x_prs_entry *pe,
					     unsigned int *lu);
int mv_pp2x_prs_sram_bit_get(struct mv_pp2x_prs_entry *pe, int bitNum,
				   unsigned int *bit);
int mv_pp2x_prs_sw_sram_lu_done_get(struct mv_pp2x_prs_entry *pe,
					      unsigned int *bit);
int mv_pp2x_prs_sw_sram_flowid_gen_get(struct mv_pp2x_prs_entry *pe,
						 unsigned int *bit);
int mv_pp2x_prs_sw_sram_ri_get(struct mv_pp2x_prs_entry *pe,
				       unsigned int *bits,
				       unsigned int *enable);
int mv_pp2x_prs_sw_sram_ai_get(struct mv_pp2x_prs_entry *pe,
				       unsigned int *bits,
				       unsigned int *enable);
int mv_pp2x_prs_sw_sram_offset_set(struct mv_pp2x_prs_entry *pe,
					   unsigned int type,
					   int offset, unsigned int op);
int mv_pp2x_prs_sw_sram_offset_get(struct mv_pp2x_prs_entry *pe,
					   unsigned int *type,
					   int *offset, unsigned int *op);
void mv_pp2x_prs_hw_port_init(struct mv_pp2x_hw *hw, int port,
				    int lu_first, int lu_max, int offset);
void mv_pp2x_prs_sw_clear(struct mv_pp2x_prs_entry *pe);
void mv_pp2x_prs_hw_inv(struct mv_pp2x_hw *hw, int index);
void mv_pp2x_prs_tcam_lu_set(struct mv_pp2x_prs_entry *pe, unsigned int lu);
void mv_pp2x_prs_tcam_port_set(struct mv_pp2x_prs_entry *pe,
				      unsigned int port, bool add);
void mv_pp2x_prs_tcam_port_map_set(struct mv_pp2x_prs_entry *pe,
					    unsigned int ports);
void mv_pp2x_prs_tcam_data_byte_set(struct mv_pp2x_prs_entry *pe,
					    unsigned int offs,
					    unsigned char byte,
					    unsigned char enable);
void mv_pp2x_prs_tcam_ai_update(struct mv_pp2x_prs_entry *pe,
					unsigned int bits,
					unsigned int enable);
void mv_pp2x_prs_sram_ri_update(struct mv_pp2x_prs_entry *pe,
					unsigned int bits, unsigned int mask);
void mv_pp2x_prs_sram_ai_update(struct mv_pp2x_prs_entry *pe,
					unsigned int bits, unsigned int mask);
void mv_pp2x_prs_sram_next_lu_set(struct mv_pp2x_prs_entry *pe,
					unsigned int lu);
void mv_pp2x_prs_sw_sram_lu_done_set(struct mv_pp2x_prs_entry *pe);
void mv_pp2x_prs_sw_sram_lu_done_clear(struct mv_pp2x_prs_entry *pe);
void mv_pp2x_prs_sw_sram_flowid_set(struct mv_pp2x_prs_entry *pe);
void mv_pp2x_prs_sw_sram_flowid_clear(struct mv_pp2x_prs_entry *pe);
int mv_pp2x_prs_hw_write(struct mv_pp2x_hw *hw, struct mv_pp2x_prs_entry *pe);
int mv_pp2x_cls_hw_lkp_read(struct mv_pp2x_hw *hw, int lkpid, int way,
				   struct mv_pp2x_cls_lookup_entry *fe);
int mv_pp2x_cls_hw_lkp_write(struct mv_pp2x_hw *hw, int lkpid, int way,
				   struct mv_pp2x_cls_lookup_entry *fe);
int mv_pp2x_cls_lkp_port_way_set(struct mv_pp2x_hw *hw, int port, int way);
int mv_pp2x_cls_hw_lkp_print(struct mv_pp2x_hw *hw, int lkpid, int way);
int mv_pp2x_cls_sw_lkp_rxq_get(struct mv_pp2x_cls_lookup_entry *lkp, int *rxq);
int mv_pp2x_cls_sw_lkp_rxq_set(struct mv_pp2x_cls_lookup_entry *fe, int rxq);
int mv_pp2x_cls_sw_lkp_en_get(struct mv_pp2x_cls_lookup_entry *lkp, int *en);
int mv_pp2x_cls_sw_lkp_en_set(struct mv_pp2x_cls_lookup_entry *lkp, int en);
int mv_pp2x_cls_sw_lkp_flow_get(struct mv_pp2x_cls_lookup_entry *lkp,
				       int *flow_idx);
int mv_pp2x_cls_sw_lkp_flow_set(struct mv_pp2x_cls_lookup_entry *lkp,
				       int flow_idx);
int mv_pp2x_cls_sw_lkp_mod_get(struct mv_pp2x_cls_lookup_entry *lkp,
				       int *mod_base);
int mv_pp2x_cls_sw_lkp_mod_set(struct mv_pp2x_cls_lookup_entry *lkp,
				       int mod_base);
int mv_pp2x_cls_hw_flow_read(struct mv_pp2x_hw *hw, int index,
				    struct mv_pp2x_cls_flow_entry *fe);
int mv_pp2x_cls_sw_flow_dump(struct mv_pp2x_cls_flow_entry *fe);
int mv_pp2x_cls_hw_regs_dump(struct mv_pp2x_hw *hw);
int mv_pp2x_cls_hw_lkp_hit_get(struct mv_pp2x_hw *hw, int lkpid, int way,
				     unsigned int *cnt);
int mv_pp2x_cls_hw_flow_dump(struct mv_pp2x_hw *hw);
int mv_pp2x_cls_hw_flow_hits_dump(struct mv_pp2x_hw *hw);
void mv_pp2x_cls_flow_write(struct mv_pp2x_hw *hw,
				 struct mv_pp2x_cls_flow_entry *fe);
int mv_pp2x_cls_sw_flow_port_set(struct mv_pp2x_cls_flow_entry *fe,
					int type, int portid);
int mv_pp2x_cls_sw_flow_hek_num_set(struct mv_pp2x_cls_flow_entry *fe,
					      int num_of_fields);
int mv_pp2x_cls_sw_flow_hek_set(struct mv_pp2x_cls_flow_entry *fe,
					int field_index, int field_id);
int mv_pp2x_cls_sw_flow_portid_select(struct mv_pp2x_cls_flow_entry *fe,
					     int from);
int mv_pp2x_cls_sw_flow_pppoe_set(struct mv_pp2x_cls_flow_entry *fe, int mode);
int mv_pp2x_cls_sw_flow_vlan_set(struct mv_pp2x_cls_flow_entry *fe, int mode);
int mv_pp2x_cls_sw_flow_macme_set(struct mv_pp2x_cls_flow_entry *fe, int mode);
int mv_pp2x_cls_sw_flow_udf7_set(struct mv_pp2x_cls_flow_entry *fe, int mode);
int mv_pp2x_cls_sw_flow_seq_ctrl_set(struct mv_pp2x_cls_flow_entry *fe,
					    int mode);
int mv_pp2x_cls_sw_flow_engine_set(struct mv_pp2x_cls_flow_entry *fe,
					   int engine, int is_last);
int mv_pp2x_cls_sw_flow_extra_set(struct mv_pp2x_cls_flow_entry *fe,
					 int type, int prio);
int mv_pp2x_cls_hw_lkp_hits_dump(struct mv_pp2x_hw *hw);
int mv_pp2x_cls_sw_lkp_dump(struct mv_pp2x_cls_lookup_entry *lkp);
int mv_pp2x_cls_hw_lkp_dump(struct mv_pp2x_hw *hw);
int mv_pp2x_cls_hw_udf_set(struct mv_pp2x_hw *hw, int udf_no, int offs_id,
				 int offs_bits, int size_bits);
int mv_pp2x_cls_c2_qos_hw_read(struct mv_pp2x_hw *hw, int tbl_id, int tbl_sel,
					int tbl_line,
					struct mv_pp2x_cls_c2_qos_entry *qos);
int mv_pp2x_cls_c2_qos_hw_write(struct mv_pp2x_hw *hw,
					struct mv_pp2x_cls_c2_qos_entry *qos);
int mvPp2ClsC2QosPrioGet(struct mv_pp2x_cls_c2_qos_entry *qos, int *prio);
int mvPp2ClsC2QosDscpGet(struct mv_pp2x_cls_c2_qos_entry *qos, int *dscp);
int mvPp2ClsC2QosColorGet(struct mv_pp2x_cls_c2_qos_entry *qos, int *color);
int mvPp2ClsC2QosGpidGet(struct mv_pp2x_cls_c2_qos_entry *qos, int *gpid);
int mvPp2ClsC2QosQueueGet(struct mv_pp2x_cls_c2_qos_entry *qos, int *queue);
int mv_pp2x_cls_c2_qos_tbl_set(struct mv_pp2x_cls_c2_entry *c2, int tbl_id,
				     int tbl_sel);
int mv_pp2x_cls_c2_hw_write(struct mv_pp2x_hw *hw, int index,
				  struct mv_pp2x_cls_c2_entry *c2);
int mv_pp2x_cls_c2_hw_read(struct mv_pp2x_hw *hw, int index,
				  struct mv_pp2x_cls_c2_entry *c2);
int mv_pp2x_cls_c2_sw_words_dump(struct mv_pp2x_cls_c2_entry *c2);
int mv_pp2x_cls_c2_hit_cntr_clear_all(struct mv_pp2x_hw *hw);
int mv_pp2x_cls_c2_hit_cntr_read(struct mv_pp2x_hw *hw, int index, u32 *cntr);
int mv_pp2x_cls_c2_hit_cntr_dump(struct mv_pp2x_hw *hw);
int mv_pp2x_cls_c2_regs_dump(struct mv_pp2x_hw *hw);
int mv_pp2x_cls_c2_rule_set(struct mv_pp2x_port *port, u8 start_queue);
u8 mv_pp2x_cls_c2_rule_queue_get(struct mv_pp2x_hw *hw, u32 rule_idx);
void mv_pp2x_cls_c2_rule_queue_set(struct mv_pp2x_hw *hw, u32 rule_idx,
					   u8 queue);
u8 mv_pp2x_cls_c2_pbit_tbl_queue_get(struct mv_pp2x_hw *hw, u8 tbl_id,
					     u8 tbl_line);
void mv_pp2x_cls_c2_pbit_tbl_queue_set(struct mv_pp2x_hw *hw, u8 tbl_id,
					      u8 tbl_line, u8 queue);
int mv_pp2x_cls_c2_hw_inv(struct mv_pp2x_hw *hw, int index);
void mv_pp2x_cls_c2_hw_inv_all(struct mv_pp2x_hw *hw);
int mv_pp2x_cls_c2_tcam_byte_set(struct mv_pp2x_cls_c2_entry *c2,
					unsigned int offs,
					unsigned char byte,
					unsigned char enable);
int mv_pp2x_cls_c2_qos_queue_set(struct mv_pp2x_cls_c2_qos_entry *qos,
					 u8 queue);
int mv_pp2x_cls_c2_color_set(struct mv_pp2x_cls_c2_entry *c2, int cmd,
				   int from);
int mv_pp2x_cls_c2_prio_set(struct mv_pp2x_cls_c2_entry *c2, int cmd,
				 int prio, int from);
int mv_pp2x_cls_c2_dscp_set(struct mv_pp2x_cls_c2_entry *c2, int cmd,
				  int dscp, int from);
int mv_pp2x_cls_c2_queue_low_set(struct mv_pp2x_cls_c2_entry *c2, int cmd,
					 int queue, int from);
int mv_pp2x_cls_c2_queue_high_set(struct mv_pp2x_cls_c2_entry *c2, int cmd,
					  int queue, int from);
int mv_pp2x_cls_c2_forward_set(struct mv_pp2x_cls_c2_entry *c2, int cmd);
int mv_pp2x_cls_c2_rss_set(struct mv_pp2x_cls_c2_entry *c2, int cmd,
				 int rss_en);
int mv_pp2x_cls_c2_flow_id_en(struct mv_pp2x_cls_c2_entry *c2,
				    int flowid_en);

int mv_pp22_rss_tbl_entry_set(struct mv_pp2x_hw *hw,
				struct mv_pp22_rss_entry *rss);
int mv_pp22_rss_tbl_entry_get(struct mv_pp2x_hw *hw,
				struct mv_pp22_rss_entry *rss);

int mv_pp22_rss_rxq_set(struct mv_pp2x_port *port, u32 cos_width);

void mv_pp22_rss_c2_enable(struct mv_pp2x_port *port, bool en);
int mv_pp22_rss_hw_dump(struct mv_pp2x_hw *hw);

#endif /* _MVPP2_HW_H_ */
