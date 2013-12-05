#
# Makefile for the Marvell Gigabit Ethernet driver
#

ifneq ($(MACHINE),)
include $(srctree)/$(MACHINE)/config/mvRules.mk
endif

PP2_DIR       = ../../mv_hal/pp2
PP2_GMAC_DIR  = ../../mv_hal/pp2/gmac
PP2_GBE_DIR   = ../../mv_hal/pp2/gbe
PP2_BM_DIR    = ../../mv_hal/pp2/bm
PP2_PRS_DIR   = ../../mv_hal/pp2/prs
PP2_CLS_DIR   = ../../mv_hal/pp2/cls
PP2_PME_DIR   = ../../mv_hal/pp2/pme
PP2_PLCR_DIR  = ../../mv_hal/pp2/plcr
PP2_WOL_DIR   = ../../mv_hal/pp2/wol
PP2_DPI_DIR   = ../../mv_hal/pp2/dpi
PP2_COMMON_DIR= ../../mv_hal/pp2/common

mv_pp2-objs +=	$(PP2_GMAC_DIR)/mvEthGmacApi.o
mv_pp2-objs +=	$(PP2_GBE_DIR)/mvPp2Gbe.o $(PP2_GBE_DIR)/mvPp2GbeDebug.o \
		$(PP2_GBE_DIR)/mvPp2AddrDec.o
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
mv_pp2-objs += net_dev/mv_eth_napi_sysfs.o net_dev/mv_eth_pme_sysfs.o net_dev/mv_eth_pon_sysfs.o
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

ccflags-y       += -I$(PLAT_PATH_I)/$(HAL_PP2_DIR)
ifeq ($(NETMAP),y)
ccflags-y       += -DCONFIG_NETMAP -I$(NETMAP_DIR) -I$(NETMAP_DIR)/../sys
endif

obj-$(CONFIG_MV_ETH_PP2) += mv_pp2.o
