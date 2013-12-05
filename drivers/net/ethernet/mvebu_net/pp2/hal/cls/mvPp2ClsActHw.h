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
Marvell Commercial License Option

If you received this File from Marvell and you have entered into a commercial
license agreement (a "Commercial License") with Marvell, the File is licensed
to you under the terms of the applicable Commercial License.

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
********************************************************************************
Marvell BSD License Option

If you received this File from Marvell, you may opt to use, redistribute and/or
modify this File under the following licensing terms.
Redistribution and use in source and binary forms, with or without modification,
are permitted provided that the following conditions are met:

    *   Redistributions of source code must retain the above copyright notice,
	this list of conditions and the following disclaimer.

    *   Redistributions in binary form must reproduce the above copyright
	notice, this list of conditions and the following disclaimer in the
	documentation and/or other materials provided with the distribution.

    *   Neither the name of Marvell nor the names of its contributors may be
	used to endorse or promote products derived from this software without
	specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

*******************************************************************************/

#ifndef __MV_CLS_ACT_HW_H__
#define __MV_CLS_ACT_HW_H__


/*-------------------------------------------------------------------------------*/
/*		Classifier engines Actions Table offsets	    		 */
/*-------------------------------------------------------------------------------*/

/*action_tbl*/
#define ACT_TBL_ID			0
#define ACT_TBL_ID_BITS			6
#define ACT_TBL_ID_MASK			((1 << ACT_TBL_ID_BITS) - 1)

#define ACT_TBL_SEL			6
#define ACT_TBL_SEL_MASK		(1 << ACT_TBL_SEL)

#define ACT_TBL_PRI_DSCP		7
#define ACT_TBL_PRI_DSCP_MASK		(1 << ACT_TBL_PRI_DSCP)

#define ACT_TBL_GEM_ID			8
#define ACT_TBL_GEM_ID_MASK		(1 << ACT_TBL_GEM_ID)

#define ACT_TBL_LOW_Q			9
#define ACT_TBL_LOW_Q_MASK		(1 << ACT_TBL_LOW_Q)

#define ACT_TBL_HIGH_Q			10
#define ACT_TBL_HIGH_Q_MASK		(1 << ACT_TBL_HIGH_Q)

#define ACT_TBL_COLOR			11
#define ACT_TBL_COLOR_MASK		(1 << ACT_TBL_COLOR)

/*actions*/
#define ACT_COLOR			0
#define ACT_COLOR_BITS			3
#define ACT_COLOR_MASK			(((1 << ACT_COLOR_BITS) - 1) << ACT_COLOR)

#define ACT_PRI				3
#define ACT_PRI_BITS			2
#define ACT_PRI_MASK			(((1 << ACT_PRI_BITS) - 1) << ACT_PRI)
#define ACT_PRI_MAX			((1 << ACT_PRI_BITS) - 1)


#define ACT_DSCP			5
#define ACT_DSCP_BITS			2
#define ACT_DSCP_MASK			(((1 << ACT_DSCP_BITS) - 1) << ACT_DSCP)

#define ACT_GEM_ID			7
#define ACT_GEM_ID_BITS			2
#define ACT_GEM_ID_MASK			(((1 << ACT_GEM_ID_BITS) - 1) << ACT_GEM_ID)

#define ACT_LOW_Q			9
#define ACT_LOW_Q_BITS			2
#define ACT_LOW_Q_MASK			(((1 << ACT_LOW_Q_BITS) - 1) << ACT_LOW_Q)


#define ACT_HIGH_Q			11
#define ACT_HIGH_Q_BITS			2
#define ACT_HIGH_Q_MASK			(((1 << ACT_HIGH_Q_BITS) - 1) << ACT_HIGH_Q)

#define ACT_FWD				13
#define ACT_FWD_BITS			3
#define ACT_FWD_MASK			(((1 << ACT_FWD_BITS) - 1) << ACT_FWD)

#define ACT_POLICER_SELECT		16
#define ACT_POLICER_SELECT_BITS		2
#define ACT_POLICER_SELECT_MASK		(((1 << ACT_POLICER_SELECT_BITS) - 1) << ACT_POLICER_SELECT)

#define ACT_FLOW_ID_EN			18
#define ACT_FLOW_ID_EN_MASK		(1 << ACT_FLOW_ID_EN)

/*qos_attr*/
#define ACT_QOS_ATTR_MDF_PRI		0
#define ACT_QOS_ATTR_PRI_BITS		3
#define ACT_QOS_ATTR_MDF_PRI_MASK	(((1 << ACT_QOS_ATTR_PRI_BITS) - 1) << ACT_QOS_ATTR_MDF_PRI)
#define ACT_QOS_ATTR_PRI_MAX		((1 << ACT_QOS_ATTR_PRI_BITS) - 1)

#define ACT_QOS_ATTR_MDF_DSCP		3
#define ACT_QOS_ATTR_DSCP_BITS		6
#define ACT_QOS_ATTR_MDF_DSCP_MASK	(((1 << ACT_QOS_ATTR_DSCP_BITS) - 1) << ACT_QOS_ATTR_MDF_DSCP)
#define ACT_QOS_ATTR_DSCP_MAX		((1 << ACT_QOS_ATTR_DSCP_BITS) - 1)

#define ACT_QOS_ATTR_MDF_GEM_ID		9
#define ACT_QOS_ATTR_GEM_ID_BITS	12
#define ACT_QOS_ATTR_MDF_GEM_ID_MASK	(((1 << ACT_QOS_ATTR_GEM_ID_BITS) - 1) << ACT_QOS_ATTR_MDF_GEM_ID)
#define ACT_QOS_ATTR_GEM_ID_MAX		((1 << ACT_QOS_ATTR_GEM_ID_BITS) - 1)


#define ACT_QOS_ATTR_MDF_LOW_Q		21
#define ACT_QOS_ATTR_MDF_LOW_Q_BITS	3
#define ACT_QOS_ATTR_MDF_LOW_Q_MAX	((1 << ACT_QOS_ATTR_MDF_LOW_Q_BITS) - 1)
#define ACT_QOS_ATTR_MDF_LOW_Q_MASK	(ACT_QOS_ATTR_MDF_LOW_Q_MAX << ACT_QOS_ATTR_MDF_LOW_Q)

#define ACT_QOS_ATTR_MDF_HIGH_Q		24
#define ACT_QOS_ATTR_MDF_HIGH_Q_BITS	5
#define ACT_QOS_ATTR_MDF_HIGH_Q_MAX	((1 << ACT_QOS_ATTR_MDF_HIGH_Q_BITS) - 1)
#define ACT_QOS_ATTR_MDF_HIGH_Q_MASK	(ACT_QOS_ATTR_MDF_HIGH_Q_MAX << ACT_QOS_ATTR_MDF_HIGH_Q)

#define ACT_QOS_ATTR_Q_MAX		((1 << (ACT_QOS_ATTR_MDF_HIGH_Q_BITS + ACT_QOS_ATTR_MDF_LOW_Q_BITS)) - 1)
/*hwf_attr*/

#define	ACT_HWF_ATTR_DPTR		1
#define	ACT_HWF_ATTR_DPTR_BITS		15
#define	ACT_HWF_ATTR_DPTR_MASK		(((1 << ACT_HWF_ATTR_DPTR_BITS) - 1) << ACT_HWF_ATTR_DPTR)
#define	ACT_HWF_ATTR_DPTR_MAX		((1 << ACT_HWF_ATTR_DPTR_BITS) - 1)

#define	ACT_HWF_ATTR_IPTR		16
#define	ACT_HWF_ATTR_IPTR_BITS		8
#define	ACT_HWF_ATTR_IPTR_MASK		(((1 << ACT_HWF_ATTR_IPTR_BITS) - 1) << ACT_HWF_ATTR_IPTR)
#define	ACT_HWF_ATTR_IPTR_MAX		((1 << ACT_HWF_ATTR_IPTR_BITS) - 1)

#define	ACT_HWF_ATTR_CHKSM_EN		24
#define	ACT_HWF_ATTR_CHKSM_EN_MASK	(1 << ACT_HWF_ATTR_CHKSM_EN)

/*
  PPv2.1 (feature MAS 3.7) new field in action table (c2, c3)
 */
#define ACT_HWF_ATTR_MTU_INX		25
#define ACT_HWF_ATTR_MTU_INX_BITS	4
#define ACT_HWF_ATTR_MTU_INX_MAX	((1 << ACT_HWF_ATTR_MTU_INX_BITS) - 1)
#define	ACT_HWF_ATTR_MTU_INX_MASK	((ACT_HWF_ATTR_MTU_INX_MAX) << ACT_HWF_ATTR_MTU_INX)


/*MV_U32 dup_attr*/
#define ACT_DUP_FID			0
#define ACT_DUP_FID_BITS		8
#define ACT_DUP_FID_MASK		(((1 << ACT_DUP_FID_BITS) - 1) << ACT_DUP_FID)
#define ACT_DUP_FID_MAX			((1 << ACT_DUP_FID_BITS) - 1)


#define ACT_DUP_COUNT			8
#define ACT_DUP_COUNT_BITS		4
#define ACT_DUP_COUNT_MASK		(((1 << ACT_DUP_COUNT_BITS) - 1) << ACT_DUP_COUNT)
#define ACT_DUP_COUNT_MAX		14

#ifdef CONFIG_MV_ETH_PP2_1
#define ACT_DUP_POLICER_ID		24
#define ACT_DUP_POLICER_ID_BITS		5
#else
#define ACT_DUP_POLICER_ID		28
#define ACT_DUP_POLICER_ID_BITS		4
#endif

#define ACT_DUP_POLICER_MASK		(((1 << ACT_DUP_POLICER_ID_BITS) - 1) << ACT_DUP_POLICER_ID)
#define ACT_DUP_POLICER_MAX		((1 << ACT_DUP_POLICER_ID_BITS) - 1)

/*only in ppv2.1*/
#define ACT_DUP_POLICER_BANK_BIT	29
#define ACT_DUP_POLICER_BANK_MASK	(1 << ACT_DUP_POLICER_BANK_BIT)


/*-------------------------------------------------------------------------------*/
/*		Classifier engines Actions Table offsets	    		 */
/*-------------------------------------------------------------------------------*/

typedef enum {
	COLOR_NO_UPDATE = 0,
	COLOR_NO_UPDATE_AND_LOCK = 1,
	COLOR_GREEN = 2,
	COLOR_GREEN_AND_LOCK = 3,
	COLOR_YELLOW = 4,
	COLOR_YELLOW_AND_LOCK = 5,
	COLOR_RED = 6,
	COLOR_RED_AND_LOCK = 7
} MV_PP2_CLS_COLOR_CMD;

typedef enum MV_PP2_CLS_HWF_CMD {
	HWF_NO_UPDATE = 0,
	HWF_NO_UPDATE_AND_LOCK,
	SWF,
	SWF_AND_LOCK,
	HWF,
	HWF_AND_LOCK,
	HWF_AND_LOW_LATENCY,
	HWF_AND_LOW_LATENCY_AND_LOCK
} MV_PP2_CLS_HWF_CMD;

typedef enum {
	NO_UPDATE = 0,
	NO_UPDATE_AND_LOCK,
	UPDATE,
	UPDATE_AND_LOCK
} MV_PP2_CLS_CMD;

typedef enum {
	GREEN = 0,
	YELLOW,
	RED
} MV_PP2_CLS_COLOR;

#endif /*__MV_CLS_ACT_HW_H__*/
