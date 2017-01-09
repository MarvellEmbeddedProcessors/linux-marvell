#ifndef _DT_BINDINGS_PHY_COMPHY_MVEBU
#define _DT_BINDINGS_PHY_COMPHY_MVEBU

/* A lane is described by 2 fields:
 *	- 8 lsb represent the index of the lane
 *	- 24 msb represent the mode
 */
#define COMPHY_DEF(x, y)	(((x) << 8) | (y))
/* Macro the extract the mode from lane description */
#define COMPHY_GET_MODE(x)	((x & 0xFFFFFF00) >> 8)
/* Macro the extract the id from lane description */
#define COMPHY_GET_ID(x)	(x & 0xff)

#define	COMPHY_UNUSED		COMPHY_DEF(0xff, 0xff)
#define COMPHY_SATA0		COMPHY_DEF(0x1,  0x0)
#define COMPHY_SATA1		COMPHY_DEF(0x1,  0x1)
#define COMPHY_SGMII0		COMPHY_DEF(0x2,  0x0)	/* SGMII 1G */
#define COMPHY_SGMII1		COMPHY_DEF(0x2,  0x1)	/* SGMII 1G */
#define COMPHY_SGMII2		COMPHY_DEF(0x2,  0x2)	/* SGMII 1G */
#define COMPHY_HS_SGMII0	COMPHY_DEF(0x3,  0x0)	/* SGMII 2.5G */
#define COMPHY_HS_SGMII1	COMPHY_DEF(0x3,  0x1)	/* SGMII 2.5G */
#define COMPHY_HS_SGMII2	COMPHY_DEF(0x3,  0x2)	/* SGMII 2.5G */
#define COMPHY_USB3H0		COMPHY_DEF(0x4,  0x0)
#define COMPHY_USB3H1		COMPHY_DEF(0x4,  0x1)
#define COMPHY_USB3D0		COMPHY_DEF(0x5,  0x0)
#define COMPHY_PCIE0		COMPHY_DEF(0x6,  0x0)
#define COMPHY_PCIE1		COMPHY_DEF(0x6,  0x1)
#define COMPHY_PCIE2		COMPHY_DEF(0x6,  0x2)
#define COMPHY_PCIE3		COMPHY_DEF(0x6,  0x3)
#define COMPHY_RXAUI0		COMPHY_DEF(0x7,  0x0)
#define COMPHY_RXAUI1		COMPHY_DEF(0x7,  0x1)
#define COMPHY_XFI		COMPHY_DEF(0x8,  0x0)
#define COMPHY_SFI		COMPHY_DEF(0x9,  0x0)

#define COMPHY_SATA_MODE	0x1
#define COMPHY_SGMII_MODE	0x2	/* SGMII 1G */
#define COMPHY_HS_SGMII_MODE	0x3	/* SGMII 2.5G */
#define COMPHY_USB3H_MODE	0x4
#define COMPHY_USB3D_MODE	0x5
#define COMPHY_PCIE_MODE	0x6
#define COMPHY_RXAUI_MODE	0x7
#define COMPHY_XFI_MODE		0x8
#define COMPHY_SFI_MODE		0x9

#endif /* _DT_BINDINGS_PHY_COMPHY_MVEBU */

