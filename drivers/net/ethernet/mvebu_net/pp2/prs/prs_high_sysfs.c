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
#include "prs/mvPp2PrsHw.h"
#include "prs/mvPp2Prs.h"


static ssize_t mv_prs_high_help(char *b)
{
	int o = 0;

	o += scnprintf(b + o, PAGE_SIZE - o, "cd                 debug       - move to parser low level sysfs directory\n");
	o += scnprintf(b + o, PAGE_SIZE - o, "cat                dump        - dump all valid HW entries\n");

	o += scnprintf(b + o, PAGE_SIZE - o, "\n");
	o += scnprintf(b + o, PAGE_SIZE - o, "echo a b c d e     > flow      - Add flow entry to HW\n");
	o += scnprintf(b + o, PAGE_SIZE - o, "                                 flowId <a>, result <b>, result mask <c>, port <d>, tcam index <e>.\n");
	o += scnprintf(b + o, PAGE_SIZE - o, "\n");
	o += scnprintf(b + o, PAGE_SIZE - o, "echo a b [1|0]   > vlan1     - Add/Delete single vlan: tpid1 <a>, port map <b>\n");
	o += scnprintf(b + o, PAGE_SIZE - o, "echo a b c [1|0] > vlan2     - Add/Delete double vlan: tpid1 <a>, tpid2 <b>, port map <c>\n");
	o += scnprintf(b + o, PAGE_SIZE - o, "echo a b c d e   > vlan3     - Triple vlan entry tpid1 <a>, tpid2 <b>, tpid3 <c>\n");
	o += scnprintf(b + o, PAGE_SIZE - o, "                               ports bitmap <d>, add/del <e=1/0>\n");
	o += scnprintf(b + o, PAGE_SIZE - o, "echo 1           > vlan_del  - Delete all vlan entries.\n");
	o += scnprintf(b + o, PAGE_SIZE - o, "echo a b c d e f > mac_range - Add mac entry to HW\n");
	o += scnprintf(b + o, PAGE_SIZE - o, "                               port map <a>, da <b> mask <c>, ri <d> mask <e>, end <f>\n");
	o += scnprintf(b + o, PAGE_SIZE - o, "echo a b c       > mac_del   - Delete mac entry from HW\n");
	o += scnprintf(b + o, PAGE_SIZE - o, "                               port map <a>, da <b>, da mask <c>\n");
	o += scnprintf(b + o, PAGE_SIZE - o, "echo a b c d e   > etype_add - Add ethertype entry to HW\n");
	o += scnprintf(b + o, PAGE_SIZE - o, "                               port map <a>, etype <b>, ri <c> mask <d>, end <e>\n");
	o += scnprintf(b + o, PAGE_SIZE - o, "echo a b         > etype_del - Delete etype <b> entry from HW for ports in port map <a>\n");
	o += scnprintf(b + o, PAGE_SIZE - o, "\n");
	o += scnprintf(b + o, PAGE_SIZE - o, "echo p [0|1|2|3] > tag       - None[0], Marvell Header[1], DSA tag[2], EDSA tag[3]\n");
	/* etypeDsaMod and etypeDsa meaningless if  port in DSA/EDSA mode */
	o += scnprintf(b + o, PAGE_SIZE - o, "echo p {0|1}     > etypeMod  - Expected EtherType DSA[0]/EDSA[1] if port tag is not DSA/EDSA\n");
	/* typeDsa meaningless if  all ports in DSA/EDSA mode */
	o += scnprintf(b + o, PAGE_SIZE - o, "echo [hex]       > etypeDsa  - Expected DSA/EDSA ethertype [hex]\n");
	o += scnprintf(b + o, PAGE_SIZE - o, "\n");
	return o;
}


static ssize_t mv_prs_high_show(struct device *dev,
				  struct device_attribute *attr, char *buf)
{
	const char      *name = attr->attr.name;
	int             off = 0;

	if (!capable(CAP_NET_ADMIN))
		return -EPERM;

	if (!strcmp(name, "dump"))
		mvPp2PrsHwDump();
	else
		off += mv_prs_high_help(buf);

	return off;
}

static ssize_t mv_prs_high_store_unsigned(struct device *dev,
				   struct device_attribute *attr, const char *buf, size_t len)
{
	const char    *name = attr->attr.name;
	unsigned int  err = 0, a = 0, b = 0, c = 0, d = 0, e = 0;
	unsigned long flags;

	if (!capable(CAP_NET_ADMIN))
		return -EPERM;

	sscanf(buf, "%x %x %x %x %x", &a, &b, &c, &d, &e);

	local_irq_save(flags);

	if (!strcmp(name, "flow"))
		mvPrsFlowIdGen(e, a, b, c, d);
	else if (!strcmp(name, "vlan1"))
		mvPp2PrsSingleVlan(a, b, c);
	else if (!strcmp(name, "vlan2"))
		mvPp2PrsDoubleVlan(a, b, c, d);
	else if (!strcmp(name, "vlan3"))
		mvPp2PrsTripleVlan(a, b, c, d, e);
	else if (!strcmp(name, "vlan_del"))
		mvPp2PrsVlanAllDel();
	else if (!strcmp(name, "etype_add"))
		mvPrsEthTypeSet(a, b, c, d, e);
	else if (!strcmp(name, "etype_del"))
		mvPrsEthTypeDel(a, b);
	else if (!strcmp(name, "tag"))
		mvPp2PrsTagModeSet(a, b);
	else if (!strcmp(name, "etypeMod"))
		mvPp2PrsEtypeDsaModeSet(a, b);
	else if (!strcmp(name, "etypeDsa"))
		mvPp2PrsEtypeDsaSet(a);
	else {
		err = 1;
		printk(KERN_ERR "%s: illegal operation <%s>\n", __func__, attr->attr.name);
	}
	local_irq_restore(flags);

	if (err)
		printk(KERN_ERR "%s: <%s>, error %d\n", __func__, attr->attr.name, err);

	return err ? -EINVAL : len;
}


static ssize_t mv_prs_high_store_str(struct device *dev,
				   struct device_attribute *attr, const char *buf, size_t len)
{
	const char    *name = attr->attr.name;
	unsigned int  err = 0, prt = 0, ri = 0, ri_mask = 0, fin = 0;
	unsigned char da[MV_MAC_ADDR_SIZE];
	unsigned char da_mask[MV_MAC_ADDR_SIZE];

	unsigned long flags;

	if (!capable(CAP_NET_ADMIN))
		return -EPERM;

	sscanf(buf,
		"%x %2hhx:%2hhx:%2hhx:%2hhx:%2hhx:%2hhx %2hhx:%2hhx:%2hhx:%2hhx:%2hhx:%2hhx %x %x %x",
		&prt, da, da+1, da+2, da+3, da+4, da+5,
		da_mask, da_mask+1, da_mask+2, da_mask+3, da_mask+4, da_mask+5, &ri, &ri_mask, &fin);

	local_irq_save(flags);

	if (!strcmp(name, "mac_range"))
		mvPrsMacDaRangeSet(prt, da, da_mask, ri, ri_mask, fin);
	else if (!strcmp(name, "mac_del"))
		mvPrsMacDaRangeDel(prt, da, da_mask);
	else {
		err = 1;
		printk(KERN_ERR "%s: illegal operation <%s>\n", __func__, attr->attr.name);
	}
	local_irq_restore(flags);

	if (err)
		printk(KERN_ERR "%s: <%s>, error %d\n", __func__, attr->attr.name, err);

	return err ? -EINVAL : len;
}

static DEVICE_ATTR(dump,		S_IRUSR, mv_prs_high_show, NULL);
static DEVICE_ATTR(flow,		S_IWUSR, mv_prs_high_show, mv_prs_high_store_unsigned);
static DEVICE_ATTR(vlan1,		S_IWUSR, mv_prs_high_show, mv_prs_high_store_unsigned);
static DEVICE_ATTR(vlan2,		S_IWUSR, mv_prs_high_show, mv_prs_high_store_unsigned);
static DEVICE_ATTR(vlan3,		S_IWUSR, mv_prs_high_show, mv_prs_high_store_unsigned);
static DEVICE_ATTR(vlan_del,		S_IWUSR, mv_prs_high_show, mv_prs_high_store_unsigned);
static DEVICE_ATTR(etype_add,		S_IWUSR, mv_prs_high_show, mv_prs_high_store_unsigned);
static DEVICE_ATTR(etype_del,		S_IWUSR, mv_prs_high_show, mv_prs_high_store_unsigned);
static DEVICE_ATTR(mac_range,		S_IWUSR, mv_prs_high_show, mv_prs_high_store_str);
static DEVICE_ATTR(mac_del,		S_IWUSR, mv_prs_high_show, mv_prs_high_store_str);
static DEVICE_ATTR(tag,			S_IWUSR, mv_prs_high_show, mv_prs_high_store_unsigned);
static DEVICE_ATTR(etypeMod,		S_IWUSR, mv_prs_high_show, mv_prs_high_store_unsigned);
static DEVICE_ATTR(etypeDsa,		S_IWUSR, mv_prs_high_show, mv_prs_high_store_unsigned);
static DEVICE_ATTR(help,		S_IRUSR, mv_prs_high_show, NULL);


static struct attribute *prs_high_attrs[] = {
	&dev_attr_dump.attr,
	&dev_attr_help.attr,
	&dev_attr_flow.attr,
	&dev_attr_vlan1.attr,
	&dev_attr_vlan2.attr,
	&dev_attr_vlan3.attr,
	&dev_attr_vlan_del.attr,
	&dev_attr_etype_add.attr,
	&dev_attr_etype_del.attr,
	&dev_attr_mac_range.attr,
	&dev_attr_mac_del.attr,
	&dev_attr_tag.attr,
	&dev_attr_etypeMod.attr,
	&dev_attr_etypeDsa.attr,
    NULL
};

static struct attribute_group prs_high_group = {
	.name = "prsHigh",
	.attrs = prs_high_attrs,
};

int mv_pp2_prs_high_sysfs_init(struct kobject *pp2_kobj)
{
	int err;

	err = sysfs_create_group(pp2_kobj, &prs_high_group);
	if (err)
		pr_err("sysfs group %s failed %d\n", prs_high_group.name, err);

	return err;
}

int mv_pp2_prs_high_sysfs_exit(struct kobject *pp2_kobj)
{
	sysfs_remove_group(pp2_kobj, &prs_high_group);

	return 0;
}

