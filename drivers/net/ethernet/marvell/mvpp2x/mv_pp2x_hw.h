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

static inline void mv_pp2x_cm3_write(struct mv_pp2x_hw *hw, u32 offset, u32 data)
{
	void *reg_ptr = hw->gop.gop_110.cm3_base + offset;

	writel(data, reg_ptr);
}

static inline u32 mv_pp2x_cm3_read(struct mv_pp2x_hw *hw, u32 offset)
{
	void *reg_ptr = hw->gop.gop_110.cm3_base + offset;

	return readl(reg_ptr);
}

static inline void mv_pp2x_write(struct mv_pp2x_hw *hw, u32 offset, u32 data)
{
	void *reg_ptr = hw->cpu_base[0] + offset;

	writel(data, reg_ptr);
}

static inline void mv_pp2x_relaxed_write(struct mv_pp2x_hw *hw, u32 offset, u32 data,
					 int cpu)
{
	void *reg_ptr;

	cpu = hw->mv_pp2x_no_single_mode * cpu;
	reg_ptr = hw->cpu_base[cpu] + offset;

	writel_relaxed(data, reg_ptr);
}

static inline u32 mv_pp2x_read(struct mv_pp2x_hw *hw, u32 offset)
{
	u32 val;

	void *reg_ptr = hw->cpu_base[0] + offset;

	val = readl(reg_ptr);

	return val;
}

static inline u32 mv_pp2x_relaxed_read(struct mv_pp2x_hw *hw, u32 offset, int cpu)
{
	u32 val;
	void *reg_ptr;

	cpu = hw->mv_pp2x_no_single_mode * cpu;

	reg_ptr = hw->cpu_base[cpu] + offset;
	val = readl_relaxed(reg_ptr);
	return val;
}

static inline void mv_pp22_thread_write(struct mv_pp2x_hw *hw, u32 sw_thread,
					u32 offset, u32 data)
{
	writel(data, hw->base + sw_thread * MVPP2_ADDR_SPACE_SIZE + offset);
}

static inline u32 mv_pp22_thread_read(struct mv_pp2x_hw *hw, u32 sw_thread,
				      u32 offset)
{
	return readl(hw->base + sw_thread * MVPP2_ADDR_SPACE_SIZE + offset);
}

static inline void mv_pp22_thread_relaxed_write(struct mv_pp2x_hw *hw,
						u32 sw_thread,
						u32 offset, u32 data)
{
	writel_relaxed(data, hw->base + sw_thread * MVPP2_ADDR_SPACE_SIZE + offset);
}

static inline u32 mv_pp22_thread_relaxed_read(struct mv_pp2x_hw *hw,
					      u32 sw_thread,
					      u32 offset)
{
	return readl_relaxed(hw->base + sw_thread * MVPP2_ADDR_SPACE_SIZE + offset);
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
	u32 val = mv_pp2x_read(&port->priv->hw, MVPP2_RXQ_STATUS_REG(rxq_id));

	return val & MVPP2_RXQ_OCCUPIED_MASK;
}

/* Get number of Rx descriptors occupied by received packets */
static inline int mv_pp2x_rxq_free(struct mv_pp2x_port *port, int rxq_id)
{
	u32 val = mv_pp2x_read(&port->priv->hw, MVPP2_RXQ_STATUS_REG(rxq_id));

	return ((val & MVPP2_RXQ_NON_OCCUPIED_MASK) >>
		MVPP2_RXQ_NON_OCCUPIED_OFFSET);
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

static inline dma_addr_t mv_pp2x_bm_phys_addr_get(struct mv_pp2x_hw *hw, u32 pool)
{
	dma_addr_t val;

	val = mv_pp2x_read(hw, MVPP2_BM_PHY_ALLOC_REG(pool));

	/*  Disregard BM_ADDR_HIGH_ALLOC if Buffer Manager failed to return buffer */
	if (!val)
		return 0;

#ifdef CONFIG_PHYS_ADDR_T_64BIT
	{
	u64 val2;

	val2 = mv_pp2x_read(hw, MVPP22_BM_PHY_VIRT_HIGH_ALLOC_REG);
	val2 &= MVPP22_BM_PHY_HIGH_ALLOC_MASK;
	val |= (val2 << 32);
	}
#endif

	return val;
}

static inline void mv_pp2x_bm_hw_pool_create(struct mv_pp2x_hw *hw, u32 pool,
					     dma_addr_t pool_addr, int size)
{
	u32 val;

	mv_pp2x_write(hw, MVPP2_BM_POOL_BASE_ADDR_REG(pool),
		      lower_32_bits(pool_addr));
#if defined(CONFIG_ARCH_DMA_ADDR_T_64BIT) && defined(CONFIG_PHYS_ADDR_T_64BIT)
	val = mv_pp2x_read(hw, MVPP22_BM_POOL_BASE_ADDR_HIGH_REG);
	val &= ~MVPP22_BM_POOL_BASE_ADDR_HIGH_MASK;
	val |= (upper_32_bits(pool_addr) & MVPP22_ADDR_HIGH_MASK);
	mv_pp2x_write(hw, MVPP22_BM_POOL_BASE_ADDR_HIGH_REG, val);
#endif
	mv_pp2x_write(hw, MVPP2_BM_POOL_SIZE_REG(pool), size);

	val = mv_pp2x_read(hw, MVPP2_BM_POOL_CTRL_REG(pool));
	val |= MVPP2_BM_START_MASK;
	val &= ~MVPP2_BM_LOW_THRESH_MASK;
	val &= ~MVPP2_BM_HIGH_THRESH_MASK;

	/* Set 8 Pools BPPI threshold if BM underrun protection feature were enabled */
	if (hw->mv_bm_underrun_protect) {
		val |= MVPP2_BM_LOW_THRESH_VALUE(MVPP23_BM_BPPI_8POOL_LOW_THRESH);
		val |= MVPP2_BM_HIGH_THRESH_VALUE(MVPP23_BM_BPPI_8POOL_HIGH_THRESH);
	} else {
		val |= MVPP2_BM_LOW_THRESH_VALUE(MVPP2_BM_BPPI_LOW_THRESH);
		val |= MVPP2_BM_HIGH_THRESH_VALUE(MVPP2_BM_BPPI_HIGH_THRESH);
	}

	mv_pp2x_write(hw, MVPP2_BM_POOL_CTRL_REG(pool), val);
}

static inline void mv_pp2x_bm_pool_put_virtual(struct mv_pp2x_hw *hw, u32 pool,
					       dma_addr_t buf_phys_addr,
					      u8 *buf_virt_addr, int cpu)
{
#if defined(CONFIG_ARCH_DMA_ADDR_T_64BIT) && defined(CONFIG_PHYS_ADDR_T_64BIT)
	mv_pp2x_relaxed_write(hw, MVPP22_BM_PHY_VIRT_HIGH_RLS_REG,
			      upper_32_bits(buf_phys_addr), cpu);
#endif

	mv_pp2x_relaxed_write(hw, MVPP2_BM_VIRT_RLS_REG,
			      lower_32_bits((uintptr_t)buf_virt_addr), cpu);

	mv_pp2x_relaxed_write(hw, MVPP2_BM_PHY_RLS_REG(pool),
			      lower_32_bits(buf_phys_addr), cpu);
}

/* Release buffer to BM */
static inline void mv_pp2x_bm_pool_put(struct mv_pp2x_hw *hw, u32 pool,
				       dma_addr_t buf_phys_addr, int cpu)
{
#if defined(CONFIG_ARCH_DMA_ADDR_T_64BIT) && defined(CONFIG_PHYS_ADDR_T_64BIT)
	mv_pp2x_relaxed_write(hw, MVPP22_BM_PHY_VIRT_HIGH_RLS_REG,
			      upper_32_bits(buf_phys_addr), cpu);
#endif

	mv_pp2x_relaxed_write(hw, MVPP2_BM_PHY_RLS_REG(pool),
			      lower_32_bits(buf_phys_addr), cpu);
}

/* Release multicast buffer */
static inline void mv_pp2x_bm_pool_mc_put(struct mv_pp2x_port *port, int pool,
					  u32 buf_phys_addr,
						   u32 buf_virt_addr,
						   int mc_id, int cpu)
{
	u32 val = 0;

	val |= (mc_id & MVPP21_BM_MC_ID_MASK);
	mv_pp2x_write(&port->priv->hw, MVPP21_BM_MC_RLS_REG, val);
	/*TODO : YuvalC, this is just workaround to compile.
	 * Need to handle mv_pp2x_buff_hdr_rx().
	 */
	mv_pp2x_bm_pool_put(&port->priv->hw, pool,
			    (dma_addr_t)(buf_phys_addr |
			    MVPP2_BM_PHY_RLS_MC_BUFF_MASK), cpu);
}

static inline void mv_pp2x_port_interrupts_enable(struct mv_pp2x_port *port)
{
	int sw_thread_mask = 0, i;
	struct queue_vector *q_vec = port->q_vector;

	for (i = 0; i < port->num_qvector; i++)
		sw_thread_mask |= q_vec[i].sw_thread_mask;
	mv_pp2x_write(&port->priv->hw, MVPP2_ISR_ENABLE_REG(port->id),
		      MVPP2_ISR_ENABLE_INTERRUPT(sw_thread_mask));
}

static inline void mv_pp22_port_uio_interrupts_enable(struct mv_pp2x_port *port)
{
	int sw_thread_mask = 0, i;
	struct uio_queue_vector  *q_vec = &port->uio.q_vector[0];

	for (i = 0; i < port->uio.num_qvector; i++)
		sw_thread_mask |= (1 << q_vec[i].sw_thread_id);

	mv_pp2x_write(&port->priv->hw, MVPP2_ISR_ENABLE_REG(port->id),
		      MVPP2_ISR_ENABLE_INTERRUPT(sw_thread_mask));
}

static inline void mv_pp2x_port_interrupts_disable(struct mv_pp2x_port *port)
{
	int sw_thread_mask = 0, i;
	struct queue_vector *q_vec = port->q_vector;

	for (i = 0; i < port->num_qvector; i++)
		sw_thread_mask |= q_vec[i].sw_thread_mask;

	mv_pp2x_write(&port->priv->hw, MVPP2_ISR_ENABLE_REG(port->id),
		      MVPP2_ISR_DISABLE_INTERRUPT(sw_thread_mask));
}

static inline void mv_pp22_port_uio_interrupts_disable(struct mv_pp2x_port *port)
{
	int sw_thread_mask = 0, i;
	struct uio_queue_vector  *q_vec = &port->uio.q_vector[0];

	for (i = 0; i < port->uio.num_qvector; i++)
		sw_thread_mask |= (1 << q_vec[i].sw_thread_id);

	mv_pp2x_write(&port->priv->hw, MVPP2_ISR_ENABLE_REG(port->id),
		      MVPP2_ISR_DISABLE_INTERRUPT(sw_thread_mask));
}

static inline void mv_pp2x_qvector_interrupt_enable(struct queue_vector *q_vec)
{
	struct mv_pp2x_port *port = q_vec->parent;

	mv_pp2x_write(&port->priv->hw, MVPP2_ISR_ENABLE_REG(port->id),
		      MVPP2_ISR_ENABLE_INTERRUPT(q_vec->sw_thread_mask));
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
		sw_thread = 0;
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
			mv_pp2x_read(&port->priv->hw,
				     MVPP21_TXQ_SENT_REG(id));
		else
			mv_pp2x_read(&port->priv->hw,
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
		DMA_BIT_MASK(40)));
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

#if defined(CONFIG_ARCH_DMA_ADDR_T_64BIT) && defined(CONFIG_PHYS_ADDR_T_64BIT)
	*buf_phys_addr_p &= ~(DMA_BIT_MASK(40));
	*buf_phys_addr_p |= phys_addr & DMA_BIT_MASK(40);
#else
	*((dma_addr_t *)buf_phys_addr_p) = phys_addr;
	*((u8 *)buf_phys_addr_p + sizeof(dma_addr_t)) = 0; /*5th byte*/
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
void mv_pp2x_prs_mac_uc_promisc_set(struct mv_pp2x_hw *hw, int port, bool add);
void mv_pp2x_prs_mac_mc_promisc_set(struct mv_pp2x_hw *hw, int port, bool add);
int mv_pp2x_prs_mac_da_accept(struct mv_pp2x_port *port,
			      const u8 *da, bool add);
int mv_pp2x_prs_vid_entry_accept(struct net_device *dev, u16 proto, u16 vid, bool add);
int mv_pp2x_prs_def_flow(struct mv_pp2x_port *port);
int mv_pp2x_prs_flow_set(struct mv_pp2x_port *port);
void mv_pp2x_prs_mac_entry_del(struct mv_pp2x_port *port,
			       enum mv_pp2x_l2_cast l2_cast,
			       enum mv_pp2x_mac_del_option op);
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
struct mv_pp2x_tx_desc *mv_pp2x_txq_prev_desc_get(
		struct mv_pp2x_aggr_tx_queue *aggr_txq);
int mv_pp2x_txq_alloc_reserved_desc(struct mv_pp2x *priv,
				    struct mv_pp2x_tx_queue *txq,
				    int num, int cpu);
int mv_pp2x_aggr_desc_num_read(struct mv_pp2x *priv, int cpu);
int mv_pp2x_aggr_desc_num_check(struct mv_pp2x *priv,
				struct mv_pp2x_aggr_tx_queue *aggr_txq,
				int num, int cpu);
void mv_pp2x_rxq_offset_set(struct mv_pp2x_port *port,
			    int prxq, int offset);
void mv_pp2x_bm_pool_bufsize_set(struct mv_pp2x_hw *hw,
				 struct mv_pp2x_bm_pool *bm_pool,
				 int buf_size);
void mv_pp2x_pool_refill(struct mv_pp2x *priv, u32 pool,
			 dma_addr_t phys_addr, int cpu);

void mv_pp2x_pool_refill_virtual(struct mv_pp2x *priv, u32 pool,
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
			      struct mv_pp2x_rx_queue *rxq);
void mv_pp2x_rx_time_coal_set(struct mv_pp2x_port *port,
			      struct mv_pp2x_rx_queue *rxq);
void mv_pp2x_tx_done_pkts_coal_set_val(struct mv_pp2x_port *port, int address_space, u32 val);
void mv_pp2x_tx_done_pkts_coal_set(struct mv_pp2x_port *port, int address_space);
void mv_pp2x_tx_done_pkts_coal_set_all(struct mv_pp2x_port *port);
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
int mv_pp2x_prs_sram_bit_get(struct mv_pp2x_prs_entry *pe, int bit_num,
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
int mv_pp2x_cls_sw_flow_hek_get(struct mv_pp2x_cls_flow_entry *fe,
				int *num_of_fields, int field_ids[]);
int mv_pp2x_cls_sw_flow_port_get(struct mv_pp2x_cls_flow_entry *fe,
				 int *type, int *portid);

int mv_pp2x_cls_hw_lkp_hit_get(struct mv_pp2x_hw *hw, int lkpid, int way,
			       unsigned int *cnt);
void mv_pp2x_cls_flow_write(struct mv_pp2x_hw *hw,
			    struct mv_pp2x_cls_flow_entry *fe);
void mv_pp2x_cls_flow_read(struct mv_pp2x_hw *hw, int index,
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
int mv_pp2x_cls_sw_flow_engine_get(struct mv_pp2x_cls_flow_entry *fe,
				   int *engine, int *is_last);
int mv_pp2x_cls_sw_flow_engine_set(struct mv_pp2x_cls_flow_entry *fe,
				   int engine, int is_last);
int mv_pp2x_cls_sw_flow_extra_get(struct mv_pp2x_cls_flow_entry *fe,
				  int *type, int *prio);
int mv_pp2x_cls_sw_flow_extra_set(struct mv_pp2x_cls_flow_entry *fe,
				  int type, int prio);
int mv_pp2x_cls_hw_flow_hit_get(struct mv_pp2x_hw *hw,
				int index,  unsigned int *cnt);
int mv_pp2x_cls_hw_udf_set(struct mv_pp2x_hw *hw, int udf_no, int offs_id,
			   int offs_bits, int size_bits);
int mv_pp2x_cls_c2_qos_hw_read(struct mv_pp2x_hw *hw, int tbl_id, int tbl_sel,
			       int tbl_line,
			       struct mv_pp2x_cls_c2_qos_entry *qos);
int mv_pp2x_cls_c2_qos_hw_write(struct mv_pp2x_hw *hw,
				struct mv_pp2x_cls_c2_qos_entry *qos);
int mv_pp2_cls_c2_qos_prio_get(struct mv_pp2x_cls_c2_qos_entry *qos, int *prio);
int mv_pp2_cls_c2_qos_dscp_get(struct mv_pp2x_cls_c2_qos_entry *qos, int *dscp);
int mv_pp2_cls_c2_qos_color_get(struct mv_pp2x_cls_c2_qos_entry *qos, int *color);
int mv_pp2_cls_c2_qos_gpid_get(struct mv_pp2x_cls_c2_qos_entry *qos, int *gpid);
int mv_pp2_cls_c2_qos_queue_get(struct mv_pp2x_cls_c2_qos_entry *qos, int *queue);
int mv_pp2x_cls_c2_qos_tbl_set(struct mv_pp2x_cls_c2_entry *c2, int tbl_id,
			       int tbl_sel);
int mv_pp2x_cls_c2_hw_write(struct mv_pp2x_hw *hw, int index,
			    struct mv_pp2x_cls_c2_entry *c2);
int mv_pp2x_cls_c2_hw_read(struct mv_pp2x_hw *hw, int index,
			   struct mv_pp2x_cls_c2_entry *c2);
int mv_pp2x_cls_c2_hit_cntr_clear_all(struct mv_pp2x_hw *hw);
int mv_pp2x_cls_c2_hit_cntr_read(struct mv_pp2x_hw *hw, int index, u32 *cntr);
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

void mv_pp2x_tx_fifo_size_set(struct mv_pp2x_hw *hw, u32 port_id, u32 val);

void mv_pp2x_tx_fifo_threshold_set(struct mv_pp2x_hw *hw, u32 port_id, u32 val);

int mv_pp2x_check_hw_buf_num(struct mv_pp2x *priv, struct mv_pp2x_bm_pool *bm_pool);
void mv_pp22_set_net_comp(struct mv_pp2x *priv);
int mvcpn110_mac_hw_init(struct mv_pp2x_port *port);
void mv_pp2x_counters_stat_update(struct mv_pp2x_port *port,
				  struct gop_stat *gop_statistics);
void mv_pp2x_counters_stat_clear(struct mv_pp2x_port *port);
void mv_pp2x_aggr_txq_pend_send(struct mv_pp2x_port *port,
				struct mv_pp2x_cp_pcpu *cp_pcpu,
				struct mv_pp2x_aggr_tx_queue *aggr_txq, int address_space);

/* Interface to CM3 processor for Dying Gasp feature */
void mv_pp2x_dying_gasp_set_dst_mac(struct mv_pp2x_port *port, unsigned char *mac_addr);
void mv_pp2x_dying_gasp_set_src_mac(struct mv_pp2x_port *port, unsigned char *mac_addr);
void mv_pp2x_dying_gasp_set_ether_type(struct mv_pp2x_port *port, u32 ether_type);
void mv_pp2x_dying_gasp_set_vlan(struct mv_pp2x_port *port, u32 vlan);
void mv_pp2x_dying_gasp_set_data_patern(struct mv_pp2x_port *port, unsigned char *data, int data_size);
void mv_pp2x_dying_gasp_set_packet_control(struct mv_pp2x_port *port, int packets_count, int packet_size);
void mv_pp2x_dying_gasp_control(struct mv_pp2x_port *port, bool en);

#endif /* _MVPP2_HW_H_ */
