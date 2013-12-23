/*
 * USB UTMI support for Armada 38x platform.
 *
 * Copyright (C) 2013 Marvell
 *
 * Gregory CLEMENT <gregory.clement@free-electrons.com>
 *
 * This file is licensed under the terms of the GNU General Public
 * License version 2.  This program is licensed "as is" without any
 * warranty of any kind, whether express or implied.
 *
 */

#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/of_address.h>
#include <linux/io.h>
#include <linux/slab.h>

#define USB_UTMI_PHY_CTRL_STATUS(i)	(0x20+i*4)
#define		USB_UTMI_TX_BITSTUFF_EN BIT(1)
#define		USB_UTMI_PU_PHY			BIT(5)
#define		USB_UTMI_VBUS_ON_PHY	BIT(6)

static struct of_device_id of_usb_utmi_table[] = {
	{ .compatible = "marvell,armada-380-usb-utmi", },
	{ /* end of list */ },
};

static int __init mvebu_usb_utmi_init(void)
{
	struct device_node *np;

	np = of_find_matching_node(NULL, of_usb_utmi_table);
	if (np) {
		void __iomem *usb_utmi_base, *utmi_base;

		usb_utmi_base = of_iomap(np, 0);
		BUG_ON(!usb_utmi_base);

		utmi_base = of_iomap(np, 1);
		BUG_ON(!utmi_base);

		writel(USB_UTMI_TX_BITSTUFF_EN |
			   USB_UTMI_PU_PHY |
			   USB_UTMI_VBUS_ON_PHY,
			   usb_utmi_base + USB_UTMI_PHY_CTRL_STATUS(0));

		/*
		 * Magic init... the registers and their value are not
		 * documented
		 */
		writel(0x40605205, utmi_base);
		writel(0x409, utmi_base + 4);
		writel(0x1be7f6f, utmi_base + 0xc);

		of_node_put(np);
		iounmap(utmi_base);
		iounmap(usb_utmi_base);
	}

	return 0;
}
postcore_initcall(mvebu_usb_utmi_init);
