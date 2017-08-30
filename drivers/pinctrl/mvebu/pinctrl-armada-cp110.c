/*
 * Marvell Armada CP110 pinctrl driver based on mvebu pinctrl core
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
#include <linux/syscore_ops.h>

#include "pinctrl-mvebu.h"

/* Global list of devices (struct mvebu_pinctrl_soc_info) */
static LIST_HEAD(drvdata_list);

static void __iomem *cp0_mpp_base;

static int armada_cp110_0_mpp_ctrl_get(unsigned pid, unsigned long *config)
{
	return default_mpp_ctrl_get(cp0_mpp_base, pid, config);
}

static int armada_cp110_0_mpp_ctrl_set(unsigned pid, unsigned long config)
{
	return default_mpp_ctrl_set(cp0_mpp_base, pid, config);
}

static void __iomem *cp1_mpp_base;

static int armada_cp110_1_mpp_ctrl_get(unsigned pid, unsigned long *config)
{
	return default_mpp_ctrl_get(cp1_mpp_base, pid, config);
}

static int armada_cp110_1_mpp_ctrl_set(unsigned pid, unsigned long config)
{
	return default_mpp_ctrl_set(cp1_mpp_base, pid, config);
}
/* In Armada-70x0 (single CP) all the MPPs are available.
** In Armada-80x0 (dual CP) the MPPs are split into 2 parts, MPPs 0-31 from
** CP1, and MPPs 32-62 from CP0, the below flags (V_ARMADA_80X0_CP0,
** V_ARMADA_80X0_CP1) set which MPP is available to the CPx.
** The x_PLUS enum mean that the MPP available for CPx and for Armada70x0
 */
enum {
	V_ARMADA_70X0 = BIT(0),
	V_ARMADA_80X0_CP0 = BIT(1),
	V_ARMADA_80X0_CP1 = BIT(2),
	V_ARMADA_80X0_CP0_PLUS = (V_ARMADA_70X0 | V_ARMADA_80X0_CP0),
	V_ARMADA_80X0_CP1_PLUS = (V_ARMADA_70X0 | V_ARMADA_80X0_CP1),
};

static struct mvebu_mpp_mode armada_cp110_mpp_modes[] = {
	MPP_MODE(0,
		 MPP_VAR_FUNCTION(0,	"gpio",		NULL,		V_ARMADA_80X0_CP1_PLUS),
		 MPP_VAR_FUNCTION(1,	"dev",		"ale1",		V_ARMADA_80X0_CP1_PLUS),
		 MPP_VAR_FUNCTION(2,	"au",		"i2smclk",	V_ARMADA_80X0_CP1_PLUS),
		 MPP_VAR_FUNCTION(3,	"ge0",		"rxd3",		V_ARMADA_80X0_CP1_PLUS),
		 MPP_VAR_FUNCTION(4,	"tdm",		"pclk",		V_ARMADA_80X0_CP1_PLUS),
		 MPP_VAR_FUNCTION(6,	"ptp",		"pulse",	V_ARMADA_80X0_CP1_PLUS),
		 MPP_VAR_FUNCTION(7,	"mss_i2c",	"sda",		V_ARMADA_80X0_CP1_PLUS),
		 MPP_VAR_FUNCTION(8,	"uart0",	"rxd",		V_ARMADA_80X0_CP1_PLUS),
		 MPP_VAR_FUNCTION(9,	"sata0",	"present_act",	V_ARMADA_80X0_CP1_PLUS),
		 MPP_VAR_FUNCTION(10,	"ge",		"mdio",		V_ARMADA_80X0_CP1_PLUS)),
	MPP_MODE(1,
		 MPP_VAR_FUNCTION(0,	"gpio",		NULL,		V_ARMADA_80X0_CP1_PLUS),
		 MPP_VAR_FUNCTION(1,	"dev",		"ale0",		V_ARMADA_80X0_CP1_PLUS),
		 MPP_VAR_FUNCTION(2,	"au",		"i2sdo_spdifo",	V_ARMADA_80X0_CP1_PLUS),
		 MPP_VAR_FUNCTION(3,	"ge0",		"rxd2",		V_ARMADA_80X0_CP1_PLUS),
		 MPP_VAR_FUNCTION(4,	"tdm",		"drx",		V_ARMADA_80X0_CP1_PLUS),
		 MPP_VAR_FUNCTION(6,	"ptp",		"clk",		V_ARMADA_80X0_CP1_PLUS),
		 MPP_VAR_FUNCTION(7,	"mss_i2c",	"sck",		V_ARMADA_80X0_CP1_PLUS),
		 MPP_VAR_FUNCTION(8,	"uart0",	"txd",		V_ARMADA_80X0_CP1_PLUS),
		 MPP_VAR_FUNCTION(9,	"sata1",	"present_act",	V_ARMADA_80X0_CP1_PLUS),
		 MPP_VAR_FUNCTION(10,	"ge",		"mdc",		V_ARMADA_80X0_CP1_PLUS)),
	MPP_MODE(2,
		 MPP_VAR_FUNCTION(0,	"gpio",		NULL,		V_ARMADA_80X0_CP1_PLUS),
		 MPP_VAR_FUNCTION(1,	"dev",		"ad15",		V_ARMADA_80X0_CP1_PLUS),
		 MPP_VAR_FUNCTION(2,	"au",		"i2sextclk",	V_ARMADA_80X0_CP1_PLUS),
		 MPP_VAR_FUNCTION(3,	"ge0",		"rxd1",		V_ARMADA_80X0_CP1_PLUS),
		 MPP_VAR_FUNCTION(4,	"tdm",		"dtx",		V_ARMADA_80X0_CP1_PLUS),
		 MPP_VAR_FUNCTION(5,	"mss_uart",	"rxd",		V_ARMADA_80X0_CP1_PLUS),
		 MPP_VAR_FUNCTION(6,	"ptp",		"pclk_out",	V_ARMADA_80X0_CP1_PLUS),
		 MPP_VAR_FUNCTION(7,	"i2c1",		"sck",		V_ARMADA_80X0_CP1_PLUS),
		 MPP_VAR_FUNCTION(8,	"uart1",	"rxd",		V_ARMADA_80X0_CP1_PLUS),
		 MPP_VAR_FUNCTION(9,	"sata0",	"present_act",	V_ARMADA_80X0_CP1_PLUS),
		 MPP_VAR_FUNCTION(10,	"xg",		"mdc",		V_ARMADA_80X0_CP1_PLUS)),
	MPP_MODE(3,
		 MPP_VAR_FUNCTION(0,	"gpio",		NULL,		V_ARMADA_80X0_CP1_PLUS),
		 MPP_VAR_FUNCTION(1,	"dev",		"ad14",		V_ARMADA_80X0_CP1_PLUS),
		 MPP_VAR_FUNCTION(2,	"au",		"i2slrclk",	V_ARMADA_80X0_CP1_PLUS),
		 MPP_VAR_FUNCTION(3,	"ge0",		"rxd0",		V_ARMADA_80X0_CP1_PLUS),
		 MPP_VAR_FUNCTION(4,	"tdm",		"fsync",	V_ARMADA_80X0_CP1_PLUS),
		 MPP_VAR_FUNCTION(5,	"mss_uart",	"txd",		V_ARMADA_80X0_CP1_PLUS),
		 MPP_VAR_FUNCTION(6,	"pcie",		"rstoutn",	V_ARMADA_80X0_CP1_PLUS),
		 MPP_VAR_FUNCTION(7,	"i2c1",		"sda",		V_ARMADA_80X0_CP1_PLUS),
		 MPP_VAR_FUNCTION(8,	"uart1",	"txd",		V_ARMADA_80X0_CP1_PLUS),
		 MPP_VAR_FUNCTION(9,	"sata1",	"present_act",	V_ARMADA_80X0_CP1_PLUS),
		 MPP_VAR_FUNCTION(10,	"xg",		"mdio",		V_ARMADA_80X0_CP1_PLUS)),
	MPP_MODE(4,
		 MPP_VAR_FUNCTION(0,	"gpio",		NULL,		V_ARMADA_80X0_CP1_PLUS),
		 MPP_VAR_FUNCTION(1,	"dev",		"ad13",		V_ARMADA_80X0_CP1_PLUS),
		 MPP_VAR_FUNCTION(2,	"au",		"i2sbclk",	V_ARMADA_80X0_CP1_PLUS),
		 MPP_VAR_FUNCTION(3,	"ge0",		"rxctl",	V_ARMADA_80X0_CP1_PLUS),
		 MPP_VAR_FUNCTION(4,	"tdm",		"rstn",		V_ARMADA_80X0_CP1_PLUS),
		 MPP_VAR_FUNCTION(5,	"mss_uart",	"rxd",		V_ARMADA_80X0_CP1_PLUS),
		 MPP_VAR_FUNCTION(6,	"uart1",	"cts",		V_ARMADA_80X0_CP1_PLUS),
		 MPP_VAR_FUNCTION(7,	"pcie0",	"clkreq",	V_ARMADA_80X0_CP1_PLUS),
		 MPP_VAR_FUNCTION(8,	"uart3",	"rxd",		V_ARMADA_80X0_CP1_PLUS),
		 MPP_VAR_FUNCTION(10,	"ge",		"mdc",		V_ARMADA_80X0_CP1_PLUS)),
	MPP_MODE(5,
		 MPP_VAR_FUNCTION(0,	"gpio",		NULL,		V_ARMADA_80X0_CP1_PLUS),
		 MPP_VAR_FUNCTION(1,	"dev",		"ad12",		V_ARMADA_80X0_CP1_PLUS),
		 MPP_VAR_FUNCTION(2,	"au",		"i2sdi",	V_ARMADA_80X0_CP1_PLUS),
		 MPP_VAR_FUNCTION(3,	"ge0",		"rxclk",	V_ARMADA_80X0_CP1_PLUS),
		 MPP_VAR_FUNCTION(4,	"tdm",		"intn",		V_ARMADA_80X0_CP1_PLUS),
		 MPP_VAR_FUNCTION(5,	"mss_uart",	"txd",		V_ARMADA_80X0_CP1_PLUS),
		 MPP_VAR_FUNCTION(6,	"uart1",	"rts",		V_ARMADA_80X0_CP1_PLUS),
		 MPP_VAR_FUNCTION(7,	"pcie1",	"clkreq",	V_ARMADA_80X0_CP1_PLUS),
		 MPP_VAR_FUNCTION(8,	"uart3",	"txd",		V_ARMADA_80X0_CP1_PLUS),
		 MPP_VAR_FUNCTION(10,	"ge",		"mdio",		V_ARMADA_80X0_CP1_PLUS)),
	MPP_MODE(6,
		 MPP_VAR_FUNCTION(0,	"gpio",		NULL,		V_ARMADA_80X0_CP1_PLUS),
		 MPP_VAR_FUNCTION(1,	"dev",		"ad11",		V_ARMADA_80X0_CP1_PLUS),
		 MPP_VAR_FUNCTION(3,	"ge0",		"txd3",		V_ARMADA_80X0_CP1_PLUS),
		 MPP_VAR_FUNCTION(4,	"spi0",		"csn2",		V_ARMADA_80X0_CP1_PLUS),
		 MPP_VAR_FUNCTION(5,	"au",		"i2sextclk",	V_ARMADA_80X0_CP1_PLUS),
		 MPP_VAR_FUNCTION(6,	"sata1",	"present_act",	V_ARMADA_80X0_CP1_PLUS),
		 MPP_VAR_FUNCTION(7,	"pcie2",	"clkreq",	V_ARMADA_80X0_CP1_PLUS),
		 MPP_VAR_FUNCTION(8,	"uart0",	"rxd",		V_ARMADA_80X0_CP1_PLUS),
		 MPP_VAR_FUNCTION(9,	"ptp",		"pulse",	V_ARMADA_80X0_CP1_PLUS)),
	MPP_MODE(7,
		 MPP_VAR_FUNCTION(0,	"gpio",		NULL,		V_ARMADA_80X0_CP1_PLUS),
		 MPP_VAR_FUNCTION(1,	"dev",		"ad10",		V_ARMADA_80X0_CP1_PLUS),
		 MPP_VAR_FUNCTION(3,	"ge0",		"txd2",		V_ARMADA_80X0_CP1_PLUS),
		 MPP_VAR_FUNCTION(4,	"spi0",		"csn1",		V_ARMADA_80X0_CP1_PLUS),
		 MPP_VAR_FUNCTION(5,	"spi1",		"csn1",		V_ARMADA_80X0_CP1_PLUS),
		 MPP_VAR_FUNCTION(6,	"sata0",	"present_act",	V_ARMADA_80X0_CP1_PLUS),
		 MPP_VAR_FUNCTION(7,	"led",		"data",		V_ARMADA_80X0_CP1_PLUS),
		 MPP_VAR_FUNCTION(8,	"uart0",	"txd",		V_ARMADA_80X0_CP1_PLUS),
		 MPP_VAR_FUNCTION(9,	"ptp",		"clk",		V_ARMADA_80X0_CP1_PLUS)),
	MPP_MODE(8,
		 MPP_VAR_FUNCTION(0,	"gpio",		NULL,		V_ARMADA_80X0_CP1_PLUS),
		 MPP_VAR_FUNCTION(1,	"dev",		"ad9",		V_ARMADA_80X0_CP1_PLUS),
		 MPP_VAR_FUNCTION(3,	"ge0",		"txd1",		V_ARMADA_80X0_CP1_PLUS),
		 MPP_VAR_FUNCTION(4,	"spi0",		"csn0",		V_ARMADA_80X0_CP1_PLUS),
		 MPP_VAR_FUNCTION(5,	"spi1",		"csn0",		V_ARMADA_80X0_CP1_PLUS),
		 MPP_VAR_FUNCTION(6,	"uart0",	"cts",		V_ARMADA_80X0_CP1_PLUS),
		 MPP_VAR_FUNCTION(7,	"led",		"stb",		V_ARMADA_80X0_CP1_PLUS),
		 MPP_VAR_FUNCTION(8,	"uart2",	"rxd",		V_ARMADA_80X0_CP1_PLUS),
		 MPP_VAR_FUNCTION(9,	"ptp",		"pclk_out",	V_ARMADA_80X0_CP1_PLUS),
		 MPP_VAR_FUNCTION(10,	"synce1",	"clk",		V_ARMADA_80X0_CP1_PLUS)),
	MPP_MODE(9,
		 MPP_VAR_FUNCTION(0,	"gpio",		NULL,		V_ARMADA_80X0_CP1_PLUS),
		 MPP_VAR_FUNCTION(1,	"dev",		"ad8",		V_ARMADA_80X0_CP1_PLUS),
		 MPP_VAR_FUNCTION(3,	"ge0",		"txd0",		V_ARMADA_80X0_CP1_PLUS),
		 MPP_VAR_FUNCTION(4,	"spi0",		"mosi",		V_ARMADA_80X0_CP1_PLUS),
		 MPP_VAR_FUNCTION(5,	"spi1",		"mosi",		V_ARMADA_80X0_CP1_PLUS),
		 MPP_VAR_FUNCTION(7,	"pcie",		"rstoutn",	V_ARMADA_80X0_CP1_PLUS),
		 MPP_VAR_FUNCTION(10,	"synce2",	"clk",		V_ARMADA_80X0_CP1_PLUS)),
	MPP_MODE(10,
		 MPP_VAR_FUNCTION(0,	"gpio",		NULL,		V_ARMADA_80X0_CP1_PLUS),
		 MPP_VAR_FUNCTION(1,	"dev",		"readyn",	V_ARMADA_80X0_CP1_PLUS),
		 MPP_VAR_FUNCTION(3,	"ge0",		"txctl",	V_ARMADA_80X0_CP1_PLUS),
		 MPP_VAR_FUNCTION(4,	"spi0",		"miso",		V_ARMADA_80X0_CP1_PLUS),
		 MPP_VAR_FUNCTION(5,	"spi1",		"miso",		V_ARMADA_80X0_CP1_PLUS),
		 MPP_VAR_FUNCTION(6,	"uart0",	"cts",		V_ARMADA_80X0_CP1_PLUS),
		 MPP_VAR_FUNCTION(7,	"sata1",	"present_act",	V_ARMADA_80X0_CP1_PLUS)),
	MPP_MODE(11,
		 MPP_VAR_FUNCTION(0,	"gpio",		NULL,		V_ARMADA_80X0_CP1_PLUS),
		 MPP_VAR_FUNCTION(1,	"dev",		"wen1",		V_ARMADA_80X0_CP1_PLUS),
		 MPP_VAR_FUNCTION(3,	"ge0",		"txclkout",	V_ARMADA_80X0_CP1_PLUS),
		 MPP_VAR_FUNCTION(4,	"spi0",		"clk",		V_ARMADA_80X0_CP1_PLUS),
		 MPP_VAR_FUNCTION(5,	"spi1",		"clk",		V_ARMADA_80X0_CP1_PLUS),
		 MPP_VAR_FUNCTION(6,	"uart0",	"rts",		V_ARMADA_80X0_CP1_PLUS),
		 MPP_VAR_FUNCTION(7,	"led",		"clk",		V_ARMADA_80X0_CP1_PLUS),
		 MPP_VAR_FUNCTION(8,	"uart2",	"txd",		V_ARMADA_80X0_CP1_PLUS),
		 MPP_VAR_FUNCTION(9,	"sata0",	"present_act",	V_ARMADA_80X0_CP1_PLUS)),
	MPP_MODE(12,
		 MPP_VAR_FUNCTION(0,	"gpio",		NULL,		V_ARMADA_80X0_CP1_PLUS),
		 MPP_VAR_FUNCTION(1,	"dev",		"clk_out",	V_ARMADA_80X0_CP1_PLUS),
		 MPP_VAR_FUNCTION(2,	"nf",		"rbn1",		V_ARMADA_80X0_CP1_PLUS),
		 MPP_VAR_FUNCTION(3,	"spi1",		"csn1",		V_ARMADA_80X0_CP1_PLUS),
		 MPP_VAR_FUNCTION(4,	"ge0",		"rxclk",	V_ARMADA_80X0_CP1_PLUS)),
	MPP_MODE(13,
		 MPP_VAR_FUNCTION(0,	"gpio",		NULL,		V_ARMADA_80X0_CP1_PLUS),
		 MPP_VAR_FUNCTION(1,	"dev",		"burstn",	V_ARMADA_80X0_CP1_PLUS),
		 MPP_VAR_FUNCTION(2,	"nf",		"rbn0",		V_ARMADA_80X0_CP1_PLUS),
		 MPP_VAR_FUNCTION(3,	"spi1",		"miso",		V_ARMADA_80X0_CP1_PLUS),
		 MPP_VAR_FUNCTION(4,	"ge0",		"rxctl",	V_ARMADA_80X0_CP1_PLUS),
		 MPP_VAR_FUNCTION(8,	"mss_spi",	"miso",		V_ARMADA_80X0_CP1_PLUS)),
	MPP_MODE(14,
		 MPP_VAR_FUNCTION(0,	"gpio",		NULL,		V_ARMADA_80X0_CP1_PLUS),
		 MPP_VAR_FUNCTION(1,	"dev",		"bootcsn",	V_ARMADA_80X0_CP1_PLUS),
		 MPP_VAR_FUNCTION(2,	"dev",		"csn0",		V_ARMADA_80X0_CP1_PLUS),
		 MPP_VAR_FUNCTION(3,	"spi1",		"csn0",		V_ARMADA_80X0_CP1_PLUS),
		 MPP_VAR_FUNCTION(4,	"spi0",		"csn3",		V_ARMADA_80X0_CP1_PLUS),
		 MPP_VAR_FUNCTION(5,	"au",		"i2sextclk",	V_ARMADA_80X0_CP1_PLUS),
		 MPP_VAR_FUNCTION(6,	"spi0",		"miso",		V_ARMADA_80X0_CP1_PLUS),
		 MPP_VAR_FUNCTION(7,	"sata0",	"present_act",	V_ARMADA_80X0_CP1_PLUS),
		 MPP_VAR_FUNCTION(8,	"mss_spi",	"csn",		V_ARMADA_80X0_CP1_PLUS)),
	MPP_MODE(15,
		 MPP_VAR_FUNCTION(0,	"gpio",		NULL,		V_ARMADA_80X0_CP1_PLUS),
		 MPP_VAR_FUNCTION(1,	"dev",		"ad7",		V_ARMADA_80X0_CP1_PLUS),
		 MPP_VAR_FUNCTION(3,	"spi1",		"mosi",		V_ARMADA_80X0_CP1_PLUS),
		 MPP_VAR_FUNCTION(6,	"spi0",		"mosi",		V_ARMADA_80X0_CP1_PLUS),
		 MPP_VAR_FUNCTION(8,	"mss_spi",	"mosi",		V_ARMADA_80X0_CP1_PLUS),
		 MPP_VAR_FUNCTION(11,	"ptp",		"pulse_cp2cp",	V_ARMADA_80X0_CP1_PLUS)),
	MPP_MODE(16,
		 MPP_VAR_FUNCTION(0,	"gpio",		NULL,		V_ARMADA_80X0_CP1_PLUS),
		 MPP_VAR_FUNCTION(1,	"dev",		"ad6",		V_ARMADA_80X0_CP1_PLUS),
		 MPP_VAR_FUNCTION(3,	"spi1",		"clk",		V_ARMADA_80X0_CP1_PLUS),
		 MPP_VAR_FUNCTION(8,	"mss_spi",	"clk",		V_ARMADA_80X0_CP1_PLUS)),
	MPP_MODE(17,
		 MPP_VAR_FUNCTION(0,	"gpio",		NULL,		V_ARMADA_80X0_CP1_PLUS),
		 MPP_VAR_FUNCTION(1,	"dev",		"ad5",		V_ARMADA_80X0_CP1_PLUS),
		 MPP_VAR_FUNCTION(4,	"ge0",		"txd3",		V_ARMADA_80X0_CP1_PLUS)),
	MPP_MODE(18,
		 MPP_VAR_FUNCTION(0,	"gpio",		NULL,		V_ARMADA_80X0_CP1_PLUS),
		 MPP_VAR_FUNCTION(1,	"dev",		"ad4",		V_ARMADA_80X0_CP1_PLUS),
		 MPP_VAR_FUNCTION(4,	"ge0",		"txd2",		V_ARMADA_80X0_CP1_PLUS),
		 MPP_VAR_FUNCTION(11,	"ptp",		"clk_cp2cp",	V_ARMADA_80X0_CP1_PLUS)),
	MPP_MODE(19,
		 MPP_VAR_FUNCTION(0,	"gpio",		NULL,		V_ARMADA_80X0_CP1_PLUS),
		 MPP_VAR_FUNCTION(1,	"dev",		"ad3",		V_ARMADA_80X0_CP1_PLUS),
		 MPP_VAR_FUNCTION(4,	"ge0",		"txd1",		V_ARMADA_80X0_CP1_PLUS),
		 MPP_VAR_FUNCTION(11,	"wakeup",	"out_cp2cp",	V_ARMADA_80X0_CP1_PLUS)),
	MPP_MODE(20,
		 MPP_VAR_FUNCTION(0,	"gpio",		NULL,		V_ARMADA_80X0_CP1_PLUS),
		 MPP_VAR_FUNCTION(1,	"dev",		"ad2",		V_ARMADA_80X0_CP1_PLUS),
		 MPP_VAR_FUNCTION(4,	"ge0",		"txd0",		V_ARMADA_80X0_CP1_PLUS)),
	MPP_MODE(21,
		 MPP_VAR_FUNCTION(0,	"gpio",		NULL,		V_ARMADA_80X0_CP1_PLUS),
		 MPP_VAR_FUNCTION(1,	"dev",		"ad1",		V_ARMADA_80X0_CP1_PLUS),
		 MPP_VAR_FUNCTION(4,	"ge0",		"txctl",	V_ARMADA_80X0_CP1_PLUS),
		 MPP_VAR_FUNCTION(11,	"sei",		"in_cp2cp",	V_ARMADA_80X0_CP1_PLUS)),
	MPP_MODE(22,
		 MPP_VAR_FUNCTION(0,	"gpio",		NULL,		V_ARMADA_80X0_CP1_PLUS),
		 MPP_VAR_FUNCTION(1,	"dev",		"ad0",		V_ARMADA_80X0_CP1_PLUS),
		 MPP_VAR_FUNCTION(4,	"ge0",		"txclkout",	V_ARMADA_80X0_CP1_PLUS),
		 MPP_VAR_FUNCTION(11,	"wakeup",	"in_cp2cp",	V_ARMADA_80X0_CP1_PLUS)),
	MPP_MODE(23,
		 MPP_VAR_FUNCTION(0,	"gpio",		NULL,		V_ARMADA_80X0_CP1_PLUS),
		 MPP_VAR_FUNCTION(1,	"dev",		"a1",		V_ARMADA_80X0_CP1_PLUS),
		 MPP_VAR_FUNCTION(5,	"au",		"i2smclk",	V_ARMADA_80X0_CP1_PLUS),
		 MPP_VAR_FUNCTION(11,	"link",		"rd_in_cp2cp",	V_ARMADA_80X0_CP1_PLUS)),
	MPP_MODE(24,
		 MPP_VAR_FUNCTION(0,	"gpio",		NULL,		V_ARMADA_80X0_CP1_PLUS),
		 MPP_VAR_FUNCTION(1,	"dev",		"a0",		V_ARMADA_80X0_CP1_PLUS),
		 MPP_VAR_FUNCTION(5,	"au",		"i2slrclk",	V_ARMADA_80X0_CP1_PLUS)),
	MPP_MODE(25,
		 MPP_VAR_FUNCTION(0,	"gpio",		NULL,		V_ARMADA_80X0_CP1_PLUS),
		 MPP_VAR_FUNCTION(1,	"dev",		"oen",		V_ARMADA_80X0_CP1_PLUS),
		 MPP_VAR_FUNCTION(5,	"au",		"i2sdo_spdifo",	V_ARMADA_80X0_CP1_PLUS)),
	MPP_MODE(26,
		 MPP_VAR_FUNCTION(0,	"gpio",		NULL,		V_ARMADA_80X0_CP1_PLUS),
		 MPP_VAR_FUNCTION(1,	"dev",		"wen0",		V_ARMADA_80X0_CP1_PLUS),
		 MPP_VAR_FUNCTION(5,	"au",		"i2sbclk",	V_ARMADA_80X0_CP1_PLUS)),
	MPP_MODE(27,
		 MPP_VAR_FUNCTION(0,	"gpio",		NULL,		V_ARMADA_80X0_CP1_PLUS),
		 MPP_VAR_FUNCTION(1,	"dev",		"csn0",		V_ARMADA_80X0_CP1_PLUS),
		 MPP_VAR_FUNCTION(2,	"spi1",		"miso",		V_ARMADA_80X0_CP1_PLUS),
		 MPP_VAR_FUNCTION(3,	"mss_gpio4",	NULL,		V_ARMADA_80X0_CP1_PLUS),
		 MPP_VAR_FUNCTION(4,	"ge0",		"rxd3",		V_ARMADA_80X0_CP1_PLUS),
		 MPP_VAR_FUNCTION(5,	"spi0",		"csn4",		V_ARMADA_80X0_CP1_PLUS),
		 MPP_VAR_FUNCTION(8,	"ge",		"mdio",		V_ARMADA_80X0_CP1_PLUS),
		 MPP_VAR_FUNCTION(9,	"sata0",	"present_act",	V_ARMADA_80X0_CP1_PLUS),
		 MPP_VAR_FUNCTION(10,	"uart0",	"rts",		V_ARMADA_80X0_CP1_PLUS),
		 MPP_VAR_FUNCTION(11,	"rei",		"in_cp2cp",	V_ARMADA_80X0_CP1_PLUS)),
	MPP_MODE(28,
		 MPP_VAR_FUNCTION(0,	"gpio",		NULL,		V_ARMADA_80X0_CP1_PLUS),
		 MPP_VAR_FUNCTION(1,	"dev",		"csn1",		V_ARMADA_80X0_CP1_PLUS),
		 MPP_VAR_FUNCTION(2,	"spi1",		"csn0",		V_ARMADA_80X0_CP1_PLUS),
		 MPP_VAR_FUNCTION(3,	"mss_gpio5",	NULL,		V_ARMADA_80X0_CP1_PLUS),
		 MPP_VAR_FUNCTION(4,	"ge0",		"rxd2",		V_ARMADA_80X0_CP1_PLUS),
		 MPP_VAR_FUNCTION(5,	"spi0",		"csn5",		V_ARMADA_80X0_CP1_PLUS),
		 MPP_VAR_FUNCTION(6,	"pcie2",	"clkreq",	V_ARMADA_80X0_CP1_PLUS),
		 MPP_VAR_FUNCTION(7,	"ptp",		"pulse",	V_ARMADA_80X0_CP1_PLUS),
		 MPP_VAR_FUNCTION(8,	"ge",		"mdc",		V_ARMADA_80X0_CP1_PLUS),
		 MPP_VAR_FUNCTION(9,	"sata1",	"present_act",	V_ARMADA_80X0_CP1_PLUS),
		 MPP_VAR_FUNCTION(10,	"uart0",	"cts",		V_ARMADA_80X0_CP1_PLUS),
		 MPP_VAR_FUNCTION(11,	"led",		"data",		V_ARMADA_80X0_CP1_PLUS)),
	MPP_MODE(29,
		 MPP_VAR_FUNCTION(0,	"gpio",		NULL,		V_ARMADA_80X0_CP1_PLUS),
		 MPP_VAR_FUNCTION(1,	"dev",		"csn2",		V_ARMADA_80X0_CP1_PLUS),
		 MPP_VAR_FUNCTION(2,	"spi1",		"mosi",		V_ARMADA_80X0_CP1_PLUS),
		 MPP_VAR_FUNCTION(3,	"mss_gpio6",	NULL,		V_ARMADA_80X0_CP1_PLUS),
		 MPP_VAR_FUNCTION(4,	"ge0",		"rxd1",		V_ARMADA_80X0_CP1_PLUS),
		 MPP_VAR_FUNCTION(5,	"spi0",		"csn6",		V_ARMADA_80X0_CP1_PLUS),
		 MPP_VAR_FUNCTION(6,	"pcie1",	"clkreq",	V_ARMADA_80X0_CP1_PLUS),
		 MPP_VAR_FUNCTION(7,	"ptp",		"clk",		V_ARMADA_80X0_CP1_PLUS),
		 MPP_VAR_FUNCTION(8,	"mss_i2c",	"sda",		V_ARMADA_80X0_CP1_PLUS),
		 MPP_VAR_FUNCTION(9,	"sata0",	"present_act",	V_ARMADA_80X0_CP1_PLUS),
		 MPP_VAR_FUNCTION(10,	"uart0",	"rxd",		V_ARMADA_80X0_CP1_PLUS),
		 MPP_VAR_FUNCTION(11,	"led",		"stb",		V_ARMADA_80X0_CP1_PLUS)),
	MPP_MODE(30,
		 MPP_VAR_FUNCTION(0,	"gpio",		NULL,		V_ARMADA_80X0_CP1_PLUS),
		 MPP_VAR_FUNCTION(1,	"dev",		"csn3",		V_ARMADA_80X0_CP1_PLUS),
		 MPP_VAR_FUNCTION(2,	"spi1",		"clk",		V_ARMADA_80X0_CP1_PLUS),
		 MPP_VAR_FUNCTION(3,	"mss_gpio7",	NULL,		V_ARMADA_80X0_CP1_PLUS),
		 MPP_VAR_FUNCTION(4,	"ge0",		"rxd0",		V_ARMADA_80X0_CP1_PLUS),
		 MPP_VAR_FUNCTION(5,	"spi0",		"csn7",		V_ARMADA_80X0_CP1_PLUS),
		 MPP_VAR_FUNCTION(6,	"pcie0",	"clkreq",	V_ARMADA_80X0_CP1_PLUS),
		 MPP_VAR_FUNCTION(7,	"ptp",		"pclk_out",	V_ARMADA_80X0_CP1_PLUS),
		 MPP_VAR_FUNCTION(8,	"mss_i2c",	"sck",		V_ARMADA_80X0_CP1_PLUS),
		 MPP_VAR_FUNCTION(9,	"sata1",	"present_act",	V_ARMADA_80X0_CP1_PLUS),
		 MPP_VAR_FUNCTION(10,	"uart0",	"txd",		V_ARMADA_80X0_CP1_PLUS),
		 MPP_VAR_FUNCTION(11,	"led",		"clk",		V_ARMADA_80X0_CP1_PLUS)),
	MPP_MODE(31,
		 MPP_VAR_FUNCTION(0,	"gpio",		NULL,		V_ARMADA_80X0_CP1_PLUS),
		 MPP_VAR_FUNCTION(1,	"dev",		"a2",		V_ARMADA_80X0_CP1_PLUS),
		 MPP_VAR_FUNCTION(3,	"mss_gpio4",	NULL,		V_ARMADA_80X0_CP1_PLUS),
		 MPP_VAR_FUNCTION(6,	"pcie",		"rstoutn",	V_ARMADA_80X0_CP1_PLUS),
		 MPP_VAR_FUNCTION(8,	"ge",		"mdc",		V_ARMADA_80X0_CP1_PLUS)),
	MPP_MODE(32,
		 MPP_VAR_FUNCTION(0,	"gpio",		NULL,		V_ARMADA_80X0_CP0_PLUS),
		 MPP_VAR_FUNCTION(1,	"mii",		"col",		V_ARMADA_80X0_CP0_PLUS),
		 MPP_VAR_FUNCTION(2,	"mii",		"txerr",	V_ARMADA_80X0_CP0_PLUS),
		 MPP_VAR_FUNCTION(3,	"mss_spi",	"miso",		V_ARMADA_80X0_CP0_PLUS),
		 MPP_VAR_FUNCTION(4,	"tdm",		"drx",		V_ARMADA_80X0_CP0_PLUS),
		 MPP_VAR_FUNCTION(5,	"au",		"i2sextclk",	V_ARMADA_80X0_CP0_PLUS),
		 MPP_VAR_FUNCTION(6,	"au",		"i2sdi",	V_ARMADA_80X0_CP0_PLUS),
		 MPP_VAR_FUNCTION(7,	"ge",		"mdio",		V_ARMADA_80X0_CP0_PLUS),
		 MPP_VAR_FUNCTION(8,	"sdio",		"v18_en",	V_ARMADA_80X0_CP0_PLUS),
		 MPP_VAR_FUNCTION(9,	"pcie1",	"clkreq",	V_ARMADA_80X0_CP0_PLUS),
		 MPP_VAR_FUNCTION(10,	"mss_gpio0",	NULL,		V_ARMADA_80X0_CP0_PLUS)),
	MPP_MODE(33,
		 MPP_VAR_FUNCTION(0,	"gpio",		NULL,		V_ARMADA_80X0_CP0_PLUS),
		 MPP_VAR_FUNCTION(1,	"mii",		"txclk",	V_ARMADA_80X0_CP0_PLUS),
		 MPP_VAR_FUNCTION(2,	"sdio",		"pwr10",	V_ARMADA_80X0_CP0_PLUS),
		 MPP_VAR_FUNCTION(3,	"mss_spi",	"csn",		V_ARMADA_80X0_CP0_PLUS),
		 MPP_VAR_FUNCTION(4,	"tdm",		"fsync",	V_ARMADA_80X0_CP0_PLUS),
		 MPP_VAR_FUNCTION(5,	"au",		"i2smclk",	V_ARMADA_80X0_CP0_PLUS),
		 MPP_VAR_FUNCTION(6,	"sdio",		"bus_pwr",	V_ARMADA_80X0_CP0_PLUS),
		 MPP_VAR_FUNCTION(8,	"xg",		"mdio",		V_ARMADA_80X0_CP0_PLUS),
		 MPP_VAR_FUNCTION(9,	"pcie2",	"clkreq",	V_ARMADA_80X0_CP0_PLUS),
		 MPP_VAR_FUNCTION(10,	"mss_gpio1",	NULL,		V_ARMADA_80X0_CP0_PLUS)),
	MPP_MODE(34,
		 MPP_VAR_FUNCTION(0,	"gpio",		NULL,		V_ARMADA_80X0_CP0_PLUS),
		 MPP_VAR_FUNCTION(1,	"mii",		"rxerr",	V_ARMADA_80X0_CP0_PLUS),
		 MPP_VAR_FUNCTION(2,	"sdio",		"pwr11",	V_ARMADA_80X0_CP0_PLUS),
		 MPP_VAR_FUNCTION(3,	"mss_spi",	"mosi",		V_ARMADA_80X0_CP0_PLUS),
		 MPP_VAR_FUNCTION(4,	"tdm",		"dtx",		V_ARMADA_80X0_CP0_PLUS),
		 MPP_VAR_FUNCTION(5,	"au",		"i2slrclk",	V_ARMADA_80X0_CP0_PLUS),
		 MPP_VAR_FUNCTION(6,	"sdio",		"wr_protect",	V_ARMADA_80X0_CP0_PLUS),
		 MPP_VAR_FUNCTION(7,	"ge",		"mdc",		V_ARMADA_80X0_CP0_PLUS),
		 MPP_VAR_FUNCTION(9,	"pcie0",	"clkreq",	V_ARMADA_80X0_CP0_PLUS),
		 MPP_VAR_FUNCTION(10,	"mss_gpio2",	NULL,		V_ARMADA_80X0_CP0_PLUS)),
	MPP_MODE(35,
		 MPP_VAR_FUNCTION(0,	"gpio",		NULL,		V_ARMADA_80X0_CP0_PLUS),
		 MPP_VAR_FUNCTION(1,	"sata1",	"present_act",	V_ARMADA_80X0_CP0_PLUS),
		 MPP_VAR_FUNCTION(2,	"i2c1",		"sda",		V_ARMADA_80X0_CP0_PLUS),
		 MPP_VAR_FUNCTION(3,	"mss_spi",	"clk",		V_ARMADA_80X0_CP0_PLUS),
		 MPP_VAR_FUNCTION(4,	"tdm",		"pclk",		V_ARMADA_80X0_CP0_PLUS),
		 MPP_VAR_FUNCTION(5,	"au",		"i2sdo_spdifo",	V_ARMADA_80X0_CP0_PLUS),
		 MPP_VAR_FUNCTION(6,	"sdio",		"card_detect",	V_ARMADA_80X0_CP0_PLUS),
		 MPP_VAR_FUNCTION(7,	"xg",		"mdio",		V_ARMADA_80X0_CP0_PLUS),
		 MPP_VAR_FUNCTION(8,	"ge",		"mdio",		V_ARMADA_80X0_CP0_PLUS),
		 MPP_VAR_FUNCTION(9,	"pcie",		"rstoutn",	V_ARMADA_80X0_CP0_PLUS),
		 MPP_VAR_FUNCTION(10,	"mss_gpio3",	NULL,		V_ARMADA_80X0_CP0_PLUS)),
	MPP_MODE(36,
		 MPP_VAR_FUNCTION(0,	"gpio",		NULL,		V_ARMADA_80X0_CP0_PLUS),
		 MPP_VAR_FUNCTION(1,	"synce2",	"clk",		V_ARMADA_80X0_CP0_PLUS),
		 MPP_VAR_FUNCTION(2,	"i2c1",		"sck",		V_ARMADA_80X0_CP0_PLUS),
		 MPP_VAR_FUNCTION(3,	"ptp",		"clk",		V_ARMADA_80X0_CP0_PLUS),
		 MPP_VAR_FUNCTION(4,	"synce1",	"clk",		V_ARMADA_80X0_CP0_PLUS),
		 MPP_VAR_FUNCTION(5,	"au",		"i2sbclk",	V_ARMADA_80X0_CP0_PLUS),
		 MPP_VAR_FUNCTION(6,	"sata0",	"present_act",	V_ARMADA_80X0_CP0_PLUS),
		 MPP_VAR_FUNCTION(7,	"xg",		"mdc",		V_ARMADA_80X0_CP0_PLUS),
		 MPP_VAR_FUNCTION(8,	"ge",		"mdc",		V_ARMADA_80X0_CP0_PLUS),
		 MPP_VAR_FUNCTION(9,	"pcie2",	"clkreq",	V_ARMADA_80X0_CP0_PLUS),
		 MPP_VAR_FUNCTION(10,	"mss_gpio5",	NULL,		V_ARMADA_80X0_CP0_PLUS)),
	MPP_MODE(37,
		 MPP_VAR_FUNCTION(0,	"gpio",		NULL,		V_ARMADA_80X0_CP0_PLUS),
		 MPP_VAR_FUNCTION(1,	"uart2",	"rxd",		V_ARMADA_80X0_CP0_PLUS),
		 MPP_VAR_FUNCTION(2,	"i2c0",		"sck",		V_ARMADA_80X0_CP0_PLUS),
		 MPP_VAR_FUNCTION(3,	"ptp",		"pclk_out",	V_ARMADA_80X0_CP0_PLUS),
		 MPP_VAR_FUNCTION(4,	"tdm",		"intn",		V_ARMADA_80X0_CP0_PLUS),
		 MPP_VAR_FUNCTION(5,	"mss_i2c",	"sck",		V_ARMADA_80X0_CP0_PLUS),
		 MPP_VAR_FUNCTION(6,	"sata1",	"present_act",	V_ARMADA_80X0_CP0_PLUS),
		 MPP_VAR_FUNCTION(7,	"ge",		"mdc",		V_ARMADA_80X0_CP0_PLUS),
		 MPP_VAR_FUNCTION(8,	"xg",		"mdc",		V_ARMADA_80X0_CP0_PLUS),
		 MPP_VAR_FUNCTION(9,	"pcie1",	"clkreq",	V_ARMADA_80X0_CP0_PLUS),
		 MPP_VAR_FUNCTION(10,	"mss_gpio6",	NULL,		V_ARMADA_80X0_CP0_PLUS),
		 MPP_VAR_FUNCTION(11,	"link",		"rd_out_cp2cp",	V_ARMADA_80X0_CP0_PLUS)),
	MPP_MODE(38,
		 MPP_VAR_FUNCTION(0,	"gpio",		NULL,		V_ARMADA_80X0_CP0_PLUS),
		 MPP_VAR_FUNCTION(1,	"uart2",	"txd",		V_ARMADA_80X0_CP0_PLUS),
		 MPP_VAR_FUNCTION(2,	"i2c0",		"sda",		V_ARMADA_80X0_CP0_PLUS),
		 MPP_VAR_FUNCTION(3,	"ptp",		"pulse",	V_ARMADA_80X0_CP0_PLUS),
		 MPP_VAR_FUNCTION(4,	"tdm",		"rstn",		V_ARMADA_80X0_CP0_PLUS),
		 MPP_VAR_FUNCTION(5,	"mss_i2c",	"sda",		V_ARMADA_80X0_CP0_PLUS),
		 MPP_VAR_FUNCTION(6,	"sata0",	"present_act",	V_ARMADA_80X0_CP0_PLUS),
		 MPP_VAR_FUNCTION(7,	"ge",		"mdio",		V_ARMADA_80X0_CP0_PLUS),
		 MPP_VAR_FUNCTION(8,	"xg",		"mdio",		V_ARMADA_80X0_CP0_PLUS),
		 MPP_VAR_FUNCTION(9,	"au",		"i2sextclk",	V_ARMADA_80X0_CP0_PLUS),
		 MPP_VAR_FUNCTION(10,	"mss_gpio7",	NULL,		V_ARMADA_80X0_CP0_PLUS),
		 MPP_VAR_FUNCTION(11,	"ptp",		"pulse_cp2cp",	V_ARMADA_80X0_CP0_PLUS)),
	MPP_MODE(39,
		 MPP_VAR_FUNCTION(0,	"gpio",		NULL,		V_ARMADA_80X0_CP0_PLUS),
		 MPP_VAR_FUNCTION(1,	"sdio",		"wr_protect",	V_ARMADA_80X0_CP0_PLUS),
		 MPP_VAR_FUNCTION(4,	"au",		"i2sbclk",	V_ARMADA_80X0_CP0_PLUS),
		 MPP_VAR_FUNCTION(5,	"ptp",		"clk",		V_ARMADA_80X0_CP0_PLUS),
		 MPP_VAR_FUNCTION(6,	"spi0",		"csn1",		V_ARMADA_80X0_CP0_PLUS),
		 MPP_VAR_FUNCTION(9,	"sata1",	"present_act",	V_ARMADA_80X0_CP0_PLUS),
		 MPP_VAR_FUNCTION(10,	"mss_gpio0",	NULL,		V_ARMADA_80X0_CP0_PLUS)),
	MPP_MODE(40,
		 MPP_VAR_FUNCTION(0,	"gpio",		NULL,		V_ARMADA_80X0_CP0_PLUS),
		 MPP_VAR_FUNCTION(1,	"sdio",		"pwr11",	V_ARMADA_80X0_CP0_PLUS),
		 MPP_VAR_FUNCTION(2,	"synce1",	"clk",		V_ARMADA_80X0_CP0_PLUS),
		 MPP_VAR_FUNCTION(3,	"mss_i2c",	"sda",		V_ARMADA_80X0_CP0_PLUS),
		 MPP_VAR_FUNCTION(4,	"au",		"i2sdo_spdifo",	V_ARMADA_80X0_CP0_PLUS),
		 MPP_VAR_FUNCTION(5,	"ptp",		"pclk_out",	V_ARMADA_80X0_CP0_PLUS),
		 MPP_VAR_FUNCTION(6,	"spi0",		"clk",		V_ARMADA_80X0_CP0_PLUS),
		 MPP_VAR_FUNCTION(7,	"uart1",	"txd",		V_ARMADA_80X0_CP0_PLUS),
		 MPP_VAR_FUNCTION(8,	"ge",		"mdio",		V_ARMADA_80X0_CP0_PLUS),
		 MPP_VAR_FUNCTION(9,	"sata0",	"present_act",	V_ARMADA_80X0_CP0_PLUS),
		 MPP_VAR_FUNCTION(10,	"mss_gpio1",	NULL,		V_ARMADA_80X0_CP0_PLUS)),
	MPP_MODE(41,
		 MPP_VAR_FUNCTION(0,	"gpio",		NULL,		V_ARMADA_80X0_CP0_PLUS),
		 MPP_VAR_FUNCTION(1,	"sdio",		"pwr10",	V_ARMADA_80X0_CP0_PLUS),
		 MPP_VAR_FUNCTION(2,	"sdio",		"bus_pwr",	V_ARMADA_80X0_CP0_PLUS),
		 MPP_VAR_FUNCTION(3,	"mss_i2c",	"sck",		V_ARMADA_80X0_CP0_PLUS),
		 MPP_VAR_FUNCTION(4,	"au",		"i2slrclk",	V_ARMADA_80X0_CP0_PLUS),
		 MPP_VAR_FUNCTION(5,	"ptp",		"pulse",	V_ARMADA_80X0_CP0_PLUS),
		 MPP_VAR_FUNCTION(6,	"spi0",		"mosi",		V_ARMADA_80X0_CP0_PLUS),
		 MPP_VAR_FUNCTION(7,	"uart1",	"rxd",		V_ARMADA_80X0_CP0_PLUS),
		 MPP_VAR_FUNCTION(8,	"ge",		"mdc",		V_ARMADA_80X0_CP0_PLUS),
		 MPP_VAR_FUNCTION(9,	"sata1",	"present_act",	V_ARMADA_80X0_CP0_PLUS),
		 MPP_VAR_FUNCTION(10,	"mss_gpio2",	NULL,		V_ARMADA_80X0_CP0_PLUS),
		 MPP_VAR_FUNCTION(11,	"rei",		"out_cp2cp",	V_ARMADA_80X0_CP0_PLUS)),
	MPP_MODE(42,
		 MPP_VAR_FUNCTION(0,	"gpio",		NULL,		V_ARMADA_80X0_CP0_PLUS),
		 MPP_VAR_FUNCTION(1,	"sdio",		"v18_en",	V_ARMADA_80X0_CP0_PLUS),
		 MPP_VAR_FUNCTION(2,	"sdio",		"wr_protect",	V_ARMADA_80X0_CP0_PLUS),
		 MPP_VAR_FUNCTION(3,	"synce2",	"clk",		V_ARMADA_80X0_CP0_PLUS),
		 MPP_VAR_FUNCTION(4,	"au",		"i2smclk",	V_ARMADA_80X0_CP0_PLUS),
		 MPP_VAR_FUNCTION(5,	"mss_uart",	"txd",		V_ARMADA_80X0_CP0_PLUS),
		 MPP_VAR_FUNCTION(6,	"spi0",		"miso",		V_ARMADA_80X0_CP0_PLUS),
		 MPP_VAR_FUNCTION(7,	"uart1",	"cts",		V_ARMADA_80X0_CP0_PLUS),
		 MPP_VAR_FUNCTION(8,	"xg",		"mdc",		V_ARMADA_80X0_CP0_PLUS),
		 MPP_VAR_FUNCTION(9,	"sata0",	"present_act",	V_ARMADA_80X0_CP0_PLUS),
		 MPP_VAR_FUNCTION(10,	"mss_gpio4",	NULL,		V_ARMADA_80X0_CP0_PLUS)),
	MPP_MODE(43,
		 MPP_VAR_FUNCTION(0,	"gpio",		NULL,		V_ARMADA_80X0_CP0_PLUS),
		 MPP_VAR_FUNCTION(1,	"sdio",		"card_detect",	V_ARMADA_80X0_CP0_PLUS),
		 MPP_VAR_FUNCTION(3,	"synce1",	"clk",		V_ARMADA_80X0_CP0_PLUS),
		 MPP_VAR_FUNCTION(4,	"au",		"i2sextclk",	V_ARMADA_80X0_CP0_PLUS),
		 MPP_VAR_FUNCTION(5,	"mss_uart",	"rxd",		V_ARMADA_80X0_CP0_PLUS),
		 MPP_VAR_FUNCTION(6,	"spi0",		"csn0",		V_ARMADA_80X0_CP0_PLUS),
		 MPP_VAR_FUNCTION(7,	"uart1",	"rts",		V_ARMADA_80X0_CP0_PLUS),
		 MPP_VAR_FUNCTION(8,	"xg",		"mdio",		V_ARMADA_80X0_CP0_PLUS),
		 MPP_VAR_FUNCTION(9,	"sata1",	"present_act",	V_ARMADA_80X0_CP0_PLUS),
		 MPP_VAR_FUNCTION(10,	"mss_gpio5",	NULL,		V_ARMADA_80X0_CP0_PLUS),
		 MPP_VAR_FUNCTION(11,	"wakeup",	"out_cp2cp",	V_ARMADA_80X0_CP0_PLUS)),
	MPP_MODE(44,
		 MPP_VAR_FUNCTION(0,	"gpio",		NULL,		V_ARMADA_80X0_CP0_PLUS),
		 MPP_VAR_FUNCTION(1,	"ge1",		"txd2",		V_ARMADA_80X0_CP0_PLUS),
		 MPP_VAR_FUNCTION(7,	"uart0",	"rts",		V_ARMADA_80X0_CP0_PLUS),
		 MPP_VAR_FUNCTION(11,	"ptp",		"clk_cp2cp",	V_ARMADA_80X0_CP0_PLUS)),
	MPP_MODE(45,
		 MPP_VAR_FUNCTION(0,	"gpio",		NULL,		V_ARMADA_80X0_CP0_PLUS),
		 MPP_VAR_FUNCTION(1,	"ge1",		"txd3",		V_ARMADA_80X0_CP0_PLUS),
		 MPP_VAR_FUNCTION(7,	"uart0",	"txd",		V_ARMADA_80X0_CP0_PLUS),
		 MPP_VAR_FUNCTION(9,	"pcie",		"rstoutn",	V_ARMADA_80X0_CP0_PLUS)),
	MPP_MODE(46,
		 MPP_VAR_FUNCTION(0,	"gpio",		NULL,		V_ARMADA_80X0_CP0_PLUS),
		 MPP_VAR_FUNCTION(1,	"ge1",		"txd1",		V_ARMADA_80X0_CP0_PLUS),
		 MPP_VAR_FUNCTION(7,	"uart1",	"rts",		V_ARMADA_80X0_CP0_PLUS)),
	MPP_MODE(47,
		 MPP_VAR_FUNCTION(0,	"gpio",		NULL,		V_ARMADA_80X0_CP0_PLUS),
		 MPP_VAR_FUNCTION(1,	"ge1",		"txd0",		V_ARMADA_80X0_CP0_PLUS),
		 MPP_VAR_FUNCTION(5,	"spi1",		"clk",		V_ARMADA_80X0_CP0_PLUS),
		 MPP_VAR_FUNCTION(7,	"uart1",	"txd",		V_ARMADA_80X0_CP0_PLUS),
		 MPP_VAR_FUNCTION(8,	"ge",		"mdc",		V_ARMADA_80X0_CP0_PLUS)),
	MPP_MODE(48,
		 MPP_VAR_FUNCTION(0,	"gpio",		NULL,		V_ARMADA_80X0_CP0_PLUS),
		 MPP_VAR_FUNCTION(1,	"ge1",		"txctl_txen",	V_ARMADA_80X0_CP0_PLUS),
		 MPP_VAR_FUNCTION(5,	"spi1",		"mosi",		V_ARMADA_80X0_CP0_PLUS),
		 MPP_VAR_FUNCTION(8,	"xg",		"mdc",		V_ARMADA_80X0_CP0_PLUS),
		 MPP_VAR_FUNCTION(11,	"wakeup",	"in_cp2cp",	V_ARMADA_80X0_CP0_PLUS)),
	MPP_MODE(49,
		 MPP_VAR_FUNCTION(0,	"gpio",		NULL,		V_ARMADA_80X0_CP0_PLUS),
		 MPP_VAR_FUNCTION(1,	"ge1",		"txclkout",	V_ARMADA_80X0_CP0_PLUS),
		 MPP_VAR_FUNCTION(2,	"mii",		"crs",		V_ARMADA_80X0_CP0_PLUS),
		 MPP_VAR_FUNCTION(5,	"spi1",		"miso",		V_ARMADA_80X0_CP0_PLUS),
		 MPP_VAR_FUNCTION(7,	"uart1",	"rxd",		V_ARMADA_80X0_CP0_PLUS),
		 MPP_VAR_FUNCTION(8,	"ge",		"mdio",		V_ARMADA_80X0_CP0_PLUS),
		 MPP_VAR_FUNCTION(9,	"pcie0",	"clkreq",	V_ARMADA_80X0_CP0_PLUS),
		 MPP_VAR_FUNCTION(10,	"sdio",		"v18_en",	V_ARMADA_80X0_CP0_PLUS),
		 MPP_VAR_FUNCTION(11,	"sei",		"out_cp2cp",	V_ARMADA_80X0_CP0_PLUS)),
	MPP_MODE(50,
		 MPP_VAR_FUNCTION(0,	"gpio",		NULL,		V_ARMADA_80X0_CP0_PLUS),
		 MPP_VAR_FUNCTION(1,	"ge1",		"rxclk",	V_ARMADA_80X0_CP0_PLUS),
		 MPP_VAR_FUNCTION(2,	"mss_i2c",	"sda",		V_ARMADA_80X0_CP0_PLUS),
		 MPP_VAR_FUNCTION(5,	"spi1",		"csn0",		V_ARMADA_80X0_CP0_PLUS),
		 MPP_VAR_FUNCTION(6,	"uart2",	"txd",		V_ARMADA_80X0_CP0_PLUS),
		 MPP_VAR_FUNCTION(7,	"uart0",	"rxd",		V_ARMADA_80X0_CP0_PLUS),
		 MPP_VAR_FUNCTION(8,	"xg",		"mdio",		V_ARMADA_80X0_CP0_PLUS),
		 MPP_VAR_FUNCTION(10,	"sdio",		"pwr11",	V_ARMADA_80X0_CP0_PLUS)),
	MPP_MODE(51,
		 MPP_VAR_FUNCTION(0,	"gpio",		NULL,		V_ARMADA_80X0_CP0_PLUS),
		 MPP_VAR_FUNCTION(1,	"ge1",		"rxd0",		V_ARMADA_80X0_CP0_PLUS),
		 MPP_VAR_FUNCTION(2,	"mss_i2c",	"sck",		V_ARMADA_80X0_CP0_PLUS),
		 MPP_VAR_FUNCTION(5,	"spi1",		"csn1",		V_ARMADA_80X0_CP0_PLUS),
		 MPP_VAR_FUNCTION(6,	"uart2",	"rxd",		V_ARMADA_80X0_CP0_PLUS),
		 MPP_VAR_FUNCTION(7,	"uart0",	"cts",		V_ARMADA_80X0_CP0_PLUS),
		 MPP_VAR_FUNCTION(10,	"sdio",		"pwr10",	V_ARMADA_80X0_CP0_PLUS)),
	MPP_MODE(52,
		 MPP_VAR_FUNCTION(0,	"gpio",		NULL,		V_ARMADA_80X0_CP0_PLUS),
		 MPP_VAR_FUNCTION(1,	"ge1",		"rxd1",		V_ARMADA_80X0_CP0_PLUS),
		 MPP_VAR_FUNCTION(2,	"synce1",	"clk",		V_ARMADA_80X0_CP0_PLUS),
		 MPP_VAR_FUNCTION(4,	"synce2",	"clk",		V_ARMADA_80X0_CP0_PLUS),
		 MPP_VAR_FUNCTION(5,	"spi1",		"csn2",		V_ARMADA_80X0_CP0_PLUS),
		 MPP_VAR_FUNCTION(7,	"uart1",	"cts",		V_ARMADA_80X0_CP0_PLUS),
		 MPP_VAR_FUNCTION(8,	"led",		"clk",		V_ARMADA_80X0_CP0_PLUS),
		 MPP_VAR_FUNCTION(9,	"pcie",		"rstoutn",	V_ARMADA_80X0_CP0_PLUS),
		 MPP_VAR_FUNCTION(10,	"pcie0",	"clkreq",	V_ARMADA_80X0_CP0_PLUS)),
	MPP_MODE(53,
		 MPP_VAR_FUNCTION(0,	"gpio",		NULL,		V_ARMADA_80X0_CP0_PLUS),
		 MPP_VAR_FUNCTION(1,	"ge1",		"rxd2",		V_ARMADA_80X0_CP0_PLUS),
		 MPP_VAR_FUNCTION(3,	"ptp",		"clk",		V_ARMADA_80X0_CP0_PLUS),
		 MPP_VAR_FUNCTION(5,	"spi1",		"csn3",		V_ARMADA_80X0_CP0_PLUS),
		 MPP_VAR_FUNCTION(7,	"uart1",	"rxd",		V_ARMADA_80X0_CP0_PLUS),
		 MPP_VAR_FUNCTION(8,	"led",		"stb",		V_ARMADA_80X0_CP0_PLUS),
		 MPP_VAR_FUNCTION(11,	"sdio",		"led",		V_ARMADA_80X0_CP0_PLUS)),
	MPP_MODE(54,
		 MPP_VAR_FUNCTION(0,	"gpio",		NULL,		V_ARMADA_80X0_CP0_PLUS),
		 MPP_VAR_FUNCTION(1,	"ge1",		"rxd3",		V_ARMADA_80X0_CP0_PLUS),
		 MPP_VAR_FUNCTION(2,	"synce2",	"clk",		V_ARMADA_80X0_CP0_PLUS),
		 MPP_VAR_FUNCTION(3,	"ptp",		"pclk_out",	V_ARMADA_80X0_CP0_PLUS),
		 MPP_VAR_FUNCTION(4,	"synce1",	"clk",		V_ARMADA_80X0_CP0_PLUS),
		 MPP_VAR_FUNCTION(8,	"led",		"data",		V_ARMADA_80X0_CP0_PLUS),
		 MPP_VAR_FUNCTION(10,	"sdio",		"hw_rst",	V_ARMADA_80X0_CP0_PLUS),
		 MPP_VAR_FUNCTION(11,	"sdio",		"wr_protect",	V_ARMADA_80X0_CP0_PLUS)),
	MPP_MODE(55,
		 MPP_VAR_FUNCTION(0,	"gpio",		NULL,		V_ARMADA_80X0_CP0_PLUS),
		 MPP_VAR_FUNCTION(1,	"ge1",		"rxctl_rxdv",	V_ARMADA_80X0_CP0_PLUS),
		 MPP_VAR_FUNCTION(3,	"ptp",		"pulse",	V_ARMADA_80X0_CP0_PLUS),
		 MPP_VAR_FUNCTION(10,	"sdio",		"led",		V_ARMADA_80X0_CP0_PLUS),
		 MPP_VAR_FUNCTION(11,	"sdio",		"card_detect",	V_ARMADA_80X0_CP0_PLUS)),
	MPP_MODE(56,
		 MPP_VAR_FUNCTION(0,	"gpio",		NULL,		V_ARMADA_80X0_CP0_PLUS),
		 MPP_VAR_FUNCTION(4,	"tdm",		"drx",		V_ARMADA_80X0_CP0_PLUS),
		 MPP_VAR_FUNCTION(5,	"au",		"i2sdo_spdifo",	V_ARMADA_80X0_CP0_PLUS),
		 MPP_VAR_FUNCTION(6,	"spi0",		"clk",		V_ARMADA_80X0_CP0_PLUS),
		 MPP_VAR_FUNCTION(7,	"uart1",	"rxd",		V_ARMADA_80X0_CP0_PLUS),
		 MPP_VAR_FUNCTION(9,	"sata1",	"present_act",	V_ARMADA_80X0_CP0_PLUS),
		 MPP_VAR_FUNCTION(14,	"sdio",		"clk",		V_ARMADA_80X0_CP0_PLUS)),
	MPP_MODE(57,
		 MPP_VAR_FUNCTION(0,	"gpio",		NULL,		V_ARMADA_80X0_CP0_PLUS),
		 MPP_VAR_FUNCTION(2,	"mss_i2c",	"sda",		V_ARMADA_80X0_CP0_PLUS),
		 MPP_VAR_FUNCTION(3,	"ptp",		"pclk_out",	V_ARMADA_80X0_CP0_PLUS),
		 MPP_VAR_FUNCTION(4,	"tdm",		"intn",		V_ARMADA_80X0_CP0_PLUS),
		 MPP_VAR_FUNCTION(5,	"au",		"i2sbclk",	V_ARMADA_80X0_CP0_PLUS),
		 MPP_VAR_FUNCTION(6,	"spi0",		"mosi",		V_ARMADA_80X0_CP0_PLUS),
		 MPP_VAR_FUNCTION(7,	"uart1",	"txd",		V_ARMADA_80X0_CP0_PLUS),
		 MPP_VAR_FUNCTION(9,	"sata0",	"present_act",	V_ARMADA_80X0_CP0_PLUS),
		 MPP_VAR_FUNCTION(14,	"sdio",		"cmd",		V_ARMADA_80X0_CP0_PLUS)),
	MPP_MODE(58,
		 MPP_VAR_FUNCTION(0,	"gpio",		NULL,		V_ARMADA_80X0_CP0_PLUS),
		 MPP_VAR_FUNCTION(2,	"mss_i2c",	"sck",		V_ARMADA_80X0_CP0_PLUS),
		 MPP_VAR_FUNCTION(3,	"ptp",		"clk",		V_ARMADA_80X0_CP0_PLUS),
		 MPP_VAR_FUNCTION(4,	"tdm",		"rstn",		V_ARMADA_80X0_CP0_PLUS),
		 MPP_VAR_FUNCTION(5,	"au",		"i2sdi",	V_ARMADA_80X0_CP0_PLUS),
		 MPP_VAR_FUNCTION(6,	"spi0",		"miso",		V_ARMADA_80X0_CP0_PLUS),
		 MPP_VAR_FUNCTION(7,	"uart1",	"cts",		V_ARMADA_80X0_CP0_PLUS),
		 MPP_VAR_FUNCTION(8,	"led",		"clk",		V_ARMADA_80X0_CP0_PLUS),
		 MPP_VAR_FUNCTION(14,	"sdio",		"d0",		V_ARMADA_80X0_CP0_PLUS)),
	MPP_MODE(59,
		 MPP_VAR_FUNCTION(0,	"gpio",		NULL,		V_ARMADA_80X0_CP0_PLUS),
		 MPP_VAR_FUNCTION(1,	"mss_gpio7",	NULL,		V_ARMADA_80X0_CP0_PLUS),
		 MPP_VAR_FUNCTION(2,	"synce2",	"clk",		V_ARMADA_80X0_CP0_PLUS),
		 MPP_VAR_FUNCTION(4,	"tdm",		"fsync",	V_ARMADA_80X0_CP0_PLUS),
		 MPP_VAR_FUNCTION(5,	"au",		"i2slrclk",	V_ARMADA_80X0_CP0_PLUS),
		 MPP_VAR_FUNCTION(6,	"spi0",		"csn0",		V_ARMADA_80X0_CP0_PLUS),
		 MPP_VAR_FUNCTION(7,	"uart0",	"cts",		V_ARMADA_80X0_CP0_PLUS),
		 MPP_VAR_FUNCTION(8,	"led",		"stb",		V_ARMADA_80X0_CP0_PLUS),
		 MPP_VAR_FUNCTION(9,	"uart1",	"txd",		V_ARMADA_80X0_CP0_PLUS),
		 MPP_VAR_FUNCTION(14,	"sdio",		"d1",		V_ARMADA_80X0_CP0_PLUS)),
	MPP_MODE(60,
		 MPP_VAR_FUNCTION(0,	"gpio",		NULL,		V_ARMADA_80X0_CP0_PLUS),
		 MPP_VAR_FUNCTION(1,	"mss_gpio6",	NULL,		V_ARMADA_80X0_CP0_PLUS),
		 MPP_VAR_FUNCTION(3,	"ptp",		"pulse",	V_ARMADA_80X0_CP0_PLUS),
		 MPP_VAR_FUNCTION(4,	"tdm",		"dtx",		V_ARMADA_80X0_CP0_PLUS),
		 MPP_VAR_FUNCTION(5,	"au",		"i2smclk",	V_ARMADA_80X0_CP0_PLUS),
		 MPP_VAR_FUNCTION(6,	"spi0",		"csn1",		V_ARMADA_80X0_CP0_PLUS),
		 MPP_VAR_FUNCTION(7,	"uart0",	"rts",		V_ARMADA_80X0_CP0_PLUS),
		 MPP_VAR_FUNCTION(8,	"led",		"data",		V_ARMADA_80X0_CP0_PLUS),
		 MPP_VAR_FUNCTION(9,	"uart1",	"rxd",		V_ARMADA_80X0_CP0_PLUS),
		 MPP_VAR_FUNCTION(14,	"sdio",		"d2",		V_ARMADA_80X0_CP0_PLUS)),
	MPP_MODE(61,
		 MPP_VAR_FUNCTION(0,	"gpio",		NULL,		V_ARMADA_80X0_CP0_PLUS),
		 MPP_VAR_FUNCTION(1,	"mss_gpio5",	NULL,		V_ARMADA_80X0_CP0_PLUS),
		 MPP_VAR_FUNCTION(3,	"ptp",		"clk",		V_ARMADA_80X0_CP0_PLUS),
		 MPP_VAR_FUNCTION(4,	"tdm",		"pclk",		V_ARMADA_80X0_CP0_PLUS),
		 MPP_VAR_FUNCTION(5,	"au",		"i2sextclk",	V_ARMADA_80X0_CP0_PLUS),
		 MPP_VAR_FUNCTION(6,	"spi0",		"csn2",		V_ARMADA_80X0_CP0_PLUS),
		 MPP_VAR_FUNCTION(7,	"uart0",	"txd",		V_ARMADA_80X0_CP0_PLUS),
		 MPP_VAR_FUNCTION(8,	"uart2",	"txd",		V_ARMADA_80X0_CP0_PLUS),
		 MPP_VAR_FUNCTION(9,	"sata1",	"present_act",	V_ARMADA_80X0_CP0_PLUS),
		 MPP_VAR_FUNCTION(10,	"ge",		"mdio",		V_ARMADA_80X0_CP0_PLUS),
		 MPP_VAR_FUNCTION(14,	"sdio",		"d3",		V_ARMADA_80X0_CP0_PLUS)),
	MPP_MODE(62,
		 MPP_VAR_FUNCTION(0,	"gpio",		NULL,		V_ARMADA_80X0_CP0_PLUS),
		 MPP_VAR_FUNCTION(1,	"mss_gpio4",	NULL,		V_ARMADA_80X0_CP0_PLUS),
		 MPP_VAR_FUNCTION(2,	"synce1",	"clk",		V_ARMADA_80X0_CP0_PLUS),
		 MPP_VAR_FUNCTION(3,	"ptp",		"pclk_out",	V_ARMADA_80X0_CP0_PLUS),
		 MPP_VAR_FUNCTION(5,	"sata1",	"present_act",	V_ARMADA_80X0_CP0_PLUS),
		 MPP_VAR_FUNCTION(6,	"spi0",		"csn3",		V_ARMADA_80X0_CP0_PLUS),
		 MPP_VAR_FUNCTION(7,	"uart0",	"rxd",		V_ARMADA_80X0_CP0_PLUS),
		 MPP_VAR_FUNCTION(8,	"uart2",	"rxd",		V_ARMADA_80X0_CP0_PLUS),
		 MPP_VAR_FUNCTION(9,	"sata0",	"present_act",	V_ARMADA_80X0_CP0_PLUS),
		 MPP_VAR_FUNCTION(10,	"ge",		"mdc",		V_ARMADA_80X0_CP0_PLUS)),
};

static const struct of_device_id armada_cp110_pinctrl_of_match[] = {
	{
		.compatible	= "marvell,a70x0-pinctrl",
		.data		= (void *) V_ARMADA_70X0,
	},
	{
		.compatible	= "marvell,a80x0-cp0-pinctrl",
		.data		= (void *) V_ARMADA_80X0_CP0,
	},
	{
		.compatible	= "marvell,a80x0-cp1-pinctrl",
		.data		= (void *) V_ARMADA_80X0_CP1,
	},
	{ },
};

static struct mvebu_mpp_ctrl armada_cp110_0_mpp_controls[] = {
	MPP_FUNC_CTRL(0, 62, NULL, armada_cp110_0_mpp_ctrl),
};

static struct mvebu_mpp_ctrl armada_cp110_1_mpp_controls[] = {
	MPP_FUNC_CTRL(0, 62, NULL, armada_cp110_1_mpp_ctrl),
};

static struct pinctrl_gpio_range armada_cp110_0_mpp_gpio_ranges[] = {
	MPP_GPIO_RANGE(0, 0, 20, 32),
	MPP_GPIO_RANGE(1, 32, 52, 31),
};

static struct pinctrl_gpio_range armada_cp110_1_mpp_gpio_ranges[] = {
	MPP_GPIO_RANGE(0, 0, 20, 32),
};

static int armada_cp110_pinctrl_probe(struct platform_device *pdev)
{
	struct mvebu_pinctrl_soc_info *soc;
	struct mvebu_pinctrl_pm_save *pm_save;
	const struct of_device_id *match =
		of_match_device(armada_cp110_pinctrl_of_match, &pdev->dev);
	struct resource *res;

	if (!match)
		return -ENODEV;

	soc = devm_kzalloc(&pdev->dev,
			   sizeof(struct mvebu_pinctrl_soc_info), GFP_KERNEL);
	if (!soc)
		return -ENOMEM;

	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);

	soc->variant = (u64)match->data & 0xff;

	switch (soc->variant) {
	case V_ARMADA_70X0:
	case V_ARMADA_80X0_CP0:
		cp0_mpp_base = devm_ioremap_resource(&pdev->dev, res);
		if (IS_ERR(cp0_mpp_base))
			return PTR_ERR(cp0_mpp_base);
		soc->controls = armada_cp110_0_mpp_controls;
		soc->ncontrols = ARRAY_SIZE(armada_cp110_0_mpp_controls);
		soc->gpioranges = armada_cp110_0_mpp_gpio_ranges;
		soc->ngpioranges = ARRAY_SIZE(armada_cp110_0_mpp_gpio_ranges);
		soc->modes = armada_cp110_mpp_modes;
		soc->nmodes = armada_cp110_0_mpp_controls[0].npins;
		break;
	case V_ARMADA_80X0_CP1:
		cp1_mpp_base = devm_ioremap_resource(&pdev->dev, res);
		if (IS_ERR(cp1_mpp_base))
			return PTR_ERR(cp1_mpp_base);
		soc->controls = armada_cp110_1_mpp_controls;
		soc->ncontrols = ARRAY_SIZE(armada_cp110_1_mpp_controls);
		soc->gpioranges = armada_cp110_1_mpp_gpio_ranges;
		soc->ngpioranges = ARRAY_SIZE(armada_cp110_1_mpp_gpio_ranges);
		soc->modes = armada_cp110_mpp_modes;
		soc->nmodes = armada_cp110_1_mpp_controls[0].npins;
		break;
	}

#ifdef CONFIG_PM
	pm_save = devm_kzalloc(&pdev->dev,
			       sizeof(struct mvebu_pinctrl_pm_save),
			       GFP_KERNEL);
	if (!pm_save)
		return -ENOMEM;

	pm_save->length = resource_size(res);
	/* Allocate memory to save the register value before suspend. */
	pm_save->regs = (unsigned int *)devm_kzalloc(&pdev->dev,
						     pm_save->length,
						     GFP_KERNEL);
	if (!pm_save->regs)
		return -ENOMEM;

	soc->pm_save = pm_save;
#endif /* CONFIG_PM */

	pdev->dev.platform_data = soc;

	/* Add to the global list */
	list_add_tail(&soc->node, &drvdata_list);

	return mvebu_pinctrl_probe(pdev);
}

static int armada_cp110_pinctrl_remove(struct platform_device *pdev)
{
	return mvebu_pinctrl_remove(pdev);
}

#ifdef CONFIG_PM
/* armada_cp110_pinctrl_suspend - save pinctrl register for suspend */
static int armada_cp110_pinctrl_suspend(void)
{
	struct mvebu_pinctrl_soc_info *soc;
	void __iomem *mpp_base;

	list_for_each_entry(soc, &drvdata_list, node) {
		unsigned int offset, i = 0;

		mpp_base = (soc->variant == V_ARMADA_80X0_CP1) ? cp1_mpp_base :
								 cp0_mpp_base;
		for (offset = 0; offset < soc->pm_save->length;
		     offset += sizeof(unsigned int))
			soc->pm_save->regs[i++] = readl(mpp_base + offset);
	}

	return 0;
}

/* armada_cp110_pinctrl_resume - restore pinctrl register for suspend */
static void armada_cp110_pinctrl_resume(void)
{
	struct mvebu_pinctrl_soc_info *soc;
	void __iomem *mpp_base;

	list_for_each_entry_reverse(soc, &drvdata_list, node) {
		unsigned int offset, i = 0;

		mpp_base = (soc->variant == V_ARMADA_80X0_CP1) ? cp1_mpp_base :
								 cp0_mpp_base;
		for (offset = 0; offset < soc->pm_save->length;
		     offset += sizeof(unsigned int))
			writel(soc->pm_save->regs[i++], mpp_base + offset);
	}
}

#else
#define armada_cp110_pinctrl_suspend		NULL
#define armada_cp110_pinctrl_resume		NULL
#endif /* CONFIG_PM */

static struct syscore_ops armada_cp110_pinctrl_syscore_ops = {
	.suspend	= armada_cp110_pinctrl_suspend,
	.resume		= armada_cp110_pinctrl_resume,
};

static struct platform_driver armada_cp110_pinctrl_driver = {
	.driver = {
		.name = "armada-cp110-pinctrl",
		.of_match_table = of_match_ptr(armada_cp110_pinctrl_of_match),
	},
	.probe = armada_cp110_pinctrl_probe,
	.remove = armada_cp110_pinctrl_remove,
};

static int __init armada_cp110_pinctrl_drv_register(void)
{
	/*
	 * Register syscore ops for save/restore of registers across suspend.
	 * It's important to ensure that this driver is running at an earlier
	 * initcall level than any arch-specific init calls that install syscore
	 * ops that turn off pad retention.
	 */
	register_syscore_ops(&armada_cp110_pinctrl_syscore_ops);

	return platform_driver_register(&armada_cp110_pinctrl_driver);
}
postcore_initcall(armada_cp110_pinctrl_drv_register);

static void __exit armada_cp110_pinctrl_drv_unregister(void)
{
	platform_driver_unregister(&armada_cp110_pinctrl_driver);
}
module_exit(armada_cp110_pinctrl_drv_unregister);

MODULE_AUTHOR("Hanna Hawa <hannah@marvell.com>");
MODULE_DESCRIPTION("Marvell Armada CP-110 pinctrl driver");
MODULE_LICENSE("GPL v2");
