/* Copyright (c) 2014-2015, The Linux Foundation. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 and
 * only version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#ifndef _CORESIGHT_CORESIGHT_ETM_H
#define _CORESIGHT_CORESIGHT_ETM_H

#include "coresight-priv.h"

/*
 * Device registers:
 * 0x000 - 0x100: Global	registers
 * 0x100 - 0xD00: Channel	registers
 * 0xD00 - 0xFD0: Profiling	registers
 * 0xFDO - 0xFFC: ID		registers
 */

/* Global registers (0x000-0x2FC) */
/* Main control and configuration registers */
#define AXI_MON_CTL			0x00
#define AXI_MON_DYN_CTL			0x04
#define AXI_MON_STAT			0x08
#define AXI_MON_EV_CLR			0x10
#define AXI_MON_EV_SW_TRIG		0x20
#define AXI_MON_VER			0x30

/* Channel registers */
#define AXI_MON_CH_BASE(x)		(0x100 + 0x100 * x)
#define AXI_MON_CH_CTL(x)		(AXI_MON_CH_BASE(x) + 0x00)
#define AXI_MON_CH_REF_ADDR_L(x)	(AXI_MON_CH_BASE(x) + 0x10)
#define AXI_MON_CH_REF_ADDR_H(x)	(AXI_MON_CH_BASE(x) + 0x14)
#define AXI_MON_CH_USE_ADDR_L(x)	(AXI_MON_CH_BASE(x) + 0x18)
#define AXI_MON_CH_USE_ADDR_H(x)	(AXI_MON_CH_BASE(x) + 0x1C)
#define AXI_MON_CH_REF_ID(x)		(AXI_MON_CH_BASE(x) + 0x20)
#define AXI_MON_CH_USE_ID(x)		(AXI_MON_CH_BASE(x) + 0x24)
#define AXI_MON_CH_REF_USER(x)		(AXI_MON_CH_BASE(x) + 0x28)
#define AXI_MON_CH_USE_USER(x)		(AXI_MON_CH_BASE(x) + 0x2C)
#define AXI_MON_CH_REF_ATTR(x)		(AXI_MON_CH_BASE(x) + 0x30)
#define AXI_MON_CH_USE_ATTR(x)		(AXI_MON_CH_BASE(x) + 0x34)
#define AXI_MON_CH_REF_AUX_ATTR(x)	(AXI_MON_CH_BASE(x) + 0x38)
#define AXI_MON_CH_USE_AUX_ATTR(x)	(AXI_MON_CH_BASE(x) + 0x3C)
#define AXI_MON_CH_COMP_CTL(x)		(AXI_MON_CH_BASE(x) + 0x50)
#define AXI_MON_CH_COMP_MAX(x)		(AXI_MON_CH_BASE(x) + 0x60)
#define AXI_MON_CH_COMP_MIN(x)		(AXI_MON_CH_BASE(x) + 0x68)
#define AXI_MON_CH_COUNT(x)		(AXI_MON_CH_BASE(x) + 0x90)
#define AXI_MON_CH_RLD(x)		(AXI_MON_CH_BASE(x) + 0x98)

/* Profiling registers */
#define AXI_MON_PR_CTL			(0xD00)
#define AXI_MON_SAMP_CNT		(0xD20)
#define AXI_MON_SAMP_CNT_RLD		(0xD24)
#define AXI_MON_PR_SMP_CYC		(0xD50)
#define AXI_MON_PR_SMP_TRANS		(0xD54)
#define AXI_MON_PR_SMP_BEATS		(0xD58)
#define AXI_MON_PR_SMP_BYTES		(0xD5C)
#define AXI_MON_PR_SMP_LATEN		(0xD60)
#define AXI_MON_PR_SMP_MAX		(0xD64)
#define AXI_MON_PR_SMP_MIN		(0xD68)

/* Comperator configuration */
#define AXI_MON_COMP_ENABLE		(1 << 31)
#define AXI_MON_COMP_ADDR		(0 << 24)
#define AXI_MON_COMP_WIDTH_32		(31 << 8)

/* Channel control */
#define AXI_MON_CHAN_ENABLE		(1 << 31)
#define AXI_MON_CHAN_IRQ_ENABLE		(1 << 9)
#define AXI_MON_CHAN_TRIG_ENABLE	(1 << 8)

/* Global control */
#define AXI_MON_ENABLE			(1 << 31)
#define AXI_MON_IRQ_ENABLE		(1 << 14)
	/* rev 2 only */
#define AXI_MON_DYN_DEACT		(1 << 13)
#define AXI_MON_DYN_ACT			(1 << 12)
	/* rev 1 only */
#define AXI_MON_EVENT_ENABLE		(1 << 12)

/* Dynamic control */
#define DYN_CTL_FREEZE_OFF		(0)

/* Attribute register */
#define AXI_CHAN_ATTR(dom, cache, qos, prot)	(dom << 24 | cache << 16 | qos << 8 | prot)

/* Axi Mon Event Clear register */
#define AXI_MON_EV_PE			(1 << 17)
#define AXI_MON_EV_SMPR			(1 << 24)

/* AXI Mon Stat register */
#define AXI_MON_STAT_SIP_OFF		(23)
#define AXI_MON_STAT_SIP_MASK		(0x3 << AXI_MON_STAT_SIP_OFF)

/* SW Trigger register */
#define AXI_EV_SW_TRIG_SAMPLE_EN	(1 << 16)

/* Profiling Control */
#define AXI_MON_PROF_EN_OFF		(31)
#define AXI_MON_PROF_CYCG_OFF		(8)
#define AXI_MON_PROF_CYCG_MASK		(7)
#define AXI_MON_PROF_CYCG_4_CYC		(1)

/* Supported versions */
#define AXI_MON_VER_MASK		(0x0F)
#define AXI_MON_REV_1			(0x00)
#define AXI_MON_REV_2			(0x01)

#define AXI_MON_MAX_CHANNELS		(12)
#define AXI_MON_MAX_BUS_WIDTH		(48)


/* Address comparator access types */
enum axim_comp_type {
	AXIM_COMP_ADDR,
	AXIM_COMP_USER,
};
/* Event trigger modes */
enum axim_event_mode {
	AXIM_EVENT_MODE_MATCH = 0,
	AXIM_EVENT_MODE_OVERFLOW = 1,
};

/**
 * struct axim_chan_data - holds the axi monitor channel configuration
 *
 * @addr_start:		Physical start address to match
 * @addr_end:		Physical end address to match
 * @id(_mask):		AXI AxID field to match
 * @user(_mask):	AXI AxUSER field to match
 * @domain(_mask):	AXI AxDOMAIN field to match
 * @cache(_mask):	AXI AxCACHE field to match
 * @qos(_mask):		AXI AxQOS field to match
 * @prot(_mask):	AXI AxPROT field to match
 * @event_mode:		Selects when to generate an event
 * @event_thresh:	Counter threshold to generate event
 * @enable:		Is this AXIM channel currently enable.
 */
struct axim_chan_data {
	u64	addr_start;
	u64	addr_end;
	u32	id;
	u32	id_mask;
	u32	user;
	u32	user_mask;
	u8	domain;
	u8	domain_mask;
	u8	cache;
	u8	cache_mask;
	u8	qos;
	u8	qos_mask;
	u8	prot;
	u8	prot_mask;
	enum axim_event_mode event_mode;
	u32	event_thresh;
	bool	enable;
};

/**
 * struct axim_drvdata - specifics associated to an AXIM component
 * @base:       Memory mapped base address for this component.
 * @dev:        The device entity associated to this component.
 * @csdev:      Component vitals needed by the framework.
 * @enable:	Is this AXIM currently enable.
 * @boot_enable:True if we should start tracing at boot time.
 * @latency_en: Indicate latency measurment support.
 * @trace_en:	Indicate trace support.
 * @prof_en:	Indicate profiling support.
 * @nr_chan:	Number of comparator channels.
 * @curr_chan:	Channel Number for configuration.
 * @nr_prof_reg:Number of profiling registers.
 * @major:	Major HW version.
 * @minor:	Minor HW version.
 * @prof_cyc_mul: Cycle multiplier for profiling clock events.
 * @clock_freq_mhz: Profiler clock frequency in MHz.
 * @channels:	Channel descriptor.
 */
struct axim_drvdata {
	void __iomem			*base;
	struct device			*dev;
	struct coresight_device		*csdev;
	struct clk			*clk;
	bool				enable;
	bool				boot_enable;
	bool				latency_en;
	bool				trace_en;
	bool				prof_en;
	u8				nr_chan;
	u8				curr_chan;
	u8				nr_prof_reg;
	u8				major;
	u8				minor;
	u8				prof_cyc_mul;
	u32				bus_width;
	u32				clock_freq_mhz;
	struct axim_chan_data		channel[AXI_MON_MAX_CHANNELS];
};

#endif
