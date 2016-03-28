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

#ifndef TM_SYSFS_DEBUG__H
#define TM_SYSFS_DEBUG__H

#include "common/mv_sw_if.h"

int tm_sysfs_enable_debug(uint8_t en);

int tm_sysfs_read_node(int level, uint16_t index);

int tm_sysfs_read_node_hw(int level, uint16_t index);

int tm_sysfs_print_ports_name(void);

int tm_sysfs_dump_port_hw(uint32_t port_index);

int tm_sysfs_trace_queues(uint32_t timeout, uint8_t full_path);

int tm_sysfs_set_elig(int level, uint16_t index, uint32_t eligible);

int tm_sysfs_set_elig_per_queue_range(uint32_t startInd, uint32_t endInd, uint8_t elig);

int tm_sysfs_show_elig_func(int level, uint32_t index);

const char *tm_sysfs_level_str(int level);

#endif /* TM_DEBUG_SYSFS__H */
