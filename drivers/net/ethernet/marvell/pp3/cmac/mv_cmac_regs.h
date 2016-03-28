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
#ifndef	__mv_cmac_regs_h__
#define	__mv_cmac_regs_h__


/* Cmac Status */
#define MV_CMAC_CMAC_STATUS_REG								(0x002fc000)
#define MV_CMAC_CMAC_STATUS_CMAC_IDLE_OFFS		0
#define MV_CMAC_CMAC_STATUS_CMAC_IDLE_MASK    \
		(0x00000001 << MV_CMAC_CMAC_STATUS_CMAC_IDLE_OFFS)


/* Cmac Il Next Hop C1 Field Change */
#define MV_CMAC_CMAC_IL_NEXT_HOP_C1_FIELD_CHANGE_REG					(0x002fc040)
#define MV_CMAC_CMAC_IL_NEXT_HOP_C1_FIELD_CHANGE_IND_FIFO_COPY_NEXT_HOP_C1_FIELD_OFFS		0
#define MV_CMAC_CMAC_IL_NEXT_HOP_C1_FIELD_CHANGE_IND_FIFO_COPY_NEXT_HOP_C1_FIELD_MASK    \
		(0x00000001 << MV_CMAC_CMAC_IL_NEXT_HOP_C1_FIELD_CHANGE_IND_FIFO_COPY_NEXT_HOP_C1_FIELD_OFFS)


/* Cmac La Next Hop C1 Field Change */
#define MV_CMAC_CMAC_LA_NEXT_HOP_C1_FIELD_CHANGE_REG					(0x002fc044)
#define MV_CMAC_CMAC_LA_NEXT_HOP_C1_FIELD_CHANGE_LAD_FIFO_COPY_NEXT_HOP_C1_FIELD_OFFS		0
#define MV_CMAC_CMAC_LA_NEXT_HOP_C1_FIELD_CHANGE_LAD_FIFO_COPY_NEXT_HOP_C1_FIELD_MASK    \
		(0x00000001 << MV_CMAC_CMAC_LA_NEXT_HOP_C1_FIELD_CHANGE_LAD_FIFO_COPY_NEXT_HOP_C1_FIELD_OFFS)


/* Cmac Il Enq Checksum Offset */
#define MV_CMAC_CMAC_IL_ENQ_CHECKSUM_OFFSET_REG					(0x002fc048)
#define MV_CMAC_CMAC_IL_ENQ_CHECKSUM_OFFSET_CHECKSUM_OFFSET_OFFS		0
#define MV_CMAC_CMAC_IL_ENQ_CHECKSUM_OFFSET_CHECKSUM_OFFSET_MASK    \
		(0x0000007f << MV_CMAC_CMAC_IL_ENQ_CHECKSUM_OFFSET_CHECKSUM_OFFSET_OFFS)


/* Cmac Il Axi Config */
#define MV_CMAC_CMAC_IL_AXI_CONFIG_REG					(0x002fc080)
#define MV_CMAC_CMAC_IL_AXI_CONFIG_AXI4_IL_DEQ_PORT_NUMBER_OFFS		0
#define MV_CMAC_CMAC_IL_AXI_CONFIG_AXI4_IL_DEQ_PORT_NUMBER_MASK    \
		(0x00000fff << MV_CMAC_CMAC_IL_AXI_CONFIG_AXI4_IL_DEQ_PORT_NUMBER_OFFS)


/* Cmac La Axi Config */
#define MV_CMAC_CMAC_LA_AXI_CONFIG_REG					(0x002fc084)
#define MV_CMAC_CMAC_LA_AXI_CONFIG_AXI4_LA_DEQ_PORT_NUMBER_OFFS		0
#define MV_CMAC_CMAC_LA_AXI_CONFIG_AXI4_LA_DEQ_PORT_NUMBER_MASK    \
		(0x00000fff << MV_CMAC_CMAC_LA_AXI_CONFIG_AXI4_LA_DEQ_PORT_NUMBER_OFFS)


/* Cmac Debug Il Fifo Fill Level */
#define MV_CMAC_CMAC_DEBUG_IL_FIFO_FILL_LEVEL_REG					(0x002fc0c0)
#define MV_CMAC_CMAC_DEBUG_IL_FIFO_FILL_LEVEL_IND_FIFO_OFFS		0
#define MV_CMAC_CMAC_DEBUG_IL_FIFO_FILL_LEVEL_IND_FIFO_MASK    \
		(0x000000ff << MV_CMAC_CMAC_DEBUG_IL_FIFO_FILL_LEVEL_IND_FIFO_OFFS)

#define MV_CMAC_CMAC_DEBUG_IL_FIFO_FILL_LEVEL_INE_FIFO_OFFS		8
#define MV_CMAC_CMAC_DEBUG_IL_FIFO_FILL_LEVEL_INE_FIFO_MASK    \
		(0x000000ff << MV_CMAC_CMAC_DEBUG_IL_FIFO_FILL_LEVEL_INE_FIFO_OFFS)


/* Cmac Debug La Fifo Fill Level */
#define MV_CMAC_CMAC_DEBUG_LA_FIFO_FILL_LEVEL_REG				(0x002fc0c4)
#define MV_CMAC_CMAC_DEBUG_LA_FIFO_FILL_LEVEL_LAD_FIFO_OFFS		0
#define MV_CMAC_CMAC_DEBUG_LA_FIFO_FILL_LEVEL_LAD_FIFO_MASK    \
		(0x000000ff << MV_CMAC_CMAC_DEBUG_LA_FIFO_FILL_LEVEL_LAD_FIFO_OFFS)

#define MV_CMAC_CMAC_DEBUG_LA_FIFO_FILL_LEVEL_LAE_FIFO_OFFS		8
#define MV_CMAC_CMAC_DEBUG_LA_FIFO_FILL_LEVEL_LAE_FIFO_MASK    \
		(0x000000ff << MV_CMAC_CMAC_DEBUG_LA_FIFO_FILL_LEVEL_LAE_FIFO_OFFS)


/* Cmac Debug Il Eip Packet Count */
#define MV_CMAC_CMAC_DEBUG_IL_EIP_PCKT_CNT_REG					(0x002fc0c8)
#define MV_CMAC_CMAC_DEBUG_IL_EIP_PCKT_CNT_EIP_IL_PACKET_COUNT_OFFS		0
#define MV_CMAC_CMAC_DEBUG_IL_EIP_PCKT_CNT_EIP_IL_PACKET_COUNT_MASK    \
		(0x0000ffff << MV_CMAC_CMAC_DEBUG_IL_EIP_PCKT_CNT_EIP_IL_PACKET_COUNT_OFFS)


/* Cmac Debug La Eip Packet Count */
#define MV_CMAC_CMAC_DEBUG_LA_EIP_PCKT_CNT_REG					(0x002fc0cc)
#define MV_CMAC_CMAC_DEBUG_LA_EIP_PCKT_CNT_EIP_LA_PACKET_COUNT_OFFS		0
#define MV_CMAC_CMAC_DEBUG_LA_EIP_PCKT_CNT_EIP_LA_PACKET_COUNT_MASK    \
		(0x0000ffff << MV_CMAC_CMAC_DEBUG_LA_EIP_PCKT_CNT_EIP_LA_PACKET_COUNT_OFFS)


/* Cmac Spare  */
#define MV_CMAC_CMAC_SPARE_REG(n)						(0x002fc0e0 + n*4)
#define MV_CMAC_CMAC_SPARE_CMAC_SPR_OFFS		0

#endif /* __mv_cmac_regs_h__ */
