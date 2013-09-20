/*
 * Device Tree support for Armada 375 platforms.
 *
 * Copyright (C) 2013 Marvell
 *
 * Lior Amsalem <alior@marvell.com>
 * Gregory CLEMENT <gregory.clement@free-electrons.com>
 * Thomas Petazzoni <thomas.petazzoni@free-electrons.com>
 *
 * This file is licensed under the terms of the GNU General Public
 * License version 2.  This program is licensed "as is" without any
 * warranty of any kind, whether express or implied.
 */

#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/of_address.h>
#include <linux/of_platform.h>
#include <linux/clocksource.h>
#include <linux/io.h>
#include <linux/clk/mvebu.h>
#include <linux/dma-mapping.h>
#include <linux/mbus.h>
#include <linux/irqchip.h>
#include <asm/hardware/cache-l2x0.h>
#include <asm/mach/arch.h>
#include <asm/mach/map.h>
#include <asm/mach/time.h>
#include "common.h"
#include "coherency.h"

static void __init armada_375_timer_and_clk_init(void)
{
	mvebu_clocks_init();
	clocksource_of_init();
	coherency_init();
	BUG_ON(mvebu_mbus_dt_init(coherency_available()));
	l2x0_of_init(0, ~0UL);
}

static const char * const armada_375_dt_compat[] = {
	"marvell,armada375",
	NULL,
};

DT_MACHINE_START(ARMADA_375_DT, "Marvell Armada 375 (Device Tree)")
	.map_io		= debug_ll_io_init,
	.init_irq	= irqchip_init,
	.init_time	= armada_375_timer_and_clk_init,
	.restart	= mvebu_restart,
	.dt_compat	= armada_375_dt_compat,
MACHINE_END
