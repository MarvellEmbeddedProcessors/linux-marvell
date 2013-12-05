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


/* Subdirectories of neta menu */

int mv_neta_pme_sysfs_init(struct kobject *pp2_kobj);
int mv_neta_pme_sysfs_exit(struct kobject *pp2_kobj);

int mv_neta_gbe_sysfs_init(struct kobject *pp2_kobj);
int mv_neta_gbe_sysfs_exit(struct kobject *pp2_kobj);

int mv_neta_bm_sysfs_init(struct kobject *gbe_kobj);
int mv_neta_bm_sysfs_exit(struct kobject *gbe_kobj);
int mv_neta_hwf_sysfs_init(struct kobject *gbe_kobj);
int mv_neta_hwf_sysfs_exit(struct kobject *gbe_kobj);
int mv_neta_pnc_sysfs_init(struct kobject *gbe_kobj);
int mv_neta_pnc_sysfs_exit(struct kobject *gbe_kobj);

int mv_neta_wol_sysfs_init(struct kobject *gbe_kobj);
int mv_neta_wol_sysfs_exit(struct kobject *gbe_kobj);

int mv_neta_pon_sysfs_init(struct kobject *gbe_kobj);
int mv_neta_pon_sysfs_exit(struct kobject *gbe_kobj);
#ifdef CONFIG_MV_ETH_L2FW
int mv_neta_l2fw_sysfs_init(struct kobject *pp2_kobj);
int mv_neta_l2fw_sysfs_exit(struct kobject *pp2_kobj);
#endif

#endif /* __mv_eth_sysfs_h__ */
