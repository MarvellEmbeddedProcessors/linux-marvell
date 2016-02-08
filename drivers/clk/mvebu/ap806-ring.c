/*
 * Marvell Armada AP806 ring clocks
 *
 * Copyright (C) 2016 Marvell
 *
 * Thomas Petazzoni <thomas.petazzoni@free-electrons.com>
 *
 * This file is licensed under the terms of the GNU General Public
 * License version 2.  This program is licensed "as is" without any
 * warranty of any kind, whether express or implied.
 */

#include <linux/kernel.h>
#include <linux/clk.h>
#include <linux/clk-provider.h>
#include <linux/io.h>
#include <linux/of.h>
#include <linux/of_address.h>

#define AP806_RING_DIV_NUM	5

static struct clk *ap806_ring_clks[AP806_RING_DIV_NUM];

static struct clk_onecell_data ap806_ring_clk_data = {
	.clks = ap806_ring_clks,
	.clk_num = AP806_RING_DIV_NUM,
};

static void __init ap806_ring_clk_init(struct device_node *np)
{
	void __iomem *base;
	const char *parent;
	u32 reg;
	int i;

	base = of_iomap(np, 0);
	if (WARN_ON(!base))
		return;

	reg = readl(base);

	iounmap(base);

	parent = of_clk_get_parent_name(np, 0);

	for (i = 0; i < AP806_RING_DIV_NUM; i++) {
		unsigned long divider;
		const char *name;

		/* Each clock is represented by 6 bits */
		divider = (reg >> (6 * i)) & 0x3f;

		of_property_read_string_index(np, "clock-output-names",
					      i, &name);

		ap806_ring_clks[i] =
			clk_register_fixed_factor(NULL, name, parent,
						  0, 1, divider);
	}

	of_clk_add_provider(np, of_clk_src_onecell_get, &ap806_ring_clk_data);
}

CLK_OF_DECLARE(ap806_ring_clk, "marvell,armada-ap806-ring-clock",
	       ap806_ring_clk_init);
