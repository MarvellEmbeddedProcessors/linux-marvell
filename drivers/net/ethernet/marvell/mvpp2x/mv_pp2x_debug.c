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
#include <linux/netdevice.h>
#include <linux/etherdevice.h>
#include <linux/platform_device.h>
#include <linux/skbuff.h>
#include <linux/inetdevice.h>
#include <linux/mbus.h>
#include <linux/module.h>
#include <linux/interrupt.h>
#include <linux/cpumask.h>
#include <linux/of.h>
#include <linux/of_irq.h>
#include <linux/of_mdio.h>
#include <linux/of_net.h>
#include <linux/of_address.h>
#include <linux/of_device.h>

#include <linux/phy.h>
#include <linux/clk.h>
#include <linux/hrtimer.h>
#include <linux/ktime.h>
#include <uapi/linux/ppp_defs.h>
#include <net/ip.h>
#include <net/ipv6.h>

#include "mv_pp2x.h"
#include "mv_pp2x_hw.h"
#include "mv_pp2x_debug.h"

void mv_pp2x_print_reg(struct mv_pp2x_hw *hw, unsigned int reg_addr,
		       char *reg_name)
{
	DBG_MSG("  %-32s: 0x%x = 0x%08x\n", reg_name, reg_addr,
		mv_pp2x_read(hw, reg_addr));
}

void mv_pp2x_print_reg2(struct mv_pp2x_hw *hw, unsigned int reg_addr,
			char *reg_name, unsigned int index)
{
	char buf[64];

	sprintf(buf, "%s[%d]", reg_name, index);
	DBG_MSG("  %-32s: 0x%x = 0x%08x\n", reg_name, reg_addr,
		mv_pp2x_read(hw, reg_addr));
}

void mv_pp2x_bm_pool_regs(struct mv_pp2x_hw *hw, int pool)
{
	if (mv_pp2x_max_check(pool, MVPP2_BM_POOLS_NUM, "bm_pool"))
		return;

	DBG_MSG("\n[BM pool registers: pool=%d]\n", pool);
	mv_pp2x_print_reg(hw, MVPP2_BM_POOL_BASE_ADDR_REG(pool),
			  "MV_BM_POOL_BASE_REG");
	mv_pp2x_print_reg(hw, MVPP2_BM_POOL_SIZE_REG(pool),
			  "MVPP2_BM_POOL_SIZE_REG");
	mv_pp2x_print_reg(hw, MVPP2_BM_POOL_READ_PTR_REG(pool),
			  "MVPP2_BM_POOL_READ_PTR_REG");
	mv_pp2x_print_reg(hw, MVPP2_BM_POOL_PTRS_NUM_REG(pool),
			  "MVPP2_BM_POOL_PTRS_NUM_REG");
	mv_pp2x_print_reg(hw, MVPP2_BM_BPPI_READ_PTR_REG(pool),
			  "MVPP2_BM_BPPI_READ_PTR_REG");
	mv_pp2x_print_reg(hw, MVPP2_BM_BPPI_PTRS_NUM_REG(pool),
			  "MVPP2_BM_BPPI_PTRS_NUM_REG");
	mv_pp2x_print_reg(hw, MVPP2_BM_POOL_CTRL_REG(pool),
			  "MVPP2_BM_POOL_CTRL_REG");
	mv_pp2x_print_reg(hw, MVPP2_BM_INTR_CAUSE_REG(pool),
			  "MVPP2_BM_INTR_CAUSE_REG");
	mv_pp2x_print_reg(hw, MVPP2_BM_INTR_MASK_REG(pool),
			  "MVPP2_BM_INTR_MASK_REG");
}
EXPORT_SYMBOL(mv_pp2x_bm_pool_regs);

void mv_pp2x_bm_pool_drop_count(struct mv_pp2x_hw *hw, int pool)
{
	if (mv_pp2x_max_check(pool, MVPP2_BM_POOLS_NUM, "bm_pool"))
		return;

	mv_pp2x_print_reg2(hw, MVPP2_BM_DROP_CNTR_REG(pool),
			   "MVPP2_BM_DROP_CNTR_REG", pool);
	mv_pp2x_print_reg2(hw, MVPP2_BM_MC_DROP_CNTR_REG(pool),
			   "MVPP2_BM_MC_DROP_CNTR_REG", pool);
}
EXPORT_SYMBOL(mv_pp2x_bm_pool_drop_count);

void mv_pp2x_pool_status(struct mv_pp2x *priv, int log_pool_num)
{
	struct mv_pp2x_bm_pool *bm_pool = NULL;
	int /*buf_size,*/ total_size, i, pool;

	if (mv_pp2x_max_check(log_pool_num, MVPP2_BM_SWF_POOL_OUT_OF_RANGE,
			      "log_pool"))
		return;

	for (i = 0; i < priv->num_pools; i++) {
		if (priv->bm_pools[i].log_id == log_pool_num) {
			bm_pool = &priv->bm_pools[i];
			pool = bm_pool->id;
		}
	}
	if (!bm_pool) {
		pr_err("%s: Logical BM pool %d is not initialized\n",
		       __func__, log_pool_num);
		return;
	}

	total_size = RX_TOTAL_SIZE(bm_pool->buf_size);

	DBG_MSG(
		"\n%12s log_pool=%d, phy_pool=%d: pkt_size=%d, buf_size=%d total_size=%d\n",
		mv_pp2x_pools[log_pool_num].description, log_pool_num, pool,
		bm_pool->pkt_size, bm_pool->buf_size, total_size);
	DBG_MSG("\tcapacity=%d, buf_num=%d, in_use=%u, in_use_thresh=%u\n",
		bm_pool->size, bm_pool->buf_num, atomic_read(&bm_pool->in_use),
		bm_pool->in_use_thresh);
}
EXPORT_SYMBOL(mv_pp2x_pool_status);

void mv_pp2_pool_stats_print(struct mv_pp2x *priv, int log_pool_num)
{
	int i, pool;
	struct mv_pp2x_bm_pool *bm_pool = NULL;

	if (mv_pp2x_max_check(log_pool_num, MVPP2_BM_SWF_POOL_OUT_OF_RANGE,
			      "log_pool"))
		return;

	for (i = 0; i < priv->num_pools; i++) {
		if (priv->bm_pools[i].log_id == log_pool_num) {
			bm_pool = &priv->bm_pools[i];
			pool = bm_pool->id;
		}
	}
	if (!bm_pool) {
		pr_err("%s: Logical BM pool %d is not initialized\n",
		       __func__, log_pool_num);
		return;
	}

#ifdef MVPP2_DEBUG
	DBG_MSG("skb_alloc_oom    = %u\n", bm_pool->stats.skb_alloc_oom);
	DBG_MSG("skb_alloc_ok     = %u\n", bm_pool->stats.skb_alloc_ok);
	DBG_MSG("bm_put           = %u\n", bm_pool->stats.bm_put);
	memset(&bm_pool->stats, 0, sizeof(bm_pool->stats));
#endif /* MVPP2_DEBUG */
}
EXPORT_SYMBOL(mv_pp2_pool_stats_print);

void mvPp2RxDmaRegsPrint(struct mv_pp2x *priv, bool print_all,
			 int start, int stop)
{
	int i, num_rx_queues, result;
	bool enabled;

	struct mv_pp2x_hw *hw = &priv->hw;

	num_rx_queues = (MVPP2_MAX_PORTS * priv->pp2xdata->pp2x_max_port_rxqs);
	if (stop >= num_rx_queues || start > stop || start < 0) {
		DBG_MSG("\nERROR: wrong inputs\n");
		return;
	}

	DBG_MSG("\n[RX DMA regs]\n");
	DBG_MSG("\nRXQs [0..%d], registers\n", num_rx_queues - 1);

	for (i = start; i <= stop; i++) {
		if (!print_all) {
			result = mv_pp2x_read(hw, MVPP2_RXQ_CONFIG_REG(i));
			enabled = !(result & MVPP2_RXQ_DISABLE_MASK);
		}
		if (print_all || enabled) {
			DBG_MSG("RXQ[%d]:\n", i);
			mv_pp2x_print_reg(hw, MVPP2_RXQ_STATUS_REG(i),
					  "MVPP2_RX_STATUS");
			mv_pp2x_print_reg2(hw, MVPP2_RXQ_CONFIG_REG(i),
					   "MVPP2_RXQ_CONFIG_REG", i);
		}
	}
	DBG_MSG("\nBM pools [0..%d] registers\n", MVPP2_BM_POOLS_NUM - 1);
	for (i = 0; i < MVPP2_BM_POOLS_NUM; i++) {
		if (!print_all) {
			enabled = mv_pp2x_read(hw, MVPP2_BM_POOL_CTRL_REG(i)) &
				MVPP2_BM_STATE_MASK;
		}
		if (print_all || enabled) {
			DBG_MSG("POOL[%d]:\n", i);
			mv_pp2x_print_reg2(hw, MVPP2_POOL_BUF_SIZE_REG(i),
					   "MVPP2_POOL_BUF_SIZE_REG", i);
		}
	}
	DBG_MSG("\nIngress ports [0..%d] registers\n", MVPP2_MAX_PORTS - 1);
	for (i = 0; i < MVPP2_MAX_PORTS; i++) {
		mv_pp2x_print_reg2(hw, MVPP2_RX_CTRL_REG(i),
				   "MVPP2_RX_CTRL_REG", i);
	}
	DBG_MSG("\n");
}
EXPORT_SYMBOL(mvPp2RxDmaRegsPrint);

static void mvPp2RxQueueDetailedShow(struct mv_pp2x *priv,
				     struct mv_pp2x_rx_queue *pp_rxq)
{
	int i;
	struct mv_pp2x_rx_desc *rx_desc = pp_rxq->first_desc;

	for (i = 0; i < pp_rxq->size; i++) {
		DBG_MSG("%3d. desc=%p, status=%08x, data_size=%4d",
			i, rx_desc+i, rx_desc[i].status,
			rx_desc[i].data_size);
		if (priv->pp2_version == PPV21) {
			DBG_MSG("buf_addr=%lx, buf_cookie=%p",
				(unsigned long)
				mv_pp21_rxdesc_phys_addr_get(rx_desc),
				mv_pp21_rxdesc_cookie_get(rx_desc));
		} else {
			DBG_MSG("buf_addr=%lx, buf_cookie=%p",
				(unsigned long)
				mv_pp22_rxdesc_phys_addr_get(rx_desc),
				mv_pp22_rxdesc_cookie_get(rx_desc));
		}
		DBG_MSG("parser_info=%03x\n", rx_desc->rsrvd_parser);
	}
}

/* Show Port/Rxq descriptors ring */
void mvPp2RxqShow(struct mv_pp2x *priv, int port, int rxq, int mode)
{
	struct mv_pp2x_port *pp_port;
	struct mv_pp2x_rx_queue *pp_rxq;

	pp_port = mv_pp2x_port_struct_get(priv, port);

	if (pp_port == NULL) {
		DBG_MSG("port #%d is not initialized\n", port);
		return;
	}

	if (mv_pp2x_max_check(rxq, pp_port->num_rx_queues, "logical rxq"))
		return;

	pp_rxq = pp_port->rxqs[rxq];

	if (pp_rxq->first_desc == NULL) {
		DBG_MSG("rxq #%d of port #%d is not initialized\n", rxq, port);
		return;
	}

	DBG_MSG("\n[PPv2 RxQ show: port=%d, logical rxq=%d -> phys rxq=%d]\n",
		port, pp_rxq->log_id, pp_rxq->id);

	DBG_MSG("size=%d, pkts_coal=%d, time_coal=%d\n",
		pp_rxq->size, pp_rxq->pkts_coal, pp_rxq->time_coal);

	DBG_MSG(
		"first_virt_addr=%p, first_dma_addr=%lx, next_rx_desc=%d, rxq_cccupied=%d, rxq_nonoccupied=%d\n",
		pp_rxq->first_desc,
		(unsigned long)MVPP2_DESCQ_MEM_ALIGN(pp_rxq->descs_phys),
		pp_rxq->next_desc_to_proc,
		mv_pp2x_rxq_received(pp_port, pp_rxq->id),
		mv_pp2x_rxq_free(pp_port, pp_rxq->id));
	DBG_MSG("virt_mem_area_addr=%p, dma_mem_area_addr=%lx\n",
		pp_rxq->desc_mem, (unsigned long)pp_rxq->descs_phys);

	if (mode)
		mvPp2RxQueueDetailedShow(priv, pp_rxq);
}
EXPORT_SYMBOL(mvPp2RxqShow);

void mvPp2PhysRxqRegs(struct mv_pp2x *pp2, int rxq)
{
	struct mv_pp2x_hw *hw  = &pp2->hw;

	DBG_MSG("\n[PPv2 RxQ registers: global rxq=%d]\n", rxq);

	mv_pp2x_write(hw, MVPP2_RXQ_NUM_REG, rxq);
	mv_pp2x_print_reg(hw, MVPP2_RXQ_NUM_REG,
			  "MVPP2_RXQ_NUM_REG");
	mv_pp2x_print_reg(hw, MVPP2_RXQ_DESC_ADDR_REG,
			  "MVPP2_RXQ_DESC_ADDR_REG");
	mv_pp2x_print_reg(hw, MVPP2_RXQ_DESC_SIZE_REG,
			  "MVPP2_RXQ_DESC_SIZE_REG");
	mv_pp2x_print_reg(hw, MVPP2_RXQ_STATUS_REG(rxq),
			  "MVPP2_RXQ_STATUS_REG");
	mv_pp2x_print_reg(hw, MVPP2_RXQ_THRESH_REG,
			  "MVPP2_RXQ_THRESH_REG");
	mv_pp2x_print_reg(hw, MVPP2_RXQ_INDEX_REG,
			  "MVPP2_RXQ_INDEX_REG");
	mv_pp2x_print_reg(hw, MVPP2_RXQ_CONFIG_REG(rxq),
			  "MVPP2_RXQ_CONFIG_REG");
}
EXPORT_SYMBOL(mvPp2PhysRxqRegs);

void mvPp2PortRxqRegs(struct mv_pp2x *pp2, int port, int rxq)
{
	int phy_rxq;
	struct mv_pp2x_port *pp2_port = mv_pp2x_port_struct_get(pp2, port);

	DBG_MSG("\n[PPv2 RxQ registers: port=%d, local rxq=%d]\n", port, rxq);

	if (rxq >= MVPP2_MAX_RXQ)
		return;

	if (!pp2_port)
		return;

	phy_rxq = pp2_port->first_rxq + rxq;
	mvPp2PhysRxqRegs(pp2, phy_rxq);
}
EXPORT_SYMBOL(mvPp2PortRxqRegs);


void mv_pp22_isr_rx_group_regs(struct mv_pp2x *priv, int port, bool print_all)
{
	int val, i, num_threads, cpu_offset, cpu;
	struct mv_pp2x_hw *hw = &priv->hw;
	struct mv_pp2x_port *pp_port;


	pp_port = mv_pp2x_port_struct_get(priv, port);
	if (!pp_port) {
		DBG_MSG("Input Error\n %s", __func__);
		return;
	}

	if (print_all)
		num_threads = MVPP2_MAX_SW_THREADS;
	else
		num_threads = pp_port->num_qvector;

	for (i = 0; i < num_threads; i++) {
		DBG_MSG("\n[PPv2 RxQ GroupConfig registers: port=%d cpu=%d]",
				port, i);

		val = (port << MVPP22_ISR_RXQ_GROUP_INDEX_GROUP_OFFSET) | i;
		mv_pp2x_write(hw, MVPP22_ISR_RXQ_GROUP_INDEX_REG, val);

		mv_pp2x_print_reg(hw, MVPP22_ISR_RXQ_GROUP_INDEX_REG,
				  "MVPP22_ISR_RXQ_GROUP_INDEX_REG");
		mv_pp2x_print_reg(hw, MVPP22_ISR_RXQ_SUB_GROUP_CONFIG_REG,
				  "MVPP22_ISR_RXQ_SUB_GROUP_CONFIG_REG");
		/*reg_val = mv_pp2x_read(hw,
		 *	MVPP22_ISR_RXQ_SUB_GROUP_CONFIG_REG);
		 */
		/*start_queue  = reg_val &
		 *	MVPP22_ISR_RXQ_SUB_GROUP_STARTQ_MASK;
		 */
		/*sub_group_size = reg_val &
		 *	MVPP22_ISR_RXQ_SUB_GROUP_SIZE_MASK;
		 */
	}
	DBG_MSG("\n[PPv2 Port Interrupt Enable register : port=%d]\n", port);
	mv_pp2x_print_reg(hw, MVPP2_ISR_ENABLE_REG(port),
			  "MVPP2_ISR_ENABLE_REG");

	DBG_MSG("\n[PPv2 Eth Occupied Interrupt registers: port=%d]\n", port);
	for (i = 0; i < num_threads; i++) {
		if (print_all)
			cpu = i;
		else
			cpu = pp_port->q_vector[i].sw_thread_id;
		cpu_offset = cpu*MVPP2_ADDR_SPACE_SIZE;
		DBG_MSG("cpu=%d]\n", cpu);
		mv_pp2x_print_reg(hw, cpu_offset +
				  MVPP2_ISR_RX_TX_CAUSE_REG(port),
				  "MVPP2_ISR_RX_TX_CAUSE_REG");
		mv_pp2x_print_reg(hw, cpu_offset +
				  MVPP2_ISR_RX_TX_MASK_REG(port),
				  "MVPP2_ISR_RX_TX_MASK_REG");
	}

}
EXPORT_SYMBOL(mv_pp22_isr_rx_group_regs);

static void mvPp2TxQueueDetailedShow(struct mv_pp2x *priv,
				     void *pp_txq, bool aggr_queue)
{
	int i, j, size;
	struct mv_pp2x_tx_desc *tx_desc;

	if (aggr_queue) {
		size = ((struct mv_pp2x_aggr_tx_queue *)pp_txq)->size;
		tx_desc = ((struct mv_pp2x_aggr_tx_queue *)pp_txq)->first_desc;
	} else {
		size = ((struct mv_pp2x_tx_queue *)pp_txq)->size;
		tx_desc = ((struct mv_pp2x_tx_queue *)pp_txq)->first_desc;
	}

	for (i = 0; i < 16/*size*/; i++) {
		DBG_MSG(
			"%3d. desc=%p, cmd=%x, data_size=%-4d pkt_offset=%-3d, phy_txq=%d\n",
		   i, tx_desc+i, tx_desc[i].command, tx_desc[i].data_size,
		   tx_desc[i].packet_offset, tx_desc[i].phys_txq);
		if (priv->pp2_version == PPV21) {
			DBG_MSG("buf_phys_addr=%08x, buf_cookie=%08x\n",
				tx_desc[i].u.pp21.buf_phys_addr,
				tx_desc[i].u.pp21.buf_cookie);
			DBG_MSG(
				"hw_cmd[0]=%x, hw_cmd[1]=%x, hw_cmd[2]=%x, rsrvd1=%x\n",
				tx_desc[i].u.pp21.rsrvd_hw_cmd[0],
				tx_desc[i].u.pp21.rsrvd_hw_cmd[1],
				tx_desc[i].u.pp21.rsrvd_hw_cmd[2],
				tx_desc[i].u.pp21.rsrvd1);
		} else {
			DBG_MSG(
				"     rsrvd_hw_cmd1=%llx, buf_phys_addr_cmd2=%llx, buf_cookie_bm_cmd3=%llx\n",
				tx_desc[i].u.pp22.rsrvd_hw_cmd1,
				tx_desc[i].u.pp22.buf_phys_addr_hw_cmd2,
				tx_desc[i].u.pp22.buf_cookie_bm_qset_hw_cmd3);

			for (j = 0; j < 8; j++)
				DBG_MSG("%d:%x\n", j, *((u32 *)(tx_desc+i)+j));
		}
	}
}

/* Show Port/TXQ descriptors ring */
void mvPp2TxqShow(struct mv_pp2x *priv, int port, int txq, int mode)
{
	struct mv_pp2x_port *pp_port;
	struct mv_pp2x_tx_queue *pp_txq;
	struct mv_pp2x_txq_pcpu *txq_pcpu;
	int cpu;

	pp_port = mv_pp2x_port_struct_get(priv, port);

	if (pp_port == NULL) {
		DBG_MSG("port #%d is not initialized\n", port);
		return;
	}

	if (mv_pp2x_max_check(txq, pp_port->num_tx_queues, "logical txq"))
		return;

	pp_txq = pp_port->txqs[txq];

	if (pp_txq->first_desc == NULL) {
		DBG_MSG("txq #%d of port #%d is not initialized\n", txq, port);
		return;
	}

	DBG_MSG("\n[PPv2 TxQ show: port=%d, logical_txq=%d]\n",
		port, pp_txq->log_id);

	DBG_MSG("physical_txq=%d, size=%d, pkts_coal=%d\n",
		pp_txq->id, pp_txq->size, pp_txq->pkts_coal);

	DBG_MSG("first_virt_addr=%p, first_dma_addr=%lx, next_tx_desc=%d\n",
		pp_txq->first_desc,
		(unsigned long)MVPP2_DESCQ_MEM_ALIGN(pp_txq->descs_phys),
		pp_txq->next_desc_to_proc);
	DBG_MSG("virt_mem_area_addr=%p, dma_mem_area_addr=%lx\n",
		pp_txq->desc_mem, (unsigned long)pp_txq->descs_phys);

	for_each_online_cpu(cpu) {
		txq_pcpu = per_cpu_ptr(pp_txq->pcpu, cpu);
		DBG_MSG("\n[PPv2 TxQ %d cpu=%d show:\n", txq, cpu);

		DBG_MSG("cpu=%d, size=%d, reserved_num=%d\n",
			txq_pcpu->cpu, txq_pcpu->size,
			txq_pcpu->reserved_num);
		DBG_MSG("txq_put_index=%d, txq_get_index=%d\n",
			txq_pcpu->txq_put_index, txq_pcpu->txq_get_index);
		DBG_MSG("tx_skb=%p, tx_buffs=%p\n",
			txq_pcpu->tx_skb, txq_pcpu->tx_buffs);
	}

	if (mode)
		mvPp2TxQueueDetailedShow(priv, pp_txq, 0);
}
EXPORT_SYMBOL(mvPp2TxqShow);

/* Show CPU aggregation TXQ descriptors ring */
void mvPp2AggrTxqShow(struct mv_pp2x *priv, int cpu, int mode)
{

	struct mv_pp2x_aggr_tx_queue *aggr_queue = NULL;
	int i;

	DBG_MSG("\n[PPv2 AggrTxQ: cpu=%d]\n", cpu);

	for (i = 0; i < priv->num_aggr_qs; i++) {
		if (priv->aggr_txqs[i].id == cpu) {
			aggr_queue = &priv->aggr_txqs[i];
			break;
		}
	}
	if (!aggr_queue) {
		DBG_MSG("aggr_txq for cpu #%d is not initialized\n", cpu);
		return;
	}

	DBG_MSG("id=%d, size=%d, count=%d, next_desc=%d, pending_cntr=%d\n",
		aggr_queue->id,
		aggr_queue->size, aggr_queue->count,
		aggr_queue->next_desc_to_proc,
		mv_pp2x_aggr_desc_num_read(priv, cpu));

	if (mode)
		mvPp2TxQueueDetailedShow(priv, aggr_queue, 1);

}
EXPORT_SYMBOL(mvPp2AggrTxqShow);

void mvPp2PhysTxqRegs(struct mv_pp2x *priv, int txq)
{
	struct mv_pp2x_hw *hw = &priv->hw;

	DBG_MSG("\n[PPv2 TxQ registers: global txq=%d]\n", txq);

	if (mv_pp2x_max_check(txq, MVPP2_TXQ_TOTAL_NUM, "global txq"))
		return;

	mv_pp2x_write(hw, MVPP2_TXQ_NUM_REG, txq);
	mv_pp2x_print_reg(hw, MVPP2_TXQ_NUM_REG,
			  "MVPP2_TXQ_NUM_REG");
	mv_pp2x_print_reg(hw, MVPP2_TXQ_DESC_ADDR_LOW_REG,
			  "MVPP2_TXQ_DESC_ADDR_LOW_REG");
	if (priv->pp2_version == PPV22)
		mv_pp2x_print_reg(hw, MVPP22_TXQ_DESC_ADDR_HIGH_REG,
				  "MVPP22_TXQ_DESC_ADDR_HIGH_REG");
	mv_pp2x_print_reg(hw, MVPP2_TXQ_DESC_SIZE_REG,
			  "MVPP2_TXQ_DESC_SIZE_REG");
	mv_pp2x_print_reg(hw, MVPP2_TXQ_DESC_HWF_SIZE_REG,
			  "MVPP2_TXQ_DESC_HWF_SIZE_REG");
	mv_pp2x_print_reg(hw, MVPP2_TXQ_INDEX_REG,
			  "MVPP2_TXQ_INDEX_REG");
	mv_pp2x_print_reg(hw, MVPP2_TXQ_PREF_BUF_REG,
			  "MVPP2_TXQ_PREF_BUF_REG");
	mv_pp2x_print_reg(hw, MVPP2_TXQ_PENDING_REG,
			  "MVPP2_TXQ_PENDING_REG");
	mv_pp2x_print_reg(hw, MVPP2_TXQ_INT_STATUS_REG,
			  "MVPP2_TXQ_INT_STATUS_REG");
	if (priv->pp2_version == PPV21)
		mv_pp2x_print_reg(hw, MVPP21_TXQ_SENT_REG(txq),
				  "MVPP21_TXQ_SENT_REG");
	else
		mv_pp2x_print_reg(hw, MVPP22_TXQ_SENT_REG(txq),
				  "MVPP22_TXQ_SENT_REG");
}
EXPORT_SYMBOL(mvPp2PhysTxqRegs);

void mvPp2PortTxqRegs(struct mv_pp2x *priv, int port, int txq)
{
	struct mv_pp2x_port *pp2_port;

	pp2_port = mv_pp2x_port_struct_get(priv, port);

	if (mv_pp2x_max_check(txq, pp2_port->num_tx_queues, "port txq"))
		return;

	DBG_MSG("\n[PPv2 TxQ registers: port=%d, local txq=%d]\n", port, txq);

	mvPp2PhysTxqRegs(priv, pp2_port->txqs[txq]->id);
}
EXPORT_SYMBOL(mvPp2PortTxqRegs);

void mvPp2AggrTxqRegs(struct mv_pp2x *priv, int cpu)
{
	struct mv_pp2x_hw *hw = &priv->hw;

	DBG_MSG("\n[PP2 Aggr TXQ registers: cpu=%d]\n", cpu);

	mv_pp2x_print_reg(hw, MVPP2_AGGR_TXQ_DESC_ADDR_REG(cpu),
			  "MVPP2_AGGR_TXQ_DESC_ADDR_REG");
	mv_pp2x_print_reg(hw, MVPP2_AGGR_TXQ_DESC_SIZE_REG(cpu),
			  "MVPP2_AGGR_TXQ_DESC_SIZE_REG");
	mv_pp2x_print_reg(hw, MVPP2_AGGR_TXQ_STATUS_REG(cpu),
			  "MVPP2_AGGR_TXQ_STATUS_REG");
	mv_pp2x_print_reg(hw, MVPP2_AGGR_TXQ_INDEX_REG(cpu),
			  "MVPP2_AGGR_TXQ_INDEX_REG");
}
EXPORT_SYMBOL(mvPp2AggrTxqRegs);

void mvPp2V1TxqDbgCntrs(struct mv_pp2x *priv, int port, int txq)
{
	struct mv_pp2x_hw *hw = &priv->hw;

	DBG_MSG("\n------ [Port #%d txq #%d counters] -----\n", port, txq);
	mv_pp2x_write(hw, MVPP2_CNT_IDX_REG, MVPP2_CNT_IDX_TX(port, txq));
	mv_pp2x_print_reg(hw, MVPP2_CNT_IDX_REG,
			  "MVPP2_CNT_IDX_REG");
	mv_pp2x_print_reg(hw, MVPP2_TX_DESC_ENQ_REG,
			  "MVPP2_TX_DESC_ENQ_REG");
	mv_pp2x_print_reg(hw, MVPP2_TX_DESC_ENQ_TO_DRAM_REG,
			  "MVPP2_TX_DESC_ENQ_TO_DRAM_REG");
	mv_pp2x_print_reg(hw, MVPP2_TX_BUF_ENQ_TO_DRAM_REG,
			  "MVPP2_TX_BUF_ENQ_TO_DRAM_REG");
	mv_pp2x_print_reg(hw, MVPP2_TX_DESC_HWF_ENQ_REG,
			  "MVPP2_TX_DESC_HWF_ENQ_REG");
	mv_pp2x_print_reg(hw, MVPP2_TX_PKT_DQ_REG,
			  "MVPP2_TX_PKT_DQ_REG");
	mv_pp2x_print_reg(hw, MVPP2_TX_PKT_FULLQ_DROP_REG,
			  "MVPP2_TX_PKT_FULLQ_DROP_REG");
	mv_pp2x_print_reg(hw, MVPP2_TX_PKT_EARLY_DROP_REG,
			  "MVPP2_TX_PKT_EARLY_DROP_REG");
	mv_pp2x_print_reg(hw, MVPP2_TX_PKT_BM_DROP_REG,
			  "MVPP2_TX_PKT_BM_DROP_REG");
	mv_pp2x_print_reg(hw, MVPP2_TX_PKT_BM_MC_DROP_REG,
			  "MVPP2_TX_PKT_BM_MC_DROP_REG");
}
EXPORT_SYMBOL(mvPp2V1TxqDbgCntrs);

void mvPp2V1DropCntrs(struct mv_pp2x *priv, int port)
{
	int q;
	struct mv_pp2x_hw *hw = &priv->hw;
	struct mv_pp2x_port *pp2_port = mv_pp2x_port_struct_get(priv, port);

	DBG_MSG("\n[global drop counters]\n");
	mv_pp2x_print_reg(hw, MVPP2_V1_OVERFLOW_MC_DROP_REG,
			  "MV_PP2_OVERRUN_DROP_REG");

	DBG_MSG("\n[Port #%d Drop counters]\n", port);
	mv_pp2x_print_reg(hw, MV_PP2_OVERRUN_DROP_REG(port),
			  "MV_PP2_OVERRUN_DROP_REG");
	mv_pp2x_print_reg(hw, MV_PP2_CLS_DROP_REG(port),
			  "MV_PP2_CLS_DROP_REG");

	for (q = 0; q < pp2_port->num_tx_queues; q++) {
		DBG_MSG("\n------ [Port #%d txp #%d txq #%d counters] -----\n",
				port, port, q);
		mv_pp2x_write(hw, MVPP2_CNT_IDX_REG, MVPP2_CNT_IDX_TX(port, q));
		mv_pp2x_print_reg(hw, MVPP2_TX_PKT_FULLQ_DROP_REG,
				  "MV_PP2_TX_PKT_FULLQ_DROP_REG");
		mv_pp2x_print_reg(hw, MVPP2_TX_PKT_EARLY_DROP_REG,
				  "MV_PP2_TX_PKT_EARLY_DROP_REG");
		mv_pp2x_print_reg(hw, MVPP2_TX_PKT_BM_DROP_REG,
				  "MV_PP2_TX_PKT_BM_DROP_REG");
		mv_pp2x_print_reg(hw, MVPP2_TX_PKT_BM_MC_DROP_REG,
				  "MV_PP2_TX_PKT_BM_MC_DROP_REG");
	}

	for (q = pp2_port->first_rxq; q < (pp2_port->first_rxq +
			pp2_port->num_rx_queues); q++) {
		DBG_MSG("\n------ [Port #%d, rxq #%d counters] -----\n",
			port, q);
		mv_pp2x_write(hw, MVPP2_CNT_IDX_REG, q);
		mv_pp2x_print_reg(hw, MVPP2_RX_PKT_FULLQ_DROP_REG,
				  "MV_PP2_RX_PKT_FULLQ_DROP_REG");
		mv_pp2x_print_reg(hw, MVPP2_RX_PKT_EARLY_DROP_REG,
				  "MV_PP2_RX_PKT_EARLY_DROP_REG");
		mv_pp2x_print_reg(hw, MVPP2_RX_PKT_BM_DROP_REG,
				  "MV_PP2_RX_PKT_BM_DROP_REG");
	}
}
EXPORT_SYMBOL(mvPp2V1DropCntrs);

void mvPp2TxRegs(struct mv_pp2x *priv)
{
	struct mv_pp2x_hw *hw = &priv->hw;
	int i;

	DBG_MSG("\n[TX general registers]\n");

	mv_pp2x_print_reg(hw, MVPP2_TX_SNOOP_REG, "MVPP2_TX_SNOOP_REG");
	if (priv->pp2_version == PPV21) {
		mv_pp2x_print_reg(hw, MVPP21_TX_FIFO_THRESH_REG,
				  "MVPP21_TX_FIFO_THRESH_REG");
	} else {
		for (i = 0 ; i < MVPP2_MAX_PORTS; i++) {
			mv_pp2x_print_reg(hw, MVPP22_TX_FIFO_THRESH_REG(i),
					  "MVPP22_TX_FIFO_THRESH_REG");
		}
	}
	mv_pp2x_print_reg(hw, MVPP2_TX_PORT_FLUSH_REG,
			  "MVPP2_TX_PORT_FLUSH_REG");
}
EXPORT_SYMBOL(mvPp2TxRegs);

void mvPp2TxSchedRegs(struct mv_pp2x *priv, int port)
{
	struct mv_pp2x_hw *hw = &priv->hw;
	struct mv_pp2x_port *pp2_port = mv_pp2x_port_struct_get(priv, port);
	int physTxp, txq;

	physTxp = mv_pp2x_egress_port(pp2_port);

	DBG_MSG("\n[TXP Scheduler registers: port=%d, physPort=%d]\n",
			port, physTxp);

	mv_pp2x_write(hw, MVPP2_TXP_SCHED_PORT_INDEX_REG, physTxp);
	mv_pp2x_print_reg(hw, MVPP2_TXP_SCHED_PORT_INDEX_REG,
			  "MV_PP2_TXP_SCHED_PORT_INDEX_REG");
	mv_pp2x_print_reg(hw, MVPP2_TXP_SCHED_Q_CMD_REG,
			  "MV_PP2_TXP_SCHED_Q_CMD_REG");
	mv_pp2x_print_reg(hw, MVPP2_TXP_SCHED_CMD_1_REG,
			  "MV_PP2_TXP_SCHED_CMD_1_REG");
	mv_pp2x_print_reg(hw, MVPP2_TXP_SCHED_FIXED_PRIO_REG,
			  "MV_PP2_TXP_SCHED_FIXED_PRIO_REG");
	mv_pp2x_print_reg(hw, MVPP2_TXP_SCHED_PERIOD_REG,
			  "MV_PP2_TXP_SCHED_PERIOD_REG");
	mv_pp2x_print_reg(hw, MVPP2_TXP_SCHED_MTU_REG,
			  "MV_PP2_TXP_SCHED_MTU_REG");
	mv_pp2x_print_reg(hw, MVPP2_TXP_SCHED_REFILL_REG,
			  "MV_PP2_TXP_SCHED_REFILL_REG");
	mv_pp2x_print_reg(hw, MVPP2_TXP_SCHED_TOKEN_SIZE_REG,
			  "MV_PP2_TXP_SCHED_TOKEN_SIZE_REG");
	mv_pp2x_print_reg(hw, MVPP2_TXP_SCHED_TOKEN_CNTR_REG,
			  "MV_PP2_TXP_SCHED_TOKEN_CNTR_REG");

	for (txq = 0; txq < MVPP2_MAX_TXQ; txq++) {
		DBG_MSG("\n[TxQ Scheduler registers: port=%d, txq=%d]\n",
			port, txq);
		mv_pp2x_print_reg(hw, MVPP2_TXQ_SCHED_REFILL_REG(txq),
				  "MV_PP2_TXQ_SCHED_REFILL_REG");
		mv_pp2x_print_reg(hw, MVPP2_TXQ_SCHED_TOKEN_SIZE_REG(txq),
				  "MV_PP2_TXQ_SCHED_TOKEN_SIZE_REG");
		mv_pp2x_print_reg(hw, MVPP2_TXQ_SCHED_TOKEN_CNTR_REG(txq),
				  "MV_PP2_TXQ_SCHED_TOKEN_CNTR_REG");
	}
}
EXPORT_SYMBOL(mvPp2TxSchedRegs);

/* Calculate period and tokens accordingly with required rate and accuracy */
int mvPp2RateCalc(int rate, unsigned int accuracy, unsigned int *pPeriod,
		  unsigned int *pTokens)
{
	/* Calculate refill tokens and period - rate [Kbps] =
	 * tokens [bits] * 1000 / period [usec]
	 */
	/* Assume:  Tclock [MHz] / BasicRefillNoOfClocks = 1
	*/
	unsigned int period, tokens, calc;

	if (rate == 0) {
		/* Disable traffic from the port: tokens = 0 */
		if (pPeriod != NULL)
			*pPeriod = 1000;

		if (pTokens != NULL)
			*pTokens = 0;

		return 0;
	}

	/* Find values of "period" and "tokens" match "rate" and
	 * "accuracy" when period is minimal
	 */
	for (period = 1; period <= 1000; period++) {
		tokens = 1;
		while (1)	{
			calc = (tokens * 1000) / period;
			if (((MV_ABS(calc - rate) * 100) / rate) <= accuracy) {
				if (pPeriod != NULL)
					*pPeriod = period;

				if (pTokens != NULL)
					*pTokens = tokens;

				return 0;
			}
			if (calc > rate)
				break;

			tokens++;
		}
	}
	return -1;
}

/* Set bandwidth limitation for TX port
 *   rate [Kbps]    - steady state TX bandwidth limitation
 */
int mvPp2TxpRateSet(struct mv_pp2x *priv, int port, int rate)
{
	u32 regVal;
	unsigned int tokens, period, txPortNum, accuracy = 0;
	int status;
	struct mv_pp2x_hw *hw = &priv->hw;
	struct mv_pp2x_port *pp2_port = mv_pp2x_port_struct_get(priv, port);

	if (port >= MVPP2_MAX_PORTS)
		return -1;

	txPortNum = mv_pp2x_egress_port(pp2_port);
	mv_pp2x_write(hw, MVPP2_TXP_SCHED_PORT_INDEX_REG, txPortNum);

	regVal = mv_pp2x_read(hw, MVPP2_TXP_SCHED_PERIOD_REG);

	status = mvPp2RateCalc(rate, accuracy, &period, &tokens);
	if (status != MV_OK) {
		DBG_MSG(
			"%s: Can't provide rate of %d [Kbps] with accuracy of %d [%%]\n",
			__func__, rate, accuracy);
		return status;
	}
	if (tokens > MVPP2_TXP_REFILL_TOKENS_MAX)
		tokens = MVPP2_TXP_REFILL_TOKENS_MAX;

	if (period > MVPP2_TXP_REFILL_PERIOD_MAX)
		period = MVPP2_TXP_REFILL_PERIOD_MAX;

	regVal = mv_pp2x_read(hw, MVPP2_TXP_SCHED_REFILL_REG);

	regVal &= ~MVPP2_TXP_REFILL_TOKENS_ALL_MASK;
	regVal |= MVPP2_TXP_REFILL_TOKENS_MASK(tokens);

	regVal &= ~MVPP2_TXP_REFILL_PERIOD_ALL_MASK;
	regVal |= MVPP2_TXP_REFILL_PERIOD_MASK(period);

	mv_pp2x_write(hw, MVPP2_TXP_SCHED_REFILL_REG, regVal);

	return 0;
}
EXPORT_SYMBOL(mvPp2TxpRateSet);

/* Set maximum burst size for TX port
 *   burst [bytes] - number of bytes to be sent with maximum possible TX rate,
 *                    before TX rate limitation will take place.
 */
int mvPp2TxpBurstSet(struct mv_pp2x *priv, int port, int burst)
{
	u32 size, mtu;
	int txPortNum;
	struct mv_pp2x_hw *hw = &priv->hw;
	struct mv_pp2x_port *pp2_port = mv_pp2x_port_struct_get(priv, port);

	if (port >= MVPP2_MAX_PORTS)
		return -1;

	txPortNum = mv_pp2x_egress_port(pp2_port);
	mv_pp2x_write(hw, MVPP2_TXP_SCHED_PORT_INDEX_REG, txPortNum);

	/* Calculate Token Bucket Size */
	size = 8 * burst;

	if (size > MVPP2_TXP_TOKEN_SIZE_MAX)
		size = MVPP2_TXP_TOKEN_SIZE_MAX;

	/* Token bucket size must be larger then MTU */
	mtu = mv_pp2x_read(hw, MVPP2_TXP_SCHED_MTU_REG);
	if (mtu > size) {
		DBG_MSG("%s Error: Bucket size (%d bytes) < MTU (%d bytes)\n",
			__func__, (size / 8), (mtu / 8));
		return -1;
	}
	mv_pp2x_write(hw, MVPP2_TXP_SCHED_TOKEN_SIZE_REG, size);

	return 0;
}
EXPORT_SYMBOL(mvPp2TxpBurstSet);

/* Set bandwidth limitation for TXQ
 *   rate  [Kbps]  - steady state TX rate limitation
 */
int mvPp2TxqRateSet(struct mv_pp2x *priv, int port, int txq, int rate)
{
	u32		regVal;
	unsigned int	txPortNum, period, tokens, accuracy = 0;
	int	status;
	struct mv_pp2x_hw *hw = &priv->hw;
	struct mv_pp2x_port *pp2_port = mv_pp2x_port_struct_get(priv, port);

	if (port >= MVPP2_MAX_PORTS)
		return -1;

	if (txq >= MVPP2_MAX_TXQ)
		return -1;

	status = mvPp2RateCalc(rate, accuracy, &period, &tokens);
	if (status != MV_OK) {
		DBG_MSG(
			"%s: Can't provide rate of %d [Kbps] with accuracy of %d [%%]\n",
			__func__, rate, accuracy);
		return status;
	}

	txPortNum = mv_pp2x_egress_port(pp2_port);
	mv_pp2x_write(hw, MVPP2_TXP_SCHED_PORT_INDEX_REG, txPortNum);

	if (tokens > MVPP2_TXQ_REFILL_TOKENS_MAX)
		tokens = MVPP2_TXQ_REFILL_TOKENS_MAX;

	if (period > MVPP2_TXQ_REFILL_PERIOD_MAX)
		period = MVPP2_TXQ_REFILL_PERIOD_MAX;

	regVal = mv_pp2x_read(hw, MVPP2_TXQ_SCHED_REFILL_REG(txq));

	regVal &= ~MVPP2_TXQ_REFILL_TOKENS_ALL_MASK;
	regVal |= MVPP2_TXQ_REFILL_TOKENS_MASK(tokens);

	regVal &= ~MVPP2_TXQ_REFILL_PERIOD_ALL_MASK;
	regVal |= MVPP2_TXQ_REFILL_PERIOD_MASK(period);

	mv_pp2x_write(hw, MVPP2_TXQ_SCHED_REFILL_REG(txq), regVal);

	return 0;
}
EXPORT_SYMBOL(mvPp2TxqRateSet);

/* Set maximum burst size for TX port
 *   burst [bytes] - number of bytes to be sent with maximum possible TX rate,
 *                    before TX bandwidth limitation will take place.
 */
int mvPp2TxqBurstSet(struct mv_pp2x *priv, int port, int txq, int burst)
{
	u32  size, mtu;
	int txPortNum;
	struct mv_pp2x_hw *hw = &priv->hw;
	struct mv_pp2x_port *pp2_port = mv_pp2x_port_struct_get(priv, port);

	if (port >= MVPP2_MAX_PORTS)
		return -1;

	if (txq >= MVPP2_MAX_TXQ)
		return -1;

	txPortNum = mv_pp2x_egress_port(pp2_port);
	mv_pp2x_write(hw, MVPP2_TXP_SCHED_PORT_INDEX_REG, txPortNum);

	/* Calculate Tocket Bucket Size */
	size = 8 * burst;

	if (size > MVPP2_TXQ_TOKEN_SIZE_MAX)
		size = MVPP2_TXQ_TOKEN_SIZE_MAX;

	/* Tocken bucket size must be larger then MTU */
	mtu = mv_pp2x_read(hw, MVPP2_TXP_SCHED_MTU_REG);
	if (mtu > size) {
		DBG_MSG(
			"%s Error: Bucket size (%d bytes) < MTU (%d bytes)\n",
			__func__, (size / 8), (mtu / 8));
		return -1;
	}

	mv_pp2x_write(hw, MVPP2_TXQ_SCHED_TOKEN_SIZE_REG(txq), size);

	return 0;
}
EXPORT_SYMBOL(mvPp2TxqBurstSet);

/* Set TXQ to work in FIX priority mode */
int mvPp2TxqFixPrioSet(struct mv_pp2x *priv, int port, int txq)
{
	u32 regVal;
	int txPortNum;
	struct mv_pp2x_hw *hw = &priv->hw;
	struct mv_pp2x_port *pp2_port = mv_pp2x_port_struct_get(priv, port);

	if (port >= MVPP2_MAX_PORTS)
		return -1;

	if (txq >= MVPP2_MAX_TXQ)
		return -1;

	txPortNum = mv_pp2x_egress_port(pp2_port);
	mv_pp2x_write(hw, MVPP2_TXP_SCHED_PORT_INDEX_REG, txPortNum);

	regVal = mv_pp2x_read(hw, MVPP2_TXP_SCHED_FIXED_PRIO_REG);
	regVal |= (1 << txq);
	mv_pp2x_write(hw, MVPP2_TXP_SCHED_FIXED_PRIO_REG, regVal);

	return MV_OK;
}
EXPORT_SYMBOL(mvPp2TxqFixPrioSet);

/* Set TXQ to work in WRR mode and set relative weight. */
/*   Weight range [1..N] */
int mvPp2TxqWrrPrioSet(struct mv_pp2x *priv, int port, int txq, int weight)
{
	u32 regVal, mtu, mtu_aligned, weight_min;
	int txPortNum;
	struct mv_pp2x_hw *hw = &priv->hw;
	struct mv_pp2x_port *pp2_port = mv_pp2x_port_struct_get(priv, port);

	if (port >= MVPP2_MAX_PORTS)
		return -1;

	if (txq >= MVPP2_MAX_TXQ)
		return -1;

	txPortNum = mv_pp2x_egress_port(pp2_port);
	mv_pp2x_write(hw, MVPP2_TXP_SCHED_PORT_INDEX_REG, txPortNum);

	/* Weight * 256 bytes * 8 bits must be larger then MTU [bits] */
	mtu = mv_pp2x_read(hw, MVPP2_TXP_SCHED_MTU_REG);

	/* WA for wrong Token bucket update: Set MTU value =
	 * 3*real MTU value, now get read MTU
	 */
	mtu /= MV_AMPLIFY_FACTOR_MTU;
	mtu /= MV_BIT_NUM_OF_BYTE; /* move to bytes */
	mtu_aligned = MV_ALIGN_UP(mtu, MV_WRR_WEIGHT_UNIT);
	weight_min = mtu_aligned / MV_WRR_WEIGHT_UNIT;

	if ((weight < weight_min) || (weight > MVPP2_TXQ_WRR_WEIGHT_MAX)) {
		DBG_MSG("%s Error: weight=%d is out of range %d...%d\n",
			__func__, weight, weight_min,
			MVPP2_TXQ_WRR_WEIGHT_MAX);
		return -1;
	}

	regVal = mv_pp2x_read(hw, MVPP2_TXQ_SCHED_WRR_REG(txq));

	regVal &= ~MVPP2_TXQ_WRR_WEIGHT_ALL_MASK;
	regVal |= MVPP2_TXQ_WRR_WEIGHT_MASK(weight);
	mv_pp2x_write(hw, MVPP2_TXQ_SCHED_WRR_REG(txq), regVal);

	regVal = mv_pp2x_read(hw, MVPP2_TXP_SCHED_FIXED_PRIO_REG);
	regVal &= ~(1 << txq);
	mv_pp2x_write(hw, MVPP2_TXP_SCHED_FIXED_PRIO_REG, regVal);

	return 0;
}
EXPORT_SYMBOL(mvPp2TxqWrrPrioSet);

void mvPp2V1RxqDbgCntrs(struct mv_pp2x *priv, int port, int rxq)
{
	struct mv_pp2x_port *pp_port;
	int phy_rxq;
	struct mv_pp2x_hw *hw = &priv->hw;

	pp_port = mv_pp2x_port_struct_get(priv, port);
	if (pp_port)
		phy_rxq = pp_port->first_rxq + rxq;
	else
		return;

	DBG_MSG("\n------ [Port #%d, rxq #%d counters] -----\n", port, rxq);
	mv_pp2x_write(hw, MVPP2_CNT_IDX_REG, phy_rxq);
	mv_pp2x_print_reg(hw, MVPP2_RX_PKT_FULLQ_DROP_REG,
			  "MV_PP2_RX_PKT_FULLQ_DROP_REG");
	mv_pp2x_print_reg(hw, MVPP2_RX_PKT_EARLY_DROP_REG,
			  "MVPP2_V1_RX_PKT_EARLY_DROP_REG");
	mv_pp2x_print_reg(hw, MVPP2_RX_PKT_BM_DROP_REG,
			  "MVPP2_V1_RX_PKT_BM_DROP_REG");
	mv_pp2x_print_reg(hw, MVPP2_RX_DESC_ENQ_REG,
			  "MVPP2_V1_RX_DESC_ENQ_REG");
}
EXPORT_SYMBOL(mvPp2V1RxqDbgCntrs);

void mvPp2RxFifoRegs(struct mv_pp2x_hw *hw, int port)
{
	DBG_MSG("\n[Port #%d RX Fifo]\n", port);
	mv_pp2x_print_reg(hw, MVPP2_RX_DATA_FIFO_SIZE_REG(port),
			  "MVPP2_RX_DATA_FIFO_SIZE_REG");
	mv_pp2x_print_reg(hw, MVPP2_RX_ATTR_FIFO_SIZE_REG(port),
			  "MVPP2_RX_ATTR_FIFO_SIZE_REG");
	DBG_MSG("\n[Global RX Fifo regs]\n");
	mv_pp2x_print_reg(hw, MVPP2_RX_MIN_PKT_SIZE_REG,
			  "MVPP2_RX_MIN_PKT_SIZE_REG");
}
EXPORT_SYMBOL(mvPp2RxFifoRegs);

static char *mv_pp2x_prs_l2_info_str(unsigned int l2_info)
{
	switch (l2_info << MVPP2_PRS_RI_L2_CAST_OFFS) {
	case MVPP2_PRS_RI_L2_UCAST:
		return "Ucast";
	case MVPP2_PRS_RI_L2_MCAST:
		return "Mcast";
	case MVPP2_PRS_RI_L2_BCAST:
		return "Bcast";
	default:
		return "Unknown";
	}
	return NULL;
}

static char *mv_pp2x_prs_vlan_info_str(unsigned int vlan_info)
{
	switch (vlan_info << MVPP2_PRS_RI_VLAN_OFFS) {
	case MVPP2_PRS_RI_VLAN_NONE:
		return "None";
	case MVPP2_PRS_RI_VLAN_SINGLE:
		return "Single";
	case MVPP2_PRS_RI_VLAN_DOUBLE:
		return "Double";
	case MVPP2_PRS_RI_VLAN_TRIPLE:
		return "Triple";
	default:
		return "Unknown";
	}
	return NULL;
}

void mv_pp2x_rx_desc_print(struct mv_pp2x *priv, struct mv_pp2x_rx_desc *desc)
{
	int i;
	u32 *words = (u32 *) desc;

	DBG_MSG("RX desc - %p: ", desc);
	for (i = 0; i < 8; i++)
		DBG_MSG("%8.8x ", *words++);
	DBG_MSG("\n");

	DBG_MSG("pkt_size=%d, L3_offs=%d, IP_hlen=%d, ",
	       desc->data_size,
	       (desc->status & MVPP2_RXD_L3_OFFSET_MASK) >>
			MVPP2_RXD_L3_OFFSET_OFFS,
	       (desc->status & MVPP2_RXD_IP_HLEN_MASK) >>
			MVPP2_RXD_IP_HLEN_OFFS);

	DBG_MSG("L2=%s, ",
		mv_pp2x_prs_l2_info_str((desc->rsrvd_parser &
			MVPP2_RXD_L2_CAST_MASK) >> MVPP2_RXD_L2_CAST_OFFS));

	DBG_MSG("VLAN=");
	DBG_MSG("%s, ",
		mv_pp2x_prs_vlan_info_str((desc->rsrvd_parser &
			MVPP2_RXD_VLAN_INFO_MASK) >> MVPP2_RXD_VLAN_INFO_OFFS));

	DBG_MSG("L3=");
	if (MVPP2_RXD_L3_IS_IP4(desc->status))
		DBG_MSG("IPv4 (hdr=%s), ",
			MVPP2_RXD_IP4_HDR_ERR(desc->status) ? "bad" : "ok");
	else if (MVPP2_RXD_L3_IS_IP4_OPT(desc->status))
		DBG_MSG("IPv4 Options (hdr=%s), ",
			MVPP2_RXD_IP4_HDR_ERR(desc->status) ? "bad" : "ok");
	else if (MVPP2_RXD_L3_IS_IP4_OTHER(desc->status))
		DBG_MSG("IPv4 Other (hdr=%s), ",
			MVPP2_RXD_IP4_HDR_ERR(desc->status) ? "bad" : "ok");
	else if (MVPP2_RXD_L3_IS_IP6(desc->status))
		DBG_MSG("IPv6, ");
	else if (MVPP2_RXD_L3_IS_IP6_EXT(desc->status))
		DBG_MSG("IPv6 Ext, ");
	else
		DBG_MSG("Unknown, ");

	if (desc->status & MVPP2_RXD_IP_FRAG_MASK)
		DBG_MSG("Frag, ");

	DBG_MSG("L4=");
	if (MVPP2_RXD_L4_IS_TCP(desc->status))
		DBG_MSG("TCP (csum=%s)", (desc->status &
			MVPP2_RXD_L4_CHK_OK_MASK) ? "Ok" : "Bad");
	else if (MVPP2_RXD_L4_IS_UDP(desc->status))
		DBG_MSG("UDP (csum=%s)", (desc->status &
			MVPP2_RXD_L4_CHK_OK_MASK) ? "Ok" : "Bad");
	else
		DBG_MSG("Unknown");

	DBG_MSG("\n");

	DBG_MSG("Lookup_ID=0x%x, cpu_code=0x%x\n",
		(desc->rsrvd_parser &
			MVPP2_RXD_LKP_ID_MASK) >> MVPP2_RXD_LKP_ID_OFFS,
		(desc->rsrvd_parser &
			MVPP2_RXD_CPU_CODE_MASK) >> MVPP2_RXD_CPU_CODE_OFFS);

	if (priv->pp2_version == PPV22) {
		DBG_MSG("buf_phys_addr = 0x%llx\n",
			desc->u.pp22.buf_phys_addr_key_hash &
			DMA_BIT_MASK(40));
		DBG_MSG("buf_virt_addr = 0x%llx\n",
			desc->u.pp22.buf_cookie_bm_qset_cls_info &
			DMA_BIT_MASK(40));
	}
}

/* Dump memory in specific format:
 * address: X1X1X1X1 X2X2X2X2 ... X8X8X8X8
 */
void mv_pp2x_skb_dump(struct sk_buff *skb, int size, int access)
{
	int i, j;
	void *addr = skb->head + NET_SKB_PAD;
	uintptr_t memAddr = (uintptr_t)addr;

	DBG_MSG("skb=%p, buf=%p, ksize=%d\n", skb, skb->head,
			(int)ksize(skb->head));

	if (access == 0)
		access = 1;

	if ((access != 4) && (access != 2) && (access != 1)) {
		DBG_MSG("%d wrong access size. Access must be 1 or 2 or 4\n",
				access);
		return;
	}
	memAddr = MV_ALIGN_DOWN((uintptr_t)addr, 4);
	size = MV_ALIGN_UP(size, 4);
	addr = (void *)MV_ALIGN_DOWN((uintptr_t)addr, access);
	while (size > 0) {
		DBG_MSG("%08lx: ", memAddr);
		i = 0;
		/* 32 bytes in the line */
		while (i < 32) {
			if (memAddr >= (uintptr_t)addr) {
				switch (access) {
				case 1:
					DBG_MSG("%02x ",
						MV_MEMIO8_READ(memAddr));
					break;

				case 2:
					DBG_MSG("%04x ",
						MV_MEMIO16_READ(memAddr));
					break;

				case 4:
					DBG_MSG("%08x ",
						MV_MEMIO32_READ(memAddr));
					break;
				}
			} else {
				for (j = 0; j < (access * 2 + 1); j++)
					DBG_MSG(" ");
			}
			i += access;
			memAddr += access;
			size -= access;
			if (size <= 0)
				break;
		}
		DBG_MSG("\n");
	}
}

/* Wrap the API just for debug */
int mv_pp2x_wrap_cos_mode_set(struct mv_pp2x_port *port,
			      enum mv_pp2x_cos_classifier cos_mode)
{
	return mv_pp2x_cos_classifier_set(port, cos_mode);
}
EXPORT_SYMBOL(mv_pp2x_wrap_cos_mode_set);

int mv_pp2x_wrap_cos_mode_get(struct mv_pp2x_port *port)
{
	return mv_pp2x_cos_classifier_get(port);
}
EXPORT_SYMBOL(mv_pp2x_wrap_cos_mode_get);

int mv_pp2x_wrap_cos_pri_map_set(struct mv_pp2x_port *port, int cos_pri_map)
{
	return mv_pp2x_cos_pri_map_set(port, cos_pri_map);
}
EXPORT_SYMBOL(mv_pp2x_wrap_cos_pri_map_set);

int mv_pp2x_wrap_cos_pri_map_get(struct mv_pp2x_port *port)
{
	return mv_pp2x_cos_pri_map_get(port);
}
EXPORT_SYMBOL(mv_pp2x_wrap_cos_pri_map_get);

int mv_pp2x_wrap_cos_dflt_value_set(struct mv_pp2x_port *port, int cos_value)
{
	return mv_pp2x_cos_default_value_set(port, cos_value);
}
EXPORT_SYMBOL(mv_pp2x_wrap_cos_dflt_value_set);

int mv_pp2x_wrap_cos_dflt_value_get(struct mv_pp2x_port *port)
{
	return mv_pp2x_cos_default_value_get(port);
}
EXPORT_SYMBOL(mv_pp2x_wrap_cos_dflt_value_get);

int mv_pp22_wrap_rss_mode_set(struct mv_pp2x_port *port, int rss_mode)
{
	return mv_pp22_rss_mode_set(port, rss_mode);
}
EXPORT_SYMBOL(mv_pp22_wrap_rss_mode_set);

int mv_pp22_wrap_rss_dflt_cpu_set(struct mv_pp2x_port *port, int default_cpu)
{
	return mv_pp22_rss_default_cpu_set(port, default_cpu);
}
EXPORT_SYMBOL(mv_pp22_wrap_rss_dflt_cpu_set);

/* mv_pp2x_port_bind_cpu_set
*  -- Bind the port to cpu when rss disabled.
*/
int mv_pp2x_port_bind_cpu_set(struct mv_pp2x_port *port, u8 bind_cpu)
{
	int ret = 0;
	u8 bound_cpu_first_rxq;

	if (port->priv->pp2_cfg.rss_cfg.rss_en) {
		netdev_err(port->dev,
			"cannot bind cpu to port when rss is enabled\n");
		return -EINVAL;
	}

	if (!(port->priv->cpu_map & (1 << bind_cpu))) {
		netdev_err(port->dev, "invalid cpu(%d)\n", bind_cpu);
		return -EINVAL;
	}

	/* Check original cpu and new cpu is same or not */
	if (bind_cpu != ((port->priv->pp2_cfg.rx_cpu_map >> (port->id * 4)) &
	    0xF)) {
		port->priv->pp2_cfg.rx_cpu_map &= (~(0xF << (port->id * 4)));
		port->priv->pp2_cfg.rx_cpu_map |= ((bind_cpu & 0xF) <<
						   (port->id * 4));
		bound_cpu_first_rxq = mv_pp2x_bound_cpu_first_rxq_calc(port);
		ret = mv_pp2x_cls_c2_rule_set(port, bound_cpu_first_rxq);
	}

	return ret;
}
EXPORT_SYMBOL(mv_pp2x_port_bind_cpu_set);

static void mv_pp2x_bm_queue_map_dump(struct mv_pp2x_hw *hw, int queue)
{
	unsigned int regVal, shortQset, longQset;

	DBG_MSG("-------- queue #%d --------\n", queue);

	mv_pp2x_write(hw, MVPP2_BM_PRIO_IDX_REG, queue);
	regVal = mv_pp2x_read(hw, MVPP2_BM_CPU_QSET_REG);

	shortQset = ((regVal & (MVPP2_BM_CPU_SHORT_QSET_MASK)) >>
		    MVPP2_BM_CPU_SHORT_QSET_OFFS);
	longQset = ((regVal & (MVPP2_BM_CPU_LONG_QSET_MASK)) >>
		    MVPP2_BM_CPU_LONG_QSET_OFFS);
	DBG_MSG("CPU SHORT QSET = 0x%02x\n", shortQset);
	DBG_MSG("CPU LONG QSET  = 0x%02x\n", longQset);

	regVal = mv_pp2x_read(hw, MVPP2_BM_HWF_QSET_REG);
	shortQset = ((regVal & (MVPP2_BM_HWF_SHORT_QSET_MASK)) >>
		    MVPP2_BM_HWF_SHORT_QSET_OFFS);
	longQset = ((regVal & (MVPP2_BM_HWF_LONG_QSET_MASK)) >>
		    MVPP2_BM_HWF_LONG_QSET_OFFS);
	DBG_MSG("HWF SHORT QSET = 0x%02x\n", shortQset);
	DBG_MSG("HWF LONG QSET  = 0x%02x\n", longQset);
}

static bool mv_pp2x_bm_priority_en(struct mv_pp2x_hw *hw)
{
	return ((mv_pp2x_read(hw, MVPP2_BM_PRIO_CTRL_REG) == 0) ? false : true);
}

void mv_pp2x_bm_queue_map_dump_all(struct mv_pp2x_hw *hw)
{
	int queue;

	if (!mv_pp2x_bm_priority_en(hw))
		DBG_MSG("Note: The buffers priority algorithms is disabled.\n");

	for (queue = 0; queue <= MVPP2_BM_PRIO_IDX_MAX; queue++)
		mv_pp2x_bm_queue_map_dump(hw, queue);
}
EXPORT_SYMBOL(mv_pp2x_bm_queue_map_dump_all);

int mv_pp2x_cls_c2_qos_prio_set(struct mv_pp2x_cls_c2_qos_entry *qos, u8 pri)
{
	if (!qos)
		return -EINVAL;

	qos->data &= ~MVPP2_CLS2_QOS_TBL_PRI_MASK;
	qos->data |= (((u32)pri) << MVPP2_CLS2_QOS_TBL_PRI_OFF);
	return 0;
}
EXPORT_SYMBOL(mv_pp2x_cls_c2_qos_prio_set);

int mv_pp2x_cls_c2_qos_dscp_set(struct mv_pp2x_cls_c2_qos_entry *qos, u8 dscp)
{
	if (!qos)
		return -EINVAL;

	qos->data &= ~MVPP2_CLS2_QOS_TBL_DSCP_MASK;
	qos->data |= (((u32)dscp) << MVPP2_CLS2_QOS_TBL_DSCP_OFF);
	return 0;
}
EXPORT_SYMBOL(mv_pp2x_cls_c2_qos_dscp_set);

int mv_pp2x_cls_c2_qos_color_set(struct mv_pp2x_cls_c2_qos_entry *qos, u8 color)
{
	if (!qos)
		return -EINVAL;

	qos->data &= ~MVPP2_CLS2_QOS_TBL_COLOR_MASK;
	qos->data |= (((u32)color) << MVPP2_CLS2_QOS_TBL_COLOR_OFF);
	return 0;
}
EXPORT_SYMBOL(mv_pp2x_cls_c2_qos_color_set);

int mv_pp2x_cls_c2_queue_set(struct mv_pp2x_cls_c2_entry *c2, int cmd,
		int queue, int from)
{
	int status = 0;
	int qHigh, qLow;

	/* cmd validation in set functions */

	qHigh = (queue & MVPP2_CLS2_ACT_QOS_ATTR_QH_MASK) >>
		MVPP2_CLS2_ACT_QOS_ATTR_QH_OFF;
	qLow = (queue & MVPP2_CLS2_ACT_QOS_ATTR_QL_MASK) >>
		MVPP2_CLS2_ACT_QOS_ATTR_QL_OFF;

	status |= mv_pp2x_cls_c2_queue_low_set(c2, cmd, qLow, from);
	status |= mv_pp2x_cls_c2_queue_high_set(c2, cmd, qHigh, from);

	return status;
}
EXPORT_SYMBOL(mv_pp2x_cls_c2_queue_set);

int mv_pp2x_cls_c2_mtu_set(struct mv_pp2x_cls_c2_entry *c2, int mtu_inx)
{
	if (mv_pp2x_ptr_validate(c2) == MV_ERROR)
		return MV_ERROR;

	if (mv_pp2x_range_validate(mtu_inx, 0,
	    (1 << MVPP2_CLS2_ACT_HWF_ATTR_MTUIDX_BITS) - 1) == MV_ERROR)
		return MV_ERROR;

	c2->sram.regs.hwf_attr &= ~MVPP2_CLS2_ACT_HWF_ATTR_MTUIDX_MASK;
	c2->sram.regs.hwf_attr |= (mtu_inx <<
				  MVPP2_CLS2_ACT_HWF_ATTR_MTUIDX_OFF);

	return MV_OK;
}
EXPORT_SYMBOL(mv_pp2x_cls_c2_mtu_set);

int mv_pp2x_debug_param_set(u32 param)
{
	debug_param = param;
	return 0;
}
EXPORT_SYMBOL(mv_pp2x_debug_param_set);

int mv_pp2x_debug_param_get(void)
{
	return debug_param;
}
EXPORT_SYMBOL(mv_pp2x_debug_param_get);


static int mv_pp2x_prs_hw_tcam_cnt_dump(struct mv_pp2x_hw *hw,
					int tid, unsigned int *cnt)
{
	unsigned int regVal;

	if (mv_pp2x_range_validate(tid, 0,
	    MVPP2_PRS_TCAM_SRAM_SIZE - 1) == MV_ERROR)
		return MV_ERROR;

	/* write index */
	mv_pp2x_write(hw, MVPP2_PRS_TCAM_HIT_IDX_REG, tid);

	regVal = mv_pp2x_read(hw, MVPP2_PRS_TCAM_HIT_CNT_REG);
	regVal &= MVPP2_PRS_TCAM_HIT_CNT_MASK;

	if (cnt)
		*cnt = regVal;
	else
		DBG_MSG("HIT COUNTER: %d\n", regVal);

	return MV_OK;
}

static int mv_pp2x_prs_sw_sram_ri_dump(struct mv_pp2x_prs_entry *pe)
{
	unsigned int data, mask;
	int i, bitsOffs = 0;
	char bits[100];

	if (mv_pp2x_ptr_validate(pe) == MV_ERROR)
		return MV_ERROR;

	mv_pp2x_prs_sw_sram_ri_get(pe, &data, &mask);
	if (mask == 0)
		return 0;

	DBG_MSG("\n       ");

	DBG_MSG("S_RI=");
	for (i = (MVPP2_PRS_SRAM_RI_CTRL_BITS-1); i > -1 ; i--)
		if (mask & (1 << i)) {
			DBG_MSG("%d", ((data & (1 << i)) != 0));
			bitsOffs += sprintf(bits + bitsOffs, "%d:", i);
		} else
			DBG_MSG("x");

	bits[bitsOffs] = '\0';
	DBG_MSG(" %s", bits);

	return 0;
}

static int mv_pp2x_prs_sw_sram_ai_dump(struct mv_pp2x_prs_entry *pe)
{
	int i, bitsOffs = 0;
	unsigned int data, mask;
	char bits[30];

	if (mv_pp2x_ptr_validate(pe) == MV_ERROR)
		return MV_ERROR;

	mv_pp2x_prs_sw_sram_ai_get(pe, &data, &mask);

	if (mask == 0)
		return 0;

	DBG_MSG("\n       ");

	DBG_MSG("S_AI=");
	for (i = (MVPP2_PRS_SRAM_AI_CTRL_BITS-1); i > -1 ; i--)
		if (mask & (1 << i)) {
			DBG_MSG("%d", ((data & (1 << i)) != 0));
			bitsOffs += sprintf(bits + bitsOffs, "%d:", i);
		} else
			DBG_MSG("x");
	bits[bitsOffs] = '\0';
	DBG_MSG(" %s", bits);
	return 0;
}

int mv_pp2x_prs_sw_dump(struct mv_pp2x_prs_entry *pe)
{
	u32 op, type, lu, done, flowid;
	int	shift, offset, i;

	if (mv_pp2x_ptr_validate(pe) == MV_ERROR)
		return MV_ERROR;

	/* hw entry id */
	DBG_MSG("[%4d] ", pe->index);

	i = MVPP2_PRS_TCAM_WORDS - 1;
	DBG_MSG("%1.1x ", pe->tcam.word[i--] & 0xF);

	while (i >= 0)
		DBG_MSG("%4.4x ", (pe->tcam.word[i--]) & 0xFFFF);

	DBG_MSG("| ");

	/*DBG_MSG(PRS_SRAM_FMT, PRS_SRAM_VAL(pe->sram.word)); */
	DBG_MSG("%4.4x %8.8x %8.8x %8.8x", pe->sram.word[3] & 0xFFFF,
		 pe->sram.word[2],  pe->sram.word[1],  pe->sram.word[0]);

	DBG_MSG("\n       ");

	i = MVPP2_PRS_TCAM_WORDS - 1;
	DBG_MSG("%1.1x ", (pe->tcam.word[i--] >> 16) & 0xF);

	while (i >= 0)
		DBG_MSG("%4.4x ", ((pe->tcam.word[i--]) >> 16)  & 0xFFFF);

	DBG_MSG("| ");

	mv_pp2x_prs_sw_sram_shift_get(pe, &shift);
	DBG_MSG("SH=%d ", shift);

	mv_pp2x_prs_sw_sram_offset_get(pe, &type, &offset, &op);
	if (offset != 0 || ((op >> MVPP2_PRS_SRAM_OP_SEL_SHIFT_BITS) != 0))
		DBG_MSG("UDFT=%u UDFO=%d ", type, offset);

	DBG_MSG("op=%u ", op);

	mv_pp2x_prs_sw_sram_next_lu_get(pe, &lu);
	DBG_MSG("LU=%u ", lu);

	mv_pp2x_prs_sw_sram_lu_done_get(pe, &done);
	DBG_MSG("%s ", done ? "DONE" : "N_DONE");

	/*flow id generation bit*/
	mv_pp2x_prs_sw_sram_flowid_gen_get(pe, &flowid);
	DBG_MSG("%s ", flowid ? "FIDG" : "N_FIDG");

	if ((pe->tcam.word[MVPP2_PRS_TCAM_INV_WORD] & MVPP2_PRS_TCAM_INV_MASK))
		DBG_MSG(" [inv]");

	if (mv_pp2x_prs_sw_sram_ri_dump(pe))
		return MV_ERROR;

	if (mv_pp2x_prs_sw_sram_ai_dump(pe))
		return MV_ERROR;

	DBG_MSG("\n");

	return 0;

}
EXPORT_SYMBOL(mv_pp2x_prs_sw_dump);

int mv_pp2x_prs_hw_dump(struct mv_pp2x_hw *hw)
{
	int index;
	struct mv_pp2x_prs_entry pe;


	DBG_MSG("%s\n", __func__);

	for (index = 0; index < MVPP2_PRS_TCAM_SRAM_SIZE; index++) {
		pe.index = index;
		mv_pp2x_prs_hw_read(hw, &pe);
		if ((pe.tcam.word[MVPP2_PRS_TCAM_INV_WORD] &
			MVPP2_PRS_TCAM_INV_MASK) ==
			MVPP2_PRS_TCAM_ENTRY_VALID) {
			mv_pp2x_prs_sw_dump(&pe);
			mv_pp2x_prs_hw_tcam_cnt_dump(hw, index, NULL);
			DBG_MSG("-----------------------------------------\n");
		}
	}

	return MV_OK;
}
EXPORT_SYMBOL(mv_pp2x_prs_hw_dump);

int mv_pp2x_prs_hw_regs_dump(struct mv_pp2x_hw *hw)
{
	int i;
	char reg_name[100];

	mv_pp2x_print_reg(hw, MVPP2_PRS_INIT_LOOKUP_REG,
			  "MVPP2_PRS_INIT_LOOKUP_REG");
	mv_pp2x_print_reg(hw, MVPP2_PRS_INIT_OFFS_REG(0),
			  "MVPP2_PRS_INIT_OFFS_0_3_REG");
	mv_pp2x_print_reg(hw, MVPP2_PRS_INIT_OFFS_REG(4),
			  "MVPP2_PRS_INIT_OFFS_4_7_REG");
	mv_pp2x_print_reg(hw, MVPP2_PRS_MAX_LOOP_REG(0),
			  "MVPP2_PRS_MAX_LOOP_0_3_REG");
	mv_pp2x_print_reg(hw, MVPP2_PRS_MAX_LOOP_REG(4),
			  "MVPP2_PRS_MAX_LOOP_4_7_REG");

	/*mv_pp2x_print_reg(hw, MVPP2_PRS_INTR_CAUSE_REG,
	 *		     "MVPP2_PRS_INTR_CAUSE_REG");
	 */
	/*mv_pp2x_print_reg(hw, MVPP2_PRS_INTR_MASK_REG,
	 *		     "MVPP2_PRS_INTR_MASK_REG");
	 */
	mv_pp2x_print_reg(hw, MVPP2_PRS_TCAM_IDX_REG,
			  "MVPP2_PRS_TCAM_IDX_REG");

	for (i = 0; i < MVPP2_PRS_TCAM_WORDS; i++) {
		sprintf(reg_name, "MVPP2_PRS_TCAM_DATA_%d_REG", i);
		mv_pp2x_print_reg(hw, MVPP2_PRS_TCAM_DATA_REG(i),
			reg_name);
	}
	mv_pp2x_print_reg(hw, MVPP2_PRS_SRAM_IDX_REG,
			  "MVPP2_PRS_SRAM_IDX_REG");

	for (i = 0; i < MVPP2_PRS_SRAM_WORDS; i++) {
		sprintf(reg_name, "MVPP2_PRS_SRAM_DATA_%d_REG", i);
		mv_pp2x_print_reg(hw, MVPP2_PRS_SRAM_DATA_REG(i),
			reg_name);
	}

	mv_pp2x_print_reg(hw, MVPP2_PRS_EXP_REG,
			  "MVPP2_PRS_EXP_REG");
	mv_pp2x_print_reg(hw, MVPP2_PRS_TCAM_CTRL_REG,
			  "MVPP2_PRS_TCAM_CTRL_REG");

	return MV_OK;
}
EXPORT_SYMBOL(mv_pp2x_prs_hw_regs_dump);


int mv_pp2x_prs_hw_hits_dump(struct mv_pp2x_hw *hw)
{
	int index;
	unsigned int cnt;
	struct mv_pp2x_prs_entry pe;

	for (index = 0; index < MVPP2_PRS_TCAM_SRAM_SIZE; index++) {
		pe.index = index;
		mv_pp2x_prs_hw_read(hw, &pe);
		if ((pe.tcam.word[MVPP2_PRS_TCAM_INV_WORD] &
			MVPP2_PRS_TCAM_INV_MASK) ==
			MVPP2_PRS_TCAM_ENTRY_VALID) {
			mv_pp2x_prs_hw_tcam_cnt_dump(hw, index, &cnt);
			if (cnt == 0)
				continue;
			mv_pp2x_prs_sw_dump(&pe);
			DBG_MSG("INDEX: %d       HITS: %d\n", index, cnt);
			DBG_MSG("-----------------------------------------\n");
		}
	}
	return MV_OK;
}
EXPORT_SYMBOL(mv_pp2x_prs_hw_hits_dump);

int mvPp2ClsC2QosSwDump(struct mv_pp2x_cls_c2_qos_entry *qos)
{
	int int32bit;
	int status = 0;

	if (mv_pp2x_ptr_validate(qos) == MV_ERROR)
		return MV_ERROR;

	DBG_MSG(
	"TABLE	SEL	LINE	PRI	DSCP	COLOR	GEM_ID	QUEUE\n");

	/* table id */
	DBG_MSG("0x%2.2x\t", qos->tbl_id);

	/* table sel */
	DBG_MSG("0x%1.1x\t", qos->tbl_sel);

	/* table line */
	DBG_MSG("0x%2.2x\t", qos->tbl_line);

	/* priority */
	status |= mvPp2ClsC2QosPrioGet(qos, &int32bit);
	DBG_MSG("0x%1.1x\t", int32bit);

	/* dscp */
	status |= mvPp2ClsC2QosDscpGet(qos, &int32bit);
	DBG_MSG("0x%2.2x\t", int32bit);

	/* color */
	status |= mvPp2ClsC2QosColorGet(qos, &int32bit);
	DBG_MSG("0x%1.1x\t", int32bit);

	/* gem port id */
	status |= mvPp2ClsC2QosGpidGet(qos, &int32bit);
	DBG_MSG("0x%3.3x\t", int32bit);

	/* queue */
	status |= mvPp2ClsC2QosQueueGet(qos, &int32bit);
	DBG_MSG("0x%2.2x", int32bit);

	DBG_MSG("\n");

	return status;
}
/*----------------------------------------------------------------------*/

int mv_pp2x_cls_c2_qos_dscp_hw_dump(struct mv_pp2x_hw *hw)
{
	int tbl_id, tbl_line, int32bit;
	struct mv_pp2x_cls_c2_qos_entry qos;

	for (tbl_id = 0; tbl_id < MVPP2_CLS_C2_QOS_DSCP_TBL_NUM; tbl_id++) {

		DBG_MSG("\n------------ DSCP TABLE %d ------------\n", tbl_id);
		DBG_MSG("LINE	DSCP	COLOR	GEM_ID	QUEUE\n");
		for (tbl_line = 0; tbl_line < MVPP2_CLS_C2_QOS_DSCP_TBL_SIZE;
				tbl_line++) {
			mv_pp2x_cls_c2_qos_hw_read(hw, tbl_id,
				1/*DSCP*/, tbl_line, &qos);
			DBG_MSG("0x%2.2x\t", qos.tbl_line);
			mvPp2ClsC2QosDscpGet(&qos, &int32bit);
			DBG_MSG("0x%2.2x\t", int32bit);
			mvPp2ClsC2QosColorGet(&qos, &int32bit);
			DBG_MSG("0x%1.1x\t", int32bit);
			mvPp2ClsC2QosGpidGet(&qos, &int32bit);
			DBG_MSG("0x%3.3x\t", int32bit);
			mvPp2ClsC2QosQueueGet(&qos, &int32bit);
			DBG_MSG("0x%2.2x", int32bit);
			DBG_MSG("\n");
		}
	}
	return MV_OK;
}
EXPORT_SYMBOL(mv_pp2x_cls_c2_qos_dscp_hw_dump);

/*----------------------------------------------------------------------*/

int mv_pp2x_cls_c2_qos_prio_hw_dump(struct mv_pp2x_hw *hw)
{
	int tbl_id, tbl_line, int32bit;

	struct mv_pp2x_cls_c2_qos_entry qos;

	for (tbl_id = 0; tbl_id < MVPP2_CLS_C2_QOS_PRIO_TBL_NUM; tbl_id++) {

		DBG_MSG("\n-------- PRIORITY TABLE %d -----------\n", tbl_id);
		DBG_MSG("LINE	PRIO	COLOR	GEM_ID	QUEUE\n");

		for (tbl_line = 0; tbl_line < MVPP2_CLS_C2_QOS_PRIO_TBL_SIZE;
				tbl_line++) {
			mv_pp2x_cls_c2_qos_hw_read(hw, tbl_id,
				0/*PRIO*/, tbl_line, &qos);
			DBG_MSG("0x%2.2x\t", qos.tbl_line);
			mvPp2ClsC2QosPrioGet(&qos, &int32bit);
			DBG_MSG("0x%1.1x\t", int32bit);
			mvPp2ClsC2QosColorGet(&qos, &int32bit);
			DBG_MSG("0x%1.1x\t", int32bit);
			mvPp2ClsC2QosGpidGet(&qos, &int32bit);
			DBG_MSG("0x%3.3x\t", int32bit);
			mvPp2ClsC2QosQueueGet(&qos, &int32bit);
			DBG_MSG("0x%2.2x", int32bit);
			DBG_MSG("\n");
		}
	}
	return MV_OK;
}
EXPORT_SYMBOL(mv_pp2x_cls_c2_qos_prio_hw_dump);

/*----------------------------------------------------------------------*/

int mv_pp2x_cls_c2_sw_dump(struct mv_pp2x_cls_c2_entry *c2)
{
	int id, sel, type, gemid, low_q, high_q, color, int32bit;

	if (mv_pp2x_ptr_validate(c2) == MV_ERROR)
		return MV_ERROR;

	mv_pp2x_cls_c2_sw_words_dump(c2);
	DBG_MSG("\n");

	/*------------------------------*/
	/*	action_tbl 0x1B30	*/
	/*------------------------------*/

	id = ((c2->sram.regs.action_tbl &
	      (MVPP2_CLS2_ACT_DATA_TBL_ID_MASK)) >>
	       MVPP2_CLS2_ACT_DATA_TBL_ID_OFF);
	sel = ((c2->sram.regs.action_tbl &
	       (MVPP2_CLS2_ACT_DATA_TBL_SEL_MASK)) >>
		MVPP2_CLS2_ACT_DATA_TBL_SEL_OFF);
	type = ((c2->sram.regs.action_tbl &
	       (MVPP2_CLS2_ACT_DATA_TBL_PRI_DSCP_MASK)) >>
		MVPP2_CLS2_ACT_DATA_TBL_PRI_DSCP_OFF);
	gemid = ((c2->sram.regs.action_tbl &
		 (MVPP2_CLS2_ACT_DATA_TBL_GEM_ID_MASK)) >>
		  MVPP2_CLS2_ACT_DATA_TBL_GEM_ID_OFF);
	low_q = ((c2->sram.regs.action_tbl &
		 (MVPP2_CLS2_ACT_DATA_TBL_LOW_Q_MASK)) >>
		  MVPP2_CLS2_ACT_DATA_TBL_LOW_Q_OFF);
	high_q = ((c2->sram.regs.action_tbl &
		  (MVPP2_CLS2_ACT_DATA_TBL_HIGH_Q_MASK)) >>
		   MVPP2_CLS2_ACT_DATA_TBL_HIGH_Q_OFF);
	color = ((c2->sram.regs.action_tbl &
		 (MVPP2_CLS2_ACT_DATA_TBL_COLOR_MASK)) >>
		  MVPP2_CLS2_ACT_DATA_TBL_COLOR_OFF);

	DBG_MSG("FROM_QOS_%s_TBL[%2.2d]:  ", sel ? "DSCP" : "PRI", id);
	if (type)
		DBG_MSG("%s	", sel ? "DSCP" : "PRIO");
	if (color)
		DBG_MSG("COLOR	");
	if (gemid)
		DBG_MSG("GEMID	");
	if (low_q)
		DBG_MSG("LOW_Q	");
	if (high_q)
		DBG_MSG("HIGH_Q	");
	DBG_MSG("\n");

	DBG_MSG("FROM_ACT_TBL:		");
	if (type == 0)
		DBG_MSG("%s	", sel ? "DSCP" : "PRI");
	if (gemid == 0)
		DBG_MSG("GEMID	");
	if (low_q == 0)
		DBG_MSG("LOW_Q	");
	if (high_q == 0)
		DBG_MSG("HIGH_Q	");
	if (color == 0)
		DBG_MSG("COLOR	");
	DBG_MSG("\n\n");

	/*------------------------------*/
	/*	actions 0x1B60		*/
	/*------------------------------*/

	DBG_MSG(
		"ACT_CMD:	COLOR	PRIO	DSCP	GEMID	LOW_Q	HIGH_Q	FWD	POLICER	FID	RSS\n");
	DBG_MSG("			");

	DBG_MSG(
		"%1.1d\t%1.1d\t%1.1d\t%1.1d\t%1.1d\t%1.1d\t%1.1d\t%1.1d\t%1.1d\t%1.1d\t",
		((c2->sram.regs.actions &
			MVPP2_CLS2_ACT_DATA_TBL_COLOR_MASK) >>
			MVPP2_CLS2_ACT_DATA_TBL_COLOR_OFF),
		((c2->sram.regs.actions &
			MVPP2_CLS2_ACT_PRI_MASK) >>
			MVPP2_CLS2_ACT_PRI_OFF),
		((c2->sram.regs.actions &
			MVPP2_CLS2_ACT_DSCP_MASK) >>
			MVPP2_CLS2_ACT_DSCP_OFF),
		((c2->sram.regs.actions &
			MVPP2_CLS2_ACT_DATA_TBL_GEM_ID_MASK) >>
			MVPP2_CLS2_ACT_DATA_TBL_GEM_ID_OFF),
		((c2->sram.regs.actions &
			MVPP2_CLS2_ACT_DATA_TBL_LOW_Q_MASK) >>
			MVPP2_CLS2_ACT_DATA_TBL_LOW_Q_OFF),
		((c2->sram.regs.actions &
			MVPP2_CLS2_ACT_DATA_TBL_HIGH_Q_MASK) >>
			MVPP2_CLS2_ACT_DATA_TBL_HIGH_Q_OFF),
		((c2->sram.regs.actions &
			MVPP2_CLS2_ACT_FRWD_MASK) >>
			MVPP2_CLS2_ACT_FRWD_OFF),
		((c2->sram.regs.actions &
			MVPP2_CLS2_ACT_PLCR_MASK) >>
			MVPP2_CLS2_ACT_PLCR_OFF),
		((c2->sram.regs.actions &
			MVPP2_CLS2_ACT_FLD_EN_MASK) >>
			MVPP2_CLS2_ACT_FLD_EN_OFF),
		((c2->sram.regs.actions &
			MVPP2_CLS2_ACT_RSS_MASK) >>
			MVPP2_CLS2_ACT_RSS_OFF));
	DBG_MSG("\n\n");

	/*------------------------------*/
	/*	qos_attr 0x1B64		*/
	/*------------------------------*/
	DBG_MSG(
		"ACT_ATTR:		PRIO	DSCP	GEMID	LOW_Q	HIGH_Q	QUEUE\n");
	DBG_MSG("		");
	/* modify priority */
	int32bit = ((c2->sram.regs.qos_attr &
		    MVPP2_CLS2_ACT_QOS_ATTR_PRI_MASK) >>
		    MVPP2_CLS2_ACT_QOS_ATTR_PRI_OFF);
	DBG_MSG("	%1.1d\t", int32bit);

	/* modify dscp */
	int32bit = ((c2->sram.regs.qos_attr &
		    MVPP2_CLS2_ACT_QOS_ATTR_DSCP_MASK) >>
		    MVPP2_CLS2_ACT_QOS_ATTR_DSCP_OFF);
	DBG_MSG("%2.2d\t", int32bit);

	/* modify gemportid */
	int32bit = ((c2->sram.regs.qos_attr &
		    MVPP2_CLS2_ACT_QOS_ATTR_GEM_MASK) >>
		    MVPP2_CLS2_ACT_QOS_ATTR_GEM_OFF);
	DBG_MSG("0x%4.4x\t", int32bit);

	/* modify low Q */
	int32bit = ((c2->sram.regs.qos_attr &
		    MVPP2_CLS2_ACT_QOS_ATTR_QL_MASK) >>
		    MVPP2_CLS2_ACT_QOS_ATTR_QL_OFF);
	DBG_MSG("%1.1d\t", int32bit);

	/* modify high Q */
	int32bit = ((c2->sram.regs.qos_attr &
		    MVPP2_CLS2_ACT_QOS_ATTR_QH_MASK) >>
		    MVPP2_CLS2_ACT_QOS_ATTR_QH_OFF);
	DBG_MSG("0x%2.2x\t", int32bit);

	/*modify queue*/
	int32bit = ((c2->sram.regs.qos_attr &
		    (MVPP2_CLS2_ACT_QOS_ATTR_QL_MASK |
		    MVPP2_CLS2_ACT_QOS_ATTR_QH_MASK)));
	int32bit >>= MVPP2_CLS2_ACT_QOS_ATTR_QL_OFF;

	DBG_MSG("0x%2.2x\t", int32bit);
	DBG_MSG("\n\n");

	/*------------------------------*/
	/*	hwf_attr 0x1B68		*/
	/*------------------------------*/
	DBG_MSG("HWF_ATTR:		IPTR	DPTR	CHKSM   MTU_IDX\n");
	DBG_MSG("			");

	/* HWF modification instraction pointer */
	int32bit = ((c2->sram.regs.hwf_attr &
		    MVPP2_CLS2_ACT_HWF_ATTR_IPTR_MASK) >>
		    MVPP2_CLS2_ACT_HWF_ATTR_IPTR_OFF);
	DBG_MSG("0x%1.1x\t", int32bit);

	/* HWF modification data pointer */
	int32bit = ((c2->sram.regs.hwf_attr &
		    MVPP2_CLS2_ACT_HWF_ATTR_DPTR_MASK) >>
		    MVPP2_CLS2_ACT_HWF_ATTR_DPTR_OFF);
	DBG_MSG("0x%4.4x\t", int32bit);

	/* HWF modification instraction pointer */
	int32bit = ((c2->sram.regs.hwf_attr &
		    MVPP2_CLS2_ACT_HWF_ATTR_L4CHK_MASK) >>
		    MVPP2_CLS2_ACT_HWF_ATTR_L4CHK_OFF);
	DBG_MSG("%s\t", int32bit ? "ENABLE " : "DISABLE");

	/* mtu index */
	int32bit = ((c2->sram.regs.hwf_attr &
		    MVPP2_CLS2_ACT_HWF_ATTR_MTUIDX_MASK) >>
		    MVPP2_CLS2_ACT_HWF_ATTR_MTUIDX_OFF);
	DBG_MSG("0x%1.1x\t", int32bit);
	DBG_MSG("\n\n");

	/*------------------------------*/
	/*	CLSC2_ATTR2 0x1B6C	*/
	/*------------------------------*/
	DBG_MSG("RSS_ATTR:		RSS_EN\n");
	DBG_MSG("			%d\n",
		((c2->sram.regs.rss_attr &
			MVPP2_CLS2_ACT_DUP_ATTR_RSSEN_MASK) >>
			MVPP2_CLS2_ACT_DUP_ATTR_RSSEN_OFF));
	DBG_MSG("\n");

	/*------------------------------*/
	/*	seq_attr 0x1B70		*/
	/*------------------------------*/
	/*PPv2.1 new feature MAS 3.14*/
	DBG_MSG("SEQ_ATTR:		ID	MISS\n");
	DBG_MSG("			0x%2.2x    0x%2.2x\n",
		((c2->sram.regs.seq_attr &
			MVPP21_CLS2_ACT_SEQ_ATTR_ID_MASK) >>
			MVPP21_CLS2_ACT_SEQ_ATTR_ID),
		((c2->sram.regs.seq_attr &
			MVPP21_CLS2_ACT_SEQ_ATTR_MISS_MASK) >>
			MVPP21_CLS2_ACT_SEQ_ATTR_MISS_OFF));
	DBG_MSG("\n\n");

	return MV_OK;
}

/*----------------------------------------------------------------------*/
int	mv_pp2x_cls_c2_hw_dump(struct mv_pp2x_hw *hw)
{
	int index;
	unsigned cnt;

	struct mv_pp2x_cls_c2_entry c2;

	memset(&c2, 0, sizeof(c2));

	for (index = 0; index < MVPP2_CLS_C2_TCAM_SIZE; index++) {
		mv_pp2x_cls_c2_hw_read(hw, index, &c2);
		if (c2.inv == 0) {
			mv_pp2x_cls_c2_sw_dump(&c2);
			mv_pp2x_cls_c2_hit_cntr_read(hw, index, &cnt);
			DBG_MSG("HITS: %d\n", cnt);
			DBG_MSG("-----------------------------------------\n");
		}
	}
	return MV_OK;
}
EXPORT_SYMBOL(mv_pp2x_cls_c2_hw_dump);

/*----------------------------------------------------------------------*/

