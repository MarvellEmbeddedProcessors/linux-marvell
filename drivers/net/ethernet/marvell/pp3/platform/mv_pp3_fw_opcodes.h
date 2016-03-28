/*
* ***************************************************************************
* Copyright (C) 2015 Marvell International Ltd.
* ***************************************************************************
* This program is free software: you can redistribute it and/or modify it
* under the terms of the GNU General Public License as published by the Free
* Software Foundation, either version 2 of the License, or any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program.  If not, see <http://www.gnu.org/licenses/>.
* ***************************************************************************
*/

#ifndef __mv_pp3_fw_opcodes_h__
#define __mv_pp3_fw_opcodes_h__

/* Message opcodes for Software <-> Firmware communication */
enum mv_pp3_fw_msg_opcode {

	MV_IDLE_MSG = 0,
	MV_FW_MSG_CHAN_SET,
	MV_FW_VERSION_GET,
	MV_FW_MEM_REQ_GET,
	MV_FW_MEM_REQ_SET,
	MV_FW_EMAC_VPORT_SET,
	MV_FW_EMAC_VPORT_GET,
	MV_FW_CPU_VPORT_SET,
	MV_FW_CPU_VPORT_GET,
	MV_FW_INTERNAL_CPU_PORT_SET,
	MV_FW_INTERNAL_CPU_PORT_GET,
	MV_FW_BM_POOL_SET,
	MV_FW_BM_POOL_GET,
	MV_FW_VQ_MAP_SET,
	MV_FW_VQ_MAP_GET,
	MV_FW_VQ_POLICER_SET,
	MV_FW_VQ_POLICER_GET,
	MV_FW_COS_TO_VQ_SET,
	MV_FW_COS_TO_VQ_GET,
	MV_FW_VPORT_STATE_SET,
	MV_FW_VPORT_MAC_SET,
	MV_FW_VPORT_MAC_LIST_SET,
	MV_FW_VPORT_MAC_LIST_GET,
	MV_FW_VPORT_L2_OPTION_SET,
	MV_FW_VPORT_MTU_SET,
	MV_FW_HWQ_STATS_GET,
	MV_FW_SWQ_STATS_GET,
	MV_FW_VPORT_STATS_GET,
	MV_FW_BM_POOL_STATS_GET,
	MV_FW_MSG_CHAN_STATS_GET,
	MV_FW_RESET_STATISTICS,
	MV_FW_LINK_CHANGE_NOTE,
	MV_FW_INTERNAL_CPU_PORT_RX_PKT_MODE_SET,
	MV_FW_VPORT_DEF_DEST_SET,
	MV_FW_CPU_VPORT_MAP,

	MV_FW_EXT_MSG_OPCODE_BASE = 0x80,
};

#endif /* __mv_pp3_fw_opcodes_h__ */
