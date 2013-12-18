#include <Copyright.h>

/*******************************************************************************
* gtPage2Access.c
*
* DESCRIPTION:
*       API definitions for Page 2 access
*
* DEPENDENCIES:
*
* FILE REVISION NUMBER:
*******************************************************************************/

#include <msApi.h>
#include <gtSem.h>
#include <gtHwCntl.h>
#include <gtDrvSwRegs.h>

/* Set to enable/disable Page 2 rester list */
/*******************************************************************************
* gtP2SetAccessRMUPage2
*
* DESCRIPTION:
*        This routine sets to access registers of page 2(RMU).
*
* INPUTS:
*        access  - TRUE or FALSE
*
* OUTPUTS:
*        None.
*
* RETURNS:
*        GT_OK   - on success
*        GT_FAIL - on error
*        GT_NOT_SUPPORTED - if current device does not support this feature.
*
* COMMENTS:
*
*******************************************************************************/
GT_STATUS gtP2SetAccessRMUPage2
(
    IN GT_QD_DEV    *dev,
    IN GT_BOOL      access
)
{
    GT_U16          data;
    GT_STATUS       retVal;         /* Functions return value.      */

    DBG_INFO(("gtP2SetAccessRMUPage2 Called.\n"));

    if (!IS_IN_DEV_GROUP(dev,DEV_RMU_PAGE2))
    {
        DBG_INFO(("GT_NOT_SUPPORTED\n"));
        return GT_NOT_SUPPORTED;
    }

    data = (access==GT_TRUE) ? 1 : 0; /* bit location? */

    /* Set Page 2 access.            */
    retVal = hwWritePortReg(dev,0x17, 0x1a,data);

    if(retVal != GT_OK)
    {
        DBG_INFO(("Failed.\n"));
    }
    else
    {
        DBG_INFO(("OK.\n"));
    }
    return retVal;
}

/*******************************************************************************
* gtP2GetAccessRMUPage2
*
* DESCRIPTION:
*        This routine gets to access registers of page 2(RMU).
*
* INPUTS:
*        None.
*
* OUTPUTS:
*        access  - TRUE or FALSE
*
* RETURNS:
*        GT_OK   - on success
*        GT_FAIL - on error
*        GT_NOT_SUPPORTED - if current device does not support this feature.
*
* COMMENTS:
*
*******************************************************************************/
GT_STATUS gtP2GetAccessRMUPage2
(
    IN GT_QD_DEV    *dev,
    OUT GT_BOOL     *access
)
{
    GT_U16          data;
    GT_STATUS       retVal;         /* Functions return value.      */

    DBG_INFO(("gtP2GetAccessRMUPage2 Called.\n"));

    if (!IS_IN_DEV_GROUP(dev,DEV_RMU_PAGE2))
    {
        DBG_INFO(("GT_NOT_SUPPORTED\n"));
        return GT_NOT_SUPPORTED;
    }


    /* Get Page 2 access state.            */
    retVal = hwReadPortReg(dev,0x17, 0x1a,&data);

    if(retVal != GT_OK)
    {
        DBG_INFO(("Failed.\n"));
    }
    else
    {
        DBG_INFO(("OK.\n"));
    }
    *access = (data==1) ? GT_TRUE : GT_FALSE; /* bit location? */
    return retVal;
}

/* Page 2 stats APIs */

/****************************************************************************/
/* STATS operation function declaration.                                    */
/****************************************************************************/
static GT_STATUS statsOperationPerform
(
    IN   GT_QD_DEV            *dev,
    IN   GT_STATS_OPERATION   statsOp,
    IN   GT_U8                port,
    IN   GT_STATS_COUNTERS_PAGE2    counter,
    OUT  GT_VOID              *statsData
);

static GT_STATUS statsCapture
(
    IN GT_QD_DEV  *dev,
    IN GT_U8             bank,
    IN GT_U8      port
);

static GT_STATUS statsCaptureClear
(
    IN GT_QD_DEV  *dev,
    IN GT_U8             bank,
    IN GT_U8      port
);

static GT_STATUS statsReadCounter
(
    IN   GT_QD_DEV        *dev,
    IN   GT_U32            counter,
    OUT  GT_U32            *statsData
);

static GT_STATUS statsReadRealtimeCounter
(
    IN   GT_QD_DEV      *dev,
    IN   GT_U8             port,
    IN   GT_U32            counter,
    OUT  GT_U32            *statsData
);



/*******************************************************************************
* gstatsPg2GetPortCounter
*
* DESCRIPTION:
*        This routine gets a specific counter of the given port
*
* INPUTS:
*        port - the logical port number.
*        counter - the counter which will be read
*
* OUTPUTS:
*        statsData - points to 32bit data storage for the MIB counter
*
* RETURNS:
*        GT_OK      - on success
*        GT_FAIL    - on error
*
* COMMENTS:
*        None
*
* GalTis:
*
*******************************************************************************/
GT_STATUS gstatsPg2GetPortCounter
(
    IN  GT_QD_DEV        *dev,
    IN  GT_LPORT        port,
    IN  GT_STATS_COUNTERS_PAGE2    counter,
    OUT GT_U32            *statsData
)
{
    GT_STATUS    retVal;
    GT_U8        hwPort;         /* physical port number         */

    DBG_INFO(("gstatsPg2GetPortCounter Called.\n"));

    /* translate logical port to physical port */
    hwPort = GT_LPORT_2_PORT(port);

    /* check if device supports this feature */
    if((retVal = IS_VALID_API_CALL(dev,hwPort, DEV_RMON)) != GT_OK)
    {
        return retVal;
    }

    /* Gigabit Switch does not support this status. */
    if (!IS_IN_DEV_GROUP(dev,DEV_RMON_TYPE_3))
    {
        DBG_INFO(("GT_NOT_SUPPORTED\n"));
        return GT_NOT_SUPPORTED;
    }

	/* Enter Page2 access */
    retVal = gtP2SetAccessRMUPage2(dev, GT_TRUE);
    if(retVal != GT_OK)
    {
        DBG_INFO(("Failed (gtP2SetAccessRMUPage2 returned GT_FAIL).\n"));
        return retVal;
    }

    retVal = statsOperationPerform(dev,STATS_READ_COUNTER,hwPort,counter,(GT_VOID*)statsData);
    if(retVal != GT_OK)
    {
        DBG_INFO(("Failed (statsOperationPerform returned GT_FAIL).\n"));
        return retVal;
    }

	/* Exit Page2 access */
    retVal = gtP2SetAccessRMUPage2(dev, GT_FALSE);
    if(retVal != GT_OK)
    {
        DBG_INFO(("Failed (gtP2SetAccessRMUPage2 returned GT_FAIL).\n"));
        return retVal;
    }

    DBG_INFO(("OK.\n"));
    return GT_OK;

}

/*******************************************************************************
* gstatsPg2GetPortCounterClear
*
* DESCRIPTION:
*        This routine gets a specific counter of the given port and clear.
*
* INPUTS:
*        port - the logical port number.
*        counter - the counter which will be read
*
* OUTPUTS:
*        statsData - points to 32bit data storage for the MIB counter
*
* RETURNS:
*        GT_OK      - on success
*        GT_FAIL    - on error
*
* COMMENTS:
*        None
*
* GalTis:
*
*******************************************************************************/
GT_STATUS gstatsPg2GetPortCounterClear
(
    IN  GT_QD_DEV        *dev,
    IN  GT_LPORT        port,
    IN  GT_STATS_COUNTERS_PAGE2    counter,
    OUT GT_U32            *statsData
)
{
    GT_STATUS    retVal;
    GT_U8        hwPort;         /* physical port number         */

    DBG_INFO(("gstatsPg2GetPortCounterClear Called.\n"));

    /* translate logical port to physical port */
    hwPort = GT_LPORT_2_PORT(port);

    /* check if device supports this feature */
    if((retVal = IS_VALID_API_CALL(dev,hwPort, DEV_RMON)) != GT_OK)
    {
        return retVal;
    }

    /* Gigabit Switch does not support this status. */
    if (!IS_IN_DEV_GROUP(dev,DEV_RMON_TYPE_3))
    {
        DBG_INFO(("GT_NOT_SUPPORTED\n"));
        return GT_NOT_SUPPORTED;
    }

	/* Enter Page2 access */
    retVal = gtP2SetAccessRMUPage2(dev, GT_TRUE);
    if(retVal != GT_OK)
    {
        DBG_INFO(("Failed (gtP2SetAccessRMUPage2 returned GT_FAIL).\n"));
        return retVal;
    }

    retVal = statsOperationPerform(dev,STATS_READ_COUNTER_CLEAR,hwPort,counter,(GT_VOID*)statsData);
    if(retVal != GT_OK)
    {
        DBG_INFO(("Failed (statsOperationPerform returned GT_FAIL).\n"));
        return retVal;
    }

	/* Exit Page2 access */
    retVal = gtP2SetAccessRMUPage2(dev, GT_FALSE);
    if(retVal != GT_OK)
    {
        DBG_INFO(("Failed (gtP2SetAccessRMUPage2 returned GT_FAIL).\n"));
        return retVal;
    }

    DBG_INFO(("OK.\n"));
    return GT_OK;

}


/*******************************************************************************
* gstatsPg2GetPortAllCounters
*
* DESCRIPTION:
*       This routine gets all counters of the given port
*
* INPUTS:
*       port - the logical port number.
*
* OUTPUTS:
*       statsCounterSet - points to GT_STATS_COUNTER_SET_PAGE2 for the MIB counters
*
* RETURNS:
*       GT_OK      - on success
*       GT_FAIL    - on error
*
* COMMENTS:
*       None
*
* GalTis:
*
*******************************************************************************/
GT_STATUS gstatsPg2GetPortAllCounters
(
    IN  GT_QD_DEV               *dev,
    IN  GT_LPORT        port,
    OUT GT_STATS_COUNTER_SET_PAGE2    *statsCounterSet
)
{
    GT_STATUS    retVal;
    GT_U8        hwPort;         /* physical port number         */

    DBG_INFO(("gstatsPg2GetPortAllCounters Called.\n"));

    /* translate logical port to physical port */
    hwPort = GT_LPORT_2_PORT(port);

    /* check if device supports this feature */
    if((retVal = IS_VALID_API_CALL(dev,hwPort, DEV_RMON)) != GT_OK)
    {
        return retVal;
    }

    /* Gigabit Switch does not support this status. */
    if (!IS_IN_DEV_GROUP(dev,DEV_RMON_TYPE_3))
    {
        DBG_INFO(("GT_NOT_SUPPORTED\n"));
        return GT_NOT_SUPPORTED;
    }

	/* Enter Page2 access */
    retVal = gtP2SetAccessRMUPage2(dev, GT_TRUE);
    if(retVal != GT_OK)
    {
        DBG_INFO(("Failed (gtP2SetAccessRMUPage2 returned GT_FAIL).\n"));
        return retVal;
    }

    retVal = statsOperationPerform(dev,STATS_READ_ALL,hwPort,0,(GT_VOID*)statsCounterSet);
    if(retVal != GT_OK)
    {
        DBG_INFO(("Failed (statsOperationPerform returned GT_FAIL).\n"));
        return retVal;
    }

	/* Exit Page2 access */
    retVal = gtP2SetAccessRMUPage2(dev, GT_FALSE);
    if(retVal != GT_OK)
    {
        DBG_INFO(("Failed (gtP2SetAccessRMUPage2 returned GT_FAIL).\n"));
        return retVal;
    }

    DBG_INFO(("OK.\n"));
    return GT_OK;

}

/*******************************************************************************
* gstatsPg2GetPortAllCountersClear
*
* DESCRIPTION:
*       This routine gets all counters of the given port and clear
*
* INPUTS:
*       port - the logical port number.
*
* OUTPUTS:
*       statsCounterSet - points to GT_STATS_COUNTER_SET_PAGE2 for the MIB counters
*
* RETURNS:
*       GT_OK      - on success
*       GT_FAIL    - on error
*
* COMMENTS:
*       None
*
* GalTis:
*
*******************************************************************************/
GT_STATUS gstatsPg2GetPortAllCountersClear
(
    IN  GT_QD_DEV               *dev,
    IN  GT_LPORT        port,
    OUT GT_STATS_COUNTER_SET_PAGE2    *statsCounterSet
)
{
    GT_STATUS    retVal;
    GT_U8        hwPort;         /* physical port number         */

    DBG_INFO(("gstatsPg2GetPortAllCountersClear Called.\n"));

    /* translate logical port to physical port */
    hwPort = GT_LPORT_2_PORT(port);

    /* check if device supports this feature */
    if((retVal = IS_VALID_API_CALL(dev,hwPort, DEV_RMON)) != GT_OK)
    {
        return retVal;
    }

    /* Gigabit Switch does not support this status. */
    if (!IS_IN_DEV_GROUP(dev,DEV_RMON_TYPE_3))
    {
        DBG_INFO(("GT_NOT_SUPPORTED\n"));
        return GT_NOT_SUPPORTED;
    }

	/* Enter Page2 access */
    retVal = gtP2SetAccessRMUPage2(dev, GT_TRUE);
    if(retVal != GT_OK)
    {
        DBG_INFO(("Failed (gtP2SetAccessRMUPage2 returned GT_FAIL).\n"));
        return retVal;
    }

    retVal = statsOperationPerform(dev,STATS_READ_ALL_CLEAR,hwPort,0,(GT_VOID*)statsCounterSet);
    if(retVal != GT_OK)
    {
        DBG_INFO(("Failed (statsOperationPerform returned GT_FAIL).\n"));
        return retVal;
    }

	/* Exit Page2 access */
    retVal = gtP2SetAccessRMUPage2(dev, GT_FALSE);
    if(retVal != GT_OK)
    {
        DBG_INFO(("Failed (gtP2SetAccessRMUPage2 returned GT_FAIL).\n"));
        return retVal;
    }

    DBG_INFO(("OK.\n"));
    return GT_OK;

}


/*******************************************************************************
* gstatsPg2GetRealtimePortCounter
*
* DESCRIPTION:
*        This routine gets a specific realtime counter of the given port
*
* INPUTS:
*        port - the logical port number.
*        counter - the counter which will be read
*
* OUTPUTS:
*        statsData - points to 32bit data storage for the MIB counter
*
* RETURNS:
*        GT_OK      - on success
*        GT_FAIL    - on error
*
* COMMENTS:
*
*******************************************************************************/
GT_STATUS gstatsPg2GetRealtimePortCounter
(
    IN  GT_QD_DEV        *dev,
    IN  GT_LPORT        port,
    IN  GT_STATS_COUNTERS_PAGE2   counter,
    OUT GT_U32            *statsData
)
{
    GT_STATUS    retVal;
    GT_U8        hwPort;         /* physical port number         */

    DBG_INFO(("gstatsPg2GetRealtimePortCounter Called.\n"));

    /* translate logical port to physical port */
    hwPort = GT_LPORT_2_PORT(port);

    /* check if device supports this feature */
    if((retVal = IS_VALID_API_CALL(dev,hwPort, DEV_RMON)) != GT_OK)
    {
        return retVal;
    }

    /* check if device supports this feature */
    if (!IS_IN_DEV_GROUP(dev,DEV_RMON_REALTIME_SUPPORT))
    {
        DBG_INFO(("GT_NOT_SUPPORTED\n"));
        return GT_NOT_SUPPORTED;
    }

	/* Enter Page2 access */
    retVal = gtP2SetAccessRMUPage2(dev, GT_TRUE);
    if(retVal != GT_OK)
    {
        DBG_INFO(("Failed (gtP2SetAccessRMUPage2 returned GT_FAIL).\n"));
        return retVal;
    }

    retVal = statsOperationPerform(dev,STATS_READ_REALTIME_COUNTER,hwPort,counter,(GT_VOID*)statsData);
    if(retVal != GT_OK)
    {
        DBG_INFO(("Failed (statsOperationPerform returned GT_FAIL).\n"));
        return retVal;
    }

	/* Exit Page2 access */
    retVal = gtP2SetAccessRMUPage2(dev, GT_FALSE);
    if(retVal != GT_OK)
    {
        DBG_INFO(("Failed (gtP2SetAccessRMUPage2 returned GT_FAIL).\n"));
        return retVal;
    }

    DBG_INFO(("OK.\n"));
    return GT_OK;

}


/****************************************************************************/
/* Internal use functions.                                                  */
/****************************************************************************/

/*******************************************************************************
* statsOperationPerform
*
* DESCRIPTION:
*       This function is used by all stats control functions, and is responsible
*       to write the required operation into the stats registers.
*
* INPUTS:
*       statsOp       - The stats operation bits to be written into the stats
*                     operation register.
*       port        - port number
*       counter     - counter to be read if it's read operation
*
* OUTPUTS:
*       statsData   - points to the data storage where the MIB counter will be saved.
*
* RETURNS:
*       GT_OK on success,
*       GT_FAIL otherwise.
*
* COMMENTS:
*
*******************************************************************************/

static GT_STATUS statsOperationPerform
(
    IN   GT_QD_DEV            *dev,
    IN   GT_STATS_OPERATION   statsOp,
    IN   GT_U8                port,
    IN   GT_STATS_COUNTERS_PAGE2    counter,
    OUT  GT_VOID              *statsData
)
{
    GT_STATUS       retVal;         /* Functions return value.      */
    GT_U16          data; /* Data to be set into the      */
                                    /* register.                    */
    GT_U32 statsCounter;
    GT_U32 lastCounter;
    GT_U16            portNum;
	GT_U8 bank;

    gtSemTake(dev,dev->statsRegsSem,OS_WAIT_FOREVER);

    if (IS_IN_DEV_GROUP(dev,DEV_RMON_PORT_BITS))
    {
        portNum = (port + 1) << 5;
    }
    else
    {
        portNum = (GT_U16)port;
    }

    /* Wait until the stats in ready. */
#ifdef GT_RMGMT_ACCESS
    {
      HW_DEV_REG_ACCESS regAccess;

      regAccess.entries = 1;

      regAccess.rw_reg_list[0].cmd = HW_REG_WAIT_TILL_0;
      regAccess.rw_reg_list[0].addr = CALC_SMI_DEV_ADDR(dev, 0, GLOBAL_REG_ACCESS);
      regAccess.rw_reg_list[0].reg = QD_REG_STATS_OPERATION;
      regAccess.rw_reg_list[0].data = 15;
      retVal = hwAccessMultiRegs(dev, &regAccess);
      if(retVal != GT_OK)
      {
        gtSemGive(dev,dev->statsRegsSem);
        return retVal;
      }
      histoData = qdLong2Short(regAccess.rw_reg_list[1].data);
    }
#else
    data = 1;
    while(data == 1)
    {
        retVal = hwGetGlobalRegField(dev,QD_REG_STATS_OPERATION,15,1,&data);
        if(retVal != GT_OK)
        {
            gtSemGive(dev,dev->statsRegsSem);
            return retVal;
        }
    }

#endif

    /* Set the STAT Operation register */
    switch (statsOp)
    {
        case STATS_READ_COUNTER:
			bank = (counter&GT_PAGE2_BANK1)?1:0;
            retVal = statsCapture(dev, bank, port);
            if(retVal != GT_OK)
            {
                gtSemGive(dev,dev->statsRegsSem);
                return retVal;
            }

            retVal = statsReadCounter(dev,counter,(GT_U32*)statsData);
            if(retVal != GT_OK)
            {
                gtSemGive(dev,dev->statsRegsSem);
                return retVal;
            }
            break;

        case STATS_READ_COUNTER_CLEAR:
			bank = (counter&GT_PAGE2_BANK1)?1:0;
            retVal = statsCaptureClear(dev, bank, port);
            if(retVal != GT_OK)
            {
                gtSemGive(dev,dev->statsRegsSem);
                return retVal;
            }

            retVal = statsReadCounter(dev,counter,(GT_U32*)statsData);
            if(retVal != GT_OK)
            {
                gtSemGive(dev,dev->statsRegsSem);
                return retVal;
            }
            break;

        case STATS_READ_REALTIME_COUNTER:
            retVal = statsReadRealtimeCounter(dev,port,counter,(GT_U32*)statsData);
            if(retVal != GT_OK)
            {
                gtSemGive(dev,dev->statsRegsSem);
                return retVal;
            }

            break;

        case STATS_READ_ALL:
          for(bank=0; bank<2; bank++)
		  {
            retVal = statsCapture(dev, bank, port);
            if(retVal != GT_OK)
            {
                gtSemGive(dev,dev->statsRegsSem);
                return retVal;
            }

            lastCounter = (bank==0)?(GT_U32)STATS_PG2_Late : (GT_U32)STATS_PG2_OutMGMT;

            for(statsCounter=0; statsCounter<=lastCounter; statsCounter++)
            {
                retVal = statsReadCounter(dev,statsCounter,((GT_U32*)statsData + statsCounter));
                if(retVal != GT_OK)
                {
                    gtSemGive(dev,dev->statsRegsSem);
                    return retVal;
                }
            }
		  }
            break;

        case STATS_READ_ALL_CLEAR:
          for(bank=0; bank<2; bank++)
		  {
            retVal = statsCaptureClear(dev,bank, port);
            if(retVal != GT_OK)
            {
                gtSemGive(dev,dev->statsRegsSem);
                return retVal;
            }

            lastCounter = (bank==0)?(GT_U32)STATS_PG2_Late : (GT_U32)STATS_PG2_OutMGMT;

            for(statsCounter=0; statsCounter<=lastCounter; statsCounter++)
            {
                retVal = statsReadCounter(dev,statsCounter,((GT_U32*)statsData + statsCounter));
                if(retVal != GT_OK)
                {
                    gtSemGive(dev,dev->statsRegsSem);
                    return retVal;
                }
            }
		  }
            break;

        default:

            gtSemGive(dev,dev->statsRegsSem);
            return GT_FAIL;
    }

    gtSemGive(dev,dev->statsRegsSem);
    return GT_OK;
}


/*******************************************************************************
* statsCapture
*
* DESCRIPTION:
*       This function is used to capture all counters of a port.
*
* INPUTS:
*       port        - port number
*
* OUTPUTS:
*        None.
*
* RETURNS:
*       GT_OK on success,
*       GT_FAIL otherwise.
*
* COMMENTS:
*        If Semaphore is used, Semaphore should be acquired before this function call.
*******************************************************************************/
static GT_STATUS statsCapture
(
    IN GT_QD_DEV            *dev,
    IN GT_U8             bank,
    IN GT_U8             port
)
{
    GT_STATUS       retVal;         /* Functions return value.      */
    GT_U16          data;           /* Data to be set into the      */
                                    /* register.                    */
    GT_U16            portNum;

    if (IS_IN_DEV_GROUP(dev,DEV_RMON_PORT_BITS))
    {
        portNum = (port + 1) << 5;
    }
    else
    {
        portNum = (GT_U16)port;
    }

#ifdef GT_RMGMT_ACCESS
    {
      HW_DEV_REG_ACCESS regAccess;

      regAccess.entries = 1;

      regAccess.rw_reg_list[0].cmd = HW_REG_WAIT_TILL_0;
      regAccess.rw_reg_list[0].addr = CALC_SMI_DEV_ADDR(dev, 0, GLOBAL_REG_ACCESS);
      regAccess.rw_reg_list[0].reg = QD_REG_STATS_OPERATION;
      regAccess.rw_reg_list[0].data = 15;
      retVal = hwAccessMultiRegs(dev, &regAccess);
      if(retVal != GT_OK)
      {
        return retVal;
      }
    }
#else
    data = 1;
       while(data == 1)
    {
        retVal = hwGetGlobalRegField(dev,QD_REG_STATS_OPERATION,15,1,&data);
        if(retVal != GT_OK)
           {
               return retVal;
        }
       }
#endif

    data = (1 << 15) | (GT_STATS_CAPTURE_PORT << 12) | portNum | (bank<<9);
    retVal = hwWriteGlobalReg(dev,QD_REG_STATS_OPERATION,data);
    if(retVal != GT_OK)
    {
        return retVal;
    }

    return GT_OK;

}

/*******************************************************************************
* statsCaptureClear
*
* DESCRIPTION:
*       This function is used to capture all counters of a port and clear.
*
* INPUTS:
*       port        - port number
*
* OUTPUTS:
*        None.
*
* RETURNS:
*       GT_OK on success,
*       GT_FAIL otherwise.
*
* COMMENTS:
*        If Semaphore is used, Semaphore should be acquired before this function call.
*******************************************************************************/
static GT_STATUS statsCaptureClear
(
    IN GT_QD_DEV            *dev,
    IN GT_U8             bank,
    IN GT_U8             port
)
{
    GT_STATUS       retVal;         /* Functions return value.      */
    GT_U16          data;/* Data to be set into the      */
                                    /* register.                    */
    GT_U16            portNum;

    if (IS_IN_DEV_GROUP(dev,DEV_RMON_PORT_BITS))
    {
        portNum = (port + 1) << 5;
    }
    else
    {
        portNum = (GT_U16)port;
    }

#ifdef GT_RMGMT_ACCESS
    {
      HW_DEV_REG_ACCESS regAccess;

      regAccess.entries = 1;

      regAccess.rw_reg_list[0].cmd = HW_REG_WAIT_TILL_0;
      regAccess.rw_reg_list[0].addr = CALC_SMI_DEV_ADDR(dev, 0, GLOBAL_REG_ACCESS);
      regAccess.rw_reg_list[0].reg = QD_REG_STATS_OPERATION;
      regAccess.rw_reg_list[0].data = 15;
      retVal = hwAccessMultiRegs(dev, &regAccess);
      if(retVal != GT_OK)
      {
        return retVal;
      }
    }
#else
    data = 1;
       while(data == 1)
    {
        retVal = hwGetGlobalRegField(dev,QD_REG_STATS_OPERATION,15,1,&data);
        if(retVal != GT_OK)
           {
               return retVal;
        }
       }
#endif

    data = (1 << 15) | (GT_STATS_CAPTURE_PORT_CLEAR << 12) | portNum | (bank<<9);
    retVal = hwWriteGlobalReg(dev,QD_REG_STATS_OPERATION,data);
    if(retVal != GT_OK)
    {
        return retVal;
    }

    return GT_OK;

}


/*******************************************************************************
* statsReadCounter
*
* DESCRIPTION:
*       This function is used to read a captured counter.
*
* INPUTS:
*       counter     - counter to be read if it's read operation
*
* OUTPUTS:
*       statsData   - points to the data storage where the MIB counter will be saved.
*
* RETURNS:
*       GT_OK on success,
*       GT_FAIL otherwise.
*
* COMMENTS:
*        If Semaphore is used, Semaphore should be acquired before this function call.
*******************************************************************************/
static GT_STATUS statsReadCounter
(
    IN   GT_QD_DEV      *dev,
    IN   GT_U32            counter,
    OUT  GT_U32            *statsData
)
{
    GT_STATUS   retVal;         /* Functions return value.            */
    GT_U16      data;/* Data to be set into the  register. */
	GT_U8       bank;
#ifndef GT_RMGMT_ACCESS
    GT_U16    counter3_2;     /* Counter Register Bytes 3 & 2       */
    GT_U16    counter1_0;     /* Counter Register Bytes 1 & 0       */
#endif

#ifdef GT_RMGMT_ACCESS
    {
      HW_DEV_REG_ACCESS regAccess;

      regAccess.entries = 1;

      regAccess.rw_reg_list[0].cmd = HW_REG_WAIT_TILL_0;
      regAccess.rw_reg_list[0].addr = CALC_SMI_DEV_ADDR(dev, 0, GLOBAL_REG_ACCESS);
      regAccess.rw_reg_list[0].reg = QD_REG_STATS_OPERATION;
      regAccess.rw_reg_list[0].data = 15;
      retVal = hwAccessMultiRegs(dev, &regAccess);
      if(retVal != GT_OK)
      {
        return retVal;
      }
    }
#else
    data = 1;
       while(data == 1)
    {
        retVal = hwGetGlobalRegField(dev,QD_REG_STATS_OPERATION,15,1,&data);
        if(retVal != GT_OK)
           {
               return retVal;
        }
       }
#endif

	bank = (counter&GT_PAGE2_BANK1)?1:0;
    data = (GT_U16)((1 << 15) | (GT_STATS_READ_COUNTER << 12) | counter | (bank<<9));
    retVal = hwWriteGlobalReg(dev,QD_REG_STATS_OPERATION,data);
    if(retVal != GT_OK)
    {
        return retVal;
    }

#ifdef GT_RMGMT_ACCESS
    {
      HW_DEV_REG_ACCESS regAccess;

      regAccess.entries = 3;

      regAccess.rw_reg_list[0].cmd = HW_REG_WAIT_TILL_0;
      regAccess.rw_reg_list[0].addr = CALC_SMI_DEV_ADDR(dev, 0, GLOBAL_REG_ACCESS);
      regAccess.rw_reg_list[0].reg = QD_REG_STATS_OPERATION;
      regAccess.rw_reg_list[0].data = 15;
      regAccess.rw_reg_list[1].cmd = HW_REG_READ;
      regAccess.rw_reg_list[1].addr = CALC_SMI_DEV_ADDR(dev, 0, GLOBAL_REG_ACCESS);
      regAccess.rw_reg_list[1].reg = QD_REG_STATS_COUNTER3_2;
      regAccess.rw_reg_list[1].data = 0;
      regAccess.rw_reg_list[2].cmd = HW_REG_READ;
      regAccess.rw_reg_list[2].addr = CALC_SMI_DEV_ADDR(dev, 0, GLOBAL_REG_ACCESS);
      regAccess.rw_reg_list[2].reg = QD_REG_STATS_COUNTER1_0;
      regAccess.rw_reg_list[2].data = 0;
      retVal = hwAccessMultiRegs(dev, &regAccess);
      if(retVal != GT_OK)
      {
        return retVal;
      }
      *statsData = (regAccess.rw_reg_list[1].data << 16) | regAccess.rw_reg_list[2].data;
    }
#else
    data = 1;
       while(data == 1)
    {
        retVal = hwGetGlobalRegField(dev,QD_REG_STATS_OPERATION,15,1,&data);
        if(retVal != GT_OK)
           {
               return retVal;
        }
       }

    retVal = hwReadGlobalReg(dev,QD_REG_STATS_COUNTER3_2,&counter3_2);
    if(retVal != GT_OK)
    {
        return retVal;
    }

    retVal = hwReadGlobalReg(dev,QD_REG_STATS_COUNTER1_0,&counter1_0);
    if(retVal != GT_OK)
    {
        return retVal;
    }

    *statsData = (counter3_2 << 16) | counter1_0;
#endif

    return GT_OK;

}


/*******************************************************************************
* statsReadRealtimeCounter
*
* DESCRIPTION:
*       This function is used to read a realtime counter.
*
* INPUTS:
*       port     - port to be accessed
*       counter  - counter to be read if it's read operation
*
* OUTPUTS:
*       statsData   - points to the data storage where the MIB counter will be saved.
*
* RETURNS:
*       GT_OK on success,
*       GT_FAIL otherwise.
*
* COMMENTS:
*        If Semaphore is used, Semaphore should be acquired before this function call.
*******************************************************************************/
static GT_STATUS statsReadRealtimeCounter
(
    IN   GT_QD_DEV      *dev,
    IN   GT_U8             port,
    IN   GT_U32            counter,
    OUT  GT_U32            *statsData
)
{
    GT_STATUS   retVal;         /* Functions return value.            */
    GT_U16      data, histoData;/* Data to be set into the  register. */
    GT_U16    counter3_2;     /* Counter Register Bytes 3 & 2       */
    GT_U16    counter1_0;     /* Counter Register Bytes 1 & 0       */
    GT_U8  bank;

    /* Get the Histogram mode bit.                */
    retVal = hwReadGlobalReg(dev,QD_REG_STATS_OPERATION,&histoData);
    if(retVal != GT_OK)
    {
        return retVal;
    }

    histoData &= 0xC00;

#ifdef GT_RMGMT_ACCESS
    {
      HW_DEV_REG_ACCESS regAccess;

      regAccess.entries = 1;

      regAccess.rw_reg_list[0].cmd = HW_REG_WAIT_TILL_0;
      regAccess.rw_reg_list[0].addr = CALC_SMI_DEV_ADDR(dev, 0, GLOBAL_REG_ACCESS);
      regAccess.rw_reg_list[0].reg = QD_REG_STATS_OPERATION;
      regAccess.rw_reg_list[0].data = 15;
      retVal = hwAccessMultiRegs(dev, &regAccess);
      if(retVal != GT_OK)
      {
        return retVal;
      }
    }
#else
    data = 1;
       while(data == 1)
    {
        retVal = hwGetGlobalRegField(dev,QD_REG_STATS_OPERATION,15,1,&data);
        if(retVal != GT_OK)
           {
               return retVal;
        }
       }
#endif

	bank = (counter&GT_PAGE2_BANK1)?1:0;
    data = (GT_U16)((1 << 15) | (GT_STATS_READ_COUNTER << 12) | ((port+1) << 5) | counter | (bank<<9));
    retVal = hwWriteGlobalReg(dev,QD_REG_STATS_OPERATION,data);
    if(retVal != GT_OK)
    {
        return retVal;
    }

#ifdef GT_RMGMT_ACCESS
    {
      HW_DEV_REG_ACCESS regAccess;

      regAccess.entries = 1;

      regAccess.rw_reg_list[0].cmd = HW_REG_WAIT_TILL_0;
      regAccess.rw_reg_list[0].addr = CALC_SMI_DEV_ADDR(dev, 0, GLOBAL_REG_ACCESS);
      regAccess.rw_reg_list[0].reg = QD_REG_STATS_OPERATION;
      regAccess.rw_reg_list[0].data = 15;
      retVal = hwAccessMultiRegs(dev, &regAccess);
      if(retVal != GT_OK)
      {
        return retVal;
      }
    }
#else
    data = 1;
       while(data == 1)
    {
        retVal = hwGetGlobalRegField(dev,QD_REG_STATS_OPERATION,15,1,&data);
        if(retVal != GT_OK)
           {
               return retVal;
        }
       }
#endif

    retVal = hwReadGlobalReg(dev,QD_REG_STATS_COUNTER3_2,&counter3_2);
    if(retVal != GT_OK)
    {
        return retVal;
    }

    retVal = hwReadGlobalReg(dev,QD_REG_STATS_COUNTER1_0,&counter1_0);
    if(retVal != GT_OK)
    {
        return retVal;
    }

    *statsData = (counter3_2 << 16) | counter1_0;

    return GT_OK;

}

/* Page 2 ATU APIs */


/* Page 2 SMI APIs */
