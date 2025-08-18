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

static struct dentry *pan_dir;

struct dentry *pan_dbgfs_dir(void)
{
	struct dentry *parent;

	if (pan_dir)
		return pan_dir;

	parent = debugfs_lookup("cn10k", NULL);
	if (!parent) {
		pr_err("%s", "Could not find dir cn10ka in debugfs\n");
		return NULL;
	}

	/* pan fl tbl would be initialized before pan rvu */
	pan_dir = debugfs_lookup("pan", parent);
	if (!pan_dir)
		pan_dir = debugfs_create_dir("pan", parent);

	return pan_dir;
}

void pan_dbgfs_rm_full_dir(void)
{
	static struct dentry *parent;

	if (pan_dir) {
		parent = debugfs_lookup("cn10k", NULL);
		if (!parent)
			return;
		pan_dir = debugfs_lookup("pan", parent);
	}

	debugfs_remove_recursive(pan_dir);
}

void pan_dbgfs_rm_sub_dir(const char *dirname)
{
	static struct dentry *parent, *dir;

	if (!pan_dir) {
		parent = debugfs_lookup("cn10k", NULL);

		if (!parent)
			return;

		pan_dir = debugfs_lookup("pan", parent);
		if (!pan_dir)
			return;
	}

	dir = debugfs_lookup(dirname, pan_dir);
	if (!dir)
		return;

	debugfs_remove_recursive(dir);
}

void pan_dbgfs_rm_file(const char *fname)
{
	static struct dentry *parent, *file;

	if (!pan_dir) {
		parent = debugfs_lookup("cn10k", NULL);
		if (!parent)
			return;

		pan_dir = debugfs_lookup("pan", parent);
		if (!pan_dir)
			return;
	}

	file = debugfs_lookup(fname, pan_dir);
	if (!file)
		return;

	debugfs_remove(file);
}
