/*********************************************************************
 * DebugFS for Marvell PPv2 network controller for Armada 7k/8k SoC.
 *
 * Copyright (C) 2018 Marvell
 *
 * Yan Markman <ymarkman@marvell.com>
 *
 * This file is licensed under the terms of the GNU General Public
 * License version 2. This program is licensed "as is" without any
 * warranty of any kind, whether express or implied.
 */

/* DEBUG-FS has several HW/SW functions/functionalities
 * which are not present in mvpp2.c.
 * So there are some extra HW/SW defines required
 * and defined in this mvpp2_dbgfs.h
 */

#ifndef __MVPP2_DBGFS_H__
#define __MVPP2_DBGFS_H__

#define MVPP2_C_INCLUDE_ONLY
#include "mvpp2_def.h"

#ifndef MVPP2_BIT_TO_BYTE
#define MVPP2_BIT_TO_BYTE	MVPP2_SRAM_BIT_TO_BYTE
#endif

#define MVPP2_PRS_EXP_REG			0x1214
#define MVPP2_PRS_TCAM_HIT_IDX_REG		0x1240
#define MVPP2_PRS_TCAM_HIT_CNT_REG		0x1244
#define MVPP2_PRS_TCAM_HIT_CNT_BITS		16
#define MVPP2_PRS_TCAM_HIT_CNT_MASK     (((1 << 16/*BITS*/) - 1) << 0/*OFFS*/)
#define MVPP2_PRS_TCAM_ENTRY_VALID	0

/*********** Classifier C1**************************/
#define MVPP2_CLS_LKP_TBL_HIT_REG		0x7700
#define MVPP2_CLS_FLOW_TBL_HIT_REG		0x7704
#define MVPP2_CLS_LKP_INDEX_LKP_OFFS		0

#define MVPP2_CLS_FLOWS_TBL_SIZE	MVPP2_FLOW_TBL_SIZE
#define MVPP2_CLS_FLOWS_TBL_FIELDS_MAX	4

#define MVPP2_CLS_PORT_SPID_REG		0x1830
#define MVPP2_CLS_SPID_UNI_BASE_REG	0x1840
#define MVPP2_CLS_SPID_UNI_REGS		4

#define MVPP2_CLS_GEM_VIRT_INDEX_REG	0x1A00
#define MVPP2_CLS_GEM_VIRT_INDEX_BITS	(7)
#define MVPP2_CLS_GEM_VIRT_INDEX_MAX	(((1 << MVPP2_CLS_GEM_VIRT_INDEX_BITS) - 1) << 0)
#define MVPP2_CLS_GEM_VIRT_REGS_NUM	128
#define MVPP2_CLS_GEM_VIRT_REG		0x1A04

#define MVPP2_CLS_GEM_VIRT_BITS			12
#define MVPP2_CLS_GEM_VIRT_MAX		((1 << MVPP2_CLS_GEM_VIRT_BITS) - 1)
#define MVPP2_CLS_GEM_VIRT_MASK		(((1 << MVPP2_CLS_GEM_VIRT_BITS) - 1) << 0)
#define MVPP2_CLS_UDF_BASE_REG		0x1860
#define MVPP2_CLS_UDF_REG(index)	(MVPP2_CLS_UDF_BASE_REG + ((index) * 4)) /*index <=63*/
#define MVPP2_CLS_UDF_REGS_NUM		64
#define MVPP2_CLS_UDF_BASE_REGS		8

#define MVPP2_CLS_MTU_BASE_REG		0x1900
#define MVPP2_CLS_MTU_REG(num)	(MVPP2_CLS_MTU_BASE_REG + ((num) * 4))
#define MVPP2_CLS_MTU_OFFS		0
#define MVPP2_CLS_MTU_BITS		16
#define MVPP2_CLS_MTU_MAX	((1 << MVPP2_CLS_MTU_BITS) - 1)
#define MVPP2_CLS_MTU_MASK	(((1 << MVPP2_CLS_MTU_BITS) - 1) << MVPP2_CLS_MTU_OFFS)

#define MVPP2_CLS_SEQ_SIZE_REG		0x19D4
#define MVPP2_CLS_PCTRL_BASE_REG	0x1880
#define MVPP2_CLS_PCTRL_REG(port)	(MVPP2_CLS_PCTRL_BASE_REG + 4 * (port))

#define MVPP2_FLOWID_RXQ		MVPP2_FLOWID_RXQ_OFFS
#define MVPP2_FLOWID_EN			MVPP2_FLOWID_EN_OFFS
#define MVPP2_FLOWID_MODE		8
#define MVPP2_FLOWID_MODE_BITS		8
#define MVPP2_FLOWID_MODE_MASK		(((1 << \
			MVPP2_FLOWID_MODE_BITS) - 1) << MVPP2_FLOWID_MODE)

/*********** Classifier C2**************************/
#define MVPP2_CLS2_ACT_SEQ_ATTR_REG		0x1B70
#define MVPP2_CLS2_ACT_SEQ_ATTR_ID		0

#define MVPP21_CLS2_ACT_SEQ_ATTR_MISS_OFF	8
#define MVPP22_CLS2_ACT_SEQ_ATTR_MISS_OFF	16
#define MVPP2_CLS2_ACT_SEQ_ATTR_MISS_OFF	MVPP22_CLS2_ACT_SEQ_ATTR_MISS_OFF
#define MVPP21_CLS2_ACT_SEQ_ATTR_ID_MASK	0x000000ff
#define MVPP22_CLS2_ACT_SEQ_ATTR_ID_MASK	0x0000ffff
#define MVPP2_CLS2_ACT_SEQ_ATTR_ID_MASK		MVPP22_CLS2_ACT_SEQ_ATTR_ID_MASK
#define MVPP21_CLS2_ACT_SEQ_ATTR_MISS_MASK	0x00000100
#define MVPP22_CLS2_ACT_SEQ_ATTR_MISS_MASK	0x00001000
#define MVPP2_CLS2_ACT_SEQ_ATTR_MISS_MASK	MVPP22_CLS2_ACT_SEQ_ATTR_MISS_MASK

/*********** Classifier Parser**************************/
#define MVPP2_PRS_SRAM_OP_SEL_SHIFT_BITS	2
#define MVPP2_PRS_SRAM_SHIFT_MASK		((1 << 8) - 1)
#define MVPP22_RSS_TBL_NUM			8
#define MVPP22_RSS_TBL_LINE_NUM			32

#define MVPP2_CNT_IDX_REG		0x7040
/* LKP counters index */
#define MVPP2_CNT_IDX_LKP(lkp, way)	((way) << 6 | (lkp))
/* Flow counters index */
#define MVPP2_CNT_IDX_FLOW(index)	(index)
/* TX counters index */
#define MVPP2_CNT_IDX_TX(port, txq)	(((16 + port) << 3) | (txq))

enum mvpp22_rss_access_sel {
	MVPP22_RSS_ACCESS_POINTER,
	MVPP22_RSS_ACCESS_TBL,
};

union mvpp22_rss_access_entry {
	struct mvpp22_rss_tbl_ptr pointer;
	struct mvpp22_rss_tbl_entry entry;
};

struct mvpp22_rss_entry {
	enum mvpp22_rss_access_sel sel;
	union mvpp22_rss_access_entry u;
};

#endif /* __MVPP2_DBGFS_H__ */
