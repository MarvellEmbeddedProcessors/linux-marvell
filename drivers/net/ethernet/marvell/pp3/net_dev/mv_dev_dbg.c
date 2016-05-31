/*******************************************************************************t
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
#include <linux/skbuff.h>
#include "platform/mv_pp3.h"
#include "platform/mv_pp3_config.h"
#include "bm/mv_bm.h"
#include "hmac/mv_hmac.h"
#include "hmac/mv_hmac_bm.h"
#include "emac/mv_emac.h"
#include "fw/mv_pp3_fw_msg.h"
#include "mv_netdev_structs.h"
#include "mv_netdev.h"
#include "mv_dev_vq.h"
#include "vport/mv_pp3_vq.h"
#include "vport/mv_pp3_vport.h"
#include "vport/mv_pp3_cpu.h"
#include "vport/mv_pp3_pool.h"
#include "msg/mv_pp3_msg_chan.h"
#include "gop/mv_gop_if.h"
#include "gop/mv_mib_regs.h"

#ifdef CONFIG_MV_PP3_TM_SUPPORT
#include "tm/mv_tm.h"
#include "tm/wrappers/mv_tm_scheme.h"
#endif

#define PP3_DBG_POOLS_STATS_DUMP(pools, num, cpus, name)\
	pp3_dbg_pools_stats_dump(pools, num, cpus, offsetof(struct pp3_pool_stats, name)/4, #name)


void pp3_dbg_separate_line_print(int length)
{
	int i;

	for (i = 0; i < length; i++)
		pr_cont("-");

	pr_cont("\n");
}
/*---------------------------------------------------------------------------*/

void pp3_dbg_ingress_vqs_show(struct net_device *dev)
{
	int vq, vqs_num;
	u16 pkts;
	struct	pp3_dev_priv *dev_priv;
	struct mv_nss_drop drop;
	struct mv_nss_sched sched;

	dev_priv = mv_pp3_dev_priv_ready_get(dev);
	if (!dev_priv)
		return;

	if (mv_pp3_dev_ingress_vqs_num_get(dev, &vqs_num)) {
		pr_err("%s: Error - Can't get number of ingress virtual queues\n", dev->name);
		return;
	}

	pr_info("\n");
	pr_info("vq: %8s %8s %8s %8s  %8s %8s\n", "length", "prio", "weight", "DWRR", "TD", "RED");
	pr_info("    %8s %8s %8s %8s  %8s %8s\n", "[pkts]", "[0-7]", "", "[E/D]", "[KB]", "[KB]");

	for (vq = 0; vq < vqs_num; vq++) {

		if (mv_pp3_dev_ingress_vq_size_get(dev, vq, &pkts))
			pkts = -1;

		if (mv_pp3_dev_ingress_vq_drop_get(dev, vq, &drop))
			memset(&drop, 0, sizeof(drop));

		if (mv_pp3_dev_ingress_vq_sched_get(dev, vq, &sched))
			memset(&sched, 0, sizeof(sched));

		pr_info("%-2d: %8d %8d %8d %8s %8d %8d\n", vq, pkts,
			sched.priority,	sched.weight, (sched.wrr_enable) ? "En" : "Dis",
			drop.td, drop.red);
	}
}
/*---------------------------------------------------------------------------*/

void pp3_dbg_ingress_vqs_print(struct net_device *dev)
{
	int vq, vqs_num, cpu, anode, bnode;
	struct pp3_vport *vp;
	struct	pp3_dev_priv *dev_priv;
	struct pp3_vq *vq_priv;

	dev_priv = mv_pp3_dev_priv_ready_get(dev);
	if (!dev_priv)
		return;

	pr_cont("\n");

	pr_cont("        ");
	for_each_possible_cpu(cpu) {
		if (cpumask_test_cpu(cpu, &dev_priv->rx_cpus))
			pr_cont("            CPU[%d]                  ", cpu);
	}
	pr_cont("\n");

	pr_cont("vq:     ");

	for_each_possible_cpu(cpu) {
		if (cpumask_test_cpu(cpu, &dev_priv->rx_cpus))
			pr_cont("frame  swq   hwq   anode   bnode    ");
	}

	pr_cont("\n");

	if (mv_pp3_dev_ingress_vqs_num_get(dev, &vqs_num)) {
		pr_err("%s: Error - Can't get number of ingress virtual queues\n", dev->name);
		return;
	}
	for (vq = 0; vq < vqs_num; vq++) {

		pr_cont("%-2d:       ", vq);
		for_each_possible_cpu(cpu) {

			vp = dev_priv->cpu_vp[cpu];
			if (!vp)
				continue;

			vq_priv = vp->rx_vqs[vq];
			if (!vq_priv || !vq_priv->swq)
				continue;

			/* Get parent Anode for HWQ */
			if (mv_tm_scheme_parent_node_get(TM_Q_LEVEL, vq_priv->hwq, &anode)) {
				pr_err("%s: can't get bnode for anode %d\n", __func__, anode);
				anode = -1;
			} else if (mv_tm_scheme_parent_node_get(TM_A_LEVEL, anode, &bnode)) {
				pr_err("%s: can't get bnode for anode %d\n", __func__, anode);
				bnode = -1;
			}
			pr_cont("%d    %2d    %3d    %3d    %3d        ",
				vq_priv->swq->frame_num, vq_priv->swq->swq, vq_priv->hwq,
				anode, bnode);
		}
		pr_cont("\n");
	}
	pr_info("\n");
}
/*---------------------------------------------------------------------------*/


void pp3_dbg_egress_vqs_show(struct net_device *dev)
{
	int vq, vqs_num;
	u16 pkts;
	struct pp3_dev_priv *dev_priv;
	struct mv_nss_meter shaper;
	struct mv_nss_sched sched;
	struct mv_nss_drop drop;

	dev_priv = mv_pp3_dev_priv_ready_get(dev);
	if (!dev_priv)
		return;

	if (mv_pp3_dev_egress_vqs_num_get(dev, &vqs_num)) {
		pr_err("%s: Error - Can't get number of egress virtual queues\n", dev->name);
		return;
	}

	pr_info("\n");
	pr_info("vq:   %6s %8s  %8s %8s %8s %8s %8s %8s %8s %8s\n",
		"length", "prio", "weight", "DWRR", "TD", "RED", "CIR", "EIR", "CBS", "EBS");
	pr_info("      %6s %8s  %8s %8s %8s %8s %8s %8s %8s %8s\n",
		"[pkts]", "[0-7]", "", "[E/D]", "[KB]", "[KB]", "[Mbps]", "[Mbps]", "[KB]", "[KB]");

	for (vq = 0; vq < vqs_num; vq++) {

		if (mv_pp3_dev_egress_vq_size_get(dev, vq, &pkts))
			pkts = -1;

		if (mv_pp3_dev_egress_vq_rate_limit_get(dev, vq, &shaper))
			memset(&shaper, 0, sizeof(shaper));

		if (mv_pp3_dev_egress_vq_sched_get(dev, vq, &sched))
			memset(&sched, 0, sizeof(sched));

		if (mv_pp3_dev_egress_vq_drop_get(dev, vq, &drop))
			memset(&drop, 0, sizeof(drop));

		pr_info("%-2d: %8d %8d %8d  %8s %8d %8d %8d %8d %8d %8d\n",
			vq, pkts, sched.priority,
			sched.weight, (sched.wrr_enable) ? "En " : "Dis",
			drop.td, drop.red,
			shaper.cir, shaper.eir, shaper.cbs, shaper.ebs);
	}
}
/*---------------------------------------------------------------------------*/

void pp3_dbg_egress_vqs_print(struct net_device *dev)
{
	int vq, vqs_num, cpu, anode, bnode;
	struct pp3_dev_priv *dev_priv;
	struct pp3_vport *vp, *emac_vp = NULL;
	struct pp3_vq *vq_priv;

	dev_priv = mv_pp3_dev_priv_ready_get(dev);
	if (!dev_priv)
		return;

	if (mv_pp3_dev_egress_vqs_num_get(dev, &vqs_num)) {
		pr_err("%s: Error - Can't get number of ingress virtual queues\n", dev->name);
		return;
	}
	vp = dev_priv->vport;
	if (vp && (vp->type == MV_PP3_NSS_PORT_ETH))
		emac_vp = vp;

	pr_cont("\n");

	pr_cont("        ");
	for_each_possible_cpu(cpu)
		pr_cont("            CPU[%d]                   ", cpu);

	if (emac_vp)
		pr_cont("EMAC[%d]  ", emac_vp->vport);

	pr_cont("\n");

	pr_cont("vq:     ");

	for_each_possible_cpu(cpu)
		pr_cont("frame  swq   hwq   anode   bnode     ");

	if (emac_vp)
		pr_cont("  hwq    ");

	pr_cont("\n");

	for (vq = 0; vq < vqs_num; vq++) {

		pr_cont("%-2d:       ", vq);
		for_each_possible_cpu(cpu) {

			vp = dev_priv->cpu_vp[cpu];
			if (!vp)
				continue;

			vq_priv = vp->tx_vqs[vq];
			if (!vq_priv || !vq_priv->swq)
				break;

			/* Get parent Anode for HWQ */
			if (mv_tm_scheme_parent_node_get(TM_Q_LEVEL, vq_priv->hwq, &anode)) {
				pr_err("%s: can't get bnode for anode %d\n", __func__, anode);
				anode = -1;
			} else if (mv_tm_scheme_parent_node_get(TM_A_LEVEL, anode, &bnode)) {
				pr_err("%s: can't get bnode for anode %d\n", __func__, anode);
				bnode = -1;
			}
			pr_cont("%d    %2d    %3d    %3d    %3d         ",
				vq_priv->swq->frame_num, vq_priv->swq->swq, vq_priv->hwq,
				anode, bnode);
		}
		if (emac_vp) {
			vq_priv = emac_vp->tx_vqs[vq];
			if (vq_priv)
				pr_cont("%3d      ", vq_priv->hwq);
		}
		pr_cont("\n");
	}
	pr_info("\n");
}
/*---------------------------------------------------------------------------*/

/*TODO: currently print only HWQ - improve this function */
static void pp3_dbg_dev_emacs_resources_line_dump(struct pp3_vport *emac_vp)
{
	int i;

	pr_info("\n");
	pr_info("emac%d:  TX%-35s", emac_vp->vport, "");

	/* emac not initialized */
	if (!(emac_vp->port.emac.flags & BIT(MV_PP3_EMAC_F_INIT_BIT)))
		return;

	for (i = 0; i < emac_vp->tx_vqs_num; i++) {
		if (!(emac_vp->tx_vqs[i]))
			continue;

		pr_cont("%-3d ", emac_vp->tx_vqs[i]->hwq);
	}
	pr_info("        RX%-35s", "");

	for (i = 0; i < emac_vp->rx_vqs_num; i++) {
		if (!(emac_vp->rx_vqs[i]))
			continue;

		pr_cont("%-3d ", emac_vp->rx_vqs[i]->hwq);
	}
}

/*---------------------------------------------------------------------------
pp3_dbg_dev_emacs_resources_dump
	print display EMACs HW resources that PP3 driver uses for all purposes
	inputs:
---------------------------------------------------------------------------*/
void pp3_dbg_dev_emacs_resources_dump(void)
{
	int i;

	if (!pp3_vports) {
		pr_info("System is not initialzied\n");
		return;
	}

	for (i = 0; i < mv_pp3_ports_num_get(pp3_device); i++)
		if ((pp3_vports[i]) && (pp3_vports[i]->type == MV_PP3_NSS_PORT_ETH))
			pp3_dbg_dev_emacs_resources_line_dump(pp3_vports[i]);

	pr_cont("\n");
}
/*---------------------------------------------------------------------------*/

/*---------------------------------------------------------------------------
pp3_dbg_dev_channel_resources_dump
	print out Sw and HW resources used by channels
	inputs:
---------------------------------------------------------------------------*/
static void pp3_dbg_dev_channel_resources_dump(void)
{
	struct mv_pp3_channel *chan_ctrl;
	int i, j, chan_num;
	int node, cpu;
	char cpu_str[20], tmp[3];

	chan_num = mv_pp3_chan_num_get();

	for (i = 0; i < chan_num; i++) {
		chan_ctrl = mv_pp3_chan_get(i);

		mv_pp3_cfg_hmac_tx_anode_get(chan_ctrl->hmac_hw_txq, &node);
		pr_info("\n");
		pr_info("chan%d:  TX   ", i);
		pr_cont("%-4s %-5s %-6d %-6s %-6d %-32d %-28d %2d (ppc0)   %-4s", "NA", "NA", chan_ctrl->frame, "NA",
				chan_ctrl->hmac_sw_txq, chan_ctrl->hmac_hw_txq, node,
				mv_pp3_cfg_hmac_pnode_get(PP3_PPC0_DP), "NA");
		pr_info("%-8s", "");

		j = 0;
		cpu_str[0] = '\0';
		for_each_possible_cpu(cpu) {
			if ((chan_ctrl->cpu_mask >> cpu) & 1) {
				sprintf(tmp, " %d", cpu);
				strcat(cpu_str, tmp);
				if (j++)
					strcat(cpu_str, ",");
			}
		}

		mv_pp3_cfg_hmac_rx_anode_get(chan_ctrl->hmac_hw_rxq, &node);
		pr_cont("RX   %-4s %-5d %-6d %-6d %-6d %-32d %-28d %2d (hmac)   %-4s\n", cpu_str, chan_ctrl->irq_num,
				chan_ctrl->frame, chan_ctrl->event_group, chan_ctrl->hmac_sw_rxq,
				chan_ctrl->hmac_hw_rxq, node, mv_pp3_cfg_hmac_pnode_get(PP3_HMAC_RX), "NA");
	}
}

/*---------------------------------------------------------------------------
pp3_dbg_dev_bm_resources_dump
	print out Sw and HW resources used by channels
	inputs:
---------------------------------------------------------------------------*/
static void pp3_dbg_dev_bm_resources_dump(void)
{
	struct pp3_cpu *cpu_ctrl;
	int cpu;
	bool first;

	pr_info("\n");

	first = true;
	for_each_possible_cpu(cpu) {
		if (first) {
			pr_info("bm_put: TX   ");
			first = false;
		} else {
			pr_info("%-13s", "");
		}

		cpu_ctrl = pp3_cpus[cpu];
		pr_cont("%-4d %-5s %-6d %-6s %-6d", cpu, "NA", cpu_ctrl->bm_frame, "NA", cpu_ctrl->bm_swq);
	}
	pr_info("\n");
	first = true;
	for_each_possible_cpu(cpu) {
		struct pp3_cpu *cpu_ctrl;

		cpu_ctrl = pp3_cpus[cpu];
		if (first) {
			pr_info("bm_get: RX   ");
			first = false;
		} else {
			pr_info("%-13s", "");
		}
		pr_cont("%-4d %-5s %-6d %-6s %-6d", cpu, "NA", cpu_ctrl->bm_frame, "NA", cpu_ctrl->bm_swq);
	}
	pr_cont("\n");
}
/*---------------------------------------------------------------------------*/

static void pp3_dbg_dev_rx_resources_line_dump(struct pp3_dev_priv *dev_priv)
{
	int cpu, q, node;
	bool first;
	struct pp3_vport *cpu_vp;
	char queues[64];
	char anodes[64];
	char tmp[16];

	pr_info("\n");
	first = true;
	for_each_possible_cpu(cpu) {
		memset(queues, 0, sizeof(queues));
		memset(anodes, 0, sizeof(anodes));

		cpu_vp = dev_priv->cpu_vp[cpu];
		if (!cpu_vp || !cpumask_test_cpu(cpu, &dev_priv->rx_cpus))
			continue;

		if (first) {
			pr_info("        RX   ");
			first = false;
		} else {
			pr_info("%-13s", "");
		}
		pr_cont("%-4d %-5d %-6d", cpu, cpu_vp->port.cpu.irq_num, cpu_vp->rx_vqs[0]->swq->frame_num);
		pr_cont(" %-6d", cpu_vp->rx_vqs[0]->swq->queue.rx.irq_group);

		pr_cont("%2d:%-2d ", cpu_vp->rx_vqs[0]->swq->swq, cpu_vp->rx_vqs[cpu_vp->rx_vqs_num-1]->swq->swq);

		pr_cont("%-2s", "");
		for (q = 0; (q < cpu_vp->rx_vqs_num) && (q < 8); q++) {
			sprintf(tmp, "%d ", cpu_vp->rx_vqs[q]->hwq);

			strcat(queues, tmp);
			mv_pp3_cfg_hmac_rx_anode_get(cpu_vp->rx_vqs[q]->hwq, &node);
			sprintf(tmp, "%d ", node);
			strcat(anodes, tmp);
		}
		pr_cont("%-32s %-28s ", queues, anodes);

		/* TODO print pools per interface and not per group */
		pr_cont("%2d (hmac)   %-2d (L) %-2d (S)", mv_pp3_cfg_hmac_pnode_get(PP3_HMAC_RX),
			dev_priv->cpu_shared->long_pool ? dev_priv->cpu_shared->long_pool->pool : -1,
			dev_priv->cpu_shared->short_pool ? dev_priv->cpu_shared->short_pool->pool : -1);

		if (cpu_vp->rx_vqs_num > 8) {
			memset(queues, 0, sizeof(queues));
			memset(anodes, 0, sizeof(anodes));

			pr_info("%-45s", "");
			for (q = 8; (q < cpu_vp->rx_vqs_num); q++) {
				sprintf(tmp, "%d ", cpu_vp->rx_vqs[q]->hwq);

				strcat(queues, tmp);
				mv_pp3_cfg_hmac_rx_anode_get(cpu_vp->rx_vqs[q]->hwq + q, &node);
				sprintf(tmp, "%d ", node);
				strcat(anodes, tmp);
			}
			pr_cont("%-32s %-28s ", queues, anodes);
		}
	}
}
/*---------------------------------------------------------------------------*/

static void pp3_dbg_dev_tx_resources_line_dump(struct pp3_dev_priv *dev_priv)
{
	int cpu, q, node;
	struct pp3_vport *cpu_vp;
	struct pp3_pool *ppool;
	bool first;
	char queues[64];
	char anodes[64];
	char tmp[16];

	pr_info("\n");
	first = true;
	for_each_possible_cpu(cpu) {
		memset(queues, 0, sizeof(queues));
		memset(anodes, 0, sizeof(anodes));

		cpu_vp = dev_priv->cpu_vp[cpu];
		if (!cpu_vp)
			continue;

		if (first) {
			pr_info("%-5s:  TX   ", dev_priv->dev->name);
			first = false;
		} else {
			pr_info("%-13s", "");
		}
		pr_cont("%-4d %-5s %-6d %-6s", cpu, "NA", cpu_vp->tx_vqs[0]->swq->frame_num, "NA");

		pr_cont("%2d:%-2d ", cpu_vp->tx_vqs[0]->swq->swq, cpu_vp->tx_vqs[cpu_vp->tx_vqs_num-1]->swq->swq);

		pr_cont("%-2s", "");

		for (q = 0; (q < cpu_vp->tx_vqs_num) && (q < 8); q++) {
			sprintf(tmp, "%d ", cpu_vp->tx_vqs[q]->hwq);
			strcat(queues, tmp);
			mv_pp3_cfg_hmac_tx_anode_get(cpu_vp->tx_vqs[q]->hwq, &node);
			sprintf(tmp, "%d ", node);
			strcat(anodes, tmp);
		}
		pr_cont("%-32s %-28s ", queues, anodes);

		ppool = dev_priv->cpu_shared->txdone_pool;

		pr_cont("%2d (ppc0)   %-2d (txdone)",
			mv_pp3_cfg_hmac_pnode_get(PP3_PPC0_DP),
			ppool ? ppool->pool : -1);

		if (cpu_vp->tx_vqs_num > 8) {
			memset(queues, 0, sizeof(queues));
			memset(anodes, 0, sizeof(anodes));

			pr_info("%-45s", "");
			for (q = 8; (q < cpu_vp->tx_vqs_num); q++) {
				sprintf(tmp, "%d ", cpu_vp->tx_vqs[q]->hwq);
				strcat(queues, tmp);
				mv_pp3_cfg_hmac_tx_anode_get(cpu_vp->tx_vqs[q]->hwq, &node);
				sprintf(tmp, "%d ", node);
				strcat(anodes, tmp);
			}
			pr_cont("%-32s %-28s ", queues, anodes);
		}
	}
}

/*---------------------------------------------------------------------------
pp3_dbg_dev_resources_dump
	print display HW resources that PP3 driver uses for all purposes
	inputs:
---------------------------------------------------------------------------*/
void pp3_dbg_dev_resources_dump(void)
{
	int i;
	struct pp3_dev_priv *dev_priv;

	if (!mv_pp3_shared_initialized(pp3_device)) {
		pr_info("System is not initialzied\n");
		return;
	}

	pr_info("\n");
	pr_info("------Linux----------");
	pr_cont("   ------HMAC--------");
	pr_cont("   -------------------QM--------------------------------------------------");
	pr_cont("   -----BM------");
	pr_info("\n");
	pr_info("Name    Dir  CPU  IRQ");
	pr_cont("   Frame  ISR   SWQs");
	pr_cont("    HWQs                             A node                        P node ");
	pr_cont("    BM Pool");
	for (i = 0; i < mv_pp3_dev_num_get(); i++) {
		dev_priv = mv_pp3_dev_priv_get(i);

		if (!dev_priv || !(dev_priv->flags & MV_PP3_F_INIT))
			continue;

		pp3_dbg_dev_tx_resources_line_dump(dev_priv);
		pp3_dbg_dev_rx_resources_line_dump(dev_priv);
	}
	pr_info("-----------------------------------------------------------------------------------------------");
	pr_cont("-------------------------------------\n");
	pp3_dbg_dev_channel_resources_dump();
	pr_info("-----------------------------------------------------------------------------------------------");
	pr_cont("-------------------------------------\n");
	pp3_dbg_dev_emacs_resources_dump();
	pr_info("-----------------------------------------------------------------------------------------------");
	pr_cont("-------------------------------------\n");
	pp3_dbg_dev_bm_resources_dump();
	pr_info("-----------------------------------------------------------------------------------------------");
	pr_cont("-------------------------------------\n");
}

/*---------------------------------------------------------------------------*/

/*---------------------------------------------------------------------------*/
void pp3_dbg_dev_mac_mc_print(struct net_device *dev)
{
	struct list_head *mc_addr;
	struct mac_mc_info *mc_addr_node;
	struct pp3_dev_priv *dev_priv = MV_PP3_PRIV(dev);
	int i = 0;

	pr_info("Port %d MAC Multicast Addresses:", dev_priv->vport->vport);
	list_for_each(mc_addr, &dev_priv->mac_list) {
		mc_addr_node = (struct mac_mc_info *)mc_addr;
		pr_info("#%d\t%02x:%02x:%02x:%02x:%02x:%02x", i++,
			mc_addr_node->mac[0], mc_addr_node->mac[1], mc_addr_node->mac[2],
			mc_addr_node->mac[3], mc_addr_node->mac[4], mc_addr_node->mac[5]);
	}
	pr_info("\n");
}

/*---------------------------------------------------------------------------*/
/* Calculate SUM of RXQs and TXQs counters in the group and update group statistics */
static void pp3_dbg_dev_nic_cnt_update_all(struct pp3_dev_priv *dev_priv)
{
	int q, cpu;
	struct pp3_vport *cpu_vp;
	struct pp3_swq *rx_swq;
	struct pp3_swq *tx_swq;

	/* Update calculated counters by sum of the counter for all queues */
	for_each_possible_cpu(cpu) {

		cpu_vp = dev_priv->cpu_vp[cpu];
		if (!cpu_vp)
			continue;

		/* Clear RX calculated statistics before update */
		cpu_vp->port.cpu.stats.rx_pkt_calc = 0;
		cpu_vp->port.cpu.stats.rx_drop_calc = 0;
		cpu_vp->port.cpu.stats.rx_cfh_dummy_calc = 0;
		cpu_vp->port.cpu.stats.rx_cfh_invalid_calc = 0;
		cpu_vp->port.cpu.stats.rx_cfh_reassembly_calc = 0;

		for (q = 0; q < MV_PP3_VQ_NUM; q++) {
			if (cpu_vp->rx_vqs[q]) {
				rx_swq = cpu_vp->rx_vqs[q]->swq;
				cpu_vp->port.cpu.stats.rx_pkt_calc += rx_swq->stats.pkts;
				cpu_vp->port.cpu.stats.rx_drop_calc += rx_swq->stats.pkts_drop;
				/*cpu_vp->port.cpu.stats.rx_cfh_dummy_calc += rx_swq->stats.rx_cfh_dummy;
				cpu_vp->port.cpu.stats.rx_cfh_invalid_calc += rx_swq->stats.rx_cfh_invalid;
				cpu_vp->port.cpu.stats.rx_cfh_reassembly_calc += rx_swq->stats.rx_cfh_reassembly;*/
			}
		}
		/* Clear TX calculated statistics before update */
		cpu_vp->port.cpu.stats.tx_pkt_calc = 0;
		cpu_vp->port.cpu.stats.tx_drop_calc = 0;
		cpu_vp->port.cpu.stats.tx_no_resource_calc = 0;
		for (q = 0; q < MV_PP3_VQ_NUM; q++) {
			if (cpu_vp->tx_vqs[q]) {
				tx_swq = cpu_vp->tx_vqs[q]->swq;
				cpu_vp->port.cpu.stats.tx_pkt_calc += tx_swq->stats.pkts;
				cpu_vp->port.cpu.stats.tx_drop_calc += tx_swq->stats.pkts_drop;
				cpu_vp->port.cpu.stats.tx_no_resource_calc += tx_swq->stats.pkts_errors;
			}
		}
	}
}
/*---------------------------------------------------------------------------*/

/* Clear Host software collected statistics of network interface */
static void pp3_dbg_dev_nic_stats_clear(struct pp3_dev_priv *dev_priv)
{
	int cpu;
	struct pp3_vport *cpu_vp;

	for_each_possible_cpu(cpu) {
		cpu_vp = dev_priv->cpu_vp[cpu];
		if (cpu_vp)
			memset(&cpu_vp->port.cpu.stats, 0, sizeof(struct pp3_cpu_vp_stats));
	}
}
/*---------------------------------------------------------------------------*/

/* Print Host software collected statistics of network interface */
static void pp3_dbg_dev_nic_stats_dump(struct pp3_dev_priv *dev_priv)
{
	char name[100];

	sprintf(name, "\n%s stats:", dev_priv->dev->name);
	pr_cont("\n%-24s", name);

	mv_pp3_cpu_vport_cnt_dump(dev_priv->cpu_vp, CONFIG_NR_CPUS);
}
/*---------------------------------------------------------------------------*/

/* Calculate SUM of single counter from TXQ statistics for all TXQs in virtual port */
static int pp3_dbg_sum_txqs_cnt_get(struct pp3_vport *vp, int cnt_index)
{
	int q, sum = 0;
	unsigned int *stats;

	for (q = 0; q < MV_PP3_VQ_NUM; q++)
		if (vp->tx_vqs[q]) {
			stats = (unsigned int *)&vp->tx_vqs[q]->swq->stats;
			sum += stats[cnt_index];
		}

	return sum;
}
/*---------------------------------------------------------------------------*/

/* Clear statistics for all TXQs belong the virtual port */
void pp3_dbg_vport_txqs_stats_clear(struct pp3_vport *vp)
{
	int q;

	for (q = 0; q < MV_PP3_VQ_NUM; q++)
		if ((vp->type == MV_PP3_NSS_PORT_CPU) && (vp->tx_vqs[q])) {
			memset(&vp->tx_vqs[q]->swq->stats, 0, sizeof(struct pp3_swq_stats));
			pp3_fw_clear_stat_set(MV_PP3_FW_SWQ_STAT, vp->tx_vqs[q]->swq->swq);
			pp3_fw_clear_stat_set(MV_PP3_FW_HWQ_STAT, vp->tx_vqs[q]->hwq);
		}
}

/* Clear statistics for all TXQs belong the network interface */
void pp3_dbg_txqs_stats_clear(struct pp3_dev_priv *dev_priv)
{
	int cpu;
	struct pp3_vport *cpu_vp;

	/* clear emac txqs stats */
	if (dev_priv->vport)
		pp3_dbg_vport_txqs_stats_clear(dev_priv->vport);

	/* clear cpu txqs stats */
	for_each_possible_cpu(cpu) {

		cpu_vp = dev_priv->cpu_vp[cpu];
		if (!cpu_vp)
			continue;

		pp3_dbg_vport_txqs_stats_clear(cpu_vp);
	}
}
/*---------------------------------------------------------------------------*/

/* Clear statistics for all TXQs belong the virtual port */
void pp3_dbg_vport_rxqs_stats_clear(struct pp3_vport *vp)
{
	int q;

	for (q = 0; q < MV_PP3_VQ_NUM; q++)
		if ((vp->type == MV_PP3_NSS_PORT_CPU) && (vp->rx_vqs[q])) {
			memset(&vp->rx_vqs[q]->swq->stats, 0, sizeof(struct pp3_swq_stats));
			pp3_fw_clear_stat_set(MV_PP3_FW_SWQ_STAT, vp->rx_vqs[q]->swq->swq);
			pp3_fw_clear_stat_set(MV_PP3_FW_HWQ_STAT, vp->rx_vqs[q]->hwq);
			vp->rx_vqs[q]->swq->stats_reset_flag = true;
		}
}
/*---------------------------------------------------------------------------*/

/* Clear statistics for all RXQs belong the network interface */
void pp3_dbg_rxqs_stats_clear(struct pp3_dev_priv *dev_priv)
{
	int cpu;
	struct pp3_vport *cpu_vp;

	/* clear emac rxqs stats */
	if (dev_priv->vport)
		pp3_dbg_vport_rxqs_stats_clear(dev_priv->vport);

	/* clear cpu txqs stats */
	for_each_possible_cpu(cpu) {

		cpu_vp = dev_priv->cpu_vp[cpu];
		if (!cpu_vp)
			continue;

		pp3_dbg_vport_rxqs_stats_clear(cpu_vp);
	}
}
/*---------------------------------------------------------------------------*/

/* Print statistics for all RX and TX VQs belong the network interface */
void pp3_dbg_dev_vqs_stats_dump(struct pp3_dev_priv *dev_priv)
{
	int cpu;
	struct pp3_vport *cpu_vp;

	pr_info("%s: queues stats\n", dev_priv->dev->name);
	mv_pp3_vqueue_cnt_dump_header(MV_MAX(dev_priv->rxqs_per_cpu, dev_priv->txqs_per_cpu));

	/* print cpu rxqs stats */
	for_each_possible_cpu(cpu) {

		cpu_vp = dev_priv->cpu_vp[cpu];
		if (!cpu_vp)
			continue;

		if (cpumask_test_cpu(cpu, &dev_priv->rx_cpus)) {
			mv_pp3_vqueue_cnt_dump("rx_", cpu_vp->port.cpu.cpu_num,
					cpu_vp->rx_vqs, cpu_vp->rx_vqs_num, true);
		}
	}
	/* print cpu txqs stats */
	for_each_possible_cpu(cpu) {

		cpu_vp = dev_priv->cpu_vp[cpu];
		if (!cpu_vp)
			continue;

		/* print txqs stats */
		mv_pp3_vqueue_cnt_dump("tx_", cpu_vp->port.cpu.cpu_num, cpu_vp->tx_vqs, cpu_vp->tx_vqs_num, true);
	}
	pr_info("\n");
}
/*---------------------------------------------------------------------------*/

/* Dump single counter for number of BM pools.
   if cpus_num = 0 - only sum of all CPUs numer will be printed
*/
static void pp3_dbg_pools_stats_dump(struct pp3_pool *pools[], int num, u32 cpus_num,
					int cnt_index, const char *name)
{
	u32 total;
	u32 *stats;
	struct pp3_pool *ppool;
	int pool, cpu;

	/* Each line contains 3 colomns per pool: CPU0, CPU1 and SUM */
	pr_cont("%-24s", name);
	for (pool = 0; pool < num; pool++) {
		total = 0;
		ppool = pools[pool];
		if (!ppool)
			continue;

		for_each_possible_cpu(cpu) {
			if (cpu >= cpus_num)
				break;

			if (cnt_index == -1) {
				pr_cont("%-15u", cpu);
				continue;
			}
			stats = (u32 *)PPOOL_STATS(ppool, cpu);
			pr_cont("%-15u", stats[cnt_index]);

			total += stats[cnt_index];
		}
		if (cnt_index == -1)
			pr_cont("%-15s", "SUM");
		else
			pr_cont("%-15u", total);
	}
	pr_cont("\n");
}
/*---------------------------------------------------------------------------*/

/* Clear statistics for all BM pools belong network interface */
static void pp3_dbg_dev_pool_stats_clear(struct pp3_dev_priv *dev_priv)
{
	if (!dev_priv->cpu_shared)
		return;

	/* Clear short pool statistics */
	if (dev_priv->cpu_shared->short_pool)
		pp3_dbg_pool_stats_clear(dev_priv->cpu_shared->short_pool->pool);

	/* Clear long pool statistics */
	if (dev_priv->cpu_shared->long_pool)
		pp3_dbg_pool_stats_clear(dev_priv->cpu_shared->long_pool->pool);

	/* Clear LRO pool statistics */
	if (dev_priv->cpu_shared->lro_pool)
		pp3_dbg_pool_stats_clear(dev_priv->cpu_shared->lro_pool->pool);

	/* Clear TX Done pools statistics for all CPUs */
	pp3_dbg_pool_stats_clear(dev_priv->cpu_shared->txdone_pool->pool);
}
/*---------------------------------------------------------------------------*/

static void pp3_dbg_dev_lnx_pools_stats_dump(struct pp3_dev_priv *dev_priv)
{
	char name[15];
	struct pp3_pool *pools[CONFIG_NR_CPUS];
	int cpu, num = 0;
	int cpu_num = 0;

	pools[num] = dev_priv->cpu_shared->txdone_pool;

	pr_cont("\n%-24s", "Linux pools stats:");
	for_each_possible_cpu(cpu) {
		sprintf(name, "cpu%d-[%d]", cpu, pools[num] ? pools[num]->pool : -1);
		pr_cont("%-15s", name);
		cpu_num++;
	}
	pr_cont("%-15s\n", "SUM");
	pp3_dbg_separate_line_print(24 + 15 * (cpu_num + 1));

	num++;
	PP3_DBG_POOLS_STATS_DUMP(pools, num, cpu_num, buff_get_timeout_err);
	PP3_DBG_POOLS_STATS_DUMP(pools, num, cpu_num, buff_get_request);
	PP3_DBG_POOLS_STATS_DUMP(pools, num, cpu_num, buff_get);
	PP3_DBG_POOLS_STATS_DUMP(pools, num, cpu_num, buff_get_zero);
	PP3_DBG_POOLS_STATS_DUMP(pools, num, cpu_num, buff_get_dummy);
	PP3_DBG_POOLS_STATS_DUMP(pools, num, cpu_num, buff_free);
#ifdef CONFIG_MV_PP3_TSO
	PP3_DBG_POOLS_STATS_DUMP(pools, num, cpu_num, buff_free_tso);
#endif
}
/*---------------------------------------------------------------------------*/

static void pp3_dbg_dev_rx_pools_stats_dump(struct pp3_dev_priv *dev_priv)
{
	char name[15];
	struct pp3_pool *pools[2];
	int i, num = 0;
	u32 cpus_num;

	pr_cont("\n%-24s", "RX pools stats:");
	pools[num] = dev_priv->cpu_shared->long_pool;
	sprintf(name, "Long-[%d]", pools[num] ? pools[num]->pool : -1);
	pr_cont("%-45s", name);
	num++;

	if (dev_priv->cpu_shared->short_pool) {
		pools[num] = dev_priv->cpu_shared->short_pool;
		sprintf(name, "Short-[%d]", pools[num] ? pools[num]->pool : -1);
		pr_cont("%-45s", name);
		num++;
	}
	pr_cont("\n");
	pp3_dbg_separate_line_print(24 + (45 * num));

	pr_cont("%-24s", "buff_num");
	for (i = 0; i < num; i++)
		pr_cont("%-45u", pools[i]->buf_num);

	pr_cont("\n");
	pr_cont("%-24s", "buff_in_use");
	for (i = 0; i < num; i++)
		pr_cont("%-45u", atomic_read(&pools[i]->in_use));

	pr_info("\n");

	cpus_num = num_possible_cpus();
	pp3_dbg_pools_stats_dump(pools, num, cpus_num, -1, "CPUs");
	pp3_dbg_separate_line_print(24 + (45 * num));

	PP3_DBG_POOLS_STATS_DUMP(pools, num, cpus_num, buff_rx);
	PP3_DBG_POOLS_STATS_DUMP(pools, num, cpus_num, buff_put);
	PP3_DBG_POOLS_STATS_DUMP(pools, num, cpus_num, buff_alloc);
	PP3_DBG_POOLS_STATS_DUMP(pools, num, cpus_num, buff_alloc_err);
#ifdef CONFIG_MV_PP3_SKB_RECYCLE
	PP3_DBG_POOLS_STATS_DUMP(pools, num, cpus_num, buff_recycled_ok);
	PP3_DBG_POOLS_STATS_DUMP(pools, num, cpus_num, buff_recycled_err);
#endif /* CONFIG_MV_PP3_SKB_RECYCLE */
}
/*---------------------------------------------------------------------------*/

/* Clear global statistics for all CPUs */
void pp3_dbg_cpu_stats_clear(void)
{
	int cpu;

	for_each_possible_cpu(cpu)
		memset(&pp3_cpus[cpu]->stats, 0, sizeof(struct pp3_cpu_stats));

}

/*---------------------------------------------------------------------------*/
/* Clear timers statistics relevant for network device */
static void pp3_dbg_dev_timers_stats_clear(struct pp3_dev_priv *dev_priv)
{
	int cpu;
	struct pp3_vport *cpu_vp;

	for_each_possible_cpu(cpu) {
		cpu_vp = dev_priv->cpu_vp[cpu];
		memset(&cpu_vp->port.cpu.txdone_timer.stats, 0, sizeof(struct pp3_timer_stats));
	}
}
/*---------------------------------------------------------------------------*/
/* Print timers statistics relevant for network device */
static void pp3_dbg_dev_timers_stats_dump(struct net_device *dev)
{
	int cpu;
	struct pp3_vport *cpu_vp;
	struct pp3_dev_priv *dev_priv = MV_PP3_PRIV(dev);

	/* print CPU statistics */
	pr_cont("\n%-24s", "timers stats:");

	for_each_possible_cpu(cpu)
		pr_cont("CPU%-10d", cpu);

	pr_info("-------------------------------------------------------------\n");

	pr_cont("%-24s", "timer_add");
	for_each_possible_cpu(cpu) {
		cpu_vp = dev_priv->cpu_vp[cpu];
		if (cpu_vp)
			pr_cont("%-13u", cpu_vp->port.cpu.txdone_timer.stats.timer_add);
	}

	pr_cont("\n%-24s", "timer_sched");
	for_each_possible_cpu(cpu) {
		cpu_vp = dev_priv->cpu_vp[cpu];
		if (cpu_vp)
			pr_cont("%-13u", cpu_vp->port.cpu.txdone_timer.stats.timer_sched);
	}


}
/*---------------------------------------------------------------------------*/

/* Print Firmware statistics relevant for network device */
void pp3_dbg_dev_fw_stats_dump(struct net_device *dev)
{
	struct pp3_dev_priv *dev_priv = MV_PP3_PRIV(dev);

	pr_info("%s: Firmware stats\n", dev_priv->dev->name);
	if (dev_priv->vport)
		mv_pp3_vport_fw_stats_dump(dev_priv->vport->vport);
}
/*---------------------------------------------------------------------------*/

/* Clear all Host software collected statistics relevant for network interface */
void pp3_dbg_dev_stats_clear(struct net_device *dev)
{
	struct pp3_dev_priv *dev_priv = MV_PP3_PRIV(dev);

	if (dev_priv->vport)
		mv_pp3_vport_fw_stats_clear(dev_priv->vport->vport);

	/* Clear global statistics */
	pp3_dbg_cpu_stats_clear();

	/* Clear tx done timer statistics */
	pp3_dbg_dev_timers_stats_clear(dev_priv);

	/* Clear BM pool statistics */
	pp3_dbg_dev_pool_stats_clear(dev_priv);

	/* Clear RXQs and TXQs statistics */
	pp3_dbg_txqs_stats_clear(dev_priv);
	pp3_dbg_rxqs_stats_clear(dev_priv);

	/* Clear Group statistics */
	pp3_dbg_dev_nic_stats_clear(dev_priv);
}
/*---------------------------------------------------------------------------*/

/* Print all Host software collected statistics relevant for network interface */
void pp3_dbg_dev_pools_stats_dump(struct net_device *dev)
{
	struct pp3_dev_priv *dev_priv = MV_PP3_PRIV(dev);

	/* print BM pool statistics */
	pp3_dbg_dev_rx_pools_stats_dump(dev_priv);
	pp3_dbg_dev_lnx_pools_stats_dump(dev_priv);
}

/* Print all Host software collected statistics relevant for network interface */
void pp3_dbg_dev_stats_dump(struct net_device *dev)
{
	struct pp3_dev_priv *dev_priv = mv_pp3_dev_priv_ready_get(dev);

	if (!dev_priv)
		return;

	/*Print timers statistics */
	pp3_dbg_dev_timers_stats_dump(dev);

	/* aggregation of rxqs and txqs counters */
	pp3_dbg_dev_nic_cnt_update_all(dev_priv);

	pr_cont("\n");

	/* print BM pool statistics */
	pp3_dbg_dev_rx_pools_stats_dump(dev_priv);
	pp3_dbg_dev_lnx_pools_stats_dump(dev_priv);

	/* NIC statistics */
	pp3_dbg_dev_nic_stats_dump(dev_priv);
}
/*---------------------------------------------------------------------------*/

/* Print all Host software queues collected statistics relevant for network interface */
void pp3_dbg_dev_queues_stats_dump(struct net_device *dev)
{
	struct pp3_dev_priv *dev_priv = mv_pp3_dev_priv_ready_get(dev);

	if (!dev_priv)
		return;

	pr_cont("\n");

	pp3_dbg_dev_vqs_stats_dump(dev_priv);

	pr_cont("\n");
}
/*---------------------------------------------------------------------------*/

void pp3_dbg_dev_status_print(struct net_device *dev)
{
	struct pp3_dev_priv *dev_priv = mv_pp3_dev_priv_ready_get(dev);
	int cpu;
	char cpus_str[16];

	if (!dev_priv)
		return;

	scnprintf(cpus_str, sizeof(cpus_str), "%*pb", cpumask_pr_args(&dev_priv->rx_cpus));

	pr_info("\n");
	pr_info("Interface %s:\n", dev->name);

#ifdef CONFIG_MV_PP3_DEBUG_CODE
	pr_info("Flags                       : 0x%x\n", (unsigned)dev_priv->flags);
	pr_info("GNSS ops                    : %p\n", dev_priv->cpu_shared->gnss_ops);
#endif
#ifdef CONFIG_MV_PP3_SKB_RECYCLE
	pr_info("SKB recycle                 : %s\n",  mv_pp3_is_nic_skb_recycle() ? "Enable" : "Disable");
#endif

	pr_info("RX packet mode              : %s\n", mv_pp3_pkt_mode_str(dev_priv->cpu_shared->rx_pkt_mode));
	pr_info("RX CPUs mask                : 0x%s\n", cpus_str);
	pr_info("RX default CPU              : %d\n", MV_PP3_CPU_VPORT_TO_CPU(dev_priv->vport->dest_vp));
	pr_info("RXQ maximum capacity [pkts] : %d\n", dev_priv->rxq_capacity);
	pr_info("TXQ maximum capacity [pkts] : %d\n", dev_priv->txq_capacity);
	pr_info("RX coalescing [pkts]        : %d\n", dev_priv->rx_pkt_coal);
	pr_info("RX time coalescing [usec]   : %d\n", dev_priv->rx_time_coal);

	pr_info("Long pool                   : %d\n",
		dev_priv->cpu_shared->long_pool ? dev_priv->cpu_shared->long_pool->pool : -1);

	pr_info("Short pool                  : %d\n",
		dev_priv->cpu_shared->short_pool ? dev_priv->cpu_shared->short_pool->pool : -1);

	if (dev_priv->cpu_shared->lro_pool)
		pr_info("LRO pool                    : %d\n", dev_priv->cpu_shared->lro_pool->pool);

	pr_info("Internal CPU vports         : ");
	for_each_possible_cpu(cpu) {
		if (dev_priv->cpu_vp[cpu])
			pr_cont("%d   ", dev_priv->cpu_vp[cpu]->vport);
	}
	pr_info("Number of ingress VQs       : ");
	for_each_possible_cpu(cpu) {
		if (dev_priv->cpu_vp[cpu])
			pr_cont("%d   ", dev_priv->cpu_vp[cpu]->rx_vqs_num);
	}
	pr_info("Number of egress VQs        : ");
	for_each_possible_cpu(cpu) {
		if (dev_priv->cpu_vp[cpu])
			pr_cont("%d   ", dev_priv->cpu_vp[cpu]->tx_vqs_num);
	}

	if (dev_priv->vport->type == MV_PP3_NSS_PORT_ETH)
		pr_info("EMAC vport                  : %d\n", dev_priv->vport->vport);
	else if (dev_priv->vport->type == MV_PP3_NSS_PORT_EXT)
		pr_info("External vport              : %d\n", dev_priv->vport->vport);

	return;
}
/*---------------------------------------------------------------------------*/

void pp3_dbg_dev_pools_status_print(struct net_device *dev)
{
	struct pp3_dev_priv *dev_priv = mv_pp3_dev_priv_ready_get(dev);

	if (!dev_priv)
		return;

	if (dev_priv->cpu_shared->long_pool)
		pp3_dbg_pool_status_print(dev_priv->cpu_shared->long_pool->pool);

	if (dev_priv->cpu_shared->short_pool)
		pp3_dbg_pool_status_print(dev_priv->cpu_shared->short_pool->pool);

	if (dev_priv->cpu_shared->lro_pool)
		pp3_dbg_pool_status_print(dev_priv->cpu_shared->lro_pool->pool);
}
/*---------------------------------------------------------------------------*/

void pp3_dbg_dev_flags(struct net_device *dev, u32 flag, u32 en)
{
	struct pp3_dev_priv *dev_priv = MV_PP3_PRIV(dev);
	u32 bit_flag;

	bit_flag = (fls(flag) - 1);

	if (en)
		dev_priv->flags |= (1 << bit_flag);
	else
		dev_priv->flags &= ~(1 << bit_flag);

	return;
}

/*---------------------------------------------------------------------------*/

void pp3_dbg_cpu_flags(int cpu, u32 flag, u32 en)
{
	u32 bit_flag;

	bit_flag = (fls(flag) - 1);

	if (en)
		pp3_cpus[cpu]->flags |= (1 << bit_flag);
	else
		pp3_cpus[cpu]->flags &= ~(1 << bit_flag);

	return;
}
/*---------------------------------------------------------------------------*/

void pp3_dbg_dev_mac_show(struct net_device *dev)
{
	struct netdev_hw_addr *ha;
	int i;
	char prefix[16], suffix[32];

	pr_info("%s: MAC addresses\n", dev->name);
	mv_mac_addr_print("device_addr   : ", dev->dev_addr, NULL);
	mv_mac_addr_print("permanent_addr: ", dev->perm_addr, NULL);

	i = 0;
	pr_info("\nunicast list: dev->uc\n");
	netdev_for_each_uc_addr(ha, dev) {
		sprintf(prefix, "%2d: ", i++);
		sprintf(suffix, " - ref = %d, global = %s",
			ha->refcount, ha->global_use ? "true" : "false");
		mv_mac_addr_print(prefix, ha->addr, suffix);
	}
	i = 0;
	pr_info("\nmulticast list: dev->mc\n");
	netdev_for_each_mc_addr(ha, dev) {
		sprintf(prefix, "%2d: ", i++);
		sprintf(suffix, " - ref = %d, global = %s",
			ha->refcount, ha->global_use ? "true" : "false");
		mv_mac_addr_print(prefix, ha->addr, suffix);
	}
}
/*---------------------------------------------------------------------------*/

void pp3_dbg_skb_dump(struct sk_buff *skb)
{
	pr_info("skb=%p: head=%p, data=%p, tail=%p, end=%p, ksize(head)=%d\n",
			skb, skb->head, skb->data, skb->tail, skb->end, ksize(skb->head));
	pr_info("\t mac=%p, network=%p, transport=%p, headroom=%d, end_offs=%d\n",
			skb_mac_header(skb), skb_network_header(skb), skb_transport_header(skb),
			skb_headroom(skb), skb_end_offset(skb));
	pr_info("\t truesize=%d, len=%d, data_len=%d, mac_len=%d\n",
			skb->truesize, skb->len, skb->data_len, skb->mac_len);
	pr_info("\t users=%d, dataref=%d, nr_frags=%d, gso_size=%d, gso_segs=%d\n",
			atomic_read(&skb->users), atomic_read(&skb_shinfo(skb)->dataref),
			skb_shinfo(skb)->nr_frags, skb_shinfo(skb)->gso_size, skb_shinfo(skb)->gso_segs);
	pr_info("\t proto=%d, ip_summed=%d, priority=%d cloned=%d\n\n",
			ntohs(skb->protocol), skb->ip_summed, skb->priority, skb->cloned);
}
/*---------------------------------------------------------------------------*/

char *pp3_dbg_l3_info_str(unsigned int l3_info)
{
	switch (l3_info) {

	case L3_RX_IP4:
		return "ipv4";
	case L3_RX_IP4_FRAG:
		return "ipv4 frag";
	case L3_RX_IP4_OPT:
		return "ipv4 opt";
	case L3_RX_IP4_ERR:
		return "ipv4 error";
	case L3_RX_IP6:
		return "ipv6";
	case L3_RX_IP6_EXT:
		return "ipv6 ext";
	case L3_RX_ARP:
		return "arp";
	default:
		return "Unknown";
	}
	return NULL;
}
/*---------------------------------------------------------------------------*/
char *pp3_dbg_l4_info_str(unsigned int l4_info)
{
	switch (l4_info) {

	case L4_RX_TCP:
		return "tcp";
	case L4_RX_TCP_CS_ERR:
		return "tcp(csum=BAD)";
	case L4_RX_UDP:
		return "udp";
	case L4_RX_UDP_LITE:
		return "udp lite";
	case L4_RX_UDP_CS_ERR:
		return "udp(csum=BAD)";
	case L4_RX_IGMP:
		return "igmp";
	default:
		return "Unknown";
	}

	return NULL;
}
/*---------------------------------------------------------------------------*/
char *pp3_dbg_l2_info_str(unsigned int l2_info)
{
	switch (l2_info) {

	case L2_UCAST:
		return "ucast";
	case L2_MCAST:
		return "mcast";
	case L2_IP_MCAST:
		return "ip_mcast";
	case L2_BCAST:
		return "bcast";

	default:
		return "Unknown";
	}

	return NULL;
}
/*---------------------------------------------------------------------------*/
char *pp3_dbg_vlan_info_str(unsigned int vlan_info)
{
	switch (vlan_info) {

	case VLAN_UNTAGGED:
		return "untagged";
	case VLAN_SINGLE:
		return "single";
	case VLAN_DOUBLE:
		return "double";
	default:
		return "Unknown";
	}

	return NULL;
}
/*---------------------------------------------------------------------------*/
void pp3_dbg_cfh_hdr_dump(struct mv_cfh_common *cfh)
{
	if (!cfh)
		return;

	pr_info("cfh_hdr:\n");
	pr_info("%p:  0x%8.8X 0x%8.8X 0x%8.8X 0x%8.8X 0x%8.8X 0x%8.8X 0x%8.8X 0x%8.8X\n",
		cfh, cfh->plen_order, cfh->ctrl, cfh->tag1, cfh->tag2,
		cfh->phys_l, cfh->vm_bp, cfh->marker_l, cfh->l3_l4_info);
}

/*---------------------------------------------------------------------------*/
void pp3_dbg_cfh_common_dump(struct mv_cfh_common *cfh)
{
	int data_size, cfh_len, pkt_len;

	cfh_len = MV_CFH_LEN_GET(cfh->ctrl);
	pkt_len = MV_CFH_PKT_LEN_GET(cfh->plen_order);

	data_size = MV_MIN(cfh_len - MV_PP3_CFH_HDR_SIZE, pkt_len);

	pp3_dbg_cfh_hdr_dump(cfh);

	if (data_size > 0) {
		u8 *tmp = (u8 *)(cfh+1);
		pr_info("cfh data:");
		mv_debug_mem_dump((void *)tmp, data_size, 1);
	}
}
/*---------------------------------------------------------------------------*/

int pp3_dbg_cfh_rx_checker(struct pp3_dev_priv *dev_priv, u32 *ptr)
{
	int bpid, pkt_len, cfh_len, wr_offset, cpu, swq, q;
	struct mv_cfh_common *cfh = (struct mv_cfh_common *) ptr;
	struct pp3_vport *cpu_vp;
	bool flag = false;

	cfh_len = MV_CFH_LEN_GET(cfh->ctrl);
	bpid = MV_CFH_BPID_GET(cfh->vm_bp);
	pkt_len = MV_CFH_PKT_LEN_GET(cfh->plen_order);
	wr_offset = MV_CFH_WR_GET(cfh->ctrl) * MV_CFH_WR_RES;

	/* The following cases are supported:
	*  1. cfh_len is 64 bytes, the whole packet is in DRAM buffer
	*  2. cfh_len is always 128 bytes, packet header is in CFH.
	*/
	if ((cfh_len != (MV_PP3_CFH_HDR_SIZE + MV_PP3_CFH_MDATA_SIZE)) && (cfh_len != MV_PP3_CFH_MAX_SIZE)) {
		pr_err("%s: invalid cfh_length %d\n", __func__, cfh_len);
		goto err;
	}

	if ((pkt_len <= (cfh_len - MV_PP3_CFH_HDR_SIZE)) && (cfh->phys_l)) {
		pr_err("%s: unneseccary BM buffer allocate by FW\n", __func__);
		pr_err("%s: cfh_len = %d, pkt_len = %d phys_l = 0x%8x\n", __func__, cfh_len, pkt_len, cfh->phys_l);
		goto err;
	}

	if ((pkt_len - MV_PP3_CFH_MDATA_SIZE) > MV_RX_PKT_SIZE(dev_priv->dev->mtu)) {
		pr_err("%s: Invalid packet length %d, MTU = %d, max packet length = %d",
				__func__, pkt_len, dev_priv->dev->mtu, MV_RX_PKT_SIZE(dev_priv->dev->mtu));
		goto err;
	}

	if (cfh_len == MV_PP3_CFH_MAX_SIZE) {
		pkt_len -= MV_PP3_CFH_MDATA_SIZE;
		if (pkt_len >= MV_PP3_CFH_PAYLOAD_MAX_SIZE) {
			pr_err("%s: Unexpected Split packet", __func__);
			goto err;
		}
	}

	cpu = smp_processor_id();
	if (!dev_priv->cpu_vp[cpu])
		goto err;

	cpu_vp = dev_priv->cpu_vp[cpu];
	/* validate SWQ number */
	swq = MV_CFH_SWQ_GET(cfh->ctrl);

	for (q = 0; q < cpu_vp->rx_vqs_num; q++) {
		struct pp3_swq *rxq_ctrl = cpu_vp->rx_vqs[q]->swq;

		if (rxq_ctrl && (swq == MV_PP3_HMAC_PHYS_SWQ_NUM(rxq_ctrl->swq, rxq_ctrl->frame_num))) {
			flag = true;
			break;
		}
	}
	if (!flag) {
		pr_err("%s: invalid swq number %d\n", __func__, swq);
		goto err;
	}

	/* validate BM pool number and buffer pointers */
	if (cfh->phys_l) {
		struct sk_buff *skb = (struct sk_buff *)cfh->marker_l;

		if (!skb) {
			pr_err("%s: invalid marker_l\n", __func__);
			goto err;
		}

		if ((dev_priv->cpu_shared->long_pool && (dev_priv->cpu_shared->long_pool->pool != bpid)) &&
		   (dev_priv->cpu_shared->short_pool && (dev_priv->cpu_shared->short_pool->pool != bpid)) &&
		   (dev_priv->cpu_shared->lro_pool && (dev_priv->cpu_shared->lro_pool->pool != bpid))) {
			pr_err("%s: invalid bm pool %d\n", __func__, bpid);
			goto err;
		}

		if (virt_to_phys(skb->head) != cfh->phys_l) {
			pr_err("%s: incorrect BM pointers\n", __func__);
			pr_err("%s: virt_l: skb=%p, head=%p (0x%x), phys_l: 0x%0x\n",
				__func__, skb, skb->head, virt_to_phys(skb->head), cfh->phys_l);
			goto err;
		}
	}
	return 0;

err:
	pr_err("%s: Error - invalid CFH found\n", dev_priv->dev->name);
	pp3_dbg_cfh_rx_dump(cfh);
	return -1;
}
/*---------------------------------------------------------------------------*/

void pp3_dbg_cfh_rx_dump(struct mv_cfh_common *rx_cfh)
{
	pp3_dbg_cfh_common_dump(rx_cfh);

	if (MV_CFH_PP_MODE_GET(rx_cfh->ctrl) == PP_RX_REASEM_PACKET) {
		pr_info("Reassembly CFH, skip over 32 bytes");
		rx_cfh++;
	}

	pr_info("cfh=0x%p, pp_mode = %d, pkt_len=%d, wr_offs=%d, sw_q=%d, cfh_len=%d, mdata = %d\n",
			rx_cfh,
			MV_CFH_PP_MODE_GET(rx_cfh->ctrl),
			MV_CFH_PKT_LEN_GET(rx_cfh->plen_order),
			MV_CFH_WR_GET(rx_cfh->ctrl),
			MV_CFH_SWQ_GET(rx_cfh->ctrl),
			MV_CFH_LEN_GET(rx_cfh->ctrl),
			MV_CFH_MDATA_BIT_GET(rx_cfh->ctrl));

	pr_info("phys_addr=0x%02x%08x, virt_add= 0x%02x%08x, pool_id=%d\n",
			MV_CFH_PHYS_H_GET(rx_cfh->vm_bp), rx_cfh->phys_l,
			MV_CFH_VIRT_H_GET(rx_cfh->l3_l4_info), rx_cfh->marker_l, MV_CFH_BPID_GET(rx_cfh->vm_bp));

	pr_info("L2_info=%s, Vlan_info=%s, L3_offs=%d, L3_info=%s, IP_hlen=%d, L4_info=%s\n",
		pp3_dbg_l2_info_str(MV_CFH_L2_INFO_GET(rx_cfh->l3_l4_info)),
		pp3_dbg_vlan_info_str(MV_CFH_VLAN_INFO_GET(rx_cfh->l3_l4_info)),
		MV_CFH_L3_OFFS_GET(rx_cfh->l3_l4_info),
		pp3_dbg_l3_info_str(MV_CFH_L3_INFO_RX_GET(rx_cfh->l3_l4_info)),
		MV_CFH_IPHDR_LEN_GET(rx_cfh->l3_l4_info),
		pp3_dbg_l4_info_str(MV_CFH_L4_INFO_RX_GET(rx_cfh->l3_l4_info)));

	pr_info("Mgmt=%d, Mac2Me=%d\n",
			MV_CFH_MGMT_BIT_GET(rx_cfh->l3_l4_info),
			MV_CFH_MACME_BIT_GET(rx_cfh->l3_l4_info));
}
/*---------------------------------------------------------------------------*/
