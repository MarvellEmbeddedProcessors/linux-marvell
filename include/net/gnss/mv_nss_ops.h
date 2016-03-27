/*
* ***************************************************************************
* Copyright (C) 2015 Marvell International Ltd.
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
/*  mv_nss_ops.h */

#ifndef __MV_NSS_OPS_H__
#define __MV_NSS_OPS_H__

#include <linux/types.h>
#include <linux/netdevice.h>
#include <linux/skbuff.h>



/*
 * struct mv_nss_if_ops
 *
 * Description:
 *       NSS compatible network device operations.
 *
 * Fields:
 *       recv_pause  - Stop traffic receive.
 *       recv_resume - Resume traffic receive.
 *       shutdown    - NSS shutdown notification.
 */
struct mv_nss_if_ops {
	void (*recv_pause)(struct net_device *iface, int cos);
	void (*recv_resume)(struct net_device *iface, int cos);
	void (*shutdown)(void);
};

/*
 * struct mv_gnss_ops
 *
 * Description:
 *       NSS common operations.
 *
 * Fields:
 *       alloc_skb        - Allocate new skb.
 *       free_skb         - Release skb.
 *       receive_skb      - Send skb for processing.
 *       get_metadata_skb - Retrieve pointer to packet metadata.
 *                          Metadata is valid if and only if
 *                          the returned pointer is non NULL.
 *       init_metadata    - Mark metadata associated with skb
 *                          as valid or invalid. If metadata
 *                          is marked as invalid, get_metadata_skb
 *                          would return NULL.
 *       xmit_pause       - Stop sending traffic to network device.
 *       xmit_resume      - Resume sending traffic to network device.
 *       register         - Register a compatible NSS network device
 *                          with fast path application.
 *       unregister -       Unregister a compatible NSS network device
 *                          from fast path application.
 */
struct mv_nss_ops {
	struct sk_buff* (*alloc_skb)(unsigned int size, gfp_t priority);
	void (*free_skb)(struct sk_buff *skb);
	int (*receive_skb)(struct sk_buff *skb);
	void* (*get_metadata_skb)(struct sk_buff *skb);
	void* (*init_metadata_skb)(struct sk_buff *skb); /*returns a pointer to on success*/
	void (*remove_metadata_skb)(struct sk_buff *skb);
	int (*xmit_pause)(struct net_device *iface, int cos);
	int (*xmit_resume)(struct net_device *iface, int cos);
	int (*register_iface)(struct net_device *iface, struct mv_nss_if_ops *if_ops);
	int (*unregister_iface)(struct net_device *iface);
};

/*
 * mv_nss_ops_set
 *
 * Description:
 *       Set common NSS operations by software fast path application.
 *
 * Parameters:
 *       ops - Pointer to common NSS operations.
 *
 * Returns:
 *    None.
 */

void mv_nss_ops_set(struct mv_nss_ops *ops);

/*
 * mv_nss_ops_get
 *
 * Description:
 *       Set common NSS operations by software fast path application.
 *
 * Parameters:
 *       None.
 *
 * Returns:
 *    Pointer to common NSS operations.
 */
struct mv_nss_ops *mv_nss_ops_get(const struct net_device *iface);


int mv_nss_ops_prefix_add(const char *name);
int mv_nss_ops_prefix_del(const char *name);
void mv_nss_ops_show(void);
void mv_nss_ops_prefix_list_show(void);
void mv_nss_ops_prefix_list_clear(void);
void mv_nss_ops_filter_on(bool filter);




#endif /* __MV_NSS_OPS_H__ */
