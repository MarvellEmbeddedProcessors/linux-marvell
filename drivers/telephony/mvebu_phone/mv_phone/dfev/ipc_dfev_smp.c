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
#include <linux/interrupt.h>
#include <linux/slab.h>
#include <plat/ipc_dfev.h>

#define IPC_DFEV_FIFO_SIZE		16	/* must be power of 2 */

#define IPC_DFEV_MSG_TYPE_COMMAND	0
#define IPC_DFEV_MSG_TYPE_EVENT		1
#define IPC_DFEV_MSG_TYPE_RX		2
#define IPC_DFEV_MSG_TYPE_TX		3
#define IPC_DFEV_MSG_TYPE_PRINT		4

struct ipc_dfev_channel {
	atomic_t	read_idx;
	atomic_t	write_idx;
	void		*fifo[IPC_DFEV_FIFO_SIZE];
};

static struct ipc_dfev_channel ctrl_dfev2host;
static struct ipc_dfev_channel ctrl_host2dfev;

static struct ipc_dfev_channel rx_dfev2host;
static struct ipc_dfev_channel rx_host2dfev;

static struct ipc_dfev_channel tx_dfev2host;
static struct ipc_dfev_channel tx_host2dfev;

static struct ipc_dfev_channel return_dfev2host;
static struct ipc_dfev_channel return_host2dfev;

static struct tasklet_struct ipc_dfev_tasklet;

struct ipc_dfev_ctrl_buffer {
	int				type;
	struct ipc_dfev_ctrl_msg	msg;
};

struct ipc_dfev_ctrl_handle {
	struct ipc_dfev_channel		*ctrl_rx;
	struct ipc_dfev_channel		*ctrl_tx;

	struct ipc_dfev_ctrl_ops	*ctrl_ops;
	struct tasklet_struct		*tasklet;
};

static struct ipc_dfev_ctrl_handle ipc_dfev_ctrl_handle_dfev = {
	.ctrl_rx	= &ctrl_host2dfev,
	.ctrl_tx	= &ctrl_dfev2host,

	.tasklet	= &ipc_dfev_tasklet,
};

static struct ipc_dfev_ctrl_handle ipc_dfev_ctrl_handle_host = {
	.ctrl_rx	= &ctrl_dfev2host,
	.ctrl_tx	= &ctrl_host2dfev,

	.tasklet	= NULL,
};

struct ipc_dfev_data_buffer {
	int				type;
	struct ipc_dfev_data_msg	msg;
};

struct ipc_dfev_data_handle {
	struct ipc_dfev_channel		*rx_rx;
	struct ipc_dfev_channel		*rx_tx;

	struct ipc_dfev_channel		*tx_rx;
	struct ipc_dfev_channel		*tx_tx;

	struct ipc_dfev_channel		*return_rx;
	struct ipc_dfev_channel		*return_tx;

	struct ipc_dfev_data_ops	*data_ops;
	struct tasklet_struct		*tasklet;
};

static struct ipc_dfev_data_handle ipc_dfev_data_handle_dfev = {
	.rx_rx		= &rx_host2dfev,
	.rx_tx		= &rx_dfev2host,

	.tx_rx		= &tx_host2dfev,
	.tx_tx		= &tx_dfev2host,

	.return_rx	= &return_host2dfev,
	.return_tx	= &return_dfev2host,

	.tasklet	= &ipc_dfev_tasklet,
};

static struct ipc_dfev_data_handle ipc_dfev_data_handle_host = {
	.rx_rx		= &rx_dfev2host,
	.rx_tx		= &rx_host2dfev,

	.tx_rx		= &tx_dfev2host,
	.tx_tx		= &tx_host2dfev,

	.return_rx	= &return_dfev2host,
	.return_tx	= &return_host2dfev,

	.tasklet	= NULL,
};

static int ipc_dfev_channel_is_empty(struct ipc_dfev_channel *ch)
{
	BUG_ON(ch == NULL);
	return ((atomic_read(&ch->write_idx) - atomic_read(&ch->read_idx)) == 0);
}

static int ipc_dfev_channel_is_full(struct ipc_dfev_channel *ch)
{
	BUG_ON(ch == NULL);
	return ((atomic_read(&ch->write_idx) - atomic_read(&ch->read_idx)) >=
						IPC_DFEV_FIFO_SIZE);
}

static int ipc_dfev_msg_put(struct ipc_dfev_channel *ch, void *msg)
{
	unsigned int idx;

	if (ipc_dfev_channel_is_full(ch))
		return -ENOMEM;

	idx = atomic_read(&ch->write_idx) & (IPC_DFEV_FIFO_SIZE - 1);
	ch->fifo[idx] = msg;
	atomic_inc(&ch->write_idx);

	return 0;
}

static void *ipc_dfev_msg_get(struct ipc_dfev_channel *ch)
{
	unsigned int idx;
	void *msg;

	if (ipc_dfev_channel_is_empty(ch))
		return NULL;

	idx = atomic_read(&ch->read_idx) & (IPC_DFEV_FIFO_SIZE - 1);
	msg = ch->fifo[idx];
	atomic_inc(&ch->read_idx);

	return msg;
}

static void ipc_dfev_tasklet_func(unsigned long data)
{
	int again;

	do {
		again = 0;

		if (ipc_dfev_ctrl_handle_host.ctrl_ops != NULL)
			if (ipc_dfev_ctrl_poll(&ipc_dfev_ctrl_handle_host) == 0)
				again = 1;

		if (ipc_dfev_data_handle_host.data_ops != NULL) {
			if (ipc_dfev_data_poll_rx(&ipc_dfev_data_handle_host) == 0)
				again = 1;

			if (ipc_dfev_data_poll_tx(&ipc_dfev_data_handle_host) == 0)
				again = 1;

			if (ipc_dfev_data_poll_return(&ipc_dfev_data_handle_host) == 0)
				again = 1;
		}
	} while (again);
}

/*
 * Control Path API
 */
struct ipc_dfev_ctrl_handle *ipc_dfev_ctrl_init(enum ipc_dfev_mode mode,
					struct ipc_dfev_ctrl_ops *ctrl_ops)
{
	switch (mode) {
	case IPC_DFEV_MODE_INTERRUPT:
		/* Host side uses INTERRUPT mode */
		ipc_dfev_ctrl_handle_host.ctrl_ops = ctrl_ops;
		return &ipc_dfev_ctrl_handle_host;
	case IPC_DFEV_MODE_POLLING:
		/* DFEV side uses POLLING mode */
		ipc_dfev_ctrl_handle_dfev.ctrl_ops = ctrl_ops;
		return &ipc_dfev_ctrl_handle_dfev;
	default:
		return NULL;
	}
}
EXPORT_SYMBOL(ipc_dfev_ctrl_init);

void ipc_dfev_ctrl_exit(struct ipc_dfev_ctrl_handle *handle)
{
	handle->ctrl_ops = NULL;
}
EXPORT_SYMBOL(ipc_dfev_ctrl_exit);

int ipc_dfev_ctrl_poll(struct ipc_dfev_ctrl_handle *handle)
{
	struct ipc_dfev_ctrl_buffer *buffer;
	void *msg;

	msg = ipc_dfev_msg_get(handle->ctrl_rx);
	if (!msg)
		return -ENOMSG;

	buffer = container_of(msg, struct ipc_dfev_ctrl_buffer, msg);
	switch (buffer->type) {
	case IPC_DFEV_MSG_TYPE_COMMAND:
		if (handle->ctrl_ops && handle->ctrl_ops->ipc_dfev_command_callback)
			handle->ctrl_ops->ipc_dfev_command_callback(msg);
		else
			ipc_dfev_ctrl_msg_put(handle, msg);
		break;
	case IPC_DFEV_MSG_TYPE_EVENT:
		if (handle->ctrl_ops && handle->ctrl_ops->ipc_dfev_event_callback)
			handle->ctrl_ops->ipc_dfev_event_callback(msg);
		else
			ipc_dfev_ctrl_msg_put(handle, msg);
		break;
	default:
		pr_err("IPC DFEV: Unknown ctrl message type %u!\n", buffer->type);
	}

	return 0;
}
EXPORT_SYMBOL(ipc_dfev_ctrl_poll);

struct ipc_dfev_ctrl_msg *ipc_dfev_ctrl_msg_get(struct ipc_dfev_ctrl_handle *handle)
{
	struct ipc_dfev_ctrl_buffer *buffer;

	buffer = kmalloc(sizeof(struct ipc_dfev_ctrl_buffer), GFP_ATOMIC);
	if (!buffer)
		return NULL;

	return &buffer->msg;
}
EXPORT_SYMBOL(ipc_dfev_ctrl_msg_get);

void ipc_dfev_ctrl_msg_put(struct ipc_dfev_ctrl_handle *handle, struct ipc_dfev_ctrl_msg *msg)
{
	kfree(container_of(msg, struct ipc_dfev_ctrl_buffer, msg));
}
EXPORT_SYMBOL(ipc_dfev_ctrl_msg_put);

int ipc_dfev_send_command(struct ipc_dfev_ctrl_handle *handle, struct ipc_dfev_ctrl_msg *msg)
{
	struct ipc_dfev_ctrl_buffer *buffer;
	int retval;

	buffer = container_of(msg, struct ipc_dfev_ctrl_buffer, msg);
	buffer->type = IPC_DFEV_MSG_TYPE_COMMAND;

	retval = ipc_dfev_msg_put(handle->ctrl_tx, msg);

	if (handle->tasklet)
		tasklet_schedule(handle->tasklet);

	return retval;
}
EXPORT_SYMBOL(ipc_dfev_send_command);

int ipc_dfev_send_event(struct ipc_dfev_ctrl_handle *handle, struct ipc_dfev_ctrl_msg *msg)
{
	struct ipc_dfev_ctrl_buffer *buffer;
	int retval;

	buffer = container_of(msg, struct ipc_dfev_ctrl_buffer, msg);
	buffer->type = IPC_DFEV_MSG_TYPE_EVENT;

	retval = ipc_dfev_msg_put(handle->ctrl_tx, msg);

	if (handle->tasklet)
		tasklet_schedule(handle->tasklet);

	return retval;

}
EXPORT_SYMBOL(ipc_dfev_send_event);

/*
 * Data Path API
 */
struct ipc_dfev_data_handle *ipc_dfev_data_init(enum ipc_dfev_mode mode,
					struct ipc_dfev_data_ops *data_ops)
{
	switch (mode) {
	case IPC_DFEV_MODE_INTERRUPT:
		/* Host side uses INTERRUPT mode */
		ipc_dfev_data_handle_host.data_ops = data_ops;
		return &ipc_dfev_data_handle_host;
	case IPC_DFEV_MODE_POLLING:
		/* DFEV side uses POLLING mode */
		ipc_dfev_data_handle_dfev.data_ops = data_ops;
		return &ipc_dfev_data_handle_dfev;
	default:
		return NULL;
	}
}
EXPORT_SYMBOL(ipc_dfev_data_init);

void ipc_dfev_data_exit(struct ipc_dfev_data_handle *handle)
{
	handle->data_ops = NULL;
}
EXPORT_SYMBOL(ipc_dfev_data_exit);

struct ipc_dfev_data_msg *ipc_dfev_data_msg_get(struct ipc_dfev_data_handle *handle)
{
	struct ipc_dfev_data_buffer *buffer;

	buffer = kmalloc(sizeof(struct ipc_dfev_data_buffer), GFP_ATOMIC);
	if (!buffer)
		return NULL;

	return &buffer->msg;
}
EXPORT_SYMBOL(ipc_dfev_data_msg_get);

void ipc_dfev_data_msg_put(struct ipc_dfev_data_handle *handle, struct ipc_dfev_data_msg *msg)
{
	kfree(container_of(msg, struct ipc_dfev_data_buffer, msg));
}
EXPORT_SYMBOL(ipc_dfev_data_msg_put);

int ipc_dfev_send_tx(struct ipc_dfev_data_handle *handle, struct ipc_dfev_data_msg *msg)
{
	struct ipc_dfev_data_buffer *buffer;
	int retval;

	buffer = container_of(msg, struct ipc_dfev_data_buffer, msg);
	buffer->type = IPC_DFEV_MSG_TYPE_TX;

	retval = ipc_dfev_msg_put(handle->tx_tx, msg);

	if (handle->tasklet)
		tasklet_schedule(handle->tasklet);

	return retval;

}
EXPORT_SYMBOL(ipc_dfev_send_tx);

int ipc_dfev_send_rx(struct ipc_dfev_data_handle *handle, struct ipc_dfev_data_msg *msg)
{
	struct ipc_dfev_data_buffer *buffer;
	int retval;

	buffer = container_of(msg, struct ipc_dfev_data_buffer, msg);
	buffer->type = IPC_DFEV_MSG_TYPE_RX;

	retval = ipc_dfev_msg_put(handle->rx_tx, msg);

	if (handle->tasklet)
		tasklet_schedule(handle->tasklet);

	return retval;
}
EXPORT_SYMBOL(ipc_dfev_send_rx);

int ipc_dfev_send_tx_return(struct ipc_dfev_data_handle *handle, struct ipc_dfev_data_msg *msg)
{
	struct ipc_dfev_data_buffer *buffer;
	int retval;

	buffer = container_of(msg, struct ipc_dfev_data_buffer, msg);
	buffer->type = IPC_DFEV_MSG_TYPE_TX;

	retval = ipc_dfev_msg_put(handle->return_tx, msg);

	if (handle->tasklet)
		tasklet_schedule(handle->tasklet);

	return retval;
}
EXPORT_SYMBOL(ipc_dfev_send_tx_return);

int ipc_dfev_send_rx_return(struct ipc_dfev_data_handle *handle, struct ipc_dfev_data_msg *msg)
{
	struct ipc_dfev_data_buffer *buffer;
	int retval;

	buffer = container_of(msg, struct ipc_dfev_data_buffer, msg);
	buffer->type = IPC_DFEV_MSG_TYPE_RX;

	retval = ipc_dfev_msg_put(handle->return_tx, msg);

	if (handle->tasklet)
		tasklet_schedule(handle->tasklet);

	return retval;
}
EXPORT_SYMBOL(ipc_dfev_send_rx_return);

static void ipc_dfev_data_handle_msg(struct ipc_dfev_data_handle *handle,
						struct ipc_dfev_data_msg *msg)
{
	struct ipc_dfev_data_buffer *buffer;

	buffer = container_of(msg, struct ipc_dfev_data_buffer, msg);
	switch (buffer->type) {
	case IPC_DFEV_MSG_TYPE_RX:
		if (handle->data_ops && handle->data_ops->ipc_dfev_rx_callback)
			handle->data_ops->ipc_dfev_rx_callback(msg);
		else
			ipc_dfev_data_msg_put(handle, msg);
		break;
	case IPC_DFEV_MSG_TYPE_TX:
		if (handle->data_ops && handle->data_ops->ipc_dfev_tx_callback)
			handle->data_ops->ipc_dfev_tx_callback(msg);
		else
			ipc_dfev_data_msg_put(handle, msg);
		break;
	default:
		pr_err("IPC DFEV: Unknown data message type %u!\n", buffer->type);
	}
}

int ipc_dfev_data_poll_rx(struct ipc_dfev_data_handle *handle)
{
	void *msg;

	msg = ipc_dfev_msg_get(handle->rx_rx);
	if (!msg)
		return -ENOMSG;

	ipc_dfev_data_handle_msg(handle, msg);

	return 0;
}
EXPORT_SYMBOL(ipc_dfev_data_poll_rx);

int ipc_dfev_data_poll_tx(struct ipc_dfev_data_handle *handle)
{
	void *msg;

	msg = ipc_dfev_msg_get(handle->tx_rx);
	if (!msg)
		return -ENOMSG;

	ipc_dfev_data_handle_msg(handle, msg);

	return 0;
}
EXPORT_SYMBOL(ipc_dfev_data_poll_tx);

int ipc_dfev_data_poll_return(struct ipc_dfev_data_handle *handle)
{
	void *msg;

	msg = ipc_dfev_msg_get(handle->return_rx);
	if (!msg)
		return -ENOMSG;

	ipc_dfev_data_handle_msg(handle, msg);

	return 0;
}
EXPORT_SYMBOL(ipc_dfev_data_poll_return);

static int __init ipc_dfev_init_module(void)
{
	tasklet_init(&ipc_dfev_tasklet, ipc_dfev_tasklet_func, 0);

	return 0;
}

static void __exit ipc_dfev_cleanup_module(void)
{
	tasklet_kill(&ipc_dfev_tasklet);
}

module_init(ipc_dfev_init_module);
module_exit(ipc_dfev_cleanup_module);
MODULE_DESCRIPTION("Marvell Inter-Processor DFEV Communication Driver");
MODULE_AUTHOR("Piotr Ziecik <kosmo@semihalf.com>");
MODULE_LICENSE("GPL");
