/*
 * Marvell Armada AP806 core clocks
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

/*
 * AP806 PLLs:
 *   0 - DDR
 *   1 - Ring
 *   2 - CPU
 */

#define AP806_PLL_NUM	3
#define AP806_PLL_FREQ  7

/* SAR parameters to get the PLL data */
struct apclk_sar {
	int mask;
	int offset;
	const char *name;
};

static const struct apclk_sar
ap806_core_clk_sar[AP806_PLL_NUM]  __initconst = {
	{ .mask = 0x7,	.offset = 21 },
	{ .mask = 0x7,	.offset = 18 },
	{ .mask = 0x7,	.offset = 15 },
};

static struct clk *ap806_core_clks[AP806_PLL_NUM];

static struct clk_onecell_data ap806_core_clk_data = {
	.clks = ap806_core_clks,
	.clk_num = AP806_PLL_NUM,
};

/* mapping between SAR value to frequency */
static const u32
ap806_core_clk_freq[AP806_PLL_NUM][AP806_PLL_FREQ] __initconst = {
	{ 2400000000, 2100000000, 1800000000,
	  1600000000, 1300000000, 1300000000,
	  1300000000 },
	{ 2000000000, 1800000000, 1600000000,
	  1400000000, 1200000000, 1200000000,
	  1200000000 },
	{ 2500000000, 2200000000, 2000000000,
	  1700000000, 1600000000, 1200000000,
	  1200000000 },
};

static unsigned long __init ap806_core_clk_get_freq(u32 reg, int clk_idx)
{
	int freq_idx;
	const struct apclk_sar *clk_info;

	clk_info = &ap806_core_clk_sar[clk_idx];

	freq_idx = (reg >> clk_info->offset) & clk_info->mask;
	if (WARN_ON(freq_idx > AP806_PLL_FREQ))
		return 0;
	else
		return ap806_core_clk_freq[clk_idx][freq_idx];
}

static void __init ap806_core_clk_init(struct device_node *np)
{
	void __iomem *base;
	u32 reg;
	int i;

	base = of_iomap(np, 0);
	if (WARN_ON(!base))
		return;

	reg = readl(base);

	iounmap(base);

	for (i = 0; i < AP806_PLL_NUM; i++) {
		unsigned long freq;
		const char *name;

		freq = ap806_core_clk_get_freq(reg, i);

		of_property_read_string_index(np, "clock-output-names",
					      i, &name);

		ap806_core_clks[i] =
			clk_register_fixed_rate(NULL, name, NULL,
						CLK_IS_ROOT, freq);
	}

	of_clk_add_provider(np, of_clk_src_onecell_get,
			    &ap806_core_clk_data);
}

CLK_OF_DECLARE(ap806_core_clk, "marvell,armada-ap806-core-clock",
	       ap806_core_clk_init);
