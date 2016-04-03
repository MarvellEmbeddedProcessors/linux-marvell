/*
* ***************************************************************************
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
#include <linux/clk.h>
#include <linux/clkdev.h>
#include <linux/clk-provider.h>
#include <linux/io.h>
#include <linux/of.h>
#include <linux/of_address.h>
#include <linux/syscore_ops.h>

#include "common.h"
#include "armada-3700.h"

/*
 * Core Clock
 */

static void __iomem *northbase; /* kernel address of MVEBU_NORTH_CLOCK_REGS_BASE */
#define clkr32(reg)	readl(northbase + (reg))

const char *tbg_clk_name[MVEBU_A3700_TBG_CLK_NUM] = {"tbg_a_p", "tbg_b_p", "tbg_a_s", "tbg_b_s"};

/***************************************************************************************************
  * get_ref_clk
  *
  * return: reference clock in MHz (25 or 40)
 ***************************************************************************************************/
static u32 get_ref_clk(void)
{
	u32 regval;

	regval = (clkr32(MVEBU_TEST_PIN_LATCH_N) & MVEBU_XTAL_MODE_MASK) >> MVEBU_XTAL_MODE_OFFS;

	if (regval == MVEBU_XTAL_CLOCK_25MHZ)
		return 25;
	else
		return 40;
}

/***************************************************************************************************
  * get_tbg_clk
  *
  * return: reference clock in Hz
 ***************************************************************************************************/
static u32 get_tbg_clk(enum a3700_clock_line tbg_typ)
{
	u32 tbg_M, tbg_N, vco_div;
	u32 ref, reg_val;

	/* get ref clock */
	ref = get_ref_clk();

	/* get M, N */
	reg_val = clkr32(MVEBU_NORTH_BRG_TBG_CTRL7);
	tbg_M = ((tbg_typ == TBG_A_S) || (tbg_typ == TBG_A_P)) ?
		((reg_val  >> MVEBU_TBG_A_REFDIV_OFFSET) & MVEBU_TBG_DIV_MASK) :
		((reg_val >> MVEBU_TBG_B_REFDIV_OFFSET) & MVEBU_TBG_DIV_MASK);
	tbg_M = (tbg_M == 0) ? 1 : tbg_M;

	reg_val = clkr32(MVEBU_NORTH_BRG_TBG_CTRL0);
	tbg_N = ((tbg_typ == TBG_A_S) || (tbg_typ == TBG_A_P)) ?
		((reg_val >> MVEBU_TBG_A_FBDIV_OFFSET) & MVEBU_TBG_DIV_MASK) :
		((reg_val >> MVEBU_TBG_B_FBDIV_OFFSET) & MVEBU_TBG_DIV_MASK);

	if ((tbg_typ == TBG_A_S) || (tbg_typ == TBG_B_S)) {
		/* get SE VCODIV */
		reg_val = clkr32(MVEBU_NORTH_BRG_TBG_CTRL1);
		reg_val = (tbg_typ == TBG_A_S) ?
			  ((reg_val >> MVEBU_TBG_A_VCODIV_SE_OFFSET) & MVEBU_TBG_DIV_MASK) :
			  ((reg_val >> MVEBU_TBG_B_VCODIV_SE_OFFSET) & MVEBU_TBG_DIV_MASK);
	} else {
		/* get DIFF VCODIV */
		reg_val = clkr32(MVEBU_NORTH_BRG_TBG_CTRL8);
		reg_val = (tbg_typ == TBG_A_P) ?
			  ((reg_val >> MVEBU_TBG_A_VCODIV_DIFF_OFFSET) & MVEBU_TBG_DIV_MASK) :
			  ((reg_val >> MVEBU_TBG_B_VCODIV_DIFF_OFFSET) & MVEBU_TBG_DIV_MASK);
	}
	if (reg_val > MVEBU_TBG_VCODIV_MAX)
		return 0; /* invalid */

	vco_div = 0x1 << reg_val;

	/* align with the FS */
	return (((tbg_N * ref) << 2)/(tbg_M * vco_div)) * 1000000;
}

enum {
	A3700_TBG_TO_CPU_CLK = 0,
	A3700_TBG_TO_DDR_CLK,
	A3700_TBG_TO_SATA_CLK,
	A3700_TBG_TO_MMC_CLK,
	A3700_TBG_TO_USB_CLK,
	A3700_TBG_TO_GBE0_CLK,
	A3700_TBG_TO_GBE1_CLK,
};

static const struct coreclk_ratio armada_3700_coreclk_ratios[] __initconst = {
	{ .id = A3700_TBG_TO_CPU_CLK, .name = "cpu" },
	{ .id = A3700_TBG_TO_DDR_CLK, .name = "ddr" },
	{ .id = A3700_TBG_TO_SATA_CLK, .name = "sata-host" },
	{ .id = A3700_TBG_TO_MMC_CLK, .name = "mmc" },
	{ .id = A3700_TBG_TO_USB_CLK, .name = "usb32-ss-sys" },
	{ .id = A3700_TBG_TO_GBE0_CLK, .name = "gbe0-core" },
	{ .id = A3700_TBG_TO_GBE1_CLK, .name = "gbe1-core" },
};

/***************************************************************************************************
  * get the clock ratio and TBG module according to the clock id
 ***************************************************************************************************/
static void __init armada_3700_get_clk_ratio(
	void __iomem *base, int id, int *tbg, int *mult, int *div)
{
	int prscl1, prscl2, div2;
	*mult = 1;

	switch (id) {
	case A3700_TBG_TO_CPU_CLK:
		*tbg = (clkr32(MVEBU_NORTH_CLOCK_TBG_SELECT_REG) >> TBG_WCPU_PCLK_SEL_OFFSET) & MVEBU_TBG_CLK_SEL_MASK;
		*div = (clkr32(MVEBU_NORTH_CLOCK_DIVIDER_SELECT0_REG) >> WCPU_CLK_DIV_PRSCL_OFFSET) &
					MVEBU_TBG_CLK_PRSCL_MASK;
		break;

	case A3700_TBG_TO_DDR_CLK:
		*tbg = TBG_A_S;/* DDR always use TBG_A_S */
		*div = 2;
		break;

	case A3700_TBG_TO_SATA_CLK:
		*tbg = (clkr32(MVEBU_NORTH_CLOCK_TBG_SELECT_REG) >> TBG_SATA_HOST_PCLK_SEL_OFFSET) &
					MVEBU_TBG_CLK_SEL_MASK;
		prscl1 = (clkr32(MVEBU_NORTH_CLOCK_DIVIDER_SELECT2_REG) >> SATA_HOST_CLK_PRSCL1_OFFSET) &
					MVEBU_TBG_CLK_PRSCL_MASK;
		prscl2 = (clkr32(MVEBU_NORTH_CLOCK_DIVIDER_SELECT2_REG) >> SATA_HOST_CLK_PRSCL2_OFFSET) &
					MVEBU_TBG_CLK_PRSCL_MASK;
		*div = prscl1 * prscl2;
		break;

	case A3700_TBG_TO_MMC_CLK:
		*tbg = (clkr32(MVEBU_NORTH_CLOCK_TBG_SELECT_REG) >> TBG_MMC_PCLK_SEL_OFFSET) &
					MVEBU_TBG_CLK_SEL_MASK;
		prscl1 = (clkr32(MVEBU_NORTH_CLOCK_DIVIDER_SELECT2_REG) >> MMC_CLK_PRSCL1_OFFSET) &
					MVEBU_TBG_CLK_PRSCL_MASK;
		prscl2 = (clkr32(MVEBU_NORTH_CLOCK_DIVIDER_SELECT2_REG) >> MMC_CLK_PRSCL2_OFFSET) &
					MVEBU_TBG_CLK_PRSCL_MASK;
		*div = prscl1 * prscl2;
		break;

	case A3700_TBG_TO_USB_CLK:
		*tbg = (clkr32(MVEBU_SOUTH_CLOCK_TBG_SELECT_REG) >> TBG_USB32_SS_CLK_SEL_OFFSET) &
					MVEBU_TBG_CLK_SEL_MASK;
		prscl1 = (clkr32(MVEBU_SOUTH_CLOCK_DIVIDER_SELECT0_REG) >> USB32_SS_SYS_CLK_PRSCL1_OFFSET) &
					MVEBU_TBG_CLK_PRSCL_MASK;
		prscl2 = (clkr32(MVEBU_SOUTH_CLOCK_DIVIDER_SELECT0_REG) >> USB32_SS_SYS_CLK_PRSCL2_OFFSET) &
					MVEBU_TBG_CLK_PRSCL_MASK;
		*div = prscl1 * prscl2;
		break;

	case A3700_TBG_TO_GBE0_CLK:
		*tbg = (clkr32(MVEBU_SOUTH_CLOCK_TBG_SELECT_REG) >> TBG_GBE_CORE_CLK_SEL_OFFSET) &
					MVEBU_TBG_CLK_SEL_MASK;
		prscl1 = (clkr32(MVEBU_SOUTH_CLOCK_DIVIDER_SELECT1_REG) >> GBE_CORE_CLK_PRSCL1_OFFSET) &
					MVEBU_TBG_CLK_PRSCL_MASK;
		prscl2 = (clkr32(MVEBU_SOUTH_CLOCK_DIVIDER_SELECT1_REG) >> GBE_CORE_CLK_PRSCL2_OFFSET) &
					MVEBU_TBG_CLK_PRSCL_MASK;
		div2 = (clkr32(MVEBU_SOUTH_CLOCK_DIVIDER_SELECT1_REG) >> GBE0_CORE_CLK_DIV_OFFSET) &
					MVEBU_TBG_CLK_DIV_MASK;
		*div = (prscl1 * prscl2) << div2;
		break;

	case A3700_TBG_TO_GBE1_CLK:
		*tbg = (clkr32(MVEBU_SOUTH_CLOCK_TBG_SELECT_REG) >> TBG_GBE_CORE_CLK_SEL_OFFSET) &
					MVEBU_TBG_CLK_SEL_MASK;
		prscl1 = (clkr32(MVEBU_SOUTH_CLOCK_DIVIDER_SELECT1_REG) >> GBE_CORE_CLK_PRSCL1_OFFSET) &
					MVEBU_TBG_CLK_PRSCL_MASK;
		prscl2 = (clkr32(MVEBU_SOUTH_CLOCK_DIVIDER_SELECT1_REG) >> GBE_CORE_CLK_PRSCL2_OFFSET) &
					MVEBU_TBG_CLK_PRSCL_MASK;
		div2 = (clkr32(MVEBU_SOUTH_CLOCK_DIVIDER_SELECT1_REG) >> GBE1_CORE_CLK_DIV_OFFSET) &
					MVEBU_TBG_CLK_DIV_MASK;
		*div = (prscl1 * prscl2) << div2;
		break;

	default:
		*div = 0;
		break;
	}
}

static u32 __init armada_3700_get_tbg_freq(void __iomem *base, int tbg_index)
{
	return get_tbg_clk(tbg_index);
}

static const char *__init armada_3700_get_tbg_name(int tbg_index)
{
	return tbg_clk_name[tbg_index];
}

static const struct a3700_clk_desc armada_3700_coreclks = {
	.get_tbg_freq = armada_3700_get_tbg_freq,
	.get_tbg_name = armada_3700_get_tbg_name,
	.get_clk_ratio = armada_3700_get_clk_ratio,
	.ratios = armada_3700_coreclk_ratios,
	.num_ratios = ARRAY_SIZE(armada_3700_coreclk_ratios),
	.num_tbg = MVEBU_A3700_TBG_CLK_NUM,
};

void __init armada3700_coreclk_setup(struct device_node *np,
				const struct a3700_clk_desc *desc)
{
	struct clk_onecell_data *clk_data;
	void __iomem *base;
	unsigned long rate;
	int i;

	clk_data = kmalloc(sizeof(*clk_data), GFP_KERNEL);
	if (!clk_data)
		return;

	base = of_iomap(np, 0);
	if (WARN_ON(!base))
		return;

	northbase = base; /* save it into static variable, only needed once */

	/* Allocate struct for TBG_A(P/S), TBG_B(P/S), and ratio clocks */
	clk_data->clk_num = desc->num_tbg + desc->num_ratios;

	clk_data->clks = kcalloc(clk_data->clk_num, sizeof(struct clk *),
				GFP_KERNEL);
	if (WARN_ON(!clk_data->clks)) {
		iounmap(base);
		return;
	}

	/* Register TBG clocks */
	for (i = 0; i < desc->num_tbg; i++) {
		rate = desc->get_tbg_freq(base, i);
		clk_data->clks[i] = clk_register_fixed_rate(NULL, desc->get_tbg_name(i), NULL,
							   CLK_IS_ROOT, rate);
		WARN_ON(IS_ERR(clk_data->clks[i]));
	}

	/* Register fixed-factor clocks derived from TBG clock */
	for (i = 0; i < desc->num_ratios; i++) {
		const char *rclk_name = desc->ratios[i].name;
		int tbg, mult, div;

		desc->get_clk_ratio(base, desc->ratios[i].id, &tbg, &mult, &div);
		clk_data->clks[desc->num_tbg+i] = clk_register_fixed_factor(NULL, rclk_name,
				       desc->get_tbg_name(tbg), 0, mult, div);
		WARN_ON(IS_ERR(clk_data->clks[desc->num_tbg+i]));
	};

	/* register isn't needed anymore */
	iounmap(base);

	of_clk_add_provider(np, of_clk_src_onecell_get, clk_data);
}


static void __init armada_3700_coreclk_init(struct device_node *np)
{
	armada3700_coreclk_setup(np, &armada_3700_coreclks);
}

CLK_OF_DECLARE(armada_3700_core_clk, "marvell,armada-3700-core-clock",
	       armada_3700_coreclk_init);

/*
 * Clock Gating Control
 */
static const struct clk_gating_soc_desc armada_3700_north_bridge_gating_desc[] __initconst = {
	{ "sata-host-gate", "sata-host", 3, 0, CLK_GATE_SET_TO_DISABLE },
	{ "twsi2", NULL, 16, 0, CLK_GATE_SET_TO_DISABLE },
	{ "twsi1", NULL, 17, 0, CLK_GATE_SET_TO_DISABLE },
	{ }
};

static void __init
armada_3700_north_bridge_clk_gating_init(struct device_node *np)
{
	mvebu_clk_gating_setup(np, armada_3700_north_bridge_gating_desc);
}
CLK_OF_DECLARE(armada_3700_north_bridge_clk_gating,
	       "marvell,armada-3700-north-bridge-gating-clock",
	       armada_3700_north_bridge_clk_gating_init);

static const struct clk_gating_soc_desc armada_3700_south_bridge_gating_desc[] __initconst = {
	{ "gbe1-gate", "gbe1-core", 4, 0, CLK_GATE_SET_TO_DISABLE },
	{ "gbe0-gate", "gbe0-core", 5, 0, CLK_GATE_SET_TO_DISABLE },
	{ "pcie", NULL, 14, 0, CLK_GATE_SET_TO_DISABLE },
	{ "usb32-ss-sys-gate", "usb32-ss-sys", 17, 0, CLK_GATE_SET_TO_DISABLE },
	{ }
};

static void __init
armada_3700_south_bridge_clk_gating_init(struct device_node *np)
{
	mvebu_clk_gating_setup(np, armada_3700_south_bridge_gating_desc);
}
CLK_OF_DECLARE(armada_3700_south_bridge_clk_gating,
	       "marvell,armada-3700-south-bridge-gating-clock",
	       armada_3700_south_bridge_clk_gating_init);
