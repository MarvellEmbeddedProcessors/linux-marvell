// SPDX-License-Identifier: GPL-2.0
/* Marvell RVU Admin Function driver
 *
 * Copyright (C) 2024 Marvell.
 *
 */

#include <linux/debugfs.h>

#include "struct.h"
#include "rvu.h"
#include "debugfs.h"
#include "cn20k/npc.h"

static int npc_mcam_layout_show(struct seq_file *s, void *unused)
{
	int i, j, sbd, idx0, idx1, vidx0, vidx1;
	struct npc_priv_t *npc_priv;
	char buf0[32], buf1[32];
	struct npc_subbank *sb;
	unsigned int bw0, bw1;
	bool v0, v1;
	int pf1, pf2;
	bool e0, e1;
	void *map;

	npc_priv = s->private;

	sbd = npc_priv->subbank_depth;

	for (i = npc_priv->num_subbanks - 1; i >= 0; i--) {
		sb = &npc_priv->sb[i];
		mutex_lock(&sb->lock);

		if (sb->flags & NPC_SUBBANK_FLAG_FREE)
			goto next;

		bw0 = bitmap_weight(sb->b0map, npc_priv->subbank_depth);
		if (sb->key_type == NPC_MCAM_KEY_X4) {
			seq_printf(s, "\n\nsubbank:%u, x4, free=%u, used=%u\n",
				   sb->idx, sb->free_cnt, bw0);

			for (j = sbd - 1; j >= 0; j--) {
				if (!test_bit(j, sb->b0map))
					continue;

				idx0 = sb->b0b + j;
				map = xa_load(&npc_priv->xa_idx2pf_map, idx0);
				pf1 = xa_to_value(map);

				map = xa_load(&npc_priv->xa_idx2vidx_map, idx0);
				if (map) {
					vidx0 = xa_to_value(map);
					snprintf(buf0, sizeof(buf0), "v:%u", vidx0);
				}

				seq_printf(s, "\t%u(%#x) %s\n", idx0, pf1,
					   map ? buf0 : " ");
			}
			goto next;
		}

		bw1 = bitmap_weight(sb->b1map, npc_priv->subbank_depth);
		seq_printf(s, "\n\nsubbank:%u, x2, free=%u, used=%u\n",
			   sb->idx, sb->free_cnt, bw0 + bw1);
		seq_printf(s, "bank1(%03u)   vidx\t\tbank0(%03u)   vidx\n", bw1, bw0);

		for (j = sbd - 1; j >= 0; j--) {
			e0 = test_bit(j, sb->b0map);
			e1 = test_bit(j, sb->b1map);

			if (!e1 && !e0)
				continue;

			if (e1 && e0) {
				idx0 = sb->b0b + j;
				map = xa_load(&npc_priv->xa_idx2pf_map, idx0);
				pf1 = xa_to_value(map);

				map = xa_load(&npc_priv->xa_idx2vidx_map, idx0);
				v0 = !!map;
				if (v0) {
					vidx0 = xa_to_value(map);
					snprintf(buf0, sizeof(buf0), "v:%05u", vidx0);
				}

				idx1 = sb->b1b + j;
				map = xa_load(&npc_priv->xa_idx2pf_map, idx1);
				pf2 = xa_to_value(map);

				map = xa_load(&npc_priv->xa_idx2vidx_map, idx1);
				v1 = !!map;
				if (v1) {
					vidx1 = xa_to_value(map);
					snprintf(buf1, sizeof(buf1), "v:%05u", vidx1);
				}

				seq_printf(s, "%05u(%#x) %s\t\t%05u(%#x) %s\n",
					   idx1, pf2, v1 ? buf1 : "       ",
					   idx0, pf1, v0 ? buf0 : "       ");

				continue;
			}

			if (e0) {
				idx0 = sb->b0b + j;
				map = xa_load(&npc_priv->xa_idx2pf_map, idx0);
				pf1 = xa_to_value(map);
				map = xa_load(&npc_priv->xa_idx2vidx_map, idx0);
				if (map) {
					vidx0 = xa_to_value(map);
					snprintf(buf0, sizeof(buf0), "v:%05u", vidx0);
				}

				seq_printf(s, "\t\t   \t\t%05u(%#x) %s\n", idx0, pf1,
					   map ? buf0 : " ");
				continue;
			}

			idx1 = sb->b1b + j;
			map = xa_load(&npc_priv->xa_idx2pf_map, idx1);
			pf1 = xa_to_value(map);

			map = xa_load(&npc_priv->xa_idx2vidx_map, idx1);
			if (map) {
				vidx1 = xa_to_value(map);
				snprintf(buf1, sizeof(buf1), "v:%05u", vidx1);
			}

			seq_printf(s, "%05u(%#x) %s\n", idx1, pf1,
				   map ? buf1 : " ");
		}
next:
		mutex_unlock(&sb->lock);
	}
	return 0;
}

DEFINE_SHOW_ATTRIBUTE(npc_mcam_layout);

static int npc_mcam_default_show(struct seq_file *s, void *unused)
{
	struct npc_priv_t *npc_priv;
	unsigned long index;
	u16 ptr[4], pcifunc;
	struct rvu *rvu;
	int rc, i;
	void *map;

	npc_priv = npc_priv_get();
	rvu = s->private;

	seq_puts(s, "\npcifunc\tBcast\tmcast\tpromisc\tucast\n");

	xa_for_each(&npc_priv->xa_pf_map, index, map) {
		pcifunc = index;

		for (i = 0; i < ARRAY_SIZE(ptr); i++)
			ptr[i] = USHRT_MAX;

		rc = npc_cn20k_dft_rules_idx_get(rvu, pcifunc, &ptr[0],
						 &ptr[1], &ptr[2], &ptr[3]);
		if (rc)
			continue;

		seq_printf(s, "%#x\t", pcifunc);
		for (i = 0; i < ARRAY_SIZE(ptr); i++) {
			if (ptr[i] != USHRT_MAX)
				seq_printf(s, "%u\t", ptr[i]);
			else
				seq_puts(s, "\t");
		}
		seq_puts(s, "\n");
	}

	return 0;
}
DEFINE_SHOW_ATTRIBUTE(npc_mcam_default);

static int npc_vidx2idx_map_show(struct seq_file *s, void *unused)
{
	struct npc_priv_t *npc_priv;
	unsigned long index, start;
	struct xarray *xa;
	void *map;

	npc_priv = s->private;
	start = npc_priv->bank_depth * 2;
	xa = &npc_priv->xa_vidx2idx_map;

	seq_puts(s, "\nvidx\tmcam_idx\n");

	xa_for_each_start(xa, index, map, start)
		seq_printf(s, "%lu\t%lu\n", index, xa_to_value(map));
	return 0;
}
DEFINE_SHOW_ATTRIBUTE(npc_vidx2idx_map);

static int npc_idx2vidx_map_show(struct seq_file *s, void *unused)
{
	struct npc_priv_t *npc_priv;
	unsigned long index;
	struct xarray *xa;
	void *map;

	npc_priv = s->private;
	xa = &npc_priv->xa_idx2vidx_map;

	seq_puts(s, "\nmidx\tvidx\n");

	xa_for_each(xa, index, map)
		seq_printf(s, "%lu\t%lu\n", index, xa_to_value(map));
	return 0;
}
DEFINE_SHOW_ATTRIBUTE(npc_idx2vidx_map);

static int npc_defrag_show(struct seq_file *s, void *unused)
{
	struct npc_defrag_show_node *node;
	struct npc_priv_t *npc_priv;
	u16 sbd, bdm;

	npc_priv = s->private;
	bdm = npc_priv->bank_depth - 1;
	sbd = npc_priv->subbank_depth;

	seq_puts(s, "\nold(sb)   ->    new(sb)\t\tvidx\n");

	mutex_lock(&npc_priv->lock);
	list_for_each_entry(node, &npc_priv->defrag_lh, list)
		seq_printf(s, "%u(%u)\t%u(%u)\t%u\n", node->old_midx,
			   (node->old_midx & bdm) / sbd,
			   node->new_midx,
			   (node->new_midx & bdm) / sbd,
			   node->vidx);
	mutex_unlock(&npc_priv->lock);
	return 0;
}

DEFINE_SHOW_ATTRIBUTE(npc_defrag);

int npc_cn20k_debugfs_init(struct rvu *rvu)
{
	struct npc_priv_t *npc_priv = npc_priv_get();
	struct dentry *npc_dentry;

	npc_dentry = debugfs_create_file("mcam_layout", 0444, rvu->rvu_dbg.npc,
					 npc_priv, &npc_mcam_layout_fops);

	if (!npc_dentry)
		return -EFAULT;

	npc_dentry = debugfs_create_file("mcam_default", 0444, rvu->rvu_dbg.npc,
					 rvu, &npc_mcam_default_fops);
	if (!npc_dentry)
		return -EFAULT;

	npc_dentry = debugfs_create_file("vidx2idx", 0444, rvu->rvu_dbg.npc, npc_priv,
					 &npc_vidx2idx_map_fops);
	if (!npc_dentry)
		return -EFAULT;

	npc_dentry = debugfs_create_file("idx2vidx", 0444, rvu->rvu_dbg.npc, npc_priv,
					 &npc_idx2vidx_map_fops);
	if (!npc_dentry)
		return -EFAULT;

	npc_dentry = debugfs_create_file("defrag", 0444, rvu->rvu_dbg.npc, npc_priv,
					 &npc_defrag_fops);
	if (!npc_dentry)
		return -EFAULT;

	return 0;
}

void npc_cn20k_debugfs_deinit(struct rvu *rvu)
{
	debugfs_remove_recursive(rvu->rvu_dbg.npc);
}
