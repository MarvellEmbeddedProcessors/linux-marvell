/* SPDX-License-Identifier: GPL-2.0 */
/* Marvell Octeon RVU Generic Physical Function driver
 *
 * Copyright (C) 2024 Marvell.
 */
#include <linux/device.h>
#include <linux/pci.h>
#include <rvu_trace.h>
#include "mbox.h"

#define RVU_PFFUNC(pf, func)    \
	((((pf) & RVU_PFVF_PF_MASK) << RVU_PFVF_PF_SHIFT) | \
	(((func) & RVU_PFVF_FUNC_MASK) << RVU_PFVF_FUNC_SHIFT))

#define NAME_SIZE		32
#define OTX2_GEN_PF_FUNC_BLKADDR_SHIFT         20
#define GEN_PF_CN20K_PFAF_MBOX_BASE       0x80000
#define GEN_PF_CN20K_PFVF_MBOX_IRQS		4
#define GEN_PF_CN20K_PFVF_MBOX_IRQ_NAME	       32

/* HW capability flags */
#define CN10K_MBOX  0

struct gen_pf_dev;

struct flr_work {
	struct work_struct work;
	struct gen_pf_dev *pfdev;
};

struct mbox {
	struct otx2_mbox	mbox;
	struct work_struct	mbox_wrk;
	struct otx2_mbox	mbox_up;
	struct work_struct	mbox_up_wrk;
	struct gen_pf_dev	*pfvf;
	void			*bbuf_base; /* Bounce buffer for mbox memory */
	struct mutex		lock;   /* serialize mailbox access */
	int			num_msgs; /* mbox number of messages */
	int			up_num_msgs; /* mbox_up number of messages */
};

struct gen_pf_irq_data {
	u64 intr_status;
	void (*pf_queue_work_hdlr)(struct mbox *mb, struct workqueue_struct *mw,
				   int first, int mdevs, u64 intr);
	struct gen_pf_dev *pf;
	char irq_name[GEN_PF_CN20K_PFVF_MBOX_IRQ_NAME];
	int vec_num;
	int start;
	int mdevs;
};

struct gen_pf_hw {
	struct gen_pf_irq_data	*pfvf_irq_devid[4];
	unsigned long		cap_flag;
};

struct gen_pf_vf_config {
	struct gen_pf_dev *pf;
#define GEN_PF_MAX_REQ_SIZE	256
	u8 cfg_buff[GEN_PF_MAX_REQ_SIZE];
	struct delayed_work vf_work;
};

struct gen_pf_dev {
	struct pci_dev		*pdev;
	struct device		*dev;
	void __iomem		*reg_base;
	char			*irq_name;
	struct workqueue_struct *flr_wq;
	struct flr_work		*flr_wrk;
	struct work_struct	mbox_wrk;
	struct work_struct	mbox_wrk_up;

	/* Mbox */
	struct mbox		mbox;
	struct mbox		*mbox_pfvf;
	struct workqueue_struct *mbox_wq;
	struct workqueue_struct *mbox_pfvf_wq;
	struct qmem		*pfvf_mbox_addr;
	/* CN20K PF<->VF mailbox IRQ vector data */
	struct gen_pf_irq_data	irq_data[GEN_PF_CN20K_PFVF_MBOX_IRQS];
	struct qmem		*mbox_qmem;
	struct gen_pf_hw	hw;
	int			pf;
	u16			pcifunc; /* RVU PF_FUNC */
	u8			total_vfs;
	struct gen_pf_vf_config	*vf_configs;
};

/* Mbox APIs */
static inline int rvu_gen_pf_sync_mbox_msg(struct mbox *mbox)
{
	int err;

	if (!otx2_mbox_nonempty(&mbox->mbox, 0))
		return 0;
	otx2_mbox_msg_send(&mbox->mbox, 0);
	err = otx2_mbox_wait_for_rsp(&mbox->mbox, 0);
	if (err)
		return err;

	return otx2_mbox_check_rsp_msgs(&mbox->mbox, 0);
}

static inline int rvu_gen_pf_sync_mbox_up_msg(struct mbox *mbox, int devid)
{
	int err;

	if (!otx2_mbox_nonempty(&mbox->mbox_up, devid))
		return 0;
	otx2_mbox_msg_send_up(&mbox->mbox_up, devid);
	err = otx2_mbox_wait_for_rsp(&mbox->mbox_up, devid);
	if (err)
		return err;

	return otx2_mbox_check_rsp_msgs(&mbox->mbox_up, devid);
}

#define M(_name, _id, _fn_name, _req_type, _rsp_type)			\
static struct _req_type __maybe_unused					\
*gen_pf_mbox_alloc_msg_ ## _fn_name(struct mbox *mbox)			\
{									\
	struct _req_type *req;						\
	u16 id = _id;							\
									\
	req = (struct _req_type *)otx2_mbox_alloc_msg_rsp(		\
		&mbox->mbox, 0, sizeof(struct _req_type),		\
		sizeof(struct _rsp_type));				\
	if (!req)							\
		return NULL;						\
	req->hdr.sig = OTX2_MBOX_REQ_SIG;				\
	req->hdr.id = id;						\
	trace_otx2_msg_alloc(mbox->mbox.pdev, id, sizeof(*req));	\
	return req;							\
}

MBOX_MESSAGES
#undef M

/* Mbox bounce buffer APIs */
static inline int otx2_mbox_bbuf_init(struct mbox *mbox, struct pci_dev *pdev)
{
	struct otx2_mbox *otx2_mbox;
	struct otx2_mbox_dev *mdev;

	mbox->bbuf_base = devm_kmalloc(&pdev->dev, MBOX_SIZE, GFP_KERNEL);

	if (!mbox->bbuf_base)
		return -ENOMEM;

	/* Overwrite mbox mbase to point to bounce buffer, so that PF/VF
	 * prepare all mbox messages in bounce buffer instead of directly
	 * in hw mbox memory.
	 */
	otx2_mbox = &mbox->mbox;
	mdev = &otx2_mbox->dev[0];
	mdev->mbase = mbox->bbuf_base;

	otx2_mbox = &mbox->mbox_up;
	mdev = &otx2_mbox->dev[0];
	mdev->mbase = mbox->bbuf_base;
	return 0;
}

static inline void otx2_sync_mbox_bbuf(struct otx2_mbox *mbox, int devid)
{
	u16 msgs_offset = ALIGN(sizeof(struct mbox_hdr), MBOX_MSG_ALIGN);
	void *hw_mbase = mbox->hwbase + (devid * MBOX_SIZE);
	struct otx2_mbox_dev *mdev = &mbox->dev[devid];
	struct mbox_hdr *hdr;
	u64 msg_size;

	if (mdev->mbase == hw_mbase)
		return;

	hdr = hw_mbase + mbox->rx_start;
	msg_size = hdr->msg_size;

	if (msg_size > mbox->rx_size - msgs_offset)
		msg_size = mbox->rx_size - msgs_offset;

	/* Copy mbox messages from mbox memory to bounce buffer */
	memcpy(mdev->mbase + mbox->rx_start,
	       hw_mbase + mbox->rx_start, msg_size + msgs_offset);
}
