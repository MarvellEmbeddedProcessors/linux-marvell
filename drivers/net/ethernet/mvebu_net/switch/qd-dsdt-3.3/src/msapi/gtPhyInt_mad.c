#include <Copyright.h>

/********************************************************************************
* gtPhyInt.h
*
* DESCRIPTION:
* API definitions for PHY interrupt handling
*
* DEPENDENCIES:
* None.
*
* FILE REVISION NUMBER:
* $Revision: 10 $
*******************************************************************************/

#include <msApi.h>
#include <gtHwCntl.h>
#include <gtDrvSwRegs.h>
#include <gtDrvConfig.h>
#include "madApi.h"

/*******************************************************************************
* gprtPhyIntEnable_mad
*
* DESCRIPTION:
* Enable/Disable one PHY Interrupt
* This register determines whether the INT# pin is asserted when an interrupt
* event occurs. When an interrupt occurs, the corresponding bit is set and
* remains set until register 19 is read via the SMI. When interrupt enable
* bits are not set in register 18, interrupt status bits in register 19 are
* still set when the corresponding interrupt events occur. However, the INT#
* is not asserted.
*
* INPUTS:
* port -   The logical port number, unless SERDES device is accessed
*          The physical address, if SERDES device is accessed
* intType - the type of interrupt to enable/disable. any combination of
*            GT_SPEED_CHANGED,
*            GT_DUPLEX_CHANGED,
*            GT_PAGE_RECEIVED,
*            GT_AUTO_NEG_COMPLETED,
*            GT_LINK_STATUS_CHANGED,
*            GT_SYMBOL_ERROR,
*            GT_FALSE_CARRIER,
*            GT_FIFO_FLOW,
*            GT_CROSSOVER_CHANGED,    ( Copper only )
*            GT_DOWNSHIFT_DETECT,    ( for 1000M Copper only )
*            GT_ENERGY_DETECT,        ( for 1000M Copper only )
*            GT_POLARITY_CHANGED, and ( Copper only )
*            GT_JABBER                (Copper only )
*
*
* OUTPUTS:
* None.
*
* RETURNS:
* GT_OK - on success
* GT_FAIL - on error
*
* COMMENTS:
* For 88E6131, 88E6122, and 88E6108 devices, Serdes port can be accessed using
* logical port number.
* For 88E6161 88E6165 and 88E6352 devices, Serdes port 5 (address 0xD/0xF) can be accessed
* using logical port number, but not port 4 (since port 4 could be an internal
* PHY.)
*******************************************************************************/


GT_STATUS gprtPhyIntEnable_mad
(
IN GT_QD_DEV    *dev,
IN GT_LPORT    port,
IN GT_U16    intType
)
{
    GT_U8           hwPort;         /* the physical port number     */
    MAD_INT_TYPE    mintType;

    DBG_INFO(("gprtPhyIntEnable_mad Called.\n"));

    /* translate LPORT to hardware port */
    hwPort = GT_LPORT_2_PHY(port);

    if((IS_IN_DEV_GROUP(dev,DEV_SERDES_CORE)) && (hwPort > 3))
    {
        if(!(dev->validSerdesVec & (1 << hwPort)))
        {
            if(!((IS_IN_DEV_GROUP(dev,DEV_SERDES_ACCESS_CONFIG)) && (hwPort == 4)))
                GT_GET_SERDES_PORT(dev,&hwPort);
        }
        if(hwPort >= dev->maxPhyNum)
        {
            return GT_NOT_SUPPORTED;
        }
    }

    /* check if the port is configurable */
    if(!IS_CONFIGURABLE_PHY(dev,hwPort))
    {
        return GT_NOT_SUPPORTED;
    }

    mintType.intGroup0 = 0;
    mintType.intGroup1 = 0;
    mintType.intGroup0 = intType;
    if(mdIntSetEnable(&(dev->mad_dev),hwPort,&mintType) != MAD_OK)
    {
        DBG_INFO(("Call mdIntSetEnable failed.\n"));
        return GT_FAIL;
    }

    return GT_OK;

}

/*******************************************************************************
* gprtGetPhyIntStatus_mad
*
* DESCRIPTION:
* Check to see if a specific type of interrupt occured
*
* INPUTS:
* port -   The logical port number, unless SERDES device is accessed
*          The physical address, if SERDES device is accessed
* intType - the type of interrupt which causes an interrupt.
*            any combination of
*            GT_SPEED_CHANGED,
*            GT_DUPLEX_CHANGED,
*            GT_PAGE_RECEIVED,
*            GT_AUTO_NEG_COMPLETED,
*            GT_LINK_STATUS_CHANGED,
*            GT_SYMBOL_ERROR,
*            GT_FALSE_CARRIER,
*            GT_FIFO_FLOW,
*            GT_CROSSOVER_CHANGED,    ( Copper only )
*            GT_DOWNSHIFT_DETECT,    ( for 1000M Copper only )
*            GT_ENERGY_DETECT,        ( for 1000M Copper only )
*            GT_POLARITY_CHANGED, and ( Copper only )
*            GT_JABBER                (Copper only )
*
* OUTPUTS:
* None.
*
* RETURNS:
* GT_OK - on success
* GT_FAIL - on error
*
* COMMENTS:
* For 88E6131, 88E6122, and 88E6108 devices, Serdes port can be accessed using
* logical port number.
* For 88E6161 88E6165 and 88E6352 devices, Serdes port 5 (address 0xD/0xF) can be accessed
* using logical port number, but not port 4 (since port 4 could be an internal
* PHY.)
*
*******************************************************************************/

GT_STATUS gprtGetPhyIntStatus_mad
(
IN   GT_QD_DEV  *dev,
IN   GT_LPORT   port,
OUT  GT_U16*    intType
)
{
    GT_U8           hwPort;         /* the physical port number     */
    MAD_INT_TYPE    mintType;

    DBG_INFO(("gprtGetPhyIntStatus_mad Called.\n"));

    /* translate LPORT to hardware port */
    hwPort = GT_LPORT_2_PHY(port);
    if((IS_IN_DEV_GROUP(dev,DEV_SERDES_CORE)) && (hwPort > 3))
    {
        if(!(dev->validSerdesVec & (1 << hwPort)))
        {
            if(!((IS_IN_DEV_GROUP(dev,DEV_SERDES_ACCESS_CONFIG)) && (hwPort == 4)))
                GT_GET_SERDES_PORT(dev,&hwPort);
        }
        if(hwPort >= dev->maxPhyNum)
        {
            return GT_NOT_SUPPORTED;
        }
    }

    /* check if the port is configurable */
    if(!IS_CONFIGURABLE_PHY(dev,hwPort))
    {
        return GT_NOT_SUPPORTED;
    }

    if(mdIntGetStatus(&(dev->mad_dev),hwPort,&mintType) != MAD_OK)
    {
        DBG_INFO(("Call mdIntGetStatus failed.\n"));
        return GT_FAIL;
    }

    *intType = mintType.intGroup0;

    return GT_OK;
}

/*******************************************************************************
* gprtGetPhyIntPortSummary_mad
*
* DESCRIPTION:
* Lists the ports that have active interrupts. It provides a quick way to
* isolate the interrupt so that the MAC or switch does not have to poll the
* interrupt status register (19) for all ports. Reading this register does not
* de-assert the INT# pin
*
* INPUTS:
* none
*
* OUTPUTS:
* GT_U8 *intPortMask - bit Mask with the bits set for the corresponding
* phys with active interrupt. E.g., the bit number 0 and 2 are set when
* port number 0 and 2 have active interrupt
*
* RETURNS:
* GT_OK - on success
* GT_FAIL - on error
*
* COMMENTS:
* 88E3081 data sheet register 20
* For 88E6165, 88E6375 devices, geventGetDevIntStatus should be used instead.
*
*******************************************************************************/

GT_STATUS gprtGetPhyIntPortSummary_mad
(
IN  GT_QD_DEV  *dev,
OUT GT_U16     *intPortMask
)
{
    GT_STATUS       retVal;
    GT_U8           hwPort;         /* the physical port number     */
    GT_U16          portVec;
    MAD_U32         mportVec;

    DBG_INFO(("gprtGetPhyIntPortSummary_mad Called.\n"));

    /* translate LPORT 0 to hardware port */
    hwPort = GT_LPORT_2_PORT(0);

    *intPortMask=0;

    if (IS_IN_DEV_GROUP(dev,DEV_DEV_PHY_INTERRUPT))
    {
        return GT_NOT_SUPPORTED;
    }

    if (IS_IN_DEV_GROUP(dev,DEV_INTERNAL_GPHY))
    {
        /* get the interrupt port summary from global register */
        retVal = hwGetGlobal2RegField(dev,QD_REG_PHYINT_SOURCE,0,dev->maxPorts,&portVec);
        GT_GIG_PHY_INT_MASK(dev,portVec);
        *intPortMask = (GT_U16)GT_PORTVEC_2_LPORTVEC(portVec);
    }
    else
    {
        /* get the interrupt port summary from phy */
      if(mdIntGetPortSummary(&(dev->mad_dev), &mportVec) != MAD_OK)
      {
        DBG_INFO(("Call mdIntGetPortSummary failed.\n"));
        return GT_FAIL;
      }
      portVec = mportVec;
      *intPortMask = (GT_U16)GT_PORTVEC_2_LPORTVEC(portVec);
    }

    return GT_OK;

}
