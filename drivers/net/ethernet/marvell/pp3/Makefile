#
# Makefile for the marvell PPv3 network driver.
#

obj-$(CONFIG_MVPP3) += mv_pp3.o

PLAT_DIR := drivers/net/ethernet/marvell

INCLUDE_DIRS       := -I$(PLAT_DIR)
INCLUDE_DIRS       += -I$(PLAT_DIR)/common
INCLUDE_DIRS       += -I$(srctree)/arch/arm/mach-mvebu
INCLUDE_DIRS       += -I$(PLAT_DIR)/pp3
INCLUDE_DIRS       += -I$(PLAT_DIR)/net_complex

export INCLUDE_DIRS
ccflags-y	+= $(INCLUDE_DIRS)
ccflags-y += -I$(PLAT_DIR)/pp3/tm/platform
ccflags-y += -I$(PLAT_DIR)/pp3/tm/core
ccflags-y += -I$(PLAT_DIR)/pp3/tm/core/resource_management
ccflags-y += -I$(PLAT_DIR)/pp3/tm/wrappers

mv_pp3-objs := common/mv_stack.o common/mv_common.o
mv_pp3-objs += platform/mv_pp3.o platform/mv_pp3_config.o platform/mv_pp3_debug_sysfs.o platform/mv_pp3_init_sysfs.o
mv_pp3-objs += vport/mv_pp3_vq.o vport/mv_pp3_swq.o vport/mv_pp3_pool.o vport/mv_pp3_vport.o vport/mv_pp3_cpu.o
mv_pp3-objs += vport/mv_vport_sysfs.o
mv_pp3-objs += platform/mv_pp3_version.o
mv_pp3-objs += hmac/mv_hmac.o hmac/mv_hmac_sysfs.o
mv_pp3-objs += emac/mv_emac.o emac/mv_emac_sysfs.o
mv_pp3-objs += fw/mv_pp3_fw_msg.o fw/mv_fw.o fw/mv_fw_sysfs.o
mv_pp3-objs += msg/mv_pp3_msg_chan.o msg/mv_pp3_msg_drv.o msg/mv_pp3_msg_sysfs.o
mv_pp3-objs += bm/mv_bm.o bm/mv_bm_sysfs.o
mv_pp3-objs += qm/mv_qm.o qm/mv_qm_regs.o qm/mv_qm_sysfs.o
mv_pp3-objs += cmac/mv_cmac.o cmac/mv_cmac_sysfs.o
mv_pp3-objs += net_dev/mv_netdev.o net_dev/mv_dev_dbg.o net_dev/mv_dev_sysfs.o net_dev/mv_dev_debug_sysfs.o
mv_pp3-objs += net_dev/mv_dev_vq.o net_dev/mv_dev_vq_sysfs.o net_dev/mv_ethtool.o net_dev/mv_dev_init_sysfs.o
mv_pp3-objs += gnss/mv_pp3_gnss_api.o gnss/mv_pp3_gnss_stats_api.o
#mv_pp3-objs += net_dev/mv_dev_bpi_sysfs.o

ifeq ($(CONFIG_MV_PP3_TM_SUPPORT),y)
mv_pp3-objs += tm/platform/tm_sysfs.o tm/platform/tm_get_gen_param_implementation.o
mv_pp3-objs += tm/platform/tm_locking_imp.o tm/platform/tm_os_if.o tm/platform/tm_rw_registers_implementation.o
mv_pp3-objs += tm/platform/tm_alias.o tm/platform/tm_to_qmtm_enums.o tm/platform/tm_hw_conf.o tm/core/tm_ctl.o
mv_pp3-objs += tm/platform/tm_sysfs_debug.o tm/platform/tm_sysfs_drop.o tm/platform/tm_sysfs_shaping.o
mv_pp3-objs += tm/core/set_hw_registers.o tm/core/tm_drop.o tm/core/tm_elig_prio_func.o tm/core/tm_errors.o
mv_pp3-objs += tm/core/tm_nodes_create.o tm/core/tm_nodes_ctl.o tm/core/tm_nodes_read.o tm/core/tm_nodes_reorder.o
mv_pp3-objs += tm/core/tm_nodes_status.o tm/core/tm_nodes_tree.o tm/core/tm_nodes_update.o tm/core/tm_nodes_utils.o
mv_pp3-objs += tm/core/tm_registers_processing.o tm/core/tm_sched.o tm/core/tm_set_local_db_defaults.o tm/core/tm_shaping.o
mv_pp3-objs += tm/core/resource_management/rm_chunk.o tm/core/resource_management/rm_ctl.o
mv_pp3-objs += tm/core/resource_management/rm_free.o tm/core/resource_management/rm_list.o
mv_pp3-objs += tm/core/resource_management/rm_reorder.o tm/core/resource_management/rm_status.o
mv_pp3-objs += tm/core/resource_management/rm_alloc.o
mv_pp3-objs += tm/wrappers/mv_tm_drop.o tm/wrappers/mv_tm_sched.o tm/wrappers/mv_tm_shaping.o tm/wrappers/mv_tm_scheme.o
mv_pp3-objs += tm/mv_tm.o
endif

mv_pp3-objs += gop/a390_mg_if.o gop/mv_gop_if.o gop/mv_mib_cntrs.o gop/mv_gop_sysfs.o
mv_pp3-objs += gop/mac/mv_xlg_mac_if.o gop/mac/mv_gmac_if.o
mv_pp3-objs += gop/pcs/mv_xpcs_if.o gop/pcs/mv_gpcs_if.o
mv_pp3-objs += gop/serdes/mv_serdes_if.o gop/mv_smi.o
mv_pp3-objs += gop/mv_ptp_if.o gop/mv_ptp_if_serv.o gop/mv_tai_clock.o
mv_pp3-objs += net_dev/mv_ptp_uio.o

