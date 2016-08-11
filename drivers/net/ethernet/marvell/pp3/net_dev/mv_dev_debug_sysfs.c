/*
* ***************************************************************************
* Copyright (C) 2015 Marvell International Ltd.
* ***************************************************************************
* This program is free software: you can redistribute it and/or modify it
* under the terms of the GNU General Public License as published by the Free
* Software Foundation, either version 2 of the License, or any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program.  If not, see <http://www.gnu.org/licenses/>.
* ***************************************************************************
*/


#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/types.h>
#include <linux/capability.h>
#include <linux/platform_device.h>
#include <linux/netdevice.h>

#include "mv_netdev.h"
#include "mv_netdev_structs.h"
#include "mv_dev_vq.h"
#include "mv_dev_dbg.h"
#include "common/mv_hw_if.h"


#define MV_HLP(STR) { o += scnprintf(b + o, PAGE_SIZE - o, STR); }

static ssize_t pp3_dev_debug_help(char *b)
{
	int o = 0;

#ifdef CONFIG_MV_PP3_DEBUG_CODE
MV_HLP("\n");
MV_HLP("cat                       help           - show this help\n");
MV_HLP("echo [ifname]           > mac_show       - show MAC addresses for network interface\n");
#if 0
MV_HLP("echo [cpu]              > txdone_history - show tx done history staistics\n");
#endif
MV_HLP("echo [0|1|2]            > internal_debug - action on error: warn|stop|panic\n");
MV_HLP("echo [ifname] [mask]    > debug          - enable/disable network interface debug messages\n");
MV_HLP("echo [ifname] [cir] [eir] [cbs] [ebs] > tx_shaper\n");
MV_HLP("                                         - set shaping rates and burst sizes for egress emac port\n");
#if 0
MV_HLP("echo [cpu] [mask]       > cpu_debug      - enable/disable cpu debug messages\n");
#ifndef CONFIG_MV_PP3_FPGA
MV_HLP("echo [0|1]              > rx_isr_mode    - set rx ISR mode for all network interfaces. 0-poll, 1-isr\n");
#endif /* CONFIG_MV_PP3_FPGA */
#endif
MV_HLP("echo [ifname] [rx] [tx] > create         - create new virtual network device (no emac connectivity)\n");
MV_HLP("\n");
MV_HLP("parameters:\n");
MV_HLP("      [ifname]    - network interface name e.g. nic0, nic1, etc\n");
MV_HLP("      [mode]      - 0 for 4 HWQ per SWQ, 1 for 1 HWQ per SWQ\n");
MV_HLP("      [mask]      - for debug command: b0-rx, b1-tx, b2-isr, b3-poll\n");
MV_HLP("                  - for cpu_devug command: b0-buff push, b1-buff pop\n");
MV_HLP("      [rx]        - number of rx virtual queueus\n");
MV_HLP("      [tx]        - number of tx virtual queueus\n");
MV_HLP("      [cir]       - committed information rate in [Mbps] units, granularity of [10 Mbps]\n");
MV_HLP("      [eir]       - excessive information rate in [Mbps] units, granularity of [10 Mbps]\n");
MV_HLP("      [cbs]       - committed burst size in [KBytes]\n");
MV_HLP("      [ebs]       - excessive burst size in [KBytes]\n");
#endif
	return o;
}


static ssize_t pp3_dev_debug_show(struct device *dev,
				  struct device_attribute *attr, char *buf)
{
	int off = 0;

	if (!capable(CAP_NET_ADMIN))
		return -EPERM;

	off = pp3_dev_debug_help(buf);

	return off;
}

ssize_t pp3_dev_debug_dec_store(struct device *dev, struct device_attribute *attr,
					const char *buf, size_t len)
{
	const char      *name = attr->attr.name;
	int             fields, err = 1;
	unsigned int    p;
	unsigned long   flags;

	if (!capable(CAP_NET_ADMIN))
		return -EPERM;

	/* Read port and value */
	p = 0;
	fields = sscanf(buf, "%d", &p);

	local_irq_save(flags);

#ifdef PP3_INTERNAL_DEBUG
	if (!strcmp(name, "internal_debug")) {
		if (fields == 1)
			err = mv_pp3_ctrl_internal_debug_set(p);
	}
#endif

	if (err)
		pr_err("%s: operation <%s> FAILED. err = %d\n", __func__, name, err);

	local_irq_restore(flags);

	return err ? -EINVAL : len;
}

static ssize_t pp3_dev_debug_store(struct device *dev,
				   struct device_attribute *attr, const char *buf, size_t len)
{
	const char      *name = attr->attr.name;
	int             fields, err = 1;
	unsigned int    a, b, c, d;
	unsigned long   flags;
	char		if_name[10];
	struct net_device *netdev;

	if (!capable(CAP_NET_ADMIN))
		return -EPERM;

	/* Read port and value */
	err = b = c = 0;

	fields = sscanf(buf, "%s %d %d %d %d", if_name, &a, &b, &c, &d);

	/* Create external network interface (without EMAC connectivity) */
	if (!strcmp(name, "create")) {
		if (fields == 3) {
			local_irq_save(flags);
			if (mv_pp3_netdev_init(if_name, a, b))
				err = 0;

			local_irq_restore(flags);
		}
		return err ? -EINVAL : len;
	}

	netdev = dev_get_by_name(&init_net, if_name);
	if (!mv_pp3_dev_is_valid(netdev)) {
		pr_err("%s in not pp3 device\n", if_name);
		if (netdev)
			dev_put(netdev);
		return -EINVAL;
	}

	local_irq_save(flags);

	if (!strcmp(name, "debug")) {
		if (fields == 2) {
			pp3_dbg_dev_flags(netdev, MV_PP3_F_DBG_RX, a & 0x1);
			pp3_dbg_dev_flags(netdev, MV_PP3_F_DBG_TX, a & 0x2);
			pp3_dbg_dev_flags(netdev, MV_PP3_F_DBG_ISR, a & 0x4);
			pp3_dbg_dev_flags(netdev, MV_PP3_F_DBG_POLL, a & 0x8);
			pp3_dbg_dev_flags(netdev, MV_PP3_F_DBG_SG, a & 0x80);
			dev_put(netdev);
			err = 0;
		}
	} else if (!strcmp(name, "mac_show")) {
		if (fields == 1) {
			pp3_dbg_dev_mac_show(netdev);
			dev_put(netdev);
			err = 0;
		}
	} else if (!strcmp(name, "tx_shaper")) {
		if (fields == 5) {
			struct mv_nss_meter meter;

			meter.cir = a;
			meter.eir = b;
			meter.cbs = c;
			meter.ebs = d;
			meter.enable = true;
			err = mv_pp3_dev_egress_vport_shaper_set(netdev, &meter);
			dev_put(netdev);
		}
	} else {
		pr_err("%s: illegal operation <%s>\n", __func__, attr->attr.name);
	}

	local_irq_restore(flags);

	if (err)
		pr_err("%s: error %d\n", __func__, err);

	return err ? -EINVAL : len;
}

static DEVICE_ATTR(help,		S_IRUSR, pp3_dev_debug_show, NULL);
static DEVICE_ATTR(internal_debug,	S_IWUSR, NULL, pp3_dev_debug_dec_store);
static DEVICE_ATTR(debug,		S_IWUSR, NULL, pp3_dev_debug_store);
static DEVICE_ATTR(mac_show,		S_IWUSR, NULL, pp3_dev_debug_store);
static DEVICE_ATTR(tx_shaper,		S_IWUSR, NULL, pp3_dev_debug_store);
static DEVICE_ATTR(create,		S_IWUSR, NULL, pp3_dev_debug_store);

#if 0
static DEVICE_ATTR(txdone_history,	S_IWUSR, NULL, pp3_dev_debug_hex_store);
static DEVICE_ATTR(cpu_debug,		S_IWUSR, NULL, pp3_dev_debug_hex_store);
#ifndef CONFIG_MV_PP3_FPGA
static DEVICE_ATTR(rx_isr_mode,		S_IWUSR, NULL, pp3_dev_debug_hex_store);
#endif /* !CONFIG_MV_PP3_FPGA */
#endif

static struct attribute *pp3_dev_debug_attrs[] = {
	&dev_attr_help.attr,
	&dev_attr_debug.attr,
	&dev_attr_mac_show.attr,
	&dev_attr_tx_shaper.attr,
	&dev_attr_create.attr,
	&dev_attr_internal_debug.attr,
#if 0
	&dev_attr_cpu_debug.attr,
	&dev_attr_txdone_history.attr,
#ifndef CONFIG_MV_PP3_FPGA
	&dev_attr_rx_isr_mode.attr,
#endif	/* !CONFIG_MV_PP3_FPGA */
#endif
	NULL
};

static struct attribute_group pp3_dev_debug_group = {
	.name = "debug",
	.attrs = pp3_dev_debug_attrs,
};

int mv_pp3_dev_debug_sysfs_init(struct kobject *pp3_kobj)
{
	int err;

	err = sysfs_create_group(pp3_kobj, &pp3_dev_debug_group);
	if (err) {
		pr_err("sysfs group failed for dev debug%d\n", err);
		return err;
	}

	return 0;
}

int mv_pp3_dev_debug_sysfs_exit(struct kobject *pp3_kobj)
{
	sysfs_remove_group(pp3_kobj, &pp3_dev_debug_group);

	return 0;
}


