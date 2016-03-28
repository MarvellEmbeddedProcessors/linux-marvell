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

#ifndef TM_PAYLOADS_H
#define TM_PAYLOADS_H

/** NPU Register payload TM_Sched_AlvlBankEccErrStatus (PLID#1241).
 *
 * Used by TM.Sched.AlvlBankEccErrStatus.
 */
struct TM_Sched_AlvlBankEccErrStatus {
	uint64_t UncEccErr:5;       /**< byte[0-7],bit[0-4] */
	uint64_t _reserved_1:11;    /**< byte[0-7],bit[5-15] */
	uint64_t CorrEccErr:5;      /**< byte[0-7],bit[16-20] */
	uint64_t _reserved_2:43;    /**< byte[0-7],bit[21-63] */
}; /* PLID#1241 */

/** NPU Register payload TM_Drop_DPSource (PLID#1264).
 *
 * Used by TM.Drop.DPSource.
 */
struct TM_Drop_DPSource {
	uint64_t QueueSrc:3;        /**< byte[0-7],bit[0-2] */
	uint64_t _reserved_1:5;     /**< byte[0-7],bit[3-7] */
	uint64_t AlvlSrc:3;         /**< byte[0-7],bit[8-10] */
	uint64_t _reserved_2:5;     /**< byte[0-7],bit[11-15] */
	uint64_t BlvlSrc:3;         /**< byte[0-7],bit[16-18] */
	uint64_t _reserved_3:5;     /**< byte[0-7],bit[19-23] */
	uint64_t ClvlSrc:3;         /**< byte[0-7],bit[24-26] */
	uint64_t _reserved_4:5;     /**< byte[0-7],bit[27-31] */
	uint64_t PortSrc:3;         /**< byte[0-7],bit[32-34] */
	uint64_t _reserved_5:29;    /**< byte[0-7],bit[35-63] */
}; /* PLID#1264 */

/** NPU Register payload TM_Sched_QueueL1ClusterStateLo (PLID#1390).
 *
 * Used by TM.Sched.QueueL1ClusterStateLo.
 */
struct TM_Sched_QueueL1ClusterStateLo {
	uint64_t Status:40;         /**< byte[0-7],bit[0-39] */
	uint64_t _reserved_1:24;    /**< byte[0-7],bit[40-63] */
}; /* PLID#1390 */

/** NPU Register payload TM_Drop_BlvlDropPrfWREDDPRatio (PLID#1421).
 *
 * Used by TM.Drop.BlvlDropPrfWREDDPRatio.
 */
struct TM_Drop_BlvlDropPrfWREDDPRatio {
	uint64_t DPRatio0:6;        /**< byte[0-7],bit[0-5] */
	uint64_t _reserved_1:2;     /**< byte[0-7],bit[6-7] */
	uint64_t DPRatio1:6;        /**< byte[0-7],bit[8-13] */
	uint64_t _reserved_2:2;     /**< byte[0-7],bit[14-15] */
	uint64_t DPRatio2:4;        /**< byte[0-7],bit[16-19] */
	uint64_t _reserved_3:44;    /**< byte[0-7],bit[20-63] */
}; /* PLID#1421 */

/** NPU Register payload TM_Sched_ClvlRRDWRRStatus01 (PLID#1383).
 *
 * Used by TM.Sched.ClvlRRDWRRStatus01.
 */
struct TM_Sched_ClvlRRDWRRStatus01 {
	uint64_t Status:26;         /**< byte[0-7],bit[0-25] */
	uint64_t _reserved_1:38;    /**< byte[0-7],bit[26-63] */
}; /* PLID#1383 */

/** NPU Register payload TM_Sched_PortBPFromQMgr (PLID#1380).
 *
 * Used by TM.Sched.PortBPFromQMgr.
 */
struct TM_Sched_PortBPFromQMgr {
	uint64_t BP;                /**< byte[0-7] */
}; /* PLID#1380 */

/** NPU Register payload TM_Drop_PortInstAndAvgQueueLength (PLID#1429).
 *
 * Used by TM.Drop.PortInstAndAvgQueueLength.
 */
struct TM_Drop_PortInstAndAvgQueueLength {
	uint64_t QL:29;             /**< byte[0-7],bit[0-28] */
	uint64_t _reserved_1:3;     /**< byte[0-7],bit[29-31] */
	uint64_t AQL:29;            /**< byte[0-7],bit[32-60] */
	uint64_t _reserved_2:3;     /**< byte[0-7],bit[61-63] */
}; /* PLID#1429 */

/** NPU Register payload TM_Sched_BlvlPerStatus (PLID#1245).
 *
 * Used by TM.Sched.BlvlPerStatus.
 */
struct TM_Sched_BlvlPerStatus {
	uint64_t PerRoundCnt:10;    /**< byte[0-7],bit[0-9] */
	uint64_t _reserved_1:22;    /**< byte[0-7],bit[10-31] */
	uint64_t PerPointer:11;     /**< byte[0-7],bit[32-42] */
	uint64_t _reserved_2:21;    /**< byte[0-7],bit[43-63] */
}; /* PLID#1245 */

/** NPU Register payload TM_Sched_ClvltoPortAndBlvlRangeMap (PLID#1405).
 *
 * Used by TM.Sched.ClvltoPortAndBlvlRangeMap.
 */
struct TM_Sched_ClvltoPortAndBlvlRangeMap {
	uint64_t BlvlLo:5;          /**< byte[0-7],bit[0-4] */
	uint64_t _reserved_1:19;    /**< byte[0-7],bit[5-23] */
	uint64_t BlvlHi:5;          /**< byte[0-7],bit[24-28] */
	uint64_t _reserved_2:19;    /**< byte[0-7],bit[29-47] */
	uint64_t Port:4;            /**< byte[0-7],bit[48-51] */
	uint64_t _reserved_3:12;    /**< byte[0-7],bit[52-63] */
}; /* PLID#1405 */

/** NPU Register payload TM_Sched_AlvlDWRRPrioEn (PLID#1398).
 *
 * Used by TM.Sched.AlvlDWRRPrioEn.
 */
struct TM_Sched_AlvlDWRRPrioEn {
	uint64_t En:8;              /**< byte[0-7],bit[0-7] */
	uint64_t _reserved_1:56;    /**< byte[0-7],bit[8-63] */
}; /* PLID#1398 */

/** NPU Register payload TM_Drop_QueueREDCurve_Color (PLID#1423).
 *
 * Used by TM.Drop.QueueREDCurve.Color[0-2].
 */
struct TM_Drop_QueueREDCurve_Color {
	uint64_t Prob:6;            /**< byte[0-7],bit[0-5] */
	uint64_t _reserved_1:58;    /**< byte[0-7],bit[6-63] */
}; /* PLID#1423 */

/** NPU Register payload TM_Sched_ErrCnt (PLID#1180).
 *
 * Used by TM.Sched.ErrCnt.
 */
struct TM_Sched_ErrCnt {
	uint64_t Cnt:16;            /**< byte[0-7],bit[0-15] */
	uint64_t _reserved_1:48;    /**< byte[0-7],bit[16-63] */
}; /* PLID#1180 */

/** NPU Register payload TM_Sched_PortMyQ (PLID#1377).
 *
 * Used by TM.Sched.PortMyQ.
 */
struct TM_Sched_PortMyQ {
	uint64_t Status:40;         /**< byte[0-7],bit[0-39] */
	uint64_t _reserved_1:24;    /**< byte[0-7],bit[40-63] */
}; /* PLID#1377 */

/** NPU Register payload TM_Sched_QueuePerStatus (PLID#1393).
 *
 * Used by TM.Sched.QueuePerStatus.
 */
struct TM_Sched_QueuePerStatus {
	uint64_t PerRoundCnt:10;    /**< byte[0-7],bit[0-9] */
	uint64_t _reserved_1:22;    /**< byte[0-7],bit[10-31] */
	uint64_t PerPointer:11;     /**< byte[0-7],bit[32-42] */
	uint64_t _reserved_2:21;    /**< byte[0-7],bit[43-63] */
}; /* PLID#1393 */

/** NPU Register payload TM_Sched_PortShpBucketLvls (PLID#1411).
 *
 * Used by TM.Sched.PortShpBucketLvls.
 */
struct TM_Sched_PortShpBucketLvls {
	uint64_t MinLvl:28;         /**< byte[0-7],bit[0-27] */
	uint64_t _reserved_1:4;     /**< byte[0-7],bit[28-31] */
	uint64_t MaxLvl:28;         /**< byte[0-7],bit[32-59] */
	uint64_t _reserved_2:4;     /**< byte[0-7],bit[60-63] */
}; /* PLID#1411 */

/** NPU Register payload TM_Sched_ClvlDWRRPrioEn (PLID#1398).
 *
 * Used by TM.Sched.ClvlDWRRPrioEn.
 */
struct TM_Sched_ClvlDWRRPrioEn {
	uint64_t En:8;              /**< byte[0-7],bit[0-7] */
	uint64_t _reserved_1:56;    /**< byte[0-7],bit[8-63] */
}; /* PLID#1398 */

/** NPU Register payload TM_Sched_QueueBank3EccErrStatus (PLID#1241).
 *
 * Used by TM.Sched.QueueBank3EccErrStatus.
 */
struct TM_Sched_QueueBank3EccErrStatus {
	uint64_t UncEccErr:5;       /**< byte[0-7],bit[0-4] */
	uint64_t _reserved_1:11;    /**< byte[0-7],bit[5-15] */
	uint64_t CorrEccErr:5;      /**< byte[0-7],bit[16-20] */
	uint64_t _reserved_2:43;    /**< byte[0-7],bit[21-63] */
}; /* PLID#1241 */

/** NPU Register payload TM_Sched_AlvlDef (PLID#1414).
 *
 * Used by TM.Sched.AlvlDef.
 */
struct TM_Sched_AlvlDef {
	uint64_t Deficit:22;        /**< byte[0-7],bit[0-21] */
	uint64_t _reserved_1:42;    /**< byte[0-7],bit[22-63] */
}; /* PLID#1414 */

/** NPU Register payload TM_Sched_BlvlWFS (PLID#1379).
 *
 * Used by TM.Sched.BlvlWFS.
 */
struct TM_Sched_BlvlWFS {
	uint64_t WFS:32;            /**< byte[0-7],bit[0-31] */
	uint64_t _reserved_1:32;    /**< byte[0-7],bit[32-63] */
}; /* PLID#1379 */

/** NPU Register payload TM_Sched_ClvlEligPrioFuncPtr (PLID#1395).
 *
 * Used by TM.Sched.ClvlEligPrioFuncPtr.
 */
struct TM_Sched_ClvlEligPrioFuncPtr {
	uint64_t Ptr:6;             /**< byte[0-7],bit[0-5] */
	uint64_t _reserved_1:58;    /**< byte[0-7],bit[6-63] */
}; /* PLID#1395 */

/** NPU Register payload TM_Drop_BlvlDropPrfWREDScaleRatio (PLID#1419).
 *
 * Used by TM.Drop.BlvlDropPrfWREDScaleRatio.
 */
struct TM_Drop_BlvlDropPrfWREDScaleRatio {
	uint64_t ScaleRatioColor0:10;/**< byte[0-7],bit[0-9] */
	uint64_t _reserved_1:6;     /**< byte[0-7],bit[10-15] */
	uint64_t ScaleRatioColor1:10;/**< byte[0-7],bit[16-25] */
	uint64_t _reserved_2:6;     /**< byte[0-7],bit[26-31] */
	uint64_t ScaleRatioColor2:10;/**< byte[0-7],bit[32-41] */
	uint64_t _reserved_3:22;    /**< byte[0-7],bit[42-63] */
}; /* PLID#1419 */

/** NPU Register payload TM_Sched_BlvlMyQ (PLID#1386).
 *
 * Used by TM.Sched.BlvlMyQ.
 */
struct TM_Sched_BlvlMyQ {
	uint64_t Status:28;         /**< byte[0-7],bit[0-27] */
	uint64_t _reserved_1:36;    /**< byte[0-7],bit[28-63] */
}; /* PLID#1386 */

/** NPU Register payload TM_Sched_EccMemParams (PLID#1231).
 *
 * Used by TM.Sched.EccMemParams[0-45].
 */
struct TM_Sched_EccMemParams {
	uint64_t Counter:8;         /**< byte[0-7],bit[0-7] */
	uint64_t Address:24;        /**< byte[0-7],bit[8-31] */
	uint64_t Syndrom:32;        /**< byte[0-7],bit[32-63] */
}; /* PLID#1231 */

/** NPU Register payload TM_Sched_TMtoTMBlvlBPState (PLID#1385).
 *
 * Used by TM.Sched.TMtoTMBlvlBPState.
 */
struct TM_Sched_TMtoTMBlvlBPState {
	uint64_t BPState:1;         /**< byte[0-7],bit[0] */
	uint64_t _reserved_1:63;    /**< byte[0-7],bit[1-63] */
}; /* PLID#1385 */

/** NPU Register payload TM_Sched_BlvlL0ClusterStateHi (PLID#1417).
 *
 * Used by TM.Sched.BlvlL0ClusterStateHi.
 */
struct TM_Sched_BlvlL0ClusterStateHi {
	uint64_t status;            /**< byte[0-7] */
}; /* PLID#1417 */

/** NPU Register payload TM_Drop_TMtoTMPktGenQuantum (PLID#1266).
 *
 * Used by TM.Drop.TMtoTMPktGenQuantum.
 */
struct TM_Drop_TMtoTMPktGenQuantum {
	uint64_t Quantum:16;        /**< byte[0-7],bit[0-15] */
	uint64_t _reserved_1:48;    /**< byte[0-7],bit[16-63] */
}; /* PLID#1266 */

/** NPU Register payload TM_Drop_PortDropPrfWREDMinThresh (PLID#1420).
 *
 * Used by TM.Drop.PortDropPrfWREDMinThresh.
 */
struct TM_Drop_PortDropPrfWREDMinThresh {
	uint64_t MinTHColor0:10;    /**< byte[0-7],bit[0-9] */
	uint64_t _reserved_1:6;     /**< byte[0-7],bit[10-15] */
	uint64_t MinTHColor1:10;    /**< byte[0-7],bit[16-25] */
	uint64_t _reserved_2:6;     /**< byte[0-7],bit[26-31] */
	uint64_t MinTHColor2:10;    /**< byte[0-7],bit[32-41] */
	uint64_t _reserved_3:22;    /**< byte[0-7],bit[42-63] */
}; /* PLID#1420 */

/** NPU Register payload TM_Sched_ClvlNodeState (PLID#1381).
 *
 * Used by TM.Sched.ClvlNodeState.
 */
struct TM_Sched_ClvlNodeState {
	uint64_t State:11;          /**< byte[0-7],bit[0-10] */
	uint64_t _reserved_1:53;    /**< byte[0-7],bit[11-63] */
}; /* PLID#1381 */

/** NPU Register payload TM_Drop_PortInstAndAvgQueueLengthPerCoS_CoS (PLID#1429).
 *
 * Used by TM.Drop.PortInstAndAvgQueueLengthPerCoS.CoS[0-7].
 */
struct TM_Drop_PortInstAndAvgQueueLengthPerCoS_CoS {
	uint64_t QL:29;             /**< byte[0-7],bit[0-28] */
	uint64_t _reserved_1:3;     /**< byte[0-7],bit[29-31] */
	uint64_t AQL:29;            /**< byte[0-7],bit[32-60] */
	uint64_t _reserved_2:3;     /**< byte[0-7],bit[61-63] */
}; /* PLID#1429 */

/** NPU Register payload TM_Drop_Drp_Decision_to_Query_debug (PLID#1268).
 *
 * Used by TM.Drop.Drp_Decision_to_Query_debug.
 */
struct TM_Drop_Drp_Decision_to_Query_debug {
	uint64_t Decision_fields:27;/**< byte[0-7],bit[0-26] */
	uint64_t _reserved_1:5;     /**< byte[0-7],bit[27-31] */
	uint64_t Debug_En:1;        /**< byte[0-7],bit[32] */
	uint64_t _reserved_2:31;    /**< byte[0-7],bit[33-63] */
}; /* PLID#1268 */

/** NPU Register payload TM_Sched_PortBankEccErrStatus (PLID#1239).
 *
 * Used by TM.Sched.PortBankEccErrStatus.
 */
struct TM_Sched_PortBankEccErrStatus {
	uint64_t UncEccErr:2;       /**< byte[0-7],bit[0-1] */
	uint64_t _reserved_1:14;    /**< byte[0-7],bit[2-15] */
	uint64_t CorrEccErr:2;      /**< byte[0-7],bit[16-17] */
	uint64_t _reserved_2:46;    /**< byte[0-7],bit[18-63] */
}; /* PLID#1239 */

/** NPU Register payload TM_Sched_AlvlRRDWRRStatus45 (PLID#1389).
 *
 * Used by TM.Sched.AlvlRRDWRRStatus45.
 */
struct TM_Sched_AlvlRRDWRRStatus45 {
	uint64_t Status:34;         /**< byte[0-7],bit[0-33] */
	uint64_t _reserved_1:30;    /**< byte[0-7],bit[34-63] */
}; /* PLID#1389 */

/** NPU Register payload TM_Drop_PortDropPrfWREDDPRatio_CoSRes (PLID#1421).
 *
 * Used by TM.Drop.PortDropPrfWREDDPRatio_CoSRes[0-7].
 */
struct TM_Drop_PortDropPrfWREDDPRatio_CoSRes {
	uint64_t DPRatio0:6;        /**< byte[0-7],bit[0-5] */
	uint64_t _reserved_1:2;     /**< byte[0-7],bit[6-7] */
	uint64_t DPRatio1:6;        /**< byte[0-7],bit[8-13] */
	uint64_t _reserved_2:2;     /**< byte[0-7],bit[14-15] */
	uint64_t DPRatio2:4;        /**< byte[0-7],bit[16-19] */
	uint64_t _reserved_3:44;    /**< byte[0-7],bit[20-63] */
}; /* PLID#1421 */

/** NPU Register payload TM_Drop_Drp_Decision_hierarchy_to_Query_debug (PLID#1269).
 *
 * Used by TM.Drop.Drp_Decision_hierarchy_to_Query_debug.
 */
struct TM_Drop_Drp_Decision_hierarchy_to_Query_debug {
	uint64_t Hierarchy_fields:46;/**< byte[0-7],bit[0-45] */
	uint64_t _reserved_1:18;    /**< byte[0-7],bit[46-63] */
}; /* PLID#1269 */

/** NPU Register payload TM_Drop_ExcCnt (PLID#1180).
 *
 * Used by TM.Drop.ExcCnt.
 */
struct TM_Drop_ExcCnt {
	uint64_t Cnt:16;            /**< byte[0-7],bit[0-15] */
	uint64_t _reserved_1:48;    /**< byte[0-7],bit[16-63] */
}; /* PLID#1180 */

/** NPU Register payload TM_Drop_FirstExc (PLID#1259).
 *
 * Used by TM.Drop.FirstExc.
 */
struct TM_Drop_FirstExc {
	uint64_t ForcedErr:1;       /**< byte[0-7],bit[0] */
	uint64_t CorrECCErr:1;      /**< byte[0-7],bit[1] */
	uint64_t UncECCErr:1;       /**< byte[0-7],bit[2] */
	uint64_t QuesryRespSyncFifoFull:1;/**< byte[0-7],bit[3] */
	uint64_t QueryReqFifoOverflow:1;/**< byte[0-7],bit[4] */
	uint64_t AgingFifoOverflow:1;/**< byte[0-7],bit[5] */
	uint64_t _reserved_1:58;    /**< byte[0-7],bit[6-63] */
}; /* PLID#1259 */

/** NPU Register payload TM_Sched_PortNodeState (PLID#1376).
 *
 * Used by TM.Sched.PortNodeState.
 */
struct TM_Sched_PortNodeState {
	uint64_t State:18;          /**< byte[0-7],bit[0-17] */
	uint64_t _reserved_1:46;    /**< byte[0-7],bit[18-63] */
}; /* PLID#1376 */

/** NPU Register payload TM_Sched_ClvlRRDWRRStatus67 (PLID#1383).
 *
 * Used by TM.Sched.ClvlRRDWRRStatus67.
 */
struct TM_Sched_ClvlRRDWRRStatus67 {
	uint64_t Status:26;         /**< byte[0-7],bit[0-25] */
	uint64_t _reserved_1:38;    /**< byte[0-7],bit[26-63] */
}; /* PLID#1383 */

/** NPU Register payload TM_Sched_QueueEligPrioFunc (PLID#1408).
 *
 * Used by TM.Sched.QueueEligPrioFunc.
 */
struct TM_Sched_QueueEligPrioFunc {
	uint64_t FuncOut0:9;        /**< byte[0-7],bit[0-8] */
	uint64_t _reserved_1:7;     /**< byte[0-7],bit[9-15] */
	uint64_t FuncOut1:9;        /**< byte[0-7],bit[16-24] */
	uint64_t _reserved_2:7;     /**< byte[0-7],bit[25-31] */
	uint64_t FuncOut2:9;        /**< byte[0-7],bit[32-40] */
	uint64_t _reserved_3:7;     /**< byte[0-7],bit[41-47] */
	uint64_t FuncOut3:9;        /**< byte[0-7],bit[48-56] */
	uint64_t _reserved_4:7;     /**< byte[0-7],bit[57-63] */
}; /* PLID#1408 */

/** NPU Register payload TM_Drop_PortDropPrfWREDDPRatio (PLID#1421).
 *
 * Used by TM.Drop.PortDropPrfWREDDPRatio.
 */
struct TM_Drop_PortDropPrfWREDDPRatio {
	uint64_t DPRatio0:6;        /**< byte[0-7],bit[0-5] */
	uint64_t _reserved_1:2;     /**< byte[0-7],bit[6-7] */
	uint64_t DPRatio1:6;        /**< byte[0-7],bit[8-13] */
	uint64_t _reserved_2:2;     /**< byte[0-7],bit[14-15] */
	uint64_t DPRatio2:4;        /**< byte[0-7],bit[16-19] */
	uint64_t _reserved_3:44;    /**< byte[0-7],bit[20-63] */
}; /* PLID#1421 */

/** NPU Register payload TM_Sched_BlvlRRDWRRStatus67 (PLID#1363).
 *
 * Used by TM.Sched.BlvlRRDWRRStatus67.
 */
struct TM_Sched_BlvlRRDWRRStatus67 {
	uint64_t Status:30;         /**< byte[0-7],bit[0-29] */
	uint64_t _reserved_1:34;    /**< byte[0-7],bit[30-63] */
}; /* PLID#1363 */

/** NPU Register payload TM_Sched_BlvlL1ClusterStateHi (PLID#1387).
 *
 * Used by TM.Sched.BlvlL1ClusterStateHi.
 */
struct TM_Sched_BlvlL1ClusterStateHi {
	uint64_t Status:42;         /**< byte[0-7],bit[0-41] */
	uint64_t _reserved_1:22;    /**< byte[0-7],bit[42-63] */
}; /* PLID#1387 */

/** NPU Register payload TM_Drop_QueueDropPrfWREDParams (PLID#1426).
 *
 * Used by TM.Drop.QueueDropPrfWREDParams.
 */
struct TM_Drop_QueueDropPrfWREDParams {
	uint64_t CurveIndexColor0:3;/**< byte[0-7],bit[0-2] */
	uint64_t _reserved_1:5;     /**< byte[0-7],bit[3-7] */
	uint64_t CurveIndexColor1:3;/**< byte[0-7],bit[8-10] */
	uint64_t _reserved_2:5;     /**< byte[0-7],bit[11-15] */
	uint64_t CurveIndexColor2:3;/**< byte[0-7],bit[16-18] */
	uint64_t _reserved_3:5;     /**< byte[0-7],bit[19-23] */
	uint64_t ScaleExpColor0:5;  /**< byte[0-7],bit[24-28] */
	uint64_t _reserved_4:3;     /**< byte[0-7],bit[29-31] */
	uint64_t ScaleExpColor1:5;  /**< byte[0-7],bit[32-36] */
	uint64_t _reserved_5:3;     /**< byte[0-7],bit[37-39] */
	uint64_t ScaleExpColor2:5;  /**< byte[0-7],bit[40-44] */
	uint64_t _reserved_6:3;     /**< byte[0-7],bit[45-47] */
	uint64_t ColorTDEn:1;       /**< byte[0-7],bit[48] */
	uint64_t _reserved_7:7;     /**< byte[0-7],bit[49-55] */
	uint64_t AQLExp:4;          /**< byte[0-7],bit[56-59] */
	uint64_t _reserved_8:4;     /**< byte[0-7],bit[60-63] */
}; /* PLID#1426 */

/** NPU Register payload TM_Sched_PortBPFromSTF (PLID#1380).
 *
 * Used by TM.Sched.PortBPFromSTF.
 */
struct TM_Sched_PortBPFromSTF {
	uint64_t BP;                /**< byte[0-7] */
}; /* PLID#1380 */

/** NPU Register payload TM_Sched_ExcMask (PLID#1248).
 *
 * Used by TM.Sched.ExcMask.
 */
struct TM_Sched_ExcMask {
	uint64_t ForcedErr:1;       /**< byte[0-7],bit[0] */
	uint64_t CorrECCErr:1;      /**< byte[0-7],bit[1] */
	uint64_t UncECCErr:1;       /**< byte[0-7],bit[2] */
	uint64_t BPBSat:1;          /**< byte[0-7],bit[3] */
	uint64_t TBNegSat:1;        /**< byte[0-7],bit[4] */
	uint64_t FIFOOvrflowErr:1;  /**< byte[0-7],bit[5] */
	uint64_t _reserved_1:58;    /**< byte[0-7],bit[6-63] */
}; /* PLID#1248 */

/** NPU Register payload TM_Drop_BlvlDropProb (PLID#1430).
 *
 * Used by TM.Drop.BlvlDropProb.
 */
struct TM_Drop_BlvlDropProb {
	uint64_t DropProb:13;       /**< byte[0-7],bit[0-12] */
	uint64_t _reserved_1:51;    /**< byte[0-7],bit[13-63] */
}; /* PLID#1430 */

/** NPU Register payload TM_Sched_TMtoTMBpFIFOBp (PLID#1206).
 *
 * Used by TM.Sched.TMtoTMBpFIFOBp.
 */
struct TM_Sched_TMtoTMBpFIFOBp {
	uint64_t ClrThresh:5;       /**< byte[0-7],bit[0-4] */
	uint64_t _reserved_1:27;    /**< byte[0-7],bit[5-31] */
	uint64_t SetThresh:5;       /**< byte[0-7],bit[32-36] */
	uint64_t _reserved_2:27;    /**< byte[0-7],bit[37-63] */
}; /* PLID#1206 */

/** NPU Register payload TM_Sched_PortWFS (PLID#1379).
 *
 * Used by TM.Sched.PortWFS.
 */
struct TM_Sched_PortWFS {
	uint64_t WFS:32;            /**< byte[0-7],bit[0-31] */
	uint64_t _reserved_1:32;    /**< byte[0-7],bit[32-63] */
}; /* PLID#1379 */

/** NPU Register payload TM_Drop_PortDropPrfTailDrpThresh (PLID#1422).
 *
 * Used by TM.Drop.PortDropPrfTailDrpThresh.
 */
struct TM_Drop_PortDropPrfTailDrpThresh {
	uint64_t TailDropThresh:19; /**< byte[0-7],bit[0-18] */
	uint64_t _reserved_1:13;    /**< byte[0-7],bit[19-31] */
	uint64_t TailDropThreshRes:1;/**< byte[0-7],bit[32] */
	uint64_t _reserved_2:31;    /**< byte[0-7],bit[33-63] */
}; /* PLID#1422 */

/** NPU Register payload TM_Sched_PortRRDWRRStatus45 (PLID#1378).
 *
 * Used by TM.Sched.PortRRDWRRStatus45.
 */
struct TM_Sched_PortRRDWRRStatus45 {
	uint64_t Status:20;         /**< byte[0-7],bit[0-19] */
	uint64_t _reserved_1:44;    /**< byte[0-7],bit[20-63] */
}; /* PLID#1378 */

/** NPU Register payload TM_Sched_QueueBank1EccErrStatus (PLID#1241).
 *
 * Used by TM.Sched.QueueBank1EccErrStatus.
 */
struct TM_Sched_QueueBank1EccErrStatus {
	uint64_t UncEccErr:5;       /**< byte[0-7],bit[0-4] */
	uint64_t _reserved_1:11;    /**< byte[0-7],bit[5-15] */
	uint64_t CorrEccErr:5;      /**< byte[0-7],bit[16-20] */
	uint64_t _reserved_2:43;    /**< byte[0-7],bit[21-63] */
}; /* PLID#1241 */

/** NPU Register payload TM_Drop_AlvlDropPrfWREDParams (PLID#1426).
 *
 * Used by TM.Drop.AlvlDropPrfWREDParams.
 */
struct TM_Drop_AlvlDropPrfWREDParams {
	uint64_t CurveIndexColor0:3;/**< byte[0-7],bit[0-2] */
	uint64_t _reserved_1:5;     /**< byte[0-7],bit[3-7] */
	uint64_t CurveIndexColor1:3;/**< byte[0-7],bit[8-10] */
	uint64_t _reserved_2:5;     /**< byte[0-7],bit[11-15] */
	uint64_t CurveIndexColor2:3;/**< byte[0-7],bit[16-18] */
	uint64_t _reserved_3:5;     /**< byte[0-7],bit[19-23] */
	uint64_t ScaleExpColor0:5;  /**< byte[0-7],bit[24-28] */
	uint64_t _reserved_4:3;     /**< byte[0-7],bit[29-31] */
	uint64_t ScaleExpColor1:5;  /**< byte[0-7],bit[32-36] */
	uint64_t _reserved_5:3;     /**< byte[0-7],bit[37-39] */
	uint64_t ScaleExpColor2:5;  /**< byte[0-7],bit[40-44] */
	uint64_t _reserved_6:3;     /**< byte[0-7],bit[45-47] */
	uint64_t ColorTDEn:1;       /**< byte[0-7],bit[48] */
	uint64_t _reserved_7:7;     /**< byte[0-7],bit[49-55] */
	uint64_t AQLExp:4;          /**< byte[0-7],bit[56-59] */
	uint64_t _reserved_8:4;     /**< byte[0-7],bit[60-63] */
}; /* PLID#1426 */

/** NPU Register payload TM_Sched_PortTokenBucketBurstSize (PLID#1397).
 *
 * Used by TM.Sched.PortTokenBucketBurstSize.
 */
struct TM_Sched_PortTokenBucketBurstSize {
	uint64_t MinBurstSz:17;     /**< byte[0-7],bit[0-16] */
	uint64_t _reserved_1:15;    /**< byte[0-7],bit[17-31] */
	uint64_t MaxBurstSz:17;     /**< byte[0-7],bit[32-48] */
	uint64_t _reserved_2:15;    /**< byte[0-7],bit[49-63] */
}; /* PLID#1397 */

/** NPU Register payload TM_Sched_BlvlPerRateShpPrms (PLID#1254).
 *
 * Used by TM.Sched.BlvlPerRateShpPrms.
 */
struct TM_Sched_BlvlPerRateShpPrms {
	uint64_t N:14;              /**< byte[0-7],bit[0-13] */
	uint64_t _reserved_1:2;     /**< byte[0-7],bit[14-15] */
	uint64_t K:14;              /**< byte[0-7],bit[16-29] */
	uint64_t _reserved_2:2;     /**< byte[0-7],bit[30-31] */
	uint64_t L:14;              /**< byte[0-7],bit[32-45] */
	uint64_t _reserved_3:18;    /**< byte[0-7],bit[46-63] */
}; /* PLID#1254 */

/** NPU Register payload TM_Sched_BlvlDef (PLID#1414).
 *
 * Used by TM.Sched.BlvlDef.
 */
struct TM_Sched_BlvlDef {
	uint64_t Deficit:22;        /**< byte[0-7],bit[0-21] */
	uint64_t _reserved_1:42;    /**< byte[0-7],bit[22-63] */
}; /* PLID#1414 */

/** NPU Register payload TM_Drop_AlvlDropPrfTailDrpThresh (PLID#1422).
 *
 * Used by TM.Drop.AlvlDropPrfTailDrpThresh.
 */
struct TM_Drop_AlvlDropPrfTailDrpThresh {
	uint64_t TailDropThresh:19; /**< byte[0-7],bit[0-18] */
	uint64_t _reserved_1:13;    /**< byte[0-7],bit[19-31] */
	uint64_t TailDropThreshRes:1;/**< byte[0-7],bit[32] */
	uint64_t _reserved_2:31;    /**< byte[0-7],bit[33-63] */
}; /* PLID#1422 */

/** NPU Register payload TM_Sched_ClvlRRDWRRStatus23 (PLID#1383).
 *
 * Used by TM.Sched.ClvlRRDWRRStatus23.
 */
struct TM_Sched_ClvlRRDWRRStatus23 {
	uint64_t Status:26;         /**< byte[0-7],bit[0-25] */
	uint64_t _reserved_1:38;    /**< byte[0-7],bit[26-63] */
}; /* PLID#1383 */

/** NPU Register payload TM_Sched_PortEligPrioFunc (PLID#1394).
 *
 * Used by TM.Sched.PortEligPrioFunc.
 */
struct TM_Sched_PortEligPrioFunc {
	uint64_t FuncOut0:9;        /**< byte[0-7],bit[0-8] */
	uint64_t _reserved_1:7;     /**< byte[0-7],bit[9-15] */
	uint64_t FuncOut1:9;        /**< byte[0-7],bit[16-24] */
	uint64_t _reserved_2:7;     /**< byte[0-7],bit[25-31] */
	uint64_t FuncOut2:9;        /**< byte[0-7],bit[32-40] */
	uint64_t _reserved_3:7;     /**< byte[0-7],bit[41-47] */
	uint64_t FuncOut3:9;        /**< byte[0-7],bit[48-56] */
	uint64_t _reserved_4:7;     /**< byte[0-7],bit[57-63] */
}; /* PLID#1394 */

/** NPU Register payload TM_Sched_ClvlMyQEccErrStatus (PLID#1242).
 *
 * Used by TM.Sched.ClvlMyQEccErrStatus.
 */
struct TM_Sched_ClvlMyQEccErrStatus {
	uint64_t UncEccErr:1;       /**< byte[0-7],bit[0] */
	uint64_t _reserved_1:15;    /**< byte[0-7],bit[1-15] */
	uint64_t CorrEccErr:1;      /**< byte[0-7],bit[16] */
	uint64_t _reserved_2:47;    /**< byte[0-7],bit[17-63] */
}; /* PLID#1242 */

/** NPU Register payload TM_Sched_ExcCnt (PLID#1180).
 *
 * Used by TM.Sched.ExcCnt.
 */
struct TM_Sched_ExcCnt {
	uint64_t Cnt:16;            /**< byte[0-7],bit[0-15] */
	uint64_t _reserved_1:48;    /**< byte[0-7],bit[16-63] */
}; /* PLID#1180 */

/** NPU Register payload TM_Sched_QueueL0ClusterStateHi (PLID#1417).
 *
 * Used by TM.Sched.QueueL0ClusterStateHi.
 */
struct TM_Sched_QueueL0ClusterStateHi {
	uint64_t status;            /**< byte[0-7] */
}; /* PLID#1417 */

/** NPU Register payload TM_Sched_AlvlRRDWRRStatus67 (PLID#1389).
 *
 * Used by TM.Sched.AlvlRRDWRRStatus67.
 */
struct TM_Sched_AlvlRRDWRRStatus67 {
	uint64_t Status:34;         /**< byte[0-7],bit[0-33] */
	uint64_t _reserved_1:30;    /**< byte[0-7],bit[34-63] */
}; /* PLID#1389 */

/** NPU Register payload TM_Sched_QueuePerRateShpPrms (PLID#1254).
 *
 * Used by TM.Sched.QueuePerRateShpPrms.
 */
struct TM_Sched_QueuePerRateShpPrms {
	uint64_t N:14;              /**< byte[0-7],bit[0-13] */
	uint64_t _reserved_1:2;     /**< byte[0-7],bit[14-15] */
	uint64_t K:14;              /**< byte[0-7],bit[16-29] */
	uint64_t _reserved_2:2;     /**< byte[0-7],bit[30-31] */
	uint64_t L:14;              /**< byte[0-7],bit[32-45] */
	uint64_t _reserved_3:18;    /**< byte[0-7],bit[46-63] */
}; /* PLID#1254 */

/** NPU Register payload TM_Sched_QueueWFS (PLID#1379).
 *
 * Used by TM.Sched.QueueWFS.
 */
struct TM_Sched_QueueWFS {
	uint64_t WFS:32;            /**< byte[0-7],bit[0-31] */
	uint64_t _reserved_1:32;    /**< byte[0-7],bit[32-63] */
}; /* PLID#1379 */

/** NPU Register payload TM_Sched_QueueDef (PLID#1414).
 *
 * Used by TM.Sched.QueueDef.
 */
struct TM_Sched_QueueDef {
	uint64_t Deficit:22;        /**< byte[0-7],bit[0-21] */
	uint64_t _reserved_1:42;    /**< byte[0-7],bit[22-63] */
}; /* PLID#1414 */

/** NPU Register payload TM_Drop_PortDropPrfTailDrpThresh_CoSRes (PLID#1422).
 *
 * Used by TM.Drop.PortDropPrfTailDrpThresh_CoSRes[0-7].
 */
struct TM_Drop_PortDropPrfTailDrpThresh_CoSRes {
	uint64_t TailDropThresh:19; /**< byte[0-7],bit[0-18] */
	uint64_t _reserved_1:13;    /**< byte[0-7],bit[19-31] */
	uint64_t TailDropThreshRes:1;/**< byte[0-7],bit[32] */
	uint64_t _reserved_2:31;    /**< byte[0-7],bit[33-63] */
}; /* PLID#1422 */

/** NPU Register payload TM_Drop_WREDDropProbMode (PLID#1262).
 *
 * Used by TM.Drop.WREDDropProbMode.
 */
struct TM_Drop_WREDDropProbMode {
	uint64_t Queue:1;           /**< byte[0-7],bit[0] */
	uint64_t _reserved_1:7;     /**< byte[0-7],bit[1-7] */
	uint64_t Alvl:1;            /**< byte[0-7],bit[8] */
	uint64_t _reserved_2:7;     /**< byte[0-7],bit[9-15] */
	uint64_t Blvl:1;            /**< byte[0-7],bit[16] */
	uint64_t _reserved_3:7;     /**< byte[0-7],bit[17-23] */
	uint64_t Clvl:1;            /**< byte[0-7],bit[24] */
	uint64_t _reserved_4:7;     /**< byte[0-7],bit[25-31] */
	uint64_t Port:1;            /**< byte[0-7],bit[32] */
	uint64_t _reserved_5:31;    /**< byte[0-7],bit[33-63] */
}; /* PLID#1262 */

/** NPU Register payload TM_Sched_GeneralEccErrStatus (PLID#1235).
 *
 * Used by TM.Sched.GeneralEccErrStatus.
 */
struct TM_Sched_GeneralEccErrStatus {
	uint64_t UncEccErr:6;       /**< byte[0-7],bit[0-5] */
	uint64_t _reserved_1:10;    /**< byte[0-7],bit[6-15] */
	uint64_t CorrEccErr:6;      /**< byte[0-7],bit[16-21] */
	uint64_t _reserved_2:42;    /**< byte[0-7],bit[22-63] */
}; /* PLID#1235 */

/** NPU Register payload TM_Drop_PortDropPrfWREDMinThresh_CoSRes (PLID#1420).
 *
 * Used by TM.Drop.PortDropPrfWREDMinThresh_CoSRes[0-7].
 */
struct TM_Drop_PortDropPrfWREDMinThresh_CoSRes {
	uint64_t MinTHColor0:10;    /**< byte[0-7],bit[0-9] */
	uint64_t _reserved_1:6;     /**< byte[0-7],bit[10-15] */
	uint64_t MinTHColor1:10;    /**< byte[0-7],bit[16-25] */
	uint64_t _reserved_2:6;     /**< byte[0-7],bit[26-31] */
	uint64_t MinTHColor2:10;    /**< byte[0-7],bit[32-41] */
	uint64_t _reserved_3:22;    /**< byte[0-7],bit[42-63] */
}; /* PLID#1420 */

/** NPU Register payload TM_Sched_PortRangeMap (PLID#1401).
 *
 * Used by TM.Sched.PortRangeMap.
 */
struct TM_Sched_PortRangeMap {
	uint64_t Lo:4;              /**< byte[0-7],bit[0-3] */
	uint64_t _reserved_1:20;    /**< byte[0-7],bit[4-23] */
	uint64_t Hi:4;              /**< byte[0-7],bit[24-27] */
	uint64_t _reserved_2:36;    /**< byte[0-7],bit[28-63] */
}; /* PLID#1401 */

/** NPU Register payload TM_Sched_BlvlL1ClusterStateLo (PLID#1387).
 *
 * Used by TM.Sched.BlvlL1ClusterStateLo.
 */
struct TM_Sched_BlvlL1ClusterStateLo {
	uint64_t Status:42;         /**< byte[0-7],bit[0-41] */
	uint64_t _reserved_1:22;    /**< byte[0-7],bit[42-63] */
}; /* PLID#1387 */

/** NPU Register payload TM_Sched_ClvlPerConf (PLID#1256).
 *
 * Used by TM.Sched.ClvlPerConf.
 */
struct TM_Sched_ClvlPerConf {
	uint64_t PerEn:1;           /**< byte[0-7],bit[0] */
	uint64_t _reserved_1:15;    /**< byte[0-7],bit[1-15] */
	uint64_t PerInterval:12;    /**< byte[0-7],bit[16-27] */
	uint64_t _reserved_2:20;    /**< byte[0-7],bit[28-47] */
	uint64_t DecEn:1;           /**< byte[0-7],bit[48] */
	uint64_t _reserved_3:15;    /**< byte[0-7],bit[49-63] */
}; /* PLID#1256 */

/** NPU Register payload TM_Drop_QueueDropProb (PLID#1430).
 *
 * Used by TM.Drop.QueueDropProb.
 */
struct TM_Drop_QueueDropProb {
	uint64_t DropProb:13;       /**< byte[0-7],bit[0-12] */
	uint64_t _reserved_1:51;    /**< byte[0-7],bit[13-63] */
}; /* PLID#1430 */

/** NPU Register payload TM_Sched_AlvlEligPrioFunc (PLID#1394).
 *
 * Used by TM.Sched.AlvlEligPrioFunc.
 */
struct TM_Sched_AlvlEligPrioFunc {
	uint64_t FuncOut0:9;        /**< byte[0-7],bit[0-8] */
	uint64_t _reserved_1:7;     /**< byte[0-7],bit[9-15] */
	uint64_t FuncOut1:9;        /**< byte[0-7],bit[16-24] */
	uint64_t _reserved_2:7;     /**< byte[0-7],bit[25-31] */
	uint64_t FuncOut2:9;        /**< byte[0-7],bit[32-40] */
	uint64_t _reserved_3:7;     /**< byte[0-7],bit[41-47] */
	uint64_t FuncOut3:9;        /**< byte[0-7],bit[48-56] */
	uint64_t _reserved_4:7;     /**< byte[0-7],bit[57-63] */
}; /* PLID#1394 */

/** NPU Register payload TM_Drop_PortREDCurve_CoS (PLID#1423).
 *
 * Used by TM.Drop.PortREDCurve_CoS[0-7].
 */
struct TM_Drop_PortREDCurve_CoS {
	uint64_t Prob:6;            /**< byte[0-7],bit[0-5] */
	uint64_t _reserved_1:58;    /**< byte[0-7],bit[6-63] */
}; /* PLID#1423 */

/** NPU Register payload TM_Sched_PortExtBPEn (PLID#1189).
 *
 * Used by TM.Sched.PortExtBPEn.
 */
struct TM_Sched_PortExtBPEn {
	uint64_t En:1;              /**< byte[0-7],bit[0] */
	uint64_t _reserved_1:63;    /**< byte[0-7],bit[1-63] */
}; /* PLID#1189 */

/** NPU Register payload TM_Sched_AlvlL0ClusterStateHi (PLID#1417).
 *
 * Used by TM.Sched.AlvlL0ClusterStateHi.
 */
struct TM_Sched_AlvlL0ClusterStateHi {
	uint64_t status;            /**< byte[0-7] */
}; /* PLID#1417 */

/** NPU Register payload TM_Sched_BlvlQuantum (PLID#1404).
 *
 * Used by TM.Sched.BlvlQuantum.
 */
struct TM_Sched_BlvlQuantum {
	uint64_t Quantum:14;        /**< byte[0-7],bit[0-13] */
	uint64_t _reserved_1:50;    /**< byte[0-7],bit[14-63] */
}; /* PLID#1404 */

/** NPU Register payload TM_Sched_Id (PLID#1249).
 *
 * Used by TM.Sched.Id.
 */
struct TM_Sched_Id {
	uint64_t UID:8;             /**< byte[0-7],bit[0-7] */
	uint64_t SUID:8;            /**< byte[0-7],bit[8-15] */
	uint64_t _reserved_1:48;    /**< byte[0-7],bit[16-63] */
}; /* PLID#1249 */

/** NPU Register payload TM_Sched_PortRRDWRRStatus67 (PLID#1378).
 *
 * Used by TM.Sched.PortRRDWRRStatus67.
 */
struct TM_Sched_PortRRDWRRStatus67 {
	uint64_t Status:20;         /**< byte[0-7],bit[0-19] */
	uint64_t _reserved_1:44;    /**< byte[0-7],bit[20-63] */
}; /* PLID#1378 */

/** NPU Register payload TM_Drop_QueueDropPrfTailDrpThresh (PLID#1422).
 *
 * Used by TM.Drop.QueueDropPrfTailDrpThresh.
 */
struct TM_Drop_QueueDropPrfTailDrpThresh {
	uint64_t TailDropThresh:19; /**< byte[0-7],bit[0-18] */
	uint64_t _reserved_1:13;    /**< byte[0-7],bit[19-31] */
	uint64_t TailDropThreshRes:1;/**< byte[0-7],bit[32] */
	uint64_t _reserved_2:31;    /**< byte[0-7],bit[33-63] */
}; /* PLID#1422 */

/** NPU Register payload TM_Sched_ClvlBPFromSTF (PLID#1380).
 *
 * Used by TM.Sched.ClvlBPFromSTF.
 */
struct TM_Sched_ClvlBPFromSTF {
	uint64_t BP;                /**< byte[0-7] */
}; /* PLID#1380 */

/** NPU Register payload TM_Sched_BlvlEligPrioFuncPtr (PLID#1395).
 *
 * Used by TM.Sched.BlvlEligPrioFuncPtr.
 */
struct TM_Sched_BlvlEligPrioFuncPtr {
	uint64_t Ptr:6;             /**< byte[0-7],bit[0-5] */
	uint64_t _reserved_1:58;    /**< byte[0-7],bit[6-63] */
}; /* PLID#1395 */

/** NPU Register payload TM_Drop_ErrCnt (PLID#1180).
 *
 * Used by TM.Drop.ErrCnt.
 */
struct TM_Drop_ErrCnt {
	uint64_t Cnt:16;            /**< byte[0-7],bit[0-15] */
	uint64_t _reserved_1:48;    /**< byte[0-7],bit[16-63] */
}; /* PLID#1180 */

/** NPU Register payload TM_Sched_AlvlPerRateShpPrms (PLID#1254).
 *
 * Used by TM.Sched.AlvlPerRateShpPrms.
 */
struct TM_Sched_AlvlPerRateShpPrms {
	uint64_t N:14;              /**< byte[0-7],bit[0-13] */
	uint64_t _reserved_1:2;     /**< byte[0-7],bit[14-15] */
	uint64_t K:14;              /**< byte[0-7],bit[16-29] */
	uint64_t _reserved_2:2;     /**< byte[0-7],bit[30-31] */
	uint64_t L:14;              /**< byte[0-7],bit[32-45] */
	uint64_t _reserved_3:18;    /**< byte[0-7],bit[46-63] */
}; /* PLID#1254 */

/** NPU Register payload TM_Sched_QueuePerConf (PLID#1257).
 *
 * Used by TM.Sched.QueuePerConf.
 */
struct TM_Sched_QueuePerConf {
	uint64_t PerEn:1;           /**< byte[0-7],bit[0] */
	uint64_t _reserved_1:15;    /**< byte[0-7],bit[1-15] */
	uint64_t PerInterval:12;    /**< byte[0-7],bit[16-27] */
	uint64_t _reserved_2:20;    /**< byte[0-7],bit[28-47] */
	uint64_t DecEn:1;           /**< byte[0-7],bit[48] */
	uint64_t _reserved_3:15;    /**< byte[0-7],bit[49-63] */
}; /* PLID#1257 */

/** NPU Register payload TM_Drop_AlvlDropPrfWREDDPRatio (PLID#1421).
 *
 * Used by TM.Drop.AlvlDropPrfWREDDPRatio.
 */
struct TM_Drop_AlvlDropPrfWREDDPRatio {
	uint64_t DPRatio0:6;        /**< byte[0-7],bit[0-5] */
	uint64_t _reserved_1:2;     /**< byte[0-7],bit[6-7] */
	uint64_t DPRatio1:6;        /**< byte[0-7],bit[8-13] */
	uint64_t _reserved_2:2;     /**< byte[0-7],bit[14-15] */
	uint64_t DPRatio2:4;        /**< byte[0-7],bit[16-19] */
	uint64_t _reserved_3:44;    /**< byte[0-7],bit[20-63] */
}; /* PLID#1421 */

/** NPU Register payload TM_Sched_AlvlQuantum (PLID#1404).
 *
 * Used by TM.Sched.AlvlQuantum.
 */
struct TM_Sched_AlvlQuantum {
	uint64_t Quantum:14;        /**< byte[0-7],bit[0-13] */
	uint64_t _reserved_1:50;    /**< byte[0-7],bit[14-63] */
}; /* PLID#1404 */

/** NPU Register payload TM_Drop_QueueAvgQueueLength (PLID#1431).
 *
 * Used by TM.Drop.QueueAvgQueueLength.
 */
struct TM_Drop_QueueAvgQueueLength {
	uint64_t AQL:29;            /**< byte[0-7],bit[0-28] */
	uint64_t _reserved_1:35;    /**< byte[0-7],bit[29-63] */
}; /* PLID#1431 */

/** NPU Register payload TM_Sched_BLevelShaperBucketNeg (PLID#1416).
 *
 * Used by TM.Sched.BLevelShaperBucketNeg.
 */
struct TM_Sched_BLevelShaperBucketNeg {
	uint64_t MinTBNeg:32;       /**< byte[0-7],bit[0-31] */
	uint64_t MaxTBNeg:32;       /**< byte[0-7],bit[32-63] */
}; /* PLID#1416 */

/** NPU Register payload TM_Drop_ForceErr (PLID#1183).
 *
 * Used by TM.Drop.ForceErr.
 */
struct TM_Drop_ForceErr {
	uint64_t ForcedErr:1;       /**< byte[0-7],bit[0] */
	uint64_t _reserved_1:63;    /**< byte[0-7],bit[1-63] */
}; /* PLID#1183 */

/** NPU Register payload TM_Sched_PortDWRRBytesPerBurstsLimit (PLID#1255).
 *
 * Used by TM.Sched.PortDWRRBytesPerBurstsLimit.
 */
struct TM_Sched_PortDWRRBytesPerBurstsLimit {
	uint64_t limit:7;           /**< byte[0-7],bit[0-6] */
	uint64_t _reserved_1:57;    /**< byte[0-7],bit[7-63] */
}; /* PLID#1255 */

/** NPU Register payload TM_Sched_EccConfig (PLID#1258).
 *
 * Used by TM.Sched.EccConfig.
 */
struct TM_Sched_EccConfig {
	uint64_t LockFirstErronousEvent:1;/**< byte[0-7],bit[0] */
	uint64_t ErrInsMode:1;      /**< byte[0-7],bit[1] */
	uint64_t QtoAErrInsEnable:1;/**< byte[0-7],bit[2] */
	uint64_t AIDErrInsEnable:1; /**< byte[0-7],bit[3] */
	uint64_t CPerConfErrInsEnable:1;/**< byte[0-7],bit[4] */
	uint64_t BPerConfErrInsEnable:1;/**< byte[0-7],bit[5] */
	uint64_t CTBErrInsEnable:1; /**< byte[0-7],bit[6] */
	uint64_t BTBErrInsEnable:1; /**< byte[0-7],bit[7] */
	uint64_t CTBNegErrInsEnable:1;/**< byte[0-7],bit[8] */
	uint64_t CTBNeg2ErrInsEnable:1;/**< byte[0-7],bit[9] */
	uint64_t BTBNegErrInsEnable:1;/**< byte[0-7],bit[10] */
	uint64_t BTBNeg2ErrInsEnable:1;/**< byte[0-7],bit[11] */
	uint64_t ATBNegErrInsEnable:1;/**< byte[0-7],bit[12] */
	uint64_t ATBNeg2ErrInsEnable:1;/**< byte[0-7],bit[13] */
	uint64_t QTBNegErrInsEnable:4;/**< byte[0-7],bit[14-17] */
	uint64_t QTBNeg2ErrInsEnable:4;/**< byte[0-7],bit[18-21] */
	uint64_t QWFSErrInsEnable:4;/**< byte[0-7],bit[22-25] */
	uint64_t AWFSErrInsEnable:1;/**< byte[0-7],bit[26] */
	uint64_t BWFSErrInsEnable:1;/**< byte[0-7],bit[27] */
	uint64_t CMyQErrInsEnable:1;/**< byte[0-7],bit[28] */
	uint64_t BMyQErrInsEnable:1;/**< byte[0-7],bit[29] */
	uint64_t AMyQErrInsEnable:1;/**< byte[0-7],bit[30] */
	uint64_t PDWRRErrInsEnable:1;/**< byte[0-7],bit[31] */
	uint64_t CDWRRErrInsEnable:1;/**< byte[0-7],bit[32] */
	uint64_t BDWRRErrInsEnable:1;/**< byte[0-7],bit[33] */
	uint64_t ADWRRErrInsEnable:1;/**< byte[0-7],bit[34] */
	uint64_t CParentErrInsEnable:1;/**< byte[0-7],bit[35] */
	uint64_t BParentErrInsEnable:1;/**< byte[0-7],bit[36] */
	uint64_t AParentErrInsEnable:1;/**< byte[0-7],bit[37] */
	uint64_t CLastErrInsEnable:1;/**< byte[0-7],bit[38] */
	uint64_t BLastErrInsEnable:1;/**< byte[0-7],bit[39] */
	uint64_t ALastErrInsEnable:1;/**< byte[0-7],bit[40] */
	uint64_t QLastErrInsEnable:1;/**< byte[0-7],bit[41] */
	uint64_t QFuncErrInsEnable:1;/**< byte[0-7],bit[42] */
	uint64_t AStateErrInsEnable:1;/**< byte[0-7],bit[43] */
	uint64_t QStateErrInsEnable:1;/**< byte[0-7],bit[44] */
	uint64_t BGrandParentErrInsEnable:1;/**< byte[0-7],bit[45] */
	uint64_t AGrandParentErrInsEnable:1;/**< byte[0-7],bit[46] */
	uint64_t QGrandParentErrInsEnable:1;/**< byte[0-7],bit[47] */
	uint64_t _reserved_1:16;    /**< byte[0-7],bit[48-63] */
}; /* PLID#1258 */

/** NPU Register payload TM_Sched_QueueBank0EccErrStatus (PLID#1241).
 *
 * Used by TM.Sched.QueueBank0EccErrStatus.
 */
struct TM_Sched_QueueBank0EccErrStatus {
	uint64_t UncEccErr:5;       /**< byte[0-7],bit[0-4] */
	uint64_t _reserved_1:11;    /**< byte[0-7],bit[5-15] */
	uint64_t CorrEccErr:5;      /**< byte[0-7],bit[16-20] */
	uint64_t _reserved_2:43;    /**< byte[0-7],bit[21-63] */
}; /* PLID#1241 */

/** NPU Register payload TM_Drop_QueueDropProfPtr (PLID#1427).
 *
 * Used by TM.Drop.QueueDropProfPtr.
 */
struct TM_Drop_QueueDropProfPtr {
	uint64_t ProfPtr0:12;       /**< byte[0-7],bit[0-11] */
	uint64_t _reserved_1:4;     /**< byte[0-7],bit[12-15] */
	uint64_t ProfPtr1:12;       /**< byte[0-7],bit[16-27] */
	uint64_t _reserved_2:4;     /**< byte[0-7],bit[28-31] */
	uint64_t ProfPtr2:12;       /**< byte[0-7],bit[32-43] */
	uint64_t _reserved_3:4;     /**< byte[0-7],bit[44-47] */
	uint64_t ProfPtr3:12;       /**< byte[0-7],bit[48-59] */
	uint64_t _reserved_4:4;     /**< byte[0-7],bit[60-63] */
}; /* PLID#1427 */

/** NPU Register payload TM_Drop_AlvlDropProb (PLID#1430).
 *
 * Used by TM.Drop.AlvlDropProb.
 */
struct TM_Drop_AlvlDropProb {
	uint64_t DropProb:13;       /**< byte[0-7],bit[0-12] */
	uint64_t _reserved_1:51;    /**< byte[0-7],bit[13-63] */
}; /* PLID#1430 */

/** NPU Register payload TM_Drop_QueueDropPrfWREDMinThresh (PLID#1420).
 *
 * Used by TM.Drop.QueueDropPrfWREDMinThresh.
 */
struct TM_Drop_QueueDropPrfWREDMinThresh {
	uint64_t MinTHColor0:10;    /**< byte[0-7],bit[0-9] */
	uint64_t _reserved_1:6;     /**< byte[0-7],bit[10-15] */
	uint64_t MinTHColor1:10;    /**< byte[0-7],bit[16-25] */
	uint64_t _reserved_2:6;     /**< byte[0-7],bit[26-31] */
	uint64_t MinTHColor2:10;    /**< byte[0-7],bit[32-41] */
	uint64_t _reserved_3:22;    /**< byte[0-7],bit[42-63] */
}; /* PLID#1420 */

/** NPU Register payload TM_Sched_ALvltoBlvlAndQueueRangeMap (PLID#1407).
 *
 * Used by TM.Sched.ALvltoBlvlAndQueueRangeMap.
 */
struct TM_Sched_ALvltoBlvlAndQueueRangeMap {
	uint64_t QueueLo:9;         /**< byte[0-7],bit[0-8] */
	uint64_t _reserved_1:15;    /**< byte[0-7],bit[9-23] */
	uint64_t QueueHi:9;         /**< byte[0-7],bit[24-32] */
	uint64_t _reserved_2:15;    /**< byte[0-7],bit[33-47] */
	uint64_t Blvl:5;            /**< byte[0-7],bit[48-52] */
	uint64_t _reserved_3:11;    /**< byte[0-7],bit[53-63] */
}; /* PLID#1407 */

/** NPU Register payload TM_Sched_QueueAMap (PLID#1410).
 *
 * Used by TM.Sched.QueueAMap.
 */
struct TM_Sched_QueueAMap {
	uint64_t Alvl:7;            /**< byte[0-7],bit[0-6] */
	uint64_t _reserved_1:57;    /**< byte[0-7],bit[7-63] */
}; /* PLID#1410 */

/** NPU Register payload TM_Sched_BlvlBankEccErrStatus (PLID#1241).
 *
 * Used by TM.Sched.BlvlBankEccErrStatus.
 */
struct TM_Sched_BlvlBankEccErrStatus {
	uint64_t UncEccErr:5;       /**< byte[0-7],bit[0-4] */
	uint64_t _reserved_1:11;    /**< byte[0-7],bit[5-15] */
	uint64_t CorrEccErr:5;      /**< byte[0-7],bit[16-20] */
	uint64_t _reserved_2:43;    /**< byte[0-7],bit[21-63] */
}; /* PLID#1241 */

/** NPU Register payload TM_Sched_QueueL2ClusterStateLo (PLID#1392).
 *
 * Used by TM.Sched.QueueL2ClusterStateLo.
 */
struct TM_Sched_QueueL2ClusterStateLo {
	uint64_t Status:48;         /**< byte[0-7],bit[0-47] */
	uint64_t _reserved_1:16;    /**< byte[0-7],bit[48-63] */
}; /* PLID#1392 */

/** NPU Register payload TM_Drop_AlvlDropPrfWREDScaleRatio (PLID#1419).
 *
 * Used by TM.Drop.AlvlDropPrfWREDScaleRatio.
 */
struct TM_Drop_AlvlDropPrfWREDScaleRatio {
	uint64_t ScaleRatioColor0:10;/**< byte[0-7],bit[0-9] */
	uint64_t _reserved_1:6;     /**< byte[0-7],bit[10-15] */
	uint64_t ScaleRatioColor1:10;/**< byte[0-7],bit[16-25] */
	uint64_t _reserved_2:6;     /**< byte[0-7],bit[26-31] */
	uint64_t ScaleRatioColor2:10;/**< byte[0-7],bit[32-41] */
	uint64_t _reserved_3:22;    /**< byte[0-7],bit[42-63] */
}; /* PLID#1419 */

/** NPU Register payload TM_Sched_BlvlShpBucketLvls (PLID#1413).
 *
 * Used by TM.Sched.BlvlShpBucketLvls.
 */
struct TM_Sched_BlvlShpBucketLvls {
	uint64_t MinLvl:23;         /**< byte[0-7],bit[0-22] */
	uint64_t _reserved_1:9;     /**< byte[0-7],bit[23-31] */
	uint64_t MaxLvl:23;         /**< byte[0-7],bit[32-54] */
	uint64_t _reserved_2:9;     /**< byte[0-7],bit[55-63] */
}; /* PLID#1413 */

/** NPU Register payload TM_Drop_ClvlDropPrfTailDrpThresh_CoS (PLID#1422).
 *
 * Used by TM.Drop.ClvlDropPrfTailDrpThresh.CoS[0-7].
 */
struct TM_Drop_ClvlDropPrfTailDrpThresh_CoS {
	uint64_t TailDropThresh:19; /**< byte[0-7],bit[0-18] */
	uint64_t _reserved_1:13;    /**< byte[0-7],bit[19-31] */
	uint64_t TailDropThreshRes:1;/**< byte[0-7],bit[32] */
	uint64_t _reserved_2:31;    /**< byte[0-7],bit[33-63] */
}; /* PLID#1422 */

/** NPU Register payload TM_Drop_ErrStus (PLID#1259).
 *
 * Used by TM.Drop.ErrStus.
 */
struct TM_Drop_ErrStus {
	uint64_t ForcedErr:1;       /**< byte[0-7],bit[0] */
	uint64_t CorrECCErr:1;      /**< byte[0-7],bit[1] */
	uint64_t UncECCErr:1;       /**< byte[0-7],bit[2] */
	uint64_t QuesryRespSyncFifoFull:1;/**< byte[0-7],bit[3] */
	uint64_t QueryReqFifoOverflow:1;/**< byte[0-7],bit[4] */
	uint64_t AgingFifoOverflow:1;/**< byte[0-7],bit[5] */
	uint64_t _reserved_1:58;    /**< byte[0-7],bit[6-63] */
}; /* PLID#1259 */

/** NPU Register payload TM_Sched_AlvlPerConf (PLID#1257).
 *
 * Used by TM.Sched.AlvlPerConf.
 */
struct TM_Sched_AlvlPerConf {
	uint64_t PerEn:1;           /**< byte[0-7],bit[0] */
	uint64_t _reserved_1:15;    /**< byte[0-7],bit[1-15] */
	uint64_t PerInterval:12;    /**< byte[0-7],bit[16-27] */
	uint64_t _reserved_2:20;    /**< byte[0-7],bit[28-47] */
	uint64_t DecEn:1;           /**< byte[0-7],bit[48] */
	uint64_t _reserved_3:15;    /**< byte[0-7],bit[49-63] */
}; /* PLID#1257 */

/** NPU Register payload TM_Sched_ClvlL0ClusterStateLo (PLID#1384).
 *
 * Used by TM.Sched.ClvlL0ClusterStateLo.
 */
struct TM_Sched_ClvlL0ClusterStateLo {
	uint64_t Status;            /**< byte[0-7] */
}; /* PLID#1384 */

/** NPU Register payload TM_Sched_AlvlL1ClusterStateLo (PLID#1390).
 *
 * Used by TM.Sched.AlvlL1ClusterStateLo.
 */
struct TM_Sched_AlvlL1ClusterStateLo {
	uint64_t Status:40;         /**< byte[0-7],bit[0-39] */
	uint64_t _reserved_1:24;    /**< byte[0-7],bit[40-63] */
}; /* PLID#1390 */

/** NPU Register payload TM_Sched_QueueBank2EccErrStatus (PLID#1241).
 *
 * Used by TM.Sched.QueueBank2EccErrStatus.
 */
struct TM_Sched_QueueBank2EccErrStatus {
	uint64_t UncEccErr:5;       /**< byte[0-7],bit[0-4] */
	uint64_t _reserved_1:11;    /**< byte[0-7],bit[5-15] */
	uint64_t CorrEccErr:5;      /**< byte[0-7],bit[16-20] */
	uint64_t _reserved_2:43;    /**< byte[0-7],bit[21-63] */
}; /* PLID#1241 */

/** NPU Register payload TM_Sched_ClvlPerStatus (PLID#1244).
 *
 * Used by TM.Sched.ClvlPerStatus.
 */
struct TM_Sched_ClvlPerStatus {
	uint64_t PerRoundCnt:10;    /**< byte[0-7],bit[0-9] */
	uint64_t _reserved_1:22;    /**< byte[0-7],bit[10-31] */
	uint64_t PerPointer:9;      /**< byte[0-7],bit[32-40] */
	uint64_t _reserved_2:23;    /**< byte[0-7],bit[41-63] */
}; /* PLID#1244 */

/** NPU Register payload TM_Sched_PortPerRateShpPrms (PLID#1254).
 *
 * Used by TM.Sched.PortPerRateShpPrms.
 */
struct TM_Sched_PortPerRateShpPrms {
	uint64_t N:14;              /**< byte[0-7],bit[0-13] */
	uint64_t _reserved_1:2;     /**< byte[0-7],bit[14-15] */
	uint64_t K:14;              /**< byte[0-7],bit[16-29] */
	uint64_t _reserved_2:2;     /**< byte[0-7],bit[30-31] */
	uint64_t L:14;              /**< byte[0-7],bit[32-45] */
	uint64_t _reserved_3:18;    /**< byte[0-7],bit[46-63] */
}; /* PLID#1254 */

/** NPU Register payload TM_Sched_PortRRDWRRStatus01 (PLID#1378).
 *
 * Used by TM.Sched.PortRRDWRRStatus01.
 */
struct TM_Sched_PortRRDWRRStatus01 {
	uint64_t Status:20;         /**< byte[0-7],bit[0-19] */
	uint64_t _reserved_1:44;    /**< byte[0-7],bit[20-63] */
}; /* PLID#1378 */

/** NPU Register payload TM_Sched_BlvlPerConf (PLID#1257).
 *
 * Used by TM.Sched.BlvlPerConf.
 */
struct TM_Sched_BlvlPerConf {
	uint64_t PerEn:1;           /**< byte[0-7],bit[0] */
	uint64_t _reserved_1:15;    /**< byte[0-7],bit[1-15] */
	uint64_t PerInterval:12;    /**< byte[0-7],bit[16-27] */
	uint64_t _reserved_2:20;    /**< byte[0-7],bit[28-47] */
	uint64_t DecEn:1;           /**< byte[0-7],bit[48] */
	uint64_t _reserved_3:15;    /**< byte[0-7],bit[49-63] */
}; /* PLID#1257 */

/** NPU Register payload TM_Sched_BlvlNodeState (PLID#1381).
 *
 * Used by TM.Sched.BlvlNodeState.
 */
struct TM_Sched_BlvlNodeState {
	uint64_t State:11;          /**< byte[0-7],bit[0-10] */
	uint64_t _reserved_1:53;    /**< byte[0-7],bit[11-63] */
}; /* PLID#1381 */

/** NPU Register payload TM_Sched_AlvlL1ClusterStateHi (PLID#1390).
 *
 * Used by TM.Sched.AlvlL1ClusterStateHi.
 */
struct TM_Sched_AlvlL1ClusterStateHi {
	uint64_t Status:40;         /**< byte[0-7],bit[0-39] */
	uint64_t _reserved_1:24;    /**< byte[0-7],bit[40-63] */
}; /* PLID#1390 */

/** NPU Register payload TM_Sched_BlvlPerRateShpPrmsInt (PLID#1238).
 *
 * Used by TM.Sched.BlvlPerRateShpPrmsInt.
 */
struct TM_Sched_BlvlPerRateShpPrmsInt {
	uint64_t B:3;               /**< byte[0-7],bit[0-2] */
	uint64_t _reserved_1:29;    /**< byte[0-7],bit[3-31] */
	uint64_t I:3;               /**< byte[0-7],bit[32-34] */
	uint64_t _reserved_2:29;    /**< byte[0-7],bit[35-63] */
}; /* PLID#1238 */

/** NPU Register payload TM_Sched_PortQuantumsPriosHi (PLID#1400).
 *
 * Used by TM.Sched.PortQuantumsPriosHi.
 */
struct TM_Sched_PortQuantumsPriosHi {
	uint64_t Quantum4:9;        /**< byte[0-7],bit[0-8] */
	uint64_t _reserved_1:7;     /**< byte[0-7],bit[9-15] */
	uint64_t Quantum5:9;        /**< byte[0-7],bit[16-24] */
	uint64_t _reserved_2:7;     /**< byte[0-7],bit[25-31] */
	uint64_t Quantum6:9;        /**< byte[0-7],bit[32-40] */
	uint64_t _reserved_3:7;     /**< byte[0-7],bit[41-47] */
	uint64_t Quantum7:9;        /**< byte[0-7],bit[48-56] */
	uint64_t _reserved_4:7;     /**< byte[0-7],bit[57-63] */
}; /* PLID#1400 */

/** NPU Register payload TM_Sched_AlvlL2ClusterStateHi (PLID#1391).
 *
 * Used by TM.Sched.AlvlL2ClusterStateHi.
 */
struct TM_Sched_AlvlL2ClusterStateHi {
	uint64_t Status:46;         /**< byte[0-7],bit[0-45] */
	uint64_t _reserved_1:18;    /**< byte[0-7],bit[46-63] */
}; /* PLID#1391 */

/** NPU Register payload TM_Drop_AlvlInstAndAvgQueueLength (PLID#1429).
 *
 * Used by TM.Drop.AlvlInstAndAvgQueueLength.
 */
struct TM_Drop_AlvlInstAndAvgQueueLength {
	uint64_t QL:29;             /**< byte[0-7],bit[0-28] */
	uint64_t _reserved_1:3;     /**< byte[0-7],bit[29-31] */
	uint64_t AQL:29;            /**< byte[0-7],bit[32-60] */
	uint64_t _reserved_2:3;     /**< byte[0-7],bit[61-63] */
}; /* PLID#1429 */

/** NPU Register payload TM_Sched_BlvlTokenBucketBurstSize (PLID#1403).
 *
 * Used by TM.Sched.BlvlTokenBucketBurstSize.
 */
struct TM_Sched_BlvlTokenBucketBurstSize {
	uint64_t MinBurstSz:12;     /**< byte[0-7],bit[0-11] */
	uint64_t _reserved_1:20;    /**< byte[0-7],bit[12-31] */
	uint64_t MaxBurstSz:12;     /**< byte[0-7],bit[32-43] */
	uint64_t _reserved_2:20;    /**< byte[0-7],bit[44-63] */
}; /* PLID#1403 */

/** NPU Register payload TM_Drop_AlvlREDCurve_Color (PLID#1423).
 *
 * Used by TM.Drop.AlvlREDCurve.Color[0-2].
 */
struct TM_Drop_AlvlREDCurve_Color {
	uint64_t Prob:6;            /**< byte[0-7],bit[0-5] */
	uint64_t _reserved_1:58;    /**< byte[0-7],bit[6-63] */
}; /* PLID#1423 */

/** NPU Register payload TM_Drop_EccConfig (PLID#1270).
 *
 * Used by TM.Drop.EccConfig.
 */
struct TM_Drop_EccConfig {
	uint64_t LockFirstErronousEvent:1;/**< byte[0-7],bit[0] */
	uint64_t ErrInsMode:1;      /**< byte[0-7],bit[1] */
	uint64_t QAqlErrInsEnable:1;/**< byte[0-7],bit[2] */
	uint64_t AAqlErrInsEnable:1;/**< byte[0-7],bit[3] */
	uint64_t BAqlErrInsEnable:1;/**< byte[0-7],bit[4] */
	uint64_t CAqlErrInsEnable:1;/**< byte[0-7],bit[5] */
	uint64_t PAqlErrInsEnable:1;/**< byte[0-7],bit[6] */
	uint64_t PAqlCoSErrInsEnable:1;/**< byte[0-7],bit[7] */
	uint64_t QCoSErrInsEnable:1;/**< byte[0-7],bit[8] */
	uint64_t AProfPtrErrInsEnable:1;/**< byte[0-7],bit[9] */
	uint64_t BProfPtrErrInsEnable:1;/**< byte[0-7],bit[10] */
	uint64_t CProfPtrErrInsEnable:8;/**< byte[0-7],bit[11-18] */
	uint64_t QCurveErrInsEnable:3;/**< byte[0-7],bit[19-21] */
	uint64_t ACurveErrInsEnable:3;/**< byte[0-7],bit[22-24] */
	uint64_t QProfErrInsEnable:1;/**< byte[0-7],bit[25] */
	uint64_t AProfErrInsEnable:1;/**< byte[0-7],bit[26] */
	uint64_t BProfErrInsEnable:1;/**< byte[0-7],bit[27] */
	uint64_t CProfErrInsEnable:8;/**< byte[0-7],bit[28-35] */
	uint64_t PProfErrInsEnable:8;/**< byte[0-7],bit[36-43] */
	uint64_t PGProfErrInsEnable:1;/**< byte[0-7],bit[44] */
	uint64_t _reserved_1:19;    /**< byte[0-7],bit[45-63] */
}; /* PLID#1270 */

/** NPU Register payload TM_Drop_ClvlDropProfPtr_CoS (PLID#1424).
 *
 * Used by TM.Drop.ClvlDropProfPtr_CoS[0-7].
 */
struct TM_Drop_ClvlDropProfPtr_CoS {
	uint64_t ProfPtr0:1;        /**< byte[0-7],bit[0-0] */
	uint64_t _reserved_1:7;     /**< byte[0-7],bit[1-7] */
	uint64_t ProfPtr1:1;        /**< byte[0-7],bit[8-8] */
	uint64_t _reserved_2:7;     /**< byte[0-7],bit[9-15] */
	uint64_t ProfPtr2:1;        /**< byte[0-7],bit[16-16] */
	uint64_t _reserved_3:7;     /**< byte[0-7],bit[17-23] */
	uint64_t ProfPtr3:1;        /**< byte[0-7],bit[24-24] */
	uint64_t _reserved_4:7;     /**< byte[0-7],bit[25-31] */
	uint64_t ProfPtr4:1;        /**< byte[0-7],bit[32-32] */
	uint64_t _reserved_5:7;     /**< byte[0-7],bit[33-39] */
	uint64_t ProfPtr5:1;        /**< byte[0-7],bit[40-40] */
	uint64_t _reserved_6:7;     /**< byte[0-7],bit[41-47] */
	uint64_t ProfPtr6:1;        /**< byte[0-7],bit[48-48] */
	uint64_t _reserved_7:7;     /**< byte[0-7],bit[49-55] */
	uint64_t ProfPtr7:1;        /**< byte[0-7],bit[56-56] */
	uint64_t _reserved_8:7;     /**< byte[0-7],bit[57-63] */
}; /* PLID#1424 */

/** NPU Register payload TM_Sched_PortShaperBucketNeg (PLID#1415).
 *
 * Used by TM.Sched.PortShaperBucketNeg.
 */
struct TM_Sched_PortShaperBucketNeg {
	uint64_t MinTBNeg:32;       /**< byte[0-7],bit[0-31] */
	uint64_t MaxNeg:32;         /**< byte[0-7],bit[32-63] */
}; /* PLID#1415 */

/** NPU Register payload TM_Drop_BlvlDropPrfWREDParams (PLID#1418).
 *
 * Used by TM.Drop.BlvlDropPrfWREDParams.
 */
struct TM_Drop_BlvlDropPrfWREDParams {
	uint64_t CurveIndexColor0:2;/**< byte[0-7],bit[0-1] */
	uint64_t _reserved_1:6;     /**< byte[0-7],bit[2-7] */
	uint64_t CurveIndexColor1:2;/**< byte[0-7],bit[8-9] */
	uint64_t _reserved_2:6;     /**< byte[0-7],bit[10-15] */
	uint64_t CurveIndexColor2:2;/**< byte[0-7],bit[16-17] */
	uint64_t _reserved_3:6;     /**< byte[0-7],bit[18-23] */
	uint64_t ScaleExpColor0:5;  /**< byte[0-7],bit[24-28] */
	uint64_t _reserved_4:3;     /**< byte[0-7],bit[29-31] */
	uint64_t ScaleExpColor1:5;  /**< byte[0-7],bit[32-36] */
	uint64_t _reserved_5:3;     /**< byte[0-7],bit[37-39] */
	uint64_t ScaleExpColor2:5;  /**< byte[0-7],bit[40-44] */
	uint64_t _reserved_6:3;     /**< byte[0-7],bit[45-47] */
	uint64_t ColorTDEn:1;       /**< byte[0-7],bit[48] */
	uint64_t _reserved_7:7;     /**< byte[0-7],bit[49-55] */
	uint64_t AQLExp:4;          /**< byte[0-7],bit[56-59] */
	uint64_t _reserved_8:4;     /**< byte[0-7],bit[60-63] */
}; /* PLID#1418 */

/** NPU Register payload TM_Sched_TMtoTMPortBPState (PLID#1380).
 *
 * Used by TM.Sched.TMtoTMPortBPState.
 */
struct TM_Sched_TMtoTMPortBPState {
	uint64_t BP;                /**< byte[0-7] */
}; /* PLID#1380 */

/** NPU Register payload TM_Sched_AlvlPerStatus (PLID#1245).
 *
 * Used by TM.Sched.AlvlPerStatus.
 */
struct TM_Sched_AlvlPerStatus {
	uint64_t PerRoundCnt:10;    /**< byte[0-7],bit[0-9] */
	uint64_t _reserved_1:22;    /**< byte[0-7],bit[10-31] */
	uint64_t PerPointer:11;     /**< byte[0-7],bit[32-42] */
	uint64_t _reserved_2:21;    /**< byte[0-7],bit[43-63] */
}; /* PLID#1245 */

/** NPU Register payload TM_Sched_AlvlNodeState (PLID#1381).
 *
 * Used by TM.Sched.AlvlNodeState.
 */
struct TM_Sched_AlvlNodeState {
	uint64_t State:11;          /**< byte[0-7],bit[0-10] */
	uint64_t _reserved_1:53;    /**< byte[0-7],bit[11-63] */
}; /* PLID#1381 */

/** NPU Register payload TM_Sched_PortDefPrioLo (PLID#1412).
 *
 * Used by TM.Sched.PortDefPrioLo.
 */
struct TM_Sched_PortDefPrioLo {
	uint64_t Deficit0:16;       /**< byte[0-7],bit[0-15] */
	uint64_t Deficit1:16;       /**< byte[0-7],bit[16-31] */
	uint64_t Deficit2:16;       /**< byte[0-7],bit[32-47] */
	uint64_t Deficit3:16;       /**< byte[0-7],bit[48-63] */
}; /* PLID#1412 */

/** NPU Register payload TM_Sched_ClvlMyQ (PLID#1382).
 *
 * Used by TM.Sched.ClvlMyQ.
 */
struct TM_Sched_ClvlMyQ {
	uint64_t Status:34;         /**< byte[0-7],bit[0-33] */
	uint64_t _reserved_1:30;    /**< byte[0-7],bit[34-63] */
}; /* PLID#1382 */

/** NPU Register payload TM_Sched_CLevelShaperBucketNeg (PLID#1416).
 *
 * Used by TM.Sched.CLevelShaperBucketNeg.
 */
struct TM_Sched_CLevelShaperBucketNeg {
	uint64_t MinTBNeg:32;       /**< byte[0-7],bit[0-31] */
	uint64_t MaxTBNeg:32;       /**< byte[0-7],bit[32-63] */
}; /* PLID#1416 */

/** NPU Register payload TM_Sched_ClvlTokenBucketBurstSize (PLID#1403).
 *
 * Used by TM.Sched.ClvlTokenBucketBurstSize.
 */
struct TM_Sched_ClvlTokenBucketBurstSize {
	uint64_t MinBurstSz:12;     /**< byte[0-7],bit[0-11] */
	uint64_t _reserved_1:20;    /**< byte[0-7],bit[12-31] */
	uint64_t MaxBurstSz:12;     /**< byte[0-7],bit[32-43] */
	uint64_t _reserved_2:20;    /**< byte[0-7],bit[44-63] */
}; /* PLID#1403 */

/** NPU Register payload TM_Drop_BlvlDropProfPtr (PLID#1424).
 *
 * Used by TM.Drop.BlvlDropProfPtr.
 */
struct TM_Drop_BlvlDropProfPtr {
	uint64_t ProfPtr0:3;        /**< byte[0-7],bit[0-2] */
	uint64_t _reserved_1:5;     /**< byte[0-7],bit[3-7] */
	uint64_t ProfPtr1:3;        /**< byte[0-7],bit[8-10] */
	uint64_t _reserved_2:5;     /**< byte[0-7],bit[11-15] */
	uint64_t ProfPtr2:3;        /**< byte[0-7],bit[16-18] */
	uint64_t _reserved_3:5;     /**< byte[0-7],bit[19-23] */
	uint64_t ProfPtr3:3;        /**< byte[0-7],bit[24-26] */
	uint64_t _reserved_4:5;     /**< byte[0-7],bit[27-31] */
	uint64_t ProfPtr4:3;        /**< byte[0-7],bit[32-34] */
	uint64_t _reserved_5:5;     /**< byte[0-7],bit[35-39] */
	uint64_t ProfPtr5:3;        /**< byte[0-7],bit[40-42] */
	uint64_t _reserved_6:5;     /**< byte[0-7],bit[43-47] */
	uint64_t ProfPtr6:3;        /**< byte[0-7],bit[48-50] */
	uint64_t _reserved_7:5;     /**< byte[0-7],bit[51-55] */
	uint64_t ProfPtr7:3;        /**< byte[0-7],bit[56-58] */
	uint64_t _reserved_8:5;     /**< byte[0-7],bit[59-63] */
}; /* PLID#1424 */

/** NPU Register payload TM_Sched_BlvlRRDWRRStatus23 (PLID#1363).
 *
 * Used by TM.Sched.BlvlRRDWRRStatus23.
 */
struct TM_Sched_BlvlRRDWRRStatus23 {
	uint64_t Status:30;         /**< byte[0-7],bit[0-29] */
	uint64_t _reserved_1:34;    /**< byte[0-7],bit[30-63] */
}; /* PLID#1363 */

/** NPU Register payload TM_Drop_ExcMask (PLID#1260).
 *
 * Used by TM.Drop.ExcMask.
 */
struct TM_Drop_ExcMask {
	uint64_t ForcedErr:1;       /**< byte[0-7],bit[0] */
	uint64_t CorrECCErr:1;      /**< byte[0-7],bit[1] */
	uint64_t UncECCErr:1;       /**< byte[0-7],bit[2] */
	uint64_t QuesryRespSyncFifoFull:1;/**< byte[0-7],bit[3] */
	uint64_t QueryReqFifoOverflow:1;/**< byte[0-7],bit[4] */
	uint64_t AgingFifoOverflow:1;/**< byte[0-7],bit[5] */
	uint64_t _reserved_1:58;    /**< byte[0-7],bit[6-63] */
}; /* PLID#1260 */

/** NPU Register payload TM_Drop_ClvlInstAndAvgQueueLength (PLID#1429).
 *
 * Used by TM.Drop.ClvlInstAndAvgQueueLength.
 */
struct TM_Drop_ClvlInstAndAvgQueueLength {
	uint64_t QL:29;             /**< byte[0-7],bit[0-28] */
	uint64_t _reserved_1:3;     /**< byte[0-7],bit[29-31] */
	uint64_t AQL:29;            /**< byte[0-7],bit[32-60] */
	uint64_t _reserved_2:3;     /**< byte[0-7],bit[61-63] */
}; /* PLID#1429 */

/** NPU Register payload TM_Sched_QueuePerRateShpPrmsInt (PLID#1238).
 *
 * Used by TM.Sched.QueuePerRateShpPrmsInt.
 */
struct TM_Sched_QueuePerRateShpPrmsInt {
	uint64_t B:3;               /**< byte[0-7],bit[0-2] */
	uint64_t _reserved_1:29;    /**< byte[0-7],bit[3-31] */
	uint64_t I:3;               /**< byte[0-7],bit[32-34] */
	uint64_t _reserved_2:29;    /**< byte[0-7],bit[35-63] */
}; /* PLID#1238 */

/** NPU Register payload TM_Sched_ClvlTokenBucketTokenEnDiv (PLID#1402).
 *
 * Used by TM.Sched.ClvlTokenBucketTokenEnDiv.
 */
struct TM_Sched_ClvlTokenBucketTokenEnDiv {
	uint64_t MinToken:12;       /**< byte[0-7],bit[0-11] */
	uint64_t MinTokenRes:3;     /**< byte[0-7],bit[12-14] */
	uint64_t _reserved_1:1;     /**< byte[0-7],bit[15] */
	uint64_t MaxToken:12;       /**< byte[0-7],bit[16-27] */
	uint64_t MaxTokenRes:3;     /**< byte[0-7],bit[28-30] */
	uint64_t _reserved_2:1;     /**< byte[0-7],bit[31] */
	uint64_t MinDivExp:3;       /**< byte[0-7],bit[32-34] */
	uint64_t _reserved_3:5;     /**< byte[0-7],bit[35-39] */
	uint64_t MaxDivExp:3;       /**< byte[0-7],bit[40-42] */
	uint64_t _reserved_4:21;    /**< byte[0-7],bit[43-63] */
}; /* PLID#1402 */

/** NPU Register payload TM_Sched_QueueNodeState (PLID#1381).
 *
 * Used by TM.Sched.QueueNodeState.
 */
struct TM_Sched_QueueNodeState {
	uint64_t State:11;          /**< byte[0-7],bit[0-10] */
	uint64_t _reserved_1:53;    /**< byte[0-7],bit[11-63] */
}; /* PLID#1381 */

/** NPU Register payload TM_Drop_RespLocalDPSel (PLID#1265).
 *
 * Used by TM.Drop.RespLocalDPSel.
 */
struct TM_Drop_RespLocalDPSel {
	uint64_t DPSel:3;           /**< byte[0-7],bit[0-2] */
	uint64_t _reserved_1:5;     /**< byte[0-7],bit[3-7] */
	uint64_t PortDPSel:1;       /**< byte[0-7],bit[8] */
	uint64_t _reserved_2:55;    /**< byte[0-7],bit[9-63] */
}; /* PLID#1265 */

/** NPU Register payload TM_Sched_PortRRDWRRStatus23 (PLID#1378).
 *
 * Used by TM.Sched.PortRRDWRRStatus23.
 */
struct TM_Sched_PortRRDWRRStatus23 {
	uint64_t Status:20;         /**< byte[0-7],bit[0-19] */
	uint64_t _reserved_1:44;    /**< byte[0-7],bit[20-63] */
}; /* PLID#1378 */

/** NPU Register payload TM_Sched_TreeDeqEn (PLID#1251).
 *
 * Used by TM.Sched.TreeDeqEn.
 */
struct TM_Sched_TreeDeqEn {
	uint64_t En:1;              /**< byte[0-7],bit[0] */
	uint64_t _reserved_1:63;    /**< byte[0-7],bit[1-63] */
}; /* PLID#1251 */

/** NPU Register payload TM_Drop_EccMemParams (PLID#1231).
 *
 * Used by TM.Drop.EccMemParams[0-42].
 */
struct TM_Drop_EccMemParams {
	uint64_t Counter:8;         /**< byte[0-7],bit[0-7] */
	uint64_t Address:24;        /**< byte[0-7],bit[8-31] */
	uint64_t Syndrom:32;        /**< byte[0-7],bit[32-63] */
}; /* PLID#1231 */

/** NPU Register payload TM_Sched_PortDefPrioHi (PLID#1412).
 *
 * Used by TM.Sched.PortDefPrioHi.
 */
struct TM_Sched_PortDefPrioHi {
	uint64_t Deficit0:16;       /**< byte[0-7],bit[0-15] */
	uint64_t Deficit1:16;       /**< byte[0-7],bit[16-31] */
	uint64_t Deficit2:16;       /**< byte[0-7],bit[32-47] */
	uint64_t Deficit3:16;       /**< byte[0-7],bit[48-63] */
}; /* PLID#1412 */

/** NPU Register payload TM_Sched_ClvlWFS (PLID#1379).
 *
 * Used by TM.Sched.ClvlWFS.
 */
struct TM_Sched_ClvlWFS {
	uint64_t WFS:32;            /**< byte[0-7],bit[0-31] */
	uint64_t _reserved_1:32;    /**< byte[0-7],bit[32-63] */
}; /* PLID#1379 */

/** NPU Register payload TM_Sched_AlvlL0ClusterStateLo (PLID#1384).
 *
 * Used by TM.Sched.AlvlL0ClusterStateLo.
 */
struct TM_Sched_AlvlL0ClusterStateLo {
	uint64_t Status;            /**< byte[0-7] */
}; /* PLID#1384 */

/** NPU Register payload TM_Drop_PortDropPrfWREDScaleRatio_CoSRes (PLID#1419).
 *
 * Used by TM.Drop.PortDropPrfWREDScaleRatio_CoSRes[0-7].
 */
struct TM_Drop_PortDropPrfWREDScaleRatio_CoSRes {
	uint64_t ScaleRatioColor0:10;/**< byte[0-7],bit[0-9] */
	uint64_t _reserved_1:6;     /**< byte[0-7],bit[10-15] */
	uint64_t ScaleRatioColor1:10;/**< byte[0-7],bit[16-25] */
	uint64_t _reserved_2:6;     /**< byte[0-7],bit[26-31] */
	uint64_t ScaleRatioColor2:10;/**< byte[0-7],bit[32-41] */
	uint64_t _reserved_3:22;    /**< byte[0-7],bit[42-63] */
}; /* PLID#1419 */

/** NPU Register payload TM_Sched_QueueTokenBucketTokenEnDiv (PLID#1402).
 *
 * Used by TM.Sched.QueueTokenBucketTokenEnDiv.
 */
struct TM_Sched_QueueTokenBucketTokenEnDiv {
	uint64_t MinToken:12;       /**< byte[0-7],bit[0-11] */
	uint64_t MinTokenRes:3;     /**< byte[0-7],bit[12-14] */
	uint64_t _reserved_1:1;     /**< byte[0-7],bit[15] */
	uint64_t MaxToken:12;       /**< byte[0-7],bit[16-27] */
	uint64_t MaxTokenRes:3;     /**< byte[0-7],bit[28-30] */
	uint64_t _reserved_2:1;     /**< byte[0-7],bit[31] */
	uint64_t MinDivExp:3;       /**< byte[0-7],bit[32-34] */
	uint64_t _reserved_3:5;     /**< byte[0-7],bit[35-39] */
	uint64_t MaxDivExp:3;       /**< byte[0-7],bit[40-42] */
	uint64_t _reserved_4:21;    /**< byte[0-7],bit[43-63] */
}; /* PLID#1402 */

/** NPU Register payload TM_Sched_QueueQuantum (PLID#1404).
 *
 * Used by TM.Sched.QueueQuantum.
 */
struct TM_Sched_QueueQuantum {
	uint64_t Quantum:14;        /**< byte[0-7],bit[0-13] */
	uint64_t _reserved_1:50;    /**< byte[0-7],bit[14-63] */
}; /* PLID#1404 */

/** NPU Register payload TM_Sched_AlvlMyQ (PLID#1388).
 *
 * Used by TM.Sched.AlvlMyQ.
 */
struct TM_Sched_AlvlMyQ {
	uint64_t Status:22;         /**< byte[0-7],bit[0-21] */
	uint64_t _reserved_1:42;    /**< byte[0-7],bit[22-63] */
}; /* PLID#1388 */

/** NPU Register payload TM_Sched_TMtoTMAlvlBPState (PLID#1385).
 *
 * Used by TM.Sched.TMtoTMAlvlBPState.
 */
struct TM_Sched_TMtoTMAlvlBPState {
	uint64_t BPState:1;         /**< byte[0-7],bit[0] */
	uint64_t _reserved_1:63;    /**< byte[0-7],bit[1-63] */
}; /* PLID#1385 */

/** NPU Register payload TM_Drop_PortDropPrfWREDParams (PLID#1418).
 *
 * Used by TM.Drop.PortDropPrfWREDParams.
 */
struct TM_Drop_PortDropPrfWREDParams {
	uint64_t CurveIndexColor0:2;/**< byte[0-7],bit[0-1] */
	uint64_t _reserved_1:6;     /**< byte[0-7],bit[2-7] */
	uint64_t CurveIndexColor1:2;/**< byte[0-7],bit[8-9] */
	uint64_t _reserved_2:6;     /**< byte[0-7],bit[10-15] */
	uint64_t CurveIndexColor2:2;/**< byte[0-7],bit[16-17] */
	uint64_t _reserved_3:6;     /**< byte[0-7],bit[18-23] */
	uint64_t ScaleExpColor0:5;  /**< byte[0-7],bit[24-28] */
	uint64_t _reserved_4:3;     /**< byte[0-7],bit[29-31] */
	uint64_t ScaleExpColor1:5;  /**< byte[0-7],bit[32-36] */
	uint64_t _reserved_5:3;     /**< byte[0-7],bit[37-39] */
	uint64_t ScaleExpColor2:5;  /**< byte[0-7],bit[40-44] */
	uint64_t _reserved_6:3;     /**< byte[0-7],bit[45-47] */
	uint64_t ColorTDEn:1;       /**< byte[0-7],bit[48] */
	uint64_t _reserved_7:7;     /**< byte[0-7],bit[49-55] */
	uint64_t AQLExp:4;          /**< byte[0-7],bit[56-59] */
	uint64_t _reserved_8:4;     /**< byte[0-7],bit[60-63] */
}; /* PLID#1418 */

/** NPU Register payload TM_Sched_AlvlTokenBucketBurstSize (PLID#1403).
 *
 * Used by TM.Sched.AlvlTokenBucketBurstSize.
 */
struct TM_Sched_AlvlTokenBucketBurstSize {
	uint64_t MinBurstSz:12;     /**< byte[0-7],bit[0-11] */
	uint64_t _reserved_1:20;    /**< byte[0-7],bit[12-31] */
	uint64_t MaxBurstSz:12;     /**< byte[0-7],bit[32-43] */
	uint64_t _reserved_2:20;    /**< byte[0-7],bit[44-63] */
}; /* PLID#1403 */

/** NPU Register payload TM_Sched_AlvlMyQEccErrStatus (PLID#1242).
 *
 * Used by TM.Sched.AlvlMyQEccErrStatus.
 */
struct TM_Sched_AlvlMyQEccErrStatus {
	uint64_t UncEccErr:1;       /**< byte[0-7],bit[0] */
	uint64_t _reserved_1:15;    /**< byte[0-7],bit[1-15] */
	uint64_t CorrEccErr:1;      /**< byte[0-7],bit[16] */
	uint64_t _reserved_2:47;    /**< byte[0-7],bit[17-63] */
}; /* PLID#1242 */

/** NPU Register payload TM_Sched_ClvlBankEccErrStatus (PLID#1243).
 *
 * Used by TM.Sched.ClvlBankEccErrStatus.
 */
struct TM_Sched_ClvlBankEccErrStatus {
	uint64_t UncEccErr:4;       /**< byte[0-7],bit[0-3] */
	uint64_t _reserved_1:12;    /**< byte[0-7],bit[4-15] */
	uint64_t CorrEccErr:4;      /**< byte[0-7],bit[16-19] */
	uint64_t _reserved_2:44;    /**< byte[0-7],bit[20-63] */
}; /* PLID#1243 */

/** NPU Register payload TM_Drop_ClvlDropPrfWREDMinThresh_CoS (PLID#1420).
 *
 * Used by TM.Drop.ClvlDropPrfWREDMinThresh.CoS[0-7].
 */
struct TM_Drop_ClvlDropPrfWREDMinThresh_CoS {
	uint64_t MinTHColor0:10;    /**< byte[0-7],bit[0-9] */
	uint64_t _reserved_1:6;     /**< byte[0-7],bit[10-15] */
	uint64_t MinTHColor1:10;    /**< byte[0-7],bit[16-25] */
	uint64_t _reserved_2:6;     /**< byte[0-7],bit[26-31] */
	uint64_t MinTHColor2:10;    /**< byte[0-7],bit[32-41] */
	uint64_t _reserved_3:22;    /**< byte[0-7],bit[42-63] */
}; /* PLID#1420 */

/** NPU Register payload TM_Sched_ClvlL0ClusterStateHi (PLID#1417).
 *
 * Used by TM.Sched.ClvlL0ClusterStateHi.
 */
struct TM_Sched_ClvlL0ClusterStateHi {
	uint64_t status;            /**< byte[0-7] */
}; /* PLID#1417 */

/** NPU Register payload TM_Drop_ClvlREDCurve_CoS (PLID#1423).
 *
 * Used by TM.Drop.ClvlREDCurve.CoS[0-7].
 */
struct TM_Drop_ClvlREDCurve_CoS {
	uint64_t Prob:6;            /**< byte[0-7],bit[0-5] */
	uint64_t _reserved_1:58;    /**< byte[0-7],bit[6-63] */
}; /* PLID#1423 */

/** NPU Register payload TM_Sched_QueueEccErrStatus (PLID#1246).
 *
 * Used by TM.Sched.QueueEccErrStatus.
 */
struct TM_Sched_QueueEccErrStatus {
	uint64_t UncEccErr:7;       /**< byte[0-7],bit[0-6] */
	uint64_t _reserved_1:9;     /**< byte[0-7],bit[7-15] */
	uint64_t CorrEccErr:7;      /**< byte[0-7],bit[16-22] */
	uint64_t _reserved_2:41;    /**< byte[0-7],bit[23-63] */
}; /* PLID#1246 */

/** NPU Register payload TM_Sched_PortTokenBucketTokenEnDiv (PLID#1396).
 *
 * Used by TM.Sched.PortTokenBucketTokenEnDiv.
 */
struct TM_Sched_PortTokenBucketTokenEnDiv {
	uint64_t MinToken:12;       /**< byte[0-7],bit[0-11] */
	uint64_t MinTokenRes:3;     /**< byte[0-7],bit[12-14] */
	uint64_t _reserved_1:1;     /**< byte[0-7],bit[15] */
	uint64_t MaxToken:12;       /**< byte[0-7],bit[16-27] */
	uint64_t MaxTokenRes:3;     /**< byte[0-7],bit[28-30] */
	uint64_t _reserved_2:1;     /**< byte[0-7],bit[31] */
	uint64_t Periods:13;        /**< byte[0-7],bit[32-44] */
	uint64_t _reserved_3:19;    /**< byte[0-7],bit[45-63] */
}; /* PLID#1396 */

/** NPU Register payload TM_Drop_BlvlREDCurve_Table (PLID#1423).
 *
 * Used by TM.Drop.BlvlREDCurve[0-3].Table.
 */
struct TM_Drop_BlvlREDCurve_Table {
	uint64_t Prob:6;            /**< byte[0-7],bit[0-5] */
	uint64_t _reserved_1:58;    /**< byte[0-7],bit[6-63] */
}; /* PLID#1423 */

/** NPU Register payload TM_Sched_ClvlPerRateShpPrms (PLID#1254).
 *
 * Used by TM.Sched.ClvlPerRateShpPrms.
 */
struct TM_Sched_ClvlPerRateShpPrms {
	uint64_t N:14;              /**< byte[0-7],bit[0-13] */
	uint64_t _reserved_1:2;     /**< byte[0-7],bit[14-15] */
	uint64_t K:14;              /**< byte[0-7],bit[16-29] */
	uint64_t _reserved_2:2;     /**< byte[0-7],bit[30-31] */
	uint64_t L:14;              /**< byte[0-7],bit[32-45] */
	uint64_t _reserved_3:18;    /**< byte[0-7],bit[46-63] */
}; /* PLID#1254 */

/** NPU Register payload TM_Sched_QueueTokenBucketBurstSize (PLID#1403).
 *
 * Used by TM.Sched.QueueTokenBucketBurstSize.
 */
struct TM_Sched_QueueTokenBucketBurstSize {
	uint64_t MinBurstSz:12;     /**< byte[0-7],bit[0-11] */
	uint64_t _reserved_1:20;    /**< byte[0-7],bit[12-31] */
	uint64_t MaxBurstSz:12;     /**< byte[0-7],bit[32-43] */
	uint64_t _reserved_2:20;    /**< byte[0-7],bit[44-63] */
}; /* PLID#1403 */

/** NPU Register payload TM_Sched_AlvlEligPrioFuncPtr (PLID#1395).
 *
 * Used by TM.Sched.AlvlEligPrioFuncPtr.
 */
struct TM_Sched_AlvlEligPrioFuncPtr {
	uint64_t Ptr:6;             /**< byte[0-7],bit[0-5] */
	uint64_t _reserved_1:58;    /**< byte[0-7],bit[6-63] */
}; /* PLID#1395 */

/** NPU Register payload TM_Sched_PortEccErrStatus (PLID#1239).
 *
 * Used by TM.Sched.PortEccErrStatus.
 */
struct TM_Sched_PortEccErrStatus {
	uint64_t UncEccErr:2;       /**< byte[0-7],bit[0-1] */
	uint64_t _reserved_1:14;    /**< byte[0-7],bit[2-15] */
	uint64_t CorrEccErr:2;      /**< byte[0-7],bit[16-17] */
	uint64_t _reserved_2:46;    /**< byte[0-7],bit[18-63] */
}; /* PLID#1239 */

/** NPU Register payload TM_Sched_TMtoTMQueueBPState (PLID#1385).
 *
 * Used by TM.Sched.TMtoTMQueueBPState.
 */
struct TM_Sched_TMtoTMQueueBPState {
	uint64_t BPState:1;         /**< byte[0-7],bit[0] */
	uint64_t _reserved_1:63;    /**< byte[0-7],bit[1-63] */
}; /* PLID#1385 */

/** NPU Register payload TM_Drop_QueueDropPrfWREDScaleRatio (PLID#1419).
 *
 * Used by TM.Drop.QueueDropPrfWREDScaleRatio.
 */
struct TM_Drop_QueueDropPrfWREDScaleRatio {
	uint64_t ScaleRatioColor0:10;/**< byte[0-7],bit[0-9] */
	uint64_t _reserved_1:6;     /**< byte[0-7],bit[10-15] */
	uint64_t ScaleRatioColor1:10;/**< byte[0-7],bit[16-25] */
	uint64_t _reserved_2:6;     /**< byte[0-7],bit[26-31] */
	uint64_t ScaleRatioColor2:10;/**< byte[0-7],bit[32-41] */
	uint64_t _reserved_3:22;    /**< byte[0-7],bit[42-63] */
}; /* PLID#1419 */

/** NPU Register payload TM_Sched_BlvlRRDWRRStatus01 (PLID#1363).
 *
 * Used by TM.Sched.BlvlRRDWRRStatus01.
 */
struct TM_Sched_BlvlRRDWRRStatus01 {
	uint64_t Status:30;         /**< byte[0-7],bit[0-29] */
	uint64_t _reserved_1:34;    /**< byte[0-7],bit[30-63] */
}; /* PLID#1363 */

/** NPU Register payload TM_Sched_BLvltoClvlAndAlvlRangeMap (PLID#1406).
 *
 * Used by TM.Sched.BLvltoClvlAndAlvlRangeMap.
 */
struct TM_Sched_BLvltoClvlAndAlvlRangeMap {
	uint64_t AlvlLo:7;          /**< byte[0-7],bit[0-6] */
	uint64_t _reserved_1:17;    /**< byte[0-7],bit[7-23] */
	uint64_t AlvlHi:7;          /**< byte[0-7],bit[24-30] */
	uint64_t _reserved_2:17;    /**< byte[0-7],bit[31-47] */
	uint64_t Clvl:4;            /**< byte[0-7],bit[48-51] */
	uint64_t _reserved_3:12;     /**< byte[0-7],bit[52-63] */
}; /* PLID#1406 */

/** NPU Register payload TM_Drop_AlvlDropPrfWREDMinThresh (PLID#1420).
 *
 * Used by TM.Drop.AlvlDropPrfWREDMinThresh.
 */
struct TM_Drop_AlvlDropPrfWREDMinThresh {
	uint64_t MinTHColor0:10;    /**< byte[0-7],bit[0-9] */
	uint64_t _reserved_1:6;     /**< byte[0-7],bit[10-15] */
	uint64_t MinTHColor1:10;    /**< byte[0-7],bit[16-25] */
	uint64_t _reserved_2:6;     /**< byte[0-7],bit[26-31] */
	uint64_t MinTHColor2:10;    /**< byte[0-7],bit[32-41] */
	uint64_t _reserved_3:22;    /**< byte[0-7],bit[42-63] */
}; /* PLID#1420 */

/** NPU Register payload TM_Sched_BlvlMyQEccErrStatus (PLID#1242).
 *
 * Used by TM.Sched.BlvlMyQEccErrStatus.
 */
struct TM_Sched_BlvlMyQEccErrStatus {
	uint64_t UncEccErr:1;       /**< byte[0-7],bit[0] */
	uint64_t _reserved_1:15;    /**< byte[0-7],bit[1-15] */
	uint64_t CorrEccErr:1;      /**< byte[0-7],bit[16] */
	uint64_t _reserved_2:47;    /**< byte[0-7],bit[17-63] */
}; /* PLID#1242 */

/** NPU Register payload TM_Sched_ClvlQuantum (PLID#1404).
 *
 * Used by TM.Sched.ClvlQuantum.
 */
struct TM_Sched_ClvlQuantum {
	uint64_t Quantum:14;        /**< byte[0-7],bit[0-13] */
	uint64_t _reserved_1:50;    /**< byte[0-7],bit[14-63] */
}; /* PLID#1404 */

/** NPU Register payload TM_Sched_QueueL2ClusterStateHi (PLID#1392).
 *
 * Used by TM.Sched.QueueL2ClusterStateHi.
 */
struct TM_Sched_QueueL2ClusterStateHi {
	uint64_t Status:48;         /**< byte[0-7],bit[0-47] */
	uint64_t _reserved_1:16;    /**< byte[0-7],bit[48-63] */
}; /* PLID#1392 */

/** NPU Register payload TM_Sched_ClvlRRDWRRStatus45 (PLID#1383).
 *
 * Used by TM.Sched.ClvlRRDWRRStatus45.
 */
struct TM_Sched_ClvlRRDWRRStatus45 {
	uint64_t Status:26;         /**< byte[0-7],bit[0-25] */
	uint64_t _reserved_1:38;    /**< byte[0-7],bit[26-63] */
}; /* PLID#1383 */

/** NPU Register payload TM_Drop_PortDropProb (PLID#1430).
 *
 * Used by TM.Drop.PortDropProb.
 */
struct TM_Drop_PortDropProb {
	uint64_t DropProb:13;       /**< byte[0-7],bit[0-12] */
	uint64_t _reserved_1:51;    /**< byte[0-7],bit[13-63] */
}; /* PLID#1430 */

/** NPU Register payload TM_Drop_QueueDropPrfWREDDPRatio (PLID#1421).
 *
 * Used by TM.Drop.QueueDropPrfWREDDPRatio.
 */
struct TM_Drop_QueueDropPrfWREDDPRatio {
	uint64_t DPRatio0:6;        /**< byte[0-7],bit[0-5] */
	uint64_t _reserved_1:2;     /**< byte[0-7],bit[6-7] */
	uint64_t DPRatio1:6;        /**< byte[0-7],bit[8-13] */
	uint64_t _reserved_2:2;     /**< byte[0-7],bit[14-15] */
	uint64_t DPRatio2:4;        /**< byte[0-7],bit[16-19] */
	uint64_t _reserved_3:44;    /**< byte[0-7],bit[20-63] */
}; /* PLID#1421 */

/** NPU Register payload TM_Drop_PortDropPrfWREDScaleRatio (PLID#1419).
 *
 * Used by TM.Drop.PortDropPrfWREDScaleRatio.
 */
struct TM_Drop_PortDropPrfWREDScaleRatio {
	uint64_t ScaleRatioColor0:10;/**< byte[0-7],bit[0-9] */
	uint64_t _reserved_1:6;     /**< byte[0-7],bit[10-15] */
	uint64_t ScaleRatioColor1:10;/**< byte[0-7],bit[16-25] */
	uint64_t _reserved_2:6;     /**< byte[0-7],bit[26-31] */
	uint64_t ScaleRatioColor2:10;/**< byte[0-7],bit[32-41] */
	uint64_t _reserved_3:22;    /**< byte[0-7],bit[42-63] */
}; /* PLID#1419 */

/** NPU Register payload TM_Sched_AlvlTokenBucketTokenEnDiv (PLID#1402).
 *
 * Used by TM.Sched.AlvlTokenBucketTokenEnDiv.
 */
struct TM_Sched_AlvlTokenBucketTokenEnDiv {
	uint64_t MinToken:12;       /**< byte[0-7],bit[0-11] */
	uint64_t MinTokenRes:3;     /**< byte[0-7],bit[12-14] */
	uint64_t _reserved_1:1;     /**< byte[0-7],bit[15] */
	uint64_t MaxToken:12;       /**< byte[0-7],bit[16-27] */
	uint64_t MaxTokenRes:3;     /**< byte[0-7],bit[28-30] */
	uint64_t _reserved_2:1;     /**< byte[0-7],bit[31] */
	uint64_t MinDivExp:3;       /**< byte[0-7],bit[32-34] */
	uint64_t _reserved_3:5;     /**< byte[0-7],bit[35-39] */
	uint64_t MaxDivExp:3;       /**< byte[0-7],bit[40-42] */
	uint64_t _reserved_4:21;    /**< byte[0-7],bit[43-63] */
}; /* PLID#1402 */

/** NPU Register payload TM_Drop_TMtoTMDPCoSSel (PLID#1267).
 *
 * Used by TM.Drop.TMtoTMDPCoSSel.
 */
struct TM_Drop_TMtoTMDPCoSSel {
	uint64_t CDPCoSSel:3;       /**< byte[0-7],bit[0-2] */
	uint64_t _reserved_1:61;    /**< byte[0-7],bit[3-63] */
}; /* PLID#1267 */

/** NPU Register payload TM_Sched_FirstExc (PLID#1247).
 *
 * Used by TM.Sched.FirstExc.
 */
struct TM_Sched_FirstExc {
	uint64_t ForcedErr:1;       /**< byte[0-7],bit[0] */
	uint64_t CorrECCErr:1;      /**< byte[0-7],bit[1] */
	uint64_t UncECCErr:1;       /**< byte[0-7],bit[2] */
	uint64_t BPBSat:1;          /**< byte[0-7],bit[3] */
	uint64_t TBNegSat:1;        /**< byte[0-7],bit[4] */
	uint64_t FIFOOvrflowErr:1;  /**< byte[0-7],bit[5] */
	uint64_t _reserved_1:58;    /**< byte[0-7],bit[6-63] */
}; /* PLID#1247 */

/** NPU Register payload TM_Sched_QueueShaperBucketNeg (PLID#1416).
 *
 * Used by TM.Sched.QueueShaperBucketNeg.
 */
struct TM_Sched_QueueShaperBucketNeg {
	uint64_t MinTBNeg:32;       /**< byte[0-7],bit[0-31] */
	uint64_t MaxTBNeg:32;       /**< byte[0-7],bit[32-63] */
}; /* PLID#1416 */

/** NPU Register payload TM_Sched_PortDWRRPrioEn (PLID#1398).
 *
 * Used by TM.Sched.PortDWRRPrioEn.
 */
struct TM_Sched_PortDWRRPrioEn {
	uint64_t En:8;              /**< byte[0-7],bit[0-7] */
	uint64_t _reserved_1:56;    /**< byte[0-7],bit[8-63] */
}; /* PLID#1398 */

/** NPU Register payload TM_Sched_PortEligPrioFuncPtr (PLID#1395).
 *
 * Used by TM.Sched.PortEligPrioFuncPtr.
 */
struct TM_Sched_PortEligPrioFuncPtr {
	uint64_t Ptr:6;             /**< byte[0-7],bit[0-5] */
	uint64_t _reserved_1:58;    /**< byte[0-7],bit[6-63] */
}; /* PLID#1395 */

/** NPU Register payload TM_Sched_ClvlEccErrStatus (PLID#1241).
 *
 * Used by TM.Sched.ClvlEccErrStatus.
 */
struct TM_Sched_ClvlEccErrStatus {
	uint64_t UncEccErr:5;       /**< byte[0-7],bit[0-4] */
	uint64_t _reserved_1:11;    /**< byte[0-7],bit[5-15] */
	uint64_t CorrEccErr:5;      /**< byte[0-7],bit[16-20] */
	uint64_t _reserved_2:43;    /**< byte[0-7],bit[21-63] */
}; /* PLID#1241 */

/** NPU Register payload TM_Sched_PortPerConf (PLID#1253).
 *
 * Used by TM.Sched.PortPerConf.
 */
struct TM_Sched_PortPerConf {
	uint64_t PerEn:1;           /**< byte[0-7],bit[0] */
	uint64_t _reserved_1:15;    /**< byte[0-7],bit[1-15] */
	uint64_t PerInterval:8;     /**< byte[0-7],bit[16-23] */
	uint64_t _reserved_2:24;    /**< byte[0-7],bit[24-47] */
	uint64_t DecEn:1;           /**< byte[0-7],bit[48] */
	uint64_t _reserved_3:15;    /**< byte[0-7],bit[49-63] */
}; /* PLID#1253 */

/** NPU Register payload TM_Sched_PortQuantumsPriosLo (PLID#1399).
 *
 * Used by TM.Sched.PortQuantumsPriosLo.
 */
struct TM_Sched_PortQuantumsPriosLo {
	uint64_t Quantum0:9;        /**< byte[0-7],bit[0-8] */
	uint64_t _reserved_1:7;     /**< byte[0-7],bit[9-15] */
	uint64_t Quantum1:9;        /**< byte[0-7],bit[16-24] */
	uint64_t _reserved_2:7;     /**< byte[0-7],bit[25-31] */
	uint64_t Quantum2:9;        /**< byte[0-7],bit[32-40] */
	uint64_t _reserved_3:7;     /**< byte[0-7],bit[41-47] */
	uint64_t Quantum3:9;        /**< byte[0-7],bit[48-56] */
	uint64_t _reserved_4:7;     /**< byte[0-7],bit[57-63] */
}; /* PLID#1399 */

/** NPU Register payload TM_Sched_AlvlL2ClusterStateLo (PLID#1391).
 *
 * Used by TM.Sched.AlvlL2ClusterStateLo.
 */
struct TM_Sched_AlvlL2ClusterStateLo {
	uint64_t Status:46;         /**< byte[0-7],bit[0-45] */
	uint64_t _reserved_1:18;    /**< byte[0-7],bit[46-63] */
}; /* PLID#1391 */

/** NPU Register payload TM_Sched_ClvlEligPrioFunc (PLID#1394).
 *
 * Used by TM.Sched.ClvlEligPrioFunc.
 */
struct TM_Sched_ClvlEligPrioFunc {
	uint64_t FuncOut0:9;        /**< byte[0-7],bit[0-8] */
	uint64_t _reserved_1:7;     /**< byte[0-7],bit[9-15] */
	uint64_t FuncOut1:9;        /**< byte[0-7],bit[16-24] */
	uint64_t _reserved_2:7;     /**< byte[0-7],bit[25-31] */
	uint64_t FuncOut2:9;        /**< byte[0-7],bit[32-40] */
	uint64_t _reserved_3:7;     /**< byte[0-7],bit[41-47] */
	uint64_t FuncOut3:9;        /**< byte[0-7],bit[48-56] */
	uint64_t _reserved_4:7;     /**< byte[0-7],bit[57-63] */
}; /* PLID#1394 */

/** NPU Register payload TM_Sched_TreeDWRRPrioEn (PLID#1252).
 *
 * Used by TM.Sched.TreeDWRRPrioEn.
 */
struct TM_Sched_TreeDWRRPrioEn {
	uint64_t PrioEn:8;          /**< byte[0-7],bit[0-7] */
	uint64_t _reserved_1:56;    /**< byte[0-7],bit[8-63] */
}; /* PLID#1252 */

/** NPU Register payload TM_Sched_QueueL1ClusterStateHi (PLID#1390).
 *
 * Used by TM.Sched.QueueL1ClusterStateHi.
 */
struct TM_Sched_QueueL1ClusterStateHi {
	uint64_t Status:40;         /**< byte[0-7],bit[0-39] */
	uint64_t _reserved_1:24;    /**< byte[0-7],bit[40-63] */
}; /* PLID#1390 */

/** NPU Register payload TM_Sched_AlvlWFS (PLID#1379).
 *
 * Used by TM.Sched.AlvlWFS.
 */
struct TM_Sched_AlvlWFS {
	uint64_t WFS:32;            /**< byte[0-7],bit[0-31] */
	uint64_t _reserved_1:32;    /**< byte[0-7],bit[32-63] */
}; /* PLID#1379 */

/** NPU Register payload TM_Drop_Id (PLID#1261).
 *
 * Used by TM.Drop.Id.
 */
struct TM_Drop_Id {
	uint64_t UID:8;             /**< byte[0-7],bit[0-7] */
	uint64_t SUID:8;            /**< byte[0-7],bit[8-15] */
	uint64_t _reserved_1:48;    /**< byte[0-7],bit[16-63] */
}; /* PLID#1261 */

/** NPU Register payload TM_Drop_ClvlDropPrfWREDDPRatio_CoS (PLID#1421).
 *
 * Used by TM.Drop.ClvlDropPrfWREDDPRatio.CoS[0-7].
 */
struct TM_Drop_ClvlDropPrfWREDDPRatio_CoS {
	uint64_t DPRatio0:6;        /**< byte[0-7],bit[0-5] */
	uint64_t _reserved_1:2;     /**< byte[0-7],bit[6-7] */
	uint64_t DPRatio1:6;        /**< byte[0-7],bit[8-13] */
	uint64_t _reserved_2:2;     /**< byte[0-7],bit[14-15] */
	uint64_t DPRatio2:4;        /**< byte[0-7],bit[16-19] */
	uint64_t _reserved_3:44;    /**< byte[0-7],bit[20-63] */
}; /* PLID#1421 */

/** NPU Register payload TM_Sched_BlvlL0ClusterStateLo (PLID#1384).
 *
 * Used by TM.Sched.BlvlL0ClusterStateLo.
 */
struct TM_Sched_BlvlL0ClusterStateLo {
	uint64_t Status;            /**< byte[0-7] */
}; /* PLID#1384 */

/** NPU Register payload TM_Drop_ClvlDropPrfWREDScaleRatio_CoS (PLID#1419).
 *
 * Used by TM.Drop.ClvlDropPrfWREDScaleRatio.CoS[0-7].
 */
struct TM_Drop_ClvlDropPrfWREDScaleRatio_CoS {
	uint64_t ScaleRatioColor0:10;/**< byte[0-7],bit[0-9] */
	uint64_t _reserved_1:6;     /**< byte[0-7],bit[10-15] */
	uint64_t ScaleRatioColor1:10;/**< byte[0-7],bit[16-25] */
	uint64_t _reserved_2:6;     /**< byte[0-7],bit[26-31] */
	uint64_t ScaleRatioColor2:10;/**< byte[0-7],bit[32-41] */
	uint64_t _reserved_3:22;    /**< byte[0-7],bit[42-63] */
}; /* PLID#1419 */

/** NPU Register payload TM_Sched_BlvlTokenBucketTokenEnDiv (PLID#1402).
 *
 * Used by TM.Sched.BlvlTokenBucketTokenEnDiv.
 */
struct TM_Sched_BlvlTokenBucketTokenEnDiv {
	uint64_t MinToken:12;       /**< byte[0-7],bit[0-11] */
	uint64_t MinTokenRes:3;     /**< byte[0-7],bit[12-14] */
	uint64_t _reserved_1:1;     /**< byte[0-7],bit[15] */
	uint64_t MaxToken:12;       /**< byte[0-7],bit[16-27] */
	uint64_t MaxTokenRes:3;     /**< byte[0-7],bit[28-30] */
	uint64_t _reserved_2:1;     /**< byte[0-7],bit[31] */
	uint64_t MinDivExp:3;       /**< byte[0-7],bit[32-34] */
	uint64_t _reserved_3:5;     /**< byte[0-7],bit[35-39] */
	uint64_t MaxDivExp:3;       /**< byte[0-7],bit[40-42] */
	uint64_t _reserved_4:21;    /**< byte[0-7],bit[43-63] */
}; /* PLID#1402 */

/** NPU Register payload TM_Sched_AlvlPerRateShpPrmsInt (PLID#1238).
 *
 * Used by TM.Sched.AlvlPerRateShpPrmsInt.
 */
struct TM_Sched_AlvlPerRateShpPrmsInt {
	uint64_t B:3;               /**< byte[0-7],bit[0-2] */
	uint64_t _reserved_1:29;    /**< byte[0-7],bit[3-31] */
	uint64_t I:3;               /**< byte[0-7],bit[32-34] */
	uint64_t _reserved_2:29;    /**< byte[0-7],bit[35-63] */
}; /* PLID#1238 */

/** NPU Register payload TM_Drop_PortDropProbPerCoS_CoS (PLID#1430).
 *
 * Used by TM.Drop.PortDropProbPerCoS_CoS[0-7].
 */
struct TM_Drop_PortDropProbPerCoS_CoS {
	uint64_t DropProb:13;       /**< byte[0-7],bit[0-12] */
	uint64_t _reserved_1:51;    /**< byte[0-7],bit[13-63] */
}; /* PLID#1430 */

/** NPU Register payload TM_Sched_ClvlShpBucketLvls (PLID#1413).
 *
 * Used by TM.Sched.ClvlShpBucketLvls.
 */
struct TM_Sched_ClvlShpBucketLvls {
	uint64_t MinLvl:23;         /**< byte[0-7],bit[0-22] */
	uint64_t _reserved_1:9;     /**< byte[0-7],bit[23-31] */
	uint64_t MaxLvl:23;         /**< byte[0-7],bit[32-54] */
	uint64_t _reserved_2:9;     /**< byte[0-7],bit[55-63] */
}; /* PLID#1413 */

/** NPU Register payload TM_Sched_AlvlRRDWRRStatus01 (PLID#1389).
 *
 * Used by TM.Sched.AlvlRRDWRRStatus01.
 */
struct TM_Sched_AlvlRRDWRRStatus01 {
	uint64_t Status:34;         /**< byte[0-7],bit[0-33] */
	uint64_t _reserved_1:30;    /**< byte[0-7],bit[34-63] */
}; /* PLID#1389 */

/** NPU Register payload TM_Sched_ClvlPerRateShpPrmsInt (PLID#1238).
 *
 * Used by TM.Sched.ClvlPerRateShpPrmsInt.
 */
struct TM_Sched_ClvlPerRateShpPrmsInt {
	uint64_t B:3;               /**< byte[0-7],bit[0-2] */
	uint64_t _reserved_1:29;    /**< byte[0-7],bit[3-31] */
	uint64_t I:3;               /**< byte[0-7],bit[32-34] */
	uint64_t _reserved_2:29;    /**< byte[0-7],bit[35-63] */
}; /* PLID#1238 */

/** NPU Register payload TM_Sched_TreeRRDWRRStatus (PLID#1237).
 *
 * Used by TM.Sched.TreeRRDWRRStatus.
 */
struct TM_Sched_TreeRRDWRRStatus {
	uint64_t Status;            /**< byte[0-7] */
}; /* PLID#1237 */

/** NPU Register payload TM_Sched_CLvlDef (PLID#1414).
 *
 * Used by TM.Sched.CLvlDef.
 */
struct TM_Sched_CLvlDef {
	uint64_t Deficit:22;        /**< byte[0-7],bit[0-21] */
	uint64_t _reserved_1:42;    /**< byte[0-7],bit[22-63] */
}; /* PLID#1414 */

/** NPU Register payload TM_Drop_QueueCoSConf (PLID#1428).
 *
 * Used by TM.Drop.QueueCoSConf.
 */
struct TM_Drop_QueueCoSConf {
	uint64_t QueueCos0:3;       /**< byte[0-7],bit[0-2] */
	uint64_t QueueCos1:3;       /**< byte[0-7],bit[3-5] */
	uint64_t QueueCos2:3;       /**< byte[0-7],bit[6-8] */
	uint64_t QueueCos3:3;       /**< byte[0-7],bit[9-11] */
	uint64_t _reserved_1:52;    /**< byte[0-7],bit[12-63] */
}; /* PLID#1428 */

/** NPU Register payload TM_Sched_QueueL0ClusterStateLo (PLID#1384).
 *
 * Used by TM.Sched.QueueL0ClusterStateLo.
 */
struct TM_Sched_QueueL0ClusterStateLo {
	uint64_t Status;            /**< byte[0-7] */
}; /* PLID#1384 */

/** NPU Register payload TM_Sched_ScrubDisable (PLID#1236).
 *
 * Used by TM.Sched.ScrubDisable.
 */
struct TM_Sched_ScrubDisable {
	uint64_t Dis:1;             /**< byte[0-7],bit[0] */
	uint64_t _reserved_1:63;    /**< byte[0-7],bit[1-63] */
}; /* PLID#1236 */

/** NPU Register payload TM_Sched_ScrubSlotAlloc (PLID#1250).
 *
 * Used by TM.Sched.ScrubSlotAlloc.
 */
struct TM_Sched_ScrubSlotAlloc {
	uint64_t QueueSlots:6;      /**< byte[0-7],bit[0-5] */
	uint64_t _reserved_1:2;     /**< byte[0-7],bit[6-7] */
	uint64_t AlvlSlots:6;       /**< byte[0-7],bit[8-13] */
	uint64_t _reserved_2:2;     /**< byte[0-7],bit[14-15] */
	uint64_t BlvlSlots:6;       /**< byte[0-7],bit[16-21] */
	uint64_t _reserved_3:2;     /**< byte[0-7],bit[22-23] */
	uint64_t ClvlSlots:6;       /**< byte[0-7],bit[24-29] */
	uint64_t _reserved_4:2;     /**< byte[0-7],bit[30-31] */
	uint64_t PortSlots:6;       /**< byte[0-7],bit[32-37] */
	uint64_t _reserved_5:26;    /**< byte[0-7],bit[38-63] */
}; /* PLID#1250 */

/** NPU Register payload TM_Sched_ALevelShaperBucketNeg (PLID#1416).
 *
 * Used by TM.Sched.ALevelShaperBucketNeg.
 */
struct TM_Sched_ALevelShaperBucketNeg {
	uint64_t MinTBNeg:32;       /**< byte[0-7],bit[0-31] */
	uint64_t MaxTBNeg:32;       /**< byte[0-7],bit[32-63] */
}; /* PLID#1416 */

/** NPU Register payload TM_Sched_TMtoTMClvlBPState (PLID#1385).
 *
 * Used by TM.Sched.TMtoTMClvlBPState.
 */
struct TM_Sched_TMtoTMClvlBPState {
	uint64_t BPState:1;         /**< byte[0-7],bit[0] */
	uint64_t _reserved_1:63;    /**< byte[0-7],bit[1-63] */
}; /* PLID#1385 */

/** NPU Register payload TM_Sched_AlvlShpBucketLvls (PLID#1413).
 *
 * Used by TM.Sched.AlvlShpBucketLvls.
 */
struct TM_Sched_AlvlShpBucketLvls {
	uint64_t MinLvl:23;         /**< byte[0-7],bit[0-22] */
	uint64_t _reserved_1:9;     /**< byte[0-7],bit[23-31] */
	uint64_t MaxLvl:23;         /**< byte[0-7],bit[32-54] */
	uint64_t _reserved_2:9;     /**< byte[0-7],bit[55-63] */
}; /* PLID#1413 */

/** NPU Register payload TM_Drop_BlvlDropPrfWREDMinThresh (PLID#1420).
 *
 * Used by TM.Drop.BlvlDropPrfWREDMinThresh.
 */
struct TM_Drop_BlvlDropPrfWREDMinThresh {
	uint64_t MinTHColor0:10;    /**< byte[0-7],bit[0-9] */
	uint64_t _reserved_1:6;     /**< byte[0-7],bit[10-15] */
	uint64_t MinTHColor1:10;    /**< byte[0-7],bit[16-25] */
	uint64_t _reserved_2:6;     /**< byte[0-7],bit[26-31] */
	uint64_t MinTHColor2:10;    /**< byte[0-7],bit[32-41] */
	uint64_t _reserved_3:22;    /**< byte[0-7],bit[42-63] */
}; /* PLID#1420 */

/** NPU Register payload TM_Sched_QueueEligPrioFuncPtr (PLID#1409).
 *
 * Used by TM.Sched.QueueEligPrioFuncPtr.
 */
struct TM_Sched_QueueEligPrioFuncPtr {
	uint64_t Ptr:6;             /**< byte[0-7],bit[0-5] */
	uint64_t _reserved_1:58;    /**< byte[0-7],bit[6-63] */
}; /* PLID#1409 */

/** NPU Register payload TM_Drop_ClvlDropProb (PLID#1430).
 *
 * Used by TM.Drop.ClvlDropProb.
 */
struct TM_Drop_ClvlDropProb {
	uint64_t DropProb:13;       /**< byte[0-7],bit[0-12] */
	uint64_t _reserved_1:51;    /**< byte[0-7],bit[13-63] */
}; /* PLID#1430 */

/** NPU Register payload TM_Sched_ForceErr (PLID#1183).
 *
 * Used by TM.Sched.ForceErr.
 */
struct TM_Sched_ForceErr {
	uint64_t ForcedErr:1;       /**< byte[0-7],bit[0] */
	uint64_t _reserved_1:63;    /**< byte[0-7],bit[1-63] */
}; /* PLID#1183 */

/** NPU Register payload TM_Drop_PortDropPrfWREDParams_CoSRes (PLID#1418).
 *
 * Used by TM.Drop.PortDropPrfWREDParams_CoSRes[0-7].
 */
struct TM_Drop_PortDropPrfWREDParams_CoSRes {
	uint64_t CurveIndexColor0:2;/**< byte[0-7],bit[0-1] */
	uint64_t _reserved_1:6;     /**< byte[0-7],bit[2-7] */
	uint64_t CurveIndexColor1:2;/**< byte[0-7],bit[8-9] */
	uint64_t _reserved_2:6;     /**< byte[0-7],bit[10-15] */
	uint64_t CurveIndexColor2:2;/**< byte[0-7],bit[16-17] */
	uint64_t _reserved_3:6;     /**< byte[0-7],bit[18-23] */
	uint64_t ScaleExpColor0:5;  /**< byte[0-7],bit[24-28] */
	uint64_t _reserved_4:3;     /**< byte[0-7],bit[29-31] */
	uint64_t ScaleExpColor1:5;  /**< byte[0-7],bit[32-36] */
	uint64_t _reserved_5:3;     /**< byte[0-7],bit[37-39] */
	uint64_t ScaleExpColor2:5;  /**< byte[0-7],bit[40-44] */
	uint64_t _reserved_6:3;     /**< byte[0-7],bit[45-47] */
	uint64_t ColorTDEn:1;       /**< byte[0-7],bit[48] */
	uint64_t _reserved_7:7;     /**< byte[0-7],bit[49-55] */
	uint64_t AQLExp:4;          /**< byte[0-7],bit[56-59] */
	uint64_t _reserved_8:4;     /**< byte[0-7],bit[60-63] */
}; /* PLID#1418 */

/** NPU Register payload TM_Drop_ClvlDropPrfWREDParams_CoS (PLID#1418).
 *
 * Used by TM.Drop.ClvlDropPrfWREDParams.CoS[0-7].
 */
struct TM_Drop_ClvlDropPrfWREDParams_CoS {
	uint64_t CurveIndexColor0:2;/**< byte[0-7],bit[0-1] */
	uint64_t _reserved_1:6;     /**< byte[0-7],bit[2-7] */
	uint64_t CurveIndexColor1:2;/**< byte[0-7],bit[8-9] */
	uint64_t _reserved_2:6;     /**< byte[0-7],bit[10-15] */
	uint64_t CurveIndexColor2:2;/**< byte[0-7],bit[16-17] */
	uint64_t _reserved_3:6;     /**< byte[0-7],bit[18-23] */
	uint64_t ScaleExpColor0:5;  /**< byte[0-7],bit[24-28] */
	uint64_t _reserved_4:3;     /**< byte[0-7],bit[29-31] */
	uint64_t ScaleExpColor1:5;  /**< byte[0-7],bit[32-36] */
	uint64_t _reserved_5:3;     /**< byte[0-7],bit[37-39] */
	uint64_t ScaleExpColor2:5;  /**< byte[0-7],bit[40-44] */
	uint64_t _reserved_6:3;     /**< byte[0-7],bit[45-47] */
	uint64_t ColorTDEn:1;       /**< byte[0-7],bit[48] */
	uint64_t _reserved_7:7;     /**< byte[0-7],bit[49-55] */
	uint64_t AQLExp:4;          /**< byte[0-7],bit[56-59] */
	uint64_t _reserved_8:4;     /**< byte[0-7],bit[60-63] */
}; /* PLID#1418 */

/** NPU Register payload TM_Sched_BlvlEccErrStatus (PLID#1235).
 *
 * Used by TM.Sched.BlvlEccErrStatus.
 */
struct TM_Sched_BlvlEccErrStatus {
	uint64_t UncEccErr:6;       /**< byte[0-7],bit[0-5] */
	uint64_t _reserved_1:10;    /**< byte[0-7],bit[6-15] */
	uint64_t CorrEccErr:6;      /**< byte[0-7],bit[16-21] */
	uint64_t _reserved_2:42;    /**< byte[0-7],bit[22-63] */
}; /* PLID#1235 */

/** NPU Register payload TM_Sched_BlvlRRDWRRStatus45 (PLID#1363).
 *
 * Used by TM.Sched.BlvlRRDWRRStatus45.
 */
struct TM_Sched_BlvlRRDWRRStatus45 {
	uint64_t Status:30;         /**< byte[0-7],bit[0-29] */
	uint64_t _reserved_1:34;    /**< byte[0-7],bit[30-63] */
}; /* PLID#1363 */

/** NPU Register payload TM_Sched_QueueShpBucketLvls (PLID#1413).
 *
 * Used by TM.Sched.QueueShpBucketLvls.
 */
struct TM_Sched_QueueShpBucketLvls {
	uint64_t MinLvl:23;         /**< byte[0-7],bit[0-22] */
	uint64_t _reserved_1:9;     /**< byte[0-7],bit[23-31] */
	uint64_t MaxLvl:23;         /**< byte[0-7],bit[32-54] */
	uint64_t _reserved_2:9;     /**< byte[0-7],bit[55-63] */
}; /* PLID#1413 */

/** NPU Register payload TM_Sched_PortPerRateShpPrmsInt (PLID#1238).
 *
 * Used by TM.Sched.PortPerRateShpPrmsInt.
 */
struct TM_Sched_PortPerRateShpPrmsInt {
	uint64_t B:3;               /**< byte[0-7],bit[0-2] */
	uint64_t _reserved_1:29;    /**< byte[0-7],bit[3-31] */
	uint64_t I:3;               /**< byte[0-7],bit[32-34] */
	uint64_t _reserved_2:29;    /**< byte[0-7],bit[35-63] */
}; /* PLID#1238 */

/** NPU Register payload TM_Drop_BlvlDropPrfTailDrpThresh (PLID#1422).
 *
 * Used by TM.Drop.BlvlDropPrfTailDrpThresh.
 */
struct TM_Drop_BlvlDropPrfTailDrpThresh {
	uint64_t TailDropThresh:19; /**< byte[0-7],bit[0-18] */
	uint64_t _reserved_1:13;    /**< byte[0-7],bit[19-31] */
	uint64_t TailDropThreshRes:1;/**< byte[0-7],bit[32] */
	uint64_t _reserved_2:31;    /**< byte[0-7],bit[33-63] */
}; /* PLID#1422 */

/** NPU Register payload TM_Drop_AlvlDropProfPtr (PLID#1425).
 *
 * Used by TM.Drop.AlvlDropProfPtr.
 */
struct TM_Drop_AlvlDropProfPtr {
	uint64_t ProfPtr0:3;        /**< byte[0-7],bit[0-2] */
	uint64_t _reserved_1:13;    /**< byte[0-7],bit[3-15] */
	uint64_t ProfPtr1:3;        /**< byte[0-7],bit[16-18] */
	uint64_t _reserved_2:13;    /**< byte[0-7],bit[19-31] */
	uint64_t ProfPtr2:3;        /**< byte[0-7],bit[32-34] */
	uint64_t _reserved_3:13;    /**< byte[0-7],bit[35-47] */
	uint64_t ProfPtr3:3;        /**< byte[0-7],bit[48-50] */
	uint64_t _reserved_4:13;    /**< byte[0-7],bit[51-63] */
}; /* PLID#1425 */

/** NPU Register payload TM_Drop_WREDMaxProbModePerColor (PLID#1263).
 *
 * Used by TM.Drop.WREDMaxProbModePerColor.
 */
struct TM_Drop_WREDMaxProbModePerColor {
	uint64_t Queue:6;           /**< byte[0-7],bit[0-5] */
	uint64_t _reserved_1:2;     /**< byte[0-7],bit[6-7] */
	uint64_t Alvl:6;            /**< byte[0-7],bit[8-13] */
	uint64_t _reserved_2:2;     /**< byte[0-7],bit[14-15] */
	uint64_t Blvl:6;            /**< byte[0-7],bit[16-21] */
	uint64_t _reserved_3:2;     /**< byte[0-7],bit[22-23] */
	uint64_t Clvl:6;            /**< byte[0-7],bit[24-29] */
	uint64_t _reserved_4:2;     /**< byte[0-7],bit[30-31] */
	uint64_t Port:6;            /**< byte[0-7],bit[32-37] */
	uint64_t _reserved_5:26;    /**< byte[0-7],bit[38-63] */
}; /* PLID#1263 */

/** NPU Register payload TM_Sched_AlvlEccErrStatus (PLID#1246).
 *
 * Used by TM.Sched.AlvlEccErrStatus.
 */
struct TM_Sched_AlvlEccErrStatus {
	uint64_t UncEccErr:7;       /**< byte[0-7],bit[0-6] */
	uint64_t _reserved_1:9;     /**< byte[0-7],bit[7-15] */
	uint64_t CorrEccErr:7;      /**< byte[0-7],bit[16-22] */
	uint64_t _reserved_2:41;    /**< byte[0-7],bit[23-63] */
}; /* PLID#1246 */

/** NPU Register payload TM_Sched_AlvlRRDWRRStatus23 (PLID#1389).
 *
 * Used by TM.Sched.AlvlRRDWRRStatus23.
 */
struct TM_Sched_AlvlRRDWRRStatus23 {
	uint64_t Status:34;         /**< byte[0-7],bit[0-33] */
	uint64_t _reserved_1:30;    /**< byte[0-7],bit[34-63] */
}; /* PLID#1389 */

/** NPU Register payload TM_Sched_ErrStus (PLID#1247).
 *
 * Used by TM.Sched.ErrStus.
 */
struct TM_Sched_ErrStus {
	uint64_t ForcedErr:1;       /**< byte[0-7],bit[0] */
	uint64_t CorrECCErr:1;      /**< byte[0-7],bit[1] */
	uint64_t UncECCErr:1;       /**< byte[0-7],bit[2] */
	uint64_t BPBSat:1;          /**< byte[0-7],bit[3] */
	uint64_t TBNegSat:1;        /**< byte[0-7],bit[4] */
	uint64_t FIFOOvrflowErr:1;  /**< byte[0-7],bit[5] */
	uint64_t _reserved_1:58;    /**< byte[0-7],bit[6-63] */
}; /* PLID#1247 */

/** NPU Register payload TM_Drop_PortREDCurve (PLID#1423).
 *
 * Used by TM.Drop.PortREDCurve.
 */
struct TM_Drop_PortREDCurve {
	uint64_t Prob:6;            /**< byte[0-7],bit[0-5] */
	uint64_t _reserved_1:58;    /**< byte[0-7],bit[6-63] */
}; /* PLID#1423 */

/** NPU Register payload TM_Sched_BlvlEligPrioFunc (PLID#1394).
 *
 * Used by TM.Sched.BlvlEligPrioFunc.
 */
struct TM_Sched_BlvlEligPrioFunc {
	uint64_t FuncOut0:9;        /**< byte[0-7],bit[0-8] */
	uint64_t _reserved_1:7;     /**< byte[0-7],bit[9-15] */
	uint64_t FuncOut1:9;        /**< byte[0-7],bit[16-24] */
	uint64_t _reserved_2:7;     /**< byte[0-7],bit[25-31] */
	uint64_t FuncOut2:9;        /**< byte[0-7],bit[32-40] */
	uint64_t _reserved_3:7;     /**< byte[0-7],bit[41-47] */
	uint64_t FuncOut3:9;        /**< byte[0-7],bit[48-56] */
	uint64_t _reserved_4:7;     /**< byte[0-7],bit[57-63] */
}; /* PLID#1394 */

/** NPU Register payload TM_Drop_BlvlInstAndAvgQueueLength (PLID#1429).
 *
 * Used by TM.Drop.BlvlInstAndAvgQueueLength.
 */
struct TM_Drop_BlvlInstAndAvgQueueLength {
	uint64_t QL:29;             /**< byte[0-7],bit[0-28] */
	uint64_t _reserved_1:3;     /**< byte[0-7],bit[29-31] */
	uint64_t AQL:29;            /**< byte[0-7],bit[32-60] */
	uint64_t _reserved_2:3;     /**< byte[0-7],bit[61-63] */
}; /* PLID#1429 */

/** NPU Register payload TM_Sched_PortPerStatus (PLID#1240).
 *
 * Used by TM.Sched.PortPerStatus.
 */
struct TM_Sched_PortPerStatus {
	uint64_t PerPointer:6;      /**< byte[0-7],bit[0-5] */
	uint64_t _reserved_1:58;    /**< byte[0-7],bit[6-63] */
}; /* PLID#1240 */

/** NPU Register payload TM_Sched_BlvlDWRRPrioEn (PLID#1398).
 *
 * Used by TM.Sched.BlvlDWRRPrioEn.
 */
struct TM_Sched_BlvlDWRRPrioEn {
	uint64_t En:8;              /**< byte[0-7],bit[0-7] */
	uint64_t _reserved_1:56;    /**< byte[0-7],bit[8-63] */
}; /* PLID#1398 */

/** NPU Register payload TM_Drop_AgingUpdEnable (PLID#1251).
 *
 * Used by TM.Drop.AgingUpdEnable.
 */
struct TM_Drop_AgingUpdEnable {
	uint64_t En:1;              /**< byte[0-7],bit[0] */
	uint64_t _reserved_1:63;    /**< byte[0-7],bit[1-63] */
}; /* PLID#1251 */

#endif /* TM_PAYLOADS_H */
