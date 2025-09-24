/* SPDX-License-Identifier: GPL-2.0-only
 * Copyright (C) 2021 Marvell.
 */

#ifndef __ZIP_VF_DEBUGFS_H__
#define __ZIP_VF_DEBUGFS_H__

#include "zip_vf.h"

struct zip_vf_registers {
	char *reg_name;
	u64 reg_offset;
};

int __init zip_vf_debugfs_init(void);
void __exit zip_vf_debugfs_exit(void);

#endif /* __ZIP_VF_DEBUGFS_H__ */
