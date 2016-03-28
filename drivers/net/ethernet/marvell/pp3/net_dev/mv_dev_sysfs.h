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

#ifndef __mv_dev_sysfs_h__
#define __mv_dev_sysfs_h__

int mv_pp3_dev_sysfs_init(struct kobject *pp3_kobj);
int mv_pp3_dev_sysfs_exit(struct kobject *pp3_kobj);

int mv_pp3_dev_init_sysfs_init(struct kobject *dev_kobj);
int mv_pp3_dev_init_sysfs_exit(struct kobject *dev_kobj);

int mv_pp3_dev_debug_sysfs_init(struct kobject *dev_kobj);
int mv_pp3_dev_debug_sysfs_exit(struct kobject *dev_kobj);

int mv_pp3_dev_vq_sysfs_init(struct kobject *dev_kobj);
int mv_pp3_dev_vq_sysfs_exit(struct kobject *dev_kobj);

int mv_pp3_dev_qos_sysfs_init(struct kobject *dev_kobj);
int mv_pp3_dev_qos_sysfs_exit(struct kobject *dev_kobj);

int mv_pp3_dev_rss_sysfs_init(struct kobject *dev_kobj);
int mv_pp3_dev_rss_sysfs_exit(struct kobject *dev_kobj);

int mv_pp3_dev_bpi_sysfs_init(struct kobject *dev_kobj);
int mv_pp3_dev_bpi_sysfs_exit(struct kobject *dev_kobj);

int mv_pp3_dev_fp_sysfs_init(struct kobject *dev_kobj);
int mv_pp3_dev_fp_sysfs_exit(struct kobject *dev_kobj);

#endif /* __mv_dev_sysfs_h__ */
