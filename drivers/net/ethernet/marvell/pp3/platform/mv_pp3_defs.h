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


#ifndef __mv_pp3_defs_h__
#define __mv_pp3_defs_h__

/*---------------------------------------------------------------------------*/
/*			global HW configuration				     */
/*---------------------------------------------------------------------------*/
#define MV_PP3_CPU_NUM			(2)   /* Must be equal or larger than CONFIG_NR_CPUS */
#define MV_PP3_EMAC_NUM			(4)   /* Number of EMACs in board */
#define MV_PP3_GOP_MAC_NUM		(4)   /* Max number of MACs in project GOP */

#define MV_PP3_EMAC_VP_NUM		MV_PP3_EMAC_NUM
#define MV_PP3_CPU_VP_NUM		((MV_PP3_EMAC_VP_NUM + 1) * MV_PP3_CPU_NUM)

#define MV_PP3_HFRM_TIME_COAL_PROF_NUM	(2)   /* Number of HMAC frame coalescing */
#define MV_PP3_HFRM_NUM			(4)   /* Number of HMAC frame in packets processor */
#define MV_PP3_HFRM_Q_NUM		(16)  /* Number of queues couples in each HMAC frame */

#define MV_PP3_PPC_MAX_NUM		(2)	/* max number of PPCs in system */

#define MV_PP3_QM_BP_RULES_NUM		(64)	/* Number of QM internal back pressure groups */

#define MV_PP3_EMAC_BASE(_emac_)	(0x000CA000 + (0x1000 * (_emac_)))

/*---------------------------------------------------------------------------*/
/*			global SW configuration				     */
/*---------------------------------------------------------------------------*/
#define MV_PP3_INTERNAL_CPU_PORT_MIN    (0)
#define MV_PP3_INTERNAL_CPU_PORT_NUM	(32)
#define MV_PP3_INTERNAL_CPU_PORT_MAX	(MV_PP3_INTERNAL_CPU_PORT_MIN + MV_PP3_INTERNAL_CPU_PORT_NUM - 1)

/* translate cpu number to cpu virtual port id */
#define MV_PP3_CPU_VPORT_ID(_cpu_id_)	(MV_NSS_CPU_PORT_MIN + (_cpu_id_))
/* translate cpu virtual port it to CPU number */
#define MV_PP3_CPU_VPORT_TO_CPU(cpu_vp)	((cpu_vp) - MV_NSS_CPU_PORT_MIN)


#define MV_PP3_DEV_NUM			(MV_NSS_EXT_PORT_NUM + MV_PP3_EMAC_NUM) /* max number of network devices */

#define MV_PP3_BM_POOL_HROOM_RES	(32)

#define MV_PP3_HMAC_BM_Q_SIZE		(1024) /* size of rxq/txq that used for BM pools access */
#define MV_PP3_CHAN_SIZE		(512)  /* new channel size */

#define MV_PP3_DEBUG_BUFFER		1024
#define MV_PP3_RSS_MAX_HASH		(0xFFFFFFFF)

#define MV_PP3_VQ_NUM			16
#define MV_PP3_PRIO_NUM			16
#define MV_PP3_SCHED_PRIO_NUM		8

#define MV_PP3_QM_128B_UNITS		128
#define MV_PP3_QM_16B_UNITS		16

#define MV_PP3_QM_UNITS			MV_PP3_QM_128B_UNITS

#define MV_PP3_TXDONE_HRTIMER_PERIOD	(1000)	/* default value for digh resolution timer in usec */

/* Default Drop thresholds per HWQ: */
/* TD  = 20 packets 2K bytes each = 40 KBytes */
/* RED = 10 packets 2K bytes each = 20 KBytes */
#define MV_PP3_INGRESS_TD_DEF           (2 * 20) /* 40 KBytes */
#define MV_PP3_INGRESS_RED_DEF          (2 * 10) /* 20 KBytes */

/*---------------------------------------------------------------------------*/
/*			global SW thresholds configuration		     */
/*---------------------------------------------------------------------------*/
/* thresholds in KB */
#define MV_PP3_10G_LOW_THR		200
#define MV_PP3_10G_PAUSE_THR		250
#define MV_PP3_10G_HIGH_THR		300

#define MV_PP3_2_5G_LOW_THR		90
#define MV_PP3_2_5G_PAUSE_THR		100
#define MV_PP3_2_5G_HIGH_THR		120

#define MV_PP3_1G_LOW_THR		30
#define MV_PP3_1G_PAUSE_THR		35
#define MV_PP3_1G_HIGH_THR		40

#define MV_PP3_HMAC_LOW_THR		1
#define MV_PP3_HMAC_HIGH_THR		3
#define MV_PP3_HMAC_PAUSE_THR		(MV_PP3_HMAC_HIGH_THR * 2) /*not in use*/

/* QM internal back pressure thresholds in KB */
#define MV_PP3_QM_BPI_XON		1
#define MV_PP3_QM_BPI_XOFF		2

/* Number of buffers in the pool calculated as */
/* number of CPUs * (number of RXQs per CPU * number of packets in each RXQ  + EXTRA */
#define MV_PP3_RX_BUFS_EXTRA		2000

/* PP3 queue type */
enum mv_pp3_queue_type {
	MV_PP3_EMAC_TO_PPC = 0,	/* relevant for emac_vport */
	MV_PP3_PPC_TO_EMAC,	/* relevant for emac_vport */
	MV_PP3_HMAC_TO_PPC,	/* relevant for cpu_vport */
	MV_PP3_PPC_TO_HMAC,	/* relevant for cpu_vport */
};

#endif /* __mv_pp3_defs_h__ */
