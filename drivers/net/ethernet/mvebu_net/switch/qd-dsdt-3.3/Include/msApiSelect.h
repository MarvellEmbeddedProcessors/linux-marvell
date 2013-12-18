 #include <Copyright.h>

/********************************************************************************
* msApiSelect.h
*
* DESCRIPTION:
*       API selection for QuarterDeck Device
*
* DEPENDENCIES:
*
* FILE REVISION NUMBER:
*
*******************************************************************************/

#ifndef __msApiSelect_h
#define __msApiSelect_h

/* Micro definitions */
/* Customers set self selection for DSDT  in here */

/* DSDT phy API use Mad driver(DSDT/phy) */
#if 1
#undef GT_USE_MAD
#else
#define GT_USE_MAD 1
#endif

/* DSDT uses RMGMT to replace SMI */
#if 1
#undef GT_RMGMT_ACCESS
#else
#define GT_RMGMT_ACCESS 1
#endif

/* Only for Keystone FPGA design of PTP */
#if 1
#undef CONFIG_AVB_FPGA
#undef CONFIG_AVB_FPGA_2
/*
#else
#define CONFIG_AVB_FPGA  1
#define CONFIG_AVB_FPGA_2 1
*/
#endif

/* To use port mapping functions in Dev configuration */
#if 1
#undef GT_PORT_MAP_IN_DEV
#else
#define GT_PORT_MAP_IN_DEV  1
#endif



#endif /* __msApiSelect_h */
