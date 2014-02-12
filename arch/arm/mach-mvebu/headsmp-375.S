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

	__CPUINIT
#ifdef CONFIG_A375_CPU1_ENABLE_WA

#define CPU_RESUME_ADDR_REG 0xf10182d4

.global a375_smp_cpu1_enable_code_start
.global a375_smp_cpu1_enable_code_end

ENTRY(a375_smp_cpu1_enable_code_start)
a375_smp_cpu1_enable_code_start:
	ldr     r0, [pc, #4]
	ldr     r1, [r0]
	mov     pc, r1
	.word   CPU_RESUME_ADDR_REG
a375_smp_cpu1_enable_code_end:
ENDPROC(a375_smp_cpu1_enable_code_end)
#endif /* CONFIG_A375_CPU1_ENABLE_WA */


ENTRY(a375_secondary_startup)
	bl      v7_invalidate_l1
	b	secondary_startup
ENDPROC(a375_secondary_startup)
