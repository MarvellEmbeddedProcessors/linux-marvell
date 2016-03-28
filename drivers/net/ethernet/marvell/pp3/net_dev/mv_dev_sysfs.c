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
#include "mv_dev_dbg.h"
#include "common/mv_hw_if.h"
#include "mv_dev_sysfs.h"


static ssize_t pp3_dev_help(char *b)
{
	int o = 0;
	o += scnprintf(b+o, PAGE_SIZE-o, "\n");
	o += scnprintf(b+o, PAGE_SIZE-o, "cd                        init             - go to init time configuration sub directory\n");
	o += scnprintf(b+o, PAGE_SIZE-o, "cd                        debug            - go to debug configuration sub directory\n");
	o += scnprintf(b+o, PAGE_SIZE-o, "cd                        vq               - go to virtual queues configuration sub directory\n");
#if 0
	o += scnprintf(b+o, PAGE_SIZE-o, "cd                        bpi              - go to internal back pressure configuration sub directory\n");
#endif

	o += scnprintf(b+o, PAGE_SIZE-o, "cat                       sys_conf         - show HW resources allocation\n");
	o += scnprintf(b+o, PAGE_SIZE-o, "echo [ifname]           > status           - show network interface status\n");
	o += scnprintf(b+o, PAGE_SIZE-o, "echo [ifname]           > stats            - show network interface statistics (per CPU details)\n");
	o += scnprintf(b+o, PAGE_SIZE-o, "echo [ifname]           > q_stats          - show network interface statistics (per SW queue details)\n");
	o += scnprintf(b+o, PAGE_SIZE-o, "echo [ifname]           > pool_stats       - show network interface BM pools statistics\n");
	o += scnprintf(b+o, PAGE_SIZE-o, "echo [ifname]           > fw_stats         - show network interface FW statistics\n");
	o += scnprintf(b+o, PAGE_SIZE-o, "echo [ifname]           > clear_stats      - clear network interface statistics\n");
#if 0
	o += scnprintf(b+o, PAGE_SIZE-o, "echo [ifname] [mode]    > rx_pkt_mode      - set RX mode for packets ingress from the network interface\n");
#endif
	o += scnprintf(b+o, PAGE_SIZE-o, "echo [ifname] [cpu]     > cpu_affinity     - set default CPU to process packets from the network interface\n");

	o += scnprintf(b+o, PAGE_SIZE-o, "\n");
	o += scnprintf(b+o, PAGE_SIZE-o, "parameters:\n");
	o += scnprintf(b+o, PAGE_SIZE-o, "      [ifname]                     - network interface name e.g. nic0, nic1, etc\n");
#if 0
	o += scnprintf(b+o, PAGE_SIZE-o, "      [mode]                       - RX packets mode: 0 - the whole packet is in DRAM, 1 - packet header is in CFH\n");
#endif
	return o;
}

static ssize_t pp3_dev_show(struct device *dev,
				  struct device_attribute *attr, char *buf)
{
	const char *name = attr->attr.name;
	int err;
	int off = 0;

	if (!capable(CAP_NET_ADMIN))
		return -EPERM;

	if (!strcmp(name, "help"))
		off = pp3_dev_help(buf);
	else if (!strcmp(name, "sys_conf"))
		pp3_dbg_dev_resources_dump();
/*
	else if (!strcmp(name, "cpu_status"))
		pp3_dbg_cpu_status_print();
*/
	else {
		err = 1;
		pr_err("%s: illegal operation <%s>\n", __func__, attr->attr.name);
	}

	return off;
}

static ssize_t pp3_dev_store(struct device *dev,
				   struct device_attribute *attr, const char *buf, size_t len)
{
	const char      *name = attr->attr.name;
	int             err;
	unsigned int    b, c;
	unsigned long   flags;
	char		if_name[10];
	struct net_device *netdev;
	struct	pp3_dev_priv *dev_priv;

	if (!capable(CAP_NET_ADMIN))
		return -EPERM;

	/* Read port and value */
	err = b = c = 0;

	sscanf(buf, "%s %d %d", if_name, &b, &c);


	netdev = dev_get_by_name(&init_net, if_name);

	if (!netdev) {
		pr_err("%s: illegal interface <%s>\n", __func__, if_name);
		return -EINVAL;
	}

	dev_priv = MV_PP3_PRIV(netdev);

	local_irq_save(flags);

	if (!strcmp(name, "status")) {
		pp3_dbg_dev_status_print(netdev);
		pp3_dbg_dev_pools_status_print(netdev);
	} else if (!strcmp(name, "cpu_affinity")) {
		mv_pp3_cpu_affinity_set(netdev, b);
	} else if (!strcmp(name, "stats")) {
		pp3_dbg_dev_stats_dump(netdev);
	} else if (!strcmp(name, "q_stats")) {
		pp3_dbg_dev_queues_stats_dump(netdev);
	} else if (!strcmp(name, "fw_stats")) {
		pp3_dbg_dev_fw_stats_dump(netdev);
	} else if (!strcmp(name, "clear_stats")) {
		pp3_dbg_dev_stats_clear(netdev);
	} else if (!strcmp(name, "pool_stats")) {
		pp3_dbg_dev_pools_stats_dump(netdev);
#if 0
	} else if (!strcmp(name, "rx_pkt_mode")) {
		mv_pp3_rx_pkt_mode_set(netdev, b);
#endif /* if 0 */
	} else {
		err = 1;
		pr_err("%s: illegal operation <%s>\n", __func__, attr->attr.name);
	}

	local_irq_restore(flags);
	dev_put(netdev);

	if (err)
		pr_err("%s: error %d\n", __func__, err);

	return err ? -EINVAL : len;
}

static DEVICE_ATTR(status,		S_IWUSR, NULL, pp3_dev_store);
static DEVICE_ATTR(cpu_affinity,	S_IWUSR, NULL, pp3_dev_store);
static DEVICE_ATTR(stats,		S_IWUSR, NULL, pp3_dev_store);
static DEVICE_ATTR(q_stats,		S_IWUSR, NULL, pp3_dev_store);
static DEVICE_ATTR(clear_stats,		S_IWUSR, NULL, pp3_dev_store);
static DEVICE_ATTR(pool_stats,		S_IWUSR, NULL, pp3_dev_store);
static DEVICE_ATTR(fw_stats,		S_IWUSR, NULL, pp3_dev_store);
#if 0
static DEVICE_ATTR(rx_pkt_mode,		S_IWUSR, NULL, pp3_dev_store);
#endif
static DEVICE_ATTR(sys_conf,		S_IRUSR, pp3_dev_show, NULL);
static DEVICE_ATTR(help,		S_IRUSR, pp3_dev_show, NULL);

static struct attribute *pp3_dev_attrs[] = {
	&dev_attr_help.attr,
	&dev_attr_sys_conf.attr,
	&dev_attr_status.attr,
	&dev_attr_cpu_affinity.attr,
	&dev_attr_stats.attr,
	&dev_attr_q_stats.attr,
	&dev_attr_fw_stats.attr,
	&dev_attr_clear_stats.attr,
	&dev_attr_pool_stats.attr,
#if 0
	&dev_attr_rx_pkt_mode.attr,
#endif
	NULL
};

static struct attribute_group pp3_dev_group = {
	.attrs = pp3_dev_attrs,
};


int mv_pp3_dev_sysfs_init(struct kobject *pp3_kobj)
{
	int err;
	struct kobject *dev_kobj;

	dev_kobj = kobject_create_and_add("dev", pp3_kobj);
	if (!dev_kobj) {
		printk(KERN_ERR"%s: cannot create dev kobject\n", __func__);
		return -ENOMEM;
	}

	err = sysfs_create_group(dev_kobj, &pp3_dev_group);
	if (err) {
		pr_err("sysfs group %s failed %d\n", pp3_dev_group.name, err);
		return err;
	}
	err = mv_pp3_dev_vq_sysfs_init(dev_kobj);
	if (err)
		return err;

#if 0
	err = mv_pp3_dev_bpi_sysfs_init(dev_kobj);
	if (err)
		return err;
#endif /*if 0 */

	err = mv_pp3_dev_init_sysfs_init(dev_kobj);
	if (err)
		return err;

	err = mv_pp3_dev_debug_sysfs_init(dev_kobj);
	if (err)
		return err;

	return 0;
}

int mv_pp3_dev_sysfs_exit(struct kobject *pp3_kobj)
{
	int err;
#if 0

	err = mv_pp3_dev_bpi_sysfs_exit(pp3_kobj);
	if (err)
		return err;
#endif

	err = mv_pp3_dev_init_sysfs_exit(pp3_kobj);
	if (err)
		return err;

	err = mv_pp3_dev_debug_sysfs_exit(pp3_kobj);
	if (err)
		return err;

	err = mv_pp3_dev_vq_sysfs_exit(pp3_kobj);
	if (err)
		return err;

	sysfs_remove_group(pp3_kobj, &pp3_dev_group);

	return 0;
}


