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

#include "mvCommon.h"
#include "mvTypes.h"
#include "dpi/mvPp2DpiHw.h"


static ssize_t dpi_help(char *b)
{
	int o = 0;

	o += scnprintf(b + o, PAGE_SIZE - o, "arguments c, d, o, s, n: decimal numbers\n");
	o += scnprintf(b + o, PAGE_SIZE - o, "arguments b, m         : hexadecimal numbers\n");
	o += scnprintf(b + o, PAGE_SIZE - o, "[c] counter valid range [0..%d]\n", MV_PP2_DPI_CNTRS - 1);
	o += scnprintf(b + o, PAGE_SIZE - o, "[o] window offset valid range [0..%d] bytes\n", MV_PP2_DPI_WIN_OFFSET_MAX);
	o += scnprintf(b + o, PAGE_SIZE - o, "[s] window size valid range [0..%d] bytes\n", MV_PP2_DPI_WIN_SIZE_MAX);
	o += scnprintf(b + o, PAGE_SIZE - o, "[n] number of descriptors valid range [0..%d]\n", MV_PP2_DPI_Q_SIZE_MAX);
	o += scnprintf(b + o, PAGE_SIZE - o, "\n");

	o += scnprintf(b + o, PAGE_SIZE - o, "cat            help    - Show this help\n");
	o += scnprintf(b + o, PAGE_SIZE - o, "cat            regs    - Show DPI hardware registers\n");
	o += scnprintf(b + o, PAGE_SIZE - o, "echo d         queues  - Show DPI request and result queues. 0-brief, 1-full\n");
	o += scnprintf(b + o, PAGE_SIZE - o, "echo c o s   > win     - Set window offset [o] and size [s] for DPI counter [c]\n");
	o += scnprintf(b + o, PAGE_SIZE - o, "echo b m     > cntrs   - Set map of counters [m] to be incremented for byte [b]\n");
	o += scnprintf(b + o, PAGE_SIZE - o, "echo c b 0|1 > cntr_en - On/Off incrementing of DPI counter [c] for byte [b]\n");
	o += scnprintf(b + o, PAGE_SIZE - o, "echo c       > disable - Disable incrementing of DPI counter [c] for all bytes\n");
	o += scnprintf(b + o, PAGE_SIZE - o, "echo n       > q_size  - Set number of descriptors [n] for DPI queues\n");
	o += scnprintf(b + o, PAGE_SIZE - o, "echo data    > do_req  - Put DPI request for [data=xxxxxx...] and print results\n");

	return o;
}

static ssize_t dpi_show(struct device *dev,
			struct device_attribute *attr, char *buf)
{
	const char  *name = attr->attr.name;

	if (!capable(CAP_NET_ADMIN))
		return -EPERM;

	if (!strcmp(name, "help"))
		return dpi_help(buf);

	if (!strcmp(name, "regs")) {
		mvPp2DpiRegs();
	} else if (!strcmp(name, "queues")) {
		mvPp2DpiQueueShow(0);
	} else {
		printk(KERN_ERR "%s: illegal operation <%s>\n", __func__, attr->attr.name);
		return -EINVAL;
	}
	return 0;
}

static ssize_t dpi_dec_store(struct device *dev,
			struct device_attribute *attr, const char *buf, size_t len)
{
	const char    *name = attr->attr.name;
	unsigned int  err = 0, a = 0, b = 0, c = 0;
	unsigned long flags;

	if (!capable(CAP_NET_ADMIN))
		return -EPERM;

	sscanf(buf, "%d %d %d", &a, &b, &c);

	local_irq_save(flags);

	if (!strcmp(name, "queues")) {
		mvPp2DpiQueueShow(a);
	} else if (!strcmp(name, "win")) {
		err = mvPp2DpiCntrWinSet(a, b, c);
	} else if (!strcmp(name, "disable")) {
		err = mvPp2DpiCntrDisable(a);
	} else if (!strcmp(name, "q_size")) {
		if (mvPp2DpiQueuesDelete())
			pr_err("DPI: %s command error. Can't delete queues\n", name);
		err = mvPp2DpiQueuesCreate(a);
	} else
		printk(KERN_ERR "%s: illegal operation <%s>\n", __func__, name);

	local_irq_restore(flags);

	if (err)
		printk(KERN_ERR "%s: <%s>, error %d\n", __func__, attr->attr.name, err);

	return err ? -EINVAL : len;
}

static ssize_t dpi_hex_store(struct device *dev,
			struct device_attribute *attr, const char *buf, size_t len)
{
	const char    *name = attr->attr.name;
	unsigned int  err = 0, a = 0, b = 0;
	unsigned long flags;

	if (!capable(CAP_NET_ADMIN))
		return -EPERM;

	sscanf(buf, "%x %x", &a, &b);

	local_irq_save(flags);

	if (!strcmp(name, "cntrs"))
		mvPp2DpiByteConfig(a, b);
	else
		printk(KERN_ERR "%s: illegal operation <%s>\n", __func__, name);

	local_irq_restore(flags);

	if (err)
		printk(KERN_ERR "%s: <%s>, error %d\n", __func__, attr->attr.name, err);

	return err ? -EINVAL : len;
}

static ssize_t dpi_dec_hex_store(struct device *dev,
			struct device_attribute *attr, const char *buf, size_t len)
{
	const char    *name = attr->attr.name;
	unsigned int  err = 0, a = 0, b = 0, c = 0;
	unsigned long flags;

	if (!capable(CAP_NET_ADMIN))
		return -EPERM;

	sscanf(buf, "%d %x %d", &a, &b, &c);

	local_irq_save(flags);

	if (!strcmp(name, "cntr_en"))
		mvPp2DpiCntrByteSet(a, b, c);
	else
		printk(KERN_ERR "%s: illegal operation <%s>\n", __func__, name);

	local_irq_restore(flags);

	if (err)
		printk(KERN_ERR "%s: <%s>, error %d\n", __func__, attr->attr.name, err);

	return err ? -EINVAL : len;
}

static char		dpi_pkt_data[MV_PP2_DPI_MAX_PKT_SIZE];
static unsigned char	dpi_counters[MV_PP2_DPI_CNTRS];
#define DPI_REQUEST_TIMEOUT  100000

static int mv_pp2_dpi_do_request(char *data, int size, unsigned char *counters)
{
	unsigned int timeout = DPI_REQUEST_TIMEOUT;
	unsigned long phys_addr;
	int ready_num;

	phys_addr = mvOsCacheFlush(NULL, data, size);
	if (mvPp2DpiRequestSet(phys_addr, size)) {
		pr_err("%s: DPI request set failed\n", __func__);
		return -EINVAL;
	}
	/* Start processing */
	wmb();
	mvPp2DpiReqPendAdd(1);

	/* Wait for response is ready */
	ready_num = 0;
	while (ready_num == 0) {
		timeout--;
		if (timeout == 0) {
			pr_err("%s: DPI result get timeout\n", __func__);
			return -EINVAL;
		}
		ready_num = mvPp2DpiResOccupGet();
	}
	pr_info("DPI request is ready after %d\n", DPI_REQUEST_TIMEOUT - timeout);
	if (ready_num != 1)
		pr_warning("%s: %d requests became ready - only one processsed\n",
			__func__, ready_num);

	/* Process single response - copy counters */
	mvOsCacheIoSync(NULL);
	mvPp2DpiResultGet(dpi_counters, MV_PP2_DPI_CNTRS);

	/* Enable HW to reuse Response descriptors */
	wmb();
	mvPp2DpiResOccupDec(ready_num);

	return 0;
}

static ssize_t dpi_string_store(struct device *dev,
			struct device_attribute *attr, const char *buf, size_t len)
{
	const char    *name = attr->attr.name;
	unsigned int  err = 0, size = 0, i;

	if (!capable(CAP_NET_ADMIN))
		return -EPERM;

	if (!strcmp(name, "do_req")) {
		size = strlen(buf) / 2;
		if (size > sizeof(dpi_pkt_data))
			size = sizeof(dpi_pkt_data);
		mvHexToBin(buf, dpi_pkt_data, size);
		err = mv_pp2_dpi_do_request(dpi_pkt_data, size, dpi_counters);
		if (!err) {
			for (i = 0; i < MV_PP2_DPI_CNTRS; i++)
				pr_info("#%2d  -  %d\n", i, dpi_counters[i]);

			pr_info("\n");
		}
	} else
		printk(KERN_ERR "%s: illegal operation <%s>\n", __func__, name);

	if (err)
		printk(KERN_ERR "%s: <%s>, error %d\n", __func__, attr->attr.name, err);

	return err ? -EINVAL : len;
}

static DEVICE_ATTR(help,          S_IRUSR, dpi_show, NULL);
static DEVICE_ATTR(regs,          S_IRUSR, dpi_show, NULL);
static DEVICE_ATTR(queues,        S_IRUSR | S_IWUSR, dpi_show, dpi_dec_store);
static DEVICE_ATTR(win,           S_IWUSR, NULL, dpi_dec_store);
static DEVICE_ATTR(cntrs,         S_IWUSR, NULL, dpi_hex_store);
static DEVICE_ATTR(cntr_en,       S_IWUSR, NULL, dpi_dec_hex_store);
static DEVICE_ATTR(disable,       S_IWUSR, NULL, dpi_dec_store);
static DEVICE_ATTR(q_size,        S_IWUSR, NULL, dpi_dec_store);
static DEVICE_ATTR(do_req,        S_IWUSR, NULL, dpi_string_store);

static struct attribute *dpi_attrs[] = {
	&dev_attr_help.attr,
	&dev_attr_regs.attr,
	&dev_attr_queues.attr,
	&dev_attr_win.attr,
	&dev_attr_cntrs.attr,
	&dev_attr_cntr_en.attr,
	&dev_attr_disable.attr,
	&dev_attr_q_size.attr,
	&dev_attr_do_req.attr,
	NULL
};


static struct attribute_group mv_dpi_group = {
	.name = "dpi",
	.attrs = dpi_attrs,
};

int mv_pp2_dpi_sysfs_init(struct kobject *pp2_kobj)
{
	int err = 0;

	err = sysfs_create_group(pp2_kobj, &mv_dpi_group);
	if (err)
		printk(KERN_INFO "sysfs group %s failed %d\n", mv_dpi_group.name, err);

	return err;
}

int mv_pp2_dpi_sysfs_exit(struct kobject *pp2_kobj)
{
	sysfs_remove_group(pp2_kobj, &mv_dpi_group);

	return 0;
}

