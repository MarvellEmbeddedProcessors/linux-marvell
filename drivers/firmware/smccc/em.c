// SPDX-License-Identifier: GPL-2.0-only
/*
 * Arm Errata Management firmware interface.
 *
 * This firmware interface advertises support for firmware mitigations for CPU
 * errata. It can also be used to discover erratum where the 'configurations
 * affected' depends on the integration.
 *
 * Copyright (C) 2022 ARM Limited
 */

#define pr_fmt(fmt) "arm_smccc_em: " fmt

#include <linux/arm-smccc.h>
#include <linux/errno.h>
#include <linux/printk.h>

#include <asm/alternative.h>

#include <uapi/linux/psci.h>

static bool em_features_supported;
static bool em_workaround_config_supported;

int arm_smccc_em_cpu_workaround_config(u32 erratum_id, u64 midr_el1, u64 arg0)
{
	struct arm_smccc_res res;

	if (!READ_ONCE(em_workaround_config_supported))
		return -EOPNOTSUPP;

	arm_smccc_1_1_invoke(ARM_SMCCC_EM_CPU_WORKAROUND_CONFIG, erratum_id,
			     midr_el1, arg0, &res);
	switch (res.a0) {
	case SMCCC_RET_NOT_SUPPORTED:
		return -EOPNOTSUPP;
	case SMCCC_EM_RET_INVALID_PARAMTER:
		return -EINVAL;
	case SMCCC_EM_RET_UNKNOWN:
		return -ENOENT;
	};

	if ((signed long)res.a0 >= 0)
		return res.a0;
	return -EIO;
}

int arm_smccc_em_cpu_features(u32 erratum_id)
{
	struct arm_smccc_res res;

	if (!READ_ONCE(em_features_supported))
		return -EOPNOTSUPP;

	arm_smccc_1_1_invoke(ARM_SMCCC_EM_CPU_ERRATUM_FEATURES, erratum_id, 0, &res);
	switch (res.a0) {
	case SMCCC_RET_NOT_SUPPORTED:
		return -EOPNOTSUPP;
	case SMCCC_EM_RET_INVALID_PARAMTER:
		return -EINVAL;
	case SMCCC_EM_RET_UNKNOWN:
		return -ENOENT;
	case SMCCC_EM_RET_HIGHER_EL_MITIGATION:
	case SMCCC_EM_RET_NOT_AFFECTED:
	case SMCCC_EM_RET_AFFECTED:
		return res.a0;
	};

	return -EIO;
}

int __init arm_smccc_em_init(void)
{
	u32 major_ver, minor_ver;
	struct arm_smccc_res res;
	enum arm_smccc_conduit conduit = arm_smccc_1_1_get_conduit();

	if (conduit == SMCCC_CONDUIT_NONE)
		return -EOPNOTSUPP;

	arm_smccc_1_1_invoke(ARM_SMCCC_EM_VERSION, &res);
	if (res.a0 == SMCCC_RET_NOT_SUPPORTED)
		return -EOPNOTSUPP;

	major_ver = PSCI_VERSION_MAJOR(res.a0);
	minor_ver = PSCI_VERSION_MINOR(res.a0);
	if (major_ver != 1)
		return -EIO;

	arm_smccc_1_1_invoke(ARM_SMCCC_EM_FEATURES,
			     ARM_SMCCC_EM_CPU_ERRATUM_FEATURES, &res);
	if (res.a0 == SMCCC_RET_NOT_SUPPORTED)
		return -EOPNOTSUPP;

	pr_info("SMCCC Errata Management Interface v%d.%d\n",
		major_ver, minor_ver);

	WRITE_ONCE(em_features_supported, true);

	arm_smccc_1_1_invoke(ARM_SMCCC_EM_FEATURES,
			     ARM_SMCCC_EM_CPU_WORKAROUND_CONFIG, &res);
	if (res.a0 != SMCCC_RET_NOT_SUPPORTED)
		WRITE_ONCE(em_workaround_config_supported, true);

	return 0;
}
