// SPDX-License-Identifier: GPL-2.0
/*
 * CN20K (CN206XXS) CLKGEN Clock Driver
 *
 * Registers all CLKGEN instances as Linux Common Clock Framework clocks.
 * Provides read-only observation of clock frequencies, PLL/ARO lock status,
 * droop detection, and hardware frequency measurement.
 *
 * Copyright (c) 2026 Marvell.
 */

#include <linux/clk-provider.h>
#include <linux/debugfs.h>
#include <linux/io.h>
#include <linux/module.h>
#include <linux/of.h>
#include <linux/of_address.h>
#include <linux/platform_device.h>
#include <linux/seq_file.h>
#include <linux/delay.h>

#include <dt-bindings/clock/marvell,cn20k.h>

/* CLKGEN register offsets relative to per-instance base */
#define CLKGEN_PGM		0x00
#define CLKGEN_MAN		0x08
#define CLKGEN_POWER		0x18
#define CLKGEN_FREQ_CHK		0x20
#define CLKGEN_PLL_SETTINGS(p)	(0x40 + (p) * 0x8)
#define CLKGEN_CONST		0x70
#define CLKGEN_ARO_DROOP	0x90
#define CLKGEN_ARO_SETTINGS	0xA0

/* Each CLKGEN instance is at phys_base + (index << 32) */
#define CLKGEN_INST_STRIDE_SHIFT	32
#define CLKGEN_INST_REG_SIZE		0x200
#define CLKGEN_PHYS_BASE		0xCD0000000000ULL

/* CLKGEN_PGM fields */
#define PGM_CUR_LOCK		BIT_ULL(63)
#define PGM_ALT_REF_SHIFT	61
#define PGM_ALT_REF_MASK	GENMASK_ULL(62, 61)
#define PGM_CUR_DROOP		BIT_ULL(60)
#define PGM_CUR_SEL_SHIFT	57
#define PGM_CUR_SEL_MASK	GENMASK_ULL(59, 57)
#define PGM_CUR_MUL_SHIFT	48
#define PGM_CUR_MUL_MASK	GENMASK_ULL(54, 48)

/* CLKGEN_CONST fields */
#define CONST_ARO_PRESENT	BIT_ULL(8)
#define CONST_PLL1_PRESENT	BIT_ULL(7)
#define CONST_PLL0_PRESENT	BIT_ULL(6)
#define CONST_ANALOG_PLLS	BIT_ULL(9)

/* CLKGEN_POWER fields */
#define POWER_ARO_PD		BIT_ULL(62)
#define POWER_PLL1_PD		BIT_ULL(61)
#define POWER_PLL0_PD		BIT_ULL(60)
#define POWER_PLL_DROOP		BIT_ULL(15)

/* CLKGEN_PLL_SETTINGS / CLKGEN_ARO_SETTINGS shared fields */
#define PLLARO_IN_USE		BIT_ULL(63)
#define PLLARO_LOCK		BIT_ULL(60)
#define PLLARO_POWER_DOWN	BIT_ULL(55)
#define PLLARO_VCO_MUL_SHIFT	34
#define PLLARO_VCO_MUL_MASK	GENMASK_ULL(43, 34)
#define PLLARO_VCO_FRACT_SHIFT	24
#define PLLARO_VCO_FRACT_MASK	GENMASK_ULL(33, 24)
#define PLLARO_UPDATE_RATE_MASK	GENMASK_ULL(9, 0)

/* CLKGEN_FREQ_CHK fields */
#define FREQ_CHK_CALC		BIT_ULL(63)
#define FREQ_CHK_RESULT_MASK	GENMASK_ULL(16, 0)

/* CLKGEN_ARO_DROOP fields */
#define ARO_DROOP_DETECT	BIT_ULL(63)

/* CLKGEN_SEL_E values */
#define SEL_RUNT	0
#define SEL_REFCLK	1
#define SEL_BYPASS	2
#define SEL_OFF		3
#define SEL_PLL0	4
#define SEL_PLL1	5
#define SEL_ARO		6
#define SEL_SWAP_PLL	7

static const char * const sel_names[] = {
	"RUNT", "REFCLK", "BYPASS", "OFF", "PLL0", "PLL1", "ARO", "SWAP_PLL"
};

struct cn20k_clkgen_desc {
	const char	*name;
	u16		index;
	u8		refclk;
};

static const struct cn20k_clkgen_desc cn20k_clkgen_descs[] = {
	/* AP core clocks */
	{ "coreclk0",   0x00, 0 }, { "coreclk1",   0x01, 0 },
	{ "coreclk2",   0x02, 0 }, { "coreclk3",   0x03, 0 },
	{ "coreclk4",   0x04, 0 }, { "coreclk5",   0x05, 0 },
	{ "coreclk6",   0x06, 0 }, { "coreclk7",   0x07, 0 },
	{ "coreclk8",   0x08, 0 }, { "coreclk9",   0x09, 0 },
	{ "coreclk10",  0x0A, 0 }, { "coreclk11",  0x0B, 0 },
	{ "coreclk12",  0x0C, 0 }, { "coreclk13",  0x0D, 0 },
	{ "coreclk14",  0x0E, 0 }, { "coreclk15",  0x0F, 0 },
	{ "coreclk16",  0x10, 0 }, { "coreclk17",  0x11, 0 },
	{ "coreclk18",  0x12, 0 }, { "coreclk19",  0x13, 0 },
	{ "coreclk20",  0x14, 0 }, { "coreclk21",  0x15, 0 },
	{ "coreclk22",  0x16, 0 }, { "coreclk23",  0x17, 0 },
	{ "coreclk24",  0x18, 0 }, { "coreclk25",  0x19, 0 },
	{ "coreclk26",  0x1A, 0 }, { "coreclk27",  0x1B, 0 },
	{ "coreclk28",  0x1C, 0 }, { "coreclk29",  0x1D, 0 },
	{ "coreclk30",  0x1E, 0 }, { "coreclk31",  0x1F, 0 },
	{ "coreclk32",  0x20, 0 }, { "coreclk33",  0x21, 0 },
	{ "coreclk34",  0x22, 0 }, { "coreclk35",  0x23, 0 },
	{ "coreclk36",  0x24, 0 }, { "coreclk37",  0x25, 0 },
	{ "coreclk38",  0x26, 0 }, { "coreclk39",  0x27, 0 },
	{ "coreclk40",  0x28, 0 }, { "coreclk41",  0x29, 0 },
	/* System clocks */
	{ "cn20k_ioclk",    0xC0, 0 },
	{ "cn20k_sclk",     0xC1, 0 },
	{ "cn20k_meshclk",  0xC2, 0 },
	{ "cn20k_cptclk",   0xC3, 0 },
	{ "cn20k_mlclk",    0xC4, 0 },
	{ "cn20k_netclk",   0xC5, 0 },
	{ "cn20k_ptpclk",   0xC7, 3 },
	{ "cn20k_btsclk",   0xC8, 0 },
	{ "cn20k_rgmiiclk", 0xC9, 0 },
	/* PCIe clocks */
	{ "cn20k_pcieclk0", 0xE0, 4 },
	{ "cn20k_pcieclk1", 0xE1, 5 },
	{ "cn20k_pcieclk2", 0xE2, 6 },
	/* DDR interface clocks */
	{ "cn20k_dipclk0",  0xF0, 1 },
	{ "cn20k_dipclk1",  0xF1, 2 },
	{ "cn20k_dipclk2",  0xF2, 1 },
	{ "cn20k_dipclk3",  0xF3, 2 },
	{ "cn20k_dipclk4",  0xF4, 1 },
	{ "cn20k_dipclk5",  0xF5, 2 },
	{ "cn20k_dipclk6",  0xF6, 1 },
	{ "cn20k_dipclk7",  0xF7, 2 },
};

#define CN20K_NUM_CLKS	ARRAY_SIZE(cn20k_clkgen_descs)

struct cn20k_clk {
	struct clk_hw			hw;
	void __iomem			*base;
	const struct cn20k_clkgen_desc	*desc;
};

struct cn20k_clk_provider {
	struct device		*dev;
	struct cn20k_clk	clks[CN20K_NUM_CLKS];
	struct clk_hw_onecell_data *hw_data;
	struct dentry		*debugfs_root;
};

#define to_cn20k_clk(hw)	container_of(hw, struct cn20k_clk, hw)

static inline void __iomem *cn20k_clk_reg(struct cn20k_clk *clk, u32 offset)
{
	return clk->base + offset;
}

/* 
 * Calculate the clock rate for a given clock.
 * 
 * @hw: the clock handle
 * @parent_rate: the rate of the parent clock
 * 
 * @return: the clock rate
 */
static unsigned long cn20k_clk_recalc_rate(struct clk_hw *hw,
					   unsigned long parent_rate)
{
	struct cn20k_clk *clk = to_cn20k_clk(hw);
	u64 pgm, aro_set;
	u8 cur_sel, cur_mul, alt_ref;
	u16 vco_mul, vco_fract, update_rate;
	u32 ticks;

	pgm = readq(cn20k_clk_reg(clk, CLKGEN_PGM));

	cur_sel = FIELD_GET(PGM_CUR_SEL_MASK, pgm);
	cur_mul = FIELD_GET(PGM_CUR_MUL_MASK, pgm);
	alt_ref = FIELD_GET(PGM_ALT_REF_MASK, pgm);

	if (cur_sel == SEL_ARO) {
		aro_set = readq(cn20k_clk_reg(clk, CLKGEN_ARO_SETTINGS));
		vco_mul = FIELD_GET(PLLARO_VCO_MUL_MASK, aro_set);
		vco_fract = FIELD_GET(PLLARO_VCO_FRACT_MASK, aro_set);
		update_rate = FIELD_GET(PLLARO_UPDATE_RATE_MASK, aro_set);

		ticks = (u32)vco_mul * 50 + vco_fract;
		if (update_rate == 50)
			return (unsigned long)ticks * 2 * 1000000UL;
		else if (update_rate == 100)
			return (unsigned long)ticks * 1000000UL;
		else if (update_rate > 0)
			return (unsigned long)ticks * 100000000UL / update_rate;
		return 0;
	}

	if (cur_mul <= 3)
		return 0;

	if (alt_ref == 0)
		return (unsigned long)cur_mul * 50 * 1000000UL;

	/* ALT_REF 10 or 11: units are 30.72 MHz */
	return (unsigned long)cur_mul * 30720000UL;
}

/* 
 * Check if the clock is enabled.
 * 
 * @hw: the clock handle
 * 
 * @return: 1 if the clock is enabled, 0 otherwise
 */
static int cn20k_clk_is_enabled(struct clk_hw *hw)
{
	struct cn20k_clk *clk = to_cn20k_clk(hw);
	u64 pgm;
	u8 cur_sel;

	pgm = readq(cn20k_clk_reg(clk, CLKGEN_PGM));
	cur_sel = FIELD_GET(PGM_CUR_SEL_MASK, pgm);

	return cur_sel != SEL_OFF;
}

static const struct clk_ops cn20k_clk_ops = {
	.recalc_rate = cn20k_clk_recalc_rate,
	.is_enabled = cn20k_clk_is_enabled,
};

static int cn20k_index_to_array(u16 index)
{
	int i;

	for (i = 0; i < CN20K_NUM_CLKS; i++) {
		if (cn20k_clkgen_descs[i].index == index)
			return i;
	}
	return -EINVAL;
}

static struct clk_hw *cn20k_clk_hw_get(struct of_phandle_args *clkspec,
					void *data)
{
	struct clk_hw_onecell_data *hw_data = data;
	u32 id = clkspec->args[0];
	int idx;

	idx = cn20k_index_to_array(id);
	if (idx < 0 || idx >= hw_data->num)
		return ERR_PTR(-EINVAL);

	return hw_data->hws[idx];
}

/* 
 * Show the PLL / ARO lock and power status.
 * 
 * @s: the sequence file
 * @data: the data
 * 
 * @return: 0 on success, error code otherwise
 */
static int cn20k_pll_status_show(struct seq_file *s, void *data)
{
	struct cn20k_clk_provider *prov = s->private;
	int i;

	seq_printf(s, "%-12s %-7s %-6s  %-6s %-6s %-6s  %-6s %-6s %-6s  %-5s\n",
		   "CLOCK", "SOURCE", "LOCK",
		   "PLL0", "P0_LCK", "P0_PWR",
		   "PLL1", "P1_LCK", "P1_PWR",
		   "DROOP");

	for (i = 0; i < CN20K_NUM_CLKS; i++) {
		struct cn20k_clk *clk = &prov->clks[i];
		u64 pgm, cst, pwr, pll0 = 0, pll1 = 0, aro_drp = 0;
		u8 cur_sel;
		bool pll0_present, pll1_present, aro_present;

		pgm = readq(cn20k_clk_reg(clk, CLKGEN_PGM));
		cst = readq(cn20k_clk_reg(clk, CLKGEN_CONST));
		pwr = readq(cn20k_clk_reg(clk, CLKGEN_POWER));

		cur_sel = FIELD_GET(PGM_CUR_SEL_MASK, pgm);
		pll0_present = !!(cst & CONST_PLL0_PRESENT);
		pll1_present = !!(cst & CONST_PLL1_PRESENT);
		aro_present  = !!(cst & CONST_ARO_PRESENT);

		if (pll0_present)
			pll0 = readq(cn20k_clk_reg(clk, CLKGEN_PLL_SETTINGS(0)));
		if (pll1_present)
			pll1 = readq(cn20k_clk_reg(clk, CLKGEN_PLL_SETTINGS(1)));
		if (aro_present)
			aro_drp = readq(cn20k_clk_reg(clk, CLKGEN_ARO_DROOP));

		seq_printf(s, "%-12s %-7s %-6s  %-6s %-6s %-6s  %-6s %-6s %-6s  %-5s\n",
			   clk->desc->name,
			   sel_names[cur_sel & 7],
			   (pgm & PGM_CUR_LOCK) ? "YES" : "NO",
			   pll0_present ? "yes" : "--",
			   pll0_present ? ((pll0 & PLLARO_LOCK) ? "LOCK" : "UNLK") : "--",
			   pll0_present ? ((pwr & POWER_PLL0_PD) ? "off" : "on") : "--",
			   pll1_present ? "yes" : "--",
			   pll1_present ? ((pll1 & PLLARO_LOCK) ? "LOCK" : "UNLK") : "--",
			   pll1_present ? ((pwr & POWER_PLL1_PD) ? "off" : "on") : "--",
			   aro_present ? ((aro_drp & ARO_DROOP_DETECT) ? "YES" : "no") : "--");
	}
	return 0;
}
DEFINE_SHOW_ATTRIBUTE(cn20k_pll_status);

static const char * const refclk_names[] = {
	"REF_CLK0 (100 MHz)",
	"REF_CLK1 (100 MHz)",
	"REF_CLK2 (100 MHz)",
	"REF_CLK3 (156.25 MHz)",
	"REF_CLK4 (100 MHz, SSC-opt)",
	"REF_CLK5 (100 MHz, SSC-opt)",
	"REF_CLK6 (100 MHz, SSC-opt)",
};
#define REFCLK_COUNT	ARRAY_SIZE(refclk_names)

static int cn20k_refclk_status_show(struct seq_file *s, void *data)
{
	struct cn20k_clk_provider *prov = s->private;
	const char *evidence[REFCLK_COUNT] = {};
	bool detected[REFCLK_COUNT] = {};
	int i, r;

	for (i = 0; i < CN20K_NUM_CLKS; i++) {
		struct cn20k_clk *clk = &prov->clks[i];
		u64 pgm;
		u8 cur_sel;

		r = clk->desc->refclk;
		if (detected[r])
			continue;

		pgm = readq(cn20k_clk_reg(clk, CLKGEN_PGM));
		cur_sel = FIELD_GET(PGM_CUR_SEL_MASK, pgm);

		if ((pgm & PGM_CUR_LOCK) && cur_sel >= SEL_PLL0) {
			detected[r] = true;
			evidence[r] = clk->desc->name;
		}
	}

	for (r = 0; r < REFCLK_COUNT; r++) {
		seq_printf(s, "%-28s %s",
			   refclk_names[r],
			   detected[r] ? "DETECTED" : "NOT DETECTED");
		if (evidence[r])
			seq_printf(s, "  (via %s PLL lock)", evidence[r]);
		seq_putc(s, '\n');
	}
	return 0;
}
DEFINE_SHOW_ATTRIBUTE(cn20k_refclk_status);

/* 
 * Show the hardware frequency measurement via FREQ_CHK.
 * 
 * @s: the sequence file
 * @data: the data
 * 
 * @return: 0 on success, error code otherwise
 */
static int cn20k_freq_check_show(struct seq_file *s, void *data)
{
	struct cn20k_clk_provider *prov = s->private;
	int i;

	seq_printf(s, "%-12s %10s %10s\n", "CLOCK", "EXPECTED", "MEASURED");

	for (i = 0; i < CN20K_NUM_CLKS; i++) {
		struct cn20k_clk *clk = &prov->clks[i];
		u64 val;
		u32 measured_mhz;
		unsigned long rate;
		int retries = 100;

		rate = cn20k_clk_recalc_rate(&clk->hw, 0);

		writeq(FREQ_CHK_CALC, cn20k_clk_reg(clk, CLKGEN_FREQ_CHK));
		udelay(50);

		measured_mhz = 0;
		while (retries-- > 0) {
			val = readq(cn20k_clk_reg(clk, CLKGEN_FREQ_CHK));
			if (!(val & FREQ_CHK_CALC)) {
				measured_mhz = FIELD_GET(FREQ_CHK_RESULT_MASK, val) / 10;
				break;
			}
			udelay(10);
		}

		seq_printf(s, "%-12s %7lu MHz %7u MHz\n",
			   clk->desc->name,
			   rate / 1000000,
			   measured_mhz);
	}
	return 0;
}
DEFINE_SHOW_ATTRIBUTE(cn20k_freq_check);

/* 
 * Show the droop status.
 * 
 * @s: the sequence file
 * @data: the data
 * 
 * @return: 0 on success, error code otherwise
 */
static int cn20k_droop_status_show(struct seq_file *s, void *data)
{
	struct cn20k_clk_provider *prov = s->private;
	int i;

	seq_printf(s, "%-12s %-5s %-10s\n", "CLOCK", "ARO", "DROOP");

	for (i = 0; i < CN20K_NUM_CLKS; i++) {
		struct cn20k_clk *clk = &prov->clks[i];
		u64 cst, pwr, aro_drp;
		bool aro_present;

		cst = readq(cn20k_clk_reg(clk, CLKGEN_CONST));
		aro_present = !!(cst & CONST_ARO_PRESENT);

		if (!aro_present) {
			seq_printf(s, "%-12s --    --\n", clk->desc->name);
			continue;
		}

		pwr = readq(cn20k_clk_reg(clk, CLKGEN_POWER));
		aro_drp = readq(cn20k_clk_reg(clk, CLKGEN_ARO_DROOP));

		seq_printf(s, "%-12s %-5s %-10s\n",
			   clk->desc->name,
			   (pwr & POWER_ARO_PD) ? "off" : "on",
			   (aro_drp & ARO_DROOP_DETECT) ? "DETECTED" : "none");
	}
	return 0;
}
DEFINE_SHOW_ATTRIBUTE(cn20k_droop_status);

static int cn20k_clkgen_probe(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	struct cn20k_clk_provider *prov;
	struct clk_hw_onecell_data *hw_data;
	int i, ret;

	prov = devm_kzalloc(dev, sizeof(*prov), GFP_KERNEL);
	if (!prov)
		return -ENOMEM;

	prov->dev = dev;

	hw_data = devm_kzalloc(dev, struct_size(hw_data, hws, CN20K_NUM_CLKS),
			       GFP_KERNEL);
	if (!hw_data)
		return -ENOMEM;

	hw_data->num = CN20K_NUM_CLKS;
	prov->hw_data = hw_data;

	for (i = 0; i < CN20K_NUM_CLKS; i++) {
		struct cn20k_clk *clk = &prov->clks[i];
		struct clk_init_data init = {};
		phys_addr_t phys;

		clk->desc = &cn20k_clkgen_descs[i];

		phys = CLKGEN_PHYS_BASE +
		       ((u64)clk->desc->index << CLKGEN_INST_STRIDE_SHIFT);
		clk->base = devm_ioremap(dev, phys, CLKGEN_INST_REG_SIZE);
		if (!clk->base) {
			dev_warn(dev, "Failed to map clock %s at %pa, skipping\n",
				 clk->desc->name, &phys);
			continue;
		}

		init.name = clk->desc->name;
		init.ops = &cn20k_clk_ops;
		init.num_parents = 0;
		init.flags = CLK_GET_RATE_NOCACHE;

		clk->hw.init = &init;

		ret = devm_clk_hw_register(dev, &clk->hw);
		if (ret) {
			dev_err(dev, "Failed to register clock %s: %d\n",
				clk->desc->name, ret);
			return ret;
		}

		hw_data->hws[i] = &clk->hw;
	}

	ret = devm_of_clk_add_hw_provider(dev, cn20k_clk_hw_get, hw_data);
	if (ret) {
		dev_err(dev, "Failed to add clock provider: %d\n", ret);
		return ret;
	}

	/* debugfs: /sys/kernel/debug/cn20k-clkgen/ */
	prov->debugfs_root = debugfs_create_dir("cn20k-clkgen", NULL);
	debugfs_create_file("pll_status", 0444, prov->debugfs_root,
			    prov, &cn20k_pll_status_fops);
	debugfs_create_file("refclk_status", 0444, prov->debugfs_root,
			    prov, &cn20k_refclk_status_fops);
	debugfs_create_file("freq_check", 0444, prov->debugfs_root,
			    prov, &cn20k_freq_check_fops);
	debugfs_create_file("droop_status", 0444, prov->debugfs_root,
			    prov, &cn20k_droop_status_fops);

	platform_set_drvdata(pdev, prov);

	dev_info(dev, "Registered %d CLKGEN clocks\n", (int)CN20K_NUM_CLKS);
	return 0;
}

static void cn20k_clkgen_remove(struct platform_device *pdev)
{
	struct cn20k_clk_provider *prov = platform_get_drvdata(pdev);

	debugfs_remove_recursive(prov->debugfs_root);

}

static const struct of_device_id cn20k_clkgen_of_match[] = {
	{ .compatible = "marvell,cn20k-clkgen" },
	{ }
};
MODULE_DEVICE_TABLE(of, cn20k_clkgen_of_match);

static struct platform_driver cn20k_clkgen_driver = {
	.driver = {
		.name = "cn20k-clkgen",
		.of_match_table = cn20k_clkgen_of_match,
	},
	.probe = cn20k_clkgen_probe,
	.remove = cn20k_clkgen_remove,
};
module_platform_driver(cn20k_clkgen_driver);

MODULE_AUTHOR("George Cherian <george.cherian@marvell.com>");
MODULE_DESCRIPTION("Marvell CN20K CLKGEN Clock Driver");
MODULE_LICENSE("GPL");
