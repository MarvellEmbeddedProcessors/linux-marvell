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

#ifndef __mv_pp3_msg_chan_h__
#define __mv_pp3_msg_chan_h__

#include <linux/interrupt.h>

#include "platform/mv_pp3.h"
#include "common/mv_sw_if.h"
#include "fw/mv_fw_shared.h"
#include "msg/mv_pp3_msg.h"

#define MV_PP3_MSG_BUF_HROOM	32

#ifdef CONFIG_MV_CHAN_STAT_INF
#define CHAN_STAT_INFO(c) c
#else
#define CHAN_STAT_INFO(c)
#endif


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

/* channel counters */
struct mv_pp3_chan_cntrs {
	u32 msg_tx;		/* total number of sent messages */
	u32 msg_tx_pend;	/* total number of pended messages */
	u32 msg_rx;		/* number of received messages */
	u32 msg_tx_err;		/* number of didn't sent messages */
	u32 msg_rx_err;		/* number of errors through message rx */
	u32 events_cntr;	/* number of ISR calls */
};

/* channel configuration parameters */
struct mv_pp3_channel {
	u8			id;		/* channel number */
	u8			cpu_mask;	/* channel cpu mask */
	u8			frame;		/* HMAC frame number */
	u8			hmac_sw_rxq;	/* HMAC queue number in frame */
	u8			hmac_sw_txq;	/* HMAC queue number in frame */
	u16			hmac_hw_rxq;	/* HMAC queue number in frame */
	u16			hmac_hw_txq;	/* HMAC queue number in frame */
	u16			size;		/* max number of messages in queue */
	u8			event_group;	/* Rx queue event group number */
	u8			bm_pool_id;	/* used for messages that Host send to Firmware */
	u8			buf_headroom;	/* headroom defined for BM pool buffer */
	u32			status;
	int			*ready_to_send;		/* list of CFH sizes waiting for send */
	u16			ready_to_send_ind;	/* CFH index - ready_to_send */
	u16			free_ind;		/* free index in ready for send */
	mv_pp3_chan_rcv_func	rcv_func;
	struct mv_pp3_chan_cntrs ch_stat;	/* channel statistics */
	struct tasklet_struct	channel_tasklet;
	int			irq_num;	/* IRQ number */
	char			mv_chan_isr[15];
};

/* HOST <-> FW message format and fields definition */
struct host_msg {
	u32				common_cfh[MV_PP3_CFH_COMMON_WORDS];
	struct mv_pp3_fw_msg_header	msg_header;
	u32				fw_msg[1];
};


/* Init messenger facility - call ones
Return:
	>= 0 - unique channel ID,
	-1   - failure
*/
int mv_pp3_messenger_init(struct mv_pp3 *priv);
void mv_pp3_messenger_close(void);

/* internal channel interface functions declartion */
void mv_pp3_msg_receive(int chan);
void mv_pp3_chan_create_cmd(int size, int flags);

void mv_pp3_msg_rx_poll(int cpu);

/* sysfs related */
void mv_pp3_channel_show(int ch_num);
int mv_pp3_chan_sysfs_init(struct kobject *pp3_kobj);
int mv_pp3_chan_sysfs_exit(struct kobject *hmac_kobj);

/* debug related */
struct mv_pp3_channel *mv_pp3_chan_get(int ch_num);
int mv_pp3_chan_num_get(void);

#endif /* __mv_pp3_msg_chan_h__ */
