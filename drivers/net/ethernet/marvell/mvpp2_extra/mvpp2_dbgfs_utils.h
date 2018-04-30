/*********************************************************************
 * DebugFS for Marvell PPv2 network controller for Armada 7k/8k SoC.
 *
 * Copyright (C) 2018 Marvell
 *
 * Yan Markman <ymarkman@marvell.com>
 * This file is licensed under the terms of the GNU General Public
 * License version 2. This program is licensed "as is" without any
 * warranty of any kind, whether express or implied.
 */

/*******************  @CLS  ***************************************************
 * { "cls_lkp_hw_dump"	, ARG_RO, { 0 }, mvpp2_cls_hw_lkp_dump      , "CLS  lookup ID table from HW" },
 * { "cls_lkp_hw_hits"	, ARG_RO, { 0 }, mvpp2_cls_hw_lkp_hits_dump , "CLS  nonZero hits lookup ID entires" },
 * { "cls_flow_hw_dump"	, ARG_RO, { 0 }, mvpp2_cls_hw_flow_dump     , "CLS  flow table from HW" },
 * { "cls_flow_hw_hits"	, ARG_RO, { 0 }, mvpp2_cls_hw_flow_hits_dump, "CLS  nonZero hits flow tab entries" },
 * { "cls_hw_regs"	, ARG_RO, { 0 }, mvpp2_cls_hw_regs_dump     , "CLS  classifier top registers" },
 */

static void mvpp2_cls_hw_lkp_read(struct mvpp2 *priv, int lkpid, int way,
				  struct mvpp2_cls_lookup_entry *fe)
{
	unsigned int reg_val = 0;

	/* write index reg */
	reg_val = (way << MVPP2_CLS_LKP_INDEX_WAY_OFFS) |
		(lkpid << MVPP2_CLS_LKP_INDEX_LKP_OFFS);
	mvpp2_write(priv, MVPP2_CLS_LKP_INDEX_REG, reg_val);

	fe->way = way;
	fe->lkpid = lkpid;
	fe->data = mvpp2_read(priv, MVPP2_CLS_LKP_TBL_REG);
}

static void mvpp2_cls_sw_lkp_rxq_get(struct mvpp2_cls_lookup_entry *lkp,
				     int *rxq)
{
	*rxq =  (lkp->data & MVPP2_FLOWID_RXQ_MASK) >> MVPP2_FLOWID_RXQ;
}

static void mvpp2_cls_sw_lkp_en_get(struct mvpp2_cls_lookup_entry *lkp,
				    int *en)
{
	*en = (lkp->data & MVPP2_FLOWID_EN_MASK) >> MVPP2_FLOWID_EN;
}

static void mvpp2_cls_sw_lkp_flow_get(struct mvpp2_cls_lookup_entry *lkp,
				      int *flow_idx)
{
	*flow_idx = (lkp->data & MVPP2_FLOWID_FLOW_MASK) >> MVPP2_FLOWID_FLOW;
}

static void mvpp2_cls_sw_lkp_mod_get(struct mvpp2_cls_lookup_entry *lkp,
				     int *mod_base)
{
	*mod_base = (lkp->data & MVPP2_FLOWID_MODE_MASK) >> MVPP2_FLOWID_MODE;
}

static void mvpp2_cls_hw_lkp_hit_get(struct mvpp2 *priv, int lkpid, int way,
				     unsigned int *cnt)
{
	/*set index */
	mvpp2_write(priv, MVPP2_CNT_IDX_REG, MVPP2_CNT_IDX_LKP(lkpid, way));
	*cnt = mvpp2_read(priv, MVPP2_CLS_LKP_TBL_HIT_REG);
}

static void mvpp2_cls_sw_flow_hek_get(struct mvpp2_cls_flow_entry *fe,
				      int *num_of_fields, int field_ids[])
{
	int index;

	*num_of_fields = (fe->data[1] & MVPP2_FLOW_FIELDS_NUM_MASK) >>
						MVPP2_FLOW_FIELDS_NUM;

	for (index = 0; index < (*num_of_fields); index++)
		field_ids[index] = (fe->data[2] & MVPP2_FLOW_FIELD_MASK(index)) >>
						MVPP2_FLOW_FIELD_ID(index);
}

static void mvpp2_cls_sw_flow_port_get(struct mvpp2_cls_flow_entry *fe,
				       int *type, int *portid)
{
	*type = (fe->data[0] & MVPP2_FLOW_PORT_TYPE_MASK)
				>> MVPP2_FLOW_PORT_TYPE;
	*portid = (fe->data[0] & MVPP2_FLOW_PORT_ID_MASK)
				>> MVPP2_FLOW_PORT_ID;
}

static void mvpp2_cls_sw_flow_engine_get(struct mvpp2_cls_flow_entry *fe,
					 int *engine, int *is_last)
{
	*engine = (fe->data[0] & MVPP2_FLOW_ENGINE_MASK)
				>> MVPP2_FLOW_ENGINE;
	*is_last = fe->data[0] & MVPP2_FLOW_LAST_MASK;
}

static void mvpp2_cls_sw_flow_extra_get(struct mvpp2_cls_flow_entry *fe,
					int *type, int *prio)
{
	*type = (fe->data[1] & MVPP2_FLOW_LKP_TYPE_MASK)
				>> MVPP2_FLOW_LKP_TYPE;
	*prio = (fe->data[1] & MVPP2_FLOW_FIELD_PRIO_MASK)
				>> MVPP2_FLOW_FIELD_PRIO;
}

static void mvpp2_cls_hw_flow_hit_get(struct mvpp2 *priv,
				      int index,  unsigned int *cnt)
{
	mvpp2_write(priv, MVPP2_CNT_IDX_REG, MVPP2_CNT_IDX_FLOW(index));
	*cnt = mvpp2_read(priv, MVPP2_CLS_FLOW_TBL_HIT_REG);
}

static void mvpp2_cls_hw_flow_read(struct mvpp2 *priv, int index,
				   struct mvpp2_cls_flow_entry *fe)
{
	mvpp2_write(priv, MVPP2_CLS_FLOW_INDEX_REG, index);
	fe->index = index;
	fe->data[0] = mvpp2_read(priv, MVPP2_CLS_FLOW_TBL0_REG);
	fe->data[1] = mvpp2_read(priv, MVPP2_CLS_FLOW_TBL1_REG);
	fe->data[2] = mvpp2_read(priv, MVPP2_CLS_FLOW_TBL2_REG);
}

static int mvpp2_cls_hw_lkp_dump(struct seq_file *m, void *v)
{
	struct mvpp2 *priv = mvpp2_global_config.hw;
	int index, way, int32bit, ind;
	unsigned int uint32bit;

	struct mvpp2_cls_lookup_entry lkp;

	seq_puts(m, "< ID  WAY >:	RXQ	EN	FLOW	MODE_BASE  HITS\n");
	for (index = 0; index < MVPP2_CLS_LKP_TBL_SIZE ; index++)
		for (way = 0; way < 2 ; way++)	{
			ind = (way << MVPP2_CLS_LKP_INDEX_WAY_OFFS) | index;
			mvpp2_cls_hw_lkp_read(priv, index, way, &lkp);
			seq_printf(m, " 0x%2.2x  %1.1d\t", lkp.lkpid, lkp.way);
			mvpp2_cls_sw_lkp_rxq_get(&lkp, &int32bit);
			seq_printf(m, "0x%2.2x\t", int32bit);
			mvpp2_cls_sw_lkp_en_get(&lkp, &int32bit);
			seq_printf(m, "%1.1d\t", int32bit);
			mvpp2_cls_sw_lkp_flow_get(&lkp, &int32bit);
			seq_printf(m, "0x%3.3x\t", int32bit);
			mvpp2_cls_sw_lkp_mod_get(&lkp, &int32bit);
			seq_printf(m, " 0x%2.2x\t", int32bit);
			mvpp2_cls_hw_lkp_hit_get(priv, index, way, &uint32bit);
			seq_printf(m, " 0x%8.8x\n", uint32bit);
		}
	return 0;
}

static int mvpp2_cls_hw_lkp_hits_dump(struct seq_file *m, void *v)
{
	struct mvpp2 *priv = mvpp2_global_config.hw;
	int index, way, entry_ind;
	unsigned int cnt;

	seq_puts(m, "< ID  WAY >:	HITS\n");
	for (index = 0; index < MVPP2_CLS_LKP_TBL_SIZE ; index++) {
		for (way = 0; way < 2 ; way++)	{
			entry_ind = (way << MVPP2_CLS_LKP_INDEX_WAY_OFFS) | index;
			mvpp2_cls_hw_lkp_hit_get(priv, index, way,  &cnt);
			if (!cnt)
				continue;
			seq_printf(m, " 0x%2.2x  %1.1d\t0x%8.8x\n",
				   index, way, cnt);
		}
	}
	return 0;
}

static int mvpp2_cls_sw_flow_dump(struct seq_file *m,
				  struct mvpp2_cls_flow_entry *fe)
{
	int	int32bit_1, int32bit_2, i;
	int	fields_arr[MVPP2_CLS_FLOWS_TBL_FIELDS_MAX];

	seq_puts(m, "INDEX: F[0] F[1] F[2] F[3] PRT[T  ID] ENG LAST LKP_TYP  PRIO\n");
	/*index*/
	seq_printf(m, "0x%3.3x  ", fe->index);

	/*filed[0] filed[1] filed[2] filed[3]*/
	mvpp2_cls_sw_flow_hek_get(fe, &int32bit_1, fields_arr);

	for (i = 0 ; i < MVPP2_CLS_FLOWS_TBL_FIELDS_MAX; i++)
		if (i < int32bit_1)
			seq_printf(m, "0x%2.2x ", fields_arr[i]);
		else
			seq_puts(m, " NA  ");

	/*port_type port_id*/
	mvpp2_cls_sw_flow_port_get(fe, &int32bit_1, &int32bit_2);
	seq_printf(m, "[%1d  0x%3.3x]  ", int32bit_1, int32bit_2);
	/* engine_num last_bit*/
	mvpp2_cls_sw_flow_engine_get(fe, &int32bit_1, &int32bit_2);
	seq_printf(m, "%1d   %1d    ", int32bit_1, int32bit_2);
	/* lookup_type priority*/
	mvpp2_cls_sw_flow_extra_get(fe, &int32bit_1, &int32bit_2);
	seq_printf(m, "0x%2.2x    0x%2.2x", int32bit_1, int32bit_2);
	seq_puts(m, "\n");

	seq_puts(m, "       PPPEO   VLAN   MACME   UDF7   SELECT SEQ_CTRL\n");
	seq_printf(m, "         %1u      %1u      %1u       %1u      %1lu      %1u\n",
		   (fe->data[0] & MVPP2_FLOW_PPPOE_MASK) >> MVPP2_FLOW_PPPOE,
		   (fe->data[0] & MVPP2_FLOW_VLAN_MASK) >> MVPP2_FLOW_VLAN,
		   (fe->data[0] & MVPP2_FLOW_MACME_MASK) >> MVPP2_FLOW_MACME,
		   (fe->data[0] & MVPP2_FLOW_UDF7_MASK) >> MVPP2_FLOW_UDF7,
		   (fe->data[0] & MVPP2_FLOW_PORT_ID_SEL_MASK) >>
		   MVPP2_FLOW_PORT_ID_SEL,
		   (fe->data[1] & MVPP2_FLOW_SEQ_CTRL_MASK) >>
		   MVPP2_FLOW_SEQ_CTRL);
	return 0;
}

static int mvpp2_cls_hw_flow_dump(struct seq_file *m, void *v)
{
	struct mvpp2 *priv = mvpp2_global_config.hw;
	int index;
	unsigned int cnt;
	struct mvpp2_cls_flow_entry fe;

	for (index = 0; index < MVPP2_CLS_FLOWS_TBL_SIZE ; index++) {
		mvpp2_cls_hw_flow_read(priv, index, &fe);
		mvpp2_cls_sw_flow_dump(m, &fe);
		mvpp2_cls_hw_flow_hit_get(priv, index, &cnt);
		seq_printf(m, "HITS = %d\n", cnt);
		seq_puts(m, "-------------------------------------------------\n");
	}
	return 0;
}

static int mvpp2_cls_hw_flow_hits_dump(struct seq_file *m, void *v)
{
	struct mvpp2 *priv = mvpp2_global_config.hw;
	int index;
	unsigned int cnt;
	struct mvpp2_cls_flow_entry fe;

	for (index = 0; index < MVPP2_CLS_FLOWS_TBL_SIZE ; index++) {
		mvpp2_cls_hw_flow_hit_get(priv, index, &cnt);
		if (!cnt)
			continue;
		mvpp2_cls_hw_flow_read(priv, index, &fe);
		mvpp2_cls_sw_flow_dump(m, &fe);
		seq_printf(m, "HITS = %d\n", cnt);
	}
	return 0;
}

int mvpp2_cls_hw_regs_dump(struct seq_file *m, void *v)
{
	struct mvpp2 *priv = mvpp2_global_config.hw;
	int i = 0;
	char name[100];

	mvpp2_prt_reg(m, priv,
		      MVPP2_CLS_MODE_REG, "MVPP2_CLS_MODE_REG");
	mvpp2_prt_reg(m, priv,
		      MVPP2_CLS_PORT_WAY_REG, "MVPP2_CLS_PORT_WAY_REG");
	mvpp2_prt_reg(m, priv,
		      MVPP2_CLS_LKP_INDEX_REG, "MVPP2_CLS_LKP_INDEX_REG");
	mvpp2_prt_reg(m, priv,
		      MVPP2_CLS_LKP_TBL_REG, "MVPP2_CLS_LKP_TBL_REG");
	mvpp2_prt_reg(m, priv,
		      MVPP2_CLS_FLOW_INDEX_REG, "MVPP2_CLS_FLOW_INDEX_REG");
	mvpp2_prt_reg(m, priv,
		      MVPP2_CLS_FLOW_TBL0_REG, "MVPP2_CLS_FLOW_TBL0_REG");
	mvpp2_prt_reg(m, priv,
		      MVPP2_CLS_FLOW_TBL1_REG, "MVPP2_CLS_FLOW_TBL1_REG");
	mvpp2_prt_reg(m, priv,
		      MVPP2_CLS_FLOW_TBL2_REG, "MVPP2_CLS_FLOW_TBL2_REG");
	mvpp2_prt_reg(m, priv,
		      MVPP2_CLS_PORT_SPID_REG, "MVPP2_CLS_PORT_SPID_REG");

	for (i = 0; i < MVPP2_CLS_SPID_UNI_REGS; i++) {
		sprintf(name, "MVPP2_CLS_SPID_UNI_%d_REG", i);
		mvpp2_prt_reg(m, priv,
			      (MVPP2_CLS_SPID_UNI_BASE_REG + (4 * i)),
			      name);
	}
	for (i = 0; i < MVPP2_CLS_GEM_VIRT_REGS_NUM; i++) {
		/* indirect access */
		mvpp2_write(priv, MVPP2_CLS_GEM_VIRT_INDEX_REG, i);
		sprintf(name, "MVPP2_CLS_GEM_VIRT_%d_REG", i);
		mvpp2_prt_reg(m, priv, MVPP2_CLS_GEM_VIRT_REG, name);
	}
	for (i = 0; i < MVPP2_CLS_UDF_BASE_REGS; i++)	{
		sprintf(name, "MVPP2_CLS_UDF_REG_%d_REG", i);
		mvpp2_prt_reg(m, priv, MVPP2_CLS_UDF_REG(i), name);
	}
	for (i = 0; i < 16; i++) {
		sprintf(name, "MVPP2_CLS_MTU_%d_REG", i);
		mvpp2_prt_reg(m, priv, MVPP2_CLS_MTU_REG(i), name);
	}
	for (i = 0; i < MVPP2_MAX_PORTS; i++) {
		sprintf(name, "MVPP2_CLS_OVER_RXQ_LOW_%d_REG", i);
		mvpp2_prt_reg(m, priv, MVPP2_CLS_OVERSIZE_RXQ_LOW_REG(i), name);
	}
	for (i = 0; i < MVPP2_MAX_PORTS; i++) {
		sprintf(name, "MVPP2_CLS_SWFWD_P2HQ_%d_REG", i);
		mvpp2_prt_reg(m, priv, MVPP2_CLS_SWFWD_P2HQ_REG(i), name);
	}

	mvpp2_prt_reg(m, priv,
		      MVPP2_CLS_SWFWD_PCTRL_REG, "MVPP2_CLS_SWFWD_PCTRL_REG");
	mvpp2_prt_reg(m, priv,
		      MVPP2_CLS_SEQ_SIZE_REG, "MVPP2_CLS_SEQ_SIZE_REG");

	for (i = 0; i < MVPP2_MAX_PORTS; i++) {
		sprintf(name, "MVPP2_CLS_PCTRL_%d_REG", i);
		mvpp2_prt_reg(m, priv, MVPP2_CLS_PCTRL_REG(i), name);
	}
	return 0;
}

/********************  @CLS2 (C2)  ********************************************
 * { "cls2_act_hw_dump"	, ARG_RO, { 0 }, mvpp2_cls2_hw_dump         , "CLS2 action table enrties" },
 * { "cls2_dscp_hw_dump", ARG_RO, { 0 }, mvpp2_cls2_qos_dscp_hw_dump, "CLS2 QoS dscp tables" },
 * { "cls2_prio_hw_dump", ARG_RO, { 0 }, mvpp2_cl2_qos_prio_hw_dump , "CLS2 QoS priority tables" },
 * { "cls2_cnt_dump"	, ARG_RO, { 0 }, mvpp2_cls2_hit_cntr_dump   , "CLS2 hit counters" },
 * { "cls2_hw_regs"	, ARG_RO, { 0 }, mvpp2_cls2_regs_dump       , "CLS2 C2-classifier registers" },
 */
static void mvpp2_cls_c2_hw_read(struct mvpp2 *priv, int index,
				 struct mvpp2_cls_c2_entry *c2)
{
	unsigned int reg_val;
	int	tcm_idx;

	c2->index = index;
	/* write index reg */
	mvpp2_write(priv, MVPP2_CLS2_TCAM_IDX_REG, index);
	/* read inValid bit*/
	reg_val = mvpp2_read(priv, MVPP2_CLS2_TCAM_INV_REG);
	c2->inv = (reg_val & MVPP2_CLS2_TCAM_INV_INVALID_MASK) >>
			MVPP2_CLS2_TCAM_INV_INVALID_OFF;
	if (c2->inv)
		return;

	for (tcm_idx = 0; tcm_idx < MVPP2_CLS_C2_TCAM_WORDS; tcm_idx++)
		c2->tcam.words[tcm_idx] = mvpp2_read(priv, MVPP2_CLS2_TCAM_DATA_REG(tcm_idx));

	/* read action_tbl 0x1B30 */
	c2->sram.regs.action_tbl = mvpp2_read(priv, MVPP2_CLS2_ACT_DATA_REG);
	/* read actions 0x1B60 */
	c2->sram.regs.actions = mvpp2_read(priv, MVPP2_CLS2_ACT_REG);
	/* read qos_attr 0x1B64 */
	c2->sram.regs.qos_attr = mvpp2_read(priv, MVPP2_CLS2_ACT_QOS_ATTR_REG);
	/* read hwf_attr 0x1B68 */
	c2->sram.regs.hwf_attr = mvpp2_read(priv, MVPP2_CLS2_ACT_HWF_ATTR_REG);
	/* read hwf_attr 0x1B6C */
	c2->sram.regs.rss_attr = mvpp2_read(priv, MVPP2_CLS2_ACT_DUP_ATTR_REG);
	/* read seq_attr 0x1B70 */
	c2->sram.regs.seq_attr = mvpp2_read(priv, MVPP2_CLS2_ACT_SEQ_ATTR_REG);
}

static void mvpp2_cls_c2_hit_cntr_read(struct mvpp2 *priv, int index, u32 *cntr)
{
	unsigned int value = 0;

	/* write index reg */
	mvpp2_write(priv, MVPP2_CLS2_TCAM_IDX_REG, index);
	value = mvpp2_read(priv, MVPP2_CLS2_HIT_CTR_REG);
	*cntr = value;
}

static void mvpp2_cls_c2_qos_hw_read(struct mvpp2 *priv, int tbl_id,
				     int tbl_sel, int tbl_line,
				     struct mvpp2_cls_c2_qos_entry *qos)
{
	unsigned int reg_val = 0;

	qos->tbl_id = tbl_id;
	qos->tbl_sel = tbl_sel;
	qos->tbl_line = tbl_line;
	/* write index reg */
	reg_val |= (tbl_line << MVPP2_CLS2_DSCP_PRI_INDEX_LINE_OFF);
	reg_val |= (tbl_sel << MVPP2_CLS2_DSCP_PRI_INDEX_SEL_OFF);
	reg_val |= (tbl_id << MVPP2_CLS2_DSCP_PRI_INDEX_TBL_ID_OFF);
	mvpp2_write(priv, MVPP2_CLS2_DSCP_PRI_INDEX_REG, reg_val);
	/* read data reg*/
	qos->data = mvpp2_read(priv, MVPP2_CLS2_QOS_TBL_REG);
}

static void mvpp2_cls_c2_qos_queue_get(struct mvpp2_cls_c2_qos_entry *qos,
				       int *queue)
{
	*queue = (qos->data & MVPP2_CLS2_QOS_TBL_QUEUENUM_MASK) >>
		MVPP2_CLS2_QOS_TBL_QUEUENUM_OFF;
}

static void mvpp2_cls_c2_qos_dscp_get(struct mvpp2_cls_c2_qos_entry *qos,
				      int *dscp)
{
	*dscp = (qos->data & MVPP2_CLS2_QOS_TBL_DSCP_MASK) >>
		MVPP2_CLS2_QOS_TBL_DSCP_OFF;
}

static void mvpp2_cls_c2_qos_gpid_get(struct mvpp2_cls_c2_qos_entry *qos,
				      int *gpid)
{
	*gpid = (qos->data & MVPP2_CLS2_QOS_TBL_GEMPORT_MASK) >>
		MVPP2_CLS2_QOS_TBL_GEMPORT_OFF;
}

static void mvpp2_cls_c2_qos_prio_get(struct mvpp2_cls_c2_qos_entry *qos,
				      int *prio)
{
	*prio = (qos->data & MVPP2_CLS2_QOS_TBL_PRI_MASK) >>
		MVPP2_CLS2_QOS_TBL_PRI_OFF;
}

static void mvpp2_cls_c2_qos_color_get(struct mvpp2_cls_c2_qos_entry *qos,
				       int *color)
{
	*color = (qos->data & MVPP2_CLS2_QOS_TBL_COLOR_MASK) >>
				MVPP2_CLS2_QOS_TBL_COLOR_OFF;
}

static void mvpp2_cls2_sw_words_dump(struct seq_file *m,
				     struct mvpp2_cls_c2_entry *c2)
{
	int i;

	/* hw entry id */
	seq_printf(m, "[0x%3.3x] ", c2->index);

	i = MVPP2_CLS_C2_TCAM_WORDS - 1;

	while (i >= 0)
		seq_printf(m, "%4.4x ", (c2->tcam.words[i--]) & 0xFFFF);

	seq_puts(m, "| ");
	seq_printf(m, "%8.8x %8.8x %8.8x %8.8x %8.8x", c2->sram.words[4],
		   c2->sram.words[3], c2->sram.words[2], c2->sram.words[1],
		   c2->sram.words[0]);
	/*tcam inValid bit*/
	seq_printf(m, " %s", (c2->inv == 1) ? "[inv]" : "[valid]");
	seq_puts(m, "\n        ");

	i = MVPP2_CLS_C2_TCAM_WORDS - 1;

	while (i >= 0)
		seq_printf(m, "%4.4x ",
			   ((c2->tcam.words[i--] >> 16)  & 0xFFFF));

	seq_puts(m, "\n");
}

static int mvpp2_cls2_sw_dump(struct seq_file *m, struct mvpp2_cls_c2_entry *c2)
{
	int id, sel, type, gemid, low_q, high_q, color, int32bit;

	mvpp2_cls2_sw_words_dump(m, c2);
	seq_puts(m, "\n");

	/*------------------------------*/
	/*	action_tbl 0x1B30	*/
	/*------------------------------*/

	id = (c2->sram.regs.action_tbl & MVPP2_CLS2_ACT_DATA_TBL_ID_MASK)
			>> MVPP2_CLS2_ACT_DATA_TBL_ID_OFF;
	sel = (c2->sram.regs.action_tbl & MVPP2_CLS2_ACT_DATA_TBL_SEL_MASK)
			>> MVPP2_CLS2_ACT_DATA_TBL_SEL_OFF;
	type = (c2->sram.regs.action_tbl & MVPP2_CLS2_ACT_DATA_TBL_PRI_DSCP_MASK)
			>> MVPP2_CLS2_ACT_DATA_TBL_PRI_DSCP_OFF;
	gemid = (c2->sram.regs.action_tbl & MVPP2_CLS2_ACT_DATA_TBL_GEM_ID_MASK)
			>> MVPP2_CLS2_ACT_DATA_TBL_GEM_ID_OFF;
	low_q = (c2->sram.regs.action_tbl & MVPP2_CLS2_ACT_DATA_TBL_LOW_Q_MASK)
			>> MVPP2_CLS2_ACT_DATA_TBL_LOW_Q_OFF;
	high_q = (c2->sram.regs.action_tbl & MVPP2_CLS2_ACT_DATA_TBL_HIGH_Q_MASK)
			>> MVPP2_CLS2_ACT_DATA_TBL_HIGH_Q_OFF;
	color = (c2->sram.regs.action_tbl & MVPP2_CLS2_ACT_DATA_TBL_COLOR_MASK)
			>> MVPP2_CLS2_ACT_DATA_TBL_COLOR_OFF;

	seq_printf(m, "FROM_QOS_%s_TBL[%2.2d]:  ", sel ? "DSCP" : "PRI", id);
	if (type)
		seq_printf(m, "%s	", sel ? "DSCP" : "PRIO");
	if (color)
		seq_puts(m, "COLOR	");
	if (gemid)
		seq_puts(m, "GEMID	");
	if (low_q)
		seq_puts(m, "LOW_Q	");
	if (high_q)
		seq_puts(m, "HIGH_Q	");
	seq_puts(m, "\n");

	seq_puts(m, "FROM_ACT_TBL:		");
	if (type == 0)
		seq_printf(m, "%s	", sel ? "DSCP" : "PRI");
	if (gemid == 0)
		seq_puts(m, "GEMID	");
	if (low_q == 0)
		seq_puts(m, "LOW_Q	");
	if (high_q == 0)
		seq_puts(m, "HIGH_Q	");
	if (color == 0)
		seq_puts(m, "COLOR	");
	seq_puts(m, "\n\n");

	/*------------------------------*/
	/*	actions 0x1B60		*/
	/*------------------------------*/

	seq_puts(m,
		 "ACT_CMD:	COLOR	PRIO	DSCP	GEMID	LOW_Q	HIGH_Q	FWD	POLICER	FID	RSS\n");
	seq_puts(m, "		");

	seq_printf(m,
		   "%1.1d\t%1.1d\t%1.1d\t%1.1d\t%1.1d\t%1.1d\t%1.1d\t%1.1d\t%1.1d\t%1.1d\t",
		   ((c2->sram.regs.actions &
			MVPP2_CLS2_ACT_COLOR_MASK) >>
			MVPP2_CLS2_ACT_COLOR_OFF),
		   ((c2->sram.regs.actions &
			MVPP2_CLS2_ACT_PRI_MASK) >>
			MVPP2_CLS2_ACT_PRI_OFF),
		   ((c2->sram.regs.actions &
			MVPP2_CLS2_ACT_DSCP_MASK) >>
			MVPP2_CLS2_ACT_DSCP_OFF),
		   ((c2->sram.regs.actions &
			MVPP2_CLS2_ACT_GEM_MASK) >>
			MVPP2_CLS2_ACT_GEM_OFF),
		   ((c2->sram.regs.actions &
			MVPP2_CLS2_ACT_QL_MASK) >>
			MVPP2_CLS2_ACT_QL_OFF),
		   ((c2->sram.regs.actions &
			MVPP2_CLS2_ACT_QH_MASK) >>
			MVPP2_CLS2_ACT_QH_OFF),
		   ((c2->sram.regs.actions &
			MVPP2_CLS2_ACT_FRWD_MASK) >>
			MVPP2_CLS2_ACT_FRWD_OFF),
		   ((c2->sram.regs.actions &
			MVPP2_CLS2_ACT_PLCR_MASK) >>
			MVPP2_CLS2_ACT_PLCR_OFF),
		   ((c2->sram.regs.actions &
			MVPP2_CLS2_ACT_FLD_EN_MASK) >>
			MVPP2_CLS2_ACT_FLD_EN_OFF),
		   ((c2->sram.regs.actions &
			MVPP2_CLS2_ACT_RSS_MASK) >>
			MVPP2_CLS2_ACT_RSS_OFF));
	seq_puts(m, "\n\n");

	/*------------------------------*/
	/*	qos_attr 0x1B64		*/
	/*------------------------------*/
	seq_puts(m,
		 "ACT_ATTR:		PRIO	DSCP	GEMID	LOW_Q	HIGH_Q	QUEUE\n");
	seq_puts(m, "		");
	/* modify priority */
	int32bit = ((c2->sram.regs.qos_attr &
		    MVPP2_CLS2_ACT_QOS_ATTR_PRI_MASK) >>
		    MVPP2_CLS2_ACT_QOS_ATTR_PRI_OFF);
	seq_printf(m, "	%1.1d\t", int32bit);

	/* modify dscp */
	int32bit = ((c2->sram.regs.qos_attr &
		    MVPP2_CLS2_ACT_QOS_ATTR_DSCP_MASK) >>
		    MVPP2_CLS2_ACT_QOS_ATTR_DSCP_OFF);
	seq_printf(m, "%2.2d\t", int32bit);

	/* modify gemportid */
	int32bit = ((c2->sram.regs.qos_attr &
		    MVPP2_CLS2_ACT_QOS_ATTR_GEM_MASK) >>
		    MVPP2_CLS2_ACT_QOS_ATTR_GEM_OFF);
	seq_printf(m, "0x%4.4x\t", int32bit);

	/* modify low Q */
	int32bit = ((c2->sram.regs.qos_attr &
		    MVPP2_CLS2_ACT_QOS_ATTR_QL_MASK) >>
		    MVPP2_CLS2_ACT_QOS_ATTR_QL_OFF);
	seq_printf(m, "%1.1d\t", int32bit);

	/* modify high Q */
	int32bit = ((c2->sram.regs.qos_attr &
		    MVPP2_CLS2_ACT_QOS_ATTR_QH_MASK) >>
		    MVPP2_CLS2_ACT_QOS_ATTR_QH_OFF);
	seq_printf(m, "0x%2.2x\t", int32bit);

	/*modify queue*/
	int32bit = ((c2->sram.regs.qos_attr &
		    (MVPP2_CLS2_ACT_QOS_ATTR_QL_MASK |
		    MVPP2_CLS2_ACT_QOS_ATTR_QH_MASK)));
	int32bit >>= MVPP2_CLS2_ACT_QOS_ATTR_QL_OFF;

	seq_printf(m, "0x%2.2x\t", int32bit);
	seq_puts(m, "\n\n");

	/*------------------------------*/
	/*	hwf_attr 0x1B68		*/
	/*------------------------------*/
	seq_puts(m, "HWF_ATTR:		IPTR	DPTR	CHKSM   MTU_IDX\n");
	seq_puts(m, "			");

	/* HWF modification instraction pointer */
	int32bit = ((c2->sram.regs.hwf_attr &
		    MVPP2_CLS2_ACT_HWF_ATTR_IPTR_MASK) >>
		    MVPP2_CLS2_ACT_HWF_ATTR_IPTR_OFF);
	seq_printf(m, "0x%1.1x\t", int32bit);

	/* HWF modification data pointer */
	int32bit = ((c2->sram.regs.hwf_attr &
		    MVPP2_CLS2_ACT_HWF_ATTR_DPTR_MASK) >>
		    MVPP2_CLS2_ACT_HWF_ATTR_DPTR_OFF);
	seq_printf(m, "0x%4.4x\t", int32bit);

	/* HWF modification instraction pointer */
	int32bit = ((c2->sram.regs.hwf_attr &
		    MVPP2_CLS2_ACT_HWF_ATTR_L4CHK_MASK) >>
		    MVPP2_CLS2_ACT_HWF_ATTR_L4CHK_OFF);
	seq_printf(m, "%s\t", int32bit ? "ENABLE " : "DISABLE");

	/* mtu index */
	int32bit = ((c2->sram.regs.hwf_attr &
		    MVPP2_CLS2_ACT_HWF_ATTR_MTUIDX_MASK) >>
		    MVPP2_CLS2_ACT_HWF_ATTR_MTUIDX_OFF);
	seq_printf(m, "0x%1.1x\t", int32bit);
	seq_puts(m, "\n\n");

	/*------------------------------*/
	/*	CLSC2_ATTR2 0x1B6C	*/
	/*------------------------------*/
	seq_puts(m, "RSS_ATTR:		RSS_EN		DUP_COUNT	DUP_PTR\n");
	seq_printf(m, "			%d		%d		%d\n",
		   ((c2->sram.regs.rss_attr &
			MVPP2_CLS2_ACT_DUP_ATTR_RSSEN_MASK) >>
			MVPP2_CLS2_ACT_DUP_ATTR_RSSEN_OFF),
		   ((c2->sram.regs.rss_attr &
			MVPP2_CLS2_ACT_DUP_ATTR_DUPCNT_MASK) >>
			MVPP2_CLS2_ACT_DUP_ATTR_DUPCNT_OFF),
		   ((c2->sram.regs.rss_attr &
			MVPP2_CLS2_ACT_DUP_ATTR_DUPID_MASK) >>
			MVPP2_CLS2_ACT_DUP_ATTR_DUPID_OFF));
	seq_puts(m, "\n");

	/*------------------------------*/
	/*	seq_attr 0x1B70		*/
	/*------------------------------*/
	/*PPv2.1 new feature MAS 3.14*/
	seq_puts(m, "SEQ_ATTR:		ID	MISS\n");
	seq_printf(m, "			0x%2.2x    0x%2.2x\n",
		   ((c2->sram.regs.seq_attr &
			MVPP2_CLS2_ACT_SEQ_ATTR_ID_MASK) >>
			MVPP2_CLS2_ACT_SEQ_ATTR_ID),
		   ((c2->sram.regs.seq_attr &
			MVPP2_CLS2_ACT_SEQ_ATTR_MISS_MASK) >>
			MVPP2_CLS2_ACT_SEQ_ATTR_MISS_OFF));
	/* No blank line before HITS printing */
	return 0;
}

static int mvpp2_cls2_hw_dump(struct seq_file *m, void *v)
{
	struct mvpp2 *priv = mvpp2_global_config.hw;
	int index;
	unsigned int cnt;
	struct mvpp2_cls_c2_entry c2;

	memset(&c2, 0, sizeof(c2));

	for (index = 0; index < MVPP2_CLS_C2_TCAM_SIZE; index++) {
		mvpp2_cls_c2_hw_read(priv, index, &c2);
		if (c2.inv)
			continue;
		mvpp2_cls2_sw_dump(m, &c2);
		mvpp2_cls_c2_hit_cntr_read(priv, index, &cnt);
		seq_printf(m, "HITS: %d\n", cnt);
		seq_puts(m, "-----------------------------------------\n");
	}
	return 0;
}

static int mvpp2_cls2_qos_dscp_hw_dump(struct seq_file *m, void *v)
{
	struct mvpp2 *priv = mvpp2_global_config.hw;
	int tbl_id, tbl_line, int32bit;
	struct mvpp2_cls_c2_qos_entry qos;

	for (tbl_id = 0; tbl_id < MVPP2_CLS_C2_QOS_DSCP_TBL_NUM; tbl_id++) {
		seq_printf(m, "\n--------- DSCP TABLE %d ---------\n", tbl_id);
		seq_puts(m, "LINE	DSCP	COLOR	GEM_ID	QUEUE\n");
		for (tbl_line = 0; tbl_line < MVPP2_CLS_C2_QOS_DSCP_TBL_SIZE; tbl_line++) {
			mvpp2_cls_c2_qos_hw_read(priv, tbl_id,
						 1/*DSCP*/, tbl_line, &qos);
			seq_printf(m, "0x%2.2x\t", qos.tbl_line);
			mvpp2_cls_c2_qos_dscp_get(&qos, &int32bit);
			seq_printf(m, "0x%2.2x\t", int32bit);
			mvpp2_cls_c2_qos_color_get(&qos, &int32bit);
			seq_printf(m, "0x%1.1x\t", int32bit);
			mvpp2_cls_c2_qos_gpid_get(&qos, &int32bit);
			seq_printf(m, "0x%3.3x\t", int32bit);
			mvpp2_cls_c2_qos_queue_get(&qos, &int32bit);
			seq_printf(m, "0x%2.2x", int32bit);
			seq_puts(m, "\n");
		}
	}
	return 0;
}

int mvpp2_cl2_qos_prio_hw_dump(struct seq_file *m, void *v)
{
	struct mvpp2 *priv = mvpp2_global_config.hw;
	int tbl_id, tbl_line, int32bit;

	struct mvpp2_cls_c2_qos_entry qos;

	for (tbl_id = 0; tbl_id < MVPP2_CLS_C2_QOS_PRIO_TBL_NUM; tbl_id++) {
		seq_printf(m, "\n------ PRIORITY TABLE %d ---------\n", tbl_id);
		seq_puts(m, "LINE	PRIO	COLOR	GEM_ID	QUEUE\n");

		for (tbl_line = 0; tbl_line < MVPP2_CLS_C2_QOS_PRIO_TBL_SIZE; tbl_line++) {
			mvpp2_cls_c2_qos_hw_read(priv, tbl_id,
						 0/*PRIO*/, tbl_line, &qos);
			seq_printf(m, "0x%2.2x\t", qos.tbl_line);
			mvpp2_cls_c2_qos_prio_get(&qos, &int32bit);
			seq_printf(m, "0x%1.1x\t", int32bit);
			mvpp2_cls_c2_qos_color_get(&qos, &int32bit);
			seq_printf(m, "0x%1.1x\t", int32bit);
			mvpp2_cls_c2_qos_gpid_get(&qos, &int32bit);
			seq_printf(m, "0x%3.3x\t", int32bit);
			mvpp2_cls_c2_qos_queue_get(&qos, &int32bit);
			seq_printf(m, "0x%2.2x", int32bit);
			seq_puts(m, "\n");
		}
	}
	return 0;
}

static int mvpp2_cls2_hit_cntr_dump(struct seq_file *m, void *v)
{
	struct mvpp2 *priv = mvpp2_global_config.hw;
	int i;
	unsigned int cnt;

	for (i = 0; i < MVPP2_CLS_C2_TCAM_SIZE; i++) {
		mvpp2_cls_c2_hit_cntr_read(priv, i, &cnt);
		if (!cnt)
			continue;
		seq_printf(m, "INDEX: 0x%8.8X	VAL: 0x%8.8X\n", i, cnt);
	}
	return 0;
}

static int mvpp2_cls2_regs_dump(struct seq_file *m, void *v)
{
	struct mvpp2 *priv = mvpp2_global_config.hw;
	int i;
	char name[100];

	mvpp2_prt_reg(m, priv,
		      MVPP2_CLS2_TCAM_IDX_REG, "MVPP2_CLS2_TCAM_IDX_REG");

	for (i = 0; i < MVPP2_CLS_C2_TCAM_WORDS; i++) {
		sprintf(name, "MVPP2_CLS2_TCAM_DATA_%d_REG", i);
		mvpp2_prt_reg(m, priv, MVPP2_CLS2_TCAM_DATA_REG(i), name);
	}
	mvpp2_prt_reg(m, priv,
		      MVPP2_CLS2_TCAM_INV_REG,
		      "MVPP2_CLS2_TCAM_INV_REG");
	mvpp2_prt_reg(m, priv,
		      MVPP2_CLS2_ACT_DATA_REG,
		      "MVPP2_CLS2_ACT_DATA_REG");
	mvpp2_prt_reg(m, priv,
		      MVPP2_CLS2_DSCP_PRI_INDEX_REG,
		      "MVPP2_CLS2_DSCP_PRI_INDEX_REG");
	mvpp2_prt_reg(m, priv,
		      MVPP2_CLS2_QOS_TBL_REG,
		      "MVPP2_CLS2_QOS_TBL_REG");
	mvpp2_prt_reg(m, priv,
		      MVPP2_CLS2_ACT_REG,
		      "MVPP2_CLS2_ACT_REG");
	mvpp2_prt_reg(m, priv,
		      MVPP2_CLS2_ACT_QOS_ATTR_REG,
		      "MVPP2_CLS2_ACT_QOS_ATTR_REG");
	mvpp2_prt_reg(m, priv,
		      MVPP2_CLS2_ACT_HWF_ATTR_REG,
		      "MVPP2_CLS2_ACT_HWF_ATTR_REG");
	mvpp2_prt_reg(m, priv,
		      MVPP2_CLS2_ACT_DUP_ATTR_REG,
		      "MVPP2_CLS2_ACT_DUP_ATTR_REG");
	mvpp2_prt_reg(m, priv,
		      MVPP2_CLS2_ACT_SEQ_ATTR_REG,
		      "MVPP22_CLS2_ACT_SEQ_ATTR_REG");
	return 0;
}

/*************  @PRS  *********************************************************
 * { "prs_dump"	, ARG_RO, { 0 }, mvpp2_show_prs_hw	 , "PRS  dump all valid HW entries" },
 * { "prs_hits"	, ARG_RO, { 0 }, mvpp2_show_prs_hw_hits, "PRS  dump hit counters with HW entries" },
 * { "prs_regs"	, ARG_RO, { 0 }, mvpp2_show_prs_hw_regs, "PRS  dump parser registers" },
 */

/* Read tcam entry from hw */
static int mvpp2_prs_hw_read(struct mvpp2 *priv, struct mvpp2_prs_entry *pe)
{
	int i;
	int cpu = get_cpu();

	/* Write tcam index - indirect access */
	mvpp2_percpu_write(priv, cpu, MVPP2_PRS_TCAM_IDX_REG, pe->index);

	pe->tcam.word[MVPP2_PRS_TCAM_INV_WORD] =
		mvpp2_percpu_read(priv, cpu,
				  MVPP2_PRS_TCAM_DATA_REG(MVPP2_PRS_TCAM_INV_WORD));
	if (pe->tcam.word[MVPP2_PRS_TCAM_INV_WORD] & MVPP2_PRS_TCAM_INV_MASK) {
		put_cpu();
		return MVPP2_PRS_TCAM_ENTRY_INVALID;
	}
	for (i = 0; i < MVPP2_PRS_TCAM_WORDS; i++)
		pe->tcam.word[i] = mvpp2_percpu_read(priv, cpu, MVPP2_PRS_TCAM_DATA_REG(i));

	/* Write sram index - indirect access */
	mvpp2_percpu_write(priv, cpu, MVPP2_PRS_SRAM_IDX_REG, pe->index);
	for (i = 0; i < MVPP2_PRS_SRAM_WORDS; i++)
		pe->sram.word[i] = mvpp2_percpu_read(priv, cpu, MVPP2_PRS_SRAM_DATA_REG(i));
	put_cpu();
	return 0;
}

static int mvpp2_prs_sw_sram_next_lu_get(struct mvpp2_prs_entry *pe,
					 unsigned int *lu)
{
	*lu = pe->sram.byte[MVPP2_BIT_TO_BYTE(
		MVPP2_PRS_SRAM_NEXT_LU_OFFS)];
	*lu = ((*lu) >> MVPP2_PRS_SRAM_NEXT_LU_OFFS % 8);
	*lu &= MVPP2_PRS_SRAM_NEXT_LU_MASK;
	return 0;
}

/* Obtain ri bits from sram sw entry */
static unsigned int m_prs_sw_sram_ri_get(struct mvpp2_prs_entry *pe,
					 unsigned int *bits,
					 unsigned int *enable)
{
	*bits = pe->sram.word[MVPP2_PRS_SRAM_RI_OFFS / 32];
	*enable = pe->sram.word[MVPP2_PRS_SRAM_RI_CTRL_OFFS / 32];
	return 0;
}

static int mvpp2_prs_sw_sram_ai_get(struct mvpp2_prs_entry *pe,
				    unsigned int *bits, unsigned int *enable)
{
	*bits = (pe->sram.byte[MVPP2_BIT_TO_BYTE(
		MVPP2_PRS_SRAM_AI_OFFS)] >> (MVPP2_PRS_SRAM_AI_OFFS % 8)) |
		(pe->sram.byte[MVPP2_BIT_TO_BYTE(
			MVPP2_PRS_SRAM_AI_OFFS +
			MVPP2_PRS_SRAM_AI_CTRL_BITS)] <<
			(8 - (MVPP2_PRS_SRAM_AI_OFFS % 8)));

	*enable = (pe->sram.byte[MVPP2_BIT_TO_BYTE(
		MVPP2_PRS_SRAM_AI_CTRL_OFFS)] >>
		(MVPP2_PRS_SRAM_AI_CTRL_OFFS % 8)) |
		(pe->sram.byte[MVPP2_BIT_TO_BYTE(
			MVPP2_PRS_SRAM_AI_CTRL_OFFS +
			MVPP2_PRS_SRAM_AI_CTRL_BITS)] <<
			(8 - (MVPP2_PRS_SRAM_AI_CTRL_OFFS % 8)));

	*bits &= MVPP2_PRS_SRAM_AI_MASK;
	*enable &= MVPP2_PRS_SRAM_AI_MASK;
	return 0;
}

static int mvpp2_prs_sram_bit_get(struct mvpp2_prs_entry *pe, int bit_num,
				  unsigned int *bit)
{
	*bit = pe->sram.byte[MVPP2_BIT_TO_BYTE(bit_num)] &
		(1 << (bit_num % 8));
	*bit = (*bit) >> (bit_num % 8);
	return 0;
}

static int mvpp2_prs_sw_sram_lu_done_get(struct mvpp2_prs_entry *pe,
					 unsigned int *bit)
{
	return mvpp2_prs_sram_bit_get(pe, MVPP2_PRS_SRAM_LU_DONE_BIT, bit);
}

static int mvpp2_prs_sw_sram_flowid_gen_get(struct mvpp2_prs_entry *pe,
					    unsigned int *bit)
{
	return mvpp2_prs_sram_bit_get(pe, MVPP2_PRS_SRAM_LU_GEN_BIT, bit);
}

#undef MVPP2_PRS_SRAM_UDF_OFFS
#define MVPP2_PRS_SRAM_UDF_OFFS		73
static int mvpp2_prs_sw_sram_offset_get(struct mvpp2_prs_entry *pe,
					unsigned int *type, int *offset,
					unsigned int *op)
{
	int sign;

	*type = pe->sram.byte[MVPP2_BIT_TO_BYTE(
		MVPP2_PRS_SRAM_UDF_TYPE_OFFS)] >>
		(MVPP2_PRS_SRAM_UDF_TYPE_OFFS % 8);
	*type &= MVPP2_PRS_SRAM_UDF_TYPE_MASK;

	*offset = (pe->sram.byte[MVPP2_BIT_TO_BYTE(
		MVPP2_PRS_SRAM_UDF_OFFS)] >>
		(MVPP2_PRS_SRAM_UDF_OFFS % 8)) & 0x7f;
	*offset |= (pe->sram.byte[
		    MVPP2_BIT_TO_BYTE(MVPP2_PRS_SRAM_UDF_OFFS +
		    MVPP2_PRS_SRAM_UDF_BITS)] <<
		    (8 - (MVPP2_PRS_SRAM_UDF_OFFS % 8))) & 0x80;

	*op = (pe->sram.byte[MVPP2_BIT_TO_BYTE(
		MVPP2_PRS_SRAM_OP_SEL_SHIFT_OFFS)] >>
		(MVPP2_PRS_SRAM_OP_SEL_SHIFT_OFFS % 8)) & 0x7;
	*op |= (pe->sram.byte[MVPP2_BIT_TO_BYTE(
		MVPP2_PRS_SRAM_OP_SEL_SHIFT_OFFS +
		MVPP2_PRS_SRAM_OP_SEL_SHIFT_BITS)] <<
		(8 - (MVPP2_PRS_SRAM_OP_SEL_SHIFT_OFFS % 8))) & 0x18;

	/* if signed bit is tes */
	sign = pe->sram.byte[MVPP2_BIT_TO_BYTE(
		MVPP2_PRS_SRAM_UDF_SIGN_BIT)] &
		(1 << (MVPP2_PRS_SRAM_UDF_SIGN_BIT % 8));
	if (sign != 0)
		*offset = 1 - (*offset);

	return 0;
}

static int mvpp2_prs_sw_sram_shift_get(struct mvpp2_prs_entry *pe, int *shift)
{
	int sign;

	sign = pe->sram.byte[MVPP2_BIT_TO_BYTE(
		MVPP2_PRS_SRAM_SHIFT_SIGN_BIT)] &
		(1 << (MVPP2_PRS_SRAM_SHIFT_SIGN_BIT % 8));
	*shift = ((int)(pe->sram.byte[MVPP2_BIT_TO_BYTE(
		MVPP2_PRS_SRAM_SHIFT_OFFS)])) &
		MVPP2_PRS_SRAM_SHIFT_MASK;

	if (sign == 1)
		*shift *= -1;
	return 0;
}

static int mvpp2_prs_hw_tcam_cnt_dump(struct seq_file *m, struct mvpp2 *priv,
				      int tid, unsigned int *cnt)
{
	unsigned int reg_val;
	int cpu;

	if (mvpp2_range_validate(m, tid, 0, MVPP2_PRS_TCAM_SRAM_SIZE - 1))
		return -EINVAL;

	/* write index */
	cpu = get_cpu();
	mvpp2_percpu_write(priv, cpu, MVPP2_PRS_TCAM_HIT_IDX_REG, tid);
	reg_val = mvpp2_percpu_read(priv, cpu, MVPP2_PRS_TCAM_HIT_CNT_REG);
	put_cpu();
	reg_val &= MVPP2_PRS_TCAM_HIT_CNT_MASK;

	if (cnt) {
		/* First read after reset returns HIT_CNT = -1 */
		*cnt = (reg_val == MVPP2_PRS_TCAM_HIT_CNT_MASK) ? 0 : reg_val;
	} else {
		seq_printf(m, "HIT COUNTER: %d\n", reg_val);
	}
	return 0;
}

static int mvpp2_prs_sw_sram_ri_dump(struct seq_file *m,
				     struct mvpp2_prs_entry *pe)
{
	unsigned int data, mask;
	int i, bit_offs = 0;
	char bits[100];

	m_prs_sw_sram_ri_get(pe, &data, &mask);
	if (mask == 0)
		return 0;
	seq_puts(m, "\n       ");
	seq_puts(m, "S_RI=");
	for (i = (MVPP2_PRS_SRAM_RI_CTRL_BITS - 1); i > -1 ; i--) {
		if (mask & (1 << i)) {
			seq_printf(m, "%d", ((data & (1 << i)) != 0));
			bit_offs += sprintf(bits + bit_offs, "%d:", i);
		} else {
			seq_puts(m, "x");
		}
	}
	bits[bit_offs] = '\0';
	seq_printf(m, " %s", bits);
	return 0;
}

static int mvpp2_prs_sw_sram_ai_dump(struct seq_file *m,
				     struct mvpp2_prs_entry *pe)
{
	int i, bit_offs = 0;
	unsigned int data, mask;
	char bits[30];

	mvpp2_prs_sw_sram_ai_get(pe, &data, &mask);
	if (mask == 0)
		return 0;

	seq_puts(m, "\n       ");
	seq_puts(m, "S_AI=");
	for (i = (MVPP2_PRS_SRAM_AI_CTRL_BITS - 1); i > -1 ; i--) {
		if (mask & (1 << i)) {
			seq_printf(m, "%d", ((data & (1 << i)) != 0));
			bit_offs += sprintf(bits + bit_offs, "%d:", i);
		} else {
			seq_puts(m, "x");
		}
	}
	bits[bit_offs] = '\0';
	seq_printf(m, " %s", bits);
	return 0;
}

static int mvpp2_prs_sw_dump(struct seq_file *m, struct mvpp2_prs_entry *pe)
{
	u32 op, type, lu, done, flowid;
	int	shift, offset, i;

	seq_printf(m, "[%4d] ", pe->index);
	i = MVPP2_PRS_TCAM_WORDS - 1;
	seq_printf(m, "%1.1x ", pe->tcam.word[i--] & 0xF);
	while (i >= 0)
		seq_printf(m, "%4.4x ", (pe->tcam.word[i--]) & 0xFFFF);
	seq_puts(m, "| ");
	seq_printf(m, "%4.4x %8.8x %8.8x %8.8x", pe->sram.word[3] & 0xFFFF,
		   pe->sram.word[2],  pe->sram.word[1],  pe->sram.word[0]);
	seq_puts(m, "\n       ");
	i = MVPP2_PRS_TCAM_WORDS - 1;
	seq_printf(m, "%1.1x ", (pe->tcam.word[i--] >> 16) & 0xF);
	while (i >= 0)
		seq_printf(m, "%4.4x ", ((pe->tcam.word[i--]) >> 16)  & 0xFFFF);
	seq_puts(m, "| ");

	mvpp2_prs_sw_sram_shift_get(pe, &shift);

	seq_printf(m, "SH=%d ", shift);

	mvpp2_prs_sw_sram_offset_get(pe, &type, &offset, &op);
	if (offset != 0 || ((op >> MVPP2_PRS_SRAM_OP_SEL_SHIFT_BITS) != 0))
		seq_printf(m, "UDFT=%u UDFO=%d ", type, offset);

	seq_printf(m, "op=%u ", op);

	mvpp2_prs_sw_sram_next_lu_get(pe, &lu);
	seq_printf(m, "LU=%u ", lu);

	mvpp2_prs_sw_sram_lu_done_get(pe, &done);
	seq_printf(m, "%s ", done ? "DONE" : "N_DONE");

	/*flow id generation bit*/
	mvpp2_prs_sw_sram_flowid_gen_get(pe, &flowid);
	seq_printf(m, "%s ", flowid ? "FIDG" : "N_FIDG");

	if ((pe->tcam.word[MVPP2_PRS_TCAM_INV_WORD] & MVPP2_PRS_TCAM_INV_MASK))
		seq_puts(m, " [inv]");
	if (mvpp2_prs_sw_sram_ri_dump(m, pe))
		return -EINVAL;
	if (mvpp2_prs_sw_sram_ai_dump(m, pe))
		return -EINVAL;
	seq_puts(m, "\n");
	return 0;
}

int mvpp2_show_prs_hw_regs(struct seq_file *m, void *v)
{
	int i;
	char reg_name[100];
	struct mvpp2 *priv = mvpp2_global_config.hw;

	mvpp2_show_reg(m, priv,
		       MVPP2_PRS_INIT_LOOKUP_REG,
		       "MVPP2_PRS_INIT_LOOKUP_REG");
	mvpp2_show_reg(m, priv,
		       MVPP2_PRS_INIT_OFFS_REG(0),
		       "MVPP2_PRS_INIT_OFFS_0_3_REG");
	mvpp2_show_reg(m, priv,
		       MVPP2_PRS_INIT_OFFS_REG(4),
		       "MVPP2_PRS_INIT_OFFS_4_7_REG");
	mvpp2_show_reg(m, priv,
		       MVPP2_PRS_MAX_LOOP_REG(0),
		       "MVPP2_PRS_MAX_LOOP_0_3_REG");
	mvpp2_show_reg(m, priv,
		       MVPP2_PRS_MAX_LOOP_REG(4),
		       "MVPP2_PRS_MAX_LOOP_4_7_REG");
	mvpp2_show_reg(m, priv,
		       MVPP2_PRS_TCAM_IDX_REG,
		       "MVPP2_PRS_TCAM_IDX_REG");

	for (i = 0; i < MVPP2_PRS_TCAM_WORDS; i++) {
		sprintf(reg_name, "MVPP2_PRS_TCAM_DATA_%d_REG", i);
		mvpp2_show_reg(m, priv, MVPP2_PRS_TCAM_DATA_REG(i),
			       reg_name);
	}
	mvpp2_show_reg(m, priv,
		       MVPP2_PRS_SRAM_IDX_REG, "MVPP2_PRS_SRAM_IDX_REG");

	for (i = 0; i < MVPP2_PRS_SRAM_WORDS; i++) {
		sprintf(reg_name, "MVPP2_PRS_SRAM_DATA_%d_REG", i);
		mvpp2_show_reg(m, priv, MVPP2_PRS_SRAM_DATA_REG(i),
			       reg_name);
	}
	mvpp2_show_reg(m, priv,
		       MVPP2_PRS_EXP_REG, "MVPP2_PRS_EXP_REG");
	mvpp2_show_reg(m, priv,
		       MVPP2_PRS_TCAM_CTRL_REG, "MVPP2_PRS_TCAM_CTRL_REG");
	return 0;
}

static int mvpp2_prs_hw_dump_util(struct seq_file *m, int req_hit)
{
	int index, valid_cntr, min, max;
	unsigned int cnt, cnt_total;
	struct mvpp2_prs_entry pe;
	struct mvpp2 *priv = mvpp2_global_config.hw;
	bool hits_only = (req_hit >= MVPP2_PRS_TCAM_SRAM_SIZE);

	valid_cntr = 0;
	if (req_hit >= 0 && req_hit < MVPP2_PRS_TCAM_SRAM_SIZE) {
		min = req_hit;
		max = req_hit + 1;
	} else {
		min = 0;
		max = MVPP2_PRS_TCAM_SRAM_SIZE;
	}
	cnt = 0;
	cnt_total = 0;

	for (index = min; index < max; index++) {
		pe.index = index;
		mvpp2_prs_hw_read(priv, &pe);
		if ((pe.tcam.word[MVPP2_PRS_TCAM_INV_WORD] &
		    MVPP2_PRS_TCAM_INV_MASK) ==
		    MVPP2_PRS_TCAM_ENTRY_VALID) {
			if (req_hit >= 0) {
				mvpp2_prs_hw_tcam_cnt_dump(m, priv, index, &cnt);
				if (hits_only && !cnt)
					continue;
				cnt_total += cnt;
			}
			valid_cntr++;
			mvpp2_prs_sw_dump(m, &pe);
			if (cnt)
				seq_printf(m, "       HITS: %d\n", cnt);
			seq_puts(m, "-----------------------------------------\n");
		}
	}
	if (req_hit >= 0)
		seq_printf(m, "   %d entries, total hits %u\n",
			   valid_cntr, cnt_total);
	else
		seq_printf(m, "   %d entries\n", valid_cntr);
	return 0;
}

static int mvpp2_show_prs_hw(struct seq_file *m, void *v)
{
	return mvpp2_prs_hw_dump_util(m, -1);
}

static int mvpp2_show_prs_hw_hits(struct seq_file *m, void *v)
{
	return mvpp2_prs_hw_dump_util(m, MVPP2_PRS_TCAM_SRAM_SIZE);
}

/***************  @RSS  *******************************************************
 * { "rss_hw_dump"	, ARG_RO, { 0 }, mvpp2_rss_hw_dump	, "RSS  dump rxq in rss-table entry" },
 * { "rss_hw_rxq_tbl"	, ARG_RO, { 0 }, mvpp2_rss_hw_rxq_tbl	, "RSS  dump rxq table assignment" },
 */
static int mvpp2__rss_tbl_entry_get(struct mvpp2 *priv,
				    struct mvpp22_rss_entry *rss)
{
	unsigned int reg_val = 0;
	int cpu;

	if (rss->sel == MVPP22_RSS_ACCESS_POINTER) {
		/* Set index */
		reg_val |= (rss->u.pointer.rxq_idx <<
				MVPP22_RSS_IDX_RXQ_NUM_OFF);
		cpu = get_cpu();
		mvpp2_percpu_write(priv, cpu, MVPP22_RSS_IDX_REG, reg_val);
		/* Read entry */
		rss->u.pointer.rss_tbl_ptr =
			mvpp2_percpu_read(priv, cpu, MVPP22_RSS_RXQ2RSS_TBL_REG) &
			       MVPP22_RSS_RXQ2RSS_TBL_POINT_MASK;
		put_cpu();
	} else if (rss->sel == MVPP22_RSS_ACCESS_TBL) {
		if (rss->u.entry.tbl_id >= MVPP22_RSS_TBL_NUM ||
		    rss->u.entry.tbl_line >= MVPP22_RSS_TBL_LINE_NUM)
			return -EINVAL;
		/* Read index */
		cpu = get_cpu();
		reg_val |= (rss->u.entry.tbl_line <<
				MVPP22_RSS_IDX_ENTRY_NUM_OFF |
			   rss->u.entry.tbl_id <<
				MVPP22_RSS_IDX_TBL_NUM_OFF);
		mvpp2_percpu_write(priv, cpu, MVPP22_RSS_IDX_REG, reg_val);
		/* Read entry */
		rss->u.entry.rxq = mvpp2_percpu_read(priv, cpu,
						     MVPP22_RSS_TBL_ENTRY_REG) &
						     MVPP22_RSS_TBL_ENTRY_MASK;
		rss->u.entry.width = mvpp2_percpu_read(priv, cpu,
						       MVPP22_RSS_WIDTH_REG) &
						       MVPP22_RSS_WIDTH_MASK;
		put_cpu();
	}
	return 0;
}

static int mvpp2_rss_hw_dump(struct seq_file *m, void *v)
{
	int tbl_id, tbl_line;
	struct mvpp22_rss_entry rss_entry;
	struct mvpp2 *priv = mvpp2_global_config.hw;

	memset(&rss_entry, 0, sizeof(struct mvpp22_rss_entry));
	rss_entry.sel = MVPP22_RSS_ACCESS_TBL;

	for (tbl_id = 0; tbl_id < MVPP22_RSS_TBL_NUM; tbl_id++) {
		seq_printf(m, "\n-------- RSS TABLE %d-----------\n", tbl_id);
		seq_puts(m, "HASH	QUEUE	WIDTH\n");

		for (tbl_line = 0; tbl_line < MVPP22_RSS_TBL_LINE_NUM;
			tbl_line++) {
			rss_entry.u.entry.tbl_id = tbl_id;
			rss_entry.u.entry.tbl_line = tbl_line;
			mvpp2__rss_tbl_entry_get(priv, &rss_entry);
			seq_printf(m, "0x%2.2x\t", rss_entry.u.entry.tbl_line);
			seq_printf(m, "0x%2.2x\t", rss_entry.u.entry.rxq);
			seq_printf(m, "0x%2.2x", rss_entry.u.entry.width);
			seq_puts(m, "\n");
		}
	}
	return 0;
}

static int mvpp2_rss_hw_rxq_tbl(struct seq_file *m, void *v)
{
	int rxq, port;
	struct mvpp22_rss_entry rss_entry;
	struct mvpp2 *priv = mvpp2_global_config.hw;

	memset(&rss_entry, 0, sizeof(struct mvpp22_rss_entry));
	rss_entry.sel = MVPP22_RSS_ACCESS_POINTER;

	for (port = 0; port < MVPP2_MAX_PORTS; port++) {
		seq_printf(m, "\n------- RXQ TABLE PORT %d----------\n", port);
		seq_puts(m, "QUEUE	RSS TBL\n");

		for (rxq = 0; rxq < MVPP22_RSS_TBL_LINE_NUM; rxq++) {
			rss_entry.u.pointer.rxq_idx = port * MVPP22_RSS_TBL_LINE_NUM + rxq;
			if (mvpp2__rss_tbl_entry_get(priv, &rss_entry))
				return -1;
		}
	}
	return 0;
}

/******************  @UC MAC    ***********************************************
 * { "uc_filter_add"	, UCADD , { 0 }, mvpp2_uc_filter_dump	, "UC MAC  add       to given ifname" },
 * { "uc_filter_del"	, UCDEL , { 0 }, mvpp2_uc_filter_dump	, "UC MAC delete   from given ifname" },
 * { "uc_filter_delall"	, UCDELA, { 0 }, mvpp2_uc_filter_dump	, "UC MAC delete ALL (flush) uc-macs" },
 * { "uc_filter_show"	, ARG_RO, { 0 }, mvpp2_uc_filter_dump	, "UC MAC list on Pre-Set ethN interface" },
 */
static int mvpp2_uc_filter_dump(struct seq_file *m, void *v)
{
	struct net_device *netdev;
	struct netdev_hw_addr *ha;

	netdev = dev_get_by_name(&init_net, mvpp2_global_config.ifname);
	if (!netdev)
		return -EINVAL;

	if (netdev_uc_count(netdev) == 0) {
		seq_printf(m, "%s: UC list is empty\n", netdev->name);
		goto exit;
	}
	seq_printf(m, "%s: UC list dump:\n", netdev->name);
	netdev_for_each_uc_addr(ha, netdev) {
		seq_printf(m, "%02x:%02x:%02x:%02x:%02x:%02x\n",
			   ha->addr[0], ha->addr[1],
			   ha->addr[2], ha->addr[3],
			   ha->addr[4], ha->addr[5]);
	}
exit:
	dev_put(netdev);
	return 0;
}

static size_t mvpp2_uc_filter_flush_delall(char *cmd, size_t size)
{
	struct netdev_hw_addr *ha;
	int nargs, eth_no;
	struct net_device *netdev;
	int cpn;

	nargs = sscanf(cmd, "eth%d", &eth_no);
	if (nargs != 1)
		return -EINVAL;

	netdev = mvpp2_dbgfs_get_netdev(eth_no, &cpn, false);
	if (!netdev)
		return -EINVAL;

	netdev_for_each_uc_addr(ha, netdev)
		dev_uc_del(netdev, ha->addr);

	dev_put(netdev);
	return size;
}

static size_t mvpp2_uc_filter_set(bool add, char *cmd, size_t size)
{
	int nargs, eth_no;
	struct net_device *netdev;
	u8 mac[ETH_ALEN];
	u32 inp[ETH_ALEN];
	int i, cpn;

	nargs = sscanf(cmd, "eth%d %2x:%2x:%2x:%2x:%2x:%x", &eth_no,
		       &inp[0], &inp[1], &inp[2], &inp[3], &inp[4], &inp[5]);
	if (nargs != (1 + ETH_ALEN) || inp[5] > 0xff)
		return -EINVAL;
	for (i = 0; i < ETH_ALEN; i++)
		mac[i] = (u8)inp[i];
	if (!is_valid_ether_addr(mac))
		return -EINVAL;

	netdev = mvpp2_dbgfs_get_netdev(eth_no, &cpn, false);
	if (!netdev)
		return -EINVAL;
	if (add) {
		if (dev_uc_add(netdev, mac))
			size = -EEXIST;
	} else {
		if (dev_uc_del(netdev, mac))
			size = -ENOENT;
	}
	dev_put(netdev);
	return size;
}
