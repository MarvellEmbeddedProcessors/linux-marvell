/*
 * SMP support: Entry point for secondary CPUs
 *
 * Copyright (C) 2013  Marvell
 *
 * Gregory CLEMENT <gregory.clement@free-electrons.com>
 *
 * This file is licensed under the terms of the GNU General Public
 * License version 2.  This program is licensed "as is" without any
 * warranty of any kind, whether express or implied.
 */

#include <linux/linkage.h>
#include <linux/init.h>

	__INIT

ENTRY(a375_secondary_startup)
	bl      v7_invalidate_l1
	b	secondary_startup
ENDPROC(a375_secondary_startup)
