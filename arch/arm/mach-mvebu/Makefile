ifeq ($(CONFIG_CPU_BIG_ENDIAN),y)
ENDIAN      = BE
else
ENDIAN      = LE
endif

ccflags-$(CONFIG_ARCH_MULTIPLATFORM) := -I$(srctree)/$(src)/include \
	-I$(srctree)/arch/arm/plat-orion/include

AFLAGS_coherency_ll.o		:= -Wa,-march=armv7-a

obj-y				 += system-controller.o mvebu-soc-id.o
obj-$(CONFIG_MACH_ARMADA_370_XP) += armada-370-xp.o
obj-$(CONFIG_MACH_ARMADA_375) += armada-375.o
obj-$(CONFIG_MACH_ARMADA_380) += armada-38x.o pm.o
obj-$(CONFIG_ARCH_MVEBU)	 += \
	coherency.o coherency_ll.o pmsu.o pmsu_ll.o cpu-reset.o \
	common.o usb-cluster.o usb-utmi.o serdes.o
ifeq ($(CONFIG_SMP),y)
obj-$(CONFIG_MACH_ARMADA_XP)	 += platsmp.o headsmp.o
obj-$(CONFIG_MACH_ARMADA_375)	 += platsmp-375.o headsmp-375.o
obj-$(CONFIG_MACH_ARMADA_380)	 += platsmp-380.o headsmp-380.o
endif
obj-$(CONFIG_HOTPLUG_CPU)        += hotplug.o
obj-$(CONFIG_PROC_FS)            += dump_mv_regs.o
obj-$(CONFIG_MVEBU_SOFT_POWEROFF)      += soft-poweroff.o
obj-y	+= linux_oss/mvOs.o


COMMON_DIR	:= $(srctree)/arch/arm/mach-mvebu/include/mach
ccflags-y	+= -I$(COMMON_DIR) -DMV_CPU_$(ENDIAN)
