/*******************************************************************************
Copyright (C) Marvell International Ltd. and its affiliates

This software file (the "File") is owned and distributed by Marvell
International Ltd. and/or its affiliates ("Marvell") under the following
alternative licensing terms.  Once you have made an election to distribute the
File under one of the following license alternatives, please (i) delete this
introductory statement regarding license alternatives, (ii) delete the two
license alternatives that you have not elected to use and (iii) preserve the
Marvell copyright notice above.

********************************************************************************
Marvell Commercial License Option

If you received this File from Marvell and you have entered into a commercial
license agreement (a "Commercial License") with Marvell, the File is licensed
to you under the terms of the applicable Commercial License.

********************************************************************************
Marvell GPL License Option

If you received this File from Marvell, you may opt to use, redistribute and/or
modify this File in accordance with the terms and conditions of the General
Public License Version 2, June 1991 (the "GPL License"), a copy of which is
available along with the File in the license.txt file or by writing to the Free
Software Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 or
on the worldwide web at http://www.gnu.org/licenses/gpl.txt.

THE FILE IS DISTRIBUTED AS-IS, WITHOUT WARRANTY OF ANY KIND, AND THE IMPLIED
WARRANTIES OF MERCHANTABILITY OR FITNESS FOR A PARTICULAR PURPOSE ARE EXPRESSLY
DISCLAIMED.  The GPL License provides additional details about this warranty
disclaimer.
********************************************************************************
Marvell BSD License Option

If you received this File from Marvell, you may opt to use, redistribute and/or
modify this File under the following licensing terms.
Redistribution and use in source and binary forms, with or without modification,
are permitted provided that the following conditions are met:

    *   Redistributions of source code must retain the above copyright notice,
	    this list of conditions and the following disclaimer.

    *   Redistributions in binary form must reproduce the above copyright
	notice, this list of conditions and the following disclaimer in the
	documentation and/or other materials provided with the distribution.

    *   Neither the name of Marvell nor the names of its contributors may be
	used to endorse or promote products derived from this software without
	specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

*******************************************************************************/

#ifndef __INCMVNFCH
#define __INCMVNFCH

/******************************************************************************
 * Usage model:
 *	The following describes the usage model for the below APIs.
 *	the sequences below does not handle errors, and assume all stages pass
 *	successfully.
 *	Nand Read of 2 pages, PDMA mode:
 *		- mvNfcInit(MV_NFC_PDMA_ACCESS)
 *		- mvNfcSelectChip(...)
 *		- [If interrupt mode]
 *		  Enable RD_DATA_REQ & CMD_DONE interrupts.
 *		- mvNfcCommandIssue(MV_NFC_CMD_READ)
 *		- [In interrupt mode]
 *		  Block till one of the above interrupts is triggered.
 *		- [In polling mode]
 *		  Poll on mvNfcStatusGet() till STATUS_RDD_REQ is returned.
 *		- [In interrupt mode]
 *		  Disable the RD_DATA_REQ interrupt.
 *		- mvNfcReadWrite()
 *		- Block till CMD_DONE interrupt is issued (Or Error).
 *		  OR
 *		  Poll on mvNfcStatusGet() till CMD_DONE is returned. (Or Error).
 *		- Wait for PDMA done interrupt to signal data ready in buffer.
 *		==> At this stage, data is ready in the read buffer.
 *		- [If interrupt mode]
 *		  Enable RD_DATA_REQ & CMD_DONE interrupts.
 *		- mvNfcCommandIssue(MV_NFC_CMD_READ_LAST_NAKED)
 *		- [In interrupt mode]
 *		  Block till one of the above interrupts is triggered.
 *		- [In polling mode]
 *		  Poll on mvNfcStatusGet() till STATUS_RDD_REQ is returned.
 *		- [In interrupt mode]
 *		  Disable the RD_DATA_REQ interrupt.
 *		- mvNfcReadWrite()
 *		- Block till CMD_DONE interrupt is issued (Or Error).
 *		  OR
 *		  Poll on mvNfcStatusGet() till CMD_DONE is returned. (Or Error).
 *		- Wait for PDMA done interrupt to signal data ready in buffer.
 *		==> At this stage, second page data is ready in the read buffer.
 *		- mvNfcSelectChip(MV_NFC_CS_NONE)
 *
 *	Nand Write of single page, PIO mode:
 *		- mvNfcInit(MV_NFC_PIO_ACCESS)
 *		- mvNfcSelectChip(...)
 *		- [If interrupt mode]
 *		  Enable WR_DATA_REQ & CMD_DONE interrupts.
 *		- mvNfcCommandIssue(MV_NFC_CMD_WRITE_MONOLITHIC)
 *		- [In interrupt mode]
 *		  Block till one of the above interrupts is triggered.
 *		- [In polling mode]
 *		  Poll on mvNfcStatusGet() till STATUS_WRD_REQ is returned.
 *		- [In interrupt mode]
 *		  Disable the WR_DATA_REQ interrupt.
 *		- mvNfcReadWrite()
 *		- Block till CMD_DONE interrupt is issued (Or Error).
 *		  OR
 *		  Poll on mvNfcStatusGet() till CMD_DONE is returned. (Or Error).
 *		==> At this stage, data was written to the flash device.
 *		- mvNfcSelectChip(MV_NFC_CS_NONE)
 *
 *	Nand Erase of a single block:
 *		- mvNfcInit(...)
 *		- mvNfcSelectChip(...)
 *		- [If interrupt mode]
 *		  Enable CMD_DONE interrupts.
 *		- mvNfcCommandIssue(MV_NFC_CMD_ERASE)
 *		- [In interrupt mode]
 *		  Block till the above interrupt is triggered.
 *		- [In polling mode]
 *		  Poll on mvNfcStatusGet() till STATUS_CMD_DONE is returned.
 *		==> At this stage, flash block was erased from the flash device.
 *		- mvNfcSelectChip(MV_NFC_CS_NONE)
 *
 ******************************************************************************/


#ifdef __cplusplus
extern "C" {
#endif

#if !defined(CONFIG_OF)
#include "ctrlEnv/mvCtrlEnvSpec.h"
#include "mvSysNfcConfig.h"
#endif

/********************************/
/* Enums and structures		*/
/********************************/

/* Maximum Chain length */
#define MV_NFC_MAX_DESC_CHAIN		0x800

/* Supported page sizes */
#define MV_NFC_512B_PAGE		512
#define MV_NFC_2KB_PAGE			2048
#define MV_NFC_4KB_PAGE			4096
#define MV_NFC_8KB_PAGE			8192

#define MV_NFC_MAX_CHUNK_SIZE		(2048)

/* Nand controller status bits.		*/
#define MV_NFC_STATUS_CMD_REQ		0x1
#define MV_NFC_STATUS_RDD_REQ		0x2
#define MV_NFC_STATUS_WRD_REQ		0x4
#define MV_NFC_STATUS_COR_ERROR		0x8
#define MV_NFC_STATUS_UNC_ERROR		0x10
#define MV_NFC_STATUS_BBD		0x20	/* Bad Block Detected */
#define MV_NFC_STATUS_CMDD		0x80	/* Command Done */
#define MV_NFC_STATUS_PAGED		0x200	/* Page Done */
#define MV_NFC_STATUS_RDY		0x800	/* Device Ready */

/* Nand controller interrupt bits.	*/
#define MV_NFC_WR_CMD_REQ_INT		0x1
#define MV_NFC_RD_DATA_REQ_INT		0x2
#define MV_NFC_WR_DATA_REQ_INT		0x4
#define MV_NFC_CORR_ERR_INT		0x8
#define MV_NFC_UNCORR_ERR_INT		0x10
#define MV_NFC_CS1_BAD_BLK_DETECT_INT	0x20
#define MV_NFC_CS0_BAD_BLK_DETECT_INT	0x40
#define MV_NFC_CS1_CMD_DONE_INT		0x80
#define MV_NFC_CS0_CMD_DONE_INT		0x100
#define MV_NFC_DEVICE_READY_INT		0x800

/* Max number of buffers chunks for as single read / write operation */
#define MV_NFC_RW_MAX_BUFF_NUM		16

#define NUM_OF_PPAGE_BYTES		128

/* ECC mode options.			*/
typedef enum {
	MV_NFC_ECC_HAMMING = 0,		/* 1 bit */
	MV_NFC_ECC_BCH_2K,		/* 4 bit */
	MV_NFC_ECC_BCH_1K,		/* 8 bit */
	MV_NFC_ECC_BCH_704B,		/* 12 bit */
	MV_NFC_ECC_BCH_512B,		/* 16 bit */
	MV_NFC_ECC_DISABLE,
	MV_NFC_ECC_MAX_CNT
} MV_NFC_ECC_MODE;

typedef enum {
	MV_NFC_PIO_ACCESS,
	MV_NFC_PDMA_ACCESS
} MV_NFC_IO_MODE;

typedef enum {
	MV_NFC_PIO_READ,
	MV_NFC_PIO_WRITE,
	MV_NFC_PIO_NONE
} MV_NFC_PIO_RW_MODE;


typedef enum {
	MV_NFC_IF_1X8,
	MV_NFC_IF_1X16,
	MV_NFC_IF_2X8
} MV_NFC_IF_MODE;


/* Flash device CS.			*/
typedef enum {
	MV_NFC_CS_0,
	MV_NFC_CS_1,
	MV_NFC_CS_2,
	MV_NFC_CS_3,
	MV_NFC_CS_NONE
} MV_NFC_CHIP_SEL;


/*
 *	ioMode		The access mode by which the unit will operate (PDMA / PIO).
 *	eccMode		The ECC mode to configure the controller to.
 *	ifMode		The NAND chip connection mode, 8-bit / 16-bit / gang mode.
 *	autoStatusRead	Whether to automatically read the flash status after each
 *			erase / write commands.
 *	tclk		System TCLK.
 *	readyBypass	Whether to wait for the RnB sugnal to be deasserted after
 *			waiting the tR or skip it and move directly to the next step.
 *	osHandle	OS specific handle used for allocating command buffer
 *	regsPhysAddr	Physical address of internal registers (used in DMA
 *			mode only)
 *	dataPdmaIntMask Interrupt mask for PDMA data channel (used in DMA mode
 *			only).
 *	cmdPdmaIntMask	Interrupt mask for PDMA command channel (used in DMA
 *			mode only).
 */
typedef struct {
	MV_NFC_IO_MODE		ioMode;
	MV_NFC_ECC_MODE		eccMode;
	MV_NFC_IF_MODE		ifMode;
	MV_BOOL			autoStatusRead;
	MV_U32			tclk;
	MV_BOOL			readyBypass;
	MV_VOID			*osHandle;
	MV_U32			regsPhysAddr;
#ifdef MV_INCLUDE_PDMA
	MV_U32			dataPdmaIntMask;
	MV_U32			cmdPdmaIntMask;
#endif
} MV_NFC_INFO;


typedef enum {
	MV_NFC_CMD_READ_ID = 0,
	MV_NFC_CMD_READ_STATUS,
	MV_NFC_CMD_ERASE,
	MV_NFC_CMD_MULTIPLANE_ERASE,
	MV_NFC_CMD_RESET,

	MV_NFC_CMD_CACHE_READ_SEQ,
	MV_NFC_CMD_CACHE_READ_RAND,
	MV_NFC_CMD_EXIT_CACHE_READ,
	MV_NFC_CMD_CACHE_READ_START,
	MV_NFC_CMD_READ_MONOLITHIC,
	MV_NFC_CMD_READ_MULTIPLE,
	MV_NFC_CMD_READ_NAKED,
	MV_NFC_CMD_READ_LAST_NAKED,
	MV_NFC_CMD_READ_DISPATCH,

	MV_NFC_CMD_WRITE_MONOLITHIC,
	MV_NFC_CMD_WRITE_MULTIPLE,
	MV_NFC_CMD_WRITE_NAKED,
	MV_NFC_CMD_WRITE_LAST_NAKED,
	MV_NFC_CMD_WRITE_DISPATCH,
	MV_NFC_CMD_WRITE_DISPATCH_START,
	MV_NFC_CMD_WRITE_DISPATCH_END,

	MV_NFC_CMD_COUNT	/* This should be the last enum */

} MV_NFC_CMD_TYPE;


/*
 * Nand information structure.
 *	flashId		The ID of the flash information structure representing the timing
 *			and physical layout data of the flash device.
 *	cmdsetId	The ID of the command-set structure holding the access
 *			commands for the flash device.
 *	flashWidth	Flash device interface width in bits.
 *	autoStatusRead	Whether to automatically read the flash status after each
 *			erase / write commands.
 *	tclk		System TCLK.
 *	readyBypass	Whether to wait for the RnB signal to be deasserted after
 *			waiting the tR or skip it and move directly to the next step.
 *	ioMode		Controller access mode (PDMA / PIO).
 *	eccMode		Flash ECC mode (Hamming, BCH, None).
 *	ifMode		Flash interface mode.
 *	currC		The current flash CS currently being accessed.
 *	dataChanHndl	Pointer to the data DMA channel
 *	cmdChanHndl	Pointer to the command DMA Channel
 *	cmdBuff		Command buffer information (used in DMA only)
 *	regsPhysAddr	Physical address of internal registers (used in DMA
 *			mode only)
 *	dataPdmaIntMask Interrupt mask for PDMA data channel (used in DMA mode
 *			only).
 *	cmdPdmaIntMask	Interrupt mask for PDMA command channel (used in DMA
 *			mode only).
 */
typedef struct {
	MV_U32		flashIdx;
	MV_U32		cmdsetIdx;
	MV_U32		flashWidth;
	MV_U32		dfcWidth;
	MV_BOOL		autoStatusRead;
	MV_BOOL		readyBypass;
	MV_NFC_IO_MODE	ioMode;
	MV_NFC_ECC_MODE	eccMode;
	MV_NFC_IF_MODE	ifMode;
	MV_NFC_CHIP_SEL	currCs;
#ifdef MV_INCLUDE_PDMA
	MV_PDMA_CHANNEL	dataChanHndl;
	MV_PDMA_CHANNEL	cmdChanHndl;
#endif
	MV_BUF_INFO	cmdBuff;
	MV_BUF_INFO	cmdDescBuff;
	MV_BUF_INFO	dataDescBuff;
	MV_U32		regsPhysAddr;
#ifdef MV_INCLUDE_PDMA
	MV_U32		dataPdmaIntMask;
	MV_U32		cmdPdmaIntMask;
#endif
} MV_NFC_CTRL;

/*
 * Nand multi command information structure.
 *	cmd		The command to be issued.
 *	pageAddr	The flash page address to operate on.
 *	pageCount	Number of pages to read / write.
 *	virtAddr	The virtual address of the buffer to copy data to
 *			from (For relevant commands).
 *	physAddr	The physical address of the buffer to copy data to
 *			from (For relevant commands).
 *	The following parameters might only be used when working in Gagned PDMA
 *	and the pageCount must be set to 1.
 *	For ganged mode, the use might need to split the NAND stack read
 *	write buffer into several buffers according to what the HW expects.
 *	e.g. NAND stack expects data in the following format:
 *	---------------------------
 *	| Data (4K) | Spare | ECC |
 *	---------------------------
 *	While NAND controller expects data to be in the following format:
 *	-----------------------------------------------------
 *	| Data (2K) | Spare | ECC | Data (2K) | Spare | ECC |
 *	-----------------------------------------------------
 *	numSgBuffs	Number of buffers to split the HW buffer into
 *			If 1, then buffOffset & buffSize are ignored.
 *	sgBuffAddr	Array holding the address of the buffers into which the
 *			HW data should be split (Or read into).
 *	sgBuffSize	Array holding the size of each sub-buffer, entry "i"
 *			represents the size in bytes of the buffer starting at
 *			offset buffOffset[i].
 */
typedef struct {
	MV_NFC_CMD_TYPE	cmd;
	MV_U32		pageAddr;
	MV_U32		pageCount;
	MV_U32		*virtAddr;
	MV_U32		physAddr;
	MV_U32		numSgBuffs;
	MV_U32		sgBuffAddr[MV_NFC_RW_MAX_BUFF_NUM];
	MV_U32		*sgBuffAddrVirt[MV_NFC_RW_MAX_BUFF_NUM];
	MV_U32		sgBuffSize[MV_NFC_RW_MAX_BUFF_NUM];
	MV_U32		length;
} MV_NFC_MULTI_CMD;

typedef struct {
	MV_U32		cmdb0;
	MV_U32		cmdb1;
	MV_U32		cmdb2;
	MV_U32		cmdb3;
} MV_NFC_CMD;

struct MV_NFC_HAL_DATA {
	void (*mvCtrlNandClkSetFunction) (int);    /* Controller NAND clock div  */
};
/** Micron MT29F NAND driver (ONFI):  Parameter Page Data   */
struct parameter_page_t {
	char signature[5];			/* Parameter page signature (ONFI) */
	MV_U16 rev_num;				/* Revision number */
	MV_U16 feature;				/* Features supported */
	MV_U16 command;				/* Optional commands supported */
	char manufacturer[13];			/* Device manufacturer */
	char model[21];				/* Device part number */
	MV_U8 jedec_id;				/* Manufacturer ID (Micron = 2Ch) */
	MV_U16 date_code;			/* Date code */
	MV_U32 data_bytes_per_page;		/* Number of data bytes per page */
	MV_U16 spare_bytes_per_page;		/* Number of spare bytes per page */
	MV_U32 data_bytes_per_partial_page;	/* Number of data bytes per partial page */
	MV_U16 spare_bytes_per_partial_page;	/* Number of spare bytes per partial page */
	MV_U32 pages_per_block;			/* Number of pages per block */
	MV_U32 blocks_per_lun;			/* Number of blocks per unit */
	MV_U8 luns_per_ce;			/* Number of logical units (LUN) per chip enable */
	MV_U8 num_addr_cycles;			/* Number of address cycles */
	MV_U8 bit_per_cell;			/* Number of bits per cell (1 = SLC; >1= MLC) */
	MV_U16 max_bad_blocks_per_lun;		/* Bad blocks maximum per unit */
	MV_U16 block_endurance;			/* Block endurance */
	MV_U8 guarenteed_valid_blocks;		/* Guaranteed valid blocks at beginning of target */
	MV_U16 block_endurance_guarenteed_valid; /* Block endurance for guaranteed valid blocks */
	MV_U8 num_programs_per_page;		/* Number of programs per page */
	MV_U8 partial_prog_attr;		/* Partial programming attributes */
	MV_U8 num_ECC_bits;			/* Number of bits ECC bits */
	MV_U8 num_interleaved_addr_bits;	/* Number of interleaved address bits */
	MV_U8 interleaved_op_attr;		/* Interleaved operation attributes */
};


/********************************/
/* Functions API		*/
/********************************/
MV_STATUS mvNfcInit(MV_NFC_INFO *nfcInfo, MV_NFC_CTRL *nfcCtrl, struct MV_NFC_HAL_DATA *halData);
MV_STATUS mvNfcSelectChip(MV_NFC_CTRL *nfcCtrl, MV_NFC_CHIP_SEL chip);
MV_STATUS mvNfcCommandPio(MV_NFC_CTRL *nfcCtrl, MV_NFC_MULTI_CMD *cmd_desc, MV_BOOL next);
MV_STATUS mvNfcCommandMultiple(MV_NFC_CTRL *nfcCtrl, MV_NFC_MULTI_CMD *descInfo, MV_U32 descCnt);
MV_U32    mvNfcStatusGet(MV_NFC_CTRL *nfcCtrl, MV_NFC_CMD_TYPE cmd, MV_U32 *value);
MV_STATUS mvNfcIntrSet(MV_NFC_CTRL *nfcCtrl, MV_U32 intMask, MV_BOOL enable);
MV_STATUS mvNfcReadWrite(MV_NFC_CTRL *nfcCtrl, MV_NFC_CMD_TYPE cmd, MV_U32 *virtBufAddr, MV_U32 physBuffAddr);
MV_VOID   mvNfcReadWritePio(MV_NFC_CTRL *nfcCtrl, MV_U32 *buff, MV_U32 data_len, MV_NFC_PIO_RW_MODE mode);
MV_VOID   mvNfcAddress2RowConvert(MV_NFC_CTRL *nfcCtrl, MV_U32 address, MV_U32 *row, MV_U32 *colOffset);
MV_VOID   mvNfcAddress2BlockConvert(MV_NFC_CTRL *nfcCtrl, MV_U32 address, MV_U32 *blk);
MV_8     *mvNfcFlashModelGet(MV_NFC_CTRL *nfcCtrl);
MV_STATUS mvNfcFlashPageSizeGet(MV_NFC_CTRL *nfcCtrl, MV_U32 *size, MV_U32 *totalSize);
MV_STATUS mvNfcFlashBlockSizeGet(MV_NFC_CTRL *nfcCtrl, MV_U32 *size);
MV_STATUS mvNfcFlashBlockNumGet(MV_NFC_CTRL *nfcCtrl, MV_U32 *numBlocks);
MV_STATUS mvNfcDataLength(MV_NFC_CTRL *nfcCtrl, MV_NFC_CMD_TYPE cmd, MV_U32 *data_len);
MV_STATUS mvNfcTransferDataLength(MV_NFC_CTRL *nfcCtrl, MV_NFC_CMD_TYPE cmd, MV_U32 *data_len);
MV_STATUS mvNfcFlashIdGet(MV_NFC_CTRL *nfcCtrl, MV_U32 *flashId);
MV_STATUS mvNfcUnitStateStore(MV_U32 *stateData, MV_U32 *len);
MV_NFC_ECC_MODE mvNfcEccModeSet(MV_NFC_CTRL *nfcCtrl, MV_NFC_ECC_MODE eccMode);
MV_U32    mvNfcBadBlockPageNumber(MV_NFC_CTRL *nfcCtrl);
MV_STATUS mvNfcReset(void);
void mvNfcPrintParamPage(void);

#ifdef __cplusplus
}
#endif


#endif /* __INCMVNFCH */
