#include <Copyright.h>

/*******************************************************************************
* gtAVB.c
*
* DESCRIPTION:
*       API definitions for Precise Time Protocol logic
*
* DEPENDENCIES:
*
* FILE REVISION NUMBER:
*******************************************************************************/

#include <msApi.h>
#include <gtSem.h>
#include <gtHwCntl.h>
#include <gtDrvSwRegs.h>


#ifdef CONFIG_AVB_FPGA

#undef USE_SINGLE_READ

#define AVB_SMI_ADDR        0xC

#define QD_REG_PTP_INT_OFFSET        0
#define QD_REG_PTP_INTEN_OFFSET        1
#define QD_REG_PTP_FREQ_OFFSET        4
#define QD_REG_PTP_PHASE_OFFSET        6
#define QD_REG_PTP_CLK_CTRL_OFFSET    4
#define QD_REG_PTP_CYCLE_INTERVAL_OFFSET        5
#define QD_REG_PTP_CYCLE_ADJ_OFFSET                6
#define QD_REG_PTP_PLL_CTRL_OFFSET    7
#define QD_REG_PTP_CLK_SRC_OFFSET    0x9
#define QD_REG_PTP_P9_MODE_OFFSET    0xA
#define QD_REG_PTP_RESET_OFFSET        0xB

#define GT_PTP_MERGE_32BIT(_high16,_low16)    (((_high16)<<16)|(_low16))
#define GT_PTP_GET_HIGH16(_data)    ((_data) >> 16) & 0xFFFF
#define GT_PTP_GET_LOW16(_data)        (_data) & 0xFFFF

#if 0

#define AVB_FPGA_READ_REG       gprtGetPhyReg
#define AVB_FPGA_WRITE_REG      gprtSetPhyReg
unsigned int (*avbFpgaReadReg)(void* unused, unsigned int port, unsigned int reg, unsigned int* data);
unsigned int (*avbFpgaWriteReg)(void* unused, unsigned int port, unsigned int reg, unsigned int data);
#else

/* for RMGMT access  and can be controlled by <sw_apps -rmgmt 0/1> */
unsigned int (*avbFpgaReadReg)(void* unused, unsigned int port, unsigned int reg, GT_U32* data)=gprtGetPhyReg;
unsigned int (*avbFpgaWriteReg)(void* unused, unsigned int port, unsigned int reg, GT_U32 data)=gprtSetPhyReg;
#define AVB_FPGA_READ_REG       avbFpgaReadReg
#define AVB_FPGA_WRITE_REG      avbFpgaWriteReg

#endif /* 0 */

#endif

#if 0
#define GT_PTP_BUILD_TIME(_time1, _time2)    (((_time1) << 16) | (_time2))
#define GT_PTP_L16_TIME(_time1)    ((_time1) & 0xFFFF)
#define GT_PTP_H16_TIME(_time1)    (((_time1) >> 16) & 0xFFFF)
#endif


/****************************************************************************/
/* PTP operation function declaration.                                    */
/****************************************************************************/
extern GT_STATUS ptpOperationPerform
(
    IN   GT_QD_DEV             *dev,
    IN   GT_PTP_OPERATION    ptpOp,
    INOUT GT_PTP_OP_DATA     *opData
);


/*******************************************************************************
* gavbGetPriority
*
* DESCRIPTION:
*        Priority overwrite.
*        Supported priority type is defined as GT_AVB_PRI_TYPE.
*        Priority is either 3 bits or 2 bits depending on priority type.
*            GT_AVB_HI_FPRI        - priority is 0 ~ 7
*            GT_AVB_HI_QPRI        - priority is 0 ~ 3
*            GT_AVB_LO_FPRI        - priority is 0 ~ 7
*            GT_AVB_LO_QPRI        - priority is 0 ~ 3
*            GT_LEGACY_HI_FPRI    - priority is 0 ~ 7
*            GT_LEGACY_HI_QPRI    - priority is 0 ~ 3
*            GT_LEGACY_LO_FPRI    - priority is 0 ~ 7
*            GT_LEGACY_LO_QPRI    - priority is 0 ~ 3
*
* INPUTS:
*         priType    - GT_AVB_PRI_TYPE
*
* OUTPUTS:
*        pri    - priority
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
GT_STATUS gavbGetPriority
(
    IN  GT_QD_DEV     *dev,
    IN  GT_AVB_PRI_TYPE        priType,
    OUT GT_U32        *pri
)
{
    GT_STATUS           retVal;
    GT_PTP_OPERATION    op;
    GT_PTP_OP_DATA        opData;
    GT_U16        mask, reg, bitPos;

    DBG_INFO(("gavbGetPriority Called.\n"));

#ifndef CONFIG_AVB_FPGA
    /* check if device supports this feature */
    if (!IS_IN_DEV_GROUP(dev,DEV_AVB_POLICY))
    {
        DBG_INFO(("GT_NOT_SUPPORTED\n"));
        return GT_NOT_SUPPORTED;
    }
#endif

    switch (priType)
    {
        case GT_AVB_HI_FPRI:
            mask = 0x7;
            reg = 0;
            bitPos = 12;
            break;
        case GT_AVB_HI_QPRI:
            mask = 0x3;
            reg = 0;
            bitPos = 8;
            break;
        case GT_AVB_LO_FPRI:
            mask = 0x7;
            reg = 0;
            bitPos = 4;
            break;
        case GT_AVB_LO_QPRI:
            mask = 0x3;
            reg = 0;
            bitPos = 0;
            break;
        case GT_LEGACY_HI_FPRI:
            mask = 0x7;
            reg = 4;
            bitPos = 12;
            break;
        case GT_LEGACY_HI_QPRI:
            mask = 0x3;
            reg = 4;
            bitPos = 8;
            break;
        case GT_LEGACY_LO_FPRI:
            mask = 0x7;
            reg = 4;
            bitPos = 4;
            break;
        case GT_LEGACY_LO_QPRI:
            mask = 0x3;
            reg = 4;
            bitPos = 0;
            break;
        default:
            return GT_BAD_PARAM;
    }

    opData.ptpBlock = 0x1;    /* AVB Policy register space */

    opData.ptpPort = 0xF;    /* Global register */
    op = PTP_READ_DATA;

    opData.ptpAddr = reg;

    if((retVal = ptpOperationPerform(dev, op, &opData)) != GT_OK)
    {
        DBG_INFO(("Failed reading TAI register.\n"));
        return GT_FAIL;
    }

    *pri = (GT_U32)(opData.ptpData >> bitPos) & mask;

    DBG_INFO(("OK.\n"));
    return GT_OK;
}


/*******************************************************************************
* gavbSetPriority
*
* DESCRIPTION:
*        Priority overwrite.
*        Supported priority type is defined as GT_AVB_PRI_TYPE.
*        Priority is either 3 bits or 2 bits depending on priority type.
*            GT_AVB_HI_FPRI        - priority is 0 ~ 7
*            GT_AVB_HI_QPRI        - priority is 0 ~ 3
*            GT_AVB_LO_FPRI        - priority is 0 ~ 7
*            GT_AVB_LO_QPRI        - priority is 0 ~ 3
*            GT_LEGACY_HI_FPRI    - priority is 0 ~ 7
*            GT_LEGACY_HI_QPRI    - priority is 0 ~ 3
*            GT_LEGACY_LO_FPRI    - priority is 0 ~ 7
*            GT_LEGACY_LO_QPRI    - priority is 0 ~ 3
*
* INPUTS:
*         priType    - GT_AVB_PRI_TYPE
*        pri    - priority
*
* OUTPUTS:
*        None
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
GT_STATUS gavbSetPriority
(
    IN  GT_QD_DEV     *dev,
    IN  GT_AVB_PRI_TYPE        priType,
    IN  GT_U32        pri
)
{
    GT_STATUS           retVal;
    GT_PTP_OPERATION    op;
    GT_PTP_OP_DATA        opData;
    GT_U16        mask, reg, bitPos;

    DBG_INFO(("gavbSetPriority Called.\n"));

#ifndef CONFIG_AVB_FPGA
    /* check if device supports this feature */
    if (!IS_IN_DEV_GROUP(dev,DEV_AVB_POLICY))
    {
        DBG_INFO(("GT_NOT_SUPPORTED\n"));
        return GT_NOT_SUPPORTED;
    }
#endif

    switch (priType)
    {
        case GT_AVB_HI_FPRI:
            mask = 0x7;
            reg = 0;
            bitPos = 12;
            break;
        case GT_AVB_HI_QPRI:
            mask = 0x3;
            reg = 0;
            bitPos = 8;
            break;
        case GT_AVB_LO_FPRI:
            mask = 0x7;
            reg = 0;
            bitPos = 4;
            break;
        case GT_AVB_LO_QPRI:
            mask = 0x3;
            reg = 0;
            bitPos = 0;
            break;
        case GT_LEGACY_HI_FPRI:
            mask = 0x7;
            reg = 4;
            bitPos = 12;
            break;
        case GT_LEGACY_HI_QPRI:
            mask = 0x3;
            reg = 4;
            bitPos = 8;
            break;
        case GT_LEGACY_LO_FPRI:
            mask = 0x7;
            reg = 4;
            bitPos = 4;
            break;
        case GT_LEGACY_LO_QPRI:
            mask = 0x3;
            reg = 4;
            bitPos = 0;
            break;
        default:
            return GT_BAD_PARAM;
    }

    if (pri & (~mask))
    {
        return GT_BAD_PARAM;
    }

    opData.ptpBlock = 0x1;    /* AVB Policy register space */

    opData.ptpPort = 0xF;    /* Global register */
    op = PTP_READ_DATA;

    opData.ptpAddr = reg;

    if((retVal = ptpOperationPerform(dev, op, &opData)) != GT_OK)
    {
        DBG_INFO(("Failed reading TAI register.\n"));
        return GT_FAIL;
    }

    opData.ptpData &= ~(mask << bitPos);
    opData.ptpData |= (pri << bitPos);

    op = PTP_WRITE_DATA;

    if((retVal = ptpOperationPerform(dev, op, &opData)) != GT_OK)
    {
        DBG_INFO(("Failed writing TAI register.\n"));
        return GT_FAIL;
    }

    DBG_INFO(("OK.\n"));
    return GT_OK;
}



/*******************************************************************************
* gavbGetAVBHiLimit
*
* DESCRIPTION:
*        AVB Hi Frame Limit.
*        When these bits are zero, normal frame processing occurs.
*        When it's non-zero, they are used to define the maximum frame size allowed
*        for AVB frames that can be placed into the GT_AVB_HI_QPRI queue. Frames
*        that are over this size limit are filtered. The only exception to this
*        is non-AVB frames that get their QPriAvb assigned by the Priority Override
*        Table
*
* INPUTS:
*         None
*
* OUTPUTS:
*        limit    - Hi Frame Limit
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
GT_STATUS gavbGetAVBHiLimit
(
    IN  GT_QD_DEV     *dev,
    OUT GT_U32        *limit
)
{
    GT_STATUS           retVal;
    GT_PTP_OPERATION    op;
    GT_PTP_OP_DATA        opData;

    DBG_INFO(("gavbGetAVBHiLimit Called.\n"));

#ifndef CONFIG_AVB_FPGA
    /* check if device supports this feature */
    if (!IS_IN_DEV_GROUP(dev,DEV_AVB_POLICY))
    {
        DBG_INFO(("GT_NOT_SUPPORTED\n"));
        return GT_NOT_SUPPORTED;
    }
#endif

    opData.ptpBlock = 0x1;    /* AVB Policy register space */

    opData.ptpPort = 0xF;    /* Global register */
    op = PTP_READ_DATA;

    opData.ptpAddr = 8;

    if((retVal = ptpOperationPerform(dev, op, &opData)) != GT_OK)
    {
        DBG_INFO(("Failed reading DisPTP.\n"));
        return GT_FAIL;
    }

    *limit = (GT_U32)(opData.ptpData & 0x7FF);

    DBG_INFO(("OK.\n"));
    return GT_OK;
}


/*******************************************************************************
* gavbSetAVBHiLimit
*
* DESCRIPTION:
*        AVB Hi Frame Limit.
*        When these bits are zero, normal frame processing occurs.
*        When it's non-zero, they are used to define the maximum frame size allowed
*        for AVB frames that can be placed into the GT_AVB_HI_QPRI queue. Frames
*        that are over this size limit are filtered. The only exception to this
*        is non-AVB frames that get their QPriAvb assigned by the Priority Override
*        Table
*
* INPUTS:
*        limit    - Hi Frame Limit
*
* OUTPUTS:
*         None
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
GT_STATUS gavbSetAVBHiLimit
(
    IN  GT_QD_DEV     *dev,
    IN  GT_U32        limit
)
{
    GT_STATUS           retVal;
    GT_PTP_OPERATION    op;
    GT_PTP_OP_DATA        opData;

    DBG_INFO(("gavbSetAVBHiLimit Called.\n"));

#ifndef CONFIG_AVB_FPGA
    /* check if device supports this feature */
    if (!IS_IN_DEV_GROUP(dev,DEV_AVB_POLICY))
    {
        DBG_INFO(("GT_NOT_SUPPORTED\n"));
        return GT_NOT_SUPPORTED;
    }
#endif

    opData.ptpBlock = 0x1;    /* AVB Policy register space */

    opData.ptpPort = 0xF;    /* Global register */
    op = PTP_WRITE_DATA;

    opData.ptpAddr = 8;
    opData.ptpData = (GT_U16)limit;

    if((retVal = ptpOperationPerform(dev, op, &opData)) != GT_OK)
    {
        DBG_INFO(("Failed reading DisPTP.\n"));
        return GT_FAIL;
    }


    DBG_INFO(("OK.\n"));
    return GT_OK;
}


/*******************************************************************************
* gavbGetPtpExtClk
*
* DESCRIPTION:
*        PTP external clock select.
*        When this bit is cleared to a zero, the PTP core gets its clock from
*        an internal 125MHz clock based on the device's XTAL_IN input.
*        When this bit is set to a one, the PTP core gets its clock from the device's
*        PTP_EXTCLK pin.
*
* INPUTS:
*         None
*
* OUTPUTS:
*        extClk    - GT_TRUE if external clock is selected, GT_FALSE otherwise
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
GT_STATUS gavbGetPtpExtClk
(
    IN  GT_QD_DEV     *dev,
    OUT GT_BOOL        *extClk
)
{
    GT_STATUS           retVal;
    GT_PTP_OPERATION    op;
    GT_PTP_OP_DATA        opData;

    DBG_INFO(("gavbGetPtpExtClk Called.\n"));

#ifndef CONFIG_AVB_FPGA
    /* check if device supports this feature */
    if (!IS_IN_DEV_GROUP(dev,DEV_AVB_POLICY_RECOVER_CLK))
    {
        DBG_INFO(("GT_NOT_SUPPORTED\n"));
        return GT_NOT_SUPPORTED;
    }
#endif

    opData.ptpBlock = 0x1;    /* AVB Policy register space */

    opData.ptpPort = 0xF;    /* Global register */
    op = PTP_READ_DATA;

    opData.ptpAddr = 0xB;

    if((retVal = ptpOperationPerform(dev, op, &opData)) != GT_OK)
    {
        DBG_INFO(("Failed reading DisPTP.\n"));
        return GT_FAIL;
    }

    *extClk = (GT_U32)(opData.ptpData >> 15) & 0x1;

    DBG_INFO(("OK.\n"));
    return GT_OK;
}



/*******************************************************************************
* gavbSetPtpExtClk
*
* DESCRIPTION:
*        PTP external clock select.
*        When this bit is cleared to a zero, the PTP core gets its clock from
*        an internal 125MHz clock based on the device's XTAL_IN input.
*        When this bit is set to a one, the PTP core gets its clock from the device's
*        PTP_EXTCLK pin.
*
* INPUTS:
*        extClk    - GT_TRUE if external clock is selected, GT_FALSE otherwise
*
* OUTPUTS:
*         None
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
GT_STATUS gavbSetPtpExtClk
(
    IN  GT_QD_DEV     *dev,
    IN  GT_BOOL        extClk
)
{
    GT_STATUS           retVal;
    GT_PTP_OPERATION    op;
    GT_PTP_OP_DATA        opData;

    DBG_INFO(("gavbSetPtpExtClk Called.\n"));

#ifndef CONFIG_AVB_FPGA
    /* check if device supports this feature */
    if (!IS_IN_DEV_GROUP(dev,DEV_AVB_POLICY_RECOVER_CLK))
    {
        DBG_INFO(("GT_NOT_SUPPORTED\n"));
        return GT_NOT_SUPPORTED;
    }
#endif

    opData.ptpBlock = 0x1;    /* AVB Policy register space */

    opData.ptpPort = 0xF;    /* Global register */
    op = PTP_READ_DATA;

    opData.ptpAddr = 0xB;

    if((retVal = ptpOperationPerform(dev, op, &opData)) != GT_OK)
    {
        DBG_INFO(("Failed reading DisPTP.\n"));
        return GT_FAIL;
    }

    if(extClk)
        opData.ptpData |= 0x8000;
    else
        opData.ptpData &= ~0x8000;

    op = PTP_WRITE_DATA;

    if((retVal = ptpOperationPerform(dev, op, &opData)) != GT_OK)
    {
        DBG_INFO(("Failed reading DisPTP.\n"));
        return GT_FAIL;
    }

    DBG_INFO(("OK.\n"));
    return GT_OK;
}


/*******************************************************************************
* gavbGetRecClkSel
*
* DESCRIPTION:
*        Synchronous Ethernet Recovered Clock Select.
*        This field indicate the internal PHY number whose recovered clock will
*        be presented on the SE_RCLK0 or SE_RCLK1 pin depending on the recClk selection.
*
* INPUTS:
*        recClk    - GT_AVB_RECOVERED_CLOCK type
*
* OUTPUTS:
*        clkSel    - recovered clock selection
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
GT_STATUS gavbGetRecClkSel
(
    IN  GT_QD_DEV     *dev,
    IN  GT_AVB_RECOVERED_CLOCK    recClk,
    OUT GT_U32        *clkSel
)
{
    GT_STATUS           retVal;
    GT_PTP_OPERATION    op;
    GT_PTP_OP_DATA        opData;
    GT_U16        bitPos;

    DBG_INFO(("gavbGetRecClkSel Called.\n"));

#ifndef CONFIG_AVB_FPGA
    /* check if device supports this feature */
    if (!IS_IN_DEV_GROUP(dev,DEV_AVB_POLICY))
    {
        DBG_INFO(("GT_NOT_SUPPORTED\n"));
        return GT_NOT_SUPPORTED;
    }
    if (!IS_IN_DEV_GROUP(dev,DEV_AVB_POLICY_RECOVER_CLK))
    {
        DBG_INFO(("GT_NOT_SUPPORTED\n"));
        return GT_NOT_SUPPORTED;
    }
#endif

    switch (recClk)
    {
        case GT_PRIMARY_RECOVERED_CLOCK:
            bitPos = 0;
            break;
        case GT_SECONDARY_RECOVERED_CLOCK:
            bitPos = 4;
            break;
        default:
            return GT_BAD_PARAM;
    }

    opData.ptpBlock = 0x1;    /* AVB Policy register space */

    opData.ptpPort = 0xF;    /* Global register */
    op = PTP_READ_DATA;

    opData.ptpAddr = 0xB;

    if((retVal = ptpOperationPerform(dev, op, &opData)) != GT_OK)
    {
        DBG_INFO(("Failed reading DisPTP.\n"));
        return GT_FAIL;
    }

    *clkSel = (GT_U32)(opData.ptpData >> bitPos) & 0x7;

    DBG_INFO(("OK.\n"));
    return GT_OK;
}


/*******************************************************************************
* gavbSetRecClkSel
*
* DESCRIPTION:
*        Synchronous Ethernet Recovered Clock Select.
*        This field indicate the internal PHY number whose recovered clock will
*        be presented on the SE_RCLK0 or SE_RCLK1 pin depending on the recClk selection.
*
* INPUTS:
*        recClk    - GT_AVB_RECOVERED_CLOCK type
*        clkSel    - recovered clock selection (should be less than 8)
*
* OUTPUTS:
*        None
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
GT_STATUS gavbSetRecClkSel
(
    IN  GT_QD_DEV     *dev,
    IN  GT_AVB_RECOVERED_CLOCK    recClk,
    IN  GT_U32        clkSel
)
{
    GT_STATUS           retVal;
    GT_PTP_OPERATION    op;
    GT_PTP_OP_DATA        opData;
    GT_U16        bitPos;

    DBG_INFO(("gavbSetRecClkSel Called.\n"));

#ifndef CONFIG_AVB_FPGA
    /* check if device supports this feature */
    if (!IS_IN_DEV_GROUP(dev,DEV_AVB_POLICY))
    {
        DBG_INFO(("GT_NOT_SUPPORTED\n"));
        return GT_NOT_SUPPORTED;
    }
    if (!IS_IN_DEV_GROUP(dev,DEV_AVB_POLICY_RECOVER_CLK))
    {
        DBG_INFO(("GT_NOT_SUPPORTED\n"));
        return GT_NOT_SUPPORTED;
    }
#endif

    if (clkSel > 0x7)
        return GT_BAD_PARAM;

    switch (recClk)
    {
        case GT_PRIMARY_RECOVERED_CLOCK:
            bitPos = 0;
            break;
        case GT_SECONDARY_RECOVERED_CLOCK:
            bitPos = 4;
            break;
        default:
            return GT_BAD_PARAM;
    }

    opData.ptpBlock = 0x1;    /* AVB Policy register space */

    opData.ptpPort = 0xF;    /* Global register */
    op = PTP_READ_DATA;

    opData.ptpAddr = 0xB;

    if((retVal = ptpOperationPerform(dev, op, &opData)) != GT_OK)
    {
        DBG_INFO(("Failed reading DisPTP.\n"));
        return GT_FAIL;
    }

    opData.ptpData &= ~(0x7 << bitPos);
    opData.ptpData |= clkSel << bitPos;

    op = PTP_WRITE_DATA;

    if((retVal = ptpOperationPerform(dev, op, &opData)) != GT_OK)
    {
        DBG_INFO(("Failed reading DisPTP.\n"));
        return GT_FAIL;
    }

    DBG_INFO(("OK.\n"));
    return GT_OK;
}


/*******************************************************************************
* gavbGetAvbOuiBytes
*
* DESCRIPTION:
*        AVB OUI Limit Filter bytes(0 ~ 2).
*        When all three of the AvbOui Bytes are zero, normal frame processing occurs.
*        When any of the three AvbOui Bytes are non-zero, all AVB frames must have a
*        destination address whose 1st three bytes of the DA match these three
*        AvbOui Bytes or the frame will be filtered.
*
* INPUTS:
*         None
*
* OUTPUTS:
*        ouiBytes    - 3 bytes of OUI field in Ethernet address format
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
GT_STATUS gavbGetAvbOuiBytes
(
    IN  GT_QD_DEV     *dev,
    OUT GT_U8        *obiBytes
)
{
    GT_STATUS           retVal;
    GT_PTP_OPERATION    op;
    GT_PTP_OP_DATA        opData;

    DBG_INFO(("gavbGetAvbOuiBytes Called.\n"));

#ifndef CONFIG_AVB_FPGA
    /* check if device supports this feature */
    if (!IS_IN_DEV_GROUP(dev,DEV_AVB_POLICY))
    {
        DBG_INFO(("GT_NOT_SUPPORTED\n"));
        return GT_NOT_SUPPORTED;
    }
#endif

    opData.ptpBlock = 0x1;    /* AVB Policy register space */

    opData.ptpPort = 0xF;    /* Global register */
    op = PTP_READ_DATA;

    opData.ptpAddr = 0xC;

    if((retVal = ptpOperationPerform(dev, op, &opData)) != GT_OK)
    {
        DBG_INFO(("Failed reading DisPTP.\n"));
        return GT_FAIL;
    }

    obiBytes[0] = (GT_U8)((opData.ptpData >> 8) & 0xFF);
    obiBytes[1] = (GT_U8)(opData.ptpData & 0xFF);

    opData.ptpAddr = 0xD;

    if((retVal = ptpOperationPerform(dev, op, &opData)) != GT_OK)
    {
        DBG_INFO(("Failed reading DisPTP.\n"));
        return GT_FAIL;
    }

    obiBytes[2] = (GT_U8)((opData.ptpData >> 8) & 0xFF);

    DBG_INFO(("OK.\n"));
    return GT_OK;
}

/*******************************************************************************
* gavbSetAvbOuiBytes
*
* DESCRIPTION:
*        AVB OUI Limit Filter bytes(0 ~ 2).
*        When all three of the AvbOui Bytes are zero, normal frame processing occurs.
*        When any of the three AvbOui Bytes are non-zero, all AVB frames must have a
*        destination address whose 1st three bytes of the DA match these three
*        AvbOui Bytes or the frame will be filtered.
*
* INPUTS:
*        ouiBytes    - 3 bytes of OUI field in Ethernet address format
*
* OUTPUTS:
*         None
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
GT_STATUS gavbSetAvbOuiBytes
(
    IN  GT_QD_DEV     *dev,
    IN  GT_U8        *obiBytes
)
{
    GT_STATUS           retVal;
    GT_PTP_OPERATION    op;
    GT_PTP_OP_DATA        opData;

    DBG_INFO(("gavbSetAvbOuiBytes Called.\n"));

#ifndef CONFIG_AVB_FPGA
    /* check if device supports this feature */
    if (!IS_IN_DEV_GROUP(dev,DEV_AVB_POLICY))
    {
        DBG_INFO(("GT_NOT_SUPPORTED\n"));
        return GT_NOT_SUPPORTED;
    }
#endif

    opData.ptpBlock = 0x1;    /* AVB Policy register space */

    opData.ptpPort = 0xF;    /* Global register */
    op = PTP_WRITE_DATA;

    opData.ptpAddr = 0xC;

    opData.ptpData = (obiBytes[0] << 8) | obiBytes[1];

    if((retVal = ptpOperationPerform(dev, op, &opData)) != GT_OK)
    {
        DBG_INFO(("Failed reading DisPTP.\n"));
        return GT_FAIL;
    }

    opData.ptpAddr = 0xD;
    opData.ptpData = (obiBytes[2] << 8);

    if((retVal = ptpOperationPerform(dev, op, &opData)) != GT_OK)
    {
        DBG_INFO(("Failed reading DisPTP.\n"));
        return GT_FAIL;
    }

    DBG_INFO(("OK.\n"));
    return GT_OK;
}


/*******************************************************************************
* gavbGetAvbMode
*
* DESCRIPTION:
*        Port's AVB Mode.
*
* INPUTS:
*        port    - the logical port number
*
* OUTPUTS:
*        mode    - GT_AVB_MODE type
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
GT_STATUS gavbGetAvbMode
(
    IN  GT_QD_DEV     *dev,
    IN    GT_LPORT    port,
    OUT GT_AVB_MODE    *mode
)
{
    GT_STATUS           retVal;
    GT_PTP_OPERATION    op;
    GT_PTP_OP_DATA        opData;
    GT_U8          hwPort;         /* the physical port number     */

    DBG_INFO(("gavbGetAvbMode Called.\n"));

#ifndef CONFIG_AVB_FPGA
    /* check if device supports this feature */
    if (!IS_IN_DEV_GROUP(dev,DEV_AVB_POLICY))
    {
        DBG_INFO(("GT_NOT_SUPPORTED\n"));
        return GT_NOT_SUPPORTED;
    }
#endif

    /* translate LPORT to hardware port */
    hwPort = GT_LPORT_2_PORT(port);
    if (hwPort == GT_INVALID_PORT)
        return GT_BAD_PARAM;

    opData.ptpBlock = 0x1;    /* AVB Policy register space */

    opData.ptpPort = (GT_U16)hwPort;
    op = PTP_READ_DATA;

    opData.ptpAddr = 0;

    if((retVal = ptpOperationPerform(dev, op, &opData)) != GT_OK)
    {
        DBG_INFO(("Failed reading DisPTP.\n"));
        return GT_FAIL;
    }

    *mode = (GT_AVB_MODE)((opData.ptpData >> 14) & 0x3);

    DBG_INFO(("OK.\n"));
    return GT_OK;
}


/*******************************************************************************
* gavbSetAvbMode
*
* DESCRIPTION:
*        Port's AVB Mode.
*
* INPUTS:
*        port    - the logical port number
*        mode    - GT_AVB_MODE type
*
* OUTPUTS:
*        None
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
GT_STATUS gavbSetAvbMode
(
    IN  GT_QD_DEV     *dev,
    IN    GT_LPORT    port,
    IN  GT_AVB_MODE    mode
)
{
    GT_STATUS           retVal;
    GT_PTP_OPERATION    op;
    GT_PTP_OP_DATA        opData;
    GT_U8          hwPort;         /* the physical port number     */

    DBG_INFO(("gavbSetAvbMode Called.\n"));

#ifndef CONFIG_AVB_FPGA
    /* check if device supports this feature */
    if (!IS_IN_DEV_GROUP(dev,DEV_AVB_POLICY))
    {
        DBG_INFO(("GT_NOT_SUPPORTED\n"));
        return GT_NOT_SUPPORTED;
    }
#endif

    /* translate LPORT to hardware port */
    hwPort = GT_LPORT_2_PORT(port);
    if (hwPort == GT_INVALID_PORT)
        return GT_BAD_PARAM;

    opData.ptpBlock = 0x1;    /* AVB Policy register space */

    opData.ptpPort = (GT_U16)hwPort;
    op = PTP_READ_DATA;

    opData.ptpAddr = 0;

    if((retVal = ptpOperationPerform(dev, op, &opData)) != GT_OK)
    {
        DBG_INFO(("Failed reading DisPTP.\n"));
        return GT_FAIL;
    }

    opData.ptpData &= ~(0x3 << 14);
    opData.ptpData |= (mode << 14);

    op = PTP_WRITE_DATA;

    opData.ptpAddr = 0;

    if((retVal = ptpOperationPerform(dev, op, &opData)) != GT_OK)
    {
        DBG_INFO(("Failed reading DisPTP.\n"));
        return GT_FAIL;
    }

    DBG_INFO(("OK.\n"));
    return GT_OK;
}

/*******************************************************************************
* gavbGetAvbOverride
*
* DESCRIPTION:
*        AVB Override.
*        When disabled, normal frame processing occurs.
*        When enabled, the egress portion of this port is considered AVB even if
*        the ingress portion is not.
*
* INPUTS:
*        port    - the logical port number
*
* OUTPUTS:
*        en        - GT_TRUE if enabled, GT_FALSE otherwise
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
GT_STATUS gavbGetAvbOverride
(
    IN  GT_QD_DEV     *dev,
    IN    GT_LPORT    port,
    OUT GT_BOOL        *en
)
{
    GT_STATUS           retVal;
    GT_PTP_OPERATION    op;
    GT_PTP_OP_DATA        opData;
    GT_U8          hwPort;         /* the physical port number     */

    DBG_INFO(("gavbGetAvbOverride Called.\n"));

#ifndef CONFIG_AVB_FPGA
    /* check if device supports this feature */
    if (!IS_IN_DEV_GROUP(dev,DEV_AVB_POLICY))
    {
        DBG_INFO(("GT_NOT_SUPPORTED\n"));
        return GT_NOT_SUPPORTED;
    }
#endif

    /* translate LPORT to hardware port */
    hwPort = GT_LPORT_2_PORT(port);
    if (hwPort == GT_INVALID_PORT)
        return GT_BAD_PARAM;

    opData.ptpBlock = 0x1;    /* AVB Policy register space */

    opData.ptpPort = (GT_U16)hwPort;
    op = PTP_READ_DATA;

    opData.ptpAddr = 0;

    if((retVal = ptpOperationPerform(dev, op, &opData)) != GT_OK)
    {
        DBG_INFO(("Failed AVB operation.\n"));
        return GT_FAIL;
    }

    *en = (opData.ptpData >> 13) & 0x1;

    DBG_INFO(("OK.\n"));
    return GT_OK;
}


/*******************************************************************************
* gavbSetAvbOverride
*
* DESCRIPTION:
*        AVB Override.
*        When disabled, normal frame processing occurs.
*        When enabled, the egress portion of this port is considered AVB even if
*        the ingress portion is not.
*
* INPUTS:
*        port    - the logical port number
*        en        - GT_TRUE to enable, GT_FALSE otherwise
*
* OUTPUTS:
*        None
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
GT_STATUS gavbSetAvbOverride
(
    IN  GT_QD_DEV     *dev,
    IN    GT_LPORT    port,
    IN  GT_BOOL        en
)
{
    GT_STATUS           retVal;
    GT_PTP_OPERATION    op;
    GT_PTP_OP_DATA        opData;
    GT_U8          hwPort;         /* the physical port number     */

    DBG_INFO(("gavbSetAvbOverride Called.\n"));

#ifndef CONFIG_AVB_FPGA
    /* check if device supports this feature */
    if (!IS_IN_DEV_GROUP(dev,DEV_AVB_POLICY))
    {
        DBG_INFO(("GT_NOT_SUPPORTED\n"));
        return GT_NOT_SUPPORTED;
    }
#endif

    /* translate LPORT to hardware port */
    hwPort = GT_LPORT_2_PORT(port);
    if (hwPort == GT_INVALID_PORT)
        return GT_BAD_PARAM;

    opData.ptpBlock = 0x1;    /* AVB Policy register space */

    opData.ptpPort = (GT_U16)hwPort;
    op = PTP_READ_DATA;

    opData.ptpAddr = 0;

    if((retVal = ptpOperationPerform(dev, op, &opData)) != GT_OK)
    {
        DBG_INFO(("Failed AVB operation.\n"));
        return GT_FAIL;
    }

    if (en)
        opData.ptpData |= (0x1 << 13);
    else
        opData.ptpData &= ~(0x1 << 13);

    op = PTP_WRITE_DATA;

    opData.ptpAddr = 0;

    if((retVal = ptpOperationPerform(dev, op, &opData)) != GT_OK)
    {
        DBG_INFO(("Failed AVB operation.\n"));
        return GT_FAIL;
    }

    DBG_INFO(("OK.\n"));
    return GT_OK;
}


/*******************************************************************************
* gavbGetFilterBadAvb
*
* DESCRIPTION:
*        Filter Bad AVB frames.
*        When disabled, normal frame processing occurs.
*        When enabled, frames that are considered Bad AVB frames are filtered.
*
* INPUTS:
*        port    - the logical port number
*
* OUTPUTS:
*        en        - GT_TRUE if enabled, GT_FALSE otherwise
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
GT_STATUS gavbGetFilterBadAvb
(
    IN  GT_QD_DEV     *dev,
    IN    GT_LPORT    port,
    OUT GT_BOOL        *en
)
{
    GT_STATUS           retVal;
    GT_PTP_OPERATION    op;
    GT_PTP_OP_DATA        opData;
    GT_U8          hwPort;         /* the physical port number     */

    DBG_INFO(("gavbGetFilterBadAvb Called.\n"));

#ifndef CONFIG_AVB_FPGA
    /* check if device supports this feature */
    if (!IS_IN_DEV_GROUP(dev,DEV_AVB_POLICY))
    {
        DBG_INFO(("GT_NOT_SUPPORTED\n"));
        return GT_NOT_SUPPORTED;
    }
#endif

    /* translate LPORT to hardware port */
    hwPort = GT_LPORT_2_PORT(port);
    if (hwPort == GT_INVALID_PORT)
        return GT_BAD_PARAM;

    opData.ptpBlock = 0x1;    /* AVB Policy register space */

    opData.ptpPort = (GT_U16)hwPort;
    op = PTP_READ_DATA;

    opData.ptpAddr = 0;

    if((retVal = ptpOperationPerform(dev, op, &opData)) != GT_OK)
    {
        DBG_INFO(("Failed AVB operation.\n"));
        return GT_FAIL;
    }

    *en = (opData.ptpData >> 12) & 0x1;

    DBG_INFO(("OK.\n"));
    return GT_OK;
}


/*******************************************************************************
* gavbSetFilterBadAvb
*
* DESCRIPTION:
*        Filter Bad AVB frames.
*        When disabled, normal frame processing occurs.
*        When enabled, frames that are considered Bad AVB frames are filtered.
*
* INPUTS:
*        port    - the logical port number
*        en        - GT_TRUE to enable, GT_FALSE otherwise
*
* OUTPUTS:
*        None
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
GT_STATUS gavbSetFilterBadAvb
(
    IN  GT_QD_DEV     *dev,
    IN    GT_LPORT    port,
    IN  GT_BOOL        en
)
{
    GT_STATUS           retVal;
    GT_PTP_OPERATION    op;
    GT_PTP_OP_DATA        opData;
    GT_U8          hwPort;         /* the physical port number     */

    DBG_INFO(("gavbSetFilterBadAvb Called.\n"));

#ifndef CONFIG_AVB_FPGA
    /* check if device supports this feature */
    if (!IS_IN_DEV_GROUP(dev,DEV_AVB_POLICY))
    {
        DBG_INFO(("GT_NOT_SUPPORTED\n"));
        return GT_NOT_SUPPORTED;
    }
#endif

    /* translate LPORT to hardware port */
    hwPort = GT_LPORT_2_PORT(port);
    if (hwPort == GT_INVALID_PORT)
        return GT_BAD_PARAM;

    opData.ptpBlock = 0x1;    /* AVB Policy register space */

    opData.ptpPort = (GT_U16)hwPort;
    op = PTP_READ_DATA;

    opData.ptpAddr = 0;

    if((retVal = ptpOperationPerform(dev, op, &opData)) != GT_OK)
    {
        DBG_INFO(("Failed AVB operation.\n"));
        return GT_FAIL;
    }

    if (en)
        opData.ptpData |= (0x1 << 12);
    else
        opData.ptpData &= ~(0x1 << 12);

    op = PTP_WRITE_DATA;

    opData.ptpAddr = 0;

    if((retVal = ptpOperationPerform(dev, op, &opData)) != GT_OK)
    {
        DBG_INFO(("Failed AVB operation.\n"));
        return GT_FAIL;
    }

    DBG_INFO(("OK.\n"));
    return GT_OK;
}


/*******************************************************************************
* gavbGetAvbTunnel
*
* DESCRIPTION:
*        AVB Tunnel.
*        When disabled, normal frame processing occurs.
*        When enabled, the port based VLAN Table masking, 802.1Q VLAN membership
*        masking and the Trunk Masking is bypassed for any frame entering this port
*        that is considered AVB by DA. This includes unicast as well as multicast
*        frame
*
* INPUTS:
*        port    - the logical port number
*
* OUTPUTS:
*        en        - GT_TRUE if enabled, GT_FALSE otherwise
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
GT_STATUS gavbGetAvbTunnel
(
    IN  GT_QD_DEV     *dev,
    IN    GT_LPORT    port,
    OUT GT_BOOL        *en
)
{
    GT_STATUS           retVal;
    GT_PTP_OPERATION    op;
    GT_PTP_OP_DATA        opData;
    GT_U8          hwPort;         /* the physical port number     */

    DBG_INFO(("gavbGetAvbTunnel Called.\n"));

#ifndef CONFIG_AVB_FPGA
    /* check if device supports this feature */
    if (!IS_IN_DEV_GROUP(dev,DEV_AVB_POLICY))
    {
        DBG_INFO(("GT_NOT_SUPPORTED\n"));
        return GT_NOT_SUPPORTED;
    }
#endif

    /* translate LPORT to hardware port */
    hwPort = GT_LPORT_2_PORT(port);
    if (hwPort == GT_INVALID_PORT)
        return GT_BAD_PARAM;

    opData.ptpBlock = 0x1;    /* AVB Policy register space */

    opData.ptpPort = (GT_U16)hwPort;
    op = PTP_READ_DATA;

    opData.ptpAddr = 0;

    if((retVal = ptpOperationPerform(dev, op, &opData)) != GT_OK)
    {
        DBG_INFO(("Failed AVB operation.\n"));
        return GT_FAIL;
    }

    *en = (opData.ptpData >> 11) & 0x1;

    DBG_INFO(("OK.\n"));
    return GT_OK;
}


/*******************************************************************************
* gavbSetAvbTunnel
*
* DESCRIPTION:
*        AVB Tunnel.
*        When disabled, normal frame processing occurs.
*        When enabled, the port based VLAN Table masking, 802.1Q VLAN membership
*        masking and the Trunk Masking is bypassed for any frame entering this port
*        that is considered AVB by DA. This includes unicast as well as multicast
*        frame
*
* INPUTS:
*        port    - the logical port number
*        en        - GT_TRUE to enable, GT_FALSE otherwise
*
* OUTPUTS:
*        None
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
GT_STATUS gavbSetAvbTunnel
(
    IN  GT_QD_DEV     *dev,
    IN    GT_LPORT    port,
    IN  GT_BOOL        en
)
{
    GT_STATUS           retVal;
    GT_PTP_OPERATION    op;
    GT_PTP_OP_DATA        opData;
    GT_U8          hwPort;         /* the physical port number     */

    DBG_INFO(("GT_STATUS gavbGetAvbTunnel Called.\n"));

#ifndef CONFIG_AVB_FPGA
    /* check if device supports this feature */
    if (!IS_IN_DEV_GROUP(dev,DEV_AVB_POLICY))
    {
        DBG_INFO(("GT_NOT_SUPPORTED\n"));
        return GT_NOT_SUPPORTED;
    }
#endif

    /* translate LPORT to hardware port */
    hwPort = GT_LPORT_2_PORT(port);
    if (hwPort == GT_INVALID_PORT)
        return GT_BAD_PARAM;

    opData.ptpBlock = 0x1;    /* AVB Policy register space */

    opData.ptpPort = (GT_U16)hwPort;
    op = PTP_READ_DATA;

    opData.ptpAddr = 0;

    if((retVal = ptpOperationPerform(dev, op, &opData)) != GT_OK)
    {
        DBG_INFO(("Failed AVB operation.\n"));
        return GT_FAIL;
    }

    if (en)
        opData.ptpData |= (0x1 << 11);
    else
        opData.ptpData &= ~(0x1 << 11);

    op = PTP_WRITE_DATA;

    opData.ptpAddr = 0;

    if((retVal = ptpOperationPerform(dev, op, &opData)) != GT_OK)
    {
        DBG_INFO(("Failed AVB operation.\n"));
        return GT_FAIL;
    }

    DBG_INFO(("OK.\n"));
    return GT_OK;
}


/*******************************************************************************
* gavbGetAvbFramePolicy
*
* DESCRIPTION:
*        AVB Hi or Lo frame policy mapping.
*        Supported policies are defined in GT_AVB_FRAME_POLICY.
*
* INPUTS:
*        port    - the logical port number
*        fType    - GT_AVB_FRAME_TYPE
*
* OUTPUTS:
*        policy    - GT_AVB_FRAME_POLICY
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
GT_STATUS gavbGetAvbFramePolicy
(
    IN  GT_QD_DEV     *dev,
    IN    GT_LPORT    port,
    IN    GT_AVB_FRAME_TYPE    fType,
    OUT GT_AVB_FRAME_POLICY        *policy
)
{
    GT_STATUS           retVal;
    GT_PTP_OPERATION    op;
    GT_PTP_OP_DATA        opData;
    GT_U8         hwPort;         /* the physical port number     */
    GT_U16        bitPos;

    DBG_INFO(("gavbGetAvbFramePolicy Called.\n"));

#ifndef CONFIG_AVB_FPGA
    /* check if device supports this feature */
    if (!IS_IN_DEV_GROUP(dev,DEV_AVB_POLICY))
    {
        DBG_INFO(("GT_NOT_SUPPORTED\n"));
        return GT_NOT_SUPPORTED;
    }
#endif

    /* translate LPORT to hardware port */
    hwPort = GT_LPORT_2_PORT(port);
    if (hwPort == GT_INVALID_PORT)
        return GT_BAD_PARAM;

    switch (fType)
    {
        case AVB_HI_FRAME:
            bitPos = 2;
            break;
        case AVB_LO_FRAME:
            bitPos = 0;
            break;
        default:
            return GT_BAD_PARAM;
    }

    opData.ptpBlock = 0x1;    /* AVB Policy register space */

    opData.ptpPort = (GT_U16)hwPort;
    op = PTP_READ_DATA;

    opData.ptpAddr = 0;

    if((retVal = ptpOperationPerform(dev, op, &opData)) != GT_OK)
    {
        DBG_INFO(("Failed AVB operation.\n"));
        return GT_FAIL;
    }

    *policy = (opData.ptpData >> bitPos) & 0x3;

    DBG_INFO(("OK.\n"));
    return GT_OK;
}

/*******************************************************************************
* gavbSetAvbFramePolicy
*
* DESCRIPTION:
*        AVB Hi or Lo frame policy mapping.
*        Supported policies are defined in GT_AVB_FRAME_POLICY.
*
* INPUTS:
*        port    - the logical port number
*        fType    - GT_AVB_FRAME_TYPE
*        policy    - GT_AVB_FRAME_POLICY
*
* OUTPUTS:
*        None
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
GT_STATUS gavbSetAvbFramePolicy
(
    IN  GT_QD_DEV     *dev,
    IN    GT_LPORT    port,
    IN    GT_AVB_FRAME_TYPE    fType,
    IN  GT_AVB_FRAME_POLICY        policy
)
{
    GT_STATUS           retVal;
    GT_PTP_OPERATION    op;
    GT_PTP_OP_DATA        opData;
    GT_U8         hwPort;         /* the physical port number     */
    GT_U16        bitPos;

    DBG_INFO(("gavbSetAvbFramePolicy Called.\n"));

#ifndef CONFIG_AVB_FPGA
    /* check if device supports this feature */
    if (!IS_IN_DEV_GROUP(dev,DEV_AVB_POLICY))
    {
        DBG_INFO(("GT_NOT_SUPPORTED\n"));
        return GT_NOT_SUPPORTED;
    }
#endif

    /* translate LPORT to hardware port */
    hwPort = GT_LPORT_2_PORT(port);
    if (hwPort == GT_INVALID_PORT)
        return GT_BAD_PARAM;

    switch (fType)
    {
        case AVB_HI_FRAME:
            bitPos = 2;
            break;
        case AVB_LO_FRAME:
            bitPos = 0;
            break;
        default:
            return GT_BAD_PARAM;
    }

    opData.ptpBlock = 0x1;    /* AVB Policy register space */

    opData.ptpPort = (GT_U16)hwPort;
    op = PTP_READ_DATA;

    opData.ptpAddr = 0;

    if((retVal = ptpOperationPerform(dev, op, &opData)) != GT_OK)
    {
        DBG_INFO(("Failed AVB operation.\n"));
        return GT_FAIL;
    }

    opData.ptpData &= ~(0x3 << bitPos);
    opData.ptpData |= (policy & 0x3) << bitPos;

    op = PTP_WRITE_DATA;

    opData.ptpAddr = 0;

    if((retVal = ptpOperationPerform(dev, op, &opData)) != GT_OK)
    {
        DBG_INFO(("Failed AVB operation.\n"));
        return GT_FAIL;
    }

    DBG_INFO(("OK.\n"));
    return GT_OK;
}

/******************************************************************************
*
*
*******************************************************************************/
/* Amber QAV API */
/*******************************************************************************
* gqavSetPortQpriXQTSToken
*
* DESCRIPTION:
*        This routine set Priority Queue 0-3 time slot tokens on a port.
*        The setting value is number of tokens that need to be subtracted at each
*        QTS interval boundary.
*
* INPUTS:
*        port    - the logical port number
*        queue     - 0 - 3
*        qtsToken - number of tokens.
*
* OUTPUTS:
*        None.
*
* RETURNS:
*        GT_OK      - on success
*        GT_FAIL    - on error
*        GT_BAD_PARAM - if input parameters are beyond range.
*        GT_NOT_SUPPORTED - if current device does not support this feature.
*
* COMMENTS:
*        None.
*
*******************************************************************************/
GT_STATUS gqavSetPortQpriXQTSToken
(
    IN  GT_QD_DEV     *dev,
    IN    GT_LPORT    port,
    IN  GT_U8        queue,
    IN  GT_U16        qtsToken
)
{
    GT_STATUS           retVal;
    GT_PTP_OPERATION    op;
    GT_PTP_OP_DATA        opData;
    GT_U32            hwPort;

    DBG_INFO(("gqavSetPortQpriXQTSToken Called.\n"));

    /* translate LPORT to hardware port */
    hwPort = (GT_U32)GT_LPORT_2_PORT(port);

    /* check if device supports this feature */
    if (!IS_IN_DEV_GROUP(dev,DEV_QAV))
    {
        DBG_INFO(("GT_NOT_SUPPORTED\n"));
        return GT_NOT_SUPPORTED;
    }
    if (!IS_IN_DEV_GROUP(dev,DEV_QAV_QPRI_QTS_TOKEN))
    {
        DBG_INFO(("GT_NOT_SUPPORTED\n"));
        return GT_NOT_SUPPORTED;
    }

    /* check if queue is beyond range */
    if (!IS_IN_DEV_GROUP(dev,DEV_FE_AVB_FAMILY))
    {
      if (queue>0x3)
      {
        DBG_INFO(("GT_BAD_PARAM\n"));
        return GT_BAD_PARAM;
      }
    }
    else
    {
      if ((queue>0x3)||(queue<0x2))
      {
        DBG_INFO(("GT_BAD_PARAM\n"));
        return GT_BAD_PARAM;
      }
    }

    /* check if qtsToken is beyond range */
    if (!IS_IN_DEV_GROUP(dev,DEV_FE_AVB_FAMILY))
    {
      if (qtsToken>0x7fff)
      {
        DBG_INFO(("GT_BAD_PARAM\n"));
        return GT_BAD_PARAM;
      }
    }
    else
    {
      if (qtsToken>0x3fff)
      {
        DBG_INFO(("GT_BAD_PARAM\n"));
        return GT_BAD_PARAM;
      }
    }

    opData.ptpBlock = 0x2;    /* QAV register space */
    opData.ptpAddr = queue*2;

    opData.ptpPort = hwPort;

    op = PTP_WRITE_DATA;

    if (!IS_IN_DEV_GROUP(dev,DEV_FE_AVB_FAMILY))
      opData.ptpData = qtsToken&0x7fff;
    else
      opData.ptpData = qtsToken&0x3fff;

    if((retVal = ptpOperationPerform(dev, op, &opData)) != GT_OK)
    {
        DBG_INFO(("Failed writing QTS token for port %d queue %d.\n", port, queue));
        return GT_FAIL;
    }

    DBG_INFO(("OK.\n"));
    return GT_OK;
}

/*******************************************************************************
* gqavGetPortQpriXQTSToken
*
* DESCRIPTION:
*        This routine get Priority Queue 0-3 time slot tokens on a port.
*        The setting value is number of tokens that need to be subtracted at each
*        QTS interval boundary.
*
* INPUTS:
*        port    - the logical port number
*        queue - 0 - 3
*
* OUTPUTS:
*        qtsToken - number of tokens
*
* RETURNS:
*        GT_OK      - on success
*        GT_FAIL    - on error
*        GT_BAD_PARAM - if input parameters are beyond range.
*        GT_NOT_SUPPORTED - if current device does not support this feature.
*
* COMMENTS:
*        None.
*
*******************************************************************************/
GT_STATUS gqavGetPortQpriXQTSToken
(
    IN  GT_QD_DEV     *dev,
    IN    GT_LPORT    port,
    IN  GT_U8        queue,
    OUT GT_U16        *qtsToken
)
{
    GT_STATUS           retVal;
    GT_PTP_OPERATION    op;
    GT_PTP_OP_DATA        opData;
    GT_U32            hwPort;

    DBG_INFO(("gqavGetPortQpriXQTSToken Called.\n"));

    /* translate LPORT to hardware port */
    hwPort = (GT_U32)GT_LPORT_2_PORT(port);

    /* check if device supports this feature */
    if (!IS_IN_DEV_GROUP(dev,DEV_QAV))
    {
        DBG_INFO(("GT_NOT_SUPPORTED\n"));
        return GT_NOT_SUPPORTED;
    }
    if (!IS_IN_DEV_GROUP(dev,DEV_QAV_QPRI_QTS_TOKEN))
    {
        DBG_INFO(("GT_NOT_SUPPORTED\n"));
        return GT_NOT_SUPPORTED;
    }

    /* check if queue is beyond range */
    if (!IS_IN_DEV_GROUP(dev,DEV_FE_AVB_FAMILY))
    {
      if (queue>0x3)
      {
        DBG_INFO(("GT_BAD_PARAM\n"));
        return GT_BAD_PARAM;
      }
    }
    else
    {
      if ((queue>0x3)||(queue<0x2))
      {
        DBG_INFO(("GT_BAD_PARAM\n"));
        return GT_BAD_PARAM;
      }
    }

    opData.ptpBlock = 0x2;    /* QAV register space */

    opData.ptpAddr = queue*2;
    opData.ptpPort = hwPort;

    op = PTP_READ_DATA;

    if((retVal = ptpOperationPerform(dev, op, &opData)) != GT_OK)
    {
        DBG_INFO(("Failed reading QTS token for port %d queue %d.\n", port, queue));
        return GT_FAIL;
    }

    if (!IS_IN_DEV_GROUP(dev,DEV_FE_AVB_FAMILY))
      *qtsToken =(GT_U16)opData.ptpData&0x7fff;
    else
      *qtsToken =(GT_U16)opData.ptpData&0x3fff;

    DBG_INFO(("OK.\n"));
    return GT_OK;

}

/*******************************************************************************
* gqavSetPortQpriXBurstBytes
*
* DESCRIPTION:
*        This routine set Priority Queue 0-3 Burst Bytes on a port.
*        This value specifies the number of credits in bytes that can be
*        accumulated when the queue is blocked from sending out a frame due to
*        higher priority queue frames being sent out.
*
* INPUTS:
*        port    - the logical port number
*        queue - 0 - 3
*        burst - number of credits in bytes .
*
* OUTPUTS:
*        None.
*
* RETURNS:
*        GT_OK      - on success
*        GT_FAIL    - on error
*        GT_BAD_PARAM - if input parameters are beyond range.
*        GT_NOT_SUPPORTED - if current device does not support this feature.
*
* COMMENTS:
*        None
*
*******************************************************************************/
GT_STATUS gqavSetPortQpriXBurstBytes
(
    IN  GT_QD_DEV     *dev,
    IN    GT_LPORT    port,
    IN  GT_U8        queue,
    IN  GT_U16        burst
)
{
    GT_STATUS           retVal;
    GT_PTP_OPERATION    op;
    GT_PTP_OP_DATA        opData;
    GT_U32            hwPort;

    DBG_INFO(("gqavSetPortQpriXBurstBytes Called.\n"));

    /* translate LPORT to hardware port */
    hwPort = (GT_U32)GT_LPORT_2_PORT(port);

    /* check if device supports this feature */
    if (!IS_IN_DEV_GROUP(dev,DEV_QAV))
    {
        DBG_INFO(("GT_NOT_SUPPORTED\n"));
        return GT_NOT_SUPPORTED;
    }
    if (!IS_IN_DEV_GROUP(dev,DEV_QAV_QPRI_QTS_TOKEN))
    {
        DBG_INFO(("GT_NOT_SUPPORTED\n"));
        return GT_NOT_SUPPORTED;
    }

    /* check if queue is beyond range */
    if (!IS_IN_DEV_GROUP(dev,DEV_FE_AVB_FAMILY))
    {
      if (queue>0x3)
      {
        DBG_INFO(("GT_BAD_PARAM\n"));
        return GT_BAD_PARAM;
      }
    }
    else
    {
      if ((queue>0x3)||(queue<0x2))
      {
        DBG_INFO(("GT_BAD_PARAM\n"));
        return GT_BAD_PARAM;
      }
    }

    /* check if burst is beyond range */
    if (burst>0x7fff)
    {
        DBG_INFO(("GT_BAD_PARAM\n"));
        return GT_BAD_PARAM;
    }

    opData.ptpBlock = 0x2;    /* QAV register space */
    opData.ptpAddr = queue*2+1;

    opData.ptpPort = hwPort;

    op = PTP_WRITE_DATA;

    opData.ptpData = burst&0x7fff;

    if((retVal = ptpOperationPerform(dev, op, &opData)) != GT_OK)
    {
        DBG_INFO(("Failed writing Burst bytes for port %d queue %d.\n", port, queue));
        return GT_FAIL;
    }

    DBG_INFO(("OK.\n"));
    return GT_OK;
}
/*******************************************************************************
* gqavGetPortQpriXBurstBytes
*
* DESCRIPTION:
*        This routine get Priority Queue 0-3 Burst Bytes on a port.
*        This value specifies the number of credits in bytes that can be
*        accumulated when the queue is blocked from sending out a frame due to
*        higher priority queue frames being sent out.
*
* INPUTS:
*        port    - the logical port number
*        queue    - 0 - 3
*
* OUTPUTS:
*        burst - number of credits in bytes .
*
* RETURNS:
*        GT_OK      - on success
*        GT_FAIL    - on error
*        GT_BAD_PARAM - if input parameters are beyond range.
*        GT_NOT_SUPPORTED - if current device does not support this feature.
*
* COMMENTS:
*        None
*
*******************************************************************************/
GT_STATUS gqavGetPortQpriXBurstBytes
(
    IN  GT_QD_DEV     *dev,
    IN    GT_LPORT    port,
    IN  GT_U8        queue,
    OUT GT_U16        *burst
)
{
    GT_STATUS           retVal;
    GT_PTP_OPERATION    op;
    GT_PTP_OP_DATA        opData;
    GT_U32            hwPort;

    DBG_INFO(("gqavgetPortQpriXBurstBytes Called.\n"));

    /* translate LPORT to hardware port */
    hwPort = (GT_U32)GT_LPORT_2_PORT(port);

    /* check if device supports this feature */
    if (!IS_IN_DEV_GROUP(dev,DEV_QAV))
    {
        DBG_INFO(("GT_NOT_SUPPORTED\n"));
        return GT_NOT_SUPPORTED;
    }
    if (!IS_IN_DEV_GROUP(dev,DEV_QAV_QPRI_QTS_TOKEN))
    {
        DBG_INFO(("GT_NOT_SUPPORTED\n"));
        return GT_NOT_SUPPORTED;

    }
    /* check if queue is beyond range */
    if (!IS_IN_DEV_GROUP(dev,DEV_FE_AVB_FAMILY))
    {
      if (queue>0x3)
      {
        DBG_INFO(("GT_BAD_PARAM\n"));
        return GT_BAD_PARAM;
      }
    }
    else
    {
      if ((queue>0x3)||(queue<0x2))
      {
        DBG_INFO(("GT_BAD_PARAM\n"));
        return GT_BAD_PARAM;
      }
    }

    opData.ptpBlock = 0x2;    /* QAV register space */

    opData.ptpAddr = queue*2+1;
    opData.ptpPort = hwPort;

    op = PTP_READ_DATA;

    if((retVal = ptpOperationPerform(dev, op, &opData)) != GT_OK)
    {
        DBG_INFO(("Failed reading Burst bytes for port %d queue %d.\n", port, queue));
        return GT_FAIL;
    }

    *burst = (GT_U16)opData.ptpData&0x7fff;

    DBG_INFO(("OK.\n"));
    return GT_OK;

}

/*******************************************************************************
* gqavSetPortQpriXRate
*
* DESCRIPTION:
*        This routine set Priority Queue 2-3 rate on a port.
*
* INPUTS:
*        port    - the logical port number
*        queue - 2 - 3
*        rate - number of credits in bytes .
*
* OUTPUTS:
*        None.
*
* RETURNS:
*        GT_OK      - on success
*        GT_FAIL    - on error
*        GT_BAD_PARAM - if input parameters are beyond range.
*        GT_NOT_SUPPORTED - if current device does not support this feature.
*
* COMMENTS:
*        None
*
*******************************************************************************/
GT_STATUS gqavSetPortQpriXRate
(
    IN  GT_QD_DEV     *dev,
    IN    GT_LPORT    port,
    IN  GT_U8        queue,
    IN  GT_U16        rate
)
{
    GT_STATUS           retVal;
    GT_PTP_OPERATION    op;
    GT_PTP_OP_DATA        opData;
    GT_U32            hwPort;

    DBG_INFO(("gqavSetPortQpriXRate Called.\n"));

    /* translate LPORT to hardware port */
    hwPort = (GT_U32)GT_LPORT_2_PORT(port);

    /* check if device supports this feature */
    if (!IS_IN_DEV_GROUP(dev,DEV_QAV))
    {
        DBG_INFO(("GT_NOT_SUPPORTED\n"));
        return GT_NOT_SUPPORTED;
    }
    if (!IS_IN_DEV_GROUP(dev, DEV_QAV_QPRI_RATE))
    {
        DBG_INFO(("GT_NOT_SUPPORTED\n"));
        return GT_NOT_SUPPORTED;
    }

    /* check if queue is beyond range */
    if (!IS_IN_DEV_GROUP(dev,DEV_FE_AVB_FAMILY))
    {
      if (queue>0x3)
      {
        DBG_INFO(("GT_BAD_PARAM\n"));
        return GT_BAD_PARAM;
      }
    }
    else
    {
      if ((queue>0x3)||(queue<0x2))
      {
        DBG_INFO(("GT_BAD_PARAM\n"));
        return GT_BAD_PARAM;
      }
    }

    /* check if rate  is beyond range */
    if (rate>0x0fff)
    {
        DBG_INFO(("GT_BAD_PARAM\n"));
        return GT_BAD_PARAM;
    }

    opData.ptpBlock = 0x2;    /* QAV register space */
    opData.ptpAddr = queue*2+1;

    opData.ptpPort = hwPort;

    op = PTP_WRITE_DATA;

    opData.ptpData = rate&0x0fff;

    if((retVal = ptpOperationPerform(dev, op, &opData)) != GT_OK)
    {
        DBG_INFO(("Failed writing rate bytes for port %d queue %d.\n", port, queue));
        return GT_FAIL;
    }

    DBG_INFO(("OK.\n"));
    return GT_OK;
}
/*******************************************************************************
* gqavGetPortQpriXRate
*
* DESCRIPTION:
*        This routine get Priority Queue 2-3 rate Bytes on a port.
*
* INPUTS:
*        port    - the logical port number
*        queue    - 2 - 3
*
* OUTPUTS:
*        rate - number of credits in bytes .
*
* RETURNS:
*        GT_OK      - on success
*        GT_FAIL    - on error
*        GT_BAD_PARAM - if input parameters are beyond range.
*        GT_NOT_SUPPORTED - if current device does not support this feature.
*
* COMMENTS:
*        None
*
*******************************************************************************/
GT_STATUS gqavGetPortQpriXRate
(
    IN  GT_QD_DEV     *dev,
    IN    GT_LPORT    port,
    IN  GT_U8        queue,
    OUT GT_U16        *rate
)
{
    GT_STATUS           retVal;
    GT_PTP_OPERATION    op;
    GT_PTP_OP_DATA        opData;
    GT_U32            hwPort;

    DBG_INFO(("gqavgetPortQpriXRate Called.\n"));

    /* translate LPORT to hardware port */
    hwPort = (GT_U32)GT_LPORT_2_PORT(port);

    /* check if device supports this feature */
    if (!IS_IN_DEV_GROUP(dev,DEV_QAV))
    {
        DBG_INFO(("GT_NOT_SUPPORTED\n"));
        return GT_NOT_SUPPORTED;
    }
    if (!IS_IN_DEV_GROUP(dev,DEV_QAV_QPRI_RATE))
    {
        DBG_INFO(("GT_NOT_SUPPORTED\n"));
        return GT_NOT_SUPPORTED;

    }
    /* check if queue is beyond range */
    if (!IS_IN_DEV_GROUP(dev,DEV_FE_AVB_FAMILY))
    {
      if (queue>0x3)
      {
        DBG_INFO(("GT_BAD_PARAM\n"));
        return GT_BAD_PARAM;
      }
    }
    else
    {
      if ((queue>0x3)||(queue<0x2))
      {
        DBG_INFO(("GT_BAD_PARAM\n"));
        return GT_BAD_PARAM;
      }
    }

    opData.ptpBlock = 0x2;    /* QAV register space */

    opData.ptpAddr = queue*2+1;
    opData.ptpPort = hwPort;

    op = PTP_READ_DATA;

    if((retVal = ptpOperationPerform(dev, op, &opData)) != GT_OK)
    {
        DBG_INFO(("Failed reading Rate bytes for port %d queue %d.\n", port, queue));
        return GT_FAIL;
    }

    *rate = (GT_U16)opData.ptpData&0x0fff;

    DBG_INFO(("OK.\n"));
    return GT_OK;

}
/*******************************************************************************
* gqavSetPortQpriXHiLimit
*
* DESCRIPTION:
*        This routine set Priority Queue 2-3 HiLimit on a port.
*
* INPUTS:
*        port    - the logical port number
*        queue - 2 - 3
*        hiLimit - number of credits in bytes .
*
* OUTPUTS:
*        None.
*
* RETURNS:
*        GT_OK      - on success
*        GT_FAIL    - on error
*        GT_BAD_PARAM - if input parameters are beyond range.
*        GT_NOT_SUPPORTED - if current device does not support this feature.
*
* COMMENTS:
*        None
*
*******************************************************************************/
GT_STATUS gqavSetPortQpriXHiLimit
(
    IN  GT_QD_DEV     *dev,
    IN    GT_LPORT    port,
    IN  GT_U8        queue,
    IN  GT_U16        hiLimit
)
{
    GT_STATUS           retVal;
    GT_PTP_OPERATION    op;
    GT_PTP_OP_DATA        opData;
    GT_U32            hwPort;

    DBG_INFO(("gqavSetPortQpriXHiLimit Called.\n"));

    /* translate LPORT to hardware port */
    hwPort = (GT_U32)GT_LPORT_2_PORT(port);

    /* check if device supports this feature */
    if (!IS_IN_DEV_GROUP(dev,DEV_QAV))
    {
        DBG_INFO(("GT_NOT_SUPPORTED\n"));
        return GT_NOT_SUPPORTED;
    }
    if (!IS_IN_DEV_GROUP(dev, DEV_QAV_QPRI_RATE))
    {
        DBG_INFO(("GT_NOT_SUPPORTED\n"));
        return GT_NOT_SUPPORTED;
    }

    /* check if queue is beyond range */
    if ((queue>0x3)||(queue<2))
    {
        DBG_INFO(("GT_BAD_PARAM\n"));
        return GT_BAD_PARAM;
    }

    /* check if rate  is beyond range */
    if (hiLimit>0x0fff)
    {
        DBG_INFO(("GT_BAD_PARAM\n"));
        return GT_BAD_PARAM;
    }

    opData.ptpBlock = 0x2;    /* QAV register space */
    opData.ptpAddr = queue*2+1;

    opData.ptpPort = hwPort;

    op = PTP_WRITE_DATA;

    opData.ptpData = hiLimit&0x0fff;

    if((retVal = ptpOperationPerform(dev, op, &opData)) != GT_OK)
    {
        DBG_INFO(("Failed writing Burst bytes for port %d queue %d.\n", port, queue));
        return GT_FAIL;
    }

    DBG_INFO(("OK.\n"));
    return GT_OK;
}
/*******************************************************************************
* gqavGetPortQpriXHiLimit
*
* DESCRIPTION:
*        This routine get Priority Queue 2-3 HiLimit Bytes on a port.
*
* INPUTS:
*        port    - the logical port number
*        queue    - 2 - 3
*
* OUTPUTS:
*        hiLimit - number of credits in bytes .
*
* RETURNS:
*        GT_OK      - on success
*        GT_FAIL    - on error
*        GT_BAD_PARAM - if input parameters are beyond range.
*        GT_NOT_SUPPORTED - if current device does not support this feature.
*
* COMMENTS:
*        None
*
*******************************************************************************/
GT_STATUS gqavGetPortQpriXHiLimit
(
    IN  GT_QD_DEV     *dev,
    IN    GT_LPORT    port,
    IN  GT_U8        queue,
    OUT GT_U16        *hiLimit
)
{
    GT_STATUS           retVal;
    GT_PTP_OPERATION    op;
    GT_PTP_OP_DATA        opData;
    GT_U32            hwPort;

    DBG_INFO(("gqavgetPortQpriXHiLimit Called.\n"));

    /* translate LPORT to hardware port */
    hwPort = (GT_U32)GT_LPORT_2_PORT(port);

    /* check if device supports this feature */
    if (!IS_IN_DEV_GROUP(dev,DEV_QAV))
    {
        DBG_INFO(("GT_NOT_SUPPORTED\n"));
        return GT_NOT_SUPPORTED;
    }
    if (!IS_IN_DEV_GROUP(dev,DEV_QAV_QPRI_RATE))
    {
        DBG_INFO(("GT_NOT_SUPPORTED\n"));
        return GT_NOT_SUPPORTED;

    }
    /* check if queue is beyond range */
    if ((queue>0x3)||(queue<2))
    {
        DBG_INFO(("GT_BAD_PARAM\n"));
        return GT_BAD_PARAM;
    }

    opData.ptpBlock = 0x2;    /* QAV register space */

    opData.ptpAddr = queue*2+1;
    opData.ptpPort = hwPort;

    op = PTP_READ_DATA;

    if((retVal = ptpOperationPerform(dev, op, &opData)) != GT_OK)
    {
        DBG_INFO(("Failed reading Hi Limit bytes for port %d queue %d.\n", port, queue));
        return GT_FAIL;
    }

    *hiLimit = (GT_U16)opData.ptpData&0x7fff;

    DBG_INFO(("OK.\n"));
    return GT_OK;

}

/*******************************************************************************
* gqavSetPortQavEnable
*
* DESCRIPTION:
*        This routine set QAV enable status on a port.
*
* INPUTS:
*        port    - the logical port number
*        en        - GT_TRUE: QAV enable, GT_FALSE: QAV disable
*
* OUTPUTS:
*        None.
*
* RETURNS:
*        GT_OK      - on success
*        GT_FAIL    - on error
*        GT_BAD_PARAM - if input parameters are beyond range.
*        GT_NOT_SUPPORTED - if current device does not support this feature.
*
* COMMENTS:
*        None
*
*******************************************************************************/
GT_STATUS gqavSetPortQavEnable
(
    IN  GT_QD_DEV     *dev,
    IN    GT_LPORT    port,
    IN  GT_BOOL        en
)
{
    GT_STATUS           retVal;
    GT_PTP_OPERATION    op;
    GT_PTP_OP_DATA        opData;
    GT_U32            hwPort;

    DBG_INFO(("gqavSetPortQavEnable Called.\n"));

    /* translate LPORT to hardware port */
    hwPort = (GT_U32)GT_LPORT_2_PORT(port);

    /* check if device supports this feature */
    if (!IS_IN_DEV_GROUP(dev,DEV_QAV))
    {
        DBG_INFO(("GT_NOT_SUPPORTED\n"));
        return GT_NOT_SUPPORTED;
    }

    opData.ptpBlock = 0x2;    /* QAV register space */
    opData.ptpAddr = 8;

    opData.ptpPort = hwPort;

    op = PTP_WRITE_DATA;

    opData.ptpData = (en==GT_TRUE)?0x8000:0;

    if((retVal = ptpOperationPerform(dev, op, &opData)) != GT_OK)
    {
        DBG_INFO(("Failed writing QAV enable for port %d.\n", port));
        return GT_FAIL;
    }

    DBG_INFO(("OK.\n"));
    return GT_OK;
}


/*******************************************************************************
* gqavGetPortQavEnable
*
* DESCRIPTION:
*        This routine get QAV enable status on a port.
*
* INPUTS:
*        port    - the logical port number
*
* OUTPUTS:
*        en        - GT_TRUE: QAV enable, GT_FALSE: QAV disable
*
* RETURNS:
*        GT_OK      - on success
*        GT_FAIL    - on error
*        GT_BAD_PARAM - if input parameters are beyond range.
*        GT_NOT_SUPPORTED - if current device does not support this feature.
*
* COMMENTS:
*        None
*
*******************************************************************************/
GT_STATUS gqavGetPortQavEnable
(
    IN  GT_QD_DEV     *dev,
    IN    GT_LPORT    port,
    OUT GT_BOOL        *en
)
{
    GT_STATUS           retVal;
    GT_PTP_OPERATION    op;
    GT_PTP_OP_DATA        opData;
    GT_U32            hwPort;

    DBG_INFO(("gqavGetPortQavEnable Called.\n"));

    /* translate LPORT to hardware port */
    hwPort = (GT_U32)GT_LPORT_2_PORT(port);

    /* check if device supports this feature */
    if (!IS_IN_DEV_GROUP(dev,DEV_QAV))
    {
        DBG_INFO(("GT_NOT_SUPPORTED\n"));
        return GT_NOT_SUPPORTED;
    }

    opData.ptpBlock = 0x2;    /* QAV register space */
    opData.ptpAddr = 8;

    opData.ptpPort = hwPort;

    op = PTP_READ_DATA;

    if((retVal = ptpOperationPerform(dev, op, &opData)) != GT_OK)
    {
        DBG_INFO(("Failed reading QAV enable for port %d.\n", port));
        return GT_FAIL;
    }

    *en = ((opData.ptpData&0x8000)==0)?GT_FALSE:GT_TRUE;

    DBG_INFO(("OK.\n"));
    return GT_OK;
}


/*******************************************************************************/
/* QAV Global resters processing */
/*******************************************************************************
* gqavSetGlobalAdminMGMT
*
* DESCRIPTION:
*        This routine set to accept Admit Management Frames always.
*
* INPUTS:
*        en - GT_TRUE to set MGMT frame accepted always,
*             GT_FALSE do not set MGMT frame accepted always
*
* OUTPUTS:
*        None.
*
* RETURNS:
*        GT_OK      - on success
*        GT_FAIL    - on error
*        GT_BAD_PARAM - if input parameters are beyond range.
*        GT_NOT_SUPPORTED - if current device does not support this feature.
*
* COMMENTS:
*        None
*
*******************************************************************************/
GT_STATUS gqavSetGlobalAdminMGMT
(
    IN  GT_QD_DEV     *dev,
    IN  GT_BOOL        en
)
{
    GT_STATUS           retVal;
    GT_PTP_OPERATION    op;
    GT_PTP_OP_DATA        opData;

    DBG_INFO(("gqavSetGlobalAdminMGMT Called.\n"));

    /* check if device supports this feature */
    if (!IS_IN_DEV_GROUP(dev,DEV_QAV))
    {
        DBG_INFO(("GT_NOT_SUPPORTED\n"));
        return GT_NOT_SUPPORTED;
    }

    opData.ptpBlock = 0x2;    /* QAV register space */
    opData.ptpAddr = 0;

    opData.ptpPort = 0xF;

    op = PTP_READ_DATA;
    if((retVal = ptpOperationPerform(dev, op, &opData)) != GT_OK)
    {
        DBG_INFO(("Failed reading QAV global config admin MGMT.\n"));
        return GT_FAIL;
    }

    op = PTP_WRITE_DATA;

    opData.ptpData &= ~0x8000;
    if (en)
        opData.ptpData |= 0x8000;

    if((retVal = ptpOperationPerform(dev, op, &opData)) != GT_OK)
    {
        DBG_INFO(("Failed writing QAV global config admin MGMT.\n"));
        return GT_FAIL;
    }

    DBG_INFO(("OK.\n"));
    return GT_OK;
}

/*******************************************************************************
* gqavGetGlobalAdminMGMT
*
* DESCRIPTION:
*        This routine get setting of Admit Management Frames always.
*
* INPUTS:
*        None.
*
* OUTPUTS:
*        en - GT_TRUE to set MGMT frame accepted always,
*             GT_FALSE do not set MGMT frame accepted always
*
* RETURNS:
*        GT_OK      - on success
*        GT_FAIL    - on error
*        GT_BAD_PARAM - if input parameters are beyond range.
*        GT_NOT_SUPPORTED - if current device does not support this feature.
*
* COMMENTS:
*        None
*
*******************************************************************************/
GT_STATUS gqavGetGlobalAdminMGMT
(
    IN  GT_QD_DEV     *dev,
    OUT GT_BOOL        *en
)
{
    GT_STATUS           retVal;
    GT_PTP_OPERATION    op;
    GT_PTP_OP_DATA        opData;

    DBG_INFO(("gqavGetGlobalAdminMGMT Called.\n"));

    /* check if device supports this feature */
    if (!IS_IN_DEV_GROUP(dev,DEV_QAV))
    {
        DBG_INFO(("GT_NOT_SUPPORTED\n"));
        return GT_NOT_SUPPORTED;
    }

    opData.ptpBlock = 0x2;    /* QAV register space */
    opData.ptpAddr = 0;

    opData.ptpPort = 0xF;

    op = PTP_READ_DATA;
    if((retVal = ptpOperationPerform(dev, op, &opData)) != GT_OK)
    {
        DBG_INFO(("Failed reading QAV global config admin MGMT.\n"));
        return GT_FAIL;
    }

    if (opData.ptpData&0x8000)
      *en = GT_TRUE;
    else
      *en = GT_FALSE;

    DBG_INFO(("OK.\n"));
    return GT_OK;
}


/*******************************************************************************
* gqavSetGlobalIsoPtrThreshold
*
* DESCRIPTION:
*        This routine set Global Isochronous Queue Pointer Threshold.
*        This field indicates the total number of isochronous pointers
*        that are reserved for isochronous streams. The value is expected to be
*        computed in SRP software and programmed into hardware based on the total
*        aggregate isochronous streams configured to go through this device..
*
* INPUTS:
*        isoPtrs -  total number of isochronous pointers
*
* OUTPUTS:
*        None.
*
* RETURNS:
*        GT_OK      - on success
*        GT_FAIL    - on error
*        GT_BAD_PARAM - if input parameters are beyond range.
*        GT_NOT_SUPPORTED - if current device does not support this feature.
*
* COMMENTS:
*        None
*
*******************************************************************************/
GT_STATUS gqavSetGlobalIsoPtrThreshold
(
    IN  GT_QD_DEV     *dev,
    IN  GT_U16        isoPtrs
)
{
    GT_STATUS           retVal;
    GT_PTP_OPERATION    op;
    GT_PTP_OP_DATA        opData;

    DBG_INFO(("gqavSetGlobalIsoPtrThreshold Called.\n"));

    /* check if device supports this feature */
    if (!IS_IN_DEV_GROUP(dev,DEV_QAV))
    {
        DBG_INFO(("GT_NOT_SUPPORTED\n"));
        return GT_NOT_SUPPORTED;
    }

    /* check if isoPtrs is beyond range */
    if (!(IS_IN_DEV_GROUP(dev,DEV_88ESPANNAK_FAMILY)))
    {
      if (isoPtrs>0x3ff)
      {
        DBG_INFO(("GT_BAD_PARAM\n"));
        return GT_BAD_PARAM;
      }
    }
    else
    {
      if (isoPtrs>0x1ff)
      {
        DBG_INFO(("GT_BAD_PARAM\n"));
        return GT_BAD_PARAM;
      }
    }

    opData.ptpBlock = 0x2;    /* QAV register space */
    opData.ptpAddr = 0;

    opData.ptpPort = 0xF;

    op = PTP_READ_DATA;
    if((retVal = ptpOperationPerform(dev, op, &opData)) != GT_OK)
    {
        DBG_INFO(("Failed reading QAV global config Isochronous Queue Pointer Threshold.\n"));
        return GT_FAIL;
    }

    op = PTP_WRITE_DATA;

    if (!(IS_IN_DEV_GROUP(dev,DEV_88ESPANNAK_FAMILY)))
      opData.ptpData &= ~0x3ff;
    else
      opData.ptpData &= ~0x1ff;
    opData.ptpData |= isoPtrs;

    if((retVal = ptpOperationPerform(dev, op, &opData)) != GT_OK)
    {
        DBG_INFO(("Failed writing QAV global config Isochronous Queue Pointer Threshold.\n"));
        return GT_FAIL;
    }

    DBG_INFO(("OK.\n"));
    return GT_OK;
}


/*******************************************************************************
* gqavGetGlobalIsoPtrThreshold
*
* DESCRIPTION:
*        This routine get Global Isochronous Queue Pointer Threshold.
*        This field indicates the total number of isochronous pointers
*        that are reserved for isochronous streams. The value is expected to be
*        computed in SRP software and programmed into hardware based on the total
*        aggregate isochronous streams configured to go through this device..
*
* INPUTS:
*        None.
*
* OUTPUTS:
*        isoPtrs -  total number of isochronous pointers
*
* RETURNS:
*        GT_OK      - on success
*        GT_FAIL    - on error
*        GT_BAD_PARAM - if input parameters are beyond range.
*        GT_NOT_SUPPORTED - if current device does not support this feature.
*
* COMMENTS:
*        None
*
*******************************************************************************/
GT_STATUS gqavGetGlobalIsoPtrThreshold
(
    IN  GT_QD_DEV     *dev,
    OUT GT_U16        *isoPtrs
)
{
    GT_STATUS           retVal;
    GT_PTP_OPERATION    op;
    GT_PTP_OP_DATA        opData;

    DBG_INFO(("gqavGetGlobalIsoPtrThreshold Called.\n"));

    /* check if device supports this feature */
    if (!IS_IN_DEV_GROUP(dev,DEV_QAV))
    {
        DBG_INFO(("GT_NOT_SUPPORTED\n"));
        return GT_NOT_SUPPORTED;
    }

    opData.ptpBlock = 0x2;    /* QAV register space */
    opData.ptpAddr = 0;

    opData.ptpPort = 0xF;

    op = PTP_READ_DATA;
    if((retVal = ptpOperationPerform(dev, op, &opData)) != GT_OK)
    {
        DBG_INFO(("Failed reading Isochronous Queue Pointer Threshold.\n"));
        return GT_FAIL;
    }

    if (!(IS_IN_DEV_GROUP(dev,DEV_88ESPANNAK_FAMILY)))
      *isoPtrs = (GT_U16)opData.ptpData&0x3ff;
    else
      *isoPtrs = (GT_U16)opData.ptpData&0x1ff;

    DBG_INFO(("OK.\n"));
    return GT_OK;
}


/*******************************************************************************
* gqavSetGlobalDisQSD4MGMT
*
* DESCRIPTION:
*        This routine set Disable Queue Scheduler Delays for Management frames..
*
* INPUTS:
*        en - GT_TRUE, it indicates to the Queue Controller to disable applying Queue
*            Scheduler Delays and the corresponding rate regulator does not account
*            for MGMT frames through this queue.
*            GT_FALSE, the MGMT frames follow similar rate regulation and delay
*            regulation envelope as specified for the isochronous queue that the
*            MGMT frames are sharing with.
*
* OUTPUTS:
*        None.
*
* RETURNS:
*        GT_OK      - on success
*        GT_FAIL    - on error
*        GT_NOT_SUPPORTED - if current device does not support this feature.
*
* COMMENTS:
*        None
*
*******************************************************************************/
GT_STATUS gqavSetGlobalDisQSD4MGMT
(
    IN  GT_QD_DEV     *dev,
    IN  GT_BOOL        en
)
{
    GT_STATUS           retVal;
    GT_PTP_OPERATION    op;
    GT_PTP_OP_DATA        opData;

    DBG_INFO(("gqavSetGlobalDisQSD4MGMT Called.\n"));

    /* check if device supports this feature */
    if (!IS_IN_DEV_GROUP(dev,DEV_QAV))
    {
        DBG_INFO(("GT_NOT_SUPPORTED\n"));
        return GT_NOT_SUPPORTED;
    }

    opData.ptpBlock = 0x2;    /* QAV register space */
    opData.ptpAddr = 3;

    opData.ptpPort = 0xF;

    op = PTP_READ_DATA;
    if((retVal = ptpOperationPerform(dev, op, &opData)) != GT_OK)
    {
        DBG_INFO(("Failed reading Disable Queue Scheduler Delay for MGMT frames.\n"));
        return GT_FAIL;
    }

    op = PTP_WRITE_DATA;

    opData.ptpData &= ~0x4000;
    if (en==GT_TRUE)
        opData.ptpData |= 0x4000;

    if((retVal = ptpOperationPerform(dev, op, &opData)) != GT_OK)
    {
        DBG_INFO(("Failed writing Disable Queue Scheduler Delay for MGMT frames.\n"));
        return GT_FAIL;
    }

    DBG_INFO(("OK.\n"));
    return GT_OK;
}

/*******************************************************************************
* gqavGetGlobalDisQSD4MGMT
*
* DESCRIPTION:
*        This routine Get Disable Queue Scheduler Delays for Management frames..
*
* INPUTS:
*        None.
*
* OUTPUTS:
*        en - GT_TRUE, it indicates to the Queue Controller to disable applying Queue
*            Scheduler Delays and the corresponding rate regulator does not account
*            for MGMT frames through this queue.
*            GT_FALSE, the MGMT frames follow similar rate regulation and delay
*            regulation envelope as specified for the isochronous queue that the
*            MGMT frames are sharing with.
*
* RETURNS:
*        GT_OK      - on success
*        GT_FAIL    - on error
*        GT_NOT_SUPPORTED - if current device does not support this feature.
*
* COMMENTS:
*        None
*
*******************************************************************************/
GT_STATUS gqavGetGlobalDisQSD4MGMT
(
    IN  GT_QD_DEV     *dev,
    OUT GT_BOOL        *en
)
{
    GT_STATUS           retVal;
    GT_PTP_OPERATION    op;
    GT_PTP_OP_DATA        opData;

    DBG_INFO(("gqavGetGlobalDisQSD4MGMT Called.\n"));

    /* check if device supports this feature */
    if (!IS_IN_DEV_GROUP(dev,DEV_QAV))
    {
        DBG_INFO(("GT_NOT_SUPPORTED\n"));
        return GT_NOT_SUPPORTED;
    }

    opData.ptpBlock = 0x2;    /* QAV register space */
    opData.ptpAddr = 3;

    opData.ptpPort = 0xF;

    op = PTP_READ_DATA;
    if((retVal = ptpOperationPerform(dev, op, &opData)) != GT_OK)
    {
        DBG_INFO(("Failed reading Disable Queue Scheduler Delay for MGMT frames.\n"));
        return GT_FAIL;
    }

    if (opData.ptpData&0x4000)
      *en = GT_TRUE;
    else
      *en = GT_FALSE;

    DBG_INFO(("OK.\n"));
    return GT_OK;
}


/*******************************************************************************
* gqavSetGlobalInterrupt
*
* DESCRIPTION:
*        This routine set QAV interrupt enable,
*        The QAV interrypts include:
*        [GT_QAV_INT_ENABLE_ENQ_LMT_BIT]      # EnQ Limit Interrupt Enable
*        [GT_QAV_INT_ENABLE_ISO_DEL_BIT]      # Iso Delay Interrupt Enable
*        [GT_QAV_INT_ENABLE_ISO_DIS_BIT]      # Iso Discard Interrupt Enable
*        [GT_QAV_INT_ENABLE_ISO_LIMIT_EX_BIT] # Iso Packet Memory Exceeded
*                                              Interrupt Enable
*
* INPUTS:
*        intEn - [GT_QAV_INT_ENABLE_ENQ_LMT_BIT] OR
*                [GT_QAV_INT_ENABLE_ISO_DEL_BIT] OR
*                [GT_QAV_INT_ENABLE_ISO_DIS_BIT] OR
*                [GT_QAV_INT_ENABLE_ISO_LIMIT_EX_BIT]
*
* OUTPUTS:
*        None.
*
* RETURNS:
*        GT_OK      - on success
*        GT_FAIL    - on error
*        GT_BAD_PARAM - if input parameters are beyond range.
*        GT_NOT_SUPPORTED - if current device does not support this feature.
*
* COMMENTS:
*        None
*
*******************************************************************************/
GT_STATUS gqavSetGlobalInterrupt
(
    IN  GT_QD_DEV     *dev,
    IN  GT_U16        intEn
)
{
    GT_STATUS           retVal;
    GT_PTP_OPERATION    op;
    GT_PTP_OP_DATA        opData;

    DBG_INFO(("gqavSetGlobalInterrupt Called.\n"));

    /* check if device supports this feature */
    if (!IS_IN_DEV_GROUP(dev,DEV_QAV))
    {
        DBG_INFO(("GT_NOT_SUPPORTED\n"));
        return GT_NOT_SUPPORTED;
    }

    opData.ptpBlock = 0x2;    /* QAV register space */
    opData.ptpAddr = 8;

    opData.ptpPort = 0xF;

    op = PTP_READ_DATA;

    if((retVal = ptpOperationPerform(dev, op, &opData)) != GT_OK)
    {
        DBG_INFO(("Failed reading Interrupt enable status.\n"));
        return GT_FAIL;
    }

    op = PTP_WRITE_DATA;

    opData.ptpData &= ~0xffff;
    if (IS_IN_DEV_GROUP(dev,DEV_88E6351_AVB_FAMILY))
      opData.ptpData |= (intEn&0x87);
    else
      opData.ptpData |= (intEn&0x03);

    if((retVal = ptpOperationPerform(dev, op, &opData)) != GT_OK)
    {
        DBG_INFO(("Failed writing Interrupt enable status.\n"));
        return GT_FAIL;
    }

    DBG_INFO(("OK.\n"));
    return GT_OK;
}

/*******************************************************************************
* gqavGetGlobalInterrupt
*
* DESCRIPTION:
*       This routine get QAV interrupt status and enable status,
*        The QAV interrypt status include:
*         [GT_QAV_INT_STATUS_ENQ_LMT_BIT]      # Enqueue Delay Limit exceeded
*         [GT_QAV_INT_STATUS_ISO_DEL_BIT]      # Iso Delay Interrupt Status
*         [GT_QAV_INT_STATUS_ISO_DIS_BIT]      # Iso Discard Interrupt Status
*         [GT_QAV_INT_STATUS_ISO_LIMIT_EX_BIT] # Iso Packet Memory Exceeded
*                                                Interrupt Status
*        The QAV interrypt enable status include:
*         [GT_QAV_INT_ENABLE_ENQ_LMT_BIT]      # EnQ Limit Interrupt Enable
*         [GT_QAV_INT_ENABLE_ISO_DEL_BIT]      # Iso Delay Interrupt Enable
*         [GT_QAV_INT_ENABLE_ISO_DIS_BIT]      # Iso Discard Interrupt Enable
*         [GT_QAV_INT_ENABLE_ISO_LIMIT_EX_BIT] # Iso Packet Memory Exceeded
*                                                  Interrupt Enable
*
* INPUTS:
*        None.
*
* OUTPUTS:
*        intEnSt - [GT_QAV_INT_STATUS_ENQ_LMT_BIT] OR
*                [GT_QAV_INT_STATUS_ISO_DEL_BIT] OR
*                [GT_QAV_INT_STATUS_ISO_DIS_BIT] OR
*                [GT_QAV_INT_STATUS_ISO_LIMIT_EX_BIT] OR
*                [GT_QAV_INT_ENABLE_ENQ_LMT_BIT] OR
*                [GT_QAV_INT_ENABLE_ISO_DEL_BIT] OR
*                [GT_QAV_INT_ENABLE_ISO_DIS_BIT] OR
*                [GT_QAV_INT_ENABLE_ISO_LIMIT_EX_BIT]
*
* RETURNS:
*        GT_OK      - on success
*        GT_FAIL    - on error
*        GT_BAD_PARAM - if input parameters are beyond range.
*        GT_NOT_SUPPORTED - if current device does not support this feature.
*
* COMMENTS:
*        None
*
*******************************************************************************/
GT_STATUS gqavGetGlobalInterrupt
(
    IN  GT_QD_DEV     *dev,
    OUT GT_U16        *intEnSt
)
{
    GT_STATUS           retVal;
    GT_PTP_OPERATION    op;
    GT_PTP_OP_DATA        opData;

    DBG_INFO(("gqavGetGlobalInterrupt Called.\n"));

    /* check if device supports this feature */
    if (!IS_IN_DEV_GROUP(dev,DEV_QAV))
    {
        DBG_INFO(("GT_NOT_SUPPORTED\n"));
        return GT_NOT_SUPPORTED;
    }

    opData.ptpBlock = 0x2;    /* QAV register space */
    opData.ptpAddr = 8;

    opData.ptpPort = 0xF;

    op = PTP_READ_DATA;
    if((retVal = ptpOperationPerform(dev, op, &opData)) != GT_OK)
    {
        DBG_INFO(("Failed reading Interrupt status.\n"));
        return GT_FAIL;
    }

    if (IS_IN_DEV_GROUP(dev,DEV_88E6351_AVB_FAMILY))
      *intEnSt = (GT_U16)opData.ptpData & 0x8787;
    else
      *intEnSt = (GT_U16)opData.ptpData & 0x0303;

    DBG_INFO(("OK.\n"));
    return GT_OK;
}

/*******************************************************************************
* gqavGetGlobalIsoInterruptPort
*
* DESCRIPTION:
*        This routine get Isochronous interrupt port.
*        This field indicates the port number for IsoDisInt or IsoLimitExInt
*        bits. Only one such interrupt condition can be detected by hardware at one
*        time. Once an interrupt bit has been set along with the IsoIntPort, the
*        software would have to come and clear the bits before hardware records
*        another interrupt event.
*
* INPUTS:
*        None.
*
* OUTPUTS:
*        port - port number for IsoDisInt or IsoLimitExInt bits.
*
* RETURNS:
*        GT_OK      - on success
*        GT_FAIL    - on error
*        GT_BAD_PARAM - if input parameters are beyond range.
*        GT_NOT_SUPPORTED - if current device does not support this feature.
*
* COMMENTS:
*        None
*
*******************************************************************************/
GT_STATUS gqavGetGlobalIsoInterruptPort
(
    IN  GT_QD_DEV     *dev,
    OUT GT_U8        *port
)
{
    GT_STATUS           retVal;
    GT_PTP_OPERATION    op;
    GT_PTP_OP_DATA        opData;

    DBG_INFO(("gqavGetGlobalIsoInterruptPort Called.\n"));

    /* check if device supports this feature */
    if (!IS_IN_DEV_GROUP(dev,DEV_QAV))
    {
        DBG_INFO(("GT_NOT_SUPPORTED\n"));
        return GT_NOT_SUPPORTED;
    }

    opData.ptpBlock = 0x2;    /* QAV register space */
    opData.ptpAddr = 9;

    opData.ptpPort = 0xF;

    op = PTP_READ_DATA;
    if((retVal = ptpOperationPerform(dev, op, &opData)) != GT_OK)
    {
        DBG_INFO(("Failed reading Isochronous interrupt port..\n"));
        return GT_FAIL;
    }

    *port = (GT_U8)opData.ptpData&0xf;

    DBG_INFO(("OK.\n"));
    return GT_OK;
}


/*******************************************************************************
* gqavSetGlobalIsoDelayLmt
*
* DESCRIPTION:
*        This routine set Isochronous queue delay Limit
*        This field represents a per-port isochronous delay limit that
*        will be checked by the queue controller logic to ensure no isochronous
*        packets suffer more than this delay w.r.t to their eligibility time slot.
*        This represents the number of Queue Time Slots. The interval for the QTS
*        can be configured using the register in Qav Global Configuration, Offset 0x2.
*
* INPUTS:
*        limit - per-port isochronous delay limit.
*
* OUTPUTS:
*        None.
*
* RETURNS:
*        GT_OK      - on success
*        GT_FAIL    - on error
*        GT_BAD_PARAM - if input parameters are beyond range.
*        GT_NOT_SUPPORTED - if current device does not support this feature.
*
* COMMENTS:
*        None
*
*******************************************************************************/
GT_STATUS gqavSetGlobalIsoDelayLmt
(
    IN  GT_QD_DEV     *dev,
    IN  GT_U8        limit
)
{
    GT_STATUS           retVal;
    GT_PTP_OPERATION    op;
    GT_PTP_OP_DATA        opData;

    DBG_INFO(("gqavSetGlobalIsoDelayLmt Called.\n"));

    /* check if device supports this feature */
    if (!IS_IN_DEV_GROUP(dev,DEV_QAV))
    {
        DBG_INFO(("GT_NOT_SUPPORTED\n"));
        return GT_NOT_SUPPORTED;
    }
    if (!IS_IN_DEV_GROUP(dev,DEV_QAV_ISO_DELAY_LIMIT))
    {
        DBG_INFO(("GT_NOT_SUPPORTED\n"));
        return GT_NOT_SUPPORTED;
    }

    opData.ptpBlock = 0x2;    /* QAV register space */
    opData.ptpAddr = 10;

    opData.ptpPort = 0xF;

    op = PTP_READ_DATA;
    if((retVal = ptpOperationPerform(dev, op, &opData)) != GT_OK)
    {
        DBG_INFO(("Failed reading Isochronous queue delay Limit.\n"));
        return GT_FAIL;
    }

    op = PTP_WRITE_DATA;

    opData.ptpData &= ~0xff;
    opData.ptpData |= (limit&0xff);

    if((retVal = ptpOperationPerform(dev, op, &opData)) != GT_OK)
    {
        DBG_INFO(("Failed writing Isochronous queue delay Limit.\n"));
        return GT_FAIL;
    }

    DBG_INFO(("OK.\n"));
    return GT_OK;
}

/*******************************************************************************
* gqavGetGlobalIsoDelayLmt
*
* DESCRIPTION:
*        This routine get Isochronous queue delay Limit
*        This field represents a per-port isochronous delay limit that
*        will be checked by the queue controller logic to ensure no isochronous
*        packets suffer more than this delay w.r.t to their eligibility time slot.
*        This represents the number of Queue Time Slots. The interval for the QTS
*        can be configured using the register in Qav Global Configuration, Offset 0x2.
*
* INPUTS:
*        None.
*
* OUTPUTS:
*        limit - per-port isochronous delay limit.
*
* RETURNS:
*        GT_OK      - on success
*        GT_FAIL    - on error
*        GT_BAD_PARAM - if input parameters are beyond range.
*        GT_NOT_SUPPORTED - if current device does not support this feature.
*
* COMMENTS:
*        None
*
*******************************************************************************/
GT_STATUS gqavGetGlobalIsoDelayLmt
(
    IN  GT_QD_DEV     *dev,
    OUT GT_U8        *limit
)
{
    GT_STATUS           retVal;
    GT_PTP_OPERATION    op;
    GT_PTP_OP_DATA        opData;

    DBG_INFO(("gqavGetGlobalIsoDelayLmt Called.\n"));

    /* check if device supports this feature */
    if (!IS_IN_DEV_GROUP(dev,DEV_QAV))
    {
        DBG_INFO(("GT_NOT_SUPPORTED\n"));
        return GT_NOT_SUPPORTED;
    }
    if (!IS_IN_DEV_GROUP(dev,DEV_QAV_ISO_DELAY_LIMIT))
    {
        DBG_INFO(("GT_NOT_SUPPORTED\n"));
        return GT_NOT_SUPPORTED;
    }

    opData.ptpBlock = 0x2;    /* QAV register space */
    opData.ptpAddr = 10;

    opData.ptpPort = 0xF;

    op = PTP_READ_DATA;
    if((retVal = ptpOperationPerform(dev, op, &opData)) != GT_OK)
    {
        DBG_INFO(("Failed reading Isochronous queue delay Limit.\n"));
        return GT_FAIL;
    }

    *limit = (GT_U8)(opData.ptpData)&0xff;

    DBG_INFO(("OK.\n"));
    return GT_OK;
}

/*******************************************************************************
* gqavSetGlobalIsoMonEn
*
* DESCRIPTION:
*       This routine set Isochronous monitor enable
*        Set GT_TRUE: this bit enables the statistics gathering capabilities stated
*        in PTP Global Status Registers Offset 0xD, 0xE and 0xF. Once enabled, the
*        software is expected to program the IsoMonPort (PTP Global Status Offset
*        0xD) indicating which port of the device does the software wants to monitor.
*        Upon setting this bit, the hardware collects IsoHiDisCtr, IsoLoDisCtr and
*        IsoSchMissCtr values for the port indicated by IsoMonPort till this bit is
*        set to a zero.
*        Set GT_FALSE: this bit disables the statistics gathering capabilities.
*
* INPUTS:
*        en - GT_TRUE / GT_FALSE.
*
* OUTPUTS:
*        None.
*
* RETURNS:
*        GT_OK      - on success
*        GT_FAIL    - on error
*        GT_BAD_PARAM - if input parameters are beyond range.
*        GT_NOT_SUPPORTED - if current device does not support this feature.
*
* COMMENTS:
*        None
*
*******************************************************************************/
GT_STATUS gqavSetGlobalIsoMonEn
(
    IN  GT_QD_DEV     *dev,
    IN  GT_BOOL        en
)
{
    GT_STATUS           retVal;
    GT_PTP_OPERATION    op;
    GT_PTP_OP_DATA        opData;

    DBG_INFO(("gqavSetGlobalIsoMonEn Called.\n"));

    /* check if device supports this feature */
    if (!IS_IN_DEV_GROUP(dev,DEV_QAV))
    {
        DBG_INFO(("GT_NOT_SUPPORTED\n"));
        return GT_NOT_SUPPORTED;
    }

    opData.ptpBlock = 0x2;    /* QAV register space */
    opData.ptpAddr = 12;

    opData.ptpPort = 0xF;

    op = PTP_READ_DATA;
    if((retVal = ptpOperationPerform(dev, op, &opData)) != GT_OK)
    {
        DBG_INFO(("Failed reading Isochronous monitor enable.\n"));
        return GT_FAIL;
    }

    op = PTP_WRITE_DATA;

    opData.ptpData &= ~0x8000;
    if (en)
        opData.ptpData |= 0x8000;

    if((retVal = ptpOperationPerform(dev, op, &opData)) != GT_OK)
    {
        DBG_INFO(("Failed writing Isochronous monitor enable.\n"));
        return GT_FAIL;
    }

    DBG_INFO(("OK.\n"));
    return GT_OK;
}

/*******************************************************************************
* gqavGetGlobalIsoMonEn
*
* DESCRIPTION:
*        This routine get Isochronous monitor enable
*        Set GT_TRUE: this bit enables the statistics gathering capabilities stated
*        in PTP Global Status Registers Offset 0xD, 0xE and 0xF. Once enabled, the
*        software is expected to program the IsoMonPort (PTP Global Status Offset
*        0xD) indicating which port of the device does the software wants to monitor.
*        Upon setting this bit, the hardware collects IsoHiDisCtr, IsoLoDisCtr and
*        IsoSchMissCtr values for the port indicated by IsoMonPort till this bit is
*        set to a zero.
*        Set GT_FALSE: this bit disables the statistics gathering capabilities.
*
* INPUTS:
*        None.
*
* OUTPUTS:
*        en - GT_TRUE / GT_FALSE.
*
* RETURNS:
*        GT_OK      - on success
*        GT_FAIL    - on error
*        GT_BAD_PARAM - if input parameters are beyond range.
*        GT_NOT_SUPPORTED - if current device does not support this feature.
*
* COMMENTS:
*        None
*
*******************************************************************************/
GT_STATUS gqavGetGlobalIsoMonEn
(
    IN  GT_QD_DEV     *dev,
    OUT GT_BOOL        *en
)
{
    GT_STATUS           retVal;
    GT_PTP_OPERATION    op;
    GT_PTP_OP_DATA        opData;

    DBG_INFO(("gqavGetGlobalIsoMonEn Called.\n"));

    /* check if device supports this feature */
    if (!IS_IN_DEV_GROUP(dev,DEV_QAV))
    {
        DBG_INFO(("GT_NOT_SUPPORTED\n"));
        return GT_NOT_SUPPORTED;
    }

    opData.ptpBlock = 0x2;    /* QAV register space */
    opData.ptpAddr = 12;

    opData.ptpPort = 0xF;

    op = PTP_READ_DATA;
    if((retVal = ptpOperationPerform(dev, op, &opData)) != GT_OK)
    {
        DBG_INFO(("Failed reading Isochronous monitor enable.\n"));
        return GT_FAIL;
    }

    if (opData.ptpData&0x8000)
      *en = 1;
    else
      *en = 0;

    DBG_INFO(("OK.\n"));
    return GT_OK;
}

/*******************************************************************************
* gqavSetGlobalIsoMonPort
*
* DESCRIPTION:
*        This routine set Isochronous monitoring port.
*        This field is updated by software along with Iso Mon En bit
*        (Qav Global Status, offset 0xD) and it indicates the port number that
*        the software wants the hardware to start monitoring i.e., start updating
*        IsoHiDisCtr, IsoLoDisCtr and IsoSchMissCtr. The queue controller clears
*        the above stats when IsoMonPort is changed..
*
* INPUTS:
*        port -  port number .
*
* OUTPUTS:
*        None.
*
* RETURNS:
*        GT_OK      - on success
*        GT_FAIL    - on error
*        GT_BAD_PARAM - if input parameters are beyond range.
*        GT_NOT_SUPPORTED - if current device does not support this feature.
*
* COMMENTS:
*        None
*
*******************************************************************************/
GT_STATUS gqavSetGlobalIsoMonPort
(
    IN  GT_QD_DEV     *dev,
    IN  GT_U16        port
)
{
    GT_STATUS           retVal;
    GT_PTP_OPERATION    op;
    GT_PTP_OP_DATA        opData;

    DBG_INFO(("gqavSetGlobalIsoMonPort Called.\n"));

    /* check if device supports this feature */
    if (!IS_IN_DEV_GROUP(dev,DEV_QAV))
    {
        DBG_INFO(("GT_NOT_SUPPORTED\n"));
        return GT_NOT_SUPPORTED;
    }

    /* check if port is beyond range */
    if (port>0xf)
    {
        DBG_INFO(("GT_BAD_PARAM\n"));
        return GT_BAD_PARAM;
    }

    opData.ptpBlock = 0x2;    /* QAV register space */
    opData.ptpAddr = 12;

    opData.ptpPort = 0xF;

    op = PTP_READ_DATA;
    if((retVal = ptpOperationPerform(dev, op, &opData)) != GT_OK)
    {
        DBG_INFO(("Failed reading Isochronous monitoring port.\n"));
        return GT_FAIL;
    }

    op = PTP_WRITE_DATA;

    opData.ptpData &= ~0xf;
    opData.ptpData |= port;

    if((retVal = ptpOperationPerform(dev, op, &opData)) != GT_OK)
    {
        DBG_INFO(("Failed writing Isochronous monitoring port.\n"));
        return GT_FAIL;
    }

    DBG_INFO(("OK.\n"));
    return GT_OK;
}


/*******************************************************************************
* gqavGetGlobalIsoMonPort
*
* DESCRIPTION:
*        This routine get Isochronous monitoring port.
*        This field is updated by software along with Iso Mon En bit
*        (Qav Global Status, offset 0xD) and it indicates the port number that
*        the software wants the hardware to start monitoring i.e., start updating
*        IsoHiDisCtr, IsoLoDisCtr and IsoSchMissCtr. The queue controller clears
*        the above stats when IsoMonPort is changed..
*
* INPUTS:
*        None.
*
* OUTPUTS:
*        port -  port number.
*
* RETURNS:
*        GT_OK      - on success
*        GT_FAIL    - on error
*        GT_BAD_PARAM - if input parameters are beyond range.
*        GT_NOT_SUPPORTED - if current device does not support this feature.
*
* COMMENTS:
*        None
*
*******************************************************************************/
GT_STATUS gqavGetGlobalIsoMonPort
(
    IN  GT_QD_DEV     *dev,
    OUT GT_U16        *port
)
{
    GT_STATUS           retVal;
    GT_PTP_OPERATION    op;
    GT_PTP_OP_DATA        opData;

    DBG_INFO(("gqavGetGlobalIsoMonPort Called.\n"));

    /* check if device supports this feature */
    if (!IS_IN_DEV_GROUP(dev,DEV_QAV))
    {
        DBG_INFO(("GT_NOT_SUPPORTED\n"));
        return GT_NOT_SUPPORTED;
    }

    opData.ptpBlock = 0x2;    /* QAV register space */
    opData.ptpAddr = 12;

    opData.ptpPort = 0xF;

    op = PTP_READ_DATA;
    if((retVal = ptpOperationPerform(dev, op, &opData)) != GT_OK)
    {
        DBG_INFO(("Failed reading Isochronous monitoring port.\n"));
        return GT_FAIL;
    }

    *port = (GT_U16)opData.ptpData&0xf;

    DBG_INFO(("OK.\n"));
    return GT_OK;
}

/*******************************************************************************
* gqavSetGlobalIsoHiDisCtr
*
* DESCRIPTION:
*        This routine set Isochronous hi queue discard counter.
*        This field is updated by hardware when instructed to do so by
*        enabling the IsoMonEn bit in Qav Global Status Register Offset 0xD.
*        This is an upcounter of number of isochronous hi packets discarded
*        by Queue Controller.
*
* INPUTS:
*        disCtr - upcounter of number of isochronous hi packets discarded
*
* OUTPUTS:
*        None.
*
* RETURNS:
*        GT_OK      - on success
*        GT_FAIL    - on error
*        GT_BAD_PARAM - if input parameters are beyond range.
*        GT_NOT_SUPPORTED - if current device does not support this feature.
*
* COMMENTS:
*        None
*
*******************************************************************************/
GT_STATUS gqavSetGlobalIsoHiDisCtr
(
    IN  GT_QD_DEV     *dev,
    IN  GT_U8        disCtr
)
{
    GT_STATUS           retVal;
    GT_PTP_OPERATION    op;
    GT_PTP_OP_DATA        opData;

    DBG_INFO(("gqavSetGlobalIsoHiDisCtr Called.\n"));

    /* check if device supports this feature */
    if (!IS_IN_DEV_GROUP(dev,DEV_QAV))
    {
        DBG_INFO(("GT_NOT_SUPPORTED\n"));
        return GT_NOT_SUPPORTED;
    }

    opData.ptpBlock = 0x2;    /* QAV register space */
    opData.ptpAddr = 13;

    opData.ptpPort = 0xF;

    op = PTP_READ_DATA;
    if((retVal = ptpOperationPerform(dev, op, &opData)) != GT_OK)
    {
        DBG_INFO(("Failed reading Isochronous hi queue discard counter..\n"));
        return GT_FAIL;
    }

    op = PTP_WRITE_DATA;

    opData.ptpData &= ~0xff00;
    if (disCtr)
        opData.ptpData |= (disCtr<<8);

    if((retVal = ptpOperationPerform(dev, op, &opData)) != GT_OK)
    {
        DBG_INFO(("Failed writing Isochronous hi queue discard counter..\n"));
        return GT_FAIL;
    }

    DBG_INFO(("OK.\n"));
    return GT_OK;
}

/*******************************************************************************
* gqavGetGlobalIsoHiDisCtr
*
* DESCRIPTION:
*        This routine get Isochronous hi queue discard counter.
*        This field is updated by hardware when instructed to do so by
*        enabling the IsoMonEn bit in Qav Global Status Register Offset 0xD.
*        This is an upcounter of number of isochronous hi packets discarded
*        by Queue Controller.
*
* INPUTS:
*        None.
*
* OUTPUTS:
*        disCtr - upcounter of number of isochronous hi packets discarded
*
* RETURNS:
*        GT_OK      - on success
*        GT_FAIL    - on error
*        GT_BAD_PARAM - if input parameters are beyond range.
*        GT_NOT_SUPPORTED - if current device does not support this feature.
*
* COMMENTS:
*        None
*
*******************************************************************************/
GT_STATUS gqavGetGlobalIsoHiDisCtr
(
    IN  GT_QD_DEV     *dev,
    OUT GT_U8        *disCtr
)
{
    GT_STATUS           retVal;
    GT_PTP_OPERATION    op;
    GT_PTP_OP_DATA        opData;

    DBG_INFO(("gqavGetGlobalIsoHiDisCtr Called.\n"));

    /* check if device supports this feature */
    if (!IS_IN_DEV_GROUP(dev,DEV_QAV))
    {
        DBG_INFO(("GT_NOT_SUPPORTED\n"));
        return GT_NOT_SUPPORTED;
    }

    opData.ptpBlock = 0x2;    /* QAV register space */
    opData.ptpAddr = 13;

    opData.ptpPort = 0xF;

    op = PTP_READ_DATA;
    if((retVal = ptpOperationPerform(dev, op, &opData)) != GT_OK)
    {
        DBG_INFO(("Failed reading Isochronous hi queue discard counter.\n"));
        return GT_FAIL;
    }

    *disCtr = (GT_U8)(opData.ptpData>>8)&0xff;

    DBG_INFO(("OK.\n"));
    return GT_OK;
}


/*******************************************************************************
* gqavSetGlobalIsoLoDisCtr
*
* DESCRIPTION:
*        This routine set Isochronous Lo queue discard counter.
*        This field is updated by hardware when instructed to do so by
*        enabling the IsoMonEn bit in Qav Global Status Register Offset 0xD.
*        This is an upcounter of number of isochronous lo packets discarded
*        by Queue Controller.
*
* INPUTS:
*        disCtr - upcounter of number of isochronous lo packets discarded
*
* OUTPUTS:
*        None.
*
* RETURNS:
*        GT_OK      - on success
*        GT_FAIL    - on error
*        GT_BAD_PARAM - if input parameters are beyond range.
*        GT_NOT_SUPPORTED - if current device does not support this feature.
*
* COMMENTS:
*        None
*
*******************************************************************************/
GT_STATUS gqavSetGlobalIsoLoDisCtr
(
    IN  GT_QD_DEV     *dev,
    IN  GT_U8        disCtr
)
{
    GT_STATUS           retVal;
    GT_PTP_OPERATION    op;
    GT_PTP_OP_DATA        opData;

    DBG_INFO(("gqavSetGlobalIsoLoDisCtr Called.\n"));

    /* check if device supports this feature */
    if (!IS_IN_DEV_GROUP(dev,DEV_QAV))
    {
        DBG_INFO(("GT_NOT_SUPPORTED\n"));
        return GT_NOT_SUPPORTED;
    }

    opData.ptpBlock = 0x2;    /* QAV register space */
    opData.ptpAddr = 13;

    opData.ptpPort = 0xF;

    op = PTP_READ_DATA;
    if((retVal = ptpOperationPerform(dev, op, &opData)) != GT_OK)
    {
        DBG_INFO(("Failed reading Isochronous lo queue discard counter.\n"));
        return GT_FAIL;
    }

    op = PTP_WRITE_DATA;

    opData.ptpData &= ~0xff;
    opData.ptpData |= (disCtr&0xff);

    if((retVal = ptpOperationPerform(dev, op, &opData)) != GT_OK)
    {
        DBG_INFO(("Failed writing Isochronous lo queue discard counter.\n"));
        return GT_FAIL;
    }

    DBG_INFO(("OK.\n"));
    return GT_OK;
}


/*******************************************************************************
* gqavGetGlobalIsoLoDisCtr
*
* DESCRIPTION:
*        This routine set Isochronous Lo queue discard counter.
*        This field is updated by hardware when instructed to do so by
*        enabling the IsoMonEn bit in Qav Global Status Register Offset 0xD.
*        This is an upcounter of number of isochronous lo packets discarded
*        by Queue Controller.
*
* INPUTS:
*        None.
*
* OUTPUTS:
*        disCtr - upcounter of number of isochronous lo packets discarded
*
* RETURNS:
*        GT_OK      - on success
*        GT_FAIL    - on error
*        GT_BAD_PARAM - if input parameters are beyond range.
*        GT_NOT_SUPPORTED - if current device does not support this feature.
*
* COMMENTS:
*        None
*
*******************************************************************************/
GT_STATUS gqavGetGlobalIsoLoDisCtr
(
    IN  GT_QD_DEV     *dev,
    OUT GT_U8        *disCtr
)
{
    GT_STATUS           retVal;
    GT_PTP_OPERATION    op;
    GT_PTP_OP_DATA        opData;

    DBG_INFO(("gqavGetGlobalIsoLoDisCtr Called.\n"));

    /* check if device supports this feature */
    if (!IS_IN_DEV_GROUP(dev,DEV_QAV))
    {
        DBG_INFO(("GT_NOT_SUPPORTED\n"));
        return GT_NOT_SUPPORTED;
    }

    opData.ptpBlock = 0x2;    /* QAV register space */
    opData.ptpAddr = 13;

    opData.ptpPort = 0xF;

    op = PTP_READ_DATA;
    if((retVal = ptpOperationPerform(dev, op, &opData)) != GT_OK)
    {
        DBG_INFO(("Failed reading Isochronous lo queue discard counter.\n"));
        return GT_FAIL;
    }

    *disCtr = (GT_U8)(opData.ptpData)&0xff;

    DBG_INFO(("OK.\n"));
    return GT_OK;
}
