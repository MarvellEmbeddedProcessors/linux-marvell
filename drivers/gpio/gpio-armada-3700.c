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

#include <linux/clk.h>
#include <linux/err.h>
#include <linux/gpio.h>
#include <linux/io.h>
#include <linux/irq.h>
#include <linux/irqchip/chained_irq.h>
#include <linux/irqdomain.h>
#include <linux/module.h>
#include <linux/of_irq.h>
#include <linux/of_device.h>
#include <linux/pinctrl/consumer.h>
#include <linux/slab.h>

/* GPIO unit register offsets for A3700. */
#define GPIO_OUT_OFF		0x0018
#define GPIO_IO_CONF_OFF	0x0000
#define GPIO_DATA_IN_OFF	0x0010

/* GPIO interrupt reg block, starts from 13c00/18c00 */
#define GPIO_EDGE_MASK_OFF	0x0000
#define GPIO_INT_POL_OFF	0x0008
#define GPIO_EDGE_CAUSE_OFF	0x0010

#define MVEBU_MAX_GPIO_PER_BANK		36

#define MVEBU_GPIO_CHIP_TYPE_NUM	1

/* IRQ mask type is u32 and has 32 bits, generic irq chip supports max 32 interrupts. */
#define MVEBU_IRQ_MASK_BIT_NUM_MAX	32
/* When a gpio bank has more than 32 gpios(e.g. a3700), we need more than 1 generic irq chip. */
/* For a mvebu gpio bank, the max gpio number is not more than 64, so we need at most 2 generic irq chip for a bank. */
#define MVEBU_IRQ_CHIP_NUM	2

struct mvebu_gpio_chip {
	struct gpio_chip   chip;
	spinlock_t	   lock;
	void __iomem	  *membase;
	void __iomem	  *interrupt_membase;
	int		   irqbase;
	struct irq_domain *domain;

	/* Used to preserve GPIO registers across suspend/resume. */
	u32                out_reg;
	u32                out_reg_hi;
	u32                io_conf_reg;
	u32                io_conf_reg_hi;
	u32                irq_pol_reg;
	u32                irq_pol_reg_hi;
	u32                edge_mask_regs[4];
};

/*
 * Functions returning addresses of individual registers for a given
 * A3700 GPIO controller.
 *
 * There are up to 36 GPIO pin in A3700, so there are two continuous
 * registers to configure a single feature. for example, to enable
 * GPIO output, for pin 0 - 31, it is in register GPIO_OUT_OFF,
 * for pin 32 - 35, it is in register GPIO_OUT_OFF + 4.
 */
#define GPIO_IO_BITWIDTH	32
#define GPIO_REG_OFF(PIN)	((PIN / GPIO_IO_BITWIDTH) * sizeof(u32))
#define GPIO_REG_PIN_OFF(PIN)	(PIN % GPIO_IO_BITWIDTH)

static inline void __iomem *mvebu_gpioreg_out(struct mvebu_gpio_chip *mvchip, u32 pin)
{
	return mvchip->membase + GPIO_OUT_OFF + GPIO_REG_OFF(pin);
}

static inline void __iomem *mvebu_gpioreg_io_conf(struct mvebu_gpio_chip *mvchip, u32 pin)
{
	return mvchip->membase + GPIO_IO_CONF_OFF + GPIO_REG_OFF(pin);
}

static inline void __iomem *mvebu_gpioreg_data_in(struct mvebu_gpio_chip *mvchip, u32 pin)
{
	return mvchip->membase + GPIO_DATA_IN_OFF + GPIO_REG_OFF(pin);
}

static inline void __iomem *mvebu_gpioreg_int_pol(struct mvebu_gpio_chip *mvchip, u32 pin)
{
	return mvchip->interrupt_membase + GPIO_INT_POL_OFF + GPIO_REG_OFF(pin);
}

static inline void __iomem *mvebu_gpioreg_edge_cause(struct mvebu_gpio_chip *mvchip, u32 pin)
{

	return mvchip->interrupt_membase + GPIO_EDGE_CAUSE_OFF + GPIO_REG_OFF(pin);
}

static inline void __iomem *mvebu_gpioreg_edge_mask(struct mvebu_gpio_chip *mvchip, u32 pin)
{
	return mvchip->interrupt_membase + GPIO_EDGE_MASK_OFF + GPIO_REG_OFF(pin);
}

/* Functions implementing the gpio_chip methods for A3700 */
static void mvebu_gpio_set(struct gpio_chip *chip, unsigned pin, int value)
{
	struct mvebu_gpio_chip *mvchip = container_of(chip, struct mvebu_gpio_chip, chip);
	unsigned long flags;
	void __iomem *out_reg_addr;
	u32 u;

	out_reg_addr = mvebu_gpioreg_out(mvchip, pin);
	pin = GPIO_REG_PIN_OFF(pin);

	spin_lock_irqsave(&mvchip->lock, flags);
	u = readl_relaxed(out_reg_addr);
	if (value)
		u |= 1 << pin;
	else
		u &= ~(1 << pin);
	writel_relaxed(u, out_reg_addr);
	spin_unlock_irqrestore(&mvchip->lock, flags);
}

static int mvebu_gpio_get(struct gpio_chip *chip, unsigned pin)
{
	struct mvebu_gpio_chip *mvchip = container_of(chip, struct mvebu_gpio_chip, chip);
	u32 u;

	/*
	 * Return value depends on PIN direction:
	 * - output, return output value.
	 * - input, return input value.
	 */
	if (readl_relaxed(mvebu_gpioreg_io_conf(mvchip, pin)) & (1 << GPIO_REG_PIN_OFF(pin)))
		u = readl_relaxed(mvebu_gpioreg_out(mvchip, pin));
	else
		u = readl_relaxed(mvebu_gpioreg_data_in(mvchip, pin));

	pin = GPIO_REG_PIN_OFF(pin);
	return (u >> pin) & 1;
}

static int mvebu_gpio_direction_input(struct gpio_chip *chip, unsigned pin)
{
	struct mvebu_gpio_chip *mvchip = container_of(chip, struct mvebu_gpio_chip, chip);
	unsigned long flags;
	void __iomem *io_reg_addr;
	int ret;
	u32 u;

	/* Check with the pinctrl driver whether this pin is usable as an input GPIO. */
	ret = pinctrl_gpio_direction_input(chip->base + pin);
	if (ret) {
		dev_err(chip->cdev, "gpio_direction_input, pin (%d) is not a GPIO pin\n", pin);
		return ret;
	}

	io_reg_addr = mvebu_gpioreg_io_conf(mvchip, pin);
	pin = GPIO_REG_PIN_OFF(pin);

	spin_lock_irqsave(&mvchip->lock, flags);
	u = readl_relaxed(io_reg_addr);
	u &= ~(1 << pin);
	writel_relaxed(u, io_reg_addr);
	spin_unlock_irqrestore(&mvchip->lock, flags);

	return 0;
}

static int mvebu_gpio_direction_output(struct gpio_chip *chip, unsigned pin, int value)
{
	struct mvebu_gpio_chip *mvchip = container_of(chip, struct mvebu_gpio_chip, chip);
	unsigned long flags;
	void __iomem *io_reg_addr;
	int ret;
	u32 u;

	/* Check with the pinctrl driver whether this pin is usable as an output GPIO. */
	ret = pinctrl_gpio_direction_output(chip->base + pin);
	if (ret) {
		dev_err(chip->cdev, "gpio_direction_output, pin (%d) is not a GPIO pin\n", pin);
		return ret;
	}

	/* Set output value first. */
	mvebu_gpio_set(chip, pin, value);

	io_reg_addr = mvebu_gpioreg_io_conf(mvchip, pin);
	pin = GPIO_REG_PIN_OFF(pin);

	spin_lock_irqsave(&mvchip->lock, flags);
	u = readl_relaxed(io_reg_addr);
	u |= 1 << pin;
	writel_relaxed(u, io_reg_addr);
	spin_unlock_irqrestore(&mvchip->lock, flags);

	return 0;
}

static int mvebu_gpio_to_irq(struct gpio_chip *chip, unsigned pin)
{
	struct mvebu_gpio_chip *mvchip = container_of(chip, struct mvebu_gpio_chip, chip);

	return irq_create_mapping(mvchip->domain, pin);
}

static void mvebu_gpio_irq_ack(struct irq_data *d)
{
	struct irq_chip_generic *gc = irq_data_get_irq_chip_data(d);
	struct mvebu_gpio_chip *mvchip = gc->private;
	u32 pin = d->irq - mvchip->irqbase;
	u32 mask = 1 << GPIO_REG_PIN_OFF(pin);

	irq_gc_lock(gc);
	writel_relaxed(mask, mvebu_gpioreg_edge_cause(mvchip, pin));
	irq_gc_unlock(gc);
}

static void mvebu_gpio_edge_irq_mask(struct irq_data *d)
{
	struct irq_chip_generic *gc = irq_data_get_irq_chip_data(d);
	struct mvebu_gpio_chip *mvchip = gc->private;
	struct irq_chip_type *ct = irq_data_get_chip_type(d);
	u32 pin = d->irq - mvchip->irqbase;
	u32 mask = 1 << GPIO_REG_PIN_OFF(pin);

	irq_gc_lock(gc);
	ct->mask_cache_priv &= ~mask;
	writel_relaxed(ct->mask_cache_priv, mvebu_gpioreg_edge_mask(mvchip, pin));
	irq_gc_unlock(gc);
}

static void mvebu_gpio_edge_irq_unmask(struct irq_data *d)
{
	struct irq_chip_generic *gc = irq_data_get_irq_chip_data(d);
	struct mvebu_gpio_chip *mvchip = gc->private;
	struct irq_chip_type *ct = irq_data_get_chip_type(d);
	u32 pin = d->irq - mvchip->irqbase;
	u32 mask = 1 << GPIO_REG_PIN_OFF(pin);

	irq_gc_lock(gc);
	ct->mask_cache_priv |= mask;
	writel_relaxed(ct->mask_cache_priv, mvebu_gpioreg_edge_mask(mvchip, pin));
	irq_gc_unlock(gc);
}

/*****************************************************************************
 * This routine sets the flow type (IRQ_TYPE_LEVEL/etc.) of an IRQ for a3700
 *
 * GPIO_INT_POL register controls the interrupt polarity.
 *
 * Edge IRQ handlers:  Change in DATA_IN are latched in EDGE_CAUSE.
 *		       Interrupt are masked by EDGE_MASK registers.
 * Both-edge handlers: Similar to regular Edge handlers, but also swaps
 *		       the polarity to catch the next line transaction.
 *		       This is a race condition that might not perfectly
 *		       work on some use cases.
 *
 * Every eight GPIO lines are grouped (OR'ed) before going up to main
 * cause register.
 *
 *	  data-in     EDGE  cause    mask
 *     -----| |----------| |-----| |----- to main cause reg
 *	     X
 *	  polarity
 *
 ****************************************************************************/
static int mvebu_gpio_irq_set_type(struct irq_data *d, unsigned int type)
{
	struct irq_chip_generic *gc = irq_data_get_irq_chip_data(d);
	struct irq_chip_type *ct = irq_data_get_chip_type(d);
	struct mvebu_gpio_chip *mvchip = gc->private;
	int pin;
	u32 u;

	pin = d->hwirq;

	u = readl_relaxed(mvebu_gpioreg_io_conf(mvchip, pin)) & (1 << GPIO_REG_PIN_OFF(pin));
	if (u)
		return -EINVAL;

	type &= IRQ_TYPE_SENSE_MASK;
	if (type == IRQ_TYPE_NONE)
		return -EINVAL;

	/* Check if we need to change chip and handler. */
	if (!(ct->type & type))
		if (irq_setup_alt_chip(d, type))
			return -EINVAL;

	/* Configure interrupt polarity. */
	switch (type) {
	case IRQ_TYPE_EDGE_RISING:
		u = readl_relaxed(mvebu_gpioreg_int_pol(mvchip, pin));
		u &= ~(1 << GPIO_REG_PIN_OFF(pin));
		writel_relaxed(u, mvebu_gpioreg_int_pol(mvchip, pin));
		break;
	case IRQ_TYPE_EDGE_FALLING:
		u = readl_relaxed(mvebu_gpioreg_int_pol(mvchip, pin));
		u |= 1 << GPIO_REG_PIN_OFF(pin);
		writel_relaxed(u, mvebu_gpioreg_int_pol(mvchip, pin));
		break;
	case IRQ_TYPE_EDGE_BOTH: {
		u32 v;

		v = readl_relaxed(mvebu_gpioreg_data_in(mvchip, pin));

		/* Set initial polarity based on current input level. */
		u = readl_relaxed(mvebu_gpioreg_int_pol(mvchip, pin));
		if (v & (1 << GPIO_REG_PIN_OFF(pin)))
			u |= 1 << GPIO_REG_PIN_OFF(pin);		/* falling */
		else
			u &= ~(1 << GPIO_REG_PIN_OFF(pin));	/* rising */
		writel_relaxed(u, mvebu_gpioreg_int_pol(mvchip, pin));
		break;
	}
	default:
		pr_err("Now a3700 only support edge IRQ types!!!");
		return -EINVAL;
	}
	return 0;
}

static void mvebu_gpio_irq_handler(struct irq_desc *desc)
{
	struct mvebu_gpio_chip *mvchip = irq_desc_get_handler_data(desc);
	struct irq_chip *chip = irq_desc_get_chip(desc);
	u32 cause, type;
	int i, start;

	if (mvchip == NULL)
		return;

	chained_irq_enter(chip, desc);

	for (start = 0; start < mvchip->chip.ngpio; start += GPIO_IO_BITWIDTH) {
		cause = readl_relaxed(mvebu_gpioreg_edge_cause(mvchip, start)) &
			readl_relaxed(mvebu_gpioreg_edge_mask(mvchip, start));

		for (i = 0; (i < (mvchip->chip.ngpio - start)) && (i < GPIO_IO_BITWIDTH); i++) {
			int irq;

			irq = mvchip->irqbase + start + i;

			if (!(cause & (1 << i)))
				continue;

			type = irq_get_trigger_type(irq);
			if ((type & IRQ_TYPE_SENSE_MASK) == IRQ_TYPE_EDGE_BOTH) {
				/* Swap polarity (race with GPIO line) */
				u32 polarity, level;

				level = readl_relaxed(mvebu_gpioreg_data_in(mvchip, start));
				polarity = readl_relaxed(mvebu_gpioreg_int_pol(mvchip, start));
				if ((polarity ^ level) & (1 << i)) {
					polarity ^= 1 << i;
					writel_relaxed(polarity, mvebu_gpioreg_int_pol(mvchip, start));
				} else {
					/*
					 * For spurious irq, which gpio level is not as expected after
					 * incoming edge, just ack the gpio irq.
					 */
					writel_relaxed(1 << i, mvebu_gpioreg_edge_cause(mvchip, start));
					continue;
				}
			}

			generic_handle_irq(irq);
		}
	}

	chained_irq_exit(chip, desc);
}

#ifdef CONFIG_DEBUG_FS
#include <linux/seq_file.h>

static void mvebu_gpio_dbg_show(struct seq_file *s, struct gpio_chip *chip)
{
	struct mvebu_gpio_chip *mvchip = container_of(chip, struct mvebu_gpio_chip, chip);
	u32 out, io_conf, data_in, int_pol, cause, edg_msk;
	int pin;

	for (pin = 0; pin < chip->ngpio; pin++) {
		const char *label;
		u32 msk;
		bool is_out;

		label = gpiochip_is_requested(chip, pin);
		if (!label)
			continue;

		out	= readl_relaxed(mvebu_gpioreg_out(mvchip, pin));
		io_conf = readl_relaxed(mvebu_gpioreg_io_conf(mvchip, pin));
		data_in = readl_relaxed(mvebu_gpioreg_data_in(mvchip, pin));
		int_pol	= readl_relaxed(mvebu_gpioreg_int_pol(mvchip, pin));
		cause	= readl_relaxed(mvebu_gpioreg_edge_cause(mvchip, pin));
		edg_msk	= readl_relaxed(mvebu_gpioreg_edge_mask(mvchip, pin));

		pin = GPIO_REG_PIN_OFF(pin);

		msk = 1 << pin;
		if (io_conf & msk)
			is_out = true;
		else
			is_out = false;

		seq_printf(s, " gpio-%-3d (%-20.20s)", chip->base + pin, label);

		if (is_out) {
			seq_printf(s, " out %s\n",
				   out & msk ? "hi" : "lo");
			continue;
		}

		seq_printf(s, " in  %s (act %s) - IRQ",
			   data_in & msk  ? "hi" : "lo",
			   int_pol & msk ? "lo" : "hi");
		if (!(edg_msk & msk)) {
			seq_puts(s, " disabled\n");
			continue;
		}
		if (edg_msk & msk)
			seq_puts(s, " edge ");
		seq_printf(s, " (%s)\n", cause & msk ? "pending" : "clear  ");
	}
}

#else
#define mvebu_gpio_dbg_show NULL
#endif

static const struct of_device_id mvebu_gpio_of_match[] = {
	{ .compatible = "marvell,armada3700-gpio", },
	{},
};
MODULE_DEVICE_TABLE(of, mvebu_gpio_of_match);

static int mvebu_gpio_suspend(struct platform_device *pdev, pm_message_t state)
{
	struct mvebu_gpio_chip *mvchip = platform_get_drvdata(pdev);

	/*
	 * There are 36 GPIO pin for A3700, so there are two registers holding
	 * values, pin 0 - 31 is at the first reg, 32 - 35 are at the second
	 */
	mvchip->out_reg = readl(mvebu_gpioreg_out(mvchip, 0));
	mvchip->io_conf_reg = readl(mvebu_gpioreg_io_conf(mvchip, 0));
	mvchip->out_reg_hi = readl(mvebu_gpioreg_out(mvchip, GPIO_IO_BITWIDTH));
	mvchip->io_conf_reg_hi = readl(mvebu_gpioreg_io_conf(mvchip, GPIO_IO_BITWIDTH));
	mvchip->irq_pol_reg = readl(mvebu_gpioreg_int_pol(mvchip, 0));
	mvchip->irq_pol_reg_hi = readl(mvebu_gpioreg_int_pol(mvchip, GPIO_IO_BITWIDTH));
	mvchip->edge_mask_regs[0] = readl(mvebu_gpioreg_edge_mask(mvchip, 0));
	if (mvchip->chip.ngpio > GPIO_IO_BITWIDTH)
		mvchip->edge_mask_regs[1] = readl(mvebu_gpioreg_edge_mask(mvchip, GPIO_IO_BITWIDTH));

	return 0;
}

static int mvebu_gpio_resume(struct platform_device *pdev)
{
	struct mvebu_gpio_chip *mvchip = platform_get_drvdata(pdev);

	/*
	 * There are 36 GPIO pin for A3700, so there are two registers holding
	 * values, pin 0 - 31 is at the first reg, 32 - 35 are at the second
	 */
	writel(mvchip->out_reg, mvebu_gpioreg_out(mvchip, 0));
	writel(mvchip->io_conf_reg, mvebu_gpioreg_io_conf(mvchip, 0));
	writel(mvchip->out_reg_hi, mvebu_gpioreg_out(mvchip, GPIO_IO_BITWIDTH));
	writel(mvchip->io_conf_reg_hi, mvebu_gpioreg_io_conf(mvchip, GPIO_IO_BITWIDTH));
	writel(mvchip->irq_pol_reg, mvebu_gpioreg_int_pol(mvchip, 0));
	writel(mvchip->irq_pol_reg_hi, mvebu_gpioreg_int_pol(mvchip, GPIO_IO_BITWIDTH));
	writel(mvchip->edge_mask_regs[0], mvebu_gpioreg_edge_mask(mvchip, 0));
	if (mvchip->chip.ngpio > GPIO_IO_BITWIDTH)
		writel(mvchip->edge_mask_regs[1], mvebu_gpioreg_edge_mask(mvchip, GPIO_IO_BITWIDTH));

	return 0;
}

static int mvebu_gpio_probe(struct platform_device *pdev)
{
	struct mvebu_gpio_chip *mvchip;
	const struct of_device_id *match;
	struct device_node *np = pdev->dev.of_node;
	struct resource *res;
	struct irq_chip_generic *gc;
	struct irq_chip_generic *gcs[MVEBU_IRQ_CHIP_NUM];
	struct irq_chip_type *ct;
	struct clk *clk;
	unsigned int ngpios;
	unsigned int gpio_base = -1;
	int i, id;
	int err;
	int irq_num;
	unsigned int gc_num = 0;
	u32 irq_mask;

	match = of_match_device(mvebu_gpio_of_match, &pdev->dev);
	if (match == NULL)
		return -ENODEV;

	mvchip = devm_kzalloc(&pdev->dev, sizeof(struct mvebu_gpio_chip),
			      GFP_KERNEL);
	if (!mvchip)
		return -ENOMEM;

	platform_set_drvdata(pdev, mvchip);

	if (of_property_read_u32(pdev->dev.of_node, "ngpios", &ngpios)) {
		dev_err(&pdev->dev, "Missing ngpios OF property\n");
		return -ENODEV;
	}

	if (of_property_read_u32(pdev->dev.of_node, "gpiobase", &gpio_base))
		gpio_base = -1;

	id = of_alias_get_id(pdev->dev.of_node, "gpio");
	if (id < 0) {
		dev_err(&pdev->dev, "Couldn't get OF id\n");
		return id;
	}

	clk = devm_clk_get(&pdev->dev, NULL);
	/* Not all SoCs require a clock. */
	if (!IS_ERR(clk))
		clk_prepare_enable(clk);

	mvchip->chip.label = dev_name(&pdev->dev);
	mvchip->chip.parent = &pdev->dev;
	mvchip->chip.request = gpiochip_generic_request;
	mvchip->chip.free = gpiochip_generic_free;
	mvchip->chip.direction_input = mvebu_gpio_direction_input;
	mvchip->chip.get = mvebu_gpio_get;
	mvchip->chip.direction_output = mvebu_gpio_direction_output;
	mvchip->chip.set = mvebu_gpio_set;
	mvchip->chip.dbg_show = mvebu_gpio_dbg_show;
	mvchip->chip.to_irq = mvebu_gpio_to_irq;
	mvchip->chip.ngpio = ngpios;
	mvchip->chip.can_sleep = false;
	mvchip->chip.of_node = np;
	if (gpio_base != -1)
		mvchip->chip.base = gpio_base;
	else
		mvchip->chip.base = id * MVEBU_MAX_GPIO_PER_BANK;

	spin_lock_init(&mvchip->lock);
	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	mvchip->membase = devm_ioremap_resource(&pdev->dev, res);
	if (IS_ERR(mvchip->membase))
		return PTR_ERR(mvchip->membase);

	res = platform_get_resource(pdev, IORESOURCE_MEM, 1);
	mvchip->interrupt_membase = devm_ioremap_resource(&pdev->dev,
						       res);
	if (IS_ERR(mvchip->interrupt_membase))
		return PTR_ERR(mvchip->interrupt_membase);

	/*
	 * Mask and clear GPIO interrupts.
	 * a3700 interrupt status registers are RW1C.
	 */
	writel_relaxed(~0, mvebu_gpioreg_edge_cause(mvchip, 0));
	writel_relaxed(0, mvebu_gpioreg_edge_mask(mvchip, 0));
	if (ngpios > GPIO_IO_BITWIDTH) {
		writel_relaxed(~0, mvebu_gpioreg_edge_cause(mvchip, GPIO_IO_BITWIDTH));
		writel_relaxed(0, mvebu_gpioreg_edge_mask(mvchip, GPIO_IO_BITWIDTH));
	}

	gpiochip_add(&mvchip->chip);

	/* Some gpio controllers do not provide irq support. */
	irq_num = of_irq_count(np);
	if (!irq_num)
		return 0;

	/* Setup the interrupt handlers. */
	for (i = 0; i < irq_num; i++) {
		int irq = platform_get_irq(pdev, i);

		if (irq < 0)
			continue;

		irq_set_chained_handler_and_data(irq, mvebu_gpio_irq_handler, mvchip);
	}

	mvchip->irqbase = irq_alloc_descs(-1, 0, ngpios, -1);
	if (mvchip->irqbase < 0) {
		dev_err(&pdev->dev, "no irqs\n");
		err = mvchip->irqbase;
		goto err_gpio;
	}

	/*
	 * Since irq_setup_generic_chip() can only set up max 32 interrupts,
	 * we need to create multi generic irq chips.
	 */
	for (i = 0; i < ngpios; i += MVEBU_IRQ_MASK_BIT_NUM_MAX) {
		gc = irq_alloc_generic_chip("mvebu_gpio_irq", MVEBU_GPIO_CHIP_TYPE_NUM,
					    mvchip->irqbase + i, mvchip->membase, handle_edge_irq);
		if (!gc) {
			dev_err(&pdev->dev, "Can't allocate generic irq_chip\n");
			err = -ENOMEM;
			goto err_gpio;
		}

		if (gc_num >= MVEBU_IRQ_CHIP_NUM) {
			dev_err(&pdev->dev, "Can't create more than %d generic irq_chips\n", MVEBU_IRQ_CHIP_NUM);
			err = -EPERM;
			kfree(gc);
			goto err_gpio;
		}
		gcs[gc_num++] = gc;

		gc->private = mvchip;
		ct = &gc->chip_types[0];
		ct->type = IRQ_TYPE_EDGE_RISING | IRQ_TYPE_EDGE_FALLING;
		ct->chip.irq_ack = mvebu_gpio_irq_ack;
		ct->chip.irq_mask = mvebu_gpio_edge_irq_mask;
		ct->chip.irq_unmask = mvebu_gpio_edge_irq_unmask;
		ct->chip.irq_set_type = mvebu_gpio_irq_set_type;
		ct->chip.name = mvchip->chip.label;

		if ((i + MVEBU_IRQ_MASK_BIT_NUM_MAX) < ngpios)
			irq_mask = IRQ_MSK(MVEBU_IRQ_MASK_BIT_NUM_MAX);
		else
			irq_mask = IRQ_MSK(ngpios - i);
		irq_setup_generic_chip(gc, irq_mask, 0,
			       IRQ_NOREQUEST, IRQ_LEVEL | IRQ_NOPROBE);
	}

	/* Setup irq domain on top of the generic chip. */
	mvchip->domain = irq_domain_add_simple(np, mvchip->chip.ngpio,
					       mvchip->irqbase,
					       &irq_domain_simple_ops,
					       mvchip);
	if (!mvchip->domain) {
		dev_err(&pdev->dev, "couldn't allocate irq domain %s (DT).\n",
			mvchip->chip.label);
		err = -ENODEV;
		goto err_gpio;
	}

	return 0;

err_gpio:
	/* Remove the created generic irq chips. */
	for (i = 0; i < gc_num; i++) {
		if (((i + 1) * MVEBU_IRQ_MASK_BIT_NUM_MAX) < ngpios)
			irq_mask = IRQ_MSK(MVEBU_IRQ_MASK_BIT_NUM_MAX);
		else
			irq_mask = IRQ_MSK(ngpios - i * MVEBU_IRQ_MASK_BIT_NUM_MAX);
		irq_remove_generic_chip(gcs[i], irq_mask, IRQ_NOREQUEST, IRQ_LEVEL | IRQ_NOPROBE);
		kfree(gcs[i]);
	}

	gpiochip_remove(&mvchip->chip);

	return err;
}

static struct platform_driver mvebu_gpio_driver = {
	.driver		= {
		.name		= "mvebu-a3700-gpio",
		.of_match_table = mvebu_gpio_of_match,
	},
	.probe		= mvebu_gpio_probe,
	.suspend        = mvebu_gpio_suspend,
	.resume         = mvebu_gpio_resume,
};
module_platform_driver(mvebu_gpio_driver);
