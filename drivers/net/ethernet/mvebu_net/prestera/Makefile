#
# Makefile for the Marvell Prestera driver.
#

obj-$(CONFIG_MV_INCLUDE_PRESTERA) += mv_prestera.o

ifeq ($(CONFIG_ARCH_MVEBU),y)
ccflags-y       += -Idrivers/net/ethernet/mvebu_net/prestera/pci \
				-Idrivers/net/ethernet/mvebu_net/prestera/platform \
				-Iarch/arm/plat-orion \
				-Idrivers
else
ccflags-y       += -Idrivers/net/ethernet/marvell/prestera/pci \
				-Idrivers/net/ethernet/marvell/prestera/platform \
				-Iarch/arm/plat-armada \
				-Iarch/arm/plat-armada/mv_drivers_lsp \
				-Idrivers
endif

mv_prestera-objs := platform/mv_prestera.o platform/mv_prestera_pltfm.o platform/mv_prestera_irq.o platform/mv_prestera_smi.o platform/mv_pss_api.o

obj-$(CONFIG_MV_INCLUDE_PRESTERA_KERNELEXT) += kerneldrv/2_6/mv_KernelExt.o
mv_prestera-objs += platform/presteraPpDriverPci.o platform/presteraPpDriverPciHalf.o platform/presteraPpDriverPexMbus.o

ccflags-y  += -DMV_CPU_LE

