// SPDX-License-Identifier: GPL-2.0
/* Marvell CPRI GPINT handling
 *
 * Copyright (C) 2025 Marvell International Ltd.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */
#include <linux/io.h>
#include <linux/kernel.h>
#include <linux/interrupt.h>
#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/of.h>
#include <linux/of_device.h>
#include <linux/of_irq.h>
#include <linux/platform_device.h>
#include <linux/pci.h>

#define CPRI_IP_AXI_INT_STATUS(mhab) \
	(0x100ULL | (((mhab) & 0x7) << 20))

#define CPRI_IP_AXI_INT(mhab) \
	(0x108ULL | (((mhab) & 0x7) << 20))

#define PSM_CNF20K_INT_GP_SUM_W1C(a)	(0xE0000 + (a) * 0x100)

#define MAX_NUM_CHIPLETS		2
#define CPRI_INT_MASK			0x3F

#define RVU_FUNC_BLKADDR_SHIFT          20
#define CNF20K_FUNC_BLKADDR_MASK        0xFFULL
#define BPHY_BLKADDR_MASK		0x3F

typedef int (*connip_irq_cb_t)(u32 instance, u32 pss_int);

struct mrvl_cpri_hwcfg {
	u8 num_chiplets;
	u8 max_mhabs;	/* per chiplet */
};

struct mrvl_cpri_gpint_ctx {
	struct pci_dev *pdev;
	struct msix_entry msix_ent;
	void __iomem *cpri_regbase;
	u8 num_chiplets;
	u8 max_mhabs;
	int irq[MAX_NUM_CHIPLETS];
	connip_irq_cb_t irq_cb;
};

struct mrvl_cpri_gpint_ctx *g_drv_ctx;

static u64 cpri_read_reg(struct mrvl_cpri_gpint_ctx *ml, int chiplet_id, u64 offset)
{
	u64 blkaddr;

	switch (chiplet_id) {
	case 0:
		blkaddr = 0x20;
		break;
	case 1:
		blkaddr = 0x28;
		break;
	default:
		break;
	}
	offset &= ~(BPHY_BLKADDR_MASK << RVU_FUNC_BLKADDR_SHIFT);
	offset |= (blkaddr << RVU_FUNC_BLKADDR_SHIFT);

	return readq(ml->cpri_regbase + offset);
}

static void cpri_write_reg(struct mrvl_cpri_gpint_ctx *ml, int chiplet_id, u64 offset, u64 val)
{
	u64 blkaddr;

	switch (chiplet_id) {
	case 0:
		blkaddr = 0x20;
		break;
	case 1:
		blkaddr = 0x28;
		break;
	default:
		break;
	}
	offset &= ~(BPHY_BLKADDR_MASK << RVU_FUNC_BLKADDR_SHIFT);
	offset |= (blkaddr << RVU_FUNC_BLKADDR_SHIFT);

	writeq(val, ml->cpri_regbase + offset);
}

int mrvl_cpri_gpint_create_dev(phys_addr_t regbase, size_t size,
			       int irq0, int irq1,
			       u32 num_chiplets)
{
	struct platform_device_info pinfo;
	struct platform_device *pdev;
	struct mrvl_cpri_hwcfg pdata;
	struct resource res[3];

	memset(res, 0, sizeof(res));

	/* CPRI MEM region */
	res[0].start = regbase;
	res[0].end   = regbase + size - 1;
	res[0].flags = IORESOURCE_MEM;

	/* BPHY chiplet 0, GPINT0 */
	res[1].start = irq0;
	res[1].end   = irq0;
	res[1].flags = IORESOURCE_IRQ;

	/* BPHY chiplet 1, GPINT0 */
	res[2].start = irq1;
	res[2].end   = irq1;
	res[2].flags = IORESOURCE_IRQ;

	pdata.num_chiplets = num_chiplets;
	if (num_chiplets > 1)
		pdata.max_mhabs = 3;
	else
		pdata.max_mhabs = 5;

	memset(&pinfo, 0, sizeof(pinfo));
	pinfo.name = "mrvl-cpri-gpint";
	pinfo.id = PLATFORM_DEVID_AUTO;
	pinfo.res = res;
	pinfo.num_res = ARRAY_SIZE(res);
	pinfo.data = &pdata;
	pinfo.size_data = sizeof(pdata);

	pdev = platform_device_register_full(&pinfo);
	if (IS_ERR(pdev)) {
		pr_err("Failed to create mrvl-cpri-gpint device\n");
		return PTR_ERR(pdev);
	}

	return 0;
}
EXPORT_SYMBOL(mrvl_cpri_gpint_create_dev);

int mrvl_cpri_gpint_register_irq_cb(connip_irq_cb_t func)
{
	if (!g_drv_ctx) {
		pr_err("Driver data not available, probe failed\n");
		return -ENOENT;
	}

	if (!func)
		return -EIO;

	g_drv_ctx->irq_cb = func;

	return 0;
}
EXPORT_SYMBOL(mrvl_cpri_gpint_register_irq_cb);

void mrvl_cpri_gpint_unregister_irq_cb(void)
{
	if (!g_drv_ctx) {
		pr_err("Driver data not available, probe failed\n");
		return;
	}

	g_drv_ctx->irq_cb = NULL;
}
EXPORT_SYMBOL(mrvl_cpri_gpint_unregister_irq_cb);

static irqreturn_t mrvl_cpri_gpint_handler(int irq, void *dev)
{
	struct mrvl_cpri_gpint_ctx *ml =
		platform_get_drvdata((struct platform_device *)dev);
	u32 instance, pss_int;
	u64 gpint_regoff, regval;
	int ret, chiplet_id;
	u8 num_mhabs;

	if (irq == ml->irq[0])
		chiplet_id = 0;
	else
		chiplet_id = 1;

	/* GPINT0 offset */
	gpint_regoff = PSM_CNF20K_INT_GP_SUM_W1C(0);

	/* Clear GPINT status */
	regval = cpri_read_reg(ml, chiplet_id, gpint_regoff);
	regval &= 0xFFFFFFFF;
	cpri_write_reg(ml, chiplet_id, gpint_regoff, regval);

	num_mhabs = ml->num_chiplets * ml->max_mhabs;

	for (instance = 0; instance < num_mhabs; instance++) {
		if (!(regval & (1 << instance)))
			continue;
		pss_int = (u32)cpri_read_reg(ml, chiplet_id, CPRI_IP_AXI_INT_STATUS(instance));
		if (ml->irq_cb) {
			ret = ml->irq_cb(instance, pss_int);
			if (ret < 0)
				dev_err(dev,
					"Error %d from CPRI IRQ callback\n",
					ret);
		}
		/* clear AXI_INT */
		cpri_write_reg(ml, chiplet_id, CPRI_IP_AXI_INT_STATUS(instance), pss_int);
	}

	return IRQ_HANDLED;
}

static int mrvl_cpri_gpint_probe(struct platform_device *pdev)
{
	struct mrvl_cpri_hwcfg *pdata = dev_get_platdata(&pdev->dev);
	struct device *dev = &pdev->dev;
	struct mrvl_cpri_gpint_ctx *ml;
	int num = pdata->num_chiplets;
	int ret = 0, irq0, irq1;
	struct resource *res;

	if (num == 0 || num > MAX_NUM_CHIPLETS) {
		dev_err(dev, "Invalid number of chiplets %d\n", num);
		return -EINVAL;
	}

	ml = devm_kzalloc(dev, sizeof(*ml), GFP_KERNEL);
	if (!ml)
		return -ENOMEM;

	platform_set_drvdata(pdev, ml);

	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	ml->cpri_regbase = ioremap(res->start, resource_size(res));
	if (IS_ERR(ml->cpri_regbase)) {
		dev_err(dev, "Error: ioremap CPRI registers\n");
		ret = PTR_ERR(ml->cpri_regbase);
		goto err_free;
	}

	irq0 = platform_get_irq(pdev, 0);
	if (irq0 < 0) {
		ret = irq0;
		goto err_unmap;
	}

	ml->irq[0] = irq0;

	ret = request_irq(ml->irq[0], mrvl_cpri_gpint_handler, 0,
			  "mrvl bphy cplt0 gpint", pdev);
	if (ret) {
		dev_err(dev, "failed to register irq handler\n");
		goto err_unmap;
	}

	if (num > 1) {
		irq1 = platform_get_irq(pdev, 1);
		if (irq1 < 0) {
			ret = irq1;
			goto err_irq0;
		}

		ml->irq[1] = irq1;

		ret = request_irq(ml->irq[1], mrvl_cpri_gpint_handler, 0,
				  "mrvl bphy cplt1 gpint", pdev);
		if (ret) {
			dev_err(dev, "failed to register irq handler\n");
			goto err_irq0;
		}
	}

	ml->num_chiplets = pdata->num_chiplets;
	ml->max_mhabs = pdata->max_mhabs;

	g_drv_ctx = ml;

	return 0;

err_irq0:
	free_irq(ml->irq[0], pdev);
err_unmap:
	iounmap(ml->cpri_regbase);
err_free:
	devm_kfree(&pdev->dev, ml);
	return ret;
}

static int mrvl_cpri_gpint_remove(struct platform_device *pdev)
{
	struct mrvl_cpri_gpint_ctx *ml = platform_get_drvdata(pdev);
	int i;

	if (!ml)
		return 0;

	/* Free IRQs */
	for (i = 0; i < ml->num_chiplets; i++) {
		if (ml->irq[i] > 0)
			free_irq(ml->irq[i], pdev);
	}

	/* Unmap registers */
	if (ml->cpri_regbase)
		iounmap(ml->cpri_regbase);

	devm_kfree(&pdev->dev, ml);

	return 0;
}

static struct platform_driver mrvl_cpri_gpint_driver = {
	.probe = mrvl_cpri_gpint_probe,
	.remove = mrvl_cpri_gpint_remove,
	.driver = {
		.name = "mrvl-cpri-gpint",
	},
};

module_platform_driver(mrvl_cpri_gpint_driver);

MODULE_DESCRIPTION("Marvell CPRI GPINT Driver");
MODULE_AUTHOR("Marvell International Ltd.");
MODULE_LICENSE("GPL");
