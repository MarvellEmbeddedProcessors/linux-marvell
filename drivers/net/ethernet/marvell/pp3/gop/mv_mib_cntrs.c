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
#include "common/mv_sw_if.h"
#include "platform/mv_pp3_defs.h"
#include "gop/mv_gop_if.h"
#include "mv_mib_regs.h"


static u32 pp3_port_mib_shadow[MV_PP3_GOP_MAC_NUM][MV_PP3_MIB_LATE_COLLISION/sizeof(u32)+1];

/*******************************************************************************
* mv_pp3_mib_counter_read - Read a MIB counter
*
* DESCRIPTION:
*       This function reads a MIB counter of a specific ethernet port.
*       NOTE - Read from MV_PP3_MIB_GOOD_OCTETS_RECEIVED_LOW or
*              MV_PP3_MIB_GOOD_OCTETS_SENT_LOW counters will return 64 bits value,
*              so p_high_32 pointer should not be NULL in this case.
*
* INPUT:
*       port     - Ethernet Port number.
*       offset   - MIB counter offset.
*
* OUTPUT:
*       p_high_32 - pointer to place where 32 most significant bits
*                   of the counter will be stored.
*
* RETURN:
*       32 low significant bits of MIB counter value.
*
*******************************************************************************/
static u32 pp3_gop_mib_counter_read(int port, unsigned int offset, u32 *p_high_32)
{
	u32 val_low_32, val_high_32;
	int abs_offset;

	val_low_32 = mv_gop_reg_read(MV_PP3_MIB_COUNTERS_BASE(port) + offset);
	pp3_port_mib_shadow[port][offset/sizeof(u32)] += val_low_32;

	/* Implement FEr ETH. Erroneous Value when Reading the Upper 32-bits    */
	/* of a 64-bit MIB Counter.                                             */
	if ((offset == MV_PP3_MIB_GOOD_OCTETS_RECEIVED_LOW) || (offset == MV_PP3_MIB_GOOD_OCTETS_SENT_LOW)) {
		abs_offset = MV_PP3_MIB_COUNTERS_BASE(port) + offset + 4;

		val_high_32 = mv_gop_reg_read(abs_offset);
		pp3_port_mib_shadow[port][(offset + 4)/sizeof(u32)] += val_high_32;

		if (p_high_32 != NULL)
			*p_high_32 = pp3_port_mib_shadow[port][(offset + 4)/sizeof(u32)];
	}
	return pp3_port_mib_shadow[port][offset/sizeof(u32)];
}

/*******************************************************************************
* mv_pp3_mib_counters_clear - Clear all MIB counters
*
* DESCRIPTION:
*       This function clears all MIB counters
*
* INPUT:
*       port      - Ethernet Port number.
*
* RETURN:   void
*
*******************************************************************************/
void mv_pp3_gop_mib_counters_clear(int port)
{
	int i, abs_offset;

	if (port >= MV_PP3_GOP_MAC_NUM) {
		pr_err("%s: illegal port number %d", __func__, port);
		return;
	}

	/* clear counters shadow */
	memset(pp3_port_mib_shadow[port], 0, MV_PP3_MIB_LATE_COLLISION);
	/* Perform dummy reads from MIB counters */
	/* Read of last counter clear all counter were read before */
	for (i = MV_PP3_MIB_GOOD_OCTETS_RECEIVED_LOW; i <= MV_PP3_MIB_LATE_COLLISION; i += 4) {
		abs_offset = MV_PP3_MIB_COUNTERS_BASE(port) + i;
		mv_gop_reg_read(abs_offset);
	}
}

static void pp3_mib_print(int port, u32 offset, char *mib_name)
{
	u32 reg_low, reg_high = 0;

	reg_low = pp3_gop_mib_counter_read(port, offset, &reg_high);

	if (!reg_high)
		pr_info("  %-32s: 0x%02x = %u\n", mib_name, offset, reg_low);
	else
		pr_info("  %-32s: 0x%02x = 0x%08x%08x\n", mib_name, offset, reg_high, reg_low);

}

/*******************************************************************************
* mv_pp3_gop_mib_counter_get - return a MIB counter
*
* DESCRIPTION:
*       This function reads a MIB counter of a specific ethernet port.
*       NOTE - Read from MV_PP3_MIB_GOOD_OCTETS_RECEIVED_LOW or
*              MV_PP3_MIB_GOOD_OCTETS_SENT_LOW counters will return 64 bits value,
*              so p_high_32 pointer should not be NULL in this case.
*
* INPUT:
*       port     - Ethernet Port number.
*       offset   - MIB counter offset.
*
* OUTPUT:
*       p_high_32 - pointer to place where 32 most significant bits
*                   of the counter will be stored.
*
* RETURN:
*       32 low significant bits of MIB counter value.
*
*******************************************************************************/
u32 mv_pp3_gop_mib_counter_get(int port, unsigned int offset, u32 *p_high_32)
{
	int i;

	if (port >= MV_PP3_GOP_MAC_NUM) {
		pr_err("%s: illegal port number %d", __func__, port);
		return 0;
	}

	/* Read all MIB counters */
	for (i = MV_PP3_MIB_GOOD_OCTETS_RECEIVED_LOW; i <= MV_PP3_MIB_LATE_COLLISION; i += 4)
		pp3_gop_mib_counter_read(port, i, p_high_32);

	/* return specific counter value from shadow */
	if ((offset == MV_PP3_MIB_GOOD_OCTETS_RECEIVED_LOW) || (offset == MV_PP3_MIB_GOOD_OCTETS_SENT_LOW)) {
		if (p_high_32 != NULL)
			*p_high_32 = pp3_port_mib_shadow[port][(offset + 4)/sizeof(u32)];
	}
	return pp3_port_mib_shadow[port][offset/sizeof(u32)];
}

/* Print MIB counters of the Ethernet port */
void mv_pp3_gop_mib_counters_show(int port)
{
	if (port >= MV_PP3_GOP_MAC_NUM) {
		pr_err("%s: illegal port number %d", __func__, port);
		return;
	}

	pr_info("\nMIBs: port=%d, base=0x%x\n", port, MV_PP3_MIB_COUNTERS_BASE(port));

	pr_info("\n[Rx]\n");
	pp3_mib_print(port, MV_PP3_MIB_GOOD_OCTETS_RECEIVED_LOW, "GOOD_OCTETS_RECEIVED");
	pp3_mib_print(port, MV_PP3_MIB_BAD_OCTETS_RECEIVED, "BAD_OCTETS_RECEIVED");

	pp3_mib_print(port, MV_PP3_MIB_UNICAST_FRAMES_RECEIVED, "UNCAST_FRAMES_RECEIVED");
	pp3_mib_print(port, MV_PP3_MIB_BROADCAST_FRAMES_RECEIVED, "BROADCAST_FRAMES_RECEIVED");
	pp3_mib_print(port, MV_PP3_MIB_MULTICAST_FRAMES_RECEIVED, "MULTICAST_FRAMES_RECEIVED");

	pr_info("\n[RMON]\n");
	pp3_mib_print(port, MV_PP3_MIB_FRAMES_64_OCTETS, "FRAMES_64_OCTETS");
	pp3_mib_print(port, MV_PP3_MIB_FRAMES_65_TO_127_OCTETS, "FRAMES_65_TO_127_OCTETS");
	pp3_mib_print(port, MV_PP3_MIB_FRAMES_128_TO_255_OCTETS, "FRAMES_128_TO_255_OCTETS");
	pp3_mib_print(port, MV_PP3_MIB_FRAMES_256_TO_511_OCTETS, "FRAMES_256_TO_511_OCTETS");
	pp3_mib_print(port, MV_PP3_MIB_FRAMES_512_TO_1023_OCTETS, "FRAMES_512_TO_1023_OCTETS");
	pp3_mib_print(port, MV_PP3_MIB_FRAMES_1024_TO_MAX_OCTETS, "FRAMES_1024_TO_MAX_OCTETS");

	pr_info("\n[Tx]\n");
	pp3_mib_print(port, MV_PP3_MIB_GOOD_OCTETS_SENT_LOW, "GOOD_OCTETS_SENT");
	pp3_mib_print(port, MV_PP3_MIB_UNICAST_FRAMES_SENT, "UNICAST_FRAMES_SENT");
	pp3_mib_print(port, MV_PP3_MIB_MULTICAST_FRAMES_SENT, "MULTICAST_FRAMES_SENT");
	pp3_mib_print(port, MV_PP3_MIB_BROADCAST_FRAMES_SENT, "BROADCAST_FRAMES_SENT");
	pp3_mib_print(port, MV_PP3_MIB_CRC_ERRORS_SENT, "CRC_ERRORS_SENT");

	pr_info("\n[FC control]\n");
	pp3_mib_print(port, MV_PP3_MIB_FC_RECEIVED, "FC_RECEIVED");
	pp3_mib_print(port, MV_PP3_MIB_FC_SENT, "FC_SENT");

	pr_info("\n[Errors]\n");
	pp3_mib_print(port, MV_PP3_MIB_RX_FIFO_OVERRUN, "MV_PP3_MIB_RX_FIFO_OVERRUN");
	pp3_mib_print(port, MV_PP3_MIB_UNDERSIZE_RECEIVED, "UNDERSIZE_RECEIVED");
	pp3_mib_print(port, MV_PP3_MIB_FRAGMENTS_RECEIVED, "FRAGMENTS_RECEIVED");
	pp3_mib_print(port, MV_PP3_MIB_OVERSIZE_RECEIVED, "OVERSIZE_RECEIVED");
	pp3_mib_print(port, MV_PP3_MIB_JABBER_RECEIVED, "JABBER_RECEIVED");
	pp3_mib_print(port, MV_PP3_MIB_MAC_RECEIVE_ERROR, "MAC_RECEIVE_ERROR");
	pp3_mib_print(port, MV_PP3_MIB_BAD_CRC_EVENT, "BAD_CRC_EVENT");
	pp3_mib_print(port, MV_PP3_MIB_COLLISION, "COLLISION");
	/* This counter must be read last. Read it clear all the counters */
	pp3_mib_print(port, MV_PP3_MIB_LATE_COLLISION, "LATE_COLLISION");
}
