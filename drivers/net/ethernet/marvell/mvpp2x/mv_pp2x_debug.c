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

#include <linux/kernel.h>
#include <linux/netdevice.h>
#include <linux/etherdevice.h>
#include <linux/platform_device.h>
#include <linux/skbuff.h>

#include "mv_pp2x.h"
#include "mv_pp2x_hw.h"
#include "mv_pp2x_debug.h"

int mv_pp2x_debug_param_set(u32 param)
{
	debug_param = param;
	return 0;
}
EXPORT_SYMBOL(mv_pp2x_debug_param_set);

int mv_pp2x_debug_param_get(void)
{
	return debug_param;
}
EXPORT_SYMBOL(mv_pp2x_debug_param_get);

/* Extra debug */
void mv_pp2x_skb_dump(struct sk_buff *skb, int size, int access)
{
	int i, j;
	void *addr = skb->head + NET_SKB_PAD;
	uintptr_t mem_addr = (uintptr_t)addr;

	DBG_MSG("skb=%p, buf=%p, ksize=%d\n", skb, skb->head,
		(int)ksize(skb->head));

	if (access == 0)
		access = 1;

	if ((access != 4) && (access != 2) && (access != 1)) {
		pr_err("%d wrong access size. Access must be 1 or 2 or 4\n",
		       access);
		return;
	}
	mem_addr = round_down((uintptr_t)addr, 4);
	size = round_up(size, 4);
	addr = (void *)round_down((uintptr_t)addr, access);

	while (size > 0) {
		DBG_MSG("%08lx: ", mem_addr);
		i = 0;
		/* 32 bytes in the line */
		while (i < 32) {
			if (mem_addr >= (uintptr_t)addr) {
				switch (access) {
				case 1:
					DBG_MSG("%02x ",
						ioread8((void *)mem_addr));
					break;

				case 2:
					DBG_MSG("%04x ",
						ioread16((void *)mem_addr));
					break;

				case 4:
					DBG_MSG("%08x ",
						ioread32((void *)mem_addr));
					break;
				}
			} else {
				for (j = 0; j < (access * 2 + 1); j++)
					DBG_MSG(" ");
			}
			i += access;
			mem_addr += access;
			size -= access;
			if (size <= 0)
				break;
		}
		DBG_MSG("\n");
	}
}

