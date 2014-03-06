#ifndef __ASM_ARCH_ORION_NFC_H
#define __ASM_ARCH_ORION_NFC_H

#include <linux/mtd/mtd.h>
#include <linux/mtd/partitions.h>
#include "mvCommon.h"
#include "mvOs.h"
#ifdef CONFIG_MV_INCLUDE_PDMA
#include "pdma/mvPdma.h"
#endif
#include "hal/mvNfc.h"
#include "hal/mvNfcRegs.h"

enum nfc_page_size {
	NFC_PAGE_512B = 0,
	NFC_PAGE_2KB,
	NFC_PAGE_4KB,
	NFC_PAGE_8KB,
	NFC_PAGE_16KB,
	NFC_PAGE_SIZE_MAX_CNT
};

struct nfc_platform_data {
	unsigned int		tclk;		/* Clock supplied to NFC */
	unsigned int		nfc_width;	/* Width of NFC 16/8 bits */
	unsigned int		num_devs;	/* Number of NAND devices
						   (2 for ganged mode).   */
	unsigned int		num_cs;		/* Number of NAND devices
						   chip-selects.	  */
	unsigned int		use_dma;	/* Enable/Disable DMA 1/0 */
	MV_NFC_ECC_MODE		ecc_type;
	struct mtd_partition	*parts;
	unsigned int		nr_parts;
};
#endif /* __ASM_ARCH_ORION_NFC_H */
