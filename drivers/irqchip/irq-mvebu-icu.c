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

#define pr_fmt(fmt) "mvebu-icu: " fmt

#include <linux/irq.h>
#include <linux/interrupt.h>
#include <linux/irqchip/chained_irq.h>
#include <linux/irqdomain.h>
#include <linux/kernel.h>
#include <linux/of_address.h>
#include <linux/of_irq.h>
#include <linux/irqchip.h>

#include <dt-bindings/interrupt-controller/mvebu-icu.h>

#define ICU_MAX_IRQS		208
#define ICU_MAX_SPI_IRQ_IN_GIC	128
#define ICU_GIC_SPI_BASE0	64
#define ICU_GIC_SPI_BASE1	288

#define ICU_INT_ENABLE_OFFSET   (24)
#define ICU_IS_EDGE_OFFSET      (28)
#define ICU_GROUP_OFFSET        (29)

#define ICU_SETSPI_NSR_AL	(0x10)
#define ICU_SETSPI_NSR_AH	(0x14)
#define ICU_CLRSPI_NSR_AL	(0x18)
#define ICU_CLRSPI_NSR_AH	(0x1C)

#define ICU_INT_CFG(x)          (0x100 + 4 * x)

#define ICU_GET_GIC_BASE_BY_IDX(x)	((x < ICU_GIC_SPI_BASE0) ? \
					(ICU_GIC_SPI_BASE0) : (ICU_GIC_SPI_BASE1 - ICU_GIC_SPI_BASE0))
#define ICU_GET_IDX_BY_GIC_BASE(x)	((x < ICU_GIC_SPI_BASE1) ? \
					(x - ICU_GIC_SPI_BASE0) : (x - ICU_GIC_SPI_BASE1))

#define ICU_GET_GIC_IRQ(x)	(x + ((ICU_GET_GIC_BASE_BY_IDX(x)) - 32))

#define ICU_GET_GIC_IDX(x)	(ICU_GET_IDX_BY_GIC_BASE(x))

#define ICU_SATA0_IRQ_INT		109
#define ICU_SATA1_IRQ_INT		107

struct mvebu_icu_irq_data {
	void __iomem *base;	/* ICU register base */
	void __iomem *gicp_clr_spi_base;
	struct irq_domain *domain;
};

static DEFINE_SPINLOCK(icu_lock);
static DECLARE_BITMAP(icu_irq_alloc, ICU_MAX_SPI_IRQ_IN_GIC);

static void mvebu_icu_irq_chip_eoi(struct irq_data *data)
{
	struct mvebu_icu_irq_data *icu = data->domain->host_data;
	struct irq_data *irq_parent = data->parent_data;
	int irq_msg_num = ICU_GET_GIC_IDX(irqd_to_hwirq(irq_parent));

	if (!irqd_is_level_type(data)) {
		/*
		 * Workaround for edge interrupts support by GICP:
		 * Since GICP supports only level interrupts and don't clear
		 * edge interrupts, we need to clear interrupt by ourselves.
		 * Clear the interrupt only for interrupts configured as Edge.
		 */
		writel(irq_msg_num, icu->gicp_clr_spi_base);
	}
	/* Invoke the standard EOI on the parent interrupt function */
	irq_chip_eoi_parent(data);
}

static struct irq_chip mvebu_icu_irq_chip = {
	.name			= "ICU",
	.irq_mask		= irq_chip_mask_parent,
	.irq_unmask		= irq_chip_unmask_parent,
	.irq_eoi		= mvebu_icu_irq_chip_eoi,
	.irq_set_type           = irq_chip_set_type_parent,
#ifdef CONFIG_SMP
	.irq_set_affinity       = irq_chip_set_affinity_parent,
#endif
};

static int mvebu_icu_irq_parent_domain_alloc(struct irq_domain *domain,
		unsigned int virq, unsigned int type, int *irq_msg_num)
{
	struct irq_fwspec fwspec;

	if (!irq_domain_get_of_node(domain->parent)) {
		pr_err("No parent node offset found\n");
		return -EINVAL;
	}

	/* Find first free interrupt in ICU pool */
	spin_lock(&icu_lock);
	*irq_msg_num = find_first_zero_bit(icu_irq_alloc, ICU_MAX_SPI_IRQ_IN_GIC);
	if (*irq_msg_num == ICU_MAX_SPI_IRQ_IN_GIC) {
		pr_err("No free ICU interrupt found\n");
		spin_unlock(&icu_lock);
		return -EINVAL;
	}
	set_bit(*irq_msg_num, icu_irq_alloc);
	spin_unlock(&icu_lock);

	/* Prepare allocation data to send to parent
	 * param0: IRQ type (SPI/LPI)
	 * param1: IRQ number
	 * param2: IRQ type (EDGE/LEVEL_LOW/HIGH)
	 */
	fwspec.fwnode = domain->parent->fwnode;
	fwspec.param_count = 3;
	fwspec.param[0] = 0; /* 0 = SPI interrupts */
	fwspec.param[1] = ICU_GET_GIC_IRQ(*irq_msg_num);
	fwspec.param[2] = type;

	/* Allocate the IRQ in the parent */
	return irq_domain_alloc_irqs_parent(domain, virq, 1, &fwspec);
}

static int mvebu_icu_irq_domain_translate(struct irq_domain *d,
		struct irq_fwspec *fwspec,
		unsigned long *hwirq,
		unsigned int *type)
{
	unsigned int icu_group;

	/* Check the count of the parameters in dt */
	if (WARN_ON(fwspec->param_count < 3)) {
		pr_err("ICU: wrong ICU parameter count %d\n", fwspec->param_count);
		return -EINVAL;
	}

	/* Only ICU group type is handled */
	icu_group = fwspec->param[0];
	if (icu_group != ICU_GRP_NSR && icu_group != ICU_GRP_SR &&
		icu_group != ICU_GRP_SEI && icu_group != ICU_GRP_REI) {
		pr_err("ICU: wrong ICU type %x\n", icu_group);
		return -EINVAL;
	}

	*hwirq = fwspec->param[1];
	if (*hwirq < 0) {
		pr_err("ICU: invalid interrupt number %ld\n", *hwirq);
		return -EINVAL;
	}

	/* Mask the type to prevent wrong DT configuration */
	*type = fwspec->param[2] & IRQ_TYPE_SENSE_MASK;

	return 0;
}

static int mvebu_icu_irq_domain_alloc(struct irq_domain *domain, unsigned int virq,
				   unsigned int nr_irqs, void *args)
{
	int err = 0, irq_msg_num = 0;
	unsigned long hwirq;
	unsigned int type = 0;
	unsigned int icu_group, icu_int;
	struct irq_fwspec *fwspec = args;
	struct mvebu_icu_irq_data *icu = domain->host_data;
	struct irq_data *irq, *irq_parent;

	err = mvebu_icu_irq_domain_translate(domain, fwspec, &hwirq, &type);
	if (err) {
		pr_err("ICU: failed to translate ICU parameters\n");
		return err;
	}
	icu_group = fwspec->param[0];

	err = mvebu_icu_irq_parent_domain_alloc(domain, virq, type, &irq_msg_num);
	if (err) {
		pr_err("ICU: failed to allocate ICU interrupt in parent domain\n");
		return err;
	}

	/*
	 * Clear Non-Secure SPI in GICP,
	 * in case it was asserted in bootloader.
	 */
	if (icu_group == ICU_GRP_NSR)
		writel(irq_msg_num, icu->gicp_clr_spi_base);

	/* Configure the ICU with irq number & type */
	icu_int  = (irq_msg_num) | (1 << ICU_INT_ENABLE_OFFSET);
	if (type & IRQ_TYPE_EDGE_RISING)
		icu_int |= 1 << ICU_IS_EDGE_OFFSET;
	else
		icu_int |= 0 << ICU_IS_EDGE_OFFSET;
	icu_int |= icu_group << ICU_GROUP_OFFSET;
	writel(icu_int, icu->base + ICU_INT_CFG(hwirq));

	/* The SATA unit has 2 ports, and a dedicated ICU entry per port.
	** The ahci sata driver supports only one irq interrupt per SATA unit.
	** to solve this conflict, we configure the 2 SATA wired interrupts in the
	** south bridge into 1 GIC interrupt in the north bridge.
	** Even if only a single port is enabled, if sata node is enabled, both
	** interrupts are configured. (regardless of which port is actually in use)
	** The ICU index of SATA0 = 107, SATA1 = 109
	*/
	if (hwirq == ICU_SATA0_IRQ_INT || hwirq == ICU_SATA1_IRQ_INT) {
		writel(icu_int, icu->base + ICU_INT_CFG(ICU_SATA0_IRQ_INT));
		writel(icu_int, icu->base + ICU_INT_CFG(ICU_SATA1_IRQ_INT));
	}

	err = irq_domain_set_hwirq_and_chip(domain, virq, hwirq, &mvebu_icu_irq_chip, icu);
	if (err) {
		pr_err("ICU: failed to set the data to IRQ domain\n");
		return err;
	}

	irq = irq_get_irq_data(virq);
	irq_parent = irq->parent_data;

	pr_debug("ICU interrupt %d mapped parent interrupt %d\n",
			(int)irqd_to_hwirq(irq), (int)irqd_to_hwirq(irq_parent));

	return 0;
}

static void mvebu_icu_irq_domain_free(struct irq_domain *domain,
				   unsigned int virq, unsigned int nr_irqs)
{
	struct mvebu_icu_irq_data *icu = domain->host_data;
	struct irq_data *irq = irq_get_irq_data(virq);
	struct irq_data *irq_parent = irq->parent_data;
	int irq_msg_num = ICU_GET_GIC_IDX(irqd_to_hwirq(irq_parent));

	WARN_ON(nr_irqs != 1);

	spin_lock(&icu_lock);
	/* Clear the allocated bit of the interrupt */
	clear_bit(irq_msg_num, icu_irq_alloc);
	spin_unlock(&icu_lock);
	writel(0, icu->base + ICU_INT_CFG(irqd_to_hwirq(irq)));

	irq_domain_free_irqs_parent(domain, virq, nr_irqs);
}

static const struct irq_domain_ops mvebu_icu_domain_ops = {
	.translate		= mvebu_icu_irq_domain_translate,
	.alloc			= mvebu_icu_irq_domain_alloc,
	.free			= mvebu_icu_irq_domain_free,
};

static int __init mvebu_icu_of_init(struct device_node *node, struct device_node *parent)
{
	int ret;
	resource_size_t gicp_clr_spi_base;
	struct mvebu_icu_irq_data *icu;
	struct irq_domain *parent_domain;
	u32 gicp_spi_reg[4];
	u32 i, icu_int;

	icu = kzalloc(sizeof(struct mvebu_icu_irq_data), GFP_KERNEL);
	if (!icu)
		return -ENOMEM;

	icu->base = of_iomap(node, 0);
	if (!icu->base) {
		pr_err("Failed to map icu base address.\n");
		ret = -ENOMEM;
		goto err_free_icu;
	}

	/* Get the addresses of clear/set GICP SPI messages
	** on the Host side (AP)
	**/
	ret = of_property_read_u32_array(node, "gicp-spi", gicp_spi_reg, ARRAY_SIZE(gicp_spi_reg));
	if (ret) {
		pr_err("Failed to get GICP SPI addresses from DT\n");
		goto err_free_icu;
	}

	parent_domain = irq_find_matching_host(parent, DOMAIN_BUS_ANY);
	if (!parent_domain) {
		pr_err("Unable to locate ICU parent domain - %s\n", parent->full_name);
		goto err_iounmap;
	}

	icu->domain = irq_domain_add_hierarchy(parent_domain, 0, ICU_MAX_SPI_IRQ_IN_GIC,
			node, &mvebu_icu_domain_ops, icu);
	if (!icu->domain) {
		pr_err("Failed to create ICU domain\n");
		goto err_iounmap;
	}

	icu->domain->parent = parent_domain;

	/* Set Clear/Set ICU SPI message address in AP */
	writel(gicp_spi_reg[0], icu->base + ICU_SETSPI_NSR_AH);
	writel(gicp_spi_reg[1], icu->base + ICU_SETSPI_NSR_AL);
	writel(gicp_spi_reg[2], icu->base + ICU_CLRSPI_NSR_AH);
	writel(gicp_spi_reg[3], icu->base + ICU_CLRSPI_NSR_AL);

	gicp_clr_spi_base = (u64)gicp_spi_reg[3];

	icu->gicp_clr_spi_base = ioremap(gicp_clr_spi_base, 0x4);
	if (!icu->gicp_clr_spi_base) {
		pr_err("Fail to map GICP SPI_CLR register\n");
		ret = -ENOMEM;
		goto err_iounmap;
	}

	/* Clean all ICU interrupts with type SPI_NSR, required to avoid
	** unpredictable SPI assignments done by firmware
	**/
	for (i = 0 ; i < ICU_MAX_IRQS ; i++) {
		icu_int = readl(icu->base + ICU_INT_CFG(i));
		if ((icu_int >> ICU_GROUP_OFFSET) == ICU_GRP_NSR)
			writel(0x0, icu->base + ICU_INT_CFG(i));
	}

	pr_debug("ICU irq chip init successfully\n");

	return 0;

err_iounmap:
	iounmap(icu->base);
err_free_icu:
	kfree(icu);

	return ret;
}

IRQCHIP_DECLARE(mvebu_icu, "marvell,icu", mvebu_icu_of_init);
