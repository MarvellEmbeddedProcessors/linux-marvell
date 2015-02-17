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

#include <linux/init.h>
#include <linux/slab.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/spi/spi.h>
#include "voiceband/tdm/mvTdm.h"
#include "voiceband/mvSysTdmSpi.h"

#define DRV_NAME "mvebu_phone_spi"

#undef MVEBU_PHONE_SPI_DEBUG

struct spi_device *slic_spi;

/* Telephony register read via SPI interface. */
void mvSysTdmSpiRead(u16 line_id, u8 *cmd_buff, u8 cmd_size,
		     u8 *data_buff, u8 data_size, u32 spi_type)
{
	int err;

#ifdef MVEBU_PHONE_SPI_DEBUG
	pr_info("%s():line(%d) Spi ID=%d line_id=%d Spi CS=%d Spi type=%d\n",
		__func__, __LINE__, slic_spi->master->bus_num, line_id,
		slic_spi->chip_select , spi_type);
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
void mvSysTdmSpiWrite(u16 line_id, u8 *cmd_buff, u8 cmd_size,
		      u8 *data_buff, u8 data_size, u32 spi_type)
{
	int err;
	struct spi_message m;
	struct spi_transfer t[2] = { { .tx_buf = (const void *)cmd_buff,
				       .len = cmd_size, },
				     { .tx_buf = (const void *)data_buff,
				       .len = data_size, }, };

#ifdef MVEBU_PHONE_SPI_DEBUG
	pr_info("%s():line(%d) Spi ID=%d line_id=%d Spi CS=%d Spi type=%d\n",
		__func__, __LINE__, slic_spi->master->bus_num, line_id,
		slic_spi->chip_select , spi_type);
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

	err = spi_setup(spi);
	if (err) {
		dev_err(&spi->dev, "spi setup failed\n");
		return err;
	}

	slic_spi = spi;

	dev_info(&spi->dev, "registered slic spi device at bus #%d, CS #%d",
		 spi->master->bus_num, spi->chip_select);

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
