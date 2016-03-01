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

#ifndef __CLK_MVEBU_ARMADA_3700_H_
#define __CLK_MVEBU_ARMADA_3700_H_

enum a3700_clock_src {
	TBG_A,
	TBG_B
};

enum a3700_clock_line {
	TBG_A_P = 0,
	TBG_B_P = 1,
	TBG_A_S = 2,
	TBG_B_S = 3
};

struct a3700_clk_desc {
	u32 (*get_tbg_freq)(void __iomem *reg, int tbg_index);
	const char* (*get_tbg_name)(int tbg_index);
	void (*get_clk_ratio)(void __iomem *reg, int id, int *tbg_index, int *mult, int *div);
	const struct coreclk_ratio *ratios;
	int num_ratios;
	int num_tbg;
};

#define MVEBU_A3700_TBG_CLK_NUM		4

/* Register offset of NORTH_CLOCK_REGS_BASE */
#define MVEBU_SOUTH_CLOCK_REGS_BASE	(0x5000)
#define MVEBU_TESTPIN_NORTH_REG_BASE	(0x800)

/* Reset sample */
#define MVEBU_TEST_PIN_LATCH_N		(MVEBU_TESTPIN_NORTH_REG_BASE + 0x8)

/********************************/
/* REF Clock                    */
/********************************/
#define MVEBU_XTAL_MODE_MASK		0x00000200
#define MVEBU_XTAL_MODE_OFFS		(9)
#define MVEBU_XTAL_CLOCK_25MHZ		(0x0)
#define MVEBU_XTAL_CLOCK_40MHZ		(0x1)

/****************/
/* North Bridge */
/****************/
#define MVEBU_NORTH_BRG_PLL_BASE		(0x200)
#define MVEBU_NORTH_BRG_TBG_CFG			(MVEBU_NORTH_BRG_PLL_BASE + 0x0)
#define MVEBU_NORTH_BRG_TBG_CTRL0		(MVEBU_NORTH_BRG_PLL_BASE + 0x4)
#define MVEBU_NORTH_BRG_TBG_CTRL1		(MVEBU_NORTH_BRG_PLL_BASE + 0x8)
#define MVEBU_NORTH_BRG_TBG_CTRL2		(MVEBU_NORTH_BRG_PLL_BASE + 0xC)
#define MVEBU_NORTH_BRG_TBG_CTRL3		(MVEBU_NORTH_BRG_PLL_BASE + 0x10)
#define MVEBU_NORTH_BRG_TBG_CTRL4		(MVEBU_NORTH_BRG_PLL_BASE + 0x14)
#define MVEBU_NORTH_BRG_TBG_CTRL5		(MVEBU_NORTH_BRG_PLL_BASE + 0x18)
#define MVEBU_NORTH_BRG_TBG_CTRL6		(MVEBU_NORTH_BRG_PLL_BASE + 0x1C)
#define MVEBU_NORTH_BRG_TBG_CTRL7		(MVEBU_NORTH_BRG_PLL_BASE + 0x20)
#define MVEBU_NORTH_BRG_TBG_CTRL8		(MVEBU_NORTH_BRG_PLL_BASE + 0x30)

/* tbg definition */
#define MVEBU_TBG_CLK_SEL_MASK			0x3
#define MVEBU_TBG_CLK_PRSCL_MASK		0x7
#define MVEBU_TBG_CLK_DIV_MASK			0x1

#define MVEBU_TBG_DIV_MASK			0x1FFUL
#define MVEBU_TBG_A_REFDIV_OFFSET		(0)
#define MVEBU_TBG_B_REFDIV_OFFSET		(16)

#define MVEBU_TBG_A_FBDIV_OFFSET		(2)
#define MVEBU_TBG_B_FBDIV_OFFSET		(18)

#define MVEBU_TBG_VCODIV_MAX	0x7
#define MVEBU_TBG_A_VCODIV_SE_OFFSET		(0)
#define MVEBU_TBG_A_VCODIV_DIFF_OFFSET		(1)
#define MVEBU_TBG_B_VCODIV_SE_OFFSET		(16)
#define MVEBU_TBG_B_VCODIV_DIFF_OFFSET		(17)

/* north bridge clock TBG select register */
#define MVEBU_NORTH_CLOCK_TBG_SELECT_REG	(0x0)
#define TBG_WCPU_PCLK_SEL_OFFSET		(22)
#define TBG_SATA_HOST_PCLK_SEL_OFFSET		(2)
#define TBG_MMC_PCLK_SEL_OFFSET		(0)

/* north bridge clock divider select registers */
#define MVEBU_NORTH_CLOCK_DIVIDER_SELECT0_REG	(0x4)
#define WCPU_CLK_DIV_PRSCL_OFFSET	(28)

#define MVEBU_NORTH_CLOCK_DIVIDER_SELECT1_REG	(0x8)

#define MVEBU_NORTH_CLOCK_DIVIDER_SELECT2_REG	(0xC)
#define MMC_CLK_PRSCL1_OFFSET		(16)
#define MMC_CLK_PRSCL2_OFFSET		(13)
#define SATA_HOST_CLK_PRSCL1_OFFSET		(10)
#define SATA_HOST_CLK_PRSCL2_OFFSET		(7)

/****************/
/* South Bridge */
/****************/

/* north bridge clock TBG select register */
#define MVEBU_SOUTH_CLOCK_TBG_SELECT_REG	(MVEBU_SOUTH_CLOCK_REGS_BASE + 0x0)
#define TBG_USB32_SS_CLK_SEL_OFFSET	(18)
#define TBG_GBE_CORE_CLK_SEL_OFFSET	(8)

#define MVEBU_SOUTH_CLOCK_DIVIDER_SELECT0_REG	(MVEBU_SOUTH_CLOCK_REGS_BASE + 0x4)
#define USB32_SS_SYS_CLK_PRSCL1_OFFSET		(18)
#define USB32_SS_SYS_CLK_PRSCL2_OFFSET		(15)


#define MVEBU_SOUTH_CLOCK_DIVIDER_SELECT1_REG	(MVEBU_SOUTH_CLOCK_REGS_BASE + 0x8)
#define GBE_CORE_CLK_PRSCL1_OFFSET		(21)
#define GBE_CORE_CLK_PRSCL2_OFFSET		(18)
#define GBE0_CORE_CLK_DIV_OFFSET		(14)
#define GBE1_CORE_CLK_DIV_OFFSET		(13)

#define MVEBU_SOUTH_CLOCK_DIVIDER_SELECT2_REG	(MVEBU_SOUTH_CLOCK_REGS_BASE + 0xC)

#endif

