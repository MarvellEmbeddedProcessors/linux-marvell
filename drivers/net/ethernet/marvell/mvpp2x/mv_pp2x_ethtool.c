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

#define MV_PP2_STATS_LEN	ARRAY_SIZE(mv_pp2x_gstrings_stats)
#define MV_PP2_TEST_LEN		ARRAY_SIZE(mv_pp2x_gstrings_test)
#define MV_PP2_REGS_GMAC_LEN	54
#define MV_PP2_REGS_XLG_LEN	25
#define MV_PP2_TEST_MASK1	0xFFFF
#define MV_PP2_TEST_MASK2	0x00FE
#define MV_PP2_TEST_MASK3	0x0
#define MV_PP2_TEST_PATTERN1	0xFFFF
#define MV_PP2_TEST_PATTERN2	0x00FE
#define MV_PP2_TEST_PATTERN3	0x0

static const char mv_pp2x_gstrings_test[][ETH_GSTRING_LEN] = {
	"Link test        (on/offline)",
	"register test    (on/offline)",
};

static const char mv_pp2x_gstrings_stats[][ETH_GSTRING_LEN] = {
	/* device-specific stats */
	"rx_packets", "rx_bytes", "tx_packets", "tx_bytes",
};

int mv_pp2x_check_speed_duplex_valid(struct ethtool_cmd *cmd,
					struct mv_port_link_status *pstatus)
{
	switch (cmd->duplex) {
	case DUPLEX_FULL:
		pstatus->duplex = MV_PORT_DUPLEX_FULL;
		break;
	case DUPLEX_HALF:
		pstatus->duplex = MV_PORT_DUPLEX_HALF;
		break;
	default:
		pr_err("Wrong duplex configuration\n");
		return -1;
	}

	switch (cmd->speed) {
	case SPEED_100:
		pstatus->speed = MV_PORT_SPEED_100;
		return 0;
	case SPEED_10:
		pstatus->speed = MV_PORT_SPEED_10;
		return 0;
	case SPEED_1000:
		pstatus->speed = MV_PORT_SPEED_1000;
		if (cmd->duplex)
			return 0;
		pr_err("1G port doesn't support half duplex\n");
		return -1;
	default:
		pr_err("Wrong speed configuration\n");
		return -1;
	}
}


int mv_pp2x_autoneg_check_valid(struct mv_mac_data *mac, struct gop_hw *gop,
			struct ethtool_cmd *cmd, struct mv_port_link_status *pstatus)
{

	int port_num = mac->gop_index;
	int err;

	switch (mac->phy_mode) {
	case PHY_INTERFACE_MODE_RGMII:
	case PHY_INTERFACE_MODE_SGMII:
	case PHY_INTERFACE_MODE_QSGMII:
		err = mv_gop110_check_port_type(gop, port_num);
		if (err) {
			pr_err("GOP %d set to 1000Base-X and cannot be changed\n", port_num);
			return -EINVAL;
		}
		if (!cmd->autoneg) {
			err = mv_pp2x_check_speed_duplex_valid(cmd, pstatus);
			if (err)
				return -EINVAL;
		}
	break;
	case PHY_INTERFACE_MODE_XAUI:
	case PHY_INTERFACE_MODE_RXAUI:
	case PHY_INTERFACE_MODE_KR:
		pr_err("XLG GOP %d doesn't support autonegotiation\n", port_num);
		return -ENODEV;

	break;
	default:
		pr_err("%s: Wrong port mode (%d)\n", __func__, mac->phy_mode);
		return -1;
	}
	return 0;

}

/* Ethtool methods */

/* Ethtool statistic */
static void mv_pp2x_eth_tool_get_ethtool_stats(struct net_device *dev,
	struct ethtool_stats *stats, u64 *data)
{

	struct mv_pp2x_port *port = netdev_priv(dev);
	int cpu = 0;

	data[0] = 0;

	for_each_possible_cpu(cpu) {
		struct mv_pp2x_pcpu_stats *stats = per_cpu_ptr(port->stats, cpu);

		u64_stats_update_begin(&stats->syncp);
		data[0] += stats->rx_packets;
		data[1] += stats->rx_bytes;
		data[2] += stats->tx_packets;
		data[3] += stats->tx_bytes;
		u64_stats_update_end(&stats->syncp);
		}
}

static void mv_pp2x_eth_tool_get_strings(struct net_device *dev,
					u32 stringset, u8 *data)
{

	switch (stringset) {
	case ETH_SS_TEST:
		memcpy(data, *mv_pp2x_gstrings_test, sizeof(mv_pp2x_gstrings_test));
		break;
	case ETH_SS_STATS:
		memcpy(data, *mv_pp2x_gstrings_stats, sizeof(mv_pp2x_gstrings_stats));
		break;
	default:
		break;
		}

}

static int mv_pp2x_eth_tool_get_sset_count(struct net_device *dev, int sset)
{

	switch (sset) {
	case ETH_SS_TEST:
		return MV_PP2_TEST_LEN;
	case ETH_SS_STATS:
		return MV_PP2_STATS_LEN;
	default:
		return -EOPNOTSUPP;
	}

}

/* Restart autonegotiation function */
int mv_pp2x_eth_tool_nway_reset(struct net_device *dev)
{
	struct mv_pp2x_port *port = netdev_priv(dev);
	struct gop_hw *gop = &port->priv->hw.gop;
	struct mv_mac_data *mac = &port->mac_data;
	int err;

	if (!(mac->flags & MV_EMAC_F_INIT)) {
		pr_err("%s: interface %s is not initialized\n", __func__, dev->name);
		return -EOPNOTSUPP;
	}

	switch (mac->phy_mode) {
	case PHY_INTERFACE_MODE_RGMII:
	case PHY_INTERFACE_MODE_SGMII:
	case PHY_INTERFACE_MODE_QSGMII:
		err = mv_gop110_check_port_type(gop, mac->gop_index);
		if (err) {
			pr_err("GOP %d set to 1000Base-X\n", mac->gop_index);
			return -EINVAL;
		}
		mv_gop110_autoneg_restart(gop, mac);
	break;
	case PHY_INTERFACE_MODE_XAUI:
	case PHY_INTERFACE_MODE_RXAUI:
	case PHY_INTERFACE_MODE_KR:
		pr_err("XLG GOP %d doesn't support autonegotiation\n", mac->gop_index);
		return -ENODEV;
	break;
	default:
		pr_err("%s: Wrong port mode (%d)\n", __func__, mac->phy_mode);
		return -1;
	}

	return 0;
}

/* Get settings (phy address, speed) for ethtools */
static int mv_pp2x_ethtool_get_settings(struct net_device *dev,
					struct ethtool_cmd *cmd)
{
	struct mv_pp2x_port *port = netdev_priv(dev);
	struct mv_port_link_status	status;
	phy_interface_t			phy_mode;

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
		    (phy_mode == PHY_INTERFACE_MODE_RXAUI) ||
		    (phy_mode == PHY_INTERFACE_MODE_KR)) {
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
			cmd->advertising = (ADVERTISED_10baseT_Half |
				ADVERTISED_10baseT_Full |
				ADVERTISED_100baseT_Half |
				ADVERTISED_100baseT_Full |
				ADVERTISED_1000baseT_Full |
				ADVERTISED_Autoneg | ADVERTISED_TP |
				ADVERTISED_MII);
			cmd->transceiver = XCVR_INTERNAL;
			cmd->port = PORT_MII;

			/* check if speed and duplex are AN */
			if (mv_gop110_port_autoneg_status(&port->priv->hw.gop,
					   &port->mac_data)) {
				cmd->autoneg = AUTONEG_ENABLE;
			} else {
				cmd->autoneg = AUTONEG_DISABLE;
			}
		}

		return 0;
	}

	return phy_ethtool_gset(port->mac_data.phy_dev, cmd);
}

/* Set settings (phy address, speed) for ethtools */
static int mv_pp2x_ethtool_set_settings(struct net_device *dev,
					struct ethtool_cmd *cmd)
{
	struct mv_pp2x_port *port = netdev_priv(dev);
	int err;
	struct mv_port_link_status status;
	struct gop_hw *gop = &port->priv->hw.gop;
	struct mv_mac_data *mac = &port->mac_data;
	int gop_port = mac->gop_index;

	err = mv_pp2x_autoneg_check_valid(mac, gop, cmd, &status);

	if (err < 0) {
		pr_err("Wrong negotiation mode set\n");
		return err;
	}

	mv_gop110_force_link_mode_set(gop, mac, false, true);
	mv_gop110_gmac_set_autoneg(gop, mac, cmd->autoneg);
	if (cmd->autoneg)
		mv_gop110_autoneg_restart(gop, mac);
	else
		mv_gop110_gmac_speed_duplex_set(gop, gop_port, status.speed, status.duplex);
	mv_gop110_force_link_mode_set(gop, mac, false, false);

	return 0;
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

static int mv_pp2x_ethtool_get_rxfh(struct net_device *dev, u32 *indir, u8 *key,
				    u8 *hfunc)
{
	size_t copy_size;
	struct mv_pp2x_port *port = netdev_priv(dev);

	/* Single mode doesn't support RSS features */
	if (port->priv->pp2_cfg.queue_mode == MVPP2_QDIST_SINGLE_MODE)
		return -EOPNOTSUPP;

	if (hfunc)
		*hfunc = ETH_RSS_HASH_TOP;

	if (!indir)
		return 0;

	copy_size = ARRAY_SIZE(port->priv->rx_indir_table);
	memcpy(indir, port->priv->rx_indir_table, copy_size * sizeof(u32));

	return 0;
}

static int mv_pp2x_ethtool_set_rxfh(struct net_device *dev, const u32 *indir,
				    const u8 *key, const u8 hfunc)
{
	int i, err;
	struct mv_pp2x_port *port = netdev_priv(dev);

	/* Single mode doesn't support RSS features */
	if (port->priv->pp2_cfg.queue_mode == MVPP2_QDIST_SINGLE_MODE)
		return -EOPNOTSUPP;

	/* We require at least one supported parameter to be changed
	 * and no change in any of the unsupported parameters
	 */
	if (key ||
	    (hfunc != ETH_RSS_HASH_NO_CHANGE && hfunc != ETH_RSS_HASH_TOP))
		return -EOPNOTSUPP;

	if (!indir)
		return 0;

	for (i = 0; i < ARRAY_SIZE(port->priv->rx_indir_table); i++)
		port->priv->rx_indir_table[i] = indir[i];

	err =  mv_pp22_rss_rxfh_indir_set(port);
	if (err) {
		netdev_err(dev, "fail to change rxfh indir table");
		return err;
	}

	return 0;
}

static int mv_pp2x_ethtool_get_regs_len(struct net_device *dev)
{
	struct mv_pp2x_port *port = netdev_priv(dev);
	struct mv_mac_data *mac = &port->mac_data;

	switch (mac->phy_mode) {
	case PHY_INTERFACE_MODE_RGMII:
	case PHY_INTERFACE_MODE_SGMII:
	case PHY_INTERFACE_MODE_QSGMII:
		return MV_PP2_REGS_GMAC_LEN * sizeof(u32);
	case PHY_INTERFACE_MODE_XAUI:
	case PHY_INTERFACE_MODE_RXAUI:
	case PHY_INTERFACE_MODE_KR:
		return MV_PP2_REGS_XLG_LEN * sizeof(u32);
	default:
		pr_err("%s: Wrong port mode (%d)", __func__, mac->phy_mode);
		return -1;
	}
}

/*ethtool get registers function */
static void mv_pp2x_ethtool_get_regs(struct net_device *dev,
				     struct ethtool_regs *regs, void *p)
{
	struct mv_pp2x_port *port = netdev_priv(dev);
	struct mv_mac_data *mac = &port->mac_data;

	if (!port) {
		netdev_err(dev, "%s is not supported on %s\n",
			   __func__, dev->name);
		return;
	}

	regs->version = port->priv->pp2_version;

	switch (mac->phy_mode) {
	case PHY_INTERFACE_MODE_RGMII:
	case PHY_INTERFACE_MODE_SGMII:
	case PHY_INTERFACE_MODE_QSGMII:
		memset(p, 0, MV_PP2_REGS_GMAC_LEN * sizeof(u32));
		mv_gop110_gmac_registers_dump(port, p);
	break;
	case PHY_INTERFACE_MODE_XAUI:
	case PHY_INTERFACE_MODE_RXAUI:
	case PHY_INTERFACE_MODE_KR:
		memset(p, 0, MV_PP2_REGS_XLG_LEN * sizeof(u32));
		mv_gop110_xlg_registers_dump(port, p);
	break;
	default:
		pr_err("%s: Wrong port mode (%d)", __func__, mac->phy_mode);
		return;
	}
}

static u64 mv_pp2x_eth_tool_link_test(struct mv_pp2x_port *port)
{
	struct mv_port_link_status	status;

	pr_info("Link testing starting\n");

	mv_gop110_port_link_status(&port->priv->hw.gop,
					&port->mac_data, &status);

	if (status.linkup)
		return 0;
	return 1;
}

static bool mv_pp2x_reg_pattern_test(void *reg, u32 offset, u32 mask, u32 write)
{
	static const u32 test[] = {0x5A5A5A5A, 0xA5A5A5A5, 0x00000000, 0xFFFFFFFF};
	u32 read, old;
	int i;

	if (!mask)
		return false;
	old = mv_gop_gen_read(reg, offset);

	for (i = 0; i < ARRAY_SIZE(test); i++) {
		mv_gop_gen_write(reg, offset, write & test[i]);
		read = mv_gop_gen_read(reg, offset);
		if (read != (write & test[i] & mask)) {
			pr_err("pattern test reg %p(test 0x%08X write 0x%08X mask 0x%08X) failed: ",
			      reg, test[i], write, mask);
			pr_err("got 0x%08X expected 0x%08X\n", read, (write & test[i] & mask));
			mv_gop_gen_write(reg, offset, old);
			return true;
		}
	}

	mv_gop_gen_write(reg, offset, old);

	return false;
}

static u64 mv_pp2x_eth_tool_reg_test(struct mv_pp2x_port *port)
{
	int ind;
	int err = 0;
	struct mv_mac_data *mac = &port->mac_data;
	int gop_port = mac->gop_index;
	struct gop_hw *gop = &port->priv->hw.gop;
	void *reg = gop->gop_110.gmac.base + gop_port * gop->gop_110.gmac.obj_size;

	pr_info("Register testing starting\n");

	err += mv_pp2x_reg_pattern_test(reg, MV_GMAC_PORT_CTRL0_REG, MV_PP2_TEST_MASK1, MV_PP2_TEST_PATTERN1);
	err += mv_pp2x_reg_pattern_test(reg, MV_GMAC_PORT_CTRL1_REG, MV_PP2_TEST_MASK1, MV_PP2_TEST_PATTERN1);
	err += mv_pp2x_reg_pattern_test(reg, MV_GMAC_PORT_CTRL2_REG, MV_PP2_TEST_MASK1, MV_PP2_TEST_PATTERN1);
	err += mv_pp2x_reg_pattern_test(reg, MV_GMAC_PORT_AUTO_NEG_CFG_REG, MV_PP2_TEST_MASK1, MV_PP2_TEST_PATTERN1);
	err += mv_pp2x_reg_pattern_test(reg, MV_GMAC_PORT_STATUS0_REG, MV_PP2_TEST_MASK3, MV_PP2_TEST_PATTERN3);
	err += mv_pp2x_reg_pattern_test(reg, MV_GMAC_PORT_SERIAL_PARAM_CFG_REG, MV_PP2_TEST_MASK1,
					MV_PP2_TEST_PATTERN1);
	err += mv_pp2x_reg_pattern_test(reg, MV_GMAC_PORT_FIFO_CFG_0_REG, MV_PP2_TEST_MASK1, MV_PP2_TEST_PATTERN1);
	err += mv_pp2x_reg_pattern_test(reg, MV_GMAC_PORT_FIFO_CFG_1_REG, MV_PP2_TEST_MASK1, MV_PP2_TEST_PATTERN1);
	err += mv_pp2x_reg_pattern_test(reg, MV_GMAC_PORT_SERDES_CFG0_REG, MV_PP2_TEST_MASK1, MV_PP2_TEST_PATTERN1);
	err += mv_pp2x_reg_pattern_test(reg, MV_GMAC_PORT_SERDES_CFG1_REG, MV_PP2_TEST_MASK1, MV_PP2_TEST_PATTERN1);
	err += mv_pp2x_reg_pattern_test(reg, MV_GMAC_PORT_SERDES_CFG2_REG, MV_PP2_TEST_MASK1, MV_PP2_TEST_PATTERN1);
	err += mv_pp2x_reg_pattern_test(reg, MV_GMAC_PORT_SERDES_CFG3_REG, MV_PP2_TEST_MASK3, MV_PP2_TEST_PATTERN3);
	err += mv_pp2x_reg_pattern_test(reg, MV_GMAC_PORT_PRBS_STATUS_REG, MV_PP2_TEST_MASK3, MV_PP2_TEST_PATTERN3);
	err += mv_pp2x_reg_pattern_test(reg, MV_GMAC_PORT_PRBS_ERR_CNTR_REG, MV_PP2_TEST_MASK3, MV_PP2_TEST_PATTERN3);
	err += mv_pp2x_reg_pattern_test(reg, MV_GMAC_PORT_STATUS1_REG, MV_PP2_TEST_MASK3, MV_PP2_TEST_PATTERN3);
	err += mv_pp2x_reg_pattern_test(reg, MV_GMAC_PORT_MIB_CNTRS_CTRL_REG, MV_PP2_TEST_MASK2, MV_PP2_TEST_PATTERN2);
	err += mv_pp2x_reg_pattern_test(reg, MV_GMAC_PORT_CTRL3_REG, MV_PP2_TEST_MASK1, MV_PP2_TEST_PATTERN1);
	err += mv_pp2x_reg_pattern_test(reg, MV_GMAC_QSGMII_REG, MV_PP2_TEST_MASK1, MV_PP2_TEST_PATTERN1);
	err += mv_pp2x_reg_pattern_test(reg, MV_GMAC_QSGMII_STATUS_REG, MV_PP2_TEST_MASK3, MV_PP2_TEST_PATTERN3);
	err += mv_pp2x_reg_pattern_test(reg, MV_GMAC_QSGMII_PRBS_CNTR_REG, MV_PP2_TEST_MASK3, MV_PP2_TEST_PATTERN3);
	for (ind = 0; ind < 8; ind++)
		err += mv_pp2x_reg_pattern_test(reg, MV_GMAC_CCFC_PORT_SPEED_TIMER_REG(ind), MV_PP2_TEST_MASK1,
						MV_PP2_TEST_PATTERN1);

	for (ind = 0; ind < 4; ind++)
		err += mv_pp2x_reg_pattern_test(reg, MV_GMAC_FC_DSA_TAG_REG(ind), MV_PP2_TEST_MASK1,
						MV_PP2_TEST_PATTERN1);

	err += mv_pp2x_reg_pattern_test(reg, MV_GMAC_LINK_LEVEL_FLOW_CTRL_WINDOW_REG_0, MV_PP2_TEST_MASK1,
					MV_PP2_TEST_PATTERN1);
	err += mv_pp2x_reg_pattern_test(reg, MV_GMAC_LINK_LEVEL_FLOW_CTRL_WINDOW_REG_1, MV_PP2_TEST_MASK1,
					MV_PP2_TEST_PATTERN1);
	err += mv_pp2x_reg_pattern_test(reg, MV_GMAC_PORT_CTRL4_REG, MV_PP2_TEST_MASK1, MV_PP2_TEST_PATTERN1);
	err += mv_pp2x_reg_pattern_test(reg, MV_GMAC_PORT_SERIAL_PARAM_1_CFG_REG, MV_PP2_TEST_MASK1,
					MV_PP2_TEST_PATTERN1);
	err += mv_pp2x_reg_pattern_test(reg, MV_GMAC_LPI_CTRL_0_REG, MV_PP2_TEST_MASK1, MV_PP2_TEST_PATTERN1);
	err += mv_pp2x_reg_pattern_test(reg, MV_GMAC_LPI_CTRL_1_REG, MV_PP2_TEST_MASK1, MV_PP2_TEST_PATTERN1);
	err += mv_pp2x_reg_pattern_test(reg, MV_GMAC_LPI_CTRL_2_REG, MV_PP2_TEST_MASK1, MV_PP2_TEST_PATTERN1);
	err += mv_pp2x_reg_pattern_test(reg, MV_GMAC_LPI_STATUS_REG, MV_PP2_TEST_MASK3, MV_PP2_TEST_PATTERN3);
	err += mv_pp2x_reg_pattern_test(reg, MV_GMAC_LPI_CNTR_REG, MV_PP2_TEST_MASK3, MV_PP2_TEST_PATTERN3);
	err += mv_pp2x_reg_pattern_test(reg, MV_GMAC_PULSE_1_MS_LOW_REG, MV_PP2_TEST_MASK1, MV_PP2_TEST_PATTERN1);
	err += mv_pp2x_reg_pattern_test(reg, MV_GMAC_PULSE_1_MS_HIGH_REG, MV_PP2_TEST_MASK1, MV_PP2_TEST_PATTERN1);
	err += mv_pp2x_reg_pattern_test(reg, MV_GMAC_INTERRUPT_MASK_REG, MV_PP2_TEST_MASK1, MV_PP2_TEST_PATTERN1);
	err += mv_pp2x_reg_pattern_test(reg, MV_GMAC_INTERRUPT_SUM_MASK_REG, MV_PP2_TEST_MASK1, MV_PP2_TEST_PATTERN1);

	if (err)
		return 1;
	return 0;
}

static void mv_pp2x_eth_tool_diag_test(struct net_device *netdev,
	struct ethtool_test *test, u64 *data)
{
	struct mv_pp2x_port *port = netdev_priv(netdev);
	int i;
	struct mv_mac_data *mac = &port->mac_data;

	if (!(mac->flags & MV_EMAC_F_INIT)) {
		pr_err("%s: interface %s is not initialized\n", __func__, netdev->name);
		for (i = 0; i < MV_PP2_TEST_LEN; i++)
			data[i] = -ENONET;
		test->flags |= ETH_TEST_FL_FAILED;
		return;
	}

	memset(data, 0, MV_PP2_TEST_LEN * sizeof(u64));

	switch (mac->phy_mode) {
	case PHY_INTERFACE_MODE_RGMII:
	case PHY_INTERFACE_MODE_SGMII:
	case PHY_INTERFACE_MODE_QSGMII:
	break;
	case PHY_INTERFACE_MODE_XAUI:
	case PHY_INTERFACE_MODE_RXAUI:
	case PHY_INTERFACE_MODE_KR:
		pr_err("10G Phy mode (%d) do not support test\n", mac->phy_mode);
		return;
	default:
		pr_err("%s: Wrong port mode (%d\n)", __func__, mac->phy_mode);
		return;
	}

	data[0] = mv_pp2x_eth_tool_link_test(port);
	data[1] = mv_pp2x_eth_tool_reg_test(port);
	for (i = 0; i < MV_PP2_TEST_LEN; i++)
		test->flags |= data[i] ? ETH_TEST_FL_FAILED : 0;

	msleep_interruptible(4 * 1000);
}

static const struct ethtool_ops mv_pp2x_eth_tool_ops = {
	.get_link		= ethtool_op_get_link,
	.get_settings		= mv_pp2x_ethtool_get_settings,
	.set_settings		= mv_pp2x_ethtool_set_settings,
	.set_coalesce		= mv_pp2x_ethtool_set_coalesce,
	.get_coalesce		= mv_pp2x_ethtool_get_coalesce,
	.nway_reset		= mv_pp2x_eth_tool_nway_reset,
	.get_drvinfo		= mv_pp2x_ethtool_get_drvinfo,
	.get_ethtool_stats	= mv_pp2x_eth_tool_get_ethtool_stats,
	.get_sset_count		= mv_pp2x_eth_tool_get_sset_count,
	.get_strings		= mv_pp2x_eth_tool_get_strings,
	.get_ringparam		= mv_pp2x_ethtool_get_ringparam,
	.set_ringparam		= mv_pp2x_ethtool_set_ringparam,
	.get_rxfh_indir_size	= mv_pp2x_ethtool_get_rxfh_indir_size,
	.get_rxnfc		= mv_pp2x_ethtool_get_rxnfc,
	.get_rxfh		= mv_pp2x_ethtool_get_rxfh,
	.set_rxfh		= mv_pp2x_ethtool_set_rxfh,
	.get_regs_len           = mv_pp2x_ethtool_get_regs_len,
	.get_regs		= mv_pp2x_ethtool_get_regs,
	.self_test		= mv_pp2x_eth_tool_diag_test,
};

void mv_pp2x_set_ethtool_ops(struct net_device *netdev)
{
	netdev->ethtool_ops = &mv_pp2x_eth_tool_ops;
}
