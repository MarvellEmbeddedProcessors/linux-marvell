/*
* ***************************************************************************
* Copyright (C) 2015 Marvell International Ltd.
* ***************************************************************************
* This program is free software: you can redistribute it and/or modify it
* under the terms of the GNU General Public License as published by the Free
* Software Foundation, either version 2 of the License, or any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program.  If not, see <http://www.gnu.org/licenses/>.
* ***************************************************************************/
#ifndef __mv_pp3_fw_msg_h__
#define __mv_pp3_fw_msg_h__


#include "common/mv_sw_if.h"
#include "platform/mv_pp3.h"
#include "vport/mv_pp3_pool.h"
#include "vport/mv_pp3_vport.h"
#include "mv_pp3_fw_msg_structs.h"

/*------------------------------------------------------------------------------*/
/*				Host FW API					*/
/*------------------------------------------------------------------------------*/

/* get request */
int pp3_fw_mem_bufs_alloc_size_get(void);
int pp3_fw_version_get(struct mv_pp3_version *fw_ver);
int pp3_fw_mem_bufs_alloc_set(unsigned int size, unsigned int buf_num, unsigned int *bufs);
int pp3_fw_vport_stat_get(int vport, struct mv_pp3_fw_vport_stat *vport_stat);
int pp3_fw_hwq_stat_get(unsigned short q, bool clean, struct mv_pp3_fw_hwq_stat *hwq_stat);
int pp3_fw_swq_stat_get(unsigned char q, struct mv_pp3_fw_swq_stat *sw_stat);
int pp3_fw_bm_pool_stat_get(unsigned char q, struct mv_pp3_fw_bm_pool_stat *stat);
int pp3_fw_channel_stat_get(unsigned char q, struct mv_pp3_fw_msg_chan_stat *stat);
int pp3_fw_clear_stat_set(unsigned char stat_type, unsigned short num);
int pp3_fw_emac_vport_msg_get(int vport, struct mv_pp3_fw_emac_vport *out_msg);
int pp3_fw_emac_vport_msg_show(int vport);

/* set request */
int pp3_fw_vport_rx_pkt_mode_set(int vport, enum mv_pp3_pkt_mode mode);
int pp3_fw_bm_pool_set(struct pp3_pool *pool);
int pp3_fw_link_changed(int emac_num, bool link_is_up);
int pp3_fw_vport_state_set(int vport, bool enable_port);
int pp3_fw_vport_def_dest_set(int vport, int dest_vport);
int pp3_fw_vport_mtu_set(int vport, int mtu);
int pp3_fw_vport_mac_list_set(int vport,  unsigned char macs_list_size, unsigned char *macs_list);
int pp3_fw_vport_mac_list_get(int vport,  unsigned char macs_list_size, unsigned char *macs_list,
					unsigned char *macs_valid);
int pp3_fw_emac_vport_set(struct pp3_vport *vp_cfg, unsigned char *mac);
int pp3_fw_internal_cpu_vport_set(struct pp3_vport *vp_cfg);
int pp3_fw_cpu_vport_set(int vport);
int pp3_fw_cpu_vport_map(int vport, int user_cpu_vp, int int_cpu_vp);
int pp3_fw_vq_map_set(int vport, int vq, enum mv_pp3_queue_type q_type, int swq, int hwq);
int pp3_fw_cos_to_vq_set(int vport, int vq, enum mv_pp3_queue_type q_type, int cos);
int pp3_fw_port_mac_addr(int vport, unsigned char *addr);
int pp3_fw_port_l2_filter_mode(int vport, enum mv_nss_l2_option opt, bool state);
int pp3_fw_sync(void);

/* print assist */
void pp3_fw_hwq_stat_print(struct mv_pp3_fw_hwq_stat *stat);
void pp3_fw_swq_stat_print(struct mv_pp3_fw_swq_stat *stat);
void pp3_fw_bmpool_stat_print(struct mv_pp3_fw_bm_pool_stat *stat);
void pp3_fw_msg_stat_print(struct mv_pp3_fw_msg_chan_stat *stat);

#endif /* __mv_pp3_fw_msg_h__ */

