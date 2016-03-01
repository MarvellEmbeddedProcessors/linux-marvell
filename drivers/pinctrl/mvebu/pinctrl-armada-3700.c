/*
 * Marvell Armada 3700 pinctrl driver based on mvebu pinctrl core
 *
 * Copyright (C) 2016 Marvell
 *
 * Terry Zhou <bjzhou@marvell.com>
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
#include <linux/clk.h>
#include <linux/of.h>
#include <linux/of_device.h>
#include <linux/pinctrl/pinctrl.h>

#include "pinctrl-mvebu.h"

#define MVEBU_MPPS_PER_REG	32
#define MVEBU_MPP_BITS		1
#define MVEBU_MPP_MASK		0x1

enum armada_3700_bridge {
	I_NORTHBRIDGE	= 0,
	I_SOUTHBRIDGE,
	I_MAXCONTROLLER,
};

static void __iomem *mpp_base[I_MAXCONTROLLER];/* north & south bridge mpp base*/

static int armada_3700_nb_mpp_ctrl_get(unsigned pid, unsigned long *config)
{
	return default_mpp_ctrl_get(mpp_base[I_NORTHBRIDGE], pid, config);
}

static int armada_3700_nb_mpp_ctrl_set(unsigned pid, unsigned long config)
{
	return default_mpp_ctrl_set(mpp_base[I_NORTHBRIDGE], pid, config);
}

static int armada_3700_sb_mpp_ctrl_get(unsigned pid, unsigned long *config)
{
	return default_mpp_ctrl_get(mpp_base[I_SOUTHBRIDGE], pid, config);
}

static int armada_3700_sb_mpp_ctrl_set(unsigned pid, unsigned long config)
{
	return default_mpp_ctrl_set(mpp_base[I_SOUTHBRIDGE], pid, config);
}

static struct mvebu_mpp_mode armada_3700_nb_mpp_modes[] = {
	MPP_MODE(0,
	   MPP_FUNCTION(0x0, "jtag", NULL),
	   MPP_FUNCTION(0x1, "gpio", NULL)),
	MPP_MODE(1,
	   MPP_FUNCTION(0x0, "sdio", NULL),
	   MPP_FUNCTION(0x1, "gpio", NULL)),
	MPP_MODE(2,
	   MPP_FUNCTION(0x0, "mmc", NULL),
	   MPP_FUNCTION(0x1, "gpio", NULL)),
	MPP_MODE(3,
	   MPP_FUNCTION(0x0, "pwm0", NULL),
	   MPP_FUNCTION(0x1, "gpio", NULL)),
	MPP_MODE(4,
	   MPP_FUNCTION(0x0, "pwm1", NULL),
	   MPP_FUNCTION(0x1, "gpio", NULL)),
	MPP_MODE(5,
	   MPP_FUNCTION(0x0, "pwm2", NULL),
	   MPP_FUNCTION(0x1, "gpio", NULL)),
	MPP_MODE(6,
	   MPP_FUNCTION(0x0, "pwm3", NULL),
	   MPP_FUNCTION(0x1, "gpio", NULL)),
	MPP_MODE(7,
	   MPP_FUNCTION(0x0, "pmic1", NULL),
	   MPP_FUNCTION(0x1, "gpio", NULL)),
	MPP_MODE(8,
	   MPP_FUNCTION(0x0, "pmic0", NULL),
	   MPP_FUNCTION(0x1, "gpio", NULL)),
	MPP_MODE(9,
	   MPP_FUNCTION(0x0, "i2c2", NULL),
	   MPP_FUNCTION(0x1, "gpio", NULL)),
	MPP_MODE(10,
	   MPP_FUNCTION(0x0, "i2c1", NULL),
	   MPP_FUNCTION(0x1, "gpio", NULL)),
	MPP_MODE(11,
	   MPP_FUNCTION(0x0, "fcs0", NULL),
	   MPP_FUNCTION(0x1, "gpio", NULL)),
	MPP_MODE(12,
	   MPP_FUNCTION(0x0, "fcs1", NULL),
	   MPP_FUNCTION(0x1, "gpio", NULL)),
	MPP_MODE(13,
	   MPP_FUNCTION(0x0, "fcs2", NULL),
	   MPP_FUNCTION(0x1, "gpio", NULL)),
	MPP_MODE(14,
	   MPP_FUNCTION(0x0, "fcs3", NULL),
	   MPP_FUNCTION(0x1, "gpio", NULL)),
	MPP_MODE(15,
	   MPP_FUNCTION(0x0, "spi", NULL),
	   MPP_FUNCTION(0x1, "gpio", NULL)),
	MPP_MODE(16,
	   MPP_FUNCTION(0x0, "1wire", NULL),
	   MPP_FUNCTION(0x1, "gpio", NULL)),
	MPP_MODE(17,
	   MPP_FUNCTION(0x0, "uart1", NULL),
	   MPP_FUNCTION(0x1, "gpio", NULL)),
	MPP_MODE(18,
	   MPP_FUNCTION(0x0, "spi", "quad"),
	   MPP_FUNCTION(0x1, "gpio", NULL)),
	MPP_MODE(19,
	   MPP_FUNCTION(0x0, "spi", "cs"),
	   MPP_FUNCTION(0x1, "uart2", NULL)),
	MPP_MODE(20,
	   MPP_FUNCTION(0x0, "led", "led0"),
	   MPP_FUNCTION(0x1, "pulllow", NULL)),
	MPP_MODE(21,
	   MPP_FUNCTION(0x0, "led", "led1"),
	   MPP_FUNCTION(0x1, "pulllow", NULL)),
	MPP_MODE(22,
	   MPP_FUNCTION(0x0, "led", "led2"),
	   MPP_FUNCTION(0x1, "pulllow", NULL)),
	MPP_MODE(23,
	   MPP_FUNCTION(0x0, "led", "led3"),
	   MPP_FUNCTION(0x1, "pulllow", NULL)),
};

static struct mvebu_mpp_mode armada_3700_sb_mpp_modes[] = {
	MPP_MODE(0,
	   MPP_FUNCTION(0x0, "usb32", NULL),
	   MPP_FUNCTION(0x1, "gpio", NULL)),
	MPP_MODE(1,
	   MPP_FUNCTION(0x0, "usb2", NULL),
	   MPP_FUNCTION(0x1, "gpio", NULL)),
	MPP_MODE(2,
	   MPP_FUNCTION(0x0, "sdio", NULL),
	   MPP_FUNCTION(0x1, "gpio", NULL)),
	MPP_MODE(3,
	   MPP_FUNCTION(0x0, "mii", NULL),
	   MPP_FUNCTION(0x1, "gpio", NULL)),
	MPP_MODE(4,
	   MPP_FUNCTION(0x0, "pcie1", NULL),
	   MPP_FUNCTION(0x1, "gpio", NULL)),
	MPP_MODE(5,
	   MPP_FUNCTION(0x0, "ptp", NULL),
	   MPP_FUNCTION(0x1, "gpio", NULL)),
	MPP_MODE(6,
	   MPP_FUNCTION(0x0, "ptp", "clk"),
	   MPP_FUNCTION(0x1, "mii", "txerr")),
	MPP_MODE(7,
	   MPP_FUNCTION(0x0, "ptp", "trig"),
	   MPP_FUNCTION(0x1, "mii", "carriersense")),
	MPP_MODE(8,
	   MPP_FUNCTION(0x0, "collision", NULL),
	   MPP_FUNCTION(0x1, "mii", "txerr")),
};

static struct mvebu_pinctrl_soc_info armada_3700_pinctrl_info[I_MAXCONTROLLER];

static const struct of_device_id armada_3700_pinctrl_of_match[] = {
	{
		.compatible = "marvell,armada-3700-nb-pinctrl",
		.data       = (void *) I_NORTHBRIDGE
	},
	{
		.compatible = "marvell,armada-3700-sb-pinctrl",
		.data       = (void *) I_SOUTHBRIDGE,
	},
	{ },
};

static struct mvebu_mpp_ctrl armada_3700_nb_mpp_controls[] = {
	MPP_FUNC_CTRL(0, 23, NULL, armada_3700_nb_mpp_ctrl),
};

static struct mvebu_mpp_ctrl armada_3700_sb_mpp_controls[] = {
	MPP_FUNC_CTRL(0, 8, NULL, armada_3700_sb_mpp_ctrl),
};

static struct pinctrl_gpio_range armada_3700_nb_mpp_gpio_ranges[] = {
	MPP_GPIO_RANGE(0,   0,  0, 24),
};

static struct pinctrl_gpio_range armada_3700_sb_mpp_gpio_ranges[] = {
	MPP_GPIO_RANGE(0,   0,  0, 9),
};


static int armada_3700_pinctrl_probe(struct platform_device *pdev)
{
	const struct of_device_id *match =
		of_match_device(armada_3700_pinctrl_of_match, &pdev->dev);
	unsigned index;
	struct mvebu_pinctrl_soc_info *soc;
	struct resource *res;

	if (!match)
		return -ENODEV;

	index = (unsigned) match->data;
	if (index > I_MAXCONTROLLER) {
		dev_err(&pdev->dev, "controller index error, index=%d max=%d\n", index, I_MAXCONTROLLER);
		return -ENODEV;
	}

	/* armada3700 need to set mpps_per_reg */
	mvebu_pinctrl_set_mpps(MVEBU_MPPS_PER_REG);

	soc = &armada_3700_pinctrl_info[index];
	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	mpp_base[index] = devm_ioremap_resource(&pdev->dev, res);
	if (IS_ERR(mpp_base[index]))
		return PTR_ERR(mpp_base[index]);

	soc->variant = 0; /* no variants for Armada 3700 */
	if (index == I_NORTHBRIDGE) {
		soc->controls = armada_3700_nb_mpp_controls;
		soc->ncontrols = ARRAY_SIZE(armada_3700_nb_mpp_controls);
		soc->modes = armada_3700_nb_mpp_modes;
		soc->nmodes = ARRAY_SIZE(armada_3700_nb_mpp_modes);
		soc->gpioranges = armada_3700_nb_mpp_gpio_ranges;
		soc->ngpioranges = ARRAY_SIZE(armada_3700_nb_mpp_gpio_ranges);
	} else if (index == I_SOUTHBRIDGE) {
		soc->controls = armada_3700_sb_mpp_controls;
		soc->ncontrols = ARRAY_SIZE(armada_3700_sb_mpp_controls);
		soc->modes = armada_3700_sb_mpp_modes;
		soc->nmodes = ARRAY_SIZE(armada_3700_sb_mpp_modes);
		soc->gpioranges = armada_3700_sb_mpp_gpio_ranges;
		soc->ngpioranges = ARRAY_SIZE(armada_3700_sb_mpp_gpio_ranges);
	}

	pdev->dev.platform_data = soc;

	return mvebu_pinctrl_probe(pdev);
}

static int armada_3700_pinctrl_remove(struct platform_device *pdev)
{
	return mvebu_pinctrl_remove(pdev);
}

static struct platform_driver armada_3700_pinctrl_driver = {
	.driver = {
		.name = "armada-3700-pinctrl",
		.of_match_table = armada_3700_pinctrl_of_match,
	},
	.probe = armada_3700_pinctrl_probe,
	.remove = armada_3700_pinctrl_remove,
};

module_platform_driver(armada_3700_pinctrl_driver);

MODULE_AUTHOR("Terry Zhou <bjzhou@marvell.com>");
MODULE_DESCRIPTION("Marvell Armada 3700 pinctrl driver");
MODULE_LICENSE("GPL v2");
