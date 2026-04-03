/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (C) 2026 Marvell
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef _MRVL_CN10K_DEV_SEC_PROV_H
#define _MRVL_CN10K_DEV_SEC_PROV_H

#include <linux/ioctl.h>
#include <linux/types.h>

#define DEBUG_STATUS_REG_LCS_MASK   0xF

#define RKEK_SIZE_BITS                     256
#define RKEK_SIZE_WORDS                    ((RKEK_SIZE_BITS + 31) / 32)
#define MAX_NUM_KEY_CONTAINER  4
#define MAX_ZMODP_BIG_NUM_LEN32            128     /** 128*32=4096 bit */
#define MAX_FIELD_NUM_COEF                 128     /** 4096 bits maximum */
#define MAX_TOKEN_SIZE_WORD                8       /* 32 bytes */
#define DSA_KEY_TOKEN_PAIR_MAX_DATA_LEN32  \
                                (MAX_ZMODP_BIG_NUM_LEN32 + \
                                 MAX_FIELD_NUM_COEF + \
                                 1 + MAX_TOKEN_SIZE_WORD)


#define PLAT_OCTEONTX_EHSM_ADVANCE_LIFECYCLE_STATE  0xc2000b32
#define PLAT_OCTEONTX_EHSM_KAK_PROVISION    0xc2000b33
#define PLAT_OCTEONTX_EHSM_READ_CSR         0xc2000b16
#define PLAT_OCTEONTX_EHSM_GET_TIM0_KAK_INFO  0xc2000b31

typedef struct csr_request {
    __u32 reg_offset;
    __u32 value;
} csr_req_t;

enum lifecycle_state
{
    LIFE_STATE_NEWCHIP                     = 0x0,
    LIFE_STATE_PROVISION                   = 0x1,
    LIFE_STATE_DEPLOY                      = 0x3,
    LIFE_STATE_PERMANENT_SOC_JTAG          = 0x5,
    LIFE_STATE_FA                          = 0x7,
    LIFE_STATE_ERROR,
};

/**
 * Boot provisioning mode
 */
enum boot_provisioning_mode
{
    BOOT_MODE_SECURE = 1,
    BOOT_MODE_ENCRYPTED = 2,
    BOOT_MODE_MEASURED = 4,
    BOOT_MODE_NON_SECURE = 9,
    BOOT_MODE_NON_ENCRYPTED = 10,
    BOOT_MODE_NON_MEASURED = 12,
    BOOT_MODE_MEASURED_UNENFORCED = 20,
};

enum dsa_scheme_id
{
    DSA_SCHEME_UNPROVISIONED    = 0,
    PKCS_PSS_SHA256_2K          = 1,
    PKCS_PSS_SHA256_3K          = 2,
    PKCS_PSS_SHA256_4K          = 3,
    /* reserved for future use */
    ECDSA_P256_SHA256           = 5,
    ECDSA_P384_SHA384           = 6,
    ECDSA_P521_SHA512           = 7,
};

/*
 * Shared with ATF. Binary layout crosses the SMC boundary -- changes
 * to the types below through provision_drv_msg_t require matching updates
 * in the ATF header.
 */
struct dsa_key_token_pair
{
    __u32 key_id;            /** 0..3, upper 24 bits of key ID in TIM */
    __u32 token_len_bit;
    enum dsa_scheme_id dsa_scheme;
    __u32 key_len_bit;
    __u32 data[DSA_KEY_TOKEN_PAIR_MAX_DATA_LEN32];
};

struct sec_boot_provision {
	enum dsa_scheme_id dsa_scheme;
	__u32 token_len_bit;
	struct dsa_key_token_pair keys[MAX_NUM_KEY_CONTAINER];
};

enum key_gen_option
{
    /** Use internal method to generate 256-bit random number */
    SP800_90_DRBG = 1,
    /**
     * Use supplied OEM 256-bit number after a bytewise reverse
     * cross 256-bit field
     */
    OEM_INPUT = 2,
};

enum rkek_key_provision_option
{
    /**
     * Side load 256-bit TruPUF generated UDS into the key registers of the
     * AES hardware engine and encrypt the RKEK using AES ECB256 and burn
     * the resulting cyphertext into OTP memory.
     */
    RKEK_PROV_ENCRYPTED_BY_PUF = 0x5A7A5AFE,
    /**
     * Burn the RKEK directly into the OTP memory
     */
    RKEK_PROV_PLAINTEXT = 0x5A7A0000,
};

typedef struct provision_config {
	enum lifecycle_state target_lcs;
	enum boot_provisioning_mode mode;
	enum key_gen_option rkek_option;
	__u32 rkek[RKEK_SIZE_WORDS];
	enum rkek_key_provision_option rkek_encryption;
	struct sec_boot_provision sec_boot;
	__u32 reserved[16];
} provision_config_t;

typedef struct provision_drv_msg {
	__u32 is_tim_prov;
	__u32 key_bitmap;
	provision_config_t provision_cfg;
} provision_drv_msg_t;

enum regs
{
    /* Request input registers */
    BOOTROM_STATUS          = 0x1,
    ROOT_TRUST_STATUS       = 0x2,
    CHAIN_OF_TRUST_STATUS   = 0x3,
    UUID0                   = 0x4,
    UUID1                   = 0x5,
    UUID2                   = 0x6,
    KEY_REVOC_STATUS        = 0x7,
    FW_SEC_VER              = 0x8,
    LCS_DEBUG_STATUS        = 0x9,
    REMAINING_CONFIG_STATUS = 0xA
};

/*
 * Shared with the userspace app. Layout crosses the ioctl boundary --
 * changes to the types, ioctls, or constants below require matching
 * updates in the app header.
 */
struct tim0_key_slot_info {
	__u32 key_id;
	__u32 dsa_scheme;
	__u32 key_len_bit;
	__u32 token_len_bit;
};

struct tim0_key_info {
	struct tim0_key_slot_info keys[MAX_NUM_KEY_CONTAINER];
};


/* ROOT_TRUST_STATUS register bit definitions */
#define ROT_RKEK_LOCK_BIT		5
#define ROT_KAK_PROV_SHIFT		11
#define ROT_KAK_PROV_MASK		0xF
#define ROT_RKEK_PROV_BIT		20

#define ROT_RKEK_PROVISIONED(rot)	(!!((rot) & (1U << ROT_RKEK_PROV_BIT)))
#define ROT_RKEK_LOCKED(rot)		(!!((rot) & (1U << ROT_RKEK_LOCK_BIT)))
#define ROT_KAK_PROVISIONED(rot)	(((rot) >> ROT_KAK_PROV_SHIFT) & ROT_KAK_PROV_MASK)
#define ROT_KAK_SLOT_PROVISIONED(rot, i) \
	(!!((rot) & (1U << (ROT_KAK_PROV_SHIFT + (i)))))

struct tim_prov_request {
	__u32 bitmap;
	__u32 has_rkek;
	__u32 rkek[RKEK_SIZE_WORDS];
};

int cn10k_dev_sec_provision_send_smc(provision_drv_msg_t *msg);
int cn10k_dev_sec_get_tim0_key_info(struct tim0_key_info *info);
int cn10k_dev_sec_read_csr(struct csr_request *csr_req);
int cn10k_dev_sec_deploy_lcs(void);
void cn10k_dev_sec_provision_rkek(__u32 has_rkek, __u32 *user_rkek, provision_config_t *cfg);

#define DRV_MAGIC  0xE1

#define PROVISION_FROM_FILE  _IOWR(DRV_MAGIC, 0, void *)
#define PROVISION_TIM0_KEYS _IOW(DRV_MAGIC, 1, struct tim_prov_request)
#define LCS_DEPLOY          _IO(DRV_MAGIC, 3)
#define GET_TIM0_KEY_INFO   _IOR(DRV_MAGIC, 5, struct tim0_key_info)
#define GET_OTP_PROVISION_STATUS _IOR(DRV_MAGIC, 6, __u32)
#define GET_BOOTROM_STATUS       _IOR(DRV_MAGIC, 7, __u32)

#endif /* _MRVL_CN10K_DEV_SEC_PROV_H */
