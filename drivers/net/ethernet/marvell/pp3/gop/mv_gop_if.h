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
#ifndef __mv_gop_if_h__
#define __mv_gop_if_h__

#include <linux/kernel.h>
#include <linux/io.h>
#include "common/mv_sw_if.h"


/* pp3_gop_ctrl flags */
#define mv_gop_F_DEBUG_BIT		0
#define mv_gop_F_ATTACH_BIT		1

#define mv_gop_F_DEBUG		(1 << mv_gop_F_DEBUG_BIT)
#define mv_gop_F_ATTACH		(1 << mv_gop_F_ATTACH_BIT)

enum gop_port_flags {NOT_CREATED, CREATED, UNDER_RESET, ENABLED};
enum mv_gop_access_mode {DIRECT_ACCESS, INDIRECT_MG_ACCESS};

struct gop_port_ctrl {
	enum mv_port_mode port_mode;
	u32  flags;
};

/* gop access init */
void mv_gop_init(struct mv_io_addr *gop_regs, int ports_num, enum mv_gop_access_mode mode);
int mv_gop_addrs_size_get(u32 *va_base, u32 *pa_base, u32 *size);

/* port configuration function */
int  mv_pp3_gop_port_init(int port_num, enum mv_port_mode port_mode);
int  mv_pp3_gop_port_reset(int port_num);
void mv_pp3_gop_port_enable(int port_num);
void mv_pp3_gop_port_disable(int port_num);
void mv_pp3_gop_port_periodic_xon_set(int port_num, int enable);
void mv_pp3_gop_port_lb_set(int port_num, int is_gmii, int is_pcs_en);
bool mv_pp3_gop_port_is_link_up(int port_num);
int  mv_pp3_gop_port_link_status(int port_num, struct mv_port_link_status *pstatus);

/* get port speed and duplex */
int mv_pp3_gop_speed_duplex_get(int port_num, enum mv_port_speed *speed, enum mv_port_duplex *duplex);
/* set port speed and duplex */
int mv_pp3_gop_speed_duplex_set(int port_num, enum mv_port_speed speed, enum mv_port_duplex duplex);
int mv_pp3_gop_autoneg_restart(int port_num);
int mv_pp3_gop_fl_cfg(int port_num);

/* sysfs functions */
int mv_pp3_gop_sysfs_init(struct kobject *pp3_kobj);
int mv_pp3_gop_sysfs_exit(struct kobject *pp3_kobj);
int mv_pp3_gop_port_regs(int port_num);
int mv_pp3_gop_status_show(int port_num);

/* interrupt processing */
int mv_pp3_gop_port_events_unmask(int port_num);
int mv_pp3_gop_port_events_mask(int port_num);
int mv_pp3_gop_port_events_clear(int port_num);

/******************************************************************************/
/*                      GOP register acceess Functions                        */
/******************************************************************************/
u32  mv_gop_reg_read(u32 reg_addr);
void mv_gop_reg_write(u32 reg_addr, u32 data);
void mv_gop_reg_print(char *reg_name, u32 reg);

void mv_gop_isr_summary_mask(int port);
void mv_gop_isr_summary_unmask(int port);
u32 mv_gop_isr_summary_cause_get(int port);
u32 mv_gop_port_isr_cause_get(int port);
void mv_gop_port_isr_mask(int port);
void mv_gop_port_isr_unmask(int port);
void mv_gop_port_sum_isr_mask(int port);
void mv_gop_port_sum_isr_unmask(int port);

/******************************************************************************/
/*                         MIB Counters Functions                             */
/******************************************************************************/
u32 mv_pp3_gop_mib_counter_get(int port, u32 offset, u32 *p_high_32);
void mv_pp3_gop_mib_counters_clear(int port);
void mv_pp3_gop_mib_counters_show(int port);

#endif /* __mv_gop_if_h__ */
