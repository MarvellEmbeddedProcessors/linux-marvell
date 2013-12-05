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
#include "plcr/mvPp2PlcrHw.h"


static ssize_t plcr_help(char *buf)
{
	int off = 0;

	off += scnprintf(buf + off, PAGE_SIZE - off, "all arguments are decimal numbers\n");
	off += scnprintf(buf + off, PAGE_SIZE - off, "\n");

	off += scnprintf(buf + off, PAGE_SIZE - off, "cat             help      - Show this help\n");
	off += scnprintf(buf + off, PAGE_SIZE - off, "cat             regs      - Show PLCR hardware registers\n");
	off += scnprintf(buf + off, PAGE_SIZE - off, "cat             dump      - Dump all policers configuration and status\n");
	off += scnprintf(buf + off, PAGE_SIZE - off, "echo p        > dump      - Dump policer <p> configuration and status\n");
	off += scnprintf(buf + off, PAGE_SIZE - off, "echo p        > v1_tb_dump- Dump policer <p> token bucket counters\n");


	off += scnprintf(buf + off, PAGE_SIZE - off, "echo period   > period    - Set token update base period\n");
	off += scnprintf(buf + off, PAGE_SIZE - off, "echo 0|1      > rate      - Enable <1> or Disable <0> addition of tokens to token buckets\n");
	off += scnprintf(buf + off, PAGE_SIZE - off, "echo bytes    > min_pkt   - Set minimal packet length\n");
	off += scnprintf(buf + off, PAGE_SIZE - off, "echo 0|1      > edrop     - Enable <1> or Disable <0> early packets drop\n");
#ifdef CONFIG_MV_ETH_PP2_1
	off += scnprintf(buf + off, PAGE_SIZE - off, "echo mode     > mode      - Set policer mode of operation 0-bank01 1-bank10 2-parallal\n");
#endif
	off += scnprintf(buf + off, PAGE_SIZE - off, "echo p 0|1    > enable    - Enable <1> or Disable <0> policer <p>\n");
	off += scnprintf(buf + off, PAGE_SIZE - off, "echo p 0|1    > color     - Set color mode for policer <p>: 0-blind, 1-aware\n");
	off += scnprintf(buf + off, PAGE_SIZE - off, "echo p u t    > config    - Set token units <u> and update type <t> for policer <p>\n");
	off += scnprintf(buf + off, PAGE_SIZE - off, "echo p num    > tokens    - Set number of tokens for each update for policer <p>\n");
	off += scnprintf(buf + off, PAGE_SIZE - off, "echo p c e    > bucket    - Set commit <c> and exceed <e> bucket sizes for policer <p>\n");
	off += scnprintf(buf + off, PAGE_SIZE - off, "echo i tr     > cpu_v0_tr - Set value <tr> to CPU (SWF) threshold <i>\n");
	off += scnprintf(buf + off, PAGE_SIZE - off, "echo i tr     > hwf_v0_tr - Set value <tr> to HWF threshold <i>\n");
	off += scnprintf(buf + off, PAGE_SIZE - off, "echo i tr     > cpu_v1_tr - Set value <tr> to CPU (SWF) threshold <i>\n");
	off += scnprintf(buf + off, PAGE_SIZE - off, "echo i tr     > hwf_v1_tr - Set value <tr> to HWF threshold <i>\n");
	off += scnprintf(buf + off, PAGE_SIZE - off, "echo rxq i    > rxq_tr    - Set thershold <i> to be used for RXQ <rxq>\n");
	off += scnprintf(buf + off, PAGE_SIZE - off, "echo txq i    > txq_tr    - Set thershold <i> to be used for TXQ <txq>\n");

	return off;
}

static ssize_t plcr_show(struct device *dev,
			struct device_attribute *attr, char *buf)
{
	const char  *name = attr->attr.name;

	if (!capable(CAP_NET_ADMIN))
		return -EPERM;

	if (!strcmp(name, "help"))
		return plcr_help(buf);

	if (!strcmp(name, "regs")) {
		mvPp2PlcrHwRegs();
	} else	if (!strcmp(name, "dump")) {
		mvPp2PlcrHwDumpAll();
	} else {
		printk(KERN_ERR "%s: illegal operation <%s>\n", __func__, attr->attr.name);
		return -EINVAL;
	}
	return 0;
}

static ssize_t plcr_dec_store(struct device *dev,
			struct device_attribute *attr, const char *buf, size_t len)
{
	const char    *name = attr->attr.name;
	unsigned int  err = 0, p = 0, i = 0, v = 0;
	unsigned long flags;

	if (!capable(CAP_NET_ADMIN))
		return -EPERM;

	sscanf(buf, "%d %d %d", &p, &i, &v);

	local_irq_save(flags);

	if (!strcmp(name, "dump")) {
		mvPp2PlcrHwDumpSingle(p);
	} else if (!strcmp(name, "v1_tb_dump")) {
		mvPp2V1PlcrTbCntDump(p);
	} else	if (!strcmp(name, "period")) {
		mvPp2PlcrHwBasePeriodSet(p);
	} else	if (!strcmp(name, "rate")) {
		mvPp2PlcrHwBaseRateGenEnable(p);
	} else	if (!strcmp(name, "min_pkt")) {
		mvPp2PlcrHwMinPktLen(p);
	} else	if (!strcmp(name, "edrop")) {
		mvPp2PlcrHwEarlyDropSet(p);
	} else	if (!strcmp(name, "enable")) {
		mvPp2PlcrHwEnable(p, i);
	/* only for ppv2.1 */
	} else	if (!strcmp(name, "mode")) {
		mvPp2PlcrHwMode(p);
	} else  if (!strcmp(name, "color")) {
		mvPp2PlcrHwColorModeSet(p, i);
	} else	if (!strcmp(name, "config")) {
		mvPp2PlcrHwTokenConfig(p, i, v);
	} else	if (!strcmp(name, "tokens")) {
		mvPp2PlcrHwTokenValue(p, i);
	} else	if (!strcmp(name, "bucket")) {
		mvPp2PlcrHwBucketSizeSet(p, i, v);
	} else	if (!strcmp(name, "cpu_v0_tr")) {
		mvPp2V0PlcrHwCpuThreshSet(p, i);
	} else	if (!strcmp(name, "cpu_v1_tr")) {
		mvPp2V1PlcrHwCpuThreshSet(p, i);
	} else	if (!strcmp(name, "hwf_v0_tr")) {
		mvPp2V0PlcrHwHwfThreshSet(p, i);
	} else	if (!strcmp(name, "hwf_v1_tr")) {
		mvPp2V1PlcrHwHwfThreshSet(p, i);
	} else	if (!strcmp(name, "rxq_tr")) {
		mvPp2PlcrHwRxqThreshSet(p, i);
	} else	if (!strcmp(name, "txq_tr")) {
		mvPp2PlcrHwTxqThreshSet(p, i);
	} else
		printk(KERN_ERR "%s: illegal operation <%s>\n", __func__, name);

	local_irq_restore(flags);

	if (err)
		printk(KERN_ERR "%s: <%s>, error %d\n", __func__, attr->attr.name, err);

	return err ? -EINVAL : len;
}


static DEVICE_ATTR(help,          S_IRUSR, plcr_show, NULL);
static DEVICE_ATTR(regs,          S_IRUSR, plcr_show, NULL);
static DEVICE_ATTR(dump,          S_IRUSR | S_IWUSR, plcr_show, plcr_dec_store);

static DEVICE_ATTR(v1_tb_dump,    S_IWUSR, NULL,     plcr_dec_store);
static DEVICE_ATTR(period,        S_IWUSR, NULL,     plcr_dec_store);
static DEVICE_ATTR(rate,          S_IWUSR, NULL,     plcr_dec_store);
static DEVICE_ATTR(min_pkt,       S_IWUSR, NULL,     plcr_dec_store);
static DEVICE_ATTR(edrop,         S_IWUSR, NULL,     plcr_dec_store);
static DEVICE_ATTR(enable,        S_IWUSR, NULL,     plcr_dec_store);
/*mode - only for ppv2.1 */
static DEVICE_ATTR(mode  ,        S_IWUSR, NULL,     plcr_dec_store);
static DEVICE_ATTR(color,         S_IWUSR, NULL,     plcr_dec_store);
static DEVICE_ATTR(config,        S_IWUSR, NULL,     plcr_dec_store);
static DEVICE_ATTR(tokens,        S_IWUSR, NULL,     plcr_dec_store);
static DEVICE_ATTR(bucket,        S_IWUSR, NULL,     plcr_dec_store);
static DEVICE_ATTR(cpu_v0_tr,     S_IWUSR, NULL,     plcr_dec_store);
static DEVICE_ATTR(hwf_v0_tr,     S_IWUSR, NULL,     plcr_dec_store);
static DEVICE_ATTR(cpu_v1_tr,     S_IWUSR, NULL,     plcr_dec_store);
static DEVICE_ATTR(hwf_v1_tr,     S_IWUSR, NULL,     plcr_dec_store);
static DEVICE_ATTR(rxq_tr,        S_IWUSR, NULL,     plcr_dec_store);
static DEVICE_ATTR(txq_tr,        S_IWUSR, NULL,     plcr_dec_store);


static struct attribute *plcr_attrs[] = {
	&dev_attr_help.attr,
	&dev_attr_regs.attr,
	&dev_attr_dump.attr,
	&dev_attr_v1_tb_dump.attr,
	&dev_attr_period.attr,
	&dev_attr_rate.attr,
	&dev_attr_min_pkt.attr,
	&dev_attr_edrop.attr,
	&dev_attr_enable.attr,
	/* mode - only for ppv2.1 */
	&dev_attr_mode.attr,
	&dev_attr_color.attr,
	&dev_attr_config.attr,
	&dev_attr_tokens.attr,
	&dev_attr_bucket.attr,
	&dev_attr_cpu_v0_tr.attr,
	&dev_attr_hwf_v0_tr.attr,
	&dev_attr_cpu_v1_tr.attr,
	&dev_attr_hwf_v1_tr.attr,
	&dev_attr_rxq_tr.attr,
	&dev_attr_txq_tr.attr,

	NULL
};

static struct attribute_group plcr_group = {
	.name = "plcr",
	.attrs = plcr_attrs,
};

int mv_pp2_plcr_sysfs_init(struct kobject *pp2_kobj)
{
	int err;

	err = sysfs_create_group(pp2_kobj, &plcr_group);
	if (err)
		printk(KERN_INFO "sysfs group %s failed %d\n", plcr_group.name, err);

	return err;
}

int mv_pp2_plcr_sysfs_exit(struct kobject *pp2_kobj)
{
	sysfs_remove_group(pp2_kobj, &plcr_group);

	return 0;
}

