/*
* ***************************************************************************
* Copyright (C) 2016 Marvell International Ltd.
* ***************************************************************************
*
* Redistribution and use in source and binary forms, with or without
* modification, are permitted provided that the following conditions are met:
*
* Redistributions of source code must retain the above copyright notice, this
* list of conditions and the following disclaimer.
*
* Redistributions in binary form must reproduce the above copyright notice,
* this list of conditions and the following disclaimer in the documentation
* and/or other materials provided with the distribution.
*
* Neither the name of Marvell nor the names of its contributors may be used
* to endorse or promote products derived from this software without specific
* prior written permission.
*
* THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
* AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
* IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
* ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
* LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY,
* OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
* SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
* INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
* CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
* ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
* POSSIBILITY OF SUCH DAMAGE.
*
***************************************************************************
*/


#define pr_fmt(fmt) "mvebu-sei: " fmt

#include <linux/irq.h>
#include <linux/interrupt.h>
#include <linux/irqchip/chained_irq.h>
#include <linux/irqdomain.h>
#include <linux/kernel.h>
#include <linux/of_address.h>
#include <linux/of_irq.h>
#include <linux/irqchip.h>

#define GICP_SECR(idx)		(0x0  + (idx * 0x4)) /* Cause register */
#define GICP_SEMR(idx)		(0x20 + (idx * 0x4)) /* Mask register */

#define SEI_IRQS_PER_REG_CNT	32
#define SEI_IRQ_MASK_VAL	0
#define SEI_IRQ_CAUSE_VAL	0xFFFFFFFF
				/* Write 1 (to all bits) to clear the cause bit */

#define SEI_IRQ_REG_CNT		2
#define SEI_IRQ_REG_IDX(irq_id)	(irq_id / SEI_IRQS_PER_REG_CNT)
#define SEI_IRQ_REG_BIT(irq_id)	(irq_id % SEI_IRQS_PER_REG_CNT)

struct sei_data {
	struct resource res;	/* SEI register resource */
	void __iomem *base;	/* SEI register base */
	u32 parent_irq;		/* Parent IRQ */
	u32 nr_irqs;		/* The number interrupts in this SEI */
	struct irq_domain *domain;
};

static void mvebu_sei_reset(struct sei_data *sei)
{
	unsigned int i;

	/* ACK and mask all interrupts */
	for (i = 0; i < SEI_IRQ_REG_CNT; i++) {
		writel(SEI_IRQ_CAUSE_VAL, sei->base + GICP_SECR(i));
		writel(SEI_IRQ_MASK_VAL, sei->base + GICP_SEMR(i));
	}
}

static void mvebu_sei_mask_irq(struct irq_data *d)
{
	struct sei_data *sei = irq_data_get_irq_chip_data(d);
	u32 reg_idx = SEI_IRQ_REG_IDX(d->hwirq);
	u32 reg;

	reg =  readl(sei->base + GICP_SEMR(reg_idx));
	/* 1 disables the interrupt */
	reg |= (1 << SEI_IRQ_REG_BIT(d->hwirq));
	writel(reg, sei->base + GICP_SEMR(reg_idx));
}

static void mvebu_sei_unmask_irq(struct irq_data *d)
{
	struct sei_data *sei = irq_data_get_irq_chip_data(d);
	u32 reg_idx = SEI_IRQ_REG_IDX(d->hwirq);
	u32 reg;

	reg =  readl(sei->base + GICP_SEMR(reg_idx));
	/* 0 enables the interrupt */
	reg &= ~(1 << SEI_IRQ_REG_BIT(d->hwirq));
	writel(reg, sei->base + GICP_SEMR(reg_idx));
}

static struct irq_chip mvebu_sei_irq_chip = {
	.name			= "SEI",
	.irq_mask		= mvebu_sei_mask_irq,
	.irq_unmask		= mvebu_sei_unmask_irq,
};

static int mvebu_sei_irq_map(struct irq_domain *domain, unsigned int virq, irq_hw_number_t hwirq)
{
	struct sei_data *sei = domain->host_data;

	irq_set_chip_data(virq, sei);
	irq_set_chip_and_handler(virq, &mvebu_sei_irq_chip, handle_level_irq);

	irq_set_status_flags(virq, IRQ_LEVEL);
	irq_set_probe(virq);

	return 0;
}

static const struct irq_domain_ops mvebu_sei_domain_ops = {
	.map = mvebu_sei_irq_map,
	.xlate = irq_domain_xlate_onecell,
};

static void mvebu_sei_handle_cascade_irq(struct irq_desc *desc)
{
	struct sei_data *sei = irq_desc_get_handler_data(desc);
	struct irq_chip *chip = irq_desc_get_chip(desc);
	unsigned long irq_bit, irqmap;
	unsigned long irqn; /* the absolute irq id /(out of 64 irqs)*/
	unsigned int cascade_irq;
	unsigned int reg_idx;

	chained_irq_enter(chip, desc);

	/* read both SEI cause registers (64 bits) */
	for (reg_idx = 0; reg_idx < SEI_IRQ_REG_CNT; reg_idx++) {
		irqmap = readl_relaxed(sei->base + GICP_SECR(reg_idx));

		/* call handler for each set bit */
		for_each_set_bit(irq_bit, &irqmap, SEI_IRQS_PER_REG_CNT) {
			/* Calculate IRQ according to Cause register ID */
			irqn = irq_bit + reg_idx * SEI_IRQS_PER_REG_CNT;

			cascade_irq = irq_find_mapping(sei->domain, irqn);
			generic_handle_irq(cascade_irq); /* Call IRQ handler */
		}

		/* SEI cause register is Write-1-Clear (i.e. the interrupt
		 * indication is cleared when writing 1 to it) so we
		 * write the same value to clear the interrupt indication
		 */
		writel(irqmap, sei->base + GICP_SECR(reg_idx));
	}

	chained_irq_exit(chip, desc);
}

static struct sei_data *sei_global;

static int __init mvebu_sei_of_init(struct device_node *node, struct device_node *parent)
{
	int ret;
	struct sei_data *sei;

	sei = kzalloc(sizeof(struct sei_data), GFP_KERNEL);
	if (!sei)
		return -ENOMEM;
	sei_global = sei;

	ret = of_address_to_resource(node, 0, &sei->res);
	if (ret) {
		pr_err("Failed to allocate sei resource.\n");
		goto err_free_sei;
	}

	sei->base = ioremap(sei->res.start, resource_size(&sei->res));
	if (!sei->base) {
		pr_err("Failed to map sei resource\n");
		ret = -ENOMEM;
		goto err_free_sei;
	}

	/* set number of IRQs */
	sei->nr_irqs = SEI_IRQS_PER_REG_CNT * SEI_IRQ_REG_CNT;

	sei->domain = irq_domain_add_linear(node, sei->nr_irqs, &mvebu_sei_domain_ops, sei);
	if (!sei->domain) {
		pr_err("Failed to allocate irq domain\n");
		ret = -ENOMEM;
		goto err_iounmap;
	}

	sei->parent_irq = irq_of_parse_and_map(node, 0);
	if (sei->parent_irq <= 0) {
		pr_err("Failed to parse parent interrupt\n");
		ret = -EINVAL;
		goto err_free_domain;
	}

	irq_set_chained_handler(sei->parent_irq, mvebu_sei_handle_cascade_irq);
	irq_set_handler_data(sei->parent_irq, sei);

	mvebu_sei_reset(sei);
	pr_info("registered with %d irqs\n", sei->nr_irqs);

	return 0;

err_free_domain:
	irq_domain_remove(sei->domain);
err_iounmap:
	iounmap(sei->base);
err_free_sei:
	kfree(sei);

	return ret;
}

IRQCHIP_DECLARE(mvebu_sei, "marvell,sei", mvebu_sei_of_init);
