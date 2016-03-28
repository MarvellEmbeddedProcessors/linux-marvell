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

#ifndef __mv_pp3_h__
#define __mv_pp3_h__

#include <linux/kernel.h>
#include <linux/types.h>
#include <linux/module.h>
#include <linux/io.h>
#include <linux/of.h>
#include <linux/of_irq.h>
#include <linux/platform_device.h>
#include <linux/netdevice.h>
#include <linux/workqueue.h>

#ifdef CONFIG_MV_GNSS_SUPPORT
#include <net/gnss/mv_nss_defs.h>
#else
#include <net/mvebu/mv_nss.h>
#endif

#include "common/mv_sw_if.h"
#include "common/mv_hw_if.h"
#include "mv_a2m.h"
#include "mv_pp3_defs.h"
#include "mv_pp3_cfh.h"


struct mv_pp3 {
	struct platform_device	*pdev;
	struct  platform_device	*sysfs_pdev;
	struct mv_io_addr	a2m_regs[MV_PP3_A2M_MAX_MASTER];
	struct mv_io_addr	amb_regs;
	struct mv_io_addr	nss_regs;
	struct mv_io_addr	gop_regs;
	phys_addr_t		int_regs_paddr;
	u32			tclk_hz;
	int			short_pool_buf_size;
	unsigned int		ports_num;
	unsigned int		irq_base;
	bool			initialized;
};

extern struct mv_pp3 *pp3_device;

enum mv_pp3_timer_type {
	MV_PP3_HRES_TIMER,
	MV_PP3_NORMAL_TIMER
};

enum mv_pp3_timer_internal_type {
	MV_PP3_TASKLET,
	MV_PP3_WORKQUEUE
};

struct pp3_timer_stats {
	unsigned int timer_add;
	unsigned int timer_sched;
	unsigned int user_cnt1; /* generic counter, used as time usec_elapsed */
	unsigned int user_cnt2;	/* generic counter, count timer iterations */

};

struct pp3_timer_work {
	struct work_struct work;
	unsigned long cookie;
	void (*cb_func)(unsigned long param);
};

#define MV_PP3_TIMER_INIT_BIT		0
#define MV_PP3_TIMER_SCHED_BIT		1

struct mv_pp3_timer {
	unsigned int		usec;          /* timer period int micro seconds */
	unsigned int		cpu;	       /* cpu to run timer on */
	enum mv_pp3_timer_type	type;
	struct tasklet_struct	tasklet;
	struct workqueue_struct *wq;
	struct pp3_timer_work	*timer_work;
#ifdef CONFIG_HIGH_RES_TIMERS
	struct hrtimer		hr_timer;      /* high resolutin timer */
#endif
	struct timer_list	normal_timer;  /* normal timer */
	unsigned long		flags;
	struct pp3_timer_stats  stats;
};
/*---------------------------------------------------------------------------*/
static inline void mv_pp3_timer_complete(struct mv_pp3_timer *pp3_timer)
{
	clear_bit(MV_PP3_TIMER_SCHED_BIT, &(pp3_timer->flags));
}

/*---------------------------------------------------------------------------*/
static inline bool mv_pp3_timer_is_initialized(struct mv_pp3_timer *pp3_timer)
{
	return test_bit(MV_PP3_TIMER_INIT_BIT, &(pp3_timer->flags)) ? true : false;
}
/*---------------------------------------------------------------------------*/
static inline bool mv_pp3_timer_is_running(struct mv_pp3_timer *pp3_timer)
{
	return test_bit(MV_PP3_TIMER_SCHED_BIT, &(pp3_timer->flags)) ? true : false;
}
/*---------------------------------------------------------------------------*/
/* add time period to timer on the current CPU */
static inline void mv_pp3_timer_add(struct mv_pp3_timer *pp3_timer)
{
	if (test_and_set_bit(MV_PP3_TIMER_SCHED_BIT, &(pp3_timer->flags)))
		return;

	STAT_INFO(pp3_timer->stats.timer_add++);

#ifdef CONFIG_HIGH_RES_TIMERS
	if (pp3_timer->type == MV_PP3_HRES_TIMER) {
		ktime_t interval = ktime_set(0, pp3_timer->usec * 1000); /* 0 seconds, delay_in_ns nanoseconds */
		hrtimer_start(&pp3_timer->hr_timer, interval, HRTIMER_MODE_REL_PINNED);
		return;
	}
#endif /* CONFIG_HIGH_RES_TIMERS */
	if (pp3_timer->usec) {
		pp3_timer->normal_timer.expires = jiffies + usecs_to_jiffies(pp3_timer->usec);
		add_timer_on(&pp3_timer->normal_timer, pp3_timer->cpu);
	}
}


/* Timer initialization */
int mv_pp3_timer_init(struct mv_pp3_timer *pp3_timer, unsigned int cpu, unsigned int usec,
			enum mv_pp3_timer_internal_type type, void (*func)(unsigned long), unsigned long cookie);
/* Timer kill */
int mv_pp3_timer_kill(struct mv_pp3_timer *pp3_timer);

 /* Add time period to timer on the current CPU */
void mv_pp3_timer_add(struct mv_pp3_timer *pp3_timer);

/* Set timer time period, input in micro seconds */
int mv_pp3_timer_usec_set(struct mv_pp3_timer *pp3_timer, unsigned long usec);

#if 0
/* Get current timer type, high resolution or normal */
enum mv_pp3_timer_type mv_pp3_timer_type_get(struct mv_pp3_timer *pp3_timer);

#endif
/* Return value of TCLOCK (core) in Hz */
static inline u32 mv_pp3_silicon_tclk_get(void)
{
	return pp3_device->tclk_hz;
}

/* Return virtual address for NSS access */
static inline void *mv_pp3_nss_regs_vaddr_get(void)
{
	return pp3_device->nss_regs.vaddr;
}

/* Return physical address for internal registers access */
static inline phys_addr_t mv_pp3_internal_regs_paddr_get(struct mv_pp3 *priv)
{
	return priv->int_regs_paddr;
}

static inline struct platform_device *mv_pp3_platform_dev_get(struct mv_pp3 *priv)
{
	return priv->pdev;
}

static inline struct device *mv_pp3_dev_get(struct mv_pp3 *priv)
{
	struct platform_device *pdev = mv_pp3_platform_dev_get(priv);

	return &pdev->dev;
}

static inline int mv_pp3_ports_num_get(struct mv_pp3 *priv)
{
	return priv->ports_num;
}

static inline bool mv_pp3_shared_initialized(struct mv_pp3 *priv)
{
	return priv->initialized;
}

static inline int mv_pp3_irq_base_get(struct mv_pp3 *priv)
{
	return priv->irq_base;
}

/* Function prorotypes */
int mv_pp3_fdt_mac_address_get(struct device_node *np, unsigned char *mac_addr);
int mv_pp3_ftd_mac_data_get(struct device_node *np, struct mv_mac_data *mac_data);
struct mv_pp3_version *mv_pp3_get_driver_version(void);
int mv_pp3_shared_start(struct mv_pp3 *priv);
int mv_pp3_init_sysfs_init(struct kobject *pp3_kobj);
int mv_pp3_init_sysfs_exit(struct kobject *pp3_kobj);
int mv_pp3_debug_sysfs_init(struct kobject *pp3_kobj);
int mv_pp3_debug_sysfs_exit(struct kobject *pp3_kobj);
int mv_pp3_nss_drain(struct mv_pp3 *priv);
int mv_pp3_dp_q_find(u16 td, u16 red);
void mv_pp3_dp_q_free(int dp_id);


#endif /* __mv_pp3_h__ */
