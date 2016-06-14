/*******************************************************************************
Copyright (C) Marvell International Ltd. and its affiliates

This software file (the "File") is owned and distributed by Marvell
International Ltd. and/or its affiliates ("Marvell") under the following
alternative licensing terms.  Once you have made an election to distribute the
File under one of the following license alternatives, please (i) delete this
introductory statement regarding license alternatives, (ii) delete the two
license alternatives that you have not elected to use and (iii) preserve the
Marvell copyright notice above.


********************************************************************************
Marvell GPL License Option

If you received this File from Marvell, you may opt to use, redistribute and/or
modify this File in accordance with the terms and conditions of the General
Public License Version 2, June 1991 (the "GPL License"), a copy of which is
available along with the File in the license.txt file or by writing to the Free
Software Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 or
on the worldwide web at http://www.gnu.org/licenses/gpl.txt.

THE FILE IS DISTRIBUTED AS-IS, WITHOUT WARRANTY OF ANY KIND, AND THE IMPLIED
WARRANTIES OF MERCHANTABILITY OR FITNESS FOR A PARTICULAR PURPOSE ARE EXPRESSLY
DISCLAIMED.  The GPL License provides additional details about this warranty
disclaimer.
*******************************************************************************/
#include <linux/kernel.h>
#include <linux/version.h>
#include <linux/netdevice.h>
#include <linux/etherdevice.h>
#include <linux/platform_device.h>
#include <linux/skbuff.h>
#include <linux/module.h>
#include <linux/inetdevice.h>
#include <linux/interrupt.h>
#include <linux/mbus.h>
#include <linux/prefetch.h>
#include <asm/setup.h>
#include <net/ip.h>
#include <net/ipv6.h>
#include <linux/list.h>
#include <linux/firmware.h>
#include <linux/of_irq.h>
#ifdef CONFIG_MV_PP3_FPGA
#include <linux/pci.h>
#endif
#include <linux/phy.h>
#include <linux/of_irq.h>
#include <linux/clk.h>
#include <linux/of_address.h>
#include <linux/dma-mapping.h>

#include <net/gnss/mv_nss_defs.h>
#include <net/gnss/mv_nss_metadata.h>
#include <net/gnss/mv_nss_ops.h>
#include "mv_gnss_wrap.h"
#include "common/mv_sw_if.h"
#include "platform/mv_pp3.h"
#include "platform/mv_pp3_config.h"
#include "hmac/mv_hmac.h"
#include "hmac/mv_hmac_bm.h"
#include "emac/mv_emac.h"
#include "emac/mv_emac_regs.h"
#include "cmac/mv_cmac.h"
#include "fw/mv_pp3_fw_msg.h"
#include "fw/mv_fw.h"
#include "bm/mv_bm.h"
#include "mv_netdev.h"
#include "vport/mv_pp3_vport.h"
#include "vport/mv_pp3_vq.h"
#include "mv_netdev_structs.h"
#include "msg/mv_pp3_msg_drv.h"
#include "bm/mv_bm.h"
#include "qm/mv_qm.h"
#ifdef CONFIG_MV_PP3_TM_SUPPORT
#include "tm/mv_tm.h"
#endif

#include "mv_dev_sysfs.h"
#include "mv_dev_vq.h"
#include "mv_ethtool.h"

#ifdef CONFIG_MV_PP3_FPGA
#include "gmac/mv_gmac.h"
#else
#include "gop/mv_gop_if.h"
#include "gop/mv_smi.h"
#include "gop/mv_ptp_regs.h"
#endif

#ifdef CONFIG_MV_PP3_PTP_SERVICE
#include "net_dev/mv_ptp_hook.c"
#endif

static struct mv_nss_if_ops mv_pp3_nss_if_ops = {
	.recv_pause     = mv_pp3_dev_rx_pause,
	.recv_resume    = mv_pp3_dev_rx_resume,
	/* .shutdown  */
};

/* debug parameters */
#ifdef PP3_INTERNAL_DEBUG
static bool internal_debug_en;
static bool debug_stop_rx;

bool mv_pp3_is_internal_debug(void)
{
	return internal_debug_en;
}

int mv_pp3_ctrl_internal_debug_set(int en)
{
	internal_debug_en = (en != 0);
	return 0;
}
#endif /* PP3_INTERNAL_DEBUG */

/* global data */
static int pp3_ports_num;
static struct pp3_dev_priv **pp3_netdev;
static int pp3_netdev_next;

/* ISR related */
static bool mv_pp3_run_hmac_interrupts;
static int mv_pp3_irq_rx_base;
static struct mv_pp3 *pp3_priv;

static const struct net_device_ops mv_pp3_netdev_ops;

/* functions */
static int mv_pp3_tx_done(struct net_device *dev, int tx_todo);
static void mv_pp3_txdone_timer_callback(unsigned long data);
static int mv_pp3_check_mtu_valid(int mtu);
static int mv_pp3_rx(struct net_device *dev, struct pp3_vport *cpu_vp, struct pp3_vq *rx_vq, int budget);

#ifndef CONFIG_MV_PP3_FPGA
static int mv_pp3_affinity_notifier_release(int irq_num);
#endif /* !CONFIG_MV_PP3_FPGA */


#ifdef CONFIG_MV_PP3_SKB_RECYCLE
static bool mv_pp3_skb_recycle = CONFIG_MV_PP3_SKB_RECYCLE_DEF;


int mv_pp3_ctrl_nic_skb_recycle(int en)
{
	mv_pp3_skb_recycle = (en != 0);
	return 0;
}

bool mv_pp3_is_nic_skb_recycle(void)
{
	return mv_pp3_skb_recycle;
}
#else
bool mv_pp3_skb_recycle;
#endif /* CONFIG_MV_PP3_SKB_RECYCLE */

int mv_pp3_dev_num_get(void)
{
	return pp3_ports_num;
}

struct pp3_dev_priv *mv_pp3_dev_priv_get(int i)
{
	if (mv_pp3_max_check(i, pp3_ports_num, "netdev"))
		return NULL;

	if (pp3_netdev == NULL)
		return NULL;

	return  pp3_netdev[i];
}

int mv_pp3_dev_cpu_inuse(struct net_device *dev, int cpu)
{
	struct pp3_dev_priv *dev_priv;
	if (!dev)
		return -1;

	dev_priv = MV_PP3_PRIV(dev);

	if (!dev_priv->cpu_vp[cpu])
		return 0;

	return cpumask_test_cpu(cpu, &dev_priv->rx_cpus);
}

int mv_pp3_dev_rxvq_num_get(struct net_device *dev, int cpu)
{
	struct pp3_dev_priv *dev_priv;

	if (!dev)
		goto err;

	dev_priv = MV_PP3_PRIV(dev);

	if (!dev_priv->cpu_vp[cpu])
		goto err;


	return dev_priv->cpu_vp[cpu]->rx_vqs_num;
err:
	pr_err("%s: function failed\n", __func__);
	return -1;

}
/*
   Get EMAC or external virtual port number and
   return network device if such vport found,
   otherwise return NULL.
*/
struct net_device *mv_pp3_vport_dev_get(int vport)
{
	int i;

	for (i = 0; i < pp3_netdev_next; i++) {
		if (pp3_netdev[i] && pp3_netdev[i]->vport &&
		    (pp3_netdev[i]->vport->vport == vport))
			return pp3_netdev[i]->dev;
	}
	return NULL;
}

bool mv_pp3_dev_is_valid(struct net_device *dev)
{
	int i;

	if (!dev)
		return false;

	for (i = 0; i < pp3_netdev_next; i++) {
		if (pp3_netdev[i] == MV_PP3_PRIV(dev))
			return true;
	}
	return false;
}

struct pp3_dev_priv *mv_pp3_dev_priv_exist_get(struct net_device *dev)
{
	if (mv_pp3_dev_is_valid(dev))
		return MV_PP3_PRIV(dev);

	pr_err("%s in not pp3 device\n", dev->name);
	return NULL;
}

struct pp3_dev_priv *mv_pp3_dev_priv_ready_get(struct net_device *dev)
{
	struct pp3_dev_priv *dev_priv = mv_pp3_dev_priv_exist_get(dev);

	if (!dev_priv)
		return NULL;

	if (!(dev_priv->flags & MV_PP3_F_INIT)) {
		pr_err("%s in not initialized yet\n", dev->name);
		return NULL;
	}
	return dev_priv;
}

static inline struct pp3_dev_priv *mv_pp3_emac_dev_priv_get(int emac_num)
{
	struct pp3_dev_priv *p;
	int i;
	for (i = 0; i < pp3_netdev_next; i++) {
		p = pp3_netdev[i];
		if (p && p->vport && (p->vport->port.emac.emac_num == emac_num))
				return p;
	}
	pr_err("%s: private device for emac port %d not exist\n", __func__, emac_num);
	return NULL;
}

/*---------------------------------------------------------------------------
description:
	Update FW according to port sw structure

return values:
		success: 0
		fail: -1
---------------------------------------------------------------------------*/
static int mv_pp3_dev_fw_update(struct pp3_dev_priv *dev_priv)
{
	int cpu, global_cpu_vp;
	struct pp3_vport *cpu_vp;

	/*set pools*/
	if (mv_pp3_cpu_shared_fw_set_pools(dev_priv->cpu_shared)) {
		pr_err("%s: error initiating pools\n", __func__);
		return -1;
	}

	if (pp3_fw_sync()) {
		pr_err("%s: Error setting pools in FW\n", __func__);
		return -1;
	}

	for_each_possible_cpu(cpu) {
		cpu_vp = dev_priv->cpu_vp[cpu];
		if (!cpu_vp)
			continue;

		if (pp3_cpu_vport_fw_set(cpu_vp)) {
			pr_err("%s:Error, %s cpu_vport:%d cpu:%d set failed\n", __func__,
					dev_priv->dev->name, cpu_vp->vport, cpu);
			return -1;
		}
	}

	if (pp3_fw_sync()) {
		pr_err("%s: Error setting CPU vports in FW\n", __func__);
		return -1;
	}

	if (dev_priv->vport) {
		if (dev_priv->vport->type == MV_PP3_NSS_PORT_ETH) {
			if (pp3_emac_vport_fw_set(dev_priv->vport, dev_priv->dev->dev_addr)) {
				pr_err("%s:Error, %s emac_vport:%d set failed\n", __func__,
					dev_priv->dev->name, dev_priv->vport->vport);
				return -1;
			}
		}

		for_each_possible_cpu(cpu) {
			cpu_vp = dev_priv->cpu_vp[cpu];
			if (!cpu_vp)
				continue;

			if (!cpumask_test_cpu(cpu, &dev_priv->rx_cpus))
				continue;

			global_cpu_vp = MV_PP3_CPU_VPORT_ID(cpu);

			/* map EMAC/WLAN virtual port to CPU virtual port */
			if (pp3_fw_cpu_vport_map(dev_priv->vport->vport,
				global_cpu_vp, cpu_vp->vport) < 0) {
				pr_err("%s:Error, emac_vport %d map failed\n", __func__, dev_priv->vport->vport);
				return -1;
			}
		}
	}

	return pp3_fw_sync();
}
/*---------------------------------------------------------------------------*/

void mv_pp3_config_show(void)
{
	struct mv_pp3_version *drv_ver;

	drv_ver = mv_pp3_get_driver_version();
	pr_info("\nmv_pp3 driver version: \t%s:%d.%d.%d.%d",
			drv_ver->name, drv_ver->major_x, drv_ver->minor_y, drv_ver->local_z, drv_ver->debug_d);

	if (pp3_priv)
		pr_info("  o %d Network interfaces supported\n", mv_pp3_ports_num_get(pp3_priv));

	pr_info("  o %d PPCs num supported\n", mv_pp3_fw_ppc_num_get());

	pr_info("  o Cache coherency mode: %s\n", coherency_hard_mode ? "HW" : "SW");

#ifdef CONFIG_MV_PP3_STAT_ERR
	pr_info("  o ERROR statistics enabled\n");
#endif

#ifdef CONFIG_MV_PP3_STAT_INF
	pr_info("  o INFO statistics enabled\n");
#endif

#ifdef CONFIG_MV_PP3_STAT_DBG
	pr_info("  o DEBUG statistics enabled\n");
#endif

#ifdef CONFIG_MV_PP3_DEBUG_CODE
	pr_info("  o Debug messages enabled\n");
#endif

#ifdef PP3_INTERNAL_DEBUG
	pr_info("  o Internal DEBUG mode (%s)\n",  mv_pp3_is_internal_debug() ? "Enabled" : "Disabled");
#endif

#ifdef CONFIG_MV_PP3_SKB_RECYCLE
	pr_info("  o NIC SKB recycle supported (%s)\n", mv_pp3_skb_recycle ? "Enabled" : "Disabled");
#endif

}

static inline struct sk_buff *mv_pp3_skb_alloc(struct net_device *dev, int pkt_size, gfp_t gfp_mask,
										unsigned long *phys_addr)
{
	static struct device *pdev;
	static struct sk_buff *skb;

	skb = mv_pp3_gnss_skb_alloc(dev, pkt_size, gfp_mask);

	if (!skb)
		return NULL;

	/* if network device is unknown used shared_pdev for cache operations */
	if (dev)
		pdev = dev->dev.parent;
	else
		pdev = &pp3_priv->pdev->dev;

	if (phys_addr)
		*phys_addr = mv_pp3_os_dma_map_single(pdev, skb->head, pkt_size + skb_headroom(skb), DMA_FROM_DEVICE);

	return skb;
}
/*---------------------------------------------------------------------------*/
static void mv_pp3_skb_free(struct net_device *dev, struct sk_buff *skb)
{
	mv_pp3_gnss_skb_free(dev, skb);
}
/*---------------------------------------------------------------------------*/
/* Allocate RX buffer */
static inline void *mv_pp3_pool_buff_alloc(struct net_device *dev, struct pp3_pool *ppool, gfp_t gfp_mask,
					unsigned long *phys_addr)
{
	static struct sk_buff *skb;

	skb = mv_pp3_skb_alloc(dev, ppool->pkt_max_size, gfp_mask, phys_addr);

	if (!skb) {
		STAT_ERR(PPOOL_STATS(ppool, smp_processor_id())->buff_alloc_err++);
		pr_err("can't allocate %d bytes buffer for pool #%d\n",
			ppool->buf_size, ppool->pool);
		return NULL;
	}
	/* TODO inc atomic */
	STAT_DBG(PPOOL_STATS(ppool, smp_processor_id())->buff_alloc++);
	return skb;
}
/*---------------------------------------------------------------------------*/
/* Free buffer */
static inline void mv_pp3_pool_buff_free(struct net_device *dev, struct pp3_pool *ppool, void *virt)
{
	mv_pp3_skb_free(dev, (struct sk_buff *)virt);

	/* TODO inc atomic */
	STAT_DBG(PPOOL_STATS(ppool, smp_processor_id())->buff_free++);
}
/*---------------------------------------------------------------------------*/
/* Allocate new buffer and push it to bm pool */
static inline int mv_pp3_pool_refill(struct net_device *dev, struct pp3_pool *ppool,
					gfp_t gfp_mask,	int buf_num)
{
	void *virt;
	unsigned long phys_addr = 0;
	unsigned long flags  = 0;
	int i, extra = 0;


	for (i = 0; i < (buf_num + extra); i++) {
		virt = mv_pp3_pool_buff_alloc(dev, ppool, gfp_mask, &phys_addr);
		if (virt == NULL)
			break;

		MV_LIGHT_LOCK(flags);
		if (mv_pp3_pool_buff_put(ppool->pool, virt, phys_addr)) {
			mv_pp3_pool_buff_free(dev, ppool, virt);
			MV_LIGHT_UNLOCK(flags);
			break;
		}
		MV_LIGHT_UNLOCK(flags);
	}

	return i;
}

/* Flush cache of skb that doesn't copied to CFH */
static inline dma_addr_t mv_pp3_skb_cache_flush(struct net_device *dev, struct sk_buff *skb, int offset)
{
	if (skb->len > offset)
		mv_pp3_os_dma_map_single(dev->dev.parent, skb->data + offset, skb->len - offset, DMA_TO_DEVICE);

	return virt_to_phys(skb->head);
}
/*---------------------------------------------------------------------------*/

void mv_pp3_rx_time_coal_set(struct net_device *dev, int usec)
{
	struct pp3_dev_priv *dev_priv = MV_PP3_PRIV(dev);
	int cpu;

	for_each_possible_cpu(cpu) {
		if (!dev_priv->cpu_vp[cpu])
			continue;

		if (!cpumask_test_cpu(cpu, &dev_priv->rx_cpus))
			continue;

		mv_pp3_cpu_vport_rx_time_coal_set(dev_priv->cpu_vp[cpu], usec);
	}
	dev_priv->rx_time_coal = usec;
}
/*---------------------------------------------------------------------------*/

int mv_pp3_rx_time_coal_get(struct net_device *dev, int *usec)
{
	struct pp3_dev_priv *dev_priv = MV_PP3_PRIV(dev);

	if (!usec)
		goto err;
	if (!dev_priv) {
		pr_err("%s: Interface %s is not initialized\n", __func__, dev->name);
		goto err;
	}

	*usec = dev_priv->rx_time_coal;
	return 0;
err:
	pr_err("%s: function failed\n", __func__);
	return -1;
}
/*---------------------------------------------------------------------------*/

int mv_pp3_rx_pkt_coal_set(struct net_device *dev, int pkts_num)
{
	int cpu;
	struct pp3_vport *cpu_vp;
	struct pp3_dev_priv *dev_priv = MV_PP3_PRIV(dev);

	if (!dev_priv) {
		pr_err("%s: Interface %s is not initialized\n", __func__, dev->name);
		goto err;
	}
	for_each_possible_cpu(cpu) {
		cpu_vp = dev_priv->cpu_vp[cpu];
		if (!cpu_vp)
			continue;

		if (!cpumask_test_cpu(cpu, &dev_priv->rx_cpus))
			continue;

		if (mv_pp3_cpu_vport_rx_pkt_coal_set(cpu_vp, pkts_num))
			goto err;
	}
	dev_priv->rx_pkt_coal = pkts_num;
	return 0;
err:
	pr_err("%s: function failed\n", __func__);
	return -1;
}
/*---------------------------------------------------------------------------*/

int mv_pp3_rx_pkt_coal_get(struct net_device *dev, int *pkts_num)
{
	struct pp3_dev_priv *dev_priv = MV_PP3_PRIV(dev);

	if (!pkts_num)
		goto err;

	if (!dev_priv) {
		pr_err("%s: Interface %s is not initialized\n", __func__, dev->name);
		goto err;
	}

	*pkts_num = dev_priv->rx_pkt_coal;
	return 0;
err:
	pr_err("%s: function failed\n", __func__);
	return -1;
}
/*---------------------------------------------------------------------------*/

int mv_pp3_txdone_pkt_coal_set(struct net_device *dev, int pkts_num)
{
	struct pp3_dev_priv *dev_priv = MV_PP3_PRIV(dev);

	if (!dev_priv) {
		pr_err("%s: Interface %s is not initialized\n", __func__, dev->name);
		return -1;
	}

	dev_priv->tx_done_pkt_coal = MV_ALIGN_DOWN(pkts_num, MV_PP3_BUF_REQUEST_SIZE);

	return 0;
}
/*---------------------------------------------------------------------------*/

int mv_pp3_txdone_pkt_coal_get(struct net_device *dev, int *pkts_num)
{
	struct pp3_dev_priv *dev_priv = MV_PP3_PRIV(dev);

	if (!pkts_num)
		goto err;

	if (!dev_priv) {
		pr_err("%s: Interface %s is not initialized\n", __func__, dev->name);
		goto err;
	}

	*pkts_num = dev_priv->tx_done_pkt_coal;

	return 0;
err:
	pr_err("%s: function failed\n", __func__);
	return -1;

}

/*---------------------------------------------------------------------------*/

int mv_pp3_txdone_time_coal_set(struct net_device *dev, unsigned int usec)
{
	int cpu;
	struct pp3_vport *cpu_vp;
	struct pp3_dev_priv *dev_priv = MV_PP3_PRIV(dev);

	if (!dev_priv) {
		pr_err("%s: Interface %s is not initialized\n", __func__, dev->name);
		goto err;
	}

	for_each_possible_cpu(cpu) {
		cpu_vp = dev_priv->cpu_vp[cpu];
		if (!cpu_vp)
			continue;

		if (mv_pp3_timer_usec_set(&cpu_vp->port.cpu.txdone_timer, usec) < 0)
			goto err;
	}

	dev_priv->tx_done_time_coal = usec;
	return 0;
err:
	pr_err("%s: function failed\n", __func__);
	return -1;
}
/*---------------------------------------------------------------------------*/

int mv_pp3_txdone_time_coal_get(struct net_device *dev, unsigned int *usec)
{
	struct pp3_dev_priv *dev_priv = MV_PP3_PRIV(dev);

	if (!usec)
		goto err;

	if (!dev_priv) {
		pr_err("%s: Interface %s is not initialized\n", __func__, dev->name);
		goto err;
	}

	*usec = dev_priv->tx_done_time_coal;

	return 0;
err:
	pr_err("%s: function failed\n", __func__);
	return -1;
}

/*---------------------------------------------------------------------------*/

/*---------------------------------------------------------------------------*/
/*				link functions				     */
/*---------------------------------------------------------------------------*/
/* Set net device all virtual ports state in FW to disable */
static void mv_pp3_dev_fw_down(struct pp3_dev_priv *dev_priv)
{
	if (dev_priv->vport) {
		dev_priv->vport->state = false;
		pp3_fw_vport_state_set(dev_priv->vport->vport, 0);
	}
}
/*---------------------------------------------------------------------------*/
/* Set net device all virtual ports state in FW to enable */
static void mv_pp3_dev_fw_up(struct pp3_dev_priv *dev_priv)
{
	if (dev_priv->vport) {
		dev_priv->vport->state = true;
		pp3_fw_vport_state_set(dev_priv->vport->vport, 1);
	}
}
/*---------------------------------------------------------------------------*/
/* Set net device linux link status to up */
static void mv_pp3_dev_up(struct pp3_dev_priv *dev_priv)
{
	netif_carrier_on(dev_priv->dev);
	netif_tx_wake_all_queues(dev_priv->dev);
	set_bit(MV_PP3_F_IF_LINK_UP_BIT, &(dev_priv->flags));

	return;
}
/*---------------------------------------------------------------------------*/
/* Set net device linux link status to doen */
static void mv_pp3_dev_down(struct pp3_dev_priv *dev_priv)
{
	clear_bit(MV_PP3_F_IF_LINK_UP_BIT, &(dev_priv->flags));
	netif_carrier_off(dev_priv->dev);
	netif_tx_stop_all_queues(dev_priv->dev);

	return;
}
/*---------------------------------------------------------------------------*/
static void mv_pp3_dev_link_event(struct pp3_dev_priv *dev_priv)
{
	struct mv_port_link_status link_status;
	int hw_link, sw_link, emac;
	char link_str[100];

	if (dev_priv->vport->type != MV_PP3_NSS_PORT_ETH) {
		pr_err("%s: error, this function relevant only for EMAC virtual ports\n", __func__);
		return;
	}

	emac = dev_priv->vport->port.emac.emac_num;
	mv_pp3_gop_port_link_status(emac, &link_status);
	mv_link_to_str(link_status, link_str);

	sw_link = test_bit(MV_PP3_F_IF_LINK_UP_BIT, &(dev_priv->flags)) ? true : false;
	hw_link = link_status.linkup ? true : false;

	/* Check Link status on ethernet port and update SW, FW and HW relevant components */
	if (hw_link && !sw_link) {
		mv_pp3_dev_up(dev_priv); /* this function set SW link */
		pp3_fw_link_changed(emac, true);
		mv_pp3_emac_rx_enable(emac, true);
		mv_link_to_str(link_status, link_str);
		pr_info("%s %s\n", dev_priv->dev->name, link_str);
		return;
	}
	if (!hw_link && sw_link) {
		mv_pp3_emac_rx_enable(emac, false);
		pp3_fw_link_changed(emac, false);
		mv_pp3_dev_down(dev_priv); /* this function clear SW link */
		mv_link_to_str(link_status, link_str);
		pr_info("%s %s\n", dev_priv->dev->name, link_str);
		return;
	}
}

/*---------------------------------------------------------------------------*/
/* N msec periodic callback for polling					     */
/*---------------------------------------------------------------------------*/
static void mv_pp3_txdone_timer_callback(unsigned long data)
{
	unsigned long flags = 0;
	struct pp3_vport *cpu_vp = (struct pp3_vport *)data;
	struct mv_pp3_timer *pp3_timer = &cpu_vp->port.cpu.txdone_timer;
	int tx_todo, total_free;

	if (cpu_vp->port.cpu.cpu_num != smp_processor_id()) {
		pr_err("timer run on incorrect CPU (%d)\n", smp_processor_id());
		mv_pp3_timer_complete(pp3_timer);
		return;
	}

	tx_todo = cpu_vp->port.cpu.txdone_todo;
	if (tx_todo) {
		MV_LIGHT_LOCK(flags);
		STAT_INFO(cpu_vp->port.cpu.stats.txdone++);
		total_free = mv_pp3_tx_done((struct net_device *)cpu_vp->root, tx_todo);
		cpu_vp->port.cpu.txdone_todo -= total_free;
		MV_LIGHT_UNLOCK(flags);
	}

	mv_pp3_timer_complete(pp3_timer);

	if (cpu_vp->port.cpu.txdone_todo)
		mv_pp3_timer_add(pp3_timer);
}
#ifndef CONFIG_MV_PP3_FPGA
/*---------------------------------------------------------------------------*/
/* mac link change event interrupt handle (IRQ 81 - 84)                      */
/*---------------------------------------------------------------------------*/
static irqreturn_t mv_pp3_link_change_isr(int irq, void *data)
{
	struct pp3_vport *emac_vp = (struct pp3_vport *)data;

	/* mask all events from this mac */
	mv_pp3_gop_port_events_mask(emac_vp->port.emac.emac_num);
	/* read cause register to clear event */
	mv_pp3_gop_port_events_clear(emac_vp->port.emac.emac_num);

	tasklet_schedule(&emac_vp->port.emac.lc_tasklet);

	return IRQ_HANDLED;

}

void mv_pp3_link_change_tasklet(unsigned long data)
{
	struct pp3_dev_priv *dev_priv = (struct pp3_dev_priv *)data;

	mv_pp3_dev_link_event(dev_priv);
	/* Unmask interrupt */
	mv_pp3_gop_port_events_unmask(dev_priv->vport->port.emac.emac_num);
}

/*---------------------------------------------------------------------------*/
/* rx events , group interrupt handle					     */
/*---------------------------------------------------------------------------*/
irqreturn_t mv_pp3_rx_isr(int irq, void *data)
{
	int group, frame;
	struct pp3_vport *cpu_vp = (struct pp3_vport *)data;
	struct pp3_cpu_port *cpu_port = &cpu_vp->port.cpu;

	STAT_INFO(cpu_port->stats.irq++);

	frame = MV_PP3_SW_IRQ_2_HFRAME(irq - mv_pp3_irq_rx_base);
	group = MV_PP3_SW_IRQ_2_GROUP(irq - mv_pp3_irq_rx_base);

	/* TODO: frame, group validation */

	mv_pp3_hmac_group_event_mask(frame, group);

	if (napi_schedule_prep(&cpu_port->napi)) {
		/* schedule NAPI */
		__napi_schedule(&cpu_port->napi);
		STAT_INFO(cpu_port->stats.napi_sched++);
	}
	return IRQ_HANDLED;
}

/*---------------------------------------------------------------------------*/
/*  free group RX and link IRQs						     */
/*---------------------------------------------------------------------------*/
static void mv_pp3_dev_rx_irqs_free(struct pp3_dev_priv *dev_priv)
{
	struct pp3_vport *cpu_vp;
	int cpu;

	if (!dev_priv)
		return;

	for_each_online_cpu(cpu) {
		if (!dev_priv->cpu_vp[cpu])
			continue;

		if (!cpumask_test_cpu(cpu, &dev_priv->rx_cpus))
			continue;

		cpu_vp = dev_priv->cpu_vp[cpu];
		disable_irq(cpu_vp->port.cpu.irq_num);
#ifdef CONFIG_SMP
		mv_pp3_affinity_notifier_release(cpu_vp->port.cpu.irq_num);
#endif
		free_irq(cpu_vp->port.cpu.irq_num, (void *)cpu_vp);
	}
}


/*---------------------------------------------------------------------------*/
/*  free net_device RX and link IRQs					     */
/*---------------------------------------------------------------------------*/

static void mv_pp3_dev_irqs_free(struct pp3_dev_priv *dev_priv)
{
	struct pp3_vport *vp_priv;

	if (!dev_priv || !mv_pp3_run_hmac_interrupts)
		return;

	vp_priv = dev_priv->vport;

	if (vp_priv->type == MV_PP3_NSS_PORT_ETH) {
		disable_irq(vp_priv->port.emac.lc_irq_num);
		free_irq(vp_priv->port.emac.lc_irq_num, (void *)vp_priv);
	}

	/* free rx interrupts */
	mv_pp3_dev_rx_irqs_free(dev_priv);
}

/* Get CPU number for specidfied IRQ number connected to net device */
static int mv_pp3_irq_to_cpu(int irq_num)
{
	int i, j;

	for (i = 0; i < pp3_netdev_next; i++) {

		if (pp3_netdev[i]) {
			for (j = 0 ; j < CONFIG_NR_CPUS; j++) {
				if (pp3_netdev[i]->cpu_vp[j]) {
					if (pp3_netdev[i]->cpu_vp[j]->port.cpu.irq_num == irq_num)
						return pp3_netdev[i]->cpu_vp[j]->port.cpu.cpu_num;
				}
			}
		}
	}
	pr_err("%s: no network device reserved on IRQ %d\n", __func__,  irq_num);

	return -1;
}

/*---------------------------------------------------------------------------*/
/* SMP affinity change notifier                                              */
/*---------------------------------------------------------------------------*/
#ifdef CONFIG_SMP
static void mv_pp3_affinity_notify(struct irq_affinity_notify *notify, const struct cpumask *mask)
{
	const struct cpumask *irq_mask;
	int   cpu_num;

	cpu_num = mv_pp3_irq_to_cpu(notify->irq);

	irq_mask = cpumask_of(cpu_num);
	/* in case affinity not change return */
	if (cpumask_equal(mask, irq_mask))
		return;

	pr_info("SMP affinity change for IRQ %d isn't supported\n", notify->irq);
	/* over role the affinity value */
	if (irq_set_affinity(notify->irq, irq_mask)) {
		pr_err("Failed to set affinity IRQ %d to cpu %d device\n",
			notify->irq, cpu_num);
	}
}

static void mv_pp3_affinity_release(struct kref *ref)
{
	struct irq_affinity_notify *notify = container_of(ref, struct irq_affinity_notify, kref);
	kfree(notify);
}

/**
 * mv_pp3_affinity_notifier - function to register the affinity change notifier
 *
 * This adds an IRQ affinity notifier that will prevent affinity modification.
 *
 */
static int mv_pp3_affinity_notifier(int irq_num)
{

	struct irq_affinity_notify *notify = kzalloc(sizeof(*notify), GFP_KERNEL);
	int rc;

	if (!notify)
		return -ENOMEM;
	notify->notify = mv_pp3_affinity_notify;
	notify->release = mv_pp3_affinity_release;
	rc = irq_set_affinity_notifier(irq_num, notify);
	if (rc)
		kfree(notify);

	return rc;
}


/**
 * mv_pp3_affinity_notifier_init - function to un-register the affinity notifier
 *
 */
static int mv_pp3_affinity_notifier_release(int irq_num)
{

	return irq_set_affinity_notifier(irq_num, NULL);
}
#endif

/*---------------------------------------------------------------------------*/
/*  initialize group RX and link IRQs					     */
/*---------------------------------------------------------------------------*/

static int mv_pp3_dev_rx_irq_init(struct pp3_dev_priv *dev_priv)
{
	struct pp3_vport *cpu_vp;
	int cpu;

	for_each_online_cpu(cpu) {

		if (!dev_priv->cpu_vp[cpu])
			continue;

		if (!cpumask_test_cpu(cpu, &dev_priv->rx_cpus))
			continue;

		cpu_vp = dev_priv->cpu_vp[cpu];
		sprintf(cpu_vp->port.cpu.irq_name, "%s_cpu%d", dev_priv->dev->name, cpu);

		/* connect to ISR */
		if (request_irq(cpu_vp->port.cpu.irq_num, mv_pp3_rx_isr,
			IRQF_SHARED | IRQF_TRIGGER_RISING,
			cpu_vp->port.cpu.irq_name, (void *)cpu_vp)) {

				pr_err("%s: Failed to assign RX IRQ %d\n",
					 dev_priv->dev->name, cpu_vp->port.cpu.irq_num);
				goto err;
		}

		if (irq_set_affinity(cpu_vp->port.cpu.irq_num, cpumask_of(cpu_vp->port.cpu.cpu_ctrl->cpu))) {
			pr_err("%s: Failed to set affinity IRQ %d to cpu %d device\n",
				dev_priv->dev->name, cpu_vp->port.cpu.irq_num, cpu_vp->port.cpu.cpu_ctrl->cpu);
			goto err;
		}

		pr_info("%s: Assign RX IRQ %d to %s device on cpu %d\n",
			dev_priv->dev->name, cpu_vp->port.cpu.irq_num, cpu_vp->port.cpu.irq_name, cpu);

#ifdef CONFIG_SMP
		if (mv_pp3_affinity_notifier(cpu_vp->port.cpu.irq_num)) {
			pr_err("%s: Failed to set affinity Notifier IRQ %d\n",
				dev_priv->dev->name, cpu_vp->port.cpu.irq_num);
			goto err;
		}
#endif
	}

	return 0;
err:
	pr_err("%s: function failed\n", __func__);
	mv_pp3_dev_rx_irqs_free(dev_priv);
	return -1;
}

/*---------------------------------------------------------------------------*/
/*  initialize net_device RX and link IRQs				     */
/*---------------------------------------------------------------------------*/
static int mv_pp3_dev_irqs_init(struct pp3_dev_priv *dev_priv)
{
	struct pp3_vport *vp_priv;

	vp_priv = dev_priv->vport;

	if (vp_priv->type == MV_PP3_NSS_PORT_ETH) {
		sprintf(vp_priv->port.emac.lc_irq_name, "mv_mac%d_link",
						vp_priv->port.emac.emac_num);

		/* connect link change interrupt handler to IRQ */
		if (request_irq(vp_priv->port.emac.lc_irq_num, mv_pp3_link_change_isr,
			IRQF_SHARED, vp_priv->port.emac.lc_irq_name, (void *)vp_priv)) {
				pr_err("%s: Failed to assign link change IRQ (%d)\n",
					dev_priv->dev->name, vp_priv->port.emac.lc_irq_num);
				goto err;
		}

		pr_info("%s: Assign link IRQ %d\n", dev_priv->dev->name, vp_priv->port.emac.lc_irq_num);
	}

	/* connect RX interrupts handlers (one per CPU) to IRQs */
	if (mv_pp3_run_hmac_interrupts)
		if (mv_pp3_dev_rx_irq_init(dev_priv) < 0)
			goto err;

	return 0;

err:
	pr_err("%s: function failed\n", __func__);

	if (vp_priv->type == MV_PP3_NSS_PORT_ETH) {
		disable_irq(vp_priv->port.emac.lc_irq_num);
		free_irq(vp_priv->port.emac.lc_irq_num, (void *)vp_priv);
	}
	return -1;
}
#endif /* !CONFIG_MV_PP3_FPGA */

/*---------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------*/


/* Copy metadata from CFH to skb by words */
static inline int mv_pp3_mdata_copy_to_skb(struct net_device *dev, struct sk_buff *skb, struct mv_cfh_common *cfh)
{
	int i;
	u32 *mdata_src, *pmdata;

	pmdata = mv_pp3_gnss_skb_mdata_get(dev, skb);
	if (pmdata) {
		mdata_src = (((u32 *)cfh) + MV_PP3_CFH_COMMON_WORDS);

		for (i = 0; i < MV_PP3_CFH_MDATA_SIZE / 4; i++)
			pmdata[i] = mdata_src[i];

		return 0;
	}

	return -1;
}
/*---------------------------------------------------------------------------*/
/* Copy metadata from skb to CFH by words */
static inline void mv_pp3_mdata_copy_to_cfh(u32 *pmdata, struct mv_cfh_common *cfh)
{
	int i;
	u32 *mdata_dest;

	mdata_dest = (((u32 *)cfh) + MV_PP3_CFH_COMMON_WORDS);

	for (i = 0; i < MV_PP3_CFH_MDATA_SIZE / 4; i++)
		mdata_dest[i] = pmdata[i];
}
/*---------------------------------------------------------------------------*/

/* Build metadata on CFH directly */
static inline void mv_pp3_mdata_build_on_cfh(u16 port_src, u16 port_dst, u8 cos, struct mv_cfh_common *cfh)
{
	u32 *pmdata;

	/* skip first 32 bytes to meda data*/
	pmdata = (((u32 *)cfh) + MV_PP3_CFH_COMMON_WORDS);

	/* We sould not use cpu_to_be16 here, data swapped by FW */
	((struct mv_nss_metadata *)pmdata)->port_dst = port_dst;
	((struct mv_nss_metadata *)pmdata)->port_src = port_src;
	((struct mv_nss_metadata *)pmdata)->type = 0;
	((struct mv_nss_metadata *)pmdata)->cos = cos;
}
/*---------------------------------------------------------------------------*/

/*---------------------------------------------------------------------------*/
/* call to mv_pp3_rx for group's rxqs					     */
/*---------------------------------------------------------------------------*/
int mv_pp3_poll(struct napi_struct *napi, int budget)
{
	int rx_pkt_done = 0;
	struct pp3_vq *rx_vq;
	struct pp3_dev_priv *dev_priv = MV_PP3_PRIV(napi->dev);
	int cpu = smp_processor_id();
	int i, count;
	struct pp3_vport *cpu_vp = dev_priv->cpu_vp[cpu];

#ifdef CONFIG_MV_PP3_DEBUG_CODE
	if (dev_priv->flags & MV_PP3_F_DBG_POLL) {
		pr_info("%s: poll ENTER: cpu=%d, budget=%d\n",
			napi->dev->name, cpu, budget);
	}
#endif

	STAT_INFO(cpu_vp->port.cpu.stats.napi_enter++);

	cpu_vp->port.cpu.napi_master_array = cpu_vp->port.cpu.napi_next_array;

	/* check all queues belong to current group */
	/* start from high priority queue */
	for (i = cpu_vp->port.cpu.napi_q_num - 1; i >= 0; i--) {
#ifdef PP3_INTERNAL_DEBUG
		if (debug_stop_rx) {
			napi_complete(napi);
			STAT_INFO(cpu_vp->port.cpu.stats.napi_complete++);
			return rx_pkt_done;
		}
#endif
		rx_vq = cpu_vp->rx_vqs[MV_PP3_PROC_RXQ_INDEX_GET(cpu_vp->port.cpu, i)];

		/* process packet in that rx queue */
		count = mv_pp3_rx(napi->dev, cpu_vp, rx_vq, budget);

		rx_pkt_done += count;
		budget -= count;

		if (budget <= 0)
			break;
	}

	if (budget > 0) {
		napi_complete(napi);
		STAT_INFO(cpu_vp->port.cpu.stats.napi_complete++);

		if (mv_pp3_run_hmac_interrupts) {
			int frame = MV_PP3_SW_IRQ_2_HFRAME(cpu_vp->port.cpu.irq_num - mv_pp3_irq_rx_base);
			int irq_group = MV_PP3_SW_IRQ_2_GROUP(cpu_vp->port.cpu.irq_num - mv_pp3_irq_rx_base);
			mv_pp3_hmac_group_event_unmask(frame, irq_group);
		}
	}
#ifdef CONFIG_MV_PP3_DEBUG_CODE
	if (dev_priv->flags & MV_PP3_F_DBG_POLL)
		pr_info("%s: poll EXIT: cpu=%d, budget=%u, pkts_done=%u\n",
			napi->dev->name, cpu, budget, rx_pkt_done);
#endif /* CONFIG_MV_PP3_DEBUG_CODE */
	return rx_pkt_done;
}

/*------------------------------------------------------------------------------*/
/* pp3_pool_bufs_free_internal							*/
/*	Get buf_num buffers from the pool (via HMAC queue in BM mode)		*/
/*      Free the valid (non-zero) buffers					*/
/*	return number of released buffers					*/
/*------------------------------------------------------------------------------*/
static inline int pp3_pool_bufs_free_internal(int buf_num, struct net_device *dev, struct pp3_pool *ppool)
{
	int buffs_req, buffs_free, total_free = 0;
	u32  ph_addr[MV_PP3_BUF_REQUEST_SIZE], vr_addr[MV_PP3_BUF_REQUEST_SIZE], pool_id[MV_PP3_BUF_REQUEST_SIZE];
	int time_out = 0, occ, i;
	struct pp3_cpu	*cpu_ctrl = pp3_cpus[smp_processor_id()];
	int frame = cpu_ctrl->bm_frame;
	int queue = cpu_ctrl->bm_swq;
	bool zero_flag;

	static int time_out_max = 100;

	buffs_req = MV_MIN(buf_num, MV_PP3_BUF_REQUEST_SIZE);

	mv_pp3_hmac_bm_buff_request(frame, queue, ppool->pool, buffs_req);

	while (buf_num > 0) {

		STAT_INFO(PPOOL_STATS(ppool, cpu_ctrl->cpu)->buff_get_request++);

		time_out = 0;
		occ = 0;

		/* Wait for all requested buffers are got */
		while ((time_out++ < time_out_max) && (occ < buffs_req))
			occ = mv_pp3_hmac_rxq_occ_get(frame, queue);

		if (time_out >=  time_out_max) {
			STAT_ERR(PPOOL_STATS(ppool, cpu_ctrl->cpu)->buff_get_timeout_err++);
			pr_err("%s: hmac (%d:%d): timeout error on pool #%d, cpu #%d\n",
				__func__, frame, queue, ppool->pool, smp_processor_id());
			pr_err("\twaiting for %d buffers, received %d\n", buffs_req, occ);
#ifdef PP3_INTERNAL_DEBUG
			debug_stop_rx = true;
#endif
			return -1;
		}

#ifdef CONFIG_MV_PP3_DEBUG_CODE
		if (cpu_ctrl->occ_cur_buf < MV_PP3_DEBUG_BUFFER)
			cpu_ctrl->occ_debug_buf[cpu_ctrl->occ_cur_buf++] = time_out;

		if (cpu_ctrl->debug_txdone_occ < time_out)
			cpu_ctrl->debug_txdone_occ = time_out;
#endif

		mv_pp3_os_cache_io_sync(&pp3_priv->pdev->dev);
		zero_flag = false;

		for (i = 0; i < occ; i++) {

			mv_pp3_hmac_bm_buff_get(frame, queue, &pool_id[i], &ph_addr[i], &vr_addr[i]);

#ifdef CONFIG_MV_PP3_DEBUG_CODE
			if (cpu_ctrl->flags & MV_PP3_CPU_F_DBG_BUF_POP)
				if (vr_addr[i] || ph_addr[i])
					pr_info("cpu #%d pop: pool #%d, phys = 0x%08x, virt = 0x%08x\n",
						cpu_ctrl->cpu, pool_id[i], ph_addr[i], vr_addr[i]);
#endif
			STAT_DBG(PPOOL_STATS(ppool, cpu_ctrl->cpu)->buff_get++);
			if (!ph_addr[i]) {
				STAT_INFO(PPOOL_STATS(ppool, cpu_ctrl->cpu)->buff_get_zero++);
				zero_flag = true;
			}
		}

		mv_pp3_hmac_rxq_occ_set(frame, queue, buffs_req);
		buf_num -= buffs_req;
		buffs_req = MV_MIN(buf_num, MV_PP3_BUF_REQUEST_SIZE);
		if ((buffs_req > 0) && (!zero_flag))
			mv_pp3_hmac_bm_buff_request(frame, queue, ppool->pool, buffs_req);

		buffs_free = 0;
		for (i = 0; i < occ; i++) {
			if (!ph_addr[i])
				continue;

			if (unlikely(!vr_addr[i])) {
				STAT_DBG(PPOOL_STATS(ppool, cpu_ctrl->cpu)->buff_get_dummy++);
				continue;
			}
			mv_pp3_pool_buff_free(dev, ppool, (void *)vr_addr[i]);

			buffs_free++;
		}
		ppool->buf_num -= buffs_free;
		total_free += buffs_free;

		if (zero_flag)
			break;
	}

	return total_free;
}
/*---------------------------------------------------------------------------
Function:
	mv_pp3_tx_done
Description:
	Get and free buffers from Linux (tx_done) pool
return values:
	success - number of released buffers
	failed - return -1
---------------------------------------------------------------------------*/

static inline int mv_pp3_tx_done(struct net_device *dev, int tx_todo)
{
	int total_free = 0;
	struct pp3_dev_priv *dev_priv = MV_PP3_PRIV(dev);

	if (tx_todo) {
		total_free = pp3_pool_bufs_free_internal(tx_todo, dev, dev_priv->cpu_shared->txdone_pool);

#ifdef PP3_INTERNAL_DEBUG
		if (total_free < 0) {
			pr_err("%s: Invalid state, try to release %d. Total_free (%d) cannot be < 0\n",
				__func__, tx_todo, total_free);
			debug_stop_rx = true;
			return -1;
		}
	}
#endif
	return total_free;
}

/*---------------------------------------------------------------------------*/
/* Get SKB dscp */
static inline int mv_pp3_skb_dscp_get(struct net_device *dev, struct sk_buff *skb)
{
	int dscp = 0; /* default dscp */

	if (skb->protocol == htons(ETH_P_IP)) {
		struct iphdr *iph = ip_hdr(skb);

		dscp = TOS_TO_DSCP(iph->tos);
	}
	return dscp;
}
/* Get transmit SKB priority */
static inline int mv_pp3_skb_egress_prio_get(struct net_device *dev, struct sk_buff *skb)
{
	int prio = 0; /* default priority */

	if (skb->protocol == htons(ETH_P_IP)) {
		struct iphdr *iph = ip_hdr(skb);

		prio = DSCP_TO_PRIO(TOS_TO_DSCP(iph->tos));
	}
	return prio;
}

/*---------------------------------------------------------------------------*/
/* Choose TX SWQ per CPU */
#if LINUX_VERSION_CODE < KERNEL_VERSION(3, 14, 0)
static inline u16 mv_pp3_select_txq(struct net_device *dev, struct sk_buff *skb)
#else
static inline u16 mv_pp3_select_txq(struct net_device *dev, struct sk_buff *skb,
					void *accel_priv, select_queue_fallback_t fallback)
#endif
{
	return smp_processor_id();
}

/*---------------------------------------------------------------------------*/
static inline u32 mv_pp3_skb_tx_csum(struct sk_buff *skb, struct pp3_vport *cpu_vp)
{
	int cmd = 0;
	int l3_valid, l4_valid, checksum_help;

	if (skb->ip_summed == CHECKSUM_PARTIAL) {
		struct iphdr *ip4h;
		struct ipv6hdr *ip6h;
		int   ip_hdr_len = 0;
		unsigned char l4_proto;
		u16 protocol;

		if (skb->protocol == htons(ETH_P_8021Q))
			protocol = vlan_eth_hdr(skb)->h_vlan_encapsulated_proto;
		else
			protocol = skb->protocol;

		switch (protocol) {
		case htons(ETH_P_IP):
			/* Calculate IPv4 checksum and L4 checksum */
			ip4h = ip_hdr(skb);
			ip_hdr_len = ip4h->ihl;
			l4_proto = ip4h->protocol;
			break;
		case htons(ETH_P_IPV6):
			/* If not IPv4 - must be ETH_P_IPV6 - Calculate only L4 checksum */
			ip6h = ipv6_hdr(skb);
			/* Read l4_protocol from one of IPv6 extra headers */
			if (skb_network_header_len(skb) > 0)
				ip_hdr_len = (skb_network_header_len(skb) >> 2);
			l4_proto = ip6h->nexthdr;
			break;
		default:
			checksum_help = skb_checksum_help(skb);
			if (checksum_help)
				pr_err("virual port %d: cannot calculate IP TX checksum on packet with 0x%4x protocol\n",
						cpu_vp->vport, protocol);

			STAT_DBG((!checksum_help) ? cpu_vp->port.cpu.stats.tx_csum_sw++ : 0 ;)

			return 0;
		}
		cmd = mv_pp3_cfh_tx_l3_csum_offload(true, skb_network_offset(skb), protocol, ip_hdr_len, &l3_valid);
		cmd |= mv_pp3_cfh_tx_l4_csum_offload(true, l4_proto, &l4_valid);

		if (l3_valid || l4_valid) {
			STAT_DBG(cpu_vp->port.cpu.stats.tx_csum_hw++);
			return cmd;
		}
	}

	STAT_DBG(cpu_vp->port.cpu.stats.tx_csum_sw++);
	return 0;
}

#if 0
/*---------------------------------------------------------------------------*/
/* mv_pp3_skb_pool_get - get the rx pool if skb recycle is enabled,	     */
/*                       and reset skb					     */
/* return val:	rx pool or NULL if recycle not available		     */
/*									     */
/*---------------------------------------------------------------------------*/
static inline struct pp3_pool *mv_pp3_skb_recycle_pool_get(struct sk_buff *skb)
{
	struct pp3_pool *ppool = NULL;


#ifdef CONFIG_MV_PP3_SKB_RECYCLE
	if (mv_pp3_skb_recycle) {
		int bpid = mv_pp3_skb_recycle_bpid_get(skb);

		if (bpid < 0)
			return NULL;

		ppool = pp3_pools[bpid];
		if ((atomic_read(&ppool->in_use) > 0) && skb_is_recycleable(skb, ppool->pkt_max_size)) {
			atomic_dec(&ppool->in_use);
			STAT_DBG(PPOOL_STATS(ppool, smp_processor_id())->buff_recycled_ok++);
		} else {
			STAT_DBG(PPOOL_STATS(ppool, smp_processor_id())->buff_recycled_err++);
			return NULL;
		}
	}
#endif /* CONFIG_MV_PP3_SKB_RECYCLE */

	return ppool;
}

/*---------------------------------------------------------------------------*/
/* PP3 Driver NIC mode build cfhs fragmented skb function */
static inline void mv_pp3_tx_frags(struct sk_buff *skb,
				   struct mv_cfh_common *p_cfh,
				   struct net_device *dev,
				   struct pp3_tx_vq *vq_priv)
{
	int cpu = smp_processor_id();
	skb_frag_t *frag = NULL;
	int h_len = skb_headlen(skb);
	struct pp3_dev_priv *dev_priv = MV_PP3_PRIV(dev);
	struct pp3_group *group = dev_priv->groups[cpu];
	int i;
	unsigned int l3_l4_info;


	/*STAT_DBG(group->stats.tx_sg_fsize[skb_shinfo(skb)->nr_frags]++);*/

	p_cfh->ctrl = MV_CFH_RD_SET(0) | MV_CFH_LEN_SET(MV_PP3_CFH_PKT_SIZE) |
		MV_CFH_MODE_SET(HMAC_CFH) | MV_CFH_PP_MODE_SET(PP_TX_PACKET);
	p_cfh->plen_order = MV_CFH_PKT_LEN_SET(skb->len) | MV_CFH_REORDER_SET(REORD_NEW);
	p_cfh->phys_l = 0;
	p_cfh->marker_l = 0;
	p_cfh->vm_bp = 0;

	p_cfh->tag1 = MV_CFH_HWQ_SET(vq_priv->to_emac_hwq) |
		MV_CFH_ADD_CRC_BIT_SET | MV_CFH_L2_PAD_BIT_SET;

	l3_l4_info = mv_pp3_skb_tx_csum(skb, group);
	if (l3_l4_info) {
		/* QC bit set at cfh word1 only if l3 or l4 checksum are calc by HW*/
		p_cfh->l3_l4_info = l3_l4_info;
		p_cfh->ctrl |= MV_CFH_QC_BIT_SET;
	}


#ifdef CONFIG_MV_PP3_DEBUG_CODE
	if (dev_priv->flags & MV_PP3_F_DBG_SG) {
		pr_cont("%s[(1st):tx-%d|sg_tx-%d|skb:%p|cfh:%p|ipsummed:%d]: ",
			dev->name, group->stats.tx_bytes, group->stats.tx_sg_bytes, skb, p_cfh, skb->ip_summed);
		pp3_dbg_skb_dump(skb);
		pp3_dbg_cfh_hdr_dump(p_cfh);
	}
#endif

	p_cfh++;

	p_cfh->plen_order = MV_CFH_PKT_LEN_SET(h_len) | MV_CFH_REORDER_SET(REORD_NEW);
	p_cfh->ctrl = MV_CFH_RD_SET(0) | MV_CFH_LEN_SET(MV_PP3_CFH_HDR_SIZE) |
		MV_CFH_MODE_SET(HMAC_CFH) | MV_CFH_PP_MODE_SET(PP_TX_PACKET);

	p_cfh->vm_bp = 0;
	p_cfh->marker_l = 0;

	p_cfh->phys_l = mv_pp3_os_dma_map_single(dev->dev.parent, skb->data, h_len, DMA_TO_DEVICE);
	STAT_DBG(group->stats.tx_sg_frags++);

#ifdef CONFIG_MV_PP3_DEBUG_CODE
	if (dev_priv->flags & MV_PP3_F_DBG_SG) {
		pr_cont("%s[sg_tx-%d|hd_len:%d(%p:%d) cfh:%p]: ", dev->name,
			group->stats.tx_sg_bytes, h_len, skb->head, skb_headroom(skb), p_cfh);
		pp3_dbg_cfh_hdr_dump(p_cfh);
		mv_debug_mem_dump((void *)skb->head, h_len + skb_headroom(skb), 1);
	}
#endif

	for (i = 0; i < skb_shinfo(skb)->nr_frags; i++) {
		p_cfh++;

		frag = &skb_shinfo(skb)->frags[i];
		p_cfh->plen_order = MV_CFH_PKT_LEN_SET(frag->size) | MV_CFH_REORDER_SET(REORD_NEW);
		p_cfh->ctrl = MV_CFH_RD_SET(0) | MV_CFH_LEN_SET(MV_PP3_CFH_PKT_SIZE) |
			MV_CFH_MODE_SET(HMAC_CFH) | MV_CFH_PP_MODE_SET(PP_TX_PACKET);

		p_cfh->vm_bp = 0;
		p_cfh->marker_l = 0;

		p_cfh->phys_l = mv_pp3_os_dma_map_page(dev->dev.parent, skb_frag_page(frag), frag->page_offset,
							skb_frag_size(frag), DMA_TO_DEVICE);
		STAT_DBG(group->stats.tx_sg_frags++);

#ifdef CONFIG_MV_PP3_DEBUG_CODE
		if (dev_priv->flags & MV_PP3_F_DBG_SG) {
			pr_cont("%s[sg_tx-%d|fr%d-%d(%p:%d) cfh:%p]: ", dev->name,
				group->stats.tx_sg_bytes, i, skb_frag_size(frag), skb_frag_address(frag),
				frag->page_offset, p_cfh);
			pp3_dbg_cfh_hdr_dump(p_cfh);
			mv_debug_mem_dump(skb_frag_address(frag), skb_frag_size(frag), 1);
		}
#endif
	}

	p_cfh->plen_order |= MV_CFH_LAST_BIT_SET;
	p_cfh->marker_l = (unsigned int)skb;
	p_cfh->vm_bp = MV_CFH_BPID_SET(pp3_cpus[cpu]->nic_txdone_pool->pool);

#ifdef CONFIG_MV_PP3_DEBUG_CODE
	if (dev_priv->flags & (MV_PP3_F_DBG_SG | MV_PP3_F_DBG_TX)) {
			pr_cont("%s:[(lst)tx-%d|sg_tx-%d|cfh:%p|skb:%p|ip_summed:%d]: ", dev->name,
				group->stats.tx_bytes, group->stats.tx_sg_bytes, p_cfh, skb, skb->ip_summed);
			pp3_dbg_cfh_hdr_dump(p_cfh);
	}
#endif

	STAT_DBG(group->stats.tx_sg_bytes += (skb->len - MV_MH_SIZE));
	STAT_DBG(group->stats.tx_sg_pkts++);
}

#endif /* if 0 */


/*---------------------------------------------------------------------------*/
/* PP3 Driver NIC mode get skb function only for case of CFH mode */
static inline struct sk_buff *mv_pp3_skb_get(struct net_device *dev)
{
	/* TODO - support SKB recycle */
	return mv_pp3_skb_alloc(dev, MV_PP3_CFH_PAYLOAD_MAX_SIZE, GFP_ATOMIC, NULL);
}

/*---------------------------------------------------------------------------*/
/* PP3 driver receive function */
static int mv_pp3_rx(struct net_device *dev, struct pp3_vport *cpu_vp, struct pp3_vq *rx_vq, int budget)
{
	struct pp3_swq *rx_swq = rx_vq->swq;
	struct pp3_dev_priv *dev_priv = MV_PP3_PRIV(dev);
	struct pp3_cpu_shared *cpu_shared = cpu_vp->port.cpu.cpu_shared;
	struct mv_cfh_common *cfh;
	struct sk_buff *skb;
	unsigned char *cfh_pdata;
	struct pp3_pool *ppool;
	int occ_dg, num_dg, cpu;
	int wr_offset, cfh_data_len, pkt_len, cfh_len, buf_num, bpid = 0;
	int rx_short = 0, rx_long = 0, rx_unknown = 0, rx_pkt_done = 0, rx_dg_done = 0;

	occ_dg = mv_pp3_hmac_rxq_occ_get(rx_swq->frame_num, rx_swq->swq);
	if (occ_dg == 0)
		return 0;

#ifdef PP3_INTERNAL_DEBUG
	if (occ_dg > rx_swq->cur_size * MV_PP3_CFH_DG_MAX_NUM) {
		debug_stop_rx = true;
		pr_err("%s: bad occupied datagram counter %d received on frame %d, queue %d\n",
				__func__, occ_dg, rx_swq->frame_num, rx_swq->swq);
		return 0;
	}
#endif
	mv_pp3_os_cache_io_sync(dev->dev.parent);

	cpu = cpu_vp->port.cpu.cpu_ctrl->cpu;

#ifdef CONFIG_MV_PP3_DEBUG_CODE
	if (dev_priv->flags & MV_PP3_F_DBG_RX) {
		if (occ_dg)
			pr_info("\n---------- %s [rx-%d]: rxq = %d:%d, cpu = %d, budget = %d, occ_dg = %d\n",
				dev->name, DEV_PRIV_STATS(dev_priv, cpu)->rx_pkt_dev,
				rx_swq->frame_num, rx_swq->swq, cpu, budget, occ_dg);
	}
#endif /* CONFIG_MV_PP3_DEBUG_CODE */

	while ((occ_dg > 0) && (rx_pkt_done < budget)) {

		/* check if queue was paused through RX */
		if (!rx_vq->valid)
			break;

		cfh = (struct mv_cfh_common *)mv_pp3_hmac_rxq_next_cfh(rx_swq->frame_num, rx_swq->swq, &num_dg);
#ifdef PP3_INTERNAL_DEBUG
		if (num_dg == 0) {
			debug_stop_rx = true;
			break;
		}

		if (num_dg > occ_dg) {
			mv_pp3_hmac_rxq_cfh_free(rx_swq->frame_num, rx_swq->swq, num_dg);
			/* only part of CFH is moved to DRAM */
			/* in next interrupt will processed full CFH */
			pr_info("%s: only part of CFH is moved to DRAM (num_dg = %d, occ_dg = %d)\n",
				dev->name, num_dg, occ_dg);

			debug_stop_rx = true;

			break;
		}
#endif
		occ_dg -= num_dg;
		rx_dg_done += num_dg;

		if (!cfh) {
			STAT_INFO(rx_swq->stats.pkts_errors++);
			continue;
		}
		/* Prefetch CFH */
		prefetch(cfh);

#ifdef CONFIG_MV_PP3_DEBUG_CODE
		if (dev_priv->flags & MV_PP3_F_DBG_RX) {
			pr_cont("\n********** %s [rx-%d]: ",
				dev->name, DEV_PRIV_STATS(dev_priv, cpu)->rx_pkt_dev + rx_pkt_done);
			pp3_dbg_cfh_rx_dump(cfh);
		}
#endif

#ifdef PP3_INTERNAL_DEBUG
		if (pp3_dbg_cfh_rx_checker(dev_priv, (u32 *)cfh) < 0) {
			/*debug_stop_rx = true;*/
			break;
		}
#endif
		skb = (struct sk_buff *)cfh->marker_l;
		if (skb) {
			bpid = MV_CFH_BPID_GET(cfh->vm_bp);
			if (cpu_shared->short_pool && (bpid == cpu_shared->short_pool->pool)) {
				ppool = cpu_shared->short_pool;
				rx_short++;
			} else if (bpid == cpu_shared->long_pool->pool) {
				ppool = cpu_shared->long_pool;
				rx_long++;
			} else {
				ppool = NULL;
				rx_unknown++;
			}
		}
		/* write offset in 32 bytes granularity */
		wr_offset = MV_CFH_WR_GET(cfh->ctrl) * MV_CFH_WR_RES;
		cfh_len = MV_CFH_LEN_GET(cfh->ctrl);

		pkt_len = MV_CFH_PKT_LEN_GET(cfh->plen_order);

		pkt_len -= MV_PP3_CFH_MDATA_SIZE;

		/* The following cases are supported:
		*  1. cfh_len is 64 bytes, the whole packet is in DRAM buffer
		*  2. cfh_len is always 128 bytes, packet header is in CFH.
		*/
		if (num_dg == MV_PP3_CFH_PKT_DG_SIZE) {
			/* No packet in CFH. Only Common CFH part (32) + MetaData (32) */
			/* The whole packet is in BM buffer (DRAM) */

			/* prefetch 2 cache lines of packet header for read */
			prefetch(skb->data);
			prefetch(skb->data + cache_line_size());

			cfh_data_len = 0;
			STAT_DBG(cpu_vp->port.cpu.stats.rx_buf_pkt++);

		} else {

			cfh_pdata = (unsigned char *)cfh + MV_PP3_CFH_PKT_SIZE;
			cfh_data_len = MV_MIN(pkt_len, MV_PP3_CFH_PAYLOAD_MAX_SIZE);

			if (pkt_len < MV_PP3_CFH_PAYLOAD_MAX_SIZE) {
				/* The whole packet is located in CFH. DRAM buffer is empty */
				STAT_DBG(cpu_vp->port.cpu.stats.rx_cfh_pkt++);

				skb = mv_pp3_skb_alloc(dev, MV_PP3_CFH_PAYLOAD_MAX_SIZE, GFP_ATOMIC, NULL);
				if (unlikely(!skb)) {
					STAT_ERR(rx_swq->stats.pkts_drop++);
					DEV_PRIV_STATS(dev_priv, cpu)->rx_drop_dev++;
					continue;
				}

			} else {
				int delta;
				/* Packet is split. Header (64 bytes) is in CFH, the rest is in BM buffer (DRAM) */
				STAT_DBG(cpu_vp->port.cpu.stats.rx_split_pkt++);
				/* prefetch 2 cache lines of packet header for write */
				prefetchw(skb->data);
				prefetchw(skb->data + cache_line_size());

				delta = wr_offset - (cfh_data_len + skb_headroom(skb));
				if (delta)
					skb_reserve(skb, delta);
			}

			/* Copy data including MH */
			/* skb->data and cfh_pdata are always aligned 4 bytes */
			memcpy(skb->data, cfh_pdata, cfh_data_len);
		}

#ifdef CONFIG_MV_PP3_PTP_SERVICE
		mv_pp3_is_pkt_ptp_rx_proc(dev_priv, cfh, pkt_len, skb->data, rx_pkt_done);
#endif

		/* RX function processing */
		skb_put(skb, pkt_len);
		skb_pull_inline(skb, MV_MH_SIZE);

		/*copy meta data form CFH to skb buffer */
		mv_pp3_mdata_copy_to_skb(dev, skb, cfh);

#ifdef CONFIG_MV_PP3_DEBUG_CODE
		if (dev_priv->flags & MV_PP3_F_DBG_RX) {
			pr_info("\n");
			pp3_dbg_skb_dump(skb);
			mv_debug_mem_dump((void *)skb->head, skb->len + skb_headroom(skb), 1);
		}
#endif
		skb->ip_summed  = mv_pp3_rx_csum(cpu_vp, cfh, skb) ? CHECKSUM_UNNECESSARY : CHECKSUM_NONE;

		rx_pkt_done++;

		DEV_PRIV_STATS(dev_priv, cpu)->rx_bytes_dev += pkt_len - MV_MH_SIZE;
		STAT_DBG(cpu_vp->port.cpu.stats.rx_netif++);
		/* TODO remove rx_bytes from vport stats, use swq stats */
		STAT_DBG(cpu_vp->port.cpu.stats.rx_bytes += pkt_len - MV_MH_SIZE);
		STAT_DBG(rx_swq->stats.bytes += pkt_len - MV_MH_SIZE);

		if (mv_pp3_gnss_skb_receive(dev, skb))
			STAT_DBG(cpu_vp->port.cpu.stats.rx_netif_drop++);

	} /* while */

	/* refill short and long pools */
	if (rx_short) {
		ppool = cpu_shared->short_pool;
		STAT_DBG(PPOOL_STATS(ppool, cpu)->buff_rx += rx_short);
		buf_num = mv_pp3_pool_refill(dev, ppool, GFP_ATOMIC, rx_short);

#ifdef PP3_INTERNAL_DEBUG
		if (buf_num != rx_short) {
			pr_err("%s: Can't refill buffer to BM pool #%d on cpu #%d\n",
				dev->name, bpid, cpu);
			debug_stop_rx = true;
		}
#endif
	}

	if (rx_long) {
		ppool = cpu_shared->long_pool;
		STAT_DBG(PPOOL_STATS(ppool, cpu)->buff_rx += rx_long);
		buf_num = mv_pp3_pool_refill(dev, ppool, GFP_ATOMIC, rx_long);

#ifdef PP3_INTERNAL_DEBUG
		if (buf_num != rx_long) {
			pr_err("%s: Can't refill buffer to BM pool #%d on cpu #%d\n",
				dev->name, bpid, cpu);
			debug_stop_rx = true;
		}
#endif
	}

	if (rx_unknown) {
		/* Increment counter for packets from unknown pool */
		STAT_DBG(cpu_vp->port.cpu.stats.rx_no_pool += rx_unknown);
	}

	DEV_PRIV_STATS(dev_priv, cpu)->rx_pkt_dev += rx_pkt_done;
	STAT_DBG(rx_swq->stats.pkts += rx_pkt_done);

	if (rx_dg_done > 0)
		mv_pp3_hmac_rxq_occ_set(rx_swq->frame_num, rx_swq->swq, rx_dg_done);

	return rx_pkt_done;
}
/*---------------------------------------------------------------------------*/

int mv_pp3_pool_bufs_add(int buf_num, struct net_device *dev, struct pp3_pool *ppool)
{
	int size, i;

	size = ppool->buf_size;

	if (size == 0) {
		pr_err("%s: invalid pool #%d state: buf_size = %d\n", __func__, ppool->pool, size);
		return -EINVAL;
	}

	i = mv_pp3_pool_refill(dev, ppool, GFP_KERNEL, buf_num);

	if (i != buf_num)
		pr_err("Can't add all required buffers to BM pool #%d on cpu #%d\n",
			ppool->pool, smp_processor_id());

	ppool->buf_num += i;

	ppool->in_use_thresh = ppool->buf_num / 4;

	pr_info("%s pool #%d:  buf_size=%4d - %d of %d buffers added\n",
		mv_pp3_pool_name_get(ppool), ppool->pool, size, i, buf_num);

	return 0;
}
/*---------------------------------------------------------------------------*/

/* Free number of buffers [buf_num] from BM pool [pool] */
int mv_pp3_pool_bufs_free(int buf_num, struct net_device *dev, struct pp3_pool *ppool)
{
	unsigned long flags = 0;
	int cpu = smp_processor_id();
	struct pp3_cpu *cpu_ctrl;
	int free_buf = 0, time_out = 0, buf_num_old;
	int time_out_max = 1000;

	cpu_ctrl = pp3_cpus[cpu];

	if (!cpu_ctrl) {
		pr_err("%s: CPU %d pointer in NULL\n", __func__, cpu);
		return -EINVAL;
	}

	if (!ppool) {
		pr_err("%s: pool pointer in NULL\n", __func__);
		return -EINVAL;
	}

	if (buf_num == 0)
		return 0;


	buf_num_old = ppool->buf_num;

	if (ppool->buf_num < buf_num)
		/* cannot release more bufers than exist in pool */
		buf_num = ppool->buf_num;

	MV_LIGHT_LOCK(flags);
	while ((time_out++ < time_out_max) && (buf_num > 0)) {
		free_buf = pp3_pool_bufs_free_internal(buf_num, dev, ppool);

		if (free_buf < 0) {
#ifdef CONFIG_MV_PP3_DEBUG_CODE
			pr_err("%s: Error, function failed. Try to release %d buffers\n",
				__func__, buf_num);
#endif
			MV_LIGHT_UNLOCK(flags);
			return -1;
		}
		buf_num -= free_buf;
	}
	MV_LIGHT_UNLOCK(flags);

	pr_info("%s pool #%d:  buf_size=%4d - free %d of %d buffers\n",
		mv_pp3_pool_name_get(ppool),
		ppool->pool, ppool->buf_size, buf_num_old - ppool->buf_num, buf_num_old);

	if (time_out >= time_out_max) {
		pr_err("%s: timeout - retries exceeded\n", __func__);
		return -1;
	}

	return 0;
}
/*---------------------------------------------------------------------------*/
static int mv_pp3_set_mac_addr_internals(struct net_device *dev, u8 *mac)
{
	struct pp3_dev_priv *dev_priv = MV_PP3_PRIV(dev);
	int i;

	if (mv_pp3_shared_initialized(pp3_priv)) {
		if (pp3_fw_port_mac_addr(dev_priv->vport->vport, mac) < 0) {
			pr_err("%s: MAC address set command failed\n", __func__);
			return -1;
		}
	}
	/* set addr in the device */
	for (i = 0; i < MV_MAC_ADDR_SIZE; i++)
		dev->dev_addr[i] = mac[i];

	return 0;
}
/*---------------------------------------------------------------------------*/
int mv_pp3_set_mac_addr(struct net_device *dev, void *p)
{
	struct sockaddr *addr = p;
	struct pp3_dev_priv *dev_priv;

	if (!dev || !addr) {
		pr_err("%s: cannot change MAC for device %p", __func__, dev);
		return -1;
	}

	dev_priv = MV_PP3_PRIV(dev);

	if (!is_valid_ether_addr(addr->sa_data))
		return -EADDRNOTAVAIL;

	if (dev_priv->vport->type != MV_PP3_NSS_PORT_ETH) {
		memcpy(dev->dev_addr, addr->sa_data, MV_MAC_ADDR_SIZE);
		return 0;
	}

	if (!netif_running(dev)) {
		if (mv_pp3_set_mac_addr_internals(dev, addr->sa_data) == -1)
			goto error;
	} else {
		if (dev->netdev_ops->ndo_stop(dev)) {
			pr_err("%s: stop interface failed\n", dev->name);
			goto error;
		}

		if (mv_pp3_set_mac_addr_internals(dev, addr->sa_data) == -1)
			goto error;

		if (dev->netdev_ops->ndo_open(dev)) {
			pr_err("%s: start interface failed\n", dev->name);
			goto error;
		}
	}
	return 0;
error:
	pr_err("%s: set mac addr failed\n", dev->name);
	return -1;
}
/*---------------------------------------------------------------------------*/
struct net_device_stats *mv_pp3_get_stats(struct net_device *dev)
{
	struct pp3_netdev_stats *stats;
	struct pp3_dev_priv *dev_priv = MV_PP3_PRIV(dev);
	int cpu;

	u32 tx_pkts, rx_pkts, tx_bytes, rx_bytes, tx_drp, rx_drp, rx_err;


	tx_pkts = rx_pkts = tx_bytes = rx_bytes = tx_drp = rx_drp = rx_err = 0;

	for_each_online_cpu(cpu) {
		stats = per_cpu_ptr(dev_priv->dev_stats, cpu);

		tx_pkts += stats->tx_pkt_dev;
		rx_pkts += stats->rx_pkt_dev;

		tx_bytes += stats->tx_bytes_dev;
		rx_bytes += stats->rx_bytes_dev;

		tx_drp += stats->tx_drop_dev;
		rx_drp += stats->rx_drop_dev;

		rx_err += stats->rx_err_dev;
	}

	dev->stats.tx_packets = tx_pkts;
	dev->stats.rx_packets = rx_pkts;

	dev->stats.tx_bytes = tx_bytes;
	dev->stats.rx_bytes = rx_bytes;

	dev->stats.tx_dropped = tx_drp;
	dev->stats.rx_dropped = rx_drp;

	dev->stats.rx_errors = rx_err;

	return &(dev->stats);
}
/*---------------------------------------------------------------------------*/
static int mv_pp3_proc_mac_mc(struct net_device *dev)
{
	int macs_list_size;
	unsigned char *macs_list;
	struct netdev_hw_addr *ha;
	struct pp3_dev_priv *dev_priv = MV_PP3_PRIV(dev);
	int err, offset = 0;

	macs_list_size = netdev_hw_addr_list_count(&dev->mc);

	/* currently FW support up to three mcast addresses */
	if (macs_list_size >= MV_PP3_MAC_ADDR_NUM) {
		/* in such case last addresses will not passed to FW */
		pr_err("Error: support up to %d mcast address\n", MV_PP3_MAC_ADDR_NUM - 1);
		return -1;
	}

	macs_list = kzalloc(macs_list_size * MV_MAC_ADDR_SIZE, GFP_ATOMIC);

	if (!macs_list) {
		pr_err("%s: out of memory\n", __func__);
		return -ENOMEM;
	}

	netdev_for_each_mc_addr(ha, dev) {
		memcpy(&macs_list[offset], ha->addr, MV_MAC_ADDR_SIZE);
/*
		pr_info("%02x:%02x:%02x:%02x:%02x:%02x\n",
			macs_list[offset + 0], macs_list[offset + 1], macs_list[offset + 2],
			macs_list[offset + 3], macs_list[offset + 4], macs_list[offset + 5]);
*/
		offset += MV_MAC_ADDR_SIZE;
	}

	err = pp3_fw_vport_mac_list_set(dev_priv->vport->vport, macs_list_size, macs_list);
	if (err < 0) {
		pr_err("%s error: %s failed to send mcast list to FW\n", __func__, dev->name);
		kfree(macs_list);
		return -1;
	}
	kfree(macs_list);
	return 0;
}

/*---------------------------------------------------------------------------*/
void mv_pp3_set_rx_mode(struct net_device *dev)
{
	struct pp3_dev_priv *dev_priv = MV_PP3_PRIV(dev);
	unsigned char l2_ops;
	int vport;

	if (!mv_pp3_shared_initialized(pp3_priv))
		return;

	if (dev_priv->vport->type != MV_PP3_NSS_PORT_ETH)
		return;

	if (dev->flags & IFF_PROMISC)
		/* Accept all */
		l2_ops = MV_NSS_PROMISC_MODE;
	else {
		if (dev->flags & IFF_ALLMULTI)
			/* Accept all multicast */
			l2_ops = MV_NSS_ALL_MCAST_MODE;
		else {
			/* Accept Unicast to me */
			l2_ops = MV_NSS_NON_PROMISC_MODE;
			/* Accept initialized Multicast */
			if (!netdev_mc_empty(dev))
				if (mv_pp3_proc_mac_mc(dev) < 0) {
					pr_err("%s: failed to set multicast list\n", __func__);
					return;
				}
		}
	}
	vport = dev_priv->vport->vport;
	/* set/clear relevant bits in fw l2 ops bitmap */
	pp3_fw_port_l2_filter_mode(vport, MV_NSS_L2_UCAST_PROMISC, l2_ops & BIT(MV_NSS_L2_UCAST_PROMISC));
	pp3_fw_port_l2_filter_mode(vport, MV_NSS_L2_MCAST_PROMISC, l2_ops & BIT(MV_NSS_L2_MCAST_PROMISC));
	pp3_fw_port_l2_filter_mode(vport, MV_NSS_L2_BCAST_ADM, l2_ops & BIT(MV_NSS_L2_BCAST_ADM));

	/* Update l2_options field in virtual port */
	dev_priv->vport->port.emac.l2_options &=
		~(BIT(MV_NSS_L2_UCAST_PROMISC) | BIT(MV_NSS_L2_MCAST_PROMISC) | BIT(MV_NSS_L2_BCAST_ADM));
	dev_priv->vport->port.emac.l2_options |= l2_ops;
}

/* Set CPU affinity (default CPU) for network interface */
/* This CPU will process ingress traffic if RSS is disabled */
int mv_pp3_cpu_affinity_set(struct net_device *dev, int cpu)
{
	struct pp3_dev_priv *dev_priv;
	struct pp3_vport *vp_priv;

	if (mv_pp3_max_check(cpu, nr_cpu_ids, "cpu"))
		return -1;

	dev_priv = MV_PP3_PRIV(dev);
	if (!dev_priv) {
		pr_err("Can't set cpu affinity - %s in not initialized\n", dev->name);
		return -1;
	}
	vp_priv = dev_priv->vport;
	if (vp_priv) {
		if (!dev_priv->cpu_vp[cpu] || !cpumask_test_cpu(cpu, &dev_priv->rx_cpus)) {
			pr_err("%s: Unexpected CPU affinity %d\n", dev->name, cpu);
			return -1;
		}

		if (pp3_fw_vport_def_dest_set(vp_priv->vport, MV_PP3_CPU_VPORT_ID(cpu)) < 0) {
			pr_warn("%s Error: FW vport %d default destination update failed\n",
				__func__, vp_priv->vport);
			return -1;
		}
		vp_priv->dest_vp = MV_PP3_CPU_VPORT_ID(cpu);
	}
	return 0;
}

/*--------------------------------------------------------------------------- *
 * mv_pp3_late_init							      *
 * use this to any late stage initialion or semantic validattion.	      *
 *----------------------------------------------------------------------------*/
static int mv_pp3_late_init(struct net_device *dev)
{
	dev->features = NETIF_F_IP_CSUM | NETIF_F_IPV6_CSUM | NETIF_F_RXCSUM;

	dev->hw_features |= NETIF_F_IP_CSUM | NETIF_F_IPV6_CSUM | NETIF_F_RXCSUM;

#ifdef CONFIG_MV_PP3_SG
	dev->features |= NETIF_F_SG;
	dev->hw_features |= NETIF_F_SG;

#ifdef CONFIG_MV_PP3_TSO
	dev->features |= NETIF_F_TSO;
	dev->hw_features |= NETIF_F_TSO;
#endif /* CONFIG_MV_PP3_TSO */
#endif /* CONFIG_MV_PP3_SG */

	dev->vlan_features |= dev->features;

	return 0;
}

static netdev_features_t mv_pp3_netdev_fix_features(struct net_device *dev, netdev_features_t features)
{
	if (MV_MAX_PKT_SIZE(dev->mtu) > MV_PP3_TX_CSUM_MAX_SIZE) {
		if (features & (NETIF_F_IP_CSUM | NETIF_F_TSO)) {
			features &= ~(NETIF_F_IP_CSUM | NETIF_F_TSO);
			pr_warn("%s: NETIF_F_IP_CSUM and NETIF_F_TSO not supported for packet size larger than %d bytes\n",
					dev->name, MV_PP3_TX_CSUM_MAX_SIZE);
		}
	}
	return features;
}

/*---------------------------------------------------------------------------*/
/* Initialize emac data in network device interface			     */
/*---------------------------------------------------------------------------*/
int mv_pp3_netdev_set_emac_params(struct net_device *dev, struct device_node *np)
{
	struct pp3_dev_priv *dev_priv;

	if (!dev) {
		pr_err("%s: network device pointer is NULL\n", __func__);
		goto err;
	}

	dev_priv = MV_PP3_PRIV(dev);

	if (of_property_read_u32(np, "id", &dev_priv->id)) {
		pr_err("could not get port ID\n");
		goto err;
	}
	mv_pp3_ftd_mac_data_get(np, &dev_priv->mac_data);
	mv_pp3_fdt_mac_address_get(np, dev_priv->dev->dev_addr);

	dev->ethtool_ops = &mv_pp3_ethtool_ops;

	/* set mac connectivity flag */
	set_bit(MV_PP3_F_MAC_CONNECT_BIT, &(dev_priv->flags));

	return 0;
err:
	pr_err("%s: function failed\n", __func__);
	return -1;
}
/*---------------------------------------------------------------------------*/

void mv_pp3_netdev_delete(struct net_device *dev)
{
	struct pp3_dev_priv *dev_priv;

	if (!dev)
		return;

	dev_priv = MV_PP3_PRIV(dev);

	if (!dev_priv)
		return;

	/*
	TODO:
	free cpu shared memory
	dev statistics
	unregister network device
	mv_pp3_dev_priv_delete(dev_priv);
	free_netdev(dev);
	*/

	pr_info("%s: not implemented yet\n", dev->name);
}

/*---------------------------------------------------------------------------*/
/* Allocate and initialize net_device structures			     */
/*---------------------------------------------------------------------------*/
struct net_device *mv_pp3_netdev_init(const char *name, int rx_vqs, int tx_vqs)
{
	struct pp3_dev_priv *dev_priv;
	struct net_device *dev;
	int rxq_num, txq_num;
	int mtu = 1500;

	rxq_num = txq_num = nr_cpu_ids;

	/* RXQs and TXQs per CPU */
	dev = alloc_etherdev_mqs(sizeof(struct pp3_dev_priv), txq_num, rxq_num);
	if (!dev)
		return NULL;

	dev->tx_queue_len = CONFIG_MV_PP3_TXQ_SIZE;
	dev->watchdog_timeo = 5 * HZ;

	SET_NETDEV_DEV(dev, mv_pp3_dev_get(pp3_priv));

	dev->mtu = mv_pp3_check_mtu_valid(mtu);
	if (dev->mtu < 0)
		return NULL;

	dev_priv = MV_PP3_PRIV(dev);

	memset(dev_priv, 0, sizeof(struct pp3_dev_priv));

	dev_priv->dev = dev;

	dev_priv->rxqs_per_cpu = rx_vqs;
	dev_priv->txqs_per_cpu = tx_vqs;
	/* Init rxq size and txq size */
	dev_priv->rxq_capacity = CONFIG_MV_PP3_RXQ_SIZE;
	dev_priv->txq_capacity = CONFIG_MV_PP3_TXQ_SIZE;
	cpumask_copy(&dev_priv->rx_cpus, cpu_possible_mask);

	/* Init rx/tx time/packet coalesce */
	dev_priv->rx_time_coal = CONFIG_MV_PP3_RX_COAL_USEC;
	dev_priv->rx_pkt_coal = CONFIG_MV_PP3_RX_COAL_PKTS;
	dev_priv->tx_done_pkt_coal = CONFIG_MV_PP3_TXDONE_COAL_PKTS;
	dev_priv->tx_done_time_coal = MV_PP3_TXDONE_TIMER_USEC_PERIOD;

	/* alloc shared cpu struct, long and short pools */
	dev_priv->cpu_shared = mv_pp3_cpu_shared_alloc(pp3_priv);
	if (!dev_priv->cpu_shared)
		goto err_free_netdev;

	dev_priv->dev_stats = alloc_percpu(struct pp3_netdev_stats);
	if (!dev_priv->dev_stats)
		goto err_free_netdev;

	dev->netdev_ops = &mv_pp3_netdev_ops;

	strcpy(dev->name, name);

	if (register_netdev(dev) < 0) {
		dev_err(&dev->dev, "failed to register\n");
		goto err_free_netdev;
	}

#ifndef CONFIG_MV_PP3_GRO
	/* register_netdev() always sets NETIF_F_GRO via NETIF_F_SOFT_FEATURES */
	dev->features &= ~NETIF_F_GRO;
#endif /* CONFIG_MV_PP3_GRO */

	if (pp3_netdev_next < pp3_ports_num) {
		pp3_netdev[pp3_netdev_next++] = dev_priv;
		return dev;
	}

	pr_err("%s Error: driver support up to %d network devices\n", __func__, pp3_ports_num);

err_free_netdev:
	mv_pp3_netdev_delete(dev);
	free_netdev(dev);
	return NULL;
}
/*---------------------------------------------------------------------------*/

void mv_pp3_netdev_show(struct net_device *dev)
{
	char cpus_str[16];
	struct pp3_pool *ppool;
	struct pp3_dev_priv *dev_priv = MV_PP3_PRIV(dev);

	scnprintf(cpus_str, sizeof(cpus_str), "%*pb", cpumask_pr_args(&dev_priv->rx_cpus));

	pr_info("  o Loading network interface %s: mtu = %d, cpu_mask = 0x%s\n",
		dev->name, dev->mtu, cpus_str);

	if (test_bit(MV_PP3_F_MAC_CONNECT_BIT, &dev_priv->flags)) {
		pr_info("\t  o emac #%d         : %s (%d)\n", dev_priv->id,
			mv_port_mode_str(dev_priv->mac_data.port_mode),
			dev_priv->mac_data.port_mode);
	}
	pr_info("\t  o RX Queue support: %d VQs * %d CFHs\n",
		dev_priv->rxqs_per_cpu, dev_priv->rxq_capacity);

	pr_info("\t  o TX Queue support: %d VQs * %d CFHs\n",
		dev_priv->txqs_per_cpu, dev_priv->txq_capacity);

	ppool = dev_priv->cpu_shared->long_pool;
	if (ppool) {
		pr_cont("\t  o RX long pool    : capacity = %d packets - ", ppool->capacity);
		pr_cont("%d bytes of coherent memory allocated\n", ppool->capacity * 2 * sizeof(unsigned int));
	}

	ppool = dev_priv->cpu_shared->short_pool;
	if (ppool) {
		pr_cont("\t  o RX short pool   : capacity = %d packets - ", ppool->capacity);
		pr_cont("%d bytes of coherent memory allocated\n", ppool->capacity * 2 * sizeof(unsigned int));
	}

	ppool = dev_priv->cpu_shared->txdone_pool;
	if (ppool) {
		pr_info("\t  o TX done pool    : capacity = %d packets - ", ppool->capacity);
		pr_cont("%d bytes of coherent memory allocated\n", ppool->capacity * 2 * sizeof(unsigned int));
	}
}
/*---------------------------------------------------------------------------*/

/*---------------------------------------------------------------------------*/
/* alloc global structure memory					     */
/*---------------------------------------------------------------------------*/
int mv_pp3_netdev_global_init(struct mv_pp3 *priv)
{
	/* interrupts mode */
	mv_pp3_run_hmac_interrupts = 1;
	mv_pp3_irq_rx_base = mv_pp3_irq_base_get(priv);
	pp3_ports_num = MV_PP3_DEV_NUM;
	pp3_netdev_next = 0;

	pp3_netdev = kzalloc(pp3_ports_num * sizeof(struct pp3_dev_priv *), GFP_KERNEL);
	if (!pp3_netdev)
		return -ENOMEM;

	pp3_priv = priv;

	return 0;

	kfree(pp3_netdev);

	pr_err("%s: out of memory\n", __func__);
	return -ENOMEM;
}

/*---------------------------------------------------------------------------*/
/* return true if txdoen pool is empty, otherwise return false               */
static bool mv_pp3_dev_txdone_is_empty(struct pp3_dev_priv *dev_priv)
{
	struct pp3_vport *cpu_vp;
	int cpu;

	if (!dev_priv) {
		pr_err("%s: invalid param", __func__);
		return -1;
	}

	for_each_possible_cpu(cpu) {

		cpu_vp = dev_priv->cpu_vp[cpu];

		if (cpu_vp && cpu_vp->port.cpu.txdone_todo)
			return false;
	}

	return true;
}

/*---------------------------------------------------------------------------*/

/* Update number of buffers in the pool accordingly with new value */
static int mv_pp3_dev_pool_update(struct net_device *dev, struct pp3_pool *ppool, int size)
{
	int rc = 0;

	if (size > ppool->capacity) {
		ppool->pool_size = ppool->capacity;
		pr_warn("%s: Warning! %d pool capacity %d is less than recommended value %d\n",
			__func__, ppool->pool, ppool->capacity, size);
	} else
		ppool->pool_size = size;

	if (ppool->buf_num < ppool->pool_size)
		rc = mv_pp3_pool_bufs_add(ppool->pool_size - ppool->buf_num, dev, ppool);
	else if (ppool->buf_num > ppool->pool_size)
		rc = mv_pp3_pool_bufs_free(ppool->buf_num - ppool->pool_size, dev, ppool);

	return rc;
}
/*---------------------------------------------------------------------------*/
static int mv_pp3_dev_pools_empty(struct pp3_dev_priv *dev_priv)
{
	struct pp3_pool *ppool;

	if (!dev_priv) {
		pr_err("%s: invalid param", __func__);
		return -1;
	}

	ppool = dev_priv->cpu_shared->long_pool;
	if (ppool)
		mv_pp3_dev_pool_update(dev_priv->dev, ppool, atomic_read(&ppool->in_use));

	ppool = dev_priv->cpu_shared->short_pool;
	if (ppool)
		mv_pp3_dev_pool_update(dev_priv->dev, ppool, atomic_read(&ppool->in_use));

	ppool = dev_priv->cpu_shared->lro_pool;
	if (ppool)
		mv_pp3_dev_pool_update(dev_priv->dev, ppool, atomic_read(&ppool->in_use));

	return 0;
}
/*---------------------------------------------------------------------------*/

static int mv_pp3_dev_rx_pool_size_calc(struct pp3_dev_priv *dev_priv)
{
	int rxq, cpu, size = 0;
	struct pp3_vport *vp;
	struct pp3_vq *vq;

	/* Number of buffers in the pool must be more than: */
	/* number of CPUs * number of RXQs per CPU * maximum number of packets in each RX SWQ */
	for_each_possible_cpu(cpu) {
		vp = dev_priv->cpu_vp[cpu];
		if (!vp)
			continue;

		for (rxq = 0; rxq < vp->rx_vqs_num; rxq++) {
			vq = vp->rx_vqs[rxq];
			if (!vq || !vq->swq)
				continue;

			size += dev_priv->rxq_capacity;
		}
	}
	/* Add extra 2000 buffers for each pool */
	size += MV_PP3_RX_BUFS_EXTRA;

	return size;
}
/*---------------------------------------------------------------------------*/

static int mv_pp3_dev_pools_fill(struct pp3_dev_priv *dev_priv)
{
	struct pp3_pool *ppool;
	int size;

	if (!dev_priv) {
		pr_err("%s: invalid param", __func__);
		return -1;
	}

	size = mv_pp3_dev_rx_pool_size_calc(dev_priv);


	ppool = dev_priv->cpu_shared->long_pool;
	if (ppool)
		mv_pp3_dev_pool_update(dev_priv->dev, ppool, size);

	ppool = dev_priv->cpu_shared->short_pool;
	if (ppool)
		mv_pp3_dev_pool_update(dev_priv->dev, ppool, size);

	ppool = dev_priv->cpu_shared->lro_pool;
	if (ppool)
		mv_pp3_dev_pool_update(dev_priv->dev, ppool, size);

	return 0;
}

/*---------------------------------------------------------------------------*/
static void mv_pp3_dev_napi_enable(struct pp3_dev_priv *dev_priv)
{
	int cpu;
	for_each_possible_cpu(cpu)
		if (dev_priv->cpu_vp[cpu])
			napi_enable(&dev_priv->cpu_vp[cpu]->port.cpu.napi);
}
/*---------------------------------------------------------------------------*/
static void mv_pp3_dev_napi_disable(struct pp3_dev_priv *dev_priv)
{
	int cpu;

	for_each_possible_cpu(cpu)
		if (dev_priv->cpu_vp[cpu]) {
			/* wait until napi stop transmit */
			napi_synchronize(&dev_priv->cpu_vp[cpu]->port.cpu.napi);
			napi_disable(&dev_priv->cpu_vp[cpu]->port.cpu.napi);
		}
}
/*---------------------------------------------------------------------------*/

static int mv_pp3_dev_rxq_proc_done(struct pp3_vq *rx_vq)
{
	int time_out = 0;
	int swq = rx_vq->swq->swq;
	int frame = rx_vq->swq->frame_num;
	static int time_out_max = 1000;

	while (mv_pp3_hmac_rxq_occ_get(frame, swq) &&
			(time_out <= time_out_max))
		time_out++;

	if (time_out > time_out_max) {
		pr_err("Error %s: frame %d, queue %d proc retries exceeded\n",
			__func__, frame, swq);
		return -1;
	}

	return 0;
}
/*---------------------------------------------------------------------------*/

static int mv_pp3_dev_txq_proc_done(struct pp3_vq *tx_vq)
{
	int time_out = 0;
	int swq = tx_vq->swq->swq;
	int frame = tx_vq->swq->frame_num;
	static int time_out_max = 1000;

	/* Control path function  - do not remove delay from loop */

	while (mv_pp3_hmac_txq_occ_get(frame, swq) &&
			(time_out <= time_out_max)) {
		mdelay(1);
		time_out++;
	}

	if (time_out > time_out_max) {
		pr_err("Error %s: frame %d, queue %d proc retries exceeded\n",
			__func__, frame, swq);
		return -1;
	}

	return 0;
}
/*---------------------------------------------------------------------------*/

static int mv_pp3_dev_queues_proc_done(struct pp3_dev_priv *dev_priv)
{
	struct pp3_vport *cpu_vp;
	int vq, cpu;


	for_each_possible_cpu(cpu) {

		cpu_vp = dev_priv->cpu_vp[cpu];

		if (!cpu_vp)
			continue;

		for (vq = 0; vq < cpu_vp->rx_vqs_num; vq++) {

			if (!cpu_vp->rx_vqs[vq])
				continue;

			if (mv_pp3_dev_rxq_proc_done(cpu_vp->rx_vqs[vq]))
				return -1;
		}

		for (vq = 0; vq < cpu_vp->tx_vqs_num; vq++) {

			if (!cpu_vp->tx_vqs[vq])
				continue;

			if (mv_pp3_dev_txq_proc_done(cpu_vp->tx_vqs[vq]))
				return -1;
		}
	}

	return 0;
}
/*---------------------------------------------------------------------------*/
/*
delete for network device next private fields
	1 - CPU virtual port (per CPU)
	2 - EMAC virtual port
	3 - device statistics (per CPU)
*/
static void mv_pp3_dev_priv_delete(struct pp3_dev_priv *dev_priv)
{
	int cpu;

	if (!dev_priv)
		return;

	for_each_possible_cpu(cpu) {
		mv_pp3_vport_delete(dev_priv->cpu_vp[cpu]);
		dev_priv->cpu_vp[cpu] = NULL;
	}

	mv_pp3_vport_delete(dev_priv->vport);
	dev_priv->vport = NULL;

	free_percpu(dev_priv->dev_stats);
}
/*---------------------------------------------------------------------------*/
/*
alloc for network device next private fields
	1 - CPU virtual port (per CPU)
	2 - EMAC virtual port
	3 - device statistics (per CPU)
*/
static int mv_pp3_dev_priv_alloc(struct pp3_dev_priv *dev_priv)
{
	static int cpu_vp_index = MV_PP3_INTERNAL_CPU_PORT_MIN;
	int cpu, rx_vqs, tx_vqs;

	if (!dev_priv)
		goto err;

	if (cpu_vp_index >= MV_PP3_INTERNAL_CPU_PORT_NUM)
		goto err;

	/* TODO: define default rxq_num and txq_num for emac virtual port */

	tx_vqs = dev_priv->txqs_per_cpu;

	if (test_bit(MV_PP3_F_MAC_CONNECT_BIT, &dev_priv->flags))
		/* alloc emac virtual port */
		dev_priv->vport = mv_pp3_vport_alloc(dev_priv->id, MV_PP3_NSS_PORT_ETH, 1, tx_vqs);
	else
		/* alloc external virtual port - vqs are not relevant */
		dev_priv->vport = mv_pp3_vport_alloc(dev_priv->id, MV_PP3_NSS_PORT_EXT, 0, 0);

	if (!dev_priv->vport)
		goto oom;

	for_each_possible_cpu(cpu) {
		if (cpumask_test_cpu(cpu, &dev_priv->rx_cpus))
			rx_vqs = dev_priv->rxqs_per_cpu;
		else
			rx_vqs = 0;
		dev_priv->cpu_vp[cpu] = mv_pp3_vport_alloc(cpu_vp_index, MV_PP3_NSS_PORT_CPU, rx_vqs, tx_vqs);
		if (!dev_priv->cpu_vp[cpu])
			goto oom;

		cpu_vp_index++;
	}
	return 0;

err:
	pr_err("%s: function failed\n", __func__);
	return -1;
oom:
	mv_pp3_dev_priv_delete(dev_priv);
	pr_err("%s: Out of memory\n", __func__);
	return -ENOMEM;
}
/*---------------------------------------------------------------------------*/

static int mv_pp3_dev_priv_sw_init(struct pp3_dev_priv *dev_priv)
{
	int cpu, ret_val;
	struct pp3_vport *cpu_vp, *vp_priv;

	if (!dev_priv)
		goto err;

	if (mv_pp3_cpu_shared_sw_init(dev_priv->cpu_shared, MV_RX_PKT_SIZE(dev_priv->dev->mtu)) < 0)
		goto err;

	vp_priv = dev_priv->vport;
	/* Emac virtual port SW initialization. For emac interfaces ID == EMAC */
	if (vp_priv->type == MV_PP3_NSS_PORT_ETH) {

		/* eth interface use Linux standard functions, set ops to NULL */
		dev_priv->cpu_shared->gnss_ops = NULL;

		ret_val = mv_pp3_emac_vport_sw_init(vp_priv, dev_priv->id, &dev_priv->mac_data);
		if (ret_val < 0)
			goto err;

		vp_priv->port.emac.mtu = dev_priv->dev->mtu;

		/* create link change tasklet for interrupt handling */
		tasklet_init(&vp_priv->port.emac.lc_tasklet,
				mv_pp3_link_change_tasklet, (unsigned long)dev_priv);
	} else {
		/* set gnss ops to gnss (external) interface */
		dev_priv->cpu_shared->gnss_ops = mv_nss_ops_get(dev_priv->dev);
		if (dev_priv->cpu_shared->gnss_ops) {
			if (dev_priv->cpu_shared->gnss_ops->register_iface(dev_priv->dev, &mv_pp3_nss_if_ops))
				pr_err("%s: cannot register %s ops\n", __func__, dev_priv->dev->name);
		} else
			pr_err("%s: cannot get gnss ops for %s\n", __func__, dev_priv->dev->name);
	}

	/* Init cpu virtual ports */
	for_each_possible_cpu(cpu) {
		cpu_vp = dev_priv->cpu_vp[cpu];
		if (!cpu_vp)
			continue;

		/* Init vport CPUs pointers */
		cpu_vp->port.cpu.cpu_ctrl = mv_pp3_cpu_get(cpu);
		cpu_vp->port.cpu.cpu_shared = dev_priv->cpu_shared;
		cpu_vp->root = (void *)dev_priv->dev;

		if (cpumask_test_cpu(cpu, &dev_priv->rx_cpus)) {
			if (mv_pp3_cfg_dp_reserve_rxq(cpu_vp->vport, vp_priv->vport, cpu, cpu_vp->rx_vqs_num) < 0) {
				pr_err("%s: vport #%d failed to reserve %d ingress vq on cpu %d\n", __func__,
					cpu_vp->vport, cpu_vp->rx_vqs_num, cpu);
				goto err;
			}
			/* set first cpu vport as emac vport dflt dest */
			if (vp_priv->dest_vp == MV_NSS_PORT_NONE)
				vp_priv->dest_vp = MV_PP3_CPU_VPORT_ID(cpu);
		}

		if (mv_pp3_cfg_dp_reserve_txq(cpu_vp->vport, vp_priv->vport, cpu, cpu_vp->tx_vqs_num) < 0) {
			pr_err("%s: vport #%d failed to reserve %d egress vq on cpu %d\n", __func__,
				cpu_vp->vport, cpu_vp->tx_vqs_num, cpu);
			goto err;
		}

		if (mv_pp3_cpu_vport_sw_init(cpu_vp, &dev_priv->rx_cpus, cpu) < 0)
			goto err;

		/* Init cpu virtual port NAPI */
		netif_napi_add(dev_priv->dev, &cpu_vp->port.cpu.napi, mv_pp3_poll, 64);

		/* init txdone timer */
		mv_pp3_timer_init(&cpu_vp->port.cpu.txdone_timer, cpu,
					dev_priv->tx_done_time_coal, MV_PP3_TASKLET,
					mv_pp3_txdone_timer_callback, (unsigned long)cpu_vp);


		/* set cpu vport dflt dest to EMAC virtual port esle left unknown */
		if (vp_priv->type == MV_PP3_NSS_PORT_ETH)
			cpu_vp->dest_vp = vp_priv->vport;
	}

	/* init MC address link list */
	INIT_LIST_HEAD(&dev_priv->mac_list);
	dev_priv->mac_list_size = 0;

	/* each emac interface placed in different frame, has its own profile N0 in frame */
	/* all external interfaces work with time coalescing profile 1 has the same configuration */
	if (vp_priv->type == MV_PP3_NSS_PORT_ETH)
		dev_priv->rx_time_prof = 0;
	else
		dev_priv->rx_time_prof = 1;


	return 0;
err:
	pr_err("%s: function failed\n", __func__);
	return -1;
}
/*---------------------------------------------------------------------------*/

static int mv_pp3_dev_priv_hw_init(struct pp3_dev_priv *dev_priv)
{
	int cpu;

	if (!dev_priv)
		goto err;

	if (mv_pp3_cpu_shared_hw_init(dev_priv->cpu_shared) < 0)
		goto err;

	if (dev_priv->vport->type == MV_PP3_NSS_PORT_ETH)
		if (mv_pp3_emac_vport_hw_init(dev_priv->vport) < 0)
			goto err;

	for_each_possible_cpu(cpu) {
		if (!dev_priv->cpu_vp[cpu])
			continue;

		if (mv_pp3_cpu_vport_hw_init(dev_priv->cpu_vp[cpu]) < 0)
			goto err;

		/* configure coalescing parameters */
		if (cpumask_test_cpu(cpu, &dev_priv->rx_cpus)) {
			if (mv_pp3_cpu_vport_rx_pkt_coal_set(dev_priv->cpu_vp[cpu], dev_priv->rx_pkt_coal) < 0)
				goto err;
			if (mv_pp3_cpu_vport_rx_time_prof_set(dev_priv->cpu_vp[cpu], dev_priv->rx_time_prof) < 0)
				goto err;
			if (mv_pp3_cpu_vport_rx_time_coal_set(dev_priv->cpu_vp[cpu], dev_priv->rx_time_coal) < 0)
				goto err;
		}
	}
	/* Configure default parameters for ingress VQs */
	if (mv_pp3_dev_ingress_vqs_defaults_set(dev_priv->dev))
		goto err;

	/* Configure default parameters for egress VQs */
	if (mv_pp3_dev_egress_vqs_defaults_set(dev_priv->dev))
		goto err;

	return 0;
err:
	pr_err("%s: function failed\n", __func__);
	return -1;
}
/*---------------------------------------------------------------------------*/

int mv_pp3_dev_open(struct net_device *dev)
{
	struct pp3_dev_priv *dev_priv = MV_PP3_PRIV(dev);
	int emac;

	/* link interrupts and emac are closed */

	if (!mv_pp3_shared_initialized(pp3_priv))
		if (mv_pp3_shared_start(pp3_priv)) {
			pr_err("%s: mv_pp3_shared_start fail\n", __func__);
			return -1;
		}

	if (!(dev_priv->flags & MV_PP3_F_INIT)) {
		if (mv_pp3_dev_priv_alloc(dev_priv))
			goto err;

		if (mv_pp3_dev_priv_sw_init(dev_priv))
			goto err;

		if (mv_pp3_dev_priv_hw_init(dev_priv))
			goto err;

		if (mv_pp3_dev_fw_update(dev_priv))
			goto err;

		set_bit(MV_PP3_F_INIT_BIT, &(dev_priv->flags));
	}

	mv_pp3_dev_pools_fill(dev_priv);

	mv_pp3_dev_napi_enable(dev_priv);

	/* initialize net_device RX and link IRQs */
	if (mv_pp3_dev_irqs_init(dev_priv))
		goto err;

	if (dev_priv->vport->type == MV_PP3_NSS_PORT_ETH) {
		emac = dev_priv->vport->port.emac.emac_num;
		/* config link according to status */
		mv_pp3_dev_link_event(dev_priv);
		/* Enable gop link event */
		mv_pp3_gop_port_events_unmask(emac);
	} else
		mv_pp3_dev_up(dev_priv);

	/* set device state in FW to enable */
	mv_pp3_dev_fw_up(dev_priv);

	set_bit(MV_PP3_F_IF_UP_BIT, &(dev_priv->flags));

	return 0;
err:
	pr_err("%s: function failed\n", __func__);
	return -1;
}
/*---------------------------------------------------------------------------*/
int mv_pp3_dev_stop(struct net_device *dev)
{
	struct pp3_dev_priv *dev_priv = MV_PP3_PRIV(dev);
	int emac;

	clear_bit(MV_PP3_F_IF_UP_BIT, &(dev_priv->flags));

	if (dev_priv->vport->type == MV_PP3_NSS_PORT_ETH) {
		emac = dev_priv->vport->port.emac.emac_num;
		/* Disable gop link event */
		mv_pp3_gop_port_events_mask(emac);
		/* disable EMAC RX processing */
		mv_pp3_emac_rx_enable(emac, false);

		mv_pp3_dev_down(dev_priv);
	}

	/* set device state in FW to disable */
	mv_pp3_dev_fw_down(dev_priv);

	mdelay(10);

	mv_pp3_dev_napi_disable(dev_priv);

	/* release network device RX and link IRQs */
	mv_pp3_dev_irqs_free(dev_priv);

	/* make sure that rxqs/txqs are empty*/
	if (mv_pp3_dev_queues_proc_done(dev_priv))
		return -1;

	mv_pp3_dev_pools_empty(dev_priv);

	if (!mv_pp3_dev_txdone_is_empty(dev_priv)) {
		pr_err("%s: txdone pool is not empty\n", dev->name);
		return -1;
	}

	pr_info("%s: stopped\n", dev->name);

	return 0;
}


/* return positive if MTU is valid */
static int mv_pp3_check_mtu_valid(int mtu)
{
	if (mtu < MV_MTU_MIN) {
		pr_info("MTU value %d is too small, must be at least %d - Failed\n",
			mtu, MV_MTU_MIN);
		return -1;
	}

	if (mtu > MV_MTU_MAX) {
		pr_info("MTU value %d is too large, set to %d\n",
			mtu, MV_MTU_MAX);
		mtu = MV_MTU_MAX;
	}
	return mtu;
}

static int mv_pp3_change_mtu_internals(struct net_device *dev, int mtu)
{
	struct pp3_dev_priv *dev_priv = MV_PP3_PRIV(dev);
	struct pp3_pool *long_pool = dev_priv->cpu_shared->long_pool;
	struct pp3_vport *emac_vp;

	dev->mtu = mtu;

	if (long_pool) {
		mv_pp3_pool_long_sw_init(long_pool, long_pool->headroom,
						MV_RX_PKT_SIZE(mtu));

		if (pp3_fw_bm_pool_set(long_pool) < 0)
			pr_warn("%s: FW long pool update failed\n", __func__);
	}

	emac_vp = dev_priv->vport;

	emac_vp->port.emac.mtu = mtu;

	if (pp3_fw_vport_mtu_set(emac_vp->vport, mtu) < 0)
		pr_warn("%s: FW EMAC vport mtu set failed\n", __func__);

	/* TODO - check if necessary */
	/* netdev_update_features(dev); */

	return 0;
}
/*---------------------------------------------------------------------------*/

static int mv_pp3_change_mtu(struct net_device *dev, int mtu)
{
	int is_up = 0;
	struct pp3_dev_priv *dev_priv = MV_PP3_PRIV(dev);

	mtu = mv_pp3_check_mtu_valid(mtu);
	if (mtu < 0)
		return -EINVAL;

	/* interface not initialized yet */
	if (!test_bit(MV_PP3_F_INIT_BIT, &dev_priv->flags)) {
		dev->mtu = mtu;
		return 0;
	}

	/* Supported only for EMAC virtual ports */
	if (!dev_priv->vport || (dev_priv->vport->type != MV_PP3_NSS_PORT_ETH)) {
		pr_err("%s: not supported for %s\n", __func__, dev->name);
		return -1;
	}

	is_up = netif_running(dev);

	if (is_up && dev->netdev_ops->ndo_stop(dev)) {
		pr_err("%s: stop interface failed\n", dev->name);
		goto error;
	}

	if (mv_pp3_change_mtu_internals(dev, mtu))
		goto error;

	if (is_up && dev->netdev_ops->ndo_open(dev)) {
		pr_err("%s: start interface failed\n", dev->name);
		goto error;
	}

	pr_info("%s: mtu changed\n", dev->name);
	return 0;

error:
	pr_err("%s: change mtu failed\n", dev->name);
	return -1;
}
/*---------------------------------------------------------------------------*/

#ifdef CONFIG_MV_PP3_FPGA
static int mv_pp3_pci_probe(struct pci_dev *pdev,
	const struct pci_device_id *ent)
{
	u32 gop_vbase, vbase_address;

	/* code below relevant fot FPGA only */
	if (pci_enable_device(pdev)) {
		pr_err("Cannot enable PCI device, aborting\n");
		return -1;
	}

	if (!(pci_resource_flags(pdev, 0) & IORESOURCE_MEM)) {
		pr_err("Cannot find proper PCI device base address, aborting\n");
		return -ENODEV;
	}

	if (pci_request_regions(pdev, "mv_pp3_pci")) {
		pr_err("Cannot obtain PCI resources, aborting\n");
		return -ENODEV;
	}

	if (pci_set_dma_mask(pdev, DMA_BIT_MASK(32))) {
		pr_err("No usable DMA configuration, aborting\n");
		return -ENODEV;
	}

	vbase_address = (u32)pci_iomap(pdev, 0, 16*1024*1024);
	if (!vbase_address)
		pr_err("Cannot map device registers, aborting\n");

	mv_hw_silicon_base_addr_set(vbase_address);
	pr_info("NSS registers base      : VIRT = 0x%0x, size = %d KBytes\n", vbase_address, 16*1024);

	gop_vbase = (u32)pci_iomap(pdev, 2, 64*1024);
	if (!gop_vbase)
		pr_err("Cannot map device GOP, aborting\n");
	pr_info("GOP registers base      : VIRT = 0x%0x, size = %d KBytes\n", gop_vbase, 64);

	pp3_gmac_base_addr_set(gop_vbase);

	return 0;
}

static void mv_pp3_pci_remove(struct pci_dev *pdev)
{
	pr_err("%s:: called", __func__);
}


/*---------------------------------------------------------------------------*/
static const struct pci_device_id fpga_id_table[] = {
	{ 0x1234, 0x1234, PCI_ANY_ID, PCI_ANY_ID, 2, 0, },
};

MODULE_DEVICE_TABLE(pci, fpga_id_table);

static struct pci_driver mv_pp3_pci_driver = {
	.name	= "mv_pp3_pci",
	.id_table = fpga_id_table,
	.probe		= mv_pp3_pci_probe,
	.remove		= mv_pp3_pci_remove,
};
#endif /* CONFIG_MV_PP3_FPGA */

/*---------------------------------------------------------------------------*/

MODULE_DESCRIPTION("Marvell PPv3 Network Driver - www.marvell.com");
MODULE_AUTHOR("Dmitri Epshtein <dima@marvell.com>");
MODULE_LICENSE("GPL");
MODULE_ALIAS("platform:" MV_PP3_SHARED_NAME);
MODULE_ALIAS("platform:" MV_PP3_PORT_NAME);

/*---------------------------------------------------------------------------*/
static inline u8 mv_pp3_cos_get(struct sk_buff *skb)
{
	struct mv_nss_metadata *pmdata;

	pmdata = (struct mv_nss_metadata *)mv_pp3_gnss_skb_mdata_get(skb->dev, skb);

	if (pmdata)
		return (u8)(pmdata->cos);

	return skb->priority;
}
/*---------------------------------------------------------------------------*/
/* PP3 Driver transmit function */
static int mv_pp3_tx(struct sk_buff *skb, struct net_device *dev)
{
	struct mv_cfh_common *cfh;
	int cpu = smp_processor_id();
	int global_cpu_vp, vq = 0;
	unsigned int l3_l4_info;
	int total_free;
	struct pp3_dev_priv *dev_priv = MV_PP3_PRIV(dev);
	struct pp3_vport *cpu_vp = dev_priv->cpu_vp[cpu];
	struct pp3_vq *tx_vq = NULL;
	struct pp3_swq *tx_swq = NULL;
	struct pp3_pool *ppool = NULL;
	int pkt_len, rd_offs, cfh_data_len, cfh_dg_size, cfh_size;
	unsigned long flags = 0;
	bool pkt_in_cfh = false;
	u8 cos;
	u32 *pmdata;
#ifdef CONFIG_MV_PP3_PTP_SERVICE
	int ptp_ts_offs = 0;
	int tx_ts_queue;
#endif

#ifdef PP3_INTERNAL_DEBUG
	if (debug_stop_rx)
		return NETDEV_TX_OK;
#endif
	MV_LIGHT_LOCK(flags);

	/* No support for scatter-gather */
	if (unlikely(skb_is_nonlinear(skb))) {
		pr_err("%s: no support for scatter-gather\n", dev->name);
		goto out;
	}

	/* get priority from mdata */
	cos = mv_pp3_cos_get(skb);

#ifdef CONFIG_MV_PP3_DEBUG_CODE
	if (cos > MV_PP3_PRIO_NUM) {
		pr_err("%s: cannot map packet priority %d to queue.\n", __func__, cos);
		goto out;
	}
#endif

	/* get vqueue mapped to priority */
	vq = mv_pp3_egress_cos_to_vq_get(cpu_vp, cos);

	/* virtual queue equal software queue */
	tx_vq = cpu_vp->tx_vqs[vq];
	tx_swq = tx_vq->swq;
	/* Add dummy Marvell header to skb */
	__skb_push(skb, MV_MH_SIZE);

#ifdef CONFIG_MV_PP3_PTP_SERVICE
	ptp_ts_offs = mv_pp3_is_pkt_ptp_tx(dev_priv, skb, &tx_ts_queue);
	if (ptp_ts_offs > 0) {
		/* Send filler or/and raise Queue priority if needed */
		mv_pp3_send_filler_pkt_cfh(skb, dev);
	}
#endif

	pkt_len = skb_headlen(skb);
	rd_offs = skb_headroom(skb);

	pkt_in_cfh = (pkt_len <= MV_PP3_CFH_PAYLOAD_MAX_SIZE);
	cfh_dg_size = MV_PP3_CFH_DG_MAX_NUM;
	cfh_size = cfh_dg_size * MV_PP3_CFH_DG_SIZE;

	/* get cfh*/
	cfh = (struct mv_cfh_common *)mv_pp3_hmac_txq_next_cfh(tx_swq->frame_num, tx_swq->swq, cfh_dg_size);
	if (!cfh) {
		STAT_ERR(tx_swq->stats.pkts_errors++);
		goto out;
	}
	prefetchw(cfh);

	/* write meta data to CFH */
	pmdata = mv_pp3_gnss_skb_mdata_get(skb->dev, skb);
	if (pmdata)
		mv_pp3_mdata_copy_to_cfh(pmdata, cfh);
	else {
		global_cpu_vp = MV_PP3_CPU_VPORT_ID(cpu_vp->port.cpu.cpu_num);
		mv_pp3_mdata_build_on_cfh(global_cpu_vp, cpu_vp->dest_vp, cos, cfh);
	}

	cfh_data_len = MV_MIN(MV_PP3_CFH_PAYLOAD_MAX_SIZE, pkt_len);

	/* Copy packet header to CFH */
	memcpy((unsigned char *)cfh + MV_PP3_CFH_PKT_SIZE, skb->data, cfh_data_len);

	cfh->plen_order = MV_CFH_PKT_LEN_SET(pkt_len + MV_PP3_CFH_MDATA_SIZE) |
				MV_CFH_REORDER_SET(REORD_NEW) | MV_CFH_LAST_BIT_SET;

	cfh->ctrl = MV_CFH_RD_SET(rd_offs + MV_PP3_CFH_PAYLOAD_MAX_SIZE) |
			MV_CFH_LEN_SET(cfh_size) | MV_CFH_MDATA_BIT_SET |
			MV_CFH_MODE_SET(HMAC_CFH) | MV_CFH_PP_MODE_SET(PP_TX_PACKET_NSS);

	l3_l4_info = mv_pp3_skb_tx_csum(skb, cpu_vp);

	if (l3_l4_info) {
		/* QC bit set at cfh word1 only if l3 or l4 checksum are calc by HW*/
		cfh->l3_l4_info = l3_l4_info;
		cfh->ctrl |= MV_CFH_QC_BIT_SET;
	}

	if (pkt_in_cfh) {
		/* CFH store packet data, pdata point to start point of payload data in cfh */

		cfh->vm_bp = cfh->marker_l = cfh->phys_l = 0;

		cfh->ctrl &= ~MV_CFH_RD_MASK;

		/* TODO - add skb recycle support */
		mv_pp3_skb_free(dev, skb);
		STAT_DBG(cpu_vp->port.cpu.stats.tx_cfh_pkt++);
	} else {
		ppool = cpu_vp->port.cpu.cpu_shared->txdone_pool;
		ppool->buf_num++;
		cfh->vm_bp = MV_CFH_BPID_SET(ppool->pool);
		cpu_vp->port.cpu.txdone_todo++;
		cfh->marker_l = (unsigned int)skb;
		/* Flush Cache */
		cfh->phys_l = mv_pp3_os_dma_map_single(dev->dev.parent, skb->head,
					       pkt_len + rd_offs, DMA_TO_DEVICE);
	}

	cfh->tag1 = MV_CFH_ADD_CRC_BIT_SET | MV_CFH_L2_PAD_BIT_SET;

#ifdef CONFIG_MV_PP3_DEBUG_CODE
	if (dev_priv->flags & MV_PP3_F_DBG_TX) {
		int pkt_num = DEV_PRIV_STATS(dev_priv, cpu)->tx_pkt_dev;

		pr_cont("\n++++++++++ %s [tx-%d]: txq = %d:%d, cpu = %d\n",
			dev->name, pkt_num, tx_swq->frame_num, tx_swq->swq, cpu);

		pp3_dbg_cfh_hdr_dump(cfh);
		pr_info("cfh metadata:\n");
		mv_debug_mem_dump(((char *)(cfh) + MV_PP3_CFH_HDR_SIZE), MV_PP3_CFH_MDATA_SIZE, 1);
		pr_info("cfh payload:\n");
		mv_debug_mem_dump(((char *)(cfh) + MV_PP3_CFH_HDR_SIZE + MV_PP3_CFH_MDATA_SIZE),
						cfh_size - (MV_PP3_CFH_HDR_SIZE + MV_PP3_CFH_MDATA_SIZE), 1);
		pr_info("\n");
		pp3_dbg_skb_dump(skb);
		mv_debug_mem_dump((void *)skb->head, pkt_len + skb_headroom(skb), 1);
	}
#endif

#ifdef CONFIG_MV_PP3_PTP_SERVICE
	if (ptp_ts_offs > 0)
		mv_pp3_ptp_pkt_proc_tx(dev_priv, cfh, pkt_len, ptp_ts_offs, tx_ts_queue);
#endif

	wmb();

	/* transmit CFH */
	mv_pp3_hmac_txq_send(tx_swq->frame_num, tx_swq->swq, cfh_dg_size);

	DEV_PRIV_STATS(dev_priv, cpu)->tx_pkt_dev++;
	DEV_PRIV_STATS(dev_priv, cpu)->tx_bytes_dev += (pkt_len - MV_MH_SIZE);

	STAT_DBG(tx_swq->stats.pkts++);
	STAT_DBG(cpu_vp->port.cpu.stats.tx_bytes += (pkt_len - MV_MH_SIZE));


	if (ppool && (ppool->mode == POOL_MODE_TXDONE)) {
		if (cpu_vp->port.cpu.txdone_todo > dev_priv->tx_done_pkt_coal) {
			STAT_INFO(cpu_vp->port.cpu.stats.txdone++);
			total_free = mv_pp3_tx_done(dev, dev_priv->tx_done_pkt_coal);
			cpu_vp->port.cpu.txdone_todo -= total_free;
		}
		if (cpu_vp->port.cpu.txdone_todo)
			mv_pp3_timer_add(&cpu_vp->port.cpu.txdone_timer);
	}

	MV_LIGHT_UNLOCK(flags);

	return NETDEV_TX_OK;

out:
	mv_pp3_skb_free(dev, skb);

	DEV_PRIV_STATS(dev_priv, cpu)->tx_drop_dev++;
	STAT_INFO(tx_swq ? tx_swq->stats.pkts_drop++ : 0;);

#ifdef CONFIG_MV_PP3_DEBUG_CODE
	if (dev_priv->flags & MV_PP3_F_DBG_TX)
		pr_info("%s: packet is dropped.\n", __func__);
#endif

	MV_LIGHT_UNLOCK(flags);

	return NETDEV_TX_OK;
}
/*---------------------------------------------------------------------------*/

int mv_pp3_dev_rx_cpus_set(struct net_device *dev, int mask)
{
	int cpu;
	struct pp3_dev_priv *dev_priv = MV_PP3_PRIV(dev);

	if (dev_priv->flags & MV_PP3_F_INIT) {
		pr_err("%s: Can't set rx_cpus after init", dev->name);
		return -1;
	}
	cpumask_clear(&dev_priv->rx_cpus);
	for_each_possible_cpu(cpu) {
		if (mask & BIT(cpu))
			cpumask_set_cpu(cpu, &dev_priv->rx_cpus);
	}
	return 0;
}

/*---------------------------------------------------------------------------*/
int mv_pp3_dev_rxqs_set(struct net_device *dev, int rxqs)
{
	struct pp3_dev_priv *dev_priv = MV_PP3_PRIV(dev);

	if (dev_priv->flags & MV_PP3_F_INIT) {
		pr_err("%s: Can't change number of RXQs after init", dev->name);
		return -1;
	}
	dev_priv->rxqs_per_cpu = rxqs;

	return 0;
}
/*---------------------------------------------------------------------------*/

int mv_pp3_dev_txqs_set(struct net_device *dev, int txqs)
{
	struct pp3_dev_priv *dev_priv = MV_PP3_PRIV(dev);

	if (dev_priv->flags & MV_PP3_F_INIT) {
		pr_err("%s: Can't change number of TXQs after init", dev->name);
		return -1;
	}

	dev_priv->txqs_per_cpu = txqs;
	return 0;
}
/*---------------------------------------------------------------------------*/

void mv_pp3_dev_init_show(struct net_device *dev)
{
	char cpus_str[16];
	struct pp3_dev_priv *dev_priv = MV_PP3_PRIV(dev);

	scnprintf(cpus_str, sizeof(cpus_str), "%*pb", cpumask_pr_args(&dev_priv->rx_cpus));

	pr_info("------- %s parameters -------\n", dev->name);
	pr_info("Number of RX VQs         : %d\n", dev_priv->rxqs_per_cpu);
	pr_info("Number of TX VQs         : %d\n", dev_priv->txqs_per_cpu);
	pr_info("CPUs mask                : 0x%s\n", cpus_str);
}
/*---------------------------------------------------------------------------*/

void mv_pp3_dev_rx_pause(struct net_device *dev, int cos)
{
	int vq;

	if (mv_pp3_dev_ingress_cos_to_vq_get(dev, cos, &vq))
		return;

	mv_pp3_dev_vqs_proc_cfg(dev, vq, false);

	return;
}
/*---------------------------------------------------------------------------*/

void mv_pp3_dev_rx_resume(struct net_device *dev, int cos)
{
	int vq;

	if (mv_pp3_dev_ingress_cos_to_vq_get(dev, cos, &vq))
		return;

	mv_pp3_dev_vqs_proc_cfg(dev, vq, true);

	return;
}
/*---------------------------------------------------------------------------*/


static const struct net_device_ops mv_pp3_netdev_ops = {
	.ndo_open            = mv_pp3_dev_open,
	.ndo_start_xmit      = mv_pp3_tx,
	.ndo_get_stats	     = mv_pp3_get_stats,
	.ndo_stop            = mv_pp3_dev_stop,
	.ndo_change_mtu      = mv_pp3_change_mtu,
	.ndo_set_rx_mode     = mv_pp3_set_rx_mode,
	.ndo_set_mac_address = mv_pp3_set_mac_addr,
	.ndo_init	     = mv_pp3_late_init,
	.ndo_fix_features    = mv_pp3_netdev_fix_features,
/*
	.ndo_select_queue    = mv_pp3_select_txq,
	.ndo_tx_timeout      = mv_pp3_tx_timeout,
	.ndo_get_stats64     = mvneta_get_stats64,
*/
};

