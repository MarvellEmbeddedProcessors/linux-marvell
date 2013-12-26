#
# Makefile for the Marvell Virtual Netwrok interfaces library
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


obj-y	+= mv_mux_netdev.o mv_mux_sysfs.o mv_mux_tool.o
