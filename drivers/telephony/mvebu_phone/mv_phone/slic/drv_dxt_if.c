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

******************************************************************************/

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/interrupt.h>
#include <plat/drv_dxt_if.h>
#include "spi/mvSpi.h"
#include "voiceband/mvSysTdmSpi.h"

static int drv_dxt_spi_cs;
static int drv_dxt_irq;
static int drv_dxt_irq_dev;
static irq_handler_t drv_dxt_irq_handler;

void
drv_dxt_if_signal_interrupt(void)
{
	if (drv_dxt_irq_handler == NULL)
		return;

	drv_dxt_irq_handler(drv_dxt_irq, drv_dxt_irq_dev);
}

void
drv_dxt_if_enable_irq(unsigned int irq)
{
	/* We have only one TDM channel */
	mvSysTdmIntEnable(0);
}
EXPORT_SYMBOL(drv_dxt_if_enable_irq);

void
drv_dxt_if_disable_irq(unsigned int irq)
{
	/* We have only one TDM channel */
	mvSysTdmIntDisable(0);
}
EXPORT_SYMBOL(drv_dxt_if_disable_irq);

int
drv_dxt_if_request_irq(unsigned int irq, irq_handler_t handler,
			unsigned long flags, const char *name, void *dev)
{
	drv_dxt_irq = irq;
	drv_dxt_irq_dev = dev;
	drv_dxt_irq_handler = handler;

	return 0;
}
EXPORT_SYMBOL(drv_dxt_if_request_irq);

void
drv_dxt_if_free_irq(unsigned int irq, void *dev)
{
	drv_dxt_irq_handler = NULL;
}
EXPORT_SYMBOL(drv_dxt_if_free_irq);

void
drv_dxt_if_spi_cs_set(unsigned int dev_no, unsigned int hi_lo)
{
	if (hi_lo == 0)
		drv_dxt_spi_cs = dev_no;
	else
		drv_dxt_spi_cs = -1;
}
EXPORT_SYMBOL(drv_dxt_if_spi_cs_set);

int
drv_dxt_if_spi_ll_read_write(unsigned char *tx_data, unsigned int tx_size,
				 unsigned char *rx_data, unsigned int rx_size)
{
	uint16_t *ptr;
	int i;

	if ((tx_size & 1) || (rx_size & 1)) {
		pr_err("drv_dxt_if: SPI transfer is not word aligned!\n");
		return 0;
	}

	ptr = (uint16_t *)tx_data;
	for (i = 0; i < tx_size / 2; i++, ptr++)
		*ptr = htons(*ptr);

	if (rx_data != NULL && rx_size != 0) {
		mvSysTdmSpiRead(drv_dxt_spi_cs, tx_data, tx_size,
							rx_data, rx_size, SPI_TYPE_SLIC_LANTIQ);
	} else if (tx_data != NULL && tx_size > 2) {
		mvSysTdmSpiWrite(drv_dxt_spi_cs, tx_data, 2,
						tx_data + 2, tx_size - 2, SPI_TYPE_SLIC_LANTIQ);
	} else {
		pr_err("drv_dxt_if: Unsupported SPI access mode!\n");
	}

	ptr = (uint16_t *)rx_data;
	for (i = 0; i < rx_size / 2; i++, ptr++)
		*ptr = htons(*ptr);

	return 0;
}
EXPORT_SYMBOL(drv_dxt_if_spi_ll_read_write);
