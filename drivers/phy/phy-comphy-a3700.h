#ifndef _COMPHY_A3700_H
#define _COMPHY_A3700_H

extern const struct mvebu_comphy_soc_info a3700_comphy;

#define PLL_SET_DELAY_US		600
#define A3700_COMPHY_PLL_LOCK_TIMEOUT	1000

enum {
	COMPHY_LANE0 = 0,
	COMPHY_LANE1,
	COMPHY_LANE2,
	COMPHY_LANE_MAX,
};
#define COMPHY_SELECTOR_PHY_REG_OFFSET		0xFC
/* bit0: 0: Lane0 is GBE0; 1: Lane1 is PCIE */
#define COMPHY_SELECTOR_PCIE_GBE0_SEL_BIT	BIT(0)
/* bit4: 0: Lane1 is GBE1; 1: Lane1 is USB3 */
#define COMPHY_SELECTOR_USB3_GBE1_SEL_BIT	BIT(4)
/* bit8: 0: Lane1 is USB, Lane2 is SATA; 1: Lane2 is USB3 */
#define COMPHY_SELECTOR_USB3_PHY_SEL_BIT	BIT(8)

/* SATA PHY register offset */
#define SATAPHY_LANE2_REG_BASE_OFFSET	0x200

/* USB3 PHY offset compared to SATA PHY */
#define USB3PHY_LANE2_REG_BASE_OFFSET	0x200

/* Comphy lane2 indirect access register offset */
#define COMPHY_LANE2_INDIR_ADDR_OFFSET		0x0
#define COMPHY_LANE2_INDIR_DATA_OFFSET		0x4

/* PHY shift to get related register address */
enum {
	PCIE = 1,
	USB3,
};
#define PCIEPHY_SHFT			2
#define USB3PHY_SHFT			2
#define PHY_SHFT(unit)			((unit == PCIE) ? PCIEPHY_SHFT : USB3PHY_SHFT)

/* PHY register */
#define COMPHY_POWER_PLL_CTRL			0x01
#define PWR_PLL_CTRL_ADDR(unit)			(COMPHY_POWER_PLL_CTRL * PHY_SHFT(unit))
#define REF_FREF_SEL_OFFSET			0
#define REF_FREF_SEL_MASK			(0x1F << REF_FREF_SEL_OFFSET)
#define REF_CLOCK_SPEED_25M			0x1
#define REF_CLOCK_SPEED_40M			0x3
#define PHY_MODE_OFFSET				5
#define PHY_MODE_MASK				(7 << PHY_MODE_OFFSET)
#define PHY_MODE_SATA				0x0
#define PHY_MODE_SGMII				0x4
#define PHY_MODE_USB3				0x5

#define COMPHY_KVCO_CAL_CTRL			0x02
#define KVCO_CAL_CTRL_ADDR(unit)		(COMPHY_KVCO_CAL_CTRL * PHY_SHFT(unit))
#define USE_MAX_PLL_RATE_BIT			BIT(12)

#define COMPHY_RESERVED_REG			0x0e
#define PHYCTRL_FRM_PIN_BIT			BIT(13)

#define COMPHY_LOOPBACK_REG0			0x23
#define DIG_LB_EN_ADDR(unit)			(COMPHY_LOOPBACK_REG0 * PHY_SHFT(unit))
#define SEL_DATA_WIDTH_OFFSET			10
#define SEL_DATA_WIDTH_MASK			(0x3 << SEL_DATA_WIDTH_OFFSET)
#define DATA_WIDTH_10BIT			0x0
#define DATA_WIDTH_20BIT			0x1
#define DATA_WIDTH_40BIT			0x2
#define PLL_READY_TX_BIT			BIT(4)

#define COMPHY_SYNC_PATTERN_REG			0x24
#define SYNC_PATTERN_REG_ADDR(unit)		(COMPHY_SYNC_PATTERN_REG * PHY_SHFT(unit))
#define TXD_INVERT_BIT				BIT(10)
#define RXD_INVERT_BIT				BIT(11)

#define COMPHY_MISC_REG0_ADDR			0x4F
#define MISC_REG0_ADDR(unit)			(COMPHY_MISC_REG0_ADDR * PHY_SHFT(unit))
#define CLK100M_125M_EN				BIT(4)
#define CLK500M_EN				BIT(7)
#define PHY_REF_CLK_SEL				BIT(10)




#endif /* _COMPHY_A3700_H */

