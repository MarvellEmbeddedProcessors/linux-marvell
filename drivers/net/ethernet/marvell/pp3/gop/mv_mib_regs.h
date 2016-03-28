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

#ifndef __mv_mib_regs_h__
#define __mv_mib_regs_h__

#define MV_PP3_MIB_COUNTERS_REG_BASE		0x02000000

#define MV_PP3_MIB_PORT_OFFSET(port)		((port) * 0x400)
#define MV_PP3_MIB_COUNTERS_BASE(port)		(MV_PP3_MIB_COUNTERS_REG_BASE + MV_PP3_MIB_PORT_OFFSET(port))

/* GMAC_MIB Counters register definitions */
#define MV_PP3_MIB_GOOD_OCTETS_RECEIVED_LOW	0x0
#define MV_PP3_MIB_GOOD_OCTETS_RECEIVED_HIGH	0x4
#define MV_PP3_MIB_BAD_OCTETS_RECEIVED		0x8
#define MV_PP3_MIB_CRC_ERRORS_SENT		0xc
#define MV_PP3_MIB_UNICAST_FRAMES_RECEIVED	0x10
/* Reserved					0x14 */
#define MV_PP3_MIB_BROADCAST_FRAMES_RECEIVED	0x18
#define MV_PP3_MIB_MULTICAST_FRAMES_RECEIVED	0x1c
#define MV_PP3_MIB_FRAMES_64_OCTETS		0x20
#define MV_PP3_MIB_FRAMES_65_TO_127_OCTETS	0x24
#define MV_PP3_MIB_FRAMES_128_TO_255_OCTETS	0x28
#define MV_PP3_MIB_FRAMES_256_TO_511_OCTETS	0x2c
#define MV_PP3_MIB_FRAMES_512_TO_1023_OCTETS	0x30
#define MV_PP3_MIB_FRAMES_1024_TO_MAX_OCTETS	0x34
#define MV_PP3_MIB_GOOD_OCTETS_SENT_LOW		0x38
#define MV_PP3_MIB_GOOD_OCTETS_SENT_HIGH	0x3c
#define MV_PP3_MIB_UNICAST_FRAMES_SENT		0x40
/* Reserved					0x44 */
#define MV_PP3_MIB_MULTICAST_FRAMES_SENT	0x48
#define MV_PP3_MIB_BROADCAST_FRAMES_SENT	0x4c
/* Reserved					0x50 */
#define MV_PP3_MIB_FC_SENT			0x54
#define MV_PP3_MIB_FC_RECEIVED			0x58
#define MV_PP3_MIB_RX_FIFO_OVERRUN		0x5c
#define MV_PP3_MIB_UNDERSIZE_RECEIVED		0x60
#define MV_PP3_MIB_FRAGMENTS_RECEIVED		0x64
#define MV_PP3_MIB_OVERSIZE_RECEIVED		0x68
#define MV_PP3_MIB_JABBER_RECEIVED		0x6c
#define MV_PP3_MIB_MAC_RECEIVE_ERROR		0x70
#define MV_PP3_MIB_BAD_CRC_EVENT		0x74
#define MV_PP3_MIB_COLLISION			0x78
#define MV_PP3_MIB_LATE_COLLISION		0x7c

#endif /* mv_mib_regs_h */
