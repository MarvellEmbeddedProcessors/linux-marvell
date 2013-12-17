#
# Makefile for the Marvell Gigabit Ethernet driver
#

ifeq ($(CONFIG_ARCH_MVEBU),y)
PP2_HAL_DIR = hal
else
PP2_HAL_DIR = ../../mv_hal/pp2
endif

PP2_GMAC_DIR  = $(PP2_HAL_DIR)/gmac
PP2_GBE_DIR   = $(PP2_HAL_DIR)/gbe
PP2_BM_DIR    = $(PP2_HAL_DIR)/bm
PP2_PRS_DIR   = $(PP2_HAL_DIR)/prs
PP2_CLS_DIR   = $(PP2_HAL_DIR)/cls
PP2_PME_DIR   = $(PP2_HAL_DIR)/pme
PP2_PLCR_DIR  = $(PP2_HAL_DIR)/plcr
PP2_WOL_DIR   = $(PP2_HAL_DIR)/wol
PP2_DPI_DIR   = $(PP2_HAL_DIR)/dpi
PP2_COMMON_DIR= $(PP2_HAL_DIR)/common

mv_pp2-objs +=	$(PP2_GMAC_DIR)/mvEthGmacApi.o
mv_pp2-objs +=	$(PP2_GBE_DIR)/mvPp2Gbe.o $(PP2_GBE_DIR)/mvPp2GbeDebug.o
#mv_pp2-objs +=  $(PP2_GBE_DIR)/mvPp2AddrDec.o
mv_pp2-objs +=	$(PP2_BM_DIR)/mvBm.o
mv_pp2-objs += 	$(PP2_PRS_DIR)/mvPp2PrsHw.o $(PP2_PRS_DIR)/mvPp2Prs.o
mv_pp2-objs += 	$(PP2_CLS_DIR)/mvPp2ClsHw.o $(PP2_CLS_DIR)/mvPp2Cls2Hw.o \
		$(PP2_CLS_DIR)/mvPp2Cls3Hw.o $(PP2_CLS_DIR)/mvPp2Cls4Hw.o \
		$(PP2_CLS_DIR)/mvPp2ClsMcHw.o $(PP2_CLS_DIR)/mvPp2Classifier.o
mv_pp2-objs += 	$(PP2_PME_DIR)/mvPp2PmeHw.o
mv_pp2-objs += 	$(PP2_PLCR_DIR)/mvPp2PlcrHw.o
mv_pp2-objs +=	$(PP2_BM_DIR)/mvBm.o
mv_pp2-objs +=  $(PP2_WOL_DIR)/mvPp2Wol.o
mv_pp2-objs +=  $(PP2_DPI_DIR)/mvPp2DpiHw.o
mv_pp2-objs +=  $(PP2_COMMON_DIR)/mvPp2Common.o

mv_pp2-objs += net_dev/mv_netdev.o net_dev/mv_ethernet.o net_dev/mv_eth_sysfs.o net_dev/mv_eth_tool.o
mv_pp2-objs += net_dev/mv_eth_rx_sysfs.o net_dev/mv_eth_tx_sysfs.o net_dev/mv_eth_tx_sched_sysfs.o
mv_pp2-objs += net_dev/mv_eth_pme_sysfs.o net_dev/mv_eth_pon_sysfs.o
mv_pp2-objs += net_dev/mv_eth_bm_sysfs.o net_dev/mv_eth_qos_sysfs.o net_dev/mv_eth_dbg_sysfs.o

ifeq ($(CONFIG_MV_ETH_HWF),y)
mv_pp2-objs += net_dev/mv_eth_hwf_sysfs.o
endif

mv_pp2-objs += prs/prs_low_sysfs.o prs/prs_high_sysfs.o
mv_pp2-objs += cls/cls_sysfs.o cls/cls2_sysfs.o cls/cls3_sysfs.o cls/cls4_sysfs.o cls/cls_mc_sysfs.o
mv_pp2-objs += pme/pme_sysfs.o
mv_pp2-objs += plcr/plcr_sysfs.o
mv_pp2-objs += wol/wol_sysfs.o
mv_pp2-objs += dpi/dpi_sysfs.o

ifeq ($(CONFIG_MV_ETH_L2FW),y)
mv_pp2-objs += l2fw/l2fw_sysfs.o l2fw/mv_eth_l2fw.o
endif

ifeq ($(CONFIG_ARCH_MVEBU),y)

ccflags-y       += $(MVEBU_NET_FLAGS)

ccflags-y	+= $(INCLUDE_DIRS)

ccflags-y       += -I$(PLAT_DIR)/pp2
ccflags-y       += -I$(PLAT_DIR)/pp2/hal

else

ifneq ($(MACHINE),)
include $(srctree)/$(MACHINE)/config/mvRules.mk
endif

ccflags-y       += -I$(PLAT_PATH_I)/$(HAL_PP2_DIR)
ccflags-y       += -I$(PLAT_PATH_I)/$(HAL_ETHPHY_DIR)
ccflags-y       += -I$(PLAT_PATH_I)/$(LSP_MUX_DIR)
endif

ifeq ($(NETMAP),y)
ccflags-y       += -DCONFIG_NETMAP -I$(NETMAP_DIR) -I$(NETMAP_DIR)/../sys
endif

obj-$(CONFIG_MV_ETH_PP2) += mv_pp2.o
