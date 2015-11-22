/*
 * Marvell Armada 370/XP thermal sensor driver
 *
 * Copyright (C) 2013 Marvell
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 */
#include <linux/device.h>
#include <linux/err.h>
#include <linux/io.h>
#include <linux/kernel.h>
#include <linux/of.h>
#include <linux/module.h>
#include <linux/delay.h>
#include <linux/platform_device.h>
#include <linux/of_device.h>
#include <linux/thermal.h>
#include <linux/interrupt.h>

#define THERMAL_VALID_MASK		0x1
#define MCELSIUS(temp)			((temp) * 1000)
#define CELSIUS(temp)			((temp) / 1000)

/* Thermal Manager Control and Status Register */
#define PMU_TDC0_SW_RST_MASK		(0x1 << 1)
#define PMU_TM_DISABLE_OFFS		0
#define PMU_TM_DISABLE_MASK		(0x1 << PMU_TM_DISABLE_OFFS)
#define PMU_TDC0_REF_CAL_CNT_OFFS	11
#define PMU_TDC0_REF_CAL_CNT_MASK	(0x1ff << PMU_TDC0_REF_CAL_CNT_OFFS)
#define PMU_TDC0_OTF_CAL_MASK		(0x1 << 30)
#define PMU_TDC0_START_CAL_MASK		(0x1 << 25)

#define A375_UNIT_CONTROL_SHIFT		27
#define A375_UNIT_CONTROL_MASK		0x7
#define A375_READOUT_INVERT		BIT(15)
#define A375_HW_RESETn			BIT(8)
#define A380_HW_RESET			BIT(8)
#define A380_CONTROL_MSB_OFFSET		4
#define A380_TSEN_TC_TRIM_MASK		0x7

#define AP806_START	BIT(0)
#define AP806_RESET	BIT(1)
#define AP806_ENABLE	BIT(2)

/* For AP806 TSEN output format is signed as a 2s complement number
	ranging from-512 to +511*/
#define AP806_TSEN_OUTPUT_MSB		512
#define AP806_TSEN_OUTPUT_COMP		1024

/* Statically defined overheat threshold Celsius */
#define A380_THRESH_DEFAULT_TEMP	100
#define A380_THRESH_DEFAULT_HYST	2
#define A380_THRESH_OFFSET		16
#define A380_THRESH_HYST_MASK		0x3
#define A380_THRESH_HYST_OFFSET		26

struct armada_thermal_data;

/* Marvell EBU Thermal Sensor Dev Structure */
struct armada_thermal_priv {
	void __iomem *sensor;
	void __iomem *control;
	void __iomem *dfx;
	struct armada_thermal_data *data;
	struct platform_device *pdev;
};

struct armada_thermal_data {
	/* Initialize the sensor */
	void (*init_sensor)(struct platform_device *pdev,
			    struct armada_thermal_priv *);

	/* Test for a valid sensor value (optional) */
	bool (*is_valid)(struct armada_thermal_priv *);

	/* Formula coeficients: temp = (b + m * reg) / div */
	unsigned long coef_b;
	unsigned long coef_m;
	unsigned long coef_div;
	bool inverted;

	/* Register shift and mask to access the sensor temperature */
	unsigned int temp_shift;
	unsigned int temp_mask;
	unsigned int is_valid_shift;

	struct thermal_zone_device_ops *ops;
};

inline unsigned int armada380_thresh_val_calc(unsigned int celsius_temp,
					      struct armada_thermal_data *data)
{
	int thresh_val;

	thresh_val = ((MCELSIUS(celsius_temp) * data->coef_div) +
		      data->coef_b) / data->coef_m;

	return thresh_val & data->temp_mask;
}

inline unsigned int armada380_thresh_celsius_calc(int thresh_val,
				     int hyst, struct armada_thermal_data *data)
{
	unsigned int mcelsius_temp;

	mcelsius_temp = (((data->coef_m * (thresh_val + hyst)) -
			  data->coef_b) / data->coef_div);

	return CELSIUS(mcelsius_temp);
}

static void armada380_temp_set_threshold(struct platform_device *pdev,
					 struct armada_thermal_priv *priv)
{
	int temp, reg, hyst;
	unsigned int thresh;
	struct armada_thermal_data *data = priv->data;
	struct device_node *np = pdev->dev.of_node;

	/* get threshold value from DT */
	if (of_property_read_u32(np, "threshold", &thresh)) {
		thresh = A380_THRESH_DEFAULT_TEMP;
		dev_warn(&pdev->dev, "no threshold in DT, using default\n");
	}

	/* get hysteresis value from DT */
	if (of_property_read_u32(np, "hysteresis", &hyst)) {
		hyst = A380_THRESH_DEFAULT_HYST;
		dev_warn(&pdev->dev, "no hysteresis in DT, using default\n");
	}

	temp = armada380_thresh_val_calc(thresh, data);
	reg = readl_relaxed(priv->control + A380_CONTROL_MSB_OFFSET);

	/* Set Threshold */
	reg &= ~(data->temp_mask << A380_THRESH_OFFSET);
	reg |= (temp << A380_THRESH_OFFSET);

	/* Set Hysteresis */
	reg &= ~(A380_THRESH_HYST_MASK << A380_THRESH_HYST_OFFSET);
	reg |= (hyst << A380_THRESH_HYST_OFFSET);

	writel(reg, priv->control + A380_CONTROL_MSB_OFFSET);

	/* hysteresis calculation is 2^(2+n) */
	hyst = 1 << (hyst + 2);

	dev_info(&pdev->dev, "Overheat threshold between %d..%d\n",
		armada380_thresh_celsius_calc(temp, -hyst, data),
		armada380_thresh_celsius_calc(temp, hyst, data));
}

static void armadaxp_init_sensor(struct platform_device *pdev,
				 struct armada_thermal_priv *priv)
{
	unsigned long reg;

	reg = readl_relaxed(priv->control);
	reg |= PMU_TDC0_OTF_CAL_MASK;
	writel(reg, priv->control);

	/* Reference calibration value */
	reg &= ~PMU_TDC0_REF_CAL_CNT_MASK;
	reg |= (0xf1 << PMU_TDC0_REF_CAL_CNT_OFFS);
	writel(reg, priv->control);

	/* Reset the sensor */
	reg = readl_relaxed(priv->control);
	writel((reg | PMU_TDC0_SW_RST_MASK), priv->control);

	writel(reg, priv->control);

	/* Enable the sensor */
	reg = readl_relaxed(priv->sensor);
	reg &= ~PMU_TM_DISABLE_MASK;
	writel(reg, priv->sensor);
}

static void armada370_init_sensor(struct platform_device *pdev,
				  struct armada_thermal_priv *priv)
{
	unsigned long reg;

	reg = readl_relaxed(priv->control);
	reg |= PMU_TDC0_OTF_CAL_MASK;
	writel(reg, priv->control);

	/* Reference calibration value */
	reg &= ~PMU_TDC0_REF_CAL_CNT_MASK;
	reg |= (0xf1 << PMU_TDC0_REF_CAL_CNT_OFFS);
	writel(reg, priv->control);

	reg &= ~PMU_TDC0_START_CAL_MASK;
	writel(reg, priv->control);

	mdelay(10);
}

static void armada375_init_sensor(struct platform_device *pdev,
				  struct armada_thermal_priv *priv)
{
	unsigned long reg;

	reg = readl(priv->control + 4);
	reg &= ~(A375_UNIT_CONTROL_MASK << A375_UNIT_CONTROL_SHIFT);
	reg &= ~A375_READOUT_INVERT;
	reg &= ~A375_HW_RESETn;

	writel(reg, priv->control + 4);
	mdelay(20);

	reg |= A375_HW_RESETn;
	writel(reg, priv->control + 4);
	mdelay(50);
}

static void armada380_init_sensor(struct platform_device *pdev,
				  struct armada_thermal_priv *priv)
{
	unsigned long reg = readl_relaxed(priv->control +
					  A380_CONTROL_MSB_OFFSET);

	/* Reset hardware once */
	if (!(reg & A380_HW_RESET)) {
		reg |= A380_HW_RESET;
		writel(reg, priv->control + A380_CONTROL_MSB_OFFSET);
		mdelay(10);
	}

	/* set Tsen Tc Trim to correct default value (errata #132698) */
	reg = readl_relaxed(priv->control);
	reg &= ~A380_TSEN_TC_TRIM_MASK;
	reg |= 0x3;
	writel(reg, priv->control);

	/* Set thresholds */
	armada380_temp_set_threshold(pdev, priv);

	/* Clear on Read DFX temperature irqs cause */
	reg = readl_relaxed(priv->dfx + 0x10);

	/* Unmask DFX Temperature overheat and cooldown irqs */
	reg = readl_relaxed(priv->dfx + 0x14);
	reg |= (0x3 << 1);
	writel(reg, priv->dfx + 0x14);

	/* Unmask DFX Server irq */
	reg = readl_relaxed(priv->dfx + 0x4);
	reg |= (0x3 << 1);
	writel(reg, priv->dfx + 0x4);
}

static void armada_ap806_init_sensor(struct platform_device *pdev,
				  struct armada_thermal_priv *priv)
{
	unsigned long reg = readl_relaxed(priv->control);

	reg &= ~AP806_RESET;
	reg |= AP806_START;
	reg |= AP806_ENABLE;
	writel(reg, priv->control);
	mdelay(10);
}

static bool armada_is_valid(struct armada_thermal_priv *priv)
{
	unsigned long reg = readl_relaxed(priv->sensor);

	return (reg >> priv->data->is_valid_shift) & THERMAL_VALID_MASK;
}

static int armada_get_temp(struct thermal_zone_device *thermal,
			  int *temp)
{
	struct armada_thermal_priv *priv = thermal->devdata;
	unsigned long reg;
	unsigned long m, b, div;

	/* Valid check */
	if (priv->data->is_valid && !priv->data->is_valid(priv)) {
		dev_err(&thermal->device,
			"Temperature sensor reading not valid\n");
		return -EIO;
	}

	reg = readl_relaxed(priv->sensor);
	reg = (reg >> priv->data->temp_shift) & priv->data->temp_mask;

	/* Get formula coeficients */
	b = priv->data->coef_b;
	m = priv->data->coef_m;
	div = priv->data->coef_div;

	if (priv->data->inverted)
		*temp = ((m * reg) - b) / div;
	else
		*temp = (b - (m * reg)) / div;
	return 0;
}

static int armada_ap806_get_temp(struct thermal_zone_device *thermal, int *temp)
{
	struct armada_thermal_priv *priv = thermal->devdata;
	unsigned long reg;
	unsigned long m, b, div;

	/* Valid check */
	if (priv->data->is_valid && !priv->data->is_valid(priv)) {
		dev_err(&thermal->device,
			"Temperature sensor reading not valid\n");
		return -EIO;
	}

	reg = readl_relaxed(priv->sensor);
	reg = (reg >> priv->data->temp_shift) & priv->data->temp_mask;

	/* TSEN output format is signed as a 2s complement number
	    ranging from-512 to +511. when MSB is set, need to
	    calculate the complement number */
	if (reg >= AP806_TSEN_OUTPUT_MSB)
		reg = reg - AP806_TSEN_OUTPUT_COMP;

	/* Get formula coeficients */
	b = priv->data->coef_b;
	m = priv->data->coef_m;
	div = priv->data->coef_div;

	*temp = ((m * reg) + b) / div;

	return 0;
}

static struct thermal_zone_device_ops armada_ops = {
	.get_temp = armada_get_temp,
};

static struct thermal_zone_device_ops armada_ap806_ops = {
	.get_temp = armada_ap806_get_temp,
};

static irqreturn_t a38x_temp_irq_handler(int irq, void *data)
{
	struct armada_thermal_priv *priv = (struct armada_thermal_priv *)data;
	struct device *dev = &priv->pdev->dev;
	u32 reg;

	/* Mask Temp irq */
	reg = readl_relaxed(priv->dfx + 0x14);
	reg &= ~(0x3 << 1);
	writel(reg, priv->dfx + 0x14);

	/* Clear Temp irq cause */
	reg = readl_relaxed(priv->dfx + 0x10);

	if (reg & (0x3 << 1))
		dev_warn(dev, "Overheat critical %s threshold temperature reached\n",
			 (reg & (1 << 1)) ? "high" : "low");

	/* UnMask Temp irq */
	reg = readl_relaxed(priv->dfx + 0x14);
	reg |= (0x3 << 1);
	writel(reg, priv->dfx + 0x14);

	return IRQ_HANDLED;
}

static const struct armada_thermal_data armadaxp_data = {
	.init_sensor = armadaxp_init_sensor,
	.temp_shift = 10,
	.temp_mask = 0x1ff,
	.coef_b = 3153000000UL,
	.coef_m = 10000000UL,
	.coef_div = 13825,
	.ops = &armada_ops,
};

static const struct armada_thermal_data armada370_data = {
	.is_valid = armada_is_valid,
	.init_sensor = armada370_init_sensor,
	.is_valid_shift = 9,
	.temp_shift = 10,
	.temp_mask = 0x1ff,
	.coef_b = 3153000000UL,
	.coef_m = 10000000UL,
	.coef_div = 13825,
	.ops = &armada_ops,
};

static const struct armada_thermal_data armada375_data = {
	.is_valid = armada_is_valid,
	.init_sensor = armada375_init_sensor,
	.is_valid_shift = 10,
	.temp_shift = 0,
	.temp_mask = 0x1ff,
	.coef_b = 3171900000UL,
	.coef_m = 10000000UL,
	.coef_div = 13616,
	.ops = &armada_ops,
};

static const struct armada_thermal_data armada380_data = {
	.is_valid = armada_is_valid,
	.init_sensor = armada380_init_sensor,
	.is_valid_shift = 10,
	.temp_shift = 0,
	.temp_mask = 0x3ff,
	.coef_b = 1172499100UL,
	.coef_m = 2000096UL,
	.coef_div = 4201,
	.inverted = true,
	.ops = &armada_ops,
};

static const struct armada_thermal_data armada_ap806_data = {
	.is_valid = armada_is_valid,
	.init_sensor = armada_ap806_init_sensor,
	.is_valid_shift = 16,
	.temp_shift = 0,
	.temp_mask = 0x3ff,
	.coef_b = 153400,
	.coef_m = 425,
	.coef_div = 1,
	.inverted = true,
	.ops = &armada_ap806_ops,
};


static const struct of_device_id armada_thermal_id_table[] = {
	{
		.compatible = "marvell,armadaxp-thermal",
		.data       = &armadaxp_data,
	},
	{
		.compatible = "marvell,armada370-thermal",
		.data       = &armada370_data,
	},
	{
		.compatible = "marvell,armada375-thermal",
		.data       = &armada375_data,
	},
	{
		.compatible = "marvell,armada380-thermal",
		.data       = &armada380_data,
	},
	{
		.compatible = "marvell,armada-ap806-thermal",
		.data       = &armada_ap806_data,
	},
	{
		/* sentinel */
	},
};
MODULE_DEVICE_TABLE(of, armada_thermal_id_table);

static int armada_thermal_probe(struct platform_device *pdev)
{
	struct thermal_zone_device *thermal;
	const struct of_device_id *match;
	struct armada_thermal_priv *priv;
	struct resource *res;
	int irq;

	match = of_match_device(armada_thermal_id_table, &pdev->dev);
	if (!match)
		return -ENODEV;

	priv = devm_kzalloc(&pdev->dev, sizeof(*priv), GFP_KERNEL);
	if (!priv)
		return -ENOMEM;

	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	priv->sensor = devm_ioremap_resource(&pdev->dev, res);
	if (IS_ERR(priv->sensor))
		return PTR_ERR(priv->sensor);

	res = platform_get_resource(pdev, IORESOURCE_MEM, 1);
	priv->control = devm_ioremap_resource(&pdev->dev, res);
	if (IS_ERR(priv->control))
		return PTR_ERR(priv->control);

	res = platform_get_resource(pdev, IORESOURCE_MEM, 2);
	priv->dfx = devm_ioremap_resource(&pdev->dev, res);
	if (IS_ERR(priv->dfx))
		return PTR_ERR(priv->dfx);

	priv->data = (struct armada_thermal_data *)match->data;
	priv->data->init_sensor(pdev, priv);

	thermal = thermal_zone_device_register("armada_thermal", 0, 0,
					       priv, priv->data->ops, NULL, 0, 0);
	if (IS_ERR(thermal)) {
		dev_err(&pdev->dev,
			"Failed to register thermal zone device\n");
		return PTR_ERR(thermal);
	}

	priv->pdev = pdev;

	/* Register overheat interrupt */
	irq = platform_get_irq(pdev, 0);

	if (irq < 0) {
		dev_warn(&pdev->dev, "no irq\n");
		return irq;
	}
	if (devm_request_irq(&pdev->dev, irq, a38x_temp_irq_handler,
				0, pdev->name, priv) < 0) {
		dev_warn(&pdev->dev, "Interrupt not available.\n");
	}

	platform_set_drvdata(pdev, thermal);

	return 0;
}

static int armada_thermal_exit(struct platform_device *pdev)
{
	struct thermal_zone_device *armada_thermal =
		platform_get_drvdata(pdev);

	thermal_zone_device_unregister(armada_thermal);

	return 0;
}

static int armada_thermal_resume(struct platform_device *pdev)
{
	struct thermal_zone_device *thermal =
		platform_get_drvdata(pdev);
	struct armada_thermal_priv *priv = thermal->devdata;

	priv->data->init_sensor(pdev, priv);

	return 0;
}

static struct platform_driver armada_thermal_driver = {
	.probe = armada_thermal_probe,
	.remove = armada_thermal_exit,
#ifdef CONFIG_PM
	.resume = armada_thermal_resume,
#endif
	.driver = {
		.name = "armada_thermal",
		.of_match_table = armada_thermal_id_table,
	},
};

module_platform_driver(armada_thermal_driver);

MODULE_AUTHOR("Ezequiel Garcia <ezequiel.garcia@free-electrons.com>");
MODULE_DESCRIPTION("Armada 370/380/XP thermal driver");
MODULE_LICENSE("GPL v2");
