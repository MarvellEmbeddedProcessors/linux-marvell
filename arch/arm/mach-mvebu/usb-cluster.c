/*
 * USB cluster support for Armada 375 platform.
 *
 * Copyright (C) 2013 Marvell
 *
 * Gregory CLEMENT <gregory.clement@free-electrons.com>
 *
 * This file is licensed under the terms of the GNU General Public
 * License version 2.  This program is licensed "as is" without any
 * warranty of any kind, whether express or implied.
 *
 * Armada 375 comes with an USB2 host and device controller and an
 * USB3 controller. The USB cluster control register allows to manage
 * common features of both USB controller.
 */

#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/of_address.h>
#include <linux/io.h>
#include <linux/slab.h>

#define USB2_PHY_CONFIG_ENABLE BIT(0) /* active low */

static struct of_device_id of_usb_cluster_table[] = {
	{ .compatible = "marvell,armada-375-usb-cluster", },
	{ /* end of list */ },
};

static int __init mvebu_usb_cluster_init(void)
{
	struct device_node *np;

	np = of_find_matching_node(NULL, of_usb_cluster_table);
	if (np) {
		void __iomem *usb_cluster_base;
		u32 reg;
		struct device_node *ehci_node, *xhci_node;
		struct property *ehci_status;
		bool use_usb3 = false;

		usb_cluster_base = of_iomap(np, 0);
		BUG_ON(!usb_cluster_base);

		xhci_node = of_find_compatible_node(NULL, NULL,
						"marvell,xhci-armada-375");

		if (xhci_node && of_device_is_available(xhci_node))
			use_usb3 = true;

		ehci_node = of_find_compatible_node(NULL, NULL,
						"marvell,orion-ehci");

		if (ehci_node && of_device_is_available(ehci_node)
			&& use_usb3) {
			/*
			 * We can't use usb2 and usb3 in the same time, so let's
			 * disbale usb2 and complain about it to the user askinf
			 * to fix the device tree.
			 */

			ehci_status = kzalloc(sizeof(struct property),
					GFP_KERNEL);
			WARN_ON(!ehci_status);

			ehci_status->value = kstrdup("disabled", GFP_KERNEL);
			WARN_ON(!ehci_status->value);

			ehci_status->length = 8;
			ehci_status->name = kstrdup("status", GFP_KERNEL);
			WARN_ON(!ehci_status->name);

			of_update_property(ehci_node, ehci_status);
			pr_err("%s: xhci-armada-375 and orion-ehci are incompatible for this SoC.\n",
				__func__);
			pr_err("Please fix your dts!\n");
			pr_err("orion-ehci have been disabled by default...\n");

		}

		reg = readl(usb_cluster_base);
		if (use_usb3)
			reg |= USB2_PHY_CONFIG_ENABLE;
		else
			reg &= ~USB2_PHY_CONFIG_ENABLE;
		writel(reg, usb_cluster_base);

		of_node_put(ehci_node);
		of_node_put(xhci_node);
		of_node_put(np);
		iounmap(usb_cluster_base);
	}

	return 0;
}
postcore_initcall(mvebu_usb_cluster_init);
