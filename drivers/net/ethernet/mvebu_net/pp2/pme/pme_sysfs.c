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

#include "mvCommon.h"
#include "mvTypes.h"
#include "pme/mvPp2PmeHw.h"

static MV_PP2_PME_ENTRY  mv_pp2_pme_e;

static ssize_t pme_help(char *buf)
{
	int off = 0;

	off += scnprintf(buf + off, PAGE_SIZE - off, "t, i, a, b, c, l, s - are dec numbers\n");
	off += scnprintf(buf + off, PAGE_SIZE - off, "v, m, e             - are hex numbers\n");
	off += scnprintf(buf + off, PAGE_SIZE - off, "\n");

	off += scnprintf(buf + off, PAGE_SIZE - off, "cat              help          - Show this help\n");
	off += scnprintf(buf + off, PAGE_SIZE - off, "cat              hw_regs       - Show PME hardware registers\n");
	off += scnprintf(buf + off, PAGE_SIZE - off, "cat              sw_dump       - Show PME sw etry\n");
	off += scnprintf(buf + off, PAGE_SIZE - off, "cat              hw_i_dump     - Dump valid PME hw entries of the instruction table\n");
	off += scnprintf(buf + off, PAGE_SIZE - off, "cat              hw_i_dump_all - Dump all PME hw entries of the instruction table\n");
	off += scnprintf(buf + off, PAGE_SIZE - off, "cat              hw_i_inv      - Invalidate all PME hw entries in the table <t>\n");
	off += scnprintf(buf + off, PAGE_SIZE - off, "echo 1         > sw_clear      - Clear PME sw etry\n");
	off += scnprintf(buf + off, PAGE_SIZE - off, "echo i         > hw_i_read     - Read PME hw entry <i> from instruction table into sw entry\n");
	off += scnprintf(buf + off, PAGE_SIZE - off, "echo i         > hw_i_write    - Write sw entry to PME hw entry <i> in the instruction table\n");
	off += scnprintf(buf + off, PAGE_SIZE - off, "echo v         > sw_word       - Set 4 bytes value <v> to sw entry\n");
	off += scnprintf(buf + off, PAGE_SIZE - off, "echo v         > sw_cmd        - Set modification command to instruction table sw entry\n");
	off += scnprintf(buf + off, PAGE_SIZE - off, "echo v         > sw_data       - Set modification data (2 bytes) to to instruction table sw entry\n");
	off += scnprintf(buf + off, PAGE_SIZE - off, "echo a         > sw_type       - Set type of modification command <a> to instruction table sw entry\n");
	off += scnprintf(buf + off, PAGE_SIZE - off, "echo 0|1       > sw_last       - Set/Clear last bit in instruction table sw entry\n");
	off += scnprintf(buf + off, PAGE_SIZE - off, "echo a b c     > sw_flags      - Set/Clear flags: <a>-last, <b>-ipv4csum, <c>-l4csum\n");
	off += scnprintf(buf + off, PAGE_SIZE - off, "echo t         > hw_d_dump     - Dump non zero PME hw entries from the data table <t>\n");
	off += scnprintf(buf + off, PAGE_SIZE - off, "echo t         > hw_d_clear    - Clear all PME hw entries in the data table <t>\n");
	off += scnprintf(buf + off, PAGE_SIZE - off, "echo t i v     > hw_d_write    - Write 2b modification data <v> to entry <i> of data table <t>\n");
	off += scnprintf(buf + off, PAGE_SIZE - off, "echo t i       > hw_d_read     - Read and print 2b modification data from entry <i> of data table <t>\n");
	off += scnprintf(buf + off, PAGE_SIZE - off, "echo i v       > vlan_etype    - Set 2 bytes value <v> of VLAN ethertype <i>.\n");
	off += scnprintf(buf + off, PAGE_SIZE - off, "echo v         > vlan_def      - Set 2 bytes value <v> of default VLAN ethertype.\n");
	off += scnprintf(buf + off, PAGE_SIZE - off, "echo i v       > dsa_etype     - Set 2 bytes value <v> of DSA ethertype <i>.\n");
	off += scnprintf(buf + off, PAGE_SIZE - off, "echo v         > dsa_src_dev   - Set source device value to be set in DSA tag.\n");
	off += scnprintf(buf + off, PAGE_SIZE - off, "echo 0|1       > ttl_zero      - Action for packet with zero TTL: 0-drop, 1-forward.\n");
	off += scnprintf(buf + off, PAGE_SIZE - off, "echo v         > pppoe_etype   - Set 2 bytes value <v> of PPPoE ethertype\n");
	off += scnprintf(buf + off, PAGE_SIZE - off, "echo v         > pppoe_len     - Set 2 bytes value <v> of PPPoE length configuration\n");
	off += scnprintf(buf + off, PAGE_SIZE - off, "echo i v       > pppoe_proto   - Set 2 bytes value <v> of PPPoE protocol <i>.\n");
	off += scnprintf(buf + off, PAGE_SIZE - off, "echo a t v     > pppoe_set     - Set PPPoE header fields: version <a>, type <t> and code <v>.\n");
	off += scnprintf(buf + off, PAGE_SIZE - off, "echo s i       > max_config    - Set max header size <s bytes> and max instructions <i>.\n");

	return off;
}

static ssize_t pme_show(struct device *dev,
			struct device_attribute *attr, char *buf)
{
	const char  *name = attr->attr.name;

	if (!capable(CAP_NET_ADMIN))
		return -EPERM;

	if (!strcmp(name, "help"))
		return pme_help(buf);

	if (!strcmp(name, "sw_dump")) {
		mvPp2PmeSwDump(&mv_pp2_pme_e);
	} else if (!strcmp(name, "hw_regs")) {
		mvPp2PmeHwRegs();
	} else	if (!strcmp(name, "hw_i_dump")) {
		mvPp2PmeHwDump(0);
	} else if (!strcmp(name, "hw_i_dump_all")) {
		mvPp2PmeHwDump(1);
	} else if (!strcmp(name, "hw_i_inv")) {
		mvPp2PmeHwInvAll();
	} else {
		printk(KERN_ERR "%s: illegal operation <%s>\n", __func__, attr->attr.name);
		return -EINVAL;
	}
	return 0;
}

static ssize_t pme_dec_store(struct device *dev,
			struct device_attribute *attr, const char *buf, size_t len)
{
	const char    *name = attr->attr.name;
	unsigned int  err = 0, t = 0, i = 0, v = 0;
	unsigned long flags;
	unsigned short data;

	if (!capable(CAP_NET_ADMIN))
		return -EPERM;

	sscanf(buf, "%d %d %x", &t, &i, &v);

	local_irq_save(flags);

	if (!strcmp(name, "hw_i_write")) {
		err = mvPp2PmeHwWrite(t, &mv_pp2_pme_e);
	} else if (!strcmp(name, "sw_clear")) {
		err = mvPp2PmeSwClear(&mv_pp2_pme_e);
	} else if (!strcmp(name, "hw_i_read")) {
		err = mvPp2PmeHwRead(t, &mv_pp2_pme_e);
	} else if (!strcmp(name, "sw_flags")) {
		err = mvPp2PmeSwCmdFlagsSet(&mv_pp2_pme_e, t, i, v);
	} else if (!strcmp(name, "sw_last")) {
		err = mvPp2PmeSwCmdLastSet(&mv_pp2_pme_e, t);
	} else if (!strcmp(name, "hw_d_dump")) {
		err = mvPp2PmeHwDataTblDump(t);
	} else if (!strcmp(name, "hw_d_clear")) {
		err = mvPp2PmeHwDataTblClear(t);
	} else if (!strcmp(name, "hw_d_read")) {
		err = mvPp2PmeHwDataTblRead(t, i, &data);
		printk(KERN_INFO "Data%d table entry #%d: 0x%04x\n", t, i, data);
	} else if (!strcmp(name, "hw_d_write")) {
		data = (unsigned short)v;
		err = mvPp2PmeHwDataTblWrite(t, i, data);
	} else if (!strcmp(name, "ttl_zero")) {
		err = mvPp2PmeTtlZeroSet(t);
	} else if (!strcmp(name, "max_config")) {
		err = mvPp2PmeMaxConfig(t, i, v);
	} else if (!strcmp(name, "pppoe_set")) {
		err = mvPp2PmeMaxConfig(t, i, v);
	} else
		printk(KERN_ERR "%s: illegal operation <%s>\n", __func__, name);

	local_irq_restore(flags);

	if (err)
		printk(KERN_ERR "%s: <%s>, error %d\n", __func__, attr->attr.name, err);

	return err ? -EINVAL : len;
}

static ssize_t pme_dec_hex_store(struct device *dev,
				struct device_attribute *attr, const char *buf, size_t len)
{
	const char	*name = attr->attr.name;
	unsigned int    err = 0, i = 0, v = 0;
	unsigned long   flags;

	if (!capable(CAP_NET_ADMIN))
		return -EPERM;

	sscanf(buf, "%d %x", &i, &v);

	local_irq_save(flags);

	if (!strcmp(name, "vlan_etype"))
		mvPp2PmeVlanEtherTypeSet(i, v);
	else if (!strcmp(name, "dsa_etype"))
		mvPp2PmeDsaDefaultSet(i, v);
	else if (!strcmp(name, "pppoe_proto"))
		mvPp2PmePppoeProtoSet(i, v);
	else
		printk(KERN_ERR "%s: illegal operation <%s>\n", __func__, attr->attr.name);

	local_irq_restore(flags);

	if (err)
		printk(KERN_ERR "%s: <%s>, error %d\n", __func__, attr->attr.name, err);

	return err ? -EINVAL : len;
}

static ssize_t pme_hex_store(struct device *dev,
				struct device_attribute *attr, const char *buf, size_t len)
{
	const char	*name = attr->attr.name;
	unsigned int    err = 0, v = 0;
	unsigned long   flags;

	if (!capable(CAP_NET_ADMIN))
		return -EPERM;

	sscanf(buf, "%x", &v);

	local_irq_save(flags);

	if (!strcmp(name, "sw_word"))
		mvPp2PmeSwWordSet(&mv_pp2_pme_e, v);
	else if (!strcmp(name, "sw_cmd"))
		mvPp2PmeSwCmdSet(&mv_pp2_pme_e, v);
	else if (!strcmp(name, "sw_type"))
		mvPp2PmeSwCmdTypeSet(&mv_pp2_pme_e, v);
	else if (!strcmp(name, "sw_data"))
		mvPp2PmeSwCmdDataSet(&mv_pp2_pme_e, v);
	else if (!strcmp(name, "vlan_def"))
		mvPp2PmeVlanDefaultSet(v);
	else if (!strcmp(name, "dsa_src_dev"))
		mvPp2PmeDsaSrcDevSet(v);
	else if (!strcmp(name, "pppoe_etype"))
		mvPp2PmePppoeEtypeSet(v);
	else if (!strcmp(name, "pppoe_len"))
		mvPp2PmePppoeLengthSet(v);
	else
		printk(KERN_ERR "%s: illegal operation <%s>\n", __func__, attr->attr.name);

	local_irq_restore(flags);

	if (err)
		printk(KERN_ERR "%s: <%s>, error %d\n", __func__, attr->attr.name, err);

	return err ? -EINVAL : len;
}

static DEVICE_ATTR(help,          S_IRUSR, pme_show, NULL);
static DEVICE_ATTR(sw_dump,       S_IRUSR, pme_show, NULL);
static DEVICE_ATTR(hw_regs,       S_IRUSR, pme_show, NULL);
static DEVICE_ATTR(hw_i_dump,     S_IRUSR, pme_show, NULL);
static DEVICE_ATTR(hw_i_dump_all, S_IRUSR, pme_show, NULL);
static DEVICE_ATTR(hw_i_inv,      S_IRUSR, pme_show, NULL);
static DEVICE_ATTR(sw_clear,      S_IWUSR, NULL,     pme_dec_store);
static DEVICE_ATTR(hw_i_write,    S_IWUSR, NULL,     pme_dec_store);
static DEVICE_ATTR(hw_i_read,     S_IWUSR, NULL,     pme_dec_store);
static DEVICE_ATTR(sw_flags,      S_IWUSR, NULL,     pme_dec_store);
static DEVICE_ATTR(sw_last,       S_IWUSR, NULL,     pme_dec_store);
static DEVICE_ATTR(sw_word,       S_IWUSR, NULL,     pme_hex_store);
static DEVICE_ATTR(sw_cmd,        S_IWUSR, NULL,     pme_hex_store);
static DEVICE_ATTR(sw_type,       S_IWUSR, NULL,     pme_hex_store);
static DEVICE_ATTR(sw_data,       S_IWUSR, NULL,     pme_hex_store);
static DEVICE_ATTR(hw_d_dump,     S_IWUSR, NULL,     pme_dec_store);
static DEVICE_ATTR(hw_d_clear,    S_IWUSR, NULL,     pme_dec_store);
static DEVICE_ATTR(hw_d_write,    S_IWUSR, NULL,     pme_dec_store);
static DEVICE_ATTR(hw_d_read,     S_IWUSR, NULL,     pme_dec_store);
static DEVICE_ATTR(vlan_etype,    S_IWUSR, NULL,     pme_dec_hex_store);
static DEVICE_ATTR(vlan_def,      S_IWUSR, NULL,     pme_hex_store);
static DEVICE_ATTR(dsa_etype,     S_IWUSR, NULL,     pme_dec_hex_store);
static DEVICE_ATTR(dsa_src_dev,   S_IWUSR, NULL,     pme_hex_store);
static DEVICE_ATTR(ttl_zero,      S_IWUSR, NULL,     pme_dec_store);
static DEVICE_ATTR(pppoe_set,     S_IWUSR, NULL,     pme_dec_store);
static DEVICE_ATTR(pppoe_etype,   S_IWUSR, NULL,     pme_hex_store);
static DEVICE_ATTR(pppoe_len,     S_IWUSR, NULL,     pme_hex_store);
static DEVICE_ATTR(pppoe_proto,   S_IWUSR, NULL,     pme_dec_hex_store);
static DEVICE_ATTR(max_config,    S_IWUSR, NULL,     pme_dec_store);


static struct attribute *pme_attrs[] = {
	&dev_attr_help.attr,
	&dev_attr_sw_dump.attr,
	&dev_attr_sw_clear.attr,
	&dev_attr_hw_regs.attr,
	&dev_attr_hw_i_write.attr,
	&dev_attr_hw_i_read.attr,
	&dev_attr_hw_i_dump.attr,
	&dev_attr_hw_i_dump_all.attr,
	&dev_attr_hw_i_inv.attr,
	&dev_attr_sw_word.attr,
	&dev_attr_sw_cmd.attr,
	&dev_attr_sw_data.attr,
	&dev_attr_sw_type.attr,
	&dev_attr_sw_flags.attr,
	&dev_attr_sw_last.attr,
	&dev_attr_hw_d_read.attr,
	&dev_attr_hw_d_write.attr,
	&dev_attr_hw_d_dump.attr,
	&dev_attr_hw_d_clear.attr,
	&dev_attr_vlan_etype.attr,
	&dev_attr_vlan_def.attr,
	&dev_attr_dsa_etype.attr,
	&dev_attr_dsa_src_dev.attr,
	&dev_attr_ttl_zero.attr,
	&dev_attr_pppoe_set.attr,
	&dev_attr_pppoe_etype.attr,
	&dev_attr_pppoe_len.attr,
	&dev_attr_pppoe_proto.attr,
	&dev_attr_max_config.attr,

	NULL
};

static struct attribute_group pme_group = {
	.name = "pme",
	.attrs = pme_attrs,
};

int mv_pp2_pme_sysfs_init(struct kobject *pp2_kobj)
{
	int err;

	err = sysfs_create_group(pp2_kobj, &pme_group);
	if (err)
		printk(KERN_INFO "sysfs group %s failed %d\n", pme_group.name, err);

	return err;
}

int mv_pp2_pme_sysfs_exit(struct kobject *pp2_kobj)
{
	sysfs_remove_group(pp2_kobj, &pme_group);

	return 0;
}

