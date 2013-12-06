/*
 * Marvell Armada 380/385 pinctrl driver based on mvebu pinctrl core
 *
 * Copyright (C) 2013 Marvell
 *
 * Thomas Petazzoni <thomas.petazzoni@free-electrons.com>
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

enum {
	V_88F6810,
	V_88F6820,
};

static struct mvebu_mpp_mode armada_38x_mpp_modes[] = {
	/*
	 * TODO: fill the right functions once we have access to the
	 * HW datasheet.
	 */
	MPP_MODE(0,
		 MPP_FUNCTION(0x0, "gpio", NULL)),
	MPP_MODE(1,
		 MPP_FUNCTION(0x0, "gpio", NULL)),
	MPP_MODE(2,
		 MPP_FUNCTION(0x0, "gpio", NULL)),
	MPP_MODE(3,
		 MPP_FUNCTION(0x0, "gpio", NULL)),
	MPP_MODE(4,
		 MPP_FUNCTION(0x0, "gpio", NULL)),
	MPP_MODE(5,
		 MPP_FUNCTION(0x0, "gpio", NULL)),
	MPP_MODE(6,
		 MPP_FUNCTION(0x0, "gpio", NULL)),
	MPP_MODE(7,
		 MPP_FUNCTION(0x0, "gpio", NULL)),
	MPP_MODE(8,
		 MPP_FUNCTION(0x0, "gpio", NULL)),
	MPP_MODE(9,
		 MPP_FUNCTION(0x0, "gpio", NULL)),
	MPP_MODE(10,
		 MPP_FUNCTION(0x0, "gpio", NULL)),
	MPP_MODE(11,
		 MPP_FUNCTION(0x0, "gpio", NULL)),
	MPP_MODE(12,
		 MPP_FUNCTION(0x0, "gpio", NULL)),
	MPP_MODE(13,
		 MPP_FUNCTION(0x0, "gpio", NULL)),
	MPP_MODE(14,
		 MPP_FUNCTION(0x0, "gpio", NULL)),
	MPP_MODE(15,
		 MPP_FUNCTION(0x0, "gpio", NULL)),
	MPP_MODE(16,
		 MPP_FUNCTION(0x0, "gpio", NULL)),
	MPP_MODE(17,
		 MPP_FUNCTION(0x0, "gpio", NULL)),
	MPP_MODE(18,
		 MPP_FUNCTION(0x0, "gpio", NULL)),
	MPP_MODE(19,
		 MPP_FUNCTION(0x0, "gpio", NULL)),
	MPP_MODE(20,
		 MPP_FUNCTION(0x0, "gpio", NULL)),
	MPP_MODE(21,
		 MPP_FUNCTION(0x0, "gpio", NULL)),
	MPP_MODE(22,
		 MPP_FUNCTION(0x0, "gpio", NULL)),
	MPP_MODE(23,
		 MPP_FUNCTION(0x0, "gpio", NULL)),
	MPP_MODE(24,
		 MPP_FUNCTION(0x0, "gpio", NULL)),
	MPP_MODE(25,
		 MPP_FUNCTION(0x0, "gpio", NULL)),
	MPP_MODE(26,
		 MPP_FUNCTION(0x0, "gpio", NULL)),
	MPP_MODE(27,
		 MPP_FUNCTION(0x0, "gpio", NULL)),
	MPP_MODE(28,
		 MPP_FUNCTION(0x0, "gpio", NULL)),
	MPP_MODE(29,
		 MPP_FUNCTION(0x0, "gpio", NULL)),
	MPP_MODE(30,
		 MPP_FUNCTION(0x0, "gpio", NULL)),
	MPP_MODE(31,
		 MPP_FUNCTION(0x0, "gpio", NULL)),
	MPP_MODE(32,
		 MPP_FUNCTION(0x0, "gpio", NULL)),
	MPP_MODE(33,
		 MPP_FUNCTION(0x0, "gpio", NULL)),
	MPP_MODE(34,
		 MPP_FUNCTION(0x0, "gpio", NULL)),
	MPP_MODE(35,
		 MPP_FUNCTION(0x0, "gpio", NULL)),
	MPP_MODE(36,
		 MPP_FUNCTION(0x0, "gpio", NULL)),
	MPP_MODE(37,
		 MPP_FUNCTION(0x0, "gpio", NULL)),
	MPP_MODE(38,
		 MPP_FUNCTION(0x0, "gpio", NULL)),
	MPP_MODE(39,
		 MPP_FUNCTION(0x0, "gpio", NULL)),
	MPP_MODE(40,
		 MPP_FUNCTION(0x0, "gpio", NULL)),
	MPP_MODE(41,
		 MPP_FUNCTION(0x0, "gpio", NULL)),
	MPP_MODE(42,
		 MPP_FUNCTION(0x0, "gpio", NULL)),
	MPP_MODE(43,
		 MPP_FUNCTION(0x0, "gpio", NULL)),
	MPP_MODE(44,
		 MPP_FUNCTION(0x0, "gpio", NULL)),
	MPP_MODE(45,
		 MPP_FUNCTION(0x0, "gpio", NULL)),
	MPP_MODE(46,
		 MPP_FUNCTION(0x0, "gpio", NULL)),
	MPP_MODE(47,
		 MPP_FUNCTION(0x0, "gpio", NULL)),
	MPP_MODE(48,
		 MPP_FUNCTION(0x0, "gpio", NULL)),
	MPP_MODE(49,
		 MPP_FUNCTION(0x0, "gpio", NULL)),
	MPP_MODE(50,
		 MPP_FUNCTION(0x0, "gpio", NULL)),
	MPP_MODE(51,
		 MPP_FUNCTION(0x0, "gpio", NULL)),
	MPP_MODE(52,
		 MPP_FUNCTION(0x0, "gpio", NULL)),
	MPP_MODE(53,
		 MPP_FUNCTION(0x0, "gpio", NULL)),
	MPP_MODE(54,
		 MPP_FUNCTION(0x0, "gpio", NULL)),
	MPP_MODE(55,
		 MPP_FUNCTION(0x0, "gpio", NULL)),
	MPP_MODE(56,
		 MPP_FUNCTION(0x0, "gpio", NULL)),
	MPP_MODE(57,
		 MPP_FUNCTION(0x0, "gpio", NULL)),
	MPP_MODE(58,
		 MPP_FUNCTION(0x0, "gpio", NULL)),
	MPP_MODE(59,
		 MPP_FUNCTION(0x0, "gpio", NULL)),
};

static struct mvebu_pinctrl_soc_info armada_38x_pinctrl_info;

static struct of_device_id armada_38x_pinctrl_of_match[] = {
	{
		.compatible = "marvell,mv88f6810-pinctrl",
		.data       = (void *) V_88F6810,
	},
	{
		.compatible = "marvell,mv88f6820-pinctrl",
		.data       = (void *) V_88F6820,
	},
	{ },
};

static struct mvebu_mpp_ctrl mv88f6810_mpp_controls[] = {
	MPP_REG_CTRL(0, 59),
};

static struct pinctrl_gpio_range mv88f6810_mpp_gpio_ranges[] = {
	MPP_GPIO_RANGE(0,   0,  0, 32),
	MPP_GPIO_RANGE(1,  32, 32, 27),
};

static struct mvebu_mpp_ctrl mv88f6820_mpp_controls[] = {
	MPP_REG_CTRL(0, 59),
};

static struct pinctrl_gpio_range mv88f6820_mpp_gpio_ranges[] = {
	MPP_GPIO_RANGE(0,   0,  0, 32),
	MPP_GPIO_RANGE(1,  32, 32, 27),
};

static int armada_38x_pinctrl_probe(struct platform_device *pdev)
{
	struct mvebu_pinctrl_soc_info *soc = &armada_38x_pinctrl_info;
	const struct of_device_id *match =
		of_match_device(armada_38x_pinctrl_of_match, &pdev->dev);

	if (!match)
		return -ENODEV;

	soc->variant = (unsigned) match->data & 0xff;

	switch (soc->variant) {
	case V_88F6810:
		soc->controls = mv88f6810_mpp_controls;
		soc->ncontrols = ARRAY_SIZE(mv88f6810_mpp_controls);
		soc->modes = armada_38x_mpp_modes;
		soc->nmodes = mv88f6810_mpp_controls[0].npins;
		soc->gpioranges = mv88f6810_mpp_gpio_ranges;
		soc->ngpioranges = ARRAY_SIZE(mv88f6810_mpp_gpio_ranges);
		break;
	case V_88F6820:
		soc->controls = mv88f6820_mpp_controls;
		soc->ncontrols = ARRAY_SIZE(mv88f6820_mpp_controls);
		soc->modes = armada_38x_mpp_modes;
		soc->nmodes = mv88f6820_mpp_controls[0].npins;
		soc->gpioranges = mv88f6820_mpp_gpio_ranges;
		soc->ngpioranges = ARRAY_SIZE(mv88f6820_mpp_gpio_ranges);
		break;
	}

	pdev->dev.platform_data = soc;

	return mvebu_pinctrl_probe(pdev);
}

static int armada_38x_pinctrl_remove(struct platform_device *pdev)
{
	return mvebu_pinctrl_remove(pdev);
}

static struct platform_driver armada_38x_pinctrl_driver = {
	.driver = {
		.name = "armada-38x-pinctrl",
		.owner = THIS_MODULE,
		.of_match_table = of_match_ptr(armada_38x_pinctrl_of_match),
	},
	.probe = armada_38x_pinctrl_probe,
	.remove = armada_38x_pinctrl_remove,
};

module_platform_driver(armada_38x_pinctrl_driver);

MODULE_AUTHOR("Thomas Petazzoni <thomas.petazzoni@free-electrons.com>");
MODULE_DESCRIPTION("Marvell Armada 38x pinctrl driver");
MODULE_LICENSE("GPL v2");
