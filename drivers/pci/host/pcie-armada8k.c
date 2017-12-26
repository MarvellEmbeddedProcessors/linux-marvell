/*
 * PCIe host controller driver for Marvell Armada-8K SoCs
 *
 * Armada-8K PCIe Glue Layer Source Code
 *
 * This file is licensed under the terms of the GNU General Public
 * License version 2. This program is licensed "as is" without any
 * warranty of any kind, whether express or implied.
 */

#define pr_fmt(fmt) "armada-8k-pcie: " fmt

#include <linux/clk.h>
#include <linux/delay.h>
#include <linux/interrupt.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/of.h>
#include <linux/pci.h>
#include <linux/phy/phy.h>
#include <linux/platform_device.h>
#include <linux/resource.h>
#include <linux/of_pci.h>
#include <linux/of_irq.h>
#include <dt-bindings/phy/phy-comphy-mvebu.h>
#include <linux/of_gpio.h>

#include "pcie-designware.h"

struct armada8k_pcie {
	void __iomem		*regs_base;
	struct phy		**phys;
	int			phy_count;
	struct clk		*clk;
	struct pcie_port	pp;
	struct gpio_desc	*reset_gpio;
	enum of_gpio_flags	flags;
};

struct armada8k_pcie_rst {
	bool			is_reseted;
	struct gpio_desc	*gpio;
	struct list_head	list;
};

#define PCIE_GLOBAL_CONTROL             0x0
#define PCIE_APP_LTSSM_EN               (1 << 2)
#define PCIE_DEVICE_TYPE_OFFSET         (4)
#define PCIE_DEVICE_TYPE_MASK           (0xF)
#define PCIE_DEVICE_TYPE_EP             (0x0) /* Endpoint */
#define PCIE_DEVICE_TYPE_LEP            (0x1) /* Legacy endpoint */
#define PCIE_DEVICE_TYPE_RC             (0x4) /* Root complex */

#define PCIE_GLOBAL_STATUS              0x8
#define PCIE_GLB_STS_RDLH_LINK_UP       (1 << 1)
#define PCIE_GLB_STS_PHY_LINK_UP        (1 << 9)

#define PCIE_GLOBAL_INT_CAUSE1		0x1C
#define PCIE_GLOBAL_INT_MASK1		0x20
#define PCIE_INT_A_ASSERT_MASK		(1 << 9)
#define PCIE_INT_B_ASSERT_MASK		(1 << 10)
#define PCIE_INT_C_ASSERT_MASK		(1 << 11)
#define PCIE_INT_D_ASSERT_MASK		(1 << 12)

#define PCIE_ARCACHE_TRC                0x50
#define PCIE_AWCACHE_TRC                0x54
#define PCIE_ARUSER			0x5C
#define PCIE_AWUSER			0x60
/* AR/AW Cache defauls:
** - Normal memory
** - Write-Back
** - Read / Write allocate
*/
#define ARCACHE_DEFAULT_VALUE		0x3511
#define AWCACHE_DEFAULT_VALUE		0x5311

#define DOMAIN_OUTER_SHAREABLE		0x2
#define AX_USER_DOMAIN_MASK		0x3
#define AX_USER_DOMAIN_OFFSET		4

#define PCIE_LINK_CAPABILITY		0x7C

#define PCIE_LINK_CONTROL_LINK_STATUS	0x80
#define PCIE_LINK_SPEED_OFFSET	16
#define PCIE_LINK_SPEED_MASK	(0xF << PCIE_LINK_SPEED_OFFSET)
#define PCIE_LINK_WIDTH_OFFSET	20
#define PCIE_LINK_WIDTH_MASK	(0xF << PCIE_LINK_WIDTH_OFFSET)
#define PCIE_LINK_TRAINING		BIT(27)

#define PCIE_LINK_CTL_2			0xA0
#define TARGET_LINK_SPEED_MASK		0xF
#define LINK_SPEED_GEN_1		0x1
#define LINK_SPEED_GEN_2		0x2
#define LINK_SPEED_GEN_3		0x3

#define PCIE_MSIX_CAP_ID_NEXT_CTRL_REG	0xB0
#define PCIE_MSIX_CAP_NEXT_OFFSET_MASK	0xff00

#define PCIE_SPCIE_CAP_HEADER_REG	0x158
#define PCIE_SPCIE_NEXT_OFFSET_MASK	0xFFF00000
#define PCIE_SPCIE_NEXT_OFFSET_OFFSET	20
#define PCIE_SPCIE_NEXT_SKIP_SRIOV	0x1B8

#define PCIE_LANE_EQ_CTRL01_REG		0x164
#define PCIE_LANE_EQ_CTRL23_REG		0x168
#define PCIE_LANE_EQ_SETTING		0x55555555

#define PCIE_TPH_EXT_CAP_HDR_REG	0x1B8
#define PCIE_TPH_REQ_NEXT_PTR_MASK	0xFFF00000
#define PCIE_TPH_REQ_NEXT_PTR_OFFSET	20
#define PCIE_TPH_REQ_NEXT_SKIP_SRIOV	0x24C

#define PCIE_PORT_FORCE_OFF		0x708
#define PCIE_FORCE_EN			BIT(15)

#define PCIE_LINK_WIDTH_SPEED_CONTROL	0x80C
#define PORT_LOGIC_SPEED_CHANGE		BIT(17)

#define PCIE_GEN3_EQ_CONTROL_OFF_REG	0x8A8
#define PCIE_GEN3_EQ_PSET_REQ_VEC_MASK	0xFFFF00
#define PCIE_GEN3_EQ_PSET_REQ_VEC_OFFSET 8
#define PCIE_GEN3_EQ_PSET_4		0x10
#define PCIE_GEN3_EQU_EVAL_2MS_DISABLE	BIT(5)

#define PCIE_LINK_FLUSH_CONTROL_OFF_REG	0x8CC
#define PCIE_AUTO_FLUSH_EN_MASK		0x1

#define PCIE_LINK_UP_TIMEOUT_MS		1000
#define PCIE_SPEED_CHANGE_TIMEOUT_MS	300

#define to_armada8k_pcie(x)	container_of(x, struct armada8k_pcie, pp)

/*
 * PCIe ports on CPx share the same reset GPIO on A8K/7K-DB.
 * In future each PCIe port maybe have its own GPIO for EP reset, or some
 * PCIe ports share one GPIO and other use another one, etc..
 * So it is necessary to record the information of EP reset GPIO for each
 * PCIe port. According to above case analysis, there maybe different count
 * of GPIO in different cases, in order to support different cases, a global
 * list is involved to store the reset GPIO descriptor and flag(to indicate
 * reset has been implemented or not) accordingly, which will be shared by
 * all PCIe ports on all CPs.
 * For each PCIe port with EP reset connected to GPIO, the list will be
 * traversed to try to find the same GPIO, if it is not found, the GPIO
 * information will be allocated dynamicly and initialized and then reset EP;
 * if it is found in the list, the flag will be checked, if the flag indicates
 * EP reset already done, then skip repeated reset, or reset EP and set the
 * flag.
 * In suspend to RAM process, the list is also useful. When suspend the
 * corresponding GPIO will be searched in list and the EP reset flag will be
 * cleared if it is found. When resume the EP reset will be done like probe.
 */
static struct list_head a8k_rst_gpio_list = LIST_HEAD_INIT(a8k_rst_gpio_list);

static int armada8k_pcie_link_up(struct pcie_port *pp)
{
	u32 reg;
	struct armada8k_pcie *armada8k_pcie = to_armada8k_pcie(pp);
	u32 mask = PCIE_GLB_STS_RDLH_LINK_UP | PCIE_GLB_STS_PHY_LINK_UP;

	reg = readl(armada8k_pcie->regs_base + PCIE_GLOBAL_STATUS);

	if ((reg & mask) == mask)
		return 1;

	pr_debug("No link detected (Global-Status: 0x%08x).\n", reg);
	return 0;
}

static void armada8k_pcie_dw_configure(void __iomem *regs_base, u32 cap_speed)
{
	u32 reg;
	/*
	 * TODO (shadi@marvell.com, sr@denx.de):
	 * Need to read the serdes speed from the dts and according to it
	 * configure the PCIe gen
	 */

	/* Set link to GEN 3 */
	reg = readl(regs_base + PCIE_LINK_CTL_2);
	reg &= ~TARGET_LINK_SPEED_MASK;
	reg |= cap_speed;
	writel(reg, regs_base + PCIE_LINK_CTL_2);

	reg = readl(regs_base + PCIE_LINK_CAPABILITY);
	reg &= ~TARGET_LINK_SPEED_MASK;
	reg |= cap_speed;
	writel(reg, regs_base + PCIE_LINK_CAPABILITY);

	reg = readl(regs_base + PCIE_GEN3_EQ_CONTROL_OFF_REG);
	reg &= ~PCIE_GEN3_EQU_EVAL_2MS_DISABLE;
	writel(reg, regs_base + PCIE_GEN3_EQ_CONTROL_OFF_REG);
}

static void armada8k_pcie_dw_mvebu_pcie_config(void __iomem *regs_base)
{
	u32 reg;

	/*
	 * Set the correct hints for lane equalization.
	 *
	 * These registers consist of the following fields:
	 *	- Downstream Port Transmitter Preset - Used for equalization by
	 *	  this port when the Port is operating as a downstream Port.
	 *	- Downstream Port Receiver Preset Hint - May be used as a hint
	 *	  for receiver equalization by this port when the Port is
	 *	  operating as a downstream Port.
	 *	- Upstream Port Transmitter Preset - Field contains the
	 *	  transmit preset value sent or received during link
	 *	  equalization.
	 *	- Upstream Port Receiver Preset Hint - Field contains the
	 *	  receiver preset hint value sent or received during link
	 *	  equalization.
	 *
	 * The default values for this registers aren't optimal for our
	 * hardware, so we set the optimal values according to HW measurements.
	 */
	writel(PCIE_LANE_EQ_SETTING, regs_base + PCIE_LANE_EQ_CTRL01_REG);
	writel(PCIE_LANE_EQ_SETTING, regs_base + PCIE_LANE_EQ_CTRL23_REG);

	/*
	 * There is an issue in CPN110 that does not allow to
	 * enable/disable the link and perform "hot reset" unless
	 * the auto flush is disabled. So in order to enable the option
	 * to perform hot reset and link disable/enable we need to set
	 * auto flush to disable.
	 */
	reg = readl(regs_base + PCIE_LINK_FLUSH_CONTROL_OFF_REG);
	reg &= ~PCIE_AUTO_FLUSH_EN_MASK;
	writel(reg, regs_base + PCIE_LINK_FLUSH_CONTROL_OFF_REG);

	/*
	 * According to the electrical measurmentrs, the best preset that our
	 * receiver can handle is preset4, so we are changing the vector of
	 * presets to evaluate during the link equalization training to preset4.
	 */
	reg = readl(regs_base + PCIE_GEN3_EQ_CONTROL_OFF_REG);
	reg &= ~PCIE_GEN3_EQ_PSET_REQ_VEC_MASK;
	reg |= PCIE_GEN3_EQ_PSET_4 << PCIE_GEN3_EQ_PSET_REQ_VEC_OFFSET;
	writel(reg, regs_base + PCIE_GEN3_EQ_CONTROL_OFF_REG);

	/*
	 * Remove VPD capability from the capability list,
	 * since we don't support it.
	 */
	reg = readl(regs_base + PCIE_MSIX_CAP_ID_NEXT_CTRL_REG);
	reg &= ~PCIE_MSIX_CAP_NEXT_OFFSET_MASK;
	writel(reg, regs_base + PCIE_MSIX_CAP_ID_NEXT_CTRL_REG);

	/*
	 * The below two configurations are intended to remove SRIOV capability
	 * from the capability list, since we don't support it.
	 * The capability list is a linked list where each capability points
	 * to the next capability, so in the SRIOV capability need to set the
	 * previous capability to point to the next capability and this way
	 * the SRIOV capability will be skipped.
	 */
	reg = readl(regs_base + PCIE_TPH_EXT_CAP_HDR_REG);
	reg &= ~PCIE_TPH_REQ_NEXT_PTR_MASK;
	reg |= PCIE_TPH_REQ_NEXT_SKIP_SRIOV << PCIE_TPH_REQ_NEXT_PTR_OFFSET;
	writel(reg, regs_base + PCIE_TPH_EXT_CAP_HDR_REG);

	reg = readl(regs_base + PCIE_SPCIE_CAP_HEADER_REG);
	reg &= ~PCIE_SPCIE_NEXT_OFFSET_MASK;
	reg |= PCIE_SPCIE_NEXT_SKIP_SRIOV << PCIE_SPCIE_NEXT_OFFSET_OFFSET;
	writel(reg, regs_base + PCIE_SPCIE_CAP_HEADER_REG);
}

static int armada8k_pcie_wait_link_up(struct pcie_port *pp)
{
	unsigned long timeout;
	u32 reg;

	/*
	 * According to HW, taking Armada7k-PCAC board(as PCIe end-point card)
	 * into consideration; it is suggested to set the max delay to 1000ms.
	 */
	timeout = jiffies + PCIE_LINK_UP_TIMEOUT_MS * HZ / 1000;
	while (!armada8k_pcie_link_up(pp)) {
		if (time_after(jiffies, timeout))
			return -1;
	}

	/*
	 * Link can be established in Gen 1. It still need to wait
	 * until MAC nagaotiation(speed changes) is completed.
	 * 300ms delay is according to HW design guidelines.
	 */
	mdelay(PCIE_SPEED_CHANGE_TIMEOUT_MS);
	/* To print the link information */
	reg = readl(pp->dbi_base + PCIE_LINK_CONTROL_LINK_STATUS);
	dev_info(pp->dev, "PCIe Link up: Gen%d-x%d\n",
		(reg & PCIE_LINK_SPEED_MASK) >> PCIE_LINK_SPEED_OFFSET,
		(reg & PCIE_LINK_WIDTH_MASK) >> PCIE_LINK_WIDTH_OFFSET);

	return 0;
}

static void armada8k_pcie_host_init(struct pcie_port *pp)
{
	struct armada8k_pcie *armada8k_pcie = to_armada8k_pcie(pp);
	void __iomem *regs_base = armada8k_pcie->regs_base;
	u32 reg;

	/* Set the device to root complex mode */
	reg = readl(regs_base + PCIE_GLOBAL_CONTROL);
	reg &= ~(PCIE_DEVICE_TYPE_MASK << PCIE_DEVICE_TYPE_OFFSET);
	reg |= PCIE_DEVICE_TYPE_RC << PCIE_DEVICE_TYPE_OFFSET;
	writel(reg, regs_base + PCIE_GLOBAL_CONTROL);

	/* Set the PCIe master AxCache attributes */
	writel(ARCACHE_DEFAULT_VALUE, regs_base + PCIE_ARCACHE_TRC);
	writel(AWCACHE_DEFAULT_VALUE, regs_base + PCIE_AWCACHE_TRC);

	/* Set the PCIe master AxDomain attributes */
	reg = readl(regs_base + PCIE_ARUSER);
	reg &= ~(AX_USER_DOMAIN_MASK << AX_USER_DOMAIN_OFFSET);
	reg |= DOMAIN_OUTER_SHAREABLE << AX_USER_DOMAIN_OFFSET;
	writel(reg, regs_base + PCIE_ARUSER);

	reg = readl(regs_base + PCIE_AWUSER);
	reg &= ~(AX_USER_DOMAIN_MASK << AX_USER_DOMAIN_OFFSET);
	reg |= DOMAIN_OUTER_SHAREABLE << AX_USER_DOMAIN_OFFSET;
	writel(reg, regs_base + PCIE_AWUSER);

	dw_pcie_setup_rc(pp);

	/* Enable INT A-D interrupts */
	reg = readl(regs_base + PCIE_GLOBAL_INT_MASK1);
	reg |= PCIE_INT_A_ASSERT_MASK | PCIE_INT_B_ASSERT_MASK |
	       PCIE_INT_C_ASSERT_MASK | PCIE_INT_D_ASSERT_MASK;
	writel(reg, regs_base + PCIE_GLOBAL_INT_MASK1);

	/* Configuration only when COMPHY independency on uboot */
	if (armada8k_pcie->phy_count > 0) {
		/* DW pre link configurations */
		armada8k_pcie_dw_configure(pp->dbi_base, LINK_SPEED_GEN_3);

		/* Mvebu pre link specific configuration */
		armada8k_pcie_dw_mvebu_pcie_config(pp->dbi_base);
	}

	/*
	 * Configuration done. Start LTSSM.
	 * If the link already established in bootloader, this step does not
	 * take effect, so it is not necessary to check link status before it.
	 */
	reg = readl(regs_base + PCIE_GLOBAL_CONTROL);
	reg |= PCIE_APP_LTSSM_EN;
	writel(reg, regs_base + PCIE_GLOBAL_CONTROL);

	/* Check that link was established */
	if (armada8k_pcie_wait_link_up(pp))
		dev_err(pp->dev, "Link not up after reconfiguration\n");
}

#ifdef CONFIG_PCI_MSI
static int armada8k_pcie_msi_init(struct pcie_port *pp, struct msi_controller *chip)
{
	struct device_node *msi_node;
	struct msi_controller	*msi;

	msi_node = of_parse_phandle(pp->dev->of_node, "msi-parent", 0);
	if (!msi_node)
		return -ENXIO;

	/* Override the designware MSI chip. The designware registration
	 * method doesnt allow to supply a private msi chip so we resort
	 * to overriding it. should probably change the DW driver */
	msi = of_pci_find_msi_chip_by_node(msi_node);
	if (msi)
		*chip = *msi;

	return 0;
}
#endif

static void armada8k_pcie_clear_irq_pulse(struct pcie_port *pp)
{
	struct armada8k_pcie *armada8k_pcie = to_armada8k_pcie(pp);
	void __iomem *regs_base = armada8k_pcie->regs_base;
	u32 val;

	val = readl(regs_base + PCIE_GLOBAL_INT_CAUSE1);
	writel(val, regs_base + PCIE_GLOBAL_INT_CAUSE1);
}

static irqreturn_t armada8k_pcie_irq_handler(int irq, void *arg)
{
	struct pcie_port *pp = arg;

	armada8k_pcie_clear_irq_pulse(pp);
	return IRQ_HANDLED;
}

static struct pcie_host_ops armada8k_pcie_host_ops = {
	.link_up = armada8k_pcie_link_up,
	.host_init = armada8k_pcie_host_init,
#ifdef CONFIG_PCI_MSI
	.msi_host_init = armada8k_pcie_msi_init,
#endif
};

static int armada8k_add_pcie_port(struct pcie_port *pp,
					 struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	int ret;

	pp->root_bus_nr = -1;
	pp->ops = &armada8k_pcie_host_ops;

	pp->irq = platform_get_irq(pdev, 0);
	if (!pp->irq) {
		dev_err(dev, "failed to get irq for port\n");
		return -ENODEV;
	}

	ret = devm_request_irq(dev, pp->irq, armada8k_pcie_irq_handler,
				IRQF_SHARED, "armada8k-pcie", pp);
	if (ret) {
		dev_err(dev, "failed to request irq %d\n", pp->irq);
		return ret;
	}


	ret = dw_pcie_host_init(pp);
	if (ret) {
		dev_err(dev, "failed to initialize host\n");
		return ret;
	}

	return 0;
}

/* armada8k_pcie_rst_find
 * The function traverses the PCIe reset GPIO list and find the node matches
 * with input pointer of gpio descriptor
 * Return: if there is match, return matched node device.
 *         if there is no match, return NULL.
 */
static struct armada8k_pcie_rst *armada8k_pcie_rst_find(struct gpio_desc *gpio)
{
	struct list_head *curr;
	struct armada8k_pcie_rst *node;

	if (list_empty(&a8k_rst_gpio_list))
		return NULL;

	list_for_each(curr, &a8k_rst_gpio_list) {
		node = list_entry(curr, struct armada8k_pcie_rst, list);
		if (gpio == node->gpio)
			return node;
	}

	return NULL;
}

/* armada8k_pcie_reset
 * The function implements the PCIe reset via GPIO.
 * First, pull down the GPIO used to assert EP, and wait 1ms;
 * Second, set the GPIO output value with setting from DTS to deassert EP
 * Return: 0: success; non-zero: failed.
 */
static int armada8k_pcie_reset(struct armada8k_pcie *pcie)
{
	struct armada8k_pcie_rst *rst;

	rst = armada8k_pcie_rst_find(pcie->reset_gpio);
	/* Add to reset gpio list */
	if (!rst) {
		rst = devm_kzalloc(pcie->pp.dev,
				   sizeof(struct armada8k_pcie_rst),
				   GFP_KERNEL);
		if (!rst)
			return -ENOMEM;

		rst->gpio = pcie->reset_gpio;

		list_add(&rst->list, &a8k_rst_gpio_list);
	}

	if (rst->is_reseted == true)
		return 0;

	/* Assert EP */
	gpiod_direction_output(pcie->reset_gpio,
			       (pcie->flags & OF_GPIO_ACTIVE_LOW) ? 1 : 0);
	/* After 1ms to De-assert EP */
	mdelay(1);
	gpiod_direction_output(pcie->reset_gpio,
			       (pcie->flags & OF_GPIO_ACTIVE_LOW) ? 0 : 1);
	rst->is_reseted = true;

	return 0;
}

static int armada8k_pcie_probe(struct platform_device *pdev)
{
	struct armada8k_pcie *armada8k_pcie;
	struct pcie_port *pp;
	struct phy **phys = NULL;
	struct device *dev = &pdev->dev;
	struct resource *base;
	int reset_gpio, phy_count = 0, i = 0;
	u32 command;
	char phy_name[16];
	int ret = 0;

	armada8k_pcie = devm_kzalloc(dev, sizeof(*armada8k_pcie), GFP_KERNEL);
	if (!armada8k_pcie)
		return -ENOMEM;

	pp = &armada8k_pcie->pp;
	pp->dev = dev;

	armada8k_pcie->clk = devm_clk_get(dev, NULL);
	if (IS_ERR(armada8k_pcie->clk))
		return PTR_ERR(armada8k_pcie->clk);

	clk_prepare_enable(armada8k_pcie->clk);

	/* Config reset gpio for pcie if the reset connected to gpio */
	reset_gpio = of_get_named_gpio_flags(pdev->dev.of_node,
					     "reset-gpios", 0,
					     &armada8k_pcie->flags);
	if (reset_gpio == -EPROBE_DEFER) {
		ret = reset_gpio;
		goto fail_free;
	}
	if (gpio_is_valid(reset_gpio)) {
		armada8k_pcie->reset_gpio = gpio_to_desc(reset_gpio);
		ret = armada8k_pcie_reset(armada8k_pcie);
		if (ret) {
			dev_err(dev, "Reset EP failed!\n");
			goto fail_free;
		}
	}

	/* Get PHY count according to phy name */
	phy_count = of_property_count_strings(pdev->dev.of_node, "phy-names");
	if (phy_count > 0) {
		phys = devm_kzalloc(dev, sizeof(*phys) * phy_count, GFP_KERNEL);
		if (!phys)
			return -ENOMEM;

		for (i = 0; i < phy_count; i++) {
			snprintf(phy_name, sizeof(phy_name), "pcie-phy%d", i);
			phys[i] = devm_phy_get(dev, phy_name);
			if (IS_ERR(phys[i]))
				goto fail_free;

			/* Tell COMPHY the PCIE width based on phy command,
			 * and in PHY command callback, the width will be
			 * checked for its validation.
			 */
			switch (phy_count) {
			case PCIE_LNK_X1:
				command = COMPHY_COMMAND_PCIE_WIDTH_1;
				break;
			case PCIE_LNK_X2:
				command = COMPHY_COMMAND_PCIE_WIDTH_2;
				break;
			case PCIE_LNK_X4:
				command = COMPHY_COMMAND_PCIE_WIDTH_4;
				break;
			default:
				command = COMPHY_COMMAND_PCIE_WIDTH_UNSUPPORT;
			}
			phy_send_command(phys[i], command);

			ret = phy_init(phys[i]);
			if (ret < 0)
				goto fail_free;

			ret = phy_power_on(phys[i]);
			if (ret < 0) {
				phy_exit(phys[i]);
				goto fail_free;
			}
		}
	}

	armada8k_pcie->phys = phys;
	armada8k_pcie->phy_count = phy_count;
	platform_set_drvdata(pdev, armada8k_pcie);

	/* Get the dw-pcie unit configuration/control registers base. */
	base = platform_get_resource_byname(pdev, IORESOURCE_MEM, "ctrl");
	pp->dbi_base = devm_ioremap_resource(dev, base);
	if (IS_ERR(pp->dbi_base)) {
		dev_err(dev, "couldn't remap regs base %p\n", base);
		ret = PTR_ERR(pp->dbi_base);
		goto fail_free;
	}
	armada8k_pcie->regs_base = pp->dbi_base + 0x8000;

	pci_add_flags(PCI_REASSIGN_ALL_RSRC | PCI_REASSIGN_ALL_BUS);

	ret = armada8k_add_pcie_port(pp, pdev);
	if (ret < 0)
		goto fail_free;
	return 0;

fail_free:
	while (--i >= 0) {
		phy_power_off(phys[i]);
		phy_exit(phys[i]);
	}

	if (!IS_ERR(armada8k_pcie->clk))
		clk_disable_unprepare(armada8k_pcie->clk);

	return ret;
}

static int armada8k_pcie_suspend_noirq(struct device *dev)
{
	int i;
	struct armada8k_pcie *pcie;

	pcie = dev_get_drvdata(dev);

	/* Clear EP reset flag if it is connected to GPIO */
	if (pcie->reset_gpio) {
		struct armada8k_pcie_rst *rst;

		rst = armada8k_pcie_rst_find(pcie->reset_gpio);
		if (!rst)
			return -ENODEV;
		rst->is_reseted = false;
	}

	/* Gating clock */
	if (!IS_ERR(pcie->clk))
		clk_disable_unprepare(pcie->clk);

	/* Power off PHY */
	for (i = 0; i < pcie->phy_count; i++) {
		if (pcie->phys[i]) {
			phy_power_off(pcie->phys[i]);
			phy_exit(pcie->phys[i]);
		}
	}

	return 0;
}

static int armada8k_pcie_resume_noirq(struct device *dev)
{
	struct armada8k_pcie *pcie;
	int i, ret;

	pcie = dev_get_drvdata(dev);

	if (!IS_ERR(pcie->clk)) {
		ret = clk_prepare_enable(pcie->clk);
		if (ret) {
			dev_err(dev, "Failed to enable clock\n");
			return ret;
		}
	}

	/* Power on PHY */
	for (i = 0; i < pcie->phy_count; i++) {
		if (pcie->phys[i]) {
			u32 command;
			/* Tell COMPHY the PCIE width based on phy command,
			 * and in PHY command callback, the width will be
			 * checked for its validation.
			 */
			switch (pcie->phy_count) {
			case PCIE_LNK_X1:
				command = COMPHY_COMMAND_PCIE_WIDTH_1;
				break;
			case PCIE_LNK_X2:
				command = COMPHY_COMMAND_PCIE_WIDTH_2;
				break;
			case PCIE_LNK_X4:
				command = COMPHY_COMMAND_PCIE_WIDTH_4;
				break;
			default:
				command = COMPHY_COMMAND_PCIE_WIDTH_UNSUPPORT;
			}
			phy_send_command(pcie->phys[i], command);

			ret = phy_init(pcie->phys[i]);
			if (ret < 0)
				goto err_phy;
			ret = phy_power_on(pcie->phys[i]);
			if (ret < 0) {
				phy_exit(pcie->phys[i]);
				goto err_phy;
			}
		}
	}

	/* Reset PCIe if it is connected to GPIO */
	if (pcie->reset_gpio)
		armada8k_pcie_reset(pcie);

	/* Reinit PCIE host */
	armada8k_pcie_host_init(&pcie->pp);
	return 0;

err_phy:
	while (--i >= 0) {
		phy_power_off(pcie->phys[i]);
		phy_exit(pcie->phys[i]);
	}
	if (!IS_ERR(pcie->clk))
		clk_disable_unprepare(pcie->clk);

	return ret;
}

static const struct dev_pm_ops armada8k_pcie_pm_ops = {
	.suspend_noirq = armada8k_pcie_suspend_noirq,
	.resume_noirq = armada8k_pcie_resume_noirq,
};

static const struct of_device_id armada8k_pcie_of_match[] = {
	{ .compatible = "marvell,armada8k-pcie", },
	{},
};
MODULE_DEVICE_TABLE(of, armada8k_pcie_of_match);

static struct platform_driver armada8k_pcie_driver = {
	.probe		= armada8k_pcie_probe,
	.driver = {
		.name	= "armada8k-pcie",
		.of_match_table = of_match_ptr(armada8k_pcie_of_match),
		.pm	= &armada8k_pcie_pm_ops,
	},
};

module_platform_driver(armada8k_pcie_driver);

MODULE_DESCRIPTION("Armada 8k PCIe host controller driver");
MODULE_AUTHOR("Yehuda Yitshak <yehuday@marvell.com>");
MODULE_AUTHOR("Shadi Ammouri <shadi@marvell.com>");
MODULE_LICENSE("GPL v2");
