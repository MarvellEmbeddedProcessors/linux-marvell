#
# Makefile for the Marvell CESA driver
#
ifeq ($(CONFIG_PLAT_ARMADA),y)
include $(srctree)/$(MACHINE)/config/mvRules.mk
endif

ifeq ($(CONFIG_MV_CESA_OCF),y)
EXTRA_CFLAGS    += -I$(srctree)/crypto/ocf
endif

obj-$(CONFIG_MV_INCLUDE_CESA) += cesa_if.o
obj-$(CONFIG_MV_CESA_TOOL) += cesa_dev.o
obj-$(CONFIG_MV_CESA_TEST) += cesa_test.o
obj-$(CONFIG_MV_CESA_OCF)  += cesa_ocf_drv.o
