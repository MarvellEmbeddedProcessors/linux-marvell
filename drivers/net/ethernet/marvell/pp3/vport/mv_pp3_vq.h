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
#ifndef __mv_pp3_vq_h__
#define __mv_pp3_vq_h__


#include "common/mv_sw_if.h"
#include "platform/mv_pp3.h"
#include "mv_pp3_vport.h"
#include "mv_pp3_swq.h"


struct pp3_vport;

struct pp3_vq {
	int vq;                       /* virtual queue number */
	bool valid;                   /* true, if queue valid for napi process */
	enum mv_pp3_queue_type type;  /* virtual queue type */
	struct pp3_swq *swq;          /* pointer to RX SWQ structure - NULL for EMAC vports */
	int hwq;                      /* HWQ number for this RX VQ */
	int bpi_group;                /* internal back pressure group */
	int dp_id;                    /* drop profile identifier */
	struct mv_nss_sched *sched;  /* scheduling parameters for virtual queue */
	struct mv_nss_drop  *drop;   /* drop parameters for virtual queue */
	struct mv_nss_meter *meter;  /* shaper for TX or policer for RX parameters for virtual queue */
};


/* "alloc" functions do the following actions:
 * - Allocate zeroed memory for struct pp3_rx_vq/struct pp3_tx_vq  structure
 * - Set must fields in the structures: vq, vq_type
 * - Call rx_swq/tx_swq allocation function
 * - Save pointer to rx_swq/tx_swq structure to relevant field (rx_swq/tx_swq)
 */
struct pp3_vq *mv_pp3_vq_alloc(int vq, enum mv_pp3_queue_type type);

/* "delete" functions do the following actions:
 * - Free memory allocated for for pp3_vq/pp3_tx_vq structures
 */
void mv_pp3_vq_delete(struct pp3_vq *vq);

/* return pointer to RX SWQ stats */
struct pp3_swq_stats *mv_pp3_ingress_vq_sw_stats(struct pp3_vport *vport, int vq);

/* get reset flag indication */
int mv_pp3_ingress_vq_reset_stats_get(struct pp3_vport *vport, int vq, int *reset);

/* set reset flag */
int  mv_pp3_ingress_vq_reset_stats_set(struct pp3_vport *vport, int vq, int reset);

/* "sw_init" functions do the following actions:
 * - got relevant parameters from configurator/pp3_vport/defines
 * - set all fields in pp3_vq and pp3_tx_vq structures
 * - call "sw_init" function for relevant SWQ
 */
int mv_pp3_ingress_cpu_vq_sw_init(int vport_num, struct pp3_vq *vq, int irq_group);
int mv_pp3_ingress_emac_vq_sw_init(struct pp3_vq *vq, int emac);
int mv_pp3_egress_cpu_vq_sw_init(int vport_num, struct pp3_vq *vq, int cpu);
int mv_pp3_egress_emac_vq_sw_init(struct pp3_vq *vq, int emac);

/* "hw_init" functions do the following actions:
 * - initialize HW accordingly with information in the pp3_vq/pp3_tx_vq structures
 * - call "hw_init" function to relevant SWQ
 */
int mv_pp3_vq_hw_init(struct pp3_vq *vq);

void mv_pp3_vqueue_cnt_dump_header(int vq_num);
void mv_pp3_vqueue_cnt_dump(const char *cntr_pref, int cpu, struct pp3_vq **vq, int vq_num, bool pr_cntr_name);

/* run-time virtual queue set/get functions */
/* Some functions are supported only for special vport types: CPU or EMAC. */
/* Ingress VQs: ppc_to_hmac for CPU internal port, emac_to_ppc for EMAC vport */
int mv_pp3_ingress_vq_drop_set(struct pp3_vport *vport, int vq, struct mv_nss_drop *drop);
int mv_pp3_ingress_vq_drop_get(struct pp3_vport *vport, int vq, struct mv_nss_drop *drop);

int mv_pp3_ingress_vq_prio_set(struct pp3_vport *vport, int vq, u16 prio);
int mv_pp3_ingress_vq_weight_set(struct pp3_vport *vport, int vq, int mtu, u16 weight);
int mv_pp3_ingress_vq_sched_get(struct pp3_vport *vport, int vq, struct mv_nss_sched *sched);

int mv_pp3_ingress_cos_to_vq_set(struct pp3_vport *vport, int cos, int vq);
int mv_pp3_ingress_cos_to_vq_get(struct pp3_vport *vport, int cos, int *vq);

int mv_pp3_ingress_vq_size_set(struct pp3_vport *vport, int vq, u16 length);
int mv_pp3_ingress_vq_size_get(struct pp3_vport *vport, int vq, u16 *length);

/* Egress VQs: hmac_to_ppc for CPU internal port, ppc_to_emac for EMAC vport */
int mv_pp3_egress_vq_drop_set(struct pp3_vport *vport, int vq, struct mv_nss_drop *drop);
int mv_pp3_egress_vq_drop_get(struct pp3_vport *vport, int vq, struct mv_nss_drop *drop);

int mv_pp3_egress_vq_prio_set(struct pp3_vport *vport, int vq, u16 prio);
int mv_pp3_egress_vq_weight_set(struct pp3_vport *vport, int vq, int mtu, u16 weight);
int mv_pp3_egress_vq_sched_get(struct pp3_vport *vport, int vq, struct mv_nss_sched *sched);

int mv_pp3_egress_cos_to_vq_set(struct pp3_vport *vport, int cos, int vq);

/* Used in data path */
static inline int mv_pp3_egress_cos_to_vq_get(struct pp3_vport *vport, int cos)
{
	return vport->tx_cos_to_vq[cos];
}

int mv_pp3_egress_vq_size_set(struct pp3_vport *vport, int vq, u16 length);
int mv_pp3_egress_vq_size_get(struct pp3_vport *vport, int vq, u16 *length);

int mv_pp3_egress_vq_rate_limit_set(struct pp3_vport *vport, int vq, struct mv_nss_meter *shaper);
int mv_pp3_egress_vq_rate_limit_get(struct pp3_vport *vport, int vq, struct mv_nss_meter *shaper);

#endif /* __mv_pp3_vq_h__ */
