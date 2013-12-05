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

#include "pmt/mvPmt.h"

static MV_NETA_PMT  mv_neta_pmt_e;

static ssize_t pmt_help(char *buf)
{
	int off = 0;

	off += mvOsSPrintf(buf+off, "p, i, a, b, c - are dec numbers\n");
	off += mvOsSPrintf(buf+off, "v, m          - are hex numbers\n");
	off += mvOsSPrintf(buf+off, "\n");

	off += mvOsSPrintf(buf+off, "cat          help          - Show this help\n");
	off += mvOsSPrintf(buf+off, "cat          sw_dump       - Show sw PMT etry\n");
	off += mvOsSPrintf(buf+off, "cat          sw_clear      - Clear sw PMT etry\n");
	off += mvOsSPrintf(buf+off, "echo v     > sw_word       - Set 4 bytes value <v> to sw entry\n");
	off += mvOsSPrintf(buf+off, "echo p     > hw_regs       - Show PMT registers\n");
	off += mvOsSPrintf(buf+off, "echo p     > hw_dump       - Dump valid PMT entries of the port <p>\n");
	off += mvOsSPrintf(buf+off, "echo p     > hw_dump_all   - Dump all PMT entries of the port <p>\n");
	off += mvOsSPrintf(buf+off, "echo p i   > hw_read       - Read PMT entry <i> on port <p> into sw entry\n");
	off += mvOsSPrintf(buf+off, "echo p i   > hw_write      - Write sw entry into PMT entry <i> on port <p>\n");
	off += mvOsSPrintf(buf+off, "echo p i   > hw_inv        - Disable PMT entry <i> on port <p>\n");
	off += mvOsSPrintf(buf+off, "echo p     > hw_inv_all    - Disable all PMT entries on port <p>\n");
	off += mvOsSPrintf(buf+off, "echo 0|1   > s_last        - Set/Clear last bit\n");
	off += mvOsSPrintf(buf+off, "echo a b c > s_flags       - Set/Clear flags: <a>-last, <b>-ipv4csum, <c>-l4csum\n");
	off += mvOsSPrintf(buf+off, "echo v     > s_rep_2b      - Replace 2 bytes with value <v>\n");
	off += mvOsSPrintf(buf+off, "echo v     > s_add_2b      - Add 2 bytes with value <v> to sw entry\n");
	off += mvOsSPrintf(buf+off, "echo a b c > s_del_2b      - Delete <a> bytes, Skip <b> bytes before, Skip <c> bytes after\n");
	off += mvOsSPrintf(buf+off, "echo v m   > s_rep_lsb     - Replace LSB with value <v> and mask <m>\n");
	off += mvOsSPrintf(buf+off, "echo v m   > s_rep_msb     - Replace MSB with value <v> and mask <m>\n");
	off += mvOsSPrintf(buf+off, "echo v     > s_ip_csum     - Replace IP checksum. <v> used as additional info\n");
	off += mvOsSPrintf(buf+off, "echo v     > s_l4_csum     - Replace TCP/UDP checksum. <v> used as additional info\n");
	off += mvOsSPrintf(buf+off, "echo a b   > s_dec_lsb     - Decrement LSB, Skip <a> bytes before, Skip <b> bytes after\n");
	off += mvOsSPrintf(buf+off, "echo a b   > s_dec_msb     - Decrement MSB, Skip <a> bytes before, Skip <b> bytes after\n");
	off += mvOsSPrintf(buf+off, "echo a     > s_skip        - Skip <a> bytes. Must be even\n");
	off += mvOsSPrintf(buf+off, "echo a b c > s_jump        - Jump to entry <a>. <b>=1-skip,2-subroutine. <c>=1-green,2-yellow\n");

	return off;
}

static ssize_t pmt_show(struct device *dev,
			struct device_attribute *attr, char *buf)
{
	const char  *name = attr->attr.name;

	if (!capable(CAP_NET_ADMIN))
		return -EPERM;

	if (!strcmp(name, "help"))
		return pmt_help(buf);

	if (!strcmp(name, "sw_dump")) {
		mvNetaPmtEntryPrint(&mv_neta_pmt_e);
	} else if (!strcmp(name, "sw_clear")) {
		MV_NETA_PMT_CLEAR(&mv_neta_pmt_e);
	}

	return 0;
}

static ssize_t pmt_hw_store(struct device *dev,
			struct device_attribute *attr, const char *buf, size_t len)
{
	const char    *name = attr->attr.name;
	unsigned int  err = 0, p = 0, i = 0;
	unsigned long flags;

	if (!capable(CAP_NET_ADMIN))
		return -EPERM;

	sscanf(buf, "%d %d", &p, &i);

	local_irq_save(flags);

	if (!strcmp(name, "hw_dump")) {
		mvNetaPmtDump(p, 1);
	} else if (!strcmp(name, "hw_dump_all")) {
		mvNetaPmtDump(p, 0);
	} else if (!strcmp(name, "hw_regs")) {
		mvNetaPmtRegs(p, i);
	} else if (!strcmp(name, "hw_write")) {
		err = mvNetaPmtWrite(p, i, &mv_neta_pmt_e);
	} else if (!strcmp(name, "hw_read")) {
		err = mvNetaPmtRead(p, i, &mv_neta_pmt_e);
	} else if (!strcmp(name, "hw_inv")) {
		MV_NETA_PMT_INVALID_SET(&mv_neta_pmt_e);
		err = mvNetaPmtWrite(p, i, &mv_neta_pmt_e);
	} else if (!strcmp(name, "hw_inv_all")) {
		err = mvNetaPmtClear(p);
	} else
		printk(KERN_ERR "%s: illegal operation <%s>\n", __func__, attr->attr.name);

	local_irq_restore(flags);

	if (err)
		printk(KERN_ERR "%s: <%s>, error %d\n", __func__, attr->attr.name, err);

	return err ? -EINVAL : len;
}

static ssize_t pmt_sw_dec_store(struct device *dev,
			struct device_attribute *attr, const char *buf, size_t len)
{
	const char	*name = attr->attr.name;
	unsigned int    err = 0, a = 0, b = 0, c = 0;
	unsigned long   flags;

	if (!capable(CAP_NET_ADMIN))
		return -EPERM;

	sscanf(buf, "%d %d %d", &a, &b, &c);

	local_irq_save(flags);

	if (!strcmp(name, "s_last")) {
		mvNetaPmtLastFlag(&mv_neta_pmt_e, a);
	} else if (!strcmp(name, "s_flags")) {
		mvNetaPmtFlags(&mv_neta_pmt_e, a, b, c);
	} else if (!strcmp(name, "s_del")) {
		mvNetaPmtDelShorts(&mv_neta_pmt_e, a/2, b/2, c/2);
	} else if (!strcmp(name, "s_skip")) {
		mvNetaPmtSkip(&mv_neta_pmt_e, a/2);
	} else if (!strcmp(name, "s_dec_lsb")) {
		mvNetaPmtDecLSB(&mv_neta_pmt_e, a/2, b/2);
	} else if (!strcmp(name, "s_dec_msb")) {
		mvNetaPmtDecMSB(&mv_neta_pmt_e, a/2, b/2);
	} else if (!strcmp(name, "s_jump")) {
		mvNetaPmtJump(&mv_neta_pmt_e, a, b, c);
	} else
		printk(KERN_ERR "%s: illegal operation <%s>\n", __func__, attr->attr.name);

	local_irq_restore(flags);

	if (err)
		printk(KERN_ERR "%s: <%s>, error %d\n", __func__, attr->attr.name, err);

	return err ? -EINVAL : len;
}

static ssize_t pmt_sw_hex_store(struct device *dev,
				struct device_attribute *attr, const char *buf, size_t len)
{
	const char	*name = attr->attr.name;
	unsigned int    err = 0, v = 0, a = 0;
	unsigned long   flags;

	if (!capable(CAP_NET_ADMIN))
		return -EPERM;

	sscanf(buf, "%x %d", &v, &a);

	local_irq_save(flags);

	if (!strcmp(name, "sw_word")) {
		mv_neta_pmt_e.word = v;
	} else if (!strcmp(name, "s_rep_2b")) {
		mvNetaPmtReplace2Bytes(&mv_neta_pmt_e, v);
	} else if (!strcmp(name, "s_add_2b")) {
		mvNetaPmtAdd2Bytes(&mv_neta_pmt_e, v);
	} else if (!strcmp(name, "s_rep_lsb")) {
		mvNetaPmtReplaceLSB(&mv_neta_pmt_e, v, a);
	} else if (!strcmp(name, "s_rep_msb")) {
		mvNetaPmtReplaceMSB(&mv_neta_pmt_e, v, a);
	} else if (!strcmp(name, "s_ip_csum")) {
		mvNetaPmtReplaceIPv4csum(&mv_neta_pmt_e, v);
	} else if (!strcmp(name, "s_l4_csum")) {
		mvNetaPmtReplaceL4csum(&mv_neta_pmt_e, v);
	} else
		printk(KERN_ERR "%s: illegal operation <%s>\n", __func__, attr->attr.name);

	local_irq_restore(flags);

	if (err)
		printk(KERN_ERR "%s: <%s>, error %d\n", __func__, attr->attr.name, err);

	return err ? -EINVAL : len;
}

static DEVICE_ATTR(help,        S_IRUSR, pmt_show, NULL);
static DEVICE_ATTR(sw_dump,     S_IRUSR, pmt_show, NULL);
static DEVICE_ATTR(sw_clear,    S_IRUSR, pmt_show, NULL);
static DEVICE_ATTR(hw_regs,     S_IWUSR, pmt_show, pmt_hw_store);
static DEVICE_ATTR(hw_write,    S_IWUSR, pmt_show, pmt_hw_store);
static DEVICE_ATTR(hw_read,     S_IWUSR, pmt_show, pmt_hw_store);
static DEVICE_ATTR(hw_dump,     S_IWUSR, pmt_show, pmt_hw_store);
static DEVICE_ATTR(hw_dump_all, S_IWUSR, pmt_show, pmt_hw_store);
static DEVICE_ATTR(hw_inv,      S_IWUSR, pmt_show, pmt_hw_store);
static DEVICE_ATTR(hw_inv_all,  S_IWUSR, pmt_show, pmt_hw_store);
static DEVICE_ATTR(sw_word,     S_IWUSR, pmt_show, pmt_sw_hex_store);
static DEVICE_ATTR(s_rep_2b,    S_IWUSR, pmt_show, pmt_sw_hex_store);
static DEVICE_ATTR(s_rep_lsb,   S_IWUSR, pmt_show, pmt_sw_hex_store);
static DEVICE_ATTR(s_rep_msb,   S_IWUSR, pmt_show, pmt_sw_hex_store);
static DEVICE_ATTR(s_ip_csum,   S_IWUSR, pmt_show, pmt_sw_hex_store);
static DEVICE_ATTR(s_l4_csum,   S_IWUSR, pmt_show, pmt_sw_hex_store);
static DEVICE_ATTR(s_dec_lsb,   S_IWUSR, pmt_show, pmt_sw_dec_store);
static DEVICE_ATTR(s_dec_msb,   S_IWUSR, pmt_show, pmt_sw_dec_store);
static DEVICE_ATTR(s_add_2b,    S_IWUSR, pmt_show, pmt_sw_hex_store);
static DEVICE_ATTR(s_del,       S_IWUSR, pmt_show, pmt_sw_dec_store);
static DEVICE_ATTR(s_last,      S_IWUSR, pmt_show, pmt_sw_dec_store);
static DEVICE_ATTR(s_flags,     S_IWUSR, pmt_show, pmt_sw_dec_store);
static DEVICE_ATTR(s_skip,      S_IWUSR, pmt_show, pmt_sw_dec_store);
static DEVICE_ATTR(s_jump,      S_IWUSR, pmt_show, pmt_sw_dec_store);


static struct attribute *pmt_attrs[] = {
	&dev_attr_help.attr,
	&dev_attr_sw_dump.attr,
	&dev_attr_sw_clear.attr,
	&dev_attr_hw_regs.attr,
	&dev_attr_hw_write.attr,
	&dev_attr_hw_read.attr,
	&dev_attr_hw_dump.attr,
	&dev_attr_hw_dump_all.attr,
	&dev_attr_hw_inv.attr,
	&dev_attr_hw_inv_all.attr,
	&dev_attr_sw_word.attr,
	&dev_attr_s_rep_2b.attr,
	&dev_attr_s_add_2b.attr,
	&dev_attr_s_del.attr,
	&dev_attr_s_rep_lsb.attr,
	&dev_attr_s_rep_msb.attr,
	&dev_attr_s_ip_csum.attr,
	&dev_attr_s_l4_csum.attr,
	&dev_attr_s_dec_lsb.attr,
	&dev_attr_s_dec_msb.attr,
	&dev_attr_s_last.attr,
	&dev_attr_s_flags.attr,
	&dev_attr_s_skip.attr,
	&dev_attr_s_jump.attr,
	NULL
};

static struct attribute_group pmt_group = {
	.name = "pmt",
	.attrs = pmt_attrs,
};

int mv_neta_pme_sysfs_init(struct kobject *pp2_kobj)
{
	int err;

	err = sysfs_create_group(neta_kobj, &pme_group);
	if (err)
		printk(KERN_INFO "sysfs group %s failed %d\n", pmt_group.name, err);

	return err;
}

int mv_neta_pme_sysfs_exit(struct kobject *neta_kobj)
{
	sysfs_remove_group(neta_kobj, &pme_group);

	return 0;
}

