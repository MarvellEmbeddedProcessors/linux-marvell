/*
 * Copyright (C) 2013 Marvell International Ltd. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms and conditions of the GNU General Public License,
 * version 2, as published by the Free Software Foundation.
 */

#include <linux/module.h>
#include <linux/dma-mapping.h>
#include <linux/kernel.h>
#include <linux/delay.h>
#include <linux/ioport.h>
#include <linux/sched.h>
#include <linux/slab.h>
#include <linux/errno.h>
#include <linux/init.h>
#include <linux/timer.h>
#include <linux/list.h>
#include <linux/notifier.h>
#include <linux/interrupt.h>
#include <linux/moduleparam.h>
#include <linux/device.h>
#include <linux/usb/ch9.h>
#include <linux/usb/gadget.h>
#include <linux/usb/phy.h>
#include <linux/pm.h>
#include <linux/io.h>
#include <linux/irq.h>
#include <linux/platform_device.h>
#include <linux/platform_data/mv_usb.h>
#include <linux/clk.h>
#include <asm/unaligned.h>
#include <asm/byteorder.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <linux/of.h>
#include <linux/of_device.h>
#include <linux/pm_qos.h>
#include <linux/time.h>
#include <asm/cputype.h>
#include <linux/highmem.h>
#include <linux/of_gpio.h>
#include <linux/gpio.h>
#include <linux/phy/phy.h>
#include <linux/usb/composite.h>

#include "mvebu_u3d.h"

#define DRIVER_DESC    "Marvell Central IP USB3.0 Device Controller driver"

static unsigned int u1u2;
module_param(u1u2, uint, S_IRUGO | S_IWUSR);
MODULE_PARM_DESC(u1u2, "u1u2 enable");

static const char driver_desc[] = DRIVER_DESC;

unsigned int u1u2_enabled(void)
{
	return u1u2;
}

#define EP0_MAX_PKT_SIZE    512

/* for endpoint 0 operations */
static const struct usb_endpoint_descriptor mvc2_ep0_out_desc = {
	.bLength = USB_DT_ENDPOINT_SIZE,
	.bDescriptorType = USB_DT_ENDPOINT,
	.bEndpointAddress = USB_DIR_OUT,
	.bmAttributes = USB_ENDPOINT_XFER_CONTROL,
	.wMaxPacketSize = EP0_MAX_PKT_SIZE,
};

static const struct usb_endpoint_descriptor mvc2_ep0_in_desc = {
	.bLength = USB_DT_ENDPOINT_SIZE,
	.bDescriptorType = USB_DT_ENDPOINT,
	.bEndpointAddress = USB_DIR_IN,
	.bmAttributes = USB_ENDPOINT_XFER_CONTROL,
	.wMaxPacketSize = EP0_MAX_PKT_SIZE,
};

static struct usb_ss_ep_comp_descriptor ep0_comp = {
	.bLength = sizeof(ep0_comp),
	.bDescriptorType = USB_DT_SS_ENDPOINT_COMP,
};

static int mvc2_ep0_handle_status(struct mvc2 *cp, struct usb_ctrlrequest *ctrl)
{
	unsigned int recip;
	u16 usb_status = 0, lowpower;
	__le16 *response_pkt;
	int num, dir;
	struct mvc2_ep *ep;

	recip = ctrl->bRequestType & USB_RECIP_MASK;
	switch (recip) {
	case USB_RECIP_DEVICE:
		/*
		 * LTM will be set once we know how to set this in HW.
		 */
		if (cp->status & MVCP_STATUS_SELF_POWERED)
			usb_status |= USB_DEVICE_SELF_POWERED;

		lowpower = MV_CP_READ(MVCP_LOWPOWER);
		if (lowpower & MVCP_LOWPOWER_U1_EN)
			usb_status |= 1 << USB_DEV_STAT_U1_ENABLED;

		if (lowpower & MVCP_LOWPOWER_U2_EN)
			usb_status |= 1 << USB_DEV_STAT_U2_ENABLED;

		break;

	case USB_RECIP_INTERFACE:
		/*
		 * Function Remote Wake Capable    D0
		 * Function Remote Wakeup    D1
		 */
		break;

	case USB_RECIP_ENDPOINT:

		num = ctrl->wIndex & USB_ENDPOINT_NUMBER_MASK;
		dir = ctrl->wIndex & USB_DIR_IN;
		ep = &cp->eps[2 * num + !!dir];

		if (ep->state & MV_CP_EP_STALL)
			usb_status = 1 << USB_ENDPOINT_HALT;
		break;
	default:
		return -EINVAL;
	}

	response_pkt = (__le16 *) cp->setup_buf;
	*response_pkt = cpu_to_le16(usb_status);

	return sizeof(*response_pkt);
}

static void enable_lowpower(struct mvc2 *cp, unsigned int lowpower, int on)
{
	unsigned int val, state;

	val = MV_CP_READ(MVCP_LOWPOWER);
	if (lowpower == USB_DEVICE_U1_ENABLE)
		state = MVCP_LOWPOWER_U1_EN;
	else
		state = MVCP_LOWPOWER_U2_EN;

	if (on)
		val |= state;
	else
		val &= ~state;

	if (u1u2_enabled())
		MV_CP_WRITE(val, MVCP_LOWPOWER);
}

static int mvc2_ep0_handle_feature(struct mvc2 *cp,
				   struct usb_ctrlrequest *ctrl, int set)
{
	u32 wValue, wIndex, recip;
	int ret = -EINVAL;
	int num, dir;
	struct mvc2_ep *ep;
	unsigned long flags;

	wValue = le16_to_cpu(ctrl->wValue);
	wIndex = le16_to_cpu(ctrl->wIndex);
	recip = ctrl->bRequestType & USB_RECIP_MASK;

	switch (recip) {
	case USB_RECIP_DEVICE:
		switch (wValue) {
		case USB_DEVICE_REMOTE_WAKEUP:
			ret = 0;
			break;

		case USB_DEVICE_U1_ENABLE:
		case USB_DEVICE_U2_ENABLE:
			if (cp->dev_state != MVCP_CONFIGURED_STATE) {
				ret = -EINVAL;
				break;
			}

			ret = 0;

			enable_lowpower(cp, wValue, set);
			break;
		case USB_DEVICE_TEST_MODE:
			if (set && (wIndex & 0xff))
				cp->status |= MVCP_STATUS_TEST(wIndex >> 8);
			break;
		}
		break;
	case USB_RECIP_INTERFACE:
		switch (wValue) {
		case USB_INTRF_FUNC_SUSPEND:
			ret = 0;
		}
		break;
	case USB_RECIP_ENDPOINT:
		switch (wValue) {
		case USB_ENDPOINT_HALT:
			num = wIndex & USB_ENDPOINT_NUMBER_MASK;
			dir = wIndex & USB_DIR_IN;
			ep = &cp->eps[2 * num + !!dir];
			if (!set) {
				spin_lock_irqsave(&ep->lock, flags);
				reset_seqencenum(ep, num, dir);
				spin_unlock_irqrestore(&ep->lock, flags);
				if (!(ep->state & MV_CP_EP_WEDGE))
					usb_ep_clear_halt(&ep->ep);
			} else
				usb_ep_set_halt(&ep->ep);
			ret = 0;
		}
	}

	return ret;
}

static void mvcp_ep0_set_sel_cmpl(struct usb_ep *ep, struct usb_request *req)
{
	struct mvc2_ep *_ep = container_of(ep, struct mvc2_ep, ep);
	struct mvc2 *cp = _ep->cp;
	struct timing {
		u8 u1sel;
		u8 u1pel;
		u16 u2sel;
		u16 u2pel;
	} __packed timing;

	memcpy(&timing, req->buf, sizeof(timing));
	cp->u1sel = timing.u1sel;
	cp->u1pel = timing.u1pel;
	cp->u2sel = le16_to_cpu(timing.u2sel);
	cp->u2pel = le16_to_cpu(timing.u2pel);
}

int mvc2_std_request(struct mvc2 *cp, struct usb_ctrlrequest *r,
		     bool *delegate)
{
	int ret = 0;
	struct usb_request *req;
	u16 wLength = le16_to_cpu(r->wLength);
	u16 wValue = le16_to_cpu(r->wValue);
	u16 wIndex = le16_to_cpu(r->wIndex);

	*delegate = true;
	req = &cp->ep0_req.req;
	switch (r->bRequest) {
	case USB_REQ_SET_ADDRESS:
		if (r->bRequestType != (USB_DIR_OUT | USB_RECIP_DEVICE))
			break;

		*delegate = false;
		if (wValue > 127) {
			dev_dbg(cp->dev, "invalid device address %d\n", wValue);
			break;
		}

		if (cp->dev_state == MVCP_CONFIGURED_STATE) {
			dev_dbg(cp->dev,
				"trying to set address when configured\n");
			break;
		}

		if (wValue)
			cp->dev_state = MVCP_ADDRESS_STATE;
		else
			cp->dev_state = MVCP_DEFAULT_STATE;
		break;
	case USB_REQ_GET_STATUS:
		if (r->bRequestType != (USB_DIR_IN | USB_RECIP_DEVICE) &&
		    r->bRequestType != (USB_DIR_IN | USB_RECIP_ENDPOINT) &&
		    r->bRequestType != (USB_DIR_IN | USB_RECIP_INTERFACE))
			break;

		ret = mvc2_ep0_handle_status(cp, r);
		*delegate = false;

		break;
	case USB_REQ_CLEAR_FEATURE:
	case USB_REQ_SET_FEATURE:
		if (r->bRequestType != (USB_DIR_OUT | USB_RECIP_DEVICE) &&
		    r->bRequestType != (USB_DIR_OUT | USB_RECIP_ENDPOINT) &&
		    r->bRequestType != (USB_DIR_OUT | USB_RECIP_INTERFACE))
			break;

		ret = mvc2_ep0_handle_feature(cp, r,
					      r->bRequest ==
					      USB_REQ_SET_FEATURE);
		*delegate = false;
		break;
	case USB_REQ_SET_CONFIGURATION:
		switch (cp->dev_state) {
		case MVCP_DEFAULT_STATE:
			break;
		case MVCP_ADDRESS_STATE:
			if (wValue) {
				enable_lowpower(cp, USB_DEVICE_U1_ENABLE, 0);
				enable_lowpower(cp, USB_DEVICE_U2_ENABLE, 0);
				cp->dev_state = MVCP_CONFIGURED_STATE;
			}
			break;
		case MVCP_CONFIGURED_STATE:
			if (!wValue)
				cp->dev_state = MVCP_ADDRESS_STATE;
			break;
		}
		break;
	case USB_REQ_SET_SEL:
		*delegate = false;
		if (cp->dev_state == MVCP_DEFAULT_STATE)
			break;

		if (wLength == 6) {
			ret = wLength;
			req->complete = mvcp_ep0_set_sel_cmpl;
		}
		break;
	case USB_REQ_SET_ISOCH_DELAY:
		*delegate = false;
		if (!wIndex && !wLength) {
			ret = 0;
			cp->isoch_delay = wValue;
		}
		break;
	}

	if (ret > 0) {
		req->length = ret;
		req->zero = ret < wLength;
		req->buf = cp->setup_buf;
		ret = usb_ep_queue(cp->gadget.ep0, req, GFP_ATOMIC);
	}

	if (ret < 0)
		*delegate = false;

	return ret;
}

int eps_init(struct mvc2 *cp)
{
	struct mvc2_ep *ep;
	int i, j, ret;
	struct bd *bd;
	unsigned int phys, bd_interval;

	bd_interval = sizeof(struct bd);

	/* initialize endpoints */
	for (i = 0; i < cp->epnum * 2; i++) {
		ep = &cp->eps[i];
		ep->ep.name = ep->name;
		ep->cp = cp;
		INIT_LIST_HEAD(&ep->queue);
		INIT_LIST_HEAD(&ep->wait);
		INIT_LIST_HEAD(&ep->tmp);
		spin_lock_init(&ep->lock);

		if (i < 2) {

			strncpy(ep->name, "ep0", MAXNAME);
			usb_ep_set_maxpacket_limit(&ep->ep, EP0_MAX_PKT_SIZE);
			ep->ep.desc = (i) ? &mvc2_ep0_in_desc :
			    &mvc2_ep0_out_desc;
			ep->ep.comp_desc = &ep0_comp;
			ep->bd_sz = MAX_QUEUE_SLOT;
			ep->left_bds = MAX_QUEUE_SLOT;
			ep->dir = i ? 1 : 0;
			if (ep->dir == 1)
				ep->ep.caps.dir_in = true;
			else
				ep->ep.caps.dir_out = true;
			ep->ep.caps.type_control = true;

		} else {
			if (i & 0x1) {
				ep->dir = 1;
				snprintf(ep->name, MAXNAME, "ep%din", i >> 1);
				ep->ep.caps.dir_in = true;
			} else {
				ep->dir = 0;
				snprintf(ep->name, MAXNAME, "ep%dout", i >> 1);
				ep->ep.caps.dir_out = true;
			}
			usb_ep_set_maxpacket_limit(&ep->ep, (unsigned short) ~0);
			ep->bd_sz = MAX_QUEUE_SLOT;
			ep->left_bds = MAX_QUEUE_SLOT;
			ep->ep.caps.type_iso = true;
			ep->ep.caps.type_bulk = true;
			ep->ep.caps.type_int = true;
		}

		ep->ep_num = i / 2;

		ep->doneq_start = dma_alloc_coherent(cp->dev,
						     sizeof(struct doneq) *
						     ep->bd_sz,
						     &ep->doneq_start_phys,
						     GFP_KERNEL);
		if (ep->doneq_start == NULL) {
			dev_err(cp->dev, "failed to allocate doneq buffer!\n");
			return -ENOMEM;
		}

		ep->bd_ring = dma_alloc_coherent(cp->dev,
						 sizeof(struct bd) * ep->bd_sz,
						 &ep->bd_ring_phys, GFP_KERNEL);
		if (ep->bd_ring == NULL) {
			dev_err(cp->dev, "failed to allocate bd buffer!\n");
			return -ENOMEM;
		}
		bd = (struct bd *)ep->bd_ring;
		phys = ep->bd_ring_phys;
		/* Generate the TransferQ ring */
		for (j = 0; j < ep->bd_sz - 1; j++) {
			phys += bd_interval;
			bd->phys_next = phys;
			bd->cmd = 0;
			if (ip_ver(cp) < USB3_IP_VER_A0)
				bd->cmd = BD_NXT_PTR_JUMP;
			bd++;
		}
		bd->cmd = 0;
		if (ip_ver(cp) < USB3_IP_VER_A0)
			bd->cmd = BD_NXT_PTR_JUMP;
		bd->phys_next = ep->bd_ring_phys;
	}

	cp->setup_buf = kzalloc(EP0_MAX_PKT_SIZE, GFP_KERNEL);
	if (!cp->setup_buf)
		ret = -ENOMEM;

	return 0;
}

#define CREATE_TRACE_POINTS
/* #define ASSEMBLE_REQ */

static const char driver_name[] = "mvebu-u3d";

#define ep_dir(ep)    (((ep)->dir))

static bool irq_enabled;
bool usb3_disconnect = true;

/* return the actual ep number   */
static int ip_ep_num(struct mvc2 *cp)
{
	return MVCP_EP_COUNT;
}

static void done(struct mvc2_ep *ep, struct mvc2_req *req, int status);
static void nuke(struct mvc2_ep *ep, int status);
static void stop_activity(struct mvc2 *udc, struct usb_gadget_driver *driver);

static void set_top_int(struct mvc2 *cp, unsigned int val)
{
	if (ip_ver(cp) >= USB3_IP_VER_Z2)
		MV_CP_WRITE(val, MVCP_TOP_INT_EN);
}

static void ep_dma_enable(struct mvc2 *cp, int num, int dir, int enable)
{
	unsigned int tmp, val, reg;

	if (ip_ver(cp) <= USB3_IP_VER_Z2) {
		tmp = (dir) ? 0x10000 : 0x1;
		tmp = tmp << num;
		reg = MVCP_DMA_ENABLE;
	} else {
		tmp = DONEQ_CONFIG;
		if (dir)
			reg = SS_IN_DMA_CONTROL_REG(num);
		else
			reg = SS_OUT_DMA_CONTROL_REG(num);
	}

	val = MV_CP_READ(reg);
	if (enable)
		MV_CP_WRITE(val | tmp, reg);
	else
		MV_CP_WRITE(val & ~tmp, reg);
}

static void ep_dma_struct_init(struct mvc2 *cp,
			       struct mvc2_ep *ep, int num, int dir)
{
	dma_addr_t addr;

	addr = ep->doneq_start_phys + sizeof(struct doneq) * (ep->bd_sz - 1);
	MV_CP_WRITE(ep->bd_ring_phys, ep_dma_addr(num, dir));

	ep_dma_enable(cp, num, dir, 0);
	MV_CP_WRITE(ep->doneq_start_phys, ep_doneq_start(num, dir));
	MV_CP_WRITE(ep->doneq_start_phys, ep_doneq_read(num, dir));
	MV_CP_WRITE(addr, ep_doneq_end(num, dir));
	ep_dma_enable(cp, num, dir, 1);
}

/* Need to be included in ep lock protection */
static void mvc2_dma_reset(struct mvc2 *cp,
			   struct mvc2_ep *ep, int num, int dir)
{
	unsigned int epbit, val, creg, sreg;
	int timeout = 10000;
	struct mvc2_req *req, *tmp;
	unsigned long flags;

	spin_lock_irqsave(&ep->lock, flags);
	if (ip_ver(cp) <= USB3_IP_VER_Z2) {
		epbit = EPBIT(num, dir);
		MV_CP_WRITE(epbit, MVCP_DMA_HALT);
		while ((!(MV_CP_READ(MVCP_DMA_HALT_DONE) &
			  epbit)) && timeout-- > 0)
			cpu_relax();
		MV_CP_WRITE(epbit, MVCP_DMA_HALT_DONE);
	} else {
		if (dir) {
			creg = SS_IN_DMA_CONTROL_REG(num);
			sreg = SS_IN_EP_INT_STATUS_REG(num);
		} else {
			creg = SS_OUT_DMA_CONTROL_REG(num);
			sreg = SS_OUT_EP_INT_STATUS_REG(num);
		}
		val = MV_CP_READ(creg);
		val |= DMA_HALT;
		MV_CP_WRITE(val, creg);
		while ((!(MV_CP_READ(sreg) & DMA_HALT_DONE)) && timeout-- > 0)
			cpu_relax();
		MV_CP_WRITE(DMA_HALT_DONE, sreg);
	}

	if (timeout <= 0) {
		pr_info("### dma reset timeout, num = %d, dir = %d\n", num,
			dir);
		WARN_ON(1);
	}

	list_for_each_entry_safe(req, tmp, &ep->queue, queue)
		done(ep, req, -ESHUTDOWN);

	ep->bd_cur = ep->doneq_cur = 0;
	ep_dma_struct_init(cp, &cp->eps[2 * num + !!dir], num, dir);
	spin_unlock_irqrestore(&ep->lock, flags);
}

static struct usb_request *mvc2_alloc_request(struct usb_ep *_ep,
					      gfp_t gfp_flags)
{
	struct mvc2_req *req = NULL;

	req = kzalloc(sizeof(*req), gfp_flags);
	if (!req)
		return NULL;

	memset(req, 0, sizeof(*req));
	INIT_LIST_HEAD(&req->queue);
	return &req->req;
}

static void mvc2_free_request(struct usb_ep *_ep, struct usb_request *_req)
{
	struct mvc2_ep *ep = container_of(_ep, struct mvc2_ep, ep);
	struct mvc2_req *req = container_of(_req, struct mvc2_req, req);
	unsigned long flags;

	spin_lock_irqsave(&ep->lock, flags);
	list_del_init(&req->queue);
	spin_unlock_irqrestore(&ep->lock, flags);
	kfree(req);
}

static int
alloc_one_bd_chain(struct mvc2 *cp, struct mvc2_ep *ep, struct mvc2_req *req,
		   int num, int dir, dma_addr_t dma, unsigned length,
		   unsigned offset, unsigned *last)
{
	unsigned int bd_num, remain, bd_cur, len, buf;
	struct bd *bd;
	int left_bds, cur_bd;

	remain = length - offset;

	/* In the zero length packet case, we still need one BD to make it happen */
	if (remain)
		bd_num = (remain + BD_MAX_SIZE - 1) >> BD_SEGMENT_SHIFT;
	else
		bd_num = 1;

	bd_cur = ep->bd_cur;
	left_bds = ep->left_bds;
	if (left_bds == 0)
		goto no_bds;
	if (bd_num > left_bds)
		goto no_bds;
	ep->left_bds -= bd_num;
	WARN_ON(ep->left_bds > ep->bd_sz);

	ep->bd_cur += bd_num;
	if (ep->bd_cur >= ep->bd_sz)
		ep->bd_cur -= ep->bd_sz;

	ep->state |= MV_CP_EP_TRANSERING;
	req->bd_total += bd_num;
	buf = (unsigned int)dma;
	/*
	 * format BD chains:
	 * BD_NXT_RDY make a BD chain segment, and
	 * one BD chain segment is natually one usb_request.
	 * But with exception that if current number of BD
	 * cannot fulfill usb_request, so that we may divide
	 * one request into several segments, so that it could
	 * complete gradually.
	 * DMA engine would never cache across two segments
	 * without MVCP_EPDMA_START being set, which indicate
	 * new BD segment is coming.
	 */
	cur_bd = bd_num;
	do {
		if (remain > BD_MAX_SIZE)
			len = BD_MAX_SIZE;
		else {
			/*
			 * HW require out ep's BD length is 1024 aligned,
			 * or there is problem in receiving the compelte interrupt
			 */
			len = remain;
			if (!dir && (len & 0x3ff))
				len = ((len + 0x3ff) >> 10) << 10;
		}
		remain -= len;

		bd = ep->bd_ring + bd_cur;

		bd_cur++;
		if (bd_cur == ep->bd_sz)
			bd_cur = 0;

		if (!offset)
			req->bd = bd;

		/*
		 * There are three method to indicate one bd is finished
		 * 1. Receive the short packet which is less than 1024
		 * 2. Receive the zero length packet
		 * 3. Receive the data length equal to size set by BD
		 */
		bd->cmd = BD_NXT_RDY | BD_BUF_RDY | BD_BUF_SZ(len);
		if (ip_ver(cp) < USB3_IP_VER_A0)
			bd->cmd |= BD_NXT_PTR_JUMP;
		bd->buf = (unsigned int)dma + offset;

		offset += len;
	} while (--cur_bd > 0);

	if (*last) {
		/* Only raise the interrupt at the last bd */
#ifndef ASSEMBLE_REQ
#if 0
		/* due to usb2 rx interrupt optimization, no_interrupt is
		 * is always 1. Due to HW bug, this is currently irrelevant for
		 * our case since an interrupt will be returned regardless of
		 * BD_INT_EN.
		 */
		if (!req->req.no_interrupt)
#endif
#endif
			bd->cmd |= BD_INT_EN;
		/* At the end of one segment, clear the BD_NXT_RDY */
		bd->cmd &= ~BD_NXT_RDY;
	}
	*last = left_bds;

	return bd_num;
no_bds:
	WARN_ON(ep->ep_num == 0);
	return 0;
}

static int alloc_bds(struct mvc2 *cp, struct mvc2_ep *ep, struct mvc2_req *req)
{
	dma_addr_t dma;
	unsigned length, bd_num, actual;
	struct usb_request *request = &req->req;
	int num, dir, last;

	bd_num = 0;
	actual = req->req.actual;
	num = ep->ep.desc->bEndpointAddress & USB_ENDPOINT_NUMBER_MASK;
	dir = ep->ep.desc->bEndpointAddress & USB_DIR_IN;

	req->bd_total = 0;
	last = 1;
	if (req->req.num_mapped_sgs > 0) {
		struct scatterlist *sg = request->sg;
		struct scatterlist *s;
		int i;

		last = 0;
		for_each_sg(sg, s, request->num_mapped_sgs, i) {
			length = sg_dma_len(s);
			if (actual >= length) {
				actual -= length;
				continue;
			}

			actual += sg->offset;
			dma = sg_dma_address(s);
			if (sg_is_last(s))
				last = 1;

			bd_num = alloc_one_bd_chain(cp, ep, req, num, dir,
						    dma, length, actual, &last);
			if (last > 1)
				last = 0;
			if (!bd_num)
				break;
		}
	} else {
		dma = req->req.dma;
		length = req->req.length;

		bd_num = alloc_one_bd_chain(cp, ep, req, num, dir,
					    dma, length, actual, &last);
	}

	if (bd_num)
		list_add_tail(&req->queue, &ep->queue);

	return bd_num;
}

#ifdef ASSEMBLE_REQ
static int
alloc_in_bds(struct mvc2 *cp, struct mvc2_ep *ep, struct mvc2_req *req,
	     int *last)
{
	dma_addr_t dma;
	unsigned length, bd_num, actual;
	struct usb_request *request = &req->req;
	int num, dir;

	bd_num = 0;
	actual = req->req.actual;
	num = ep->ep.desc->bEndpointAddress & USB_ENDPOINT_NUMBER_MASK;
	dir = ep->ep.desc->bEndpointAddress & USB_DIR_IN;

	req->bd_total = 0;

	dma = req->req.dma;
	length = req->req.length;

	bd_num = alloc_one_bd_chain(cp, ep, req, num, dir,
				    dma, length, actual, last);

	list_add_tail(&req->queue, &ep->queue);

	return bd_num;
}
#endif

static inline void
mvc2_ring_incoming(struct mvc2 *cp, unsigned int num, unsigned dir)
{
	unsigned int reg;

	/* Ensure that updates to the EP Context will occur before Ring Bell */
	wmb();

	/* Ring the data incoming bell to ask hw to reload the bd chain */
	if (ip_ver(cp) <= USB3_IP_VER_Z2) {
		MV_CP_WRITE(MVCP_EPDMA_START, ep_dma_config(num, dir));
	} else {
		if (dir)
			reg = SS_IN_DMA_CONTROL_REG(num);
		else
			reg = SS_OUT_DMA_CONTROL_REG(num);
		MV_CP_WRITE(MV_CP_READ(reg) | DMA_START, reg);
	}
}

static void ep_enable(struct mvc2_ep *ep, int num, int in, int type)
{
	struct mvc2 *cp = ep->cp;
	struct usb_ep *_ep = &ep->ep;
	unsigned int config, val, config_base;
	struct mvc2_req *req, *tmp;
	unsigned long flags, ring, reg;

	/* We suppose there is no item in run queue here */
	WARN_ON(!list_empty(&ep->queue));

	ring = ep->state = 0;
	config_base = epcon(num, in);
	config = MVCP_EP_MAX_PKT(_ep->desc->wMaxPacketSize);
	if (_ep->comp_desc)
		config |= MVCP_EP_BURST(_ep->comp_desc->bMaxBurst);

	if (num) {
		config |= MVCP_EP_ENABLE | MVCP_EP_NUM(num);
		switch (type) {
		case USB_ENDPOINT_XFER_BULK:
			if (_ep->comp_desc &&
			    _ep->comp_desc->bmAttributes & 0x1f)
				ep->state |= MV_CP_EP_BULK_STREAM;
			else
				ep->state &= ~MV_CP_EP_BULK_STREAM;

			config |= MVCP_EP_TYPE_BLK;

			/* Enable bulk stream if need */
			spin_lock_irqsave(&cp->lock, flags);
			if (ip_ver(cp) <= USB3_IP_VER_Z2) {
				val = MV_CP_READ(MVCP_BULK_STREAMING_ENABLE);
				if (ep->state & MV_CP_EP_BULK_STREAM)
					val |= EPBIT(num, in);
				else
					val &= ~EPBIT(num, in);
				MV_CP_WRITE(val, MVCP_BULK_STREAMING_ENABLE);
			} else {
				if (ep->state & MV_CP_EP_BULK_STREAM)
					config |= MVCP_EP_BULK_STREAM_EN;
				else
					config &= ~MVCP_EP_BULK_STREAM_EN;
			}
			spin_unlock_irqrestore(&cp->lock, flags);
			break;
		case USB_ENDPOINT_XFER_ISOC:
			config |= MVCP_EP_TYPE_ISO;
			if (ip_ver(cp) <= USB3_IP_VER_Z2) {
				if (in)
					reg = EP_IN_BINTERVAL_REG_1_2_3 +
					    4 * (num / 4);
				else
					reg = EP_OUT_BINTERVAL_REG_1_2_3 +
					    4 * (num / 4);
				val = MV_CP_READ(reg);
				val |= (_ep->desc->bInterval) << (num % 4) * 8;
				MV_CP_WRITE(val, reg);
			} else {
				if (in)
					reg = EP_IN_BINTERVAL_REG(num);
				else
					reg = EP_OUT_BINTERVAL_REG(num);
				MV_CP_WRITE(_ep->desc->bInterval, reg);
			}
			break;
		case USB_ENDPOINT_XFER_INT:
			config |= MVCP_EP_TYPE_INT;
			break;
		}
	}

	MV_CP_WRITE(config, config_base);
	spin_unlock(&ep->lock);
	mvc2_dma_reset(cp, ep, num, in);
	spin_lock(&ep->lock);
	/* Reset sequence number */
	if (num != 0)
		reset_seqencenum(ep, num, in);

	/* Requeue the bd */
	list_for_each_entry_safe(req, tmp, &ep->wait, queue) {
		list_del_init(&req->queue);
		val = alloc_bds(cp, ep, req);
		/* Current all bds have been allocated, just wait for previous complete */
		if (val)
			ring = 1;
		else {
			dev_dbg(cp->dev, "%s %d\n", __func__, __LINE__);
			list_add(&req->queue, &ep->wait);
			break;
		}
	}

	if (ring)
		mvc2_ring_incoming(cp, num, in);
}

static int mvc2_ep_enable(struct usb_ep *_ep,
			  const struct usb_endpoint_descriptor *desc)
{
	struct mvc2_ep *ep = container_of(_ep, struct mvc2_ep, ep);
	int n = desc->bEndpointAddress & USB_ENDPOINT_NUMBER_MASK;
	int in = (desc->bEndpointAddress & USB_DIR_IN) != 0;
	unsigned int state;
	unsigned long flags;

	_ep->maxpacket = le16_to_cpu(desc->wMaxPacketSize);
	_ep->desc = desc;

	spin_lock_irqsave(&ep->lock, flags);
	ep_enable(ep, n, in, desc->bmAttributes & USB_ENDPOINT_XFERTYPE_MASK);

	state = (ep->state & MV_CP_EP_WEDGE) | MV_CP_EP_NUM(n);
	state |= in ? MV_CP_EP_DIRIN : 0;
	ep->state = state;
	spin_unlock_irqrestore(&ep->lock, flags);

	return 0;
}

static void ep_disable(struct mvc2 *cp, int num, int dir)
{
	unsigned int config;
	struct mvc2_ep *ep = &cp->eps[2 * num + !!dir];

	config = MV_CP_READ(epcon(num, dir));
	config &= ~MVCP_EP_ENABLE;
	MV_CP_WRITE(config, epcon(num, dir));

	spin_unlock(&ep->lock);
	/* nuke all pending requests (does flush) */
	nuke(ep, -ESHUTDOWN);
	spin_lock(&ep->lock);
}

static int mvc2_ep_disable(struct usb_ep *_ep)
{
	struct mvc2_ep *ep = container_of(_ep, struct mvc2_ep, ep);
	struct mvc2 *cp = ep->cp;
	unsigned long flags;

	if (!(ep->state & MV_CP_EP_NUM_MASK))
		return 0;

	spin_lock_irqsave(&ep->lock, flags);
	ep_disable(cp, ep->state & MV_CP_EP_NUM_MASK,
		   ep->state & MV_CP_EP_DIRIN);
	spin_unlock_irqrestore(&ep->lock, flags);

	return 0;
}

static inline void mvc2_send_erdy(struct mvc2 *cp)
{
	/* ep0 erdy should be smp safe, and no lock is needed */
	MV_CP_WRITE(MV_CP_READ(MVCP_ENDPOINT_0_CONFIG) |
		    MVCP_ENDPOINT_0_CONFIG_CHG_STATE, MVCP_ENDPOINT_0_CONFIG);
}

#ifndef ASSEMBLE_REQ
/* queues (submits) an I/O request to an endpoint */
static int
mvc2_ep_queue(struct usb_ep *_ep, struct usb_request *_req, gfp_t gfp_flags)
{
	struct mvc2_ep *ep = container_of(_ep, struct mvc2_ep, ep);
	struct mvc2_req *req = container_of(_req, struct mvc2_req, req);
	struct mvc2 *cp = ep->cp;
	unsigned int dir, num;
	unsigned long flags;
	int ret;

	if (_ep == NULL || _req == NULL)
		return -EINVAL;

	num = _ep->desc->bEndpointAddress & USB_ENDPOINT_NUMBER_MASK;
	/* Reset the endpoint 0 to prevent previous left data */
	if (num == 0) {
		/*
		 * After USB_GADGET_DELAYED_STATUS is set and the USB upper layer in USB function thread
		 * finishes the handling, the USB compsite layer will send request to continue with the
		 * control transfer, within this request, the request length is set 0.
		 * Since the request length will not be 0 for normal transfer, once it is 0, it means
		 * that to continue the transfer after USB_GADGET_DELAYED_STATUS. Thus the erdy is set
		 * here to notify the host that device is ready for latter transfer.
		 */
		if (!req->req.length) {
			mvc2_send_erdy(cp);
			return 0;
		}

		if (cp->ep0_dir == USB_DIR_IN)
			ep = &cp->eps[1];
		else
			ep = &cp->eps[0];

		dir = cp->ep0_dir;

		spin_lock_irqsave(&ep->lock, flags);

		MV_CP_WRITE(ep->doneq_cur * 8 + ep->doneq_start_phys,
			    ep_doneq_read(num, dir));
		ep->doneq_cur++;
		if (ep->doneq_cur == ep->bd_sz)
			ep->doneq_cur = 0;

		spin_unlock_irqrestore(&ep->lock, flags);
	} else
		dir = _ep->desc->bEndpointAddress & USB_DIR_IN;

	ret = usb_gadget_map_request(&cp->gadget, &req->req, dir);
	if (ret)
		return ret;

	_req->actual = 0;
	_req->status = -EINPROGRESS;
	spin_lock_irqsave(&ep->lock, flags);

	ret = alloc_bds(cp, ep, req);
	/* Current all bds have been allocated, just wait for previous complete */
	if (!ret) {
		dev_dbg(cp->dev, "%s %d\n", __func__, __LINE__);
		list_add_tail(&req->queue, &ep->wait);
	} else
		mvc2_ring_incoming(cp, num, dir);

	spin_unlock_irqrestore(&ep->lock, flags);

	return 0;
}
#else
static int
mvc2_ep_queue(struct usb_ep *_ep, struct usb_request *_req, gfp_t gfp_flags)
{
	struct mvc2_ep *ep = container_of(_ep, struct mvc2_ep, ep);
	struct mvc2_req *req = container_of(_req, struct mvc2_req, req), *tmp;
	struct mvc2 *cp = ep->cp;
	unsigned int dir, num;
	unsigned long flags;
	int ret, last, reqcnt;
	static int cnt;
#define CNT 10
	if (_ep == NULL || _req == NULL)
		return -EINVAL;

	num = _ep->desc->bEndpointAddress & USB_ENDPOINT_NUMBER_MASK;
	/* Reset the endpoint 0 to prevent previous left data */
	if (num == 0) {
		/*
		 * After USB_GADGET_DELAYED_STATUS is set and the USB upper layer in USB function thread
		 * finishes the handling, the USB compsite layer will send request to continue with the
		 * control transfer. Within this request, the request length is set 0.
		 * Since the request length will not be 0 for normal transfer, once it is 0, it means
		 * that to continue the transfer after USB_GADGET_DELAYED_STATUS. Thus the erdy is set
		 * here to notify the host that device is ready for latter transfer.
		 */
		if (!req->req.length) {
			mvc2_send_erdy(cp);
			return 0;
		}

		if (cp->ep0_dir == USB_DIR_IN)
			ep = &cp->eps[1];
		else
			ep = &cp->eps[0];

		dir = cp->ep0_dir;

		spin_lock_irqsave(&ep->lock, flags);
		mvc2_dma_reset(cp, ep, num, dir);
		MV_CP_WRITE(ep->doneq_cur + ep->doneq_start_phys,
			    ep_doneq_read(num, dir));
		spin_unlock_irqrestore(&ep->lock, flags);
	} else
		dir = _ep->desc->bEndpointAddress & USB_DIR_IN;

	ret = usb_gadget_map_request(&cp->gadget, &req->req, dir);
	if (ret)
		return ret;

	_req->actual = 0;
	_req->status = -EINPROGRESS;
	spin_lock_irqsave(&ep->lock, flags);

	if (dir == USB_DIR_OUT) {
		ret = alloc_bds(cp, ep, req);
		/* Current all bds have been allocated, just wait for previous complete */
		if (!ret)
			list_add_tail(&req->queue, &ep->wait);
		else
			mvc2_ring_incoming(cp, num, dir);
	} else {
		list_add_tail(&req->queue, &ep->tmp);
		cnt++;

		if (req->req.length > 1000 && cnt < CNT)
			goto out;
		if (cnt == CNT || req->req.length < 1000) {
			list_for_each_entry_safe(req, tmp, &ep->tmp, queue) {
				list_del_init(&req->queue);
				cnt--;
				if (cnt)
					last = 0;
				else
					last = 1;
#if 1
				ret = alloc_in_bds(cp, ep, req, &last);
#else
				ret = alloc_bds(cp, ep, req);
				/* Current all bds have been allocated, just wait for previous complete */
				if (!ret)
					list_add_tail(&req->queue, &ep->wait);
#endif
			}
			mvc2_ring_incoming(cp, num, dir);
		}
	}
out:
	spin_unlock_irqrestore(&ep->lock, flags);

	return 0;
}
#endif

/*
 * done() - retire a request; caller blocked irqs
 * @status : request status to be set, only works when
 * request is still in progress.
 */
static void done(struct mvc2_ep *ep, struct mvc2_req *req, int status)
{
	struct mvc2 *cp = NULL;

	cp = (struct mvc2 *)ep->cp;
	/* Removed the req from fsl_ep->queue */
	list_del_init(&req->queue);

	ep->left_bds += req->bd_total;
	WARN_ON(ep->left_bds > ep->bd_sz);

	/* req.status should be set as -EINPROGRESS in ep_queue() */
	if (req->req.status == -EINPROGRESS)
		req->req.status = status;
	else
		status = req->req.status;

	usb_gadget_unmap_request(&cp->gadget, &req->req, ep_dir(ep));

	if (status && (status != -ESHUTDOWN))
		dev_info(cp->dev, "complete %s req %p stat %d len %u/%u",
			 ep->ep.name, &req->req, status,
			 req->req.actual, req->req.length);

	spin_unlock(&ep->lock);
	/*
	 * complete() is from gadget layer,
	 * eg fsg->bulk_in_complete()
	 */
	if (req->req.complete)
		req->req.complete(&ep->ep, &req->req);

	spin_lock(&ep->lock);
}

static void ep_fifo_flush(struct mvc2 *cp, int num, int dir, int all)
{
	struct mvc2_ep *ep;

	ep = &cp->eps[2 * num + !!dir];
	/*
	 * Only current transferring bd would be transferred out,
	 * for those bd still chained after would be left untouched
	 */
	mvc2_dma_reset(cp, ep, num, dir);
}

/* dequeues (cancels, unlinks) an I/O request from an endpoint */
static int mvc2_ep_dequeue(struct usb_ep *_ep, struct usb_request *_req)
{
	struct mvc2_ep *ep = container_of(_ep, struct mvc2_ep, ep);
	struct mvc2 *cp = ep->cp;
	struct mvc2_req *req = container_of(_req, struct mvc2_req, req), *tmp;
	int num = _ep->desc->bEndpointAddress & USB_ENDPOINT_NUMBER_MASK;
	int dir = _ep->desc->bEndpointAddress & USB_DIR_IN;
	int type = _ep->desc->bmAttributes & USB_ENDPOINT_XFERTYPE_MASK;
	unsigned long flags;
	int ret, ring;

	ring = ret = 0;
	if (_ep == NULL || _req == NULL || list_empty(&req->queue))
		return -EINVAL;

	spin_lock_irqsave(&ep->lock, flags);
	ep_disable(cp, num, dir);

	list_for_each_entry(tmp, &ep->wait, queue)
	if (tmp == req)
		break;

	/* If don't find the request in both run/wait queue, quit */
	if (tmp != req) {
		ret = -EINVAL;
		goto out;
	}

	list_del_init(&req->queue);
	if (req->req.length)
		usb_gadget_unmap_request(&ep->cp->gadget, _req, dir);
	spin_unlock_irqrestore(&ep->lock, flags);
	if (req->req.complete) {
		req->req.status = -ECONNRESET;
		req->req.complete(&ep->ep, &req->req);
	}

	spin_lock_irqsave(&ep->lock, flags);
out:
	ep_enable(ep, num, dir, type);
	spin_unlock_irqrestore(&ep->lock, flags);

	return ret;
}

static int ep_set_halt(struct mvc2 *cp, int n, int in, int halt)
{
	unsigned int config, config_base;
	struct mvc2_ep *ep = &cp->eps[2 * n + !!in];
	unsigned long flags;
	int bulk;

	config_base = epcon(n, in);
	bulk = ep->ep.desc->bmAttributes & USB_ENDPOINT_XFERTYPE_MASK;

	spin_lock_irqsave(&ep->lock, flags);

	if (halt && (bulk == USB_ENDPOINT_XFER_BULK) && in
	    && !list_empty(&ep->queue)) {
		spin_unlock_irqrestore(&ep->lock, flags);
		return -EAGAIN;
	}

	config = MV_CP_READ(config_base);
	if (halt) {
		config |= MVCP_EP_STALL;
		if (n)
			ep->state |= MV_CP_EP_STALL;
	} else {
		config &= ~MVCP_EP_STALL;
		ep->state &= ~MV_CP_EP_STALL;
	}
	MV_CP_WRITE(config, config_base);
	spin_unlock_irqrestore(&ep->lock, flags);

	return 0;
}

static int mvc2_ep_set_halt(struct usb_ep *_ep, int halt)
{
	struct mvc2_ep *ep = container_of(_ep, struct mvc2_ep, ep);
	struct mvc2 *cp = ep->cp;
	unsigned int n, in;

	if (_ep == NULL || _ep->desc == NULL)
		return -EINVAL;

	if (usb_endpoint_xfer_isoc(_ep->desc))
		return -EOPNOTSUPP;

	if (!halt)
		ep->state &= ~MV_CP_EP_WEDGE;

	n = _ep->desc->bEndpointAddress & USB_ENDPOINT_NUMBER_MASK;
	in = _ep->desc->bEndpointAddress & USB_DIR_IN;

	return ep_set_halt(cp, n, in, halt);
}

static int mvc2_ep_set_wedge(struct usb_ep *_ep)
{
	struct mvc2_ep *ep = container_of(_ep, struct mvc2_ep, ep);

	ep->state |= MV_CP_EP_WEDGE;
	return mvc2_ep_set_halt(_ep, 1);
}

static void mvc2_ep_fifo_flush(struct usb_ep *_ep)
{
	struct mvc2_ep *ep = container_of(_ep, struct mvc2_ep, ep);
	struct mvc2 *cp = ep->cp;

	ep_fifo_flush(cp, ep->ep_num, ep_dir(ep), 1);
}

static struct usb_ep_ops mvc2_ep_ops = {
	.enable = mvc2_ep_enable,
	.disable = mvc2_ep_disable,

	.alloc_request = mvc2_alloc_request,
	.free_request = mvc2_free_request,

	.queue = mvc2_ep_queue,
	.dequeue = mvc2_ep_dequeue,

	.set_wedge = mvc2_ep_set_wedge,
	.set_halt = mvc2_ep_set_halt,
	.fifo_flush = mvc2_ep_fifo_flush,
};

/* delete all endpoint requests, called with spinlock held */
static void nuke(struct mvc2_ep *ep, int status)
{
	struct mvc2_req *req, *tmp;
	unsigned long flags;
	/* called with spinlock held */
	ep->stopped = 1;

	/* endpoint fifo flush */
	mvc2_ep_fifo_flush(&ep->ep);
	spin_lock_irqsave(&ep->lock, flags);
	list_for_each_entry_safe(req, tmp, &ep->queue, queue)
		done(ep, req, status);
	list_for_each_entry_safe(req, tmp, &ep->wait, queue)
		done(ep, req, status);
	spin_unlock_irqrestore(&ep->lock, flags);
}

/* stop all USB activities */
static void stop_activity(struct mvc2 *udc, struct usb_gadget_driver *driver)
{
	struct mvc2_ep *ep;

	nuke(&udc->eps[0], -ESHUTDOWN);
	nuke(&udc->eps[1], -ESHUTDOWN);

	list_for_each_entry(ep, &udc->gadget.ep_list, ep.ep_list) {
		if (ep->ep_num <= ip_ep_num(udc))
			nuke(ep, -ESHUTDOWN);
	}

	/* report disconnect; the driver is already quiesced */
	if (driver)
		driver->disconnect(&udc->gadget);
}

static void mvc2_init_interrupt(struct mvc2 *cp)
{
	int i;

	if (ip_ver(cp) <= USB3_IP_VER_Z2) {
		MV_CP_WRITE(~0, MVCP_DMA_COMPLETE_SUCCESS);
		MV_CP_WRITE(~0, MVCP_DMA_COMPLETE_ERROR);
		MV_CP_WRITE(~0, MVCP_SS_CORE_INT);
		MV_CP_WRITE(~0, MVCP_SS_SYS_INT);
		/*
		 * Don't clear the ref int status.
		 * Refer to the comments in mvc2_usb2_connect for details.
		 * val = MV_CP_READ(cp->reg->ref_int);
		 * MV_CP_WRITE(val, cp->reg->ref_int);
		 */
		MV_CP_WRITE(MVCP_SS_CORE_INTEN_SETUP
			    | MVCP_SS_CORE_INTEN_HOT_RESET
			    | MVCP_SS_CORE_INTEN_LTSSM_CHG, MVCP_SS_CORE_INTEN);
		MV_CP_WRITE(MVCP_SS_SYS_INTEN_DMA, MVCP_SS_SYS_INTEN);
		MV_CP_WRITE(MVCP_REF_INTEN_USB2_CNT
			    | MVCP_REF_INTEN_USB2_DISCNT
			    | MVCP_REF_INTEN_RESET
			    | MVCP_REF_INTEN_POWERON
			    | MVCP_REF_INTEN_POWEROFF
			    | MVCP_REF_INTEN_SUSPEND
			    | MVCP_REF_INTEN_RESUME, cp->reg->ref_inten);

		set_top_int(cp, 0xf);
	} else {
		MV_CP_WRITE(~0, MVCP_SS_CORE_INT);
		MV_CP_WRITE(~0, MVCP_TOP_INT_STATUS);
		MV_CP_WRITE(~0, SS_EP_TOP_INT_STATUS_REG);
		MV_CP_WRITE(~0, SS_EP_TOP_INT_ENABLE_REG);
		MV_CP_WRITE(~0, SS_AXI_INT_STATUS_REG);
		MV_CP_WRITE(~0, SS_AXI_INT_ENABLE_REG);

		/* enbale all interrupts of endpoints
		 * except doneq_full interrupt
		 */
		for (i = 0; i < cp->epnum; i++) {
			MV_CP_WRITE(~0, SS_IN_EP_INT_STATUS_REG(i));
			MV_CP_WRITE(~DONEQ_FULL, SS_IN_EP_INT_ENABLE_REG(i));
			MV_CP_WRITE(~0, SS_OUT_EP_INT_STATUS_REG(i));
			MV_CP_WRITE(~DONEQ_FULL, SS_OUT_EP_INT_ENABLE_REG(i));
		}
		/*
		 * Don't clear the ref int status.
		 * Refer to the comments in mvc2_usb2_connect for details.
		 * val = MV_CP_READ(cp->reg->ref_int);
		 * MV_CP_WRITE(val, cp->reg->ref_int);
		 */
		/* Since decode_err_8_10b & disparity_err will
		 * generate very frequenlty when enable U1/U2,
		 * we disable these two error interrupt
		 */
		MV_CP_WRITE(MVCP_SS_CORE_INTEN_SETUP
			    | MVCP_SS_CORE_INTEN_HOT_RESET
			    | MVCP_SS_CORE_INTEN_LTSSM_CHG
			    /*| 0x3F */, MVCP_SS_CORE_INTEN);
		MV_CP_WRITE(MVCP_REF_INTEN_USB2_CNT
			    | MVCP_REF_INTEN_USB2_DISCNT
			    | MVCP_REF_INTEN_RESET
			    | MVCP_REF_INTEN_POWERON
			    | MVCP_REF_INTEN_POWEROFF
			    | MVCP_REF_INTEN_SUSPEND
			    | MVCP_REF_INTEN_RESUME, cp->reg->ref_inten);

		set_top_int(cp, 0x4f);
	}
}

static int mvc2_pullup(struct usb_gadget *gadget, int is_on)
{
	struct mvc2 *cp = container_of(gadget, struct mvc2, gadget);
	unsigned int val;

	/*
	 * For every switch from 2.0 to 3.0, this dma global config
	 * and interrupt enable register would get reset
	 */
	if (is_on && !irq_enabled) {
		irq_enabled = true;
		enable_irq(cp->irq);
	}

	if (!usb3_disconnect)
		is_on = 1;

	mvc2_connect(cp, is_on);
	mvc2_config_mac(cp);
	val = MV_CP_READ(MVCP_DMA_GLOBAL_CONFIG);
	val |= MVCP_DMA_GLOBAL_CONFIG_RUN | MVCP_DMA_GLOBAL_CONFIG_INTCLR;
	MV_CP_WRITE(val, MVCP_DMA_GLOBAL_CONFIG);

	mvc2_init_interrupt(cp);

	if (is_on == 0)
		stop_activity(cp, cp->driver);
	return 0;
}

static int mvc2_start(struct usb_gadget *gadget,
		      struct usb_gadget_driver *driver)
{
	struct mvc2 *cp = container_of(gadget, struct mvc2, gadget);
	unsigned long flags;
	struct mvc2_ep *ep;

	cp->driver = driver;

	/* enable ep0, dma int */
	ep = &cp->eps[0];
	spin_lock_irqsave(&ep->lock, flags);
	ep_enable(ep, 0, 0, 0);
	spin_unlock_irqrestore(&ep->lock, flags);

	ep = &cp->eps[1];
	spin_lock_irqsave(&ep->lock, flags);
	ep_enable(ep, 0, 1, 0);

	spin_unlock_irqrestore(&ep->lock, flags);

	/* pullup is always on */
	mvc2_pullup(gadget, 1);

	return 0;
}

static int mvc2_first_start(struct usb_gadget *gadget,
			    struct usb_gadget_driver *driver)
{
	struct mvc2 *cp = container_of(gadget, struct mvc2, gadget);

	mvc2_start(gadget, driver);

	/* When boot with cable attached, there will be no vbus irq occurred */
	if (cp->qwork)
		queue_work(cp->qwork, &cp->vbus_work);

	return 0;
}

static int mvc2_stop(struct usb_gadget *gadget)
{
	struct mvc2 *cp = container_of(gadget, struct mvc2, gadget);

	cp->driver = NULL;
	return 0;
}

int mvc2_checkvbus(struct mvc2 *cp)
{
	int tmp;

	tmp = MV_CP_READ(cp->reg->global_control);
	return tmp & MVCP_GLOBAL_CONTROL_POWERPRESENT;
}

static int mvc2_vbus_session(struct usb_gadget *gadget, int is_active)
{
	struct mvc2 *cp = container_of(gadget, struct mvc2, gadget);
	unsigned int val;

	/* We only do real work when gadget driver is ready */
	if (!cp->driver)
		return -ENODEV;

	val = MV_CP_READ(MVCP_DMA_GLOBAL_CONFIG);
	if (is_active) {
		/* For Armada 3700, need to skip PHY HW reset */
		if (cp->phy_hw_reset)
			mvc2_hw_reset(cp);
		pm_stay_awake(cp->dev);
		/* turn on dma int */
		val |= MVCP_DMA_GLOBAL_CONFIG_RUN
		    | MVCP_DMA_GLOBAL_CONFIG_INTCLR;
		MV_CP_WRITE(val, MVCP_DMA_GLOBAL_CONFIG);
		usb_gadget_connect(&cp->gadget);
		mvc2_start(gadget, cp->driver);

	} else {
		/* need to stop activity before disable dma engine.
		 * stop_activity will call mvc2_dma_reset,
		 * if disable dma before mvc2_dma_reset, then dma reset
		 * timeout issue will happen.
		 */
		stop_activity(cp, cp->driver);

		/* disable dma engine */
		val &= ~MVCP_DMA_GLOBAL_CONFIG_RUN;
		MV_CP_WRITE(val, MVCP_DMA_GLOBAL_CONFIG);
	}

	return 0;
}

static irqreturn_t mvc2_vbus_irq(int irq, void *dev)
{
	struct mvc2 *cp = (struct mvc2 *)dev;

	/* polling VBUS and init phy may cause too much time */
	if (cp->qwork)
		queue_work(cp->qwork, &cp->vbus_work);

	return IRQ_HANDLED;
}

static void mvc2_vbus_work(struct work_struct *work)
{
	struct mvc2 *cp;
	unsigned int vbus;
	unsigned int reg;

	cp = container_of(work, struct mvc2, vbus_work);

	if (gpio_is_valid(cp->vbus_pin))
		vbus = gpio_get_value_cansleep(cp->vbus_pin);
	else {
		dev_err(cp->dev, "VBUS interrupt status is missing\n");
		return;
	}

	if (cp->prev_vbus != vbus)
		cp->prev_vbus = vbus;
	else
		return;

	if (!cp->phy_base) {
		dev_err(cp->dev, "PHY register is missing\n");
		return;
	}

	if (vbus == VBUS_HIGH) {
		reg = readl(cp->phy_base);
		reg |= 0x8000;
		writel(reg, cp->phy_base);
	} else if (vbus == VBUS_LOW) {
		reg = readl(cp->phy_base);
		reg &= ~0x8000;
		writel(reg, cp->phy_base);
	}
}

static int mvc2_vbus_draw(struct usb_gadget *gadget, unsigned mA)
{
	return -ENOTSUPP;
}

static int mvc2_set_selfpowered(struct usb_gadget *gadget, int is_selfpowered)
{
	struct mvc2 *cp = container_of(gadget, struct mvc2, gadget);

	if (is_selfpowered)
		cp->status |= MVCP_STATUS_SELF_POWERED;
	else
		cp->status &= ~MVCP_STATUS_SELF_POWERED;

	return 0;
}

#ifdef CONFIG_USB_REMOTE_WAKEUP

#define MVCP_GLOBAL_CONTROL_STATUS  0x2c
#define MVCP_GLOBAL_CONTROL_STATUS_LPFS_EXIT (1<<7)
static int mvc2_wakeup(struct usb_gadget *gadget)
{
	struct mvc2 *cp = container_of(gadget, struct mvc2, gadget);
	unsigned int phy, val;

	phy = MV_CP_READ(MVCP_PHY);
	if ((phy & MVCP_PHY_LTSSM_MASK) == LTSSM_U3) {
		dev_info(cp->dev, "usb3 is enter u3 , can be wakeup now\n");
		val = MV_CP_READ(MVCP_GLOBAL_CONTROL_STATUS);
		val |= MVCP_GLOBAL_CONTROL_STATUS_LPFS_EXIT;
		MV_CP_WRITE(val, MVCP_GLOBAL_CONTROL_STATUS);
	}

	return 0;
}
#endif

/* device controller usb_gadget_ops structure */
static const struct usb_gadget_ops mvc2_ops = {
	/* notify controller that VBUS is powered or not */
	.vbus_session = mvc2_vbus_session,

	/* constrain controller's VBUS power usage */
	.vbus_draw = mvc2_vbus_draw,
	.set_selfpowered = mvc2_set_selfpowered,

	.pullup = mvc2_pullup,
	.udc_start = mvc2_first_start,
	.udc_stop = mvc2_stop,
#ifdef CONFIG_USB_REMOTE_WAKEUP
	.wakeup = mvc2_wakeup,
#endif
};

void mvc2_handle_setup(struct mvc2 *cp)
{
	struct usb_ctrlrequest *r;
	unsigned int tmp[2];
	int ret = -EINVAL;
	bool delegate;

	tmp[0] = MV_CP_READ(MVCP_SETUP_DP_LOW);
	tmp[1] = MV_CP_READ(MVCP_SETUP_DP_HIGH);
	MV_CP_WRITE(MVCP_SETUP_CONTROL_FETCHED, MVCP_SETUP_CONTROL);

	r = (struct usb_ctrlrequest *)tmp;

	if (r->wLength) {
		if (r->bRequestType & USB_DIR_IN)
			cp->ep0_dir = USB_DIR_IN;
		else
			cp->ep0_dir = USB_DIR_OUT;
	} else
		cp->ep0_dir = USB_DIR_IN;

	ret = mvc2_std_request(cp, r, &delegate);
	if (delegate)
		ret = cp->driver->setup(&cp->gadget, r);
	/* indicate setup pharse already complete */
	mvc2_send_erdy(cp);

	/* Stall the endpoint if protocol not support */
	if (ret < 0)
		ep_set_halt(cp, 0, 0, 1);
	/*
	 * If current setup has no data pharse or failed, we would directly
	 * jump to status pharse.
	 * If the USB_GADGET_DELAYED_STATUS is set, the USB interface requests
	 * delay for it to handle the setup, thus here should not send erdy to
	 * continue the transfer. Instead, the erdy will be sent from mvc2_ep_queue,
	 * once a request with length 0 is issued.
	 */
	 if ((ret < 0) || (r->wLength == 0 && ret != USB_GADGET_DELAYED_STATUS))
		mvc2_send_erdy(cp);
}

static void mvc2_dma_complete(struct mvc2 *cp)
{
	unsigned int val, i, n, in, short_packet, finish, ret;
	struct doneq *done;
	struct mvc2_ep *ep;
	struct mvc2_req *req, *tmp;
	struct bd *bd;
	unsigned int writeq, len, doneq, ring;
	unsigned int sreg, ep_status;

	if (ip_ver(cp) <= USB3_IP_VER_Z2)
		sreg = MVCP_DMA_COMPLETE_SUCCESS;
	else
		sreg = SS_EP_TOP_INT_STATUS_REG;
	val = MV_CP_READ(sreg);
	if (!val)
		return;
	MV_CP_WRITE(val, sreg);

	for (i = 0; i < (cp->epnum << 1); i++) {
		if (!(val & (1 << i)))
			continue;

		if (i < cp->epnum) {
			n = i;
			in = 0;
		} else {
			n = i - cp->epnum;
			in = 1;
		}

		if (ip_ver(cp) >= USB3_IP_VER_Z3) {
			in = in ? 0 : 1;
			if (in)
				sreg = SS_IN_EP_INT_STATUS_REG(n);
			else
				sreg = SS_OUT_EP_INT_STATUS_REG(n);
			ep_status = MV_CP_READ(sreg);
			/* clear interrupt status */
			MV_CP_WRITE(ep_status, sreg);

			if (ep_status & COMPLETION_SUCCESS)
				goto success;

			/* some error may happen */
			pr_warn("### %s %d: num %d, dir %d, status 0x%x\n",
				__func__, __LINE__, n, in, ep_status);
			continue;
		}

success:
		ep = &cp->eps[(n << 1) + in];

		/*
		 * info hw that sw has prepared data
		 * hw would auto send erdy after data stage complete
		 */
		if (n == 0) {
			mvc2_send_erdy(cp);
			ep->state &= ~MV_CP_EP_TRANSERING;
			if (!list_empty(&ep->queue)) {
				req = list_first_entry(&ep->queue,
						       struct mvc2_req, queue);

				if (req->req.complete) {
					req->req.status = 0;
					req->req.complete(&ep->ep, &req->req);
				}
				ep->left_bds++;
				WARN_ON(req->bd_total > 1);
				WARN_ON(ep->left_bds > ep->bd_sz);
				spin_lock(&ep->lock);
				INIT_LIST_HEAD(&ep->queue);
				INIT_LIST_HEAD(&ep->wait);
				spin_unlock(&ep->lock);
			}

			continue;
		}

		writeq = MV_CP_READ(ep_doneq_write(n, in));
		if (!writeq)
			continue;

		/* Get the DoneQ write pointer relative position */
		writeq -= ep->doneq_start_phys;
		writeq /= sizeof(struct doneq);
		if (writeq == ep->bd_sz)
			writeq = 0;

		doneq = ep->doneq_cur;
		short_packet = 0;
		ring = 0;
		spin_lock(&ep->lock);
		while (doneq != writeq) {
			len = 0;
			req = list_first_entry_or_null(&ep->queue,
						       struct mvc2_req, queue);
			if (!req) {
				pr_info("req null, doneq = %d,writeq = %d\n",
					doneq, writeq);
				break;
			}
			bd = req->bd;
			finish = 1;
			do {
				done = (struct doneq *)(ep->doneq_start
							+ doneq);

				if (done->status & DONE_AXI_ERROR) {
					req->req.status = -EPROTO;
					break;
				}

				/*
				 * Note: for the short packet, if originally
				 * there are several BDs chained, but host
				 * only send short packet for the first BD,
				 * then doneq would be updated accordingly.
				 * And later BDs in the chain would be used
				 * for storing data that host send in another
				 * transfer.
				 *
				 * But if the first BD is not set as INT_EN,
				 * there would be no interrupt be generated.
				 */
				if (done->status & DONE_SHORT_PKT)
					short_packet = 1;

				len += DONE_LEN(done->status);
				doneq++;
				if (doneq == ep->bd_sz)
					doneq = 0;

				WARN_ON(doneq == (writeq + 1));
				bd->cmd = 0;
				bd++;
				ep->left_bds++;
				WARN_ON(ep->left_bds > ep->bd_sz);

			} while (--req->bd_total > 0);

			/*
			 * Ring the finish data handle bell
			 * to kick hardware to continue
			 */
			MV_CP_WRITE(doneq * 8 + ep->doneq_start_phys,
				    ep_doneq_read(n, in));
			ep->doneq_cur = doneq;

			req->req.actual += len;
			list_del_init(&req->queue);
			ep->state &= ~MV_CP_EP_TRANSERING;

			ret = UINT_MAX;
			/* There still something left not being transferred */
			if ((req->req.actual < req->req.length)
			    && !short_packet) {
				dev_dbg(cp->dev, "%s %d\n", __func__, __LINE__);
				ret = alloc_bds(cp, ep, req);
				finish = 0;
			}

			/*
			 * Refill BD if there is any request
			 * following in the chain
			 */
			while (!list_empty(&ep->wait) && ret) {
				tmp = list_first_entry(&ep->wait,
						       struct mvc2_req, queue);
				list_del_init(&tmp->queue);
				ret = alloc_bds(cp, ep, tmp);
				if (!ret)
					list_add(&tmp->queue, &ep->wait);
			}

			if (finish) {
				spin_unlock(&ep->lock);
				if (req->req.length)
					usb_gadget_unmap_request(&cp->gadget,
								 &req->req, in);

				if (req->req.complete) {
					req->req.status = 0;
					req->req.complete(&ep->ep, &req->req);
				}
				spin_lock(&ep->lock);
			}

			if (ret != UINT_MAX)
				ring = 1;
		}

		if (ring)
			mvc2_ring_incoming(cp, n, in);
		spin_unlock(&ep->lock);
	}
}

static void mvc2_process_link_change(struct mvc2 *cp)
{
	unsigned int val;

	cp->status &= ~MVCP_STATUS_POWER_MASK;
	val = MV_CP_READ(MVCP_PHY);
	switch (val & MVCP_PHY_LTSSM_MASK) {
	case LTSSM_U0:
		cp->gadget.speed = USB_SPEED_SUPER;
		cp->status |= MVCP_STATUS_U0;
		cp->status |= MVCP_STATUS_CONNECTED;
		break;
	case LTSSM_U1:
		cp->status |= MVCP_STATUS_U1;
		break;
	case LTSSM_U2:
		cp->status |= MVCP_STATUS_U2;
		break;
	case LTSSM_U3:
		cp->status |= MVCP_STATUS_U3;
		break;
	}
}

static irqreturn_t mvc2_irq(int irq, void *devid)
{
	struct mvc2 *cp = devid;
	unsigned int topint, coreint, sysint, refint, val;

	topint = MV_CP_READ(MVCP_TOP_INT_STATUS);

	if (topint == 0)
		return IRQ_HANDLED;

	MV_CP_WRITE(topint, MVCP_TOP_INT_STATUS);

	if (ip_ver(cp) <= USB3_IP_VER_Z2) {
		if (topint & MVCP_TOP_INT_SS_SYS) {
			sysint = MV_CP_READ(MVCP_SS_SYS_INT);
			MV_CP_WRITE(sysint, MVCP_SS_SYS_INT);

			if (sysint & MVCP_SS_SYS_INT_DMA)
				mvc2_dma_complete(cp);
		}
	} else {
		if (topint & MVCP_TOP_INT_SS_EP)
			mvc2_dma_complete(cp);

		if (topint & MVCP_TOP_INT_SS_AXI) {
			val = MV_CP_READ(SS_AXI_INT_STATUS_REG);
			MV_CP_WRITE(val, SS_AXI_INT_STATUS_REG);
			pr_warn("### %s %d: SS_AXI_INT_STATUS_REG = 0x%x\r\n",
				__func__, __LINE__, val);
		}
	}

	if (topint & MVCP_TOP_INT_SS_CORE) {
		coreint = MV_CP_READ(MVCP_SS_CORE_INT);
		MV_CP_WRITE(coreint, MVCP_SS_CORE_INT);

		if (coreint & MVCP_SS_CORE_INT_HOT_RESET) {
			pr_info("USB device: hot reset\n");
			stop_activity(cp, cp->driver);
		}

		if (coreint & MVCP_SS_CORE_INT_SETUP)
			mvc2_handle_setup(cp);

		if (coreint & MVCP_SS_CORE_INT_LTSSM_CHG)
			mvc2_process_link_change(cp);

		/* We enabled error interrupt from Z3,
		 * need to check the error here.
		 */
#if 0
		if (ip_ver(cp) >= USB3_IP_VER_Z3) {
			if (coreint & 0x3F)
				pr_warn("### coreint = 0x%x\n", coreint);
		}
#endif
	}

	if (topint & MVCP_TOP_INT_REF) {
		refint = MV_CP_READ(cp->reg->ref_int);
		MV_CP_WRITE(refint, cp->reg->ref_int);

		if (refint & MVCP_REF_INTEN_POWERON) {
			/*
			 * Note, during the USB3 irq disabled, there may be
			 * may times plug/unplug,
			 * thus, the power on/off interrupt
			 * may co-exisit once enable the irq again.
			 * To avoid this, we need to check
			 * the VBUS of the final state.
			 * Refer to mvc2_usb2_connect.
			 */
			if (mvc2_checkvbus(cp)) {
				pr_info("USB device: connected\n");
				usb_gadget_vbus_connect(&cp->gadget);
				cp->status |= MVCP_STATUS_CONNECTED;
			}
		}

		if (refint & MVCP_REF_INTEN_POWEROFF) {
			/*
			 * Note, during the USB3 irq disabled, there may be
			 * may times plug/unplug,
			 * thus, the power on/off interrupt
			 * may co-exisit once enable the irq again.
			 * To avoid this, we need to check
			 * the VBUS of the final state.
			 * Refer to mvc2_usb2_connect.
			 */
			if (!mvc2_checkvbus(cp)) {
				pr_info("USB device: disconnected\n");
				usb3_disconnect = true;
				usb_gadget_vbus_disconnect(&cp->gadget);
				cp->status &= ~MVCP_STATUS_CONNECTED;

				cp->gadget.speed = USB_SPEED_UNKNOWN;

				cp->status &= ~MVCP_STATUS_USB2;
				glue.status = cp->status;
				if (cp->work)
					schedule_work(cp->work);
			}
		}

		if (refint & MVCP_REF_INTEN_RESET) {
			pr_info("USB device: warm reset\n");
			/*
			 * The doneq write point will be set to 0 when warm/hot reset occurred.
			 * This will cause device abnormal, one example is CV test can't pass
			 * at this situation.
			 * Add dma reset here will set doneq write point to doneq start point.
			 */
			stop_activity(cp, cp->driver);
		}

		if ((refint & MVCP_REF_INTEN_USB2_CNT) &&
		    (MV_CP_READ(cp->reg->ref_inten) &
		     MVCP_REF_INTEN_USB2_CNT)) {
			usb3_disconnect = false;
			stop_activity(cp, cp->driver);

			cp->status |= MVCP_STATUS_USB2;
			glue.status = cp->status;
			if (cp->work)
				schedule_work(cp->work);
		}

		if ((refint & MVCP_REF_INTEN_USB2_DISCNT) &&
		    (MV_CP_READ(cp->reg->ref_inten) &
		     MVCP_REF_INTEN_USB2_DISCNT)) {
			usb3_disconnect = true;
			if (mvc2_checkvbus(cp)) {
				cp->status &= ~MVCP_STATUS_USB2;
				glue.status = cp->status;
				if (cp->work)
					schedule_work(cp->work);
			}
		}

		if (refint & MVCP_REF_INTEN_RESUME)
			pr_info("USB device: resume\n");

		if (refint & MVCP_REF_INTEN_SUSPEND)
			pr_info("USB device: suspend\n");
	}

	if (topint & MVCP_TOP_INT_USB2)
		return IRQ_NONE;

	return IRQ_HANDLED;
}

int mvc2_gadget_init(struct mvc2 *cp)
{
	int ret, i, irq;
	struct mvc2_ep *ep;

	irq = platform_get_irq(to_platform_device(cp->dev), 0);
	ret = request_irq(irq, mvc2_irq, IRQF_SHARED, "mvcp_usb3", cp);
	if (ret) {
		dev_err(cp->dev, "can't request irq %i, err: %d\n", irq, ret);
		return -EINVAL;
	}

	/* initialize gadget structure */
	cp->gadget.ops = &mvc2_ops;
	cp->gadget.ep0 = &cp->eps[0].ep;
	INIT_LIST_HEAD(&cp->gadget.ep_list);
	cp->gadget.speed = USB_SPEED_UNKNOWN;
	cp->gadget.max_speed = USB_SPEED_SUPER;
	cp->gadget.is_otg = 0;
	cp->gadget.name = driver_name;
	cp->gadget.dev.parent = cp->dev;
	cp->gadget.dev.dma_mask = cp->dev->dma_mask;
	cp->irq = irq;
	disable_irq(cp->irq);

	for (i = 0; i < cp->epnum * 2; i++) {
		ep = &cp->eps[i];
		ep->ep.ops = &mvc2_ep_ops;
		if (i > 1) {
			INIT_LIST_HEAD(&ep->ep.ep_list);
			list_add_tail(&ep->ep.ep_list, &cp->gadget.ep_list);
		}
	}

	ret = usb_add_gadget_udc(cp->dev, &cp->gadget);
	if (ret)
		return ret;

	return 0;
}

void mvc2_config_mac(struct mvc2 *cp)
{
	unsigned int val;

	/* NOTE: this setting is related to reference clock,
	 * it indicates number of ref clock pulses for 100 ns,
	 * refer to Q&A adjust 100ns timer
	 */
	val = MV_CP_READ(cp->reg->counter_pulse);
	val &= ~(0xff << 24);
	/* The formula is 2*100ns/(ref clcok speed) */
	val |= (5 << 24);
	MV_CP_WRITE(val, cp->reg->counter_pulse);

	/* set min value for transceiver side U1 tx_t12_t10 to 600ns
	 * set max value for transceiver side  U1 tx_t12_t11 to 900ns
	 * set LFPS Receive side t13 - t11 duration for U2  to 600us
	 * Receive side t13 - t11 duration for u3, set to 200us
	 */
	val = MV_CP_READ(lfps_signal(cp, 1));
	val &= ~(0x0f00000);
	val |= (0x0900000);
	MV_CP_WRITE(val, lfps_signal(cp, 1));
#if 0
	val = MV_CP_READ(lfps_signal(cp, 1));
	val &= ~(0xf8);
	val |= (0x30);
	MV_CP_WRITE(val, lfps_signal(cp, 1));
#endif
	val = MV_CP_READ(lfps_signal(cp, 1));
	val &= ~(0xff << 16);
	val |= (0x2 << 16);
	MV_CP_WRITE(val, lfps_signal(cp, 1));

	if (ip_ver(cp) < USB3_IP_VER_Z2) {
		/* IP version 2.04, 2.05 */
		val = MV_CP_READ(lfps_signal(cp, 1));
		val &= ~(0x7);
		val |= (0x3);
		MV_CP_WRITE(val, lfps_signal(cp, 1));

		val = MV_CP_READ(lfps_signal(cp, 2));
		val &= ~(0x7fff);
		val |= (0x4e20);
		MV_CP_WRITE(val, lfps_signal(cp, 2));

		val = MV_CP_READ(lfps_signal(cp, 4));
		val &= ~(0x7fff);
		val |= (0x7d0);
		MV_CP_WRITE(val, lfps_signal(cp, 4));

		MV_CP_WRITE(0x1388000d, lfps_signal(cp, 5));
	} else {
		/* IP version 2.06 above */
		/* Transmit side t11 - t10 duration for u2, max value set to 2ms  */
		val = MV_CP_READ(lfps_signal(cp, 2));
		val &= ~0xf;
		val |= 0x3;
		MV_CP_WRITE(val, lfps_signal(cp, 2));

		val = MV_CP_READ(lfps_signal(cp, 2));
		val &= ~(0xff << 16);
		val |= (0x6 << 16);
		MV_CP_WRITE(val, lfps_signal(cp, 2));

		/*Transmit side min value of t12 - t11 duration for u2, set to 100us */
		val = MV_CP_READ(lfps_signal(cp, 3));
		val &= ~0x7fff;
		val |= 0x4e20;
		MV_CP_WRITE(val, lfps_signal(cp, 3));

		val = MV_CP_READ(lfps_signal(cp, 3));
		val &= ~0x7fff;
		val |= 0x7d0;
		MV_CP_WRITE(val, lfps_signal(cp, 3));

		/*
		 * if U2 is disabled set U1 rx t13 - t11 to 900ns, if U2 is enabled,
		 * set U1 rx t13-t11 to 500us
		 */
		MV_CP_WRITE(0x13880009, lfps_signal(cp, 6));
	}

	/*reconfig LFPS length for PING to 70ns */
	val = MV_CP_READ(MVCP_TIMER_TIMEOUT(2));
	val &= ~0xff;
	val |= 0x50;
	MV_CP_WRITE(val, MVCP_TIMER_TIMEOUT(2));

	val = MV_CP_READ(MVCP_LFPS_TX_CONFIG);
	val &= ~0xf;
	val |= 0x3;
	MV_CP_WRITE(val, MVCP_LFPS_TX_CONFIG);

#ifdef ELECTRICAL_TEST
	/*set min_num_tx_ts1 to 131us, set min_num_tx_ts2 to 2us */
	val = MV_CP_READ(MVCP_TX_TSI_NUM);
	val |= 0x1000 << 16;
	MV_CP_WRITE(val, MVCP_TX_TSI_NUM);
#else
	/* for normal usage, 1us ts1 would be enough */
	val = MV_CP_READ(MVCP_TX_TSI_NUM);
	val |= 0x8 << 16;
	MV_CP_WRITE(val, MVCP_TX_TSI_NUM);
#endif
	val = MV_CP_READ(MVCP_START_STATE_DELAY);
	val |= 0x3e << 16;
	MV_CP_WRITE(val, MVCP_START_STATE_DELAY);

	val = MV_CP_READ(MVCP_TX_TSI_NUM);
	val |= 0xfff0;
	MV_CP_WRITE(val, MVCP_TX_TSI_NUM);

	if (u1u2_enabled()) {
		val = MV_CP_READ(MVCP_LOWPOWER);
		val &= ~0x3;
		MV_CP_WRITE(val, MVCP_LOWPOWER);
	}

	val = MV_CP_READ(MVCP_COUNTER_DELAY_TX);
	val |= 4 << 16;
	MV_CP_WRITE(val, MVCP_COUNTER_DELAY_TX);

	val = MV_CP_READ(MVCP_COUNTER_DELAY_TX);
	if (ip_ver(cp) <= USB3_IP_VER_Z3) {
		/*
		 * Jira NEZHA3-152/153
		 * Set the U2 Timeout Value bigger than the Max value(65024).
		 * This will make device never send LFPS.Exit, thus can
		 * avoid NEZHA3-152/153.
		 */
		val |= (65024 + 100);
	} else
		val |= 5;
	MV_CP_WRITE(val, MVCP_COUNTER_DELAY_TX);
}

/* Need to be included in ep lock protection */
void reset_seqencenum(struct mvc2_ep *ep, int num, int in)
{
	struct mvc2 *cp = ep->cp;
	unsigned int config;

	config = MV_CP_READ(epcon(num, in));
	config |= MVCP_EP_RESETSEQ;
	MV_CP_WRITE(config, epcon(num, in));
}

void mvc2_hw_reset(struct mvc2 *cp)
{
	unsigned int val, timeout = 5000;

	if (ip_ver(cp) < USB3_IP_VER_Z2) {
		val = MV_CP_READ(cp->reg->global_control);
		val |= MVCP_GLOBAL_CONTROL_SOFT_RESET;
		MV_CP_WRITE(val, cp->reg->global_control);
		/* wait controller reset complete */
		while (timeout-- > 0) {
			val = MV_CP_READ(cp->reg->global_control);
			if (!(val & MVCP_GLOBAL_CONTROL_SOFT_RESET))
				break;
			cpu_relax();
		}
	} else {
		val = MV_CP_READ(cp->reg->global_control);
		val |= MVCP_GLOBAL_CONTROL_PHYRESET;
		MV_CP_WRITE(val, cp->reg->global_control);

		val = MV_CP_READ(MVCP_SOFTWARE_RESET);
		val |= 1;
		MV_CP_WRITE(val, MVCP_SOFTWARE_RESET);
		while (timeout-- > 0) {
			val = MV_CP_READ(MVCP_SOFTWARE_RESET);
			if (!(val & 1))
				break;
			cpu_relax();
		}
	}

	/* delay before mac config */
	mdelay(100);
	mvc2_config_mac(cp);
}

void mvc2_usb2_operation(struct mvc2 *cp, int op)
{
	unsigned int val;

	if (op) {
		val = MV_CP_READ(cp->reg->global_control);
		val |= MVCP_GLOBAL_CONTROL_USB2_BUS_RESET;
		MV_CP_WRITE(val, cp->reg->global_control);
		udelay(10);
		val &= ~MVCP_GLOBAL_CONTROL_USB2_BUS_RESET;
		MV_CP_WRITE(val, cp->reg->global_control);
	}
}

void mvc2_connect(struct mvc2 *cp, int is_on)
{
	unsigned int val;

	if (is_on) {
		val = MV_CP_READ(cp->reg->global_control);
		/* bypass lowpower mode */
		val |= MVCP_GLOBAL_CONTROL_SAFE |
		    MVCP_GLOBAL_CONTROL_SOFT_CONNECT;
		MV_CP_WRITE(val, cp->reg->global_control);
	} else {
		val = MV_CP_READ(cp->reg->ref_inten);
		val &= ~MVCP_REF_INTEN_USB2_CNT;
		MV_CP_WRITE(val, cp->reg->ref_inten);

		val = MV_CP_READ(cp->reg->global_control);
		val &= ~MVCP_GLOBAL_CONTROL_SOFT_CONNECT;
		MV_CP_WRITE(val, cp->reg->global_control);
	}
}

static int mvc2_probe(struct platform_device *pdev)
{
	struct mvc2 *cp = NULL;
	struct resource *res;
	unsigned int ver;
	int ret = 0;
	void __iomem *base;
	void __iomem *phy_base = NULL;
	struct clk *clk;

	/* disable U1/U2 mode, as part of the detection WA */
	u1u2 = 0;

	/* private struct */
	cp = devm_kzalloc(&pdev->dev, sizeof(*cp), GFP_KERNEL);
	if (!cp)
		return -ENOMEM;

	/* a38x specific initializations */
	/* ungate unit clocks */
	clk = devm_clk_get(&pdev->dev, NULL);
	if (IS_ERR(clk)) {
		ret = PTR_ERR(clk);
		goto err_mem;
	}

	ret = clk_prepare_enable(clk);
	if (ret < 0)
		goto err_mem;

	/* phy address for VBUS toggling */
	res = platform_get_resource(pdev, IORESOURCE_MEM, 1);
	if (res) {
		phy_base = devm_ioremap(&pdev->dev, res->start, resource_size(res));
		if (!phy_base) {
			dev_err(&pdev->dev, "%s: register mapping failed\n", __func__);
			ret = -ENXIO;
			goto err_clk;
		}
	}

	/* general USB3 device initializations */
	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	if (!res) {
		dev_err(&pdev->dev, "missing mem resource\n");
		ret = -ENODEV;
		goto err_clk;
	}

	base = devm_ioremap_resource(&pdev->dev, res);
	if (!base) {
		dev_err(&pdev->dev, "%s: register mapping failed\n", __func__);
		ret = -ENXIO;
		goto err_clk;
	}

	ver = ioread32(base);
	if (ver == 0) {
		dev_err(&pdev->dev, "IP version error!\n");
		ret = -ENXIO;
		goto err_clk;
	}

	cp->mvc2_version = ver & 0xFFFF;

	/* setup vbus gpio */
	cp->vbus_pin = of_get_named_gpio(pdev->dev.of_node, "vbus-gpio", 0);
	if ((cp->vbus_pin == -ENODEV) || (cp->vbus_pin == -EPROBE_DEFER)) {
		ret = -EPROBE_DEFER;
		goto err_clk;
	}

	if (cp->vbus_pin < 0)
		cp->vbus_pin = -ENODEV;

	if (gpio_is_valid(cp->vbus_pin)) {
		cp->prev_vbus = 0;
		if (!devm_gpio_request(&pdev->dev, cp->vbus_pin, "mvebu-u3d")) {
			/* Use the 'any_context' version of function to allow
			 * requesting both direct GPIO interrupt (hardirq) and
			 * IO-expander's GPIO (nested interrupt)
			 */
			ret = devm_request_any_context_irq(&pdev->dev,
					       gpio_to_irq(cp->vbus_pin),
					       mvc2_vbus_irq,
					       IRQ_TYPE_EDGE_BOTH | IRQF_ONESHOT,
					       "mvebu-u3d", cp);
			if (ret < 0) {
				cp->vbus_pin = -ENODEV;
				dev_warn(&pdev->dev,
					 "failed to request vbus irq; "
					 "assuming always on\n");
			}
		}

		/* setup work queue */
		cp->qwork = create_singlethread_workqueue("mvc2_queue");
		if (!cp->qwork) {
			dev_err(&pdev->dev, "cannot create workqueue\n");
			ret = -ENOMEM;
			goto err_clk;
		}

		INIT_WORK(&cp->vbus_work, mvc2_vbus_work);
	}

	cp->reg = devm_kzalloc(&pdev->dev, sizeof(struct mvc2_register),
			       GFP_KERNEL);
	if (!cp->reg) {
		ret = -ENOMEM;
		goto err_qwork;
	}

	cp->dev = &pdev->dev;
	cp->base = base;
	cp->epnum = 16;
	if (phy_base)
		cp->phy_base = phy_base;

	if (cp->mvc2_version >= USB3_IP_VER_Z2) {
		cp->reg->lfps_signal = 0x8;
		cp->reg->counter_pulse = 0x20;
		cp->reg->ref_int = 0x24;
		cp->reg->ref_inten = 0x28;
		cp->reg->global_control = 0x2c;
	} else {
		cp->reg->lfps_signal = 0x4;
		cp->reg->counter_pulse = 0x18;
		cp->reg->ref_int = 0x1c;
		cp->reg->ref_inten = 0x20;
		cp->reg->global_control = 0x24;
	}

	/* For Armada 3700, need to skip PHY HW reset */
	if (of_device_is_compatible(pdev->dev.of_node,
				    "marvell,armada3700-u3d"))
		cp->phy_hw_reset = false;
	else
		cp->phy_hw_reset = true;

	/* Get comphy and init if there is */
	cp->comphy = devm_of_phy_get(&pdev->dev, pdev->dev.of_node, "usb");
	if (!IS_ERR(cp->comphy)) {
		ret = phy_init(cp->comphy);
		if (ret)
			goto disable_phy;

		ret = phy_power_on(cp->comphy);
		if (ret) {
			phy_exit(cp->comphy);
			goto disable_phy;
		}
	}

	spin_lock_init(&cp->lock);

	/* init irq status */
	irq_enabled = false;

	cp->eps = kzalloc(cp->epnum * sizeof(struct mvc2_ep) * 2, GFP_KERNEL);
	if (!cp->eps) {
		ret = -ENOMEM;
		goto err_qwork;
	}

	ret = mvc2_gadget_init(cp);
	if (ret < 0)
		goto err_alloc_eps;

	eps_init(cp);

	if (cp->phy_hw_reset)
		mvc2_hw_reset(cp);

	dev_set_drvdata(cp->dev, cp);
	dev_info(cp->dev, "Detected ver %x from Marvell Central IP.\n", ver);

	return 0;

err_alloc_eps:
	kfree(cp->eps);
err_qwork:
	if (cp->qwork)
		destroy_workqueue(cp->qwork);
disable_phy:
	if (cp->comphy) {
		phy_power_off(cp->comphy);
		phy_exit(cp->comphy);
	}
err_clk:
	clk_disable_unprepare(cp->clk);
err_mem:
	devm_kfree(&pdev->dev, cp);
	return ret;
}

#ifdef CONFIG_PM
static int mvc2_suspend(struct device *dev)
{
	struct mvc2 *cp = (struct mvc2 *)dev_get_drvdata(dev);

	/* Stop the current activities */
	if (cp->driver)
		stop_activity(cp, cp->driver);

	/* PHY exit if there is */
	if (cp->comphy) {
		phy_power_off(cp->comphy);
		phy_exit(cp->comphy);
	}

	return 0;
}

static int mvc2_resume(struct device *dev)
{
	struct mvc2 *cp = (struct mvc2 *)dev_get_drvdata(dev);
	int ret;

	/* PHY init if there is */
	if (cp->comphy) {
		ret = phy_init(cp->comphy);
		if (ret)
			return ret;

		ret = phy_power_on(cp->comphy);
		if (ret) {
			phy_power_off(cp->comphy);
			phy_exit(cp->comphy);
			return ret;
		}
	}

	/*
	 * USB device will be started only in mvc2_complete, once all other
	 * required device drivers have been resumed.
	 * This is done to avoid a state which U3D driver is resumed too early
	 * before mass storage thread has been resumed, which will lead to USB
	 * transfer time out.
	*/

	return 0;
}

/*
 * The PM core executes complete() callbacks after it has executed
 * the appropriate resume callbacks for all device drivers.
 * This routine enables USB3 irq in device mode, later on the USB device will be started
 * once it receives VBUS on interrupt, which starts USB device by enabling EP, and starts
 * the USB transfer between host and device.
 * Later on the USB mass storage function thread will be resumed, which will finish the
 * USB transfer to let the USB device continue to work after resume.
 * If start the USB device in "resume" operation, some device resuming after USB device
 * resuming might take long time, which leads to USB transfer time out.
 */
static void mvc2_complete(struct device *dev)
{
	struct mvc2 *cp = (struct mvc2 *)dev_get_drvdata(dev);

	/* Re-enable USB3 device irq */
	mvc2_init_interrupt(cp);
}

static const struct dev_pm_ops mvc2_pm_ops = {
	.suspend = mvc2_suspend,
	.resume = mvc2_resume,
	.complete = mvc2_complete
};
#endif

static int mvc2_remove(struct platform_device *dev)
{
	struct mvc2 *cp;

	cp = (struct mvc2 *)platform_get_drvdata(dev);
	mvc2_connect(cp, 0);

	if (cp->qwork) {
		flush_workqueue(cp->qwork);
		destroy_workqueue(cp->qwork);
	}

	/* PHY exit if there is */
	if (cp->comphy) {
		phy_power_off(cp->comphy);
		phy_exit(cp->comphy);
	}

	return 0;
}

static void mvc2_shutdown(struct platform_device *dev)
{
}

static const struct of_device_id mv_usb3_dt_match[] = {
	{.compatible = "marvell,mvebu-u3d"},
	{.compatible = "marvell,armada3700-u3d"},
	{},
};

MODULE_DEVICE_TABLE(of, mv_usb3_dt_match);

static struct platform_driver mvc2_driver = {
	.probe = mvc2_probe,
	.remove = mvc2_remove,
	.shutdown = mvc2_shutdown,
	.driver = {
		   .name = "mvebu-u3d",
#ifdef CONFIG_OF
		   .of_match_table = of_match_ptr(mv_usb3_dt_match),
#endif
#ifdef CONFIG_PM
		   .pm = &mvc2_pm_ops,
#endif
		   },
};

module_platform_driver(mvc2_driver);
MODULE_ALIAS("platform:mvc2");
MODULE_DESCRIPTION(DRIVER_DESC);
MODULE_AUTHOR("Lei Wen <leiwen@marvell.com>");
MODULE_LICENSE("GPL");
