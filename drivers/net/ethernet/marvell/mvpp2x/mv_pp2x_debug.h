/*
* ***************************************************************************
* Copyright (C) 2016 Marvell International Ltd.
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

#ifndef _MVPP2_DEBUG_H_
#define _MVPP2_DEBUG_H_

#include <linux/kernel.h>
#include <linux/skbuff.h>

#define DBG_MSG(fmt, args...)	printk(fmt, ## args)

/* Macro for alignment up. For example, MV_ALIGN_UP(0x0330, 0x20) = 0x0340   */
#define MV_ALIGN_UP(number, align) (((number) & ((align) - 1)) ? \
				    (((number) + (align)) & ~((align) - 1)) : \
				    (number))

/* Macro for alignment down. For example, MV_ALIGN_UP(0x0330, 0x20) = 0x0320 */
#define MV_ALIGN_DOWN(number, align) ((number) & ~((align) - 1))

/* CPU architecture dependent 32, 16, 8 bit read/write IO addresses */
#define MV_MEMIO32_WRITE(addr, data)    \
	((*((unsigned int *)(addr))) = ((unsigned int)(data)))

#define MV_MEMIO32_READ(addr)           \
	((*((unsigned int *)(addr))))

#define MV_MEMIO16_WRITE(addr, data)    \
	((*((unsigned short *)(addr))) = ((unsigned short)(data)))

#define MV_MEMIO16_READ(addr)           \
	((*((unsigned short *)(addr))))

#define MV_MEMIO8_WRITE(addr, data)     \
	((*((unsigned char *)(addr))) = ((unsigned char)(data)))

#define MV_MEMIO8_READ(addr)            \
	((*((unsigned char *)(addr))))

void mv_pp2x_skb_dump(struct sk_buff *skb, int size, int access);

int mv_pp2x_debug_param_set(u32 param);

int mv_pp2x_debug_param_get(void);

#endif /* _MVPP2_DEBUG_H_ */
