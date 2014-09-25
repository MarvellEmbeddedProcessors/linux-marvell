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
#include <linux/platform_device.h>
#include <linux/netdevice.h>

#include "mv_eth_sysfs.h"
#include "mv_netdev.h"


static ssize_t mv_eth_help(char *b)
{
	int o = 0; /* buffer offset */
	int s = PAGE_SIZE; /* buffer size */

	o += scnprintf(b+o, s-o, "p, txp, txq, cpu, d                   - are dec numbers\n");
	o += scnprintf(b+o, s-o, "v, mask                               - are hex numbers\n");
	o += scnprintf(b+o, s-o, "\n");

	o += scnprintf(b+o, s-o, "echo p txp         > txp_regs      - show TX registers for <p/txp>\n");
	o += scnprintf(b+o, s-o, "echo p txp txq     > txq_regs      - show TXQ registers for <p/txp/txq>\n");
	o += scnprintf(b+o, s-o, "echo p txp txq d   > txq           - show TXQ descriptors ring for <p/txp/txq>. d=0-brief, d=1-full\n");
	o += scnprintf(b+o, s-o, "echo p txp         > txp_reset     - reset TX part of the port <p/txp>\n");
	o += scnprintf(b+o, s-o, "echo p txp txq     > txq_clean     - clean TXQ <p/txp/txq> - free descriptors and buffers\n");
	o += scnprintf(b+o, s-o, "echo p txp txq cpu > txq_def       - set default <txp/txq> for packets sent to port <p> by <cpu>\n");
	o += scnprintf(b+o, s-o, "echo p txp txq d   > txq_size      - set number of descriptors <d> for <port/txp/txq>.\n");
	o += scnprintf(b+o, s-o, "echo p txp txq d   > txq_coal      - set TXP/TXQ interrupt coalesing. <d> - number of sent packets\n");
	o += scnprintf(b+o, s-o, "echo p cpu mask    > txq_mask      - set cpu <cpu> accessible txq bitmap <mask>.\n");
	o += scnprintf(b+o, s-o, "echo p txp txq d   > txq_shared    - set/reset shared bit for <port/txp/txq>. <d> - 1/0 for set/reset.\n");
	o += scnprintf(b+o, s-o, "echo d             > tx_done       - set threshold <d> to start tx_done operations\n");
	o += scnprintf(b+o, s-o, "echo p {0|1}       > mh_en         - enable Marvell Header\n");
	o += scnprintf(b+o, s-o, "echo p {0|1}       > tx_nopad      - disable zero padding on transmit\n");
	o += scnprintf(b+o, s-o, "echo p v           > tx_mh_2B      - set 2 bytes of Marvell Header for transmit\n");
	o += scnprintf(b+o, s-o, "echo p v           > tx_cmd        - set 4 bytes of TX descriptor offset 0xc\n");
#ifdef CONFIG_MV_NETA_TXDONE_IN_HRTIMER
	o += scnprintf(b+o, s-o, "echo period        > tx_period     - set Tx Done high resolution timer period\n");
	o += scnprintf(b+o, s-o, "					period: period range is [%u, %u], unit usec\n",
								MV_ETH_HRTIMER_PERIOD_MIN, MV_ETH_HRTIMER_PERIOD_MAX);
#endif

	return o;
}

static ssize_t mv_eth_show(struct device *dev,
				  struct device_attribute *attr, char *buf)
{
	int             off = 0;

	if (!capable(CAP_NET_ADMIN))
		return -EPERM;

	off = mv_eth_help(buf);

	return off;
}

static ssize_t mv_eth_port_store(struct device *dev,
				   struct device_attribute *attr, const char *buf, size_t len)
{
	const char      *name = attr->attr.name;
	int             err;
	unsigned int    p, v;
	unsigned long   flags;

	if (!capable(CAP_NET_ADMIN))
		return -EPERM;

	/* Read port and value */
	err = p = v = 0;
	sscanf(buf, "%d %x", &p, &v);

	local_irq_save(flags);

	if (!strcmp(name, "tx_cmd")) {
		err = mv_eth_ctrl_tx_cmd(p, v);
	} else if (!strcmp(name, "mh_en")) {
		err = mv_eth_ctrl_flag(p, MV_ETH_F_MH, v);
	} else if (!strcmp(name, "tx_mh_2B")) {
		err = mv_eth_ctrl_tx_mh(p, MV_16BIT_BE((u16)v));
	} else if (!strcmp(name, "tx_nopad")) {
		err = mv_eth_ctrl_flag(p, MV_ETH_F_NO_PAD, v);
	} else {
		err = 1;
		pr_err("%s: illegal operation <%s>\n", __func__, attr->attr.name);
	}

	local_irq_restore(flags);

	if (err)
		pr_err("%s: error %d\n", __func__, err);

	return err ? -EINVAL : len;
}

static ssize_t mv_eth_3_store(struct device *dev,
				   struct device_attribute *attr, const char *buf, size_t len)
{
	const char      *name = attr->attr.name;
	int             err;
	unsigned int    p, i, v;
	unsigned long   flags;

	if (!capable(CAP_NET_ADMIN))
		return -EPERM;

	err = p = i = v = 0;
	sscanf(buf, "%d %d %d", &p, &i, &v);

	local_irq_save(flags);

	if (!strcmp(name, "txp_reset")) {
		err = mv_eth_txp_reset(p, i);
	} else if (!strcmp(name, "tx_done")) {
		mv_eth_ctrl_txdone(p);
	} else if (!strcmp(name, "tx_period")) {
#ifdef CONFIG_MV_NETA_TXDONE_IN_HRTIMER
		err = mv_eth_tx_done_hrtimer_period_set(p);
#endif
	} else {
		err = 1;
		pr_err("%s: illegal operation <%s>\n", __func__, attr->attr.name);
	}

	local_irq_restore(flags);

	if (err)
		pr_err("%s: error %d\n", __func__, err);

	return err ? -EINVAL : len;
}

static ssize_t mv_eth_3_hex_store(struct device *dev,
				   struct device_attribute *attr, const char *buf, size_t len)
{
	const char      *name = attr->attr.name;
	int             err;
	unsigned int    p, i, v;
	unsigned long   flags;

	if (!capable(CAP_NET_ADMIN))
		return -EPERM;

	err = p = i = v = 0;
	sscanf(buf, "%d %d %x", &p, &i, &v);

	local_irq_save(flags);

	if (!strcmp(name, "txq_mask")) {
		err = mv_eth_cpu_txq_mask_set(p, i, v);
	} else {
		err = 1;
		pr_err("%s: illegal operation <%s>\n", __func__, attr->attr.name);
	}

	local_irq_restore(flags);

	return err ? -EINVAL : len;
}

static ssize_t mv_eth_4_store(struct device *dev,
				   struct device_attribute *attr, const char *buf, size_t len)
{
	const char      *name = attr->attr.name;
	int             err;
	unsigned int    p, txp, txq, v;
	unsigned long   flags;

	if (!capable(CAP_NET_ADMIN))
		return -EPERM;

	err = p = txp = txq = v = 0;
	sscanf(buf, "%d %d %d %d", &p, &txp, &txq, &v);

	local_irq_save(flags);

	if (!strcmp(name, "txq_def")) {
		err = mv_eth_ctrl_txq_cpu_def(p, txp, txq, v);
	} else if (!strcmp(name, "txp_regs")) {
		mvNetaTxpRegs(p, txp);
	} else if (!strcmp(name, "txq_size")) {
		err = mv_eth_ctrl_txq_size_set(p, txp, txq, v);
	} else if (!strcmp(name, "txq_coal")) {
		mv_eth_tx_done_pkts_coal_set(p, txp, txq, v);
	} else if (!strcmp(name, "txq")) {
		mvNetaTxqShow(p, txp, txq, v);
	} else if (!strcmp(name, "txq_regs")) {
		mvNetaTxqRegs(p, txp, txq);
	} else if (!strcmp(name, "txq_clean")) {
		err = mv_eth_txq_clean(p, txp, txq);
	} else if (!strcmp(name, "txq_shared")) {
		err = mv_eth_shared_set(p, txp, txq, v);
	} else {
		err = 1;
		pr_err("%s: illegal operation <%s>\n", __func__, attr->attr.name);
	}
	local_irq_restore(flags);

	if (err)
		pr_err("%s: error %d\n", __func__, err);

	return err ? -EINVAL : len;
}

static DEVICE_ATTR(help,           S_IRUSR, mv_eth_show, NULL);
static DEVICE_ATTR(txp_regs,       S_IWUSR, NULL, mv_eth_4_store);
static DEVICE_ATTR(txq_regs,       S_IWUSR, NULL, mv_eth_4_store);
static DEVICE_ATTR(txq,            S_IWUSR, NULL, mv_eth_4_store);
static DEVICE_ATTR(tx_mh_2B,       S_IWUSR, NULL, mv_eth_port_store);
static DEVICE_ATTR(tx_cmd,         S_IWUSR, NULL, mv_eth_port_store);
static DEVICE_ATTR(txp_reset,      S_IWUSR, NULL, mv_eth_3_store);
static DEVICE_ATTR(txq_clean,      S_IWUSR, NULL, mv_eth_4_store);
static DEVICE_ATTR(txq_def,        S_IWUSR, NULL, mv_eth_4_store);
static DEVICE_ATTR(txq_size,       S_IWUSR, NULL, mv_eth_4_store);
static DEVICE_ATTR(txq_coal,       S_IWUSR, NULL, mv_eth_4_store);
static DEVICE_ATTR(mh_en,          S_IWUSR, NULL, mv_eth_port_store);
static DEVICE_ATTR(tx_done,        S_IWUSR, NULL, mv_eth_3_store);
#ifdef CONFIG_MV_NETA_TXDONE_IN_HRTIMER
static DEVICE_ATTR(tx_period,      S_IWUSR, NULL, mv_eth_3_store);
#endif
static DEVICE_ATTR(txq_mask,       S_IWUSR, NULL, mv_eth_3_hex_store);
static DEVICE_ATTR(txq_shared,     S_IWUSR, NULL, mv_eth_4_store);
static DEVICE_ATTR(tx_nopad,       S_IWUSR, NULL, mv_eth_port_store);

static struct attribute *mv_eth_tx_attrs[] = {
	&dev_attr_help.attr,
	&dev_attr_txp_regs.attr,
	&dev_attr_txq_regs.attr,
	&dev_attr_txq.attr,
	&dev_attr_tx_mh_2B.attr,
	&dev_attr_tx_cmd.attr,
	&dev_attr_txp_reset.attr,
	&dev_attr_txq_clean.attr,
	&dev_attr_txq_def.attr,
	&dev_attr_txq_size.attr,
	&dev_attr_txq_coal.attr,
	&dev_attr_mh_en.attr,
	&dev_attr_tx_done.attr,
#ifdef CONFIG_MV_NETA_TXDONE_IN_HRTIMER
	&dev_attr_tx_period.attr,
#endif
	&dev_attr_txq_mask.attr,
	&dev_attr_txq_shared.attr,
	&dev_attr_tx_nopad.attr,
	NULL
};

static struct attribute_group mv_eth_tx_group = {
	.name = "tx",
	.attrs = mv_eth_tx_attrs,
};

int mv_neta_tx_sysfs_init(struct kobject *gbe_kobj)
{
	int err;

	err = sysfs_create_group(gbe_kobj, &mv_eth_tx_group);
	if (err)
		pr_err("sysfs group %s failed %d\n", mv_eth_tx_group.name, err);

	return err;
}

int mv_neta_tx_sysfs_exit(struct kobject *gbe_kobj)
{
	sysfs_remove_group(gbe_kobj, &mv_eth_tx_group);

	return 0;
}

