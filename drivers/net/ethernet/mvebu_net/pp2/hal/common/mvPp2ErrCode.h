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

#ifndef __MV_PP2_ERR_CODE_H__
#define __MV_PP2_ERR_CODE_H__

#define  MV_ERR_CODE_BASE						0x80000000
#define  MV_PP2_ERR_CODE_BASE					(MV_ERR_CODE_BASE | 0x00001000)


#define  MV_PP2_PRS						(MV_PP2_ERR_CODE_BASE | 0x00000100)
#define  MV_PP2_CLS						(MV_PP2_ERR_CODE_BASE | 0x00000200)
#define  MV_PP2_CLS2						(MV_PP2_ERR_CODE_BASE | 0x00000400)
#define  MV_PP2_CLS3						(MV_PP2_ERR_CODE_BASE | 0x00000800)
#define  MV_PP2_CLS4						(MV_PP2_ERR_CODE_BASE | 0x00000800)


/*****************************************************************************



			    E R R O R   C O D E S


*****************************************************************************/
/* #define MV_OK 0  define in mvTypes*/
#define EQUALS 0
#define NOT_EQUALS 1

/* PRS error codes */
#define  MV_PRS_ERR						(MV_PP2_PRS | 0x00)
#define  MV_PRS_OUT_OF_RAGE					(MV_PP2_PRS | 0x01)
#define  MV_PRS_NULL_POINTER					(MV_PP2_PRS | 0x02)

/* CLS error codes */
#define  MV_CLS_ERR						(MV_PP2_CLS | 0x00)
#define  MV_CLS_OUT_OF_RAGE					(MV_PP2_CLS | 0x01)

/* CLS2 error codes */
#define  MV_CLS2_ERR						(MV_PP2_CLS2 | 0x00)
#define  MV_CLS2_OUT_OF_RAGE					(MV_PP2_CLS2 | 0x01)
#define  MV_CLS2_NULL_POINTER					(MV_PP2_CLS2 | 0x02)
#define  MV_CLS2_RETRIES_EXCEEDED				(MV_PP2_CLS2 | 0x03)

/* CLS3 error codes */
#define  MV_CLS3_ERR						(MV_PP2_CLS3 | 0x00)
#define  MV_CLS3_OUT_OF_RAGE					(MV_PP2_CLS3 | 0x01)
#define  MV_CLS3_NULL_POINTER					(MV_PP2_CLS3 | 0x02)
#define  MV_CLS3_RETRIES_EXCEEDED				(MV_PP2_CLS3 | 0x03)
#define  MV_CLS3_SW_INTERNAL					(MV_PP2_CLS3 | 0x04)

/* CLS4 error codes */
#define  MV_CLS4_ERR						(MV_PP2_CLS4 | 0x00)
#define  MV_CLS4_OUT_OF_RAGE					(MV_PP2_CLS4 | 0x01)
#define  MV_CLS4_NULL_POINTER					(MV_PP2_CLS4 | 0x02)
#define  MV_CLS4_RETRIES_EXCEEDED				(MV_PP2_CLS4 | 0x03)

#endif /* __MV_PP2_ERR_CODE_H__ */
