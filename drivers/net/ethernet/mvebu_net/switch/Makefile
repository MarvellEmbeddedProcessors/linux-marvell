#
# Makefile for the Marvell Soho switches
#

ifeq ($(CONFIG_ARCH_MVEBU),y)

ccflags-y       += $(MVEBU_NET_FLAGS)
ccflags-y       += $(INCLUDE_DIRS)

else

ifneq ($(MACHINE),)
include $(srctree)/$(MACHINE)/config/mvRules.mk
endif

ccflags-y       += -I$(PLAT_PATH_I)/$(LSP_MUX_DIR)

endif # CONFIG_ARCH_MVEBU

obj-$(CONFIG_MV_INCLUDE_SWITCH)	+= mv_switch.o mv_switch_sysfs.o
