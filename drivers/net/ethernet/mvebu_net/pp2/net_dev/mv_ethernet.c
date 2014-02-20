
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

#include "mvCommon.h"
#include <linux/kernel.h>
#include <linux/version.h>
#include <linux/netdevice.h>
#include <linux/skbuff.h>
#include <linux/interrupt.h>

#include "mvOs.h"

#include "mvEthPhy.h"
#include "gbe/mvPp2Gbe.h"
#include "prs/mvPp2Prs.h"
#include "cls/mvPp2ClsHw.h"

#include "mv_netdev.h"

extern unsigned int mv_eth_pnc_ctrl_en;

static int mv_eth_set_mac_addr_internals(struct net_device *dev, void *addr);

/***********************************************************
 * mv_eth_start --                                         *
 *   start a network device. connect and enable interrupts *
 *   set hw defaults. fill rx buffers. restart phy link    *
 *   auto neg. set device link flags. report status.       *
 ***********************************************************/
int mv_eth_start(struct net_device *dev)
{
	struct eth_port *priv = MV_ETH_PRIV(dev);
	int group;

	/* in default link is down */
	netif_carrier_off(dev);
	/* Stop the TX queue - it will be enabled upon PHY status change after link-up interrupt/timer */

	netif_tx_stop_all_queues(dev);

	/* fill rx buffers, start rx/tx activity, set coalescing */
	if (mv_eth_start_internals(priv, dev->mtu) != 0) {
		printk(KERN_ERR "%s: start internals failed\n", dev->name);
		goto error;
	}
	/* enable polling on the port, must be used after netif_poll_disable */
	if (priv->flags & MV_ETH_F_CONNECT_LINUX) {
		for (group = 0; group < MV_ETH_MAX_RXQ; group++)
			if (priv->napi_group[group] && priv->napi_group[group]->napi)
				napi_enable(priv->napi_group[group]->napi);
	}
	if (priv->flags & MV_ETH_F_LINK_UP) {

		if (mv_eth_ctrl_is_tx_enabled(priv)) {
			netif_carrier_on(dev);
			netif_tx_wake_all_queues(dev);
		}
		printk(KERN_NOTICE "%s: link up\n", dev->name);
	}

	if (priv->flags & MV_ETH_F_CONNECT_LINUX) {
		/* connect to port interrupt line */
		if (request_irq(dev->irq, mv_eth_isr, (IRQF_DISABLED), dev->name, priv)) {
			printk(KERN_ERR "cannot request irq %d for %s port %d\n", dev->irq, dev->name, priv->port);
			for (group = 0; group < MV_ETH_MAX_RXQ; group++)
				if (priv->napi_group[group] && priv->napi_group[group]->napi)
					napi_disable(priv->napi_group[group]->napi);
			goto error;
		}

		/* unmask interrupts */
		on_each_cpu(mv_eth_interrupts_unmask, (void *)priv, 1);

		/* Enable interrupts for all CPUs */
		mvPp2GbeCpuInterruptsEnable(priv->port, priv->cpuMask);

		/* Unmask Port link interrupt */
		mvGmacPortIsrUnmask(priv->port);

		printk(KERN_NOTICE "%s: started\n", dev->name);
	}

	/* Enable GMAC */
	if (!MV_PON_PORT(priv->port))
		mvGmacPortEnable(priv->port);

	mv_eth_link_event(priv, 1);

	return 0;

error:
	printk(KERN_ERR "%s: start failed\n", dev->name);
	return -EINVAL;
}

/***********************************************************
 * mv_eth_stop --                                          *
 *   stop interface with linux core. stop port activity.   *
 *   free skb's from rings.                                *
 ***********************************************************/
int mv_eth_stop(struct net_device *dev)
{
	struct eth_port *priv = MV_ETH_PRIV(dev);
	struct cpu_ctrl *cpuCtrl;
	int group, cpu;

	/* stop new packets from arriving to RXQs */
	mvPp2PortIngressEnable(priv->port, MV_FALSE);

	mdelay(10);

	/* Disable interrupts for all CPUs */
	mvPp2GbeCpuInterruptsDisable(priv->port, priv->cpuMask);

	on_each_cpu(mv_eth_interrupts_mask, priv, 1);

	/* make sure that the port finished its Rx polling */
	for (group = 0; group < MV_ETH_MAX_RXQ; group++)
		if (priv->napi_group[group] && priv->napi_group[group]->napi)
			napi_disable(priv->napi_group[group]->napi);

	/* stop upper layer */
	netif_carrier_off(dev);
	netif_tx_stop_all_queues(dev);

	/* stop tx/rx activity, mask all interrupts, relese skb in rings,*/
	mv_eth_stop_internals(priv);
	for_each_possible_cpu(cpu) {
		cpuCtrl = priv->cpu_config[cpu];
		del_timer(&cpuCtrl->tx_done_timer);
		clear_bit(MV_ETH_F_TX_DONE_TIMER_BIT, &(cpuCtrl->flags));
	}
	if (dev->irq != 0)
		free_irq(dev->irq, priv);

	mvPp2PortEgressEnable(priv->port, MV_FALSE);

	if (!MV_PON_PORT(priv->port))
		mvGmacPortDisable(priv->port);

	printk(KERN_NOTICE "%s: stopped\n", dev->name);

	return 0;
}


int mv_eth_change_mtu(struct net_device *dev, int mtu)
{
	int old_mtu = dev->mtu;

	mtu = mv_eth_check_mtu_valid(dev, mtu);
	if (mtu < 0)
		return -EINVAL;

	if (!netif_running(dev)) {
		if (mv_eth_change_mtu_internals(dev, mtu) == -1)
			goto error;

		printk(KERN_NOTICE "%s: change mtu %d (packet-size %d) to %d (packet-size %d)\n",
				dev->name, old_mtu, RX_PKT_SIZE(old_mtu),
				dev->mtu, RX_PKT_SIZE(dev->mtu));
		return 0;
	}

	if (mv_eth_check_mtu_internals(dev, mtu))
		goto error;

	if (dev->netdev_ops->ndo_stop(dev)) {
		printk(KERN_ERR "%s: stop interface failed\n", dev->name);
		goto error;
	}

	if (mv_eth_change_mtu_internals(dev, mtu) == -1) {
		printk(KERN_ERR "%s change mtu internals failed\n", dev->name);
		goto error;
	}

	if (dev->netdev_ops->ndo_open(dev)) {
		printk(KERN_ERR "%s: start interface failed\n", dev->name);
		goto error;
	}
	printk(KERN_NOTICE "%s: change mtu %d (packet-size %d) to %d (packet-size %d)\n",
				dev->name, old_mtu, RX_PKT_SIZE(old_mtu), dev->mtu,
				RX_PKT_SIZE(dev->mtu));
	return 0;

error:
	printk(KERN_ERR "%s: change mtu failed\n", dev->name);
	return -1;
}

/***********************************************************
 * eth_set_mac_addr --                                   *
 *   stop port activity. set new addr in device and hw.    *
 *   restart port activity.                                *
 ***********************************************************/
static int mv_eth_set_mac_addr_internals(struct net_device *dev, void *addr)
{
	u8              *mac = &(((u8 *)addr)[2]);  /* skip on first 2B (ether HW addr type) */
	int             i;

	struct eth_port *priv = MV_ETH_PRIV(dev);

	if (!mv_eth_pnc_ctrl_en) {
		printk(KERN_ERR "%s Error: PARSER and CLASSIFIER control is disabled\n", __func__);

		/* linux stop the port */
		mv_eth_open(dev);
		return -1;
	}

	/* remove old parser entry*/
	mvPrsMacDaAccept(MV_PPV2_PORT_PHYS(priv->port), dev->dev_addr, 0);

	/*add new parser entry*/
	mvPrsMacDaAccept(MV_PPV2_PORT_PHYS(priv->port), mac, 1);

	/* set addr in the device */
	for (i = 0; i < 6; i++)
		dev->dev_addr[i] = mac[i];

#ifdef CONFIG_MV_INCLUDE_PON
	/* Update PON module */
	if (MV_PON_PORT(priv->port))
		mv_pon_set_mac_addr(addr);
#endif

	printk(KERN_NOTICE "%s: mac address changed\n", dev->name);

	return 0;
}

void mv_eth_rx_set_rx_mode(struct net_device *dev)
{
	struct eth_port     *priv = MV_ETH_PRIV(dev);
	int                 phyPort = MV_PPV2_PORT_PHYS(priv->port);

	if (!mv_eth_pnc_ctrl_en) {
		pr_err("%s Error: PARSER and CLASSIFIER control is disabled\n", __func__);
		return;
	}

	if (dev->flags & IFF_PROMISC)
		mvPrsMacPromiscousSet(phyPort, 1);
	else
		mvPrsMacPromiscousSet(phyPort, 0);

	if (dev->flags & IFF_ALLMULTI)
		mvPrsMacAllMultiSet(phyPort, 1);
	else
		mvPrsMacAllMultiSet(phyPort, 0);

	/* remove all port's mcast enries */
	mvPrsMcastDelAll(phyPort);

	if (dev->flags & IFF_MULTICAST) {
		if (!netdev_mc_empty(dev)) {
			struct netdev_hw_addr *ha;

			netdev_for_each_mc_addr(ha, dev) {
				if (mvPrsMacDaAccept(phyPort, ha->addr, 1) != MV_OK) {
					pr_err("%s: Mcast init failed\n", dev->name);
					break;
				}
			}
		}
	}
}


int     mv_eth_set_mac_addr(struct net_device *dev, void *addr)
{
	if (!netif_running(dev)) {
		if (mv_eth_set_mac_addr_internals(dev, addr) == -1)
			goto error;
		return 0;
	}

	if (dev->netdev_ops->ndo_stop(dev)) {
		printk(KERN_ERR "%s: stop interface failed\n", dev->name);
		goto error;
	}

	if (mv_eth_set_mac_addr_internals(dev, addr) == -1)
		goto error;

	if (dev->netdev_ops->ndo_open(dev)) {
		printk(KERN_ERR "%s: start interface failed\n", dev->name);
		goto error;
	}

	return 0;

error:
	printk(KERN_ERR "%s: set mac addr failed\n", dev->name);
	return -1;
}


/************************************************************
 * mv_eth_open -- Restore MAC address and call to   *
 *                mv_eth_start                               *
 ************************************************************/
int mv_eth_open(struct net_device *dev)
{

	struct	eth_port *priv = MV_ETH_PRIV(dev);
	int	phyPort = MV_PPV2_PORT_PHYS(priv->port);
	static  u8 mac_bcast[MV_MAC_ADDR_SIZE] = { 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF };

	if (mv_eth_pnc_ctrl_en) {

		if (mvPrsMacDaAccept(phyPort, mac_bcast, 1 /*add*/)) {
			printk(KERN_ERR "%s:mvPrsMacDaAccept\n", dev->name);
				return -1;
		}
		if (mvPrsMacDaAccept(phyPort, dev->dev_addr, 1 /*add*/)) {
			printk(KERN_ERR "%s: mvPrsMacDaAccept failed\n", dev->name);
				return -1;
		}
		if (mvPp2PrsTagModeSet(phyPort, MV_TAG_TYPE_MH)) {
			printk(KERN_ERR "%s: mvPp2PrsTagModeSet failed\n", dev->name);
				return -1;
		}
		if (mvPrsDefFlow(phyPort)) {
			printk(KERN_ERR "%s: mvPp2PrsDefFlow failed\n", dev->name);
				return -1;
		}
	}
	if (mv_eth_start(dev)) {
		printk(KERN_ERR "%s: start interface failed\n", dev->name);
		return -1;
	}
	return 0;
}

