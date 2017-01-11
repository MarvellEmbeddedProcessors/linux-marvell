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

#endif /* _COMPHY_A3700_H */

