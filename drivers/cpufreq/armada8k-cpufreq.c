/*
 * CPU frequency scaling support for Armada-8K platform.
 *
 * Copyright (C) 2016 Marvell
 *
 * Omri Itach <omrii@marvell.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#define pr_fmt(fmt) KBUILD_MODNAME ": " fmt

#include <linux/err.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/of.h>
#include <linux/platform_device.h>

static int __init armada8k_cpufreq_driver_init(void)
{
	struct platform_device *pdev;
	struct device_node *node;

	node = of_find_compatible_node(NULL, NULL, "marvell,ap806-cpu-clk");
	if (!node || !of_device_is_available(node))
		return -ENODEV;

	pdev = platform_device_register_simple("cpufreq-dt", -1, NULL, 0);

	return PTR_ERR_OR_ZERO(pdev);
}
module_init(armada8k_cpufreq_driver_init);

MODULE_AUTHOR("Omri Itach <omrii@marvell.com>");
MODULE_DESCRIPTION("Armada 8K cpufreq driver");
MODULE_LICENSE("GPL");
