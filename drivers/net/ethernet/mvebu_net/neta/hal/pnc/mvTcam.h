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
Marvell Commercial License Option

If you received this File from Marvell and you have entered into a commercial
license agreement (a "Commercial License") with Marvell, the File is licensed
to you under the terms of the applicable Commercial License.

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
********************************************************************************
Marvell BSD License Option

If you received this File from Marvell, you may opt to use, redistribute and/or
modify this File under the following licensing terms.
Redistribution and use in source and binary forms, with or without modification,
are permitted provided that the following conditions are met:

    *   Redistributions of source code must retain the above copyright notice,
	this list of conditions and the following disclaimer.

    *   Redistributions in binary form must reproduce the above copyright
	notice, this list of conditions and the following disclaimer in the
	documentation and/or other materials provided with the distribution.

    *   Neither the name of Marvell nor the names of its contributors may be
	used to endorse or promote products derived from this software without
	specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

*******************************************************************************/

#ifndef __MV_TCAM_H__
#define __MV_TCAM_H__

/************************** NETA PNC Registers ******************************/

#define MV_PNC_LOOP_CTRL_REG 				(MV_PNC_REG_BASE + 0x00)

#define MV_PNC_TCAM_CTRL_REG 				(MV_PNC_REG_BASE + 0x04)

#define MV_PNC_TCAM_POWER_UP_OFFS			0
#define MV_PNC_TCAM_POWER_UP(array)			(1 << ((array) + MV_PNC_TCAM_POWER_UP_OFFS))
/*-------------------------------------------------------------------------------*/

#define MV_PNC_INIT_OFFS_REG 				(MV_PNC_REG_BASE + 0x08)

#define MV_PNC_PORT_BUF_INIT_BITS			6
#define MV_PNC_PORT_BUF_INIT_MAX			((1 << MV_PNC_PORT_BUF_INIT_BITS) - 1)
#define MV_PNC_PORT_BUF_INIT_MASK(port)		(MV_PNC_PORT_BUF_INIT_MAX << ((port) * MV_PNC_PORT_BUF_INIT_BITS))
#define MV_PNC_PORT_BUF_INIT_VAL(port, val)	((val) << ((port) * MV_PNC_PORT_BUF_INIT_BITS))
/*-------------------------------------------------------------------------------*/

#define MV_PNC_INIT_LOOKUP_REG 				(MV_PNC_REG_BASE + 0x0C)

#define MV_PNC_PORT_LU_INIT_BITS			4
#define MV_PNC_PORT_LU_INIT_MAX				((1 << MV_PNC_PORT_LU_INIT_BITS) - 1)
#define MV_PNC_PORT_LU_INIT_MASK(port)		(MV_PNC_PORT_BUF_INIT_MAX << ((port) * MV_PNC_PORT_LU_INIT_BITS))
#define MV_PNC_PORT_LU_INIT_VAL(port, val)	((val) << ((port) * MV_PNC_PORT_LU_INIT_BITS))
/*-------------------------------------------------------------------------------*/

#define MV_PNC_CAUSE_REG 					(MV_PNC_REG_BASE + 0x10)
#define MV_PNC_MASK_REG 					(MV_PNC_REG_BASE + 0x14)

#define MV_PNC_HIT_SEQ0_REG					(MV_PNC_REG_BASE + 0x18)

#define MV_PNC_HIT_ENABLE_BIT				0
#define MV_PNC_HIT_ENABLE_MASK				(1 << MV_PNC_HIT_ENABLE_BIT)

#define MV_PNC_HIT_PORT_OFFS				1
#define MV_PNC_HIT_PORT_BITS				3
#define MV_PNC_HIT_PORT_MAX					((1 << MV_PNC_HIT_PORT_BITS) - 1)
#define MV_PNC_HIT_PORT_MASK				(MV_PNC_HIT_PORT_MAX << MV_PNC_HIT_PORT_OFFS)
#define MV_PNC_HIT_PORT(port)				(port << MV_PNC_HIT_PORT_OFFS)
/*-------------------------------------------------------------------------------*/

#define MV_PNC_HIT_SEQ1_REG					(MV_PNC_REG_BASE + 0x1C)
#define MV_PNC_HIT_SEQ2_REG					(MV_PNC_REG_BASE + 0x20)
#define MV_PNC_XBAR_RET_REG					(MV_PNC_REG_BASE + 0x24)
/*-------------------------------------------------------------------------------*/


#ifdef MV_ETH_PNC_AGING
#define MV_PNC_AGING_MAX_GROUP              4

/* Aging Control register */
#define MV_PNC_AGING_CTRL_REG               (MV_PNC_REG_BASE + 0x28)

#define MV_PNC_AGING_RESET_ON_READ_BIT      0
#define MV_PNC_AGING_RESET_ON_READ_MASK     (1 << MV_PNC_AGING_RESET_ON_READ_BIT)

#define MV_PNC_AGING_SCAN_VALID_BIT         1
#define MV_PNC_AGING_SCAN_VALID_MASK        (1 << MV_PNC_AGING_SCAN_VALID_BIT)

#define MV_PNC_AGING_GROUP_RESET_OFFS       2
#define MV_PNC_AGING_GROUP_RESET_MASK       (0xF << MV_PNC_AGING_GROUP_RESET_OFFS)
#define MV_PNC_AGING_GROUP_RESET(gr)        (1 << (gr + MV_PNC_AGING_GROUP_RESET_OFFS))

#define MV_PNC_AGING_SCAN_START_BIT         6
#define MV_PNC_AGING_SCAN_START_MASK        (1 << MV_PNC_AGING_SCAN_START_BIT)

#define MV_PNC_AGING_SCAN_DISABLE_BIT       7
#define MV_PNC_AGING_SCAN_DISABLE_MASK      (1 << MV_PNC_AGING_SCAN_DISABLE_BIT)
/*-------------------------------------------------------------------------------*/

#define MV_PNC_AGING_LO_THRESH_REG(gr)      (MV_PNC_REG_BASE + 0x2C + ((gr) << 2))
#define MV_PNC_AGING_HI_THRESH_REG          (MV_PNC_REG_BASE + 0x3C)
/*-------------------------------------------------------------------------------*/
#endif /* MV_ETH_PNC_AGING */

#ifdef MV_ETH_PNC_LB

#define MV_PNC_LB_TBL_ACCESS_REG            (MV_PNC_REG_BASE + 0x40)

#define MV_PNC_LB_TBL_ADDR_OFFS             0
#define MV_PNC_LB_TBL_ADDR_MASK             (0x3F << MV_PNC_LB_TBL_ADDR_OFFS)

#define MV_PNC_LB_TBL_DATA_OFFS             6
#define MV_PNC_LB_TBL_DATA_MASK             (0xFFF << MV_PNC_LB_TBL_DATA_OFFS)

#define MV_PNC_LB_TBL_WRITE_TRIG_BIT        18
#define MV_PNC_LB_TBL_WRITE_TRIG_MASK       (1 << MV_PNC_LB_TBL_WRITE_TRIG_BIT)
/*-------------------------------------------------------------------------------*/

#define MV_PNC_LB_CRC_INIT_REG              (MV_PNC_REG_BASE + 0x44)
#endif /* MV_ETH_PNC_LB */


#define MV_PNC_TCAM_ARRAY_SIZE		256
#define MV_PNC_TOTAL_DATA_SIZE		120
#define MV_PNC_LOOKUP_DATA_SIZE		24

#ifdef MV_ETH_PNC_NEW
#define PNC_TCAM_ACCESS_MASK        (BIT18)
#define PNC_SRAM_ACCESS_MASK        (BIT18 | BIT16)
#define PNC_AGING_ACCESS_MASK       (BIT18 | BIT17)
#define SRAM_LEN                    5 /* SRAM in words */
#define SRAM_FMT                    "%5.5x %8.8x %8.8x %8.8x %8.8x"
#define SRAM_VAL(p)                 p[4] & 0xFFFFF, p[3], p[2], p[1], p[0]
#else
#define PNC_TCAM_ACCESS_MASK        (BIT17)
#define PNC_SRAM_ACCESS_MASK        (BIT17 | BIT16)
#define SRAM_LEN                    4 /* SRAM in words */
#define SRAM_FMT                    "%8.8x %8.8x %8.8x %8.8x"
#define SRAM_VAL(p)                 p[3], p[2], p[1], p[0]
#endif /* MV_ETH_PNC_NEW */

#define TCAM_LEN                    7 /* TCAM key/mask in words */

#define TCAM_LINE_INDEX_OFFS	6
#define TCAM_WORD_ENTRY_OFFS	2

#define AI_WORD					6
#define AI_OFFS					0
#define AI_BITS  				7
#define AI_MASK					((1 << AI_BITS) - 1)

#define PORT_WORD				6
#define PORT_OFFS				7
#define PORT_BITS  				5
#define PORT_MASK				((1 << PORT_BITS) - 1)

#define LU_WORD					6
#define LU_OFFS					12
#define LU_BITS  				4
#define LU_MASK					((1 << LU_BITS) - 1)

#ifdef MV_ETH_PNC_NEW
#define RI_EXTRA_BITS  		    12
#define RI_EXTRA_MASK			((1 << RI_EXTRA_BITS) - 1)

#define FLOW_CTRL_BITS          8
#define FLOW_PART_BITS          4
#else
#define FLOW_CTRL_BITS          2
#define FLOW_PART_BITS          16
#endif /* MV_ETH_PNC_NEW */

#define FLOW_CTRL_MASK          ((1 << FLOW_CTRL_BITS) - 1)
#define FLOW_CTRL_HALF_MASK     ((1 << (FLOW_CTRL_BITS / 2)) - 1)
#define FLOW_PART_MASK          ((1 << FLOW_PART_BITS) - 1)

#define RI_BITS  				24
#define RI_MASK					((1 << RI_BITS) - 1)

#define SHIFT_VAL_BITS			7
#define SHIFT_VAL_MASK			((1 << SHIFT_VAL_BITS) - 1)
#define SHIFT_IDX_BITS			3
#define SHIFT_IDX_MASK			((1 << SHIFT_IDX_BITS) - 1)
#define RXQ_BITS				3
#define RXQ_MASK				((1 << RXQ_BITS) - 1)

#define LB_QUEUE_BITS           2
#define LB_QUEUE_MASK			((1 << LB_QUEUE_BITS) - 1)
#define LB_DISABLE_VALUE		0
#define LB_2_TUPLE_VALUE		1
#define LB_4_TUPLE_VALUE		2

#define FLOW_VALUE_OFFS 		0   /* 32 bits */
#define FLOW_CTRL_OFFS			32  /* 8 bits */

/* PNC SRAM Layout */
#ifdef MV_ETH_PNC_NEW
#define RI_VALUE_OFFS 			40  /* 24 bits */
#define RI_MASK_OFFS  			64  /* 24 bits */

#define RI_EXTRA_VALUE_OFFS 	88  /* 12 bits */
#define RI_EXTRA_CTRL_OFFS  	100 /* 6 bits */

#define SHIFT_VAL_OFFS 			106	/* 7 bits - shift update value offset */
#define SHIFT_IDX_OFFS 			113	/* 3 bits - shift update index offset */
#define RXQ_INFO_OFFS  			116 /* 1 bit */
#define RXQ_QUEUE_OFFS 			117 /* 3 bits */
#define LB_QUEUE_OFFS           120 /* 2 bits - load balancing queue info */
#define NEXT_LU_SHIFT_OFFS  	122 /* 3 bits */
#define LU_DONE_OFFS  			125 /* 1 bit */
#define KEY_TYPE_OFFS           126 /* 4 bits */
#define AI_VALUE_OFFS 			130 /* 7 bits */
#define AI_MASK_OFFS  			137 /* 7 bits */
#define LU_ID_OFFS  			144 /* 4 bits */
#else /* Old PNC version (z1) */
#define RI_VALUE_OFFS 			34
#define RI_MASK_OFFS  			58
#define SHIFT_VAL_OFFS 			82	/* shift update value offset */
#define SHIFT_IDX_OFFS 			89	/* shift update index offset */
#define RXQ_INFO_OFFS  			92
#define RXQ_QUEUE_OFFS 			93
#define NEXT_LU_SHIFT_OFFS  	96
#define LU_DONE_OFFS  			99
#define AI_VALUE_OFFS 			104
#define AI_MASK_OFFS  			111
#define LU_ID_OFFS  			118
#endif /* MV_ETH_PNC_NEW */

#define SHIFT_IP4_HLEN			126 /* IPv4 dynamic shift index */
#define SHIFT_IP6_HLEN			127 /* IPv6 dynamic shift index */

/*
 * TCAM misc/control
 */
#define TCAM_F_INV 				1
#define TCAM_TEXT				16

/*
 * TCAM control
 */
struct tcam_ctrl {
	unsigned int index;
	unsigned int flags;
	unsigned char text[TCAM_TEXT];
};

/*
 * TCAM key
 */
struct tcam_data {
	union {
		unsigned int word[TCAM_LEN];
		unsigned char byte[TCAM_LEN*4];
	} u;
};

/*
 * TCAM mask
 */
struct tcam_mask {
	union {
		unsigned int word[TCAM_LEN];
		unsigned char byte[TCAM_LEN*4];
	} u;
};

/*
 * SRAM entry
 */
struct sram_entry {
	unsigned int word[SRAM_LEN];
};

/*
 * TCAM entry
 */
struct tcam_entry {
	struct tcam_data data;
	struct tcam_mask mask;
	struct sram_entry sram;
	struct tcam_ctrl ctrl;
}  __attribute__((packed));

/*
 * TCAM Low Level API
 */
#ifdef MV_ETH_PNC_NEW
void sram_sw_set_rinfo_extra(struct tcam_entry *te, unsigned int ri_extra);
void sram_sw_set_load_balance(struct tcam_entry *te, unsigned int value);
#endif /* MV_ETH_PNC_NEW */

struct tcam_entry *tcam_sw_alloc(unsigned int section);
void tcam_sw_free(struct tcam_entry *te);
int tcam_sw_dump(struct tcam_entry *te, char *buf);
void tcam_sw_clear(struct tcam_entry *te);

void tcam_sw_set_port(struct tcam_entry *te, unsigned int port, unsigned int mask);
void tcam_sw_get_port(struct tcam_entry *te, unsigned int *port, unsigned int *mask);

void tcam_sw_set_lookup_all(struct tcam_entry *te);
void tcam_sw_set_lookup(struct tcam_entry *te, unsigned int lookup);
void tcam_sw_get_lookup(struct tcam_entry *te, unsigned int *lookup, unsigned int *mask);
void tcam_sw_set_ainfo(struct tcam_entry *te, unsigned int bits, unsigned int mask);
void tcam_sw_set_byte(struct tcam_entry *te, unsigned int offset, unsigned char data);

int  tcam_sw_cmp_byte(struct tcam_entry *te, unsigned int offset, unsigned char data);
int  tcam_sw_cmp_bytes(struct tcam_entry *te, unsigned int offset, unsigned int size, unsigned char *data);

void tcam_sw_set_mask(struct tcam_entry *te, unsigned int offset, unsigned char mask);
void sram_sw_set_rinfo(struct tcam_entry *te, unsigned int rinfo, unsigned int mask);
void sram_sw_set_shift_update(struct tcam_entry *te, unsigned int index, unsigned int value);
void sram_sw_set_rxq(struct tcam_entry *te, unsigned int rxq, unsigned int force);

unsigned int sram_sw_get_rxq(struct tcam_entry *te, unsigned int *force);

void sram_sw_set_next_lookup_shift(struct tcam_entry *te, unsigned int index);
void sram_sw_set_lookup_done(struct tcam_entry *te, unsigned int value);
void sram_sw_set_next_lookup_shift(struct tcam_entry *te, unsigned int value);
void sram_sw_set_ainfo(struct tcam_entry *te, unsigned int bits, unsigned int mask);
void sram_sw_set_next_lookup(struct tcam_entry *te, unsigned int lookup);
void sram_sw_set_flowid(struct tcam_entry *te, unsigned int flowid, unsigned int mask);
void sram_sw_set_flowid_partial(struct tcam_entry *te, unsigned int flowid, unsigned int idx);
void tcam_sw_text(struct tcam_entry *te, char *text);
int tcam_hw_write(struct tcam_entry *te, int tid);
int tcam_hw_read(struct tcam_entry *te, int tid);
void tcam_hw_inv(int tid);
void tcam_hw_inv_all(void);
void tcam_hw_debug(int);
int tcam_hw_dump(int);
int tcam_hw_hits(char *buf);
void tcam_hw_record(int);
int tcam_hw_init(void);

#endif

