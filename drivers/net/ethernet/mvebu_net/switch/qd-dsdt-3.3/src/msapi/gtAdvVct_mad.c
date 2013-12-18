#include <Copyright.h>
/*******************************************************************************
* gtAdvVct.c
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
* gvctGetAdvCableStatus_mad
*
* DESCRIPTION:
*       This routine perform the advanced virtual cable test for the requested
*       port and returns the the status per MDI pair.
*
* INPUTS:
*       port - logical port number.
*       mode - advance VCT mode (either First Peak or Maximum Peak)
*
* OUTPUTS:
*       cableStatus - the port copper cable status.
*
* RETURNS:
*       GT_OK   - on success
*       GT_FAIL - on error
*
* COMMENTS:
*       Internal Gigabit Phys in 88E6165 family and 88E6351 family devices
*        are supporting this API.
*
*******************************************************************************/
GT_STATUS gvctGetAdvCableDiag_mad
(
    IN  GT_QD_DEV *dev,
    IN  GT_LPORT        port,
    IN  GT_ADV_VCT_MODE mode,
    OUT GT_ADV_CABLE_STATUS *cableStatus
)
{
    GT_STATUS status=GT_OK;
    GT_BOOL ppuEn;
    GT_U8 hwPort;
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

    if (!(phyInfo.flag & GT_PHY_ADV_VCT_CAPABLE))
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

    if ( mdDiagGetAdvCableStatus(&(dev->mad_dev),port,*((MAD_ADV_VCT_MODE *)&mode),(MAD_ADV_CABLE_STATUS*)cableStatus) != MAD_OK)
    {
      DBG_INFO(("Failed to run mdDiagGetAdvCableStatus.\n"));
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
* gvctGetAdvExtendedStatus_mad
*
* DESCRIPTION:
*       This routine retrieves extended cable status, such as Pair Poloarity,
*        Pair Swap, and Pair Skew. Note that this routine will be success only
*        if 1000Base-T Link is up.
*        Note: Since DSP based cable length in extended status is based on
*             constants from test results. At present, only E1181, E1111, and
*             E1112 are available.
*
* INPUTS:
*       dev  - pointer to GT driver structure returned from qdLoadDriver
*       port - logical port number.
*
* OUTPUTS:
*       extendedStatus - the extended cable status.
*
* RETURNS:
*       GT_OK   - on success
*       GT_FAIL - on error
*
* COMMENTS:
*        Supporting Device list:
*           88E1111, 88E1112, 88E1141~6, 88E1149, and Internal Gigabit Phys
*            in 88E6165 family and 88E6351 family devices
*
*******************************************************************************/
GT_STATUS gvctGetAdvExtendedStatus_mad
(
    IN  GT_QD_DEV     *dev,
    IN  GT_LPORT   port,
    OUT GT_ADV_EXTENDED_STATUS *extendedStatus
)
{
    GT_STATUS retVal=GT_OK;
    GT_U8 hwPort;
    GT_BOOL ppuEn;
    GT_PHY_INFO    phyInfo;

    DBG_INFO(("gvctGetAdvExtendedStatus_mad Called.\n"));
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
        if((retVal = gsysSetPPUEn(dev,GT_FALSE)) != GT_OK)
        {
            DBG_INFO(("Not able to disable PPUEn.\n"));
            gtSemGive(dev,dev->phyRegsSem);
            return retVal;
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
           return GT_FALSE;
        }
    }

        gtSemGive(dev,dev->phyRegsSem);
    return retVal;
}
