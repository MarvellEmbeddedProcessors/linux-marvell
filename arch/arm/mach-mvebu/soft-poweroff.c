/*
 * Soft power off support for Armada 375 platforms.
 *
 * this file adds WFI state support, with wakeup via UART/NETA IRQ and reset machine
 * usage : 'echo 1 > /sys/devices/platform/mv_power_wol/soft_power_idle'
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
#include <linux/reboot.h>
#include <linux/perf_event.h>
#include <linux/io.h>
#include <linux/of_address.h>
#include <linux/irq.h>

#define IRQ_PRIV_GIC_UART_IRQ		44
#define IRQ_PRIV_GIC_GBE0_WOL_IRQ	112
#define NR_GIC_IRQ			118

static void soft_poweroff_unmask_irq(unsigned int irq_number)
{
	struct irq_data *irqd = irq_get_irq_data(irq_number);

	if (irqd && irqd->chip && irqd->chip->irq_unmask)
		irqd->chip->irq_unmask(irqd);
}

static void soft_poweroff_mask_all_irq(void)
{
	struct irq_data *irqd;
	int i;

	for (i = 0; i < NR_GIC_IRQ; i++) {
		irqd = irq_get_irq_data(i);
		if (irqd && irqd->chip && irqd->chip->irq_unmask)
			irqd->chip->irq_mask(irqd);
	}
}

static void soft_poweroff_pm_power_off(void)
{

	local_irq_disable();
	soft_poweroff_mask_all_irq();
	soft_poweroff_unmask_irq(IRQ_PRIV_GIC_UART_IRQ);
	soft_poweroff_unmask_irq(IRQ_PRIV_GIC_GBE0_WOL_IRQ);

	pr_info("\nEntering WFI state..\n");
	wfi();
	pr_info("\nRecovered from WFI state..\nRestarting machine.\n");

	machine_restart(NULL);
}

static ssize_t soft_poweroff(struct device *dev,
				   struct device_attribute *attr, const char *buf, size_t len)
{
	int a = 0, err = 0, ret = 0;
	struct pid_namespace *pid_ns = task_active_pid_ns(current);

	sscanf(buf, "%x", &a);

	if (a == 1) {
		pr_info("\n pm_power_off() Enabled.. simulating Syscall 'poweroff' and entering WFI state");
		pm_power_off = soft_poweroff_pm_power_off;
		ret = reboot_pid_ns(pid_ns, LINUX_REBOOT_CMD_POWER_OFF);
		if (ret)
			return ret;
		kernel_power_off();
		do_exit(0);
	} else if (a == 2) {
		pr_info("\n pm_power_off() Enabled --> use 'poweroff' Syscall to enter WFI state");
		pm_power_off = soft_poweroff_pm_power_off;
	}

	return err ? -EINVAL : len;
}

static DEVICE_ATTR(soft_poweroff, 00200 , NULL, soft_poweroff);

static struct attribute *mv_power_wol_attrs[] = {
	&dev_attr_soft_poweroff.attr,
	NULL
};

static struct attribute_group mv_power_wol_group = {
	.name = "mv_power_wol",
	.attrs = mv_power_wol_attrs,
};

int __init power_idle_sysfs_init(void)
{
		int err;
		struct device *pd;

		pd = &platform_bus;

		err = sysfs_create_group(&pd->kobj, &mv_power_wol_group);
		if (err) {
			pr_info("power idle sysfs group failed %d\n", err);
			goto out;
		}
out:
		return err;
}

module_init(power_idle_sysfs_init);
