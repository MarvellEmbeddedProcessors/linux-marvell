/*
 * Marvell Msys pinctrl driver based on mvebu pinctrl core
 *
 * Copyright (C) 2015 Marvell
 *
 * Marcin Wojtas <mw@semihalf.com>
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

#include "pinctrl-mvebu.h"

#define MVEBU_MPPS_PER_REG	8
#define MVEBU_MPP_BITS		4
#define MVEBU_MPP_MASK		0xf

static void __iomem *mpp_base;
static u32 *mpp_saved_regs;

static int msys_mpp_ctrl_get(unsigned pid, unsigned long *config)
{
	return default_mpp_ctrl_get(mpp_base, pid, config);
}

static int msys_mpp_ctrl_set(unsigned pid, unsigned long config)
{
	return default_mpp_ctrl_set(mpp_base, pid, config);
}

enum {
	V_AC3 = BIT(0),
	V_BC2 = BIT(1),
	V_BOBK = BIT(2),
	V_MSYS = (V_AC3 | V_BC2 | V_BOBK),
};

static struct mvebu_mpp_mode msys_mpp_modes[] = {
	MPP_MODE(0,
		 MPP_VAR_FUNCTION(0, "gpo",    NULL,      V_MSYS),
		 MPP_VAR_FUNCTION(2, "spi",    "mosi",    V_MSYS),
		 MPP_VAR_FUNCTION(4, "dev",    "ad8",     V_MSYS)),
	MPP_MODE(1,
		 MPP_VAR_FUNCTION(0, "gpio",   NULL,      V_MSYS),
		 MPP_VAR_FUNCTION(2, "spi",    "miso",    V_MSYS),
		 MPP_VAR_FUNCTION(4, "dev",    "ad9",     V_MSYS)),
	MPP_MODE(2,
		 MPP_VAR_FUNCTION(0, "gpo",    NULL,      V_MSYS),
		 MPP_VAR_FUNCTION(2, "spi",    "sck",     V_MSYS),
		 MPP_VAR_FUNCTION(4, "dev",    "ad10",    V_MSYS)),
	MPP_MODE(3,
		 MPP_VAR_FUNCTION(0, "gpio",   NULL,      V_MSYS),
		 MPP_VAR_FUNCTION(2, "spi",    "cs0",     V_MSYS),
		 MPP_VAR_FUNCTION(4, "dev",    "ad11",    V_MSYS)),
	MPP_MODE(4,
		 MPP_VAR_FUNCTION(0, "gpio",   NULL,      V_MSYS),
		 MPP_VAR_FUNCTION(2, "spi",    "cs1",     V_MSYS),
		 MPP_VAR_FUNCTION(3, "mstsmi", "mdc",     V_AC3 | V_BOBK),
		 MPP_VAR_FUNCTION(4, "dev",    "cs0",     V_MSYS),
		 MPP_VAR_FUNCTION(4, "nf",     "cen",     V_MSYS)),
	MPP_MODE(5,
		 MPP_VAR_FUNCTION(0, "gpio",   NULL,      V_MSYS),
		 MPP_VAR_FUNCTION(1, "pex",    "rstoutn", V_MSYS),
		 MPP_VAR_FUNCTION(2, "sdio",   "cmd",     V_BC2 | V_BOBK),
		 MPP_VAR_FUNCTION(4, "dev",    "bootcsn", V_MSYS)),
	MPP_MODE(6,
		 MPP_VAR_FUNCTION(0, "gpo",    NULL,      V_MSYS),
		 MPP_VAR_FUNCTION(2, "sdio",   "clk",     V_BC2 | V_BOBK),
		 MPP_VAR_FUNCTION(4, "dev",    "ad2",     V_MSYS)),
	MPP_MODE(7,
		 MPP_VAR_FUNCTION(0, "gpio",   NULL,      V_MSYS),
		 MPP_VAR_FUNCTION(2, "sdio",   "d0",      V_BC2 | V_BOBK),
		 MPP_VAR_FUNCTION(4, "dev",    "ale0",    V_MSYS)),
	MPP_MODE(8,
		 MPP_VAR_FUNCTION(0, "gpio",   NULL,      V_MSYS),
		 MPP_VAR_FUNCTION(2, "sdio",   "d1",      V_BC2 | V_BOBK),
		 MPP_VAR_FUNCTION(4, "dev",    "ale1",    V_MSYS)),
	MPP_MODE(9,
		 MPP_VAR_FUNCTION(0, "gpio",   NULL,      V_MSYS),
		 MPP_VAR_FUNCTION(2, "sdio",   "d2",      V_BC2 | V_BOBK),
		 MPP_VAR_FUNCTION(4, "dev",    "readyn",  V_MSYS)),
	MPP_MODE(10,
		 MPP_VAR_FUNCTION(0, "gpio",   NULL,      V_MSYS),
		 MPP_VAR_FUNCTION(2, "sdio",   "d3",      V_BC2 | V_BOBK),
		 MPP_VAR_FUNCTION(4, "dev",    "ad12",    V_MSYS)),
	MPP_MODE(11,
		 MPP_VAR_FUNCTION(0, "gpio",   NULL,      V_MSYS),
		 MPP_VAR_FUNCTION(2, "uart1",  "rxd",     V_MSYS),
		 MPP_VAR_FUNCTION(3, "uart0",  "cts",     V_MSYS),
		 MPP_VAR_FUNCTION(4, "dev",    "ad13",    V_MSYS)),
	MPP_MODE(12,
		 MPP_VAR_FUNCTION(0, "gpo",    NULL,      V_MSYS),
		 MPP_VAR_FUNCTION(2, "uart1",  "txd",     V_MSYS),
		 MPP_VAR_FUNCTION(3, "uart0",  "rts",     V_MSYS),
		 MPP_VAR_FUNCTION(4, "dev",    "ad14",    V_MSYS)),
	MPP_MODE(13,
		 MPP_VAR_FUNCTION(0, "gpio",   NULL,      V_MSYS),
		 MPP_VAR_FUNCTION(1, "pp",     "intoutn", V_MSYS),
		 MPP_VAR_FUNCTION(2, "i2c1",   "sck",     V_BOBK),
		 MPP_VAR_FUNCTION(4, "dev",    "ad15",    V_MSYS)),
	MPP_MODE(14,
		 MPP_VAR_FUNCTION(0, "gpio",   NULL,      V_MSYS),
		 MPP_VAR_FUNCTION(1, "i2c0",   "sck",     V_MSYS)),
	MPP_MODE(15,
		 MPP_VAR_FUNCTION(0, "gpio",   NULL,      V_MSYS),
		 MPP_VAR_FUNCTION(1, "i2c0",   "sda",     V_MSYS)),
	MPP_MODE(16,
		 MPP_VAR_FUNCTION(0, "gpio",   NULL,      V_MSYS),
		 MPP_VAR_FUNCTION(4, "dev",    "oen",     V_MSYS),
		 MPP_VAR_FUNCTION(4, "nf",     "ren",     V_MSYS)),
	MPP_MODE(17,
		 MPP_VAR_FUNCTION(0, "gpio",   NULL,      V_MSYS),
		 MPP_VAR_FUNCTION(4, "dev",    "clkout",  V_MSYS)),
	MPP_MODE(18,
		 MPP_VAR_FUNCTION(0, "gpio",   NULL,      V_MSYS),
		 MPP_VAR_FUNCTION(2, "i2c1",   "sda",     V_BOBK),
		 MPP_VAR_FUNCTION(3, "uart1",  "txd",     V_MSYS)),
	MPP_MODE(19,
		 MPP_VAR_FUNCTION(0, "gpio",   NULL,      V_MSYS),
		 MPP_VAR_FUNCTION(3, "uart1",  "rxd",     V_MSYS),
		 MPP_VAR_FUNCTION(4, "nf",     "rbn",     V_MSYS)),
	MPP_MODE(20,
		 MPP_VAR_FUNCTION(0, "gpio",   NULL,      V_MSYS),
		 MPP_VAR_FUNCTION(4, "dev",    "wen0",    V_MSYS),
		 MPP_VAR_FUNCTION(4, "nf",     "wen",     V_MSYS)),
	MPP_MODE(21,
		 MPP_VAR_FUNCTION(0, "gpio",   NULL,      V_MSYS),
		 MPP_VAR_FUNCTION(4, "dev",    "ad0",     V_MSYS),
		 MPP_VAR_FUNCTION(4, "nf",     "io0",     V_MSYS)),
	MPP_MODE(22,
		 MPP_VAR_FUNCTION(0, "gpio",   NULL,      V_MSYS),
		 MPP_VAR_FUNCTION(4, "dev",    "ad1",     V_MSYS),
		 MPP_VAR_FUNCTION(4, "nf",     "io1",     V_MSYS)),
	MPP_MODE(23,
		 MPP_VAR_FUNCTION(0, "gpio",   NULL,      V_MSYS),
		 MPP_VAR_FUNCTION(4, "dev",    "ad2",     V_MSYS),
		 MPP_VAR_FUNCTION(4, "nf",     "io2",     V_MSYS)),
	MPP_MODE(24,
		 MPP_VAR_FUNCTION(0, "gpio",   NULL,      V_MSYS),
		 MPP_VAR_FUNCTION(4, "dev",    "ad3",     V_MSYS),
		 MPP_VAR_FUNCTION(4, "nf",     "io3",     V_MSYS)),
	MPP_MODE(25,
		 MPP_VAR_FUNCTION(0, "gpio",   NULL,      V_MSYS),
		 MPP_VAR_FUNCTION(4, "dev",    "ad4",     V_MSYS),
		 MPP_VAR_FUNCTION(4, "nf",     "io4",     V_MSYS)),
	MPP_MODE(26,
		 MPP_VAR_FUNCTION(0, "gpio",   NULL,      V_MSYS),
		 MPP_VAR_FUNCTION(4, "dev",    "ad5",     V_MSYS),
		 MPP_VAR_FUNCTION(4, "nf",     "io5",     V_MSYS)),
	MPP_MODE(27,
		 MPP_VAR_FUNCTION(0, "gpio",   NULL,      V_MSYS),
		 MPP_VAR_FUNCTION(4, "dev",    "ad6",     V_MSYS),
		 MPP_VAR_FUNCTION(4, "nf",     "io6",     V_MSYS)),
	MPP_MODE(28,
		 MPP_VAR_FUNCTION(0, "gpio",   NULL,      V_MSYS),
		 MPP_VAR_FUNCTION(4, "dev",    "ad7",     V_MSYS),
		 MPP_VAR_FUNCTION(4, "nf",     "io7",     V_MSYS)),
	MPP_MODE(29,
		 MPP_VAR_FUNCTION(0, "gpio",   NULL,      V_MSYS),
		 MPP_VAR_FUNCTION(4, "dev",    "a0",      V_MSYS),
		 MPP_VAR_FUNCTION(4, "nf",     "cle",     V_MSYS)),
	MPP_MODE(30,
		 MPP_VAR_FUNCTION(0, "gpio",   NULL,      V_MSYS),
		 MPP_VAR_FUNCTION(4, "dev",    "a1",      V_MSYS),
		 MPP_VAR_FUNCTION(4, "nf",     "ale",     V_MSYS)),
	MPP_MODE(31,
		 MPP_VAR_FUNCTION(0, "gpio",   NULL,      V_MSYS),
		 MPP_VAR_FUNCTION(1, "slvsmi", "mdc",     V_MSYS),
		 MPP_VAR_FUNCTION(3, "mstsmi", "mdc",     V_MSYS),
		 MPP_VAR_FUNCTION(4, "dev",    "wen1",    V_MSYS)),
	MPP_MODE(32,
		 MPP_VAR_FUNCTION(0, "gpio",   NULL,      V_MSYS),
		 MPP_VAR_FUNCTION(1, "slvsmi", "mdio",    V_MSYS),
		 MPP_VAR_FUNCTION(3, "mstsmi", "mdio",    V_MSYS),
		 MPP_VAR_FUNCTION(4, "dev",    "cs1",     V_MSYS)),
};

static struct mvebu_pinctrl_soc_info msys_pinctrl_info;

static const struct of_device_id msys_pinctrl_of_match[] = {
	{
		.compatible = "marvell,ac3-pinctrl",
		.data       = (void *) V_AC3,
	},
	{
		.compatible = "marvell,bc2-pinctrl",
		.data       = (void *) V_BC2,
	},
	{
		.compatible = "marvell,bobk-pinctrl",
		.data       = (void *) V_BOBK,
	},
	{ },
};

static struct mvebu_mpp_ctrl msys_mpp_controls[] = {
	MPP_FUNC_CTRL(0, 32, NULL, msys_mpp_ctrl),
};

static struct pinctrl_gpio_range msys_mpp_gpio_ranges[] = {
	MPP_GPIO_RANGE(0,   0,  0, 32),
	MPP_GPIO_RANGE(1,  32, 32, 1),
};

int msys_pinctrl_suspend(struct platform_device *pdev, pm_message_t state)
{
	struct mvebu_pinctrl_soc_info *soc = platform_get_drvdata(pdev);
	int i, nregs;

	nregs = DIV_ROUND_UP(soc->nmodes, MVEBU_MPPS_PER_REG);

	for (i = 0; i < nregs; i++)
		mpp_saved_regs[i] = readl(mpp_base + i * 4);

	return 0;
}

int msys_pinctrl_resume(struct platform_device *pdev)
{
	struct mvebu_pinctrl_soc_info *soc = platform_get_drvdata(pdev);
	int i, nregs;

	nregs = DIV_ROUND_UP(soc->nmodes, MVEBU_MPPS_PER_REG);

	for (i = 0; i < nregs; i++)
		writel(mpp_saved_regs[i], mpp_base + i * 4);

	return 0;
}

static int msys_pinctrl_probe(struct platform_device *pdev)
{
	struct mvebu_pinctrl_soc_info *soc = &msys_pinctrl_info;
	const struct of_device_id *match =
			     of_match_device(msys_pinctrl_of_match, &pdev->dev);
	struct resource *res;
	int nregs;

	if (!match)
		return -ENODEV;

	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	mpp_base = devm_ioremap_resource(&pdev->dev, res);
	if (IS_ERR(mpp_base))
		return PTR_ERR(mpp_base);

	soc->variant = (unsigned) match->data & 0xff;
	soc->controls = msys_mpp_controls;
	soc->ncontrols = ARRAY_SIZE(msys_mpp_controls);
	soc->gpioranges = msys_mpp_gpio_ranges;
	soc->ngpioranges = ARRAY_SIZE(msys_mpp_gpio_ranges);
	soc->modes = msys_mpp_modes;
	soc->nmodes = msys_mpp_controls[0].npins;

	nregs = DIV_ROUND_UP(soc->nmodes, MVEBU_MPPS_PER_REG);

	mpp_saved_regs = devm_kmalloc(&pdev->dev, nregs * sizeof(u32),
				      GFP_KERNEL);
	if (!mpp_saved_regs)
		return -ENOMEM;

	pdev->dev.platform_data = soc;

	return mvebu_pinctrl_probe(pdev);
}

static int msys_pinctrl_remove(struct platform_device *pdev)
{
	return mvebu_pinctrl_remove(pdev);
}

static struct platform_driver msys_pinctrl_driver = {
	.driver = {
		.name = "msys-pinctrl",
		.of_match_table = of_match_ptr(msys_pinctrl_of_match),
	},
	.probe = msys_pinctrl_probe,
	.remove = msys_pinctrl_remove,
#ifdef CONFIG_PM
	.suspend = msys_pinctrl_suspend,
	.resume = msys_pinctrl_resume,
#endif

};

module_platform_driver(msys_pinctrl_driver);

MODULE_AUTHOR("Marcin Wojtas <mw@semihalf.com>");
MODULE_DESCRIPTION("Marvell Msys pinctrl driver");
MODULE_LICENSE("GPL v2");
