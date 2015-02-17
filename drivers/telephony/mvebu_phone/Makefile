#
# Makefile for mvebu_phone Marvell telephony driver
#

CPU_ARCH    = ARM
ifeq ($(CONFIG_CPU_BIG_ENDIAN),y)
ENDIAN      = BE
else
ENDIAN      = LE
endif

MVEBU_PHONE_FLAGS	:= -DMV_LINUX -DMV_CPU_$(ENDIAN) -DMV_$(CPU_ARCH)
export MVEBU_PHONE_FLAGS

ccflags-y		+= $(MVEBU_PHONE_FLAGS)

INCLUDE_DIRS		+= -I$(srctree)/drivers/telephony/mvebu_phone
INCLUDE_DIRS		+= -I$(srctree)/arch/arm/mach-mvebu/linux_oss
INCLUDE_DIRS		+= -I$(srctree)/arch/arm/mach-mvebu/include/mach
export INCLUDE_DIRS

ccflags-y		+= $(INCLUDE_DIRS)

obj-y			+= mv_phone/
