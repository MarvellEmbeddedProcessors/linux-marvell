#
# Makefile for the Marvell CESA driver
#

CPU_ARCH	= ARM
ifeq ($(CONFIG_CPU_BIG_ENDIAN),y)
ENDIAN      = BE
else
ENDIAN      = LE
endif

MVEBU_NFC_FLAGS := -DMV_LINUX -DMV_CPU_$(ENDIAN) -DMV_$(CPU_ARCH)

ccflags-y       += $(MVEBU_NFC_FLAGS)

NFC_DIR		:= drivers/mtd/nand/mvebu_nfc

INCLUDE_DIRS	+= -I$(NFC_DIR)
INCLUDE_DIRS	+= -I$(NFC_DIR)/hal
INCLUDE_DIRS	+= -I$(srctree)/arch/arm/mach-mvebu/linux_oss
INCLUDE_DIRS	+= -I$(srctree)/arch/arm/mach-mvebu/include/mach

ccflags-y	+= $(INCLUDE_DIRS)

armada_nand-y	+= nand_nfc.o
armada_nand-y	+= hal/mvNfc.o

obj-$(CONFIG_MTD_NAND_NFC)	+= armada_nand.o
