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

#include "silabs_if.h"

static void (*isi_handler)(unsigned long);

void silabs_if_enable_irq(u32 device)
{
	mv_phone_intr_enable(device);
}
EXPORT_SYMBOL(silabs_if_enable_irq);

void silabs_if_disable_irq(u32 device)
{
	mv_phone_intr_disable(device);
}
EXPORT_SYMBOL(silabs_if_disable_irq);

void silabs_if_handler_register(void *func)
{
	isi_handler = func;
}
EXPORT_SYMBOL(silabs_if_handler_register);

void silabs_if_handler_unregister(void)
{
	isi_handler = NULL;
}
EXPORT_SYMBOL(silabs_if_handler_unregister);

void silabs_if_isi_interrupt(void)
{
	if (isi_handler != NULL)
		isi_handler(0);
}
EXPORT_SYMBOL(silabs_if_isi_interrupt);

void silabs_if_spi_init(u32 line)
{
}
EXPORT_SYMBOL(silabs_if_spi_init);

void silabs_if_spi_read(u32 line, u8 *cmd, u8 cmd_size,
			u8 *data, u8 data_size)
{
	uint32_t slic_device = mv_phone_get_slic_board_type();

	if (slic_device == SLIC_SILABS_ID)
		mv_phone_spi_read(line, cmd, cmd_size, data,
				  data_size, SPI_TYPE_SLIC_ISI);
	else
		mv_phone_spi_read(line, cmd, cmd_size, data,
				  data_size, SPI_TYPE_SLIC_ZARLINK_SILABS);
}
EXPORT_SYMBOL(silabs_if_spi_read);

void silabs_if_spi_write(u32 line, u8 *cmd, u8 cmd_size,
			 u8 *data, u8 data_size)
{
	uint32_t slic_device = mv_phone_get_slic_board_type();

	if (slic_device == SLIC_SILABS_ID)
		mv_phone_spi_write(line, cmd, cmd_size, data,
				   data_size, SPI_TYPE_SLIC_ISI);
	else
		mv_phone_spi_write(line, cmd, cmd_size, data,
				   data_size, SPI_TYPE_SLIC_ZARLINK_SILABS);
}
EXPORT_SYMBOL(silabs_if_spi_write);
