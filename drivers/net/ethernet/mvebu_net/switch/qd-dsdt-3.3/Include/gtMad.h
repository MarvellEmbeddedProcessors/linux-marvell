#include <Copyright.h>
/*******************************************************************************
* gtMad.h
*
* DESCRIPTION:
*       MAD API header file.
*
* DEPENDENCIES:
*       None.
*
* FILE REVISION NUMBER:
*       $Revision: 1 $
*******************************************************************************/
GT_STATUS gvctGetAdvCableDiag_mad
(
    IN  GT_QD_DEV *dev,
    IN  GT_LPORT        port,
    IN  GT_ADV_VCT_MODE mode,
    OUT GT_ADV_CABLE_STATUS *cableStatus
);
GT_STATUS gvctGetAdvExtendedStatus_mad
(
    IN  GT_QD_DEV     *dev,
    IN  GT_LPORT   port,
    OUT GT_ADV_EXTENDED_STATUS *extendedStatus
);
#include <Copyright.h>


GT_STATUS gprtPhyReset_mad
(
    IN GT_QD_DEV *dev,
    IN GT_LPORT  port
);
GT_STATUS gprtSetPortLoopback_mad
(
    IN GT_QD_DEV *dev,
    IN GT_LPORT  port,
    IN GT_BOOL   enable
);

GT_STATUS gprtSetPortSpeed_mad
(
IN GT_QD_DEV *dev,
IN GT_LPORT  port,
IN GT_PHY_SPEED speed
);
GT_STATUS gprtPortAutoNegEnable_mad
(
    IN GT_QD_DEV *dev,
    IN GT_LPORT  port,
    IN GT_BOOL   state
);
GT_STATUS gprtPortPowerDown_mad
(
IN GT_QD_DEV *dev,
IN GT_LPORT  port,
IN GT_BOOL   state
);
GT_STATUS gprtPortRestartAutoNeg_mad
(
IN GT_QD_DEV *dev,
IN GT_LPORT  port
);
GT_STATUS gprtSetPortDuplexMode_mad
(
IN GT_QD_DEV *dev,
IN GT_LPORT  port,
IN GT_BOOL   dMode
);
GT_STATUS gprtSetPortAutoMode_mad
(
    IN GT_QD_DEV *dev,
    IN GT_LPORT  port,
    IN GT_PHY_AUTO_MODE mode
);
GT_STATUS gprtSetPause_mad
(
IN GT_QD_DEV *dev,
IN GT_LPORT  port,
IN GT_PHY_PAUSE_MODE state
);
GT_STATUS gprtSetDTEDetect_mad
(
    IN GT_QD_DEV *dev,
    IN GT_LPORT  port,
    IN GT_BOOL   state
);
GT_STATUS gprtGetDTEDetectStatus_mad
(
    IN  GT_QD_DEV *dev,
    IN  GT_LPORT  port,
    OUT GT_BOOL   *state
);
GT_STATUS gprtSetDTEDetectDropWait_mad
(
    IN  GT_QD_DEV *dev,
    IN  GT_LPORT  port,
    IN  GT_U16    waitTime
);
GT_STATUS gprtGetDTEDetectDropWait_mad
(
    IN  GT_QD_DEV *dev,
    IN  GT_LPORT  port,
    OUT GT_U16    *waitTime
);
GT_STATUS gprtSetEnergyDetect_mad
(
    IN  GT_QD_DEV *dev,
    IN  GT_LPORT  port,
    IN  GT_EDETECT_MODE   mode
);
GT_STATUS gprtGetEnergyDetect_mad
(
    IN  GT_QD_DEV *dev,
    IN  GT_LPORT  port,
    OUT GT_EDETECT_MODE   *mode
);
GT_STATUS gprtSet1000TMasterMode_mad
(
    IN  GT_QD_DEV   *dev,
    IN  GT_LPORT     port,
    IN  GT_1000T_MASTER_SLAVE   *mode
);
GT_STATUS gprtGet1000TMasterMode_mad
(
    IN  GT_QD_DEV   *dev,
    IN  GT_LPORT     port,
    OUT GT_1000T_MASTER_SLAVE   *mode
);
GT_STATUS gprtGetPhyLinkStatus_mad
(
    IN GT_QD_DEV *dev,
    IN GT_LPORT  port,
    IN GT_BOOL      *linkStatus
);
GT_STATUS gprtSetPktGenEnable_mad
(
    IN GT_QD_DEV *dev,
    IN GT_LPORT  port,
    IN GT_BOOL   en,
    IN GT_PG     *pktInfo
);
GT_STATUS gprtGetPhyReg_mad
(
    IN  GT_QD_DEV    *dev,
    IN  GT_LPORT     port,
    IN  GT_U32         regAddr,
    OUT GT_U16         *data
);
GT_STATUS gprtSetPhyReg_mad
(
    IN  GT_QD_DEV        *dev,
    IN  GT_LPORT        port,
    IN  GT_U32            regAddr,
    IN  GT_U16            inData
);
GT_STATUS gprtGetPagedPhyReg_mad
(
    IN  GT_QD_DEV *dev,
    IN  GT_U32  port,
    IN    GT_U32  regAddr,
    IN    GT_U32  page,
    OUT GT_U16* data
);
GT_STATUS gprtSetPagedPhyReg_mad
(
    IN  GT_QD_DEV *dev,
    IN  GT_U32 port,
    IN    GT_U32 regAddr,
    IN    GT_U32 page,
    IN  GT_U16 inData
);
GT_STATUS gprtPhyIntEnable_mad
(
IN GT_QD_DEV    *dev,
IN GT_LPORT    port,
IN GT_U16    intType
);
GT_STATUS gprtGetPhyIntStatus_mad
(
IN   GT_QD_DEV  *dev,
IN   GT_LPORT   port,
OUT  GT_U16*    intType
);
GT_STATUS gprtGetPhyIntPortSummary_mad
(
IN  GT_QD_DEV  *dev,
OUT GT_U16     *intPortMask
);
GT_STATUS gvctGetCableDiag_mad
(
    IN  GT_QD_DEV *dev,
    IN  GT_LPORT        port,
    OUT GT_CABLE_STATUS *cableStatus
);
GT_STATUS gvctGet1000BTExtendedStatus_mad
(
    IN  GT_QD_DEV         *dev,
    IN  GT_LPORT        port,
    OUT GT_1000BT_EXTENDED_STATUS *extendedStatus
);
