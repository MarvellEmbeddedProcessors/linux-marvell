/*
 * SerDes Shutdown platform driver .
 *
 * Power down all related SerDes interfaces: SerDes Lanes, PCIe/USB3.0 Pipes, PCIe Ref Clocks, and PON SerDes
 *
 * Copyright (C) 2013 Marvell
 *
 * Omri Itach <omrii@marvell.com>
 *
 * This file is licensed under the terms of the GNU General Public
 * License version 2.  This program is licensed "as is" without any
 * warranty of any kind, whether express or implied.
 *
 */
#include <linux/of_platform.h>
#include <linux/io.h>
#include <linux/of_address.h>

static struct of_device_id of_ip_config_table[] = {
	{.compatible = "marvell,armada-375-ip-configuration"},
	{ /* end of list */ },
};

#define PCIE_REF_CLK_CTRL_REG		0xf0
#define REF_CLK_REG_PCIE0_POWERUP	BIT(0)
#define REF_CLK_REG_PCIE1_POWERUP	BIT(1)

#define GPON_PHY_CTRL0_REG		0xf4
#define GPON_CFG_REG_PLL		BIT(0)
#define GPON_CFG_REG_RX			BIT(1)
#define GPON_CFG_REG_TX			BIT(2)
#define GPON_CFG_REG_PLL_RX_TX_MASK	(GPON_CFG_REG_PLL | GPON_CFG_REG_RX | GPON_CFG_REG_TX)
#define GPON_CFG_REG_RESET_PIN		BIT(3)
#define GPON_CFG_REG_RESET_TX		BIT(4)
#define GPON_CFG_REG_RESET_CORE		BIT(5)
#define GPON_CFG_REG_RESET_MASK		(GPON_CFG_REG_RESET_PIN | GPON_CFG_REG_RESET_TX | GPON_CFG_REG_RESET_CORE)

/* IP configuration: Power down PCIe0/1 Ref clock & PON PHY */
static void mvebu_powerdown_ip_configuration(void)
{
	struct device_node *np = NULL;
	static void __iomem *ip_base;
	u32 ip_cfg;

	np = of_find_matching_node(NULL, of_ip_config_table);
	if (!np)
		return;

	ip_base = of_iomap(np, 0);
	BUG_ON(!ip_base);

	/* PCie0/1 Ref CLK */
	ip_cfg = readl(ip_base + PCIE_REF_CLK_CTRL_REG);
	ip_cfg &= ~(REF_CLK_REG_PCIE0_POWERUP | REF_CLK_REG_PCIE1_POWERUP);	/* Power down PCIe-0/1 Ref clocks */
	writel(ip_cfg, ip_base + PCIE_REF_CLK_CTRL_REG);

	/* PON PHY */
	ip_cfg = readl(ip_base + GPON_PHY_CTRL0_REG);
	ip_cfg &= ~GPON_CFG_REG_PLL_RX_TX_MASK;	/* Power down PLL, TX, RX */
	ip_cfg |= GPON_CFG_REG_RESET_MASK;	/* PHY Reset, TX reset, Reset Core */
	writel(ip_cfg, ip_base + GPON_PHY_CTRL0_REG);

	iounmap(ip_base);
}


static struct of_device_id of_serdes_pipe_config_table[] = {
	{.compatible = "marvell,armada-375-serdes-pipe-configuration"},
	{ /* end of list */ },
};

#define PIPE_PM_OVERRIDE		BIT(1)

/* Power down SerDes through Pipes (PCIe/USB3.0): PM-OVERRIDE */
static void mvebu_powerdown_pcie_usb_serdes(int phy_number)
{
	struct device_node *np = NULL;
	static void __iomem *pipe_base;

	/* find pipe configuration node for requested PHY*/
	np = of_find_matching_node(np, of_serdes_pipe_config_table);
	if (!np)
		return;

	pipe_base = of_iomap(np, phy_number);
	BUG_ON(!pipe_base);

	writel((readl(pipe_base) | PIPE_PM_OVERRIDE) , pipe_base);
	iounmap(pipe_base);
}


static struct of_device_id of_phy_config_table[] = {
	{.compatible = "marvell,armada-375-common-phy-configuration"},
	{ /* end of list */ },
};

#define COMMON_PHY_CFG_REG(phy_num)	(0x4 * phy_num)
#define PHY_CFG_REG_POWER_UP_IVREF	BIT(1)
#define PHY_CFG_REG_PIPE_SELECT		BIT(2)
#define PHY_CFG_REG_POWER_UP_PLL	BIT(16)
#define PHY_CFG_REG_POWER_UP_TX		BIT(17)
#define PHY_CFG_REG_POWER_UP_RX		BIT(18)
#define PHY_CFG_REG_POWER_UP_MASK	(PHY_CFG_REG_POWER_UP_IVREF | PHY_CFG_REG_POWER_UP_PLL |\
					PHY_CFG_REG_POWER_UP_TX | PHY_CFG_REG_POWER_UP_RX)

/* Power down SerDes Lanes & relevant Pipes: COMMON-PHY configuration */
static void mvebu_powerdown_serdes_lanes(void)
{
	struct device_node *np = NULL;
	static void __iomem *phy_cfg_base;
	u32 phy_cfg, phy_count, i;

	np = of_find_matching_node(NULL, of_phy_config_table);
	if (!np)
		return;

	phy_cfg_base = of_iomap(np, 0);
	BUG_ON(!phy_cfg_base);
	of_property_read_u32(np, "phy-count", &phy_count);

	for (i = 0; i < phy_count; i++) {
		phy_cfg = readl(phy_cfg_base + COMMON_PHY_CFG_REG(i));

		/* if Lane is Piped to USB3.0 / PCIe */
		if (phy_cfg & PHY_CFG_REG_PIPE_SELECT)
			mvebu_powerdown_pcie_usb_serdes(i);

		phy_cfg &= ~PHY_CFG_REG_POWER_UP_MASK; /* Power down: IVREF, PLL, RX, TX */
		writel(phy_cfg, phy_cfg_base + COMMON_PHY_CFG_REG(i));
	}
	iounmap(phy_cfg_base);
}

void mvebu_serdes_shutdown(struct platform_device *pdev)
{
	mvebu_powerdown_serdes_lanes();
	mvebu_powerdown_ip_configuration();
}

static struct platform_driver mvebu_serdes_driver = {
	.shutdown = mvebu_serdes_shutdown,
	.driver = {
		.name = "common-phy-configuration",
		.owner	= THIS_MODULE,
		.of_match_table = of_match_ptr(of_phy_config_table),
	},
};

static int __init mvebu_serdes_init(void)
{
	/* if common-phy node exists, register driver (needed for SerDes configuration) */
	if (!of_find_matching_node(NULL, of_phy_config_table))
		return 0;

	return platform_driver_register(&mvebu_serdes_driver);
}

static void __exit mvebu_serdes_exit(void)
{
	platform_driver_unregister(&mvebu_serdes_driver);
}

module_init(mvebu_serdes_init);
module_exit(mvebu_serdes_exit);
