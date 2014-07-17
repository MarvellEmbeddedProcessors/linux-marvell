#include <Copyright.h>

/*******************************************************************************
* gtPIRL2.c
*
* DESCRIPTION:
*       API definitions for Port based PIRL Resources
*
* DEPENDENCIES:
*
* FILE REVISION NUMBER:
*******************************************************************************/

#include <msApi.h>
#include <gtSem.h>
#include <gtHwCntl.h>
#include <gtDrvSwRegs.h>

/* special ingress rate limit para */
static struct PIRL_PARA_TBL_T pirl2RateLimitParaTbl[] = {
	/* BI----BRF-----CBS-------EBS--------------------*/
	{0x000, 0x00, 0x000000, 0x000000},/*No rate limit*/
	{0x186, 0x03, 0xD06470, 0xFFFFF0},/*PIRL_RATE_64K*/
	{0x30D, 0x0D, 0x415370, 0xFFFFF0},/*PIRL_RATE_128K*/
	{0x186, 0x0A, 0x712D70, 0xFFFFF0},/*PIRL_RATE_192K*/
	{0x186, 0x0D, 0xA0C8F0, 0xFFFFF0},/*PIRL_RATE_256K*/
	{0x138, 0x0D, 0x4191F0, 0xFFFFF0},/*PIRL_RATE_320K*/
	{0x0C3, 0x0A, 0x712D70, 0xFFFFF0},/*PIRL_RATE_384K*/
	{0x0C3, 0x0C, 0x595FB0, 0xFFFFF0},/*PIRL_RATE_448K*/
	{0x0C3, 0x0D, 0x4191F0, 0xFFFFF0},/*PIRL_RATE_512K*/
	{0x0C3, 0x0F, 0x29C430, 0xFFFFF0},/*PIRL_RATE_576K*/
	{0x09C, 0x0D, 0x4191F0, 0xFFFFF0},/*PIRL_RATE_640K*/
	{0x07C, 0x0C, 0x597EF0, 0xFFFFF0},/*PIRL_RATE_704K*/
	{0x082, 0x0D, 0x71E8F0, 0xFFFFF0},/*PIRL_RATE_768K*/
	{0x078, 0x0D, 0x4191F0, 0xFFFFF0},/*PIRL_RATE_832K*/
	{0x084, 0x10, 0x1E69F0, 0xFFFFF0},/*PIRL_RATE_896K*/
	{0x027, 0x05, 0xB896B0, 0xFFFFF0},/*PIRL_RATE_960K*/
	{0x031, 0x07, 0xA28A28, 0xFFFFF0},/*PIRL_RATE_1M*/
	{0x031, 0x0D, 0x451460, 0xFFFFF0},/*PIRL_RATE_2M*/
	{0x021, 0x0D, 0x432C18, 0xFFFFF0},/*PIRL_RATE_3M*/
	{0x01C, 0x0F, 0x2A6070, 0xFFFFF0},/*PIRL_RATE_4M*/
	{0x065, 0x43, 0x029810, 0xFFFFF0},/*PIRL_RATE_5M*/
	{0x037, 0x2C, 0x0186A0, 0xFFFFF0},/*PIRL_RATE_6M*/
	{0x051, 0x4B, 0x0222E0, 0xFFFFF0},/*PIRL_RATE_7M*/
	{0x035, 0x38, 0x0186A0, 0xFFFFF0},/*PIRL_RATE_8M*/
	{0x03B, 0x46, 0x0186A0, 0xFFFFF0},/*PIRL_RATE_9M*/
	{0x030, 0x40, 0x015F90, 0xFFFFF0},/*PIRL_RATE_10M*/
	{0x033, 0x4B, 0x0186A0, 0xFFFFF0},/*PIRL_RATE_11M*/
	{0x033, 0x51, 0x015F90, 0xFFFFF0},/*PIRL_RATE_12M*/
	{0x02F, 0x51, 0x015F90, 0xFFFFF0},/*PIRL_RATE_13M*/
	{0x02D, 0x54, 0x013880, 0xFFFFF0},/*PIRL_RATE_14M*/
	{0x02A, 0x54, 0x013880, 0xFFFFF0},/*PIRL_RATE_15M*/
	{0x030, 0x66, 0x015F90, 0xFFFFF0},/*PIRL_RATE_16M*/
	{0x02F, 0x6A, 0x015F90, 0xFFFFF0},/*PIRL_RATE_17M*/
	{0x02A, 0x64, 0x013880, 0xFFFFF0},/*PIRL_RATE_18M*/
	{0x02D, 0x72, 0x013880, 0xFFFFF0},/*PIRL_RATE_19M*/
	{0x02C, 0x75, 0x013880, 0xFFFFF0},/*PIRL_RATE_20M*/
};


/****************************************************************************/
/* PIRL operation function declaration.                                    */
/****************************************************************************/
static GT_STATUS pirl2OperationPerform
(
    IN   GT_QD_DEV            *dev,
    IN   GT_PIRL2_OPERATION    pirlOp,
    INOUT GT_PIRL2_OP_DATA     *opData
);

static GT_STATUS pirl2Initialize
(
    IN  GT_QD_DEV              *dev
);

static GT_STATUS pirl2InitIRLResource
(
    IN  GT_QD_DEV              *dev,
    IN    GT_U32                irlPort,
    IN    GT_U32                irlRes
);

static GT_STATUS pirl2DisableIRLResource
(
    IN  GT_QD_DEV              *dev,
    IN    GT_U32                irlPort,
    IN    GT_U32                irlRes
);

static GT_STATUS pirl2DataToResource
(
    IN  GT_QD_DEV              *dev,
    IN  GT_PIRL2_DATA        *pirlData,
    OUT GT_PIRL2_RESOURCE    *res
);

static GT_STATUS pirl2ResourceToData
(
    IN  GT_QD_DEV              *dev,
    IN  GT_PIRL2_RESOURCE    *res,
    OUT GT_PIRL2_DATA        *pirlData
);

static GT_STATUS pirl2WriteResource
(
    IN  GT_QD_DEV              *dev,
    IN    GT_U32                irlPort,
    IN    GT_U32                irlRes,
    IN  GT_PIRL2_RESOURCE    *res
);

static GT_STATUS pirl2ReadResource
(
    IN  GT_QD_DEV              *dev,
    IN    GT_U32                irlPort,
    IN    GT_U32                irlRes,
    OUT GT_PIRL2_RESOURCE    *res
);

static GT_STATUS pirl2WriteTSMResource
(
    IN  GT_QD_DEV              *dev,
    IN    GT_U32                irlPort,
    IN    GT_U32                irlRes,
    IN  GT_PIRL2_TSM_RESOURCE    *res
);

static GT_STATUS pirl2ReadTSMResource
(
    IN  GT_QD_DEV              *dev,
    IN    GT_U32                irlPort,
    IN    GT_U32                irlRes,
    OUT GT_PIRL2_TSM_RESOURCE    *res
);

/*******************************************************************************
* gpirl2WriteResource
*
* DESCRIPTION:
*        This routine writes resource bucket parameters to the given resource
*        of the port.
*
* INPUTS:
*        port     - logical port number.
*        irlRes   - bucket to be used (0 ~ 4).
*        pirlData - PIRL resource parameters.
*
* OUTPUTS:
*        None.
*
* RETURNS:
*        GT_OK      - on success
*        GT_FAIL    - on error
*        GT_BAD_PARAM - if invalid parameter is given
*        GT_NOT_SUPPORTED - if current device does not support this feature.
*
* COMMENTS:
*        None
*
*******************************************************************************/
GT_STATUS gpirl2WriteResource
(
    IN  GT_QD_DEV     *dev,
    IN  GT_LPORT    port,
    IN  GT_U32        irlRes,
    IN  GT_PIRL2_DATA    *pirlData
)
{
    GT_STATUS           retVal;
    GT_PIRL2_RESOURCE    pirlRes;
    GT_U32               irlPort;         /* the physical port number     */
    GT_U32                maxRes;

    DBG_INFO(("gpirl2WriteResource Called.\n"));

    /* check if device supports this feature */
    if (!IS_IN_DEV_GROUP(dev,DEV_PIRL2_RESOURCE))
    {
        DBG_INFO(("GT_NOT_SUPPORTED\n"));
        return GT_NOT_SUPPORTED;
    }

    /* check if the given bucket number is valid */
    if (IS_IN_DEV_GROUP(dev,DEV_RESTRICTED_PIRL2_RESOURCE))
    {
        maxRes = 2;
    }
    else
    {
        maxRes = 5;
    }

    if (irlRes >= maxRes)
    {
        DBG_INFO(("GT_BAD_PARAM irlRes\n"));
        return GT_BAD_PARAM;
    }

    irlPort = (GT_U32)GT_LPORT_2_PORT(port);
    if (irlPort == GT_INVALID_PORT)
    {
        DBG_INFO(("GT_BAD_PARAM port\n"));
        return GT_BAD_PARAM;
    }

    /* Initialize internal counters */
    retVal = pirl2InitIRLResource(dev,irlPort,irlRes);
    if(retVal != GT_OK)
    {
        DBG_INFO(("PIRL Write Resource failed.\n"));
        return retVal;
    }

    /* Program the Ingress Rate Resource Parameters */
    retVal = pirl2DataToResource(dev,pirlData,&pirlRes);
    if(retVal != GT_OK)
    {
        DBG_INFO(("PIRL Data to PIRL Resource conversion failed.\n"));
        return retVal;
    }

    retVal = pirl2WriteResource(dev,irlPort,irlRes,&pirlRes);
    if(retVal != GT_OK)
    {
        DBG_INFO(("PIRL Write Resource failed.\n"));
        return retVal;
    }

    DBG_INFO(("OK.\n"));
    return GT_OK;

}

/*******************************************************************************
* gpirl2ReadResource
*
* DESCRIPTION:
*        This routine retrieves IRL Parameter.
*
* INPUTS:
*        port     - logical port number.
*        irlRes   - bucket to be used (0 ~ 4).
*
* OUTPUTS:
*        pirlData - PIRL resource parameters.
*
* RETURNS:
*        GT_OK      - on success
*        GT_FAIL    - on error
*        GT_BAD_PARAM - if invalid parameter is given
*        GT_NOT_SUPPORTED - if current device does not support this feature.
*
* COMMENTS:
*        None
*
*******************************************************************************/
GT_STATUS gpirl2ReadResource
(
    IN  GT_QD_DEV     *dev,
    IN  GT_LPORT    port,
    IN  GT_U32        irlRes,
    OUT GT_PIRL2_DATA    *pirlData
)
{
    GT_STATUS           retVal;
    GT_U32                irlPort;
    GT_PIRL2_RESOURCE    pirlRes;
    GT_U32                maxRes;

    DBG_INFO(("gpirl2ReadResource Called.\n"));

    /* check if device supports this feature */
    if (!IS_IN_DEV_GROUP(dev,DEV_PIRL2_RESOURCE))
    {
        DBG_INFO(("GT_NOT_SUPPORTED\n"));
        return GT_NOT_SUPPORTED;
    }

    /* check if the given bucket number is valid */
    if (IS_IN_DEV_GROUP(dev,DEV_RESTRICTED_PIRL2_RESOURCE))
    {
        maxRes = 2;
    }
    else
    {
        maxRes = 5;
    }

    if (irlRes >= maxRes)
    {
        DBG_INFO(("GT_BAD_PARAM irlRes\n"));
        return GT_BAD_PARAM;
    }

    irlPort = (GT_U32)GT_LPORT_2_PORT(port);
    if (irlPort == GT_INVALID_PORT)
    {
        DBG_INFO(("GT_BAD_PARAM port\n"));
        return GT_BAD_PARAM;
    }

    /* Read the Ingress Rate Resource Parameters */
    retVal = pirl2ReadResource(dev,irlPort,irlRes,&pirlRes);
    if(retVal != GT_OK)
    {
        DBG_INFO(("PIRL Read Resource failed.\n"));
        return retVal;
    }

    retVal = pirl2ResourceToData(dev,&pirlRes,pirlData);
    if(retVal != GT_OK)
    {
        DBG_INFO(("PIRL Resource to PIRL Data conversion failed.\n"));
        return retVal;
    }

    DBG_INFO(("OK.\n"));
    return GT_OK;

}


/*******************************************************************************
* gpirl2DisableResource
*
* DESCRIPTION:
*       This routine disables Ingress Rate Limiting for the given bucket.
*
* INPUTS:
*       port     - logical port number.
*        irlRes   - bucket to be used (0 ~ 4).
*
* OUTPUTS:
*       None.
*
* RETURNS:
*       GT_OK      - on success
*       GT_FAIL    - on error
*        GT_BAD_PARAM - if invalid parameter is given
*       GT_NOT_SUPPORTED - if current device does not support this feature.
*
* COMMENTS:
*       None
*
*******************************************************************************/
GT_STATUS gpirl2DisableResource
(
    IN  GT_QD_DEV     *dev,
    IN  GT_LPORT    port,
    IN  GT_U32        irlRes
)
{
    GT_STATUS           retVal;
    GT_U32                irlPort;
    GT_U32                maxRes;

    DBG_INFO(("gpirl2Dectivate Called.\n"));

    /* check if device supports this feature */
    if (!IS_IN_DEV_GROUP(dev,DEV_PIRL2_RESOURCE))
    {
        DBG_INFO(("GT_NOT_SUPPORTED\n"));
        return GT_NOT_SUPPORTED;
    }

    /* check if the given bucket number is valid */
    if (IS_IN_DEV_GROUP(dev,DEV_RESTRICTED_PIRL2_RESOURCE))
    {
        maxRes = 2;
    }
    else
    {
        maxRes = 5;
    }

    if (irlRes >= maxRes)
    {
        DBG_INFO(("GT_BAD_PARAM irlRes\n"));
        return GT_BAD_PARAM;
    }

    irlPort = (GT_U32)GT_LPORT_2_PORT(port);
    if (irlPort == GT_INVALID_PORT)
    {
        DBG_INFO(("GT_BAD_PARAM port\n"));
        return GT_BAD_PARAM;
    }

    /* disable irl resource */
    retVal = pirl2DisableIRLResource(dev, irlPort, irlRes);
    if(retVal != GT_OK)
    {
        DBG_INFO(("Getting Port State failed\n"));
        return retVal;
    }

    DBG_INFO(("OK.\n"));
    return GT_OK;
}

/*******************************************************************************
* gpirl2SetCurTimeUpInt
*
* DESCRIPTION:
*       This function sets the current time update interval.
*        Please contact FAE for detailed information.
*
* INPUTS:
*       upInt - updata interval (0 ~ 7)
*
* OUTPUTS:
*       None.
*
* RETURNS:
*       GT_OK      - on success
*       GT_FAIL    - on error
*        GT_BAD_PARAM - if invalid parameter is given
*       GT_NOT_SUPPORTED - if current device does not support this feature.
*
* COMMENTS:
*
*******************************************************************************/
GT_STATUS gpirl2SetCurTimeUpInt
(
    IN  GT_QD_DEV              *dev,
    IN    GT_U32                upInt
)
{
    GT_STATUS       retVal;        /* Functions return value */
    GT_PIRL2_OPERATION    op;
    GT_PIRL2_OP_DATA    opData;

    /* check if device supports this feature */
    if (!IS_IN_DEV_GROUP(dev,DEV_PIRL2_RESOURCE))
    {
        DBG_INFO(("GT_NOT_SUPPORTED\n"));
        return GT_NOT_SUPPORTED;
    }

    if (upInt > 0x7)
        return GT_BAD_PARAM;

    op = PIRL_READ_RESOURCE;

    opData.irlPort = 0xF;
    opData.irlRes = 0;
    opData.irlReg = 1;
    opData.irlData = 0;

    retVal = pirl2OperationPerform(dev, op, &opData);
    if (retVal != GT_OK)
    {
           DBG_INFO(("PIRL OP Failed.\n"));
           return retVal;
    }

    op = PIRL_WRITE_RESOURCE;
    opData.irlData = (opData.irlData & 0xFFF8) | (GT_U16)upInt;

    retVal = pirl2OperationPerform(dev, op, &opData);
    if (retVal != GT_OK)
    {
           DBG_INFO(("PIRL OP Failed.\n"));
           return retVal;
    }

    return GT_OK;
}


/*******************************************************************************
* gpirl2WriteTSMResource
*
* DESCRIPTION:
*        This routine writes rate resource bucket parameters in Time Slot Metering
*        mode to the given resource of the port.
*
* INPUTS:
*        port     - logical port number.
*        irlRes   - bucket to be used (0 ~ 1).
*        pirlData - PIRL TSM resource parameters.
*
* OUTPUTS:
*        None.
*
* RETURNS:
*        GT_OK      - on success
*        GT_FAIL    - on error
*        GT_BAD_PARAM - if invalid parameter is given
*        GT_NOT_SUPPORTED - if current device does not support this feature.
*
* COMMENTS:
*        Only Resource 0 and 1 can be supported for TSM Mode.
*
*******************************************************************************/
GT_STATUS gpirl2WriteTSMResource
(
    IN  GT_QD_DEV     *dev,
    IN  GT_LPORT    port,
    IN  GT_U32        irlRes,
    IN  GT_PIRL2_TSM_DATA    *pirlData
)
{
    GT_STATUS           retVal;
    GT_PIRL2_TSM_RESOURCE    pirlRes;
    GT_U32               irlPort;         /* the physical port number     */
    GT_U32                maxRes;
    GT_U32                cbs, cts, i, rate;

    DBG_INFO(("gpirl2WriteTSMResource Called.\n"));

    /* check if device supports this feature */
    if (!IS_IN_DEV_GROUP(dev,DEV_TSM_RESOURCE))
    {
        DBG_INFO(("GT_NOT_SUPPORTED\n"));
        return GT_NOT_SUPPORTED;
    }

    /* check if the given bucket number is valid */
    maxRes = 2;

    if (irlRes >= maxRes)
    {
        DBG_INFO(("GT_BAD_PARAM irlRes\n"));
        return GT_BAD_PARAM;
    }

    irlPort = (GT_U32)GT_LPORT_2_PORT(port);
    if (irlPort == GT_INVALID_PORT)
    {
        DBG_INFO(("GT_BAD_PARAM port\n"));
        return GT_BAD_PARAM;
    }

    /* Initialize internal counters */
    retVal = pirl2InitIRLResource(dev,irlPort,irlRes);
    if(retVal != GT_OK)
    {
        DBG_INFO(("PIRL Write Resource failed.\n"));
        return retVal;
    }

    if (pirlData->customSetup.isValid == GT_TRUE)
    {
        pirlRes.cbsLimit = pirlData->customSetup.cbsLimit;
        pirlRes.ctsIntv = pirlData->customSetup.ctsIntv;
        pirlRes.ebsLimit = pirlData->customSetup.ebsLimit;
        pirlRes.actionMode = pirlData->customSetup.actionMode;
    }
    else
    {
        /* convert ingressRate to cbsLimit and ctsIntv */
        cts = 1;
        cbs = 0;
        i = 3;
        rate = pirlData->ingressRate;
        while(cts < 16)
        {
            cbs = TSM_GET_CBS(rate, cts);
            if ((cbs == 0) || (cbs <= 0xFFFF))
                break;
            cts += i;
            i = cts;
        }

        if (cts > 16)
        {
            return GT_BAD_PARAM;
        }

        switch (cts)
        {
            case 1:
                pirlRes.ctsIntv = 3;
                break;
            case 4:
                pirlRes.ctsIntv = 2;
                break;
            case 8:
                pirlRes.ctsIntv = 1;
                break;
            case 16:
                pirlRes.ctsIntv = 0;
                break;
            default:
                return GT_FAIL;
        }

        pirlRes.cbsLimit = cbs;
        pirlRes.ebsLimit = 0xFFFF;
        pirlRes.actionMode = 1;
    }

    pirlRes.mgmtNrlEn = pirlData->mgmtNrlEn;
    pirlRes.priMask = pirlData->priMask;
    pirlRes.tsmMode = GT_TRUE;

    if (pirlData->tsmMode == GT_FALSE)
    {
        pirlRes.tsmMode = 0;
        pirlRes.cbsLimit = 0;
        pirlRes.ctsIntv = 0;
        pirlRes.ebsLimit = 0;
        pirlRes.actionMode = 0;
        pirlRes.mgmtNrlEn = 0;
        pirlRes.priMask = 0;
    }

    retVal = pirl2WriteTSMResource(dev,irlPort,irlRes,&pirlRes);
    if(retVal != GT_OK)
    {
        DBG_INFO(("PIRL Write Resource failed.\n"));
        return retVal;
    }

    DBG_INFO(("OK.\n"));
    return GT_OK;

}

/*******************************************************************************
* gpirl2ReadTSMResource
*
* DESCRIPTION:
*        This routine retrieves IRL Parameter.
*        Returned ingressRate would be rough number. Instead, customSetup will
*        have the exact configured value.
*
* INPUTS:
*        port     - logical port number.
*        irlRes   - bucket to be used (0 ~ 1).
*
* OUTPUTS:
*        pirlData - PIRL resource parameters.
*
* RETURNS:
*        GT_OK      - on success
*        GT_FAIL    - on error
*        GT_BAD_PARAM - if invalid parameter is given
*        GT_NOT_SUPPORTED - if current device does not support this feature.
*
* COMMENTS:
*        Only Resource 0 and 1 can be supported for TSM Mode.
*
*******************************************************************************/
GT_STATUS gpirl2ReadTSMResource
(
    IN  GT_QD_DEV     *dev,
    IN  GT_LPORT    port,
    IN  GT_U32        irlRes,
    OUT GT_PIRL2_TSM_DATA    *pirlData
)
{
    GT_STATUS           retVal;
    GT_U32                irlPort;
    GT_PIRL2_TSM_RESOURCE    pirlRes;
    GT_U32                maxRes, cbs, cts;

    DBG_INFO(("gpirl2ReadTSMResource Called.\n"));

    /* check if device supports this feature */
    if (!IS_IN_DEV_GROUP(dev,DEV_TSM_RESOURCE))
    {
        DBG_INFO(("GT_NOT_SUPPORTED\n"));
        return GT_NOT_SUPPORTED;
    }

    /* check if the given bucket number is valid */
    maxRes = 2;

    if (irlRes >= maxRes)
    {
        DBG_INFO(("GT_BAD_PARAM irlRes\n"));
        return GT_BAD_PARAM;
    }

    irlPort = (GT_U32)GT_LPORT_2_PORT(port);
    if (irlPort == GT_INVALID_PORT)
    {
        DBG_INFO(("GT_BAD_PARAM port\n"));
        return GT_BAD_PARAM;
    }

    /* Read the Ingress Rate Resource Parameters */
    retVal = pirl2ReadTSMResource(dev,irlPort,irlRes,&pirlRes);
    if(retVal != GT_OK)
    {
        DBG_INFO(("PIRL Read Resource failed.\n"));
        return retVal;
    }

    if (pirlRes.tsmMode == 0)
    {
        /* TMS Mode is not enabled */
        pirlData->tsmMode = GT_FALSE;
        pirlData->ingressRate = 0;
        pirlData->mgmtNrlEn = 0;
        pirlData->priMask = 0;
        pirlData->customSetup.isValid = 0;
        pirlData->customSetup.cbsLimit = 0;
        pirlData->customSetup.ctsIntv = 0;
        pirlData->customSetup.ebsLimit = 0;
        pirlData->customSetup.actionMode = 0;
        return GT_OK;
    }

    cbs = pirlRes.cbsLimit;
    switch (pirlRes.ctsIntv)
    {
        case 0:
            cts = 16;
            break;
        case 1:
            cts = 8;
            break;
        case 2:
            cts = 4;
            break;
        case 3:
            cts = 1;
            break;
        default:
            return GT_FAIL;
    }

    pirlData->ingressRate = TSM_GET_RATE(cbs,cts);

    pirlData->mgmtNrlEn = pirlRes.mgmtNrlEn;
    pirlData->priMask = pirlRes.priMask;

    pirlData->customSetup.isValid = GT_TRUE;
    pirlData->customSetup.cbsLimit = pirlRes.cbsLimit;
    pirlData->customSetup.ctsIntv = pirlRes.ctsIntv;
    pirlData->customSetup.ebsLimit = pirlRes.ebsLimit;
    pirlData->customSetup.actionMode = pirlRes.actionMode;

    DBG_INFO(("OK.\n"));
    return GT_OK;

}



/****************************************************************************/
/* Internal functions.                                                  */
/****************************************************************************/

/*******************************************************************************
* gpirl2Initialize
*
* DESCRIPTION:
*       This routine initializes PIRL Resources.
*
* INPUTS:
*       None
*
* OUTPUTS:
*       None
*
* RETURNS:
*       None
*
* COMMENTS:
*       None
*
*******************************************************************************/
GT_STATUS gpirl2Initialize
(
    IN  GT_QD_DEV              *dev
)
{
    GT_STATUS           retVal;

    /* check if device supports this feature */
    if (!IS_IN_DEV_GROUP(dev,DEV_PIRL2_RESOURCE))
    {
        DBG_INFO(("GT_NOT_SUPPORTED\n"));
        return GT_NOT_SUPPORTED;
    }

    retVal = pirl2Initialize(dev);
    if(retVal != GT_OK)
    {
        DBG_INFO(("Failed.\n"));
        return retVal;
    }

    return GT_OK;
}


/*******************************************************************************
* pirl2OperationPerform
*
* DESCRIPTION:
*       This function accesses Ingress Rate Command Register and Data Register.
*
* INPUTS:
*       pirlOp     - The stats operation bits to be written into the stats
*                    operation register.
*
* OUTPUTS:
*       pirlData   - points to the data storage where the MIB counter will be saved.
*
* RETURNS:
*       GT_OK on success,
*       GT_FAIL otherwise.
*
* COMMENTS:
*
*******************************************************************************/
static GT_STATUS pirl2OperationPerform
(
    IN    GT_QD_DEV             *dev,
    IN    GT_PIRL2_OPERATION    pirlOp,
    INOUT GT_PIRL2_OP_DATA        *opData
)
{
    GT_STATUS       retVal;    /* Functions return value */
    GT_U16          data;     /* temporary Data storage */

    gtSemTake(dev,dev->pirlRegsSem,OS_WAIT_FOREVER);

    /* Wait until the pirl in ready. */
#ifdef GT_RMGMT_ACCESS
    {
      HW_DEV_REG_ACCESS regAccess;

      regAccess.entries = 1;

      regAccess.rw_reg_list[0].cmd = HW_REG_WAIT_TILL_0;
      regAccess.rw_reg_list[0].addr = CALC_SMI_DEV_ADDR(dev, 0, GLOBAL2_REG_ACCESS);
      regAccess.rw_reg_list[0].reg = QD_REG_INGRESS_RATE_COMMAND;
      regAccess.rw_reg_list[0].data = 15;
      retVal = hwAccessMultiRegs(dev, &regAccess);
      if(retVal != GT_OK)
      {
        gtSemGive(dev,dev->pirlRegsSem);
        return retVal;
      }
    }
#else
    data = 1;
    while(data == 1)
    {
        retVal = hwGetGlobal2RegField(dev,QD_REG_INGRESS_RATE_COMMAND,15,1,&data);
        if(retVal != GT_OK)
        {
            gtSemGive(dev,dev->pirlRegsSem);
            return retVal;
        }
    }
#endif

    /* Set the PIRL Operation register */
    switch (pirlOp)
    {
        case PIRL_INIT_ALL_RESOURCE:
            data = (1 << 15) | (PIRL_INIT_ALL_RESOURCE << 12);
            retVal = hwWriteGlobal2Reg(dev,QD_REG_INGRESS_RATE_COMMAND,data);
            if(retVal != GT_OK)
            {
                gtSemGive(dev,dev->pirlRegsSem);
                return retVal;
            }
            break;
        case PIRL_INIT_RESOURCE:
            data = (GT_U16)((1 << 15) | (PIRL_INIT_RESOURCE << 12) |
                    (opData->irlPort << 8) |
                    (opData->irlRes << 5));
            retVal = hwWriteGlobal2Reg(dev,QD_REG_INGRESS_RATE_COMMAND,data);
            if(retVal != GT_OK)
            {
                gtSemGive(dev,dev->pirlRegsSem);
                return retVal;
            }
            break;

        case PIRL_WRITE_RESOURCE:
            data = (GT_U16)opData->irlData;
            retVal = hwWriteGlobal2Reg(dev,QD_REG_INGRESS_RATE_DATA,data);
            if(retVal != GT_OK)
            {
                gtSemGive(dev,dev->pirlRegsSem);
                return retVal;
            }

            data = (GT_U16)((1 << 15) | (PIRL_WRITE_RESOURCE << 12) |
                    (opData->irlPort << 8)    |
                    (opData->irlRes << 5)    |
                    (opData->irlReg & 0xF));
            retVal = hwWriteGlobal2Reg(dev,QD_REG_INGRESS_RATE_COMMAND,data);
            if(retVal != GT_OK)
            {
                gtSemGive(dev,dev->pirlRegsSem);
                return retVal;
            }
            break;

        case PIRL_READ_RESOURCE:
            data = (GT_U16)((1 << 15) | (PIRL_READ_RESOURCE << 12) |
                    (opData->irlPort << 8)    |
                    (opData->irlRes << 5)    |
                    (opData->irlReg & 0xF));
            retVal = hwWriteGlobal2Reg(dev,QD_REG_INGRESS_RATE_COMMAND,data);
            if(retVal != GT_OK)
            {
                gtSemGive(dev,dev->pirlRegsSem);
                return retVal;
            }

#ifdef GT_RMGMT_ACCESS
            {
              HW_DEV_REG_ACCESS regAccess;

              regAccess.entries = 1;

              regAccess.rw_reg_list[0].cmd = HW_REG_WAIT_TILL_0;
              regAccess.rw_reg_list[0].addr = CALC_SMI_DEV_ADDR(dev, 0, GLOBAL2_REG_ACCESS);
              regAccess.rw_reg_list[0].reg = QD_REG_INGRESS_RATE_COMMAND;
              regAccess.rw_reg_list[0].data = 15;
              retVal = hwAccessMultiRegs(dev, &regAccess);
              if(retVal != GT_OK)
              {
                gtSemGive(dev,dev->pirlRegsSem);
                return retVal;
              }
            }
#else
            data = 1;
            while(data == 1)
            {
                retVal = hwGetGlobal2RegField(dev,QD_REG_INGRESS_RATE_COMMAND,15,1,&data);
                if(retVal != GT_OK)
                {
                    gtSemGive(dev,dev->pirlRegsSem);
                    return retVal;
                }
            }
#endif

            retVal = hwReadGlobal2Reg(dev,QD_REG_INGRESS_RATE_DATA,&data);
            opData->irlData = (GT_U32)data;
            if(retVal != GT_OK)
            {
                gtSemGive(dev,dev->pirlRegsSem);
                return retVal;
            }
            gtSemGive(dev,dev->pirlRegsSem);
            return retVal;

        default:

            gtSemGive(dev,dev->pirlRegsSem);
            return GT_FAIL;
    }

    /* Wait until the pirl in ready. */
#ifdef GT_RMGMT_ACCESS
    {
      HW_DEV_REG_ACCESS regAccess;
      regAccess.entries = 1;

      regAccess.rw_reg_list[0].cmd = HW_REG_WAIT_TILL_0;
      regAccess.rw_reg_list[0].addr = CALC_SMI_DEV_ADDR(dev, 0, GLOBAL2_REG_ACCESS);
      regAccess.rw_reg_list[0].reg = QD_REG_INGRESS_RATE_COMMAND;
      regAccess.rw_reg_list[0].data = 15;
      retVal = hwAccessMultiRegs(dev, &regAccess);
      if(retVal != GT_OK)
      {
        gtSemGive(dev,dev->pirlRegsSem);
        return retVal;
      }
    }
#else
    data = 1;
    while(data == 1)
    {
        retVal = hwGetGlobal2RegField(dev,QD_REG_INGRESS_RATE_COMMAND,15,1,&data);
        if(retVal != GT_OK)
        {
            gtSemGive(dev,dev->pirlRegsSem);
            return retVal;
        }
    }
#endif

    gtSemGive(dev,dev->pirlRegsSem);
    return retVal;
}

/*
 * Initialize all PIRL resources to the inital state.
*/
static GT_STATUS pirl2Initialize
(
    IN  GT_QD_DEV              *dev
)
{
    GT_STATUS       retVal;    /* Functions return value */
    GT_PIRL2_OPERATION    op;

    op = PIRL_INIT_ALL_RESOURCE;


    retVal = pirl2OperationPerform(dev, op, NULL);
    if (retVal != GT_OK)
    {
        DBG_INFO(("PIRL OP Failed.\n"));
        return retVal;
    }


    retVal = gpirl2SetCurTimeUpInt(dev,4);
    if (retVal != GT_OK)
    {
        DBG_INFO(("PIRL OP Failed.\n"));
    }

    return retVal;
}

/*
 * Initialize the selected PIRL resource to the inital state.
 * This function initializes only the BSM structure for the IRL Unit.
*/
static GT_STATUS pirl2InitIRLResource
(
    IN  GT_QD_DEV              *dev,
    IN    GT_U32                irlPort,
    IN    GT_U32                irlRes
)
{
    GT_STATUS       retVal;    /* Functions return value */
    GT_PIRL2_OPERATION    op;
    GT_PIRL2_OP_DATA     opData;

    op = PIRL_INIT_RESOURCE;
    opData.irlPort = irlPort;
    opData.irlRes = irlRes;

    retVal = pirl2OperationPerform(dev, op, &opData);
    if (retVal != GT_OK)
    {
        DBG_INFO(("PIRL OP Failed.\n"));
        return retVal;
    }

    return retVal;
}

/*
 * Disable the selected PIRL resource.
*/
static GT_STATUS pirl2DisableIRLResource
(
    IN  GT_QD_DEV              *dev,
    IN    GT_U32                irlPort,
    IN    GT_U32                irlRes
)
{
    GT_STATUS       retVal;            /* Functions return value */
    GT_PIRL2_OPERATION    op;
    GT_PIRL2_OP_DATA    opData;
    int                i;

    op = PIRL_WRITE_RESOURCE;

    for(i=0; i<8; i++)
    {
        opData.irlPort = irlPort;
        opData.irlRes = irlRes;
        opData.irlReg = i;
        opData.irlData = 0;

        retVal = pirl2OperationPerform(dev, op, &opData);
        if (retVal != GT_OK)
        {
            DBG_INFO(("PIRL OP Failed.\n"));
            return retVal;
        }
    }

    return GT_OK;
}

/*find the greatest common divisor for two interger
*used to calculate BRF and BI
*/
static GT_U32 pirl2GetGCD(IN GT_U32 data1,
		IN GT_U32 data2)
{
	GT_U32 temp1, temp2, temp;

	if (data1 == 0 || data2 == 0)
		return 1;

	temp1 = data1;
	temp2 = data2;

	if (temp1 < temp2) {
		temp = temp1;
		temp1 = temp2;
		temp2 = temp;
	}
	temp = temp1 % temp2;
	while (temp) {
		temp1 = temp2;
		temp2 = temp;
		temp = temp1 % temp2;
	}

	return temp2;
}

/*
 * convert PIRL Data structure to PIRL Resource structure.
 * if PIRL Data is not valid, return GT_BAD_PARARM;
*/
static GT_STATUS pirl2DataToResource
(
    IN  GT_QD_DEV              *dev,
    IN  GT_PIRL2_DATA        *pirlData,
    OUT GT_PIRL2_RESOURCE    *res
)
{
    GT_U32 typeMask;
    GT_U32 data;
	GT_U32 burst_allocation;
	GT_U32 pirl2_cir;
	GT_U32 pirl_gcd = 1;

    gtMemSet((void*)res,0,sizeof(GT_PIRL2_RESOURCE));

    data = (GT_U32)(pirlData->accountQConf|pirlData->accountFiltered|
                    pirlData->mgmtNrlEn|pirlData->saNrlEn|pirlData->daNrlEn|
                    pirlData->samplingMode);

    if (data > 1)
    {
        DBG_INFO(("GT_BAD_PARAM (Boolean)\n"));
        return GT_BAD_PARAM;
    }

    if (IS_IN_DEV_GROUP(dev,DEV_RESTRICTED_PIRL2_RESOURCE))
    {
        if (pirlData->samplingMode != GT_FALSE)
        {
            DBG_INFO(("GT_BAD_PARAM (sampling mode)\n"));
            return GT_BAD_PARAM;
        }
    }

    res->accountQConf = pirlData->accountQConf;
    res->accountFiltered = pirlData->accountFiltered;
    res->mgmtNrlEn = pirlData->mgmtNrlEn;
    res->saNrlEn = pirlData->saNrlEn;
    res->daNrlEn = pirlData->daNrlEn;
    res->samplingMode = pirlData->samplingMode;

    switch(pirlData->actionMode)
    {
        case PIRL_ACTION_ACCEPT:
        case PIRL_ACTION_USE_LIMIT_ACTION:
            res->actionMode = pirlData->actionMode;
            break;
        default:
            DBG_INFO(("GT_BAD_PARAM actionMode\n"));
            return GT_BAD_PARAM;
    }

    switch(pirlData->ebsLimitAction)
    {
        case ESB_LIMIT_ACTION_DROP:
        case ESB_LIMIT_ACTION_FC:
            res->ebsLimitAction = pirlData->ebsLimitAction;
            break;
        default:
            DBG_INFO(("GT_BAD_PARAM ebsLimitAction\n"));
            return GT_BAD_PARAM;
    }

    switch(pirlData->fcDeassertMode)
    {
        case GT_PIRL_FC_DEASSERT_EMPTY:
        case GT_PIRL_FC_DEASSERT_CBS_LIMIT:
            res->fcDeassertMode = pirlData->fcDeassertMode;
            break;
        default:
            if(res->ebsLimitAction != ESB_LIMIT_ACTION_FC)
            {
                res->fcDeassertMode    = GT_PIRL_FC_DEASSERT_EMPTY;
                break;
            }
            DBG_INFO(("GT_BAD_PARAM fcDeassertMode\n"));
            return GT_BAD_PARAM;
    }

    if(pirlData->customSetup.isValid == GT_TRUE)
    {
        res->ebsLimit = pirlData->customSetup.ebsLimit;
        res->cbsLimit = pirlData->customSetup.cbsLimit;
        res->bktIncrement = pirlData->customSetup.bktIncrement;
        res->bktRateFactor = pirlData->customSetup.bktRateFactor;
    }
    else
    {
        if(pirlData->ingressRate == 0)
        {
            DBG_INFO(("GT_BAD_PARAM ingressRate(%i)\n",pirlData->ingressRate));
            return GT_BAD_PARAM;
        }

	if (pirlData->ingressRate < 1000) { /* less than 1Mbps */
		/* it should be divided by 64 */
		if (pirlData->ingressRate % 64) {
			DBG_INFO(("GT_BAD_PARAM ingressRate(%i)\n", pirlData->ingressRate));
			return GT_BAD_PARAM;
		}
		/* Less than 1Mbps, use special value */
		res->bktIncrement = pirl2RateLimitParaTbl[pirlData->ingressRate / 64].BI;
		res->bktRateFactor = pirl2RateLimitParaTbl[pirlData->ingressRate / 64].BRF;
		res->cbsLimit = pirl2RateLimitParaTbl[pirlData->ingressRate / 64].CBS;
		res->ebsLimit = pirl2RateLimitParaTbl[pirlData->ingressRate / 64].EBS;
	} else if (pirlData->ingressRate <= 20000) {
		/* greater or equal to 1Mbps, and less than or equal to 20Mbps, it should be divided by 1000 */
		if (pirlData->ingressRate % 1000) {
			DBG_INFO(("GT_BAD_PARAM ingressRate(%i)\n", pirlData->ingressRate));
			return GT_BAD_PARAM;
		}
		res->bktIncrement = pirl2RateLimitParaTbl[PIRL_RATE_960K + pirlData->ingressRate / 1000].BI;
		res->bktRateFactor = pirl2RateLimitParaTbl[PIRL_RATE_960K + pirlData->ingressRate / 1000].BRF;
		res->cbsLimit = pirl2RateLimitParaTbl[PIRL_RATE_960K + pirlData->ingressRate / 1000].CBS;
		res->ebsLimit = pirl2RateLimitParaTbl[PIRL_RATE_960K + pirlData->ingressRate / 1000].EBS;
	} else {/* greater than 20Mbps */
		if (pirlData->ingressRate < 100000) {
			/* it should be divided by 1000, if less than 100Mbps*/
			if (pirlData->ingressRate % 1000) {
				DBG_INFO(("GT_BAD_PARAM ingressRate(%i)\n", pirlData->ingressRate));
				return GT_BAD_PARAM;
			}
		} else {
			/* it should be divided by 10000, if more or equal than 100Mbps */
			if (pirlData->ingressRate % 10000) {
				DBG_INFO(("GT_BAD_PARAM ingressRate(%i)\n", pirlData->ingressRate));
				return GT_BAD_PARAM;
			}
		}

		pirl2_cir = pirlData->ingressRate * 1000;

		burst_allocation = pirl2_cir;

		pirl_gcd = pirl2GetGCD(pirl2_cir, PIRL_ALPHA);
		res->bktRateFactor = pirl2_cir / pirl_gcd;
		/* Correct Rate Factor because the actuall rate  will be 5/4 of the setting rate,
		so we should decrease the configuration rate to 4/5,
		if we use res->bktRateFactor = res->bktRateFactor * 4 / 5, then bktRateFactor might be a decimal,
		it will cause inaccurate,
		so here I amplify bktRateFactor by 4 and amplify bktIncrement by 5.
		And if we want to hold random size packets, we can plus 1 more to bktRateFactor, here I do not do it.
		Since in all cases, res->bktRateFactor * 4 will not be larger than 2^16,
		res->bktIncrement *5 will not be larger than 2^12,  so it's safe*/
		res->bktRateFactor = res->bktRateFactor * 4;
		res->bktIncrement = (PIRL_ALPHA / pirl_gcd) * 5;

		res->ebsLimit = RECOMMENDED_ESB_LIMIT(dev, pirlData->ingressRate);
		/* cbs = ebs - ba*bi/8, and we should avoid the counting number > ULONG_MAX*/
		if ((burst_allocation / 8) >
			(((~0UL) - RECOMMENDED_CBS_LIMIT(dev, pirlData->ingressRate)) / res->bktIncrement))
			res->cbsLimit = RECOMMENDED_CBS_LIMIT(dev, pirlData->ingressRate);
		else if (res->ebsLimit > (res->bktIncrement * (burst_allocation/8)
					+ RECOMMENDED_CBS_LIMIT(dev, pirlData->ingressRate)))
			res->cbsLimit = res->ebsLimit - res->bktIncrement * (burst_allocation/8);
		else
			res->cbsLimit = RECOMMENDED_CBS_LIMIT(dev, pirlData->ingressRate);

		DBG_INFO(("ingressRate %u pirl_gcd %u bktIncrement from 0x%x increased to 0x%x ",
			pirlData->ingressRate, pirl_gcd, res->bktIncrement / 5, res->bktIncrement));
		DBG_INFO(("bktRateFactor from 0x%x increased to 0x%x cbsLimit 0x%x\r\n",
			res->bktRateFactor / 4,	res->bktRateFactor, res->cbsLimit));
	}
    }

    switch(pirlData->bktRateType)
    {
        case BUCKET_TYPE_TRAFFIC_BASED:
            res->bktRateType = pirlData->bktRateType;

            typeMask = 0x7FFF;

            if (pirlData->bktTypeMask > typeMask)
            {
                DBG_INFO(("GT_BAD_PARAM bktTypeMask(%#x)\n",pirlData->bktTypeMask));
                return GT_BAD_PARAM;
            }

               res->bktTypeMask = pirlData->bktTypeMask;

            if (pirlData->bktTypeMask & BUCKET_TRAFFIC_ARP)
            {
                res->bktTypeMask &= ~BUCKET_TRAFFIC_ARP;
                res->bktTypeMask |= 0x80;
            }

            if (pirlData->priORpt > 1)
            {
                DBG_INFO(("GT_BAD_PARAM rpiORpt\n"));
                return GT_BAD_PARAM;
            }

            res->priORpt = pirlData->priORpt;

            if (pirlData->priMask >= (1 << 4))
            {
                DBG_INFO(("GT_BAD_PARAM priMask(%#x)\n",pirlData->priMask));
                return GT_BAD_PARAM;
            }

            res->priMask = pirlData->priMask;

            break;

        case BUCKET_TYPE_RATE_BASED:
            res->bktRateType = pirlData->bktRateType;
               res->bktTypeMask = pirlData->bktTypeMask;
            res->priORpt = pirlData->priORpt;
            res->priMask = pirlData->priMask;
            break;

        default:
            DBG_INFO(("GT_BAD_PARAM bktRateType(%#x)\n",pirlData->bktRateType));
            return GT_BAD_PARAM;
    }

    switch(pirlData->byteTobeCounted)
    {
        case GT_PIRL2_COUNT_FRAME:
        case GT_PIRL2_COUNT_ALL_LAYER1:
        case GT_PIRL2_COUNT_ALL_LAYER2:
        case GT_PIRL2_COUNT_ALL_LAYER3:
            res->byteTobeCounted = pirlData->byteTobeCounted;
            break;
        default:
            DBG_INFO(("GT_BAD_PARAM byteTobeCounted(%#x)\n",pirlData->byteTobeCounted));
            return GT_BAD_PARAM;
    }

    return GT_OK;
}

/*
 * convert PIRL Resource structure to PIRL Data structure.
*/
static GT_STATUS pirl2ResourceToData
(
    IN  GT_QD_DEV              *dev,
    IN  GT_PIRL2_RESOURCE    *res,
    OUT GT_PIRL2_DATA        *pirlData
)
{
    GT_U32    rate;
    GT_U32    factor;

    pirlData->accountQConf = res->accountQConf;
    pirlData->accountFiltered = res->accountFiltered;
    pirlData->mgmtNrlEn = res->mgmtNrlEn;
    pirlData->saNrlEn = res->saNrlEn;
    pirlData->daNrlEn = res->daNrlEn;
    pirlData->samplingMode = res->samplingMode;
    pirlData->ebsLimitAction = res->ebsLimitAction;
    pirlData->actionMode = res->actionMode;
    pirlData->fcDeassertMode = res->fcDeassertMode;

    pirlData->customSetup.isValid = GT_FALSE;

    FACTOR_FROM_BUCKET_INCREMENT(dev,res->bktIncrement,factor);

    rate = res->bktRateFactor * factor;
    if(factor == 128)
    {
        pirlData->ingressRate = rate - (rate % 1000);
    }
    else if (factor == 0)
    {
        pirlData->ingressRate = 0;
        pirlData->customSetup.isValid = GT_TRUE;
        pirlData->customSetup.ebsLimit = res->ebsLimit;
        pirlData->customSetup.cbsLimit = res->cbsLimit;
        pirlData->customSetup.bktIncrement = res->bktIncrement;
        pirlData->customSetup.bktRateFactor = res->bktRateFactor;
    }
    else
    {
        pirlData->ingressRate = rate;
    }

    pirlData->bktRateType = res->bktRateType;
    pirlData->bktTypeMask = res->bktTypeMask;

    if (pirlData->bktTypeMask & 0x80)
    {
        res->bktTypeMask &= ~0x80;
        res->bktTypeMask |= BUCKET_TRAFFIC_ARP;
    }

    pirlData->priORpt = res->priORpt;
    pirlData->priMask = res->priMask;

    pirlData->byteTobeCounted = res->byteTobeCounted;

    return GT_OK;
}

/*******************************************************************************
* pirl2WriteResource
*
* DESCRIPTION:
*       This function writes IRL Resource to BCM (Bucket Configuration Memory)
*
* INPUTS:
*       irlPort - physical port number.
*        irlRes  - bucket to be used (0 ~ 4).
*       res     - IRL Resource data
*
* OUTPUTS:
*       Nont.
*
* RETURNS:
*       GT_OK on success,
*       GT_FAIL otherwise.
*
* COMMENTS:
*
*******************************************************************************/
static GT_STATUS pirl2WriteResource
(
    IN  GT_QD_DEV              *dev,
    IN    GT_U32                irlPort,
    IN    GT_U32                irlRes,
    IN  GT_PIRL2_RESOURCE    *res
)
{
    GT_STATUS       retVal;            /* Functions return value */
    GT_U16          data[8];     /* temporary Data storage */
    GT_PIRL2_OPERATION    op;
    GT_PIRL2_OP_DATA     opData;
    int                i;

    op = PIRL_WRITE_RESOURCE;

    /* reg0 data */
    data[0] = (GT_U16)((res->bktRateType << 15) |    /* Bit[15] : Bucket Rate Type */
                      (res->bktTypeMask << 0 ));         /* Bit[14:0] : Traffic Type   */

    /* reg1 data */
    data[1] = (GT_U16)res->bktIncrement;    /* Bit[11:0] : Bucket Increment */

    /* reg2 data */
    data[2] = (GT_U16)res->bktRateFactor;    /* Bit[15:0] : Bucket Rate Factor */

    /* reg3 data */
    data[3] = (GT_U16)((res->cbsLimit & 0xFFF) << 4)|    /* Bit[15:4] : CBS Limit[11:0] */
                    (res->byteTobeCounted << 2);        /* Bit[3:0] : Bytes to be counted */

    /* reg4 data */
    data[4] = (GT_U16)(res->cbsLimit >> 12);        /* Bit[11:0] : CBS Limit[23:12] */

    /* reg5 data */
    data[5] = (GT_U16)(res->ebsLimit & 0xFFFF);        /* Bit[15:0] : EBS Limit[15:0] */

    /* reg6 data */
    data[6] = (GT_U16)((res->ebsLimit >> 16)    |    /* Bit[7:0] : EBS Limit[23:16] */
                    (res->samplingMode << 11)    |    /* Bit[11] : Sampling Mode */
                    (res->ebsLimitAction << 12)    |    /* Bit[12] : EBS Limit Action */
                    (res->actionMode << 13)        |    /* Bit[13] : Action Mode */
                    (res->fcDeassertMode << 14));    /* Bit[14] : Flow control mode */

    /* reg7 data */
    data[7] = (GT_U16)((res->daNrlEn)            |    /* Bit[0]  : DA Nrl En */
                    (res->saNrlEn << 1)            |    /* Bit[1]  : SA Nrl En */
                    (res->mgmtNrlEn << 2)         |    /* Bit[2]  : MGMT Nrl En */
                    (res->priMask << 8)         |    /* Bit[11:8] : Priority Queue Mask */
                    (res->priORpt << 12)         |    /* Bit[12] : Priority OR PacketType */
                    (res->accountFiltered << 14)|    /* Bit[14] : Account Filtered */
                    (res->accountQConf << 15));        /* Bit[15] : Account QConf */

    for(i=0; i<8; i++)
    {
        opData.irlPort = irlPort;
        opData.irlRes = irlRes;
        opData.irlReg = i;
        opData.irlData = data[i];

        retVal = pirl2OperationPerform(dev, op, &opData);
        if (retVal != GT_OK)
        {
            DBG_INFO(("PIRL OP Failed.\n"));
            return retVal;
        }
    }

    return GT_OK;
}


/*******************************************************************************
* pirl2ReadResource
*
* DESCRIPTION:
*       This function reads IRL Resource from BCM (Bucket Configuration Memory)
*
* INPUTS:
*       irlPort  - physical port number.
*        irlRes   - bucket to be used (0 ~ 4).
*
* OUTPUTS:
*       res - IRL Resource data
*
* RETURNS:
*       GT_OK on success,
*       GT_FAIL otherwise.
*
* COMMENTS:
*
*******************************************************************************/
static GT_STATUS pirl2ReadResource
(
    IN  GT_QD_DEV              *dev,
    IN    GT_U32                irlPort,
    IN    GT_U32                irlRes,
    OUT GT_PIRL2_RESOURCE    *res
)
{
    GT_STATUS       retVal;        /* Functions return value */
    GT_U16          data[8];     /* temporary Data storage */
    GT_PIRL2_OPERATION    op;
    GT_PIRL2_OP_DATA    opData;
    int                i;

    op = PIRL_READ_RESOURCE;

    for(i=0; i<8; i++)
    {
        opData.irlPort = irlPort;
        opData.irlRes = irlRes;
        opData.irlReg = i;
        opData.irlData = 0;

        retVal = pirl2OperationPerform(dev, op, &opData);
        if (retVal != GT_OK)
        {
            DBG_INFO(("PIRL OP Failed.\n"));
            return retVal;
        }

        data[i] = (GT_U16)opData.irlData;
    }


    /* reg0 data */
    res->bktRateType = (data[0] >> 15) & 0x1;
    res->bktTypeMask = (data[0] >> 0) & 0x7FFF;

    /* reg1 data */
    res->bktIncrement = data[1] & 0xFFF;

    /* reg2 data */
    res->bktRateFactor = data[2] & 0xFFFF;

    /* reg3,4 data */
    res->byteTobeCounted = (data[3] >> 2) & 0x3;
    res->cbsLimit = ((data[3] >> 4) & 0xFFF) | ((data[4] & 0xFFF) << 12);

    /* reg5,6 data */
    res->ebsLimit = data[5] | ((data[6] & 0xFF) << 16);

    /* reg6 data */
    res->samplingMode = (data[6] >> 11) & 0x1;
    res->ebsLimitAction = (data[6] >> 12) & 0x1;
    res->actionMode = (data[6] >> 13) & 0x1;
    res->fcDeassertMode = (data[6] >> 14) & 0x1;

    /* reg7 data */
    res->daNrlEn = (data[7] >> 0) & 0x1;
    res->saNrlEn = (data[7] >> 1) & 0x1;
    res->mgmtNrlEn = (data[7] >> 2) & 0x1;
    res->priMask = (data[7] >> 8) & 0xF;
    res->priORpt = (data[7] >> 12) & 0x1;
    res->accountFiltered = (data[7] >> 14) & 0x1;
    res->accountQConf = (data[7] >> 15) & 0x1;

    return GT_OK;
}


/*******************************************************************************
* pirl2WriteTSMResource
*
* DESCRIPTION:
*         This function writes IRL Resource to BCM (Bucket Configuration Memory)
*        in Time Slot Metering Mode.
*
* INPUTS:
*        irlPort - physical port number.
*        irlRes  - bucket to be used (0 ~ 1).
*        res     - IRL Resource data
*
* OUTPUTS:
*        None.
*
* RETURNS:
*        GT_OK on success,
*        GT_FAIL otherwise.
*
* COMMENTS:
*
*******************************************************************************/
static GT_STATUS pirl2WriteTSMResource
(
    IN  GT_QD_DEV              *dev,
    IN    GT_U32                irlPort,
    IN    GT_U32                irlRes,
    IN  GT_PIRL2_TSM_RESOURCE    *res
)
{
    GT_STATUS       retVal;            /* Functions return value */
    GT_U16          data[8];     /* temporary Data storage */
    GT_PIRL2_OPERATION    op;
    GT_PIRL2_OP_DATA     opData;
    int                i;

    op = PIRL_WRITE_RESOURCE;

    /* reg0 data */
    data[0] = 0;

    /* reg1 data */
    data[1] = 0;

    /* reg2 data */
    data[2] = 0;

    /* reg3 data */
    data[3] = (GT_U16)(((res->cbsLimit & 0xFFF) << 4)|    /* Bit[15:4] : CBS Limit[11:0] */
                    (0x2 << 2));                            /* Bit[3:0] : Bytes to be counted */

    /* reg4 data */
    data[4] = (GT_U16)(res->cbsLimit >> 12);        /* Bit[11:0] : CBS Limit[23:12] */

    /* reg5 data */
    data[5] = (GT_U16)(res->ebsLimit & 0xFFFF);        /* Bit[15:0] : EBS Limit[15:0] */

    /* reg6 data */
    data[6] = (GT_U16)(res->actionMode << 13);        /* Bit[13] : Action Mode */

    /* reg7 data */
    data[7] = (GT_U16)((res->tsmMode << 7)        |    /* Bit[7]  : TSM Mode */
                    (res->mgmtNrlEn << 2)         |    /* Bit[2]  : MGMT Nrl En */
                    (res->priMask << 8)         |    /* Bit[11:8] : Priority Queue Mask */
                    (res->ctsIntv << 4));            /* Bit[5:4] : Class Timer Slot Interval */

    for(i=0; i<8; i++)
    {
        opData.irlPort = irlPort;
        opData.irlRes = irlRes;
        opData.irlReg = i;
        opData.irlData = data[i];

        retVal = pirl2OperationPerform(dev, op, &opData);
        if (retVal != GT_OK)
        {
            DBG_INFO(("PIRL OP Failed.\n"));
            return retVal;
        }
    }

    return GT_OK;
}


/*******************************************************************************
* pirl2ReadTSMResource
*
* DESCRIPTION:
*        This function reads IRL Resource from BCM (Bucket Configuration Memory)
*        in Time Slot Metering Mode.
*
* INPUTS:
*        irlPort  - physical port number.
*        irlRes   - bucket to be used (0 ~ 1).
*
* OUTPUTS:
*        res - IRL Resource data
*
* RETURNS:
*         GT_OK on success,
*         GT_FAIL otherwise.
*
* COMMENTS:
*
*******************************************************************************/
static GT_STATUS pirl2ReadTSMResource
(
    IN  GT_QD_DEV              *dev,
    IN    GT_U32                irlPort,
    IN    GT_U32                irlRes,
    OUT GT_PIRL2_TSM_RESOURCE    *res
)
{
    GT_STATUS       retVal;        /* Functions return value */
    GT_U16          data[8];     /* temporary Data storage */
    GT_PIRL2_OPERATION    op;
    GT_PIRL2_OP_DATA    opData;
    int                i;

    op = PIRL_READ_RESOURCE;

    for(i=0; i<8; i++)
    {
        opData.irlPort = irlPort;
        opData.irlRes = irlRes;
        opData.irlReg = i;
        opData.irlData = 0;

        retVal = pirl2OperationPerform(dev, op, &opData);
        if (retVal != GT_OK)
        {
            DBG_INFO(("PIRL OP Failed.\n"));
            return retVal;
        }

        data[i] = (GT_U16)opData.irlData;
    }

    res->tsmMode = data[7] & (1<<7);

    if(res->tsmMode == GT_FALSE)
    {
        /* TMS mode is not set */
        res->cbsLimit = 0;
        res->ebsLimit = 0;
        res->actionMode = 0;
        res->mgmtNrlEn = 0;
        res->priMask = 0;
        res->ctsIntv = 0;

        return GT_OK;
    }

    /* reg3,4 data */
    res->cbsLimit = ((data[3] >> 4) & 0xFFF) | ((data[4] & 0xF) << 12);

    /* reg5,6 data */
    res->ebsLimit = data[5];

    /* reg6 data */
    res->actionMode = (data[6] >> 13) & 0x1;

    /* reg7 data */
    res->mgmtNrlEn = (data[7] >> 2) & 0x1;
    res->priMask = (data[7] >> 8) & 0xF;
    res->ctsIntv = (data[7] >> 4) & 0x3;

    return GT_OK;
}

#define PIRL2_DEBUG
#ifdef PIRL2_DEBUG
/*******************************************************************************
* pirl2DumpResource
*
* DESCRIPTION:
*       This function dumps IRL Resource register values.
*
* INPUTS:
*       irlPort  - physical port number.
*        irlRes   - bucket to be used (0 ~ 4).
*        dataLen  - data size.
*
* OUTPUTS:
*       data - IRL Resource data
*
* RETURNS:
*       GT_OK on success,
*       GT_FAIL otherwise.
*
* COMMENTS:
*
*******************************************************************************/
GT_STATUS pirl2DumpResource
(
    IN  GT_QD_DEV              *dev,
    IN    GT_U32                irlPort,
    IN    GT_U32                irlRes,
    IN    GT_U32                dataLen,
    OUT GT_U16                *data
)
{
    GT_STATUS       retVal;        /* Functions return value */
    GT_PIRL2_OPERATION    op;
    GT_PIRL2_OP_DATA    opData;
    GT_U32                i;

    /* check if device supports this feature */
    if (!IS_IN_DEV_GROUP(dev,DEV_PIRL2_RESOURCE))
    {
        DBG_INFO(("GT_NOT_SUPPORTED\n"));
        return GT_NOT_SUPPORTED;
    }

    op = PIRL_READ_RESOURCE;

    for(i=0; i<dataLen; i++)
    {
        opData.irlPort = irlPort;
        opData.irlRes = irlRes;
        opData.irlReg = i;
        opData.irlData = 0;

        retVal = pirl2OperationPerform(dev, op, &opData);
        if (retVal != GT_OK)
        {
            DBG_INFO(("PIRL OP Failed.\n"));
            return retVal;
        }

        data[i] = (GT_U16)opData.irlData;
    }

    return GT_OK;
}
#endif /* PIRL2_DEBUG */
