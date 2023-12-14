// SPDX-License-Identifier: GPL-2.0
/* Marvell RVU PAN driver
 *
 * Copyright (C) 2025 Marvell.
 *
 */
#include <linux/module.h>
#include <linux/skbuff.h>
#include <linux/rhashtable.h>

#include "pan_cmn.h"

static int __init pan_init(void)
{
	int rc;

	rc = pan_fl_tbl_init();
	if (rc) {
		pr_err("PAN fl table initialization failed\n");
		goto err1;
	}

	rc = pan_test_init();
	if (rc) {
		pr_err("PAN test initialization failed\n");
		goto err2;
	}

	rc = pan_rvu_init();
	if (rc) {
		pr_err("PAN rvu initialization failed\n");
		goto err3;
	}

	rc = pan_stats_init();
	if (rc) {
		pr_err("PAN stats initialization failed\n");
		goto err4;
	}

	rc = pan_tl_init();
	if (rc) {
		pr_err("PAN tl initialization failed\n");
		goto err5;
	}

	rc = pan_sw_init();
	if (rc) {
		pr_err("PAN switch initialization failed\n");
		goto err6;
	}

	return 0;

err6:
	pan_tl_deinit();
err5:
	pan_stats_deinit();

err4:
	pan_rvu_deinit();

err3:
	pan_test_deinit();

err2:
	pan_fl_tbl_deinit();

err1:
	return rc;
}

static void __exit pan_deinit(void)
{
	pan_sw_deinit();
	pan_tl_deinit();
	pan_stats_deinit();
	pan_rvu_deinit();
	pan_test_deinit();
	pan_fl_tbl_deinit();
}

module_init(pan_init);
module_exit(pan_deinit);

MODULE_AUTHOR("Marvell International Ltd.");
MODULE_DESCRIPTION("PAN driver");
MODULE_LICENSE("GPL v2");
