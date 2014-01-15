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


static ssize_t l2fw_help(char *buf)
{
	int off = 0;

	off += sprintf(buf+off, "cat rules_dump                - display L2fw rules DB\n");
	off += sprintf(buf+off, "cat ports_dump                - display L2fw ports DB\n");
	off += sprintf(buf+off, "cat stats                     - show debug information\n");

	/* inputs in decimal */
	off += sprintf(buf+off, "echo rxp txp mode > l2fw      - set L2FW mode: 0-dis,1-as_is,2-swap,3-copy,4-ipsec\n");
#ifdef CONFIG_MV_INCLUDE_XOR
	off += sprintf(buf+off, "echo rxp thresh   > l2fw_xor  - set XOR threshold in bytes for port <rxp>\n");
	#endif
	off += sprintf(buf+off, "echo rxp en       > lookup    - enable/disable hash lookup for <rxp>\n");
	off += sprintf(buf+off, "echo 1            > flush     - flush L2fw rules DB\n");

	/* inputs in hex */
	off += sprintf(buf+off, "echo sip dip txp  > rule_add  - set rule for SIP and DIP pair. [x.x.x.x]\n");

#ifdef CONFIG_MV_ETH_L2SEC
	off += sprintf(buf+off, "echo p chan       > cesa_chan - set cesa channel <chan> for port <p>.\n");
#endif

	return off;
}

static ssize_t l2fw_show(struct device *dev,
				  struct device_attribute *attr, char *buf)
{
    const char	*name = attr->attr.name;
    int             off = 0;

    if (!capable(CAP_NET_ADMIN))
	return -EPERM;

	if (!strcmp(name, "help")) {
	    off = l2fw_help(buf);
		return off;
	} else if (!strcmp(name, "rules_dump")) {
		l2fw_rules_dump();
		return off;
	} else if (!strcmp(name, "ports_dump")) {
		l2fw_ports_dump();
		return off;
	} else if (!strcmp(name, "stats")) {
		l2fw_stats();
		return off;
	}

	return off;
}



static ssize_t l2fw_hex_store(struct device *dev, struct device_attribute *attr,
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
		l2fw_flush();
	} else {
		err = 1;
		printk(KERN_ERR "%s: illegal operation <%s>\n", __func__, attr->attr.name);
	}

	local_irq_restore(flags);

	return err ? -EINVAL : len;
}

static ssize_t l2fw_ip_store(struct device *dev,
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

	if (!strcmp(name, "l2fw_add_ip"))
		l2fw_add(srcIp, dstIp, port);
	else {
		err = 1;
		printk(KERN_ERR "%s: illegal operation <%s>\n", __func__, attr->attr.name);
	}

	local_irq_restore(flags);

	if (err)
		printk(KERN_ERR "%s: <%s>, error %d\n", __func__, attr->attr.name, err);

	return err ? -EINVAL : len;
}



static ssize_t l2fw_store(struct device *dev,
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
		l2fw_lookupEn(a, b);
	else if (!strcmp(name, "l2fw"))
		l2fw(c, a, b);
#ifdef CONFIG_MV_INCLUDE_XOR
	else if (!strcmp(name, "l2fw_xor"))
		l2fw_xor(a, b);
#endif
#ifdef CONFIG_MV_ETH_L2SEC
	else if (!strcmp(name, "cesa_chan"))
		err = mv_l2sec_set_cesa_chan(a, b);
#endif
	local_irq_restore(flags);

	if (err)
		mvOsPrintf("%s: error %d\n", __func__, err);

	return err ? -EINVAL : len;

}


static DEVICE_ATTR(l2fw,		S_IWUSR, l2fw_show, l2fw_store);
#ifdef CONFIG_MV_INCLUDE_XOR
static DEVICE_ATTR(l2fw_xor,		S_IWUSR, l2fw_show, l2fw_store);
#endif
static DEVICE_ATTR(lookup,		S_IWUSR, l2fw_show, l2fw_store);
static DEVICE_ATTR(l2fw_add_ip,		S_IWUSR, l2fw_show, l2fw_ip_store);
static DEVICE_ATTR(help,		S_IRUSR, l2fw_show,  NULL);
static DEVICE_ATTR(rules_dump,		S_IRUSR, l2fw_show,  NULL);
static DEVICE_ATTR(ports_dump,		S_IRUSR, l2fw_show,  NULL);
static DEVICE_ATTR(flush,		S_IWUSR, NULL,	l2fw_hex_store);
static DEVICE_ATTR(stats,		S_IRUSR, l2fw_show, NULL);

#ifdef CONFIG_MV_ETH_L2SEC
static DEVICE_ATTR(cesa_chan,		S_IWUSR, NULL,  l2fw_store);
#endif



static struct attribute *l2fw_attrs[] = {
	&dev_attr_l2fw.attr,
#ifdef CONFIG_MV_INCLUDE_XOR
	&dev_attr_l2fw_xor.attr,
#endif
	&dev_attr_lookup.attr,
	&dev_attr_l2fw_add_ip.attr,
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

static struct attribute_group l2fw_group = {
	.name = "l2fw",
	.attrs = l2fw_attrs,
};

int mv_neta_l2fw_sysfs_init(struct kobject *neta_kobj)
{
	int err = 0;

	err = sysfs_create_group(neta_kobj, &l2fw_group);
	if (err)
		printk(KERN_INFO "sysfs group %s failed %d\n", l2fw_group.name, err);

	return err;
}

int mv_neta_l2fw_sysfs_exit(struct kobject *neta_kobj)
{
	sysfs_remove_group(neta_kobj, &l2fw_group);

	return 0;
}
