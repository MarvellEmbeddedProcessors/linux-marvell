/*
 * Generic definitions for Marvell Armada 375 SoCs
 *
 * Copyright (C) 2013 Marvell
 *
 * Gregory CLEMENT <gregory.clement@free-electrons.com>
 *
 * This file is licensed under the terms of the GNU General Public
 * License version 2.  This program is licensed "as is" without any
 * warranty of any kind, whether express or implied.
 */

#ifndef __MACH_ARMADA_375_H
#define __MACH_ARMADA_375_H

#ifdef CONFIG_SMP
void armada_375_set_bootaddr(void *boot_addr);
#endif

#endif /* __MACH_ARMADA_375_H */
