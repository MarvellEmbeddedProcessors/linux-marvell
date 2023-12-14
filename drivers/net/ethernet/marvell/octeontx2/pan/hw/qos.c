// SPDX-License-Identifier: GPL-2.0
/* Marvell RVU Ethernet driver
 *
 * Copyright (C) 2025 Marvell.
 *
 */
#include <linux/netdevice.h>
#include <linux/etherdevice.h>
#include <linux/inetdevice.h>
#include <linux/bitfield.h>

#include "otx2_common.h"
#include "cn10k.h"
#include "qos.h"
#include "hw/otx2_cmn.h"

#define OTX2_QOS_QID_INNER		0xFFFFU
#define OTX2_QOS_QID_NONE		0xFFFEU
#define OTX2_QOS_ROOT_CLASSID		0xFFFFFFFF
#define OTX2_QOS_CLASS_NONE		0
#define OTX2_QOS_DEFAULT_PRIO		0xF
#define OTX2_QOS_INVALID_SQ		0xFFFF
#define OTX2_QOS_INVALID_TXSCHQ_IDX	0xFFFF
#define CN10K_MAX_RR_WEIGHT		GENMASK_ULL(13, 0)
#define OTX2_MAX_RR_QUANTUM		GENMASK_ULL(23, 0)

static struct otx2_qos_node *
otx2_sw_node_find(struct otx2_nic *pfvf, u32 classid)
{
	struct otx2_qos_node *node = NULL;

	hash_for_each_possible(pfvf->qos.qos_hlist, node, hlist, classid) {
		if (node->classid == classid)
			break;
	}

	return node;
}

static void otx2_qos_enadis_sq(struct otx2_nic *pfvf,
			       struct otx2_qos_node *node,
			       u16 qid)
{
	if (pfvf->qos.qid_to_sqmap[qid] != OTX2_QOS_INVALID_SQ)
		otx2_qos_disable_sq(pfvf, qid);

	pfvf->qos.qid_to_sqmap[qid] = node->schq;
	otx2_qos_enable_sq(pfvf, qid);
}

static void otx2_qos_update_smq_schq(struct otx2_nic *pfvf,
				     struct otx2_qos_node *node,
				     bool action)
{
	struct otx2_qos_node *tmp;

	if (node->qid == OTX2_QOS_QID_INNER)
		return;

	list_for_each_entry(tmp, &node->child_schq_list, list) {
		if (tmp->level == NIX_TXSCH_LVL_MDQ) {
			if (action == QOS_SMQ_FLUSH)
				otx2_smq_flush(pfvf, tmp->schq);
			else
				otx2_qos_enadis_sq(pfvf, tmp, node->qid);
		}
	}
}

static void __otx2_qos_update_smq(struct otx2_nic *pfvf,
				  struct otx2_qos_node *node,
				  bool action)
{
	struct otx2_qos_node *tmp;

	list_for_each_entry(tmp, &node->child_list, list) {
		__otx2_qos_update_smq(pfvf, tmp, action);
		if (tmp->qid == OTX2_QOS_QID_INNER)
			continue;
		if (tmp->level == NIX_TXSCH_LVL_MDQ) {
			if (action == QOS_SMQ_FLUSH)
				otx2_smq_flush(pfvf, tmp->schq);
			else
				otx2_qos_enadis_sq(pfvf, tmp, tmp->qid);
		} else {
			otx2_qos_update_smq_schq(pfvf, tmp, action);
		}
	}
}

static void otx2_qos_update_smq(struct otx2_nic *pfvf,
				struct otx2_qos_node *node,
				bool action)
{
	mutex_lock(&pfvf->qos.qos_lock);
	__otx2_qos_update_smq(pfvf, node, action);
	otx2_qos_update_smq_schq(pfvf, node, action);
	mutex_unlock(&pfvf->qos.qos_lock);
}

void dup_clean_qos_queues(struct otx2_nic *pfvf)
{
	struct otx2_qos_node *root;

	root = otx2_sw_node_find(pfvf, OTX2_QOS_ROOT_CLASSID);
	if (!root)
		return;

	otx2_qos_update_smq(pfvf, root, QOS_SMQ_FLUSH);
}
