/*
 * Symmetric Multi Processing (SMP) support for Armada 380/385
 *
 * Copyright (C) 2013 Marvell
 *
 * Gregory CLEMENT <gregory.clement@free-electrons.com>
 * Thomas Petazzoni <thomas.petazzoni@free-electrons.com>
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
#include <linux/cpu.h>
#include <linux/irq.h>
#include <asm/smp_scu.h>
#include "armada-380.h"
#include "common.h"

#include "pmsu.h"

extern void a380_secondary_startup(void);
static struct notifier_block armada_380_secondary_cpu_notifier;

static int __cpuinit armada_380_boot_secondary(unsigned int cpu, struct task_struct *idle)
{
	mvebu_pmsu_set_cpu_boot_addr(cpu, a380_secondary_startup);
	mvebu_boot_cpu(cpu);
	arch_send_wakeup_ipi_mask(cpumask_of(cpu));
	return 0;
}

/*
 * Initialise the CPU possible map early - this describes the CPUs
 * which may be present or become present in the system.
 */
static void __init armada_380_smp_init_cpus(void)
{
	struct device_node *np;
	unsigned int i, ncores;

	np = of_find_node_by_name(NULL, "cpus");
	if (!np)
		panic("No 'cpus' node found\n");

	ncores = of_get_child_count(np);
	if (ncores == 0 || ncores > ARMADA_380_MAX_CPUS)
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

static void __init armada_380_smp_prepare_cpus(unsigned int max_cpus)
{
	int i;

	for (i = 0; i < max_cpus; i++)
		set_cpu_present(i, true);

	/*
	 * Register notifier to unmask SOC Private Peripheral Interrupt on
	 * second core. It have to be done here because the interrupt
	 * cannot be enabled from another CPU.
	 */
	register_cpu_notifier(&armada_380_secondary_cpu_notifier);
}

#ifdef CONFIG_HOTPLUG_CPU
static void armada_38x_cpu_die(unsigned int cpu)
{
	/*
	 * CPU hotplug is implemented by putting offline CPUs into the
	 * deep idle sleep state.
	 */
	armada_38x_do_cpu_suspend(true);
}
#endif

struct smp_operations armada_380_smp_ops __initdata = {
	.smp_init_cpus		= armada_380_smp_init_cpus,
	.smp_prepare_cpus	= armada_380_smp_prepare_cpus,
	.smp_boot_secondary	= armada_380_boot_secondary,
#ifdef CONFIG_HOTPLUG_CPU
	.cpu_die		= armada_38x_cpu_die,
#endif
};

/*
 * CPU Notifier for enabling the SOC Private Peripheral Interrupts on CPU1.
 */
static int __cpuinit armada_380_secondary_init(struct notifier_block *nfb,
					       unsigned long action, void *hcpu)
{
	struct irq_data *irqd;

	if (action == CPU_STARTING || action == CPU_STARTING_FROZEN) {
		irqd = irq_get_irq_data(IRQ_PRIV_MPIC_PPI_IRQ);
		if (irqd && irqd->chip && irqd->chip->irq_unmask)
			irqd->chip->irq_unmask(irqd);
	}

	return NOTIFY_OK;
}

static struct notifier_block __cpuinitdata armada_380_secondary_cpu_notifier = {
	.notifier_call = armada_380_secondary_init,
	.priority = INT_MIN,
};
