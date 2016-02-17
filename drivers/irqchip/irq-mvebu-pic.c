/*
 * ARM GIC pic MSI(-X) support
 * Support for Message Signaled Interrupts for systems that
 * implement ARM Generic Interrupt Controller: GICv2m.
 *
 * Copyright (C) 2014 Advanced Micro Devices, Inc.
 * Authors: Suravee Suthikulpanit <suravee.suthikulpanit@amd.com>
 *	    Harish Kasiviswanathan <harish.kasiviswanathan@amd.com>
 *	    Brandon Anderson <brandon.anderson@amd.com>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 as published
 * by the Free Software Foundation.
 */

#define pr_fmt(fmt) "mvebu-pic: " fmt

#include <linux/irq.h>
#include <linux/interrupt.h>
#include <linux/irqchip/chained_irq.h>
#include <linux/irqdomain.h>
#include <linux/kernel.h>
#include <linux/of_address.h>
#include <linux/of_irq.h>
#include <linux/slab.h>
#include <linux/bitops.h>
#include <linux/cpu.h>
#include <linux/irqchip.h>

#define PIC_CAUSE	       0x0
#define PIC_MASK	       0x4

#define PIC_MAX_IRQS		32
#define PIC_MAX_IRQ_MASK	((1UL << PIC_MAX_IRQS) - 1)

struct pic_data {
	struct resource res;	/* PIC register resource */
	void __iomem *base;	/* PIC register base */
	u32 parent_irq;		/* Parent PPI in GIC domain */
	u32 nr_irqs;		/* The number interrupts in this PIC */
	u32 irq_mask;		/* The specific interrupts on this PIC */
	struct irq_domain *domain;
};

static void mvebu_pic_reset(struct pic_data *pic)
{
	/* ACK and mask all interrupts */
	writel(0, pic->base + PIC_MASK);
	writel(PIC_MAX_IRQ_MASK, pic->base + PIC_CAUSE);
}

static void mvebu_pic_eoi_irq(struct irq_data *d)
{
	struct pic_data *pic = irq_data_get_irq_chip_data(d);

	writel(1 << d->hwirq, pic->base + PIC_CAUSE);
}

static void mvebu_pic_mask_irq(struct irq_data *d)
{
	struct pic_data *pic = irq_data_get_irq_chip_data(d);
	u32 reg;

	reg =  readl(pic->base + PIC_MASK);
	reg &= ~(1 << d->hwirq);
	writel(reg, pic->base + PIC_MASK);
}

static void mvebu_pic_unmask_irq(struct irq_data *d)
{
	struct pic_data *pic = irq_data_get_irq_chip_data(d);
	u32 reg;

	reg =  readl(pic->base + PIC_MASK);
	reg |= (1 << d->hwirq);
	writel(reg, pic->base + PIC_MASK);
}

static struct irq_chip mvebu_pic_irq_chip = {
	.name			= "PIC",
	.irq_mask		= mvebu_pic_mask_irq,
	.irq_unmask		= mvebu_pic_unmask_irq,
	.irq_eoi		= mvebu_pic_eoi_irq,
};

static int mvebu_pic_irq_map(struct irq_domain *domain, unsigned int virq, irq_hw_number_t hwirq)
{
	struct pic_data *pic = domain->host_data;

	/* Can't return an error here since it crashes the irq stack.
	 * The kernel assumes that all interrupt bewteen 0 and irq_nr
	 * are valid. However the PIC has holes so let's at least warn
	 * about them */
	if (((1 << hwirq) & pic->irq_mask) == 0)
		pr_warn("Warning: Invalid irq %ld requested\n", hwirq);

	irq_set_percpu_devid(virq);
	irq_set_chip_data(virq, pic);
	irq_set_chip_and_handler(virq, &mvebu_pic_irq_chip, handle_percpu_devid_irq);

	irq_set_status_flags(virq, IRQ_LEVEL);
	irq_set_probe(virq);

	return 0;
}

static const struct irq_domain_ops mvebu_pic_domain_ops = {
	.map = mvebu_pic_irq_map,
	.xlate = irq_domain_xlate_onecell,
};

static void mvebu_pic_handle_cascade_irq(struct irq_desc *desc)
{
	struct pic_data *pic = irq_desc_get_handler_data(desc);
	struct irq_chip *chip = irq_desc_get_chip(desc);
	unsigned long irqmap, irqn;
	unsigned int cascade_irq;

	irqmap = readl_relaxed(pic->base + PIC_CAUSE);
	chained_irq_enter(chip, desc);

	for_each_set_bit(irqn, &irqmap, BITS_PER_LONG) {
		cascade_irq = irq_find_mapping(pic->domain, irqn);
		generic_handle_irq(cascade_irq);
	}

	chained_irq_exit(chip, desc);
}

static struct pic_data *pic_global;
static int mvebu_pic_secondary_init(struct notifier_block *nfb, unsigned long action, void *hcpu)
{
	/* TODO - only here we need to use the global pointer which
	 * doesn't allow this driver to be multi instanced. Once this
	 * is resolved pic_global can be removed */
	if (action == CPU_STARTING || action == CPU_STARTING_FROZEN) {
		mvebu_pic_reset(pic_global);
		enable_percpu_irq(pic_global->parent_irq, IRQ_TYPE_NONE);
	}

	return NOTIFY_OK;
}

static struct notifier_block mvebu_pic_cpu_notifier = {
	.notifier_call = mvebu_pic_secondary_init,
	.priority = 100,
};

static int __init mvebu_pic_of_init(struct device_node *node, struct device_node *parent)
{
	int ret;
	struct pic_data *pic;

	pic = kzalloc(sizeof(struct pic_data), GFP_KERNEL);
	if (!pic)
		return -ENOMEM;
	pic_global = pic;

	ret = of_address_to_resource(node, 0, &pic->res);
	if (ret) {
		pr_err("Failed to allocate pic resource.\n");
		goto err_free_pic;
	}

	pic->base = ioremap(pic->res.start, resource_size(&pic->res));
	if (!pic->base) {
		pr_err("Failed to map pic resource\n");
		ret = -ENOMEM;
		goto err_free_pic;
	}

	if (of_property_read_u32(node, "irq-mask", &pic->irq_mask)) {
		pr_warn("Missing interrupt mask. Assuming %lx\n", PIC_MAX_IRQ_MASK);
		pic->irq_mask = PIC_MAX_IRQ_MASK;
	}
	pic->nr_irqs = get_count_order(pic->irq_mask);

	pic->domain = irq_domain_add_linear(node, pic->nr_irqs, &mvebu_pic_domain_ops, pic);
	if (!pic->domain) {
		pr_err("Failed to allocate irq domain\n");
		ret = -ENOMEM;
		goto err_iounmap;
	}

	pic->parent_irq = irq_of_parse_and_map(node, 0);
	if (pic->parent_irq <= 0) {
		pr_err("Failed to parse parent interrupt\n");
		ret = -EINVAL;
		goto err_free_domain;
	}

#ifdef CONFIG_SMP
	register_cpu_notifier(&mvebu_pic_cpu_notifier);
#endif
	irq_set_chained_handler(pic->parent_irq, mvebu_pic_handle_cascade_irq);
	irq_set_handler_data(pic->parent_irq, pic);

	mvebu_pic_reset(pic);

	pr_info("registered with %d irqs\n", pic->nr_irqs);

	return 0;

err_free_domain:
	irq_domain_remove(pic->domain);
err_iounmap:
	iounmap(pic->base);
err_free_pic:
	kfree(pic);

	return ret;
}

IRQCHIP_DECLARE(mvebu_pic, "marvell,pic", mvebu_pic_of_init);
