/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (c) 2018 Marvell International Ltd.
 */

#define pr_fmt(fmt) KBUILD_MODNAME ": " fmt

#include <linux/clk.h>
#include <linux/io.h>
#include <linux/module.h>
#include <linux/of_device.h>
#include <linux/types.h>
#include <linux/watchdog.h>

#define WDT_MAX_CYCLE_COUNT			0xFFFFFFFF
#define WDT_DEFAULT_TIMEOUT			60 /* seconds */

/* Global Counter register*/
#define COUNTER_REGISTER_OFFSET			0x4
#define COUNTER_ENABLE_BIT			BIT(0)

#define COUNTER_CTRL_MODE_OFFSET		2
#define COUNTER_CTRL_MODE_MASK			0x3

#define COUNTER_CTRL_CLK_PRE_OFFSET		8
#define COUNTER_CTRL_CLK_PRE_MASK		0xFF
#define COUNTER_CTRL_CLK_MIN_FREQ_PRESCALE	2
#define COUNTER_CTRL_CLK_MAX_FREQ_PRESCALE	255

/* Get counter reg_base according the counter_id */
#define COUNTER_REG(base, id)    ((base) + ((id) << 4))

static bool nowayout = WATCHDOG_NOWAYOUT;
static int heartbeat = -1;	/* module parameter (seconds) */

enum watchdog_timer_id {
	WATCHDOG_TIMER0,
	WATCHDOG_TIMER1,
	WATCHDOG_TIMER2,
	WATCHDOG_TIMER3,
	WATCHDOG_TIMER_MAX,
};

/* Counter Modes specify if the counter is retriggerable or oneshot
 * and what is the trigger source.
 */
#define COUNTER_CTRL_MODE_ONESHOT					0
#define COUNTER_CTRL_MODE_RETRIGGER_ENDCOUNT_PULSE			1
#define COUNTER_CTRL_MODE_RETRIGGER_ENDCOUNT_PULSE_START_ON_HW_SIGNAL	2
#define COUNTER_CTRL_MODE_RETRIGGER_ANYTIME_START_ON_HW_SIGNAL		3

struct a3700_watchdog_data {
	struct watchdog_device wdt;
	unsigned char __iomem *reg;
	unsigned char __iomem *cnt_reg;
	unsigned long clk_rate;
	unsigned int prescaler;
	struct clk *clk;
	enum watchdog_timer_id timer_id;
};

static int set_counter_mode(struct a3700_watchdog_data *data, unsigned int mode)
{
	int value;

	value = readl(COUNTER_REG(data->cnt_reg, data->timer_id));
	value &= ~(COUNTER_CTRL_MODE_MASK << COUNTER_CTRL_MODE_OFFSET);
	value |= (mode << COUNTER_CTRL_MODE_OFFSET);
	writel(value, COUNTER_REG(data->cnt_reg, data->timer_id));

	return 0;
}

static int set_counter_divider(struct a3700_watchdog_data *data,
			       unsigned int div)
{
	int value;

	value = readl(COUNTER_REG(data->cnt_reg, data->timer_id));
	value &= ~(COUNTER_CTRL_CLK_PRE_MASK << COUNTER_CTRL_CLK_PRE_OFFSET);
	value |= (div << COUNTER_CTRL_CLK_PRE_OFFSET);
	writel(value, COUNTER_REG(data->cnt_reg, data->timer_id));

	return 0;
}

static int enable_watchdog_timer(struct a3700_watchdog_data *data)
{
	unsigned int value;

	value = readl(COUNTER_REG(data->cnt_reg, data->timer_id));
	value |= COUNTER_ENABLE_BIT;
	writel(value, COUNTER_REG(data->cnt_reg, data->timer_id));

	return 0;
}

static int disable_watchdog_timer(struct a3700_watchdog_data *data)
{
	unsigned int value;

	value = readl(COUNTER_REG(data->cnt_reg, data->timer_id));
	value &= ~COUNTER_ENABLE_BIT;
	writel(value, COUNTER_REG(data->cnt_reg, data->timer_id));

	return 0;
}

static int set_watchdog_timer_counter(struct watchdog_device *wdt_dev)
{
	struct a3700_watchdog_data *data = watchdog_get_drvdata(wdt_dev);
	unsigned int value;

	value = (data->clk_rate / data->prescaler) * wdt_dev->timeout;
	writel(value, COUNTER_REG(data->cnt_reg,
	       data->timer_id) + COUNTER_REGISTER_OFFSET);

	return 0;
}

static int a3700_wdt_ping(struct watchdog_device *wdt_dev)
{
	struct a3700_watchdog_data *data = watchdog_get_drvdata(wdt_dev);

	disable_watchdog_timer(data);
	/* Set watchdog time out duration */
	set_watchdog_timer_counter(wdt_dev);
	enable_watchdog_timer(data);

	return 0;
}

static int a3700_wdt_start(struct watchdog_device *wdt_dev)
{
	struct a3700_watchdog_data *data = watchdog_get_drvdata(wdt_dev);

	/* Set watchdog time out duration */
	set_watchdog_timer_counter(wdt_dev);
	enable_watchdog_timer(data);

	return 0;
}

static int a3700_wdt_stop(struct watchdog_device *wdt_dev)
{
	struct a3700_watchdog_data *data = watchdog_get_drvdata(wdt_dev);

	disable_watchdog_timer(data);

	return 0;
}

static unsigned int a3700_wdt_get_timeleft(struct watchdog_device *wdt_dev)
{
	struct a3700_watchdog_data *data = watchdog_get_drvdata(wdt_dev);
	u32 counter_val;

	counter_val = readl(COUNTER_REG(data->cnt_reg,
			    data->timer_id) + COUNTER_REGISTER_OFFSET);

	return counter_val / (data->clk_rate / data->prescaler);
}

static int a3700_wdt_set_timeout(struct watchdog_device *wdt_dev,
				 unsigned int timeout)
{
	struct a3700_watchdog_data *data = watchdog_get_drvdata(wdt_dev);
	unsigned int div;

	/* Get the Divisor according the timeout */
	div = DIV_ROUND_UP(timeout, (WDT_MAX_CYCLE_COUNT / data->clk_rate));
	if (div < COUNTER_CTRL_CLK_MIN_FREQ_PRESCALE)
		div = COUNTER_CTRL_CLK_MIN_FREQ_PRESCALE;
	else if (div > COUNTER_CTRL_CLK_MAX_FREQ_PRESCALE) {
		pr_err("Timeout out of the MAX timeout\n");
		return -EINVAL;
	}

	/* Counter Clock Prescaler setting */
	set_counter_divider(data, div);

	data->prescaler = div;
	wdt_dev->timeout = timeout;

	return 0;
}

static const struct watchdog_info a3700_wdt_info = {
	.options = WDIOF_SETTIMEOUT | WDIOF_KEEPALIVEPING | WDIOF_MAGICCLOSE,
	.identity = "Armada3700 Watchdog",
};

static const struct watchdog_ops a3700_wdt_ops = {
	.owner = THIS_MODULE,
	.start = a3700_wdt_start,
	.stop = a3700_wdt_stop,
	.ping = a3700_wdt_ping,
	.set_timeout = a3700_wdt_set_timeout,
	.get_timeleft = a3700_wdt_get_timeleft,
};

static const struct of_device_id a3700_wdt_of_match_table[] = {
	{
		.compatible = "marvell,armada-3700-wdt",
		.data = (void *)WATCHDOG_TIMER1
	},
	{},
};
MODULE_DEVICE_TABLE(of, a3700_wdt_of_match_table);

static int a3700_wdt_probe(struct platform_device *pdev)
{
	struct a3700_watchdog_data *data;
	struct resource *res;
	const struct of_device_id *match;
	int ret;

	data = devm_kzalloc(&pdev->dev, sizeof(struct a3700_watchdog_data),
			   GFP_KERNEL);
	if (!data)
		return -ENOMEM;

	match = of_match_device(a3700_wdt_of_match_table, &pdev->dev);
	if (!match) {
		dev_err(&pdev->dev, "Error: No device match found\n");
		return -ENODEV;
	}

	data->wdt.info = &a3700_wdt_info;
	data->wdt.ops = &a3700_wdt_ops;
	data->wdt.min_timeout = 1;
	data->timer_id = (enum watchdog_timer_id)match->data;
	if (data->timer_id >= WATCHDOG_TIMER_MAX) {
		dev_err(&pdev->dev, "Error: Timer ID out of the MAX timer number");
		return -EINVAL;
	}

	/* Get reg_base from DTS*/
	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	if (!res)
		return -ENODEV;

	data->reg = devm_ioremap(&pdev->dev, res->start,
				resource_size(res));

	if (IS_ERR(data->reg))
		return PTR_ERR(data->reg);

	res = platform_get_resource(pdev, IORESOURCE_MEM, 1);
	if (!res)
		return -ENODEV;

	data->cnt_reg = devm_ioremap(&pdev->dev, res->start,
				     resource_size(res));

	if (IS_ERR(data->cnt_reg))
		return PTR_ERR(data->cnt_reg);

	/* Initialize the Counter*/
	set_counter_mode(data, COUNTER_CTRL_MODE_ONESHOT);
	set_counter_divider(data, COUNTER_CTRL_CLK_MIN_FREQ_PRESCALE);

	/* Select Watchdog Timer*/
	writel(1 << data->timer_id, data->reg);

	/* Clock initialize*/
	data->clk = clk_get(&pdev->dev, NULL);
	if (IS_ERR(data->clk))
		return PTR_ERR(data->clk);

	/*
	 * The Counter clock source is from Crystal Oscillator
	 * and the Timer divider is initialized by 2
	 */
	data->clk_rate = clk_get_rate(data->clk);
	data->prescaler = COUNTER_CTRL_CLK_MIN_FREQ_PRESCALE;

	data->wdt.timeout = WDT_DEFAULT_TIMEOUT;
	data->wdt.max_timeout = (WDT_MAX_CYCLE_COUNT / data->clk_rate) *
					     COUNTER_CTRL_CLK_MAX_FREQ_PRESCALE;
	data->wdt.parent = &pdev->dev;
	watchdog_init_timeout(&data->wdt, heartbeat, &pdev->dev);

	platform_set_drvdata(pdev, &data->wdt);
	watchdog_set_drvdata(&data->wdt, data);

	watchdog_set_nowayout(&data->wdt, nowayout);
	ret = watchdog_register_device(&data->wdt);
	if (ret) {
		dev_err(&pdev->dev, "Failed to register watchdog device\n");
		return ret;
	}
	pr_info("Initial timeout %d sec%s\n",
		data->wdt.timeout, nowayout ? ", nowayout" : "");

	return 0;
}

static int a3700_wdt_remove(struct platform_device *pdev)
{
	struct watchdog_device *wdt_dev = platform_get_drvdata(pdev);

	watchdog_unregister_device(wdt_dev);

	return 0;
}

static void a3700_wdt_shutdown(struct platform_device *pdev)
{
	struct watchdog_device *wdt_dev = platform_get_drvdata(pdev);

	a3700_wdt_stop(wdt_dev);
}

static struct platform_driver a3700_wdt_driver = {
	.probe		= a3700_wdt_probe,
	.remove		= a3700_wdt_remove,
	.shutdown	= a3700_wdt_shutdown,
	.driver		= {
		.name	= "armada3700_wdt",
		.of_match_table = a3700_wdt_of_match_table,
	},
};

module_platform_driver(a3700_wdt_driver);

MODULE_AUTHOR("Allen Yan <yanwei@marvell.com>");
MODULE_DESCRIPTION("Armada3700 Processor Watchdog");

module_param(heartbeat, int, 0000);
MODULE_PARM_DESC(heartbeat, "Initial watchdog heartbeat in seconds");

module_param(nowayout, bool, 0000);
MODULE_PARM_DESC(nowayout, "Watchdog cannot be stopped once started (default="
		 __MODULE_STRING(WATCHDOG_NOWAYOUT) ")");

MODULE_LICENSE("GPL v2");
MODULE_ALIAS("platform:armada3700_wdt");
