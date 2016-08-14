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

/* includes */
#include <linux/kernel.h>
#include <linux/io.h>
#include <linux/list.h>

#include "common/mv_sw_if.h"
#include "platform/mv_pp3.h"
#include "msg/mv_pp3_msg.h"
#include "msg/mv_pp3_msg_drv.h"
#include "fw/mv_fw_shared.h"

#ifdef PP3_DEBUG
#define PP3_MSG_DEBUG
#endif

static spinlock_t dev_msg_lock;

/* channell usage mode - CPU shared or channel per CPU */
static bool mv_msg_chan_share;

/* store here all driver messages sent to FW on channel */
static struct list_head sent_msg_list[MV_PP3_MAX_CHAN_NUM];
/* message sequentail number */
static int msg_seq_num;

/* driver messages channel ID for each cpu */
static int drv_chan_id[CONFIG_NR_CPUS];

static void pp3_chan_callback(int chan, void *msg, int size, int seq_num, int flags, u16 msg_opcode,
		int ret_code, int num_ok);

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
int mv_pp3_drv_messenger_init(int num_of_msg, bool share_ch)
{
	int chan_id;
	int cpu;

	/* create lock mechanism */
	spin_lock_init(&dev_msg_lock);

	if (share_ch) {
		cpu = smp_processor_id();
		/* created one channell for all CPUs */
		chan_id = mv_pp3_chan_create(MV_DRIVER_CL_ID, num_of_msg, pp3_chan_callback);
		if (chan_id < 0)
			return -1;
		INIT_LIST_HEAD(&sent_msg_list[chan_id]);
		for_each_possible_cpu(cpu) {
			drv_chan_id[cpu] = chan_id;
			mv_pp3_channel_reg(chan_id, cpu);
		}
		mv_msg_chan_share = true;
	} else {
		/* created channel per CPU */
		for_each_possible_cpu(cpu) {
			chan_id = mv_pp3_chan_create(MV_DRIVER_CL_ID, num_of_msg, pp3_chan_callback);
			if (chan_id < 0)
				return -1;
			drv_chan_id[cpu] = chan_id;
			INIT_LIST_HEAD(&sent_msg_list[chan_id]);
			mv_pp3_channel_reg(chan_id, cpu);
			mv_msg_chan_share = false;
		}
	}

	return 0;
}

/* Close driver messenger mechanism
 * Delete channel (or channels) according to "share_ch" flag and clear all global configuration
 * parameters
*/
void mv_pp3_drv_messenger_close(void)
{
	struct msg_reply_info *msg_info;
	int cpu, chan_id;

	for_each_possible_cpu(cpu) {
		if (drv_chan_id[cpu]) {
			chan_id = drv_chan_id[cpu];
			while (!list_empty(&sent_msg_list[chan_id])) {
				msg_info = list_first_entry(&sent_msg_list[chan_id], struct msg_reply_info,
						sent_msg);
				/* delete message from list */
				list_del(&msg_info->sent_msg);
				kfree(msg_info);
			}
		}
		mv_pp3_chan_delete(drv_chan_id[cpu]);
	}

	msg_seq_num = 0;
}

/* Send driver message to FW
Inputs:
	req_info   - message info needed by request/reply.
Return:
	>= 0- Message sequence number (message accepted and sent to firmware).
	< 0 - Failure: Queue is full, etc
*/
int mv_pp3_drv_request_send(struct request_info *req_info)
{
	struct msg_reply_info *msg_info;
	int ret_val;
	int cpu = smp_processor_id();
	int chan_id;
	unsigned long iflags = 0;
	int cur_msg_num;

	chan_id = drv_chan_id[cpu];

	msg_info = kzalloc(sizeof(struct msg_reply_info), GFP_ATOMIC);
	if (msg_info == NULL)
		return -1;

	MV_RES_LOCK(mv_msg_chan_share, &dev_msg_lock, iflags);
	cur_msg_num = msg_info->seq_num = msg_seq_num++;
	if (msg_seq_num > MV_HOST_MSG_SEQ_NUM_MASK)
		msg_seq_num = 0;

	MV_RES_UNLOCK(mv_msg_chan_share, &dev_msg_lock, iflags);

	if (req_info->req_cb) {
		/* add message to list */
		msg_info->size_of_reply = req_info->size_of_output;
		msg_info->output_buf = req_info->out_buff;
		msg_info->msg_cb = req_info->req_cb;
		msg_info->p1 = req_info->cb_params;
		MV_RES_LOCK(mv_msg_chan_share, &dev_msg_lock, iflags);
		list_add_tail(&msg_info->sent_msg, &sent_msg_list[chan_id]);
		MV_RES_UNLOCK(mv_msg_chan_share, &dev_msg_lock, iflags);
#ifdef PP3_MSG_DEBUG
		pr_info("Add req (%p) for message N%d, opcode %d\n",
			msg_info, msg_info->seq_num, req_info->msg_opcode);
#endif
	}
	MV_RES_LOCK(mv_msg_chan_share, &dev_msg_lock, iflags);
	ret_val = mv_pp3_msg_send(chan_id, req_info->in_param, req_info->size_of_input, MV_PP3_F_CFH_MSG_ACK,
		req_info->msg_opcode, msg_info->seq_num, req_info->num_of_ints);
	MV_RES_UNLOCK(mv_msg_chan_share, &dev_msg_lock, iflags);

	if (!req_info->req_cb)
		kfree(msg_info);

	return cur_msg_num;
}

/* Delete driver request message from the list of requests
Inputs:
	req_num    - request sequence number
Return:
	0  - success
	-1 - failure
*/
int mv_pp3_drv_request_delete(unsigned int req_num)
{
	struct msg_reply_info *msg_info;
	int cpu = smp_processor_id();
	int chan_id;
	unsigned long iflags = 0;

	chan_id = drv_chan_id[cpu];
	/* get message request from the list head */
	MV_RES_LOCK(mv_msg_chan_share, &dev_msg_lock, iflags);
	if (!list_empty(&sent_msg_list[chan_id]))
		msg_info = list_first_entry(&sent_msg_list[chan_id], struct msg_reply_info, sent_msg);
	else
		msg_info = NULL;
	MV_RES_UNLOCK(mv_msg_chan_share, &dev_msg_lock, iflags);
	/* compare message data */
	if ((msg_info == NULL) || (msg_info->seq_num != req_num)) {
		pr_err("Cannot delete request N%d", req_num);
		return -1;
	}
	/* delete message from list */
	list_del(&msg_info->sent_msg);
	kfree(msg_info);

	return 0;
}

static void pp3_chan_callback(int chan, void *msg, int size, int seq_num, int flags, u16 msg_opcode,
		int ret_code, int num_ok)
{
	struct msg_reply_info *msg_info;
	unsigned long iflags = 0;

	/* get message request from the list head */
	MV_RES_LOCK(mv_msg_chan_share, &dev_msg_lock, iflags);
	if (!list_empty(&sent_msg_list[chan]))
		msg_info = list_first_entry(&sent_msg_list[chan], struct msg_reply_info, sent_msg);
	else
		msg_info = NULL;
	MV_RES_UNLOCK(mv_msg_chan_share, &dev_msg_lock, iflags);
	/* verify response to request */
	if (msg_info == NULL)
		return;
	if ((msg_info != NULL) && (msg_info->seq_num != seq_num)) {
		/*pr_err("%s: Receive unexpected reply N%d for request N%d.", __func__, seq_num, msg_info->seq_num);*/
		return;
	}
#ifdef PP3_MSG_DEBUG
	{
	int i; u8 *tmp = msg;
	pr_info("Receive reply (%p) for request N%d (%d), opcode %d, return code %d\n",
		msg_info, seq_num, msg_info->seq_num, msg_opcode, ret_code);
	pr_info("size = %d, msg_info->size_of_reply = %d\naddress=%p, ", size, msg_info->size_of_reply,
		msg_info->output_buf);
	for (i = 0; i < size; i++, tmp++)
		pr_cont("%x ", *tmp);
	}
#endif
	/* copy response buffer */
	if ((msg != NULL) && (msg_info->output_buf)) {
		if (size <= msg_info->size_of_reply)
			memcpy(msg_info->output_buf, msg, size);
		else {
			memcpy(msg_info->output_buf, msg, msg_info->size_of_reply);
			pr_err("Receive reply for request N%d with size(%d) > size_of_reply(%d)\n",
				seq_num, size, msg_info->size_of_reply);
		}
	}

	if (msg_info->msg_cb) {
		struct drv_msg_cb_params *p;
		p = (struct drv_msg_cb_params *)msg_info->p1;
		p->req_num = seq_num;
		p->ret_code = ret_code;
		p->num_ok = num_ok;
		p->size_of_reply = size;
		msg_info->msg_cb((void *)msg_info->p1);
	} else if (ret_code) {
		pr_info("FW no-callback-request opcode=%d N%d/%d FAILED with ret=%d\n",
			msg_opcode, seq_num, msg_info->seq_num, ret_code);
	}

	mv_pp3_drv_request_delete(seq_num);
}
