#
# Makefile for the Marvell Prestera Device Driver
#
ifneq ($(MACHINE),)
include $(srctree)/$(MACHINE)/config/mvRules.mk
endif

CPU_ARCH= ARM
ifeq ($(CONFIG_CPU_BIG_ENDIAN),y)
ENDIAN	= BE
else
ENDIAN	= LE
endif

MVEBU_PP_FLAGS := -DMV_LINUX -DMV_CPU_$(ENDIAN) -DMV_$(CPU_ARCH)

ccflags-y       += $(MVEBU_PP_FLAGS)

INCLUDE_DIRS	+= -Idrivers/net/ethernet/mvebu_net/prestera/pci \
					-Idrivers
ifeq ($(CONFIG_ARCH_MVEBU),y)
INCLUDE_DIRS	+= -I$(srctree)/arch/arm/mach-mvebu/include
else
INCLUDE_DIRS	+= -Iarch/arm/plat-armada
endif

ccflags-y	+= $(INCLUDE_DIRS)

obj-$(CONFIG_MV_INCLUDE_PRESTERA_PCI)	+= mv_prestera_pci.o
