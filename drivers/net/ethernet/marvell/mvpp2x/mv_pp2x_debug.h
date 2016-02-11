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

#ifndef _MVPP2_DEBUG_H_
#define _MVPP2_DEBUG_H_

#include <linux/kernel.h>
#include <linux/skbuff.h>
#include <linux/platform_device.h>

#define MV_AMPLIFY_FACTOR_MTU				(3)
#define MV_BIT_NUM_OF_BYTE				(8)
#define MV_WRR_WEIGHT_UNIT				(256)

/* Macro for alignment up. For example, MV_ALIGN_UP(0x0330, 0x20) = 0x0340   */
#define MV_ALIGN_UP(number, align) (((number) & ((align) - 1)) ? \
				    (((number) + (align)) & ~((align) - 1)) : \
				    (number))

/* Macro for alignment down. For example, MV_ALIGN_UP(0x0330, 0x20) = 0x0320 */
#define MV_ALIGN_DOWN(number, align) ((number) & ~((align) - 1))
/* CPU architecture dependent 32, 16, 8 bit read/write IO addresses */
#define MV_MEMIO32_WRITE(addr, data)    \
	((*((unsigned int *)(addr))) = ((unsigned int)(data)))

#define MV_MEMIO32_READ(addr)           \
	((*((unsigned int *)(addr))))

#define MV_MEMIO16_WRITE(addr, data)    \
	((*((unsigned short *)(addr))) = ((unsigned short)(data)))

#define MV_MEMIO16_READ(addr)           \
	((*((unsigned short *)(addr))))

#define MV_MEMIO8_WRITE(addr, data)     \
	((*((unsigned char *)(addr))) = ((unsigned char)(data)))

#define MV_MEMIO8_READ(addr)            \
	((*((unsigned char *)(addr))))

/* This macro returns absolute value                                        */
#define MV_ABS(number)  (((int)(number) < 0) ? -(int)(number) : (int)(number))


void mv_pp2x_print_reg(struct mv_pp2x_hw *hw, unsigned int reg_addr,
			   char *reg_name);
void mv_pp2x_print_reg2(struct mv_pp2x_hw *hw, unsigned int reg_addr,
			     char *reg_name, unsigned int index);

void mv_pp2x_bm_pool_regs(struct mv_pp2x_hw *hw, int pool);
void mv_pp2x_bm_pool_drop_count(struct mv_pp2x_hw *hw, int pool);
void mv_pp2x_pool_status(struct mv_pp2x *priv, int log_pool_num);
void mv_pp2_pool_stats_print(struct mv_pp2x *priv, int log_pool_num);

void mvPp2RxDmaRegsPrint(struct mv_pp2x *priv, bool print_all,
			 int start, int stop);
void mvPp2RxqShow(struct mv_pp2x *priv, int port, int rxq, int mode);
void mvPp2PhysRxqRegs(struct mv_pp2x *pp2, int rxq);
void mvPp2PortRxqRegs(struct mv_pp2x *pp2, int port, int rxq);
void mv_pp22_isr_rx_group_regs(struct mv_pp2x *priv, int port, bool print_all);

void mvPp2V1RxqDbgCntrs(struct mv_pp2x *priv, int port, int rxq);
void mvPp2RxFifoRegs(struct mv_pp2x_hw *hw, int port);

void mv_pp2x_rx_desc_print(struct mv_pp2x *priv, struct mv_pp2x_rx_desc *desc);

void mv_pp2x_skb_dump(struct sk_buff *skb, int size, int access);
void mvPp2TxqShow(struct mv_pp2x *priv, int port, int txq, int mode);
void mvPp2AggrTxqShow(struct mv_pp2x *priv, int cpu, int mode);
void mvPp2PhysTxqRegs(struct mv_pp2x *priv, int txq);
void mvPp2PortTxqRegs(struct mv_pp2x *priv, int port, int txq);
void mvPp2AggrTxqRegs(struct mv_pp2x *priv, int cpu);
void mvPp2V1TxqDbgCntrs(struct mv_pp2x *priv, int port, int txq);
void mvPp2V1DropCntrs(struct mv_pp2x *priv, int port);
void mvPp2TxRegs(struct mv_pp2x *priv);
void mvPp2TxSchedRegs(struct mv_pp2x *priv, int port);
int mvPp2TxpRateSet(struct mv_pp2x *priv, int port, int rate);
int mvPp2TxpBurstSet(struct mv_pp2x *priv, int port, int burst);
int mvPp2TxqRateSet(struct mv_pp2x *priv, int port, int txq, int rate);
int mvPp2TxqBurstSet(struct mv_pp2x *priv, int port, int txq, int burst);
int mvPp2TxqFixPrioSet(struct mv_pp2x *priv, int port, int txq);
int mvPp2TxqWrrPrioSet(struct mv_pp2x *priv, int port, int txq, int weight);

int mv_pp2x_wrap_cos_mode_set(struct mv_pp2x_port *port,
			      enum mv_pp2x_cos_classifier cos_mode);
int mv_pp2x_wrap_cos_mode_get(struct mv_pp2x_port *port);
int mv_pp2x_wrap_cos_pri_map_set(struct mv_pp2x_port *port, int cos_pri_map);
int mv_pp2x_wrap_cos_pri_map_get(struct mv_pp2x_port *port);
int mv_pp2x_wrap_cos_dflt_value_set(struct mv_pp2x_port *port, int cos_value);
int mv_pp2x_wrap_cos_dflt_value_get(struct mv_pp2x_port *port);
int mv_pp22_wrap_rss_mode_set(struct mv_pp2x_port *port, int rss_mode);
int mv_pp22_wrap_rss_dflt_cpu_set(struct mv_pp2x_port *port, int default_cpu);
int mv_pp2x_port_bind_cpu_set(struct mv_pp2x_port *port, u8 bind_cpu);
int mv_pp2x_debug_param_set(u32 param);

void mv_pp2x_bm_queue_map_dump_all(struct mv_pp2x_hw *hw);

int mv_pp2x_cls_c2_qos_prio_set(struct mv_pp2x_cls_c2_qos_entry *qos, u8 pri);
int mv_pp2x_cls_c2_qos_dscp_set(struct mv_pp2x_cls_c2_qos_entry *qos, u8 dscp);
int mv_pp2x_cls_c2_qos_color_set(struct mv_pp2x_cls_c2_qos_entry *qos,
				 u8 color);
int mv_pp2x_cls_c2_queue_set(struct mv_pp2x_cls_c2_entry *c2, int cmd,
			     int queue, int from);
int mv_pp2x_cls_c2_mtu_set(struct mv_pp2x_cls_c2_entry *c2, int mtu_inx);

int mv_pp2x_prs_sw_dump(struct mv_pp2x_prs_entry *pe);
int mv_pp2x_prs_hw_dump(struct mv_pp2x_hw *hw);
int mv_pp2x_prs_hw_regs_dump(struct mv_pp2x_hw *hw);
int mv_pp2x_prs_hw_hits_dump(struct mv_pp2x_hw *hw);

int mv_pp2x_cls_c2_sw_dump(struct mv_pp2x_cls_c2_entry *c2);
int mv_pp2x_cls_c2_hw_dump(struct mv_pp2x_hw *hw);
int mv_pp2x_cls_c2_qos_dscp_hw_dump(struct mv_pp2x_hw *hw);
int mv_pp2x_cls_c2_qos_prio_hw_dump(struct mv_pp2x_hw *hw);



#endif /* _MVPP2_DEBUG_H_ */
