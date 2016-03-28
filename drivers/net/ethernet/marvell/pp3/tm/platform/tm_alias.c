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

#include "tm_alias.h"
#include "tm_os_interface.h"

struct tm_alias TM;
struct tm_alias tm_index_offset;


/** Register alias definition for the RevA TM unit.
 */
void init_tm_alias_struct(void __iomem *base)
{
	TM.silicon_base = base;

	TM.Drop.AlvlDropPrfTailDrpThresh = 0x00491200;
	TM.Drop.AlvlDropPrfWREDDPRatio = 0x004911C0;
	TM.Drop.AlvlDropPrfWREDMinThresh = 0x00491180;
	TM.Drop.AlvlDropPrfWREDParams = 0x00491100;
	TM.Drop.AlvlDropPrfWREDScaleRatio = 0x00491140;
	TM.Drop.AlvlDropProb = 0x0049BC00;
	TM.Drop.AlvlDropProfPtr = 0x00491000;
	TM.Drop.AlvlInstAndAvgQueueLength = 0x0049B800;
	TM.Drop.AlvlREDCurve.Color[0] = 0x00492000; /* Color[0] */
	TM.Drop.AlvlREDCurve.Color[1] = 0x00492800; /* Color[1] */
	TM.Drop.AlvlREDCurve.Color[2] =  0x00493000; /* Color[2] */
	TM.Drop.BlvlDropPrfTailDrpThresh = 0x00490140;
	TM.Drop.BlvlDropPrfWREDDPRatio = 0x00490100;
	TM.Drop.BlvlDropPrfWREDMinThresh = 0x004900C0;
	TM.Drop.BlvlDropPrfWREDParams = 0x00490040;
	TM.Drop.BlvlDropPrfWREDScaleRatio = 0x00490080;
	TM.Drop.BlvlDropProb = 0x0049B500;
	TM.Drop.BlvlDropProfPtr = 0x00490000;
	TM.Drop.BlvlInstAndAvgQueueLength = 0x0049B400;
	TM.Drop.BlvlREDCurve[0].Table = 0x00490400;
	TM.Drop.BlvlREDCurve[1].Table = 0x00490500;
	TM.Drop.BlvlREDCurve[2].Table = 0x00490600;
	TM.Drop.BlvlREDCurve[3].Table = 0x00490700;
	TM.Drop.ClvlDropPrfTailDrpThresh.CoS[0] = 0x00488A80; /* CoS[0] */
	TM.Drop.ClvlDropPrfTailDrpThresh.CoS[1] =  0x00488A90; /* CoS[1] */
	TM.Drop.ClvlDropPrfTailDrpThresh.CoS[2] = 0x00488AA0; /* CoS[2] */
	TM.Drop.ClvlDropPrfTailDrpThresh.CoS[3] = 0x00488AB0; /* CoS[3] */
	TM.Drop.ClvlDropPrfTailDrpThresh.CoS[4] = 0x00488AC0; /* CoS[4] */
	TM.Drop.ClvlDropPrfTailDrpThresh.CoS[5] = 0x00488AD0; /* CoS[5] */
	TM.Drop.ClvlDropPrfTailDrpThresh.CoS[6] = 0x00488AE0; /* CoS[6] */
	TM.Drop.ClvlDropPrfTailDrpThresh.CoS[7] = 0x00488AF0; /* CoS[7] */
	TM.Drop.ClvlDropPrfWREDDPRatio.CoS[0] = 0x00488A00; /* CoS[0] */
	TM.Drop.ClvlDropPrfWREDDPRatio.CoS[1] = 0x00488A10; /* CoS[1] */
	TM.Drop.ClvlDropPrfWREDDPRatio.CoS[2] = 0x00488A20; /* CoS[2] */
	TM.Drop.ClvlDropPrfWREDDPRatio.CoS[3] =  0x00488A30; /* CoS[3] */
	TM.Drop.ClvlDropPrfWREDDPRatio.CoS[4] = 0x00488A40; /* CoS[4] */
	TM.Drop.ClvlDropPrfWREDDPRatio.CoS[5] = 0x00488A50; /* CoS[5] */
	TM.Drop.ClvlDropPrfWREDDPRatio.CoS[6] = 0x00488A60; /* CoS[6] */
	TM.Drop.ClvlDropPrfWREDDPRatio.CoS[7] = 0x00488A70; /* CoS[7] */
	TM.Drop.ClvlDropPrfWREDMinThresh.CoS[0] = 0x00488980; /* CoS[0] */
	TM.Drop.ClvlDropPrfWREDMinThresh.CoS[1] = 0x00488990; /* CoS[1] */
	TM.Drop.ClvlDropPrfWREDMinThresh.CoS[2] = 0x004889A0; /* CoS[2] */
	TM.Drop.ClvlDropPrfWREDMinThresh.CoS[3] = 0x004889B0; /* CoS[3] */
	TM.Drop.ClvlDropPrfWREDMinThresh.CoS[4] = 0x004889C0; /* CoS[4] */
	TM.Drop.ClvlDropPrfWREDMinThresh.CoS[5] = 0x004889D0; /* CoS[5] */
	TM.Drop.ClvlDropPrfWREDMinThresh.CoS[6] = 0x004889E0; /* CoS[6] */
	TM.Drop.ClvlDropPrfWREDMinThresh.CoS[7] = 0x004889F0; /* CoS[7] */
	TM.Drop.ClvlDropPrfWREDParams.CoS[0] =  0x00488880; /* CoS[0] */
	TM.Drop.ClvlDropPrfWREDParams.CoS[1] =  0x00488890; /* CoS[1] */
	TM.Drop.ClvlDropPrfWREDParams.CoS[2] =  0x004888A0; /* CoS[2] */
	TM.Drop.ClvlDropPrfWREDParams.CoS[3] =  0x004888B0; /* CoS[3] */
	TM.Drop.ClvlDropPrfWREDParams.CoS[4] =  0x004888C0; /* CoS[4] */
	TM.Drop.ClvlDropPrfWREDParams.CoS[5] =  0x004888D0; /* CoS[5] */
	TM.Drop.ClvlDropPrfWREDParams.CoS[6] =  0x004888E0; /* CoS[6] */
	TM.Drop.ClvlDropPrfWREDParams.CoS[7] =  0x004888F0; /* CoS[7] */
	TM.Drop.ClvlDropPrfWREDScaleRatio.CoS[0] = 0x00488900; /* CoS[0] */
	TM.Drop.ClvlDropPrfWREDScaleRatio.CoS[1] = 0x00488910; /* CoS[1] */
	TM.Drop.ClvlDropPrfWREDScaleRatio.CoS[2] = 0x00488920; /* CoS[2] */
	TM.Drop.ClvlDropPrfWREDScaleRatio.CoS[3] = 0x00488930; /* CoS[3] */
	TM.Drop.ClvlDropPrfWREDScaleRatio.CoS[4] = 0x00488940; /* CoS[4] */
	TM.Drop.ClvlDropPrfWREDScaleRatio.CoS[5] = 0x00488950; /* CoS[5] */
	TM.Drop.ClvlDropPrfWREDScaleRatio.CoS[6] = 0x00488960; /* CoS[6] */
	TM.Drop.ClvlDropPrfWREDScaleRatio.CoS[7] = 0x00488970; /* CoS[7] */
	TM.Drop.ClvlDropProb = 0x0049B000;
	TM.Drop.ClvlDropProfPtr_CoS[0] = 0x00488800; /* ClvlDropProfPtr_CoS[0] */
	TM.Drop.ClvlDropProfPtr_CoS[1] = 0x00488810; /* ClvlDropProfPtr_CoS[1] */
	TM.Drop.ClvlDropProfPtr_CoS[2] = 0x00488820; /* ClvlDropProfPtr_CoS[2] */
	TM.Drop.ClvlDropProfPtr_CoS[3] = 0x00488830; /* ClvlDropProfPtr_CoS[3] */
	TM.Drop.ClvlDropProfPtr_CoS[4] = 0x00488840; /* ClvlDropProfPtr_CoS[4] */
	TM.Drop.ClvlDropProfPtr_CoS[5] = 0x00488850; /* ClvlDropProfPtr_CoS[5] */
	TM.Drop.ClvlDropProfPtr_CoS[6] = 0x00488860; /* ClvlDropProfPtr_CoS[6] */
	TM.Drop.ClvlDropProfPtr_CoS[7] = 0x00488870; /* ClvlDropProfPtr_CoS[7] */
	TM.Drop.ClvlInstAndAvgQueueLength = 0x0049AC00;
	TM.Drop.ClvlREDCurve.CoS[0] = 0x0048A000; /* CoS[0] */
	TM.Drop.ClvlREDCurve.CoS[1] = 0x0048A400; /* CoS[1] */
	TM.Drop.ClvlREDCurve.CoS[2] = 0x0048A800; /* CoS[2] */
	TM.Drop.ClvlREDCurve.CoS[3] = 0x0048AC00; /* CoS[3] */
	TM.Drop.ClvlREDCurve.CoS[4] = 0x0048B000; /* CoS[4] */
	TM.Drop.ClvlREDCurve.CoS[5] = 0x0048B400; /* CoS[5] */
	TM.Drop.ClvlREDCurve.CoS[6] = 0x0048B800; /* CoS[6] */
	TM.Drop.ClvlREDCurve.CoS[7] = 0x0048BC00; /* CoS[7] */
	TM.Drop.DPSource = 0x00480048;
	TM.Drop.Drp_Decision_hierarchy_to_Query_debug = 0x00480060;
	TM.Drop.Drp_Decision_to_Query_debug = 0x00480058;
	TM.Drop.EccConfig = 0x0049E000;
	TM.Drop.ErrCnt = 0x00480010;
	TM.Drop.ErrStus = 0x00480000;
	TM.Drop.ExcCnt = 0x00480018;
	TM.Drop.ExcMask = 0x00480020;
	TM.Drop.FirstExc = 0x00480008;
	TM.Drop.ForceErr = 0x00480030;
	TM.Drop.Id = 0x00480028;
	TM.Drop.PortDropPrfTailDrpThresh = 0x00488200;
	TM.Drop.PortDropPrfTailDrpThresh_CoSRes[0] = 0x00485000; /* PortDropPrfTailDrpThresh_CoSRes[0] */
	TM.Drop.PortDropPrfTailDrpThresh_CoSRes[1] = 0x00485080; /* PortDropPrfTailDrpThresh_CoSRes[1] */
	TM.Drop.PortDropPrfTailDrpThresh_CoSRes[2] = 0x00485100; /* PortDropPrfTailDrpThresh_CoSRes[2] */
	TM.Drop.PortDropPrfTailDrpThresh_CoSRes[3] = 0x00485180; /* PortDropPrfTailDrpThresh_CoSRes[3] */
	TM.Drop.PortDropPrfTailDrpThresh_CoSRes[4] = 0x00485200; /* PortDropPrfTailDrpThresh_CoSRes[4] */
	TM.Drop.PortDropPrfTailDrpThresh_CoSRes[5] = 0x00485280; /* PortDropPrfTailDrpThresh_CoSRes[5] */
	TM.Drop.PortDropPrfTailDrpThresh_CoSRes[6] = 0x00485300; /* PortDropPrfTailDrpThresh_CoSRes[6] */
	TM.Drop.PortDropPrfTailDrpThresh_CoSRes[7] = 0x00485380; /* PortDropPrfTailDrpThresh_CoSRes[7] */
	TM.Drop.PortDropPrfWREDDPRatio = 0x00488180;
	TM.Drop.PortDropPrfWREDDPRatio_CoSRes[0] = 0x00484C00; /* PortDropPrfWREDDPRatio_CoSRes[0] */
	TM.Drop.PortDropPrfWREDDPRatio_CoSRes[1] = 0x00484C80; /* PortDropPrfWREDDPRatio_CoSRes[1] */
	TM.Drop.PortDropPrfWREDDPRatio_CoSRes[2] = 0x00484D00; /* PortDropPrfWREDDPRatio_CoSRes[2] */
	TM.Drop.PortDropPrfWREDDPRatio_CoSRes[3] = 0x00484D80; /* PortDropPrfWREDDPRatio_CoSRes[3] */
	TM.Drop.PortDropPrfWREDDPRatio_CoSRes[4] = 0x00484E00; /* PortDropPrfWREDDPRatio_CoSRes[4] */
	TM.Drop.PortDropPrfWREDDPRatio_CoSRes[5] = 0x00484E80; /* PortDropPrfWREDDPRatio_CoSRes[5] */
	TM.Drop.PortDropPrfWREDDPRatio_CoSRes[6] = 0x00484F00; /* PortDropPrfWREDDPRatio_CoSRes[6] */
	TM.Drop.PortDropPrfWREDDPRatio_CoSRes[7] = 0x00484F80; /* PortDropPrfWREDDPRatio_CoSRes[7] */
	TM.Drop.PortDropPrfWREDMinThresh = 0x00488100;
	TM.Drop.PortDropPrfWREDMinThresh_CoSRes[0] = 0x00484800; /* PortDropPrfWREDMinThresh_CoSRes[0] */
	TM.Drop.PortDropPrfWREDMinThresh_CoSRes[1] = 0x00484880; /* PortDropPrfWREDMinThresh_CoSRes[1] */
	TM.Drop.PortDropPrfWREDMinThresh_CoSRes[2] = 0x00484900; /* PortDropPrfWREDMinThresh_CoSRes[2] */
	TM.Drop.PortDropPrfWREDMinThresh_CoSRes[3] = 0x00484980; /* PortDropPrfWREDMinThresh_CoSRes[3] */
	TM.Drop.PortDropPrfWREDMinThresh_CoSRes[4] = 0x00484A00; /* PortDropPrfWREDMinThresh_CoSRes[4] */
	TM.Drop.PortDropPrfWREDMinThresh_CoSRes[5] = 0x00484A80; /* PortDropPrfWREDMinThresh_CoSRes[5] */
	TM.Drop.PortDropPrfWREDMinThresh_CoSRes[6] = 0x00484B00; /* PortDropPrfWREDMinThresh_CoSRes[6] */
	TM.Drop.PortDropPrfWREDMinThresh_CoSRes[7] = 0x00484B80; /* PortDropPrfWREDMinThresh_CoSRes[7] */
	TM.Drop.PortDropPrfWREDParams = 0x00488000;
	TM.Drop.PortDropPrfWREDParams_CoSRes[0] = 0x00484000; /* PortDropPrfWREDParams_CoSRes[0] */
	TM.Drop.PortDropPrfWREDParams_CoSRes[1] = 0x00484080; /* PortDropPrfWREDParams_CoSRes[1] */
	TM.Drop.PortDropPrfWREDParams_CoSRes[2] = 0x00484100; /* PortDropPrfWREDParams_CoSRes[2] */
	TM.Drop.PortDropPrfWREDParams_CoSRes[3] = 0x00484180; /* PortDropPrfWREDParams_CoSRes[3] */
	TM.Drop.PortDropPrfWREDParams_CoSRes[4] = 0x00484200; /* PortDropPrfWREDParams_CoSRes[4] */
	TM.Drop.PortDropPrfWREDParams_CoSRes[5] = 0x00484280; /* PortDropPrfWREDParams_CoSRes[5] */
	TM.Drop.PortDropPrfWREDParams_CoSRes[6] = 0x00484300; /* PortDropPrfWREDParams_CoSRes[6] */
	TM.Drop.PortDropPrfWREDParams_CoSRes[7] = 0x00484380; /* PortDropPrfWREDParams_CoSRes[7] */
	TM.Drop.PortDropPrfWREDScaleRatio = 0x00488080;
	TM.Drop.PortDropPrfWREDScaleRatio_CoSRes[0] = 0x00484400; /* PortDropPrfWREDScaleRatio_CoSRes[0] */
	TM.Drop.PortDropPrfWREDScaleRatio_CoSRes[1] = 0x00484480; /* PortDropPrfWREDScaleRatio_CoSRes[1] */
	TM.Drop.PortDropPrfWREDScaleRatio_CoSRes[2] = 0x00484500; /* PortDropPrfWREDScaleRatio_CoSRes[2] */
	TM.Drop.PortDropPrfWREDScaleRatio_CoSRes[3] = 0x00484580; /* PortDropPrfWREDScaleRatio_CoSRes[3] */
	TM.Drop.PortDropPrfWREDScaleRatio_CoSRes[4] = 0x00484600; /* PortDropPrfWREDScaleRatio_CoSRes[4] */
	TM.Drop.PortDropPrfWREDScaleRatio_CoSRes[5] = 0x00484600; /* PortDropPrfWREDScaleRatio_CoSRes[5] */
	TM.Drop.PortDropPrfWREDScaleRatio_CoSRes[6] = 0x00484700; /* PortDropPrfWREDScaleRatio_CoSRes[6] */
	TM.Drop.PortDropPrfWREDScaleRatio_CoSRes[7] = 0x00484780; /* PortDropPrfWREDScaleRatio_CoSRes[7] */
	TM.Drop.PortDropProb = 0x0049A080;
	TM.Drop.PortDropProbPerCoS_CoS[0] = 0x0049A800; /* CoS[0] */
	TM.Drop.PortDropProbPerCoS_CoS[1] = 0x0049A880; /* CoS[1] */
	TM.Drop.PortDropProbPerCoS_CoS[2] = 0x0049A900; /* CoS[2] */
	TM.Drop.PortDropProbPerCoS_CoS[3] = 0x0049A980; /* CoS[3] */
	TM.Drop.PortDropProbPerCoS_CoS[4] = 0x0049AA00; /* CoS[4] */
	TM.Drop.PortDropProbPerCoS_CoS[5] = 0x0049AA80; /* CoS[5] */
	TM.Drop.PortDropProbPerCoS_CoS[6] = 0x0049AB00; /* CoS[6] */
	TM.Drop.PortDropProbPerCoS_CoS[7] = 0x0049AB80; /* CoS[7] */
	TM.Drop.PortInstAndAvgQueueLength = 0x0049A000;
	TM.Drop.PortInstAndAvgQueueLengthPerCoS.CoS[0] = 0x0049A400; /* CoS[0] */
	TM.Drop.PortInstAndAvgQueueLengthPerCoS.CoS[1] = 0x0049A480; /* CoS[1] */
	TM.Drop.PortInstAndAvgQueueLengthPerCoS.CoS[2] = 0x0049A500; /* CoS[2] */
	TM.Drop.PortInstAndAvgQueueLengthPerCoS.CoS[3] = 0x0049A580; /* CoS[3] */
	TM.Drop.PortInstAndAvgQueueLengthPerCoS.CoS[4] = 0x0049A600; /* CoS[4] */
	TM.Drop.PortInstAndAvgQueueLengthPerCoS.CoS[5] = 0x0049A680; /* CoS[5] */
	TM.Drop.PortInstAndAvgQueueLengthPerCoS.CoS[6] = 0x0049A700; /* CoS[6] */
	TM.Drop.PortInstAndAvgQueueLengthPerCoS.CoS[7] = 0x0049A780; /* CoS[7] */
	TM.Drop.PortREDCurve = 0x00488400;
	TM.Drop.PortREDCurve_CoS[0] = 0x00486000; /* PortREDCurve_CoS[0] */
	TM.Drop.PortREDCurve_CoS[1] = 0x00486400; /* PortREDCurve_CoS[1] */
	TM.Drop.PortREDCurve_CoS[2] = 0x00486800; /* PortREDCurve_CoS[2] */
	TM.Drop.PortREDCurve_CoS[3] = 0x00486C00; /* PortREDCurve_CoS[3] */
	TM.Drop.PortREDCurve_CoS[4] = 0x00487000; /* PortREDCurve_CoS[4] */
	TM.Drop.PortREDCurve_CoS[5] = 0x00487400; /* PortREDCurve_CoS[5] */
	TM.Drop.PortREDCurve_CoS[6] = 0x00487800; /* PortREDCurve_CoS[6] */
	TM.Drop.PortREDCurve_CoS[7] = 0x00487C00; /* PortREDCurve_CoS[7] */
	TM.Drop.QueueAvgQueueLength = 0x0049C000;
	TM.Drop.QueueCoSConf = 0x00498000;
	TM.Drop.QueueDropPrfTailDrpThresh = 0x00494600;
	TM.Drop.QueueDropPrfWREDDPRatio = 0x00494580;
	TM.Drop.QueueDropPrfWREDMinThresh = 0x00494500;
	TM.Drop.QueueDropPrfWREDParams = 0x00494400;
	TM.Drop.QueueDropPrfWREDScaleRatio = 0x00494480;
	TM.Drop.QueueDropProb = 0x0049D000;
	TM.Drop.QueueDropProfPtr = 0x00494000;
	TM.Drop.QueueREDCurve.Color[0] = 0x00496000; /* Color[0] */
	TM.Drop.QueueREDCurve.Color[1] = 0x00496800; /* Color[1] */
	TM.Drop.QueueREDCurve.Color[2] = 0x00497000; /* Color[2] */
	TM.Drop.RespLocalDPSel = 0x00480050;
	TM.Drop.WREDDropProbMode = 0x00480038;
	TM.Drop.WREDMaxProbModePerColor = 0x00480040;

	TM.Sched.ALevelShaperBucketNeg = 0x0045AC00;
	TM.Sched.ALvltoBlvlAndQueueRangeMap = 0x00453400;
	TM.Sched.AlvlDWRRPrioEn = 0x00452C00;
	TM.Sched.AlvlDef = 0x0045B000;
	TM.Sched.AlvlEligPrioFunc = 0x00451000;
	TM.Sched.AlvlEligPrioFuncPtr = 0x00452000;
	TM.Sched.AlvlPerConf = 0x00450000;
	TM.Sched.AlvlPerRateShpPrms = 0x00450008;
	TM.Sched.AlvlPerRateShpPrmsInt = 0x00463BA0;
	TM.Sched.AlvlQuantum = 0x00453000;
	TM.Sched.AlvlShpBucketLvls = 0x0045A800;
	TM.Sched.AlvlTokenBucketBurstSize = 0x00452800;
	TM.Sched.AlvlTokenBucketTokenEnDiv = 0x00452400; //0x00449100;
	TM.Sched.BLevelShaperBucketNeg = 0x0045A500;
	TM.Sched.BLvltoClvlAndAlvlRangeMap = 0x00449500;
	TM.Sched.BlvlDWRRPrioEn = 0x00449300;
	TM.Sched.BlvlDef = 0x0045A600;
	TM.Sched.BlvlEligPrioFunc = 0x00448000;
	TM.Sched.BlvlEligPrioFuncPtr = 0x00449000;
	TM.Sched.BlvlPerConf = 0x00447000;
	TM.Sched.BlvlPerRateShpPrms = 0x00447008;
	TM.Sched.BlvlPerRateShpPrmsInt = 0x004616E0;
	TM.Sched.BlvlQuantum = 0x00449400;
	TM.Sched.BlvlShpBucketLvls = 0x0045A400;
	TM.Sched.BlvlTokenBucketBurstSize = 0x00449200;
	TM.Sched.BlvlTokenBucketTokenEnDiv = 0x00449100;
	TM.Sched.CLevelShaperBucketNeg = 0x0045A280;
	TM.Sched.CLvlDef = 0x0045A300;
	TM.Sched.ClvlDWRRPrioEn = 0x00446180;
	TM.Sched.ClvlEligPrioFunc = 0x00445000;
	TM.Sched.ClvlEligPrioFuncPtr = 0x00446000;
	TM.Sched.ClvlPerConf = 0x00444000;
	TM.Sched.ClvlPerRateShpPrms = 0x00444008;
	TM.Sched.ClvlPerRateShpPrmsInt = 0x00460B60;
	TM.Sched.ClvlQuantum = 0x00446200;
	TM.Sched.ClvlShpBucketLvls = 0x0045A200;
	TM.Sched.ClvlTokenBucketBurstSize = 0x00446100;
	TM.Sched.ClvlTokenBucketTokenEnDiv = 0x00446080;
	TM.Sched.ClvltoPortAndBlvlRangeMap = 0x00446280;
	TM.Sched.EccConfig = 0x00470000;
	TM.Sched.ErrCnt = 0x00440018;
	TM.Sched.ErrStus = 0x00440000;
	TM.Sched.ExcCnt = 0x00440010;
	TM.Sched.ExcMask = 0x00440020;
	TM.Sched.FirstExc = 0x00440008;
	TM.Sched.ForceErr = 0x00440030;
	TM.Sched.Id = 0x00440028;
	TM.Sched.PortDWRRBytesPerBurstsLimit = 0x00441018;
	TM.Sched.PortDWRRPrioEn = 0x00443180;
	TM.Sched.PortDefPrioHi = 0x0045A180;
	TM.Sched.PortDefPrioLo = 0x0045A100;
	TM.Sched.PortEligPrioFunc = 0x00442000;
	TM.Sched.PortEligPrioFuncPtr = 0x00443000;
	TM.Sched.PortExtBPEn = 0x00441010;
	TM.Sched.PortPerConf = 0x00441000;
	TM.Sched.PortPerRateShpPrms = 0x00441008;
	TM.Sched.PortPerRateShpPrmsInt = 0x00460308;
	TM.Sched.PortQuantumsPriosHi = 0x00443280;
	TM.Sched.PortQuantumsPriosLo = 0x00443200;
	TM.Sched.PortRangeMap = 0x00443300;
	TM.Sched.PortShaperBucketNeg = 0x0045A080;
	TM.Sched.PortShpBucketLvls = 0x0045A000;
	TM.Sched.PortTokenBucketBurstSize = 0x00443100;
	TM.Sched.PortTokenBucketTokenEnDiv = 0x00443080;
	TM.Sched.QueueAMap = 0x00459000;
	TM.Sched.QueueDef = 0x0045E000;
	TM.Sched.QueueEligPrioFunc = 0x00454200;
	TM.Sched.QueueEligPrioFuncPtr = 0x00455000;
	TM.Sched.QueuePerConf = 0x00454000;
	TM.Sched.QueuePerRateShpPrms = 0x00454008;
	TM.Sched.QueuePerRateShpPrmsInt = 0x00465748;
	TM.Sched.QueueQuantum = 0x00458000;
	TM.Sched.QueueShaperBucketNeg = 0x0045D000;
	TM.Sched.QueueShpBucketLvls = 0x0045C000;
	TM.Sched.QueueTokenBucketBurstSize = 0x00457000;
	TM.Sched.QueueTokenBucketTokenEnDiv = 0x00456000;
	TM.Sched.ScrubDisable = 0x0045F008;
	TM.Sched.ScrubSlotAlloc = 0x00440038;
	TM.Sched.TreeDWRRPrioEn = 0x00440050;
	TM.Sched.TreeDeqEn = 0x00440048;
}

void init_tm_init_offset_struct()
{
	int i;
	tm_memset(&tm_index_offset, 0, sizeof(tm_index_offset));

	tm_index_offset.Drop.QueueDropPrfWREDParams = 0x8;
	tm_index_offset.Drop.QueueDropPrfWREDScaleRatio = 0x8;
	tm_index_offset.Drop.QueueDropPrfWREDMinThresh = 0x8;
	tm_index_offset.Drop.QueueDropPrfTailDrpThresh = 0x8;
	tm_index_offset.Drop.QueueDropPrfWREDDPRatio = 0x8;
	for (i = 0; i < 3; i++)
		tm_index_offset.Drop.QueueREDCurve.Color[i] = 0x8;

	tm_index_offset.Drop.AlvlDropPrfWREDParams = 0x8;
	tm_index_offset.Drop.AlvlDropPrfWREDScaleRatio = 0x8;
	tm_index_offset.Drop.AlvlDropPrfWREDMinThresh = 0x8;
	tm_index_offset.Drop.AlvlDropPrfTailDrpThresh = 0x8;
	tm_index_offset.Drop.AlvlDropPrfWREDDPRatio = 0x8;
	for (i = 0; i < 3; i++)
		tm_index_offset.Drop.AlvlREDCurve.Color[i] = 0x8;

	tm_index_offset.Drop.BlvlDropPrfWREDParams = 0x8;
	tm_index_offset.Drop.BlvlDropPrfWREDScaleRatio = 0x8;
	tm_index_offset.Drop.BlvlDropPrfWREDMinThresh = 0x8;
	tm_index_offset.Drop.BlvlDropPrfTailDrpThresh = 0x8;
	tm_index_offset.Drop.BlvlDropPrfWREDDPRatio = 0x8;
	for (i = 0; i < 3; i++)
		tm_index_offset.Drop.BlvlREDCurve[i].Table = 0x8;

	for (i = 0; i < 8; i++) {
		tm_index_offset.Drop.ClvlDropPrfWREDParams.CoS[i] = 0x8;
		tm_index_offset.Drop.ClvlDropPrfWREDScaleRatio.CoS[i] = 0x8;
		tm_index_offset.Drop.ClvlDropPrfWREDMinThresh.CoS[i] = 0x8;
		tm_index_offset.Drop.ClvlDropPrfTailDrpThresh.CoS[i] = 0x8;
		tm_index_offset.Drop.ClvlDropPrfWREDDPRatio.CoS[i] = 0x8;
		tm_index_offset.Drop.ClvlREDCurve.CoS[i] = 0x8;
	}


	tm_index_offset.Drop.PortREDCurve = 0x8;
	tm_index_offset.Drop.PortDropPrfWREDParams = 0x8;
	tm_index_offset.Drop.PortDropPrfWREDScaleRatio = 0x8;
	tm_index_offset.Drop.PortDropPrfWREDMinThresh = 0x8;
	tm_index_offset.Drop.PortDropPrfTailDrpThresh = 0x8;
	tm_index_offset.Drop.PortDropPrfWREDDPRatio = 0x8;

	for (i = 0; i < 8; i++) {
		tm_index_offset.Drop.PortREDCurve_CoS[i] = 0x8;
		tm_index_offset.Drop.PortDropPrfWREDParams_CoSRes[i] = 0x8;
		tm_index_offset.Drop.PortDropPrfWREDScaleRatio_CoSRes[i] = 0x8;
		tm_index_offset.Drop.PortDropPrfWREDMinThresh_CoSRes[i] = 0x8;
		tm_index_offset.Drop.PortDropPrfWREDDPRatio_CoSRes[i]  = 0x8;
		tm_index_offset.Drop.PortDropPrfTailDrpThresh_CoSRes[i] = 0x8;
	}
	tm_index_offset.Sched.QueueTokenBucketTokenEnDiv = 0x8;
	tm_index_offset.Sched.QueueTokenBucketBurstSize = 0x8;
	tm_index_offset.Sched.AlvlTokenBucketTokenEnDiv = 0x8;
	tm_index_offset.Sched.AlvlTokenBucketBurstSize = 0x8;
	tm_index_offset.Sched.BlvlTokenBucketTokenEnDiv = 0x8;
	tm_index_offset.Sched.BlvlTokenBucketBurstSize = 0x8;
	tm_index_offset.Sched.ClvlTokenBucketTokenEnDiv = 0x8;
	tm_index_offset.Sched.ClvlTokenBucketBurstSize = 0x8;

	tm_index_offset.Sched.PortRangeMap = 0x8;
	tm_index_offset.Sched.ClvltoPortAndBlvlRangeMap = 0x8;
	tm_index_offset.Sched.BLvltoClvlAndAlvlRangeMap = 0x8;
	tm_index_offset.Sched.ALvltoBlvlAndQueueRangeMap = 0x8;
	tm_index_offset.Sched.QueueAMap = 0x8;

	tm_index_offset.Sched.QueueEligPrioFunc = 0x8;
	tm_index_offset.Sched.QueueEligPrioFuncPtr = 0x8;
	tm_index_offset.Sched.QueueQuantum = 0x8;
	tm_index_offset.Drop.QueueDropProfPtr = 0x8;
	tm_index_offset.Drop.QueueAvgQueueLength = 0x8;

	tm_index_offset.Sched.ALvltoBlvlAndQueueRangeMap = 0x8;
	tm_index_offset.Sched.AlvlEligPrioFunc = 0x8;
	tm_index_offset.Sched.AlvlEligPrioFuncPtr = 0x8;
	tm_index_offset.Sched.AlvlQuantum = 0x8;
	tm_index_offset.Sched.AlvlDWRRPrioEn = 0x8;
	tm_index_offset.Drop.AlvlDropProfPtr = 0x8;
	tm_index_offset.Drop.AlvlInstAndAvgQueueLength = 0x8;

	tm_index_offset.Sched.BLvltoClvlAndAlvlRangeMap = 0x8;
	tm_index_offset.Sched.BlvlEligPrioFunc = 0x8;
	tm_index_offset.Sched.BlvlEligPrioFuncPtr = 0x8;
	tm_index_offset.Sched.BlvlQuantum = 0x8;
	tm_index_offset.Sched.BlvlDWRRPrioEn = 0x8;
	tm_index_offset.Drop.BlvlDropProfPtr = 0x8;
	tm_index_offset.Drop.BlvlInstAndAvgQueueLength = 0x8;

	tm_index_offset.Sched.ClvltoPortAndBlvlRangeMap = 0x8;
	tm_index_offset.Sched.ClvlEligPrioFunc = 0x8;
	tm_index_offset.Sched.ClvlEligPrioFuncPtr = 0x8;
	tm_index_offset.Sched.ClvlQuantum = 0x8;
	tm_index_offset.Sched.ClvlDWRRPrioEn = 0x8;
	for (i = 0; i < 8; i++)
		tm_index_offset.Drop.ClvlDropProfPtr_CoS[i] = 0x8;
	tm_index_offset.Drop.ClvlInstAndAvgQueueLength = 0x8;

	tm_index_offset.Sched.PortEligPrioFunc = 0x8;
	tm_index_offset.Sched.PortEligPrioFuncPtr = 0x8;

	tm_index_offset.Sched.PortTokenBucketTokenEnDiv = 0x8;
	tm_index_offset.Sched.PortTokenBucketBurstSize = 0x8;

	tm_index_offset.Sched.PortQuantumsPriosLo = 0x8;
	tm_index_offset.Sched.PortQuantumsPriosHi = 0x8;
	tm_index_offset.Sched.PortDWRRPrioEn = 0x8;
	tm_index_offset.Drop.PortInstAndAvgQueueLength = 0x8;

	tm_index_offset.Sched.PortDefPrioHi = 0x8;
	tm_index_offset.Sched.PortDefPrioLo = 0x8;

	tm_index_offset.Sched.CLvlDef = 0x8;
	tm_index_offset.Sched.BlvlDef = 0x8;
	tm_index_offset.Sched.AlvlDef = 0x8;
	tm_index_offset.Sched.QueueDef = 0x8;
	tm_index_offset.Drop.QueueCoSConf = 0x8; /* index is entry not the q number */

	tm_index_offset.Sched.ClvlShpBucketLvls = 0x8;
	tm_index_offset.Sched.PortShpBucketLvls = 0x8;
	tm_index_offset.Sched.BlvlShpBucketLvls = 0x8;
	tm_index_offset.Sched.AlvlShpBucketLvls = 0x8;

	tm_index_offset.Drop.AlvlDropProb = 0x8;
	tm_index_offset.Drop.ClvlDropProb = 0x8;
	tm_index_offset.Drop.BlvlDropProb = 0x8;

#ifdef SMADAR
	tm_index_offset.Drop.AlvlDropPrfWREDDPRatio = 0x40;
	tm_index_offset.Drop.AlvlDropPrfWREDMinThresh = 0x40;
	tm_index_offset.Drop.AlvlDropPrfWREDParams = 0x40;
	tm_index_offset.Drop.AlvlDropPrfWREDScaleRatio = 0x40;
	tm_index_offset.Drop.AlvlDropProb = 0x400;
	tm_index_offset.Drop.AlvlDropProfPtr = 0x8; /* 32 * 0x8 = 256B */
	tm_index_offset.Drop.AlvlInstAndAvgQueueLength = 0x400;
	tm_index_offset.Drop.AlvlREDCurve.Color[0] = 0x800; /* Color[0] */
	tm_index_offset.Drop.AlvlREDCurve.Color[1] = 0x800; /* Color[1] */
	tm_index_offset.Drop.AlvlREDCurve.Color[2] =  0x800; /* Color[2] */
	tm_index_offset.Drop.BlvlDropPrfTailDrpThresh = 0x00490140;
	tm_index_offset.Drop.BlvlDropPrfWREDDPRatio = 0x00490100;
	tm_index_offset.Drop.BlvlDropPrfWREDMinThresh = 0x004900C0;
	tm_index_offset.Drop.BlvlDropPrfWREDParams = 0x00490040;
	tm_index_offset.Drop.BlvlDropPrfWREDScaleRatio = 0x00490080;
	tm_index_offset.Drop.BlvlDropProb = 0x0049B500;
	tm_index_offset.Drop.BlvlDropProfPtr = 0x00490000;
	tm_index_offset.Drop.BlvlInstAndAvgQueueLength = 0x0049B400;
	tm_index_offset.Drop.BlvlREDCurve[0].Table = 0x00490400;
	tm_index_offset.Drop.BlvlREDCurve[1].Table = 0x00490500;
	tm_index_offset.Drop.BlvlREDCurve[2].Table = 0x00490600;
	tm_index_offset.Drop.BlvlREDCurve[3].Table = 0x00490700;
	tm_index_offset.Drop.ClvlDropPrfTailDrpThresh.CoS[0] = 0x00488A80; /* CoS[0] */
	tm_index_offset.Drop.ClvlDropPrfTailDrpThresh.CoS[1] =  0x00488A90; /* CoS[1] */
	tm_index_offset.Drop.ClvlDropPrfTailDrpThresh.CoS[2] = 0x00488AA0; /* CoS[2] */
	tm_index_offset.Drop.ClvlDropPrfTailDrpThresh.CoS[3] = 0x00488AB0; /* CoS[3] */
	tm_index_offset.Drop.ClvlDropPrfTailDrpThresh.CoS[4] = 0x00488AC0; /* CoS[4] */
	tm_index_offset.Drop.ClvlDropPrfTailDrpThresh.CoS[5] = 0x00488AD0; /* CoS[5] */
	tm_index_offset.Drop.ClvlDropPrfTailDrpThresh.CoS[6] = 0x00488AE0; /* CoS[6] */
	tm_index_offset.Drop.ClvlDropPrfTailDrpThresh.CoS[7] = 0x00488AF0; /* CoS[7] */
	tm_index_offset.Drop.ClvlDropPrfWREDDPRatio.CoS[0] = 0x00488A00; /* CoS[0] */
	tm_index_offset.Drop.ClvlDropPrfWREDDPRatio.CoS[1] = 0x00488A10; /* CoS[1] */
	tm_index_offset.Drop.ClvlDropPrfWREDDPRatio.CoS[2] = 0x00488A20; /* CoS[2] */
	tm_index_offset.Drop.ClvlDropPrfWREDDPRatio.CoS[3] =  0x00488A30; /* CoS[3] */
	tm_index_offset.Drop.ClvlDropPrfWREDDPRatio.CoS[4] = 0x00488A40; /* CoS[4] */
	tm_index_offset.Drop.ClvlDropPrfWREDDPRatio.CoS[5] = 0x00488A50; /* CoS[5] */
	tm_index_offset.Drop.ClvlDropPrfWREDDPRatio.CoS[6] = 0x00488A60; /* CoS[6] */
	tm_index_offset.Drop.ClvlDropPrfWREDDPRatio.CoS[7] = 0x00488A70; /* CoS[7] */
	tm_index_offset.Drop.ClvlDropPrfWREDMinThresh.CoS[0] = 0x00488980; /* CoS[0] */
	tm_index_offset.Drop.ClvlDropPrfWREDMinThresh.CoS[1] = 0x00488990; /* CoS[1] */
	tm_index_offset.Drop.ClvlDropPrfWREDMinThresh.CoS[2] = 0x004889A0; /* CoS[2] */
	tm_index_offset.Drop.ClvlDropPrfWREDMinThresh.CoS[3] = 0x004889B0; /* CoS[3] */
	tm_index_offset.Drop.ClvlDropPrfWREDMinThresh.CoS[4] = 0x004889C0; /* CoS[4] */
	tm_index_offset.Drop.ClvlDropPrfWREDMinThresh.CoS[5] = 0x004889D0; /* CoS[5] */
	tm_index_offset.Drop.ClvlDropPrfWREDMinThresh.CoS[6] = 0x004889E0; /* CoS[6] */
	tm_index_offset.Drop.ClvlDropPrfWREDMinThresh.CoS[7] = 0x004889F0; /* CoS[7] */
	tm_index_offset.Drop.ClvlDropPrfWREDParams.CoS[0] =  0x00488880; /* CoS[0] */
	tm_index_offset.Drop.ClvlDropPrfWREDParams.CoS[1] =  0x00488890; /* CoS[1] */
	tm_index_offset.Drop.ClvlDropPrfWREDParams.CoS[2] =  0x004888A0; /* CoS[2] */
	tm_index_offset.Drop.ClvlDropPrfWREDParams.CoS[3] =  0x004888B0; /* CoS[3] */
	tm_index_offset.Drop.ClvlDropPrfWREDParams.CoS[4] =  0x004888C0; /* CoS[4] */
	tm_index_offset.Drop.ClvlDropPrfWREDParams.CoS[5] =  0x004888D0; /* CoS[5] */
	tm_index_offset.Drop.ClvlDropPrfWREDParams.CoS[6] =  0x004888E0; /* CoS[6] */
	tm_index_offset.Drop.ClvlDropPrfWREDParams.CoS[7] =  0x004888F0; /* CoS[7] */
	tm_index_offset.Drop.ClvlDropPrfWREDScaleRatio.CoS[0] = 0x00488900; /* CoS[0] */
	tm_index_offset.Drop.ClvlDropPrfWREDScaleRatio.CoS[1] = 0x00488910; /* CoS[1] */
	tm_index_offset.Drop.ClvlDropPrfWREDScaleRatio.CoS[2] = 0x00488920; /* CoS[2] */
	tm_index_offset.Drop.ClvlDropPrfWREDScaleRatio.CoS[3] = 0x00488930; /* CoS[3] */
	tm_index_offset.Drop.ClvlDropPrfWREDScaleRatio.CoS[4] = 0x00488940; /* CoS[4] */
	tm_index_offset.Drop.ClvlDropPrfWREDScaleRatio.CoS[5] = 0x00488950; /* CoS[5] */
	tm_index_offset.Drop.ClvlDropPrfWREDScaleRatio.CoS[6] = 0x00488960; /* CoS[6] */
	tm_index_offset.Drop.ClvlDropPrfWREDScaleRatio.CoS[7] = 0x00488970; /* CoS[7] */
	tm_index_offset.Drop.ClvlDropProb = 0x0049B000;
	tm_index_offset.Drop.ClvlDropProfPtr_CoS[0] = 0x00488800; /* ClvlDropProfPtr_CoS[0] */
	tm_index_offset.Drop.ClvlDropProfPtr_CoS[1] = 0x00488810; /* ClvlDropProfPtr_CoS[1] */
	tm_index_offset.Drop.ClvlDropProfPtr_CoS[2] = 0x00488820; /* ClvlDropProfPtr_CoS[2] */
	tm_index_offset.Drop.ClvlDropProfPtr_CoS[3] = 0x00488830; /* ClvlDropProfPtr_CoS[3] */
	tm_index_offset.Drop.ClvlDropProfPtr_CoS[4] = 0x00488840; /* ClvlDropProfPtr_CoS[4] */
	tm_index_offset.Drop.ClvlDropProfPtr_CoS[5] = 0x00488850; /* ClvlDropProfPtr_CoS[5] */
	tm_index_offset.Drop.ClvlDropProfPtr_CoS[6] = 0x00488860; /* ClvlDropProfPtr_CoS[6] */
	tm_index_offset.Drop.ClvlDropProfPtr_CoS[7] = 0x00488870; /* ClvlDropProfPtr_CoS[7] */
	tm_index_offset.Drop.ClvlInstAndAvgQueueLength = 0x0049AC00;
	tm_index_offset.Drop.ClvlREDCurve.CoS[0] = 0x0048A000; /* CoS[0] */
	tm_index_offset.Drop.ClvlREDCurve.CoS[1] = 0x0048A400; /* CoS[1] */
	tm_index_offset.Drop.ClvlREDCurve.CoS[2] = 0x0048A800; /* CoS[2] */
	tm_index_offset.Drop.ClvlREDCurve.CoS[3] = 0x0048AC00; /* CoS[3] */
	tm_index_offset.Drop.ClvlREDCurve.CoS[4] = 0x0048B000; /* CoS[4] */
	tm_index_offset.Drop.ClvlREDCurve.CoS[5] = 0x0048B400; /* CoS[5] */
	tm_index_offset.Drop.ClvlREDCurve.CoS[6] = 0x0048B800; /* CoS[6] */
	tm_index_offset.Drop.ClvlREDCurve.CoS[7] = 0x0048BC00; /* CoS[7] */
	tm_index_offset.Drop.DPSource = 0x00480048;
	tm_index_offset.Drop.ErrCnt = 0x00480010;
	tm_index_offset.Drop.ErrStus = 0x00480000;
	tm_index_offset.Drop.ExcCnt = 0x00480018;
	tm_index_offset.Drop.ExcMask = 0x00480020;
	tm_index_offset.Drop.FirstExc = 0x00480008;
	tm_index_offset.Drop.ForceErr = 0x00480030;
	tm_index_offset.Drop.Id = 0x00480028;
	tm_index_offset.Drop.PortDropPrfTailDrpThresh = 0x00488200;
	tm_index_offset.Drop.PortDropPrfTailDrpThresh_CoSRes[0] = 0x00485000; /* PortDropPrfTailDrpThresh_CoSRes[0] */
	tm_index_offset.Drop.PortDropPrfTailDrpThresh_CoSRes[1] = 0x00485080; /* PortDropPrfTailDrpThresh_CoSRes[1] */
	tm_index_offset.Drop.PortDropPrfTailDrpThresh_CoSRes[2] = 0x00485100; /* PortDropPrfTailDrpThresh_CoSRes[2] */
	tm_index_offset.Drop.PortDropPrfTailDrpThresh_CoSRes[3] = 0x00485180; /* PortDropPrfTailDrpThresh_CoSRes[3] */
	tm_index_offset.Drop.PortDropPrfTailDrpThresh_CoSRes[4] = 0x00485200; /* PortDropPrfTailDrpThresh_CoSRes[4] */
	tm_index_offset.Drop.PortDropPrfTailDrpThresh_CoSRes[5] = 0x00485280; /* PortDropPrfTailDrpThresh_CoSRes[5] */
	tm_index_offset.Drop.PortDropPrfTailDrpThresh_CoSRes[6] = 0x00485300; /* PortDropPrfTailDrpThresh_CoSRes[6] */
	tm_index_offset.Drop.PortDropPrfTailDrpThresh_CoSRes[7] = 0x00485380; /* PortDropPrfTailDrpThresh_CoSRes[7] */
	tm_index_offset.Drop.PortDropPrfWREDDPRatio = 0x00488180;
	tm_index_offset.Drop.PortDropPrfWREDDPRatio_CoSRes[0] = 0x00484C00; /* PortDropPrfWREDDPRatio_CoSRes[0] */
	tm_index_offset.Drop.PortDropPrfWREDDPRatio_CoSRes[1] = 0x00484C80; /* PortDropPrfWREDDPRatio_CoSRes[1] */
	tm_index_offset.Drop.PortDropPrfWREDDPRatio_CoSRes[2] = 0x00484D00; /* PortDropPrfWREDDPRatio_CoSRes[2] */
	tm_index_offset.Drop.PortDropPrfWREDDPRatio_CoSRes[3] = 0x00484D80; /* PortDropPrfWREDDPRatio_CoSRes[3] */
	tm_index_offset.Drop.PortDropPrfWREDDPRatio_CoSRes[4] = 0x00484E00; /* PortDropPrfWREDDPRatio_CoSRes[4] */
	tm_index_offset.Drop.PortDropPrfWREDDPRatio_CoSRes[5] = 0x00484E80; /* PortDropPrfWREDDPRatio_CoSRes[5] */
	tm_index_offset.Drop.PortDropPrfWREDDPRatio_CoSRes[6] = 0x00484F00; /* PortDropPrfWREDDPRatio_CoSRes[6] */
	tm_index_offset.Drop.PortDropPrfWREDDPRatio_CoSRes[7] = 0x00484F80; /* PortDropPrfWREDDPRatio_CoSRes[7] */
	tm_index_offset.Drop.PortDropPrfWREDMinThresh = 0x00488100;
	tm_index_offset.Drop.PortDropPrfWREDMinThresh_CoSRes[0] = 0x00484800; /* PortDropPrfWREDMinThresh_CoSRes[0] */
	tm_index_offset.Drop.PortDropPrfWREDMinThresh_CoSRes[1] = 0x00484880; /* PortDropPrfWREDMinThresh_CoSRes[1] */
	tm_index_offset.Drop.PortDropPrfWREDMinThresh_CoSRes[2] = 0x00484900; /* PortDropPrfWREDMinThresh_CoSRes[2] */
	tm_index_offset.Drop.PortDropPrfWREDMinThresh_CoSRes[3] = 0x00484980; /* PortDropPrfWREDMinThresh_CoSRes[3] */
	tm_index_offset.Drop.PortDropPrfWREDMinThresh_CoSRes[4] = 0x00484A00; /* PortDropPrfWREDMinThresh_CoSRes[4] */
	tm_index_offset.Drop.PortDropPrfWREDMinThresh_CoSRes[5] = 0x00484A80; /* PortDropPrfWREDMinThresh_CoSRes[5] */
	tm_index_offset.Drop.PortDropPrfWREDMinThresh_CoSRes[6] = 0x00484B00; /* PortDropPrfWREDMinThresh_CoSRes[6] */
	tm_index_offset.Drop.PortDropPrfWREDMinThresh_CoSRes[7] = 0x00484B80; /* PortDropPrfWREDMinThresh_CoSRes[7] */
	tm_index_offset.Drop.PortDropPrfWREDParams = 0x00488000;
	tm_index_offset.Drop.PortDropPrfWREDParams_CoSRes[0] = 0x00484000; /* PortDropPrfWREDParams_CoSRes[0] */
	tm_index_offset.Drop.PortDropPrfWREDParams_CoSRes[1] = 0x00484080; /* PortDropPrfWREDParams_CoSRes[1] */
	tm_index_offset.Drop.PortDropPrfWREDParams_CoSRes[2] = 0x00484100; /* PortDropPrfWREDParams_CoSRes[2] */
	tm_index_offset.Drop.PortDropPrfWREDParams_CoSRes[3] = 0x00484180; /* PortDropPrfWREDParams_CoSRes[3] */
	tm_index_offset.Drop.PortDropPrfWREDParams_CoSRes[4] = 0x00484200; /* PortDropPrfWREDParams_CoSRes[4] */
	tm_index_offset.Drop.PortDropPrfWREDParams_CoSRes[5] = 0x00484280; /* PortDropPrfWREDParams_CoSRes[5] */
	tm_index_offset.Drop.PortDropPrfWREDParams_CoSRes[6] = 0x00484300; /* PortDropPrfWREDParams_CoSRes[6] */
	tm_index_offset.Drop.PortDropPrfWREDParams_CoSRes[7] = 0x00484380; /* PortDropPrfWREDParams_CoSRes[7] */
	tm_index_offset.Drop.PortDropPrfWREDScaleRatio = 0x00488080;
	tm_index_offset.Drop.PortDropPrfWREDScaleRatio_CoSRes[0] = 0x00484400; /* PortDropPrfWREDScaleRatio_CoSRes[0] */
	tm_index_offset.Drop.PortDropPrfWREDScaleRatio_CoSRes[1] = 0x00484480; /* PortDropPrfWREDScaleRatio_CoSRes[1] */
	tm_index_offset.Drop.PortDropPrfWREDScaleRatio_CoSRes[2] = 0x00484500; /* PortDropPrfWREDScaleRatio_CoSRes[2] */
	tm_index_offset.Drop.PortDropPrfWREDScaleRatio_CoSRes[3] = 0x00484580; /* PortDropPrfWREDScaleRatio_CoSRes[3] */
	tm_index_offset.Drop.PortDropPrfWREDScaleRatio_CoSRes[4] = 0x00484600; /* PortDropPrfWREDScaleRatio_CoSRes[4] */
	tm_index_offset.Drop.PortDropPrfWREDScaleRatio_CoSRes[5] = 0x00484600; /* PortDropPrfWREDScaleRatio_CoSRes[5] */
	tm_index_offset.Drop.PortDropPrfWREDScaleRatio_CoSRes[6] = 0x00484700; /* PortDropPrfWREDScaleRatio_CoSRes[6] */
	tm_index_offset.Drop.PortDropPrfWREDScaleRatio_CoSRes[7] = 0x00484780; /* PortDropPrfWREDScaleRatio_CoSRes[7] */
	tm_index_offset.Drop.PortDropProb = 0x0049A080;
	tm_index_offset.Drop.PortDropProbPerCoS_CoS[0] = 0x0049A800; /* CoS[0] */
	tm_index_offset.Drop.PortDropProbPerCoS_CoS[1] = 0x0049A880; /* CoS[1] */
	tm_index_offset.Drop.PortDropProbPerCoS_CoS[2] = 0x0049A900; /* CoS[2] */
	tm_index_offset.Drop.PortDropProbPerCoS_CoS[3] = 0x0049A980; /* CoS[3] */
	tm_index_offset.Drop.PortDropProbPerCoS_CoS[4] = 0x0049AA00; /* CoS[4] */
	tm_index_offset.Drop.PortDropProbPerCoS_CoS[5] = 0x0049AA80; /* CoS[5] */
	tm_index_offset.Drop.PortDropProbPerCoS_CoS[6] = 0x0049AB00; /* CoS[6] */
	tm_index_offset.Drop.PortDropProbPerCoS_CoS[7] = 0x0049AB80; /* CoS[7] */
	tm_index_offset.Drop.PortInstAndAvgQueueLength = 0x0049A000;
	tm_index_offset.Drop.PortInstAndAvgQueueLengthPerCoS.CoS[0] = 0x0049A400; /* CoS[0] */
	tm_index_offset.Drop.PortInstAndAvgQueueLengthPerCoS.CoS[1] = 0x0049A480; /* CoS[1] */
	tm_index_offset.Drop.PortInstAndAvgQueueLengthPerCoS.CoS[2] = 0x0049A500; /* CoS[2] */
	tm_index_offset.Drop.PortInstAndAvgQueueLengthPerCoS.CoS[3] = 0x0049A580; /* CoS[3] */
	tm_index_offset.Drop.PortInstAndAvgQueueLengthPerCoS.CoS[4] = 0x0049A600; /* CoS[4] */
	tm_index_offset.Drop.PortInstAndAvgQueueLengthPerCoS.CoS[5] = 0x0049A680; /* CoS[5] */
	tm_index_offset.Drop.PortInstAndAvgQueueLengthPerCoS.CoS[6] = 0x0049A700; /* CoS[6] */
	tm_index_offset.Drop.PortInstAndAvgQueueLengthPerCoS.CoS[7] = 0x0049A780; /* CoS[7] */
	tm_index_offset.Drop.PortREDCurve_CoS[0] = 0x00486000; /* PortREDCurve_CoS[0] */
	tm_index_offset.Drop.PortREDCurve_CoS[1] = 0x00486400; /* PortREDCurve_CoS[1] */
	tm_index_offset.Drop.PortREDCurve_CoS[2] = 0x00486800; /* PortREDCurve_CoS[2] */
	tm_index_offset.Drop.PortREDCurve_CoS[3] = 0x00486C00; /* PortREDCurve_CoS[3] */
	tm_index_offset.Drop.PortREDCurve_CoS[4] = 0x00487000; /* PortREDCurve_CoS[4] */
	tm_index_offset.Drop.PortREDCurve_CoS[5] = 0x00487400; /* PortREDCurve_CoS[5] */
	tm_index_offset.Drop.PortREDCurve_CoS[6] = 0x00487800; /* PortREDCurve_CoS[6] */
	tm_index_offset.Drop.PortREDCurve_CoS[7] = 0x00487C00; /* PortREDCurve_CoS[7] */
	tm_index_offset.Drop.QueueAvgQueueLength = 0x0049C000;
	tm_index_offset.Drop.QueueCoSConf = 0x00498000;
	tm_index_offset.Drop.QueueDropPrfTailDrpThresh = 0x00494600;
	tm_index_offset.Drop.QueueDropPrfWREDDPRatio = 0x00494580;
	tm_index_offset.Drop.QueueDropPrfWREDMinThresh = 0x00494500;
	tm_index_offset.Drop.QueueDropPrfWREDParams = 0x00494400;
	tm_index_offset.Drop.QueueDropPrfWREDScaleRatio = 0x00494480;
	tm_index_offset.Drop.QueueDropProb = 0x0049D000;
	tm_index_offset.Drop.QueueDropProfPtr = 0x00494000;
	tm_index_offset.Drop.QueueREDCurve.Color[0] = 0x00496000; /* Color[0] */
	tm_index_offset.Drop.QueueREDCurve.Color[1] = 0x00496800; /* Color[1] */
	tm_index_offset.Drop.QueueREDCurve.Color[2] = 0x00497000; /* Color[2] */
	tm_index_offset.Drop.RespLocalDPSel = 0x00480050;
	tm_index_offset.Drop.WREDDropProbMode = 0x00480038;
	tm_index_offset.Drop.WREDMaxProbModePerColor = 0x00480040;


	tm_index_offset.Sched.ALevelShaperBucketNeg = 0x0045AC00;
	tm_index_offset.Sched.ALvltoBlvlAndQueueRangeMap = 0x07690000;
	tm_index_offset.Sched.AlvlDWRRPrioEn = 0x07670000;
	tm_index_offset.Sched.AlvlDef = 0x07780000;
	tm_index_offset.Sched.AlvlEligPrioFunc.Entry[0] = 0x07630000; /* Entry[0] */
	tm_index_offset.Sched.AlvlEligPrioFunc.Entry[1] = 0x07630001; /* Entry[1] */
	tm_index_offset.Sched.AlvlEligPrioFunc.Entry[2] = 0x07630002; /* Entry[2] */
	tm_index_offset.Sched.AlvlEligPrioFunc.Entry[3] = 0x07630003; /* Entry[3] */
	tm_index_offset.Sched.AlvlEligPrioFunc.Entry[4] = 0x07630004; /* Entry[4] */
	tm_index_offset.Sched.AlvlEligPrioFunc.Entry[5] = 0x07630005; /* Entry[5] */
	tm_index_offset.Sched.AlvlEligPrioFunc.Entry[6] = 0x07630006; /* Entry[6] */
	tm_index_offset.Sched.AlvlEligPrioFunc.Entry[7] = 0x07630007; /* Entry[7] */
	tm_index_offset.Sched.AlvlEligPrioFuncPtr = 0x07640000;
	tm_index_offset.Sched.AlvlPerConf = 0x06D90000;
	tm_index_offset.Sched.AlvlPerRateShpPrms = 0x06DA0000;
	tm_index_offset.Sched.AlvlQuantum = 0x07680000;
	tm_index_offset.Sched.AlvlShpBucketLvls = 0x07770000;
	tm_index_offset.Sched.AlvlTokenBucketBurstSize = 0x07660000;
	tm_index_offset.Sched.AlvlTokenBucketTokenEnDiv = 0x07650000;
	tm_index_offset.Sched.BLevelShaperBucketNeg = 0x077D0000;
	tm_index_offset.Sched.BLvltoClvlAndAlvlRangeMap = 0x07620000;
	tm_index_offset.Sched.BlvlDWRRPrioEn = 0x07600000;
	tm_index_offset.Sched.BlvlDef = 0x07760000;
	tm_index_offset.Sched.BlvlEligPrioFunc.Entry[0] = 0x075C0000; /* Entry[0] */
	tm_index_offset.Sched.BlvlEligPrioFunc.Entry[1] = 0x075C0001; /* Entry[1] */
	tm_index_offset.Sched.BlvlEligPrioFunc.Entry[2] = 0x075C0002; /* Entry[2] */
	tm_index_offset.Sched.BlvlEligPrioFunc.Entry[3] = 0x075C0003; /* Entry[3] */
	tm_index_offset.Sched.BlvlEligPrioFunc.Entry[4] = 0x075C0004; /* Entry[4] */
	tm_index_offset.Sched.BlvlEligPrioFunc.Entry[5] = 0x075C0005; /* Entry[5] */
	tm_index_offset.Sched.BlvlEligPrioFunc.Entry[6] = 0x075C0006; /* Entry[6] */
	tm_index_offset.Sched.BlvlEligPrioFunc.Entry[7] = 0x075C0007; /* Entry[7] */

	tm_index_offset.Sched.BlvlEligPrioFuncPtr = 0x075D0000;
	tm_index_offset.Sched.BlvlPerConf = 0x06D70000;
	tm_index_offset.Sched.BlvlPerRateShpPrms = 0x06D80000;
	tm_index_offset.Sched.BlvlQuantum = 0x07610000;
	tm_index_offset.Sched.BlvlShpBucketLvls = 0x07750000;
	tm_index_offset.Sched.BlvlTokenBucketBurstSize = 0x075F0000;
	tm_index_offset.Sched.BlvlTokenBucketTokenEnDiv = 0x075E0000;
	tm_index_offset.Sched.CLevelShaperBucketNeg = 0x077C0000;
	tm_index_offset.Sched.CLvlDef = 0x07740000;
	tm_index_offset.Sched.ClvlDWRRPrioEn = 0x07590000;
	tm_index_offset.Sched.ClvlEligPrioFunc.Entry[0] = 0x07550000; /* Entry[0] */
	tm_index_offset.Sched.ClvlEligPrioFunc.Entry[1] = 0x07550001; /* Entry[1] */
	tm_index_offset.Sched.ClvlEligPrioFunc.Entry[2] = 0x07550002; /* Entry[2] */
	tm_index_offset.Sched.ClvlEligPrioFunc.Entry[3] = 0x07550003; /* Entry[3] */
	tm_index_offset.Sched.ClvlEligPrioFunc.Entry[4] = 0x07550004; /* Entry[4] */
	tm_index_offset.Sched.ClvlEligPrioFunc.Entry[5] = 0x07550005; /* Entry[5] */
	tm_index_offset.Sched.ClvlEligPrioFunc.Entry[6] = 0x07550006; /* Entry[6] */
	tm_index_offset.Sched.ClvlEligPrioFunc.Entry[7] = 0x07550007; /* Entry[7] */
	tm_index_offset.Sched.ClvlEligPrioFuncPtr = 0x07560000;
	tm_index_offset.Sched.ClvlPerConf = 0x06D50000;
	tm_index_offset.Sched.ClvlPerRateShpPrms = 0x06D60000;
	tm_index_offset.Sched.ClvlQuantum = 0x075A0000;
	tm_index_offset.Sched.ClvlShpBucketLvls = 0x07730000;
	tm_index_offset.Sched.ClvlTokenBucketBurstSize = 0x07580000;
	tm_index_offset.Sched.ClvlTokenBucketTokenEnDiv = 0x07570000;
	tm_index_offset.Sched.ClvltoPortAndBlvlRangeMap = 0x075B0000;
	tm_index_offset.Sched.PortDWRRBytesPerBurstsLimit = 0x06D40000;
	tm_index_offset.Sched.PortDWRRPrioEn = 0x07510000;
	tm_index_offset.Sched.PortDefPrioHi = 0x07720000;
	tm_index_offset.Sched.PortDefPrioLo = 0x07710000;
	tm_index_offset.Sched.PortEligPrioFunc.Entry[0] = 0x074D0000; /* Entry[0] */
	tm_index_offset.Sched.PortEligPrioFunc.Entry[1] = 0x074D0001; /* Entry[1] */
	tm_index_offset.Sched.PortEligPrioFunc.Entry[2] = 0x074D0002; /* Entry[2] */
	tm_index_offset.Sched.PortEligPrioFunc.Entry[3] = 0x074D0003; /* Entry[3] */
	tm_index_offset.Sched.PortEligPrioFunc.Entry[4] = 0x074D0004; /* Entry[4] */
	tm_index_offset.Sched.PortEligPrioFunc.Entry[5] = 0x074D0005; /* Entry[5] */
	tm_index_offset.Sched.PortEligPrioFunc.Entry[6] = 0x074D0006; /* Entry[6] */
	tm_index_offset.Sched.PortEligPrioFunc.Entry[7] = 0x074D0007; /* Entry[7] */
	tm_index_offset.Sched.PortEligPrioFuncPtr = 0x074E0000;
	tm_index_offset.Sched.PortExtBPEn = 0x06D30000;
	tm_index_offset.Sched.PortPerConf = 0x06D10000;
	tm_index_offset.Sched.PortPerRateShpPrms = 0x06D20000;
	tm_index_offset.Sched.PortQuantumsPriosHi = 0x07530000;
	tm_index_offset.Sched.PortQuantumsPriosLo = 0x07520000;
	tm_index_offset.Sched.PortRangeMap = 0x07540000;
	tm_index_offset.Sched.PortShaperBucketNeg = 0x077B0000;
	tm_index_offset.Sched.PortShpBucketLvls = 0x07700000;
	tm_index_offset.Sched.PortTokenBucketBurstSize = 0x07500000;
	tm_index_offset.Sched.PortTokenBucketTokenEnDiv = 0x074F0000;
	tm_index_offset.Sched.QueueAMap = 0x076F0000;
	tm_index_offset.Sched.QueueDef = 0x077A0000;
	tm_index_offset.Sched.QueueEligPrioFunc = 0x076A0000;
	tm_index_offset.Sched.QueueEligPrioFuncPtr = 0x076B0000;
	tm_index_offset.Sched.QueuePerConf = 0x06DB0000;
	tm_index_offset.Sched.QueuePerRateShpPrms = 0x06DC0000;
	tm_index_offset.Sched.QueueQuantum = 0x076E0000;
	tm_index_offset.Sched.QueueShaperBucketNeg = 0x077F0000;
	tm_index_offset.Sched.QueueShpBucketLvls = 0x07790000;
	tm_index_offset.Sched.QueueTokenBucketBurstSize = 0x076D0000;
	tm_index_offset.Sched.QueueTokenBucketTokenEnDiv = 0x076C0000;
	tm_index_offset.Sched.ScrubSlotAlloc = 0x06CD0000;
	tm_index_offset.Sched.TreeDWRRPrioEn = 0x06D00000;
	tm_index_offset.Sched.TreeDeqEn = 0x06CF0000;

#endif
}
