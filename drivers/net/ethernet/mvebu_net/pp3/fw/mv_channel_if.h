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

#ifndef __mvChnlIf_h__
#define __mvChnlIf_h__

#define MV_PP3_MAX_CHAN_NUM 16

/*
	event - HMAC Rx event
	 * 0 - QM queue - Timeout or new items added to the queue
	 *     BM queue - allocate completed
 */
enum mv_pp3_hmac_rx_event {
	MV_PP3_RX_CFH = 0,
	MV_PP3_QU_FULL
};

/* Channel flags definition */
#define MV_PP3_F_CHANNEL_CREATED	(0x1)
#define MV_PP3_F_CPU_SHARED_CHANNEL	(0x2)


/* Message flags definition */
#define MV_PP3_F_CFH_MSG_ACK		(0x1)

/* Receive message callback prototype
Inputs:
	chan -  unique channel id as returned by "mv_pp3_chan_add" function
	msg - pointer to received message
	size - size of message
*/
typedef void (*mv_pp3_chan_rcv_func)(int chan, void *msg, int size);


/* channel configuration parameters */
struct mv_pp3_channel {
	u8			id;		/* channel number */
	u8			frame;		/* HMAC frame number */
	u8			hmac_rxq_num;	/* HMAC queue number in frame */
	u8			hmac_txq_num;	/* HMAC queue number in frame */
	u16			size;		/* max number of messages in queue */
	u8			bm_pool_id;	/* used for messages that Host send to Firmware */
	u8			buf_headroom;	/* headroom defined for BM pool buffer */
	u32			flags;
	u8			cpu_num;		/* cpu number for non shared channel */
	int			*ready_to_send;		/* list of CFH sizes waiting for send */
	u16			ready_to_send_ind;	/* CFH index - ready_to_send */
	u16			free_ind;		/* free index in ready for send */
	mv_pp3_chan_rcv_func	rcv_func;
};

/* Init messenger facility - call ones */
int mv_pp3_messenger_init(void);

/* Create communication channel (bi-directional)
Inputs:
	size - number of CFHs with maximum CFH size (128 byte)
	flags - channel flags
	rcv_cb - callback function to be called when message from Firmware is received on this channel.
Return:
	positive - unique channel ID,
	negative - failure
*/
int mv_pp3_chan_create(int size, int flags, mv_pp3_chan_rcv_func rcv_cb);
int mv_pp3_def_chan_create(int *size);

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
int mv_pp3_msg_send(int chan, void *msg, int size, int flags);

#endif /* __mvChnlIf_h__ */
