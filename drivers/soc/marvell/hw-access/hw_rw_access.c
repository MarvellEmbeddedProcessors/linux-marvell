// SPDX-License-Identifier: GPL-2.0
/* Hardware device CSR Access driver
 * Copyright (C) 2021 Marvell International Ltd.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

/* This driver supports Read/Write of only OcteonTx2/OcteonTx3 HW device
 * config registers. Read/Write of System Registers are not supported.
 */

#include <linux/device.h>
#include <linux/fs.h>
#include <linux/io.h>
#include <linux/mm.h>
#include <linux/module.h>
#include <linux/overflow.h>
#include <linux/log2.h>
#include <linux/uaccess.h>
#include <linux/pci.h>
#include <linux/stddef.h>
#include <linux/debugfs.h>
#include <linux/arm-smccc.h>
#include <soc/marvell/octeontx/octeontx_smc.h>

#include "rvu_struct.h"
#include "rvu.h"
#include "mbox.h"
#include "lmac_common.h"

#define OCTEONTX_ACCESS_REG_SMCID 0xc2000fff

#define READ 0
#define WRITE 1

static bool size_32 = true;
static u64 reg_addr;
static int filevalue;
struct dentry *dirret;

#define DEVICE_NAME			"hw_access"
#define CLASS_NAME			"hw_access_class"

/* PCI device IDs */
#define	PCI_DEVID_OCTEONTX2_RVU_AF	0xA065

struct hw_reg_cfg {
	u64	regaddr; /* Register physical address within a hw device */
	u64	regval; /* Register value to be read or to write */
};

struct hw_ctx_cfg {
	u16	blkaddr;
	u16	pcifunc;
	union {
		u16	qidx;
		u16	aura;
	};
	u8	ctype;
	u8	op;
};

struct hw_cgx_info {
	u8	pf;
	u8	cgx_id;
	u8	lmac_id;
	u8	nix_idx;
};

struct hw_link_info {
	u8 cgx_id;
	u8 lmac_id;
	struct cgx_link_user_info link_info;
};

struct hw_csr_mapping {
	void __iomem *reg_base;
	bool mapped;
};

struct hw_priv_data {
	struct hw_csr_mapping *map;
	u32 total_mappings;
	struct rvu *rvu;
	struct pci_dev *pdev;
};

struct hw_csr_lookup_tbl {
	u64 base; /* Base BAR address for each HW */
	u64 size; /* Per-instance ioremap size; must equal mask + 1 and be a power of 2 */
	u8 alpha; /* Outer instance count (e.g. PF for RVU, LMAC for CGX/RPM) */
	u8 alpha_shift; /* Bit shift for alpha index; 1 << alpha_shift must be >= size */
	u16 beta; /* Inner instance count (e.g. FUNC for RVU, VF for DPI) */
	u8 beta_shift; /* Bit shift for beta index; 1 << beta_shift must be >= size */
	u64 mask; /* Low-bit mask for extracting in-instance CSR offset */
};

static const struct hw_csr_lookup_tbl lkp_tbl_cn9x[] = {
	/* [BASE] [SIZE] [ALPHA] [ALPHA SHIFT] [BETA] [BETA SHIFT] [MASK] */
	/*
	 * RVU_PF(0..31)_BAR0 : per-PF stride is 1<<36 (64 GB); each PF hosts
	 * many 256 MB "block slots" (RVUM, LMT, NPA, NIX0/1, NPC, SSO, SSOW,
	 * TIM, CPTX, NDCX, REEX, APR; see RVU_BLOCK_ADDR_E). Block 0x16 (APR)
	 * sits at +5.5 GB, so we expose the first 8 GB (next power of 2) of
	 * each PF's register window.
	 */
	{ 0x840000000000, 0x200000000, 32, 36, 1, 0, 0x1FFFFFFFF },
	/*
	 * RVU_PF(0..31)_FUNC(0..128)_BAR2 : 32 MB per FUNC
	 * (HRM pcc_bar_size_bits=25). Per-FUNC layout is
	 * block_addr[24:20] | slot[19:12] | reg[11:0]; FUNC stride is 1<<25,
	 * so size fills the FUNC window exactly.
	 */
	{ 0x840200000000, 0x2000000, 32, 36, 129, 25, 0x1FFFFFF },
	/*
	 * DPI(0..1)_PF_BAR0 : 4 GB per DPI (HRM pcc_bar_size_bits=32). Hosts
	 * DPI engine CSRs in the low MB plus SDP counters / VDMA shadow
	 * registers at multi-GB offsets.
	 */
	{ 0x86E000000000, 0x100000000, 2, 36, 1, 0, 0xFFFFFFFF },
	/* DPI(0..1)_VF(0..7)_BAR0 : 8 VFs per DPI, 1 MB each */
	{ 0x86E200000000, 0x100000, 2, 36, 8, 20, 0xFFFFF },
	/* RST_PF_BAR0 */
	{ 0x87E006000000, 0x10000, 1, 0, 1, 0, 0xFFFF },
	/* CGX(0..4)_PF_BAR0 : 5 on CN98XX, 3 on CN96XX; BAR0 = 1 MB */
	{ 0x87E0E0000000, 0x100000, 5, 24, 1, 0, 0xFFFFF },
	/* IOBN(0..2)_PF_BAR0 : 3 on CN98XX, 2 on CN96XX */
	{ 0x87E0F0000000, 0x100000, 3, 24, 1, 0, 0xFFFFF },
	/* LMC(0..5)_PF_BAR0 : 6 on CN98XX, 3 on CN96XX; BAR0 = 8 MB */
	{ 0x87E088000000, 0x800000, 6, 24, 1, 0, 0x7FFFFF },
	/* OCLA(0..6)_PF_BAR0 : 7 on CN98XX, 5 on CN96XX; BAR0 = 8 MB */
	{ 0x87E0B0000000, 0x800000, 7, 24, 1, 0, 0x7FFFFF },
	/* DTX RSL window : 16 MB flat */
	{ 0x87E0FE000000, 0x1000000, 1, 0, 1, 0, 0xFFFFFF },
};

static const struct hw_csr_lookup_tbl lkp_tbl_cn10k[] = {
	/* RVU_PF(0..31)_BAR0 : see CN9 row for block-slot layout rationale */
	{ 0x840000000000, 0x200000000, 32, 36, 1, 0, 0x1FFFFFFFF },
	/* RVU_PF(0..31)_FUNC(0..128)_BAR2 : see CN9 row, 32 MB per FUNC */
	{ 0x840200000000, 0x2000000, 32, 36, 129, 25, 0x1FFFFFF },
	/* DPI(0)_PF_BAR0 : 4 GB (HRM pcc_bar_size_bits=32); see CN9 row */
	{ 0x86E000000000, 0x100000000, 1, 36, 1, 0, 0xFFFFFFFF },
	/* DPI(0)_VF(0..31)_BAR0 : 32 VFs */
	{ 0x86E200000000, 0x100000, 1, 36, 32, 20, 0xFFFFF },
	/* RST_PF_BAR0 */
	{ 0x87E006000000, 0x10000, 1, 0, 1, 0, 0xFFFF },
	/* TAD_CMN_PF_BAR0 : single instance, 64 KB */
	{ 0x87E053000000, 0x10000, 1, 0, 1, 0, 0xFFFF },
	/* MCS(0..7)_PF_BAR0 : BAR0 is 15 MB; expose low 8 MB (power of 2) */
	{ 0x87E080000000, 0x800000, 8, 24, 1, 0, 0x7FFFFF },
	/* RPM(0..2)_PF_BAR0 : BAR0 = 8 MB */
	{ 0x87E0E0000000, 0x800000, 3, 24, 1, 0, 0x7FFFFF },
	/* IOBN(0..2)_PF_BAR0 : moved from 0x87E0F0 on CN9 to 0x87E120 on CN10K */
	{ 0x87E120000000, 0x100000, 3, 24, 1, 0, 0xFFFFF },
	/* NCB(0..4)_PF_BAR0 : CN10K-only block */
	{ 0x87E140000000, 0x100000, 5, 24, 1, 0, 0xFFFFF },
	/* DSS(0..5)_PF_BAR0 : CN10K rename of LMC; BAR0 = 4 MB */
	{ 0x87E1C0000000, 0x400000, 6, 24, 1, 0, 0x3FFFFF },
	/* TAD(0..47)_PF_BAR0 : 48 mesh TADs on CN10K, 8 MB each */
	{ 0x87E280000000, 0x800000, 48, 24, 1, 0, 0x7FFFFF },
	/* DTX RSL window : 16 MB flat, shared address with CN9 */
	{ 0x87E0FE000000, 0x1000000, 1, 0, 1, 0, 0xFFFFFF },
	/*
	 * OCLA(0..23,64)_PF_BAR0 : 24 dense + 1 sparse at index 64; alpha=65
	 * covers index 64. Indices 24..63 are reserved on silicon and will not
	 * match any valid user address (benign dead branches in the loop).
	 */
	{ 0x87E380000000, 0x800000, 65, 24, 1, 0, 0x7FFFFF },
};

/*
 * CN20K relocated the RVU PCIe apertures. Verified on silicon from the
 * kernel PCI BAR assignments:
 *
 *   AF BAR0 : 0x600400000000-0x6007ffffffff (16 GB)  <- was 0x840000000000
 *   AF BAR2 : 0x600200000000-0x600203ffffff (64 MB)  <- was 0x840200000000
 *
 * CN9K/CN10K gave every RVU_PF a fixed 64 GB BAR0 window and kept BAR2 in
 * one contiguous pf<<36|func<<25 aperture. On CN20K only the AF exposes a
 * BAR0 (hosting every block's AF register bank via the block_addr<<28 slot
 * layout) and each PF/VF BAR2 is a standalone 64 MB window placed by PCIe
 * enumeration. The block-slot layout inside BAR0 is unchanged, so AF and
 * RVU_PRIV_* access (device discovery, NIX_AF/NPA_AF/NPC_AF) only needs the
 * new base; the per-PF/VF BAR2 windows are discovered at runtime by
 * build_cn20k_csr_tbl().
 *
 * The non-RVU rows below are inherited from CN10K: the RSL/NCB blocks (RST,
 * MCS, RPM, IOBN, NCB, DSS, TAD, OCLA, DTX) sit at architecturally fixed
 * 0x87E0_xxxx_xxxx addresses that are stable across generations.
 */
static const struct hw_csr_lookup_tbl lkp_tbl_cn20k[] = {
	/* RVU AF BAR0 : map the low 8 GB of the 16 GB block_addr<<28 window */
	{ 0x600400000000, 0x200000000, 1, 0, 1, 0, 0x1FFFFFFFF },
	/*
	 * RVU per-PF/VF BAR2 : 64 MB placeholder (index 1), skipped and
	 * replaced at first open() by build_cn20k_csr_tbl() with the real
	 * PCIe-assigned windows. The 64 MB window (mask 0x3FFFFFF) holds the
	 * LF block_addr[24:20]|slot[19:12]|reg[11:0] decode plus the MSI-X
	 * table and mailbox alias.
	 */
	{ 0x600800000000, 0x4000000, 32, 26, 1, 0, 0x3FFFFFF },
	/* DPI(0)_PF_BAR0 : inherited CN10K standalone DPI aperture */
	{ 0x86E000000000, 0x100000000, 1, 36, 1, 0, 0xFFFFFFFF },
	/* DPI(0)_VF(0..31)_BAR0 */
	{ 0x86E200000000, 0x100000, 1, 36, 32, 20, 0xFFFFF },
	/* RST_PF_BAR0 : RSL bus, fixed address */
	{ 0x87E006000000, 0x10000, 1, 0, 1, 0, 0xFFFF },
	/* TAD_CMN_PF_BAR0 */
	{ 0x87E053000000, 0x10000, 1, 0, 1, 0, 0xFFFF },
	/* MCS(0..7)_PF_BAR0 */
	{ 0x87E080000000, 0x800000, 8, 24, 1, 0, 0x7FFFFF },
	/* RPM(0..2)_PF_BAR0 */
	{ 0x87E0E0000000, 0x800000, 3, 24, 1, 0, 0x7FFFFF },
	/* IOBN(0..2)_PF_BAR0 */
	{ 0x87E120000000, 0x100000, 3, 24, 1, 0, 0xFFFFF },
	/* NCB(0..4)_PF_BAR0 */
	{ 0x87E140000000, 0x100000, 5, 24, 1, 0, 0xFFFFF },
	/* DSS(0..5)_PF_BAR0 */
	{ 0x87E1C0000000, 0x400000, 6, 24, 1, 0, 0x3FFFFF },
	/* TAD(0..47)_PF_BAR0 */
	{ 0x87E280000000, 0x800000, 48, 24, 1, 0, 0x7FFFFF },
	/* DTX RSL window */
	{ 0x87E0FE000000, 0x1000000, 1, 0, 1, 0, 0xFFFFFF },
	/* OCLA(0..23,64)_PF_BAR0 */
	{ 0x87E380000000, 0x800000, 65, 24, 1, 0, 0x7FFFFF },
};

/*
 * All CSR lookup-table state, selected once at module init. The struct lives
 * in .bss (statically allocated); only .dyn (the CN20K rows) is heap-allocated.
 * CN9K/CN10K point .tbl at the static per-SoC table. CN20K has no fixed BAR2
 * formula, so .tbl is swapped to .dyn once it is built from the live PCIe BAR2
 * assignments at first open(); .cn20k_ready gates that build to run only once.
 * /dev/hw_access is opened once by a single agent, so no locking is needed.
 */
struct hw_csr_tbl_state {
	const struct hw_csr_lookup_tbl *tbl;	/* effective table (static or .dyn) */
	unsigned int len;
	unsigned int max_alpha;
	unsigned int max_beta;
	struct hw_csr_lookup_tbl *dyn;		/* CN20K heap table, else NULL */
	bool cn20k_ready;			/* CN20K table built? */
};

static struct hw_csr_tbl_state csr;

/* Upper bound on RVU PF/VF BAR2 windows discovered via PCI enumeration. */
#define MAX_RVU_BAR2_WINS	256

#define HW_ACCESS_TYPE			120

#define HW_ACCESS_CSR_READ_IOCTL	_IO(HW_ACCESS_TYPE, 1)
#define HW_ACCESS_CSR_WRITE_IOCTL	_IO(HW_ACCESS_TYPE, 2)
#define HW_ACCESS_CTX_READ_IOCTL	_IO(HW_ACCESS_TYPE, 3)
#define HW_ACCESS_CGX_INFO_IOCTL	_IO(HW_ACCESS_TYPE, 4)
#define HW_ACCESS_LINK_INFO_IOCTL	_IO(HW_ACCESS_TYPE, 5)

/*
 * Hard upper bounds kept in reach of a single page for the per-open map array.
 * Any new table entry must respect these (enforced by validate_csr_tbl()).
 */
#define MAX_ALPHA	128
#define MAX_BETA	129

static struct class *hw_reg_class;
static int major_no;

/*
 * Build the CN20K CSR table: keep the fixed rows from lkp_tbl_cn20k[] (AF BAR0,
 * DPI, RSL blocks), skip its placeholder RVU BAR2 row (index 1), and append one
 * window per enumerated Cavium RVU function using its PCIe-assigned BAR2. Each
 * window is alpha=beta=1 with mask=size-1.
 */
static int build_cn20k_csr_tbl(void)
{
	struct hw_csr_lookup_tbl *tbl;
	struct pci_dev *pdev = NULL;
	unsigned int i, n = 0;
	size_t cap;

	cap = (ARRAY_SIZE(lkp_tbl_cn20k) - 1) + MAX_RVU_BAR2_WINS;
	tbl = kcalloc(cap, sizeof(*tbl), GFP_KERNEL);
	if (!tbl)
		return -ENOMEM;

	/* Fixed-address rows: keep index 0 (AF BAR0) and 2.. (DPI/RSL). */
	tbl[n++] = lkp_tbl_cn20k[0];
	for (i = 2; i < ARRAY_SIZE(lkp_tbl_cn20k); i++)
		tbl[n++] = lkp_tbl_cn20k[i];

	/* Dynamic rows: one per RVU function BAR2 the OS enumerated. */
	while ((pdev = pci_get_device(PCI_VENDOR_ID_CAVIUM, PCI_ANY_ID, pdev))) {
		u64 base = pci_resource_start(pdev, 2);
		u64 size = pci_resource_len(pdev, 2);

		if (!base || !(pci_resource_flags(pdev, 2) & IORESOURCE_MEM))
			continue;
		if (!is_power_of_2(size) || (base & (size - 1))) {
			pr_warn("hw_access: skip %s BAR2 base=0x%llx size=0x%llx\n",
				pci_name(pdev), base, size);
			continue;
		}
		if (n >= cap) {
			pr_warn("hw_access: RVU BAR2 table full (%zu); some LFs unmapped\n",
				cap);
			pci_dev_put(pdev);
			break;
		}
		tbl[n].base        = base;
		tbl[n].size        = size;
		tbl[n].alpha       = 1;
		tbl[n].alpha_shift = 0;
		tbl[n].beta        = 1;
		tbl[n].beta_shift  = 0;
		tbl[n].mask        = size - 1;
		n++;
	}

	/* Refresh max_alpha/beta; the per-open map array is sized from these. */
	csr.max_alpha = 1;
	csr.max_beta  = 1;
	for (i = 0; i < n; i++) {
		if (tbl[i].alpha > csr.max_alpha)
			csr.max_alpha = tbl[i].alpha;
		if (tbl[i].beta > csr.max_beta)
			csr.max_beta = tbl[i].beta;
	}

	csr.dyn = tbl;
	csr.tbl = tbl;
	csr.len = n;
	pr_info("hw_access: CN20K table built (%u rows, %u dynamic BAR2)\n",
		n, n - (unsigned int)(ARRAY_SIZE(lkp_tbl_cn20k) - 1));
	return 0;
}

/*
 * Undo build_cn20k_csr_tbl(): free the dynamic table, re-arm the build and
 * restore the static rows so the next open()'s csr.tbl guard still holds.
 * Called on release() and on the open() build-failure path.
 */
static void cn20k_tbl_reset(void)
{
	kfree(csr.dyn);
	csr.dyn = NULL;
	csr.cn20k_ready = false;
	csr.tbl = lkp_tbl_cn20k;
	csr.len = ARRAY_SIZE(lkp_tbl_cn20k);
}

/*
 * Pick the CSR lookup table matching the running SoC family. Invoked once at
 * module init. Populates csr.tbl, csr.len and csr.max_alpha/max_beta.
 */
static int select_csr_tbl(void)
{
	const struct hw_csr_lookup_tbl *t;
	unsigned int n, i, ma = 0, mb = 0;

	if (is_soc_cn20kx()) {
		/*
		 * Only the fixed rows are known at init; the RVU BAR2 windows
		 * are read from PCI on first open() (build_cn20k_csr_tbl), so
		 * boot stays PCI-free.
		 */
		t = lkp_tbl_cn20k;
		n = ARRAY_SIZE(lkp_tbl_cn20k);
	} else if (is_soc_cn10kx()) {
		t = lkp_tbl_cn10k;
		n = ARRAY_SIZE(lkp_tbl_cn10k);
	} else if (is_soc_cn9x()) {
		t = lkp_tbl_cn9x;
		n = ARRAY_SIZE(lkp_tbl_cn9x);
	} else {
		pr_err("hw_access: unsupported SoC; driver handles CN9XXX, CN10K and CN20K only\n");
		return -ENODEV;
	}

	for (i = 0; i < n; i++) {
		if (t[i].alpha > ma)
			ma = t[i].alpha;
		if (t[i].beta > mb)
			mb = t[i].beta;
	}

	csr.tbl = t;
	csr.len = n;
	csr.max_alpha = ma;
	csr.max_beta = mb;
	return 0;
}

/*
 * Enforce the invariants the rest of the driver assumes:
 *   1. size == mask + 1  (so addr & mask is the in-instance offset)
 *   2. size is a power of 2 (so mask is a contiguous low-bit mask)
 *   3. size <= 1 << alpha_shift when alpha > 1 (no overlap with alpha bits)
 *   4. size <= 1 << beta_shift  when beta  > 1 (no overlap with beta  bits)
 *   5. alpha in [1, MAX_ALPHA], beta in [1, MAX_BETA]
 *   6. (base & mask) == 0 (base is aligned to the window)
 */
static int validate_csr_tbl(const struct hw_csr_lookup_tbl *t, unsigned int n)
{
	unsigned int i;

	for (i = 0; i < n; i++) {
		if (t[i].size == 0 || t[i].size != t[i].mask + 1 ||
		    (t[i].size & (t[i].size - 1)) != 0) {
			pr_err("hw_access: row %u size/mask inconsistent (size=0x%llx mask=0x%llx)\n",
			       i, t[i].size, t[i].mask);
			return -EINVAL;
		}
		if (t[i].alpha == 0 || t[i].alpha > MAX_ALPHA ||
		    t[i].beta == 0 || t[i].beta > MAX_BETA) {
			pr_err("hw_access: row %u alpha/beta out of range (alpha=%u beta=%u)\n",
			       i, t[i].alpha, t[i].beta);
			return -EINVAL;
		}
		if (t[i].alpha > 1 && t[i].size > (1ULL << t[i].alpha_shift)) {
			pr_err("hw_access: row %u size overlaps alpha_shift (size=0x%llx shift=%u)\n",
			       i, t[i].size, t[i].alpha_shift);
			return -EINVAL;
		}
		if (t[i].beta > 1 && t[i].size > (1ULL << t[i].beta_shift)) {
			pr_err("hw_access: row %u size overlaps beta_shift (size=0x%llx shift=%u)\n",
			       i, t[i].size, t[i].beta_shift);
			return -EINVAL;
		}
		if (t[i].base & t[i].mask) {
			pr_err("hw_access: row %u base 0x%llx not aligned to mask 0x%llx\n",
			       i, t[i].base, t[i].mask);
			return -EINVAL;
		}
	}
	return 0;
}

/* Check if mapping already exists else create a new one */
static int
create_mapping(struct hw_priv_data *priv_data, void __iomem **reg_base,
	       int idx, u64 base, u64 size)
{
	struct hw_csr_mapping *map;

	map = (struct hw_csr_mapping *)(priv_data->map + idx);
	if (map->mapped != true) {
		map->reg_base = ioremap(base, size);
		if (!map->reg_base) {
			pr_err("Unable to map Physical Base Address\n");
			return -ENOMEM;
		}
		pr_debug("Mapping io addr %p at index %d\n", map->reg_base,
			 idx);

		priv_data->total_mappings++;
		map->mapped = true;
	}
	*reg_base = map->reg_base;

	return 0;
}

/* To access CSR space, mappings are setup in small chunks. Unique mapping is
 * identified based on HW (eg. RVU, RPM), alpha (PF in case of RVU, CGX in RPM),
 * beta (FUNC/VF in RVU) attributes. Since each HW may have different alpha/beta
 * components, sizes to map, masking the register offsets, a lookup table is
 * defined where each index represents different values for different HWs.
 */
static int
setup_csr_mapping(struct hw_priv_data *priv_data, u64 addr,
		  void __iomem **reg_base, u64 *offset)
{
	const struct hw_csr_lookup_tbl *row;
	unsigned int i, j, k, idx;
	u64 base;
	int rc;

	for (i = 0; i < csr.len; i++) {
		row = &csr.tbl[i];
		for (j = 0; j < row->alpha; j++) {
			for (k = 0; k < row->beta; k++) {
				/* Per-instance base: row->base OR alpha OR beta */
				base = row->base |
				       ((u64)j << row->alpha_shift) |
				       ((u64)k << row->beta_shift);

				/* Half-open range [base, base + size) */
				if (addr < base || addr >= base + row->size)
					continue;

				idx = ((i * csr.max_alpha *
					csr.max_beta) +
				       (j * csr.max_beta) + k);

				rc = create_mapping(priv_data, reg_base, idx,
						    base, row->size);
				if (rc)
					return rc;

				*offset = addr & row->mask;
				return 0;
			}
		}
	}

	pr_err_ratelimited("hw_access: address 0x%llx out of range\n", addr);
	return -ERANGE;
}

static void
destroy_mapping(struct hw_priv_data *priv_data, int idx)
{
	struct hw_csr_mapping *map;

	map = (struct hw_csr_mapping *)(priv_data->map + idx);
	if (map->mapped == true) {
		pr_debug("Unmapping io addr %p at index %d\n", map->reg_base,
			 idx);
		iounmap(map->reg_base);
		priv_data->total_mappings--;
		map->mapped = false;
	}
}

/* Releasing the mappings */
static void
release_csr_mapping(struct hw_priv_data *priv_data)
{
	unsigned int i, j, k, idx;

	if (!priv_data->map)
		return;

	for (i = 0; i < csr.len; i++) {
		for (j = 0; j < csr.tbl[i].alpha; j++) {
			for (k = 0; k < csr.tbl[i].beta; k++) {
				idx = ((i * csr.max_alpha *
					csr.max_beta) +
				       (j * csr.max_beta) + k);
				destroy_mapping(priv_data, idx);
			}
		}
	}

	if (priv_data->total_mappings != 0)
		pr_err("All mappings not released, %u are remaining\n",
		       priv_data->total_mappings);
}

static int hw_access_open(struct inode *inode, struct file *filp)
{
	struct hw_priv_data *priv_data;
	size_t map_entries;

	if (!csr.tbl || !csr.len)
		return -ENODEV;

	/*
	 * CN20K has no fixed BAR2 formula: build the table from the live PCIe
	 * BAR2 windows on open(). release() frees it and re-arms the build, so
	 * a reopen (e.g. SoI restart) re-walks PCI and picks up functions added
	 * since, such as newly created VFs. Single opener, so the ready flag
	 * needs no lock.
	 */
	if (is_soc_cn20kx() && !csr.cn20k_ready) {
		int rc = build_cn20k_csr_tbl();

		if (!rc)
			rc = validate_csr_tbl(csr.tbl, csr.len);
		if (rc) {
			cn20k_tbl_reset();
			return rc;
		}
		csr.cn20k_ready = true;
	}

	priv_data = kzalloc(sizeof(*priv_data), GFP_KERNEL);
	if (!priv_data)
		return -ENOMEM;

	/*
	 * idx = i*max_alpha*max_beta + j*max_beta + k, with j<alpha<=max_alpha
	 * and k<beta<=max_beta, so the array must be sized accordingly. Use
	 * kvmalloc_array to tolerate multi-MB allocations on fragmented heaps.
	 */
	if (check_mul_overflow((size_t)csr.len,
			       (size_t)csr.max_alpha * csr.max_beta,
			       &map_entries)) {
		kfree(priv_data);
		return -EOVERFLOW;
	}
	priv_data->map = kvmalloc_array(map_entries,
					sizeof(struct hw_csr_mapping),
					GFP_KERNEL | __GFP_ZERO);
	if (!priv_data->map) {
		kfree(priv_data);
		return -ENOMEM;
	}

	priv_data->pdev = pci_get_device(PCI_VENDOR_ID_CAVIUM,
					 PCI_DEVID_OCTEONTX2_RVU_AF, NULL);
	if (!priv_data->pdev) {
		pr_err("hw_access: RVU AF PCI device not found\n");
		kvfree(priv_data->map);
		kfree(priv_data);
		return -ENODEV;
	}

	priv_data->rvu = pci_get_drvdata(priv_data->pdev);
	if (!priv_data->rvu) {
		pr_err("hw_access: RVU AF driver not bound\n");
		pci_dev_put(priv_data->pdev);
		kvfree(priv_data->map);
		kfree(priv_data);
		return -ENODEV;
	}

	filp->private_data = priv_data;

	return 0;
}

static int
hw_access_csr_read(struct hw_priv_data *priv_data, unsigned long arg)
{
	void __iomem *reg_base;
	struct hw_reg_cfg reg_cfg;
	u64 regoff;
	int rc;

	if (copy_from_user(&reg_cfg, (void __user *)arg,
			   sizeof(struct hw_reg_cfg))) {
		pr_err("Read Fault copy from user\n");

		return -EFAULT;
	}

	rc = setup_csr_mapping(priv_data, reg_cfg.regaddr, &reg_base, &regoff);
	if (rc)
		return rc;

	/* Only 64-bit MMIO is supported; 32-bit-only CSRs will trap or alias. */
	reg_cfg.regval = readq(reg_base + regoff);

	if (copy_to_user((void __user *)arg, &reg_cfg,
			 sizeof(struct hw_reg_cfg))) {
		pr_err("Fault in copy to user\n");

		return -EFAULT;
	}
	return 0;
}

static int
hw_access_csr_write(struct hw_priv_data *priv_data, unsigned long arg)
{
	struct hw_reg_cfg reg_cfg;
	void __iomem *reg_base;
	u64 regoff;
	int rc;

	if (copy_from_user(&reg_cfg, (void __user *)arg,
			   sizeof(struct hw_reg_cfg))) {
		pr_err("Write Fault in copy from user\n");

		return -EFAULT;
	}

	rc = setup_csr_mapping(priv_data, reg_cfg.regaddr, &reg_base, &regoff);
	if (rc)
		return rc;

	writeq(reg_cfg.regval, reg_base + regoff);

	return 0;
}

static int
hw_access_nix_ctx_read(struct rvu *rvu, struct hw_ctx_cfg *ctx_cfg,
		       unsigned long arg)
{
	struct nix_aq_enq_req aq_req;
	struct nix_aq_enq_rsp rsp;

	/* CN20K has wider RQ/SQ/CQ context layouts; route to the CN20K handler. */
	if (is_cn20k(rvu->pdev)) {
		struct nix_cn20k_aq_enq_req cn20k_req;
		struct nix_cn20k_aq_enq_rsp cn20k_rsp;

		memset(&cn20k_req, 0, sizeof(cn20k_req));
		cn20k_req.hdr.pcifunc = ctx_cfg->pcifunc;
		cn20k_req.ctype = ctx_cfg->ctype;
		cn20k_req.op = ctx_cfg->op;
		cn20k_req.qidx = ctx_cfg->qidx;

		if (rvu_mbox_handler_nix_cn20k_aq_enq(rvu, &cn20k_req,
						      &cn20k_rsp)) {
			pr_err("Failed to read the context\n");
			return -EINVAL;
		}

		if (copy_to_user((void __user *)arg, &cn20k_rsp,
				 sizeof(cn20k_rsp))) {
			pr_err("Fault in copy to user\n");
			return -EFAULT;
		}

		return 0;
	}

	memset(&aq_req, 0, sizeof(struct nix_aq_enq_req));
	aq_req.hdr.pcifunc = ctx_cfg->pcifunc;
	aq_req.ctype = ctx_cfg->ctype;
	aq_req.op = ctx_cfg->op;
	aq_req.qidx = ctx_cfg->qidx;

	if (rvu_mbox_handler_nix_aq_enq(rvu, &aq_req, &rsp)) {
		pr_err("Failed to read the context\n");
		return -EINVAL;
	}

	if (copy_to_user((void __user *)arg, &rsp,
			 sizeof(struct nix_aq_enq_rsp))) {
		pr_err("Fault in copy to user\n");
		return -EFAULT;
	}

	return 0;
}

static int
hw_access_npa_ctx_read(struct rvu *rvu, struct hw_ctx_cfg *ctx_cfg,
		       unsigned long arg)
{
	struct npa_aq_enq_req aq_req;
	struct npa_aq_enq_rsp rsp;

	/* CN20K has wider Aura/Pool context layouts; route to the CN20K handler. */
	if (is_cn20k(rvu->pdev)) {
		struct npa_cn20k_aq_enq_req cn20k_req;
		struct npa_cn20k_aq_enq_rsp cn20k_rsp;

		memset(&cn20k_req, 0, sizeof(cn20k_req));
		cn20k_req.hdr.pcifunc = ctx_cfg->pcifunc;
		cn20k_req.ctype = ctx_cfg->ctype;
		cn20k_req.op = ctx_cfg->op;
		cn20k_req.aura_id = ctx_cfg->aura;

		if (rvu_mbox_handler_npa_cn20k_aq_enq(rvu, &cn20k_req,
						      &cn20k_rsp)) {
			pr_err("Failed to read the npa context\n");
			return -EINVAL;
		}

		if (copy_to_user((void __user *)arg, &cn20k_rsp,
				 sizeof(cn20k_rsp))) {
			pr_err("Fault in copy to user\n");
			return -EFAULT;
		}

		return 0;
	}

	memset(&aq_req, 0, sizeof(struct npa_aq_enq_req));
	aq_req.hdr.pcifunc = ctx_cfg->pcifunc;
	aq_req.ctype = ctx_cfg->ctype;
	aq_req.op = ctx_cfg->op;
	aq_req.aura_id = ctx_cfg->aura;

	if (rvu_mbox_handler_npa_aq_enq(rvu, &aq_req, &rsp)) {
		pr_err("Failed to read the npa context\n");
		return -EINVAL;
	}

	if (copy_to_user((void __user *)arg, &rsp,
			 sizeof(struct npa_aq_enq_rsp))) {
		pr_err("Fault in copy to user\n");
		return -EFAULT;
	}

	return 0;
}

static int
hw_access_ctx_read(struct rvu *rvu, unsigned long arg)
{
	struct hw_ctx_cfg ctx_cfg;
	int rc;

	if (copy_from_user(&ctx_cfg, (void __user *)arg,
			   sizeof(struct hw_ctx_cfg))) {
		pr_err("Write Fault in copy from user\n");
		return -EFAULT;
	}

	switch (ctx_cfg.blkaddr) {
	case BLKADDR_NIX0:
	case BLKADDR_NIX1:
		rc = hw_access_nix_ctx_read(rvu, &ctx_cfg, arg);
		break;
	case BLKADDR_NPA:
		rc = hw_access_npa_ctx_read(rvu, &ctx_cfg, arg);
		break;
	default:
		rc = -EINVAL;
		break;
	}
	return rc;
}

static int
hw_access_cgx_info(struct rvu *rvu, unsigned long arg)
{
	struct hw_cgx_info cgx_info;
	struct rvu_pfvf *pfvf;
	u8 cgx_id, lmac_id, pf;

	if (copy_from_user(&cgx_info, (void __user *)arg, sizeof(struct hw_cgx_info))) {
		pr_err("Reading PF value failed: copy from user\n");
		return -EFAULT;
	}

	pf = cgx_info.pf;
	if (!(pf >= PF_CGXMAP_BASE && pf <= rvu->cgx_mapped_pfs)) {
		/* Not a CGX/RPM-mapped PF (e.g. LBK, SDP or other internal NIX
		 * PFs). User space probes every NIX PF for RPM info and skips
		 * the ones that return an error, so this is an expected result
		 * that must not spam dmesg on every periodic poll. Keep it at
		 * debug level and return -ENODEV (no such device) rather than
		 * -EFAULT (bad address), which is the accurate errno here.
		 */
		pr_debug("PF %d is not CGX/RPM-mapped\n", pf);
		return -ENODEV;
	}

	pfvf = &rvu->pf[pf];
	rvu_get_cgx_lmac_id(rvu->pf2cgxlmac_map[pf], &cgx_id, &lmac_id);
	cgx_info.cgx_id = cgx_id;
	cgx_info.lmac_id = lmac_id;
	cgx_info.nix_idx = (pfvf->nix_blkaddr == BLKADDR_NIX0) ? 0 : 1;

	if (copy_to_user((void __user *)arg, &cgx_info,
			 sizeof(struct hw_cgx_info))) {
		pr_err("Fault in copy to user\n");

		return -EFAULT;
	}
	return 0;
}

static int
hw_access_link_info(struct rvu *rvu, unsigned long arg)
{
	struct hw_link_info linfo;
	struct lmac *lmac;
	struct cgx *cgxd;

	if (copy_from_user(&linfo, (void __user *)arg,
			   sizeof(struct hw_link_info))) {
		pr_err("Reading PF value failed: copy from user\n");
		return -EFAULT;
	}

	if (linfo.cgx_id >= rvu->cgx_cnt_max)
		return -ENODEV;

	cgxd = rvu->cgx_idmap[linfo.cgx_id];
	if (!cgxd)
		return -ENODEV;

	if (linfo.lmac_id >= cgxd->max_lmac_per_mac)
		return -ENODEV;

	lmac = cgxd->lmac_idmap[linfo.lmac_id];
	if (!lmac)
		return -ENODEV;

	memcpy(&linfo.link_info, &lmac->link_info, sizeof(lmac->link_info));

	if (copy_to_user((void __user *)arg, &linfo,
			 sizeof(struct hw_link_info))) {
		pr_err("Fault in copy to user\n");

		return -EFAULT;
	}
	return 0;
}

static long hw_access_ioctl(struct file *filp, unsigned int cmd,
			    unsigned long arg)
{
	struct hw_priv_data *priv_data = filp->private_data;
	struct rvu *rvu;

	if (!priv_data || !priv_data->rvu)
		return -ENODEV;
	rvu = priv_data->rvu;

	switch (cmd) {
	case HW_ACCESS_CSR_READ_IOCTL:
		return hw_access_csr_read(priv_data, arg);

	case HW_ACCESS_CSR_WRITE_IOCTL:
		return hw_access_csr_write(priv_data, arg);

	case HW_ACCESS_CTX_READ_IOCTL:
		return hw_access_ctx_read(rvu, arg);

	case HW_ACCESS_CGX_INFO_IOCTL:
		return hw_access_cgx_info(rvu, arg);

	case HW_ACCESS_LINK_INFO_IOCTL:
		return hw_access_link_info(rvu, arg);

	default:
		pr_info("Invalid IOCTL: %d\n", cmd);

		return -EINVAL;
	}
}

static int hw_access_release(struct inode *inode, struct file *filp)
{
	struct hw_priv_data *priv_data = filp->private_data;

	if (!priv_data)
		return 0;

	release_csr_mapping(priv_data);
	pci_dev_put(priv_data->pdev);
	filp->private_data = NULL;
	kvfree(priv_data->map);
	kfree(priv_data);

	/*
	 * CN20K: drop this session's built table and re-arm the build so the
	 * next open() re-walks PCI. Must follow release_csr_mapping(), which
	 * walks csr.tbl to iounmap. Single opener, so nothing else uses csr.dyn.
	 */
	if (is_soc_cn20kx())
		cn20k_tbl_reset();

	return 0;
}

static ssize_t reg_data_read(struct file *fp, char __user *user_buffer,
			     size_t count, loff_t *position)
{
	struct arm_smccc_res smc_resp;
	u8 buf[100];
	unsigned int len;

	if (!reg_addr) {
		pr_err("Secure Reg Read failure : Invalid Reg_Addr\n");
		return -EFAULT;
	}

	arm_smccc_smc(OCTEONTX_ACCESS_REG_SMCID, 0,
		      reg_addr, READ, size_32, 0, 0, 0, &smc_resp);
	if (smc_resp.a0 != SMCCC_RET_SUCCESS)
		pr_err("Secure Reg Read failure\n");

	if (size_32)
		len = scnprintf(buf, sizeof(buf), "0x%llx:0x%x\n", reg_addr,
				(u32)smc_resp.a1);
	else
		len = scnprintf(buf, sizeof(buf), "0x%llx:0x%llx\n", reg_addr,
				(u64)smc_resp.a1);

	return simple_read_from_buffer(user_buffer, count, position, buf, len);
}

static ssize_t reg_data_write(struct file *fp, const char __user *user_buffer,
			      size_t count, loff_t *position)
{
	struct arm_smccc_res smc_resp;
	u64 reg_data = 0;
	int ret;

	if (!reg_addr) {
		pr_err("Secure Reg Write failure : Invalid Reg_Addr\n");
		return -EFAULT;
	}

	if (size_32) {
		u32 v;

		ret = kstrtou32_from_user(user_buffer, count, 0, &v);
		if (ret)
			return ret;
		reg_data = v;
	} else {
		ret = kstrtou64_from_user(user_buffer, count, 0,
					  &reg_data);
		if (ret)
			return ret;
	}
	arm_smccc_smc(OCTEONTX_ACCESS_REG_SMCID, reg_data,
		      reg_addr, WRITE, size_32, 0, 0, 0, &smc_resp);
	if (smc_resp.a0 != SMCCC_RET_SUCCESS)
		pr_err("Secure Reg Write failure\n");
	return count;
}

static const struct file_operations mmap_fops = {
	.open = hw_access_open,
	.unlocked_ioctl = hw_access_ioctl,
	.release = hw_access_release,
};

static const struct file_operations fops_reg_data = {
	.read = reg_data_read,
	.write = reg_data_write,
};

static int __init hw_access_module_init(void)
{
	static struct device *hw_reg_device;
	int rc;

	rc = select_csr_tbl();
	if (rc)
		return rc;

	rc = validate_csr_tbl(csr.tbl, csr.len);
	if (rc) {
		kfree(csr.dyn);
		csr.dyn = NULL;
		return rc;
	}

	major_no = register_chrdev(0, DEVICE_NAME, &mmap_fops);
	if (major_no < 0) {
		pr_err("failed to register a major number for %s\n",
		       DEVICE_NAME);
		return major_no;
	}

	hw_reg_class = class_create("hw_access");
	if (IS_ERR(hw_reg_class)) {
		unregister_chrdev(major_no, DEVICE_NAME);
		return PTR_ERR(hw_reg_class);
	}

	hw_reg_device = device_create(hw_reg_class, NULL,
				      MKDEV(major_no, 0), NULL,
				      DEVICE_NAME);
	if (IS_ERR(hw_reg_device)) {
		class_destroy(hw_reg_class);
		unregister_chrdev(major_no, DEVICE_NAME);
		return PTR_ERR(hw_reg_device);
	}

	/* create a directory sec_access in debufs */
	dirret = debugfs_create_dir("sec_access", NULL);
	/* create file to read/write 32/64 bit data from/to reg_addr */
	debugfs_create_file("reg_data", 0644, dirret, &filevalue,
			    &fops_reg_data);
	/* create file for reg_addr from where 32/64 bit data is read/written */
	debugfs_create_x64("reg_addr", 0644, dirret, &reg_addr);
	/* create file for choosing between 32 & 64 bit data */
	debugfs_create_bool("size_32", 0644, dirret, &size_32);
	return 0;
}

static void __exit hw_access_module_exit(void)
{
	device_destroy(hw_reg_class, MKDEV(major_no, 0));
	class_destroy(hw_reg_class);
	unregister_chrdev(major_no, DEVICE_NAME);
	debugfs_remove_recursive(dirret);
	kfree(csr.dyn);
	csr.dyn = NULL;
}

module_init(hw_access_module_init);
module_exit(hw_access_module_exit);
MODULE_AUTHOR("Marvell International Ltd.");
MODULE_LICENSE("GPL v2");
