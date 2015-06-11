/*
 * Copyright (C) 2013 Marvell International Ltd. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms and conditions of the GNU General Public License,
 * version 2, as published by the Free Software Foundation.
 */

#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/clk.h>
#include <linux/delay.h>
#include <linux/irq.h>
#include <linux/err.h>
#include <linux/io.h>
#include <linux/interrupt.h>
#include <linux/usb/gadget.h>
#include <linux/pm.h>
#include <linux/pm_qos.h>
#include <linux/usb/composite.h>

#include "mvebu_u3d.h"

#define CONNECTION_MAX_NUM    3

struct mvc2_glue glue;
static struct work_struct glue_work;
static DEFINE_MUTEX(work_lock);
struct usb_udc *udc_detect(struct list_head *udc_list,
			   struct usb_gadget_driver *driver)
{
	struct usb_udc *udc20, *udc30, *udc;
	struct mvc2 *cp;

	udc20 = udc30 = NULL;
	list_for_each_entry(udc, udc_list, list) {
		if (strncmp(udc->gadget->name, "mv_udc", 6) == 0)
			udc20 = udc;

		if (strncmp(udc->gadget->name, "mvebu-u3d", 9) == 0)
			udc30 = udc;
	}

	/* We need at least 3.0 controller driver being installed! */
	if (!udc30) {
		pr_err("Failed to detect usb3 device!\n");
		return NULL;
	}

	cp = container_of(udc30->gadget, struct mvc2, gadget);
	cp->work = &glue_work;
	glue.u20 = udc20;
	glue.u30 = udc30;

	if (glue.usb2_connect)
		return udc20;
	else
		return udc30;
}

void mvc2_usb2_connect(void)
{
	struct mvc2 *cp;
	struct usb_udc *u30 = glue.u30;
	struct usb_gadget_driver *driver = u30->driver;
	struct usb_gadget *u3d = u30->gadget;

	cp = container_of(u3d, struct mvc2, gadget);
	pr_info("USB device: USB2.0 connected\n");
	/*
	 * add de-bounce for usb cable plug
	 */
	msleep(200);
	if (mvc2_checkvbus(cp) == 0) {
		pr_info("USB device: power off\n");
		return;
	}

	/*
	 * The de-bounce time added before just can filter
	 * most cases but not all.
	 * The power off interrupt still has chance to break
	 * this workqueue.
	 * So we disable the USB3 irq here to guarantee this
	 * workqueue will not be interrupted by USB3 interrupt anymore,
	 * such as the power off interrupt, until all of the works have
	 * been done.
	 * The power off interrupt may happen when
	 * the USB3 irq was disabled.
	 * We hope this interrupt still there once
	 * we enabled USB3 irq again.
	 * To achieve this, need to keep the corresponding
	 * interrupt status(refer to mvc2_pullup,
	 * don't clear the ref int status register).
	 * Note, during the USB3 irq disabled, there may be
	 * may times plug/unplug, thus, the power on/off interrupt
	 * may co-exisit once enable the irq again.
	 * To avoid this, we need to check the VBUS of the final state,
	 * please refer to mvc2_irq.
	 */

	disable_irq(cp->irq);

	glue.usb2_connect = 1;
	usb_gadget_unregister_driver(driver);
	usb_gadget_probe_driver(driver);

	enable_irq(cp->irq);

}

void mvc2_usb2_disconnect(void)
{
	struct mvc2 *cp;
	struct usb_udc *u30 = glue.u30;
	struct usb_gadget *u3d = u30->gadget;
	struct usb_udc *u20 = glue.u20;
	struct usb_gadget_driver *driver = u20->driver;
	int has_setup = 0;

	cp = container_of(u3d, struct mvc2, gadget);

	if (u20->driver)
		driver = u20->driver;
	else if (u30->driver) {
		driver = u30->driver;
		return;
	}

	pr_info("USB device: USB2.0 disconnected\n");
	glue.usb2_connect = 0;
	usb_gadget_unregister_driver(driver);
	disable_irq(cp->irq);
	usb3_disconnect = false;

	if (ioread32(cp->base + MVCP_SS_CORE_INT) & MVCP_SS_CORE_INT_SETUP)
		has_setup = 1;
	usb_gadget_probe_driver(driver);
	usb3_disconnect = true;
	enable_irq(cp->irq);
	if (has_setup)
		mvc2_handle_setup(cp);

}

static int
u20_status_change(struct notifier_block *this, unsigned long event, void *ptr)
{
	struct mvc2 *cp;
	struct usb_gadget *u30 = glue.u30->gadget;

	cp = container_of(u30, struct mvc2, gadget);

	mvc2_usb2_operation(cp, event);

	return NOTIFY_DONE;
}

static struct notifier_block u20_status = {
	.notifier_call = u20_status_change,
};

void mv_connect_work(struct work_struct *work)
{
	struct mvc2 *cp;
	struct usb_gadget *u30 = glue.u30->gadget;

	cp = container_of(u30, struct mvc2, gadget);

	mutex_lock(&work_lock);

	if (glue.status & MVCP_STATUS_USB2)
		mvc2_usb2_connect();
	else
		mvc2_usb2_disconnect();

	mutex_unlock(&work_lock);
}

static int __init mvc2_glue_init(void)
{
	glue.u20 = glue.u30 = NULL;
	glue.usb2_connect = 0;
	mv_udc_register_status_notify(&u20_status);
	INIT_WORK(&glue_work, mv_connect_work);
	return 0;
}

device_initcall(mvc2_glue_init);
