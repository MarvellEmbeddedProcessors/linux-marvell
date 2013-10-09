/*
 * Copyright (C) 2013 Marvell
 *
 * Thomas Petazzoni <thomas.petazzoni@free-electrons.com>
 *
 * This file is licensed under the terms of the GNU General Public
 * License version 2.  This program is licensed "as is" without any
 * warranty of any kind, whether express or implied.
 */

#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/of_address.h>
#include <linux/io.h>
#include <linux/reset-controller.h>

static struct of_device_id of_cpu_reset_table[] = {
	{.compatible = "marvell,armada-370-cpu-reset", .data = (void*) 1 },
	{.compatible = "marvell,armada-xp-cpu-reset",  .data = (void*) 4 },
	{.compatible = "marvell,armada-375-cpu-reset", .data = (void*) 2 },
	{ /* end of list */ },
};

static void __iomem *cpu_reset_base;

#define CPU_RESET_OFFSET(cpu) (cpu * 0x8)
#define CPU_RESET_ASSERT      BIT(0)

static int mvebu_cpu_reset_assert(struct reset_controller_dev *rcdev,
				  unsigned long idx)
{
	u32 reg;

	reg = readl(cpu_reset_base + CPU_RESET_OFFSET(idx));
	reg |= CPU_RESET_ASSERT;
	writel(reg, cpu_reset_base + CPU_RESET_OFFSET(idx));

	return 0;
}

static int mvebu_cpu_reset_deassert(struct reset_controller_dev *rcdev,
				    unsigned long idx)
{
	u32 reg;

	reg = readl(cpu_reset_base + CPU_RESET_OFFSET(idx));
	reg &= ~CPU_RESET_ASSERT;
	writel(reg, cpu_reset_base + CPU_RESET_OFFSET(idx));

	return 0;
}

static struct reset_control_ops mvebu_cpu_reset_ops = {
	.assert = mvebu_cpu_reset_assert,
	.deassert = mvebu_cpu_reset_deassert,
};

static struct reset_controller_dev mvebu_cpu_reset_dev = {
	.ops = &mvebu_cpu_reset_ops,
};

int __init mvebu_cpu_reset_init(void)
{
	struct device_node *np;
	const struct of_device_id *match;

	np = of_find_matching_node_and_match(NULL, of_cpu_reset_table,
					     &match);
	if (np) {
		pr_info("Initializing CPU Reset module\n");
		cpu_reset_base = of_iomap(np, 0);
		mvebu_cpu_reset_dev.of_node = np;
		mvebu_cpu_reset_dev.nr_resets =
			(unsigned int) match->data;
		reset_controller_register(&mvebu_cpu_reset_dev);
	}

	return 0;
}

early_initcall(mvebu_cpu_reset_init);
