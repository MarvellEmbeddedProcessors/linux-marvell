/*
 * Marvell AP806 SoC info definitions.
 *
 * Copyright (C) 2008 Marvell Semiconductor
 *
 * This file is licensed under the terms of the GNU General Public
 * License version 2.  This program is licensed "as is" without any
 * warranty of any kind, whether express or implied.
 */

#ifndef MV_SOC_INFO_H
#define MV_SOC_INFO_H

#define GWD_IIDR2_REV_ID_OFFSET	12
#define GWD_IIDR2_REV_ID_MASK	0xF
#define APN806_REV_ID_A0	0
#define APN806_REV_ID_A1	1

int mv_soc_info_get_revision(void);


#endif /* MV_SOC_INFO_H */
