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
#include <linux/interrupt.h>

#include "mvCommon.h"
#include "mvTypes.h"
#include "wol/mvPp2Wol.h"


static ssize_t wol_help(char *buf)
{
	int of = 0;

	of += scnprintf(buf + of, PAGE_SIZE - of, "t, i, a, b, c, l, s - are dec numbers\n");
	of += scnprintf(buf + of, PAGE_SIZE - of, "v, m, e             - are hex numbers\n");
	of += scnprintf(buf + of, PAGE_SIZE - of, "\n");

	of += scnprintf(buf + of, PAGE_SIZE - of, "cat            help      - Show this help\n");
	of += scnprintf(buf + of, PAGE_SIZE - of, "cat            regs      - Show WOL registers\n");
	of += scnprintf(buf + of, PAGE_SIZE - of, "cat            status    - Show WOL status\n");
	of += scnprintf(buf + of, PAGE_SIZE - of, "echo port    > sleep     - Enter sleep mode for [port]\n");
	of += scnprintf(buf + of, PAGE_SIZE - of, "echo 1       > wakeup    - Force wakeup\n");
	of += scnprintf(buf + of, PAGE_SIZE - of, "echo mac     > magic_mac - Set MAC [a:b:c:d:e:f] for magic pattern\n");
	of += scnprintf(buf + of, PAGE_SIZE - of, "echo i ip    > arp_ip    - Set IP [a.b.c.d] for ARP IP[i] event\n");
	of += scnprintf(buf + of, PAGE_SIZE - of, "echo i o d m > ptrn      - Set pattern [i] with data [d] and mask [m] from  header offset [o]\n");
	of += scnprintf(buf + of, PAGE_SIZE - of, "                           [o] header offset: 0-127\n");
	of += scnprintf(buf + of, PAGE_SIZE - of, "                           [d] str, in format: b0:b1::b3:::b6\n");
	of += scnprintf(buf + of, PAGE_SIZE - of, "                           [m] str, in format: ff:ff::ff:::ff\n");
	of += scnprintf(buf + of, PAGE_SIZE - of, "echo [0|1]   > magic_en  - On/Off wakeup by magic packet\n");
	of += scnprintf(buf + of, PAGE_SIZE - of, "echo [0|1]   > ucast_en  - On/Off wakeup by Unicast packet\n");
	of += scnprintf(buf + of, PAGE_SIZE - of, "echo [0|1]   > mcast_en  - On/Off wakeup by Multicast packet\n");
	of += scnprintf(buf + of, PAGE_SIZE - of, "echo i [0|1] > arp_ip_en - On/Off wakeup by ARP IP [i] packet\n");
	of += scnprintf(buf + of, PAGE_SIZE - of, "echo i [0|1] > ptrn_en   - On/Off wakeup by pattern [i] packet\n");

	return of;
}

static ssize_t wol_show(struct device *dev,
			struct device_attribute *attr, char *buf)
{
	const char  *name = attr->attr.name;

	if (!capable(CAP_NET_ADMIN))
		return -EPERM;

	if (!strcmp(name, "help"))
		return wol_help(buf);

	if (!strcmp(name, "regs")) {
		mvPp2WolRegs();
	} else if (!strcmp(name, "status")) {
		mvPp2WolStatus();
	} else {
		printk(KERN_ERR "%s: illegal operation <%s>\n", __func__, attr->attr.name);
		return -EINVAL;
	}
	return 0;
}

static ssize_t wol_dec_store(struct device *dev,
			struct device_attribute *attr, const char *buf, size_t len)
{
	const char    *name = attr->attr.name;
	unsigned int  err = 0, t = 0, i = 0;
	unsigned long flags;

	if (!capable(CAP_NET_ADMIN))
		return -EPERM;

	sscanf(buf, "%d %d", &i, &t);

	local_irq_save(flags);
	if (!strcmp(name, "sleep"))
		err = mvPp2WolSleep(i);
	else if (!strcmp(name, "wakeup")) {
		if (i == 1)
			err = mvPp2WolWakeup();
	} else if (!strcmp(name, "magic_en")) {
		mvPp2WolMagicEventSet(i);
	} else if (!strcmp(name, "arp_ip_en")) {
		mvPp2WolArpEventSet(i, t);
	} else if (!strcmp(name, "ptrn_en")) {
		mvPp2WolPtrnEventSet(i, t);
	} else if (!strcmp(name, "mcast_en")) {
		mvPp2WolMcastEventSet(i);
	} else if (!strcmp(name, "ucast_en")) {
		mvPp2WolUcastEventSet(i);
	} else
		printk(KERN_ERR "%s: illegal operation <%s>\n", __func__, name);

	local_irq_restore(flags);

	if (err)
		printk(KERN_ERR "%s: <%s>, error %d\n", __func__, attr->attr.name, err);

	return err ? -EINVAL : len;
}

static ssize_t wol_mac_store(struct device *dev,
			struct device_attribute *attr, const char *buf, size_t len)
{
	const char    *name = attr->attr.name;
	unsigned int  err = 0;
	char          macStr[MV_MAC_STR_SIZE];
	MV_U8         mac[MV_MAC_ADDR_SIZE];
	unsigned long flags;

	if (!capable(CAP_NET_ADMIN))
		return -EPERM;

	sscanf(buf, "%s", macStr);

	local_irq_save(flags);

	if (!strcmp(name, "magic_mac")) {
		mvMacStrToHex(macStr, mac);
		err = mvPp2WolMagicDaSet(mac);
	} else
		printk(KERN_ERR "%s: illegal operation <%s>\n", __func__, name);

	local_irq_restore(flags);

	if (err)
		printk(KERN_ERR "%s: <%s>, error %d\n", __func__, attr->attr.name, err);

	return err ? -EINVAL : len;
}

static ssize_t wol_ip_store(struct device *dev,
			struct device_attribute *attr, const char *buf, size_t len)
{
	const char    *name = attr->attr.name;
	unsigned int  err = 0;
	int           i = 0;
	unsigned char ip[4];
	__be32        ipaddr;
	unsigned long flags;

	if (!capable(CAP_NET_ADMIN))
		return -EPERM;

	sscanf(buf, "%d %hhu.%hhu.%hhu.%hhu", &i, ip, ip + 1, ip + 2, ip + 3);

	local_irq_save(flags);

	if (!strcmp(name, "arp_ip")) {
		ipaddr = *(__be32 *)ip;
		err = mvPp2WolArpIpSet(i, ipaddr);
	} else
		printk(KERN_ERR "%s: illegal operation <%s>\n", __func__, name);

	local_irq_restore(flags);

	if (err)
		printk(KERN_ERR "%s: <%s>, error %d\n", __func__, attr->attr.name, err);

	return err ? -EINVAL : len;
}

static int wol_ptrn_get(char *ptrnStr, char *maskStr, MV_U8 *data, MV_U8 *mask, int max_size)
{
	int i, j, size;
	char tmp[3], mask_tmp[3];

	size = strlen(ptrnStr);
	i = 0;
	j = 0;
	while (i < size) {
		if (j >= max_size) {
			pr_err("pattern string is too long (max = %d): %s\n",
				max_size, ptrnStr);
			return j;
		}

		if (ptrnStr[i] == ':') {
			data[j] = 0;
			mask[j] = 0;
			j++;
			i++;
			continue;
		}
		if ((mvCharToHex(ptrnStr[i]) == -1) ||
		    (mvCharToHex(ptrnStr[i + 1]) == -1) ||
		    (((i + 2) > size) && (ptrnStr[i + 2] != ':'))) {
			pr_err("Wrong pattern string format size=%d, i=%d, j=%d: %s\n",
				size, i, j, &ptrnStr[i]);
			return -1;
		}

		tmp[0] = ptrnStr[i];
		tmp[1] = ptrnStr[i + 1];
		tmp[2] = '\0';
		mask_tmp[0] = maskStr[i];
		mask_tmp[1] = maskStr[i + 1];
		mask_tmp[2] = '\0';
		data[j] = (MV_U8) (strtol(tmp, NULL, 16));
		mask[j] = (MV_U8) (strtol(mask_tmp, NULL, 16));
		i += 3;
		j++;
	}
	return j;
}

static ssize_t wol_ptrn_store(struct device *dev,
			struct device_attribute *attr, const char *buf, size_t len)
{
	const char    *name = attr->attr.name;
	unsigned int  err = 0;
	int           size, i = 0, off = 0;
	char          ptrnStr[MV_PP2_WOL_PTRN_BYTES*3];
	char          maskStr[MV_PP2_WOL_PTRN_BYTES*3];
	char          data[MV_PP2_WOL_PTRN_BYTES];
	char          mask[MV_PP2_WOL_PTRN_BYTES];
	unsigned long flags;

	if (!capable(CAP_NET_ADMIN))
		return -EPERM;

	sscanf(buf, "%d %d %s %s", &i, &off, ptrnStr, maskStr);

	local_irq_save(flags);

	if (!strcmp(name, "ptrn")) {
		size = wol_ptrn_get(ptrnStr, maskStr, data, mask, MV_PP2_WOL_PTRN_BYTES);
		if (size != -1)
			err = mvPp2WolPtrnSet(i, off, size, data, mask);
		else
			err = 1;
	} else
		printk(KERN_ERR "%s: illegal operation <%s>\n", __func__, name);

	local_irq_restore(flags);

	if (err)
		printk(KERN_ERR "%s: <%s>, error %d\n", __func__, attr->attr.name, err);

	return err ? -EINVAL : len;
}

static DEVICE_ATTR(help,      S_IRUSR, wol_show, NULL);
static DEVICE_ATTR(regs,      S_IRUSR, wol_show, NULL);
static DEVICE_ATTR(status,    S_IRUSR, wol_show, NULL);
static DEVICE_ATTR(sleep,     S_IWUSR, NULL,     wol_dec_store);
static DEVICE_ATTR(wakeup,    S_IWUSR, NULL,     wol_dec_store);
static DEVICE_ATTR(magic_mac, S_IWUSR, NULL,     wol_mac_store);
static DEVICE_ATTR(arp_ip,    S_IWUSR, NULL,     wol_ip_store);
static DEVICE_ATTR(ptrn,      S_IWUSR, NULL,     wol_ptrn_store);
static DEVICE_ATTR(magic_en,  S_IWUSR, NULL,     wol_dec_store);
static DEVICE_ATTR(arp_ip_en, S_IWUSR, NULL,     wol_dec_store);
static DEVICE_ATTR(ptrn_en,   S_IWUSR, NULL,     wol_dec_store);
static DEVICE_ATTR(ucast_en,   S_IWUSR, NULL,    wol_dec_store);
static DEVICE_ATTR(mcast_en,   S_IWUSR, NULL,    wol_dec_store);


static struct attribute *wol_attrs[] = {
	&dev_attr_help.attr,
	&dev_attr_regs.attr,
	&dev_attr_status.attr,
	&dev_attr_sleep.attr,
	&dev_attr_wakeup.attr,
	&dev_attr_magic_mac.attr,
	&dev_attr_arp_ip.attr,
	&dev_attr_ptrn.attr,
	&dev_attr_magic_en.attr,
	&dev_attr_arp_ip_en.attr,
	&dev_attr_ptrn_en.attr,
	&dev_attr_ucast_en.attr,
	&dev_attr_mcast_en.attr,

	NULL
};

static struct attribute_group mv_wol_group = {
	.name = "wol",
	.attrs = wol_attrs,
};

#define MV_PP2_WOL_IRQ	110

int mv_pp2_wol_sysfs_init(struct kobject *pp2_kobj)
{
	int err = 0;

	err = sysfs_create_group(pp2_kobj, &mv_wol_group);
	if (err)
		printk(KERN_INFO "sysfs group %s failed %d\n", mv_wol_group.name, err);

	return err;
}

int mv_pp2_wol_sysfs_exit(struct kobject *pp2_kobj)
{
	sysfs_remove_group(pp2_kobj, &mv_wol_group);

	return 0;
}
