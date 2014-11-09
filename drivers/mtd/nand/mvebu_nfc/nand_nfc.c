/*
 * nand_nfc.c
 *
 * Copyright c 2005 Intel Corporation
 * Copyright c 2006 Marvell International Ltd.
 *
 * This driver is based on the PXA drivers/mtd/nand/pxa3xx_nand.c
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/module.h>
#include <linux/interrupt.h>
#include <linux/platform_device.h>
#include <linux/dma-mapping.h>
#include <linux/delay.h>
#include <linux/clk.h>
#include <linux/mtd/mtd.h>
#include <linux/mtd/nand.h>
#include <linux/mtd/partitions.h>
#include <linux/io.h>
#include <linux/of.h>
#include <linux/of_irq.h>
#include <linux/irq.h>
#include <linux/slab.h>
#include <asm/dma.h>
#include "mvCommon.h"
#include "mvOs.h"

#ifndef CONFIG_OF
#error nand_nfc driver supports only DT configuration
#endif

#ifdef CONFIG_MV_INCLUDE_PDMA
#include <asm/hardware/pxa-dma.h>
#include "pdma/mvPdma.h"
#include "pdma/mvPdmaRegs.h"
#endif
#include "nand_nfc.h"

#define	DRIVER_NAME	"armada-nand"

#define NFC_DPRINT(x)		/* printk (x) */
#define PRINT_LVL		KERN_DEBUG

#define	CHIP_DELAY_TIMEOUT	(20 * HZ/10)
#define NFC_MAX_NUM_OF_DESCR	(33) /* worst case in 8K ganaged */
#define NFC_8BIT1K_ECC_SPARE	(32)

#define NFC_SR_MASK		(0xfff)
#define NFC_SR_BBD_MASK		(NFC_SR_CS0_BBD_MASK | NFC_SR_CS1_BBD_MASK)

#define ARMADA_MAIN_PLL_FREQ	2000000000

char *cmd_text[] = {
	"MV_NFC_CMD_READ_ID",
	"MV_NFC_CMD_READ_STATUS",
	"MV_NFC_CMD_ERASE",
	"MV_NFC_CMD_MULTIPLANE_ERASE",
	"MV_NFC_CMD_RESET",

	"MV_NFC_CMD_CACHE_READ_SEQ",
	"MV_NFC_CMD_CACHE_READ_RAND",
	"MV_NFC_CMD_EXIT_CACHE_READ",
	"MV_NFC_CMD_CACHE_READ_START",
	"MV_NFC_CMD_READ_MONOLITHIC",
	"MV_NFC_CMD_READ_MULTIPLE",
	"MV_NFC_CMD_READ_NAKED",
	"MV_NFC_CMD_READ_LAST_NAKED",
	"MV_NFC_CMD_READ_DISPATCH",

	"MV_NFC_CMD_WRITE_MONOLITHIC",
	"MV_NFC_CMD_WRITE_MULTIPLE",
	"MV_NFC_CMD_WRITE_NAKED",
	"MV_NFC_CMD_WRITE_LAST_NAKED",
	"MV_NFC_CMD_WRITE_DISPATCH",
	"MV_NFC_CMD_WRITE_DISPATCH_START",
	"MV_NFC_CMD_WRITE_DISPATCH_END",

	"MV_NFC_CMD_COUNT"	/* This should be the last enum */

};

MV_U32 pg_sz[NFC_PAGE_SIZE_MAX_CNT] = {512, 2048, 4096, 8192, 16384};
MV_U32 mv_nand_base;
struct clk *ecc_clk;

/* error code and state */
enum {
	ERR_NONE	= 0,
	ERR_DMABUSERR	= -1,
	ERR_CMD_TO	= -2,
	ERR_DATA_TO	= -3,
	ERR_DBERR	= -4,
	ERR_BBD		= -5,
};

enum {
	STATE_READY	= 0,
	STATE_CMD_HANDLE,
	STATE_DMA_READING,
	STATE_DMA_WRITING,
	STATE_DMA_DONE,
	STATE_PIO_READING,
	STATE_PIO_WRITING,
};

struct orion_nfc_info {
	struct platform_device	 *pdev;

	void __iomem		*mmio_base;
	unsigned int		mmio_phys_base;

	unsigned int		irq;

	struct clk		*aux_clk;

	unsigned int		buf_start;
	unsigned int		buf_count;

	unsigned char		*data_buff;
	dma_addr_t		data_buff_phys;
	size_t			data_buff_size;

	/* saved column/page_addr during CMD_SEQIN */
	int			seqin_column;
	int			seqin_page_addr;

	/* relate to the command */
	unsigned int		state;
	unsigned int		use_dma;	/* use DMA ? */

	/* flash information */
	unsigned int		nfc_width;	/* Width of NFC 16/8 bits	*/
	const char		*nfc_mode;	/* NAND mode - normal or ganged	*/
	unsigned int		num_cs;		/* Number of NAND devices	*/
						/* chip-selects.		*/
	MV_NFC_ECC_MODE		ecc_type;
	enum nfc_page_size	page_size;
	uint32_t		page_per_block;	/* Pages per block (PG_PER_BLK) */
	uint32_t		flash_width;	/* Width of Flash memory (DWIDTH_M) */
	size_t			read_id_bytes;

	size_t			data_size;	/* data size in FIFO */
	size_t			read_size;
	int			retcode;
	uint32_t		dscr;		/* IRQ events - status */
	struct completion	cmd_complete;

	int			chained_cmd;
	uint32_t		column;
	uint32_t		page_addr;
	MV_NFC_CMD_TYPE		cmd;
	MV_NFC_CTRL		nfcCtrl;

	/* RW buffer chunks config */
	MV_U32			sgBuffAddr[MV_NFC_RW_MAX_BUFF_NUM];
	MV_U32			sgBuffSize[MV_NFC_RW_MAX_BUFF_NUM];
	MV_U32			sgNumBuffs;

	/* suspend / resume data */
	MV_U32			nfcUnitData[128];
	MV_U32			nfcDataLen;
	MV_U32			pdmaUnitData[128];
	MV_U32			pdmaDataLen;
};

/*
 * ECC Layout
 */

static struct nand_ecclayout ecc_latout_512B_hamming = {
	.eccbytes = 6,
	.eccpos = {8, 9, 10, 11, 12, 13 },
	.oobfree = { {2, 6} }
};

static struct nand_ecclayout ecc_layout_2KB_hamming = {
	.eccbytes = 24,
	.eccpos = {
		40, 41, 42, 43, 44, 45, 46, 47,
		48, 49, 50, 51, 52, 53, 54, 55,
		56, 57, 58, 59, 60, 61, 62, 63},
	.oobfree = { {2, 38} }
};

static struct nand_ecclayout ecc_layout_2KB_bch4bit = {
	.eccbytes = 32,
	.eccpos = {
		32, 33, 34, 35, 36, 37, 38, 39,
		40, 41, 42, 43, 44, 45, 46, 47,
		48, 49, 50, 51, 52, 53, 54, 55,
		56, 57, 58, 59, 60, 61, 62, 63},
	.oobfree = { {2, 30} }
};

static struct nand_ecclayout ecc_layout_4KB_bch4bit = {
	.eccbytes = 64,
	.eccpos = {
		32,  33,  34,  35,  36,  37,  38,  39,
		40,  41,  42,  43,  44,  45,  46,  47,
		48,  49,  50,  51,  52,  53,  54,  55,
		56,  57,  58,  59,  60,  61,  62,  63,
		96,  97,  98,  99,  100, 101, 102, 103,
		104, 105, 106, 107, 108, 109, 110, 111,
		112, 113, 114, 115, 116, 117, 118, 119,
		120, 121, 122, 123, 124, 125, 126, 127},
	/* Bootrom looks in bytes 0 & 5 for bad blocks */
	.oobfree = { {1, 4}, {6, 26}, { 64, 32} }
};

static struct nand_ecclayout ecc_layout_8KB_bch4bit = {
	.eccbytes = 128,
	.eccpos = {
		32,  33,  34,  35,  36,  37,  38,  39,
		40,  41,  42,  43,  44,  45,  46,  47,
		48,  49,  50,  51,  52,  53,  54,  55,
		56,  57,  58,  59,  60,  61,  62,  63,

		96,  97,  98,  99,  100, 101, 102, 103,
		104, 105, 106, 107, 108, 109, 110, 111,
		112, 113, 114, 115, 116, 117, 118, 119,
		120, 121, 122, 123, 124, 125, 126, 127,

		160, 161, 162, 163, 164, 165, 166, 167,
		168, 169, 170, 171, 172, 173, 174, 175,
		176, 177, 178, 179, 180, 181, 182, 183,
		184, 185, 186, 187, 188, 189, 190, 191,

		224, 225, 226, 227, 228, 229, 230, 231,
		232, 233, 234, 235, 236, 237, 238, 239,
		240, 241, 242, 243, 244, 245, 246, 247,
		248, 249, 250, 251, 252, 253, 254, 255},

	/* Bootrom looks in bytes 0 & 5 for bad blocks */
	.oobfree = { {1, 4}, {6, 26}, { 64, 32}, {128, 32}, {192, 32} }
};

static struct nand_ecclayout ecc_layout_4KB_bch8bit = {
	.eccbytes = 64,
	.eccpos = {
		32,  33,  34,  35,  36,  37,  38,  39,
		40,  41,  42,  43,  44,  45,  46,  47,
		48,  49,  50,  51,  52,  53,  54,  55,
		56,  57,  58,  59,  60,  61,  62,  63},
	/* Bootrom looks in bytes 0 & 5 for bad blocks */
	.oobfree = { {1, 4}, {6, 26},  }
};

static struct nand_ecclayout ecc_layout_8KB_bch8bit = {
	.eccbytes = 160,
	.eccpos = {
		128, 129, 130, 131, 132, 133, 134, 135,
		136, 137, 138, 139, 140, 141, 142, 143,
		144, 145, 146, 147, 148, 149, 150, 151,
		152, 153, 154, 155, 156, 157, 158, 159},
	/* Bootrom looks in bytes 0 & 5 for bad blocks */
	.oobfree = { {1, 4}, {6, 122},  }
};

static struct nand_ecclayout ecc_layout_8KB_bch12bit = {
	.eccbytes = 0,
	.eccpos = { },
	/* Bootrom looks in bytes 0 & 5 for bad blocks */
	.oobfree = { {1, 4}, {6, 58}, }
};

static struct nand_ecclayout ecc_layout_16KB_bch12bit = {
	.eccbytes = 0,
	.eccpos = { },
	/* Bootrom looks in bytes 0 & 5 for bad blocks */
	.oobfree = { {1, 4}, {6, 122},  }
};

/*
 * Define bad block scan pattern when scanning a device for factory
 * marked blocks.
 */
static uint8_t mv_scan_pattern[] = { 0xff, 0xff };

static struct nand_bbt_descr mv_sp_bb = {
	.options = NAND_BBT_SCANMVCUSTOM,
	.offs = 5,
	.len = 1,
	.pattern = mv_scan_pattern
};

static struct nand_bbt_descr mv_lp_bb = {
	.options = NAND_BBT_SCANMVCUSTOM,
	.offs = 0,
	.len = 2,
	.pattern = mv_scan_pattern
};

/*
 * Lookup Tables
 */

struct orion_nfc_naked_info {

	struct nand_ecclayout	*ecc_layout;
	struct nand_bbt_descr	*bb_info;
	uint32_t		bb_bytepos;
	uint32_t		chunk_size;
	uint32_t		chunk_spare;
	uint32_t		chunk_cnt;
	uint32_t		last_chunk_size;
	uint32_t		last_chunk_spare;
};

		/* PageSize */		/* ECc Type */
static struct orion_nfc_naked_info orion_nfc_naked_info_lkup[NFC_PAGE_SIZE_MAX_CNT][MV_NFC_ECC_MAX_CNT] = {
	/* 512B Pages */
	{  {	/* Hamming */
		&ecc_latout_512B_hamming, &mv_sp_bb, 512, 512, 16, 1, 0, 0
	}, {	/* BCH 4bit */
		NULL, NULL, 0, 0, 0, 0, 0, 0
	}, {	/* BCH 8bit */
		NULL, NULL, 0, 0, 0, 0, 0, 0
	}, {	/* BCH 12bit */
		NULL, NULL, 0, 0, 0, 0, 0, 0
	}, {	/* BCH 16bit */
		NULL, NULL, 0, 0, 0, 0, 0, 0
	}, {	/* No ECC */
		NULL, NULL, 0, 0, 0, 0, 0, 0
	}  },
	/* 2KB Pages */
	{  {	/* Hamming */
		&ecc_layout_2KB_hamming, &mv_lp_bb, 2048, 2048, 40, 1, 0, 0
	}, {	/* BCH 4bit */
		&ecc_layout_2KB_bch4bit, &mv_lp_bb, 2048, 2048, 32, 1, 0, 0
	}, {	/* BCH 8bit */
		NULL, NULL, 2018, 1024, 0, 1, 1024, 32
	}, {	/* BCH 12bit */
		NULL, NULL, 1988, 704, 0, 2, 640, 0
	}, {	/* BCH 16bit */
		NULL, NULL, 1958, 512, 0, 4, 0, 32
	}, {	/* No ECC */
		NULL, NULL, 0, 0, 0, 0, 0, 0
	}  },
	/* 4KB Pages */
	{  {	/* Hamming */
		NULL, 0, 0, 0, 0, 0, 0, 0
	}, {	/* BCH 4bit */
		&ecc_layout_4KB_bch4bit, &mv_lp_bb, 4034, 2048, 32, 2, 0, 0
	}, {	/* BCH 8bit */
		&ecc_layout_4KB_bch8bit, &mv_lp_bb, 4006, 1024, 0, 4, 0, 64
	}, {	/* BCH 12bit */
		NULL, NULL, 3946, 704,  0, 5, 576, 32
	}, {	/* BCH 16bit */
		NULL, NULL, 3886, 512, 0, 8, 0, 32
	}, {	/* No ECC */
		NULL, NULL, 0, 0, 0, 0, 0, 0
	}  },
	/* 8KB Pages */
	{  {	/* Hamming */
		NULL, 0, 0, 0, 0, 0, 0, 0
	}, {	/* BCH 4bit */
		&ecc_layout_8KB_bch4bit, &mv_lp_bb, 8102, 2048, 32, 4, 0, 0
	}, {	/* BCH 8bit */
		&ecc_layout_8KB_bch8bit, &mv_lp_bb, 7982, 1024, 0, 8, 0, 160
	}, {	/* BCH 12bit */
		&ecc_layout_8KB_bch12bit, &mv_lp_bb, 7862, 704, 0, 11, 448, 64
	}, {	/* BCH 16bit */
		NULL, NULL, 7742, 512, 0, 16, 0, 32
	}, {	/* No ECC */
		NULL, NULL, 0, 0, 0, 0, 0, 0
	}  },
	/* 16KB Pages */
	{  {	/* Hamming */
		NULL, NULL, 0, 0, 0, 0, 0, 0
	}, {	/* BCH 4bit */
		NULL, NULL, 15914, 2048, 32, 8, 0, 0
	}, {	/* BCH 8bit */
		NULL, NULL, 15930, 1024, 0, 16, 0, 352
	}, {	/* BCH 12bit */
		&ecc_layout_16KB_bch12bit, &mv_lp_bb, 15724, 704, 0, 23, 192, 128
	}, {	/* BCH 16bit */
		NULL, NULL, 15484, 512, 0, 32, 0, 32
	}, {	/* No ECC */
		NULL, NULL, 0, 0, 0, 0, 0, 0
	}  } };


#define ECC_LAYOUT	(orion_nfc_naked_info_lkup[info->page_size][info->ecc_type].ecc_layout)
#define BB_INFO		(orion_nfc_naked_info_lkup[info->page_size][info->ecc_type].bb_info)
#define	BB_BYTE_POS	(orion_nfc_naked_info_lkup[info->page_size][info->ecc_type].bb_bytepos)
#define CHUNK_CNT	(orion_nfc_naked_info_lkup[info->page_size][info->ecc_type].chunk_cnt)
#define CHUNK_SZ	(orion_nfc_naked_info_lkup[info->page_size][info->ecc_type].chunk_size)
#define CHUNK_SPR	(orion_nfc_naked_info_lkup[info->page_size][info->ecc_type].chunk_spare)
#define LST_CHUNK_SZ	(orion_nfc_naked_info_lkup[info->page_size][info->ecc_type].last_chunk_size)
#define LST_CHUNK_SPR	(orion_nfc_naked_info_lkup[info->page_size][info->ecc_type].last_chunk_spare)

struct orion_nfc_cmd_info {

	uint32_t		events_p1;	/* post command events */
	uint32_t		events_p2;	/* post data events */
	MV_NFC_PIO_RW_MODE	rw;
};

static struct orion_nfc_cmd_info orion_nfc_cmd_info_lkup[MV_NFC_CMD_COUNT] = {
	/* Phase 1 interrupts */			/* Phase 2 interrupts */
									/* Read/Write */  /* MV_NFC_CMD_xxxxxx */
	{(NFC_SR_RDDREQ_MASK),				(0),
									MV_NFC_PIO_READ}, /* READ_ID */
	{(NFC_SR_RDDREQ_MASK),				(0),
									MV_NFC_PIO_READ}, /* READ_STATUS */
	{(0),						(MV_NFC_STATUS_RDY | MV_NFC_STATUS_BBD),
									MV_NFC_PIO_NONE}, /* ERASE */
	{(0),						(0),
									MV_NFC_PIO_NONE}, /* MULTIPLANE_ERASE */
	{(0),						(MV_NFC_STATUS_RDY),
									MV_NFC_PIO_NONE}, /* RESET */
	{(0),						(0),
									MV_NFC_PIO_READ}, /* CACHE_READ_SEQ */
	{(0),						(0),
									MV_NFC_PIO_READ}, /* CACHE_READ_RAND */
	{(0),						(0),
									MV_NFC_PIO_NONE}, /* EXIT_CACHE_READ */
	{(0),						(0),
									MV_NFC_PIO_READ}, /* CACHE_READ_START */
	{(NFC_SR_RDDREQ_MASK | NFC_SR_UNCERR_MASK),	(0),
									MV_NFC_PIO_READ}, /* READ_MONOLITHIC */
	{(0),						(0),
									MV_NFC_PIO_READ}, /* READ_MULTIPLE */
	{(NFC_SR_RDDREQ_MASK | NFC_SR_UNCERR_MASK),	(0),
									MV_NFC_PIO_READ}, /* READ_NAKED */
	{(NFC_SR_RDDREQ_MASK | NFC_SR_UNCERR_MASK),	(0),
									MV_NFC_PIO_READ}, /* READ_LAST_NAKED */
	{(0),						(0),
									MV_NFC_PIO_NONE}, /* READ_DISPATCH */
	{(MV_NFC_STATUS_WRD_REQ),			(MV_NFC_STATUS_RDY | MV_NFC_STATUS_BBD),
									MV_NFC_PIO_WRITE},/* WRITE_MONOLITHIC */
	{(0),						(0),
									MV_NFC_PIO_WRITE},/* WRITE_MULTIPLE */
	{(MV_NFC_STATUS_WRD_REQ),			(MV_NFC_STATUS_PAGED),
									MV_NFC_PIO_WRITE},/* WRITE_NAKED */
	{(0),						(0),
									MV_NFC_PIO_WRITE},/* WRITE_LAST_NAKED */
	{(0),						(0),
									MV_NFC_PIO_NONE}, /* WRITE_DISPATCH */
	{(MV_NFC_STATUS_CMDD),				(0),
									MV_NFC_PIO_NONE}, /* WRITE_DISPATCH_START */
	{(0),						(MV_NFC_STATUS_RDY | MV_NFC_STATUS_BBD),
									MV_NFC_PIO_NONE}, /* WRITE_DISPATCH_END */
};

static int prepare_read_prog_cmd(struct orion_nfc_info *info,
			int column, int page_addr)
{
	MV_U32 size;

	if (mvNfcFlashPageSizeGet(&info->nfcCtrl, &size, &info->data_size)
	    != MV_OK)
		return -EINVAL;

	return 0;
}
int orion_nfc_wait_for_completion_timeout(struct orion_nfc_info *info, int timeout)
{
	return wait_for_completion_timeout(&info->cmd_complete, timeout);

}

#ifdef CONFIG_MV_INCLUDE_PDMA
static void orion_nfc_data_dma_irq(int irq, void *data)
{
	struct orion_nfc_info *info = data;
	uint32_t dcsr, intr;
	int channel = info->nfcCtrl.dataChanHndl.chanNumber;

	intr = MV_REG_READ(PDMA_INTR_CAUSE_REG);
	dcsr = MV_REG_READ(PDMA_CTRL_STATUS_REG(channel));
	MV_REG_WRITE(PDMA_CTRL_STATUS_REG(channel), dcsr);

	NFC_DPRINT((PRINT_LVL "orion_nfc_data_dma_irq(0x%x, 0x%x) - 1.\n", dcsr, intr));

	if (info->chained_cmd) {
		if (dcsr & DCSR_BUSERRINTR) {
			info->retcode = ERR_DMABUSERR;
			complete(&info->cmd_complete);
		}
		if ((info->state == STATE_DMA_READING) && (dcsr & DCSR_ENDINTR)) {
			info->state = STATE_READY;
			complete(&info->cmd_complete);
		}
		return;
	}

	if (dcsr & DCSR_BUSERRINTR) {
		info->retcode = ERR_DMABUSERR;
		complete(&info->cmd_complete);
	}

	if (info->state == STATE_DMA_WRITING) {
		info->state = STATE_DMA_DONE;
		mvNfcIntrSet(&info->nfcCtrl,  MV_NFC_STATUS_BBD | MV_NFC_STATUS_RDY , MV_TRUE);
	} else {
		info->state = STATE_READY;
		complete(&info->cmd_complete);
	}

	return;
}
#endif

static irqreturn_t orion_nfc_irq_pio(int irq, void *devid)
{
	struct orion_nfc_info *info = devid;

	/* Disable all interrupts */
	mvNfcIntrSet(&info->nfcCtrl, 0xFFF, MV_FALSE);

	/* Clear the interrupt and pass the status UP */
	info->dscr = MV_REG_READ(NFC_STATUS_REG);
	NFC_DPRINT((PRINT_LVL ">>> orion_nfc_irq_pio(0x%x)\n", info->dscr));
	MV_REG_WRITE(NFC_STATUS_REG, info->dscr);
	complete(&info->cmd_complete);

	return IRQ_HANDLED;
}

#ifdef CONFIG_MV_INCLUDE_PDMA
static irqreturn_t orion_nfc_irq_dma(int irq, void *devid)
{
	struct orion_nfc_info *info = devid;
	unsigned int status;

	status = MV_REG_READ(NFC_STATUS_REG);

	NFC_DPRINT((PRINT_LVL "orion_nfc_irq_dma(0x%x) - 1.\n", status));

	if (!info->chained_cmd) {
		if (status & (NFC_SR_RDDREQ_MASK | NFC_SR_UNCERR_MASK)) {
			if (status & NFC_SR_UNCERR_MASK)
				info->retcode = ERR_DBERR;
			mvNfcIntrSet(&info->nfcCtrl, NFC_SR_RDDREQ_MASK | NFC_SR_UNCERR_MASK, MV_FALSE);
			if (info->use_dma) {
				info->state = STATE_DMA_READING;
				mvNfcReadWrite(&info->nfcCtrl, info->cmd,
						(MV_U32 *)info->data_buff, info->data_buff_phys);
			} else {
				info->state = STATE_PIO_READING;
				complete(&info->cmd_complete);
			}
		} else if (status & NFC_SR_WRDREQ_MASK) {
			mvNfcIntrSet(&info->nfcCtrl, NFC_SR_WRDREQ_MASK, MV_FALSE);
			if (info->use_dma) {
				info->state = STATE_DMA_WRITING;
				NFC_DPRINT((PRINT_LVL "Calling mvNfcReadWrite().\n"));
				if (mvNfcReadWrite(&info->nfcCtrl, info->cmd,
						   (MV_U32 *)info->data_buff,
						   info->data_buff_phys)
				    != MV_OK)
					pr_err("mvNfcReadWrite() failed.\n");
			} else {
				info->state = STATE_PIO_WRITING;
				complete(&info->cmd_complete);
			}
		} else if (status & (NFC_SR_BBD_MASK | MV_NFC_CS0_CMD_DONE_INT |
				     NFC_SR_RDY0_MASK | MV_NFC_CS1_CMD_DONE_INT |
				     NFC_SR_RDY1_MASK)) {
			if (status & NFC_SR_BBD_MASK)
				info->retcode = ERR_BBD;
			mvNfcIntrSet(&info->nfcCtrl,  MV_NFC_STATUS_BBD |
					MV_NFC_STATUS_CMDD | MV_NFC_STATUS_RDY,
					MV_FALSE);
			info->state = STATE_READY;
			complete(&info->cmd_complete);
		}
	} else if (status & (NFC_SR_BBD_MASK | NFC_SR_RDY0_MASK |
				NFC_SR_RDY1_MASK | NFC_SR_UNCERR_MASK)) {
		if (status & (NFC_SR_BBD_MASK | NFC_SR_UNCERR_MASK))
			info->retcode = ERR_DBERR;
		mvNfcIntrSet(&info->nfcCtrl, MV_NFC_STATUS_BBD |
				MV_NFC_STATUS_RDY | MV_NFC_STATUS_CMDD,
				MV_FALSE);
		if ((info->state != STATE_DMA_READING) ||
		    (info->retcode == ERR_DBERR)) {
			info->state = STATE_READY;
			complete(&info->cmd_complete);
		}
	}
	MV_REG_WRITE(NFC_STATUS_REG, status);
	return IRQ_HANDLED;
}
#endif

static int orion_nfc_cmd_prepare(struct orion_nfc_info *info,
		MV_NFC_MULTI_CMD *descInfo, u32 *numCmds)
{
	MV_U32	i;
	MV_NFC_MULTI_CMD *currDesc;

	currDesc = descInfo;
	if (info->cmd == MV_NFC_CMD_READ_MONOLITHIC) {
		/* Main Chunks */
		for (i = 0; i < CHUNK_CNT; i++) {
			if (i == 0)
				currDesc->cmd = MV_NFC_CMD_READ_MONOLITHIC;
			else if ((i == (CHUNK_CNT-1)) && (LST_CHUNK_SZ == 0) && (LST_CHUNK_SPR == 0))
				currDesc->cmd = MV_NFC_CMD_READ_LAST_NAKED;
			else
				currDesc->cmd = MV_NFC_CMD_READ_NAKED;

			currDesc->pageAddr = info->page_addr;
			currDesc->pageCount = 1;
			currDesc->virtAddr = (MV_U32 *)(info->data_buff + (i * CHUNK_SZ));
			currDesc->physAddr = info->data_buff_phys + (i * CHUNK_SZ);
			currDesc->length = (CHUNK_SZ + CHUNK_SPR);

			if (CHUNK_SPR == 0)
				currDesc->numSgBuffs = 1;
			else {
				currDesc->numSgBuffs = 2;
				currDesc->sgBuffAddr[0] = (info->data_buff_phys + (i * CHUNK_SZ));
				currDesc->sgBuffAddrVirt[0] = (MV_U32 *)(info->data_buff + (i * CHUNK_SZ));
				currDesc->sgBuffSize[0] = CHUNK_SZ;
				currDesc->sgBuffAddr[1] = (info->data_buff_phys + (CHUNK_SZ * CHUNK_CNT) +
										LST_CHUNK_SZ + (i * CHUNK_SPR));
				currDesc->sgBuffAddrVirt[1] = (MV_U32 *)(info->data_buff + (CHUNK_SZ * CHUNK_CNT) +
										LST_CHUNK_SZ + (i * CHUNK_SPR));
				currDesc->sgBuffSize[1] = CHUNK_SPR;
			}

			currDesc++;
		}

		/* Last chunk if existing */
		if ((LST_CHUNK_SZ != 0) || (LST_CHUNK_SPR != 0)) {
			currDesc->cmd = MV_NFC_CMD_READ_LAST_NAKED;
			currDesc->pageAddr = info->page_addr;
			currDesc->pageCount = 1;
			currDesc->length = (LST_CHUNK_SPR + LST_CHUNK_SZ);

			if ((LST_CHUNK_SZ == 0) && (LST_CHUNK_SPR != 0)) {		/* Spare only */
				currDesc->virtAddr = (MV_U32 *)(info->data_buff + (CHUNK_SZ * CHUNK_CNT) +
									LST_CHUNK_SZ + (CHUNK_SPR * CHUNK_CNT));
				currDesc->physAddr = info->data_buff_phys + (CHUNK_SZ * CHUNK_CNT) +
									LST_CHUNK_SZ + (CHUNK_SPR * CHUNK_CNT);
				currDesc->numSgBuffs = 1;
				currDesc->length = LST_CHUNK_SPR;
			} else if ((LST_CHUNK_SZ != 0) && (LST_CHUNK_SPR == 0)) {	/* Data only */
				currDesc->virtAddr = (MV_U32 *)(info->data_buff + (CHUNK_SZ * CHUNK_CNT));
				currDesc->physAddr = info->data_buff_phys + (CHUNK_SZ * CHUNK_CNT);
				currDesc->numSgBuffs = 1;
				currDesc->length = LST_CHUNK_SZ;
			} else {	/* Both spare and data */
				currDesc->numSgBuffs = 2;
				currDesc->sgBuffAddr[0] = (info->data_buff_phys + (CHUNK_SZ * CHUNK_CNT));
				currDesc->sgBuffAddrVirt[0] = (MV_U32 *)(info->data_buff + (CHUNK_SZ * CHUNK_CNT));
				currDesc->sgBuffSize[0] = LST_CHUNK_SZ;
				currDesc->sgBuffAddr[1] = (info->data_buff_phys + (CHUNK_SZ * CHUNK_CNT) +
										LST_CHUNK_SZ + (CHUNK_SPR * CHUNK_CNT));
				currDesc->sgBuffAddrVirt[1] =  (MV_U32 *)(info->data_buff + (CHUNK_SZ * CHUNK_CNT) +
										LST_CHUNK_SZ + (CHUNK_SPR * CHUNK_CNT));
				currDesc->sgBuffSize[1] = LST_CHUNK_SPR;
			}
			currDesc++;
		}

		*numCmds = CHUNK_CNT + (((LST_CHUNK_SZ) || (LST_CHUNK_SPR)) ? 1 : 0);
	} else if (info->cmd == MV_NFC_CMD_WRITE_MONOLITHIC) {
		/* Write Dispatch */
		currDesc->cmd = MV_NFC_CMD_WRITE_DISPATCH_START;
		currDesc->pageAddr = info->page_addr;
		currDesc->pageCount = 1;
		currDesc->numSgBuffs = 1;
		currDesc->length = 0;
		currDesc++;

		/* Main Chunks */
		for (i = 0; i < CHUNK_CNT; i++) {
			currDesc->cmd = MV_NFC_CMD_WRITE_NAKED;
			currDesc->pageAddr = info->page_addr;
			currDesc->pageCount = 1;
			currDesc->virtAddr = (MV_U32 *)(info->data_buff + (i * CHUNK_SZ));
			currDesc->physAddr = info->data_buff_phys + (i * CHUNK_SZ);
			currDesc->length = (CHUNK_SZ + CHUNK_SPR);

			if (CHUNK_SPR == 0)
				currDesc->numSgBuffs = 1;
			else {
				currDesc->numSgBuffs = 2;
				currDesc->sgBuffAddr[0] = (info->data_buff_phys + (i * CHUNK_SZ));
				currDesc->sgBuffAddrVirt[0] = (MV_U32 *)(info->data_buff + (i * CHUNK_SZ));
				currDesc->sgBuffSize[0] = CHUNK_SZ;
				currDesc->sgBuffAddr[1] = (info->data_buff_phys + (CHUNK_SZ * CHUNK_CNT) +
										LST_CHUNK_SZ + (i * CHUNK_SPR));
				currDesc->sgBuffAddrVirt[1] = (MV_U32 *)(info->data_buff + (CHUNK_SZ * CHUNK_CNT) +
										LST_CHUNK_SZ + (i * CHUNK_SPR));
				currDesc->sgBuffSize[1] = CHUNK_SPR;
			}

			currDesc++;
		}

		/* Last chunk if existing */
		if ((LST_CHUNK_SZ != 0) || (LST_CHUNK_SPR != 0)) {
			currDesc->cmd = MV_NFC_CMD_WRITE_NAKED;
			currDesc->pageAddr = info->page_addr;
			currDesc->pageCount = 1;
			currDesc->length = (LST_CHUNK_SZ + LST_CHUNK_SPR);

			if ((LST_CHUNK_SZ == 0) && (LST_CHUNK_SPR != 0)) {		/* Spare only */
				currDesc->virtAddr = (MV_U32 *)(info->data_buff + (CHUNK_SZ * CHUNK_CNT) +
									LST_CHUNK_SZ + (CHUNK_SPR * CHUNK_CNT));
				currDesc->physAddr = info->data_buff_phys + (CHUNK_SZ * CHUNK_CNT) +
									LST_CHUNK_SZ + (CHUNK_SPR * CHUNK_CNT);
				currDesc->numSgBuffs = 1;
			} else if ((LST_CHUNK_SZ != 0) && (LST_CHUNK_SPR == 0)) {	/* Data only */
				currDesc->virtAddr = (MV_U32 *)(info->data_buff + (CHUNK_SZ * CHUNK_CNT));
				currDesc->physAddr = info->data_buff_phys + (CHUNK_SZ * CHUNK_CNT);
				currDesc->numSgBuffs = 1;
			} else {	/* Both spare and data */
				currDesc->numSgBuffs = 2;
				currDesc->sgBuffAddr[0] = (info->data_buff_phys + (CHUNK_SZ * CHUNK_CNT));
				currDesc->sgBuffAddrVirt[0] = (MV_U32 *)(info->data_buff + (CHUNK_SZ * CHUNK_CNT));
				currDesc->sgBuffSize[0] = LST_CHUNK_SZ;
				currDesc->sgBuffAddr[1] = (info->data_buff_phys + (CHUNK_SZ * CHUNK_CNT) +
										LST_CHUNK_SZ + (CHUNK_SPR * CHUNK_CNT));
				currDesc->sgBuffAddrVirt[1] = (MV_U32 *)(info->data_buff + (CHUNK_SZ * CHUNK_CNT) +
										LST_CHUNK_SZ + (CHUNK_SPR * CHUNK_CNT));
				currDesc->sgBuffSize[1] = LST_CHUNK_SPR;
			}
			currDesc++;
		}

		/* Write Dispatch END */
		currDesc->cmd = MV_NFC_CMD_WRITE_DISPATCH_END;
		currDesc->pageAddr = info->page_addr;
		currDesc->pageCount = 1;
		currDesc->numSgBuffs = 1;
		currDesc->length = 0;

		*numCmds = CHUNK_CNT + (((LST_CHUNK_SZ) || (LST_CHUNK_SPR)) ? 1 : 0) + 2;
	} else {
		descInfo[0].cmd = info->cmd;
		descInfo[0].pageAddr = info->page_addr;
		descInfo[0].pageCount = 1;
		descInfo[0].virtAddr = (MV_U32 *)info->data_buff;
		descInfo[0].physAddr = info->data_buff_phys;
		descInfo[0].numSgBuffs = 1;
		descInfo[0].length = info->data_size;
		*numCmds = 1;
	}

	return 0;
}

#ifdef CONFIG_MV_INCLUDE_PDM
static int orion_nfc_do_cmd_dma(struct orion_nfc_info *info,
		uint32_t event)
{
	uint32_t ndcr;
	int ret, timeout = CHIP_DELAY_TIMEOUT;
	MV_STATUS status;
	MV_U32	numCmds;

	/* static allocation to avoid stack overflow*/
	static MV_NFC_MULTI_CMD descInfo[NFC_MAX_NUM_OF_DESCR];

	/* Clear all status bits. */
	MV_REG_WRITE(NFC_STATUS_REG, NFC_SR_MASK);

	mvNfcIntrSet(&info->nfcCtrl, event, MV_TRUE);

	NFC_DPRINT((PRINT_LVL "\nAbout to issue dma cmd %d (cs %d) - 0x%x.\n",
				info->cmd, info->nfcCtrl.currCs,
				MV_REG_READ(NFC_CONTROL_REG)));
	if ((info->cmd == MV_NFC_CMD_READ_MONOLITHIC) ||
	    (info->cmd == MV_NFC_CMD_READ_ID) ||
	    (info->cmd == MV_NFC_CMD_READ_STATUS))
		info->state = STATE_DMA_READING;
	else
		info->state = STATE_CMD_HANDLE;
	info->chained_cmd = 1;

	orion_nfc_cmd_prepare(info, descInfo, &numCmds);

	status = mvNfcCommandMultiple(&info->nfcCtrl, descInfo, numCmds);
	if (status != MV_OK) {
		pr_err("nfcCmdMultiple() failed for cmd %d (%d).\n",
				info->cmd, status);
		goto fail;
	}

	NFC_DPRINT((PRINT_LVL "After issue command %d - 0x%x.\n",
				info->cmd, MV_REG_READ(NFC_STATUS_REG)));

	ret = orion_nfc_wait_for_completion_timeout(info, timeout);
	if (!ret) {
		pr_err("Cmd %d execution timed out (0x%x) - cs %d.\n",
				info->cmd, MV_REG_READ(NFC_STATUS_REG),
				info->nfcCtrl.currCs);
		info->retcode = ERR_CMD_TO;
		goto fail_stop;
	}

	mvNfcIntrSet(&info->nfcCtrl, event | MV_NFC_STATUS_CMDD, MV_FALSE);

	while (MV_PDMA_CHANNEL_STOPPED !=
			mvPdmaChannelStateGet(&info->nfcCtrl.dataChanHndl)) {
		if (info->retcode == ERR_NONE)
			BUG();

	}

	return 0;

fail_stop:
	ndcr = MV_REG_READ(NFC_CONTROL_REG);
	MV_REG_WRITE(NFC_CONTROL_REG, ndcr & ~NFC_CTRL_ND_RUN_MASK);
	udelay(10);
fail:
	return -ETIMEDOUT;
}
#endif

static int orion_nfc_error_check(struct orion_nfc_info *info)
{
	switch (info->cmd) {
	case MV_NFC_CMD_ERASE:
	case MV_NFC_CMD_MULTIPLANE_ERASE:
	case MV_NFC_CMD_WRITE_MONOLITHIC:
	case MV_NFC_CMD_WRITE_MULTIPLE:
	case MV_NFC_CMD_WRITE_NAKED:
	case MV_NFC_CMD_WRITE_LAST_NAKED:
	case MV_NFC_CMD_WRITE_DISPATCH:
	case MV_NFC_CMD_WRITE_DISPATCH_START:
	case MV_NFC_CMD_WRITE_DISPATCH_END:
		if (info->dscr & (MV_NFC_CS0_BAD_BLK_DETECT_INT | MV_NFC_CS1_BAD_BLK_DETECT_INT)) {
			info->retcode = ERR_BBD;
			return 1;
		}
		break;

	case MV_NFC_CMD_CACHE_READ_SEQ:
	case MV_NFC_CMD_CACHE_READ_RAND:
	case MV_NFC_CMD_EXIT_CACHE_READ:
	case MV_NFC_CMD_CACHE_READ_START:
	case MV_NFC_CMD_READ_MONOLITHIC:
	case MV_NFC_CMD_READ_MULTIPLE:
	case MV_NFC_CMD_READ_NAKED:
	case MV_NFC_CMD_READ_LAST_NAKED:
	case MV_NFC_CMD_READ_DISPATCH:
		if (info->dscr & MV_NFC_UNCORR_ERR_INT) {
			info->dscr = ERR_DBERR;
			return 1;
		}
		break;

	default:
		break;
	}

	info->retcode = ERR_NONE;
	return 0;
}

/* ==================================================================================================
 *           STEP  1		|   STEP  2   |   STEP  3   |   STEP  4   |   STEP  5   |   STEP 6
 *           COMMAND		|   WAIT FOR  |   CHK ERRS  |     PIO     |   WAIT FOR  |   CHK ERRS
 * =========================|=============|=============|=============|=============|============
 *   READ MONOLITHIC		|   RDDREQ    |   UNCERR    |    READ     |     NONE    |    NONE
 *   READ NAKED				|   RDDREQ    |   UNCERR    |    READ     |     NONE    |    NONE
 *   READ LAST NAKED		|   RDDREQ    |   UNCERR    |    READ     |     NONE    |    NONE
 *   WRITE MONOLITHIC		|   WRDREQ    |    NONE     |    WRITE    |     RDY     |    BBD
 *   WRITE DISPATCH START	|   CMDD      |    NONE     |    NONE     |     NONE    |    NONE
 *   WRITE NAKED			|   WRDREQ    |    NONE     |    WRITE    |     PAGED   |    NONE
 *   WRITE DISPATCH END		|   NONE      |    NONE     |    NONE     |     RDY     |    BBD
 *   ERASE					|   NONE      |    NONE     |    NONE     |     RDY     |    BBD
 *   READ ID				|   RDDREQ    |    NONE     |    READ     |     NONE    |    NONE
 *   READ STAT				|   RDDREQ    |    NONE     |    READ     |     NONE    |    NONE
 *   RESET					|   NONE      |    NONE     |    NONE     |     RDY     |    NONE
 */
static int orion_nfc_do_cmd_pio(struct orion_nfc_info *info)
{
	int timeout = CHIP_DELAY_TIMEOUT;
	MV_STATUS status;
	MV_U32	i, j, numCmds;
	MV_U32 ndcr;

	/* static allocation to avoid stack overflow */
	static MV_NFC_MULTI_CMD descInfo[NFC_MAX_NUM_OF_DESCR];

	/* Clear all status bits */
	MV_REG_WRITE(NFC_STATUS_REG, NFC_SR_MASK);

	NFC_DPRINT((PRINT_LVL "\nStarting PIO command %d (cs %d) - NDCR=0x%08x\n",
		   info->cmd, info->nfcCtrl.currCs, MV_REG_READ(NFC_CONTROL_REG)));

	/* Build the chain of commands */
	orion_nfc_cmd_prepare(info, descInfo, &numCmds);
	NFC_DPRINT((PRINT_LVL "Prepared %d commands in sequence\n", numCmds));

	/* Execute the commands */
	for (i = 0; i < numCmds; i++) {
		/* Verify that command is supported in PIO mode */
		if ((orion_nfc_cmd_info_lkup[descInfo[i].cmd].events_p1 == 0) &&
		    (orion_nfc_cmd_info_lkup[descInfo[i].cmd].events_p2 == 0)) {
			goto fail_stop;
		}

		/* clear the return code */
		info->dscr = 0;

		/* STEP1: Initiate the command */
		NFC_DPRINT((PRINT_LVL "About to issue Descriptor #%d (command %d, pageaddr 0x%x, length %d).\n",
			    i, descInfo[i].cmd, descInfo[i].pageAddr, descInfo[i].length));
		status = mvNfcCommandPio(&info->nfcCtrl, &descInfo[i], MV_FALSE);
		if (status != MV_OK) {
			pr_err("mvNfcCommandPio() failed for command %d (%d).\n", descInfo[i].cmd, status);
			goto fail_stop;
		}
		NFC_DPRINT((PRINT_LVL "After issue command %d (NDSR=0x%x)\n",
			   descInfo[i].cmd, MV_REG_READ(NFC_STATUS_REG)));

		/* Check if command phase interrupts events are needed */
		if (orion_nfc_cmd_info_lkup[descInfo[i].cmd].events_p1) {
			/* Enable necessary interrupts for command phase */
			NFC_DPRINT((PRINT_LVL "Enabling part1 interrupts (IRQs 0x%x)\n",
				   orion_nfc_cmd_info_lkup[descInfo[i].cmd].events_p1));
			mvNfcIntrSet(&info->nfcCtrl, orion_nfc_cmd_info_lkup[descInfo[i].cmd].events_p1, MV_TRUE);

			/* STEP2: wait for interrupt */
			if (!orion_nfc_wait_for_completion_timeout(info, timeout)) {
				pr_err("command %d execution timed out (CS %d, NDCR=0x%x, NDSR=0x%x).\n",
				       descInfo[i].cmd, info->nfcCtrl.currCs, MV_REG_READ(NFC_CONTROL_REG),
				       MV_REG_READ(NFC_STATUS_REG));
				info->retcode = ERR_CMD_TO;
				goto fail_stop;
			}

			/* STEP3: Check for errors */
			if (orion_nfc_error_check(info)) {
				NFC_DPRINT((PRINT_LVL "Command level errors (DSCR=%08x, retcode=%d)\n",
					   info->dscr, info->retcode));
				goto fail_stop;
			}
		}

		/* STEP4: PIO Read/Write data if needed */
		if (descInfo[i].numSgBuffs > 1) {
			for (j = 0; j < descInfo[i].numSgBuffs; j++) {
				NFC_DPRINT((PRINT_LVL "Starting SG#%d PIO Read/Write (%d bytes, R/W mode %d)\n", j,
					    descInfo[i].sgBuffSize[j],
					    orion_nfc_cmd_info_lkup[descInfo[i].cmd].rw));
				mvNfcReadWritePio(&info->nfcCtrl, descInfo[i].sgBuffAddrVirt[j],
						  descInfo[i].sgBuffSize[j],
						  orion_nfc_cmd_info_lkup[descInfo[i].cmd].rw);
			}
		} else {
			NFC_DPRINT((PRINT_LVL "Starting nonSG PIO Read/Write (%d bytes, R/W mode %d)\n",
				    descInfo[i].length, orion_nfc_cmd_info_lkup[descInfo[i].cmd].rw));
			mvNfcReadWritePio(&info->nfcCtrl, descInfo[i].virtAddr,
					  descInfo[i].length, orion_nfc_cmd_info_lkup[descInfo[i].cmd].rw);
		}

		/* check if data phase events are needed */
		if (orion_nfc_cmd_info_lkup[descInfo[i].cmd].events_p2) {
			/* Enable the RDY interrupt to close the transaction */
			NFC_DPRINT((PRINT_LVL "Enabling part2 interrupts (IRQs 0x%x)\n",
				   orion_nfc_cmd_info_lkup[descInfo[i].cmd].events_p2));
			mvNfcIntrSet(&info->nfcCtrl, orion_nfc_cmd_info_lkup[descInfo[i].cmd].events_p2, MV_TRUE);

			/* STEP5: Wait for transaction to finish */
			if (!orion_nfc_wait_for_completion_timeout(info, timeout)) {
				pr_err("command %d execution timed out (NDCR=0x%08x, NDSR=0x%08x, NDECCCTRL=0x%08x)\n",
				       descInfo[i].cmd, MV_REG_READ(NFC_CONTROL_REG), MV_REG_READ(NFC_STATUS_REG),
				       MV_REG_READ(NFC_ECC_CONTROL_REG));
				info->retcode = ERR_DATA_TO;
				goto fail_stop;
			}

			/* STEP6: Check for errors BB errors (in erase) */
			if (orion_nfc_error_check(info)) {
				NFC_DPRINT((PRINT_LVL "Data level errors (DSCR=0x%08x, retcode=%d)\n",
					   info->dscr, info->retcode));
				goto fail_stop;
			}
		}

		/* Fallback - in case the NFC did not reach the idle state */
		ndcr = MV_REG_READ(NFC_CONTROL_REG);
		if (ndcr & NFC_CTRL_ND_RUN_MASK) {
			NFC_DPRINT((PRINT_LVL "WRONG NFC STAUS: command %d, NDCR=0x%08x, NDSR=0x%08x, NDECCCTRL=0x%08x)\n",
				   info->cmd, MV_REG_READ(NFC_CONTROL_REG), MV_REG_READ(NFC_STATUS_REG),
				   MV_REG_READ(NFC_ECC_CONTROL_REG)));
			MV_REG_WRITE(NFC_CONTROL_REG, (ndcr & ~NFC_CTRL_ND_RUN_MASK));
		}
	}

	NFC_DPRINT((PRINT_LVL "Command done (NDCR=0x%08x, NDSR=0x%08x)\n",
		   MV_REG_READ(NFC_CONTROL_REG), MV_REG_READ(NFC_STATUS_REG)));
	info->retcode = ERR_NONE;

	return 0;

fail_stop:
	ndcr = MV_REG_READ(NFC_CONTROL_REG);
	if (ndcr & NFC_CTRL_ND_RUN_MASK) {
		pr_err("WRONG NFC STAUS: command %d, NDCR=0x%08x, NDSR=0x%08x, NDECCCTRL=0x%08x)\n",
		       info->cmd, MV_REG_READ(NFC_CONTROL_REG), MV_REG_READ(NFC_STATUS_REG),
		       MV_REG_READ(NFC_ECC_CONTROL_REG));
		MV_REG_WRITE(NFC_CONTROL_REG, (ndcr & ~NFC_CTRL_ND_RUN_MASK));
	}
	mvNfcIntrSet(&info->nfcCtrl, 0xFFF, MV_FALSE);
	udelay(10);
	return -ETIMEDOUT;
}

static int orion_nfc_dev_ready(struct mtd_info *mtd)
{
	return (MV_REG_READ(NFC_STATUS_REG) & (NFC_SR_RDY0_MASK | NFC_SR_RDY1_MASK)) ? 1 : 0;
}

static inline int is_buf_blank(uint8_t *buf, size_t len)
{
	for (; len > 0; len--)
		if (*buf++ != 0xff)
			return 0;
	return 1;
}

static void orion_nfc_cmdfunc(struct mtd_info *mtd, unsigned command,
				int column, int page_addr)
{
	struct orion_nfc_info *info = (struct orion_nfc_info *)((struct nand_chip *)mtd->priv)->priv;

	info->data_size = 0;
	info->state = STATE_READY;
	info->chained_cmd = 0;
	info->retcode = ERR_NONE;

	init_completion(&info->cmd_complete);

	switch (command) {
	case NAND_CMD_READOOB:
		info->buf_count = mtd->writesize + mtd->oobsize;
		info->buf_start = mtd->writesize + column;
		info->cmd = MV_NFC_CMD_READ_MONOLITHIC;
		info->column = column;
		info->page_addr = page_addr;
		if (prepare_read_prog_cmd(info, column, page_addr))
			break;

		if (info->use_dma)
#ifdef CONFIG_MV_INCLUDE_PDM
			orion_nfc_do_cmd_dma(info, MV_NFC_STATUS_RDY | NFC_SR_UNCERR_MASK);
#else
			pr_err("DMA mode not supported!\n");
#endif
		else
			orion_nfc_do_cmd_pio(info);

		/* We only are OOB, so if the data has error, does not matter */
		if (info->retcode == ERR_DBERR)
			info->retcode = ERR_NONE;
		break;

	case NAND_CMD_READ0:
		info->buf_start = column;
		info->buf_count = mtd->writesize + mtd->oobsize;
		memset(info->data_buff, 0xff, info->buf_count);
		info->cmd = MV_NFC_CMD_READ_MONOLITHIC;
		info->column = column;
		info->page_addr = page_addr;

		if (prepare_read_prog_cmd(info, column, page_addr))
			break;

		if (info->use_dma)
#ifdef CONFIG_MV_INCLUDE_PDM
			orion_nfc_do_cmd_dma(info, MV_NFC_STATUS_RDY | NFC_SR_UNCERR_MASK);
#else
			pr_err("DMA mode not supported!\n");
#endif
		else
			orion_nfc_do_cmd_pio(info);

		if (info->retcode == ERR_DBERR) {
			/* for blank page (all 0xff), HW will calculate its ECC as
			 * 0, which is different from the ECC information within
			 * OOB, ignore such double bit errors
			 */
			if (is_buf_blank(info->data_buff, mtd->writesize))
				info->retcode = ERR_NONE;
			else
				printk(PRINT_LVL "%s: retCode == ERR_DBERR\n", __func__);
		}
		break;
	case NAND_CMD_SEQIN:
		info->buf_start = column;
		info->buf_count = mtd->writesize + mtd->oobsize;
		memset(info->data_buff + mtd->writesize, 0xff, mtd->oobsize);

		/* save column/page_addr for next CMD_PAGEPROG */
		info->seqin_column = column;
		info->seqin_page_addr = page_addr;
		break;
	case NAND_CMD_PAGEPROG:
		info->column = info->seqin_column;
		info->page_addr = info->seqin_page_addr;
		info->cmd = MV_NFC_CMD_WRITE_MONOLITHIC;
		if (prepare_read_prog_cmd(info,
				info->seqin_column, info->seqin_page_addr)) {
			pr_err("prepare_read_prog_cmd() failed.\n");
			break;
		}

		if (info->use_dma)
#ifdef CONFIG_MV_INCLUDE_PDM
			orion_nfc_do_cmd_dma(info, MV_NFC_STATUS_RDY);
#else
			pr_err("DMA mode not supported!\n");
#endif
		else
			orion_nfc_do_cmd_pio(info);

		break;
	case NAND_CMD_ERASE1:
		info->column = 0;
		info->page_addr = page_addr;
		info->cmd = MV_NFC_CMD_ERASE;

		if (info->use_dma)
#ifdef CONFIG_MV_INCLUDE_PDM
			orion_nfc_do_cmd_dma(info, MV_NFC_STATUS_BBD | MV_NFC_STATUS_RDY);
#else
			pr_err("DMA mode not supported!\n");
#endif
		else
			orion_nfc_do_cmd_pio(info);

		break;
	case NAND_CMD_ERASE2:
		break;
	case NAND_CMD_READID:
	case NAND_CMD_STATUS:
		info->buf_start = 0;
		info->buf_count = (command == NAND_CMD_READID) ?
				info->read_id_bytes : 1;
		info->data_size = 8;
		info->column = 0;
		info->page_addr = 0;
		info->cmd = (command == NAND_CMD_READID) ?
			MV_NFC_CMD_READ_ID : MV_NFC_CMD_READ_STATUS;

		if (info->use_dma)
#ifdef CONFIG_MV_INCLUDE_PDM
			orion_nfc_do_cmd_dma(info, MV_NFC_STATUS_RDY);
#else
			pr_err("DMA mode not supported!\n");
#endif
		else
			orion_nfc_do_cmd_pio(info);

		break;
	case NAND_CMD_RESET:
#if 0
		int ret = 0;

		info->column = 0;
		info->page_addr = 0;
		info->cmd = MV_NFC_CMD_RESET;

		if (info->use_dma)
#ifdef CONFIG_MV_INCLUDE_PDM
			ret = orion_nfc_do_cmd_dma(info, MV_NFC_STATUS_CMDD);
#else
			pr_err("DMA mode not supported!\n");
#endif
		else
			ret = orion_nfc_do_cmd_pio(info);

		if (ret == 0) {
			int timeout = 2;
			uint32_t ndcr;

			while (timeout--) {
				if (MV_REG_READ(NFC_STATUS_REG) & (NFC_SR_RDY0_MASK | NFC_SR_RDY1_MASK))
					break;
				msleep(10);
			}

			ndcr = MV_REG_READ(NFC_CONTROL_REG);
			MV_REG_WRITE(NFC_CONTROL_REG, ndcr & ~NFC_CTRL_ND_RUN_MASK);
		}
#else
#ifdef MTD_NAND_NFC_INIT_RESET
		if (mvNfcReset() != MV_OK)
			pr_err("Device reset failed.\n");
#endif
#endif
		break;
	default:
		pr_err("non-supported command.\n");
		break;
	}

	if (info->retcode == ERR_DBERR) {
		pr_err("double bit error @ page %08x (%d)\n",
		       page_addr, info->cmd);
		info->retcode = ERR_NONE;
	}
}

static uint8_t orion_nfc_read_byte(struct mtd_info *mtd)
{
	struct orion_nfc_info *info = (struct orion_nfc_info *)((struct nand_chip *)mtd->priv)->priv;
	char retval = 0xFF;

	if (info->buf_start < info->buf_count)
		/* Has just send a new command? */
		retval = info->data_buff[info->buf_start++];
	return retval;
}

static u16 orion_nfc_read_word(struct mtd_info *mtd)
{
	struct orion_nfc_info *info = (struct orion_nfc_info *)((struct nand_chip *)mtd->priv)->priv;
	u16 retval = 0xFFFF;

	if (!(info->buf_start & 0x01) && info->buf_start < info->buf_count) {
		retval = *((u16 *)(info->data_buff+info->buf_start));
		info->buf_start += 2;
	} else
		pr_err("\n%s: returning 0xFFFF (%d, %d).\n", __func__, info->buf_start, info->buf_count);

	return retval;
}

static void orion_nfc_read_buf(struct mtd_info *mtd, uint8_t *buf, int len)
{
	struct orion_nfc_info *info = (struct orion_nfc_info *)((struct nand_chip *)mtd->priv)->priv;
	int real_len = min_t(size_t, len, info->buf_count - info->buf_start);

	memcpy(buf, info->data_buff + info->buf_start, real_len);
	info->buf_start += real_len;
}

static void orion_nfc_write_buf(struct mtd_info *mtd,
		const uint8_t *buf, int len)
{
	struct orion_nfc_info *info = (struct orion_nfc_info *)((struct nand_chip *)mtd->priv)->priv;
	int real_len = min_t(size_t, len, info->buf_count - info->buf_start);

	memcpy(info->data_buff + info->buf_start, buf, real_len);
	info->buf_start += real_len;
}

static void orion_nfc_select_chip(struct mtd_info *mtd, int chip)
{
	struct orion_nfc_info *info = (struct orion_nfc_info *)((struct nand_chip *)mtd->priv)->priv;
	mvNfcSelectChip(&info->nfcCtrl, MV_NFC_CS_0 + chip);
	return;
}

static int orion_nfc_waitfunc(struct mtd_info *mtd, struct nand_chip *this)
{
	struct orion_nfc_info *info = (struct orion_nfc_info *)((struct nand_chip *)mtd->priv)->priv;

	/* orion_nfc_send_command has waited for command complete */
	if (this->state == FL_WRITING || this->state == FL_ERASING) {
		if (info->retcode == ERR_NONE)
			return 0;
		else {
			/*
			 * any error make it return 0x01 which will tell
			 * the caller the erase and write fail
			 */
			return 0x01;
		}
	}

	return 0;
}

static void orion_nfc_ecc_hwctl(struct mtd_info *mtd, int mode)
{
	return;
}

static int orion_nfc_ecc_calculate(struct mtd_info *mtd,
		const uint8_t *dat, uint8_t *ecc_code)
{
	return 0;
}

static int orion_nfc_ecc_correct(struct mtd_info *mtd,
		uint8_t *dat, uint8_t *read_ecc, uint8_t *calc_ecc)
{
	struct orion_nfc_info *info = (struct orion_nfc_info *)((struct nand_chip *)mtd->priv)->priv;
	/*
	 * Any error include ERR_SEND_CMD, ERR_DBERR, ERR_BUSERR, we
	 * consider it as a ecc error which will tell the caller the
	 * read fail We have distinguish all the errors, but the
	 * nand_read_ecc only check this function return value
	 */
	if (info->retcode != ERR_NONE)
		return -1;

	return 0;
}

static int orion_nfc_detect_flash(struct orion_nfc_info *info)
{
	MV_U32 my_page_size;

	mvNfcFlashPageSizeGet(&info->nfcCtrl, &my_page_size, NULL);

	/* Translate page size to enum */
	switch (my_page_size) {
	case 512:
		info->page_size = NFC_PAGE_512B;
		break;

	case 2048:
		info->page_size = NFC_PAGE_2KB;
		break;

	case 4096:
		info->page_size = NFC_PAGE_4KB;
		break;

	case 8192:
		info->page_size = NFC_PAGE_8KB;
		break;

	case 16384:
		info->page_size = NFC_PAGE_16KB;
		break;

	default:
		return -EINVAL;
	}

	info->flash_width = info->nfc_width;
	if (info->flash_width != 16 && info->flash_width != 8)
		return -EINVAL;

	/* calculate flash information */
	info->read_id_bytes = (pg_sz[info->page_size] >= 2048) ? 4 : 2;

	return 0;
}

/* the maximum possible buffer size for ganaged 8K page with OOB data
 * is: 2 * (8K + Spare) ==> to be alligned allocate 5 MMU (4K) pages
 */
#define MAX_BUFF_SIZE	(PAGE_SIZE * 5)

static int orion_nfc_init_buff(struct orion_nfc_info *info)
{
	struct platform_device *pdev = info->pdev;

	if (info->use_dma == 0) {
		info->data_buff = devm_kzalloc(&pdev->dev, MAX_BUFF_SIZE,
					       GFP_KERNEL);
		if (info->data_buff == NULL)
			return -ENOMEM;
		return 0;
	}

	info->data_buff = dma_alloc_coherent(&pdev->dev, MAX_BUFF_SIZE,
				&info->data_buff_phys, GFP_KERNEL);
	if (info->data_buff == NULL) {
		dev_err(&pdev->dev, "failed to allocate dma buffer\n");
		return -ENOMEM;
	}
	memset(info->data_buff, 0xff, MAX_BUFF_SIZE);

#ifdef CONFIG_MV_INCLUDE_PDM
	if (pxa_request_dma_intr("nand-data", info->nfcCtrl.dataChanHndl.chanNumber,
			orion_nfc_data_dma_irq, info) < 0) {
		dev_err(&pdev->dev, "failed to request PDMA IRQ\n");
		return -ENOMEM;
	}
#endif
	return 0;
}

static uint8_t mv_bbt_pattern[] = {'M', 'V', 'B', 'b', 't', '0' };
static uint8_t mv_mirror_pattern[] = {'1', 't', 'b', 'B', 'V', 'M' };

static struct nand_bbt_descr mvbbt_main_descr = {
	.options = NAND_BBT_LASTBLOCK | NAND_BBT_CREATE | NAND_BBT_WRITE
		| NAND_BBT_2BIT | NAND_BBT_VERSION,
	.offs =	8,
	.len = 6,
	.veroffs = 14,
	.maxblocks = 8,		/* Last 8 blocks in each chip */
	.pattern = mv_bbt_pattern
};

static struct nand_bbt_descr mvbbt_mirror_descr = {
	.options = NAND_BBT_LASTBLOCK | NAND_BBT_CREATE | NAND_BBT_WRITE
		| NAND_BBT_2BIT | NAND_BBT_VERSION,
	.offs =	8,
	.len = 6,
	.veroffs = 14,
	.maxblocks = 8,		/* Last 8 blocks in each chip */
	.pattern = mv_mirror_pattern
};


static int orion_nfc_markbad(struct mtd_info *mtd, loff_t ofs)
{
	struct nand_chip *chip = mtd->priv;
	uint8_t buf[6] = {0, 0, 0, 0, 0, 0};
	int block, ret = 0;
	loff_t page_addr;

	/* Get block number */
	block = (int)(ofs >> chip->bbt_erase_shift);
	if (chip->bbt)
		chip->bbt[block >> 2] |= 0x01 << ((block & 0x03) << 1);
	ret = nand_update_bbt(mtd, ofs);

	if (ret == 0) {
		/* Get address of the next block */
		ofs += mtd->erasesize;
		ofs &= ~(mtd->erasesize - 1);

		/* Get start of oob in last page */
		ofs -= mtd->oobsize;

		page_addr = ofs;
		do_div(page_addr, mtd->writesize);

		orion_nfc_cmdfunc(mtd, NAND_CMD_SEQIN, mtd->writesize,
				page_addr);
		orion_nfc_write_buf(mtd, buf, 6);
		orion_nfc_cmdfunc(mtd, NAND_CMD_PAGEPROG, 0, page_addr);
	}

	return ret;
}


static void orion_nfc_init_nand(struct nand_chip *nand, struct orion_nfc_info *info)
{

	if (info->nfc_width == 16)
		nand->bbt_options	= (NAND_BBT_USE_FLASH |  NAND_BUSWIDTH_16);
	else
		nand->bbt_options	= NAND_BBT_USE_FLASH;

#if CONFIG_MTD_NAND_NFC_MLC_SUPPORT
	nand->oobsize_ovrd	= ((CHUNK_SPR * CHUNK_CNT) + LST_CHUNK_SPR);
	nand->bb_location	= BB_BYTE_POS;
	nand->bb_page		= mvNfcBadBlockPageNumber(&info->nfcCtrl);
#endif
	nand->waitfunc		= orion_nfc_waitfunc;
	nand->select_chip	= orion_nfc_select_chip;
	nand->dev_ready		= orion_nfc_dev_ready;
	nand->cmdfunc		= orion_nfc_cmdfunc;
	nand->read_word		= orion_nfc_read_word;
	nand->read_byte		= orion_nfc_read_byte;
	nand->read_buf		= orion_nfc_read_buf;
	nand->write_buf		= orion_nfc_write_buf;
	nand->block_markbad	= orion_nfc_markbad;
	nand->ecc.mode		= NAND_ECC_HW;
	nand->ecc.hwctl		= orion_nfc_ecc_hwctl;
	nand->ecc.calculate	= orion_nfc_ecc_calculate;
	nand->ecc.correct	= orion_nfc_ecc_correct;
	nand->ecc.size		= pg_sz[info->page_size];
	nand->ecc.layout	= ECC_LAYOUT;
	/* Driver has to set ecc.strength when using hardware ECC */
	switch (info->ecc_type) {
	case (MV_NFC_ECC_HAMMING):
		nand->ecc.strength = 1;
		break;
	case (MV_NFC_ECC_BCH_2K):
		nand->ecc.strength = 4;
		break;
	case (MV_NFC_ECC_BCH_1K):
		nand->ecc.strength = 8;
		break;
	case (MV_NFC_ECC_BCH_704B):
		nand->ecc.strength = 12;
		break;
	case (MV_NFC_ECC_BCH_512B):
		nand->ecc.strength = 16;
		break;
	default:
		nand->ecc.strength = 0;
	}
	nand->bbt_td		= &mvbbt_main_descr;
	nand->bbt_md		= &mvbbt_mirror_descr;
	nand->badblock_pattern	= BB_INFO;
	nand->chip_delay	= 25;
}

static int mvCtrlNandClkSet(int nfc_clk_freq)
{
	/* NAND clock is derived from ecc_clk according to equation
	 * nfc_clk_freq = ecc_clk / 2
	 */
	clk_set_rate(ecc_clk, nfc_clk_freq * 2);

	/* Return calculated nand clock frequency */
	nfc_clk_freq = clk_get_rate(ecc_clk) / 2;

	return nfc_clk_freq;
}

static MV_STATUS mvSysNfcInit(MV_NFC_INFO *nfcInfo, MV_NFC_CTRL *nfcCtrl)
{
	struct MV_NFC_HAL_DATA halData;

	memset(&halData, 0, sizeof(halData));

	halData.mvCtrlNandClkSetFunction = mvCtrlNandClkSet;

	return mvNfcInit(nfcInfo, nfcCtrl, &halData);
}

static int orion_nfc_probe(struct platform_device *pdev)
{
	struct orion_nfc_info *info;
	struct nand_chip *nand;
	struct mtd_info *mtd;
	struct resource *r;
	int nr_parts = 0;
	int ret, irq;
	char *stat[2] = {"Disabled", "Enabled"};
	char *ecc_stat[] = {"Hamming", "BCH 4bit", "BCH 8bit", "BCH 12bit", "BCH 16bit", "No"};
	struct mtd_part_parser_data ppdata = {};
	struct mtd_partition *parts = NULL;
	struct device_node *np = pdev->dev.of_node;
	MV_NFC_INFO nfcInfo;
	MV_STATUS status;
	MV_U32 mv_nand_offset;

	/* Allocate all data: mtd_info -> nand_chip -> orion_nfc_info */
	mtd = devm_kzalloc(&pdev->dev, sizeof(struct mtd_info), GFP_KERNEL);
	if (!mtd) {
		dev_err(&pdev->dev, "failed to allocate memory for mtd_info\n");
		return -ENOMEM;
	}

	info = devm_kzalloc(&pdev->dev, sizeof(struct orion_nfc_info),
			    GFP_KERNEL);
	if (!info) {
		dev_err(&pdev->dev, "failed to allocate memory for orion_nfc_info\n");
		return -ENOMEM;
	}

	nand = devm_kzalloc(&pdev->dev, sizeof(struct nand_chip), GFP_KERNEL);
	if (!nand) {
		dev_err(&pdev->dev, "failed to allocate memory for nand_chip\n");
		return -ENOMEM;
	}

	ecc_clk = devm_clk_get(&pdev->dev, "ecc_clk");
	if (IS_ERR(ecc_clk)) {
		dev_err(&pdev->dev, "failed to get nand clock\n");
		return PTR_ERR(ecc_clk);
	}
	ret = clk_prepare_enable(ecc_clk);
	if (ret < 0)
		goto fail_put_nand_clk;

	if (of_device_is_compatible(np, "marvell,armada-375-nand")) {
		info->aux_clk = devm_clk_get(&pdev->dev, "gateclk");
		if (IS_ERR(info->aux_clk)) {
			dev_err(&pdev->dev, "failed to get auxiliary clock\n");
			ret = PTR_ERR(info->aux_clk);
			goto fail_put_nand_clk;
		}
		ret = clk_prepare_enable(info->aux_clk);
		if (ret < 0)
			goto fail_put_clk;
	}

	/* Hookup pointers */
	info->pdev = pdev;
	nand->priv = info;
	mtd->priv = nand;
	mtd->name = DRIVER_NAME;
	mtd->owner = THIS_MODULE;

	/* Parse DT tree and acquire all necessary data */
	ret = 0;
	ret |= of_property_read_u32(np, "nfc,nfc-dma", &info->use_dma);
	ret |= of_property_read_u32(np, "nfc,nfc-width", &info->nfc_width);
	ret |= of_property_read_u32(np, "nfc,ecc-type", &info->ecc_type);
	ret |= of_property_read_u32(np, "nfc,num-cs", &info->num_cs);
	ret |= of_property_read_u32(np, "reg", &mv_nand_offset);

	/* Determine the NAND Flash Controller mode for later usage */
	info->nfc_mode = of_get_property(np, "nfc,nfc-mode", NULL);
	if (!info->nfc_mode || (strncmp(info->nfc_mode, "normal", 6) &&
	    strncmp(info->nfc_mode, "ganged", 6))) {
		ret = -EINVAL;
		goto fail_put_clk;
	}

	if (ret != 0) {
		dev_err(&pdev->dev,
		    "missing or bad NAND configuration from device tree\n");
		ret = -ENOENT;
		goto fail_put_clk;
	}

	/* Get IRQ from FDT and map it to the Linux IRQ number */
	irq = irq_of_parse_and_map(pdev->dev.of_node, 0);
	if (irq == 0) {
		dev_err(&pdev->dev,
		    "IRQ number missing in device tree or can't be mapped\n");
		ret = -ENOENT;
		goto fail_put_clk;
	}
	/* Save acquired IRQ mapping */
	info->irq = irq;

	dev_info(&pdev->dev, "Initialize HAL based NFC in %dbit mode with DMA %s using %s ECC\n",
			  info->nfc_width, stat[info->use_dma], ecc_stat[info->ecc_type]);

	r = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	if (r == NULL) {
		dev_err(&pdev->dev, "no IO memory resource defined\n");
		ret = -ENODEV;
		goto fail_dispose_irq;
	}

	r = devm_request_mem_region(&pdev->dev, r->start, r->end - r->start + 1,
				    pdev->name);
	if (r == NULL) {
		dev_err(&pdev->dev, "failed to request memory resource\n");
		ret = -EBUSY;
		goto fail_dispose_irq;
	}

	info->mmio_base = devm_ioremap(&pdev->dev, r->start,
				       r->end - r->start + 1);
	if (info->mmio_base == NULL) {
		dev_err(&pdev->dev, "ioremap() failed\n");
		ret = -ENODEV;
		goto fail_dispose_irq;
	}

	info->mmio_phys_base = r->start;

#ifdef CONFIG_MV_INCLUDE_PDMA
	if (mvPdmaHalInit(MV_PDMA_MAX_CHANNELS_NUM) != MV_OK) {
		dev_err(&pdev->dev, "mvPdmaHalInit() failed.\n");
		goto fail_dispose_irq;
	}
#endif
	/* Initialize NFC HAL */
	mv_nand_base = (MV_U32)info->mmio_base;
	nfcInfo.ioMode = (info->use_dma ? MV_NFC_PDMA_ACCESS : MV_NFC_PIO_ACCESS);
	nfcInfo.eccMode = info->ecc_type;

	if (strncmp(info->nfc_mode, "normal", 6) == 0)
		nfcInfo.ifMode = ((info->nfc_width == 8) ? MV_NFC_IF_1X8 : MV_NFC_IF_1X16);
	else
		nfcInfo.ifMode = MV_NFC_IF_2X8;
	nfcInfo.autoStatusRead = MV_FALSE;
	nfcInfo.readyBypass = MV_FALSE;
	nfcInfo.osHandle = NULL;
	nfcInfo.regsPhysAddr = mv_nand_base - mv_nand_offset;
#ifdef CONFIG_MV_INCLUDE_PDMA
	nfcInfo.dataPdmaIntMask = MV_PDMA_END_OF_RX_INTR_EN | MV_PDMA_END_INTR_EN;
	nfcInfo.cmdPdmaIntMask = 0x0;
#endif

	status = mvSysNfcInit(&nfcInfo, &info->nfcCtrl);
	if (status != MV_OK) {
		dev_err(&pdev->dev, "mvNfcInit() failed. Returned %d\n",
				status);
		goto fail_dispose_irq;
	}

	mvNfcSelectChip(&info->nfcCtrl, MV_NFC_CS_0);
	mvNfcIntrSet(&info->nfcCtrl,  0xFFF, MV_FALSE);
	mvNfcSelectChip(&info->nfcCtrl, MV_NFC_CS_1);
	mvNfcIntrSet(&info->nfcCtrl,  0xFFF, MV_FALSE);
	mvNfcSelectChip(&info->nfcCtrl, MV_NFC_CS_NONE);

	ret = orion_nfc_init_buff(info);
	if (ret)
		goto fail_dispose_irq;

	/* Clear all old events on the status register */
	MV_REG_WRITE(NFC_STATUS_REG, MV_REG_READ(NFC_STATUS_REG));
	if (info->use_dma)
#ifdef CONFIG_MV_INCLUDE_PDMA
		ret = request_irq(irq, orion_nfc_irq_dma, IRQF_DISABLED,
				pdev->name, info);
#else
		pr_err("DMA mode not supported!\n");
#endif
	else
		ret = request_irq(irq, orion_nfc_irq_pio, IRQF_DISABLED,
				pdev->name, info);

	if (ret < 0) {
		dev_err(&pdev->dev, "failed to request IRQ\n");
		goto fail_free_buf;
	}

	ret = orion_nfc_detect_flash(info);
	if (ret) {
		dev_err(&pdev->dev, "failed to detect flash\n");
		ret = -ENODEV;
		goto fail_free_irq;
	}

	orion_nfc_init_nand(nand, info);

	if (nand->ecc.layout == NULL) {
		dev_err(&pdev->dev, "Undefined ECC layout for selected nand device\n");
		ret = -ENXIO;
		goto fail_free_irq;
	}

	platform_set_drvdata(pdev, mtd);

	if (nand_scan(mtd, info->num_cs)) {
		dev_err(&pdev->dev, "failed to scan nand\n");
		ret = -ENXIO;
		goto fail_free_irq;
	}

	ppdata.of_node = pdev->dev.of_node;
	ret = mtd_device_parse_register(mtd, NULL, &ppdata, parts,
					nr_parts);
	if (ret == 0)
		return ret;
	else
		dev_err(&pdev->dev, "MTD device registration filed.\n");

	nand_release(mtd);

fail_free_irq:
	free_irq(irq, info);
fail_free_buf:
	if (info->use_dma)
		dma_free_coherent(&pdev->dev, info->data_buff_size,
			info->data_buff, info->data_buff_phys);
fail_dispose_irq:
	irq_dispose_mapping(info->irq);
fail_put_clk:
	if (of_device_is_compatible(np, "marvell,armada-375-nand"))
		clk_disable_unprepare(info->aux_clk);
fail_put_nand_clk:
	clk_disable_unprepare(ecc_clk);
	return ret;
}

static int orion_nfc_remove(struct platform_device *pdev)
{
	struct mtd_info *mtd = platform_get_drvdata(pdev);
	struct orion_nfc_info *info = (struct orion_nfc_info *)((struct nand_chip *)mtd->priv)->priv;
	struct device_node *np = pdev->dev.of_node;

	platform_set_drvdata(pdev, NULL);

	clk_disable_unprepare(ecc_clk);
	if (of_device_is_compatible(np, "marvell,armada-375-nand"))
		clk_disable_unprepare(info->aux_clk);

	/*del_mtd_device(mtd);*/
	free_irq(info->irq, info);
	irq_dispose_mapping(info->irq);

	if (info->use_dma)
		dma_free_writecombine(&pdev->dev, info->data_buff_size,
				info->data_buff, info->data_buff_phys);

	if (mtd)
		mtd_device_unregister(mtd);

	return 0;
}

#ifdef CONFIG_PM
static int orion_nfc_suspend(struct platform_device *pdev, pm_message_t state)
{
	struct mtd_info *mtd = (struct mtd_info *)platform_get_drvdata(pdev);
	struct orion_nfc_info *info = (struct orion_nfc_info *)((struct nand_chip *)mtd->priv)->priv;

	if (info->state != STATE_READY) {
		dev_err(&pdev->dev, "driver busy, state = %d\n", info->state);
		return -EAGAIN;
	}

#ifdef CONFIG_MV_INCLUDE_PDMA
	/* Store PDMA registers.	*/
	info->pdmaDataLen = 128;
	mvPdmaUnitStateStore(info->pdmaUnitData, &info->pdmaDataLen);
#endif

	/* Store NFC registers.	*/
	info->nfcDataLen = 128;
	mvNfcUnitStateStore(info->nfcUnitData, &info->nfcDataLen);
#if 0
	clk_disable(info->clk);
#endif

	return 0;
}

static int orion_nfc_resume(struct platform_device *pdev)
{
	struct mtd_info *mtd = (struct mtd_info *)platform_get_drvdata(pdev);
	struct orion_nfc_info *info = (struct orion_nfc_info *)((struct nand_chip *)mtd->priv)->priv;
	MV_U32	i;
#if 0
	clk_enable(info->clk);
#endif
#ifdef CONFIG_MV_INCLUDE_PDMA
	/* restore PDMA registers */
	for (i = 0; i < info->pdmaDataLen; i += 2)
		MV_REG_WRITE(info->pdmaUnitData[i], info->pdmaUnitData[i+1]);
#endif
	/* Clear all NAND interrupts */
	MV_REG_WRITE(NFC_STATUS_REG, MV_REG_READ(NFC_STATUS_REG));

	/* restore NAND registers */
	for (i = 0; i < info->nfcDataLen; i += 2)
		MV_REG_WRITE(info->nfcUnitData[i], info->nfcUnitData[i+1]);

	return 0;
}
#else
#define orion_nfc_suspend	NULL
#define orion_nfc_resume	NULL
#endif

static struct of_device_id mv_nfc_dt_ids[] = {
	{ .compatible = "marvell,armada-nand", },
	{ .compatible = "marvell,armada-375-nand", },
	{},
};
MODULE_DEVICE_TABLE(of, mv_nfc_dt_ids);

static struct platform_driver orion_nfc_driver = {
	.driver = {
		.name	= DRIVER_NAME,
		.owner	= THIS_MODULE,
		.of_match_table = of_match_ptr(mv_nfc_dt_ids),
	},
	.probe		= orion_nfc_probe,
	.remove		= orion_nfc_remove,
	.suspend	= orion_nfc_suspend,
	.resume		= orion_nfc_resume,
};

static int __init orion_nfc_init(void)
{
	return platform_driver_register(&orion_nfc_driver);
}
module_init(orion_nfc_init);

static void __exit orion_nfc_exit(void)
{
	platform_driver_unregister(&orion_nfc_driver);
}
module_exit(orion_nfc_exit);

MODULE_ALIAS(DRIVER_NAME);
MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("Armada NAND controller driver");
