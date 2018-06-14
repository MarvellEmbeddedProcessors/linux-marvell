/* Copyright (c) 2018 Marvell semiconductiors inc. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 and
 * only version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#include <linux/kernel.h>
#include <linux/moduleparam.h>
#include <linux/init.h>
#include <linux/types.h>
#include <linux/device.h>
#include <linux/io.h>
#include <linux/of.h>
#include <linux/err.h>
#include <linux/coresight.h>
#include <linux/perf_event.h>
#include <linux/amba/bus.h>
#include <linux/delay.h>
#include "coresight-axi-monitor.h"

static int boot_enable;
module_param_named(boot_enable, boot_enable, int, S_IRUGO);

static bool axim_check_version(struct axim_drvdata *axim)
{
	u32 major = readl_relaxed(axim->base + AXI_MON_VER) & 0xF;

	return (major > AXI_MON_REV_2);
}

static int axim_trace_id(struct coresight_device *csdev)
{
	return 0;
}

static void axim_enable_channel(struct axim_drvdata *axim, int chan_nr)
{
	struct axim_chan_data *chan = &axim->channel[chan_nr];
	u32 reg;
	u64 addr_mask, bus_mask;
	int order, offset;
	u32 reload;

	/* Find the MSB different between both addresses */
	order = ilog2(chan->addr_end ^ chan->addr_start);
	if (order < 0)
		addr_mask = 0;
	else
		addr_mask = ~((1 << (order + 1)) - 1);

	/* Limit the address mask and comperator offset to bus width */
	if (axim->bus_width < 32)
		bus_mask = (1 << axim->bus_width) - 1;
	else
		bus_mask = (0x100000000ULL << (axim->bus_width - 32)) - 1;
	addr_mask &= bus_mask;

	offset = clamp(order - 31, 0, (int)axim->bus_width - 32);

	/* First define the power of 2 aligned window
	 * This is a coarse window for address comparison
	 */
	writel(chan->addr_start & U32_MAX, axim->base +
			AXI_MON_CH_REF_ADDR_L(chan_nr));
	writel(chan->addr_start >> 32, axim->base +
			AXI_MON_CH_REF_ADDR_H(chan_nr));
	writel(addr_mask & U32_MAX, axim->base +
			AXI_MON_CH_USE_ADDR_L(chan_nr));
	writel(addr_mask >> 32, axim->base +
			AXI_MON_CH_USE_ADDR_H(chan_nr));

	/* now set precise addresses in the 32 bit comperator
	 * the comperator can also be used for user field but we
	 * always use it for address field to enable fine grain
	 * address match. The comperator can select which 32 bits
	 * of address to capture so we update the offset accordingly
	 */
	if (addr_mask) {
		writel(chan->addr_start & U32_MAX,
			axim->base + AXI_MON_CH_COMP_MIN(chan_nr));
		writel(chan->addr_end & U32_MAX,
			axim->base + AXI_MON_CH_COMP_MAX(chan_nr));
		reg = AXI_MON_COMP_ENABLE | AXI_MON_COMP_ADDR |
			AXI_MON_COMP_WIDTH_32 | offset;
		writel(reg, axim->base + AXI_MON_CH_COMP_CTL(chan_nr));
	} else {
		writel(0, axim->base + AXI_MON_CH_COMP_CTL(chan_nr));
	}

	writel(chan->user, axim->base + AXI_MON_CH_REF_USER(chan_nr));
	writel(chan->user_mask, axim->base + AXI_MON_CH_USE_USER(chan_nr));
	writel(chan->id, axim->base + AXI_MON_CH_REF_ID(chan_nr));
	writel(chan->id_mask, axim->base + AXI_MON_CH_USE_ID(chan_nr));
	writel(AXI_CHAN_ATTR(chan->domain, chan->cache, chan->qos, chan->prot),
			axim->base + AXI_MON_CH_REF_ATTR(chan_nr));
	writel(AXI_CHAN_ATTR(chan->domain_mask, chan->cache_mask,
			chan->qos_mask, chan->prot_mask),
			axim->base + AXI_MON_CH_REF_ATTR(chan_nr));

	reload = (chan->event_mode == AXIM_EVENT_MODE_OVERFLOW) ?
			(U32_MAX - (chan->event_thresh - 1)) : 0;
	writel(reload, axim->base + AXI_MON_CH_RLD(chan_nr));
	writel(reload, axim->base + AXI_MON_CH_COUNT(chan_nr));

	/* enable event triggering for this channel */
	reg  = readl(axim->base + AXI_MON_CTL);
	reg |= (1 << chan_nr);
	writel(reg, axim->base + AXI_MON_CTL);

	reg = AXI_MON_CHAN_ENABLE | AXI_MON_CHAN_TRIG_ENABLE |
		(chan->event_mode << 4);
	writel(reg, axim->base + AXI_MON_CH_CTL(axim->curr_chan));

	chan->enable = 1;
}

static void axim_disable_channel(struct axim_drvdata *axim, int chan_nr)
{
	struct axim_chan_data *chan = &axim->channel[chan_nr];
	u32 reg;

	reg  = readl(axim->base + AXI_MON_CH_CTL(chan_nr));
	reg &= ~(AXI_MON_CHAN_ENABLE);
	writel(reg, axim->base + AXI_MON_CH_CTL(chan_nr));

	chan->enable = 0;
}

static void axim_reset_channel(struct axim_drvdata *axim, int chan_nr)
{
	struct axim_chan_data *chan = &axim->channel[chan_nr];

	writel(0, axim->base + AXI_MON_CH_CTL(chan_nr));
	writel(0, axim->base + AXI_MON_CH_REF_ADDR_L(chan_nr));
	writel(0, axim->base + AXI_MON_CH_REF_ADDR_H(chan_nr));
	writel(0, axim->base + AXI_MON_CH_USE_ADDR_L(chan_nr));
	writel(0, axim->base + AXI_MON_CH_USE_ADDR_L(chan_nr));
	writel(0, axim->base + AXI_MON_CH_REF_USER(chan_nr));
	writel(0, axim->base + AXI_MON_CH_USE_USER(chan_nr));
	writel(0, axim->base + AXI_MON_CH_REF_ID(chan_nr));
	writel(0, axim->base + AXI_MON_CH_USE_ID(chan_nr));
	writel(0, axim->base + AXI_MON_CH_REF_ATTR(chan_nr));
	writel(0, axim->base + AXI_MON_CH_REF_ATTR(chan_nr));
	writel(0, axim->base + AXI_MON_CH_COMP_MIN(chan_nr));
	writel(0, axim->base + AXI_MON_CH_COMP_MAX(chan_nr));
	writel(0, axim->base + AXI_MON_CH_COMP_CTL(chan_nr));
	writel(0, axim->base + AXI_MON_CH_RLD(chan_nr));
	writel(0, axim->base + AXI_MON_CH_COUNT(chan_nr));

	chan->enable = 0;
}

static int axim_enable(struct coresight_device *csdev,
		       struct perf_event *event, u32 mode)
{
	struct axim_drvdata *axim = dev_get_drvdata(csdev->dev.parent);
	u32 reg;

	axim->enable = true;

	reg  = readl(axim->base + AXI_MON_CTL);
	reg |= AXI_MON_ENABLE;
	if (axim->major < 2)
		reg |= AXI_MON_EVENT_ENABLE;
	writel(reg, axim->base + AXI_MON_CTL);

	dev_info(axim->dev, "AXIM tracing enabled\n");
	return 0;
}

static void axim_disable(struct coresight_device *csdev,
			 struct perf_event *event)
{
	struct axim_drvdata *axim = dev_get_drvdata(csdev->dev.parent);
	u32 reg;

	reg  = readl(axim->base + AXI_MON_CTL);
	reg &= ~(1 << 31);
	writel(reg, axim->base + AXI_MON_CTL);

	axim->enable = false;

	dev_info(axim->dev, "AXIM tracing disabled\n");
}

static void axim_reset(struct axim_drvdata *axim)
{
	int i;

	/* Restore configurations to reset state */
	writel(0, axim->base + AXI_MON_CTL);
	writel(0, axim->base + AXI_MON_DYN_CTL);
	writel(U32_MAX, axim->base + AXI_MON_EV_CLR);

	for (i = 0; i < axim->nr_chan; i++) {
		axim_reset_channel(axim, i);
		memset(&axim->channel[i], 0, sizeof(struct axim_chan_data));
	}

	axim->enable = false;
}

static const struct coresight_ops_source axim_source_ops = {
	.trace_id	= axim_trace_id,
	.enable		= axim_enable,
	.disable	= axim_disable,
};

static const struct coresight_ops axim_cs_ops = {
	.source_ops	= &axim_source_ops,
};

static ssize_t reset_store(struct device *dev,
			   struct device_attribute *attr,
			   const char *buf, size_t size)
{
	struct axim_drvdata *axim = dev_get_drvdata(dev->parent);

	axim_reset(axim);

	return 0;

}
static DEVICE_ATTR_WO(reset);

static ssize_t nr_chan_show(struct device *dev,
			      struct device_attribute *attr,
			      char *buf)
{
	unsigned long val;
	struct axim_drvdata *axim = dev_get_drvdata(dev->parent);

	val = axim->nr_chan;
	return scnprintf(buf, PAGE_SIZE, "%#lx\n", val);
}

static DEVICE_ATTR_RO(nr_chan);

static ssize_t nr_prof_reg_show(struct device *dev,
			      struct device_attribute *attr,
			      char *buf)
{
	unsigned long val;
	struct axim_drvdata *axim = dev_get_drvdata(dev->parent);

	val = axim->nr_prof_reg;
	return scnprintf(buf, PAGE_SIZE, "%#lx\n", val);
}
static DEVICE_ATTR_RO(nr_prof_reg);

static ssize_t version_show(struct device *dev,
			      struct device_attribute *attr,
			      char *buf)
{
	unsigned long val;
	struct axim_drvdata *axim = dev_get_drvdata(dev->parent);

	val = axim->nr_prof_reg;
	return scnprintf(buf, PAGE_SIZE, "%d.%d\n", axim->major, axim->minor);
}
static DEVICE_ATTR_RO(version);

static ssize_t bus_width_show(struct device *dev,
			      struct device_attribute *attr,
			      char *buf)
{
	struct axim_drvdata *axim = dev_get_drvdata(dev->parent);

	return scnprintf(buf, PAGE_SIZE, "%d\n", axim->bus_width);
}
static DEVICE_ATTR_RO(bus_width);

static ssize_t curr_chan_show(struct device *dev,
		       struct device_attribute *attr,
		       char *buf)
{
	unsigned long val;
	struct axim_drvdata *axim = dev_get_drvdata(dev->parent);

	val = axim->curr_chan;
	return scnprintf(buf, PAGE_SIZE, "%#lx\n", val);
}

static ssize_t curr_chan_store(struct device *dev,
			struct device_attribute *attr,
			const char *buf, size_t size)
{
	unsigned long val;
	struct axim_drvdata *axim = dev_get_drvdata(dev->parent);

	if (kstrtoul(buf, 16, &val))
		return -EINVAL;

	if (val > axim->nr_chan)
		return -EINVAL;

	axim->curr_chan = val;
	return size;
}
static DEVICE_ATTR_RW(curr_chan);

static ssize_t counters_show(struct device *dev,
			      struct device_attribute *attr,
			      char *buf)
{
	struct axim_drvdata *axim = dev_get_drvdata(dev->parent);
	int i, ret = 0, size = 0;

	for (i = 0; i < axim->nr_chan; i++) {
		size = scnprintf(buf + ret, PAGE_SIZE, "%#x\n",
				readl(axim->base + AXI_MON_CH_COUNT(i)));
		ret += size;
	}

	return ret;
}
static DEVICE_ATTR_RO(counters);

static ssize_t mon_enable_show(struct device *dev,
		       struct device_attribute *attr,
		       char *buf)
{
	unsigned long val;
	struct axim_drvdata *axim = dev_get_drvdata(dev->parent);

	val = axim->enable;
	return scnprintf(buf, PAGE_SIZE, "%ld\n", val);
}

static ssize_t mon_enable_store(struct device *dev,
			struct device_attribute *attr,
			const char *buf, size_t size)
{
	unsigned long val;
	struct axim_drvdata *axim = dev_get_drvdata(dev->parent);

	if (kstrtoul(buf, 16, &val))
		return -EINVAL;

	if (val)
		axim_enable(axim->csdev, NULL, CS_MODE_SYSFS);
	else
		axim_disable(axim->csdev, NULL);

	return size;
}
static DEVICE_ATTR_RW(mon_enable);


static ssize_t prof_enable_show(struct device *dev,
		       struct device_attribute *attr,
		       char *buf)
{
	unsigned long val;
	struct axim_drvdata *axim = dev_get_drvdata(dev->parent);

	val = readl(axim->base + AXI_MON_PR_CTL);
	val = (val >> AXI_MON_PROF_EN_OFF) & 0x1;
	return scnprintf(buf, PAGE_SIZE, "%ld\n", val);
}

static ssize_t prof_enable_store(struct device *dev,
			struct device_attribute *attr,
			const char *buf, size_t size)
{
	unsigned long reg, val;
	struct axim_drvdata *axim = dev_get_drvdata(dev->parent);

	if (!axim->prof_en)
		return -EPERM;

	if (kstrtoul(buf, 16, &val))
		return -EINVAL;

	reg = readl(axim->base + AXI_MON_PR_CTL);
	if (val) {
		reg |= (1 << AXI_MON_PROF_EN_OFF);
		reg &= ~(AXI_MON_PROF_CYCG_MASK	<< AXI_MON_PROF_CYCG_OFF);
		reg |= (AXI_MON_PROF_CYCG_4_CYC	<< AXI_MON_PROF_CYCG_OFF);
		if (AXI_MON_PROF_CYCG_4_CYC != 0)
			axim->prof_cyc_mul = 1 << (AXI_MON_PROF_CYCG_4_CYC + 1);
		else
			axim->prof_cyc_mul = 1;
	} else {
		reg &= ~(1 << AXI_MON_PROF_EN_OFF);
	}
	writel(reg, axim->base + AXI_MON_PR_CTL);

	return size;
}
static DEVICE_ATTR_RW(prof_enable);

static char *memfmt(char *buf, unsigned long n)
{
	if (n >= (1UL << 30))
		sprintf(buf, "%lu GB", n >> 30);
	else if (n >= (1UL << 20))
		sprintf(buf, "%lu MB", n >> 20);
	else
		sprintf(buf, "%lu KB", n >> 10);
	return buf;
}


static ssize_t prof_counters_show(struct device *dev,
		       struct device_attribute *attr,
		       char *buf)
{
	int timeout = 100;
	ssize_t size = 0;
	uint32_t msec, val, trans, temp;
	uint64_t val_64;
	uint32_t min_lat, max_lat, total_lat;
	struct axim_drvdata *axim = dev_get_drvdata(dev->parent);
	char fmt_buf[16];

	/* Start event sampling */
	val = readl(axim->base + AXI_MON_EV_SW_TRIG);
	val |= AXI_EV_SW_TRIG_SAMPLE_EN;
	writel(val, axim->base + AXI_MON_EV_SW_TRIG);

	/* Wait till sampling is done. */
	while (timeout) {
		val = readl(axim->base + AXI_MON_STAT) & AXI_MON_STAT_SIP_MASK;
		if (!val)
			break;
		udelay(10);
		timeout--;
	}

	if (!timeout) {
		size += scnprintf(buf, PAGE_SIZE,
				"Error - Event sampling timeout.\n");
		return size;
	}

	/* # of cycles. */
	val_64 = readl(axim->base + AXI_MON_PR_SMP_CYC) * axim->prof_cyc_mul;
	msec = val_64 / axim->clock_freq_mhz / 1000;
	size += scnprintf(buf + size, PAGE_SIZE,
			"Cycles  - %10u [%d msec]\n", (u32)val_64, msec);

	/* # of transactions */
	trans = readl(axim->base + AXI_MON_PR_SMP_TRANS);
	size += scnprintf(buf + size, PAGE_SIZE,
			"Trans   - %10u [%d trans/sec]\n",
			trans, (u32)((u64)trans * 1000 / msec));

	/* # of AXI beats */
	val = readl(axim->base + AXI_MON_PR_SMP_BEATS);
	size += scnprintf(buf + size, PAGE_SIZE,
			"Beats   - %10u [%d beats/sec]\n",
			val, (u32)((u64)val * 1000 / msec));

	/* # of Bytes */
	val = readl(axim->base + AXI_MON_PR_SMP_BYTES);
	temp = (u32)((u64)val * 1000 / msec);
	size += scnprintf(buf + size, PAGE_SIZE,
			"Bytes   - %10u [%d B/sec, %sps]\n"
			, val, temp, memfmt(fmt_buf, temp));

	/* Latency */
	total_lat = readl(axim->base + AXI_MON_PR_SMP_LATEN);
	max_lat = readl(axim->base + AXI_MON_PR_SMP_MAX);
	min_lat = readl(axim->base + AXI_MON_PR_SMP_MIN);

	/*
	 * Convert latency values from clock cycles to nsec.
	 * Multiply by 1000 and divide by MHz
	 */
	size += scnprintf(buf + size, PAGE_SIZE,
		"Latency - %10u [Avg - %u ns]\n", total_lat,
		(u32)((u64)total_lat * 1000 / trans / axim->clock_freq_mhz));
	size += scnprintf(buf + size, PAGE_SIZE, "Min lat - %10u [%d ns]\n",
			min_lat, min_lat * 1000 / axim->clock_freq_mhz);
	size += scnprintf(buf + size, PAGE_SIZE, "Max lat - %10u [%d ns]\n",
			max_lat, max_lat * 1000 / axim->clock_freq_mhz);

	/* Resume event collection */
	val = readl(axim->base + AXI_MON_EV_CLR);
	val |= AXI_MON_EV_SMPR;
	writel(val, axim->base + AXI_MON_EV_CLR);

	if (val & AXI_MON_EV_PE)
		size += scnprintf(buf + size, PAGE_SIZE,
				"Warning - Counter overflow occurred in one of the SMP counters.\n");

	return size;
}
static DEVICE_ATTR_RO(prof_counters);

static ssize_t freeze_show(struct device *dev,
		       struct device_attribute *attr,
		       char *buf)
{
	unsigned long val;
	struct axim_drvdata *axim = dev_get_drvdata(dev->parent);

	/* TODO - create the sysfs dynamically only if rev > 1 */
	if (axim->major < 2)
		return -EINVAL;

	val = readl(axim->base + AXI_MON_DYN_CTL);
	val = (val >> DYN_CTL_FREEZE_OFF) & 0x1;
	return scnprintf(buf, PAGE_SIZE, "%ld\n", val);
}

static ssize_t freeze_store(struct device *dev,
			struct device_attribute *attr,
			const char *buf, size_t size)
{
	unsigned long val;
	struct axim_drvdata *axim = dev_get_drvdata(dev->parent);
	u32 reg;

	/* TODO - create the sysfs dynamically only if rev > 1 */
	if (axim->major < 2)
		return -EINVAL;

	if (kstrtoul(buf, 16, &val))
		return -EINVAL;

	if ((val != 1) && (val != 0))
		return -EINVAL;

	reg  = readl(axim->base + AXI_MON_DYN_CTL);
	reg &= ~(1 << DYN_CTL_FREEZE_OFF);
	reg |= val << DYN_CTL_FREEZE_OFF;
	writel(reg, axim->base + AXI_MON_DYN_CTL);

	return size;
}
static DEVICE_ATTR_RW(freeze);

static struct attribute *coresight_axim_attrs[] = {
	&dev_attr_nr_chan.attr,
	&dev_attr_nr_prof_reg.attr,
	&dev_attr_version.attr,
	&dev_attr_bus_width.attr,
	&dev_attr_curr_chan.attr,
	&dev_attr_reset.attr,
	&dev_attr_counters.attr,
	&dev_attr_mon_enable.attr,
	&dev_attr_freeze.attr,
	&dev_attr_prof_enable.attr,
	&dev_attr_prof_counters.attr,
	NULL,
};

#define axi_chan_attr(name, max)					\
static ssize_t name##_show(struct device *dev,				\
			      struct device_attribute *attr,		\
			      char *buf)				\
{									\
	unsigned long val;						\
	struct axim_drvdata *axim = dev_get_drvdata(dev->parent);	\
									\
	val = axim->channel[axim->curr_chan].name;		\
	return scnprintf(buf, PAGE_SIZE, "%#lx\n", val);		\
}									\
									\
static ssize_t name##_store(struct device *dev,				\
			struct device_attribute *attr,			\
			const char *buf, size_t size)			\
{									\
	unsigned long val;						\
	struct axim_drvdata *axim = dev_get_drvdata(dev->parent);	\
									\
	if (kstrtoul(buf, 16, &val))					\
		return -EINVAL;						\
	if (val > max)							\
		return -EINVAL;						\
									\
	axim->channel[axim->curr_chan].name = val;		\
									\
	return size;							\
}									\
static DEVICE_ATTR_RW(name)

axi_chan_attr(addr_start,  0xffffffffffffffff);
axi_chan_attr(addr_end, 0xffffffffffffffff);
axi_chan_attr(user, 0xffff);
axi_chan_attr(user_mask, 0xffff);
axi_chan_attr(id, 0xffff);
axi_chan_attr(id_mask, 0xffff);
axi_chan_attr(domain, 0x3);
axi_chan_attr(domain_mask, 0x3);
axi_chan_attr(cache, 0xf);
axi_chan_attr(cache_mask, 0xf);
axi_chan_attr(qos, 0x3);
axi_chan_attr(qos_mask, 0x3);
axi_chan_attr(prot, 0x3);
axi_chan_attr(prot_mask, 0x3);
axi_chan_attr(event_mode, 0x1);
axi_chan_attr(event_thresh, 0xffffffff);

static ssize_t enable_show(struct device *dev,
		       struct device_attribute *attr,
		       char *buf)
{
	unsigned long val;
	struct axim_drvdata *axim = dev_get_drvdata(dev->parent);

	val = readl_relaxed(axim->base + AXI_MON_CH_CTL(axim->curr_chan));
	val  = BMVAL(val, 31, 31);
	return scnprintf(buf, PAGE_SIZE, "%#lx\n", val);
}

static ssize_t enable_store(struct device *dev,
			struct device_attribute *attr,
			const char *buf, size_t size)
{
	unsigned long val;
	struct axim_drvdata *axim = dev_get_drvdata(dev->parent);

	if (kstrtoul(buf, 16, &val))
		return -EINVAL;

	if ((val != 1) && (val != 0))
		return -EINVAL;

	if (val)
		axim_enable_channel(axim, axim->curr_chan);
	else
		axim_disable_channel(axim, axim->curr_chan);

	return size;
}
static DEVICE_ATTR_RW(enable);

static ssize_t counter_show(struct device *dev,
		       struct device_attribute *attr,
		       char *buf)
{
	unsigned long reload, count;
	struct axim_drvdata *axim = dev_get_drvdata(dev->parent);

	count  = readl_relaxed(axim->base + AXI_MON_CH_COUNT(axim->curr_chan));
	reload = readl_relaxed(axim->base + AXI_MON_CH_RLD(axim->curr_chan));
	return scnprintf(buf, PAGE_SIZE, "%#lx\n", count - reload);
}

static ssize_t counter_store(struct device *dev,
			struct device_attribute *attr,
			const char *buf, size_t size)
{
	unsigned long val;
	struct axim_drvdata *axim = dev_get_drvdata(dev->parent);

	if (kstrtoul(buf, 16, &val))
		return -EINVAL;

	writel(val, axim->base + AXI_MON_CH_COUNT(axim->curr_chan));

	return size;
}
static DEVICE_ATTR_RW(counter);

static struct attribute *coresight_axim_chan_attrs[] = {
	&dev_attr_addr_start.attr,
	&dev_attr_addr_end.attr,
	&dev_attr_user.attr,
	&dev_attr_user_mask.attr,
	&dev_attr_id.attr,
	&dev_attr_id_mask.attr,
	&dev_attr_domain.attr,
	&dev_attr_domain_mask.attr,
	&dev_attr_cache.attr,
	&dev_attr_cache_mask.attr,
	&dev_attr_prot.attr,
	&dev_attr_prot_mask.attr,
	&dev_attr_qos.attr,
	&dev_attr_qos_mask.attr,
	&dev_attr_counter.attr,
	&dev_attr_event_thresh.attr,
	&dev_attr_event_mode.attr,
	&dev_attr_enable.attr,
	NULL,
};

static const struct attribute_group coresight_axim_group = {
	.attrs = coresight_axim_attrs,
};

static const struct attribute_group coresight_axim_chan_group = {
	.attrs = coresight_axim_chan_attrs,
	.name = "channel",
};

static const struct attribute_group *coresight_axim_groups[] = {
	&coresight_axim_group,
	&coresight_axim_chan_group,
	NULL,
};

static void axim_init_default_data(struct axim_drvdata *axim)
{
	u32 reg;

	reg = readl_relaxed(axim->base + AXI_MON_VER);
	axim->nr_prof_reg = BMVAL(reg, 16, 19); /* NPRR */
	axim->latency_en = BMVAL(reg, 24, 24);
	axim->trace_en = BMVAL(reg, 25, 25);
	axim->minor = BMVAL(reg, 4, 7);
	axim->major = BMVAL(reg, 0, 3) + 1;

	/* AXIM v1 doesn't have a version register and will
	 * return 0 when reading from AXI_MON_VER. For all fields it
	 * emulates a VER register fine except for nr_chan
	 */
	if (reg) {
		axim->nr_chan = BMVAL(reg, 12, 15); /* NCH */
		axim->prof_en = true;
	} else {
		axim->nr_chan = 4;
		axim->prof_en = false;
	}
}

static int axim_probe(struct amba_device *adev, const struct amba_id *id)
{
	void __iomem *base;
	struct device *dev = &adev->dev;
	struct coresight_platform_data *pdata = NULL;
	struct axim_drvdata *axim;
	struct resource *res = &adev->res;
	struct coresight_desc desc = { 0 };
	struct device_node *np = adev->dev.of_node;
	int ret;

	axim = devm_kzalloc(dev, sizeof(*axim), GFP_KERNEL);
	if (!axim)
		return -ENOMEM;

	if (np) {
		pdata = of_get_coresight_platform_data(dev, np);
		if (IS_ERR(pdata))
			return PTR_ERR(pdata);
		adev->dev.platform_data = pdata;
	}

	axim->dev = &adev->dev;
	dev_set_drvdata(dev, axim);

	/* Validity for the resource is already checked by the AMBA core */
	base = devm_ioremap_resource(dev, res);
	if (IS_ERR(base))
		return PTR_ERR(base);

	axim->base = base;

	ret = of_property_read_u32(np, "bus-width", &axim->bus_width);
	if ((ret) || (axim->bus_width > AXI_MON_MAX_BUS_WIDTH)) {
		dev_warn(dev, "Bad or missing bus-width property. assuming %d bit width\n",
				AXI_MON_MAX_BUS_WIDTH);
		axim->bus_width = AXI_MON_MAX_BUS_WIDTH;
	}

	if (axim_check_version(axim))
		return -EINVAL;

	axim_init_default_data(axim);
	axim_reset(axim);

	if (axim->prof_en) {
		axim->clk = devm_clk_get(dev, "hclk");
		if (IS_ERR(axim->clk)) {
			dev_warn(dev, "Cannot get profiling clock frequency, Disabling profiling support.\n");
			axim->prof_en = false;
		} else {
			ret = clk_prepare_enable(axim->clk);
			if (ret)
				return ret;
			axim->clock_freq_mhz =
				clk_get_rate(axim->clk) / 1000000;
		}

	}

	desc.type = CORESIGHT_DEV_TYPE_SOURCE;
	desc.subtype.source_subtype = CORESIGHT_DEV_SUBTYPE_SOURCE_PROC;
	desc.ops = &axim_cs_ops;
	desc.pdata = pdata;
	desc.dev = dev;
	desc.groups = coresight_axim_groups;
	axim->csdev = coresight_register(&desc);
	if (IS_ERR(axim->csdev))
		return PTR_ERR(axim->csdev);

	dev_info(dev, "%s %d.%d initialized\n",
			(char *)id->data, axim->major, axim->minor);

	if (boot_enable) {
		coresight_enable(axim->csdev);
		axim->boot_enable = true;
	}

	return 0;
}

const static struct amba_id axim_ids[] = {
	{       /* AXI Monitor Marvell*/
		.id	= 0x000e9ae1,
		.mask	= 0x000fffff,
		.data	= "AXIM 4.0",
	},
	{ 0, 0},
};

static struct amba_driver axim_driver = {
	.drv = {
		.name   = "coresight-axim",
		.suppress_bind_attrs = true,
	},
	.probe		= axim_probe,
	.id_table	= axim_ids,
};

builtin_amba_driver(axim_driver);
