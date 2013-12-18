#include <Copyright.h>

/********************************************************************************
* gtPhyCtrl.h
*
* DESCRIPTION:
* API definitions for PHY control facility.
*
* DEPENDENCIES:
* None.
*
* FILE REVISION NUMBER:
* $Revision: 10 $
*******************************************************************************/

#include <msApi.h>
#include <gtHwCntl.h>
#include <gtDrvConfig.h>
#include <gtDrvSwRegs.h>
#include <gtVct.h>
#include <gtSem.h>


#ifdef GT_USE_MAD
#include "madApi.h"
#include "madHwCntl.h"
#endif


/*
 * This routine set Auto-Negotiation Ad Register for Copper
*/
static
GT_STATUS translateAutoMode
(
    IN    GT_PHY_INFO  *phyInfo,
    IN    GT_PHY_AUTO_MODE mode,
    OUT      MAD_BOOL       *autoEn,
    OUT   MAD_U32      *autoMode
)
{
  MAD_BOOL        autoNegoEn;
  MAD_SPEED_MODE    speedMode;
  MAD_DUPLEX_MODE duplexMode;

    switch(mode)
    {
        case SPEED_AUTO_DUPLEX_AUTO:
          autoNegoEn = MAD_TRUE;
          speedMode  = MAD_SPEED_AUTO;
          duplexMode = MAD_AUTO_DUPLEX;
        case SPEED_1000_DUPLEX_AUTO:
          autoNegoEn = MAD_TRUE;
          speedMode  = MAD_SPEED_1000M;
          duplexMode = MAD_AUTO_DUPLEX;
          break;
        case SPEED_AUTO_DUPLEX_FULL:
          autoNegoEn = MAD_TRUE;
          speedMode  = MAD_SPEED_AUTO;
          duplexMode = MAD_FULL_DUPLEX;
          break;
        case SPEED_1000_DUPLEX_FULL:
          autoNegoEn = MAD_FALSE;
          speedMode  = MAD_SPEED_1000M;
          duplexMode = MAD_FULL_DUPLEX;
          break;
        case SPEED_1000_DUPLEX_HALF:
          autoNegoEn = MAD_FALSE;
          speedMode  = MAD_SPEED_1000M;
          duplexMode = MAD_HALF_DUPLEX;
          break;
        case SPEED_AUTO_DUPLEX_HALF:
          autoNegoEn = MAD_TRUE;
          speedMode  = MAD_SPEED_AUTO;
          duplexMode = MAD_HALF_DUPLEX;
          break;
        case SPEED_100_DUPLEX_AUTO:
          autoNegoEn = MAD_FALSE;
          speedMode  = MAD_SPEED_100M;
          duplexMode = MAD_FULL_DUPLEX;
          break;
        case SPEED_10_DUPLEX_AUTO:
          autoNegoEn = MAD_TRUE;
          speedMode  = MAD_SPEED_10M;
          duplexMode = MAD_AUTO_DUPLEX;
          break;
        case SPEED_100_DUPLEX_FULL:
          autoNegoEn = MAD_FALSE;
          speedMode  = MAD_SPEED_100M;
          duplexMode = MAD_FULL_DUPLEX;
          break;
        case SPEED_100_DUPLEX_HALF:
          autoNegoEn = MAD_FALSE;
          speedMode  = MAD_SPEED_100M;
          duplexMode = MAD_HALF_DUPLEX;
          break;
        case SPEED_10_DUPLEX_FULL:
          autoNegoEn = MAD_FALSE;
          speedMode  = MAD_SPEED_10M;
          duplexMode = MAD_FULL_DUPLEX;
          break;
        case SPEED_10_DUPLEX_HALF:
          autoNegoEn = MAD_FALSE;
          speedMode  = MAD_SPEED_10M;
          duplexMode = MAD_HALF_DUPLEX;
          break;
        default:
          DBG_INFO(("Unknown Auto Mode (%d)\n",mode));
          return GT_BAD_PARAM;
    }

    *autoEn = autoNegoEn;
    if (mdGetAutoNegoMode(autoNegoEn, speedMode, duplexMode, autoMode) != MAD_OK)
      return GT_FAIL;


    return GT_OK;
}

/*
 * This routine sets Auto Mode and Reset the phy
*/
static
GT_STATUS phySetAutoMode_mad
(
    IN GT_QD_DEV *dev,
    IN GT_U8 hwPort,
    IN GT_PHY_INFO *phyInfo,
    IN GT_PHY_AUTO_MODE mode
)
{
    MAD_STATUS    status;
    MAD_U32 autoMode;
    MAD_BOOL  autoEn;

    DBG_INFO(("phySetAutoMode_mad Called.\n"));

    status = translateAutoMode(phyInfo,mode, &autoEn, &autoMode);
    if(status != GT_OK)
    {
       return status;
    }


    if(phyInfo->flag & GT_PHY_COPPER)
    {
        if((mdCopperSetAutoNeg(&(dev->mad_dev),hwPort,autoEn, autoMode)) != MAD_OK)
        {
               return GT_FAIL;
        }

    }
    else if(phyInfo->flag & GT_PHY_FIBER)
    {
        if((mdFiberSetAutoNeg(&(dev->mad_dev),hwPort,autoEn, autoMode)) != MAD_OK)
        {
               return GT_FAIL;
        }

    }

    return GT_OK;
}


/*******************************************************************************
* gprtPhyReset_mad
*
* DESCRIPTION:
*       This routine preforms PHY reset.
*        After reset, phy will be in Autonegotiation mode.
*
* INPUTS:
* port - The logical port number, unless SERDES device is accessed
*        The physical address, if SERDES device is accessed
*
* OUTPUTS:
*       None.
*
* RETURNS:
*       GT_OK   - on success
*       GT_FAIL - on error
* COMMENTS:
* data sheet register 0.15 - Reset
* data sheet register 0.13 - Speed
* data sheet register 0.12 - Autonegotiation
* data sheet register 0.8  - Duplex Mode
*******************************************************************************/

GT_STATUS gprtPhyReset_mad
(
    IN GT_QD_DEV *dev,
    IN GT_LPORT  port
)
{

    GT_STATUS       retVal;         /* Functions return value.      */
    GT_U8           hwPort;         /* the physical port number     */
    GT_PHY_INFO        phyInfo;

    DBG_INFO(("gprtPhyReset Called.\n"));

    /* translate LPORT to hardware port */
    hwPort = GT_LPORT_2_PHY(port);

    gtSemTake(dev,dev->phyRegsSem,OS_WAIT_FOREVER);

    /* check if the port is configurable */
    if((phyInfo.phyId=GT_GET_PHY_ID(dev,hwPort)) == GT_INVALID_PHY)
    {
        gtSemGive(dev,dev->phyRegsSem);
        return GT_NOT_SUPPORTED;
    }

    if(driverFindPhyInformation(dev,hwPort,&phyInfo) != GT_OK)
    {
        DBG_INFO(("Unknown PHY device.\n"));
        gtSemGive(dev,dev->phyRegsSem);
        return GT_FAIL;
    }

    /* set Auto Negotiation AD Register */
    retVal = phySetAutoMode_mad(dev,hwPort,&phyInfo,SPEED_AUTO_DUPLEX_AUTO);

    if(retVal != GT_OK)
    {
        DBG_INFO(("Failed.\n"));
    }
    else
    {
        DBG_INFO(("OK.\n"));
    }

    gtSemGive(dev,dev->phyRegsSem);

    return retVal;
}


/*******************************************************************************
* gprtSetPortLoopback_mad
*
* DESCRIPTION:
* Enable/Disable Internal Port Loopback.
* For 10/100 Fast Ethernet PHY, speed of Loopback is determined as follows:
*   If Auto-Negotiation is enabled, this routine disables Auto-Negotiation and
*   forces speed to be 10Mbps.
*   If Auto-Negotiation is disabled, the forced speed is used.
*   Disabling Loopback simply clears bit 14 of control register(0.14). Therefore,
*   it is recommended to call gprtSetPortAutoMode for PHY configuration after
*   Loopback test.
* For 10/100/1000 Gigagbit Ethernet PHY, speed of Loopback is determined as follows:
*   If Auto-Negotiation is enabled and Link is active, the current speed is used.
*   If Auto-Negotiation is disabled, the forced speed is used.
*   All other cases, default MAC Interface speed is used. Please refer to the data
*   sheet for the information of the default MAC Interface speed.
*
*
* INPUTS:
* port - The logical port number, unless SERDES device is accessed
*        The physical address, if SERDES device is accessed
* enable - If GT_TRUE, enable loopback mode
* If GT_FALSE, disable loopback mode
*
* OUTPUTS:
* None.
*
* RETURNS:
* GT_OK - on success
* GT_FAIL - on error
*
* COMMENTS:
* data sheet register 0.14 - Loop_back
*
*******************************************************************************/

GT_STATUS gprtSetPortLoopback_mad
(
    IN GT_QD_DEV *dev,
    IN GT_LPORT  port,
    IN GT_BOOL   enable
)
{
    GT_U8           hwPort;         /* the physical port number     */
    GT_PHY_INFO        phyInfo;
    MAD_STATUS status;

    DBG_INFO(("gprtSetPortLoopback_mad Called.\n"));

    /* translate LPORT to hardware port */
    hwPort = GT_LPORT_2_PHY(port);

    gtSemTake(dev,dev->phyRegsSem,OS_WAIT_FOREVER);

    /* check if the port is configurable */
    if((phyInfo.phyId=GT_GET_PHY_ID(dev,hwPort)) == GT_INVALID_PHY)
    {
        gtSemGive(dev,dev->phyRegsSem);
        return GT_NOT_SUPPORTED;
    }

    if(driverFindPhyInformation(dev,hwPort,&phyInfo) != GT_OK)
    {
        DBG_INFO(("Unknown PHY device.\n"));
        gtSemGive(dev,dev->phyRegsSem);
        return GT_FAIL;
    }

    if((status = mdDiagSetLineLoopback(&(dev->mad_dev),port,enable)) != MAD_OK)
    {
		if(status==MAD_API_LINK_DOWN)
          return GT_OK;
        DBG_INFO(("mdDiagSetLineLoopback failed.\n"));
        gtSemGive(dev,dev->phyRegsSem);
        return GT_FAIL;
    }

    gtSemGive(dev,dev->phyRegsSem);
    return GT_OK;
}

/*******************************************************************************
* gprtSetPortSpeed_mad
*
* DESCRIPTION:
*         Sets speed for a specific logical port. This function will keep the duplex
*        mode and loopback mode to the previous value, but disable others, such as
*        Autonegotiation.
*
* INPUTS:
*        port -    The logical port number, unless SERDES device is accessed
*                The physical address, if SERDES device is accessed
*        speed - port speed.
*                PHY_SPEED_10_MBPS for 10Mbps
*                PHY_SPEED_100_MBPS for 100Mbps
*                PHY_SPEED_1000_MBPS for 1000Mbps
*
* OUTPUTS:
* None.
*
* RETURNS:
* GT_OK - on success
* GT_FAIL - on error
*
* COMMENTS:
* data sheet register 0.13 - Speed Selection (LSB)
* data sheet register 0.6  - Speed Selection (MSB)
*
*******************************************************************************/

GT_STATUS gprtSetPortSpeed_mad
(
IN GT_QD_DEV *dev,
IN GT_LPORT  port,
IN GT_PHY_SPEED speed
)
{
    GT_U8           hwPort;         /* the physical port number     */
    GT_PHY_INFO        phyInfo;
    MAD_STATUS status;
    MAD_U32 mspeed;
    MAD_DUPLEX_MODE mDuplexmod;

    DBG_INFO(("gprtSetPortSpeed_mad Called.\n"));

    /* translate LPORT to hardware port */
    hwPort = GT_LPORT_2_PHY(port);

    gtSemTake(dev,dev->phyRegsSem,OS_WAIT_FOREVER);

    /* check if the port is configurable */
    if((phyInfo.phyId=GT_GET_PHY_ID(dev,hwPort)) == GT_INVALID_PHY)
    {
        gtSemGive(dev,dev->phyRegsSem);
        return GT_NOT_SUPPORTED;
    }

    if(driverFindPhyInformation(dev,hwPort,&phyInfo) != GT_OK)
    {
        DBG_INFO(("Unknown PHY device.\n"));
        gtSemGive(dev,dev->phyRegsSem);
        return GT_FAIL;
    }
    switch (speed)
    {
      case PHY_SPEED_10_MBPS:
        mspeed = 10;
        break;
      case PHY_SPEED_100_MBPS:
        mspeed = 100;
        break;
      case PHY_SPEED_1000_MBPS:
      default:
        mspeed = 1000;
        break;
    }

    if((status = mdGetDuplexStatus(&(dev->mad_dev),port,&mDuplexmod)) != MAD_OK)
    {
        DBG_INFO(("mdGetDuplexStatus failed.\n"));
        gtSemGive(dev,dev->phyRegsSem);
        return GT_FAIL;
    }


    if((status = mdCopperSetSpeedDuplex(&(dev->mad_dev),port,mspeed,((mDuplexmod)?MAD_TRUE:MAD_FALSE))) != MAD_OK)
    {
        DBG_INFO(("mdCopperSetSpeedDuplex failed.\n"));
        gtSemGive(dev,dev->phyRegsSem);
        return GT_FAIL;
    }

    gtSemGive(dev,dev->phyRegsSem);
    return GT_OK;
}


/*******************************************************************************
* gprtPortAutoNegEnable_mad
*
* DESCRIPTION:
*         Enable/disable an Auto-Negotiation.
*        This routine simply sets Auto Negotiation bit (bit 12) of Control
*        Register and reset the phy.
*        For Speed and Duplex selection, please use gprtSetPortAutoMode.
*
* INPUTS:
*        port -    The logical port number, unless SERDES device is accessed
*                The physical address, if SERDES device is accessed
*         state - GT_TRUE for enable Auto-Negotiation,
*                GT_FALSE otherwise
*
* OUTPUTS:
*         None.
*
* RETURNS:
*         GT_OK     - on success
*         GT_FAIL     - on error
*
* COMMENTS:
*         data sheet register 0.12 - Auto-Negotiation Enable
*         data sheet register 4.8, 4.7, 4.6, 4.5 - Auto-Negotiation Advertisement
*
*******************************************************************************/
GT_STATUS gprtPortAutoNegEnable_mad
(
    IN GT_QD_DEV *dev,
    IN GT_LPORT  port,
    IN GT_BOOL   state
)
{
    GT_U8           hwPort;         /* the physical port number     */
    MAD_STATUS retVal;
    MAD_SPEED_MODE speedMode;
    MAD_DUPLEX_MODE duplexMode;
    MAD_BOOL autoNegoEn;
    MAD_U32            autoMode;

    DBG_INFO(("gprtPortAutoNegEnable_mad Called.\n"));

    /* translate LPORT to hardware port */
    hwPort = GT_LPORT_2_PHY(port);

    gtSemTake(dev,dev->phyRegsSem,OS_WAIT_FOREVER);

    /* check if the port is configurable */
    if(!IS_CONFIGURABLE_PHY(dev,hwPort))
    {
        gtSemGive(dev,dev->phyRegsSem);
        return GT_NOT_SUPPORTED;
    }

    if ((retVal=mdGetSpeedStatus(&(dev->mad_dev), hwPort, &speedMode))!=MAD_OK)
    {
        DBG_INFO(("mdGetSpeedStatus Failed.\n"));
        gtSemGive(dev,dev->phyRegsSem);
        return GT_FAIL;
    }

     if ((retVal=mdGetDuplexStatus(&(dev->mad_dev), hwPort, &duplexMode))!=MAD_OK)
    {
       DBG_INFO(("mdGetDuplexStatus Failed.\n"));
       gtSemGive(dev,dev->phyRegsSem);
       return GT_FAIL;
    }

    autoNegoEn = (state==GT_TRUE)?MAD_TRUE:MAD_FALSE;

    if ((mdGetAutoNegoMode(autoNegoEn, speedMode, duplexMode, &autoMode)) != MAD_OK)
    {
        DBG_INFO(("mdGetAutoNegoMode Failed.\n"));
        gtSemGive(dev,dev->phyRegsSem);
        return GT_FAIL;
    }

     if ((retVal=mdCopperSetAutoNeg(&(dev->mad_dev), hwPort, autoNegoEn, autoMode))!=MAD_OK)
    {
        DBG_INFO(("mdCopperSetAutoNeg Failed.\n"));
        gtSemGive(dev,dev->phyRegsSem);
        return GT_FAIL;
    }

    gtSemGive(dev,dev->phyRegsSem);
    return retVal;
}

/*******************************************************************************
* gprtPortPowerDown_mad
*
* DESCRIPTION:
*         Enable/disable (power down) on specific logical port.
*        Phy configuration remains unchanged after Power down.
*
* INPUTS:
*        port -    The logical port number, unless SERDES device is accessed
*                The physical address, if SERDES device is accessed
*         state -    GT_TRUE: power down
*                 GT_FALSE: normal operation
*
* OUTPUTS:
*         None.
*
* RETURNS:
*         GT_OK     - on success
*         GT_FAIL     - on error
*
* COMMENTS:
*         data sheet register 0.11 - Power Down
*
*******************************************************************************/

GT_STATUS gprtPortPowerDown_mad
(
IN GT_QD_DEV *dev,
IN GT_LPORT  port,
IN GT_BOOL   state
)
{
    GT_U8           hwPort;         /* the physical port number     */
    MAD_STATUS retVal;
    MAD_BOOL pwMode

    DBG_INFO(("gprtPortPowerDown_mad Called.\n"));

    /* translate LPORT to hardware port */
    hwPort = GT_LPORT_2_PHY(port);

    gtSemTake(dev,dev->phyRegsSem,OS_WAIT_FOREVER);

    /* check if the port is configurable */
    if(!IS_CONFIGURABLE_PHY(dev,hwPort))
    {
        gtSemGive(dev,dev->phyRegsSem);
        return GT_NOT_SUPPORTED;
    }

    if (state==GT_TRUE)
      pwMode = MAD_TRUE;
    else
      pwMode = MAD_FALSE;

     if ((retVal=mdSysSetPhyEnable(&(dev->mad_dev), hwPort, pwMode))!=MAD_OK)
    {
        DBG_INFO(("mdSysSetPhyEnable Failed.\n"));
        gtSemGive(dev,dev->phyRegsSem);
        return GT_FAIL;
    }

    gtSemGive(dev,dev->phyRegsSem);
    return GT_OK;
}

/*******************************************************************************
* gprtPortRestartAutoNeg_mad
*
* DESCRIPTION:
*         Restart AutoNegotiation. If AutoNegotiation is not enabled, it'll enable
*        it. Loopback and Power Down will be disabled by this routine.
*
* INPUTS:
*        port -    The logical port number, unless SERDES device is accessed
*                The physical address, if SERDES device is accessed
*
* OUTPUTS:
*         None.
*
* RETURNS:
*         GT_OK     - on success
*         GT_FAIL     - on error
*
* COMMENTS:
*         data sheet register 0.9 - Restart Auto-Negotiation
*
*******************************************************************************/

GT_STATUS gprtPortRestartAutoNeg_mad
(
IN GT_QD_DEV *dev,
IN GT_LPORT  port
)
{
    GT_STATUS       retVal;
    GT_U8           hwPort;         /* the physical port number     */
    GT_U16          u16Data;
    MAD_U32         u32Data;

    DBG_INFO(("gprtPortRestartAutoNeg_mad Called.\n"));

    /* translate LPORT to hardware port */
    hwPort = GT_LPORT_2_PHY(port);

    gtSemTake(dev,dev->phyRegsSem,OS_WAIT_FOREVER);

    /* check if the port is configurable */
    if(!IS_CONFIGURABLE_PHY(dev,hwPort))
    {
        gtSemGive(dev,dev->phyRegsSem);
        return GT_NOT_SUPPORTED;
    }

    if((retVal=mdSysGetPhyReg(&(dev->mad_dev),hwPort,QD_PHY_CONTROL_REG,&u32Data)) != MAD_OK)
    {
        DBG_INFO(("Not able to read Phy Reg(port:%d,offset:%d).\n",hwPort,QD_PHY_CONTROL_REG));
        gtSemGive(dev,dev->phyRegsSem);
        return GT_FAIL;
    }

    u16Data = u32Data;
    u16Data &= (QD_PHY_DUPLEX | QD_PHY_SPEED);
    u16Data |= (QD_PHY_RESTART_AUTONEGO | QD_PHY_AUTONEGO);

    DBG_INFO(("Write to phy(%d) register: regAddr 0x%x, data %#x",
              hwPort,QD_PHY_CONTROL_REG,u16Data));

    /* Write to Phy Control Register.  */
    if((retVal=madHwPagedSetCtrlPara(&(dev->mad_dev),hwPort,0,u16Data)) != MAD_OK)
    {
        DBG_INFO(("CallmadHwPagedSetCtrlPara failed.\n"));
        gtSemGive(dev,dev->phyRegsSem);
        return GT_FAIL;
    }

    gtSemGive(dev,dev->phyRegsSem);

    if(retVal != MAD_OK)
    {
        DBG_INFO(("Failed.\n"));
        return GT_FAIL;
    }
    else
    {
        DBG_INFO(("OK.\n"));
    }
    return GT_OK;
}

/*******************************************************************************
* gprtSetPortDuplexMode_mad
*
* DESCRIPTION:
*         Sets duplex mode for a specific logical port. This function will keep
*        the speed and loopback mode to the previous value, but disable others,
*        such as Autonegotiation.
*
* INPUTS:
*        port -    The logical port number, unless SERDES device is accessed
*                The physical address, if SERDES device is accessed
*         dMode    - dulpex mode
*
* OUTPUTS:
*         None.
*
* RETURNS:
*         GT_OK     - on success
*         GT_FAIL     - on error
*
* COMMENTS:
*         data sheet register 0.8 - Duplex Mode
*
*******************************************************************************/
GT_STATUS gprtSetPortDuplexMode_mad
(
IN GT_QD_DEV *dev,
IN GT_LPORT  port,
IN GT_BOOL   dMode
)
{
    GT_U8           hwPort;         /* the physical port number     */
    GT_U16             u16Data;
    GT_STATUS        retVal;
    MAD_U32         u32Data;

    DBG_INFO(("gprtSetPortDuplexMode_mad Called.\n"));

    /* translate LPORT to hardware port */
    hwPort = GT_LPORT_2_PHY(port);

    gtSemTake(dev,dev->phyRegsSem,OS_WAIT_FOREVER);

    /* check if the port is configurable */
    if(!IS_CONFIGURABLE_PHY(dev,hwPort))
    {
        gtSemGive(dev,dev->phyRegsSem);
        return GT_NOT_SUPPORTED;
    }

    if((retVal=mdSysGetPhyReg(&(dev->mad_dev),hwPort,QD_PHY_CONTROL_REG,&u32Data)) != MAD_OK)
    {
        DBG_INFO(("Not able to read Phy Reg(port:%d,offset:%d).\n",hwPort,QD_PHY_CONTROL_REG));
        gtSemGive(dev,dev->phyRegsSem);
        return GT_FAIL;
    }

    u16Data = u32Data;

    if(dMode)
    {
        u16Data = (u16Data & (QD_PHY_LOOPBACK | QD_PHY_SPEED | QD_PHY_SPEED_MSB)) | QD_PHY_DUPLEX;
    }
    else
    {
        u16Data = u16Data & (QD_PHY_LOOPBACK | QD_PHY_SPEED | QD_PHY_SPEED_MSB);
    }


    DBG_INFO(("Write to phy(%d) register: regAddr 0x%x, data %#x",
              hwPort,QD_PHY_CONTROL_REG,u16Data));

    /* Write to Phy Control Register.  */
    if((retVal=madHwPagedSetCtrlPara(&(dev->mad_dev),hwPort,0,u16Data)) != MAD_OK)
    {
        DBG_INFO(("CallmadHwPagedSetCtrlPara failed.\n"));
        gtSemGive(dev,dev->phyRegsSem);
        return GT_FAIL;
    }

    gtSemGive(dev,dev->phyRegsSem);
    return GT_OK;
}


/*******************************************************************************
* gprtSetPortAutoMode_mad
*
* DESCRIPTION:
*         This routine sets up the port with given Auto Mode.
*        Supported mode is as follows:
*        - Auto for both speed and duplex.
*        - Auto for speed only and Full duplex.
*        - Auto for speed only and Half duplex.
*        - Auto for duplex only and speed 1000Mbps.
*        - Auto for duplex only and speed 100Mbps.
*        - Auto for duplex only and speed 10Mbps.
*        - Speed 1000Mbps and Full duplex.
*        - Speed 1000Mbps and Half duplex.
*        - Speed 100Mbps and Full duplex.
*        - Speed 100Mbps and Half duplex.
*        - Speed 10Mbps and Full duplex.
*        - Speed 10Mbps and Half duplex.
*
*
* INPUTS:
*        port -    The logical port number, unless SERDES device is accessed
*                The physical address, if SERDES device is accessed
*         mode - Auto Mode to be written
*
* OUTPUTS:
*        None.
*
* RETURNS:
*        GT_OK   - on success
*        GT_FAIL - on error
*        GT_NOT_SUPPORTED - on device without copper
*
* COMMENTS:
*         data sheet register 4.8, 4.7, 4.6, and 4.5 Autonegotiation Advertisement
*         data sheet register 4.6, 4.5 Autonegotiation Advertisement for 1000BX
*         data sheet register 9.9, 9.8 Autonegotiation Advertisement for 1000BT
*******************************************************************************/

GT_STATUS gprtSetPortAutoMode_mad
(
    IN GT_QD_DEV *dev,
    IN GT_LPORT  port,
    IN GT_PHY_AUTO_MODE mode
)
{

    GT_STATUS       retVal;         /* Functions return value.      */
    GT_U8           hwPort;         /* the physical port number     */
    GT_PHY_INFO        phyInfo;

    DBG_INFO(("gprtSetPortAutoMode_mad Called.\n"));

    /* translate LPORT to hardware port */
    hwPort = GT_LPORT_2_PHY(port);

    retVal = GT_NOT_SUPPORTED;

    gtSemTake(dev,dev->phyRegsSem,OS_WAIT_FOREVER);

    /* check if the port is configurable */
    if((phyInfo.phyId=GT_GET_PHY_ID(dev,hwPort)) == GT_INVALID_PHY)
    {
        gtSemGive(dev,dev->phyRegsSem);
        return GT_NOT_SUPPORTED;
    }

    if(driverFindPhyInformation(dev,hwPort,&phyInfo) != GT_OK)
    {
        DBG_INFO(("Unknown PHY device.\n"));
        gtSemGive(dev,dev->phyRegsSem);
        return GT_FAIL;
    }

    retVal = phySetAutoMode_mad(dev,hwPort,&phyInfo,mode);

    gtSemGive(dev,dev->phyRegsSem);
    return retVal;

}


/*******************************************************************************
* gprtSetPause_mad
*
* DESCRIPTION:
*       This routine will set the pause bit in Autonegotiation Advertisement
*        Register. And restart the autonegotiation.
*
* INPUTS:
*        port -    The logical port number, unless SERDES device is accessed
*                The physical address, if SERDES device is accessed
*        state - GT_PHY_PAUSE_MODE enum value.
*                GT_PHY_NO_PAUSE        - disable pause
*                 GT_PHY_PAUSE        - support pause
*                GT_PHY_ASYMMETRIC_PAUSE    - support asymmetric pause
*                GT_PHY_BOTH_PAUSE    - support both pause and asymmetric pause
*
* OUTPUTS:
*       None.
*
* RETURNS:
*       GT_OK   - on success
*       GT_FAIL - on error
* COMMENTS:
* data sheet register 4.10 Autonegotiation Advertisement Register
*******************************************************************************/

GT_STATUS gprtSetPause_mad
(
IN GT_QD_DEV *dev,
IN GT_LPORT  port,
IN GT_PHY_PAUSE_MODE state
)
{
    GT_U8           hwPort;         /* the physical port number     */
    GT_PHY_INFO        phyInfo;
    MAD_STATUS retVal;
    MAD_BOOL autoNegoEn;
    MAD_U32            autoMode

    DBG_INFO(("phySetPause_mad Called.\n"));

    /* translate LPORT to hardware port */
    hwPort = GT_LPORT_2_PHY(port);

    gtSemTake(dev,dev->phyRegsSem,OS_WAIT_FOREVER);

    /* check if the port is configurable */
    if((phyInfo.phyId=GT_GET_PHY_ID(dev,hwPort)) == GT_INVALID_PHY)
    {
        gtSemGive(dev,dev->phyRegsSem);
        return GT_NOT_SUPPORTED;
    }

    if(state & GT_PHY_ASYMMETRIC_PAUSE)
    {
        if(driverFindPhyInformation(dev,hwPort,&phyInfo) != GT_OK)
        {
            DBG_INFO(("Unknown PHY device.\n"));
            gtSemGive(dev,dev->phyRegsSem);
            return GT_FAIL;
        }

        if (!(phyInfo.flag & GT_PHY_GIGABIT))
        {
            DBG_INFO(("Not Supported\n"));
            gtSemGive(dev,dev->phyRegsSem);
            return GT_BAD_PARAM;
        }

    }

     if ((retVal=mdCopperGetAutoNeg(&(dev->mad_dev), hwPort, &autoNegoEn, &autoMode))!=MAD_OK)
    {
        DBG_INFO(("mdCopperSetAutoNeg Failed.\n"));
        gtSemGive(dev,dev->phyRegsSem);
        return GT_FAIL;
    }

    if (state==GT_TRUE)
      autoMode |= MAD_AUTO_AD_ASYM_PAUSE;
    else
      autoMode &= ~MAD_AUTO_AD_ASYM_PAUSE;


     if ((retVal=mdCopperSetAutoNeg(&(dev->mad_dev), hwPort, autoNegoEn, autoMode))!=MAD_OK)
    {
        DBG_INFO(("mdCopperSetAutoNeg Failed.\n"));
        gtSemGive(dev,dev->phyRegsSem);
        return GT_FAIL;
    }


    gtSemGive(dev,dev->phyRegsSem);
    return retVal;
}

/*******************************************************************************
* gprtSetDTEDetect_mad
*
* DESCRIPTION:
*       This routine enables/disables DTE.
*
* INPUTS:
*         port - The logical port number
*         mode - either GT_TRUE(for enable) or GT_FALSE(for disable)
*
* OUTPUTS:
*       None.
*
* RETURNS:
*       GT_OK   - on success
*       GT_FAIL - on error
*
* COMMENTS:
*
*******************************************************************************/

GT_STATUS gprtSetDTEDetect_mad
(
    IN GT_QD_DEV *dev,
    IN GT_LPORT  port,
    IN GT_BOOL   state
)
{
    GT_U8           hwPort;         /* the physical port number     */
    GT_STATUS        retVal = GT_OK;
    GT_PHY_INFO    phyInfo;
    MAD_BOOL            en;

    DBG_INFO(("phySetDTE_mad Called.\n"));

    /* translate LPORT to hardware port */
    hwPort = GT_LPORT_2_PHY(port);

    gtSemTake(dev,dev->phyRegsSem,OS_WAIT_FOREVER);

    /* check if the port is configurable */
    if((phyInfo.phyId=GT_GET_PHY_ID(dev,hwPort)) == GT_INVALID_PHY)
    {
        gtSemGive(dev,dev->phyRegsSem);
        return GT_NOT_SUPPORTED;
    }

    /* check if the port supports DTE */
    if(driverFindPhyInformation(dev,hwPort,&phyInfo) != GT_OK)
    {
        DBG_INFO(("Unknown PHY device.\n"));
        gtSemGive(dev,dev->phyRegsSem);
        return GT_FAIL;
    }

    if (!(phyInfo.flag & GT_PHY_DTE_CAPABLE))
    {
        DBG_INFO(("Not Supported\n"));
        gtSemGive(dev,dev->phyRegsSem);
        return GT_NOT_SUPPORTED;
    }

    if (state==GT_TRUE)
      en = MAD_TRUE;
    else
      en = MAD_FALSE;

    if(mdCopperSetDTEDetectEnable(&(dev->mad_dev),hwPort,en,0) != MAD_OK)
    {
        DBG_INFO(("Call mdCopperSetDTEDetectEnable failed.\n"));
        gtSemGive(dev,dev->phyRegsSem);
        return GT_FAIL;
    }

    gtSemGive(dev,dev->phyRegsSem);
    return retVal;
}


/*******************************************************************************
* gprtGetDTEDetectStatus_mad
*
* DESCRIPTION:
*       This routine gets DTE status.
*
* INPUTS:
*         port - The logical port number
*
* OUTPUTS:
*       status - GT_TRUE, if link partner needs DTE power.
*                 GT_FALSE, otherwise.
*
* RETURNS:
*       GT_OK   - on success
*       GT_FAIL - on error
*
* COMMENTS:
*
*******************************************************************************/

GT_STATUS gprtGetDTEDetectStatus_mad
(
    IN  GT_QD_DEV *dev,
    IN  GT_LPORT  port,
    OUT GT_BOOL   *state
)
{
    GT_U8           hwPort;         /* the physical port number     */
    GT_STATUS        retVal = GT_OK;
    GT_PHY_INFO    phyInfo;
    MAD_BOOL    en;
    MAD_U16     dropHys;

    DBG_INFO(("gprtGetDTEStatus_mad Called.\n"));

    /* translate LPORT to hardware port */
    hwPort = GT_LPORT_2_PHY(port);

    gtSemTake(dev,dev->phyRegsSem,OS_WAIT_FOREVER);

    /* check if the port is configurable */
    if((phyInfo.phyId=GT_GET_PHY_ID(dev,hwPort)) == GT_INVALID_PHY)
    {
        gtSemGive(dev,dev->phyRegsSem);
        return GT_NOT_SUPPORTED;
    }

    /* check if the port supports DTE */
    if(driverFindPhyInformation(dev,hwPort,&phyInfo) != GT_OK)
    {
        DBG_INFO(("Unknown PHY device.\n"));
        gtSemGive(dev,dev->phyRegsSem);
        return GT_FAIL;
    }

    if (!(phyInfo.flag & GT_PHY_DTE_CAPABLE))
    {
        DBG_INFO(("Not Supported\n"));
        gtSemGive(dev,dev->phyRegsSem);
        return GT_NOT_SUPPORTED;
    }

    if(mdCopperGetDTEDetectEnable(&(dev->mad_dev),hwPort,&en,&dropHys) != MAD_OK)
    {
        DBG_INFO(("Call mdCopperSetDTEDetectEnable failed.\n"));
        gtSemGive(dev,dev->phyRegsSem);
        return GT_FAIL;
    }


     if (en==MAD_TRUE)
      *state = GT_TRUE;
    else
      *state = GT_FALSE;


    gtSemGive(dev,dev->phyRegsSem);
    return retVal;
}


/*******************************************************************************
* gprtSetDTEDetectDropWait_mad
*
* DESCRIPTION:
*       Once the PHY no longer detects that the link partner filter, the PHY
*        will wait a period of time before clearing the power over Ethernet
*        detection status bit. The wait time is 5 seconds multiplied by the
*        given value.
*
* INPUTS:
*         port - The logical port number
*       waitTime - 0 ~ 15 (unit of 4 sec.)
*
* OUTPUTS:
*        None.
*
* RETURNS:
*       GT_OK   - on success
*       GT_FAIL - on error
*
* COMMENTS:
*
*******************************************************************************/

GT_STATUS gprtSetDTEDetectDropWait_mad
(
    IN  GT_QD_DEV *dev,
    IN  GT_LPORT  port,
    IN  GT_U16    waitTime
)
{
    GT_U8           hwPort;         /* the physical port number     */
    GT_STATUS        retVal = GT_OK;
    GT_PHY_INFO    phyInfo;
    MAD_BOOL    en;
    MAD_U16     dropHys;


    DBG_INFO(("gprtSetDTEDropWait_mad Called.\n"));

    /* translate LPORT to hardware port */
    hwPort = GT_LPORT_2_PHY(port);

    gtSemTake(dev,dev->phyRegsSem,OS_WAIT_FOREVER);

    /* check if the port is configurable */
    if((phyInfo.phyId=GT_GET_PHY_ID(dev,hwPort)) == GT_INVALID_PHY)
    {
        gtSemGive(dev,dev->phyRegsSem);
        return GT_NOT_SUPPORTED;
    }

    /* check if the port supports DTE */
    if(driverFindPhyInformation(dev,hwPort,&phyInfo) != GT_OK)
    {
        DBG_INFO(("Unknown PHY device.\n"));
        gtSemGive(dev,dev->phyRegsSem);
        return GT_FAIL;
    }

    if (!(phyInfo.flag & GT_PHY_DTE_CAPABLE))
    {
        DBG_INFO(("Not Supported\n"));
        gtSemGive(dev,dev->phyRegsSem);
        return GT_NOT_SUPPORTED;
    }

    if(mdCopperGetDTEDetectEnable(&(dev->mad_dev),hwPort,&en,&dropHys) != MAD_OK)
    {
        DBG_INFO(("Call mdCopperSetDTEDetectEnable failed.\n"));
        gtSemGive(dev,dev->phyRegsSem);
        return GT_FAIL;
    }

    dropHys = waitTime;
    if(mdCopperSetDTEDetectEnable(&(dev->mad_dev),hwPort,en,dropHys) != MAD_OK)
    {
        DBG_INFO(("Call mdCopperSetDTEDetectEnable failed.\n"));
        gtSemGive(dev,dev->phyRegsSem);
        return GT_FAIL;
    }

    gtSemGive(dev,dev->phyRegsSem);
    return retVal;
}


/*******************************************************************************
* gprtGetDTEDetectDropWait_mad
*
* DESCRIPTION:
*       Once the PHY no longer detects that the link partner filter, the PHY
*        will wait a period of time before clearing the power over Ethernet
*        detection status bit. The wait time is 5 seconds multiplied by the
*        returned value.
*
* INPUTS:
*         port - The logical port number
*
* OUTPUTS:
*       waitTime - 0 ~ 15 (unit of 4 sec.)
*
* RETURNS:
*       GT_OK   - on success
*       GT_FAIL - on error
*
* COMMENTS:
*
*******************************************************************************/

GT_STATUS gprtGetDTEDetectDropWait_mad
(
    IN  GT_QD_DEV *dev,
    IN  GT_LPORT  port,
    OUT GT_U16    *waitTime
)
{
    GT_U8           hwPort;         /* the physical port number     */
    GT_STATUS        retVal = GT_OK;
    GT_PHY_INFO    phyInfo;
    MAD_BOOL    en;
    MAD_U16     dropHys;

    DBG_INFO(("gprtSetDTEDropWait_mad Called.\n"));

    /* translate LPORT to hardware port */
    hwPort = GT_LPORT_2_PHY(port);

    gtSemTake(dev,dev->phyRegsSem,OS_WAIT_FOREVER);

    /* check if the port is configurable */
    if((phyInfo.phyId=GT_GET_PHY_ID(dev,hwPort)) == GT_INVALID_PHY)
    {
        gtSemGive(dev,dev->phyRegsSem);
        return GT_NOT_SUPPORTED;
    }

    if(driverFindPhyInformation(dev,hwPort,&phyInfo) != GT_OK)
    {
        DBG_INFO(("Unknown PHY device.\n"));
        gtSemGive(dev,dev->phyRegsSem);
        return GT_FAIL;
    }

    if (!(phyInfo.flag & GT_PHY_DTE_CAPABLE))
    {
        DBG_INFO(("Not Supported\n"));
        gtSemGive(dev,dev->phyRegsSem);
        return GT_NOT_SUPPORTED;
    }

    if(mdCopperGetDTEDetectEnable(&(dev->mad_dev),hwPort,&en,&dropHys) != MAD_OK)
    {
        DBG_INFO(("Call mdCopperSetDTEDetectEnable failed.\n"));
        gtSemGive(dev,dev->phyRegsSem);
        return GT_FAIL;
    }

    *waitTime = dropHys;

    gtSemGive(dev,dev->phyRegsSem);
    return retVal;
}


/*******************************************************************************
* gprtSetEnergyDetect_mad
*
* DESCRIPTION:
*       Energy Detect power down mode enables or disables the PHY to wake up on
*        its own by detecting activity on the CAT 5 cable.
*
* INPUTS:
*         port - The logical port number
*       mode - GT_EDETECT_MODE type
*
* OUTPUTS:
*        None.
*
* RETURNS:
*       GT_OK   - on success
*       GT_FAIL - on error
*        GT_BAD_PARAM - if invalid parameter is given
*       GT_NOT_SUPPORTED - if current device does not support this feature.
*
* COMMENTS:
*
*******************************************************************************/

GT_STATUS gprtSetEnergyDetect_mad
(
    IN  GT_QD_DEV *dev,
    IN  GT_LPORT  port,
    IN  GT_EDETECT_MODE   mode
)
{
    GT_U8           hwPort;         /* the physical port number     */
    GT_U16             u16Data;
    GT_STATUS        retVal = GT_OK;
    GT_PHY_INFO    phyInfo;

    DBG_INFO(("gprtSetEnergyDetect_mad Called.\n"));

    /* translate LPORT to hardware port */
    hwPort = GT_LPORT_2_PHY(port);

    gtSemTake(dev,dev->phyRegsSem,OS_WAIT_FOREVER);

    /* check if the port is configurable */
    if((phyInfo.phyId=GT_GET_PHY_ID(dev,hwPort)) == GT_INVALID_PHY)
    {
        gtSemGive(dev,dev->phyRegsSem);
        return GT_NOT_SUPPORTED;
    }

    if(driverFindPhyInformation(dev,hwPort,&phyInfo) != GT_OK)
    {
        DBG_INFO(("Unknown PHY device.\n"));
        gtSemGive(dev,dev->phyRegsSem);
        return GT_FAIL;
    }

    if (phyInfo.flag & GT_PHY_SERDES_CORE)
    {
        DBG_INFO(("Not Supported.\n"));
        gtSemGive(dev,dev->phyRegsSem);
        return GT_NOT_SUPPORTED;
    }
    else if (phyInfo.flag & GT_PHY_GIGABIT)
    {
        /* check if the mode is valid */
        switch (mode)
        {
            case GT_EDETECT_OFF:
                u16Data = 0;
                break;
            case GT_EDETECT_SENSE_PULSE:
                u16Data = 3;
                break;
            case GT_EDETECT_SENSE:
                u16Data = 2;
                break;
            default:
                DBG_INFO(("Invalid paramerter.\n"));
                gtSemGive(dev,dev->phyRegsSem);
                return GT_BAD_PARAM;
        }
    }
    else    /* it's a Fast Ethernet device */
    {
        /* check if the mode is valid */
        switch (mode)
        {
            case GT_EDETECT_OFF:
                u16Data = 0;
                break;
            case GT_EDETECT_SENSE_PULSE:
                u16Data = 1;
                break;
            case GT_EDETECT_SENSE:
            default:
                DBG_INFO(("Invalid paramerter.\n"));
                gtSemGive(dev,dev->phyRegsSem);
                return GT_BAD_PARAM;
        }

    }

    if(mdSysSetDetectPowerDownMode(&(dev->mad_dev),hwPort,u16Data) != MAD_OK)
    {
        DBG_INFO(("Call mdSysSetDetectPowerDownMode failed.\n"));
        gtSemGive(dev,dev->phyRegsSem);
        return GT_FAIL;
    }

    gtSemGive(dev,dev->phyRegsSem);
    return retVal;
}


/*******************************************************************************
* gprtGetEnergyDetect_mad
*
* DESCRIPTION:
*       Energy Detect power down mode enables or disables the PHY to wake up on
*        its own by detecting activity on the CAT 5 cable.
*
* INPUTS:
*         port - The logical port number
*
* OUTPUTS:
*       mode - GT_EDETECT_MODE type
*
* RETURNS:
*       GT_OK   - on success
*       GT_FAIL - on error
*       GT_NOT_SUPPORTED - if current device does not support this feature.
*
* COMMENTS:
*
*******************************************************************************/

GT_STATUS gprtGetEnergyDetect_mad
(
    IN  GT_QD_DEV *dev,
    IN  GT_LPORT  port,
    OUT GT_EDETECT_MODE   *mode
)
{
    GT_U8           hwPort;         /* the physical port number     */
    GT_U16             u16Data;
    GT_STATUS        retVal = GT_OK;
    GT_PHY_INFO    phyInfo;

    DBG_INFO(("gprtGetEnergyDetect_mad Called.\n"));

    /* translate LPORT to hardware port */
    hwPort = GT_LPORT_2_PHY(port);

    gtSemTake(dev,dev->phyRegsSem,OS_WAIT_FOREVER);

    /* check if the port is configurable */
    if((phyInfo.phyId=GT_GET_PHY_ID(dev,hwPort)) == GT_INVALID_PHY)
    {
        gtSemGive(dev,dev->phyRegsSem);
        return GT_NOT_SUPPORTED;
    }

    if(driverFindPhyInformation(dev,hwPort,&phyInfo) != GT_OK)
    {
        DBG_INFO(("Unknown PHY device.\n"));
        gtSemGive(dev,dev->phyRegsSem);
        return GT_FAIL;
    }

    if(mdSysGetDetectPowerDownMode(&(dev->mad_dev),hwPort,&u16Data) != MAD_OK)
    {
        DBG_INFO(("Call mdSysSetDetectPowerDownMode failed.\n"));
        gtSemGive(dev,dev->phyRegsSem);
        return GT_FAIL;
    }

    if (phyInfo.flag & GT_PHY_SERDES_CORE)
    {
        DBG_INFO(("Not Supported.\n"));
        gtSemGive(dev,dev->phyRegsSem);
        return GT_NOT_SUPPORTED;
    }
    else if (phyInfo.flag & GT_PHY_GIGABIT)
    {
        /* read the mode */

        switch (u16Data)
        {
            case 0:
            case 1:
                *mode = GT_EDETECT_OFF;
                break;
            case 2:
                *mode = GT_EDETECT_SENSE;
                break;
            case 3:
                *mode = GT_EDETECT_SENSE_PULSE;
                break;
            default:
                DBG_INFO(("Unknown value (should not happen).\n"));
                gtSemGive(dev,dev->phyRegsSem);
                return GT_FAIL;
        }

    }
    else    /* it's a Fast Ethernet device */
    {
        switch (u16Data)
        {
            case 0:
                *mode = GT_EDETECT_OFF;
                break;
            case 1:
                *mode = GT_EDETECT_SENSE_PULSE;
                break;
            default:
                DBG_INFO(("Unknown value (shouldn not happen).\n"));
                gtSemGive(dev,dev->phyRegsSem);
                return GT_FAIL;
        }

    }

    gtSemGive(dev,dev->phyRegsSem);
    return retVal;
}


/*******************************************************************************
* gprtSet1000TMasterMode_mad
*
* DESCRIPTION:
*       This routine sets the ports 1000Base-T Master mode and restart the Auto
*        negotiation.
*
* INPUTS:
*       port - the logical port number.
*       mode - GT_1000T_MASTER_SLAVE structure
*                autoConfig   - GT_TRUE for auto, GT_FALSE for manual setup.
*                masterPrefer - GT_TRUE if Master configuration is preferred.
*
* OUTPUTS:
*       None.
*
* RETURNS:
*       GT_OK   - on success
*       GT_FAIL - on error
*
* COMMENTS:
*
*
* GalTis:
*
*******************************************************************************/
GT_STATUS gprtSet1000TMasterMode_mad
(
    IN  GT_QD_DEV   *dev,
    IN  GT_LPORT     port,
    IN  GT_1000T_MASTER_SLAVE   *mode
)
{
    GT_U8            hwPort;         /* the physical port number     */
    GT_PHY_INFO    phyInfo;
    MAD_1000T_MASTER_SLAVE    msmode

    DBG_INFO(("gprtSet1000TMasterMode_mad Called.\n"));

    /* translate LPORT to hardware port */
    hwPort = GT_LPORT_2_PHY(port);

    if (!IS_IN_DEV_GROUP(dev,DEV_GIGABIT_SWITCH))
    {
        return GT_NOT_SUPPORTED;
    }

    gtSemTake(dev,dev->phyRegsSem,OS_WAIT_FOREVER);

    /* check if the port is configurable */
    if((phyInfo.phyId=GT_GET_PHY_ID(dev,hwPort)) == GT_INVALID_PHY)
    {
        gtSemGive(dev,dev->phyRegsSem);
        return GT_NOT_SUPPORTED;
    }

    if(driverFindPhyInformation(dev,hwPort,&phyInfo) != GT_OK)
    {
        DBG_INFO(("Unknown PHY device.\n"));
        gtSemGive(dev,dev->phyRegsSem);
        return GT_FAIL;
    }

    if (!(phyInfo.flag & GT_PHY_GIGABIT) || !(phyInfo.flag & GT_PHY_COPPER))
    {
        DBG_INFO(("Not Supported\n"));
        gtSemGive(dev,dev->phyRegsSem);
        return GT_NOT_SUPPORTED;
    }

    msmode.autoConfig = mode->autoConfig;
    msmode.masterPrefer = mode->masterPrefer;

    if(mdCopperSet1000TMasterMode(&(dev->mad_dev),hwPort,&msmode) != MAD_OK)
    {
        DBG_INFO(("Call mdSysSetDetectPowerDownMode failed.\n"));
        gtSemGive(dev,dev->phyRegsSem);
        return GT_FAIL;
    }

    gtSemGive(dev,dev->phyRegsSem);
    return GT_OK;
}


/*******************************************************************************
* gprtGet1000TMasterMode_mad
*
* DESCRIPTION:
*       This routine retrieves 1000Base-T Master Mode
*
* INPUTS:
*       port - the logical port number.
*
* OUTPUTS:
*       mode - GT_1000T_MASTER_SLAVE structure
*                autoConfig   - GT_TRUE for auto, GT_FALSE for manual setup.
*                masterPrefer - GT_TRUE if Master configuration is preferred.
*
* RETURNS:
*       GT_OK   - on success
*       GT_FAIL - on error
*
* COMMENTS:
*
*
* GalTis:
*
*******************************************************************************/
GT_STATUS gprtGet1000TMasterMode_mad
(
    IN  GT_QD_DEV   *dev,
    IN  GT_LPORT     port,
    OUT GT_1000T_MASTER_SLAVE   *mode
)
{
    GT_U8            hwPort;         /* the physical port number     */
    GT_PHY_INFO    phyInfo;
    MAD_1000T_MASTER_SLAVE    msmode

    DBG_INFO(("gprtGet1000TMasterMode_mad Called.\n"));

    /* translate LPORT to hardware port */
    hwPort = GT_LPORT_2_PHY(port);

    if (!IS_IN_DEV_GROUP(dev,DEV_GIGABIT_SWITCH))
    {
        return GT_NOT_SUPPORTED;
    }

    gtSemTake(dev,dev->phyRegsSem,OS_WAIT_FOREVER);

    /* check if the port is configurable */
    if((phyInfo.phyId=GT_GET_PHY_ID(dev,hwPort)) == GT_INVALID_PHY)
    {
        gtSemGive(dev,dev->phyRegsSem);
        return GT_NOT_SUPPORTED;
    }

    if(driverFindPhyInformation(dev,hwPort,&phyInfo) != GT_OK)
    {
        DBG_INFO(("Unknown PHY device.\n"));
        gtSemGive(dev,dev->phyRegsSem);
        return GT_FAIL;
    }

    if (!(phyInfo.flag & GT_PHY_GIGABIT) || !(phyInfo.flag & GT_PHY_COPPER))
    {
        DBG_INFO(("Not Supported\n"));
        gtSemGive(dev,dev->phyRegsSem);
        return GT_NOT_SUPPORTED;
    }

    if(mdCopperGet1000TMasterMode(&(dev->mad_dev),hwPort,&msmode) != MAD_OK)
    {
        DBG_INFO(("Call mdCopperGet1000TMasterMode failed.\n"));
        gtSemGive(dev,dev->phyRegsSem);
        return GT_FAIL;
    }
    mode->autoConfig = msmode.autoConfig;
    mode->masterPrefer = msmode.masterPrefer;


    gtSemGive(dev,dev->phyRegsSem);
    return GT_OK;
}

/*******************************************************************************
* gprtGetPhyLinkStatus_mad
*
* DESCRIPTION:
*       This routine retrieves the Link status.
*
* INPUTS:
*        port -    The logical port number, unless SERDES device is accessed
*                The physical address, if SERDES device is accessed
*
* OUTPUTS:
*       linkStatus - GT_FALSE if link is not established,
*                     GT_TRUE if link is established.
*
* RETURNS:
*       GT_OK   - on success
*       GT_FAIL - on error
*
*
* COMMENTS:
*
*******************************************************************************/
GT_STATUS gprtGetPhyLinkStatus_mad
(
    IN GT_QD_DEV *dev,
    IN GT_LPORT  port,
    IN GT_BOOL      *linkStatus
)
{

    GT_U8           hwPort;         /* the physical port number     */
    GT_PHY_INFO        phyInfo;
    OUT MAD_BOOL    linkOn;

    DBG_INFO(("gprtGetPhyLinkStatus_mad Called.\n"));

    /* translate LPORT to hardware port */
    hwPort = GT_LPORT_2_PHY(port);

    gtSemTake(dev,dev->phyRegsSem,OS_WAIT_FOREVER);

    /* check if the port is configurable */
    if((phyInfo.phyId=GT_GET_PHY_ID(dev,hwPort)) == GT_INVALID_PHY)
    {
        gtSemGive(dev,dev->phyRegsSem);
         return GT_NOT_SUPPORTED;
    }

    if(mdCopperGetLinkStatus(&(dev->mad_dev),hwPort,&linkOn) != MAD_OK)
    {
        DBG_INFO(("Call mdCopperGetLinkStatus failed.\n"));
        gtSemGive(dev,dev->phyRegsSem);
        return GT_FAIL;
    }

    *linkStatus = (linkOn==MAD_TRUE)?GT_TRUE:GT_FAIL;

    gtSemGive(dev,dev->phyRegsSem);
    return GT_OK;
}


/*******************************************************************************
* gprtSetPktGenEnable_mad
*
* DESCRIPTION:
*       This routine enables or disables Packet Generator.
*       Link should be established first prior to enabling the packet generator,
*       and generator will generate packets at the speed of the established link.
*        When enables packet generator, the following information should be
*       provided:
*           Payload Type:  either Random or 5AA55AA5
*           Packet Length: either 64 or 1514 bytes
*           Error Packet:  either Error packet or normal packet
*
* INPUTS:
*        port -    The logical port number, unless SERDES device is accessed
*                The physical address, if SERDES device is accessed
*       en      - GT_TRUE to enable, GT_FALSE to disable
*       pktInfo - packet information(GT_PG structure pointer), if en is GT_TRUE.
*                 ignored, if en is GT_FALSE
*
* OUTPUTS:
*       None.
*
* RETURNS:
*       GT_OK   - on success
*       GT_FAIL - on error
*
*
* COMMENTS:
*
*******************************************************************************/
GT_STATUS gprtSetPktGenEnable_mad
(
    IN GT_QD_DEV *dev,
    IN GT_LPORT  port,
    IN GT_BOOL   en,
    IN GT_PG     *pktInfo
)
{

    GT_U8           hwPort;         /* the physical port number     */
    GT_PHY_INFO        phyInfo;
    MAD_U32   men;
    MAD_PG    mpktInfo;

    DBG_INFO(("gprtSetPktGenEnable_mad Called.\n"));

    /* translate LPORT to hardware port */
    hwPort = GT_LPORT_2_PHY(port);

    gtSemTake(dev,dev->phyRegsSem,OS_WAIT_FOREVER);

    /* check if the port is configurable */
    if((phyInfo.phyId=GT_GET_PHY_ID(dev,hwPort)) == GT_INVALID_PHY)
    {
        gtSemGive(dev,dev->phyRegsSem);
        return GT_NOT_SUPPORTED;
    }

    if(driverFindPhyInformation(dev,hwPort,&phyInfo) != GT_OK)
    {
        DBG_INFO(("Unknown PHY device.\n"));
        gtSemGive(dev,dev->phyRegsSem);
        return GT_FAIL;
    }

    if(!(phyInfo.flag & GT_PHY_PKT_GENERATOR))
    {
        DBG_INFO(("Not Supported.\n"));
        gtSemGive(dev,dev->phyRegsSem);
        return GT_NOT_SUPPORTED;
    }

    mpktInfo.payload = pktInfo->payload;
    mpktInfo.length = pktInfo->length;
    mpktInfo.tx = pktInfo->tx;
    if (en==GT_TRUE)
    {
      men =1;
      mpktInfo.en_type = MAD_PG_EN_COPPER;
    }
    else
    {
      men =0;
      mpktInfo.en_type = MAD_PG_DISABLE;
    }

    if(mdDiagSetPktGenEnable(&(dev->mad_dev),hwPort, men, &mpktInfo) != MAD_OK)
    {
        DBG_INFO(("Call mdSysSetDetectPowerDownMode failed.\n"));
        gtSemGive(dev,dev->phyRegsSem);
        return GT_FAIL;
    }

    gtSemGive(dev,dev->phyRegsSem);
    return GT_OK;
}



/*******************************************************************************
* gprtGetPhyReg_mad
*
* DESCRIPTION:
*       This routine reads Phy Registers.
*
* INPUTS:
*        port -    The logical port number, unless SERDES device is accessed
*                The physical address, if SERDES device is accessed
*       regAddr - The register's address.
*
* OUTPUTS:
*       data    - The read register's data.
*
* RETURNS:
*       GT_OK           - on success
*       GT_FAIL         - on error
*
* COMMENTS:
*       None.
*
* GalTis:
*
*******************************************************************************/
GT_STATUS gprtGetPhyReg_mad
(
    IN  GT_QD_DEV    *dev,
    IN  GT_LPORT     port,
    IN  GT_U32         regAddr,
    OUT GT_U16         *data
)
{
    MAD_U32          u32Data;           /* The register's read data.    */
    GT_U8           hwPort;         /* the physical port number     */

    DBG_INFO(("gprtGetPhyReg_mad Called.\n"));

    hwPort = GT_LPORT_2_PHY(port);
    /* hwPort = port; */

    gtSemTake(dev,dev->phyRegsSem,OS_WAIT_FOREVER);

    /* Get Phy Register. */
    if(mdSysGetPhyReg(&(dev->mad_dev),hwPort,regAddr,&u32Data) != MAD_OK)
    {
        DBG_INFO(("Failed.\n"));
        gtSemGive(dev,dev->phyRegsSem);
        return GT_FAIL;
    }

    *data = u32Data;

    gtSemGive(dev,dev->phyRegsSem);
    return GT_OK;
}

/*******************************************************************************
* gprtSetPhyReg_mad
*
* DESCRIPTION:
*       This routine writes Phy Registers.
*
* INPUTS:
*        port -    The logical port number, unless SERDES device is accessed
*                The physical address, if SERDES device is accessed
*       regAddr - The register's address.
*
* OUTPUTS:
*       data    - The read register's data.
*
* RETURNS:
*       GT_OK           - on success
*       GT_FAIL         - on error
*
* COMMENTS:
*       None.
*
* GalTis:
*
*******************************************************************************/
GT_STATUS gprtSetPhyReg_mad
(
    IN  GT_QD_DEV        *dev,
    IN  GT_LPORT        port,
    IN  GT_U32            regAddr,
    IN  GT_U16            inData
)
{
    GT_U8           hwPort;         /* the physical port number     */
    MAD_U32        data = inData;

    DBG_INFO(("gprtSetPhyReg_mad Called.\n"));

    hwPort = GT_LPORT_2_PHY(port);
    /* hwPort = port;  */

    gtSemTake(dev,dev->phyRegsSem,OS_WAIT_FOREVER);

    /* Write to Phy Register */
    if(mdSysSetPhyReg(&(dev->mad_dev),hwPort,regAddr,data) != MAD_OK)
    {
        DBG_INFO(("Failed.\n"));
        gtSemGive(dev,dev->phyRegsSem);
        return GT_FAIL;
    }

    gtSemGive(dev,dev->phyRegsSem);
    return GT_OK;
}



/*******************************************************************************
* gprtGetPagedPhyReg_mad
*
* DESCRIPTION:
*       This routine reads phy register of the given page
*
* INPUTS:
*        port     - logical port to be read
*        regAddr    - register offset to be read
*        page    - page number to be read
*
* OUTPUTS:
*        data    - value of the read register
*
* RETURNS:
*       GT_OK               - if read successed
*       GT_FAIL               - if read failed
*
* COMMENTS:
*       None.
*
*******************************************************************************/
GT_STATUS gprtGetPagedPhyReg_mad
(
    IN  GT_QD_DEV *dev,
    IN  GT_U32  port,
    IN    GT_U32  regAddr,
    IN    GT_U32  page,
    OUT GT_U16* data
)
{
    GT_PHY_INFO        phyInfo;
    GT_U8            hwPort;
    MAD_U32        u32Data;

    hwPort = GT_LPORT_2_PHY(port);

    gtSemTake(dev,dev->phyRegsSem,OS_WAIT_FOREVER);

    /* check if the port is configurable */
    if((phyInfo.phyId=GT_GET_PHY_ID(dev,hwPort)) == GT_INVALID_PHY)
    {
        gtSemGive(dev,dev->phyRegsSem);
        return GT_NOT_SUPPORTED;
    }

    if(driverFindPhyInformation(dev,hwPort,&phyInfo) != GT_OK)
    {
        DBG_INFO(("Unknown PHY device.\n"));
        gtSemGive(dev,dev->phyRegsSem);
        return GT_FAIL;
    }

    if(mdSysGetPagedPhyReg(&(dev->mad_dev),hwPort,page, regAddr,&u32Data) != MAD_OK)
    {
        DBG_INFO(("Failed.\n"));
        gtSemGive(dev,dev->phyRegsSem);
        return GT_FAIL;
    }

    *data = u32Data;

    gtSemGive(dev,dev->phyRegsSem);
    return GT_OK;
}

/*******************************************************************************
* gprtSetPagedPhyReg_mad
*
* DESCRIPTION:
*       This routine writes a value to phy register of the given page
*
* INPUTS:
*        port     - logical port to be read
*        regAddr    - register offset to be read
*        page    - page number to be read
*        data    - value of the read register
*
* OUTPUTS:
*        None
*
* RETURNS:
*       GT_OK               - if read successed
*       GT_FAIL               - if read failed
*
* COMMENTS:
*       None.
*
*******************************************************************************/
GT_STATUS gprtSetPagedPhyReg_mad
(
    IN  GT_QD_DEV *dev,
    IN  GT_U32 port,
    IN    GT_U32 regAddr,
    IN    GT_U32 page,
    IN  GT_U16 inData
)
{
    GT_PHY_INFO        phyInfo;
    GT_U8            hwPort;
    MAD_U32 data = inData;

    hwPort = GT_LPORT_2_PHY(port);

    gtSemTake(dev,dev->phyRegsSem,OS_WAIT_FOREVER);

    /* check if the port is configurable */
    if((phyInfo.phyId=GT_GET_PHY_ID(dev,hwPort)) == GT_INVALID_PHY)
    {
        gtSemGive(dev,dev->phyRegsSem);
        return GT_NOT_SUPPORTED;
    }

    if(driverFindPhyInformation(dev,hwPort,&phyInfo) != GT_OK)
    {
        DBG_INFO(("Unknown PHY device.\n"));
        gtSemGive(dev,dev->phyRegsSem);
        return GT_FAIL;
    }

    if(mdSysSetPagedPhyReg(&(dev->mad_dev),hwPort,page, regAddr, data) != MAD_OK)
    {
        DBG_INFO(("Failed.\n"));
        gtSemGive(dev,dev->phyRegsSem);
        return GT_FAIL;
    }

    gtSemGive(dev,dev->phyRegsSem);
    return GT_OK;
}
