// SPDX-License-Identifier: GPL-2.0
/* Marvell RVU PAN driver
 *
 * Copyright (C) 2025 Marvell.
 *
 */
#include <linux/types.h>
#include <linux/string.h>
#include <linux/errno.h>
#include <linux/stddef.h>
#include <linux/seq_file.h>
#include <linux/debugfs.h>
#include <linux/slab.h>

#include "pan_cmn.h"

static DEFINE_PER_CPU(struct pan_stats, pan_stats);

static char *pan_stats_fld_name[PAN_STATS_FLD_MAX] = {
	[PAN_STATS_FLD_IN_NON_SG_PKTS] = "IN_NORM\t\t",	// "ingress non sg pkts",
	[PAN_STATS_FLD_IN_SG_PKTS] = "IN_SG\t\t",		// "ingress sg pkt",
	[PAN_STATS_FLD_OUT_NON_SG_PKTS] = "OUT_NORM\t",	// "egress non sg pkts",
	[PAN_STATS_FLD_OUT_SG_PKTS] = "OUT_SG\t\t",		// "egress sg pkts",
	[PAN_STATS_FLD_DROP_PKTS] =  "DROPPED\t\t",		//"dropped pkts",
	[PAN_STATS_FLD_INTR] = "INTR_CNT\t",		// "interrupt cnt"
	[PAN_STATS_FLD_TX_DESC] =  "TX_DESC\t\t",		// "Not enough TX desc to send pkt",
	[PAN_STATS_FLD_SQE_THRESH] = "SQE_THRESH\t",	// Hit SQE thresh
	[PAN_STATS_FLD_RX_CQ_PKTS] = "RX CQ PKTS\t",		// CQ processed pkts
	[PAN_STATS_FLD_TX_CQ_PKTS] = "TX CQ PKTS\t",		// SQ processed pkts
	[PAN_STATS_FLD_INVAL_SQ] = "INVALID SQ\t",		// Invalid SQ
	[PAN_STATS_FLD_EXP_PKTS] = "EXCEPTION PKTS\t",		// Exception packets
};

static struct dentry *pan_stats_debugfs_dir(void)
{
	struct dentry *parent;

	parent = debugfs_lookup("cn10k", NULL);
	if (!parent)
		parent = debugfs_lookup("octeontx2", NULL);

	if (!parent) {
		pr_err("%s", "Could not find dir cn10ka or octeontx2 in debugfs\n");
		return NULL;
	}

	/* pan fl tbl would be initialized before pan rvu */
	parent = debugfs_lookup("pan", parent);
	if (!parent)
		return NULL;

	return parent;
}

static int pan_stats_dp_dbg_show(struct seq_file *s, void *file)
{
	struct pan_stats *stats, tot = { 0 };
	int i, cpu;

	stats = kcalloc(num_online_cpus(), sizeof(*stats), GFP_KERNEL);
	if (!stats) {
		seq_puts(s, "Not able to allocate memory\n");
		return 0;
	}

	for_each_online_cpu(cpu) {
		for (i = 0; i < PAN_STATS_FLD_MAX; i++) {
			stats[cpu].fld[i] = pan_stats_get(i, cpu);
			tot.fld[i] += stats[cpu].fld[i];
		}
	}

	seq_puts(s, "\ncpu\t\t");
	for_each_online_cpu(cpu)
		seq_printf(s, "%d\t", cpu);

	seq_puts(s, "total\n");

	for (i = 0; i < PAN_STATS_FLD_MAX; i++) {
		seq_puts(s, "\n");
		seq_printf(s, "%s", pan_stats_fld_name[i]);

		for_each_online_cpu(cpu)
			seq_printf(s, "%llu\t", stats[cpu].fld[i]);

		seq_printf(s, "%llu", tot.fld[i]);
	}

	seq_puts(s, "\n");
	kfree(stats);

	return 0;
}

static int pan_stats_dp_dbg_open(struct inode *inode, struct file *file)
{
	return single_open(file, pan_stats_dp_dbg_show, inode->i_private);
}

static const struct file_operations pan_stats_dp_dbg_ops = {
	.open		= pan_stats_dp_dbg_open,
	.read		= seq_read,
	.llseek		= seq_lseek,
	.release	= single_release,
};

u64 pan_stats_get(enum pan_stats_fld fld, int cpu)
{
	return *per_cpu_ptr(&pan_stats.fld[fld], cpu);
}

void pan_stats_inc(enum pan_stats_fld fld)
{
	this_cpu_inc(pan_stats.fld[fld]);
}

void pan_stats_add(enum pan_stats_fld fld, u32 cnt)
{
	this_cpu_add(pan_stats.fld[fld], cnt);
}

static int pan_stats_info_dbg_show(struct seq_file *s, void *file)
{
	struct net_device *dev, *netdev;
	struct pan_rvu_dev_priv *priv;
	struct iface_info *info;
	struct iface_info *tmp;
	int cnt, ret, i;

	dev = dev_get_by_name(&init_net, PAN_DEV_NAME);
	if (!dev) {
		pr_err("Could not find PAN device\n");
		return -EFAULT;
	}
	dev_put(dev);
	priv = netdev_priv(dev);

	info = kcalloc(256 + 32, sizeof(*info), GFP_KERNEL);
	if (!info) {
		seq_puts(s, "Could not allocate memory\n");
		return 0;
	}

	tmp = info;

	ret = pan_rvu_get_iface_info(info, &cnt, true);
	if (ret) {
		seq_puts(s, "Error happened while getting info\n");
		goto done;
	}

	seq_puts(s, "\n#\tis_pf\tis_sdp\tpcifunc\ttx_chan\tcnt\tlink\trx_chan\tcnt\tdev\n");
	for (i = 0; i < cnt; i++, info++) {
		if (priv->pcifunc == info->pcifunc)
			netdev = dev;
		else
			netdev = pan_rvu_get_kernel_netdev_by_pcifunc(info->pcifunc);

		seq_printf(s, "%u\t%s\t%s\t%#x\t%#x\t%u\t%u\t%#x\t%u\t%s\n", i,
			   info->is_vf ? "" : "yes",
			   info->is_sdp ? "yes" : "",
			   info->pcifunc, info->tx_chan_base,
			   info->tx_chan_cnt, info->tx_link,
			   info->rx_chan_base, info->rx_chan_cnt,
			   netdev ? netdev->name : "not in kernel");
	}

done:
	kfree(tmp);
	return 0;
}

static int pan_stats_info_dbg_open(struct inode *inode, struct file *file)
{
	return single_open(file, pan_stats_info_dbg_show, inode->i_private);
}

static const struct file_operations pan_stats_info_dbg_ops = {
	.open		= pan_stats_info_dbg_open,
	.read		= seq_read,
	.llseek		= seq_lseek,
	.release	= single_release,
};

static int pan_stats_sq_dbg_show(struct seq_file *s, void *file)
{
	struct pan_rvu_dev_priv *priv;
	struct pan_rvu_cq_info *cq_info;
	struct pan_rvu_sq_info *sq_info;
	struct pan_rvu_gbl_t *gbl;
	struct net_device *dev;
	struct otx2_hw *hw;
	int i, j;

	dev = dev_get_by_name(&init_net, PAN_DEV_NAME);
	if (!dev) {
		pr_err("Could not find PAN device\n");
		return -EFAULT;
	}
	dev_put(dev);

	gbl = pan_rvu_get_gbl();
	priv = netdev_priv(dev);
	hw = &priv->otx2_nic->hw;
	cq_info = priv->cq_info;

	seq_puts(s, "\n Global info\n");
	seq_printf(s, "sqs_total = %u\n", gbl->sqs_total);
	seq_printf(s, "sqs_usable = %u\n", gbl->sqs_usable);
	seq_printf(s, "sqs_per_core = %u\n", gbl->sqs_per_core);
	seq_printf(s, "sdp_cnt = %u\n", gbl->sdp_cnt);

	seq_printf(s, "\n sq_cnt_per_core=%u\n", cq_info->sq_cnt);

	for (i = 0; i < hw->cint_cnt; i++, cq_info++) {
		seq_printf(s, "\n*** CINT = %u ***\n", i);
		sq_info = cq_info->sq_info;

		seq_puts(s, "sq\tsq2cq\tpcifunc\tchan\n");

		for (j = 0; j < cq_info->sq_cnt; j++, sq_info++)
			seq_printf(s, "%u\t%u\t%#x\t%#x\n", sq_info->sqidx,
				   cq_info->sq2cqidxs[j],
				   sq_info->pcifunc, sq_info->tx_chan);
	}
	return 0;
}

static int pan_stats_sq_dbg_open(struct inode *inode, struct file *file)
{
	return single_open(file, pan_stats_sq_dbg_show, inode->i_private);
}

static const struct file_operations pan_stats_sq_dbg_ops = {
	.open		= pan_stats_sq_dbg_open,
	.read		= seq_read,
	.llseek		= seq_lseek,
	.release	= single_release,
};

void pan_stats_deinit(void)
{
	struct dentry *parent;

	parent = pan_stats_debugfs_dir();
	if (!parent)
		return;

	debugfs_remove(parent);
}

int pan_stats_init(void)
{
	struct dentry *parent, *file;

	parent = pan_stats_debugfs_dir();
	if (!parent)
		return -ESRCH;

	file = debugfs_create_file("stats", 0600, parent, NULL,
				   &pan_stats_dp_dbg_ops);

	if (!file)
		pr_err("%s", "Debugfs creation failed for pan stats\n");

	file = debugfs_create_file("info", 0600, parent, NULL,
				   &pan_stats_info_dbg_ops);

	if (!file)
		pr_err("%s", "Debugfs creation failed for pan info\n");

	file = debugfs_create_file("sq", 0600, parent, NULL,
				   &pan_stats_sq_dbg_ops);

	if (!file)
		pr_err("%s", "Debugfs creation failed for pan sq\n");

	return 0;
}
