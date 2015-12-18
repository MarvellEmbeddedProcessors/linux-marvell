/*
 * Copyright (C) 2016 Marvell
 *
 * Thomas Petazzoni <thomas.petazzoni@free-electrons.com>
 *
 * This file is licensed under the terms of the GNU General Public
 * License version 2.  This program is licensed "as is" without any
 * warranty of any kind, whether express or implied.
 */

#define pr_fmt(fmt) "GIC-ODMI: " fmt

#include <linux/irq.h>
#include <linux/irqchip.h>
#include <linux/irqdomain.h>
#include <linux/kernel.h>
#include <linux/msi.h>
#include <linux/of_address.h>
#include <linux/slab.h>
#include <dt-bindings/interrupt-controller/arm-gic.h>

#define GICP_ODMIN_SET			0x40
#define   GICP_ODMI_INT_NUM_SHIFT	12
#define GICP_ODMIN_GM_EP_R0		0x110
#define GICP_ODMIN_GM_EP_R1		0x114
#define GICP_ODMIN_GM_EA_R0		0x108
#define GICP_ODMIN_GM_EA_R1		0x118

/*
 * We don't support the group events, so we simply have 8 interrupts
 * per frame.
 */
#define NODMIS_PER_FRAME 8

struct odmi_data {
	struct resource res;
	void __iomem *base;
	unsigned int spi_base;
	unsigned long bm;
};

static struct odmi_data *odmis;
static unsigned int odmis_count;

/*
 * Lock protecting the allocation bitmap embedded in the odmi_data
 * structure. Only one spinlock is used, since allocation/freeing of
 * MSI interrupts is infrequent.
 */
static DEFINE_SPINLOCK(odmis_lock);

static int odmi_set_affinity(struct irq_data *irq_data,
			       const struct cpumask *mask, bool force)
{
	int ret;

	ret = irq_chip_set_affinity_parent(irq_data, mask, force);
	if (ret == IRQ_SET_MASK_OK)
		ret = IRQ_SET_MASK_OK_DONE;

	return ret;
}

static void odmi_compose_msi_msg(struct irq_data *data, struct msi_msg *msg)
{
	struct odmi_data *odmi = NULL;
	phys_addr_t addr;
	unsigned int odmi_offset, i;

	/* Search the ODMI frame handling this interrupt */
	for (i = 0; i < odmis_count; i++) {
		if (data->hwirq >= odmis[i].spi_base &&
		    data->hwirq < (odmis[i].spi_base + NODMIS_PER_FRAME)) {
			odmi = &odmis[i];
			break;
		}
	}

	BUG_ON(!odmi);

	odmi_offset = (data->hwirq - odmi->spi_base);

	addr = odmi->res.start + GICP_ODMIN_SET;

	msg->address_hi = upper_32_bits(addr);
	msg->address_lo = lower_32_bits(addr);
	msg->data = odmi_offset << GICP_ODMI_INT_NUM_SHIFT;
}

static struct irq_chip odmi_irq_chip = {
	.name			= "ODMI",
	.irq_mask		= irq_chip_mask_parent,
	.irq_unmask		= irq_chip_unmask_parent,
	.irq_eoi		= irq_chip_eoi_parent,
	.irq_set_affinity	= odmi_set_affinity,
	.irq_compose_msi_msg	= odmi_compose_msi_msg,
};

static int odmi_irq_domain_alloc(struct irq_domain *domain, unsigned int virq,
				 unsigned int nr_irqs, void *args)
{
	struct odmi_data *odmi = NULL;
	struct irq_fwspec fwspec;
	struct irq_data *d;
	unsigned int offset, hwirq;
	int i, ret;

	spin_lock(&odmis_lock);
	for (i = 0; i < odmis_count; i++) {
		offset = find_first_zero_bit(&odmis[i].bm, NODMIS_PER_FRAME);
		if (offset < NODMIS_PER_FRAME) {
			__set_bit(offset, &odmis[i].bm);
			odmi = &odmis[i];
			break;
		}
	}
	spin_unlock(&odmis_lock);

	if (!odmi)
		return -ENOSPC;

	hwirq = odmi->spi_base + offset;

	fwspec.fwnode = domain->parent->fwnode;
	fwspec.param_count = 3;
	fwspec.param[0] = GIC_SPI;
	fwspec.param[1] = hwirq - 32;
	fwspec.param[2] = IRQ_TYPE_EDGE_RISING;

	ret = irq_domain_alloc_irqs_parent(domain, virq, 1, &fwspec);
	if (ret) {
		pr_err("Cannot allocate parent IRQ\n");
		spin_lock(&odmis_lock);
		__clear_bit(offset, &odmi->bm);
		spin_unlock(&odmis_lock);
		return ret;
	}

	/* Configure the interrupt line to be edge */
	d = irq_domain_get_irq_data(domain->parent, virq);
	d->chip->irq_set_type(d, IRQ_TYPE_EDGE_RISING);

	irq_domain_set_hwirq_and_chip(domain, virq, hwirq,
				      &odmi_irq_chip, NULL);

	return 0;
}

static void odmi_irq_domain_free(struct irq_domain *domain,
				 unsigned int virq, unsigned int nr_irqs)
{
	struct irq_data *d = irq_domain_get_irq_data(domain, virq);
	struct odmi_data *odmi = NULL;
	unsigned int offset;
	int i;

	/*
	 * Check if the hwirq number is valid, and if so, to which
	 * ODMI frame it belongs
	 */
	spin_lock(&odmis_lock);
	for (i = 0; i < odmis_count; i++) {
		if (d->hwirq >= odmis[i].spi_base &&
		    d->hwirq < (odmis[i].spi_base + NODMIS_PER_FRAME)) {
			odmi = &odmis[i];
			offset = d->hwirq - odmis[i].spi_base;
			break;
		}
	}
	spin_unlock(&odmis_lock);

	if (!odmi) {
		pr_err("Failed to teardown msi. Invalid hwirq %lu\n", d->hwirq);
		return;
	}

	irq_domain_free_irqs_parent(domain, virq, nr_irqs);

	/* Actually free the MSI */
	spin_lock(&odmis_lock);
	__clear_bit(offset, &odmi->bm);
	spin_unlock(&odmis_lock);
}

static const struct irq_domain_ops odmi_domain_ops = {
	.alloc	= odmi_irq_domain_alloc,
	.free	= odmi_irq_domain_free,
};

static struct irq_chip odmi_msi_irq_chip = {
	.name	= "ODMI",
};

static struct msi_domain_ops odmi_msi_ops = {
};

static struct msi_domain_info odmi_msi_domain_info = {
	.flags	= (MSI_FLAG_USE_DEF_DOM_OPS | MSI_FLAG_USE_DEF_CHIP_OPS),
	.ops	= &odmi_msi_ops,
	.chip	= &odmi_msi_irq_chip,
};

static int __init mvebu_odmi_init(struct device_node *node,
				  struct device_node *parent)
{
	struct irq_domain *inner_domain, *plat_domain;
	int ret, i;

	if (of_property_read_u32(node, "marvell,odmi-frames", &odmis_count))
		return -EINVAL;

	odmis = kzalloc(odmis_count * sizeof(struct odmi_data), GFP_KERNEL);
	if (!odmis)
		return -ENOMEM;

	for (i = 0; i < odmis_count; i++) {
		struct odmi_data *odmi = &odmis[i];

		ret = of_address_to_resource(node, i, &odmi->res);
		if (ret)
			goto err_unmap;

		odmi->base = of_io_request_and_map(node, i, "odmi");
		if (IS_ERR(odmi->base)) {
			ret = PTR_ERR(odmi->base);
			goto err_unmap;
		}

		if (of_property_read_u32_index(node, "marvell,spi-base",
					       i, &odmi->spi_base)) {
			ret = -EINVAL;
			goto err_unmap;
		}
	}

	inner_domain = irq_domain_create_linear(of_node_to_fwnode(node),
						odmis_count * NODMIS_PER_FRAME,
						&odmi_domain_ops, NULL);
	if (!inner_domain) {
		ret = -ENOMEM;
		goto err_unmap;
	}

	inner_domain->parent = irq_find_host(parent);

	plat_domain = platform_msi_create_irq_domain(of_node_to_fwnode(node),
						     &odmi_msi_domain_info,
						     inner_domain);
	if (!plat_domain) {
		ret = -ENOMEM;
		goto err_remove_inner;
	}

	return 0;

err_remove_inner:
	irq_domain_remove(inner_domain);
err_unmap:
	for (i = 0; i < odmis_count; i++) {
		struct odmi_data *odmi = &odmis[i];
		if (odmi->base && !IS_ERR(odmi->base))
			iounmap(odmis[i].base);
	}
	kfree(odmis);
	return ret;
}

IRQCHIP_DECLARE(mvebu_odmi, "marvell,odmi-controller", mvebu_odmi_init);
