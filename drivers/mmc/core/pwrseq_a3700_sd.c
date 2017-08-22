/*
 * Driver for Marvell armada3700 SD power sequence
 *
 * Copyright (C) 2017 Marvell, All Rights Reserved.
 *
 * Author:      Zhoujie Wu <zjwu@marvell.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation version 2.
 *
 */

#include <linux/delay.h>
#include <linux/kernel.h>
#include <linux/slab.h>
#include <linux/device.h>
#include <linux/err.h>
#include <linux/of_gpio.h>
#include <linux/gpio/consumer.h>

#include <linux/mmc/host.h>

#include "pwrseq.h"

struct mmc_pwrseq_a3700 {
	struct mmc_pwrseq pwrseq;
	struct gpio_desc *pwren_gpio;
};

static void mmc_pwrseq_a3700_pre_power_on(struct mmc_host *host)
{
	struct mmc_pwrseq_a3700 *pwrseq = container_of(host->pwrseq,
					struct mmc_pwrseq_a3700, pwrseq);

	gpiod_set_value_cansleep(pwrseq->pwren_gpio, 0);
	msleep(50);
}

static void mmc_pwrseq_a3700_post_power_on(struct mmc_host *host)
{
	struct mmc_pwrseq_a3700 *pwrseq = container_of(host->pwrseq,
					struct mmc_pwrseq_a3700, pwrseq);

	gpiod_set_value_cansleep(pwrseq->pwren_gpio, 1);
}

static void mmc_pwrseq_a3700_free(struct mmc_host *host)
{
	struct mmc_pwrseq_a3700 *pwrseq = container_of(host->pwrseq,
					struct mmc_pwrseq_a3700, pwrseq);

	gpiod_put(pwrseq->pwren_gpio);
	kfree(pwrseq);
}

static struct mmc_pwrseq_ops mmc_pwrseq_a3700_ops = {
	.pre_power_on = mmc_pwrseq_a3700_pre_power_on,
	.post_power_on = mmc_pwrseq_a3700_post_power_on,
	.free = mmc_pwrseq_a3700_free,
};

struct mmc_pwrseq *mmc_pwrseq_a3700_alloc(struct mmc_host *host,
					   struct device *dev)
{
	struct mmc_pwrseq_a3700 *pwrseq;
	int ret = 0;

	pwrseq = kzalloc(sizeof(*pwrseq), GFP_KERNEL);
	if (!pwrseq)
		return ERR_PTR(-ENOMEM);

	pwrseq->pwren_gpio = gpiod_get(dev, "pwren", GPIOD_OUT_HIGH);
	if (IS_ERR(pwrseq->pwren_gpio)) {
		ret = PTR_ERR(pwrseq->pwren_gpio);
		goto free;
	}

	pwrseq->pwrseq.ops = &mmc_pwrseq_a3700_ops;

	return &pwrseq->pwrseq;
free:
	kfree(pwrseq);
	return ERR_PTR(ret);
}
