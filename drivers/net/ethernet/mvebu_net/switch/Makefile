#
# Makefile for the Marvell Key
#
ifneq ($(MACHINE),)
include $(srctree)/$(MACHINE)/config/mvRules.mk
endif

obj-$(CONFIG_MV_INCLUDE_SWITCH)	+= mv_switch.o mv_switch_sysfs.o
