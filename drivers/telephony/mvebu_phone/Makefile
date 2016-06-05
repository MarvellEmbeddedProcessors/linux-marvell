#
# Makefile for mvebu_phone Marvell telephony driver
#

CPU_ARCH    = ARM
ifeq ($(CONFIG_CPU_BIG_ENDIAN),y)
ENDIAN      = BE
else
ENDIAN      = LE
endif

ccflags-y := -DMV_LINUX -DMV_CPU_$(ENDIAN) -DMV_$(CPU_ARCH)
ccflags-y += -I$(srctree)/drivers/telephony/mvebu_phone

# MAIN COMPONENTS
PHONE_OBJS = mv_phone_dev.o
PHONE_OBJS += mv_phone_spi.o

# TAL
PHONE_OBJS += tal/tal.o
PHONE_OBJS += tal/tal_dev.o

# SLIC
PHONE_OBJS += slic/drv_dxt_if.o
PHONE_OBJS += slic/silabs_if.o
PHONE_OBJS += slic/zarlink_if.o

# TDM2C support
PHONE_OBJS += tdm2c/tdm2c.o

# TDMMC support
PHONE_OBJS += tdmmc/tdmmc.o

mv_phone-objs := $(PHONE_OBJS)
obj-y := mv_phone.o
