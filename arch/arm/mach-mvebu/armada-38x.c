/*
 * Device Tree support for Armada 380/385 platforms.
 *
 * Copyright (C) 2013 Marvell
 *
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
#include <asm/smp_scu.h>
#include <asm/mach/time.h>
#include <asm/signal.h>
#include "common.h"
#include "coherency.h"
#include "armada-380.h"

static struct of_device_id of_scu_table[] = {
	{ .compatible = "arm,cortex-a9-scu" },
	{ },
};

#define SCU_CTRL		0x00

static void __init armada_380_scu_enable(void)
{
	void __iomem *scu_base;
	u32 scu_ctrl;

	struct device_node *np = of_find_matching_node(NULL, of_scu_table);
	if (np) {
		scu_base = of_iomap(np, 0);

		scu_ctrl = __raw_readl(scu_base + SCU_CTRL);
		/* already enabled? */
		if (!(scu_ctrl & 1)) {
			/* Enable SCU Speculative linefills to L2 */
			scu_ctrl |= (1 << 3);
			__raw_writel(scu_ctrl, scu_base + SCU_CTRL);
		}
		scu_enable(scu_base);
	}
}

void __init armada_380_l2_enable(void)
{
	void __iomem *l2x0_base;
	struct device_node *np;
	unsigned int val;

	np = of_find_compatible_node(NULL, NULL, "arm,pl310-cache");
	if (!np)
		goto out;

	l2x0_base = of_iomap(np, 0);
	if (!l2x0_base) {
		of_node_put(np);
		goto out;
	}

	/* Configure the L2 PREFETCH and POWER registers */
	val = 0x48800000;
	/*
	*  Support the following configuration:
	*  Incr double linefill enable
	*  Double linefill enable
	*  Double linefill on WRAP disable
	*  NO prefetch drop enable
	 */
	writel_relaxed(val, l2x0_base + L2X0_PREFETCH_CTRL);
	val = L2X0_DYNAMIC_CLK_GATING_EN;
	writel_relaxed(val, l2x0_base + L2X0_POWER_CTRL);

	iounmap(l2x0_base);
	of_node_put(np);
out:
	if (coherency_available())
		l2x0_of_init_coherent(0, ~0UL);
	else
		l2x0_of_init(0, ~0UL);
}

static void __iomem *
armada_380_ioremap_caller(unsigned long phys_addr, size_t size,
			  unsigned int mtype, void *caller)
{
	struct resource pcie_mem;

	mvebu_mbus_get_pcie_mem_aperture(&pcie_mem);

	if (pcie_mem.start <= phys_addr && (phys_addr + size) <= pcie_mem.end)
		mtype = MT_MEMORY_SO;

	return __arm_ioremap_caller(phys_addr, size, mtype, caller);
}

static void __init armada_380_timer_and_clk_init(void)
{
	pr_notice("\n  LSP version: %s\n\n", LSP_VERSION);

	mvebu_clocks_init();
	clocksource_of_init();
	armada_380_scu_enable();
	BUG_ON(mvebu_mbus_dt_init(coherency_available()));
	arch_ioremap_caller = armada_380_ioremap_caller;
	pci_ioremap_set_mem_type(MT_MEMORY_SO);
	coherency_init();
	armada_380_l2_enable();
}

static const char * const armada_380_dt_compat[] = {
	"marvell,armada380",
	"marvell,armada385",
	"marvell,armada388",
	NULL,
};

DT_MACHINE_START(ARMADA_XP_DT, "Marvell Armada 380/385/388 (Device Tree)")
	.smp		= smp_ops(armada_380_smp_ops),
	.map_io		= debug_ll_io_init,
	.init_irq	= irqchip_init,
	.init_time	= armada_380_timer_and_clk_init,
	.restart	= mvebu_restart,
	.dt_compat	= armada_380_dt_compat,
	.flags          = (MACHINE_NEEDS_CPOLICY_WRITEALLOC |
			   MACHINE_NEEDS_SHAREABLE_PAGES),
MACHINE_END
