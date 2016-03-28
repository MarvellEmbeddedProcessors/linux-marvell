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

#ifndef __mv_fw_shared_h__
#define __mv_fw_shared_h__

/* SW- FW shared parameters definitions */

/* FW utilities 4 (do not change) DRAM memory separate blocks :
_________________________________|_______start address___________|___size____|
SP_MIRRORING_DRAM_BUFFER         |     A                         | 0x80000   |
PACKET_RECORDING_DRAM_BUFFER     |     B                         | 0x80000   |
LOGGER_DRAM_BUFFER:              |     C                         | 0x24000   |
-     Information buffer         |     C                         | 0x20000   |
-     Critical buffer            |     C + 0x20000               |  0x4000   |
KEEP_ALIVE_BUFFER                |     D                         |   0x400   |

All 4 memory blocks are allocated per PPC.
*/
/* Do not change: DRAM memory blocks list */
enum mv_fw_dram_blocks {
	SP_MIRRORING_BUFFER = 0,
	PACKET_RECORDING_BUFFER,
	LOGGER_BUFFER,
	KEEP_ALIVE_BUFFER,
	LAST_DRAM_BUFFER = 4
};

#define NUMBER_OF_PPNs    16

#define DRAM_FOR_FW_SHARED_SRAM_OFFSET   0x10

#define SP_SIZE                            2560		/* 2.5K Scratch Pad size */
#define LOGGER_ENTRY_SIZE                  16
#define KEEP_ALIVE_ENTRY_SIZE              4

#define MAX_SP_MIRROR_BUF_NUMB             16
#define MAX_INF_LOG_ENTRY_NUMB            512
#define MAX_CRITICAL_LOG_ENTRY_NUMB        64

#define SP_MIRRORING_PER_PPN_DRAM_BUFFER_SIZE        (SP_SIZE * MAX_SP_MIRROR_BUF_NUMB)
#define SP_MIRRORING_DRAM_BUFFER_SIZE                (SP_MIRRORING_PER_PPN_DRAM_BUFFER_SIZE * NUMBER_OF_PPNs)

#define PACKET_RECORDING_DRAM_BUFFER_SIZE            (MAX_INF_LOG_ENTRY_NUMB * 1024)
#define KEEP_ALIVE_BUFFER_SIZE                       (NUMBER_OF_PPNs * KEEP_ALIVE_ENTRY_SIZE)

#define INFORMATION_LOGGER_PER_PPN_DRAM_BUFFER_SIZE  (LOGGER_ENTRY_SIZE * MAX_INF_LOG_ENTRY_NUMB)
#define INFORMATION_LOGGER_DRAM_BUFFER_SIZE          (INFORMATION_LOGGER_PER_PPN_DRAM_BUFFER_SIZE * NUMBER_OF_PPNs)

#define CRITICAL_LOGGER_PER_PPN_DRAM_BUFFER_SIZE     (LOGGER_ENTRY_SIZE * MAX_CRITICAL_LOG_ENTRY_NUMB)
#define CRITICAL_LOGGER_DRAM_BUFFER_SIZE             (CRITICAL_LOGGER_PER_PPN_DRAM_BUFFER_SIZE * NUMBER_OF_PPNs)

#define LOGGER_BUFFER_SIZE                        \
			(INFORMATION_LOGGER_DRAM_BUFFER_SIZE + CRITICAL_LOGGER_DRAM_BUFFER_SIZE)

#define LOGGER_CRITICAL_DRAM_OFFSET           (INFORMATION_LOGGER_DRAM_BUFFER_SIZE)


/* CFH related */
#define MV_CFH_MSG_OFFS		(32)	/* bytes */

#define MV_CFH_FW_MSG_HEADER_SIZE	(2)	/* words */
#define MV_CFH_FW_MSG_HEADER_BYTES	(MV_CFH_FW_MSG_HEADER_SIZE * 4)	/* bytes */

/* Host to FW message types */
enum mv_pp3_h2f_msg_type {
	H2F_NO_ACK_REQ = 0,
	H2F_ACK_REPLY_REQ
};

/* FW to Host message type */
enum mv_pp3_f2h_msg_type {
	F2H_PPN_EVENT = 0,
	F2H_ACK_REPLY
};

/* Clients list */
enum mv_pp3_fw_clients {
	MV_DRIVER_CL_ID = 0,
	MV_DPAPI_CL_ID,

	MV_LAST_CL_ID
};

/* CFH message header data structure */
struct mv_pp3_fw_msg_header {
	unsigned int word0;		/* msg size[31:16], number of instanse to/from FW */
	unsigned int word1;		/*  */
};

/* FW message header fields macros. All fields offsets are start bit in current word */
/* word 0 */
#define MV_HOST_MSG_INST_NUM_OFFS	(0)
#define MV_HOST_MSG_INST_NUM_MASK	(0xFFFF)
#define MV_HOST_MSG_INST_NUM_SET(v)	(((v) & MV_HOST_MSG_INST_NUM_MASK) << MV_HOST_MSG_INST_NUM_OFFS)
#define MV_HOST_MSG_INST_NUM_GET(v)	(((v) >> MV_HOST_MSG_INST_NUM_OFFS) & MV_HOST_MSG_INST_NUM_MASK)

#define MV_HOST_MSG_SIZE_OFFS		(16)
#define MV_HOST_MSG_SIZE_MASK		(0xFFFF)
#define MV_HOST_MSG_SIZE_SET(v)		(((v) & MV_HOST_MSG_SIZE_MASK) << MV_HOST_MSG_SIZE_OFFS)
#define MV_HOST_MSG_SIZE_GET(v)		(((v) >> MV_HOST_MSG_SIZE_OFFS) & MV_HOST_MSG_SIZE_MASK)

/* word 1 */
#define MV_HOST_MSG_SEQ_NUM_OFFS	(0)
#define MV_HOST_MSG_SEQ_NUM_MASK	(0xFFF)
#define MV_HOST_MSG_SEQ_NUM_SET(v)	(((v) & MV_HOST_MSG_SEQ_NUM_MASK) << MV_HOST_MSG_SEQ_NUM_OFFS)
#define MV_HOST_MSG_SEQ_NUM_GET(v)	(((v) >> MV_HOST_MSG_SEQ_NUM_OFFS) & MV_HOST_MSG_SEQ_NUM_MASK)

#define MV_HOST_MSG_RC_OFFS		(12)
#define MV_HOST_MSG_RC_MASK		(0xF)
#define MV_HOST_MSG_RC_GET(v)		(((v) >> MV_HOST_MSG_RC_OFFS) & MV_HOST_MSG_RC_MASK)

#define MV_HOST_MSG_OPCODE_OFFS		(16)
#define MV_HOST_MSG_OPCODE_MASK		(0xFFF)
#define MV_HOST_MSG_OPCODE_SET(v)	(((v) & MV_HOST_MSG_OPCODE_MASK) << MV_HOST_MSG_OPCODE_OFFS)
#define MV_HOST_MSG_OPCODE_GET(v)	(((v) >> MV_HOST_MSG_OPCODE_OFFS) & MV_HOST_MSG_OPCODE_MASK)

#define MV_HOST_MSG_EXT_HDR_OFFS	(28)
#define MV_HOST_MSG_EXT_HDR_MASK	(0x3)
#define MV_HOST_MSG_EXT_HDR_SET(v)	(((v) & MV_HOST_MSG_EXT_HDR_MASK) << MV_HOST_MSG_EXT_HDR_OFFS)
#define MV_HOST_MSG_EXT_HDR_GET(v)	(((v) >> MV_HOST_MSG_EXT_HDR_OFFS) & MV_HOST_MSG_EXT_HDR_MASK)

#define MV_HOST_MSG_ACK_OFFS		(30)
#define MV_HOST_MSG_ACK_MASK		(1)
#define MV_HOST_MSG_ACK_SET(v)		(((v) & MV_HOST_MSG_ACK_MASK) << MV_HOST_MSG_ACK_OFFS)
#define MV_HOST_MSG_ACK_GET(v)		(((v) >> MV_HOST_MSG_ACK_OFFS) & MV_HOST_MSG_ACK_MASK)

#endif /* __mv_fw_shared_h__ */
