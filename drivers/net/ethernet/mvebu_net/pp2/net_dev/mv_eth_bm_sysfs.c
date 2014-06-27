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
#include "mv_netdev.h"

static ssize_t mv_pp2_help(char *buf)
{
	int off = 0;

#ifdef CONFIG_MV_ETH_PP2_1
	off += sprintf(buf+off, "cat                            queueMappDump   - print BM all rxq/txq to qSet mapp\n");
	off += sprintf(buf+off, "cat                            qsetConfigDump  - print BM all qSets configuration\n");
	off += sprintf(buf+off, "echo [qset] [pool]           > qsetCreate      - create qset and attach it to BM [pool]\n");
	off += sprintf(buf+off, "echo [qset]                  > qsetDelete      - delete an unused qset (not used by RXQ/TXQ)\n");
	off += sprintf(buf+off, "echo [qset] [grntd] [shared] > qsetMaxSet      - set max buff parameters for [qset]\n");
	off += sprintf(buf+off, "echo [rxq] [qset]            > rxqQsetLong     - map [rxq] long Qset to [qset]\n");
	off += sprintf(buf+off, "echo [rxq] [qset]            > rxqQsetShort    - map [rxq] short Qset to [qset]\n");
	off += sprintf(buf+off, "echo [txq] [qset]            > txqQsetLong     - map [txq] long Qset to [qset]\n");
	off += sprintf(buf+off, "echo [txq] [qset]            > txqQsetShort    - map [txq] short Qset to [qset]\n");
	off += sprintf(buf+off, "echo [qset]                  > qsetShow        - show info for Qset [qset]\n");

	off += sprintf(buf+off, "echo [pool]                  > poolDropCnt     - print BM pool drop counters\n");
#endif

	off += sprintf(buf+off, "echo [pool]                  > poolRegs        - print BM pool registers\n");
	off += sprintf(buf+off, "echo [pool]                  > poolStatus      - print BM pool status\n");
	off += sprintf(buf+off, "echo [pool] [size]           > poolSize        - set packet size to BM pool\n");
	off += sprintf(buf+off, "echo [port] [pool] [buf_num] > poolBufNum      - set buffers num for BM pool\n");
	off += sprintf(buf+off, "                                                 [port] - any port use this pool");
	off += sprintf(buf+off, "echo [port] [pool]           > longPool        - set port's long BM pool\n");
	off += sprintf(buf+off, "echo [port] [pool]           > shortPool       - set port's short BM pool\n");
	off += sprintf(buf+off, "echo [port] [pool]           > hwfLongPool     - set port's HWF long BM pool\n");
	off += sprintf(buf+off, "echo [port] [pool]           > hwfShortPool    - set port's HWF short BM pool\n");

	return off;
}

static ssize_t mv_pp2_show(struct device *dev,
				  struct device_attribute *attr, char *buf)
{
	const char	*name = attr->attr.name;
	int             off = 0;

	if (!capable(CAP_NET_ADMIN))
		return -EPERM;

	if (!strcmp(name, "queueMappDump"))
		mvBmQueueMapDumpAll();
	else if  (!strcmp(name, "qsetConfigDump"))
		mvBmQsetConfigDumpAll();

	else
		off = mv_pp2_help(buf);

	return off;
}

static ssize_t mv_pp2_port_store(struct device *dev,
				   struct device_attribute *attr, const char *buf, size_t len)
{
	const char      *name = attr->attr.name;
	int             err;
	unsigned int    a, b, c;
	unsigned long   flags;

	if (!capable(CAP_NET_ADMIN))
		return -EPERM;

	/* Read port and value */
	err = a = b = c = 0;
	sscanf(buf, "%d %d %d", &a, &b, &c);

	local_irq_save(flags);

	if (!strcmp(name, "poolRegs")) {
		mvPp2BmPoolRegs(a);
	} else if (!strcmp(name, "poolDropCnt")) {
		mvBmV1PoolDropCntDump(a);
	} else if (!strcmp(name, "poolStatus")) {
		mv_pp2_pool_status_print(a);
	} else if (!strcmp(name, "poolSize")) {
		err = mv_pp2_ctrl_pool_size_set(a, b);
	} else if (!strcmp(name, "poolBufNum")) {
		err = mv_pp2_ctrl_pool_buf_num_set(a, b, c);
	} else if (!strcmp(name, "longPool")) {
		err = mv_pp2_ctrl_long_pool_set(a, b);
	} else if (!strcmp(name, "shortPool")) {
		err = mv_pp2_ctrl_short_pool_set(a, b);
	} else if (!strcmp(name, "hwfLongPool")) {
		err = mv_pp2_ctrl_hwf_long_pool_set(a, b);
	} else if (!strcmp(name, "hwfShortPool")) {
		err = mv_pp2_ctrl_hwf_short_pool_set(a, b);
	} else if (!strcmp(name, "qsetCreate")) {
		mvBmQsetCreate(a, b);
	} else if (!strcmp(name, "qsetDelete")) {
		mvBmQsetDelete(a);
	} else if (!strcmp(name, "rxqQsetLong")) {
		mvBmRxqToQsetLongSet(a, b);
	} else if (!strcmp(name, "rxqQsetShort")) {
		mvBmRxqToQsetShortSet(a, b);
	} else if (!strcmp(name, "txqQsetLong")) {
		mvBmTxqToQsetLongSet(a, b);
	} else if (!strcmp(name, "txqQsetShort")) {
		mvBmTxqToQsetShortSet(a, b);
	} else if (!strcmp(name, "qsetMaxSet")) {
		mvBmQsetBuffMaxSet(a, b, c);
	} else if (!strcmp(name, "qsetShow")) {
		mvBmQsetShow(a);
	} else {
		err = 1;
		printk(KERN_ERR "%s: illegal operation <%s>\n", __func__, attr->attr.name);
	}

	local_irq_restore(flags);

	if (err)
		printk(KERN_ERR "%s: error %d\n", __func__, err);

	return err ? -EINVAL : len;
}

static DEVICE_ATTR(help,		S_IRUSR, mv_pp2_show, NULL);
static DEVICE_ATTR(queueMappDump,	S_IRUSR, mv_pp2_show, NULL);
static DEVICE_ATTR(qsetConfigDump,	S_IRUSR, mv_pp2_show, NULL);
static DEVICE_ATTR(poolRegs,		S_IWUSR, NULL, mv_pp2_port_store);
static DEVICE_ATTR(poolDropCnt,		S_IWUSR, NULL, mv_pp2_port_store);
static DEVICE_ATTR(poolStatus,		S_IWUSR, NULL, mv_pp2_port_store);
static DEVICE_ATTR(poolSize,		S_IWUSR, NULL, mv_pp2_port_store);
static DEVICE_ATTR(poolBufNum,		S_IWUSR, NULL, mv_pp2_port_store);
static DEVICE_ATTR(longPool,		S_IWUSR, NULL, mv_pp2_port_store);
static DEVICE_ATTR(shortPool,		S_IWUSR, NULL, mv_pp2_port_store);
static DEVICE_ATTR(hwfLongPool,		S_IWUSR, NULL, mv_pp2_port_store);
static DEVICE_ATTR(hwfShortPool,	S_IWUSR, NULL, mv_pp2_port_store);

static DEVICE_ATTR(qsetCreate,		S_IWUSR, NULL, mv_pp2_port_store);
static DEVICE_ATTR(qsetDelete,		S_IWUSR, NULL, mv_pp2_port_store);
static DEVICE_ATTR(rxqQsetLong,	S_IWUSR, NULL, mv_pp2_port_store);
static DEVICE_ATTR(rxqQsetShort,	S_IWUSR, NULL, mv_pp2_port_store);
static DEVICE_ATTR(txqQsetLong,	S_IWUSR, NULL, mv_pp2_port_store);
static DEVICE_ATTR(txqQsetShort,	S_IWUSR, NULL, mv_pp2_port_store);
static DEVICE_ATTR(qsetMaxSet,		S_IWUSR, NULL, mv_pp2_port_store);
static DEVICE_ATTR(qsetShow,		S_IWUSR, NULL, mv_pp2_port_store);

static struct attribute *mv_pp2_attrs[] = {
	&dev_attr_help.attr,
	&dev_attr_queueMappDump.attr,
	&dev_attr_qsetConfigDump.attr,
	&dev_attr_poolRegs.attr,
	&dev_attr_poolDropCnt.attr,
	&dev_attr_poolStatus.attr,
	&dev_attr_poolSize.attr,
	&dev_attr_poolBufNum.attr,
	&dev_attr_longPool.attr,
	&dev_attr_shortPool.attr,
	&dev_attr_hwfLongPool.attr,
	&dev_attr_hwfShortPool.attr,
	&dev_attr_qsetCreate.attr,
	&dev_attr_qsetDelete.attr,
	&dev_attr_rxqQsetLong.attr,
	&dev_attr_rxqQsetShort.attr,
	&dev_attr_txqQsetLong.attr,
	&dev_attr_txqQsetShort.attr,
	&dev_attr_qsetMaxSet.attr,
	&dev_attr_qsetShow.attr,
	NULL
};

static struct attribute_group mv_pp2_bm_group = {
	.name = "bm",
	.attrs = mv_pp2_attrs,
};

int mv_pp2_bm_sysfs_init(struct kobject *pp2_kobj)
{
	int err;

	err = sysfs_create_group(pp2_kobj, &mv_pp2_bm_group);
	if (err)
		printk(KERN_INFO "sysfs group failed %d\n", err);

	return err;
}

int mv_pp2_bm_sysfs_exit(struct kobject *pp2_kobj)
{
	sysfs_remove_group(pp2_kobj, &mv_pp2_bm_group);

	return 0;
}

