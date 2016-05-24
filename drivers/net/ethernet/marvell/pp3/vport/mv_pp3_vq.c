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
#include "common/mv_sw_if.h"
#include "platform/mv_pp3.h"
#include "platform/mv_pp3_config.h"
#include "hmac/mv_hmac.h"

#include "tm/wrappers/mv_tm_sched.h"
#include "tm/wrappers/mv_tm_drop.h"
#include "tm/wrappers/mv_tm_shaping.h"
#include "tm/wrappers/mv_tm_scheme.h"
#include "fw/mv_pp3_fw_msg.h"
#include "mv_pp3_vport.h"
#include "mv_pp3_vq.h"
#include "mv_pp3_swq.h"

/*---------------------------------------------------------------------------*/
/* VQs allocation functions						     */
/*---------------------------------------------------------------------------*/
struct pp3_vq *mv_pp3_vq_alloc(int vq, enum mv_pp3_queue_type type)
{
	struct pp3_vq *vq_priv = kzalloc(sizeof(struct pp3_vq), GFP_KERNEL);
	MV_PP3_NULL_PTR(vq_priv, oom);

	vq_priv->vq = vq;

	if ((type == MV_PP3_PPC_TO_HMAC) ||  (type == MV_PP3_HMAC_TO_PPC)) {
		vq_priv->swq = mv_pp3_swq_alloc();
		MV_PP3_NULL_PTR(vq_priv->swq, oom);
	}
	vq_priv->type = type;

	vq_priv->sched = kzalloc(sizeof(struct mv_nss_sched), GFP_KERNEL);
	MV_PP3_NULL_PTR(vq_priv->sched, oom);

	vq_priv->drop = kzalloc(sizeof(struct mv_nss_drop), GFP_KERNEL);
	MV_PP3_NULL_PTR(vq_priv->drop, oom);

	vq_priv->meter = kzalloc(sizeof(struct mv_nss_meter), GFP_KERNEL);
	MV_PP3_NULL_PTR(vq_priv->meter, oom);

	return vq_priv;
oom:
	mv_pp3_vq_delete(vq_priv);
	pr_err("%s: Out of memory\n", __func__);
	return NULL;
}
/*---------------------------------------------------------------------------*/

void mv_pp3_vq_delete(struct pp3_vq *vq)
{
	if (!vq)
		return;

	mv_pp3_swq_delete(vq->swq);
	kfree(vq);
}
/*---------------------------------------------------------------------------*/

int mv_pp3_ingress_cpu_vq_sw_init(int vport_num, struct pp3_vq *vq, int irq_group)
{
	int frame, swq, hwq_base, hwq_num;
	MV_PP3_NULL_PTR(vq, err);

	if (mv_pp3_cfg_dp_rxq_params_get(vport_num, &frame, &swq, &hwq_base, &hwq_num) < 0)
		goto err;

	if (mv_pp3_rx_swq_sw_init(vq->swq, frame, swq, hwq_base, irq_group) < 0)
		goto err;

	vq->hwq = hwq_base;
	vq->valid = true;

	return 0;
err:
	pr_err("%s: function failed\n", __func__);
	return -1;
}
/*---------------------------------------------------------------------------*/

int mv_pp3_ingress_emac_vq_sw_init(struct pp3_vq *vq, int emac)
{
	MV_PP3_NULL_PTR(vq, err);

	/* get EMAC dequeue port number and enqueue queue number */
	mv_pp3_cfg_emac_qm_params_get(emac, NULL, &vq->hwq);
	return 0;
err:
	pr_err("%s: function failed\n", __func__);
	return -1;
}
/*---------------------------------------------------------------------------*/

int mv_pp3_vq_hw_init(struct pp3_vq *vq)
{
	MV_PP3_NULL_PTR(vq, err);

	switch (vq->type) {
	case MV_PP3_EMAC_TO_PPC:
	case MV_PP3_PPC_TO_EMAC:
		/* Nothing to do for EMAC virtual queues */
		break;
	case MV_PP3_HMAC_TO_PPC:
		if (mv_pp3_tx_swq_hw_init(vq->swq) < 0)
			goto err;
		break;
	case MV_PP3_PPC_TO_HMAC:
		if (mv_pp3_rx_swq_hw_init(vq->swq) < 0)
			goto err;
		break;
	default:
		pr_err("%s: Unexpected vq %d type %d\n", __func__, vq->vq, vq->type);
		goto err;

	}
	return 0;
err:
	pr_err("%s: function failed\n", __func__);
	return -1;
}

/*---------------------------------------------------------------------------*/
int mv_pp3_egress_cpu_vq_sw_init(int vp, struct pp3_vq *vq, int cpu)
{
	int frame, swq, hwq;

	MV_PP3_NULL_PTR(vq, err);

	if (mv_pp3_cfg_dp_txq_params_get(vp, cpu, &frame, &swq, &hwq) < 0)
		goto err;

	if (mv_pp3_tx_swq_sw_init(vq->swq, frame, swq, hwq))
		goto err;

	vq->hwq = hwq;
	return 0;
err:
	pr_err("%s: function failed\n", __func__);
	return -1;
}
/*---------------------------------------------------------------------------*/

int mv_pp3_egress_emac_vq_sw_init(struct pp3_vq *vq, int emac)
{

	MV_PP3_NULL_PTR(vq, err);

	if (mv_pp3_cfg_dp_emac_params_get(emac, &vq->hwq) < 0)
		goto err;

	return 0;
err:
	pr_err("%s: function failed\n", __func__);
	return -1;
}
/*---------------------------------------------------------------------------*/

/* Ingress VQs run-time functions: ppc_to_hmac for CPU vport, emac_to_ppc for EMAC vport */
int mv_pp3_ingress_vq_drop_set(struct pp3_vport *vport, int vq, struct mv_nss_drop *drop)
{
	int dp_id, rc = 0;
	struct pp3_vq *vq_priv;

	if (mv_pp3_max_check(vq, vport->rx_vqs_num, "rx_vq"))
		return -1;

	vq_priv = vport->rx_vqs[vq];
	if (!vq_priv) {
		pr_err("%s: Error - ingress VQ #%d is not initialized\n", __func__, vq);
		return -1;
	}
	/* Find exist or create new drop profile with "td" and "red" values */
	dp_id = mv_pp3_dp_q_find(drop->td, drop->red);
	if (dp_id <= 0) {
		rc = -1;
		goto err;
	}

	/* Free old profile ID */
	mv_pp3_dp_q_free(vq_priv->dp_id);

	/* Configure drop_profile to HWQ */
	rc = mv_tm_dp_set(TM_Q_LEVEL, vq_priv->hwq, -1, dp_id);
	if (rc) {
		pr_err("Can't attach HWQ #%d to drop profile %d. rc=%d\n", vq_priv->hwq, dp_id, rc);
		goto err;
	}

	vq_priv->dp_id = dp_id;
	*vq_priv->drop = *drop;

err:
	if (rc)
		pr_err("%s: function failed. rc = %d\n", __func__, rc);

	return rc;
}
/*---------------------------------------------------------------------------*/
int mv_pp3_ingress_vq_reset_stats_get(struct pp3_vport *vport, int vq, int *reset)
{
	MV_PP3_NULL_PTR(vport, err);

	if (mv_pp3_max_check(vq, vport->rx_vqs_num, "rx_vq"))
		return -1;

	if (vport->type != MV_PP3_NSS_PORT_CPU)
		return -1;

	*reset = vport->rx_vqs[vq]->swq->stats_reset_flag;
	return 0;
err:
	pr_err("%s: function failed\n", __func__);
	return -1;
}
/*---------------------------------------------------------------------------*/
int mv_pp3_ingress_vq_reset_stats_set(struct pp3_vport *vport, int vq, int reset)
{
	MV_PP3_NULL_PTR(vport, err);

	if (mv_pp3_max_check(vq, vport->rx_vqs_num, "rx_vq"))
		return -1;

	if (vport->type != MV_PP3_NSS_PORT_CPU)
		return -1;

	vport->rx_vqs[vq]->swq->stats_reset_flag = reset;
	return 0;
err:
	pr_err("%s: function failed\n", __func__);
	return -1;
}


/*---------------------------------------------------------------------------*/
struct pp3_swq_stats *mv_pp3_ingress_vq_sw_stats(struct pp3_vport *vport, int vq)
{

	MV_PP3_NULL_PTR(vport, err);

	if (mv_pp3_max_check(vq, vport->rx_vqs_num, "rx_vq"))
		return NULL;

	if (vport->type != MV_PP3_NSS_PORT_CPU)
		return NULL;

	if (vport->rx_vqs[vq] && vport->rx_vqs[vq]->swq)
		return &vport->rx_vqs[vq]->swq->stats;
err:
	return NULL;
}
/*---------------------------------------------------------------------------*/
int mv_pp3_ingress_vq_drop_get(struct pp3_vport *vport, int vq, struct mv_nss_drop *drop)
{
	struct pp3_vq *vq_priv;

	if (mv_pp3_max_check(vq, vport->rx_vqs_num, "rx_vq"))
		return -1;

	vq_priv = vport->rx_vqs[vq];
	if (!vq_priv) {
		pr_err("%s: Error - ingress VQ #%d is not initialized\n", __func__, vq);
		return -1;
	}
	*drop = *vq_priv->drop;

	return 0;
}

/*-----------------------------------------------------------------------------*/
/* Set priority for given ingress virtual queue (vq) on given virtual port (vp)*/
/*-----------------------------------------------------------------------------*/
int mv_pp3_ingress_vq_prio_set(struct pp3_vport *vport, int vq, u16 prio)
{
	struct pp3_vq *vq_priv;

	if (mv_pp3_max_check(vq, vport->rx_vqs_num, "rx_vq"))
		goto err;

	if (mv_pp3_max_check(prio, MV_PP3_SCHED_PRIO_NUM, "sched_prio"))
		goto err;

	vq_priv = vport->rx_vqs[vq];
	if (!vq_priv) {
		pr_err("%s: Error - ingress VQ #%d is not initialized\n", __func__, vq);
		goto err;
	}
	/* Set priority for Q_LEVEL node */
	if (mv_tm_prio_set(TM_Q_LEVEL, vq_priv->hwq, prio))
		goto err;

	vq_priv->sched->priority = prio;

	return 0;

err:
	pr_err("%s: function failed\n", __func__);
	return -1;
}

/*-----------------------------------------------------------------------------*/
/* Set weight for given ingress virtual queue (vq) on given virtual port (vp)  */
/*-----------------------------------------------------------------------------*/
int mv_pp3_ingress_vq_weight_set(struct pp3_vport *vport, int vq, int mtu, u16 weight)
{
	u32 weight_min, weight_max;
	int anode;
	struct pp3_vq *vq_priv;
	bool enable = weight ? true : false;

	if (mv_pp3_max_check(vq, vport->rx_vqs_num, "rx_vq"))
		goto err;

	vq_priv = vport->rx_vqs[vq];
	if (!vq_priv) {
		pr_err("%s: Error - ingress VQ #%d is not initialized\n", __func__, vq);
		goto err;
	}
	/* Get weight valid range */
	if (mv_tm_quantum_range_get(mtu, &weight_min, &weight_max))
		goto err;

	/* weight == 0 - means disable WRR and don't change weight. Correct weight if wrong */
	if (weight == 0) {
		if (vq_priv->sched->weight < weight_min)
			weight = weight_min;
		else if (vq_priv->sched->weight > weight_max)
			weight = weight_max;
		else
			weight = vq_priv->sched->weight;
	}
	/* Check weight range */
	if (((u32)weight < weight_min) || ((u32)weight > weight_max)) {
		pr_err("Can't set weight for ingress VQ %d: weight %d is out of range [%d .. %d]\n",
			vq, weight, weight_min, weight_max);
		goto err;
	}

	if (weight != vq_priv->sched->weight) {
		/* Set weight for Q_LEVEL node */
		if (mv_tm_dwrr_weight(TM_Q_LEVEL, vq_priv->hwq, weight))
			goto err;

		vq_priv->sched->weight = weight;
	}

	/* Get parent Anode for HWQ */
	if (mv_tm_scheme_parent_node_get(TM_Q_LEVEL, vq_priv->hwq, &anode))
		goto err;

	/* Enable/Disable WRR for relevant priority on A_LEVEL node */
	if (mv_tm_dwrr_enable(TM_A_LEVEL, anode, vq_priv->sched->priority, enable ? MV_ON : MV_OFF))
		goto err;

	vq_priv->sched->wrr_enable = enable;

	return 0;

err:
	pr_err("%s: function failed\n", __func__);
	return -1;
}
/*-----------------------------------------------------------------------------*/

/* Get priority and weight for given ingress virtual queue (vq)	on given vport */
int mv_pp3_ingress_vq_sched_get(struct pp3_vport *vport, int vq, struct mv_nss_sched *sched)
{
	struct pp3_vq *vq_priv;

	if (mv_pp3_max_check(vq, vport->rx_vqs_num, "rx_vq"))
		goto err;

	vq_priv = vport->rx_vqs[vq];
	if (!vq_priv) {
		pr_err("%s: Error - VQ %d is not initialized\n", __func__, vq);
		goto err;
	}

	if (sched)
		*sched = *vq_priv->sched;

	return 0;

err:
	pr_err("%s: function failed\n", __func__);
	return -1;
}
/*---------------------------------------------------------------------------*/

/* COS to VQ mapping for ingress virtual port - valid only for CPU internal port */
int mv_pp3_ingress_cos_to_vq_set(struct pp3_vport *vport, int cos, int vq)
{
	/* Only internal CPU port is supported */
	if (vport->type != MV_PP3_NSS_PORT_CPU) {
		pr_err("%s is not supported for vport type %d - only for MV_PP3_NSS_PORT_CPU (%d)\n",
			__func__, vport->type, MV_PP3_NSS_PORT_CPU);
		goto err;
	}
	if (mv_pp3_max_check(vq, vport->rx_vqs_num, "rx_vq"))
		goto err;

	if (mv_pp3_max_check(cos, MV_PP3_PRIO_NUM, "cos"))
		goto err;

	/* Send message to FW - non blocking */
	if (pp3_fw_cos_to_vq_set(vport->vport, vq, MV_PP3_PPC_TO_HMAC, cos) < 0)
		goto err;

	/* Update cos_to_vq array - used for RX */
	vport->rx_cos_to_vq[cos] = vq;

	return 0;

err:
	pr_err("%s: function failed\n", __func__);
	return -1;
}
/*---------------------------------------------------------------------------*/

int mv_pp3_ingress_cos_to_vq_get(struct pp3_vport *vport, int cos, int *vq)
{

	if (!vport) {
		pr_err("%s: Error - VP is not initialized\n", __func__);
		goto err;
	}

	if (!vq) {
		pr_err("%s: Error - VQ is not initialized\n", __func__);
		goto err;
	}

	if (mv_pp3_max_check(cos, MV_PP3_PRIO_NUM, "cos"))
		goto err;


	/* TBD - send message to FW to get cos to vq mapping */
	*vq = vport->rx_cos_to_vq[cos];

	return 0;
err:
	pr_err("%s: function failed\n", __func__);
	return -1;
}
/*---------------------------------------------------------------------------*/

int mv_pp3_ingress_vq_size_set(struct pp3_vport *vport, int vq, u16 pkts)
{
	struct pp3_vq *vq_priv;
	struct pp3_swq *swq_priv;

	if (mv_pp3_max_check(vq, vport->rx_vqs_num, "rx_vq"))
		goto err;

	vq_priv = vport->rx_vqs[vq];
	if (!vq_priv) {
		pr_err("%s: Error - VQ %d is not initialized\n", __func__, vq);
		goto err;
	}
	swq_priv = vq_priv->swq;
	if (!swq_priv) {
		pr_err("%s: Error - VP #%d / VQ #%d doesn't have SWQ\n", __func__, vport->vport, vq);
		goto err;
	}
	if (mv_pp3_rx_swq_size_set(swq_priv, pkts))
		goto err;

	return 0;
err:
	pr_err("%s: function failed\n", __func__);
	return -1;
}
/*---------------------------------------------------------------------------*/

int mv_pp3_ingress_vq_size_get(struct pp3_vport *vport, int vq, u16 *pkts)
{
	struct pp3_vq *vq_priv;
	struct pp3_swq *swq_priv;

	if (mv_pp3_max_check(vq, vport->rx_vqs_num, "rx_vq"))
		goto err;

	vq_priv = vport->rx_vqs[vq];
	if (!vq_priv) {
		pr_err("%s: Error - VQ %d is not initialized\n", __func__, vq);
		goto err;
	}
	swq_priv = vq_priv->swq;
	if (!swq_priv) {
		pr_err("%s: Error - VP #%d / VQ #%d doesn't have SWQ\n", __func__, vport->vport, vq);
		goto err;
	}
	*pkts = (u16)swq_priv->cur_size;
	return 0;
err:
	pr_err("%s: function failed\n", __func__);
	return -1;
}
/*---------------------------------------------------------------------------*/

int mv_pp3_egress_vq_drop_set(struct pp3_vport *vport, int vq, struct mv_nss_drop *drop)
{
	int dp_id, rc = 0;
	struct pp3_vq *vq_priv;

	if (mv_pp3_max_check(vq, vport->tx_vqs_num, "tx_vq"))
		return -1;

	vq_priv = vport->tx_vqs[vq];
	if (!vq_priv) {
		pr_err("%s: Error - egress VQ #%d is not initialized\n", __func__, vq);
		return -1;
	}
	/* Find exist or create new drop profile with "td" and "red" values */
	dp_id = mv_pp3_dp_q_find(drop->td, drop->red);
	if (dp_id <= 0) {
		rc = -1;
		goto err;
	}

	/* Free old profile ID */
	mv_pp3_dp_q_free(vq_priv->dp_id);

	/* Configure drop_profile to PPC to EMAC HWQ */
	rc = mv_tm_dp_set(TM_Q_LEVEL, vq_priv->hwq, -1, dp_id);
	if (rc) {
		pr_err("Can't attach HWQ #%d to drop profile %d. rc=%d\n", vq_priv->hwq, dp_id, rc);
		goto err;
	}

	vq_priv->dp_id = dp_id;
	*vq_priv->drop = *drop;

err:
	if (rc)
		pr_err("%s: function failed. rc = %d\n", __func__, rc);

	return rc;
}
/*---------------------------------------------------------------------------*/

int mv_pp3_egress_vq_drop_get(struct pp3_vport *vport, int vq, struct mv_nss_drop *drop)
{
	struct pp3_vq *vq_priv;

	if (mv_pp3_max_check(vq, vport->tx_vqs_num, "tx_vq"))
		return -1;

	vq_priv = vport->tx_vqs[vq];
	if (!vq_priv) {
		pr_err("%s: Error - ingress VQ #%d is not initialized\n", __func__, vq);
		return -1;
	}
	*drop = *vq_priv->drop;

	return 0;
}
/*---------------------------------------------------------------------------*/

/* COS to VQ mapping for egress virtual port */
/* EMAC virtual port - send message to FW (ppc_to_emac hwqs) */
/* CPU internal port - save on host to choose egress swq */
int mv_pp3_egress_cos_to_vq_set(struct pp3_vport *vport, int cos, int vq)
{
	if (mv_pp3_max_check(vq, vport->tx_vqs_num, "tx_vq"))
		goto err;

	if (mv_pp3_max_check(cos, MV_PP3_PRIO_NUM, "cos"))
		goto err;

	if (vport->type == MV_PP3_NSS_PORT_ETH) {
		/* Send message to FW - non blocking */
		if (pp3_fw_cos_to_vq_set(vport->vport, vq, MV_PP3_PPC_TO_EMAC, cos) < 0)
			goto err;
	}
	if (vport->type == MV_PP3_NSS_PORT_CPU) {
		/* Update cos_to_vq array - used for TX */
		vport->tx_cos_to_vq[cos] = vq;
	}
	return 0;

err:
	pr_err("%s: function failed\n", __func__);
	return -1;
}
/*---------------------------------------------------------------------------*/

/* Set priority for given egress virtual queue (vq) on given virtual port (vp) */
int mv_pp3_egress_vq_prio_set(struct pp3_vport *vport, int vq, u16 prio)
{
	int anode;
	struct pp3_vq *vq_priv;

	if (mv_pp3_max_check(vq, vport->tx_vqs_num, "tx_vq"))
		goto err;

	if (mv_pp3_max_check(prio, MV_PP3_SCHED_PRIO_NUM, "sched_prio"))
		goto err;

	vq_priv = vport->tx_vqs[vq];
	if (!vq_priv) {
		pr_err("%s: Error - egress VQ #%d is not initialized\n", __func__, vq);
		goto err;
	}

	/* Set priority for Q_LEVEL node */
	if (mv_tm_prio_set(TM_Q_LEVEL, vq_priv->hwq, prio))
		goto err;

	/* Get parent Anode for HWQ */
	if (mv_tm_scheme_parent_node_get(TM_Q_LEVEL, vq_priv->hwq, &anode))
		goto err;

	if (vport->type == MV_PP3_NSS_PORT_CPU) {
		/* Set priority for A_LEVEL node */
		if (mv_tm_prio_set(TM_A_LEVEL, anode, prio))
			goto err;
	} else if (vport->type == MV_PP3_NSS_PORT_ETH) {
		/* Set propagated priority for A node */
		if (mv_tm_prio_set_propagated(TM_A_LEVEL, anode))
			goto err;
	}

	vq_priv->sched->priority = prio;

	return 0;

err:
	pr_err("%s: function failed\n", __func__);
	return -1;
}
/*---------------------------------------------------------------------------*/

/* Set weight for given egress virtual queue (vq) on given virtual port (vp)  */
int mv_pp3_egress_vq_weight_set(struct pp3_vport *vport, int vq, int mtu, u16 weight)
{
	u32 weight_min, weight_max;
	int anode, bnode;
	struct pp3_vq *vq_priv;
	bool enable = weight ? true : false;

	if (mv_pp3_max_check(vq, vport->tx_vqs_num, "tx_vq"))
		goto err;

	vq_priv = vport->tx_vqs[vq];
	if (!vq_priv) {
		pr_err("%s: Error - ingress VQ #%d is not initialized\n", __func__, vq);
		goto err;
	}
	/* Get weight valid range */
	if (mv_tm_quantum_range_get(mtu, &weight_min, &weight_max))
		goto err;

	/* weight == 0 - means disable WRR and don't change weight. Correct weight if wrong */
	if (weight == 0) {
		if (vq_priv->sched->weight < weight_min)
			weight = weight_min;
		else if (vq_priv->sched->weight > weight_max)
			weight = weight_max;
		else
			weight = vq_priv->sched->weight;
	}
	if (((u32)weight < weight_min) || ((u32)weight > weight_max)) {
		pr_err("Can't set weight for ingress VQ %d: weight %d is out of range [%d .. %d]\n",
			vq, weight, weight_min, weight_max);
		goto err;
	}

	/* Get parent Bnode for Anode */
	if (mv_tm_scheme_parent_node_get(TM_Q_LEVEL, vq_priv->hwq, &anode))
		goto err;

	if (weight != vq_priv->sched->weight) {
		/* Set weight for A_LEVEL node */
		if (mv_tm_dwrr_weight(TM_A_LEVEL, anode, weight))
			goto err;

		vq_priv->sched->weight = weight;
	}

	/* Get parent Bnode for Anode */
	if (mv_tm_scheme_parent_node_get(TM_A_LEVEL, anode, &bnode))
		goto err;

	/* Enable/Disable WRR for relevant priority on B_LEVEL node */
	if (mv_tm_dwrr_enable(TM_B_LEVEL, bnode, vq_priv->sched->priority, enable ? MV_ON : MV_OFF))
		goto err;

	vq_priv->sched->wrr_enable = enable;

	return 0;

err:
	pr_err("%s: function failed\n", __func__);
	return -1;
}
/*---------------------------------------------------------------------------*/

/* Get priority and weight for given egress virtual queue (vq)	on given vport */
int mv_pp3_egress_vq_sched_get(struct pp3_vport *vport, int vq, struct mv_nss_sched *sched)
{
	struct pp3_vq *vq_priv;

	if (mv_pp3_max_check(vq, vport->tx_vqs_num, "tx_vq"))
		goto err;

	vq_priv = vport->tx_vqs[vq];
	if (!vq_priv) {
		pr_err("%s: Error - VQ %d is not initialized\n", __func__, vq);
		goto err;
	}
	if (sched)
		*sched = *vq_priv->sched;

	return 0;

err:
	pr_err("%s: function failed\n", __func__);
	return -1;
}
/*---------------------------------------------------------------------------*/

int mv_pp3_egress_vq_size_set(struct pp3_vport *vport, int vq, u16 pkts)
{
	struct pp3_vq *vq_priv;
	struct pp3_swq *swq_priv;

	if (mv_pp3_max_check(vq, vport->tx_vqs_num, "tx_vq"))
		goto err;

	vq_priv = vport->tx_vqs[vq];
	if (!vq_priv) {
		pr_err("%s: Error - VQ %d is not initialized\n", __func__, vq);
		goto err;
	}
	swq_priv = vq_priv->swq;
	if (!swq_priv) {
		pr_err("%s: Error - VP #%d / VQ #%d doesn't have SWQ\n", __func__, vport->vport, vq);
		goto err;
	}
	if (mv_pp3_tx_swq_size_set(swq_priv, pkts))
		goto err;

	return 0;
err:
	pr_err("%s: function failed\n", __func__);
	return -1;
}
/*---------------------------------------------------------------------------*/

int mv_pp3_egress_vq_size_get(struct pp3_vport *vport, int vq, u16 *pkts)
{
	struct pp3_vq *vq_priv;
	struct pp3_swq *swq_priv;

	if (mv_pp3_max_check(vq, vport->tx_vqs_num, "tx_vq"))
		goto err;

	vq_priv = vport->tx_vqs[vq];
	if (!vq_priv) {
		pr_err("%s: Error - VQ %d is not initialized\n", __func__, vq);
		goto err;
	}
	swq_priv = vq_priv->swq;
	if (!swq_priv) {
		pr_err("%s: Error - VP #%d / VQ #%d doesn't have SWQ\n", __func__, vport->vport, vq);
		goto err;
	}
	*pkts = (u16)swq_priv->cur_size;
	return 0;
err:
	pr_err("%s: function failed\n", __func__);
	return -1;
}
/*---------------------------------------------------------------------------*/

int mv_pp3_egress_vq_rate_limit_set(struct pp3_vport *vport, int vq, struct mv_nss_meter *meter)
{
	int rc, anode;
	u32 cbs_good, ebs_good;
	struct pp3_vq *vq_priv;

	if (mv_pp3_max_check(vq, vport->tx_vqs_num, "tx_vq"))
		goto err;

	vq_priv = vport->tx_vqs[vq];
	if (!vq_priv) {
		pr_err("%s: Error - VQ %d is not initialized\n", __func__, vq);
		goto err;
	}
	/* Get parent Anode for HWQ */
	if (mv_tm_scheme_parent_node_get(TM_Q_LEVEL, vq_priv->hwq, &anode))
		goto err;

	cbs_good = meter->cbs;
	ebs_good = meter->ebs;
	rc = mv_tm_set_shaping_ex(TM_A_LEVEL, anode, meter->cir, meter->eir, &cbs_good, &ebs_good);
	if (rc) {
		if (meter->cbs != cbs_good) {
			/* cbs value is too small. Use minimal valid value instead */
			pr_warn("%s: cbs = %d KBytes is too small. Use cbs = %d KBytes\n",
				__func__, meter->cbs, cbs_good);
		}

		if (meter->ebs != ebs_good) {
			/* ebs value is too small. Use minimal valid value instead */
			pr_warn("%s: ebs = %d KBytes is too small. Use ebs = %d KBytes\n",
				__func__, meter->ebs, ebs_good);
		}

		rc = mv_tm_set_shaping_ex(TM_A_LEVEL, anode, meter->cir, meter->eir, &cbs_good, &ebs_good);
		if (rc)
			goto err;
	}
	*vq_priv->meter = *meter;
	return 0;
err:
	pr_err("%s: function failed\n", __func__);
	return -1;
}
/*---------------------------------------------------------------------------*/

int mv_pp3_egress_vq_rate_limit_get(struct pp3_vport *vport, int vq, struct mv_nss_meter *meter)
{
	struct pp3_vq *vq_priv;

	if (mv_pp3_max_check(vq, vport->tx_vqs_num, "tx_vq"))
		goto err;

	vq_priv = vport->tx_vqs[vq];
	if (!vq_priv) {
		pr_err("%s: Error - VQ %d is not initialized\n", __func__, vq);
		goto err;
	}
	if (meter)
		*meter = *vq_priv->meter;

	return 0;
err:
	pr_err("%s: function failed\n", __func__);
	return -1;
}
/*---------------------------------------------------------------------------*/


/*---------------------------------------------------------------------------
mv_pp3_vqueue_cnt_dump
	print vqueue counters according to number of vq_num
	print only if counter is not zeroed
	calculate and print sum value for vq_num > 1
	inputs:
		vq - array of CPU VPORTs pointers
		cpu_vp_num - size of array
		cnt_index - stat counter index
		name - counter name
---------------------------------------------------------------------------*/
void mv_pp3_vqueue_cnt_dump_header(int vq_num)
{
	mv_pp3_swq_cnt_dump_header(MV_MIN(vq_num, MV_PP3_VQ_NUM));
}
/*---------------------------------------------------------------------------*/

void mv_pp3_vqueue_cnt_dump(const char *cntr_pref, int cpu, struct pp3_vq **vq, int vq_num, bool pr_cntr_name)
{
	struct pp3_swq *swq[MV_PP3_VQ_NUM];
	int q;

	for (q = 0; (q < vq_num) && (q < MV_PP3_VQ_NUM); q++)
		swq[q] = vq[q]->swq;

	mv_pp3_swq_cnt_dump(cntr_pref, cpu, swq, q, pr_cntr_name);
}
/*---------------------------------------------------------------------------*/
