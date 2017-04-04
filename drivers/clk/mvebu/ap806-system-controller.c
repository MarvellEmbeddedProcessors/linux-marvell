/*
 * Marvell Armada AP806 System Controller
 *
 * Copyright (C) 2016 Marvell
 *
 * Thomas Petazzoni <thomas.petazzoni@free-electrons.com>
 *
 * This file is licensed under the terms of the GNU General Public
 * License version 2.  This program is licensed "as is" without any
 * warranty of any kind, whether express or implied.
 */

#define pr_fmt(fmt) "ap806-system-controller: " fmt

#include <linux/kernel.h>
#include <linux/clk.h>
#include <linux/clk-provider.h>
#include <linux/io.h>
#include <linux/mfd/syscon.h>
#include <linux/of.h>
#include <linux/of_address.h>
#include <linux/regmap.h>

#define AP806_SAR_REG			0x400
#define AP806_SAR_CLKFREQ_MODE_MASK	0x1f

#define AP806_CLK_NUM			8

static struct clk *ap806_clks[AP806_CLK_NUM];

static struct clk_onecell_data ap806_clk_data = {
	.clks = ap806_clks,
	.clk_num = AP806_CLK_NUM,
};

static void __init ap806_syscon_clk_init(struct device_node *np)
{
	unsigned int freq_mode, cpuclk_freq, dclk_freq, ringclk_freq;
	const char *name, *fixedclk_name, *ringclk_name;
	struct regmap *regmap;
	u32 reg;

	regmap = syscon_node_to_regmap(np);
	if (IS_ERR(regmap)) {
		pr_err("cannot get regmap\n");
		return;
	}

	if (regmap_read(regmap, AP806_SAR_REG, &reg)) {
		pr_err("cannot read from regmap\n");
		return;
	}

	freq_mode = reg & AP806_SAR_CLKFREQ_MODE_MASK;
	switch (freq_mode) {
	case 0x0 ... 0x1:
		cpuclk_freq = 2000;
		break;
	case 0x6 ... 0x7:
		cpuclk_freq = 1800;
		break;
	case 0x4:
	case 0xD:
		cpuclk_freq = 1600;
		break;
	case 0x1A:
		cpuclk_freq = 1400;
		break;
	case 0x14:
	case 0x17:
		cpuclk_freq = 1300;
		break;
	case 0x19:
		cpuclk_freq = 1200;
		break;
	case 0x1D:
	case 0x13:
		cpuclk_freq = 1000;
		break;
	case 0x1C:
		cpuclk_freq = 800;
		break;
	case 0x1B:
		cpuclk_freq = 600;
		break;
	default:
		/* set cpuclk_freq as invalid value to continue and
		** configure the MSS clock (used to calculate the
		** baudrate of the UART
		*/
		cpuclk_freq = 0;
		pr_err("invalid Sample at Reset value\n");
	}

	/* Get DCLK frequency (DCLK = 0.5*DDR_CLK) */
	switch (freq_mode) {
	case 0x0:
	case 0x6:
		dclk_freq = 600;
		break;
	case 0x1:
	case 0x7:
	case 0xD:
		dclk_freq = 525;
		break;
	case 0x13:
	case 0x17:
		dclk_freq = 325;
		break;
	case 0x4:
	case 0x14:
	case 0x19:
	case 0x1A:
	case 0x1B:
	case 0x1C:
	case 0x1D:
		dclk_freq = 400;
		break;
	default:
		dclk_freq = 0;
		pr_err("invalid Sample at Reset value\n");
	}

	/* Convert to hertz */
	cpuclk_freq *= 1000 * 1000;
	dclk_freq *= 1000 * 1000;

	/* In all SAR values ring_clk is twice ddr clock */
	ringclk_freq = 2 * dclk_freq;

	/* CPU clocks depend on the Sample At Reset configuration */
	of_property_read_string_index(np, "clock-output-names",
				      0, &name);
	ap806_clks[0] = clk_register_fixed_rate(NULL, name, NULL,
						CLK_IS_ROOT, cpuclk_freq);

	of_property_read_string_index(np, "clock-output-names",
				      1, &name);
	ap806_clks[1] = clk_register_fixed_rate(NULL, name, NULL, CLK_IS_ROOT,
						cpuclk_freq);

	/* Fixed clock is always 1200 Mhz */
	of_property_read_string_index(np, "clock-output-names",
				      2, &fixedclk_name);
	ap806_clks[2] = clk_register_fixed_rate(NULL, fixedclk_name, NULL, CLK_IS_ROOT,
						1200 * 1000 * 1000);

	/* MSS Clock is fixed clock divided by 6 */
	of_property_read_string_index(np, "clock-output-names",
				      3, &name);
	ap806_clks[3] = clk_register_fixed_factor(NULL, name, fixedclk_name,
						  0, 1, 6);

	/* eMMC Clock is fixed clock divided by 3 */
	of_property_read_string_index(np, "clock-output-names",
				      4, &name);
	ap806_clks[4] = clk_register_fixed_factor(NULL, name, fixedclk_name,
						  0, 1, 3);

	of_property_read_string_index(np, "clock-output-names",
				      5, &name);
	ap806_clks[5] = clk_register_fixed_rate(NULL, name, NULL, CLK_IS_ROOT,
						dclk_freq);

	of_property_read_string_index(np, "clock-output-names",
				      6, &ringclk_name);
	ap806_clks[6] = clk_register_fixed_rate(NULL, ringclk_name, NULL, CLK_IS_ROOT,
						ringclk_freq);

	/* External ring is 1/2 of ring clk */
	of_property_read_string_index(np, "clock-output-names",
				      7, &name);
	ap806_clks[7] = clk_register_fixed_factor(NULL, name, ringclk_name,
						  0, 1, 2);

	of_clk_add_provider(np, of_clk_src_onecell_get, &ap806_clk_data);
}

CLK_OF_DECLARE(ap806_syscon_clk, "marvell,ap806-system-controller",
	       ap806_syscon_clk_init);
