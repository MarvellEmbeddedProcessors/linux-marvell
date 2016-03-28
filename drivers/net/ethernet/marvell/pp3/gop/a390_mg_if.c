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
*******************************************************************************/
#include "common/mv_sw_if.h"
#include "common/mv_hw_if.h"
#include "a390_mg_if.h"
#include "mv_gop_if.h"

/* Enable address completion on 8 regions over register 0x0140 on INIT.
 * Run-time Completion registers (0x0120, .. 0x013c) are used over array
 * a390_int.addr_completion_regs[]
 */
#define MV_PP3_ADDR_COMP_CNTRL_REG_OFFS	0x0140
#define MV_PP3_ADDR_COMP_USED_REGIONS	0xff00

 /* bits of address passes as is throw PCI window */
#define NOT_ADDRESS_COMPLETION_BITS_NUM 19
/* bits of address extracted from address completion registers */
#define ADDRESS_COMPLETION_BITS_MASK    (0xFFFFFFFF << NOT_ADDRESS_COMPLETION_BITS_NUM)

struct a390_internals {
	u32 fixed_addr_filter;
	u32 dedicated_completion_msbits;
	u32 addr_completion_regs[8];
	u32 mg_base;
};

static struct a390_internals a390_int = {
	.addr_completion_regs = {0x0120, 0x0124, 0x0128, 0x012c, 0x0130, 0x0134, 0x0138, 0x013c},
};


/* return new calculated access address without silicon base */
/*        that added in real read/write function             */
u32 a390_addr_completion_cfg(u32 reg_addr)
{
	int cpu = smp_processor_id();
	int comp_indx;
	u32 address;
	u32 addr_region = (reg_addr >> NOT_ADDRESS_COMPLETION_BITS_NUM);

	if (addr_region == 0) {
		/* address comletion bits are 0, region0 used */
		return reg_addr;
	}

	/* choose region for cpu (application or ISR) */
	/* address completion region assignment: */
	/* cpu 0 all applications  - REGION 1    */
	/* cpu 0 all SW interrupts - REGION 2    */
	/* cpu 0 HW interrupt      - REGION 3    */
	/* cpu 1 all applications  - REGION 4    */
	/* cpu 1 all SW interrupts - REGION 5    */
	/* cpu 1 HW interrupt      - REGION 6    */
	/* Dedicated with FIX addr - REGION 7    */

	/* On fixed_addr_filter==0 the fixed is not set and to be ignored.
	 * Use "common" in that case
	 */
	if (a390_int.fixed_addr_filter && (addr_region == a390_int.fixed_addr_filter)) {
		address = a390_int.dedicated_completion_msbits  | (reg_addr & (~ADDRESS_COMPLETION_BITS_MASK));
		return address;
	}

	if (cpu == 0) {
		if (in_irq())
			comp_indx = 3;
		else if (in_softirq())
			comp_indx = 2;
		else
			comp_indx = 1;
	} else {
		if (in_irq())
			comp_indx = 6;
		else if (in_softirq())
			comp_indx = 5;
		else
			comp_indx = 4;
	}

	/* configure completion addr_region into HW */
	/* adders base added at the lower level cpuWrite/cpuRead */
	address = a390_int.mg_base + a390_int.addr_completion_regs[comp_indx];
	writel(addr_region, (void *)address);

	/* adders base added at the lower level cpuWrite/cpuRead */
	address = ((comp_indx << NOT_ADDRESS_COMPLETION_BITS_NUM)
		| (reg_addr & (~ADDRESS_COMPLETION_BITS_MASK)));

	return address;
}

/* --- Init-Configurations --------------------------- */
void a390_addr_completion_init(void *mg_base)
{
	a390_int.mg_base = (u32)mg_base;
	/* enable address completion on 7 regions + region 0 */
	mv_pp3_hw_reg_write(mg_base + MV_PP3_ADDR_COMP_CNTRL_REG_OFFS,
		MV_PP3_ADDR_COMP_USED_REGIONS);
}

void a390_addr_completion_fixed_init(u32 dedicated_region_no, u32 reg_base)
{
	/* Static(never changed) configuration for dedicated MG_REGION */
	u32 address;
	/* If the mg_base, reg_base are null - no dedicated configured.
	 * Currently only 1 dedicated region supported and so
	 * the procedure should be called once only
	 */
	if (!a390_int.mg_base)
		return;
	if (!dedicated_region_no)
		return;
	if (!reg_base)
		return;
	if (a390_int.dedicated_completion_msbits)
		return; /* already configured */
	a390_int.fixed_addr_filter = reg_base >> NOT_ADDRESS_COMPLETION_BITS_NUM;
	address = a390_int.mg_base + a390_int.addr_completion_regs[dedicated_region_no];
	a390_int.dedicated_completion_msbits = dedicated_region_no << NOT_ADDRESS_COMPLETION_BITS_NUM;
	/* Set fixed address completion into MG hw (for MV_PP3_DEDICATED_MG_REGION)
	 * dedicated for the given "reg_base"
	 */
	writel(a390_int.fixed_addr_filter, (void *)address);
}
