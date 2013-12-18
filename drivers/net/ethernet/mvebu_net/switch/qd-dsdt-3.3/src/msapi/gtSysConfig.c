#include <Copyright.h>

/********************************************************************************
* gtSysConfig.c
*
* DESCRIPTION:
*       API definitions for system configuration, and enabling.
*
* DEPENDENCIES:
*       None.
*
* FILE REVISION NUMBER:
*       $Revision: 4 $
*
*******************************************************************************/

#include <msApi.h>
#include <msApiPrototype.h>
#include <gtDrvConfig.h>
#include <gtSem.h>
#include <platformDeps.h>
#ifdef GT_USE_MAD
#include "madApi.h"
#include "madApiDefs.h"
#endif
#include <gtHwCntl.h>

extern GT_U8 lport2port(IN GT_U16 portVec, IN GT_LPORT  port);
extern GT_LPORT port2lport(IN GT_U16 portVec, IN GT_U8  hwPort);
extern GT_U32 lportvec2portvec(IN GT_U16 portVec, IN GT_U32  lVec);
extern GT_U32 portvec2lportvec(IN GT_U16 portVec, IN GT_U32  pVec);
static GT_BOOL gtRegister(GT_QD_DEV *qd_dev, BSP_FUNCTIONS* pBSPFunctions);

#ifdef GT_USE_MAD
static MAD_BOOL madSMIRead(MAD_DEV* dev, unsigned int smiAddr,
              unsigned int reg, unsigned int* value)
{
  GT_STATUS  status;
  GT_U16 data;
  status =     hwReadPhyReg((GT_QD_DEV *)(dev->swDev), smiAddr, reg, &data);

  if(status == GT_OK)
  {
    *value = data;
    return MAD_TRUE;
  }
  else
    return MAD_FALSE;
}

static MAD_BOOL madSMIWrite(MAD_DEV* dev, unsigned int smiAddr,
              unsigned int reg, unsigned int value)
{
  GT_STATUS  status;
  GT_U16 data;

  data = value;
  status =     hwWritePhyReg((GT_QD_DEV *)(dev->swDev), smiAddr, reg, data);

  if(status == GT_OK)
    return MAD_TRUE;
  else
    return MAD_FALSE;
}

static char * madGetDeviceName ( MAD_DEVICE_ID deviceId)
{

    switch (deviceId)
    {
        case MAD_88E10X0: return ("MAD_88E10X0 ");
        case MAD_88E10X0S: return ("MAD_88E10X0S ");
        case MAD_88E1011: return ("MAD_88E1011 ");
        case MAD_88E104X: return ("MAD_88E104X ");
        case MAD_88E1111: return ("MAD_88E1111/MAD_88E1115 ");
        case MAD_88E1112: return ("MAD_88E1112 ");
        case MAD_88E1116: return ("MAD_88E1116/MAD_88E1116R ");
        case MAD_88E114X: return ("MAD_88E114X ");
        case MAD_88E1149: return ("MAD_88E1149 ");
        case MAD_88E1149R: return ("MAD_88E1149R ");
        case MAD_SWG65G : return ("MAD_SWG65G ");
        case MAD_88E1181: return ("MAD_88E1181 ");
        case MAD_88E3016: return ("MAD_88E3015/MAD_88E3016/MAD_88E3018/MAD_88E3019 ");
/*        case MAD_88E3019: return ("MAD_88E3019 "); */
        case MAD_88E1121: return ("MAD_88E1121/MAD_88E1121R ");
        case MAD_88E3082: return ("MAD_88E3082/MAD_88E3083 ");
        case MAD_88E1240: return ("MAD_88E1240 ");
        case MAD_88E1340S: return ("MAD_88E1340S ");
        case MAD_88E1340: return ("MAD_88E1340 ");
        case MAD_88E1340M: return ("MAD_88E1340M ");
        case MAD_88E1119R: return ("MAD_88E1119R ");
        case MAD_88E1310:  return ("MAD_88E1310 ");
        case MAD_MELODY:  return ("MAD_MELODY_PHY ");
        case MAD_88E1540:  return ("MAD_88E1540 ");
        case MAD_88E3183:  return ("MAD_88E3183 ");
        case MAD_88E3061:  return ("MAD_88E3061 ");
        case MAD_88E1510:  return ("MAD_88E1510 ");
        case MAD_88E1548:  return ("MAD_88E1548 ");
        default : return (" No-name ");
    }
} ;


static MAD_STATUS madStart(GT_QD_DEV* qd_dev,  int smiPair)
{
	int port;
    MAD_STATUS status = MAD_FAIL;
    MAD_DEV* dev = (MAD_DEV*)&(qd_dev->mad_dev);
    MAD_SYS_CONFIG   cfg;
    cfg.BSPFunctions.readMii   = (FMAD_READ_MII )madSMIRead;
    cfg.BSPFunctions.writeMii  = (FMAD_WRITE_MII )madSMIWrite;
    cfg.BSPFunctions.semCreate = NULL;
    cfg.BSPFunctions.semDelete = NULL;
    cfg.BSPFunctions.semTake   = NULL;
    cfg.BSPFunctions.semGive   = NULL;

    dev->swDev = (void *)qd_dev;
	cfg.smiBaseAddr = smiPair;  /* Set SMI Address */
	cfg.switchType = MAD_SYS_SW_TYPE_NO;
	if((qd_dev->deviceId==GT_88E6320)||
	   (qd_dev->deviceId==GT_88E6310)||
	   (qd_dev->deviceId==GT_88E6310)||
	   (qd_dev->deviceId==GT_88E6310))
	{
	  cfg.switchType = MAD_SYS_SW_TYPE_1;
	}

    if((status=mdLoadDriver(&cfg, dev)) != MAD_OK)
    {
        return status;
    }
    dev->phyInfo.swPhyType = 1;  /* The Phy is part of switch*/

	/* to set parameters to ports of phy ports added in switch*/
	for (port=dev->numOfPorts; port < qd_dev->maxPhyNum; port++)
      dev->phyInfo.hwMode[port] = dev->phyInfo.hwMode[0];
    dev->numOfPorts = qd_dev->maxPhyNum;
/*    dev->numOfPorts = qd_dev->numOfPorts; */

    DBG_INFO(("Device Name   : %s\n", madGetDeviceName(dev->deviceId)));
    DBG_INFO(("Device ID     : 0x%x\n",dev->deviceId));
    DBG_INFO(("Revision      : 0x%x\n",dev->revision));
    DBG_INFO(("Base Reg Addr : 0x%x\n",dev->baseRegAddr));
/*    DBG_INFO(("No of Ports   : %d\n",dev->numOfPorts)); */
    DBG_INFO(("No of Ports   : %d\n",qd_dev->maxPhyNum));
    DBG_INFO(("QD dev        : %x\n",dev->swDev));

    DBG_INFO(("MAD has been started.\n"));

    qd_dev->use_mad = GT_TRUE;
    return MAD_OK;
}

/*
static void madClose(MAD_DEV* dev)
{
    if (dev->devEnabled)
        mdUnloadDriver(dev);
}
*/

 GT_STATUS qd_madInit(GT_QD_DEV    *dev, int phyAddr)
{
  MAD_STATUS    status;


  status = madStart(dev, phyAddr);
  if (MAD_OK != status)
  {
        DBG_INFO(("sMAD Initialization Failed.\n"));
        qdUnloadDriver(dev);
        return GT_FAIL;
  }

  return GT_OK;
}

#endif /* GT_USE_MAD */

/*******************************************************************************
* qdLoadDriver
*
* DESCRIPTION:
*       QuarterDeck Driver Initialization Routine.
*       This is the first routine that needs be called by system software.
*       It takes *cfg from system software, and retures a pointer (*dev)
*       to a data structure which includes infomation related to this QuarterDeck
*       device. This pointer (*dev) is then used for all the API functions.
*
* INPUTS:
*       cfg  - Holds device configuration parameters provided by system software.
*
* OUTPUTS:
*       dev  - Holds device information to be used for each API call.
*
* RETURNS:
*       GT_OK               - on success
*       GT_FAIL             - on error
*       GT_ALREADY_EXIST    - if device already started
*       GT_BAD_PARAM        - on bad parameters
*
* COMMENTS:
*     qdUnloadDriver is also provided to do driver cleanup.
*
*******************************************************************************/
GT_STATUS qdLoadDriver
(
    IN  GT_SYS_CONFIG   *cfg,
    OUT GT_QD_DEV    *dev
)
{
    GT_STATUS   retVal;
    GT_LPORT    port;

    DBG_INFO(("qdLoadDriver Called.\n"));

    /* Check for parameters validity        */
    if(dev == NULL)
    {
        DBG_INFO(("Failed.\n"));
        return GT_BAD_PARAM;
    }

    /* Check for parameters validity        */
    if(cfg == NULL)
    {
        DBG_INFO(("Failed.\n"));
        return GT_BAD_PARAM;
    }

    /* The initialization was already done. */
    if(dev->devEnabled)
    {
        DBG_INFO(("QuarterDeck already started.\n"));
        return GT_ALREADY_EXIST;
    }

#ifdef GT_PORT_MAP_IN_DEV
    /* Modified to add port mapping functions into device ssystem configuration. */

    if (dev->lport2port == NULL) {
      dev->lport2port = lport2port;
    }

    if (dev->port2lport == NULL) {
      dev->port2lport = port2lport;
    }

    if (dev->lportvec2portvec == NULL) {
      dev->lportvec2portvec = lportvec2portvec;
    }

    if (dev->portvec2lportvec == NULL) {
      dev->portvec2lportvec = portvec2lportvec;
    }
#endif

    if(gtRegister(dev,&(cfg->BSPFunctions)) != GT_TRUE)
    {
       DBG_INFO(("gtRegister Failed.\n"));
       return GT_FAIL;
    }
    dev->accessMode = (GT_U8)cfg->mode.scanMode;
    if (dev->accessMode == SMI_MULTI_ADDR_MODE)
    {
        dev->baseRegAddr = 0;
        dev->phyAddr = (GT_U8)cfg->mode.baseAddr;
    }
    else
    {
        dev->baseRegAddr = (GT_U8)cfg->mode.baseAddr;
        dev->phyAddr = 0;
    }


    /* Initialize the driver    */
    retVal = driverConfig(dev);
    if(retVal != GT_OK)
    {
        DBG_INFO(("driverConfig Failed.\n"));
        return retVal;
    }

    /* Initialize dev fields.         */
    dev->cpuPortNum = cfg->cpuPortNum;
    dev->maxPhyNum = 5;
    dev->devGroup = 0;
    dev->devStorage = 0;
    /* Assign Device Name */
    dev->devName = 0;
    dev->devName1 = 0;

    dev->validSerdesVec = 0;

    if((dev->deviceId&0xfff8)==GT_88EC000) /* device id 0xc00 - 0xc07 are GT_88EC0XX */
      dev->deviceId=GT_88EC000;

	if (dev->deviceId == 0xc10)
		dev->deviceId = GT_88E6352;

    switch(dev->deviceId)
    {
        case GT_88E6021:
                dev->numOfPorts = 3;
                dev->maxPorts = 3;
                dev->maxPhyNum = 2;
                dev->validPortVec = (1 << dev->numOfPorts) - 1;
                dev->validPhyVec = (1 << dev->maxPhyNum) - 1;
                dev->devName = DEV_88E6021;
                break;

        case GT_88E6051:
                dev->numOfPorts = 5;
                dev->maxPorts = 5;
                dev->maxPhyNum = 5;
                dev->validPortVec = (1 << dev->numOfPorts) - 1;
                dev->validPhyVec = (1 << dev->maxPhyNum) - 1;
                dev->devName = DEV_88E6051;
                break;

        case GT_88E6052:
                dev->numOfPorts = 7;
                dev->maxPorts = 7;
                dev->maxPhyNum = 5;
                dev->validPortVec = (1 << dev->numOfPorts) - 1;
                dev->validPhyVec = (1 << dev->maxPhyNum) - 1;
                dev->devName = DEV_88E6052;
                break;

        case GT_88E6060:
                if((dev->cpuPortNum != 4)&&(dev->cpuPortNum != 5))
                {
                    return GT_FAIL;
                }
                dev->numOfPorts = 6;
                dev->maxPorts = 6;
                dev->maxPhyNum = 5;
                dev->validPortVec = (1 << dev->numOfPorts) - 1;
                dev->validPhyVec = (1 << dev->maxPhyNum) - 1;
                dev->devName = DEV_88E6060;
                break;

        case GT_88E6031:
                dev->numOfPorts = 3;
                dev->maxPorts = 6;
                dev->maxPhyNum = 3;
                dev->validPortVec = 0x31;    /* port 0, 4, and 5 */
                dev->validPhyVec = 0x31;    /* port 0, 4, and 5 */
                dev->devName = DEV_88E6061;
                break;

        case GT_88E6061:
                dev->numOfPorts = 6;
                dev->maxPorts = 6;
                dev->maxPhyNum = 6;
                dev->validPortVec = (1 << dev->numOfPorts) - 1;
                dev->validPhyVec = (1 << dev->maxPhyNum) - 1;
                dev->devName = DEV_88E6061;
                break;

        case GT_88E6035:
                dev->numOfPorts = 3;
                dev->maxPorts = 6;
                dev->maxPhyNum = 3;
                dev->validPortVec = 0x31;    /* port 0, 4, and 5 */
                dev->validPhyVec = 0x31;    /* port 0, 4, and 5 */
                dev->devName = DEV_88E6065;
                break;

        case GT_88E6055:
                dev->numOfPorts = 5;
                dev->maxPorts = 6;
                dev->maxPhyNum = 5;
                dev->validPortVec = 0x2F;    /* port 0,1,2,3, and 5 */
                dev->validPhyVec = 0x2F;    /* port 0,1,2,3, and 5 */
                dev->devName = DEV_88E6065;
                break;

        case GT_88E6065:
                dev->numOfPorts = 6;
                dev->maxPorts = 6;
                dev->maxPhyNum = 6;
                dev->validPortVec = (1 << dev->numOfPorts) - 1;
                dev->validPhyVec = (1 << dev->maxPhyNum) - 1;
                dev->devName = DEV_88E6065;
                break;

        case GT_88E6063:
                dev->numOfPorts = 7;
                dev->maxPorts = 7;
                dev->maxPhyNum = 5;
                dev->validPortVec = (1 << dev->numOfPorts) - 1;
                dev->validPhyVec = (1 << dev->maxPhyNum) - 1;
                dev->devName = DEV_88E6063;
                break;

        case GT_FH_VPN:
                dev->numOfPorts = 7;
                dev->maxPorts = 7;
                dev->maxPhyNum = 5;
                dev->validPortVec = (1 << dev->numOfPorts) - 1;
                dev->validPhyVec = (1 << dev->maxPhyNum) - 1;
                dev->devName = DEV_FH_VPN;
                break;

        case GT_FF_EG:
                if(dev->cpuPortNum != 5)
                {
                    return GT_FAIL;
                }
                dev->numOfPorts = 6;
                dev->maxPorts = 6;
                dev->maxPhyNum = 5;
                dev->validPortVec = (1 << dev->numOfPorts) - 1;
                dev->validPhyVec = (1 << dev->maxPhyNum) - 1;
                dev->devName = DEV_FF_EG;
                break;

        case GT_FF_HG:
                dev->numOfPorts = 7;
                dev->maxPorts = 7;
                dev->maxPhyNum = 5;
                dev->validPortVec = (1 << dev->numOfPorts) - 1;
                dev->validPhyVec = (1 << dev->maxPhyNum) - 1;
                dev->devName = DEV_FF_HG;
                break;

        case GT_88E6083:
                dev->numOfPorts = 10;
                dev->maxPorts = 10;
                dev->maxPhyNum = 8;
                dev->validPortVec = (1 << dev->numOfPorts) - 1;
                dev->validPhyVec = (1 << dev->maxPhyNum) - 1;
                dev->devName = DEV_88E6083;
                break;

        case GT_88E6153:
                dev->numOfPorts = 6;
                dev->maxPorts = 6;
                dev->maxPhyNum = 6;
                dev->validPortVec = (1 << dev->numOfPorts) - 1;
                dev->validPhyVec = (1 << dev->maxPhyNum) - 1;
                dev->devName = DEV_88E6183;
                break;

        case GT_88E6181:
                dev->numOfPorts = 8;
                dev->maxPorts = 8;
                dev->maxPhyNum = 8;
                dev->validPortVec = (1 << dev->numOfPorts) - 1;
                dev->validPhyVec = (1 << dev->maxPhyNum) - 1;
                dev->devName = DEV_88E6181;
                break;

        case GT_88E6183:
                dev->numOfPorts = 10;
                dev->maxPorts = 10;
                dev->maxPhyNum = 10;
                dev->validPortVec = (1 << dev->numOfPorts) - 1;
                dev->validPhyVec = (1 << dev->maxPhyNum) - 1;
                dev->devName = DEV_88E6183;
                break;

        case GT_88E6093:
                dev->numOfPorts = 11;
                dev->maxPorts = 11;
                dev->maxPhyNum = 11;
                dev->validPortVec = (1 << dev->numOfPorts) - 1;
                dev->validPhyVec = (1 << dev->maxPhyNum) - 1;
                dev->devName = DEV_88E6093;
                break;

        case GT_88E6092:
                dev->numOfPorts = 11;
                dev->maxPorts = 11;
                dev->maxPhyNum = 11;
                dev->validPortVec = (1 << dev->numOfPorts) - 1;
                dev->validPhyVec = (1 << dev->maxPhyNum) - 1;
                dev->devName = DEV_88E6092;
                break;

        case GT_88E6095:
                dev->numOfPorts = 11;
                dev->maxPorts = 11;
                dev->maxPhyNum = 11;
                dev->validPortVec = (1 << dev->numOfPorts) - 1;
                dev->validPhyVec = (1 << dev->maxPhyNum) - 1;
                dev->devName = DEV_88E6095;
                break;

        case GT_88E6045:
                dev->numOfPorts = 6;
                dev->maxPorts = 11;
                dev->maxPhyNum = 11;
                dev->validPortVec = 0x60F;
                dev->validPhyVec = 0x60F;
                dev->devName = DEV_88E6095;
                break;

        case GT_88E6097:
                dev->numOfPorts = 11;
                dev->maxPorts = 11;
                dev->maxPhyNum = 11;
                dev->validPortVec = (1 << dev->numOfPorts) - 1;
                dev->validPhyVec = (1 << dev->maxPhyNum) - 1;
                dev->devName = DEV_88E6097;
                break;

        case GT_88E6096:
                dev->numOfPorts = 11;
                dev->maxPorts = 11;
                dev->maxPhyNum = 11;
                dev->validPortVec = (1 << dev->numOfPorts) - 1;
                dev->validPhyVec = (1 << dev->maxPhyNum) - 1;
                dev->devName = DEV_88E6096;
                break;

        case GT_88E6047:
                dev->numOfPorts = 6;
                dev->maxPorts = 11;
                dev->maxPhyNum = 11;
                dev->validPortVec = 0x60F;
                dev->validPhyVec = 0x60F;
                dev->devName = DEV_88E6097;
                break;

        case GT_88E6046:
                dev->numOfPorts = 6;
                dev->maxPorts = 11;
                dev->maxPhyNum = 11;
                dev->validPortVec = 0x60F;
                dev->validPhyVec = 0x60F;
                dev->devName = DEV_88E6096;
                break;

        case GT_88E6085:
                dev->numOfPorts = 10;
                dev->maxPorts = 11;
                dev->maxPhyNum = 11;
                dev->validPortVec = 0x6FF;
                dev->validPhyVec = 0x6FF;
                dev->devName = DEV_88E6096;
                break;

        case GT_88E6152:
                dev->numOfPorts = 6;
                dev->maxPorts = 6;
                dev->maxPhyNum = 6;
                dev->validPortVec = 0x28F;
                dev->validPhyVec = 0x28F;
                dev->devName = DEV_88E6182;
                break;

        case GT_88E6155:
                dev->numOfPorts = 6;
                dev->maxPorts = 6;
                dev->maxPhyNum = 6;
                dev->validPortVec = 0x28F;
                dev->validPhyVec = 0x28F;
                dev->devName = DEV_88E6185;
                break;

        case GT_88E6182:
                dev->numOfPorts = 10;
                dev->maxPorts = 10;
                dev->maxPhyNum = 10;
                dev->validPortVec = (1 << dev->numOfPorts) - 1;
                dev->validPhyVec = (1 << dev->maxPhyNum) - 1;
                dev->devName = DEV_88E6182;
                break;

        case GT_88E6185:
                dev->numOfPorts = 10;
                dev->maxPorts = 10;
                dev->maxPhyNum = 10;
                dev->validPortVec = (1 << dev->numOfPorts) - 1;
                dev->validPhyVec = (1 << dev->maxPhyNum) - 1;
                dev->devName = DEV_88E6185;
                break;

        case GT_88E6121:
                dev->numOfPorts = 3;
                dev->maxPorts = 8;
                dev->maxPhyNum = 3;
                dev->validPortVec = 0xE;    /* port 1, 2, and 3 */
                dev->validPhyVec = 0xE;        /* port 1, 2, and 3 */
                dev->devName = DEV_88E6108;
                break;

        case GT_88E6122:
                dev->numOfPorts = 6;
                dev->maxPorts = 8;
                dev->maxPhyNum = 16;
                dev->validPortVec = 0x7E;    /* port 1 ~ 6 */
                dev->validPhyVec = 0xF07E;    /* port 1 ~ 6, 12 ~ 15 (serdes) */
                dev->validSerdesVec = 0xF000;
                dev->devName = DEV_88E6108;
                break;

        case GT_88E6131:
        case GT_88E6108:
                dev->numOfPorts = 8;
                dev->maxPorts = 8;
                dev->maxPhyNum = 16;
                dev->validPortVec = (1 << dev->numOfPorts) - 1;
                dev->validPhyVec = (1 << dev->maxPhyNum) - 1;
                dev->validSerdesVec = 0xF000;
                dev->devName = DEV_88E6108;
                break;

        case GT_88E6123:
                dev->numOfPorts = 3;
                dev->maxPorts = 6;
                dev->maxPhyNum = 14;
                dev->validPortVec = 0x23;
                dev->validPhyVec = 0x303F;
                dev->validSerdesVec = 0x3000;
                dev->devName = DEV_88E6161;
                break;

        case GT_88E6140:
                dev->numOfPorts = 6;
                dev->maxPorts = 6;
                dev->maxPhyNum = 14;
                dev->validPortVec = (1 << dev->numOfPorts) - 1;
                dev->validPhyVec = 0x303F;
                dev->validSerdesVec = 0x3000;
                dev->devName = DEV_88E6165;
                break;

        case GT_88E6161:
                dev->numOfPorts = 6;
                dev->maxPorts = 6;
                dev->maxPhyNum = 14;
                dev->validPortVec = (1 << dev->numOfPorts) - 1;
                dev->validPhyVec = 0x303F;
                dev->validSerdesVec = 0x3000;
                dev->devName = DEV_88E6161;
                break;

        case GT_88E6165:
                dev->numOfPorts = 6;
                dev->maxPorts = 6;
                dev->maxPhyNum = 14;
                dev->validPortVec = (1 << dev->numOfPorts) - 1;
                dev->validPhyVec = 0x303F;
                dev->validSerdesVec = 0x3000;
                dev->devName = DEV_88E6165;
                break;

        case GT_88E6351:
                dev->numOfPorts = 7;
                dev->maxPorts = 7;
                dev->maxPhyNum = 5;
                dev->validPortVec = (1 << dev->numOfPorts) - 1;
                dev->validPhyVec = 0x1F;
                dev->devName = DEV_88E6351;
                break;

        case GT_88E6175:
                dev->numOfPorts = 7;
                dev->maxPorts = 7;
                dev->maxPhyNum = 5;
                dev->validPortVec = (1 << dev->numOfPorts) - 1;
                dev->validPhyVec = 0x1F;
                dev->devName1 = DEV_88E6175; /* test device group 1 */
                break;

        case GT_88E6124 :
                dev->numOfPorts = 4;
                dev->maxPorts = 7;
                dev->maxPhyNum = 7;
                dev->validPortVec = (1 << dev->numOfPorts) - 1;
                dev->validPortVec &= ~(0x7);
                dev->validPhyVec = 0x78;
                dev->devName = DEV_88E6171;
                break;

        case GT_88E6171 :
                dev->numOfPorts = 7;
                dev->maxPorts = 7;
                dev->maxPhyNum = 5;
                dev->validPortVec = (1 << dev->numOfPorts) - 1;
                dev->validPhyVec = 0x1F;
                dev->devName = DEV_88E6171;
                break;

        case GT_88E6321 :
                dev->numOfPorts = 7;
                dev->maxPorts = 7;
                dev->maxPhyNum = 5;
                dev->validPortVec = (1 << dev->numOfPorts) - 1;
                dev->validPortVec &= ~(0x7);
                dev->validPhyVec = 0x1F;
                dev->devName = DEV_88E6371;
                break;

        case GT_88E6350 :
                dev->numOfPorts = 7;
                dev->maxPorts = 7;
                dev->maxPhyNum = 5;
                dev->validPortVec = (1 << dev->numOfPorts) - 1;
                dev->validPhyVec = 0x1F;
                dev->devName = DEV_88E6371;
                break;

        case GT_88EC000 :
                dev->numOfPorts = 7;
                dev->maxPorts = 7;
                dev->maxPhyNum = 5;
                dev->validPortVec = (1 << dev->numOfPorts) - 1;
                dev->validPhyVec = 0x1F;
                dev->devName1 = DEV_88EC000;
                break;
        case GT_88E3020:
                dev->numOfPorts = 7;
                dev->maxPorts = 7;
                dev->maxPhyNum = 5;
                dev->validPortVec = (1 << dev->numOfPorts) - 1;
                dev->validPhyVec = 0x1F;
                dev->devName1 = DEV_88E3020;
                break;
        case GT_88E6020:
                dev->numOfPorts = 7;
                dev->maxPorts = 7;
                dev->maxPhyNum = 5;
                dev->validPortVec = (1 << dev->numOfPorts) - 1;
                dev->validPhyVec = 0x1F;
                dev->devName1 = DEV_88E3020;
                break;
        case GT_88E6070:
                dev->numOfPorts = 7;
                dev->maxPorts = 7;
                dev->maxPhyNum = 5;
                dev->validPortVec = (1 << dev->numOfPorts) - 1;
                dev->validPhyVec = 0x1F;
                dev->devName1 = DEV_88E3020;
                break;
        case GT_88E6071:
                dev->numOfPorts = 7;
                dev->maxPorts = 7;
                dev->maxPhyNum = 5;
                dev->validPortVec = (1 << dev->numOfPorts) - 1;
                dev->validPhyVec = 0x1F;
                dev->devName1 = DEV_88E3020;
                break;
        case GT_88E6220:
                dev->numOfPorts = 7;
                dev->maxPorts = 7;
                dev->maxPhyNum = 5;
                dev->validPortVec = (1 << dev->numOfPorts) - 1;
                dev->validPhyVec = 0x1F;
                dev->devName1 = DEV_88E3020;
                break;
        case GT_88E6250:
                dev->numOfPorts = 7;
                dev->maxPorts = 7;
                dev->maxPhyNum = 5;
                dev->validPortVec = (1 << dev->numOfPorts) - 1;
                dev->validPhyVec = 0x1F;
                dev->devName1 = DEV_88E3020;
                break;
        case GT_88E6172:
                dev->numOfPorts = 7;
                dev->validPortVec = (1 << dev->numOfPorts) - 1;
                dev->validPhyVec = 0x1F;
                dev->validSerdesVec = 0x8000;
                dev->devName = DEV_88E6172;
                break;

        case GT_88E6176:
                dev->numOfPorts = 7;
                dev->maxPorts = 7;
                dev->maxPhyNum = 5;
                dev->validPortVec = (1 << dev->numOfPorts) - 1;
                dev->validPhyVec = 0x5F;
                dev->validSerdesVec = 0x8000;
                dev->devName = DEV_88E6176;
                break;

        case GT_88E6240:
                dev->numOfPorts = 7;
                dev->maxPorts = 7;
                dev->maxPhyNum = 5;
                dev->validPortVec = (1 << dev->numOfPorts) - 1;
                dev->validPhyVec = 0x1F;
                dev->validSerdesVec = 0x8000;
                dev->devName = DEV_88E6240;
                break;

        case GT_88E6352:
                dev->numOfPorts = 7;
                dev->maxPorts = 7;
                dev->maxPhyNum = 5;
                dev->validPortVec = (1 << dev->numOfPorts) - 1;
                dev->validPhyVec = 0x1F;
                dev->validSerdesVec = 0x8000;
                dev->devName = DEV_88E6352;
                break;

        case GT_88E6115:
                dev->numOfPorts = 5;
                dev->maxPorts = 5;
                dev->maxPhyNum = 4;
                dev->validPortVec = (1 << dev->numOfPorts) - 1;
                dev->validPhyVec = 0x18;
                dev->validSerdesVec = 0x0003;
                dev->devName1 = DEV_88E6115;
                break;

        case GT_88E6125:
                dev->numOfPorts = 5;
                dev->maxPorts = 5;
                dev->maxPhyNum = 4;
                dev->validPortVec = (1 << dev->numOfPorts) - 1;
                dev->validPhyVec = 0x18;
                dev->validSerdesVec = 0x0003;
                dev->devName1 = DEV_88E6125;
                break;

        case GT_88E6310:
                dev->numOfPorts = 5;
                dev->maxPorts = 5;
                dev->maxPhyNum = 4;
                dev->validPortVec = (1 << dev->numOfPorts) - 1;
                dev->validPhyVec = 0x18;
                dev->validSerdesVec = 0x0003;
                dev->devName1 = DEV_88E6310;
                break;

        case GT_88E6320:
                dev->numOfPorts = 5;
                dev->maxPorts = 5;
                dev->maxPhyNum = 4;
                dev->validPortVec = (1 << dev->numOfPorts) - 1;
                dev->validPhyVec = 0x18;
                dev->validSerdesVec = 0x0003;
                dev->devName1 = DEV_88E6320;
                break;


        default:
                DBG_INFO(("Unknown Device. Initialization failed\n"));
                return GT_FAIL;
    }

    dev->cpuPortNum = GT_PORT_2_LPORT(cfg->cpuPortNum);

    if(dev->cpuPortNum == GT_INVALID_PORT)
    {
        if(GT_LPORT_2_PORT((GT_LPORT)cfg->cpuPortNum) != GT_INVALID_PORT)
        {
            dev->cpuPortNum = cfg->cpuPortNum;
        }
        else
        {
            return GT_BAD_CPU_PORT;
        }
    }

    /* Initialize the MultiAddress Register Access semaphore.    */
    if((dev->multiAddrSem = gtSemCreate(dev,GT_SEM_FULL)) == 0)
    {
        DBG_INFO(("semCreate Failed.\n"));
        qdUnloadDriver(dev);
        return GT_FAIL;
    }

    /* Initialize the ATU semaphore.    */
    if((dev->atuRegsSem = gtSemCreate(dev,GT_SEM_FULL)) == 0)
    {
        DBG_INFO(("semCreate Failed.\n"));
        qdUnloadDriver(dev);
        return GT_FAIL;
    }

    /* Initialize the VTU semaphore.    */
    if((dev->vtuRegsSem = gtSemCreate(dev,GT_SEM_FULL)) == 0)
    {
        DBG_INFO(("semCreate Failed.\n"));
        qdUnloadDriver(dev);
        return GT_FAIL;
    }

    /* Initialize the STATS semaphore.    */
    if((dev->statsRegsSem = gtSemCreate(dev,GT_SEM_FULL)) == 0)
    {
        DBG_INFO(("semCreate Failed.\n"));
        qdUnloadDriver(dev);
        return GT_FAIL;
    }

    /* Initialize the PIRL semaphore.    */
    if((dev->pirlRegsSem = gtSemCreate(dev,GT_SEM_FULL)) == 0)
    {
        DBG_INFO(("semCreate Failed.\n"));
        qdUnloadDriver(dev);
        return GT_FAIL;
    }

    /* Initialize the PTP semaphore.    */
    if((dev->ptpRegsSem = gtSemCreate(dev,GT_SEM_FULL)) == 0)
    {
        DBG_INFO(("semCreate Failed.\n"));
        qdUnloadDriver(dev);
        return GT_FAIL;
    }

    /* Initialize the Table semaphore.    */
    if((dev->tblRegsSem = gtSemCreate(dev,GT_SEM_FULL)) == 0)
    {
        DBG_INFO(("semCreate Failed.\n"));
        qdUnloadDriver(dev);
        return GT_FAIL;
    }

    /* Initialize the EEPROM Configuration semaphore.    */
    if((dev->eepromRegsSem = gtSemCreate(dev,GT_SEM_FULL)) == 0)
    {
        DBG_INFO(("semCreate Failed.\n"));
        qdUnloadDriver(dev);
        return GT_FAIL;
    }

    /* Initialize the PHY Device Register Access semaphore.    */
    if((dev->phyRegsSem = gtSemCreate(dev,GT_SEM_FULL)) == 0)
    {
        DBG_INFO(("semCreate Failed.\n"));
        qdUnloadDriver(dev);
        return GT_FAIL;
    }

    /* Initialize the Remote management Register Access semaphore.    */
    if((dev->hwAccessRegsSem = gtSemCreate(dev,GT_SEM_FULL)) == 0)
    {
        DBG_INFO(("semCreate Failed.\n"));
        qdUnloadDriver(dev);
        return GT_FAIL;
    }

    /* Initialize the ports states to forwarding mode. */
    if(cfg->initPorts == GT_TRUE)
    {
        for (port=0; port<dev->numOfPorts; port++)
        {
            if((retVal = gstpSetPortState(dev,port,GT_PORT_FORWARDING)) != GT_OK)
               {
                DBG_INFO(("Failed.\n"));
                qdUnloadDriver(dev);
                   return retVal;
            }
        }
    }

    dev->use_mad = GT_FALSE;
#ifdef GT_USE_MAD
	{
	  int portPhyAddr=0;
	  unsigned int validPhyVec = dev->validPhyVec;
	  while((validPhyVec&1)==0)
	  {
        validPhyVec >>= 1;
	    portPhyAddr++;
	  }
      DBG_INFO(("@@@@@@@@@@ qd_madInit\n"));
      if((retVal = qd_madInit(dev, portPhyAddr)) != GT_OK)
      {
        DBG_INFO(("Initialize MAD failed.\n"));
        qdUnloadDriver(dev);
        return retVal;
      }
	}
#endif

    if(cfg->skipInitSetup == GT_SKIP_INIT_SETUP)
    {
        dev->devEnabled = 1;
        dev->devNum = cfg->devNum;

        DBG_INFO(("OK.\n"));
        return GT_OK;
    }

    if(IS_IN_DEV_GROUP(dev,DEV_ENHANCED_CPU_PORT))
    {
        if((retVal = gsysSetRsvd2CpuEnables(dev,0)) != GT_OK)
        {
            DBG_INFO(("gsysGetRsvd2CpuEnables failed.\n"));
            qdUnloadDriver(dev);
            return retVal;
        }

        if((retVal = gsysSetRsvd2Cpu(dev,GT_FALSE)) != GT_OK)
        {
            DBG_INFO(("gsysSetRsvd2Cpu failed.\n"));
            qdUnloadDriver(dev);
            return retVal;
        }
    }

    if (IS_IN_DEV_GROUP(dev,DEV_CPU_DEST_PER_PORT))
    {
        for (port=0; port<dev->numOfPorts; port++)
        {
            retVal = gprtSetCPUPort(dev,port,dev->cpuPortNum);
            if(retVal != GT_OK)
            {
                DBG_INFO(("Failed.\n"));
                qdUnloadDriver(dev);
                   return retVal;
            }
        }
    }

    if(IS_IN_DEV_GROUP(dev,DEV_CPU_PORT))
    {
        retVal = gsysSetCPUPort(dev,dev->cpuPortNum);
        if(retVal != GT_OK)
           {
            DBG_INFO(("Failed.\n"));
            qdUnloadDriver(dev);
               return retVal;
        }
    }

    if(IS_IN_DEV_GROUP(dev,DEV_CPU_DEST))
    {
        retVal = gsysSetCPUDest(dev,dev->cpuPortNum);
        if(retVal != GT_OK)
           {
            DBG_INFO(("Failed.\n"));
            qdUnloadDriver(dev);
               return retVal;
        }
    }

    if(IS_IN_DEV_GROUP(dev,DEV_MULTICAST))
    {
        if((retVal = gsysSetRsvd2Cpu(dev,GT_FALSE)) != GT_OK)
        {
            DBG_INFO(("gsysSetRsvd2Cpu failed.\n"));
            qdUnloadDriver(dev);
            return retVal;
        }
    }

    if (IS_IN_DEV_GROUP(dev,DEV_PIRL_RESOURCE))
    {
        retVal = gpirlInitialize(dev);
        if(retVal != GT_OK)
           {
            DBG_INFO(("Failed.\n"));
            qdUnloadDriver(dev);
               return retVal;
        }
    }

    if (IS_IN_DEV_GROUP(dev,DEV_PIRL2_RESOURCE))
    {
        retVal = gpirl2Initialize(dev);
        if(retVal != GT_OK)
           {
            DBG_INFO(("Failed.\n"));
            qdUnloadDriver(dev);
               return retVal;
        }
    }

    dev->devEnabled = 1;
    dev->devNum = cfg->devNum;

    DBG_INFO(("OK.\n"));
    return GT_OK;
}

/*******************************************************************************
* sysEnable
*
* DESCRIPTION:
*       This function enables the system for full operation.
*
* INPUTS:
*       None.
*
* OUTPUTS:
*       None.
*
* RETURNS:
*       GT_OK           - on success
*       GT_FAIL         - on error
*
* COMMENTS:
*       1.  This function should be called only after successful execution of
*           qdLoadDriver().
*
*******************************************************************************/
GT_STATUS sysEnable( GT_QD_DEV *dev)
{
    DBG_INFO(("sysEnable Called.\n"));
    DBG_INFO(("OK.\n"));
    return driverEnable(dev);
}


/*******************************************************************************
* qdUnloadDriver
*
* DESCRIPTION:
*       This function unloads the QuaterDeck Driver.
*
* INPUTS:
*       None.
*
* OUTPUTS:
*       None.
*
* RETURNS:
*       GT_OK           - on success
*       GT_FAIL         - on error
*
* COMMENTS:
*       1.  This function should be called only after successful execution of
*           qdLoadDriver().
*
*******************************************************************************/
GT_STATUS qdUnloadDriver
(
    IN GT_QD_DEV* dev
)
{
    DBG_INFO(("qdUnloadDriver Called.\n"));

    /* Delete the MultiAddress mode reagister access semaphore.    */
    if(gtSemDelete(dev,dev->multiAddrSem) != GT_OK)
    {
        DBG_INFO(("Failed.\n"));
        return GT_FAIL;
    }

    /* Delete the ATU semaphore.    */
    if(gtSemDelete(dev,dev->atuRegsSem) != GT_OK)
    {
        DBG_INFO(("Failed.\n"));
        return GT_FAIL;
    }

    /* Delete the VTU semaphore.    */
    if(gtSemDelete(dev,dev->vtuRegsSem) != GT_OK)
    {
        DBG_INFO(("Failed.\n"));
        return GT_FAIL;
    }

    /* Delete the STATS semaphore.    */
    if(gtSemDelete(dev,dev->statsRegsSem) != GT_OK)
    {
        DBG_INFO(("Failed.\n"));
        return GT_FAIL;
    }

    /* Delete the PIRL semaphore.    */
    if(gtSemDelete(dev,dev->pirlRegsSem) != GT_OK)
    {
        DBG_INFO(("Failed.\n"));
        return GT_FAIL;
    }

    /* Delete the PTP semaphore.    */
    if(gtSemDelete(dev,dev->ptpRegsSem) != GT_OK)
    {
        DBG_INFO(("Failed.\n"));
        return GT_FAIL;
    }

    /* Delete the Table semaphore.    */
    if(gtSemDelete(dev,dev->tblRegsSem) != GT_OK)
    {
        DBG_INFO(("Failed.\n"));
        return GT_FAIL;
    }

    /* Delete the EEPROM Configuration semaphore.    */
    if(gtSemDelete(dev,dev->eepromRegsSem) != GT_OK)
    {
        DBG_INFO(("Failed.\n"));
        return GT_FAIL;
    }

    /* Delete the PHY Device semaphore.    */
    if(gtSemDelete(dev,dev->phyRegsSem) != GT_OK)
    {
        DBG_INFO(("Failed.\n"));
        return GT_FAIL;
    }
    /* Delete the Remote management Register Access semaphore.    */
    if(gtSemDelete(dev,dev->hwAccessRegsSem) != GT_OK)
    {
        DBG_INFO(("Failed.\n"));
        return GT_FAIL;
    }

    gtMemSet(dev,0,sizeof(GT_QD_DEV));
    return GT_OK;
}


/*******************************************************************************
* gtRegister
*
* DESCRIPTION:
*       BSP should register the following functions:
*        1) MII Read - (Input, must provide)
*            allows QuarterDeck driver to read QuarterDeck device registers.
*        2) MII Write - (Input, must provice)
*            allows QuarterDeck driver to write QuarterDeck device registers.
*        3) Semaphore Create - (Input, optional)
*            OS specific Semaphore Creat function.
*        4) Semaphore Delete - (Input, optional)
*            OS specific Semaphore Delete function.
*        5) Semaphore Take - (Input, optional)
*            OS specific Semaphore Take function.
*        6) Semaphore Give - (Input, optional)
*            OS specific Semaphore Give function.
*        Notes: 3) ~ 6) should be provided all or should not be provided at all.
*
* INPUTS:
*        pBSPFunctions - pointer to the structure for above functions.
*
* OUTPUTS:
*        None.
*
* RETURNS:
*       GT_TRUE, if input is valid. GT_FALSE, otherwise.
*
* COMMENTS:
*       This function should be called only once.
*
*******************************************************************************/
static GT_BOOL gtRegister(GT_QD_DEV *dev, BSP_FUNCTIONS* pBSPFunctions)
{
    dev->fgtReadMii =  pBSPFunctions->readMii;
    dev->fgtWriteMii = pBSPFunctions->writeMii;
#ifdef GT_RMGMT_ACCESS
    dev->fgtHwAccessMod =  pBSPFunctions->hwAccessMod;
    dev->fgtHwAccess = pBSPFunctions->hwAccess;
#endif

    dev->semCreate = pBSPFunctions->semCreate;
    dev->semDelete = pBSPFunctions->semDelete;
    dev->semTake   = pBSPFunctions->semTake  ;
    dev->semGive   = pBSPFunctions->semGive  ;

    return GT_TRUE;
}

static GT_U8 qd32_2_8[256] = {
0,1,2,3,4,5,6,7,8,9,
10,11,12,13,14,15,16,17,18,19,
20,21,22,23,24,25,26,27,28,29,
30,31,32,33,34,35,36,37,38,39,
40,41,42,43,44,45,46,47,48,49,
50,51,52,53,54,55,56,57,58,59,
60,61,62,63,64,65,66,67,68,69,
70,71,72,73,74,75,76,77,78,79,
80,81,82,83,84,85,86,87,88,89,
90,91,92,93,94,95,96,97,98,99,
100,101,102,103,104,105,106,107,108,109,
110,111,112,113,114,115,116,117,118,119,
120,121,122,123,124,125,126,127,128,129,
130,131,132,133,134,135,136,137,138,139,
140,141,142,143,144,145,146,147,148,149,
150,151,152,153,154,155,156,157,158,159,
160,161,162,163,164,165,166,167,168,169,
170,171,172,173,174,175,176,177,178,179,
180,181,182,183,184,185,186,187,188,189,
190,191,192,193,194,195,196,197,198,199,
200,201,202,203,204,205,206,207,208,209,
210,211,212,213,214,215,216,217,218,219,
220,221,222,223,224,225,226,227,228,229,
230,231,232,233,234,235,236,237,238,239,
240,241,242,243,244,245,246,247,248,249,
250,251,252,253,254,255};



GT_U8 qdLong2Char(GT_U32 data)
{
    return qd32_2_8[data&0xff];
}

GT_U8 qdShort2Char(GT_U16 data)
{
    GT_U32 dataL = data;
    return qd32_2_8[dataL&0xff];
}

GT_U16 qdLong2Short(GT_U32 data)
{
  GT_U32 data1= 1;
  if( *((GT_U16 *)&data1) )
    return *((GT_U16 *)&data);
  else
    return *((GT_U16 *)&data + 1);
}
