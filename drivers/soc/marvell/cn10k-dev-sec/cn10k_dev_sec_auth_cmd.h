/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (C) 2026 Marvell
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef _MRVL_CN10K_DEV_SEC_AUTH_CMD_H
#define _MRVL_CN10K_DEV_SEC_AUTH_CMD_H

#include "cn10k_dev_sec_provision.h"

#define TIM_MAX_RSA_BITS	4096
#define TIM_MAX_RSA_DS_SIZE	(TIM_MAX_RSA_BITS / 8)

/* ARM SMC Call FIDs */
#define PLAT_OCTEONTX_EHSM_GET_CHALLENGE	0xc2000b2e
#define PLAT_OCTEONTX_EHSM_AUTH_CMD		0xc2000b2f

enum auth_cmd_id {
	AUTH_CMD_LOAD_UDS,
	AUTH_CMD_ADV_FA_LCS,
	AUTH_CMD_ADV_KAK_REVOCATION,
	AUTH_CMD_ADV_KM_VERSION,
	AUTH_CMD_ADV_LOADER_FW_VERSION,
	AUTH_CMD_ADV_MAIN_FW_VERSION,
	AUTH_CMD_ENABLE_EHSM_DBG_TEMPORARY,
	AUTH_CMD_ENABLE_EHSM_DBG_PERSISTENT,
	AUTH_CMD_ENABLE_DBG_JTAG_TEMPORARY,
	AUTH_CMD_ENABLE_DBG_JTAG_PERSISTENT,
	AUTH_CMD_ENABLE_MCP_JTAG_TEMPORARY,
	AUTH_CMD_ENABLE_MCP_JTAG_PERSISTENT,
	AUTH_CMD_ENABLE_AP_DBGEN_TEMPORARY,
	AUTH_CMD_ENABLE_AP_DBGEN_PERSISTENT,
	AUTH_CMD_ENABLE_AP_NIDEN_TEMPORARY,
	AUTH_CMD_ENABLE_AP_NIDEN_PERSISTENT,
	AUTH_CMD_ENABLE_AP_SPIDEN_TEMPORARY,
	AUTH_CMD_ENABLE_AP_SPIDEN_PERSISTENT,
	AUTH_CMD_ENABLE_AP_SPNIDEN_TEMPORARY,
	AUTH_CMD_ENABLE_AP_SPNIDEN_PERSISTENT,
	AUTH_CMD_ENABLE_SPARE_0_TEMPORARY,
	AUTH_CMD_ENABLE_SPARE_0_PERSISTENT,
	AUTH_CMD_ENABLE_SPARE_1_TEMPORARY,
	AUTH_CMD_ENABLE_SPARE_1_PERSISTENT,
	AUTH_CMD_ID_LAST = 0x7FFFFFFF,
};

struct auth_cmd_package {
	enum auth_cmd_id cmd_id;
	/*
	 * 96-bit UUID if OTP field auth_cmd_mode is not set,
	 * 96-bit random challenge otherwise.
	 */
	__u32 challenge[3];
	__u32 reserved[4];
};

struct auth_cmd_payload_hdr {
	__u32 magic;
	__u32 payload_len_bytes;
};

struct auth_cmd_with_hdr {
	struct auth_cmd_payload_hdr payload_hdr;
	struct auth_cmd_package cmd_package;
};

/*
 * Shared with ATF. Binary layout crosses the SMC boundary —
 * changes require matching updates in the ATF ehsm-drv.h
 * (struct ehsm_auth_cmd).
 */
struct auth_cmd {
	__u32 dsa_scheme;
	struct dsa_key_token_pair key;
	struct auth_cmd_with_hdr acmd;
	__u8 signature[TIM_MAX_RSA_DS_SIZE];
};

#define GENERATE_CHALLENGE_PACKAGE \
	_IOWR(DRV_MAGIC, 8, struct auth_cmd_package)
#define EXECUTE_AUTH_CMD \
	_IOW(DRV_MAGIC, 9, struct auth_cmd)

int cn10k_dev_sec_generate_challenge(unsigned long arg);
int cn10k_dev_sec_execute_auth_cmd(unsigned long arg);

#endif /* _MRVL_CN10K_DEV_SEC_AUTH_CMD_H */
