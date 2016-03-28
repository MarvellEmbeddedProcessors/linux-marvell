/*
* ***************************************************************************
* Copyright (C) 2015 Marvell International Ltd.
* ***************************************************************************
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
* ***************************************************************************
*/

#ifndef __a390_gic_odmi_if_h__
#define __a390_gic_odmi_if_h__

#include "common/mv_hw_if.h"

#define MV_A390_GIC_INT_GROUPS_NUM	(8)	/* number of interrupt groups per frame in ODMI */
#define MV_A390_GIC_INT_SINGLE_GR_NUM	(6)	/* number of single interrupt groups per frame in ODMI */

#define MV_A390_GIC_REGS_OFFS				(0x19000)

#define MV_A390_GIC_INTERRUPT_REG(f)			(0x40 + (0x2000)*(f) + MV_A390_GIC_REGS_OFFS)

#define MV_A390_GIC_ODMI_EPR0_REG(f, g)			(0x100 + 0x2000*(f) + 0x10*(g) + MV_A390_GIC_REGS_OFFS)

#define MV_A390_GIC_RXQ_INT_SET(val, q)			(val |= (1 << ((q) * 2)))
#define MV_A390_GIC_RXQ_INT_GET(val, q)			(((val) >> ((q) * 2)) & 1)

/*-------------------------------------------------------------------------*/


/* read GIC ODMI Event Pending Register in GICP unit */
static inline u32 a390_gic_odmi_epr_read(int frame, int event_group)
{
	void __iomem *base = mv_pp3_nss_regs_vaddr_get();

	return mv_pp3_hw_reg_read(base + MV_A390_GIC_ODMI_EPR0_REG(frame, event_group));
}

/* write GIC ODMI Event Pending Register in GICP unit */
static inline void a390_gic_odmi_epr_write(int frame, int event_group, int data)
{
	void __iomem *base = mv_pp3_nss_regs_vaddr_get();

	return mv_pp3_hw_reg_write(base + MV_A390_GIC_ODMI_EPR0_REG(frame, event_group), data);
}

#endif /* __a390_gic_odmi_if_h__*/
