#include <Copyright.h>

/********************************************************************************
* gtMiiSmiIf.h
*
* DESCRIPTION:
*       Includes functions prototypes for initializing and accessing the
*       MII / SMI interface.
*       This is the only file to be included from upper layers.
*
* DEPENDENCIES:
*       None.
*
* FILE REVISION NUMBER:
*       $Revision: 2 $
*
*******************************************************************************/

#ifndef __gtMiiSmiIfh
#define __gtMiiSmiIfh


#include <msApi.h>

#ifdef __cplusplus
extern "C" {
#endif

/*******************************************************************************
* miiSmiIfInit
*
* DESCRIPTION:
*       This function initializes the MII / SMI interface.
*
* INPUTS:
*       None.
*
* OUTPUTS:
*       highSmiDevAddr - Indicates whether to use the high device register
*                     addresses when accessing switch's registers (of all kinds)
*                     i.e, the devices registers range is 0x10 to 0x1F, or to
*                     use the low device register addresses (range 0x0 to 0xF).
*                       GT_TRUE     - use high addresses (0x10 to 0x1F).
*                       GT_FALSE    - use low addresses (0x0 to 0xF).
*
* RETURNS:
*       DEVICE_ID       - on success
*       0     - on error
*
* COMMENTS:
*       None.
*
*******************************************************************************/
GT_U16 miiSmiIfInit
(
    IN  GT_QD_DEV *dev,
    OUT GT_BOOL   * highSmiDevAddr
);

/*******************************************************************************
* miiSmiManualIfInit
*
* DESCRIPTION:
*       This function returns Device ID from the given base address
*
* INPUTS:
*       baseAddr - either 0x0 or 0x10. Indicates whether to use the low device
*                    register address or high device register address.
*                    The device register range is from 0x0 to 0xF or from 0x10
*                    to 0x1F for 5 port switchs and from 0x0 to 0x1B for 8 port
*                    switchs.
*
* OUTPUTS:
*       None.
*
* RETURNS:
*       DEVICE_ID       - on success
*       0    - on error
*
* COMMENTS:
*       None.
*
*******************************************************************************/
GT_U16 miiSmiManualIfInit
(
    IN  GT_QD_DEV   *dev,
    IN  GT_U32        baseAddr
);

/*******************************************************************************
* miiSmiIfReadRegister
*
* DESCRIPTION:
*       This function reads a register throw the SMI / MII interface, to be used
*       by upper layers.
*
* INPUTS:
*       phyAddr     - The PHY address to be read.
*       regAddr     - The register address to read.
*
* OUTPUTS:
*       data        - The register's data.
*
* RETURNS:
*       GT_OK   - on success
*       GT_FAIL - on error
*
* COMMENTS:
*       None.
*
*******************************************************************************/
GT_STATUS miiSmiIfReadRegister
(
    IN  GT_QD_DEV *dev,
    IN  GT_U8     phyAddr,
    IN  GT_U8     regAddr,
    OUT GT_U16    *data
);


/*******************************************************************************
* miiSmiIfWriteRegister
*
* DESCRIPTION:
*       This function writes to a register throw the SMI / MII interface, to be
*       used by upper layers.
*
* INPUTS:
*       phyAddr     - The PHY address to be read.
*       regAddr     - The register address to read.
*       data        - The data to be written to the register.
*
* OUTPUTS:
*       None.
*
* RETURNS:
*       GT_OK   - on success
*       GT_FAIL - on error
*
* COMMENTS:
*       None.
*
*******************************************************************************/
GT_STATUS miiSmiIfWriteRegister
(
    IN  GT_QD_DEV *dev,
    IN  GT_U8     phyAddr,
    IN  GT_U8     regAddr,
    IN  GT_U16    data
);

/*****************************************************************************
* qdMultiAddrRead
*
* DESCRIPTION:
*       This function reads data from a device in the secondary MII bus.
*
* INPUTS:
*       phyAddr     - The PHY address to be read.
*       regAddr     - The register address to read.
*       value       - The storage where register date to be saved.
*
* OUTPUTS:
*       None.
*
* RETURNS:
*       GT_TRUE   - on success
*       GT_FALSE  - on error
*
* COMMENTS:
*       None.
*
*******************************************************************************/
GT_BOOL qdMultiAddrRead (GT_QD_DEV* dev, unsigned int phyAddr , unsigned int regAddr, unsigned int* value);

/*****************************************************************************
* qdMultiAddrWrite
*
* DESCRIPTION:
*       This function writes data to the device in the secondary MII bus.
*
* INPUTS:
*       phyAddr     - The PHY address to be read.
*       regAddr     - The register address to read.
*       value       - The data to be written into the register.
*
* OUTPUTS:
*       None.
*
* RETURNS:
*       GT_TRUE   - on success
*       GT_FALSE  - on error
*
* COMMENTS:
*       None.
*
*******************************************************************************/
GT_BOOL qdMultiAddrWrite (GT_QD_DEV* dev, unsigned int phyAddr , unsigned int regAddr, unsigned int value);

#ifdef GT_RMGMT_ACCESS
/*******************************************************************************
* qdAccessRegs
*
* DESCRIPTION:
*       This function access registers through device interface by user, to be
*       used by upper layers.
*
* INPUTS:
*   regList     - list of HW_DEV_RW_REG.
*     HW_DEV_RW_REG:
*     cmd - HW_REG_READ, HW_REG_WRITE, HW_REG_WAIT_TILL_0 or HW_REG_WAIT_TILL_1
*     addr - SMI Address
*     reg  - Register offset
*     data - INPUT,OUTPUT:Value in the Register or Bit number
*
* OUTPUTS:
*   regList     - list of HW_DEV_RW_REG.
*
* RETURNS:
*       GT_OK   - on success
*       GT_FAIL - on error
*
* COMMENTS:
*       None.
*
*******************************************************************************/
GT_STATUS qdAccessRegs
(
    IN  GT_QD_DEV    *dev,
    INOUT HW_DEV_REG_ACCESS *regList
);
#endif

#ifdef __cplusplus
}
#endif

#endif /* __gtMiiSmiIfh */
