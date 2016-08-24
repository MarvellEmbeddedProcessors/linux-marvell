/*******************************************************************************
ropyright (C) Marvell International Ltd. and its affiliates

This software file (the "File") is owned and distributed by Marvell
International Ltd. and/or its affiliates ("Marvell") under the following
alternative licensing terms.  Once you have made an election to distribute the
File under one of the following license alternatives, please (i) delete this
introductory statement regarding license alternatives, (ii) delete the two
license alternatives that you have not elected to use and (iii) preserve the
Marvell copyright notice above.


********************************************************************************
Marvell GPL License Option

If you received this File from Marvell, you may opt to use, redistribute and/or
modify this File in accordance with the terms and conditions of the General
Public License Version 2, June 1991 (the "GPL License"), a copy of which is
available along with the File in the license.txt file or by writing to the Free
Software Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 or
on the worldwide web at http://www.gnu.org/licenses/gpl.txt.

THE FILE IS DISTRIBUTED AS-IS, WITHOUT WARRANTY OF ANY KIND, AND THE IMPLIED
WARRANTIES OF MERCHANTABILITY OR FITNESS FOR A PARTICULAR PURPOSE ARE EXPRESSLY
DISCLAIMED.  The GPL License provides additional details about this warranty
disclaimer.
*******************************************************************************/
#include "mv_netdev_structs.h"
#include "mv_netdev.h"
#include "mv_dev_vq.h"
#include "tm/wrappers/mv_tm_drop.h"
#include "hmac/mv_hmac.h"

/* global lock for ingress VQ priority change */
static DEFINE_SPINLOCK(napi_list_lock);

/*---------------------------------------------------------------------------*/
/* Ingress virtual queue (vq) configurations.                                */
/*---------------------------------------------------------------------------*/

/* Get number of ingress virtual queues of the network device */
int mv_pp3_dev_ingress_vqs_num_get(struct net_device *dev, int *vqs_num)
{
	int cpu;
	struct pp3_vport *cpu_vp = NULL;
	struct pp3_dev_priv *dev_priv = MV_PP3_PRIV(dev);

	if (!dev_priv) {
		pr_err("%s: Interface %s is not initialized\n", __func__, dev->name);
		goto err;
	}

	/* Number of ingress VQs is the same for for both CPUs - return first found */
	for_each_possible_cpu(cpu) {
		cpu_vp = dev_priv->cpu_vp[cpu];
		if (cpu_vp && cpumask_test_cpu(cpu, &dev_priv->rx_cpus))
			break;
	}
	if (cpu_vp) {
		*vqs_num = cpu_vp->rx_vqs_num;
		return 0;
	}

err:
	pr_err("%s: function failed\n", __func__);
	return -1;
}
/*---------------------------------------------------------------------------*/

int mv_pp3_dev_ingress_cos_show(struct net_device *netdev)
{
	int cos;
	int vqs[MV_PP3_PRIO_NUM];

	pr_info("\n%s: ingress CoS to VQ mapping\n", netdev->name);
	for (cos = 0; cos < MV_PP3_PRIO_NUM; cos++) {
		if (mv_pp3_dev_ingress_cos_to_vq_get(netdev, cos, &vqs[cos]))
			vqs[cos] = -1;
	}
	pr_cont("\n");
	pr_cont("cos:  ");
	for (cos = 0; cos < MV_PP3_PRIO_NUM; cos++)
		pr_cont("%2d  ", cos);
	pr_cont("\n");
	pr_cont("vq :  ");
	for (cos = 0; cos < MV_PP3_PRIO_NUM; cos++)
		pr_cont("%2d  ", vqs[cos]);

	pr_info("\n");
	return 0;
}
/*---------------------------------------------------------------------------*/

/* Map cos value to ingress virtual queue [vq] of the network device */
int mv_pp3_dev_ingress_cos_to_vq_set(struct net_device *dev, int cos, int vq)
{
	int cpu, rc = 0;
	struct pp3_vport *cpu_vp;
	struct pp3_dev_priv *dev_priv = MV_PP3_PRIV(dev);

	if (!dev_priv) {
		pr_err("%s: Interface %s is not initialized\n", __func__, dev->name);
		goto err;
	}

	/* on ingress only CPU internal ports support cos to vq mapping */
	for_each_possible_cpu(cpu) {
		cpu_vp = dev_priv->cpu_vp[cpu];
		if (!cpu_vp || (vq >= cpu_vp->rx_vqs_num))
			continue;

		rc = mv_pp3_ingress_cos_to_vq_set(cpu_vp, cos, vq);
		if (rc)
			goto err;
	}

	return 0;
err:
	pr_err("%s: function failed\n", __func__);
	return -1;
}
/*---------------------------------------------------------------------------*/

/* Get ingress virtual queue [vq] mapped on [cos] value for network device */
int mv_pp3_dev_ingress_cos_to_vq_get(struct net_device *dev, int cos, int *vq)
{
	int cpu, rc = 0;
	struct pp3_vport *cpu_vp = NULL;
	struct pp3_dev_priv *dev_priv = MV_PP3_PRIV(dev);

	if (!dev_priv) {
		pr_err("%s: Interface %s is not initialized\n", __func__, dev->name);
		goto err;
	}
	/* on ingress only CPU internal ports support cos to vq mapping */
	for_each_possible_cpu(cpu) {
		cpu_vp = dev_priv->cpu_vp[cpu];
		if (cpu_vp && cpumask_test_cpu(cpu, &dev_priv->rx_cpus))
			break;
	}
	if (cpu_vp) {
		rc = mv_pp3_ingress_cos_to_vq_get(cpu_vp, cos, vq);
		return rc;
	}

err:
	pr_err("%s: function failed\n", __func__);
	return -1;
}
/*---------------------------------------------------------------------------*/

/* Set tail drop (TD) and random early drop (RED) values for given ingress virtual queue (vq) */
int mv_pp3_dev_ingress_vq_drop_set(struct net_device *dev, int vq, struct mv_nss_drop *drop)
{
	int cpu, rc = 0;
	struct pp3_vport *cpu_vp;
	struct pp3_dev_priv *dev_priv = MV_PP3_PRIV(dev);

	if (!dev_priv) {
		pr_err("%s: Error - %s in not initialized\n", __func__, dev->name);
		return -1;
	}
	/* On ingress drop configuration supported only for CPU internal ports */
	for_each_possible_cpu(cpu) {
		cpu_vp = dev_priv->cpu_vp[cpu];
		if (!cpu_vp || (vq >= cpu_vp->rx_vqs_num))
			continue;

		rc = mv_pp3_ingress_vq_drop_set(cpu_vp, vq, drop);
		if (rc)
			break;
	}
	return rc;
}
/*---------------------------------------------------------------------------*/

int mv_pp3_dev_ingress_vq_drop_get(struct net_device *dev, int vq, struct mv_nss_drop *drop)
{
	int cpu, rc = -1;
	struct pp3_vport *cpu_vp = NULL;
	struct pp3_dev_priv *dev_priv = MV_PP3_PRIV(dev);

	if (!dev_priv) {
		pr_err("%s: Error - %s in not initialized\n", __func__, dev->name);
		return -1;
	}

	for_each_possible_cpu(cpu) {
		/* drop configurations must be same for both CPUs - return first found */
		cpu_vp = dev_priv->cpu_vp[cpu];
		if (cpu_vp && cpumask_test_cpu(cpu, &dev_priv->rx_cpus))
			break;
	}
	if (cpu_vp)
		rc = mv_pp3_ingress_vq_drop_get(cpu_vp, vq, drop);

	return rc;
}
/*---------------------------------------------------------------------------*/

/* Set priority for given ingress virtual queue (vq) */
int mv_pp3_dev_ingress_vq_prio_set(struct net_device *dev, int vq, u16 prio)
{
	int cpu, rc = 0;
	struct pp3_vport *cpu_vp;
	struct pp3_dev_priv *dev_priv = MV_PP3_PRIV(dev);

	if (!dev_priv) {
		pr_err("%s: Error - %s in not initialized\n", __func__, dev->name);
		return -1;
	}

	for_each_possible_cpu(cpu) {
		cpu_vp = dev_priv->cpu_vp[cpu];
		if (!cpu_vp || (vq >= cpu_vp->rx_vqs_num))
			continue;

		rc = mv_pp3_ingress_vq_prio_set(cpu_vp, vq, prio);
		if (rc)
			break;
	}
	mv_pp3_dev_napi_queue_update(dev);

	return rc;
}
/*---------------------------------------------------------------------------*/

/* Set weight for given ingress virtual queue (vq) */
int mv_pp3_dev_ingress_vq_weight_set(struct net_device *dev, int vq, u16 weight)
{
	int cpu, rc = 0;
	struct pp3_vport *cpu_vp;
	struct pp3_dev_priv *dev_priv = MV_PP3_PRIV(dev);

	if (!dev_priv) {
		pr_err("%s: Error - %s in not initialized\n", __func__, dev->name);
		return -1;
	}

	for_each_possible_cpu(cpu) {
		cpu_vp = dev_priv->cpu_vp[cpu];
		if (!cpu_vp || (vq >= cpu_vp->rx_vqs_num))
			continue;

		rc = mv_pp3_ingress_vq_weight_set(cpu_vp, vq, dev->mtu, weight);
		if (rc)
			break;
	}
	return rc;
}
/*---------------------------------------------------------------------------*/

int mv_pp3_dev_ingress_vq_sched_get(struct net_device *dev, int vq, struct mv_nss_sched *sched)
{
	int cpu, rc = -1;
	struct pp3_vport *cpu_vp = NULL;
	struct pp3_dev_priv *dev_priv = MV_PP3_PRIV(dev);

	if (!dev_priv) {
		pr_err("%s: Error - %s in not initialized\n", __func__, dev->name);
		return -1;
	}

	for_each_possible_cpu(cpu) {
		/* drop configurations must be same for both CPUs - return first found */
		cpu_vp = dev_priv->cpu_vp[cpu];
		if (cpu_vp && cpumask_test_cpu(cpu, &dev_priv->rx_cpus))
			break;
	}
	if (cpu_vp)
		rc = mv_pp3_ingress_vq_sched_get(cpu_vp, vq, sched);

	return rc;
}
/*---------------------------------------------------------------------------*/

/* Set size (Xoff) [pkts] given ingress virtual queue (vq) */
int mv_pp3_dev_ingress_vq_size_set(struct net_device *dev, int vq, u16 size)
{
	int cpu, rc = -1;
	struct pp3_vport *cpu_vp;
	struct pp3_dev_priv *dev_priv = MV_PP3_PRIV(dev);

	if (!dev_priv) {
		pr_err("%s: Error - %s in not initialized\n", __func__, dev->name);
		return -1;
	}
	for_each_possible_cpu(cpu) {
		cpu_vp = dev_priv->cpu_vp[cpu];
		if (!cpu_vp || (vq >= cpu_vp->rx_vqs_num))
			continue;

		rc = mv_pp3_ingress_vq_size_set(cpu_vp, vq, size);
		if (rc)
			break;
	}
	return rc;
}
/*---------------------------------------------------------------------------*/

int mv_pp3_dev_ingress_vq_size_get(struct net_device *dev, int vq, u16 *size)
{
	int cpu, rc = -1;
	struct pp3_vport *cpu_vp = NULL;
	struct pp3_dev_priv *dev_priv = MV_PP3_PRIV(dev);

	if (!dev_priv) {
		pr_err("%s: Error - %s in not initialized\n", __func__, dev->name);
		return -1;
	}

	for_each_possible_cpu(cpu) {
		/* drop configurations must be same for both CPUs - return first found */
		cpu_vp = dev_priv->cpu_vp[cpu];
		if (cpu_vp && cpumask_test_cpu(cpu, &dev_priv->rx_cpus))
			break;
	}
	if (cpu_vp)
		rc = mv_pp3_ingress_vq_size_get(cpu_vp, vq, size);

	return rc;
}
/*---------------------------------------------------------------------------*/

int mv_pp3_dev_ingress_vqs_defaults_set(struct net_device *dev)
{
	int vq, vqs_num, prio;
	struct mv_nss_drop drop;

	if (mv_pp3_dev_ingress_vqs_num_get(dev, &vqs_num)) {
		pr_err("%s: Error - Can't get number of ingress virtual queues\n", dev->name);
		return -1;
	}
	for (vq = 0; vq < vqs_num; vq++) {

		prio = vq;
		if (prio >= MV_PP3_SCHED_PRIO_NUM)
			prio = (MV_PP3_SCHED_PRIO_NUM - 1);

		if (mv_pp3_dev_ingress_vq_prio_set(dev, vq, prio))
			return -1;

		/* Disable DWRR by default */
		if (mv_pp3_dev_ingress_vq_weight_set(dev, vq, 0))
			return -1;

		/* Set default TD and RED thresholds */
		drop.td = MV_PP3_INGRESS_TD_DEF;
		drop.red = MV_PP3_INGRESS_RED_DEF;
		drop.enable = true;
		if (mv_pp3_dev_ingress_vq_drop_set(dev, vq, &drop))
			return -1;
	}

	return 0;
}
/*---------------------------------------------------------------------------*/

/*---------------------------------------------------------------------------*/
/* Egress virtual queue (vq) configurations                                  */
/*---------------------------------------------------------------------------*/

/* Get number of egress virtual queues of the network device */
int mv_pp3_dev_egress_vqs_num_get(struct net_device *dev, int *vqs_num)
{
	int cpu;
	struct pp3_vport *cpu_vp = NULL;
	struct pp3_dev_priv *dev_priv = MV_PP3_PRIV(dev);

	if (!dev_priv) {
		pr_err("%s: Interface %s is not initialized\n", __func__, dev->name);
		goto err;
	}

	/* Number of egress VQs is the same for for both CPUs - return first found */
	for_each_possible_cpu(cpu) {
		cpu_vp = dev_priv->cpu_vp[cpu];
		if (cpu_vp)
			break;
	}
	if (cpu_vp) {
		*vqs_num = cpu_vp->tx_vqs_num;
		return 0;
	}

err:
	pr_err("%s: function failed\n", __func__);
	return -1;
}
/*---------------------------------------------------------------------------*/

int mv_pp3_dev_egress_cos_show(struct net_device *netdev)
{
	int cos;
	int vqs[MV_PP3_PRIO_NUM];

	pr_info("\n%s: egress CoS to VQ mapping\n", netdev->name);
	for (cos = 0; cos < MV_PP3_PRIO_NUM; cos++) {
		if (mv_pp3_dev_egress_cos_to_vq_get(netdev, cos, &vqs[cos]))
			vqs[cos] = -1;
	}
	pr_cont("cos:  ");
	for (cos = 0; cos < MV_PP3_PRIO_NUM; cos++)
		pr_cont("%2d  ", cos);

	pr_cont("\n");
	pr_cont("vq :  ");
	for (cos = 0; cos < MV_PP3_PRIO_NUM; cos++)
		pr_cont("%2d  ", vqs[cos]);

	pr_info("\n");
	return 0;
}
/*---------------------------------------------------------------------------*/

/* Map cos value to virtual queue [q] */
int mv_pp3_dev_egress_cos_to_vq_set(struct net_device *dev, int cos, int vq)
{
	int cpu, rc = 0;
	struct pp3_vport *vp;
	struct pp3_dev_priv *dev_priv = MV_PP3_PRIV(dev);

	if (!dev_priv) {
		pr_err("%s: Interface %s is not initialized\n", __func__, dev->name);
		goto err;
	}

	/* Single EMAC / External virtual port support cos to vq mapping */
	vp = dev_priv->vport;
	if (vp && (vp->type == MV_PP3_NSS_PORT_ETH)) {
		if (mv_pp3_egress_cos_to_vq_set(vp, cos, vq))
			goto err;
	}
	/* internal CPU ports support cos to vq mapping */
	for_each_possible_cpu(cpu) {
		vp = dev_priv->cpu_vp[cpu];
		if (!vp)
			continue;

		if (mv_pp3_egress_cos_to_vq_set(vp, cos, vq))
			goto err;
	}
	return rc;
err:
	pr_err("%s: function failed\n", __func__);
	return -1;
}
/*---------------------------------------------------------------------------*/

/* Get egress virtual queue [vq] mapped on [cos] value for network device */
int mv_pp3_dev_egress_cos_to_vq_get(struct net_device *dev, int cos, int *vq)
{
	int cpu;
	struct pp3_vport *vp = NULL;
	struct pp3_dev_priv *dev_priv = MV_PP3_PRIV(dev);

	if (!dev_priv) {
		pr_err("%s: Interface %s is not initialized\n", __func__, dev->name);
		goto err;
	}

	/*
	vp = dev_priv->vport;
	if (vp) {
		*vq = mv_pp3_egress_cos_to_vq_get(vp, cos);
		return 0;
	}
	*/
	/* CoS to VQ mapping is the same for all vports of network device */
	for_each_possible_cpu(cpu) {
		vp = dev_priv->cpu_vp[cpu];
		if (vp)
			break;
	}
	if (vp) {
		*vq = mv_pp3_egress_cos_to_vq_get(vp, cos);
		return 0;
	}
err:
	pr_err("%s: function failed\n", __func__);
	return -1;
}
/*---------------------------------------------------------------------------*/

/* Set tail drop (TD) and random early drop (RED) values for given egress virtual queue (vq) */
int mv_pp3_dev_egress_vq_drop_set(struct net_device *dev, int vq, struct mv_nss_drop *drop)
{
	int rc = -1;
	struct pp3_vport *vp;
	struct pp3_dev_priv *dev_priv = MV_PP3_PRIV(dev);

	if (!dev_priv) {
		pr_err("%s: Error - %s in not initialized\n", __func__, dev->name);
		return -1;
	}
	/* For egress direction drop configuration supported only for EMAC virtual ports */
	vp = dev_priv->vport;
	if (vp && (vp->type == MV_PP3_NSS_PORT_ETH))
		rc = mv_pp3_egress_vq_drop_set(vp, vq, drop);

	return rc;
}
/*---------------------------------------------------------------------------*/

/* Get tail drop (TD) and random early drop (RED) values for given egress virtual queue (vq) */
int mv_pp3_dev_egress_vq_drop_get(struct net_device *dev, int vq, struct mv_nss_drop *drop)
{
	int rc = -1;
	struct pp3_vport *vp;
	struct pp3_dev_priv *dev_priv = MV_PP3_PRIV(dev);

	if (!dev_priv) {
		pr_err("%s: Error - %s in not initialized\n", __func__, dev->name);
		return -1;
	}
	/* For egress direction drop configuration supported only for EMAC virtual ports */
	vp = dev_priv->vport;
	if (vp && (vp->type == MV_PP3_NSS_PORT_ETH))
		rc = mv_pp3_egress_vq_drop_get(vp, vq, drop);

	return rc;
}
/*---------------------------------------------------------------------------*/

/* Set priority for given egress virtual queue (vq) */
int mv_pp3_dev_egress_vq_prio_set(struct net_device *dev, int vq, u16 prio)
{
	int cpu, rc = 0;
	struct pp3_vport *cpu_vp, *vp;
	struct pp3_dev_priv *dev_priv = MV_PP3_PRIV(dev);

	if (!dev_priv) {
		pr_err("%s: Error - %s in not initialized\n", __func__, dev->name);
		return -1;
	}

	vp = dev_priv->vport;
	if (vp && (vp->type == MV_PP3_NSS_PORT_ETH)) {
		rc = mv_pp3_egress_vq_prio_set(vp, vq, prio);
		if (rc)
			return rc;
	}
	for_each_possible_cpu(cpu) {
		cpu_vp = dev_priv->cpu_vp[cpu];
		if (!cpu_vp)
			continue;

		rc = mv_pp3_egress_vq_prio_set(cpu_vp, vq, prio);
		if (rc)
			break;
	}
	return rc;
}
/*---------------------------------------------------------------------------*/

/* Set weight for given egress virtual queue (vq) */
int mv_pp3_dev_egress_vq_weight_set(struct net_device *dev, int vq, u16 weight)
{
	int cpu, rc = 0;
	struct pp3_vport *cpu_vp, *vp;
	struct pp3_dev_priv *dev_priv = MV_PP3_PRIV(dev);

	if (!dev_priv) {
		pr_err("%s: Error - %s in not initialized\n", __func__, dev->name);
		return -1;
	}

	vp = dev_priv->vport;
	if (vp && (vp->type == MV_PP3_NSS_PORT_ETH)) {
		rc = mv_pp3_egress_vq_weight_set(vp, vq, dev->mtu, weight);
		if (rc)
			return rc;
	}

	for_each_possible_cpu(cpu) {
		cpu_vp = dev_priv->cpu_vp[cpu];
		if (!cpu_vp)
			continue;

		rc = mv_pp3_egress_vq_weight_set(cpu_vp, vq, dev->mtu, weight);
		if (rc)
			break;
	}
	return rc;
}
/*---------------------------------------------------------------------------*/

int mv_pp3_dev_egress_vq_sched_get(struct net_device *dev, int vq, struct mv_nss_sched *sched)
{
	int cpu, rc = -1;
	struct pp3_vport *cpu_vp = NULL;
	struct pp3_dev_priv *dev_priv = MV_PP3_PRIV(dev);

	if (!dev_priv) {
		pr_err("%s: Error - %s in not initialized\n", __func__, dev->name);
		return -1;
	}

	for_each_possible_cpu(cpu) {
		cpu_vp = dev_priv->cpu_vp[cpu];
		if (!cpu_vp)
			continue;

		/* drop configurations must be same for both CPUs - return first found */
		break;
	}
	if (cpu_vp)
		rc = mv_pp3_egress_vq_sched_get(cpu_vp, vq, sched);

	return rc;
}
/*---------------------------------------------------------------------------*/

/* Set size [pkts] given egress virtual queue (vq) */
int mv_pp3_dev_egress_vq_size_set(struct net_device *dev, int vq, u16 size)
{
	int cpu, rc = -1;
	struct pp3_vport *cpu_vp;
	struct pp3_dev_priv *dev_priv = MV_PP3_PRIV(dev);

	if (!dev_priv) {
		pr_err("%s: Error - %s in not initialized\n", __func__, dev->name);
		return -1;
	}
	for_each_possible_cpu(cpu) {
		cpu_vp = dev_priv->cpu_vp[cpu];
		if (!cpu_vp)
			continue;

		rc = mv_pp3_egress_vq_size_set(cpu_vp, vq, size);
		if (rc)
			break;
	}
	return rc;
}
/*---------------------------------------------------------------------------*/

int mv_pp3_dev_egress_vq_size_get(struct net_device *dev, int vq, u16 *size)
{
	int cpu, rc = -1;
	struct pp3_vport *cpu_vp = NULL;
	struct pp3_dev_priv *dev_priv = MV_PP3_PRIV(dev);

	if (!dev_priv) {
		pr_err("%s: Error - %s in not initialized\n", __func__, dev->name);
		return -1;
	}

	for_each_possible_cpu(cpu) {
		cpu_vp = dev_priv->cpu_vp[cpu];
		if (!cpu_vp)
			continue;

		/* VQ size must be same for both CPUs - return first found */
		break;
	}
	if (cpu_vp)
		rc = mv_pp3_egress_vq_size_get(cpu_vp, vq, size);

	return rc;
}
/*---------------------------------------------------------------------------*/

/* Set rate limit values to hmac_to_ppc Anodes */
int mv_pp3_dev_egress_vq_rate_limit_set(struct net_device *dev, int vq, struct mv_nss_meter *meter)
{
	int cpu, rc = -1;
	struct pp3_vport *cpu_vp = NULL;
	struct pp3_dev_priv *dev_priv = MV_PP3_PRIV(dev);

	if (!dev_priv) {
		pr_err("%s: Error - %s in not initialized\n", __func__, dev->name);
		return -1;
	}
	for_each_possible_cpu(cpu) {
		cpu_vp = dev_priv->cpu_vp[cpu];
		if (!cpu_vp)
			continue;

		rc = mv_pp3_egress_vq_rate_limit_set(cpu_vp, vq, meter);
		if (rc)
			break;
	}
	return rc;
}
/*---------------------------------------------------------------------------*/

/* Get rate limit values for hmac_to_ppc Anodes */
int mv_pp3_dev_egress_vq_rate_limit_get(struct net_device *dev, int vq, struct mv_nss_meter *meter)
{
	int cpu, rc = -1;
	struct pp3_vport *cpu_vp = NULL;
	struct pp3_dev_priv *dev_priv = MV_PP3_PRIV(dev);

	if (!dev_priv) {
		pr_err("%s: Error - %s in not initialized\n", __func__, dev->name);
		return -1;
	}
	for_each_possible_cpu(cpu) {
		cpu_vp = dev_priv->cpu_vp[cpu];
		if (cpu_vp)
			break;
	}
	if (cpu_vp)
		rc = mv_pp3_egress_vq_rate_limit_get(cpu_vp, vq, meter);

	return rc;
}
/*---------------------------------------------------------------------------*/

/* Set rate limit values to hmac_to_ppc Anodes */
int mv_pp3_dev_egress_vport_shaper_set(struct net_device *dev, struct mv_nss_meter *meter)
{
	int rc = -1;
	struct pp3_vport *emac_vp = NULL;
	struct pp3_dev_priv *dev_priv = MV_PP3_PRIV(dev);

	if (!dev_priv) {
		pr_err("%s: Error - %s in not initialized\n", __func__, dev->name);
		return -1;
	}
	emac_vp = dev_priv->vport;
	if (emac_vp && (emac_vp->type == MV_PP3_NSS_PORT_ETH))
		rc = mv_pp3_egress_vport_shaper_set(emac_vp, meter);
	else
		pr_err("%s: Vport shaper is not supported\n", dev->name);

	return rc;
}

/*---------------------------------------------------------------------------*/
/* Set/init defaults for Queues and for Ports */
int mv_pp3_dev_egress_vqs_defaults_set(struct net_device *dev)
{
	int vq, vqs_num, prio;
	struct mv_nss_meter meter;
	struct pp3_vport *emac_vp = NULL;
	struct pp3_dev_priv *dev_priv = MV_PP3_PRIV(dev);

	if (!dev_priv) {
		pr_err("%s: Error - %s in not initialized\n", __func__, dev->name);
		return -1;
	}

	if (mv_pp3_dev_egress_vqs_num_get(dev, &vqs_num)) {
		pr_err("%s: Error - Can't get number of egress virtual queues\n", dev->name);
		return -1;
	}
	/* set default values cir and eir values for all VQs */
	/* No EMAC   : cir =  2000 Mbps, eir = 0 Mbps, cbs = 16 KBytes, ebs = 16 KBytes */
	/* RXAUI 10G : cir = 10000 Mbps, eir = 0 Mbps, cbs = 64 KBytes, ebs = 64 KBytes */
	/* SGMII 2.5G: cir =  2500 Mbps, eir = 0 Mbps, cbs = 16 KBytes, ebs = 16 KBytes */
	/* Others 1G : cir =  1000 Mbps, eir = 0 Mbps, cbs = 16 KBytes, ebs = 16 KBytes */
	if (dev_priv->vport && (dev_priv->vport->type != MV_PP3_NSS_PORT_ETH)) {
		meter.cir = 2000;
		meter.eir = 0;
		meter.cbs = 16;
		meter.ebs = 16;
	} else {
		emac_vp = dev_priv->vport;
		if (!emac_vp)
			return -1;

		if (emac_vp->port.emac.port_mode == MV_PORT_RXAUI) {
			meter.cir = 10000;
			meter.eir = 0;
			meter.cbs = 64;
			meter.ebs = 64;
		} else if (emac_vp->port.emac.port_mode == MV_PORT_SGMII2_5) {
			meter.cir = 2500;
			meter.eir = 0;
			meter.cbs = 16;
			meter.ebs = 16;
		} else {
			meter.cir = 1000;
			meter.eir = 0;
			meter.cbs = 16;
			meter.ebs = 16;
		}

		if (mv_pp3_egress_vport_shaper_set(emac_vp, &meter))
			return -1;
	}

	for (vq = 0; vq < vqs_num; vq++) {

		prio = vq;
		if (prio >= MV_PP3_SCHED_PRIO_NUM)
			prio = (MV_PP3_SCHED_PRIO_NUM - 1);

		if (mv_pp3_dev_egress_vq_prio_set(dev, vq, prio)) {
			pr_err("%s: Can't set priority %d for vq #%d\n", dev->name, prio, vq);
			return -1;
		}

		/* Disable DWRR by default */
		if (mv_pp3_dev_egress_vq_weight_set(dev, vq, 0)) {
			pr_err("%s: Can't set weight 0 (disable( for vq #%d\n", dev->name, vq);
			return -1;
		}
		if (mv_pp3_dev_egress_vq_rate_limit_set(dev, vq, &meter))
			return -1;
	}
	return 0;
}
/*---------------------------------------------------------------------------*/

static void mv_pp3_vq_prio_sort(struct pp3_vq **vq_list, int *napi_rxq, int q_nums)
{
	int tmp, i, j;

	for (i = 0; i < (q_nums - 1); i++) {
		for (j = 0; j < q_nums - 1 - i; j++) {
			if (!(vq_list[napi_rxq[j]]) || !(vq_list[napi_rxq[j+1]]))
				continue;
			if (vq_list[napi_rxq[j]]->sched->priority > vq_list[napi_rxq[j+1]]->sched->priority) {
				tmp = napi_rxq[j+1];
				napi_rxq[j+1] = napi_rxq[j];
				napi_rxq[j] = tmp;
			}
		}
	}
}

static inline int mv_pp3_free_array_ind_get(struct pp3_cpu_port *cpu)
{
	int i;

	/* find free queues list for changes:
	 * napi_proc_qs[3][MV_PP3_VQ_NUM] keeps napi/vq mapping
	 * napi_proc_qs[array_ind=i] is updated upon suspend, resume
	 *  and then "i" switched to be current-active.
	 * Before napi suspend/resume delete/add operation
	 * the napi_proc_qs[i][..vq..] must be sync to the
	 * latest valid (current) napi_next_array.
	 * Refer mv_pp3_napi_array_sync()
	 */
	for (i = 0; i < 3; i++)
		if ((i != cpu->napi_master_array) && (i != cpu->napi_next_array))
			return i;
	return -1;
}

static inline void mv_pp3_napi_array_sync(struct pp3_cpu_port *cpu,
						int old, int new, int num)
{
	int i;

	if (old == new)
		return;
	for (i = 0; i < num; i++)
		cpu->napi_proc_qs[new][i] = cpu->napi_proc_qs[old][i];
}

void mv_pp3_dev_napi_queue_update(struct net_device *dev)
{
	int cpu;
	struct pp3_dev_priv *dev_priv = MV_PP3_PRIV(dev);
	unsigned long flags = 0;
	int array_ind, i;
	struct pp3_vport *cpu_vp;

	MV_LOCK(&napi_list_lock, flags);
	for_each_possible_cpu(cpu) {

		if (!dev_priv->cpu_vp[cpu])
			continue;

		if (!cpumask_test_cpu(cpu, &dev_priv->rx_cpus))
			continue;

		cpu_vp = dev_priv->cpu_vp[cpu];

		/* find free queues list for changes */
		array_ind = mv_pp3_free_array_ind_get(&cpu_vp->port.cpu);
		if (array_ind < 0)
			continue;

		/* build new list of valid napi queues */
		for (i = 0; i < cpu_vp->rx_vqs_num; i++)
			if (cpu_vp->rx_vqs[i]->valid)
				cpu_vp->port.cpu.napi_proc_qs[array_ind][i] = cpu_vp->rx_vqs[i]->vq;
		mv_pp3_vq_prio_sort(cpu_vp->rx_vqs, cpu_vp->port.cpu.napi_proc_qs[array_ind], cpu_vp->rx_vqs_num);
		cpu_vp->port.cpu.napi_next_array = array_ind;
	}
	MV_UNLOCK(&napi_list_lock, flags);
}

/* disable RX queue for napi processing */
static void mv_pp3_dev_vq_napi_disable(struct pp3_vport *cpu_vp, int vq)
{
	struct pp3_cpu_port *cpu = &cpu_vp->port.cpu;
	int array_ind;
	int i, j;

	/* find free queues list for changes */
	array_ind = mv_pp3_free_array_ind_get(cpu);
	if (array_ind < 0)
		return;
	mv_pp3_napi_array_sync(cpu, cpu->napi_next_array, array_ind, cpu->napi_q_num);

	/* to disable, remove queue from the list of napi queues */
	for (i = 0; i < cpu->napi_q_num; i++) {
		if (vq == cpu->napi_proc_qs[array_ind][i]) {
			if ((i + 1) < cpu->napi_q_num) {
				for (j = i; (j + 1) < cpu->napi_q_num; j++)
					cpu->napi_proc_qs[array_ind][j] = cpu->napi_proc_qs[array_ind][j + 1];
			}
			cpu->napi_next_array = array_ind;
			break;
		}
	}
	cpu->napi_q_num--;
	cpu_vp->rx_vqs[vq]->valid = false;
}

/* enable RX queue for napi processing */
static void mv_pp3_dev_vq_napi_enable(struct pp3_vport *cpu_vp, int vq)
{
	struct pp3_cpu_port *cpu = &cpu_vp->port.cpu;
	int array_ind;

	/* find free queues list for changes */
	array_ind = mv_pp3_free_array_ind_get(cpu);
	if (array_ind < 0)
		return;
	mv_pp3_napi_array_sync(cpu, cpu->napi_next_array, array_ind, cpu->napi_q_num);

	/* to enable, add queue to the list of napi queues according to its priority */
	cpu->napi_proc_qs[array_ind][cpu->napi_q_num] = vq;
	cpu->napi_q_num++;
	cpu_vp->rx_vqs[vq]->valid = true;
	mv_pp3_vq_prio_sort(cpu_vp->rx_vqs, cpu->napi_proc_qs[array_ind], cpu->napi_q_num);
	cpu->napi_next_array = array_ind;
}

/* disable / enable RX queue for napi processing */
/* this function can be called only in NAPI context runs on specific CPU */
int mv_pp3_dev_vqs_proc_cfg(struct net_device *dev, int vq, bool q_enable)
{
	struct pp3_vport *cpu_vp;
	unsigned long flags = 0;
	struct pp3_dev_priv *dev_priv = MV_PP3_PRIV(dev);
	int cpu;

	if (!dev_priv) {
		pr_err("%s: Error - %s is not initialized\n", __func__, dev->name);
		goto err;
	}

	MV_LOCK(&napi_list_lock, flags);
	for_each_possible_cpu(cpu) {

		if (!dev_priv->cpu_vp[cpu])
			continue;

		if (!cpumask_test_cpu(cpu, &dev_priv->rx_cpus))
			continue;

		cpu_vp = dev_priv->cpu_vp[cpu];

		if (mv_pp3_max_check(vq, cpu_vp->rx_vqs_num, "vq"))
			break;

		if (q_enable) {
			if (cpu_vp->rx_vqs[vq]->valid)
				continue;
			mv_pp3_dev_vq_napi_enable(cpu_vp, vq);
			mv_pp3_hmac_rxq_resume(cpu_vp->rx_vqs[vq]->swq->frame_num, cpu_vp->rx_vqs[vq]->swq->swq);
			STAT_INFO(cpu_vp->rx_vqs[vq]->swq->stats.resumed++);
		} else {
			if (!cpu_vp->rx_vqs[vq]->valid)
				continue;
			mv_pp3_hmac_rxq_pause(cpu_vp->rx_vqs[vq]->swq->frame_num, cpu_vp->rx_vqs[vq]->swq->swq);
			mv_pp3_dev_vq_napi_disable(cpu_vp, vq);
			STAT_INFO(cpu_vp->rx_vqs[vq]->swq->stats.suspend++);
		}
	}
	MV_UNLOCK(&napi_list_lock, flags);

	return 0;

err:
	pr_err("%s: function failed\n", __func__);
	return -1;
}
