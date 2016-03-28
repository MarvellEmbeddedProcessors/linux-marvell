/*******************************************************************************
Copyright (C) Marvell International Ltd. and its affiliates

This software file (the "File") is owned and distributed by Marvell
International Ltd. and/or its affiliates ("Marvell") under the following
alternative licensing terms.  Once you have made an election to distribute the
File under one of the following license alternatives, please (i) delete this
introductory statement regarding license alternatives, (ii) delete the two
license alternatives that you have not elected to use and (iii) preserve the
Marvell copyright notice above.

*******************************************************************************/
#ifndef __a390_mg_if_h__
#define __a390_mg_if_h__

#define MV_PP3_DEDICATED_MG_REGION	7

void a390_addr_completion_init(void *mg_base);
void a390_addr_completion_fixed_init(u32 dedicated_region_no, u32 reg_base);
u32 a390_addr_completion_cfg(u32 reg_addr);

#endif /* __a390_mg_if_h__ */
