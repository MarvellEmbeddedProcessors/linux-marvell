#
# Makefile for the Marvell CESA driver
#

CPU_ARCH	= ARM
ifeq ($(CONFIG_CPU_BIG_ENDIAN),y)
ENDIAN      = BE
else
ENDIAN      = LE
endif

MV_DEFINE = -DMV_LINUX -DMV_CPU_$(ENDIAN) -DMV_$(CPU_ARCH)

CESA_DIR	:= drivers/crypto/mvebu_cesa

obj-y += cesa_if.o cesa_ocf_drv.o cesa_test.o

obj-$(CONFIG_MV_CESA_TOOL) += cesa_dev.o

obj-y += hal/mvCesa.o hal/mvCesaDebug.o hal/mvSHA256.o	\
	 hal/mvMD5.o hal/mvSHA1.o hal/AES/mvAesAlg.o	\
	 hal/AES/mvAesApi.o

EXTRA_INCLUDE	=  -I$(CESA_DIR)
EXTRA_INCLUDE	+= -I$(srctree)/arch/arm/mach-mvebu/include/mach
EXTRA_INCLUDE	+= -I$(srctree)/arch/arm/mach-mvebu/linux_oss
EXTRA_INCLUDE	+= -I$(CESA_DIR)/hal
EXTRA_INCLUDE	+= -I$(srctree)/crypto/ocf

ccflags-y	+= $(EXTRA_INCLUDE) $(MV_DEFINE)
