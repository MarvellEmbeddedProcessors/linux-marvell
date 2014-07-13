/*
 * Marvell Armada 370, 38x and XP SoC cpuidle driver
 *
 * Copyright (C) 2014 Marvell
 *
 * Nadav Haklai <nadavh@marvell.com>
 * Gregory CLEMENT <gregory.clement@free-electrons.com>
 *
 * This file is licensed under the terms of the GNU General Public
 * License version 2.  This program is licensed "as is" without any
 * warranty of any kind, whether express or implied.
 *
 * Maintainer: Gregory CLEMENT <gregory.clement@free-electrons.com>
 */

#include <linux/cpu_pm.h>
#include <linux/cpuidle.h>
#include <linux/module.h>
#include <linux/mvebu-v7-cpuidle.h>
#include <linux/of.h>
#include <linux/suspend.h>
#include <linux/platform_device.h>
#include <asm/cpuidle.h>

#define MVEBU_V7_FLAG_DEEP_IDLE	0x10000

static struct mvebu_v7_cpuidle *pcpuidle;

static int mvebu_v7_enter_idle(struct cpuidle_device *dev,
				struct cpuidle_driver *drv,
				int index)
{
	int ret;
	bool deepidle = false;

	cpu_pm_enter();

	if (drv->states[index].flags & MVEBU_V7_FLAG_DEEP_IDLE)
		deepidle = true;

	ret = pcpuidle->cpu_suspend(deepidle);
	if (ret)
		return ret;

	cpu_pm_exit();

	return index;
}

static struct cpuidle_driver armadaxp_cpuidle_driver = {
	.name			= "armada_xp_idle",
	.states[0]		= ARM_CPUIDLE_WFI_STATE,
	.states[1]		= {
		.enter			= mvebu_v7_enter_idle,
		.exit_latency		= 10,
		.power_usage		= 50,
		.target_residency	= 100,
		.flags			= CPUIDLE_FLAG_TIME_VALID,
		.name			= "Idle",
		.desc			= "CPU power down",
	},
	.states[2]		= {
		.enter			= mvebu_v7_enter_idle,
		.exit_latency		= 100,
		.power_usage		= 5,
		.target_residency	= 1000,
		.flags			= (CPUIDLE_FLAG_TIME_VALID |
					   MVEBU_V7_FLAG_DEEP_IDLE),
		.name			= "Deep idle",
		.desc			= "CPU and L2 Fabric power down",
	},
	.state_count = 3,
};

static struct cpuidle_driver armada370_cpuidle_driver = {
	.name			= "armada_370_idle",
	.states[0]		= ARM_CPUIDLE_WFI_STATE,
	.states[1]		= {
		.enter			= mvebu_v7_enter_idle,
		.exit_latency		= 100,
		.power_usage		= 5,
		.target_residency	= 1000,
		.flags			= (CPUIDLE_FLAG_TIME_VALID |
					   MVEBU_V7_FLAG_DEEP_IDLE),
		.name			= "Deep Idle",
		.desc			= "CPU and L2 Fabric power down",
	},
	.state_count = 2,
};

static struct cpuidle_driver armada38x_cpuidle_driver = {
	.name			= "armada_38x_idle",
	.states[0]		= ARM_CPUIDLE_WFI_STATE,
	.states[1]		= {
		.enter			= mvebu_v7_enter_idle,
		.exit_latency		= 10,
		.power_usage		= 5,
		.target_residency	= 100,
		.flags			= CPUIDLE_FLAG_TIME_VALID,
		.name			= "Idle",
		.desc			= "CPU and SCU power down",
	},
	.state_count = 2,
};

static int mvebu_v7_cpuidle_probe(struct platform_device *pdev)
{
	struct cpuidle_driver *drv;

	pcpuidle = pdev->dev.platform_data;

	if (pcpuidle->type == CPUIDLE_ARMADA_XP)
		drv = &armadaxp_cpuidle_driver;
	else if (pcpuidle->type == CPUIDLE_ARMADA_370)
		drv = &armada370_cpuidle_driver;
	else if (pcpuidle->type == CPUIDLE_ARMADA_38X)
		drv = &armada38x_cpuidle_driver;
	else
		return -EINVAL;

	return cpuidle_register(drv, NULL);
}

static struct platform_driver mvebu_v7_cpuidle_plat_driver = {
	.driver = {
		.name = "cpuidle-mvebu-v7",
		.owner = THIS_MODULE,
	},
	.probe = mvebu_v7_cpuidle_probe,
};

module_platform_driver(mvebu_v7_cpuidle_plat_driver);

MODULE_AUTHOR("Gregory CLEMENT <gregory.clement@free-electrons.com>");
MODULE_DESCRIPTION("Marvel EBU v7 cpuidle driver");
MODULE_LICENSE("GPL");
