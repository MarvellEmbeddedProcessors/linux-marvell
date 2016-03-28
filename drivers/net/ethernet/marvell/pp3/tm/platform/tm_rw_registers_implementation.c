/*******************************************************************************
Copyright (C) Marvell International Ltd. and its affiliates

This software file (the "File") is owned and distributed by Marvell
International Ltd. and/or its affiliates ("Marvell") under the following
alternative licensing terms.  Once you have made an election to distribute the
File under one of the following license alternatives, please (i) delete this
introductory statement regarding license alternatives, (ii) delete the two
license alternatives that you have not elected to use and (iii) preserve the
Marvell copyright notice above.


********************************************************************************
Marvell GPL License Option

If you received this File from Marvell, you may opt to use, redistribute and/or
modify this File in accordance with the terms and conditions of the General
Public License Version 2, June 1991 (the "GPL License"), a copy of which is
available along with the File in the license.txt file or by writing to the Free
Software Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 or
on the worldwide web at http://www.gnu.org/licenses/gpl.txt.

THE FILE IS DISTRIBUTED AS-IS, WITHOUT WARRANTY OF ANY KIND, AND THE IMPLIED
WARRANTIES OF MERCHANTABILITY OR FITNESS FOR A PARTICULAR PURPOSE ARE EXPRESSLY
DISCLAIMED.  The GPL License provides additional details about this warranty
disclaimer.
*******************************************************************************/

#include "common/mv_sw_if.h"
#include "common/mv_hw_if.h"

#include "tm_alias.h"
#include "tm_rw_registers_interface.h"
#include "tm_core_types.h"


#define QMTM_REGISTER_SIZE	2 /* All register size is 8 Bytes - 64 bits */

#define MV_PP3_HW_READ	mv_pp3_hw_read
#define MV_PP3_HW_WRITE	mv_pp3_hw_write
/*
#define MV_PP3_HW_READ(access_addr, words_num, data_ptr) 0
#define MV_PP3_HW_WRITE(access_addr, words_num, data_ptr) 0
*/
static void tm_regs_get_address_string(u32 address, int index, char *regName);
static void tm_regs_get_offset(u32 address, int index, u32 *offset);


/**
 */
int set_hw_connection(void *handle)
{
	int rc = 0;
	return rc;
}


/**
 */
int flush_hw_connection(void *handle)
{
	int rc = 0;
	return rc;
}


/**
 */
int reset_hw_connection(void *handle, int error)
{
	int rc = 0;
	return rc;
}


/**
 */
int close_hw_connection(void *handle)
{
	int rc = 0;
	return rc;
}


/**
 */
int tm_table_entry_read(void *handle,
				void *vpAddress,
				long int index,
				void *vpData)
{
	u32 tbl_addr = *(int *)vpAddress;
	u32 offset = 0;
	u32 *dataPtr = (u32 *)vpData;
	int rc = 0;

	tm_regs_get_offset(tbl_addr, index, &offset);
	MV_PP3_HW_READ(TM.silicon_base+tbl_addr+offset, QMTM_REGISTER_SIZE, dataPtr);
	if (tm_debug_on == 1) {
		char reg_name[100];
		tm_regs_get_address_string(tbl_addr, index, reg_name);
		pr_info("R  %-32s: 0x%x + 0x%x = 0x%08x\n", reg_name, tbl_addr, offset, *(unsigned int *)vpData);
	}

	return rc;
}


/**
 */
int tm_table_entry_write(void *handle,
				void *vpAddress,
				long int index,
				void *vpData)
{
	u32 tbl_addr = *(int *)vpAddress;
	u32 offset = 0;
	u32 *dataPtr = (u32 *)vpData;
	int rc = 0;
	/* u32 readData; */

	tm_regs_get_offset(tbl_addr, index, &offset);

	if (tm_debug_on == 1) { /* TBD: DefZero */
/*	MV_PP3_HW_READ(TM.silicon_base+tbl_addr+offset, QMTM_REGISTER_SIZE, &readData);
	if (readData != *dataPtr)
		pr_info("!!!  %-32s: 0x%x + 0x%x = 0x%08x\n", reg_name, tbl_addr, offset, (unsigned int)readData);
		*/
	}

	MV_PP3_HW_WRITE(TM.silicon_base+tbl_addr+offset, QMTM_REGISTER_SIZE, dataPtr);
	if (tm_debug_on == 1) {
		char reg_name[100];
		tm_regs_get_address_string(*(int *)vpAddress, index, reg_name);
		pr_info("W  %-32s: 0x%x + 0x%x = 0x%08x\n", reg_name, tbl_addr, offset, *(unsigned int *)vpData);
	}

	return rc;
}


/**
 */
int tm_register_write(void *handle, void *vpAddress, void *vpData)
{
	return tm_table_entry_write(handle, vpAddress, 0, vpData);
}


int tm_register_read(void *handle, void *vpAddress, void *vpData)
{
	return tm_table_entry_read(handle, vpAddress, 0, vpData);

}


static void tm_regs_get_address_string(u32 address, int index, char *regName)
{
	int i;

	sprintf(regName, "Unknown");
	/* Registers */
	if (address == TM.Drop.ErrStus)
		sprintf(regName, "TM.Drop.ErrStus                ");
	else if (address == TM.Drop.FirstExc)
		sprintf(regName, "TM.Drop.FirstExc               ");
	else if (address == TM.Drop.ErrCnt)
		sprintf(regName, "TM.Drop.ErrCnt                 ");
	else if (address == TM.Drop.ExcCnt)
		sprintf(regName, "TM.Drop.ExcCnt                 ");
	else if (address == TM.Drop.ExcMask)
		sprintf(regName, "TM.Drop.ExcMask                ");
	else if (address == TM.Drop.Id)
		sprintf(regName, "TM.Drop.Id                     ");
	else if (address == TM.Drop.ForceErr)
		sprintf(regName, "TM.Drop.ForceErr               ");
	else if (address == TM.Drop.WREDDropProbMode)
		sprintf(regName, "TM.Drop.WREDDropProbMode       ");
	else if (address == TM.Drop.WREDMaxProbModePerColor)
		sprintf(regName, "TM.Drop.WREDMaxProbModePerColor");
	else if (address == TM.Drop.DPSource)
		sprintf(regName, "TM.Drop.DPSource               ");
	else if (address == TM.Drop.Drp_Decision_hierarchy_to_Query_debug)
		sprintf(regName, "TM.Drop.Drp_Decision_hierarchy_to_Query_debug  ");
	else if (address == TM.Drop.Drp_Decision_to_Query_debug)
		sprintf(regName, "TM.Drop.Drp_Decision_to_Query_debug  ");
	else if (address == TM.Drop.EccConfig)
		sprintf(regName, "TM.Drop.EccConfig              ");
	else if (address == TM.Drop.RespLocalDPSel)
		sprintf(regName, "TM.Drop.RespLocalDPSel         ");

	/* Ports */
	for (i = 0; i < TM_WRED_COS; i++) {
		if (address == TM.Drop.PortDropPrfWREDParams_CoSRes[i])
			sprintf(regName, "TM.Drop.PortDropPrfWREDParams_CoSRes0%d     port%d", i, index);
		else if (address == TM.Drop.PortDropPrfWREDScaleRatio_CoSRes[i])
			sprintf(regName, "TM.Drop.PortDropPrfWREDScaleRatio_CoSRes0%d port%d", i, index);
		else if (address == TM.Drop.PortDropPrfWREDMinThresh_CoSRes[i])
			sprintf(regName, "TM.Drop.PortDropPrfWREDMinThresh_CoSRes0%d  port%d", i, index);
		else if (address == TM.Drop.PortDropPrfWREDDPRatio_CoSRes[i])
			sprintf(regName, "TM.Drop.PortDropPrfWREDDPRatio_CoSRes0%d    port%d", i, index);
		else if (address == TM.Drop.PortDropPrfTailDrpThresh_CoSRes[i])
			sprintf(regName, "TM.Drop.PortDropPrfTailDrpThresh_CoSRes0%d  port%d", i, index);
		else if (address == TM.Drop.PortREDCurve_CoS[i])
			sprintf(regName, "TM.Drop.PortREDCurve_CoS_%d %d                    ", i, index);
	}

	if (address == TM.Drop.PortDropPrfWREDParams)
		sprintf(regName, "TM.Drop.PortDropPrfWREDParams       port%d", index);
	else if (address == TM.Drop.PortDropPrfWREDScaleRatio)
		sprintf(regName, "TM.Drop.PortDropPrfWREDScaleRatio   port%d", index);
	else if (address == TM.Drop.PortDropPrfWREDMinThresh)
		sprintf(regName, "TM.Drop.PortDropPrfWREDMinThresh    port%d", index);
	else if (address == TM.Drop.PortDropPrfWREDDPRatio)
		sprintf(regName, "TM.Drop.PortDropPrfWREDDPRatio      port%d", index);
	else if (address == TM.Drop.PortDropPrfTailDrpThresh)
		sprintf(regName, "TM.Drop.PortDropPrfTailDrpThresh    port%d", index);
	else if (address == TM.Drop.PortREDCurve)
		sprintf(regName, "TM.Drop.PortREDCurve[%d]", index);

	/* C Level */
	for (i = 0; i < TM_WRED_COS; i++) {
		if (address == TM.Drop.ClvlDropProfPtr_CoS[i])
			/* Special case, more than one index in entry */
			sprintf(regName, "TM.Drop.ClvlDropProfPtr.CoS[%d]      C%d", i, index);
		else if (address == TM.Drop.ClvlDropPrfWREDParams.CoS[i])
			sprintf(regName, "TM.Drop.ClvlDropPrfWREDParams.CoS[%d]     profile%d", i, index);
		else if (address == TM.Drop.ClvlDropPrfWREDScaleRatio.CoS[i])
			sprintf(regName, "TM.Drop.ClvlDropPrfWREDScaleRatio.CoS[%d] profile%d", i, index);
		else if (address == TM.Drop.ClvlDropPrfWREDMinThresh.CoS[i])
			sprintf(regName, "TM.Drop.ClvlDropPrfWREDMinThresh.CoS[%d]  profile%d", i, index);
		else if (address == TM.Drop.ClvlDropPrfWREDDPRatio.CoS[i])
			sprintf(regName, "TM.Drop.ClvlDropPrfWREDDPRatio.CoS[%d]    profile%d", i, index);
		else if (address == TM.Drop.ClvlDropPrfTailDrpThresh.CoS[i])
			sprintf(regName, "TM.Drop.ClvlDropPrfTailDrpThresh.CoS[%d   profile%d]", i, index);
		else if (address == TM.Drop.ClvlREDCurve.CoS[i])
			sprintf(regName, "TM.Drop.ClvlREDCurve.CoS[%d][%d]", i, index); /* 128/32 = 4 curves */
	}

	/* B level */
	if (address == TM.Drop.BlvlDropProfPtr)
		/* Special case, more than one index in entry */
		sprintf(regName, "TM.Drop.BlvlDropProfPtr             B%d", index);
	else if (address == TM.Drop.BlvlDropPrfWREDParams)
		sprintf(regName, "TM.Drop.BlvlDropPrfWREDParams     profile%d", index); /* 8 Profiles */
	else if (address == TM.Drop.BlvlDropPrfWREDScaleRatio)
		sprintf(regName, "TM.Drop.BlvlDropPrfWREDScaleRatio profile%d", index); /* 8 Profiles */
	else if (address == TM.Drop.BlvlDropPrfWREDMinThresh)
		sprintf(regName, "TM.Drop.BlvlDropPrfWREDMinThresh  profile%d", index); /* 8 Profiles */
	else if (address == TM.Drop.BlvlDropPrfWREDDPRatio)
		sprintf(regName, "TM.Drop.BlvlDropPrfWREDDPRatio    profile%d", index); /* 8 Profiles */
	else if (address == TM.Drop.BlvlDropPrfTailDrpThresh)
		sprintf(regName, "TM.Drop.BlvlDropPrfTailDrpThresh  profile%d", index); /* 8 Profiles */
	for (i = 0; i < 3; i++) {
		if (address == TM.Drop.BlvlREDCurve[i].Table)
			sprintf(regName, "TM.Drop.BlvlREDCurve[%d][%d]", i, index); /* 32/32 = 1 curves */
	}

	/* A Level */
	if (address == TM.Drop.AlvlDropProb)
		sprintf(regName, "TM.Drop.AlvlDropProb              A%d", index);
	else if (address == TM.Drop.AlvlDropPrfWREDParams)
		sprintf(regName, "TM.Drop.AlvlDropPrfWREDParams     profile%d", index);/* 8 Profiles */
	else if (address == TM.Drop.AlvlDropPrfWREDScaleRatio)
		sprintf(regName, "TM.Drop.AlvlDropPrfWREDScaleRatio profile%d", index);/* 8 Profiles */
	else if (address == TM.Drop.AlvlDropPrfWREDMinThresh)
		sprintf(regName, "TM.Drop.AlvlDropPrfWREDMinThresh  profile%d", index);/* 8 Profiles */
	else if (address == TM.Drop.AlvlDropPrfWREDDPRatio)
		sprintf(regName, "TM.Drop.AlvlDropPrfWREDDPRatio    profile%d", index);/* 8 Profiles */
	else if (address == TM.Drop.AlvlDropPrfTailDrpThresh)
		sprintf(regName, "TM.Drop.AlvlDropPrfTailDrpThresh  profile%d", index);/* 8 Profiles */
	for (i = 0; i < 3; i++) {
		if (address == TM.Drop.AlvlREDCurve.Color[i])
			sprintf(regName, "TM.Drop.AlvlREDCurve.Color[%d][%d]", i, index);/* 256/32 = 8 Curves */
	}

	/* Queue Level */
	if (address == TM.Drop.QueueDropProfPtr)
		/* Special case, more than one index in entry */
		sprintf(regName, "TM.Drop.QueueDropProfPtr            Q%d", index);
	else if (address == TM.Drop.QueueDropPrfWREDParams)
		sprintf(regName, "TM.Drop.QueueDropPrfWREDParams     profile%d", index);/* 16 Profiles */
	else if (address == TM.Drop.QueueDropPrfWREDScaleRatio)
		sprintf(regName, "TM.Drop.QueueDropPrfWREDScaleRatio profile%d", index);/* 16 Profiles */
	else if (address == TM.Drop.QueueDropPrfWREDMinThresh)
		sprintf(regName, "TM.Drop.QueueDropPrfWREDMinThresh  profile%d", index);/* 16 Profiles */
	else if (address == TM.Drop.QueueDropPrfWREDDPRatio)
		sprintf(regName, "TM.Drop.QueueDropPrfWREDDPRatio    profile%d", index);/* 16 Profiles */
	else if (address == TM.Drop.QueueDropPrfTailDrpThresh)
		sprintf(regName, "TM.Drop.QueueDropPrfTailDrpThresh  profile%d", index);/* 16 Profiles */

	for (i = 0; i < 3; i++) {
		if (address == TM.Drop.QueueREDCurve.Color[i])
			sprintf(regName, "TM.Drop.QueueREDCurve.Color[%d][%d]", i, index);/* 256/32 = 8 Curves */
	}
	if (address == TM.Drop.QueueCoSConf)
		/* 128 * 4CoS = 512Nodes */
		sprintf(regName, "TM.Drop.QueueCoSConf               Q%d...Q%d", index*4, index*4+3);

	/* Port Level */
	else if (address == TM.Drop.PortInstAndAvgQueueLength)
		sprintf(regName, "TM.Drop.PortInstAndAvgQueueLength  port%d", index); /*16 ports*/
	else if (address == TM.Drop.PortDropProb)
		sprintf(regName, "TM.Drop.PortDropProb               port%d", index);/* 16 ports*/

	for (i = 0; i < 8; i++) {
		if (address == TM.Drop.PortInstAndAvgQueueLengthPerCoS.CoS[i])
			/* 8 CoS 16 Ports */
			sprintf(regName, "TM.Drop.PortInstAndAvgQueueLengthPerCoS.CoS0%d port%d", i, index);
		else if (address == TM.Drop.PortDropProbPerCoS_CoS[i])
			/* 8 CoS 16 Ports */
			sprintf(regName, "TM.Drop.PortDropProbPerCoS.CoS0%d              port%d", i, index);
	}
	/* C Level */
	if (address == TM.Drop.ClvlInstAndAvgQueueLength)
		sprintf(regName, "TM.Drop.ClvlInstAndAvgQueueLength C%d", index);/* 16 C */
	else if (address == TM.Drop.ClvlDropProb)
		sprintf(regName, "TM.Drop.ClvlDropProb"); /* 128 = 8CoS * 16C */
	/* B level */
	else if (address == TM.Drop.BlvlInstAndAvgQueueLength)
		sprintf(regName, "TM.Drop.BlvlInstAndAvgQueueLength B%d", index); /* 32 B Nodes */
	else if (address == TM.Drop.BlvlDropProb)
		sprintf(regName, "TM.Drop.BlvlDropProb              B%d", index); /* 32 B Nodes */
	/* A Level */
	else if (address == TM.Drop.AlvlDropProfPtr)
		/* Special case, more than one index in entry */
		sprintf(regName, "TM.Drop.AlvlDropProfPtr             A%d", index);
	else if (address == TM.Drop.AlvlInstAndAvgQueueLength)
		sprintf(regName, "TM.Drop.AlvlInstAndAvgQueueLength A%d", index);/* 128 A Nodes */
	/* Queue Level */
	else if (address == TM.Drop.QueueAvgQueueLength)
		sprintf(regName, "TM.Drop.QueueAvgQueueLength       Q%d", index); /* 512 Queues */
	else if (address == TM.Drop.QueueDropProb)
		sprintf(regName, "TM.Drop.QueueDropProb             Q%d", index);	/* 512 Queues */

	/* Registers*/
	else if (address == TM.Sched.ScrubDisable)
		sprintf(regName, "TM.Sched.ScrubDisable");
	else if (address == TM.Sched.ScrubSlotAlloc)
		sprintf(regName, "TM.Sched.ScrubSlotAlloc");
	else if (address == TM.Sched.EccConfig)
		sprintf(regName, "TM.Sched.EccConfig");
	else if (address == TM.Sched.ExcMask)
		sprintf(regName, "TM.Sched.ExcMask");
	else if (address == TM.Sched.TreeDeqEn)
		sprintf(regName, "TM.Sched.TreeDeqEn");
	else if (address == TM.Sched.TreeDWRRPrioEn)
		sprintf(regName, "TM.Sched.TreeDWRRPrioEn");
	else if (address == TM.Sched.PortPerConf)
		sprintf(regName, "TM.Sched.PortPerConf");
	else if (address == TM.Sched.PortPerRateShpPrms)
		sprintf(regName, "TM.Sched.PortPerRateShpPrms");
	else if (address == TM.Sched.PortPerRateShpPrmsInt)
		sprintf(regName, "TM.Sched.PortPerRateShpPrmsInt");
	else if (address == TM.Sched.PortExtBPEn)
		sprintf(regName, "TM.Sched.PortExtBPEn");
	else if (address == TM.Sched.PortDWRRBytesPerBurstsLimit)
		sprintf(regName, "TM.Sched.PortDWRRBytesPerBurstsLimit");
	else if (address == TM.Sched.ClvlPerConf)
		sprintf(regName, "TM.Sched.ClvlPerConf");
	else if (address == TM.Sched.ClvlPerRateShpPrms)
		sprintf(regName, "TM.Sched.ClvlPerRateShpPrms");
	else if (address == TM.Sched.ClvlPerRateShpPrmsInt)
		sprintf(regName, "TM.Sched.ClvlPerRateShpPrmsInt");
	else if (address == TM.Sched.BlvlPerConf)
		sprintf(regName, "TM.Sched.BlvlPerConf");
	else if (address == TM.Sched.BlvlPerRateShpPrms)
		sprintf(regName, "TM.Sched.BlvlPerRateShpPrms");
	else if (address == TM.Sched.BlvlPerRateShpPrmsInt)
		sprintf(regName, "TM.Sched.BlvlPerRateShpPrmsInt");
	else if (address == TM.Sched.AlvlPerConf)
		sprintf(regName, "TM.Sched.AlvlPerConf");
	else if (address == TM.Sched.AlvlPerRateShpPrms)
		sprintf(regName, "TM.Sched.AlvlPerRateShpPrms");
	else if (address == TM.Sched.AlvlPerRateShpPrmsInt)
		sprintf(regName, "TM.Sched.AlvlPerRateShpPrmsInt");
	else if (address == TM.Sched.QueuePerConf)
		sprintf(regName, "TM.Sched.QueuePerConf");
	else if (address == TM.Sched.QueuePerRateShpPrms)
		sprintf(regName, "TM.Sched.QueuePerRateShpPrms");
	else if (address == TM.Sched.QueuePerRateShpPrmsInt)
		sprintf(regName, "TM.Sched.QueuePerRateShpPrmsInt");
	/* tables*/
	else if (address == TM.Sched.PortEligPrioFuncPtr)
		sprintf(regName, "TM.Sched.PortEligPrioFuncPtr        port%d", index);
	else if (address == TM.Sched.PortTokenBucketTokenEnDiv)
		sprintf(regName, "TM.Sched.PortTokenBucketTokenEnDiv  port%d", index);
	else if (address == TM.Sched.PortTokenBucketBurstSize)
		sprintf(regName, "TM.Sched.PortTokenBucketBurstSize   port%d", index);
	else if (address == TM.Sched.PortDWRRPrioEn)
		sprintf(regName, "TM.Sched.PortDWRRPrioEn             port%d", index);
	else if (address == TM.Sched.PortQuantumsPriosLo)
		sprintf(regName, "TM.Sched.PortQuantumsPriosLo        port%d", index);
	else if (address == TM.Sched.PortQuantumsPriosHi)
		sprintf(regName, "TM.Sched.PortQuantumsPriosHi        port%d", index);
	else if (address == TM.Sched.PortRangeMap)
		sprintf(regName, "TM.Sched.PortRangeMap               port%d", index);
	else if (address == TM.Sched.PortEligPrioFunc)
		sprintf(regName, "TM.Sched.PortEligPrioFunc           port%d", index);

	if (address == TM.Sched.ClvlEligPrioFunc)
		sprintf(regName, "TM.Sched.ClvlEligPrioFunc           C%d", index);
	else if (address == TM.Sched.ClvlEligPrioFuncPtr)
		sprintf(regName, "TM.Sched.ClvlEligPrioFuncPtr        C%d", index);
	else if (address == TM.Sched.ClvlTokenBucketTokenEnDiv)
		sprintf(regName, "TM.Sched.ClvlTokenBucketTokenEnDiv  C%d", index);
	else if (address == TM.Sched.ClvlTokenBucketBurstSize)
		sprintf(regName, "TM.Sched.ClvlTokenBucketBurstSize   C%d", index);
	else if (address == TM.Sched.ClvlDWRRPrioEn)
		sprintf(regName, "TM.Sched.ClvlDWRRPrioEn             C%d", index);
	else if (address == TM.Sched.ClvlQuantum)
		sprintf(regName, "TM.Sched.ClvlQuantum                C%d", index);
	else if (address == TM.Sched.ClvltoPortAndBlvlRangeMap)
		sprintf(regName, "TM.Sched.ClvltoPortAndBlvlRangeMap  C%d", index);

	if (address == TM.Sched.BlvlEligPrioFunc)
		sprintf(regName, "TM.Sched.BlvlEligPrioFunc           B%d", index);
	else if (address == TM.Sched.BlvlEligPrioFuncPtr)
		sprintf(regName, "TM.Sched.BlvlEligPrioFuncPtr        B%d", index);
	else if (address == TM.Sched.BlvlTokenBucketTokenEnDiv)
		sprintf(regName, "TM.Sched.BlvlTokenBucketTokenEnDiv  B%d", index);
	else if (address == TM.Sched.BlvlTokenBucketBurstSize)
		sprintf(regName, "TM.Sched.BlvlTokenBucketBurstSize   B%d", index);
	else if (address == TM.Sched.BlvlDWRRPrioEn)
		sprintf(regName, "TM.Sched.BlvlDWRRPrioEn             B%d", index);
	else if (address == TM.Sched.BlvlQuantum)
		sprintf(regName, "TM.Sched.BlvlQuantum                B%d", index);
	else if (address == TM.Sched.BLvltoClvlAndAlvlRangeMap)
		sprintf(regName, "TM.Sched.BLvltoClvlAndAlvlRangeMap  B%d", index);

	if (address == TM.Sched.AlvlEligPrioFunc)
		sprintf(regName, "TM.Sched.AlvlEligPrioFunc           A%d", index);
	else if (address == TM.Sched.AlvlEligPrioFuncPtr)
		sprintf(regName, "TM.Sched.AlvlEligPrioFuncPtr        A%d", index);
	else if (address == TM.Sched.AlvlTokenBucketTokenEnDiv)
		sprintf(regName, "TM.Sched.AlvlTokenBucketTokenEnDiv  A%d", index);
	else if (address == TM.Sched.AlvlTokenBucketBurstSize)
		sprintf(regName, "TM.Sched.AlvlTokenBucketBurstSize   A%d", index);
	else if (address == TM.Sched.AlvlDWRRPrioEn)
		sprintf(regName, "TM.Sched.AlvlDWRRPrioEn             A%d", index);
	else if (address == TM.Sched.AlvlQuantum)
		sprintf(regName, "TM.Sched.AlvlQuantum                A%d", index);
	else if (address == TM.Sched.ALvltoBlvlAndQueueRangeMap)
		sprintf(regName, "TM.Sched.ALvltoBlvlAndQueueRangeMap A%d", index);

	else if (address == TM.Sched.QueueEligPrioFunc)
		sprintf(regName, "TM.Sched.QueueEligPrioFunc          Q%d", index);
	else if (address == TM.Sched.QueueEligPrioFuncPtr)
		sprintf(regName, "TM.Sched.QueueEligPrioFuncPtr       Q%d", index);
	else if (address == TM.Sched.QueueTokenBucketTokenEnDiv)
		sprintf(regName, "TM.Sched.QueueTokenBucketTokenEnDiv Q%d", index);
	else if (address == TM.Sched.QueueTokenBucketBurstSize)
		sprintf(regName, "TM.Sched.QueueTokenBucketBurstSize  Q%d", index);
	else if (address == TM.Sched.QueueQuantum)
		sprintf(regName, "TM.Sched.QueueQuantum               Q%d", index);
	else if (address == TM.Sched.QueueAMap)
		sprintf(regName, "TM.Sched.QueueAMap                  Q%d", index);

	else if (address == TM.Sched.PortShpBucketLvls)
		sprintf(regName, "TM.Sched.PortShpBucketLvls          Port%d", index);
	else if (address == TM.Sched.PortDefPrioHi)
		sprintf(regName, "TM.Sched.PortDefPrioHi              Port%d", index);
	else if (address == TM.Sched.PortDefPrioLo)
		sprintf(regName, "TM.Sched.PortDefPrioLo              Port%d", index);

	else if (address == TM.Sched.ClvlShpBucketLvls)
		sprintf(regName, "TM.Sched.ClvlShpBucketLvls          C%d", index);
	else if (address == TM.Sched.CLvlDef)
		sprintf(regName, "TM.Sched.CLvlDef                    C%d", index);

	else if (address == TM.Sched.BlvlShpBucketLvls)
		sprintf(regName, "TM.Sched.BlvlShpBucketLvls          B%d", index);
	else if (address == TM.Sched.BlvlDef)
		sprintf(regName, "TM.Sched.BlvlDef                    B%d", index);

	else if (address == TM.Sched.AlvlShpBucketLvls)
		sprintf(regName, "TM.Sched.AlvlShpBucketLvls          A%d", index);
	else if (address == TM.Sched.AlvlDef)
		sprintf(regName, "TM.Sched.AlvlDef                    A%d", index);

	else if (address == TM.Sched.QueueShpBucketLvls)
		sprintf(regName, "TM.Sched.QueueShpBucketLvls         Q%d", index);
	else if (address == TM.Sched.QueueDef)
		sprintf(regName, "TM.Sched.QueueDef                   Q%d", index);

	/*end of unit Sched*/
}


static void tm_regs_get_offset(u32 address, int index, u32 *offset)
{
	int i;
	*offset = 0;

	/* Queue level */
	for (i = 0; i < 3; i++) {
		if (address == TM.Drop.QueueREDCurve.Color[i])
			*offset = tm_index_offset.Drop.QueueREDCurve.Color[i] * index;
	}
	if (address == TM.Drop.QueueDropPrfWREDParams)
		*offset = tm_index_offset.Drop.QueueDropPrfWREDParams * index;
	else if (address == TM.Drop.QueueDropPrfWREDScaleRatio)
		*offset = tm_index_offset.Drop.QueueDropPrfWREDScaleRatio * index;
	else if (address == TM.Drop.QueueDropPrfWREDMinThresh)
		*offset = tm_index_offset.Drop.QueueDropPrfWREDMinThresh * index;
	else if (address == TM.Drop.QueueDropPrfTailDrpThresh)
		*offset = tm_index_offset.Drop.QueueDropPrfTailDrpThresh * index;
	else if (address == TM.Drop.QueueDropPrfWREDDPRatio)
		*offset = tm_index_offset.Drop.QueueDropPrfWREDDPRatio * index;

	/* A level */
	for (i = 0; i < 3; i++) {
		if (address == TM.Drop.AlvlREDCurve.Color[i])
			*offset = tm_index_offset.Drop.AlvlREDCurve.Color[i] * index;
	}
	if (address == TM.Drop.AlvlDropPrfWREDParams)
		*offset = tm_index_offset.Drop.AlvlDropPrfWREDParams * index;
	else if (address == TM.Drop.AlvlDropPrfWREDScaleRatio)
		*offset = tm_index_offset.Drop.AlvlDropPrfWREDScaleRatio * index;
	else if (address == TM.Drop.AlvlDropPrfWREDMinThresh)
		*offset = tm_index_offset.Drop.AlvlDropPrfWREDMinThresh * index;
	else if (address == TM.Drop.AlvlDropPrfTailDrpThresh)
		*offset = tm_index_offset.Drop.AlvlDropPrfTailDrpThresh * index;
	else if (address == TM.Drop.AlvlDropPrfWREDDPRatio)
		*offset = tm_index_offset.Drop.AlvlDropPrfWREDDPRatio * index;

	/* B level */
	for (i = 0; i < 3; i++) {
		if (address == TM.Drop.BlvlREDCurve[i].Table)
			*offset = tm_index_offset.Drop.BlvlREDCurve[i].Table * index;
	}
	if (address == TM.Drop.BlvlDropPrfWREDParams)
		*offset = tm_index_offset.Drop.BlvlDropPrfWREDParams * index;
	else if (address == TM.Drop.BlvlDropPrfWREDScaleRatio)
		*offset = tm_index_offset.Drop.BlvlDropPrfWREDScaleRatio * index;
	else if (address == TM.Drop.BlvlDropPrfWREDMinThresh)
		*offset = tm_index_offset.Drop.BlvlDropPrfWREDMinThresh * index;
	else if (address == TM.Drop.BlvlDropPrfTailDrpThresh)
		*offset = tm_index_offset.Drop.BlvlDropPrfTailDrpThresh * index;
	else if (address == TM.Drop.BlvlDropPrfWREDDPRatio)
		*offset = tm_index_offset.Drop.BlvlDropPrfWREDDPRatio * index;

	/* C level */
	for (i = 0; i < TM_WRED_COS; i++) {
		if (address == TM.Drop.ClvlREDCurve.CoS[i])
			*offset = tm_index_offset.Drop.ClvlREDCurve.CoS[i] * index;
		else if (address == TM.Drop.ClvlDropPrfWREDParams.CoS[i])
			*offset = tm_index_offset.Drop.ClvlDropPrfWREDParams.CoS[i] * index;
		else if (address == TM.Drop.ClvlDropPrfWREDScaleRatio.CoS[i])
			*offset = tm_index_offset.Drop.ClvlDropPrfWREDScaleRatio.CoS[i] * index;
		else if (address == TM.Drop.ClvlDropPrfWREDMinThresh.CoS[i])
			*offset = tm_index_offset.Drop.ClvlDropPrfWREDMinThresh.CoS[i] * index;
		else if (address == TM.Drop.ClvlDropPrfTailDrpThresh.CoS[i])
			*offset = tm_index_offset.Drop.ClvlDropPrfTailDrpThresh.CoS[i] * index;
		else if (address == TM.Drop.ClvlDropPrfWREDDPRatio.CoS[i])
			*offset = tm_index_offset.Drop.ClvlDropPrfWREDDPRatio.CoS[i] * index;
	}

	/* Port level */
	if (address == TM.Drop.PortREDCurve)
		*offset = tm_index_offset.Drop.PortREDCurve * index;
	else if (address == TM.Drop.PortDropPrfWREDParams)
		*offset = tm_index_offset.Drop.PortDropPrfWREDParams * index;
	else if (address == TM.Drop.PortDropPrfWREDScaleRatio)
		*offset = tm_index_offset.Drop.PortDropPrfWREDScaleRatio * index;
	else if (address == TM.Drop.PortDropPrfWREDMinThresh)
		*offset = tm_index_offset.Drop.PortDropPrfWREDMinThresh * index;
	else if (address == TM.Drop.PortDropPrfTailDrpThresh)
		*offset = tm_index_offset.Drop.PortDropPrfTailDrpThresh * index;
	else if (address == TM.Drop.PortDropPrfWREDDPRatio)
		*offset = tm_index_offset.Drop.PortDropPrfWREDDPRatio * index;

	for (i = 0; i < TM_WRED_COS; i++) {
		if (address == TM.Drop.PortREDCurve_CoS[i])
			*offset = tm_index_offset.Drop.PortREDCurve_CoS[i] * index;
		else if (address == TM.Drop.PortDropPrfWREDParams_CoSRes[i])
			*offset = tm_index_offset.Drop.PortDropPrfWREDParams_CoSRes[i] * index;
		else if (address == TM.Drop.PortDropPrfWREDScaleRatio_CoSRes[i])
			*offset = tm_index_offset.Drop.PortDropPrfWREDScaleRatio_CoSRes[i] * index;
		else if (address == TM.Drop.PortDropPrfWREDMinThresh_CoSRes[i])
			*offset = tm_index_offset.Drop.PortDropPrfWREDMinThresh_CoSRes[i] * index;
		else if (address == TM.Drop.PortDropPrfWREDDPRatio_CoSRes[i])
			*offset = tm_index_offset.Drop.PortDropPrfWREDDPRatio_CoSRes[i] * index;
		else if (address == TM.Drop.PortDropPrfTailDrpThresh_CoSRes[i])
			*offset = tm_index_offset.Drop.PortDropPrfTailDrpThresh_CoSRes[i] * index;
	}

	if (address == TM.Sched.QueueTokenBucketTokenEnDiv)
		*offset = tm_index_offset.Sched.QueueTokenBucketTokenEnDiv * index;
	else if (address == TM.Sched.QueueTokenBucketBurstSize)
		*offset = tm_index_offset.Sched.QueueTokenBucketBurstSize * index;
	else if (address == TM.Sched.AlvlTokenBucketTokenEnDiv)
		*offset = tm_index_offset.Sched.AlvlTokenBucketTokenEnDiv * index;
	else if (address == TM.Sched.AlvlTokenBucketBurstSize)
		*offset = tm_index_offset.Sched.AlvlTokenBucketBurstSize * index;
	else if (address == TM.Sched.BlvlTokenBucketTokenEnDiv)
		*offset = tm_index_offset.Sched.BlvlTokenBucketTokenEnDiv * index;
	else if (address == TM.Sched.BlvlTokenBucketBurstSize)
		*offset = tm_index_offset.Sched.BlvlTokenBucketBurstSize * index;
	else if (address == TM.Sched.ClvlTokenBucketTokenEnDiv)
		*offset = tm_index_offset.Sched.ClvlTokenBucketTokenEnDiv * index;
	else if (address == TM.Sched.ClvlTokenBucketBurstSize)
		*offset = tm_index_offset.Sched.ClvlTokenBucketBurstSize * index;

	else if (address == TM.Sched.PortRangeMap)
		*offset = tm_index_offset.Sched.PortRangeMap * index;
	else if (address == TM.Sched.ClvltoPortAndBlvlRangeMap)
		*offset = tm_index_offset.Sched.ClvltoPortAndBlvlRangeMap * index;
	else if (address == TM.Sched.BLvltoClvlAndAlvlRangeMap)
		*offset = tm_index_offset.Sched.BLvltoClvlAndAlvlRangeMap * index;
	else if (address == TM.Sched.ALvltoBlvlAndQueueRangeMap)
		*offset = tm_index_offset.Sched.ALvltoBlvlAndQueueRangeMap * index;
	else if (address == TM.Sched.QueueAMap)
		*offset = tm_index_offset.Sched.QueueAMap * index;

	else if (address == TM.Sched.QueueEligPrioFunc)
		*offset = tm_index_offset.Sched.QueueEligPrioFunc * index;
	else if (address == TM.Sched.QueueEligPrioFuncPtr)
		*offset = tm_index_offset.Sched.QueueEligPrioFuncPtr * index;
	else if (address == TM.Sched.QueueQuantum)
		*offset = tm_index_offset.Sched.QueueQuantum * index;
	else if (address == TM.Drop.QueueDropProfPtr)
		*offset = tm_index_offset.Drop.QueueDropProfPtr * index;
	else if (address == TM.Drop.QueueAvgQueueLength)
		*offset = tm_index_offset.Drop.QueueAvgQueueLength * index;

	else if (address == TM.Sched.ALvltoBlvlAndQueueRangeMap)
		*offset = tm_index_offset.Sched.ALvltoBlvlAndQueueRangeMap * index;
	else if (address == TM.Sched.AlvlEligPrioFunc)
		*offset = tm_index_offset.Sched.AlvlEligPrioFunc * index;
	else if (address == TM.Sched.AlvlEligPrioFuncPtr)
		*offset = tm_index_offset.Sched.AlvlEligPrioFuncPtr * index;
	else if (address == TM.Sched.AlvlQuantum)
		*offset = tm_index_offset.Sched.AlvlQuantum * index;
	else if (address == TM.Sched.AlvlDWRRPrioEn)
		*offset = tm_index_offset.Sched.AlvlDWRRPrioEn * index;
	else if (address == TM.Drop.AlvlDropProfPtr)
		*offset = tm_index_offset.Drop.AlvlDropProfPtr * index;
	else if (address == TM.Drop.AlvlInstAndAvgQueueLength)
		*offset = tm_index_offset.Drop.AlvlInstAndAvgQueueLength * index;

	else if (address == TM.Sched.BLvltoClvlAndAlvlRangeMap)
		*offset = tm_index_offset.Sched.BLvltoClvlAndAlvlRangeMap * index;
	else if (address == TM.Sched.BlvlEligPrioFunc)
		*offset = tm_index_offset.Sched.BlvlEligPrioFunc * index;
	else if (address == TM.Sched.BlvlEligPrioFuncPtr)
		*offset = tm_index_offset.Sched.BlvlEligPrioFuncPtr * index;
	else if (address == TM.Sched.BlvlQuantum)
		*offset = tm_index_offset.Sched.BlvlQuantum * index;
	else if (address == TM.Sched.BlvlDWRRPrioEn)
		*offset = tm_index_offset.Sched.BlvlDWRRPrioEn * index;
	else if (address == TM.Drop.BlvlDropProfPtr)
		*offset = tm_index_offset.Drop.BlvlDropProfPtr * index;
	else if (address == TM.Drop.BlvlInstAndAvgQueueLength)
		*offset = tm_index_offset.Drop.BlvlInstAndAvgQueueLength * index;

	else if (address == TM.Sched.ClvltoPortAndBlvlRangeMap)
		*offset = tm_index_offset.Sched.ClvltoPortAndBlvlRangeMap * index;
	else if (address == TM.Sched.ClvlEligPrioFunc)
		*offset = tm_index_offset.Sched.ClvlEligPrioFunc * index;
	else if (address == TM.Sched.ClvlEligPrioFuncPtr)
		*offset = tm_index_offset.Sched.ClvlEligPrioFuncPtr * index;
	else if (address == TM.Sched.ClvlQuantum)
		*offset = tm_index_offset.Sched.ClvlQuantum * index;
	else if (address == TM.Sched.ClvlDWRRPrioEn)
		*offset = tm_index_offset.Sched.ClvlDWRRPrioEn * index;
	else if (address == TM.Drop.ClvlInstAndAvgQueueLength)
		*offset = tm_index_offset.Drop.ClvlInstAndAvgQueueLength * index;
	for (i = 0; i < 8; i++) {
		if (address == TM.Drop.ClvlDropProfPtr_CoS[i])
			*offset = tm_index_offset.Drop.ClvlDropProfPtr_CoS[i] * index;
	}

	if (address == TM.Sched.PortEligPrioFunc)
		*offset = tm_index_offset.Sched.PortEligPrioFunc * index;
	else if (address == TM.Sched.PortEligPrioFuncPtr)
		*offset = tm_index_offset.Sched.PortEligPrioFuncPtr * index;
	else if (address == TM.Sched.PortTokenBucketTokenEnDiv)
		*offset = tm_index_offset.Sched.PortTokenBucketTokenEnDiv * index;
	else if (address == TM.Sched.PortTokenBucketBurstSize)
		*offset = tm_index_offset.Sched.PortTokenBucketBurstSize * index;

	else if (address == TM.Sched.PortQuantumsPriosLo)
		*offset = tm_index_offset.Sched.PortQuantumsPriosLo * index;
	else if (address == TM.Sched.PortQuantumsPriosHi)
		*offset = tm_index_offset.Sched.PortQuantumsPriosHi * index;
	else if (address == TM.Sched.PortDWRRPrioEn)
		*offset = tm_index_offset.Sched.PortDWRRPrioEn * index;
	else if (address == TM.Drop.PortInstAndAvgQueueLength)
		*offset = tm_index_offset.Drop.PortInstAndAvgQueueLength * index;

	else if (address == TM.Sched.PortDefPrioHi)
		*offset = tm_index_offset.Sched.PortDefPrioHi * index;
	else if (address == TM.Sched.PortDefPrioLo)
		*offset = tm_index_offset.Sched.PortDefPrioLo * index;

	else if (address == TM.Sched.CLvlDef)
		*offset = tm_index_offset.Sched.CLvlDef * index;
	else if (address == TM.Sched.BlvlDef)
		*offset = tm_index_offset.Sched.BlvlDef * index;
	else if (address == TM.Sched.AlvlDef)
		*offset = tm_index_offset.Sched.AlvlDef * index;
	else if (address == TM.Sched.QueueDef)
		*offset = tm_index_offset.Sched.QueueDef * index;
	else if (address == TM.Drop.QueueCoSConf)
		*offset = tm_index_offset.Drop.QueueCoSConf * index;

	else if (address == TM.Sched.ClvlShpBucketLvls)
		*offset = tm_index_offset.Sched.ClvlShpBucketLvls * index;
	else if (address == TM.Sched.PortShpBucketLvls)
		*offset = tm_index_offset.Sched.PortShpBucketLvls * index;
	else if (address == TM.Sched.BlvlShpBucketLvls)
		*offset = tm_index_offset.Sched.BlvlShpBucketLvls * index;
	else if (address == TM.Sched.AlvlShpBucketLvls)
		*offset = tm_index_offset.Sched.AlvlShpBucketLvls * index;
	else if (address == TM.Drop.AlvlDropProb)
		*offset = tm_index_offset.Drop.AlvlDropProb * index;
	else if (address == TM.Drop.ClvlDropProb)
		*offset = tm_index_offset.Drop.ClvlDropProb * index;
	else if (address == TM.Drop.BlvlDropProb)
		*offset = tm_index_offset.Drop.BlvlDropProb * index;
}
