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

#include "gbe/mvPp2Gbe.h"
#include "gmac/mvEthGmacApi.h"
#include "prs/mvPp2Prs.h"
#include "mv_netdev.h"
#include "mv_eth_sysfs.h"


static ssize_t mv_eth_help(char *buf)
{
	int off = 0;

	off += sprintf(buf+off, "cd                 bm          - move to BM sysfs directory\n");
	off += sprintf(buf+off, "cd                 napi        - move to NAPI groups API sysfs directory\n");
	off += sprintf(buf+off, "cd                 rx          - move to RX sysfs directory\n");
	off += sprintf(buf+off, "cd                 tx          - move to TX sysfs directory\n");
	off += sprintf(buf+off, "cd                 tx_sched    - move to TX Scheduler sysfs directory\n");
	off += sprintf(buf+off, "cd                 pon         - move to PON sysfs directory\n");
	off += sprintf(buf+off, "cd                 pme         - move to PME sysfs directory\n");
	off += sprintf(buf+off, "cd                 qos         - move to QoS sysfs directory\n\n");

#ifdef CONFIG_MV_ETH_HWF
	off += sprintf(buf+off, "cd                 qos         - move to QoS sysfs directory\n\n");
#endif
	off += sprintf(buf+off, "cat                addrDec     - show address decode registers\n");
	off += sprintf(buf+off, "echo [p]         > port        - show port [p] status\n");
	off += sprintf(buf+off, "echo [if_name]   > netdev      - show [if_name] net_device status\n");
	off += sprintf(buf+off, "echo [p]         > cntrs       - show port [p] MIB counters\n");
	off += sprintf(buf+off, "echo [p]         > stats       - show port [p] statistics\n");
	off += sprintf(buf+off, "echo [p]         > gmacRegs    - show GMAC registers for port [p]\n");
	off += sprintf(buf+off, "echo [p]         > isrRegs     - show ISR registers for port [p]\n");
	off += sprintf(buf+off, "echo [p]         > dropCntrs   - show drop counters for port [p]\n");

	off += sprintf(buf+off, "echo [0|1]       > pnc         - enable / disable Parser and Classifier access\n");
	off += sprintf(buf+off, "echo [0|1]       > skb         - enable / disable skb recycle\n");

#ifdef CONFIG_MV_ETH_DEBUG_CODE
	off += sprintf(buf+off, "echo [p] [hex]   > debug       - b0:rx, b1:tx, b2:isr, b3:poll, b4:dump, b5:b_hdr\n");
#endif
	return off;
}

static ssize_t mv_eth_show(struct device *dev,
				  struct device_attribute *attr, char *buf)
{
	const char      *name = attr->attr.name;
	int             off = 0;

	if (!capable(CAP_NET_ADMIN))
		return -EPERM;

	if (!strcmp(name, "addrDec"))
		/*mvPp2AddressDecodeRegsPrint();*/
		mvPp2AddrDecodeRegs();
	else
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
	sscanf(buf, "%d %d", &p, &v);

	local_irq_save(flags);

	if (!strcmp(name, "port")) {
		mv_eth_status_print();
		mv_eth_port_status_print(p);
		mvPp2PortStatus(p);
	} else if (!strcmp(name, "cntrs")) {
		if (!MV_PON_PORT(p))
			mvGmacMibCountersShow(p);
		else
			printk(KERN_ERR "sysfs command %s is not supported for xPON port %d\n",
				name, p);
	} else if (!strcmp(name, "isrRegs")) {
		mvPp2IsrRegs(p);
	} else if (!strcmp(name, "gmacRegs")) {
		mvGmacLmsRegs();
		mvGmacPortRegs(p);
	} else if (!strcmp(name, "dropCntrs")) {
#ifdef CONFIG_MV_ETH_PP2_1
		mvPp2V1DropCntrs(p);
#else
		mvPp2V0DropCntrs(p);
#endif
	} else if (!strcmp(name, "stats")) {
		mv_eth_port_stats_print(p);
	} else if (!strcmp(name, "pnc")) {
		mv_eth_ctrl_pnc(p);
	} else if (!strcmp(name, "skb")) {
		mv_eth_ctrl_recycle(p);
	} else {
		err = 1;
		printk(KERN_ERR "%s: illegal operation <%s>\n", __func__, attr->attr.name);
	}

	local_irq_restore(flags);

	if (err)
		printk(KERN_ERR "%s: error %d\n", __func__, err);

	return err ? -EINVAL : len;
}

static ssize_t mv_eth_2_hex_store(struct device *dev,
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

	if (!strcmp(name, "debug")) {
#ifdef CONFIG_MV_ETH_DEBUG_CODE
		err = mv_eth_ctrl_dbg_flag(p, MV_ETH_F_DBG_RX,   v & 0x1);
		err = mv_eth_ctrl_dbg_flag(p, MV_ETH_F_DBG_TX,   v & 0x2);
		err = mv_eth_ctrl_dbg_flag(p, MV_ETH_F_DBG_ISR,  v & 0x4);
		err = mv_eth_ctrl_dbg_flag(p, MV_ETH_F_DBG_POLL, v & 0x8);
		err = mv_eth_ctrl_dbg_flag(p, MV_ETH_F_DBG_DUMP, v & 0x10);
		err = mv_eth_ctrl_dbg_flag(p, MV_ETH_F_DBG_BUFF_HDR, v & 0x20);
#endif
	}

	return err ? -EINVAL : len;
}

static ssize_t mv_eth_netdev_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t len)
{
	const char *name = attr->attr.name;
	int err = 0;
	char dev_name[IFNAMSIZ];
	struct net_device *netdev;

	if (!capable(CAP_NET_ADMIN))
		return -EPERM;

	sscanf(buf, "%s", dev_name);
	netdev = dev_get_by_name(&init_net, dev_name);
	if (netdev == NULL) {
		printk(KERN_ERR "%s: network interface <%s> doesn't exist\n", __func__, dev_name);
		err = 1;
	} else {
		if (!strcmp(name, "netdev"))
			mv_eth_netdev_print(netdev);
		else {
			err = 1;
			printk(KERN_ERR "%s: illegal operation <%s>\n", __func__, attr->attr.name);
		}
		dev_put(netdev);
	}
	if (err)
		printk(KERN_ERR "%s: error %d\n", __func__, err);

	return err ? -EINVAL : len;
}

static ssize_t mv_eth_reg_store(struct device *dev,
				   struct device_attribute *attr, const char *buf, size_t len)
{
	const char      *name = attr->attr.name;
	int             err;
	unsigned int    r, v;
	unsigned long   flags;

	if (!capable(CAP_NET_ADMIN))
		return -EPERM;

	/* Read port and value */
	err = r = v = 0;
	sscanf(buf, "%x %x", &r, &v);

	local_irq_save(flags);

	if (!strcmp(name, "regRead")) {
		v = mvPp2RdReg(r);
		printk(KERN_INFO "regRead val: 0x%08x\n", v);
	}  else if (!strcmp(name, "regWrite")) {
		mvPp2WrReg(r, v);
	} else {
		err = 1;
		printk(KERN_ERR "%s: illegal operation <%s>\n", __func__, attr->attr.name);
	}

	local_irq_restore(flags);

	if (err)
		printk(KERN_ERR "%s: error %d\n", __func__, err);

	return err ? -EINVAL : len;
}

static DEVICE_ATTR(addrDec,	S_IRUSR, mv_eth_show, NULL);
static DEVICE_ATTR(help,	S_IRUSR, mv_eth_show, NULL);
#ifdef CONFIG_MV_ETH_DEBUG_CODE
static DEVICE_ATTR(debug,	S_IWUSR, NULL, mv_eth_2_hex_store);
#endif
static DEVICE_ATTR(isrRegs,	S_IWUSR, NULL, mv_eth_port_store);
static DEVICE_ATTR(gmacRegs,	S_IWUSR, NULL, mv_eth_port_store);
static DEVICE_ATTR(dropCntrs,	S_IWUSR, NULL, mv_eth_port_store);
static DEVICE_ATTR(stats,       S_IWUSR, NULL, mv_eth_port_store);
static DEVICE_ATTR(pnc,		S_IWUSR, NULL, mv_eth_port_store);
static DEVICE_ATTR(skb,         S_IWUSR, NULL, mv_eth_port_store);

static DEVICE_ATTR(port,	S_IWUSR, NULL, mv_eth_port_store);
static DEVICE_ATTR(cntrs,	S_IWUSR, NULL, mv_eth_port_store);
static DEVICE_ATTR(netdev,	S_IWUSR, NULL, mv_eth_netdev_store);

static DEVICE_ATTR(regRead,       S_IWUSR, NULL, mv_eth_reg_store);
static DEVICE_ATTR(regWrite,      S_IWUSR, NULL, mv_eth_reg_store);

static struct attribute *mv_eth_attrs[] = {
	&dev_attr_addrDec.attr,
	&dev_attr_help.attr,
#ifdef CONFIG_MV_ETH_DEBUG_CODE
	&dev_attr_debug.attr,
#endif
	&dev_attr_port.attr,
	&dev_attr_cntrs.attr,
	&dev_attr_netdev.attr,
	&dev_attr_isrRegs.attr,
	&dev_attr_gmacRegs.attr,
	&dev_attr_dropCntrs.attr,
	&dev_attr_stats.attr,
	&dev_attr_pnc.attr,
	&dev_attr_skb.attr,
	&dev_attr_regRead.attr,
	&dev_attr_regWrite.attr,
	NULL
};

static struct attribute_group mv_eth_group = {
	.attrs = mv_eth_attrs,
};

static struct kobject *gbe_kobj;

int mv_pp2_gbe_sysfs_init(struct kobject *pp2_kobj)
{
	int err;

	gbe_kobj = kobject_create_and_add("gbe", pp2_kobj);
	if (!gbe_kobj) {
		printk(KERN_ERR"%s: cannot create gbe kobject\n", __func__);
		return -ENOMEM;
	}

	err = sysfs_create_group(gbe_kobj, &mv_eth_group);
	if (err) {
		printk(KERN_INFO "sysfs group failed %d\n", err);
		return err;
	}

	mv_pp2_bm_sysfs_init(gbe_kobj);
	mv_pp2_napi_sysfs_init(gbe_kobj);
	mv_pp2_rx_sysfs_init(gbe_kobj);
	mv_pp2_tx_sysfs_init(gbe_kobj);
	mv_pp2_tx_sched_sysfs_init(gbe_kobj);
	mv_pp2_qos_sysfs_init(gbe_kobj);
	mv_pp2_pon_sysfs_init(gbe_kobj);
	mv_pp2_gbe_pme_sysfs_init(gbe_kobj);
#ifdef CONFIG_MV_ETH_HWF
	mv_pp2_gbe_hwf_sysfs_init(gbe_kobj);
#endif
	return err;
}

int mv_pp2_gbe_sysfs_exit(struct kobject *pp2_kobj)
{
	mv_pp2_gbe_pme_sysfs_exit(gbe_kobj);
	mv_pp2_pon_sysfs_exit(gbe_kobj);
	mv_pp2_qos_sysfs_exit(gbe_kobj);
	mv_pp2_tx_sched_sysfs_exit(gbe_kobj);
	mv_pp2_tx_sysfs_exit(gbe_kobj);
	mv_pp2_rx_sysfs_exit(gbe_kobj);
	mv_pp2_napi_sysfs_exit(gbe_kobj);
	mv_pp2_bm_sysfs_exit(gbe_kobj);
#ifdef CONFIG_MV_ETH_HWF
	mv_pp2_gbe_hwf_sysfs_exit(gbe_kobj);
#endif
	sysfs_remove_group(pp2_kobj, &mv_eth_group);

	return 0;
}
