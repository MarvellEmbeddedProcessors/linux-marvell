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

#ifndef __mv_pp3_msg_drv_h__
#define __mv_pp3_msg_drv_h__

/* driver request message node */
struct msg_reply_info {
	struct list_head sent_msg;
	unsigned int seq_num;
	int size_of_reply;
	void *output_buf;
	void (*msg_cb)(void *p1);
	void *p1;
};
/* driver request parameters */
struct request_info {
	unsigned int msg_opcode;
	void *in_param;
	int size_of_input;
	void *out_buff;
	int size_of_output;
	int num_of_ints;
	void (*req_cb)(void *);
	void *cb_params;
};
/* driver request callback parameters */
struct drv_msg_cb_params {
	struct completion complete;
	int req_num;
	int ret_code;
	int num_ok;
	int size_of_reply;
};

/* Init driver messenger mechanism for driver request HOST <-> FW messages.
 * Create channel (or channels) according to "share_ch" flag
Inputs:
	num_of_msg - max number of messages in channel
	share_ch   - if true, one channel is created for used by each cpu
		     if false, create channel per cpu
Return:
	0  - success
	-1 - failure
*/
int mv_pp3_drv_messenger_init(int num_of_msg, bool share_ch);

/* Close driver messenger mechanism
 * Delete channel (or channels) according to "share_ch" flag and clear all global configuration
 * parameters
*/
void mv_pp3_drv_messenger_close(void);

/* Send driver message to FW
Inputs:
	req_info   - message info needed by request/reply.
Return:
	>= 0- Message sequence number (message accepted and sent to firmware).
	< 0 - Failure: Queue is full, etc
*/
int mv_pp3_drv_request_send(struct request_info *msg_info);

/* Delete driver request message from the list of requests
Inputs:
	req_num    - request sequence number
Return:
	0  - success
	-1 - failure
*/
int mv_pp3_drv_request_delete(unsigned int req_num);


#endif /* __mv_pp3_msg_drv_h__ */
