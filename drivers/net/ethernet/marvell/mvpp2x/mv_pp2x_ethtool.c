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
#include <linux/phy/phy.h>
#include <dt-bindings/phy/phy-comphy-mvebu.h>

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
	"rx_bytes", "rx_frames", "rx_unicast", "rx_mcast", "rx_bcast",
	"tx_bytes", "tx_frames", "tx_unicast", "tx_mcast", "tx_bcast",
	"rx_pause", "tx_pause", "rx_overrun", "rx_crc", "rx_runt", "rx_giant",
	"rx_fragments_err", "rx_mac_err", "rx_jabber", "rx_sw_drop", "rx_total_err",
	"tx_drop", "tx_crc_sent", "collision", "late_collision",
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
	case DUPLEX_UNKNOWN:
		if (cmd->speed == SPEED_1000) {
			pstatus->duplex = MV_PORT_DUPLEX_FULL;
		} else {
			pstatus->duplex = MV_PORT_DUPLEX_FULL;
			pr_err("Unknown duplex configuration, full duplex set\n");
		}
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

int mv_pp2x_autoneg_gmac_check_valid(struct mv_mac_data *mac, struct gop_hw *gop,
				     struct ethtool_cmd *cmd, struct mv_port_link_status *pstatus)
{
	int port_num = mac->gop_index;
	int err;

	err = mv_gop110_check_port_type(gop, port_num);
	if (err) {
		if (cmd->autoneg) {
			pr_err("GOP %d set to 1000Base-X and doesn't support autonegotiation\n", port_num);
			return -EINVAL;
		}
		return 0;
	}
	if (!cmd->autoneg) {
		err = mv_pp2x_check_speed_duplex_valid(cmd, pstatus);
		if (err)
			return -EINVAL;
	}

	return 0;
}

int mv_pp2x_autoneg_xlg_check_valid(struct mv_mac_data *mac, struct ethtool_cmd *cmd)
{
	int port_num = mac->gop_index;

	if (cmd->autoneg) {
		pr_err("XLG GOP %d doesn't support autonegotiation\n", port_num);
		return -EINVAL;
	}

	return 0;
}

void mv_pp2x_ethtool_valid_coalesce(struct ethtool_coalesce *c,
				    struct mv_pp2x_port *port)
{
	u64 val;

	if (c->rx_max_coalesced_frames > MVPP2_MAX_OCCUPIED_THRESH)
		pr_err("RX coalesced frames value too high, rounded to %d\n",
		       MVPP2_MAX_OCCUPIED_THRESH);

	if (c->tx_max_coalesced_frames > MVPP2_MAX_TRANSMITTED_THRESH) {
		pr_err("TX coalesced frames value too high, rounded to %d\n",
		       MVPP2_MAX_TRANSMITTED_THRESH);
		c->tx_max_coalesced_frames = MVPP2_MAX_TRANSMITTED_THRESH;
	}

	val = (port->priv->hw.tclk / USEC_PER_SEC) * c->rx_coalesce_usecs;
	if (val > MVPP2_MAX_ISR_RX_THRESHOLD)
		pr_err("RX coalesced time value too high, rounded to %ld usecs\n",
		       (MVPP2_MAX_ISR_RX_THRESHOLD * USEC_PER_SEC)
			/ port->priv->hw.tclk);

	val = (port->priv->hw.tclk / USEC_PER_SEC) * c->tx_coalesce_usecs;
	if (val > MVPP22_MAX_ISR_TX_THRESHOLD) {
		pr_err("TX coalesced time value too high, rounded to %ld usecs\n",
		       (MVPP22_MAX_ISR_TX_THRESHOLD * USEC_PER_SEC)
			/ port->priv->hw.tclk);
		c->tx_coalesce_usecs =
			(MVPP22_MAX_ISR_TX_THRESHOLD * USEC_PER_SEC)
			/ port->priv->hw.tclk;
	}
}

/* Ethtool methods */

/* Ethtool statistic */
static void mv_pp2x_eth_tool_get_ethtool_stats(struct net_device *dev,
					       struct ethtool_stats *stats, u64 *data)
{
	struct mv_pp2x_port *port = netdev_priv(dev);
	struct mv_mac_data *mac = &port->mac_data;
	struct gop_hw *gop = &port->priv->hw.gop;
	int gop_port = mac->gop_index;
	struct gop_stat	*gop_statistics = &mac->gop_statistics;
	int i = 0;

	if (port->priv->pp2_version == PPV21)
		return;

	mv_gop110_mib_counters_stat_update(gop, gop_port, gop_statistics);

	data[i++] = gop_statistics->rx_byte;
	data[i++] = gop_statistics->rx_frames;
	data[i++] = gop_statistics->rx_unicast;
	data[i++] = gop_statistics->rx_mcast;
	data[i++] = gop_statistics->rx_bcast;
	data[i++] = gop_statistics->tx_byte;
	data[i++] = gop_statistics->tx_frames;
	data[i++] = gop_statistics->tx_unicast;
	data[i++] = gop_statistics->tx_mcast;
	data[i++] = gop_statistics->tx_bcast;
	data[i++] = gop_statistics->rx_pause;
	data[i++] = gop_statistics->tx_pause;
	data[i++] = gop_statistics->rx_overrun;
	data[i++] = gop_statistics->rx_crc;
	data[i++] = gop_statistics->rx_runt;
	data[i++] = gop_statistics->rx_giant;
	data[i++] = gop_statistics->rx_fragments_err;
	data[i++] = gop_statistics->rx_mac_err;
	data[i++] = gop_statistics->rx_jabber;
	data[i++] = dev->stats.rx_dropped;
	data[i++] = gop_statistics->rx_total_err + dev->stats.rx_dropped;
	data[i++] = dev->stats.tx_dropped;
	data[i++] = gop_statistics->tx_crc_sent;
	data[i++] = gop_statistics->collision;
	data[i++] = gop_statistics->late_collision;
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

	if (port->priv->pp2_version == PPV21)
		return -EOPNOTSUPP;

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
	case PHY_INTERFACE_MODE_SFI:
	case PHY_INTERFACE_MODE_XFI:
		pr_err("XLG GOP %d doesn't support autonegotiation\n", mac->gop_index);
		return -ENODEV;
	break;
	default:
		pr_err("%s: Wrong port mode (%d)\n", __func__, mac->phy_mode);
		return -1;
	}

	return 0;
}

/* Get pause fc settings for ethtools */
static void mv_pp2x_get_pauseparam(struct net_device *dev,
				   struct ethtool_pauseparam *pause)
{
	struct mv_pp2x_port *port = netdev_priv(dev);
	struct mv_port_link_status status;
	struct mv_mac_data *mac = &port->mac_data;
	struct gop_hw *gop = &port->priv->hw.gop;
	int gop_port = mac->gop_index;
	phy_interface_t phy_mode;

	if (port->priv->pp2_version == PPV21)
		return;

	phy_mode = port->mac_data.phy_mode;

	switch (phy_mode) {
	case PHY_INTERFACE_MODE_RGMII:
	case PHY_INTERFACE_MODE_SGMII:
	case PHY_INTERFACE_MODE_QSGMII:
		mv_gop110_port_link_status(gop,	mac, &status);
		pause->autoneg =
			(status.autoneg_fc ? AUTONEG_ENABLE : AUTONEG_DISABLE);
	break;
	case PHY_INTERFACE_MODE_XAUI:
	case PHY_INTERFACE_MODE_RXAUI:
	case PHY_INTERFACE_MODE_KR:
	case PHY_INTERFACE_MODE_SFI:
	case PHY_INTERFACE_MODE_XFI:
		mv_gop110_port_link_status(gop,	mac, &status);
		pause->autoneg = AUTONEG_DISABLE;
	break;
	default:
		pr_err("%s: Wrong port mode (%d)", __func__, phy_mode);
		return;
	}

	if (status.rx_fc == MV_PORT_FC_ACTIVE || status.rx_fc == MV_PORT_FC_ENABLE)
		pause->rx_pause = 1;

	if (mv_gop110_check_fca_tx_state(gop, gop_port)) {
		pause->tx_pause = 1;
		return;
	}

	if (status.tx_fc == MV_PORT_FC_ACTIVE || status.tx_fc == MV_PORT_FC_ENABLE)
		pause->tx_pause = 1;
}

/* Set pause fc settings for ethtools */
static int mv_pp2x_set_pauseparam(struct net_device *dev,
				  struct ethtool_pauseparam *pause)
{
	struct mv_pp2x_port *port = netdev_priv(dev);
	struct mv_mac_data *mac = &port->mac_data;
	struct gop_hw *gop = &port->priv->hw.gop;
	int gop_port = mac->gop_index;
	phy_interface_t phy_mode;
	int err;

	if (port->priv->pp2_version == PPV21)
		return -EOPNOTSUPP;

	if (!(mac->flags & MV_EMAC_F_INIT)) {
		pr_err("%s: interface %s is not initialized\n", __func__, dev->name);
		return -EOPNOTSUPP;
	}

	phy_mode = port->mac_data.phy_mode;

	switch (phy_mode) {
	case PHY_INTERFACE_MODE_RGMII:
	case PHY_INTERFACE_MODE_SGMII:
	case PHY_INTERFACE_MODE_QSGMII:
		if (mac->speed == SPEED_2500) {
			err = mv_gop110_check_port_type(gop, gop_port);
			if (err) {
				pr_err("Peridot module doesn't support FC\n");
				return -EINVAL;
			}
		}

		mv_gop110_force_link_mode_set(gop, mac, false, true);

		if (pause->autoneg) {
			mv_gop110_gmac_fc_set(gop, gop_port, MV_PORT_FC_AN_SYM);
			mv_gop110_autoneg_restart(gop, mac);
			mv_gop110_fca_send_periodic(gop, gop_port, false);
			}
		else	{
			mv_gop110_gmac_fc_set(gop, gop_port, MV_PORT_FC_AN_NO);
			mv_gop110_fca_send_periodic(gop, gop_port, true);
			}

		if (pause->rx_pause)
			mv_gop110_gmac_fc_set(gop, gop_port, MV_PORT_FC_RX_ENABLE);
		else
			mv_gop110_gmac_fc_set(gop, gop_port, MV_PORT_FC_RX_DISABLE);

		if (pause->tx_pause) {
			mv_gop110_gmac_fc_set(gop, gop_port, MV_PORT_FC_TX_ENABLE);
			mv_gop110_fca_tx_enable(gop, gop_port, false);
			}
		else	{
			mv_gop110_gmac_fc_set(gop, gop_port, MV_PORT_FC_TX_DISABLE);
			mv_gop110_fca_tx_enable(gop, gop_port, true);
			}

		mv_gop110_force_link_mode_set(gop, mac, false, false);
	break;
	case PHY_INTERFACE_MODE_XAUI:
	case PHY_INTERFACE_MODE_RXAUI:
	case PHY_INTERFACE_MODE_KR:
	case PHY_INTERFACE_MODE_SFI:
	case PHY_INTERFACE_MODE_XFI:
		if (pause->autoneg) {
			pr_err("10G port doesn't support fc autoneg\n");
			return -EINVAL;
			}
		if (pause->rx_pause)
			mv_gop110_xlg_mac_fc_set(gop, gop_port, MV_PORT_FC_RX_ENABLE);
		else
			mv_gop110_xlg_mac_fc_set(gop, gop_port, MV_PORT_FC_RX_DISABLE);

		if (pause->tx_pause) {
			mv_gop110_fca_tx_enable(gop, gop_port, false);
		} else	{
			mv_gop110_xlg_mac_fc_set(gop, gop_port, MV_PORT_FC_TX_DISABLE);
			mv_gop110_fca_tx_enable(gop, gop_port, true);
			}
	break;
	default:
		pr_err("%s: Wrong port mode (%d)", __func__, phy_mode);
		return -EINVAL;
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

	if (port->priv->pp2_version == PPV21) {
		if (!port->mac_data.phy_dev)
			return -ENODEV;
		return phy_ethtool_gset(port->mac_data.phy_dev, cmd);
	}

	/* No Phy device mngmt */
	if (!port->mac_data.phy_dev) {
		/*for force link port, RXAUI port and link-down ports,
		 * follow old strategy
		 */

		mv_gop110_port_link_status(&port->priv->hw.gop,
					   &port->mac_data, &status);

		if (status.linkup) {
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
		    (phy_mode == PHY_INTERFACE_MODE_KR)   ||
		    (phy_mode == PHY_INTERFACE_MODE_SFI) ||
		    (phy_mode == PHY_INTERFACE_MODE_XFI)) {
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

void mv_pp2x_ethtool_set_gmac_config(struct mv_port_link_status status, struct gop_hw *gop,
				     int gop_port, struct mv_mac_data *mac, struct ethtool_cmd *cmd)
{
	mv_gop110_force_link_mode_set(gop, mac, false, true);
	mv_gop110_gmac_set_autoneg(gop, mac, cmd->autoneg);
	if (cmd->autoneg) {
		mv_gop110_autoneg_restart(gop, mac);
		mv_gop110_gmac_fc_set(gop, gop_port, MV_PORT_FC_AN_SYM);
	} else {
		mv_gop110_gmac_speed_duplex_set(gop, gop_port, status.speed, status.duplex);
		mv_gop110_gmac_fc_set(gop, gop_port, MV_PORT_FC_AN_NO);
	}
	mv_gop110_force_link_mode_set(gop, mac, false, false);
}

int mv_pp2x_get_new_comphy_mode(struct ethtool_cmd *cmd, int port_id)
{
	if (cmd->speed == SPEED_10000 && port_id == 0)
		return COMPHY_DEF(COMPHY_SFI_MODE, port_id,
				  COMPHY_SPEED_10_3125G, COMPHY_POLARITY_NO_INVERT);
	else if (cmd->speed == SPEED_5000 && port_id == 0)
		return COMPHY_DEF(COMPHY_SFI_MODE, port_id,
				  COMPHY_SPEED_5_15625G, COMPHY_POLARITY_NO_INVERT);
	else if (cmd->speed == SPEED_2500)
		return COMPHY_DEF(COMPHY_HS_SGMII_MODE, port_id,
				  COMPHY_SPEED_3_125G, COMPHY_POLARITY_NO_INVERT);
	else if (cmd->speed == SPEED_1000 || cmd->speed == SPEED_100 ||
		 cmd->speed == SPEED_10)
		return COMPHY_DEF(COMPHY_SGMII_MODE, port_id,
				  COMPHY_SPEED_1_25G, COMPHY_POLARITY_NO_INVERT);
	else
		return -EINVAL;
}

void mv_pp2x_set_new_phy_mode(struct ethtool_cmd *cmd, struct mv_mac_data *mac)
{
	mac->flags &= ~(MV_EMAC_F_SGMII2_5 | MV_EMAC_F_5G);

	if (cmd->speed == SPEED_10000) {
		mac->phy_mode = PHY_INTERFACE_MODE_SFI;
		mac->speed = SPEED_10000;
	} else if (cmd->speed == SPEED_5000) {
		mac->phy_mode = PHY_INTERFACE_MODE_SFI;
		mac->speed = SPEED_5000;
		mac->flags |= MV_EMAC_F_5G;
	} else if (cmd->speed == SPEED_2500) {
		mac->phy_mode = PHY_INTERFACE_MODE_SGMII;
		mac->speed = SPEED_2500;
		mac->flags |= MV_EMAC_F_SGMII2_5;
	} else {
		mac->phy_mode = PHY_INTERFACE_MODE_SGMII;
		mac->speed = SPEED_1000;
	}
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
	bool phy_mode_update = false;

	/* PPv21 - only PHY should be configured
	*  PPv22 - set Serdes&GoP configuration and then configure PHY
	*/
	if (port->priv->pp2_version == PPV21) {
		if (!port->mac_data.phy_dev)
			return -ENODEV;
		else
			return phy_ethtool_sset(port->mac_data.phy_dev, cmd);
	}

	if (port->comphy)  {
		int comphy_old_mode, comphy_new_mode;

		comphy_new_mode = mv_pp2x_get_new_comphy_mode(cmd, port->id);

		if (comphy_new_mode < 0) {
			pr_err("Port ID %d: unsupported speed set\n", port->id);
			return comphy_new_mode;
		}
		comphy_old_mode = phy_get_mode(port->comphy);

		if (comphy_old_mode != comphy_new_mode) {
			err = phy_set_mode(port->comphy, comphy_new_mode);
			if (err < 0) {
				phy_set_mode(port->comphy, comphy_old_mode);
				pr_err("Port ID %d: COMPHY lane is busy\n", port->id);
				return err;
			}

			if (mac->flags & MV_EMAC_F_PORT_UP) {
				netif_carrier_off(port->dev);
				mv_gop110_port_events_mask(gop, mac);
				mv_gop110_port_disable(gop, mac, port->comphy);
				phy_power_off(port->comphy);
			}

			mv_pp2x_set_new_phy_mode(cmd, mac);
			phy_mode_update = true;
		}
	}

	if (phy_mode_update) {
		if (mac->flags & MV_EMAC_F_INIT) {
			mac->flags &= ~MV_EMAC_F_INIT;
			mvcpn110_mac_hw_init(port);
		}
		mv_pp22_set_net_comp(port->priv);

		if (mac->flags & MV_EMAC_F_PORT_UP) {
			mv_gop110_port_events_unmask(gop, mac);
			mv_gop110_port_enable(gop, mac, port->comphy);
			phy_power_on(port->comphy);
		}
	}

	switch (mac->phy_mode) {
	case PHY_INTERFACE_MODE_RGMII:
	case PHY_INTERFACE_MODE_SGMII:
	case PHY_INTERFACE_MODE_QSGMII:
		err = mv_pp2x_autoneg_gmac_check_valid(mac, gop, cmd, &status);
		if (err < 0)
			return err;
		if (cmd->speed != SPEED_2500)
			mv_pp2x_ethtool_set_gmac_config(status, gop, gop_port, mac, cmd);
	break;
	case PHY_INTERFACE_MODE_XAUI:
	case PHY_INTERFACE_MODE_RXAUI:
	case PHY_INTERFACE_MODE_KR:
	case PHY_INTERFACE_MODE_SFI:
	case PHY_INTERFACE_MODE_XFI:
		mv_pp2x_autoneg_xlg_check_valid(mac, cmd);
		if (err < 0)
			return err;
	break;
	default:
		pr_err("Wrong port mode (%d)\n", mac->phy_mode);
		return -1;
	}

	if (port->mac_data.phy_dev)
		return phy_ethtool_sset(port->mac_data.phy_dev, cmd);

	return 0;
}

/* Set interrupt coalescing for ethtools */
static int mv_pp2x_ethtool_set_coalesce(struct net_device *dev,
					struct ethtool_coalesce *c)
{
	struct mv_pp2x_port *port = netdev_priv(dev);
	int queue;

	/* Check for not supported parameters  */
	if ((c->rx_coalesce_usecs_irq) ||
	    (c->rx_max_coalesced_frames_irq) ||
	    (c->tx_coalesce_usecs_irq) ||
	    (c->tx_max_coalesced_frames_irq) ||
	    (c->stats_block_coalesce_usecs) ||
	    (c->use_adaptive_rx_coalesce) ||
	    (c->use_adaptive_tx_coalesce) ||
	    (c->pkt_rate_low) ||
	    (c->rx_coalesce_usecs_low) ||
	    (c->rx_max_coalesced_frames_low) ||
	    (c->tx_coalesce_usecs_low) ||
	    (c->tx_max_coalesced_frames_low) ||
	    (c->pkt_rate_high) ||
	    (c->rx_coalesce_usecs_high) ||
	    (c->rx_max_coalesced_frames_high) ||
	    (c->tx_coalesce_usecs_high) ||
	    (c->tx_max_coalesced_frames_high) ||
	    (c->rate_sample_interval)) {
		netdev_err(dev, "unsupported coalescing parameter\n");
		return -EOPNOTSUPP;
	}

	mv_pp2x_ethtool_valid_coalesce(c, port);

	for (queue = 0; queue < port->num_rx_queues; queue++) {
		struct mv_pp2x_rx_queue *rxq = port->rxqs[queue];

		rxq->time_coal = c->rx_coalesce_usecs;
		rxq->pkts_coal = c->rx_max_coalesced_frames;
		mv_pp2x_rx_pkts_coal_set(port, rxq);
		mv_pp2x_rx_time_coal_set(port, rxq);
	}
	port->tx_time_coal = c->tx_coalesce_usecs;
	for (queue = 0; queue < port->num_tx_queues; queue++) {
		struct mv_pp2x_tx_queue *txq = port->txqs[queue];

		txq->pkts_coal = c->tx_max_coalesced_frames;
	}
	if (port->priv->pp2xdata->interrupt_tx_done) {
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

	if (port->priv->pp2_version == PPV21)
		return -EOPNOTSUPP;

	return ARRAY_SIZE(port->priv->rx_indir_table);
}

static int mv_pp2x_get_rss_hash_opts(struct mv_pp2x_port *port,
				     struct ethtool_rxnfc *nfc)
{
	switch (nfc->flow_type) {
	case TCP_V4_FLOW:
	case TCP_V6_FLOW:
		nfc->data |= RXH_IP_SRC | RXH_IP_DST;
		nfc->data |= RXH_L4_B_0_1 | RXH_L4_B_2_3;
		break;
	case UDP_V4_FLOW:
	case UDP_V6_FLOW:
		nfc->data |= RXH_IP_SRC | RXH_IP_DST;
		if (port->rss_cfg.rss_mode == MVPP2_RSS_NF_UDP_5T)
			nfc->data |= RXH_L4_B_0_1 | RXH_L4_B_2_3;
		break;
	case IPV4_FLOW:
	case IPV6_FLOW:
		nfc->data |= RXH_IP_SRC | RXH_IP_DST;
		break;
	default:
		return -EOPNOTSUPP;
	}
	return 0;
}

static int mv_pp2x_ethtool_get_rxnfc(struct net_device *dev,
				     struct ethtool_rxnfc *cmd,
				     u32 *rules)
{
	struct mv_pp2x_port *port = netdev_priv(dev);
	int ret = -EOPNOTSUPP;

	if (port->priv->pp2_version == PPV21)
		return -EOPNOTSUPP;

	if (port->priv->pp2_cfg.queue_mode == MVPP2_QDIST_SINGLE_MODE)
		return -EOPNOTSUPP;

	if (!port)
		return -EIO;

	switch (cmd->cmd) {
	case ETHTOOL_GRXRINGS:
			cmd->data = ARRAY_SIZE(port->priv->rx_indir_table);
			ret = 0;
			break;
	case ETHTOOL_GRXFH:
			ret = mv_pp2x_get_rss_hash_opts(port, cmd);
			break;
	default:
			break;
	}

	return ret;
}

static int mv_pp2x_set_rss_hash_opt(struct mv_pp2x_port *port,
				    struct ethtool_rxnfc *nfc)
{
	if (nfc->data & ~(RXH_IP_SRC | RXH_IP_DST |
			  RXH_L4_B_0_1 | RXH_L4_B_2_3))
		return -EINVAL;

	switch (nfc->flow_type) {
	case TCP_V4_FLOW:
	case TCP_V6_FLOW:
		if (!(nfc->data & RXH_IP_SRC) ||
		    !(nfc->data & RXH_IP_DST) ||
		    !(nfc->data & RXH_L4_B_0_1) ||
		    !(nfc->data & RXH_L4_B_2_3))
			return -EINVAL;
		break;
	case UDP_V4_FLOW:
	case UDP_V6_FLOW:
		if (!(nfc->data & RXH_IP_SRC) ||
		    !(nfc->data & RXH_IP_DST))
			return -EINVAL;
		switch (nfc->data & (RXH_L4_B_0_1 | RXH_L4_B_2_3)) {
		case 0:
			mv_pp22_rss_mode_set(port, MVPP2_RSS_NF_UDP_2T);
			break;
		case (RXH_L4_B_0_1 | RXH_L4_B_2_3):
			mv_pp22_rss_mode_set(port, MVPP2_RSS_NF_UDP_5T);
			break;
		default:
			return -EINVAL;
		}
		break;
	default:
		return -EINVAL;
	}
	return 0;
}

static int mv_pp2x_ethtool_set_rxnfc(struct net_device *dev, struct ethtool_rxnfc *cmd)
{
	struct mv_pp2x_port *port = netdev_priv(dev);
	int ret = -EOPNOTSUPP;

	if (port->priv->pp2_version == PPV21)
		return -EOPNOTSUPP;

	/* Single mode doesn't support RSS features */
	if (port->priv->pp2_cfg.queue_mode == MVPP2_QDIST_SINGLE_MODE)
		return -EOPNOTSUPP;

	switch (cmd->cmd) {
	case ETHTOOL_SRXFH:
		ret =  mv_pp2x_set_rss_hash_opt(port, cmd);
		break;
	default:
		break;
	}

	return ret;
}

static int mv_pp2x_ethtool_get_rxfh(struct net_device *dev, u32 *indir, u8 *key,
				    u8 *hfunc)
{
	size_t copy_size;
	struct mv_pp2x_port *port = netdev_priv(dev);

	if (port->priv->pp2_version == PPV21)
		return -EOPNOTSUPP;

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

	if (port->priv->pp2_version == PPV21)
		return -EOPNOTSUPP;

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

	if (port->priv->pp2_version == PPV21)
		return -EOPNOTSUPP;

	switch (mac->phy_mode) {
	case PHY_INTERFACE_MODE_RGMII:
	case PHY_INTERFACE_MODE_SGMII:
	case PHY_INTERFACE_MODE_QSGMII:
		return MV_PP2_REGS_GMAC_LEN * sizeof(u32);
	case PHY_INTERFACE_MODE_XAUI:
	case PHY_INTERFACE_MODE_RXAUI:
	case PHY_INTERFACE_MODE_KR:
	case PHY_INTERFACE_MODE_SFI:
	case PHY_INTERFACE_MODE_XFI:
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

	if (port->priv->pp2_version == PPV21)
		return;

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
	case PHY_INTERFACE_MODE_SFI:
	case PHY_INTERFACE_MODE_XFI:
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

	if (port->priv->pp2_version == PPV21)
		return;

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
	case PHY_INTERFACE_MODE_SFI:
	case PHY_INTERFACE_MODE_XFI:
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
	.get_pauseparam		= mv_pp2x_get_pauseparam,
	.set_pauseparam		= mv_pp2x_set_pauseparam,
	.get_rxfh_indir_size	= mv_pp2x_ethtool_get_rxfh_indir_size,
	.get_rxnfc		= mv_pp2x_ethtool_get_rxnfc,
	.set_rxnfc		= mv_pp2x_ethtool_set_rxnfc,
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
