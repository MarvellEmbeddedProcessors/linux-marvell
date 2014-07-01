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

	o += scnprintf(b+o, s-o, "p, txq, rxq, cpu                      - are dec numbers\n");
	o += scnprintf(b+o, s-o, "v, tos                                - are hex numbers\n");
	o += scnprintf(b+o, s-o, "\n");

	o += scnprintf(b+o, s-o, "echo p             > tos           - show RX and TX TOS map for port <p>\n");
	o += scnprintf(b+o, s-o, "echo p             > vprio         - show VLAN priority map for port <p>\n");
	o += scnprintf(b+o, s-o, "echo p rxq tos     > rxq_tos       - set <rxq> for incoming IP packets with <tos>\n");
	o += scnprintf(b+o, s-o, "echo p rxq t       > rxq_type      - set RXQ for different packet types. t=0-bpdu, 1-arp, 2-tcp, 3-udp\n");
	o += scnprintf(b+o, s-o, "echo p rxq prio    > rxq_vlan      - set <rxq> for incoming VLAN packets with <prio>\n");
	o += scnprintf(b+o, s-o, "echo p txq cpu tos > txq_tos       - set <txq> for outgoing IP packets with <tos> handeled by <cpu>\n");

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

#ifdef CONFIG_MV_ETH_PNC
int run_rxq_type(int port, int q, int t)
{
	void *port_hndl = mvNetaPortHndlGet(port);

	if (port_hndl == NULL)
		return 1;

	if (!mv_eth_pnc_ctrl_en) {
		pr_err("%s: PNC control is not supported\n", __func__);
		return 1;
	}

	switch (t) {
	case 1:
		pnc_etype_arp(q);
		break;
	case 2:
		pnc_ip4_tcp(q);
		break;
	case 3:
		pnc_ip4_udp(q);
		break;
	default:
		pr_err("unsupported packet type: value=%d\n", t);
		return 1;
	}
	return 0;
}
#else
int run_rxq_type(int port, int q, int t)
{
	void *port_hndl = mvNetaPortHndlGet(port);

	if (port_hndl == NULL)
		return 1;

	switch (t) {
	case 0:
		mvNetaBpduRxq(port, q);
		break;
	case 1:
		mvNetaArpRxq(port, q);
		break;
	case 2:
		mvNetaTcpRxq(port, q);
		break;
	case 3:
		mvNetaUdpRxq(port, q);
		break;
	default:
		pr_err("unknown packet type: value=%d\n", t);
		return 1;
	}
	return 0;
}
#endif /* CONFIG_MV_ETH_PNC */

static ssize_t mv_eth_port_store(struct device *dev,
				   struct device_attribute *attr, const char *buf, size_t len)
{
	const char      *name = attr->attr.name;
	int             err;
	unsigned int    p;
	unsigned long   flags;

	if (!capable(CAP_NET_ADMIN))
		return -EPERM;

	/* Read port and value */
	err = p = 0;
	sscanf(buf, "%d", &p);

	local_irq_save(flags);

	if (!strcmp(name, "tos")) {
		mv_eth_tos_map_show(p);
	} else if (!strcmp(name, "vprio")) {
		mv_eth_vlan_prio_show(p);
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
	const char	*name = attr->attr.name;
	int		err;
	unsigned int	p, i, v;
	unsigned long	flags;

	if (!capable(CAP_NET_ADMIN))
		return -EPERM;

	err = p = i = v = 0;
	sscanf(buf, "%d %d %d", &p, &i, &v);

	local_irq_save(flags);

	if (!strcmp(name, "rxq_type"))
		err = run_rxq_type(p, i, v);
	else {
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

	if (!strcmp(name, "rxq_tos")) {
		err = mv_eth_rxq_tos_map_set(p, i, v);
	} else if (!strcmp(name, "rxq_vlan")) {
		err = mv_eth_rxq_vlan_prio_set(p, i, v);
	} else {
		err = 1;
		pr_err("%s: illegal operation <%s>\n", __func__, attr->attr.name);
	}

	local_irq_restore(flags);

	return err ? -EINVAL : len;
}

static ssize_t mv_eth_4_hex_store(struct device *dev,
				struct device_attribute *attr, const char *buf, size_t len)
{
	const char      *name = attr->attr.name;
	int             err;
	unsigned int    p, cpu, txq, v;
	unsigned long   flags;

	if (!capable(CAP_NET_ADMIN))
		return -EPERM;

	err = p = cpu = txq = v = 0;
	sscanf(buf, "%d %d %d %x", &p, &txq, &cpu, &v);

	local_irq_save(flags);

	if (!strcmp(name, "txq_tos")) {
		err = mv_eth_txq_tos_map_set(p, txq, cpu, v);
	} else {
		err = 1;
		pr_err("%s: illegal operation <%s>\n", __func__, attr->attr.name);
	}
	local_irq_restore(flags);

	if (err)
		pr_err("%s: error %d\n", __func__, err);

	return err ? -EINVAL : len;
}

static DEVICE_ATTR(help,        S_IRUSR, mv_eth_show, NULL);
static DEVICE_ATTR(tos,         S_IWUSR, mv_eth_show, mv_eth_port_store);
static DEVICE_ATTR(vprio,       S_IWUSR, mv_eth_show, mv_eth_port_store);
static DEVICE_ATTR(rxq_tos,       S_IWUSR, NULL, mv_eth_3_hex_store);
static DEVICE_ATTR(rxq_type,      S_IWUSR, NULL, mv_eth_3_store);
static DEVICE_ATTR(rxq_vlan,      S_IWUSR, NULL, mv_eth_3_hex_store);
static DEVICE_ATTR(txq_tos,     S_IWUSR, mv_eth_show, mv_eth_4_hex_store);


static struct attribute *mv_eth_qos_attrs[] = {
	&dev_attr_help.attr,
	&dev_attr_tos.attr,
	&dev_attr_vprio.attr,
	&dev_attr_rxq_tos.attr,
	&dev_attr_rxq_type.attr,
	&dev_attr_rxq_vlan.attr,
	&dev_attr_txq_tos.attr,
	NULL
};

static struct attribute_group mv_eth_qos_group = {
	.name = "qos",
	.attrs = mv_eth_qos_attrs,
};

int mv_neta_qos_sysfs_init(struct kobject *gbe_kobj)
{
	int err;

	err = sysfs_create_group(gbe_kobj, &mv_eth_qos_group);
	if (err)
		pr_err("sysfs group %s failed %d\n", mv_eth_qos_group.name, err);

	return err;
}

int mv_neta_qos_sysfs_exit(struct kobject *gbe_kobj)
{
	sysfs_remove_group(gbe_kobj, &mv_eth_qos_group);
	return 0;
}
