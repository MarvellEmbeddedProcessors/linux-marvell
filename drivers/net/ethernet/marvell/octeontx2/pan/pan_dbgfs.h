/* SPDX-License-Identifier: GPL-2.0 */
/* Marvell PAN driver
 *
 * Copyright (C) 2025 Marvell.
 *
 */
#ifndef PAN_DBGFS_H_
#define PAN_DBGFS_H_
void pan_dbgfs_rm_full_dir(void);
void pan_dbgfs_rm_file(const char *fname);
void pan_dbgfs_rm_sub_dir(const char *dirname);
struct dentry *pan_dbgfs_dir(void);

#endif // End of PAN_DBGFS_H_
