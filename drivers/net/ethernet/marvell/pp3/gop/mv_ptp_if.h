/*
* ***************************************************************************
* Copyright (C) 2015 Marvell International Ltd.
* ***************************************************************************
* This program is free software: you can redistribute it and/or modify it
* under the terms of the GNU General Public License as published by the Free
* Software Foundation, either version 2 of the License, or any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program.  If not, see <http://www.gnu.org/licenses/>.
* ***************************************************************************
*/

#ifndef _mv_ptp_if_h_
#define _mv_ptp_if_h_

/* includes */
#include "gop/mv_tai_regs.h"

#define PTP_TAI_PRT_STR	"TAI/PTP"

int mv_pp3_ptp_tclk_hz_set(u32 tclk_hz); /* from dtb "clock-frequency" */
void mv_pp3_tai_clock_init(struct platform_device *pdev);
bool mv_pp3_tai_clock_external_init(struct platform_device *pdev);
void mv_pp3_tai_clock_external_init2(bool from_external);
void mv_pp3_tai_set_nop(void);
void mv_pp3_tai_clock_cfg_external(bool from_external);
void mv_pp3_tai_clock_disable(void);
bool mv_pp3_tai_clock_enable_get(void);
void mv_pp3_tai_clock_stable_status_set(bool on);
bool mv_pp3_tai_clock_stable_status_get(void);
u16 mv_pp3_tai_clock_in_cntr_get(u32 *accumulated);
int mv_pp3_ptp_event_led_sysfs(unsigned led_gpio);

ssize_t mv_pp3_tai_clock_status_get_sysfs(char *buf);

int mv_pp3_ptp_enable(int port, bool state);
void mv_pp3_ptp_reset(int port);
void mv_pp3_ptp_reset_all_ptp_ports(void);

int mv_pp3_tai_tod_op(enum mv_pp3_tai_ptp_op op, struct mv_pp3_tai_tod *ts,
			int synced_op);
int mv_pp3_tai_tod_op_read_captured(struct mv_pp3_tai_tod *ts, u32 *status);

void mv_pp3_tai_clock_from_external_sync(int start, u32 sec, int d_sec);

void mv_pp3_ptp_reg_dump(int port);
void mv_pp3_tai_tod_dump_util(struct mv_pp3_tai_tod *ts);
void mv_pp3_tai_tod_from_linux(struct mv_pp3_tai_tod *ts);
void mv_pp3_tai_tod_to_linux(struct mv_pp3_tai_tod *ts);
void mv_pp3_tai_reg_dump(void);
void mv_pp3_tai_tod_dump(void);
int mv_pp3_tai_tod_load_set(u32 sec_h, u32 sec_l, u32 nano, u32 frac);

#endif /* _mv_ptp_if_h_ */
