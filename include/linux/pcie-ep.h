/*
 * Armada 8K PCIe EP
 * Copyright (c) 2016, Marvell Semiconductor.
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms and conditions of the GNU General Public License,
 * version 2, as published by the Free Software Foundation.
 *
 * This program is distributed in the hope it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 */
#ifndef _PCIE_EP_
#define _PCIE_EP_

#include <linux/msi.h>

struct pci_epf_header {
	u16	vendor_id;
	u16	device_id;
	u16	subsys_vendor_id;
	u16	subsys_id;
	u8	rev_id;
	u8	progif_code;
	u8	subclass_code;
	u8	baseclass_code;
	u8	cache_line_size;
	u8	io_en;
	u8	mem_en;
};


/* BAR bitmaps for use with a8k_pcie_ep_disable_bars */
#define PCIE_EP_BAR0		BIT(0)
#define PCIE_EP_BAR1		BIT(1)
#define PCIE_EP_BAR0_64		(PCIE_EP_BAR0 | PCIE_EP_BAR1)
#define PCIE_EP_BAR2		BIT(2)
#define PCIE_EP_BAR3		BIT(3)
#define PCIE_EP_BAR2_64		(PCIE_EP_BAR3 | PCIE_EP_BAR2)
#define PCIE_EP_BAR4		BIT(4)
#define PCIE_EP_BAR5		BIT(5)
#define PCIE_EP_BAR4_64		(PCIE_EP_BAR4 | PCIE_EP_BAR5)
#define PCIE_EP_BAR_ROM		BIT(8) /* matches the offset, see pci.c */
#define PCIE_EP_ALL_BARS	((BIT(9) - 1) & ~(BIT(6) || BIT(7)))

void a8k_pcie_ep_bar_map(void *ep, u32 func_id, int bar, phys_addr_t addr, u64 size);
void a8k_pcie_ep_setup_bar(void *ep, int func_id, u32 bar_num, u32 props, u64 sz);
void a8k_pcie_ep_disable_bars(void *ep, int func_id, u16 mask);
void a8k_pcie_ep_cfg_enable(void *ep, int func_id);
int  a8k_pcie_ep_get_msi(void *ep, int func_id, int vec_id, struct msi_msg *msg);
int  a8k_pcie_ep_remap_host(void *ep, u32 func_id, u64 local_base, u64 host_base, u64 size);
void a8k_pcie_ep_write_header(void *ep, int func_id, struct pci_epf_header *hdr);
void *a8k_pcie_ep_get(void);

#endif
