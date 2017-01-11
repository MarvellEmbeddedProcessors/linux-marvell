/*******************************************************************************
 * Copyright (C) 2016 Marvell International Ltd.
 *
 * This software file (the "File") is owned and distributed by Marvell
 * International Ltd. and/or its affiliates ("Marvell") under the following
 * alternative licensing terms.  Once you have made an election to distribute the
 * File under one of the following license alternatives, please (i) delete this
 * introductory statement regarding license alternatives, (ii) delete the three
 * license alternatives that you have not elected to use and (iii) preserve the
 * Marvell copyright notice above.
 *
 * ********************************************************************************
 * Marvell Commercial License Option
 *
 * If you received this File from Marvell and you have entered into a commercial
 * license agreement (a "Commercial License") with Marvell, the File is licensed
 * to you under the terms of the applicable Commercial License.
 *
 * ********************************************************************************
 * Marvell GPL License Option
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the Free
 * Software Foundation, either version 2 of the License, or any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * ********************************************************************************
 * Marvell GNU General Public License FreeRTOS Exception
 *
 * If you received this File from Marvell, you may opt to use, redistribute and/or
 * modify this File in accordance with the terms and conditions of the Lesser
 * General Public License Version 2.1 plus the following FreeRTOS exception.
 * An independent module is a module which is not derived from or based on
 * FreeRTOS.
 * Clause 1:
 * Linking FreeRTOS statically or dynamically with other modules is making a
 * combined work based on FreeRTOS. Thus, the terms and conditions of the GNU
 * General Public License cover the whole combination.
 * As a special exception, the copyright holder of FreeRTOS gives you permission
 * to link FreeRTOS with independent modules that communicate with FreeRTOS solely
 * through the FreeRTOS API interface, regardless of the license terms of these
 * independent modules, and to copy and distribute the resulting combined work
 * under terms of your choice, provided that:
 * 1. Every copy of the combined work is accompanied by a written statement that
 * details to the recipient the version of FreeRTOS used and an offer by yourself
 * to provide the FreeRTOS source code (including any modifications you may have
 * made) should the recipient request it.
 * 2. The combined work is not itself an RTOS, scheduler, kernel or related
 * product.
 * 3. The independent modules add significant and primary functionality to
 * FreeRTOS and do not merely extend the existing functionality already present in
 * FreeRTOS.
 * Clause 2:
 * FreeRTOS may not be used for any competitive or comparative purpose, including
 * the publication of any form of run time or compile time metric, without the
 * express permission of Real Time Engineers Ltd. (this is the norm within the
 * industry and is intended to ensure information accuracy).
 *
 * ********************************************************************************
 * Marvell BSD License Option
 *
 * If you received this File from Marvell, you may opt to use, redistribute and/or
 * modify this File under the following licensing terms.
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted provided that the following conditions are met:
 *
 *	* Redistributions of source code must retain the above copyright notice,
 *	  this list of conditions and the following disclaimer.
 *
 *	* Redistributions in binary form must reproduce the above copyright
 *	  notice, this list of conditions and the following disclaimer in the
 *	  documentation and/or other materials provided with the distribution.
 *
 *	* Neither the name of Marvell nor the names of its contributors may be
 *	  used to endorse or promote products derived from this software without
 *	  specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
 * ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <linux/spi/spi.h>
#include "mv_phone.h"

#define DRV_NAME "mvebu_phone_spi"

#undef MVEBU_PHONE_SPI_DEBUG

#define MAX_SLIC_DEVICES 32

/* Global array of pointers to SLIC SPI devices */
struct spi_device *slic_devs[MAX_SLIC_DEVICES];

/* Telephony register read via SPI interface. */
void mv_phone_spi_read(u16 dev_id, u8 *cmd_buff, u8 cmd_size,
		       u8 *data_buff, u8 data_size, u32 spi_type)
{
	struct spi_device *slic_spi = slic_devs[dev_id];
	int err;

#ifdef MVEBU_PHONE_SPI_DEBUG
	pr_info("%s():line(%d) Spi ID=%d dev_id=%d Spi CS=%d Spi type=%d\n",
		__func__, __LINE__, slic_spi->master->bus_num, dev_id,
		slic_spi->chip_select, spi_type);
#endif

	err = spi_write_then_read(slic_spi, (const void *)cmd_buff, cmd_size,
				  (void *)data_buff, data_size);
	if (err)
		dev_err(&slic_spi->dev, "SPI read failed\n");

#ifdef MVEBU_PHONE_SPI_DEBUG
	pr_info("CMD = 0x%x, cmd_size = 0x%x, DATA = 0x%x, data_size = 0x%x\n",
		*cmd_buff, cmd_size, *data_buff, data_size);
#endif
}

/* Telephony register write via SPI interface. */
void mv_phone_spi_write(u16 dev_id, u8 *cmd_buff, u8 cmd_size,
			u8 *data_buff, u8 data_size, u32 spi_type)
{
	int err;
	struct spi_message m;
	struct spi_device *slic_spi = slic_devs[dev_id];
	struct spi_transfer t[2] = { { .tx_buf = (const void *)cmd_buff,
				       .len = cmd_size, },
				     { .tx_buf = (const void *)data_buff,
				       .len = data_size, }, };

#ifdef MVEBU_PHONE_SPI_DEBUG
	pr_info("%s():line(%d) Spi ID=%d dev_id=%d Spi CS=%d Spi type=%d\n",
		__func__, __LINE__, slic_spi->master->bus_num, dev_id,
		slic_spi->chip_select, spi_type);
	pr_info("CMD = 0x%x, cmd_size = 0x%x, DATA = 0x%x, data_size = 0x%x\n",
		*cmd_buff, cmd_size, *data_buff, data_size);
#endif

	spi_message_init(&m);
	spi_message_add_tail(&t[0], &m);
	spi_message_add_tail(&t[1], &m);

	err = spi_sync(slic_spi, &m);
	if (err)
		dev_err(&slic_spi->dev, "SPI write failed\n");
}

static int mvebu_phone_spi_probe(struct spi_device *spi)
{
	int err;
	u32 dev_id;

	/* Obtain SLIC ID */
	err = of_property_read_u32(spi->dev.of_node, "slic-id", &dev_id);
	if (err == -EINVAL) {
		/* Assign '0' ID in case the 'slic-id' property is not used */
		dev_id = 0;
	} else if (err) {
		dev_err(&spi->dev, "unable to get SLIC ID\n");
		return err;
	} else if (dev_id >= MAX_SLIC_DEVICES) {
		dev_err(&spi->dev, "SLIC ID (%d) exceeds maximum (%d)\n",
			dev_id, MAX_SLIC_DEVICES - 1);
		return -EINVAL;
	}

	/* Check if this ID wasn't used by previous devices */
	if (slic_devs[dev_id]) {
		dev_err(&spi->dev, "overlapping ID (%d) at bus #%d, CS #%d\n",
			dev_id, spi->master->bus_num, spi->chip_select);
		return -EINVAL;
	}

	slic_devs[dev_id] = spi;

	err = spi_setup(spi);
	if (err) {
		dev_err(&spi->dev, "spi setup failed\n");
		return err;
	}

	dev_info(&spi->dev, "registered slic spi device %d at bus #%d, CS #%d",
		 dev_id, spi->master->bus_num, spi->chip_select);

	return 0;
}

static int mvebu_phone_spi_remove(struct spi_device *spi)
{
	return 0;
}

static const struct spi_device_id mvebu_phone_spi_ids[] = {
	{ "mv_slic", 0 },
	{ },
};
MODULE_DEVICE_TABLE(spi, mvebu_phone_spi_ids);

static struct spi_driver mvebu_phone_spi_driver = {
	.driver = {
		.name	= DRV_NAME,
		.owner	= THIS_MODULE,
	},
	.id_table = mvebu_phone_spi_ids,
	.probe	= mvebu_phone_spi_probe,
	.remove	= mvebu_phone_spi_remove,
};

module_spi_driver(mvebu_phone_spi_driver);

MODULE_DESCRIPTION("Marvell Telephony SPI Driver");
MODULE_AUTHOR("Marcin Wojtas <mw@semihalf.com>");
