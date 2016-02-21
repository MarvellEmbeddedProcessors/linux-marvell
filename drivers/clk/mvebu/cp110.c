/***************************************************************************
* Copyright (C) 2015 Marvell International Ltd.
* ***************************************************************************
* This program is free software: you can redistribute it and/or modify it
* under the terms of the GNU General Public License as published by the Free
* Software Foundation, either version 2 of the License, or any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program.  If not, see <http://www.gnu.org/licenses/>.
* ***************************************************************************
*/

#include <linux/kernel.h>
#include <linux/clk-provider.h>
#include <linux/io.h>
#include <linux/of.h>
#include <linux/of_address.h>
#include <linux/slab.h>
#include "common.h"

#define CPN110_APLL_CLK_FREQ	(1000000000)

/* NAND clock frequency control bit (250 / 400) */
#define NF_CLOCK_SEL_400_MASK       (0x1)

enum cp110_clock_id {
	CP110_APLL_CLK = 0,
	CP110_PPV2_CLK = 1,
	CP110_EIP_CLK  = 2,
	CP110_CORE_CLK = 3,
	CP110_NAND_CLK = 4,
	CP110_CLK_CNT
};

static struct clk *cp110_core_clks[CP110_CLK_CNT];

static struct clk_onecell_data cp110_core_clk_data = {
	.clks = cp110_core_clks,
	.clk_num = CP110_CLK_CNT,
};

static void __init cp110_clk_init(struct device_node *np)
{
	void __iomem *base;
	u32 nand_clk_ctrl;

	base = of_iomap(np, 0);
	if (WARN_ON(!base))
		return;

	nand_clk_ctrl = readl(base);
	iounmap(base);

	/* Register the APLL which is the root of the clk tree */
	cp110_core_clks[CP110_APLL_CLK] = clk_register_fixed_rate(NULL, "apll-clk",
			NULL, CLK_IS_ROOT, CPN110_APLL_CLK_FREQ);

	/* PPv2 is APLL/3 */
	cp110_core_clks[CP110_PPV2_CLK] = clk_register_fixed_factor(NULL, "ppv2-clk",
			"apll-clk", 0, 1, 3);

	/* EIP clock is APLL/2 */
	cp110_core_clks[CP110_EIP_CLK] = clk_register_fixed_factor(NULL, "eip-clk",
			"apll-clk", 0, 1, 2);

	/* Core clock is EIP/2 */
	cp110_core_clks[CP110_CORE_CLK] = clk_register_fixed_factor(NULL, "core-clk",
			"eip-clk", 0, 1, 2);

	/* NAND can be either APLL/2.5 or core clock */
	if (nand_clk_ctrl & NF_CLOCK_SEL_400_MASK)
		cp110_core_clks[CP110_NAND_CLK] = clk_register_fixed_factor(NULL, "nand-clk",
				"apll-clk", 0, 2, 5);
	else
		cp110_core_clks[CP110_NAND_CLK] = clk_register_fixed_factor(NULL, "nand-clk",
				"core-clk", 0, 1, 1);

	of_clk_add_provider(np, of_clk_src_onecell_get,
			    &cp110_core_clk_data);

}

CLK_OF_DECLARE(cp110_clk, "marvell,armada-cp110-clock", cp110_clk_init);

/*
 * Clock Gating Control
 */
static const struct clk_gating_soc_desc armada_cp110_gating_desc[] __initconst = {

	{"audio", NULL, 0},
	{"communit", NULL, 1},
	{"nand", "nand-clk", 2},
	{"pp", "ppv2-clk", 3},
	{"sd", NULL, 4},
	{"mg", NULL, 5},
	{"mg_core", NULL, 6},
	{"xor1", NULL, 7},
	{"xor0", NULL, 8},
	{"gop", NULL, 9},
	{"pcie_x1_0", NULL, 11},
	{"pcie_x1_1", NULL, 12},
	{"pcie_x4", NULL, 13},
	{"pcie_core", NULL, 14},
	{"sata", NULL, 15},
	{"sata_usb_core", NULL, 16},
	{"sdmmc_core", NULL, 18},
	{"usb3h0", NULL, 22},
	{"usb3h1", NULL, 23},
	{"usb3d", NULL, 24},
	{"eip150", "eip-clk", 25},
	{"eip197", "eip-clk", 26},
	{ }
};

static void __init armada_cp110_clk_gating_init(struct device_node *np)
{
	mvebu_clk_gating_setup(np, armada_cp110_gating_desc);
}
CLK_OF_DECLARE(armada_cp110_clk_gating, "marvell,armada-cp110-gating-clock",
	       armada_cp110_clk_gating_init);
