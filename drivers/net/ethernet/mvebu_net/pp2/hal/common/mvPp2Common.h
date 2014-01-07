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
#ifndef __MV_PP2_COMMON_H__
#define __MV_PP2_COMMON_H__

#include "mvTypes.h"
#include "mvCommon.h"
#include "mvOs.h"

#ifdef CONFIG_ARCH_MVEBU
#include "mvNetConfig.h"
#else
#include "mvSysEthConfig.h"
#endif

/*--------------------------------------------------------------------*/
/*			PP2 COMMON MACROS			      */
/*--------------------------------------------------------------------*/

#define DECIMAL_RANGE_VALIDATE(_VALUE_ , _MIN_, _MAX_) {\
	if (((_VALUE_) > (_MAX_)) || ((_VALUE_) < (_MIN_))) {\
		mvOsPrintf("%s: value %d (0x%x) is out of range [%d , %d].\n",\
				__func__, (_VALUE_), (_VALUE_), (_MIN_), (_MAX_));\
		return MV_ERROR;\
	} \
}

#define RANGE_VALIDATE(_VALUE_ , _MIN_, _MAX_) {\
	if (((_VALUE_) > (_MAX_)) || ((_VALUE_) < (_MIN_))) {\
		mvOsPrintf("%s: value 0x%X (%d) is out of range [0x%X , 0x%X].\n",\
				__func__, (_VALUE_), (_VALUE_), (_MIN_), (_MAX_));\
		return MV_ERROR;\
	} \
}

#define BIT_RANGE_VALIDATE(_VALUE_)			RANGE_VALIDATE(_VALUE_ , 0, 1)

#define POS_RANGE_VALIDATE(_VALUE_, _MAX_)		RANGE_VALIDATE(_VALUE_ , 0, _MAX_)

#define PTR_VALIDATE(_ptr_) {\
	if (_ptr_ == NULL) {\
		mvOsPrintf("%s: null pointer.\n", __func__);\
		return MV_ERROR;\
	} \
}

#define WARN_OOM(cond) if (cond) { mvOsPrintf("%s: out of memory\n", __func__); return NULL; }


/*--------------------------------------------------------------------*/
/*			PP2 COMMON FUNCTIONS			      */
/*--------------------------------------------------------------------*/


int mvPp2RdReg(unsigned int offset);

int mvPp2WrReg(unsigned int offset, unsigned int  val);

void mvPp2PrintReg(unsigned int  reg_addr, char *reg_name);
void mvPp2PrintReg2(MV_U32 reg_addr, char *reg_name, MV_U32 index);

int mvPp2SPrintReg(char *buf, unsigned int  reg_addr, char *reg_name);

void mvPp2RegPrintNonZero(MV_U32 reg_addr, char *reg_name);
void mvPp2RegPrintNonZero2(MV_U32 reg_addr, char *reg_name, MV_U32 index);

/*--------------------------------------------------------------------*/
/*			PP2 COMMON DEFINETIONS			      */
/*--------------------------------------------------------------------*/
#define NOT_IN_USE					(-1)
#define IN_USE						(1)
#define DWORD_BITS_LEN					32
#define DWORD_BYTES_LEN                                 4
#define RETRIES_EXCEEDED				15000
#define ONE_BIT_MAX					1
#define UNI_MAX						7
#define ETH_PORTS_NUM					7

/*--------------------------------------------------------------------*/
/*			PNC COMMON DEFINETIONS			      */
/*--------------------------------------------------------------------*/

/*
 HW_BYTE_OFFS
 return HW byte offset in 4 bytes register
 _offs_: native offset (LE)
 LE example: HW_BYTE_OFFS(1) = 1
 BE example: HW_BYTE_OFFS(1) = 2
*/

#if defined(MV_CPU_LE)
	#define HW_BYTE_OFFS(_offs_)		(_offs_)
#else
	#define HW_BYTE_OFFS(_offs_)		((3 - ((_offs_) % 4)) + (((_offs_) / 4) * 4))
#endif


#define TCAM_DATA_BYTE_OFFS_LE(_offs_)		(((_offs_) - ((_offs_) % 2)) * 2 + ((_offs_) % 2))
#define TCAM_DATA_MASK_OFFS_LE(_offs_)		(((_offs_) * 2) - ((_offs_) % 2)  + 2)

/*
 TCAM_DATA_BYTE/MASK
 tcam data devide into 4 bytes registers
 each register include 2 bytes of data and 2 bytes of mask
 the next macros calc data/mask offset in 4 bytes register
 _offs_: native offset (LE) in data bytes array
 relevant only for TCAM data bytes
 used by PRS and CLS2
*/
#define TCAM_DATA_BYTE(_offs_)			(HW_BYTE_OFFS(TCAM_DATA_BYTE_OFFS_LE(_offs_)))
#define TCAM_DATA_MASK(_offs_)			(HW_BYTE_OFFS(TCAM_DATA_MASK_OFFS_LE(_offs_)))




#endif /* __MV_PP2_ERR_CODE_H__ */
