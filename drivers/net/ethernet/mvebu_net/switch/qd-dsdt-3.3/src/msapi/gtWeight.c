#include <Copyright.h>

/********************************************************************************
* gtWeight.c
*
* DESCRIPTION:
*       API definitions for Round Robin Weight table access
*
* DEPENDENCIES:
*
* FILE REVISION NUMBER:
*       $Revision: $
*******************************************************************************/

#include <msApi.h>
#include <gtSem.h>
#include <gtHwCntl.h>
#include <gtDrvSwRegs.h>


/*******************************************************************************
* gsysSetQoSWeight
*
* DESCRIPTION:
*       Programmable Round Robin Weights.
*        Each port has 4 output Queues. Queue 3 has the highest priority and
*        Queue 0 has the lowest priority. When a scheduling mode of port is
*        configured as Weighted Round Robin queuing mode, the access sequece of the
*        Queue is 3,2,3,1,3,2,3,0,3,2,3,1,3,2,3 by default.
*        This sequence can be configured with this API.
*
* INPUTS:
*       weight - access sequence of the queue
*
* OUTPUTS:
*       None.
*
* RETURNS:
*       GT_OK      - on success
*       GT_FAIL    - on error
*        GT_NOT_SUPPORTED - if current device does not support this feature.
*
* COMMENTS:
*       None
*
*******************************************************************************/
GT_STATUS gsysSetQoSWeight
(
    IN  GT_QD_DEV         *dev,
    IN  GT_QoS_WEIGHT    *weight
)
{
    GT_STATUS    retVal;         /* Functions return value.      */
    GT_U16        data;
    GT_U32        len, i;

    DBG_INFO(("gsysSetQoSWeight Called.\n"));

    /* Check if Switch supports this feature. */
    if (!IS_IN_DEV_GROUP(dev,DEV_QoS_WEIGHT))
    {
        DBG_INFO(("GT_NOT_SUPPORTED\n"));
        return GT_NOT_SUPPORTED;
    }

    if (weight->len > 128)
    {
        DBG_INFO(("GT_BAD_PARAM\n"));
        return GT_BAD_PARAM;
    }

    gtSemTake(dev,dev->tblRegsSem,OS_WAIT_FOREVER);

    len = weight->len/4;

    /* program QoS Weight Table, 4 sequences at a time */

    for(i=0; i<len; i++)
    {
        /* Wait until the QoS Weight Table is ready. */
#ifdef GT_RMGMT_ACCESS
    {
      HW_DEV_REG_ACCESS regAccess;

      regAccess.entries = 2;

      regAccess.rw_reg_list[0].cmd = HW_REG_WAIT_TILL_0;
      regAccess.rw_reg_list[0].addr = CALC_SMI_DEV_ADDR(dev, 0, GLOBAL2_REG_ACCESS);
      regAccess.rw_reg_list[0].reg = QD_REG_QOS_WEIGHT;
      regAccess.rw_reg_list[0].data = 15;
      regAccess.rw_reg_list[1].cmd = HW_REG_WRITE;
      regAccess.rw_reg_list[1].addr = CALC_SMI_DEV_ADDR(dev, 0, GLOBAL2_REG_ACCESS);
      regAccess.rw_reg_list[1].reg = QD_REG_QOS_WEIGHT;
      data =  (GT_U16)((1 << 15) | (i << 8) |
            (weight->queue[i*4] & 0x3) |
            ((weight->queue[i*4+1] & 0x3) << 2) |
            ((weight->queue[i*4+2] & 0x3) << 4) |
            ((weight->queue[i*4+3] & 0x3) << 6));
      regAccess.rw_reg_list[1].data = data;
      retVal = hwAccessMultiRegs(dev, &regAccess);
      if(retVal != GT_OK)
      {
            gtSemGive(dev,dev->tblRegsSem);
        return retVal;
      }
    }
#else
        data = 1;
        while(data == 1)
        {
            retVal = hwGetGlobal2RegField(dev,QD_REG_QOS_WEIGHT,15,1,&data);
            if(retVal != GT_OK)
            {
                gtSemGive(dev,dev->tblRegsSem);
                return retVal;
            }
        }

        data =  (GT_U16)((1 << 15) | (i << 8) |
                (weight->queue[i*4] & 0x3) |
                ((weight->queue[i*4+1] & 0x3) << 2) |
                ((weight->queue[i*4+2] & 0x3) << 4) |
                ((weight->queue[i*4+3] & 0x3) << 6));

        retVal = hwWriteGlobal2Reg(dev, QD_REG_QOS_WEIGHT, data);
        if(retVal != GT_OK)
        {
               DBG_INFO(("Failed.\n"));
            gtSemGive(dev,dev->tblRegsSem);
            return retVal;
        }
#endif
    }

    /* program remaining sequences if any */
    i = weight->len % 4;
    if (i)
    {
        /* Wait until the QoS Weight Table is ready. */
#ifdef GT_RMGMT_ACCESS
        {
          HW_DEV_REG_ACCESS regAccess;

          regAccess.entries = 2;

          regAccess.rw_reg_list[0].cmd = HW_REG_WAIT_TILL_0;
          regAccess.rw_reg_list[0].addr = CALC_SMI_DEV_ADDR(dev, 0, GLOBAL2_REG_ACCESS);
          regAccess.rw_reg_list[0].reg = QD_REG_QOS_WEIGHT;
          regAccess.rw_reg_list[0].data = 15;
          regAccess.rw_reg_list[1].cmd = HW_REG_WRITE;
          regAccess.rw_reg_list[1].addr = CALC_SMI_DEV_ADDR(dev, 0, GLOBAL2_REG_ACCESS);
          regAccess.rw_reg_list[1].reg = QD_REG_QOS_WEIGHT;
          data =  (GT_U16)((1 << 15) | (len << 8));
          switch (i)
          {
            case 3:
                data |= ((weight->queue[len*4+2] & 0x3) << 4);
            case 2:
                data |= ((weight->queue[len*4+1] & 0x3) << 2);
            case 1:
                data |= ((weight->queue[len*4+0] & 0x3) << 0);
            break;
            default:
                DBG_INFO(("Should not come to this point.\n"));
                gtSemGive(dev,dev->tblRegsSem);
            return GT_FALSE;
          }
          regAccess.rw_reg_list[1].data = data;
          retVal = hwAccessMultiRegs(dev, &regAccess);
          if(retVal != GT_OK)
          {
            gtSemGive(dev,dev->tblRegsSem);
            return retVal;
          }
        }
#else
        data = 1;
        while(data == 1)
        {
            retVal = hwGetGlobal2RegField(dev,QD_REG_QOS_WEIGHT,15,1,&data);
            if(retVal != GT_OK)
            {
                gtSemGive(dev,dev->tblRegsSem);
                return retVal;
            }
        }

        data =  (GT_U16)((1 << 15) | (len << 8));

        switch (i)
        {
            case 3:
                data |= ((weight->queue[len*4+2] & 0x3) << 4);
            case 2:
                data |= ((weight->queue[len*4+1] & 0x3) << 2);
            case 1:
                data |= ((weight->queue[len*4+0] & 0x3) << 0);
                break;
            default:
                   DBG_INFO(("Should not come to this point.\n"));
                gtSemGive(dev,dev->tblRegsSem);
                return GT_FALSE;
        }

        retVal = hwWriteGlobal2Reg(dev, QD_REG_QOS_WEIGHT, data);
        if(retVal != GT_OK)
        {
               DBG_INFO(("Failed.\n"));
            gtSemGive(dev,dev->tblRegsSem);
            return retVal;
        }
#endif
    }

    /* Write the lengh of the sequence */

    /* Wait until the QoS Weight Table is ready. */
#ifdef GT_RMGMT_ACCESS
    {
      HW_DEV_REG_ACCESS regAccess;

      regAccess.entries = 2;

      regAccess.rw_reg_list[0].cmd = HW_REG_WAIT_TILL_0;
      regAccess.rw_reg_list[0].addr = CALC_SMI_DEV_ADDR(dev, 0, GLOBAL2_REG_ACCESS);
      regAccess.rw_reg_list[0].reg = QD_REG_QOS_WEIGHT;
      regAccess.rw_reg_list[0].data = 15;
      regAccess.rw_reg_list[1].cmd = HW_REG_WRITE;
      regAccess.rw_reg_list[1].addr = CALC_SMI_DEV_ADDR(dev, 0, GLOBAL2_REG_ACCESS);
      regAccess.rw_reg_list[1].reg = QD_REG_QOS_WEIGHT;
      data =  (GT_U16)((1 << 15) | (0x20 << 8) | weight->len);
      regAccess.rw_reg_list[1].data = data;
      retVal = hwAccessMultiRegs(dev, &regAccess);
    }
#else
       data = 1;
    while(data == 1)
       {
        retVal = hwGetGlobal2RegField(dev,QD_REG_QOS_WEIGHT,15,1,&data);
           if(retVal != GT_OK)
           {
            gtSemGive(dev,dev->tblRegsSem);
               return retVal;
           }
    }

    data =  (GT_U16)((1 << 15) | (0x20 << 8) | weight->len);

    retVal = hwWriteGlobal2Reg(dev, QD_REG_QOS_WEIGHT, data);
#endif

    gtSemGive(dev,dev->tblRegsSem);

    if(retVal != GT_OK)
       {
           DBG_INFO(("Failed.\n"));
           return retVal;
    }

    return GT_OK;
}


/*******************************************************************************
* gsysGetQoSWeight
*
* DESCRIPTION:
*       Programmable Round Robin Weights.
*        Each port has 4 output Queues. Queue 3 has the highest priority and
*        Queue 0 has the lowest priority. When a scheduling mode of port is
*        configured as Weighted Round Robin queuing mode, the access sequece of the
*        Queue is 3,2,3,1,3,2,3,0,3,2,3,1,3,2,3 by default.
*        This routine retrieves the access sequence of the Queue.
*
* INPUTS:
*       None.
*
* OUTPUTS:
*       weight - access sequence of the queue
*
* RETURNS:
*       GT_OK      - on success
*       GT_FAIL    - on error
*        GT_NOT_SUPPORTED - if current device does not support this feature.
*
* COMMENTS:
*       None
*
*******************************************************************************/
GT_STATUS gsysGetQoSWeight
(
    IN  GT_QD_DEV         *dev,
    OUT GT_QoS_WEIGHT    *weight
)
{
    GT_STATUS    retVal;         /* Functions return value.      */
    GT_U16        data, i;
    GT_U32        len;

    DBG_INFO(("gsysGetQoSWeight Called.\n"));

    /* Check if Switch supports this feature. */
    if (!IS_IN_DEV_GROUP(dev,DEV_QoS_WEIGHT))
    {
        DBG_INFO(("GT_NOT_SUPPORTED\n"));
        return GT_NOT_SUPPORTED;
    }

    gtSemTake(dev,dev->tblRegsSem,OS_WAIT_FOREVER);

    /* Read the lengh of the sequence */

    /* Wait until the QoS Weight Table is ready. */
#ifdef GT_RMGMT_ACCESS
    {
      HW_DEV_REG_ACCESS regAccess;

      regAccess.entries = 3;

      regAccess.rw_reg_list[0].cmd = HW_REG_WAIT_TILL_0;
      regAccess.rw_reg_list[0].addr = CALC_SMI_DEV_ADDR(dev, 0, GLOBAL2_REG_ACCESS);
      regAccess.rw_reg_list[0].reg = QD_REG_QOS_WEIGHT;
      regAccess.rw_reg_list[0].data = 15;
      regAccess.rw_reg_list[1].cmd = HW_REG_WRITE;
      regAccess.rw_reg_list[1].addr = CALC_SMI_DEV_ADDR(dev, 0, GLOBAL2_REG_ACCESS);
      regAccess.rw_reg_list[1].reg = QD_REG_QOS_WEIGHT;
      data = (0x20 << 8);
      regAccess.rw_reg_list[1].data = data;
      regAccess.rw_reg_list[2].cmd = HW_REG_READ;
      regAccess.rw_reg_list[2].addr = CALC_SMI_DEV_ADDR(dev, 0, GLOBAL2_REG_ACCESS);
      regAccess.rw_reg_list[2].reg = QD_REG_QOS_WEIGHT;
      regAccess.rw_reg_list[2].data = 0;
      retVal = hwAccessMultiRegs(dev, &regAccess);
      data = qdLong2Short(regAccess.rw_reg_list[2].data);
    }
#else
       data = 1;
    while(data == 1)
       {
        retVal = hwGetGlobal2RegField(dev,QD_REG_QOS_WEIGHT,15,1,&data);
           if(retVal != GT_OK)
           {
            gtSemGive(dev,dev->tblRegsSem);
               return retVal;
           }
    }

    data = (0x20 << 8);

    retVal = hwWriteGlobal2Reg(dev, QD_REG_QOS_WEIGHT, data);
    if(retVal != GT_OK)
       {
           DBG_INFO(("Failed.\n"));
        gtSemGive(dev,dev->tblRegsSem);
           return retVal;
    }

    retVal = hwReadGlobal2Reg(dev, QD_REG_QOS_WEIGHT, &data);
#endif
    if(retVal != GT_OK)
       {
           DBG_INFO(("Failed.\n"));
        gtSemGive(dev,dev->tblRegsSem);
           return retVal;
    }

    weight->len = data & 0xFF;

    len = weight->len/4;

    /* read QoS Weight Table, 4 sequences at a time */

    for(i=0; i<len; i++)
    {
        data = i << 8;

#ifdef GT_RMGMT_ACCESS
    {
      HW_DEV_REG_ACCESS regAccess;

      regAccess.entries = 2;

      regAccess.rw_reg_list[0].cmd = HW_REG_WRITE;
      regAccess.rw_reg_list[0].addr = CALC_SMI_DEV_ADDR(dev, 0, GLOBAL2_REG_ACCESS);
      regAccess.rw_reg_list[0].reg = QD_REG_QOS_WEIGHT;
      regAccess.rw_reg_list[0].data = data;
      regAccess.rw_reg_list[1].cmd = HW_REG_READ;
      regAccess.rw_reg_list[1].addr = CALC_SMI_DEV_ADDR(dev, 0, GLOBAL2_REG_ACCESS);
      regAccess.rw_reg_list[1].reg = QD_REG_QOS_WEIGHT;
      regAccess.rw_reg_list[1].data = 0;
      retVal = hwAccessMultiRegs(dev, &regAccess);
      data = qdLong2Short(regAccess.rw_reg_list[1].data);
    }
#else
        retVal = hwWriteGlobal2Reg(dev, QD_REG_QOS_WEIGHT, data);
        if(retVal != GT_OK)
        {
               DBG_INFO(("Failed.\n"));
            gtSemGive(dev,dev->tblRegsSem);
            return retVal;
        }

        retVal = hwReadGlobal2Reg(dev, QD_REG_QOS_WEIGHT, &data);
#endif
        if(retVal != GT_OK)
           {
               DBG_INFO(("Failed.\n"));
            gtSemGive(dev,dev->tblRegsSem);
               return retVal;
        }

        weight->queue[i*4] = data & 0x3;
        weight->queue[i*4+1] = (data >> 2) & 0x3;
        weight->queue[i*4+2] = (data >> 4) & 0x3;
        weight->queue[i*4+3] = (data >> 6) & 0x3;

    }

    /* read remaining sequences if any */
    i = (GT_U16) (weight->len % 4);
    if (i)
    {
        data = (GT_U16)(len << 8);

#ifdef GT_RMGMT_ACCESS
        {
          HW_DEV_REG_ACCESS regAccess;

          regAccess.entries = 2;

          regAccess.rw_reg_list[0].cmd = HW_REG_WRITE;
          regAccess.rw_reg_list[0].addr = CALC_SMI_DEV_ADDR(dev, 0, GLOBAL2_REG_ACCESS);
          regAccess.rw_reg_list[0].reg = QD_REG_QOS_WEIGHT;
          regAccess.rw_reg_list[0].data = data;
          regAccess.rw_reg_list[1].cmd = HW_REG_READ;
          regAccess.rw_reg_list[1].addr = CALC_SMI_DEV_ADDR(dev, 0, GLOBAL2_REG_ACCESS);
          regAccess.rw_reg_list[1].reg = QD_REG_QOS_WEIGHT;
          regAccess.rw_reg_list[1].data = 0;
          retVal = hwAccessMultiRegs(dev, &regAccess);
          data = qdLong2Short(regAccess.rw_reg_list[1].data);
        }
#else
        retVal = hwWriteGlobal2Reg(dev, QD_REG_QOS_WEIGHT, data);
        if(retVal != GT_OK)
        {
               DBG_INFO(("Failed.\n"));
            gtSemGive(dev,dev->tblRegsSem);
            return retVal;
        }

        retVal = hwReadGlobal2Reg(dev, QD_REG_QOS_WEIGHT, &data);
#endif
        if(retVal != GT_OK)
           {
               DBG_INFO(("Failed.\n"));
            gtSemGive(dev,dev->tblRegsSem);
               return retVal;
        }

        switch (i)
        {
            case 3:
                weight->queue[len*4+2] = (data >> 4) & 0x3;
            case 2:
                weight->queue[len*4+1] = (data >> 2) & 0x3;
            case 1:
                weight->queue[len*4] = data & 0x3;
                break;
            default:
                   DBG_INFO(("Should not come to this point.\n"));
                gtSemGive(dev,dev->tblRegsSem);
                return GT_FALSE;
        }
    }

    gtSemGive(dev,dev->tblRegsSem);

    return GT_OK;
}
