/*
 * Marvell Armada 3700 AVS driver
 *
 * Copyright (C) 2017 Marvell
 *
 * Evan Wang <xswang@marvell.com>
 *
 * This file is licensed under the terms of the GNU General Public
 * License version 2.  This program is licensed "as is" without any
 * warranty of any kind, whether express or implied.
 */

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/device.h>
#include <linux/err.h>
#include <linux/platform_device.h>
#include <linux/regulator/driver.h>
#include <linux/regulator/machine.h>
#include <linux/clk.h>
#include <linux/err.h>
#include <linux/interrupt.h>
#include <linux/io.h>
#include <linux/of_address.h>
#include <linux/of_device.h>
#include <linux/slab.h>

/*
 * A3700 AVS
 * -------------------------------------------
 * | CPU load level |      CPU VDD Source    |
 * -------------------------------------------
 * |    Level0      |         VDD_SET0       |
 * -------------------------------------------
 * |    Level1      |         VDD_SET1       |
 * -------------------------------------------
 * |    Level2      |         VDD_SET2       |
 * -------------------------------------------
 * |    Level3      |         VDD_SET3       |
 * -------------------------------------------
 */

/* AVS registers */
#define A3700_AVS_CTRL_0		(0x0)
#define  AVS_SOFT_RESET			(BIT(31))
#define  AVS_ENABLE			(BIT(30))
#define  AVS_SPEED_TARGET_MASK		(0x0000FFFF)
#define  AVS_HIGH_VDD_LIMIT_OFFS	(16)
#define  AVS_LOW_VDD_LIMIT_OFFS		(22)
#define  AVS_VDD_MASK			(0x3F)
#define A3700_AVS_CTRL_2		(0x8)
#define  AVS_LOW_VDD_EN			(BIT(6))
#define A3700_AVS_VSET(x)		(0x1C + 4 * (x - 1))

#define MAX_VMIN_OPTION			0x34
#define MHZ_TO_HZ			1000000

enum armada_3700_avs_lp_mode {
	LOW_VDD_MODE = 0,
	HOLD_MODE,
};

enum armada_3700_avs_vdd_set {
	VDD_SET0 = 0,
	VDD_SET1,
	VDD_SET2,
	VDD_SET3,
	VDD_SET_MAX,
};

enum armada_3700_cpu_freq_level {
	CPU_FREQ_LEVEL_600MHZ,
	CPU_FREQ_LEVEL_800MHZ,
	CPU_FREQ_LEVEL_1000MHZ,
	CPU_FREQ_LEVEL_1200MHZ,
	MAX_CPU_FREQ_LEVEL_NUM,
};

enum armada_3700_cpu_freq {
	CPU_FREQ_600MHZ = 600,
	CPU_FREQ_800MHZ = 800,
	CPU_FREQ_1000MHZ = 1000,
	CPU_FREQ_1200MHZ = 1200,
};

struct armada_3700_avs {
	struct regulator_desc desc;
	void __iomem *base;
	enum armada_3700_avs_lp_mode mode; /* low power mode */
	enum armada_3700_cpu_freq freq_level;
};

struct armada_3700_avs_vol_map {
	u32 avs_val; /* AVS value for VDD set */
	u32 volt_m; /* The corresponding voltage(mv) to AVS value */
};

/* The VDD min for each CPU frequency, from HW designer */
static int voltage_m_tbl[MAX_CPU_FREQ_LEVEL_NUM][VDD_SET_MAX] = {
	{1108, 1050, 1050, 1050}, /* 600MHZ */
	{1108, 1050, 1050, 1050}, /* 800MHZ */
	{1155, 1050, 1050, 1050}, /* 1000MHZ */
	{1202, 1108, 1050, 1050}, /* 1200MHZ */
};

/* The corresponding relationship between avs value and volatge for each CPU frequency, from HW designer */
static struct armada_3700_avs_vol_map avs_value_voltage_map[MAX_VMIN_OPTION] = {
	{0x0,  747},
	{0x1,  758},
	{0x2,  770},
	{0x3,  782},
	{0x4,  793},
	{0x5,  805},
	{0x6,  817},
	{0x7,  828},
	{0x8,  840},
	{0x9,  852},
	{0xa,  863},
	{0xb,  875},
	{0xc,  887},
	{0xd,  898},
	{0xe,  910},
	{0xf,  922},
	{0x10, 933},
	{0x11, 945},
	{0x12, 957},
	{0x13, 968},
	{0x14, 980},
	{0x15, 992},
	{0x16, 1003},
	{0x17, 1015},
	{0x18, 1027},
	{0x19, 1038},
	{0x1a, 1050},
	{0x1b, 1062},
	{0x1c, 1073},
	{0x1d, 1085},
	{0x1e, 1097},
	{0x1f, 1108},
	{0x20, 1120},
	{0x21, 1132},
	{0x22, 1143},
	{0x23, 1155},
	{0x24, 1167},
	{0x25, 1178},
	{0x26, 1190},
	{0x27, 1202},
	{0x28, 1213},
	{0x29, 1225},
	{0x2a, 1237},
	{0x2b, 1248},
	{0x2c, 1260},
	{0x2d, 1272},
	{0x2e, 1283},
	{0x2f, 1295},
	{0x30, 1307},
	{0x31, 1318},
	{0x32, 1330},
	{0x33, 1342}
};

int armada3700_avs_enable(struct regulator_dev *rdev)
{
	struct armada_3700_avs *avs = container_of(rdev->desc, struct armada_3700_avs, desc);
	u32 reg_val;

	/* Enable AVS  */
	reg_val = readl(avs->base + A3700_AVS_CTRL_0);
	reg_val |= AVS_ENABLE;
	writel(reg_val, avs->base + A3700_AVS_CTRL_0);

	return 0;
}

int armada3700_avs_disable(struct regulator_dev *rdev)
{
	struct armada_3700_avs *avs = container_of(rdev->desc, struct armada_3700_avs, desc);
	u32 reg_val;

	/* Disable AVS  */
	reg_val = readl(avs->base + A3700_AVS_CTRL_0);
	reg_val &= ~AVS_ENABLE;
	writel(reg_val, avs->base + A3700_AVS_CTRL_0);

	return 0;
}

int armada3700_avs_is_enabled(struct regulator_dev *rdev)
{
	struct armada_3700_avs *avs = container_of(rdev->desc, struct armada_3700_avs, desc);
	u32 reg_val;

	reg_val = readl(avs->base + A3700_AVS_CTRL_0);

	return (reg_val & AVS_ENABLE);
}

static struct regulator_ops armada3700_regulator_ops = {
	.enable = armada3700_avs_enable,
	.disable = armada3700_avs_disable,
	.is_enabled = armada3700_avs_is_enabled,
};

static u32 armada_3700_avs_vdd_min_get(u32 vlotage_m)
{
	int i;

	for (i = 0; i < MAX_VMIN_OPTION; i++) {
		if (avs_value_voltage_map[i].volt_m == vlotage_m)
			break;
	}
	if (i == MAX_VMIN_OPTION)
		return 0;

	return avs_value_voltage_map[i].avs_val;
}

static int armada_3700_avs_vdd_load_set(struct armada_3700_avs *avs)
{
	u32 reg_val, vdd_min;
	int i;

	/* Enable low voltage mode */
	if (avs->mode == LOW_VDD_MODE) {
		reg_val = readl(avs->base + A3700_AVS_CTRL_2);
		reg_val |= AVS_LOW_VDD_EN;
		writel(reg_val, avs->base + A3700_AVS_CTRL_2);
	} else {
		pr_err("Only low VDD mode is supported now\n");
		return -1;
	}

	/* Disable AVS before the configuration */
	reg_val = readl(avs->base + A3700_AVS_CTRL_0);
	reg_val &= ~AVS_ENABLE;
	writel(reg_val, avs->base + A3700_AVS_CTRL_0);

	/* Set VDD for VSET 1,2 and 3 with lowest VDD */
	reg_val = readl(avs->base + A3700_AVS_CTRL_0);
	reg_val &= ~((AVS_VDD_MASK << AVS_HIGH_VDD_LIMIT_OFFS) |
			(AVS_VDD_MASK << AVS_LOW_VDD_LIMIT_OFFS));

	/* Set VDD for VSET 1, VSET 2 and VSET 3 */
	for (i = VDD_SET1; i < VDD_SET_MAX; i++) {
		vdd_min = armada_3700_avs_vdd_min_get(avs->desc.volt_table[i]);
		if (vdd_min == 0) {
			pr_err("Invlid vdd min value got\n");
			return -1;
		}
		reg_val = readl(avs->base + A3700_AVS_VSET(i));
		reg_val &= ~((AVS_VDD_MASK << AVS_HIGH_VDD_LIMIT_OFFS) |
				(AVS_VDD_MASK << AVS_LOW_VDD_LIMIT_OFFS));
		reg_val |= ((vdd_min << AVS_HIGH_VDD_LIMIT_OFFS) |
				(vdd_min << AVS_LOW_VDD_LIMIT_OFFS));
		writel(reg_val, avs->base + A3700_AVS_VSET(i));
	}

	/* Enable AVS after the configuration */
	reg_val = readl(avs->base + A3700_AVS_CTRL_0);
	reg_val |= AVS_ENABLE;
	writel(reg_val, avs->base + A3700_AVS_CTRL_0);

	return 0;
}

static int armada_3700_avs_probe(struct platform_device *pdev)
{
	int ret = -EINVAL;
	struct armada_3700_avs *avs;
	struct device_node *np = pdev->dev.of_node;
	struct clk *max_cpu_clk;
	struct resource res;
	struct regulator_config config = { };
	struct regulator_dev *rdev;
	u32 max_cpu_freq;

	avs = devm_kzalloc(&pdev->dev, sizeof(*avs), GFP_KERNEL);
	if (!avs) {
		ret = -ENOMEM;
		goto err_put_node;
	}

	max_cpu_clk = devm_clk_get(&pdev->dev, NULL);
	if (IS_ERR(max_cpu_clk)) {
		dev_err(&pdev->dev, "error getting max cpu frequency\n");
		ret = PTR_ERR(max_cpu_clk);
		goto err_put_node;
	}
	max_cpu_freq = clk_get_rate(max_cpu_clk) / MHZ_TO_HZ;

	ret = of_address_to_resource(np, 0, &res);
	if (ret)
		goto free_avs;
	avs->base = devm_ioremap_resource(&pdev->dev, &res);
	if (IS_ERR(avs->base)) {
		ret = PTR_ERR(avs->base);
		goto free_avs;
	}

	if (of_find_property(np, "low-vdd-mode", NULL))
		avs->mode = LOW_VDD_MODE;
	if (of_find_property(np, "hold-mode", NULL))
		avs->mode = HOLD_MODE;

	avs->desc.name = "armada3700-avs";
	avs->desc.ops = &armada3700_regulator_ops;
	avs->desc.type = REGULATOR_VOLTAGE;
	avs->desc.owner = THIS_MODULE;
	if (max_cpu_freq == CPU_FREQ_600MHZ) {
		avs->freq_level = CPU_FREQ_LEVEL_600MHZ;
	} else if (max_cpu_freq == CPU_FREQ_800MHZ) {
		avs->freq_level = CPU_FREQ_LEVEL_800MHZ;
	} else if (max_cpu_freq == CPU_FREQ_1000MHZ) {
		avs->freq_level = CPU_FREQ_LEVEL_1000MHZ;
	} else if (max_cpu_freq == CPU_FREQ_1200MHZ) {
		avs->freq_level = CPU_FREQ_LEVEL_1200MHZ;
	} else {
		ret = -1;
		dev_err(&pdev->dev, "Unsupported CPU frwquency %dMHZ\n",
			max_cpu_freq);
		goto free_avs;
	}
	avs->desc.volt_table = voltage_m_tbl[avs->freq_level];

	ret = armada_3700_avs_vdd_load_set(avs);
	if (ret)
		goto free_avs;

	config.dev = &pdev->dev;
	config.init_data = dev_get_platdata(&pdev->dev);
	config.driver_data = avs;
	config.of_node = pdev->dev.of_node;

	rdev = devm_regulator_register(&pdev->dev, &avs->desc, &config);
	if (IS_ERR(rdev)) {
		dev_err(&pdev->dev, "Failed to register regulator %s\n",
			avs->desc.name);
		ret = PTR_ERR(rdev);
		goto free_avs;
	}
	platform_set_drvdata(pdev, rdev);

	return 0;
free_avs:
	devm_kfree(&pdev->dev, avs);
err_put_node:
	of_node_put(np);
	dev_err(&pdev->dev, "%s: failed initialization\n", __func__);
	return ret;

}

static int armada_3700_avs_remove(struct platform_device *pdev)
{
	int ret = -EINVAL;
	struct regulator_dev *rdev = platform_get_drvdata(pdev);
	struct armada_3700_avs *avs = container_of(rdev->desc, struct armada_3700_avs, desc);

	/* Disable AVS */
	ret = armada3700_avs_disable(rdev);

	/* Unregister regulator */
	devm_regulator_unregister(&pdev->dev, rdev);

	/* Unmap resource */
	devm_iounmap(&pdev->dev, avs->base);

	/* Free avs */
	devm_kfree(&pdev->dev, avs);

	return ret;
}

#ifdef CONFIG_PM
static int armada_3700_avs_resume_noirq(struct device *dev)
{
	int ret;
	struct platform_device *pdev = to_platform_device(dev);
	struct regulator_dev *rdev = platform_get_drvdata(pdev);
	struct armada_3700_avs *avs = container_of(rdev->desc, struct armada_3700_avs, desc);

	/* Resume VDD Set and enable AVS */
	ret = armada_3700_avs_vdd_load_set(avs);

	return ret;
}

static const struct dev_pm_ops armada_3700_avs_pm_ops = {
	.resume_noirq = armada_3700_avs_resume_noirq,
};

#define ARMADA_3700_AVS_PM_OPS (&armada_3700_avs_pm_ops)
#else
#define ARMADA_3700_AVS_PM_OPS NULL
#endif /* CONFIG_PM */

static const struct of_device_id armada3700_avs_of_match[] = {
	{ .compatible = "marvell,armada-3700-avs", },
	{}
};

static struct platform_driver armada3700_avs_driver = {
	.driver	= {
		.name = "armada3700-avs",
		.pm = ARMADA_3700_AVS_PM_OPS,
		.of_match_table = armada3700_avs_of_match,
	},
	.probe = armada_3700_avs_probe,
	.remove = armada_3700_avs_remove,
};

static int __init armada3700_regulator_avs_init(void)
{
	return platform_driver_register(&armada3700_avs_driver);
}
subsys_initcall(armada3700_regulator_avs_init);

