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
#include <linux/platform_device.h>

#include "mv_mux_netdev.h"
MV_MUX_TAG mux_cfg;

static ssize_t mv_mux_help(char *b)
{
	int o = 0;
	o += sprintf(b+o, "echo p             > dump         - Show gbe port [p] info\n");
	o += sprintf(b+o, "echo name          > mux_dump     - Show virt interface device info\n");
	o += sprintf(b+o, "echo name p        > add          - Attach to gbe port [p] new virtual interface\n");
	o += sprintf(b+o, "echo name          > del          - Remove virt interface\n");
	o += sprintf(b+o, "\n");
	o += sprintf(b+o, "echo p tag         > tag_type     - Set port p tag type 0-NONE,1-MH,2-DSA,3-EDSA,4-VID\n");
	o += sprintf(b+o, "echo name vid      > mux_vid      - Set virt interface vid value.\n");
	o += sprintf(b+o, "echo name mh       > mh_tx        - Set virt interface MH tX tag\n");
	o += sprintf(b+o, "echo name mh mask  > mh_rx        - Set virt interface MH RX tag and mask\n");
	o += sprintf(b+o, "echo name dsa      > dsa_tx       - Set virt interface DSA TX tag\n");
	o += sprintf(b+o, "echo name dsa mask > dsa_rx       - Set virt interface DSA RX tag and mask\n");
	o += sprintf(b+o, "echo name wL wH    > edsa_tx      - Set virt interface EDSA TX tag\n");
	o += sprintf(b+o, "echo name wL wH    > edsa_rx      - Set virt interface EDSA RX tag\n");
	o += sprintf(b+o, "echo name wL wH    > edsa_rx_mask - Set virt interface EDSA RX mask tag\n");

#ifdef CONFIG_MV_ETH_DEBUG_CODE
	o += sprintf(b+o, "echo p hex         > debug        - bit0:rx, bit1:tx\n");
#endif
	o += sprintf(b+o, "\n");
	o += sprintf(b+o, "params: name-interface name,  mh-2 bytes value(hex), dsa,edsa,vid-4 bytes value(hex)\n");

	return o;
}


static ssize_t mv_mux_show(struct device *dev,
				  struct device_attribute *attr, char *buf)
{
/*
	const char      *name = attr->attr.name;
	int             off = 0;
*/
	if (!capable(CAP_NET_ADMIN))
		return -EPERM;

	return mv_mux_help(buf);
}


static ssize_t mv_mux_netdev_store(struct device *dev,
					struct device_attribute *attr, const char *buf, size_t len)
{
	struct net_device *mux_dev;
	const char        *name = attr->attr.name;
	int               a = 0, b = 0, err = 0;
	char              dev_name[IFNAMSIZ];

	if (!capable(CAP_NET_ADMIN))
		return -EPERM;

	sscanf(buf, "%s %x %x", dev_name, &a, &b);
	mux_dev = dev_get_by_name(&init_net, dev_name);

	if (mux_dev)
		dev_put(mux_dev);

	if (!strcmp(name, "mux_dump")) {
		mv_mux_netdev_print(mux_dev);

	} else if (!strcmp(name, "mux_vid")) {
		mv_mux_vlan_set(&mux_cfg, a);
		err = mv_mux_netdev_alloc(dev_name, -1, &mux_cfg) ? 0 : 1;

	} else if (!strcmp(name, "mh_rx")) {
		mv_mux_cfg_get(mux_dev, &mux_cfg);
		mux_cfg.rx_tag_ptrn.mh = a;
		mux_cfg.rx_tag_mask.mh = b;
		err = mv_mux_netdev_alloc(dev_name, -1, &mux_cfg) ? 0 : 1;

	} else if (!strcmp(name, "dsa_rx")) {
		mv_mux_cfg_get(mux_dev, &mux_cfg);
		mux_cfg.rx_tag_ptrn.dsa = a;
		mux_cfg.rx_tag_mask.dsa = b;
		err = mv_mux_netdev_alloc(dev_name, -1, &mux_cfg) ? 0 : 1;

	} else if (!strcmp(name, "edsa_rx")) {
		mv_mux_cfg_get(mux_dev, &mux_cfg);
		mux_cfg.rx_tag_ptrn.edsa[0] = a;
		mux_cfg.rx_tag_ptrn.edsa[1] = b;
		err = mv_mux_netdev_alloc(dev_name, -1, &mux_cfg) ? 0 : 1;

	} else if (!strcmp(name, "edsa_rx_mask")) {
		mv_mux_cfg_get(mux_dev, &mux_cfg);
		mux_cfg.rx_tag_mask.edsa[0] = a;
		mux_cfg.rx_tag_mask.edsa[1] = b;
		err = mv_mux_netdev_alloc(dev_name, -1, &mux_cfg) ? 0 : 1;

	}  else if (!strcmp(name, "mh_tx")) {
		mv_mux_cfg_get(mux_dev, &mux_cfg);
		mux_cfg.tx_tag.mh = a;
		err = mv_mux_netdev_alloc(dev_name, -1, &mux_cfg) ? 0 : 1;

	}  else if (!strcmp(name, "dsa_tx")) {
		mv_mux_cfg_get(mux_dev, &mux_cfg);
		mux_cfg.tx_tag.dsa = a;
		err = mv_mux_netdev_alloc(dev_name, -1, &mux_cfg) ? 0 : 1;

	} else if (!strcmp(name, "edsa_tx")) {
		mv_mux_cfg_get(mux_dev, &mux_cfg);
		mux_cfg.tx_tag.edsa[0] = a;
		mux_cfg.tx_tag.edsa[1] = b;
		err = mv_mux_netdev_alloc(dev_name, -1, &mux_cfg) ? 0 : 1;

	} else if (!strcmp(name, "add")) {
		err =  mv_mux_netdev_add(a, mux_dev) ? 0 : 1;

	} else if (!strcmp(name, "del"))
		err = mv_mux_netdev_delete(mux_dev);

	if (err)
		printk(KERN_ERR "%s: error %d\n", __func__, err);

	return err ? -EINVAL : len;
}

static ssize_t mv_mux_store(struct device *dev,
				   struct device_attribute *attr, const char *buf, size_t len)
{
	const char      *name = attr->attr.name;
	int             err;
	unsigned int    a, b;
	unsigned long   flags;

	if (!capable(CAP_NET_ADMIN))
		return -EPERM;

	a = b = err = 0;

	sscanf(buf, "%x %x", &a, &b);

	local_irq_save(flags);

	if (!strcmp(name, "tag_type")) {
		mv_mux_tag_type_set(a, b);

	} else if (!strcmp(name, "dump")) {
		mv_mux_shadow_print(a);

#ifdef CONFIG_MV_ETH_DEBUG_CODE
	} else if (!strcmp(name, "debug")) {
		err = mv_mux_ctrl_dbg_flag(a, MV_MUX_F_DBG_RX,   b & 0x1);
		err = mv_mux_ctrl_dbg_flag(a, MV_MUX_F_DBG_TX,   b & 0x2);
#endif
	} else {
		err = 1;
		printk(KERN_ERR "%s: illegal operation <%s>\n", __func__, attr->attr.name);
	}

	local_irq_restore(flags);

	if (err)
		printk(KERN_ERR "%s: error %d\n", __func__, err);

	return err ? -EINVAL : len;
}
static DEVICE_ATTR(add,          S_IWUSR, mv_mux_show, mv_mux_netdev_store);
static DEVICE_ATTR(del,          S_IWUSR, mv_mux_show, mv_mux_netdev_store);
static DEVICE_ATTR(mux_vid,      S_IWUSR, mv_mux_show, mv_mux_netdev_store);
static DEVICE_ATTR(mh_rx,        S_IWUSR, mv_mux_show, mv_mux_netdev_store);
static DEVICE_ATTR(dsa_rx,       S_IWUSR, mv_mux_show, mv_mux_netdev_store);
static DEVICE_ATTR(edsa_rx,      S_IWUSR, mv_mux_show, mv_mux_netdev_store);
static DEVICE_ATTR(edsa_rx_mask, S_IWUSR, mv_mux_show, mv_mux_netdev_store);
static DEVICE_ATTR(mh_tx,        S_IWUSR, mv_mux_show, mv_mux_netdev_store);
static DEVICE_ATTR(dsa_tx,       S_IWUSR, mv_mux_show, mv_mux_netdev_store);
static DEVICE_ATTR(edsa_tx,      S_IWUSR, mv_mux_show, mv_mux_netdev_store);
static DEVICE_ATTR(tag_type,     S_IWUSR, mv_mux_show, mv_mux_store);
static DEVICE_ATTR(dump,         S_IWUSR, mv_mux_show, mv_mux_store);
static DEVICE_ATTR(debug,        S_IWUSR, mv_mux_show, mv_mux_store);
static DEVICE_ATTR(mux_dump,     S_IWUSR, mv_mux_show, mv_mux_netdev_store);
static DEVICE_ATTR(help,         S_IRUSR, mv_mux_show, NULL);


static struct attribute *mv_mux_attrs[] = {

	&dev_attr_add.attr,
	&dev_attr_del.attr,
	&dev_attr_mux_vid.attr,
	&dev_attr_mh_rx.attr,
	&dev_attr_dsa_rx.attr,
	&dev_attr_edsa_rx.attr,
	&dev_attr_edsa_rx_mask.attr,
	&dev_attr_mh_tx.attr,
	&dev_attr_dsa_tx.attr,
	&dev_attr_edsa_tx.attr,
	&dev_attr_tag_type.attr,
	&dev_attr_dump.attr,
	&dev_attr_help.attr,
	&dev_attr_mux_dump.attr,
	&dev_attr_debug.attr,
	NULL
};

static struct attribute_group mv_mux_group = {
	.name = "mv_mux",
	.attrs = mv_mux_attrs,
};

int __init mv_mux_sysfs_init(void)
{
	int err;
	struct device *pd;

	pd = &platform_bus;
	err = sysfs_create_group(&pd->kobj, &mv_mux_group);
	if (err)
		pr_err("Init sysfs group %s failed %d\n", mv_mux_group.name, err);

	return err;
}


module_init(mv_mux_sysfs_init);

MODULE_AUTHOR("Uri Eliyahu");
MODULE_DESCRIPTION("sysfs for marvell GbE");
MODULE_LICENSE("GPL");
