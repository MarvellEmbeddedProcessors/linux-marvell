/*
 * ID and revision information for mvebu SoCs
 *
 * Copyright (C) 2014 Marvell
 *
 * Gregory CLEMENT <gregory.clement@free-electrons.com>
 *
 * This file is licensed under the terms of the GNU General Public
 * License version 2.  This program is licensed "as is" without any
 * warranty of any kind, whether express or implied.
 *
 * All the mvebu SoCs have information related to their variant and
 * revision that can be read from the PCI control register. This is
 * done before the PCI initialization to avoid any conflict. Once the
 * ID and revision are retrieved, the mapping is freed.
 */

#define pr_fmt(fmt) "mvebu-soc-id: " fmt

#include <linux/clk.h>
#include <linux/init.h>
#include <linux/io.h>
#include <linux/kernel.h>
#include <linux/of.h>
#include <linux/of_address.h>
#include "mvebu-soc-id.h"

#define PCIE_DEV_ID_OFF		0x0
#define PCIE_DEV_REV_OFF	0x8
#define A38X_DEV_ID_OFF		0x38
#define A38X_DEV_REV_OFF	0x3C

#define SOC_ID_MASK	    0xFFFF0000
#define SOC_REV_MASK	    0xFF
#define A38X_REV_MASK	    0xF

static u32 soc_dev_id;
static u32 soc_rev;
static bool is_id_valid;

static const struct of_device_id mvebu_pcie_of_match_table[] = {
	{ .compatible = "marvell,armada-xp-pcie", },
	{ .compatible = "marvell,armada-370-pcie", },
	{},
};

static const struct of_device_id mvebu_a38x_of_match_table[] = {
	{ .compatible = "marvell,armada-380-system-controller", },
	{},
};


int mvebu_get_soc_id(u32 *dev, u32 *rev)
{
	if (is_id_valid) {
		*dev = soc_dev_id;
		*rev = soc_rev;
		return 0;
	} else
		return -1;
}

static int __init mvebu_soc_id_init(void)
{
	struct device_node *np;
	int ret = 0;
	void __iomem *reg_base;
	struct clk *clk;
	struct device_node *child;
	bool is_pcie_id;

	np = of_find_matching_node(NULL, mvebu_pcie_of_match_table);
	if (!np) {/* If no pcie for soc-id, try A38x deicated register */
		np = of_find_matching_node(NULL, mvebu_a38x_of_match_table);
		if (!np)
			return ret;
		is_pcie_id = false;
	} else {
		is_pcie_id = true;

		/*
		 * ID and revision are available from any port, so we
		 * just pick the first one
		 */
		child = of_get_next_child(np, NULL);
		if (child == NULL) {
			pr_err("cannot get pci node\n");
			ret = -ENOMEM;
			goto clk_err;
		}

		clk = of_clk_get_by_name(child, NULL);
		if (IS_ERR(clk)) {
			pr_err("cannot get clock\n");
			ret = -ENOMEM;
			goto clk_err;
		}

		ret = clk_prepare_enable(clk);
		if (ret) {
			pr_err("cannot enable clock\n");
			goto clk_err;
		}
	}
	if (is_pcie_id == true)
		reg_base = of_iomap(child, 0);
	else
		reg_base = of_iomap(np, 0);
	if (IS_ERR(reg_base)) {
		pr_err("cannot map registers\n");
		ret = -ENOMEM;
		goto res_ioremap;
	}

	if (is_pcie_id == true) {
		/* SoC ID */
		soc_dev_id = readl(reg_base + PCIE_DEV_ID_OFF) >> 16;
		/* SoC revision */
		soc_rev = readl(reg_base + PCIE_DEV_REV_OFF) & SOC_REV_MASK;
	} else {
		/* SoC ID */
		soc_dev_id = readl(reg_base + A38X_DEV_ID_OFF) >> 16;
		/* SoC revision */
		soc_rev = (readl(reg_base + A38X_DEV_REV_OFF) >> 8) & A38X_REV_MASK;
	}

	is_id_valid = true;

	pr_info("MVEBU SoC ID=0x%X, Rev=0x%X\n", soc_dev_id, soc_rev);

	iounmap(reg_base);

res_ioremap:
	if (is_pcie_id == true)
		clk_disable_unprepare(clk);

clk_err:
	if (is_pcie_id == true)
		of_node_put(child);
	of_node_put(np);

	return ret;
}
core_initcall(mvebu_soc_id_init);
