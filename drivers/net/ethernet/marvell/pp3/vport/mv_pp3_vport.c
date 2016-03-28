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

#include "platform/mv_pp3.h"
#include "platform/mv_pp3_config.h"

#include "mv_pp3_vport.h"
#include "mv_pp3_vq.h"
#include "mv_pp3_cpu.h"
#include "gop/mv_gop_if.h"
#include "gop/mv_ptp_if.h"
#include "net_dev/mv_ptp_service.h"
#include "emac/mv_emac.h"
#include "hmac/mv_hmac.h"
#include "qm/mv_qm.h"
#include "gop/mv_smi.h"
#include "tm/wrappers/mv_tm_scheme.h"
#include "tm/wrappers/mv_tm_shaping.h"
#include "fw/mv_pp3_fw_msg.h"

#define PP3_DBG_GROUP_CNT_DUMP(vp, num, name)\
	mv_pp3_cpu_vport_cnt(vp, num, offsetof(struct pp3_cpu_vp_stats, name)/4, #name)

int mv_pp3_rx_pkts_to_dg(enum mv_pp3_pkt_mode pkt_mode, int pkts)
{
	int dg = 0;

	/* CFH size depends on rx_pkt_mode */
	if (pkt_mode == MV_PP3_PKT_DRAM)
		dg = pkts * MV_PP3_CFH_PKT_DG_SIZE;
	else if (pkt_mode == MV_PP3_PKT_CFH)
		dg = (pkts * MV_PP3_CFH_DG_MAX_NUM);
	else
		pr_err("unknown rx_pkt_mode #%d\n", pkt_mode);

	return dg;
}

int mv_pp3_rx_dg_to_pkts(enum mv_pp3_pkt_mode pkt_mode, int dg)
{
	int pkts = 0;

	/* CFH size depends on rx_pkt_mode */
	if (pkt_mode == MV_PP3_PKT_DRAM)
		pkts = dg / MV_PP3_CFH_PKT_DG_SIZE;
	else if (pkt_mode == MV_PP3_PKT_CFH)
		pkts = dg / MV_PP3_CFH_DG_MAX_NUM;
	else
		pr_err("unknown rx_pkt_mode #%d\n", pkt_mode);
	return pkts;
}
/*---------------------------------------------------------------------------*/

/* array of virtual ports driver work with.                             */
/* this array doesn't store application virtual ports                   */
/* internal cpu ports are stored start from (MV_PP3_NSS_EXT_PORT_MAX + 1)  */
struct pp3_vport	**pp3_vports;
static int		pp3_vports_num;

/* Once time called vport component initialization function */
int mv_pp3_vports_global_init(struct mv_pp3 *priv, int vports_num)
{
	pp3_vports = kzalloc(vports_num * sizeof(struct pp3_vport *), GFP_KERNEL);
	if (!pp3_vports)
		return -ENOMEM;

	pp3_vports_num = vports_num;
	return 0;
}

struct pp3_vport *mv_pp3_vport_alloc(int vp, enum mv_nss_port_type vp_type, int rxqs_num, int txqs_num)
{
	struct pp3_vport *vport;
	enum mv_pp3_queue_type vq_type;
	int vq;

	/* Alloc group */
	vport = kzalloc(sizeof(struct pp3_vport), GFP_KERNEL);
	if (!vport)
		goto oom;

	/* External vport (WLAN) - skip VQs allocation */
	if (vp_type != MV_PP3_NSS_PORT_EXT) {

		vq_type = (vp_type == MV_PP3_NSS_PORT_CPU) ? MV_PP3_PPC_TO_HMAC : MV_PP3_EMAC_TO_PPC;
		/* Allocate pointers for ingress virtual queues */
		for (vq = 0; vq < rxqs_num; vq++) {
			vport->rx_vqs[vq] = mv_pp3_vq_alloc(vq, vq_type);
			if (!vport->rx_vqs[vq])
				goto oom;
		}

		vq_type = (vp_type == MV_PP3_NSS_PORT_CPU) ? MV_PP3_HMAC_TO_PPC : MV_PP3_PPC_TO_EMAC;
		/* Allocate pointers for egress virtual queues */
		for (vq = 0; vq < txqs_num; vq++) {
			vport->tx_vqs[vq] = mv_pp3_vq_alloc(vq, vq_type);
			if (!vport->tx_vqs[vq])
				goto oom;
		}

		vport->rx_vqs_num = rxqs_num;
		vport->tx_vqs_num = txqs_num;
	}

	vport->type = vp_type;
	vport->vport = vp;
	vport->dest_vp = MV_NSS_PORT_NONE;

	if (vp_type == MV_PP3_NSS_PORT_CPU)
		vp += MV_PP3_INTERNAL_CPU_PORT_BASE;

	pp3_vports[vp] = vport;

	return vport;
oom:
	mv_pp3_vport_delete(vport);
	pr_err("%s: Out of memory\n", __func__);
	return NULL;
}
/*---------------------------------------------------------------------------*/

int mv_pp3_cpu_vport_sw_init(struct pp3_vport *vport, struct cpumask *rx_cpus, int cpu)
{
	int vq, irq_group, dg, i;

	MV_PP3_NULL_PTR(vport, err);

	if (vport->type != MV_PP3_NSS_PORT_CPU) {
		pr_err("%s: vport %d - incorrect type\n", __func__, vport->vport);
		goto err;
	}

	/* init NAPI related */
	vport->port.cpu.napi_q_num = vport->rx_vqs_num;
	vport->port.cpu.napi_master_array = 0;
	vport->port.cpu.napi_next_array = 0;

	if (cpumask_test_cpu(cpu, rx_cpus)) {
		if (mv_pp3_cfg_dp_gen_irq_group(vport->vport, cpu, &irq_group) < 0)
			goto err;

		vport->port.cpu.irq_num = mv_pp3_cfg_rx_irq_get(vport->vport, irq_group);

		for (vq = 0; vq < vport->rx_vqs_num; vq++) {
			if (vport->rx_vqs[vq]) {
				if (mv_pp3_ingress_cpu_vq_sw_init(vport->vport, vport->rx_vqs[vq], irq_group)) {
					pr_err("%s: vport #%d failed to init ingress vq #%d cpu #%d\n", __func__,
						vport->vport, vq, cpu);
					goto err;
				}
				dg = mv_pp3_rx_pkts_to_dg(vport->port.cpu.cpu_shared->rx_pkt_mode, 1);
				mv_pp3_swq_cfh_size_set(vport->rx_vqs[vq]->swq, dg);

				for (i = 0; i < 3; i++)
					vport->port.cpu.napi_proc_qs[i][vq] = vport->rx_vqs[vq]->vq;
			}
		}
	}

	for (vq = 0; vq < vport->tx_vqs_num; vq++) {
		if (vport->tx_vqs[vq]) {
			if (mv_pp3_egress_cpu_vq_sw_init(vport->vport, vport->tx_vqs[vq], cpu)) {
				pr_err("%s: vport #%d failed to init egress vq #%d cpu #%d\n", __func__,
					vport->vport, vq, cpu);
				goto err;
			}
		}
	}
	vport->port.cpu.cpu_num = cpu;

	return 0;
err:
	pr_err("%s: function failed\n", __func__);
	return -1;

}
/*---------------------------------------------------------------------------*/

int mv_pp3_emac_vport_sw_init(struct pp3_vport *vport, int emac, struct mv_mac_data *mac_data)
{
	int vq;

	MV_PP3_NULL_PTR(vport, err);
	MV_PP3_NULL_PTR(mac_data, err);

	if (vport->type != MV_PP3_NSS_PORT_ETH) {
		pr_err("%s: vport %d - incorrect type\n", __func__, vport->vport);
		goto err;
	}

	for (vq = 0; vq < vport->rx_vqs_num; vq++) {
		if (vport->rx_vqs[vq]) {
			if (mv_pp3_ingress_emac_vq_sw_init(vport->rx_vqs[vq], emac)) {
				pr_err("%s: vport #%d failed to init ingress vq #%d emac #%d\n", __func__,
					vport->vport, vq, emac);
				goto err;
			}
		}
	}
	for (vq = 0; vq < vport->tx_vqs_num; vq++) {
		if (vport->tx_vqs[vq]) {
			if (mv_pp3_egress_emac_vq_sw_init(vport->tx_vqs[vq], emac)) {
				pr_err("%s: vport #%d failed to init egress vq #%d emac #%d\n", __func__,
					vport->vport, vq, emac);
				goto err;
			}
		}
	}

	vport->port.emac.emac_num = emac;
	vport->port.emac.lc_irq_num = mac_data->link_irq;
	vport->port.emac.port_mode = mac_data->port_mode;
	vport->port.emac.force_link = mac_data->force_link;
	vport->port.emac.phy_addr = mac_data->phy_addr;
	vport->port.emac.flags = 0;
	vport->port.emac.l2_options = MV_NSS_NON_PROMISC_MODE;
	return 0;
err:
	pr_err("%s: function failed\n", __func__);
	return -1;
}
/*---------------------------------------------------------------------------*/

int mv_pp3_cpu_vport_hw_init(struct pp3_vport *vport)
{
	int vq;

	MV_PP3_NULL_PTR(vport, err);

	for (vq = 0; vq < vport->rx_vqs_num; vq++) {
		if (vport->rx_vqs[vq]) {
			if (mv_pp3_vq_hw_init(vport->rx_vqs[vq])) {
				pr_err("%s: vport #%d failed to init ingress vq #%d\n", __func__,
					vport->vport, vq);
				goto err;
			}
		}
	}

	for (vq = 0; vq < vport->tx_vqs_num; vq++) {
		if (vport->tx_vqs[vq]) {
			if (mv_pp3_vq_hw_init(vport->tx_vqs[vq])) {
				pr_err("%s: vport #%d failed to init egress vq #%d\n", __func__,
					vport->vport, vq);
				goto err;
			}
		}
	}

	return 0;
err:
	pr_err("%s: function failed\n", __func__);
	return -1;

}
/*---------------------------------------------------------------------------*/
int mv_pp3_emac_vport_hw_init(struct pp3_vport *vport)
{
	int vq, emac;
	int qm_port[4] = {4, 5, 6, 7};

	MV_PP3_NULL_PTR(vport, err);

	emac = vport->port.emac.emac_num;

	/* configure port PHY address */
	mv_gop_phy_addr_cfg(emac, vport->port.emac.phy_addr);
	/* enebale MAC PTP unit */
	mv_pp3_ptp_enable(emac, true);
	mv_pp3_ptp_tai_tod_uio_init(pp3_device->pdev);
	mv_pp3_gop_port_init(emac, vport->port.emac.port_mode);
	/* disable link event on current port */
	mv_pp3_gop_port_events_mask(emac);
	mv_pp3_gop_port_enable(emac);

	if (vport->port.emac.force_link)
		mv_pp3_gop_fl_cfg(emac);

	qm_tail_ptr_mode(emac, true);

	for (vq = 0; vq < vport->rx_vqs_num; vq++) {
		if (vport->rx_vqs[vq]) {

			/* do not call to mv_pp3_vq_hw_init, function not relevant for EMAC vport*/

			/*if (mv_tm_scheme_queue_path_get(vport->rx_vqs[vq]->hwq, NULL, NULL, NULL, &qm_port))
				goto err;*/

			mv_pp3_emac_init(emac, qm_port[emac], vport->rx_vqs[vq]->hwq);

			/* config emac->pcc queue for QM secret machine */
			qm_xoff_emac_qnum_set(emac, vport->rx_vqs[vq]->hwq);

			/* set emac threshold profile and attached emac->ppc queue to profile */
			if (qm_emac_profile_set(emac, vport->port.emac.port_mode, vport->rx_vqs[vq]->hwq) < 0)
				goto err;
		}
	}
	/* Nothing to config in egress */

	vport->port.emac.flags |= BIT(MV_PP3_EMAC_F_INIT_BIT);
	return 0;
err:
	pr_err("%s: function failed\n", __func__);
	return -1;

}
/*---------------------------------------------------------------------------*/

void mv_pp3_vport_delete(struct pp3_vport *vport)
{
	int vq, vp;

	if (!vport)
		return;

	for (vq = 0; vq < vport->rx_vqs_num; vq++)
		if (vport->rx_vqs[vq])
			mv_pp3_vq_delete(vport->rx_vqs[vq]);

	for (vq = 0; vq < vport->tx_vqs_num; vq++)
		if (vport->tx_vqs[vq])
			mv_pp3_vq_delete(vport->tx_vqs[vq]);

	if (vport->type == MV_PP3_NSS_PORT_CPU)
		vp = MV_PP3_INTERNAL_CPU_PORT_BASE + vport->vport;
	else
		vp = vport->vport;

	pp3_vports[vp] = NULL;

	kfree(vport);

	pr_info("%s: delete vport %d\n", __func__, vport->vport);
}
/*---------------------------------------------------------------------------*/

/* Supported for EMAC virtual ports only */
int mv_pp3_egress_vport_shaper_set(struct pp3_vport *emac_vp, struct mv_nss_meter *meter)
{
	int rc, pnode, cbs_good, ebs_good;

	if (!emac_vp || (emac_vp->type != MV_PP3_NSS_PORT_ETH)) {
		pr_err("Supported only for EMAC vitual ports\n");
		return -1;
	}
	pnode = TM_A0_PORT_EMAC0 + emac_vp->port.emac.emac_num;
	cbs_good = meter->cbs;
	ebs_good = meter->ebs;
	rc = mv_tm_set_shaping_ex(TM_P_LEVEL, pnode, meter->cir, meter->eir, &cbs_good, &ebs_good);
	if (rc) {
		if (meter->cbs != cbs_good) {
			/* cbs value is too small. Use minimal valid value instead */
			pr_warn("%s: cbs = %d KBytes is too small. Use cbs = %d KBytes\n",
				__func__, meter->cbs, cbs_good);
		}

		if (meter->ebs != ebs_good) {
			/* ebs value is too small. Use minimal valid value instead */
			pr_warn("%s: ebs = %d KBytes is too small. Use ebs = %d KBytes\n",
				__func__, meter->ebs, ebs_good);
		}

		rc = mv_tm_set_shaping_ex(TM_P_LEVEL, pnode, meter->cir, meter->eir, &cbs_good, &ebs_good);
		if (rc)
			return rc;
	}
	return 0;
}
/*---------------------------------------------------------------------------*/

int mv_pp3_cpu_vport_rx_pkt_coal_set(struct pp3_vport *cpu_vp, int pkts_num)
{
	int vq;

	for (vq = 0; vq < cpu_vp->rx_vqs_num; vq++) {
		if (cpu_vp->rx_vqs[vq] && cpu_vp->rx_vqs[vq]->swq)
			mv_pp3_swq_rx_pkt_coal_set(cpu_vp->rx_vqs[vq]->swq, pkts_num);
	}
	return 0;
}
/*---------------------------------------------------------------------------*/

int mv_pp3_cpu_vport_rx_time_prof_set(struct pp3_vport *cpu_vp, int prof)
{
	int vq;

	for (vq = 0; vq < cpu_vp->rx_vqs_num; vq++) {
		if (cpu_vp->rx_vqs[vq] && cpu_vp->rx_vqs[vq]->swq)
			mv_pp3_swq_rx_time_prof_set(cpu_vp->rx_vqs[vq]->swq, prof);
	}
	return 0;
}
/*---------------------------------------------------------------------------*/

int mv_pp3_cpu_vport_rx_time_coal_set(struct pp3_vport *cpu_vp, int usec)
{
	int vq;

	/* all queues connected to the same timer profile */
	for (vq = 0; vq < cpu_vp->rx_vqs_num; vq++)
		if (cpu_vp->rx_vqs[vq] && cpu_vp->rx_vqs[vq]->swq)
			return mv_pp3_swq_rx_time_coal_set(cpu_vp->rx_vqs[vq]->swq, usec);

	return -1;
}
/*---------------------------------------------------------------------------*/

/*---------------------------------------------------------------------------
description:
	Update FW with rx vq map, relevant only for ppc->hmac mapping

return values:
		success: 0
		fail: -1
---------------------------------------------------------------------------*/
static int mv_pp3_vport_fw_set_rx_vq_map(struct pp3_vport *vport)
{
	int q, swq;

	if (!vport) {
		pr_err("%s: null vport ptr\n", __func__);
		return -1;
	}

	if (vport->type != MV_PP3_NSS_PORT_CPU) {
		pr_err("%s: function relevant only for ppc->hmac mapping\n", __func__);
		return -1;
	}

	for (q = 0; q < vport->rx_vqs_num; q++) {
		if (!vport->rx_vqs[q] || !vport->rx_vqs[q]->swq)
			continue;

		swq = MV_PP3_HMAC_PHYS_SWQ_NUM(vport->rx_vqs[q]->swq->swq, vport->rx_vqs[q]->swq->frame_num);

		pp3_fw_vq_map_set(vport->vport, q, MV_PP3_PPC_TO_HMAC, swq, vport->rx_vqs[q]->hwq);
	}

	return 0;
}

/*---------------------------------------------------------------------------
description:
	Update FW with tx vq map

return values:
		success: 0
		fail: -1
---------------------------------------------------------------------------*/
static int mv_pp3_vport_fw_set_tx_vq_map(struct pp3_vport *vport)
{
	int q;

	if (!vport) {
		pr_err("%s: null vport ptr\n", __func__);
		return -1;
	}

	if (vport->type != MV_PP3_NSS_PORT_ETH) {
		pr_err("%s: function relevant only for ppc->emac mapping\n", __func__);
		return -1;
	}

	for (q = 0; q < vport->tx_vqs_num; q++) {

		if (!vport->tx_vqs[q])
			continue;

		/* FW ignore swq (0) in ppc->emac mapping */
		pp3_fw_vq_map_set(vport->vport, q, MV_PP3_PPC_TO_EMAC, 0, vport->tx_vqs[q]->hwq);
	}

	return 0;
}

/*---------------------------------------------------------------------------
description:
	Update FW with cpu vport

return values:
		success: 0
		fail: -1
---------------------------------------------------------------------------*/
int pp3_cpu_vport_fw_set(struct pp3_vport *vport)
{
	if (!vport) {
		pr_err("%s:Error, null vport ptr\n", __func__);
		return -1;
	}

	pp3_fw_internal_cpu_vport_set(vport);
	/*cpu rxq*/
	mv_pp3_vport_fw_set_rx_vq_map(vport);

	return 0;
}
/*---------------------------------------------------------------------------
description:
	Update FW with  emac vport

return values:
		success: 0
		fail: -1
---------------------------------------------------------------------------*/
int pp3_emac_vport_fw_set(struct pp3_vport *vport, unsigned char *mac)
{

	if (!vport) {
		pr_info("%s: error null vp ptr\n", __func__);
		return -1;
	}

	pp3_fw_emac_vport_set(vport, mac);

	mv_pp3_vport_fw_set_tx_vq_map(vport);

	return 0;
}
/*---------------------------------------------------------------------------*/

/*---------------------------------------------------------------------------
mv_pp3_cpu_vport_cnt
	print cpu vport counter values according to number of cpu_vp_num
	print only if counter is not zeroed
	calculate and print sum value for cpu_vp_num > 1
	inputs:
		cpu_vp - array of CPU VPORTs pointers
		cpu_vp_num - size of array
		cnt_index - stat counter index
		name - counter name
---------------------------------------------------------------------------*/
static void mv_pp3_cpu_vport_cnt(struct pp3_vport **cpu_vp, int cpu_vp_num, int cnt_index, const char *name)
{
	int cpu, str_len = 0, sum = 0;
	unsigned int *stats;
	char str[100];
	bool print_flag = false;

	str_len += sprintf(str + str_len, "%-24s", name);

	for (cpu = 0; cpu < cpu_vp_num; cpu++) {
		if (cpu_vp[cpu]) {
			stats = (unsigned int *)&cpu_vp[cpu]->port.cpu.stats;
			sum += stats[cnt_index];
			str_len += sprintf(str + str_len, "%-15u", stats[cnt_index]);
			if (stats[cnt_index])
				print_flag |= true;
		} else
			str_len += sprintf(str + str_len, "%-15s", "NA");
	}

	if (cpu_vp_num > 1)
		str_len += sprintf(str + str_len, "%u\n", sum);
	else
		str_len += sprintf(str + str_len, "\n");

	if (print_flag)
		pr_cont("%s", str);
}
/*---------------------------------------------------------------------------*/

/* Print Host software collected statistics of network interface */
/*---------------------------------------------------------------------------
mv_pp3_cpu_vport_cnt_dump
	print cpu vport counter values according to number of cpu_vp_num
	print only if counter is not zeroed
	calculate and print sum value for cpu_vp_num > 1
	inputs:
		cpu_vp - array of CPU VPORTs pointers
		cpu_vp_num - size of array
		cnt_index - stat counter index
		name - counter name
---------------------------------------------------------------------------*/
void mv_pp3_cpu_vport_cnt_dump(struct pp3_vport **cpu_vp, int cpu_num)
{
	char line1[100];
	char line2[100];
	int cpu, num;
	int str_len1, str_len2;

	str_len1 = 0;
	str_len2 = 0;

	for (cpu = 0, num = 0; cpu < cpu_num; cpu++)
		if (cpu_vp[cpu]) {
			num++;
			str_len1 = sprintf(line1 + str_len1, "vport #%d %-6s", cpu_vp[cpu]->vport, "");
			str_len2 = sprintf(line2 + str_len2, " [cpu=%d] %-6s", cpu_vp[cpu]->port.cpu.cpu_num, "");
		}

	pr_cont("%s", line1);
	if (num > 1)
		pr_cont(" SUM");
	pr_cont("\n%22s %s\n", "", line2);
	pr_info("------------------------------------------------------------------------\n");

	PP3_DBG_GROUP_CNT_DUMP(cpu_vp, num, irq);
	PP3_DBG_GROUP_CNT_DUMP(cpu_vp, num, irq_err);
	PP3_DBG_GROUP_CNT_DUMP(cpu_vp, num, napi_sched);
	PP3_DBG_GROUP_CNT_DUMP(cpu_vp, num, napi_enter);
	PP3_DBG_GROUP_CNT_DUMP(cpu_vp, num, napi_complete);
	PP3_DBG_GROUP_CNT_DUMP(cpu_vp, num, rx_pkt_calc);
	PP3_DBG_GROUP_CNT_DUMP(cpu_vp, num, rx_bytes);
	PP3_DBG_GROUP_CNT_DUMP(cpu_vp, num, rx_err);
	PP3_DBG_GROUP_CNT_DUMP(cpu_vp, num, rx_csum_l3_err);
	PP3_DBG_GROUP_CNT_DUMP(cpu_vp, num, rx_csum_l4_err);
	PP3_DBG_GROUP_CNT_DUMP(cpu_vp, num, rx_drop_calc);
	PP3_DBG_GROUP_CNT_DUMP(cpu_vp, num, rx_netif);
	PP3_DBG_GROUP_CNT_DUMP(cpu_vp, num, rx_netif_drop);
	PP3_DBG_GROUP_CNT_DUMP(cpu_vp, num, rx_buf_pkt);
	PP3_DBG_GROUP_CNT_DUMP(cpu_vp, num, rx_split_pkt);
	PP3_DBG_GROUP_CNT_DUMP(cpu_vp, num, rx_cfh_pkt);
	PP3_DBG_GROUP_CNT_DUMP(cpu_vp, num, rx_cfh_dummy_calc);
	PP3_DBG_GROUP_CNT_DUMP(cpu_vp, num, rx_cfh_invalid_calc);
	PP3_DBG_GROUP_CNT_DUMP(cpu_vp, num, rx_cfh_reassembly_calc);
	PP3_DBG_GROUP_CNT_DUMP(cpu_vp, num, rx_csum_sw);
	PP3_DBG_GROUP_CNT_DUMP(cpu_vp, num, rx_csum_hw);
	PP3_DBG_GROUP_CNT_DUMP(cpu_vp, num, tx_pkt_calc);
	PP3_DBG_GROUP_CNT_DUMP(cpu_vp, num, tx_bytes);
	PP3_DBG_GROUP_CNT_DUMP(cpu_vp, num, tx_cfh_pkt);
	PP3_DBG_GROUP_CNT_DUMP(cpu_vp, num, tx_drop_calc);
	PP3_DBG_GROUP_CNT_DUMP(cpu_vp, num, tx_no_resource_calc);
	PP3_DBG_GROUP_CNT_DUMP(cpu_vp, num, tx_csum_sw);
	PP3_DBG_GROUP_CNT_DUMP(cpu_vp, num, tx_csum_hw);
	PP3_DBG_GROUP_CNT_DUMP(cpu_vp, num, tx_sg_bytes);
	PP3_DBG_GROUP_CNT_DUMP(cpu_vp, num, tx_sg_pkts);
	PP3_DBG_GROUP_CNT_DUMP(cpu_vp, num, tx_sg_err);
	PP3_DBG_GROUP_CNT_DUMP(cpu_vp, num, tx_sg_frags);
#ifdef CONFIG_MV_PP3_TSO
	PP3_DBG_GROUP_CNT_DUMP(cpu_vp, num, tx_tso_skb);
	PP3_DBG_GROUP_CNT_DUMP(cpu_vp, num, tx_tso_pkts);
	PP3_DBG_GROUP_CNT_DUMP(cpu_vp, num, tx_tso_bytes);
	PP3_DBG_GROUP_CNT_DUMP(cpu_vp, num, tx_tso_frags);
	PP3_DBG_GROUP_CNT_DUMP(cpu_vp, num, tx_tso_err);
#endif
	PP3_DBG_GROUP_CNT_DUMP(cpu_vp, num, txdone);
	pr_info("\n");
}
/*---------------------------------------------------------------------------*/
/* Print out CPU internal virtual port statistics */
int mv_pp3_cpu_vport_stats_dump(int vport)
{
	char name[100];
	struct pp3_vport *vp;

	if (vport >= MV_NSS_PORT_NONE)
		return -1;

	sprintf(name, "\nvport stats:");
	pr_info("\n%-24s", name);

	vp = pp3_vports[vport + MV_PP3_INTERNAL_CPU_PORT_BASE];
	if (vp) {
		if (vp->type == MV_PP3_NSS_PORT_CPU)
			mv_pp3_cpu_vport_cnt_dump(&vp, 1);
	} else
		pr_info("Virtual port %d not active\n", vport);

	return 0;
}
/*---------------------------------------------------------------------------*/

int mv_pp3_cpu_vport_stats_clear(int vport)
{
	struct pp3_vport *cpu_vp;

	cpu_vp = pp3_vports[vport + MV_PP3_INTERNAL_CPU_PORT_BASE];
	if (cpu_vp)
		memset(&cpu_vp->port.cpu.stats, 0, sizeof(struct pp3_cpu_vp_stats));

	return 0;
}
/*---------------------------------------------------------------------------*/

/* Print out virtual ports are used by driver */
void mv_pp3_vports_dump(void)
{
	int vport, cpu_vp;
	struct pp3_vport *vp;

	pr_info("\nVPort %8s %8s %9s %11s %11s\n", "Type", "EMAC", "DstVp", "TxVqNum", "RxVqNum");
	pr_info("------------------------------------------------------------\n");
	for (vport = MV_NSS_ETH_PORT_MIN; vport <= MV_NSS_EXT_PORT_MAX; vport++) {
		vp = pp3_vports[vport];
		if (!vp)
			continue;

		pr_info("%5d %8s %8d %8d %11d %11d\n", vp->vport,
			(vp->type == MV_PP3_NSS_PORT_ETH) ? "ETH" : "EXT",
			(vp->type == MV_PP3_NSS_PORT_ETH) ? vp->port.emac.emac_num : -1,
			vp->dest_vp, vp->tx_vqs_num, vp->rx_vqs_num);
	}
	pr_info("\n");

	pr_info("\nVPort %8s %8s %9s %11s %11s\n", "Type", "CPU", "DstVp", "TxVqNum", "RxVqNum");
	pr_info("------------------------------------------------------------\n");
	for (cpu_vp = MV_PP3_INTERNAL_CPU_PORT_MIN; cpu_vp <= MV_PP3_INTERNAL_CPU_PORT_MAX; cpu_vp++) {
		vport = cpu_vp + MV_PP3_INTERNAL_CPU_PORT_BASE;

		vp = pp3_vports[vport];
		if (!vp)
			continue;

		pr_info("%5d %8s %8d %8d %11d %11d\n", vp->vport,
			(vp->type == MV_PP3_NSS_PORT_CPU) ? "CPU" : "WRONG",
			(vp->type == MV_PP3_NSS_PORT_CPU) ? vp->port.cpu.cpu_num : -1,
			vp->dest_vp, vp->tx_vqs_num, vp->rx_vqs_num);
	}
	pr_info("\n");
}
/*---------------------------------------------------------------------------*/


/* Print statistics for all sw queues belong the CPU internal virtual port */
void mv_pp3_cpu_vport_vqs_stats_dump(int vport)
{
	struct pp3_vport *vp;
	int q_num;

	if (!pp3_vports[vport + MV_PP3_INTERNAL_CPU_PORT_BASE])
		return;

	vp = pp3_vports[vport + MV_PP3_INTERNAL_CPU_PORT_BASE];
	q_num = MV_MAX(vp->tx_vqs_num, vp->rx_vqs_num);

	pr_info("\ncpu port #%d: queues stats\n", vp->vport);
	mv_pp3_vqueue_cnt_dump_header(q_num);
	/* print txqs stats */
	mv_pp3_vqueue_cnt_dump("tx_", vp->port.cpu.cpu_num, vp->tx_vqs, vp->tx_vqs_num, true);
	/* print rxqs stats */
	mv_pp3_vqueue_cnt_dump("rx_", vp->port.cpu.cpu_num, vp->rx_vqs, vp->rx_vqs_num, true);
	pr_info("\n");
}
/*---------------------------------------------------------------------------*/

void mv_pp3_vport_fw_stat_print(int vport, struct mv_pp3_fw_vport_stat *stat)
{
	pr_info("\nvport #%2d - FW stats\n", vport);
	pr_info("------------------------------------------------------------------------\n");
	pr_info("Number of bytes received high             %10u\n", stat->rx_bytes_high);
	pr_info("Number of bytes received low              %10u\n", stat->rx_bytes_low);
	pr_info("Number of bytes transmitted high          %10u\n", stat->tx_bytes_high);
	pr_info("Number of bytes transmitted low           %10u\n", stat->tx_bytes_low);
	pr_info("Number of packets received high           %10u\n", stat->rx_packets_high);
	pr_info("Number of packets received low            %10u\n", stat->rx_packets_low);
	pr_info("Number of packets transmitted high        %10u\n", stat->tx_packets_high);
	pr_info("Number of packets transmitted low         %10u\n", stat->tx_packets_low);
	pr_info("Number of errors received high            %10u\n", stat->rx_errors_high);
	pr_info("Number of errors received low             %10u\n", stat->rx_errors_low);
	pr_info("Number of errors transmitted high         %10u\n", stat->tx_errors_high);
	pr_info("Number of errors transmitted low          %10u\n", stat->tx_errors_low);
}
/*---------------------------------------------------------------------------*/

/* Print FW vport statistics collected per EMAC or external virtual port */
void mv_pp3_vport_fw_stats_dump(int vport)
{
	struct mv_pp3_fw_vport_stat vport_stats;

	if (!pp3_vports[vport])
		return;

	if (pp3_fw_vport_stat_get(vport, &vport_stats) == 0)
		mv_pp3_vport_fw_stat_print(vport, &vport_stats);

	return;
}
/*---------------------------------------------------------------------------*/

int mv_pp3_vport_fw_stats_clear(int vport)
{
	int err;

	if (!pp3_vports[vport])
		return -1;

	err = pp3_fw_clear_stat_set(MV_PP3_FW_VPORT_STAT, vport);

	return err;
}

