/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * Copyright (C) 2026 Arm Ltd.
 */

#ifndef __ASM_CPU_ERRATA_H
#define __ASM_CPU_ERRATA_H

#include <asm/cputype.h>

struct arm64_erratum {
	u64			erratum_num;
	struct midr_range	range;
};

static inline const struct arm64_erratum *
is_midr_in_erratum_list(const struct arm64_erratum *erratum_list)
{
	const struct arm64_erratum *erratum = erratum_list;
	u32 midr = read_cpuid_id();

	while (erratum->range.model) {
		if (is_midr_in_range(midr, &erratum->range))
			return erratum;
		erratum++;
	}
	return NULL;
}

static inline const struct arm64_erratum *
is_erratum_in_erratum_list(const struct arm64_erratum *erratum_list,
			   u64 midr_el1, u64 erratum_num)
{
	const struct arm64_erratum *erratum = erratum_list;

	while (erratum->range.model) {
		if (is_midr_in_range(midr_el1, &erratum->range) &&
		    erratum_num == erratum->erratum_num)
			return erratum;
		erratum++;
	}
	return NULL;
}

extern const struct arm64_erratum erratum_bad_tc_tlb_cpus[];

void cpu_disable_bad_tc_tlb(void);

#endif
