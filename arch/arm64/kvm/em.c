// SPDX-License-Identifier: GPL-2.0
// Copyright (C) 2026 Arm Ltd.

#include <linux/arm-smccc.h>
#include <linux/kvm_host.h>
#include <kvm/arm_hypercalls.h>
#include <asm/kvm_emulate.h>
#include <asm/cpu_errata.h>

#define ARM_SMCCC_EM_VERSION_1_0	0x10000UL

int kvm_erratum_management_call(struct kvm_vcpu *vcpu)
{
	u32 func_id = smccc_get_function(vcpu);
	unsigned long val = SMCCC_RET_NOT_SUPPORTED;

	switch (func_id) {
	case ARM_SMCCC_EM_VERSION:
		val = ARM_SMCCC_EM_VERSION_1_0;
		break;
	case ARM_SMCCC_EM_FEATURES:
		switch (smccc_get_arg1(vcpu)) {
		case ARM_SMCCC_EM_VERSION:
		case ARM_SMCCC_EM_FEATURES:
		case ARM_SMCCC_EM_CPU_ERRATUM_FEATURES:
			val = SMCCC_RET_SUCCESS;
		}
		break;
	case ARM_SMCCC_EM_CPU_ERRATUM_FEATURES:
		u32 erratum_num = smccc_get_arg1(vcpu);
		u64 midr_el1 = kvm_read_vm_id_reg(vcpu->kvm, SYS_MIDR_EL1);

		if (cpus_have_final_cap(ARM64_WORKAROUND_4299121) &&
		    is_erratum_in_erratum_list(erratum_bad_tc_tlb_cpus,
					       midr_el1, erratum_num)) {
			if (is_bad_tc_tlb_mitigated()) {
				val = SMCCC_EM_RET_NOT_AFFECTED;
				break;
			}
		}

		val = SMCCC_EM_RET_UNKNOWN;
		break;
	}

	smccc_set_retval(vcpu, val, 0, 0, 0);
	return 1;
}
