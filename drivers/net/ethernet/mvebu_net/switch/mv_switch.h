/*******************************************************************************
Copyright (C) Marvell International Ltd. and its affiliates

This software file (the "File") is owned and distributed by Marvell
International Ltd. and/or its affiliates ("Marvell") under the following
alternative licensing terms.  Once you have made an election to distribute the
File under one of the following license alternatives, please (i) delete this
introductory statement regarding license alternatives, (ii) delete the two
license alternatives that you have not elected to use and (iii) preserve the
Marvell copyright notice above.


********************************************************************************
Marvell GPL License Option

If you received this File from Marvell, you may opt to use, redistribute and/or
modify this File in accordance with the terms and conditions of the General
Public License Version 2, June 1991 (the "GPL License"), a copy of which is
available along with the File in the license.txt file or by writing to the Free
Software Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 or
on the worldwide web at http://www.gnu.org/licenses/gpl.txt.

THE FILE IS DISTRIBUTED AS-IS, WITHOUT WARRANTY OF ANY KIND, AND THE IMPLIED
WARRANTIES OF MERCHANTABILITY OR FITNESS FOR A PARTICULAR PURPOSE ARE EXPRESSLY
DISCLAIMED.  The GPL License provides additional details about this warranty
disclaimer.
*******************************************************************************/
#ifndef __mv_switch_h__
#define __mv_switch_h__

#include "mv802_3.h"
#ifdef CONFIG_MV_INCLUDE_SWITCH
#include "msApiTypes.h"
#include "msApiDefs.h"
#endif
#include "mv_mux_netdev.h"

#define MV_SWITCH_DB_NUM			16

/*TPM start*/
#define MV_SWITCH_PORT_NOT_BELONG		(0)	/* VLAN does not belong to port		*/
#define MV_SWITCH_PORT_BELONG			(1)	/* VLAN belong to port			*/
#define MV_SWITCH_MAX_VLAN_NUM			(4096)	/* Maximum switch VLAN number		*/
#define MV_SWITCH_MAX_PORT_NUM			(7)	/* Maximum switch port number		*/
#define MV_SWITCH_MAX_QUEUE_NUM			(4)	/* Maximum switch queue number per port	*/
#define MV_SWITCH_CPU_PORT_NUM			(6)	/* Switch port connected to CPU		*/
#define MV_SWITCH_DEFAULT_WEIGHT		(2)	/* Switch default queue weight		*/
/*TPM end*/

#define MV_SWITCH_PHY_ACCESS			1
#define MV_SWITCH_PORT_ACCESS			2
#define MV_SWITCH_GLOBAL_ACCESS			3
#define MV_SWITCH_GLOBAL2_ACCESS		4
#define MV_SWITCH_SMI_ACCESS                	5

#define MV_SWITCH_PORT_VLAN_ID(grp, port)  ((grp) + (port) + 1)
#define MV_SWITCH_GROUP_VLAN_ID(grp)       (((grp) + 1) << 8)
#define MV_SWITCH_VLAN_TO_GROUP(vid)       ((((vid) & 0xf00) >> 8) - 1)

#define MV_SWITCH_PIRL_BKTTYPR_UNKNOWN_MULTICAST_BIT    1
#define MV_SWITCH_PIRL_BKTTYPR_BROADCAST_BIT		2
#define MV_SWITCH_PIRL_BKTTYPR_MULTICAST_BIT		3

#define MV_SWITCH_PIRL_RESOURCE_BROADCAST    1
#define MV_SWITCH_PIRL_RESOURCE_MULTICAST    2


#ifdef CONFIG_MV_INCLUDE_SWITCH
/*TPM start*/
#define SW_IF_NULL(ptr) { \
	if (ptr == NULL) {\
		pr_err("(error) %s(%d) recvd NULL pointer\n", __func__, __LINE__);\
		return MV_FAIL;\
	} \
}
#define SW_IF_ERROR_STR(rc, format, ...) { \
	if (rc) {\
		pr_err("(error) %s(%d) (rc=%d): "format, __func__, __LINE__, rc, ##__VA_ARGS__);\
		return MV_FAIL;\
	} \
}

/* port mirror direction */
enum sw_mirror_mode_t {
	MV_SWITCH_MIRROR_INGRESS,
	MV_SWITCH_MIRROR_EGRESS,
	MV_SWITCH_MIRROR_BOTH
};


/* operations requested by switch device from mux device */
struct mux_device_ops {
	int	(*update_link)(void *cookie, int link_up);
};

/* port mirror direction */
enum sw_mac_addr_type_t {
	MV_SWITCH_ALL_MAC_ADDR,
	MV_SWITCH_UNICAST_MAC_ADDR,
	MV_SWITCH_MULTICAST_MAC_ADDR,
	MV_SWITCH_DYNAMIC_MAC_ADDR,
	MV_SWITCH_STATIC_MAC_ADDR
};

/* logical port VLAN information */
struct sw_port_info_t {
	GT_DOT1Q_MODE	port_mode;
	unsigned int	vlan_blong[MV_SWITCH_MAX_VLAN_NUM];
};

/* VLAN information */
struct sw_vlan_info_t {
	unsigned int	port_bm;				/* bitmap of the ports in this VLAN */
	unsigned char	egr_mode[MV_SWITCH_MAX_PORT_NUM];	/* egress mode of each port         */
	GT_VTU_ENTRY	vtu_entry;				/* Add this member to record HW VT info to SW table */
};

enum sw_port_state_t {
	MV_SWITCH_PORT_DISABLE = 0,
	MV_SWITCH_BLOCKING,
	MV_SWITCH_LEARNING,
	MV_SWITCH_FORWARDING
};

/*TPM end*/
#endif
/*unsigned int	mv_switch_link_detection_init(struct mv_switch_pdata *plat_data);*/
void mv_switch_interrupt_mask(void);
void mv_switch_interrupt_unmask(void);
void mv_switch_interrupt_clear(void);

int     mv_switch_unload(unsigned int switch_ports_mask);
void    mv_switch_link_update_event(MV_U32 port_mask, int force_link_check);
int     mv_switch_jumbo_mode_set(int max_size);
int     mv_switch_tos_get(unsigned char tos);
int     mv_switch_tos_set(unsigned char tos, int queue);
int     mv_switch_port_based_vlan_set(unsigned int ports_mask, int set_cpu_port);
int     mv_switch_vlan_in_vtu_set(unsigned short vlan_id, unsigned short db_num, unsigned int ports_mask);
int     mv_switch_atu_db_flush(int db_num);
int     mv_switch_vlan_set(u16 vlan_grp_id, u16 port_map);
int mv_switch_promisc_set(int db, u8 promisc_on);

int     mv_switch_reg_read(int port, int reg, int type, MV_U16 *value);
int     mv_switch_reg_write(int port, int reg, int type, MV_U16 value);

#ifdef CONFIG_MV_SW_PTP
int     mv_switch_ptp_reg_read(int port, int reg, MV_U16 *value);
int     mv_switch_ptp_reg_write(int port, int reg, MV_U16 value);
#endif

void    mv_switch_stats_print(void);
void    mv_switch_status_print(void);

int     mv_switch_all_multicasts_del(int db_num);

int     mv_switch_port_add(int switch_port, u16 vlan_grp_id);
int     mv_switch_port_del(int switch_port);

int		mv_switch_default_config_get(MV_TAG_TYPE *tag_mode,
						MV_SWITCH_PRESET_TYPE *preset, int *vid, int *gbe_port);
int		mv_switch_preset_init(MV_TAG_TYPE tag_mode, MV_SWITCH_PRESET_TYPE preset, int vid);
bool		mv_switch_tag_get(int db, MV_TAG_TYPE tag_mode, MV_SWITCH_PRESET_TYPE preset, int vid, MV_MUX_TAG *tag);
unsigned int	mv_switch_group_map_get(void);
int		mv_switch_group_restart_autoneg(int db);
int		mv_switch_group_enable(int db);
int		mv_switch_group_disable(int db);
int		mv_switch_link_status_get(int db);
int		mv_switch_group_cookie_set(int db, void *cookie);
int		mv_switch_mac_update(int db, unsigned char *old_mac, unsigned char *new_mac);
int		mv_switch_mac_addr_set(int db, unsigned char *mac_addr, unsigned char op);
int		mv_switch_mux_ops_set(const struct mv_switch_mux_ops *mux_ops_ptr);

#ifdef CONFIG_MV_INCLUDE_SWITCH
/*TPM start*/
int mv_switch_mac_update(int db, unsigned char *old_mac, unsigned char *new_mac);
int mv_switch_port_discard_tag_set(unsigned int lport, GT_BOOL mode);
int mv_switch_port_discard_tag_get(unsigned int lport, GT_BOOL *mode);
int mv_switch_port_discard_untag_set(unsigned int lport, GT_BOOL mode);
int mv_switch_port_discard_untag_get(unsigned int lport, GT_BOOL *mode);
int mv_switch_port_def_vid_set(unsigned int lport, unsigned short vid);
int mv_switch_port_def_vid_get(unsigned int lport, unsigned short *vid);
int mv_switch_port_def_pri_set(unsigned int lport, unsigned char pri);
int mv_switch_port_def_pri_get(unsigned int lport, unsigned char *pri);
int mv_switch_port_vid_add(unsigned int lport, unsigned short vid, unsigned char egr_mode, bool belong);
int mv_switch_port_vid_del(unsigned int lport, unsigned short vid);
int mv_switch_vid_get(unsigned int vid, GT_VTU_ENTRY *vtu_entry, unsigned int *found);
int mv_switch_port_vid_egress_mode_set(unsigned int lport, unsigned short vid, unsigned char egr_mode);
int mv_switch_unknown_unicast_flood_set(unsigned char lport, GT_BOOL enable);
int mv_switch_unknown_unicast_flood_get(unsigned char lport, GT_BOOL *enable);
int mv_switch_unknown_multicast_flood_set(unsigned char lport, GT_BOOL enable);
int mv_switch_unknown_multicast_flood_get(unsigned char lport, GT_BOOL *enable);
int mv_switch_broadcast_flood_set(GT_BOOL enable);
int mv_switch_broadcast_flood_get(GT_BOOL *enable);
int mv_switch_port_count3_get(unsigned int lport, GT_STATS_COUNTER_SET3 *count);
int mv_switch_port_drop_count_get(unsigned int lport, GT_PORT_STAT2 *count);
int mv_switch_port_count_clear(unsigned int lport);
int mv_switch_count_clear(void);
int mv_switch_ingr_limit_mode_set(unsigned int lport, GT_RATE_LIMIT_MODE mode);
int mv_switch_ingr_limit_mode_get(unsigned int lport, GT_RATE_LIMIT_MODE *mode);
int mv_switch_ingr_police_rate_set(unsigned int	lport,
					GT_PIRL2_COUNT_MODE	count_mode,
					unsigned int		cir,
					GT_U32		bktTypeMask);
int mv_switch_ingr_police_rate_get(unsigned int		lport,
				   GT_PIRL2_COUNT_MODE	*count_mode,
				   unsigned int		*cir);
int mv_switch_egr_rate_limit_set(unsigned int lport, GT_PIRL_ELIMIT_MODE mode, unsigned int rate);
int mv_switch_egr_rate_limit_get(unsigned int lport, GT_PIRL_ELIMIT_MODE *mode, unsigned int *rate);
int mv_switch_ingr_broadcast_rate_set(unsigned int lport, GT_PIRL2_COUNT_MODE count_mode, unsigned int cir);
int mv_switch_ingr_broadcast_rate_get(unsigned int lport, GT_PIRL2_COUNT_MODE *count_mode, unsigned int *cir);
int mv_switch_ingr_multicast_rate_set(unsigned int lport, GT_PIRL2_COUNT_MODE count_mode, unsigned int cir);
int mv_switch_ingr_multicast_rate_get(unsigned int lport, GT_PIRL2_COUNT_MODE *count_mode, unsigned int *cir);
int mv_switch_port_mirror_set(unsigned int sport, enum sw_mirror_mode_t mode, GT_BOOL enable, unsigned int dport);
int mv_switch_port_mirror_get(unsigned int sport, enum sw_mirror_mode_t mode, GT_BOOL *enable, unsigned int *dport);
int mv_switch_age_time_set(unsigned int time);
int mv_switch_age_time_get(unsigned int *time);
int mv_switch_mac_learn_disable_set(unsigned int lport, GT_BOOL enable);
int mv_switch_mac_learn_disable_get(unsigned int lport, GT_BOOL *enable);
int mv_switch_learn2all_enable_set(GT_BOOL enable);
int mv_switch_learn2all_enable_get(GT_BOOL *enable);
int mv_switch_mac_limit_set(unsigned int lport, unsigned int mac_num);
int mv_switch_mac_limit_get(unsigned int lport, unsigned int *mac_num);
int mv_switch_mac_addr_add(unsigned int port_bm, unsigned char mac_addr[6], unsigned int mode);
int mv_switch_mac_addr_del(unsigned int lport, unsigned char mac_addr[6]);
int mv_switch_port_qos_mode_set(unsigned int lport, GT_PORT_SCHED_MODE mode);
int mv_switch_port_qos_mode_get(unsigned int lport, GT_PORT_SCHED_MODE *mode);
int mv_switch_queue_weight_set(unsigned int lport, unsigned char queue, unsigned char weight);
int mv_switch_queue_weight_get(unsigned int lport, unsigned char queue, unsigned char *weight);
int mv_switch_mtu_set(unsigned int mtu);
int mv_switch_mtu_get(unsigned int *mtu);
int mv_switch_link_state_get(unsigned int lport, GT_BOOL *state);
int mv_switch_duplex_state_get(unsigned int lport, GT_BOOL *state);
int mv_switch_speed_state_get(unsigned int lport, GT_PORT_SPEED_MODE *speed);
int mv_switch_port_vlan_filter_set(unsigned int lport, unsigned char filter);
int mv_switch_port_vlan_filter_get(unsigned int lport, unsigned char *filter);
int mv_switch_port_vlan_mode_set(unsigned int lport, GT_DOT1Q_MODE mode);
int mv_switch_port_vlan_mode_get(unsigned int lport, GT_DOT1Q_MODE *mode);
int mv_switch_port_mac_filter_mode_set(unsigned int lport, GT_SA_FILTERING mode);
int mv_switch_port_mac_filter_mode_get(unsigned int lport, GT_SA_FILTERING *mode);
int mv_switch_port_mac_filter_entry_add(unsigned int lport, unsigned char *mac,
	unsigned short vlan, GT_SA_FILTERING mode);
int mv_switch_port_mac_filter_entry_del(unsigned int lport, unsigned char *mac,
	unsigned short	vlan, GT_SA_FILTERING mode);
int mv_switch_port_vlan_set(unsigned int lport, GT_LPORT mem_port[], unsigned int mem_num);
int mv_switch_port_vlan_get(unsigned int lport, GT_LPORT mem_port[], unsigned int *mem_num);
int mv_switch_mh_mode_set(unsigned char lport, GT_BOOL enable);
int mv_switch_mh_mode_get(unsigned char lport, GT_BOOL *enable);
int mv_switch_frame_mode_set(unsigned char lport, GT_FRAME_MODE mode);
int mv_switch_frame_mode_get(unsigned char lport, GT_FRAME_MODE *mode);
int mv_switch_etype_set(unsigned char lport, unsigned short etype);
int mv_switch_etype_get(unsigned char lport, unsigned short *etype);
int mv_switch_port_preamble_set(unsigned char lport, unsigned short preamble);
int mv_switch_port_preamble_get(unsigned char lport, unsigned short *preamble);
int mv_switch_port_force_speed_set(unsigned int lport, GT_BOOL enable, unsigned int mode);
int mv_switch_port_force_speed_get(unsigned int lport, GT_BOOL *enable, unsigned int *mode);
int mv_switch_port_force_duplex_set(unsigned int lport, GT_BOOL enable, GT_BOOL value);
int mv_switch_port_force_duplex_get(unsigned int lport, GT_BOOL *enable, GT_BOOL *value);
int mv_switch_atu_next_entry_get(GT_ATU_ENTRY *atu_entry);
int mv_switch_vtu_flush(void);
int mv_switch_atu_flush(GT_FLUSH_CMD flush_cmd, unsigned short db_num);
unsigned int mv_switch_port_num_get(void);
GT_QD_DEV *mv_switch_qd_dev_get(void);
int mv_switch_vtu_shadow_dump(void);
int mv_switch_vlan_tunnel_set(unsigned int lport, GT_BOOL mode);
int mv_switch_port_force_link_set(unsigned int lport, GT_BOOL enable, GT_BOOL value);
int mv_switch_port_force_link_get(unsigned int lport, GT_BOOL *enable, GT_BOOL *value);
int mv_switch_port_state_set(unsigned int lport, enum sw_port_state_t state);
int mv_switch_port_state_get(unsigned int lport, enum sw_port_state_t *state);
int mv_switch_cpu_port_get(unsigned int *cpu_port);
/*TPM end*/
#endif
#endif /* __mv_switch_h__ */
