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
Marvell Commercial License Option

If you received this File from Marvell and you have entered into a commercial
license agreement (a "Commercial License") with Marvell, the File is licensed
to you under the terms of the applicable Commercial License.

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
********************************************************************************
Marvell BSD License Option

If you received this File from Marvell, you may opt to use, redistribute and/or
modify this File under the following licensing terms.
Redistribution and use in source and binary forms, with or without modification,
are permitted provided that the following conditions are met:

    *   Redistributions of source code must retain the above copyright notice,
	this list of conditions and the following disclaimer.

    *   Redistributions in binary form must reproduce the above copyright
	notice, this list of conditions and the following disclaimer in the
	documentation and/or other materials provided with the distribution.

    *   Neither the name of Marvell nor the names of its contributors may be
	used to endorse or promote products derived from this software without
	specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

*******************************************************************************/

#include "mv_mux_tool.h"

/******************************************************************************
*mv_mux_tool_get_settings
*Description:
*	ethtool	get standard port settings
*INPUT:
*	netdev	Network device structure pointer
*OUTPUT
*	cmd	command (settings)
*RETURN:
*	0 for success
*
*******************************************************************************/
int mv_mux_tool_get_settings(struct net_device *netdev, struct ethtool_cmd *cmd)
{
	struct mux_netdev *pmux_priv = MV_MUX_PRIV(netdev);
	struct net_device *root = mux_eth_shadow[pmux_priv->port].root;

	if (!root)
		return -ENETUNREACH;

	return __ethtool_get_settings(root, cmd);
}
/******************************************************************************
*mv_mux_tool_get_drvinfo
*Description:
*	ethtool get driver information
*INPUT:
*	netdev	Network device structure pointer
*	info	driver information
*OUTPUT
*	info	driver information
*RETURN:
*	None
*
*******************************************************************************/
void mv_mux_tool_get_drvinfo(struct net_device *netdev, struct ethtool_drvinfo *info)
{
	struct mux_netdev *pmux_priv = MV_MUX_PRIV(netdev);
	struct net_device *root = mux_eth_shadow[pmux_priv->port].root;

	if (!root || !root->ethtool_ops || !root->ethtool_ops->get_drvinfo)
		return;


	root->ethtool_ops->get_drvinfo(root, info);
}

/******************************************************************************
*mv_mux_tool_get_coalesce
*Description:
*	ethtool get RX/TX coalesce parameters
*INPUT:
*	netdev	Network device structure pointer
*OUTPUT
*	cmd	Coalesce parameters
*RETURN:
*	0 on success
*
*******************************************************************************/
int mv_mux_tool_get_coalesce(struct net_device *netdev, struct ethtool_coalesce *cmd)
{
	struct mux_netdev *pmux_priv = MV_MUX_PRIV(netdev);
	struct net_device *root = mux_eth_shadow[pmux_priv->port].root;

	if (!root || !root->ethtool_ops || !root->ethtool_ops->get_coalesce)
		return -ENETUNREACH;

	return root->ethtool_ops->get_coalesce(root, cmd);

}
/******************************************************************************
*mv_mux_tool_get_pauseparam
*Description:
*	ethtool get pause parameters
*INPUT:
*	netdev	Network device structure pointer
*OUTPUT
*	pause	Pause paranmeters
*RETURN:
*	None
*
*******************************************************************************/
void mv_mux_tool_get_pauseparam(struct net_device *netdev, struct ethtool_pauseparam *pause)
{
	struct mux_netdev *pmux_priv = MV_MUX_PRIV(netdev);
	struct net_device *root = mux_eth_shadow[pmux_priv->port].root;

	if (!root || !root->ethtool_ops || !root->ethtool_ops->get_pauseparam)
		return;

	root->ethtool_ops->get_pauseparam(root, pause);
}


/******************************************************************************
* mv_mux_tool_nway_reset
* Description:
*	ethtool restart auto negotiation
* INPUT:
*	netdev	Network device structure pointer
* OUTPUT
*	None
* RETURN:
*	0 on success
*
*******************************************************************************/
#ifdef CONFIG_MV_INCLUDE_SWITCH
int mv_mux_tool_nway_reset(struct net_device *mux_dev)
{
	struct mux_netdev *pdev_priv;

	pdev_priv = MV_MUX_PRIV(mux_dev);
	/* restart group autoneg */
	if (mv_switch_group_restart_autoneg(pdev_priv->idx))
		return -EINVAL;

	return 0;
}
#endif

/******************************************************************************
* mv_mux_tool_get_link
* Description:
*	ethtool get link status
* INPUT:
*	netdev	Network device structure pointer
* OUTPUT
*	None
* RETURN:
*	0 if link is down, 1 if link is up
*
*******************************************************************************/
#ifdef CONFIG_MV_INCLUDE_SWITCH
u32 mv_mux_tool_get_link(struct net_device *mux_dev)
{
	struct mux_netdev *pdev_priv;

	pdev_priv = MV_MUX_PRIV(mux_dev);

	return mv_switch_link_status_get(pdev_priv->idx);
}
#endif


const struct ethtool_ops mv_mux_tool_ops = {
	.get_settings	= mv_mux_tool_get_settings,
	.get_pauseparam	= mv_mux_tool_get_pauseparam,
	.get_coalesce	= mv_mux_tool_get_coalesce,
	.get_link	= ethtool_op_get_link,
	.get_drvinfo	= mv_mux_tool_get_drvinfo,
#ifdef CONFIG_MV_INCLUDE_SWITCH
	.nway_reset	= mv_mux_tool_nway_reset,
	.get_link	= mv_mux_tool_get_link,
#endif
};
