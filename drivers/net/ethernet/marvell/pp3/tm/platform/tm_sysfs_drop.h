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

#ifndef TM_SYSFS_DROP__H
#define TM_SYSFS_DROP__H

#include "common/mv_sw_if.h"

int tm_sysfs_read_drop_profiles(void);

int tm_sysfs_read_wred_curves(void);

int tm_sysfs_params_show(void);

int tm_sysfs_read_drop_profile(int level, int cos, uint16_t prof_index);

int tm_sysfs_read_drop_profile_bw(int level, int cos, uint16_t prof_index);

int tm_sysfs_cbtd_thr_set(uint32_t threshold);

int tm_sysfs_catd_thr_set(int color, uint32_t threshold);

int tm_sysfs_wred_thr_set(int color, uint32_t min, uint32_t max);

int tm_sysfs_wred_curve_set(int color, uint32_t curve_ind, uint32_t curve_scale);

int tm_sysfs_drop_profile_set(int level, uint16_t index, int cos);


#endif /* TM_SYSFS_DROP__H */
