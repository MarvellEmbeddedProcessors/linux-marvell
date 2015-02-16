#
# Makefile for the Marvell Prestera Device Driver
#
ifneq ($(MACHINE),)
include $(srctree)/$(MACHINE)/config/mvRules.mk
endif

ccflags-y       += -Idrivers/net/ethernet/marvell/prestera/pci \
					-Iarch/arm/plat-armada \
					-Idrivers

obj-$(CONFIG_MV_INCLUDE_PRESTERA_PCI)	+= mv_prestera_pci.o
