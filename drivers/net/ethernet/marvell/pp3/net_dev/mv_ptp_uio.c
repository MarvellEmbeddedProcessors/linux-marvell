/*
* ***************************************************************************
* Copyright (C) 2015 Marvell International Ltd.
* ***************************************************************************
* This program is free software: you can redistribute it and/or modify it
* under the terms of the GNU General Public License as published by the Free
* Software Foundation, either version 2 of the License, or any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program.  If not, see <http://www.gnu.org/licenses/>.
* ***************************************************************************
*/

/******************************************************************************
**
**      UIO deriver and device adding
**
** Gives direct access to TAI/PTP registers in user-space over UIO-mapping
** Complicated actions are executed over ".set = write_store_cmd":
**     write to /sys/module/mv_pp3/parameters/ts_tai_tod_uio
**
** If CONFIG_MV_PP3_PTP_SERVICE is not enabled, does nothing but
** provides stub function mv_pp3_ptp_tai_tod_uio_init()
*******************************************************************************
*/
#ifndef CONFIG_MV_PP3_PTP_SERVICE
int mv_pp3_ptp_tai_tod_uio_init(struct platform_device *shared_pdev)
{ return 0; }
#else /* CONFIG_MV_PP3_PTP_SERVICE */

/* includes */
#include <linux/kernel.h>
#include <linux/uio_driver.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/platform_device.h>
#include <linux/io.h>
#include <linux/of.h>

#include "gop/a390_mg_if.h"
#include "gop/mv_gop_if.h"
#include "gop/mv_ptp_regs.h"
#include "gop/mv_tai_regs.h"
#include "net_dev/mv_ptp_service.h"


#define TS_NAME	"ts_tai_tod"
#define TS_NAME_UIO	"ts_tai_tod_uio"

struct ts_ptp_uio {
	struct uio_info uio_info;
	/* Auxiliary parameters */
	u32 dedicated_mg_region;
	u32 dedicated_mg_region_offs;
};

static struct ts_ptp_uio *ts_ptp_uio;

static int read_show_cmd(char *buf, const struct kernel_param *kp)
{
	struct ts_ptp_uio *p = (void *)(*(u32 *)(kp->arg));
	if (p) {
		sprintf(buf, "region=%d region_offs=%x size=%lx",
			p->dedicated_mg_region, p->dedicated_mg_region_offs,
			(unsigned long)p->uio_info.mem[0].size);
		pr_debug("%s device is used with parameters: %s\n", kp->name, buf);
	}
	return strlen(buf);
}

static int write_store_cmd(const char *buf, const struct kernel_param *kp)
{
	struct mv_pp3_tai_tod *ts;
	int rc;
	if (buf[0]) {
		pr_err("%s: write/store called with ASCII: <%s>\n", TS_NAME_UIO, buf);
		return 0;
	}
	ts = (struct mv_pp3_tai_tod *)buf;
	if (ts->operation == MV_TAI_GET_CAPTURE) {
		/* For DEBUG only */
		/* mv_pp3_tai_tod_op(ts->operation, ts, 0); - already in "ts" */
		mv_pp3_tai_tod_dump_util(ts);
		return 0;
	}
	/* Real operation called over "parameters/ts_tai_tod_uio" write
	 * as an alternative to the UIO direct access in user space
	 */
	rc = mv_pp3_tai_tod_op(ts->operation, ts, 0);
	return rc;
}

static const struct kernel_param_ops param_ops = {
	.get = read_show_cmd,
	.set = write_store_cmd,
};
module_param_cb(ts_tai_tod_uio, &param_ops, &ts_ptp_uio, 0644);


static int ts_ptp_uio_probe(struct platform_device *pdev)
{
	int ret;
	u32 gop_pa;
	u32 gop_va;
	u32 gop_size;

	ts_ptp_uio = kzalloc(sizeof(struct ts_ptp_uio), GFP_KERNEL);
	if (!ts_ptp_uio) {
		pr_err("%s: out of memory\n", __func__);
		return -ENOMEM;
	}

	ret = mv_gop_addrs_size_get(&gop_va, &gop_pa, &gop_size);
	if (ret) {
		pr_err("%s: mv_gop_base_addr_get() failed\n", __func__);
		ret = 0; /* say OK to continue sys-up without this driver */
		goto err;
	}

	platform_set_drvdata(pdev, ts_ptp_uio);
	ts_ptp_uio->uio_info.name = TS_NAME;
	ts_ptp_uio->uio_info.version = "v1";
#ifdef MV_PP3_DEDICATED_MG_REGION
	/* Address-convert for TAI/PTP registers
	 *   TAI: 0x03180A00..0x03180B00
	 *   PTP: 0x03180800..0x03180874 ... port3:0x03183800..0x03183874
	 *   => TAI/PTP Register(offset) 0318pXXX
	 * With indirect MG address completion the final Mapping is:
	 *    0318pXXX -> REGION7(111b << 19) -> 0038pXXX
	 * TAI-access un User-space is like:
	 *    *(u32*)(gop_va + 0x00380000 + RegisterOFFS)
	 *
	 * Without dedicated region the UIO has no mmap and could be used
	 * by application only over read_show_cmd, write_store_cmd
	 */
	ts_ptp_uio->uio_info.mem[0].name = TS_NAME;
	ts_ptp_uio->uio_info.mem[0].addr = gop_pa;
	ts_ptp_uio->uio_info.mem[0].internal_addr = (void *)gop_va;
	ts_ptp_uio->uio_info.mem[0].size = gop_size;
	ts_ptp_uio->dedicated_mg_region = MV_PP3_DEDICATED_MG_REGION;
	ts_ptp_uio->dedicated_mg_region_offs = MV_PP3_DEDICATED_MG_REGION << 19;
	ts_ptp_uio->uio_info.mem[0].memtype = UIO_MEM_PHYS;
#endif
	ts_ptp_uio->uio_info.priv = ts_ptp_uio;

	if (uio_register_device(&pdev->dev, &ts_ptp_uio->uio_info)) {
		pr_err("%s: register device fails!\n", __func__);
		goto err;
	}
	return 0;
err:
	kfree(ts_ptp_uio);
	ts_ptp_uio = NULL;
	return ret;
}

static struct platform_driver ts_tai_tod_driver = {
	.driver = {
		.name = TS_NAME,
		.owner = THIS_MODULE
	},
};

int mv_pp3_ptp_tai_tod_uio_init(struct platform_device *shared_pdev)
{
	/* This probe-init is extention of mv_pp3_shared_probe()
	 * but called in very late stage (!) as part of PTP init.
	 * It is using the shared_pdev imported from net_dev
	 */
	int rc;

	/* Could be called more than once but only 1 created */
	if (ts_ptp_uio)
		return 0;

	if (!shared_pdev)
		return 0;
	rc = ts_ptp_uio_probe(shared_pdev);
	if (!rc)
		rc = platform_driver_register(&ts_tai_tod_driver);

	if (rc)
		pr_err("%s: Can't register %s driver. rc=%d\n", __func__, TS_NAME, rc);
	return rc;
}

MODULE_AUTHOR("Yan Markman");
MODULE_DESCRIPTION("UIO driver for Marvell TAI-ToD");
MODULE_LICENSE("GPL");
#endif /* CONFIG_MV_PP3_PTP_SERVICE */
