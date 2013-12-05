/*
 * Power Management Service Unit(PMSU) support for Armada 370/XP platforms.
 *
 * Copyright (C) 2012 Marvell
 *
 * Yehuda Yitschak <yehuday@marvell.com>
 * Gregory Clement <gregory.clement@free-electrons.com>
 * Thomas Petazzoni <thomas.petazzoni@free-electrons.com>
 *
 * This file is licensed under the terms of the GNU General Public
 * License version 2.  This program is licensed "as is" without any
 * warranty of any kind, whether express or implied.
 *
 * The Armada 370 and Armada XP SOCs have a power management service
 * unit which is responsible for various tasks, including setting the
 * boot address of secondary CPUs.
 */

#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/of_address.h>
#include <linux/io.h>
#include <linux/smp.h>
#include <asm/smp_plat.h>

static void __iomem *pmsu_mp_base;

#define PMSU_BOOT_ADDR_REDIRECT_OFFSET(cpu)	((cpu * 0x100) + 0x24)

static struct of_device_id of_pmsu_table[] = {
	{.compatible = "marvell,armada-370-xp-pmsu"},
	{.compatible = "marvell,armada-380-pmsu"},
	{ /* end of list */ },
};

#ifdef CONFIG_SMP
void mvebu_pmsu_set_boot_addr(unsigned int cpu_id, void *boot_addr)
{
	int hw_cpu;

	if (!pmsu_mp_base) {
		pr_warn("Can't set CPU boot address. PMSU is uninitialized\n");
		return;
	}

	hw_cpu = cpu_logical_map(cpu_id);

	writel(virt_to_phys(boot_addr), pmsu_mp_base +
			PMSU_BOOT_ADDR_REDIRECT_OFFSET(hw_cpu));

}
#endif

static int __init armada_370_xp_pmsu_init(void)
{
	struct device_node *np;

	np = of_find_matching_node(NULL, of_pmsu_table);
	if (np) {
		pr_info("Initializing Power Management Service Unit\n");
		pmsu_mp_base = of_iomap(np, 0);
	}

	return 0;
}

early_initcall(armada_370_xp_pmsu_init);
