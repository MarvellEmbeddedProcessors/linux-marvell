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

struct pan_host_mbox {
	struct mbox		mbox_host;
	struct workqueue_struct	*mbox_host_wq;
};

struct pan_host_mbox ph_mbox;

static irqreturn_t pan_mbox_host_intr_handler(int irq, void *pf_irq)
{
	struct otx2_nic *pf = (struct otx2_nic *)pf_irq;
	struct mbox *mw = &ph_mbox.mbox_host;
	struct otx2_mbox_dev *mdev;
	struct otx2_mbox *mbox;
	struct mbox_hdr *hdr;

	/* Clear the IRQ */
	otx2_write64(pf, RVU_PF_VFPF_MBOX_INTX(1), BIT_ULL(63) | BIT_ULL(62));

	mbox = &mw->mbox;
	mdev = &mbox->dev[0];
	otx2_sync_mbox_bbuf(mbox, 0);

	/*TODO: handle mbox_up */
	hdr = (struct mbox_hdr *)(mdev->mbase + mbox->rx_start);
	if (hdr->num_msgs)
		queue_work(ph_mbox.mbox_host_wq, &mw->mbox_wrk);

	return IRQ_HANDLED;
}

void pan_mbox_disable_host_intr(struct otx2_nic *pf)
{
	int vector = pci_irq_vector(pf->pdev, RVU_PF_INT_VEC_VFPF_MBOX1);

	/* Disable Host => PAN mailbox IRQ */
	otx2_write64(pf, RVU_PF_INT_ENA_W1C, BIT_ULL(0));
	free_irq(vector, pf);
}

int pan_mbox_register_host_intr(struct otx2_nic *pf, bool probe_af)
{
	struct otx2_hw *hw = &pf->hw;
	char *irq_name;
	int err;

	/* Register mailbox interrupt handler */
	irq_name = &hw->irq_name[RVU_PF_INT_VEC_AFPF_MBOX * NAME_SIZE];
	snprintf(irq_name, NAME_SIZE, "Host_Pan_Mbox_Intr");
	err = request_irq(pci_irq_vector(pf->pdev, RVU_PF_INT_VEC_VFPF_MBOX1),
			  pan_mbox_host_intr_handler, 0, irq_name, pf);
	if (err) {
		dev_err(pf->dev,
			"RVUPF: IRQ registration failed for Host mbox irq\n");
		return err;
	}

	/* Enable mailbox interrupt for msgs coming from AF.
	 * First clear to avoid spurious interrupts, if any.
	 */
	otx2_write64(pf, RVU_PF_VFPF_MBOX_INTX(1), BIT_ULL(63));
	otx2_write64(pf, RVU_PF_VFPF_MBOX_INT_ENA_W1SX(1), BIT_ULL(63));
	return 0;
}

void pan_mbox_host_destroy(void)
{
	struct mbox *mbox = &ph_mbox.mbox_host;

	if (ph_mbox.mbox_host_wq) {
		destroy_workqueue(ph_mbox.mbox_host_wq);
		ph_mbox.mbox_host_wq = NULL;
	}

	if (mbox->mbox.hwbase)
		iounmap((void __iomem *)mbox->mbox.hwbase);

	otx2_mbox_destroy(&mbox->mbox);
	otx2_mbox_destroy(&mbox->mbox_up);
}

static int pan_mbox_process_host_msg(struct otx2_nic *pf,
				     struct otx2_mbox *mbox,
				     struct mbox_msghdr *msg)
{
	if (msg->id >= MBOX_MSG_MAX) {
		dev_err(pf->dev,
			"Mbox msg with unknown ID 0x%x\n", msg->id);
		return -EINVAL;
	}

	/* Check if valid, if not reply with an invalid msg */
	if (msg->sig != OTX2_MBOX_REQ_SIG) {
		dev_err(pf->dev,
			"Signature mismatch, msg->sig=0x%x\n", msg->sig);
		return -EINVAL;
	}

	switch (msg->id) {
#if IS_ENABLED(CONFIG_OCTEONTX_PAN_KTLS_TX)
	case PAN_KTLS_ADD_CONN:
	case PAN_KTLS_DEL_CONN:
		pan_ktls_process_mbox(pf, msg);
		break;
#endif
	default:
		dev_err(pf->dev,
			"Unhandled msg id = %d\n", msg->id);
		break;
	}

	return 0;
}

static void pan_mbox_host_handler(struct work_struct *work)
{
	struct otx2_mbox_dev *mdev;
	struct mbox_hdr *rsp_hdr;
	struct mbox_msghdr *msg;
	struct otx2_mbox *mbox;
	struct mbox *pan_mbox;
	struct otx2_nic *pf;
	int offset, id;
	u16 num_msgs;

	pan_mbox = container_of(work, struct mbox, mbox_wrk);
	mbox = &pan_mbox->mbox;
	mdev = &mbox->dev[0];
	rsp_hdr = (struct mbox_hdr *)(mdev->mbase + mbox->rx_start);
	num_msgs = rsp_hdr->num_msgs;

	offset = mbox->rx_start + ALIGN(sizeof(*rsp_hdr), MBOX_MSG_ALIGN);
	pf = pan_mbox->pfvf;

	for (id = 0; id < num_msgs; id++) {
		msg = (struct mbox_msghdr *)(mdev->mbase + offset);
		pan_mbox_process_host_msg(pf, mbox, msg);
		offset = mbox->rx_start + msg->next_msgoff;
		if (mdev->msgs_acked == (num_msgs - 1))
			__otx2_mbox_reset(mbox, 0);
		mdev->msgs_acked++;
	}

	/* Send Ack */
	otx2_write64(pf, RVU_PF_VFPF_MBOX_INT_W1SX(1), BIT_ULL(62));
}

int pan_mbox_host_init(struct otx2_nic *pf)
{
	struct mbox *mbox = &ph_mbox.mbox_host;
	void __iomem *hwbase;
	u8 *mem;
	int err;

	mbox->pfvf = pf;
	ph_mbox.mbox_host_wq = alloc_ordered_workqueue("otx2_host_mailbox",
						       WQ_HIGHPRI | WQ_MEM_RECLAIM);
	if (!ph_mbox.mbox_host_wq)
		return -ENOMEM;

	/* Mailbox is a reserved memory (in RAM) region shared between
	 * admin function (i.e AF) and this PF, shouldn't be mapped as
	 * device memory to allow unaligned accesses.
	 */
	hwbase = ioremap_wc(pci_resource_start(pf->pdev, PCI_MBOX_BAR_NUM),
			    512 * 1024);
	if (!hwbase) {
		dev_err(pf->dev, "Unable to map PFAF mailbox region\n");
		err = -ENOMEM;
		goto exit;
	}

	/* PAN PF mbox size = 64K, after that LMTST region */
	mem = (u8 *)hwbase;
	mem += 0x10000;
	hwbase = (void __iomem *)mem;

	err = otx2_mbox_init(&mbox->mbox, hwbase, pf->pdev, pf->reg_base,
			     MBOX_DIR_VFPF, 1);
	if (err)
		goto exit;

	err = otx2_mbox_init(&mbox->mbox_up, hwbase, pf->pdev, pf->reg_base,
			     MBOX_DIR_VFPF_UP, 1);
	if (err)
		goto exit;

	err = otx2_mbox_bbuf_init(mbox, pf->pdev);
	if (err)
		goto exit;

	INIT_WORK(&mbox->mbox_wrk, pan_mbox_host_handler);
	mutex_init(&mbox->lock);
	return 0;

exit:
	pan_mbox_host_destroy();
	return err;
}
