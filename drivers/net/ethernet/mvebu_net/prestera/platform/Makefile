#
# Makefile for the Marvell Prestera Device Driver
#
ifneq ($(MACHINE),)
include $(srctree)/$(MACHINE)/config/mvRules.mk
endif


obj-$(CONFIG_MV_INCLUDE_PRESTERA)	+= mvPrestera.o mv_prestera_pltfm.o
mvPrestera-objs				+= mv_prestera.o mv_prestera_irq.o mv_prestera_smi.o mv_pss_api.o
mvPrestera-objs				+= presteraPpDriverPci.o presteraPpDriverPciHalf.o presteraPpDriverPexMbus.o

