/*
* Suspend/resume
*
* Copyright (C) 2014 Marvell
*
* Thomas Petazzoni <thomas.petazzoni@free-electrons.com>
*
* This file is licensed under the terms of the GNU General Public
* License version 2.  This program is licensed "as is" without any
* warranty of any kind, whether express or implied.
*/

#include <linux/cpu_pm.h>
#include <linux/init.h>
#include <linux/io.h>
#include <linux/suspend.h>

#include <asm/cacheflush.h>
#include <asm/proc-fns.h>
#include <asm/suspend.h>
#include "coherency.h"
#include <linux/mbus.h>
#include <linux/clk/mvebu.h>
#include <linux/of_address.h>

static void __iomem *ddr_base;
static void __iomem *gpio_base;

static struct of_device_id of_pm_table[] = {
	{.compatible = "marvell,armada-380-pm"},
	{ /* end of list */ },
};

extern void armada_380_scu_enable(void);
extern void armada_38x_cpu_mem_resume(void);
extern void armada_380_resume(void);
extern void armada_380_timer_resume(void);
extern void mvebu_mbus_suspend(void);
extern void mvebu_mbus_resume(void);
extern int armada_38x_cpuidle_init(void);
extern void mvebu_pcie_suspend(void);
extern void mvebu_pcie_resume(void);

/*
 * Store boot information used by bin header
 */
#define BOOT_INFO_ADDR		(0x3000)
#define BOOT_MAGIC_WORD	(0xDEADB002)
#define REG_LIST_END		(0xFFFFFFFF)
#define BOOTROM_INTER_REGS_PHYS_BASE	(0xD0000000)
#define INTER_REGS_PHYS_BASE	(0xF1000000)

#define SDRAM_WIN_BASE_REG(x)	(0x20180 + (0x8*x))
#define SDRAM_WIN_CTRL_REG(x)	(0x20184 + (0x8*x))
#define MAX_CS_COUNT		4

extern void mvebu_mbus_get_sdram_window(int win, u32 *base, u32 *size);
void armada_store_boot_info(void)
{
	int *store_addr = (int *)BOOT_INFO_ADDR;
	int *resume_pc, win;
	u32 base, size;

	store_addr = (int *)phys_to_virt((int)store_addr);
	resume_pc = (int *)virt_to_phys(armada_38x_cpu_mem_resume);
	/*
	* Store magic word indicating suspend to ram and return address.
	* Use writel to avoid endianes issues
	*/
	writel((int)(BOOT_MAGIC_WORD), store_addr++);
	writel((int)(resume_pc), store_addr++);

	/*
	* Now store registers that need to be proggrammed before
	* comming back to linux. format is addr->value
	*/

	/* Save AXI windows data */
	for (win = 0; win < 4; win++) {
		mvebu_mbus_get_sdram_window(win, &base, &size);

		writel(BOOTROM_INTER_REGS_PHYS_BASE + SDRAM_WIN_BASE_REG(win), store_addr++);
		writel(base, store_addr++);

		writel(BOOTROM_INTER_REGS_PHYS_BASE + SDRAM_WIN_CTRL_REG(win), store_addr++);
		writel(size, store_addr++);
	}

	/* Mark the end of the boot info*/
	writel(REG_LIST_END, store_addr);
}

/* Function defined in coherency_ll.S */
int enter_mem_suspend(void __iomem *ddr_addr, void __iomem *gpio_addr);

static int mvebu_powerdown(unsigned long val)
{
	/* flush L1 dcache: v7_flush_kern_cache_all */
	v7_flush_kern_cache_all();

	/* flush L2 */
	outer_flush_all();

	enter_mem_suspend((void *)ddr_base, (void *)gpio_base);

	return 0;
}

static int mvebu_pm_enter(suspend_state_t state)
{
	pr_info("Suspending Armada 38x\n");

	cpu_pm_enter();
	cpu_cluster_pm_enter();

	armada_store_boot_info();

	mvebu_pcie_suspend();
	mvebu_mbus_suspend();

	cpu_suspend(0, mvebu_powerdown);

	pr_info("Restoring Armada 38x\n");

	armada_380_scu_enable();

	mvebu_mbus_resume();
	armada_380_timer_resume();
	mvebu_pcie_resume();

	cpu_cluster_pm_exit();
	cpu_pm_exit();

	armada_38x_cpuidle_init();

	return 0;
}

static const struct platform_suspend_ops mvebu_pm_ops = {
	.enter = mvebu_pm_enter,
	.valid = suspend_valid_only_mem,
};

static int __init mvebu_pm_init(void)
{
	struct device_node *np;

	np = of_find_matching_node(NULL, of_pm_table);
	if (np) {
		pr_info("Initializing Suspend to RAM\n");
		ddr_base = of_iomap(np, 0);
		gpio_base = of_iomap(np, 1);
		of_node_put(np);
	}
	suspend_set_ops(&mvebu_pm_ops);

	return 0;
}

arch_initcall(mvebu_pm_init);
