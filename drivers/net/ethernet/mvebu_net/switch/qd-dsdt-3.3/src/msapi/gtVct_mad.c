#include <Copyright.h>
/*******************************************************************************
* gtVct.c
*
* DESCRIPTION:
*       API for Marvell Virtual Cable Tester.
*
* DEPENDENCIES:
*       None.
*
* FILE REVISION NUMBER:
*       $Revision: 1 $
*******************************************************************************/
#include <msApi.h>
#include <gtVct.h>
#include <gtDrvConfig.h>
#include <gtDrvSwRegs.h>
#include <gtHwCntl.h>
#include <gtSem.h>

#include <madApi.h>


/*******************************************************************************
* gvctGetCableStatus_mad
*
* DESCRIPTION:
*       This routine perform the virtual cable test for the requested port,
*       and returns the the status per MDI pair.
*
* INPUTS:
*       port - logical port number.
*
* OUTPUTS:
*       cableStatus - the port copper cable status.
*       cableLen    - the port copper cable length.
*
* RETURNS:
*       GT_OK   - on success
*       GT_FAIL - on error
*
* COMMENTS:
*       Internal Gigabit Phys in 88E6165 family and 88E6351 family devices
*        are not supported by this API. For those devices, gvctGetAdvCableDiag
*        API can be used, instead.
*
*******************************************************************************/
GT_STATUS gvctGetCableDiag_mad
(
    IN  GT_QD_DEV *dev,
    IN  GT_LPORT        port,
    OUT GT_CABLE_STATUS *cableStatus
)
{
    GT_STATUS status=GT_OK;
    GT_U8 hwPort;
    GT_BOOL ppuEn;
    GT_PHY_INFO    phyInfo;

    DBG_INFO(("gvctGetCableDiag_mad Called.\n"));
    hwPort = GT_LPORT_2_PHY(port);

    gtSemTake(dev,dev->phyRegsSem,OS_WAIT_FOREVER);

    /* check if the port is configurable */
    if((phyInfo.phyId=GT_GET_PHY_ID(dev,hwPort)) == GT_INVALID_PHY)
    {
        gtSemGive(dev,dev->phyRegsSem);
        return GT_NOT_SUPPORTED;
    }

    /* check if the port supports VCT */
    if(driverFindPhyInformation(dev,hwPort,&phyInfo) != GT_OK)
    {
        DBG_INFO(("Unknown PHY device.\n"));
        gtSemGive(dev,dev->phyRegsSem);
        return GT_FAIL;
    }

    if (!(phyInfo.flag & GT_PHY_VCT_CAPABLE))
    {
        DBG_INFO(("Not Supported\n"));
        gtSemGive(dev,dev->phyRegsSem);
        return GT_NOT_SUPPORTED;
    }

    /* Need to disable PPUEn for safe. */
    if(gsysGetPPUEn(dev,&ppuEn) != GT_OK)
    {
        ppuEn = GT_FALSE;
    }

    if(ppuEn != GT_FALSE)
    {
        if((status= gsysSetPPUEn(dev,GT_FALSE)) != GT_OK)
        {
            DBG_INFO(("Not able to disable PPUEn.\n"));
            gtSemGive(dev,dev->phyRegsSem);
            return status;
        }
        gtDelay(250);
    }

    if ( mdDiagGetCableStatus(&(dev->mad_dev),port, (MAD_CABLE_STATUS*)cableStatus) != MAD_OK)
    {
      DBG_INFO(("Failed to run mdDiagGetCableStatus.\n"));
      gtSemGive(dev,dev->phyRegsSem);
      return GT_FALSE;
    }

    if(ppuEn != GT_FALSE)
    {
        if(gsysSetPPUEn(dev,ppuEn) != GT_OK)
        {
            DBG_INFO(("Not able to enable PPUEn.\n"));
            status = GT_FAIL;
        }
    }

    gtSemGive(dev,dev->phyRegsSem);
    return status;
}



/*******************************************************************************
* gvctGet1000BTExtendedStatus_mad
*
* DESCRIPTION:
*       This routine retrieves Pair Skew, Pair Swap, and Pair Polarity
*
* INPUTS:
*       dev - device context.
*       port - logical port number.
*
* OUTPUTS:
*       extendedStatus - extended cable status.
*
* RETURNS:
*       GT_OK   - on success
*       GT_FAIL - on error
*
* COMMENTS:
*       Internal Gigabit Phys in 88E6165 family and 88E6351 family devices
*        are not supported by this API. For those devices, gvctGetAdvExtendedStatus
*        API can be used, instead.
*
*******************************************************************************/
GT_STATUS gvctGet1000BTExtendedStatus_mad
(
    IN  GT_QD_DEV         *dev,
    IN  GT_LPORT        port,
    OUT GT_1000BT_EXTENDED_STATUS *extendedStatus
)
{
    GT_STATUS status=GT_OK;
    GT_U8 hwPort;
    GT_BOOL ppuEn;
    GT_PHY_INFO    phyInfo;

    DBG_INFO(("gvctGetCableDiag_mad Called.\n"));
    hwPort = GT_LPORT_2_PHY(port);

    gtSemTake(dev,dev->phyRegsSem,OS_WAIT_FOREVER);

    /* check if the port is configurable */
    if((phyInfo.phyId=GT_GET_PHY_ID(dev,hwPort)) == GT_INVALID_PHY)
    {
        gtSemGive(dev,dev->phyRegsSem);
        return GT_NOT_SUPPORTED;
    }

    /* check if the port supports VCT */
    if(driverFindPhyInformation(dev,hwPort,&phyInfo) != GT_OK)
    {
        DBG_INFO(("Unknown PHY device.\n"));
        gtSemGive(dev,dev->phyRegsSem);
        return GT_FAIL;
    }

    if (!(phyInfo.flag & GT_PHY_EX_CABLE_STATUS))
    {
        DBG_INFO(("Not Supported\n"));
        gtSemGive(dev,dev->phyRegsSem);
        return GT_NOT_SUPPORTED;
    }

    /* Need to disable PPUEn for safe. */
    if(gsysGetPPUEn(dev,&ppuEn) != GT_OK)
    {
        ppuEn = GT_FALSE;
    }

    if(ppuEn != GT_FALSE)
    {
        if((status= gsysSetPPUEn(dev,GT_FALSE)) != GT_OK)
        {
            DBG_INFO(("Not able to disable PPUEn.\n"));
            gtSemGive(dev,dev->phyRegsSem);
            return status;
        }
        gtDelay(250);
    }

    if ( mdDiagGet1000BTExtendedStatus(&(dev->mad_dev),port,(MAD_1000BT_EXTENDED_STATUS*)extendedStatus) != MAD_OK)
    {
      DBG_INFO(("Failed to run mdDiagGet1000BTExtendedStatus.\n"));
      gtSemGive(dev,dev->phyRegsSem);
      return GT_FALSE;
    }


    if(ppuEn != GT_FALSE)
    {
        if(gsysSetPPUEn(dev,ppuEn) != GT_OK)
        {
            DBG_INFO(("Not able to enable PPUEn.\n"));
            status = GT_FAIL;
        }
    }

    gtSemGive(dev,dev->phyRegsSem);
    return status;
}
