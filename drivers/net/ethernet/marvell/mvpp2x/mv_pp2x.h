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

#ifndef _MVPP2_H_
#define _MVPP2_H_
#ifdef ARMADA_390
#include <linux/interrupt.h>
#endif
#include <linux/skbuff.h>
#include <linux/netdevice.h>
#include <linux/string.h>
#include <linux/log2.h>

#include "mv_pp2x_hw_type.h"
#include "mv_gop110_hw_type.h"

#define MVPP2_DRIVER_NAME "mvpp2"
#define MVPP2_DRIVER_VERSION "1.0"

#define PFX			MVPP2_DRIVER_NAME ": "

#define IRQ_NAME_SIZE (36)


#if defined(CONFIG_MV_PP2_FPGA) || defined(CONFIG_MV_PP2_PALLADIUM)
#define CONFIG_MV_PP2_POLLING
#endif

#ifdef CONFIG_MV_PP2_PALLADIUM
#define PALAD(x)	x
#else
#define PALAD(x)
#endif

#ifdef CONFIG_MV_PP2_FPGA
#define FPGA	1
#else
#define FPGA	0
#endif

#if defined(CONFIG_MV_PP2_PALLADIUM)
/*These are the indexes of
 * MVPP2_PRS_FL_IP4_UNTAG_NO_OPV4_OPTIONS/MVPP2_PRS_FL_NON_IP_UNTAG
 * in mv_pp2x_prs_flow_id_array[]
 */
#define MVPP2_PRS_FL_IP4_UNTAG_NO_OPV4_OPTIONS	40
#define MVPP2_PRS_FL_NON_IP_UNTAG_INDEX		50
#endif

#define DBG_MSG(fmt, args...)	printk(fmt, ## args)

#ifdef MVPP2_DEBUG
#define STAT_DBG(c) c
#else
#define STAT_DBG(c)
#endif

#ifdef MV_PP22_GOP_DEBUG
#define GOP_DEBUG(x)	x
#else
#define GOP_DEBUG(x)
#endif

#define MV_ETH_SKB_SHINFO_SIZE	SKB_DATA_ALIGN(sizeof(struct skb_shared_info))

/* START - Taken from mvPp2Commn.h, need to order TODO */
/*--------------------------------------------------------------------*/
/*			PP2 COMMON DEFINETIONS			      */
/*--------------------------------------------------------------------*/

#define MV_ERROR		(-1)
#define MV_OK			(0)

#define WAY_MAX			1

/*--------------------------------------------------------------------*/
/*			PP2 COMMON DEFINETIONS			      */
/*--------------------------------------------------------------------*/
#define NOT_IN_USE					(-1)
#define IN_USE						(1)
#define DWORD_BITS_LEN					32
#define DWORD_BYTES_LEN                                 4
#define RETRIES_EXCEEDED				15000
#define ONE_BIT_MAX					1
#define UNI_MAX						7
#define ETH_PORTS_NUM					7

/*--------------------------------------------------------------------*/
/*			PNC COMMON DEFINETIONS			      */
/*--------------------------------------------------------------------*/

/* HW_BYTE_OFFS
 * return HW byte offset in 4 bytes register
 * _offs_: native offset (LE)
 * LE example: HW_BYTE_OFFS(1) = 1
 * BE example: HW_BYTE_OFFS(1) = 2
 */

#if defined(__LITTLE_ENDIAN)
#define HW_BYTE_OFFS(_offs_) (_offs_)
#else
#define HW_BYTE_OFFS(_offs_) ((3 - ((_offs_) % 4)) + (((_offs_) / 4) * 4))
#endif

#define SRAM_BIT_TO_BYTE(_bit_) HW_BYTE_OFFS((_bit_) / 8)


#define TCAM_DATA_BYTE_OFFS_LE(_offs_)		(((_offs_) - \
	((_offs_) % 2)) * 2 + ((_offs_) % 2))
#define TCAM_DATA_MASK_OFFS_LE(_offs_) (((_offs_) * 2) - ((_offs_) % 2)  + 2)

/* TCAM_DATA_BYTE/MASK
 * tcam data devide into 4 bytes registers
 * each register include 2 bytes of data and 2 bytes of mask
 * the next macros calc data/mask offset in 4 bytes register
 * _offs_: native offset (LE) in data bytes array
 * relevant only for TCAM data bytes
 * used by PRS and CLS2
 */
#define TCAM_DATA_BYTE(_offs_) (HW_BYTE_OFFS(TCAM_DATA_BYTE_OFFS_LE(_offs_)))
#define TCAM_DATA_MASK(_offs_) (HW_BYTE_OFFS(TCAM_DATA_MASK_OFFS_LE(_offs_)))

/*END - Taken from mvPp2Commn.h, need to order TODO */
/*--------------------------------------------------------------------*/

#define __FILENAME__ (strrchr(__FILE__, '/') ? \
	strrchr(__FILE__, '/') + 1 : __FILE__)


#ifdef MVPP2_VERBOSE
#define MVPP2_PRINT_2LINE() \
	pr_info("Passed: %s(%d)\n", __func__, __LINE__)
#define MVPP2_PRINT_LINE() \
	pr_info("Passed: %s(%d)\n", __func__, __LINE__)

#define MVPP2_PRINT_VAR(var) \
	pr_info("%s(%d): "#var"=0x%lx\n", __func__, __LINE__,\
		(unsigned long)var)
#define MVPP2_PRINT_VAR_NAME(var, name) \
	pr_info("%s(%d): %s=0x%lx\n", __func__, __LINE__, name, var)
#else
#define MVPP2_PRINT_LINE()
#define MVPP2_PRINT_2LINE()
#define MVPP2_PRINT_VAR(var)
#define MVPP2_PRINT_VAR_NAME(var, name)
#endif

/* Descriptor ring Macros */
#define MVPP2_QUEUE_NEXT_DESC(q, index) \
	(((index) < (q)->last_desc) ? ((index) + 1) : 0)

#define MVPP2_QUEUE_DESC_PTR(q, index)                 \
	((q)->first_desc + index)

/* Various constants */
#define MVPP2_MAX_SW_THREADS	4
#define MVPP2_MAX_CPUS		4
#define MVPP2_MAX_SHARED	1

/* Coalescing */
#define MVPP2_TXDONE_COAL_PKTS		64
#define MVPP2_TXDONE_HRTIMER_PERIOD_NS	1000000UL
#define MVPP2_TXDONE_COAL_USEC		0 /* No tx_time_coalescing */

#define MVPP2_RX_COAL_PKTS		32
#define MVPP2_RX_COAL_USEC		64

/* BM constants */
#define MVPP2_BM_POOLS_NUM		16
#define MVPP2_BM_POOLS_MAX_ALLOC_NUM	4 /* Max num of allowed BM pools allocations*/
#define MVPP2_BM_POOL_SIZE_MAX		(16 * 1024 - \
					MVPP2_BM_POOL_PTR_ALIGN / 4)
#define MVPP2_BM_POOL_PTR_ALIGN		128

#ifdef CONFIG_MV_PP2_PALLADIUM
#define MVPP2_BM_SHORT_BUF_NUM		256
#define MVPP2_BM_LONG_BUF_NUM		256
#define MVPP2_BM_JUMBO_BUF_NUM		256
#else
#define MVPP2_BM_SHORT_BUF_NUM		2048
#define MVPP2_BM_LONG_BUF_NUM		1024
#define MVPP2_BM_JUMBO_BUF_NUM		512
#endif

#define MVPP2_ALL_BUFS			0

#define RX_TOTAL_SIZE(buf_size)		((buf_size) + MV_ETH_SKB_SHINFO_SIZE)
#define RX_TRUE_SIZE(total_size)	roundup_pow_of_two(total_size)
extern  u32 debug_param;


/* Convert cpu_id to sw_thread_id */
#define QV_THR_2_CPU(sw_thread_id)	(sw_thread_id - first_addr_space)
#define QV_CPU_2_THR(cpu_id)		(first_addr_space + cpu_id)

#define PPV2_MAX_NUM_IRQ		4

enum mvppv2_version {
	PPV21 = 21,
	PPV22
};

enum mv_pp2x_queue_vector_type {
	MVPP2_SHARED,
	MVPP2_PRIVATE
};

enum mv_pp2x_queue_distribution_mode {
	/* All queues are shared.
	 * PPv2.1: this is the only supported mode.
	 * PPv2.2: Requires (N+1) interrupts. All rx_queues are
	 * configured on the additional interrupt.
	 */
	MVPP2_QDIST_SINGLE_MODE,
	MVPP2_QDIST_MULTI_MODE	/* PPv2.2 only requires N interrupts */
};

enum mv_pp2x_cos_classifier {
	MVPP2_COS_CLS_VLAN, /* CoS based on VLAN pri */
	MVPP2_COS_CLS_DSCP,
	MVPP2_COS_CLS_VLAN_DSCP, /* CoS based on VLAN pri, */
				/*if untagged and IP, then based on DSCP */
	MVPP2_COS_CLS_DSCP_VLAN
};

enum mv_pp2x_rss_nf_udp_mode {
	MVPP2_RSS_NF_UDP_2T,	/* non-frag UDP packet hash value
				* is calculated based on 2T
				*/
	MVPP2_RSS_NF_UDP_5T	/* non-frag UDP packet hash value
				*is calculated based on 5T
				*/
};

struct mv_mac_data {
	u8			gop_index;
	unsigned long		flags;
	/* Whether a PHY is present, and if yes, at which address. */
	int			phy_addr;
	phy_interface_t		phy_mode; /* RXAUI, SGMII, etc. */
	struct phy_device	*phy_dev;
	struct device_node	*phy_node;
	int			link_irq;
	char			irq_name[IRQ_NAME_SIZE];
	bool			force_link;
	unsigned int		autoneg;
	unsigned int		link;
	unsigned int		duplex;
	unsigned int		speed;
};

/* Masks used for pp3_emac flags */
#define MV_EMAC_F_LINK_UP_BIT	0
#define MV_EMAC_F_INIT_BIT	1
#define MV_EMAC_F_SGMII2_5_BIT	2

#define MV_EMAC_F_LINK_UP	(1 << MV_EMAC_F_LINK_UP_BIT)
#define MV_EMAC_F_INIT		(1 << MV_EMAC_F_INIT_BIT)
#define MV_EMAC_F_SGMII2_5	(1 << MV_EMAC_F_SGMII2_5_BIT)

#define MVPP2_NO_LINK_IRQ	0

/* Per-CPU Tx queue control */
struct mv_pp2x_txq_pcpu {
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

	/* Array of transmitted buffers' physical addresses */
	dma_addr_t *tx_buffs;

	/* Index of last TX DMA descriptor that was inserted */
	int txq_put_index;

	/* Index of the TX DMA descriptor to be cleaned up */
	int txq_get_index;
};

struct mv_pp2x_tx_queue {
	/* Physical number of this Tx queue */
	u8 id;

	/* Logical number of this Tx queue */
	u8 log_id;

	/* Number of Tx DMA descriptors in the descriptor ring */
	int size;

	/* Per-CPU control of physical Tx queues */
	struct mv_pp2x_txq_pcpu __percpu *pcpu;

	u32 pkts_coal;

	/* Virtual pointer to address of the Tx DMA descriptors
	* memory_allocation
	*/
	void *desc_mem;

	/* Virtual address of thex Tx DMA descriptors array */
	struct mv_pp2x_tx_desc *first_desc;

	/* DMA address of the Tx DMA descriptors array */
	dma_addr_t descs_phys;

	/* Index of the last Tx DMA descriptor */
	int last_desc;

	/* Index of the next Tx DMA descriptor to process */
	int next_desc_to_proc;
};

struct mv_pp2x_aggr_tx_queue {
	/* Physical number of this Tx queue */
	u8 id;

	/* Number of Tx DMA descriptors in the descriptor ring */
	int size;

	/* Number of currently used Tx DMA descriptor in the descriptor ring */
	int count;

	/* Virtual pointer to address of the Aggr_Tx DMA descriptors
	* memory_allocation
	*/
	void *desc_mem;

	/* Virtual pointer to address of the Aggr_Tx DMA descriptors array */
	struct mv_pp2x_tx_desc *first_desc;

	/* DMA address of the Tx DMA descriptors array */
	dma_addr_t descs_phys;

	/* Index of the last Tx DMA descriptor */
	int last_desc;

	/* Index of the next Tx DMA descriptor to process */
	int next_desc_to_proc;
};

struct mv_pp2x_rx_queue {
	/* RX queue number, in the range 0-31 for physical RXQs */
	u8 id;

	/* Port's logic RXQ number to which physical RXQ is mapped */
	int log_id;

	/* Num of rx descriptors in the rx descriptor ring */
	int size;

	u32 pkts_coal;
	u32 time_coal;

	/* Virtual pointer to address of the Rx DMA descriptors
	* memory_allocation
	*/
	void *desc_mem;

	/* Virtual address of the RX DMA descriptors array */
	struct mv_pp2x_rx_desc *first_desc;

	/* DMA address of the RX DMA descriptors array */
	dma_addr_t descs_phys;

	/* Index of the last RX DMA descriptor */
	int last_desc;

	/* Index of the next RX DMA descriptor to process */
	int next_desc_to_proc;

	/* ID of port to which physical RXQ is mapped */
	int port;

};

struct avanta_lp_gop_hw {
	void __iomem *lms_base;
};

struct mv_mac_unit_desc {
	void __iomem *base;
	u32  obj_size;
};

struct cpn110_gop_hw {
	struct mv_mac_unit_desc gmac;
	struct mv_mac_unit_desc xlg_mac;
	struct mv_mac_unit_desc serdes;
	struct mv_mac_unit_desc xmib;
	struct mv_mac_unit_desc ptp;
	void __iomem *smi_base;
	void __iomem *xsmi_base;
	void __iomem *mspg_base;
	void __iomem *xpcs_base;
	void __iomem *rfu1_base;

#ifdef MV_PP22_GOP_DEBUG
	static struct gop_port_ctrl gop_port_debug[MVCPN110_GOP_MAC_NUM];
#endif
};

struct gop_hw {
	union {
		struct avanta_lp_gop_hw gop_alp;
		struct cpn110_gop_hw gop_110;
	};
};

struct mv_pp2x_hw {
	/* Shared registers' base addresses */
	void __iomem *base;	/* PPV22 base_address as received in
				 *devm_ioremap_resource().
				 */
	void __iomem *lms_base;
	void __iomem *cpu_base[MVPP2_MAX_CPUS];

	struct gop_hw gop;
	/* ppv22_base_address for each CPU.
	 * PPv2.2 - cpu_base[x] = base +
	 * cpu_index[smp_processor_id]*MV_PP2_SPACE_64K,
	 * for non-participating CPU it is NULL.
	 * PPv2.1 cpu_base[x] = base
	 */
	/* Common clocks */
	struct clk *pp_clk;
	struct clk *gop_clk;
	struct clk *gop_core_clk;
	struct clk *mg_clk;
	struct clk *mg_core_clk;

	u32 tclk;

	/* PRS shadow table */
	struct mv_pp2x_prs_shadow *prs_shadow;
	/* PRS auxiliary table for double vlan entries control */
	bool *prs_double_vlans;
	/* CLS shadow info for update in running time */
	struct mv_pp2x_cls_shadow *cls_shadow;
	/* C2 shadow info */
	struct mv_pp2x_c2_shadow *c2_shadow;
};

struct mv_pp2x_cos {
	u8 cos_classifier;	/* CoS based on VLAN or DSCP */
	u8 num_cos_queues;	/* number of queue to do CoS */
	u8 default_cos;		/* Default CoS value for non-IP or non-VLAN */
	u8 reserved;
	u32 pri_map;		/* 32 bits, each nibble maps a cos_value(0~7)
				* to a queue.
				*/
};

struct mv_pp2x_rss {
	u8 rss_mode; /*UDP packet */
	u8 dflt_cpu; /*non-IP packet */
	u8 rss_en;
};

struct mv_pp2x_param_config {
	struct mv_pp2x_cos cos_cfg;
	struct mv_pp2x_rss rss_cfg;
	u8 first_bm_pool;
	bool jumbo_pool; /* pp2 always supports 2 pools :
			 * short=MV_DEF_256, long=MV_DEF_2K.
			 * Param defines option to have additional pool,
			 * jumbo=MV_DEF_10K.
			 */
	u8 first_sw_thread; /* The index of the first PPv2.2
			* sub-address space for this NET_INSTANCE.
			*/
	u8 first_log_rxq; /* The first cos rx queue used in the port */
	u8 cell_index; /* The cell_index of the PPv22
			* (could be 0,1, set according to dtsi)
			*/
	enum mv_pp2x_queue_distribution_mode queue_mode;
	u32 rx_cpu_map; /* The CPU that port bind, each port has a nibble
			* indexed by port_id, nibble value is CPU id
			*/
};

/* Shared Packet Processor resources */
struct mv_pp2x {
	enum mvppv2_version pp2_version; /* Redundant, consider to delete.
					* (prevents extra pointer lookup from
					* mv_pp2x_platform_data)
					*/
	struct	mv_pp2x_hw hw;
	struct mv_pp2x_platform_data *pp2xdata;

	u16 cpu_map; /* Bitmap of the participating cpu's */

	struct mv_pp2x_param_config pp2_cfg;

	/* List of pointers to port structures */
	u16 num_ports;
	struct mv_pp2x_port **port_list;

	/* Aggregated TXQs */
	u16 num_aggr_qs;
	struct mv_pp2x_aggr_tx_queue *aggr_txqs;

	/* BM pools */
	u16 num_pools;
	struct mv_pp2x_bm_pool *bm_pools;

	/* RX flow hash indir'n table, in pp22, the table contains the
	* CPU idx according to weight
	*/
	u32 rx_indir_table[MVPP22_RSS_TBL_LINE_NUM];
};

struct mv_pp2x_pcpu_stats {
	struct	u64_stats_sync syncp;
	u64	rx_packets;
	u64	rx_bytes;
	u64	tx_packets;
	u64	tx_bytes;
};

/* Per-CPU port control */
struct mv_pp2x_port_pcpu {
	struct hrtimer tx_done_timer;
#ifdef CONFIG_MV_PP2_PALLADIUM
	struct timer_list slow_tx_done_timer;
#endif
	bool timer_scheduled;
	/* Tasklet for egress finalization */
	struct tasklet_struct tx_done_tasklet;
};

struct queue_vector {
	unsigned int irq;
	char irq_name[IRQ_NAME_SIZE];
	struct napi_struct napi;
	enum mv_pp2x_queue_vector_type qv_type;
	u16 sw_thread_id; /* address_space index used to
			* retrieve interrupt_cause
			*/
	u16 sw_thread_mask; /* Mask for Interrupt PORT_ENABLE Register */
	u8 first_rx_queue; /* Relative to port */
	u8 num_rx_queues;
	u32 pending_cause_rx; /* mask in absolute port_queues, not relative as
			* in Ethernet Occupied Interrupt Cause (EthOccIC))
			*/
	struct mv_pp2x_port *parent;
};

struct mv_pp2x_port {
	u8 id;

	u8 num_irqs;
	u32 *of_irqs;

	struct mv_pp2x *priv;

	struct mv_mac_data mac_data;
	struct tasklet_struct	link_change_tasklet;

	/* Per-port registers' base address */
	void __iomem *base;

	/* Index of port's first physical RXQ */
	u8 first_rxq;

	/* port's  number of rx_queues */
	u8 num_rx_queues;
	/* port's  number of tx_queues */
	u8 num_tx_queues;

	struct mv_pp2x_rx_queue **rxqs; /*Each Port has up tp 32 rxq_queues.*/
	struct mv_pp2x_tx_queue **txqs;
	struct net_device *dev;

	int pkt_size; /* pkt_size determines which is pool_long:
			* jumbo_pool or regular long_pool.
			*/

	/* Per-CPU port control */
	struct mv_pp2x_port_pcpu __percpu *pcpu;
	/* Flags */
	unsigned long flags;

	u16 tx_ring_size;
	u16 rx_ring_size;

	u32 tx_time_coal;
	struct mv_pp2x_pcpu_stats __percpu *stats;

	struct mv_pp2x_bm_pool *pool_long; /* Pointer to the pool_id
					* (long or jumbo)
					*/
	struct mv_pp2x_bm_pool *pool_short; /* Pointer to the short pool_id */

	u32 num_qvector;
	/* q_vector is the parameter that will be passed to
	 * mv_pp2_isr(int irq, void *dev_id=q_vector)
	 */
	struct queue_vector q_vector[MVPP2_MAX_CPUS + MVPP2_MAX_SHARED];
};

struct pp2x_hw_params {
	u8 desc_queue_addr_shift;
};

struct mv_pp2x_platform_data {
	enum mvppv2_version pp2x_ver;
	u8 pp2x_max_port_rxqs;
	u8 num_port_irq;
	bool multi_addr_space;
	bool interrupt_tx_done;
	bool multi_hw_instance;
	void (*mv_pp2x_rxq_short_pool_set)(struct mv_pp2x_hw *, int, int);
	void (*mv_pp2x_rxq_long_pool_set)(struct mv_pp2x_hw *, int, int);
	void (*mv_pp2x_port_queue_vectors_init)(struct mv_pp2x_port *);
	void (*mv_pp2x_port_isr_rx_group_cfg)(struct mv_pp2x_port *);
	struct pp2x_hw_params hw;
#ifdef CONFIG_64BIT
	uintptr_t skb_base_addr;
	uintptr_t skb_base_mask;
#endif
};

static inline int mv_pp2x_max_check(int value, int limit, char *name)
{
	if ((value < 0) || (value >= limit)) {
		DBG_MSG("%s %d is out of range [0..%d]\n",
			name ? name : "value", value, (limit - 1));
		return 1;
	}
	return 0;
}

static inline struct mv_pp2x_port *mv_pp2x_port_struct_get(struct mv_pp2x *priv,
							   int port)
{
	int i;

	for (i = 0; i < priv->num_ports; i++) {
		if (priv->port_list[i]->id == port)
			return priv->port_list[i];
	}
	return NULL;
}

static inline u8 mv_pp2x_cosval_queue_map(struct mv_pp2x_port *port,
					  u8 cos_value)
{
	int cos_width, cos_mask;

	cos_width = ilog2(roundup_pow_of_two(
			  port->priv->pp2_cfg.cos_cfg.num_cos_queues));
	cos_mask  = (1 << cos_width) - 1;

	return((port->priv->pp2_cfg.cos_cfg.pri_map >>
	       (cos_value * 4)) & cos_mask);
}

static inline u8 mv_pp2x_bound_cpu_first_rxq_calc(struct mv_pp2x_port *port)
{
	u8 cos_width, bind_cpu;

	cos_width = ilog2(roundup_pow_of_two(
			  port->priv->pp2_cfg.cos_cfg.num_cos_queues));
	bind_cpu = (port->priv->pp2_cfg.rx_cpu_map >> (4 * port->id)) & 0xF;

	return(port->first_rxq + (bind_cpu << cos_width));
}

/* Swap RX descriptor to be BE */
static inline void mv_pp21_rx_desc_swap(struct mv_pp2x_rx_desc *rx_desc)
{
	cpu_to_le32s(&rx_desc->status);
	cpu_to_le16s(&rx_desc->rsrvd_parser);
	cpu_to_le16s(&rx_desc->data_size);
	cpu_to_le32s(&rx_desc->u.pp21.buf_phys_addr);
	cpu_to_le32s(&rx_desc->u.pp21.buf_cookie);
	cpu_to_le16s(&rx_desc->u.pp21.rsrvd_gem);
	cpu_to_le16s(&rx_desc->u.pp21.rsrvd_l4csum);
	cpu_to_le16s(&rx_desc->u.pp21.rsrvd_cls_info);
	cpu_to_le32s(&rx_desc->u.pp21.rsrvd_flow_id);
	cpu_to_le32s(&rx_desc->u.pp21.rsrvd_abs);
}

static inline void mv_pp22_rx_desc_swap(struct mv_pp2x_rx_desc *rx_desc)
{
	cpu_to_le32s(&rx_desc->status);
	cpu_to_le16s(&rx_desc->rsrvd_parser);
	cpu_to_le16s(&rx_desc->data_size);
	cpu_to_le16s(&rx_desc->u.pp22.rsrvd_gem);
	cpu_to_le16s(&rx_desc->u.pp22.rsrvd_l4csum);
	cpu_to_le32s(&rx_desc->u.pp22.rsrvd_timestamp);
	cpu_to_le64s(&rx_desc->u.pp22.buf_phys_addr_key_hash);
	cpu_to_le64s(&rx_desc->u.pp22.buf_cookie_bm_qset_cls_info);
}

/* Swap TX descriptor to be BE */
static inline void mv_pp21_tx_desc_swap(struct mv_pp2x_tx_desc *tx_desc)
{
	cpu_to_le32s(&tx_desc->command);
	cpu_to_le16s(&tx_desc->data_size);
	cpu_to_le32s(&tx_desc->u.pp21.buf_phys_addr);
	cpu_to_le32s(&tx_desc->u.pp21.buf_cookie);
	cpu_to_le32s(&tx_desc->u.pp21.rsrvd_hw_cmd[0]);
	cpu_to_le32s(&tx_desc->u.pp21.rsrvd_hw_cmd[1]);
	cpu_to_le32s(&tx_desc->u.pp21.rsrvd_hw_cmd[2]);
	cpu_to_le32s(&tx_desc->u.pp21.rsrvd1);
}

static inline void mv_pp22_tx_desc_swap(struct mv_pp2x_tx_desc *tx_desc)
{
	cpu_to_le32s(&tx_desc->command);
	cpu_to_le16s(&tx_desc->data_size);
	cpu_to_le64s(&tx_desc->u.pp22.rsrvd_hw_cmd1);
	cpu_to_le64s(&tx_desc->u.pp22.buf_phys_addr_hw_cmd2);
	cpu_to_le64s(&tx_desc->u.pp22.buf_cookie_bm_qset_hw_cmd3);
}

struct mv_pp2x_pool_attributes {
	char description[32];
	int pkt_size;
	int buf_num;
};

extern struct mv_pp2x_pool_attributes mv_pp2x_pools[];

#if defined(CONFIG_MV_PP2_FPGA) || defined(CONFIG_MV_PP2_PALLADIUM)
void *mv_pp2x_vfpga_address_get(void);
#endif

void mv_pp2x_bm_bufs_free(struct mv_pp2x *priv, struct mv_pp2x_bm_pool *bm_pool,
			  int buf_num, bool is_skb);
int mv_pp2x_bm_bufs_add(struct mv_pp2x_port *port,
			struct mv_pp2x_bm_pool *bm_pool, int buf_num);
int mv_pp2x_bm_pool_add(struct device *dev, struct mv_pp2x *priv,
			u32 *pool_num, u32 pkt_size);
int mv_pp2x_bm_pool_destroy(struct device *dev, struct mv_pp2x *priv,
			   struct mv_pp2x_bm_pool *bm_pool, bool is_skb);
int mv_pp2x_swf_bm_pool_assign(struct mv_pp2x_port *port, u32 rxq,
			       u32 long_id, u32 short_id);
int mv_pp2x_open(struct net_device *dev);
int mv_pp2x_stop(struct net_device *dev);
void mv_pp2x_txq_inc_put(enum mvppv2_version pp2_ver,
			 struct mv_pp2x_txq_pcpu *txq_pcpu,
			 struct sk_buff *skb,
			 struct mv_pp2x_tx_desc *tx_desc);
int mv_pp2x_check_ringparam_valid(struct net_device *dev,
				  struct ethtool_ringparam *ring);
void mv_pp2x_start_dev(struct mv_pp2x_port *port);
void mv_pp2x_stop_dev(struct mv_pp2x_port *port);
void mv_pp2x_cleanup_rxqs(struct mv_pp2x_port *port);
int mv_pp2x_setup_rxqs(struct mv_pp2x_port *port);
int mv_pp2x_setup_txqs(struct mv_pp2x_port *port);
void mv_pp2x_cleanup_txqs(struct mv_pp2x_port *port);
void mv_pp2x_set_ethtool_ops(struct net_device *netdev);
int mv_pp22_rss_rxfh_indir_set(struct mv_pp2x_port *port);
int mv_pp2x_cos_classifier_set(struct mv_pp2x_port *port,
			       enum mv_pp2x_cos_classifier cos_mode);
int mv_pp2x_cos_classifier_get(struct mv_pp2x_port *port);
int mv_pp2x_cos_pri_map_set(struct mv_pp2x_port *port, int cos_pri_map);
int mv_pp2x_cos_pri_map_get(struct mv_pp2x_port *port);
int mv_pp2x_cos_default_value_set(struct mv_pp2x_port *port, int cos_value);
int mv_pp2x_cos_default_value_get(struct mv_pp2x_port *port);
int mv_pp22_rss_mode_set(struct mv_pp2x_port *port, int rss_mode);
int mv_pp22_rss_default_cpu_set(struct mv_pp2x_port *port, int default_cpu);
int mv_pp2x_txq_reserved_desc_num_proc(struct mv_pp2x *priv,
				       struct mv_pp2x_tx_queue *txq,
				       struct mv_pp2x_txq_pcpu *txq_pcpu,
				       int num);

#endif /*_MVPP2_H_*/

