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
#ifndef __mv_eth_sysfs_h__
#define __mv_eth_sysfs_h__

int mv_mux_sysfs_init(struct kobject *pp2_kobj);

/* Subdirectories of pp2 menu */
int mv_pp2_prs_low_sysfs_init(struct kobject *pp2_kobj);
int mv_pp2_prs_low_sysfs_exit(struct kobject *pp2_kobj);

int mv_pp2_prs_high_sysfs_init(struct kobject *pp2_kobj);
int mv_pp2_prs_high_sysfs_exit(struct kobject *pp2_kobj);

int mv_pp2_cls_sysfs_init(struct kobject *pp2_kobj);
int mv_pp2_cls_sysfs_exit(struct kobject *pp2_kobj);

int mv_pp2_cls2_sysfs_init(struct kobject *pp2_kobj);
int mv_pp2_cls2_sysfs_exit(struct kobject *pp2_kobj);

int mv_pp2_cls3_sysfs_init(struct kobject *pp2_kobj);
int mv_pp2_cls3_sysfs_exit(struct kobject *pp2_kobj);

int mv_pp2_cls4_sysfs_init(struct kobject *pp2_kobj);
int mv_pp2_cls4_sysfs_exit(struct kobject *pp2_kobj);

int mv_pp2_mc_sysfs_init(struct kobject *pp2_kobj);
int mv_pp2_mc_sysfs_exit(struct kobject *pp2_kobj);

int mv_pp2_pme_sysfs_init(struct kobject *pp2_kobj);
int mv_pp2_pme_sysfs_exit(struct kobject *pp2_kobj);

int mv_pp2_plcr_sysfs_init(struct kobject *pp2_kobj);
int mv_pp2_plcr_sysfs_exit(struct kobject *pp2_kobj);

int mv_pp2_gbe_sysfs_init(struct kobject *pp2_kobj);
int mv_pp2_gbe_sysfs_exit(struct kobject *pp2_kobj);

/* Subdirectories of gbe menu */
int mv_pp2_bm_sysfs_init(struct kobject *gbe_kobj);
int mv_pp2_bm_sysfs_exit(struct kobject *gbe_kobj);

int mv_pp2_napi_sysfs_init(struct kobject *pp2_kobj);
int mv_pp2_napi_sysfs_exit(struct kobject *pp2_kobj);

int mv_pp2_rx_sysfs_init(struct kobject *pp2_kobj);
int mv_pp2_rx_sysfs_exit(struct kobject *pp2_kobj);

int mv_pp2_tx_sysfs_init(struct kobject *pp2_kobj);
int mv_pp2_tx_sysfs_exit(struct kobject *pp2_kobj);

int mv_pp2_tx_sched_sysfs_init(struct kobject *pp2_kobj);
int mv_pp2_tx_sched_sysfs_exit(struct kobject *pp2_kobj);

int mv_pp2_qos_sysfs_init(struct kobject *pp2_kobj);
int mv_pp2_qos_sysfs_exit(struct kobject *pp2_kobj);

int mv_pp2_pon_sysfs_init(struct kobject *pp2_kobj);
int mv_pp2_pon_sysfs_exit(struct kobject *pp2_kobj);

int mv_pp2_gbe_pme_sysfs_init(struct kobject *gbe_kobj);
int mv_pp2_gbe_pme_sysfs_exit(struct kobject *gbe_kobj);

#ifdef CONFIG_MV_PP2_HWF
int mv_pp2_gbe_hwf_sysfs_init(struct kobject *gbe_kobj);
int mv_pp2_gbe_hwf_sysfs_exit(struct kobject *gbe_kobj);
#endif /* CONFIG_MV_PP2_HWF */

int mv_pp2_dbg_sysfs_init(struct kobject *pp2_kobj);
int mv_pp2_dbg_sysfs_exit(struct kobject *pp2_kobj);

int mv_pp2_wol_sysfs_init(struct kobject *pp2_kobj);
int mv_pp2_wol_sysfs_exit(struct kobject *pp2_kobj);

int mv_pp2_dpi_sysfs_init(struct kobject *pp2_kobj);
int mv_pp2_dpi_sysfs_exit(struct kobject *pp2_kobj);

#ifdef CONFIG_MV_PP2_L2FW
int mv_pp2_l2fw_sysfs_init(struct kobject *pp2_kobj);
int mv_pp2_l2fw_sysfs_exit(struct kobject *pp2_kobj);
#endif

#endif /* __mv_eth_sysfs_h__ */
