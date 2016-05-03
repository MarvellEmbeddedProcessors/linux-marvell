/*
* ***************************************************************************
* Copyright (C) 2016 Marvell International Ltd.
* ***************************************************************************
* This program is free software: you can redistribute it and/or modify it
* under the terms of the GNU General Public License as published by the Free
* Software Foundation, either version 2 of the License, or any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program.  If not, see <http://www.gnu.org/licenses/>.
* ***************************************************************************
*
* Based on drivers/bus/arm-ccn.c
*
*/

#include <linux/interrupt.h>
#include <linux/irqdomain.h>
#include <linux/module.h>
#include <linux/of.h>
#include <linux/io.h>
#include <linux/perf_event.h>
#include <linux/platform_device.h>
#include <asm/irq_regs.h>

#define pmu_to_spmu(c)	(container_of(c, struct mvebu_spmu, pmu))

#define SPMU_EVCNTR(idx)	(0x0 + (0x8 * (idx)))
#define SPMU_EVTYPER(idx)	(0x400 + (4 * (idx)))
#define SPMU_EVCNTSR(idx)	(0x620 + (4 * (idx)))
#define SPMU_CNTENSET		(0xC00)
#define SPMU_CNTENCLR		(0xC20)
#define SPMU_INTENSET		(0xC40)
#define SPMU_INTENCLR		(0xC60)
#define SPMU_OVSCLR		(0xC80)
#define SPMU_CR_REG		(0xE04)

/*
 * SPMU_CR_REG: config reg
 */
#define SPMU_CR_E		(1 << 0) /* Enable all counters */
#define SPMU_PMCR_P		(1 << 1) /* Reset all counters */
#define SPMU_PMCR_C		(1 << 2) /* Cycle counter reset */
#define	SPMU_CR_N_SHIFT		11	 /* Number of counters supported */
#define	SPMU_CR_N_MASK		0x1f

/*
 * SPMU_EVTYPE: Event selection reg
 */
#define	SPMU_EVTYPE_EVENT	0x3ff		/* Mask for EVENT bits */


#define MVEBU_SPMU_COUNTER_MASK		0xffffffffULL
#define MVEBU_SPMU_COUNTER_MAX		0x7fffffff

#define SPMU_NUM_PERF_COUNTERS		4

struct mvebu_spmu {
	struct device *dev;
	void __iomem *base;
	unsigned int irq;

	spinlock_t config_lock;

	DECLARE_BITMAP(used_mask, SPMU_NUM_PERF_COUNTERS + 1);
	struct {
		struct perf_event *event;
		uint32_t correction;
	} ev_info[SPMU_NUM_PERF_COUNTERS];

	cpumask_t cpu;
	struct notifier_block cpu_nb;
	struct pmu pmu;
};

/*
 * CCU SPMU Performance Events.
 */
enum ccu_pmu_perf_types {
	CCU_SFT_TBL_LOOKUP			= 0x100,
	CCU_SFT_TBL_HIT				= 0x101,
	CCU_SFT_ISSUE_RECLAIM_REQ		= 0x102,
	CCU_SFT_TAG_RAM_PAR_ERR			= 0x103,
	CCU_L3_READ_REQ				= 0x120,
	CCU_L3_READ_MISS			= 0x121,
	CCU_L3_READ_LINEFILL			= 0x122,
	CCU_L3_READ_INST_REQ			= 0x123,
	CCU_L3_READ_INST_MISS			= 0x124,
	CCU_L3_READ_DATA_REQ			= 0x125,
	CCU_L3_READ_DATA_MISS			= 0x126,
	CCU_L3_WRITE_REQ			= 0x127,
	CCU_L3_WRITE_MISS			= 0x128,
	CCU_L3_WRITE_PARTIAL			= 0x129,
	CCU_L3_WRITE_LINEFILL			= 0x12A,
	CCU_L3_MAINTENANCE_REQ			= 0x12B,
	CCU_L3_MAINTENANCE_HIT			= 0x12C,
	CCU_L3_CACHE_LINE_EVECT			= 0x12D,
	CCU_L3_CORR_ECC				= 0x12E,
	CCU_L3_UNCORR				= 0x12F,
	CCU_L3_TAG_MEM_PARITY_ERROR		= 0x130,
	CCU_L3_EVICT_BUF_FULL			= 0x131,
	CCU_L3_LINE_READ_BUFF_FULL		= 0x132,
	CCU_EVENTS_LAST
};

/* Perf definitions for PMU event attributes in sysfs */
ssize_t mvebu_events_sysfs_show(struct device *dev,
				struct device_attribute *attr, char *page)
{
	struct perf_pmu_events_attr *pmu_attr;

	pmu_attr = container_of(attr, struct perf_pmu_events_attr, attr);
	return sprintf(page, "event=0x%04llx\n", pmu_attr->id);
}

#define EVENT_VAR(_cat, _name)		event_attr_##_cat##_##_name
#define EVENT_PTR(_cat, _name)		(&EVENT_VAR(_cat, _name).attr.attr)
#define SPMU_EVENT_PTR(cat, name)	EVENT_PTR(cat, name)

#define SPMU_EVENT_ATTR(cat, name, id)			\
	PMU_EVENT_ATTR(name, EVENT_VAR(cat, name), id, mvebu_events_sysfs_show)

SPMU_EVENT_ATTR(ccu, sft_tbl_lookup,		CCU_SFT_TBL_LOOKUP);
SPMU_EVENT_ATTR(ccu, sft_tbl_hit,		CCU_SFT_TBL_HIT);
SPMU_EVENT_ATTR(ccu, sft_issue_reclaim_req,	CCU_SFT_ISSUE_RECLAIM_REQ);
SPMU_EVENT_ATTR(ccu, sft_tag_ram_par_err,	CCU_SFT_TAG_RAM_PAR_ERR);
SPMU_EVENT_ATTR(ccu, llc_read_linefill,		CCU_L3_READ_LINEFILL);
SPMU_EVENT_ATTR(ccu, llc_read_inst_req,		CCU_L3_READ_INST_REQ);
SPMU_EVENT_ATTR(ccu, llc_read_inst_miss,	CCU_L3_READ_INST_MISS);
SPMU_EVENT_ATTR(ccu, llc_read_data_req,		CCU_L3_READ_DATA_REQ);
SPMU_EVENT_ATTR(ccu, llc_read_data_miss,	CCU_L3_READ_DATA_MISS);
SPMU_EVENT_ATTR(ccu, llc_write_partial,		CCU_L3_WRITE_PARTIAL);
SPMU_EVENT_ATTR(ccu, llc_write_linefill,	CCU_L3_WRITE_LINEFILL);
SPMU_EVENT_ATTR(ccu, llc_maintenance_req,	CCU_L3_MAINTENANCE_REQ);
SPMU_EVENT_ATTR(ccu, llc_maintenance_hit,	CCU_L3_MAINTENANCE_HIT);
SPMU_EVENT_ATTR(ccu, llc_cache_line_evect,	CCU_L3_CACHE_LINE_EVECT);
SPMU_EVENT_ATTR(ccu, llc_corr_ecc,		CCU_L3_CORR_ECC);
SPMU_EVENT_ATTR(ccu, llc_uncorr,		CCU_L3_UNCORR);
SPMU_EVENT_ATTR(ccu, llc_tag_mem_parity_error,	CCU_L3_TAG_MEM_PARITY_ERROR);
SPMU_EVENT_ATTR(ccu, llc_evict_buf_full,	CCU_L3_EVICT_BUF_FULL);
SPMU_EVENT_ATTR(ccu, llc_line_read_buff_full,	CCU_L3_LINE_READ_BUFF_FULL);

static ssize_t mvebu_spmu_cpumask_show(struct device *dev,
				     struct device_attribute *attr, char *buf)
{
	struct mvebu_spmu *spmu = pmu_to_spmu(dev_get_drvdata(dev));

	return cpumap_print_to_pagebuf(true, buf, &spmu->cpu);
}

static struct device_attribute mvebu_spmu_cpumask_attr =
		__ATTR(cpumask, S_IRUGO, mvebu_spmu_cpumask_show, NULL);

static struct attribute *mvebu_spmu_cpumask_attrs[] = {
	&mvebu_spmu_cpumask_attr.attr,
	NULL,
};

static struct attribute_group mvebu_spmu_cpumask_attr_group = {
	.attrs = mvebu_spmu_cpumask_attrs,
};


PMU_FORMAT_ATTR(event,		"config:0-49");

static struct attribute *mvebu_spmu_format_attr[] = {
	&format_attr_event.attr,
	NULL,
};

struct attribute_group mvebu_spmu_format_group = {
	.name = "format",
	.attrs = mvebu_spmu_format_attr,
};

static struct attribute *mvebu_spmu_events_attrs[] = {
	SPMU_EVENT_PTR(ccu, sft_tbl_lookup),
	SPMU_EVENT_PTR(ccu, sft_tbl_hit),
	SPMU_EVENT_PTR(ccu, sft_issue_reclaim_req),
	SPMU_EVENT_PTR(ccu, sft_tag_ram_par_err),
	SPMU_EVENT_PTR(ccu, llc_read_linefill),
	SPMU_EVENT_PTR(ccu, llc_read_inst_req),
	SPMU_EVENT_PTR(ccu, llc_read_inst_miss),
	SPMU_EVENT_PTR(ccu, llc_read_data_req),
	SPMU_EVENT_PTR(ccu, llc_read_data_miss),
	SPMU_EVENT_PTR(ccu, llc_write_partial),
	SPMU_EVENT_PTR(ccu, llc_write_linefill),
	SPMU_EVENT_PTR(ccu, llc_maintenance_req),
	SPMU_EVENT_PTR(ccu, llc_maintenance_hit),
	SPMU_EVENT_PTR(ccu, llc_cache_line_evect),
	SPMU_EVENT_PTR(ccu, llc_corr_ecc),
	SPMU_EVENT_PTR(ccu, llc_uncorr),
	SPMU_EVENT_PTR(ccu, llc_tag_mem_parity_error),
	SPMU_EVENT_PTR(ccu, llc_evict_buf_full),
	SPMU_EVENT_PTR(ccu, llc_line_read_buff_full),
	NULL
};

static struct attribute_group mvebu_spmu_events_attr_group = {
	.name = "events",
	.attrs = mvebu_spmu_events_attrs,
};

static const struct attribute_group *mvebu_spmu_attr_groups[] = {
	&mvebu_spmu_events_attr_group,
	&mvebu_spmu_format_group,
	&mvebu_spmu_cpumask_attr_group,
	NULL
};


#define C(_x) PERF_COUNT_HW_CACHE_##_x

static const u32 mvebu_spmu_cache_events[][C(OP_MAX)][C(RESULT_MAX)] = {
	[C(LL)] = {
		[C(OP_READ)] = {
			[C(RESULT_ACCESS)]	= CCU_L3_READ_REQ,
			[C(RESULT_MISS)]	= CCU_L3_READ_MISS,
		},
		[C(OP_WRITE)] = {
			[C(RESULT_ACCESS)]	= CCU_L3_WRITE_REQ,
			[C(RESULT_MISS)]	= CCU_L3_WRITE_MISS,
		},
	},
};

static int mvebu_spmu_cache_event(u64 config)
{
	unsigned int cache_type, cache_op, cache_result;
	int ret;

	cache_type = (config >>  0) & 0xff;
	cache_op = (config >>  8) & 0xff;
	cache_result = (config >> 16) & 0xff;

	if (cache_type >= ARRAY_SIZE(mvebu_spmu_cache_events) ||
	    cache_op >= C(OP_MAX) ||
	    cache_result >= C(RESULT_MAX))
		return -EINVAL;

	ret = mvebu_spmu_cache_events[cache_type][cache_op][cache_result];

	if (ret == 0)
		return -EINVAL;

	return ret;
}

static inline uint32_t mvebu_spmu_read_counter(struct mvebu_spmu *spmu, int idx)
{
	u32 v;

	v = readl(spmu->base + SPMU_EVCNTR(idx));
	v -= spmu->ev_info[idx].correction;
	return v;
}

static inline void mvebu_spmu_write_counter(struct perf_event *event, int idx, uint32_t v)
{
	uint32_t new_v;
	struct mvebu_spmu *spmu = pmu_to_spmu(event->pmu);

	new_v = (v + 1) >> 1;
	spmu->ev_info[idx].correction = (new_v << 1) - v;
	writel(new_v, spmu->base + SPMU_EVCNTR(idx));
}

static void mvebu_perf_event_update(struct perf_event *event,
				     struct hw_perf_event *hwc, int idx)
{
	struct mvebu_spmu *spmu = pmu_to_spmu(event->pmu);
	uint64_t prev_raw_count, new_raw_count;
	int64_t delta;

	do {
		prev_raw_count = local64_read(&hwc->prev_count);
		new_raw_count = mvebu_spmu_read_counter(spmu, event->hw.idx);
	} while (local64_cmpxchg(&hwc->prev_count, prev_raw_count,
				 new_raw_count) != prev_raw_count);

	delta = (new_raw_count - prev_raw_count) & MVEBU_SPMU_COUNTER_MASK;

	local64_add(delta, &event->count);
	local64_sub(delta, &hwc->period_left);
}

static bool mvebu_perf_event_set_period(struct perf_event *event,
					 struct hw_perf_event *hwc, int idx)
{
	bool rc = false;
	s64 left;

	if (!is_sampling_event(event)) {
		left = MVEBU_SPMU_COUNTER_MAX;
	} else {
		s64 period = hwc->sample_period;

		left = local64_read(&hwc->period_left);
		if (left <= -period) {
			left = period;
			local64_set(&hwc->period_left, left);
			hwc->last_period = period;
			rc = true;
		} else if (left <= 0) {
			left += period;
			local64_set(&hwc->period_left, left);
			hwc->last_period = period;
			rc = true;
		}
		if (left > MVEBU_SPMU_COUNTER_MAX)
			left = MVEBU_SPMU_COUNTER_MAX;
	}

	local64_set(&hwc->prev_count, -left);
	mvebu_spmu_write_counter(event, idx, -left);
	perf_event_update_userpage(event);

	return rc;
}

static void mvebu_spmu_enable(struct pmu *pmu)
{
	struct mvebu_spmu *spmu = pmu_to_spmu(pmu);
	u32 reg;

	/* Enable all counters */
	reg = readl(spmu->base + SPMU_CR_REG);
	reg |= SPMU_CR_E;
	writel(reg, spmu->base + SPMU_CR_REG);
}

static void mvebu_spmu_disable(struct pmu *pmu)
{
	struct mvebu_spmu *spmu = pmu_to_spmu(pmu);
	u32 reg;

	/* Disable all counters */
	reg = readl(spmu->base + SPMU_CR_REG);
	reg &= ~SPMU_CR_E;
	writel(reg, spmu->base + SPMU_CR_REG);
}


static int mvebu_spmu_event_init(struct perf_event *event)
{
	int ret;

	switch (event->attr.type) {
	case PERF_TYPE_HW_CACHE:
		ret = mvebu_spmu_cache_event(event->attr.config);
		if (ret < 0)
			return ret;
		event->hw.config = ret;
		return 0;

	default:
		if ((event->attr.config >= CCU_SFT_TBL_LOOKUP) &&
		    (event->attr.config <= CCU_EVENTS_LAST)) {
			event->hw.config = event->attr.config;
			return 0;
		}

		return -ENOENT;
	}
}


/*
 * Starts/Stops a counter present on the PMU. The PMI handler
 * should stop the counter when perf_event_overflow() returns
 * !0. ->start() will be used to continue.
 */
static void mvebu_spmu_start(struct perf_event *event, int flags)
{
	struct mvebu_spmu *spmu = pmu_to_spmu(event->pmu);
	struct hw_perf_event *hwc = &event->hw;
	int idx = hwc->idx;

	if (WARN_ON_ONCE(idx == -1))
		return;

	if (flags & PERF_EF_RELOAD)
		mvebu_perf_event_set_period(event, hwc, idx);

	hwc->state = 0;
	writel(1 << idx, spmu->base + SPMU_CNTENSET);
	writel(1 << idx, spmu->base + SPMU_INTENSET);
}

static void mvebu_spmu_stop(struct perf_event *event, int flags)
{
	struct mvebu_spmu *spmu = pmu_to_spmu(event->pmu);
	struct hw_perf_event *hwc = &event->hw;
	int idx = hwc->idx;

	if (!(hwc->state & PERF_HES_STOPPED)) {
		writel(1 << idx, spmu->base + SPMU_CNTENCLR);
		writel(1 << idx, spmu->base + SPMU_INTENCLR);
		hwc->state |= PERF_HES_STOPPED;
	}

	if ((flags & PERF_EF_UPDATE) &&
	    !(event->hw.state & PERF_HES_UPTODATE)) {
		mvebu_perf_event_update(event, &event->hw, idx);
		event->hw.state |= PERF_HES_UPTODATE;
	}
}


/*
 * Adds/Removes a counter to/from the PMU, can be done inside
 * a transaction, see the ->*_txn() methods.
 */
static int mvebu_spmu_add(struct perf_event *event, int flags)
{
	struct mvebu_spmu *spmu = pmu_to_spmu(event->pmu);
	struct hw_perf_event *hwc = &event->hw;
	int idx = hwc->idx;
	u32 reg;

	if (__test_and_set_bit(idx, spmu->used_mask)) {
		idx = find_first_zero_bit(spmu->used_mask,
					  SPMU_NUM_PERF_COUNTERS);
		if (idx == SPMU_NUM_PERF_COUNTERS)
			return -EAGAIN;

		__set_bit(idx, spmu->used_mask);
		hwc->idx = idx;
	}
	spmu->ev_info[idx].event = event;

	/* Set the event type. */
	reg = readl(spmu->base + SPMU_EVTYPER(idx));
	reg &= ~SPMU_EVTYPE_EVENT;
	reg |= event->hw.config;
	writel(reg, spmu->base + SPMU_EVTYPER(idx));

	if (flags & PERF_EF_START)
		mvebu_spmu_start(event, PERF_EF_RELOAD);

	perf_event_update_userpage(event);

	return 0;
}

static void mvebu_spmu_del(struct perf_event *event, int flags)
{
	struct mvebu_spmu *spmu = pmu_to_spmu(event->pmu);

	mvebu_spmu_stop(event, PERF_EF_UPDATE);
	__clear_bit(event->hw.idx, spmu->used_mask);
	perf_event_update_userpage(event);
}

static void mvebu_spmu_read(struct perf_event *event)
{
	mvebu_perf_event_update(event, &event->hw, event->hw.idx);
}

irqreturn_t mvebu_spmu_irq_handler(int irq, void *dev_id)
{
	irqreturn_t rc = IRQ_NONE;
	struct mvebu_spmu *spmu = (struct mvebu_spmu *)dev_id;
	struct perf_event *event;
	struct hw_perf_event *hwc;
	u64 last_period;
	u32 reg, i;

	reg = readl(spmu->base + SPMU_OVSCLR);

	for (i = find_first_bit(spmu->used_mask, SPMU_NUM_PERF_COUNTERS);
	     i < SPMU_NUM_PERF_COUNTERS;
	     i = find_next_bit(spmu->used_mask, SPMU_NUM_PERF_COUNTERS, i + 1)) {

		/* Check for overflow event. */
		if (((1 << i) & reg) == 0)
			continue;

		event = spmu->ev_info[i].event;
		hwc = &event->hw;

		mvebu_perf_event_update(event, hwc, i);
		last_period = hwc->last_period;
		if (mvebu_perf_event_set_period(event, hwc, i)) {
			struct perf_sample_data data;
			struct pt_regs *regs = get_irq_regs();

			perf_sample_data_init(&data, 0, last_period);
			if (perf_event_overflow(event, &data, regs))
				mvebu_spmu_stop(event, 0);
		}

		rc = IRQ_HANDLED;
	}

	/* Clear the interrupt. */
	writel(reg, spmu->base + SPMU_OVSCLR);

	return rc;
}


static int mvebu_spmu_cpu_notifier(struct notifier_block *nb,
		unsigned long action, void *hcpu)
{
	struct mvebu_spmu *spmu = container_of(nb, struct mvebu_spmu, cpu_nb);
	unsigned int cpu = (long)hcpu;
	unsigned int target;

	switch (action & ~CPU_TASKS_FROZEN) {
	case CPU_DOWN_PREPARE:
		if (!cpumask_test_and_clear_cpu(cpu, &spmu->cpu))
			break;
		target = cpumask_any_but(cpu_online_mask, cpu);
		if (target >= nr_cpu_ids)
			break;
		perf_pmu_migrate_context(&spmu->pmu, cpu, target);
		cpumask_set_cpu(target, &spmu->cpu);
		if (spmu->irq)
			WARN_ON(irq_set_affinity(spmu->irq, &spmu->cpu) != 0);
		break;
	default:
		break;
	}

	return NOTIFY_OK;
}


static int mvebu_spmu_pmu_init(struct mvebu_spmu *spmu)
{
	char *name = "ccu";
	int err;

	spin_lock_init(&spmu->config_lock);

	/* Pick one CPU which we will use to collect data from SPMU... */
	cpumask_set_cpu(smp_processor_id(), &spmu->cpu);

	/*
	 * Change the cpu mask when the selected one goes offline. Priority is
	 * picked to have a chance to migrate events before perf is notified.
	 */
	spmu->cpu_nb.notifier_call = mvebu_spmu_cpu_notifier;
	spmu->cpu_nb.priority = CPU_PRI_PERF + 1,
	err = register_cpu_notifier(&spmu->cpu_nb);
	if (err)
		goto error_cpu_notifier;

	/* Also make sure that the overflow interrupt is handled by this CPU */
	if (spmu->irq) {
		err = irq_set_affinity(spmu->irq, &spmu->cpu);
		if (err) {
			dev_err(spmu->dev, "Failed to set interrupt affinity!\n");
			goto error_set_affinity;
		}
	}

	spmu->pmu.attr_groups = mvebu_spmu_attr_groups;
	spmu->pmu.pmu_enable = mvebu_spmu_enable;
	spmu->pmu.pmu_disable = mvebu_spmu_disable;
	spmu->pmu.event_init = mvebu_spmu_event_init;
	spmu->pmu.add = mvebu_spmu_add;
	spmu->pmu.del = mvebu_spmu_del;
	spmu->pmu.start = mvebu_spmu_start;
	spmu->pmu.stop = mvebu_spmu_stop;
	spmu->pmu.read = mvebu_spmu_read;

	err = perf_pmu_register(&spmu->pmu, name, -1);
	if (err)
		free_irq(spmu->irq, NULL);

	return err;

error_set_affinity:
	unregister_cpu_notifier(&spmu->cpu_nb);
error_cpu_notifier:
	return err;
}


static int mvebu_spmu_probe(struct platform_device *pdev)
{
	struct mvebu_spmu *spmu;
	struct resource *res;
	int err;

	spmu = devm_kzalloc(&pdev->dev, sizeof(*spmu), GFP_KERNEL);
	if (!spmu)
		return -ENOMEM;

	spmu->dev = &pdev->dev;
	platform_set_drvdata(pdev, spmu);

	res = platform_get_resource(pdev, IORESOURCE_IRQ, 0);
	if (!res) {
		dev_err(spmu->dev, "Failed to retrieve IRQ number.\n");
		return -EINVAL;
	}
	spmu->irq = res->start;

	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	if (!res) {
		dev_err(spmu->dev, "Failed to retrieve registers base address.\n");
		return -EINVAL;
	}

	if (!devm_request_mem_region(spmu->dev, res->start,
			resource_size(res), pdev->name)) {
		dev_err(spmu->dev, "Failed to request mem region.\n");
		return -EBUSY;
	}

	spmu->base = devm_ioremap(spmu->dev, res->start,
				resource_size(res));
	if (!spmu->base) {
		dev_err(spmu->dev, "Failed to map registers region.\n");
		return -EFAULT;
	}

	err = devm_request_irq(spmu->dev, spmu->irq, mvebu_spmu_irq_handler, 0,
			pdev->name /*dev_name(spmu->dev)*/, spmu);
	if (err) {
		dev_err(spmu->dev, "Failed to request SPMU IRQ.\n");
		return err;
	}

	return mvebu_spmu_pmu_init(spmu);
}

static void mvebu_spmu_cleanup(struct mvebu_spmu *spmu)
{
	irq_set_affinity(spmu->irq, cpu_possible_mask);
	unregister_cpu_notifier(&spmu->cpu_nb);
	perf_pmu_unregister(&spmu->pmu);
}

static int mvebu_spmu_remove(struct platform_device *pdev)
{
	struct mvebu_spmu *spmu = platform_get_drvdata(pdev);

	mvebu_spmu_cleanup(spmu);

	return 0;
}

static const struct of_device_id mvebu_spmu_match[] = {
	{ .compatible = "marvell,mvebu-ccu-pmu", },
	{},
};

static struct platform_driver mvebu_spmu_driver = {
	.driver = {
		.name = "mvebu-spmu",
		.of_match_table = mvebu_spmu_match,
	},
	.probe = mvebu_spmu_probe,
	.remove = mvebu_spmu_remove,
};

static int __init mvebu_spmu_init(void)
{
	return platform_driver_register(&mvebu_spmu_driver);
}

static void __exit mvebu_spmu_exit(void)
{
	platform_driver_unregister(&mvebu_spmu_driver);
}

module_init(mvebu_spmu_init);
module_exit(mvebu_spmu_exit);

MODULE_AUTHOR("Shadi Ammouri <shadi@marvell.com>");
MODULE_LICENSE("GPL");

