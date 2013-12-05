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

#ifndef __mvPp2Wol_h__
#define __mvPp2Wol_h__

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#include "pp2/gbe/mvPp2Gbe.h"

/*********************************** RX Policer Registers *******************/

#define MV_PP2_WOL_MODE_REG                 (MV_PP2_REG_BASE + 0x400)

#define MV_PP2_WOL_GO_SLEEP_BIT             0
#define MV_PP2_WOL_GO_SLEEP_MASK            (1 << MV_PP2_WOL_GO_SLEEP_BIT)

#define MV_PP2_WOL_IS_SLEEP_BIT             1
#define MV_PP2_WOL_IS_SLEEP_MASK            (1 << MV_PP2_WOL_IS_SLEEP_BIT)

#define MV_PP2_WOL_SLEEP_PORT_OFFS          4
#define MV_PP2_WOL_SLEEP_PORT_BITS          3
#define MV_PP2_WOL_SLEEP_PORT_MAX           ((1 << MV_PP2_WOL_SLEEP_PORT_BITS) - 1)
#define MV_PP2_WOL_SLEEP_PORT_ALL_MASK      (MV_PP2_WOL_SLEEP_PORT_MAX << MV_PP2_WOL_SLEEP_PORT_OFFS)
#define MV_PP2_WOL_SLEEP_PORT_MASK(p)       (((p) & MV_PP2_WOL_SLEEP_PORT_MAX) << MV_PP2_WOL_SLEEP_PORT_OFFS)
/*---------------------------------------------------------------------------------------------*/

#define MV_PP2_WOL_MAC_HIGH_REG             (MV_PP2_REG_BASE + 0x410)
#define MV_PP2_WOL_MAC_LOW_REG              (MV_PP2_REG_BASE + 0x414)
/*---------------------------------------------------------------------------------------------*/

#define MV_PP2_WOL_ARP_IP_NUM               2

#define MV_PP2_WOL_ARP_IP0_REG              (MV_PP2_REG_BASE + 0x418)
#define MV_PP2_WOL_ARP_IP1_REG              (MV_PP2_REG_BASE + 0x41C)
#define MV_PP2_WOL_ARP_IP_REG(idx)          (MV_PP2_WOL_ARP_IP0_REG + ((idx) << 2))
/*---------------------------------------------------------------------------------------------*/

#define MV_PP2_WOL_PTRN_NUM                 4
#define MV_PP2_WOL_PTRN_BYTES               128
#define MV_PP2_WOL_PTRN_REGS                (MV_PP2_WOL_PTRN_BYTES / 4)

#define MV_PP2_WOL_WAKEUP_EN_REG            (MV_PP2_REG_BASE + 0x420)
#define MV_PP2_WOL_INTR_CAUSE_REG           (MV_PP2_REG_BASE + 0x424)
#define MV_PP2_WOL_INTR_MASK_REG            (MV_PP2_REG_BASE + 0x428)

/* Bits are the same for all three registers above */
#define MV_PP2_WOL_PTRN_IDX_BIT(idx)        (0 + (idx))
#define MV_PP2_WOL_PTRN_IDX_MASK(idx)       (1 << MV_PP2_WOL_PTRN_IDX_BIT(idx))

#define MV_PP2_WOL_MAGIC_PTRN_BIT           4
#define MV_PP2_WOL_MAGIC_PTRN_MASK          (1 << MV_PP2_WOL_MAGIC_PTRN_BIT)

#define MV_PP2_WOL_ARP_IP0_BIT              5
#define MV_PP2_WOL_ARP_IP1_BIT              6
#define MV_PP2_WOL_ARP_IP_MASK(idx)         (1 << (MV_PP2_WOL_ARP_IP0_BIT + (idx)))

#define MV_PP2_WOL_UCAST_BIT                7
#define MV_PP2_WOL_UCAST_MASK               (1 << MV_PP2_WOL_UCAST_BIT)

#define MV_PP2_WOL_MCAST_BIT                8
#define MV_PP2_WOL_MCAST_MASK               (1 << MV_PP2_WOL_MCAST_BIT)
/*---------------------------------------------------------------------------------------------*/

#define MV_PP2_WOL_PTRN_SIZE_REG            (MV_PP2_REG_BASE + 0x430)

#define MV_PP2_WOL_PTRN_SIZE_BITS           8
#define MV_PP2_WOL_PTRN_SIZE_MAX            ((1 << MV_PP2_WOL_PTRN_SIZE_BITS) - 1)
#define MV_PP2_WOL_PTRN_SIZE_MAX_MASK(i)    (MV_PP2_WOL_PTRN_SIZE_MAX << ((i) << MV_PP2_WOL_PTRN_SIZE_BITS))
#define MV_PP2_WOL_PTRN_SIZE_MASK(i, s)     ((s) << ((i) * MV_PP2_WOL_PTRN_SIZE_BITS))
/*---------------------------------------------------------------------------------------------*/

#define MV_PP2_WOL_PTRN_IDX_REG             (MV_PP2_REG_BASE + 0x434)
#define MV_PP2_WOL_PTRN_DATA_REG(i)         (MV_PP2_REG_BASE + 0x500 + ((i) << 2))
#define MV_PP2_WOL_PTRN_MASK_REG(i)         (MV_PP2_REG_BASE + 0x580 + ((i) << 2))

#define MV_PP2_WOL_PTRN_DATA_BYTE_MASK(i)   (0xFF << ((i) * 8))
#define MV_PP2_WOL_PTRN_DATA_BYTE(i, b)     ((b)  << ((i) * 8))
#define MV_PP2_WOL_PTRN_MASK_BIT(i)         (1    << ((i) * 8))
/*---------------------------------------------------------------------------------------------*/

/* WoL APIs */
void      mvPp2WolRegs(void);
void      mvPp2WolStatus(void);
MV_STATUS mvPp2WolSleep(int port);
MV_STATUS mvPp2WolWakeup(void);
int       mvPp2WolIsSleep(int *port);
MV_STATUS mvPp2WolMagicDaSet(MV_U8 *mac_da);
MV_STATUS mvPp2WolArpIpSet(int idx, MV_U32 ip);
MV_STATUS mvPp2WolPtrnSet(int idx, int size, MV_U8 *data, MV_U8 *mask);
MV_STATUS mvPp2WolArpEventSet(int idx, int enable);
MV_STATUS mvPp2WolMcastEventSet(int enable);
MV_STATUS mvPp2WolUcastEventSet(int enable);
MV_STATUS mvPp2WolMagicEventSet(int enable);
MV_STATUS mvPp2WolPtrnEventSet(int idx, int enable);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __mvPp2Wol_h__ */
