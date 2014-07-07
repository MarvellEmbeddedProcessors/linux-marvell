/*
 * Driver for Marvell PPv2 network controller for Armada 375 SoC.
 *
 * Copyright (C) 2014 Marvell
 *
 * Marcin Wojtas <mw@semihalf.com>
 *
 * This file is licensed under the terms of the GNU General Public
 * License version 2. This program is licensed "as is" without any
 * warranty of any kind, whether express or implied.
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
#include <net/ip.h>
#include <net/ipv6.h>
#include <uapi/linux/ppp_defs.h>
#include <linux/of.h>
#include <linux/of_irq.h>
#include <linux/of_mdio.h>
#include <linux/of_net.h>
#include <linux/of_address.h>
#include <linux/phy.h>
#include <linux/clk.h>

/* #define MVPP2_DEBUGS */
#ifdef MVPP2_DEBUGS
#define dprintk(a...) printk(a)
#else
#define dprintk(a...)
#endif

/* Packet Processor registers */

/* RX Fifo Registers */
#define MVPP2_RX_DATA_FIFO_SIZE_REG(port)	(0x00 + 4 * (port))
#define MVPP2_RX_ATTR_FIFO_SIZE_REG(port)	(0x20 + 4 * (port))
#define MVPP2_RX_MIN_PKT_SIZE_REG		0x60
#define MVPP2_RX_FIFO_INIT_REG			0x64

/* RX DMA Top Registers */
#define MVPP2_RX_CTRL_REG(port)			(0x140 + 4 * (port))
#define     MVPP2_RX_LOW_LATENCY_PKT_SIZE(s)	(((s) & 0xfff) << 16)
#define     MVPP2_RX_USE_PSEUDO_FOR_CSUM_MASK	BIT(31)
#define MVPP2_POOL_BUF_SIZE_REG(pool)		(0x180 + 4 * (pool))
#define     MVPP2_POOL_BUF_SIZE_OFFSET		5
#define MVPP2_RXQ_CONFIG_REG(rxq)		(0x800 + 4 * (rxq))
#define     MVPP2_SNOOP_PKT_SIZE_MASK		0x1ff
#define     MVPP2_SNOOP_BUF_HDR_MASK		BIT(9)
#define     MVPP2_RXQ_POOL_SHORT_OFFS		20
#define     MVPP2_RXQ_POOL_SHORT_MASK		0x700000
#define     MVPP2_RXQ_POOL_LONG_OFFS		24
#define     MVPP2_RXQ_POOL_LONG_MASK		0x7000000
#define     MVPP2_RXQ_PACKET_OFFSET_OFFS	28
#define     MVPP2_RXQ_PACKET_OFFSET_MASK	0x70000000
#define     MVPP2_RXQ_DISABLE_MASK		BIT(31)

/* Parser Registers */
#define MVPP2_PRS_INIT_LOOKUP_REG		0x1000
#define     MVPP2_PRS_PORT_LU_MAX		0xf
#define     MVPP2_PRS_PORT_LU_MASK(port)	(0xff << ((port) * 4))
#define     MVPP2_PRS_PORT_LU_VAL(port, val)	((val) << ((port) * 4))
#define MVPP2_PRS_INIT_OFFS_REG(port)		(0x1004 + ((port) & 4))
#define     MVPP2_PRS_INIT_OFF_MASK(port)	(0x3f << (((port) % 4) * 8))
#define     MVPP2_PRS_INIT_OFF_VAL(port, val)	((val) << (((port) % 4) * 8))
#define MVPP2_PRS_MAX_LOOP_REG(port)		(0x100c + ((port) & 4))
#define     MVPP2_PRS_MAX_LOOP_MASK(port)	(0xff << (((port) % 4) * 8))
#define     MVPP2_PRS_MAX_LOOP_VAL(port, val)	((val) << (((port) % 4) * 8))
#define MVPP2_PRS_TCAM_IDX_REG			0x1100
#define MVPP2_PRS_TCAM_DATA_REG(idx)		(0x1104 + (idx) * 4)
#define     MVPP2_PRS_TCAM_INV_MASK		BIT(31)
#define MVPP2_PRS_SRAM_IDX_REG			0x1200
#define MVPP2_PRS_SRAM_DATA_REG(idx)		(0x1204 + (idx) * 4)
#define MVPP2_PRS_TCAM_CTRL_REG			0x1230
#define     MVPP2_PRS_TCAM_EN_MASK		BIT(0)

/* Classifier Registers */
#define MVPP2_CLS_MODE_REG			0x1800
#define     MVPP2_CLS_MODE_ACTIVE_MASK		BIT(0)
#define MVPP2_CLS_PORT_WAY_REG			0x1810
#define     MVPP2_CLS_PORT_WAY_MASK(port)	(1 << (port))
#define MVPP2_CLS_LKP_INDEX_REG			0x1814
#define     MVPP2_CLS_LKP_INDEX_WAY_OFFS	6
#define MVPP2_CLS_LKP_TBL_REG			0x1818
#define     MVPP2_CLS_LKP_TBL_RXQ_MASK		0xff
#define     MVPP2_CLS_LKP_TBL_LOOKUP_EN_MASK	BIT(25)
#define MVPP2_CLS_FLOW_INDEX_REG		0x1820
#define MVPP2_CLS_FLOW_TBL0_REG			0x1824
#define MVPP2_CLS_FLOW_TBL1_REG			0x1828
#define MVPP2_CLS_FLOW_TBL2_REG			0x182c
#define MVPP2_CLS_OVERSIZE_RXQ_LOW_REG(port)	(0x1980 + ((port) * 4))
#define     MVPP2_CLS_OVERSIZE_RXQ_LOW_BITS	3
#define     MVPP2_CLS_OVERSIZE_RXQ_LOW_MASK	0x7
#define MVPP2_CLS_SWFWD_P2HQ_REG(port)		(0x19b0 + ((port) * 4))
#define MVPP2_CLS_SWFWD_PCTRL_REG		0x19d0
#define     MVPP2_CLS_SWFWD_PCTRL_MASK(port)	(1 << (port))


/* Descriptor Manager Top Registers */
#define MVPP2_RXQ_NUM_REG			0x2040
#define MVPP2_RXQ_DESC_ADDR_REG			0x2044
#define MVPP2_RXQ_DESC_SIZE_REG			0x2048
#define     MVPP2_RXQ_DESC_SIZE_MASK		0x3ff0
#define MVPP2_RXQ_STATUS_UPDATE_REG(rxq)	(0x3000 + 4 * (rxq))
#define     MVPP2_RXQ_NUM_PROCESSED_OFFSET	0
#define     MVPP2_RXQ_NUM_NEW_OFFSET		16
#define MVPP2_RXQ_STATUS_REG(rxq)		(0x3400 + 4 * (rxq))
#define     MVPP2_RXQ_OCCUPIED_MASK		0x3fff
#define     MVPP2_RXQ_NON_OCCUPIED_OFFSET	16
#define     MVPP2_RXQ_NON_OCCUPIED_MASK		0x3fff0000
#define MVPP2_RXQ_THRESH_REG			0x204c
#define     MVPP2_OCCUPIED_THRESH_OFFSET	0
#define     MVPP2_OCCUPIED_THRESH_MASK		0x3fff
#define MVPP2_RXQ_INDEX_REG			0x2050
#define MVPP2_TXQ_NUM_REG			0x2080
#define MVPP2_TXQ_DESC_ADDR_REG			0x2084
#define MVPP2_TXQ_DESC_SIZE_REG			0x2088
#define     MVPP2_TXQ_DESC_SIZE_MASK		0x3ff0
#define MVPP2_AGGR_TXQ_UPDATE_REG		0x2090
#define MVPP2_TXQ_THRESH_REG			0x2094
#define     MVPP2_TRANSMITTED_THRESH_OFFSET	16
#define     MVPP2_TRANSMITTED_THRESH_MASK	0x3fff0000
#define MVPP2_TXQ_INDEX_REG			0x2098
#define MVPP2_TXQ_PREF_BUF_REG			0x209c
#define     MVPP2_PREF_BUF_PTR(desc)		((desc) & 0xfff)
#define     MVPP2_PREF_BUF_SIZE_4		(BIT(12) | BIT(13))
#define     MVPP2_PREF_BUF_SIZE_16		(BIT(12) | BIT(14))
#define     MVPP2_PREF_BUF_THRESH(val)		((val) << 17)
#define     MVPP2_TXQ_DRAIN_EN_MASK		BIT(31)
#define MVPP2_TXQ_PENDING_REG			0x20a0
#define     MVPP2_TXQ_PENDING_MASK		0x3fff
#define MVPP2_TXQ_INT_STATUS_REG		0x20a4
#define MVPP2_TXQ_SENT_REG(txq)			(0x3c00 + 4 * (txq))
#define     MVPP2_TRANSMITTED_COUNT_OFFSET	16
#define     MVPP2_TRANSMITTED_COUNT_MASK	0x3fff0000
#define MVPP2_TXQ_RSVD_REQ_REG			0x20b0
#define     MVPP2_TXQ_RSVD_REQ_Q_OFFSET		16
#define MVPP2_TXQ_RSVD_RSLT_REG			0x20b4
#define     MVPP2_TXQ_RSVD_RSLT_MASK		0x3fff
#define MVPP2_TXQ_RSVD_CLR_REG			0x20b8
#define     MVPP2_TXQ_RSVD_CLR_OFFSET		16
#define MVPP2_AGGR_TXQ_DESC_ADDR_REG(cpu)	(0x2100 + 4 * (cpu))
#define MVPP2_AGGR_TXQ_DESC_SIZE_REG(cpu)	(0x2140 + 4 * (cpu))
#define     MVPP2_AGGR_TXQ_DESC_SIZE_MASK	0x3ff0
#define MVPP2_AGGR_TXQ_STATUS_REG(cpu)		(0x2180 + 4 * (cpu))
#define     MVPP2_AGGR_TXQ_PENDING_MASK		0x3fff
#define MVPP2_AGGR_TXQ_INDEX_REG(cpu)		(0x21c0 + 4 * (cpu))

/* MBUS bridge registers */
#define MVPP2_WIN_BASE(w)			(0x4000 + ((w) << 2))
#define MVPP2_WIN_SIZE(w)			(0x4020 + ((w) << 2))
#define MVPP2_WIN_REMAP(w)			(0x4040 + ((w) << 2))
#define MVPP2_BASE_ADDR_ENABLE			0x4060

/* Interrupt Cause and Mask registers */
#define MVPP2_ISR_RX_THRESHOLD_REG(rxq)		(0x5200 + 4 * (rxq))
#define MVPP2_ISR_RXQ_GROUP_REG(rxq)		(0x5400 + 4 * (rxq))
#define MVPP2_ISR_ENABLE_REG(port)		(0x5420 + 4 * (port))
#define     MVPP2_ISR_ENABLE_INTERRUPT(mask)	((mask) & 0xffff)
#define     MVPP2_ISR_DISABLE_INTERRUPT(mask)	(((mask) << 16) & 0xffff0000)
#define MVPP2_ISR_RX_TX_CAUSE_REG(port)		(0x5480 + 4 * (port))
#define     MVPP2_CAUSE_RXQ_OCCUP_DESC_ALL_MASK	0xffff
#define     MVPP2_CAUSE_TXQ_OCCUP_DESC_ALL_MASK	0xff0000
#define     MVPP2_CAUSE_RX_FIFO_OVERRUN_MASK	BIT(24)
#define     MVPP2_CAUSE_FCS_ERR_MASK		BIT(25)
#define     MVPP2_CAUSE_TX_FIFO_UNDERRUN_MASK	BIT(26)
#define     MVPP2_CAUSE_TX_EXCEPTION_SUM_MASK	BIT(29)
#define     MVPP2_CAUSE_RX_EXCEPTION_SUM_MASK	BIT(30)
#define     MVPP2_CAUSE_MISC_SUM_MASK		BIT(31)
#define MVPP2_ISR_RX_TX_MASK_REG(port)		(0x54a0 + 4 * (port))
#define MVPP2_ISR_PON_RX_TX_MASK_REG		0x54bc
#define     MVPP2_PON_CAUSE_RXQ_OCCUP_DESC_ALL_MASK	0xffff
#define     MVPP2_PON_CAUSE_TXP_OCCUP_DESC_ALL_MASK	0x3fc00000
#define     MVPP2_PON_CAUSE_MISC_SUM_MASK		BIT(31)
#define MVPP2_ISR_MISC_CAUSE_REG		0x55b0

/* Buffer Manager registers */
#define MVPP2_BM_POOL_BASE_REG(pool)		(0x6000 + ((pool) * 4))
#define     MVPP2_BM_POOL_BASE_ADDR_MASK	0xfffff80
#define MVPP2_BM_POOL_SIZE_REG(pool)		(0x6040 + ((pool) * 4))
#define     MVPP2_BM_POOL_SIZE_MASK		0xfff0
#define MVPP2_BM_POOL_READ_PTR_REG(pool)	(0x6080 + ((pool) * 4))
#define     MVPP2_BM_POOL_GET_READ_PTR_MASK	0xfff0
#define MVPP2_BM_POOL_PTRS_NUM_REG(pool)	(0x60c0 + ((pool) * 4))
#define     MVPP2_BM_POOL_PTRS_NUM_MASK		0xfff0
#define MVPP2_BM_BPPI_READ_PTR_REG(pool)	(0x6100 + ((pool) * 4))
#define MVPP2_BM_BPPI_PTRS_NUM_REG(pool)	(0x6140 + ((pool) * 4))
#define     MVPP2_BM_BPPI_PTR_NUM_MASK		0x7ff
#define     MVPP2_BM_BPPI_PREFETCH_FULL_MASK	BIT(16)
#define MVPP2_BM_POOL_CTRL_REG(pool)		(0x6200 + ((pool) * 4))
#define     MVPP2_BM_START_MASK			BIT(0)
#define     MVPP2_BM_STOP_MASK			BIT(1)
#define     MVPP2_BM_STATE_MASK			BIT(4)
#define     MVPP2_BM_LOW_THRESH_OFFS		8
#define     MVPP2_BM_LOW_THRESH_MASK		0x7f00
#define     MVPP2_BM_LOW_THRESH_VALUE(val)	((val) << \
						MVPP2_BM_LOW_THRESH_OFFS)
#define     MVPP2_BM_HIGH_THRESH_OFFS		16
#define     MVPP2_BM_HIGH_THRESH_MASK		0x7f0000
#define     MVPP2_BM_HIGH_THRESH_VALUE(val)	((val) << \
						MVPP2_BM_HIGH_THRESH_OFFS)
#define MVPP2_BM_INTR_CAUSE_REG(pool)		(0x6240 + ((pool) * 4))
#define     MVPP2_BM_RELEASED_DELAY_MASK	BIT(0)
#define     MVPP2_BM_ALLOC_FAILED_MASK		BIT(1)
#define     MVPP2_BM_BPPE_EMPTY_MASK		BIT(2)
#define     MVPP2_BM_BPPE_FULL_MASK		BIT(3)
#define     MVPP2_BM_AVAILABLE_BP_LOW_MASK	BIT(4)
#define MVPP2_BM_INTR_MASK_REG(pool)		(0x6280 + ((pool) * 4))
#define MVPP2_BM_PHY_ALLOC_REG(pool)		(0x6400 + ((pool) * 4))
#define     MVPP2_BM_PHY_ALLOC_GRNTD_MASK	BIT(0)
#define MVPP2_BM_VIRT_ALLOC_REG			0x6440
#define MVPP2_BM_PHY_RLS_REG(pool)		(0x6480 + ((pool) * 4))
#define     MVPP2_BM_PHY_RLS_MC_BUFF_MASK	BIT(0)
#define     MVPP2_BM_PHY_RLS_PRIO_EN_MASK	BIT(1)
#define     MVPP2_BM_PHY_RLS_GRNTD_MASK		BIT(2)
#define MVPP2_BM_VIRT_RLS_REG			0x64c0
#define MVPP2_BM_MC_RLS_REG			0x64c4
#define     MVPP2_BM_MC_ID_MASK			0xfff
#define     MVPP2_BM_FORCE_RELEASE_MASK		BIT(12)

/* TX Scheduler registers */
#define MVPP2_TXP_SCHED_PORT_INDEX_REG		0x8000
#define MVPP2_TXP_SCHED_Q_CMD_REG		0x8004
#define     MVPP2_TXP_SCHED_ENQ_MASK		0xff
#define     MVPP2_TXP_SCHED_DISQ_OFFSET		8
#define MVPP2_TXP_SCHED_CMD_1_REG		0x8010
#define MVPP2_TXP_SCHED_PERIOD_REG		0x8018
#define MVPP2_TXP_SCHED_MTU_REG			0x801c
#define     MVPP2_TXP_MTU_MAX			0x7FFFF
#define MVPP2_TXP_SCHED_REFILL_REG		0x8020
#define     MVPP2_TXP_REFILL_TOKENS_ALL_MASK	0x7ffff
#define     MVPP2_TXP_REFILL_PERIOD_ALL_MASK	0x3ff00000
#define     MVPP2_TXP_REFILL_PERIOD_MASK(v)	((v) << 20)
#define MVPP2_TXP_SCHED_TOKEN_SIZE_REG		0x8024
#define     MVPP2_TXP_TOKEN_SIZE_MAX		0xffffffff
#define MVPP2_TXQ_SCHED_REFILL_REG(q)		(0x8040 + ((q) << 2))
#define     MVPP2_TXQ_REFILL_TOKENS_ALL_MASK	0x7ffff
#define     MVPP2_TXQ_REFILL_PERIOD_ALL_MASK	0x3ff00000
#define     MVPP2_TXQ_REFILL_PERIOD_MASK(v)	((v) << 20)
#define MVPP2_TXQ_SCHED_TOKEN_SIZE_REG(q)	(0x8060 + ((q) << 2))
#define     MVPP2_TXQ_TOKEN_SIZE_MAX		0x7fffffff
#define MVPP2_TXQ_SCHED_TOKEN_CNTR_REG(q)	(0x8080 + ((q) << 2))
#define     MVPP2_TXQ_TOKEN_CNTR_MAX		0xffffffff

/* TX general registers */
#define MVPP2_TX_SNOOP_REG			0x8800
#define MVPP2_TX_PORT_FLUSH_REG			0x8810
#define     MVPP2_TX_PORT_FLUSH_MASK(port)	(1 << (port))

/* LMS registers */
#define MVPP2_SRC_ADDR_MIDDLE			0x24
#define MVPP2_SRC_ADDR_HIGH			0x28
#define MVPP2_MIB_COUNTERS_BASE(port)		(0x1000 + ((port) >> 1) * \
						0x400 + (port) * 0x400)
#define     MVPP2_MIB_LATE_COLLISION		0x7c
#define MVPP2_ISR_SUM_MASK_REG			0x220c
#define MVPP2_MNG_EXTENDED_GLOBAL_CTRL_REG	0x305c
#define MVPP2_EXT_GLOBAL_CTRL_DEFAULT		0x27

/* Per-port registers */
#define MVPP2_GMAC_CTRL_0_REG			0x0
#define      MVPP2_GMAC_PORT_EN_MASK		BIT(0)
#define      MVPP2_GMAC_MAX_RX_SIZE_OFFS	2
#define      MVPP2_GMAC_MAX_RX_SIZE_MASK	0x7ffc
#define      MVPP2_GMAC_MIB_CNTR_EN_MASK	BIT(15)
#define MVPP2_GMAC_CTRL_1_REG			0x4
#define      MVPP2_GMAC_PERIODIC_XON_EN_MASK	BIT(0)
#define      MVPP2_GMAC_GMII_LB_EN_MASK		BIT(5)
#define      MVPP2_GMAC_PCS_LB_EN_BIT		6
#define      MVPP2_GMAC_PCS_LB_EN_MASK		BIT(6)
#define      MVPP2_GMAC_SA_LOW_OFFS		7
#define MVPP2_GMAC_CTRL_2_REG			0x8
#define      MVPP2_GMAC_INBAND_AN_MASK		BIT(0)
#define      MVPP2_GMAC_PCS_ENABLE_MASK		BIT(3)
#define      MVPP2_GMAC_PORT_RGMII_MASK		BIT(4)
#define      MVPP2_GMAC_PORT_RESET_MASK		BIT(6)
#define MVPP2_GMAC_AUTONEG_CONFIG		0xc
#define      MVPP2_GMAC_FORCE_LINK_DOWN		BIT(0)
#define      MVPP2_GMAC_FORCE_LINK_PASS		BIT(1)
#define      MVPP2_GMAC_CONFIG_MII_SPEED	BIT(5)
#define      MVPP2_GMAC_CONFIG_GMII_SPEED	BIT(6)
#define      MVPP2_GMAC_AN_SPEED_EN		BIT(7)
#define      MVPP2_GMAC_CONFIG_FULL_DUPLEX	BIT(12)
#define      MVPP2_GMAC_AN_DUPLEX_EN		BIT(13)
#define MVPP2_GMAC_PORT_FIFO_CFG_1_REG		0x1c
#define      MVPP2_GMAC_TX_FIFO_MIN_TH_OFFS	6
#define      MVPP2_GMAC_TX_FIFO_MIN_TH_ALL_MASK	0x1fc0
#define      MVPP2_GMAC_TX_FIFO_MIN_TH_MASK(v)	(((v) << 6) & \
					MVPP2_GMAC_TX_FIFO_MIN_TH_ALL_MASK)

#define MVPP2_CAUSE_TXQ_SENT_DESC_ALL_MASK	0xff

/* Descriptor ring Macros */
#define MVPP2_QUEUE_NEXT_DESC(q, index) \
	(((index) < (q)->last_desc) ? ((index) + 1) : 0)

/* Various constants */

/* Coalescing */
#define MVPP2_TXDONE_COAL_PKTS_THRESH	15
#define MVPP2_RX_COAL_PKTS		32
#define MVPP2_RX_COAL_USEC		100

/* The two bytes Marvell header. Either contains a special value used
 * by Marvell switches when a specific hardware mode is enabled (not
 * supported by this driver) or is filled automatically by zeroes on
 * the RX side. Those two bytes being at the front of the Ethernet
 * header, they allow to have the IP header aligned on a 4 bytes
 * boundary automatically: the hardware skips those two bytes on its
 * own.
 */
#define MVPP2_MH_SIZE			2
#define MVPP2_ETH_TYPE_LEN		2
#define MVPP2_PPPOE_HDR_SIZE		8
#define MVPP2_VLAN_TAG_LEN		4

/* Lbtd 802.3 type */
#define MVPP2_IP_LBDT_TYPE		0xfffa

#define MVPP2_CPU_D_CACHE_LINE_SIZE	32
#define MVPP2_TX_CSUM_MAX_SIZE		9800

/* Timeout constants */
#define MVPP2_TX_DISABLE_TIMEOUT_MSEC	1000
#define MVPP2_RX_DISABLE_TIMEOUT_MSEC	1000
#define MVPP2_TX_FIFO_EMPTY_TIMEOUT	10000
#define MVPP2_PORT_DISABLE_WAIT_TCLOCKS	5000
#define MVPP2_TX_PENDING_TIMEOUT_MSEC	1000

#define MVPP2_TX_MTU_MAX		0x7ffff

/* Rx port number of PON port */
#define MVPP2_PON_PORT_ID		7

/* Maximum number of T-CONTs of PON port */
#define MVPP2_MAX_TCONT			16

/* Maximum number of supported ports */
#define MVPP2_MAX_PORTS			4

/* Maximum number of TXQs used by single port */
#define MVPP2_MAX_TXQ			8

/* Maximum number of TXQs used by single port */
#define MVPP2_MAX_RXQ			8

/* Total number of RXQs available to all ports */
#define MVPP2_RXQ_TOTAL_NUM		(MVPP2_MAX_PORTS * MVPP2_MAX_RXQ)

/* Max number of Rx descriptors */
#define MVPP2_MAX_RXD			128

/* Max number of Tx descriptors */
#define MVPP2_MAX_TXD			1024

/* Amount of Tx descriptors that can be reserved at once by CPU */
#define MVPP2_CPU_DESC_CHUNK		64

/* Max number of Tx descriptors in each aggregated queue */
#define MVPP2_AGGR_TXQ_SIZE		256

/* Descriptor aligned size */
#define MVPP2_DESC_ALIGNED_SIZE		32

/* Descriptor alignment mask */
#define MVPP2_TX_DESC_ALIGN		(MVPP2_DESC_ALIGNED_SIZE - 1)

/* RX FIFO constants */
#define MVPP2_RX_FIFO_PORT_DATA_SIZE	0x2000
#define MVPP2_RX_FIFO_PORT_ATTR_SIZE	0x80
#define MVPP2_RX_FIFO_PORT_MIN_PKT	0x80

/* RX buffer constants */
#define MVPP2_SKB_SHINFO_SIZE \
	SKB_DATA_ALIGN(sizeof(struct skb_shared_info))

#define MVPP2_RX_PKT_SIZE(mtu) \
	ALIGN((mtu) + MVPP2_MH_SIZE + MVPP2_VLAN_TAG_LEN + \
	      ETH_HLEN + ETH_FCS_LEN, MVPP2_CPU_D_CACHE_LINE_SIZE)

#define MVPP2_RX_BUF_SIZE(pkt_size)	((pkt_size) + NET_SKB_PAD)
#define MVPP2_RX_TOTAL_SIZE(buf_size)	((buf_size) + MVPP2_SKB_SHINFO_SIZE)
#define MVPP2_RX_MAX_PKT_SIZE(total_size) \
	((total_size) - NET_SKB_PAD - MVPP2_SKB_SHINFO_SIZE)

#define MVPP2_BIT_TO_BYTE(bit)		((bit) / 8)

/* IPv6 max L3 address size */
#define MVPP2_MAX_L3_ADDR_SIZE		16

/* Port flags */
#define MVPP2_F_LOOPBACK		BIT(0)

/* Marvell tag types */
enum mvpp2_tag_type {
	MVPP2_TAG_TYPE_NONE = 0,
	MVPP2_TAG_TYPE_MH   = 1,
	MVPP2_TAG_TYPE_DSA  = 2,
	MVPP2_TAG_TYPE_EDSA = 3,
	MVPP2_TAG_TYPE_VLAN = 4,
	MVPP2_TAG_TYPE_LAST = 5
};

/* Parser constants */
#define MVPP2_PRS_TCAM_SRAM_SIZE	256
#define MVPP2_PRS_TCAM_WORDS		6
#define MVPP2_PRS_SRAM_WORDS		4
#define MVPP2_PRS_FLOW_ID_SIZE		64
#define MVPP2_PRS_FLOW_ID_MASK		0x3f
#define MVPP2_PRS_TCAM_ENTRY_INVALID	1
#define MVPP2_PRS_TCAM_DSA_TAGGED_BIT	BIT(5)
#define MVPP2_PRS_IPV4_HEAD		0x40
#define MVPP2_PRS_IPV4_HEAD_MASK	0xf0
#define MVPP2_PRS_IPV4_MC		0xe0
#define MVPP2_PRS_IPV4_MC_MASK		0xf0
#define MVPP2_PRS_IPV4_BC_MASK		0xff
#define MVPP2_PRS_IPV4_IHL		0x5
#define MVPP2_PRS_IPV4_IHL_MASK		0xf
#define MVPP2_PRS_IPV6_MC		0xff
#define MVPP2_PRS_IPV6_MC_MASK		0xff
#define MVPP2_PRS_IPV6_HOP_MASK		0xff
#define MVPP2_PRS_TCAM_PROTO_MASK	0xff
#define MVPP2_PRS_TCAM_PROTO_MASK_L	0x3f
#define MVPP2_PRS_DBL_VLANS_MAX		100
/* Tcam structure:
 * - lookup ID - 4 bits
 * - port ID - 1 byte
 * - additional information - 1 byte
 * - header data - 8 bytes
 * The fields are represented by MVPP2_PRS_TCAM_DATA_REG(5)->(0).
 */
#define MVPP2_PRS_AI_BITS			8
#define MVPP2_PRS_PORT_MASK			0xff
#define MVPP2_PRS_LU_MASK			0xf
#define MVPP2_PRS_TCAM_DATA_BYTE(offs)		\
				    (((offs) - ((offs) % 2)) * 2 + ((offs) % 2))
#define MVPP2_PRS_TCAM_DATA_BYTE_EN(offs)	\
					      (((offs) * 2) - ((offs) % 2)  + 2)
#define MVPP2_PRS_TCAM_AI_BYTE			16
#define MVPP2_PRS_TCAM_PORT_BYTE		17
#define MVPP2_PRS_TCAM_LU_BYTE			20
#define MVPP2_PRS_TCAM_EN_OFFS(offs)		((offs) + 2)
#define MVPP2_PRS_TCAM_INV_WORD			5
/* Tcam entries ID */
#define MVPP2_PE_DROP_ALL		0
#define MVPP2_PE_FIRST_FREE_TID		1
#define MVPP2_PE_LAST_FREE_TID		(MVPP2_PRS_TCAM_SRAM_SIZE - 31)
#define MVPP2_PE_IP6_EXT_PROTO_UN	(MVPP2_PRS_TCAM_SRAM_SIZE - 30)
#define MVPP2_PE_MAC_MC_IP6		(MVPP2_PRS_TCAM_SRAM_SIZE - 29)
#define MVPP2_PE_IP6_ADDR_UN		(MVPP2_PRS_TCAM_SRAM_SIZE - 28)
#define MVPP2_PE_IP4_ADDR_UN		(MVPP2_PRS_TCAM_SRAM_SIZE - 27)
#define MVPP2_PE_LAST_DEFAULT_FLOW	(MVPP2_PRS_TCAM_SRAM_SIZE - 26)
#define MVPP2_PE_FIRST_DEFAULT_FLOW	(MVPP2_PRS_TCAM_SRAM_SIZE - 19)
#define MVPP2_PE_EDSA_TAGGED		(MVPP2_PRS_TCAM_SRAM_SIZE - 18)
#define MVPP2_PE_EDSA_UNTAGGED		(MVPP2_PRS_TCAM_SRAM_SIZE - 17)
#define MVPP2_PE_DSA_TAGGED		(MVPP2_PRS_TCAM_SRAM_SIZE - 16)
#define MVPP2_PE_DSA_UNTAGGED		(MVPP2_PRS_TCAM_SRAM_SIZE - 15)
#define MVPP2_PE_ETYPE_EDSA_TAGGED	(MVPP2_PRS_TCAM_SRAM_SIZE - 14)
#define MVPP2_PE_ETYPE_EDSA_UNTAGGED	(MVPP2_PRS_TCAM_SRAM_SIZE - 13)
#define MVPP2_PE_ETYPE_DSA_TAGGED	(MVPP2_PRS_TCAM_SRAM_SIZE - 12)
#define MVPP2_PE_ETYPE_DSA_UNTAGGED	(MVPP2_PRS_TCAM_SRAM_SIZE - 11)
#define MVPP2_PE_MH_DEFAULT		(MVPP2_PRS_TCAM_SRAM_SIZE - 10)
#define MVPP2_PE_DSA_DEFAULT		(MVPP2_PRS_TCAM_SRAM_SIZE - 9)
#define MVPP2_PE_IP6_PROTO_UN		(MVPP2_PRS_TCAM_SRAM_SIZE - 8)
#define MVPP2_PE_IP4_PROTO_UN		(MVPP2_PRS_TCAM_SRAM_SIZE - 7)
#define MVPP2_PE_ETH_TYPE_UN		(MVPP2_PRS_TCAM_SRAM_SIZE - 6)
#define MVPP2_PE_VLAN_DBL		(MVPP2_PRS_TCAM_SRAM_SIZE - 5)
#define MVPP2_PE_VLAN_NONE		(MVPP2_PRS_TCAM_SRAM_SIZE - 4)
#define MVPP2_PE_MAC_MC_ALL		(MVPP2_PRS_TCAM_SRAM_SIZE - 3)
#define MVPP2_PE_MAC_PROMISCUOUS	(MVPP2_PRS_TCAM_SRAM_SIZE - 2)
#define MVPP2_PE_MAC_NON_PROMISCUOUS	(MVPP2_PRS_TCAM_SRAM_SIZE - 1)
/* Sram structure
 * The fields are represented by MVPP2_PRS_TCAM_DATA_REG(3)->(0).
 */
#define MVPP2_PRS_SRAM_RI_OFFS			0
#define MVPP2_PRS_SRAM_RI_WORD			0
#define MVPP2_PRS_SRAM_RI_CTRL_OFFS		32
#define MVPP2_PRS_SRAM_RI_CTRL_WORD		1
#define MVPP2_PRS_SRAM_RI_CTRL_BITS		32
#define MVPP2_PRS_SRAM_SHIFT_OFFS		64
#define MVPP2_PRS_SRAM_SHIFT_SIGN_BIT		72
#define MVPP2_PRS_SRAM_UDF_OFFS			73
#define MVPP2_PRS_SRAM_UDF_BITS			8
#define MVPP2_PRS_SRAM_UDF_MASK			0xff
#define MVPP2_PRS_SRAM_UDF_SIGN_BIT		81
#define MVPP2_PRS_SRAM_UDF_TYPE_OFFS		82
#define MVPP2_PRS_SRAM_UDF_TYPE_MASK		0x7
#define MVPP2_PRS_SRAM_UDF_TYPE_L3		1
#define MVPP2_PRS_SRAM_UDF_TYPE_L4		4
#define MVPP2_PRS_SRAM_OP_SEL_SHIFT_OFFS	85
#define MVPP2_PRS_SRAM_OP_SEL_SHIFT_MASK	0x3
#define MVPP2_PRS_SRAM_OP_SEL_SHIFT_ADD		1
#define MVPP2_PRS_SRAM_OP_SEL_SHIFT_IP4_ADD	2
#define MVPP2_PRS_SRAM_OP_SEL_SHIFT_IP6_ADD	3
#define MVPP2_PRS_SRAM_OP_SEL_UDF_OFFS		87
#define MVPP2_PRS_SRAM_OP_SEL_UDF_BITS		2
#define MVPP2_PRS_SRAM_OP_SEL_UDF_MASK		0x3
#define MVPP2_PRS_SRAM_OP_SEL_UDF_ADD		0
#define MVPP2_PRS_SRAM_OP_SEL_UDF_IP4_ADD	2
#define MVPP2_PRS_SRAM_OP_SEL_UDF_IP6_ADD	3
#define MVPP2_PRS_SRAM_OP_SEL_BASE_OFFS		89
#define MVPP2_PRS_SRAM_AI_OFFS			90
#define MVPP2_PRS_SRAM_AI_CTRL_OFFS		98
#define MVPP2_PRS_SRAM_AI_CTRL_BITS		8
#define MVPP2_PRS_SRAM_AI_MASK			0xff
#define MVPP2_PRS_SRAM_NEXT_LU_OFFS		106
#define MVPP2_PRS_SRAM_NEXT_LU_MASK		0xf
#define MVPP2_PRS_SRAM_LU_DONE_BIT		110
#define MVPP2_PRS_SRAM_LU_GEN_BIT		111
/* Sram result info bits assignment */
#define MVPP2_PRS_RI_MAC_ME_MASK		0x1
#define MVPP2_PRS_RI_DSA_MASK			0x2
#define MVPP2_PRS_RI_VLAN_MASK			0xc
#define MVPP2_PRS_RI_VLAN_NONE			~(BIT(2) | BIT(3))
#define MVPP2_PRS_RI_VLAN_SINGLE		BIT(2)
#define MVPP2_PRS_RI_VLAN_DOUBLE		BIT(3)
#define MVPP2_PRS_RI_VLAN_TRIPLE		(BIT(2) | BIT(3))
#define MVPP2_PRS_RI_CPU_CODE_MASK		0x70
#define MVPP2_PRS_RI_CPU_CODE_RX_SPEC		BIT(4)
#define MVPP2_PRS_RI_L2_CAST_MASK		0x600
#define MVPP2_PRS_RI_L2_UCAST			~(BIT(9) | BIT(10))
#define MVPP2_PRS_RI_L2_MCAST			BIT(9)
#define MVPP2_PRS_RI_L2_BCAST			BIT(10)
#define MVPP2_PRS_RI_PPPOE_MASK			0x800
#define MVPP2_PRS_RI_L3_PROTO_MASK		0x7000
#define MVPP2_PRS_RI_L3_UN			~(BIT(12) | BIT(13) | BIT(14))
#define MVPP2_PRS_RI_L3_IP4			BIT(12)
#define MVPP2_PRS_RI_L3_IP4_OPT			BIT(13)
#define MVPP2_PRS_RI_L3_IP4_OTHER		(BIT(12) | BIT(13))
#define MVPP2_PRS_RI_L3_IP6			BIT(14)
#define MVPP2_PRS_RI_L3_IP6_EXT			(BIT(12) | BIT(14))
#define MVPP2_PRS_RI_L3_ARP			(BIT(13) | BIT(14))
#define MVPP2_PRS_RI_L3_ADDR_MASK		0x18000
#define MVPP2_PRS_RI_L3_UCAST			~(BIT(15) | BIT(16))
#define MVPP2_PRS_RI_L3_MCAST			BIT(15)
#define MVPP2_PRS_RI_L3_BCAST			(BIT(15) | BIT(16))
#define MVPP2_PRS_RI_IP_FRAG_MASK		0x20000
#define MVPP2_PRS_RI_UDF3_MASK			0x300000
#define MVPP2_PRS_RI_UDF3_RX_SPECIAL		BIT(21)
#define MVPP2_PRS_RI_L4_PROTO_MASK		0x1c00000
#define MVPP2_PRS_RI_L4_TCP			BIT(22)
#define MVPP2_PRS_RI_L4_UDP			BIT(23)
#define MVPP2_PRS_RI_L4_OTHER			(BIT(22) | BIT(23))
#define MVPP2_PRS_RI_UDF7_MASK			0x60000000
#define MVPP2_PRS_RI_UDF7_IP6_LITE		BIT(29)
#define MVPP2_PRS_RI_DROP_MASK			0x80000000
/* Sram additional info bits assignment */
#define MVPP2_PRS_IPV4_DIP_AI_BIT		BIT(0)
#define MVPP2_PRS_IPV6_NO_EXT_AI_BIT		BIT(0)
#define MVPP2_PRS_IPV6_EXT_AI_BIT		BIT(1)
#define MVPP2_PRS_IPV6_EXT_AH_AI_BIT		BIT(2)
#define MVPP2_PRS_IPV6_EXT_AH_LEN_AI_BIT	BIT(3)
#define MVPP2_PRS_IPV6_EXT_AH_L4_AI_BIT		BIT(4)
#define MVPP2_PRS_SINGLE_VLAN_AI		0
#define MVPP2_PRS_DBL_VLAN_AI_BIT		BIT(7)

/* DSA/EDSA type */
#define MVPP2_PRS_TAGGED		true
#define MVPP2_PRS_UNTAGGED		false
#define MVPP2_PRS_EDSA			true
#define MVPP2_PRS_DSA			false

/* MAC entries, shadow udf */
enum mvpp2_prs_udf {
	MVPP2_PRS_UDF_MAC_DEF,
	MVPP2_PRS_UDF_MAC_RANGE,
	MVPP2_PRS_UDF_L2_DEF,
	MVPP2_PRS_UDF_L2_DEF_COPY,
	MVPP2_PRS_UDF_L2_USER,
};

/* Lookup ID */
enum mvpp2_prs_lookup {
	MVPP2_PRS_LU_MH,
	MVPP2_PRS_LU_MAC,
	MVPP2_PRS_LU_DSA,
	MVPP2_PRS_LU_VLAN,
	MVPP2_PRS_LU_L2,
	MVPP2_PRS_LU_PPPOE,
	MVPP2_PRS_LU_IP4,
	MVPP2_PRS_LU_IP6,
	MVPP2_PRS_LU_FLOWS,
	MVPP2_PRS_LU_LAST,
};

/* L3 cast enum */
enum mvpp2_prs_l3_cast {
	MVPP2_PRS_L3_UNI_CAST,
	MVPP2_PRS_L3_MULTI_CAST,
	MVPP2_PRS_L3_BROAD_CAST
};

/* Multicast MAC kinds */
enum mvpp2_prs_mac_mc {
	MVPP2_PRS_IP4_MAC_MC,
	MVPP2_PRS_IP6_MAC_MC,
	MVPP2_PRS_MAX_MAC_MC
};

/* Classifier constants */
#define MVPP2_CLS_FLOWS_TBL_SIZE	512
#define MVPP2_CLS_FLOWS_TBL_DATA_WORDS	3
#define MVPP2_CLS_LKP_TBL_SIZE		64

/* BM constants */
#define MVPP2_BM_POOLS_NUM		8
#define MVPP2_BM_LONG_BUF_NUM		1024
#define MVPP2_BM_SHORT_BUF_NUM		2048
#define MVPP2_BM_POOL_SIZE_MAX		(16*1024 - MVPP2_BM_POOL_PTR_ALIGN/4)
#define MVPP2_BM_POOL_PTR_ALIGN		128
#define MVPP2_BM_SWF_LONG_POOL(port)	((port > 2) ? 2 : port)
#define MVPP2_BM_SWF_SHORT_POOL		3

/* BM cookie (32 bits) definition */
#define MVPP2_BM_COOKIE_POOL_OFFS	8
#define MVPP2_BM_COOKIE_CPU_OFFS	24

/* BM short pool packet size
 * These value assure that for SWF the total number
 * of bytes allocated for each buffer will be 512
 */
#define MVPP2_BM_SHORT_PKT_SIZE		MVPP2_RX_MAX_PKT_SIZE(512)

enum mvpp2_bm_type {
	MVPP2_BM_FREE,
	MVPP2_BM_SWF_LONG,
	MVPP2_BM_SWF_SHORT
};

/* Definitions */

/* Shared Packet Processor resources */
struct mvpp2 {
	/* Shared registers' base addresses */
	void __iomem *base;
	void __iomem *lms_base;

	/* Common clocks */
	struct clk *pp_clk;
	struct clk *gop_clk;

	/* List of pointers to port structures */
	struct mvpp2_port **port_list;

	/* Aggregated TXQs */
	struct mvpp2_tx_queue *aggr_txqs;

	/* BM pools */
	struct mvpp2_bm_pool *bm_pools;

	/* PRS shadow table */
	struct mvpp2_prs_shadow *prs_shadow;
	/* PRS auxiliary table for double vlan entries control */
	bool *prs_double_vlans;

	/* Tclk value */
	u32 tclk;

	spinlock_t lock;
};

struct mvpp2_pcpu_stats {
	struct	u64_stats_sync syncp;
	u64	rx_packets;
	u64	rx_bytes;
	u64	tx_packets;
	u64	tx_bytes;
};

struct mvpp2_port {
	u8 id;

	struct mvpp2 *pp2;

	/* Per-port registers' base address */
	void __iomem *base;

	struct mvpp2_rx_queue **rxqs;
	struct mvpp2_tx_queue **txqs;
	struct net_device *dev;

	int pkt_size;

	u32 pending_cause_rx;
	struct napi_struct napi;

	/* Flags */
	unsigned long flags;

	u8 mcast_count[256];
	u16 tx_ring_size;
	u16 rx_ring_size;
	struct mvpp2_pcpu_stats *stats;

	struct phy_device *phy_dev;
	phy_interface_t phy_interface;
	struct device_node *phy_node;
	unsigned int link;
	unsigned int duplex;
	unsigned int speed;

	struct mvpp2_bm_pool *pool_long;
	struct mvpp2_bm_pool *pool_short;

	/* Number of physical transmit ports - should be set to 1 for
	 * ethernet ports and 16 for PON port
	 */
	u8 txp_num;

	/* Index of first port's physical RXQ */
	u8 first_rxq;
};

/* The mvpp2_tx_desc and mvpp2_rx_desc structures describe the
 * layout of the transmit and reception DMA descriptors, and their
 * layout is therefore defined by the hardware design
 */

#define MVPP2_TXD_L3_OFF_SHIFT		0
#define MVPP2_TXD_IP_HLEN_SHIFT		8
#define MVPP2_TXD_L4_CSUM_FRAG		BIT(13)
#define MVPP2_TXD_L4_CSUM_NOT		BIT(14)
#define MVPP2_TXD_IP_CSUM_DISABLE	BIT(15)
#define MVPP2_TXD_PADDING_DISABLE	BIT(23)
#define MVPP2_TXD_L4_UDP		BIT(24)
#define MVPP2_TXD_L3_IP6		BIT(26)
#define MVPP2_TXD_L_DESC		BIT(28)
#define MVPP2_TXD_F_DESC		BIT(29)

#define MVPP2_RXD_ERR_SUMMARY		BIT(15)
#define MVPP2_RXD_ERR_CODE_MASK		(BIT(13) | BIT(14))
#define MVPP2_RXD_ERR_CRC		0x0
#define MVPP2_RXD_ERR_OVERRUN		BIT(13)
#define MVPP2_RXD_ERR_RESOURCE		(BIT(13) | BIT(14))
#define MVPP2_RXD_BM_POOL_ID_OFFS	16
#define MVPP2_RXD_BM_POOL_ID_MASK	(BIT(16) | BIT(17) | BIT(18))
#define MVPP2_RXD_HWF_SYNC		BIT(21)
#define MVPP2_RXD_L4_CSUM_OK		BIT(22)
#define MVPP2_RXD_IP4_HEADER_ERR	BIT(24)
#define MVPP2_RXD_L4_TCP		BIT(25)
#define MVPP2_RXD_L4_UDP		BIT(26)
#define MVPP2_RXD_L3_IP4		BIT(28)
#define MVPP2_RXD_L3_IP6		BIT(30)
#define MVPP2_RXD_BUF_HDR		BIT(31)

struct mvpp2_tx_desc {
	u32 command;		/* Options used by HW for packet transmitting.*/
	u8  packet_offset;	/* the offset from the buffer beginning	*/
	u8  phys_txq;		/* destination queue ID			*/
	u16 data_size;		/* data size of transmitted packet in bytes */
	u32 buf_phys_addr;	/* physical addr of transmitted buffer	*/
	u32 buf_cookie;		/* cookie for access to TX buffer in tx path */
	u32 reserved1[3];	/* hw_cmd (for future use, BM, PON, PNC) */
	u32 reserved2;		/* reserved (for future use)		*/
};

struct mvpp2_rx_desc {
	u32 status;		/* info about received packet		*/
	u16 reserved1;		/* parser_info (for future use, PnC)	*/
	u16 data_size;		/* size of received packet in bytes	*/
	u32 buf_phys_addr;	/* physical address of the buffer	*/
	u32 buf_cookie;		/* cookie for access to RX buffer in rx path */
	u16 reserved2;		/* gem_port_id (for future use, PON)	*/
	u16 reserved3;		/* csum_l4 (for future use, PnC)	*/
	u8  reserved4;		/* bm_qset (for future use, BM)		*/
	u8  reserved5;
	u16 reserved6;		/* classify_info (for future use, PnC)	*/
	u32 reserved7;		/* flow_id (for future use, PnC) */
	u32 reserved8;
};

/* Per-CPU Tx queue control */
struct mvpp2_txq_pcpu {
	int cpu;

	/* Number of Tx DMA descriptors in the descriptor ring */
	int size;

	/* Number of currently used Tx DMA descriptor in the
	 * descriptor ring
	 */
	int count;

	/* Number of Tx DMA descriptors reserved for each CPU */
	int reserved_num;

	/* Array of transmitted skb */
	struct sk_buff **tx_skb;

	/* Index of last TX DMA descriptor that was inserted */
	int txq_put_index;

	/* Index of the TX DMA descriptor to be cleaned up */
	int txq_get_index;
};

struct mvpp2_tx_queue {
	/* Physical number of this Tx queue */
	u8 id;

	/* Logical number of this Tx queue */
	u8 log_id;

	/* Number of port's egress port - 0 for ethernet, 0-15 for PON */
	u8 txp;

	/* Number of Tx DMA descriptors in the descriptor ring */
	int size;

	/* Number of Tx DMA descriptors to be used in software forwarding */
	int swf_size;

	/* Number of currently used Tx DMA descriptor in the
	 * descriptor ring
	 */
	int count;

	/* Per-CPU control of physical Tx queues */
	struct mvpp2_txq_pcpu __percpu *pcpu;

	/* Array of transmitted skb */
	struct sk_buff **tx_skb;

	u32 done_pkts_coal;

	/* Virtual address of thex Tx DMA descriptors array */
	struct mvpp2_tx_desc *descs;

	/* DMA address of the Tx DMA descriptors array */
	dma_addr_t descs_phys;

	/* Index of the last Tx DMA descriptor */
	int last_desc;

	/* Index of the next Tx DMA descriptor to process */
	int next_desc_to_proc;

	struct mvpp2 *pp2;
};

struct mvpp2_rx_queue {
	/* RX queue number, in the range 0-31 for physical RXQs */
	u8 id;

	/* Num of rx descriptors in the rx descriptor ring */
	int size;

	u32 pkts_coal;
	u32 time_coal;

	/* Virtual address of the RX DMA descriptors array */
	struct mvpp2_rx_desc *descs;

	/* DMA address of the RX DMA descriptors array */
	dma_addr_t descs_phys;

	/* Index of the last RX DMA descriptor */
	int last_desc;

	/* Index of the next RX DMA descriptor to process */
	int next_desc_to_proc;

	/* ID of port to which physical RXQ is mapped */
	int port;

	/* Port's logic RXQ number to which physical RXQ is mapped */
	int logic_rxq;
};

union mvpp2_prs_tcam_entry {
	u32 word[MVPP2_PRS_TCAM_WORDS];
	u8  byte[MVPP2_PRS_TCAM_WORDS * 4];
};

union mvpp2_prs_sram_entry {
	u32 word[MVPP2_PRS_SRAM_WORDS];
	u8  byte[MVPP2_PRS_SRAM_WORDS * 4];
};

struct mvpp2_prs_entry {
	u32 index;
	union mvpp2_prs_tcam_entry tcam;
	union mvpp2_prs_sram_entry sram;
};

struct mvpp2_prs_shadow {
	bool valid;
	bool finish;

	/* Lookup ID */
	int lu;

	/* User defined offset */
	int udf;

	/* Result info */
	u32 ri;
	u32 ri_mask;
};

struct mvpp2_cls_flow_entry {
	u32 index;
	u32 data[MVPP2_CLS_FLOWS_TBL_DATA_WORDS];
};

struct mvpp2_cls_lkp_entry {
	u32 lkpid;
	u32 way;
	u32 data;
};

struct mvpp2_bm_pool {
	/* Pool number in the range 0-7 */
	int id;
	enum mvpp2_bm_type type;

	/* Buffer Pointers Pool External (BPPE) size */
	int size;
	/* Number of buffers for this pool */
	int buf_num;
	/* Pool buffer size */
	int buf_size;
	/* Packet size */
	int pkt_size;

	/* BPPE virtual base address */
	u32 *virt_addr;
	/* BPPE physical base address */
	dma_addr_t phys_addr;

	/* Ports using BM pool */
	u32 port_map;

	/* Occupied buffers indicator */
	atomic_t in_use;
	int in_use_thresh;

	spinlock_t lock;
};

struct mvpp2_buff_hdr {
	u32 next_buff_phys_addr;
	u32 next_buff_virt_addr;
	u16 byte_count;
	u16 info;
	u8  reserved1;		/* bm_qset (for future use, BM)		*/
};

/* Buffer header info bits */
#define MVPP2_B_HDR_INFO_MC_ID_MASK	0xfff
#define MVPP2_B_HDR_INFO_MC_ID(info)	((info) & MVPP2_B_HDR_INFO_MC_ID_MASK)
#define MVPP2_B_HDR_INFO_LAST_OFFS	12
#define MVPP2_B_HDR_INFO_LAST_MASK	BIT(12)
#define MVPP2_B_HDR_INFO_IS_LAST(info) \
	   ((info & MVPP2_B_HDR_INFO_LAST_MASK) >> MVPP2_B_HDR_INFO_LAST_OFFS)

/* Static declaractions */

/* Number of RXQs used by single port */
static int rxq_number = MVPP2_MAX_RXQ;
/* Number of TXQs used by single port */
static int txq_number = MVPP2_MAX_TXQ;

#define MVPP2_DRIVER_NAME "mvpp2"
#define MVPP2_DRIVER_VERSION "1.0"

/* Utility/helper methods */

static void mvpp2_write(struct mvpp2 *pp2, u32 offset, u32 data)
{
	writel(data, pp2->base + offset);
}

static u32 mvpp2_read(struct mvpp2 *pp2, u32 offset)
{
	return readl(pp2->base + offset);
}

static void mvpp2_txq_inc_get(struct mvpp2_txq_pcpu *txq_pcpu)
{
	txq_pcpu->txq_get_index++;
	if (txq_pcpu->txq_get_index == txq_pcpu->size)
		txq_pcpu->txq_get_index = 0;
}

static void mvpp2_txq_inc_put(struct mvpp2_txq_pcpu *txq_pcpu,
			      struct sk_buff *skb)
{
	txq_pcpu->tx_skb[txq_pcpu->txq_put_index] = skb;
	txq_pcpu->txq_put_index++;
	if (txq_pcpu->txq_put_index == txq_pcpu->size)
		txq_pcpu->txq_put_index = 0;
}

static inline void mvpp2_mib_counters_clear(struct mvpp2_port *pp)
{
	int i;
	u32 dummy;

	/* Perform dummy reads from MIB counters */
	for (i = 0; i < MVPP2_MIB_LATE_COLLISION; i += 4)
		dummy = readl(pp->pp2->lms_base +
			      (MVPP2_MIB_COUNTERS_BASE(pp->id) + i));
}

/* Get number of physical egress port */
static int mvpp2_egress_port(struct mvpp2_port *pp, int txp)
{
	return MVPP2_MAX_TCONT + pp->id + txp;
}

/* Get number of physical TXQ */
static inline int mvpp2_txq_phys(int port, int txq)
{
	return (MVPP2_MAX_TCONT + port) * MVPP2_MAX_TXQ + txq;
}

/* Parser configuration routines */

/* Update parser tcam and sram hw entries */
static int mvpp2_prs_hw_write(struct mvpp2 *pp2, struct mvpp2_prs_entry *pe)
{
	int i;

	if (!pe)
		return -ENOMEM;

	if (pe->index > MVPP2_PRS_TCAM_SRAM_SIZE - 1)
		return -EINVAL;

	/* Clear entry invalidation bit */
	pe->tcam.word[MVPP2_PRS_TCAM_INV_WORD] &= ~MVPP2_PRS_TCAM_INV_MASK;

	/* Write tcam index - indirect access */
	mvpp2_write(pp2, MVPP2_PRS_TCAM_IDX_REG, pe->index);
	for (i = 0; i < MVPP2_PRS_TCAM_WORDS; i++)
		mvpp2_write(pp2, MVPP2_PRS_TCAM_DATA_REG(i), pe->tcam.word[i]);

	/* Write sram index - indirect access */
	mvpp2_write(pp2, MVPP2_PRS_SRAM_IDX_REG, pe->index);
	for (i = 0; i < MVPP2_PRS_SRAM_WORDS; i++)
		mvpp2_write(pp2, MVPP2_PRS_SRAM_DATA_REG(i), pe->sram.word[i]);

	dprintk("Write parser entry %d\n", pe->index);

	return 0;
}

/* Read tcam entry from hw */
static int mvpp2_prs_hw_read(struct mvpp2 *pp2, struct mvpp2_prs_entry *pe)
{
	int i;

	if (!pe)
		return -ENOMEM;

	if (pe->index > MVPP2_PRS_TCAM_SRAM_SIZE - 1)
		return -EINVAL;

	/* Write tcam index - indirect access */
	mvpp2_write(pp2, MVPP2_PRS_TCAM_IDX_REG, pe->index);

	pe->tcam.word[MVPP2_PRS_TCAM_INV_WORD] = mvpp2_read(pp2,
			      MVPP2_PRS_TCAM_DATA_REG(MVPP2_PRS_TCAM_INV_WORD));
	if (pe->tcam.word[MVPP2_PRS_TCAM_INV_WORD] & MVPP2_PRS_TCAM_INV_MASK)
		return MVPP2_PRS_TCAM_ENTRY_INVALID;

	for (i = 0; i < MVPP2_PRS_TCAM_WORDS; i++)
		pe->tcam.word[i] = mvpp2_read(pp2, MVPP2_PRS_TCAM_DATA_REG(i));

	/* Write sram index - indirect access */
	mvpp2_write(pp2, MVPP2_PRS_SRAM_IDX_REG, pe->index);

	for (i = 0; i < MVPP2_PRS_SRAM_WORDS; i++)
		pe->sram.word[i] = mvpp2_read(pp2, MVPP2_PRS_SRAM_DATA_REG(i));

	dprintk("Read parser entry %d\n", pe->index);

	return 0;
}

/* Invalidate tcam hw entry */
static void mvpp2_prs_hw_inv(struct mvpp2 *pp2, int index)
{
	/* Write index - indirect access */
	mvpp2_write(pp2, MVPP2_PRS_TCAM_IDX_REG, index);
	mvpp2_write(pp2, MVPP2_PRS_TCAM_DATA_REG(MVPP2_PRS_TCAM_INV_WORD),
		    MVPP2_PRS_TCAM_INV_MASK);
}

/* Enable shadow table entry and set its lookup ID */
static void mvpp2_prs_shadow_set(struct mvpp2 *pp2, int index, int lu)
{
	pp2->prs_shadow[index].valid = true;
	pp2->prs_shadow[index].lu = lu;
}

/* Update ri fields in shadow table entry */
static void mvpp2_prs_shadow_ri_set(struct mvpp2 *pp2, int index,
				    unsigned int ri, unsigned int ri_mask)
{
	pp2->prs_shadow[index].ri_mask = ri_mask;
	pp2->prs_shadow[index].ri = ri;
}

/* Update lookup field in tcam sw entry */
static void mvpp2_prs_tcam_lu_set(struct mvpp2_prs_entry *pe, unsigned int lu)
{
	pe->tcam.byte[MVPP2_PRS_TCAM_LU_BYTE] = lu;
	pe->tcam.byte[MVPP2_PRS_TCAM_EN_OFFS(MVPP2_PRS_TCAM_LU_BYTE)] =
							      MVPP2_PRS_LU_MASK;
}

/* Update mask for single port in tcam sw entry */
static void mvpp2_prs_tcam_port_set(struct mvpp2_prs_entry *pe,
				    unsigned int port, bool add)
{
	if (add)
		pe->tcam.byte[MVPP2_PRS_TCAM_EN_OFFS(MVPP2_PRS_TCAM_PORT_BYTE)]
								&= ~(1 << port);
	else
		pe->tcam.byte[MVPP2_PRS_TCAM_EN_OFFS(MVPP2_PRS_TCAM_PORT_BYTE)]
								 |= (1 << port);
}
/* Update port map in tcam sw entry */
static void mvpp2_prs_tcam_port_map_set(struct mvpp2_prs_entry *pe,
					unsigned int ports)
{
	pe->tcam.byte[MVPP2_PRS_TCAM_PORT_BYTE] = 0;
	pe->tcam.byte[MVPP2_PRS_TCAM_EN_OFFS(MVPP2_PRS_TCAM_PORT_BYTE)] &=
					  (unsigned char)(~MVPP2_PRS_PORT_MASK);
	pe->tcam.byte[MVPP2_PRS_TCAM_EN_OFFS(MVPP2_PRS_TCAM_PORT_BYTE)] |=
					       ((~ports) & MVPP2_PRS_PORT_MASK);
}

/* Obtain port map from tcam sw entry */
static void mvpp2_prs_tcam_port_map_get(struct mvpp2_prs_entry *pe,
					unsigned int *ports)
{
	*ports =
	    (~pe->tcam.byte[MVPP2_PRS_TCAM_EN_OFFS(MVPP2_PRS_TCAM_PORT_BYTE)]) &
	    MVPP2_PRS_PORT_MASK;
}

/* Set byte of data and its enable bits in tcam sw entry */
static void mvpp2_prs_tcam_data_byte_set(struct mvpp2_prs_entry *pe,
					 unsigned int offs, unsigned char byte,
					 unsigned char enable)
{
	pe->tcam.byte[MVPP2_PRS_TCAM_DATA_BYTE(offs)] = byte;
	pe->tcam.byte[MVPP2_PRS_TCAM_DATA_BYTE_EN(offs)] = enable;
}

/* Clear byte of data and its enable bits in tcam sw entry */
static void mvpp2_prs_tcam_data_byte_clear(struct mvpp2_prs_entry *pe,
					 unsigned int offs)
{
	pe->tcam.byte[MVPP2_PRS_TCAM_DATA_BYTE(offs)] = 0x0;
	pe->tcam.byte[MVPP2_PRS_TCAM_DATA_BYTE_EN(offs)] = 0x0;
}

/* Get byte of data and its enable bits from tcam sw entry */
static void mvpp2_prs_tcam_data_byte_get(struct mvpp2_prs_entry *pe,
					 unsigned int offs, unsigned char *byte,
					 unsigned char *enable)
{
	*byte = pe->tcam.byte[MVPP2_PRS_TCAM_DATA_BYTE(offs)];
	*enable = pe->tcam.byte[MVPP2_PRS_TCAM_DATA_BYTE_EN(offs)];
}

/* Compare tcam data bytes with a pattern */
static bool mvpp2_prs_tcam_data_cmp(struct mvpp2_prs_entry *pe,
					   unsigned int offs, unsigned int size,
					   unsigned char *bytes)
{
	unsigned char byte;
	unsigned char mask;
	int index;

	for (index = 0; index < size; index++) {
		mvpp2_prs_tcam_data_byte_get(pe, offs + index, &byte, &mask);

		if (byte != bytes[index])
			return -1;
	}
	return 0;
}

/* Update ai bits in tcam sw entry */
static void mvpp2_prs_tcam_ai_update(struct mvpp2_prs_entry *pe,
				     unsigned int bits, unsigned int enable)
{
	int i;

	for (i = 0; i < MVPP2_PRS_AI_BITS; i++)
		if (enable & BIT(i)) {
			if (bits & BIT(i))
				pe->tcam.byte[MVPP2_PRS_TCAM_AI_BYTE] |=
								       (1 << i);
			else
				pe->tcam.byte[MVPP2_PRS_TCAM_AI_BYTE] &=
								      ~(1 << i);
		}

	pe->tcam.byte[MVPP2_PRS_TCAM_EN_OFFS(MVPP2_PRS_TCAM_AI_BYTE)] |= enable;
}

/* Get ai bits from tcam sw entry */
static void mvpp2_prs_tcam_ai_get(struct mvpp2_prs_entry *pe, unsigned int *bits,
				unsigned int *enable)
{
	*bits = pe->tcam.byte[MVPP2_PRS_TCAM_AI_BYTE];
	*enable = pe->tcam.byte[MVPP2_PRS_TCAM_EN_OFFS(MVPP2_PRS_TCAM_AI_BYTE)];
}

/* Set ethertype in tcam sw entry */
static void mvpp2_prs_match_etype(struct mvpp2_prs_entry *pe, int offset,
				  unsigned short ethertype)
{
	mvpp2_prs_tcam_data_byte_set(pe, offset + 0, ethertype >> 8, 0xff);
	mvpp2_prs_tcam_data_byte_set(pe, offset + 1, ethertype & 0xff, 0xff);
}

/* Set bits in sram sw entry */
static void mvpp2_prs_sram_bits_set(struct mvpp2_prs_entry *pe, int bit_num,
				    int val)
{
	pe->sram.byte[MVPP2_BIT_TO_BYTE(bit_num)] |= (val << (bit_num % 8));
}

/* Clear bits in sram sw entry */
static void mvpp2_prs_sram_bits_clear(struct mvpp2_prs_entry *pe, int bit_num,
				      int val)
{
	pe->sram.byte[MVPP2_BIT_TO_BYTE(bit_num)] &= ~(val << (bit_num % 8));
}

/* Update ri bits in sram sw entry */
static void mvpp2_prs_sram_ri_update(struct mvpp2_prs_entry *pe,
				     unsigned int bits, unsigned int mask)
{
	unsigned int i;

	for (i = 0; i < MVPP2_PRS_SRAM_RI_CTRL_BITS; i++)
		if (mask & BIT(i)) {
			if (bits & BIT(i))
				mvpp2_prs_sram_bits_set(pe,
					      MVPP2_PRS_SRAM_RI_OFFS + i, 1);
			else
				mvpp2_prs_sram_bits_clear(pe,
					      MVPP2_PRS_SRAM_RI_OFFS + i, 1);
			mvpp2_prs_sram_bits_set(pe,
					 MVPP2_PRS_SRAM_RI_CTRL_OFFS + i, 1);
		}
}

/* Obtain ri bits and their mask from sram sw entry */
static void mvpp2_prs_sram_ri_get(struct mvpp2_prs_entry *pe,
				  unsigned int *bits, unsigned int *mask)
{
	*bits = pe->sram.word[MVPP2_PRS_SRAM_RI_WORD];
	*mask = pe->sram.word[MVPP2_PRS_SRAM_RI_CTRL_WORD];
}

/* Update ai bits in sram sw entry */
static void mvpp2_prs_sram_ai_update(struct mvpp2_prs_entry *pe,
				     unsigned int bits, unsigned int mask)
{
	unsigned int i;

	for (i = 0; i < MVPP2_PRS_SRAM_AI_CTRL_BITS; i++)
		if (mask & BIT(i)) {
			if (bits & BIT(i))
				mvpp2_prs_sram_bits_set(pe,
					      MVPP2_PRS_SRAM_AI_OFFS + i, 1);
			else
				mvpp2_prs_sram_bits_clear(pe,
					      MVPP2_PRS_SRAM_AI_OFFS + i, 1);
			mvpp2_prs_sram_bits_set(pe,
					 MVPP2_PRS_SRAM_AI_CTRL_OFFS + i, 1);
		}
}

/* Read ai bits from sram sw entry */
static void mvpp2_prs_sram_ai_get(struct mvpp2_prs_entry *pe,
				  unsigned int *bits, unsigned int *enable)
{
	*bits = (pe->sram.byte[MVPP2_BIT_TO_BYTE(MVPP2_PRS_SRAM_AI_OFFS)]
		>> (MVPP2_PRS_SRAM_AI_OFFS % 8)) |
		(pe->sram.byte[MVPP2_BIT_TO_BYTE(MVPP2_PRS_SRAM_AI_OFFS +
		MVPP2_PRS_SRAM_AI_CTRL_BITS)] <<
		(8 - (MVPP2_PRS_SRAM_AI_OFFS % 8)));

	*enable = (pe->sram.byte[MVPP2_BIT_TO_BYTE(MVPP2_PRS_SRAM_AI_CTRL_OFFS)]
		  >> (MVPP2_PRS_SRAM_AI_CTRL_OFFS % 8)) |
		  (pe->sram.byte[MVPP2_BIT_TO_BYTE(MVPP2_PRS_SRAM_AI_CTRL_OFFS +
		  MVPP2_PRS_SRAM_AI_CTRL_BITS)] <<
		  (8 - (MVPP2_PRS_SRAM_AI_CTRL_OFFS % 8)));

	*bits &= MVPP2_PRS_SRAM_AI_MASK;
	*enable &= MVPP2_PRS_SRAM_AI_MASK;
}

/* In sram sw entry set lookup ID field of the tcam key to be used in the next
 * lookup interation
 */
static void mvpp2_prs_sram_next_lu_set(struct mvpp2_prs_entry *pe,
				       unsigned int lu)
{
	mvpp2_prs_sram_bits_clear(pe, MVPP2_PRS_SRAM_NEXT_LU_OFFS,
				  MVPP2_PRS_SRAM_NEXT_LU_MASK);
	mvpp2_prs_sram_bits_set(pe, MVPP2_PRS_SRAM_NEXT_LU_OFFS, lu);
}

/* In the sram sw entry set sign and value of the next lookup offset
 * and the offset value generated to the classifier
 */
static void mvpp2_prs_sram_shift_set(struct mvpp2_prs_entry *pe, int shift,
				     unsigned int op)
{
	/* Set sign */
	if (shift < 0) {
		mvpp2_prs_sram_bits_set(pe, MVPP2_PRS_SRAM_SHIFT_SIGN_BIT, 1);
		shift = 0 - shift;
	} else
		mvpp2_prs_sram_bits_clear(pe, MVPP2_PRS_SRAM_SHIFT_SIGN_BIT, 1);

	/* Set value */
	pe->sram.byte[MVPP2_BIT_TO_BYTE(MVPP2_PRS_SRAM_SHIFT_OFFS)] =
							   (unsigned char)shift;

	/* Reset and set operation */
	mvpp2_prs_sram_bits_clear(pe, MVPP2_PRS_SRAM_OP_SEL_SHIFT_OFFS,
				  MVPP2_PRS_SRAM_OP_SEL_SHIFT_MASK);
	mvpp2_prs_sram_bits_set(pe, MVPP2_PRS_SRAM_OP_SEL_SHIFT_OFFS, op);

	/* Set base offset as current */
	mvpp2_prs_sram_bits_clear(pe, MVPP2_PRS_SRAM_OP_SEL_BASE_OFFS, 1);
}

/* In the sram sw entry set sign and value of the user defined offset
 * generated to the classifier
 */
static void mvpp2_prs_sram_offset_set(struct mvpp2_prs_entry *pe,
				      unsigned int type, int offset,
				      unsigned int op)
{
	/* Set sign */
	if (offset < 0) {
		mvpp2_prs_sram_bits_set(pe, MVPP2_PRS_SRAM_UDF_SIGN_BIT, 1);
		offset = 0 - offset;
	} else
		mvpp2_prs_sram_bits_clear(pe, MVPP2_PRS_SRAM_UDF_SIGN_BIT, 1);

	/* Set value */
	mvpp2_prs_sram_bits_clear(pe, MVPP2_PRS_SRAM_UDF_OFFS,
				  MVPP2_PRS_SRAM_UDF_MASK);
	mvpp2_prs_sram_bits_set(pe, MVPP2_PRS_SRAM_UDF_OFFS, offset);
	pe->sram.byte[MVPP2_BIT_TO_BYTE(MVPP2_PRS_SRAM_UDF_OFFS +
					MVPP2_PRS_SRAM_UDF_BITS)] &=
	      ~(MVPP2_PRS_SRAM_UDF_MASK >> (8 - (MVPP2_PRS_SRAM_UDF_OFFS % 8)));
	pe->sram.byte[MVPP2_BIT_TO_BYTE(MVPP2_PRS_SRAM_UDF_OFFS +
					MVPP2_PRS_SRAM_UDF_BITS)] |=
				(offset >> (8 - (MVPP2_PRS_SRAM_UDF_OFFS % 8)));

	/* Set offset type */
	mvpp2_prs_sram_bits_clear(pe, MVPP2_PRS_SRAM_UDF_TYPE_OFFS,
				  MVPP2_PRS_SRAM_UDF_TYPE_MASK);
	mvpp2_prs_sram_bits_set(pe, MVPP2_PRS_SRAM_UDF_TYPE_OFFS, type);

	/* Set offset operation */
	mvpp2_prs_sram_bits_clear(pe, MVPP2_PRS_SRAM_OP_SEL_UDF_OFFS,
				  MVPP2_PRS_SRAM_OP_SEL_UDF_MASK);
	mvpp2_prs_sram_bits_set(pe, MVPP2_PRS_SRAM_OP_SEL_UDF_OFFS, op);

	pe->sram.byte[MVPP2_BIT_TO_BYTE(MVPP2_PRS_SRAM_OP_SEL_UDF_OFFS +
					MVPP2_PRS_SRAM_OP_SEL_UDF_BITS)] &=
					     ~(MVPP2_PRS_SRAM_OP_SEL_UDF_MASK >>
				    (8 - (MVPP2_PRS_SRAM_OP_SEL_UDF_OFFS % 8)));

	pe->sram.byte[MVPP2_BIT_TO_BYTE(MVPP2_PRS_SRAM_OP_SEL_UDF_OFFS +
					MVPP2_PRS_SRAM_OP_SEL_UDF_BITS)] |=
			     (op >> (8 - (MVPP2_PRS_SRAM_OP_SEL_UDF_OFFS % 8)));

	/* Set base offset as current */
	mvpp2_prs_sram_bits_clear(pe, MVPP2_PRS_SRAM_OP_SEL_BASE_OFFS, 1);
}

/* Find parser flow entry */
static struct mvpp2_prs_entry *mvpp2_prs_flow_find(struct mvpp2 *pp2, int flow)
{
	struct mvpp2_prs_entry *pe;
	unsigned int enable;
	unsigned int bits;
	int tid;

	pe = kzalloc(sizeof(struct mvpp2_prs_entry), GFP_KERNEL);
	if (!pe)
		return NULL;
	mvpp2_prs_tcam_lu_set(pe, MVPP2_PRS_LU_FLOWS);

	/* Go through the all entires with MVPP2_PRS_LU_FLOWS */
	for (tid = MVPP2_PRS_TCAM_SRAM_SIZE - 1; tid >= 0; tid--) {
		if ((!pp2->prs_shadow[tid].valid) ||
		    (pp2->prs_shadow[tid].lu != MVPP2_PRS_LU_FLOWS))
			continue;

		pe->index = tid;
		mvpp2_prs_hw_read(pp2, pe);
		mvpp2_prs_sram_ai_get(pe, &bits, &enable);

		/* Sram store classification lookup ID in AI bits [5:0] */
		if ((bits & MVPP2_PRS_FLOW_ID_MASK) == flow)
			return pe;
	}
	kfree(pe);

	return NULL;
}

/* Return first free tcam index, seeking from start to end
 * If start < end - seek bottom-->up
 * If start > end - seek up-->bottom
 */
static int mvpp2_prs_tcam_first_free(struct mvpp2 *pp2, int start, int end)
{
	int tid;
	bool found = false;

	if (start < end)
		for (tid = start; tid <= end; tid++) {
			if (!pp2->prs_shadow[tid].valid) {
				found = true;
				break;
			}
		}
	else
		for (tid = start; tid >= end; tid--) {
			if (!pp2->prs_shadow[tid].valid) {
				found = true;
				break;
			}
		}

	if (found && (tid < MVPP2_PRS_TCAM_SRAM_SIZE) && (tid > -1))
		return tid;
	else
		return -ERANGE;
}

/* Enable/disable dropping all mac da's */
static void mvpp2_prs_mac_drop_all_set(struct mvpp2 *pp2, int port, bool add)
{
	struct mvpp2_prs_entry pe;

	if (pp2->prs_shadow[MVPP2_PE_DROP_ALL].valid) {
		/* Entry exist - update port only */
		pe.index = MVPP2_PE_DROP_ALL;
		mvpp2_prs_hw_read(pp2, &pe);
	} else {
		/* Entry doesn't exist - create new */
		memset(&pe, 0, sizeof(struct mvpp2_prs_entry));
		mvpp2_prs_tcam_lu_set(&pe, MVPP2_PRS_LU_MAC);
		pe.index = MVPP2_PE_DROP_ALL;

		/* Non-promiscuous mode for all ports - DROP unknown packets */
		mvpp2_prs_sram_ri_update(&pe, MVPP2_PRS_RI_DROP_MASK,
					 MVPP2_PRS_RI_DROP_MASK);

		mvpp2_prs_sram_bits_set(&pe, MVPP2_PRS_SRAM_LU_GEN_BIT, 1);
		mvpp2_prs_sram_next_lu_set(&pe, MVPP2_PRS_LU_FLOWS);

		/* Update shadow table */
		mvpp2_prs_shadow_set(pp2, pe.index, MVPP2_PRS_LU_MAC);

		/* Mask all ports */
		mvpp2_prs_tcam_port_map_set(&pe, 0);
	}

	/* Update port mask */
	mvpp2_prs_tcam_port_set(&pe, port, add);

	mvpp2_prs_hw_write(pp2, &pe);
}

/* Set port to promiscuous mode */
static void mvpp2_prs_mac_promisc_set(struct mvpp2 *pp2, int port, bool add)
{
	struct mvpp2_prs_entry pe;

	/* Promiscous mode - Accept unknown packets */

	if (pp2->prs_shadow[MVPP2_PE_MAC_PROMISCUOUS].valid) {
		/* Entry exist - update port only */
		pe.index = MVPP2_PE_MAC_PROMISCUOUS;
		mvpp2_prs_hw_read(pp2, &pe);
	} else {
		/* Entry doesn't exist - create new */
		memset(&pe, 0, sizeof(struct mvpp2_prs_entry));
		mvpp2_prs_tcam_lu_set(&pe, MVPP2_PRS_LU_MAC);
		pe.index = MVPP2_PE_MAC_PROMISCUOUS;

		/* Continue - set next lookup */
		mvpp2_prs_sram_next_lu_set(&pe, MVPP2_PRS_LU_DSA);

		/* Set result info bits */
		mvpp2_prs_sram_ri_update(&pe, MVPP2_PRS_RI_L2_UCAST,
					 MVPP2_PRS_RI_L2_CAST_MASK);

		/* Shift to ethertype */
		mvpp2_prs_sram_shift_set(&pe, 2 * ETH_ALEN,
					 MVPP2_PRS_SRAM_OP_SEL_SHIFT_ADD);

		/* Mask all ports */
		mvpp2_prs_tcam_port_map_set(&pe, 0);

		/* Update shadow table */
		mvpp2_prs_shadow_set(pp2, pe.index, MVPP2_PRS_LU_MAC);
	}

	/* Update port mask */
	mvpp2_prs_tcam_port_set(&pe, port, add);

	mvpp2_prs_hw_write(pp2, &pe);
}

/* Accept all multicast */
static void mvpp2_prs_mac_all_multi_set(struct mvpp2 *pp2, int port, bool add)
{
	struct mvpp2_prs_entry pe;
	unsigned int index = 0;
	unsigned int i;

	/* Ethernet multicast address first byte is
	 * 0x01 for IPv4 and 0x33 for IPv6
	 */
	unsigned char da_mc[MVPP2_PRS_MAX_MAC_MC] = { 0x01, 0x33 };

	for (i = MVPP2_PRS_IP4_MAC_MC; i < MVPP2_PRS_MAX_MAC_MC; i++) {
		if (i == MVPP2_PRS_IP4_MAC_MC)
			index = MVPP2_PE_MAC_MC_ALL;
		else
			index = MVPP2_PE_MAC_MC_IP6;

		if (pp2->prs_shadow[index].valid) {
			/* Entry exist - update port only */
			pe.index = index;
			mvpp2_prs_hw_read(pp2, &pe);
		} else {
			/* Entry doesn't exist - create new */
			memset(&pe, 0, sizeof(struct mvpp2_prs_entry));
			mvpp2_prs_tcam_lu_set(&pe, MVPP2_PRS_LU_MAC);
			pe.index = index;

			/* Continue - set next lookup */
			mvpp2_prs_sram_next_lu_set(&pe, MVPP2_PRS_LU_DSA);

			/* Set result info bits */
			mvpp2_prs_sram_ri_update(&pe, MVPP2_PRS_RI_L2_MCAST,
						 MVPP2_PRS_RI_L2_CAST_MASK);

			/* Update tcam entry data first byte */
			mvpp2_prs_tcam_data_byte_set(&pe, 0, da_mc[i], 0xff);

			/* Shift to ethertype */
			mvpp2_prs_sram_shift_set(&pe, 2 * ETH_ALEN,
					       MVPP2_PRS_SRAM_OP_SEL_SHIFT_ADD);

			/* Mask all ports */
			mvpp2_prs_tcam_port_map_set(&pe, 0);

			/* Update shadow table */
			mvpp2_prs_shadow_set(pp2, pe.index, MVPP2_PRS_LU_MAC);
		}

		/* Update port mask */
		mvpp2_prs_tcam_port_set(&pe, port, add);

		mvpp2_prs_hw_write(pp2, &pe);
	}
}

/* Set entry for dsa packets */
static void mvpp2_prs_dsa_tag_set(struct mvpp2 *pp2, int port, bool add,
				  bool tagged, bool extend)
{
	struct mvpp2_prs_entry pe;
	int tid;
	int shift;

	if (extend) {
		if (tagged)
			tid = MVPP2_PE_EDSA_TAGGED;
		else
			tid = MVPP2_PE_EDSA_UNTAGGED;
		shift = 8;
	} else {
		if (tagged)
			tid = MVPP2_PE_DSA_TAGGED;
		else
			tid = MVPP2_PE_DSA_UNTAGGED;
		shift = 4;
	}

	if (pp2->prs_shadow[tid].valid) {
		/* Entry exist - update port only */
		pe.index = tid;
		mvpp2_prs_hw_read(pp2, &pe);
	} else {
		/* Entry doesn't exist - create new */
		memset(&pe, 0, sizeof(struct mvpp2_prs_entry));
		mvpp2_prs_tcam_lu_set(&pe, MVPP2_PRS_LU_DSA);
		pe.index = tid;

		/* Shift 4 bytes if DSA tag or 8 bytes in case of EDSA tag*/
		mvpp2_prs_sram_shift_set(&pe, shift,
					 MVPP2_PRS_SRAM_OP_SEL_SHIFT_ADD);

		/* Update shadow table */
		mvpp2_prs_shadow_set(pp2, pe.index, MVPP2_PRS_LU_DSA);

		if (tagged) {
			/* Set tagged bit in DSA tag */
			mvpp2_prs_tcam_data_byte_set(&pe, 0,
						 MVPP2_PRS_TCAM_DSA_TAGGED_BIT,
						 MVPP2_PRS_TCAM_DSA_TAGGED_BIT);
			/* Clear all ai bits for next iteration */
			mvpp2_prs_sram_ai_update(&pe, 0,
						 MVPP2_PRS_SRAM_AI_MASK);
			/* If packet is tagged continue check vlans */
			mvpp2_prs_sram_next_lu_set(&pe, MVPP2_PRS_LU_VLAN);
		} else {
			/* Set result info bits to 'no vlans' */
			mvpp2_prs_sram_ri_update(&pe, MVPP2_PRS_RI_VLAN_NONE,
						 MVPP2_PRS_RI_VLAN_MASK);
			mvpp2_prs_sram_next_lu_set(&pe, MVPP2_PRS_LU_L2);
		}

		/* Mask all ports */
		mvpp2_prs_tcam_port_map_set(&pe, 0);
	}

	/* Update port mask */
	mvpp2_prs_tcam_port_set(&pe, port, add);

	mvpp2_prs_hw_write(pp2, &pe);
}

/* Set entry for dsa ethertype */
static void mvpp2_prs_dsa_tag_ethertype_set(struct mvpp2 *pp2, int port,
					   bool add, bool tagged, bool extend)
{
	struct mvpp2_prs_entry pe;
	int tid;
	int shift;
	int port_mask;

	if (extend) {
		if (tagged)
			tid = MVPP2_PE_ETYPE_EDSA_TAGGED;
		else
			tid = MVPP2_PE_ETYPE_EDSA_UNTAGGED;
		port_mask = 0;
		shift = 8;
	} else {
		if (tagged)
			tid = MVPP2_PE_ETYPE_DSA_TAGGED;
		else
			tid = MVPP2_PE_ETYPE_DSA_UNTAGGED;
		port_mask = MVPP2_PRS_PORT_MASK;
		shift = 4;
	}

	if (pp2->prs_shadow[tid].valid) {
		/* Entry exist - update port only */
		pe.index = tid;
		mvpp2_prs_hw_read(pp2, &pe);
	} else {
		/* Entry doesn't exist - create new */
		memset(&pe, 0, sizeof(struct mvpp2_prs_entry));
		mvpp2_prs_tcam_lu_set(&pe, MVPP2_PRS_LU_DSA);
		pe.index = tid;

		/* Set ethertype*/
		mvpp2_prs_match_etype(&pe, 0, ETH_P_EDSA);
		mvpp2_prs_match_etype(&pe, 2, 0);

		mvpp2_prs_sram_ri_update(&pe, MVPP2_PRS_RI_DSA_MASK,
				       MVPP2_PRS_RI_DSA_MASK);
		/* Shift ethertype + 2 byte reserved + tag*/
		mvpp2_prs_sram_shift_set(&pe, 2 + MVPP2_ETH_TYPE_LEN + shift,
					 MVPP2_PRS_SRAM_OP_SEL_SHIFT_ADD);

		/* Update shadow table */
		mvpp2_prs_shadow_set(pp2, pe.index, MVPP2_PRS_LU_DSA);

		if (tagged) {
			/* Set tagged bit in DSA tag */
			mvpp2_prs_tcam_data_byte_set(&pe,
						 MVPP2_ETH_TYPE_LEN + 2 + 3,
						 MVPP2_PRS_TCAM_DSA_TAGGED_BIT,
						 MVPP2_PRS_TCAM_DSA_TAGGED_BIT);
			/* Clear all ai bits for next iteration */
			mvpp2_prs_sram_ai_update(&pe, 0,
						 MVPP2_PRS_SRAM_AI_MASK);
			/* If packet is tagged continue check vlans */
			mvpp2_prs_sram_next_lu_set(&pe, MVPP2_PRS_LU_VLAN);
		} else {
			/* Set result info bits to 'no vlans' */
			mvpp2_prs_sram_ri_update(&pe, MVPP2_PRS_RI_VLAN_NONE,
						 MVPP2_PRS_RI_VLAN_MASK);
			mvpp2_prs_sram_next_lu_set(&pe, MVPP2_PRS_LU_L2);
		}
		/* Mask/unmask all ports, depending on dsa type */
		mvpp2_prs_tcam_port_map_set(&pe, port_mask);
	}

	/* Update port mask */
	mvpp2_prs_tcam_port_set(&pe, port, add);

	mvpp2_prs_hw_write(pp2, &pe);
}

/* Search for existing single/triple vlan entry */
static struct mvpp2_prs_entry *mvpp2_prs_vlan_find(struct mvpp2 *pp2,
						   unsigned short tpid, int ai)
{
	struct mvpp2_prs_entry *pe;
	unsigned int ri_bits;
	unsigned int ai_bits;
	unsigned int enable;
	unsigned char tpid_arr[2];
	int tid;

	pe = kzalloc(sizeof(struct mvpp2_prs_entry), GFP_KERNEL);
	if (!pe)
		return NULL;
	mvpp2_prs_tcam_lu_set(pe, MVPP2_PRS_LU_VLAN);

	tpid_arr[0] = ((unsigned char *)&tpid)[1];
	tpid_arr[1] = ((unsigned char *)&tpid)[0];

	/* Go through the all entries with MVPP2_PRS_LU_VLAN */
	for (tid = MVPP2_PE_FIRST_FREE_TID;
	     tid <= MVPP2_PE_LAST_FREE_TID; tid++) {
		if ((!pp2->prs_shadow[tid].valid) ||
		    (pp2->prs_shadow[tid].lu != MVPP2_PRS_LU_VLAN))
			continue;

		pe->index = tid;

		mvpp2_prs_hw_read(pp2, pe);
		if (!mvpp2_prs_tcam_data_cmp(pe, 0, 2, tpid_arr)) {
			/* Get vlan type */
			mvpp2_prs_sram_ri_get(pe, &ri_bits, &enable);
			ri_bits = (ri_bits & MVPP2_PRS_RI_VLAN_MASK);

			/* Get current ai value from tcam */
			mvpp2_prs_tcam_ai_get(pe, &ai_bits, &enable);
			/* Clear double vlan bit */
			ai_bits &= ~MVPP2_PRS_DBL_VLAN_AI_BIT;

			if (ai != ai_bits)
				continue;

			if ((ri_bits == MVPP2_PRS_RI_VLAN_SINGLE) ||
			    (ri_bits == MVPP2_PRS_RI_VLAN_TRIPLE))
				return pe;
		}
	}
	kfree(pe);

	return NULL;
}

/* Add/update single/triple vlan entry */
static int mvpp2_prs_vlan_add(struct mvpp2 *pp2, unsigned short tpid, int ai,
			      unsigned int port_map)
{
	struct mvpp2_prs_entry *pe = NULL;
	unsigned int bits;
	unsigned int enable;
	int tid_aux;
	int tid;

	pe = mvpp2_prs_vlan_find(pp2, tpid, ai);

	if (!pe) {
		/* Create new tcam entry */
		tid = mvpp2_prs_tcam_first_free(pp2, MVPP2_PE_LAST_FREE_TID,
						MVPP2_PE_FIRST_FREE_TID);
		if (tid == -ERANGE) {
			dprintk("%s: No free TCAM entry\n", __func__);
			return tid;
		}

		pe = kzalloc(sizeof(struct mvpp2_prs_entry), GFP_KERNEL);
		if (!pe)
			return -ENOMEM;

		/* Get last double vlan tid */
		for (tid_aux = MVPP2_PE_LAST_FREE_TID;
		     tid_aux >= MVPP2_PE_FIRST_FREE_TID; tid_aux--) {
			if ((!pp2->prs_shadow[tid_aux].valid) ||
			    (pp2->prs_shadow[tid_aux].lu != MVPP2_PRS_LU_VLAN))
				continue;

			pe->index = tid_aux;
			mvpp2_prs_hw_read(pp2, pe);
			mvpp2_prs_sram_ri_get(pe, &bits, &enable);
			if ((bits & MVPP2_PRS_RI_VLAN_MASK) ==
			    MVPP2_PRS_RI_VLAN_DOUBLE)
				break;
		}
		if (tid <= tid_aux) {
			dprintk("%s: Too much triple or single vlans entries\n",
				   __func__);
			return -1;
		}

		memset(pe, 0 , sizeof(struct mvpp2_prs_entry));
		mvpp2_prs_tcam_lu_set(pe, MVPP2_PRS_LU_VLAN);
		pe->index = tid;

		mvpp2_prs_match_etype(pe, 0, tpid);

		mvpp2_prs_sram_next_lu_set(pe, MVPP2_PRS_LU_L2);
		/* Shift 4 bytes - skip 1 vlan tag */
		mvpp2_prs_sram_shift_set(pe, MVPP2_VLAN_TAG_LEN,
					 MVPP2_PRS_SRAM_OP_SEL_SHIFT_ADD);
		/* Clear all ai bits for next iteration */
		mvpp2_prs_sram_ai_update(pe, 0, MVPP2_PRS_SRAM_AI_MASK);

		if (ai == MVPP2_PRS_SINGLE_VLAN_AI) {
			dprintk("%s: Adding single-VLAN\n", __func__);
			mvpp2_prs_sram_ri_update(pe, MVPP2_PRS_RI_VLAN_SINGLE,
						 MVPP2_PRS_RI_VLAN_MASK);
		} else {
			dprintk("%s: Adding triple-VLAN-%d\n", __func__, ai);
			ai |= MVPP2_PRS_DBL_VLAN_AI_BIT;
			mvpp2_prs_sram_ri_update(pe, MVPP2_PRS_RI_VLAN_TRIPLE,
						 MVPP2_PRS_RI_VLAN_MASK);
		}
		mvpp2_prs_tcam_ai_update(pe, ai, MVPP2_PRS_SRAM_AI_MASK);

		mvpp2_prs_shadow_set(pp2, pe->index, MVPP2_PRS_LU_VLAN);
	}
	/* Update ports' mask */
	mvpp2_prs_tcam_port_map_set(pe, port_map);

	mvpp2_prs_hw_write(pp2, pe);

	kfree(pe);

	return 0;
}

/* Get first free double vlan ai number */
static int mvpp2_prs_double_vlan_ai_free_get(struct mvpp2 *pp2)
{
	int i;

	for (i = 1; i < MVPP2_PRS_DBL_VLANS_MAX; i++)
		if (!pp2->prs_double_vlans[i])
			return i;

	return -ERANGE;
}

/* Search for existing double vlan entry */
static struct mvpp2_prs_entry *mvpp2_prs_double_vlan_find(struct mvpp2 *pp2,
							  unsigned short tpid1,
							  unsigned short tpid2)
{
	struct mvpp2_prs_entry *pe;
	unsigned int enable;
	unsigned int bits;
	unsigned char tpid_arr1[2];
	unsigned char tpid_arr2[2];
	int tid;

	tpid_arr1[0] = ((unsigned char *)&tpid1)[1];
	tpid_arr1[1] = ((unsigned char *)&tpid1)[0];

	tpid_arr2[0] = ((unsigned char *)&tpid2)[1];
	tpid_arr2[1] = ((unsigned char *)&tpid2)[0];

	pe = kzalloc(sizeof(struct mvpp2_prs_entry), GFP_KERNEL);
	if (!pe)
		return NULL;
	mvpp2_prs_tcam_lu_set(pe, MVPP2_PRS_LU_VLAN);

	/* Go through the all entries with MVPP2_PRS_LU_VLAN */
	for (tid = MVPP2_PE_FIRST_FREE_TID;
	     tid <= MVPP2_PE_LAST_FREE_TID; tid++) {
		if ((!pp2->prs_shadow[tid].valid) ||
		    (pp2->prs_shadow[tid].lu != MVPP2_PRS_LU_VLAN))
			continue;

		pe->index = tid;
		mvpp2_prs_hw_read(pp2, pe);

		if (!mvpp2_prs_tcam_data_cmp(pe, 0, 2, tpid_arr1) &&
		    !mvpp2_prs_tcam_data_cmp(pe, 4, 2, tpid_arr2)) {
			mvpp2_prs_sram_ri_get(pe, &bits, &enable);
			if ((bits & MVPP2_PRS_RI_VLAN_MASK) ==
			    MVPP2_PRS_RI_VLAN_DOUBLE)
				return pe;
		}
	}
	kfree(pe);

	return NULL;
}

/* Add or update double vlan entry */
static int mvpp2_prs_double_vlan_add(struct mvpp2 *pp2, unsigned short tpid1,
				     unsigned short tpid2,
				     unsigned int port_map)
{
	struct mvpp2_prs_entry *pe = NULL;
	unsigned int bits;
	unsigned int enable;
	int tid_aux;
	int tid;
	int ai;

	pe = mvpp2_prs_double_vlan_find(pp2, tpid1, tpid2);

	if (!pe) {
		/* Create new tcam entry */
		tid = mvpp2_prs_tcam_first_free(pp2, MVPP2_PE_FIRST_FREE_TID,
						MVPP2_PE_LAST_FREE_TID);
		if (tid == -ERANGE) {
			dprintk("%s: No free TCAM entry\n", __func__);
			return tid;
		}

		pe = kzalloc(sizeof(struct mvpp2_prs_entry), GFP_KERNEL);
		if (!pe)
			return -ENOMEM;

		/* Set ai value for new double vlan entry */
		ai = mvpp2_prs_double_vlan_ai_free_get(pp2);
		if (ai == -ERANGE) {
			dprintk("%s: Can't add one more double vlan entry\n",
				__func__);
			return ai;
		}

		/* Get first single/triple vlan tid */
		for (tid_aux = MVPP2_PE_FIRST_FREE_TID;
		     tid_aux <= MVPP2_PE_LAST_FREE_TID; tid_aux++) {
			if ((!pp2->prs_shadow[tid_aux].valid) ||
			    (pp2->prs_shadow[tid_aux].lu != MVPP2_PRS_LU_VLAN))
				continue;

			pe->index = tid_aux;
			mvpp2_prs_hw_read(pp2, pe);
			mvpp2_prs_sram_ri_get(pe, &bits, &enable);
			bits &= MVPP2_PRS_RI_VLAN_MASK;
			if ((bits == MVPP2_PRS_RI_VLAN_SINGLE) ||
			    (bits == MVPP2_PRS_RI_VLAN_TRIPLE))
				break;
		}
		if (tid >= tid_aux) {
			dprintk("%s: Double vlans entries overlapping\n",
				__func__);
			return -ERANGE;
		}

		memset(pe, 0 , sizeof(struct mvpp2_prs_entry));
		mvpp2_prs_tcam_lu_set(pe, MVPP2_PRS_LU_VLAN);
		pe->index = tid;

		pp2->prs_double_vlans[ai] = true;

		mvpp2_prs_match_etype(pe, 0, tpid1);
		mvpp2_prs_match_etype(pe, 4, tpid2);

		mvpp2_prs_sram_next_lu_set(pe, MVPP2_PRS_LU_VLAN);
		/* Shift 8 bytes - skip 2 vlan tags */
		mvpp2_prs_sram_shift_set(pe, 2 * MVPP2_VLAN_TAG_LEN,
					 MVPP2_PRS_SRAM_OP_SEL_SHIFT_ADD);
		mvpp2_prs_sram_ri_update(pe, MVPP2_PRS_RI_VLAN_DOUBLE,
					 MVPP2_PRS_RI_VLAN_MASK);
		mvpp2_prs_sram_ai_update(pe, (ai | MVPP2_PRS_DBL_VLAN_AI_BIT),
					 MVPP2_PRS_SRAM_AI_MASK);

		mvpp2_prs_shadow_set(pp2, pe->index, MVPP2_PRS_LU_VLAN);
	}
	/* Update ports' mask */
	mvpp2_prs_tcam_port_map_set(pe, port_map);

	mvpp2_prs_hw_write(pp2, pe);

	kfree(pe);

	return 0;
}

/* IPv4 header parsing for fragmentation and L4 offset */
static int mvpp2_prs_ip4_proto(struct mvpp2 *pp2, unsigned short proto,
			       unsigned int ri, unsigned int ri_mask)
{
	struct mvpp2_prs_entry pe;
	int tid;

	if ((proto != IPPROTO_TCP) && (proto != IPPROTO_UDP) &&
	    (proto != IPPROTO_IGMP)) {
		dprintk("%s: IPv4 unsupported protocol %d\n", __func__, proto);
		return -EINVAL;
	}

	/* Fragmented packet */
	tid = mvpp2_prs_tcam_first_free(pp2, MVPP2_PE_FIRST_FREE_TID,
					MVPP2_PE_LAST_FREE_TID);
	if (tid == -ERANGE) {
		dprintk("%s: No free TCAM entry\n", __func__);
		return tid;
	}

	memset(&pe, 0, sizeof(struct mvpp2_prs_entry));
	mvpp2_prs_tcam_lu_set(&pe, MVPP2_PRS_LU_IP4);
	pe.index = tid;

	/* Set next lu to IPv4 */
	mvpp2_prs_sram_next_lu_set(&pe, MVPP2_PRS_LU_IP4);
	mvpp2_prs_sram_shift_set(&pe, 12, MVPP2_PRS_SRAM_OP_SEL_SHIFT_ADD);
	/* Set L4 offset */
	mvpp2_prs_sram_offset_set(&pe, MVPP2_PRS_SRAM_UDF_TYPE_L4,
				  sizeof(struct iphdr) - 4,
				  MVPP2_PRS_SRAM_OP_SEL_UDF_ADD);
	mvpp2_prs_sram_ai_update(&pe, MVPP2_PRS_IPV4_DIP_AI_BIT,
				 MVPP2_PRS_IPV4_DIP_AI_BIT);
	mvpp2_prs_sram_ri_update(&pe, ri | MVPP2_PRS_RI_IP_FRAG_MASK,
				 ri_mask | MVPP2_PRS_RI_IP_FRAG_MASK);

	mvpp2_prs_tcam_data_byte_set(&pe, 5, proto, MVPP2_PRS_TCAM_PROTO_MASK);
	mvpp2_prs_tcam_ai_update(&pe, 0, MVPP2_PRS_IPV4_DIP_AI_BIT);
	/* Unmask all ports */
	mvpp2_prs_tcam_port_map_set(&pe, MVPP2_PRS_PORT_MASK);

	/* Update shadow table and hw entry */
	mvpp2_prs_shadow_set(pp2, pe.index, MVPP2_PRS_LU_IP4);
	mvpp2_prs_hw_write(pp2, &pe);

	/* Not fragmented packet */
	tid = mvpp2_prs_tcam_first_free(pp2, MVPP2_PE_FIRST_FREE_TID,
					MVPP2_PE_LAST_FREE_TID);
	if (tid == -ERANGE) {
		dprintk("%s: No free TCAM entry\n", __func__);
		return tid;
	}

	pe.index = tid;
	/* Clear ri before updating */
	pe.sram.word[MVPP2_PRS_SRAM_RI_WORD] = 0x0;
	pe.sram.word[MVPP2_PRS_SRAM_RI_CTRL_WORD] = 0x0;
	mvpp2_prs_sram_ri_update(&pe, ri, ri_mask);

	mvpp2_prs_tcam_data_byte_set(&pe, 2, 0x00, MVPP2_PRS_TCAM_PROTO_MASK_L);
	mvpp2_prs_tcam_data_byte_set(&pe, 3, 0x00, MVPP2_PRS_TCAM_PROTO_MASK);

	/* Update shadow table and hw entry */
	mvpp2_prs_shadow_set(pp2, pe.index, MVPP2_PRS_LU_IP4);
	mvpp2_prs_hw_write(pp2, &pe);

	return 0;
}

/* IPv4 L3 multicast or broadcast */
static int mvpp2_prs_ip4_cast(struct mvpp2 *pp2, unsigned short l3_cast)
{
	struct mvpp2_prs_entry pe;
	int mask;
	int tid;

	tid = mvpp2_prs_tcam_first_free(pp2, MVPP2_PE_FIRST_FREE_TID,
					MVPP2_PE_LAST_FREE_TID);
	if (tid == -ERANGE) {
		dprintk("%s: No free TCAM entry\n", __func__);
		return tid;
	}

	memset(&pe, 0, sizeof(struct mvpp2_prs_entry));
	mvpp2_prs_tcam_lu_set(&pe, MVPP2_PRS_LU_IP4);
	pe.index = tid;

	switch (l3_cast) {
	case MVPP2_PRS_L3_MULTI_CAST:
		mvpp2_prs_tcam_data_byte_set(&pe, 0, MVPP2_PRS_IPV4_MC,
					     MVPP2_PRS_IPV4_MC_MASK);
		mvpp2_prs_sram_ri_update(&pe, MVPP2_PRS_RI_L3_MCAST,
					 MVPP2_PRS_RI_L3_ADDR_MASK);
		break;
	case  MVPP2_PRS_L3_BROAD_CAST:
		mask = MVPP2_PRS_IPV4_BC_MASK;
		mvpp2_prs_tcam_data_byte_set(&pe, 0, mask, mask);
		mvpp2_prs_tcam_data_byte_set(&pe, 1, mask, mask);
		mvpp2_prs_tcam_data_byte_set(&pe, 2, mask, mask);
		mvpp2_prs_tcam_data_byte_set(&pe, 3, mask, mask);
		mvpp2_prs_sram_ri_update(&pe, MVPP2_PRS_RI_L3_BCAST,
					 MVPP2_PRS_RI_L3_ADDR_MASK);
		break;
	default:
		dprintk("%s: Invalid Input\n", __func__);
		return -EINVAL;
	}

	/* Finished: go to flowid generation */
	mvpp2_prs_sram_next_lu_set(&pe, MVPP2_PRS_LU_FLOWS);
	mvpp2_prs_sram_bits_set(&pe, MVPP2_PRS_SRAM_LU_GEN_BIT, 1);

	mvpp2_prs_tcam_ai_update(&pe, MVPP2_PRS_IPV4_DIP_AI_BIT,
				 MVPP2_PRS_IPV4_DIP_AI_BIT);
	/* Unmask all ports */
	mvpp2_prs_tcam_port_map_set(&pe, MVPP2_PRS_PORT_MASK);

	/* Update shadow table and hw entry */
	mvpp2_prs_shadow_set(pp2, pe.index, MVPP2_PRS_LU_IP4);
	mvpp2_prs_hw_write(pp2, &pe);

	return 0;
}

/* Set entries for protocols over IPv6  */
static int mvpp2_prs_ip6_proto(struct mvpp2 *pp2, unsigned short proto,
			       unsigned int ri, unsigned int ri_mask)
{
	struct mvpp2_prs_entry pe;
	int tid;

	if ((proto != IPPROTO_TCP) && (proto != IPPROTO_UDP) &&
	    (proto != IPPROTO_ICMPV6) && (proto != IPPROTO_IPIP)) {
		dprintk("%s: IPv6 unsupported protocol %d\n", __func__, proto);
		return -EINVAL;
	}

	tid = mvpp2_prs_tcam_first_free(pp2, MVPP2_PE_FIRST_FREE_TID,
					MVPP2_PE_LAST_FREE_TID);
	if (tid == -ERANGE) {
		dprintk("%s: No free TCAM entry\n", __func__);
		return tid;
	}

	memset(&pe, 0, sizeof(struct mvpp2_prs_entry));
	mvpp2_prs_tcam_lu_set(&pe, MVPP2_PRS_LU_IP6);
	pe.index = tid;

	/* Finished: go to flowid generation */
	mvpp2_prs_sram_next_lu_set(&pe, MVPP2_PRS_LU_FLOWS);
	mvpp2_prs_sram_bits_set(&pe, MVPP2_PRS_SRAM_LU_GEN_BIT, 1);
	mvpp2_prs_sram_ri_update(&pe, ri, ri_mask);
	mvpp2_prs_sram_offset_set(&pe, MVPP2_PRS_SRAM_UDF_TYPE_L4,
				  sizeof(struct ipv6hdr) - 6,
				  MVPP2_PRS_SRAM_OP_SEL_UDF_ADD);

	mvpp2_prs_tcam_data_byte_set(&pe, 0, proto, MVPP2_PRS_TCAM_PROTO_MASK);
	mvpp2_prs_tcam_ai_update(&pe, MVPP2_PRS_IPV6_NO_EXT_AI_BIT,
				 MVPP2_PRS_IPV6_NO_EXT_AI_BIT);
	/* Unmask all ports */
	mvpp2_prs_tcam_port_map_set(&pe, MVPP2_PRS_PORT_MASK);

	/* Write HW */
	mvpp2_prs_shadow_set(pp2, pe.index, MVPP2_PRS_LU_IP6);
	mvpp2_prs_hw_write(pp2, &pe);

	return 0;
}

/* IPv6 L3 multicast entry */
static int mvpp2_prs_ip6_cast(struct mvpp2 *pp2, unsigned short l3_cast)
{
	struct mvpp2_prs_entry pe;
	int tid;

	if (l3_cast != MVPP2_PRS_L3_MULTI_CAST) {
		dprintk("%s: Invalid Input\n", __func__);
		return -EINVAL;
	}

	tid = mvpp2_prs_tcam_first_free(pp2, MVPP2_PE_FIRST_FREE_TID,
					MVPP2_PE_LAST_FREE_TID);
	if (tid == -ERANGE) {
		dprintk("%s: No free TCAM entry\n", __func__);
		return tid;
	}

	memset(&pe, 0, sizeof(struct mvpp2_prs_entry));
	mvpp2_prs_tcam_lu_set(&pe, MVPP2_PRS_LU_IP6);
	pe.index = tid;

	/* Finished: go to flowid generation */
	mvpp2_prs_sram_next_lu_set(&pe, MVPP2_PRS_LU_IP6);
	mvpp2_prs_sram_ri_update(&pe, MVPP2_PRS_RI_L3_MCAST,
				 MVPP2_PRS_RI_L3_ADDR_MASK);
	mvpp2_prs_sram_ai_update(&pe, MVPP2_PRS_IPV6_NO_EXT_AI_BIT,
				 MVPP2_PRS_IPV6_NO_EXT_AI_BIT);
	/* Shift back to IPv6 NH */
	mvpp2_prs_sram_shift_set(&pe, -18, MVPP2_PRS_SRAM_OP_SEL_SHIFT_ADD);

	mvpp2_prs_tcam_data_byte_set(&pe, 0, MVPP2_PRS_IPV6_MC,
				     MVPP2_PRS_IPV6_MC_MASK);
	mvpp2_prs_tcam_ai_update(&pe, 0, MVPP2_PRS_IPV6_NO_EXT_AI_BIT);
	/* Unmask all ports */
	mvpp2_prs_tcam_port_map_set(&pe, MVPP2_PRS_PORT_MASK);

	/* Update shadow table and hw entry */
	mvpp2_prs_shadow_set(pp2, pe.index, MVPP2_PRS_LU_IP6);
	mvpp2_prs_hw_write(pp2, &pe);

	return 0;
}

/* Parser per-port initialization */
static void mvpp2_prs_hw_port_init(struct mvpp2 *pp2, int port, int lu_first,
			   int lu_max, int offset)
{
	u32 reg_val;

	/* Set lookup ID */
	reg_val = mvpp2_read(pp2, MVPP2_PRS_INIT_LOOKUP_REG);
	reg_val &= ~MVPP2_PRS_PORT_LU_MASK(port);
	reg_val |=  MVPP2_PRS_PORT_LU_VAL(port, lu_first);
	mvpp2_write(pp2, MVPP2_PRS_INIT_LOOKUP_REG, reg_val);

	/* Set maximum number of loops for packet received from port */
	reg_val = mvpp2_read(pp2, MVPP2_PRS_MAX_LOOP_REG(port));
	reg_val &= ~MVPP2_PRS_MAX_LOOP_MASK(port);
	reg_val |= MVPP2_PRS_MAX_LOOP_VAL(port, lu_max);
	mvpp2_write(pp2, MVPP2_PRS_MAX_LOOP_REG(port), reg_val);

	/* Set initial offset for packet header extraction for the first
	 * searching loop
	 */
	reg_val = mvpp2_read(pp2, MVPP2_PRS_INIT_OFFS_REG(port));
	reg_val &= ~MVPP2_PRS_INIT_OFF_MASK(port);
	reg_val |= MVPP2_PRS_INIT_OFF_VAL(port, offset);
	mvpp2_write(pp2, MVPP2_PRS_INIT_OFFS_REG(port), reg_val);
}

/* Default flow entries initialization for all ports */
static void mvpp2_prs_def_flow_init(struct mvpp2 *pp2)
{
	struct mvpp2_prs_entry pe;
	int port;

	for (port = 0; port < MVPP2_MAX_PORTS; port++) {
		memset(&pe, 0, sizeof(struct mvpp2_prs_entry));
		mvpp2_prs_tcam_lu_set(&pe, MVPP2_PRS_LU_FLOWS);
		pe.index = MVPP2_PE_FIRST_DEFAULT_FLOW - port;

		/* Mask all ports */
		mvpp2_prs_tcam_port_map_set(&pe, 0);

		/* Set flow ID*/
		mvpp2_prs_sram_ai_update(&pe, port, MVPP2_PRS_FLOW_ID_MASK);
		mvpp2_prs_sram_bits_set(&pe, MVPP2_PRS_SRAM_LU_DONE_BIT, 1);

		/* Update shadow table and hw entry */
		mvpp2_prs_shadow_set(pp2, pe.index, MVPP2_PRS_LU_FLOWS);
		mvpp2_prs_hw_write(pp2, &pe);
	}
}

/* Set default entry for Marvell Header field */
static void mvpp2_prs_mh_init(struct mvpp2 *pp2)
{
	struct mvpp2_prs_entry pe;

	memset(&pe, 0, sizeof(struct mvpp2_prs_entry));

	pe.index = MVPP2_PE_MH_DEFAULT;
	mvpp2_prs_tcam_lu_set(&pe, MVPP2_PRS_LU_MH);
	mvpp2_prs_sram_shift_set(&pe, MVPP2_MH_SIZE,
				 MVPP2_PRS_SRAM_OP_SEL_SHIFT_ADD);
	mvpp2_prs_sram_next_lu_set(&pe, MVPP2_PRS_LU_MAC);

	/* Unmask all ports */
	mvpp2_prs_tcam_port_map_set(&pe, MVPP2_PRS_PORT_MASK);

	/* Update shadow table and hw entry */
	mvpp2_prs_shadow_set(pp2, pe.index, MVPP2_PRS_LU_MH);
	mvpp2_prs_hw_write(pp2, &pe);
}

/* Set default entires (place holder) for promiscuous, non-promiscuous and
 * multicast MAC addresses
 */
static void mvpp2_prs_mac_init(struct mvpp2 *pp2)
{
	struct mvpp2_prs_entry pe;

	memset(&pe, 0, sizeof(struct mvpp2_prs_entry));

	/* Non-promiscuous mode for all ports - DROP unknown packets */
	pe.index = MVPP2_PE_MAC_NON_PROMISCUOUS;
	mvpp2_prs_tcam_lu_set(&pe, MVPP2_PRS_LU_MAC);

	mvpp2_prs_sram_ri_update(&pe, MVPP2_PRS_RI_DROP_MASK,
				 MVPP2_PRS_RI_DROP_MASK);
	mvpp2_prs_sram_bits_set(&pe, MVPP2_PRS_SRAM_LU_GEN_BIT, 1);
	mvpp2_prs_sram_next_lu_set(&pe, MVPP2_PRS_LU_FLOWS);

	/* Unmask all ports */
	mvpp2_prs_tcam_port_map_set(&pe, MVPP2_PRS_PORT_MASK);

	/* Update shadow table and hw entry */
	mvpp2_prs_shadow_set(pp2, pe.index, MVPP2_PRS_LU_MAC);
	mvpp2_prs_hw_write(pp2, &pe);

	/* place holders only - no ports */
	mvpp2_prs_mac_drop_all_set(pp2, 0, false);
	mvpp2_prs_mac_promisc_set(pp2, 0, false);
	mvpp2_prs_mac_all_multi_set(pp2, 0, false);
}

/* Set default entries for various types of dsa packets */
static void mvpp2_prs_dsa_init(struct mvpp2 *pp2)
{
	struct mvpp2_prs_entry pe;

	/* None tagged EDSA entry - place holder */
	mvpp2_prs_dsa_tag_set(pp2, 0, false, MVPP2_PRS_UNTAGGED,
			      MVPP2_PRS_EDSA);

	/* Tagged EDSA entry - place holder */
	mvpp2_prs_dsa_tag_set(pp2, 0, false, MVPP2_PRS_TAGGED, MVPP2_PRS_EDSA);

	/* None tagged DSA entry - place holder */
	mvpp2_prs_dsa_tag_set(pp2, 0, false, MVPP2_PRS_UNTAGGED, MVPP2_PRS_DSA);

	/* Tagged DSA entry - place holder */
	mvpp2_prs_dsa_tag_set(pp2, 0, false, MVPP2_PRS_TAGGED, MVPP2_PRS_DSA);

	/* None tagged EDSA ethertype entry - place holder*/
	mvpp2_prs_dsa_tag_ethertype_set(pp2, 0, false,
					MVPP2_PRS_UNTAGGED, MVPP2_PRS_EDSA);

	/* Tagged EDSA ethertype entry - place holder*/
	mvpp2_prs_dsa_tag_ethertype_set(pp2, 0, false,
					MVPP2_PRS_TAGGED, MVPP2_PRS_EDSA);

	/* None tagged DSA ethertype entry */
	mvpp2_prs_dsa_tag_ethertype_set(pp2, 0, true,
					MVPP2_PRS_UNTAGGED, MVPP2_PRS_DSA);

	/* Tagged DSA ethertype entry */
	mvpp2_prs_dsa_tag_ethertype_set(pp2, 0, true,
					MVPP2_PRS_TAGGED, MVPP2_PRS_DSA);

	/* Set default entry, in case DSA or EDSA tag not found */
	memset(&pe, 0, sizeof(struct mvpp2_prs_entry));
	mvpp2_prs_tcam_lu_set(&pe, MVPP2_PRS_LU_DSA);
	pe.index = MVPP2_PE_DSA_DEFAULT;
	mvpp2_prs_sram_next_lu_set(&pe, MVPP2_PRS_LU_VLAN);

	/* Shift 0 bytes */
	mvpp2_prs_sram_shift_set(&pe, 0, MVPP2_PRS_SRAM_OP_SEL_SHIFT_ADD);
	mvpp2_prs_shadow_set(pp2, pe.index, MVPP2_PRS_LU_MAC);

	/* Clear all sram ai bits for next iteration */
	mvpp2_prs_sram_ai_update(&pe, 0, MVPP2_PRS_SRAM_AI_MASK);

	/* Unmask all ports */
	mvpp2_prs_tcam_port_map_set(&pe, MVPP2_PRS_PORT_MASK);

	mvpp2_prs_hw_write(pp2, &pe);
}

/* Match basic ethertypes */
static int mvpp2_prs_etype_init(struct mvpp2 *pp2)
{
	struct mvpp2_prs_entry pe;
	int tid;

	/* Ethertype: PPPoE */
	tid = mvpp2_prs_tcam_first_free(pp2, MVPP2_PE_FIRST_FREE_TID,
					MVPP2_PE_LAST_FREE_TID);
	if (tid == -ERANGE) {
		dprintk("%s: No free TCAM entry (PPPoE)\n", __func__);
		return tid;
	}

	memset(&pe, 0, sizeof(struct mvpp2_prs_entry));
	mvpp2_prs_tcam_lu_set(&pe, MVPP2_PRS_LU_L2);
	pe.index = tid;

	mvpp2_prs_match_etype(&pe, 0, ETH_P_PPP_SES);

	mvpp2_prs_sram_shift_set(&pe, MVPP2_PPPOE_HDR_SIZE,
				 MVPP2_PRS_SRAM_OP_SEL_SHIFT_ADD);
	mvpp2_prs_sram_next_lu_set(&pe, MVPP2_PRS_LU_PPPOE);
	mvpp2_prs_sram_ri_update(&pe, MVPP2_PRS_RI_PPPOE_MASK,
				 MVPP2_PRS_RI_PPPOE_MASK);

	/* Update shadow table and hw entry */
	mvpp2_prs_shadow_set(pp2, pe.index, MVPP2_PRS_LU_L2);
	pp2->prs_shadow[pe.index].udf = MVPP2_PRS_UDF_L2_DEF;
	pp2->prs_shadow[pe.index].finish = false;
	mvpp2_prs_shadow_ri_set(pp2, pe.index, MVPP2_PRS_RI_PPPOE_MASK,
				MVPP2_PRS_RI_PPPOE_MASK);
	mvpp2_prs_hw_write(pp2, &pe);

	/* Ethertype: ARP */
	tid = mvpp2_prs_tcam_first_free(pp2, MVPP2_PE_FIRST_FREE_TID,
					MVPP2_PE_LAST_FREE_TID);
	if (tid == -ERANGE) {
		dprintk("%s: No free TCAM entry (ARP)\n", __func__);
		return tid;
	}

	memset(&pe, 0, sizeof(struct mvpp2_prs_entry));
	mvpp2_prs_tcam_lu_set(&pe, MVPP2_PRS_LU_L2);
	pe.index = tid;

	mvpp2_prs_match_etype(&pe, 0, ETH_P_ARP);

	/* Generate flow in the next iteration*/
	mvpp2_prs_sram_next_lu_set(&pe, MVPP2_PRS_LU_FLOWS);
	mvpp2_prs_sram_bits_set(&pe, MVPP2_PRS_SRAM_LU_GEN_BIT, 1);
	mvpp2_prs_sram_ri_update(&pe, MVPP2_PRS_RI_L3_ARP,
				 MVPP2_PRS_RI_L3_PROTO_MASK);
	/* Set L3 offset */
	mvpp2_prs_sram_offset_set(&pe, MVPP2_PRS_SRAM_UDF_TYPE_L3,
				  MVPP2_ETH_TYPE_LEN,
				  MVPP2_PRS_SRAM_OP_SEL_UDF_ADD);

	/* Update shadow table and hw entry */
	mvpp2_prs_shadow_set(pp2, pe.index, MVPP2_PRS_LU_L2);
	pp2->prs_shadow[pe.index].udf = MVPP2_PRS_UDF_L2_DEF;
	pp2->prs_shadow[pe.index].finish = true;
	mvpp2_prs_shadow_ri_set(pp2, pe.index, MVPP2_PRS_RI_L3_ARP,
				MVPP2_PRS_RI_L3_PROTO_MASK);
	mvpp2_prs_hw_write(pp2, &pe);

	/* Ethertype: LBTD */
	tid = mvpp2_prs_tcam_first_free(pp2, MVPP2_PE_FIRST_FREE_TID,
					MVPP2_PE_LAST_FREE_TID);
	if (tid == -ERANGE) {
		dprintk("%s: No free TCAM entry (LBTD)\n", __func__);
		return tid;
	}

	memset(&pe, 0, sizeof(struct mvpp2_prs_entry));
	mvpp2_prs_tcam_lu_set(&pe, MVPP2_PRS_LU_L2);
	pe.index = tid;

	mvpp2_prs_match_etype(&pe, 0, MVPP2_IP_LBDT_TYPE);

	/* Generate flow in the next iteration*/
	mvpp2_prs_sram_next_lu_set(&pe, MVPP2_PRS_LU_FLOWS);
	mvpp2_prs_sram_bits_set(&pe, MVPP2_PRS_SRAM_LU_GEN_BIT, 1);
	mvpp2_prs_sram_ri_update(&pe, MVPP2_PRS_RI_CPU_CODE_RX_SPEC |
				 MVPP2_PRS_RI_UDF3_RX_SPECIAL,
				 MVPP2_PRS_RI_CPU_CODE_MASK |
				 MVPP2_PRS_RI_UDF3_MASK);
	/* Set L3 offset */
	mvpp2_prs_sram_offset_set(&pe, MVPP2_PRS_SRAM_UDF_TYPE_L3,
				  MVPP2_ETH_TYPE_LEN,
				  MVPP2_PRS_SRAM_OP_SEL_UDF_ADD);

	/* Update shadow table and hw entry */
	mvpp2_prs_shadow_set(pp2, pe.index, MVPP2_PRS_LU_L2);
	pp2->prs_shadow[pe.index].udf = MVPP2_PRS_UDF_L2_DEF;
	pp2->prs_shadow[pe.index].finish = true;
	mvpp2_prs_shadow_ri_set(pp2, pe.index, MVPP2_PRS_RI_CPU_CODE_RX_SPEC |
				MVPP2_PRS_RI_UDF3_RX_SPECIAL,
				MVPP2_PRS_RI_CPU_CODE_MASK |
				MVPP2_PRS_RI_UDF3_MASK);
	mvpp2_prs_hw_write(pp2, &pe);

	/* Ethertype: IPv4 without options */
	tid = mvpp2_prs_tcam_first_free(pp2, MVPP2_PE_FIRST_FREE_TID,
					MVPP2_PE_LAST_FREE_TID);
	if (tid == -ERANGE) {
		dprintk("%s: No free TCAM entry (IPv4 wo options)\n", __func__);
		return tid;
	}

	memset(&pe, 0, sizeof(struct mvpp2_prs_entry));
	mvpp2_prs_tcam_lu_set(&pe, MVPP2_PRS_LU_L2);
	pe.index = tid;

	mvpp2_prs_match_etype(&pe, 0, ETH_P_IP);
	mvpp2_prs_tcam_data_byte_set(&pe, MVPP2_ETH_TYPE_LEN,
				     MVPP2_PRS_IPV4_HEAD | MVPP2_PRS_IPV4_IHL,
				     MVPP2_PRS_IPV4_HEAD_MASK |
				     MVPP2_PRS_IPV4_IHL_MASK);

	mvpp2_prs_sram_next_lu_set(&pe, MVPP2_PRS_LU_IP4);
	mvpp2_prs_sram_ri_update(&pe, MVPP2_PRS_RI_L3_IP4,
				 MVPP2_PRS_RI_L3_PROTO_MASK);
	/* Skip eth_type + 4 bytes of IP header */
	mvpp2_prs_sram_shift_set(&pe, MVPP2_ETH_TYPE_LEN + 4,
				 MVPP2_PRS_SRAM_OP_SEL_SHIFT_ADD);
	/* Set L3 offset */
	mvpp2_prs_sram_offset_set(&pe, MVPP2_PRS_SRAM_UDF_TYPE_L3,
				  MVPP2_ETH_TYPE_LEN,
				  MVPP2_PRS_SRAM_OP_SEL_UDF_ADD);

	/* Update shadow table and hw entry */
	mvpp2_prs_shadow_set(pp2, pe.index, MVPP2_PRS_LU_L2);
	pp2->prs_shadow[pe.index].udf = MVPP2_PRS_UDF_L2_DEF;
	pp2->prs_shadow[pe.index].finish = false;
	mvpp2_prs_shadow_ri_set(pp2, pe.index, MVPP2_PRS_RI_L3_IP4,
				MVPP2_PRS_RI_L3_PROTO_MASK);
	mvpp2_prs_hw_write(pp2, &pe);

	/* Ethertype: IPv4 with options */
	tid = mvpp2_prs_tcam_first_free(pp2, MVPP2_PE_FIRST_FREE_TID,
					MVPP2_PE_LAST_FREE_TID);
	if (tid == -ERANGE) {
		dprintk("%s: No free TCAM entry (IPv4 w options)\n", __func__);
		return tid;
	}

	pe.index = tid;

	/* Clear tcam data before updating */
	mvpp2_prs_tcam_data_byte_clear(&pe, MVPP2_ETH_TYPE_LEN);
	mvpp2_prs_tcam_data_byte_set(&pe, MVPP2_ETH_TYPE_LEN,
				     MVPP2_PRS_IPV4_HEAD,
				     MVPP2_PRS_IPV4_HEAD_MASK);

	/* Clear ri before updating */
	pe.sram.word[MVPP2_PRS_SRAM_RI_WORD] = 0x0;
	pe.sram.word[MVPP2_PRS_SRAM_RI_CTRL_WORD] = 0x0;
	mvpp2_prs_sram_ri_update(&pe, MVPP2_PRS_RI_L3_IP4_OPT,
				 MVPP2_PRS_RI_L3_PROTO_MASK);

	/* Update shadow table and hw entry */
	mvpp2_prs_shadow_set(pp2, pe.index, MVPP2_PRS_LU_L2);
	pp2->prs_shadow[pe.index].udf = MVPP2_PRS_UDF_L2_DEF;
	pp2->prs_shadow[pe.index].finish = false;
	mvpp2_prs_shadow_ri_set(pp2, pe.index, MVPP2_PRS_RI_L3_IP4_OPT,
				MVPP2_PRS_RI_L3_PROTO_MASK);
	mvpp2_prs_hw_write(pp2, &pe);

	/* Ethertype: IPv6 without options */
	tid = mvpp2_prs_tcam_first_free(pp2, MVPP2_PE_FIRST_FREE_TID,
					MVPP2_PE_LAST_FREE_TID);
	if (tid == -ERANGE) {
		dprintk("%s: No free TCAM entry (IPv6)\n", __func__);
		return tid;
	}

	memset(&pe, 0, sizeof(struct mvpp2_prs_entry));
	mvpp2_prs_tcam_lu_set(&pe, MVPP2_PRS_LU_L2);
	pe.index = tid;

	mvpp2_prs_match_etype(&pe, 0, ETH_P_IPV6);

	/* Skip DIP of IPV6 header */
	mvpp2_prs_sram_shift_set(&pe, MVPP2_ETH_TYPE_LEN + 8 +
				 MVPP2_MAX_L3_ADDR_SIZE,
				 MVPP2_PRS_SRAM_OP_SEL_SHIFT_ADD);
	mvpp2_prs_sram_next_lu_set(&pe, MVPP2_PRS_LU_IP6);
	mvpp2_prs_sram_ri_update(&pe, MVPP2_PRS_RI_L3_IP6,
				 MVPP2_PRS_RI_L3_PROTO_MASK);
	/* Set L3 offset */
	mvpp2_prs_sram_offset_set(&pe, MVPP2_PRS_SRAM_UDF_TYPE_L3,
				  MVPP2_ETH_TYPE_LEN,
				  MVPP2_PRS_SRAM_OP_SEL_UDF_ADD);

	mvpp2_prs_shadow_set(pp2, pe.index, MVPP2_PRS_LU_L2);
	pp2->prs_shadow[pe.index].udf = MVPP2_PRS_UDF_L2_DEF;
	pp2->prs_shadow[pe.index].finish = false;
	mvpp2_prs_shadow_ri_set(pp2, pe.index, MVPP2_PRS_RI_L3_IP6,
				MVPP2_PRS_RI_L3_PROTO_MASK);
	mvpp2_prs_hw_write(pp2, &pe);

	/* Default entry for MVPP2_PRS_LU_L2 - Unknown ethtype */
	memset(&pe, 0, sizeof(struct mvpp2_prs_entry));
	mvpp2_prs_tcam_lu_set(&pe, MVPP2_PRS_LU_L2);
	pe.index = MVPP2_PE_ETH_TYPE_UN;

	/* Unmask all ports */
	mvpp2_prs_tcam_port_map_set(&pe, MVPP2_PRS_PORT_MASK);

	/* Generate flow in the next iteration*/
	mvpp2_prs_sram_bits_set(&pe, MVPP2_PRS_SRAM_LU_GEN_BIT, 1);
	mvpp2_prs_sram_next_lu_set(&pe, MVPP2_PRS_LU_FLOWS);
	mvpp2_prs_sram_ri_update(&pe, MVPP2_PRS_RI_L3_UN,
				 MVPP2_PRS_RI_L3_PROTO_MASK);
	/* Set L3 offset even it's unknown L3 */
	mvpp2_prs_sram_offset_set(&pe, MVPP2_PRS_SRAM_UDF_TYPE_L3,
				  MVPP2_ETH_TYPE_LEN,
				  MVPP2_PRS_SRAM_OP_SEL_UDF_ADD);

	/* Update shadow table and hw entry */
	mvpp2_prs_shadow_set(pp2, pe.index, MVPP2_PRS_LU_L2);
	pp2->prs_shadow[pe.index].udf = MVPP2_PRS_UDF_L2_DEF;
	pp2->prs_shadow[pe.index].finish = true;
	mvpp2_prs_shadow_ri_set(pp2, pe.index, MVPP2_PRS_RI_L3_UN,
				MVPP2_PRS_RI_L3_PROTO_MASK);
	mvpp2_prs_hw_write(pp2, &pe);

	return 0;
}

/* Configure vlan entries and detect up to 2 successive VLAN tags.
 * Possible options:
 * 0x8100, 0x88A8
 * 0x8100, 0x8100
 * 0x8100
 * 0x88A8
 */
static int mvpp2_prs_vlan_init(struct platform_device *pdev, struct mvpp2 *pp2)
{
	struct mvpp2_prs_entry pe;
	int err;

	pp2->prs_double_vlans = devm_kzalloc(&pdev->dev, sizeof(bool) *
					     MVPP2_PRS_DBL_VLANS_MAX,
					     GFP_KERNEL);
	if (!pp2->prs_double_vlans)
		return -ENOMEM;

	/* Double VLAN: 0x8100, 0x88A8 */
	err = mvpp2_prs_double_vlan_add(pp2, ETH_P_8021Q, ETH_P_8021AD,
					MVPP2_PRS_PORT_MASK);
	if (err)
		return err;

	/* Double VLAN: 0x8100, 0x8100 */
	err = mvpp2_prs_double_vlan_add(pp2, ETH_P_8021Q, ETH_P_8021Q,
					MVPP2_PRS_PORT_MASK);
	if (err)
		return err;

	/* Single VLAN: 0x88a8 */
	err = mvpp2_prs_vlan_add(pp2, ETH_P_8021AD, MVPP2_PRS_SINGLE_VLAN_AI,
				 MVPP2_PRS_PORT_MASK);
	if (err)
		return err;

	/* Single VLAN: 0x8100 */
	err = mvpp2_prs_vlan_add(pp2, ETH_P_8021Q, MVPP2_PRS_SINGLE_VLAN_AI,
				 MVPP2_PRS_PORT_MASK);
	if (err)
		return err;

	/* Set default double vlan entry */
	memset(&pe, 0, sizeof(struct mvpp2_prs_entry));
	mvpp2_prs_tcam_lu_set(&pe, MVPP2_PRS_LU_VLAN);
	pe.index = MVPP2_PE_VLAN_DBL;

	mvpp2_prs_sram_next_lu_set(&pe, MVPP2_PRS_LU_L2);
	/* Clear ai for next iterations */
	mvpp2_prs_sram_ai_update(&pe, 0, MVPP2_PRS_SRAM_AI_MASK);
	mvpp2_prs_sram_ri_update(&pe, MVPP2_PRS_RI_VLAN_DOUBLE,
				 MVPP2_PRS_RI_VLAN_MASK);

	mvpp2_prs_tcam_ai_update(&pe, MVPP2_PRS_DBL_VLAN_AI_BIT,
				 MVPP2_PRS_DBL_VLAN_AI_BIT);
	/* Unmask all ports */
	mvpp2_prs_tcam_port_map_set(&pe, MVPP2_PRS_PORT_MASK);

	/* Update shadow table and hw entry */
	mvpp2_prs_shadow_set(pp2, pe.index, MVPP2_PRS_LU_VLAN);
	mvpp2_prs_hw_write(pp2, &pe);

	/* Set default vlan none entry */
	memset(&pe, 0, sizeof(struct mvpp2_prs_entry));
	mvpp2_prs_tcam_lu_set(&pe, MVPP2_PRS_LU_VLAN);
	pe.index = MVPP2_PE_VLAN_NONE;

	mvpp2_prs_sram_next_lu_set(&pe, MVPP2_PRS_LU_L2);
	mvpp2_prs_sram_ri_update(&pe, MVPP2_PRS_RI_VLAN_NONE,
				 MVPP2_PRS_RI_VLAN_MASK);

	/* Unmask all ports */
	mvpp2_prs_tcam_port_map_set(&pe, MVPP2_PRS_PORT_MASK);

	/* Update shadow table and hw entry */
	mvpp2_prs_shadow_set(pp2, pe.index, MVPP2_PRS_LU_VLAN);
	mvpp2_prs_hw_write(pp2, &pe);

	return 0;
}

/* Set entries for PPPoE ethertype */
static int mvpp2_prs_pppoe_init(struct mvpp2 *pp2)
{
	struct mvpp2_prs_entry pe;
	int tid;

	/* IPv4 over PPPoE with options */
	tid = mvpp2_prs_tcam_first_free(pp2, MVPP2_PE_FIRST_FREE_TID,
					MVPP2_PE_LAST_FREE_TID);
	if (tid == -ERANGE) {
		dprintk("%s: No free TCAM entry (IPv4 w options)\n", __func__);
		return tid;
	}

	memset(&pe, 0, sizeof(struct mvpp2_prs_entry));
	mvpp2_prs_tcam_lu_set(&pe, MVPP2_PRS_LU_PPPOE);
	pe.index = tid;

	mvpp2_prs_match_etype(&pe, 0, PPP_IP);

	mvpp2_prs_sram_next_lu_set(&pe, MVPP2_PRS_LU_IP4);
	mvpp2_prs_sram_ri_update(&pe, MVPP2_PRS_RI_L3_IP4_OPT,
				 MVPP2_PRS_RI_L3_PROTO_MASK);
	/* Skip eth_type + 4 bytes of IP header */
	mvpp2_prs_sram_shift_set(&pe, MVPP2_ETH_TYPE_LEN + 4,
				 MVPP2_PRS_SRAM_OP_SEL_SHIFT_ADD);
	/* Set L3 offset */
	mvpp2_prs_sram_offset_set(&pe, MVPP2_PRS_SRAM_UDF_TYPE_L3,
				  MVPP2_ETH_TYPE_LEN,
				  MVPP2_PRS_SRAM_OP_SEL_UDF_ADD);

	/* Update shadow table and hw entry */
	mvpp2_prs_shadow_set(pp2, pe.index, MVPP2_PRS_LU_PPPOE);
	mvpp2_prs_hw_write(pp2, &pe);

	/* IPv4 over PPPoE without options */
	tid = mvpp2_prs_tcam_first_free(pp2, MVPP2_PE_FIRST_FREE_TID,
					MVPP2_PE_LAST_FREE_TID);
	if (tid == -ERANGE) {
		dprintk("%s: No free TCAM entry (IPv4 wo options)\n", __func__);
		return tid;
	}

	pe.index = tid;

	mvpp2_prs_tcam_data_byte_set(&pe, MVPP2_ETH_TYPE_LEN,
				     MVPP2_PRS_IPV4_HEAD | MVPP2_PRS_IPV4_IHL,
				     MVPP2_PRS_IPV4_HEAD_MASK |
				     MVPP2_PRS_IPV4_IHL_MASK);

	/* Clear ri before updating */
	pe.sram.word[MVPP2_PRS_SRAM_RI_WORD] = 0x0;
	pe.sram.word[MVPP2_PRS_SRAM_RI_CTRL_WORD] = 0x0;
	mvpp2_prs_sram_ri_update(&pe, MVPP2_PRS_RI_L3_IP4,
				 MVPP2_PRS_RI_L3_PROTO_MASK);

	/* Update shadow table and hw entry */
	mvpp2_prs_shadow_set(pp2, pe.index, MVPP2_PRS_LU_PPPOE);
	mvpp2_prs_hw_write(pp2, &pe);

	/* IPv6 over PPPoE */
	tid = mvpp2_prs_tcam_first_free(pp2, MVPP2_PE_FIRST_FREE_TID,
					MVPP2_PE_LAST_FREE_TID);
	if (tid == -ERANGE) {
		dprintk("%s: No free TCAM entry (IPv6)\n", __func__);
		return tid;
	}

	memset(&pe, 0, sizeof(struct mvpp2_prs_entry));
	mvpp2_prs_tcam_lu_set(&pe, MVPP2_PRS_LU_PPPOE);
	pe.index = tid;

	mvpp2_prs_match_etype(&pe, 0, PPP_IPV6);

	mvpp2_prs_sram_next_lu_set(&pe, MVPP2_PRS_LU_IP6);
	mvpp2_prs_sram_ri_update(&pe, MVPP2_PRS_RI_L3_IP6,
				 MVPP2_PRS_RI_L3_PROTO_MASK);
	/* Skip eth_type + 4 bytes of IPv6 header */
	mvpp2_prs_sram_shift_set(&pe, MVPP2_ETH_TYPE_LEN + 4,
				 MVPP2_PRS_SRAM_OP_SEL_SHIFT_ADD);
	/* Set L3 offset */
	mvpp2_prs_sram_offset_set(&pe, MVPP2_PRS_SRAM_UDF_TYPE_L3,
				  MVPP2_ETH_TYPE_LEN,
				  MVPP2_PRS_SRAM_OP_SEL_UDF_ADD);

	/* Update shadow table and hw entry */
	mvpp2_prs_shadow_set(pp2, pe.index, MVPP2_PRS_LU_PPPOE);
	mvpp2_prs_hw_write(pp2, &pe);

	/* Non-IP over PPPoE */
	tid = mvpp2_prs_tcam_first_free(pp2, MVPP2_PE_FIRST_FREE_TID,
					MVPP2_PE_LAST_FREE_TID);
	if (tid == -ERANGE) {
		dprintk("%s: No free TCAM entry (non-IP)\n", __func__);
		return tid;
	}

	memset(&pe, 0, sizeof(struct mvpp2_prs_entry));
	mvpp2_prs_tcam_lu_set(&pe, MVPP2_PRS_LU_PPPOE);
	pe.index = tid;

	mvpp2_prs_sram_ri_update(&pe, MVPP2_PRS_RI_L3_UN,
				 MVPP2_PRS_RI_L3_PROTO_MASK);

	/* Finished: go to flowid generation */
	mvpp2_prs_sram_next_lu_set(&pe, MVPP2_PRS_LU_FLOWS);
	mvpp2_prs_sram_bits_set(&pe, MVPP2_PRS_SRAM_LU_GEN_BIT, 1);
	/* Set L3 offset even if it's unknown L3 */
	mvpp2_prs_sram_offset_set(&pe, MVPP2_PRS_SRAM_UDF_TYPE_L3,
				  MVPP2_ETH_TYPE_LEN,
				  MVPP2_PRS_SRAM_OP_SEL_UDF_ADD);

	/* Update shadow table and hw entry */
	mvpp2_prs_shadow_set(pp2, pe.index, MVPP2_PRS_LU_PPPOE);
	mvpp2_prs_hw_write(pp2, &pe);

	return 0;
}

/* Initialize entries for IPv4 */
static int mvpp2_prs_ip4_init(struct mvpp2 *pp2)
{
	struct mvpp2_prs_entry pe;
	int err;

	/* Set entries for TCP, UDP and IGMP over IPv4 */
	err = mvpp2_prs_ip4_proto(pp2, IPPROTO_TCP, MVPP2_PRS_RI_L4_TCP,
				  MVPP2_PRS_RI_L4_PROTO_MASK);
	if (err)
		return err;

	err = mvpp2_prs_ip4_proto(pp2, IPPROTO_UDP, MVPP2_PRS_RI_L4_UDP,
				  MVPP2_PRS_RI_L4_PROTO_MASK);
	if (err)
		return err;

	err = mvpp2_prs_ip4_proto(pp2, IPPROTO_IGMP,
				  MVPP2_PRS_RI_CPU_CODE_RX_SPEC |
				  MVPP2_PRS_RI_UDF3_RX_SPECIAL,
				  MVPP2_PRS_RI_CPU_CODE_MASK |
				  MVPP2_PRS_RI_UDF3_MASK);
	if (err)
		return err;

	/* IPv4 Broadcast */
	err = mvpp2_prs_ip4_cast(pp2, MVPP2_PRS_L3_BROAD_CAST);
	if (err)
		return err;

	/* IPv4 Multicast */
	err = mvpp2_prs_ip4_cast(pp2, MVPP2_PRS_L3_MULTI_CAST);
	if (err)
		return err;

	/* Default IPv4 entry for unknown protocols */
	memset(&pe, 0, sizeof(struct mvpp2_prs_entry));
	mvpp2_prs_tcam_lu_set(&pe, MVPP2_PRS_LU_IP4);
	pe.index = MVPP2_PE_IP4_PROTO_UN;

	/* Set next lu to IPv4 */
	mvpp2_prs_sram_next_lu_set(&pe, MVPP2_PRS_LU_IP4);
	mvpp2_prs_sram_shift_set(&pe, 12, MVPP2_PRS_SRAM_OP_SEL_SHIFT_ADD);
	/* Set L4 offset */
	mvpp2_prs_sram_offset_set(&pe, MVPP2_PRS_SRAM_UDF_TYPE_L4,
				  sizeof(struct iphdr) - 4,
				  MVPP2_PRS_SRAM_OP_SEL_UDF_ADD);
	mvpp2_prs_sram_ai_update(&pe, MVPP2_PRS_IPV4_DIP_AI_BIT,
				 MVPP2_PRS_IPV4_DIP_AI_BIT);
	mvpp2_prs_sram_ri_update(&pe, MVPP2_PRS_RI_L4_OTHER,
				 MVPP2_PRS_RI_L4_PROTO_MASK);

	mvpp2_prs_tcam_ai_update(&pe, 0, MVPP2_PRS_IPV4_DIP_AI_BIT);
	/* Unmask all ports */
	mvpp2_prs_tcam_port_map_set(&pe, MVPP2_PRS_PORT_MASK);

	/* Update shadow table and hw entry */
	mvpp2_prs_shadow_set(pp2, pe.index, MVPP2_PRS_LU_IP4);
	mvpp2_prs_hw_write(pp2, &pe);

	/* Default IPv4 entry for unicast address */
	memset(&pe, 0, sizeof(struct mvpp2_prs_entry));
	mvpp2_prs_tcam_lu_set(&pe, MVPP2_PRS_LU_IP4);
	pe.index = MVPP2_PE_IP4_ADDR_UN;

	/* Finished: go to flowid generation */
	mvpp2_prs_sram_next_lu_set(&pe, MVPP2_PRS_LU_FLOWS);
	mvpp2_prs_sram_bits_set(&pe, MVPP2_PRS_SRAM_LU_GEN_BIT, 1);
	mvpp2_prs_sram_ri_update(&pe, MVPP2_PRS_RI_L3_UCAST,
				 MVPP2_PRS_RI_L3_ADDR_MASK);

	mvpp2_prs_tcam_ai_update(&pe, MVPP2_PRS_IPV4_DIP_AI_BIT,
				 MVPP2_PRS_IPV4_DIP_AI_BIT);
	/* Unmask all ports */
	mvpp2_prs_tcam_port_map_set(&pe, MVPP2_PRS_PORT_MASK);

	/* Update shadow table and hw entry */
	mvpp2_prs_shadow_set(pp2, pe.index, MVPP2_PRS_LU_IP4);
	mvpp2_prs_hw_write(pp2, &pe);

	return 0;
}

/* Initialize entries for IPv6 */
static int mvpp2_prs_ip6_init(struct mvpp2 *pp2)
{
	struct mvpp2_prs_entry pe;
	int tid;
	int err;

	/* Set entries for TCP, UDP and ICMP over IPv6 */
	err = mvpp2_prs_ip6_proto(pp2, IPPROTO_TCP,
				  MVPP2_PRS_RI_L4_TCP,
				  MVPP2_PRS_RI_L4_PROTO_MASK);
	if (err)
		return err;

	err = mvpp2_prs_ip6_proto(pp2, IPPROTO_UDP,
				  MVPP2_PRS_RI_L4_UDP,
				  MVPP2_PRS_RI_L4_PROTO_MASK);
	if (err)
		return err;

	err = mvpp2_prs_ip6_proto(pp2, IPPROTO_ICMPV6,
				  MVPP2_PRS_RI_CPU_CODE_RX_SPEC |
				  MVPP2_PRS_RI_UDF3_RX_SPECIAL,
				  MVPP2_PRS_RI_CPU_CODE_MASK |
				  MVPP2_PRS_RI_UDF3_MASK);
	if (err)
		return err;

	/* IPv4 is the last header. This is similar case as 6-TCP or 17-UDP */
	/* Result Info: UDF7=1, DS lite */
	err = mvpp2_prs_ip6_proto(pp2, IPPROTO_IPIP, MVPP2_PRS_RI_UDF7_IP6_LITE,
				  MVPP2_PRS_RI_UDF7_MASK);
	if (err)
		return err;

	/* IPv6 multicast */
	err = mvpp2_prs_ip6_cast(pp2, MVPP2_PRS_L3_MULTI_CAST);
	if (err)
		return err;

	/* Entry for checking hop limit */
	tid = mvpp2_prs_tcam_first_free(pp2, MVPP2_PE_FIRST_FREE_TID,
					MVPP2_PE_LAST_FREE_TID);
	if (tid == -ERANGE) {
		dprintk("%s: No free TCAM entry\n", __func__);
		return tid;
	}

	memset(&pe, 0, sizeof(struct mvpp2_prs_entry));
	mvpp2_prs_tcam_lu_set(&pe, MVPP2_PRS_LU_IP6);
	pe.index = tid;

	/* Finished: go to flowid generation */
	mvpp2_prs_sram_next_lu_set(&pe, MVPP2_PRS_LU_FLOWS);
	mvpp2_prs_sram_bits_set(&pe, MVPP2_PRS_SRAM_LU_GEN_BIT, 1);
	mvpp2_prs_sram_ri_update(&pe, MVPP2_PRS_RI_L3_UN |
				 MVPP2_PRS_RI_DROP_MASK,
				 MVPP2_PRS_RI_L3_PROTO_MASK |
				 MVPP2_PRS_RI_DROP_MASK);

	mvpp2_prs_tcam_data_byte_set(&pe, 1, 0x00, MVPP2_PRS_IPV6_HOP_MASK);
	mvpp2_prs_tcam_ai_update(&pe, MVPP2_PRS_IPV6_NO_EXT_AI_BIT,
				 MVPP2_PRS_IPV6_NO_EXT_AI_BIT);

	/* Update shadow table and hw entry */
	mvpp2_prs_shadow_set(pp2, pe.index, MVPP2_PRS_LU_IP4);
	mvpp2_prs_hw_write(pp2, &pe);

	/* Default IPv6 entry for unknown protocols */
	memset(&pe, 0, sizeof(struct mvpp2_prs_entry));
	mvpp2_prs_tcam_lu_set(&pe, MVPP2_PRS_LU_IP6);
	pe.index = MVPP2_PE_IP6_PROTO_UN;

	/* Finished: go to flowid generation */
	mvpp2_prs_sram_next_lu_set(&pe, MVPP2_PRS_LU_FLOWS);
	mvpp2_prs_sram_bits_set(&pe, MVPP2_PRS_SRAM_LU_GEN_BIT, 1);
	mvpp2_prs_sram_ri_update(&pe, MVPP2_PRS_RI_L4_OTHER,
				 MVPP2_PRS_RI_L4_PROTO_MASK);
	/* Set L4 offset relatively to our current place */
	mvpp2_prs_sram_offset_set(&pe, MVPP2_PRS_SRAM_UDF_TYPE_L4,
				  sizeof(struct ipv6hdr) - 4,
				  MVPP2_PRS_SRAM_OP_SEL_UDF_ADD);

	mvpp2_prs_tcam_ai_update(&pe, MVPP2_PRS_IPV6_NO_EXT_AI_BIT,
				 MVPP2_PRS_IPV6_NO_EXT_AI_BIT);
	/* Unmask all ports */
	mvpp2_prs_tcam_port_map_set(&pe, MVPP2_PRS_PORT_MASK);

	/* Update shadow table and hw entry */
	mvpp2_prs_shadow_set(pp2, pe.index, MVPP2_PRS_LU_IP4);
	mvpp2_prs_hw_write(pp2, &pe);

	/* Default IPv6 entry for unknown ext protocols */
	memset(&pe, 0, sizeof(struct mvpp2_prs_entry));
	mvpp2_prs_tcam_lu_set(&pe, MVPP2_PRS_LU_IP6);
	pe.index = MVPP2_PE_IP6_EXT_PROTO_UN;

	/* Finished: go to flowid generation */
	mvpp2_prs_sram_next_lu_set(&pe, MVPP2_PRS_LU_FLOWS);
	mvpp2_prs_sram_bits_set(&pe, MVPP2_PRS_SRAM_LU_GEN_BIT, 1);
	mvpp2_prs_sram_ri_update(&pe, MVPP2_PRS_RI_L4_OTHER,
				 MVPP2_PRS_RI_L4_PROTO_MASK);

	mvpp2_prs_tcam_ai_update(&pe, MVPP2_PRS_IPV6_EXT_AI_BIT,
				 MVPP2_PRS_IPV6_EXT_AI_BIT);
	/* Unmask all ports */
	mvpp2_prs_tcam_port_map_set(&pe, MVPP2_PRS_PORT_MASK);

	/* Update shadow table and hw entry */
	mvpp2_prs_shadow_set(pp2, pe.index, MVPP2_PRS_LU_IP4);
	mvpp2_prs_hw_write(pp2, &pe);

	/* Default IPv6 entry for unicast address */
	memset(&pe, 0, sizeof(struct mvpp2_prs_entry));
	mvpp2_prs_tcam_lu_set(&pe, MVPP2_PRS_LU_IP6);
	pe.index = MVPP2_PE_IP6_ADDR_UN;

	/* Finished: go to IPv6 again */
	mvpp2_prs_sram_next_lu_set(&pe, MVPP2_PRS_LU_IP6);
	mvpp2_prs_sram_ri_update(&pe, MVPP2_PRS_RI_L3_UCAST,
				 MVPP2_PRS_RI_L3_ADDR_MASK);
	mvpp2_prs_sram_ai_update(&pe, MVPP2_PRS_IPV6_NO_EXT_AI_BIT,
				 MVPP2_PRS_IPV6_NO_EXT_AI_BIT);
	/* Shift back to IPV6 NH */
	mvpp2_prs_sram_shift_set(&pe, -18, MVPP2_PRS_SRAM_OP_SEL_SHIFT_ADD);

	mvpp2_prs_tcam_ai_update(&pe, 0, MVPP2_PRS_IPV6_NO_EXT_AI_BIT);
	/* Unmask all ports */
	mvpp2_prs_tcam_port_map_set(&pe, MVPP2_PRS_PORT_MASK);

	/* Update shadow table and hw entry */
	mvpp2_prs_shadow_set(pp2, pe.index, MVPP2_PRS_LU_IP6);
	mvpp2_prs_hw_write(pp2, &pe);

	return 0;
}

/* Parser default initialization */
static int mvpp2_prs_default_init(struct platform_device *pdev,
				  struct mvpp2 *pp2)
{
	int port;
	int err;
	int index;
	int i;

	/* Enable tcam table */
	mvpp2_write(pp2, MVPP2_PRS_TCAM_CTRL_REG, MVPP2_PRS_TCAM_EN_MASK);

	/* Clear all tcam and sram entries */
	for (index = 0; index < MVPP2_PRS_TCAM_SRAM_SIZE; index++) {
		mvpp2_write(pp2, MVPP2_PRS_TCAM_IDX_REG, index);
		for (i = 0; i < MVPP2_PRS_TCAM_WORDS; i++)
			mvpp2_write(pp2, MVPP2_PRS_TCAM_DATA_REG(i), 0);

		mvpp2_write(pp2, MVPP2_PRS_SRAM_IDX_REG, index);
		for (i = 0; i < MVPP2_PRS_SRAM_WORDS; i++)
			mvpp2_write(pp2, MVPP2_PRS_SRAM_DATA_REG(i), 0);
	}

	/* Invalidate all tcam entries */
	for (index = 0; index < MVPP2_PRS_TCAM_SRAM_SIZE; index++)
		mvpp2_prs_hw_inv(pp2, index);

	pp2->prs_shadow = devm_kzalloc(&pdev->dev, MVPP2_PRS_TCAM_SRAM_SIZE *
				       sizeof(struct mvpp2_prs_shadow),
				       GFP_KERNEL);
	if (!pp2->prs_shadow)
		return -ENOMEM;

	/* Always start from lookup = 0 */
	for (port = 0; port < MVPP2_MAX_PORTS; port++)
		mvpp2_prs_hw_port_init(pp2, port, MVPP2_PRS_LU_MH,
				       MVPP2_PRS_PORT_LU_MAX, 0);

	mvpp2_prs_def_flow_init(pp2);

	mvpp2_prs_mh_init(pp2);

	mvpp2_prs_mac_init(pp2);

	mvpp2_prs_dsa_init(pp2);

	err = mvpp2_prs_etype_init(pp2);
	if (err)
		return err;

	err = mvpp2_prs_vlan_init(pdev, pp2);
	if (err)
		return err;

	err = mvpp2_prs_pppoe_init(pp2);
	if (err)
		return err;

	err = mvpp2_prs_ip6_init(pp2);
	if (err)
		return err;

	err = mvpp2_prs_ip4_init(pp2);
	if (err)
		return err;

	return 0;
}

/* Compare MAC DA with tcam entry data */
static bool mvpp2_prs_mac_range_equals(struct mvpp2_prs_entry *pe,
				       const u8 *da, unsigned char *mask)
{
	int index;
	unsigned char tcam_byte;
	unsigned char tcam_mask;

	for (index = 0; index < ETH_ALEN; index++) {
		mvpp2_prs_tcam_data_byte_get(pe, index, &tcam_byte, &tcam_mask);
		if (tcam_mask != mask[index])
			return false;

		if ((tcam_mask & tcam_byte) != (da[index] & mask[index]))
			return false;
	}

	return true;
}

/* Find tcam entry with matched pair <MAC DA, port> */
static struct mvpp2_prs_entry *mvpp2_prs_mac_da_range_find(struct mvpp2 *pp2,
							   int pmap,
							   const u8 *da,
							   unsigned char *mask,
							   int udf_type)
{
	struct mvpp2_prs_entry *pe;
	unsigned int entry_pmap;
	int tid;

	pe = kzalloc(sizeof(struct mvpp2_prs_entry), GFP_KERNEL);
	if (!pe)
		return NULL;
	mvpp2_prs_tcam_lu_set(pe, MVPP2_PRS_LU_MAC);

	dprintk("%s: pmap = 0x%x, mac = %02x:%02x:%02x:%02x:%02x:%02x\n",
		__func__, pmap, da[0], da[1], da[2], da[3], da[4], da[5]);

	/* Go through the all entires with MVPP2_PRS_LU_MAC */
	for (tid = MVPP2_PE_FIRST_FREE_TID;
	     tid <= MVPP2_PE_LAST_FREE_TID; tid++) {
		if (!pp2->prs_shadow[tid].valid ||
		    (pp2->prs_shadow[tid].lu != MVPP2_PRS_LU_MAC) ||
		    (pp2->prs_shadow[tid].udf != udf_type))
			continue;

		pe->index = tid;
		mvpp2_prs_hw_read(pp2, pe);
		mvpp2_prs_tcam_port_map_get(pe, &entry_pmap);

		if (mvpp2_prs_mac_range_equals(pe, da, mask) &&
		    (entry_pmap == pmap)) {
			dprintk("%s: entry found\n", __func__);
			return pe;
		}
	}
	kfree(pe);

	return NULL;
}

/* Update parser's mac da entry */
static int mvpp2_prs_mac_da_accept(struct mvpp2 *pp2, int port,
				   const u8 *da, bool add)
{
	struct mvpp2_prs_entry *pe = NULL;
	unsigned int pmap;
	unsigned int len;
	unsigned int ri;
	unsigned char mask[ETH_ALEN] = { 0xff, 0xff, 0xff, 0xff, 0xff, 0xff };
	int tid;

	/* Scan TCAM and see if entry with this <MAC DA, port> already exist */
	pe = mvpp2_prs_mac_da_range_find(pp2, (1 << port), da, mask,
					 MVPP2_PRS_UDF_MAC_DEF);

	/* No such entry */
	if (!pe) {
		if (!add) {
			dprintk("%s: The entry already doesn't exist\n",
				__func__);
			return 0;
		}

		/* Create new TCAM entry */
		/* Find first range mac entry*/
		for (tid = MVPP2_PE_FIRST_FREE_TID;
		     tid <= MVPP2_PE_LAST_FREE_TID; tid++)
			if (pp2->prs_shadow[tid].valid &&
			    (pp2->prs_shadow[tid].lu == MVPP2_PRS_LU_MAC) &&
			    (pp2->prs_shadow[tid].udf ==
						       MVPP2_PRS_UDF_MAC_RANGE))
				break;

		/* Go through the all entries from first to last */
		tid = mvpp2_prs_tcam_first_free(pp2, MVPP2_PE_FIRST_FREE_TID,
						tid - 1);
		if (tid == -ERANGE) {
			dprintk("%s: No free TCAM entry\n", __func__);
			return tid;
		}

		pe = kzalloc(sizeof(struct mvpp2_prs_entry), GFP_KERNEL);
		if (!pe)
			return -1;
		mvpp2_prs_tcam_lu_set(pe, MVPP2_PRS_LU_MAC);
		pe->index = tid;

		/* Mask all ports */
		mvpp2_prs_tcam_port_map_set(pe, 0);
	}

	/* Update port mask */
	mvpp2_prs_tcam_port_set(pe, port, add);

	/* Invalidate the entry if no ports are left enabled */
	mvpp2_prs_tcam_port_map_get(pe, &pmap);
	if (pmap == 0) {
		if (add) {
			kfree(pe);
			dprintk("%s: Wrong port map value\n", __func__);
			return -1;
		}
		mvpp2_prs_hw_inv(pp2, pe->index);
		pp2->prs_shadow[pe->index].valid = false;
		kfree(pe);
		return 0;
	}

	/* Continue - set next lookup */
	mvpp2_prs_sram_next_lu_set(pe, MVPP2_PRS_LU_DSA);

	/* Set match on DA */
	len = ETH_ALEN;
	while (len--)
		mvpp2_prs_tcam_data_byte_set(pe, len, da[len], 0xff);

	/* Set result info bits */
	if (is_broadcast_ether_addr(da)) {
		ri = MVPP2_PRS_RI_L2_BCAST;
		dprintk("%s: bcast-port-%d\n", __func__, port);
	} else if (is_multicast_ether_addr(da)) {
		ri = MVPP2_PRS_RI_L2_MCAST;
		dprintk("%s: mcast-port-%d\n", __func__, port);
	} else {
		ri = MVPP2_PRS_RI_L2_UCAST | MVPP2_PRS_RI_MAC_ME_MASK;
		dprintk("%s: ucast-port-%d\n", __func__, port);
	}
	mvpp2_prs_sram_ri_update(pe, ri, MVPP2_PRS_RI_L2_CAST_MASK |
				 MVPP2_PRS_RI_MAC_ME_MASK);
	mvpp2_prs_shadow_ri_set(pp2, pe->index, ri, MVPP2_PRS_RI_L2_CAST_MASK |
				MVPP2_PRS_RI_MAC_ME_MASK);

	/* Shift to ethertype */
	mvpp2_prs_sram_shift_set(pe, 2 * ETH_ALEN,
				 MVPP2_PRS_SRAM_OP_SEL_SHIFT_ADD);

	/* Update shadow table and hw entry */
	pp2->prs_shadow[pe->index].udf = MVPP2_PRS_UDF_MAC_DEF;
	mvpp2_prs_shadow_set(pp2, pe->index, MVPP2_PRS_LU_MAC);
	mvpp2_prs_hw_write(pp2, pe);

	kfree(pe);

	return 0;
}

/* Delete all port's multicast simple (not range) entries */
static void mvpp2_prs_mcast_del_all(struct mvpp2 *pp2, int port)
{
	struct mvpp2_prs_entry pe;
	int index;
	int tid;
	unsigned char da[ETH_ALEN], da_mask[ETH_ALEN];

	for (tid = MVPP2_PE_FIRST_FREE_TID;
	     tid <= MVPP2_PE_LAST_FREE_TID; tid++) {
		if (!pp2->prs_shadow[tid].valid ||
		    (pp2->prs_shadow[tid].lu != MVPP2_PRS_LU_MAC) ||
		    (pp2->prs_shadow[tid].udf != MVPP2_PRS_UDF_MAC_DEF))
			continue;

		/* Only simple mac entries */
		pe.index = tid;
		mvpp2_prs_hw_read(pp2, &pe);

		/* Read mac addr from entry */
		for (index = 0; index < ETH_ALEN; index++)
			mvpp2_prs_tcam_data_byte_get(&pe, index, &da[index],
						     &da_mask[index]);

		if (is_broadcast_ether_addr(da))
			continue;

		if (is_multicast_ether_addr(da))
			/* Delete entry */
			mvpp2_prs_mac_da_accept(pp2, port, da, false);
	}
}

static int mvpp2_prs_tag_mode_set(struct mvpp2 *pp2, int port, int type)
{
	switch (type) {

	case MVPP2_TAG_TYPE_EDSA:
		/* Add port to EDSA entries */
		mvpp2_prs_dsa_tag_set(pp2, port, true,
				      MVPP2_PRS_TAGGED, MVPP2_PRS_EDSA);
		mvpp2_prs_dsa_tag_set(pp2, port, true,
				      MVPP2_PRS_UNTAGGED, MVPP2_PRS_EDSA);
		/* Remove port from DSA entries */
		mvpp2_prs_dsa_tag_set(pp2, port, false,
				      MVPP2_PRS_TAGGED, MVPP2_PRS_DSA);
		mvpp2_prs_dsa_tag_set(pp2, port, false,
				      MVPP2_PRS_UNTAGGED, MVPP2_PRS_DSA);
		break;

	case MVPP2_TAG_TYPE_DSA:
		/* Add port to DSA entries */
		mvpp2_prs_dsa_tag_set(pp2, port, true,
				      MVPP2_PRS_TAGGED, MVPP2_PRS_DSA);
		mvpp2_prs_dsa_tag_set(pp2, port, true,
				      MVPP2_PRS_UNTAGGED, MVPP2_PRS_DSA);
		/* Remove port from EDSA entries */
		mvpp2_prs_dsa_tag_set(pp2, port, false,
				      MVPP2_PRS_TAGGED, MVPP2_PRS_EDSA);
		mvpp2_prs_dsa_tag_set(pp2, port, false,
				      MVPP2_PRS_UNTAGGED, MVPP2_PRS_EDSA);
		break;

	case MVPP2_TAG_TYPE_MH:
	case MVPP2_TAG_TYPE_NONE:
		/* Remove port form EDSA and DSA entries */
		mvpp2_prs_dsa_tag_set(pp2, port, false,
				      MVPP2_PRS_TAGGED, MVPP2_PRS_DSA);
		mvpp2_prs_dsa_tag_set(pp2, port, false,
				      MVPP2_PRS_UNTAGGED, MVPP2_PRS_DSA);
		mvpp2_prs_dsa_tag_set(pp2, port, false,
				      MVPP2_PRS_TAGGED, MVPP2_PRS_EDSA);
		mvpp2_prs_dsa_tag_set(pp2, port, false,
				      MVPP2_PRS_UNTAGGED, MVPP2_PRS_EDSA);
		break;

	default:
		if ((type < 0) || (type > MVPP2_TAG_TYPE_EDSA))
			return -EINVAL;
	}

	return 0;
}

/* Set prs flow for the port */
static int mvpp2_prs_def_flow(struct mvpp2_port *pp)
{
	struct mvpp2_prs_entry *pe;
	int tid;

	pe = mvpp2_prs_flow_find(pp->pp2, pp->id);

	/* Such entry not exist */
	if (!pe) {
		/* Go through the all entires from last to first */
		tid = mvpp2_prs_tcam_first_free(pp->pp2,
					       MVPP2_PE_LAST_FREE_TID,
					       MVPP2_PE_FIRST_FREE_TID);
		if (tid == -ERANGE) {
			dprintk("%s: No free TCAM entry\n", __func__);
			return tid;
		}

		pe = kzalloc(sizeof(struct mvpp2_prs_entry), GFP_KERNEL);
		if (!pe)
			return -ENOMEM;

		mvpp2_prs_tcam_lu_set(pe, MVPP2_PRS_LU_FLOWS);
		pe->index = tid;

		/* Set flow ID*/
		mvpp2_prs_sram_ai_update(pe, pp->id, MVPP2_PRS_FLOW_ID_MASK);
		mvpp2_prs_sram_bits_set(pe, MVPP2_PRS_SRAM_LU_DONE_BIT, 1);

		/* Update shadow table */
		mvpp2_prs_shadow_set(pp->pp2, pe->index, MVPP2_PRS_LU_FLOWS);

	}

	mvpp2_prs_tcam_port_map_set(pe, (1 << pp->id));
	mvpp2_prs_hw_write(pp->pp2, pe);
	kfree(pe);

	return 0;
}

/* Classifier configuration routines */

/* Update classification flow table registers */
static void mvpp2_cls_hw_flow_write(struct mvpp2 *pp2,
				    struct mvpp2_cls_flow_entry *fe)
{
	/* Write to index reg - indirect access */
	mvpp2_write(pp2, MVPP2_CLS_FLOW_INDEX_REG, fe->index);

	mvpp2_write(pp2, MVPP2_CLS_FLOW_TBL0_REG, fe->data[0]);
	mvpp2_write(pp2, MVPP2_CLS_FLOW_TBL1_REG, fe->data[1]);
	mvpp2_write(pp2, MVPP2_CLS_FLOW_TBL2_REG, fe->data[2]);
}

/* Update classification lookup table register */
static void mvpp2_cls_hw_lkp_write(struct mvpp2 *pp2,
				   struct mvpp2_cls_lkp_entry *le)
{
	u32 reg_val = 0;

	/* Write to index reg - indirect access */
	reg_val = (le->way << MVPP2_CLS_LKP_INDEX_WAY_OFFS) | le->lkpid;
	mvpp2_write(pp2, MVPP2_CLS_LKP_INDEX_REG, reg_val);

	mvpp2_write(pp2, MVPP2_CLS_LKP_TBL_REG, le->data);
}

/* Classifier default initialization */
static void mvpp2_cls_default_init(struct mvpp2 *pp2)
{
	struct mvpp2_cls_lkp_entry le;
	struct mvpp2_cls_flow_entry fe;
	int index;

	/* Enable Classifier */
	mvpp2_write(pp2, MVPP2_CLS_MODE_REG,
		    MVPP2_CLS_MODE_ACTIVE_MASK);

	/* Clear cls flow table */
	fe.data[0] = 0;
	fe.data[1] = 0;
	fe.data[2] = 0;
	for (index = 0; index < MVPP2_CLS_FLOWS_TBL_SIZE; index++) {
		fe.index = index;
		mvpp2_cls_hw_flow_write(pp2, &fe);
	}

	/* Clear cls lookup table */
	le.data = 0;
	for (index = 0; index < MVPP2_CLS_LKP_TBL_SIZE; index++) {
		le.lkpid = index;
		le.way = 0;
		mvpp2_cls_hw_lkp_write(pp2, &le);
		le.way = 1;
		mvpp2_cls_hw_lkp_write(pp2, &le);
	}
}

/* Port's classifier configuration */
static void mvpp2_cls_hw_port_def_config(struct mvpp2_port *pp, int way,
					 int lkpid)
{
	struct mvpp2_cls_lkp_entry le;
	u32 reg_val;

	/* Set way for the port */
	reg_val = mvpp2_read(pp->pp2, MVPP2_CLS_PORT_WAY_REG);
	if (way)
		reg_val |= MVPP2_CLS_PORT_WAY_MASK(pp->id);
	else
		reg_val &= ~MVPP2_CLS_PORT_WAY_MASK(pp->id);
	mvpp2_write(pp->pp2, MVPP2_CLS_PORT_WAY_REG, reg_val);

	/* The entry to be accessed in lookupid decoding table
	 * is acording to way and lkpid
	 */
	le.way = way;
	le.lkpid = lkpid;
	le.data = 0;

	/* Set initial CPU queue for receiving packets */
	le.data &= ~MVPP2_CLS_LKP_TBL_RXQ_MASK;
	le.data |= pp->first_rxq;

	/* Disable classification engines */
	le.data &= ~MVPP2_CLS_LKP_TBL_LOOKUP_EN_MASK;

	/* Update lookupid table entry */
	mvpp2_cls_hw_lkp_write(pp->pp2, &le);
}

/* Set CPU queue number for oversize packets */
static void mvpp2_cls_hw_oversize_rxq_set(struct mvpp2_port *pp)
{
	u32 reg_val;

	mvpp2_write(pp->pp2, MVPP2_CLS_OVERSIZE_RXQ_LOW_REG(pp->id),
		    pp->first_rxq & MVPP2_CLS_OVERSIZE_RXQ_LOW_MASK);

	mvpp2_write(pp->pp2, MVPP2_CLS_SWFWD_P2HQ_REG(pp->id),
		    (pp->first_rxq >> MVPP2_CLS_OVERSIZE_RXQ_LOW_BITS));

	reg_val = mvpp2_read(pp->pp2, MVPP2_CLS_SWFWD_PCTRL_REG);
	reg_val |= MVPP2_CLS_SWFWD_PCTRL_MASK(pp->id);
	mvpp2_write(pp->pp2, MVPP2_CLS_SWFWD_PCTRL_REG, reg_val);
}

/* Buffer Manager configuration routines */

/* Create pool */
static int mvpp2_bm_pool_create(struct platform_device *pdev,
				struct mvpp2 *pp2,
				struct mvpp2_bm_pool *bm_pool, int size)
{
	int size_bytes;
	u32 reg_val;

	size_bytes = sizeof(u32) * size;
	bm_pool->virt_addr = dma_alloc_coherent(&pdev->dev, size_bytes,
						&bm_pool->phys_addr,
						GFP_KERNEL);
	if (!bm_pool->virt_addr)
		return -ENOMEM;

	if (!IS_ALIGNED((u32)bm_pool->virt_addr, MVPP2_BM_POOL_PTR_ALIGN)) {
		dma_free_coherent(&pdev->dev, size_bytes, bm_pool->virt_addr,
				  bm_pool->phys_addr);
		dev_err(&pdev->dev, "BM pool %d is not %d bytes aligned\n",
			bm_pool->id, MVPP2_BM_POOL_PTR_ALIGN);
		return -ENOMEM;
	}

	mvpp2_write(pp2, MVPP2_BM_POOL_BASE_REG(bm_pool->id),
		    bm_pool->phys_addr);
	mvpp2_write(pp2, MVPP2_BM_POOL_SIZE_REG(bm_pool->id), size);

	reg_val = mvpp2_read(pp2, MVPP2_BM_POOL_CTRL_REG(bm_pool->id));
	reg_val |= MVPP2_BM_START_MASK;
	mvpp2_write(pp2, MVPP2_BM_POOL_CTRL_REG(bm_pool->id), reg_val);

	bm_pool->type = MVPP2_BM_FREE;
	bm_pool->size = size;
	bm_pool->pkt_size = 0;
	bm_pool->buf_num = 0;
	atomic_set(&bm_pool->in_use, 0);
	spin_lock_init(&bm_pool->lock);

	return 0;
}

/* Set pool buffer size */
static void mvpp2_bm_pool_bufsize_set(struct mvpp2 *pp2,
				      struct mvpp2_bm_pool *bm_pool,
				      int buf_size)
{
	u32 reg_val;

	bm_pool->buf_size = buf_size;

	reg_val = ALIGN(buf_size, 1 << MVPP2_POOL_BUF_SIZE_OFFSET);
	mvpp2_write(pp2, MVPP2_POOL_BUF_SIZE_REG(bm_pool->id), reg_val);
}

/* Free "num" buffers from the pool */
static int mvpp2_bm_bufs_free(struct mvpp2 *pp2,
			      struct mvpp2_bm_pool *bm_pool, int num)
{
	int i;

	if (num >= bm_pool->buf_num)
		/* Free all buffers from the pool */
		num = bm_pool->buf_num;

	for (i = 0; i < num; i++) {
		u32 vaddr;

		/* Get buffer virtual adress (indirect access) */
		mvpp2_read(pp2, MVPP2_BM_PHY_ALLOC_REG(bm_pool->id));
		vaddr = mvpp2_read(pp2, MVPP2_BM_VIRT_ALLOC_REG);
		if (!vaddr)
			break;
		dev_kfree_skb_any((struct sk_buff *)vaddr);
	}

	/* Update BM driver with number of buffers removed from pool */
	bm_pool->buf_num -= i;
	return i;
}

/* Cleanup pool */
static int mvpp2_bm_pool_destroy(struct platform_device *pdev,
				 struct mvpp2 *pp2,
				 struct mvpp2_bm_pool *bm_pool)
{
	int num;
	u32 reg_val;

	num = mvpp2_bm_bufs_free(pp2, bm_pool, bm_pool->buf_num);
	if (num != bm_pool->buf_num) {
		WARN(1, "cannot free all buffers in pool %d\n", bm_pool->id);
		return 0;
	}

	reg_val = mvpp2_read(pp2, MVPP2_BM_POOL_CTRL_REG(bm_pool->id));
	reg_val |= MVPP2_BM_STOP_MASK;
	mvpp2_write(pp2, MVPP2_BM_POOL_CTRL_REG(bm_pool->id), reg_val);

	dma_free_coherent(&pdev->dev, sizeof(u32) * bm_pool->size,
			  bm_pool->virt_addr,
			  bm_pool->phys_addr);
	return 0;
}

static int mvpp2_bm_pools_init(struct platform_device *pdev,
			       struct mvpp2 *pp2)
{
	int i, err, size;
	struct mvpp2_bm_pool *bm_pool;

	/* Create all pools with maximum size */
	size = MVPP2_BM_POOL_SIZE_MAX;
	for (i = 0; i < MVPP2_BM_POOLS_NUM; i++) {
		bm_pool = &pp2->bm_pools[i];
		bm_pool->id = i;
		err = mvpp2_bm_pool_create(pdev, pp2, bm_pool, size);
		if (err)
			goto err_unroll_pools;
		mvpp2_bm_pool_bufsize_set(pp2, bm_pool, 0);
	}
	return 0;

err_unroll_pools:
	dev_err(&pdev->dev, "failed to create BM pool %d, size %d\n", i, size);
	for (i = i - 1; i >= 0; i--)
		mvpp2_bm_pool_destroy(pdev, pp2, &pp2->bm_pools[i]);
	return err;
}

static int mvpp2_bm_init(struct platform_device *pdev, struct mvpp2 *pp2)
{
	int i, err;

	for (i = 0; i < MVPP2_BM_POOLS_NUM; i++) {
		/* Mask BM all interrupts */
		mvpp2_write(pp2, MVPP2_BM_INTR_MASK_REG(i), 0);
		/* Clear BM cause register */
		mvpp2_write(pp2, MVPP2_BM_INTR_CAUSE_REG(i), 0);
	}

	/* Allocate and initialize BM pools */
	pp2->bm_pools = devm_kzalloc(&pdev->dev, MVPP2_BM_POOLS_NUM *
				     sizeof(struct mvpp2_bm_pool), GFP_KERNEL);
	if (!pp2->bm_pools)
		return -ENOMEM;

	err = mvpp2_bm_pools_init(pdev, pp2);
	if (err < 0)
		return err;
	return 0;
}

/* Attach long pool to rxq */
static void mvpp2_rxq_long_pool_set(struct mvpp2_port *pp,
				    int lrxq, int long_pool)
{
	u32 reg_val;
	int prxq;

	/* Get queue physical ID */
	prxq = pp->rxqs[lrxq]->id;

	reg_val = mvpp2_read(pp->pp2, MVPP2_RXQ_CONFIG_REG(prxq));
	reg_val &= ~MVPP2_RXQ_POOL_LONG_MASK;
	reg_val |= ((long_pool << MVPP2_RXQ_POOL_LONG_OFFS) &
		    MVPP2_RXQ_POOL_LONG_MASK);

	mvpp2_write(pp->pp2, MVPP2_RXQ_CONFIG_REG(prxq), reg_val);
}

/* Attach short pool to rxq */
static void mvpp2_rxq_short_pool_set(struct mvpp2_port *pp,
				     int lrxq, int short_pool)
{
	u32 reg_val;
	int prxq;

	/* Get queue physical ID */
	prxq = pp->rxqs[lrxq]->id;

	reg_val = mvpp2_read(pp->pp2, MVPP2_RXQ_CONFIG_REG(prxq));
	reg_val &= ~MVPP2_RXQ_POOL_SHORT_MASK;
	reg_val |= ((short_pool << MVPP2_RXQ_POOL_SHORT_OFFS) &
		    MVPP2_RXQ_POOL_SHORT_MASK);

	mvpp2_write(pp->pp2, MVPP2_RXQ_CONFIG_REG(prxq), reg_val);
}

/* Allocate skb for BM pool */
static struct sk_buff *mvpp2_skb_alloc(struct mvpp2_port *pp,
				       struct mvpp2_bm_pool *bm_pool,
				       dma_addr_t *buf_phys_addr,
				       gfp_t gfp_mask)
{
	struct sk_buff *skb;
	dma_addr_t phys_addr;

	skb = __dev_alloc_skb(bm_pool->pkt_size, gfp_mask);
	if (!skb)
		return NULL;

	phys_addr = dma_map_single(pp->dev->dev.parent, skb->head,
				    MVPP2_RX_BUF_SIZE(bm_pool->pkt_size),
				    DMA_FROM_DEVICE);
	if (unlikely(dma_mapping_error(pp->dev->dev.parent, phys_addr))) {
		dev_kfree_skb_any(skb);
		return NULL;
	}
	*buf_phys_addr = phys_addr;

	return skb;
}

/* Set pool number in a BM cookie */
static inline u32 mvpp2_bm_cookie_pool_set(u32 cookie, int pool)
{
	u32 bm;

	bm = cookie & ~(0xFF << MVPP2_BM_COOKIE_POOL_OFFS);
	bm |= ((pool & 0xFF) << MVPP2_BM_COOKIE_POOL_OFFS);

	return bm;
}

/* Get pool number from a BM cookie */
static inline int mvpp2_bm_cookie_pool_get(u32 cookie)
{
	return (cookie >> MVPP2_BM_COOKIE_POOL_OFFS) & 0xFF;
}

/* Release buffer to BM */
static inline void mvpp2_bm_pool_put(struct mvpp2_port *pp, int pool,
				     u32 buf_phys_addr, u32 buf_virt_addr)
{
	mvpp2_write(pp->pp2, MVPP2_BM_VIRT_RLS_REG, buf_virt_addr);
	mvpp2_write(pp->pp2, MVPP2_BM_PHY_RLS_REG(pool), buf_phys_addr);
}

/* Release multicast buffer */
static void mvpp2_bm_pool_mc_put(struct mvpp2_port *pp, int pool,
				 u32 buf_phys_addr, u32 buf_virt_addr,
				 int mc_id, int is_force)
{
	u32 reg_val = 0;

	reg_val |= (mc_id & MVPP2_BM_MC_ID_MASK);
	if (is_force)
		reg_val |= MVPP2_BM_FORCE_RELEASE_MASK;

	mvpp2_write(pp->pp2, MVPP2_BM_MC_RLS_REG, reg_val);
	mvpp2_bm_pool_put(pp, pool,
			  buf_phys_addr | MVPP2_BM_PHY_RLS_MC_BUFF_MASK,
			  buf_virt_addr);
}

/* Refill BM pool */
static void mvpp2_pool_refill(struct mvpp2_port *pp, u32 bm,
			      u32 phys_addr, u32 cookie)
{
	int pool = mvpp2_bm_cookie_pool_get(bm);
	unsigned long flags;

	/* It's very odd to need irq_save/restore here,
	 * TODO: Check?
	 */
	local_irq_save(flags);

	mvpp2_bm_pool_put(pp, pool, phys_addr, cookie);

	local_irq_restore(flags);
}

/* Allocate buffers for the pool */
static int mvpp2_bm_bufs_add(struct mvpp2_port *pp,
			     struct mvpp2_bm_pool *bm_pool, int buf_num)
{
	struct sk_buff *skb;
	int i, buf_size, total_size;
	u32 bm;
	dma_addr_t phys_addr;

	buf_size = MVPP2_RX_BUF_SIZE(bm_pool->pkt_size);
	total_size = MVPP2_RX_TOTAL_SIZE(buf_size);

	if (buf_num < 0 ||
	   (buf_num + bm_pool->buf_num > bm_pool->size)) {
		netdev_err(pp->dev,
			   "cannot allocate %d buffers for pool %d\n",
			   buf_num, bm_pool->id);
		return 0;
	}

	bm = mvpp2_bm_cookie_pool_set(0, bm_pool->id);
	for (i = 0; i < buf_num; i++) {
		skb = mvpp2_skb_alloc(pp, bm_pool, &phys_addr, GFP_KERNEL);
		if (!skb)
			break;

		mvpp2_pool_refill(pp, bm, (u32)phys_addr, (u32)skb);
	}

	/* Update BM driver with number of buffers added to pool */
	bm_pool->buf_num += i;
	bm_pool->in_use_thresh = bm_pool->buf_num / 4;

	netdev_dbg(pp->dev,
		   "%s pool %d: pkt_size=%4d, buf_size=%4d, total_size=%4d\n",
		   bm_pool->type == MVPP2_BM_SWF_SHORT ? "short" : " long",
		   bm_pool->id, bm_pool->pkt_size, buf_size, total_size);

	netdev_dbg(pp->dev,
		   "%s pool %d: %d of %d buffers added\n",
		   bm_pool->type == MVPP2_BM_SWF_SHORT ? "short" : " long",
		   bm_pool->id, i, buf_num);
	return i;
}

/* Notify the driver that BM pool is being used as specific type and return the
 * pool pointer on success
 */
static struct mvpp2_bm_pool *mvpp2_bm_pool_use(struct mvpp2_port *pp, int pool,
					 enum mvpp2_bm_type type, int pkt_size)
{
	unsigned long flags = 0;
	struct mvpp2_bm_pool *new_pool = &pp->pp2->bm_pools[pool];
	int num;

	if (new_pool->type != MVPP2_BM_FREE && new_pool->type != type) {
		netdev_err(pp->dev, "mixing pool types is forbidden\n");
		return NULL;
	}

	spin_lock_irqsave(&new_pool->lock, flags);

	if (new_pool->type == MVPP2_BM_FREE)
		new_pool->type = type;

	/* Allocate buffers in case BM pool is used as long pool, but packet
	 * size doesn't match MTU or BM pool hasn't being used yet
	 */
	if (((type == MVPP2_BM_SWF_LONG) && (pkt_size > new_pool->pkt_size)) ||
	    (new_pool->pkt_size == 0)) {
		int pkts_num;

		/* Set default buffer number or free all the buffers in case
		 * the pool is not empty
		 */
		pkts_num = new_pool->buf_num;
		if (pkts_num == 0)
			pkts_num = type == MVPP2_BM_SWF_LONG ?
				   MVPP2_BM_LONG_BUF_NUM :
				   MVPP2_BM_SHORT_BUF_NUM;
		else
			mvpp2_bm_bufs_free(pp->pp2, new_pool, pkts_num);

		new_pool->pkt_size = pkt_size;

		/* Allocate buffers for this pool */
		num = mvpp2_bm_bufs_add(pp, new_pool, pkts_num);
		if (num != pkts_num) {
			WARN(1, "pool %d: %d of %d allocated\n",
			     new_pool->id, num, pkts_num);
			/* We need to undo the bufs_add() allocations */
			spin_unlock_irqrestore(&new_pool->lock, flags);
			return NULL;
		}
	}

	mvpp2_bm_pool_bufsize_set(pp->pp2, new_pool,
				  MVPP2_RX_BUF_SIZE(new_pool->pkt_size));

	spin_unlock_irqrestore(&new_pool->lock, flags);

	return new_pool;
}

/* Initialize pools for swf */
static int mvpp2_swf_bm_pool_init(struct mvpp2_port *pp)
{
	unsigned long flags = 0;
	int rxq;

	if (!pp->pool_long) {
		pp->pool_long =
			mvpp2_bm_pool_use(pp, MVPP2_BM_SWF_LONG_POOL(pp->id),
					  MVPP2_BM_SWF_LONG,
					  pp->pkt_size);
		if (!pp->pool_long)
			return -ENOMEM;

		spin_lock_irqsave(&pp->pool_long->lock, flags);
		pp->pool_long->port_map |= (1 << pp->id);
		spin_unlock_irqrestore(&pp->pool_long->lock, flags);

		for (rxq = 0; rxq < rxq_number; rxq++)
			mvpp2_rxq_long_pool_set(pp, rxq, pp->pool_long->id);
	}

	if (!pp->pool_short) {
		pp->pool_short =
			mvpp2_bm_pool_use(pp, MVPP2_BM_SWF_SHORT_POOL,
					  MVPP2_BM_SWF_SHORT,
					  MVPP2_BM_SHORT_PKT_SIZE);
		if (!pp->pool_short)
			return -ENOMEM;

		spin_lock_irqsave(&pp->pool_short->lock, flags);
		pp->pool_short->port_map |= (1 << pp->id);
		spin_unlock_irqrestore(&pp->pool_short->lock, flags);

		for (rxq = 0; rxq < rxq_number; rxq++)
			mvpp2_rxq_short_pool_set(pp, rxq, pp->pool_short->id);
	}

	return 0;
}

static int mvpp2_bm_update_mtu(struct net_device *dev, int mtu)
{
	struct mvpp2_port *pp = netdev_priv(dev);
	struct mvpp2_bm_pool *port_pool = pp->pool_long;
	int num, pkts_num = port_pool->buf_num;
	int pkt_size = MVPP2_RX_PKT_SIZE(mtu);

	if (mtu == dev->mtu)
		goto mtu_out;

	/* Update BM pool with new buffer size */
	num = mvpp2_bm_bufs_free(pp->pp2, port_pool, pkts_num);
	if (num != pkts_num) {
		WARN(1, "cannot free all buffers in pool %d\n", port_pool->id);
		return -EIO;
	}

	port_pool->pkt_size = pkt_size;
	num = mvpp2_bm_bufs_add(pp, port_pool, pkts_num);
	if (num != pkts_num) {
		WARN(1, "pool %d: %d of %d allocated\n",
		     port_pool->id, num, pkts_num);
		return -EIO;
	}

	mvpp2_bm_pool_bufsize_set(pp->pp2, port_pool,
				  MVPP2_RX_BUF_SIZE(port_pool->pkt_size));

mtu_out:
	dev->mtu = mtu;
	netdev_update_features(dev);
	return 0;
}

static inline void mvpp2_cpu_interrupts_enable(struct mvpp2_port *pp, int cpu)
{
	int cpu_mask = 1 << cpu;

	mvpp2_write(pp->pp2, MVPP2_ISR_ENABLE_REG(pp->id),
		    MVPP2_ISR_ENABLE_INTERRUPT(cpu_mask));
}

static inline void mvpp2_cpu_interrupts_disable(struct mvpp2_port *pp, int cpu)
{
	int cpu_mask = 1 << cpu;

	mvpp2_write(pp->pp2, MVPP2_ISR_ENABLE_REG(pp->id),
		    MVPP2_ISR_DISABLE_INTERRUPT(cpu_mask));
}

/* Mask the current CPU's Rx/Tx interrupts */
static void mvpp2_interrupts_mask(void *arg)
{
	struct mvpp2_port *pp = arg;

	mvpp2_write(pp->pp2, MVPP2_ISR_RX_TX_MASK_REG(pp->id), 0);
}

/* Unmask the current CPU's Rx/Tx interrupts */
static void mvpp2_interrupts_unmask(void *arg)
{
	struct mvpp2_port *pp = arg;

	mvpp2_write(pp->pp2, MVPP2_ISR_RX_TX_MASK_REG(pp->id),
		    (MVPP2_CAUSE_MISC_SUM_MASK |
		     MVPP2_CAUSE_TXQ_OCCUP_DESC_ALL_MASK |
		     MVPP2_CAUSE_RXQ_OCCUP_DESC_ALL_MASK));
}

/* Port configuration routines */

static void mvpp2_port_mii_set(struct mvpp2_port *pp)
{
	u32 reg, val = 0;

	if (pp->phy_interface == PHY_INTERFACE_MODE_SGMII)
		val = MVPP2_GMAC_PCS_ENABLE_MASK |
		      MVPP2_GMAC_INBAND_AN_MASK;
	else if (pp->phy_interface == PHY_INTERFACE_MODE_RGMII)
		val = MVPP2_GMAC_PORT_RGMII_MASK;

	reg = readl(pp->base + MVPP2_GMAC_CTRL_2_REG);
	writel(reg | val, pp->base + MVPP2_GMAC_CTRL_2_REG);
}

static void mvpp2_port_enable(struct mvpp2_port *pp)
{
	u32 reg_val;

	reg_val = readl(pp->base + MVPP2_GMAC_CTRL_0_REG);
	reg_val |= MVPP2_GMAC_PORT_EN_MASK;
	reg_val |= MVPP2_GMAC_MIB_CNTR_EN_MASK;
	writel(reg_val, pp->base + MVPP2_GMAC_CTRL_0_REG);
}

static void mvpp2_port_disable(struct mvpp2_port *pp)
{
	u32 reg_val;

	reg_val = readl(pp->base + MVPP2_GMAC_CTRL_0_REG);
	reg_val &= ~(MVPP2_GMAC_PORT_EN_MASK);
	writel(reg_val, pp->base + MVPP2_GMAC_CTRL_0_REG);
}

/* Set IEEE 802.3x Flow Control Xon Packet Transmission Mode */
/* TODO: Only one user? Just disable? */
static void mvpp2_port_periodic_xon_set(struct mvpp2_port *pp, bool enable)
{
	u32 reg_val;

	reg_val = readl(pp->base + MVPP2_GMAC_CTRL_1_REG);

	if (enable)
		reg_val |= MVPP2_GMAC_PERIODIC_XON_EN_MASK;
	else
		reg_val &= ~MVPP2_GMAC_PERIODIC_XON_EN_MASK;

	writel(reg_val, pp->base + MVPP2_GMAC_CTRL_1_REG);
}

/* Configure loopback port */
static void mvpp2_port_loopback_set(struct mvpp2_port *pp)
{
	u32 reg_val;

	reg_val = readl(pp->base + MVPP2_GMAC_CTRL_1_REG);

	if (pp->speed == 1000)
		reg_val |= MVPP2_GMAC_GMII_LB_EN_MASK;
	else
		reg_val &= ~MVPP2_GMAC_GMII_LB_EN_MASK;

	if (pp->phy_interface == PHY_INTERFACE_MODE_SGMII)
		reg_val |= MVPP2_GMAC_PCS_LB_EN_MASK;
	else
		reg_val &= ~MVPP2_GMAC_PCS_LB_EN_MASK;

	writel(reg_val, pp->base + MVPP2_GMAC_CTRL_1_REG);
}

/* Enable/disable port reset state */
/* TODO: We have only one user, revisit this */
static void mvpp2_port_reset_set(struct mvpp2_port *pp, bool set_reset)
{
	u32 reg_val;

	reg_val = readl(pp->base + MVPP2_GMAC_CTRL_2_REG);
	reg_val &= ~MVPP2_GMAC_PORT_RESET_MASK;

	if (set_reset)
		reg_val |= MVPP2_GMAC_PORT_RESET_MASK;
	else
		reg_val &= ~MVPP2_GMAC_PORT_RESET_MASK;

	writel(reg_val, pp->base + MVPP2_GMAC_CTRL_2_REG);

	if (!set_reset)
		while (readl(pp->base + MVPP2_GMAC_CTRL_2_REG) &
		       MVPP2_GMAC_PORT_RESET_MASK)
			continue;
}

/* Change maximum receive size of the port */
static inline void mvpp2_gmac_max_rx_size_set(struct mvpp2_port *pp)
{
	int reg_val;

	reg_val = readl(pp->base + MVPP2_GMAC_CTRL_0_REG);
	reg_val &= ~MVPP2_GMAC_MAX_RX_SIZE_MASK;
	reg_val |= (((pp->pkt_size - MVPP2_MH_SIZE) / 2) <<
		    MVPP2_GMAC_MAX_RX_SIZE_OFFS);
	writel(reg_val, pp->base + MVPP2_GMAC_CTRL_0_REG);
}

/* Set defaults to the MVPP2 port */
static void mvpp2_defaults_set(struct mvpp2_port *pp)
{
	int reg_val;
	int cpu;
	int txp;
	int queue;
	int ptxq;
	int lrxq;
	int tx_port_num;

	/* Configure port to loopback if needed */
	if (pp->flags & MVPP2_F_LOOPBACK)
		mvpp2_port_loopback_set(pp);

	/* Update TX FIFO MIN Threshold */
	reg_val = readl(pp->base + MVPP2_GMAC_PORT_FIFO_CFG_1_REG);
	reg_val &= ~MVPP2_GMAC_TX_FIFO_MIN_TH_ALL_MASK;
	/* Min. TX threshold must be less than minimal packet length */
	reg_val |= MVPP2_GMAC_TX_FIFO_MIN_TH_MASK(64 - 4 - 2);
	writel(reg_val, pp->base + MVPP2_GMAC_PORT_FIFO_CFG_1_REG);

	for (txp = 0; txp < pp->txp_num; txp++) {
		/* Disable Legacy WRR, Disable EJP, Release from reset */
		tx_port_num = mvpp2_egress_port(pp, txp);
		mvpp2_write(pp->pp2, MVPP2_TXP_SCHED_PORT_INDEX_REG,
			    tx_port_num);
		mvpp2_write(pp->pp2, MVPP2_TXP_SCHED_CMD_1_REG, 0);

		/* Close bandwidth for all queues */
		for (queue = 0; queue < MVPP2_MAX_TXQ; queue++) {
			ptxq = mvpp2_txq_phys(pp->id, queue);
			mvpp2_write(pp->pp2,
				    MVPP2_TXQ_SCHED_TOKEN_CNTR_REG(ptxq), 0);
		}

		/* Set refill period to 1 usec, refill tokens
		 * and bucket size to maximum
		 */
		mvpp2_write(pp->pp2, MVPP2_TXP_SCHED_PERIOD_REG,
			    pp->pp2->tclk / 1000000);
		reg_val = mvpp2_read(pp->pp2, MVPP2_TXP_SCHED_REFILL_REG);
		reg_val &= ~MVPP2_TXP_REFILL_PERIOD_ALL_MASK;
		reg_val |= MVPP2_TXP_REFILL_PERIOD_MASK(1);
		reg_val |= MVPP2_TXP_REFILL_TOKENS_ALL_MASK;
		mvpp2_write(pp->pp2, MVPP2_TXP_SCHED_REFILL_REG, reg_val);
		reg_val = MVPP2_TXP_TOKEN_SIZE_MAX;
		mvpp2_write(pp->pp2, MVPP2_TXP_SCHED_TOKEN_SIZE_REG, reg_val);
	}
	/* Set MaximumLowLatencyPacketSize value to 256 */
	mvpp2_write(pp->pp2, MVPP2_RX_CTRL_REG(pp->id),
		    MVPP2_RX_USE_PSEUDO_FOR_CSUM_MASK |
		    MVPP2_RX_LOW_LATENCY_PKT_SIZE(256));

	/* Enable Rx cache snoop */
	for (lrxq = 0; lrxq < rxq_number; lrxq++) {
		queue = pp->rxqs[lrxq]->id;
		reg_val = mvpp2_read(pp->pp2, MVPP2_RXQ_CONFIG_REG(queue));
		reg_val |= MVPP2_SNOOP_PKT_SIZE_MASK |
			   MVPP2_SNOOP_BUF_HDR_MASK;
		mvpp2_write(pp->pp2, MVPP2_RXQ_CONFIG_REG(queue), reg_val);
	}

	/* At default, mask all interrupts to all present cpus */
	for_each_present_cpu(cpu)
		mvpp2_cpu_interrupts_disable(pp, cpu);
}

/* Enable/disable receiving packets */
static void mvpp2_ingress_enable(struct mvpp2_port *pp, bool en)
{
	u32 reg_val;
	int lrxq, queue;

	for (lrxq = 0; lrxq < rxq_number; lrxq++) {
		queue = pp->rxqs[lrxq]->id;
		reg_val = mvpp2_read(pp->pp2, MVPP2_RXQ_CONFIG_REG(queue));
		if (en)
			reg_val &= ~MVPP2_RXQ_DISABLE_MASK;
		else
			reg_val |= MVPP2_RXQ_DISABLE_MASK;
		mvpp2_write(pp->pp2, MVPP2_RXQ_CONFIG_REG(queue), reg_val);
	}
}

/* Disable transmit via physical egress queue
 * - HW doesn't take descriptors from DRAM
 */
static void mvpp2_txp_disable(struct mvpp2_port *pp, int txp)
{
	u32 reg_data;
	int delay;
	int tx_port_num = mvpp2_egress_port(pp, txp);

	/* Issue stop command for active channels only */
	mvpp2_write(pp->pp2, MVPP2_TXP_SCHED_PORT_INDEX_REG, tx_port_num);
	reg_data = (mvpp2_read(pp->pp2, MVPP2_TXP_SCHED_Q_CMD_REG)) &
		    MVPP2_TXP_SCHED_ENQ_MASK;
	if (reg_data != 0)
		mvpp2_write(pp->pp2, MVPP2_TXP_SCHED_Q_CMD_REG,
			    (reg_data << MVPP2_TXP_SCHED_DISQ_OFFSET));

	/* Wait for all Tx activity to terminate. */
	delay = 0;
	do {
		if (delay >= MVPP2_TX_DISABLE_TIMEOUT_MSEC) {
			netdev_warn(pp->dev,
				    "Tx stop timed out, status=0x%08x\n",
				    reg_data);
			break;
		}
		mdelay(1);
		delay++;

		/* Check port TX Command register that all
		 * Tx queues are stopped
		 */
		reg_data = mvpp2_read(pp->pp2, MVPP2_TXP_SCHED_Q_CMD_REG);
	} while (reg_data & MVPP2_TXP_SCHED_ENQ_MASK);
}

/* Enable transmit via physical egress queue
 * - HW starts take descriptors from DRAM
 */
static void mvpp2_txp_enable(struct mvpp2_port *pp, int txp)
{
	u32 qmap;
	int queue;
	int tx_port_num = mvpp2_egress_port(pp, txp);

	/* Enable all initialized TXs. */
	qmap = 0;
	for (queue = 0; queue < txq_number; queue++) {
		struct mvpp2_tx_queue *txq = pp->txqs[queue];
		if (txq->descs != NULL)
			qmap |= (1 << queue);
	}

	mvpp2_write(pp->pp2, MVPP2_TXP_SCHED_PORT_INDEX_REG, tx_port_num);
	mvpp2_write(pp->pp2, MVPP2_TXP_SCHED_Q_CMD_REG, qmap);
}

/* Enable/disable fetching descriptors from initialized TXQs */
static void mvpp2_egress_enable(struct mvpp2_port *pp, bool en)
{
	int txp;

	for (txp = 0; txp < pp->txp_num; txp++)
		if (en)
			/* Enable all physical TXQs */
			mvpp2_txp_enable(pp, txp);
		else
			/* Disable all physical TXQs */
			mvpp2_txp_disable(pp, txp);
}

/* Rx descriptors helper methods */

/* Get number of Rx descriptors occupied by received packets */
static inline int
mvpp2_rxq_received(struct mvpp2_port *pp, int rxq_id)
{
	u32 val = mvpp2_read(pp->pp2, MVPP2_RXQ_STATUS_REG(rxq_id));
	return val & MVPP2_RXQ_OCCUPIED_MASK;
}

/* Update Rx queue status with the number of occupied and available
 * Rx descriptor slots.
 */
static inline void
mvpp2_rxq_status_update(struct mvpp2_port *pp, int rxq_id,
			int used_count, int free_count)
{
	/* Decrement the number of used descriptors and increment count
	 * increment the number of free descriptors.
	 */
	u32 val = used_count | (free_count << MVPP2_RXQ_NUM_NEW_OFFSET);
	mvpp2_write(pp->pp2, MVPP2_RXQ_STATUS_UPDATE_REG(rxq_id), val);
}

/* Get pointer to next RX descriptor to be processed by SW */
static inline struct mvpp2_rx_desc *
mvpp2_rxq_next_desc_get(struct mvpp2_rx_queue *rxq)
{
	int rx_desc = rxq->next_desc_to_proc;

	rxq->next_desc_to_proc = MVPP2_QUEUE_NEXT_DESC(rxq, rx_desc);
	prefetch(rxq->descs + rxq->next_desc_to_proc);
	return rxq->descs + rx_desc;
}

/* Set rx queue offset */
static void mvpp2_rxq_offset_set(struct mvpp2_port *pp,
				 int prxq, int offset)
{
	u32 reg_val;

	/* Convert offset from bytes to units of 32 bytes */
	offset = offset >> 5;

	reg_val = mvpp2_read(pp->pp2, MVPP2_RXQ_CONFIG_REG(prxq));
	reg_val &= ~MVPP2_RXQ_PACKET_OFFSET_MASK;

	/* Offset is in */
	reg_val |= ((offset << MVPP2_RXQ_PACKET_OFFSET_OFFS) &
		    MVPP2_RXQ_PACKET_OFFSET_MASK);

	mvpp2_write(pp->pp2, MVPP2_RXQ_CONFIG_REG(prxq), reg_val);
}

/* Obtain BM cookie information from descriptor */
static u32 mvpp2_bm_cookie_build(struct mvpp2_rx_desc *rx_desc)
{
	int pool = (rx_desc->status & MVPP2_RXD_BM_POOL_ID_MASK) >>
		   MVPP2_RXD_BM_POOL_ID_OFFS;
	int cpu = smp_processor_id();

	return ((pool & 0xFF) << MVPP2_BM_COOKIE_POOL_OFFS) |
	       ((cpu & 0xFF) << MVPP2_BM_COOKIE_CPU_OFFS);
}

/* Tx descriptors helper methods */

/* Get number of Tx descriptors waiting to be transmitted by HW */
static int mvpp2_txq_pend_desc_num_get(struct mvpp2_port *pp,
				       struct mvpp2_tx_queue *txq)
{
	u32 reg_val;

	mvpp2_write(pp->pp2, MVPP2_TXQ_NUM_REG, txq->id);
	reg_val = mvpp2_read(pp->pp2, MVPP2_TXQ_PENDING_REG);

	return reg_val & MVPP2_TXQ_PENDING_MASK;
}

/* Get pointer to next Tx descriptor to be processed (send) by HW */
static struct mvpp2_tx_desc *
mvpp2_txq_next_desc_get(struct mvpp2_tx_queue *txq)
{
	int tx_desc = txq->next_desc_to_proc;

	txq->next_desc_to_proc = MVPP2_QUEUE_NEXT_DESC(txq, tx_desc);
	return txq->descs + tx_desc;
}

/* Update HW with number of aggregated Tx descriptors to be sent */
static void mvpp2_aggr_txq_pend_desc_add(struct mvpp2_port *pp, int pending)
{
	/* aggregated access - relevant TXQ number is written in TX desc */
	mvpp2_write(pp->pp2, MVPP2_AGGR_TXQ_UPDATE_REG, pending);
}

/* Get number of occupied aggregated Tx descriptors */
static u32 mvpp2_aggr_txq_pend_desc_num_get(struct mvpp2 *pp2, int cpu)
{
	u32 reg_val;

	reg_val = mvpp2_read(pp2, MVPP2_AGGR_TXQ_STATUS_REG(cpu));

	return reg_val & MVPP2_AGGR_TXQ_PENDING_MASK;
}

/* Check if there are enough free descriptors in aggregated txq. If not, try to
 * update number of occupied descriptors from HW and repeat checking.
 */
static int mvpp2_aggr_desc_num_check(struct mvpp2 *pp2,
				     struct mvpp2_tx_queue *aggr_txq, int num)
{
	if ((aggr_txq->count + num) > aggr_txq->size) {
		aggr_txq->count = mvpp2_aggr_txq_pend_desc_num_get(pp2,
							    smp_processor_id());
	}
	if ((aggr_txq->count + num) > aggr_txq->size)
		return -ERANGE;

	return 0;
}

/* Reserved Tx descriptors allocation request */
static int mvpp2_txq_alloc_reserved_desc(struct mvpp2 *pp2,
					 struct mvpp2_tx_queue *txq, int num)
{
	u32 reg_val;

	reg_val = (txq->id << MVPP2_TXQ_RSVD_REQ_Q_OFFSET) | num;
	mvpp2_write(pp2, MVPP2_TXQ_RSVD_REQ_REG, reg_val);

	reg_val = mvpp2_read(pp2, MVPP2_TXQ_RSVD_RSLT_REG);

	return reg_val & MVPP2_TXQ_RSVD_RSLT_MASK;
}

/* Check if there are enough reserved descriptors for transmission. If not, try
 * to reqest chunk of reserved descriptors and check again.
 */
static int mvpp2_txq_reserved_desc_num_proc(struct mvpp2 *pp2,
					    struct mvpp2_tx_queue *txq,
					    struct mvpp2_txq_pcpu *txq_pcpu,
					    int num)
{
	struct mvpp2_txq_pcpu *txq_pcpu_aux;

	if (txq_pcpu->reserved_num < num) {
		int req, cpu, new_reserved, desc_count = 0;

		/* Compute total of used descriptors */
		for_each_present_cpu(cpu) {
			txq_pcpu_aux = per_cpu_ptr(txq->pcpu, cpu);
			desc_count += txq_pcpu_aux->count;
			desc_count += txq_pcpu_aux->reserved_num;
		}

		req = max(MVPP2_CPU_DESC_CHUNK, num - txq_pcpu->reserved_num);
		desc_count += req;

		if (desc_count > txq->swf_size)
			return -ERANGE;

		new_reserved = mvpp2_txq_alloc_reserved_desc(pp2, txq, req);

		txq_pcpu->reserved_num += new_reserved;

		if (txq_pcpu->reserved_num < num)
			return -ERANGE;
	}

	return 0;
}

/* Release the last allocated Tx descriptor. Useful to handle DMA
 * mapping failures in the Tx path.
 */
static void mvpp2_txq_desc_put(struct mvpp2_tx_queue *txq)
{
	if (txq->next_desc_to_proc == 0)
		txq->next_desc_to_proc = txq->last_desc - 1;
	else
		txq->next_desc_to_proc--;
}

/* Set Tx descriptors fields relevant for CSUM calculation */
static u32 mvpp2_txq_desc_csum(int l3_offs, int l3_proto,
				int ip_hdr_len, int l4_proto)
{
	u32 command;

	/* fields: L3_offset, IP_hdrlen, L3_type, G_IPv4_chk,
	 * G_L4_chk, L4_type required only for checksum calculation
	 */
	command = (l3_offs << MVPP2_TXD_L3_OFF_SHIFT);
	command |= (ip_hdr_len << MVPP2_TXD_IP_HLEN_SHIFT);
	command |= MVPP2_TXD_IP_CSUM_DISABLE;

	if (l3_proto == swab16(ETH_P_IP)) {
		command &= ~MVPP2_TXD_IP_CSUM_DISABLE;	/* enable IPv4 csum */
		command &= ~MVPP2_TXD_L3_IP6;		/* enable IPv4 */
	} else
		command |= MVPP2_TXD_L3_IP6;		/* enable IPv6 */

	if (l4_proto == IPPROTO_TCP) {
		command &= ~MVPP2_TXD_L4_UDP;		/* enable TCP */
		command &= ~MVPP2_TXD_L4_CSUM_FRAG;	/* generate L4 csum */
	} else if (l4_proto == IPPROTO_UDP) {
		command |= MVPP2_TXD_L4_UDP;		/* enable UDP */
		command &= ~MVPP2_TXD_L4_CSUM_FRAG;	/* generate L4 csum */
	} else
		command |= MVPP2_TXD_L4_CSUM_NOT;

	return command;
}

/* Get number of sent descriptors and decrement counter.
 * The number of sent descriptors is returned.
 * Per-CPU access
 */
static inline int mvpp2_txq_sent_desc_proc(struct mvpp2_port *pp,
					   struct mvpp2_tx_queue *txq)
{
	u32 reg_val;

	/* Reading status reg resets transmitted descriptor counter */
	reg_val = mvpp2_read(pp->pp2, MVPP2_TXQ_SENT_REG(txq->id));

	return (reg_val & MVPP2_TRANSMITTED_COUNT_MASK) >>
		MVPP2_TRANSMITTED_COUNT_OFFSET;
}

static void mvpp2_txq_sent_counter_clear(void *arg)
{
	struct mvpp2_tx_queue *txq = arg;
	struct mvpp2 *pp2 = txq->pp2;
	int reg_val;

	reg_val = mvpp2_read(pp2, MVPP2_TXQ_SENT_REG(txq->id));
}

/* Queues helepr methods */

/* Set max sizes for Tx queues */
static void mvpp2_txp_max_tx_size_set(struct mvpp2_port *pp, int txp)
{
	u32	reg_val, size, mtu;
	int	txq, tx_port_num;

	mtu = pp->pkt_size * 8;
	if (mtu > MVPP2_TXP_MTU_MAX)
		mtu = MVPP2_TXP_MTU_MAX;

	/* WA for wrong Token bucket update: Set MTU value = 3*real MTU value */
	mtu = 3 * mtu;

	/* Indirect access to registers */
	tx_port_num = mvpp2_egress_port(pp, txp);
	mvpp2_write(pp->pp2, MVPP2_TXP_SCHED_PORT_INDEX_REG, tx_port_num);

	/* Set MTU */
	reg_val = mvpp2_read(pp->pp2, MVPP2_TXP_SCHED_MTU_REG);
	reg_val &= ~MVPP2_TXP_MTU_MAX;
	reg_val |= mtu;
	mvpp2_write(pp->pp2, MVPP2_TXP_SCHED_MTU_REG, reg_val);

	/* TXP token size and all TXQs token size must be larger that MTU */
	reg_val = mvpp2_read(pp->pp2, MVPP2_TXP_SCHED_TOKEN_SIZE_REG);
	size = reg_val & MVPP2_TXP_TOKEN_SIZE_MAX;
	if (size < mtu) {
		size = mtu;
		reg_val &= ~MVPP2_TXP_TOKEN_SIZE_MAX;
		reg_val |= size;
		mvpp2_write(pp->pp2, MVPP2_TXP_SCHED_TOKEN_SIZE_REG, reg_val);
	}
	for (txq = 0; txq < txq_number; txq++) {
		reg_val = mvpp2_read(pp->pp2,
				     MVPP2_TXQ_SCHED_TOKEN_SIZE_REG(txq));
		size = reg_val & MVPP2_TXQ_TOKEN_SIZE_MAX;
		if (size < mtu) {
			size = mtu;
			reg_val &= ~MVPP2_TXQ_TOKEN_SIZE_MAX;
			reg_val |= size;
			mvpp2_write(pp->pp2,
				    MVPP2_TXQ_SCHED_TOKEN_SIZE_REG(txq),
				    reg_val);
		}
	}
}

/* Set the number of packets that will be received before Rx interrupt
 * will be generated by HW.
 */
static void mvpp2_rx_pkts_coal_set(struct mvpp2_port *pp,
				   struct mvpp2_rx_queue *rxq, u32 pkts)
{
	u32 val;

	val = (pkts & MVPP2_OCCUPIED_THRESH_MASK);
	mvpp2_write(pp->pp2, MVPP2_RXQ_NUM_REG, rxq->id);
	mvpp2_write(pp->pp2, MVPP2_RXQ_THRESH_REG, val);

	rxq->pkts_coal = pkts;
}

/* Set the time delay in usec before RX interrupt will be generated by
 * HW.
 */
static void mvpp2_rx_time_coal_set(struct mvpp2_port *pp,
				    struct mvpp2_rx_queue *rxq, u32 usec)
{
	u32 val;

	val = (pp->pp2->tclk / 1000000) * usec;
	mvpp2_write(pp->pp2, MVPP2_ISR_RX_THRESHOLD_REG(rxq->id), val);

	rxq->time_coal = usec;
}

/* Set threshold for TX_DONE pkts coalescing */
static void mvpp2_tx_done_pkts_coal_set(void *arg)
{
	struct mvpp2_tx_queue *txq = arg;
	struct mvpp2 *pp2 = txq->pp2;
	u32 pkts = txq->done_pkts_coal;
	u32 reg_val;

	reg_val = (pkts << MVPP2_TRANSMITTED_THRESH_OFFSET) &
		   MVPP2_TRANSMITTED_THRESH_MASK;
	mvpp2_write(pp2, MVPP2_TXQ_NUM_REG, txq->id);
	mvpp2_write(pp2, MVPP2_TXQ_THRESH_REG, reg_val);
}

/* Free Tx queue skbuffs */
static void mvpp2_txq_bufs_free(struct mvpp2_port *pp,
				struct mvpp2_tx_queue *txq,
				struct mvpp2_txq_pcpu *txq_pcpu, int num)
{
	int i;

	for (i = 0; i < num; i++) {
		struct mvpp2_tx_desc *tx_desc = txq->descs +
							txq_pcpu->txq_get_index;
		struct sk_buff *skb = txq_pcpu->tx_skb[txq_pcpu->txq_get_index];

		mvpp2_txq_inc_get(txq_pcpu);

		if (!skb)
			continue;

		dma_unmap_single(pp->dev->dev.parent, tx_desc->buf_phys_addr,
				 tx_desc->data_size, DMA_TO_DEVICE);
		dev_kfree_skb_any(skb);
	}
}

static inline struct mvpp2_rx_queue *mvpp2_get_rx_queue(struct mvpp2_port *pp,
							u32 cause)
{
	int queue = fls(cause) - 1;
	return pp->rxqs[queue];
}

static inline struct mvpp2_tx_queue *mvpp2_get_tx_queue(struct mvpp2_port *pp,
							u32 cause)
{
	int queue = fls(cause >> 16) - 1;
	return pp->txqs[queue];
}

/* Handle end of transmission */
static void mvpp2_txq_done(struct mvpp2_port *pp, struct mvpp2_tx_queue *txq,
			   struct mvpp2_txq_pcpu *txq_pcpu)
{
	struct netdev_queue *nq = netdev_get_tx_queue(pp->dev, txq->log_id);
	int tx_done;

	if (txq_pcpu->cpu != smp_processor_id())
		netdev_err(pp->dev, "wrong cpu on the end of Tx processing\n");

	tx_done = mvpp2_txq_sent_desc_proc(pp, txq);
	if (!tx_done)
		return;
	mvpp2_txq_bufs_free(pp, txq, txq_pcpu, tx_done);

	txq_pcpu->count -= tx_done;

	if (netif_tx_queue_stopped(nq))
		if (txq_pcpu->size - txq_pcpu->count >= MAX_SKB_FRAGS + 1)
			netif_tx_wake_queue(nq);

	return;
}

/* Rx/Tx queue initialization/cleanup methods */

/* Allocate and initialize descriptors for aggr TXQ */
static int mvpp2_aggr_txq_init(struct platform_device *pdev,
			       struct mvpp2_tx_queue *aggr_txq,
			       int desc_num, int cpu,
			       struct mvpp2 *pp2)
{
	/* Allocate memory for TX descriptors */
	aggr_txq->descs = dma_alloc_coherent(&pdev->dev,
				desc_num * MVPP2_DESC_ALIGNED_SIZE,
				&aggr_txq->descs_phys, GFP_KERNEL);
	if (!aggr_txq->descs)
		return -ENOMEM;

	/* Make sure descriptor address is cache line size aligned  */
	BUG_ON(aggr_txq->descs !=
	       PTR_ALIGN(aggr_txq->descs, MVPP2_CPU_D_CACHE_LINE_SIZE));

	aggr_txq->last_desc = aggr_txq->size - 1;

	/* Aggr TXQ no reset WA */
	aggr_txq->next_desc_to_proc = mvpp2_read(pp2,
						 MVPP2_AGGR_TXQ_INDEX_REG(cpu));

	/* Set Tx descriptors queue starting address */
	/* indirect access */
	mvpp2_write(pp2, MVPP2_AGGR_TXQ_DESC_ADDR_REG(cpu),
		    aggr_txq->descs_phys);
	mvpp2_write(pp2, MVPP2_AGGR_TXQ_DESC_SIZE_REG(cpu), desc_num);

	return 0;
}

/* Create a specified Rx queue */
static int mvpp2_rxq_init(struct mvpp2_port *pp,
			   struct mvpp2_rx_queue *rxq)

{
	rxq->size = pp->rx_ring_size;

	/* Allocate memory for RX descriptors */
	rxq->descs = dma_alloc_coherent(pp->dev->dev.parent,
					rxq->size * MVPP2_DESC_ALIGNED_SIZE,
					&rxq->descs_phys, GFP_KERNEL);
	if (!rxq->descs)
		return -ENOMEM;

	BUG_ON(rxq->descs !=
	       PTR_ALIGN(rxq->descs, MVPP2_CPU_D_CACHE_LINE_SIZE));

	rxq->last_desc = rxq->size - 1;

	/* Zero occupied and non-occupied counters - direct access */
	mvpp2_write(pp->pp2, MVPP2_RXQ_STATUS_REG(rxq->id), 0);

	/* Set Rx descriptors queue starting address - indirect access */
	mvpp2_write(pp->pp2, MVPP2_RXQ_NUM_REG, rxq->id);
	mvpp2_write(pp->pp2, MVPP2_RXQ_DESC_ADDR_REG, rxq->descs_phys);
	mvpp2_write(pp->pp2, MVPP2_RXQ_DESC_SIZE_REG, rxq->size);
	mvpp2_write(pp->pp2, MVPP2_RXQ_INDEX_REG, 0);

	/* Set Offset */
	mvpp2_rxq_offset_set(pp, rxq->id, NET_SKB_PAD);

	/* Set coalescing pkts and time */
	mvpp2_rx_pkts_coal_set(pp, rxq, rxq->pkts_coal);
	mvpp2_rx_time_coal_set(pp, rxq, rxq->time_coal);

	/* Add number of descriptors ready for receiving packets */
	mvpp2_rxq_status_update(pp, rxq->id, 0, rxq->size);

	return 0;
}

/* Push packets received by the RXQ to BM pool */
static void mvpp2_rxq_drop_pkts(struct mvpp2_port *pp,
				struct mvpp2_rx_queue *rxq)
{
	int rx_received, i;

	rx_received = mvpp2_rxq_received(pp, rxq->id);
	if (!rx_received)
		return;

	for (i = 0; i < rx_received; i++) {
		struct mvpp2_rx_desc *rx_desc = mvpp2_rxq_next_desc_get(rxq);
		struct mvpp2_bm_pool *bm_pool;
		int pool;
		u32 bm;

		bm = mvpp2_bm_cookie_build(rx_desc);
		pool = mvpp2_bm_cookie_pool_get(bm);
		bm_pool = &pp->pp2->bm_pools[pool];

		mvpp2_pool_refill(pp, bm, rx_desc->buf_phys_addr,
				  rx_desc->buf_cookie);
	}
	mvpp2_rxq_status_update(pp, rxq->id, rx_received, rx_received);
}

/* Cleanup Rx queue */
static void mvpp2_rxq_deinit(struct mvpp2_port *pp,
			     struct mvpp2_rx_queue *rxq)
{
	mvpp2_rxq_drop_pkts(pp, rxq);

	if (rxq->descs)
		dma_free_coherent(pp->dev->dev.parent,
				  rxq->size * MVPP2_DESC_ALIGNED_SIZE,
				  rxq->descs,
				  rxq->descs_phys);

	rxq->descs             = NULL;
	rxq->last_desc         = 0;
	rxq->next_desc_to_proc = 0;
	rxq->descs_phys        = 0;

	/* Clear Rx descriptors queue starting address and size;
	 * free descriptor number
	 */
	mvpp2_write(pp->pp2, MVPP2_RXQ_STATUS_REG(rxq->id), 0);
	mvpp2_write(pp->pp2, MVPP2_RXQ_NUM_REG, rxq->id);
	mvpp2_write(pp->pp2, MVPP2_RXQ_DESC_ADDR_REG, 0);
	mvpp2_write(pp->pp2, MVPP2_RXQ_DESC_SIZE_REG, 0);
}

/* Create and initialize a Tx queue */
static int mvpp2_txq_init(struct mvpp2_port *pp, int txp,
			  struct mvpp2_tx_queue *txq)
{
	u32 reg_val;
	int cpu, desc;
	int desc_per_txq;
	int tx_port_num;
	struct mvpp2_txq_pcpu *txq_pcpu;

	/* Allocate memory for Tx descriptors */
	txq->descs = dma_alloc_coherent(pp->dev->dev.parent,
				txq->size * MVPP2_DESC_ALIGNED_SIZE,
				&txq->descs_phys, GFP_KERNEL);
	if (!txq->descs)
		return -ENOMEM;

	/* Make sure descriptor address is cache line size aligned  */
	BUG_ON(txq->descs !=
	       PTR_ALIGN(txq->descs, MVPP2_CPU_D_CACHE_LINE_SIZE));

	txq->last_desc = txq->size - 1;

	/* Set Tx descriptors queue starting address - indirect access */
	mvpp2_write(pp->pp2, MVPP2_TXQ_NUM_REG, txq->id);
	mvpp2_write(pp->pp2, MVPP2_TXQ_DESC_ADDR_REG, txq->descs_phys);
	mvpp2_write(pp->pp2, MVPP2_TXQ_DESC_SIZE_REG, txq->size &
					     MVPP2_TXQ_DESC_SIZE_MASK);
	mvpp2_write(pp->pp2, MVPP2_TXQ_INDEX_REG, 0);
	mvpp2_write(pp->pp2, MVPP2_TXQ_RSVD_CLR_REG,
		    txq->id << MVPP2_TXQ_RSVD_CLR_OFFSET);
	reg_val = mvpp2_read(pp->pp2, MVPP2_TXQ_PENDING_REG);
	reg_val &= ~MVPP2_TXQ_PENDING_MASK;
	mvpp2_write(pp->pp2, MVPP2_TXQ_PENDING_REG, reg_val);

	/* Calculate base address in prefetch buffer. We reserve 16 descriptors
	 * for each existing TXQ.
	 * TCONTS for PON port must be continuous from 0 to MVPP2_MAX_TCONT
	 * GBE ports assumed to be continious from 0 to MVPP2_MAX_PORTS
	 */
	desc_per_txq = 16;
	desc = (pp->id * MVPP2_MAX_TXQ * desc_per_txq) +
	       (txq->log_id * desc_per_txq);

	mvpp2_write(pp->pp2, MVPP2_TXQ_PREF_BUF_REG,
		    MVPP2_PREF_BUF_PTR(desc) | MVPP2_PREF_BUF_SIZE_16 |
		    MVPP2_PREF_BUF_THRESH(desc_per_txq/2));

	/* WRR / EJP configuration - indirect access */
	tx_port_num = mvpp2_egress_port(pp, txp);
	mvpp2_write(pp->pp2, MVPP2_TXP_SCHED_PORT_INDEX_REG, tx_port_num);

	reg_val = mvpp2_read(pp->pp2, MVPP2_TXQ_SCHED_REFILL_REG(txq->log_id));
	reg_val &= ~MVPP2_TXQ_REFILL_PERIOD_ALL_MASK;
	reg_val |= MVPP2_TXQ_REFILL_PERIOD_MASK(1);
	reg_val |= MVPP2_TXQ_REFILL_TOKENS_ALL_MASK;
	mvpp2_write(pp->pp2, MVPP2_TXQ_SCHED_REFILL_REG(txq->log_id), reg_val);

	reg_val = MVPP2_TXQ_TOKEN_SIZE_MAX;
	mvpp2_write(pp->pp2, MVPP2_TXQ_SCHED_TOKEN_SIZE_REG(txq->log_id),
		    reg_val);

	for_each_present_cpu(cpu) {
		txq_pcpu = per_cpu_ptr(txq->pcpu, cpu);
		txq_pcpu->tx_skb = kmalloc(txq_pcpu->size *
					   sizeof(*txq_pcpu->tx_skb),
					   GFP_KERNEL);
		if (!txq_pcpu->tx_skb) {
			dma_free_coherent(pp->dev->dev.parent,
					  txq->size * MVPP2_DESC_ALIGNED_SIZE,
					  txq->descs, txq->descs_phys);
			return -ENOMEM;
		}

		txq_pcpu->count = 0;
		txq_pcpu->reserved_num = 0;
		txq_pcpu->txq_put_index = 0;
		txq_pcpu->txq_get_index = 0;
	}

	on_each_cpu(mvpp2_txq_sent_counter_clear, txq, 1);
	on_each_cpu(mvpp2_tx_done_pkts_coal_set, txq, 1);

	return 0;
}

/* Free allocated TXQ resources */
static void mvpp2_txq_deinit(struct mvpp2_port *pp,
			     struct mvpp2_tx_queue *txq)
{
	struct mvpp2_txq_pcpu *txq_pcpu;
	int cpu;

	for_each_present_cpu(cpu) {
		txq_pcpu = per_cpu_ptr(txq->pcpu, cpu);
		kfree(txq_pcpu->tx_skb);
	}

	if (txq->descs)
		dma_free_coherent(pp->dev->dev.parent,
				  txq->size * MVPP2_DESC_ALIGNED_SIZE,
				  txq->descs, txq->descs_phys);

	txq->descs             = NULL;
	txq->last_desc         = 0;
	txq->next_desc_to_proc = 0;
	txq->descs_phys        = 0;

	/* Set minimum bandwidth for disabled TXQs */
	mvpp2_write(pp->pp2, MVPP2_TXQ_SCHED_TOKEN_CNTR_REG(txq->id), 0);

	/* Set Tx descriptors queue starting address and size */
	mvpp2_write(pp->pp2, MVPP2_TXQ_NUM_REG, txq->id);
	mvpp2_write(pp->pp2, MVPP2_TXQ_DESC_ADDR_REG, 0);
	mvpp2_write(pp->pp2, MVPP2_TXQ_DESC_SIZE_REG, 0);
}

/* Cleanup Tx ports */
static void mvpp2_txp_clean(struct mvpp2_port *pp, int txp,
			    struct mvpp2_tx_queue *txq)
{
	struct mvpp2_txq_pcpu *txq_pcpu;
	int delay, pending, cpu;
	u32 reg_val;

	mvpp2_write(pp->pp2, MVPP2_TXQ_NUM_REG, txq->id);
	reg_val = mvpp2_read(pp->pp2, MVPP2_TXQ_PREF_BUF_REG);
	reg_val |= MVPP2_TXQ_DRAIN_EN_MASK;
	mvpp2_write(pp->pp2, MVPP2_TXQ_PREF_BUF_REG, reg_val);

	/* The napi queue has been stopped so wait for all packets
	 * to be transmitted.
	 */
	delay = 0;
	do {
		if (delay >= MVPP2_TX_PENDING_TIMEOUT_MSEC) {
			netdev_warn(pp->dev,
				    "port %d: cleaning queue %d timed out\n",
				    pp->id, txq->log_id);
			break;
		}
		mdelay(1);
		delay++;

		pending = mvpp2_txq_pend_desc_num_get(pp, txq);
	} while (pending);

	reg_val &= ~MVPP2_TXQ_DRAIN_EN_MASK;
	mvpp2_write(pp->pp2, MVPP2_TXQ_PREF_BUF_REG, reg_val);

	on_each_cpu(mvpp2_txq_sent_counter_clear, txq, 1);

	for_each_present_cpu(cpu) {
		txq_pcpu = per_cpu_ptr(txq->pcpu, cpu);

		/* Release all packets */
		mvpp2_txq_bufs_free(pp, txq, txq_pcpu, txq_pcpu->count);

		/* Reset queue */
		txq_pcpu->count = 0;
		txq_pcpu->txq_put_index = 0;
		txq_pcpu->txq_get_index = 0;
	}
}

/* Cleanup all Tx queues */
static void mvpp2_cleanup_txqs(struct mvpp2_port *pp)
{
	struct mvpp2_tx_queue *txq;
	int txp, queue;
	u32 reg_val;

	reg_val = mvpp2_read(pp->pp2, MVPP2_TX_PORT_FLUSH_REG);

	/* Reset Tx ports and delete Tx queues */
	for (txp = 0; txp < pp->txp_num; txp++) {
		reg_val |= MVPP2_TX_PORT_FLUSH_MASK(pp->id);
		mvpp2_write(pp->pp2, MVPP2_TX_PORT_FLUSH_REG, reg_val);

		for (queue = 0; queue < txq_number; queue++) {
			txq = pp->txqs[txp * txq_number + queue];
			mvpp2_txp_clean(pp, txp, txq);
			mvpp2_txq_deinit(pp, txq);
		}

		reg_val &= ~MVPP2_TX_PORT_FLUSH_MASK(pp->id);
		mvpp2_write(pp->pp2, MVPP2_TX_PORT_FLUSH_REG, reg_val);
	}
}

/* Cleanup all Rx queues */
static void mvpp2_cleanup_rxqs(struct mvpp2_port *pp)
{
	int queue;

	for (queue = 0; queue < rxq_number; queue++)
		mvpp2_rxq_deinit(pp, pp->rxqs[queue]);
}

/* Init all Rx queues for port */
static int mvpp2_setup_rxqs(struct mvpp2_port *pp)
{
	int queue, err;

	for (queue = 0; queue < rxq_number; queue++) {
		err = mvpp2_rxq_init(pp, pp->rxqs[queue]);
		if (err)
			goto err_cleanup;
	}
	return 0;

err_cleanup:
	mvpp2_cleanup_rxqs(pp);
	return err;
}

/* Init all tx queues for port */
static int mvpp2_setup_txqs(struct mvpp2_port *pp)
{
	struct mvpp2_tx_queue *txq;
	int txp, queue, err;

	for (txp = 0; txp < pp->txp_num; txp++) {
		for (queue = 0; queue < txq_number; queue++) {
			txq = pp->txqs[txp * txq_number + queue];
			err = mvpp2_txq_init(pp, txp, txq);
			if (err)
				goto err_cleanup;
		}
	}
	return 0;

err_cleanup:
	mvpp2_cleanup_txqs(pp);
	return err;
}

/* The callback for per-port interrupt */
static irqreturn_t mvpp2_isr(int irq, void *dev_id)
{
	struct mvpp2_port *pp = (struct mvpp2_port *)dev_id;
	int cpu;

	for_each_present_cpu(cpu)
		mvpp2_cpu_interrupts_disable(pp, cpu);

	napi_schedule(&pp->napi);

	return IRQ_HANDLED;
}

/* Adjust link */
void mvpp2_link_event(struct net_device *dev)
{
	struct mvpp2_port *pp = netdev_priv(dev);
	struct phy_device *phydev = pp->phy_dev;
	int status_change = 0;
	u32 val;

	if (phydev->link) {
		if ((pp->speed != phydev->speed) ||
		    (pp->duplex != phydev->duplex)) {
			u32 val;

			val = readl(pp->base + MVPP2_GMAC_AUTONEG_CONFIG);
			val &= ~(MVPP2_GMAC_CONFIG_MII_SPEED |
				 MVPP2_GMAC_CONFIG_GMII_SPEED |
				 MVPP2_GMAC_CONFIG_FULL_DUPLEX |
				 MVPP2_GMAC_AN_SPEED_EN |
				 MVPP2_GMAC_AN_DUPLEX_EN);

			if (phydev->duplex)
				val |= MVPP2_GMAC_CONFIG_FULL_DUPLEX;

			if (phydev->speed == SPEED_1000)
				val |= MVPP2_GMAC_CONFIG_GMII_SPEED;
			else
				val |= MVPP2_GMAC_CONFIG_MII_SPEED;

			writel(val, pp->base + MVPP2_GMAC_AUTONEG_CONFIG);

			pp->duplex = phydev->duplex;
			pp->speed  = phydev->speed;
		}
	}

	if (phydev->link != pp->link) {
		if (!phydev->link) {
			pp->duplex = -1;
			pp->speed = 0;
		}

		pp->link = phydev->link;
		status_change = 1;
	}

	if (status_change) {
		if (phydev->link) {
			val = readl(pp->base + MVPP2_GMAC_AUTONEG_CONFIG);
			val |= (MVPP2_GMAC_FORCE_LINK_PASS |
				MVPP2_GMAC_FORCE_LINK_DOWN);
			writel(val, pp->base + MVPP2_GMAC_AUTONEG_CONFIG);
			mvpp2_egress_enable(pp, true);
			mvpp2_ingress_enable(pp, true);
			netdev_info(pp->dev, "link up");
		} else {
			mvpp2_ingress_enable(pp, false);
			mvpp2_egress_enable(pp, false);
			netdev_info(pp->dev, "link down\n");
		}
	}
}

/* Main RX/TX processing routines */

/* Display more error info */
static void mvpp2_rx_error(struct mvpp2_port *pp,
			   struct mvpp2_rx_desc *rx_desc)
{
	u32 status = rx_desc->status;

	switch (status & MVPP2_RXD_ERR_CODE_MASK) {
	case MVPP2_RXD_ERR_CRC:
		netdev_err(pp->dev, "bad rx status %08x (crc error), size=%d\n",
			   status, rx_desc->data_size);
		break;
	case MVPP2_RXD_ERR_OVERRUN:
		netdev_err(pp->dev, "bad rx status %08x (overrun error), size=%d\n",
			   status, rx_desc->data_size);
		break;
	case MVPP2_RXD_ERR_RESOURCE:
		netdev_err(pp->dev, "bad rx status %08x (resource error), size=%d\n",
			   status, rx_desc->data_size);
		break;
	}
}

/* Handle RX checksum offload */
static void mvpp2_rx_csum(struct mvpp2_port *pp, u32 status,
			  struct sk_buff *skb)
{
	if (((status & MVPP2_RXD_L3_IP4) &&
	    !(status & MVPP2_RXD_IP4_HEADER_ERR)) ||
	    (status & MVPP2_RXD_L3_IP6))
		if (((status & MVPP2_RXD_L4_UDP) ||
		     (status & MVPP2_RXD_L4_TCP)) &&
		     (status & MVPP2_RXD_L4_CSUM_OK)) {
			skb->csum = 0;
			skb->ip_summed = CHECKSUM_UNNECESSARY;
			return;
		}

	skb->ip_summed = CHECKSUM_NONE;
}

/* Reuse skb if possible, or allocate a new skb and add it to BM pool */
static int mvpp2_rx_refill(struct mvpp2_port *pp, struct mvpp2_bm_pool *bm_pool,
			   u32 bm, int is_recycle)
{
	struct sk_buff *skb;
	dma_addr_t phys_addr;

	if (is_recycle &&
	   (atomic_read(&bm_pool->in_use) < bm_pool->in_use_thresh))
		return 0;

	/* No recycle or too many buffers are in use, so allocate a new skb */
	skb = mvpp2_skb_alloc(pp, bm_pool, &phys_addr, GFP_ATOMIC);
	if (!skb)
		return -ENOMEM;

	mvpp2_pool_refill(pp, bm, (u32)phys_addr, (u32)skb);
	atomic_dec(&bm_pool->in_use);
	return 0;
}

/* Handle tx checksum */
static u32 mvpp2_skb_tx_csum(struct mvpp2_port *pp, struct sk_buff *skb)
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
		} else
			return MVPP2_TXD_L4_CSUM_NOT;

		return mvpp2_txq_desc_csum(skb_network_offset(skb),
				skb->protocol, ip_hdr_len, l4_proto);
	}

	return MVPP2_TXD_L4_CSUM_NOT | MVPP2_TXD_IP_CSUM_DISABLE;
}

static void mvpp2_buff_hdr_rx(struct mvpp2_port *pp,
			      struct mvpp2_rx_desc *rx_desc)
{
	struct mvpp2_buff_hdr *buff_hdr;
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
	buff_phys_addr = rx_desc->buf_phys_addr;
	buff_virt_addr = rx_desc->buf_cookie;

	do {
		skb = (struct sk_buff *)buff_virt_addr;
		buff_hdr = (struct mvpp2_buff_hdr *)skb->head;

		mc_id = MVPP2_B_HDR_INFO_MC_ID(buff_hdr->info);

		buff_phys_addr_next = buff_hdr->next_buff_phys_addr;
		buff_virt_addr_next = buff_hdr->next_buff_virt_addr;

		/* Release buffer */
		mvpp2_bm_pool_mc_put(pp, pool_id, buff_phys_addr,
				     buff_virt_addr, mc_id, 0);

		buff_phys_addr = buff_phys_addr_next;
		buff_virt_addr = buff_virt_addr_next;

	} while (!MVPP2_B_HDR_INFO_IS_LAST(buff_hdr->info));
}

/* Main rx processing */
static int mvpp2_rx(struct mvpp2_port *pp, int rx_todo,
		    struct mvpp2_rx_queue *rxq)
{
	struct net_device *dev = pp->dev;
	int rx_received, rx_filled, i;
	u32 rcvd_pkts = 0;
	u32 rcvd_bytes = 0;

	/* Get number of received packets and clamp the to-do */
	rx_received = mvpp2_rxq_received(pp, rxq->id);
	if (rx_todo > rx_received)
		rx_todo = rx_received;

	rx_filled = 0;
	for (i = 0; i < rx_todo; i++) {
		struct mvpp2_rx_desc *rx_desc = mvpp2_rxq_next_desc_get(rxq);
		struct mvpp2_bm_pool *bm_pool;
		struct sk_buff *skb;
		u32 bm, rx_status;
		int pool, rx_bytes, err;

		rx_filled++;
		rx_status = rx_desc->status;
		rx_bytes = rx_desc->data_size - MVPP2_MH_SIZE;

		bm = mvpp2_bm_cookie_build(rx_desc);
		pool = mvpp2_bm_cookie_pool_get(bm);
		bm_pool = &pp->pp2->bm_pools[pool];
		/* Check if buffer header is used */
		if (rx_status & MVPP2_RXD_BUF_HDR) {
			mvpp2_buff_hdr_rx(pp, rx_desc);
			continue;
		}

		/* In case of an error, release the requested buffer pointer
		 * to the Buffer Manager. This request process is controlled
		 * by the hardware, and the information about the buffer is
		 * comprised by the RX descriptor.
		 */
		if (rx_status & MVPP2_RXD_ERR_SUMMARY) {
			dev->stats.rx_errors++;
			mvpp2_rx_error(pp, rx_desc);
			mvpp2_pool_refill(pp, bm, rx_desc->buf_phys_addr,
					  rx_desc->buf_cookie);
			continue;
		}

		skb = (struct sk_buff *)rx_desc->buf_cookie;

		rcvd_pkts++;
		rcvd_bytes += rx_bytes;
		atomic_inc(&bm_pool->in_use);

		skb_reserve(skb, MVPP2_MH_SIZE);
		skb_put(skb, rx_bytes);
		skb->protocol = eth_type_trans(skb, dev);
		mvpp2_rx_csum(pp, rx_status, skb);

		napi_gro_receive(&pp->napi, skb);

		err = mvpp2_rx_refill(pp, bm_pool, bm, 0);
		if (err) {
			netdev_err(pp->dev, "failed to refill BM pools\n");
			rx_filled--;
		}
	}

	if (rcvd_pkts) {
		struct mvpp2_pcpu_stats *stats = this_cpu_ptr(pp->stats);

		u64_stats_update_begin(&stats->syncp);
		stats->rx_packets += rcvd_pkts;
		stats->rx_bytes   += rcvd_bytes;
		u64_stats_update_end(&stats->syncp);
	}

	/* Update Rx queue management counters */
	wmb();
	mvpp2_rxq_status_update(pp, rxq->id, rx_todo, rx_filled);

	return rx_todo;
}

/* Handle tx fragmentation processing */
static int mvpp2_tx_frag_process(struct mvpp2_port *pp, struct sk_buff *skb,
				 struct mvpp2_tx_queue *aggr_txq,
				 struct mvpp2_tx_queue *txq)
{
	struct mvpp2_txq_pcpu *txq_pcpu = this_cpu_ptr(txq->pcpu);
	struct mvpp2_tx_desc *tx_desc;
	int i;
	dma_addr_t buf_phys_addr;

	for (i = 0; i < skb_shinfo(skb)->nr_frags; i++) {
		skb_frag_t *frag = &skb_shinfo(skb)->frags[i];
		void *addr = page_address(frag->page.p) + frag->page_offset;
		tx_desc = mvpp2_txq_next_desc_get(aggr_txq);
		tx_desc->phys_txq = txq->id;
		tx_desc->data_size = frag->size;

		buf_phys_addr = dma_map_single(pp->dev->dev.parent, addr,
					       tx_desc->data_size,
					       DMA_TO_DEVICE);
		if (dma_mapping_error(pp->dev->dev.parent, buf_phys_addr)) {
			mvpp2_txq_desc_put(txq);
			goto error;
		}

		tx_desc->packet_offset = buf_phys_addr & MVPP2_TX_DESC_ALIGN;
		tx_desc->buf_phys_addr = buf_phys_addr & (~MVPP2_TX_DESC_ALIGN);

		if (i == (skb_shinfo(skb)->nr_frags - 1)) {
			/* Last descriptor */
			tx_desc->command = MVPP2_TXD_L_DESC;
			mvpp2_txq_inc_put(txq_pcpu, skb);
		} else {
			/* Descriptor in the middle: Not First, Not Last */
			tx_desc->command = 0;
			mvpp2_txq_inc_put(txq_pcpu, NULL);
		}
	}

	return 0;

error:
	/* Release all descriptors that were used to map fragments of
	 * this packet, as well as the corresponding DMA mappings
	 */
	for (i = i - 1; i >= 0; i--) {
		tx_desc = txq->descs + i;
		dma_unmap_single(pp->dev->dev.parent,
				 tx_desc->buf_phys_addr,
				 tx_desc->data_size,
				 DMA_TO_DEVICE);
		mvpp2_txq_desc_put(txq);
	}

	return -ENOMEM;
}

/* Main tx processing */
static int mvpp2_tx(struct sk_buff *skb, struct net_device *dev)
{
	struct mvpp2_port *pp = netdev_priv(dev);
	struct mvpp2_tx_queue *txq, *aggr_txq;
	struct mvpp2_txq_pcpu *txq_pcpu;
	struct mvpp2_tx_desc *tx_desc;
	dma_addr_t buf_phys_addr;
	int frags = 0;
	u16 txq_id;
	u32 tx_cmd;

	if (!netif_running(dev))
		goto out;

	txq_id = skb_get_queue_mapping(skb);
	txq = pp->txqs[txq_id];
	txq_pcpu = this_cpu_ptr(txq->pcpu);
	aggr_txq = &pp->pp2->aggr_txqs[smp_processor_id()];

	frags = skb_shinfo(skb)->nr_frags + 1;

	/* Check number of available descriptors */
	if (mvpp2_aggr_desc_num_check(pp->pp2, aggr_txq, frags) ||
	    mvpp2_txq_reserved_desc_num_proc(pp->pp2, txq, txq_pcpu, frags)) {
		frags = 0;
		goto out;
	}

	/* Get a descriptor for the first part of the packet */
	tx_desc = mvpp2_txq_next_desc_get(aggr_txq);
	tx_desc->phys_txq = txq->id;
	tx_desc->data_size = skb_headlen(skb);

	buf_phys_addr = dma_map_single(dev->dev.parent, skb->data,
				       tx_desc->data_size, DMA_TO_DEVICE);
	if (unlikely(dma_mapping_error(dev->dev.parent, buf_phys_addr))) {
		mvpp2_txq_desc_put(txq);
		frags = 0;
		goto out;
	}
	tx_desc->packet_offset = buf_phys_addr & MVPP2_TX_DESC_ALIGN;
	tx_desc->buf_phys_addr = buf_phys_addr & ~MVPP2_TX_DESC_ALIGN;

	tx_cmd = mvpp2_skb_tx_csum(pp, skb);

	if (frags == 1) {
		/* First and Last descriptor */
		tx_cmd |= MVPP2_TXD_F_DESC | MVPP2_TXD_L_DESC;
		tx_desc->command = tx_cmd;
		mvpp2_txq_inc_put(txq_pcpu, skb);
	} else {
		/* First but not Last */
		tx_cmd |= MVPP2_TXD_F_DESC | MVPP2_TXD_PADDING_DISABLE;
		tx_desc->command = tx_cmd;
		mvpp2_txq_inc_put(txq_pcpu, NULL);

		/* Continue with other skb fragments */
		if (mvpp2_tx_frag_process(pp, skb, aggr_txq, txq)) {
			dma_unmap_single(dev->dev.parent,
					 tx_desc->buf_phys_addr,
					 tx_desc->data_size,
					 DMA_TO_DEVICE);
			mvpp2_txq_desc_put(txq);
			frags = 0;
			goto out;
		}
	}

	txq_pcpu->reserved_num -= frags;
	txq_pcpu->count += frags;
	aggr_txq->count += frags;

	/* Enable transmit */
	wmb();
	mvpp2_aggr_txq_pend_desc_add(pp, frags);

	if (txq_pcpu->size - txq_pcpu->count < MAX_SKB_FRAGS + 1) {
		struct netdev_queue *nq = netdev_get_tx_queue(dev, txq_id);
		netif_tx_stop_queue(nq);
	}
out:
	if (frags > 0) {
		struct mvpp2_pcpu_stats *stats = this_cpu_ptr(pp->stats);

		u64_stats_update_begin(&stats->syncp);
		stats->tx_packets++;
		stats->tx_bytes += skb->len;
		u64_stats_update_end(&stats->syncp);
	} else {
		dev->stats.tx_dropped++;
		dev_kfree_skb_any(skb);
	}

	return NETDEV_TX_OK;
}

static inline void mvpp2_cause_error(struct net_device *dev, int cause)
{
	if (cause & MVPP2_CAUSE_FCS_ERR_MASK)
		netdev_err(dev, "FCS error\n");
	if (cause & MVPP2_CAUSE_RX_FIFO_OVERRUN_MASK)
		netdev_err(dev, "rx fifo overrun error\n");
	if (cause & MVPP2_CAUSE_TX_FIFO_UNDERRUN_MASK)
		netdev_err(dev, "tx fifo underrun error\n");
}

static void mvpp2_handle_cpu(void *arg)
{
	struct mvpp2_port *pp = arg;
	u32 cause_rx_tx, cause_tx, cause_misc;

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
	cause_rx_tx = mvpp2_read(pp->pp2, MVPP2_ISR_RX_TX_CAUSE_REG(pp->id));
	cause_tx = cause_rx_tx & MVPP2_CAUSE_TXQ_OCCUP_DESC_ALL_MASK;
	cause_misc = cause_rx_tx & MVPP2_CAUSE_MISC_SUM_MASK;

	if (cause_misc) {
		mvpp2_cause_error(pp->dev, cause_misc);

		/* Clear the cause register */
		mvpp2_write(pp->pp2, MVPP2_ISR_MISC_CAUSE_REG, 0);
		mvpp2_write(pp->pp2, MVPP2_ISR_RX_TX_CAUSE_REG(pp->id),
			    cause_rx_tx & ~MVPP2_CAUSE_MISC_SUM_MASK);
	}

	/* Release TX descriptors */
	if (cause_tx) {
		struct mvpp2_tx_queue *txq = mvpp2_get_tx_queue(pp, cause_tx);
		struct mvpp2_txq_pcpu *txq_pcpu = this_cpu_ptr(txq->pcpu);

		if (!txq)
			return;

		if (txq_pcpu->count)
			mvpp2_txq_done(pp, txq, txq_pcpu);
	}
}

static int mvpp2_poll(struct napi_struct *napi, int budget)
{
	u32 cause_rx_tx, cause_rx;
	int rx_done = 0;
	struct mvpp2_port *pp = netdev_priv(napi->dev);

	if (!netif_running(pp->dev)) {
		napi_complete(napi);
		return rx_done;
	}

	on_each_cpu(mvpp2_handle_cpu, pp, 1);

	cause_rx_tx = mvpp2_read(pp->pp2, MVPP2_ISR_RX_TX_CAUSE_REG(pp->id));
	cause_rx = cause_rx_tx & MVPP2_CAUSE_RXQ_OCCUP_DESC_ALL_MASK;

	/* Process RX packets */
	cause_rx |= pp->pending_cause_rx;
	while (cause_rx && budget > 0) {
		int count;
		struct mvpp2_rx_queue *rxq;

		rxq = mvpp2_get_rx_queue(pp, cause_rx);
		if (!rxq)
			break;

		count = mvpp2_rx(pp, budget, rxq);
		rx_done += count;
		budget -= count;
		if (budget > 0) {
			/* Clear the bit associated to this Rx queue
			 * so that next iteration will continue from
			 * the next Rx queue.
			 */
			cause_rx &= ~(1 << rxq->logic_rxq);
		}
	}

	if (budget > 0) {
		int cpu;

		cause_rx = 0;
		napi_complete(napi);

		/* TODO: Check this wmb() */
		wmb();
		for_each_present_cpu(cpu)
			mvpp2_cpu_interrupts_enable(pp, cpu);
	}
	pp->pending_cause_rx = cause_rx;
	return rx_done;
}

/* Set hw internals when starting port */
static int mvpp2_start_dev(struct mvpp2_port *pp)
{
	int cpu, txp, err;

	mvpp2_gmac_max_rx_size_set(pp);

	/* Allocate the Rx/Tx queues */
	err = mvpp2_setup_rxqs(pp);
	if (err)
		return err;
	err = mvpp2_setup_txqs(pp);
	if (err)
		goto err_cleanup_rxqs;

	for (txp = 0; txp < pp->txp_num; txp++)
		mvpp2_txp_max_tx_size_set(pp, txp);

	/* Initialize pools for swf */
	err = mvpp2_swf_bm_pool_init(pp);
	if (err)
		goto err_cleanup_txqs;

	napi_enable(&pp->napi);

	/* Unmask and enable interrupts on all CPUs */
	on_each_cpu(mvpp2_interrupts_unmask, pp, 1);
	for_each_present_cpu(cpu)
		mvpp2_cpu_interrupts_enable(pp, cpu);

	mvpp2_port_enable(pp);
	phy_start(pp->phy_dev);
	netif_tx_start_all_queues(pp->dev);

	return 0;

err_cleanup_txqs:
	mvpp2_cleanup_txqs(pp);
err_cleanup_rxqs:
	mvpp2_cleanup_rxqs(pp);
	return err;
}

/* Set hw internals when stopping port */
static void mvpp2_stop_dev(struct mvpp2_port *pp)
{
	int cpu;

	/* Stop new packets from arriving to RXQs */
	mvpp2_ingress_enable(pp, false);

	mdelay(10);

	/* Disable and mask interrupts on all CPUs */
	for_each_present_cpu(cpu)
		mvpp2_cpu_interrupts_disable(pp, cpu);
	on_each_cpu(mvpp2_interrupts_mask, pp, 1);

	napi_disable(&pp->napi);

	netif_carrier_off(pp->dev);
	netif_tx_stop_all_queues(pp->dev);

	mvpp2_cleanup_rxqs(pp);
	mvpp2_cleanup_txqs(pp);

	mvpp2_egress_enable(pp, false);
	mvpp2_port_disable(pp);
	phy_stop(pp->phy_dev);
}

/* Return positive if MTU is valid */
static inline int mvpp2_check_mtu_valid(struct net_device *dev, int mtu)
{
	if (mtu < 68) {
		netdev_err(dev, "cannot change mtu to less than 68\n");
		return -EINVAL;
	}

	/* 9676 == 9700 - 20 and rounding to 8 */
	if (mtu > 9676) {
		netdev_info(dev, "illegal MTU value %d, round to 9676\n", mtu);
		mtu = 9676;
	}

	if (!IS_ALIGNED(MVPP2_RX_PKT_SIZE(mtu), 8)) {
		netdev_info(dev, "illegal MTU value %d, round to %d\n", mtu,
			    ALIGN(MVPP2_RX_PKT_SIZE(mtu), 8));
		mtu = ALIGN(MVPP2_RX_PKT_SIZE(mtu), 8);
	}

	return mtu;
}

static void mvpp2_get_mac_address(struct mvpp2_port *pp, unsigned char *addr)
{
	u32 mac_addr_l, mac_addr_m, mac_addr_h;

	mac_addr_l = readl(pp->base + MVPP2_GMAC_CTRL_1_REG);
	mac_addr_m = readl(pp->pp2->lms_base + MVPP2_SRC_ADDR_MIDDLE);
	mac_addr_h = readl(pp->pp2->lms_base + MVPP2_SRC_ADDR_HIGH);
	addr[0] = (mac_addr_h >> 24) & 0xFF;
	addr[1] = (mac_addr_h >> 16) & 0xFF;
	addr[2] = (mac_addr_h >> 8) & 0xFF;
	addr[3] = mac_addr_h & 0xFF;
	addr[4] = mac_addr_m & 0xFF;
	addr[5] = (mac_addr_l >> MVPP2_GMAC_SA_LOW_OFFS) & 0xFF;
}

static int mvpp2_phy_connect(struct mvpp2_port *pp)
{
	struct phy_device *phy_dev;

	phy_dev = of_phy_connect(pp->dev, pp->phy_node, mvpp2_link_event, 0,
				 pp->phy_interface);
	if (!phy_dev) {
		netdev_err(pp->dev, "cannot connect to phy\n");
		return -ENODEV;
	}
	phy_dev->supported &= PHY_GBIT_FEATURES;
	phy_dev->advertising = phy_dev->supported;

	pp->phy_dev = phy_dev;
	pp->link    = 0;
	pp->duplex  = 0;
	pp->speed   = 0;

	return 0;
}

static void mvpp2_phy_disconnect(struct mvpp2_port *pp)
{
	phy_disconnect(pp->phy_dev);
	pp->phy_dev = NULL;
}

/* Start the port */
static int mvpp2_open(struct net_device *dev)
{
	struct mvpp2_port *pp = netdev_priv(dev);
	unsigned char mac_bcast[ETH_ALEN] = { 0xff, 0xff, 0xff,
					      0xff, 0xff, 0xff };
	int err;

	err = mvpp2_prs_mac_da_accept(pp->pp2, pp->id, mac_bcast, true);
	if (err) {
		netdev_err(dev, "mvpp2_prs_mac_da_accept BC failed\n");
		return err;
	}
	err = mvpp2_prs_mac_da_accept(pp->pp2, pp->id, dev->dev_addr, true);
	if (err) {
		netdev_err(dev, "mvpp2_prs_mac_da_accept MC failed\n");
		return err;
	}
	err = mvpp2_prs_tag_mode_set(pp->pp2, pp->id, MVPP2_TAG_TYPE_MH);
	if (err) {
		netdev_err(dev, "mvpp2_prs_tag_mode_set failed\n");
		return err;
	}
	err = mvpp2_prs_def_flow(pp);
	if (err) {
		netdev_err(dev, "mvpp2_prs_def_flow failed\n");
		return err;
	}

	/* Configure Rx packet size according to the current MTU */
	pp->pkt_size = MVPP2_RX_PKT_SIZE(pp->dev->mtu);

	err = request_irq(dev->irq, mvpp2_isr, 0, dev->name, pp);
	if (err) {
		netdev_err(pp->dev, "cannot request irq %d\n", dev->irq);
		return err;
	}

	/* In default link is down */
	netif_carrier_off(pp->dev);

	err = mvpp2_phy_connect(pp);
	if (err < 0)
		goto err_free_irq;

	err = mvpp2_start_dev(pp);
	if (err < 0)
		goto err_phy_disconnect;

	return 0;

err_phy_disconnect:
	mvpp2_phy_disconnect(pp);
err_free_irq:
	free_irq(pp->dev->irq, pp);
	return err;
}

static int mvpp2_stop(struct net_device *dev)
{
	struct mvpp2_port *pp = netdev_priv(dev);

	mvpp2_stop_dev(pp);
	mvpp2_phy_disconnect(pp);
	free_irq(dev->irq, pp);
	return 0;
}

void mvpp2_set_rx_mode(struct net_device *dev)
{
	struct mvpp2_port *pp = netdev_priv(dev);
	struct netdev_hw_addr *ha;

	mvpp2_prs_mac_promisc_set(pp->pp2, pp->id, dev->flags & IFF_PROMISC);
	mvpp2_prs_mac_all_multi_set(pp->pp2, pp->id, dev->flags & IFF_ALLMULTI);

	/* Remove all pp->id's mcast enries */
	mvpp2_prs_mcast_del_all(pp->pp2, pp->id);

	if (dev->flags & IFF_MULTICAST && !netdev_mc_empty(dev))
		netdev_for_each_mc_addr(ha, dev)
			mvpp2_prs_mac_da_accept(pp->pp2, pp->id,
						ha->addr, true);
}

static int mvpp2_set_mac_address(struct net_device *dev, void *p)
{
	struct mvpp2_port *pp = netdev_priv(dev);
	const struct sockaddr *addr = p;
	int err;

	if (!is_valid_ether_addr(addr->sa_data))
		return -EADDRNOTAVAIL;

	if (netif_running(dev))
		return -EBUSY;

	/* Remove old parser entry */
	err = mvpp2_prs_mac_da_accept(pp->pp2, pp->id, dev->dev_addr, false);
	if (err)
		return err;

	/* Add new parser entry */
	err = mvpp2_prs_mac_da_accept(pp->pp2, pp->id, addr->sa_data, true);
	if (err)
		return err;

	/* Set addr in the device */
	memcpy(dev->dev_addr, addr->sa_data, ETH_ALEN);
	return 0;
}

static int mvpp2_change_mtu(struct net_device *dev, int mtu)
{
	struct mvpp2_port *pp = netdev_priv(dev);
	int err;

	mtu = mvpp2_check_mtu_valid(dev, mtu);
	if (mtu < 0) {
		err = mtu;
		goto error;
	}

	if (!netif_running(dev)) {
		err = mvpp2_bm_update_mtu(dev, mtu);
		if (err)
			goto error;
		pp->pkt_size =  MVPP2_RX_PKT_SIZE(mtu);
		mvpp2_gmac_max_rx_size_set(pp);
		return 0;
	}

	mvpp2_stop_dev(pp);

	err = mvpp2_bm_update_mtu(dev, mtu);
	if (err)
		goto error;

	pp->pkt_size =  MVPP2_RX_PKT_SIZE(mtu);

	err = mvpp2_start_dev(pp);
	if (err)
		goto error;
	mvpp2_egress_enable(pp, true);
	mvpp2_ingress_enable(pp, true);

	return 0;

error:
	netdev_err(dev, "fail to change MTU");
	return err;
}

struct rtnl_link_stats64 *mvpp2_get_stats64(struct net_device *dev,
					    struct rtnl_link_stats64 *stats)
{
	struct mvpp2_port *pp = netdev_priv(dev);
	unsigned int start;
	int cpu;

	for_each_possible_cpu(cpu) {
		struct mvpp2_pcpu_stats *cpu_stats;
		u64 rx_packets;
		u64 rx_bytes;
		u64 tx_packets;
		u64 tx_bytes;

		cpu_stats = per_cpu_ptr(pp->stats, cpu);
		do {
			start = u64_stats_fetch_begin_bh(&cpu_stats->syncp);
			rx_packets = cpu_stats->rx_packets;
			rx_bytes   = cpu_stats->rx_bytes;
			tx_packets = cpu_stats->tx_packets;
			tx_bytes   = cpu_stats->tx_bytes;
		} while (u64_stats_fetch_retry_bh(&cpu_stats->syncp, start));

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

/* Ethtool methods */

/* Get settings (phy address, speed) for ethtools */
int mvpp2_ethtool_get_settings(struct net_device *dev, struct ethtool_cmd *cmd)
{
	struct mvpp2_port *pp = netdev_priv(dev);

	if (!pp->phy_dev)
		return -ENODEV;
	return phy_ethtool_gset(pp->phy_dev, cmd);
}

/* Set settings (phy address, speed) for ethtools */
int mvpp2_ethtool_set_settings(struct net_device *dev, struct ethtool_cmd *cmd)
{
	struct mvpp2_port *pp = netdev_priv(dev);

	if (!pp->phy_dev)
		return -ENODEV;
	return phy_ethtool_sset(pp->phy_dev, cmd);
}

/* Set interrupt coalescing for ethtools */
static int mvpp2_ethtool_set_coalesce(struct net_device *dev,
				      struct ethtool_coalesce *c)
{
	struct mvpp2_port *pp = netdev_priv(dev);
	int queue;
	int txp;

	for (queue = 0; queue < rxq_number; queue++) {
		struct mvpp2_rx_queue *rxq = pp->rxqs[queue];
		rxq->time_coal = c->rx_coalesce_usecs;
		rxq->pkts_coal = c->rx_max_coalesced_frames;
		mvpp2_rx_pkts_coal_set(pp, rxq, rxq->pkts_coal);
		mvpp2_rx_time_coal_set(pp, rxq, rxq->time_coal);
	}

	for (txp = 0; txp < pp->txp_num; txp++) {
		for (queue = 0; queue < txq_number; queue++) {
			struct mvpp2_tx_queue *txq =
					     pp->txqs[txp * txq_number + queue];
			txq->done_pkts_coal = c->tx_max_coalesced_frames;
			on_each_cpu(mvpp2_tx_done_pkts_coal_set, txq, 1);
		}
	}

	return 0;
}

/* get coalescing for ethtools */
static int mvpp2_ethtool_get_coalesce(struct net_device *dev,
				      struct ethtool_coalesce *c)
{
	struct mvpp2_port *pp = netdev_priv(dev);
	c->rx_coalesce_usecs        = pp->rxqs[0]->time_coal;
	c->rx_max_coalesced_frames  = pp->rxqs[0]->pkts_coal;
	c->tx_max_coalesced_frames =  pp->txqs[0]->done_pkts_coal;
	return 0;
}

static void mvpp2_ethtool_get_drvinfo(struct net_device *dev,
				      struct ethtool_drvinfo *drvinfo)
{
	strlcpy(drvinfo->driver, MVPP2_DRIVER_NAME,
		sizeof(drvinfo->driver));
	strlcpy(drvinfo->version, MVPP2_DRIVER_VERSION,
		sizeof(drvinfo->version));
	strlcpy(drvinfo->bus_info, dev_name(&dev->dev),
		sizeof(drvinfo->bus_info));
}

static void mvpp2_ethtool_get_ringparam(struct net_device *netdev,
					struct ethtool_ringparam *ring)
{
	struct mvpp2_port *pp = netdev_priv(netdev);

	ring->rx_max_pending = MVPP2_MAX_RXD;
	ring->tx_max_pending = MVPP2_MAX_TXD;
	ring->rx_pending = pp->rx_ring_size;
	ring->tx_pending = pp->tx_ring_size;
}

static int mvpp2_ethtool_set_ringparam(struct net_device *dev,
				       struct ethtool_ringparam *ring)
{
	struct mvpp2_port *pp = netdev_priv(dev);

	if ((ring->rx_pending == 0) || (ring->tx_pending == 0))
		return -EINVAL;
	pp->rx_ring_size = ring->rx_pending < MVPP2_MAX_RXD ?
		ring->rx_pending : MVPP2_MAX_RXD;
	pp->tx_ring_size = ring->tx_pending < MVPP2_MAX_TXD ?
		ring->tx_pending : MVPP2_MAX_TXD;

	if (netif_running(dev)) {
		mvpp2_stop(dev);
		if (mvpp2_open(dev)) {
			netdev_err(dev,
				   "cannot change ring parameter\n");
			return -ENOMEM;
		}
	}

	return 0;
}

/* Device ops */

static const struct net_device_ops mvpp2_netdev_ops = {
	.ndo_open            = mvpp2_open,
	.ndo_stop            = mvpp2_stop,
	.ndo_start_xmit      = mvpp2_tx,
	.ndo_set_rx_mode     = mvpp2_set_rx_mode,
	.ndo_set_mac_address = mvpp2_set_mac_address,
	.ndo_change_mtu      = mvpp2_change_mtu,
	.ndo_get_stats64     = mvpp2_get_stats64,
};

const struct ethtool_ops mvpp2_eth_tool_ops = {
	.get_link       = ethtool_op_get_link,
	.get_settings   = mvpp2_ethtool_get_settings,
	.set_settings   = mvpp2_ethtool_set_settings,
	.set_coalesce   = mvpp2_ethtool_set_coalesce,
	.get_coalesce   = mvpp2_ethtool_get_coalesce,
	.get_drvinfo    = mvpp2_ethtool_get_drvinfo,
	.get_ringparam  = mvpp2_ethtool_get_ringparam,
	.set_ringparam	= mvpp2_ethtool_set_ringparam,
};

/* Driver initialization */

static void mvpp2_port_power_up(struct mvpp2_port *pp)
{
	mvpp2_port_mii_set(pp);
	mvpp2_port_periodic_xon_set(pp, false);
	mvpp2_port_reset_set(pp, false);
}

/* Initialize port HW */
static int mvpp2_port_init(struct mvpp2_port *pp)
{
	struct device *dev = pp->dev->dev.parent;
	struct mvpp2 *pp2 = pp->pp2;
	struct mvpp2_txq_pcpu *txq_pcpu;
	int queue, txp, cpu;

	if (pp->first_rxq + rxq_number > MVPP2_RXQ_TOTAL_NUM)
		return -EINVAL;

	/* Disable port */
	mvpp2_egress_enable(pp, false);
	mvpp2_port_disable(pp);

	pp->txqs = devm_kzalloc(dev, pp->txp_num * txq_number *
				sizeof(struct mvpp2_tx_queue *), GFP_KERNEL);
	if (!pp->txqs)
		return -ENOMEM;

	/* Associate physical Tx queues to this port and initialize.
	 * The mapping is predefined.
	 */
	for (txp = 0; txp < pp->txp_num; txp++) {
		for (queue = 0; queue < txq_number; queue++) {
			int txq_idx = txp * txq_number + queue;
			int queue_phy_id = mvpp2_txq_phys(pp->id, queue);
			struct mvpp2_tx_queue *txq;

			txq = devm_kzalloc(dev, sizeof(*txq), GFP_KERNEL);
			if (!txq)
				return -ENOMEM;

			txq->pcpu = alloc_percpu(struct mvpp2_txq_pcpu);
			if (!txq->pcpu)
				return -ENOMEM;

			txq->id = queue_phy_id;
			txq->log_id = queue;
			txq->txp = txp;
			txq->size = pp->tx_ring_size;
			txq->swf_size = txq->size - (num_present_cpus() *
							  MVPP2_CPU_DESC_CHUNK);
			txq->done_pkts_coal = MVPP2_TXDONE_COAL_PKTS_THRESH;
			for_each_present_cpu(cpu) {
				txq_pcpu = per_cpu_ptr(txq->pcpu, cpu);
				txq_pcpu->cpu = cpu;
				txq_pcpu->size = txq->size;
			}

			txq->pp2 = pp2;
			pp->txqs[txq_idx] = txq;
		}
	}

	pp->rxqs = devm_kzalloc(dev, rxq_number *
				sizeof(struct mvpp2_rx_queue *), GFP_KERNEL);
	if (!pp->rxqs)
		return -ENOMEM;

	/* Allocate and initialize Rx queue for this port */
	for (queue = 0; queue < rxq_number; queue++) {
		struct mvpp2_rx_queue *rxq;
		/* Map physical RXQ to port's logical RXQ */
		rxq = devm_kzalloc(dev, sizeof(*rxq), GFP_KERNEL);
		if (!rxq)
			return -ENOMEM;
		/* Map this Rx queue to a physical queue */
		rxq->id = pp->first_rxq + queue;
		rxq->port = pp->id;
		rxq->logic_rxq = queue;

		pp->rxqs[queue] = rxq;
	}

	/* Configure Rx queue group interrupt for this port */
	mvpp2_write(pp2, MVPP2_ISR_RXQ_GROUP_REG(pp->id), rxq_number);

	/* Create Rx descriptor rings */
	for (queue = 0; queue < rxq_number; queue++) {
		struct mvpp2_rx_queue *rxq = pp->rxqs[queue];
		rxq->size = pp->rx_ring_size;
		rxq->pkts_coal = MVPP2_RX_COAL_PKTS;
		rxq->time_coal = MVPP2_RX_COAL_USEC;
	}

	mvpp2_ingress_enable(pp, false);

	/* Port default configuration */
	mvpp2_defaults_set(pp);

	/* Port's classifier configuration */
	mvpp2_cls_hw_oversize_rxq_set(pp);
	mvpp2_cls_hw_port_def_config(pp, 0, pp->id);

	return 0;
}

/* Ports initialization */
static int mvpp2_port_probe(struct platform_device *pdev,
			    struct device_node *port_node,
			    struct mvpp2 *pp2,
			    int *next_first_rxq)
{
	struct device_node *phy_node;
	struct mvpp2_port *pp;
	struct net_device *dev;
	struct resource *res;
	const char *dt_mac_addr;
	const char *mac_from;
	char hw_mac_addr[ETH_ALEN];
	u32 id;
	int features;
	int phy_mode;
	int pp2_common_regs_num = 2;
	int err, i;

	dev = alloc_etherdev_mqs(sizeof(struct mvpp2_port), txq_number,
				 rxq_number);
	if (!dev)
		return -ENOMEM;

	dev->irq = irq_of_parse_and_map(port_node, 0);
	if (dev->irq == 0) {
		err = -EINVAL;
		goto err_free_netdev;
	}

	phy_node = of_parse_phandle(port_node, "phy", 0);
	if (!phy_node) {
		dev_err(&pdev->dev, "missing phy\n");
		err = -ENODEV;
		goto err_free_irq;
	}

	phy_mode = of_get_phy_mode(port_node);
	if (phy_mode < 0) {
		dev_err(&pdev->dev, "incorrect phy mode\n");
		err = phy_mode;
		goto err_free_irq;
	}

	if (of_property_read_u32(port_node, "port-id", &id)) {
		err = -EINVAL;
		dev_err(&pdev->dev, "missing port-id value\n");
		goto err_free_irq;
	}

	dev->tx_queue_len = MVPP2_MAX_TXD;
	dev->watchdog_timeo = 5 * HZ;
	dev->netdev_ops = &mvpp2_netdev_ops;
	dev->ethtool_ops = &mvpp2_eth_tool_ops;

	pp = netdev_priv(dev);

	if (of_property_read_bool(port_node, "marvell,loopback"))
		pp->flags |= MVPP2_F_LOOPBACK;

	pp->pp2 = pp2;
	pp->id = id;
	pp->txp_num = 1;
	pp->first_rxq = *next_first_rxq;
	pp->phy_node = phy_node;
	pp->phy_interface = phy_mode;

	res = platform_get_resource(pdev, IORESOURCE_MEM,
				    pp2_common_regs_num + id);
	pp->base = devm_ioremap_resource(&pdev->dev, res);
	if (IS_ERR(pp->base)) {
		err = PTR_ERR(pp->base);
		dev_err(&pdev->dev, "cannot obtain port base address\n");
		goto err_free_irq;
	}

	/* Alloc per-cpu stats */
	pp->stats = alloc_percpu(struct mvpp2_pcpu_stats);
	if (!pp->stats) {
		err = -ENOMEM;
		goto err_free_irq;
	}

	dt_mac_addr = of_get_mac_address(port_node);
	if (dt_mac_addr && is_valid_ether_addr(dt_mac_addr)) {
		mac_from = "device tree";
		memcpy(dev->dev_addr, dt_mac_addr, ETH_ALEN);
	} else {
		mvpp2_get_mac_address(pp, hw_mac_addr);
		if (is_valid_ether_addr(hw_mac_addr)) {
			mac_from = "hardware";
			memcpy(dev->dev_addr, hw_mac_addr, ETH_ALEN);
		} else {
			mac_from = "random";
			eth_hw_addr_random(dev);
		}
	}

	pp->tx_ring_size = MVPP2_MAX_TXD;
	pp->rx_ring_size = MVPP2_MAX_RXD;
	pp->dev = dev;
	SET_NETDEV_DEV(dev, &pdev->dev);

	err = mvpp2_port_init(pp);
	if (err < 0) {
		dev_err(&pdev->dev, "failed to init port %d\n", id);
		goto err_free_stats;
	}
	mvpp2_port_power_up(pp);

	netif_napi_add(dev, &pp->napi, mvpp2_poll, NAPI_POLL_WEIGHT);
	features = NETIF_F_SG | NETIF_F_IP_CSUM;
	dev->features = features | NETIF_F_RXCSUM;
	dev->hw_features |= features | NETIF_F_RXCSUM | NETIF_F_GRO;
	dev->vlan_features |= features;

	err = register_netdev(dev);
	if (err < 0) {
		dev_err(&pdev->dev, "failed to register netdev\n");
		goto err_free_txq_pcpu;
	}
	netdev_info(dev, "Using %s mac address %pM\n", mac_from, dev->dev_addr);

	/* Increment the first Rx queue number to be used by the next port */
	*next_first_rxq += rxq_number;
	pp2->port_list[id] = pp;
	return 0;

err_free_txq_pcpu:
	for (i = 0; i < txq_number; i++)
		free_percpu(pp->txqs[i]->pcpu);
err_free_stats:
	free_percpu(pp->stats);
err_free_irq:
	irq_dispose_mapping(dev->irq);
err_free_netdev:
	free_netdev(dev);
	return err;
}

/* Ports removal routine */
static void mvpp2_port_remove(struct mvpp2_port *pp)
{
	int i;

	unregister_netdev(pp->dev);
	free_percpu(pp->stats);
	for (i = 0; i < txq_number; i++)
		free_percpu(pp->txqs[i]->pcpu);
	irq_dispose_mapping(pp->dev->irq);
	free_netdev(pp->dev);
}

/* Initialize decoding windows */
static void mvpp2_conf_mbus_windows(const struct mbus_dram_target_info *dram,
				    struct mvpp2 *pp2)
{
	u32 win_enable;
	int i;

	for (i = 0; i < 6; i++) {
		mvpp2_write(pp2, MVPP2_WIN_BASE(i), 0);
		mvpp2_write(pp2, MVPP2_WIN_SIZE(i), 0);

		if (i < 4)
			mvpp2_write(pp2, MVPP2_WIN_REMAP(i), 0);
	}

	win_enable = 0;

	for (i = 0; i < dram->num_cs; i++) {
		const struct mbus_dram_window *cs = dram->cs + i;
		mvpp2_write(pp2, MVPP2_WIN_BASE(i),
			    (cs->base & 0xffff0000) | (cs->mbus_attr << 8) |
			    dram->mbus_dram_target_id);

		mvpp2_write(pp2, MVPP2_WIN_SIZE(i),
			    (cs->size - 1) & 0xffff0000);

		win_enable |= (1 << i);
	}

	mvpp2_write(pp2, MVPP2_BASE_ADDR_ENABLE, win_enable);
}

/* Initialize Rx FIFO's */
static void mvpp2_rx_fifo_init(struct mvpp2 *pp2)
{
	int port;

	for (port = 0; port < MVPP2_MAX_PORTS; port++) {
		mvpp2_write(pp2, MVPP2_RX_DATA_FIFO_SIZE_REG(port),
			    MVPP2_RX_FIFO_PORT_DATA_SIZE);
		mvpp2_write(pp2, MVPP2_RX_ATTR_FIFO_SIZE_REG(port),
			    MVPP2_RX_FIFO_PORT_ATTR_SIZE);
	}

	mvpp2_write(pp2, MVPP2_RX_MIN_PKT_SIZE_REG, MVPP2_RX_FIFO_PORT_MIN_PKT);
	mvpp2_write(pp2, MVPP2_RX_FIFO_INIT_REG, 0x1);
}

/* Initialize network controller common part HW */
static int mvpp2_init(struct platform_device *pdev, struct mvpp2 *pp2)
{
	const struct mbus_dram_target_info *dram_target_info;
	int err, i;

	/* Checks for hardware constraints */
	if (rxq_number % 4 || (rxq_number > MVPP2_MAX_RXQ) ||
			      (txq_number > MVPP2_MAX_TXQ)) {
		dev_err(&pdev->dev, "invalid queue size parameter\n");
		return -EINVAL;
	}

	/* MBUS windows configuration */
	dram_target_info = mv_mbus_dram_info();
	if (dram_target_info)
		mvpp2_conf_mbus_windows(dram_target_info, pp2);

	/* Allocate and initialize aggregated TXQs */
	pp2->aggr_txqs = devm_kzalloc(&pdev->dev, num_present_cpus() *
				      sizeof(struct mvpp2_tx_queue),
				      GFP_KERNEL);
	if (!pp2->aggr_txqs)
		return -ENOMEM;

	for_each_present_cpu(i) {
		pp2->aggr_txqs[i].id = i;
		pp2->aggr_txqs[i].size = MVPP2_AGGR_TXQ_SIZE;
		err = mvpp2_aggr_txq_init(pdev, &pp2->aggr_txqs[i],
					  MVPP2_AGGR_TXQ_SIZE, i, pp2);
		if (err < 0)
			return err;
	}

	/* Rx Fifo Init */
	mvpp2_rx_fifo_init(pp2);

	/* Reset Rx queue group interrupt configuration */
	for (i = 0; i < MVPP2_MAX_PORTS; i++)
		mvpp2_write(pp2, MVPP2_ISR_RXQ_GROUP_REG(i), rxq_number);

	writel(MVPP2_EXT_GLOBAL_CTRL_DEFAULT,
	       pp2->lms_base + MVPP2_MNG_EXTENDED_GLOBAL_CTRL_REG);

	/* Allow cache snoop when transmiting packets */
	mvpp2_write(pp2, MVPP2_TX_SNOOP_REG, 0x1);

	/* Buffer Manager initialization */
	err = mvpp2_bm_init(pdev, pp2);
	if (err < 0)
		return err;

	/* Parser default initialization */
	err = mvpp2_prs_default_init(pdev, pp2);
	if (err < 0)
		return err;

	/* Classifier default initialization */
	mvpp2_cls_default_init(pp2);

	return 0;
}

static int mvpp2_probe(struct platform_device *pdev)
{
	struct device_node *dn = pdev->dev.of_node;
	struct device_node *port_node;
	struct mvpp2 *pp2;
	struct resource *res;
	int port_count, first_rxq;
	int err;

	pp2 = devm_kzalloc(&pdev->dev, sizeof(struct mvpp2), GFP_KERNEL);
	if (!pp2)
		return -ENOMEM;

	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	pp2->base = devm_ioremap_resource(&pdev->dev, res);
	if (IS_ERR(pp2->base))
		return PTR_ERR(pp2->base);

	res = platform_get_resource(pdev, IORESOURCE_MEM, 1);
	pp2->lms_base = devm_ioremap_resource(&pdev->dev, res);
	if (IS_ERR(pp2->lms_base))
		return PTR_ERR(pp2->lms_base);

	pp2->pp_clk = devm_clk_get(&pdev->dev, "pp_clk");
	if (IS_ERR(pp2->pp_clk))
		return PTR_ERR(pp2->pp_clk);
	err = clk_prepare_enable(pp2->pp_clk);
	if (err < 0)
		return err;

	pp2->gop_clk = devm_clk_get(&pdev->dev, "gop_clk");
	if (IS_ERR(pp2->gop_clk)) {
		err = PTR_ERR(pp2->gop_clk);
		goto err_pp_clk;
	}
	err = clk_prepare_enable(pp2->gop_clk);
	if (err < 0)
		goto err_pp_clk;

	/* Get system's tclk rate */
	pp2->tclk = clk_get_rate(pp2->pp_clk);

	/* Initialize network controller common part HW */
	err = mvpp2_init(pdev, pp2);
	if (err < 0) {
		dev_err(&pdev->dev, "failed to initialize controller\n");
		goto err_gop_clk;
	}

	port_count = of_get_available_child_count(dn);
	if (port_count == 0) {
		dev_err(&pdev->dev, "no ports enabled\n");
		goto err_gop_clk;
	}

	pp2->port_list = devm_kzalloc(&pdev->dev, port_count *
				      sizeof(struct mvpp2_port *),
				      GFP_KERNEL);
	if (!pp2->port_list) {
		err = -ENOMEM;
		goto err_gop_clk;
	}

	/* Initialize ports */
	first_rxq = 0;
	for_each_available_child_of_node(dn, port_node) {
		err = mvpp2_port_probe(pdev, port_node, pp2, &first_rxq);
		if (err < 0)
			goto err_gop_clk;
	}

	platform_set_drvdata(pdev, pp2);
	return 0;

err_gop_clk:
	clk_disable_unprepare(pp2->gop_clk);
err_pp_clk:
	clk_disable_unprepare(pp2->pp_clk);
	return err;
}

static int mvpp2_remove(struct platform_device *pdev)
{
	struct mvpp2 *pp2 = platform_get_drvdata(pdev);
	struct device_node *dn = pdev->dev.of_node;
	struct device_node *port_node;
	int i = 0;

	for_each_available_child_of_node(dn, port_node) {
		if (pp2->port_list[i])
			mvpp2_port_remove(pp2->port_list[i]);
		i++;
	}

	for (i = 0; i < MVPP2_BM_POOLS_NUM; i++) {
		struct mvpp2_bm_pool *bm_pool = &pp2->bm_pools[i];
		mvpp2_bm_pool_destroy(pdev, pp2, bm_pool);
	}

	for_each_present_cpu(i) {
		struct mvpp2_tx_queue *aggr_txq = &pp2->aggr_txqs[i];
		dma_free_coherent(&pdev->dev,
				  MVPP2_AGGR_TXQ_SIZE * MVPP2_DESC_ALIGNED_SIZE,
				  aggr_txq->descs,
				  aggr_txq->descs_phys);
	}

	clk_disable_unprepare(pp2->pp_clk);
	clk_disable_unprepare(pp2->gop_clk);

	return 0;
}

static const struct of_device_id mvpp2_match[] = {
	{ .compatible = "marvell,armada-375-pp2" },
	{ }
};
MODULE_DEVICE_TABLE(of, mvpp2_match);

static struct platform_driver mvpp2_driver = {
	.probe = mvpp2_probe,
	.remove = mvpp2_remove,
	.driver = {
		.name = MVPP2_DRIVER_NAME,
		.of_match_table = mvpp2_match,
	},
};

module_platform_driver(mvpp2_driver);

MODULE_DESCRIPTION("Marvell PPv2 Ethernet Driver - www.marvell.com");
MODULE_AUTHOR("Marcin Wojtas <mw@semihalf.com>");
MODULE_LICENSE("GPL");

module_param(rxq_number, int, S_IRUGO);
module_param(txq_number, int, S_IRUGO);
