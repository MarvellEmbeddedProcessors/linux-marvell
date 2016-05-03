/*
 * Marvell MSYS SoC clocks
 *
 * Copyright (C) 2012 Marvell
 *
 * Gregory CLEMENT <gregory.clement@free-electrons.com>
 * Sebastian Hesselbarth <sebastian.hesselbarth@gmail.com>
 * Andrew Lunn <andrew@lunn.ch>
 *
 * This file is licensed under the terms of the GNU General Public
 * License version 2.  This program is licensed "as is" without any
 * warranty of any kind, whether express or implied.
 */

#include <linux/kernel.h>
#include <linux/clk-provider.h>
#include <linux/io.h>
#include <linux/of.h>
#include "common.h"


/* For Msys AlleyCat3 Sample At Reset the CPU, DDR and Main PLL clocks are all
 * defined in the same time
 *
 * SAR1[20:18]   : CPU frequency    DDR frequency   MPLL frequency
 *		 1  =  667 MHz	    667 MHz	    2000 MHz
 *		 2  =  400 MHz	    400 MHz	    400 MHz
 *		 3  =  800 MHz	    800 MHz	    800 MHz
 *		 5  =  800 MHz	    400 MHz	    800 MHz
 *		 others reserved.
 */

#define	    SAR1_MSYS_CPU_DDR_MPLL_FREQ_OPT		18
#define	    SAR1_MSYS_CPU_DDR_MPLL_FREQ_OPT_MASK	0x7

static u32 __init msys_get_tclk_freq(void __iomem *sar)
{
	/* Tclk = 200MHz, no SaR dependency */
	return 200000000;
}

static const u32 msys_ac3_cpu_frequencies[] __initconst = {
	0,
	667000000,
	400000000,
	800000000,
	0,
	800000000,
	0, 0,
};

static u32 __init msys_get_cpu_freq(void __iomem *sar)
{
	u32 cpu_freq = 0;
	u8 cpu_freq_select = 0;

	cpu_freq_select = ((readl(sar) >> SAR1_MSYS_CPU_DDR_MPLL_FREQ_OPT) &
			   SAR1_MSYS_CPU_DDR_MPLL_FREQ_OPT_MASK);

	if (of_machine_is_compatible("marvell,msys-ac3"))
		cpu_freq = msys_ac3_cpu_frequencies[cpu_freq_select];

	if (!cpu_freq)
		pr_err("CPU freq select unsupported %d\n", cpu_freq_select);

	return cpu_freq;
}

enum {MSYS_CPU_TO_DDR, MSYS_CPU_TO_MPLL};

static const struct coreclk_ratio msys_core_ratios[] __initconst = {
	{ .id = MSYS_CPU_TO_DDR, .name = "ddrclk" },
	{ .id = MSYS_CPU_TO_MPLL, .name = "mpll" },
};

static const int __initconst msys_ac3_cpu_mpll_ratios[8][2] = {
	{0, 1}, {3, 1}, {1, 1}, {1, 1},
	{0, 1}, {1, 1}, {0, 1}, {0, 1},
};

static const int __initconst msys_ac3_cpu_ddr_ratios[8][2] = {
	{0, 1}, {1, 1}, {1, 1}, {1, 1},
	{0, 1}, {1, 2}, {0, 1}, {0, 1},
};

static void __init msys_get_clk_ratio(
	void __iomem *sar, int id, int *mult, int *div)
{
	u32 opt = ((readl(sar) >> SAR1_MSYS_CPU_DDR_MPLL_FREQ_OPT) &
		SAR1_MSYS_CPU_DDR_MPLL_FREQ_OPT_MASK);

	switch (id) {
	case MSYS_CPU_TO_DDR:
		if (of_machine_is_compatible("marvell,msys-ac3")) {
			*mult = msys_ac3_cpu_ddr_ratios[opt][0];
			*div = msys_ac3_cpu_ddr_ratios[opt][1];
		}
		break;
	case MSYS_CPU_TO_MPLL:
		if (of_machine_is_compatible("marvell,msys-ac3")) {
			*mult = msys_ac3_cpu_mpll_ratios[opt][0];
			*div = msys_ac3_cpu_mpll_ratios[opt][1];
		}
		break;
	}
}

static const struct coreclk_soc_desc msys_core_clocks = {
	.get_tclk_freq = msys_get_tclk_freq,
	.get_cpu_freq = msys_get_cpu_freq,
	.get_clk_ratio = msys_get_clk_ratio,
	.ratios = msys_core_ratios,
	.num_ratios = ARRAY_SIZE(msys_core_ratios),
};


/*
 * Clock Gating Control
 */

static const struct clk_gating_soc_desc msys_gating_desc[] __initconst = {
	{ "ge1", NULL, 3, 0 },
	{ "ge0", NULL, 4, 0 },
	{ "pex00", NULL, 5, 0 },
	{ "sdio", NULL, 17, 0 },
	{ "usb0", NULL, 18, 0 },
	{ "xor0", NULL, 22, 0 },
	{ }
};

static void __init msys_clk_init(struct device_node *np)
{
	struct device_node *cgnp =
		of_find_compatible_node(NULL, NULL, "marvell,msys-gating-clock");

	mvebu_coreclk_setup(np, &msys_core_clocks);

	if (cgnp)
		mvebu_clk_gating_setup(cgnp, msys_gating_desc);
}
CLK_OF_DECLARE(msys_clk, "marvell,msys-core-clock", msys_clk_init);

