// SPDX-License-Identifier: GPL-2.0
/* Marvell RVU Ethernet driver
 *
 * Copyright (C) 2024 Marvell.
 *
 */

#include "otx2_common.h"
#include "otx2_reg.h"
#include "otx2_struct.h"
#include "cn10k.h"

static struct dev_hw_ops cn20k_hw_ops = {
	.pfaf_mbox_intr_handler = cn20k_pfaf_mbox_intr_handler,
	.vfaf_mbox_intr_handler = cn20k_vfaf_mbox_intr_handler,
	.pfvf_mbox_intr_handler = cn20k_pfvf_mbox_intr_handler,
};

void cn20k_init(struct otx2_nic *pfvf)
{
	pfvf->hw_ops = &cn20k_hw_ops;
}
EXPORT_SYMBOL(cn20k_init);
/* CN20K mbox AF => PFx irq handler */
irqreturn_t cn20k_pfaf_mbox_intr_handler(int irq, void *pf_irq)
{
	struct otx2_nic *pf = pf_irq;
	struct mbox *mw = &pf->mbox;
	struct otx2_mbox_dev *mdev;
	struct otx2_mbox *mbox;
	struct mbox_hdr *hdr;
	int pf_trig_val;

	pf_trig_val = otx2_read64(pf, RVU_PF_INT) & 0x3;

	/* Clear the IRQ */
	otx2_write64(pf, RVU_PF_INT, pf_trig_val);

	if (pf_trig_val & BIT_ULL(0)) {
		mbox = &mw->mbox_up;
		mdev = &mbox->dev[0];
		otx2_sync_mbox_bbuf(mbox, 0);

		hdr = (struct mbox_hdr *)(mdev->mbase + mbox->rx_start);
		if (hdr->num_msgs)
			queue_work(pf->mbox_wq, &mw->mbox_up_wrk);

		trace_otx2_msg_interrupt(pf->pdev, "UP message from AF to PF",
					 BIT_ULL(0));

		trace_otx2_msg_status(pf->pdev, "PF-AF up work queued(int)",
				      hdr->num_msgs);
	}

	if (pf_trig_val & BIT_ULL(1)) {
		mbox = &mw->mbox;
		mdev = &mbox->dev[0];
		otx2_sync_mbox_bbuf(mbox, 0);

		hdr = (struct mbox_hdr *)(mdev->mbase + mbox->rx_start);
		if (hdr->num_msgs)
			queue_work(pf->mbox_wq, &mw->mbox_wrk);
		trace_otx2_msg_interrupt(pf->pdev, "DOWN reply from AF to PF",
					 BIT_ULL(1));

		trace_otx2_msg_status(pf->pdev, "PF-AF down work queued(int)",
				      hdr->num_msgs);
	}

	return IRQ_HANDLED;
}

irqreturn_t cn20k_vfaf_mbox_intr_handler(int irq, void *vf_irq)
{
	struct otx2_nic *vf = vf_irq;
	struct otx2_mbox_dev *mdev;
	struct otx2_mbox *mbox;
	struct mbox_hdr *hdr;
	int vf_trig_val;

	vf_trig_val = otx2_read64(vf, RVU_VF_INT) & 0x3;
	/* Clear the IRQ */
	otx2_write64(vf, RVU_VF_INT, vf_trig_val);

	/* Read latest mbox data */
	smp_rmb();

	if (vf_trig_val & BIT_ULL(1)) {
		/* Check for PF => VF response messages */
		mbox = &vf->mbox.mbox;
		mdev = &mbox->dev[0];
		otx2_sync_mbox_bbuf(mbox, 0);

		hdr = (struct mbox_hdr *)(mdev->mbase + mbox->rx_start);
		if (hdr->num_msgs)
			queue_work(vf->mbox_wq, &vf->mbox.mbox_wrk);

		trace_otx2_msg_interrupt(mbox->pdev, "DOWN reply from PF0 to VF",
					 BIT_ULL(1));
	}

	if (vf_trig_val & BIT_ULL(0)) {
		/* Check for PF => VF notification messages */
		mbox = &vf->mbox.mbox_up;
		mdev = &mbox->dev[0];
		otx2_sync_mbox_bbuf(mbox, 0);

		hdr = (struct mbox_hdr *)(mdev->mbase + mbox->rx_start);
		if (hdr->num_msgs)
			queue_work(vf->mbox_wq, &vf->mbox.mbox_up_wrk);

		trace_otx2_msg_interrupt(mbox->pdev, "UP message from PF0 to VF",
					 BIT_ULL(0));
	}

	return IRQ_HANDLED;
}

void cn20k_enable_pfvf_mbox_intr(struct otx2_nic *pf, int numvfs)
{
	/* Clear PF <=> VF mailbox IRQ */
	otx2_write64(pf, RVU_MBOX_PF_VFPF_INTX(0), ~0ull);
	otx2_write64(pf, RVU_MBOX_PF_VFPF_INTX(1), ~0ull);
	otx2_write64(pf, RVU_MBOX_PF_VFPF1_INTX(0), ~0ull);
	otx2_write64(pf, RVU_MBOX_PF_VFPF1_INTX(1), ~0ull);

	/* Enable PF <=> VF mailbox IRQ */
	otx2_write64(pf, RVU_MBOX_PF_VFPF_INT_ENA_W1SX(0), INTR_MASK(numvfs));
	otx2_write64(pf, RVU_MBOX_PF_VFPF1_INT_ENA_W1SX(0), INTR_MASK(numvfs));
	if (numvfs > 64) {
		numvfs -= 64;
		otx2_write64(pf, RVU_MBOX_PF_VFPF_INT_ENA_W1SX(1),
			     INTR_MASK(numvfs));
		otx2_write64(pf, RVU_MBOX_PF_VFPF1_INT_ENA_W1SX(1),
			     INTR_MASK(numvfs));
	}
}

void cn20k_disable_pfvf_mbox_intr(struct otx2_nic *pf, int numvfs)
{
	int vector, intr_vec, vec = 0;

	/* Disable PF <=> VF mailbox IRQ */
	otx2_write64(pf, RVU_MBOX_PF_VFPF_INT_ENA_W1CX(0), ~0ull);
	otx2_write64(pf, RVU_MBOX_PF_VFPF_INT_ENA_W1CX(1), ~0ull);
	otx2_write64(pf, RVU_MBOX_PF_VFPF1_INT_ENA_W1CX(0), ~0ull);
	otx2_write64(pf, RVU_MBOX_PF_VFPF1_INT_ENA_W1CX(1), ~0ull);

	otx2_write64(pf, RVU_MBOX_PF_VFPF_INTX(0), ~0ull);
	otx2_write64(pf, RVU_MBOX_PF_VFPF1_INTX(0), ~0ull);

	if (numvfs > 64) {
		otx2_write64(pf, RVU_MBOX_PF_VFPF_INTX(1), ~0ull);
		otx2_write64(pf, RVU_MBOX_PF_VFPF1_INTX(1), ~0ull);
	}

	for (intr_vec = RVU_MBOX_PF_INT_VEC_VFPF_MBOX0; intr_vec <=
			RVU_MBOX_PF_INT_VEC_VFPF1_MBOX1; intr_vec++, vec++) {
		vector = pci_irq_vector(pf->pdev, intr_vec);
		free_irq(vector, pf->hw.pfvf_irq_devid[vec]);
	}
}

irqreturn_t cn20k_pfvf_mbox_intr_handler(int irq, void *pf_irq)
{
	struct pf_irq_data *irq_data = pf_irq;
	struct otx2_nic *pf = irq_data->pf;
	struct mbox *mbox;
	u64 intr;

	/* Sync with mbox memory region */
	rmb();

	/* Clear interrupts */
	intr = otx2_read64(pf, irq_data->intr_status);
	otx2_write64(pf, irq_data->intr_status, intr);
	mbox = pf->mbox_pfvf;

	if (intr)
		trace_otx2_msg_interrupt(pf->pdev, "VF(s) to PF", intr);

	irq_data->pf_queue_work_hdlr(mbox, pf->mbox_pfvf_wq, irq_data->start,
				     irq_data->mdevs, intr);

	return IRQ_HANDLED;
}

int cn20k_register_pfvf_mbox_intr(struct otx2_nic *pf, int numvfs)
{
	struct otx2_hw *hw = &pf->hw;
	struct pf_irq_data *irq_data;
	int intr_vec, ret, vec = 0;
	char *irq_name;

	/* irq data for 4 PF intr vectors */
	irq_data = devm_kcalloc(pf->dev, 4,
				sizeof(struct pf_irq_data), GFP_KERNEL);
	if (!irq_data)
		return -ENOMEM;

	for (intr_vec = RVU_MBOX_PF_INT_VEC_VFPF_MBOX0; intr_vec <=
			RVU_MBOX_PF_INT_VEC_VFPF1_MBOX1; intr_vec++, vec++) {
		switch (intr_vec) {
		case RVU_MBOX_PF_INT_VEC_VFPF_MBOX0:
			irq_data[vec].intr_status =
						RVU_MBOX_PF_VFPF_INTX(0);
			irq_data[vec].start = 0;
			irq_data[vec].mdevs = 64;
			break;
		case RVU_MBOX_PF_INT_VEC_VFPF_MBOX1:
			irq_data[vec].intr_status =
						RVU_MBOX_PF_VFPF_INTX(1);
			irq_data[vec].start = 64;
			irq_data[vec].mdevs = 96;
			break;
		case RVU_MBOX_PF_INT_VEC_VFPF1_MBOX0:
			irq_data[vec].intr_status =
						RVU_MBOX_PF_VFPF1_INTX(0);
			irq_data[vec].start = 0;
			irq_data[vec].mdevs = 64;
			break;
		case RVU_MBOX_PF_INT_VEC_VFPF1_MBOX1:
			irq_data[vec].intr_status =
						RVU_MBOX_PF_VFPF1_INTX(1);
			irq_data[vec].start = 64;
			irq_data[vec].mdevs = 96;
			break;
		}
		irq_data[vec].pf_queue_work_hdlr = otx2_queue_vf_work;
		irq_data[vec].vec_num = intr_vec;
		irq_data[vec].pf = pf;

		/* Register mailbox interrupt handler */
		irq_name = &hw->irq_name[intr_vec * NAME_SIZE];
		if (pf->pcifunc)
			snprintf(irq_name, NAME_SIZE,
				 "RVUPF%d_VF%d Mbox%d", rvu_get_pf(pf->pcifunc),
				 vec / 2, vec % 2);
		else
			snprintf(irq_name, NAME_SIZE, "RVUPF_VF%d Mbox%d",
				 vec / 2, vec % 2);

		hw->pfvf_irq_devid[vec] = &irq_data[vec];
		ret = request_irq(pci_irq_vector(pf->pdev, intr_vec),
				  pf->hw_ops->pfvf_mbox_intr_handler, 0,
				  irq_name,
				  &irq_data[vec]);
		if (ret) {
			dev_err(pf->dev,
				"RVUPF: IRQ registration failed for PFVF mbox0 irq\n");
			return ret;
		}
	}

	cn20k_enable_pfvf_mbox_intr(pf, numvfs);

	return 0;
}

static int cn20k_tc_get_entry_index(struct otx2_flow_config *flow_cfg,
				    struct otx2_tc_flow *node)
{
	struct otx2_tc_flow *tmp;
	int index = 0;

	list_for_each_entry(tmp, &flow_cfg->flow_list_tc, list) {
		if (tmp == node)
			return index;

		index++;
	}

	return 0;
}

static int cn20k_tc_free_mcam_entry(struct otx2_nic *nic, u16 entry)
{
	struct npc_mcam_free_entry_req *req;
	int err;

	mutex_lock(&nic->mbox.lock);
	req = otx2_mbox_alloc_msg_npc_mcam_free_entry(&nic->mbox);
	if (!req) {
		mutex_unlock(&nic->mbox.lock);
		return -ENOMEM;
	}

	req->entry = entry;
	/* Send message to AF to free MCAM entries */
	err = otx2_sync_mbox_msg(&nic->mbox);
	if (err) {
		mutex_unlock(&nic->mbox.lock);
		return err;
	}

	mutex_unlock(&nic->mbox.lock);

	return 0;
}

static bool cn20k_tc_check_entry_shiftable(struct otx2_nic *nic,
					   struct otx2_flow_config *flow_cfg,
					   struct otx2_tc_flow *node, int index,
					   bool error)
{
	struct otx2_tc_flow *first, *tmp, *n;
	int i = 0;
	u32 prio;
	u8 type;

	first = list_first_entry(&flow_cfg->flow_list_tc, struct otx2_tc_flow,
				 list);
	type = first->kw_type;

	/* Check all the nodes from start to given index (including index) has
	 * same type i.e, either X2 or X4
	 */
	list_for_each_entry_safe(tmp, n, &flow_cfg->flow_list_tc, list) {
		if (i > index)
			break;

		if (type != tmp->kw_type) {
			/* List has both X2 and X4 entries so entries cannot be
			 * shifted to save MCAM space.
			 */
			if (error)
				dev_err(nic->dev, "Rule %d cannot be shifted to %d\n",
					tmp->prio, prio);
			return false;
		}

		type = tmp->kw_type;
		prio = tmp->prio;
		i++;
	}

	return true;
}

void cn20k_tc_update_mcam_table_del_req(struct otx2_nic *nic,
					struct otx2_flow_config *flow_cfg,
					struct otx2_tc_flow *node)
{
	struct otx2_tc_flow *first, *tmp, *n;
	int i = 0, index;
	u16 cntr_val = 0;
	u16 entry;

	index = cn20k_tc_get_entry_index(flow_cfg, node);
	first = list_first_entry(&flow_cfg->flow_list_tc, struct otx2_tc_flow,
				 list);
	entry = first->entry;

	/* If entries cannot be shifted then delete given entry
	 * and free it to AF too.
	 */
	if (!cn20k_tc_check_entry_shiftable(nic, flow_cfg, node, index, false)) {
		list_del(&node->list);
		entry = node->entry;
		goto free_mcam_entry;
	}

	/* Find and delete the entry from the list and re-install
	 * all the entries from beginning to the index of the
	 * deleted entry to higher mcam indexes.
	 */
	list_for_each_entry_safe(tmp, n, &flow_cfg->flow_list_tc, list) {
		if (node == tmp) {
			list_del(&tmp->list);
			break;
		}

		otx2_del_mcam_flow_entry(nic, tmp->entry, &cntr_val);
		tmp->entry = (list_next_entry(tmp, list))->entry;
		tmp->req.entry = tmp->entry;
		tmp->req.cntr_val = cntr_val;
	}

	list_for_each_entry_safe(tmp, n, &flow_cfg->flow_list_tc, list) {
		if (i == index)
			break;

		otx2_add_mcam_flow_entry(nic, &tmp->req);
		i++;
	}

free_mcam_entry:
	if (cn20k_tc_free_mcam_entry(nic, entry))
		netdev_err(nic->netdev, "Freeing entry %d to AF failed\n",
			   first->entry);
}

int cn20k_tc_update_mcam_table_add_req(struct otx2_nic *nic,
				       struct otx2_flow_config *flow_cfg,
				       struct otx2_tc_flow *node)
{
	struct otx2_tc_flow *tmp;
	u16 cntr_val = 0;
	int list_idx, i;
	int entry, prev;

	/* Find the index of the entry(list_idx) whose priority
	 * is greater than the new entry and re-install all
	 * the entries from beginning to list_idx to higher
	 * mcam indexes.
	 */
	list_idx = otx2_tc_add_to_flow_list(flow_cfg, node);
	entry = node->entry;
	if (!cn20k_tc_check_entry_shiftable(nic, flow_cfg, node,
					    list_idx, true)) {
		/* Due to mix of X2 and X4, entries cannot be shifted.
		 * In this case free the entry allocated for this rule.
		 */
		if (cn20k_tc_free_mcam_entry(nic, entry))
			netdev_err(nic->netdev,
				   "Freeing entry %d to AF failed\n", entry);
		return -EINVAL;
	}

	for (i = 0; i < list_idx; i++) {
		tmp = otx2_tc_get_entry_by_index(flow_cfg, i);
		if (!tmp)
			return -ENOMEM;

		otx2_del_mcam_flow_entry(nic, tmp->entry, &cntr_val);
		prev = tmp->entry;
		tmp->entry = entry;
		tmp->req.entry = tmp->entry;
		tmp->req.cntr_val = cntr_val;
		otx2_add_mcam_flow_entry(nic, &tmp->req);
		entry = prev;
	}

	return entry;
}

#define MAX_TC_HW_PRIORITY		125
#define MAX_TC_VF_PRIORITY		126
#define MAX_TC_PF_PRIORITY		127

static int __cn20k_tc_alloc_entry(struct otx2_nic *nic,
				  struct npc_install_flow_req *flow_req,
				  u16 *entry, u8 *type,
				  u32 tc_priority, bool hw_priority)
{
	struct otx2_flow_config *flow_cfg = nic->flow_cfg;
	struct npc_install_flow_req *req;
	struct npc_install_flow_rsp *rsp;
	struct otx2_tc_flow *tmp;
	int ret = 0;

	req = otx2_mbox_alloc_msg_npc_install_flow(&nic->mbox);
	if (!req)
		return -ENOMEM;

	memcpy(&flow_req->hdr, &req->hdr, sizeof(struct mbox_msghdr));
	memcpy(req, flow_req, sizeof(struct npc_install_flow_req));
	req->alloc_entry = 1;

	/* Allocate very least priority for first rule */
	if (hw_priority || list_empty(&flow_cfg->flow_list_tc)) {
		req->ref_prio = NPC_MCAM_LEAST_PRIO;
	} else {
		req->ref_prio = NPC_MCAM_HIGHER_PRIO;
		tmp = list_first_entry(&flow_cfg->flow_list_tc,
				       struct otx2_tc_flow, list);
		req->ref_entry = tmp->entry;
	}

	ret = otx2_sync_mbox_msg(&nic->mbox);
	if (ret)
		return ret;

	rsp = (struct npc_install_flow_rsp *)otx2_mbox_get_rsp(&nic->mbox.mbox,
							       0, &req->hdr);

	if (entry)
		*entry = rsp->entry;
	if (type)
		*type = rsp->kw_type;

	return ret;
}

int cn20k_tc_alloc_entry(struct otx2_nic *nic,
			 struct flow_cls_offload *tc_flow_cmd,
			 struct otx2_tc_flow *new_node,
			 struct npc_install_flow_req *flow_req)
{
	bool hw_priority = false;
	u16 entry_from_af;
	u8 entry_type;
	int ret;

	if (is_otx2_vf(nic->pcifunc))
		flow_req->hw_prio = MAX_TC_VF_PRIORITY;
	else
		flow_req->hw_prio = MAX_TC_PF_PRIORITY;

	if (new_node->prio <= MAX_TC_HW_PRIORITY) {
		flow_req->hw_prio = new_node->prio;
		hw_priority = true;
	}

	mutex_lock(&nic->mbox.lock);

	ret = __cn20k_tc_alloc_entry(nic, flow_req, &entry_from_af, &entry_type,
				     new_node->prio, hw_priority);
	if (ret) {
		mutex_unlock(&nic->mbox.lock);
		return ret;
	}

	new_node->kw_type = entry_type;
	new_node->entry = entry_from_af;

	mutex_unlock(&nic->mbox.lock);

	return 0;
}
