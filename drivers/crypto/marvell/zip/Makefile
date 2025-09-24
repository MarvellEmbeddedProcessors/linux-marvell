# SPDX-License-Identifier: GPL-2.0-only

obj-$(CONFIG_CRYPTO_DEV_OCTEONTX2_ZIP) += octeontx-zip.o octeontx-zipvf.o

octeontx-zip-objs := zip_pf_main.o
octeontx-zipvf-objs := zip_vf_deflate.o zip_vf_debugfs.o zip_vf_main.o

ccflags-y += -I$(srctree)/drivers/net/ethernet/cavium/octeontx-83xx
