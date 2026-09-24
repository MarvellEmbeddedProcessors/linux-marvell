// SPDX-License-Identifier: GPL-2.0
/*
 * Driver extension for Marvell User-space SDK.
 *
 * Copyright (C) 2024 Marvell
 *
 * Wilson Ding <dingwei@marvell.com>
 */

#include <linux/align.h>
#include <linux/atomic.h>
#include <linux/netdevice.h>
#include <linux/platform_device.h>
#include <linux/of.h>
#include <linux/uio_driver.h>

#include "mvpp2.h"
#include "mvpp2_musdk.h"

static const char * const mvpp2_uio_legacy_map_names[] = {
	"pp",
	"mspg",
	"cm3",
};

struct mvpp2_musdk_saved_ctx {
	unsigned int nqvecs;
	unsigned int nrxqs;
	unsigned int ntxqs;
	int mtu;
	bool rxhash_en;
};

struct mvpp2_musdk_port {
	struct mvpp2_port *parent;
	struct mvpp2_musdk_saved_ctx saved_ctx;
	bool enabled;
};

static inline void *mvpp2_musdk_port(const struct mvpp2_port *port)
{
	return (char *)port + ALIGN(sizeof(struct mvpp2_port), NETDEV_ALIGN);
}

size_t mvpp2_musdk_port_priv_size(void)
{
	return sizeof(struct mvpp2_musdk_port);
}

void mvpp2_musdk_port_init(struct mvpp2_port *port)
{
	struct mvpp2_musdk_port *musdk_port = mvpp2_musdk_port(port);

	musdk_port->parent = port;
}

static void mvpp2_musdk_ctx_save(struct mvpp2_port *port)
{
	struct mvpp2_musdk_port *musdk_port = mvpp2_musdk_port(port);
	struct net_device *dev = port->dev;

	musdk_port->saved_ctx.nqvecs = port->nqvecs;
	musdk_port->saved_ctx.nrxqs = port->nrxqs;
	musdk_port->saved_ctx.ntxqs = port->ntxqs;
	musdk_port->saved_ctx.mtu = dev->mtu;
	musdk_port->saved_ctx.rxhash_en = !!(dev->hw_features & NETIF_F_RXHASH);

	port->nqvecs = 0;
	port->nrxqs = 0;
	port->ntxqs = 0;
}

static void mvpp2_musdk_ctx_restore(struct mvpp2_port *port)
{
	struct mvpp2_musdk_port *musdk_port = mvpp2_musdk_port(port);

	port->nqvecs = musdk_port->saved_ctx.nqvecs;
	port->nrxqs = musdk_port->saved_ctx.nrxqs;
	port->ntxqs = musdk_port->saved_ctx.ntxqs;
}

int mvpp2_musdk_port_enable(struct mvpp2_port *port, mvpp2_musdk_port_cb_t cb)
{
	struct mvpp2 *priv = port->priv;
	struct mvpp2_musdk_port *musdk_port = mvpp2_musdk_port(port);
	struct net_device *dev = port->dev;
	int err;

	if (priv->percpu_pools) {
		netdev_err(dev, "MUSDK mode not supported with percpu page pools\n");
		return -EINVAL;
	}

	if (musdk_port->enabled) {
		netdev_err(dev, "Port already in MUSDK mode\n");
		return -EBUSY;
	}

	/* Disable Queues and IntVec allocations for MUSDK,
	 * but save original values.
	 */
	mvpp2_musdk_ctx_save(port);

	if (musdk_port->saved_ctx.rxhash_en) {
		dev->hw_features &= ~NETIF_F_RXHASH;
		netdev_update_features(dev);
	}

	if (cb) {
		err = cb(port);
		if (err) {
			netdev_err(dev, "Failed to enable MUSDK port\n");
			return err;
		}
	}

	musdk_port->enabled = true;

	return 0;
}

int mvpp2_musdk_port_disable(struct mvpp2_port *port, mvpp2_musdk_port_cb_t cb)
{
	struct mvpp2_musdk_port *musdk_port = mvpp2_musdk_port(port);
	struct net_device *dev = port->dev;
	int err;

	if (!musdk_port->enabled) {
		netdev_err(dev, "Port not in MUSDK mode\n");
		return -EINVAL;
	}

	/* Back to Kernel mode */
	mvpp2_musdk_ctx_restore(port);

	if (musdk_port->saved_ctx.rxhash_en) {
		dev->hw_features |= NETIF_F_RXHASH;
		netdev_update_features(dev);
	}

	if (cb) {
		err = cb(port);
		if (err) {
			netdev_err(dev, "Failed to disable MUSDK port\n");
			return err;
		}
	}

	musdk_port->enabled = false;

	return 0;
}

/**
 * mvpp2_musdk_uio_init - Register a UIO device for MUSDK userspace access.
 *
 * Eliminates the need for the off-tree mv_pp_uio module and any additional
 * DTS node by reusing the memory resources already declared for the mvpp2
 * platform device. The UIO device keeps the legacy DT node name, such as
 * "cp0-pp-uio@0", so existing userspace discovery still works.
 */
int mvpp2_musdk_uio_init(struct platform_device *pdev, struct mvpp2 *priv)
{
	struct device *dev = &pdev->dev;
	struct device_node *node;
	struct uio_info *uio;
	struct resource *res;
	char *uio_name = NULL;
	resource_size_t start;
	int i, err;

	uio = devm_kzalloc(dev, sizeof(*uio), GFP_KERNEL);
	if (!uio)
		return -ENOMEM;

	for (node = dev_of_node(dev); node; node = of_get_parent(node)) {
		const char *name = node->name;

		if (name && name[0] == 'c' && name[1] == 'p' &&
		    name[2] >= '0' && name[2] <= '9') {
			unsigned int cp_id = 0;
			const char *p = name + 2;

			while (*p >= '0' && *p <= '9') {
				cp_id = cp_id * 10 + (*p - '0');
				p++;
			}

			if (*p == '\0') {
				uio_name = devm_kasprintf(dev, GFP_KERNEL,
						      "cp%u-pp-uio@0", cp_id);
				break;
			}
		}
	}

	if (!uio_name) {
		res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
		if (res) {
			start = res->start;
			if (start >= 0xf2000000ULL && start < 0xf8000000ULL &&
			    !((start - 0xf2000000ULL) % 0x02000000ULL)) {
				uio_name = devm_kasprintf(dev, GFP_KERNEL,
						      "cp%llu-pp-uio@0",
						      (unsigned long long)
						      ((start - 0xf2000000ULL) /
						       0x02000000ULL));
			}
		}
	}

	if (!uio_name)
		uio_name = devm_kstrdup(dev, "mv_pp_uio", GFP_KERNEL);

	if (!uio_name)
		return -ENOMEM;

	uio->name = uio_name;

	uio->version = "1.0";

	/* Map every IORESOURCE_MEM region that the mvpp2 platform device
	 * already owns. MUSDK needs direct access to these same regions.
	 */
	for (i = 0; i < MAX_UIO_MAPS; i++) {
		res = platform_get_resource(pdev, IORESOURCE_MEM, i);
		if (!res)
			break;
		uio->mem[i].memtype = UIO_MEM_PHYS;
		uio->mem[i].addr    = res->start & PAGE_MASK;
		uio->mem[i].size    = PAGE_ALIGN(resource_size(res));
		if (i < ARRAY_SIZE(mvpp2_uio_legacy_map_names))
			uio->mem[i].name = mvpp2_uio_legacy_map_names[i];
		else
			uio->mem[i].name = res->name;
	}

	if (i == 0) {
		dev_warn(dev, "mvpp2 UIO: no memory regions found\n");
		return -ENODEV;
	}

	err = uio_register_device(dev, uio);
	if (err) {
		dev_err(dev, "mvpp2 UIO: failed to register device: %d\n", err);
		return err;
	}

	priv->uio = uio;
	dev_info(dev, "mvpp2 UIO: registered '%s' with %d memory map(s)\n",
		 uio->name, i);
	return 0;
}

/**
 * mvpp2_musdk_uio_exit - Unregister the MUSDK UIO device.
 */
void mvpp2_musdk_uio_exit(struct mvpp2 *priv)
{
	if (priv->uio) {
		uio_unregister_device(priv->uio);
		priv->uio = NULL;
	}
}
