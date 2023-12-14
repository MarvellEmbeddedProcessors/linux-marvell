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
#include <linux/mm.h>
#include <linux/jhash.h>
#include <linux/rhashtable.h>
#include <linux/seq_file.h>
#include <linux/debugfs.h>

#include "pan_cmn.h"

static RADIX_TREE(tbls_rdx_tree_h, GFP_KERNEL);

static struct pan_fl_tbl_rdx_node *
pan_rdxn[PAN_FL_TBL_TYPE_MAX];

struct pan_fl_tbl_rdx_node *
pan_fl_tbl_rdx_node_get(enum pan_fl_tbl_type type)
{
	return pan_rdxn[type];
}

static u32 pan_fl_tbl_hash(const void *data, u32 len, u32 seed)
{
	const struct pan_tuple *tuple = data;
	u32 hash;

	if (pan_tuple_flags_is_set(tuple, PAN_TUPLE_FLAG_HASH))
		return pan_tuple_hash_get(tuple);

	hash = jhash(tuple, offsetof(struct pan_tuple, __end), seed);
	/* first 16 bits are reserved for flows based on NPC match id
	 * clear first 16 bits
	 */
	hash = hash & 0xffff0000;
	pan_tuple_hash_set((struct pan_tuple *)tuple, hash);

	return hash;
}

static u32 pan_fl_tbl_hash_obj(const void *data, u32 len, u32 seed)
{
	const struct pan_fl_tbl_node *tn = data;
	u32 hash;

	if (pan_tuple_flags_is_set(&tn->tuple, PAN_TUPLE_FLAG_HASH))
		return pan_tuple_hash_get(&tn->tuple);

	hash = jhash(&tn->tuple, offsetof(struct pan_tuple, __end), seed);
	/* first 16 bits are reserved for flows based on NPC match id
	 * clear first 16 bits
	 */
	hash = hash & 0xffff0000;
	pan_tuple_hash_set((struct pan_tuple *)&tn->tuple, hash);

	return hash;
}

/* TODO: optimize by adding different functions for ipv4 and ipv6 */
static int pan_fl_tbl_hash_cmp(struct rhashtable_compare_arg *arg,
			       const void *ptr)
{
	const struct pan_tuple *tuple = arg->key;
	const struct pan_fl_tbl_node *tn = ptr;

	if (memcmp(&tn->tuple, tuple, offsetof(struct pan_tuple, __end)))
		return 1;

	return 0;
}

static struct rhashtable_params rht_v4_params = {
	.head_offset		= offsetof(struct pan_fl_tbl_node, rh_node),
	.hashfn			= pan_fl_tbl_hash,
	.key_offset             = offsetof(struct pan_fl_tbl_node, tuple),
	.obj_hashfn		= pan_fl_tbl_hash_obj,
	.obj_cmpfn		= pan_fl_tbl_hash_cmp,
	.automatic_shrinking	= true,
};

static int pan_fl_tbl_offl_hash_cmp(struct rhashtable_compare_arg *arg,
				    const void *ptr)
{
	const struct pan_tuple *tuple = arg->key;
	const struct pan_fl_tbl_node *tn = ptr;

	if (!pan_tuple_flags_is_set(&tn->tuple, PAN_TUPLE_FLAG_HASH))
		return 1;

	if (tn->tuple.hash != tuple->hash)
		return 1;

	return 0;
}

static struct rhashtable_params *
pan_fl_tbl_rht_params_get(enum pan_fl_tbl_type type)
{
	return &rht_v4_params;
}

static struct rhashtable_params rht_offl_params = {
	.head_offset		= offsetof(struct pan_fl_tbl_node, rh_node),
	.hashfn			= pan_fl_tbl_hash,
	.key_offset             = offsetof(struct pan_fl_tbl_node, tuple),
	.obj_hashfn		= pan_fl_tbl_hash_obj,
	.obj_cmpfn		= pan_fl_tbl_offl_hash_cmp,
	.automatic_shrinking	= true,
};

enum pan_fl_tbl_type pan_fl_tbl_find_type(struct pan_tuple *tuple)
{
	if (tuple->flags & PAN_TUPLE_FLAG_L3_PROTO_V4)
		return PAN_FL_TBL_TYPE_IPV4;

	WARN_ON_ONCE(!(tuple->flags & PAN_TUPLE_FLAG_L3_PROTO_V6));

	return PAN_FL_TBL_TYPE_IPV6;
}

static struct pan_fl_tbl_rdx_node *
pan_fl_tble_init_one_table(enum pan_fl_tbl_type type)
{
	struct rhashtable_params *rht_params;
	struct pan_fl_tbl_rdx_node *node;
	int rc;

	node = kvzalloc(sizeof(*node), GFP_KERNEL_ACCOUNT);
	if (!node)
		return NULL;

	rht_params = pan_fl_tbl_rht_params_get(type);
	if (!rht_params) {
		pr_err("Could not find apt rht params for type=%u\n", type);
		return NULL;
	}

	node->index = type;
	node->rht_params = *rht_params;
	spin_lock_init(&node->lock);

	/* Initialize hash table */
	rc = rhashtable_init(&node->ht, rht_params);
	if (rc) {
		pr_err("Error happened to init %u type table\n", type);
		return NULL;
	}

	/* Init stage, no locking required */
	rc = radix_tree_insert(&tbls_rdx_tree_h, node->index, node);
	if (rc) {
		pr_err("Error happened to add %u type table\n", type);
		return NULL;
	}

	return node;
}

int __pan_fl_tbl_offl_lookup_n_res(struct pan_tuple *tuple, struct pan_fl_tbl_res **res)
{
	struct pan_fl_tbl_rdx_node *rdx;
	struct pan_fl_tbl_node *node;
	int err = 0;

	rcu_read_lock_bh();

	/* TODO: fix PAN_FL_TBL_TYPE_IPV4 ? */
	rdx = radix_tree_lookup(&tbls_rdx_tree_h, PAN_FL_TBL_TYPE_IPV4);
	if (unlikely(!rdx)) {
		pr_err("Not able to find radix node for type=%u\n", PAN_FL_TBL_TYPE_IPV4);
		err = -ENOENT;
		goto err;
	}

	node = rhashtable_lookup_fast(&rdx->ht, tuple, rht_offl_params);
	if (!node) {
		pr_debug("Not able to find node\n");
		PAN_TUPLE_DUMP(tuple);
		err = -ESRCH;
		goto err;
	}

	this_cpu_inc(*node->hits);

	*res = &node->res;
err:
	rcu_read_unlock_bh();
	return err;
}

int __pan_fl_tbl_lookup_n_res(struct pan_tuple *tuple, struct pan_fl_tbl_res **res)
{
	struct pan_fl_tbl_rdx_node *rdx;
	struct pan_fl_tbl_node *node;
	enum pan_fl_tbl_type type;
	int err = 0;

	rcu_read_lock_bh();

	type = pan_fl_tbl_find_type(tuple);
	rdx = radix_tree_lookup(&tbls_rdx_tree_h, type);
	if (unlikely(!rdx)) {
		pr_err("Not able to find radix node for type=%u\n", type);
		err = -ENOENT;
		goto err;
	}

	node = rhashtable_lookup_fast(&rdx->ht, tuple, rdx->rht_params);
	if (!node) {
		pr_err("Not able to find node\n");
		PAN_TUPLE_DUMP(tuple);
		err = -ESRCH;
		goto err;
	}

	this_cpu_inc(*node->hits);

	*res = &node->res;
err:
	rcu_read_unlock_bh();
	return err;
}

int pan_fl_tbl_lookup(struct pan_tuple *tuple)
{
	struct pan_fl_tbl_rdx_node *rdx;
	struct pan_fl_tbl_node *node;
	enum pan_fl_tbl_type type;
	int err = 0;

	rcu_read_lock_bh();

	type = pan_fl_tbl_find_type(tuple);
	rdx = radix_tree_lookup(&tbls_rdx_tree_h, type);
	if (unlikely(!rdx)) {
		pr_err("Not able to find radix node for type=%u\n", type);
		err = -ENOENT;
		goto err;
	}

	node = rhashtable_lookup_fast(&rdx->ht, tuple, rdx->rht_params);
	if (!node) {
		pr_err("Not able to find node\n");
		PAN_TUPLE_DUMP(tuple);
		err = -ESRCH;
		goto err;
	}

err:
	rcu_read_unlock_bh();
	return err;
}

int pan_fl_tbl_lookup_and_cb(struct pan_tuple *tuple, struct sk_buff *skb)
{
	struct pan_fl_tbl_rdx_node *rdx;
	struct pan_fl_tbl_node *node;
	enum pan_fl_tbl_type type;
	int ret = -1;
	int err = 0;

	rcu_read_lock_bh();

	type = pan_fl_tbl_find_type(tuple);
	rdx = radix_tree_lookup(&tbls_rdx_tree_h, type);
	if (unlikely(!rdx)) {
		pr_debug("Not able to find radix node for type=%u\n", type);
		err = -ENOENT;
		goto err;
	}

	node = rhashtable_lookup_fast(&rdx->ht, tuple, rdx->rht_params);
	if (!node) {
		pr_debug("%s", "Not able to find node\n");
		PAN_TUPLE_DUMP(tuple);
		err = -ESRCH;
		goto err;
	}

	this_cpu_inc(*node->hits);

	if (node->ig_cb)
		ret = node->ig_cb(&node->tuple, skb, node->arg);

	pr_debug("Ingress cb executed ret=%d\n", ret);

	if (node->eg_cb)
		ret = node->eg_cb(&node->tuple, skb, node->arg);

	pr_debug("Egress cb executed ret=%d\n", ret);

err:
	rcu_read_unlock_bh();
	return err;
}

static int pan_fl_tbl_register_cb(struct pan_tuple *tuple, tbl_node_cb cb,
				  void *arg, bool is_ig)
{
	struct pan_fl_tbl_rdx_node *rdx;
	struct pan_fl_tbl_node *node;
	enum pan_fl_tbl_type type;
	tbl_node_cb *fn;

	type = pan_fl_tbl_find_type(tuple);
	rdx = radix_tree_lookup(&tbls_rdx_tree_h, type);
	if (unlikely(!rdx)) {
		pr_err("Not able to find radix node for type=%u\n", type);
		return -ENOENT;
	}

	spin_lock(&rdx->lock);

	node = rhashtable_lookup_fast(&rdx->ht, tuple, rdx->rht_params);
	if (!node) {
		spin_unlock(&rdx->lock);
		pr_err("Not able to find node\n");
		PAN_TUPLE_DUMP(tuple);
		return -ESRCH;
	}

	fn = is_ig ? &node->ig_cb : &node->eg_cb;
	if (*fn) {
		spin_unlock(&rdx->lock);
		pr_err("cb is already set (is_ig=%u)\n", is_ig);
		return -EFAULT;
	}

	*fn = cb;
	node->arg = arg;
	spin_unlock(&rdx->lock);

	return 0;
}

int pan_fl_tbl_register_ig_cb(struct pan_tuple *tuple, tbl_node_cb cb,
			      void *arg)
{
	return pan_fl_tbl_register_cb(tuple, cb, arg, true);
}

int pan_fl_tbl_register_eg_cb(struct pan_tuple *tuple, tbl_node_cb cb,
			      void *arg)
{
	return pan_fl_tbl_register_cb(tuple, cb, arg, true);
}

static struct pr_debugfs_info {
	enum pan_fl_tbl_type type;
} v4_debug_info, v6_debug_info;

static u64 get_all_cpu_total(u64 __percpu *cntr)
{
	int cpu;
	u64 counter = 0;

	for_each_possible_cpu(cpu)
		counter += *per_cpu_ptr(cntr, cpu);
	return counter;
}

int __pan_fl_tbl_offl_get_hit_cnt(struct pan_tuple *tuple, u64 *hits)
{
	struct pan_fl_tbl_rdx_node *rdx;
	struct pan_fl_tbl_node *node;
	int err = 0;

	rcu_read_lock_bh();

	/* TODO: fix PAN_FL_TBL_TYPE_IPV4 ? */
	rdx = radix_tree_lookup(&tbls_rdx_tree_h, PAN_FL_TBL_TYPE_IPV4);
	if (unlikely(!rdx)) {
		pr_err("Not able to find radix node for type=%u\n",
		       PAN_FL_TBL_TYPE_IPV4);
		err = -ENOENT;
		goto err;
	}

	node = rhashtable_lookup_fast(&rdx->ht, tuple, rht_offl_params);
	if (!node) {
		pr_err("Not able to find node\n");
		PAN_TUPLE_DUMP(tuple);
		err = -ESRCH;
		goto err;
	}

	*hits = get_all_cpu_total(node->hits);
err:
	rcu_read_unlock_bh();
	return err;
}

static int pr_debugfs_show(struct seq_file *m, void *v)
{
	struct pr_debugfs_info *info;
	struct pan_fl_tbl_rdx_node *rdx;
	struct pan_fl_tbl_node *node;
	struct rhashtable_iter hti;
	struct pan_rvu_gbl_t *gbl;
	struct rhash_head *pos;
	int total = 0;
	char buf[256];
	int len = 0;
	u16 pcifunc;

	info = m->private;

	rdx = radix_tree_lookup(&tbls_rdx_tree_h, info->type);
	if (unlikely(!rdx)) {
		seq_printf(m, "Not able to find radix node for type=%u\n",
			   info->type);
		return 0;
	}

	rhashtable_walk_enter(&rdx->ht, &hti);
	rhashtable_walk_start(&hti);

	gbl = pan_rvu_get_gbl();

	while ((pos = rhashtable_walk_next(&hti))) {
		len = 0;
		if (PTR_ERR(pos) == -EAGAIN) {
			pr_info("Info: encountered resize\n");
			continue;
		} else if (IS_ERR(pos)) {
			pr_warn("Test failed: rhashtable_walk_next() error: %ld\n",
				PTR_ERR(pos));
			break;
		}

		node = container_of(pos, struct pan_fl_tbl_node, rh_node);

		if (node->ig_cb)
			len += snprintf(buf + len, sizeof(buf) - len,
					"%s", "ig_cb,");
		if (node->eg_cb)
			len += snprintf(buf + len, sizeof(buf) - len,
					"%s", "eg_cb,");
		if (node->eg_cb)
			len += snprintf(buf + len, sizeof(buf) - len,
					"%s", "arg,");

		pan_tuple_dump2sysfs(m, &node->tuple, total);

		len += snprintf(buf + len, sizeof(buf) - len, "(%s),",
				(node->res.dir & FLOW_OFFLOAD_DIR_REPLY) ?
				"BIDI" : "UNIDI");

		pcifunc = gbl->sqoff2pcifunc[node->res.pcifuncoff];
		len += snprintf(buf + len, sizeof(buf) - len,
				"(pcifunc %#x),", pcifunc);

		len += snprintf(buf + len, sizeof(buf) - len,
				"(hits %llu),",
				get_all_cpu_total(node->hits));

		if (node->ig_cb)
			len += snprintf(buf + len, sizeof(buf) - len,
					"%s", "ig_cb,");
		if (node->eg_cb)
			len += snprintf(buf + len, sizeof(buf) - len,
					"%s", "eg_cb,");
		if (node->eg_cb)
			len += snprintf(buf + len, sizeof(buf) - len,
					"%s", "arg,");
		if (len) {
			buf[strlen(buf) - 1] = '\0';
			seq_printf(m, "(%s)", buf);
		}

		seq_puts(m, "\n");

		total++;
	}

	rhashtable_walk_stop(&hti);
	rhashtable_walk_exit(&hti);

	return 0;
}
DEFINE_SHOW_ATTRIBUTE(pr_debugfs);

static void pan_fl_tbl_debugfs_deinit(void)
{
	struct dentry *parent;

	parent = debugfs_lookup("cn10k", NULL);
	if (!parent)
		parent = debugfs_lookup("octeontx2", NULL);

	if (!parent) {
		pr_err("Could not find dir cn10ka or octeontx2 in debugfs\n");
		return;
	}

	parent = debugfs_lookup("pan", parent);
	if (!parent)
		return;

	debugfs_remove_recursive(parent);
}

static int pan_fl_tbl_debugfs_init(void)
{
	struct dentry *parent;
	struct dentry *file;

	parent = debugfs_lookup("cn10k", NULL);
	if (!parent)
		parent = debugfs_lookup("octeontx2", NULL);

	if (!parent) {
		pr_err("Could not find dir cn10ka or octeontx2 in debugfs\n");
		return -ESRCH;
	}

	parent = debugfs_create_dir("pan", parent);
	if (!parent) {
		pr_err("Could not create dir pan\n");
		return -ESRCH;
	}

	v4_debug_info.type = PAN_FL_TBL_TYPE_IPV4;
	file = debugfs_create_file("v4_fl_tbl", 0400, parent, (void *)&v4_debug_info,
				   &pr_debugfs_fops);
	if (!file) {
		pr_err("Could not create v4_fl_tbl debugfs entry\n");
		return -EFAULT;
	}

	v6_debug_info.type = PAN_FL_TBL_TYPE_IPV6;
	file = debugfs_create_file("v6_fl_tbl", 0600, parent, (void *)&v6_debug_info,
				   &pr_debugfs_fops);
	if (!file) {
		pr_err("Could not create v6_fl_tbl debugfs entry\n");
		return -EFAULT;
	}

	return 0;
}

int pan_fl_tbl_offl_del(struct pan_tuple *tuple)
{
	struct pan_fl_tbl_res *res, *rpair;
	struct pan_fl_tbl_rdx_node *rdx;
	struct pan_fl_tbl_node *node;
	int err;

	rcu_read_lock_bh();

	/* TODO: fix this */
	rdx = radix_tree_lookup(&tbls_rdx_tree_h, PAN_FL_TBL_TYPE_IPV4);
	if (unlikely(!rdx)) {
		pr_err("Not able to find radix node for type=%u\n",
		       PAN_FL_TBL_TYPE_IPV4);
		err = -ENOENT;
		goto err;
	}

	spin_lock(&rdx->lock);
	node = rhashtable_lookup_fast(&rdx->ht, tuple, rht_offl_params);
	if (!node) {
		spin_unlock(&rdx->lock);
		pr_err("Not able to find node\n");
		PAN_TUPLE_DUMP(tuple);
		err = -ESRCH;
		goto err;
	}

	free_percpu(node->hits);

	res = &node->res;

	if (res->pair) {
		rpair = res->pair;
		rpair->pair = NULL;
		rpair->dir = FLOW_OFFLOAD_DIR_ORIGINAL;

		res->pair = NULL;
		res->dir = FLOW_OFFLOAD_DIR_ORIGINAL;
	}

	err = rhashtable_remove_fast(&rdx->ht, &node->rh_node,
				     rht_offl_params);
	spin_unlock(&rdx->lock);

	if (err) {
		pr_err("Err happened while removing the node\n");
		PAN_TUPLE_DUMP(tuple);
		err = -EFAULT;
		goto err;
	}

	tuple->flags &= ~PAN_TUPLE_FLAG_HASH;
	kvfree_rcu(node, rcu);

err:
	rcu_read_unlock_bh();
	return err;
}

int pan_fl_tbl_del(struct pan_tuple *tuple)
{
	struct pan_fl_tbl_res *res, *rpair;
	struct pan_fl_tbl_rdx_node *rdx;
	struct pan_fl_tbl_node *node;
	enum pan_fl_tbl_type type;
	int err;

	rcu_read_lock_bh();

	type =  pan_fl_tbl_find_type(tuple);
	rdx = radix_tree_lookup(&tbls_rdx_tree_h, type);
	if (unlikely(!rdx)) {
		pr_err("Not able to find radix node for type=%u\n", type);
		err = -ENOENT;
		goto err;
	}

	spin_lock(&rdx->lock);
	node = rhashtable_lookup_fast(&rdx->ht, tuple, rdx->rht_params);
	if (!node) {
		spin_unlock(&rdx->lock);
		pr_err("Not able to find node\n");
		PAN_TUPLE_DUMP(tuple);
		err = -ESRCH;
		goto err;
	}

	free_percpu(node->hits);

	if (res->pair) {
		rpair = res->pair;
		rpair->pair = NULL;
		rpair->dir = FLOW_OFFLOAD_DIR_ORIGINAL;

		res->pair = NULL;
		res->dir = FLOW_OFFLOAD_DIR_ORIGINAL;
	}

	err = rhashtable_remove_fast(&rdx->ht, &node->rh_node,
				     rdx->rht_params);
	spin_unlock(&rdx->lock);

	if (err) {
		pr_err("Err happened while removing the node\n");
		PAN_TUPLE_DUMP(tuple);
		err = -EFAULT;
		goto err;
	}

	tuple->flags &= ~PAN_TUPLE_FLAG_HASH;
	kvfree_rcu(node, rcu);

err:
	rcu_read_unlock_bh();
	return err;
}

int pan_fl_tbl_add(struct pan_tuple *tuple, struct pan_fl_tbl_res *res, u64 *handle)
{
	struct pan_fl_tbl_node *node, *tmp;
	struct pan_fl_tbl_rdx_node *rdx;
	enum pan_fl_tbl_type type;
	int err = 0;

	node = kvzalloc(sizeof(*node), GFP_KERNEL_ACCOUNT);
	if (!node)
		return -ENOMEM;

	node->hits = alloc_percpu(u64);
	if (!node->hits) {
		pr_err("hits per cpu allocation failed\n");
		kvfree(node);
		return -ENOMEM;
	}

	rcu_read_lock_bh();

	type =  pan_fl_tbl_find_type(tuple);
	rdx = radix_tree_lookup(&tbls_rdx_tree_h, type);
	if (unlikely(!rdx)) {
		pr_err("Not able to find radix node for type=%u\n", type);
		err = -ESRCH;
		goto err;
	}

	node->tuple = *tuple;
	node->res = *res;

	spin_lock(&rdx->lock);
	tmp = rhashtable_lookup_fast(&rdx->ht, tuple, rdx->rht_params);
	if (tmp) {
		spin_unlock(&rdx->lock);
		pr_debug("%s", "entry exist already\n");
		PAN_TUPLE_DUMP(tuple);
		err = -EEXIST;
		goto err;
	}

	err = rhashtable_insert_fast(&rdx->ht, &node->rh_node,
				     rdx->rht_params);

	if (handle)
		*handle = (u64)(&node->tuple);

	spin_unlock(&rdx->lock);

	if (err) {
		pr_debug("%s", "entry insert failed\n");
		PAN_TUPLE_DUMP(tuple);
		err = -EFAULT;
		goto err;
	}

	rcu_read_unlock_bh();
	return 0;

err:
	rcu_read_unlock_bh();
	free_percpu(node->hits);
	node->hits = NULL;
	kvfree(node);
	return err;
}

int pan_fl_tbl_offl_add(struct pan_tuple *tuple, struct pan_fl_tbl_res *res)
{
	struct pan_fl_tbl_node *node, *tmp;
	struct pan_fl_tbl_rdx_node *rdx;
	int err = 0;

	node = kvzalloc(sizeof(*node), GFP_KERNEL_ACCOUNT);
	if (!node)
		return -ENOMEM;

	node->hits = alloc_percpu(u64);
	if (!node->hits) {
		kvfree(node);
		return -ENOMEM;
	}

	rcu_read_lock_bh();

	/* TODO: fix IPV4 table ? */
	rdx = radix_tree_lookup(&tbls_rdx_tree_h, PAN_FL_TBL_TYPE_IPV4);
	if (unlikely(!rdx)) {
		pr_err("Not able to find radix node for type=%u\n",
		       PAN_FL_TBL_TYPE_IPV4);
		err = -ENOENT;
		goto err;
	}

	node->tuple = *tuple;
	node->res = *res;

	spin_lock(&rdx->lock);
	tmp = rhashtable_lookup_fast(&rdx->ht, tuple, rht_offl_params);
	if (tmp) {
		spin_unlock(&rdx->lock);
		pr_debug("%s", "entry exist already\n");
		PAN_TUPLE_DUMP(tuple);
		err = -EEXIST;
		goto err;
	}

	err = rhashtable_insert_fast(&rdx->ht, &node->rh_node,
				     rht_offl_params);

	spin_unlock(&rdx->lock);

	if (err) {
		pr_debug("%s", "entry insert failed\n");
		PAN_TUPLE_DUMP(tuple);
		err = -EFAULT;
		goto err;
	}

	rcu_read_unlock_bh();
	return 0;

err:
	rcu_read_unlock_bh();
	free_percpu(node->hits);
	node->hits = NULL;
	kvfree(node);
	return err;
}

static void pan_fl_tbl_node_free_fn(void *ptr, void *arg)
{
	struct pan_fl_tbl_node *node = ptr;

	free_percpu(node->hits);
	kvfree(node);
}

void pan_fl_tbl_deinit(void)
{
	struct pan_fl_tbl_rdx_node *node;
	struct radix_tree_iter iter;
	void __rcu **slot;

	pan_fl_tbl_debugfs_deinit();

	radix_tree_for_each_slot(slot, &tbls_rdx_tree_h, &iter, 0) {
		node = radix_tree_deref_slot(slot);
		pr_debug("Destroying table %lu type table\n", node->index);
		rhashtable_free_and_destroy(&node->ht, pan_fl_tbl_node_free_fn, NULL);
		radix_tree_delete(&tbls_rdx_tree_h, iter.index);
		kvfree(node);
	}
}

int pan_fl_tbl_init(void)
{
	struct pan_fl_tbl_rdx_node *n;
	u8 i;

	for (i = 0; i < PAN_FL_TBL_TYPE_MAX; i++) {
		n = pan_fl_tble_init_one_table(i);
		if (!n)
			goto err;
		pan_rdxn[i] = n;
		pr_debug("Initialize table type %u\n", i);
	}

	pan_fl_tbl_debugfs_init();
	return 0;
err:
	pan_fl_tbl_deinit();
	return -EFAULT;
}
