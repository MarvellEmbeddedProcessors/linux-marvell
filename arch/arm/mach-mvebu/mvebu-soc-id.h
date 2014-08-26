/*
 * Marvell EBU SoC ID and revision definitions.
 *
 * Copyright (C) 2014 Marvell Semiconductor
 *
 * This file is licensed under the terms of the GNU General Public
 * License version 2.  This program is licensed "as is" without any
 * warranty of any kind, whether express or implied.
 */

#ifndef __LINUX_MVEBU_SOC_ID_H
#define __LINUX_MVEBU_SOC_ID_H

/* Armada 370 ID */
#define MV6710_DEV_ID		0x6710
#define MV6707_DEV_ID		0x6707

/* Armada 370 Revision */
#define MV6710_A1_REV		0x1

/* Armada XP ID */
#define MV78230_DEV_ID		0x7823
#define MV78260_DEV_ID		0x7826
#define MV78460_DEV_ID		0x7846

/* Armada XP Revision */
#define MV78XX0_A0_REV		0x1
#define MV78XX0_B0_REV		0x2

/* Armada A375 ID */
#define MV88F6720_DEV_ID	0x6720

/* Armada A375 Revision */
#define MV88F6720_A0_REV	0x1

/* Armada A38x ID */
#define MV88F6810_DEV_ID	0x6810
#define MV88F6811_DEV_ID	0x6811
#define MV88F6820_DEV_ID	0x6820
#define MV88F6828_DEV_ID	0x6828

/* Armada A38x Revision */
#define MV88F68xx_Z1_REV	0x0
#define MV88F68xx_A0_REV	0x4

/* Armada KW2 ID */
#define MV88F6510_DEV_ID	0x6510
#define MV88F6530_DEV_ID	0x6530
#define MV88F6560_DEV_ID	0x6560
#define MV88F6601_DEV_ID	0x6601

#ifdef CONFIG_ARCH_MVEBU
int mvebu_get_soc_id(u32 *dev, u32 *rev);
#else
static inline int mvebu_get_soc_id(u32 *dev, u32 *rev)
{
	return -1;
}
#endif

#endif /* __LINUX_MVEBU_SOC_ID_H */
