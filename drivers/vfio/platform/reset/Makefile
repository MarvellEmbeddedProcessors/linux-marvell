vfio-platform-calxedaxgmac-y := vfio_platform_calxedaxgmac.o
vfio-platform-xhci-y := vfio_platform_xhci.o
vfio-platform-xor-v2-y := vfio_platform_xor_v2.o
vfio-platform-amdxgbe-y := vfio_platform_amdxgbe.o

ccflags-y += -Idrivers/vfio/platform

obj-$(CONFIG_VFIO_PLATFORM_CALXEDAXGMAC_RESET) += vfio-platform-calxedaxgmac.o
obj-$(CONFIG_VFIO_PLATFORM_XHCI_RESET) += vfio-platform-xhci.o
obj-$(CONFIG_VFIO_PLATFORM_XOR_V2_RESET) += vfio-platform-xor-v2.o
obj-$(CONFIG_VFIO_PLATFORM_AMDXGBE_RESET) += vfio-platform-amdxgbe.o
