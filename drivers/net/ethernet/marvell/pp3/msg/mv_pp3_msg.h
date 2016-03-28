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

#ifndef __mv_pp3_msg_h__
#define __mv_pp3_msg_h__

/* max channel number */
#define MV_PP3_MAX_CHAN_NUM 16

/* Message flags definition */
#define MV_PP3_F_CFH_MSG_ACK		(0x1)
#define MV_PP3_F_CFH_EXT_HDR		(0x2)

/* Clients name */
extern unsigned char *mv_pp3_sys_clients[];

/* Receive message callback prototype
Inputs:
	chan - unique channel id as returned by mv_pp3_chan_add() function
	msg  - pointer to received message
	size - size of message
	seq_num - message sequence number
	flags - message flags:
		Bit 0 - Acknowledge or Reply is received.
		Bit 1 - Buffer extension header existence
	msg_opcode - unique message type identifier.
	ret_code - status returned by the Firmware for the whole request.
	num_ok - number of messages successfully processed by FW.
*/
typedef void (*mv_pp3_chan_rcv_func)(int chan, void *msg, int size, int seq_num, int flags, u16 msg_opcode,
		int ret_code, int num_ok);

/* Create communication channel (bi-directional)
Inputs:
	client_id - channel client ID
	size - number of CFHs with maximum CFH size (128 byte)
	rcv_cb - callback function to be called when message from Firmware is received on this channel.
Return:
	 0 - success
	-1 - fail
*/
int mv_pp3_chan_create(unsigned char client_id, int size, mv_pp3_chan_rcv_func rcv_cb);

/* Prepare message CFH and trigger it sending to firmware.
Inputs:
	chan - unique channel id
	msg  - pointer to message to send
	size - size of message (in bytes)
	flags - message flags
	msg_opcode - one from known opcodes (see enum mv_pp3_fw_nic_msg_opcode)
	msg_seq_num - message sequence number managed by sender
	num  - number of instances for bulk requests support
Return:
	0  - Message accepted and/or sent to firmware.
	-1 - Failure: Queue is full, etc
*/
int mv_pp3_msg_send(int chan, void *msg, int size, int flags, u16 msg_opcode, int msg_seq_num, int num);

/* Registerate RX channel event to specified cpu
Inputs:
	chan - unique channel id
	cpu - cpu number
Return:
	0  - success.
	-1 - failure
*/
int mv_pp3_channel_reg(int chan, int cpu_num);

/* Close communication channel
Inputs:
	chan - unique channel id
eturn:
	 0 - success
	-1 - fail
*/
int mv_pp3_chan_delete(int chan);

/* Enable / disable print out of sent / received messages */
void mv_pp3_debug_message_print_en(bool rx_en, bool tx_en);

#endif /* __mv_pp3_msg_h__ */
