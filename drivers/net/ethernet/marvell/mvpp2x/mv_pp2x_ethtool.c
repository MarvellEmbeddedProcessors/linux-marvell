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
#include <linux/version.h>
#include <linux/of.h>
#include <linux/of_irq.h>
#include <linux/of_mdio.h>
#include <linux/of_net.h>
#include <linux/of_address.h>
#include <linux/phy.h>
#include <linux/clk.h>
#include <uapi/linux/ppp_defs.h>
#include <net/ip.h>
#include <net/ipv6.h>

#include "mv_pp2x.h"
#include "mv_pp2x_hw.h"
#include "mv_gop110_hw.h"

/* Ethtool methods */

#ifdef CONFIG_MV_PP2_FPGA

#define ETH_MIB_GOOD_OCTETS_RECEIVED_LOW    0x0
#define ETH_MIB_GOOD_OCTETS_RECEIVED_HIGH   0x4
#define ETH_MIB_BAD_OCTETS_RECEIVED         0x8
#define ETH_MIB_INTERNAL_MAC_TRANSMIT_ERR   0xc
#define ETH_MIB_GOOD_FRAMES_RECEIVED        0x10
#define ETH_MIB_BAD_FRAMES_RECEIVED         0x14
#define ETH_MIB_BROADCAST_FRAMES_RECEIVED   0x18
#define ETH_MIB_MULTICAST_FRAMES_RECEIVED   0x1c
#define ETH_MIB_FRAMES_64_OCTETS            0x20
#define ETH_MIB_FRAMES_65_TO_127_OCTETS     0x24
#define ETH_MIB_FRAMES_128_TO_255_OCTETS    0x28
#define ETH_MIB_FRAMES_256_TO_511_OCTETS    0x2c
#define ETH_MIB_FRAMES_512_TO_1023_OCTETS   0x30
#define ETH_MIB_FRAMES_1024_TO_MAX_OCTETS   0x34
#define ETH_MIB_GOOD_OCTETS_SENT_LOW        0x38
#define ETH_MIB_GOOD_OCTETS_SENT_HIGH       0x3c
#define ETH_MIB_GOOD_FRAMES_SENT            0x40
#define ETH_MIB_EXCESSIVE_COLLISION         0x44
#define ETH_MIB_MULTICAST_FRAMES_SENT       0x48
#define ETH_MIB_BROADCAST_FRAMES_SENT       0x4c
#define ETH_MIB_UNREC_MAC_CONTROL_RECEIVED  0x50
#define ETH_MIB_FC_SENT                     0x54
#define ETH_MIB_GOOD_FC_RECEIVED            0x58
#define ETH_MIB_BAD_FC_RECEIVED             0x5c
#define ETH_MIB_UNDERSIZE_RECEIVED          0x60
#define ETH_MIB_FRAGMENTS_RECEIVED          0x64
#define ETH_MIB_OVERSIZE_RECEIVED           0x68
#define ETH_MIB_JABBER_RECEIVED             0x6c
#define ETH_MIB_MAC_RECEIVE_ERROR           0x70
#define ETH_MIB_BAD_CRC_EVENT               0x74
#define ETH_MIB_COLLISION                   0x78
#define ETH_MIB_LATE_COLLISION              0x7c

#endif

/* Get settings (phy address, speed) for ethtools */
static int mv_pp2x_ethtool_get_settings(struct net_device *dev,
					struct ethtool_cmd *cmd)
{
	struct mv_pp2x_port *port = netdev_priv(dev);

#ifdef CONFIG_MV_PP2_FPGA
	int val;
	void *addr;
	int port_id = port->id;
	void *mv_pp2_vfpga_address;

	pr_emerg("\n\n\n\nmv_pp2x_ethtool_get_drvinfo(%d):dev->name=%s port->id=%d\n",
		 __LINE__, dev->name, port->id);

	mv_pp2_vfpga_address = mv_pp2_vfpga_address_get();

	addr = mv_pp2_vfpga_address + 0x100000 + 0x1000 + (port_id * 0x400) +
			ETH_MIB_GOOD_FRAMES_SENT;
	val   = readl(addr);
	pr_emerg("ETH_MIB_GOOD_FRAMES_SENT          =%d  : 0x%p\n", val, addr);

	addr = mv_pp2_vfpga_address + 0x100000 + 0x1000 + (port_id * 0x400) +
			ETH_MIB_GOOD_FRAMES_RECEIVED;
	val   = readl(addr);
	pr_emerg("ETH_MIB_GOOD_FRAMES_RECEIVED      =%d  : 0x%p\n", val, addr);

	addr = mv_pp2_vfpga_address + 0x100000 + 0x1000 + (port_id * 0x400) +
			ETH_MIB_GOOD_OCTETS_RECEIVED_LOW;
	val  = readl(addr);
	pr_emerg("ETH_MIB_GOOD_OCTETS_RECEIVED_LOW  =%d  : 0x%p\n", val, addr);

	addr = mv_pp2_vfpga_address + 0x100000 + 0x1000 + (port_id * 0x400) +
			ETH_MIB_GOOD_OCTETS_RECEIVED_HIGH;
	val  = readl(addr);
	pr_emerg("ETH_MIB_GOOD_OCTETS_RECEIVED_HIGH =%d  : 0x%p\n", val, addr);

	addr = mv_pp2_vfpga_address + 0x100000 + 0x1000 + (port_id * 0x400) +
			ETH_MIB_BAD_OCTETS_RECEIVED;
	val  = readl(addr);
	pr_emerg("ETH_MIB_BAD_OCTETS_RECEIVED       =%d  : 0x%p\n", val, addr);

	addr = mv_pp2_vfpga_address + 0x100000 + 0x1000 + (port_id * 0x400) +
			ETH_MIB_INTERNAL_MAC_TRANSMIT_ERR;
	val  = readl(addr);
	pr_emerg("ETH_MIB_INTERNAL_MAC_TRANSMIT_ERR =%d  : 0x%p\n", val, addr);

	addr = mv_pp2_vfpga_address + 0x100000 + 0x1000 + (port_id * 0x400) +
			ETH_MIB_BAD_FRAMES_RECEIVED;
	val  = readl(addr);
	pr_emerg("ETH_MIB_BAD_FRAMES_RECEIVED       =%d  : 0x%p\n", val, addr);

	addr = mv_pp2_vfpga_address + 0x100000 + 0x1000 + (port_id * 0x400) +
			ETH_MIB_BROADCAST_FRAMES_RECEIVED;
	val  = readl(addr);
	pr_emerg("ETH_MIB_BROADCAST_FRAMES_RECEIVED =%d  : 0x%p\n", val, addr);

	addr = mv_pp2_vfpga_address + 0x100000 + 0x1000 + (port_id * 0x400) +
			ETH_MIB_MULTICAST_FRAMES_RECEIVED;
	val  = readl(addr);
	pr_emerg("ETH_MIB_MULTICAST_FRAMES_RECEIVED =%d  : 0x%p\n", val, addr);

	addr = mv_pp2_vfpga_address + 0x100000 + 0x1000 + (port_id * 0x400) +
			ETH_MIB_FRAMES_64_OCTETS;
	val  = readl(addr);
	pr_emerg("ETH_MIB_FRAMES_64_OCTETS          =%d  : 0x%p\n", val, addr);

	addr = mv_pp2_vfpga_address + 0x100000 + 0x1000 + (port_id * 0x400) +
			ETH_MIB_GOOD_OCTETS_SENT_LOW;
	val  = readl(addr);
	pr_emerg("ETH_MIB_GOOD_OCTETS_SENT_LOW      =%d  : 0x%p\n", val, addr);

	addr = mv_pp2_vfpga_address + 0x100000 + 0x1000 + (port_id * 0x400) +
			ETH_MIB_EXCESSIVE_COLLISION;
	val  = readl(addr);
	pr_emerg("ETH_MIB_EXCESSIVE_COLLISION       =%d  : 0x%p\n", val, addr);

	addr = mv_pp2_vfpga_address + 0x100000 + 0x1000 + (port_id * 0x400) +
			ETH_MIB_BROADCAST_FRAMES_SENT;
	val  = readl(addr);
	pr_emerg("ETH_MIB_BROADCAST_FRAMES_SENT     =%d  : 0x%p\n", val, addr);

	addr = mv_pp2_vfpga_address + 0x100000 + 0x1000 + (port_id * 0x400) +
			ETH_MIB_UNREC_MAC_CONTROL_RECEIVED;
	val  = readl(addr);
	pr_emerg("ETH_MIB_UNREC_MAC_CONTROL_RECEIVED=%d  : 0x%p\n", val, addr);

	addr = mv_pp2_vfpga_address + 0x100000 + 0x1000 + (port_id * 0x400) +
			ETH_MIB_FC_SENT;
	val  = readl(addr);
	pr_emerg("ETH_MIB_FC_SENT                   =%d  : 0x%p\n", val, addr);

	addr = mv_pp2_vfpga_address + 0x100000 + 0x1000 + (port_id * 0x400) +
			ETH_MIB_GOOD_FC_RECEIVED;
	val  = readl(addr);
	pr_emerg("ETH_MIB_GOOD_FC_RECEIVED          =%d  : 0x%p\n", val, addr);

	addr = mv_pp2_vfpga_address + 0x100000 + 0x1000 + (port_id * 0x400) +
			ETH_MIB_BAD_FC_RECEIVED;
	val  = readl(addr);
	pr_emerg("ETH_MIB_BAD_FC_RECEIVED           =%d  : 0x%p\n", val, addr);

	addr = mv_pp2_vfpga_address + 0x100000 + 0x1000 + (port_id * 0x400) +
			ETH_MIB_UNDERSIZE_RECEIVED;
	val  = readl(addr);
	pr_emerg("ETH_MIB_UNDERSIZE_RECEIVED        =%d  : 0x%p\n", val, addr);

	addr = mv_pp2_vfpga_address + 0x100000 + 0x1000 + (port_id * 0x400) +
			ETH_MIB_OVERSIZE_RECEIVED;
	val  = readl(addr);
	pr_emerg("ETH_MIB_OVERSIZE_RECEIVED         =%d  : 0x%p\n", val, addr);

	addr = mv_pp2_vfpga_address + 0x100000 + 0x1000 + (port_id * 0x400) +
			ETH_MIB_JABBER_RECEIVED;
	val  = readl(addr);
	pr_emerg("ETH_MIB_JABBER_RECEIVED           =%d  : 0x%p\n", val, addr);

	addr = mv_pp2_vfpga_address + 0x100000 + 0x1000 + (port_id * 0x400) +
			ETH_MIB_MAC_RECEIVE_ERROR;
	val  = readl(addr);
	pr_emerg("ETH_MIB_MAC_RECEIVE_ERROR         =%d  : 0x%p\n", val, addr);

	addr = mv_pp2_vfpga_address + 0x100000 + 0x1000 + (port_id * 0x400) +
			ETH_MIB_BAD_CRC_EVENT;
	val  = readl(addr);
	pr_emerg("ETH_MIB_BAD_CRC_EVENT             =%d  : 0x%p\n", val, addr);

	addr = mv_pp2_vfpga_address + 0x100000 + 0x1000 + (port_id * 0x400) +
			ETH_MIB_COLLISION;
	val  = readl(addr);
	pr_emerg("ETH_MIB_COLLISION                 =%d  : 0x%p\n", val, addr);

	addr = mv_pp2_vfpga_address + 0x100000 + 0x1000 + (port_id * 0x400) +
			ETH_MIB_LATE_COLLISION;
	val  = readl(addr);
	pr_emerg("ETH_MIB_LATE_COLLISION            =%d  : 0x%p\n", val, addr);

	val = readl(port->base + 0x10);
	pr_emerg("print_reg(%d):port_id=%d: [0x%p] = 0x%x\n", __LINE__,
		 port_id, port->base + 0x10, val);
	return 0;
#else
	struct mv_port_link_status	status;
	phy_interface_t			phy_mode;

	mv_gop110_mib_counters_show(&port->priv->hw.gop,
		port->mac_data.gop_index);

	/* No Phy device mngmt */
	if (!port->mac_data.phy_dev) {
		/*for force link port, RXAUI port and link-down ports,
		 * follow old strategy
		 */

		mv_gop110_port_link_status(&port->priv->hw.gop,
					   &port->mac_data, &status);

		if (status.linkup == true) {
			switch (status.speed) {
			case MV_PORT_SPEED_10000:
				cmd->speed = SPEED_10000;
				break;
			case MV_PORT_SPEED_1000:
				cmd->speed = SPEED_1000;
				break;
			case MV_PORT_SPEED_100:
				cmd->speed = SPEED_100;
				break;
			case MV_PORT_SPEED_10:
				cmd->speed = SPEED_10;
				break;
			default:
				return -EINVAL;
			}
			if (status.duplex == MV_PORT_DUPLEX_FULL)
				cmd->duplex = 1;
			else
				cmd->duplex = 0;
		} else {
			cmd->speed  = SPEED_UNKNOWN;
			cmd->duplex = SPEED_UNKNOWN;
		}

		phy_mode = port->mac_data.phy_mode;
		if ((phy_mode == PHY_INTERFACE_MODE_XAUI) ||
		    (phy_mode == PHY_INTERFACE_MODE_RXAUI)) {
			cmd->autoneg = AUTONEG_DISABLE;
			cmd->supported = (SUPPORTED_10000baseT_Full |
				SUPPORTED_FIBRE);
			cmd->advertising = (ADVERTISED_10000baseT_Full |
				ADVERTISED_FIBRE);
			cmd->port = PORT_FIBRE;
			cmd->transceiver = XCVR_EXTERNAL;
		} else {
			cmd->supported = (SUPPORTED_10baseT_Half |
				SUPPORTED_10baseT_Full |
				SUPPORTED_100baseT_Half	|
				SUPPORTED_100baseT_Full |
				SUPPORTED_Autoneg | SUPPORTED_TP |
				SUPPORTED_MII |	SUPPORTED_1000baseT_Full);
			cmd->transceiver = XCVR_INTERNAL;
			cmd->port = PORT_MII;

			/* check if speed and duplex are AN */
			if (status.speed == MV_PORT_SPEED_AN &&
			    status.duplex == MV_PORT_DUPLEX_AN) {
				cmd->lp_advertising = cmd->advertising = 0;
				cmd->autoneg = AUTONEG_ENABLE;
			} else {
				cmd->autoneg = AUTONEG_DISABLE;
			}
		}

		return 0;
	}

	return phy_ethtool_gset(port->mac_data.phy_dev, cmd);
#endif
}

/* Set settings (phy address, speed) for ethtools */
static int mv_pp2x_ethtool_set_settings(struct net_device *dev,
					struct ethtool_cmd *cmd)
{
	struct mv_pp2x_port *port = netdev_priv(dev);

	if (!port->mac_data.phy_dev)
		return -ENODEV;
#if !defined(CONFIG_MV_PP2_PALLADIUM)
	return phy_ethtool_sset(port->mac_data.phy_dev, cmd);
#else
	return 0;
#endif
}

/* Set interrupt coalescing for ethtools */
static int mv_pp2x_ethtool_set_coalesce(struct net_device *dev,
					struct ethtool_coalesce *c)
{
	struct mv_pp2x_port *port = netdev_priv(dev);
	int queue;

	for (queue = 0; queue < port->num_rx_queues; queue++) {
		struct mv_pp2x_rx_queue *rxq = port->rxqs[queue];

		rxq->time_coal = c->rx_coalesce_usecs;
		rxq->pkts_coal = c->rx_max_coalesced_frames;
		mv_pp2x_rx_pkts_coal_set(port, rxq, rxq->pkts_coal);
		mv_pp2x_rx_time_coal_set(port, rxq, rxq->time_coal);
	}
	port->tx_time_coal = c->tx_coalesce_usecs;
	for (queue = 0; queue < port->num_tx_queues; queue++) {
		struct mv_pp2x_tx_queue *txq = port->txqs[queue];

		txq->pkts_coal = c->tx_max_coalesced_frames;
	}
	if (port->priv->pp2xdata->interrupt_tx_done == true) {
		mv_pp2x_tx_done_time_coal_set(port, port->tx_time_coal);
		on_each_cpu(mv_pp2x_tx_done_pkts_coal_set, port, 1);
	}

	return 0;
}

/* get coalescing for ethtools */
static int mv_pp2x_ethtool_get_coalesce(struct net_device *dev,
					struct ethtool_coalesce *c)
{
	struct mv_pp2x_port *port = netdev_priv(dev);

	c->rx_coalesce_usecs        = port->rxqs[0]->time_coal;
	c->rx_max_coalesced_frames  = port->rxqs[0]->pkts_coal;
	c->tx_max_coalesced_frames  = port->txqs[0]->pkts_coal;
	c->tx_coalesce_usecs        = port->tx_time_coal;

	return 0;
}

static void mv_pp2x_ethtool_get_drvinfo(struct net_device *dev,
					struct ethtool_drvinfo *drvinfo)
{
	strlcpy(drvinfo->driver, MVPP2_DRIVER_NAME,
		sizeof(drvinfo->driver));
	strlcpy(drvinfo->version, MVPP2_DRIVER_VERSION,
		sizeof(drvinfo->version));
	strlcpy(drvinfo->bus_info, dev_name(&dev->dev),
		sizeof(drvinfo->bus_info));
}

static void mv_pp2x_ethtool_get_ringparam(struct net_device *dev,
					  struct ethtool_ringparam *ring)
{
	struct mv_pp2x_port *port = netdev_priv(dev);

	ring->rx_max_pending = MVPP2_MAX_RXD;
	ring->tx_max_pending = MVPP2_MAX_TXD;
	ring->rx_pending = port->rx_ring_size;
	ring->tx_pending = port->tx_ring_size;
}

static int mv_pp2x_ethtool_set_ringparam(struct net_device *dev,
					 struct ethtool_ringparam *ring)
{
	struct mv_pp2x_port *port = netdev_priv(dev);
	u16 prev_rx_ring_size = port->rx_ring_size;
	u16 prev_tx_ring_size = port->tx_ring_size;
	int err;

	err = mv_pp2x_check_ringparam_valid(dev, ring);
	if (err)
		return err;

	if (!netif_running(dev)) {
		port->rx_ring_size = ring->rx_pending;
		port->tx_ring_size = ring->tx_pending;
		return 0;
	}

	/* The interface is running, so we have to force a
	 * reallocation of the queues
	 */
	mv_pp2x_stop_dev(port);
	mv_pp2x_cleanup_rxqs(port);
	mv_pp2x_cleanup_txqs(port);

	port->rx_ring_size = ring->rx_pending;
	port->tx_ring_size = ring->tx_pending;

	err = mv_pp2x_setup_rxqs(port);
	if (err) {
		/* Reallocate Rx queues with the original ring size */
		port->rx_ring_size = prev_rx_ring_size;
		ring->rx_pending = prev_rx_ring_size;
		err = mv_pp2x_setup_rxqs(port);
		if (err)
			goto err_out;
	}
	err = mv_pp2x_setup_txqs(port);
	if (err) {
		/* Reallocate Tx queues with the original ring size */
		port->tx_ring_size = prev_tx_ring_size;
		ring->tx_pending = prev_tx_ring_size;
		err = mv_pp2x_setup_txqs(port);
		if (err)
			goto err_clean_rxqs;
	}

	mv_pp2x_start_dev(port);

	return 0;

err_clean_rxqs:
	mv_pp2x_cleanup_rxqs(port);
err_out:
	netdev_err(dev, "fail to change ring parameters");
	return err;
}

static u32 mv_pp2x_ethtool_get_rxfh_indir_size(struct net_device *dev)
{
	struct mv_pp2x_port *port = netdev_priv(dev);

	return ARRAY_SIZE(port->priv->rx_indir_table);
}

static int mv_pp2x_ethtool_get_rxnfc(struct net_device *dev,
				     struct ethtool_rxnfc *info,
				     u32 *rules)
{
	struct mv_pp2x_port *port = netdev_priv(dev);

	if (port->priv->pp2_cfg.queue_mode == MVPP2_QDIST_SINGLE_MODE)
		return -EOPNOTSUPP;

	if (info->cmd == ETHTOOL_GRXRINGS) {
		if (port)
			info->data = ARRAY_SIZE(port->priv->rx_indir_table);
	}
	return 0;
}

static const struct ethtool_ops mv_pp2x_eth_tool_ops = {
	.get_link		= ethtool_op_get_link,
	.get_settings		= mv_pp2x_ethtool_get_settings,
	.set_settings		= mv_pp2x_ethtool_set_settings,
	.set_coalesce		= mv_pp2x_ethtool_set_coalesce,
	.get_coalesce		= mv_pp2x_ethtool_get_coalesce,
	.get_drvinfo		= mv_pp2x_ethtool_get_drvinfo,
	.get_ringparam		= mv_pp2x_ethtool_get_ringparam,
	.set_ringparam		= mv_pp2x_ethtool_set_ringparam,
	/* For rxfh relevant command, only support LK-3.18 */
	.get_rxfh_indir_size	= mv_pp2x_ethtool_get_rxfh_indir_size,
	.get_rxnfc		= mv_pp2x_ethtool_get_rxnfc,
};

void mv_pp2x_set_ethtool_ops(struct net_device *netdev)
{
	netdev->ethtool_ops = &mv_pp2x_eth_tool_ops;
}
