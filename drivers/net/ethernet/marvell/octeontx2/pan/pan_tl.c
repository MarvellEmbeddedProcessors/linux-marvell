// SPDX-License-Identifier: GPL-2.0
/* Marvell RVU PAN driver
 *
 * Copyright (C) 2025 Marvell.
 *
 */
#include <linux/types.h>
#include <linux/string.h>
#include <linux/errno.h>
#include <linux/stddef.h>
#include <linux/seq_file.h>
#include <linux/debugfs.h>
#include <linux/slab.h>

#include "pan_cmn.h"

static int pan_tl_txsch_alloc(struct otx2_nic *pfvf)
{
	struct pan_rvu_gbl_t *gbl;
	struct nix_txsch_alloc_req *req;
	struct nix_txsch_alloc_rsp *rsp;
	int lvl, schq, rc;

	/* Get memory to put this msg */
	req = otx2_mbox_alloc_msg_nix_txsch_alloc(&pfvf->mbox);
	if (!req)
		return -ENOMEM;

	gbl = pan_rvu_get_gbl();

	req->flags = NIX_TXSCH_ALLOC_FLAG_PAN;

	/* Request one schq per level */
	for (lvl = 0; lvl < NIX_TXSCH_LVL_CNT; lvl++) {
		req->schq[lvl] = 1;

		/* TODO: Handler multiple SDP channel case */
		if (gbl->sdp_cnt)
			req->schq[lvl]++;
	}

	rc = otx2_sync_mbox_msg(&pfvf->mbox);
	if (rc)
		return rc;

	rsp = (struct nix_txsch_alloc_rsp *)
	      otx2_mbox_get_rsp(&pfvf->mbox.mbox, 0, &req->hdr);
	if (IS_ERR(rsp))
		return PTR_ERR(rsp);

	/* Setup transmit scheduler list */
	for (lvl = 0; lvl < NIX_TXSCH_LVL_CNT; lvl++) {
		pfvf->hw.txschq_cnt[lvl] = rsp->schq[lvl];
		for (schq = 0; schq < rsp->schq[lvl]; schq++)
			pfvf->hw.txschq_list[lvl][schq] =
				rsp->schq_list[lvl][schq];
	}

	pfvf->hw.txschq_link_cfg_lvl = rsp->link_cfg_lvl;
	pfvf->hw.txschq_aggr_lvl_rr_prio = rsp->aggr_lvl_rr_prio;

	return 0;
}

static int pan_tl_txschq_cfg(struct otx2_nic *pfvf, int lvl, bool is_sdp)
{
	u16 (*schq_list)[MAX_TXSCHQ_PER_FUNC];
	struct otx2_hw *hw = &pfvf->hw;
	struct nix_txschq_config *req;
	u16 schq, parent;
	u64 dwrr_val;
	int q;

	q = is_sdp ? 1 : 0;
	dwrr_val = mtu_to_dwrr_weight(pfvf, pfvf->tx_max_pktlen);

	req = otx2_mbox_alloc_msg_nix_txschq_cfg(&pfvf->mbox);
	if (!req)
		return -ENOMEM;

	req->lvl = lvl;
	req->num_regs = 1;

	schq_list = hw->txschq_list;

	schq = schq_list[lvl][q];
	/* Set topology e.t.c configuration */
	if (lvl == NIX_TXSCH_LVL_SMQ) {
		req->reg[0] = NIX_AF_SMQX_CFG(schq);
		req->regval[0] = ((u64)pfvf->tx_max_pktlen << 8) | OTX2_MIN_MTU;
		req->regval[0] |= (0x20ULL << 51) | (0x80ULL << 39) |
				  (0x2ULL << 36);
		/* Set link type for DWRR MTU selection on CN10K silicons */
		if (!is_dev_otx2(pfvf->pdev))
			req->regval[0] |= FIELD_PREP(GENMASK_ULL(58, 57),
						(u64)hw->smq_link_type);
		req->num_regs++;
		/* MDQ config */
		parent = schq_list[NIX_TXSCH_LVL_TL4][q];
		req->reg[1] = NIX_AF_MDQX_PARENT(schq);
		req->regval[1] = parent << 16;
		req->num_regs++;
		/* Set DWRR quantum */
		req->reg[2] = NIX_AF_MDQX_SCHEDULE(schq);
		req->regval[2] =  dwrr_val;
	} else if (lvl == NIX_TXSCH_LVL_TL4) {
		parent = schq_list[NIX_TXSCH_LVL_TL3][q];
		req->reg[0] = NIX_AF_TL4X_PARENT(schq);
		req->regval[0] = (u64)parent << 16;
		req->num_regs++;
		req->reg[1] = NIX_AF_TL4X_SCHEDULE(schq);
		req->regval[1] = dwrr_val;
	} else if (lvl == NIX_TXSCH_LVL_TL3) {
		parent = schq_list[NIX_TXSCH_LVL_TL2][q];
		req->reg[0] = NIX_AF_TL3X_PARENT(schq);
		req->regval[0] = (u64)parent << 16;
		req->num_regs++;
		req->reg[1] = NIX_AF_TL3X_SCHEDULE(schq);
		req->regval[1] = dwrr_val;
	} else if (lvl == NIX_TXSCH_LVL_TL2) {
		parent = schq_list[NIX_TXSCH_LVL_TL1][q];
		req->reg[0] = NIX_AF_TL2X_PARENT(schq);
		req->regval[0] = (u64)parent << 16;

		req->num_regs++;
		req->reg[1] = NIX_AF_TL2X_SCHEDULE(schq);
		req->regval[1] = (u64)hw->txschq_aggr_lvl_rr_prio << 24 | dwrr_val;

	} else if (lvl == NIX_TXSCH_LVL_TL1) {
		/* Default config for TL1.
		 * For VF this is always ignored.
		 */

		/* On CN10K, if RR_WEIGHT is greater than 16384, HW will
		 * clip it to 16384, so configuring a 24bit max value
		 * will work on both OTx2 and CN10K.
		 */
		req->reg[0] = NIX_AF_TL1X_SCHEDULE(schq);
		req->regval[0] = TXSCH_TL1_DFLT_RR_QTM;

		req->num_regs++;
		req->reg[1] = NIX_AF_TL1X_TOPOLOGY(schq);
		req->regval[1] = hw->txschq_aggr_lvl_rr_prio << 1;

		req->num_regs++;
		req->reg[2] = NIX_AF_TL1X_CIR(schq);
		req->regval[2] = 0;
	}

	return otx2_sync_mbox_msg(&pfvf->mbox);
}

int pan_tl_txschq_rsrcs(struct otx2_nic *pf)
{
	int err, lvl;

	err = pan_tl_txsch_alloc(pf);
	if (err) {
		pr_err("Failed to allocate TXSCH\n");
		goto err;
	}

	for (lvl = 0; lvl < NIX_TXSCH_LVL_CNT; lvl++) {
		err = pan_tl_txschq_cfg(pf, lvl, false);
		if (err) {
			pr_err("Failed to config TXSCH\n");
			goto err;
		}

		if (pf->hw.txschq_cnt[lvl] == 1)
			continue;

		err = pan_tl_txschq_cfg(pf, lvl, true);
		if (err) {
			pr_err("Failed to config TXSCH\n");
			goto err;
		}
	}
err:
	return err;

};

void pan_tl_txschq_free_one(struct otx2_nic *pfvf, u16 lvl, u16 schq)
{
	struct nix_txsch_free_req *free_req;
	int err;

	mutex_lock(&pfvf->mbox.lock);

	free_req = otx2_mbox_alloc_msg_nix_txsch_free(&pfvf->mbox);
	if (!free_req) {
		mutex_unlock(&pfvf->mbox.lock);
		netdev_err(pfvf->netdev,
			   "Failed alloc txschq free req\n");
		return;
	}

	free_req->schq_lvl = lvl;
	free_req->schq = schq;

	if (lvl == NIX_TXSCH_LVL_TL1)
		free_req->flags = TXSCHQ_FREE_PAN_TL1;

	err = otx2_sync_mbox_msg(&pfvf->mbox);
	if (err) {
		netdev_err(pfvf->netdev,
			   "Failed stop txschq %d at level %d\n", schq, lvl);
	}

	mutex_unlock(&pfvf->mbox.lock);
}

int pan_tl_set_links(bool set_sdp)
{
	struct pan_rvu_dev_priv *priv;
	struct nix_txschq_config *req;
	struct otx2_nic *otx2_nic;
	struct net_device *dev;
	struct otx2_hw *hw;
	int reg_idx;
	int idx;
	int ret;

	dev = dev_get_by_name(&init_net, PAN_DEV_NAME);
	if (!dev) {
		pr_err("Could not find PAN device\n");
		return -EFAULT;
	}
	dev_put(dev);
	priv = netdev_priv(dev);
	otx2_nic = priv->otx2_nic;
	hw = &otx2_nic->hw;

	mutex_lock(&otx2_nic->mbox.lock);

	req = otx2_mbox_alloc_msg_nix_txschq_cfg(&otx2_nic->mbox);
	if (!req) {
		pr_err("Could not alloc req for mbox\n");
		return -ENOMEM;
	}

	for (reg_idx = 0; reg_idx < 13; reg_idx++) {
		idx = hw->txschq_list[NIX_TXSCH_LVL_TL3][0];
		req->reg[reg_idx] = NIX_AF_TL3_TL2X_LINKX_CFG(idx, reg_idx);
		req->regval[reg_idx] = BIT_ULL(12);
	}

	req->lvl = NIX_TXSCH_LVL_TL3;
	req->num_regs = reg_idx;

	ret = otx2_sync_mbox_msg(&otx2_nic->mbox);
	if (ret) {
		pr_err("Error in calling mbox\n");
		goto done;
	}

	if (!set_sdp)
		goto done;

	req = otx2_mbox_alloc_msg_nix_txschq_cfg(&otx2_nic->mbox);
	if (!req) {
		pr_err("Could not alloc req for mbox\n");
		return -ENOMEM;
	}

	req->lvl = NIX_TXSCH_LVL_TL4;
	req->num_regs = 1;

	idx = hw->txschq_list[NIX_TXSCH_LVL_TL4][1];
	req->reg[0] = NIX_AF_TL4X_SDP_LINK_CFG(idx);

	/* TODO: set relative channel for all SDP channels */
	req->regval[0] = BIT_ULL(12) | (0 & 0xff);

	ret = otx2_sync_mbox_msg(&otx2_nic->mbox);
	if (ret) {
		pr_err("Error in calling mbox\n");
		goto done;
	}

done:
	mutex_unlock(&otx2_nic->mbox.lock);
	return ret;
}

void pan_tl_deinit(void)
{
}

int pan_tl_init(void)
{
	return 0;
}
