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
#include "dbg-trace.h"
#include "mvSysHwConfig.h"
#include "boardEnv/mvBoardEnvLib.h"

#include "eth-phy/mvEthPhy.h"
#include "gbe/mvNeta.h"
#include "pnc/mvPnc.h"

#include "mv_netdev.h"

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
	if (priv->flags & MV_ETH_F_CONNECT_LINUX)
		for (group = 0; group < CONFIG_MV_ETH_NAPI_GROUPS; group++)
			napi_enable(priv->napiGroup[group]);

	if (priv->flags & MV_ETH_F_LINK_UP) {
		if (mv_eth_ctrl_is_tx_enabled(priv)) {
			netif_carrier_on(dev);
			netif_tx_wake_all_queues(dev);
		}
		printk(KERN_NOTICE "%s: link up\n", dev->name);
	}
	if (priv->flags & MV_ETH_F_CONNECT_LINUX) {
		/* connect to port interrupt line */
		if (request_irq(dev->irq, mv_eth_isr, (IRQF_DISABLED|IRQF_SAMPLE_RANDOM), "mv_eth", priv)) {
			printk(KERN_ERR "cannot request irq %d for %s port %d\n", dev->irq, dev->name, priv->port);
			if (priv->flags & MV_ETH_F_CONNECT_LINUX)
				napi_disable(priv->napiGroup[CPU_GROUP_DEF]);
			goto error;
		}

		/* unmask interrupts */
		on_each_cpu(mv_eth_interrupts_unmask, priv, 1);

		printk(KERN_NOTICE "%s: started\n", dev->name);
	}
	return 0;
error:
	printk(KERN_ERR "%s: start failed\n", dev->name);
	return -1;
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

	/* first make sure that the port finished its Rx polling - see tg3 */
	for (group = 0; group < CONFIG_MV_ETH_NAPI_GROUPS; group++)
		napi_disable(priv->napiGroup[group]);

	/* stop upper layer */
	netif_carrier_off(dev);
	netif_tx_stop_all_queues(dev);

	/* stop tx/rx activity, mask all interrupts, relese skb in rings,*/
	mv_eth_stop_internals(priv);
	for_each_possible_cpu(cpu) {
		cpuCtrl = priv->cpu_config[cpu];
		del_timer(&cpuCtrl->tx_done_timer);
		clear_bit(MV_ETH_F_TX_DONE_TIMER_BIT, &(cpuCtrl->flags));
		del_timer(&cpuCtrl->cleanup_timer);
		clear_bit(MV_ETH_F_CLEANUP_TIMER_BIT, &(cpuCtrl->flags));
	}

	if (dev->irq != 0)
		free_irq(dev->irq, priv);

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

		printk(KERN_NOTICE "%s: change mtu %d (buffer-size %d) to %d (buffer-size %d)\n",
				dev->name, old_mtu, RX_PKT_SIZE(old_mtu),
				dev->mtu, RX_PKT_SIZE(dev->mtu));
		return 0;
	}

	if (mv_eth_check_mtu_internals(dev, mtu))
		goto error;

	if (mv_eth_stop(dev)) {
		printk(KERN_ERR "%s: stop interface failed\n", dev->name);
		goto error;
	}

	if (mv_eth_change_mtu_internals(dev, mtu) == -1) {
		printk(KERN_ERR "%s change mtu internals failed\n", dev->name);
		goto error;
	}

	if (mv_eth_start(dev)) {
		printk(KERN_ERR "%s: start interface failed\n", dev->name);
		goto error;
	}
	printk(KERN_NOTICE "%s: change mtu %d (buffer-size %d) to %d (buffer-size %d)\n",
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
	struct eth_port *priv = MV_ETH_PRIV(dev);
	u8              *mac = &(((u8 *)addr)[2]);  /* skip on first 2B (ether HW addr type) */
	int             i;

#ifdef CONFIG_MV_ETH_PNC
	if (mv_eth_pnc_ctrl_en) {
		if (pnc_mac_me(priv->port, mac, CONFIG_MV_ETH_RXQ_DEF)) {
			printk(KERN_ERR "%s: ethSetMacAddr failed\n", dev->name);
			return -1;
		}
	} else {
		printk(KERN_ERR "%s: PNC control is disabled\n", __func__);
		return -1;
	}
#else
	/* remove previous address table entry */
	if (mvNetaMacAddrSet(priv->port, dev->dev_addr, -1) != MV_OK) {
		printk(KERN_ERR "%s: ethSetMacAddr failed\n", dev->name);
		return -1;
	}

	/* set new addr in hw */
	if (mvNetaMacAddrSet(priv->port, mac, CONFIG_MV_ETH_RXQ_DEF) != MV_OK) {
		printk(KERN_ERR "%s: ethSetMacAddr failed\n", dev->name);
		return -1;
	}
#endif /* CONFIG_MV_ETH_PNC */

	/* set addr in the device */
	for (i = 0; i < 6; i++)
		dev->dev_addr[i] = mac[i];

	printk(KERN_NOTICE "%s: mac address changed\n", dev->name);

	return 0;
}

#ifdef CONFIG_MV_ETH_PNC
void mv_eth_set_multicast_list(struct net_device *dev)
{
	struct eth_port     *priv = MV_ETH_PRIV(dev);
	int                 rxq = CONFIG_MV_ETH_RXQ_DEF;
/*
	printk("%s - mv_eth_set_multicast_list: flags=0x%x, mc_count=%d\n",
		dev->name, dev->flags, dev->mc_count);
*/
	if (!mv_eth_pnc_ctrl_en) {
		printk(KERN_ERR "%s: PNC control is disabled\n", __func__);
		return;
	}

	if (dev->flags & IFF_PROMISC) {
		/* Accept all */
		pnc_mac_me(priv->port, NULL, rxq);
		pnc_mcast_all(priv->port, 1);
	} else {
		/* Accept Unicast to me */
		pnc_mac_me(priv->port, dev->dev_addr, rxq);

		if (dev->flags & IFF_ALLMULTI) {
			/* Accept all multicast */
			pnc_mcast_all(priv->port, 1);
		} else {
			/* Accept only initialized Multicast */
			pnc_mcast_all(priv->port, 0);
			pnc_mcast_me(priv->port, NULL);

			/* Number of entires for all ports is restricted by CONFIG_MV_ETH_PNC_MCAST_NUM */
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 34)
			if (!netdev_mc_empty(dev)) {
				struct netdev_hw_addr *ha;

				netdev_for_each_mc_addr(ha, dev) {
					if (pnc_mcast_me(priv->port, ha->addr)) {
						printk(KERN_ERR "%s: Mcast init failed\n", dev->name);
						break;
					}
				}
			}
#else
			{
				struct dev_mc_list *curr_addr = dev->mc_list;
				int                i;
				for (i = 0; i < dev->mc_count; i++, curr_addr = curr_addr->next) {
					if (!curr_addr)
						break;
					if (pnc_mcast_me(priv->port, curr_addr->dmi_addr)) {
						printk(KERN_ERR "%s: Mcast init failed - %d of %d\n",
							dev->name, i, dev->mc_count);
						break;
					}
				}
			}
#endif /* KERNEL_VERSION >= 2.6.34 */
		}
	}
}
#else /* !CONFIG_MV_ETH_PNC - legacy parser */
void mv_eth_set_multicast_list(struct net_device *dev)
{
	struct eth_port    *priv = MV_ETH_PRIV(dev);
	int                queue = CONFIG_MV_ETH_RXQ_DEF;

	if (dev->flags & IFF_PROMISC) {
		/* Accept all: Multicast + Unicast */
		mvNetaRxUnicastPromiscSet(priv->port, MV_TRUE);
		mvNetaSetUcastTable(priv->port, queue);
		mvNetaSetSpecialMcastTable(priv->port, queue);
		mvNetaSetOtherMcastTable(priv->port, queue);
	} else {
		/* Accept single Unicast */
		mvNetaRxUnicastPromiscSet(priv->port, MV_FALSE);
		mvNetaSetUcastTable(priv->port, -1);
		if (mvNetaMacAddrSet(priv->port, dev->dev_addr, queue) != MV_OK)
			printk(KERN_ERR "%s: netaSetMacAddr failed\n", dev->name);

		if (dev->flags & IFF_ALLMULTI) {
			/* Accept all Multicast */
			mvNetaSetSpecialMcastTable(priv->port, queue);
			mvNetaSetOtherMcastTable(priv->port, queue);
		} else {
			/* Accept only initialized Multicast */
			mvNetaSetSpecialMcastTable(priv->port, -1);
			mvNetaSetOtherMcastTable(priv->port, -1);

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 34)
			if (!netdev_mc_empty(dev)) {
				struct netdev_hw_addr *ha;

				netdev_for_each_mc_addr(ha, dev) {
					mvNetaMcastAddrSet(priv->port, ha->addr, queue);
				}
			}
#else
			{
				struct dev_mc_list *curr_addr = dev->mc_list;
				int                i;
				for (i = 0; i < dev->mc_count; i++, curr_addr = curr_addr->next) {
					if (!curr_addr)
						break;
					mvNetaMcastAddrSet(priv->port, curr_addr->dmi_addr, queue);
				}
			}
#endif /* KERNEL_VERSION >= 2.6.34 */
		}
	}
}
#endif /* CONFIG_MV_ETH_PNC */


int     mv_eth_set_mac_addr(struct net_device *dev, void *addr)
{
	if (!netif_running(dev)) {
		if (mv_eth_set_mac_addr_internals(dev, addr) == -1)
			goto error;
		return 0;
	}

	if (mv_eth_stop(dev)) {
		printk(KERN_ERR "%s: stop interface failed\n", dev->name);
		goto error;
	}

	if (mv_eth_set_mac_addr_internals(dev, addr) == -1)
		goto error;

	if (mv_eth_start(dev)) {
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
	struct eth_port	*priv = MV_ETH_PRIV(dev);
	int         queue = CONFIG_MV_ETH_RXQ_DEF;

#ifdef CONFIG_MV_ETH_PNC
	if (mv_eth_pnc_ctrl_en) {
		if (pnc_mac_me(priv->port, dev->dev_addr, queue)) {
			printk(KERN_ERR "%s: ethSetMacAddr failed\n", dev->name);
			return -1;
		}
	} else
		printk(KERN_ERR "%s: PNC control is disabled\n", __func__);
#else /* Legacy parser */
	if (mvNetaMacAddrSet(priv->port, dev->dev_addr, queue) != MV_OK) {
		printk(KERN_ERR "%s: ethSetMacAddr failed\n", dev->name);
		return -1;
	}
#endif /* CONFIG_MV_ETH_PNC */

	if (mv_eth_start(dev)) {
		printk(KERN_ERR "%s: start interface failed\n", dev->name);
		return -1;
	}
	return 0;
}
