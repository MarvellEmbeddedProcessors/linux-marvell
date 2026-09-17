/* SPDX-License-Identifier: GPL-2.0 */
/* Marvell RVU Admin Function driver
 *
 * Copyright (C) 2024 Marvell.
 *
 */

#ifndef NIX_CN20K_H
#define NIX_CN20K_H

#define NIX_RX_INL_PROFILE_CNT 8

struct nix_rx_inl_profile_users {
	struct list_head list;
	u16 pf_func;
};

struct nix_rx_inl_profile {
	struct list_head pfvf_list;
	u64 def_cfg;
	u64 extract_cfg;
	u64 gen_cfg;
	u64 prot_field_cfg[NIX_RX_INL_PROFILE_PROTO_CNT];
};

/* NIX_AF_LFX_RX_VTAG_TYPEX[IPSEC_QSEL_ALG] field mask, bits [11:8] */
#define NIX_AF_RX_VTAG_TYPE_IPSEC_QSEL_ALG	GENMASK_ULL(11, 8)

/* Max VLAN_CFG table entries (NIX_AF_RX_IPSEC_VLAN_CFGX, index 0..15) */
#define NIX_RX_IPSEC_VLAN_CFG_CNT		16

struct nix_cn20k_hw {
	/* Lock to manage RX inline profiles */
	struct mutex rx_inl_lock;
	struct nix_rx_inl_profile inl_profiles[NIX_RX_INL_PROFILE_CNT];
	DECLARE_BITMAP(rx_inl_profile_bmap, NIX_RX_INL_PROFILE_CNT);
	/* Bitmap and ownership tracking for IPsec VLAN_CFG table entries */
	DECLARE_BITMAP(ipsec_vlan_cfg_bmap, NIX_RX_IPSEC_VLAN_CFG_CNT);
	u16 ipsec_vlan_cfg_owner[NIX_RX_IPSEC_VLAN_CFG_CNT];
};

struct nix_hw;

void rvu_nix_block_cn20k_init(struct rvu *rvu, struct nix_hw *nix_hw);
int rvu_nix_cn20k_free_resources(struct rvu *rvu, u16 pcifunc);
void nix_free_rx_inl_queues(struct rvu *rvu, u16 pcifunc);

#define NIX_AF_CN20K_CPT_INST_CREDIT_TH    GENMASK_ULL(54, 33)
#define NIX_AF_CN20K_CPT_INST_CREDIT_BPID  GENMASK_ULL(32, 22)
#endif /* NIX_CN20K_H */
