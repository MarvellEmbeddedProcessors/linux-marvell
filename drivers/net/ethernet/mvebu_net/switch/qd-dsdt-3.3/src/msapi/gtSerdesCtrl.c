#include <Copyright.h>

/********************************************************************************
* gtSerdesCtrl.h
*
* DESCRIPTION:
* API definitions for Phy Serdes control facility.
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


/*******************************************************************************
* gprtGetSerdesMode
*
* DESCRIPTION:
*       This routine reads Serdes Interface Mode.
*
* INPUTS:
*        port -  The physical SERDES device address(4/5)
*
* OUTPUTS:
*       mode    - Serdes Interface Mode
*
* RETURNS:
*       GT_OK           - on success
*       GT_FAIL         - on error
*
* COMMENTS:
*       logical port number is supported only for the devices made production
*       before 2009.
*  (Serdes devices: 88E6131, 88E6122, 88E6108, 88E6161, 88E6165 and 88E352 family)
*
*******************************************************************************/
GT_STATUS gprtGetSerdesMode
(
    IN  GT_QD_DEV    *dev,
    IN  GT_LPORT     port,
    IN  GT_SERDES_MODE *mode
)
{
    GT_U16          u16Data;           /* The register's read data.    */
    GT_U8           hwPort;         /* the physical port number     */


    DBG_INFO(("gprtGetSerdesMode Called.\n"));

    if(!IS_IN_DEV_GROUP(dev,DEV_SERDES_CORE))
    {
        return GT_NOT_SUPPORTED;
    }

    /* check if input is logical port number */
    hwPort = GT_LPORT_2_PORT(port);
    GT_GET_SERDES_PORT(dev,&hwPort);


    gtSemTake(dev,dev->phyRegsSem,OS_WAIT_FOREVER);

    /* Get Phy Register. */
    if(hwGetPhyRegField(dev,hwPort,16,0,2,&u16Data) != GT_OK)
    {
        DBG_INFO(("Failed.\n"));
        gtSemGive(dev,dev->phyRegsSem);
        return GT_FAIL;
    }

    *mode = u16Data;

    gtSemGive(dev,dev->phyRegsSem);
    return GT_OK;
}


/*******************************************************************************
* gprtSetSerdesMode
*
* DESCRIPTION:
*       This routine sets Serdes Interface Mode.
*
* INPUTS:
*       port -  The physical SERDES device address(4/5)
*       mode    - Serdes Interface Mode
*
* OUTPUTS:
*        None.
*
* RETURNS:
*       GT_OK           - on success
*       GT_FAIL         - on error
*
* COMMENTS:
*       logical port number is supported only for the devices made production
*       before 2009.
*  (Serdes devices: 88E6131, 88E6122, 88E6108, 88E6161, 88E6165 and 88E352 family)
*
*******************************************************************************/
GT_STATUS gprtSetSerdesMode
(
    IN  GT_QD_DEV    *dev,
    IN  GT_LPORT     port,
    IN  GT_SERDES_MODE mode
)
{
    GT_U16          u16Data;
    GT_U8           hwPort;         /* the physical port number     */
    GT_STATUS    retVal;

    DBG_INFO(("gprtSetSerdesMode Called.\n"));

    if(!IS_IN_DEV_GROUP(dev,DEV_SERDES_CORE))
    {
        return GT_NOT_SUPPORTED;
    }

    /* check if input is logical port number */
    hwPort = GT_LPORT_2_PORT(port);
    GT_GET_SERDES_PORT(dev,&hwPort);

    u16Data = mode;

    gtSemTake(dev,dev->phyRegsSem,OS_WAIT_FOREVER);

    /* Set Phy Register. */
    if(hwSetPhyRegField(dev,hwPort,16,0,2,u16Data) != GT_OK)
    {
        DBG_INFO(("Failed.\n"));
        gtSemGive(dev,dev->phyRegsSem);
        return GT_FAIL;
    }

    gtSemGive(dev,dev->phyRegsSem);
    retVal = hwPhyReset(dev,hwPort,0xFF);
    gtSemGive(dev,dev->phyRegsSem);
    return retVal;
}



#if 0
/*******************************************************************************
* gprtGetSerdesReg
*
* DESCRIPTION:
*       This routine reads Phy Serdes Registers.
*
* INPUTS:
*       port -    The logical port number.
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
GT_STATUS gprtGetSerdesReg
(
    IN  GT_QD_DEV    *dev,
    IN  GT_LPORT     port,
    IN  GT_U32         regAddr,
    OUT GT_U16         *data
)
{
    GT_U16          u16Data;           /* The register's read data.    */
    GT_U8           hwPort;         /* the physical port number     */
#ifdef GT_USE_MAD
    if (dev->use_mad==GT_TRUE)
        return gprtGetSerdesReg_mad(dev, port, regAddr, data);
#endif

    DBG_INFO(("gprtGetSerdesReg Called.\n"));

    if(!IS_IN_DEV_GROUP(dev,DEV_SERDES_CORE))
    {
        return GT_NOT_SUPPORTED;
    }

    /* check if input is logical port number */
    hwPort = GT_LPORT_2_PORT(port);
    GT_GET_SERDES_PORT(dev,&hwPort);

    if(hwPort > dev->maxPhyNum)
    {
        /* check if input is physical serdes address */
        if(dev->validSerdesVec & (1<<port))
        {
            hwPort = (GT_U8)port;
        }
        else
            return GT_NOT_SUPPORTED;
    }

    gtSemTake(dev,dev->phyRegsSem,OS_WAIT_FOREVER);

    /* Get Phy Register. */
    if(hwGetPhyRegField(dev,hwPort,16,0,2,&u16Data) != GT_OK)
    {
        DBG_INFO(("Failed.\n"));
        gtSemGive(dev,dev->phyRegsSem);
        return GT_FAIL;
    }

    *mode = u16Data;

    gtSemGive(dev,dev->phyRegsSem);
    return GT_OK;

}

/*******************************************************************************
* gprtSetSerdesReg
*
* DESCRIPTION:
*       This routine writes Phy Serdes Registers.
*
* INPUTS:
*       port -    The logical port number.
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
GT_STATUS gprtSetSerdesReg
(
    IN  GT_QD_DEV        *dev,
    IN  GT_LPORT        port,
    IN  GT_U32            regAddr,
    IN  GT_U16            data
)
{
    GT_U8           hwPort;         /* the physical port number     */

#ifdef GT_USE_MAD
    if (dev->use_mad==GT_TRUE)
        return gprtSetSerdesReg_mad(dev, port, regAddr, data);
#endif

    DBG_INFO(("gprtSetSerdesReg Called.\n"));

/*    hwPort = GT_LPORT_2_PHY(port); */
    hwPort = qdLong2Char(port);

    gtSemTake(dev,dev->phyRegsSem,OS_WAIT_FOREVER);

    /* Write to Phy Register */
    if(hwWritePhyReg(dev,hwPort,(GT_U8)regAddr,data) != GT_OK)
    {
        DBG_INFO(("Failed.\n"));
        gtSemGive(dev,dev->phyRegsSem);
        return GT_FAIL;
    }

    gtSemGive(dev,dev->phyRegsSem);
    return GT_OK;
}


#endif
