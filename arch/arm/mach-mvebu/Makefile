ccflags-$(CONFIG_ARCH_MULTIPLATFORM) := -I$(srctree)/$(src)/include \
	-I$(srctree)/arch/arm/plat-orion/include

AFLAGS_coherency_ll.o		:= -Wa,-march=armv7-a

obj-y				 += system-controller.o
obj-$(CONFIG_MACH_ARMADA_370_XP) += armada-370-xp.o
obj-$(CONFIG_MACH_ARMADA_375) += armada-375.o
obj-$(CONFIG_ARCH_MVEBU)	 += \
	coherency.o coherency_ll.o pmsu.o cpu-reset.o \
	common.o
ifeq ($(CONFIG_SMP),y)
obj-$(CONFIG_MACH_ARMADA_XP)	 += platsmp.o headsmp.o
obj-$(CONFIG_MACH_ARMADA_375)	 += platsmp-375.o headsmp-375.o
endif
obj-$(CONFIG_HOTPLUG_CPU)        += hotplug.o
