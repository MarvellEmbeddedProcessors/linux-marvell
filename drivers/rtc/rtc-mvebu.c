/*
 * Marvell EBU SoC Realtime Clock Driver (RTC)
 *
 * Copyright (C) 2014 Marvell
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation version 2 of the License.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Based on a driver made by Brian Mahaffy <bmahaffy@marvell.com>
 */

#include <linux/module.h>
#include <linux/rtc.h>
#include <linux/fs.h>
#include <linux/slab.h>
#include <linux/io.h>
#include <linux/irq.h>
#include <linux/platform_device.h>
#include <linux/of.h>
#include <linux/interrupt.h>
#include <linux/delay.h>

/* The RTC DRS, revision 1.2, indicates that firmware should wait 5us after every register write */
/* to the RTC hard macro so that the required update can occur without holding off the system bus */
#define RTC_READ_REG(reg)		ioread32(rtc->regbase_rtc + reg)
#define RTC_WRITE_REG(val, reg)		{ iowrite32(val, rtc->regbase_rtc + reg); udelay(5); }

#define RTC_0HZ 0x00
#define RTC_1HZ 0x04
#define RTC_2HZ 0x08
#define RTC_4HZ 0x10
#define RTC_8HZ 0x20

#define RTC_NOMINAL_TIMING 0x0

#define RTC_STATUS_REG_OFFS		0x0
#define RTC_IRQ_1_CONFIG_REG_OFFS	0x4
#define RTC_IRQ_2_CONFIG_REG_OFFS	0x8
#define RTC_TIME_REG_OFFS		0xC
#define RTC_ALARM_1_REG_OFFS		0x10
#define RTC_ALARM_2_REG_OFFS		0x14
#define RTC_CLOCK_CORR_REG_OFFS		0x18
#define RTC_TEST_CONFIG_REG_OFFS	0x1C

#define RTC_SZ_STATUS_ALARM1_MASK	0x1
#define RTC_SZ_STATUS_ALARM2_MASK	0x2
#define RTC_SZ_TIMING_RESERVED1_MASK	0xFFFF0000
#define RTC_SZ_INTERRUPT1_INT1AE_MASK	0x1
#define RTC_SZ_INTERRUPT1_RESERVED1_MASK	0xFFFFFFC0
#define RTC_SZ_INTERRUPT2_INT2FE_MASK	0x2
#define RTC_SZ_INTERRUPT2_RESERVED1_MASK	0xFFFFFFC0

typedef struct mvebu_rtc_s {
	struct rtc_device *rtc_dev;
	void __iomem      *regbase_rtc;
	void __iomem      *regbase_soc;
	spinlock_t         lock;
	uint32_t           irq;
	uint32_t           periodic_freq;
} mvebu_rtc_t;

static bool rtc_init_state(mvebu_rtc_t *rtc)
{
	/* Update RTC-MBUS bridge timing parameters */
	writel(0xFD4D4CFA, rtc->regbase_soc);

	/* Make sure we are not in any test mode */
	RTC_WRITE_REG(0, RTC_TEST_CONFIG_REG_OFFS);
	msleep_interruptible(500);

	/* Setup nominal register access timing */
	RTC_WRITE_REG(RTC_NOMINAL_TIMING, RTC_CLOCK_CORR_REG_OFFS);

	/* Turn off Int1 sources & clear the Alarm count */
	RTC_WRITE_REG(0, RTC_IRQ_1_CONFIG_REG_OFFS);
	RTC_WRITE_REG(0, RTC_ALARM_1_REG_OFFS);

	/* Turn off Int2 sources & clear the Periodic count */
	RTC_WRITE_REG(0, RTC_IRQ_2_CONFIG_REG_OFFS);
	RTC_WRITE_REG(0, RTC_ALARM_2_REG_OFFS);

	/* Clear any pending Status bits */
	RTC_WRITE_REG((RTC_SZ_STATUS_ALARM1_MASK | RTC_SZ_STATUS_ALARM2_MASK), RTC_STATUS_REG_OFFS);
	{
		uint32_t stat, alrm1, alrm2, int1, int2, tstcfg;

		stat   = RTC_READ_REG(RTC_STATUS_REG_OFFS) & 0xFF;
		alrm1  = RTC_READ_REG(RTC_ALARM_1_REG_OFFS);
		int1   = RTC_READ_REG(RTC_IRQ_1_CONFIG_REG_OFFS) & 0xFF;
		alrm2  = RTC_READ_REG(RTC_ALARM_2_REG_OFFS);
		int2   = RTC_READ_REG(RTC_IRQ_2_CONFIG_REG_OFFS) & 0xFF;
		tstcfg = RTC_READ_REG(RTC_TEST_CONFIG_REG_OFFS) & 0xFF;

		if ((0xFC == stat)  &&
			(0 == alrm1) && (0xC0 == int1) &&
			(0 == alrm2) && (0xC0 == int2) &&
			(0 == tstcfg)) {
			return true;
		} else {
			return false;
		}
	}
}

/*
 * Calculate the next alarm time given the
 * requested alarm time and the current time...
 */
static void rtc_next_alarm_time(struct rtc_time *next, struct rtc_time *now, struct rtc_time *alrm)
{
	unsigned long now_time;
	unsigned long next_time;

	next->tm_year = now->tm_year;
	next->tm_mon  = now->tm_mon;
	next->tm_mday = now->tm_mday;
	next->tm_hour = alrm->tm_hour;
	next->tm_min  = alrm->tm_min;
	next->tm_sec  = alrm->tm_sec;

	rtc_tm_to_time(now, &now_time);
	rtc_tm_to_time(next, &next_time);

	/* Do we need to advance a day? */
	if (next_time < now_time) {
		next_time += 60 * 60 * 24;
		rtc_time_to_tm(next_time, next);
	}
}

static int rtc_setup_alarm(mvebu_rtc_t *rtc, struct rtc_time *alrm)
{
	struct rtc_time alarm_tm, now_tm;
	unsigned long now, time;
	int ret;

	do {
		now = RTC_READ_REG(RTC_TIME_REG_OFFS);
		rtc_time_to_tm(now, &now_tm);
		rtc_next_alarm_time(&alarm_tm, &now_tm, alrm);
		ret = rtc_tm_to_time(&alarm_tm, &time);

		if (ret != 0)
			continue;
		else
			RTC_WRITE_REG(time, RTC_ALARM_1_REG_OFFS);

	} while (now != RTC_READ_REG(RTC_TIME_REG_OFFS));

	return ret;
}

static irqreturn_t mvebu_rtc_irq_handler(int irq, void *rtc_ptr)
{
	mvebu_rtc_t *rtc = (mvebu_rtc_t *)rtc_ptr;
	irqreturn_t irq_status = IRQ_NONE;
	uint32_t rtc_status_reg, val;

	val = readl(rtc->regbase_soc + 0x8);
	writel(0, rtc->regbase_soc + 0x8);

	rtc_status_reg = RTC_READ_REG(RTC_STATUS_REG_OFFS);

	/* Has Alarm 1 triggered? */
	/* TBD - properly check IRQ status */
	/* if (rtc_status_reg & RTC_SZ_STATUS_ALARM1_MASK) { */
		/* Disable the interrupt.  This alarm is complete... */
		RTC_WRITE_REG(RTC_SZ_INTERRUPT1_RESERVED1_MASK, RTC_IRQ_1_CONFIG_REG_OFFS);
		RTC_WRITE_REG(RTC_SZ_STATUS_ALARM1_MASK, RTC_STATUS_REG_OFFS);

		rtc_update_irq(rtc->rtc_dev, 1, (RTC_IRQF | RTC_AF));
		irq_status = IRQ_HANDLED;
	/* } */
#if 0
	/* Has either the 1Hz or user periodic frequency triggered?
	 NOTE: This is a periodic interrupt.  It keeps on trucking... */
	if (rtc_status_reg & RTC_SZ_STATUS_ALARM2_MASK) {
		RTC_WRITE_REG(RTC_SZ_STATUS_ALARM2_MASK, RTC_STATUS_REG_OFFS);

		if (rtc->periodic_freq == RTC_1HZ)
			rtc_update_irq(rtc->rtc_dev, 1, (RTC_IRQF | RTC_UF));
		else
			rtc_update_irq(rtc->rtc_dev, 1, (RTC_IRQF | RTC_PF));

		irq_status = IRQ_HANDLED;
	}
#endif

	return irq_status;
}

static int mvebu_rtc_read_time(struct device *dev, struct rtc_time *tm)
{
	mvebu_rtc_t *rtc = dev_get_drvdata(dev);
	unsigned long time, time_check;

	spin_lock_irq(&rtc->lock);

	time = RTC_READ_REG(RTC_TIME_REG_OFFS);

	/* WA for failing time read attempts. The HW ERRATA information should be added here */
	/* if detected more than one second between two time reads, read once again */
	time_check = RTC_READ_REG(RTC_TIME_REG_OFFS);
	if ((time_check - time) > 1)
		time_check = RTC_READ_REG(RTC_TIME_REG_OFFS);
	/* End of WA */

	rtc_time_to_tm(time_check, tm);
	spin_unlock_irq(&rtc->lock);

	return 0;
}

static int mvebu_rtc_set_time(struct device *dev, struct rtc_time *tm)
{
	mvebu_rtc_t *rtc = dev_get_drvdata(dev);
	unsigned long time;

	if (rtc_tm_to_time(tm, &time) == 0) {
		spin_lock_irq(&rtc->lock);
		/* WA for failing time set attempts. The HW ERRATA information should be added here */
		RTC_WRITE_REG(0, RTC_STATUS_REG_OFFS);
		mdelay(100);
		/* End of SW WA */
		RTC_WRITE_REG(time, RTC_TIME_REG_OFFS);
		spin_unlock_irq(&rtc->lock);
	}

	return 0;
}

static int mvebu_rtc_read_alarm(struct device *dev, struct rtc_wkalrm *alrm)
{
	mvebu_rtc_t *rtc = dev_get_drvdata(dev);
	unsigned long time;

	spin_lock_irq(&rtc->lock);

	rtc_time_to_tm((time = RTC_READ_REG(RTC_ALARM_1_REG_OFFS)), &alrm->time);
	alrm->enabled = (RTC_READ_REG(RTC_IRQ_1_CONFIG_REG_OFFS) & RTC_SZ_INTERRUPT1_INT1AE_MASK) ? 1 : 0;

	spin_unlock_irq(&rtc->lock);

	return 0;
}

static int mvebu_rtc_set_alarm(struct device *dev, struct rtc_wkalrm *alrm)
{
	mvebu_rtc_t *rtc = dev_get_drvdata(dev);
	unsigned long time;
	uint32_t val;

	if (rtc_tm_to_time(&alrm->time, &time) == 0) {
		spin_lock_irq(&rtc->lock);

		if (rtc_setup_alarm(rtc, &alrm->time) == 0) {
			if (alrm->enabled) {
				RTC_WRITE_REG(RTC_SZ_INTERRUPT1_INT1AE_MASK, RTC_IRQ_1_CONFIG_REG_OFFS);
				val = readl(rtc->regbase_soc + 0x8);
				writel(val | (0x1 << 2), rtc->regbase_soc + 0x8);
			}
		}
		spin_unlock_irq(&rtc->lock);
	}

	return 0;
}

static int mvebu_rtc_alarm_irq_enable(struct device *dev, unsigned int enable)
{
	mvebu_rtc_t *rtc = dev_get_drvdata(dev);
	spin_lock_irq(&rtc->lock);

	if (enable) {
		RTC_WRITE_REG(RTC_SZ_INTERRUPT1_INT1AE_MASK, RTC_IRQ_1_CONFIG_REG_OFFS);
	} else {
		RTC_WRITE_REG(RTC_SZ_INTERRUPT1_RESERVED1_MASK, RTC_IRQ_1_CONFIG_REG_OFFS);
	}

	spin_unlock_irq(&rtc->lock);

	return 0;
}


static const struct rtc_class_ops mvebu_rtc_ops = {
	.read_time         = mvebu_rtc_read_time,
	.set_time          = mvebu_rtc_set_time,
	.read_alarm        = mvebu_rtc_read_alarm,
	.set_alarm         = mvebu_rtc_set_alarm,
	.alarm_irq_enable  = mvebu_rtc_alarm_irq_enable,
};

static int mvebu_rtc_probe(struct platform_device *pdev)
{
	int ret = 0;
	struct resource *res;
	mvebu_rtc_t *rtc = kzalloc(sizeof(struct mvebu_rtc_s), GFP_KERNEL);

	if (unlikely(!rtc)) {
		ret = -ENOMEM;
		goto exit;
	} else
		platform_set_drvdata(pdev, rtc);

	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	if (unlikely(res == NULL)) {
		dev_err(&pdev->dev, "No IO resource\n");
		ret = -ENOENT;
		goto errExit1;
	}

	if (!request_mem_region(res->start, resource_size(res), pdev->name)) {
		dev_err(&pdev->dev, "No MEM resource\n");
		ret = -EBUSY;
		goto errExit1;
	}

	rtc->regbase_rtc = ioremap(res->start, resource_size(res));
	if (unlikely(!rtc->regbase_rtc)) {
		dev_err(&pdev->dev, "No REMAP resource\n");
		ret = -EINVAL;
		goto errExit2;
	}

	res = platform_get_resource(pdev, IORESOURCE_MEM, 1);
	if (unlikely(res == NULL)) {
		dev_err(&pdev->dev, "No IO resource\n");
		ret = -ENOENT;
		goto errExit1;
	}

	if (!request_mem_region(res->start, resource_size(res), pdev->name)) {
		dev_err(&pdev->dev, "No MEM resource\n");
		ret = -EBUSY;
		goto errExit1;
	}

	rtc->regbase_soc = ioremap(res->start, resource_size(res));
	if (unlikely(!rtc->regbase_soc)) {
		dev_err(&pdev->dev, "No REMAP resource\n");
		ret = -EINVAL;
		goto errExit2;
	}

	rtc->irq = platform_get_irq(pdev, 0);
	if (unlikely(rtc->irq <= 0)) {
		dev_err(&pdev->dev, "No IRQ resource\n");
		ret = -ENOENT;
		goto errExit1;
	}
	/* Init the state of the RTC, failure indicates there is probably no battery */

	{
		uint32_t count = 3;

		while (!rtc_init_state(rtc) && --count)
			;

		if (count == 0) {
				dev_err(&pdev->dev, "%probe: Error in rtc_init_state\n", __func__);
			ret = -ENODEV;
			goto errExit3;
		}
	}

	spin_lock_init(&rtc->lock);

	/* register shared periodic/carry/alarm irq */
	ret = request_irq(rtc->irq, mvebu_rtc_irq_handler, 0, "mvebu-rtc", rtc);
	if (unlikely(ret)) {
		dev_err(&pdev->dev, "Request IRQ failed with %d, IRQ %d\n", ret, rtc->irq);
		ret = ret;
		goto errExit3;
	}

	device_init_wakeup(&pdev->dev, 1);
	rtc->rtc_dev = rtc_device_register(pdev->name, &pdev->dev, &mvebu_rtc_ops, THIS_MODULE);
	if (IS_ERR(rtc->rtc_dev)) {
		dev_err(&pdev->dev, "%s: error in rtc_device_register\n", __func__);
		ret = PTR_ERR(rtc);
		goto errExit4;
	}

	rtc->periodic_freq = RTC_0HZ;
	device_init_wakeup(&pdev->dev, 1);
	goto exit;

errExit4:
	free_irq(rtc->irq, rtc);

errExit3:
	iounmap(rtc->regbase_rtc);

errExit2:
	release_mem_region(res->start, resource_size(res));

errExit1:
	kfree(rtc);

exit:
	return ret;
}

static int __exit mvebu_rtc_remove(struct platform_device *pdev)
{
	struct resource *res;
	mvebu_rtc_t *rtc = platform_get_drvdata(pdev);

	if (rtc) {
		rtc_device_unregister(rtc->rtc_dev);
		free_irq(rtc->irq, rtc);
		iounmap(rtc->regbase_rtc);
		res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
		release_mem_region(res->start, resource_size(res));
		res = platform_get_resource(pdev, IORESOURCE_MEM, 1);
		release_mem_region(res->start, resource_size(res));
		platform_set_drvdata(pdev, NULL);
		kfree(rtc);
	}

	return 0;
}

#ifdef CONFIG_OF
static struct of_device_id rtc_mvebu_of_match_table[] = {
	{ .compatible = "marvell,mvebu-rtc", },
	{}
};
#endif

static struct platform_driver mvebu_rtc_driver = {
	.probe      = mvebu_rtc_probe,
	.remove     = __exit_p(mvebu_rtc_remove),
	.driver     = {
		.name   = "mvebu-rtc",
		.owner  = THIS_MODULE,
		.of_match_table = of_match_ptr(rtc_mvebu_of_match_table),
	},
};

static int __init mvebu_rtc_init(void)
{
	return platform_driver_register(&mvebu_rtc_driver);
}

static void __exit mvebu_rtc_exit(void)
{
	platform_driver_unregister(&mvebu_rtc_driver);
}

module_init(mvebu_rtc_init);
module_exit(mvebu_rtc_exit);

MODULE_LICENSE("GPL v2");
MODULE_AUTHOR("Nadav Haklai <nadavh@marvell.com>");
MODULE_DESCRIPTION("Marvell EBU SoC Realtime Clock Driver (RTC)");
