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
	struct npc_priv_t *npc_priv;
	struct npc_subbank *sb;
	unsigned int bw0, bw1;
	int i, j, sbd;
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

				map = xa_load(&npc_priv->xa_idx2pf_map, sb->b0b + j);
				pf1 = xa_to_value(map);

				seq_printf(s, "\t%u(%#x)\n", sb->b0b + j, pf1);
			}
			goto next;
		}

		bw1 = bitmap_weight(sb->b1map, npc_priv->subbank_depth);
		seq_printf(s, "\n\nsubbank:%u, x2, free=%u, used=%u\n",
			   sb->idx, sb->free_cnt, bw0 + bw1);
		seq_printf(s, "bank1(%u)\t\tbank0(%u)\n", bw1, bw0);

		for (j = sbd - 1; j >= 0; j--) {
			e0 = test_bit(j, sb->b0map);
			e1 = test_bit(j, sb->b1map);

			if (!e1 && !e0)
				continue;

			if (e1 && e0) {
				map = xa_load(&npc_priv->xa_idx2pf_map, sb->b0b + j);
				pf1 = xa_to_value(map);

				map = xa_load(&npc_priv->xa_idx2pf_map, sb->b1b + j);
				pf2 = xa_to_value(map);

				seq_printf(s, "%u(%#x)\t\t%u(%#x)\n",
					   sb->b1b + j, pf2,
					   sb->b0b + j, pf1);

				continue;
			}

			if (e0) {
				map = xa_load(&npc_priv->xa_idx2pf_map, sb->b0b + j);
				pf1 = xa_to_value(map);
				seq_printf(s, "\t\t\t%u(%#x)\n", sb->b0b + j, pf1);
				continue;
			}

			map = xa_load(&npc_priv->xa_idx2pf_map, sb->b1b + j);
			pf1 = xa_to_value(map);
			seq_printf(s, "%u(%#x)\n", sb->b1b + j, pf1);
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

	return 0;
}

void npc_cn20k_debugfs_deinit(struct rvu *rvu)
{
	debugfs_remove_recursive(rvu->rvu_dbg.npc);
}
