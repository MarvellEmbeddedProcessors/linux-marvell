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

#include <net/gnss/mv_nss_metadata.h>
#include <net/gnss/mv_nss_ops.h>
#include "vport/mv_pp3_cpu.h"
#include "mv_netdev_structs.h"


/*---------------------------------------------------------------------------*/
static inline struct mv_nss_ops *mv_pp3_gnss_ops_get(struct net_device *dev)
{
	if (dev) {
		struct pp3_dev_priv *dev_priv =  MV_PP3_PRIV(dev);

		if (dev_priv && dev_priv->cpu_shared)
			return dev_priv->cpu_shared->gnss_ops;
	}
	return NULL;
}
/*---------------------------------------------------------------------------*/
/*			GNSS WRAPPERS					     */
/*---------------------------------------------------------------------------*/
static inline struct sk_buff *mv_pp3_gnss_skb_alloc(struct net_device *dev, int pkt_size, gfp_t gfp_mask)
{
	struct sk_buff *skb;
	struct mv_nss_ops *gnss_ops = mv_pp3_gnss_ops_get(dev);

	if (gnss_ops && gnss_ops->alloc_skb)
		skb = gnss_ops->alloc_skb(pkt_size, GFP_DMA | gfp_mask);
	else
		skb = __dev_alloc_skb(pkt_size, GFP_DMA | gfp_mask);

	return skb;
}
/*---------------------------------------------------------------------------*/
static inline void mv_pp3_gnss_skb_free(struct net_device *dev, struct sk_buff *skb)
{
	struct mv_nss_ops *gnss_ops = mv_pp3_gnss_ops_get(dev);
	struct mv_gnss_metadata *pmdata = NULL;

	if (gnss_ops && gnss_ops->free_skb && gnss_ops->get_metadata_skb)
		pmdata =  gnss_ops->get_metadata_skb(skb);

	pmdata ? gnss_ops->free_skb(skb) : dev_kfree_skb_any(skb);
}
/*---------------------------------------------------------------------------*/
static inline int  mv_pp3_gnss_skb_receive(struct net_device *dev, struct sk_buff *skb)
{
	int status;
	struct mv_nss_ops *gnss_ops = mv_pp3_gnss_ops_get(dev);

	if (gnss_ops && gnss_ops->receive_skb) {
		skb->dev = dev;
		status = gnss_ops->receive_skb(skb);
	} else {
		skb->protocol = eth_type_trans(skb, dev);
		status =  netif_receive_skb(skb);
	}
	return status;
}
/*---------------------------------------------------------------------------*/
static inline u32 *mv_pp3_gnss_skb_mdata_get(struct net_device *dev, struct sk_buff *skb)
{
	struct mv_nss_metadata *pmdata = NULL;
	struct mv_nss_ops *gnss_ops = mv_pp3_gnss_ops_get(dev);

	if (gnss_ops && gnss_ops->get_metadata_skb)
		pmdata = gnss_ops->get_metadata_skb(skb);

	return (u32 *)pmdata;
}
