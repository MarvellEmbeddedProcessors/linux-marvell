/*******************************************************************************
Copyright (C) Marvell International Ltd. and its affiliates

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

#ifndef __mv_dev_vq_h__
#define __mv_dev_vq_h__

#include "common/mv_sw_if.h"
#include "platform/mv_pp3.h"
#include "vport/mv_pp3_vq.h"

/* Ingress VQs configuration APIs - for all interfaces */
int mv_pp3_dev_ingress_vqs_num_get(struct net_device *dev, int *vqs_num);
int mv_pp3_dev_ingress_vqs_defaults_set(struct net_device *dev);
int mv_pp3_dev_ingress_vq_drop_set(struct net_device *dev, int vq, struct mv_nss_drop *drop);
int mv_pp3_dev_ingress_vq_drop_get(struct net_device *dev, int vq, struct mv_nss_drop *drop);

int mv_pp3_dev_ingress_vq_prio_set(struct net_device *dev, int vq, u16 prio);
int mv_pp3_dev_ingress_vq_weight_set(struct net_device *dev, int vq, u16 weight);
int mv_pp3_dev_ingress_vq_sched_get(struct net_device *dev, int vq, struct mv_nss_sched *sched);

int mv_pp3_dev_ingress_cos_show(struct net_device *netdev);
int mv_pp3_dev_ingress_cos_to_vq_set(struct net_device *dev, int cos, int vq);
int mv_pp3_dev_ingress_cos_to_vq_get(struct net_device *dev, int cos, int *vq);

int mv_pp3_dev_ingress_vq_size_set(struct net_device *dev, int vq, u16 length);
int mv_pp3_dev_ingress_vq_size_get(struct net_device *dev, int vq, u16 *length);

/* Egress VQs configuration APIs - for all interfaces */
int mv_pp3_dev_egress_vqs_num_get(struct net_device *dev, int *vqs_num);
int mv_pp3_dev_egress_vqs_defaults_set(struct net_device *dev);
int mv_pp3_dev_egress_vq_drop_set(struct net_device *dev, int vq, struct mv_nss_drop *drop);
int mv_pp3_dev_egress_vq_drop_get(struct net_device *dev, int vq, struct mv_nss_drop *drop);

int mv_pp3_dev_egress_vq_prio_set(struct net_device *dev, int vq, u16 prio);
int mv_pp3_dev_egress_vq_weight_set(struct net_device *dev, int vq, u16 weight);
int mv_pp3_dev_egress_vq_sched_get(struct net_device *dev, int vq, struct mv_nss_sched *sched);

int mv_pp3_dev_egress_cos_show(struct net_device *netdev);
int mv_pp3_dev_egress_cos_to_vq_set(struct net_device *dev, int cos, int vq);
int mv_pp3_dev_egress_cos_to_vq_get(struct net_device *dev, int cos, int *vq);

int mv_pp3_dev_egress_vq_size_set(struct net_device *dev, int vq, u16 length);
int mv_pp3_dev_egress_vq_size_get(struct net_device *dev, int vq, u16 *length);

int mv_pp3_dev_egress_vq_rate_limit_set(struct net_device *dev, int vq, struct mv_nss_meter *meter);
int mv_pp3_dev_egress_vq_rate_limit_get(struct net_device *dev, int vq, struct mv_nss_meter *meter);

int mv_pp3_dev_egress_vport_shaper_set(struct net_device *dev, struct mv_nss_meter *meter);

void mv_pp3_dev_napi_queue_update(struct net_device *dev);
int mv_pp3_dev_vqs_proc_cfg(struct net_device *dev, int vq, bool q_enable);

#if 0
int mv_pp3_egress_vq_emac_set(struct net_device *dev, int vq, u32 emac);
int mv_pp3_egress_vq_bpi_thresh_set(struct net_device *dev, int vq, int xon, int xoff, enum mv_pp3_bpi_level level);
int mv_pp3_ingress_vq_bpi_thresh_set(int xon, int xoff);
int mv_pp3_egress_bpi_dump(struct net_device *dev);

#endif

#endif /* __mv_dev_vq_h__ */
