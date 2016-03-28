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

#include "vport/mv_pp3_vport.h"

static ssize_t pp3_dev_help(char *b)
{
	int o = 0;
	o += scnprintf(b+o, PAGE_SIZE-o, "cat                   show           - show all active virtual ports\n");
	o += scnprintf(b+o, PAGE_SIZE-o, "echo [cpu_vp]       > stats          - show CPU internal virtual port statistics\n");
	o += scnprintf(b+o, PAGE_SIZE-o, "echo [cpu_vp]       > q_stats        - show CPU internal virtual port RX and TX queues statistics\n");
	o += scnprintf(b+o, PAGE_SIZE-o, "echo [eth_vp]       > fw_stats       - show EMAC / external virtual port FW statistics\n");
	o += scnprintf(b+o, PAGE_SIZE-o, "echo [vp]           > clear_stats    - clear virtual port statistics\n");

	o += scnprintf(b+o, PAGE_SIZE-o, "\n");
	o += scnprintf(b+o, PAGE_SIZE-o, "parameters:\n");
	o += scnprintf(b+o, PAGE_SIZE-o, "      [eth_vp]      - ETH type virtual port number\n");
	o += scnprintf(b+o, PAGE_SIZE-o, "      [cpu_vp]      - internal CPU type virtual port number\n");
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
	else if (!strcmp(name, "show"))
		mv_pp3_vports_dump();
	else {
		err = 1;
		pr_err("%s: illegal operation <%s>\n", __func__, attr->attr.name);
	}

	return off;
}

static ssize_t pp3_dev_store(struct device *dev, struct device_attribute *attr,
					const char *buf, size_t len)
{
	const char      *name = attr->attr.name;
	int             err;
	int             vport;
	unsigned long   flags, num;

	if (!capable(CAP_NET_ADMIN))
		return -EPERM;

	/* Read port and value */
	err = 0;
	num = sscanf(buf, "%d %d", &vport);

	local_irq_save(flags);

	if (!strcmp(name, "stats"))
		mv_pp3_cpu_vport_stats_dump(vport);
	else if (!strcmp(name, "q_stats"))
		mv_pp3_cpu_vport_vqs_stats_dump(vport);
	else if (!strcmp(name, "fw_stats"))
		mv_pp3_vport_fw_stats_dump(vport);
	else if (!strcmp(name, "clear_stats")) {
		mv_pp3_cpu_vport_stats_clear(vport);
		mv_pp3_vport_fw_stats_clear(vport);
	} else {
		err = 1;
		pr_err("%s: illegal operation <%s>\n", __func__, attr->attr.name);
	}
	local_irq_restore(flags);

	if (err)
		pr_err("%s: error %d\n", __func__, err);

	return err ? -EINVAL : len;
}

static DEVICE_ATTR(show,		S_IRUSR, pp3_dev_show, NULL);
static DEVICE_ATTR(help,		S_IRUSR, pp3_dev_show, NULL);

static DEVICE_ATTR(stats,		S_IWUSR, NULL, pp3_dev_store);
static DEVICE_ATTR(q_stats,		S_IWUSR, NULL, pp3_dev_store);
static DEVICE_ATTR(fw_stats,		S_IWUSR, NULL, pp3_dev_store);
static DEVICE_ATTR(clear_stats,		S_IWUSR, NULL, pp3_dev_store);

static struct attribute *pp3_dev_attrs[] = {
	&dev_attr_help.attr,
	&dev_attr_show.attr,
	&dev_attr_stats.attr,
	&dev_attr_q_stats.attr,
	&dev_attr_fw_stats.attr,
	&dev_attr_clear_stats.attr,

	NULL
};


static struct attribute_group pp3_dev_group = {
	.attrs = pp3_dev_attrs,
};


int mv_pp3_vport_sysfs_init(struct kobject *pp3_kobj)
{
	int err;
	struct kobject *dev_kobj;

	dev_kobj = kobject_create_and_add("vport", pp3_kobj);
	if (!dev_kobj) {
		pr_err("%s: cannot create vport kobject\n", __func__);
		return -ENOMEM;
	}

	err = sysfs_create_group(dev_kobj, &pp3_dev_group);
	if (err) {
		pr_err("sysfs group %s failed %d\n", pp3_dev_group.name, err);
		return err;
	}

	return err;
}

int mv_pp3_vport_sysfs_exit(struct kobject *pp3_kobj)
{
	sysfs_remove_group(pp3_kobj, &pp3_dev_group);

	return 0;
}


