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
export INCLUDE_DIRS

ccflags-y	+= $(INCLUDE_DIRS)

obj-y	+= common/mvCommon.o common/mvStack.o common/mvDebug.o

obj-$(CONFIG_MV_ETH_INCLUDE_PHY)	+= phy/mvEthPhy.o phy/phy_sysfs.o

obj-$(CONFIG_MV_ETH_INCLUDE_NETMUX)	+= netmux/

obj-$(CONFIG_MV_ETH_NETA) += neta/
obj-$(CONFIG_MV_ETH_PP2) += pp2/

