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

#include <linux/module.h>
#include <linux/netdevice.h>
#include "common/mv_sw_if.h"
#include "mv_pp3_msg.h"
#include "mv_pp3_msg_chan.h"

static ssize_t mv_channel_help(char *b)
{
	int o = 0;

	o += scnprintf(b+o, PAGE_SIZE-o, "\n");
	o += scnprintf(b+o, PAGE_SIZE-o, "echo [n] [f]     > create_chan  - Create host <-> fw channel\n");
	o += scnprintf(b+o, PAGE_SIZE-o, "echo [c]         > chan_show    - Show message channel status and statistics\n");
	o += scnprintf(b+o, PAGE_SIZE-o, "echo [c] [t] [s] > send_msg     - Send message [s] opcode [t] to channel [c]\n");
	o += scnprintf(b+o, PAGE_SIZE-o, "echo [c]         > rx_msg       - Receive all messages from channel [c]\n");
	o += scnprintf(b+o, PAGE_SIZE-o, "echo [rx] [tx]   > debug        - Enable/disable debug messages print out\n");
	o += scnprintf(b+o, PAGE_SIZE-o, "\n");
	o += scnprintf(b+o, PAGE_SIZE-o, "parameters:\n");
	o += scnprintf(b+o, PAGE_SIZE-o, "      [n] max number of messages in channel\n");
	o += scnprintf(b+o, PAGE_SIZE-o, "      [f] channel flags\n");
	o += scnprintf(b+o, PAGE_SIZE-o, "      [c] channel number\n");
	o += scnprintf(b+o, PAGE_SIZE-o, "      [t] message opcode\n");
	o += scnprintf(b+o, PAGE_SIZE-o, "      [s] string\n");
	o += scnprintf(b+o, PAGE_SIZE-o, "      [rx/tx] 0 - disable, 1 - enable\n");
	o += scnprintf(b+o, PAGE_SIZE-o, "\n");

	return o;
}

static ssize_t mv_channel_show(struct device *dev,
				struct device_attribute *attr, char *buf)
{
	int             off = 0;

	if (!capable(CAP_NET_ADMIN))
		return -EPERM;

	off = mv_channel_help(buf);

	return off;
}

static ssize_t mv_channel_store(struct device *dev,
				struct device_attribute *attr, const char *buf, size_t len)
{
	const char      *name = attr->attr.name;
	int             err, ret;
	unsigned int    c;
	unsigned int  type;
	char msg[256];
	unsigned char imsg[128];

	if (!capable(CAP_NET_ADMIN))
		return -EPERM;

	/* Read first 3 parameters */
	err = c = 0;
	ret = sscanf(buf, "%x %d %s", &c, &type, msg);

	if (!strcmp(name, "chan_show"))
		mv_pp3_channel_show(c);
	else if (!strcmp(name, "send_msg")) {
		str_to_hex(msg, strlen(msg), imsg, strlen(msg)/2);
		mv_pp3_msg_send(c, imsg, strlen(msg)/2, MV_PP3_F_CFH_MSG_ACK, (u16) type, 0, 1);
	} else if (!strcmp(name, "rx_msg"))
		mv_pp3_msg_receive(c);
	else if (!strcmp(name, "create_chan"))
		mv_pp3_chan_create_cmd(c, type);
	else if (!strcmp(name, "debug")) {
		bool rx_en, tx_en;

		rx_en = (c) ? true : false;
		tx_en = (type) ? true : false;
		mv_pp3_debug_message_print_en(rx_en, tx_en);
	} else {
		err = 1;
		pr_err("%s: illegal operation <%s>\n", __func__, attr->attr.name);
	}

	return err ? -EINVAL : len;
}

static DEVICE_ATTR(help, S_IRUSR, mv_channel_show, NULL);
static DEVICE_ATTR(create_chan, S_IWUSR, NULL, mv_channel_store);
static DEVICE_ATTR(chan_show, S_IWUSR, NULL, mv_channel_store);
static DEVICE_ATTR(send_msg, S_IWUSR, NULL, mv_channel_store);
static DEVICE_ATTR(rx_msg, S_IWUSR, NULL, mv_channel_store);
static DEVICE_ATTR(debug, S_IWUSR, NULL, mv_channel_store);


static struct attribute *mv_attrs[] = {
	&dev_attr_help.attr,
	&dev_attr_create_chan.attr,
	&dev_attr_chan_show.attr,
	&dev_attr_send_msg.attr,
	&dev_attr_rx_msg.attr,
	&dev_attr_debug.attr,
	NULL
};

static struct attribute_group mv_chan_group = {
	.name = "msg",
	.attrs = mv_attrs,
};

int mv_pp3_chan_sysfs_init(struct kobject *pp3_kobj)
{
	int err;

	err = sysfs_create_group(pp3_kobj, &mv_chan_group);
	if (err) {
		pr_err("sysfs group failed %d\n", err);
		return err;
	}

	return err;
}

int mv_pp3_chan_sysfs_exit(struct kobject *hmac_kobj)
{
	sysfs_remove_group(hmac_kobj, &mv_chan_group);
	return 0;
}
