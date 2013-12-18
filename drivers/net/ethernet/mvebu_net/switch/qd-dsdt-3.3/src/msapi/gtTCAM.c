#include <Copyright.h>

/*******************************************************************************
* gtTCAM.c
*
* DESCRIPTION:
*       API definitions for control of Ternary Content Addressable Memory
*
* DEPENDENCIES:
*
* FILE REVISION NUMBER:
*******************************************************************************/

#include <msApi.h>
#include <gtSem.h>
#include <gtHwCntl.h>
#include <gtDrvSwRegs.h>


GT_STATUS tcamReadGlobal3Reg
(
  IN GT_QD_DEV *dev,
  IN  GT_U8    regAddr,
  OUT GT_U16   *data
)
{
  GT_STATUS           retVal;
  retVal = hwReadGlobal3Reg(dev, regAddr, data);
  if(retVal != GT_OK)
  {
    return retVal;
  }

  return GT_OK;
}

GT_STATUS tcamWriteGlobal3Reg
(
  IN  GT_QD_DEV *dev,
  IN  GT_U8    regAddr,
  IN  GT_U16   data
)
{
  GT_STATUS           retVal;

  retVal = hwWriteGlobal3Reg(dev, regAddr, data);
  if(retVal != GT_OK)
  {
    return retVal;
  }
  return GT_OK;
}

/****************************************************************************/
/* TCAM operation function declaration.                                    */
/****************************************************************************/
static GT_STATUS tcamOperationPerform
(
    IN   GT_QD_DEV             *dev,
    IN   GT_TCAM_OPERATION    tcamOp,
    INOUT GT_TCAM_OP_DATA    *opData
);

/*******************************************************************************
* gtcamFlushAll
*
* DESCRIPTION:
*       This routine is to flush all entries. A Flush All command will initialize
*       TCAM Pages 0 and 1, offsets 0x02 to 0x1B to 0x0000, and TCAM Page 2 offset
*       0x02 to 0x05 to 0x0000 for all TCAM entries with the exception that TCAM
*       Page 0 offset 0x02 will be initialized to 0x00FF.
*
*
* INPUTS:
*        None.
*
* OUTPUTS:
*        None.
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
GT_STATUS gtcamFlushAll
(
    IN  GT_QD_DEV     *dev
)
{
    GT_STATUS           retVal;
    GT_TCAM_OPERATION    op;
    GT_TCAM_OP_DATA     tcamOpData;

    DBG_INFO(("gtcamFlushAll Called.\n"));

    /* check if device supports this feature */
    if (!IS_IN_DEV_GROUP(dev,DEV_TCAM))
    {
        DBG_INFO(("GT_NOT_SUPPORTED\n"));
        return GT_NOT_SUPPORTED;
    }

    /* Program Tuning register */
    op = TCAM_FLUSH_ALL;
    tcamOpData.tcamEntry = 0xFF;
    retVal = tcamOperationPerform(dev,op, &tcamOpData);
    if(retVal != GT_OK)
    {
        DBG_INFO(("Failed (tcamOperationPerform returned GT_FAIL).\n"));
        return retVal;
    }

    DBG_INFO(("OK.\n"));
    return GT_OK;

}

/*******************************************************************************
* gtcamFlushEntry
*
* DESCRIPTION:
*       This routine is to flush a single entry. A Flush a single TCAM entry command
*       will write the same values to a TCAM entry as a Flush All command, but it is
*       done to the selected single TCAM entry only.
*
*
* INPUTS:
*        tcamPointer - pointer to the desired entry of TCAM (0 ~ 254)
*
* OUTPUTS:
*        None.
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
GT_STATUS gtcamFlushEntry
(
    IN  GT_QD_DEV     *dev,
    IN  GT_U32        tcamPointer
)
{
    GT_STATUS           retVal;
    GT_TCAM_OPERATION    op;
    GT_TCAM_OP_DATA     tcamOpData;

    DBG_INFO(("gtcamFlushEntry Called.\n"));

    /* check if device supports this feature */
    if (!IS_IN_DEV_GROUP(dev,DEV_TCAM))
    {
        DBG_INFO(("GT_NOT_SUPPORTED\n"));
        return GT_NOT_SUPPORTED;
    }

    /* check if the given pointer is valid */
    if (tcamPointer > 0xFE)
    {
        DBG_INFO(("GT_BAD_PARAM\n"));
        return GT_BAD_PARAM;
    }

    /* Program Tuning register */
    op = TCAM_FLUSH_ALL;
    tcamOpData.tcamEntry = tcamPointer;
    retVal = tcamOperationPerform(dev,op, &tcamOpData);
    if(retVal != GT_OK)
    {
        DBG_INFO(("Failed (tcamOperationPerform returned GT_FAIL).\n"));
        return retVal;
    }

    DBG_INFO(("OK.\n"));
    return GT_OK;

}

/*******************************************************************************
* gtcamLoadEntry
*
* DESCRIPTION:
*       This routine loads a TCAM entry.
*       The load sequence of TCAM entry is critical. Each TCAM entry is made up of
*       3 pages of data. All 3 pages need to loaded in a particular order for the TCAM
*       to operate correctly while frames are flowing through the switch.
*       If the entry is currently valid, it must first be flushed. Then page 2 needs
*       to be loaded first, followed by page 1 and then finally page 0.
*       Each page load requires its own write TCAMOp with these TCAM page bits set
*       accordingly.
*
* INPUTS:
*        tcamPointer - pointer to the desired entry of TCAM (0 ~ 254)
*        tcamData    - Tcam entry Data
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
GT_STATUS gtcamLoadEntry
(
    IN  GT_QD_DEV     *dev,
    IN  GT_U32        tcamPointer,
    IN  GT_TCAM_DATA        *tcamData
)
{
    GT_STATUS           retVal;
    GT_TCAM_OPERATION    op;
    GT_TCAM_OP_DATA     tcamOpData;

    DBG_INFO(("gtcamLoadEntry Called.\n"));

    /* check if device supports this feature */
    if (!IS_IN_DEV_GROUP(dev,DEV_TCAM))
    {
        DBG_INFO(("GT_NOT_SUPPORTED\n"));
        return GT_NOT_SUPPORTED;
    }

    /* check if the given pointer is valid */
    if ((tcamPointer > 0xFE)||(tcamData==NULL))
    {
        DBG_INFO(("GT_BAD_PARAM\n"));
        return GT_BAD_PARAM;
    }

    /* Program Tuning register */
    op = TCAM_LOAD_ENTRY;
    tcamOpData.tcamPage = 0; /* useless */
    tcamOpData.tcamEntry = tcamPointer;
    tcamOpData.tcamDataP = tcamData;
    retVal = tcamOperationPerform(dev,op, &tcamOpData);
    if(retVal != GT_OK)
    {
        DBG_INFO(("Failed (tcamOperationPerform returned GT_FAIL).\n"));
        return retVal;
    }

    DBG_INFO(("OK.\n"));
    return GT_OK;

}

/*******************************************************************************
* gtcamPurgyEntry
*
* DESCRIPTION:
*       This routine Purgy a TCAM entry.
*
* INPUTS:
*        tcamPointer - pointer to the desired entry of TCAM (0 ~ 254)
*        tcamData    - Tcam entry Data
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
GT_STATUS gtcamPurgyEntry
(
    IN  GT_QD_DEV     *dev,
    IN  GT_U32        tcamPointer,
    IN  GT_TCAM_DATA        *tcamData
)
{
    GT_STATUS           retVal;
    GT_TCAM_OPERATION    op;
    GT_TCAM_OP_DATA     tcamOpData;

    DBG_INFO(("gtcamPurgyEntry Called.\n"));

    /* check if device supports this feature */
    if (!IS_IN_DEV_GROUP(dev,DEV_TCAM))
    {
        DBG_INFO(("GT_NOT_SUPPORTED\n"));
        return GT_NOT_SUPPORTED;
    }

    /* check if the given pointer is valid */
    if ((tcamPointer > 0xFE)||(tcamData==NULL))
    {
        DBG_INFO(("GT_BAD_PARAM\n"));
        return GT_BAD_PARAM;
    }

    /* Program Tuning register */
    op = TCAM_PURGE_ENTRY;
    tcamOpData.tcamPage = 0; /* useless */
    tcamOpData.tcamEntry = tcamPointer;
    tcamOpData.tcamDataP = tcamData;
    retVal = tcamOperationPerform(dev,op, &tcamOpData);
    if(retVal != GT_OK)
    {
        DBG_INFO(("Failed (tcamOperationPerform returned GT_FAIL).\n"));
        return retVal;
    }

    DBG_INFO(("OK.\n"));
    return GT_OK;

}


/*******************************************************************************
* gtcamReadTCAMData
*
* DESCRIPTION:
*       This routine loads the global 3 offsets 0x02 to 0x1B registers with
*       the data found in the TCAM entry and its TCAM page pointed to by the TCAM
*       entry and TCAM page bits of this register (bits 7:0 and 11:10 respectively.
*
*
* INPUTS:
*        tcamPointer - pointer to the desired entry of TCAM (0 ~ 254)
*
* OUTPUTS:
*        tcamData    - Tcam entry Data
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
GT_STATUS gtcamReadTCAMData
(
    IN  GT_QD_DEV     *dev,
    IN  GT_U32        tcamPointer,
    OUT GT_TCAM_DATA        *tcamData
)
{
    GT_STATUS           retVal;
    GT_TCAM_OPERATION    op;
    GT_TCAM_OP_DATA     tcamOpData;

    DBG_INFO(("gtcamReadTCAMData Called.\n"));

    /* check if device supports this feature */
    if (!IS_IN_DEV_GROUP(dev,DEV_TCAM))
    {
        DBG_INFO(("GT_NOT_SUPPORTED\n"));
        return GT_NOT_SUPPORTED;
    }

    /* check if the given pointer is valid */
    if ((tcamPointer > 0xFE)||(tcamData==NULL))
    {
        DBG_INFO(("GT_BAD_PARAM\n"));
        return GT_BAD_PARAM;
    }

    /* Program Tuning register */
    op = TCAM_READ_ENTRY;
    tcamOpData.tcamPage = 0; /* useless */
    tcamOpData.tcamEntry = tcamPointer;
    tcamOpData.tcamDataP = tcamData;
    retVal = tcamOperationPerform(dev,op, &tcamOpData);
    if(retVal != GT_OK)
    {
        DBG_INFO(("Failed (tcamOperationPerform returned GT_FAIL).\n"));
        return retVal;
    }

    DBG_INFO(("OK.\n"));
    return GT_OK;

}

/*******************************************************************************
* gtcamGetNextTCAMData
*
* DESCRIPTION:
*       This routine  finds the next higher TCAM Entry number that is valid (i.e.,
*       any entry whose Page 0 offset 0x02 is not equal to 0x00FF). The TCAM Entry
*       register (bits 7:0) is used as the TCAM entry to start from. To find
*       the lowest number TCAM Entry that is valid, start the Get Next operation
*       with TCAM Entry set to 0xFF.
*
*
* INPUTS:
*        tcamPointer - start pointer entry of TCAM (0 ~ 255)
*
* OUTPUTS:
*        tcamPointer - next pointer entry of TCAM (0 ~ 255)
*        tcamData    - Tcam entry Data
*
* RETURNS:
*       GT_OK      - on success
*       GT_FAIL    - on error
*       GT_BAD_PARAM - if invalid parameter is given
*       GT_NOT_SUPPORTED - if current device does not support this feature.
*
* COMMENTS:
*       None
*
*******************************************************************************/
GT_STATUS gtcamGetNextTCAMData
(
    IN  GT_QD_DEV     *dev,
    IN  GT_U32        *tcamPointer,
    OUT GT_TCAM_DATA        *tcamData
)
{
    GT_STATUS           retVal;
    GT_TCAM_OPERATION    op;
    GT_TCAM_OP_DATA     tcamOpData;

    DBG_INFO(("gtcamGetNextTCAMData Called.\n"));

    /* check if device supports this feature */
    if (!IS_IN_DEV_GROUP(dev,DEV_TCAM))
    {
        DBG_INFO(("GT_NOT_SUPPORTED\n"));
        return GT_NOT_SUPPORTED;
    }

    /* check if the given pointer is valid */
    if ((*tcamPointer > 0xFF)||(tcamData==NULL))
    {
        DBG_INFO(("GT_BAD_PARAM\n"));
        return GT_BAD_PARAM;
    }

    /* Program Tuning register */
    op = TCAM_GET_NEXT_ENTRY;
    tcamOpData.tcamPage = 0; /* useless */
    tcamOpData.tcamEntry = *tcamPointer;
    tcamOpData.tcamDataP = tcamData;
    retVal = tcamOperationPerform(dev,op, &tcamOpData);
    if(retVal != GT_OK)
    {
        DBG_INFO(("Failed (tcamOperationPerform returned GT_FAIL).\n"));
        return retVal;
    }

    *tcamPointer = tcamOpData.tcamEntry;
    tcamData->rawFrmData[0].frame0.paraFrm.pg0Op = qdLong2Short(tcamOpData.tcamEntry);
    tcamData->rawFrmData[0].frame0.paraFrm.pg0Op &= 0xff;
    DBG_INFO(("OK.\n"));
    return GT_OK;

}

#if 0
/*******************************************************************************
* gtcamAddEntry
*
* DESCRIPTION:
*       Creates the new entry in TCAM.
*
* INPUTS:
*       tcamEntry    - TCAM entry to insert to the TCAM.
*
* OUTPUTS:
*       None
*
* RETURNS:
*       GT_OK          - on success
*       GT_FAIL        - on error
*       GT_BAD_PARAM   - on invalid port vector
*
* COMMENTS:
*
* GalTis:
*
*******************************************************************************/
GT_STATUS gtcamAddEntry
(
    IN GT_QD_DEV    *dev,
    IN GT_ATU_ENTRY *tcamEntry
)
{
    GT_STATUS       retVal;
    GT_ATU_ENTRY    entry;

    DBG_INFO(("gtcamAddEntry Called.\n"));
    DBG_INFO(("OK.\n"));
    return GT_OK;
}

/*******************************************************************************
* gtcamDelEntry
*
* DESCRIPTION:
*       Deletes TCAM entry.
*
* INPUTS:
*       tcamEntry - TCAM entry.
*
* OUTPUTS:
*       None.
*
* RETURNS:
*       GT_OK           - on success
*       GT_FAIL         - on error
*       GT_NO_RESOURCE  - failed to allocate a t2c struct
*       GT_NO_SUCH      - if specified address entry does not exist
*
* COMMENTS:
*
*******************************************************************************/
GT_STATUS gtcamDelEntry
(
    IN GT_QD_DEV    *dev,
    IN GT_ETHERADDR  *tcamEntry
)
{
    GT_STATUS retVal;
    GT_ATU_ENTRY    entry;

    DBG_INFO(("gtcamDelEntry Called.\n"));

    DBG_INFO(("OK.\n"));
    return GT_OK;
}


#endif

/****************************************************************************/
/* Internal functions.                                                  */
/****************************************************************************/
static GT_STATUS tcamSetPage0Data(GT_QD_DEV *dev, GT_TCAM_DATA *tcamDataP, GT_U8 extFrame)
{
  GT_STATUS       retVal;    /* Functions return value */
  int i, startLoc, endReg;


  tcamDataP->rawFrmData[extFrame].frame0.paraFrm.maskType=tcamDataP->frameTypeMask;
  if(extFrame==1)
  {
    tcamDataP->rawFrmData[extFrame].frame0.paraFrm.frame0Type=0;
  }
  else
  {
    tcamDataP->rawFrmData[extFrame].frame0.paraFrm.frame0Type=tcamDataP->frameType;
  }
  {
    tcamDataP->rawFrmData[extFrame].frame0.paraFrm.type0Res=0;
    tcamDataP->rawFrmData[extFrame].frame0.paraFrm.spvRes=0;
    tcamDataP->rawFrmData[extFrame].frame0.paraFrm.spvMask=tcamDataP->spvMask;
    tcamDataP->rawFrmData[extFrame].frame0.paraFrm.spv=tcamDataP->spv;
    tcamDataP->rawFrmData[extFrame].frame0.paraFrm.ppri0Mask=tcamDataP->ppriMask;
    tcamDataP->rawFrmData[extFrame].frame0.paraFrm.ppri0=tcamDataP->ppri;
    tcamDataP->rawFrmData[extFrame].frame0.paraFrm.pvid0MaskHi=(tcamDataP->pvidMask>>8);
    tcamDataP->rawFrmData[extFrame].frame0.paraFrm.pvid0Hi=(tcamDataP->pvid>>8);
    tcamDataP->rawFrmData[extFrame].frame0.paraFrm.pvidMask0Low=tcamDataP->pvidMask&0xff;
    tcamDataP->rawFrmData[extFrame].frame0.paraFrm.pvid0Low=tcamDataP->pvid&0xff;
  }

  if(extFrame==1)
    startLoc =48;
  else
    startLoc = 0;
  for(i=0; i<(22); i++)
  {
    tcamDataP->rawFrmData[extFrame].frame0.paraFrm.frame0[i].struc.mask=tcamDataP->frameOctetMask[i+startLoc];
    tcamDataP->rawFrmData[extFrame].frame0.paraFrm.frame0[i].struc.oct=tcamDataP->frameOctet[i+startLoc];
  }

  if(extFrame==0)
    endReg =0x1c;
  else
    endReg = 3;

  for(i=2; i<endReg; i++)
  {
    retVal = tcamWriteGlobal3Reg(dev,i, tcamDataP->rawFrmData[extFrame].frame0.frame[i]);
    if(retVal != GT_OK)
    {
      return retVal;
    }
  }

  return GT_OK;
}
static GT_STATUS tcamSetPage1Data(GT_QD_DEV *dev, GT_TCAM_DATA *tcamDataP, GT_U8 extFrame)
{
  GT_STATUS       retVal;    /* Functions return value */
  int i, startLoc;

  if(extFrame==1)
    startLoc =48;
  else
    startLoc = 0;
  for(i=0; i<(26); i++)
  {
    tcamDataP->rawFrmData[extFrame].frame1.paraFrm.frame1[i].struc.mask=tcamDataP->frameOctetMask[i+23+startLoc];
    tcamDataP->rawFrmData[extFrame].frame1.paraFrm.frame1[i].struc.mask=tcamDataP->frameOctet[i+23+startLoc];
  }
  for(i=2; i<0x1c; i++)
  {
    retVal = tcamWriteGlobal3Reg(dev, i,tcamDataP->rawFrmData[extFrame].frame1.frame[i]);
    if(retVal != GT_OK)
    {
     return retVal;
    }
  }

  return GT_OK;
}

static GT_STATUS tcamSetPage2Data(GT_QD_DEV *dev, GT_TCAM_DATA *tcamDataP, GT_U8 extFrame)
{
  GT_STATUS       retVal;    /* Functions return value */
  GT_U16          data;     /* temporary Data storage */
  int i, endReg;

  tcamDataP->continu = 0;
  if((extFrame!=1)&&(tcamDataP->is96Frame==1))
      tcamDataP->continu = 1;

  tcamDataP->rawFrmData[extFrame].frame2.paraFrm.continu=tcamDataP->continu;
  if(extFrame==0)
  {
    tcamDataP->rawFrmData[extFrame].frame2.paraFrm.interrupt=tcamDataP->interrupt;
    tcamDataP->rawFrmData[extFrame].frame2.paraFrm.IncTcamCtr=tcamDataP->IncTcamCtr;
    tcamDataP->rawFrmData[extFrame].frame2.paraFrm. pg2res1=0;
    tcamDataP->rawFrmData[extFrame].frame2.paraFrm.vidData=tcamDataP->vidData&0x07ff;

    tcamDataP->rawFrmData[extFrame].frame2.paraFrm.nextId=tcamDataP->nextId;
    tcamDataP->rawFrmData[extFrame].frame2.paraFrm.pg2res2=0;
    tcamDataP->rawFrmData[extFrame].frame2.paraFrm.qpriData=tcamDataP->qpriData;
    tcamDataP->rawFrmData[extFrame].frame2.paraFrm.pg2res3=0;
    tcamDataP->rawFrmData[extFrame].frame2.paraFrm.fpriData=tcamDataP->fpriData;

    tcamDataP->rawFrmData[extFrame].frame2.paraFrm.pg2res4=0;
    tcamDataP->rawFrmData[extFrame].frame2.paraFrm.qpriAvbData=tcamDataP->qpriAvbData;
    tcamDataP->rawFrmData[extFrame].frame2.paraFrm.pg2res5=0;
    tcamDataP->rawFrmData[extFrame].frame2.paraFrm.dpvData=tcamDataP->dpvData;

    tcamDataP->rawFrmData[extFrame].frame2.paraFrm.pg2res6=0;
    tcamDataP->rawFrmData[extFrame].frame2.paraFrm.factionData=tcamDataP->factionData;
    tcamDataP->rawFrmData[extFrame].frame2.paraFrm.pg2res7=0;
    tcamDataP->rawFrmData[extFrame].frame2.paraFrm.ldBalanceData=tcamDataP->ldBalanceData;
  }

  if(extFrame==0)
    endReg = 6;
  else
    endReg = 3;

  for(i=2; i<endReg; i++)
  {
    retVal = tcamWriteGlobal3Reg(dev, i,tcamDataP->rawFrmData[extFrame].frame2.frame[i]);
    if(retVal != GT_OK)
    {
     return retVal;
    }
  }

  if(extFrame==0)
  {
    data = (tcamDataP->debugPort );
    retVal = tcamWriteGlobal3Reg(dev,QD_REG_TCAM_P2_DEBUG_PORT,data);
    if(retVal != GT_OK)
    {
       return retVal;
    }
    data = ((tcamDataP->highHit<<8) | (tcamDataP->lowHit<<0)  );
    retVal = tcamWriteGlobal3Reg(dev,QD_REG_TCAM_P2_ALL_HIT,data);
    if(retVal != GT_OK)
    {
      return retVal;
    }
  }
  return GT_OK;
}


static GT_STATUS tcamGetPage0Data(GT_QD_DEV *dev, GT_TCAM_DATA *tcamDataP, GT_U8 extFrame)
{
  GT_STATUS       retVal;    /* Functions return value */
  int i, startLoc, endReg;

  if(extFrame==0)
    endReg =0x1c;
  else
    endReg = 3;

  for(i=2; i<endReg; i++)
  {
    retVal = tcamReadGlobal3Reg(dev, i, &tcamDataP->rawFrmData[extFrame].frame0.frame[i]);
    if(retVal != GT_OK)
    {
      return retVal;
    }
  }

  if(extFrame==0)
  {
  tcamDataP->frameTypeMask=qdShort2Char(tcamDataP->rawFrmData[extFrame].frame0.paraFrm.maskType);
  tcamDataP->frameType=qdShort2Char(tcamDataP->rawFrmData[extFrame].frame0.paraFrm.frame0Type);

  tcamDataP->spvMask=qdShort2Char(tcamDataP->rawFrmData[extFrame].frame0.paraFrm.spvMask);
  tcamDataP->spv=qdShort2Char(tcamDataP->rawFrmData[extFrame].frame0.paraFrm.spv);
  tcamDataP->ppriMask=qdShort2Char(tcamDataP->rawFrmData[extFrame].frame0.paraFrm.ppri0Mask);
  tcamDataP->pvidMask=tcamDataP->rawFrmData[extFrame].frame0.paraFrm.pvid0MaskHi;
  tcamDataP->pvidMask <<=8;
  tcamDataP->ppri=qdShort2Char(tcamDataP->rawFrmData[extFrame].frame0.paraFrm.ppri0);
  tcamDataP->pvid=tcamDataP->rawFrmData[extFrame].frame0.paraFrm.pvid0Hi;
  tcamDataP->pvid <<=8;
  tcamDataP->pvidMask |= tcamDataP->rawFrmData[extFrame].frame0.paraFrm.pvidMask0Low;
  tcamDataP->pvid |= tcamDataP->rawFrmData[extFrame].frame0.paraFrm.pvid0Low;
  }

  if(extFrame==1)
    startLoc =48;
  else
    startLoc = 0;
  for(i=0; i<(22); i++)
  {
    tcamDataP->frameOctetMask[i+startLoc]=qdShort2Char(tcamDataP->rawFrmData[extFrame].frame0.paraFrm.frame0[i].struc.mask);
    tcamDataP->frameOctet[i+startLoc]=qdShort2Char(tcamDataP->rawFrmData[extFrame].frame0.paraFrm.frame0[i].struc.oct);
  }

  return GT_OK;
}
static GT_STATUS tcamGetPage1Data(GT_QD_DEV *dev, GT_TCAM_DATA *tcamDataP, GT_U8 extFrame)
{
  GT_STATUS       retVal;    /* Functions return value */
  int i, startLoc;

  for(i=2; i<0x1c; i++)
  {
    retVal = tcamReadGlobal3Reg(dev,i,&tcamDataP->rawFrmData[extFrame].frame1.frame[i]);
    if(retVal != GT_OK)
    {
     return retVal;
    }
  }
  if(extFrame==1)
    startLoc =48;
  else
    startLoc = 0;
  for(i=0; i<(26); i++)
  {
    tcamDataP->frameOctetMask[i+23+startLoc]=qdShort2Char(tcamDataP->rawFrmData[extFrame].frame1.paraFrm.frame1[i].struc.mask);
    tcamDataP->frameOctet[i+23+startLoc]=qdShort2Char(tcamDataP->rawFrmData[extFrame].frame1.paraFrm.frame1[i].struc.mask);
  }

  return GT_OK;
}

static GT_STATUS tcamGetPage2Data(GT_QD_DEV *dev, GT_TCAM_DATA *tcamDataP, GT_U8 extFrame)
{
  GT_STATUS       retVal;    /* Functions return value */
  GT_U16          data;     /* temporary Data storage */
  int i, endReg;

/*  if(extFrame!=0)
    return GT_OK;
*/

  if(extFrame==0)
    endReg = 6;
  else
    endReg = 3;

  for(i=2; i<endReg; i++)
  {
    retVal = tcamReadGlobal3Reg(dev, i, &tcamDataP->rawFrmData[extFrame].frame2.frame[i]);
    if(retVal != GT_OK)
    {
     return retVal;
    }
  }
  if(extFrame==0)
  {

    tcamDataP->continu=qdShort2Char(tcamDataP->rawFrmData[extFrame].frame2.paraFrm.continu);
    tcamDataP->interrupt=qdShort2Char(tcamDataP->rawFrmData[extFrame].frame2.paraFrm.IncTcamCtr);
    tcamDataP->vidData=tcamDataP->rawFrmData[extFrame].frame2.paraFrm.vidData;
    tcamDataP->nextId=qdShort2Char(tcamDataP->rawFrmData[extFrame].frame2.paraFrm.nextId);
    tcamDataP->qpriData=qdShort2Char(tcamDataP->rawFrmData[extFrame].frame2.paraFrm.qpriData);
    tcamDataP->fpriData=qdShort2Char(tcamDataP->rawFrmData[extFrame].frame2.paraFrm.fpriData);
    tcamDataP->qpriAvbData=qdShort2Char(tcamDataP->rawFrmData[extFrame].frame2.paraFrm.qpriAvbData);
    tcamDataP->dpvData=qdShort2Char(tcamDataP->rawFrmData[extFrame].frame2.paraFrm.dpvData);
    tcamDataP->factionData=tcamDataP->rawFrmData[extFrame].frame2.paraFrm.factionData;
    tcamDataP->ldBalanceData=qdShort2Char(tcamDataP->rawFrmData[extFrame].frame2.paraFrm.ldBalanceData);

    i = QD_REG_TCAM_P2_DEBUG_PORT;
    retVal = tcamReadGlobal3Reg(dev,QD_REG_TCAM_P2_DEBUG_PORT,&data);
    if(retVal != GT_OK)
    {
       return retVal;
    }
    tcamDataP->rawFrmData[extFrame].frame2.frame[i] = data;
    tcamDataP->debugPort = (data)&0xf;
    i = QD_REG_TCAM_P2_ALL_HIT;
    retVal = tcamReadGlobal3Reg(dev,QD_REG_TCAM_P2_ALL_HIT,&data);
    if(retVal != GT_OK)
    {
       return retVal;
    }
    tcamDataP->rawFrmData[extFrame].frame2.frame[i] = data;
    tcamDataP->highHit = (data>>8)&0xff;
    tcamDataP->lowHit = data&0xff;
  }
  return GT_OK;
}

static GT_STATUS waitTcamReady(GT_QD_DEV           *dev)
{
    GT_STATUS       retVal;    /* Functions return value */
#ifdef GT_RMGMT_ACCESS
    {
      HW_DEV_REG_ACCESS regAccess;

      regAccess.entries = 1;

      regAccess.rw_reg_list[0].cmd = HW_REG_WAIT_TILL_0;
      regAccess.rw_reg_list[0].addr = CALC_SMI_DEV_ADDR(dev, 0, GLOBAL3_REG_ACCESS);
      regAccess.rw_reg_list[0].reg = QD_REG_TCAM_OPERATION;
      regAccess.rw_reg_list[0].data = 15;
      retVal = hwAccessMultiRegs(dev, &regAccess);
      if(retVal != GT_OK)
      {
        gtSemGive(dev,dev->tblRegsSem);
        return retVal;
      }
    }
#else
    GT_U16          data;     /* temporary Data storage */
    data = 1;
    while(data == 1)
    {
        retVal = hwGetGlobal3RegField(dev,QD_REG_TCAM_OPERATION,15,1,&data);
        if(retVal != GT_OK)
        {
            gtSemGive(dev,dev->tblRegsSem);
            return retVal;
        }
    }
#endif
    return GT_OK;
}


/*******************************************************************************
* tcamOperationPerform
*
* DESCRIPTION:
*       This function accesses TCAM Table
*
* INPUTS:
*       tcamOp   - The tcam operation
*       tcamData - address and data to be written into TCAM
*
* OUTPUTS:
*       tcamData - data read from TCAM pointed by address
*
* RETURNS:
*       GT_OK on success,
*       GT_FAIL otherwise.
*
* COMMENTS:
*
*******************************************************************************/
static GT_STATUS tcamOperationPerform
(
    IN    GT_QD_DEV           *dev,
    IN    GT_TCAM_OPERATION   tcamOp,
    INOUT GT_TCAM_OP_DATA     *opData
)
{
  GT_STATUS       retVal;    /* Functions return value */
  GT_U16          data;     /* temporary Data storage */

  gtSemTake(dev,dev->tblRegsSem,OS_WAIT_FOREVER);

  /* Wait until the tcam in ready. */
  retVal = waitTcamReady(dev);
  if(retVal != GT_OK)
  {
    gtSemGive(dev,dev->tblRegsSem);
    return retVal;
  }

  /* Set the TCAM Operation register */
  switch (tcamOp)
  {
    case TCAM_FLUSH_ALL:
      data = 0;
      data = (1 << 15) | (tcamOp << 12);
      retVal = tcamWriteGlobal3Reg(dev,QD_REG_TCAM_OPERATION,data);
      if(retVal != GT_OK)
      {
        gtSemGive(dev,dev->tblRegsSem);
        return retVal;
      }
      break;

    case TCAM_FLUSH_ENTRY:
    {
      int i, extFrame;
      if(opData->tcamDataP->is96Frame==1)
        extFrame = 2;
      else
        extFrame = 1;
      for(i=0; i<extFrame; i++)
      {
        data = 0;
        data = qdLong2Short((1 << 15) | (tcamOp << 12) | (opData->tcamEntry+i)) ;
        retVal = tcamWriteGlobal3Reg(dev,QD_REG_TCAM_OPERATION,data);
        if(retVal != GT_OK)
        {
          gtSemGive(dev,dev->tblRegsSem);
          return retVal;
        }
      }
    }
      break;

    case TCAM_LOAD_ENTRY:
    case TCAM_PURGE_ENTRY:
    {
      int i, extFrame;
      if((opData->tcamDataP->is96Frame==1)&&(tcamOp!=TCAM_PURGE_ENTRY))
        extFrame = 2;
      else
        extFrame = 1;

      for(i=0; i<extFrame; i++)
      {
        if(tcamOp!=TCAM_PURGE_ENTRY)
        {
          retVal = tcamSetPage2Data(dev, opData->tcamDataP, i);
          if(retVal != GT_OK)
          {
            gtSemGive(dev,dev->tblRegsSem);
            return retVal;
          }

          data = 0;
          data = qdLong2Short((1 << 15) | (TCAM_LOAD_ENTRY << 12) | (2 << 10) | (opData->tcamEntry+i));
          retVal = tcamWriteGlobal3Reg(dev,QD_REG_TCAM_OPERATION,data);
          if(retVal != GT_OK)
          {
            gtSemGive(dev,dev->tblRegsSem);
            return retVal;
          }
          retVal = waitTcamReady(dev);
          if(retVal != GT_OK)
          {
            gtSemGive(dev,dev->tblRegsSem);
            return retVal;
          }

          retVal = tcamSetPage1Data(dev, opData->tcamDataP, i);
          if(retVal != GT_OK)
          {
            gtSemGive(dev,dev->tblRegsSem);
            return retVal;
          }
          data = 0;
          data = qdLong2Short((1 << 15) | (TCAM_LOAD_ENTRY << 12) | (1 << 10) | (opData->tcamEntry+i)) ;
          retVal = tcamWriteGlobal3Reg(dev,QD_REG_TCAM_OPERATION,data);
          if(retVal != GT_OK)
          {
            gtSemGive(dev,dev->tblRegsSem);
            return retVal;
          }

          retVal = waitTcamReady(dev);
          if(retVal != GT_OK)
          {
            gtSemGive(dev,dev->tblRegsSem);
            return retVal;
          }

        }
        retVal = tcamSetPage0Data(dev,  opData->tcamDataP, i);
        if(retVal != GT_OK)
        {
          gtSemGive(dev,dev->tblRegsSem);
          return retVal;
        }
        if(tcamOp==TCAM_PURGE_ENTRY)
        {
          data = 0xffff ;
          retVal = tcamWriteGlobal3Reg(dev,QD_REG_TCAM_P0_KEYS_1,data);
          if(retVal != GT_OK)
          {
            gtSemGive(dev,dev->tblRegsSem);
            return retVal;
          }
#if 0
          retVal = tcamReadGlobal3Reg(dev, QD_REG_TCAM_P0_KEYS_1, &data);
          if(retVal != GT_OK)
          {
            gtSemGive(dev,dev->tblRegsSem);
            return retVal;
          }
#endif
        }
        data = 0;
        data = qdLong2Short((1 << 15) | (TCAM_LOAD_ENTRY << 12) | (0 << 10) | (opData->tcamEntry+i));
        retVal = tcamWriteGlobal3Reg(dev,QD_REG_TCAM_OPERATION,data);
        if(retVal != GT_OK)
        {
          gtSemGive(dev,dev->tblRegsSem);
          return retVal;
        }
        retVal = waitTcamReady(dev);
        if(retVal != GT_OK)
        {
          gtSemGive(dev,dev->tblRegsSem);
          return retVal;
        }

#if 0 /* Test read back */
          retVal = tcamReadGlobal3Reg(dev, QD_REG_TCAM_P0_KEYS_1, &data);
          if(retVal != GT_OK)
          {
            gtSemGive(dev,dev->tblRegsSem);
            return retVal;
          }

        data = 0;
        data = (1 << 15) | (TCAM_READ_ENTRY << 12) | (0 << 10) | (opData->tcamEntry+i);
        retVal = tcamWriteGlobal3Reg(dev,QD_REG_TCAM_OPERATION,data);
        if(retVal != GT_OK)
        {
          gtSemGive(dev,dev->tblRegsSem);
          return retVal;
        }
        retVal = waitTcamReady(dev);
        if(retVal != GT_OK)
        {
          gtSemGive(dev,dev->tblRegsSem);
          return retVal;
        }
{
  int j;
  for(j=0; j<0x1c; j++)
  {
/*          retVal = tcamReadGlobal3Reg(dev, QD_REG_TCAM_P0_KEYS_1, &data); */
          retVal = tcamReadGlobal3Reg(dev, j, &data);
          if(retVal != GT_OK)
          {
            gtSemGive(dev,dev->tblRegsSem);
            return retVal;
          }
  }
}

#endif
      }
  }
    break;

    case TCAM_GET_NEXT_ENTRY:
    {
      data = 0;
      data = qdLong2Short((1 << 15) | (tcamOp << 12) | (opData->tcamEntry)) ;
      retVal = tcamWriteGlobal3Reg(dev,QD_REG_TCAM_OPERATION,data);
      if(retVal != GT_OK)
      {
        gtSemGive(dev,dev->tblRegsSem);
        return retVal;
      }
      /* Wait until the tcam in ready. */
      retVal = waitTcamReady(dev);
      if(retVal != GT_OK)
      {
        gtSemGive(dev,dev->tblRegsSem);
        return retVal;
      }

      retVal = tcamReadGlobal3Reg(dev,QD_REG_TCAM_OPERATION, &data);
      if(retVal != GT_OK)
      {
        gtSemGive(dev,dev->tblRegsSem);
        return retVal;
      }

/*      if(opData->tcamEntry == 0xff)   If ask to find the lowest entry*/
      {
        if ((data&0xff)==0xff)
        {
          retVal = tcamReadGlobal3Reg(dev,QD_REG_TCAM_P0_KEYS_1, &data);
          if(retVal != GT_OK)
          {
            gtSemGive(dev,dev->tblRegsSem);
            return retVal;
          }
          if(data==0x00ff)
          {
            /* No higher valid TCAM entry */
            return GT_OK;
          }
          else
          {
            /* The highest valid TCAM entry found*/
          }
        }
      }

      /* Get next entry and read the entry */
      opData->tcamEntry = data&0xff;
    }
    case TCAM_READ_ENTRY:
    {
      int i, extFrame;
      if(opData->tcamDataP->is96Frame==1)
        extFrame = 2;
      else
        extFrame = 1;
      for(i=0; i<extFrame; i++)
      {
        data = 0;
        /* Read page 0 */
        data = qdLong2Short((1 << 15) | (tcamOp << 12) | (0 << 10) | (opData->tcamEntry+i)) ;
        opData->tcamDataP->rawFrmData[i].frame0.frame[0] = data;
        retVal = tcamWriteGlobal3Reg(dev,QD_REG_TCAM_OPERATION,data);
        if(retVal != GT_OK)
        {
          gtSemGive(dev,dev->tblRegsSem);
          return retVal;
        }
        /* Wait until the tcam in ready. */
        retVal = waitTcamReady(dev);
        if(retVal != GT_OK)
        {
          gtSemGive(dev,dev->tblRegsSem);
          return retVal;
        }

        retVal = tcamGetPage0Data(dev, opData->tcamDataP, i);
        if(retVal != GT_OK)
        {
          gtSemGive(dev,dev->tblRegsSem);
          return retVal;
        }

        data = 0;
        /* Read page 1 */
        data = qdLong2Short((1 << 15) | (tcamOp << 12) | (1 << 10) | (opData->tcamEntry+i)) ;
        retVal = tcamWriteGlobal3Reg(dev,QD_REG_TCAM_OPERATION,data);
        if(retVal != GT_OK)
        {
          gtSemGive(dev,dev->tblRegsSem);
          return retVal;
        }
        /* Wait until the tcam in ready. */
        retVal = waitTcamReady(dev);
        if(retVal != GT_OK)
        {
          gtSemGive(dev,dev->tblRegsSem);
          return retVal;
        }

        retVal = tcamGetPage1Data(dev, opData->tcamDataP, i);
        if(retVal != GT_OK)
        {
          gtSemGive(dev,dev->tblRegsSem);
          return retVal;
        }

        data = 0;
        /* Read page 2 */
        data = qdLong2Short((1 << 15) | (tcamOp << 12) | (2 << 10) | (opData->tcamEntry+i)) ;
        retVal = tcamWriteGlobal3Reg(dev,QD_REG_TCAM_OPERATION,data);
        if(retVal != GT_OK)
        {
          gtSemGive(dev,dev->tblRegsSem);
          return retVal;
        }
        /* Wait until the tcam in ready. */
        retVal = waitTcamReady(dev);
        if(retVal != GT_OK)
        {
          gtSemGive(dev,dev->tblRegsSem);
          return retVal;
        }

        retVal = tcamGetPage2Data(dev, opData->tcamDataP, i);
        if(retVal != GT_OK)
        {
          gtSemGive(dev,dev->tblRegsSem);
          return retVal;
        }
      }
    }
    break;

    default:
      gtSemGive(dev,dev->tblRegsSem);
      return GT_FAIL;
}

  gtSemGive(dev,dev->tblRegsSem);
  return retVal;
}
