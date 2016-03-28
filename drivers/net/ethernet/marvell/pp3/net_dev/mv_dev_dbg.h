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

#include "platform/mv_pp3.h"

void pp3_dbg_dev_resources_dump(void);
void pp3_dbg_dev_txq_status_print(struct net_device *dev, int queue);
void pp3_dbg_dev_rxq_status_print(struct net_device *dev, int queue);
void pp3_dbg_dev_flags(struct net_device *dev, u32 flag, u32 en);
void pp3_dbg_dev_status_print(struct net_device *dev);
void pp3_dbg_dev_pools_status_print(struct net_device *dev);
void pp3_dbg_dev_mac_mc_print(struct net_device *dev);
void pp3_dbg_cpu_status_print(void);
void pp3_dbg_cpu_flags(int cpu, u32 flag, u32 en);
void pp3_dbg_skb_dump(struct sk_buff *skb);
void pp3_dbg_dev_stats_dump(struct net_device *dev);
void pp3_dbg_dev_queues_stats_dump(struct net_device *dev);
void pp3_dbg_dev_pools_stats_dump(struct net_device *dev);
void pp3_dbg_dev_fw_stats_dump(struct net_device *dev);
void pp3_dbg_dev_stats_clear(struct net_device *dev);
void pp3_dbg_cfh_common_dump(struct mv_cfh_common *cfh);
void pp3_dbg_cfh_rx_dump(struct mv_cfh_common *rx_cfh);
void pp3_dbg_cfh_hdr_dump(struct mv_cfh_common *cfh);
int pp3_dbg_cfh_rx_checker(struct pp3_dev_priv *dev_priv, u32 *ptr);
void pp3_dbg_txdone_occ_show(int cpu);
void pp3_dbg_dev_path_stats_dump(struct pp3_dev_priv *dev_src, struct pp3_dev_priv *dev_dst);

void pp3_dbg_ingress_vqs_print(struct net_device *dev);
void pp3_dbg_ingress_vqs_show(struct net_device *dev);
void pp3_dbg_ingress_qos_show(struct net_device *dev);

void pp3_dbg_egress_vqs_print(struct net_device *dev);
void pp3_dbg_egress_vqs_show(struct net_device *dev);
void pp3_dbg_egress_qos_show(struct net_device *dev);

void pp3_dbg_dev_mac_show(struct net_device *dev);
char *pp3_dbg_l2_info_str(unsigned int l2_info);
char *pp3_dbg_vlan_info_str(unsigned int vlan_info);
char *pp3_dbg_l3_info_str(unsigned int l3_info);
char *pp3_dbg_l4_info_str(unsigned int l4_info);
