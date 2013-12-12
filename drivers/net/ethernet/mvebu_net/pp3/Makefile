#
# Makefile for the Marvell PPv3 network driver.
#

obj-$(CONFIG_MV_PP3) += mv_pp3.o

ccflags-y       += -Idrivers/net/ethernet/marvell/pp3

mv_pp3-objs := net_dev/mv_netdev.o hmac/mv_hmac.o emac/mv_emac.o emac/mv_emac_sysfs.o
