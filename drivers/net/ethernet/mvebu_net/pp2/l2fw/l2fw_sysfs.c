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

#include "mvTypes.h"
#include "mv_eth_l2fw.h"
#ifdef CONFIG_MV_ETH_L2SEC
#include "mv_eth_l2sec.h"
#endif
#include "linux/inet.h"


static ssize_t mv_l2fw_help(char *buf)
{
	int off = 0;

	off += sprintf(buf+off, "cat               rules_dump - Display L2FW rules DB\n");
	off += sprintf(buf+off, "cat               ports_dump - Display L2FW ports DB\n");
	off += sprintf(buf+off, "cat               stats      - Show debug information\n");
	off += sprintf(buf+off, "\n");
	off += sprintf(buf+off, "echo p [1|0]      > l2fw     - Enable/Disable L2FW for port <p>\n");
	off += sprintf(buf+off, "echo rxp txp mode > bind     - Set <rxp-->txp>, mode: 0-as_is, 1-swap, 2-copy\n");
	off += sprintf(buf+off, "echo rxp thresh   > xor      - Set XOR threshold for port <rxp>\n");
	off += sprintf(buf+off, "echo rxp [1|0]    > lookup   - Enable/Disable L3 lookup for port <rxp>\n");
	off += sprintf(buf+off, "echo 1            > flush    - Flush L2FW rules DB\n");
	off += sprintf(buf+off, "echo sip dip txp  > add_ip   - Set L3 lookup rule, sip, dip in a.b.c.d format\n");
#ifdef CONFIG_MV_ETH_L2SEC
	off += sprintf(buf+off, "echo p chan       > cesa     - Set cesa channel <chan> for port <p>.\n");
#endif
	return off;
}

static ssize_t mv_l2fw_show(struct device *dev,
				  struct device_attribute *attr, char *buf)
{
	int off = 0;
	const char *name = attr->attr.name;

	if (!capable(CAP_NET_ADMIN))
		return -EPERM;

	if (!strcmp(name, "help"))
		off = mv_l2fw_help(buf);

	else if (!strcmp(name, "rules_dump"))
		mv_l2fw_rules_dump();

	else if (!strcmp(name, "ports_dump"))
		mv_l2fw_ports_dump();

	else if (!strcmp(name, "stats"))
		mv_l2fw_stats();

	return off;
}



static ssize_t mv_l2fw_hex_store(struct device *dev, struct device_attribute *attr,
				const char *buf, size_t len)
{
	const char      *name = attr->attr.name;
	int             err;
	unsigned int    addr1, addr2;
	int port;
	unsigned long   flags;

	if (!capable(CAP_NET_ADMIN))
		return -EPERM;
	err = addr1 = addr2 = port = 0;

	local_irq_save(flags);

	if (!strcmp(name, "flush")) {
		mv_l2fw_flush();
	} else {
		err = 1;
		printk(KERN_ERR "%s: illegal operation <%s>\n", __func__, attr->attr.name);
	}

	local_irq_restore(flags);

	return err ? -EINVAL : len;
}

static ssize_t mv_l2fw_ip_store(struct device *dev,
			 struct device_attribute *attr, const char *buf, size_t len)
{
	const char *name = attr->attr.name;

	unsigned int err = 0;
	unsigned int srcIp = 0, dstIp = 0;
	unsigned char *sipArr = (unsigned char *)&srcIp;
	unsigned char *dipArr = (unsigned char *)&dstIp;
	int port;
	unsigned long flags;

	if (!capable(CAP_NET_ADMIN))
		return -EPERM;

	sscanf(buf, "%hhu.%hhu.%hhu.%hhu %hhu.%hhu.%hhu.%hhu %d",
		sipArr, sipArr+1, sipArr+2, sipArr+3,
		dipArr, dipArr+1, dipArr+2, dipArr+3, &port);

	printk(KERN_INFO "0x%x->0x%x in %s\n", srcIp, dstIp, __func__);
	local_irq_save(flags);

	if (!strcmp(name, "add_ip"))
		mv_l2fw_add(srcIp, dstIp, port);
	else {
		err = 1;
		printk(KERN_ERR "%s: illegal operation <%s>\n", __func__, attr->attr.name);
	}

	local_irq_restore(flags);

	if (err)
		printk(KERN_ERR "%s: <%s>, error %d\n", __func__, attr->attr.name, err);

	return err ? -EINVAL : len;
}



static ssize_t mv_l2fw_store(struct device *dev,
				   struct device_attribute *attr, const char *buf, size_t len)
{
	const char	*name = attr->attr.name;
	int             err;

	unsigned int    a, b, c;
	unsigned long   flags;

	if (!capable(CAP_NET_ADMIN))
		return -EPERM;

	err = a = b = c = 0;
	sscanf(buf, "%d %d %d", &a, &b, &c);

	local_irq_save(flags);
	if (!strcmp(name, "lookup"))
		mv_l2fw_lookupEn(a, b);
#ifdef CONFIG_MV_INCLUDE_XOR
	else if (!strcmp(name, "xor"))
		mv_l2fw_xor(a, b);
#endif
	else if (!strcmp(name, "l2fw"))
		err = mv_l2fw_set(a, b);

	else if (!strcmp(name, "bind"))
		err = mv_l2fw_port(a, b, c);

#ifdef CONFIG_MV_ETH_L2SEC
	else if (!strcmp(name, "cesa_chan"))
		err = mv_l2sec_set_cesa_chan(a, b);
#endif
	local_irq_restore(flags);

	if (err)
		mvOsPrintf("%s: error %d\n", __func__, err);

	return err ? -EINVAL : len;

}


static DEVICE_ATTR(l2fw,		S_IWUSR, mv_l2fw_show, mv_l2fw_store);
static DEVICE_ATTR(bind,		S_IWUSR, mv_l2fw_show, mv_l2fw_store);
static DEVICE_ATTR(lookup,		S_IWUSR, mv_l2fw_show, mv_l2fw_store);
static DEVICE_ATTR(add_ip,		S_IWUSR, mv_l2fw_show, mv_l2fw_ip_store);
static DEVICE_ATTR(help,		S_IRUSR, mv_l2fw_show, NULL);
static DEVICE_ATTR(rules_dump,		S_IRUSR, mv_l2fw_show, NULL);
static DEVICE_ATTR(ports_dump,		S_IRUSR, mv_l2fw_show, NULL);
static DEVICE_ATTR(stats,		S_IRUSR, mv_l2fw_show, NULL);
static DEVICE_ATTR(flush,		S_IWUSR, NULL,	mv_l2fw_hex_store);

#ifdef CONFIG_MV_ETH_L2SEC
static DEVICE_ATTR(cesa_chan,		S_IWUSR, NULL,  mv_l2fw_store);
#endif
#ifdef CONFIG_MV_INCLUDE_XOR
static DEVICE_ATTR(xor,		S_IWUSR, mv_l2fw_show, mv_l2fw_store);
#endif



static struct attribute *mv_l2fw_attrs[] = {
	&dev_attr_l2fw.attr,
	&dev_attr_bind.attr,
#ifdef CONFIG_MV_INCLUDE_XOR
	&dev_attr_xor.attr,
#endif
	&dev_attr_lookup.attr,
	&dev_attr_add_ip.attr,
	&dev_attr_help.attr,
	&dev_attr_rules_dump.attr,
	&dev_attr_ports_dump.attr,
	&dev_attr_flush.attr,
	&dev_attr_stats.attr,
#ifdef CONFIG_MV_ETH_L2SEC
	&dev_attr_cesa_chan.attr,
#endif
	NULL
};

static struct attribute_group mv_l2fw_group = {
	.name = "l2fw",
	.attrs = mv_l2fw_attrs,
};

int mv_pp2_l2fw_sysfs_init(struct kobject *pp2_kobj)
{
	int err = 0;

	err = sysfs_create_group(pp2_kobj, &mv_l2fw_group);
	if (err)
		printk(KERN_INFO "sysfs group %s failed %d\n", mv_l2fw_group.name, err);

	return err;
}

int mv_pp2_l2fw_sysfs_exit(struct kobject *pp2_kobj)
{
	sysfs_remove_group(pp2_kobj, &mv_l2fw_group);

	return 0;
}
