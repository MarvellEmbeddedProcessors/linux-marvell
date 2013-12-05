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
#include <linux/etherdevice.h>
#include <linux/skbuff.h>
#include <net/ip.h>
#include <net/ipv6.h>
#include <linux/mii.h>

#include "mvOs.h"
#include "mvDebug.h"
#include "dbg-trace.h"
#include "mvSysHwConfig.h"
#include "boardEnv/mvBoardEnvLib.h"
#include "ctrlEnv/mvCtrlEnvLib.h"
#include "eth-phy/mvEthPhy.h"
#include "mvSysEthPhyApi.h"
#include "mvSysNetaApi.h"


#include "gbe/mvNeta.h"
#include "bm/mvBm.h"

#include "mv_switch.h"
#include "mv_netdev.h"

#include "mvOs.h"
#include "mvSysHwConfig.h"

#ifdef CONFIG_MV_ETH_PNC
#include "pnc/mvPnc.h"
#endif /* CONFIG_MV_ETH_PNC */


#define MV_ETH_TOOL_AN_TIMEOUT	5000


static int isSwitch(struct eth_port *priv)
{
	return priv->tagged;
}


/******************************************************************************
* mv_eth_tool_restore_settings
* Description:
*	restore saved speed/dublex/an settings
* INPUT:
*	netdev		Network device structure pointer
* OUTPUT
*	None
* RETURN:
*	0 for success
*
*******************************************************************************/
int mv_eth_tool_restore_settings(struct net_device *netdev)
{
	struct eth_port 	*priv = MV_ETH_PRIV(netdev);
	int 				mv_phy_speed, mv_phy_duplex;
	MV_U32			    mv_phy_addr = mvBoardPhyAddrGet(priv->port);
	MV_ETH_PORT_SPEED	mv_mac_speed;
	MV_ETH_PORT_DUPLEX	mv_mac_duplex;
	int			err = -EINVAL;

	 if ((priv == NULL) || (isSwitch(priv)))
		 return -EOPNOTSUPP;

	switch (priv->speed_cfg) {
	case SPEED_10:
		mv_phy_speed  = 0;
		mv_mac_speed = MV_ETH_SPEED_10;
		break;
	case SPEED_100:
		mv_phy_speed  = 1;
		mv_mac_speed = MV_ETH_SPEED_100;
		break;
	case SPEED_1000:
		mv_phy_speed  = 2;
		mv_mac_speed = MV_ETH_SPEED_1000;
		break;
	default:
		return -EINVAL;
	}

	switch (priv->duplex_cfg) {
	case DUPLEX_HALF:
		mv_phy_duplex = 0;
		mv_mac_duplex = MV_ETH_DUPLEX_HALF;
		break;
	case DUPLEX_FULL:
		mv_phy_duplex = 1;
		mv_mac_duplex = MV_ETH_DUPLEX_FULL;
		break;
	default:
		return -EINVAL;
	}

	if (priv->autoneg_cfg == AUTONEG_ENABLE) {
		err = mvNetaSpeedDuplexSet(priv->port, MV_ETH_SPEED_AN, MV_ETH_DUPLEX_AN);
		if (!err)
			err = mvEthPhyAdvertiseSet(mv_phy_addr, priv->advertise_cfg);
		/* Restart AN on PHY enables it */
		if (!err) {

			err = mvEthPhyRestartAN(mv_phy_addr, MV_ETH_TOOL_AN_TIMEOUT);
			if (err == MV_TIMEOUT) {
				MV_ETH_PORT_STATUS ps;

				mvNetaLinkStatus(priv->port, &ps);

				if (!ps.linkup)
					err = 0;
			}
		}
	} else if (priv->autoneg_cfg == AUTONEG_DISABLE) {
		err = mvEthPhyDisableAN(mv_phy_addr, mv_phy_speed, mv_phy_duplex);
		if (!err)
			err = mvNetaSpeedDuplexSet(priv->port, mv_mac_speed, mv_mac_duplex);
	} else {
		err = -EINVAL;
	}
	return err;
}




/******************************************************************************
* mv_eth_tool_get_settings
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
int mv_eth_tool_get_settings(struct net_device *netdev, struct ethtool_cmd *cmd)
{
	struct eth_port 	*priv = MV_ETH_PRIV(netdev);
	u16			lp_ad, stat1000;
	MV_U32			mv_phy_addr;
	MV_ETH_PORT_SPEED 	speed;
	MV_ETH_PORT_DUPLEX 	duplex;
	MV_ETH_PORT_STATUS      status;

	if ((priv == NULL) || (isSwitch(priv)) || (MV_PON_PORT(priv->port))) {
		printk(KERN_ERR "%s is not supported on %s\n", __func__, netdev->name);
		return -EOPNOTSUPP;
	}

	cmd->supported = (SUPPORTED_10baseT_Half | SUPPORTED_10baseT_Full | SUPPORTED_100baseT_Half
			| SUPPORTED_100baseT_Full | SUPPORTED_Autoneg | SUPPORTED_TP | SUPPORTED_MII
			| SUPPORTED_1000baseT_Full);

	mv_phy_addr = mvBoardPhyAddrGet(priv->port);

	mvNetaLinkStatus(priv->port, &status);

	if (status.linkup != MV_TRUE) {
		/* set to Unknown */
		cmd->speed  = -1;
		cmd->duplex = -1;
	} else {
		switch (status.speed) {
		case MV_ETH_SPEED_1000:
			cmd->speed = SPEED_1000;
			break;
		case MV_ETH_SPEED_100:
			cmd->speed = SPEED_100;
			break;
		case MV_ETH_SPEED_10:
			cmd->speed = SPEED_10;
			break;
		default:
			return -EINVAL;
		}
		if (status.duplex == MV_ETH_DUPLEX_FULL)
			cmd->duplex = 1;
		else
			cmd->duplex = 0;
	}

	cmd->port = PORT_MII;
	cmd->phy_address = mv_phy_addr;
	cmd->transceiver = XCVR_INTERNAL;
	/* check if speed and duplex are AN */
	mvNetaSpeedDuplexGet(priv->port, &speed, &duplex);
	if (speed == MV_ETH_SPEED_AN && duplex == MV_ETH_DUPLEX_AN) {
		cmd->lp_advertising = cmd->advertising = 0;
		cmd->autoneg = AUTONEG_ENABLE;
		mvEthPhyAdvertiseGet(mv_phy_addr, (MV_U16 *)&(cmd->advertising));

		mvEthPhyRegRead(mv_phy_addr, MII_LPA, &lp_ad);
		if (lp_ad & LPA_LPACK)
			cmd->lp_advertising |= ADVERTISED_Autoneg;
		if (lp_ad & ADVERTISE_10HALF)
			cmd->lp_advertising |= ADVERTISED_10baseT_Half;
		if (lp_ad & ADVERTISE_10FULL)
			cmd->lp_advertising |= ADVERTISED_10baseT_Full;
		if (lp_ad & ADVERTISE_100HALF)
			cmd->lp_advertising |= ADVERTISED_100baseT_Half;
		if (lp_ad & ADVERTISE_100FULL)
			cmd->lp_advertising |= ADVERTISED_100baseT_Full;

		mvEthPhyRegRead(mv_phy_addr, MII_STAT1000, &stat1000);
		if (stat1000 & LPA_1000HALF)
			cmd->lp_advertising |= ADVERTISED_1000baseT_Half;
		if (stat1000 & LPA_1000FULL)
			cmd->lp_advertising |= ADVERTISED_1000baseT_Full;
	} else
		cmd->autoneg = AUTONEG_DISABLE;

	return 0;
}


/******************************************************************************
* mv_eth_tool_set_settings
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
int mv_eth_tool_set_settings(struct net_device *dev, struct ethtool_cmd *cmd)
{
	struct eth_port *priv = MV_ETH_PRIV(dev);
	int _speed, _duplex, _autoneg, _advertise, err;

	if ((priv == NULL) || (isSwitch(priv)) || (MV_PON_PORT(priv->port))) {
		printk(KERN_ERR "%s is not supported on %s\n", __func__, dev->name);
		return -EOPNOTSUPP;
	}

	_duplex  = priv->duplex_cfg;
	_speed   = priv->speed_cfg;
	_autoneg = priv->autoneg_cfg;
	_advertise = priv->advertise_cfg;

	priv->duplex_cfg = cmd->duplex;
	priv->speed_cfg = cmd->speed;
	priv->autoneg_cfg = cmd->autoneg;
	priv->advertise_cfg = cmd->advertising;
	err = mv_eth_tool_restore_settings(dev);

	if (err) {
		priv->duplex_cfg = _duplex;
		priv->speed_cfg = _speed;
		priv->autoneg_cfg = _autoneg;
		priv->advertise_cfg = _advertise;
	}
	return err;
}

/******************************************************************************
* mv_eth_tool_get_regs_len
* Description:
*	ethtool get registers array length
* INPUT:
*	netdev		Network device structure pointer
* OUTPUT
*	None
* RETURN:
*	registers array length
*
*******************************************************************************/
int mv_eth_tool_get_regs_len(struct net_device *netdev)
{
#define MV_ETH_TOOL_REGS_LEN 32

	return (MV_ETH_TOOL_REGS_LEN * sizeof(uint32_t));
}


/******************************************************************************
* mv_eth_tool_get_drvinfo
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
void mv_eth_tool_get_drvinfo(struct net_device *netdev,
			     struct ethtool_drvinfo *info)
{
	strcpy(info->driver, "mv_eth");
	/*strcpy(info->version, LSP_VERSION);*/
	strcpy(info->fw_version, "N/A");
	strcpy(info->bus_info, "Mbus");
/*   TBD
	info->n_stats = MV_ETH_TOOL_STATS_LEN;
*/
	info->testinfo_len = 0;
	info->regdump_len = mv_eth_tool_get_regs_len(netdev);
	info->eedump_len = 0;
}


/******************************************************************************
* mv_eth_tool_get_regs
* Description:
*	ethtool get registers array
* INPUT:
*	netdev		Network device structure pointer
*	regs		registers information
* OUTPUT
*	p		registers array
* RETURN:
*	None
*
*******************************************************************************/
void mv_eth_tool_get_regs(struct net_device *netdev,
			  struct ethtool_regs *regs, void *p)
{
	struct eth_port	*priv = MV_ETH_PRIV(netdev);
	uint32_t 	*regs_buff = p;

	if ((priv == NULL) || MV_PON_PORT(priv->port)) {
		printk(KERN_ERR "%s is not supported on %s\n", __func__, netdev->name);
		return;
	}

	memset(p, 0, MV_ETH_TOOL_REGS_LEN * sizeof(uint32_t));

	regs->version = mvCtrlModelRevGet();

	/* ETH port registers */
	regs_buff[0]  = MV_REG_READ(ETH_PORT_STATUS_REG(priv->port));
	regs_buff[1]  = MV_REG_READ(ETH_PORT_SERIAL_CTRL_REG(priv->port));
	regs_buff[2]  = MV_REG_READ(ETH_PORT_CONFIG_REG(priv->port));
	regs_buff[3]  = MV_REG_READ(ETH_PORT_CONFIG_EXTEND_REG(priv->port));
	regs_buff[4]  = MV_REG_READ(ETH_SDMA_CONFIG_REG(priv->port));
/*	regs_buff[5]  = MV_REG_READ(ETH_TX_FIFO_URGENT_THRESH_REG(priv->port)); */
	regs_buff[6]  = MV_REG_READ(ETH_RX_QUEUE_COMMAND_REG(priv->port));
	/* regs_buff[7]  = MV_REG_READ(ETH_TX_QUEUE_COMMAND_REG(priv->port)); */
	regs_buff[8]  = MV_REG_READ(ETH_INTR_CAUSE_REG(priv->port));
	regs_buff[9]  = MV_REG_READ(ETH_INTR_CAUSE_EXT_REG(priv->port));
	regs_buff[10] = MV_REG_READ(ETH_INTR_MASK_REG(priv->port));
	regs_buff[11] = MV_REG_READ(ETH_INTR_MASK_EXT_REG(priv->port));
	/* ETH Unit registers */
	regs_buff[16] = MV_REG_READ(ETH_PHY_ADDR_REG(priv->port));
	regs_buff[17] = MV_REG_READ(ETH_UNIT_INTR_CAUSE_REG(priv->port));
	regs_buff[18] = MV_REG_READ(ETH_UNIT_INTR_MASK_REG(priv->port));
	regs_buff[19] = MV_REG_READ(ETH_UNIT_ERROR_ADDR_REG(priv->port));
	regs_buff[20] = MV_REG_READ(ETH_UNIT_INT_ADDR_ERROR_REG(priv->port));

}




/******************************************************************************
* mv_eth_tool_nway_reset
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
int mv_eth_tool_nway_reset(struct net_device *netdev)
{
	struct eth_port *priv = MV_ETH_PRIV(netdev);
	MV_U32	        phy_addr;

	if ((priv == NULL) || (isSwitch(priv)) || (MV_PON_PORT(priv->port))) {
		printk(KERN_ERR "interface %s is not supported\n", netdev->name);
		return -EOPNOTSUPP;
	}

	phy_addr = mvBoardPhyAddrGet(priv->port);
	if (mvEthPhyRestartAN(phy_addr, MV_ETH_TOOL_AN_TIMEOUT) != MV_OK)
		return -EINVAL;

	return 0;
}

/******************************************************************************
* mv_eth_tool_get_link
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
u32 mv_eth_tool_get_link(struct net_device *netdev)
{
	struct eth_port     *pp = MV_ETH_PRIV(netdev);

	if (pp == NULL) {
		printk(KERN_ERR "interface %s is not supported\n", netdev->name);
		return -EOPNOTSUPP;
	}

#ifdef CONFIG_MV_PON
	if (MV_PON_PORT(pp->port))
		return mv_pon_link_status();
#endif /* CONFIG_MV_PON */

	return mvNetaLinkIsUp(pp->port);
}
/******************************************************************************
* mv_eth_tool_get_coalesce
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
int mv_eth_tool_get_coalesce(struct net_device *netdev,
			     struct ethtool_coalesce *cmd)
{
	struct eth_port *pp = MV_ETH_PRIV(netdev);
	/* get coal parameters only for rxq=0, txp=txq=0 !!!
	   notice that if you use ethtool to set coal, then all queues have the same value */
	cmd->rx_coalesce_usecs = pp->rx_time_coal_cfg;
	cmd->rx_max_coalesced_frames = pp->rx_pkts_coal_cfg;
	cmd->tx_max_coalesced_frames = pp->tx_pkts_coal_cfg;

	/* Adaptive RX coalescing parameters */
	cmd->rx_coalesce_usecs_low = pp->rx_time_low_coal_cfg;
	cmd->rx_coalesce_usecs_high = pp->rx_time_high_coal_cfg;
	cmd->rx_max_coalesced_frames_low = pp->rx_pkts_low_coal_cfg;
	cmd->rx_max_coalesced_frames_high = pp->rx_pkts_high_coal_cfg;
	cmd->pkt_rate_low = pp->pkt_rate_low_cfg;
	cmd->pkt_rate_high = pp->pkt_rate_high_cfg;
	cmd->rate_sample_interval = pp->rate_sample_cfg;
	cmd->use_adaptive_rx_coalesce = pp->rx_adaptive_coal_cfg;

	return 0;
}

/******************************************************************************
* mv_eth_tool_set_coalesce
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
int mv_eth_tool_set_coalesce(struct net_device *netdev,
			     struct ethtool_coalesce *cmd)
{
	struct eth_port *pp = MV_ETH_PRIV(netdev);
	int rxq, txp, txq;

	/* can't set rx coalesce with both 0 pkts and 0 usecs,  tx coalesce supports only pkts */
	if ((!cmd->rx_coalesce_usecs && !cmd->rx_max_coalesced_frames) || (!cmd->tx_max_coalesced_frames))
		return -EPERM;

	if (!cmd->use_adaptive_rx_coalesce) {
		for (rxq = 0; rxq < CONFIG_MV_ETH_RXQ; rxq++) {
			mv_eth_rx_pkts_coal_set(pp->port, rxq, cmd->rx_max_coalesced_frames);
			mv_eth_rx_time_coal_set(pp->port, rxq, cmd->rx_coalesce_usecs);
		}
	}

	pp->rx_time_coal_cfg = cmd->rx_coalesce_usecs;
	pp->rx_pkts_coal_cfg = cmd->rx_max_coalesced_frames;
	for (txp = 0; txp < pp->txp_num; txp++)
		for (txq = 0; txq < CONFIG_MV_ETH_TXQ; txq++)
			mv_eth_tx_done_pkts_coal_set(pp->port, txp, txq, cmd->tx_max_coalesced_frames);
	pp->tx_pkts_coal_cfg = cmd->tx_max_coalesced_frames;

	/* Adaptive RX coalescing parameters */
	pp->rx_time_low_coal_cfg = cmd->rx_coalesce_usecs_low;
	pp->rx_time_high_coal_cfg = cmd->rx_coalesce_usecs_high;
	pp->rx_pkts_low_coal_cfg = cmd->rx_max_coalesced_frames_low;
	pp->rx_pkts_high_coal_cfg = cmd->rx_max_coalesced_frames_high;
	pp->pkt_rate_low_cfg = cmd->pkt_rate_low;
	pp->pkt_rate_high_cfg = cmd->pkt_rate_high;

	if (cmd->rate_sample_interval > 0)
		pp->rate_sample_cfg = cmd->rate_sample_interval;

	/* check if adaptive rx is on - reset rate calculation parameters */
	if (!pp->rx_adaptive_coal_cfg && cmd->use_adaptive_rx_coalesce) {
		pp->rx_timestamp = jiffies;
		pp->rx_rate_pkts = 0;
	}
	pp->rx_adaptive_coal_cfg = cmd->use_adaptive_rx_coalesce;
	pp->rate_current = 0; /* Unknown */

	return 0;
}


/******************************************************************************
* mv_eth_tool_get_ringparam
* Description:
*	ethtool get ring parameters
* INPUT:
*	netdev		Network device structure pointer
* OUTPUT
*	ring		Ring paranmeters
* RETURN:
*	None
*
*******************************************************************************/
void mv_eth_tool_get_ringparam(struct net_device *netdev,
				struct ethtool_ringparam *ring)
{
/*	printk("in %s \n",__FUNCTION__); */
}

/******************************************************************************
* mv_eth_tool_get_pauseparam
* Description:
*	ethtool get pause parameters
* INPUT:
*	netdev		Network device structure pointer
* OUTPUT
*	pause		Pause paranmeters
* RETURN:
*	None
*
*******************************************************************************/
void mv_eth_tool_get_pauseparam(struct net_device *netdev,
				struct ethtool_pauseparam *pause)
{
	struct eth_port      *priv = MV_ETH_PRIV(netdev);
	int                  port = priv->port;
	MV_ETH_PORT_STATUS   portStatus;
	MV_ETH_PORT_FC       flowCtrl;

	if ((priv == NULL) || (isSwitch(priv)) || (MV_PON_PORT(priv->port))) {
		printk(KERN_ERR "%s is not supported on %s\n", __func__, netdev->name);
		return;
	}

	mvNetaFlowCtrlGet(port, &flowCtrl);
	if ((flowCtrl == MV_ETH_FC_AN_NO) || (flowCtrl == MV_ETH_FC_AN_SYM) || (flowCtrl == MV_ETH_FC_AN_ASYM))
		pause->autoneg = AUTONEG_ENABLE;
	else
		pause->autoneg = AUTONEG_DISABLE;

	mvNetaLinkStatus(port, &portStatus);
	if (portStatus.rxFc == MV_ETH_FC_DISABLE)
		pause->rx_pause = 0;
	else
		pause->rx_pause = 1;

	if (portStatus.txFc == MV_ETH_FC_DISABLE)
		pause->tx_pause = 0;
	else
		pause->tx_pause = 1;
}




/******************************************************************************
* mv_eth_tool_set_pauseparam
* Description:
*	ethtool configure pause parameters
* INPUT:
*	netdev		Network device structure pointer
*	pause		Pause paranmeters
* OUTPUT
*	None
* RETURN:
*	0 on success
*
*******************************************************************************/
int mv_eth_tool_set_pauseparam(struct net_device *netdev,
				struct ethtool_pauseparam *pause)
{
	struct eth_port *priv = MV_ETH_PRIV(netdev);
	int				port = priv->port;
	MV_U32			phy_addr;
	MV_STATUS		status = MV_FAIL;

	if ((priv == NULL) || (isSwitch(priv)) || (MV_PON_PORT(priv->port))) {
		printk(KERN_ERR "%s is not supported on %s\n", __func__, netdev->name);
		return -EOPNOTSUPP;
	}

	if (pause->rx_pause && pause->tx_pause) { /* Enable FC */
		if (pause->autoneg) { /* autoneg enable */
			status = mvNetaFlowCtrlSet(port, MV_ETH_FC_AN_SYM);
		} else { /* autoneg disable */
			status = mvNetaFlowCtrlSet(port, MV_ETH_FC_ENABLE);
		}
	} else if (!pause->rx_pause && !pause->tx_pause) { /* Disable FC */
		if (pause->autoneg) { /* autoneg enable */
			status = mvNetaFlowCtrlSet(port, MV_ETH_FC_AN_NO);
		} else { /* autoneg disable */
			status = mvNetaFlowCtrlSet(port, MV_ETH_FC_DISABLE);
		}
	}
	/* Only symmetric change for RX and TX flow control is allowed */
	if (status == MV_OK) {
		phy_addr = mvBoardPhyAddrGet(priv->port);
		status = mvEthPhyRestartAN(phy_addr, MV_ETH_TOOL_AN_TIMEOUT);
	}
	if (status != MV_OK)
		return -EINVAL;

	return 0;
}

/******************************************************************************
* mv_eth_tool_get_strings
* Description:
*	ethtool get strings (used for statistics and self-test descriptions)
* INPUT:
*	netdev		Network device structure pointer
*	stringset	strings parameters
* OUTPUT
*	data		output data
* RETURN:
*	None
*
*******************************************************************************/
void mv_eth_tool_get_strings(struct net_device *netdev,
			     uint32_t stringset, uint8_t *data)
{
}

/******************************************************************************
* mv_eth_tool_get_stats_count
* Description:
*	ethtool get statistics count (number of stat. array entries)
* INPUT:
*	netdev		Network device structure pointer
* OUTPUT
*	None
* RETURN:
*	statistics count
*
*******************************************************************************/
int mv_eth_tool_get_stats_count(struct net_device *netdev)
{
	return 0;
}

#if LINUX_VERSION_CODE < KERNEL_VERSION(3, 4, 25)
static int mv_eth_tool_get_rxfh_indir(struct net_device *netdev,
					struct ethtool_rxfh_indir *indir)
{
#if defined(MV_ETH_PNC_LB) && defined(CONFIG_MV_ETH_PNC)
	struct eth_port 	*priv = MV_ETH_PRIV(netdev);
	size_t copy_size =
		min_t(size_t, indir->size, ARRAY_SIZE(priv->rx_indir_table));

	indir->size = ARRAY_SIZE(priv->rx_indir_table);

	memcpy(indir->ring_index, priv->rx_indir_table,
	       copy_size * sizeof(indir->ring_index[0]));
	return 0;
#else
	return -EOPNOTSUPP;
#endif
}

static int mv_eth_tool_set_rxfh_indir(struct net_device *netdev,
				   const struct ethtool_rxfh_indir *indir)
{
#if defined(MV_ETH_PNC_LB) && defined(CONFIG_MV_ETH_PNC)
	int i;
	struct eth_port 	*priv = MV_ETH_PRIV(netdev);
	for (i = 0; i < indir->size; i++) {
		priv->rx_indir_table[i] = indir->ring_index[i];
		mvPncLbRxqSet(i, priv->rx_indir_table[i]);
	}
	return 0;
#else
	return -EOPNOTSUPP;
#endif
}

static int mv_eth_tool_get_rxnfc(struct net_device *dev, struct ethtool_rxnfc *info,
									u32 *rules)
{
	if (info->cmd == ETHTOOL_GRXRINGS) {
		struct eth_port *pp = MV_ETH_PRIV(dev);
		if (pp)
			info->data = ARRAY_SIZE(pp->rx_indir_table);
	}
	return 0;
}

/******************************************************************************
* mv_eth_tool_set_rx_ntuple
* Description:
*	ethtool set mapping from 2t/5t rule to rxq/drop
*	ignore mask parameters (assume mask=0xFF for each byte provided)
*	support only tcp4 / udp4 protocols
*	support only full 2t/5t rules:
*		** 2t - must provide src-ip, dst-ip
*		** 5t - must provide src-ip, dst-ip, src-port, dst-port
* INPUT:
*	netdev		Network device structure pointer
*	ntuple
* OUTPUT
*	None
* RETURN:
*
*******************************************************************************/
static int mv_eth_tool_set_rx_ntuple(struct net_device *dev, struct ethtool_rx_ntuple *ntuple)
{
#ifdef CONFIG_MV_ETH_PNC_L3_FLOW
	unsigned int sip, dip, ports, sport, dport, proto;
	struct eth_port *pp;

	if ((ntuple->fs.flow_type != TCP_V4_FLOW) && (ntuple->fs.flow_type != UDP_V4_FLOW))
		return -EOPNOTSUPP;

	if ((ntuple->fs.action >= CONFIG_MV_ETH_RXQ) || (ntuple->fs.action < ETHTOOL_RXNTUPLE_ACTION_CLEAR))
		return -EINVAL;

	if (ntuple->fs.flow_type == TCP_V4_FLOW)
		proto = 6; /* tcp */
	else
		proto = 17; /* udp */

	sip = ntuple->fs.h_u.tcp_ip4_spec.ip4src;
	dip = ntuple->fs.h_u.tcp_ip4_spec.ip4dst;
	sport = ntuple->fs.h_u.tcp_ip4_spec.psrc;
	dport = ntuple->fs.h_u.tcp_ip4_spec.pdst;
	if (!sip || !dip)
		return -EINVAL;

	pp = MV_ETH_PRIV(dev);
	if (!sport || !dport) { /* 2-tuple */
		pnc_ip4_2tuple_rxq(pp->port, sip, dip, ntuple->fs.action);
	} else {
		ports = (dport << 16) | ((sport << 16) >> 16);
		pnc_ip4_5tuple_rxq(pp->port, sip, dip, ports, proto, ntuple->fs.action);
	}

	return 0;
#else
	return 1;
#endif /* CONFIG_MV_ETH_PNC_L3_FLOW */
}
#endif /* #if LINUX_VERSION_CODE < KERNEL_VERSION(3, 4, 25) */

/******************************************************************************
* mv_eth_tool_get_ethtool_stats
* Description:
*	ethtool get statistics
* INPUT:
*	netdev		Network device structure pointer
*	stats		stats parameters
* OUTPUT
*	data		output data
* RETURN:
*	None
*
*******************************************************************************/
void mv_eth_tool_get_ethtool_stats(struct net_device *netdev,
				   struct ethtool_stats *stats, uint64_t *data)
{

}

const struct ethtool_ops mv_eth_tool_ops = {
	.get_settings				= mv_eth_tool_get_settings,
	.set_settings				= mv_eth_tool_set_settings,
	.get_drvinfo				= mv_eth_tool_get_drvinfo,
	.get_regs_len				= mv_eth_tool_get_regs_len,
	.get_regs				= mv_eth_tool_get_regs,
	.nway_reset				= mv_eth_tool_nway_reset,
	.get_link				= mv_eth_tool_get_link,
	.get_coalesce				= mv_eth_tool_get_coalesce,
	.set_coalesce				= mv_eth_tool_set_coalesce,
	.get_ringparam  			= mv_eth_tool_get_ringparam,
	.get_pauseparam				= mv_eth_tool_get_pauseparam,
	.set_pauseparam				= mv_eth_tool_set_pauseparam,
	.get_strings				= mv_eth_tool_get_strings,/*TODO: complete implementation */
	.get_ethtool_stats			= mv_eth_tool_get_ethtool_stats,

#if LINUX_VERSION_CODE <= KERNEL_VERSION(2, 6, 32)
	.get_stats_count			= mv_eth_tool_get_stats_count,
#endif

#if LINUX_VERSION_CODE < KERNEL_VERSION(3, 4, 25)
	.get_rxfh_indir				= mv_eth_tool_get_rxfh_indir,
	.set_rxfh_indir				= mv_eth_tool_set_rxfh_indir,
	.get_rxnfc				= mv_eth_tool_get_rxnfc,
	.set_rx_ntuple				= mv_eth_tool_set_rx_ntuple,
#endif
};

