/*
 * Core functions for Marvell System On Chip
 *
 * Copyright (C) 2012 Marvell
 *
 * Lior Amsalem <alior@marvell.com>
 * Gregory CLEMENT <gregory.clement@free-electrons.com>
 * Thomas Petazzoni <thomas.petazzoni@free-electrons.com>
 *
 * This file is licensed under the terms of the GNU General Public
 * License version 2.  This program is licensed "as is" without any
 * warranty of any kind, whether express or implied.
 */

#ifndef __ARCH_MVEBU_COMMON_H
#define __ARCH_MVEBU_COMMON_H

#define LSP_VERSION    "linux-3.10.70-2015_T1.1p7"

void mvebu_restart(char mode, const char *cmd);

void armada_xp_cpu_die(unsigned int cpu);
void mvebu_pmsu_set_cpu_boot_addr(int hw_cpu, void *boot_addr);
int mvebu_boot_cpu(int cpu);

int mvebu_pm_suspend_init(void (*board_pm_enter)(void __iomem *sdram_reg,
							u32 srcmd));
void mvebu_pm_register_init(int (*board_pm_init)(void));
#endif
