/*
 * CPU frequency scaling support for Armada 3700 platform.
 *
 * Copyright (C) 2017 Marvell
 *
 * Victor Gu <xigu@marvell.com>
 * Evan Wang <xswang@marvell.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/clk.h>
#include <linux/cpu.h>
#include <linux/cpufreq.h>
#include <linux/err.h>
#include <linux/interrupt.h>
#include <linux/io.h>
#include <linux/module.h>
#include <linux/of_address.h>
#include <linux/of_device.h>
#include <linux/of_irq.h>
#include <linux/pm_opp.h>
#include <linux/platform_device.h>
#include <linux/slab.h>

/* CPU LOAD index */
enum armada3700_cpu_load_index {
	MVEBU_CPU_LOAD_0 = 0,
	MVEBU_CPU_LOAD_1,
	MVEBU_CPU_LOAD_2,
	MVEBU_CPU_LOAD_3,
	MVEBU_CPU_LOAD_MAX
};

#define MHZ_TO_HZ 1000000

struct a3700_cpu_freq_div_info {
	u32 cpu_freq_max;/* MHz */
	u8 divider[MVEBU_CPU_LOAD_MAX];
};

static struct a3700_cpu_freq_div_info a3700_divider_info[] = {
	{.cpu_freq_max = 1200, .divider = {1, 2, 4, 6} },
	{.cpu_freq_max = 1000, .divider = {1, 2, 4, 5} },
	{.cpu_freq_max = 800,  .divider = {1, 2, 3, 4} },
	{.cpu_freq_max = 600,  .divider = {2, 4, 5, 6} },
};

static struct a3700_cpu_freq_div_info *armada_3700_cpu_freq_info_get(u32 max_cpu_freq)
{
	int i;

	for (i = 0; i < ARRAY_SIZE(a3700_divider_info); i++) {
		if (max_cpu_freq == a3700_divider_info[i].cpu_freq_max)
			break;
	}
	if (i == ARRAY_SIZE(a3700_divider_info)) {
		pr_err("unsupported CPU frequency %d MHz\n", max_cpu_freq);
		return NULL;
	}

	return &a3700_divider_info[i];
}

static int __init armada3700_cpufreq_driver_init(void)
{
	struct platform_device *pdev;
	struct device_node *node;
	struct a3700_cpu_freq_div_info *divider_info;
	struct device *cpu_dev;
	struct clk *clk;
	int load_level, ret;
	unsigned int cur_frequency;

	node = of_find_compatible_node(NULL, NULL, "marvell,armada-37xx-cpu-pm-clk");
	if (!node || !of_device_is_available(node))
		return -ENODEV;

	/*
	 * On CPU 0 register the operating points
	 * supported (which are the nominal CPU frequency and full integer
	 * divisions of it).
	 */
	cpu_dev = get_cpu_device(0);
	if (!cpu_dev) {
		dev_err(cpu_dev, "Cannot get CPU\n");
		return -ENODEV;
	}

	clk = clk_get(cpu_dev, 0);
	if (IS_ERR(clk)) {
		dev_err(cpu_dev, "Cannot get clock for CPU0\n");
		return PTR_ERR(clk);
	}

	/* Get nominal (current) CPU frequency */
	cur_frequency = clk_get_rate(clk);
	if (!cur_frequency) {
		dev_err(cpu_dev, "Failed to get clock rate for CPU\n");
		return -EINVAL;
	}

	divider_info = armada_3700_cpu_freq_info_get(cur_frequency / MHZ_TO_HZ);
	if (!divider_info) {
		dev_err(cpu_dev, "Failed to get freq divider info for CPU\n");
		return -EINVAL;
	}

	/*
	 * In case of a failure of dev_pm_opp_add(), we don't
	 * bother with cleaning up the registered OPP (there's
	 * no function to do so), and simply cancel the
	 * registration of the cpufreq device.
	 * Armada 3700 supports up to four CPU loads, but here register higher three loads,
	 * the lowest CPU load will be added by DTS.
	 */
	cur_frequency *= divider_info->divider[MVEBU_CPU_LOAD_0];
	for (load_level = MVEBU_CPU_LOAD_0; load_level < MVEBU_CPU_LOAD_3; load_level++) {
		ret = dev_pm_opp_add(cpu_dev, cur_frequency / divider_info->divider[load_level], 0);
		if (ret)
			return ret;
	}

	pdev = platform_device_register_simple("cpufreq-dt", -1, NULL, 0);

	return PTR_ERR_OR_ZERO(pdev);
}
module_init(armada3700_cpufreq_driver_init);

MODULE_AUTHOR("Victor Gu <xigu@marvell.com>");
MODULE_AUTHOR("Evan Wang <xswang@marvell.com>");
MODULE_DESCRIPTION("Armada 3700 cpufreq driver");
MODULE_LICENSE("GPL");

