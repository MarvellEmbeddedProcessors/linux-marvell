/*
 * Marvell EBU cpuidle defintion
 *
 * Copyright (C) 2014 Marvell
 *
 * Gregory CLEMENT <gregory.clement@free-electrons.com>
 *
 * This file is licensed under the terms of the GNU General Public
 * License version 2.  This program is licensed "as is" without any
 * warranty of any kind, whether express or implied.
 *
 */

#ifndef __LINUX_MVEBU_V7_CPUIDLE_H__
#define __LINUX_MVEBU_V7_CPUIDLE_H__

enum mvebu_v7_cpuidle_types {
	CPUIDLE_ARMADA_XP,
	CPUIDLE_ARMADA_370,
	CPUIDLE_ARMADA_38X,
};

struct mvebu_v7_cpuidle {
	int type;
	int (*cpu_suspend)(unsigned long deepidle);
};

#endif
