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
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/workqueue.h>
#include <plat/ipc_dfev.h>

#include "mvTypes.h"
#include "mvOs.h"
#include "mvDebug.h"
#include "mvCommon.h"
#include "mvStack.h"
#include "mvIpc.h"
#include "mv_ipc/linux_amp/mv_ipc_os.h"
#include "mv_ipc/linux_amp/mv_ipc_common.h"
#include "cpu/mvCpu.h"
#include "ctrlEnv/mvCtrlEnvLib.h"

#define ipc_attach_chn(linkId, chnId, cpu, ret)	mvIpcAttachChannel(linkId, chnId, cpu, ret)
#define ipc_dettach_chn(linkId, chnId)		mvIpcDettachChannel(linkId, chnId)
#define ipc_close_chn(linkId, chnId)		mvIpcCloseChannel(linkId, chnId)
#define ipc_tx_msg(linkId, chnId, msg)		mvIpcTxMsg(linkId, chnId, msg)
#define ipc_rx_msg(linkId, chnId)		mvIpcRxMsg(linkId, chnId)
#define ipc_tx_ready(linkId, chnId)		mvIpcIsTxReady(linkId, chnId)
#define ipc_release_msg(linkId, chnId, msg)	mvIpcReleaseMsg(linkId, chnId, msg)
#define ipc_sh_malloc(linkId, size)		mvIpcShmemMalloc(linkId, size)
#define ipc_open_chn(linkId, chnId, rx_clbk)	mvIpcOpenChannel(linkId, chnId, rx_clbk)
#define ipc_enable_chn_rx(linkId, chnId)	mvIpcEnableChnRx(linkId, chnId)
#define ipc_disable_chn_rx(linkId, chnId)	mvIpcDisableChnRx(linkId, chnId)
#define ipc_virt_to_phys(linkId, virt_addr)	mvIpcOsVirt2Phys(linkId, virt_addr)
#define ipc_phys_to_virt(linkId, phys_addr)	mvIpcOsPhys2Virt(linkId, phys_addr)

#define IPC_DFEV_SLAVE_TO_MASTER_WAIT_TIME	1
#define IPC_DFEV_SLAVE_TO_MASTER_WAIT_LOOPS	100

/* IPC Identifiers */
#define IPC_DFEV_LINK_ID		0
#define IPC_DFEV_CTRL_CHAN_ID		0
#define IPC_DFEV_DATA_TX_CHAN_ID	1
#define IPC_DFEV_DATA_RX_CHAN_ID	2
#define IPC_DFEV_DATA_RETURN_CHAN_ID	3

/* Message type identifiers */
#define IPC_DFEV_MSG_TYPE_COMMAND	0
#define IPC_DFEV_MSG_TYPE_EVENT		1
#define IPC_DFEV_MSG_TYPE_RX		2
#define IPC_DFEV_MSG_TYPE_TX		3
#define IPC_DFEV_MSG_TYPE_PRINT		4

struct ipc_dfev_ctrl_buffer {
	atomic_t			in_use;
	struct ipc_dfev_ctrl_msg	msg;
};

struct ipc_dfev_ctrl_pool {
	spinlock_t			lock;
	unsigned int			idx;
	struct ipc_dfev_ctrl_buffer	*fifo[IPC_DFEV_CTRL_BUFFERS];
};

struct ipc_dfev_data_buffer {
	atomic_t			in_use;
	struct ipc_dfev_data_msg	msg;
};

struct ipc_dfev_data_pool {
	spinlock_t			lock;
	unsigned int			idx;
	struct ipc_dfev_data_buffer	*fifo[IPC_DFEV_DATA_BUFFERS];
};

static struct ipc_dfev_ctrl_handle {
	int				dummy;
} ipc_dfev_ctrl_handle;

static struct ipc_dfev_data_handle {
	int				dummy;
} ipc_dfev_data_handle;

static struct ipc_dfev_ctrl_ops *ipc_dfev_ctrl_ops;
static struct ipc_dfev_data_ops *ipc_dfev_data_ops;

static struct ipc_dfev_ctrl_pool ipc_dfev_ctrl_pool;
static struct ipc_dfev_data_pool ipc_dfev_data_pool;

static struct delayed_work ipc_dfev_linkup;

#define ipc_dfev_init_pool(ch, perror)					\
	do {								\
		unsigned int i;						\
									\
		*(perror) = 0;						\
		spin_lock_init(&(ch)->lock);				\
									\
		for (i = 0; i < ARRAY_SIZE((ch)->fifo); i++) {		\
			(ch)->fifo[i] = ipc_sh_malloc(IPC_DFEV_LINK_ID,	\
					     sizeof(*((ch)->fifo[i])));	\
			if ((ch)->fifo[i] == NULL) {			\
				*(perror) = -ENOMEM;			\
				break;					\
			}						\
									\
			atomic_set(&(ch)->fifo[i]->in_use, 0);		\
		}							\
	} while (0)

#define ipc_dfev_get_buffer(ch, pbuffer)				\
	do {								\
		unsigned long flags;					\
		unsigned int idx;					\
									\
		spin_lock_irqsave(&(ch)->lock, flags);			\
									\
		idx = (ch)->idx & (ARRAY_SIZE((ch)->fifo) - 1);		\
		if (atomic_read(&(ch)->fifo[idx]->in_use) == 0) {	\
			*(pbuffer) = (ch)->fifo[idx];			\
			atomic_set(&(*(pbuffer))->in_use, 1);		\
			(ch)->idx += 1;					\
		}							\
									\
		spin_unlock_irqrestore(&(ch)->lock, flags);		\
	} while (0)

static void ipc_dfev_handshake(struct work_struct *dummy)
{
	unsigned int channels_to_attach = 4;
	int attached;

	ipc_attach_chn(IPC_DFEV_LINK_ID, IPC_DFEV_CTRL_CHAN_ID,
			mvIpcGetlinkRemoteNodeId(IPC_DFEV_LINK_ID), &attached);
	if (attached)
		channels_to_attach -= 1;

	ipc_attach_chn(IPC_DFEV_LINK_ID, IPC_DFEV_DATA_TX_CHAN_ID,
			mvIpcGetlinkRemoteNodeId(IPC_DFEV_LINK_ID), &attached);
	if (attached)
		channels_to_attach -= 1;

	ipc_attach_chn(IPC_DFEV_LINK_ID, IPC_DFEV_DATA_RX_CHAN_ID,
			mvIpcGetlinkRemoteNodeId(IPC_DFEV_LINK_ID), &attached);
	if (attached)
		channels_to_attach -= 1;

	ipc_attach_chn(IPC_DFEV_LINK_ID, IPC_DFEV_DATA_RETURN_CHAN_ID,
			mvIpcGetlinkRemoteNodeId(IPC_DFEV_LINK_ID), &attached);
	if (attached)
		channels_to_attach -= 1;

	if (channels_to_attach != 0)
		schedule_delayed_work(&ipc_dfev_linkup, HZ);
}

static int ipc_dfev_send(int type, int channel, void *message)
{
	MV_IPC_MSG msg;

	msg.type = type;
	msg.size = 0;
	msg.ptr = ipc_virt_to_phys(IPC_DFEV_LINK_ID, message);

	if (ipc_tx_msg(IPC_DFEV_LINK_ID, channel, &msg) != MV_OK)
		return -EAGAIN;

	return 0;
}

static int ipc_dfev_open_channel(MV_U32 linkId, MV_U32 chId,
							MV_IPC_RX_CLBK rx_clbk)
{
	int attached, counter, status;

	counter = 0;
	do {
		status = ipc_open_chn(linkId, chId, rx_clbk);
		if (status == MV_NOT_STARTED) {
			if (counter++ >= IPC_DFEV_SLAVE_TO_MASTER_WAIT_LOOPS)
				return -ENODEV;
			udelay(IPC_DFEV_SLAVE_TO_MASTER_WAIT_TIME);
			continue;
		}

		if (status != MV_OK) {
			pr_err("IPC DFEV: Failed to open IPC channel %d-%d\n",
								linkId, chId);
			return -ENODEV;
		}
	} while (status != MV_OK);

	status = ipc_attach_chn(linkId, chId,
				mvIpcGetlinkRemoteNodeId(linkId), &attached);
	if (status != MV_OK) {
		pr_err("IPC DFEV: Failed to attach IPC channel %d-%d\n",
								linkId, chId);
		ipc_close_chn(linkId, chId);
		return -ENODEV;
	}

	ipc_enable_chn_rx(linkId, chId);

	return 0;
}

/*
 * Control Path API
 */
struct ipc_dfev_ctrl_handle *ipc_dfev_ctrl_init(enum ipc_dfev_mode mode,
					struct ipc_dfev_ctrl_ops *ctrl_ops)
{
	if (ctrl_ops == NULL)
		return NULL;

	/* Wait for IPC link */
	while (!ipc_tx_ready(IPC_DFEV_LINK_ID, IPC_DFEV_CTRL_CHAN_ID))
		msleep(25);

	switch (mode) {
	case IPC_DFEV_MODE_INTERRUPT:
		ipc_enable_chn_rx(IPC_DFEV_LINK_ID, IPC_DFEV_CTRL_CHAN_ID);
		break;
	case IPC_DFEV_MODE_POLLING:
		ipc_disable_chn_rx(IPC_DFEV_LINK_ID, IPC_DFEV_CTRL_CHAN_ID);
		break;
	default:
		return NULL;
	}

	ipc_dfev_ctrl_ops = ctrl_ops;

	return &ipc_dfev_ctrl_handle;
}
EXPORT_SYMBOL(ipc_dfev_ctrl_init);

void ipc_dfev_ctrl_exit(struct ipc_dfev_ctrl_handle *handle)
{
	ipc_dfev_ctrl_ops = NULL;
}
EXPORT_SYMBOL(ipc_dfev_ctrl_exit);

int ipc_dfev_ctrl_poll(struct ipc_dfev_ctrl_handle *handle)
{
	return ipc_rx_msg(IPC_DFEV_LINK_ID, IPC_DFEV_CTRL_CHAN_ID);
}
EXPORT_SYMBOL(ipc_dfev_ctrl_poll);

struct ipc_dfev_ctrl_msg *ipc_dfev_ctrl_msg_get(struct ipc_dfev_ctrl_handle *handle)
{
	struct ipc_dfev_ctrl_buffer *buffer = NULL;

	ipc_dfev_get_buffer(&ipc_dfev_ctrl_pool, &buffer);

	if (!buffer)
		return NULL;

	return &buffer->msg;
}
EXPORT_SYMBOL(ipc_dfev_ctrl_msg_get);

void ipc_dfev_ctrl_msg_put(struct ipc_dfev_ctrl_handle *handle, struct ipc_dfev_ctrl_msg *msg)
{
	struct ipc_dfev_ctrl_buffer *buffer;

	buffer = container_of(msg, struct ipc_dfev_ctrl_buffer, msg);
	atomic_set(&buffer->in_use, 0);
}
EXPORT_SYMBOL(ipc_dfev_ctrl_msg_put);

int ipc_dfev_send_command(struct ipc_dfev_ctrl_handle *handle, struct ipc_dfev_ctrl_msg *msg)
{
	return ipc_dfev_send(IPC_DFEV_MSG_TYPE_COMMAND, IPC_DFEV_CTRL_CHAN_ID, msg);
}
EXPORT_SYMBOL(ipc_dfev_send_command);

int ipc_dfev_send_event(struct ipc_dfev_ctrl_handle *handle, struct ipc_dfev_ctrl_msg *msg)
{
	return ipc_dfev_send(IPC_DFEV_MSG_TYPE_EVENT, IPC_DFEV_CTRL_CHAN_ID, msg);
}
EXPORT_SYMBOL(ipc_dfev_send_event);

static int ipc_dfev_ctrl_msg_handler(MV_IPC_MSG *msg)
{
	struct ipc_dfev_ctrl_msg *message;

	message = ipc_phys_to_virt(IPC_DFEV_LINK_ID, msg->ptr);

	switch (msg->type) {
	case IPC_DFEV_MSG_TYPE_COMMAND:
		if (ipc_dfev_ctrl_ops && ipc_dfev_ctrl_ops->ipc_dfev_command_callback)
			ipc_dfev_ctrl_ops->ipc_dfev_command_callback(message);
		else
			ipc_dfev_ctrl_msg_put(NULL, message);
		break;
	case IPC_DFEV_MSG_TYPE_EVENT:
		if (ipc_dfev_ctrl_ops && ipc_dfev_ctrl_ops->ipc_dfev_event_callback)
			ipc_dfev_ctrl_ops->ipc_dfev_event_callback(message);
		else
			ipc_dfev_ctrl_msg_put(NULL, message);
		break;
	case IPC_DFEV_MSG_TYPE_PRINT:
		if (message->size)
			printk(KERN_INFO "Dragonite: %s", message->payload);
		ipc_dfev_ctrl_msg_put(NULL, message);
		break;
	default:
		pr_err("IPC DFEV: Unknown ctrl message type %u!\n", msg->type);
	}

	ipc_release_msg(IPC_DFEV_LINK_ID, IPC_DFEV_CTRL_CHAN_ID, msg);
	return 0;
}

/*
 * Data Path API
 */
struct ipc_dfev_data_handle *ipc_dfev_data_init(enum ipc_dfev_mode mode,
					struct ipc_dfev_data_ops *data_ops)
{
	if (data_ops == NULL)
		return NULL;

	/* Wait for IPC link */
	while (!ipc_tx_ready(IPC_DFEV_LINK_ID, IPC_DFEV_DATA_RX_CHAN_ID) ||
	       !ipc_tx_ready(IPC_DFEV_LINK_ID, IPC_DFEV_DATA_TX_CHAN_ID) ||
	       !ipc_tx_ready(IPC_DFEV_LINK_ID, IPC_DFEV_DATA_RETURN_CHAN_ID)) {
		msleep(25);
	}

	switch (mode) {
	case IPC_DFEV_MODE_INTERRUPT:
		ipc_enable_chn_rx(IPC_DFEV_LINK_ID, IPC_DFEV_DATA_RX_CHAN_ID);
		ipc_enable_chn_rx(IPC_DFEV_LINK_ID, IPC_DFEV_DATA_TX_CHAN_ID);
		ipc_enable_chn_rx(IPC_DFEV_LINK_ID, IPC_DFEV_DATA_RETURN_CHAN_ID);
		break;
	case IPC_DFEV_MODE_POLLING:
		ipc_disable_chn_rx(IPC_DFEV_LINK_ID, IPC_DFEV_DATA_RX_CHAN_ID);
		ipc_disable_chn_rx(IPC_DFEV_LINK_ID, IPC_DFEV_DATA_TX_CHAN_ID);
		ipc_disable_chn_rx(IPC_DFEV_LINK_ID, IPC_DFEV_DATA_RETURN_CHAN_ID);
		break;
	default:
		return NULL;
	}

	ipc_dfev_data_ops = data_ops;
	return &ipc_dfev_data_handle;
}
EXPORT_SYMBOL(ipc_dfev_data_init);

void ipc_dfev_data_exit(struct ipc_dfev_data_handle *handle)
{
	ipc_dfev_data_ops = NULL;
}
EXPORT_SYMBOL(ipc_dfev_data_exit);

struct ipc_dfev_data_msg *ipc_dfev_data_msg_get(struct ipc_dfev_data_handle *handle)
{
	struct ipc_dfev_data_buffer *buffer = NULL;

	ipc_dfev_get_buffer(&ipc_dfev_data_pool, &buffer);

	if (!buffer)
		return NULL;

	return &buffer->msg;
}
EXPORT_SYMBOL(ipc_dfev_data_msg_get);

void ipc_dfev_data_msg_put(struct ipc_dfev_data_handle *handle, struct ipc_dfev_data_msg *msg)
{
	struct ipc_dfev_data_buffer *buffer;

	buffer = container_of(msg, struct ipc_dfev_data_buffer, msg);
	atomic_set(&buffer->in_use, 0);
}
EXPORT_SYMBOL(ipc_dfev_data_msg_put);

int ipc_dfev_send_tx(struct ipc_dfev_data_handle *handle, struct ipc_dfev_data_msg *msg)
{
	return ipc_dfev_send(IPC_DFEV_MSG_TYPE_TX, IPC_DFEV_DATA_TX_CHAN_ID, msg);
}
EXPORT_SYMBOL(ipc_dfev_send_tx);

int ipc_dfev_send_rx(struct ipc_dfev_data_handle *handle, struct ipc_dfev_data_msg *msg)
{
	return ipc_dfev_send(IPC_DFEV_MSG_TYPE_RX, IPC_DFEV_DATA_RX_CHAN_ID, msg);
}
EXPORT_SYMBOL(ipc_dfev_send_rx);

int ipc_dfev_send_tx_return(struct ipc_dfev_data_handle *handle, struct ipc_dfev_data_msg *msg)
{
	return ipc_dfev_send(IPC_DFEV_MSG_TYPE_TX, IPC_DFEV_DATA_RETURN_CHAN_ID, msg);
}
EXPORT_SYMBOL(ipc_dfev_send_tx_return);

int ipc_dfev_send_rx_return(struct ipc_dfev_data_handle *handle, struct ipc_dfev_data_msg *msg)
{
	return ipc_dfev_send(IPC_DFEV_MSG_TYPE_RX, IPC_DFEV_DATA_RETURN_CHAN_ID, msg);
}
EXPORT_SYMBOL(ipc_dfev_send_rx_return);

int ipc_dfev_data_poll_rx(struct ipc_dfev_data_handle *handle)
{
	return ipc_rx_msg(IPC_DFEV_LINK_ID, IPC_DFEV_DATA_RX_CHAN_ID);
}
EXPORT_SYMBOL(ipc_dfev_data_poll_rx);

int ipc_dfev_data_poll_tx(struct ipc_dfev_data_handle *handle)
{
	return ipc_rx_msg(IPC_DFEV_LINK_ID, IPC_DFEV_DATA_TX_CHAN_ID);
}
EXPORT_SYMBOL(ipc_dfev_data_poll_tx);

int ipc_dfev_data_poll_return(struct ipc_dfev_data_handle *handle)
{
	return ipc_rx_msg(IPC_DFEV_LINK_ID, IPC_DFEV_DATA_RETURN_CHAN_ID);
}
EXPORT_SYMBOL(ipc_dfev_data_poll_return);

static void ipc_dfev_data_handler(MV_IPC_MSG *msg)
{
	struct ipc_dfev_data_msg *message;

	message = ipc_phys_to_virt(IPC_DFEV_LINK_ID, msg->ptr);

	switch (msg->type) {
	case IPC_DFEV_MSG_TYPE_RX:
		if (ipc_dfev_data_ops && ipc_dfev_data_ops->ipc_dfev_rx_callback)
			ipc_dfev_data_ops->ipc_dfev_rx_callback(message);
		else
			ipc_dfev_data_msg_put(NULL, message);
		break;
	case IPC_DFEV_MSG_TYPE_TX:
		if (ipc_dfev_data_ops && ipc_dfev_data_ops->ipc_dfev_tx_callback)
			ipc_dfev_data_ops->ipc_dfev_tx_callback(message);
		else
			ipc_dfev_data_msg_put(NULL, message);
		break;
	default:
		pr_err("IPC DFEV: Unknown data message type %u!\n", msg->type);
	}
}

static int ipc_dfev_data_rx(MV_IPC_MSG *msg)
{
	ipc_dfev_data_handler(msg);
	ipc_release_msg(IPC_DFEV_LINK_ID, IPC_DFEV_DATA_RX_CHAN_ID, msg);
	return 0;
}

static int ipc_dfev_data_tx(MV_IPC_MSG *msg)
{
	ipc_dfev_data_handler(msg);
	ipc_release_msg(IPC_DFEV_LINK_ID, IPC_DFEV_DATA_TX_CHAN_ID, msg);
	return 0;
}

static int ipc_dfev_data_return(MV_IPC_MSG *msg)
{
	ipc_dfev_data_handler(msg);
	ipc_release_msg(IPC_DFEV_LINK_ID, IPC_DFEV_DATA_RETURN_CHAN_ID, msg);
	return 0;
}

static int __init ipc_dfev_init_module(void)
{
	int error;
	int link;

	if (MV_TDM_UNIT_DFEV != mvCtrlTdmUnitTypeGet())
		return 0;

	/* start link after IPC and dragonite modules are initialized, to make sure the dragonite DTCM accessable */
	for (link = 0; link < mvIpcGetNumOfLinks(); link++) {
		error = mvIpcLinkStart(link);
		if (error != MV_OK) {
			printk(KERN_ERR "IPC: IPC HAL %d initialization failed\n", 0);
			return error;
		}
	}

	/* Open Command Channel */
	error = ipc_dfev_open_channel(IPC_DFEV_LINK_ID, IPC_DFEV_CTRL_CHAN_ID, ipc_dfev_ctrl_msg_handler);
	if (error)
		goto error0;

	/* Open Data Channels */
	error = ipc_dfev_open_channel(IPC_DFEV_LINK_ID, IPC_DFEV_DATA_RX_CHAN_ID, ipc_dfev_data_rx);
	if (error)
		goto error1;

	error = ipc_dfev_open_channel(IPC_DFEV_LINK_ID, IPC_DFEV_DATA_TX_CHAN_ID, ipc_dfev_data_tx);
	if (error)
		goto error2;

	error = ipc_dfev_open_channel(IPC_DFEV_LINK_ID, IPC_DFEV_DATA_RETURN_CHAN_ID, ipc_dfev_data_return);
	if (error)
		goto error3;

	/* Allocate shared memory for command channel */
	ipc_dfev_init_pool(&ipc_dfev_ctrl_pool, &error);
	if (error) {
		pr_err("IPC DFEV: Failed to allocate shared memory for IPC link %u\n", IPC_DFEV_LINK_ID);
		goto error4;
	}

	/* Allocate shared memory for data channel */
	ipc_dfev_init_pool(&ipc_dfev_data_pool, &error);
	if (error) {
		pr_err("IPC DFEV: Failed to allocate shared memory for IPC link %u!\n", IPC_DFEV_LINK_ID);
		goto error4;
	}

	INIT_DELAYED_WORK(&ipc_dfev_linkup, ipc_dfev_handshake);
	schedule_delayed_work(&ipc_dfev_linkup, HZ);

	return 0;

error4:
	ipc_close_chn(IPC_DFEV_LINK_ID, IPC_DFEV_DATA_RETURN_CHAN_ID);
error3:
	ipc_close_chn(IPC_DFEV_LINK_ID, IPC_DFEV_DATA_TX_CHAN_ID);
error2:
	ipc_close_chn(IPC_DFEV_LINK_ID, IPC_DFEV_DATA_RX_CHAN_ID);
error1:
	ipc_close_chn(IPC_DFEV_LINK_ID, IPC_DFEV_CTRL_CHAN_ID);
error0:
	return error;
}

static void __exit ipc_dfev_cleanup_module(void)
{
}

module_init(ipc_dfev_init_module);
module_exit(ipc_dfev_cleanup_module);
MODULE_DESCRIPTION("Marvell Inter-Processor DFEV Communication Driver");
MODULE_AUTHOR("Piotr Ziecik <kosmo@semihalf.com>");
MODULE_LICENSE("GPL");
