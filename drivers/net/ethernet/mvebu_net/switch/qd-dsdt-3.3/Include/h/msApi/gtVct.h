#include <Copyright.h>

/*******************************************************************************
* gtPhy.h
*
* DESCRIPTION:
*       API definitions for Marvell Phy functionality.
*
* DEPENDENCIES:
*       None.
*
* FILE REVISION NUMBER:
*       $Revision: 1 $
*******************************************************************************/

#ifndef __gtPhyh
#define __gtPhyh

#include "msApi.h"

#ifdef __cplusplus
extern "C" {
#endif

#define MARVELL_OUI_MSb        0x0141
#define MARVELL_OUI_LSb        0x0C00
#define OUI_LSb_MASK        0xFC00
#define PHY_MODEL_MASK        0x03F0
#define PHY_REV_MASK        0x000F

#define DEV_E3082        0x8 << 4
#define DEV_E104X        0x2 << 4
#define DEV_E1111        0xC << 4
#define DEV_E1112        0x9 << 4
#define DEV_E114X        0xD << 4
#define DEV_E1149        0xA << 4
#define DEV_E1181        0xE << 4
#define DEV_EC010        0x3 << 4
#define DEV_G15LV        0xB << 4    /* 88E6165 internal copper phy, 88E1240 */
#define DEV_S15LV        0x0 << 4    /* 88E6165 internal SERDES */
#define DEV_MELODY       0x26 << 4   /* 88EC000 internal copper phy, 88E3020-88E6250 */
#define DEV_G65G         0x27 << 4    /* 88E6375 (Amber) internal copper phy, 88E1340 */
#define DEV_E1540        0x2B << 4    /* 88E6352 (Agate) internal copper phy, 88E1540 */

typedef struct _GT_PHY_INFO
{
    GT_U32    phyId;        /* Marvell PHY ID (register 3) */
    GT_U32    anyPage;    /* each bit represents if the corresponding register is any page */
    GT_U32    flag;        /* see below for definition */
    GT_U8    vctType;    /* VCT Register Type */
    GT_U8    exStatusType;    /* EX Status Register Type */
    GT_U8    dteType;    /* DTE Register Type */
    GT_U8    pktGenType;    /* Pkt Generator Reg. Type */
    GT_U8    macIfLoopType;        /* MAC IF Loopback Reg. Type */
    GT_U8    lineLoopType;        /* Line Loopback Reg. Type */
    GT_U8    exLoopType;        /* External Loopback Reg. Type */
    GT_U8    pageType;        /* Page Restriction Type */
} GT_PHY_INFO;

/* GT_PHY_INFO flag definition */
#define GT_PHY_VCT_CAPABLE        0x0001
#define GT_PHY_DTE_CAPABLE        0x0002
#define GT_PHY_EX_CABLE_STATUS    0x0004
#define GT_PHY_ADV_VCT_CAPABLE    0x0008
#define GT_PHY_PKT_GENERATOR    0x0010
#define GT_PHY_MAC_IF_LOOP        0x0100
#define GT_PHY_LINE_LOOP        0x0200
#define GT_PHY_EXTERNAL_LOOP    0x0400
#define GT_PHY_RESTRICTED_PAGE    0x0800
#define GT_PHY_GIGABIT            0x8000
#define GT_PHY_COPPER             0x4000
#define GT_PHY_FIBER              0x2000
#define GT_PHY_SERDES_CORE        0x1000

/* VCT Register Type */
#define GT_PHY_VCT_TYPE1    1    /* 10/100 Fast Ethernet */
#define GT_PHY_VCT_TYPE2    2    /* 1000M without page support */
#define GT_PHY_VCT_TYPE3    3    /* 1000M without page but with work around */
#define GT_PHY_VCT_TYPE4    4    /* 1000M with page support */

/* ADV VCT Register Type */
#define GT_PHY_ADV_VCT_TYPE1    5    /* 88E1181 type device, not supported */
#define GT_PHY_ADV_VCT_TYPE2    6    /* 88E6165 family devies */

/* Extended Status Type */
#define GT_PHY_EX_STATUS_TYPE1    1    /* 88E1111, 88E1141, 88E1145 */
#define GT_PHY_EX_STATUS_TYPE2    2    /* 88E1112 */
#define GT_PHY_EX_STATUS_TYPE3    3    /* 88E1149 */
#define GT_PHY_EX_STATUS_TYPE4    4    /* 88E1181 */
#define GT_PHY_EX_STATUS_TYPE5    5    /* 88E1116 */
#define GT_PHY_EX_STATUS_TYPE6    6    /* 88E6165 family devices */

/* DTE Register Type */
#define GT_PHY_DTE_TYPE1    1    /* 10/100 Fast Ethernet with workaround */
#define GT_PHY_DTE_TYPE2    2    /* 1000M without page support */
#define GT_PHY_DTE_TYPE3    3    /* 1000M without page but with work around */
#define GT_PHY_DTE_TYPE4    4    /* 1000M with page support */
#define GT_PHY_DTE_TYPE5    5    /* 10/100 Fast Ethernet */

/* Pkt Generator Register Type */
#define GT_PHY_PKTGEN_TYPE1    1    /* Uses Register 30 */
#define GT_PHY_PKTGEN_TYPE2    2    /* Uses Register 16 */
#define GT_PHY_PKTGEN_TYPE3    3    /* Uses Register 25 */

/* MAC Interface Loopback Register Type */
#define GT_PHY_LOOPBACK_TYPE0    0    /* Don't do anything */
#define GT_PHY_LOOPBACK_TYPE1    1    /* 0.14 only */
#define GT_PHY_LOOPBACK_TYPE2    2    /* For DEV_G15LV like device */
#define GT_PHY_LOOPBACK_TYPE3    3    /* For DEV_S15LV like device */
#define GT_PHY_LOOPBACK_TYPE4    4    /* For DEV_E1111 like device */

/* Line Loopback Register Type */
#define GT_PHY_LINE_LB_TYPE1    1    /* 0_2.14 */
#define GT_PHY_LINE_LB_TYPE2    2    /* 21_2.14 */
#define GT_PHY_LINE_LB_TYPE3    3    /* 20.14 */
#define GT_PHY_LINE_LB_TYPE4    4    /* 16.12 */

/* External Loopback Register Type */
#define GT_PHY_EX_LB_TYPE0    0    /* Don't do anything */
#define GT_PHY_EX_LB_TYPE1    1    /* For DEV_E1111 like dev */
#define GT_PHY_EX_LB_TYPE2    2    /* For DEV_E1149 like dev */

/* Restricted Page Access Type */
#define GT_PHY_PAGE_WRITE_BACK    0    /* For every device */
#define GT_PHY_PAGE_DIS_AUTO1    1    /* For 88E1111 type */
#define GT_PHY_PAGE_DIS_AUTO2    2    /* For 88E1121 type */
#define GT_PHY_NO_PAGE            3    /* No Pages */


/* definition for formula to calculate actual distance */
#ifdef FP_SUPPORT
#define FORMULA_PHY100M(_data)    ((_data)*0.7861 - 18.862)
#define FORMULA_PHY1000M(_data)    ((_data)*0.8018 - 28.751)
#else
#define FORMULA_PHY100M(_data)    (((long)(_data)*7861 - 188620)/10000 + (((((long)(_data)*7861 - 188620)%10000) >= 5000)?1:0))
#define FORMULA_PHY1000M(_data)    (((long)(_data)*8018 - 287510)/10000 + (((((long)(_data)*8018 - 287510)%10000) >= 5000)?1:0))
#endif

#define GT_ADV_VCT_CALC(_data)        \
        (((long)(_data)*8333 - 191667)/10000 + (((((long)(_data)*8333 - 191667)%10000) >= 5000)?1:0))

#define GT_ADV_VCT_CALC_SHORT(_data)        \
        (((long)(_data)*7143 - 71429)/10000 + (((((long)(_data)*7143 - 71429)%10000) >= 5000)?1:0))

/* macro to check VCT Failure */
#define IS_VCT_FAILED(_reg)        \
        (((_reg) & 0xFF) == 0xFF)

/* macro to find out if Amplitude is zero */
#define IS_ZERO_AMPLITUDE(_reg)    \
        (((_reg) & 0x7F00) == 0)

/* macro to retrieve Amplitude */
#define GET_AMPLITUDE(_reg)    \
        (((_reg) & 0x7F00) >> 8)

/* macro to find out if Amplitude is positive */
#define IS_POSITIVE_AMPLITUDE(_reg)    \
        (((_reg) & 0x8000) == 0x8000)

typedef struct _VCT_REGISTER
{
    GT_U8    page;
    GT_U8    regOffset;
} VCT_REGISTER;


#ifdef __cplusplus
}
#endif

#endif /* __gtPhyh */
