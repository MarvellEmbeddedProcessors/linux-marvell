/*
 * Marvell Armada-3700 SPI controller driver
 *
 * Author: Wilson Ding <dingwei@marvell.com>
 * Copyright (C) 2007-2008 Marvell Ltd.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/clk.h>
#include <linux/completion.h>
#include <linux/delay.h>
#include <linux/err.h>
#include <linux/interrupt.h>
#include <linux/io.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/of.h>
#include <linux/of_irq.h>
#include <linux/of_device.h>
#include <linux/spi/spi.h>


#define DRIVER_NAME					"armada_3700_spi"

#define A3700_SPI_TIMEOUT			10 /* 10 msec */

/* SPI Register Offest */
#define A3700_SPI_IF_CTRL_REG		0x00
#define A3700_SPI_IF_CFG_REG			0x04
#define A3700_SPI_DATA_OUT_REG		0x08
#define A3700_SPI_DATA_IN_REG		0x0C
#define A3700_SPI_IF_INST_REG		0x10
#define A3700_SPI_IF_ADDR_REG		0x14
#define A3700_SPI_IF_RMODE_REG		0x18
#define A3700_SPI_IF_HDR_CNT_REG		0x1C
#define A3700_SPI_IF_DIN_CNT_REG		0x20
#define A3700_SPI_IF_TIME_REG		0x24
#define A3700_SPI_INT_STAT_REG		0x28
#define A3700_SPI_INT_MASK_REG		0x2C

/* A3700_SPI_IF_CTRL_REG */
#define A3700_SPI_EN					(1 << 16)
#define A3700_SPI_ADDR_NOT_CONFIG	(1 << 12)
#define A3700_SPI_WFIFO_OVERFLOW		(1 << 11)
#define A3700_SPI_WFIFO_UNDERFLOW	(1 << 10)
#define A3700_SPI_RFIFO_OVERFLOW		(1 << 9)
#define A3700_SPI_RFIFO_UNDERFLOW	(1 << 8)
#define A3700_SPI_WFIFO_FULL			(1 << 7)
#define A3700_SPI_WFIFO_EMPTY		(1 << 6)
#define A3700_SPI_RFIFO_FULL			(1 << 5)
#define A3700_SPI_RFIFO_EMPTY		(1 << 4)
#define A3700_SPI_WFIFO_RDY			(1 << 3)
#define A3700_SPI_RFIFO_RDY			(1 << 2)
#define A3700_SPI_XFER_RDY			(1 << 1)
#define A3700_SPI_XFER_DONE			(1 << 0)

/* A3700_SPI_IF_CFG_REG */
#define A3700_SPI_WFIFO_THRS			(1 << 28)
#define A3700_SPI_RFIFO_THRS			(1 << 24)
#define A3700_SPI_AUTO_CS			(1 << 20)
#define A3700_SPI_DMA_RD_EN			(1 << 18)
#define A3700_SPI_FIFO_MODE			(1 << 17)
#define A3700_SPI_SRST				(1 << 16)
#define A3700_SPI_XFER_START			(1 << 15)
#define A3700_SPI_XFER_STOP			(1 << 14)
#define A3700_SPI_INST_PIN			(1 << 13)
#define A3700_SPI_ADDR_PIN			(1 << 12)
#define A3700_SPI_FIFO_FLUSH			(1 << 9)
#define A3700_SPI_RW_EN				(1 << 8)
#define A3700_SPI_CLK_POL			(1 << 7)
#define A3700_SPI_CLK_PHA			(1 << 6)
#define A3700_SPI_BYTE_LEN			(1 << 5)
#define A3700_SPI_CLK_PRESCALE		(1 << 0)
#define A3700_SPI_CLK_PRESCALE_MASK		(0x1f)

#define A3700_SPI_WFIFO_THRS_BIT		28
#define A3700_SPI_RFIFO_THRS_BIT		24
#define A3700_SPI_FIFO_THRS_MASK		0x7

#define A3700_SPI_DATA_PIN_BIT		10
#define A3700_SPI_DATA_PIN_MASK		0x3

/* A3700_SPI_IF_HDR_CNT_REG */
#define A3700_SPI_DUMMY_CNT_BIT		12
#define A3700_SPI_DUMMY_CNT_MASK		0x7
#define A3700_SPI_RMODE_CNT_BIT		8
#define A3700_SPI_RMODE_CNT_MASK		0x3
#define A3700_SPI_ADDR_CNT_BIT		4
#define A3700_SPI_ADDR_CNT_MASK		0x7
#define A3700_SPI_INSTR_CNT_BIT		0
#define A3700_SPI_INSTR_CNT_MASK		0x3


struct a3700_spi_initdata {
	unsigned int cs_num;
	unsigned int mode;
	unsigned int bits_per_word_mask;
	unsigned int instr_cnt;
	unsigned int addr_cnt;
};

/* struct a3700_spi .flags */
#define HAS_FIFO					(1 << 0)
#define AUTO_CS						(1 << 1)
#define XFER_POLL					(1 << 2)

/* PIN mode */
enum a3700_spi_pin_mode {
	A3700_SPI_SGL_PIN,
	A3700_SPI_DUAL_PIN,
	A3700_SPI_QUAD_PIN,
};

struct a3700_spi_status {
	bool                    cs_active;
	bool                    last_xfer;
	const u8               *tx_buf;
	u8                     *rx_buf;
	unsigned int			buf_len;
	unsigned int            byte_len;
	unsigned int            wait_mask;
	struct completion       done;
};

struct a3700_max_hdr_cnt {
	unsigned int            addr_cnt;
	unsigned int            instr_cnt;
	unsigned int            hdr_cnt; /* addr_cnt + instr_cnt = hdr_cnt */
};

struct a3700_spi {
	struct spi_master      *master;
	void __iomem           *base;
	struct clk             *clk;
	unsigned int	input_clk_freq;
	unsigned int	max_clk_freq;
	unsigned int            irq;
	unsigned int            flags;
	enum a3700_spi_pin_mode  pin_mode;
	struct a3700_max_hdr_cnt max_cnt;
	struct a3700_spi_status  status;

	int (*spi_pre_xfer)(struct spi_device *spi, struct spi_transfer *xfer);
	int (*spi_do_xfer)(struct spi_device *spi);
	int (*spi_post_xfer)(struct spi_device *spi, struct spi_transfer *xfer);
	bool (*spi_wait_xfer)(struct spi_device *spi);
};


static u32 spireg_read(struct a3700_spi *a3700_spi, u32 offset)
{
	return readl(a3700_spi->base + offset);
}

static void spireg_write(struct a3700_spi *a3700_spi, u32 offset, u32 data)
{
	writel(data, a3700_spi->base + offset);
}

static void a3700_spi_auto_cs_set(struct a3700_spi *a3700_spi)
{
	u32 val;

	val = spireg_read(a3700_spi, A3700_SPI_IF_CFG_REG);
	if (a3700_spi->flags & AUTO_CS)
		val |= A3700_SPI_AUTO_CS;
	else
		val &= ~A3700_SPI_AUTO_CS;
	spireg_write(a3700_spi, A3700_SPI_IF_CFG_REG, val);
}

static void a3700_spi_activate_cs(struct a3700_spi *a3700_spi, unsigned int cs)
{
	u32 val;

	val = spireg_read(a3700_spi, A3700_SPI_IF_CTRL_REG);
	val |= (A3700_SPI_EN << cs);
	spireg_write(a3700_spi, A3700_SPI_IF_CTRL_REG, val);

	a3700_spi->status.cs_active = true;
}

static void a3700_spi_deactivate_cs(struct a3700_spi *a3700_spi,
	unsigned int cs)
{
	u32 val;

	val = spireg_read(a3700_spi, A3700_SPI_IF_CTRL_REG);
	val &= ~(A3700_SPI_EN << cs);
	spireg_write(a3700_spi, A3700_SPI_IF_CTRL_REG, val);

	a3700_spi->status.cs_active = false;
}

static int a3700_spi_pin_mode_set(struct a3700_spi *a3700_spi)
{
	u32 val;

	if (a3700_spi->pin_mode > A3700_SPI_SGL_PIN)
		return -EINVAL;

	/*
	 * Only support single mode: 1x_instr_pin,
	 * 1x_addr_pin and 1x_data_pin.
	 */
	val = spireg_read(a3700_spi, A3700_SPI_IF_CTRL_REG);
	val &= ~(A3700_SPI_INST_PIN | A3700_SPI_ADDR_PIN);
	val &= ~(A3700_SPI_DATA_PIN_MASK << A3700_SPI_DATA_PIN_BIT);
	spireg_write(a3700_spi, A3700_SPI_IF_CTRL_REG, val);

	return 0;
}

static void a3700_spi_fifo_mode_set(struct a3700_spi *a3700_spi)
{
	u32 val;

	val = spireg_read(a3700_spi, A3700_SPI_IF_CFG_REG);
	if (a3700_spi->flags & HAS_FIFO)
		val |= A3700_SPI_FIFO_MODE;
	else
		val &= ~A3700_SPI_FIFO_MODE;
	spireg_write(a3700_spi, A3700_SPI_IF_CFG_REG, val);
}

static int a3700_spi_fifo_flush(struct a3700_spi *a3700_spi)
{
	int timeout = A3700_SPI_TIMEOUT;
	u32 val;

	val = spireg_read(a3700_spi, A3700_SPI_IF_CFG_REG);
	val |= A3700_SPI_FIFO_FLUSH;
	spireg_write(a3700_spi, A3700_SPI_IF_CFG_REG, val);

	while (--timeout) {
		val = spireg_read(a3700_spi, A3700_SPI_IF_CFG_REG);
		if (!(val & A3700_SPI_FIFO_FLUSH))
			return 0;
		udelay(1);
	}

	return -ETIMEDOUT;
}

static void a3700_spi_mode_set(struct a3700_spi *a3700_spi,
	unsigned int mode_bits)
{
	u32 val;

	val = spireg_read(a3700_spi, A3700_SPI_IF_CFG_REG);
	if (mode_bits & SPI_CPOL)
		val |= A3700_SPI_CLK_POL;
	if (mode_bits & SPI_CPHA)
		val |= A3700_SPI_CLK_PHA;
	spireg_write(a3700_spi, A3700_SPI_IF_CFG_REG, val);
}

static int a3700_spi_baudrate_set(struct a3700_spi *a3700_spi,
	unsigned int speed_hz)
{
	u32 val;
	u32 prescale;

	/* calculate Prescaler = (spi_input_freq / spi_max_freq) */
	prescale = a3700_spi->input_clk_freq / a3700_spi->max_clk_freq;

	val = spireg_read(a3700_spi, A3700_SPI_IF_CFG_REG);
	val = val & ~A3700_SPI_CLK_PRESCALE_MASK;

	val = val | (prescale & A3700_SPI_CLK_PRESCALE_MASK);
	spireg_write(a3700_spi, A3700_SPI_IF_CFG_REG, val);

	return 0;
}

static void a3700_spi_bytelen_set(struct a3700_spi *a3700_spi, unsigned int len)
{
	u32 val;

	val = spireg_read(a3700_spi, A3700_SPI_IF_CFG_REG);
	if (len == 4)
		val |= A3700_SPI_BYTE_LEN;
	else
		val &= ~A3700_SPI_BYTE_LEN;
	spireg_write(a3700_spi, A3700_SPI_IF_CFG_REG, val);

	a3700_spi->status.byte_len = len;
}

static void a3700_spi_fifo_thres_set(struct a3700_spi *a3700_spi,
	unsigned int bytes)
{
	u32 val;

	if (a3700_spi->flags & HAS_FIFO) {
		val = spireg_read(a3700_spi, A3700_SPI_IF_CFG_REG);
		val &= ~(A3700_SPI_FIFO_THRS_MASK << A3700_SPI_RFIFO_THRS_BIT);
		val |= (bytes - 1) << A3700_SPI_RFIFO_THRS_BIT;
		val &= ~(A3700_SPI_FIFO_THRS_MASK << A3700_SPI_WFIFO_THRS_BIT);
		val |= (7 - bytes) << A3700_SPI_WFIFO_THRS_BIT;
		spireg_write(a3700_spi, A3700_SPI_IF_CFG_REG, val);
	}
}

static int a3700_spi_init(struct a3700_spi *a3700_spi)
{
	struct spi_master *master = a3700_spi->master;
	u32 val;
	int ret = 0;
	int i;

	/* Reset SPI unit */
	val = spireg_read(a3700_spi, A3700_SPI_IF_CFG_REG);
	val |= A3700_SPI_SRST;
	spireg_write(a3700_spi, A3700_SPI_IF_CFG_REG, val);

	for (i = 0; i < A3700_SPI_TIMEOUT; i++)
		udelay(1);

	val = spireg_read(a3700_spi, A3700_SPI_IF_CFG_REG);
	val &= ~A3700_SPI_SRST;
	spireg_write(a3700_spi, A3700_SPI_IF_CFG_REG, val);

	/* Disable AUTO_CS and deactivate all chip-selects */
	a3700_spi_auto_cs_set(a3700_spi);
	for (i = 0; i < master->num_chipselect; i++)
		a3700_spi_deactivate_cs(a3700_spi, i);

	/* Set PIN mode */
	ret = a3700_spi_pin_mode_set(a3700_spi);
	if (ret) {
		pr_err("pin mode set failed\n");
		goto out;
	}

	/* Enable FIFO mode and flush FIFO */
	a3700_spi_fifo_mode_set(a3700_spi);
	ret = a3700_spi_fifo_flush(a3700_spi);
	if (ret) {
		pr_err("fifo flush failed\n");
		goto out;
	}

	/* Set SPI mode */
	a3700_spi_mode_set(a3700_spi, master->mode_bits);

	/* Reset counters */
	spireg_write(a3700_spi, A3700_SPI_IF_HDR_CNT_REG, 0);
	spireg_write(a3700_spi, A3700_SPI_IF_DIN_CNT_REG, 0);

	/* Mask the interrupts and clear cause bits */
	spireg_write(a3700_spi, A3700_SPI_INT_MASK_REG, 0);
	spireg_write(a3700_spi, A3700_SPI_INT_STAT_REG, ~0U);

out:
	return ret;
}

static bool a3700_spi_wait_ctl_bit_set(struct spi_device *spi)
{
	struct a3700_spi *a3700_spi;
	int i;
	u32 val;

	a3700_spi = spi_master_get_devdata(spi->master);

	for (i = 0; i < A3700_SPI_TIMEOUT; i++) {
		val = spireg_read(a3700_spi, A3700_SPI_IF_CTRL_REG);
		if (val & a3700_spi->status.wait_mask)
			break;
		udelay(1);
	}

	a3700_spi->status.wait_mask = 0;

	return i == A3700_SPI_TIMEOUT ? false : true;
}

static irqreturn_t a3700_spi_interrupt(int irq, void *dev_id)
{
	struct spi_master *master = dev_id;
	struct a3700_spi *a3700_spi;
	u32 cause;

	a3700_spi = spi_master_get_devdata(master);

	/* Get interrupt causes */
	cause = spireg_read(a3700_spi, A3700_SPI_INT_STAT_REG);

	/* mask and acknowledge the SPI interrupts */
	spireg_write(a3700_spi, A3700_SPI_INT_MASK_REG, 0);
	spireg_write(a3700_spi, A3700_SPI_INT_STAT_REG, cause);

	/* Wake up the transfer */
	if (a3700_spi->status.wait_mask & cause)
		complete(&a3700_spi->status.done);

	return IRQ_HANDLED;
}

static bool a3700_spi_wait_completion(struct spi_device *spi)
{
	struct a3700_spi *a3700_spi;
	unsigned int timeout;
	unsigned int ctrl_reg;

	a3700_spi = spi_master_get_devdata(spi->master);

	/* SPI interrupt is edge-triggered, which means a interrupt will
	  * be generated only when detecting specific status bit changed
	  * from '0' to '1'. So when we start waiting for a interrupt, we
	  * need to check status bit in control reg first, if it is already 1,
	  * then do not need to wait for interrupt
	*/
	ctrl_reg = spireg_read(a3700_spi, A3700_SPI_IF_CTRL_REG);
	if (a3700_spi->status.wait_mask & ctrl_reg)
		return true;

	reinit_completion(&a3700_spi->status.done);

	spireg_write(a3700_spi, A3700_SPI_INT_MASK_REG,
		       a3700_spi->status.wait_mask);

	timeout = wait_for_completion_timeout(&a3700_spi->status.done,
			msecs_to_jiffies(A3700_SPI_TIMEOUT));

	a3700_spi->status.wait_mask = 0;

	if (timeout == 0) {
		/* there might be the case that right after we checked the
		  * status bits in this routine and before start to wait for
		  * interrupt by wait_for_completion_timeout, the interrupt
		  * happens, to avoid missing it we need to double check
		  * status bits in control reg, if it is already 1, then
		  * consider that we have the interrupt successfully and
		  * return true.
		  */
		ctrl_reg = spireg_read(a3700_spi, A3700_SPI_IF_CTRL_REG);
		if (a3700_spi->status.wait_mask & ctrl_reg)
			return true;

		spireg_write(a3700_spi, A3700_SPI_INT_MASK_REG, 0);
		return false;
	}

	return true;
}

static bool a3700_spi_transfer_wait(struct spi_device *spi,
	unsigned int bit_mask)
{
	struct a3700_spi *a3700_spi;

	a3700_spi = spi_master_get_devdata(spi->master);
	a3700_spi->status.wait_mask = bit_mask;

	return a3700_spi->spi_wait_xfer(spi);
}

static int a3700_spi_transfer_setup(struct spi_device *spi,
	struct spi_transfer *xfer)
{
	struct a3700_spi *a3700_spi;
	unsigned int byte_len;
	int	ret = 0;

	a3700_spi = spi_master_get_devdata(spi->master);

	if (!(a3700_spi->flags & AUTO_CS) && !a3700_spi->status.cs_active) {
		/* Set SPI clock prescaler */
		ret = a3700_spi_baudrate_set(a3700_spi, xfer->speed_hz);
		if (ret) {
			dev_err(&spi->dev, "set baudrate failed\n");
			goto out;
		}

		byte_len = xfer->bits_per_word >> 3;

		/* Set byte length */
		a3700_spi_bytelen_set(a3700_spi, byte_len);

		/* Set FIFO threshold */
		a3700_spi_fifo_thres_set(a3700_spi, byte_len);

		/* Activate CS */
		a3700_spi_activate_cs(a3700_spi, spi->chip_select);
	}

out:
	return ret;
}

/* Legacy mode -- non FIFO mode */
static int a3700_spi_transfer_start_legacy(struct spi_device *spi,
	struct spi_transfer *xfer)
{
	struct a3700_spi *a3700_spi;

	a3700_spi = spi_master_get_devdata(spi->master);

	a3700_spi->status.tx_buf  = xfer->tx_buf;
	a3700_spi->status.rx_buf  = xfer->rx_buf;
	a3700_spi->status.buf_len = xfer->len;

	if (!a3700_spi_transfer_wait(spi, A3700_SPI_XFER_RDY)) {
		dev_err(&spi->dev, "wait transfer ready timed out\n");
		return -ETIMEDOUT;
	}

	/* Start READ transfer by writing dummy data to DOUT register */
	if (xfer->rx_buf)
		spireg_write(a3700_spi, A3700_SPI_DATA_OUT_REG, 0);

	return 0;
}

static int a3700_spi_read_data(struct a3700_spi *a3700_spi)
{
	struct a3700_spi_status *status = &a3700_spi->status;
	u32 val;

	if (status->buf_len % status->byte_len)
		return -EINVAL;

	/* Read bytes from data in register */
	val = spireg_read(a3700_spi, A3700_SPI_DATA_IN_REG);
	if (status->byte_len == 4) {
		status->rx_buf[0] = (val >> 24) & 0xff;
		status->rx_buf[1] = (val >> 16) & 0xff;
		status->rx_buf[2] = (val >> 8) & 0xff;
		status->rx_buf[3] = val & 0xff;
	} else
		status->rx_buf[0] = val & 0xff;

	status->buf_len -= status->byte_len;
	status->rx_buf  += status->byte_len;

	/* Request next 1 or 4 bytes data */
	if (status->buf_len)
		spireg_write(a3700_spi, A3700_SPI_DATA_OUT_REG, 0);

	return 0;
}

static int a3700_spi_write_data(struct a3700_spi *a3700_spi)
{
	struct a3700_spi_status *status = &a3700_spi->status;
	u32 val;

	if (status->buf_len % status->byte_len)
		return -EINVAL;

	/* Write bytes from data out register */
	val = 0;
	if (status->byte_len == 4) {
		val |= status->tx_buf[0] << 24;
		val |= status->tx_buf[1] << 16;
		val |= status->tx_buf[2] << 8;
		val |= status->tx_buf[3];
	} else
		val = status->tx_buf[0];

	spireg_write(a3700_spi, A3700_SPI_DATA_OUT_REG, val);

	status->buf_len -= status->byte_len;
	status->tx_buf  += status->byte_len;

	return 0;
}

static int a3700_spi_do_transfer_legacy(struct spi_device *spi)
{
	struct a3700_spi *a3700_spi;
	int ret = 0;

	a3700_spi = spi_master_get_devdata(spi->master);

	while (a3700_spi->status.buf_len) {
		if (!a3700_spi_transfer_wait(spi, A3700_SPI_XFER_RDY)) {
			dev_err(&spi->dev, "wait transfer ready timed out\n");
			ret = -ETIMEDOUT;
			goto error;
		}

		if (a3700_spi->status.tx_buf) {
			ret = a3700_spi_write_data(a3700_spi);
			if (ret)
				goto error;
		}

		if (a3700_spi->status.rx_buf) {
			ret = a3700_spi_read_data(a3700_spi);
			if (ret)
				goto error;
		}
	}

out:
	return ret;
error:
	a3700_spi_deactivate_cs(a3700_spi, spi->chip_select);
	goto out;
}

static int a3700_spi_transfer_finish_legacy(struct spi_device *spi,
	struct spi_transfer *xfer)
{
	struct a3700_spi *a3700_spi;

	a3700_spi = spi_master_get_devdata(spi->master);

	if (!a3700_spi_transfer_wait(spi, A3700_SPI_XFER_RDY)) {
		dev_err(&spi->dev, "wait transfer ready timed out\n");
		return -ETIMEDOUT;
	}

	if (a3700_spi->status.last_xfer || xfer->cs_change)
		a3700_spi_deactivate_cs(a3700_spi, spi->chip_select);

	return 0;
}

/* Non legacy mode -- FIFO mode */
static void a3700_spi_header_set(struct a3700_spi *a3700_spi)
{
	struct a3700_spi_status *status = &a3700_spi->status;
	struct a3700_max_hdr_cnt *max_cnt = &a3700_spi->max_cnt;
	unsigned int instr_cnt, addr_cnt;
	u32 val;

	instr_cnt = addr_cnt = 0;

	/* Clear the header registers */
	spireg_write(a3700_spi, A3700_SPI_IF_INST_REG, 0);
	spireg_write(a3700_spi, A3700_SPI_IF_ADDR_REG, 0);
	spireg_write(a3700_spi, A3700_SPI_IF_RMODE_REG, 0);

	/* Set header counters */
	val = 0;
	if (status->buf_len <= max_cnt->hdr_cnt && status->tx_buf) {
		instr_cnt = min(status->buf_len, max_cnt->instr_cnt);
		addr_cnt  = status->buf_len - max_cnt->instr_cnt;
		val |= ((instr_cnt & A3700_SPI_INSTR_CNT_MASK)
			<< A3700_SPI_INSTR_CNT_BIT);
		val |= ((addr_cnt & A3700_SPI_ADDR_CNT_MASK)
			<< A3700_SPI_ADDR_CNT_BIT);
	}
	spireg_write(a3700_spi, A3700_SPI_IF_HDR_CNT_REG, val);

	/* Update the buffer length to be transferred */
	status->buf_len -= (instr_cnt + addr_cnt);

	/* Set Instruction */
	val = 0;
	while (instr_cnt--) {
		val = (val << 8) | status->tx_buf[0];
		status->tx_buf++;
	}
	spireg_write(a3700_spi, A3700_SPI_IF_INST_REG, val);

	/* Set Address */
	val = 0;
	while (addr_cnt--) {
		val = (val << 8) | status->tx_buf[0];
		status->tx_buf++;
	}
	spireg_write(a3700_spi, A3700_SPI_IF_ADDR_REG, val);
}

static int a3700_spi_transfer_start_non_legacy(struct spi_device *spi,
	struct spi_transfer *xfer)
{
	struct a3700_spi *a3700_spi;
	u32 val;

	a3700_spi = spi_master_get_devdata(spi->master);

	a3700_spi->status.tx_buf  = xfer->tx_buf;
	a3700_spi->status.rx_buf  = xfer->rx_buf;
	a3700_spi->status.buf_len = xfer->len;

	/* Transfer headers */
	a3700_spi_header_set(a3700_spi);

	if (xfer->rx_buf) {
		/* Set read data length */
		spireg_write(a3700_spi, A3700_SPI_IF_DIN_CNT_REG,
			a3700_spi->status.buf_len);
		/* Start READ transfer */
		val = spireg_read(a3700_spi, A3700_SPI_IF_CFG_REG);
		val &= ~A3700_SPI_RW_EN;
		val |= A3700_SPI_XFER_START;
		spireg_write(a3700_spi, A3700_SPI_IF_CFG_REG, val);
	} else if (xfer->tx_buf) {
		/* Start Write transfer */
		val = spireg_read(a3700_spi, A3700_SPI_IF_CFG_REG);
		val |= (A3700_SPI_XFER_START | A3700_SPI_RW_EN);
		spireg_write(a3700_spi, A3700_SPI_IF_CFG_REG, val);
	}

	return 0;
}

static int a3700_is_rfifo_empty(struct a3700_spi *a3700_spi)
{
	u32 val = spireg_read(a3700_spi, A3700_SPI_IF_CTRL_REG);

	return (val & A3700_SPI_RFIFO_EMPTY);
}

static int a3700_spi_fifo_read(struct a3700_spi *a3700_spi)
{
	struct a3700_spi_status *status = &a3700_spi->status;
	u32 val;

	if (status->buf_len % status->byte_len)
		return -EINVAL;

	while (!a3700_is_rfifo_empty(a3700_spi) && status->buf_len) {
		/* Read bytes from data in register */
		val = spireg_read(a3700_spi, A3700_SPI_DATA_IN_REG);
		if (status->byte_len == 4) {
			status->rx_buf[0] = (val >> 24) & 0xff;
			status->rx_buf[1] = (val >> 16) & 0xff;
			status->rx_buf[2] = (val >> 8) & 0xff;
			status->rx_buf[3] = val & 0xff;
		} else
			status->rx_buf[0] = val & 0xff;


		status->buf_len -= status->byte_len;
		status->rx_buf  += status->byte_len;
	}

	return 0;
}

static int a3700_is_wfifo_full(struct a3700_spi *a3700_spi)
{
	u32 val;

	val = spireg_read(a3700_spi, A3700_SPI_IF_CTRL_REG);
	return (val & A3700_SPI_WFIFO_FULL);
}

static int a3700_spi_fifo_write(struct a3700_spi *a3700_spi)
{
	struct a3700_spi_status *status = &a3700_spi->status;
	u32 val;

	if (status->buf_len % status->byte_len)
		return -EINVAL;

	while (!a3700_is_wfifo_full(a3700_spi) && status->buf_len) {
		/* Write bytes to data out register */
		val = 0;
		if (status->byte_len == 4) {
			val |= status->tx_buf[0] << 24;
			val |= status->tx_buf[1] << 16;
			val |= status->tx_buf[2] << 8;
			val |= status->tx_buf[3];
		} else
			val = status->tx_buf[0];

		spireg_write(a3700_spi, A3700_SPI_DATA_OUT_REG, val);

		status->buf_len -= status->byte_len;
		status->tx_buf  += status->byte_len;
	}

	return 0;
}

static void a3700_spi_transfer_abort_non_legacy(struct a3700_spi *a3700_spi)
{
	int timeout = A3700_SPI_TIMEOUT;
	u32 val;

	val = spireg_read(a3700_spi, A3700_SPI_IF_CFG_REG);
	val |= A3700_SPI_XFER_STOP;
	spireg_write(a3700_spi, A3700_SPI_IF_CFG_REG, val);

	while (--timeout) {
		val = spireg_read(a3700_spi, A3700_SPI_IF_CFG_REG);
		if (!(val & A3700_SPI_XFER_START))
			break;
		udelay(1);
	}

	a3700_spi_fifo_flush(a3700_spi);

	val &= ~A3700_SPI_XFER_STOP;
	spireg_write(a3700_spi, A3700_SPI_IF_CFG_REG, val);
}

static int a3700_spi_do_transfer_non_legacy(struct spi_device *spi)
{
	struct a3700_spi *a3700_spi;
	int ret = 0;

	a3700_spi = spi_master_get_devdata(spi->master);

	while (a3700_spi->status.buf_len) {
		if (a3700_spi->status.tx_buf) {
			/* Wait wfifo ready */
			if (!a3700_spi_transfer_wait(spi,
						     A3700_SPI_WFIFO_RDY)) {
				dev_err(&spi->dev,
					"wait wfifo ready timed out\n");
				ret = -ETIMEDOUT;
				goto error;
			}
			/* Fill up the wfifo */
			ret = a3700_spi_fifo_write(a3700_spi);
			if (ret)
				goto error;
		} else if (a3700_spi->status.rx_buf) {
			/* Wait rfifo ready */
			if (!a3700_spi_transfer_wait(spi,
						     A3700_SPI_RFIFO_RDY)) {
				dev_err(&spi->dev,
					"wait rfifo ready timed out\n");
				ret = -ETIMEDOUT;
				goto error;
			}
			/* Drain out the rfifo */
			ret = a3700_spi_fifo_read(a3700_spi);
			if (ret)
				goto error;
		}
	}

out:
	return ret;
error:
	a3700_spi_transfer_abort_non_legacy(a3700_spi);
	a3700_spi_deactivate_cs(a3700_spi, spi->chip_select);
	goto out;
}

static int a3700_spi_transfer_finish_non_legacy(struct spi_device *spi,
	struct spi_transfer *xfer)
{
	struct a3700_spi *a3700_spi;
	int timeout = A3700_SPI_TIMEOUT;
	u32 val;

	a3700_spi = spi_master_get_devdata(spi->master);

	/*
	 * Stop a write transfer in fifo mode:
	 *	- wait all the bytes in wfifo to be shifted out
	 *	 - set XFER_STOP bit
	 *	- wait XFER_START bit clear
	 *	- clear XFER_STOP bit
	 * Stop a read transfer in fifo mode:
	 *	- the hardware is to reset the XFER_START bit
	 *	   after the number of bytes indicated in DIN_CNT
	 *	   register
	 *	- just wait XFER_START bit clear
	 */
	if (a3700_spi->status.tx_buf) {
		if (!a3700_spi_transfer_wait(spi, A3700_SPI_WFIFO_EMPTY)) {
			dev_err(&spi->dev, "wait write fifo empty timed out\n");
			return -ETIMEDOUT;
		}

		val = spireg_read(a3700_spi, A3700_SPI_IF_CFG_REG);
		val |= A3700_SPI_XFER_STOP;
		spireg_write(a3700_spi, A3700_SPI_IF_CFG_REG, val);
	}

	while (--timeout) {
		val = spireg_read(a3700_spi, A3700_SPI_IF_CFG_REG);
		if (!(val & A3700_SPI_XFER_START))
			break;
		udelay(1);
	}

	if (timeout == 0) {
		dev_err(&spi->dev, "wait transfer start clear timed out\n");
		return -ETIMEDOUT;
	}

	val &= ~A3700_SPI_XFER_STOP;
	spireg_write(a3700_spi, A3700_SPI_IF_CFG_REG, val);

	if (a3700_spi->status.last_xfer || xfer->cs_change)
		a3700_spi_deactivate_cs(a3700_spi, spi->chip_select);

	return 0;
}

static int a3700_spi_transfer_one_message(struct spi_master *master,
	struct spi_message *mesg)
{
	struct spi_device *spi = mesg->spi;
	struct a3700_spi *a3700_spi = spi_master_get_devdata(master);
	struct spi_transfer *xfer = NULL;
	struct a3700_spi_status *xfer_stat = &a3700_spi->status;
	int ret = 0;

	list_for_each_entry(xfer, &mesg->transfers, transfer_list) {

		dev_dbg(&spi->dev, "<xfer> rx_buf %p, tx_buf %p, len %d, BPW %d\n",
			xfer->rx_buf, xfer->tx_buf, xfer->len,
			xfer->bits_per_word);

		ret = a3700_spi_transfer_setup(spi, xfer);
		if (ret)
			goto out;

		ret = a3700_spi->spi_pre_xfer(spi, xfer);
		if (ret)
			goto out;

		ret = a3700_spi->spi_do_xfer(spi);
		if (ret)
			goto out;

		xfer_stat->last_xfer = list_is_last(&xfer->transfer_list,
			&mesg->transfers);
		ret = a3700_spi->spi_post_xfer(spi, xfer);
		if (ret)
			goto out;

		mesg->actual_length += (xfer->len - xfer_stat->buf_len);
	}

out:
	mesg->status = ret;
	spi_finalize_current_message(master);

	return 0;
}


static const struct a3700_spi_initdata armada_3700_spi_initdata = {
	.cs_num             = 4,
	.mode               = SPI_CPHA | SPI_CPOL,
	.bits_per_word_mask = SPI_BPW_MASK(8) | SPI_BPW_MASK(32),
	.instr_cnt          = 1,
	.addr_cnt           = 3,
};

static const struct of_device_id a3700_spi_of_match_table[] = {
	{
		.compatible = "marvell,armada3700-spi",
		.data       = &armada_3700_spi_initdata,
	},
	{
	}
};
MODULE_DEVICE_TABLE(of, a3700_spi_of_match_table);

static int a3700_spi_probe(struct platform_device *pdev)
{
	const struct of_device_id *of_id;
	const struct a3700_spi_initdata *initdata;
	struct device_node *of_node;
	struct resource *res;
	struct spi_master *master;
	struct a3700_spi *spi;
	u32 cell_index;
	int ret = 0;

	of_node = pdev->dev.of_node;

	of_id = of_match_device(a3700_spi_of_match_table, &pdev->dev);
	if (!of_id) {
		dev_err(&pdev->dev, "no device found\n");
		ret = -ENODEV;
		goto out;
	}
	initdata = of_id->data;

	master = spi_alloc_master(&pdev->dev, sizeof(*spi));
	if (master == NULL) {
		dev_err(&pdev->dev, "master allocation failed\n");
		ret = -ENOMEM;
		goto out;
	}

	if (pdev->id != -1)
		master->bus_num = pdev->id;
	if (of_node) {
		if (!of_property_read_u32(of_node, "cell-index", &cell_index))
			master->bus_num = cell_index;
	}

	master->dev.of_node          = of_node;
	master->mode_bits            = initdata->mode;
	master->num_chipselect       = initdata->cs_num;
	master->bits_per_word_mask   = initdata->bits_per_word_mask;
	master->transfer_one_message = a3700_spi_transfer_one_message;

	platform_set_drvdata(pdev, master);

	spi = spi_master_get_devdata(master);
	memset(spi, 0, sizeof(struct a3700_spi));

	spi->master = master;

	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	spi->base = devm_ioremap_resource(&pdev->dev, res);
	if (IS_ERR(spi->base)) {
		ret = PTR_ERR(spi->base);
		goto error;
	}

	spi->irq = irq_of_parse_and_map(of_node, 0);
	if (spi->irq == 0) {
		dev_err(&pdev->dev, "enable spi poll mode\n");
		spi->flags |= XFER_POLL;
		spi->spi_wait_xfer = a3700_spi_wait_ctl_bit_set;
	} else {
		spi->spi_wait_xfer = a3700_spi_wait_completion;
		init_completion(&spi->status.done);
	}

	of_property_read_u32(of_node, "max-frequency", &spi->max_clk_freq);
	if (!spi->max_clk_freq) {
		dev_err(&pdev->dev, "could not find spi-max-frequency\n");
		goto error_clk;
	}
	of_property_read_u32(of_node, "clock-frequency", &spi->input_clk_freq);
	if (!spi->input_clk_freq) {
		dev_err(&pdev->dev, "could not find clock-frequency\n");
		goto error_clk;
	}
	/* TO-DO: there is no gating clock driver so far, and SPI clock
	 * configuration has been done in boot rom. So we are good for now.
	 * but when gating clock is ready, need to enable the clock here.
	 */

	if (of_find_property(of_node, "fifo-mode", NULL)) {
		dev_err(&pdev->dev, "fifo mode\n");
		spi->flags |= HAS_FIFO;
		spi->spi_pre_xfer  = a3700_spi_transfer_start_non_legacy;
		spi->spi_do_xfer   = a3700_spi_do_transfer_non_legacy;
		spi->spi_post_xfer = a3700_spi_transfer_finish_non_legacy;

		spi->max_cnt.instr_cnt = initdata->instr_cnt;
		spi->max_cnt.addr_cnt  = initdata->addr_cnt;
		spi->max_cnt.hdr_cnt   = initdata->instr_cnt
				+ initdata->addr_cnt;
	} else {
		dev_err(&pdev->dev, "legacy mode\n");
		spi->spi_pre_xfer  = a3700_spi_transfer_start_legacy;
		spi->spi_do_xfer   = a3700_spi_do_transfer_legacy;
		spi->spi_post_xfer = a3700_spi_transfer_finish_legacy;

		spi->max_cnt.instr_cnt = 0;
		spi->max_cnt.addr_cnt  = 0;
		spi->max_cnt.hdr_cnt   = 0;
	}

	spi->pin_mode = A3700_SPI_SGL_PIN; /* fix-me: add device tree support */

	ret = a3700_spi_init(spi);
	if (ret)
		goto error_clk;

	if (!(spi->flags & XFER_POLL)) {
		ret = devm_request_irq(&pdev->dev, spi->irq,
			a3700_spi_interrupt, 0, dev_name(&pdev->dev), master);
		if (ret) {
			dev_err(&pdev->dev, "could not request IRQ: %d\n", ret);
			goto error_clk;
		}
	}

	ret = spi_register_master(master);
	if (ret)
		goto error_clk;

out:
	return ret;

error_clk:
	/* TO-DO: when gating clock is ready, need to disable the clock. */

error:
	spi_master_put(master);
	goto out;
}

static int a3700_spi_remove(struct platform_device *pdev)
{
	struct spi_master *master = platform_get_drvdata(pdev);

	/* TO-DO: when gating clock is ready, need to disable the clock. */

	spi_unregister_master(master);

	return 0;
}

MODULE_ALIAS("platform:" DRIVER_NAME);


static struct platform_driver a3700_spi_driver = {
	.driver = {
		.name	= DRIVER_NAME,
		.owner	= THIS_MODULE,
		.of_match_table = of_match_ptr(a3700_spi_of_match_table),
	},
	.probe		= a3700_spi_probe,
	.remove		= a3700_spi_remove,
};

module_platform_driver(a3700_spi_driver);

MODULE_DESCRIPTION("Armada-3700 SPI driver");
MODULE_AUTHOR("Wilson Ding <dingwei@marvell.com>");
MODULE_LICENSE("GPL");
