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
* ***************************************************************************
*/

/* This file provides interface between external application and driver functionality */

#include "platform/mv_pp3_defs.h"
#include <linux/types.h>
#include <net/gnss/mv_nss_defs.h>

int mv_pp3_gnss_sys_init(void);
int mv_pp3_gnss_dev_create(unsigned short vport, bool state, unsigned char *mac);
int mv_pp3_gnss_dev_delete(unsigned short vport);
int mv_pp3_gnss_dev_rx_cpus_set(unsigned int rx_cpus_mask);
int mv_pp3_gnss_dev_rxqs_set(unsigned int rxqs);
int mv_pp3_gnss_dev_txqs_set(unsigned int txqs);
void mv_pp3_gnss_dev_init_show(void);
enum mv_nss_port_type mv_pp3_gnss_vport_type_get(unsigned short vport);

/* Virtual port functions */
int mv_pp3_gnss_vport_mtu_get(unsigned short vport);
int mv_pp3_gnss_vport_mtu_set(unsigned short vport, int mtu);

int mv_pp3_gnss_vport_state_get(unsigned short vport);
int mv_pp3_gnss_vport_state_set(unsigned short vport, bool enable);

int mv_pp3_gnss_vport_def_dst_get(unsigned short vport);
int mv_pp3_gnss_vport_def_dst_set(unsigned short vport, unsigned short def_dst);

int mv_pp3_gnss_vport_ucast_list_get(unsigned short vport, unsigned char *mac_list, int max_num, int *num);
int mv_pp3_gnss_vport_ucast_list_set(unsigned short vport, unsigned char *mac_list, int num);
/* Delete (op=0) / Add (op=1) unicast address from L2 filter */
int mv_pp3_gnss_vport_ucast_addr_set(unsigned short vport, unsigned char *mac_addr, int op);

int mv_pp3_gnss_vport_mcast_list_get(unsigned short vport, unsigned char *mac_list, int max_num, int *num);
int mv_pp3_gnss_vport_mcast_list_set(unsigned short vport, unsigned char *mac_list, int num);
/* Delete (op=0) / Add (op=1) multicast address from L2 filter */
int mv_pp3_gnss_vport_mcast_addr_set(unsigned short vport, unsigned char *mac_addr, int op);

int mv_pp3_gnss_vport_l2_options_get(unsigned short vport, unsigned char *l2_options);
int mv_pp3_gnss_vport_l2_options_set(unsigned short vport, unsigned char l2_options);

int mv_pp3_gnss_vport_cos_get(unsigned short vport, unsigned char *cos);
int mv_pp3_gnss_vport_cos_set(unsigned short vport, unsigned char cos);

/* Function for VQ configuration */
/* [vport] argument is EMAC or External virtual port. */
int mv_pp3_gnss_ingress_vq_drop_set(unsigned short vport, int vq, struct mv_nss_drop *drop);
int mv_pp3_gnss_ingress_vq_drop_get(unsigned short vport, int vq, struct mv_nss_drop *drop);
int mv_pp3_gnss_ingress_vq_sched_set(unsigned short vport, int vq, struct mv_nss_sched *sched);
int mv_pp3_gnss_ingress_vq_sched_get(unsigned short vport, int vq, struct mv_nss_sched *sched);

int mv_pp3_gnss_ingress_cos_to_vq_set(unsigned short vport, int cos, int vq);
int mv_pp3_gnss_ingress_cos_to_vq_get(unsigned short vport, int cos, int *vq);

int mv_pp3_gnss_ingress_vq_size_set(unsigned short vport, int vq, u16 length);
int mv_pp3_gnss_ingress_vq_size_get(unsigned short vport, int vq, u16 *length);


int mv_pp3_gnss_egress_vq_drop_set(unsigned short vport, int vq, struct mv_nss_drop *drop);
int mv_pp3_gnss_egress_vq_drop_get(unsigned short vport, int vq, struct mv_nss_drop *drop);
int mv_pp3_gnss_egress_vq_sched_set(unsigned short vport, int vq, struct mv_nss_sched *sched);
int mv_pp3_gnss_egress_vq_sched_get(unsigned short vport, int vq, struct mv_nss_sched *sched);

int mv_pp3_gnss_egress_cos_to_vq_set(unsigned short vport, int cos, int vq);
int mv_pp3_gnss_egress_cos_to_vq_get(unsigned short vport, int cos, int *vq);

int mv_pp3_gnss_egress_vq_size_set(unsigned short vport, int vq, u16 length);
int mv_pp3_gnss_egress_vq_size_get(unsigned short vport, int vq, u16 *length);

int mv_pp3_gnss_egress_vq_rate_limit_set(unsigned short vport, int vq, struct mv_nss_meter *meter);
int mv_pp3_gnss_egress_vq_rate_limit_get(unsigned short vport, int vq, struct mv_nss_meter *meter);

int mv_pp3_gnss_ingress_vport_stats_init(int vport, unsigned int msec); /* stop to collect statistics if if msec = 0*/
int mv_pp3_gnss_ingress_msec_all_set(unsigned int msec);
int mv_pp3_gnss_ingress_vport_stats_get(int vport, bool clean, int size, struct mv_nss_vq_stats res_stats[]);
int mv_pp3_gnss_ingress_vport_ext_stats_get(int vport, bool clean, int size,
						struct mv_nss_vq_advance_stats res_stats[]);
int mv_pp3_gnss_ingress_vport_ext_stats_clean(int vport);
int mv_pp3_gnss_ingress_vport_stats_clean(int vport);
int mv_pp3_gnss_ext_vport_msec_get(void);

int mv_pp3_gnss_state_get(bool *state);

