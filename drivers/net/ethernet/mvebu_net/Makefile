# Makefile for mvebu_net Marvell network drivers


CPU_ARCH    = ARM
ifeq ($(CONFIG_CPU_BIG_ENDIAN),y)
ENDIAN      = BE
else
ENDIAN      = LE
endif

MVEBU_NET_FLAGS := -DMV_LINUX -DMV_CPU_$(ENDIAN) -DMV_$(CPU_ARCH)
export MVEBU_NET_FLAGS

ccflags-y       += $(MVEBU_NET_FLAGS)

PLAT_DIR 	:= drivers/net/ethernet/mvebu_net
export PLAT_DIR

INCLUDE_DIRS	+= -I$(PLAT_DIR)
INCLUDE_DIRS	+= -I$(PLAT_DIR)/common
INCLUDE_DIRS	+= -I$(srctree)/arch/arm/mach-mvebu/linux_oss
INCLUDE_DIRS	+= -I$(PLAT_DIR)/switch
INCLUDE_DIRS	+= -I$(PLAT_DIR)/netmux
INCLUDE_DIRS	+= -I$(PLAT_DIR)/phy
INCLUDE_DIRS	+= -I$(PLAT_DIR)/pp3
INCLUDE_DIRS	+= -I$(srctree)/arch/arm/mach-mvebu
export INCLUDE_DIRS

ccflags-y	+= $(INCLUDE_DIRS)

ifneq ($(CONFIG_MV_ETH_PP2)$(CONFIG_MV_ETH_NETA),)
obj-y	+= common/mvCommon.o common/mvStack.o common/mvDebug.o common/mvList.o
endif

obj-$(CONFIG_MV_ETH_INCLUDE_PHY)	+= phy/mvEthPhy.o phy/phy_sysfs.o

obj-$(CONFIG_MV_ETH_INCLUDE_NET_COMPLEX)	+= net_complex/mv_net_complex_a39x.o

ifeq ($(CONFIG_MV_ETH_INCLUDE_NETMUX),y)
obj-y	+= netmux/
endif

ifeq ($(CONFIG_MV_ETH_NETA),y)
obj-y += neta/
endif
ifeq ($(CONFIG_MV_ETH_PP2),y)
obj-y += pp2/
endif
ifeq ($(CONFIG_MV_PP3),y)
obj-y += pp3/
endif
ifeq ($(CONFIG_MV_INCLUDE_PRESTERA),y)
obj-y += prestera/
endif
ifeq ($(CONFIG_MV_INCLUDE_PRESTERA_PCI),y)
obj-y += prestera/pci/
endif
