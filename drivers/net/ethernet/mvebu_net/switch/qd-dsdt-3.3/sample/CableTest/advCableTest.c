#include <Copyright.h>
/********************************************************************************
* testApi.c
*
* DESCRIPTION:
*       API test functions
*
* DEPENDENCIES:   Platform.
*
* FILE REVISION NUMBER:
*
*******************************************************************************/
#include "msSample.h"

GT_STATUS advVctTest(GT_QD_DEV *dev, GT_LPORT port);
GT_STATUS getAdvExtendedStatus(GT_QD_DEV *dev, GT_LPORT port);

void displayAdvVCTResult
(
    GT_ADV_CABLE_STATUS *cableStatus,
    int    channel
)
{
    int i;

    switch(cableStatus->cableStatus[channel])
    {
        case GT_ADV_VCT_FAIL:
            MSG_PRINT(("Advanced Cable Test Failed\n"));
            break;
        case GT_ADV_VCT_NORMAL:
            MSG_PRINT(("Cable Test Passed. No problem found.\n"));
            break;
        case GT_ADV_VCT_IMP_GREATER_THAN_115:
            MSG_PRINT(("Cable Test Passed. Impedance is greater than 115 Ohms.\n"));
            MSG_PRINT(("Approximatly %i meters from the tested port.\n",cableStatus->u[channel].dist2fault));
            break;
        case GT_ADV_VCT_IMP_LESS_THAN_85:
            MSG_PRINT(("Cable Test Passed. Impedance is less than 85 Ohms.\n"));
            MSG_PRINT(("Approximatly %i meters from the tested port.\n",cableStatus->u[channel].dist2fault));
            break;
        case GT_ADV_VCT_OPEN:
            MSG_PRINT(("Cable Test Passed. Open Cable.\n"));
            MSG_PRINT(("Approximatly %i meters from the tested port.\n",cableStatus->u[channel].dist2fault));
            break;
        case GT_ADV_VCT_SHORT:
            MSG_PRINT(("Cable Test Passed. Shorted Cable.\n"));
            MSG_PRINT(("Approximatly %i meters from the tested port.\n",cableStatus->u[channel].dist2fault));
            break;
        case GT_ADV_VCT_CROSS_PAIR_SHORT:
            MSG_PRINT(("Cable Test Passed.\n"));
            for(i=0; i<GT_MDI_PAIR_NUM; i++)
            {
                if(cableStatus->u[channel].crossShort.channel[i] == GT_TRUE)
                {
                    MSG_PRINT(("\tCross pair short with channel %i.\n",i));
                    MSG_PRINT(("\tApproximatly %i meters from the tested port.\n",
                                    cableStatus->u[channel].crossShort.dist2fault[i]));
                }
            }
            break;
        default:
            MSG_PRINT(("Unknown Test Result.\n"));
            break;
    }
}

/* Advanced VCT (TDR) */
GT_STATUS advVctTest(GT_QD_DEV *dev, GT_LPORT port)
{
    GT_STATUS status;
    int i, j;
    GT_ADV_VCT_MODE mode;
    GT_ADV_CABLE_STATUS advCableStatus;

    GT_ADV_VCT_MOD mod[2] = {
        GT_ADV_VCT_FIRST_PEAK,
        GT_ADV_VCT_MAX_PEAK
    };

    char modeStr[2][32] = {
        "(Adv VCT First PEAK)",
        "(Adv VCT MAX PEAK)"
    };
printf("!!!! sample adv Cable Test Result for Port %i\n",port);

    if (dev == 0)
    {
        MSG_PRINT(("GT driver is not initialized\n"));
        return GT_FAIL;
    }

    for (j=0; j<2; j++)
    {
        mode.mode=mod[j];
        mode.transChanSel=GT_ADV_VCT_TCS_NO_CROSSPAIR;
        mode.sampleAvg = 0;
        mode.peakDetHyst =0;

        /*
         *    Start and get Cable Test Result
         */
        status = GT_OK;
printf("!!!! 1 sample adv Cable Test Result for Port %i\n",port);

        if((status = gvctGetAdvCableDiag(dev,port,
                                mode,&advCableStatus)) != GT_OK)
        {
            MSG_PRINT(("gvctGetAdvCableDiag return Failed\n"));
            return status;
        }

        MSG_PRINT(("\nCable Test Result %s for Port %i\n", modeStr[j], (int)port));

        for(i=0; i<GT_MDI_PAIR_NUM; i++)
        {
            MSG_PRINT(("MDI PAIR %i:\n",i));
            displayAdvVCTResult(&advCableStatus, i);
        }
    }

    return GT_OK;
}

/* Advanced DSP VCT */
GT_STATUS getAdvExtendedStatus(GT_QD_DEV *dev, GT_LPORT port)
{
    GT_STATUS status;
    GT_ADV_EXTENDED_STATUS extendedStatus;
    int i;
    char ch;

    if (dev == 0)
    {
        MSG_PRINT(("GT driver is not initialized\n"));
        return GT_FAIL;
    }

    /*
     *     Start getting Extended Information.
     */
    if((status = gvctGetAdvExtendedStatus(dev,port, &extendedStatus)) != GT_OK)
    {
        MSG_PRINT(("gvctGetAdvExtendedStatus return Failed\n"));
        return status;
    }

    if (!extendedStatus.isValid)
    {
        MSG_PRINT(("Not able to get Extended Status.\n"));
        MSG_PRINT(("Please check if 1000B-T Link is established on Port %i.\n",(int)port));
        return status;
    }

    /* Pair Polarity */
    MSG_PRINT(("Pair Polarity:\n"));
    for(i=0; i<GT_MDI_PAIR_NUM; i++)
    {
        MSG_PRINT(("MDI PAIR %i: %s\n",i,
                    (extendedStatus.pairPolarity[i] == GT_POSITIVE)?"Positive":"Negative"));
    }

    /* Pair Swap */
    MSG_PRINT(("Pair Swap:\n"));
    for(i=0; i<GT_MDI_PAIR_NUM; i++)
    {
        switch(extendedStatus.pairSwap[i])
        {
            case GT_CHANNEL_A:
                ch = 'A';
                break;
            case GT_CHANNEL_B:
                ch = 'B';
                break;
            case GT_CHANNEL_C:
                ch = 'C';
                break;
            case GT_CHANNEL_D:
                ch = 'D';
                break;
            default:
                MSG_PRINT(("Error: reported unknown Pair Swap %i\n",extendedStatus.pairSwap[i]));
                ch = 'U';
                break;
        }

        MSG_PRINT(("MDI PAIR %i: Channel %c\n",i,ch));
    }

    /* Pair Polarity */
    MSG_PRINT(("Pair Skew:\n"));
    for(i=0; i<GT_MDI_PAIR_NUM; i++)
    {
        MSG_PRINT(("MDI PAIR %i: %ins\n",i,(int)extendedStatus.pairSkew[i]));
    }

    /* Pair Polarity */
    MSG_PRINT(("Cable Len:\n"));
    for(i=0; i<GT_MDI_PAIR_NUM; i++)
    {
        MSG_PRINT(("MDI PAIR %i: approximately %im\n",i,(int)extendedStatus.cableLen[i]));
    }

    return GT_OK;
}
