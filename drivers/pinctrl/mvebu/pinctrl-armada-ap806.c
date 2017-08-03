/*
 * Marvell Armada ap806 pinctrl driver based on mvebu pinctrl core
 *
 * Copyright (C) 2015 Marvell
 *
 * Thomas Petazzoni <thomas.petazzoni@free-electrons.com>
 * Hanna Hawa <hannah@marvell.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#include <linux/err.h>
#include <linux/init.h>
#include <linux/io.h>
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/of.h>
#include <linux/of_device.h>
#include <linux/pinctrl/pinctrl.h>
#include <linux/of_address.h>

#include "pinctrl-mvebu.h"

#define EMMC_PHY_CTRL_SDPHY_EN	BIT(0)

static void __iomem *mpp_base;
static void __iomem *emmc_phy_ctrl_reg;

static int armada_ap806_mpp_ctrl_get(unsigned pid, unsigned long *config)
{
	return default_mpp_ctrl_get(mpp_base, pid, config);
}

static int armada_ap806_mpp_ctrl_set(unsigned pid, unsigned long config)
{
	/* To enable SDIO/eMMC in Armada-APN806, need to configure PHY mux.
	 * eMMC/SD PHY register responsible for muxing between MPPs and SD/eMMC
	 * controller:
	 * - Bit0 enabled SDIO/eMMC PHY is used as a MPP muxltiplexer,
	 * - Bit0 disabled SDIO/eMMC PHY is connected to SDIO/eMMC controller
	 * If pin function is set to eMMC/SD, then configure the eMMC/SD PHY
	 * muxltiplexer register to be on SDIO/eMMC controller
	 * If the MPP is configured to sdio, the pin0 as clk is mandatory,
	 * to avoid config eMMC/SD PHY register repeatly, it is ok to set the
	 * register only when MPP pin0(pid==0) is configured as "sdio"
	 * (config == 0x1).
	 */
	if (emmc_phy_ctrl_reg && pid == 0 && config == 0x1) {
		u32 reg;

		reg = readl(emmc_phy_ctrl_reg);
		reg &= ~EMMC_PHY_CTRL_SDPHY_EN;
		writel(reg, emmc_phy_ctrl_reg);
	}

	return default_mpp_ctrl_set(mpp_base, pid, config);
}

static struct mvebu_mpp_mode armada_ap806_mpp_modes[] = {
	MPP_MODE(0,
		 MPP_FUNCTION(0, "gpio",    NULL),
		 MPP_FUNCTION(1, "sdio",    "clk"),
		 MPP_FUNCTION(3, "spi0",    "clk")),
	MPP_MODE(1,
		 MPP_FUNCTION(0, "gpio",    NULL),
		 MPP_FUNCTION(1, "sdio",    "cmd"),
		 MPP_FUNCTION(3, "spi0",    "miso")),
	MPP_MODE(2,
		 MPP_FUNCTION(0, "gpio",    NULL),
		 MPP_FUNCTION(1, "sdio",    "d0"),
		 MPP_FUNCTION(3, "spi0",    "mosi")),
	MPP_MODE(3,
		 MPP_FUNCTION(0, "gpio",    NULL),
		 MPP_FUNCTION(1, "sdio",    "d1"),
		 MPP_FUNCTION(3, "spi0",    "cs0n")),
	MPP_MODE(4,
		 MPP_FUNCTION(0, "gpio",    NULL),
		 MPP_FUNCTION(1, "sdio",    "d2"),
		 MPP_FUNCTION(3, "i2c0",    "sda")),
	MPP_MODE(5,
		 MPP_FUNCTION(0, "gpio",    NULL),
		 MPP_FUNCTION(1, "sdio",    "d3"),
		 MPP_FUNCTION(3, "i2c0",    "sdk")),
	MPP_MODE(6,
		 MPP_FUNCTION(0, "gpio",    NULL),
		 MPP_FUNCTION(1, "sdio",    "ds")),
	MPP_MODE(7,
		 MPP_FUNCTION(0, "gpio",    NULL),
		 MPP_FUNCTION(1, "sdio",    "d4"),
		 MPP_FUNCTION(3, "uart1",   "rxd")),
	MPP_MODE(8,
		 MPP_FUNCTION(0, "gpio",    NULL),
		 MPP_FUNCTION(1, "sdio",    "d5"),
		 MPP_FUNCTION(3, "uart1",   "txd")),
	MPP_MODE(9,
		 MPP_FUNCTION(0, "gpio",    NULL),
		 MPP_FUNCTION(1, "sdio",    "d6"),
		 MPP_FUNCTION(3, "spi0",    "cs1n")),
	MPP_MODE(10,
		 MPP_FUNCTION(0, "gpio",    NULL),
		 MPP_FUNCTION(1, "sdio",    "d7")),
	MPP_MODE(11,
		 MPP_FUNCTION(0, "gpio",    NULL),
		 MPP_FUNCTION(3, "uart0",   "txd")),
	MPP_MODE(12,
		 MPP_FUNCTION(0, "gpio",    NULL),
		 MPP_FUNCTION(1, "sdio",    "pw_off"),
		 MPP_FUNCTION(2, "sdio",    "hw_rst")),
	MPP_MODE(13,
		 MPP_FUNCTION(0, "gpio",    NULL)),
	MPP_MODE(14,
		 MPP_FUNCTION(0, "gpio",    NULL)),
	MPP_MODE(15,
		 MPP_FUNCTION(0, "gpio",    NULL)),
	MPP_MODE(16,
		 MPP_FUNCTION(0, "gpio",    NULL)),
	MPP_MODE(17,
		 MPP_FUNCTION(0, "gpio",    NULL)),
	MPP_MODE(18,
		 MPP_FUNCTION(0, "gpio",    NULL)),
	MPP_MODE(19,
		 MPP_FUNCTION(0, "gpio",    NULL),
		 MPP_FUNCTION(3, "uart0",   "rxd"),
		 MPP_FUNCTION(4, "sdio",    "pw_off")),
};

static struct mvebu_pinctrl_soc_info armada_ap806_pinctrl_info;

static const struct of_device_id armada_ap806_pinctrl_of_match[] = {
	{
		.compatible = "marvell,ap806-pinctrl",
	},
	{ },
};

static struct mvebu_mpp_ctrl armada_ap806_mpp_controls[] = {
	MPP_FUNC_CTRL(0, 19, NULL, armada_ap806_mpp_ctrl),
};

static struct pinctrl_gpio_range armada_ap806_mpp_gpio_ranges[] = {
	MPP_GPIO_RANGE(0,   0,  0, 20),
};

static int armada_ap806_pinctrl_probe(struct platform_device *pdev)
{
	struct mvebu_pinctrl_soc_info *soc = &armada_ap806_pinctrl_info;
	const struct of_device_id *match =
		of_match_device(armada_ap806_pinctrl_of_match, &pdev->dev);
	struct resource *res;

	if (!match)
		return -ENODEV;

	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	mpp_base = devm_ioremap_resource(&pdev->dev, res);
	if (IS_ERR(mpp_base))
		return PTR_ERR(mpp_base);

	/* Get the eMMC PHY IO Control 0 Register base
	 * Usage of this reg will be required in case MMC is enabled.
	 */
	res = platform_get_resource_byname(pdev, IORESOURCE_MEM, "mmcio");
	if (res) {
		emmc_phy_ctrl_reg = devm_ioremap_resource(&pdev->dev, res);
		if (IS_ERR(emmc_phy_ctrl_reg))
			return PTR_ERR(emmc_phy_ctrl_reg);
	} else {
		dev_warn(&pdev->dev, "mmcio reg not present in DT\n");
	}

	soc->variant = 0; /* no variants for Armada AP806 */
	soc->controls = armada_ap806_mpp_controls;
	soc->ncontrols = ARRAY_SIZE(armada_ap806_mpp_controls);
	soc->gpioranges = armada_ap806_mpp_gpio_ranges;
	soc->ngpioranges = ARRAY_SIZE(armada_ap806_mpp_gpio_ranges);
	soc->modes = armada_ap806_mpp_modes;
	soc->nmodes = armada_ap806_mpp_controls[0].npins;

	pdev->dev.platform_data = soc;

	return mvebu_pinctrl_probe(pdev);
}

static int armada_ap806_pinctrl_remove(struct platform_device *pdev)
{
	return mvebu_pinctrl_remove(pdev);
}

static struct platform_driver armada_ap806_pinctrl_driver = {
	.driver = {
		.name = "armada-ap806-pinctrl",
		.of_match_table = of_match_ptr(armada_ap806_pinctrl_of_match),
	},
	.probe = armada_ap806_pinctrl_probe,
	.remove = armada_ap806_pinctrl_remove,
};

module_platform_driver(armada_ap806_pinctrl_driver);

MODULE_AUTHOR("Thomas Petazzoni <thomas.petazzoni@free-electrons.com>");
MODULE_DESCRIPTION("Marvell Armada ap806 pinctrl driver");
MODULE_LICENSE("GPL v2");
