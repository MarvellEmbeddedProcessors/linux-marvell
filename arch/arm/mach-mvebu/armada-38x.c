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
#include <linux/of_fdt.h>
#include <linux/of_platform.h>
#include <linux/clocksource.h>
#include <linux/io.h>
#include <linux/clk/mvebu.h>
#include <linux/dma-mapping.h>
#include <linux/memblock.h>
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

extern void __iomem *scu_base;

static struct of_device_id of_scu_table[] = {
	{ .compatible = "arm,cortex-a9-scu" },
	{ },
};

#define SCU_CTRL		0x00

void armada_380_scu_enable(void)
{
	u32 scu_ctrl;

	struct device_node *np = of_find_matching_node(NULL, of_scu_table);
	if (np) {
		scu_base = of_iomap(np, 0);

		scu_ctrl = readl_relaxed(scu_base + SCU_CTRL);
		/* already enabled? */
		if (!(scu_ctrl & 1)) {
			/* Enable SCU Speculative linefills to L2 */
			scu_ctrl |= (1 << 3);
			writel_relaxed(scu_ctrl, scu_base + SCU_CTRL);
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
	val = 0x58800000;
	/*
	*  Support the following configuration:
	*  Incr double linefill enable
	*  Data prefetch enable
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

/*
 * When returning from suspend, the platform goes through the
 * bootloader, which executes its DDR3 training code. This code has
 * the unfortunate idea of using the first 10 KB of each DRAM bank to
 * exercise the RAM and calculate the optimal timings. Therefore, this
 * area of RAM is overwritten, and shouldn't be used by the kernel if
 * suspend/resume is supported.
 */

#ifdef CONFIG_SUSPEND
#define MVEBU_DDR_TRAINING_AREA_SZ (10 * SZ_1K)
static int __init mvebu_scan_mem(unsigned long node, const char *uname,
				 int depth, void *data)
{
	const char *type = of_get_flat_dt_prop(node, "device_type", NULL);
	const __be32 *reg, *endp;
	int l;

	if (type == NULL || strcmp(type, "memory"))
		return 0;

	reg = of_get_flat_dt_prop(node, "linux,usable-memory", &l);
	if (reg == NULL)
		reg = of_get_flat_dt_prop(node, "reg", &l);
	if (reg == NULL)
		return 0;

	endp = reg + (l / sizeof(__be32));
	while ((endp - reg) >= (dt_root_addr_cells + dt_root_size_cells)) {
		u64 base, size;

		base = dt_mem_next_cell(dt_root_addr_cells, &reg);
		size = dt_mem_next_cell(dt_root_size_cells, &reg);

		memblock_reserve(base, MVEBU_DDR_TRAINING_AREA_SZ);
	}

	return 0;
}

static void __init mvebu_memblock_reserve(void)
{
	of_scan_flat_dt(mvebu_scan_mem, NULL);
}
#else
static void __init mvebu_memblock_reserve(void) {}
#endif



DT_MACHINE_START(ARMADA_XP_DT, "Marvell Armada 380/385/388 (Device Tree)")
	.smp		= smp_ops(armada_380_smp_ops),
	.map_io		= debug_ll_io_init,
	.init_irq	= irqchip_init,
	.init_time	= armada_380_timer_and_clk_init,
	.restart	= mvebu_restart,
	.reserve        = mvebu_memblock_reserve,
	.dt_compat	= armada_380_dt_compat,
	.flags          = (MACHINE_NEEDS_CPOLICY_WRITEALLOC |
			   MACHINE_NEEDS_SHAREABLE_PAGES),
MACHINE_END
