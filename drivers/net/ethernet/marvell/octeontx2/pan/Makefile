# SPDX-License-Identifier: GPL-2.0
#
# Makefile for Marvell's RVU Ethernet device drivers
#
ccflags-y += -I$(srctree)/drivers/net/ethernet/marvell/octeontx2/af
ccflags-y += -I$(srctree)/drivers/net/ethernet/marvell/octeontx2/pan
ccflags-y += -I$(srctree)/drivers/net/ethernet/marvell/octeontx2/pan/switch
ccflags-y += -I$(srctree)/drivers/net/ethernet/marvell/octeontx2/nic
ccflags-y += -I$(srctree)/drivers/soc/marvell/rvu_af_extn

PAN_OBJS:= pan_fl_tbl.o  pan_init.o  pan_parse_buf.o  \
	   pan_rvu.o  pan_stats.o  pan_test.o  pan_tl.o  pan_tuple.o \
	   switch/pan_sw.o switch/pan_sw_l2.o switch/pan_sw_l3.o

HW_OBJS:= hw/otx2_common.o hw/otx2_pf.o hw/otx2_txrx.o hw/qos.o hw/cn10k.o

obj-$(CONFIG_OCTEONTX_PAN) += pan.o
pan-y := $(PAN_OBJS) $(HW_OBJS)
