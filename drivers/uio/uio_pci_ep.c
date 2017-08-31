/*
 * Copyright (C) 2017 Marvell International Ltd.
 *
 * UIO PCIe end point driver
 *
 * This driver exposes PCI EP resources to user-space
 * It is currently coupled to a8k PCI EP driver but it
 * in the future it will use the standard PCI EP stack
 *
 * This program is free software: you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation, either version 2 of the
 * License, or any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 */

#include <linux/module.h>
#include <linux/device.h>
#include <linux/string.h>
#include <linux/slab.h>
#include <linux/platform_device.h>
#include <linux/uio_driver.h>
#include <linux/of_address.h>
#include <linux/of_irq.h>
#include <linux/pcie-ep.h>
#include <linux/pci_ids.h>
#include <linux/pci_regs.h>
#include <linux/mm.h>

struct uio_pci {
	struct device	*dev;
	void		*ep;
	struct resource	*host_map;
};

/* make sure we have at least one mem regions to map the host ram */
#define MAX_BAR_MAP	(MAX_UIO_MAPS > 6 ? 6 : MAX_UIO_MAPS - 1)

static int uio_pci_ep_probe(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	struct device_node *node = dev->of_node;
	void *ep;
	struct uio_pci *uio_pci;
	struct pci_epf_header hdr;
	struct resource *res;
	struct uio_info *info;
	struct uio_mem *mem;
	char *name;
	struct page *pg;
	int    bar_id, mem_id, ret;
	u16 bar_mask = 0;
	u32 val;

	info = devm_kzalloc(dev, sizeof(*info), GFP_KERNEL);
	if (!info)
		return -ENOMEM;

	uio_pci = devm_kzalloc(dev, sizeof(*uio_pci), GFP_KERNEL);
	if (!uio_pci)
		return -ENOMEM;

	/* connect the objects */
	platform_set_drvdata(pdev, info);
	info->priv = uio_pci;
	uio_pci->dev = dev;

	/* store private data */
	info->name = "uio-pci-ep";
	info->version = "1.0.0";

	/* setup the PCI EP topology. This should eventually move to the PCI EP Function driver */
	uio_pci->ep = ep = a8k_pcie_ep_get();
	if (!ep) {
		dev_err(dev, "Failed to find PCI EP driver\n");
		return -EPROBE_DEFER;
	}

	/* Configure the EP PCIe header */
	memset(&hdr, 0, sizeof(hdr));
	hdr.vendor_id = PCI_VENDOR_ID_MARVELL;
	hdr.mem_en = 1;

	ret = of_property_read_u32(node, "device-id", &val);
	if (ret) {
		dev_err(dev, "missing device-id from DT node\n");
		return ret;
	}
	hdr.device_id = val;

	ret = of_property_read_u32(node, "class-code", &val);
	if (ret) {
		dev_err(dev, "missing class-code from DT node\n");
		return ret;
	}
	hdr.baseclass_code = val;

	ret = of_property_read_u32(node, "subclass-code", &val);
	if (ret) {
		dev_err(dev, "missing subclass-code from DT node\n");
		return ret;
	}
	hdr.subclass_code = val;

	a8k_pcie_ep_write_header(ep, 0, &hdr);

	/* Setup the BARs according to device tree */
	for (bar_id = 0, mem_id = 0; bar_id < MAX_BAR_MAP; bar_id++) {
		name = devm_kzalloc(dev, 6 * sizeof(char), GFP_KERNEL);
		if (!name) {
			dev_err(dev, "Failed to allocate UIO mem name\n");
			return -ENOMEM;
		}

		snprintf(name, 5, "bar%d", bar_id);
		res = platform_get_resource_byname(pdev, IORESOURCE_MEM, name);
		if (!res) {
			kfree(name);
			continue;
		}

		bar_mask |= 1 << bar_id;

		/* Setup the UIO memory attributes */
		mem = &info->mem[mem_id];
		mem->memtype = UIO_MEM_PHYS;
		mem->size = resource_size(res);
		mem->name = name;

		if (!is_power_of_2(mem->size)) {
			dev_err(dev, "BAR-%d size in not power of 2\n", bar_id);
			return -EINVAL;
		}

		/* NULL means allocate RAM backup */
		if (!res->start) {
			pg = alloc_pages(GFP_KERNEL | __GFP_ZERO, get_order(mem->size));
			if (!pg) {
				dev_err(dev, "Failed to allocate RAM for resource %pR\n", res);
				return -ENOMEM;
			}
			mem->internal_addr = page_address(pg);
			mem->addr = virt_to_phys(mem->internal_addr);
		} else {
			mem->internal_addr = devm_ioremap_resource(dev, res);
			if (!mem->internal_addr) {
				dev_err(dev, "Failed to map resource %pR\n", res);
				return -ENODEV;
			}
			mem->addr = res->start;
		}

		/* Now create the BAR to match the memory region */
		a8k_pcie_ep_setup_bar(ep, 0, bar_id, PCI_BASE_ADDRESS_SPACE_MEMORY | PCI_BASE_ADDRESS_MEM_TYPE_32,
				      mem->size);
		a8k_pcie_ep_bar_map(ep, 0, bar_id, (phys_addr_t)mem->addr, mem->size);

		/* First 2 BARs in HW are 64 bit BARs and consume 2 BAR slots */
		if (bar_id < 4) {
			bar_id++;
			bar_mask |= 1 << bar_id;
		}
		mem_id++;
	}

	a8k_pcie_ep_disable_bars(ep, 0, ~bar_mask);

	/* remap host RAM to local memory space  using shift mapping.
	 * i.e. address 0x0 in host becomes uio_pci->host_map->start.
	 * Additionally map the host physical space to the virtual memory using ioremap.
	 */
	uio_pci->host_map = platform_get_resource_byname(pdev, IORESOURCE_MEM, "host-map");
	if (!uio_pci->host_map) {
		dev_err(dev, "Device tree missing host mappings\n");
		return -ENODEV;
	}
	a8k_pcie_ep_remap_host(ep, 0, uio_pci->host_map->start, 0x0, resource_size(uio_pci->host_map));

	/* Describe the host as a UIO space */
	name = devm_kzalloc(dev, 16 * sizeof(char), GFP_KERNEL);
	if (!name)
		return -ENOMEM;

	snprintf(name, 16, "host-map");
	mem = &info->mem[mem_id];
	mem->memtype = UIO_MEM_PHYS;
	mem->size = resource_size(uio_pci->host_map);
	mem->name = name;
	mem->addr = uio_pci->host_map->start;
	mem->internal_addr = devm_ioremap_resource(dev, uio_pci->host_map);
	if (IS_ERR(mem->internal_addr)) {
		dev_err(dev, "Failed to map host memory %pR\n", uio_pci->host_map);
		return -ENODEV;
	}

	/* register the UIO device */
	if (uio_register_device(dev, info) != 0) {
		dev_err(dev, "UIO registration failed\n");
		return -ENODEV;
	}

	/* Finally, enable the PCIe host to detect us */
	a8k_pcie_ep_cfg_enable(ep, 0);

	dev_info(dev, "Registered UIO PCI EP successfully\n");

	return 0;
}

static int uio_pci_ep_remove(struct platform_device *pdev)
{
	struct uio_info *info = platform_get_drvdata(pdev);

	uio_unregister_device(info);
	return 0;
}

static const struct of_device_id uio_pci_ep_match[] = {
	{ .compatible = "marvell,pci-ep-uio", },
	{}
};
MODULE_DEVICE_TABLE(of, uio_pci_ep_match);

static struct platform_driver uio_pci_ep_driver = {
	.driver = {
		.name = "marvell,pci-ep-uio",
		.owner = THIS_MODULE,
		.of_match_table = uio_pci_ep_match,
	},
	.probe = uio_pci_ep_probe,
	.remove = uio_pci_ep_remove,
};
module_platform_driver(uio_pci_ep_driver);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Yehuda Yitschak <yehuday@marvell.com>");
MODULE_DESCRIPTION("PCI EP Function UIO driver");
