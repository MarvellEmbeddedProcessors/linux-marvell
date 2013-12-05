/*
 * Copyright (C) 2013 Marvell
 *
 * Gregory Clement <gregory.clement@free-electrons.com>
 *
 * This file is licensed under the terms of the GNU General Public
 * License version 2.  This program is licensed "as is" without any
 * warranty of any kind, whether express or implied.
 */

#ifndef __LINUX_XHCI_MVEBU_H
#define __LINUX_XHCI_MVEBU_H

#ifdef CONFIG_USB_XHCI_MVEBU
int xhci_mvebu_probe(struct platform_device *pdev);
int xhci_mvebu_remove(struct platform_device *pdev);
#else
#define xhci_mvebu_probe NULL
#define xhci_mvebu_remove NULL
#endif
#endif /* __LINUX_XHCI_MVEBU_H */
