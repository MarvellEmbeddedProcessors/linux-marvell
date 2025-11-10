// SPDX-License-Identifier: GPL-2.0
/*
 * Driver extension for Marvell User-space SDK.
 *
 * Copyright (C) 2024 Marvell
 *
 * Wilson Ding <dingwei@marvell.com>
 */

#include <linux/align.h>
#include <linux/netdevice.h>

#include "mvpp2.h"
#include "mvpp2_musdk.h"

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
