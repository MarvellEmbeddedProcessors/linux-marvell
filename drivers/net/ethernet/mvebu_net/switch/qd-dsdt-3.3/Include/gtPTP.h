#ifdef CONFIG_AVB_FPGA

/*******************************************************************************
* gptpGetFPGAIntStatus
*
* DESCRIPTION:
*       This routine gets interrupt status of PTP logic.
*
* INPUTS:
*       None.
*
* OUTPUTS:
*        ptpInt    - PTP Int Status
*
* RETURNS:
*       GT_OK      - on success
*       GT_FAIL    - on error
*       GT_NOT_SUPPORTED - if current device does not support this feature.
*
* COMMENTS:
*       None
*
*******************************************************************************/
GT_STATUS gptpGetFPGAIntStatus
(
    IN  GT_QD_DEV     *dev,
    OUT GT_U32        *ptpInt
);

/*******************************************************************************
* gptpSetFPGAIntStatus
*
* DESCRIPTION:
*       This routine sets interrupt status of PTP logic.
*
* INPUTS:
*    ptpInt    - PTP Int Status
*
* OUTPUTS:
*       None.
*
* RETURNS:
*       GT_OK      - on success
*       GT_FAIL    - on error
*       GT_NOT_SUPPORTED - if current device does not support this feature.
*
* COMMENTS:
*       None
*
*******************************************************************************/
GT_STATUS gptpSetFPGAIntStatus
(
    IN  GT_QD_DEV     *dev,
    OUT GT_U32    ptpInt
);

/*******************************************************************************
* gptpSetFPGAIntEn
*
* DESCRIPTION:
*       This routine enables PTP interrupt.
*
* INPUTS:
*        ptpInt    - PTP Int Status (1 to enable, 0 to disable)
*
* OUTPUTS:
*       None.
*
* RETURNS:
*       GT_OK      - on success
*       GT_FAIL    - on error
*       GT_NOT_SUPPORTED - if current device does not support this feature.
*
* COMMENTS:
*       None
*
*******************************************************************************/
GT_STATUS gptpSetFPGAIntEn
(
    IN  GT_QD_DEV     *dev,
    IN  GT_U32        ptpInt
);

/*******************************************************************************
* gptpGetClockSource
*
* DESCRIPTION:
*       This routine gets PTP Clock source setup.
*
* INPUTS:
*       None.
*
* OUTPUTS:
*        clkSrc    - PTP clock source (A/D Device or FPGA)
*
* RETURNS:
*       GT_OK      - on success
*       GT_FAIL    - on error
*       GT_NOT_SUPPORTED - if current device does not support this feature.
*
* COMMENTS:
*       None
*
*******************************************************************************/
GT_STATUS gptpGetClockSource
(
    IN  GT_QD_DEV     *dev,
    OUT GT_PTP_CLOCK_SRC     *clkSrc
);

/*******************************************************************************
* gptpSetClockSource
*
* DESCRIPTION:
*       This routine sets PTP Clock source setup.
*
* INPUTS:
*        clkSrc    - PTP clock source (A/D Device or FPGA)
*
* OUTPUTS:
*       None.
*
* RETURNS:
*       GT_OK      - on success
*       GT_FAIL    - on error
*       GT_NOT_SUPPORTED - if current device does not support this feature.
*
* COMMENTS:
*       None
*
*******************************************************************************/
GT_STATUS gptpSetClockSource
(
    IN  GT_QD_DEV     *dev,
    IN  GT_PTP_CLOCK_SRC     clkSrc
);

/*******************************************************************************
* gptpGetP9Mode
*
* DESCRIPTION:
*       This routine gets Port 9 Mode.
*
* INPUTS:
*       None.
*
* OUTPUTS:
*        mode - Port 9 mode (GT_PTP_P9_MODE enum type)
*
* RETURNS:
*       GT_OK      - on success
*       GT_FAIL    - on error
*       GT_NOT_SUPPORTED - if current device does not support this feature.
*
* COMMENTS:
*       None
*
*******************************************************************************/
GT_STATUS gptpGetP9Mode
(
    IN  GT_QD_DEV     *dev,
    OUT GT_PTP_P9_MODE     *mode
);

/*******************************************************************************
* gptpSetP9Mode
*
* DESCRIPTION:
*       This routine sets Port 9 Mode.
*
* INPUTS:
*        mode - Port 9 mode (GT_PTP_P9_MODE enum type)
*
* OUTPUTS:
*       None.
*
* RETURNS:
*       GT_OK      - on success
*       GT_FAIL    - on error
*       GT_NOT_SUPPORTED - if current device does not support this feature.
*
* COMMENTS:
*       None
*
*******************************************************************************/
GT_STATUS gptpSetP9Mode
(
    IN  GT_QD_DEV     *dev,
    IN  GT_PTP_P9_MODE     mode
);

/*******************************************************************************
* gptpReset
*
* DESCRIPTION:
*       This routine performs software reset for PTP logic.
*
* INPUTS:
*        None.
*
* OUTPUTS:
*       None.
*
* RETURNS:
*       GT_OK      - on success
*       GT_FAIL    - on error
*       GT_NOT_SUPPORTED - if current device does not support this feature.
*
* COMMENTS:
*       None
*
*******************************************************************************/
GT_STATUS gptpReset
(
    IN  GT_QD_DEV     *dev
);

/*******************************************************************************
* gptpGetCycleAdjustEn
*
* DESCRIPTION:
*       This routine checks if PTP Duty Cycle Adjustment is enabled.
*
* INPUTS:
*       None.
*
* OUTPUTS:
*        adjEn    - GT_TRUE if enabled, GT_FALSE otherwise
*
* RETURNS:
*       GT_OK      - on success
*       GT_FAIL    - on error
*       GT_NOT_SUPPORTED - if current device does not support this feature.
*
* COMMENTS:
*       None
*
*******************************************************************************/
GT_STATUS gptpGetCycleAdjustEn
(
    IN  GT_QD_DEV     *dev,
    OUT GT_BOOL        *adjEn
);

/*******************************************************************************
* gptpSetCycleAdjustEn
*
* DESCRIPTION:
*       This routine enables/disables PTP Duty Cycle Adjustment.
*
* INPUTS:
*        adjEn    - GT_TRUE to enable, GT_FALSE to disable
*
* OUTPUTS:
*       None.
*
* RETURNS:
*       GT_OK      - on success
*       GT_FAIL    - on error
*       GT_NOT_SUPPORTED - if current device does not support this feature.
*
* COMMENTS:
*       None
*
*******************************************************************************/
GT_STATUS gptpSetCycleAdjustEn
(
    IN  GT_QD_DEV     *dev,
    IN  GT_BOOL        adjEn
);

/*******************************************************************************
* gptpGetCycleAdjust
*
* DESCRIPTION:
*       This routine gets clock duty cycle adjustment value.
*
* INPUTS:
*       None.
*
* OUTPUTS:
*        adj    - adjustment value (GT_PTP_CLOCK_ADJUSTMENT structure)
*
* RETURNS:
*       GT_OK      - on success
*       GT_FAIL    - on error
*       GT_NOT_SUPPORTED - if current device does not support this feature.
*
* COMMENTS:
*       None
*
*******************************************************************************/
GT_STATUS gptpGetCycleAdjust
(
    IN  GT_QD_DEV     *dev,
    OUT GT_PTP_CLOCK_ADJUSTMENT    *adj
);

/*******************************************************************************
* gptpSetCycleAdjust
*
* DESCRIPTION:
*       This routine sets clock duty cycle adjustment value.
*
* INPUTS:
*        adj    - adjustment value (GT_PTP_CLOCK_ADJUSTMENT structure)
*
* OUTPUTS:
*       None.
*
* RETURNS:
*       GT_OK      - on success
*       GT_FAIL    - on error
*       GT_NOT_SUPPORTED - if current device does not support this feature.
*
* COMMENTS:
*       None
*
*******************************************************************************/
GT_STATUS gptpSetCycleAdjust
(
    IN  GT_QD_DEV     *dev,
    IN  GT_PTP_CLOCK_ADJUSTMENT    *adj
);

/*******************************************************************************
* gptpGetPLLEn
*
* DESCRIPTION:
*       This routine checks if PLL is enabled.
*
* INPUTS:
*       None.
*
* OUTPUTS:
*        en        - GT_TRUE if enabled, GT_FALSE otherwise
*        freqSel    - PLL Frequency Selection (default 0x3 - 22.368MHz)
*
* RETURNS:
*       GT_OK      - on success
*       GT_FAIL    - on error
*       GT_NOT_SUPPORTED - if current device does not support this feature.
*
* COMMENTS:
*       PLL Frequence selection is based on the Clock Recovery PLL device.
*        IDT MK1575-01 is the default PLL device.
*
*******************************************************************************/
GT_STATUS gptpGetPLLEn
(
    IN  GT_QD_DEV     *dev,
    OUT GT_BOOL        *en,
    OUT GT_U32        *freqSel
);

/*******************************************************************************
* gptpSetPLLEn
*
* DESCRIPTION:
*       This routine enables/disables PLL device.
*
* INPUTS:
*        en        - GT_TRUE to enable, GT_FALSE to disable
*        freqSel    - PLL Frequency Selection (default 0x3 - 22.368MHz)
*                  Meaningful only when enabling PLL device
*
* OUTPUTS:
*       None.
*
* RETURNS:
*       GT_OK      - on success
*       GT_FAIL    - on error
*       GT_NOT_SUPPORTED - if current device does not support this feature.
*
* COMMENTS:
*       PLL Frequence selection is based on the Clock Recovery PLL device.
*        IDT MK1575-01 is the default PLL device.
*
*******************************************************************************/
GT_STATUS gptpSetPLLEn
(
    IN  GT_QD_DEV     *dev,
    IN  GT_BOOL        en,
    IN  GT_U32        freqSel
);

/*******************************************************************************
* gptpGetDDSReg
*
* DESCRIPTION:
*       This routine gets DDS register data.
*
* INPUTS:
*    ddsReg    - DDS Register
*
* OUTPUTS:
*    ddsData    - register data
*
* RETURNS:
*       GT_OK      - on success
*       GT_FAIL    - on error
*       GT_NOT_SUPPORTED - if current device does not support this feature.
*
* COMMENTS:
*       None
*
*******************************************************************************/
GT_STATUS gptpGetDDSReg
(
    IN  GT_QD_DEV     *dev,
    IN  GT_U32    ddsReg,
    OUT GT_U32    *ddsData
);

/*******************************************************************************
* gptpSetDDSReg
*
* DESCRIPTION:
*       This routine sets DDS register data.
*    DDS register data written by this API are not affected until gptpUpdateDDSReg API is called.
*
* INPUTS:
*    ddsReg    - DDS Register
*    ddsData    - register data
*
* OUTPUTS:
*    none
*
* RETURNS:
*       GT_OK      - on success
*       GT_FAIL    - on error
*       GT_NOT_SUPPORTED - if current device does not support this feature.
*
* COMMENTS:
*       None
*
*******************************************************************************/
GT_STATUS gptpSetDDSReg
(
    IN  GT_QD_DEV     *dev,
    IN  GT_U32    ddsReg,
    IN  GT_U32    ddsData
);

/*******************************************************************************
* gptpUpdateDDSReg
*
* DESCRIPTION:
*       This routine updates DDS register data.
*    DDS register data written by gptpSetDDSReg are not affected until this API is called.
*
* INPUTS:
*    none
*
* OUTPUTS:
*    none
*
* RETURNS:
*       GT_OK      - on success
*       GT_FAIL    - on error
*       GT_NOT_SUPPORTED - if current device does not support this feature.
*
* COMMENTS:
*       None
*
*******************************************************************************/
GT_STATUS gptpUpdateDDSReg
(
    IN  GT_QD_DEV     *dev
);

/*******************************************************************************
* gptpSetADFReg
*
* DESCRIPTION:
*       This routine sets ADF4156 register data.
*
* INPUTS:
*    adfData    - register data
*
* OUTPUTS:
*    none
*
* RETURNS:
*       GT_OK      - on success
*       GT_FAIL    - on error
*       GT_NOT_SUPPORTED - if current device does not support this feature.
*
* COMMENTS:
*       None
*
*******************************************************************************/
GT_STATUS gptpSetADFReg
(
    IN  GT_QD_DEV     *dev,
    IN  GT_U32    adfData
);

#endif  /*  CONFIG_AVB_FPGA */
