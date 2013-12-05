/*
 * Copyright (C) 2013 Marvell
 *
 * Thomas Petazzoni <thomas.petazzoni@free-electrons.com>
 *
 * This file is licensed under the terms of the GNU General Public
 * License version 2.  This program is licensed "as is" without any
 * warranty of any kind, whether express or implied.
 */

#include <linux/err.h>
#include <linux/of.h>
#include <linux/of_address.h>
#include <linux/reset.h>
#include <asm/smp_plat.h>

/*
 * This function uses the reset framework to deassert a CPU, which
 * makes it boot.
 */
int mvebu_boot_cpu(int cpu)
{
	struct device_node *np;
	int hw_cpu = cpu_logical_map(cpu);

	for_each_node_by_type(np, "cpu") {
		struct reset_control *rstc;
		int icpu;

		of_property_read_u32(np, "reg", &icpu);
		if (icpu != hw_cpu)
			continue;

		rstc = of_reset_control_get(np, NULL);
		if (IS_ERR(rstc)) {
			pr_err("Cannot get reset for CPU%d\n", cpu);
			return PTR_ERR(rstc);
		}

		reset_control_deassert(rstc);
		reset_control_put(rstc);
		return 0;
	}

	return -EINVAL;
}
