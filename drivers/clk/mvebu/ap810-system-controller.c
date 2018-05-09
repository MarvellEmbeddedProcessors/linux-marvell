/*
 * Marvell Armada AP810 System Controller
 *
 * Copyright (C) 2018 Marvell
 *
 * Thomas Petazzoni <thomas.petazzoni@free-electrons.com>
 * Konstantin Porotchkin <kostap@marvell.com>
 *
 * This file is licensed under the terms of the GNU General Public
 * License version 2.  This program is licensed "as is" without any
 * warranty of any kind, whether express or implied.
 */

#define pr_fmt(fmt) "ap810-system-controller: " fmt

#include <linux/clk-provider.h>
#include <linux/mfd/syscon.h>
#include <linux/init.h>
#include <linux/of.h>
#include <linux/of_address.h>
#include <linux/platform_device.h>
#include <linux/regmap.h>

/* Clock frequency mode is not shown correctly in SAR status
 * register of AP810-A0 (offset is 0x400, or 0x6f4400)
 * Use scratchpad register at offset 0x6f43e4 filled by ATF
 * for obtaining the real frequency mode
 */
#define AP810_SARSR_REG			0x400
#define AP810_A0_FREQ_MODE_REG		0x3e4
#define AP810_CLOCK_FREQ_MODE_OFFS	0
#define AP810_CLOCK_FREQ_MODE_MASK	(0x7 << AP810_CLOCK_FREQ_MODE_OFFS)

#define AP810_EFUSE_STATUS_REG		0x410
#define AP810_HIGHV_MODE_OFFS		24
#define AP810_HIGHV_MODE_MASK		(1 << AP810_HIGHV_MODE_OFFS)

#define AP810_SAR_CLK_OPT_NUM		8
#define AP810_CLK_NUM			8

#define AP810_CPU_CLK_IDX		0
#define AP810_DDR_CLK_IDX		1
#define AP810_FAB_CLK_IDX		2
#define AP810_EIP197_CLK_IDX		3
#define AP810_MAX_CLK_IDX		4

#define MHZ				(1000 * 1000)

static unsigned int
	ap810_hp_clk_tbl[AP810_SAR_CLK_OPT_NUM][AP810_MAX_CLK_IDX] = {
		{ 1600,	1600, 1200, 1200 },	/* 0 */
		{ 2000,	2400, 1200, 1200 },	/* 1 */
		{ 2000,	2667, 1400, 1200 },	/* 2 */
		{ 2200, 2400, 1400, 1200 },	/* 3 */
		{ 2200, 2667, 1400, 1200 },	/* 4 */
		{ 2500, 2667, 1400, 1200 },	/* 5 */
		{ 2500, 2933, 1400, 1200 },	/* 6 */
		{ 2700, 3200, 1400, 1200 }	/* 7 */
	};

static unsigned int
	ap810_lp_clk_tbl[AP810_SAR_CLK_OPT_NUM][AP810_MAX_CLK_IDX] = {
		{ 1200, 1600, 800,  1000 },	/* 0 */
		{ 1600, 2400, 800,  1000 },	/* 1 */
		{ 1600, 2667, 1200, 1000 },	/* 2 */
		{ 1800, 2400, 1200, 1000 },	/* 3 */
		{ 1800, 2667, 1300, 1000 },	/* 4 */
		{ 2000, 2400, 1200, 1000 },	/* 5 */
		{ 2000, 2667, 1300, 1000 },	/* 6 */
		{ 2000, 2667, 1300, 1000 }	/* 7 */
	};

static struct clk *ap810_clks[AP810_CLK_NUM];

static struct clk_onecell_data ap810_clk_data = {
	.clks = ap810_clks,
	.clk_num = AP810_CLK_NUM,
};

static char *ap810_unique_name(struct device *dev, struct device_node *np,
			       char *name)
{
	const __be32 *reg;
	u64 addr;

	reg = of_get_property(np, "reg", NULL);
	addr = of_translate_address(np, reg);
	return devm_kasprintf(dev, GFP_KERNEL, "%llx-%s",
			(unsigned long long)addr, name);
}

static int ap810_syscon_common_probe(struct platform_device *pdev,
				     struct device_node *syscon_node)
{
	unsigned int freq_mode, cpuclk_freq;
	unsigned int ringclk_freq, eipclk_freq;
	const char *name, *fixedclk_name, *ringclk_name;
	bool is_hp;
	struct device *dev = &pdev->dev;
	struct device_node *np = dev->of_node;
	struct regmap *regmap;
	u32 reg, ap810_revision;
	int ret;

	regmap = syscon_node_to_regmap(syscon_node);
	if (IS_ERR(regmap)) {
		dev_err(dev, "cannot get regmap\n");
		return PTR_ERR(regmap);
	}

	/* TODO - call the SoC info driver for reading the AP810 revision ID */
	ap810_revision = 0;

	/* Get the frequuency mode */
	if (ap810_revision == 0)
		ret = regmap_read(regmap, AP810_A0_FREQ_MODE_REG, &reg);
	else
		ret = regmap_read(regmap, AP810_SARSR_REG, &reg);

	if (ret) {
		dev_err(dev, "cannot read from regmap\n");
		return ret;
	}

	freq_mode = reg & AP810_CLOCK_FREQ_MODE_MASK;
	freq_mode >>= AP810_CLOCK_FREQ_MODE_OFFS;


	/* Get the Low/High SoC power variant */
	ret = regmap_read(regmap, AP810_EFUSE_STATUS_REG, &reg);
	if (ret) {
		dev_err(dev, "cannot read from regmap\n");
		return ret;
	}

	is_hp = !!(reg & AP810_HIGHV_MODE_MASK);

	/* Obtain MHz values from HW clocks tables */
	if (is_hp) {
		cpuclk_freq = ap810_hp_clk_tbl[freq_mode][AP810_CPU_CLK_IDX];
		ringclk_freq = ap810_hp_clk_tbl[freq_mode][AP810_FAB_CLK_IDX];
		eipclk_freq = ap810_hp_clk_tbl[freq_mode][AP810_EIP197_CLK_IDX];
	} else {
		cpuclk_freq = ap810_lp_clk_tbl[freq_mode][AP810_CPU_CLK_IDX];
		ringclk_freq = ap810_lp_clk_tbl[freq_mode][AP810_FAB_CLK_IDX];
		eipclk_freq = ap810_lp_clk_tbl[freq_mode][AP810_EIP197_CLK_IDX];
	}

	/* continue in case of a bad SAR value and configure
	 *  the MSS clock (used to calculate the baudrate of the UART)
	 */
	if (cpuclk_freq == 0)
		pr_err("invalid frequency mode %d in Sample at Reset\n",
		       freq_mode);

	/* Convert all clocks to hertz (DCLK = 0.5*DDR_CLK) */
	eipclk_freq *= MHZ;
	cpuclk_freq *= MHZ;
	ringclk_freq *= MHZ;

	/* CPU clocks depend on the Sample At Reset configuration */
	name = ap810_unique_name(dev, syscon_node, "cpu-cluster-0-1");
	ap810_clks[0] = clk_register_fixed_rate(dev, name, NULL,
						0, cpuclk_freq);
	if (IS_ERR(ap810_clks[0])) {
		ret = PTR_ERR(ap810_clks[0]);
		goto fail0;
	}

	name = ap810_unique_name(dev, syscon_node, "cpu-cluster-2-3");
	ap810_clks[1] = clk_register_fixed_rate(dev, name, NULL, 0,
						cpuclk_freq);
	if (IS_ERR(ap810_clks[1])) {
		ret = PTR_ERR(ap810_clks[1]);
		goto fail1;
	}

	/* Ring clocks are derived from Sample At Reset configuration */
	ringclk_name = ap810_unique_name(dev, syscon_node, "ring");
	ap810_clks[2] = clk_register_fixed_rate(dev, ringclk_name, NULL, 0,
						ringclk_freq);
	if (IS_ERR(ap810_clks[2])) {
		ret = PTR_ERR(ap810_clks[2]);
		goto fail2;
	}

	/* DMA Clock is ring clock divided by 2 */
	name = ap810_unique_name(dev, syscon_node, "rdma");
	ap810_clks[3] = clk_register_fixed_factor(NULL, name,
						  ringclk_name,
						  0, 1, 2);
	if (IS_ERR(ap810_clks[3])) {
		ret = PTR_ERR(ap810_clks[3]);
		goto fail3;
	}

	/* EIP197 clocks are derived from Sample At Reset configuration */
	name = ap810_unique_name(dev, syscon_node, "eip197");
	ap810_clks[4] = clk_register_fixed_rate(dev, name, NULL, 0,
						eipclk_freq);
	if (IS_ERR(ap810_clks[4])) {
		ret = PTR_ERR(ap810_clks[4]);
		goto fail4;
	}

	/* Fixed clock is always 1200 Mhz */
	fixedclk_name = ap810_unique_name(dev, syscon_node, "fixed");
	ap810_clks[5] = clk_register_fixed_rate(dev, fixedclk_name, NULL,
						0, 1200 * 1000 * 1000);
	if (IS_ERR(ap810_clks[5])) {
		ret = PTR_ERR(ap810_clks[5]);
		goto fail5;
	}

	/* MSS Clock is fixed clock divided by 6 */
	name = ap810_unique_name(dev, syscon_node, "mss");
	ap810_clks[6] = clk_register_fixed_factor(NULL, name,
						  fixedclk_name,
						  0, 1, 6);
	if (IS_ERR(ap810_clks[6])) {
		ret = PTR_ERR(ap810_clks[6]);
		goto fail6;
	}

	/* SDIO(/eMMC) Clock is fixed clock divided by 3 */
	name = ap810_unique_name(dev, syscon_node, "sdio");
	ap810_clks[7] = clk_register_fixed_factor(NULL, name,
						  fixedclk_name,
						  0, 1, 3);
	if (IS_ERR(ap810_clks[7])) {
		ret = PTR_ERR(ap810_clks[7]);
		goto fail7;
	}

	of_clk_add_provider(np, of_clk_src_onecell_get, &ap810_clk_data);
	ret = of_clk_add_provider(np, of_clk_src_onecell_get, &ap810_clk_data);
	if (ret)
		goto fail_clk_add;

	return 0;

fail_clk_add:
	clk_unregister_fixed_factor(ap810_clks[7]);	/* sdio */
fail7:
	clk_unregister_fixed_factor(ap810_clks[6]);	/* mss */
fail6:
	clk_unregister_fixed_rate(ap810_clks[5]);	/* fixed */
fail5:
	clk_unregister_fixed_rate(ap810_clks[4]);	/* eip197 */
fail4:
	clk_unregister_fixed_factor(ap810_clks[3]);	/* rdma */
fail3:
	clk_unregister_fixed_rate(ap810_clks[2]);	/* ring */
fail2:
	clk_unregister_fixed_rate(ap810_clks[1]);	/* cpu-cluster-2-3 */
fail1:
	clk_unregister_fixed_rate(ap810_clks[0]);	/* cpu-cluster-0-1 */
fail0:
	return ret;
}

static int ap810_syscon_legacy_probe(struct platform_device *pdev)
{
	dev_warn(&pdev->dev, FW_WARN "Using legacy device tree binding\n");
	dev_warn(&pdev->dev, FW_WARN "Update your device tree:\n");
	dev_warn(&pdev->dev, FW_WARN
		 "This binding won't be supported in future kernel\n");

	return ap810_syscon_common_probe(pdev, pdev->dev.of_node);

}

static int ap810_clock_probe(struct platform_device *pdev)
{
	return ap810_syscon_common_probe(pdev, pdev->dev.of_node->parent);
}

static const struct of_device_id ap810_syscon_legacy_of_match[] = {
	{ .compatible = "marvell,ap810-system-controller", },
	{ }
};

static struct platform_driver ap810_syscon_legacy_driver = {
	.probe = ap810_syscon_legacy_probe,
	.driver		= {
		.name	= "marvell-ap810-system-controller",
		.of_match_table = ap810_syscon_legacy_of_match,
		.suppress_bind_attrs = true,
	},
};
builtin_platform_driver(ap810_syscon_legacy_driver);

static const struct of_device_id ap810_clock_of_match[] = {
	{ .compatible = "marvell,ap810-clock", },
	{ }
};

static struct platform_driver ap810_clock_driver = {
	.probe = ap810_clock_probe,
	.driver		= {
		.name	= "marvell-ap810-clock",
		.of_match_table = ap810_clock_of_match,
		.suppress_bind_attrs = true,
	},
};
builtin_platform_driver(ap810_clock_driver);
