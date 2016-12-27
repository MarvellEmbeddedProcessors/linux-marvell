/*
 * Copyright (C) 2015-2018 Marvell International Ltd.

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

#include <linux/mv_soc_info.h>
#include <linux/of_address.h>

int soc_revision_id = -1;

/* Get SoC revision ID */
int mv_soc_info_get_ap_revision(void)
{
	void __iomem *ap_rev_info_reg;
	const unsigned int *reg;
	struct device_node *node;
	phys_addr_t paddr;

	/* if soc_revision_id was already initialized,
	 * no need to read it again
	 */
	if (soc_revision_id != -1)
		return soc_revision_id;

	/* TODO add support for CP110, CP115, AP807 and AP810 revisions */
	node = of_find_compatible_node(NULL, NULL, "marvell,ap806-rev-info");
	if (!node) {
		pr_err("Read soc-rev-info node failed\n");
		goto free_node;
	}

	/* Read the offset of the register */
	reg = of_get_property(node, "reg", NULL);
	if (!reg) {
		pr_err("Read reg property failed from soc-rev-info node\n");
		goto free_node;
	}

	/* Translate the offset to physical address */
	paddr = of_translate_address(node, reg);
	if (paddr == OF_BAD_ADDR) {
		pr_err("of_translate_address failed for soc-rev-info\n");
		goto free_node;
	}

	ap_rev_info_reg = ioremap(paddr, reg[1]);
	if (!ap_rev_info_reg) {
		pr_err("ioremap() failed for soc-rev-info register\n");
		goto free_node;
	}

	/* read gwd_iidr2 register and set the revision global*/
	soc_revision_id = (readl(ap_rev_info_reg) >>
			   GWD_IIDR2_REV_ID_OFFSET) &
			   GWD_IIDR2_REV_ID_MASK;

	pr_debug("%s: Detected SoC revision A%d\n",
		 __func__, soc_revision_id == APN806_REV_ID_A0 ? 0 : 1);

	/* Release resources */
	iounmap(ap_rev_info_reg);
	of_node_put(node);

	return soc_revision_id;

free_node:
	of_node_put(node);
	return -EINVAL;
}
