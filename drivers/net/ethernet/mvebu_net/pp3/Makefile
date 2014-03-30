#
# Makefile for the Marvell PPv3 network driver.
#

obj-$(CONFIG_MV_PP3) += mv_pp3.o

ccflags-y       += -Idrivers/net/ethernet/marvell/pp3

mv_pp3-objs := net_dev/mv_netdev.o hmac/mv_hmac.o emac/mv_emac.o
mv_pp3-objs += emac/mv_emac_sysfs.o hmac/mv_hmac_sysfs.o net_dev/mv_dev_sysfs.o
mv_pp3-objs += gmac/mv_gmac.o fw/mv_channel_if.o common/mv_stack.o
mv_pp3-objs += fw/mv_fw.o fw/mv_fw_sysfs.o
mv_pp3-objs += bm/mv_bm.o bm/mv_bm_sysfs.o bm/mv_bm_regs.o
mv_pp3-objs += qm/mv_qm.o qm/mv_qm_regs.o
