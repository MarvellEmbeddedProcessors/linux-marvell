#
# Makefile for the Marvell Gigabit Ethernet driver
#

ifneq ($(MACHINE),)
include $(srctree)/$(MACHINE)/config/mvRules.mk
endif

NETA_DIR       = ../../mv_hal/neta
NETA_GBE_DIR   = $(NETA_DIR)/gbe
NETA_BM_DIR    = $(NETA_DIR)/bm
NETA_PNC_DIR   = $(NETA_DIR)/pnc
NETA_PMT_DIR   = $(NETA_DIR)/pmt

mv_neta-objs += $(NETA_GBE_DIR)/mvNeta.o $(NETA_GBE_DIR)/mvNetaDebug.o \
		$(NETA_GBE_DIR)/mvNetaAddrDec.o

ifeq ($(CONFIG_MV_ETH_PNC),y)
mv_neta-objs += $(NETA_PNC_DIR)/mvTcam.o  $(NETA_PNC_DIR)/mvPncAging.o \
		$(NETA_PNC_DIR)/mvPnc.o $(NETA_PNC_DIR)/mvPncLb.o
mv_neta-objs += pnc/pnc_sysfs.o

ifeq ($(CONFIG_MV_ETH_PNC_L3_FLOW),y)
mv_neta-objs += $(NETA_PNC_DIR)/mvPncRxq.o pnc/rxq_map_sysfs.o
endif

ifeq ($(CONFIG_MV_ETH_PNC_WOL),y)
mv_neta-objs += $(NETA_PNC_DIR)/mvPncWol.o pnc/wol_sysfs.o
endif
endif

ifeq ($(CONFIG_MV_ETH_BM),y)
mv_neta-objs += $(NETA_BM_DIR)/mvBm.o bm/bm_sysfs.o bm/mv_eth_bm.o
endif

ifeq ($(CONFIG_MV_ETH_HWF),y)
mv_neta-objs += hwf/hwf_bm.o
endif

ifeq ($(CONFIG_MV_ETH_PMT),y)
mv_neta-objs += $(NETA_PMT_DIR)/mvPmt.o pmt/pmt_sysfs.o
endif

ifeq ($(CONFIG_MV_ETH_HWF),y)
mv_neta-objs += $(NETA_GBE_DIR)/mvHwf.o hwf/hwf_sysfs.o
endif

mv_neta-objs += net_dev/mv_netdev.o net_dev/mv_ethernet.o net_dev/mv_eth_sysfs.o net_dev/mv_eth_tool.o

ifeq ($(CONFIG_MV_ETH_NFP),y)
mv_neta-objs += net_dev/mv_eth_nfp.o
endif

ifeq ($(CONFIG_MV_ETH_L2FW),y)
mv_neta-objs += l2fw/l2fw_sysfs.o l2fw/mv_eth_l2fw.o

ifeq ($(CONFIG_MV_ETH_L2SEC),y)
mv_neta-objs += l2fw/mv_eth_l2sec.o
endif
endif

ifeq ($(CONFIG_MV_PON),y)
mv_neta-objs += mv_pon_sysfs.o
endif

ifeq ($(NETMAP),y)
ccflags-y       += -DCONFIG_NETMAP -I$(NETMAP_DIR) -I$(NETMAP_DIR)/../sys
endif

obj-$(CONFIG_MV_ETH_NETA) += mv_neta.o
