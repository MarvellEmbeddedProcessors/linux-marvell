/*
 * Copyright (c) 2010 Imagination Technologies, Inc.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#include <linux/module.h>
#include <linux/delay.h>
#include <linux/mtd/mtd.h>
#include <linux/mtd/partitions.h>
#include <linux/mtd/nand.h>
#include <linux/spi/spi.h>

#include "gd5f_spinand.h"

#define CACHE_BUF			4352
#define BUFSIZE				(10 * 64 * 4096)
#define MAX_WAIT_JIFFIES		(40 * HZ)
#define MAX_WAIT_ERASE_JIFFIES		((HZ * 400) / 1000)
#define AVERAGE_WAIT_JIFFIES		((HZ * 20) / 1000)

#ifdef CONFIG_MTD_SPINAND_ONDIEECC

static int enable_hw_ecc;
static int enable_read_hw_ecc;

static struct nand_ecclayout ecc_layout_4KB_8bit = {
	.eccbytes = 128,
	.eccpos = {
		128, 129, 130, 131, 132, 133, 134, 135,
		136, 137, 138, 139, 140, 141, 142, 143,
		144, 145, 146, 147, 148, 149, 150, 151,
		152, 153, 154, 155, 156, 157, 158, 159,
		160, 161, 162, 163, 164, 165, 166, 167,
		168, 169, 170, 171, 172, 173, 174, 175,
		176, 177, 178, 179, 180, 181, 182, 183,
		184, 185, 186, 187, 188, 189, 190, 191,
		192, 193, 194, 195, 196, 197, 198, 199,
		200, 201, 202, 203, 204, 205, 206, 207,
		208, 209, 210, 211, 212, 213, 214, 215,
		216, 217, 218, 219, 220, 221, 222, 223,
		224, 225, 226, 227, 228, 229, 230, 231,
		232, 233, 234, 235, 236, 237, 238, 239,
		240, 241, 242, 243, 244, 245, 246, 247,
		248, 249, 250, 251, 252, 253, 254, 255
	},
	.oobfree = { {1, 127} }
};
#endif

static struct nand_flash_dev gd5f_spinand_flash_types[] = {
	{
		.name = "4Gb SPI NAND 3.3V",
		/* MID, DID, DID */
		.id = { 0xC8, 0xB4, 0x68 },
		/* Page size in bytes */
		.pagesize = 4096,
		/* Chip size in MB */
		.chipsize = 512,
		/* Erase size in bytes = page size x number pages per block*/
		.erasesize = 4096 * 64,
		.id_len = 3,
		/* OOB size per page in bytes */
		.oobsize = 256,
		/* ECC correctability = no of ECC bits per step */
		.ecc.strength_ds = 8,
		/* EC step */
		.ecc.step_ds = 512
	}
};

/* mtd_to_state - obtain the spinand state from the mtd info provided */
static inline struct spinand_state *mtd_to_state(struct mtd_info *mtd)
{
	struct nand_chip *chip = (struct nand_chip *)mtd->priv;
	struct spinand_info *info = (struct spinand_info *)chip->priv;
	struct spinand_state *state = (struct spinand_state *)info->priv;

	return state;
}

/*
 * spinand_cmd - prepare command to be sent to the SPI nand
 * Set up the command buffer to send to the SPI controller.
 * The command buffer has to be initialized to 0.
 */
static int spinand_cmd(struct spi_device *spi, struct spinand_cmd *cmd)
{
	struct spi_message message;
	struct spi_transfer x[4];
	u8 dummy = 0xff;

	spi_message_init(&message);
	memset(x, 0, sizeof(x));
	/* Command part*/
	x[0].len = 1;
	x[0].tx_buf = &cmd->cmd;
	spi_message_add_tail(&x[0], &message);
	/* Address part */
	if (cmd->n_addr) {
		x[1].len = cmd->n_addr;
		x[1].tx_buf = cmd->addr;
		spi_message_add_tail(&x[1], &message);
	}
	/* Dummy part */
	if (cmd->n_dummy) {
		x[2].len = cmd->n_dummy;
		x[2].tx_buf = &dummy;
		spi_message_add_tail(&x[2], &message);
	}
	/* Data to be transmitted */
	if (cmd->n_tx) {
		x[3].len = cmd->n_tx;
		x[3].tx_buf = cmd->tx_buf;
		spi_message_add_tail(&x[3], &message);
	}
	/* Data to be received */
	if (cmd->n_rx) {
		x[3].len = cmd->n_rx;
		x[3].rx_buf = cmd->rx_buf;
		spi_message_add_tail(&x[3], &message);
	}

	return spi_sync(spi, &message);
}

/*
 * spinand_get_feature - send command to get feature register
 * spinand_set_feature - send command to set feature register
 *
 * The GET FEATURES (0Fh) and SET FEATURES (1Fh) commands are used to
 * alter the device behavior from the default power-on behavior.
 * These commands use a 1-byte feature address to determine which feature
 * is to be read or modified
 */
static int spinand_get_feature(struct spi_device *spi_nand, u8 feature_reg,
			       u8 *value)
{
	struct spinand_cmd cmd = {0};
	int ret;

	cmd.cmd = SPI_NAND_GET_FEATURE_INS;

	/* Check the register address */
	if (feature_reg != SPI_NAND_PROTECTION_REG_ADDR &&
	    feature_reg != SPI_NAND_FEATURE_EN_REG_ADDR &&
	    feature_reg != SPI_NAND_STATUS_REG_ADDR &&
	    feature_reg != SPI_NAND_DS_REG_ADDR)
		return -1;

	cmd.n_addr = 1;
	cmd.addr[0] = feature_reg;
	cmd.n_rx = 1;
	cmd.rx_buf = value;

	ret = spinand_cmd(spi_nand, &cmd);
	if (ret < 0)
		dev_err(&spi_nand->dev, "Error %d read feature reg.\n", ret);
	return ret;
}

static int spinand_set_feature(struct spi_device *spi_nand, u8 feature_reg,
			       u8 value)
{
	int ret;
	struct spinand_cmd cmd = {0};

	cmd.cmd = SPI_NAND_SET_FEATURE;

	/* Check the register address */
	if (feature_reg != SPI_NAND_PROTECTION_REG_ADDR &&
	    feature_reg != SPI_NAND_FEATURE_EN_REG_ADDR &&
		feature_reg != SPI_NAND_STATUS_REG_ADDR &&
		feature_reg != SPI_NAND_DS_REG_ADDR)
		return -1;

	cmd.n_addr = 1;
	cmd.addr[0] = feature_reg;
	cmd.n_tx = 1;
	cmd.tx_buf = &value;

	ret = spinand_cmd(spi_nand, &cmd);
	if (ret < 0)
		dev_err(&spi_nand->dev, "Error %d set feture reg.\n", ret);

	return ret;
}

/*
 * spinand_get_status
 * spinand_get_protection
 * spinand_get_feature_en
 * spinand_get_driver_strength
 *
 * Read the specific feature register using spinand_get_feature
 */
static inline int
spinand_get_status(struct spi_device *spi_nand, u8 *value)
{
	return
	spinand_get_feature(spi_nand, SPI_NAND_STATUS_REG_ADDR, value);
}

static inline int
spinand_get_protection(struct spi_device *spi_nand, u8 *value)
{
	return
	spinand_get_feature(spi_nand, SPI_NAND_PROTECTION_REG_ADDR, value);
}

static inline int
spinand_get_feature_en(struct spi_device *spi_nand, u8 *value)
{
	return
	spinand_get_feature(spi_nand, SPI_NAND_FEATURE_EN_REG_ADDR, value);
}

static inline int
spinand_get_driver_strength(struct spi_device *spi_nand, u8 *value)
{
	return
	spinand_get_feature(spi_nand, SPI_NAND_DS_REG_ADDR, value);
}

/*
 * spinand_set_status
 * spinand_set_protection
 * spinand_set_feature_en
 * spinand_set_driver_strength
 *
 * Set the specific feature register using spinand_set_feature
 */
static inline int
spinand_set_status(struct spi_device *spi_nand, u8 value)
{
	return
	spinand_set_feature(spi_nand, SPI_NAND_STATUS_REG_ADDR, value);
}

static inline int
spinand_set_protection(struct spi_device *spi_nand, u8 value)
{
	return
	spinand_set_feature(spi_nand, SPI_NAND_PROTECTION_REG_ADDR, value);
}

static inline int
spinand_set_feature_en(struct spi_device *spi_nand, u8 value)
{
	return
	spinand_set_feature(spi_nand, SPI_NAND_FEATURE_EN_REG_ADDR, value);
}

static inline int
spinand_set_driver_strength(struct spi_device *spi_nand, u8 value)
{
	return
	spinand_set_feature(spi_nand, SPI_NAND_DS_REG_ADDR, value);
}

/*
 * isNAND_busy - check the operation in progress bit and return
 * if NAND chip is busy or not.
 * This function checks the Operation In Progress (OIP) bit to
 * determine whether the NAND memory is busy with a program execute,
 * page read, block erase or reset command.
 */
static inline int isNAND_busy(struct spi_device *spi_nand)
{
	u8 status;
	int ret;

	/* Read the status register and check the OIP bit */
	ret = spinand_get_status(spi_nand, &status);
	if (ret)
		return ret;
	if (status & SPI_NAND_OIP)
		return 1;
	else
		return 0;
}

/* wait_execution_complete - wait for the current operation to finish */
static inline int wait_execution_complete(struct spi_device *spi_nand,
					  u32 timeout)
{
	int ret;
	unsigned long deadline = jiffies + timeout;

	do {
		ret = isNAND_busy(spi_nand);
		if (!ret)
			return 0;
		if (ret < 0)
			return ret;
	} while (!time_after_eq(jiffies, deadline));

	return -1;
}

/* spinand_reset - send RESET command to NAND device */
static void spinand_reset(struct spi_device *spi_nand)
{
	struct spinand_cmd cmd = {0};
	int ret;

	cmd.cmd = SPI_NAND_RESET;
	ret = spinand_cmd(spi_nand, &cmd);
	if (ret < 0) {
		dev_err(&spi_nand->dev, "Reset SPI NAND failed!\n");
		return;
	}

	/* OIP status can be read from 300ns after reset*/
	udelay(1);
	/* Wait for execution to complete */
	ret = wait_execution_complete(spi_nand, MAX_WAIT_JIFFIES);
	if (ret) {
		if (ret < 0)
			dev_err(&spi_nand->dev,
				"%s: Wait execution complete failed!\n",
				__func__);
		else
			dev_err(&spi_nand->dev,
				"%s: Wait execution complete timedout!\n",
				__func__);
	}
}

/*
 * spinand_write_enable - send command to enable write or erase of
 * the nand cells.
 * Before one can write or erase the nand cells, the write enable
 * has to be set. After write or erase, the write enable bit is
 * automatically cleared.
 */
static int spinand_write_enable(struct spi_device *spi_nand)
{
	struct spinand_cmd cmd = {0};

	cmd.cmd = SPI_NAND_WRITE_ENABLE;

	return spinand_cmd(spi_nand, &cmd);
}

/*
 * spinand_read_id - Read SPI nand ID
 * Byte 0: Manufacture ID
 * Byte 1: Device ID 1
 * Byte 2: Device ID 2
 */
static int spinand_read_id(struct spi_device *spi_nand, u8 *id)
{
	int ret;
	u8 nand_id[3];
	struct spinand_cmd cmd = {0};

	cmd.cmd = SPI_NAND_READ_ID;
	cmd.n_rx = 3;
	cmd.rx_buf = nand_id;
	ret = spinand_cmd(spi_nand, &cmd);
	if (ret < 0) {
		dev_err(&spi_nand->dev, "Error %d reading id\n", ret);
		return ret;
	}
	id[0] = nand_id[0];
	id[1] = nand_id[1];
	id[2] = nand_id[2];

	return ret;
}

/*
 * spinand_read_page_to_cache - send command to read data from the device and
 * into the internal cache
 * The read can specify the page to be read into cache
 * (page_id >= 0, page_id < no_pages, no_pages=no_blocks*no_pages_per_block)
 * no_blocks and no_pages_per_block depend on the size of the flash
 */
static int spinand_read_page_to_cache(struct spi_device *spi_nand, u32 page_id)
{
	struct spinand_cmd cmd = {0};

	cmd.cmd = SPI_NAND_PAGE_READ_INS;
	cmd.n_addr = 3;
	cmd.addr[0] = (u8)((page_id & 0x00FF0000) >> 16);
	cmd.addr[1] = (u8)((page_id & 0x0000FF00) >> 8);
	cmd.addr[2] = (u8)(page_id & 0x000000FF);

	return spinand_cmd(spi_nand, &cmd);
}

/*
 * spinand_read_from_cache - send command to read out the data from the
 * cache register
 * The read can specify a byte offset within the page
 * (byte_id >= 0, byte_id < size_of_page)
 * The read can specify a length to be read (len > 0 && len < size_of_page)
 * size_of_page depends on the size of the flash
 */
static int spinand_read_from_cache(struct spi_device *spi_nand,
				   u16 byte_id, u16 len, u8 *rbuf)
{
	struct spinand_cmd cmd = {0};

	cmd.cmd = SPI_NAND_READ_CACHE_INS;
	cmd.n_addr = 3;
	cmd.addr[0] = 0;
	cmd.addr[1] = (u8)((byte_id & 0x0000FF00) >> 8);
	cmd.addr[2] = (u8)(byte_id & 0x000000FF);
	cmd.n_rx = len;
	cmd.rx_buf = rbuf;

	return spinand_cmd(spi_nand, &cmd);
}

/*
 * spinand_read_page - read data from the flash by first reading the
 * corresponding page into the internal cache and after reading out the
 * data from it.
 * The read can specify the page to be read into cache
 * (page_id >= 0, page_id < no_pages, no_pages=no_blocks*no_pages_per_block)
 * no_blocks and no_pages_per_block depend on the size of the flash
 * The read can specify a byte offset within the page
 * (byte_id >= 0, byte_id < size_of_page)
 * The read can specify a length to be read (len > 0 && len < size_of_page)
 * size_of_page depends on the size of the flash
 */
static int spinand_read_page(struct spi_device *spi_nand, u32 page_id,
			     u16 offset, u16 len, u8 *rbuf)
{
	int ret;
	u8 feature_reg, status;

	/* Enable ECC if HW ECC available */
#ifdef CONFIG_MTD_SPINAND_ONDIEECC
	if (enable_read_hw_ecc) {
		if ((spinand_get_feature_en(spi_nand, &feature_reg) < 0) ||
		    (spinand_set_feature_en(spi_nand, feature_reg |
					    SPI_NAND_ECC_EN) < 0))
			dev_err(&spi_nand->dev, "Enable HW ECC failed.");
	}
#endif

	/* Read page from device to internal cache */
	ret = spinand_read_page_to_cache(spi_nand, page_id);
	if (ret < 0) {
		dev_err(&spi_nand->dev, "Error %d reading page to cache.\n",
			ret);
		return ret;
	}

	/* Wait until the operation completes or a timeout occurs. */
	ret = wait_execution_complete(spi_nand, MAX_WAIT_JIFFIES);
	if (ret) {
		if (ret < 0) {
			dev_err(&spi_nand->dev,
				"%s: Wait execution complete failed!\n",
				__func__);
			return ret;
		}
			dev_err(&spi_nand->dev,
				"%s: Wait execution complete timedout!\n",
				__func__);
			return -1;
	}

	/* Check status register for uncorrectable errors */
	ret = spinand_get_status(spi_nand, &status);
	if (ret < 0) {
		dev_err(&spi_nand->dev, "Error %d reading status register.\n",
			(int)ret);
		return ret;
	}
	status &= SPI_NAND_ECC_UNABLE_TO_CORRECT;
	if (status  == SPI_NAND_ECC_UNABLE_TO_CORRECT) {
		dev_err(&spi_nand->dev, "ECC error reading page %d.\n",
			page_id);
		return -1;
	}

	/* Read page from internal cache to our buffers */
	ret = spinand_read_from_cache(spi_nand, offset, len, rbuf);
	if (ret < 0) {
		dev_err(&spi_nand->dev, "Error %d reading from cache.\n",
			(int)ret);
		return ret;
	}

	/* Disable ECC if HW ECC available */
#ifdef CONFIG_MTD_SPINAND_ONDIEECC
	if (enable_read_hw_ecc) {
		if ((spinand_get_feature_en(spi_nand, &feature_reg) < 0) ||
		    (spinand_set_feature_en(spi_nand, feature_reg &
						(~SPI_NAND_ECC_EN)) < 0))
			dev_err(&spi_nand->dev, "Disable HW ECC failed.");
		enable_read_hw_ecc = 0;
	}
#endif
	return ret;
}

/*
 * spinand_program_data_to_cache - send command to program data to cache
 * The write can specify a byte offset within the page
 * (byte_id >= 0, byte_id < size_of_page)
 * The write can specify a length to be written
 * (len > 0 && len < size_of_page)
 * size_of_page depends on the size of the flash
 */
static int spinand_program_data_to_cache(struct spi_device *spi_nand,
					 u16 byte_id, u16 len, u8 *wbuf)
{
	struct spinand_cmd cmd = {0};

	cmd.cmd = SPI_NAND_PROGRAM_LOAD_INS;
	cmd.n_addr = 2;
	cmd.addr[0] = (u8)((byte_id & 0x0000FF00) >> 8);
	cmd.addr[1] = (u8)(byte_id & 0x000000FF);
	cmd.n_tx = len;
	cmd.tx_buf = wbuf;

	return spinand_cmd(spi_nand, &cmd);
}

/*
 * spinand_program_execute - writes a page from cache to NAND array
 * The write can specify the page to be programmed
 * (page_id >= 0, page_id < no_pages, no_pages=no_blocks*no_pages_per_block)
 * no_blocks and no_pages_per_block depend on the size of the flash
 */
static int spinand_program_execute(struct spi_device *spi_nand, u16 page_id)
{
	struct spinand_cmd cmd = {0};

	cmd.cmd = SPI_NAND_PROGRAM_EXEC_INS;
	cmd.n_addr = 3;
	cmd.addr[0] = (u8)((page_id & 0x00FF0000) >> 16);
	cmd.addr[1] = (u8)((page_id & 0x0000FF00) >> 8);
	cmd.addr[2] = (u8)(page_id & 0x000000FF);

	return spinand_cmd(spi_nand, &cmd);
}

/*
 * spinand_program_page - sequence to program a page
 * The write can specify the page to be programmed
 * (page_id >= 0, page_id < no_pages, no_pages=no_blocks*no_pages_per_block)
 * no_blocks and no_pages_per_block depend on the size of the flash
 * The write can specify a byte offset within the page
 * (byte_id >= 0, byte_id < size_of_page)
 * The write can specify a length to be written
 * (len > 0 && len < size_of_page)
 * size_of_page depends on the size of the flash
 * Command sequence: WRITE ENABLE, PROGRAM LOAD, PROGRAM EXECUTE,
 * GET FEATURE command to read the status
 */
static int spinand_program_page(struct spi_device *spi_nand,
				u16 page_id, u16 offset, u16 len, u8 *buf)
{
	int ret;
	u8 status, feature_reg;
	u8 *wbuf;

	/* Enable ECC if HW ECC available */
#ifdef CONFIG_MTD_SPINAND_ONDIEECC
	unsigned int i, j;

	enable_read_hw_ecc = 0;
	wbuf = devm_kzalloc(&spi_nand->dev, CACHE_BUF, GFP_KERNEL);
	ret = spinand_read_page(spi_nand, page_id, 0, CACHE_BUF, wbuf);
	if (ret < 0) {
		dev_err(&spi_nand->dev, "Error %d on page read.\n",
			(int)ret);
		return ret;
	}
	for (i = offset, j = 0; j < len; i++, j++)
		wbuf[i] &= buf[j];
	len += offset;
	if (enable_hw_ecc) {
		if ((spinand_get_feature_en(spi_nand, &feature_reg) < 0) ||
		    (spinand_set_feature_en(spi_nand, feature_reg |
						SPI_NAND_ECC_EN) < 0))
			dev_err(&spi_nand->dev, "Enable HW ECC failed.");
	}
#else
	wbuf = buf;
#endif

	/* Enable capability of programming NAND cells */
	ret = spinand_write_enable(spi_nand);
	if (ret < 0) {
		dev_err(&spi_nand->dev, "Error %d on write enable.\n",
			(int)ret);
		return ret;
	}

	/* Issue program cache command */
	ret = spinand_program_data_to_cache(spi_nand, offset, len, wbuf);
	if (ret < 0) {
		dev_err(&spi_nand->dev, "Error %d when programming cache.\n",
			(int)ret);
		return ret;
	}

	/* Issue program execute command */
	ret = spinand_program_execute(spi_nand, page_id);
	if (ret < 0) {
		dev_err(&spi_nand->dev, "Error %d when programming NAND cells.\n",
			(int)ret);
		return ret;
	}

	/* Wait until the operation completes or a timeout occurs. */
	ret = wait_execution_complete(spi_nand, MAX_WAIT_JIFFIES);
	if (ret) {
		if (ret < 0) {
			dev_err(&spi_nand->dev,
				"%s: Wait execution complete failed!\n",
				__func__);
			return ret;
		}
			dev_err(&spi_nand->dev,
				"%s Wait execution complete timedout!\n",
				__func__);
			return -1;
	}

	/* Check status register for program fail bit */
	ret = spinand_get_status(spi_nand, &status);
	if (ret < 0) {
		dev_err(&spi_nand->dev, "Error %d reading status register.\n",
			(int)ret);
		return ret;
	}
	if (status & SPI_NAND_PF) {
		dev_err(&spi_nand->dev, "Program failed on page %d\n", page_id);
		return -1;
	}

	/* Disable ECC if HW ECC available */
#ifdef CONFIG_MTD_SPINAND_ONDIEECC
	if (enable_hw_ecc) {
		if ((spinand_get_feature_en(spi_nand, &feature_reg) < 0) ||
		    (spinand_set_feature_en(spi_nand,
		     feature_reg & (~SPI_NAND_ECC_EN)) < 0))
			dev_err(&spi_nand->dev, "Disable HW ECC failed.");
		enable_hw_ecc = 0;
	}
#endif

	return 0;
}

#ifdef CONFIG_MTD_SPINAND_ONDIEECC
static int spinand_write_page_hwecc(struct mtd_info *mtd,
				    struct nand_chip *chip, const uint8_t *buf, int oob_required, int page)
{
	enable_hw_ecc = 1;
	chip->write_buf(mtd, buf, chip->ecc.size * chip->ecc.steps);
	return 0;
}

static int spinand_read_page_hwecc(struct mtd_info *mtd, struct nand_chip *chip,
				   u8 *buf, int oob_required, int page)
{
	u8 status;
	int ret;
	struct spinand_info *info = (struct spinand_info *)chip->priv;

	enable_read_hw_ecc = 1;

	/* Read data and OOB area */
	chip->read_buf(mtd, buf, chip->ecc.size * chip->ecc.steps);
	if (oob_required)
		chip->read_buf(mtd, chip->oob_poi, mtd->oobsize);

	/* Wait until the operation completes or a timeout occurs. */
	ret = wait_execution_complete(info->spi, MAX_WAIT_JIFFIES);
	if (ret) {
		if (ret < 0) {
			pr_err("%s: Wait execution complete failed!\n",
			       __func__);
			return ret;
		}
		pr_err("%s: Wait execution complete timedout!\n",
		       __func__);
			return -1;
	}

	/* Check status register for uncorrectable errors */
	ret = spinand_get_status(info->spi, &status);
	if (ret < 0) {
		pr_err("Error %d reading status register.\n", ret);
		return ret;
	}
	status &= SPI_NAND_ECC_UNABLE_TO_CORRECT;
	if (status  == SPI_NAND_ECC_UNABLE_TO_CORRECT) {
		pr_info("ECC error reading page.\n");
		mtd->ecc_stats.failed++;
	}
	if (status && (status != SPI_NAND_ECC_UNABLE_TO_CORRECT))
		mtd->ecc_stats.corrected++;
	return 0;
}
#endif

/*
 * spinand_erase_block_command - erase a block
 * The erase can specify the block to be erased
 * (block_id >= 0, block_id < no_blocks)
 * no_blocks depends on the size of the flash
 */
static int spinand_erase_block_command(struct spi_device *spi_nand,
				       u16 block_id)
{
	struct spinand_cmd cmd = {0};

	cmd.cmd = SPI_NAND_BLOCK_ERASE_INS;
	cmd.n_addr = 3;
	cmd.addr[0] = 0;
	cmd.addr[1] = (u8)((block_id & 0x0000FF00) >> 8);
	cmd.addr[2] = (u8)(block_id & 0x000000FF);

	return spinand_cmd(spi_nand, &cmd);
}

/*
 * spinand_erase_block - erase a block
 * The erase can specify the block to be erased
 * (block_id >= 0, block_id < no_blocks)
 * no_blocks depends on the size of the flash
 * Command sequence: WRITE ENBALE, BLOCK ERASE,
 * GET FEATURES command to read the status register
 */
static int spinand_erase_block(struct spi_device *spi_nand, u16 block_id)
{
	int ret;
	u8 status;

	/* Enable capability of erasing NAND cells */
	ret = spinand_write_enable(spi_nand);
	if (ret < 0) {
		dev_err(&spi_nand->dev, "Error %d on write enable.\n",
			(int)ret);
		return ret;
	}

	ret = spinand_erase_block_command(spi_nand, block_id);
	if (ret < 0) {
		dev_err(&spi_nand->dev, "Error %d when erasing block.\n",
			(int)ret);
		return ret;
	}
	ret = wait_execution_complete(spi_nand, MAX_WAIT_JIFFIES);
	if (ret) {
		if (ret < 0) {
			dev_err(&spi_nand->dev,
				"%s: Wait execution complete failed!\n",
				__func__);
			return ret;
		}
			dev_err(&spi_nand->dev,
				"%s: Wait execution complete timedout!\n",
				__func__);
			return -1;
	}

	/* Check status register for erase fail bit */
	ret = spinand_get_status(spi_nand, &status);
	if (ret < 0) {
		dev_err(&spi_nand->dev, "Error %d reading status register.\n",
			(int)ret);
		return ret;
	}
	if (status & SPI_NAND_EF) {
		dev_err(&spi_nand->dev, "Erase fail on block %d\n", block_id);
		return -1;
	}
	return 0;
}

static void spinand_write_buf(struct mtd_info *mtd, const uint8_t *buf, int len)
{
	struct spinand_state *state = mtd_to_state(mtd);

	memcpy(state->buf + state->buf_ptr, buf, len);
	state->buf_ptr += len;
}

static void spinand_read_buf(struct mtd_info *mtd, uint8_t *buf, int len)
{
	struct spinand_state *state = mtd_to_state(mtd);

	memcpy(buf, state->buf + state->buf_ptr, len);
	state->buf_ptr += len;
}

static uint8_t spinand_read_byte(struct mtd_info *mtd)
{
	struct spinand_state *state = mtd_to_state(mtd);
	u8 data;

	data = state->buf[state->buf_ptr];
	state->buf_ptr++;
	return data;
}

static int spinand_wait(struct mtd_info *mtd, struct nand_chip *chip)
{
	struct spinand_info *info = (struct spinand_info *)chip->priv;
	int state = chip->state;
	u32 timeout;

	if (state == FL_ERASING)
		timeout = MAX_WAIT_ERASE_JIFFIES;
	else
		timeout = AVERAGE_WAIT_JIFFIES;
	return wait_execution_complete(info->spi, timeout);
}

static void spinand_select_chip(struct mtd_info *mtd, int dev)
{
}

static void spinand_cmdfunc(struct mtd_info *mtd, unsigned int command,
			    int column, int page)
{
	struct nand_chip *chip = (struct nand_chip *)mtd->priv;
	struct spinand_info *info = (struct spinand_info *)chip->priv;
	struct spinand_state *state = (struct spinand_state *)info->priv;

	switch (command) {
	case NAND_CMD_READ1:
	case NAND_CMD_READ0:
		state->buf_ptr = 0;
		spinand_read_page(info->spi, page, 0, mtd->writesize,
				  state->buf);
		break;
	case NAND_CMD_READOOB:
		state->buf_ptr = 0;
		spinand_read_page(info->spi, page, mtd->writesize, mtd->oobsize,
				  state->buf);
		break;
	case NAND_CMD_RNDOUT:
		state->buf_ptr = column;
		break;
	case NAND_CMD_READID:
		state->buf_ptr = 0;
		spinand_read_id(info->spi, (u8 *)state->buf);
		break;
	case NAND_CMD_PARAM:
		state->buf_ptr = 0;
		break;
	/* ERASE1 performs the entire erase operation*/
	case NAND_CMD_ERASE1:
		spinand_erase_block(info->spi, page);
		break;
	/* SEQIN sets up the addr buffer and all registers except the length */
	case NAND_CMD_SEQIN:
		state->col = column;
		state->row = page;
		state->buf_ptr = 0;
		break;
	/* PAGEPROG reuses all of the setup from SEQIN and adds the length */
	case NAND_CMD_PAGEPROG:
		spinand_program_page(info->spi, state->row, state->col,
				     state->buf_ptr, state->buf);
		break;
	/* RESET command */
	case NAND_CMD_RESET:
		if (wait_execution_complete(info->spi, MAX_WAIT_JIFFIES))
			dev_err(&info->spi->dev,
				"%s Wait execution complete timedout!\n",
				__func__);
		spinand_reset(info->spi);
		break;
	default:
		dev_err(&mtd->dev, "Command 0x%x not implementd or unknown.\n",
			command);
	}
}

static int gd5f_ecc_init(struct nand_ecc_ctrl *ecc, int strength,
			 int ecc_stepsize, int page_size)
{
	if (strength == 8 && ecc_stepsize == 512 && page_size == 4096) {
		ecc->mode = NAND_ECC_HW;
		ecc->size = 512;
		ecc->bytes = 16;
		ecc->steps = 8;
		ecc->strength = 8;
		ecc->total = ecc->steps * ecc->bytes;
		ecc->read_page = spinand_read_page_hwecc;
		ecc->write_page = spinand_write_page_hwecc;
		ecc->layout = &ecc_layout_4KB_8bit;
	} else {
		pr_err("ECC strength %d at page size %d is not supported\n",
		       strength, page_size);
			return -ENODEV;
	}
	return 0;
}

/*
 * spinand_probe - set up the device driver parameters to make
 * the device available.
 */
static int spinand_probe(struct spi_device *spi_nand)
{
	struct mtd_info *mtd;
	struct nand_chip *chip;
	struct spinand_info *info;
	struct spinand_state *state;
	struct mtd_part_parser_data ppdata;
	struct nand_flash_dev gd9f_flash_dev[2];
	u16 id;
	int i, num, ret;

	/* Allocate, verify and initialize spinand_info */
	info  = devm_kzalloc(&spi_nand->dev, sizeof(struct spinand_info),
			     GFP_KERNEL);
	if (!info)
		return -ENOMEM;
	info->spi = spi_nand;

	/*
	 * Allocate and verify spinand_state
	 * Add a reference to it in the spinand_info structure
	 */
	state = devm_kzalloc(&spi_nand->dev, sizeof(struct spinand_state),
			     GFP_KERNEL);
	if (!state)
		return -ENOMEM;
	info->priv = state;

	/* Allocate and verify buffer for data */
	state->buf_ptr = 0;
	state->buf = devm_kzalloc(&spi_nand->dev, BUFSIZE, GFP_KERNEL);
	if (!state->buf)
		return -ENOMEM;

	/* Allocate and verify nand chip */
	chip = devm_kzalloc(&spi_nand->dev, sizeof(struct nand_chip),
			    GFP_KERNEL);
	if (!chip)
		return -ENOMEM;

	/* Allocate and verify mtd info structure */
	mtd = devm_kzalloc(&spi_nand->dev, sizeof(struct mtd_info), GFP_KERNEL);
	if (!mtd)
		return -ENOMEM;
	dev_set_drvdata(&spi_nand->dev, mtd);
	mtd->priv = chip;
	mtd->name = dev_name(&spi_nand->dev);
	mtd->owner = THIS_MODULE;

	/* Fill chip structure with handle functions */
	chip->priv	= info;
	chip->read_buf	= spinand_read_buf;
	chip->write_buf	= spinand_write_buf;
	chip->read_byte	= spinand_read_byte;
	chip->cmdfunc	= spinand_cmdfunc;
	chip->waitfunc	= spinand_wait;
	chip->options	|= NAND_CACHEPRG;
	chip->select_chip = spinand_select_chip;

	/* Read ID and establish type of chip */
	chip->cmdfunc(mtd, NAND_CMD_READID, 0, 0);
	id = *((u16 *)(state->buf));
	num = ARRAY_SIZE(gd5f_spinand_flash_types);
	for (i = 0; i < num; i++) {
		if (*((u16 *)(gd5f_spinand_flash_types[i].id)) == id)
			break;
	}
	if (i == num) {
		pr_err("Error! Flash is not defined.\n");
		return -EINVAL;
	}

	memcpy(&gd9f_flash_dev[0], gd5f_spinand_flash_types + i,
	       sizeof(struct nand_flash_dev));
	gd9f_flash_dev[1].name = NULL;
	/* This should set up mtd->writesize, mtd->oobsize, etc. */
	if (nand_scan_ident(mtd, 1, gd9f_flash_dev))
		return -ENXIO;

	/* Establish HW ECC parameters if there is on die ECC */
#ifdef CONFIG_MTD_SPINAND_ONDIEECC
	gd5f_ecc_init(&chip->ecc, chip->ecc_strength_ds, chip->ecc_step_ds,
		      mtd->writesize);
#else
	chip->ecc.mode	= NAND_ECC_SOFT;
	if (spinand_disable_ecc(spi_nand) < 0)
		pr_info("%s: Disable ecc failed!\n", __func__);
#endif

	ppdata.of_node = spi_nand->dev.of_node;

	/* Unlock the device */
	ret = spinand_set_protection(spi_nand, SPI_NAND_PROTECTED_ALL_UNLOCKED);
	if (ret < 0) {
		pr_info("%s: Unlocking device failed!\n", __func__);
		return ret;
	}

	return mtd_device_parse_register(mtd, NULL, &ppdata, NULL, 0);
}

/*
 * spinand_remove: Remove the device driver parameters and
 * free up allocated memory
 */
static int spinand_remove(struct spi_device *spi)
{
	mtd_device_unregister(dev_get_drvdata(&spi->dev));
	return 0;
}

static const struct of_device_id gd5f_spinand_dt_ids[] = {
	{ .compatible = "gigadevice,gd5f", },
};

/* Device name structure description */
static struct spi_driver spinand_driver = {
	.driver = {
		.name		= "gd5f",
		.bus		= &spi_bus_type,
		.owner		= THIS_MODULE,
		.of_match_table	= gd5f_spinand_dt_ids,
	},
	.probe		= spinand_probe,
	.remove		= spinand_remove,
};

module_spi_driver(spinand_driver);

MODULE_DESCRIPTION("SPI NAND driver for GigaDevice flash");
MODULE_AUTHOR("Ionela Voinescu <ionela.voinescu at imgtec.com>");
MODULE_LICENSE("GPL v2");
