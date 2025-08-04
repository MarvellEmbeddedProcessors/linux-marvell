# SPDX-License-Identifier: GPL-2.0
#
# Makefile for Marvell's RVU BPHY device drivers
#

obj-$(CONFIG_OCTEONTX2_BPHY_PF) += rvu_bphy_nicpf.o

rvu_bphy_nicpf-y := otx2_bphypf.o otx2_bphypf_ethtool.o

obj-$(CONFIG_OCTEONTX2_BPHY_CNF20K_CPRI_PF) += rvu_bphy_cnf20k_cpri_pf.o

rvu_bphy_cnf20k_cpri_pf-y := cnf20k_cpri.o cnf20k_cpri_ethtool.o

ccflags-y += -I$(srctree)/drivers/net/ethernet/marvell/octeontx2/bphy
ccflags-y += -I$(srctree)/drivers/net/ethernet/marvell/octeontx2/af
ccflags-y += -I$(srctree)/drivers/net/ethernet/marvell/octeontx2/nic
ccflags-y += -I$(srctree)/drivers/soc/marvell/rvu_af_extn
