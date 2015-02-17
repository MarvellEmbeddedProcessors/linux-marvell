#
# Makefile for the Marvell Phone Device Driver
#
#

ifneq ($(CONFIG_ARCH_MVEBU),y)
ifneq ($(MACHINE),)
include $(srctree)/$(MACHINE)/config/mvRules.mk
endif

VB_SRC_PATH = ../../mv_hal/voiceband
else
VB_SRC_PATH = ../voiceband
endif
LSP_TAL_PATH = tal
LSP_SPI_PATH = spi
LSP_SLIC_PATH = slic
LSP_TDM_PATH = tdm
LSP_DFEV_PATH = dfev

obj-$(CONFIG_MV_INCLUDE_TDM) += mv_phone.o

# TAL
PHONE_OBJS = $(LSP_TAL_PATH)/tal.o $(LSP_TAL_PATH)/tal_dev.o

ifneq ($(CONFIG_ARCH_MVEBU),y)
# TDM HAL
PHONE_OBJS += ../../../../../$(MACHINE)/mv_hal_if/mvSysTdm.o
endif

# SPI and SLIC, always compile SPI and all SLIC in none DFEV mode
ifneq ($(MV_LANTIQ_DFEV_SUPPORT),y)
	PHONE_OBJS += $(LSP_SPI_PATH)/spi_dev.o
	PHONE_OBJS += $(LSP_SLIC_PATH)/drv_dxt_if.o
	PHONE_OBJS += $(LSP_SLIC_PATH)/silabs_if.o
	PHONE_OBJS += $(LSP_SLIC_PATH)/zarlink_if.o
endif

ifeq ($(CONFIG_ARCH_MVEBU),y)
	PHONE_OBJS += ../mv_phone_dev.o
	PHONE_OBJS += ../mv_phone_spi.o

	ccflags-y += $(MVEBU_PHONE_FLAGS)
	ccflags-y += $(INCLUDE_DIRS)
endif

# TDM2C support
ifeq ($(CONFIG_MV_TDM2C_SUPPORT),y)
ifneq ($(CONFIG_ARCH_MVEBU),y)
	PHONE_OBJS += $(VB_SRC_PATH)/tdm/mvTdmAddrDec.o
endif
	PHONE_OBJS += $(VB_SRC_PATH)/tdm/mvTdm.o
	PHONE_OBJS += $(LSP_TDM_PATH)/tdm_if.o
endif

# TDMMC support
ifeq ($(CONFIG_MV_TDMMC_SUPPORT),y)
	PHONE_OBJS += $(VB_SRC_PATH)/commUnit/mvCommUnit.o $(VB_SRC_PATH)/commUnit/mvCommUnitAddrDec.o
	PHONE_OBJS += $(LSP_TDM_PATH)/tdm_if.o
endif

# DFEV support
ifeq ($(CONFIG_MV_LANTIQ_DFEV_SUPPORT),y)
	IPC_DFEV_MODE = SMP
ifeq ($(CONFIG_MV_IPC_LINUX_DRIVER),y)
	IPC_DFEV_MODE = AMP
endif
ifeq ($(CONFIG_MV_IPC_FREERTOS_DRIVER),y)
	IPC_DFEV_MODE = AMP
endif
	PHONE_OBJS += $(LSP_SLIC_PATH)/sdd_if.o
	PHONE_OBJS += $(LSP_DFEV_PATH)/dfev_if.o
ifeq ($(IPC_DFEV_MODE),AMP)
	PHONE_OBJS += $(LSP_DFEV_PATH)/ipc_dfev.o
else
	PHONE_OBJS += $(LSP_DFEV_PATH)/ipc_dfev_smp.o
endif
endif

mv_phone-objs := $(PHONE_OBJS)
obj-y := mv_phone.o
