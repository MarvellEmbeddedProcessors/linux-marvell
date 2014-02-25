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

/* includes */
#include <linux/kernel.h>
#include <linux/io.h>
#include <linux/slab.h>
#include <linux/spinlock.h>

#include "common/mv_hw_if.h"
#include "common/mv_pp3_config.h"
#include "common/mv_stack.h"
#include "hmac/mv_hmac.h"
#include "hmac/mv_hmac_bm.h"
#include "mv_channel_if.h"
#include "mv_host_fw_if.h"


/* channel info */
static struct mv_pp3_channel mv_pp3_chan_ctrl[MV_PP3_MAX_CHAN_NUM];
/* number if active channels */
static int mv_pp3_active_chan_num;

/* stack of free buffers for messenger usage */
void *mv_pp3_msgr_buffers;

/* global one lock for all channels */
spinlock_t msngr_channel_lock;

/* Init messenger facility - call ones */
int mv_pp3_messenger_init(void)
{
	int bm_pool;
	int frame, queue, size;
	int i;
	void *buf_ptr;

	/* create lock mechanism */
	spin_lock_init(&msngr_channel_lock);

	/* create HMAC queue for messenger pool interface */
	/* get from configurator bm queue parameters (frame, queue, size) */
	frame = queue = 0;
	size = 0;
	mv_pp3_hmac_bm_queue_init(frame, queue, size);

	/* get from configurator GP pool number for messenger usage */
	bm_pool = 20;
	/* call to BM pool init wrapper - pool sixe */
	/* mv_pp3_bm_pool_init(bm_pool, 64*8);*/

	/* fill pool with buffers */
	/* mv_pp3_bm_pool_bufs_add(bm_pool, MV_PP3_MSG_BUFF_SIZE, 10); */

	/* create stack of free buffers for future usage by large messages */
	mv_pp3_msgr_buffers = mv_stack_create(MV_PP3_MSGR_BUF_NUM);
	for (i = 0; i < MV_PP3_MSGR_BUF_NUM; i++) {

		buf_ptr = kmalloc(MV_PP3_MSG_BUFF_SIZE, GFP_KERNEL);
		if (buf_ptr != NULL)
			mv_stack_push(mv_pp3_msgr_buffers, (u32)buf_ptr);
		else
			return -1;
	}
	return 0;
}

/* Create communication channel (bi-directional)
Inputs:
	size - number of CFHs with maximum CFH size (128 byte)
	flags - channel flags
	rcv_cb - callback function to be called when message from Firmware is received on this channel.
Return:
	positive - unique channel ID,
	negative - failure
*/
int mv_pp3_chan_create(int size, int flags, mv_pp3_chan_rcv_func rcv_cb)
{
	struct mv_pp3_msg_chan_cfg msg;
	void *msg_ptr = &msg;
	int chan_num;
	int frame, queue;
	int msg_flags;
	int group;

	if (mv_pp3_active_chan_num == MV_PP3_MAX_CHAN_NUM)
		return -1;

	/* TBD - call to configurator for frame number and queue number to use */
	frame = 0;
	queue = 0;

	spin_lock(&msngr_channel_lock);
	/* get ID of current channnel */
	chan_num = mv_pp3_active_chan_num++;
	spin_unlock(&msngr_channel_lock);

	/* create HMAC queue pair */
	mv_pp3_hmac_rxq_init(frame, queue, size*MV_PP3_CFH_MAX_SIZE);
	mv_pp3_hmac_txq_init(frame, queue, size*MV_PP3_CFH_MAX_SIZE, 0);

	mv_pp3_chan_ctrl[chan_num].size = size;
	/* create queue for ready to be sent CFHs */
	mv_pp3_chan_ctrl[chan_num].ready_to_send = kmalloc(size, GFP_KERNEL);
	if (mv_pp3_chan_ctrl[chan_num].ready_to_send == NULL)
		return -1;
	mv_pp3_chan_ctrl[chan_num].ready_to_send_ind = 0;
	mv_pp3_chan_ctrl[chan_num].free_ind = 0;

	mv_pp3_chan_ctrl[chan_num].hmac_rxq_num = queue;
	mv_pp3_chan_ctrl[chan_num].hmac_txq_num = queue;
	mv_pp3_chan_ctrl[chan_num].rcv_func = rcv_cb;
	mv_pp3_chan_ctrl[chan_num].frame = frame;
	mv_pp3_chan_ctrl[chan_num].id = chan_num;
	mv_pp3_chan_ctrl[chan_num].flags = flags;
	if (!(flags & MV_PP3_F_CPU_SHARED_CHANNEL))
		mv_pp3_chan_ctrl[chan_num].cpu_num = smp_processor_id();

	/* connect HMAC queue to interrupt group */
	/* TBD - call to configurator for SPI group */
	group = 2;
	mv_pp3_hmac_rxq_event_cfg(frame, queue, MV_PP3_RX_CFH, group);
	/* TBD - init all ISR releated */
	/* connect ISR to IRQ */
	/* mv_pp3_chan_isr(&mv_pp3_chan_ctrl[chan_num]) */

	/* enable channel */
	mv_pp3_hmac_rxq_enable(mv_pp3_chan_ctrl[chan_num].frame, mv_pp3_chan_ctrl[chan_num].hmac_rxq_num);
	mv_pp3_hmac_txq_enable(mv_pp3_chan_ctrl[chan_num].frame, mv_pp3_chan_ctrl[chan_num].hmac_txq_num);
	mv_pp3_chan_ctrl[chan_num].flags += MV_PP3_F_CHANNEL_CREATED;

	/* build channel create message with SW and HW queues numbers */
	msg.chan_id = chan_num;
	/* TBD - get from configurator SW, HW q numbers and BM (rx/tx) pools IDs */
	msg.hmac_sw_rxq = queue + 16 * frame;
	msg.hmac_hw_rxq = 364;
	msg.bm_pool_id = 20; /* BM pool for fw -> hmac messages */
	msg.buf_headroom = 32;
	msg.pool_buf_size = MV_PP3_MSG_BUFF_SIZE;

	mv_pp3_chan_ctrl[chan_num].bm_pool_id = 20; /* BM pool for hmac -> fw messages */
	mv_pp3_chan_ctrl[chan_num].buf_headroom = 32;

	/* send message through CPU default channel */
	msg_flags = 0;
	mv_pp3_msg_send(smp_processor_id(), msg_ptr, sizeof(msg), msg_flags);

	return chan_num;
}

/* Prepare message CFH and trigger it sending to firmware.
Inputs:
	chan - unique channel id
	msg  - pointer to message to send
	size - size of message (in bytes)
	flags - message flags
Return:
	0		- Message is accepted and sent to firmware.
	Other	- Failure: Queue is full, etc
*/
int mv_pp3_msg_send(int chan, void *msg, int size, int flags)
{
	struct cfh_common_format *cfh;
	char *msg_cfh;
	int cfh_size;
	u32 buf_addr;
	unsigned long irq_flags;

	if (!(mv_pp3_chan_ctrl[chan].flags & MV_PP3_F_CHANNEL_CREATED))
		/* channel wasn't created */
		return -1;

	if (size > (MV_PP3_CFH_MAX_SIZE - MV_PP3_CFH_MIN_SIZE))

		/* send message in buffer (by pointer) */
		/* calc CFH size */
		cfh_size = MV_PP3_CFH_MIN_SIZE/MV_PP3_HMAC_DG_SIZE;
	else
		/* send in CFH */
		cfh_size = (size + MV_PP3_CFH_MIN_SIZE)/MV_PP3_HMAC_DG_SIZE;

	/* disable interrupt on the current cpu */
	local_irq_save(irq_flags);
	/* get pointer to CFH */
	cfh = (struct cfh_common_format *)mv_pp3_hmac_txq_next_cfh(mv_pp3_chan_ctrl[chan].frame,
		mv_pp3_chan_ctrl[chan].hmac_rxq_num, cfh_size);
	/* enable interrupt on the current cpu */
	local_irq_restore(irq_flags);
	/* check CFH pointer */
	if (cfh == NULL)
		return -1;	/* no free CFH in queue */

	if (size > (MV_PP3_CFH_MAX_SIZE - MV_PP3_CFH_MIN_SIZE)) {

		/* send message in buffer (by pointer) */
		/* find free buffer and copy message */
		buf_addr = mv_stack_pop(mv_pp3_msgr_buffers);
		if (buf_addr)
			memcpy((char *)buf_addr + mv_pp3_chan_ctrl[chan].buf_headroom, msg, size);
		else
			return -1;

		/* write all relevant data to CFH */
		cfh->cfh_length = MV_PP3_CFH_MIN_SIZE;
		cfh->marker_low = buf_addr;
		cfh->addr_low = virt_to_phys((char *)buf_addr);
		cfh->bp_id = mv_pp3_chan_ctrl[chan].bm_pool_id;

	} else {

		/* send message in CFH (by value) */
		/* write all relevant data to CFH */
		cfh->cfh_length = size + MV_PP3_CFH_MIN_SIZE;
		msg_cfh = (char *)cfh + MV_CFH_MSG_OFFS;
		memcpy(msg_cfh, msg, size);
	}
	cfh->cfh_format = (HMAC_CFH << MV_PP3_MCG_CFH_MODE_OFFS) + PP_MESSAGE;
	cfh->tag2 = chan << MV_PP3_MCG_CHAN_ID_OFFS;

	if (spin_trylock(&msngr_channel_lock))
		/* send CFH to FW */
		mv_pp3_hmac_txq_send(mv_pp3_chan_ctrl[chan].frame, mv_pp3_chan_ctrl[chan].hmac_txq_num, cfh_size);
	else {
		/* store message for send it later */
		/* disable interrupt on the current cpu */
		local_irq_save(irq_flags);
		mv_pp3_chan_ctrl[chan].ready_to_send[mv_pp3_chan_ctrl[chan].free_ind++] = cfh_size;
		if (mv_pp3_chan_ctrl[chan].free_ind == mv_pp3_chan_ctrl[chan].size)
			mv_pp3_chan_ctrl[chan].free_ind = 0;
		/* enable interrupt on the current cpu */
		local_irq_restore(irq_flags);
	}

	return 0;
}

/* Prepare message CFH and trigger it sending to firmware.
Inputs:
	chan - unique channel id
	msg  - pointer to message to send
	size - size of message (in bytes)
	mode - message mode.
		0 - by value. Message will be copied before function return.
		1 - by pointer. "msg" pointer must be used after function return.
Return:
	0		- Message is accepted and sent to firmware.
	Other	- Failure: Queue is full, etc
*/
void mv_pp3_chan_isr(struct mv_pp3_channel *chan)
{
	struct cfh_common_format *cfh;
	int dg_num, proc_dg;
	int cfh_size;
	bool ack_rec = false;

	if (chan) {
		if (!(chan->flags & MV_PP3_F_CHANNEL_CREATED))
			return; /* channel wasn't created */

		/* get number of recieved DG */
		dg_num = proc_dg = mv_pp3_hmac_rxq_occ_get(chan->frame, chan->hmac_rxq_num);
		while (proc_dg > 0) {
			/* get msg from HMAC RX queue */
			cfh = (struct cfh_common_format *)
				mv_pp3_hmac_rxq_next_cfh(chan->frame, chan->hmac_rxq_num, &cfh_size);
			proc_dg -= cfh_size;
			if (cfh == NULL)
				continue; /* get WA CFH */

			if (MV_PP3_MSG_ACK_GET(cfh))
				ack_rec = true;

			if (chan->rcv_func == NULL)
				continue;

			if (cfh_size == MV_PP3_CFH_DG_NUM) {
				chan->rcv_func(chan->id, (char *)cfh->marker_low + chan->buf_headroom,
					cfh->pkt_length);
				/* return buffer to free buffers array */
				if (!mv_stack_push(mv_pp3_msgr_buffers, cfh->marker_low)) {
					/* if stack full, return buffer to pool */
					;/*mv_pp3_bm_pool_buf_add(bm_pool, cfh->marker_low);*/
				}
			} else
				chan->rcv_func(chan->id, (char *)cfh + MV_CFH_MSG_OFFS, cfh_size - MV_CFH_MSG_DG_OFFS);
		}
		if (ack_rec) {
			u16 cfh_ind = mv_pp3_chan_ctrl[chan->id].ready_to_send_ind++;

			cfh_size = mv_pp3_chan_ctrl[chan->id].ready_to_send[cfh_ind];
			if (cfh_size > 0) {
				mv_pp3_hmac_txq_send(mv_pp3_chan_ctrl[chan->id].frame,
						mv_pp3_chan_ctrl[chan->id].hmac_txq_num, cfh_size);
				mv_pp3_chan_ctrl[chan->id].ready_to_send[cfh_ind] = 0;
				if (mv_pp3_chan_ctrl[chan->id].ready_to_send_ind == mv_pp3_chan_ctrl[chan->id].size)
					mv_pp3_chan_ctrl[chan->id].ready_to_send_ind = 0;
			}
			spin_unlock(&msngr_channel_lock);
		}

		/* Write a number of processed datagram */
		mv_pp3_hmac_rxq_occ_set(chan->frame, chan->hmac_rxq_num, proc_dg);
	}

	return;
}
