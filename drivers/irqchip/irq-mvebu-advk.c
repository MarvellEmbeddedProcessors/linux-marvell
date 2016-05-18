/*
 * Marvell Armada 3700 PCIe auxiliary IRQ controller
 *
 * Copyright (C) 2016 Marvell
 *
 * Marcin Wojtas <mw@semihalf.com>
 * Hezi Shahmoon <hezi@marvell.com>
 *
 * This file is licensed under the terms of the GNU General Public
 * License version 2.  This program is licensed "as is" without any
 * warranty of any kind, whether express or implied.
 */

#include <linux/clk.h>
#include <linux/init.h>
#include <linux/interrupt.h>
#include <linux/io.h>
#include <linux/irqchip.h>
#include <linux/irq.h>
#include <linux/irqchip/chained_irq.h>
#include <linux/irqdomain.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/msi.h>
#include <linux/of_address.h>
#include <linux/of_irq.h>
#include <linux/of_pci.h>

/* Registers relative to 'core_base' */
#define ADVK_PCIE_CORE_ISR0_STATUS_REG			0x0
#define ADVK_PCIE_CORE_ISR0_MASK_REG			0x4
#define ADVK_PCIE_CORE_ISR1_STATUS_REG			0x8
#define ADVK_PCIE_CORE_ISR1_MASK_REG			0xc
#define ADVK_PCIE_CORE_MSI_ADDR_LOW_REG			0x10
#define ADVK_PCIE_CORE_MSI_ADDR_HIGH_REG		0x14
#define ADVK_PCIE_CORE_MSI_STATUS_REG			0x18
#define ADVK_PCIE_CORE_MSI_MASK_REG			0x1c
#define ADVK_PCIE_CORE_MSI_PAYLOAD_REG			0x5c

/* PCIE_CORE_ISR0 fields and helper macros */
#define     ADVK_PCIE_INTR_FLR_INT			(1 << 26)
#define     ADVK_PCIE_INTR_MSG_LTR			(1 << 25)
#define     ADVK_PCIE_INTR_MSI_INT_PENDING		(1 << 24)
#define     ADVK_PCIE_INTR_INTD_DEASSERT		(1 << 23)
#define     ADVK_PCIE_INTR_INTC_DEASSERT		(1 << 22)
#define     ADVK_PCIE_INTR_INTB_DEASSERT		(1 << 21)
#define     ADVK_PCIE_INTR_INTA_DEASSERT		(1 << 20)
#define     ADVK_PCIE_INTR_INTD_ASSERT			(1 << 19)
#define     ADVK_PCIE_INTR_INTC_ASSERT			(1 << 18)
#define     ADVK_PCIE_INTR_INTB_ASSERT			(1 << 17)
#define     ADVK_PCIE_INTR_INTA_ASSERT			(1 << 16)
#define     ADVK_PCIE_INTR_FAT_ERR			(1 << 13)
#define     ADVK_PCIE_INTR_NFAT_ERR			(1 << 12)
#define     ADVK_PCIE_INTR_CORR_ERR			(1 << 11)
#define     ADVK_PCIE_INTR_LMI_LOCAL_INT		(1 << 10)
#define     ADVK_PCIE_INTR_LEGACY_INT_SENT		(1 << 9)
#define     ADVK_PCIE_INTR_MSG_PM_ACTIVE_STATE_NAK	(1 << 8)
#define     ADVK_PCIE_INTR_MSG_PM_PME			(1 << 7)
#define     ADVK_PCIE_INTR_MSG_PM_TURN_OFF		(1 << 6)
#define     ADVK_PCIE_INTR_MSG_PME_TO_ACK		(1 << 5)
#define     ADVK_PCIE_INTR_INB_DP_FERR_PERR_IRQ		(1 << 4)
#define     ADVK_PCIE_INTR_OUTB_DP_FERR_PERR_IRQ	(1 << 3)
#define     ADVK_PCIE_INTR_INBOUND_MSG			(1 << 2)
#define     ADVK_PCIE_INTR_LINK_DOWN			(1 << 1)
#define     ADVK_PCIE_INTR_HOT_RESET			(1 << 0)

#define ADVK_PCIE_CORE_ISR0_INTX_MASK (		\
	ADVK_PCIE_INTR_INTA_ASSERT |		\
	ADVK_PCIE_INTR_INTB_ASSERT |		\
	ADVK_PCIE_INTR_INTC_ASSERT |		\
	ADVK_PCIE_INTR_INTD_ASSERT)

#define ADVK_PCIE_INTR_INTX_ASSERT(val)			(1 << (15 + (val)))

#define ADVK_PCIE_INTR_ISR0_ALL (		\
	ADVK_PCIE_INTR_FLR_INT |		\
	ADVK_PCIE_INTR_MSG_LTR |		\
	ADVK_PCIE_INTR_MSI_INT_PENDING |	\
	ADVK_PCIE_INTR_INTD_DEASSERT |		\
	ADVK_PCIE_INTR_INTC_DEASSERT |		\
	ADVK_PCIE_INTR_INTB_DEASSERT |		\
	ADVK_PCIE_INTR_INTA_DEASSERT |		\
	ADVK_PCIE_INTR_INTD_ASSERT |		\
	ADVK_PCIE_INTR_INTC_ASSERT |		\
	ADVK_PCIE_INTR_INTB_ASSERT |		\
	ADVK_PCIE_INTR_INTA_ASSERT |		\
	ADVK_PCIE_INTR_FAT_ERR |		\
	ADVK_PCIE_INTR_NFAT_ERR |		\
	ADVK_PCIE_INTR_CORR_ERR |		\
	ADVK_PCIE_INTR_LMI_LOCAL_INT |		\
	ADVK_PCIE_INTR_LEGACY_INT_SENT |	\
	ADVK_PCIE_INTR_MSG_PM_ACTIVE_STATE_NAK |\
	ADVK_PCIE_INTR_MSG_PM_PME |		\
	ADVK_PCIE_INTR_MSG_PM_TURN_OFF |	\
	ADVK_PCIE_INTR_MSG_PME_TO_ACK |		\
	ADVK_PCIE_INTR_INB_DP_FERR_PERR_IRQ |	\
	ADVK_PCIE_INTR_OUTB_DP_FERR_PERR_IRQ |	\
	ADVK_PCIE_INTR_INBOUND_MSG |		\
	ADVK_PCIE_INTR_LINK_DOWN |		\
	ADVK_PCIE_INTR_HOT_RESET)

/* PCIE_CORE_ISR1 fields and helper macros */
#define     ADVK_PCIE_INTR_POWER_STATE_CHANGE		(1 << 4)
#define     ADVK_PCIE_INTR_FLUSH			(1 << 5)

#define ADVK_PCIE_INTR_ISR1_ALL (		\
	ADVK_PCIE_INTR_POWER_STATE_CHANGE |	\
	ADVK_PCIE_INTR_FLUSH)

/* PCIe IRQ registers relative to main_irq_base */
#define ADVK_PCIE_IRQ_REG				0
#define ADVK_PCIE_IRQ_MASK_REG				0x4
#define     ADVK_PCIE_IRQ_CMDQ_INT			(1 << 0)
#define     ADVK_PCIE_IRQ_MSI_STATUS_INT		(1 << 1)
#define     ADVK_PCIE_IRQ_CMD_SENT_DONE			(1 << 3)
#define     ADVK_PCIE_IRQ_DMA_INT			(1 << 4)
#define     ADVK_PCIE_IRQ_IB_DXFERDONE			(1 << 5)
#define     ADVK_PCIE_IRQ_OB_DXFERDONE			(1 << 6)
#define     ADVK_PCIE_IRQ_OB_RXFERDONE			(1 << 7)
#define     ADVK_PCIE_IRQ_COMPQ_INT			(1 << 12)
#define     ADVK_PCIE_IRQ_DIR_RD_DDR_DET		(1 << 13)
#define     ADVK_PCIE_IRQ_DIR_WR_DDR_DET		(1 << 14)
#define     ADVK_PCIE_IRQ_CORE_INT			(1 << 16)
#define     ADVK_PCIE_IRQ_CORE_INT_PIO			(1 << 17)
#define     ADVK_PCIE_IRQ_DPMU_INT			(1 << 18)
#define     ADVK_PCIE_IRQ_PCIE_MIS_INT			(1 << 19)
#define     ADVK_PCIE_IRQ_MSI_INT1_DET			(1 << 20)
#define     ADVK_PCIE_IRQ_MSI_INT2_DET			(1 << 21)
#define     ADVK_PCIE_IRQ_RC_DBELL_DET			(1 << 22)
#define     ADVK_PCIE_IRQ_EP_STATUS			(1 << 23)

/* Global mask*/
#define ADVK_PCIE_IRQ_MASK_ALL (		\
	ADVK_PCIE_IRQ_CMDQ_INT |		\
	ADVK_PCIE_IRQ_MSI_STATUS_INT |		\
	ADVK_PCIE_IRQ_CMD_SENT_DONE |		\
	ADVK_PCIE_IRQ_DMA_INT |			\
	ADVK_PCIE_IRQ_IB_DXFERDONE |		\
	ADVK_PCIE_IRQ_OB_DXFERDONE |		\
	ADVK_PCIE_IRQ_OB_RXFERDONE |		\
	ADVK_PCIE_IRQ_COMPQ_INT |		\
	ADVK_PCIE_IRQ_DIR_RD_DDR_DET |		\
	ADVK_PCIE_IRQ_DIR_WR_DDR_DET |		\
	ADVK_PCIE_IRQ_CORE_INT |		\
	ADVK_PCIE_IRQ_CORE_INT_PIO |		\
	ADVK_PCIE_IRQ_DPMU_INT |		\
	ADVK_PCIE_IRQ_PCIE_MIS_INT |		\
	ADVK_PCIE_IRQ_MSI_INT1_DET |		\
	ADVK_PCIE_IRQ_MSI_INT2_DET |		\
	ADVK_PCIE_IRQ_RC_DBELL_DET |		\
	ADVK_PCIE_IRQ_EP_STATUS)

/* Enabled sources */
#define ADVK_PCIE_IRQ_MASK_ENABLE_INTS		(ADVK_PCIE_IRQ_CORE_INT)

#define ADVK_LEGACY_IRQ_NUM			4
#define ADVK_MSI_IRQ_NUM			32

static void __iomem *core_base;
static void __iomem *main_irq_base;
static struct irq_domain *armada_3700_advk_domain;
static int parent_irq;
#ifdef CONFIG_PCI_MSI
static struct irq_domain *armada_3700_advk_msi_domain;
static DECLARE_BITMAP(msi_irq_in_use, ADVK_MSI_IRQ_NUM);
static DEFINE_MUTEX(msi_used_lock);
static phys_addr_t msi_msg_base;

static int armada_3700_advk_alloc_msi(void)
{
	int hwirq;

	mutex_lock(&msi_used_lock);
	hwirq = find_first_zero_bit(msi_irq_in_use, ADVK_MSI_IRQ_NUM);
	if (hwirq >= ADVK_MSI_IRQ_NUM)
		hwirq = -ENOSPC;
	else
		set_bit(hwirq, msi_irq_in_use);
	mutex_unlock(&msi_used_lock);

	return hwirq;
}

static void armada_3700_advk_free_msi(int hwirq)
{
	mutex_lock(&msi_used_lock);
	if (!test_bit(hwirq, msi_irq_in_use))
		pr_err("trying to free unused MSI#%d\n", hwirq);
	else
		clear_bit(hwirq, msi_irq_in_use);
	mutex_unlock(&msi_used_lock);
}

static int armada_3700_advk_setup_msi_irq(struct msi_controller *chip,
					  struct pci_dev *pdev,
					  struct msi_desc *desc)
{
	struct msi_msg msg;
	int virq, hwirq;

	/* We support MSI, but not MSI-X */
	if (desc->msi_attrib.is_msix)
		return -EINVAL;

	hwirq = armada_3700_advk_alloc_msi();
	if (hwirq < 0)
		return hwirq;

	virq = irq_create_mapping(armada_3700_advk_msi_domain, hwirq);
	if (!virq) {
		armada_3700_advk_free_msi(hwirq);
		return -EINVAL;
	}

	irq_set_msi_desc(virq, desc);

	msg.address_lo = lower_32_bits(msi_msg_base);
	msg.address_hi = upper_32_bits(msi_msg_base);
	msg.data = virq;

	pci_write_msi_msg(virq, &msg);

	return 0;
}

static void armada_3700_advk_teardown_msi_irq(struct msi_controller *chip,
					   unsigned int irq)
{
	struct irq_data *d = irq_get_irq_data(irq);
	unsigned long hwirq = d->hwirq;

	irq_dispose_mapping(irq);
	armada_3700_advk_free_msi(hwirq);
}

static struct irq_chip armada_3700_advk_msi_irq_chip = {
	.name = "advk_msi",
	.irq_enable = pci_msi_unmask_irq,
	.irq_disable = pci_msi_mask_irq,
	.irq_mask = pci_msi_mask_irq,
	.irq_unmask = pci_msi_unmask_irq,
};

static int armada_3700_advk_msi_map(struct irq_domain *domain,
				    unsigned int virq, irq_hw_number_t hw)
{
	irq_set_chip_and_handler(virq, &armada_3700_advk_msi_irq_chip,
				 handle_simple_irq);

	return 0;
}

static const struct irq_domain_ops armada_3700_advk_msi_irq_ops = {
	.map = armada_3700_advk_msi_map,
};

static int armada_3700_advk_msi_init(struct device_node *node)
{
	struct msi_controller *msi_chip;
	void *msi_msg_base_virt;
	int ret;

	msi_chip = kzalloc(sizeof(*msi_chip), GFP_KERNEL);
	if (!msi_chip)
		return -ENOMEM;

	msi_chip->setup_irq = armada_3700_advk_setup_msi_irq;
	msi_chip->teardown_irq = armada_3700_advk_teardown_msi_irq;
	msi_chip->of_node = node;

	msi_msg_base_virt = kzalloc(sizeof(u16), GFP_KERNEL);
	if (!msi_msg_base_virt) {
		ret = -ENOMEM;
		goto err_base;
	}

	msi_msg_base = virt_to_phys(msi_msg_base_virt);

	writel(lower_32_bits(msi_msg_base),
	       core_base + ADVK_PCIE_CORE_MSI_ADDR_LOW_REG);
	writel(upper_32_bits(msi_msg_base),
	       core_base + ADVK_PCIE_CORE_MSI_ADDR_HIGH_REG);

	armada_3700_advk_msi_domain =
		irq_domain_add_linear(NULL, ADVK_MSI_IRQ_NUM,
				      &armada_3700_advk_msi_irq_ops,
				      NULL);
	if (!armada_3700_advk_msi_domain) {
		ret = -ENOMEM;
		goto err_domain;
	}

	ret = of_pci_msi_chip_add(msi_chip);
	if (ret < 0)
		goto err_chip_add;

	return 0;

err_chip_add:
	irq_domain_remove(armada_3700_advk_msi_domain);
err_domain:
	kfree(msi_msg_base_virt);
err_base:
	kfree(msi_chip);

	return ret;
}
#else
static inline int armada_3700_advk_msi_init(struct device_node *node)
{
	return 0;
}
#endif

static void armada_3700_advk_irq_mask(struct irq_data *d)
{
	irq_hw_number_t hwirq = irqd_to_hwirq(d);
	u32 mask;

	mask = readl(core_base + ADVK_PCIE_CORE_ISR0_MASK_REG);
	mask |= ADVK_PCIE_INTR_INTX_ASSERT(hwirq);
	writel(mask, core_base + ADVK_PCIE_CORE_ISR0_MASK_REG);
}

static void armada_3700_advk_irq_unmask(struct irq_data *d)
{
	irq_hw_number_t hwirq = irqd_to_hwirq(d);
	u32 mask;

	mask = readl(core_base + ADVK_PCIE_CORE_ISR0_MASK_REG);
	mask = ~(ADVK_PCIE_INTR_INTX_ASSERT(hwirq));
	writel(mask, core_base + ADVK_PCIE_CORE_ISR0_MASK_REG);
}

static struct irq_chip armada_3700_advk_irq_chip = {
	.name		= "advk_pcie",
	.irq_mask       = armada_3700_advk_irq_mask,
	.irq_mask_ack   = armada_3700_advk_irq_mask,
	.irq_unmask     = armada_3700_advk_irq_unmask,
};

static int armada_3700_advk_irq_map(struct irq_domain *h,
				    unsigned int virq, irq_hw_number_t hwirq)
{
	armada_3700_advk_irq_mask(irq_get_irq_data(virq));
	irq_set_status_flags(virq, IRQ_LEVEL);
	irq_set_chip_and_handler(virq, &armada_3700_advk_irq_chip,
				 handle_level_irq);

	return 0;
}

static const struct irq_domain_ops armada_3700_advk_irq_ops = {
	.map = armada_3700_advk_irq_map,
	.xlate = irq_domain_xlate_onecell,
};

static void armada_3700_advk_msi_handler(void)
{
	u32 msi_val, msi_mask, msi_status, msi_idx;
	u16 msi_data;

	msi_mask = readl(core_base + ADVK_PCIE_CORE_MSI_MASK_REG);
	msi_val = readl(core_base + ADVK_PCIE_CORE_MSI_STATUS_REG);
	msi_status = msi_val & ~msi_mask;

	for (msi_idx = 0; msi_idx < ADVK_MSI_IRQ_NUM; msi_idx++) {
		if (!(BIT(msi_idx) & msi_status))
			continue;

		writel(BIT(msi_idx),
		       core_base + ADVK_PCIE_CORE_MSI_STATUS_REG);

		msi_data = readl(core_base +
				 ADVK_PCIE_CORE_MSI_PAYLOAD_REG) & 0xFF;
		generic_handle_irq(msi_data);
	}

	writel(ADVK_PCIE_INTR_MSI_INT_PENDING,
	       core_base + ADVK_PCIE_CORE_ISR0_STATUS_REG);
}

static void armada_3700_advk_isr0_handler(void)
{
	u32 val, mask, status;
	int i;

	val = readl(core_base + ADVK_PCIE_CORE_ISR0_STATUS_REG);
	mask = readl(core_base + ADVK_PCIE_CORE_ISR0_MASK_REG);
	status = val & ((~mask) & ADVK_PCIE_INTR_ISR0_ALL);

	if (!status) {
		writel(val, core_base + ADVK_PCIE_CORE_ISR0_STATUS_REG);
		return;
	}

	/* Process MSI interrupts */
	if (status & ADVK_PCIE_INTR_MSI_INT_PENDING)
		armada_3700_advk_msi_handler();

	/* Process legacy interrupts */
	for (i = 1; i <= ADVK_LEGACY_IRQ_NUM; i++) {
		if (!(status & ADVK_PCIE_INTR_INTX_ASSERT(i)))
			continue;

		writel(ADVK_PCIE_INTR_INTX_ASSERT(i),
		       core_base + ADVK_PCIE_CORE_ISR0_STATUS_REG);
		generic_handle_irq(irq_find_mapping(armada_3700_advk_domain, i));
	}
}

static void armada_3700_advk_handle_cascade_irq(struct irq_desc *desc)
{
	struct irq_chip *chip = irq_desc_get_chip(desc);
	u32 status;

	chained_irq_enter(chip, desc);

	status = readl(main_irq_base + ADVK_PCIE_IRQ_REG);

	if (status & ADVK_PCIE_IRQ_CORE_INT) {
		armada_3700_advk_isr0_handler();
		writel(ADVK_PCIE_IRQ_CORE_INT,
		       main_irq_base + ADVK_PCIE_IRQ_REG);
	}

	chained_irq_exit(chip, desc);
}

static void armada_3700_advk_init_hw(void)
{
	u32 mask;

	/* Clear all interrupts. */
	writel(ADVK_PCIE_INTR_ISR0_ALL,
	       core_base + ADVK_PCIE_CORE_ISR0_STATUS_REG);
	writel(ADVK_PCIE_INTR_ISR1_ALL,
	       core_base + ADVK_PCIE_CORE_ISR1_STATUS_REG);
	writel(ADVK_PCIE_IRQ_MASK_ALL,
	       main_irq_base + ADVK_PCIE_IRQ_REG);

	/* Disable All ISR0/1 Sources */
	mask = ADVK_PCIE_INTR_ISR0_ALL;
#ifdef CONFIG_PCI_MSI
	mask &= ~ADVK_PCIE_INTR_MSI_INT_PENDING;
#endif
	writel(mask, core_base + ADVK_PCIE_CORE_ISR0_MASK_REG);

	mask = ADVK_PCIE_INTR_ISR1_ALL;
	writel(mask, core_base + ADVK_PCIE_CORE_ISR1_MASK_REG);

#ifdef CONFIG_PCI_MSI
	/* Unmask all MSI's */
	writel(0, core_base + ADVK_PCIE_CORE_MSI_MASK_REG);
#endif

	/* Enable summary interrupt for GIC SPI source */
	mask = ADVK_PCIE_IRQ_MASK_ALL & (~ADVK_PCIE_IRQ_MASK_ENABLE_INTS);
	writel(mask, main_irq_base + ADVK_PCIE_IRQ_MASK_REG);
}

static int __init armada_3700_advk_of_init(struct device_node *node,
					     struct device_node *parent)
{
	int ret;

	core_base = of_iomap(node, 0);
	if (IS_ERR(core_base))
		return PTR_ERR(core_base);

	main_irq_base = of_iomap(node, 1);
	if (IS_ERR(main_irq_base)) {
		ret = PTR_ERR(main_irq_base);
		goto err_main;
	}

	armada_3700_advk_domain =
		irq_domain_add_linear(node, ADVK_LEGACY_IRQ_NUM,
				      &armada_3700_advk_irq_ops, NULL);
	if (!armada_3700_advk_domain) {
		ret = -ENOMEM;
		goto err_domain;
	}

	armada_3700_advk_msi_init(node);

	armada_3700_advk_init_hw();

	parent_irq = irq_of_parse_and_map(node, 0);
	irq_set_chained_handler(parent_irq,
				armada_3700_advk_handle_cascade_irq);

	return 0;

err_domain:
	iounmap(main_irq_base);
err_main:
	iounmap(core_base);

	return ret;
}

IRQCHIP_DECLARE(armada_3700_advk, "marvell,advk-ic", armada_3700_advk_of_init);
