// SPDX-License-Identifier: GPL-2.0
/* Marvell RVU Physical Function ethernet driver
 *
 * Copyright (C) 2025 Marvell.
 *
 */

#include <linux/module.h>
#include <linux/interrupt.h>
#include <linux/pci.h>
#include <linux/etherdevice.h>
#include <linux/of.h>
#include <linux/if_vlan.h>
#include <linux/iommu.h>
#include <net/ip.h>
#include <linux/bpf.h>
#include <linux/bpf_trace.h>
#include <linux/bitfield.h>

#include "otx2_reg.h"
#include "otx2_common.h"
#include "../../nic/otx2_txrx.h"
#include "otx2_struct.h"
#include "otx2_ptp.h"
#include "cn10k.h"
#include "qos.h"
#include <rvu_trace.h>
#include "pan_cmn.h"

void pan_cpt_inst_flush(struct otx2_nic *pf, struct cpt_inst_s *inst,
			u64 size, u64 io_addr)
{
	struct otx2_lmt_info *lmt_info;
	u64 val = 0, tar_addr = 0;

	lmt_info = per_cpu_ptr(pf->hw.lmt_info, smp_processor_id());
	/* FIXME: val[0:10] LMT_ID.
	 * [12:15] no of LMTST - 1 in the burst.
	 * [19:63] data size of each LMTST in the burst except first.
	 */
	val = (lmt_info->lmt_id & 0x7FF);
	/* Target address for LMTST flush tells HW how many 128bit
	 * words are present.
	 * tar_addr[6:4] size of first LMTST - 1 in units of 128b.
	 */
	tar_addr |= io_addr | (((size / 16) - 1) & 0x7) << 4;
	dma_wmb();
	memcpy((u64 *)lmt_info->lmt_addr, inst, size);
	cn10k_lmt_flush(val, tar_addr);
	dma_wmb();
}

int pan_cpt_wait_for_respose(struct cpt_res_s *res)
{
	unsigned long timeout = jiffies + msecs_to_jiffies(10000);

	do {
		if (time_after(jiffies, timeout)) {
			pr_err("CPT response timeout\n");
			return -EBUSY;
		}
	} while (res->compcode == 0);

	if (!(res->compcode == 1 || res->compcode == 6) || res->uc_compcode) {
		pr_info("compcode=%x\tdoneint=%x\n", res->compcode,
			res->doneint);
		pr_info("uc_compcode=%x\tuc_info=%llx\tesn=%llx\n",
			res->uc_compcode, (u64)res->uc_info, res->esn);
	}

	return 0;
}

int pan_cpt_tx_lf_alloc(struct otx2_nic *pf)
{
	struct cpt_lf_alloc_req_msg *req;
	int ret = 0;

	mutex_lock(&pf->mbox.lock);
	req = otx2_mbox_alloc_msg_cpt_lf_alloc(&pf->mbox);
	if (!req) {
		ret = -ENOMEM;
		goto error;
	}

	/* use self nix pf function */
	req->nix_pf_func = pf->pcifunc;
	/* Enable AE, SE and SE-IE Engine Groups */
	req->eng_grpmsk = 0x7;

	ret = otx2_sync_mbox_msg(&pf->mbox);

error:
	mutex_unlock(&pf->mbox.lock);
	return ret;
}

/* Allocate memory for CPT outbound Instruction queue.
 * Instruction queue memory format is:
 *      -----------------------------
 *     | Instruction Group memory    |
 *     |  (CPT_LF_Q_SIZE[SIZE_DIV40] |
 *     |   x 16 Bytes)               |
 *     |                             |
 *      ----------------------------- <-- CPT_LF_Q_BASE[ADDR]
 *     | Flow Control (128 Bytes)    |
 *     |                             |
 *      -----------------------------
 *     |  Instruction Memory         |
 *     |  (CPT_LF_Q_SIZE[SIZE_DIV40] |
 *     |   × 40 × 64 bytes)          |
 *     |                             |
 *      -----------------------------
 */
int pan_cpt_tx_iq_alloc(struct otx2_nic *pf, struct cn10k_cpt_inst_queue *iq)
{
	iq->size = CN10K_CPT_INST_QLEN_BYTES + CN10K_CPT_Q_FC_LEN +
		    CN10K_CPT_INST_GRP_QLEN_BYTES + OTX2_ALIGN;

	iq->real_vaddr = dma_alloc_coherent(pf->dev, iq->size,
					    &iq->real_dma_addr, GFP_KERNEL);
	if (!iq->real_vaddr)
		return -ENOMEM;

	/* iq->vaddr/dma_addr points to Flow Control location */
	iq->vaddr = iq->real_vaddr + CN10K_CPT_INST_GRP_QLEN_BYTES;
	iq->dma_addr = iq->real_dma_addr + CN10K_CPT_INST_GRP_QLEN_BYTES;

	/* Align pointers */
	iq->vaddr = PTR_ALIGN(iq->vaddr, OTX2_ALIGN);
	iq->dma_addr = PTR_ALIGN(iq->dma_addr, OTX2_ALIGN);
	return 0;
}

void pan_cpt_tx_iq_free(struct otx2_nic *pf,
			struct cn10k_cpt_inst_queue *iq)
{
	if (!iq->real_vaddr)
		dma_free_coherent(pf->dev, iq->size, iq->real_vaddr,
				  iq->real_dma_addr);

	iq->real_vaddr = NULL;
	iq->vaddr = NULL;
}

int pan_cpt_alloc(struct otx2_nic *pfvf, u16 qidx)
{
	struct otx2_qset *qset = &pfvf->qset;
	struct otx2_rcv_queue *rq;
	int err;

	rq = &qset->rq[qidx];
	/* TODO: revisit later */
	/* Allocate memory for NIX SQE and CPT SG. NIX and CPT SG are same
	 * in size. Allocate memory for CPT SG same as NIX SQE to keep size
	 * and base address aligned.
	 */
	err = qmem_alloc(pfvf->dev, &rq->cpt_resp, qset->rqe_cnt, 64);
	if (err)
		return err;

	return 0;
}

int pan_cpt_attach(struct otx2_nic *pfvf)
{
	struct rsrc_attach *attach;
	int err;

	/* ktls offload  supported on cn10k */
	if (is_dev_otx2(pfvf->pdev))
		return 0;

	mutex_lock(&pfvf->mbox.lock);
	/* Get memory to put this msg */
	attach = otx2_mbox_alloc_msg_attach_resources(&pfvf->mbox);
	if (!attach) {
		mutex_unlock(&pfvf->mbox.lock);
		return -ENOMEM;
	}

	attach->cptlfs = true;
	attach->modify = 1;

	/* Send attach request to AF */
	err = otx2_sync_mbox_msg(&pfvf->mbox);
	if (err) {
		mutex_unlock(&pfvf->mbox.lock);
		return err;
	}

	mutex_unlock(&pfvf->mbox.lock);
	return 0;
}
