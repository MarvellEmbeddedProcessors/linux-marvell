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

#ifndef __mvHwIf_h__
#define __mvHwIf_h__

#include <linux/kernel.h>
#include <linux/io.h>

struct pp3_unit_info {
	u32 base_addr; /* unit base address = silicon addr + unit offset */
	u32 ins_offs;  /* unit instance offset - for multiple units */
};

/*
	access_addr - absolute address: Silicon base + unit base + table base + entry offset
	words_num   - number of words (word = 32 bits) to read
	data_ptr    - pointer to an array containing the read data
 */
static inline void mv_pp3_hw_read(u32 access_addr, int words_num, u32 *data_ptr)
{
	int i;

	for (i = 0; i < words_num; i++)
		data_ptr[i] = readl(access_addr + 4*i);
}

/*
	access_addr - absolute address: Silicon base + unit base + table base + entry offset
	words_num   - number of words (word = 32 bits) to write
	data_ptr    - pointer to an array containing the write data
 */
static inline void mv_pp3_hw_write(u32 access_addr, int words_num, u32 *data_ptr)
{
	int i;

	for (i = 0; i < words_num; i++)
		writel(data_ptr[i], access_addr + 4*i);
}

/*
	access_addr - absolute address: Silicon base + unit base + register offset
	return register value
 */
static inline u32 mv_pp3_hw_reg_read(u32 access_addr)
{
	return readl(access_addr);
}

/*
	access_addr - absolute address: Silicon base + unit base + register offset
	write data to register
 */
static inline void mv_pp3_hw_reg_write(u32 access_addr, u32 data)
{
#ifdef PP3_DEBUG
	pr_info("\nwrite reg 0x%x, data 0x%x", access_addr, data);
#endif
	writel(data, access_addr);
}


u32 mv_hw_silicon_base_addr_get(void);
u32 mv_cmn_sram_base_addr_get(void);
u32 mv_gop_base_addr_get(void);

#endif /* __mvHwIf_h__ */
