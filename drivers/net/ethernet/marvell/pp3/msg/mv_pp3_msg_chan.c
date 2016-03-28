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
#include <linux/slab.h>
#include <linux/spinlock.h>

#include "common/mv_sw_if.h"
#include "common/mv_stack.h"
#include "platform/mv_pp3.h"
#include "platform/mv_pp3_config.h"
#include "platform/mv_pp3_fw_opcodes.h"
#include "hmac/mv_hmac.h"
#include "hmac/mv_hmac_bm.h"
#include "fw/mv_pp3_fw_msg.h"
#include "mv_pp3_msg.h"
#include "mv_pp3_msg_chan.h"

/* platform pointer for dma operation */
static struct mv_pp3 *pp3_priv;

/* channel info */
static struct mv_pp3_channel mv_pp3_chan_ctrl[MV_PP3_MAX_CHAN_NUM];
/* number of active channels */
static int mv_pp3_active_chan_num;

/* global one lock for all channels */
static spinlock_t msngr_channel_lock;
static bool msngr_free;

/* debug print configuration variables */
static bool mv_pp3_rx_msg_print_en;
static bool mv_pp3_tx_msg_print_en;

static u32 __percpu *mv_pp3_ch_bmp;

/* forward function declaration */
static void mv_pp3_chan_rx_event(struct mv_pp3_channel *chan);
static void mv_pp3_message_print(char *str, struct host_msg *cfh_ptr, int size, int num, int opcode, int chan);

/* Return number of active channels */
int mv_pp3_chan_num_get(void)
{
	return mv_pp3_active_chan_num;
}

/* Return pointer to channel sturcture or NULL if ch_num is invalid */
struct mv_pp3_channel *mv_pp3_chan_get(int ch_num)
{
	if (ch_num >= mv_pp3_active_chan_num)
		return NULL;

	return &mv_pp3_chan_ctrl[ch_num];
}


/* Init messenger facility - call ones */
int mv_pp3_messenger_init(struct mv_pp3 *priv)
{
	int id;

	pp3_priv = priv;

	mv_pp3_ch_bmp = alloc_percpu(sizeof(u32));

	/* create lock mechanism */
	spin_lock_init(&msngr_channel_lock);
	msngr_free = true;

	/* create default channel for messenger control nessages */
	id = mv_pp3_chan_create(MV_DRIVER_CL_ID, 100, NULL);
	if (id < 0) {
		pr_err("%s: Failed to create default messenger channel\n", __func__);
		return -1;
	}
	mv_pp3_channel_reg(id, 0);

	return 0;
}

/* Interrupt handling */
irqreturn_t mv_msg_isr(int irq, void *pchan)
{
	struct mv_pp3_channel *chan_ctrl = (struct mv_pp3_channel *)pchan;

	chan_ctrl->ch_stat.events_cntr++;

	/* Mask all RX events of relevant queue */
	mv_pp3_hmac_rxq_event_disable(chan_ctrl->frame, chan_ctrl->hmac_sw_rxq);

	tasklet_schedule(&chan_ctrl->channel_tasklet);

	return IRQ_HANDLED;
}

void mv_pp3_msg_tasklet(unsigned long data)
{
	struct mv_pp3_channel *chan_ctrl = (struct mv_pp3_channel *)data;

	/* TODO: verify that interrupt occured */

	if (chan_ctrl->status & MV_PP3_F_CHANNEL_CREATED)
		mv_pp3_chan_rx_event(chan_ctrl);

	/* Unmask all RX events of relevant queue */
	mv_pp3_hmac_rxq_event_enable(chan_ctrl->frame, chan_ctrl->hmac_sw_rxq);
}


/* Create communication channel (bi-directional)
Inputs:
	client_id - channel client ID
	size - number of CFHs with maximum CFH size (128 byte)
	rcv_cb - callback function to be called when message from Firmware is received on this channel.
Return:
	positive - unique channel ID,
	negative - failure
*/
int mv_pp3_chan_create(unsigned char client_id, int size, mv_pp3_chan_rcv_func rcv_cb)
{
	struct mv_pp3_fw_msg_chan_cfg msg;
	int chan_num;
	int frame, queue;
	int msg_flags;
	int group, irq_num;
	unsigned char txq;
	unsigned long iflags = 0;

	if (pp3_priv == NULL) {
		pr_err("%s: system doesn't configure properly\n", __func__);
		return -1;
	}
	if (mv_pp3_active_chan_num == MV_PP3_MAX_CHAN_NUM)
		return -1;

	MV_LOCK(&msngr_channel_lock, iflags);
	/* get ID of current channnel */
	chan_num = mv_pp3_active_chan_num++;
	MV_UNLOCK(&msngr_channel_lock, iflags);

	/* get free frame number and queue number to use by channel */
	if (mv_pp3_cfg_chan_sw_params_get(chan_num, &frame, &queue, &group, &irq_num))
		return -1;

	/* create HMAC queue pair */
	if (mv_pp3_hmac_rxq_init(frame, queue, size*MV_PP3_CFH_MAX_SIZE/MV_PP3_CFH_DG_SIZE) == NULL)
		return -1;
	if (mv_pp3_hmac_txq_init(frame, queue, size*MV_PP3_CFH_MAX_SIZE/MV_PP3_CFH_DG_SIZE, 0) == NULL)
		return -1;

	mv_pp3_chan_ctrl[chan_num].size = size;
	/* create array to store size of "ready to send" CFHs */
	mv_pp3_chan_ctrl[chan_num].ready_to_send = kzalloc(size*4, GFP_KERNEL);
	if (mv_pp3_chan_ctrl[chan_num].ready_to_send == NULL) {
		pr_err("%s: Failed to allocate %d bytes for channel %d\n", __func__, size*4, chan_num);
		return -1;
	}
	mv_pp3_chan_ctrl[chan_num].ready_to_send_ind = 0;
	mv_pp3_chan_ctrl[chan_num].free_ind = 0;

	mv_pp3_chan_ctrl[chan_num].hmac_sw_rxq = queue;
	mv_pp3_chan_ctrl[chan_num].hmac_sw_txq = queue;
	mv_pp3_chan_ctrl[chan_num].event_group = group;
	mv_pp3_chan_ctrl[chan_num].rcv_func = rcv_cb;
	mv_pp3_chan_ctrl[chan_num].frame = frame;
	mv_pp3_chan_ctrl[chan_num].id = chan_num;
	mv_pp3_chan_ctrl[chan_num].status = 0;
	memset(&mv_pp3_chan_ctrl[chan_num].ch_stat, 0, sizeof(struct mv_pp3_chan_cntrs));

	/* connect HMAC queue to interrupt group */
	mv_pp3_hmac_rxq_event_cfg(frame, queue, MV_PP3_RX_CFH, group);

	/*  get HW q numbers and BM ID */
	if (mv_pp3_cfg_chan_hw_params_get(chan_num, &msg.hmac_hw_rxq, &txq))
		return -1;
	/* configure back pressure on HW queue */
	mv_pp3_hmac_rxq_bp_node_set(frame, queue, MV_QM_Q_NODE, msg.hmac_hw_rxq);

	mv_pp3_chan_ctrl[chan_num].hmac_hw_rxq = msg.hmac_hw_rxq;
	mv_pp3_chan_ctrl[chan_num].hmac_hw_txq = txq;

	mv_pp3_hmac_queue_qm_mode_cfg(mv_pp3_chan_ctrl[chan_num].frame, mv_pp3_chan_ctrl[chan_num].hmac_sw_rxq, txq);

	/* enable channel */
	mv_pp3_hmac_rxq_enable(mv_pp3_chan_ctrl[chan_num].frame, mv_pp3_chan_ctrl[chan_num].hmac_sw_rxq);
	mv_pp3_hmac_txq_enable(mv_pp3_chan_ctrl[chan_num].frame, mv_pp3_chan_ctrl[chan_num].hmac_sw_txq);

	/* build channel create message with SW and HW queues numbers */
	msg.chan_id = chan_num;
	msg.hmac_sw_rxq = queue + MV_PP3_HFRM_Q_NUM * frame;
	msg.hmac_hw_rxq = cpu_to_be16(msg.hmac_hw_rxq);

	/* create tasklet for interrupt handling */
	tasklet_init(&mv_pp3_chan_ctrl[chan_num].channel_tasklet, mv_pp3_msg_tasklet,
		(unsigned long)&mv_pp3_chan_ctrl[chan_num]);

	/* connect ISR to IRQ */
	sprintf(mv_pp3_chan_ctrl[chan_num].mv_chan_isr, "pp3_channel_%d", chan_num);
	if (request_irq(irq_num, mv_msg_isr, IRQF_SHARED | IRQF_TRIGGER_RISING,
			mv_pp3_chan_ctrl[chan_num].mv_chan_isr, &mv_pp3_chan_ctrl[chan_num]))
		pr_err("Failed to assign IRQ %d to channel %d\n", irq_num, chan_num);
	else
		pr_info("Assign IRQ %d to channel %d\n", irq_num, chan_num);
	mv_pp3_chan_ctrl[chan_num].irq_num = irq_num;

	/* send message with channel info on default channel used only for system control */
	msg_flags = MV_PP3_F_CFH_MSG_ACK;
	mv_pp3_msg_send(mv_pp3_chan_ctrl[0].id, &msg, sizeof(msg), msg_flags, MV_FW_MSG_CHAN_SET, 0, 1);
	mv_pp3_chan_ctrl[chan_num].status |= MV_PP3_F_CHANNEL_CREATED;

	/* Enable events on HMAC only */
	mv_pp3_hmac_group_event_unmask(frame, group);

	return chan_num;
}
EXPORT_SYMBOL(mv_pp3_chan_create);

/* Registerate RX channel event to specified cpu
Inputs:
	chan - unique channel id
	cpu - cpu number
Return:
	0  - success.
	-1 - failure
*/
int mv_pp3_channel_reg(int chan, int cpu)
{
	if (!(mv_pp3_chan_ctrl[chan].status & MV_PP3_F_CHANNEL_CREATED))
		/* channel not created */
		return -1;

	*(per_cpu_ptr(mv_pp3_ch_bmp, cpu)) |= (1 << chan);

	if (irq_set_affinity(mv_pp3_chan_ctrl[chan].irq_num, cpumask_of(cpu)))
		pr_err("Failed to set affinity IRQ %d to cpu %d device\n",
			mv_pp3_chan_ctrl[chan].irq_num, cpu);

	mv_pp3_chan_ctrl[chan].cpu_mask |= (1 << cpu);

	return 0;

}
EXPORT_SYMBOL(mv_pp3_channel_reg);

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
int mv_pp3_msg_send(int chan, void *msg, int size, int flags, u16 msg_opcode, int msg_seq_num, int num)
{
	struct host_msg *cfh_ptr;
	int msg_size;
	int cfh_size;		/* CFH size in datagrams (16 bytes each) */
	unsigned long iflags = 0;

	if (!(mv_pp3_chan_ctrl[chan].status & MV_PP3_F_CHANNEL_CREATED) && (msg_opcode != MV_FW_MSG_CHAN_SET)) {
		/* channel wasn't created */
		pr_err("\n%s:: try send message to unknown channel %d", __func__, chan);
		return -1;
	}

	/* calculate real FW message size = user msg + msg header */
	msg_size = size + MV_CFH_FW_MSG_HEADER_BYTES;
	if (msg_size > MV_PP3_MSG_BUFF_SIZE) {
		mv_pp3_chan_ctrl[chan].ch_stat.msg_tx_err++;
		pr_err("\n%s:: channel %d: cannot send message of %d bytes", __func__, chan, msg_size);
		return -1;
	}

	if (msg_size > (MV_PP3_CFH_MAX_SIZE - MV_PP3_CFH_HDR_SIZE))

		/* send message in buffer (by pointer) */
		/* calc CFH size */
		cfh_size = (MV_PP3_CFH_HDR_SIZE + MV_CFH_FW_MSG_HEADER_BYTES)/MV_PP3_CFH_DG_SIZE;
	else {
		/* send in CFH */
		/* calc CFH size alligned to 16 bytes (1 datagram) */
		cfh_size = (MV_ALIGN_UP(msg_size, MV_PP3_CFH_DG_SIZE) +
			MV_PP3_CFH_HDR_SIZE)/MV_PP3_CFH_DG_SIZE;
	}

	MV_LOCK(&msngr_channel_lock, iflags);
	/* get pointer to CFH */
	cfh_ptr = (struct host_msg *)mv_pp3_hmac_txq_next_cfh(mv_pp3_chan_ctrl[chan].frame,
		mv_pp3_chan_ctrl[chan].hmac_sw_txq, cfh_size);

	/* check CFH pointer */
	if (cfh_ptr == NULL) {
		mv_pp3_chan_ctrl[chan].ch_stat.msg_tx_err++;
		pr_err("\n%s:: channel %d: no free CFH", __func__, chan);
		MV_UNLOCK(&msngr_channel_lock, iflags);
		return -1;	/* no free CFH in queue */
	}

	/* ONLY for debug phase:: clean CFH memory */
	memset(cfh_ptr, 0, (cfh_size * MV_PP3_CFH_DG_SIZE));

	/* build message header and convert it to BE from native cpu format */
	cfh_ptr->msg_header.word0 = cpu_to_be32(MV_HOST_MSG_INST_NUM_SET(num) |
		MV_HOST_MSG_SIZE_SET(msg_size));

	cfh_ptr->msg_header.word1 = MV_HOST_MSG_OPCODE_SET(msg_opcode);
	cfh_ptr->msg_header.word1 |= MV_HOST_MSG_SEQ_NUM_SET(msg_seq_num);
	if (flags & MV_PP3_F_CFH_EXT_HDR)
		cfh_ptr->msg_header.word1 |= MV_HOST_MSG_EXT_HDR_SET(1);
	if (flags & MV_PP3_F_CFH_MSG_ACK)
		cfh_ptr->msg_header.word1 |= MV_HOST_MSG_ACK_SET(1);
	cfh_ptr->msg_header.word1 = cpu_to_be32(cfh_ptr->msg_header.word1);

	/* fill common CFH fields */
	cfh_ptr->common_cfh[2] = MV_HOST_MSG_CHAN_ID(chan);

	/* up message size to 16 bytes only for common CFH part */
	/* in message header put real msg_size */
	if (msg_size < MV_PP3_CFH_DG_SIZE)
		msg_size = MV_PP3_CFH_DG_SIZE;

	if (msg_size > (MV_PP3_CFH_MAX_SIZE - MV_PP3_CFH_HDR_SIZE)) {
		mv_pp3_chan_ctrl[chan].ch_stat.msg_tx_err++;
		pr_err("\n%s:: channel %d: message too large (%d)", __func__, chan, msg_size);
		MV_UNLOCK(&msngr_channel_lock, iflags);
		return -1;
	} else {
		/* send message in CFH (by value) by write all relevant data to CFH */
		/* BUILD CFH in descriptor mode (not message mode) */
		cfh_ptr->common_cfh[0] = MV_HOST_MSG_PACKET_LENGTH(msg_size) + MV_HOST_MSG_DESCR_MODE;
		cfh_ptr->common_cfh[1] = (MV_HOST_MSG_CFH_LENGTH(msg_size + MV_PP3_CFH_HDR_SIZE)) +
			(HMAC_CFH << MV_CFH_MODE_OFFS) + (PP_TX_MESSAGE << MV_CFH_PP_MODE_OFFS);
		if (size > 0)
			memcpy((char *)(cfh_ptr->fw_msg), msg, size);
	}

	cfh_size += mv_pp3_hmac_txq_dummy_dg_get(mv_pp3_chan_ctrl[chan].frame, mv_pp3_chan_ctrl[chan].hmac_sw_txq);

	if (msngr_free) {
		msngr_free = false;
		/* send CFH to FW */
		wmb();
		if (mv_pp3_tx_msg_print_en)
			mv_pp3_message_print("Sent", cfh_ptr, size, msg_seq_num, msg_opcode, chan);
		mv_pp3_hmac_txq_send(mv_pp3_chan_ctrl[chan].frame, mv_pp3_chan_ctrl[chan].hmac_sw_txq, cfh_size);
		mv_pp3_chan_ctrl[chan].ch_stat.msg_tx++;
	} else {
		if (mv_pp3_tx_msg_print_en)
			mv_pp3_message_print("Pend", cfh_ptr, size, msg_seq_num, msg_opcode, chan);
		/* store message for send it later */
		mv_pp3_chan_ctrl[chan].ready_to_send[mv_pp3_chan_ctrl[chan].free_ind] = cfh_size;
		mv_pp3_chan_ctrl[chan].free_ind++;
		if (mv_pp3_chan_ctrl[chan].free_ind == mv_pp3_chan_ctrl[chan].size)
			mv_pp3_chan_ctrl[chan].free_ind = 0;
		mv_pp3_chan_ctrl[chan].ch_stat.msg_tx_pend++;
	}
	MV_UNLOCK(&msngr_channel_lock, iflags);

	return 0;
}
EXPORT_SYMBOL(mv_pp3_msg_send);


static void mv_pp3_send_pend_msg(int bitmap)
{
	static int last_proc;   /* last channel with pending CFH that was sent */
	u16 cfh_ind;
	int cfh_size;
	int chan = 0, i;

	/* choose channel for transmit */
	/* channel 0 must be handle first */

	if (!bitmap)
		return;
	else if (bitmap & 1)
		chan = 0;
	else if (bitmap & (~(1 << last_proc))) {
		bitmap &= ~(1 << last_proc);
		for (i = 0; i < mv_pp3_active_chan_num; i++) {
			if ((bitmap >> i) & 1) {
				chan = i;
				last_proc = i;
				break;
			}
		}
	} else if (bitmap & (1 << last_proc))
		chan = last_proc;

	cfh_ind = mv_pp3_chan_ctrl[chan].ready_to_send_ind;

	if (mv_pp3_chan_ctrl[chan].ready_to_send[cfh_ind] != 0) {

		cfh_size = mv_pp3_chan_ctrl[chan].ready_to_send[cfh_ind];

		if (mv_pp3_tx_msg_print_en)
			pr_info("%s: send message (%d) size (%d) on channel %d\n", __func__, cfh_ind, cfh_size, chan);

		/* Memory barrier before start transmit */
		wmb();

		mv_pp3_hmac_txq_send(mv_pp3_chan_ctrl[chan].frame, mv_pp3_chan_ctrl[chan].hmac_sw_txq, cfh_size);
		mv_pp3_chan_ctrl[chan].ready_to_send[cfh_ind] = 0;
		mv_pp3_chan_ctrl[chan].ready_to_send_ind++;
		if (mv_pp3_chan_ctrl[chan].ready_to_send_ind == mv_pp3_chan_ctrl[chan].size)
			mv_pp3_chan_ctrl[chan].ready_to_send_ind = 0;
		mv_pp3_chan_ctrl[chan].ch_stat.msg_tx++;
		mv_pp3_chan_ctrl[chan].ch_stat.msg_tx_pend--;
	}
}

/* Rx message event handler.
Inputs:
	chan - unique channel id
*/
static void mv_pp3_chan_rx_event(struct mv_pp3_channel *chan)
{
	struct host_msg *cfh_ptr;
	int dg_num, proc_dg;
	int cfh_size;
	bool ack_rec = false;
	int msg_seq_num;
	u16 msg_opcode;
	int msg_size;
	u32 word0, word1;
	unsigned long iflags = 0;
	int msg_flags;
	int ret_code, num_ok;

	if (chan) {
		if (!(chan->status & MV_PP3_F_CHANNEL_CREATED))
			return; /* channel wasn't created */

		/* get number of recieved DG */
		dg_num = proc_dg = mv_pp3_hmac_rxq_occ_get(chan->frame, chan->hmac_sw_rxq);
		if (dg_num > chan->size*MV_PP3_CFH_MAX_SIZE/MV_PP3_CFH_DG_SIZE) {
			pr_err("%s: bad occupite datagramm counter %d received on channel %d: frame %d, queue %d.",
				__func__, dg_num, chan->id, chan->frame, chan->hmac_sw_rxq);
			return;
		}
		mv_pp3_os_cache_io_sync(mv_pp3_dev_get(pp3_priv));

		while (proc_dg > 0) {
			/* get msg from HMAC RX queue */
			cfh_ptr = (struct host_msg *)
				mv_pp3_hmac_rxq_next_cfh(chan->frame, chan->hmac_sw_rxq, &cfh_size);
			if ((cfh_size == 0) || (cfh_size > MV_PP3_CFH_MAX_SIZE) || (cfh_size > proc_dg)) {
				chan->ch_stat.msg_rx_err++;
				pr_err("%s: error CFH with wrong size %d received on channel %d (%d). Frame %d, queue %d.",
					__func__, cfh_size, chan->id, dg_num, chan->frame, chan->hmac_sw_rxq);
				break;
			}
			if (cfh_size > proc_dg) {
				mv_pp3_hmac_rxq_cfh_free(chan->frame, chan->hmac_sw_rxq, cfh_size);
				/* only part of CFH is moved to DRAM */
				/* in next interrupt will processed full CFH */
				break;
			}
			proc_dg -= cfh_size;
			if (cfh_ptr == NULL)
				continue; /* get WA CFH */
			chan->ch_stat.msg_rx++;
			/* convert BE message header to native cpu format */
			word0 = be32_to_cpu(cfh_ptr->msg_header.word0);
			word1 = be32_to_cpu(cfh_ptr->msg_header.word1);

			msg_flags = 0;
			if (MV_HOST_MSG_ACK_GET(word1)) {
				ack_rec = true;
				msg_flags |= MV_PP3_F_CFH_MSG_ACK;
			}
			if (MV_HOST_MSG_EXT_HDR_GET(word1))
				msg_flags |= MV_PP3_F_CFH_EXT_HDR;

			/* get message header fields */
			num_ok = MV_HOST_MSG_INST_NUM_GET(word0);
			msg_opcode = MV_HOST_MSG_OPCODE_GET(word1);
			msg_size = MV_HOST_MSG_SIZE_GET(word0) - MV_CFH_FW_MSG_HEADER_BYTES;

			msg_seq_num = MV_HOST_MSG_SEQ_NUM_GET(word1);
			ret_code = MV_HOST_MSG_RC_GET(word1);

			if (mv_pp3_rx_msg_print_en) {
				MV_LOCK(&msngr_channel_lock, iflags);
				mv_pp3_message_print("Receive", cfh_ptr, msg_size, msg_seq_num, msg_opcode, chan->id);
				MV_UNLOCK(&msngr_channel_lock, iflags);
			}

			if (chan->rcv_func == NULL)
				continue;

			chan->rcv_func(chan->id, (char *)cfh_ptr->fw_msg, msg_size, msg_seq_num, msg_flags,
				msg_opcode, ret_code, num_ok);
		}
		if (ack_rec) {
			u16 cfh_ind;
			int i;
			u32 bitmap = 0;
			MV_LOCK(&msngr_channel_lock, iflags);

			/* scan all channels and build bitmap of candidate with pending req */
			for (i = 0; i < mv_pp3_active_chan_num; i++) {
				if (mv_pp3_chan_ctrl[i].size == 0)
					continue;
				cfh_ind = mv_pp3_chan_ctrl[i].ready_to_send_ind;
				if (mv_pp3_chan_ctrl[i].ready_to_send &&
					(mv_pp3_chan_ctrl[i].ready_to_send[cfh_ind] != 0))
					bitmap |= (1 << i);
			}

			if (bitmap == 0)
				msngr_free = true;
			else
				mv_pp3_send_pend_msg(bitmap);

			MV_UNLOCK(&msngr_channel_lock, iflags);
		}

		/* Write a number of processed datagram */
		if ((dg_num - proc_dg) > 0)
			mv_pp3_hmac_rxq_occ_set(chan->frame, chan->hmac_sw_rxq, dg_num - proc_dg);
	}

	return;
}

/*********************** sysFS functions *******************************/
void mv_pp3_channel_show(int ch_num)
{
	struct mv_pp3_channel *ch_ptr = &mv_pp3_chan_ctrl[ch_num];

	if (ch_ptr->rcv_func == NULL)
		pr_info("\nChannel %d: no callback function connected", ch_num);
	else
		pr_info("\nChannel %d:", ch_num);
	pr_info("\tHMAC: frame %d, RXQ %d, TXQ %d", ch_ptr->frame, ch_ptr->hmac_sw_rxq, ch_ptr->hmac_sw_txq);
	pr_info("\tStatistics:");
	pr_info("\t\tNumber of sent messages              :%10d\n", ch_ptr->ch_stat.msg_tx);
	pr_info("\t\tNext index to add pended message     :%10d\n", ch_ptr->free_ind);
	pr_info("\t\tNext index to send pended message    :%10d\n", ch_ptr->ready_to_send_ind);
	pr_info("\t\tNumber of received messages          :%10d\n", ch_ptr->ch_stat.msg_rx);
	pr_info("\t\tNumber of errors through messages TX :%10d\n", ch_ptr->ch_stat.msg_tx_err);
	pr_info("\t\tNumber of errors through messages RX :%10d\n", ch_ptr->ch_stat.msg_rx_err);
	pr_info("\t\tNumber of messages waited for tx     :%10d\n", ch_ptr->ch_stat.msg_tx_pend);
	pr_info("\t\tNumber of rx events                  :%10d\n", ch_ptr->ch_stat.events_cntr);
	pr_info("\n");

	return;
}

/*************************************** debug functions **********************************************/
void mv_pp3_msg_receive(int chan)
{
	struct mv_pp3_channel *ch_ptr = &mv_pp3_chan_ctrl[chan];

	mv_pp3_chan_rx_event(ch_ptr);
}

/* Channel callback
Inputs:
	chan - unique channel id
	msg  - pointer to message to send
	size - size of message (in bytes)
	seq_num - message sequence number
	flags - message flags:
		Bit 0 - Acknowledge or Reply is received.
		Bit 1 - Buffer extension header existence
	msg_opcode - unique message type identifier.
	ret_code - status returned by the Firmware for the whole request.
	num_ok - number of messages successfully processed by FW.
*/
void pp3_msg_chan_callback(int chan, void *msg, int size, int seq_num, int flags, u16 msg_opcode,
		int ret_code, int num_ok)
{
	int i;
	u8 *msg_ptr = (u8 *)msg;

	pr_info("\nReceive %d bytes MSG seq_num=%d, flags=%d, msg_opcode=%d ::\n", size, seq_num, flags, msg_opcode);
	for (i = 0; i < size; i++)
		pr_info("0x%x ", msg_ptr[i]);
}

void mv_pp3_chan_create_cmd(int size, int flags)
{
	int chan;

	chan = mv_pp3_chan_create(MV_DRIVER_CL_ID, size, pp3_msg_chan_callback);

	pr_info("Create HOST <-> FW channel %d\n", chan);
}

void mv_pp3_msg_rx_poll(int cpu)
{
	int i;

	/* check RX occup counter on each active channel */
	for (i = 0; i < MV_PP3_MAX_CHAN_NUM; i++) {
		if ((mv_pp3_chan_ctrl[i].status & MV_PP3_F_CHANNEL_CREATED) &&
		    (*(per_cpu_ptr(mv_pp3_ch_bmp, cpu)) & (1 << i)))
			mv_pp3_chan_rx_event(&mv_pp3_chan_ctrl[i]);
	}
}

/* Close communication channel
Inputs:
	chan - unique channel id
eturn:
	 0 - success
	-1 - fail
*/
int mv_pp3_chan_delete(int chan)
{
	if (!(mv_pp3_chan_ctrl[chan].status & MV_PP3_F_CHANNEL_CREATED)) {
		pr_err("Channel %d wasn't created", chan);
		return -1;
	}
	/* disconnect IRQ */
	disable_irq(mv_pp3_chan_ctrl[chan].irq_num);
	free_irq(mv_pp3_chan_ctrl[chan].irq_num, (void *)&mv_pp3_chan_ctrl[chan]);

	/* disable channel */
	mv_pp3_hmac_rxq_flush(mv_pp3_chan_ctrl[chan].frame, mv_pp3_chan_ctrl[chan].hmac_sw_rxq);
	mv_pp3_hmac_rxq_delete(mv_pp3_chan_ctrl[chan].frame, mv_pp3_chan_ctrl[chan].hmac_sw_rxq);
	mv_pp3_hmac_txq_flush(mv_pp3_chan_ctrl[chan].frame, mv_pp3_chan_ctrl[chan].hmac_sw_txq);
	mv_pp3_hmac_txq_delete(mv_pp3_chan_ctrl[chan].frame, mv_pp3_chan_ctrl[chan].hmac_sw_txq);
	kfree(mv_pp3_chan_ctrl[chan].ready_to_send);
	memset(&mv_pp3_chan_ctrl[chan], 0, sizeof(struct mv_pp3_channel));

	return 0;
}
EXPORT_SYMBOL(mv_pp3_chan_delete);

static void mv_pp3_message_print(char *str, struct host_msg *cfh_ptr, int size, int num, int opcode, int chan)
{

	u8  *tmp;
	int i;

	pr_info("\n%s on cpu %d message N%d opcode %d with data length %d to channel %d:",
		str, smp_processor_id(), num, opcode, size + MV_CFH_FW_MSG_HEADER_BYTES, chan);

	tmp = (u8 *)&(cfh_ptr->msg_header);
	pr_info("Message header: ");
	for (i = 0; i < 8; i++)
		pr_cont("%02x ", tmp[i]);
	pr_info("Message data  : ");
	tmp = (u8 *)cfh_ptr->fw_msg;
	for (i = 0; i < size; i++) {
		if ((i != 0) && ((i%16) == 0))
			pr_cont("\n                ");
		pr_cont("%02x ", tmp[i]);
	}
	pr_info("\n");
}

/* Enable / disable print out of sent / received messages */
void mv_pp3_debug_message_print_en(bool rx_en, bool tx_en)
{
	mv_pp3_rx_msg_print_en = rx_en;
	mv_pp3_tx_msg_print_en = tx_en;
}

void mv_pp3_messenger_close(void)
{
	int i;

	/* delete all channels */
	for (i = 0; i < MV_PP3_MAX_CHAN_NUM; i++) {
		if (mv_pp3_chan_ctrl[i].status & MV_PP3_F_CHANNEL_CREATED)
			mv_pp3_chan_delete(i);
	}

	/* number of active channels */
	mv_pp3_active_chan_num = 0;
}

