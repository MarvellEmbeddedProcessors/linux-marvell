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

#include "mv_pp3.h"

#include <net/gnss/mv_nss_defs.h>
#include <linux/mbus.h>
#include <linux/phy.h>
#include <linux/of_net.h>
#include <linux/msi.h>
#ifdef CONFIG_HIGH_RES_TIMERS
#include <linux/hrtimer.h>
#include <linux/ktime.h>
#endif
#include "gop/mv_gop_if.h"
#include "gop/mv_ptp_if.h"
#include "gop/mv_smi.h"
#include "hmac/mv_hmac.h"
#include "emac/mv_emac.h"
#include "cmac/mv_cmac.h"
#include "fw/mv_fw.h"
#include "fw/mv_pp3_fw_msg_structs.h"
#include "fw/mv_pp3_fw_msg.h"
#include "msg/mv_pp3_msg_chan.h"
#include "bm/mv_bm.h"
#include "qm/mv_qm.h"
#include "tm/mv_tm.h"
#include "tm/wrappers/mv_tm_drop.h"
#include "vport/mv_pp3_vport.h"
#include "vport/mv_pp3_cpu.h"
#include "msg/mv_pp3_msg_drv.h"
#include "net_dev/mv_netdev.h"
#include "net_dev/mv_dev_sysfs.h"
#include "common/mv_sw_if.h"
#include "a390_gic_odmi_if.h"
#include "gnss/mv_pp3_gnss_api.h"
#include "coherency.h"

#define MV_PP3_SHARED_NAME        "mv_pp3_shared"

struct mv_pp3	*pp3_device;

/* Manage drop profiles for HWQ level */
struct pp3_dp_ctrl {
	u32 id;
	u16 td;
	u16 red;
	int ref_count;
};

bool coherency_hard_mode;

static u8   mv_pp3_dp_q_curve_id;
static struct pp3_dp_ctrl mv_pp3_dp_q_ctrl[MV_TM_NUM_QUEUE_DROP_PROF];

/* Reconfigure Drop profile */
static int mv_pp3_dp_q_set(int dp_id, u16 td, u16 red)
{
	int i, rc;
	struct mv_tm_drop_profile drop_profile;

	drop_profile.cbtd_threshold = (u32)(td * 1024 / 16);

	/* Colored Tail Drop Enable: 0 - WRED, 1 - CATD */
	drop_profile.color_td_en = 0;

	/* Set the same values for all colors */
	for (i = 0; i < MV_TM_NUM_OF_COLORS; i++) {
		drop_profile.min_threshold[i] = (u32)(red * 1024 / 16);
		drop_profile.max_threshold[i] = (u32)(td * 1024 / 16);
		drop_profile.curve_id[i] = mv_pp3_dp_q_curve_id;
		drop_profile.curve_scale[i] = 0;
	}
	rc = mv_tm_drop_profile_set(TM_Q_LEVEL, dp_id, -1, &drop_profile);
	if (rc) {
		pr_err("Can't set drop profile #%d. rc=%d\n", dp_id, rc);
		return rc;
	}

	pr_info("Drop profile #%d is created: td=%d, red=%d\n",
		dp_id, td, red);

	return 0;
}
/*---------------------------------------------------------------------------*/

/* Create default curve */
static int mv_pp3_dp_q_curve_create(void)
{
	int i, rc;

	/* Once time initialization. We will use the same curve for all drop profiles */
	memset(mv_pp3_dp_q_ctrl, 0, sizeof(mv_pp3_dp_q_ctrl));
	for (i = 0; i < MV_TM_NUM_QUEUE_DROP_PROF; i++)
		mv_pp3_dp_q_ctrl[i].id = i;

	/* for Q_LEVEL: cos=-1, maximum drop probability is 50% */
	rc = mv_tm_create_wred_curve(TM_Q_LEVEL, -1, 50, &mv_pp3_dp_q_curve_id);
	if (rc) {
		pr_err("Can't create WRED curve: err = %d\n", rc);
		return -1;
	}
	return 0;
}
/*---------------------------------------------------------------------------*/

int mv_pp3_dp_q_find(u16 td, u16 red)
{
	int i, rc;
	struct pp3_dp_ctrl *dp_ctrl = NULL;

	/* Look for existing drop profile with "td" and "red" values */
	for (i = 1; i < MV_TM_NUM_QUEUE_DROP_PROF; i++) {
		dp_ctrl = &mv_pp3_dp_q_ctrl[i];

		if ((dp_ctrl->ref_count > 0) && (dp_ctrl->td == td) && (dp_ctrl->red == red)) {
			dp_ctrl->ref_count++;
			return i;
		}
	}
	/* Look for unused drop profile */
	for (i = 1; i < MV_TM_NUM_QUEUE_DROP_PROF; i++) {
		dp_ctrl = &mv_pp3_dp_q_ctrl[i];
		if (dp_ctrl->ref_count == 0) {
			rc = mv_pp3_dp_q_set(i, td, red);
			if (rc)
				return rc;

			dp_ctrl->td = td;
			dp_ctrl->red = red;
			dp_ctrl->ref_count++;
			return i;
		}
	}
	pr_err("No free drop profile for td=%u, red=%u\n", td, red);
	return 0;
}
/*---------------------------------------------------------------------------*/

/* Free DP - decrement reference count */
void mv_pp3_dp_q_free(int dp_id)
{
	struct pp3_dp_ctrl *dp_ctrl = NULL;

	/* dp_id == 0 is default drop profile for ALL HWQs with no profile */
	if (dp_id != 0) {
		dp_ctrl = &mv_pp3_dp_q_ctrl[dp_id];
		if (dp_ctrl && dp_ctrl->ref_count > 0)
			dp_ctrl->ref_count--;
	}
}
/*---------------------------------------------------------------------------*/

int mv_pp3_nss_drain(struct mv_pp3 *priv)
{
	int cpu;

	/* stop all open network interfaces */
	/*mv_pp3_netdev_close_all();*/

	pr_info("%s: CMAC state wait ==> %d\n", __func__, mv_cmac_idle_state_check());
	mv_pp3_ppc_idle_wait_all();

	for_each_possible_cpu(cpu)
		mv_pp3_cpu_close(priv, cpu);


	tm_close();
	mv_pp3_drv_messenger_close();
	mv_pp3_messenger_close();

	mv_pp3_fw_memory_free(priv);

	priv->initialized = false;

	/* re-init configurator clients */
	mv_pp3_configurator_close();

	return 0;
}
/*---------------------------------------------------------------------------*/

/* Create sysfs commands tree under directory: "/sys/devices/platform/pp3" */
static int pp3_sysfs_init(struct mv_pp3 *priv)
{
	struct device *pd;

	pd = bus_find_device_by_name(&platform_bus_type, NULL, "pp3");
	if (!pd) {
		priv->sysfs_pdev = platform_device_register_simple("pp3", -1, NULL, 0);
		pd = bus_find_device_by_name(&platform_bus_type, NULL, "pp3");
	}

	if (!pd) {
		pr_err("%s: cannot find pp3 device\n", __func__);
		return -1;
	}
	mv_pp3_gop_sysfs_init(&pd->kobj);
	mv_pp3_emac_sysfs_init(&pd->kobj);
	mv_pp3_hmac_sysfs_init(&pd->kobj);
	mv_pp3_fw_sysfs_init(&pd->kobj);
	mv_pp3_bm_sysfs_init(&pd->kobj);
	mv_pp3_qm_sysfs_init(&pd->kobj);
	mv_pp3_tm_sysfs_init(&pd->kobj);
	mv_pp3_chan_sysfs_init(&pd->kobj);
	mv_pp3_dev_sysfs_init(&pd->kobj);
	mv_pp3_init_sysfs_init(&pd->kobj);
	mv_pp3_debug_sysfs_init(&pd->kobj);
	mv_pp3_cmac_sysfs_init(&pd->kobj);
	mv_pp3_vport_sysfs_init(&pd->kobj);

	return 0;
}
/*---------------------------------------------------------------------------*/

static void pp3_sysfs_exit(struct mv_pp3 *priv)
{
	struct device *pd;

	pd = bus_find_device_by_name(&platform_bus_type, NULL, "pp3");
	if (!pd) {
		pr_err("%s: cannot find pp3 device\n", __func__);
		return;
	}
	mv_pp3_vport_sysfs_exit(&pd->kobj);
	mv_pp3_debug_sysfs_exit(&pd->kobj);
	mv_pp3_init_sysfs_exit(&pd->kobj);
	mv_pp3_dev_sysfs_exit(&pd->kobj);
	mv_pp3_emac_sysfs_exit(&pd->kobj);
	mv_pp3_gop_sysfs_exit(&pd->kobj);
	mv_pp3_hmac_sysfs_exit(&pd->kobj);
	mv_pp3_cmac_sysfs_exit(&pd->kobj);
	mv_pp3_fw_sysfs_exit(&pd->kobj);
	mv_pp3_bm_sysfs_exit(&pd->kobj);
	mv_pp3_qm_sysfs_exit(&pd->kobj);
	mv_pp3_tm_sysfs_exit(&pd->kobj);
	mv_pp3_chan_sysfs_exit(&pd->kobj);

	platform_device_unregister(priv->sysfs_pdev);
}

/*---------------------------------------------------------------------------*/
#ifdef CONFIG_HIGH_RES_TIMERS
/* high resolution timer callback function */
static enum hrtimer_restart mv_pp3_hr_timer_callback(struct hrtimer *timer)
{
	struct mv_pp3_timer *pp3_timer = container_of(timer, struct mv_pp3_timer, hr_timer);

	if (pp3_timer->wq) {
		struct work_struct work = pp3_timer->timer_work->work;
		/* TODO debug error */
		if (!queue_work(pp3_timer->wq, &work))
			pr_err("%s: Internal error, work already in queue\n", __func__);
	} else
		tasklet_schedule(&pp3_timer->tasklet);

	return HRTIMER_NORESTART;
}
#endif /* CONFIG_HIGH_RES_TIMERS */
/*---------------------------------------------------------------------------*/

/* normal timer callback function */
static void mv_pp3_normal_timer_callback(unsigned long data)
{

	struct mv_pp3_timer *pp3_timer = (struct mv_pp3_timer *)data;
	STAT_INFO(pp3_timer->stats.timer_sched++);

	if (pp3_timer->wq) {
		if (!queue_work(pp3_timer->wq, &pp3_timer->timer_work->work))
			pr_err("%s: Internal error, work already in queue\n", __func__);
	} else
		tasklet_schedule(&pp3_timer->tasklet);
	return;
}

/* work queue callback function */
/*---------------------------------------------------------------------------*/
static void mv_pp3_workqueue_callback(struct work_struct *curr_work)
{
	struct pp3_timer_work *timer_work = container_of(curr_work, struct pp3_timer_work, work);
	timer_work->cb_func(timer_work->cookie);
}

/*---------------------------------------------------------------------------*/
/* pp3 timer initialization
	func   - callback function, must call to mv_pp3_timer_complete
	usec   - timer time interval
	cookie - cookie for user usage
*/
int mv_pp3_timer_init(struct mv_pp3_timer *pp3_timer, unsigned int cpu, unsigned int usec,
			enum mv_pp3_timer_internal_type type,
			void (*func)(unsigned long), unsigned long cookie)
{
	if (!pp3_timer) {
		pr_err("%s: timer pointer is NULL\n", __func__);
		return -EINVAL;
	}

	if (test_bit(MV_PP3_TIMER_INIT_BIT, &pp3_timer->flags)) {
		pr_err("%s: error - timer already initialized\n", __func__);
		return -1;
	}

	memset(pp3_timer, 0, sizeof(struct mv_pp3_timer));

#ifdef CONFIG_HIGH_RES_TIMERS
	/* Init high resolution timer */
	hrtimer_init(&pp3_timer->hr_timer, CLOCK_MONOTONIC, HRTIMER_MODE_REL_PINNED);
	pp3_timer->hr_timer.function = mv_pp3_hr_timer_callback;
#endif /* CONFIG_HIGH_RES_TIMERS */

	/* Init normal timer */
	init_timer(&pp3_timer->normal_timer);
	pp3_timer->normal_timer.function = mv_pp3_normal_timer_callback;
	pp3_timer->normal_timer.data = (unsigned long)pp3_timer;
	clear_bit(MV_PP3_TIMER_SCHED_BIT, &(pp3_timer->flags));
	set_bit(MV_PP3_TIMER_INIT_BIT, &(pp3_timer->flags));

	/* Tasklet init */
	if (type == MV_PP3_TASKLET) {
		tasklet_init(&pp3_timer->tasklet, func, cookie);
		pp3_timer->wq = NULL;

	} else {
		pp3_timer->wq = create_singlethread_workqueue("pp3_timer_wq");
		if (!pp3_timer->wq) {
			pr_err("%s - Out of memeory\n", __func__);
			return -1;
		}

		pp3_timer->timer_work = kmalloc(sizeof(struct pp3_timer_work), GFP_KERNEL);
		INIT_WORK(&pp3_timer->timer_work->work, mv_pp3_workqueue_callback);
		pp3_timer->timer_work->cookie = cookie;
		pp3_timer->timer_work->cb_func = func;
	}

	/* set timer usec and type */
	mv_pp3_timer_usec_set(pp3_timer, usec);
	pp3_timer->cpu = cpu;
	pr_info("PP3 timer (%s based) initialized on CPU #%d successfully\n",
		(type == MV_PP3_WORKQUEUE) ? "work queue" : "tasklet", cpu);

	return 0;
}
/*---------------------------------------------------------------------------*/
/* kill timer */
int mv_pp3_timer_kill(struct mv_pp3_timer *pp3_timer)
{
	if (!pp3_timer) {
		pr_err("%s: timer pointer is NULL\n", __func__);
		return -EINVAL;
	}
	if (pp3_timer->wq) {
		flush_workqueue(pp3_timer->wq);
		destroy_workqueue(pp3_timer->wq);
		kfree(pp3_timer->timer_work);
	} else
		tasklet_kill(&pp3_timer->tasklet);

	return 0;
}
/*---------------------------------------------------------------------------*/
int mv_pp3_timer_usec_set(struct mv_pp3_timer *pp3_timer, unsigned long usec)
{
	/* 1 jiffy is minimal time or normal timer */
	if (usec < jiffies_to_usecs(1)) {
#ifndef CONFIG_HIGH_RES_TIMERS
		pr_err("%s: Error - invalid time period, must be >= %d usec\n", __func__, jiffies_to_usecs(1));
		return -1;
#endif
		pp3_timer->type =  MV_PP3_HRES_TIMER;
	} else
		pp3_timer->type =  MV_PP3_NORMAL_TIMER;

	pp3_timer->usec = usec;

	return 0;
}

#if 0
/*---------------------------------------------------------------------------*/
/* return timer type that currently in use */
enum mv_pp3_timer_type mv_pp3_timer_type_get(struct mv_pp3_timer *pp3_timer)
{
	return pp3_timer->type;
}
#endif

/*---------------------------------------------------------------------------*/
/* Create and remap window to access GoP registers */
static int mv_pp3_gop_window_remap(struct platform_device *pdev, struct mv_io_addr *gop_addr)
{
	struct device_node *np = pdev->dev.of_node;
	struct device_node *gop_node;
	u32 target_id, win_attr;
	u32 remap_addr, gop_size, gop_pbase;

	/*
	 * The mvebu-mbus DT binding currently doesn't allow
	 * describing static windows with the remap capability, so we
	 * simply use the mvebu-mbus API to dynamically create the
	 * required window. This should be changed once mvebu-mbus is
	 * extended to cover such a case.
	 */
	gop_node = of_parse_phandle(np, "gop_access", 0);
	if (gop_node) {
		if (of_property_read_u32(gop_node, "gop-base", &gop_pbase)) {
			pr_err("could not get gop base\n");
			return -1;
		}
		if (of_property_read_u32(gop_node, "gop-size", &gop_size)) {
			pr_err("could not get gop size\n");
			return -1;
		}
		if (of_property_read_u32(gop_node, "mg-target-id", &target_id)) {
			pr_err("could not get target_id\n");
			return -1;
		}
		if (of_property_read_u32(gop_node, "mg-attr", &win_attr)) {
			pr_err("could not get win_attr\n");
			return -1;
		}
		if (of_property_read_u32(gop_node, "mg-remap-base", &remap_addr)) {
			pr_err("could not get remap_addr\n");
			return -1;
		}
		if (mvebu_mbus_add_window_remap_by_id(target_id, win_attr,
				(phys_addr_t)gop_pbase, (size_t)gop_size, (phys_addr_t)remap_addr)) {
			pr_err("can't remap gop window\n");
			return -1;
		}
		gop_addr->paddr = (phys_addr_t)gop_pbase;
		gop_addr->size = (size_t)gop_size;
	} else {
		pr_err("can't find gop_access node\n");
		return -1;
	}
	return 0;
}

static int mv_pp3_a2m_win_init(struct mv_pp3 *priv)
{
	int i, master;
	const struct mbus_dram_target_info *dram;

	/* First disable all A2M address decode windows */
	for (master = 0; master < MV_PP3_A2M_MAX_MASTER; master++) {
		for (i = 0; i < MV_PP3_A2M_MAX_DECODE_WIN; i++)
			mv_pp3_hw_reg_write(priv->a2m_regs[master].vaddr + MV_PP3_A2M_WIN_CTRL_REG(i), 0);
	}

	dram = mv_mbus_dram_info();
	if (!dram) {
		pr_err("%s: No DRAM information\n", __func__);
		return -ENODEV;
	}
	pr_info("A2M decoding windows initialization: DRAM info found - num_cs=%d\n", dram->num_cs);
	for (i = 0; i < dram->num_cs; i++) {
		const struct mbus_dram_window *cs = dram->cs + i;
		u32 baseReg, base = cs->base;
		u32 ctrlReg, size = cs->size;
		u8 attr = cs->mbus_attr;
		u8 target = dram->mbus_dram_target_id;

		/* check if address is aligned to the size */
		if (MV_IS_NOT_ALIGN(base, size)) {
			pr_err("%s: Wrong DRAM info cs=%d. Addr=0x%08x is not aligned to size=0x%x\n",
			   __func__, i, base, size);
			return -EINVAL;
		}

		if (!MV_IS_POWER_OF_2(size)) {
			pr_err("%s: Wrong DRAM info cs=%d. Size=0x%x is not a power of 2\n",
				__func__, i, size);
			return -EINVAL;
		}

		baseReg = (base & PP3_A2M_WIN_BASE_MASK);

		/* set size */
		ctrlReg = ((((size / (64 * 1024)) - 1) << PP3_A2M_WIN_SIZE_OFFS) & PP3_A2M_WIN_SIZE_MASK);

		/* set attributes */
		ctrlReg |= ((attr << PP3_A2M_WIN_ATTR_OFFS) & PP3_A2M_WIN_ATTR_MASK);

		/* set target ID */
		ctrlReg |= ((target << PP3_A2M_WIN_TARGET_OFFS) & PP3_A2M_WIN_TARGET_MASK);

		/* set user attribute enable bit */
		ctrlReg |= PP3_A2M_WIN_USR_ATTR_MASK;

		/* set user attribute enable bit */
		ctrlReg |= PP3_A2M_WIN_ENABLE_MASK;
		pr_info("%s: DRAM CS #%d, base=0x%x, size=0x%x, target=0x%x, attr=0x%x\n", __func__,
			i, base, size, target, attr);

		for (master = 0; master < MV_PP3_A2M_MAX_MASTER; master++) {
			mv_pp3_hw_reg_write(priv->a2m_regs[master].vaddr + MV_PP3_A2M_WIN_CTRL_REG(i), ctrlReg);
			mv_pp3_hw_reg_write(priv->a2m_regs[master].vaddr + MV_PP3_A2M_WIN_BASE_REG(i), baseReg);
			pr_info("%s: DRAM CS #%d, Master=%d, baseReg: %p=0x%x, ctrlReg: %p=0x%x\n", __func__,
				i, master, priv->a2m_regs[master].vaddr + MV_PP3_A2M_WIN_BASE_REG(i), baseReg,
					priv->a2m_regs[master].vaddr + MV_PP3_A2M_WIN_CTRL_REG(i), ctrlReg);
		}
	}

	for (master = 0; master < MV_PP3_A2M_MAX_MASTER; master++) {
		u32 ctrlReg;
		/* open window to ODMI registers */
		ctrlReg = ((1 << PP3_A2M_WIN_TARGET_OFFS) & PP3_A2M_WIN_TARGET_MASK);
		/* set user attribute enable bit */
		ctrlReg |= PP3_A2M_WIN_ENABLE_MASK;

		/* Init 64K window for internal registers */
		mv_pp3_hw_reg_write(priv->a2m_regs[master].vaddr + MV_PP3_A2M_WIN_CTRL_REG(i), ctrlReg);
		mv_pp3_hw_reg_write(priv->a2m_regs[master].vaddr + MV_PP3_A2M_WIN_BASE_REG(i),
					priv->int_regs_paddr + MV_A390_GIC_REGS_OFFS);

		/* init AMB registers */
		mv_pp3_hw_reg_write(priv->amb_regs.vaddr + PP3_AMB_CTRL0_REG(master),
					priv->int_regs_paddr + MV_A390_GIC_REGS_OFFS);
		mv_pp3_hw_reg_write(priv->amb_regs.vaddr + PP3_AMB_CTRL1_REG(master), 0);
		mv_pp3_hw_reg_write(priv->amb_regs.vaddr + PP3_AMB_MASK0_REG(master), 0x7fff);
		mv_pp3_hw_reg_write(priv->amb_regs.vaddr + PP3_AMB_MASK1_REG(master), 0);
	}
	return 0;
}

/* Print information from struct mv_pp3 */
void mv_pp3_device_show(struct mv_pp3 *priv)
{
	int master;

	pr_info("Core Tclock rate        : %d MHz\n", priv->tclk_hz / (1000 * 1000));
	pr_info("Number of ports         : %d\n", priv->ports_num);
	pr_info("IRQ base                : %d\n", priv->irq_base);
	pr_info("NSS registers base      : PHYS = %pa, VIRT = %p, size = %d KBytes\n",
		&priv->nss_regs.paddr, priv->nss_regs.vaddr, priv->nss_regs.size / 1024);
	pr_info("GoP registers base      : PHYS = %pa, VIRT = %p, size = %d KBytes\n",
		&priv->gop_regs.paddr, priv->gop_regs.vaddr, priv->gop_regs.size / 1024);
	pr_info("AMB registers base      : PHYS = %pa, VIRT = %p, size = %d KBytes\n",
		&priv->amb_regs.paddr, priv->amb_regs.vaddr, priv->amb_regs.size / 1024);
	for (master = 0; master < MV_PP3_A2M_MAX_MASTER; master++) {
		pr_info("A2M_%d registers base    : PHYS = %pa, VIRT = %p, size = %d KBytes\n",
			master, &priv->a2m_regs[master].paddr,
			priv->a2m_regs[master].vaddr, priv->a2m_regs[master].size / 1024);
	}
	pr_info("Short pool buffer size  : %d\n", priv->short_pool_buf_size);
}

/*---------------------------------------------------------------------------*/
/* HW initialization of pools 0-3 (QM internal pools)			     */
/*---------------------------------------------------------------------------*/
static int mv_pp3_bm_qm_init(void)
{
	int ppc_num, hw_txq, hwq_num, ret_val = 0;
	struct pp3_pool *pools[2];

	/* config rd/wr dram attributes  */
	bm_attr_all_pools_def_set();

	pools[0] = mv_pp3_pool_get(BM_QM_GPM_POOL_0);
	pools[1] = mv_pp3_pool_get(BM_QM_GPM_POOL_1);

	mv_pp3_pools_qm_gpm_sw_init(pools[0], pools[1]);
	ret_val = mv_pp3_pools_qm_hw_init(pools[0], pools[1]);
	if (ret_val < 0)
		goto err;

	pools[0] = mv_pp3_pool_get(BM_QM_DRAM_POOL_0);
	pools[1] = mv_pp3_pool_get(BM_QM_DRAM_POOL_1);
	if (pools[0] || pools[1]) {
		mv_pp3_pools_qm_dram_sw_init(pools[0], pools[1]);
		ret_val = mv_pp3_pools_qm_hw_init(pools[0], pools[1]);
		if (ret_val < 0)
			goto err;
	}

	bm_enable();

	qm_clear_hw_config();
	ppc_num = mv_pp3_fw_ppc_num_get();
	qm_default_set(ppc_num);
	qm_dma_gpm_pools_def_enable();

	mv_pp3_cfg_dp_hw_txq_get(&hw_txq, &hwq_num);

	/* set hmac->ppc queues for secret machine */
	qm_xoff_hmac_qs_set(hw_txq, hwq_num);

	/* set hmac threshold profile,
	   attached hmac->ppc queues to profile */

	qm_hmac_profile_set(hw_txq, hwq_num);

	return 0;
err:
	pr_err("%s: function failed\n", __func__);
	return -1;
}
/*---------------------------------------------------------------------------*/

int mv_pp3_shared_start(struct mv_pp3 *priv)
{
	int cpu, frame, profile, rc;
	struct mv_pp3_version fw_ver, *drv_ver;
	char *version_name;
	int ver_name_size;

	if (priv->initialized)
		return 0;

	/* smi init */
	mv_gop_smi_init();

	/* load fw */
	if (mv_pp3_fw_load()) {
		pr_err("Firmware load failed\n");
		return -1;
	}

	/* default frame configuration */
	for (frame = 0; frame < MV_PP3_HFRM_NUM; frame++) {
		mv_pp3_hmac_frame_cfg(frame, 0);

		for (profile = 0; profile < MV_PP3_HFRM_TIME_COAL_PROF_NUM; profile++)
			mv_pp3_hmac_frame_time_coal_set(frame, profile, CONFIG_MV_PP3_RX_COAL_USEC);
	}

#ifdef CONFIG_MV_PP3_TM_SUPPORT
	tm_cfg1();
#endif /* CONFIG_MV_PP3_TM_SUPPORT */

	/* Create default DP curve */
	mv_pp3_dp_q_curve_create();

	/* init configurator clients */
	mv_pp3_configurator_init(priv);

	mv_pp3_bm_qm_init();

	/* default CMAC and EIP-197 configuration */
	mv_pp3_cmac_config();

	/* run ppn */
	mv_pp3_ppc_run_all();

	/* init cpu's structures */
	for_each_possible_cpu(cpu) {
		mv_pp3_cpu_sw_init(pp3_cpus[cpu]);
		mv_pp3_cpu_hw_init(pp3_cpus[cpu]);
	}

	mv_pp3_messenger_init(priv);
	mv_pp3_drv_messenger_init(MV_PP3_CHAN_SIZE, false);

	/* get FW version */
	rc = pp3_fw_version_get(&fw_ver);
	if (rc) {
		pr_err("FW version is unknown. rc = %d\n", rc);
		return -1;
	}

	ver_name_size = sizeof(fw_ver.name);
	version_name = kzalloc(ver_name_size + 1, GFP_KERNEL);
	if (!version_name)
		return -ENOMEM;

	drv_ver = mv_pp3_get_driver_version();
	memcpy(version_name, drv_ver->name, ver_name_size);
	pr_info("\n");
	pr_info("Driver version: %s:%02d.%02d.%d",
		version_name, drv_ver->major_x, drv_ver->minor_y, drv_ver->local_z);
	if (drv_ver->debug_d)
		pr_cont(".%d\n", drv_ver->debug_d);

	memcpy(version_name, fw_ver.name, ver_name_size);
	pr_info("FW version:     %s:%02d.%02d.%d\n",
		version_name, fw_ver.major_x, fw_ver.minor_y, fw_ver.local_z);
	if (fw_ver.debug_d)
		pr_cont(".%d\n", fw_ver.debug_d);

	pr_info("\n");
	kfree(version_name);

	/* Send request for memory buffer size needed by FW */
	if (mv_pp3_fw_memory_alloc(pp3_device) < 0)
		return -1;

	/* initialized CPUs virtual ports */
	for_each_possible_cpu(cpu)
		if (pp3_fw_cpu_vport_set(MV_PP3_CPU_VPORT_ID(cpu)) < 0)
			return -1;

	priv->initialized = true;

	return 0;
}

static void mv_pp3_set_msi_msg(struct msi_desc *desc, struct msi_msg *msg)
{
	pr_info("PP3 MSI CB LO:%x HI:%x DATA:%x\n", msg->address_lo, msg->address_hi, msg->data);
}

static int mv_pp3_shared_probe(struct platform_device *pdev)
{
	int master, pool, cpu, i, ret;
	struct device_node *np = pdev->dev.of_node;
	struct device_node *child;
	struct msi_desc *msi_desc;
	struct resource *a2m[MV_PP3_A2M_MAX_MASTER], *amb, *nss_regs;
	struct net_device *dev;
	char name[20];

	coherency_hard_mode = coherency_available();

	pp3_device = kzalloc(sizeof(struct mv_pp3), GFP_KERNEL);
	if (!pp3_device) {
		pr_err("%s: out of memory\n", __func__);
		return -ENOMEM;
	}

	pp3_device->pdev = pdev;
	of_property_read_u32(np, "clock-frequency", &pp3_device->tclk_hz);
	pp3_device->ports_num = MV_PP3_EMAC_NUM;

	ret = platform_msi_domain_alloc_irqs(&pdev->dev, 32,
					     mv_pp3_set_msi_msg);
	if (ret) {
		pr_err("pp3: paltform domain alloc error:%s\n", np->full_name);
		return ret;
	}

	msi_desc = first_msi_entry(&pdev->dev);
	if (!msi_desc) {
		pr_err("no msi desc for pdev name:%s\n", np->full_name);
		/*platform_msi_domain_free_irqs(&pdev->dev);*/
		return -ENODEV;
	}

	pp3_device->irq_base = msi_desc->irq;

	pr_info("PP3 PROBE RX_ISR_BASE:%u full name:%s\n", pp3_device->irq_base, np->full_name);

	/*
	 * The mvebu-mbus DT binding currently doesn't allow
	 * describing static windows with the remap capability, so we
	 * simply use the mvebu-mbus API to dynamically create the
	 * required window. This should be changed once mvebu-mbus is
	 * extended to cover such a case.
	 */
	if (mv_pp3_gop_window_remap(pdev, &pp3_device->gop_regs) == -1)
		return -ENODEV;

	pp3_device->gop_regs.vaddr = devm_ioremap(&pdev->dev, pp3_device->gop_regs.paddr,
								pp3_device->gop_regs.size);
	if (!pp3_device->gop_regs.vaddr) {
		pr_err("Can not map MG registers, aborting\n");
		return -EBUSY;
	}

	for (master = 0; master < MV_PP3_A2M_MAX_MASTER; master++) {
		a2m[master] = platform_get_resource(pdev, IORESOURCE_MEM, master);
		if (!a2m[master]) {
			pr_err("Can not find A2M_%d registers base address, aborting\n", master);
			return -ENODEV;
		}
		pp3_device->a2m_regs[master].paddr = a2m[master]->start;
		pp3_device->a2m_regs[master].size = resource_size(a2m[master]);
		pp3_device->a2m_regs[master].vaddr = devm_ioremap(&pdev->dev, a2m[master]->start,
									resource_size(a2m[master]));
		if (!pp3_device->a2m_regs[master].vaddr) {
			pr_err("Can not map A2M_%d registers, aborting\n", master);
			return -EBUSY;
		}
	}
	/* store physical base address of CPU cluster */
	pp3_device->int_regs_paddr = pp3_device->a2m_regs[0].paddr & 0xFF000000;

	/* map AMB registers space */
	amb = platform_get_resource(pdev, IORESOURCE_MEM, 2);
	if (!amb) {
		pr_err("Can not find AMB registers base address, aborting\n");
		return -ENODEV;
	}
	pp3_device->amb_regs.paddr = amb->start;
	pp3_device->amb_regs.size = resource_size(amb);
	pp3_device->amb_regs.vaddr = devm_ioremap(&pdev->dev, amb->start, resource_size(amb));
	if (!pp3_device->amb_regs.vaddr) {
		pr_err("Cannot map PP3 AMB registers, aborting\n");
		return -EBUSY;
	}

	/* map NSS registers space */
	nss_regs = platform_get_resource(pdev, IORESOURCE_MEM, 3);
	if (!nss_regs) {
		pr_err("Can not find NSS registers base address, aborting\n");
		return -ENODEV;
	}
	pp3_device->nss_regs.paddr = nss_regs->start;
	pp3_device->nss_regs.size = resource_size(nss_regs);
	pp3_device->nss_regs.vaddr = devm_ioremap(&pdev->dev, nss_regs->start, resource_size(nss_regs));
	if (!pp3_device->nss_regs.vaddr) {
		pr_err("Can not map NSS registers, aborting\n");
		return -EBUSY;
	}
	pp3_device->short_pool_buf_size = CONFIG_MV_PP3_BM_SHORT_BUF_SIZE;
	mv_pp3_device_show(pp3_device);

	if (mv_pp3_a2m_win_init(pp3_device)) {
		pr_err("Cannot initialize PP3 A2M windows, aborting\n");
		return -EBUSY;
	}

	/* Basic initialization for all sub-modules and allocate all shared resources */
	mv_gop_init(&pp3_device->gop_regs, MV_PP3_GOP_MAC_NUM, INDIRECT_MG_ACCESS);

	/* TAI clock init (must be after gop) */
	mv_pp3_ptp_tclk_hz_set(pp3_device->tclk_hz);
	mv_pp3_tai_clock_init(pdev);

	mv_pp3_emac_global_init(MV_PP3_EMAC_NUM);
	for (i = 0; i < MV_PP3_EMAC_NUM; i++) {
		/* EMACs are enabled by HW default. Disable them first. */
		mv_pp3_emac_unit_base(i, pp3_device->nss_regs.vaddr + MV_PP3_EMAC_BASE(i));
		mv_pp3_emac_rx_enable(i, 0);
	}

	qm_init(pp3_device->nss_regs.vaddr);
	tm_global_init(pp3_device->nss_regs.vaddr, "A390");
	mv_pp3_hmac_init(pp3_device);
	mv_pp3_cmac_init(pp3_device->nss_regs.vaddr);
	mv_pp3_bm_init(pp3_device->nss_regs.vaddr);

	/* set number of active PPCS */
	mv_pp3_fw_ppc_num_set(CONFIG_MV_PP3_PPC_NUM);

	/* init FW and allocate DRAM memory for each active PPC */
	mv_pp3_fw_init(pp3_device);

	if (mv_pp3_vports_global_init(pp3_device, MV_PP3_COMMON_VPORTS_NUM))
		return -ENODEV;

	if (mv_pp3_cpus_global_init(pp3_device, nr_cpu_ids))
		return -ENODEV;

	if (mv_pp3_pools_global_init(pp3_device, BM_POOLS_NUM))
		return -ENODEV;

	/* QM pools allocation */
	pr_info("  o QM GPM pools memory allocation\n");
	for (pool = BM_QM_GPM_POOL_0; pool <= BM_QM_GPM_POOL_1; pool++) {
		struct pp3_pool *ppool;

		pr_cont("\t  o GPM pool %d: capacity = %d elements - ", pool, BM_QM_GPM_POOL_CAPACITY);
		ppool = mv_pp3_pool_alloc(BM_QM_GPM_POOL_CAPACITY);
		if (!ppool)
			return -ENOMEM;

		pr_cont("%d bytes of coherent memory allocated\n", ppool->capacity * sizeof(unsigned int));
		mv_pp3_pool_set_id(ppool, pool);
	}

	/* allocate CPU private and pools */
	for_each_possible_cpu(cpu) {
		pr_info("  o CPU %d memory allocation\n", cpu);
		if (mv_pp3_cpu_alloc(cpu) == NULL)
			return -ENOMEM;
	}

	mv_pp3_netdev_global_init(pp3_device);

	/* Create netdevice interfaces if needed */
	for_each_child_of_node(np, child) {
		if (!of_device_is_available(child))
			continue;

		if (!strcmp(child->name, "nic")) {
			of_property_read_u32(child, "id", &i);
			sprintf(name, "nic%d", i);
			dev = mv_pp3_netdev_init(name, CONFIG_MV_PP3_RXQ_NUM, CONFIG_MV_PP3_TXQ_NUM);
			if (!dev)
				return -ENODEV;

			mv_pp3_netdev_set_emac_params(dev, child);
			mv_pp3_netdev_show(dev);
		}
	}
	if (mv_pp3_gnss_dev_create(MV_NSS_EXT_PORT_MAX, false, NULL))
		return -ENODEV;

	if (pp3_sysfs_init(pp3_device) < 0)
		pr_err("NSS init - sysfs initialization failed\n");

	return 0;
}
/* Init mac data according to node mac data */
int mv_pp3_ftd_mac_data_get(struct device_node *np, struct mv_mac_data *mac_data)
{
	struct device_node *emac_node;
	struct device_node *phy_node;
	int phy_mode, err;
	const char *force_link;
	u32 speed;

	/* TODO add emac id to printouts */
	emac_node = of_parse_phandle(np, "emac-data", 0);
	if (!emac_node) {
		pr_info("%s: No EMAC data\n", __func__);
		return 0;
	}

	phy_mode = of_get_phy_mode(emac_node);

	switch (phy_mode) {
	case PHY_INTERFACE_MODE_SGMII:
		speed = 0;
		/* check phy speed */
		of_property_read_u32(emac_node, "phy-speed", &speed);
		switch (speed) {
		case 1000:
			mac_data->port_mode = MV_PORT_SGMII;
			break;
		case 2500:
			mac_data->port_mode = MV_PORT_SGMII2_5;
			break;
		default:
			mac_data->port_mode = MV_PORT_SGMII;
			break;
		}
		break;
	case PHY_INTERFACE_MODE_RXAUI:
		mac_data->port_mode = MV_PORT_RXAUI;
		break;
	case PHY_INTERFACE_MODE_QSGMII:
		mac_data->port_mode = MV_PORT_QSGMII;
		break;
	case PHY_INTERFACE_MODE_RGMII:
		mac_data->port_mode = MV_PORT_RGMII;
		break;
	default:
		pr_err("%s: incorrect phy-mode\n", __func__);
		return -1;
	}

	phy_node = of_parse_phandle(emac_node, "phy", 0);
	if (phy_node) {
		if (of_property_read_u32(phy_node, "reg", &mac_data->phy_addr))
			pr_err("%s: NO PHY address\n", __func__);
	}

	mac_data->link_irq = irq_of_parse_and_map(emac_node, 0);
	pr_info("PP3 LINK IRQ: %d for %s\n", mac_data->link_irq, emac_node->full_name);
	mac_data->force_link = false;
	err = of_property_read_string(emac_node, "force-link", &force_link);
	if (err >= 0) {
		if (!strcasecmp(force_link, "yes"))
			mac_data->force_link = true;
	}
	return 0;
}

int mv_pp3_fdt_mac_address_get(struct device_node *np, unsigned char *mac_addr)
{
	struct device_node *emac_node;
	const char *node_mac_addr = NULL;

	emac_node = of_parse_phandle(np, "emac-data", 0);
	if (!emac_node) {
		/* TODO add emac id to printouts */
		pr_info("%s: No EMAC data\n", __func__);
		return 0;
	}

	node_mac_addr = of_get_mac_address(emac_node);
	if (node_mac_addr != NULL)
		memcpy(mac_addr, node_mac_addr, MV_MAC_ADDR_SIZE);

	return 0;
}

static int mv_pp3_shared_remove(struct platform_device *pdev)
{
	/* free all shared resources */
	pr_err("%s:: called", __func__);
	return 0;
}

static const struct of_device_id mv_pp3_shared_match[] = {
	{ .compatible = "marvell,armada-390-pp3" },
	{}
};
MODULE_DEVICE_TABLE(of, mv_pp3_shared_match);

static struct platform_driver mv_pp3_shared_driver = {
	.probe		= mv_pp3_shared_probe,
	.remove		= mv_pp3_shared_remove,
	.driver = {
		.name	= MV_PP3_SHARED_NAME,
		.owner	= THIS_MODULE,
		.of_match_table = mv_pp3_shared_match,
	},
};

static int __init mv_pp3_init_module(void)
{
	int rc = 0;

	rc = platform_driver_register(&mv_pp3_shared_driver);
	if (rc) {
		pr_err("%s: Can't register %s driver. rc=%d\n",
			__func__, mv_pp3_shared_driver.driver.name, rc);
		return rc;
	}
	return rc;
}
module_init(mv_pp3_init_module);
/*---------------------------------------------------------------------------*/

static void __exit mv_pp3_cleanup_module(void)
{
	platform_driver_unregister(&mv_pp3_shared_driver);
}
module_exit(mv_pp3_cleanup_module);
/*-------------------------------------------------*/

MODULE_DESCRIPTION("Marvell PPv3 Network Driver - www.marvell.com");
MODULE_AUTHOR("Dmitri Epshtein <dima@marvell.com>");
MODULE_LICENSE("GPL");

