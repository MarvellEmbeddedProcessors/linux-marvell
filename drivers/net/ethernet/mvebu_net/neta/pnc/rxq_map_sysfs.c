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
*******************************************************************************/
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/types.h>
#include <linux/capability.h>
#include <linux/kernel.h>
#include <linux/platform_device.h>

#include "mvOs.h"
#include "mvCommon.h"
#include "ctrlEnv/mvCtrlEnvLib.h"

#include "gbe/mvNeta.h"
#include "pnc/mvPnc.h"

#include "net_dev/mv_netdev.h"


static ssize_t rxq_map_help(char *buf)
{
	int off = 0;

	off += sprintf(buf+off, "cat                                   dump_all  - dump all rxq mapping rules\n");
	off += sprintf(buf+off, "echo port sip dip rxq               > ip4_rxq   - add new mapping rule from <sip> <dip> to <rxq> via <port>\n");
	off += sprintf(buf+off, "echo port sip dip                   > ip4_drop  - add new mapping rule from <sip> <dip> to drop via <port>\n");
	off += sprintf(buf+off, "echo port sip dip                   > ip4_del   - delete existing rule with <sip> <dip>\n");

	off += sprintf(buf+off, "echo port sip dip sport dport rxq   > udp4_rxq  - add new mapping rule from 5-tuple to <rxq> via <port>\n");
	off += sprintf(buf+off, "echo port sip dip sport dport       > udp4_drop - add new mapping rule from 5-tuple to drop via <port>\n");
	off += sprintf(buf+off, "echo port sip dip sport dport       > udp4_del  - delete existing rule\n");

	off += sprintf(buf+off, "echo port sip dip sport dport rxq   > tcp4_rxq  - add new mapping rule from 5-tuple to <rxq> via <port>\n");
	off += sprintf(buf+off, "echo port sip dip sport dport       > tcp4_drop - add new mapping rule from 5-tuple to drop via <port>\n");
	off += sprintf(buf+off, "echo port sip dip sport dport       > tcp4_del  - delete existing rule\n");

	off += sprintf(buf+off, "\nparameters: sip/dip = xxx.xxx.xxx.xxx\n");

	return off;
}

static ssize_t rxq_map_show(struct device *dev,
				  struct device_attribute *attr, char *buf)
{
	const char   *name = attr->attr.name;

	if (!capable(CAP_NET_ADMIN))
		return -EPERM;

	if (!strcmp(name, "dump_all"))
		pnc_rxq_map_dump();
	else if (!strcmp(name, "help"))
		return rxq_map_help(buf);

	return 0;
}


static ssize_t rxq_map_2t_store(struct device *dev,
			 struct device_attribute *attr, const char *buf, size_t len)
{
	unsigned int res = 0, err = 0, sip, dip, port;
	int rxq;
	const char *name = attr->attr.name;

	if (!capable(CAP_NET_ADMIN))
		return -EPERM;

	res = sscanf(buf, "%d %hhu.%hhu.%hhu.%hhu %hhu.%hhu.%hhu.%hhu %d", &port, (unsigned char *)(&sip), (unsigned char *)(&sip) + 1,
		(unsigned char *)(&sip) + 2, (unsigned char *)(&sip) + 3, (unsigned char *)(&dip), (unsigned char *)(&dip) + 1,
		(unsigned char *)(&dip) + 2, (unsigned char *)(&dip) + 3, &rxq);
	if (res < 9)
		return -EINVAL;

	if (!strcmp(name, "ip4_drop"))
		rxq = -1;
	else if (!strcmp(name, "ip4_del"))
		rxq = -2;

	err = pnc_ip4_2tuple_rxq(port, sip, dip, rxq);

	return err ? -EINVAL : len;
}

static ssize_t rxq_map_5t_store(struct device *dev,
			 struct device_attribute *attr, const char *buf, size_t len)
{
	unsigned int res = 0, err = 0, sip, dip, ports, proto, port;
	int rxq;
	const char *name = attr->attr.name;

	if (!capable(CAP_NET_ADMIN))
		return -EPERM;

	res = sscanf(buf, "%d %hhu.%hhu.%hhu.%hhu %hhu.%hhu.%hhu.%hhu %hu %hu %d", &port, (unsigned char *)(&sip),
		(unsigned char *)(&sip) + 1, (unsigned char *)(&sip) + 2, (unsigned char *)(&sip) + 3, (unsigned char *)(&dip),
		(unsigned char *)(&dip) + 1, (unsigned char *)(&dip) + 2, (unsigned char *)(&dip) + 3,
		(u16 *)&ports + 1, (u16 *)&ports, &rxq);
	if (res < 11)
		return -EINVAL;

	if (!strcmp(name, "udp4_drop") || !strcmp(name, "tcp4_drop"))
		rxq = -1;
	else if (!strcmp(name, "udp4_del") || !strcmp(name, "tcp4_del"))
		rxq = -2;

	if (name[0] == 't')
		proto = 6; /* tcp */
	else
		proto = 17; /* udp */

	err = pnc_ip4_5tuple_rxq(port, sip, dip, MV_BYTE_SWAP_32BIT(ports), proto, rxq);

	return err ? -EINVAL : len;
}


static DEVICE_ATTR(help,		S_IRUSR, rxq_map_show, rxq_map_2t_store);
static DEVICE_ATTR(dump_all,		S_IRUSR, rxq_map_show, rxq_map_2t_store);
//static DEVICE_ATTR(dump,        S_IWUSR, rxq_map_show, rxq_map_2t_store);
static DEVICE_ATTR(ip4_rxq,	S_IWUSR, rxq_map_show, rxq_map_2t_store);
static DEVICE_ATTR(ip4_drop,	S_IWUSR, rxq_map_show, rxq_map_2t_store);
static DEVICE_ATTR(ip4_del,	S_IWUSR, rxq_map_show, rxq_map_2t_store);
static DEVICE_ATTR(udp4_rxq,	S_IWUSR, rxq_map_show, rxq_map_5t_store);
static DEVICE_ATTR(udp4_drop,	S_IWUSR, rxq_map_show, rxq_map_5t_store);
static DEVICE_ATTR(udp4_del,	S_IWUSR, rxq_map_show, rxq_map_5t_store);
static DEVICE_ATTR(tcp4_rxq,	S_IWUSR, rxq_map_show, rxq_map_5t_store);
static DEVICE_ATTR(tcp4_drop,	S_IWUSR, rxq_map_show, rxq_map_5t_store);
static DEVICE_ATTR(tcp4_del,	S_IWUSR, rxq_map_show, rxq_map_5t_store);

static struct attribute *rxq_map_attrs[] = {
	&dev_attr_help.attr,
	&dev_attr_dump_all.attr,
	&dev_attr_ip4_rxq.attr,
	&dev_attr_ip4_drop.attr,
	&dev_attr_ip4_del.attr,
	&dev_attr_udp4_rxq.attr,
	&dev_attr_udp4_drop.attr,
	&dev_attr_udp4_del.attr,
	&dev_attr_tcp4_rxq.attr,
	&dev_attr_tcp4_drop.attr,
	&dev_attr_tcp4_del.attr,
	NULL
};

static struct attribute_group rxq_map_group = {
	.name = "rxq_map",
	.attrs = rxq_map_attrs,
};

int __devinit rxq_map_sysfs_init(struct kobject *kobj)
{
	int err;

	err = sysfs_create_group(kobj, &rxq_map_group);
	if (err) {
		printk(KERN_INFO "sysfs group failed %d\n", err);
		goto out;
	}
out:
	return err;
}



MODULE_AUTHOR("Yoni Farhadian");
MODULE_DESCRIPTION("PNC rule to rxq map for Marvell NetA");
MODULE_LICENSE("GPL");
