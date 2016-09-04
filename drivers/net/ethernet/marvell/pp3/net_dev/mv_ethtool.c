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
#include <linux/etherdevice.h>
#include <linux/netdevice.h>
#include <linux/mv_pp3.h>
#include "common/mv_sw_if.h"

#ifdef CONFIG_MV_PP3_FPGA
#include "gmac/mv_gmac.h"
#else /* CONFIG_MV_PP3_FPGA */
#include "gop/mv_gop_if.h"
#endif /* !CONFIG_MV_PP3_FPGA */

#include "mv_netdev.h"
#include "mv_netdev_structs.h"
#include "fw/mv_fw_shared.h"
#include "fw/mv_pp3_fw_msg.h"
#include "fw/mv_pp3_fw_msg_structs.h"


/******************************************************************************
* mv_pp3_eth_tool_get_settings
* Description:
*	ethtool get standard port settings
* INPUT:
*	netdev		Network device structure pointer
* OUTPUT
*	cmd		command (settings)
* RETURN:
*	0 for success
*
*******************************************************************************/
int mv_pp3_eth_tool_get_settings(struct net_device *netdev, struct ethtool_cmd *cmd)
{
	struct pp3_dev_priv		*priv = MV_PP3_PRIV(netdev);
	enum mv_port_speed		speed;
	enum mv_port_duplex		duplex;
	struct mv_port_link_status	status;
	enum mv_port_mode		port_mode;

	cmd->phy_address = priv->mac_data.phy_addr;
	port_mode = (enum mv_port_mode)priv->mac_data.port_mode;
	cmd->speed  = SPEED_UNKNOWN;
	cmd->duplex = SPEED_UNKNOWN;

	if (priv->flags & MV_PP3_F_INIT) {

		mv_pp3_gop_port_link_status(priv->id, &status);

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
		/* check if speed and duplex are AN */
		speed = MV_PORT_SPEED_AN;
		duplex = MV_PORT_DUPLEX_AN;
		if ((port_mode == MV_PORT_RXAUI) || (port_mode == MV_PORT_XAUI))
			cmd->autoneg = AUTONEG_DISABLE;
		else {
			mv_pp3_gop_speed_duplex_get(priv->id, &speed, &duplex);
			if (speed == MV_PORT_SPEED_AN && duplex == MV_PORT_DUPLEX_AN) {
				cmd->lp_advertising = cmd->advertising = 0;
				cmd->autoneg = AUTONEG_ENABLE;
			} else
				cmd->autoneg = AUTONEG_DISABLE;
		}
	}

	if ((port_mode == MV_PORT_RXAUI) || (port_mode == MV_PORT_XAUI)) {
		cmd->supported = (SUPPORTED_FIBRE | SUPPORTED_10000baseT_Full);
		cmd->supported = (SUPPORTED_10000baseT_Full | SUPPORTED_FIBRE);
		cmd->advertising = (ADVERTISED_10000baseT_Full | ADVERTISED_FIBRE);
		cmd->port = PORT_FIBRE;
		cmd->transceiver = XCVR_EXTERNAL;
		cmd->speed = SPEED_10000;
		cmd->duplex = 1;
	} else {
		cmd->supported = (SUPPORTED_10baseT_Half | SUPPORTED_10baseT_Full | SUPPORTED_100baseT_Half
			| SUPPORTED_100baseT_Full | SUPPORTED_Autoneg | SUPPORTED_TP | SUPPORTED_MII
			| SUPPORTED_1000baseT_Full);
		cmd->transceiver = XCVR_INTERNAL;
		cmd->port = PORT_MII;
	}

	return 0;
}


/******************************************************************************
* mv_pp3_eth_tool_set_settings
* Description:
*	ethtool set standard port settings
* INPUT:
*	netdev		Network device structure pointer
*	cmd		command (settings)
* OUTPUT
*	None
* RETURN:
*	0 for success
*
*******************************************************************************/
int mv_pp3_eth_tool_set_settings(struct net_device *dev, struct ethtool_cmd *cmd)
{
	struct pp3_dev_priv		*priv = MV_PP3_PRIV(dev);
	enum mv_port_speed		speed;
	enum mv_port_duplex		duplex;
	enum mv_port_mode		port_mode;
	int err;

	if (!(priv->flags & MV_PP3_F_INIT)) {
		pr_err("%s: interface %s is not initialized\n", __func__, dev->name);
		return -EOPNOTSUPP;
	}

	port_mode = (enum mv_port_mode)priv->mac_data.port_mode;
	if ((port_mode == MV_PORT_RXAUI) || (port_mode == MV_PORT_XAUI)) {
		pr_info("cannot change port setting for %s interface", dev->name);
		return -EOPNOTSUPP;
	}
	if (cmd->autoneg == AUTONEG_DISABLE) {
		switch (cmd->speed) {
		case SPEED_10:
			speed = MV_PORT_SPEED_10;
			break;
		case SPEED_100:
			speed = MV_PORT_SPEED_100;
			break;
		case SPEED_1000:
			speed = MV_PORT_SPEED_1000;
			break;
		default:
			return -EINVAL;
		}

		switch (cmd->duplex) {
		case DUPLEX_HALF:
			duplex = MV_PORT_DUPLEX_HALF;
			break;
		case DUPLEX_FULL:
			duplex = MV_PORT_DUPLEX_FULL;
			break;
		default:
			return -EINVAL;
		}
		err = mv_pp3_gop_speed_duplex_set(priv->id, speed, duplex);

	} else if (cmd->autoneg == AUTONEG_ENABLE)
		err = mv_pp3_gop_speed_duplex_set(priv->id, MV_PORT_SPEED_AN, MV_PORT_DUPLEX_AN);
	else
		err = -EINVAL;

	return err;
}

/******************************************************************************
* mv_pp3_eth_tool_get_drvinfo
* Description:
*	ethtool get driver information
* INPUT:
*	netdev		Network device structure pointer
*	info		driver information
* OUTPUT
*	info		driver information
* RETURN:
*	None
*
*******************************************************************************/
void mv_pp3_eth_tool_get_drvinfo(struct net_device *netdev, struct ethtool_drvinfo *info)
{
	struct pp3_dev_priv *dev_priv = MV_PP3_PRIV(netdev);
	struct mv_pp3_version fw_ver, *drv_ver;

	drv_ver = mv_pp3_get_driver_version();
	/* get from FW run version for each client */
	if (dev_priv->flags & MV_PP3_F_INIT) {
		pp3_fw_version_get(&fw_ver);
		sprintf(info->fw_version, "%d.%d.%d", fw_ver.major_x, fw_ver.minor_y, fw_ver.local_z);
	} else
		strcpy(info->fw_version, "N/A");

	strlcpy(info->driver, MV_PP3_PORT_NAME, sizeof(info->driver));
	sprintf(info->version, "%d.%d.%d", drv_ver->major_x, drv_ver->minor_y, drv_ver->local_z);
	strcpy(info->bus_info, "N/A");

	info->n_stats = 0;
	info->testinfo_len = 0;
	info->regdump_len = 0;
	info->eedump_len = 0;
}

/******************************************************************************
* mv_pp3_eth_tool_get_link
* Description:
*	ethtool get link status
* INPUT:
*	netdev		Network device structure pointer
* OUTPUT
*	None
* RETURN:
*	0 if link is down, 1 if link is up
*
*******************************************************************************/
u32 mv_pp3_eth_tool_get_link(struct net_device *netdev)
{
	struct pp3_dev_priv	*priv = MV_PP3_PRIV(netdev);

	return mv_pp3_gop_port_is_link_up(priv->id);
}

/******************************************************************************
* mv_pp3_eth_tool_get_coalesce
* Description:
*	ethtool get RX/TX coalesce parameters
* INPUT:
*	netdev		Network device structure pointer
* OUTPUT
*	cmd		Coalesce parameters
* RETURN:
*	0 on success
*
*******************************************************************************/
int mv_pp3_eth_tool_get_coalesce(struct net_device *netdev, struct ethtool_coalesce *cmd)
{
	mv_pp3_rx_time_coal_get(netdev, &cmd->rx_coalesce_usecs);
	mv_pp3_rx_pkt_coal_get(netdev, &cmd->rx_max_coalesced_frames);
	mv_pp3_txdone_time_coal_get(netdev, &cmd->tx_coalesce_usecs);
	mv_pp3_txdone_pkt_coal_get(netdev, &cmd->tx_max_coalesced_frames);

	return 0;
}

/******************************************************************************
* mv_pp3_eth_tool_set_coalesce
* Description:
*	ethtool set RX/TX coalesce parameters
* INPUT:
*	netdev		Network device structure pointer
*	cmd		Coalesce parameters
* OUTPUT
*	None
* RETURN:
*	0 on success
*
*******************************************************************************/
int mv_pp3_eth_tool_set_coalesce(struct net_device *netdev, struct ethtool_coalesce *cmd)
{
	int rx_time_coal;
	int rx_pkt_coal;
	int tx_pkt_coal;

	if (cmd->rx_coalesce_usecs) {
		mv_pp3_rx_time_coal_get(netdev, &rx_time_coal);
		if (rx_time_coal != cmd->rx_coalesce_usecs)
			mv_pp3_rx_time_coal_set(netdev, cmd->rx_coalesce_usecs);
	}
	if (cmd->rx_max_coalesced_frames) {
		mv_pp3_rx_pkt_coal_get(netdev, &rx_pkt_coal);
		if (rx_pkt_coal != cmd->rx_max_coalesced_frames)
			mv_pp3_rx_pkt_coal_set(netdev, cmd->rx_max_coalesced_frames);
	}
	if (cmd->tx_max_coalesced_frames) {
		mv_pp3_txdone_pkt_coal_get(netdev, &tx_pkt_coal);
		if (tx_pkt_coal != cmd->tx_max_coalesced_frames)
			mv_pp3_txdone_pkt_coal_set(netdev, cmd->tx_max_coalesced_frames);
	}
	if (cmd->tx_coalesce_usecs)
		mv_pp3_txdone_time_coal_set(netdev, cmd->tx_coalesce_usecs);

	return 0;
}

/******************************************************************************
* mv_pp3_eth_tool_nway_reset
* Description:
*	ethtool restart auto negotiation
* INPUT:
*	netdev		Network device structure pointer
* OUTPUT
*	None
* RETURN:
*	0 on success
*
*******************************************************************************/
int mv_pp3_eth_tool_nway_reset(struct net_device *dev)
{
	struct pp3_dev_priv *priv = MV_PP3_PRIV(dev);

	if (!(priv->flags & MV_PP3_F_INIT)) {
		pr_err("%s: interface %s is not initialized\n", __func__, dev->name);
		return -EOPNOTSUPP;
	}

	mv_pp3_gop_autoneg_restart(priv->id);

	return 0;
}

/* NIC ports ethtool functions */
const struct ethtool_ops mv_pp3_ethtool_ops = {
	.get_drvinfo				= mv_pp3_eth_tool_get_drvinfo,
	.get_link				= mv_pp3_eth_tool_get_link,
	.get_settings				= mv_pp3_eth_tool_get_settings,
	.set_settings				= mv_pp3_eth_tool_set_settings,
	.get_coalesce				= mv_pp3_eth_tool_get_coalesce,
	.set_coalesce				= mv_pp3_eth_tool_set_coalesce,
	.nway_reset				= mv_pp3_eth_tool_nway_reset,
};

/* NSS ports ethtool functions */
const struct ethtool_ops mv_pp3_gnss_ethtool_ops = {
	.get_drvinfo				= mv_pp3_eth_tool_get_drvinfo,
	.get_coalesce				= mv_pp3_eth_tool_get_coalesce,
	.set_coalesce				= mv_pp3_eth_tool_set_coalesce,
};
