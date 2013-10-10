/*
 * Symmetric Multi Processing (SMP) support for Armada 375
 *
 * Copyright (C) 2013 Marvell
 *
 * Gregory CLEMENT <gregory.clement@free-electrons.com>
 *
 * This file is licensed under the terms of the GNU General Public
 * License version 2.  This program is licensed "as is" without any
 * warranty of any kind, whether express or implied.
 *
 */

#include <linux/init.h>
#include <linux/io.h>
#include <linux/of.h>
#include <linux/smp.h>
#include <asm/smp_scu.h>
#include "armada-375.h"
#include "common.h"
#ifdef CONFIG_A375_CPU1_ENABLE_WA
#include <linux/mbus.h>
#endif

#include "pmsu.h"

#ifdef CONFIG_A375_CPU1_ENABLE_WA
#define CRYPT0_ENG_ID	41
#define CRYPT0_ENG_ATTR	0x1
#define SRAM_PHYS_BASE	0xFFFF0000

extern void* a375_smp_cpu1_enable_code_end;
extern void* a375_smp_cpu1_enable_code_start;

void a375_smp_cpu1_enable_wa(void)
{
	u32 code_len;
	void __iomem *sram_virt_base;

	mvebu_mbus_add_window_by_id(CRYPT0_ENG_ID, CRYPT0_ENG_ATTR,
				SRAM_PHYS_BASE, SZ_64K);
	sram_virt_base = ioremap(SRAM_PHYS_BASE, SZ_64K);

	code_len = 4 * (&a375_smp_cpu1_enable_code_end
			- &a375_smp_cpu1_enable_code_start);

	memcpy(sram_virt_base, &a375_smp_cpu1_enable_code_start, code_len);
}
#endif

extern void a375_secondary_startup(void);

static int __cpuinit armada_375_boot_secondary(unsigned int cpu, struct task_struct *idle)
{
	/*
	 * Write the address of secondary startup into the system-wide
	 * flags register. The boot monitor waits until it receives a
	 * soft interrupt, and then the secondary CPU branches to this
	 * address.
	 */
	armada_375_set_bootaddr(a375_secondary_startup);
	mvebu_boot_cpu(cpu);
	arch_send_wakeup_ipi_mask(cpumask_of(cpu));
	return 0;
}

/*
 * Initialise the CPU possible map early - this describes the CPUs
 * which may be present or become present in the system.
 */
static void __init armada_375_smp_init_cpus(void)
{
	struct device_node *np;
	unsigned int i, ncores;

	np = of_find_node_by_name(NULL, "cpus");
	if (!np)
		panic("No 'cpus' node found\n");

	ncores = of_get_child_count(np);
	if (ncores == 0 || ncores > ARMADA_375_MAX_CPUS)
		panic("Invalid number of CPUs in DT\n");

	/* Limit possible CPUs to defconfig */
	if (ncores > nr_cpu_ids) {
		pr_warn("SMP: %d CPUs physically present. Only %d configured.",
			ncores, nr_cpu_ids);
		pr_warn("Clipping CPU count to %d\n", nr_cpu_ids);
		ncores = nr_cpu_ids;
	}

	for (i = 0; i < ncores; ++i)
		set_cpu_possible(i, true);
}

static void __init armada_375_smp_prepare_cpus(unsigned int max_cpus)
{
	int i;

	for (i = 0; i < max_cpus; i++)
		set_cpu_present(i, true);

#ifdef CONFIG_A375_CPU1_ENABLE_WA
	a375_smp_cpu1_enable_wa();
#endif
}

struct smp_operations armada_375_smp_ops __initdata = {
	.smp_init_cpus		= armada_375_smp_init_cpus,
	.smp_prepare_cpus	= armada_375_smp_prepare_cpus,
	.smp_boot_secondary	= armada_375_boot_secondary,
#ifdef CONFIG_HOTPLUG_CPU
	.cpu_die		= armada_xp_cpu_die,
#endif
};
