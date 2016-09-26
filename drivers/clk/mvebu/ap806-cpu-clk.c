/*
 * Marvell Armada AP806 CPU Clock Controller
 *
 * Copyright (C) 2016 Marvell
 *
 * Omri Itach <omrii@marvell.com>
 *
 * This file is licensed under the terms of the GNU General Public
 * License version 2.  This program is licensed "as is" without any
 * warranty of any kind, whether express or implied.
 */

#include <linux/clk-provider.h>
#include <linux/clk.h>
#include <linux/err.h>
#include <linux/kernel.h>
#include <linux/mfd/syscon.h>
#include <linux/of.h>
#include <linux/of_device.h>
#include <linux/regmap.h>
#include <linux/delay.h>

/* Stub clocks id */
#define AP806_CPU_CLUSTER0		0
#define AP806_CPU_CLUSTER1		1
#define AP806_CPUS_PER_CLUSTER		2
#define APN806_CPU1_MASK		0x1

#define APN806_CLUSTER_NUM_OFFSET	8
#define APN806_CLUSTER_NUM_MASK		(1 << APN806_CLUSTER_NUM_OFFSET)


#define KHZ_TO_HZ			1000

/* CPU DFS register mapping*/
#define CA72MP2_0_PLL_CR_0_REG_OFFSET(cluster_index)	(0x78 + cluster_index * 0x14)
#define CA72MP2_0_PLL_CR_1_REG_OFFSET(cluster_index)	(0x80 + cluster_index * 0x14)
#define CA72MP2_0_PLL_CR_2_REG_OFFSET(cluster_index)	(0x84 + cluster_index * 0x14)

#define PLL_CR_0_CPU_CLK_DIV_RATIO_MASK		0x3f
#define PLL_CR_0_CPU_CLK_RELOAD_FORCE_OFFSET	24
#define PLL_CR_0_CPU_CLK_RELOAD_RATIO_OFFSET	16

#define to_clk(hw) container_of(hw, struct ap806_clk, hw)

/*
 * struct ap806_clk: CPU cluster clock controller instance
 * @max_cpu_freq: Max CPU frequency - Sample at rest boot configuration
 * @cluster: Cluster clock controller index
 * @clk_name: Cluster clock controller name
 * @parent_name: Cluster clock controller parent name
 * @hw: HW specific structure of Cluster clock controller
 * @pll_cr_base: CA72MP2 Register base (Device Sample at Reset register)
 */
struct ap806_clk {
	int max_cpu_freq;
	int cluster;

	const char *clk_name;
	const char *parent_name;

	struct device *dev;
	struct clk_hw hw;

	struct regmap *pll_cr_base;
};

static struct clk **cluster_clks;
static struct clk_onecell_data clk_data;

static unsigned long ap806_clk_recalc_rate(struct clk_hw *hw, unsigned long parent_rate)
{
	unsigned long rate = 0;
	struct ap806_clk *clk = to_clk(hw);
	int cpu_clkdiv_ratio;

	/* AP806 supports 2 Clusters */
	if (clk->cluster != AP806_CPU_CLUSTER0 && clk->cluster != AP806_CPU_CLUSTER1) {
		dev_err(clk->dev, "%s: un-supported clock cluster id %d\n",
			__func__, clk->cluster);
		return -EINVAL;
	}

	regmap_read(clk->pll_cr_base, CA72MP2_0_PLL_CR_0_REG_OFFSET(clk->cluster), &cpu_clkdiv_ratio);
	rate = clk->max_cpu_freq / (cpu_clkdiv_ratio & PLL_CR_0_CPU_CLK_DIV_RATIO_MASK);
	rate *= KHZ_TO_HZ;	/* convert from kHz to Hz */

	return rate;
}

static int ap806_clk_set_rate(struct clk_hw *hw, unsigned long rate,
			      unsigned long parent_rate)
{
	struct ap806_clk *clk = to_clk(hw);
	unsigned long new_rate = rate / KHZ_TO_HZ;  /* kHz */
	int reg, divider = clk->max_cpu_freq / new_rate;

	/* AP806 supports 2 Clusters */
	if (clk->cluster != AP806_CPU_CLUSTER0 && clk->cluster != AP806_CPU_CLUSTER1) {
		dev_err(clk->dev, "%s: un-supported clock cluster id %d\n",
			__func__, clk->cluster);
		return -EINVAL;
	}

	/* 1. Set CPU divider */
	regmap_write(clk->pll_cr_base, CA72MP2_0_PLL_CR_0_REG_OFFSET(clk->cluster), divider);

	/* 2. Set Reload force */
	regmap_read(clk->pll_cr_base, CA72MP2_0_PLL_CR_1_REG_OFFSET(clk->cluster), &reg);
	reg |= 0x1 << PLL_CR_0_CPU_CLK_RELOAD_FORCE_OFFSET;
	regmap_write(clk->pll_cr_base, CA72MP2_0_PLL_CR_1_REG_OFFSET(clk->cluster), reg);

	/* 3. Set Reload ratio */
	regmap_read(clk->pll_cr_base, CA72MP2_0_PLL_CR_2_REG_OFFSET(clk->cluster), &reg);
	reg |= 0x1 << PLL_CR_0_CPU_CLK_RELOAD_RATIO_OFFSET;
	regmap_write(clk->pll_cr_base, CA72MP2_0_PLL_CR_2_REG_OFFSET(clk->cluster), reg);

	/* 4. Wait for stabilizing CPU Clock */
	ndelay(100);

	/* 5. Clear Reload ratio */
	regmap_read(clk->pll_cr_base, CA72MP2_0_PLL_CR_2_REG_OFFSET(clk->cluster), &reg);
	reg &= ~0x1 << PLL_CR_0_CPU_CLK_RELOAD_RATIO_OFFSET;
	regmap_write(clk->pll_cr_base, CA72MP2_0_PLL_CR_2_REG_OFFSET(clk->cluster), reg);

	return 0;
}

static long ap806_clk_round_rate(struct clk_hw *hw, unsigned long rate,
				 unsigned long *parent_rate)
{
	struct ap806_clk *clk = to_clk(hw);
	unsigned long new_rate = rate / KHZ_TO_HZ;  /* kHz */
	int divider = clk->max_cpu_freq / new_rate;

	/* AP806 supports 2 Clusters */
	if (clk->cluster != AP806_CPU_CLUSTER0 && clk->cluster != AP806_CPU_CLUSTER1) {
		dev_err(clk->dev, "%s: un-supported clock cluster id %d\n",
			__func__, clk->cluster);
		return -EINVAL;
	}

	new_rate = (clk->max_cpu_freq / divider) * KHZ_TO_HZ; /* convert kHz to Hz */
	return new_rate;
}

static const struct clk_ops ap806_clk_ops = {
	.recalc_rate	= ap806_clk_recalc_rate,
	.round_rate	= ap806_clk_round_rate,
	.set_rate	= ap806_clk_set_rate,
};

static int ap806_clk_probe(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	struct ap806_clk *ap806_clk;
	struct clk *max_cpu_clk;
	struct device_node *dn, *np = pdev->dev.of_node;
	int ret, nclusters = 0, ncpus = 0, cluster_index = 0;
	struct regmap *reg;

	/* set initial CPU frequency as maximum possible frequency */
	max_cpu_clk = devm_clk_get(&pdev->dev, NULL);
	if (IS_ERR(max_cpu_clk)) {
		dev_err(dev, "error getting max cpu frequency\n");
		return PTR_ERR(max_cpu_clk);
	}

	reg = syscon_node_to_regmap(np);
	if (IS_ERR(reg)) {
		pr_err("cannot get pll_cr_base regmap\n");
		return PTR_ERR(reg);
	}

	for_each_node_by_type(dn, "cpu")
		ncpus++;

	/* DFS for AP806 is controlled per cluster (2 CPUs per cluster),
	 * so allocate structs per cluster
	 */
	nclusters = ncpus / AP806_CPUS_PER_CLUSTER;

	ap806_clk = devm_kzalloc(dev, nclusters * sizeof(*ap806_clk), GFP_KERNEL);
	if (WARN_ON(!ap806_clk))
		return -ENOMEM;

	cluster_clks = devm_kzalloc(dev, nclusters * sizeof(*cluster_clks), GFP_KERNEL);
	if (WARN_ON(!cluster_clks))
		return -ENOMEM;

	for_each_node_by_type(dn, "cpu") {
		struct clk_init_data init;
		struct clk *clk;
		char *clk_name = devm_kzalloc(dev, 5, GFP_KERNEL);
		int cpu, err;

		if (WARN_ON(!clk_name))
			return -ENOMEM;

		err = of_property_read_u32(dn, "reg", &cpu);
		if (WARN_ON(err))
			return err;

		/* initialize only for 1st CPU of each cluster (CPU0, CPU2, ..) */
		if (cpu & APN806_CPU1_MASK)
			continue;

		cluster_index = (cpu & APN806_CLUSTER_NUM_MASK) >> APN806_CLUSTER_NUM_OFFSET;
		sprintf(clk_name, "cluster%d", cluster_index);
		ap806_clk[cluster_index].parent_name = of_clk_get_parent_name(np, 0);
		ap806_clk[cluster_index].clk_name = clk_name;
		ap806_clk[cluster_index].cluster = cluster_index;
		ap806_clk[cluster_index].pll_cr_base = reg;
		ap806_clk[cluster_index].max_cpu_freq = clk_get_rate(max_cpu_clk) / 1000; /* Mhz */
		ap806_clk[cluster_index].hw.init = &init;
		ap806_clk[cluster_index].dev = dev;
		init.name = ap806_clk[cluster_index].clk_name;
		init.ops = &ap806_clk_ops;
		init.num_parents = 0;
		init.flags = CLK_GET_RATE_NOCACHE | CLK_IS_ROOT;

		clk = devm_clk_register(dev, &ap806_clk[cluster_index].hw);
		if (IS_ERR(clk))
			return PTR_ERR(clk);

		cluster_clks[cluster_index] = clk;
	}

	clk_data.clk_num = cluster_index + 1;
	clk_data.clks = cluster_clks;

	ret = of_clk_add_provider(np, of_clk_src_onecell_get, &clk_data);
	if (ret) {
		dev_err(dev, "failed to register OF clock provider\n");
		return ret;
	}

	return 0;
}

static const struct of_device_id ap806_clk_of_match[] = {
	{ .compatible = "marvell,ap806-cpu-clk", },
	{}
};

static struct platform_driver ap806_clk_driver = {
	.driver	= {
		.name = "ap806-clk",
		.of_match_table = ap806_clk_of_match,
	},
	.probe = ap806_clk_probe,
};

static int __init ap806_clk_init(void)
{
	return platform_driver_register(&ap806_clk_driver);
}
subsys_initcall(ap806_clk_init);

