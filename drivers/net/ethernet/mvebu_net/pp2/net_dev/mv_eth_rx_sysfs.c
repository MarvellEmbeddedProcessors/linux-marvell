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
#include "gbe/mvPp2Gbe.h"
#include "prs/mvPp2Prs.h"
#include "mv_netdev.h"

static ssize_t mv_eth_help(char *b)
{
	int o = 0;

	o += sprintf(b+o, "cat                    rxDmaRegs   - show RX DMA registers\n");
	o += sprintf(b+o, "echo [p]             > rxFifoRegs  - show RX FIFO registers for port <p>\n");
	o += sprintf(b+o, "echo [rxq]           > gRxqRegs    - show RXQ registers for global <rxq>\n");
#ifdef CONFIG_MV_ETH_PP2_1
	o += sprintf(b+o, "echo [p] [rxq]       > rxqCounters - show RXQ counters for <p/rxq>.\n");
#endif
	o += sprintf(b+o, "echo [p] [rxq]       > pRxqRegs    - show RXQ registers for global <rxq>\n");
	o += sprintf(b+o, "echo [p] [rxq] [0|1] > rxqShow     - show RXQ descriptors ring for <p/rxq>\n");
	o += sprintf(b+o, "echo [p] [rxq] [v]   > rxqSize     - set number of descriptors <v> for <port/rxq>.\n");
	o += sprintf(b+o, "echo [p] [hex] [0|1] > mhRxSpec    - set MH value [hex] for RX special packets\n");
	o += sprintf(b+o, "echo [p] [m]         > prefetch    - set RX prefetch mode for port [p]\n");
	o += sprintf(b+o, "                                   [m]: 0-disable, 1-descriptor, 2-packet header, 3-both\n");

	return o;
}

static ssize_t mv_eth_show(struct device *dev,
				  struct device_attribute *attr, char *buf)
{
	const char      *name = attr->attr.name;
	int             off = 0;

	if (!capable(CAP_NET_ADMIN))
		return -EPERM;

	if (!strcmp(name, "rxDmaRegs"))
		mvPp2RxDmaRegsPrint();
	else
		off = mv_eth_help(buf);

	return off;
}

static ssize_t mv_eth_port_store(struct device *dev,
				   struct device_attribute *attr, const char *buf, size_t len)
{
	const char      *name = attr->attr.name;
	int             err;
	unsigned int    p, v, a;
	unsigned long   flags;

	if (!capable(CAP_NET_ADMIN))
		return -EPERM;

	/* Read port and value */
	err = p = v = a = 0;
	sscanf(buf, "%d %d %d", &p, &v, &a);

	local_irq_save(flags);

	if (!strcmp(name, "rxqShow")) {
		mvPp2RxqShow(p, v, a);
	} else if (!strcmp(name, "gRxqRegs")) {
		mvPp2PhysRxqRegs(p);
	} else if (!strcmp(name, "pRxqRegs")) {
		mvPp2PortRxqRegs(p, v);
	} else if (!strcmp(name, "rxFifoRegs")) {
		mvPp2RxFifoRegs(p);
	} else if (!strcmp(name, "rxqSize")) {
		mv_eth_ctrl_rxq_size_set(p, v, a);
	} else if (!strcmp(name, "rxqCounters")) {
		mvPp2V1RxqDbgCntrs(p, v);
	} else if (!strcmp(name, "prefetch")) {
		err |= mv_eth_ctrl_flag(p, MV_ETH_F_RX_DESC_PREFETCH, v & 0x1);
		err |= mv_eth_ctrl_flag(p, MV_ETH_F_RX_PKT_PREFETCH, v & 0x2);
	} else {
		err = 1;
		printk(KERN_ERR "%s: illegal operation <%s>\n", __func__, attr->attr.name);
	}

	local_irq_restore(flags);

	if (err)
		printk(KERN_ERR "%s: error %d\n", __func__, err);

	return err ? -EINVAL : len;
}

static ssize_t mv_eth_rx_hex_store(struct device *dev,
				struct device_attribute *attr, const char *buf, size_t len)
{
	const char      *name = attr->attr.name;
	int             err;
	unsigned int    p, v, a;
	unsigned long   flags;

	if (!capable(CAP_NET_ADMIN))
		return -EPERM;

	/* Read port and value */
	err = p = v = a = 0;
	sscanf(buf, "%d %x %x", &p, &v, &a);

	local_irq_save(flags);

	if (!strcmp(name, "mhRxSpec")) {
		mvPrsMhRxSpecialSet(MV_PPV2_PORT_PHYS(p), v, a);
	} else {
		err = 1;
		printk(KERN_ERR "%s: illegal operation <%s>\n", __func__, attr->attr.name);
	}

	local_irq_restore(flags);

	if (err)
		printk(KERN_ERR "%s: error %d\n", __func__, err);

	return err ? -EINVAL : len;
}

static DEVICE_ATTR(help,        S_IRUSR, mv_eth_show, NULL);
static DEVICE_ATTR(rxDmaRegs,  	S_IRUSR, mv_eth_show, NULL);
static DEVICE_ATTR(rxqCounters, S_IWUSR, NULL, mv_eth_port_store);
static DEVICE_ATTR(rxqShow,     S_IWUSR, NULL, mv_eth_port_store);
static DEVICE_ATTR(gRxqRegs,    S_IWUSR, NULL, mv_eth_port_store);
static DEVICE_ATTR(pRxqRegs,    S_IWUSR, NULL, mv_eth_port_store);
static DEVICE_ATTR(rxFifoRegs,  S_IWUSR, NULL, mv_eth_port_store);
static DEVICE_ATTR(rxqSize,   	S_IWUSR, NULL, mv_eth_port_store);
static DEVICE_ATTR(mhRxSpec,	S_IWUSR, NULL, mv_eth_rx_hex_store);
static DEVICE_ATTR(prefetch,	S_IWUSR, NULL, mv_eth_port_store);

static struct attribute *mv_eth_attrs[] = {
	&dev_attr_help.attr,
	&dev_attr_rxDmaRegs.attr,
	&dev_attr_rxqShow.attr,
	&dev_attr_rxqCounters.attr,
	&dev_attr_gRxqRegs.attr,
	&dev_attr_pRxqRegs.attr,
	&dev_attr_rxFifoRegs.attr,
	&dev_attr_rxqSize.attr,
	&dev_attr_mhRxSpec.attr,
	&dev_attr_prefetch.attr,
	NULL
};

static struct attribute_group mv_eth_rx_group = {
	.name = "rx",
	.attrs = mv_eth_attrs,
};

int mv_pp2_rx_sysfs_init(struct kobject *gbe_kobj)
{
	int err;

	err = sysfs_create_group(gbe_kobj, &mv_eth_rx_group);
	if (err)
		pr_err("sysfs group %s failed %d\n", mv_eth_rx_group.name, err);

	return err;
}

int mv_pp2_rx_sysfs_exit(struct kobject *gbe_kobj)
{
	sysfs_remove_group(gbe_kobj, &mv_eth_rx_group);

	return 0;
}
