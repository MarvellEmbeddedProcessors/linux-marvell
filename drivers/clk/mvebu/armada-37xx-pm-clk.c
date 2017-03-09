/*
 * Marvell Armada3700 CPU Clock PM driver
 *
 * Copyright (C) 2016 Marvell
 *
 * Evan Wang <xswang@marvell.com>
 *
 * This file is licensed under the terms of the GNU General Public
 * License version 2.  This program is licensed "as is" without any
 * warranty of any kind, whether express or implied.
 */

#include <linux/clk-provider.h>
#include <linux/clk.h>
#include <linux/err.h>
#include <linux/kernel.h>
#include <linux/of.h>
#include <linux/of_device.h>
#include <linux/delay.h>
#include <linux/completion.h>
#include <linux/err.h>
#include <linux/io.h>
#include <linux/module.h>
#include <linux/of.h>
#include <linux/of_device.h>

/* A3700 clock PM
 * -------------------------------------------
 * | CPU load level |      CPU Freq Divider  |
 * -------------------------------------------
 * |    Level0      |         L0_Div         |
 * -------------------------------------------
 * |    Level1      |         L1_Div         |
 * -------------------------------------------
 * |    Level2      |         L2_Div         |
 * -------------------------------------------
 * |    Level3      |         L3_Div         |
 * -------------------------------------------
 */

/* North bridge PM configuration registers of DVFS related */
#define A3700_PM_NB_CLK_SHIFT		(0x18)
#define A3700_PM_NB_L0_L1_CONFIG_REG	(0x18 - A3700_PM_NB_CLK_SHIFT)
#define A3700_PM_NB_L2_L3_CONFIG_REG	(0x1C - A3700_PM_NB_CLK_SHIFT)
#define  A3700_PM_NB_TBG_DIV_LX_OFF	(13)
#define  A3700_PM_NB_TBG_DIV_LX_MASK	(0x7)
#define  A3700_PM_NB_CLK_SEL_LX_OFF	(11)
#define  A3700_PM_NB_CLK_SEL_LX_MASK	(0x1)
#define  A3700_PM_NB_TBG_SEL_LX_OFF	(9)
#define  A3700_PM_NB_TBG_SEL_LX_MASK	(0x3)
#define  A3700_PM_NB_VDD_SEL_LX_OFF	(6)
#define  A3700_PM_NB_VDD_SEL_LX_MASK	(0x3)
#define  A3700_PM_NB_LX_CONFIG_SHIFT	(16)

#define A3700_PM_NB_DYNAMIC_MODE_REG	(0x24 - A3700_PM_NB_CLK_SHIFT)
#define  A3700_PM_NB_DFS_EN_OFF		(31)
#define  A3700_PM_NB_VDD_EN_OFF		(30)
#define  A3700_PM_NB_DIV_EN_OFF		(29)
#define  A3700_PM_NB_TBG_EN_OFF		(28)
#define  A3700_PM_NB_CLK_SEL_EN_OFF	(26)

#define A3700_PM_NB_CPU_LOAD_REG	(0x30 - A3700_PM_NB_CLK_SHIFT)
#define  A3700_PM_NB_CPU_LOAD_OFF	(0)
#define  A3700_PM_NB_CPU_LOAD_MASK	(0x3)

#define KHZ_TO_HZ			1000
#define MHZ_TO_KHZ			1000
#define MHZ_TO_HZ			1000000

#define to_clk(hw) container_of(hw, struct armada_3700_clk_pm, hw)

/* Clock source selection */
enum armada_3700_clk_select {
	CLK_SEL_OSC = 0,
	CLK_SEL_TBG,
};

/* TBG source selection */
enum armada_3700_tbg_select {
	TBG_A_P = 0,
	TBG_B_P = 1,
	TBG_A_S = 2,
	TBG_B_S = 3
};

/* DVFS LOAD index */
enum armada_3700_dvfs_load_index {
	DVFS_LOAD_0 = 0,
	DVFS_LOAD_1,
	DVFS_LOAD_2,
	DVFS_LOAD_3,
	DVFS_LOAD_MAX_NUM
};

/*
 * struct armada_3700_clk_pm:
 * @max_cpu_freq: Max CPU frequency, kHz
 * @divider: the TBG divider for the max cpu frequency
 * @clk_name: Clock controller name
 * @hw: HW specific structure of Cluster clock controller
 * @reg: register base of a3700 clock power management
 * @data: clock one cell data
 */
struct armada_3700_clk_pm {
	int max_cpu_freq;
	int divider;
	const char *clk_name;
	struct device *dev;
	struct clk_hw hw;
	void __iomem *reg;
	struct clk_onecell_data *data;
};

/*
 * struct armada_a3700_clk_pm_div: the cpu clock information for power management
 * @max_cpu_freq: Max CPU frequency supported
 * @clk_sel: Clock source selection
 * @tbg_sel: TBG source selection
 * @divder: Clock divider for each dvfs load level
 */
struct armada_a3700_clk_pm_info {
	u32 cpu_freq_max;/* KHz */
	enum armada_3700_clk_select clk_sel;
	enum armada_3700_tbg_select tbg_sel;
	u8 divider[DVFS_LOAD_MAX_NUM];
};

/*
 * CPU clock TBG divider static array
 * TBG divider 0 and 7 mean active-high for clock output,
 * thus they are not listed below.
 * For other divider values from 1 to 6, proper divider values should be set
 * for each level from small to big.
 * CPU frequency mapping:
 * original CPU freq    1st CPU freq     2nd CPU freq    3rd CPU freq    4th CPU freq
 *   1200M                1200M             600M             300M            200M
 *   1000M                1000M             500M             250M            200M
 *   800M                 800M              400M             266M            200M
 *   600M                 600M              300M             240M            200M
 */
static struct armada_a3700_clk_pm_info a3700_pm_info[] = {
	{.cpu_freq_max = 1200, .clk_sel = CLK_SEL_TBG, .tbg_sel = TBG_A_P, .divider = {1, 2, 4, 6} },
	{.cpu_freq_max = 1000, .clk_sel = CLK_SEL_TBG, .tbg_sel = TBG_B_S, .divider = {1, 2, 4, 5} },
	{.cpu_freq_max = 800,  .clk_sel = CLK_SEL_TBG, .tbg_sel = TBG_A_P, .divider = {1, 2, 3, 4} },
	{.cpu_freq_max = 600,  .clk_sel = CLK_SEL_TBG, .tbg_sel = TBG_A_P, .divider = {2, 4, 5, 6} },
};

static struct armada_a3700_clk_pm_info *armada_3700_clk_pm_info_get(u32 max_cpu_freq)
{
	int i;

	for (i = 0; i < ARRAY_SIZE(a3700_pm_info); i++) {
		if (max_cpu_freq == a3700_pm_info[i].cpu_freq_max)
			break;
	}
	if (i == ARRAY_SIZE(a3700_pm_info)) {
		pr_err("unsupported CPU frequency %d MHz\n", max_cpu_freq);
		return NULL;
	}

	return &a3700_pm_info[i];
}

static void armada3700_clk_pm_enable_dvfs(struct armada_3700_clk_pm *clk_pm_data)
{
	unsigned int reg;

	/* Disable dynamic CPU voltage and frequency scaling */
	reg = readl(clk_pm_data->reg + A3700_PM_NB_DYNAMIC_MODE_REG);
	reg &= ~(1 << A3700_PM_NB_DFS_EN_OFF);
	writel(reg, clk_pm_data->reg + A3700_PM_NB_DYNAMIC_MODE_REG);

	/* Set highest CPU Load 0 by default */
	reg = readl(clk_pm_data->reg + A3700_PM_NB_CPU_LOAD_REG);
	reg &= ~(A3700_PM_NB_CPU_LOAD_MASK << A3700_PM_NB_CPU_LOAD_OFF);
	reg |= ((DVFS_LOAD_0 & A3700_PM_NB_CPU_LOAD_MASK) << A3700_PM_NB_CPU_LOAD_OFF);
	writel(reg, clk_pm_data->reg + A3700_PM_NB_CPU_LOAD_REG);

	/* Enable dynamic CPU voltage and frequency scaling */
	reg = readl(clk_pm_data->reg + A3700_PM_NB_DYNAMIC_MODE_REG);
	reg |= (1 << A3700_PM_NB_DFS_EN_OFF);
	reg |= (1 << A3700_PM_NB_VDD_EN_OFF);
	reg |= (1 << A3700_PM_NB_DIV_EN_OFF);
	reg |= (1 << A3700_PM_NB_TBG_EN_OFF);
	reg |= (1 << A3700_PM_NB_CLK_SEL_EN_OFF);
	writel(reg, clk_pm_data->reg + A3700_PM_NB_DYNAMIC_MODE_REG);
}

static void armada3700_clk_pm_disable_dvfs(struct armada_3700_clk_pm *clk_pm_data)
{
	unsigned int reg;

	/* Disable dynamic CPU voltage and frequency scaling */
	reg = readl(clk_pm_data->reg + A3700_PM_NB_DYNAMIC_MODE_REG);
	reg &= ~(1 << A3700_PM_NB_DFS_EN_OFF);
	reg &= ~(1 << A3700_PM_NB_VDD_EN_OFF);
	reg &= ~(1 << A3700_PM_NB_DIV_EN_OFF);
	reg &= ~(1 << A3700_PM_NB_TBG_EN_OFF);
	reg &= ~(1 << A3700_PM_NB_CLK_SEL_EN_OFF);
	writel(reg, clk_pm_data->reg + A3700_PM_NB_DYNAMIC_MODE_REG);

	/* Enable dynamic CPU voltage and frequency scaling */
	reg = readl(clk_pm_data->reg + A3700_PM_NB_DYNAMIC_MODE_REG);
	reg |= (1 << A3700_PM_NB_DFS_EN_OFF);
	writel(reg, clk_pm_data->reg + A3700_PM_NB_DYNAMIC_MODE_REG);

	/* Set highest CPU Load 0 by default */
	reg = readl(clk_pm_data->reg + A3700_PM_NB_CPU_LOAD_REG);
	reg &= ~(A3700_PM_NB_CPU_LOAD_MASK << A3700_PM_NB_CPU_LOAD_OFF);
	reg |= ((DVFS_LOAD_0 & A3700_PM_NB_CPU_LOAD_MASK) << A3700_PM_NB_CPU_LOAD_OFF);
	writel(reg, clk_pm_data->reg + A3700_PM_NB_CPU_LOAD_REG);
}


static bool armada3700_clk_pm_dvfs_is_enabled(struct armada_3700_clk_pm *clk_pm_data)
{
	bool is_enabled = false;
	unsigned int reg;

	reg = readl(clk_pm_data->reg + A3700_PM_NB_DYNAMIC_MODE_REG);
	if (reg & (1 << A3700_PM_NB_DFS_EN_OFF))
		is_enabled = true;

	return is_enabled;
}

static unsigned long armada_3700_clk_recalc_rate(struct clk_hw *hw, unsigned long parent_rate)
{
	unsigned long rate = 0;
	struct armada_a3700_clk_pm_info *clk_pm_info = NULL;
	struct armada_3700_clk_pm *clk = to_clk(hw);
	int load_level = readl(clk->reg + A3700_PM_NB_CPU_LOAD_REG) * A3700_PM_NB_CPU_LOAD_MASK;

	if (armada3700_clk_pm_dvfs_is_enabled(clk)) {
		clk_pm_info = armada_3700_clk_pm_info_get(clk->max_cpu_freq);
		if (clk_pm_info)
			rate = (clk->max_cpu_freq * clk->divider) / clk_pm_info->divider[load_level];
	} else {
		rate = clk->max_cpu_freq;
	}

	rate *= MHZ_TO_HZ;

	return rate;
}

static long armada_3700_clk_round_rate(struct clk_hw *hw, unsigned long rate,
				 unsigned long *parent_rate)
{
	struct armada_3700_clk_pm *clk = to_clk(hw);
	unsigned int new_rate = rate / KHZ_TO_HZ;  /* kHz */
	int divider = (clk->max_cpu_freq * clk->divider * MHZ_TO_KHZ) / new_rate;

	if (armada3700_clk_pm_dvfs_is_enabled(clk))
		new_rate = ((clk->max_cpu_freq * clk->divider * MHZ_TO_KHZ) / divider) * KHZ_TO_HZ;
	else
		new_rate = clk->max_cpu_freq * MHZ_TO_HZ;

	return new_rate;
}

static int armada3700_clk_set_rate(struct clk_hw *hw, unsigned long rate,
			      unsigned long parent_rate)
{
	struct armada_a3700_clk_pm_info *clk_pm_info = NULL;
	struct armada_3700_clk_pm *clk = to_clk(hw);
	unsigned int new_rate = rate / KHZ_TO_HZ;  /* KHz */
	int divider, load_level;
	u32 reg_val;

	/* Calculate the clock divider */
	divider = (clk->max_cpu_freq * clk->divider * KHZ_TO_HZ) / new_rate;

	clk_pm_info = armada_3700_clk_pm_info_get(clk->max_cpu_freq);
	if (clk_pm_info == NULL)
		return -1;

	if (armada3700_clk_pm_dvfs_is_enabled(clk)) {
		for (load_level = DVFS_LOAD_0; load_level < DVFS_LOAD_MAX_NUM; load_level++) {
			if (clk_pm_info->divider[load_level] == divider)
				break;
		}
		if (load_level == DVFS_LOAD_MAX_NUM) {
			dev_err(clk->dev, "invalid CPU new rate %d kHz\n", new_rate);
			return -1;
		}
	} else {
		load_level = DVFS_LOAD_0;
	}

	/* Set CPU load status register */
	reg_val = readl(clk->reg + A3700_PM_NB_CPU_LOAD_REG);
	reg_val &= ~(A3700_PM_NB_CPU_LOAD_MASK << A3700_PM_NB_CPU_LOAD_OFF);
	reg_val |= ((load_level & A3700_PM_NB_CPU_LOAD_MASK) << A3700_PM_NB_CPU_LOAD_OFF);
	writel(reg_val, clk->reg + A3700_PM_NB_CPU_LOAD_REG);

	return 0;
}

static const struct clk_ops armada_3700_clk_ops = {
	.recalc_rate	= armada_3700_clk_recalc_rate,
	.round_rate	= armada_3700_clk_round_rate,
	.set_rate	= armada3700_clk_set_rate,
};

static int armada_3700_clk_pm_div_array_set(struct armada_3700_clk_pm *clk_pm_data)
{
	void __iomem *reg_addr;
	struct armada_a3700_clk_pm_info *clk_pm_info = NULL;
	int load_level, shift;
	u32 reg_val;
	u8 clk_sel, tbg_sel, tbg_div, vdd_sel;

	clk_pm_info = armada_3700_clk_pm_info_get(clk_pm_data->max_cpu_freq);
	if (clk_pm_info == NULL)
		return -1;

	clk_pm_data->divider = clk_pm_info->divider[DVFS_LOAD_0];

	for (load_level = DVFS_LOAD_0; load_level < DVFS_LOAD_MAX_NUM; load_level++) {
		reg_addr = clk_pm_data->reg;
		if (load_level <= DVFS_LOAD_1)
			reg_addr += A3700_PM_NB_L0_L1_CONFIG_REG;
		else
			reg_addr += A3700_PM_NB_L2_L3_CONFIG_REG;

		/* Acquire shift within register */
		if (load_level == DVFS_LOAD_0 || load_level == DVFS_LOAD_2)
			shift = A3700_PM_NB_LX_CONFIG_SHIFT;
		else
			shift = 0;

		clk_sel = clk_pm_info->clk_sel;
		tbg_sel = clk_pm_info->tbg_sel;
		tbg_div = clk_pm_info->divider[load_level];
		vdd_sel = load_level;

		reg_val = readl(reg_addr);

		/* Set clock source */
		reg_val &= ~(A3700_PM_NB_CLK_SEL_LX_MASK << (shift + A3700_PM_NB_CLK_SEL_LX_OFF));
		reg_val |= (clk_sel & A3700_PM_NB_CLK_SEL_LX_MASK) << (shift + A3700_PM_NB_CLK_SEL_LX_OFF);

		/* Set TBG source */
		reg_val &= ~(A3700_PM_NB_TBG_SEL_LX_MASK << (shift + A3700_PM_NB_TBG_SEL_LX_OFF));
		reg_val |= (tbg_sel & A3700_PM_NB_TBG_SEL_LX_MASK) << (shift + A3700_PM_NB_TBG_SEL_LX_OFF);

		/* Set clock divider */
		reg_val &= ~(A3700_PM_NB_TBG_DIV_LX_MASK << (shift + A3700_PM_NB_TBG_DIV_LX_OFF));
		reg_val |= (tbg_div & A3700_PM_NB_TBG_DIV_LX_MASK) << (shift + A3700_PM_NB_TBG_DIV_LX_OFF);

		/* Set VDD divider */
		reg_val &= ~(A3700_PM_NB_VDD_SEL_LX_MASK << (shift + A3700_PM_NB_VDD_SEL_LX_OFF));
		reg_val |= (vdd_sel & A3700_PM_NB_VDD_SEL_LX_MASK) << (shift + A3700_PM_NB_VDD_SEL_LX_OFF);

		writel(reg_val, reg_addr);
	}

	return 0;
}

static int armada_3700_clk_pm_probe(struct platform_device *pdev)
{
	struct device_node *np = pdev->dev.of_node;
	struct device *dev = &pdev->dev;
	struct clk_onecell_data *clk_cell_data;
	struct armada_3700_clk_pm *a3700_clk_pm_data;
	struct clk_init_data init;
	struct clk *clk, **clks;
	struct resource *res;
	struct clk *max_cpu_clk;
	void __iomem *reg;
	int ret;

	clk_cell_data = devm_kzalloc(&pdev->dev, sizeof(struct clk_onecell_data), GFP_KERNEL);
	if (WARN_ON(!clk_cell_data))
		return -ENOMEM;

	clks = devm_kzalloc(dev, sizeof(*clks), GFP_KERNEL);
	if (WARN_ON(!clks)) {
		ret = -ENOMEM;
		goto free_cell_data;
	}

	a3700_clk_pm_data = devm_kzalloc(dev, sizeof(*a3700_clk_pm_data), GFP_KERNEL);
	if (WARN_ON(!a3700_clk_pm_data)) {
		ret = -ENOMEM;
		goto free_clks;
	}

	platform_set_drvdata(pdev, a3700_clk_pm_data);

	/* Get maximum possible frequency */
	max_cpu_clk = devm_clk_get(&pdev->dev, NULL);
	if (IS_ERR(max_cpu_clk)) {
		dev_err(dev, "error getting max cpu frequency\n");
		ret = PTR_ERR(max_cpu_clk);
		goto free_clk_pm_data;
	}

	/* Map register space */
	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	reg = devm_ioremap_resource(dev, res);
	if (IS_ERR(reg)) {
		ret = PTR_ERR(reg);
		goto free_clk_pm_data;
	}

	a3700_clk_pm_data->max_cpu_freq = clk_get_rate(max_cpu_clk) / MHZ_TO_HZ;
	a3700_clk_pm_data->clk_name = "a37xx-clk-pm";
	a3700_clk_pm_data->reg = reg;
	a3700_clk_pm_data->dev = dev;
	a3700_clk_pm_data->hw.init = &init;
	a3700_clk_pm_data->data = clk_cell_data;

	/* Set clock PM divider array */
	ret = armada_3700_clk_pm_div_array_set(a3700_clk_pm_data);
	if (ret) {
		dev_err(dev, "failed to set clock fdivider array for power management\n");
		return ret;
	}

	init.name = a3700_clk_pm_data->clk_name;
	init.ops = &armada_3700_clk_ops;
	init.num_parents = 0;
	init.flags = CLK_GET_RATE_NOCACHE | CLK_IS_ROOT;

	/* clock register */
	clk = devm_clk_register(dev, &a3700_clk_pm_data->hw);
	if (IS_ERR(clk)) {
		ret = PTR_ERR(clk);
		goto unmap_res;
	}
	*clks = clk;

	clk_cell_data->clks = clks;
	clk_cell_data->clk_num = 1;
	ret = of_clk_add_provider(np, of_clk_src_onecell_get, clk_cell_data);
	if (ret) {
		dev_err(dev, "failed to register OF clock provider\n");
		goto unregister_clk;
	}

	/* Enabled DVFS */
	armada3700_clk_pm_enable_dvfs(a3700_clk_pm_data);

	return 0;

unregister_clk:
	clk_unregister(a3700_clk_pm_data->data->clks[0]);
unmap_res:
	devm_iounmap(&pdev->dev, reg);
	devm_release_mem_region(&pdev->dev, res->start, resource_size(res));
free_clk_pm_data:
	devm_kfree(&pdev->dev, a3700_clk_pm_data);
free_clks:
	devm_kfree(&pdev->dev, clks);
free_cell_data:
	devm_kfree(&pdev->dev, clk_cell_data);

	return ret;
}

static int armada_3700_clk_pm_remove(struct platform_device *pdev)
{
	struct device_node *np = pdev->dev.of_node;
	struct armada_3700_clk_pm *a3700_clk_pm_data = platform_get_drvdata(pdev);

	/* Disbale DVFS */
	armada3700_clk_pm_disable_dvfs(a3700_clk_pm_data);

	/* Delete clock provider */
	of_clk_del_provider(np);

	/* Unregister clock */
	clk_unregister(a3700_clk_pm_data->data->clks[0]);

	/* Free resources */
	devm_iounmap(&pdev->dev, a3700_clk_pm_data->reg);
	devm_kfree(&pdev->dev, *a3700_clk_pm_data->data->clks);
	devm_kfree(&pdev->dev, a3700_clk_pm_data->data);
	devm_kfree(&pdev->dev, a3700_clk_pm_data);

	return 0;
}

#ifdef CONFIG_PM
static int armada_3700_clk_pm_resume_noirq(struct device *dev)
{
	struct platform_device *pdev = to_platform_device(dev);
	struct armada_3700_clk_pm *a3700_clk_pm_data = platform_get_drvdata(pdev);
	int ret;

	/* Set clock PM divider array */
	ret = armada_3700_clk_pm_div_array_set(a3700_clk_pm_data);
	if (ret) {
		dev_err(dev, "failed to set clock fdivider array for power management\n");
		return ret;
	}

	/* Enable DVFS */
	armada3700_clk_pm_enable_dvfs(a3700_clk_pm_data);

	return 0;
}

static const struct dev_pm_ops armada_3700_clk_pm_ops = {
	.resume_noirq = armada_3700_clk_pm_resume_noirq,
};

#define ARMADA_3700_CLK_PM_OPS (&armada_3700_clk_pm_ops)
#else
#define ARMADA_3700_CLK_PM_OPS NULL
#endif /* CONFIG_PM */

static const struct of_device_id armada37xx_clk_pm_of_match[] = {
	{ .compatible = "marvell,armada-37xx-cpu-pm-clk", },
	{}
};

static struct platform_driver armada3700_clk_pm_driver = {
	.driver	= {
		.name = "armada37xx-clk-pm",
		.pm = ARMADA_3700_CLK_PM_OPS,
		.of_match_table = armada37xx_clk_pm_of_match,
	},
	.probe = armada_3700_clk_pm_probe,
	.remove = armada_3700_clk_pm_remove,
};

static int __init armada3700_clk_pm_init(void)
{
	return platform_driver_register(&armada3700_clk_pm_driver);
}
subsys_initcall(armada3700_clk_pm_init);

