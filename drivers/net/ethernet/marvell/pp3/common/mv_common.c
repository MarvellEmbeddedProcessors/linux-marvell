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
#include "mv_sw_if.h"

int mv_pp3_max_check(int value, int limit, char *name)
{
	if ((value < 0) || (value >= limit)) {
		pr_err("%s %d is out of range [0..%d]\n",
			name ? name : "value", value, (limit - 1));
		return 1;
	}
	return 0;
}

const char *mv_pp3_pkt_mode_str(enum mv_pp3_pkt_mode mode)
{
	const char *pkt_mode_str;

	switch (mode) {
	case MV_PP3_PKT_DRAM:
		pkt_mode_str = "DRAM     ";
		break;

	case MV_PP3_PKT_CFH:
		pkt_mode_str = "CFH      ";
		break;
	default:
		pkt_mode_str = "Unknown  ";
	}
	return pkt_mode_str;
}

const char *mv_hash_type_str(enum mv_hash_type hash)
{
	const char *hash_str;

	switch (hash) {
	case MV_HASH_NONE:
		hash_str = "None     ";
		break;
	case MV_HASH_SA:      /* MAC SA */
		hash_str = "MAC_SA   ";
		break;
	case MV_HASH_2_TUPLE: /* SIP + DIP */
		hash_str = "2-tuples ";
		break;
	case MV_HASH_4_TUPLE: /* SIP + DIP + SPort + DPort */
		hash_str = "4-tuples ";
		break;
	default:
		hash_str = "Unknown  ";
	}
	return hash_str;
}

const char *mv_port_mode_str(enum mv_port_mode mode)
{
	const char *mode_str;

	switch (mode) {
	case MV_PORT_RXAUI:
		mode_str = "RXAUI    ";
		break;
	case MV_PORT_XAUI:
		mode_str = "XAUI     ";
		break;
	case MV_PORT_SGMII:
		mode_str = "SGMII    ";
		break;
	case MV_PORT_SGMII2_5:
		mode_str = "SGMII_2.5";
		break;
	case MV_PORT_QSGMII:
		mode_str = "QSGMII   ";
		break;
	case MV_PORT_RGMII:
		mode_str = "RGMII    ";
		break;
	default:
		mode_str = "Unknown  ";
	}
	return mode_str;
}

void mv_link_to_str(struct mv_port_link_status status, char *str)
{
	char *speed_str;
	char *duplex_str;

	if (!status.linkup) {
		sprintf(str, "Link is Down");
		return;
	}

	/* update speed string */
	switch (status.speed) {
	case MV_PORT_SPEED_10:
		speed_str = "10";
		break;
	case MV_PORT_SPEED_100:
		speed_str = "100";
		break;
	case MV_PORT_SPEED_1000:
		speed_str = "1000";
		break;
	case MV_PORT_SPEED_2000:
		speed_str = "2500";
		break;
	case MV_PORT_SPEED_10000:
		speed_str = "10000";
		break;
	default:
		speed_str = "unknown speed";
	}
	/* update duplex string */
	switch (status.duplex) {
	case MV_PORT_DUPLEX_HALF:
		duplex_str = "Half";
		break;
	case MV_PORT_DUPLEX_FULL:
		duplex_str = "Full";
		break;
	default:
		duplex_str = "unknown";
	}

	sprintf(str, "Link is Up %s Mbps %s Duplex\n", speed_str, duplex_str);
}
/*
Description:
	Dump memory in specific format:
	address: X1X1X1X1 X2X2X2X2 ... X8X8X8X8
Inputs :
	addr - buffer address
	size - num of bytes to dump
	access - width of read access
*/

void mv_debug_mem_dump(void *addr, int size, int access)
{
	int i, j;
	u32 mem_addr = (u32) addr;

	if (access == 0)
		access = 1;

	if ((access != 4) && (access != 2) && (access != 1)) {
		pr_info("%d wrong access size. Access must be 1 or 2 or 4\n", access);
		return;
	}

	mem_addr = MV_ALIGN_DOWN((unsigned int)addr, 4);
	size = MV_ALIGN_UP(size, 4);
	addr = (void *)MV_ALIGN_DOWN((unsigned int)addr, access);
	while (size > 0) {
		pr_info("%08x: ", mem_addr);
		i = 0;
		/* 32 bytes in the line */
		while (i < 32) {
			if (mem_addr >= (u32) addr) {
				switch (access) {
				case 1:
					pr_cont("%02x ", ((*((unsigned char *)(mem_addr)))));
					break;

				case 2:
					pr_cont("%04x ", ((*((unsigned short *)(mem_addr)))));
					break;

				case 4:
					pr_cont("%08x ", ((*((unsigned int *)(mem_addr)))));
					break;
				}
			} else {
				for (j = 0; j < (access * 2 + 1); j++)
					pr_cont(" ");
			}
			i += access;
			mem_addr += access;
			size -= access;
			if (size <= 0)
				break;
		}
	}
	pr_cont("\n");
}


/*
Description:
	Allocate number of memory buffers of request size.
Inputs:
	size - buffer size in bytes
	buf_num - number of buffers to allocate
	buff - pointer to array of buffers pointers
Return:
	true on success
	false on fail
*/
static bool mv_bufs_alloc(int buf_num, int size, unsigned int *buff)
{
	int i, j;

	for (i = 0; i < buf_num; i++) {
		buff[i] = (unsigned int) kzalloc(size, GFP_KERNEL);
		if (buff[i] == 0)
			break;
	}
	if (i < buf_num) {
		/* error through allocation */
		for (j = 0 ; j < i; j++)
			kfree((unsigned int *)buff[j]);
		return false;
	}
	return true;
}

/*
Description:
	Allocate memory buffer of request size.
	If there is no one buffer of requested size, try to allocate smaller buffers.
	Start from 2 buffers and continue with power of 2 buffers number.
	Stop if buffers size goes less than linux PAGE_SIZE.
Inputs:
	req_size - buffer size in kbytes
	max_bufs_num - max number of buffers to allocate
	buff - pointer to array of buffers pointers
Return:
	number of allocated buffers or
	0 for fail
*/
int mv_memory_buffer_alloc(unsigned int req_size, int max_bufs_num, unsigned int *buff)
{
	unsigned int cur_size, buf_num;

	/* cur_size in bytes */
	cur_size = req_size * 1024;
	buf_num = 1;

	while (buf_num <= max_bufs_num) {

		/* cur_size aligned to PAGE_SIZE */
		cur_size = MV_ALIGN_UP(cur_size, PAGE_SIZE);
		if (mv_bufs_alloc(buf_num, cur_size, buff))
			return buf_num;

		cur_size = cur_size / 2;
		if (cur_size < PAGE_SIZE)
			/* min buffer size is linux page size  (4KB) */
			return 0;

		buf_num = buf_num * 2;
	}

	return 0;
}
EXPORT_SYMBOL(mv_memory_buffer_alloc);

/*
Description:
 read field from entry
 requested filed cannot cross words in the entry
Inputs:
	offs  - field start offset in bits
	bits  - field size in bits
	entry - entry memory
return:
	field value
*/

unsigned int mv_field_get(int offs, int bits,  unsigned int *entry)
{
	int word, offs_in_word;
	unsigned int maks = (1 << bits) - 1;

	word = offs / 32;
	offs_in_word = (offs % 32);

	return (entry[word] >> offs_in_word) & maks;
}
/*
Description:
 write field to entry
 filed cannot cross words in the entry
Inputs:
	offs  - field start offset in bits
	bits  - field size in bits
	entry - entry memory
	val - field value
return:
	void
*/

void mv_field_set(int offs, int bits, unsigned int *entry,  unsigned int val)
{
	int word, offs_in_word;
	unsigned int maks = (1 << bits) - 1;

	word = offs / 32;
	offs_in_word = (offs % 32);

	/* clear field */
	entry[word] &= ~(maks << offs_in_word);
	/* set new field */
	entry[word] |= (val << offs_in_word);
}

