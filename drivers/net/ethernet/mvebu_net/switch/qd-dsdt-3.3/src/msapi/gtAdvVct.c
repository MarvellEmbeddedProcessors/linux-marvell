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
#ifdef GT_USE_MAD
#include <gtMad.h>
#endif

#ifdef GT_USE_MAD
#include "gtAdvVct_mad.c"
#endif

#define GT_LOOKUP_TABLE_ENTRY  128  /* 73 */

#define GT_ADV_VCT_ACCEPTABLE_SHORT_CABLE  11

static  GT_U8 tbl_1181[GT_LOOKUP_TABLE_ENTRY] =
                    {  2,  4,  8, 14, 18, 20, 25, 30, 33, 36,
                      39, 42, 46, 48, 51, 54, 57, 59, 62, 64,
                      66, 69, 71, 73, 75, 77, 80, 81, 83, 85,
                      87, 88, 90, 93, 95, 97, 98,100,101,103,
                     104,106,106,107,109,110,111,113,114,115,
                     116,118,119,120,121,122,124,125,126,127,
                     128,129,130,131,132,133,134,135,136,137,
                     138,139,140};

static  GT_U8 tbl_1111[GT_LOOKUP_TABLE_ENTRY] =
                    {  0,  2,  4, 5, 6, 9, 13, 17, 20, 23,
                      27, 30, 33, 35, 38, 41, 43, 46, 48, 51,
                      53, 55, 58, 60, 62, 64, 66, 68, 70, 72,
                      73, 75, 77, 79, 80, 82, 84, 85, 87, 88,
                      90, 91, 93, 94, 96, 97, 98,100,101,102,
                     104,105,106,107,109,110,111,112,113,114,
                     116,117,118,119,120,121,122,123,124,125,
                     126,127,128,129,130,131,132,133,134,134};

static  GT_U8 tbl_1112[GT_LOOKUP_TABLE_ENTRY] =   /* from 17*/
                    {  0,  4,  8, 11, 14, 18, 21, 24, 28, 31,
                      34, 37, 39, 42, 44, 47, 49, 52, 54, 56,
                      58, 60, 62, 64, 66, 68, 70, 72, 74, 75,
                      77, 79, 80, 82, 83, 85, 87, 88, 89, 91,
                      92, 94, 95, 96, 98, 99,100,101,103,104,
                      105,106,107,108,109,111,112,113,114,115,
                      116,117,118,119,120,121,122,123,124,124,
                      125,126,127,128,129,130,131,131,132,133,
                      134,135,135,136,137,138,139,139,140,141,
                      142,142,143,144,144,145,146,147,147,148};

static  GT_U8 tbl_1116[GT_LOOKUP_TABLE_ENTRY] =   /* from 16*/
                    {  2,  4,  8, 14, 18, 20, 25, 30, 33, 36,
                      39, 42, 46, 48, 51, 54, 57, 59, 62, 64,
                      66, 69, 71, 73, 75, 77, 80, 81, 83, 85,
                      87, 88, 90, 93, 95, 97, 98, 100, 101, 103,
                      104,106,106,107,109,110,111,113,114,115,
                      116,118,119,120,121,122,124,125,126,127,
                      128,129,130,131,132,133,134,135,136,137,
                      138,139,140};

static  GT_U8 tbl_1240[GT_LOOKUP_TABLE_ENTRY] =
                    {  1,  2,  5, 10, 13, 15, 18, 22, 26, 30,
                      33, 35, 38, 40, 43, 45, 48, 51, 53, 55,
                      58, 60, 63, 65, 68, 69, 70, 71, 73, 75,
                      77, 79, 80, 81, 82, 83, 85, 87, 88, 90,
                      91, 92, 93, 95, 97, 98,100,101,102,103,
                     105,106,107,108,109,110,111,112,113,114,
                     115,116,117,118,119,120,121,122,123,124,
                     125,126,127,128,129,130};

/*******************************************************************************
* getDetailedAdvVCTResult
*
* DESCRIPTION:
*        This routine differenciate Open/Short from Impedance mismatch.
*
* INPUTS:
*        amp - amplitude
*        len - distance to fault
*        vctResult - test result
*                    (Impedance mismatch, either > 115 ohms, or < 85 ohms)
*
* OUTPUTS:
*
* RETURNS:
*       GT_ADV_VCT_STATUS
*
* COMMENTS:
*       This routine assumes test result is not normal nor cross pair short.
*
*******************************************************************************/
static
GT_ADV_VCT_STATUS getDetailedAdvVCTResult
(
    IN  GT_U32  devType,
    IN  GT_U32  amp,
    IN  GT_U32  len,
    IN  GT_ADV_VCT_STATUS result
)
{
    GT_ADV_VCT_STATUS vctResult;
    GT_BOOL    update = GT_FALSE;

    DBG_INFO(("getDetailedAdvVCTResult Called.\n"));

    if (devType == GT_PHY_ADV_VCT_TYPE2)
    {
        if(len < 10)
        {
            if(amp > 54)  /* 90 x 0.6 */
                update = GT_TRUE;
        }
        else if(len < 50)
        {
            if(amp > 42) /* 70 x 0.6 */
                update = GT_TRUE;
        }
        else if(len < 110)
        {
            if(amp > 30)  /* 50 x 0.6 */
                update = GT_TRUE;
        }
        else if(len < 140)
        {
            if(amp > 24)  /* 40 x 0.6 */
                update = GT_TRUE;
        }
        else
        {
            if(amp > 18) /* 30 x 0.6 */
                update = GT_TRUE;
        }
    }
    else
    {
        if(len < 10)
        {
            if(amp > 90)
                update = GT_TRUE;
        }
        else if(len < 50)
        {
            if(amp > 70)
                update = GT_TRUE;
        }
        else if(len < 110)
        {
            if(amp > 50)
                update = GT_TRUE;
        }
        else if(len < 140)
        {
            if(amp > 40)
                update = GT_TRUE;
        }
        else
        {
            if(amp > 30)
                update = GT_TRUE;
        }
    }


    switch (result)
    {
        case GT_ADV_VCT_IMP_GREATER_THAN_115:
                if(update)
                    vctResult = GT_ADV_VCT_OPEN;
                else
                    vctResult = result;
                break;
        case GT_ADV_VCT_IMP_LESS_THAN_85:
                if(update)
                    vctResult = GT_ADV_VCT_SHORT;
                else
                    vctResult = result;
                break;
        default:
                vctResult = result;
                break;
    }

    return vctResult;
}

/*******************************************************************************
* analizeAdvVCTResult
*
* DESCRIPTION:
*        This routine analize the Advanced VCT result.
*
* INPUTS:
*        channel - channel number where test was run
*        crossChannelReg - register values after the test is completed
*        mode    - use formula for normal cable case
*
* OUTPUTS:
*        cableStatus - analized test result.
*
* RETURNS:
*        -1, or distance to fault
*
* COMMENTS:
*        None.
*
*******************************************************************************/
static
GT_16 analizeAdvVCTNoCrosspairResult
(
    IN  GT_U32  devType,
    IN  int     channel,
    IN  GT_U16 *crossChannelReg,
    IN  GT_BOOL isShort,
    OUT GT_ADV_CABLE_STATUS *cableStatus
)
{
    int len;
    GT_16 dist2fault;
    GT_ADV_VCT_STATUS vctResult = GT_ADV_VCT_NORMAL;

    DBG_INFO(("analizeAdvVCTNoCrosspairResult Called.\n"));
    DBG_INFO(("analizeAdvVCTNoCrosspairResult chan %d reg data %x\n", channel, crossChannelReg[channel]));

    dist2fault = -1;

    /* check if test is failed */
    if(IS_VCT_FAILED(crossChannelReg[channel]))
    {
        cableStatus->cableStatus[channel] = GT_ADV_VCT_FAIL;
        return dist2fault;
    }

    /* Check if fault detected */
    if(IS_ZERO_AMPLITUDE(crossChannelReg[channel]))
    {
        cableStatus->cableStatus[channel] = GT_ADV_VCT_NORMAL;
        return dist2fault;
    }

    /* find out test result by reading Amplitude */
    if(IS_POSITIVE_AMPLITUDE(crossChannelReg[channel]))
    {
        vctResult = GT_ADV_VCT_IMP_GREATER_THAN_115;
    }
    else
    {
        vctResult = GT_ADV_VCT_IMP_LESS_THAN_85;
    }

    /*
     * now, calculate the distance for GT_ADV_VCT_IMP_GREATER_THAN_115 and
     * GT_ADV_VCT_IMP_LESS_THAN_85
     */
    switch (vctResult)
    {
        case GT_ADV_VCT_IMP_GREATER_THAN_115:
        case GT_ADV_VCT_IMP_LESS_THAN_85:
            if(!isShort)
            {
                len = (int)GT_ADV_VCT_CALC(crossChannelReg[channel] & 0xFF);
            }
            else
            {
                len = (int)GT_ADV_VCT_CALC_SHORT(crossChannelReg[channel] & 0xFF);
            }
            DBG_INFO(("@@@@ no cross len %d\n", len));

            if (len < 0)
                len = 0;
            cableStatus->u[channel].dist2fault = (GT_16)len;
            vctResult = getDetailedAdvVCTResult(
                                    devType,
                                    GET_AMPLITUDE(crossChannelReg[channel]),
                                    len,
                                    vctResult);
            dist2fault = (GT_16)len;
            break;
        default:
            break;
    }

    cableStatus->cableStatus[channel] = vctResult;

    return dist2fault;
}


static
GT_16 analizeAdvVCTResult
(
    IN  GT_U32  devType,
    IN  int     channel,
    IN  GT_U16 *crossChannelReg,
    IN  GT_BOOL isShort,
    OUT GT_ADV_CABLE_STATUS *cableStatus
)
{
    int i, len;
    GT_16 dist2fault;
    GT_ADV_VCT_STATUS vctResult = GT_ADV_VCT_NORMAL;

    DBG_INFO(("analizeAdvVCTResult(Crosspair) chan %d reg data %x\n", channel, crossChannelReg[channel]));
    DBG_INFO(("analizeAdvVCTResult Called.\n"));

    dist2fault = -1;

    /* check if test is failed */
    for (i=0; i<GT_MDI_PAIR_NUM; i++)
    {
        if(IS_VCT_FAILED(crossChannelReg[i]))
        {
            cableStatus->cableStatus[channel] = GT_ADV_VCT_FAIL;
            return dist2fault;
        }
    }

    /* find out test result by reading Amplitude */
    for (i=0; i<GT_MDI_PAIR_NUM; i++)
    {
        if (i == channel)
        {
            if(!IS_ZERO_AMPLITUDE(crossChannelReg[i]))
            {
                if(IS_POSITIVE_AMPLITUDE(crossChannelReg[i]))
                {
                    vctResult = GT_ADV_VCT_IMP_GREATER_THAN_115;
                }
                else
                {
                    vctResult = GT_ADV_VCT_IMP_LESS_THAN_85;
                }
            }
            continue;
        }

        if(IS_ZERO_AMPLITUDE(crossChannelReg[i]))
            continue;

        vctResult = GT_ADV_VCT_CROSS_PAIR_SHORT;
        break;
    }

    /* if it is cross pair short, check the distance for each channel */
    if(vctResult == GT_ADV_VCT_CROSS_PAIR_SHORT)
    {
        cableStatus->cableStatus[channel] = GT_ADV_VCT_CROSS_PAIR_SHORT;
        for (i=0; i<GT_MDI_PAIR_NUM; i++)
        {
            if(IS_ZERO_AMPLITUDE(crossChannelReg[i]))
            {
                cableStatus->u[channel].crossShort.channel[i] = GT_FALSE;
                cableStatus->u[channel].crossShort.dist2fault[i] = 0;
                continue;
            }

            cableStatus->u[channel].crossShort.channel[i] = GT_TRUE;
            if(!isShort)
                len = (int)GT_ADV_VCT_CALC(crossChannelReg[i] & 0xFF);
            else
                len = (int)GT_ADV_VCT_CALC_SHORT(crossChannelReg[i] & 0xFF);
            DBG_INFO(("@@@@ len %d\n", len));

            if (len < 0)
                len = 0;
            cableStatus->u[channel].crossShort.dist2fault[i] = (GT_16)len;
            dist2fault = (GT_16)len;
        }

        return dist2fault;
    }

    /*
     * now, calculate the distance for GT_ADV_VCT_IMP_GREATER_THAN_115 and
     * GT_ADV_VCT_IMP_LESS_THAN_85
     */
    switch (vctResult)
    {
        case GT_ADV_VCT_IMP_GREATER_THAN_115:
        case GT_ADV_VCT_IMP_LESS_THAN_85:
            if(isShort)
                len = (int)GT_ADV_VCT_CALC(crossChannelReg[channel] & 0xFF);
            else
                len = (int)GT_ADV_VCT_CALC_SHORT(crossChannelReg[channel] & 0xFF);
            if (len < 0)
                len = 0;
            cableStatus->u[channel].dist2fault = (GT_16)len;
            vctResult = getDetailedAdvVCTResult(
                                    devType,
                                    GET_AMPLITUDE(crossChannelReg[channel]),
                                    len,
                                    vctResult);
            dist2fault = (GT_16)len;
            break;
        default:
            break;
    }

    cableStatus->cableStatus[channel] = vctResult;

    return dist2fault;
}


/*******************************************************************************
* runAdvCableTest_1181
*
* DESCRIPTION:
*        This routine performs the advanced virtual cable test for the PHY with
*        multiple page mode and returns the the status per MDIP/N.
*
* INPUTS:
*        port - logical port number.
*        mode - GT_TRUE, if short cable detect is required
*               GT_FALSE, otherwise
*
* OUTPUTS:
*        cableStatus - the port copper cable status.
*        tooShort    - if known distance to fault is too short
*
* RETURNS:
*        GT_OK   - on success
*        GT_FAIL - on error
*
* COMMENTS:
*        None.
*
*******************************************************************************/
static
GT_STATUS runAdvCableTest_1181
(
    IN  GT_QD_DEV       *dev,
    IN  GT_U8           hwPort,
    IN    GT_PHY_INFO        *phyInfo,
    IN  GT_BOOL         mode,
    OUT GT_ADV_CABLE_STATUS *cableStatus,
    OUT GT_BOOL         *tooShort
)
{
    GT_STATUS retVal;
    GT_U16 u16Data;
    GT_U16 crossChannelReg[GT_MDI_PAIR_NUM];
    int i,j;
    GT_16  dist2fault;

    VCT_REGISTER regList[GT_MDI_PAIR_NUM][GT_MDI_PAIR_NUM] = {
                            {{8,16},{8,17},{8,18},{8,19}},  /* channel 0 */
                            {{8,24},{8,25},{8,26},{8,27}},  /* channel 1 */
                            {{9,16},{9,17},{9,18},{9,19}},  /* channel 2 */
                            {{9,24},{9,25},{9,26},{9,27}}   /* channel 3 */
                            };

    DBG_INFO(("runAdvCableTest_1181 Called.\n"));

    if (mode)
        *tooShort = GT_FALSE;

    /*
     * start Advanced Virtual Cable Tester
     */
    if((retVal = hwSetPagedPhyRegField(
                        dev,hwPort,8,QD_REG_ADV_VCT_CONTROL_8,15,1,phyInfo->anyPage,1)) != GT_OK)
    {
        DBG_INFO(("Writing to paged phy reg failed.\n"));
        return retVal;
    }

    /*
     * loop until test completion and result is valid
     */
    do
    {
        if((retVal = hwReadPagedPhyReg(
                            dev,hwPort,8,QD_REG_ADV_VCT_CONTROL_8,phyInfo->anyPage,&u16Data)) != GT_OK)
        {
            DBG_INFO(("Reading from paged phy reg failed.\n"));
            return retVal;
        }
    } while(u16Data & 0x8000);

    DBG_INFO(("Page 8 of Reg20 after test : %0#x.\n", u16Data));

    for (i=0; i<GT_MDI_PAIR_NUM; i++)
    {
        /*
         * read the test result for the cross pair against selected MDI Pair
         */
        for (j=0; j<GT_MDI_PAIR_NUM; j++)
        {
            if((retVal = hwReadPagedPhyReg(
                                dev,hwPort,
                                regList[i][j].page,
                                regList[i][j].regOffset,
                                phyInfo->anyPage,
                                &crossChannelReg[j])) != GT_OK)
            {
                DBG_INFO(("Reading from paged phy reg failed.\n"));
                return retVal;
            }
        }

        /*
         * analyze the test result for RX Pair
         */
        dist2fault = analizeAdvVCTResult(phyInfo->vctType, i, crossChannelReg, mode, cableStatus);

        if(mode)
        {
            if ((dist2fault>=0) && (dist2fault<GT_ADV_VCT_ACCEPTABLE_SHORT_CABLE))
            {
                DBG_INFO(("Distance to Fault is too Short. So, rerun after changing pulse width\n"));
                *tooShort = GT_TRUE;
                break;
            }
        }
    }

    return GT_OK;
}



/*******************************************************************************
* getAdvCableStatus_1181
*
* DESCRIPTION:
*        This routine performs the virtual cable test for the PHY with
*        multiple page mode and returns the the status per MDIP/N.
*
* INPUTS:
*        port - logical port number.
*        mode - advance VCT mode (either First Peak or Maximum Peak)
*
* OUTPUTS:
*        cableStatus - the port copper cable status.
*
* RETURNS:
*        GT_OK   - on success
*        GT_FAIL - on error
*
* COMMENTS:
*        None.
*
*******************************************************************************/
static
GT_STATUS getAdvCableStatus_1181
(
    IN  GT_QD_DEV          *dev,
    IN  GT_U8           hwPort,
    IN    GT_PHY_INFO        *phyInfo,
    IN  GT_ADV_VCT_MODE mode,
    OUT GT_ADV_CABLE_STATUS *cableStatus
)
{
    GT_STATUS retVal;
    GT_U16 orgPulse, u16Data;
    GT_BOOL flag, tooShort;

    flag = GT_TRUE;

    /*
     * set Adv VCT Mode
     */
    switch (mode.mode)
    {
        case GT_ADV_VCT_FIRST_PEAK:
            break;
        case GT_ADV_VCT_MAX_PEAK:
            break;
        default:
            DBG_INFO(("Unknown Advanced VCT Mode.\n"));
            return GT_BAD_PARAM;
    }

    u16Data = (mode.mode<<6) | (mode.peakDetHyst) | (mode.sampleAvg<<8);
    if((retVal = hwSetPagedPhyRegField(
                        dev,hwPort,8,QD_REG_ADV_VCT_CONTROL_8,0,11,phyInfo->anyPage,u16Data)) != GT_OK)
    {
        DBG_INFO(("Writing to paged phy reg failed.\n"));
        return retVal;
    }

    if (flag)
    {
        /* save original Pulse Width */
        if((retVal = hwGetPagedPhyRegField(
                            dev,hwPort,9,23,10,2,phyInfo->anyPage,&orgPulse)) != GT_OK)
        {
            DBG_INFO(("Reading paged phy reg failed.\n"));
            return retVal;
        }

        /* set the Pulse Width with default value */
        if (orgPulse != 0)
        {
            if((retVal = hwSetPagedPhyRegField(
                                dev,hwPort,9,23,10,2,phyInfo->anyPage,0)) != GT_OK)
            {
                DBG_INFO(("Writing to paged phy reg failed.\n"));
                return retVal;
            }
        }
    }

    if((retVal=runAdvCableTest_1181(dev,hwPort,phyInfo,flag,cableStatus,&tooShort)) != GT_OK)
    {
        DBG_INFO(("Running advanced VCT failed.\n"));
        return retVal;
    }

    if (flag)
    {
        if(tooShort)
        {
            /* set the Pulse Width with minimum width */
            if((retVal = hwSetPagedPhyRegField(
                                dev,hwPort,9,23,10,2,phyInfo->anyPage,3)) != GT_OK)
            {
                DBG_INFO(("Writing to paged phy reg failed.\n"));
                return retVal;
            }

            /* run the Adv VCT again */
            if((retVal=runAdvCableTest_1181(dev,hwPort,phyInfo,GT_FALSE,cableStatus,&tooShort)) != GT_OK)
            {
                DBG_INFO(("Running advanced VCT failed.\n"));
                return retVal;
            }

        }

        /* set the Pulse Width back to the original value */
        if((retVal = hwSetPagedPhyRegField(
                            dev,hwPort,9,23,10,2,phyInfo->anyPage,orgPulse)) != GT_OK)
        {
            DBG_INFO(("Writing to paged phy reg failed.\n"));
            return retVal;
        }

    }

    return GT_OK;
}


static
GT_STATUS runAdvCableTest_1116_set
(
    IN  GT_QD_DEV          *dev,
    IN  GT_U8           hwPort,
    IN    GT_PHY_INFO        *phyInfo,
    IN  GT_32           channel,
    IN  GT_ADV_VCT_TRANS_CHAN_SEL        crosspair
)
{
    GT_STATUS retVal;

    DBG_INFO(("runAdvCableTest_1116_set Called.\n"));

    /*
     * start Advanced Virtual Cable Tester
     */
    if((retVal = hwSetPagedPhyRegField(
                        dev,hwPort,5,QD_REG_ADV_VCT_CONTROL_5,15,1,phyInfo->anyPage,1)) != GT_OK)
    {
        DBG_INFO(("Writing to paged phy reg failed.\n"));
        return retVal;
    }

    return GT_OK;
}

static
GT_STATUS runAdvCableTest_1116_check
(
    IN  GT_QD_DEV       *dev,
    IN  GT_U8           hwPort,
    IN    GT_PHY_INFO        *phyInfo
)
{
    GT_STATUS retVal;
    GT_U16 u16Data;

    /*
     * loop until test completion and result is valid
     */
    do {
        if((retVal = hwReadPagedPhyReg(
                            dev,hwPort,5,QD_REG_ADV_VCT_CONTROL_5,phyInfo->anyPage,&u16Data)) != GT_OK)
        {
            DBG_INFO(("Reading from paged phy reg failed.\n"));
            return retVal;
        }
    } while (u16Data & 0x8000);

    return GT_OK;
}

static
GT_STATUS runAdvCableTest_1116_get
(
    IN  GT_QD_DEV          *dev,
    IN  GT_U8           hwPort,
    IN    GT_PHY_INFO        *phyInfo,
    IN  GT_ADV_VCT_TRANS_CHAN_SEL    crosspair,
    IN  GT_32            channel,
    OUT GT_ADV_CABLE_STATUS *cableStatus,
    OUT GT_BOOL         *tooShort
)
{
    GT_STATUS retVal;
    GT_U16 u16Data;
    GT_U16 crossChannelReg[GT_MDI_PAIR_NUM];
    int j;
    GT_16  dist2fault;
    GT_BOOL         mode;
    GT_BOOL         localTooShort[GT_MDI_PAIR_NUM];

    VCT_REGISTER regList[GT_MDI_PAIR_NUM] = { {5,16},{5,17},{5,18},{5,19} };

    mode = GT_TRUE;

    DBG_INFO(("runAdvCableTest_1116_get Called.\n"));

    if ((retVal = hwReadPagedPhyReg(
                        dev,hwPort,5,QD_REG_ADV_VCT_CONTROL_5,phyInfo->anyPage,&u16Data)) != GT_OK)
    {
        DBG_INFO(("Reading from paged phy reg failed.\n"));
        return retVal;
    }

    DBG_INFO(("Page 5 of Reg23 after test : %0#x.\n", u16Data));

    /*
     * read the test result for the cross pair against selected MDI Pair
     */
    for (j=0; j<GT_MDI_PAIR_NUM; j++)
    {
        if((retVal = hwReadPagedPhyReg(
                                dev,hwPort,
                                regList[j].page,
                                regList[j].regOffset,
                                phyInfo->anyPage,
                                &crossChannelReg[j])) != GT_OK)
        {
            DBG_INFO(("Reading from paged phy reg failed.\n"));
            return retVal;
        }
        DBG_INFO(("@@@@@ reg channel %d is %x \n", j, crossChannelReg[j]));
    }

    /*
     * analyze the test result for RX Pair
     */
    for (j=0; j<GT_MDI_PAIR_NUM; j++)
    {
        if (crosspair!=GT_ADV_VCT_TCS_NO_CROSSPAIR)
            dist2fault = analizeAdvVCTResult(phyInfo->vctType, j, crossChannelReg, mode&(*tooShort), cableStatus);
        else
            dist2fault = analizeAdvVCTNoCrosspairResult(phyInfo->vctType, j, crossChannelReg, mode&(*tooShort), cableStatus);

        localTooShort[j]=GT_FALSE;
        if((mode)&&(*tooShort==GT_FALSE))
        {
            if ((dist2fault>=0) && (dist2fault<GT_ADV_VCT_ACCEPTABLE_SHORT_CABLE))
            {
                DBG_INFO(("@@@#@@@@ it is too short dist2fault %d\n", dist2fault));
                DBG_INFO(("Distance to Fault is too Short. So, rerun after changing pulse width\n"));
                localTooShort[j]=GT_TRUE;
            }
        }
    }

    /* check and decide if length is too short */
    for (j=0; j<GT_MDI_PAIR_NUM; j++)
    {
        if (localTooShort[j]==GT_FALSE) break;
    }

    if (j==GT_MDI_PAIR_NUM)
        *tooShort = GT_TRUE;

    return GT_OK;
}

static
GT_STATUS runAdvCableTest_1116
(
    IN  GT_QD_DEV          *dev,
    IN  GT_U8           hwPort,
    IN    GT_PHY_INFO        *phyInfo,
    IN  GT_BOOL         mode,
    IN  GT_ADV_VCT_TRANS_CHAN_SEL   crosspair,
    OUT GT_ADV_CABLE_STATUS *cableStatus,
    OUT GT_BOOL         *tooShort
)
{
    GT_STATUS retVal;
    GT_32  channel;

    DBG_INFO(("runAdvCableTest_1116 Called.\n"));

    if (crosspair!=GT_ADV_VCT_TCS_NO_CROSSPAIR)
    {
        channel = crosspair - GT_ADV_VCT_TCS_CROSSPAIR_0;
    }
    else
    {
        channel = 0;
    }

    /* Set transmit channel */
    if((retVal=runAdvCableTest_1116_set(dev,hwPort, phyInfo,channel, crosspair)) != GT_OK)
    {
        DBG_INFO(("Running advanced VCT failed.\n"));
        return retVal;
    }

    /*
     * check test completion
     */
    retVal = runAdvCableTest_1116_check(dev,hwPort,phyInfo);
    if (retVal != GT_OK)
    {
        DBG_INFO(("Running advanced VCT failed.\n"));
        return retVal;
    }

    /*
     * read the test result for the cross pair against selected MDI Pair
     */
    retVal = runAdvCableTest_1116_get(dev, hwPort, phyInfo, crosspair,
                                    channel,cableStatus,(GT_BOOL *)tooShort);

    if(retVal != GT_OK)
    {
        DBG_INFO(("Running advanced VCT get failed.\n"));
    }

    return retVal;
}

static
GT_STATUS getAdvCableStatus_1116
(
    IN  GT_QD_DEV       *dev,
    IN  GT_U8           hwPort,
    IN    GT_PHY_INFO        *phyInfo,
    IN  GT_ADV_VCT_MODE mode,
    OUT GT_ADV_CABLE_STATUS *cableStatus
)
{
    GT_STATUS retVal;
    GT_U16 orgPulse, u16Data;
    GT_BOOL flag, tooShort;
    GT_ADV_VCT_TRANS_CHAN_SEL crosspair;

    flag = GT_TRUE;
    crosspair = mode.transChanSel;

    /*
     * Check Adv VCT Mode
     */
    switch (mode.mode)
    {
        case GT_ADV_VCT_FIRST_PEAK:
        case GT_ADV_VCT_MAX_PEAK:
                break;

        default:
                DBG_INFO(("Unknown ADV VCT Mode.\n"));
                return GT_NOT_SUPPORTED;
    }

    if((retVal = hwGetPagedPhyRegField(
                            dev,hwPort,5,QD_REG_ADV_VCT_CONTROL_5,0,13,phyInfo->anyPage,&u16Data)) != GT_OK)
    {
        DBG_INFO(("Reading paged phy reg failed.\n"));
        return retVal;
    }

    u16Data |= ((mode.mode<<6) | (mode.transChanSel<<11));
    if (mode.peakDetHyst) u16Data |= (mode.peakDetHyst);
    if (mode.sampleAvg) u16Data |= (mode.sampleAvg<<8) ;

    if((retVal = hwSetPagedPhyRegField(
                        dev,hwPort,5,QD_REG_ADV_VCT_CONTROL_5,0,13,phyInfo->anyPage,u16Data)) != GT_OK)
    {
        DBG_INFO(("Writing to paged phy reg failed.\n"));
        return retVal;
    }

    if (flag)
    {
        /* save original Pulse Width */
        if((retVal = hwGetPagedPhyRegField(dev,hwPort,5,28,10,2,phyInfo->anyPage,&orgPulse)) != GT_OK)
        {
            DBG_INFO(("Reading paged phy reg failed.\n"));
            return retVal;
        }

        /* set the Pulse Width with default value */
        if (orgPulse != 0)
        {
            if((retVal = hwSetPagedPhyRegField(dev,hwPort,5,28,10,2,phyInfo->anyPage,0)) != GT_OK)
            {
                DBG_INFO(("Writing to paged phy reg failed.\n"));
                return retVal;
            }
        }
        tooShort=GT_FALSE;
    }

    if((retVal=runAdvCableTest_1116(dev,hwPort,phyInfo,flag,crosspair,
                                    cableStatus,&tooShort)) != GT_OK)
    {
        DBG_INFO(("Running advanced VCT failed.\n"));
        return retVal;
    }

    if (flag)
    {
        if(tooShort)
        {
            /* set the Pulse Width with minimum width */
            if((retVal = hwSetPagedPhyRegField(
                                        dev,hwPort,5,28,10,2,phyInfo->anyPage,3)) != GT_OK)
            {
                DBG_INFO(("Writing to paged phy reg failed.\n"));
                return retVal;
            }

            /* run the Adv VCT again */
            if((retVal=runAdvCableTest_1116(dev,hwPort,phyInfo,GT_FALSE,crosspair,
                                        cableStatus,&tooShort)) != GT_OK)
            {
                DBG_INFO(("Running advanced VCT failed.\n"));
                return retVal;
            }

        }

        /* set the Pulse Width back to the original value */
        if((retVal = hwSetPagedPhyRegField(
                                dev,hwPort,5,28,10,2,phyInfo->anyPage,orgPulse)) != GT_OK)
        {
            DBG_INFO(("Writing to paged phy reg failed.\n"));
            return retVal;
        }

    }

    return GT_OK;
}


/*******************************************************************************
* gvctGetAdvCableStatus
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
GT_STATUS gvctGetAdvCableDiag
(
    IN  GT_QD_DEV *dev,
    IN  GT_LPORT        port,
    IN  GT_ADV_VCT_MODE mode,
    OUT GT_ADV_CABLE_STATUS *cableStatus
)
{
    GT_STATUS status;
    GT_U8 hwPort;
    GT_U16 u16Data, org0;
    GT_BOOL ppuEn;
    GT_PHY_INFO    phyInfo;
    GT_BOOL            autoOn, autoNeg;
    GT_U16            pageReg;
    int i;

#ifdef GT_USE_MAD
    if (dev->use_mad==GT_TRUE)
        return gvctGetAdvCableDiag_mad(dev, port, mode, cableStatus);
#endif

    DBG_INFO(("gvctGetCableDiag Called.\n"));
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

    if(driverPagedAccessStart(dev,hwPort,phyInfo.pageType,&autoOn,&pageReg) != GT_OK)
    {
        gtSemGive(dev,dev->phyRegsSem);
        return GT_FAIL;
    }

    /*
     * If Fiber is used, simply return with test fail.
     */
    if(phyInfo.flag & GT_PHY_FIBER)
    {
        if((status= hwReadPagedPhyReg(dev,hwPort,1,17,phyInfo.anyPage,&u16Data)) != GT_OK)
        {
        gtSemGive(dev,dev->phyRegsSem);
            return status;
        }

        if(u16Data & 0x400)
        {
            for (i=0; i<GT_MDI_PAIR_NUM; i++)
            {
                cableStatus->cableStatus[i] = GT_ADV_VCT_FAIL;
            }
        gtSemGive(dev,dev->phyRegsSem);
            return GT_OK;
        }
    }

    /*
     * Check the link
     */
    if((status= hwReadPagedPhyReg(dev,hwPort,0,17,phyInfo.anyPage,&u16Data)) != GT_OK)
    {
        DBG_INFO(("Not able to reset the Phy.\n"));
        gtSemGive(dev,dev->phyRegsSem);
        return status;
    }

    autoNeg = GT_FALSE;
    org0 = 0;
    if (!(u16Data & 0x400))
    {
        /* link is down, so disable auto-neg if enabled */
        if((status= hwReadPagedPhyReg(dev,hwPort,0,0,phyInfo.anyPage,&u16Data)) != GT_OK)
        {
            DBG_INFO(("Not able to reset the Phy.\n"));
        gtSemGive(dev,dev->phyRegsSem);
            return status;
        }

        org0 = u16Data;

        if (u16Data & 0x1000)
        {
            u16Data = 0x140;

            /* link is down, so disable auto-neg if enabled */
            if((status= hwWritePagedPhyReg(dev,hwPort,0,0,phyInfo.anyPage,u16Data)) != GT_OK)
            {
                DBG_INFO(("Not able to reset the Phy.\n"));
                return status;
            }

            if((status= hwPhyReset(dev,hwPort,0xFF)) != GT_OK)
            {
                DBG_INFO(("Not able to reset the Phy.\n"));
        gtSemGive(dev,dev->phyRegsSem);
                return status;
            }
            autoNeg = GT_TRUE;
        }
    }

    switch(phyInfo.vctType)
    {
        case GT_PHY_ADV_VCT_TYPE1:
            status = getAdvCableStatus_1181(dev,hwPort,&phyInfo,mode,cableStatus);
            break;
        case GT_PHY_ADV_VCT_TYPE2:
            status = getAdvCableStatus_1116(dev,hwPort,&phyInfo,mode,cableStatus);
            break;
        default:
            status = GT_FAIL;
            break;
    }

    if (autoNeg)
    {
        if((status= hwPhyReset(dev,hwPort,org0)) != GT_OK)
        {
            DBG_INFO(("Not able to reset the Phy.\n"));
            goto cableDiagCleanup;
        gtSemGive(dev,dev->phyRegsSem);
            return status;
        }
    }
cableDiagCleanup:

    if(driverPagedAccessStop(dev,hwPort,phyInfo.pageType,autoOn,pageReg) != GT_OK)
    {
        gtSemGive(dev,dev->phyRegsSem);
        return GT_FAIL;
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
* dspLookup
*
* DESCRIPTION:
*       This routine returns cable length (meters) by reading DSP Lookup table.
*
* INPUTS:
*       regValue - register 21
*
* OUTPUTS:
*       cableLen - cable length (unit of meters).
*
* RETURNS:
*       GT_OK   - on success
*       GT_FAIL - on error
*
* COMMENTS:
*       None.
*
*******************************************************************************/
static
GT_STATUS dspLookup
(
    IN    GT_PHY_INFO        *phyInfo,
    IN  GT_U16 regValue,
    OUT GT_32  *cableLen
)
{
    GT_U16 startEntry,tableEntry;
    GT_U8* tbl;
    switch(phyInfo->exStatusType)
    {
        case GT_PHY_EX_STATUS_TYPE1:    /* 88E1111/88E1141/E1145 */
            startEntry = 18-1;
            tableEntry = 80;
            tbl = tbl_1111;
            break;

        case GT_PHY_EX_STATUS_TYPE2:    /* 88E1112 */
            startEntry = 17;
            tableEntry = 100;
            tbl = tbl_1112;
            break;

        case GT_PHY_EX_STATUS_TYPE3:   /* 88E1149 has no reference constans*/
            startEntry = 16;
            tableEntry = 73;
            tbl = tbl_1181;
            break;

        case GT_PHY_EX_STATUS_TYPE4:   /* 88E1181 */
            startEntry = 16;
            tableEntry = 73;
            tbl = tbl_1181;
            break;

        case GT_PHY_EX_STATUS_TYPE5:   /* 88E1116 88E1121 */
            startEntry = 16;
            tableEntry = 73;
            tbl = tbl_1116;
            break;

        case GT_PHY_EX_STATUS_TYPE6:   /* 88E6165 Internal Phy */
            if ((phyInfo->phyId & PHY_MODEL_MASK) == DEV_G65G)
                startEntry = 18;
            else
                startEntry = 21;
            tableEntry = 76;
            tbl = tbl_1240;
            break;

        default:
            return GT_NOT_SUPPORTED;
    }

    if (tbl == NULL)
    {
        *cableLen = -1;
        return GT_OK;
    }

    if (regValue < startEntry)
    {
        *cableLen = 0;
        return GT_OK;
    }

    if (regValue >= (tableEntry+startEntry-1))
    {
        regValue = tableEntry-1;
    }
    else
    {
        regValue -= startEntry;
    }

    *cableLen = (GT_32)tbl[regValue];
    return GT_OK;
}

/*******************************************************************************
* getDSPDistance_1111
*
* DESCRIPTION:
*       This routine returns cable length (meters) from DSP method.
*       This routine is for the 88E1111 like devices.
*
* INPUTS:
*       mdi - pair of each MDI (0..3).
*
* OUTPUTS:
*       cableLen - cable length (unit of meters).
*
* RETURNS:
*       GT_OK   - on success
*       GT_FAIL - on error
*
* COMMENTS:
*       None.
*
*******************************************************************************/
static
GT_STATUS getDSPDistance_1111
(
    IN  GT_QD_DEV *dev,
    IN  GT_U8  hwPort,
    IN    GT_PHY_INFO        *phyInfo,
    IN  GT_U32 mdi,
    OUT GT_32 *cableLen
)
{
    GT_U16     data, pageNum;
    GT_STATUS  retVal;

    DBG_INFO(("getDSPDistance Called.\n"));

    pageNum = 0x8754 + (GT_U16)((mdi << 12)&0xf000);

    if((retVal = hwReadPagedPhyReg(dev,hwPort,(GT_U8)pageNum,31,phyInfo->anyPage,&data)) != GT_OK)
    {
        DBG_INFO(("Reading length of MDI pair failed.\n"));
        return retVal;
    }

    return dspLookup(phyInfo,data,cableLen);
}


/*******************************************************************************
* getDSPDistance_1181
*
* DESCRIPTION:
*       This routine returns cable length (meters) from DSP method.
*       This routine is for the 88E1181 like devices.
*
* INPUTS:
*       mdi - pair of each MDI (0..3).
*
* OUTPUTS:
*       cableLen - cable length (unit of meters).
*
* RETURNS:
*       GT_OK   - on success
*       GT_FAIL - on error
*
* COMMENTS:
*       None.
*
*******************************************************************************/
static
GT_STATUS getDSPDistance_1181
(
    IN  GT_QD_DEV *dev,
    IN  GT_U8  hwPort,
    IN    GT_PHY_INFO        *phyInfo,
    IN  GT_U32 mdi,
    OUT GT_32 *cableLen
)
{
    GT_U16     data, retryCount;
    GT_STATUS  retVal;

    DBG_INFO(("getDSPDistance Called.\n"));

    /* Set the required bits for Cable length register */
    if((retVal = hwWritePagedPhyReg(dev,hwPort,0xff,19,phyInfo->anyPage,(GT_U16)(0x1018+(0xff&mdi)))) != GT_OK)
    {
        DBG_INFO(("Writing to paged phy reg failed.\n"));
        return retVal;
    }

    retryCount = 1000;

    do
    {
        if(retryCount == 0)
        {
            DBG_INFO(("Ready bit of Cable length resiter is not set.\n"));
            return GT_FAIL;
        }

        /* Check the ready bit of Cable length register */
        if((retVal = hwGetPagedPhyRegField(dev,hwPort,0xff,19,15,1,phyInfo->anyPage,&data)) != GT_OK)
        {
            DBG_INFO(("Writing to paged phy reg failed.\n"));
            return retVal;
        }

        retryCount--;

    } while(!data);

    /* read length of MDI pair */
    if((retVal = hwReadPagedPhyReg(dev,hwPort,0xff,21,phyInfo->anyPage,&data)) != GT_OK)
    {
        DBG_INFO(("Reading length of MDI pair failed.\n"));
        return retVal;
    }

    return dspLookup(phyInfo,data,cableLen);
}


/*******************************************************************************
* getDSPDistance_1240
*
* DESCRIPTION:
*       This routine returns cable length (meters) from DSP method.
*       This routine is for the 88E1181 like devices.
*
* INPUTS:
*       mdi - pair of each MDI (0..3).
*
* OUTPUTS:
*       cableLen - cable length (unit of meters).
*
* RETURNS:
*       GT_OK   - on success
*       GT_FAIL - on error
*
* COMMENTS:
*       None.
*
*******************************************************************************/
static
GT_STATUS getDSPDistance_1240
(
    IN  GT_QD_DEV *dev,
    IN  GT_U8  hwPort,
    IN    GT_PHY_INFO        *phyInfo,
    IN  GT_U32 mdi,
    OUT GT_32 *cableLen
)
{
    GT_U16     data, retryCount;
    GT_STATUS  retVal;

    DBG_INFO(("getDSPDistance Called.\n"));

    /* Set the required bits for Cable length register */
    if((retVal = hwWritePagedPhyReg(dev,hwPort,0xff,16,phyInfo->anyPage,(GT_U16)(0x1118+(0xff&mdi)))) != GT_OK)
    {
        DBG_INFO(("Writing to paged phy reg failed.\n"));
        return retVal;
    }

    retryCount = 1000;

    do
    {
        if(retryCount == 0)
        {
            DBG_INFO(("Ready bit of Cable length resiter is not set.\n"));
            return GT_FAIL;
        }

        /* Check the ready bit of Cable length register */
        if((retVal = hwGetPagedPhyRegField(dev,hwPort,0xff,16,15,1,phyInfo->anyPage,&data)) != GT_OK)
        {
            DBG_INFO(("Writing to paged phy reg failed.\n"));
            return retVal;
        }

        retryCount--;

    } while(!data);

    /* read length of MDI pair */
    if((retVal = hwReadPagedPhyReg(dev,hwPort,0xff,18,phyInfo->anyPage,&data)) != GT_OK)
    {
        DBG_INFO(("Reading length of MDI pair failed.\n"));
        return retVal;
    }

    return dspLookup(phyInfo,data,cableLen);
}



/*******************************************************************************
* getExStatus_28
*
* DESCRIPTION:
*       This routine retrieves Pair Skew, Pair Swap, and Pair Polarity
*        for 1000M phy with multiple page mode
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
*       None.
*
*******************************************************************************/
static GT_STATUS getExStatus_28
(
    IN  GT_QD_DEV         *dev,
    IN  GT_U8            hwPort,
    IN    GT_PHY_INFO        *phyInfo,
    OUT GT_ADV_EXTENDED_STATUS *extendedStatus
)
{
    GT_STATUS retVal;
    GT_U16 u16Data, i;

    extendedStatus->isValid = GT_FALSE;
    /* DSP based cable length */
    for (i=0; i<GT_MDI_PAIR_NUM; i++)
    {
        if((retVal = getDSPDistance_1111(dev,hwPort,phyInfo,i,(GT_32 *)&(extendedStatus->cableLen[i]))) != GT_OK)
        {
            DBG_INFO(("getDSPDistance failed.\n"));
            return retVal;
        }
    }


    /*
     * get data from 28_5 register for pair swap
     */
    if((retVal = hwReadPagedPhyReg(
                    dev,hwPort,5,28,phyInfo->anyPage,&u16Data)) != GT_OK)
    {
        DBG_INFO(("Reading from paged phy reg failed.\n"));
        return retVal;
    }

    /* if bit 6 is not set, it's not valid. */
    if (!(u16Data & 0x40))
    {
        DBG_INFO(("Valid Bit is not set (%0#x).\n", u16Data));
        return GT_OK;
    }

    extendedStatus->isValid = GT_TRUE;

    /* get Pair Polarity */
    for(i=0; i<GT_MDI_PAIR_NUM; i++)
    {
        switch((u16Data >> i) & 0x1)
        {
            case 0:
                extendedStatus->pairPolarity[i] = GT_POSITIVE;
                break;
            default:
                extendedStatus->pairPolarity[i] = GT_NEGATIVE;
            break;
        }
    }

    /* get Pair Swap for Channel A and B */
    if (u16Data & 0x10)
    {
        extendedStatus->pairSwap[0] = GT_CHANNEL_A;
        extendedStatus->pairSwap[1] = GT_CHANNEL_B;
    }
    else
    {
        extendedStatus->pairSwap[0] = GT_CHANNEL_B;
        extendedStatus->pairSwap[1] = GT_CHANNEL_A;
    }

    /* get Pair Swap for Channel C and D */
    if (u16Data & 0x20)
    {
        extendedStatus->pairSwap[2] = GT_CHANNEL_C;
        extendedStatus->pairSwap[3] = GT_CHANNEL_D;
    }
    else
    {
        extendedStatus->pairSwap[2] = GT_CHANNEL_D;
        extendedStatus->pairSwap[3] = GT_CHANNEL_C;
    }

    /*
     * get data from 28_4 register for pair skew
     */
    if((retVal = hwReadPagedPhyReg(
                    dev,hwPort,4,28,phyInfo->anyPage,&u16Data)) != GT_OK)
    {
        DBG_INFO(("Reading from paged phy reg failed.\n"));
        return retVal;
    }

    /* get Pair Skew */
    for(i=0; i<GT_MDI_PAIR_NUM; i++)
    {
        extendedStatus->pairSkew[i] = ((u16Data >> i*4) & 0xF) * 8;
    }

    return GT_OK;
}


/*******************************************************************************
* getExStatus
*
* DESCRIPTION:
*       This routine retrieves Pair Skew, Pair Swap, and Pair Polarity
*        for 1000M phy with multiple page mode
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
*       None.
*
*******************************************************************************/
static GT_STATUS getExStatus
(
    IN  GT_QD_DEV         *dev,
    IN  GT_U8            hwPort,
    IN    GT_PHY_INFO        *phyInfo,
    OUT GT_ADV_EXTENDED_STATUS *extendedStatus
)
{
    GT_STATUS retVal;
    GT_U16 u16Data, i;

    extendedStatus->isValid = GT_FALSE;
    /* DSP based cable length */
    switch(phyInfo->exStatusType)
    {
        case GT_PHY_EX_STATUS_TYPE1:
        case GT_PHY_EX_STATUS_TYPE2:
            for (i=0; i<GT_MDI_PAIR_NUM; i++)
            {
                if((retVal = getDSPDistance_1111(dev,hwPort,phyInfo,i,(GT_32 *)&extendedStatus->cableLen[i])) != GT_OK)
                {
                    DBG_INFO(("getDSPDistance failed.\n"));
                    return retVal;
                }
            }
            break;
        case GT_PHY_EX_STATUS_TYPE3:
        case GT_PHY_EX_STATUS_TYPE4:
        case GT_PHY_EX_STATUS_TYPE5:
            for (i=0; i<GT_MDI_PAIR_NUM; i++)
            {
                if((retVal = getDSPDistance_1181(dev,hwPort,phyInfo,i,(GT_32 *)&extendedStatus->cableLen[i])) != GT_OK)
                {
                    DBG_INFO(("getDSPDistance failed.\n"));
                    return retVal;
                }
            }
            break;

        case GT_PHY_EX_STATUS_TYPE6:
            for (i=0; i<GT_MDI_PAIR_NUM; i++)
            {
                if((retVal = getDSPDistance_1240(dev,hwPort,phyInfo,i,(GT_32 *)&extendedStatus->cableLen[i])) != GT_OK)
                {
                    DBG_INFO(("getDSPDistance failed.\n"));
                    return retVal;
                }
            }
            break;

        default:
            return GT_NOT_SUPPORTED;
    }

    /*
     * get data from 21_5 register for pair swap
     */
    if((retVal = hwReadPagedPhyReg(
                    dev,hwPort,5,QD_REG_PAIR_SWAP_STATUS,phyInfo->anyPage,&u16Data)) != GT_OK)
    {
        DBG_INFO(("Reading from paged phy reg failed.\n"));
        return retVal;
    }

    /* if bit 6 is not set, it's not valid. */
    if (!(u16Data & 0x40))
    {
        DBG_INFO(("Valid Bit is not set (%0#x).\n", u16Data));
        return GT_OK;
    }

    extendedStatus->isValid = GT_TRUE;

    /* get Pair Polarity */
    for(i=0; i<GT_MDI_PAIR_NUM; i++)
    {
        switch((u16Data >> i) & 0x1)
        {
            case 0:
                extendedStatus->pairPolarity[i] = GT_POSITIVE;
                break;
            default:
                extendedStatus->pairPolarity[i] = GT_NEGATIVE;
            break;
        }
    }

    /* get Pair Swap for Channel A and B */
    if (u16Data & 0x10)
    {
        extendedStatus->pairSwap[0] = GT_CHANNEL_A;
        extendedStatus->pairSwap[1] = GT_CHANNEL_B;
    }
    else
    {
        extendedStatus->pairSwap[0] = GT_CHANNEL_B;
        extendedStatus->pairSwap[1] = GT_CHANNEL_A;
    }

    /* get Pair Swap for Channel C and D */
    if (u16Data & 0x20)
    {
        extendedStatus->pairSwap[2] = GT_CHANNEL_C;
        extendedStatus->pairSwap[3] = GT_CHANNEL_D;
    }
    else
    {
        extendedStatus->pairSwap[2] = GT_CHANNEL_D;
        extendedStatus->pairSwap[3] = GT_CHANNEL_C;
    }

    /*
     * get data from 20_5 register for pair skew
     */
    if((retVal = hwReadPagedPhyReg(
                    dev,hwPort,5,QD_REG_PAIR_SKEW_STATUS,phyInfo->anyPage,&u16Data)) != GT_OK)
    {
        DBG_INFO(("Reading from paged phy reg failed.\n"));
        return retVal;
    }

    /* get Pair Skew */
    for(i=0; i<GT_MDI_PAIR_NUM; i++)
    {
        extendedStatus->pairSkew[i] = ((u16Data >> i*4) & 0xF) * 8;
    }


    return GT_OK;
}


/*******************************************************************************
* gvctGetAdvExtendedStatus
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
GT_STATUS gvctGetAdvExtendedStatus
(
    IN  GT_QD_DEV     *dev,
    IN  GT_LPORT   port,
    OUT GT_ADV_EXTENDED_STATUS *extendedStatus
)
{
    GT_STATUS retVal;
    GT_U8 hwPort;
    GT_BOOL ppuEn;
    GT_PHY_INFO    phyInfo;
    GT_BOOL            autoOn;
    GT_U16            pageReg;

#ifdef GT_USE_MAD
    if (dev->use_mad==GT_TRUE)
        return gvctGetAdvExtendedStatus_mad(dev, port, extendedStatus);
#endif

    DBG_INFO(("gvctGetAdvExtendedStatus Called.\n"));
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

    if(driverPagedAccessStart(dev,hwPort,phyInfo.pageType,&autoOn,&pageReg) != GT_OK)
    {
        gtSemGive(dev,dev->phyRegsSem);
        return GT_FAIL;
    }

    switch(phyInfo.exStatusType)
    {
        case GT_PHY_EX_STATUS_TYPE1:
            if((retVal = getExStatus_28(dev,hwPort,&phyInfo,extendedStatus)) != GT_OK)
            {
                DBG_INFO(("Getting Extanded Cable Status failed.\n"));
                break;
            }
            break;

        case GT_PHY_EX_STATUS_TYPE2:
        case GT_PHY_EX_STATUS_TYPE3:
        case GT_PHY_EX_STATUS_TYPE4:
        case GT_PHY_EX_STATUS_TYPE5:
        case GT_PHY_EX_STATUS_TYPE6:
            if((retVal = getExStatus(dev,hwPort,&phyInfo,extendedStatus)) != GT_OK)
            {
                DBG_INFO(("Getting Extanded Cable Status failed.\n"));
                break;
            }

            break;
        default:
            retVal = GT_NOT_SUPPORTED;
    }

        gtSemGive(dev,dev->phyRegsSem);
    return retVal;
}
