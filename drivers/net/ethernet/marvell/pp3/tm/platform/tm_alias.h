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

#ifndef TM_ALIAS_H
#define TM_ALIAS_H

#include "common/mv_sw_if.h"

/** Register alias declaration for the TM unit.
 */
extern struct tm_alias {

	void __iomem *silicon_base;
	struct {
		int AlvlDropPrfTailDrpThresh;
		int AlvlDropPrfWREDDPRatio;
		int AlvlDropPrfWREDMinThresh;
		int AlvlDropPrfWREDParams;
		int AlvlDropPrfWREDScaleRatio;
		int AlvlDropProb;
		int AlvlDropProfPtr;
		int AlvlInstAndAvgQueueLength;
		struct {
			int Color[3];
		} AlvlREDCurve;
		int BlvlDropPrfTailDrpThresh;
		int BlvlDropPrfWREDDPRatio;
		int BlvlDropPrfWREDMinThresh;
		int BlvlDropPrfWREDParams;
		int BlvlDropPrfWREDScaleRatio;
		int BlvlDropProb;
		int BlvlDropProfPtr;
		int BlvlInstAndAvgQueueLength;
		struct {
			int Table;
		} BlvlREDCurve[4];
		struct {
			int CoS[8];
		} ClvlDropPrfTailDrpThresh;
		struct {
			int CoS[8];
		} ClvlDropPrfWREDDPRatio;
		struct {
			int CoS[8];
		} ClvlDropPrfWREDMinThresh;
		struct {
			int CoS[8];
		} ClvlDropPrfWREDParams;
		struct {
			int CoS[8];
		} ClvlDropPrfWREDScaleRatio;
		int ClvlDropProb;
		int ClvlDropProfPtr_CoS[8];
		int ClvlInstAndAvgQueueLength;
		struct {
			int CoS[8];
		} ClvlREDCurve;
		int DPSource;
		int Drp_Decision_hierarchy_to_Query_debug;/*NEW*/
		int Drp_Decision_to_Query_debug;/*NEW*/
		int EccConfig;/*NEW*/
		int EccMemParams[43];/*NEW*/
		int ErrCnt;
		int ErrStus;
		int ExcCnt;
		int ExcMask;
		int FirstExc;
		int ForceErr;
		int Id;
		int PortDropPrfTailDrpThresh;
		int PortDropPrfTailDrpThresh_CoSRes[8];
		int PortDropPrfWREDDPRatio;
		int PortDropPrfWREDDPRatio_CoSRes[8];
		int PortDropPrfWREDMinThresh;
		int PortDropPrfWREDMinThresh_CoSRes[8];
		int PortDropPrfWREDParams;
		int PortDropPrfWREDParams_CoSRes[8];
		int PortDropPrfWREDScaleRatio;
		int PortDropPrfWREDScaleRatio_CoSRes[8];
		int PortDropProb;
		int PortDropProbPerCoS_CoS[8];
		int PortInstAndAvgQueueLength;
		struct {
			int CoS[8];
		} PortInstAndAvgQueueLengthPerCoS;
		int PortREDCurve;
		int PortREDCurve_CoS[8];
		int QueueAvgQueueLength;
		int QueueCoSConf;
		int QueueDropPrfTailDrpThresh;
		int QueueDropPrfWREDDPRatio;
		int QueueDropPrfWREDMinThresh;
		int QueueDropPrfWREDParams;
		int QueueDropPrfWREDScaleRatio;
		int QueueDropProb;
		int QueueDropProfPtr;
		struct {
			int Color[3];
		} QueueREDCurve;
		int RespLocalDPSel;
		int WREDDropProbMode;
		int WREDMaxProbModePerColor;
	} Drop;
	struct {
		int ALevelShaperBucketNeg;
		int ALvltoBlvlAndQueueRangeMap;
		int AlvlBankEccErrStatus;/*NEW*/
		int AlvlDWRRPrioEn;
		int AlvlDef;
		int AlvlEccErrStatus;/*NEW*/
		int AlvlEligPrioFunc;
		int AlvlEligPrioFuncPtr;
		int AlvlL0ClusterStateHi;/*NEW*/
		int AlvlL0ClusterStateLo;/*NEW*/
		int AlvlL1ClusterStateHi;/*NEW*/
		int AlvlL1ClusterStateLo;/*NEW*/
		int AlvlL2ClusterStateHi;/*NEW*/
		int AlvlL2ClusterStateLo;/*NEW*/
		int AlvlMyQ;/*NEW*/
		int AlvlMyQEccErrStatus;/*NEW*/
		int AlvlNodeState;/*NEW*/
		int AlvlPerConf;
		int AlvlPerRateShpPrms;
		int AlvlPerRateShpPrmsInt;/*NEW*/
		int AlvlPerStatus;/*NEW*/
		int AlvlQuantum;
		int AlvlRRDWRRStatus01;/*NEW*/
		int AlvlRRDWRRStatus23;/*NEW*/
		int AlvlRRDWRRStatus45;/*NEW*/
		int AlvlRRDWRRStatus67;/*NEW*/
		int AlvlShpBucketLvls;
		int AlvlTokenBucketBurstSize;
		int AlvlTokenBucketTokenEnDiv;
		int AlvlWFS;/*NEW*/
		int BLevelShaperBucketNeg;
		int BLvltoClvlAndAlvlRangeMap;
		int BlvlBankEccErrStatus;/*NEW*/
		int BlvlDWRRPrioEn;
		int BlvlDef;
		int BlvlEccErrStatus;/*NEW*/
		int BlvlEligPrioFunc;
		int BlvlEligPrioFuncPtr;
		int BlvlL0ClusterStateHi;/*NEW*/
		int BlvlL0ClusterStateLo;/*NEW*/
		int BlvlL1ClusterStateHi;/*NEW*/
		int BlvlL1ClusterStateLo;/*NEW*/
		int BlvlMyQ;/*NEW*/
		int BlvlMyQEccErrStatus;/*NEW*/
		int BlvlNodeState;/*NEW*/
		int BlvlPerConf;
		int BlvlPerRateShpPrms;
		int BlvlPerRateShpPrmsInt;/*NEW*/
		int BlvlPerStatus;/*NEW*/
		int BlvlQuantum;
		int BlvlRRDWRRStatus01;/*NEW*/
		int BlvlRRDWRRStatus23;/*NEW*/
		int BlvlRRDWRRStatus45;/*NEW*/
		int BlvlRRDWRRStatus67;/*NEW*/
		int BlvlShpBucketLvls;
		int BlvlTokenBucketBurstSize;
		int BlvlTokenBucketTokenEnDiv;
		int BlvlWFS;/*NEW*/
		int CLevelShaperBucketNeg;
		int CLvlDef;
		int ClvlBPFromSTF;/*NEW*/
		int ClvlBankEccErrStatus;/*NEW*/
		int ClvlDWRRPrioEn;
		int ClvlEccErrStatus;/*NEW*/
		int ClvlEligPrioFunc;
		int ClvlEligPrioFuncPtr;
		int ClvlL0ClusterStateHi;/*NEW*/
		int ClvlL0ClusterStateLo;/*NEW*/
		int ClvlMyQ;/*NEW*/
		int ClvlMyQEccErrStatus;/*NEW*/
		int ClvlNodeState;/*NEW*/
		int ClvlPerConf;
		int ClvlPerRateShpPrms;
		int ClvlPerRateShpPrmsInt;/*NEW*/
		int ClvlPerStatus;/*NEW*/
		int ClvlQuantum;
		int ClvlRRDWRRStatus01;/*NEW*/
		int ClvlRRDWRRStatus23;/*NEW*/
		int ClvlRRDWRRStatus45;/*NEW*/
		int ClvlRRDWRRStatus67;/*NEW*/
		int ClvlShpBucketLvls;
		int ClvlTokenBucketBurstSize;
		int ClvlTokenBucketTokenEnDiv;
		int ClvlWFS;/*NEW*/
		int ClvltoPortAndBlvlRangeMap;
		int EccConfig;/*NEW*/
		int EccMemParams[46];/*NEW*/
		int ErrCnt;
		int ErrStus;
		int ExcCnt;
		int ExcMask;
		int FirstExc;
		int ForceErr;
		int GeneralEccErrStatus;/*NEW*/
		int Id;
		int PortBPFromQMgr;/*NEW*/
		int PortBPFromSTF;/*NEW*/
		int PortBankEccErrStatus;/*NEW*/
		int PortDWRRBytesPerBurstsLimit;
		int PortDWRRPrioEn;
		int PortDefPrioHi;
		int PortDefPrioLo;
		int PortEccErrStatus;/*NEW*/
		int PortEligPrioFunc;
		int PortEligPrioFuncPtr;
		int PortExtBPEn;
		int PortMyQ;/*NEW*/
		int PortNodeState;/*NEW*/
		int PortPerConf;
		int PortPerRateShpPrms;
		int PortPerRateShpPrmsInt;/*NEW*/
		int PortPerStatus;/*NEW*/
		int PortQuantumsPriosHi;
		int PortQuantumsPriosLo;
		int PortRRDWRRStatus01;/*NEW*/
		int PortRRDWRRStatus23;/*NEW*/
		int PortRRDWRRStatus45;/*NEW*/
		int PortRRDWRRStatus67;/*NEW*/
		int PortRangeMap;
		int PortShaperBucketNeg;
		int PortShpBucketLvls;
		int PortTokenBucketBurstSize;
		int PortTokenBucketTokenEnDiv;
		int PortWFS;/*NEW*/
		int QueueAMap;
		int QueueBank0EccErrStatus;/*NEW*/
		int QueueBank1EccErrStatus;/*NEW*/
		int QueueBank2EccErrStatus;/*NEW*/
		int QueueBank3EccErrStatus;/*NEW*/
		int QueueDef;
		int QueueEccErrStatus;/*NEW*/
		int QueueEligPrioFunc;
		int QueueEligPrioFuncPtr;
		int QueueL0ClusterStateHi;/*NEW*/
		int QueueL0ClusterStateLo;/*NEW*/
		int QueueL1ClusterStateHi;/*NEW*/
		int QueueL1ClusterStateLo;/*NEW*/
		int QueueL2ClusterStateHi;/*NEW*/
		int QueueL2ClusterStateLo;/*NEW*/
		int QueueNodeState;/*NEW*/
		int QueuePerConf;
		int QueuePerRateShpPrms;
		int QueuePerRateShpPrmsInt;/*NEW*/
		int QueuePerStatus;/*NEW*/
		int QueueQuantum;
		int QueueShaperBucketNeg;
		int QueueShpBucketLvls;
		int QueueTokenBucketBurstSize;
		int QueueTokenBucketTokenEnDiv;
		int QueueWFS;/*NEW*/
		int ScrubDisable;/*NEW*/
		int ScrubSlotAlloc;
		int TreeDWRRPrioEn;
		int TreeDeqEn;
		int TreeRRDWRRStatus;/*NEW*/
	} Sched;
} TM;

extern struct tm_alias tm_index_offset;


void init_tm_alias_struct(void __iomem *base);
void init_tm_init_offset_struct(void);

#endif /* TM_ALIAS_H */
