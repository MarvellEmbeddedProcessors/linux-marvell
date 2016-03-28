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

#ifndef __mv_sw_if_h__
#define __mv_sw_if_h__

#include <linux/kernel.h>
#include <linux/io.h>
#include <linux/errno.h>
#include <linux/slab.h>
#include <linux/module.h>
#include <linux/netdevice.h>
#include <linux/interrupt.h>
#include <linux/if_vlan.h>
#include <linux/platform_device.h>


#define __ATTRIBUTE_PACKED__	__packed
#define MV_MALLOC	kmalloc

#define	MV_MAC_ADDR_SIZE	(6)
#define MV_MAC_STR_SIZE		(20)
#define MV_MH_SIZE		(2)
#define MV_CRC_SIZE		(4)
#define MV_DSCP_NUM		(64)

#define MV_MTU_MIN		(68)
/* Layer2 packet info EtherType + Double VLAN + MAC_SA + MAC_DA + Marvell header */
#define MV_L2_HLEN		(MV_MH_SIZE + 2 * VLAN_HLEN + ETH_HLEN)

/* MTU = MRU - MV_L2_SIZE */
#define MV_MTU_MAX		((10 * 1024) - MV_L2_HLEN)
#define MV_EXT_PORT_MTU		(7904)

#define MV_MAC_IS_EQUAL(_mac1, _mac2)			\
	(!memcmp((_mac1), (_mac2), MV_MAC_ADDR_SIZE))


/* MTU + EtherType + Double VLAN + MAC_SA + MAC_DA + Marvell header */
#define MV_MAX_PKT_SIZE(mtu)	((mtu) + MV_L2_HLEN)

#define MV_RX_PKT_SIZE(mtu)	SKB_DATA_ALIGN(MV_MAX_PKT_SIZE(mtu) + ETH_FCS_LEN)


#ifdef CONFIG_MV_PP3_STAT_ERR
#define STAT_ERR(c) c
#else
#define STAT_ERR(c)
#endif

#ifdef CONFIG_MV_PP3_STAT_INF
#define STAT_INFO(c) c
#else
#define STAT_INFO(c)
#endif

#ifdef CONFIG_MV_PP3_STAT_DBG
#define STAT_DBG(c) c
#else
#define STAT_DBG(c)
#endif

/******************************************************
 * interrupt control --                               *
 ******************************************************/
#define MV_TRYLOCK(lock, flags)                               \
	(in_interrupt() ? spin_trylock((lock)) :              \
		spin_trylock_irqsave((lock), (flags)))

#define MV_LOCK(lock, flags)					\
do {								\
	if (in_interrupt())					\
		spin_lock((lock));				\
	else							\
		spin_lock_irqsave((lock), (flags));		\
} while (0)

#define MV_UNLOCK(lock, flags)					\
do {								\
	if (in_interrupt())					\
		spin_unlock((lock));				\
	else							\
		spin_unlock_irqrestore((lock), (flags));	\
} while (0)

#define MV_LIGHT_LOCK(flags)					\
do {								\
	if (!in_interrupt())					\
		local_irq_save(flags);				\
} while (0)

#define MV_LIGHT_UNLOCK(flags)					\
do {								\
	if (!in_interrupt())					\
		local_irq_restore(flags);			\
} while (0)

/* resource lock depend on usage - used by any cpu or dedicate to specific one */
#define MV_RES_LOCK(share_res, lock, flags)	\
do {						\
	if (share_res)				\
		MV_LOCK(lock, flags);		\
	else					\
		MV_LIGHT_LOCK(flags);		\
} while (0)

#define MV_RES_UNLOCK(share_res, lock, flags)	\
do {						\
	if (share_res)				\
		MV_UNLOCK(lock, flags);		\
	else					\
		MV_LIGHT_UNLOCK(flags);		\
} while (0)

/******************************************************
 * align memory allocateion                           *
 ******************************************************/
/* Macro for testing aligment. Positive if number is NOT aligned   */
#define MV_IS_NOT_ALIGN(number, align)      ((number) & ((align) - 1))

/* Macro for alignment up. For example, MV_ALIGN_UP(0x0330, 0x20) = 0x0340   */
#define MV_ALIGN_UP(number, align)                             \
(((number) & ((align) - 1)) ? (((number) + (align)) & ~((align)-1)) : (number))

/* Macro for alignment down. For example, MV_ALIGN_UP(0x0330, 0x20) = 0x0320 */
#define MV_ALIGN_DOWN(number, align) ((number) & ~((align)-1))

/* Return mask of all ones for any number of bits less than 32 */
#define MV_ALL_ONES_MASK(bits)	((1 << (bits)) - 1)

/* Check that num is power of 2 */
#define MV_IS_POWER_OF_2(num) ((num != 0) && ((num & (num - 1)) == 0))

/* Sets the field located at the specified in data.     */
#define MV_U32_SET_FIELD(data, mask, val)		((data) = (((data) & ~(mask)) | (val)))

/* Returns value with bitNum Set or Clear */
#define MV_SET_BIT(word, bitNum, bitVal) (((word) & ~(1 << (bitNum))) | (bitVal << bitNum))
#define MV_GET_BIT(word, bitNum)        (((word) & (1 << (bitNum))) >> (bitNum))

/* QM/BM related */
#define MV_MIN(a , b) (((a) < (b)) ? (a) : (b))
#define MV_MAX(a , b) (((a) > (b)) ? (a) : (b))

#define MV_UNIT_OF__8_BYTES	8
#define MV_UNIT_OF_64_BYTES	64

#define MV_32_BITS		32
#define MV_40_BITS		40
#define MV_WORD_BITS		32
#define MV_BYTE_BITS		8

/* Error definitions*/
/* Error Codes */

#define MV_OK			0
#define MV_ON			1
#define MV_OFF			0


/******************************************************
 * validation macros                                  *
 ******************************************************/
#define MV_PP3_NULL_PTR(_ptr_, _label_)	\
do {					\
	if (!(_ptr_)) {			\
		pr_err("%s: Error - unexpected NULL pointer\n", __func__);\
		goto _label_;		\
	}				\
} while (0)

/* port related */
enum mv_reset {RESET, UNRESET};

enum mv_port_mode {
	MV_PORT_RXAUI,
	MV_PORT_XAUI,
	MV_PORT_SGMII,
	MV_PORT_SGMII2_5,
	MV_PORT_QSGMII,
	MV_PORT_RGMII
};

enum mv_port_speed {
	MV_PORT_SPEED_AN,
	MV_PORT_SPEED_10,
	MV_PORT_SPEED_100,
	MV_PORT_SPEED_1000,
	MV_PORT_SPEED_2000,
	MV_PORT_SPEED_10000
};

enum mv_port_duplex {
	MV_PORT_DUPLEX_AN,
	MV_PORT_DUPLEX_HALF,
	MV_PORT_DUPLEX_FULL
};

enum mv_port_fc {
	MV_PORT_FC_AN_NO,
	MV_PORT_FC_AN_SYM,
	MV_PORT_FC_AN_ASYM,
	MV_PORT_FC_DISABLE,
	MV_PORT_FC_ENABLE,
	MV_PORT_FC_ACTIVE
};

struct mv_port_link_status {
	int			linkup; /*flag*/
	enum mv_port_speed	speed;
	enum mv_port_duplex	duplex;
	enum mv_port_fc		rx_fc;
	enum mv_port_fc		tx_fc;
};

/* different loopback types can be configure on different levels: MAC, PCS, SERDES */
enum mv_lb_type {
	MV_DISABLE_LB,
	MV_RX_2_TX_LB,
	MV_TX_2_RX_LB,         /* on SERDES level - analog loopback */
	MV_TX_2_RX_DIGITAL_LB  /* on SERDES level - digital loopback */
};

enum mv_sched_type {
	MV_SCHED_STRICT,
	MV_SCHED_WRR,
	MV_SCHED_INV
};

enum mv_priority_type {
	MV_PRIO_INV = 0,
	MV_PRIO_LOW,
	MV_PRIO_HIGH,
	MV_PRIO_SPEC
};

/* QoS profile configuration modes */
enum mv_qos_mode {
	MV_QOS_MODE_DISABLED = 0,
	MV_QOS_DSCP_VPRIO,
	MV_QOS_VPRIO_DSCP,
	MV_QOS_MODE_LAST
};

/* HASH types */
enum mv_hash_type {
	MV_HASH_NONE = 0,
	MV_HASH_SA,	 /* MAC SA */
	MV_HASH_2_TUPLE, /* SIP + DIP */
	MV_HASH_4_TUPLE, /* SIP + DIP + SPort + DPort */
	MV_HASH_LAST
};

/* PP3 packet modes */
enum mv_pp3_pkt_mode {
	MV_PP3_PKT_DRAM,    /* All packet data store in DRAM buffer, no data in CFH */
	MV_PP3_PKT_CFH,     /* Packet Data split between CFH to DRAM, first 64 bytes store in CFH */
	MV_PP3_PKT_LAST
};

enum mv_state {
	MV_STATE_DISABLE = 0,
	MV_STATE_ENABLE,
	MV_STATE_LAST,
};

/* store here all MC MACs sent to FW */
struct mac_mc_info {
	struct list_head head;
	u8	mac[MV_MAC_ADDR_SIZE];
};

struct mv_io_addr {
	phys_addr_t	paddr;
	void __iomem	*vaddr;
	size_t		size;
};
struct mv_a40 {
	u32 virt_lsb;                   /* byte[ 0-11] ,bit[ 0-31] */
	u32 dma_lsb;                    /* byte[ 0-11] ,bit[32-63] */
	u8  virt_msb;                   /* byte[ 0-11] ,bit[64-71] */
	u8  dma_msb;                    /* byte[ 0-11] ,bit[72-79] */
	u8 _reserved[2];                /* byte[ 0-11] ,bit[80-95] */
};

struct mv_unit_info {
	void __iomem	*base_addr; /* unit base address = silicon addr + unit offset */
	u32		ins_offs;  /* unit instance offset - for multiple units */
};

struct mv_mac_data {
	/* Whether a PHY is present, and if yes, at which address. */
	int			phy_addr;
	enum mv_port_mode	port_mode;
	int			link_irq;
	bool			force_link;
};

enum mv_nss_port_type {
	MV_PP3_NSS_PORT_ETH, /* EMAC virtual port	   */
	MV_PP3_NSS_PORT_CPU, /* CPU virtual port	   */
	MV_PP3_NSS_PORT_EXT, /* NSS virtual port	   */
	MV_PP3_NSS_PORT_LAST
};

/* Invalid virtual port definition */
#define MV_PP3_NSS_PORT_INV	MV_PP3_NSS_PORT_LAST


/* CFH common fields definitions */
/* CFH word 0 */
#define MV_HOST_MSG_PACKET_LENGTH(l)	((l) & 0xFFFF)
#define MV_HOST_MSG_DESCR_MODE		(2 << 30)
/* CFH word 1 */
#define MV_HOST_MSG_CFH_LENGTH(l)	(((l) & 0xFF) << 16)
/* CFH word 2 */
#define MV_HOST_MSG_CHAN_ID(l)		(((l) & 0xFF) << 16)
/* CFH word 5 */
#define MV_HOST_MSG_BPID(l)		(((l) & 0xFF) << 24)


/* convert one char symbol to 8 bit interger hex format */
static inline unsigned char char_to_hex(char msg)
{
	unsigned char tmp = 0;

	if ((msg >= '0') && (msg <= '9'))
		tmp = msg - '0';
	else if ((msg >= 'a') && (msg <= 'f'))
		tmp = msg - 'a' + 10;
	else if ((msg >= 'A') && (msg <= 'F'))
		tmp = msg - 'A' + 10;

	return tmp;
}

static inline void mv_mac_addr_print(char *prefix, unsigned char *mac_addr, char *suffix)
{
	pr_info("%s %02x:%02x:%02x:%02x:%02x:%02x %s\n", prefix ? prefix : "",
		mac_addr[0], mac_addr[1], mac_addr[2],
		mac_addr[3], mac_addr[4], mac_addr[5], suffix ? suffix : "");
}

/* convert asci string of known size to 8 bit interger hex format array */
static inline void str_to_hex(char *msg, int size, unsigned char *imsg, int new_size)
{
	int i, j;
	unsigned char tmp;

	for (i = 0, j = 0; j < new_size; i = i + 2, j++) {
		/* build high byte nible */
		tmp = (char_to_hex(msg[i]) << 4);
		/* build low byte nible */
		tmp += char_to_hex(msg[i+1]);
		imsg[j] = tmp;
	}
}

/* convert u32 value from BE to cpu native */
static inline void mv_be32_convert(u32 *ptr, int size)
{
	int i;
	for (i = 0; i < size; i++, ptr++)
		*ptr = be32_to_cpu(*ptr);
}

/* convert mac address in format xx:xx:xx:xx:xx:xx to array of unsigned char [6] */
static inline void mv_mac_str2hex(const char *mac_str, u8 *mac_hex)
{
	int i;
	char tmp[3];
	u8 tmp1;

	for (i = 0; i < 6; i++) {
		tmp[0] = mac_str[(i * 3) + 0];
		tmp[1] = mac_str[(i * 3) + 1];
		tmp[2] = '\0';
		str_to_hex(tmp, 3, &tmp1, 1);
		mac_hex[i] = tmp1;
	}
	return;
}
static inline void mv_u32_memcpy(u32 *dst, u32 *src, int bytes)
{
	int i, words;

	/* copy whole number of words */
	words = bytes >> 2;
	for (i = 0; i < words; i++)
		*dst++ = *src++;

	/* copy rest bytes if needed */
	bytes = bytes - (words << 2);
	if (bytes) {
		u8 *src_u8 = (u8 *)src;
		u8 *dst_u8 = (u8 *)dst;
		switch (bytes) {
		case 1:
			dst_u8[0] = src_u8[0];
			break;
		case 2:
			dst_u8[0] = src_u8[0];
			dst_u8[1] = src_u8[1];
			break;

		case 3:
			dst_u8[0] = src_u8[0];
			dst_u8[1] = src_u8[1];
			dst_u8[2] = src_u8[2];
			break;
		}
	}
}
/******************************************************
 * common functions				      *
 ******************************************************/
void mv_debug_mem_dump(void *addr, int size, int access);
unsigned int mv_field_get(int offs, int bits,  unsigned int *entry);
void mv_field_set(int offs, int bits, unsigned int *entry,  unsigned int val);
const char *mv_port_mode_str(enum mv_port_mode mode);
const char *mv_hash_type_str(enum mv_hash_type hash);
int mv_memory_buffer_alloc(unsigned int req_size, int max_bufs_num, unsigned int *buff);
const char *mv_pp3_pkt_mode_str(enum mv_pp3_pkt_mode mode);
int mv_pp3_max_check(int value, int limit, char *name);
void mv_link_to_str(struct mv_port_link_status status, char *str);


#endif /* __mv_sw_if_h__ */
