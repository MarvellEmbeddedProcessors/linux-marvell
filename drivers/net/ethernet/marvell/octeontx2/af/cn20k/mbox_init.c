// SPDX-License-Identifier: GPL-2.0
/* Marvell RVU Admin Function driver
 *
 * Copyright (C) 2024 Marvell.
 *
 */

#include <linux/interrupt.h>
#include <linux/irq.h>

#include "rvu_trace.h"
#include "mbox.h"
#include "reg.h"
#include "api.h"

int cn20k_rvu_get_mbox_regions(struct rvu *rvu, void **mbox_addr,
			       int num, int type, unsigned long *pf_bmap)
{
	int region;
	u64 bar;

	for (region = 0; region < num; region++) {
		if (!test_bit(region, pf_bmap))
			continue;

		bar = (u64)phys_to_virt((u64)rvu->ng_rvu->pf_mbox_addr->base);
		bar += region * MBOX_SIZE;

		mbox_addr[region] = (void *)bar;

		if (!mbox_addr[region])
			return -ENOMEM;
	}
	return 0;
}

int cn20k_rvu_mbox_init(struct rvu *rvu, int type, int ndevs)
{
	int dev;

	if (!is_cn20k(rvu->pdev))
		return 0;

	for (dev = 0; dev < ndevs; dev++)
		rvu_write64(rvu, BLKADDR_RVUM,
			    RVU_MBOX_AF_PFX_CFG(dev), ilog2(MBOX_SIZE));

	return 0;
}
