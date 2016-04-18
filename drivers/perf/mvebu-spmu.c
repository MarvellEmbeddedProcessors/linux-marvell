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
*/

#include <asm/irq_regs.h>

#include <linux/of.h>
#include <linux/perf/arm_pmu.h>
#include <linux/platform_device.h>
#include <linux/io.h>

#define SPMU_EVCNTR(idx)	(0x0 + (0x8 * (idx)))
#define SPMU_EVTYPER(idx)	(0x400 + (4 * (idx)))
#define SPMU_EVCNTSR(idx)	(0x620 + (4 * (idx)))
#define SPMU_CNTENSET(idx)	(0xC00)
#define SPMU_CNTENCLR(idx)	(0xC20)
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


struct ccu_data {
	void __iomem		*reg_base;
};


/*
 * CCU SPMU Performance Events.
 */
enum ccu_pmu_perf_types {
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
};

/* CCU-SPMU HW events mapping. */
static const unsigned ccu_pmu_perf_map[PERF_COUNT_HW_MAX] = {
	PERF_MAP_ALL_UNSUPPORTED,
};


static const unsigned ccu_pmu_perf_cache_map[PERF_COUNT_HW_CACHE_MAX]
						[PERF_COUNT_HW_CACHE_OP_MAX]
						[PERF_COUNT_HW_CACHE_RESULT_MAX] = {
	PERF_CACHE_MAP_ALL_UNSUPPORTED,

	[C(LL)][C(OP_READ)][C(RESULT_ACCESS)]	= CCU_L3_READ_REQ,
	[C(LL)][C(OP_READ)][C(RESULT_MISS)]	= CCU_L3_READ_MISS,
	[C(LL)][C(OP_WRITE)][C(RESULT_ACCESS)]	= CCU_L3_WRITE_REQ,
	[C(LL)][C(OP_WRITE)][C(RESULT_MISS)]	= CCU_L3_WRITE_MISS,
};


static inline int mvebu_spmu_counter_valid(struct arm_pmu *cpu_pmu, int idx)
{
	return idx >= 0 && idx < cpu_pmu->num_events;
}

static inline u32 mvebu_spmu_read_counter(struct perf_event *event)
{
	struct arm_pmu *cpu_pmu = to_arm_pmu(event->pmu);
	struct hw_perf_event *hwc = &event->hw;
	int idx = hwc->idx;
	u32 value = 0;
	struct ccu_data *ccu_data = platform_get_drvdata(cpu_pmu->plat_device);

	if (!mvebu_spmu_counter_valid(cpu_pmu, idx))
		pr_err("CPU%u reading wrong counter %d\n",
			smp_processor_id(), idx);
	else
		value = readl(ccu_data->reg_base + SPMU_EVCNTR(idx));

	return value;
}

static inline void mvebu_spmu_write_counter(struct perf_event *event, u32 value)
{
	struct arm_pmu *cpu_pmu = to_arm_pmu(event->pmu);
	struct ccu_data *ccu_data = platform_get_drvdata(cpu_pmu->plat_device);
	struct hw_perf_event *hwc = &event->hw;
	int idx = hwc->idx;

	value = (value + 1) >> 1;

	if (!mvebu_spmu_counter_valid(cpu_pmu, idx))
		pr_err("CPU%u writing wrong counter %d\n",
				smp_processor_id(), idx);
	else
		writel(value, ccu_data->reg_base + SPMU_EVCNTR(idx));
}

static inline void mvebu_spmu_write_evtype(struct ccu_data *ccu_data, int idx, u32 val)
{
	u32 reg;

	reg = readl(ccu_data->reg_base + SPMU_EVTYPER(idx));
	reg &= ~SPMU_EVTYPE_EVENT;
	reg |= val;
	writel(reg, ccu_data->reg_base + SPMU_EVTYPER(idx));
}

static inline int mvebu_spmu_enable_counter(struct ccu_data *ccu_data, int idx)
{
	writel(1 << idx, ccu_data->reg_base + SPMU_CNTENSET(idx));
	return idx;
}

static inline int mvebu_spmu_disable_counter(struct ccu_data *ccu_data, int idx)
{
	writel(1 << idx, ccu_data->reg_base + SPMU_CNTENCLR(idx));
	return idx;
}

static void mvebu_spmu_enable_event(struct perf_event *event)
{
	unsigned long flags;
	struct hw_perf_event *hwc = &event->hw;
	struct arm_pmu *cpu_pmu = to_arm_pmu(event->pmu);
	struct ccu_data *ccu_data = platform_get_drvdata(cpu_pmu->plat_device);
	struct pmu_hw_events *events = this_cpu_ptr(cpu_pmu->hw_events);
	int idx = hwc->idx;

	/*
	 * Enable counter and interrupt, and set the counter to count
	 * the event that we're interested in.
	 */
	raw_spin_lock_irqsave(&events->pmu_lock, flags);

	/*
	 * Disable counter
	 */
	mvebu_spmu_disable_counter(ccu_data, idx);

	/*
	 * Set event (if destined for PMNx counters).
	 */
	mvebu_spmu_write_evtype(ccu_data, idx, hwc->config_base);

	/*
	 * Enable counter
	 */
	mvebu_spmu_enable_counter(ccu_data, idx);

	raw_spin_unlock_irqrestore(&events->pmu_lock, flags);
}

static void mvebu_spmu_disable_event(struct perf_event *event)
{
	unsigned long flags;
	struct hw_perf_event *hwc = &event->hw;
	struct arm_pmu *cpu_pmu = to_arm_pmu(event->pmu);
	struct pmu_hw_events *events = this_cpu_ptr(cpu_pmu->hw_events);
	int idx = hwc->idx;
	struct ccu_data *ccu_data = platform_get_drvdata(cpu_pmu->plat_device);

	/*
	 * Disable counter and interrupt
	 */
	raw_spin_lock_irqsave(&events->pmu_lock, flags);

	/*
	 * Disable counter
	 */
	mvebu_spmu_disable_counter(ccu_data, idx);

	raw_spin_unlock_irqrestore(&events->pmu_lock, flags);
}

static void mvebu_spmu_start(struct arm_pmu *cpu_pmu)
{
	struct ccu_data *ccu_data = platform_get_drvdata(cpu_pmu->plat_device);
	struct pmu_hw_events *events = this_cpu_ptr(cpu_pmu->hw_events);
	unsigned long flags;
	u32 reg;

	raw_spin_lock_irqsave(&events->pmu_lock, flags);

	/* Enable all counters */
	reg = readl(ccu_data->reg_base + SPMU_CR_REG);
	reg |= SPMU_CR_E;
	writel(reg, ccu_data->reg_base + SPMU_CR_REG);
	raw_spin_unlock_irqrestore(&events->pmu_lock, flags);
}

static void mvebu_spmu_stop(struct arm_pmu *cpu_pmu)
{
	struct ccu_data *ccu_data = platform_get_drvdata(cpu_pmu->plat_device);
	struct pmu_hw_events *events = this_cpu_ptr(cpu_pmu->hw_events);
	unsigned long flags;
	u32 reg;

	raw_spin_lock_irqsave(&events->pmu_lock, flags);
	/* Disable all counters */
	reg = readl(ccu_data->reg_base + SPMU_CR_REG);
	reg &= ~SPMU_CR_E;
	writel(reg, ccu_data->reg_base + SPMU_CR_REG);
	raw_spin_unlock_irqrestore(&events->pmu_lock, flags);
}

static int mvebu_spmu_get_event_idx(struct pmu_hw_events *cpuc,
				  struct perf_event *event)
{
	int idx;
	struct arm_pmu *cpu_pmu = to_arm_pmu(event->pmu);

	/*
	 * For anything other than a cycle counter, try and use
	 * the events counters
	 */
	for (idx = 1; idx < cpu_pmu->num_events; ++idx) {
		if (!test_and_set_bit(idx, cpuc->used_mask))
			return idx;
	}

	/* The counters are all in use. */
	return -EAGAIN;
}

static void mvebu_spmu_reset(void *info)
{
	struct arm_pmu *cpu_pmu = (struct arm_pmu *)info;
	struct ccu_data *ccu_data = platform_get_drvdata(cpu_pmu->plat_device);
	u32 idx, nb_cnt = cpu_pmu->num_events;
	u32 reg;

	/* Reset the counters only once */
	if (smp_processor_id() != 0)
		return;

	/* The counter and interrupt enable registers are unknown at reset. */
	for (idx = 0; idx < nb_cnt; ++idx)
		mvebu_spmu_disable_counter(ccu_data, idx);

	/* Initialize & Reset PMNC: C and P bits. */
	reg = readl(ccu_data->reg_base + SPMU_CR_REG);
	reg |= (SPMU_PMCR_P | SPMU_PMCR_C);
	writel(reg, ccu_data->reg_base + SPMU_CR_REG);
}

static int ccu_pmu_map_event(struct perf_event *event)
{
	int ret;

	ret = armpmu_map_event(event, &ccu_pmu_perf_map,
			&ccu_pmu_perf_cache_map,
			SPMU_EVTYPE_EVENT);
	return ret;
}

static void mvebu_spmu_read_num_events(void *info)
{
	struct arm_pmu *cpu_pmu = (struct arm_pmu *)info;
	struct ccu_data *ccu_data = platform_get_drvdata(cpu_pmu->plat_device);
	u32 reg;

	/* Read the nb of CNTx counters supported from CR reg */
	reg = readl(ccu_data->reg_base + SPMU_CR_REG);

	cpu_pmu->num_events += ((reg >> SPMU_CR_N_SHIFT) & SPMU_CR_N_MASK);
}

static int mvebu_spmu_probe_num_events(struct arm_pmu *arm_pmu)
{
	return smp_call_function_any(&arm_pmu->supported_cpus,
				    mvebu_spmu_read_num_events,
				    arm_pmu, 1);
}

static int ccu_pmu_init(struct arm_pmu *cpu_pmu)
{
	cpu_pmu->enable			= mvebu_spmu_enable_event,
	cpu_pmu->disable		= mvebu_spmu_disable_event,
	cpu_pmu->read_counter		= mvebu_spmu_read_counter,
	cpu_pmu->write_counter		= mvebu_spmu_write_counter,
	cpu_pmu->get_event_idx		= mvebu_spmu_get_event_idx,
	cpu_pmu->start			= mvebu_spmu_start,
	cpu_pmu->stop			= mvebu_spmu_stop,
	cpu_pmu->reset			= mvebu_spmu_reset,
	cpu_pmu->max_period		= 0xFFFFFFFFll,
	cpu_pmu->name			= "mvebu_ccu_pmu";
	cpu_pmu->map_event		= ccu_pmu_map_event;
	return mvebu_spmu_probe_num_events(cpu_pmu);
}

static const struct of_device_id mvebu_spmu_of_device_ids[] = {
	{.compatible = "marvell,mvebu-ccu-pmu",	.data = ccu_pmu_init},
	{},
};

static int mvebu_spmu_device_probe(struct platform_device *pdev)
{
	struct ccu_data	*drv_data;
	struct resource	*r;

	drv_data = devm_kzalloc(&pdev->dev, sizeof(struct ccu_data), GFP_KERNEL);
	if (!drv_data)
		return -ENOMEM;

	r = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	drv_data->reg_base = devm_ioremap_resource(&pdev->dev, r);
	if (IS_ERR(drv_data->reg_base))
		return PTR_ERR(drv_data->reg_base);

	platform_set_drvdata(pdev, drv_data);

	return arm_pmu_device_probe(pdev, mvebu_spmu_of_device_ids, NULL);
}

static struct platform_driver mvebu_spmu_driver = {
	.driver		= {
		.name	= "mvebu-spmu",
		.of_match_table = mvebu_spmu_of_device_ids,
	},
	.probe		= mvebu_spmu_device_probe,
};

static int __init register_ccu_pmu_driver(void)
{
	return platform_driver_register(&mvebu_spmu_driver);
}
device_initcall(register_ccu_pmu_driver);

