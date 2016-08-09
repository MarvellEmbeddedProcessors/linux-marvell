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

#include <linux/completion.h>
#include <linux/bug.h>
#include <linux/list.h>
#include "common/mv_sw_if.h"
#include "platform/mv_pp3.h"
#include "vport/mv_pp3_vport.h"
#include "vport/mv_pp3_pool.h"
#include "vport/mv_pp3_cpu.h"
#include "msg/mv_pp3_msg_drv.h"
#include "fw/mv_pp3_fw_msg_structs.h"
#include "fw/mv_pp3_fw_msg.h"
#include "fw/mv_fw.h"

#ifdef CONFIG_MV_PP3_FPGA
#include "gmac/mv_gmac.h"
#else /* CONFIG_MV_PP3_FPGA */
#include "gop/mv_gop_if.h"
#endif /* !CONFIG_MV_PP3_FPGA */


static void pp3_fw_struct_size_checker(void)
{
	BUILD_BUG_ON(sizeof(struct mv_pp3_fw_msg_chan_cfg) % 4);
	BUILD_BUG_ON(sizeof(struct mv_pp3_version) % 4);
	BUILD_BUG_ON(sizeof(struct mv_pp3_fw_emac_vport) % 4);
	BUILD_BUG_ON(sizeof(struct mv_pp3_fw_cpu_vport) % 4);
	BUILD_BUG_ON(sizeof(struct mv_pp3_fw_internal_cpu_port) % 4);
	BUILD_BUG_ON(sizeof(struct mv_pp3_fw_cpu_vport_map) % 4);
	BUILD_BUG_ON(sizeof(struct mv_pp3_fw_vport_mac_list) % 4);
	BUILD_BUG_ON(sizeof(struct mv_pp3_fw_vport_mtu) % 4);
	BUILD_BUG_ON(sizeof(struct mv_pp3_fw_vport_def_dest) % 4);
	BUILD_BUG_ON(sizeof(struct mv_pp3_fw_vport_rx_pkt_mode) % 4);
	BUILD_BUG_ON(sizeof(struct mv_pp3_fw_vq_map) % 4);
	BUILD_BUG_ON(sizeof(struct mv_pp3_fw_cos_to_vq) % 4);
	BUILD_BUG_ON(sizeof(struct mv_pp3_fw_bm_pool) % 4);
	BUILD_BUG_ON(sizeof(struct mv_pp3_fw_vport_mac) % 4);
	BUILD_BUG_ON(sizeof(struct mv_pp3_fw_l2_option) % 4);
	BUILD_BUG_ON(sizeof(struct mv_pp3_fw_vport_stat) % 4);
	BUILD_BUG_ON(sizeof(struct mv_pp3_fw_hwq_stat) % 4);
	BUILD_BUG_ON(sizeof(struct mv_pp3_fw_swq_stat) % 4);
	BUILD_BUG_ON(sizeof(struct mv_pp3_fw_bm_pool_stat) % 4);
	BUILD_BUG_ON(sizeof(struct mv_pp3_fw_msg_chan_stat) % 4);
	BUILD_BUG_ON(sizeof(struct mv_pp3_fw_mem_get) % 4);
	BUILD_BUG_ON(sizeof(struct mv_pp3_fw_mem_set) % 4);
	BUILD_BUG_ON(sizeof(struct mv_pp3_fw_reset_stat) % 4);
	BUILD_BUG_ON(sizeof(struct mv_pp3_fw_link_change_note) % 4);
}

/*---------------------------------------------------------------------------
description:
	Callback function used for wait to responce from FW and check return
	info
---------------------------------------------------------------------------*/
static void drv_msg_cb_with_complete(struct drv_msg_cb_params *p)
{
	/*pr_info("Get response from FW for request N%d, return code 0x%x\n", p->req_num, p->ret_code);*/
	complete(&(p->complete));

	return;
}

/*---------------------------------------------------------------------------
description:
	Send any request to FW, wait for acknowledge and check
	return status.
inputs:
	opcode - request opcode
	in_p   - request structure
	in_size - size of request structure in bytes
outputs:
	stat - requested data converted to LE
return values:
		success: 0
		fail: -1
---------------------------------------------------------------------------*/
static inline int pp3_fw_set_wait_req(int opcode, void *in_p, int in_size, void *out_p, int out_size)
{
	struct drv_msg_cb_params req_params;
	struct request_info req_info;
	int req_num;

	init_completion(&req_params.complete);

	/* send message */
	req_info.msg_opcode = opcode;
	req_info.in_param = in_p;
	req_info.size_of_input = in_size;
	req_info.out_buff = out_p;
	req_info.size_of_output = out_size;
	req_info.req_cb = (void (*)(void *))drv_msg_cb_with_complete;
	req_info.cb_params = &req_params;
	req_info.num_of_ints = 1;

	req_num = mv_pp3_drv_request_send(&req_info);
	/* The return value is 0 if timed out, and positive (at least 1, or number of
	 jiffies left till timeout) if completed. */
	if (wait_for_completion_interruptible_timeout(&req_params.complete, 1000) == 0) {
		pr_err("No response from FW for request N%d with opcode %d\n", req_num, opcode);
		/* delete request from list head */
		mv_pp3_drv_request_delete(req_num);
		return -1;
	}
	/* convert reply to OK/FAIL status */
	if (req_params.ret_code != 0) {
		pr_err("Get bad response from FW for request N%d return code 0x%x\n",
			req_params.req_num, req_params.ret_code);
			return -1;
	}
	return 0;
}

/*---------------------------------------------------------------------------
description:
	Send simple "set request" request to FW, no wait for acknowledge and check
	return status.
inputs:
	opcode - request opcode
	in_p   - request structure
	in_size - size of request structure in bytes
outputs:
	none
return values:
		success: 0
		fail: -1
---------------------------------------------------------------------------*/
static inline int pp3_fw_set_simple_req(enum mv_pp3_fw_msg_opcode opcode, void *in_p, int in_size)
{
	struct request_info req_info;
	int ret_val;

	/* send message */
	req_info.msg_opcode = opcode;
	req_info.in_param = in_p;
	req_info.size_of_input = in_size;
	req_info.out_buff = NULL;
	req_info.size_of_output = 0;
	req_info.req_cb = NULL;
	req_info.cb_params = NULL;
	req_info.num_of_ints = 1;

	ret_val = mv_pp3_drv_request_send(&req_info);
	return ret_val;
}

/* get FW version info */
int pp3_fw_version_get(struct mv_pp3_version *fw_ver)
{
	return pp3_fw_set_wait_req(MV_FW_VERSION_GET, NULL, 0, fw_ver, sizeof(struct mv_pp3_version));
}


/* Set RX packet mode for Vport */
int pp3_fw_vport_rx_pkt_mode_set(int vport, enum mv_pp3_pkt_mode mode)
{
	struct mv_pp3_fw_vport_rx_pkt_mode msg;

	memset(&msg, 0, sizeof(struct mv_pp3_fw_vport_rx_pkt_mode));
	msg.int_cpu_port = cpu_to_be16((unsigned short)vport);
	msg.rx_pkt_mode = (unsigned char)mode;

	return pp3_fw_set_simple_req(MV_FW_INTERNAL_CPU_PORT_RX_PKT_MODE_SET,
					&msg, sizeof(struct mv_pp3_fw_vport_rx_pkt_mode));
}

/*---------------------------------------------------------------------------
 description:
	Link change event message. When lik up send speed and duplex configuration

Return:
	>= 0- Message sequence number (message accepted and sent to firmware).
	< 0 - Failure

---------------------------------------------------------------------------*/
int pp3_fw_link_changed(int emac_num, bool link_is_up)
{
	struct mv_pp3_fw_link_change_note msg;
	struct mv_port_link_status port_cfg;

	msg.emac_num = (unsigned char)emac_num;
	msg.link_status = (link_is_up) ? 1 : 0;
	if (link_is_up) {
#ifdef CONFIG_MV_PP3_FPGA
		pp3_gmac_link_status(emac_num, &port_cfg);
#else /* CONFIG_MV_PP3_FPGA */
		mv_pp3_gop_port_link_status(emac_num, &port_cfg);
#endif /* !CONFIG_MV_PP3_FPGA */
		msg.speed = (unsigned char)port_cfg.speed;
		msg.duplex = (unsigned char)port_cfg.duplex;
	} else {
		msg.speed = 0;
		msg.duplex = 0;
	}

	return pp3_fw_set_simple_req(MV_FW_LINK_CHANGE_NOTE, &msg, sizeof(struct mv_pp3_fw_link_change_note));
}

/*---------------------------------------------------------------------------
description:
	Enable / disable virtual port
	Send MV_FW_EMAC_VPORT_SET or MV_FW_CPU_VPORT_SET message

Return:
	>= 0- Message sequence number (message accepted and sent to firmware).
	< 0 - Failure
---------------------------------------------------------------------------*/

int pp3_fw_vport_state_set(int vport, bool port_enable)
{
	struct mv_pp3_fw_vport_state msg;

	memset(&msg, 0, sizeof(struct mv_pp3_fw_vport_state));
	msg.vport = cpu_to_be16((unsigned short)vport);
	msg.state = (port_enable) ? 1 : 0;

	return pp3_fw_set_simple_req(MV_FW_VPORT_STATE_SET, &msg, sizeof(struct mv_pp3_fw_vport_state));
}

/*---------------------------------------------------------------------------
description:
	Set MTU for virtual port
	Send MV_FW_VPORT_MTU_SET message

Return:
	>= 0- Message sequence number (message accepted and sent to firmware).
	< 0 - Failure
---------------------------------------------------------------------------*/

int pp3_fw_vport_mtu_set(int vport, int mtu)
{
	struct mv_pp3_fw_vport_mtu msg;

	memset(&msg, 0, sizeof(struct mv_pp3_fw_vport_state));
	msg.vport = cpu_to_be16((unsigned short)vport);
	msg.mtu = cpu_to_be16((unsigned short)mtu);

	return pp3_fw_set_simple_req(MV_FW_VPORT_MTU_SET, &msg, sizeof(struct mv_pp3_fw_vport_mtu));
}
/*---------------------------------------------------------------------------
description:
	Set defauld destination for virtual port
	Send MV_FW_VPORT_DEF_DEST_SET message

Return:
	>= 0- Message sequence number (message accepted and sent to firmware).
	< 0 - Failure
---------------------------------------------------------------------------*/

int pp3_fw_vport_def_dest_set(int vport, int dest_vport)
{
	struct mv_pp3_fw_vport_def_dest msg;

	memset(&msg, 0, sizeof(struct mv_pp3_fw_vport_def_dest));
	msg.vport = cpu_to_be16((unsigned short)vport);
	msg.def_dst_vport = cpu_to_be16((unsigned short)dest_vport);

	return pp3_fw_set_simple_req(MV_FW_VPORT_DEF_DEST_SET, &msg, sizeof(struct mv_pp3_fw_vport_def_dest));
}
/*---------------------------------------------------------------------------
description:
	Create EMAC virtual port
	By default EMAC virtual port is disabled.
	Send MV_FW_EMAC_VPORT_SET message

Return:
	>= 0- Message sequence number (message accepted and sent to firmware).
	< 0 - Failure
---------------------------------------------------------------------------*/
int pp3_fw_emac_vport_set(struct pp3_vport *vp_cfg, unsigned char *mac)
{
	struct mv_pp3_fw_emac_vport msg;

	memset(&msg, 0, sizeof(struct mv_pp3_fw_emac_vport));
	msg.vport = cpu_to_be16((unsigned short)vp_cfg->vport);
	msg.mtu = cpu_to_be16(vp_cfg->port.emac.mtu);
	msg.vport_dst = cpu_to_be16((unsigned short)vp_cfg->dest_vp);
	msg.state = (unsigned char)vp_cfg->state;
	msg.cos = (unsigned char)vp_cfg->cos;
	msg.l2_options = vp_cfg->port.emac.l2_options;

	memcpy(msg.mac_addr, mac, MV_MAC_ADDR_SIZE);
/*
	pr_info("%s: vport:%d, mtu:%d, cos:%d, vp_dst:%d l2_opt:%02X mac:%02x:%02x:%02x:%02x:%02x:%02x\n",
		__func__, be16_to_cpu(msg.vport), be16_to_cpu(msg.mtu),
		msg.cos, be16_to_cpu(msg.vport_dst), msg.l2_options,
		msg.mac_addr[0], msg.mac_addr[1], msg.mac_addr[2],
		msg.mac_addr[3], msg.mac_addr[4], msg.mac_addr[5]);
*/
	return pp3_fw_set_simple_req(MV_FW_EMAC_VPORT_SET, &msg, sizeof(struct mv_pp3_fw_emac_vport));
}
/*---------------------------------------------------------------------------*/

int pp3_fw_emac_vport_msg_get(int vport, struct mv_pp3_fw_emac_vport *out_msg)
{
	int err;
	struct mv_pp3_fw_emac_vport in_msg;

	memset(&in_msg, 0, sizeof(in_msg));
	memset(out_msg, 0, sizeof(*out_msg));

	in_msg.vport = cpu_to_be16((unsigned short)vport);

	err = pp3_fw_set_wait_req(MV_FW_EMAC_VPORT_GET, &in_msg, sizeof(in_msg),
						out_msg, sizeof(*out_msg));
	return err;
}
/*---------------------------------------------------------------------------*/

int pp3_fw_emac_vport_msg_show(int vport)
{
	int err = -1;
	struct mv_pp3_fw_emac_vport out_msg;

	if (pp3_vports[vport]) {
		pr_info("---- EMAC virtual port #%d information from Firmware ----\n", vport);
		err = pp3_fw_emac_vport_msg_get(vport, &out_msg);
	}
	if (!err) {
		pr_info("state       : %s\n", out_msg.state ? "Enable" : "Disable");
		pr_info("mtu         : %u\n", out_msg.mtu);
		pr_info("def_dst_vp  : %u\n", out_msg.vport_dst);
		pr_info("cos         : %u\n", out_msg.cos);
		pr_info("L2 options  : 0x%02x\n", out_msg.l2_options);
		mv_mac_addr_print("ucast MAC   :", &out_msg.mac_addr[0], NULL);
	} else
		pr_err("vport #%d FW information is unavailable. err = %d\n", vport, err);
	return err;
}
/*---------------------------------------------------------------------------*/

/*---------------------------------------------------------------------------
description:
	Create and enable CPU virtual port
	Send MV_FW_INTERNAL_CPU_VPORT_SET message

Return:
	>= 0- Message sequence number (message accepted and sent to firmware).
	< 0 - Failure
---------------------------------------------------------------------------*/

int pp3_fw_internal_cpu_vport_set(struct pp3_vport *vp_cfg)
{
	struct mv_pp3_fw_internal_cpu_port msg;

	memset(&msg, 0, sizeof(struct mv_pp3_fw_internal_cpu_port));

	msg.int_cpu_port = cpu_to_be16((unsigned short)vp_cfg->vport);
	msg.rx_pkt_mode = (unsigned char)vp_cfg->port.cpu.cpu_shared->rx_pkt_mode;

	if (vp_cfg->port.cpu.cpu_shared->long_pool)
		msg.bm_long_pool = (unsigned char)vp_cfg->port.cpu.cpu_shared->long_pool->pool;
	else {
		pr_err("%s: long pool must be initialized\n", __func__);
		return -1;
	}

	msg.bm_short_pool = (vp_cfg->port.cpu.cpu_shared->short_pool) ?
					(unsigned char)vp_cfg->port.cpu.cpu_shared->short_pool->pool : -1;

	msg.bm_lro_pool = (vp_cfg->port.cpu.cpu_shared->lro_pool) ?
					(unsigned char)vp_cfg->port.cpu.cpu_shared->lro_pool->pool : -1;

	return pp3_fw_set_simple_req(MV_FW_INTERNAL_CPU_PORT_SET, &msg, sizeof(struct mv_pp3_fw_internal_cpu_port));
}
/*---------------------------------------------------------------------------
description:
	Set CPU virtual port
	Send MV_FW_CPU_VPORT_SET message
Return:
	>= 0- Message sequence number (message accepted and sent to firmware).
	< 0 - Failure
---------------------------------------------------------------------------*/
int pp3_fw_cpu_vport_set(int vport)
{
	struct mv_pp3_fw_cpu_vport msg;

	memset(&msg, 0, sizeof(struct mv_pp3_fw_cpu_vport));

	msg.vport = cpu_to_be16((unsigned short)vport);
	msg.vport_dst = cpu_to_be16((unsigned short)MV_NSS_PORT_DROP);
	msg.state = 1;
	msg.cos = 0;

	return pp3_fw_set_simple_req(MV_FW_CPU_VPORT_SET, &msg, sizeof(struct mv_pp3_fw_cpu_vport));
}
/*---------------------------------------------------------------------------
description:
	Set internal virtual port to be used between
	EMAC/WLAN virtual port and CPU virtual port
	Send MV_FW_CPU_VPORT_MAP message

Return:
	>= 0- Message sequence number (message accepted and sent to firmware).
	< 0 - Failure
---------------------------------------------------------------------------*/

int pp3_fw_cpu_vport_map(int vport, int cpu_vp, int int_cpu_vp)
{
	struct mv_pp3_fw_cpu_vport_map msg;

	memset(&msg, 0, sizeof(struct mv_pp3_fw_cpu_vport_map));

	msg.vport = cpu_to_be16((unsigned short)vport);
	msg.cpu_vport = cpu_to_be16((unsigned short)cpu_vp);
	msg.int_cpu_port = cpu_to_be16((unsigned short)int_cpu_vp);
/*
	pr_info("%s: vport:%d, cpu_vp:%d int_cpu_vp:%d\n",
			__func__, be16_to_cpu(msg.vport), be16_to_cpu(msg.cpu_vport), be16_to_cpu(msg.int_cpu_port));
*/
	return pp3_fw_set_simple_req(MV_FW_CPU_VPORT_MAP, &msg, sizeof(struct mv_pp3_fw_cpu_vport_map));
}

/*---------------------------------------------------------------------------
description:
	set list of mac addresses for EMAC virtual port
	By default EMAC virtual port is disabled.
	Send MV_FW_VPORT_MAC_LIST_SET message

Return:
	>= 0- Message sequence number (message accepted and sent to firmware).
	< 0 - Failure
---------------------------------------------------------------------------*/
int pp3_fw_vport_mac_list_set(int vport, unsigned char macs_list_size, unsigned char *macs_list)
{
	struct mv_pp3_fw_vport_mac_list msg;
	int i;

	memset(&msg, 0, sizeof(struct mv_pp3_fw_vport_mac_list));

	msg.vport = cpu_to_be16((unsigned short)vport);

	for (i = 0; i < macs_list_size && i < MV_PP3_MAC_ADDR_NUM; i++)
		memcpy(msg.mac_addr_list[i], &macs_list[i * MV_MAC_ADDR_SIZE], MV_MAC_ADDR_SIZE);

	msg.mac_addr_list_size = i;

	return pp3_fw_set_simple_req(MV_FW_VPORT_MAC_LIST_SET, &msg, sizeof(msg));
}
/*---------------------------------------------------------------------------*/

/* Get list of MAC addresses for EMAC virtual port */
int pp3_fw_vport_mac_list_get(int vport,  unsigned char macs_list_size, unsigned char *macs_list,
					unsigned char *macs_valid)
{
	int i;
	struct mv_pp3_fw_vport_mac_list in_msg, out_msg;

	in_msg.vport = cpu_to_be16((unsigned short)vport);
	in_msg.mac_addr_list_size = macs_list_size;

	if (pp3_fw_set_wait_req(MV_FW_VPORT_MAC_LIST_GET, &in_msg, sizeof(in_msg),
							&out_msg, sizeof(out_msg)) == 0) {

		if (out_msg.mac_addr_list_size > macs_list_size)
			out_msg.mac_addr_list_size = macs_list_size;

		*macs_valid = out_msg.mac_addr_list_size;
		for (i = 0; i < out_msg.mac_addr_list_size; i++)
			memcpy(&macs_list[i * MV_MAC_ADDR_SIZE], out_msg.mac_addr_list[i], MV_MAC_ADDR_SIZE);

		return 0;
	}
	return -1;
}

/*---------------------------------------------------------------------------
description:
	BM pool configurationn
	Send MV_FW_BM_POOL_SET message

Return:
	>= 0- Message sequence number (message accepted and sent to firmware).
	< 0 - Failure
---------------------------------------------------------------------------*/
int pp3_fw_bm_pool_set(struct pp3_pool *pool)
{
	struct mv_pp3_fw_bm_pool msg;

	memset(&msg, 0, sizeof(struct mv_pp3_fw_bm_pool));
	msg.bm_pool_num = pool->pool;
	msg.buf_headroom = (pool->headroom / MV_PP3_BM_POOL_HROOM_RES);
	msg.buf_size = cpu_to_be16(pool->pkt_max_size + pool->headroom);
	/* pair mode */
	msg.pe_size = 1;

	return pp3_fw_set_simple_req(MV_FW_BM_POOL_SET, &msg, sizeof(struct mv_pp3_fw_bm_pool));
}

/*---------------------------------------------------------------------------
description:
	Ingress / egress VQ to SWQ/HWQ map configuration
	Send MV_FW_VQ_MAP_SET message

Return:
	>= 0- Message sequence number (message accepted and sent to firmware).
	< 0 - Failure
---------------------------------------------------------------------------*/
int pp3_fw_vq_map_set(int vport, int vq, enum mv_pp3_queue_type q_type, int swq, int hwq)
{
	struct mv_pp3_fw_vq_map msg;

	memset(&msg, 0, sizeof(struct mv_pp3_fw_vq_map));
	msg.vport = cpu_to_be16((unsigned short)vport);
	msg.type = (unsigned char)q_type;
	msg.vq = (unsigned char)vq;
	msg.hwq = cpu_to_be16((unsigned short)hwq);
	msg.swq = (unsigned char)swq;
/*
	pr_info("%s: sent: vport:%d, type:%d, vq:%d, swq:%d, hwq:%d\n", __func__,
		cpu_to_be16(msg.vport), msg.type, msg.vq, msg.swq, be16_to_cpu(msg.hwq));
*/
	return pp3_fw_set_simple_req(MV_FW_VQ_MAP_SET, &msg, sizeof(struct mv_pp3_fw_vq_map));
}

/*---------------------------------------------------------------------------
description:
	Map ingress/egress CoS value to VQ per port
	Send MV_FW_COS_TO_VQ_SET message

Return:
	>= 0- Message sequence number (message accepted and sent to firmware).
	< 0 - Failure
---------------------------------------------------------------------------*/
int pp3_fw_cos_to_vq_set(int vport, int vq, enum mv_pp3_queue_type q_type, int cos)
{
	struct mv_pp3_fw_cos_to_vq msg;

	msg.vport = cpu_to_be16((unsigned short)vport);
	msg.type = (unsigned char) q_type;
	msg.vq = (unsigned char) vq;
	msg.cos = (unsigned char) cos;
/*
	pr_info("%s: sent: vport:%d, type:%d, vq:%d, cos:%d\n", __func__, q_type,
		cpu_to_be16(msg.vport), msg.vq, msg.cos);
*/
	return pp3_fw_set_simple_req(MV_FW_COS_TO_VQ_SET, &msg, sizeof(struct mv_pp3_fw_cos_to_vq));
}

/*---------------------------------------------------------------------------
description:
	Set virtual port MAC addres
	Send MV_FW_VPORT_MAC_SET message

Return:
	>= 0- Message sequence number (message accepted and sent to firmware).
	< 0 - Failure

---------------------------------------------------------------------------*/
int pp3_fw_port_mac_addr(int vport, unsigned char *addr)
{
	struct mv_pp3_fw_vport_mac msg;

	memset(&msg, 0, sizeof(struct mv_pp3_fw_vport_mac));
	msg.vport = cpu_to_be16((unsigned short)vport);
	memcpy(msg.mac, addr, MV_MAC_ADDR_SIZE);

	return pp3_fw_set_simple_req(MV_FW_VPORT_MAC_SET, &msg, sizeof(struct mv_pp3_fw_vport_mac));
}

/*---------------------------------------------------------------------------
description:
	Set L2 filter mode for virtual port
	Send MV_FW_L2_FILTER_SET message

Return:
	>= 0- Message sequence number (message accepted and sent to firmware).
	< 0 - Failure
---------------------------------------------------------------------------*/
int pp3_fw_port_l2_filter_mode(int vport, enum mv_nss_l2_option opt, bool state)
{
	struct mv_pp3_fw_l2_option msg;

	memset(&msg, 0, sizeof(struct mv_pp3_fw_l2_option));
	msg.vport = cpu_to_be16((unsigned char) vport);
	msg.option = (unsigned char) opt;
	msg.state = (state) ? 1 : 0;

	return pp3_fw_set_simple_req(MV_FW_VPORT_L2_OPTION_SET, &msg, sizeof(struct mv_pp3_fw_l2_option));
}

/*---------------------------------------------------------------------------
description:
	Send request for memory buffer size needed by FW

Return:
	>= 0- memory buffer size
	< 0 - Failure
---------------------------------------------------------------------------*/
int pp3_fw_mem_bufs_alloc_size_get(void)
{
	struct mv_pp3_fw_mem_get msg;
	struct mv_pp3_fw_mem_get req_size;		/* buffer size in kbytes */

	msg.size = 0;

	if (pp3_fw_set_wait_req(MV_FW_MEM_REQ_GET, &msg, sizeof(msg), &req_size, sizeof(req_size)) < 0)
		return -1;

	return be32_to_cpu(req_size.size);
}

/*---------------------------------------------------------------------------
description:
	Send message to FW with allocated buffers addresses

Return:
	>= 0- Message sequence number (message accepted and sent to firmware).
	< 0 - Failure
---------------------------------------------------------------------------*/
int pp3_fw_mem_bufs_alloc_set(unsigned int size, unsigned int buf_num, unsigned int *bufs)
{
	struct mv_pp3_fw_mem_set res;
	struct request_info req_info;
	int i, ret_val = 0;

	memset(&res, 0, sizeof(res));
	res.buf_num = cpu_to_be16(buf_num);
	res.size = cpu_to_be16(size/buf_num);

	for (i = 0; i < buf_num; i++)
		res.buffer[i] = bufs[i];

	/* fill request structure */
	req_info.msg_opcode = MV_FW_MEM_REQ_SET;
	req_info.in_param = &res;
	req_info.size_of_input = sizeof(res);
	req_info.out_buff = NULL;
	req_info.size_of_output = 0;
	req_info.req_cb = NULL;
	req_info.cb_params = NULL;
	req_info.num_of_ints = 1;

	ret_val = mv_pp3_drv_request_send(&req_info);

	return ret_val;
}

/*---------------------------------------------------------------------------
description:
	Send syncronization msg and wait for completion

return values:
		success: msg return code
		fail: -1
---------------------------------------------------------------------------*/
int pp3_fw_sync(void)
{
	struct request_info req_info;
	struct drv_msg_cb_params req_params;
	int msg_num;

	pp3_fw_struct_size_checker();

	/* send sync message and wait to complete */
	init_completion(&req_params.complete);

	/* fill request structure */
	req_info.msg_opcode = MV_IDLE_MSG;
	req_info.in_param = NULL;
	req_info.size_of_input = 0;
	req_info.out_buff = NULL;
	req_info.size_of_output = 0;
	req_info.req_cb = (void (*)(void *))drv_msg_cb_with_complete;
	req_info.cb_params = &req_params;
	req_info.num_of_ints = 1;

	msg_num = mv_pp3_drv_request_send(&req_info);
	/* The return value is 0 if timed out, and positive (at least 1, or number of
	 jiffies left till timeout) if completed. */
	if (wait_for_completion_interruptible_timeout(&req_params.complete, 1000) == 0) {
		pr_err("No response from FW. System doesn't configured properly\n");
		mv_pp3_drv_request_delete(msg_num);
		return -1;
	}

	if (req_params.ret_code != 0)
		pr_err("Get bad response from FW, ret_val = %d\nSystem doesn't ready\n", req_params.ret_code);

	return req_params.ret_code;
}

/*---------------------------------------------------------------------------
description:
	Send request for vport statistics

Return:
	= 0 - Success.
	< 0 - Failure.
---------------------------------------------------------------------------*/
int pp3_fw_vport_stat_get(int vport, struct mv_pp3_fw_vport_stat *vport_stat)
{
	unsigned short param;

	/* vport validity checked in caller */
	param = cpu_to_be16((unsigned short)vport);
	if (pp3_fw_set_wait_req(MV_FW_VPORT_STATS_GET, &param, sizeof(param), vport_stat,
		sizeof(struct mv_pp3_fw_vport_stat)) == 0)
		mv_be32_convert((u32 *)vport_stat, sizeof(struct mv_pp3_fw_vport_stat)/sizeof(u32));
	else
		return -1;

	return 0;
}

/*---------------------------------------------------------------------------
description:
	Send request for HW queue statistics

Return:
	= 0 - Success.
	< 0 - Failure.
---------------------------------------------------------------------------*/
int pp3_fw_hwq_stat_get(unsigned short q, bool clean, struct mv_pp3_fw_hwq_stat *hw_stat)
{
	struct mv_pp3_fw_hwq_stats_get msg_stat;

	if (!mv_pp3_fw_is_available())
		return -1;

	msg_stat.hwq = cpu_to_be16(q);
	msg_stat.clear = clean;
	msg_stat.reserved = 0;

	if (pp3_fw_set_wait_req(MV_FW_HWQ_STATS_GET, &msg_stat, sizeof(struct mv_pp3_fw_hwq_stats_get), hw_stat,
		sizeof(struct mv_pp3_fw_hwq_stat)) == 0)
		mv_be32_convert((u32 *)hw_stat, sizeof(struct mv_pp3_fw_hwq_stat)/sizeof(u32));
	else
		return -1;

	return 0;
}

/*---------------------------------------------------------------------------
description:
	Send request for SW queue statistics

Return:
	= 0 - Success.
	< 0 - Failure.
---------------------------------------------------------------------------*/
int pp3_fw_swq_stat_get(unsigned char q, struct mv_pp3_fw_swq_stat *sw_stat)
{
	if (!mv_pp3_fw_is_available())
		return -1;

	if (pp3_fw_set_wait_req(MV_FW_SWQ_STATS_GET, &q, sizeof(unsigned char), sw_stat,
		sizeof(struct mv_pp3_fw_swq_stat)) == 0)
		mv_be32_convert((u32 *)sw_stat, sizeof(struct mv_pp3_fw_swq_stat)/sizeof(u32));
	else
		return -1;

	return 0;
}

/*---------------------------------------------------------------------------
description:
	Send request for BM pool statistics

Return:
	= 0 - Success.
	< 0 - Failure.
---------------------------------------------------------------------------*/
int pp3_fw_bm_pool_stat_get(unsigned char q, struct mv_pp3_fw_bm_pool_stat *stat)
{
	if (!mv_pp3_fw_is_available())
		return -1;

	if (pp3_fw_set_wait_req(MV_FW_BM_POOL_STATS_GET, &q, sizeof(unsigned char), stat,
		sizeof(struct mv_pp3_fw_bm_pool_stat)) == 0)
		mv_be32_convert((u32 *)stat, sizeof(struct mv_pp3_fw_bm_pool_stat)/sizeof(u32));
	else
		return -1;

	return 0;
}

/*---------------------------------------------------------------------------
description:
	Send request for messenger channel statistics

Return:
	= 0 - Success.
	< 0 - Failure.
---------------------------------------------------------------------------*/
int pp3_fw_channel_stat_get(unsigned char q, struct mv_pp3_fw_msg_chan_stat *stat)
{
	if (!mv_pp3_fw_is_available())
		return -1;

	if (pp3_fw_set_wait_req(MV_FW_MSG_CHAN_STATS_GET, &q, sizeof(unsigned char), stat,
		sizeof(struct mv_pp3_fw_msg_chan_stat)) == 0)
		mv_be32_convert((u32 *)stat, sizeof(struct mv_pp3_fw_msg_chan_stat)/sizeof(u32));
	else
		return -1;

	return 0;
}

/*---------------------------------------------------------------------------
description:
	Send request for messenger channel statistics

Return:
	= 0 - Success.
	< 0 - Failure.
---------------------------------------------------------------------------*/
int pp3_fw_clear_stat_set(unsigned char stat_type, unsigned short num)
{
	struct mv_pp3_fw_reset_stat msg_stat;

	if (!mv_pp3_fw_is_available())
		return -1;

	msg_stat.type = stat_type;
	msg_stat.reserved = 0;
	msg_stat.index = cpu_to_be16(num);

	if (pp3_fw_set_wait_req(MV_FW_RESET_STATISTICS, &msg_stat, sizeof(struct mv_pp3_fw_reset_stat), NULL, 0) != 0)
		return -1;

	return 0;
}

void pp3_fw_hwq_stat_print(struct mv_pp3_fw_hwq_stat *stat)
{
	pr_info("Number of enqueue packets high %10u\n", stat->hwq_pkt_high);
	pr_info("Number of enqueue packets low  %10u\n", stat->hwq_pkt_low);
	pr_info("Number of enqueue octets  high %10u\n", stat->hwq_oct_high);
	pr_info("Number of enqueue octets  low  %10u\n", stat->hwq_oct_low);
	pr_info("Number of dropped packets high %10u\n", stat->hwq_pkt_drop_high);
	pr_info("Number of dropped packets low  %10u\n", stat->hwq_pkt_drop_low);
}

void pp3_fw_swq_stat_print(struct mv_pp3_fw_swq_stat *stat)
{
	pr_info("Number of enqueue to specific SWQ failed          %10u\n", stat->swq_enq_err_cntr);
	pr_info("Number of enqueue to specific SWQ success high    %10u\n", stat->swq_enq_cntr_high);
	pr_info("Number of enqueue to specific SWQ success low     %10u\n", stat->swq_enq_cntr_low);
}

void pp3_fw_bmpool_stat_print(struct mv_pp3_fw_bm_pool_stat *stat)
{
	pr_info("Number of times FW allocate buffers from specific BM pool    %10u\n", stat->bm_alloc_cntr);
	pr_info("Number of times FW free buffer to specific BM pool           %10u\n", stat->bm_free_cntr);
}

void pp3_fw_msg_stat_print(struct mv_pp3_fw_msg_chan_stat *stat)
{
	pr_info("Number of requests received by FW              %10u\n", stat->msg_request_cntr);
	pr_info("Number of requests that FW failed to execute   %10u\n", stat->msg_request_err);
	pr_info("Number of replies sent by FW                   %10u\n", stat->msg_reply_cntr);
	pr_info("Number of events sent by FW                    %10u\n", stat->msg_event_cntr);
}
