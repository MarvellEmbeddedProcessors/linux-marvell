/*
 * RTC driver for the Armada 38x & A8K Marvell SoCs
 *
 * Copyright (C) 2015 Marvell
 *
 * Gregory Clement <gregory.clement@free-electrons.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 */

#include <linux/of_device.h>
#include <linux/delay.h>
#include <linux/io.h>
#include <linux/module.h>
#include <linux/of.h>
#include <linux/platform_device.h>
#include <linux/rtc.h>

#define RTC_IRQ1_CONF			0x4
#define RTC_IRQ_AL_EN			BIT(0)
#define RTC_IRQ_FREQ_EN			BIT(1)
#define RTC_IRQ_FREQ_1HZ		BIT(2)
#define RTC_TIME			0xC
#define RTC_ALARM1			0x10

/* armada38x Interrupt registers  */
#define RTC_38X_STATUS			0x0
#define RTC_38X_STATUS_ALARM1		BIT(0)
#define RTC_38X_STATUS_ALARM2		BIT(1)
#define SOC_RTC_38X_INTERRUPT		0x8
#define SOC_RTC_38X_ALARM1		BIT(0)
#define SOC_RTC_38X_ALARM2		BIT(1)
#define SOC_RTC_38X_ALARM1_MASK		BIT(2)
#define SOC_RTC_38X_ALARM2_MASK		BIT(3)

/* armada70x0 Interrupt registers  */
#define RTC_70X0_STATUS			0x90
#define RTC_70X0_ALARM_MASK		0x94
#define RTC_70X0_ALARM1_MASK		BIT(1)
#define RTC_70X0_ALARM2_MASK		BIT(0)

/* armada38x SoC registers  */
#define RTC_38X_BRIDGE_TIMING_CTRL_REG_OFFS		0x0
#define RTC_38X_WRCLK_PERIOD_OFFS			0
#define RTC_38X_WRCLK_PERIOD_MASK			(0x3FF << RTC_38X_WRCLK_PERIOD_OFFS)
#define RTC_38X_READ_OUTPUT_DELAY_OFFS			26
#define RTC_38X_READ_OUTPUT_DELAY_MASK			(0x1F << RTC_38X_READ_OUTPUT_DELAY_OFFS)

/* armada70x0 SoC registers */
#define RTC_70X0_BRIDGE_TIMING_CTRL0_REG_OFFS		0x0
#define RTC_70X0_WRCLK_PERIOD_OFFS			0
#define RTC_70X0_WRCLK_PERIOD_MASK			(0xFFFF << RTC_70X0_WRCLK_PERIOD_OFFS)
#define RTC_70X0_WRCLK_SETUP_OFFS			16
#define RTC_70X0_WRCLK_SETUP_MASK			(0xFFFF << RTC_70X0_WRCLK_SETUP_OFFS)
#define RTC_70X0_BRIDGE_TIMING_CTRL1_REG_OFFS		0x4
#define RTC_70X0_READ_OUTPUT_DELAY_OFFS			0
#define RTC_70X0_READ_OUTPUT_DELAY_MASK			(0xFFFF << RTC_70X0_READ_OUTPUT_DELAY_OFFS)

#define SAMPLE_NR 100

/* Armada-38x SoC supports alarm 1
 * Armada-8K SoC sopports alarm 2 (though it has 2 alarm units, as
 *				   alarm 1 interrupt is not connected)
 */
#define ALARM1	0
#define ALARM2	1

#define ALARM_REG(base, alarm)		(base + alarm * sizeof(u32))

struct armada38x_rtc {
	struct rtc_device   *rtc_dev;
	void __iomem	    *regs;
	void __iomem	    *regs_soc;
	spinlock_t	    lock;
	int		    irq;
	struct armada38x_rtc_data *data;
};

struct armada38x_rtc_data {
	int		active_alarm;		/* the alarm which is wired to the interrupt */
	void (*update_mbus_timing)(struct armada38x_rtc *rtc); /* Initialize the RTC-MBUS bridge timing */
	unsigned long (*read_rtc_reg)(struct armada38x_rtc *rtc, uint8_t rtc_reg);
	void (*rtc_alarm_irq_ack)(struct armada38x_rtc *rtc);
	void (*rtc_mask_interrupt)(struct armada38x_rtc *rtc);
};

/*
 * According to the datasheet, the OS should wait 5us after every
 * register write to the RTC hard macro so that the required update
 * can occur without holding off the system bus
 * According to errata FE-3124064, Write to any RTC register
 * may fail. As a workaround, before writing to RTC
 * register, issue a dummy write of 0x0 twice to RTC Status
 * register.
 */

static void rtc_delayed_write(u32 val, struct armada38x_rtc *rtc, int offset)
{
	writel(0, rtc->regs + RTC_38X_STATUS);
	writel(0, rtc->regs + RTC_38X_STATUS);
	writel(val, rtc->regs + offset);
	udelay(5);
}

/* Update RTC-MBUS bridge timing parameters */
static void rtc_update_70x0_mbus_timing_params(struct armada38x_rtc *rtc)
{
	uint32_t reg;

	reg = readl(rtc->regs_soc + RTC_70X0_BRIDGE_TIMING_CTRL0_REG_OFFS);
	reg &= ~RTC_70X0_WRCLK_PERIOD_MASK;
	reg |= 0x3FF << RTC_70X0_WRCLK_PERIOD_OFFS;
	reg &= ~RTC_70X0_WRCLK_SETUP_MASK;
	reg |= 0x29 << RTC_70X0_WRCLK_SETUP_OFFS;
	writel(reg, rtc->regs_soc + RTC_70X0_BRIDGE_TIMING_CTRL0_REG_OFFS);

	reg = readl(rtc->regs_soc + RTC_70X0_BRIDGE_TIMING_CTRL1_REG_OFFS);
	reg &= ~RTC_70X0_READ_OUTPUT_DELAY_MASK;
	reg |= 0x3F << RTC_70X0_READ_OUTPUT_DELAY_OFFS;
	writel(reg, rtc->regs_soc + RTC_70X0_BRIDGE_TIMING_CTRL1_REG_OFFS);
}

static void rtc_update_38x_mbus_timing_params(struct armada38x_rtc *rtc)
{
	uint32_t reg;

	reg = readl(rtc->regs_soc + RTC_38X_BRIDGE_TIMING_CTRL_REG_OFFS);
	reg &= ~RTC_38X_WRCLK_PERIOD_MASK;
	reg |= 0x3FF << RTC_38X_WRCLK_PERIOD_OFFS; /*Maximum value*/
	reg &= ~RTC_38X_READ_OUTPUT_DELAY_MASK;
	reg |= 0x1F << RTC_38X_READ_OUTPUT_DELAY_OFFS; /*Maximum value*/
	writel(reg, rtc->regs_soc + RTC_38X_BRIDGE_TIMING_CTRL_REG_OFFS);
}

struct str_value_to_freq {
	unsigned long value;
	uint8_t freq;
} __packed;

static unsigned long read_rtc_38x_reg_wa(struct armada38x_rtc *rtc, uint8_t rtc_reg)
{
	unsigned long value_array[SAMPLE_NR], i, j, value;
	unsigned long max = 0, index_max = SAMPLE_NR - 1;
	struct str_value_to_freq value_to_freq[SAMPLE_NR];

	for (i = 0; i < SAMPLE_NR; i++) {
		value_to_freq[i].freq = 0;
		value_array[i] = readl(rtc->regs + rtc_reg);
	}
	for (i = 0; i < SAMPLE_NR; i++) {
		value = value_array[i];
		/*
		 * if value appears in value_to_freq array so add the counter of value,
		 * if didn't appear yet in counters array then allocate new member of
		 * value_to_freq array with counter = 1
		 */
		for (j = 0; j < SAMPLE_NR; j++) {
			if (value_to_freq[j].freq == 0 ||
					value_to_freq[j].value == value)
				break;
			if (j == (SAMPLE_NR - 1))
				break;
		}
		if (value_to_freq[j].freq == 0)
			value_to_freq[j].value = value;
		value_to_freq[j].freq++;
		/*find the most common result*/
		if (max < value_to_freq[j].freq) {
			index_max = j;
			max = value_to_freq[j].freq;
		}
	}
	return value_to_freq[index_max].value;
}

static unsigned long read_rtc_reg(struct armada38x_rtc *rtc, uint8_t rtc_reg)
{
	unsigned long value = readl(rtc->regs + rtc_reg);

	return value;
}

static void armada38x_rtc_alarm_irq_ack(struct armada38x_rtc *rtc)
{
	u32 val;

	val = rtc->data->read_rtc_reg(rtc, SOC_RTC_38X_INTERRUPT);
	writel(val & ~SOC_RTC_38X_ALARM1, rtc->regs_soc + SOC_RTC_38X_INTERRUPT);

	/* Ack the event */
	rtc_delayed_write(RTC_38X_STATUS_ALARM1, rtc, RTC_38X_STATUS);
}

static void armada70x0_rtc_alarm_irq_ack(struct armada38x_rtc *rtc)
{
	/* Only Alarm 2 is wired to ICU/GIC */
	rtc_delayed_write(RTC_70X0_ALARM2_MASK, rtc, RTC_70X0_STATUS);
}

static void armada38x_rtc_mask_interrupt(struct armada38x_rtc *rtc)
{
	u32 val;

	val = rtc->data->read_rtc_reg(rtc, SOC_RTC_38X_INTERRUPT);
	writel(val | SOC_RTC_38X_ALARM1_MASK,
	rtc->regs_soc + SOC_RTC_38X_INTERRUPT);
}

static void armada70x0_rtc_mask_interrupt(struct armada38x_rtc *rtc)
{
	u32 val;

	val = rtc->data->read_rtc_reg(rtc, RTC_70X0_ALARM_MASK);
	/* Only Alarm 2 is wired to ICU/GIC */
	writel(val | RTC_70X0_ALARM2_MASK, rtc->regs + RTC_70X0_ALARM_MASK);
}

static const struct armada38x_rtc_data armada38x_data = {
	.active_alarm = ALARM1,
	.update_mbus_timing = rtc_update_38x_mbus_timing_params,
	.read_rtc_reg = read_rtc_38x_reg_wa,
	.rtc_alarm_irq_ack = armada38x_rtc_alarm_irq_ack,
	.rtc_mask_interrupt = armada38x_rtc_mask_interrupt,
};

static const struct armada38x_rtc_data armada70x0_data = {
	.active_alarm = ALARM2, /* Only alarm 2 is wired to ICU/GIC */
	.update_mbus_timing = rtc_update_70x0_mbus_timing_params,
	.read_rtc_reg = read_rtc_reg,
	.rtc_alarm_irq_ack = armada70x0_rtc_alarm_irq_ack,
	.rtc_mask_interrupt = armada70x0_rtc_mask_interrupt,
};

#ifdef CONFIG_OF
static const struct of_device_id armada38x_rtc_of_match_table[] = {
	{
		.compatible	= "marvell,armada-380-rtc",
		.data		= &armada38x_data,
	},
	{
		.compatible	= "marvell,armada8k-rtc",
		.data		= &armada70x0_data,
	},
	{
		 /* sentinel */
	},
};
MODULE_DEVICE_TABLE(of, armada38x_rtc_of_match_table);
#endif

static int armada38x_rtc_read_time(struct device *dev, struct rtc_time *tm)
{
	struct armada38x_rtc *rtc = dev_get_drvdata(dev);
	unsigned long time, flags;

	spin_lock_irqsave(&rtc->lock, flags);
	time = rtc->data->read_rtc_reg(rtc, RTC_TIME);
	spin_unlock_irqrestore(&rtc->lock, flags);

	rtc_time_to_tm(time, tm);

	return 0;
}

static int armada38x_rtc_set_time(struct device *dev, struct rtc_time *tm)
{
	struct armada38x_rtc *rtc = dev_get_drvdata(dev);
	int ret = 0;
	unsigned long time, flags;

	ret = rtc_tm_to_time(tm, &time);

	if (ret)
		goto out;

	spin_lock_irqsave(&rtc->lock, flags);
	rtc_delayed_write(time, rtc, RTC_TIME);
	spin_unlock_irqrestore(&rtc->lock, flags);

out:
	return ret;
}

static int armada38x_rtc_read_alarm(struct device *dev, struct rtc_wkalrm *alrm)
{
	struct armada38x_rtc *rtc = dev_get_drvdata(dev);
	unsigned long time, flags;
	u32 val;

	spin_lock_irqsave(&rtc->lock, flags);

	time = rtc->data->read_rtc_reg(rtc, ALARM_REG(RTC_ALARM1, rtc->data->active_alarm));
	val = rtc->data->read_rtc_reg(rtc, ALARM_REG(RTC_IRQ1_CONF, rtc->data->active_alarm)) & RTC_IRQ_AL_EN;

	spin_unlock_irqrestore(&rtc->lock, flags);

	alrm->enabled = val ? 1 : 0;
	rtc_time_to_tm(time,  &alrm->time);

	return 0;
}

static int armada38x_rtc_set_alarm(struct device *dev, struct rtc_wkalrm *alrm)
{
	struct armada38x_rtc *rtc = dev_get_drvdata(dev);
	unsigned long time, flags;
	int ret = 0;

	ret = rtc_tm_to_time(&alrm->time, &time);

	if (ret)
		goto out;

	spin_lock_irqsave(&rtc->lock, flags);

	rtc_delayed_write(time, rtc, ALARM_REG(RTC_ALARM1, rtc->data->active_alarm));

	if (alrm->enabled) {
		/* enable the alarm */
		rtc_delayed_write(RTC_IRQ_AL_EN, rtc, ALARM_REG(RTC_IRQ1_CONF, rtc->data->active_alarm));

		/* mask RTC interrupt */
		rtc->data->rtc_mask_interrupt(rtc);
	}

	spin_unlock_irqrestore(&rtc->lock, flags);

out:
	return ret;
}

static int armada38x_rtc_alarm_irq_enable(struct device *dev,
					 unsigned int enabled)
{
	struct armada38x_rtc *rtc = dev_get_drvdata(dev);
	unsigned long flags;

	spin_lock_irqsave(&rtc->lock, flags);

	if (enabled)
		rtc_delayed_write(RTC_IRQ_AL_EN, rtc, ALARM_REG(RTC_IRQ1_CONF, rtc->data->active_alarm));
	else
		rtc_delayed_write(0, rtc, ALARM_REG(RTC_IRQ1_CONF, rtc->data->active_alarm));

	spin_unlock_irqrestore(&rtc->lock, flags);

	return 0;
}

static irqreturn_t armada38x_rtc_alarm_irq(int irq, void *data)
{
	struct armada38x_rtc *rtc = data;
	u32 val;
	int event = RTC_IRQF | RTC_AF;

	dev_dbg(&rtc->rtc_dev->dev, "%s:irq(%d)\n", __func__, irq);

	spin_lock(&rtc->lock);

	/* Ack the event */
	rtc->data->rtc_alarm_irq_ack(rtc);

	val = rtc->data->read_rtc_reg(rtc, ALARM_REG(RTC_IRQ1_CONF, rtc->data->active_alarm));
	/* disable the alarm */
	rtc_delayed_write(0, rtc, ALARM_REG(RTC_IRQ1_CONF, rtc->data->active_alarm));

	spin_unlock(&rtc->lock);

	if (val & RTC_IRQ_FREQ_EN) {
		if (val & RTC_IRQ_FREQ_1HZ)
			event |= RTC_UF;
		else
			event |= RTC_PF;
	}
	rtc_update_irq(rtc->rtc_dev, 1, event);

	return IRQ_HANDLED;
}

static struct rtc_class_ops armada38x_rtc_ops = {
	.read_time = armada38x_rtc_read_time,
	.set_time = armada38x_rtc_set_time,
	.read_alarm = armada38x_rtc_read_alarm,
	.set_alarm = armada38x_rtc_set_alarm,
	.alarm_irq_enable = armada38x_rtc_alarm_irq_enable,
};

static __init int armada38x_rtc_probe(struct platform_device *pdev)
{
	struct resource *res;
	const struct of_device_id *match;
	struct armada38x_rtc *rtc;
	int ret;

	match = of_match_device(armada38x_rtc_of_match_table, &pdev->dev);
	if (!match)
		return -ENODEV;

	rtc = devm_kzalloc(&pdev->dev, sizeof(struct armada38x_rtc),
			    GFP_KERNEL);
	if (!rtc)
		return -ENOMEM;

	spin_lock_init(&rtc->lock);

	res = platform_get_resource_byname(pdev, IORESOURCE_MEM, "rtc");
	rtc->regs = devm_ioremap_resource(&pdev->dev, res);
	if (IS_ERR(rtc->regs))
		return PTR_ERR(rtc->regs);
	res = platform_get_resource_byname(pdev, IORESOURCE_MEM, "rtc-soc");
	rtc->regs_soc = devm_ioremap_resource(&pdev->dev, res);
	if (IS_ERR(rtc->regs_soc))
		return PTR_ERR(rtc->regs_soc);

	rtc->data = (struct armada38x_rtc_data *)match->data;
	rtc->irq = platform_get_irq(pdev, 0);

	if (rtc->irq < 0) {
		dev_err(&pdev->dev, "no irq\n");
		return rtc->irq;
	}
	if (devm_request_irq(&pdev->dev, rtc->irq, armada38x_rtc_alarm_irq,
				0, pdev->name, rtc) < 0) {
		dev_warn(&pdev->dev, "Interrupt not available.\n");
		rtc->irq = -1;
		/*
		 * If there is no interrupt available then we can't
		 * use the alarm
		 */
		armada38x_rtc_ops.set_alarm = NULL;
		armada38x_rtc_ops.alarm_irq_enable = NULL;
	}

	platform_set_drvdata(pdev, rtc);
	if (rtc->irq != -1)
		device_init_wakeup(&pdev->dev, 1);

	/* Update RTC-MBUS bridge timing parameters */
	rtc->data->update_mbus_timing(rtc);

	rtc->rtc_dev = devm_rtc_device_register(&pdev->dev, pdev->name,
					&armada38x_rtc_ops, THIS_MODULE);
	if (IS_ERR(rtc->rtc_dev)) {
		ret = PTR_ERR(rtc->rtc_dev);
		dev_err(&pdev->dev, "Failed to register RTC device: %d\n", ret);
		return ret;
	}

	return 0;
}

#ifdef CONFIG_PM_SLEEP
static int armada38x_rtc_suspend(struct device *dev)
{
	if (device_may_wakeup(dev)) {
		struct armada38x_rtc *rtc = dev_get_drvdata(dev);

		enable_irq_wake(rtc->irq);
	}

	return 0;
}

static int armada38x_rtc_resume(struct device *dev)
{
	if (device_may_wakeup(dev)) {
		struct armada38x_rtc *rtc = dev_get_drvdata(dev);

		/* Update RTC-MBUS bridge timing parameters */
		rtc->data->update_mbus_timing(rtc);

		disable_irq_wake(rtc->irq);
	}

	return 0;
}
#endif

static SIMPLE_DEV_PM_OPS(armada38x_rtc_pm_ops,
			 armada38x_rtc_suspend, armada38x_rtc_resume);

static struct platform_driver armada38x_rtc_driver = {
	.driver		= {
		.name	= "armada38x-rtc",
		.pm	= &armada38x_rtc_pm_ops,
		.of_match_table = of_match_ptr(armada38x_rtc_of_match_table),
	},
};

module_platform_driver_probe(armada38x_rtc_driver, armada38x_rtc_probe);

MODULE_DESCRIPTION("Marvell Armada 38x RTC driver");
MODULE_AUTHOR("Gregory CLEMENT <gregory.clement@free-electrons.com>");
MODULE_LICENSE("GPL");
