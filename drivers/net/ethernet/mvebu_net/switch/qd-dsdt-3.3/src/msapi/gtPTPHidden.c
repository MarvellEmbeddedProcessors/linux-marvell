/* Hidden APIs */


/*******************************************************************************
* gtaiGetSocClkPer
*
* DESCRIPTION:
*         SoC clock period
*        This specifies clock period for the clock that gets generated from the
*        PTP block to the reset of the SoC. The period is specified in TSClkPer
*        increments
*
* INPUTS:
*         None.
*
* OUTPUTS:
*        clkPer    - clock period
*
* RETURNS:
*         GT_OK      - on success
*         GT_FAIL    - on error
*         GT_NOT_SUPPORTED - if current device does not support this feature.
*
* COMMENTS:
*         None
*
*******************************************************************************/
GT_STATUS gtaiGetSocClkPer
(
    IN  GT_QD_DEV     *dev,
    OUT GT_U32        *clkPer
)
{
    GT_STATUS           retVal;
    GT_PTP_OPERATION    op;
    GT_PTP_OP_DATA        opData;

    DBG_INFO(("gtaiGetTimeIncAmt Called.\n"));

#ifndef CONFIG_AVB_FPGA
    /* check if device supports this feature */
    if (!IS_IN_DEV_GROUP(dev,DEV_TAI))
    {
        DBG_INFO(("GT_NOT_SUPPORTED\n"));
        return GT_NOT_SUPPORTED;
    }
#endif

    opData.ptpBlock = 0x0;    /* PTP register space */

    opData.ptpPort = 0xE;    /* TAI register */
    op = PTP_READ_DATA;

    opData.ptpAddr = 6;

    if((retVal = ptpOperationPerform(dev, op, &opData)) != GT_OK)
    {
        DBG_INFO(("Failed reading TAI register.\n"));
        return GT_FAIL;
    }

    *clkPer = (GT_U32)(opData.ptpData & 0x1FF);

    DBG_INFO(("OK.\n"));
    return GT_OK;
}


/*******************************************************************************
* gtaiSetSocClkPer
*
* DESCRIPTION:
*         SoC clock period
*        This specifies clock period for the clock that gets generated from the
*        PTP block to the reset of the SoC. The period is specified in TSClkPer
*        increments
*
* INPUTS:
*        clkPer    - clock period
*
* OUTPUTS:
*         None.
*
* RETURNS:
*         GT_OK      - on success
*         GT_FAIL    - on error
*         GT_NOT_SUPPORTED - if current device does not support this feature.
*
* COMMENTS:
*         None
*
*******************************************************************************/
GT_STATUS gtaiSetSocClkPer
(
    IN  GT_QD_DEV     *dev,
    IN  GT_U32        clkPer
)
{
    GT_STATUS           retVal;
    GT_PTP_OPERATION    op;
    GT_PTP_OP_DATA        opData;

    DBG_INFO(("gtaiSetSocClkPer Called.\n"));

#ifndef CONFIG_AVB_FPGA
    /* check if device supports this feature */
    if (!IS_IN_DEV_GROUP(dev,DEV_TAI))
    {
        DBG_INFO(("GT_NOT_SUPPORTED\n"));
        return GT_NOT_SUPPORTED;
    }
#endif

    opData.ptpBlock = 0x0;    /* PTP register space */

    opData.ptpPort = 0xE;    /* TAI register */
    op = PTP_READ_DATA;

    opData.ptpAddr = 6;

    if((retVal = ptpOperationPerform(dev, op, &opData)) != GT_OK)
    {
        DBG_INFO(("Failed reading TAI register.\n"));
        return GT_FAIL;
    }

    op = PTP_WRITE_DATA;
    opData.ptpData &= ~0x1FF;
    opData.ptpData |= (clkPer & 0x1FF);

    if((retVal = ptpOperationPerform(dev, op, &opData)) != GT_OK)
    {
        DBG_INFO(("Failed writing TAI register.\n"));
        return GT_FAIL;
    }

    DBG_INFO(("OK.\n"));
    return GT_OK;
}


/*******************************************************************************
* gtaiGetSocClkComp
*
* DESCRIPTION:
*        Soc clock compensation amount in pico seconds.
*        This field specifies the remainder amount for when the clock is being
*        generated with a period specifed by the clkPer. The hardware logic keeps
*        track of the remainder for every clock tick generation and compensates for it.
*
* INPUTS:
*         None.
*
* OUTPUTS:
*        amount    - clock compensation amount in pico seconds
*
* RETURNS:
*         GT_OK      - on success
*         GT_FAIL    - on error
*         GT_NOT_SUPPORTED - if current device does not support this feature.
*
* COMMENTS:
*         None
*
*******************************************************************************/
GT_STATUS gtaiGetSocClkComp
(
    IN  GT_QD_DEV     *dev,
    OUT GT_U32        *amount
)
{
    GT_STATUS           retVal;
    GT_PTP_OPERATION    op;
    GT_PTP_OP_DATA        opData;

    DBG_INFO(("gtaiGetSocClkComp Called.\n"));

#ifndef CONFIG_AVB_FPGA
    /* check if device supports this feature */
    if (!IS_IN_DEV_GROUP(dev,DEV_TAI))
    {
        DBG_INFO(("GT_NOT_SUPPORTED\n"));
        return GT_NOT_SUPPORTED;
    }
#endif

    opData.ptpBlock = 0x0;    /* PTP register space */

    opData.ptpPort = 0xE;    /* TAI register */
    op = PTP_READ_DATA;

    opData.ptpAddr = 7;

    if((retVal = ptpOperationPerform(dev, op, &opData)) != GT_OK)
    {
        DBG_INFO(("Failed reading TAI register.\n"));
        return GT_FAIL;
    }

    *amount = (GT_U32)opData.ptpData;

    DBG_INFO(("OK.\n"));
    return GT_OK;
}


/*******************************************************************************
* gtaiSetSocClkComp
*
* DESCRIPTION:
*        Soc clock compensation amount in pico seconds.
*        This field specifies the remainder amount for when the clock is being
*        generated with a period specifed by the clkPer. The hardware logic keeps
*        track of the remainder for every clock tick generation and compensates for it.
*
* INPUTS:
*        amount    - clock compensation amount in pico seconds
*
* OUTPUTS:
*         None.
*
* RETURNS:
*         GT_OK      - on success
*         GT_FAIL    - on error
*         GT_NOT_SUPPORTED - if current device does not support this feature.
*
* COMMENTS:
*         None
*
*******************************************************************************/
GT_STATUS gtaiSetSocClkComp
(
    IN  GT_QD_DEV     *dev,
    IN  GT_U32        amount
)
{
    GT_STATUS           retVal;
    GT_PTP_OPERATION    op;
    GT_PTP_OP_DATA        opData;

    DBG_INFO(("gtaiSetSocClkComp Called.\n"));

#ifndef CONFIG_AVB_FPGA
    /* check if device supports this feature */
    if (!IS_IN_DEV_GROUP(dev,DEV_TAI))
    {
        DBG_INFO(("GT_NOT_SUPPORTED\n"));
        return GT_NOT_SUPPORTED;
    }
#endif

    opData.ptpBlock = 0x0;    /* PTP register space */

    opData.ptpPort = 0xE;    /* TAI register */
    op = PTP_WRITE_DATA;

    opData.ptpAddr = 7;

    opData.ptpData = (GT_U16)amount;

    if((retVal = ptpOperationPerform(dev, op, &opData)) != GT_OK)
    {
        DBG_INFO(("Failed writing TAI register.\n"));
        return GT_FAIL;
    }

    DBG_INFO(("OK.\n"));
    return GT_OK;
}
