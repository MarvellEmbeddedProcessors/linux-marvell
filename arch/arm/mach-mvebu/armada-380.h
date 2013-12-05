/*
 * Generic definitions for Marvell Armada 380 SoCs
 *
 * Copyright (C) 2013 Marvell
 *
 * Thomas Petazzoni <thomas.petazzoni@free-electrons.com>
 *
 * This file is licensed under the terms of the GNU General Public
 * License version 2.  This program is licensed "as is" without any
 * warranty of any kind, whether express or implied.
 */

#ifndef __MACH_ARMADA_380_H
#define __MACH_ARMADA_380_H

#ifdef CONFIG_SMP
#define ARMADA_380_MAX_CPUS 2
extern struct smp_operations armada_380_smp_ops;
#endif

#endif /* __MACH_ARMADA_380_H */
