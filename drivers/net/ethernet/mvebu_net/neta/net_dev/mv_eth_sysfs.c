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
#include <linux/netdevice.h>

#include "gbe/mvNeta.h"
#include "mv_netdev.h"
#include "mv_eth_sysfs.h"

static ssize_t mv_eth_help(char *b)
{
	int o = 0; /* buffer offset */
	int s = PAGE_SIZE; /* buffer size */

	o += scnprintf(b+o, s-o, "p, txp, d, l, s                    - are dec numbers\n");
	o += scnprintf(b+o, s-o, "v                                  - are hex numbers\n");
	o += scnprintf(b+o, s-o, "\n");

	o += scnprintf(b+o, s-o, "cat                ports           - show all ports info\n");
	o += scnprintf(b+o, s-o, "cd                 rx              - move to RX sysfs directory\n");
	o += scnprintf(b+o, s-o, "cd                 tx              - move to TX sysfs directory\n");
	o += scnprintf(b+o, s-o, "cd                 tx_sched        - move to TX Scheduler sysfs directory\n");
	o += scnprintf(b+o, s-o, "cd                 qos             - move to QoS sysfs directory\n");
	o += scnprintf(b+o, s-o, "cd                 rss             - move to RSS sysfs directory\n");
	o += scnprintf(b+o, s-o, "echo p d           > stack         - show pools stack for port <p>. d=0-brief, d=1-full\n");
	o += scnprintf(b+o, s-o, "echo p             > port          - show a port info\n");
	o += scnprintf(b+o, s-o, "echo [if_name]     > netdev        - show <if_name> net_device status\n");
	o += scnprintf(b+o, s-o, "echo p             > stats         - show a port statistics\n");
	o += scnprintf(b+o, s-o, "echo p txp         > cntrs         - show a port counters\n");
	o += scnprintf(b+o, s-o, "echo p             > mac           - show MAC info for port <p>\n");
	o += scnprintf(b+o, s-o, "echo p             > p_regs        - show port registers for <p>\n");
#ifdef MV_ETH_GMAC_NEW
	o += scnprintf(b+o, s-o, "echo p             > gmac_regs     - show gmac registers for <p>\n");
#endif /* MV_ETH_GMAC_NEW */
#ifdef CONFIG_MV_ETH_PNC
	if (MV_NETA_PNC_CAP())
		o += scnprintf(b+o, s-o, "echo {0|1}         > pnc           - enable / disable PNC access\n");
#endif /* CONFIG_MV_ETH_PNC */
	o += scnprintf(b+o, s-o, "echo {0|1}         > skb           - enable / disable SKB recycle\n");
	o += scnprintf(b+o, s-o, "echo p v           > debug         - bit0:rx, bit1:tx, bit2:isr, bit3:poll, bit4:dump\n");
	o += scnprintf(b+o, s-o, "echo p l s         > buf_num       - set number of long <l> and short <s> buffers allocated for port <p>\n");
	o += scnprintf(b+o, s-o, "echo p wol         > pm_mode       - set port <p> pm mode. 1 wol, 0 suspend.\n");

	return o;
}

static ssize_t mv_eth_show(struct device *dev,
				  struct device_attribute *attr, char *buf)
{
	const char      *name = attr->attr.name;
	unsigned int    p;
	int             off = 0;

	if (!capable(CAP_NET_ADMIN))
		return -EPERM;

	if (!strcmp(name, "ports")) {
		mv_eth_status_print();

		for (p = 0; p <= CONFIG_MV_ETH_PORTS_NUM; p++)
			mv_eth_port_status_print(p);
	} else {
		off = mv_eth_help(buf);
	}

	return off;
}

static ssize_t mv_eth_netdev_store(struct device *dev,
				struct device_attribute *attr, const char *buf, size_t len)
{
	const char        *name = attr->attr.name;
	int               err = 0;
	char              dev_name[IFNAMSIZ];
	struct net_device *netdev;

	if (!capable(CAP_NET_ADMIN))
		return -EPERM;

	sscanf(buf, "%s", dev_name);
	netdev = dev_get_by_name(&init_net, dev_name);
	if (netdev == NULL) {
		pr_err("%s: network interface <%s> doesn't exist\n",
			__func__, dev_name);
		err = 1;
	} else {
		if (!strcmp(name, "netdev"))
			mv_eth_netdev_print(netdev);
		else {
			err = 1;
			pr_err("%s: illegal operation <%s>\n", __func__, attr->attr.name);
		}
		dev_put(netdev);
	}
	if (err)
		pr_err("%s: error %d\n", __func__, err);

	return err ? -EINVAL : len;
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

	if (!strcmp(name, "debug")) {
		err = mv_eth_ctrl_flag(p, MV_ETH_F_DBG_RX,   v & 0x1);
		err = mv_eth_ctrl_flag(p, MV_ETH_F_DBG_TX,   v & 0x2);
		err = mv_eth_ctrl_flag(p, MV_ETH_F_DBG_ISR,  v & 0x4);
		err = mv_eth_ctrl_flag(p, MV_ETH_F_DBG_POLL, v & 0x8);
		err = mv_eth_ctrl_flag(p, MV_ETH_F_DBG_DUMP, v & 0x10);
	} else if (!strcmp(name, "skb")) {
		mv_eth_ctrl_recycle(p);
	} else if (!strcmp(name, "port")) {
		mv_eth_status_print();
		mvNetaPortStatus(p);
		mv_eth_port_status_print(p);
	} else if (!strcmp(name, "stack")) {
		mv_eth_stack_print(p, v);
	} else if (!strcmp(name, "stats")) {
		mv_eth_port_stats_print(p);
	} else if (!strcmp(name, "mac")) {
		mv_eth_mac_show(p);
	} else if (!strcmp(name, "p_regs")) {
		pr_info("\n[NetA Port: port=%d]\n", p);
		mvEthRegs(p);
		pr_info("\n");
		mvEthPortRegs(p);
		mvNetaPortRegs(p);
#ifdef MV_ETH_GMAC_NEW
	} else if (!strcmp(name, "gmac_regs")) {
		mvNetaGmacRegs(p);
#endif /* MV_ETH_GMAC_NEW */
#ifdef CONFIG_MV_ETH_PNC
	} else if (!strcmp(name, "pnc")) {
		mv_eth_ctrl_pnc(p);
#endif /* CONFIG_MV_ETH_PNC */
	} else if (!strcmp(name, "pm_mode")) {
		err = mv_eth_wol_mode_set(p, v);
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

	if (!strcmp(name, "buf_num")) {
		err = mv_eth_ctrl_port_buf_num_set(p, i, v);
	} else {
		err = 1;
		pr_err("%s: illegal operation <%s>\n", __func__, attr->attr.name);
	}

	local_irq_restore(flags);

	if (err)
		pr_err("%s: error %d\n", __func__, err);

	return err ? -EINVAL : len;
}

static ssize_t mv_eth_2_store(struct device *dev,
				   struct device_attribute *attr, const char *buf, size_t len)
{
	const char      *name = attr->attr.name;
	int             err;
	unsigned int    p, txp;
	unsigned long   flags;

	if (!capable(CAP_NET_ADMIN))
		return -EPERM;

	err = p = txp = 0;
	sscanf(buf, "%d %d", &p, &txp);

	local_irq_save(flags);

	if (!strcmp(name, "cntrs")) {
		mvEthPortCounters(p, txp);
		mvEthPortRmonCounters(p, txp);
	} else {
		err = 1;
		pr_err("%s: illegal operation <%s>\n", __func__, attr->attr.name);
	}
	local_irq_restore(flags);

	if (err)
		pr_err("%s: error %d\n", __func__, err);

	return err ? -EINVAL : len;
}

static DEVICE_ATTR(buf_num,     S_IWUSR, mv_eth_show, mv_eth_3_store);
static DEVICE_ATTR(debug,       S_IWUSR, mv_eth_show, mv_eth_port_store);
static DEVICE_ATTR(cntrs,       S_IWUSR, mv_eth_show, mv_eth_2_store);
static DEVICE_ATTR(port,        S_IWUSR, mv_eth_show, mv_eth_port_store);
static DEVICE_ATTR(stack,        S_IWUSR, mv_eth_show, mv_eth_port_store);
static DEVICE_ATTR(mac,         S_IWUSR, mv_eth_show, mv_eth_port_store);
static DEVICE_ATTR(stats,       S_IWUSR, mv_eth_show, mv_eth_port_store);
static DEVICE_ATTR(skb,	        S_IWUSR, mv_eth_show, mv_eth_port_store);
static DEVICE_ATTR(ports,       S_IRUSR, mv_eth_show, NULL);
static DEVICE_ATTR(help,        S_IRUSR, mv_eth_show, NULL);
static DEVICE_ATTR(p_regs,      S_IWUSR, mv_eth_show, mv_eth_port_store);
static DEVICE_ATTR(gmac_regs,   S_IWUSR, mv_eth_show, mv_eth_port_store);
#ifdef CONFIG_MV_ETH_PNC
static DEVICE_ATTR(pnc,         S_IWUSR, NULL, mv_eth_port_store);
#endif /* CONFIG_MV_ETH_PNC */
static DEVICE_ATTR(pm_mode,	S_IWUSR, mv_eth_show, mv_eth_port_store);
static DEVICE_ATTR(netdev,       S_IWUSR, NULL, mv_eth_netdev_store);

static struct attribute *mv_eth_attrs[] = {

	&dev_attr_buf_num.attr,
	&dev_attr_debug.attr,
	&dev_attr_port.attr,
	&dev_attr_stack.attr,
	&dev_attr_stats.attr,
	&dev_attr_cntrs.attr,
	&dev_attr_ports.attr,
	&dev_attr_netdev.attr,
	&dev_attr_mac.attr,
	&dev_attr_skb.attr,
	&dev_attr_p_regs.attr,
	&dev_attr_gmac_regs.attr,
	&dev_attr_help.attr,
#ifdef CONFIG_MV_ETH_PNC
    &dev_attr_pnc.attr,
#endif /* CONFIG_MV_ETH_PNC */
	&dev_attr_pm_mode.attr,
	NULL
};

static struct attribute_group mv_eth_group = {
	.attrs = mv_eth_attrs,
};

static struct kobject *gbe_kobj;

int mv_neta_gbe_sysfs_init(struct kobject *neta_kobj)
{
	int err;

	gbe_kobj = kobject_create_and_add("gbe", neta_kobj);
	if (!gbe_kobj) {
		pr_err("%s: cannot create gbe kobject\n", __func__);
		return -ENOMEM;
	}

	err = sysfs_create_group(gbe_kobj, &mv_eth_group);
	if (err) {
		pr_info("sysfs group failed %d\n", err);
		return err;
	}

	err = mv_neta_rx_sysfs_init(gbe_kobj);
	if (err) {
		pr_info("sysfs group failed %d\n", err);
		return err;
	}

	err = mv_neta_tx_sysfs_init(gbe_kobj);
	if (err) {
		pr_info("sysfs group failed %d\n", err);
		return err;
	}

	err = mv_neta_tx_sched_sysfs_init(gbe_kobj);
	if (err) {
		pr_info("sysfs group failed %d\n", err);
		return err;
	}

	err = mv_neta_qos_sysfs_init(gbe_kobj);
	if (err) {
		pr_info("sysfs group failed %d\n", err);
		return err;
	}

	err = mv_neta_rss_sysfs_init(gbe_kobj);
	if (err) {
		pr_info("sysfs group failed %d\n", err);
		return err;
	}

	return err;
}

int mv_neta_gbe_sysfs_exit(struct kobject *neta_kobj)
{
	sysfs_remove_group(neta_kobj, &mv_eth_group);

	return 0;
}
