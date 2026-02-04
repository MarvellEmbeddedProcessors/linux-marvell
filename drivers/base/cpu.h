/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (C) 2026 ARM Ltd.
 *
 * Pre-processor hook for per-architecture definition of vulnerability files
 *
 */
#include <asm/cpu.h>

#ifndef ARCH_CPU_VULN_DEF
#define ARCH_CPU_VULN_DEF
#endif
#ifndef ARCH_CPU_VULN_ATTR
#define ARCH_CPU_VULN_ATTR
#endif
#ifndef ARCH_CPU_VULN_ENTRY
#define ARCH_CPU_VULN_ENTRY
#endif
