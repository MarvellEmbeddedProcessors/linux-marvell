/* SPDX-License-Identifier: GPL-2.0 */
/* Marvell RVU Admin Function driver
 *
 * Copyright (C) 2024 Marvell.
 *
 */

#ifndef RVU_CPT_H
#define RVU_CPT_H

#define CPT_AF_MAX_RXC_QUEUES	16
#define CPT_AF_MAX_CTX_ILEN	GENMASK(2, 0)
#define CPT_AF_NIX_QUEUE	GENMASK_ULL(7, 4)
#define CPT_AF_RXC_QUEUE	GENMASK_ULL(31, 28)
#define CPT_AF_ENG_GRPMASK	GENMASK(55, 48)
#define CPT_AF_QUEUE_PRI	GENMASK(2, 0)
#define CPT_AF_CTX_ILEN		GENMASK(19, 17)
#define CPT_AF_INFLIGHT_LIMIT	GENMASK(47, 40)
#define CPT_AF_CTX_PF_FUNC	GENMASK(31, 16)
#define CPT_AF_SSO_PF_FUNC	GENMASK(47, 32)
#define CPT_AF_NIX_PF_FUNC	GENMASK(63, 48)

/* Length of initial context fetch in 128 byte words */
#define CPT_CTX_ILEN    1ULL

#define IPSEC_GEN_CFG_EGRP    GENMASK_ULL(50, 48)
#define IPSEC_GEN_CFG_OPCODE  GENMASK_ULL(47, 32)
#define IPSEC_GEN_CFG_PARAM1  GENMASK_ULL(31, 16)
#define IPSEC_GEN_CFG_PARAM2  GENMASK_ULL(15, 0)

#define CPT_INST_QSEL_BLOCK   GENMASK_ULL(28, 24)
#define CPT_INST_QSEL_PF_FUNC GENMASK_ULL(23, 8)
#define CPT_INST_QSEL_SLOT    GENMASK_ULL(7, 0)

#define CPT_INST_CREDIT_HYST  GENMASK_ULL(61, 56)
#define CPT_INST_CREDIT_TH    GENMASK_ULL(53, 32)
#define CPT_INST_CREDIT_BPID  GENMASK_ULL(30, 22)
#define CPT_INST_CREDIT_CNT   GENMASK_ULL(21, 0)

#define CPT_CTX_INVAL_PFFUNC  GENMASK_ULL(63, 48)

#define RXC_ZOMBIE_COUNT  GENMASK_ULL(60, 48)
#define RXC_ZOMBIE_THRES  GENMASK_ULL(59, 48)
#define RXC_ZOMBIE_LIMIT  GENMASK_ULL(43, 32)

#define RXC_ACTIVE_COUNT  GENMASK_ULL(60, 48)
#define RXC_ACTIVE_THRES  GENMASK_ULL(27, 16)
#define RXC_ACTIVE_LIMIT  GENMASK_ULL(11, 0)

/* CPT_AF_UCCX_CTL bits: CQ_ENA (0), CQ_ENA_SWARN (1). */
#define CPT_AF_UCC_CTL_CQ_ENA		BIT_ULL(0)
#define CPT_AF_UCC_CTL_CQ_ENA_SWARN	BIT_ULL(1)
#define CPT_AF_UCC_CTL_CQ_ENA_MASK	(CPT_AF_UCC_CTL_CQ_ENA | \
					 CPT_AF_UCC_CTL_CQ_ENA_SWARN)
/* Risc-V related registers*/
#define CPT_RV_BOOT_ADDR 0x7c2
#define CPT_RV_MTVEC_ADDR 0x305
#define CPT_RV_MTVEC_MEM 0x7ff800

#define CPT_RV_EXE_AXS_CTL_ADDR (0x10)
#define CPT_RV_EXE_AXS_CTL_REGION_DMEM 0
#define CPT_RV_EXE_AXS_CTL_REGION_CSR 1
/* RVT EXE Access Control register */
union otx2_cpt_rvt_exe_axs_ctl {
	u64 u;
	struct {
		u64 addr : 18;
		u64 rsvd : 14;
		u64 region : 2;
		u64 rsvd1 : 29;
		u64 auto_incr : 1;
	} s;
};

#define INIT_RV_EXE_AXS_CTL(_reg, _addr, _region, _auto_incr) \
	do { \
		typeof(_reg) *__r = &(_reg); \
		__r->u = 0; \
		__r->s.addr = (_addr); \
		__r->s.region = (_region); \
		__r->s.auto_incr = (_auto_incr); \
	} while (0)

#define CPT_RV_EXE_AXS_DAT_ADDR (0x18)
/* RVT EXE Access Data register */
union otx2_cpt_rvt_exe_axs_dat {
	u64 u;
	struct {
		u64 addr : 64;
	} s;
};

#define INIT_RV_EXE_AXS_DAT(_reg, _addr) \
	do { \
		typeof(_reg) *__r = &(_reg); \
		__r->u = 0; \
		__r->s.addr = (_addr); \
	} while (0)

/* CPT Engine Control Bus Command Enumeration */
#define CPT_RV_EXE_CTL_CMD_E__RD_CFG_REG_M  0x1
#define CPT_RV_EXE_CTL_CMD_E__WR_CFG_REG_M  0x0

union otx2_cpt_af_exe_cfg_cmd {
	u64 u;
	struct {
		u64 exe : 8;
		u64 rsvd1 : 24;
		u64 addr : 16;
		u64 dat_ld_busy: 1;
		u64 rsvd2 : 14;
		u64 cmd : 1;
	} s;
};

#define INIT_AF_EXE_CFG_CMD(_reg, _exe, _addr, _cmd) \
	do { \
		typeof(_reg) *__r = &(_reg); \
		__r->u = 0; \
		__r->s.exe = (_exe); \
		__r->s.addr = (_addr); \
		__r->s.cmd = (_cmd); \
	} while (0)

/* CPT_AF_CONSTANTS1 bits: MAX_SE, MAX_IE, MAX_AE, MAX_RE */
#define MAX_RE  GENMASK_ULL(63, 48)
#define MAX_AE  GENMASK_ULL(47, 32)
#define MAX_IE  GENMASK_ULL(31, 16)
#define MAX_SE  GENMASK_ULL(15, 0)

struct rvu_cpt {
	/* PCIFUNC to CPT RX Queue map */
	u16                     cptpfvf_map[CPT_AF_MAX_RXC_QUEUES];
	DECLARE_BITMAP(cpt_rx_queue_bitmap, CPT_AF_MAX_RXC_QUEUES);
};

void rvu_cn20k_cpt_init(struct rvu *rvu);
int otx2_cpt_que_pri_mask(struct rvu *rvu);
void cpt_cn20k_rxc_time_cfg(struct rvu *rvu, int blkaddr,
			    struct cpt_rxc_time_cfg_req *req,
			    struct cpt_rxc_time_cfg_req *save);
void cpt_cn20k_rxc_teardown(struct rvu *rvu, u16 pcifunc, int blkaddr);
int cpt_cn20k_ctx_flush(struct rvu *rvu, int blkaddr, u16 pcifunc);
int cpt_cn20k_re_flt_init(struct rvu *rvu);
void cpt_cn20k_re_flt_destroy(void);
bool cpt_cn20k_re_flt_handler(int eng, struct rvu *rvu);
#endif /* RVU_CPT_H */
