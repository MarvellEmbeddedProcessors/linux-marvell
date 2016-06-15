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
#include "platform/mv_pp3.h"
#include "fw/mv_pp3_fw_msg.h"
#include "fw/mv_fw.h"
#include "net_dev/mv_dev_vq.h"
#include "net_dev/mv_netdev.h"

#include "mv_pp3_gnss_api.h"
#include <net/gnss/mv_nss_defs.h>

/* For external interfaces RX supported only on CPU0 */
#define MV_PP3_GNSS_DEV_RX_CPUS_MASK	0x1

static unsigned int mv_pp3_gnss_dev_rx_cpus_mask = MV_PP3_GNSS_DEV_RX_CPUS_MASK;
static unsigned int mv_pp3_gnss_rxq_num = CONFIG_MV_PP3_GNSS_RXQ_NUM;
static unsigned int mv_pp3_gnss_txq_num = CONFIG_MV_PP3_GNSS_TXQ_NUM;

int mv_pp3_gnss_sys_init(void)
{
	int err;

	if (pp3_device == NULL) {
		pr_err("PP3 driver is not probed yet\n");
		return -1;
	}
	if (mv_pp3_shared_initialized(pp3_device))
		return 0;

	rtnl_lock();
	err = mv_pp3_shared_start(pp3_device);
	rtnl_unlock();

	return err;
}
EXPORT_SYMBOL(mv_pp3_gnss_sys_init);

int mv_pp3_gnss_dev_rx_cpus_set(unsigned int rx_cpus_mask)
{
	mv_pp3_gnss_dev_rx_cpus_mask = rx_cpus_mask;
	return 0;
}

int mv_pp3_gnss_dev_rxqs_set(unsigned int rxqs)
{
	mv_pp3_gnss_rxq_num = rxqs;
	return 0;
}

int mv_pp3_gnss_dev_txqs_set(unsigned int txqs)
{
	mv_pp3_gnss_txq_num = txqs;
	return 0;
}

void mv_pp3_gnss_dev_init_show(void)
{
	pr_info("------- nssX network interfaces initialization parameters -------\n");
	pr_info("Number of RX VQs         : %u\n", mv_pp3_gnss_rxq_num);
	pr_info("Number of TX VQs         : %u\n", mv_pp3_gnss_txq_num);
	pr_info("CPUs mask                : 0x%u\n", mv_pp3_gnss_dev_rx_cpus_mask);
}

/*---------------------------------------------------------------------------
  mv_pp3_gnss_dev_create:
	description: Create networke device with name gnss%d where %d is vport
		     MTU, RXQs number and TXQs number are taken from globals defenititons

	input      : vport - virtual port id
		     state - network device state after creation

	return     : 0 for success, otherwise return negative integer
---------------------------------------------------------------------------*/
int mv_pp3_gnss_dev_create(unsigned short vport, bool state, unsigned char *mac)
{
	int rc = 0;
	char name[10];
	struct net_device *dev;
	struct pp3_dev_priv *dev_priv;
	int msec;

	/* Special name nss0 for network interface used for default gateway */
	if (vport == MV_NSS_EXT_PORT_MAX)
		sprintf(name, "nss%d", 0);
	else
		sprintf(name, "nss%d", vport);

	/* check MAC validation */
	if (mac && !is_valid_ether_addr(mac))
		return -EADDRNOTAVAIL;

	dev = mv_pp3_netdev_init(name, mv_pp3_gnss_rxq_num, mv_pp3_gnss_txq_num);
	if (!dev)
		goto oom;

	/* copy MAC */
	if (mac)
		memcpy(dev->dev_addr, mac, MV_MAC_ADDR_SIZE);

	dev->mtu = MV_EXT_PORT_MTU;
	dev_priv = MV_PP3_PRIV(dev);
	dev_priv->id = vport;

	if (mv_pp3_dev_rx_cpus_set(dev, mv_pp3_gnss_dev_rx_cpus_mask))
		pr_warn("%s: Can't set rx_cpus mask\n", dev->name);

	mv_pp3_netdev_show(dev);

	if (state) {
		rtnl_lock();
		/* Disconnect TX from Linux stack */
		netif_tx_stop_all_queues(dev);
		/* Open external device and alloc HW recources */
		rc = dev_open(dev);
		rtnl_unlock();
	}

	if (rc < 0)
		return rc;

	/* Init statistics extension if already set */
	msec = mv_pp3_gnss_ext_vport_msec_get();
	if (msec)
		rc = mv_pp3_gnss_ingress_vport_stats_init(vport, msec);

	return rc;
oom:
	pr_err("%s: Out of memory\n", __func__);
	return -ENOMEM;
}
EXPORT_SYMBOL(mv_pp3_gnss_dev_create);

/*---------------------------------------------------------------------------
  mv_pp3_gnss_dev_delete:
	description: delete networke device that attached to
		     virtual port (external or EMAC)
		     release all relevants HW resources and memories

	input      : vport - virtual port id

	return     : 0 for success, otherwise return negative integer
---------------------------------------------------------------------------*/
int mv_pp3_gnss_dev_delete(unsigned short vport)
{
	int err;
	struct pp3_dev_priv *dev_priv;
	struct net_device *dev = mv_pp3_vport_dev_get(vport);

	if (!dev) {
		pr_err("%s: network device for virtual port %d not exist\n",
				__func__,  vport);
		return -ENODEV;
	}

	dev_priv = MV_PP3_PRIV(dev);

	/* stop network decive */
	rtnl_lock();
	err = dev_close(dev);
	rtnl_unlock();

	if (err < 0) {
		pr_err("%s: failed to close network device %s\n", __func__, dev->name);
		return err;
	}

	mv_pp3_netdev_delete(dev);

	return 0;
}
EXPORT_SYMBOL(mv_pp3_gnss_dev_delete);

/*---------------------------------------------------------------------------
  mv_pp3_gnss_vport_type_get:
	description: get virtual port type
		     External APIs can use this function in order to know if
		     network device (for virtual port) created by driver or not.

	input      : vport - virtual port id

	return     : return virtual port type if netork device is exist,
		     Otherwise return MV_NSS_PORT_INV.
---------------------------------------------------------------------------*/
enum mv_nss_port_type mv_pp3_gnss_vport_type_get(unsigned short vport)
{
	struct pp3_dev_priv *dev_priv;
	struct net_device *dev = mv_pp3_vport_dev_get(vport);

	if (!dev)
		return MV_PP3_NSS_PORT_INV;

	dev_priv = MV_PP3_PRIV(dev);

	return dev_priv->vport->type;
}
EXPORT_SYMBOL(mv_pp3_gnss_vport_type_get);

/*---------------------------------------------------------------------------
  mv_pp3_gnss_vport_state_get:
	description: check virtual port state state

	input      : vport - virtual port id

	return     : fail    - return negative integer if virtual
			       port not created by driver
		     success - 0 if virtual port is disabled
			       1 if virtual port is enabled
---------------------------------------------------------------------------*/
int mv_pp3_gnss_vport_state_get(unsigned short vport)
{
	struct pp3_dev_priv *dev_priv;
	struct net_device *dev = mv_pp3_vport_dev_get(vport);

	if (!dev) {
		pr_err("%s: network device for virtual port %d not exist\n", __func__,  vport);
		return -ENODEV;
	}

	dev_priv = MV_PP3_PRIV(dev);

	return dev_priv->vport->state;
}
EXPORT_SYMBOL(mv_pp3_gnss_vport_state_get);

/*---------------------------------------------------------------------------
  mv_pp3_gnss_vport_state_set:
	description: set virtual port state in driver and FW
		     driver call to standard Linux dev_open/dev_stop function
		     for the network decive that connected to the virtual port.

	input      : vport - virtual port id

	return     : 0 for success, otherwise return negative integer
---------------------------------------------------------------------------*/
int mv_pp3_gnss_vport_state_set(unsigned short vport, bool enable)
{
	int ret_val;
	struct net_device *dev = mv_pp3_vport_dev_get(vport);

	if (!dev) {
		pr_err("%s: network device for virtual port %d not exist\n", __func__,  vport);
		return -ENODEV;
	}

	rtnl_lock();
	ret_val = enable ? dev_open(dev) : dev_close(dev);
	rtnl_unlock();

	return ret_val;
}
EXPORT_SYMBOL(mv_pp3_gnss_vport_state_set);
/*---------------------------------------------------------------------------*/

int mv_pp3_gnss_vport_mtu_get(unsigned short vport)
{
	struct net_device *dev = mv_pp3_vport_dev_get(vport);

	if (!dev) {
		pr_err("%s: network device for virtual port %d not exist\n", __func__,  vport);
		return -ENODEV;
	}
	return dev->mtu;
}
EXPORT_SYMBOL(mv_pp3_gnss_vport_mtu_get);
/*---------------------------------------------------------------------------*/

int mv_pp3_gnss_vport_mtu_set(unsigned short vport, int mtu)
{
	int rc;
	struct net_device *dev = mv_pp3_vport_dev_get(vport);

	if (!dev) {
		pr_err("%s: network device for virtual port %d not exist\n", __func__,  vport);
		return -ENODEV;
	}
	rtnl_lock();
	rc = dev_set_mtu(dev, mtu);
	rtnl_unlock();

	return rc;
}
EXPORT_SYMBOL(mv_pp3_gnss_vport_mtu_set);
/*---------------------------------------------------------------------------*/

int mv_pp3_gnss_vport_def_dst_get(unsigned short vport)
{
	struct net_device *dev = mv_pp3_vport_dev_get(vport);
	struct pp3_dev_priv *dev_priv;

	if (!dev) {
		pr_err("%s: network device for virtual port %d not exist\n", __func__,  vport);
		return -ENODEV;
	}
	dev_priv = MV_PP3_PRIV(dev);
	return dev_priv->vport->dest_vp;
}
EXPORT_SYMBOL(mv_pp3_gnss_vport_def_dst_get);
/*---------------------------------------------------------------------------*/

int mv_pp3_gnss_vport_def_dst_set(unsigned short vport, unsigned short def_dst)
{
	struct net_device *dev = mv_pp3_vport_dev_get(vport);
	struct pp3_dev_priv *dev_priv;

	if (!dev) {
		pr_err("%s: network device for virtual port %d not exist\n", __func__,  vport);
		return -ENODEV;
	}
	dev_priv = MV_PP3_PRIV(dev);
	if (pp3_fw_vport_def_dest_set(dev_priv->vport->vport, def_dst) < 0) {
		pr_warn("%s Error: FW vport %d default destination update failed\n",
				dev->name, def_dst);
		return -1;
	}
	dev_priv->vport->dest_vp = def_dst;

	return 0;
}
EXPORT_SYMBOL(mv_pp3_gnss_vport_def_dst_set);
/*---------------------------------------------------------------------------*/

int mv_pp3_gnss_vport_mcast_list_get(unsigned short vport, unsigned char *mac_list, int max_num, int *num)
{
	int count, offset;
	struct netdev_hw_addr *ha;
	struct net_device *dev = mv_pp3_vport_dev_get(vport);
	struct pp3_dev_priv *dev_priv;

	if (!dev) {
		pr_err("%s: network device for virtual port %d not exist\n", __func__,  vport);
		return -ENODEV;
	}
	dev_priv = MV_PP3_PRIV(dev);
	if (!dev_priv->vport || (dev_priv->vport->type != MV_PP3_NSS_PORT_ETH)) {
		pr_err("%s: Failed on vport #%d. Applicable only for EMAC virtual ports\n", __func__, vport);
		return -EINVAL;
	}
	count = offset = 0;
	netdev_for_each_mc_addr(ha, dev) {
		if (count >= max_num)
			break;

		memcpy(&mac_list[offset], ha->addr, MV_MAC_ADDR_SIZE);
		offset += MV_MAC_ADDR_SIZE;
		count++;
		/*
		pr_info("%02x:%02x:%02x:%02x:%02x:%02x\n",
			macs_list[offset + 0], macs_list[offset + 1], macs_list[offset + 2],
			macs_list[offset + 3], macs_list[offset + 4], macs_list[offset + 5]);
		*/
	}
	if (num)
		*num = count;

	return 0;
}
EXPORT_SYMBOL(mv_pp3_gnss_vport_mcast_list_get);
/*---------------------------------------------------------------------------*/

/* Replace dev->mc list with new one */
int mv_pp3_gnss_vport_mcast_list_set(unsigned short vport, unsigned char *mac_list, int num)
{
	int rc, i, offset;
	struct net_device *dev = mv_pp3_vport_dev_get(vport);
	struct pp3_dev_priv *dev_priv;

	if (!dev) {
		pr_err("%s: network device for virtual port %d not exist\n", __func__,  vport);
		return -ENODEV;
	}
	dev_priv = MV_PP3_PRIV(dev);
	if (!dev_priv->vport || (dev_priv->vport->type != MV_PP3_NSS_PORT_ETH)) {
		pr_err("%s: Failed on vport #%d. Applicable only for EMAC virtual ports\n", __func__, vport);
		return -EINVAL;
	}
	dev_mc_flush(dev);
	offset = 0;
	for (i = 0; i < num; i++) {
		rc = dev_mc_add(dev, &mac_list[offset]);
		if (rc)
			return -EINVAL;
		offset += MV_MAC_ADDR_SIZE;
	}
	return 0;
}
EXPORT_SYMBOL(mv_pp3_gnss_vport_mcast_list_set);
/*---------------------------------------------------------------------------*/

/* Delete (op=0) / Add (op=1) multicast address from L2 filter */
int mv_pp3_gnss_vport_mcast_addr_set(unsigned short vport, unsigned char *mac_addr, int op)
{
	int rc = -EINVAL;
	struct net_device *dev = mv_pp3_vport_dev_get(vport);
	struct pp3_dev_priv *dev_priv;

	if (!dev) {
		pr_err("%s: network device for virtual port %d not exist\n", __func__,  vport);
		return -ENODEV;
	}
	dev_priv = MV_PP3_PRIV(dev);
	if (!dev_priv->vport || (dev_priv->vport->type != MV_PP3_NSS_PORT_ETH)) {
		pr_err("%s: Failed on vport #%d. Applicable only for EMAC virtual ports\n", __func__, vport);
		return -EINVAL;
	}
	if (op == 0)
		rc = dev_mc_del(dev, mac_addr);
	else if (op == 1)
		rc = dev_mc_add(dev, mac_addr);
	else
		pr_err("%s: unsupported command: op=%d on vp=%d (%s)\n",
			__func__, op, vport, dev->name);

	return rc;
}
EXPORT_SYMBOL(mv_pp3_gnss_vport_mcast_addr_set);
/*---------------------------------------------------------------------------*/

int mv_pp3_gnss_vport_l2_options_get(unsigned short vport, unsigned char *l2_options)
{
	struct net_device *dev = mv_pp3_vport_dev_get(vport);
	struct pp3_dev_priv *dev_priv;

	if (!dev) {
		pr_err("%s: network device for virtual port %d not exist\n", __func__,  vport);
		return -ENODEV;
	}
	dev_priv = MV_PP3_PRIV(dev);
	if (!dev_priv->vport || (dev_priv->vport->type != MV_PP3_NSS_PORT_ETH)) {
		pr_err("%s: Failed on vport #%d. Applicable only for EMAC virtual ports\n", __func__, vport);
		return -EINVAL;
	}
	*l2_options = dev_priv->vport->port.emac.l2_options;
	return 0;
}
EXPORT_SYMBOL(mv_pp3_gnss_vport_l2_options_get);
/*---------------------------------------------------------------------------*/

int mv_pp3_gnss_vport_l2_options_set(unsigned short vport, unsigned char l2_options)
{
	int bit;
	unsigned int changes;
	struct net_device *dev = mv_pp3_vport_dev_get(vport);
	struct pp3_dev_priv *dev_priv;

	if (!dev) {
		pr_err("%s: network device for virtual port %d not exist\n", __func__,  vport);
		return -ENODEV;
	}
	dev_priv = MV_PP3_PRIV(dev);
	if (!dev_priv->vport || (dev_priv->vport->type != MV_PP3_NSS_PORT_ETH)) {
		pr_err("%s: Failed on vport #%d. Applicable only for EMAC virtual ports\n", __func__, vport);
		return -EINVAL;
	}

	/* set L2 features known to Linux */
	if ((dev->flags & IFF_PROMISC) ^ (l2_options & MV_NSS_PROMISC_MODE)) {
		/* Values are different. Update network device promiscous mode */
		rtnl_lock();
		dev_set_promiscuity(dev, (l2_options & MV_NSS_PROMISC_MODE) ? 1 : -1);
		rtnl_unlock();
	}

	if ((dev->flags & IFF_ALLMULTI) ^ (l2_options & MV_NSS_ALL_MCAST_MODE)) {
		/* Values are different. Update network device allmulticast mode */
		rtnl_lock();
		dev_set_allmulti(dev, (l2_options & MV_NSS_ALL_MCAST_MODE) ? 1 : -1);
		rtnl_unlock();
	}
	/* For all other bits send only message to firmware */
	changes = l2_options ^ dev_priv->vport->port.emac.l2_options;
	for (bit = MV_NSS_L2_UCAST_PROMISC; bit < MV_NSS_L2_OPTION_LAST; bit++) {
		if (changes & BIT(bit))
			pp3_fw_port_l2_filter_mode(vport, bit, l2_options & BIT(bit));
	}
	/* remember L2 options were set */
	dev_priv->vport->port.emac.l2_options = l2_options;

	return 0;
}
EXPORT_SYMBOL(mv_pp3_gnss_vport_l2_options_set);
/*---------------------------------------------------------------------------*/

int mv_pp3_gnss_vport_cos_get(unsigned short vport, unsigned char *cos)
{
	struct net_device *dev = mv_pp3_vport_dev_get(vport);
	struct pp3_dev_priv *dev_priv;

	if (!dev) {
		pr_err("%s: network device for virtual port %d not exist\n", __func__,  vport);
		return -ENODEV;
	}
	dev_priv = MV_PP3_PRIV(dev);
	*cos  = dev_priv->vport->cos;

	return 0;
}
EXPORT_SYMBOL(mv_pp3_gnss_vport_cos_get);
/*---------------------------------------------------------------------------*/

int mv_pp3_gnss_vport_cos_set(unsigned short vport, unsigned char cos)
{
#if 0 /* wait for FW support */
	struct net_device *dev = mv_pp3_vport_dev_get(vport);
	struct pp3_dev_priv *dev_priv;

	if (!dev) {
		pr_err("%s: network device for virtual port %d not exist\n", __func__,  vport);
		return -ENODEV;
	}
	dev_priv = MV_PP3_PRIV(dev);
	/* Add message */
	if (pp3_fw_vport_cos_set(dev_priv->vport->vport, cos) < 0) {
		pr_warn("%s Error: FW vport %d default CoS update failed\n", dev->name, cos);
		return -1;
	}
	dev_priv->vport->cos = cos;
#endif
	return 0;
}
EXPORT_SYMBOL(mv_pp3_gnss_vport_cos_set);
/*---------------------------------------------------------------------------*/
/* Function for VQ configuration */
/* [vport] argument is EMAC or External virtual port. */

/* Set drop parameters (TD and RED) for ingress [vq] of the [vport] */
int mv_pp3_gnss_ingress_vq_drop_set(unsigned short vport, int vq, struct mv_nss_drop *drop)
{
	int rc = 0;
	struct net_device *dev = mv_pp3_vport_dev_get(vport);

	if (!dev) {
		pr_err("%s: network device for virtual port %d not exist\n", __func__,  vport);
		return -ENODEV;
	}
	rc = mv_pp3_dev_ingress_vq_drop_set(dev, vq, drop);
	return rc;
}
EXPORT_SYMBOL(mv_pp3_gnss_ingress_vq_drop_set);
/*---------------------------------------------------------------------------*/

/* Get drop parameters (TD and RED) for ingress [vq] of the [vport] */
int mv_pp3_gnss_ingress_vq_drop_get(unsigned short vport, int vq, struct mv_nss_drop *drop)
{
	int rc = 0;
	struct net_device *dev = mv_pp3_vport_dev_get(vport);

	if (!dev) {
		pr_err("%s: network device for virtual port %d not exist\n", __func__,  vport);
		return -ENODEV;
	}
	rc = mv_pp3_dev_ingress_vq_drop_get(dev, vq, drop);

	return rc;
}
EXPORT_SYMBOL(mv_pp3_gnss_ingress_vq_drop_get);
/*---------------------------------------------------------------------------*/

/* Set scheduling parameters (prio and weight) for ingress [vq] of the [vport] */
int mv_pp3_gnss_ingress_vq_sched_set(unsigned short vport, int vq, struct mv_nss_sched *sched)
{
	int rc = 0;
	struct net_device *dev = mv_pp3_vport_dev_get(vport);

	if (!dev) {
		pr_err("%s: network device for virtual port %d not exist\n", __func__,  vport);
		return -ENODEV;
	}
	rc = mv_pp3_dev_ingress_vq_prio_set(dev, vq, sched->priority);
	if (!rc)
		rc = mv_pp3_dev_ingress_vq_weight_set(dev, vq, sched->weight);

	return rc;
}
EXPORT_SYMBOL(mv_pp3_gnss_ingress_vq_sched_set);
/*---------------------------------------------------------------------------*/

/* Get scheduling parameters (prio and weight) for ingress [vq] of the [vport] */
int mv_pp3_gnss_ingress_vq_sched_get(unsigned short vport, int vq, struct mv_nss_sched *sched)
{
	int rc = 0;
	struct net_device *dev = mv_pp3_vport_dev_get(vport);

	if (!dev) {
		pr_err("%s: network device for virtual port %d not exist\n", __func__,  vport);
		return -ENODEV;
	}
	rc = mv_pp3_dev_ingress_vq_sched_get(dev, vq, sched);

	return rc;
}
EXPORT_SYMBOL(mv_pp3_gnss_ingress_vq_sched_get);
/*---------------------------------------------------------------------------*/

/* Map cos value to vq for ingress vq of the vport */
int mv_pp3_gnss_ingress_cos_to_vq_set(unsigned short vport, int cos, int vq)
{
	int rc = 0;
	struct net_device *dev = mv_pp3_vport_dev_get(vport);

	if (!dev) {
		pr_err("%s: network device for virtual port %d not exist\n", __func__,  vport);
		return -ENODEV;
	}
	rc = mv_pp3_dev_ingress_cos_to_vq_set(dev, cos, vq);

	return rc;
}
EXPORT_SYMBOL(mv_pp3_gnss_ingress_cos_to_vq_set);
/*---------------------------------------------------------------------------*/

/* get vq mapped on cos value for ingress vq of the vport */
int mv_pp3_gnss_ingress_cos_to_vq_get(unsigned short vport, int cos, int *vq)
{
	int rc = 0;
	struct net_device *dev = mv_pp3_vport_dev_get(vport);

	if (!dev) {
		pr_err("%s: network device for virtual port %d not exist\n", __func__,  vport);
		return -ENODEV;
	}
	rc = mv_pp3_dev_ingress_cos_to_vq_get(dev, cos, vq);

	return rc;
}
EXPORT_SYMBOL(mv_pp3_gnss_ingress_cos_to_vq_get);
/*---------------------------------------------------------------------------*/

/* Set ingress VQ size in packets - SWQ part of VQ */
/* XOFF/XON threshold - must be less that RXQ capacity */
int mv_pp3_gnss_ingress_vq_size_set(unsigned short vport, int vq, u16 length)
{
	int rc;
	struct net_device *dev = mv_pp3_vport_dev_get(vport);

	if (!dev) {
		pr_err("%s: network device for virtual port %d not exist\n", __func__,  vport);
		return -ENODEV;
	}
	rc = mv_pp3_dev_ingress_vq_size_set(dev, vq, length);

	return rc;
}
EXPORT_SYMBOL(mv_pp3_gnss_ingress_vq_size_set);
/*---------------------------------------------------------------------------*/

/* Get ingress VQ size in packets - SWQ part of VQ */
/* XOFF/XON threshold - must be less that RXQ capacity */
int mv_pp3_gnss_ingress_vq_size_get(unsigned short vport, int vq, u16 *length)
{
	int rc;
	struct net_device *dev = mv_pp3_vport_dev_get(vport);

	if (!dev) {
		pr_err("%s: network device for virtual port %d not exist\n", __func__,  vport);
		return -ENODEV;
	}
	rc = mv_pp3_dev_ingress_vq_size_get(dev, vq, length);

	return rc;
}
EXPORT_SYMBOL(mv_pp3_gnss_ingress_vq_size_get);
/*---------------------------------------------------------------------------*/

/* Set drop parameters (TD and RED) for egress [vq] of the [vport] */
int mv_pp3_gnss_egress_vq_drop_set(unsigned short vport, int vq, struct mv_nss_drop *drop)
{
	int rc = 0;
	struct net_device *dev = mv_pp3_vport_dev_get(vport);

	if (!dev) {
		pr_err("%s: network device for virtual port %d not exist\n", __func__,  vport);
		return -ENODEV;
	}
	rc = mv_pp3_dev_egress_vq_drop_set(dev, vq, drop);
	return rc;
}
EXPORT_SYMBOL(mv_pp3_gnss_egress_vq_drop_set);
/*---------------------------------------------------------------------------*/

/* Get drop parameters (TD and RED) for egress [vq] of the [vport] */
int mv_pp3_gnss_egress_vq_drop_get(unsigned short vport, int vq, struct mv_nss_drop *drop)
{
	int rc = 0;
	struct net_device *dev = mv_pp3_vport_dev_get(vport);

	if (!dev) {
		pr_err("%s: network device for virtual port %d not exist\n", __func__,  vport);
		return -ENODEV;
	}
	rc = mv_pp3_dev_egress_vq_drop_get(dev, vq, drop);

	return rc;
}
EXPORT_SYMBOL(mv_pp3_gnss_egress_vq_drop_get);
/*---------------------------------------------------------------------------*/

/* Set scheduling parameters (prio and weight) for ingress [vq] of the [vport] */
int mv_pp3_gnss_egress_vq_sched_set(unsigned short vport, int vq, struct mv_nss_sched *sched)
{
	int rc = 0;
	struct net_device *dev = mv_pp3_vport_dev_get(vport);

	if (!dev) {
		pr_err("%s: network device for virtual port %d not exist\n", __func__,  vport);
		return -ENODEV;
	}
	rc = mv_pp3_dev_egress_vq_prio_set(dev, vq, sched->priority);
	if (!rc)
		rc = mv_pp3_dev_egress_vq_weight_set(dev, vq, sched->weight);

	return rc;
}
EXPORT_SYMBOL(mv_pp3_gnss_egress_vq_sched_set);
/*---------------------------------------------------------------------------*/

/* Get scheduling parameters (prio and weight) for egress [vq] of the [vport] */
int mv_pp3_gnss_egress_vq_sched_get(unsigned short vport, int vq, struct mv_nss_sched *sched)
{
	int rc = 0;
	struct net_device *dev = mv_pp3_vport_dev_get(vport);

	if (!dev) {
		pr_err("%s: network device for virtual port %d not exist\n", __func__,  vport);
		return -ENODEV;
	}
	rc = mv_pp3_dev_egress_vq_sched_get(dev, vq, sched);

	return rc;
}
EXPORT_SYMBOL(mv_pp3_gnss_egress_vq_sched_get);
/*---------------------------------------------------------------------------*/

/* Map cos value to vq for egress vq of the vport */
int mv_pp3_gnss_egress_cos_to_vq_set(unsigned short vport, int cos, int vq)
{
	int rc = 0;
	struct net_device *dev = mv_pp3_vport_dev_get(vport);

	if (!dev) {
		pr_err("%s: network device for virtual port %d not exist\n", __func__,  vport);
		return -ENODEV;
	}
	rc = mv_pp3_dev_egress_cos_to_vq_set(dev, cos, vq);

	return rc;
}
EXPORT_SYMBOL(mv_pp3_gnss_egress_cos_to_vq_set);
/*---------------------------------------------------------------------------*/

/* get vq mapped on cos value for egress vq of the vport */
int mv_pp3_gnss_egress_cos_to_vq_get(unsigned short vport, int cos, int *vq)
{
	int rc = 0;
	struct net_device *dev = mv_pp3_vport_dev_get(vport);

	if (!dev) {
		pr_err("%s: network device for virtual port %d not exist\n", __func__,  vport);
		return -ENODEV;
	}
	rc = mv_pp3_dev_egress_cos_to_vq_get(dev, cos, vq);

	return rc;
}
EXPORT_SYMBOL(mv_pp3_gnss_egress_cos_to_vq_get);
/*---------------------------------------------------------------------------*/

/* Set egress VQ size in packets - SWQ part of VQ */
/* Must be less than egress VQ capacity defined in Kconfig */
int mv_pp3_gnss_egress_vq_size_set(unsigned short vport, int vq, u16 length)
{
	int rc;
	struct net_device *dev = mv_pp3_vport_dev_get(vport);

	if (!dev) {
		pr_err("%s: network device for virtual port %d not exist\n", __func__,  vport);
		return -ENODEV;
	}
	rc = mv_pp3_dev_egress_vq_size_set(dev, vq, length);

	return rc;
}
EXPORT_SYMBOL(mv_pp3_gnss_egress_vq_size_set);
/*---------------------------------------------------------------------------*/

/* Get egress VQ size in packets - SWQ part of VQ */
int mv_pp3_gnss_egress_vq_size_get(unsigned short vport, int vq, u16 *length)
{
	int rc;
	struct net_device *dev = mv_pp3_vport_dev_get(vport);

	if (!dev) {
		pr_err("%s: network device for virtual port %d not exist\n", __func__,  vport);
		return -ENODEV;
	}
	rc = mv_pp3_dev_egress_vq_size_get(dev, vq, length);

	return rc;
}
EXPORT_SYMBOL(mv_pp3_gnss_egress_vq_size_get);
/*---------------------------------------------------------------------------*/

int mv_pp3_gnss_egress_vq_rate_limit_set(unsigned short vport, int vq, struct mv_nss_meter *meter)
{
	int rc;
	struct net_device *dev = mv_pp3_vport_dev_get(vport);

	if (!dev) {
		pr_err("%s: network device for virtual port %d not exist\n", __func__,  vport);
		return -ENODEV;
	}
	rc = mv_pp3_dev_egress_vq_rate_limit_set(dev, vq, meter);

	return rc;
}
EXPORT_SYMBOL(mv_pp3_gnss_egress_vq_rate_limit_set);
/*---------------------------------------------------------------------------*/

int mv_pp3_gnss_egress_vq_rate_limit_get(unsigned short vport, int vq, struct mv_nss_meter *meter)
{
	int rc;
	struct net_device *dev = mv_pp3_vport_dev_get(vport);

	if (!dev) {
		pr_err("%s: network device for virtual port %d not exist\n", __func__,  vport);
		return -ENODEV;
	}
	rc = mv_pp3_dev_egress_vq_rate_limit_get(dev, vq, meter);

	return rc;
}
EXPORT_SYMBOL(mv_pp3_gnss_egress_vq_rate_limit_get);
/*---------------------------------------------------------------------------*/

int  mv_pp3_gnss_state_get(bool *state)
{
	int ppc;

	for (ppc = 0; ppc < MV_PP3_PPC_MAX_NUM; ppc++) {
		if (!mv_fw_keep_alive_get(ppc)) {
			*state = false;
			return 0;
		}
	}

	*state = true;

	return 0;
}
EXPORT_SYMBOL(mv_pp3_gnss_state_get);

