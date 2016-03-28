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
/* includes */
#include "common/mv_sw_if.h"
#include "qm/mv_qm_regs.h"

struct qm_alias qm;
struct qm_alias qm_reg_size;
struct qm_alias qm_reg_offset;

void qm_reg_address_alias_init(u32 siliconBase)
{
	qm.ql.base      = siliconBase +      QL_UNIT_OFFSET;	/*0x00400000*/
	qm.pfe.base     = siliconBase +     PFE_UNIT_OFFSET;	/*0x00410000*/
	qm.dqf.base     = siliconBase +     DQF_UNIT_OFFSET;	/*0x00420000*/
	qm.dma.base     = siliconBase +     DMA_UNIT_OFFSET;	/*0x00430000*/
	qm.reorder.base = siliconBase + REORDER_UNIT_OFFSET;	/*0x00500000*/

	/* QL registers addresses */
	qm.ql.qptr                 = qm.ql.base + 0x00000000;
	qm.ql.qmap_port            = qm.ql.base + 0x00008000;
	qm.ql.qmap_group           = qm.ql.base + 0x00009000;
	qm.ql.rule_bpi             = qm.ql.base + 0x0000A000;
	qm.ql.low_threshold        = qm.ql.base + 0x00000400;
	qm.ql.pause_threshold      = qm.ql.base + 0x00000404;
	qm.ql.high_threshold       = qm.ql.base + 0x00000408;
	qm.ql.traffic_source       = qm.ql.base + 0x0000040C;
	qm.ql.ECC_error_cause      = qm.ql.base + 0x00000500;
	qm.ql.ECC_error_mask       = qm.ql.base + 0x00000504;
	qm.ql.Internal_error_cause = qm.ql.base + 0x00000508;
	qm.ql.internal_error_mask  = qm.ql.base + 0x0000050C;
	qm.ql.nss_general_purpose  = qm.ql.base + 0x00001000;
	qm.ql.qlen                 = qm.ql.base + 0x00002000;
	qm.ql.bp_qlen              = qm.ql.base + 0x00004000;
	qm.ql.drop_qlen            = qm.ql.base + 0x00006000;
	qm.ql.xoff_when_bm_empty_en = qm.ql.base + 0x0000600;
	qm.ql.xoff_mac_qnum        = qm.ql.base + 0x0000604;
	qm.ql.xoff_hmac_qs         = qm.ql.base + 0x0000620;


	/* PFE registers addresses */
	qm.pfe.qece_dram_base_address_hi         = qm.pfe.base + 0x00000000;
	qm.pfe.pyld_dram_base_address_hi         = qm.pfe.base + 0x00000004;
	qm.pfe.qece_dram_base_address_lo         = qm.pfe.base + 0x00000008;
	qm.pfe.pyld_dram_base_address_lo         = qm.pfe.base + 0x0000000C;
	qm.pfe.QM_VMID                           = qm.pfe.base + 0x00000010;
	qm.pfe.port_flush                        = qm.pfe.base + 0x0000001C;
	qm.pfe.AXI_read_attributes_for_swf_mode  = qm.pfe.base + 0x00000030;
	qm.pfe.AXI_read_attributes_for_rdma_mode = qm.pfe.base + 0x00000034;
	qm.pfe.AXI_read_attributes_for_hwf_qece  = qm.pfe.base + 0x00000038;
	qm.pfe.AXI_read_attributes_for_hwf_pyld  = qm.pfe.base + 0x0000003C;
	qm.pfe.max_credit_for_new_dram_req       = qm.pfe.base + 0x00000050;
	qm.pfe.ecc_error_cause                   = qm.pfe.base + 0x00000100;
	qm.pfe.ecc_error_mask                    = qm.pfe.base + 0x00000104;
	qm.pfe.internal_error_cause              = qm.pfe.base + 0x00000108;
	qm.pfe.internal_error_mask               = qm.pfe.base + 0x0000010C;
	qm.pfe.idle_status                       = qm.pfe.base + 0x00000110;
	qm.pfe.queue_flush                       = qm.pfe.base + 0x00000400;
	qm.pfe.queue_qece                        = qm.pfe.base + 0x00008000;

	/* DQF registers addresses */
	qm.dqf.Data_FIFO_params_p               = qm.dqf.base + 0x00000000;
	qm.dqf.Credit_Threshold_p               = qm.dqf.base + 0x00000040;
	qm.dqf.PPC_port_map_p                   = qm.dqf.base + 0x00000080;
	qm.dqf.data_fifo_pointers_p             = qm.dqf.base + 0x000000C0;
	qm.dqf.dqf_itnr_cause                   = qm.dqf.base + 0x00000100;
	qm.dqf.dqf_itnr_mask                    = qm.dqf.base + 0x00000104;
	qm.dqf.misc_error_intr_cause            = qm.dqf.base + 0x00000108;
	qm.dqf.misc_error_intr_mask             = qm.dqf.base + 0x00000104;
	qm.dqf.dqf_ser_summary_intr_cause       = qm.dqf.base + 0x00000110;
	qm.dqf.dqf_ser_summary_intr_mask        = qm.dqf.base + 0x00000114;
	qm.dqf.write_to_full_error_intr_cause   = qm.dqf.base + 0x00000118;
	qm.dqf.write_to_full_error_intr_mask    = qm.dqf.base + 0x0000011C;
	qm.dqf.read_from_empty_error_intr_cause = qm.dqf.base + 0x00000120;
	qm.dqf.read_from_empty_error_intr_mask  = qm.dqf.base + 0x00000124;
	qm.dqf.wrong_axi_rd_error_intr_cause    = qm.dqf.base + 0x00000128;
	qm.dqf.wrong_axi_rd_error_intr_mask     = qm.dqf.base + 0x0000012C;
	qm.dqf.mg2mem_req_addr_ctrl             = qm.dqf.base + 0x00000130;
	qm.dqf.mem2mg_resp_status               = qm.dqf.base + 0x00000134;
	qm.dqf.mem2mg_resp_data_hh              = qm.dqf.base + 0x00000138;
	qm.dqf.mem2mg_resp_data_hl              = qm.dqf.base + 0x0000013C;
	qm.dqf.mem2mg_resp_data_lh              = qm.dqf.base + 0x00000140;
	qm.dqf.mem2mg_resp_data_ll              = qm.dqf.base + 0x00000144;
	qm.dqf.dqf_macs_l3_res                  = qm.dqf.base + 0x00001000;
	qm.dqf.dqf_macs_l4_res                  = qm.dqf.base + 0x00001400;
	qm.dqf.dqf_macs_l3_ptr                  = qm.dqf.base + 0x00001800;
	qm.dqf.dqf_macs_l4_ptr                  = qm.dqf.base + 0x00001C00;
	qm.dqf.dqf_macs_desc                    = qm.dqf.base + 0x00002000;

	/* DMA registers addresses */
	qm.dma.Q_memory_allocation                = qm.dma.base + 0x00000000;
	qm.dma.gpm_thresholds                     = qm.dma.base + 0x00000050;
	qm.dma.dram_thresholds                    = qm.dma.base + 0x00000054;
	qm.dma.AXI_write_attributes_for_swf_mode  = qm.dma.base + 0x00000060;
	qm.dma.AXI_write_attributes_for_rdma_mode = qm.dma.base + 0x00000064;
	qm.dma.AXI_write_attributes_for_hwf_qece  = qm.dma.base + 0x00000068;
	qm.dma.AXI_write_attributes_for_hwf_pyld  = qm.dma.base + 0x0000006C;
	qm.dma.DRAM_VMID                          = qm.dma.base + 0x00000070;
	qm.dma.tail_pointer_en			  = qm.dma.base + 0x0000007C;
	qm.dma.idle_status                        = qm.dma.base + 0x00000080;
	qm.dma.ecc_error_cause                    = qm.dma.base + 0x00000100;
	qm.dma.ecc_error_mask                     = qm.dma.base + 0x00000104;
	qm.dma.internal_error_cause               = qm.dma.base + 0x00000108;
	qm.dma.internal_error_mask                = qm.dma.base + 0x0000010C;
	qm.dma.ceram_mac                          = qm.dma.base + 0x00000800;
	qm.dma.ceram_ppe                          = qm.dma.base + 0x00001000;
	qm.dma.qeram                              = qm.dma.base + 0x00001800;
	qm.dma.dram_fifo                          = qm.dma.base + 0x00002000;

	/* REORDER registers addresses */
	qm.reorder.ru_qe              = qm.reorder.base + 0x00000000;
	qm.reorder.ru_class           = qm.reorder.base + 0x00020000;
	qm.reorder.ru_tasks           = qm.reorder.base + 0x00028000;
	qm.reorder.ru_ptr2next        = qm.reorder.base + 0x00030000;
	qm.reorder.ru_sid_fifo        = qm.reorder.base + 0x00038000;
	qm.reorder.ru_port2class      = qm.reorder.base + 0x00040000;
	qm.reorder.ru_pool            = qm.reorder.base + 0x00090000;
	qm.reorder.ru_class_head      = qm.reorder.base + 0x00090100;
	qm.reorder.ru_ser_error_cause = qm.reorder.base + 0x00090500;
	qm.reorder.ru_ser_error_mask  = qm.reorder.base + 0x00090504;
	qm.reorder.ru_host_cmd        = qm.reorder.base + 0x00090580;
	qm.reorder.ru_task_permission = qm.reorder.base + 0x00090584;
}

void qm_reg_size_alias_init(void)
{
	u32 word_size_in_bits, byte_size_in_bits = 8;

	word_size_in_bits = 32/byte_size_in_bits;	/* word_size_in_bits = 4 */

	/* QL registers sizes */
	qm_reg_size.ql.qptr                 = 32/byte_size_in_bits/word_size_in_bits;
	qm_reg_size.ql.qmap_port            = 32/byte_size_in_bits/word_size_in_bits;
	qm_reg_size.ql.qmap_group           = 32/byte_size_in_bits/word_size_in_bits;
	qm_reg_size.ql.rule_bpi             = 64/byte_size_in_bits/word_size_in_bits;
	qm_reg_size.ql.low_threshold        = 32/byte_size_in_bits/word_size_in_bits;
	qm_reg_size.ql.pause_threshold      = 32/byte_size_in_bits/word_size_in_bits;
	qm_reg_size.ql.high_threshold       = 32/byte_size_in_bits/word_size_in_bits;
	qm_reg_size.ql.traffic_source       = 32/byte_size_in_bits/word_size_in_bits;
	qm_reg_size.ql.ECC_error_cause      = 32/byte_size_in_bits/word_size_in_bits;
	qm_reg_size.ql.ECC_error_mask       = 32/byte_size_in_bits/word_size_in_bits;
	qm_reg_size.ql.Internal_error_cause = 32/byte_size_in_bits/word_size_in_bits;
	qm_reg_size.ql.internal_error_mask  = 32/byte_size_in_bits/word_size_in_bits;
	qm_reg_size.ql.nss_general_purpose  = 32/byte_size_in_bits/word_size_in_bits;
	qm_reg_size.ql.qlen                 = 32/byte_size_in_bits/word_size_in_bits;
	qm_reg_size.ql.bp_qlen               = 32/byte_size_in_bits/word_size_in_bits;
	qm_reg_size.ql.drop_qlen             = 32/byte_size_in_bits/word_size_in_bits;
	qm_reg_size.ql.xoff_hmac_qs         = 32/byte_size_in_bits/word_size_in_bits;
	/* PFE registers sizes */
	qm_reg_size.pfe.qece_dram_base_address_hi         =  32/byte_size_in_bits/word_size_in_bits;
	qm_reg_size.pfe.pyld_dram_base_address_hi         =  32/byte_size_in_bits/word_size_in_bits;
	qm_reg_size.pfe.qece_dram_base_address_lo         =  32/byte_size_in_bits/word_size_in_bits;
	qm_reg_size.pfe.pyld_dram_base_address_lo         =  32/byte_size_in_bits/word_size_in_bits;
	qm_reg_size.pfe.QM_VMID                           =  32/byte_size_in_bits/word_size_in_bits;
	qm_reg_size.pfe.port_flush                        =  32/byte_size_in_bits/word_size_in_bits;
	qm_reg_size.pfe.AXI_read_attributes_for_swf_mode  =  32/byte_size_in_bits/word_size_in_bits;
	qm_reg_size.pfe.AXI_read_attributes_for_rdma_mode =  32/byte_size_in_bits/word_size_in_bits;
	qm_reg_size.pfe.AXI_read_attributes_for_hwf_qece  =  32/byte_size_in_bits/word_size_in_bits;
	qm_reg_size.pfe.AXI_read_attributes_for_hwf_pyld  =  32/byte_size_in_bits/word_size_in_bits;
	qm_reg_size.pfe.ecc_error_cause                   =  32/byte_size_in_bits/word_size_in_bits;
	qm_reg_size.pfe.ecc_error_mask                    =  32/byte_size_in_bits/word_size_in_bits;
	qm_reg_size.pfe.internal_error_cause              =  32/byte_size_in_bits/word_size_in_bits;
	qm_reg_size.pfe.internal_error_mask               =  32/byte_size_in_bits/word_size_in_bits;
	qm_reg_size.pfe.idle_status                       =  32/byte_size_in_bits/word_size_in_bits;
	qm_reg_size.pfe.queue_flush                       =  32/byte_size_in_bits/word_size_in_bits;
	qm_reg_size.pfe.queue_qece                        = 128/byte_size_in_bits/word_size_in_bits;

	/* DQF registers sizes */
	qm_reg_size.dqf.Data_FIFO_params_p               = 32/byte_size_in_bits/word_size_in_bits;
	qm_reg_size.dqf.Credit_Threshold_p               = 32/byte_size_in_bits/word_size_in_bits;
	qm_reg_size.dqf.PPC_port_map_p                   = 32/byte_size_in_bits/word_size_in_bits;
	qm_reg_size.dqf.data_fifo_pointers_p             = 32/byte_size_in_bits/word_size_in_bits;
	qm_reg_size.dqf.dqf_itnr_cause                   = 32/byte_size_in_bits/word_size_in_bits;
	qm_reg_size.dqf.dqf_itnr_mask                    = 32/byte_size_in_bits/word_size_in_bits;
	qm_reg_size.dqf.misc_error_intr_cause            = 32/byte_size_in_bits/word_size_in_bits;
	qm_reg_size.dqf.misc_error_intr_mask             = 32/byte_size_in_bits/word_size_in_bits;
	qm_reg_size.dqf.dqf_ser_summary_intr_cause       = 32/byte_size_in_bits/word_size_in_bits;
	qm_reg_size.dqf.dqf_ser_summary_intr_mask        = 32/byte_size_in_bits/word_size_in_bits;
	qm_reg_size.dqf.write_to_full_error_intr_cause   = 32/byte_size_in_bits/word_size_in_bits;
	qm_reg_size.dqf.write_to_full_error_intr_mask    = 32/byte_size_in_bits/word_size_in_bits;
	qm_reg_size.dqf.read_from_empty_error_intr_cause = 32/byte_size_in_bits/word_size_in_bits;
	qm_reg_size.dqf.read_from_empty_error_intr_mask  = 32/byte_size_in_bits/word_size_in_bits;
	qm_reg_size.dqf.wrong_axi_rd_error_intr_cause    = 32/byte_size_in_bits/word_size_in_bits;
	qm_reg_size.dqf.wrong_axi_rd_error_intr_mask     = 32/byte_size_in_bits/word_size_in_bits;
	qm_reg_size.dqf.mg2mem_req_addr_ctrl             = 32/byte_size_in_bits/word_size_in_bits;
	qm_reg_size.dqf.mem2mg_resp_status               = 32/byte_size_in_bits/word_size_in_bits;
	qm_reg_size.dqf.mem2mg_resp_data_hh              = 32/byte_size_in_bits/word_size_in_bits;
	qm_reg_size.dqf.mem2mg_resp_data_hl              = 32/byte_size_in_bits/word_size_in_bits;
	qm_reg_size.dqf.mem2mg_resp_data_lh              = 32/byte_size_in_bits/word_size_in_bits;
	qm_reg_size.dqf.mem2mg_resp_data_ll              = 32/byte_size_in_bits/word_size_in_bits;
	qm_reg_size.dqf.dqf_macs_l3_res                  = 32/byte_size_in_bits/word_size_in_bits;
	qm_reg_size.dqf.dqf_macs_l4_res                  = 32/byte_size_in_bits/word_size_in_bits;
	qm_reg_size.dqf.dqf_macs_l3_ptr                  = 32/byte_size_in_bits/word_size_in_bits;
	qm_reg_size.dqf.dqf_macs_l4_ptr                  = 32/byte_size_in_bits/word_size_in_bits;
	qm_reg_size.dqf.dqf_macs_desc                    = 64/byte_size_in_bits/word_size_in_bits;

	/* DMA registers addresses */
	qm_reg_size.dma.Q_memory_allocation                =  32/byte_size_in_bits/word_size_in_bits;
	qm_reg_size.dma.gpm_thresholds                     =  32/byte_size_in_bits/word_size_in_bits;
	qm_reg_size.dma.dram_thresholds                    =  32/byte_size_in_bits/word_size_in_bits;
	qm_reg_size.dma.AXI_write_attributes_for_swf_mode  =  32/byte_size_in_bits/word_size_in_bits;
	qm_reg_size.dma.AXI_write_attributes_for_rdma_mode =  32/byte_size_in_bits/word_size_in_bits;
	qm_reg_size.dma.AXI_write_attributes_for_hwf_qece  =  32/byte_size_in_bits/word_size_in_bits;
	qm_reg_size.dma.AXI_write_attributes_for_hwf_pyld  =  32/byte_size_in_bits/word_size_in_bits;
	qm_reg_size.dma.DRAM_VMID                          =  32/byte_size_in_bits/word_size_in_bits;
	qm_reg_size.dma.idle_status                        =  32/byte_size_in_bits/word_size_in_bits;
	qm_reg_size.dma.ecc_error_cause                    =  32/byte_size_in_bits/word_size_in_bits;
	qm_reg_size.dma.ecc_error_mask                     =  32/byte_size_in_bits/word_size_in_bits;
	qm_reg_size.dma.internal_error_cause               =  32/byte_size_in_bits/word_size_in_bits;
	qm_reg_size.dma.internal_error_mask                =  32/byte_size_in_bits/word_size_in_bits;
	qm_reg_size.dma.ceram_mac                          =  96/byte_size_in_bits/word_size_in_bits;
	qm_reg_size.dma.ceram_ppe                          = 160/byte_size_in_bits/word_size_in_bits;
	qm_reg_size.dma.qeram                              =  32/byte_size_in_bits/word_size_in_bits;
	qm_reg_size.dma.dram_fifo                          = 160/byte_size_in_bits/word_size_in_bits;

	/* SCHED registers addresses */
	qm_reg_size.sched.ErrStus			               = 64/byte_size_in_bits/word_size_in_bits;

	/* DROP registers addresses */
	qm_reg_size.drop.DrpErrStus                            = 64/byte_size_in_bits/word_size_in_bits;
	qm_reg_size.drop.DrpFirstExc                           = 64/byte_size_in_bits/word_size_in_bits;
	qm_reg_size.drop.DrpErrCnt                             = 64/byte_size_in_bits/word_size_in_bits;
	qm_reg_size.drop.DrpExcCnt                             = 64/byte_size_in_bits/word_size_in_bits;
	qm_reg_size.drop.DrpExcMask                            = 64/byte_size_in_bits/word_size_in_bits;
	qm_reg_size.drop.DrpId                                 = 64/byte_size_in_bits/word_size_in_bits;
	qm_reg_size.drop.DrpForceErr                           = 64/byte_size_in_bits/word_size_in_bits;
	qm_reg_size.drop.WREDDropProbMode                      = 64/byte_size_in_bits/word_size_in_bits;
	qm_reg_size.drop.WREDMaxProbModePerColor               = 64/byte_size_in_bits/word_size_in_bits;
	qm_reg_size.drop.DPSource                              = 64/byte_size_in_bits/word_size_in_bits;
	qm_reg_size.drop.RespLocalDPSel                        = 64/byte_size_in_bits/word_size_in_bits;
	qm_reg_size.drop.Drp_Decision_to_Query_debug           = 64/byte_size_in_bits/word_size_in_bits;
	qm_reg_size.drop.Drp_Decision_hierarchy_to_Query_debug = 64/byte_size_in_bits/word_size_in_bits;
	qm_reg_size.drop.TMtoTMPktGenQuantum                   = 64/byte_size_in_bits/word_size_in_bits;
	qm_reg_size.drop.TMtoTMDPCoSSel                        = 64/byte_size_in_bits/word_size_in_bits;
	qm_reg_size.drop.AgingUpdEnable                        = 64/byte_size_in_bits/word_size_in_bits;
	qm_reg_size.drop.PortInstAndAvgQueueLength             = 64/byte_size_in_bits/word_size_in_bits;
	qm_reg_size.drop.DrpEccConfig                          = 64/byte_size_in_bits/word_size_in_bits;
	qm_reg_size.drop.DrpEccMemParams                       = 64/byte_size_in_bits/word_size_in_bits;

	/* REORDER registers addresses */
	qm_reg_size.reorder.ru_qe              = 256/byte_size_in_bits/word_size_in_bits;
	qm_reg_size.reorder.ru_class           =  32/byte_size_in_bits/word_size_in_bits;
	qm_reg_size.reorder.ru_tasks           =  32/byte_size_in_bits/word_size_in_bits;
	qm_reg_size.reorder.ru_ptr2next        =  32/byte_size_in_bits/word_size_in_bits;
	qm_reg_size.reorder.ru_sid_fifo        =  32/byte_size_in_bits/word_size_in_bits;
	qm_reg_size.reorder.ru_port2class      =  32/byte_size_in_bits/word_size_in_bits;
	qm_reg_size.reorder.ru_pool            =  32/byte_size_in_bits/word_size_in_bits;
	qm_reg_size.reorder.ru_class_head      =  32/byte_size_in_bits/word_size_in_bits;
	qm_reg_size.reorder.ru_ser_error_cause =  32/byte_size_in_bits/word_size_in_bits;
	qm_reg_size.reorder.ru_ser_error_mask  =  32/byte_size_in_bits/word_size_in_bits;
	qm_reg_size.reorder.ru_host_cmd        =  32/byte_size_in_bits/word_size_in_bits;
	qm_reg_size.reorder.ru_task_permission =  32/byte_size_in_bits/word_size_in_bits;
}

void qm_reg_offset_alias_init(void)
{

	/*memset(qm_reg_offset,0,sizeof(*qm_reg_offset));*/
	/* QL registers sizes */
	qm_reg_offset.ql.qptr                 =  4;
	qm_reg_offset.ql.qmap_port            =  4;
	qm_reg_offset.ql.qmap_group           =  4;
	qm_reg_offset.ql.rule_bpi             =  8;
	qm_reg_offset.ql.low_threshold        = 16;
	qm_reg_offset.ql.pause_threshold      = 16;
	qm_reg_offset.ql.high_threshold       = 16;
	qm_reg_offset.ql.traffic_source       = 16;
	qm_reg_offset.ql.ECC_error_cause      =  4;
	qm_reg_offset.ql.ECC_error_mask       =  4;
	qm_reg_offset.ql.Internal_error_cause =  4;
	qm_reg_offset.ql.internal_error_mask  =  4;
	qm_reg_offset.ql.nss_general_purpose  =  4;
	qm_reg_offset.ql.qlen                 =  4;
	qm_reg_offset.ql.bp_qlen              =  4;
	qm_reg_offset.ql.drop_qlen            =  4;

	/* PFE registers sizes */
	qm_reg_offset.pfe.qece_dram_base_address_hi         =   4;
	qm_reg_offset.pfe.pyld_dram_base_address_hi         =   4;
	qm_reg_offset.pfe.qece_dram_base_address_lo         =   4;
	qm_reg_offset.pfe.pyld_dram_base_address_lo         =   4;
	qm_reg_offset.pfe.QM_VMID                           =   4;
	qm_reg_offset.pfe.port_flush                        =   4;
	qm_reg_offset.pfe.AXI_read_attributes_for_swf_mode  =   4;
	qm_reg_offset.pfe.AXI_read_attributes_for_rdma_mode =   4;
	qm_reg_offset.pfe.AXI_read_attributes_for_hwf_qece  =   4;
	qm_reg_offset.pfe.AXI_read_attributes_for_hwf_pyld  =   4;
	qm_reg_offset.pfe.ecc_error_cause                   =   4;
	qm_reg_offset.pfe.ecc_error_mask                    =   4;
	qm_reg_offset.pfe.internal_error_cause              =   4;
	qm_reg_offset.pfe.internal_error_mask               =   4;
	qm_reg_offset.pfe.idle_status                       =   4;
	qm_reg_offset.pfe.queue_flush                       =   4;
	qm_reg_offset.pfe.queue_qece                        =  16;

	/* DQF registers sizes */
	qm_reg_offset.dqf.Data_FIFO_params_p               = 4;
	qm_reg_offset.dqf.Credit_Threshold_p	           = 4;
	qm_reg_offset.dqf.PPC_port_map_p                   = 4;
	qm_reg_offset.dqf.data_fifo_pointers_p             = 4;
	qm_reg_offset.dqf.dqf_itnr_cause                   = 4;
	qm_reg_offset.dqf.dqf_itnr_mask                    = 4;
	qm_reg_offset.dqf.misc_error_intr_cause            = 4;
	qm_reg_offset.dqf.misc_error_intr_mask             = 4;
	qm_reg_offset.dqf.dqf_ser_summary_intr_cause       = 4;
	qm_reg_offset.dqf.dqf_ser_summary_intr_mask        = 4;
	qm_reg_offset.dqf.write_to_full_error_intr_cause   = 4;
	qm_reg_offset.dqf.write_to_full_error_intr_mask    = 4;
	qm_reg_offset.dqf.read_from_empty_error_intr_cause = 4;
	qm_reg_offset.dqf.read_from_empty_error_intr_mask  = 4;
	qm_reg_offset.dqf.wrong_axi_rd_error_intr_cause    = 4;
	qm_reg_offset.dqf.wrong_axi_rd_error_intr_mask     = 4;
	qm_reg_offset.dqf.mg2mem_req_addr_ctrl             = 4;
	qm_reg_offset.dqf.mem2mg_resp_status               = 4;
	qm_reg_offset.dqf.mem2mg_resp_data_hh              = 4;
	qm_reg_offset.dqf.mem2mg_resp_data_hl              = 4;
	qm_reg_offset.dqf.mem2mg_resp_data_lh              = 4;
	qm_reg_offset.dqf.mem2mg_resp_data_ll              = 4;
	qm_reg_offset.dqf.dqf_macs_l3_res                  = 4;
	qm_reg_offset.dqf.dqf_macs_l4_res                  = 4;
	qm_reg_offset.dqf.dqf_macs_l3_ptr                  = 4;
	qm_reg_offset.dqf.dqf_macs_l4_ptr                  = 4;
	qm_reg_offset.dqf.dqf_macs_desc                    = 4;

	/* DMA registers addresses */
	qm_reg_offset.dma.Q_memory_allocation                =  4;
	qm_reg_offset.dma.gpm_thresholds                     =  4;
	qm_reg_offset.dma.dram_thresholds                    =  4;
	qm_reg_offset.dma.AXI_write_attributes_for_swf_mode  =  4;
	qm_reg_offset.dma.AXI_write_attributes_for_rdma_mode =  4;
	qm_reg_offset.dma.AXI_write_attributes_for_hwf_qece  =  4;
	qm_reg_offset.dma.AXI_write_attributes_for_hwf_pyld  =  4;
	qm_reg_offset.dma.DRAM_VMID                          =  4;
	qm_reg_offset.dma.idle_status                        =  4;
	qm_reg_offset.dma.ecc_error_cause                    =  4;
	qm_reg_offset.dma.ecc_error_mask                     =  4;
	qm_reg_offset.dma.internal_error_cause               =  4;
	qm_reg_offset.dma.internal_error_mask                =  4;
	qm_reg_offset.dma.ceram_mac                          = 16;
	qm_reg_offset.dma.ceram_ppe                          = 32;
	qm_reg_offset.dma.qeram                              =  4;
/*#warning MY_WARNING: Register qm_reg_offset.dma.dram_fifo is not defined in CIDER properly.*/
	qm_reg_offset.dma.dram_fifo                          = 32;

	/* SCHED registers addresses */
	qm_reg_offset.sched.ErrStus                = 8;

	/* DROP registers addresses */
	qm_reg_offset.drop.DrpErrStus                            = 8;
	qm_reg_offset.drop.DrpFirstExc                           = 8;
	qm_reg_offset.drop.DrpErrCnt                             = 8;
	qm_reg_offset.drop.DrpExcCnt                             = 8;
	qm_reg_offset.drop.DrpExcMask                            = 8;
	qm_reg_offset.drop.DrpId                                 = 8;
	qm_reg_offset.drop.DrpForceErr                           = 8;
	qm_reg_offset.drop.WREDDropProbMode                      = 8;
	qm_reg_offset.drop.WREDMaxProbModePerColor               = 8;
	qm_reg_offset.drop.DPSource                              = 8;
	qm_reg_offset.drop.RespLocalDPSel                        = 8;
	qm_reg_offset.drop.Drp_Decision_to_Query_debug           = 8;

	qm_reg_offset.drop.Drp_Decision_hierarchy_to_Query_debug = 8;
	qm_reg_offset.drop.TMtoTMPktGenQuantum                   = 8;
	qm_reg_offset.drop.TMtoTMDPCoSSel                        = 8;
	qm_reg_offset.drop.AgingUpdEnable                        = 8;
	qm_reg_offset.drop.PortInstAndAvgQueueLength             = 8;
	qm_reg_offset.drop.DrpEccConfig                          = 8;
	qm_reg_offset.drop.DrpEccMemParams                       = 8;

	/* REORDER registers addresses */
	qm_reg_offset.reorder.ru_qe              = 32;
	qm_reg_offset.reorder.ru_class           =  4;
	qm_reg_offset.reorder.ru_tasks           =  4;
	qm_reg_offset.reorder.ru_ptr2next        =  4;
	qm_reg_offset.reorder.ru_sid_fifo        =  4;
	qm_reg_offset.reorder.ru_port2class      =  4;
	qm_reg_offset.reorder.ru_pool            =  4;
	qm_reg_offset.reorder.ru_class_head      =  4;
	qm_reg_offset.reorder.ru_ser_error_cause =  4;
	qm_reg_offset.reorder.ru_ser_error_mask  =  4;
	qm_reg_offset.reorder.ru_host_cmd        =  8;
	qm_reg_offset.reorder.ru_task_permission =  8;
}


#ifdef MY_HIDE
#endif /* MY_HIDE */
