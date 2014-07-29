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

#include <linux/etherdevice.h>
#include <linux/platform_device.h>
#include <linux/interrupt.h>
#include <linux/mv_switch.h>
#include <linux/module.h>

#include "mvOs.h"
#include "mvSysHwConfig.h"
#include "eth-phy/mvEthPhy.h"
#ifdef MV_INCLUDE_ETH_COMPLEX
#include "ctrlEnv/mvCtrlEthCompLib.h"
#endif /* MV_INCLUDE_ETH_COMPLEX */

#include "msApi.h"
#include "h/platform/gtMiiSmiIf.h"
#include "h/driver/gtHwCntl.h"
#include "mv_switch.h"
#include "mv_phy.h"
#include "mv_mux_netdev.h"

/*void mv_eth_switch_interrupt_clear(void);
void mv_eth_switch_interrupt_unmask(void);*/

#define MV_SWITCH_DEF_INDEX     0
#define MV_ETH_PORT_0           0
#define MV_ETH_PORT_1           1

static u16	db_port_mask[MV_SWITCH_DB_NUM];
static u16	db_link_mask[MV_SWITCH_DB_NUM];
static void	*db_cookies[MV_SWITCH_DB_NUM];

static struct sw_port_info_t	sw_port_tbl[MV_SWITCH_MAX_PORT_NUM];
static struct sw_vlan_info_t	sw_vlan_tbl[MV_SWITCH_MAX_VLAN_NUM];

/* uncomment for debug prints */
/* #define SWITCH_DEBUG */

#define SWITCH_DBG_OFF      0x0000
#define SWITCH_DBG_LOAD     0x0001
#define SWITCH_DBG_MCAST    0x0002
#define SWITCH_DBG_VLAN     0x0004
#define SWITCH_DBG_ALL      0xffff

#ifdef SWITCH_DEBUG
static u32 switch_dbg = 0xffff;
#define SWITCH_DBG(FLG, X) if ((switch_dbg & (FLG)) == (FLG)) printk X
#else
#define SWITCH_DBG(FLG, X)
#endif /* SWITCH_DEBUG */

static GT_QD_DEV qddev, *qd_dev = NULL;
static GT_SYS_CONFIG qd_cfg;

static int qd_cpu_port = -1;
static int enabled_ports_mask;
static int switch_ports_mask;
static MV_TAG_TYPE tag_mode;
static MV_SWITCH_PRESET_TYPE preset;
static int default_vid;
static int gbe_port;

static const struct mv_switch_mux_ops *mux_ops;
static const struct mv_mux_switch_ops switch_ops;

static struct tasklet_struct link_tasklet;
static int switch_irq = -1;
int switch_link_poll = 0;
static struct timer_list switch_link_timer;

static spinlock_t switch_lock;

static unsigned int mv_switch_link_detection_init(struct mv_switch_pdata *plat_data);

#ifdef CONFIG_AVANTA_LP
static GT_BOOL mv_switch_mii_read(GT_QD_DEV *dev, unsigned int phy, unsigned int reg, unsigned int *data)
{
	unsigned long flags;
	MV_U32 result, offset = 0;

	spin_lock_irqsave(&switch_lock, flags);

	offset |= (0x3 << 16);
	offset |= (phy << 7);
	offset |= (reg << 2);

	result = MV_REG_READ(offset);

	*data = result;

	spin_unlock_irqrestore(&switch_lock, flags);

	return GT_TRUE;
}
#else
static GT_BOOL mv_switch_mii_read(GT_QD_DEV *dev, unsigned int phy, unsigned int reg, unsigned int *data)
{
	unsigned long flags;
	unsigned short tmp;
	MV_STATUS status;

	spin_lock_irqsave(&switch_lock, flags);
	status = mvEthPhyRegRead(phy, reg, &tmp);
	spin_unlock_irqrestore(&switch_lock, flags);
	*data = tmp;

	if (status == MV_OK)
		return GT_TRUE;

	return GT_FALSE;
}
#endif

#ifdef CONFIG_AVANTA_LP
static GT_BOOL mv_switch_mii_write(GT_QD_DEV *dev, unsigned int phy, unsigned int reg, unsigned int data)
{
	unsigned long flags;
	MV_U32 offset = 0;

	spin_lock_irqsave(&switch_lock, flags);

	offset |= (0x3 << 16);
	offset |= (phy << 7);
	offset |= (reg << 2);

	MV_REG_WRITE(offset, data);

	spin_unlock_irqrestore(&switch_lock, flags);

	return GT_TRUE;
}
#else
static GT_BOOL mv_switch_mii_write(GT_QD_DEV *dev, unsigned int phy, unsigned int reg, unsigned int data)
{
	unsigned long flags;
	unsigned short tmp;
	MV_STATUS status;

	spin_lock_irqsave(&switch_lock, flags);
	tmp = (unsigned short)data;
	status = mvEthPhyRegWrite(phy, reg, tmp);
	spin_unlock_irqrestore(&switch_lock, flags);

	if (status == MV_OK)
		return GT_TRUE;

	return GT_FALSE;
}
#endif

static int mv_switch_port_db_get(int port)
{
	int db;

	for (db = 0; db < MV_SWITCH_DB_NUM; db++) {
		if (db_port_mask[db] & (1 << port))
			return db;
	}

	return -1;
}

int mv_switch_default_config_get(MV_TAG_TYPE *tag_mode_val,
			MV_SWITCH_PRESET_TYPE *preset_val, int *vid_val, int *gbe_port_val)
{
	*tag_mode_val = tag_mode;
	*preset_val = preset;
	*vid_val = default_vid;
	*gbe_port_val = gbe_port;

	return 0;
}

/* return true if db is exist, else return false */
bool mv_switch_tag_get(int db, MV_TAG_TYPE tag_mode, MV_SWITCH_PRESET_TYPE preset, int vid, MV_MUX_TAG *tag)
{
	unsigned int p, port_mask = db_port_mask[db];

	MV_IF_NULL_RET_STR(qd_dev, MV_FALSE, "switch dev qd_dev has not been init!\n");

	if (db_port_mask[db] == 0)
		return MV_FALSE;

	tag->tag_type = tag_mode;

	if (preset == MV_PRESET_SINGLE_VLAN) {
		if (tag_mode == MV_TAG_TYPE_MH) {
			tag->rx_tag_ptrn.mh = MV_16BIT_BE(vid << 12);
			tag->rx_tag_mask.mh = MV_16BIT_BE(0xf000);
			tag->tx_tag.mh = MV_16BIT_BE((vid << 12) | port_mask);
		} else if (tag_mode == MV_TAG_TYPE_DSA) {
			tag->rx_tag_ptrn.dsa = MV_32BIT_BE(0xc8000000 | MV_SWITCH_GROUP_VLAN_ID(vid));
			tag->rx_tag_mask.dsa = MV_32BIT_BE(0xff000f00);
			tag->tx_tag.dsa = MV_32BIT_BE(0xc8000000 | MV_SWITCH_GROUP_VLAN_ID(vid));
		}
	} else if (preset == MV_PRESET_PER_PORT_VLAN) {
		for (p = 0; p < qd_dev->numOfPorts; p++)
			if (MV_BIT_CHECK(port_mask, p) && (p != qd_cpu_port)) {
				if (tag_mode == MV_TAG_TYPE_MH) {
					tag->rx_tag_ptrn.mh = MV_16BIT_BE((vid + p) << 12);
					tag->rx_tag_mask.mh = MV_16BIT_BE(0xf000);
					tag->tx_tag.mh = MV_16BIT_BE(((vid + p) << 12) | (1 << p));
				} else if (tag_mode == MV_TAG_TYPE_DSA) {
					tag->rx_tag_ptrn.dsa =
						MV_32BIT_BE(0xc8000000 | MV_SWITCH_GROUP_VLAN_ID(vid + p));
					tag->rx_tag_mask.dsa = MV_32BIT_BE(0xff000f00);
					tag->tx_tag.dsa = MV_32BIT_BE(0xc8000000 | MV_SWITCH_GROUP_VLAN_ID(vid + p));
				}
			}
	} /* do nothing if Transparent mode */

	return MV_TRUE;
}

unsigned int mv_switch_group_map_get(void)
{
	unsigned int res = 0, db;

	for (db = 0; db < MV_SWITCH_DB_NUM; db++) {
		if (db_port_mask[db] != 0)
			res |= (1 << db);
	}

	return res;
}

static int mv_switch_group_state_set(int db, int en)
{
	unsigned int p, port_mask = db_port_mask[db];

	MV_IF_NULL_RET_STR(qd_dev, -1, "switch dev qd_dev has not been init!\n");

	/* enable/disable all ports in group */
	for (p = 0; p < qd_dev->numOfPorts; p++) {
		if (!MV_BIT_CHECK(port_mask, p))
			continue;

		if (en) {
			if (gstpSetPortState(qd_dev, p, GT_PORT_FORWARDING) != GT_OK) {
				printk(KERN_ERR "gstpSetPortState failed\n");
				return -1;
			}
		} else {
			if (gstpSetPortState(qd_dev, p, GT_PORT_DISABLE) != GT_OK) {
				printk(KERN_ERR "gstpSetPortState failed\n");
				return -1;
			}
		}
	}

	return 0;
}

int mv_switch_group_restart_autoneg(int db)
{
	unsigned int p, port_mask = db_port_mask[db];

	MV_IF_NULL_RET_STR(qd_dev, -1, "switch dev qd_dev has not been init!\n");

	/* enable/disable all ports in group */
	for (p = 0; p < qd_dev->numOfPorts; p++) {
		if (!MV_BIT_CHECK(port_mask, p))
			continue;

		if (gprtPortRestartAutoNeg(qd_dev, p) != GT_OK) {
			printk(KERN_ERR "gprtPortRestartAutoNeg failed\n");
			return -1;
		}
	}
	return 0;
}

int mv_switch_group_enable(int db)
{
	return mv_switch_group_state_set(db, 1);
}

int mv_switch_group_disable(int db)
{
	return mv_switch_group_state_set(db, 0);
}

int mv_switch_link_status_get(int db)
{
	return (db_link_mask[db] > 0);
}

int mv_switch_mux_ops_set(const struct mv_switch_mux_ops *mux_ops_ptr)
{
	mux_ops = mux_ops_ptr;

	return 0;
}

int mv_switch_group_cookie_set(int db, void *cookie)
{
	db_cookies[db] = cookie;

	return 0;
}

int mv_switch_mac_update(int db, unsigned char *old_mac, unsigned char *new_mac)
{
	int err;

	/* remove old mac */
	err = mv_switch_mac_addr_set(db, old_mac, 0);
	if (err)
		return err;

	/* add new mac */
	err = mv_switch_mac_addr_set(db, new_mac, 1);

	return err;
}
int mv_switch_mac_addr_set(int db, unsigned char *mac_addr, unsigned char op)
{
	GT_ATU_ENTRY mac_entry;
	unsigned int ports_mask;

	MV_IF_NULL_RET_STR(qd_dev, -1, "switch dev qd_dev has not been init!\n");

	memset(&mac_entry, 0, sizeof(GT_ATU_ENTRY));

	mac_entry.trunkMember = GT_FALSE;
	mac_entry.prio = 0;
	mac_entry.exPrio.useMacFPri = GT_FALSE;
	mac_entry.exPrio.macFPri = 0;
	mac_entry.exPrio.macQPri = 0;
	mac_entry.DBNum = db;
	memcpy(mac_entry.macAddr.arEther, mac_addr, 6);

	if (is_multicast_ether_addr(mac_addr)) {
		ports_mask = db_port_mask[db] | (1 << qd_cpu_port);
		mac_entry.entryState.mcEntryState = GT_MC_STATIC;
	} else {
		ports_mask = (1 << qd_cpu_port);
		mac_entry.entryState.ucEntryState = GT_UC_NO_PRI_STATIC;
	}
	mac_entry.portVec = ports_mask;

	if ((op == 0) || (mac_entry.portVec == 0)) {
		if (gfdbDelAtuEntry(qd_dev, &mac_entry) != GT_OK) {
			printk(KERN_ERR "gfdbDelAtuEntry failed\n");
			return -1;
		}
	} else {
		if (gfdbAddMacEntry(qd_dev, &mac_entry) != GT_OK) {
			printk(KERN_ERR "gfdbAddMacEntry failed\n");
			return -1;
		}
	}

	return 0;
}

int mv_switch_port_based_vlan_set(unsigned int ports_mask, int set_cpu_port)
{
	unsigned int p, pl;
	unsigned char cnt;
	GT_LPORT port_list[MAX_SWITCH_PORTS];

	MV_IF_NULL_RET_STR(qd_dev, -1, "switch dev qd_dev has not been init!\n");

	for (p = 0; p < qd_dev->numOfPorts; p++) {
		if (MV_BIT_CHECK(ports_mask, p) && (set_cpu_port || (p != qd_cpu_port))) {
			SWITCH_DBG(SWITCH_DBG_LOAD | SWITCH_DBG_MCAST | SWITCH_DBG_VLAN,
				   ("port based vlan, port %d: ", p));
			for (pl = 0, cnt = 0; pl < qd_dev->numOfPorts; pl++) {
				if (MV_BIT_CHECK(ports_mask, pl) && (pl != p)) {
					SWITCH_DBG(SWITCH_DBG_LOAD | SWITCH_DBG_MCAST | SWITCH_DBG_VLAN, ("%d ", pl));
					port_list[cnt] = pl;
					cnt++;
				}
			}
			if (gvlnSetPortVlanPorts(qd_dev, p, port_list, cnt) != GT_OK) {
				printk(KERN_ERR "gvlnSetPortVlanPorts failed\n");
				return -1;
			}
			SWITCH_DBG(SWITCH_DBG_LOAD | SWITCH_DBG_MCAST | SWITCH_DBG_VLAN, ("\n"));
		}
	}
	return 0;
}

int mv_switch_vlan_in_vtu_set(unsigned short vlan_id, unsigned short db_num, unsigned int ports_mask)
{
	GT_VTU_ENTRY vtu_entry;
	unsigned int p;

	MV_IF_NULL_RET_STR(qd_dev, -1, "switch dev qd_dev has not been init!\n");

	memset(&vtu_entry, 0, sizeof(GT_VTU_ENTRY));
	vtu_entry.sid = 1;
	vtu_entry.vid = vlan_id;
	vtu_entry.DBNum = db_num;
	vtu_entry.vidPriOverride = GT_FALSE;
	vtu_entry.vidPriority = 0;
	vtu_entry.vidExInfo.useVIDFPri = GT_FALSE;
	vtu_entry.vidExInfo.vidFPri = 0;
	vtu_entry.vidExInfo.useVIDQPri = GT_FALSE;
	vtu_entry.vidExInfo.vidQPri = 0;
	vtu_entry.vidExInfo.vidNRateLimit = GT_FALSE;
	SWITCH_DBG(SWITCH_DBG_LOAD | SWITCH_DBG_MCAST | SWITCH_DBG_VLAN, ("vtu entry: vid=0x%x, port ", vtu_entry.vid));

	for (p = 0; p < qd_dev->numOfPorts; p++) {
		if (MV_BIT_CHECK(ports_mask, p)) {
			SWITCH_DBG(SWITCH_DBG_LOAD | SWITCH_DBG_MCAST | SWITCH_DBG_VLAN, ("%d ", p));
			vtu_entry.vtuData.memberTagP[p] = MEMBER_EGRESS_UNMODIFIED;
		} else {
			vtu_entry.vtuData.memberTagP[p] = NOT_A_MEMBER;
		}
		vtu_entry.vtuData.portStateP[p] = 0;
	}

	if (gvtuAddEntry(qd_dev, &vtu_entry) != GT_OK) {
		printk(KERN_ERR "gvtuAddEntry failed\n");
		return -1;
	}

	SWITCH_DBG(SWITCH_DBG_LOAD | SWITCH_DBG_MCAST | SWITCH_DBG_VLAN, ("\n"));
	return 0;
}

int mv_switch_atu_db_flush(int db_num)
{
	MV_IF_NULL_RET_STR(qd_dev, -1, "switch dev qd_dev has not been init!\n");

	if (gfdbFlushInDB(qd_dev, GT_FLUSH_ALL, db_num) != GT_OK) {
		printk(KERN_ERR "gfdbFlushInDB failed\n");
		return -1;
	}
	return 0;
}

int mv_switch_promisc_set(int db, u8 promisc_on)
{
	int i;
	unsigned int ports_mask = db_port_mask[db];
	int vlan_grp_id = MV_SWITCH_GROUP_VLAN_ID(db);

	MV_IF_NULL_RET_STR(qd_dev, -1, "switch dev qd_dev has not been init!\n");

	if (promisc_on)
		ports_mask |= (1 << qd_cpu_port);
	else
		ports_mask &= ~(1 << qd_cpu_port);

	mv_switch_port_based_vlan_set(ports_mask, 0);

	for (i = 0; i < qd_dev->numOfPorts; i++) {
		if (MV_BIT_CHECK(ports_mask, i) && (i != qd_cpu_port)) {
			if (mv_switch_vlan_in_vtu_set(MV_SWITCH_PORT_VLAN_ID(vlan_grp_id, i),
						      MV_SWITCH_VLAN_TO_GROUP(vlan_grp_id),
							ports_mask) != 0) {
				printk(KERN_ERR "mv_switch_vlan_in_vtu_set failed\n");
				return -1;
			}
		}
	}
	db_port_mask[db] = ports_mask;

	return 0;
}

int mv_switch_vlan_set(u16 vlan_grp_id, u16 port_map)
{
	int p;

	MV_IF_NULL_RET_STR(qd_dev, -1, "switch dev qd_dev has not been init!\n");

	/* set port's default private vlan id and database number (DB per group): */
	for (p = 0; p < qd_dev->numOfPorts; p++) {
		if (MV_BIT_CHECK(port_map, p) && (p != qd_cpu_port)) {
			if (gvlnSetPortVid(qd_dev, p, MV_SWITCH_PORT_VLAN_ID(vlan_grp_id, p)) != GT_OK) {
				printk(KERN_ERR "gvlnSetPortVid failed\n");
				return -1;
			}
			if (gvlnSetPortVlanDBNum(qd_dev, p, MV_SWITCH_VLAN_TO_GROUP(vlan_grp_id)) != GT_OK) {
				printk(KERN_ERR "gvlnSetPortVlanDBNum failed\n");
				return -1;
			}
		}
	}

	/* set port's port-based vlan (CPU port is not part of VLAN) */
	if (mv_switch_port_based_vlan_set((port_map & ~(1 << qd_cpu_port)), 0) != 0)
		printk(KERN_ERR "mv_switch_port_based_vlan_set failed\n");

	/* set vtu with group vlan id (used in tx) */
	if (mv_switch_vlan_in_vtu_set(vlan_grp_id,
					MV_SWITCH_VLAN_TO_GROUP(vlan_grp_id),
					 port_map | (1 << qd_cpu_port)) != 0)
		printk(KERN_ERR "mv_switch_vlan_in_vtu_set failed\n");

	/* set vtu with each port private vlan id (used in rx) */
	for (p = 0; p < qd_dev->numOfPorts; p++) {
		if (MV_BIT_CHECK(port_map, p) && (p != qd_cpu_port)) {
			if (mv_switch_vlan_in_vtu_set(MV_SWITCH_PORT_VLAN_ID(vlan_grp_id, p),
						      MV_SWITCH_VLAN_TO_GROUP(vlan_grp_id),
						      port_map & ~(1 << qd_cpu_port)) != 0) {
				printk(KERN_ERR "mv_switch_vlan_in_vtu_set failed\n");
			}
		}
	}

	/* update SW vlan DB port mask */
	db_port_mask[MV_SWITCH_VLAN_TO_GROUP(vlan_grp_id)] = port_map & ~(1 << qd_cpu_port);

	return 0;
}

void mv_switch_link_update_event(MV_U32 port_mask, int force_link_check)
{
	int p, db;
	unsigned short phy_cause = 0;

	MV_IF_NULL_STR(qd_dev, "switch dev qd_dev has not been init!\n");

	for (p = 0; p < qd_dev->numOfPorts; p++) {
		if (MV_BIT_CHECK(port_mask, p)) {
			/* this is needed to clear the PHY interrupt */
			gprtGetPhyIntStatus(qd_dev, p, &phy_cause);

			if (force_link_check)
				phy_cause |= GT_LINK_STATUS_CHANGED;

			if (phy_cause & GT_LINK_STATUS_CHANGED) {
				char *link = NULL, *duplex = NULL, *speed = NULL;
				GT_BOOL flag;
				GT_PORT_SPEED_MODE speed_mode;

				if (gprtGetLinkState(qd_dev, p, &flag) != GT_OK) {
					printk(KERN_ERR "gprtGetLinkState failed (port %d)\n", p);
					link = "ERR";
				} else
					link = (flag) ? "up" : "down";

				if (flag) {
					if (gprtGetDuplex(qd_dev, p, &flag) != GT_OK) {
						printk(KERN_ERR "gprtGetDuplex failed (port %d)\n", p);
						duplex = "ERR";
					} else
						duplex = (flag) ? "Full" : "Half";

					if (gprtGetSpeedMode(qd_dev, p, &speed_mode) != GT_OK) {
						printk(KERN_ERR "gprtGetSpeedMode failed (port %d)\n", p);
						speed = "ERR";
					} else {
						if (speed_mode == PORT_SPEED_1000_MBPS)
							speed = "1000Mbps";
						else if (speed_mode == PORT_SPEED_100_MBPS)
							speed = "100Mbps";
						else
							speed = "10Mbps";
					}

					db = mv_switch_port_db_get(p);
					if (db != -1) {
						/* link up event for group device (i.e. mux) */
						if ((db_link_mask[db] == 0) && (mux_ops) && (mux_ops->update_link))
							mux_ops->update_link(db_cookies[db], 1);
						db_link_mask[db] |= (1 << p);
					}

					printk(KERN_ERR "Port %d: Link-%s, %s-duplex, Speed-%s.\n",
					       p, link, duplex, speed);
				} else {
					db = mv_switch_port_db_get(p);
					if (db != -1) {
						db_link_mask[db] &= ~(1 << p);
						/* link down event for group device (i.e. mux) */
						if ((db_link_mask[db] == 0) && (mux_ops) && (mux_ops->update_link))
							mux_ops->update_link(db_cookies[db], 0);
					}

					printk(KERN_ERR "Port %d: Link-down\n", p);
				}
			}
		}
	}
}

void mv_switch_link_timer_function(unsigned long data)
{
	/* GT_DEV_INT_STATUS devIntStatus; */
	MV_U32 port_mask = (data & 0xFF);

	mv_switch_link_update_event(port_mask, 0);

	if (switch_link_poll) {
		switch_link_timer.expires = jiffies + (HZ);	/* 1 second */
		add_timer(&switch_link_timer);
	}
}

static irqreturn_t mv_switch_isr(int irq, void *dev_id)
{
	mv_switch_interrupt_mask();

	tasklet_schedule(&link_tasklet);

	return IRQ_HANDLED;
}

void mv_switch_tasklet(unsigned long data)
{
	GT_DEV_INT_STATUS devIntStatus;
	MV_U32 port_mask = 0;

	MV_IF_NULL_STR(qd_dev, "switch dev qd_dev has not been init!\n");

	/* TODO: verify that switch interrupt occured */

	if (geventGetDevIntStatus(qd_dev, &devIntStatus) != GT_OK)
		printk(KERN_ERR "geventGetDevIntStatus failed\n");

	if (devIntStatus.devIntCause & GT_DEV_INT_PHY)
		port_mask = devIntStatus.phyInt & 0xFF;

	mv_switch_link_update_event(port_mask, 0);

	mv_switch_interrupt_clear();

	mv_switch_interrupt_unmask();
}

int mv_switch_jumbo_mode_set(int max_size)
{
	int i;
	GT_JUMBO_MODE jumbo_mode;

	MV_IF_NULL_RET_STR(qd_dev, -1, "switch dev qd_dev has not been init!\n");

	/* Set jumbo frames mode */
	if (max_size <= 1522)
		jumbo_mode = GT_JUMBO_MODE_1522;
	else if (max_size <= 2048)
		jumbo_mode = GT_JUMBO_MODE_2048;
	else
		jumbo_mode = GT_JUMBO_MODE_10240;

	for (i = 0; i < qd_dev->numOfPorts; i++) {
		if (gsysSetJumboMode(qd_dev, i, jumbo_mode) != GT_OK) {
			printk(KERN_ERR "gsysSetJumboMode %d failed\n", jumbo_mode);
			return -1;
		}
	}
	return 0;
}

int mv_switch_load(struct mv_switch_pdata *plat_data)
{
	int p;
	GT_STU_ENTRY	stuEntry;

	printk(KERN_ERR "  o Loading Switch QuarterDeck driver\n");

	if (qd_dev) {
		printk(KERN_ERR "    o %s: Already initialized\n", __func__);
		return 0;
	}

	memset((char *)&qd_cfg, 0, sizeof(GT_SYS_CONFIG));
	spin_lock_init(&switch_lock);

	/* init config structure for qd package */
	qd_cfg.BSPFunctions.readMii = mv_switch_mii_read;
	qd_cfg.BSPFunctions.writeMii = mv_switch_mii_write;
	qd_cfg.BSPFunctions.semCreate = NULL;
	qd_cfg.BSPFunctions.semDelete = NULL;
	qd_cfg.BSPFunctions.semTake = NULL;
	qd_cfg.BSPFunctions.semGive = NULL;
	qd_cfg.initPorts = GT_FALSE;
	qd_cfg.cpuPortNum = plat_data->switch_cpu_port;
	if (plat_data->smi_scan_mode == 1) {
		qd_cfg.mode.baseAddr = 0;
		qd_cfg.mode.scanMode = SMI_MANUAL_MODE;
	} else if (plat_data->smi_scan_mode == 2) {
		qd_cfg.mode.scanMode = SMI_MULTI_ADDR_MODE;
		qd_cfg.mode.baseAddr = plat_data->phy_addr;
	}

	/* load switch sw package */
	if (qdLoadDriver(&qd_cfg, &qddev) != GT_OK) {
		printk(KERN_ERR "qdLoadDriver failed\n");
		return -1;
	}
	qd_dev = &qddev;
	qd_cpu_port = qd_cfg.cpuPortNum;

	/* WA - Create dummy entry in STU table  */
	memset(&stuEntry, 0, sizeof(GT_STU_ENTRY));
	stuEntry.sid = 1; /* required: ((sid > 0) && (sid < 0x3F)) */
	gstuAddEntry(qd_dev, &stuEntry);

	printk(KERN_ERR "    o Device ID     : 0x%x\n", qd_dev->deviceId);
	printk(KERN_ERR "    o No. of Ports  : %d\n", qd_dev->numOfPorts);
	printk(KERN_ERR "    o CPU Port      : %ld\n", qd_dev->cpuPortNum);

	/* disable all disconnected ports */
	for (p = 0; p < qd_dev->numOfPorts; p++) {
		/* Do nothing for ports that are not part of the given switch_port_mask */
		if (!MV_BIT_CHECK(plat_data->port_mask, p))
			continue;

		/* Force link for ports that connected to GBE ports */
		if (MV_BIT_CHECK(plat_data->forced_link_port_mask, p)) {
			/* Switch port connected to GMAC - force link UP - 1000 Full with FC */
			printk(KERN_ERR "    o Setting Switch Port #%d connected to GMAC port for 1000 Full with FC\n", p);
			if (gpcsSetForceSpeed(qd_dev, p, PORT_FORCE_SPEED_1000_MBPS) != GT_OK) {
				printk(KERN_ERR "Force speed 1000mbps - Failed\n");
				return -1;
			}

			if ((gpcsSetDpxValue(qd_dev, p, GT_TRUE) != GT_OK) ||
			    (gpcsSetForcedDpx(qd_dev, p, GT_TRUE) != GT_OK)) {
				printk(KERN_ERR "Force duplex FULL - Failed\n");
				return -1;
			}

			if ((gpcsSetFCValue(qd_dev, p, GT_TRUE) != GT_OK) ||
			    (gpcsSetForcedFC(qd_dev, p, GT_TRUE) != GT_OK)) {
				printk(KERN_ERR "Force Flow Control - Failed\n");
				return -1;
			}

			if ((gpcsSetLinkValue(qd_dev, p, GT_TRUE) != GT_OK) ||
			    (gpcsSetForcedLink(qd_dev, p, GT_TRUE) != GT_OK)) {
				printk(KERN_ERR "Force Link UP - Failed\n");
				return -1;
			}

			if (gprtSetPHYDetect(qd_dev, p, GT_FALSE) != GT_OK) {
				printk(KERN_ERR "gprtSetPHYDetect failed\n");
				return -1;
			}

			continue;
		}

		/* Switch port mapped to connector on the board */

		if ((gpcsSetFCValue(qd_dev, p, GT_FALSE) != GT_OK) ||
		    (gpcsSetForcedFC(qd_dev, p, GT_FALSE) != GT_OK)) {
			printk(KERN_ERR "Force Flow Control - Failed\n");
			return -1;
		}

		/* Disable switch ports that aren't connected to CPU/PHY */
		if (!MV_BIT_CHECK(plat_data->connected_port_mask, p)) {
			printk(KERN_ERR "    o Disable disconnected Switch Port #%d and force link down\n", p);
			if (gstpSetPortState(qd_dev, p, GT_PORT_DISABLE) != GT_OK) {
				printk(KERN_ERR "gstpSetPortState failed\n");
				return -1;
			}
			if ((gpcsSetLinkValue(qd_dev, p, GT_FALSE) != GT_OK) ||
			    (gpcsSetForcedLink(qd_dev, p, GT_TRUE) != GT_OK)) {
				printk(KERN_ERR "Force Link DOWN - Failed\n");
				return -1;
			}

			if (gprtSetPHYDetect(qd_dev, p, GT_FALSE) != GT_OK) {
				printk(KERN_ERR "gprtSetPHYDetect failed\n");
				return -1;
			}
		}
	}
	return 0;
}

int mv_switch_unload(unsigned int switch_ports_mask)
{
	int i;

	printk(KERN_ERR "  o Unloading Switch QuarterDeck driver\n");

	if (qd_dev == NULL) {
		printk(KERN_ERR "    o %s: Already un-initialized\n", __func__);
		return 0;
	}

	/* Flush all addresses from the MAC address table */
	/* this also happens in mv_switch_init() but we call it here to clean-up nicely */
	/* Note: per DB address flush (gfdbFlushInDB) happens when doing ifconfig down on a Switch interface */
	if (gfdbFlush(qd_dev, GT_FLUSH_ALL) != GT_OK)
		printk(KERN_ERR "gfdbFlush failed\n");

	/* Reset VLAN tunnel mode */
	for (i = 0; i < qd_dev->numOfPorts; i++) {
		if (MV_BIT_CHECK(switch_ports_mask, i) && (i != qd_cpu_port))
			if (gprtSetVlanTunnel(qd_dev, i, GT_FALSE) != GT_OK)
				printk(KERN_ERR "gprtSetVlanTunnel failed (port %d)\n", i);
	}

	/* restore port's default private vlan id and database number to their default values after reset: */
	for (i = 0; i < qd_dev->numOfPorts; i++) {
		if (gvlnSetPortVid(qd_dev, i, 0x0001) != GT_OK) { /* that's the default according to the spec */
			printk(KERN_ERR "gvlnSetPortVid failed\n");
			return -1;
		}
		if (gvlnSetPortVlanDBNum(qd_dev, i, 0) != GT_OK) {
			printk(KERN_ERR "gvlnSetPortVlanDBNum failed\n");
			return -1;
		}
	}

	/* Port based VLAN */
	if (mv_switch_port_based_vlan_set(switch_ports_mask, 1))
		printk(KERN_ERR "mv_switch_port_based_vlan_set failed\n");

	/* Remove all entries from the VTU table */
	if (gvtuFlush(qd_dev) != GT_OK)
		printk(KERN_ERR "gvtuFlush failed\n");

	/* unload switch sw package */
	if (qdUnloadDriver(qd_dev) != GT_OK) {
		printk(KERN_ERR "qdUnloadDriver failed\n");
		return -1;
	}
	qd_dev = NULL;
	qd_cpu_port = -1;

	switch_irq = -1;
	switch_link_poll = 0;
	del_timer(&switch_link_timer);

	return 0;
}

int mv_switch_init(struct mv_switch_pdata *plat_data)
{
	unsigned int p;

	if (qd_dev == NULL) {
		printk(KERN_ERR "%s: qd_dev not initialized, call mv_switch_load() first\n", __func__);
		return -1;
	}

	/* general Switch initialization - relevant for all Switch devices */
	memset(db_port_mask, 0, sizeof(u16) * MV_SWITCH_DB_NUM);
	memset(db_link_mask, 0, sizeof(u16) * MV_SWITCH_DB_NUM);
	memset(db_cookies, 0, sizeof(void *) * MV_SWITCH_DB_NUM);

	/* disable all ports */
	for (p = 0; p < qd_dev->numOfPorts; p++) {
		if (MV_BIT_CHECK(plat_data->connected_port_mask, p))
			if (gstpSetPortState(qd_dev, p, GT_PORT_DISABLE) != GT_OK) {
				printk(KERN_ERR "gstpSetPortState failed\n");
				return -1;
			}
	}

	/* flush All counters for all ports */
	if (gstatsFlushAll(qd_dev) != GT_OK)
		printk(KERN_ERR "gstatsFlushAll failed\n");

	/* set all ports not to unmodify the vlan tag on egress */
	for (p = 0; p < qd_dev->numOfPorts; p++) {
		if (MV_BIT_CHECK(plat_data->connected_port_mask, p)) {
			if (gprtSetEgressMode(qd_dev, p, GT_UNMODIFY_EGRESS) != GT_OK) {
				printk(KERN_ERR "gprtSetEgressMode GT_UNMODIFY_EGRESS failed\n");
				return -1;
			}
		}
	}

	/* initializes the PVT Table (cross-chip port based VLAN) to all one's (initial state) */
	if (gpvtInitialize(qd_dev) != GT_OK) {
		printk(KERN_ERR "gpvtInitialize failed\n");
		return -1;
	}

	/* set priorities rules */
	for (p = 0; p < qd_dev->numOfPorts; p++) {
		if (MV_BIT_CHECK(plat_data->connected_port_mask, p)) {
			/* default port priority to queue zero */
			if (gcosSetPortDefaultTc(qd_dev, p, 0) != GT_OK)
				printk(KERN_ERR "gcosSetPortDefaultTc failed (port %d)\n", p);

			/* enable IP TOS Prio */
			if (gqosIpPrioMapEn(qd_dev, p, GT_TRUE) != GT_OK)
				printk(KERN_ERR "gqosIpPrioMapEn failed (port %d)\n", p);

			/* set IP QoS */
			if (gqosSetPrioMapRule(qd_dev, p, GT_FALSE) != GT_OK)
				printk(KERN_ERR "gqosSetPrioMapRule failed (port %d)\n", p);

			/* disable Vlan QoS Prio */
			if (gqosUserPrioMapEn(qd_dev, p, GT_FALSE) != GT_OK)
				printk(KERN_ERR "gqosUserPrioMapEn failed (port %d)\n", p);
		}
	}

	if (gfdbFlush(qd_dev, GT_FLUSH_ALL) != GT_OK)
		printk(KERN_ERR "gfdbFlush failed\n");

	mv_switch_link_detection_init(plat_data);

	/* Enable Jumbo support by default */
	mv_switch_jumbo_mode_set(9180);

	/* Configure Ethernet related LEDs, currently according to Switch ID */
	switch (qd_dev->deviceId) {
	case GT_88E6161:
	case GT_88E6165:
	case GT_88E6171:
	case GT_88E6172:
	case GT_88E6176:
	case GT_88E6351:
	case GT_88E6352:
		break;		/* do nothing */

	default:
		for (p = 0; p < qd_dev->numOfPorts; p++) {
			if ((p != qd_cpu_port) &&
				MV_BIT_CHECK(plat_data->connected_port_mask, p)) {
				if (gprtSetPhyReg(qd_dev, p, 22, 0x1FFA)) {
					/* Configure Register 22 LED0 to 0xA for Link/Act */
					printk(KERN_ERR "gprtSetPhyReg failed (port=%d)\n", p);
				}
			}
		}
		break;
	}

	/* Configure speed between GBE port and CPU port */
	gprtSet200Base(qd_dev, qd_cpu_port, plat_data->is_speed_2000);

	switch_ports_mask = plat_data->connected_port_mask;
	tag_mode = plat_data->tag_mode;
	preset = plat_data->preset;
	default_vid = plat_data->vid;
	gbe_port = plat_data->gbe_port;

	enabled_ports_mask = switch_ports_mask;

	mv_mux_switch_ops_set(&switch_ops);
	mv_mux_switch_attach(gbe_port, preset, default_vid, tag_mode, plat_data->switch_cpu_port);


#ifdef SWITCH_DEBUG
	/* for debug: */
	mv_switch_status_print();
#endif

	return 0;
}

int mv_switch_preset_init(MV_TAG_TYPE tag_mode, MV_SWITCH_PRESET_TYPE preset, int vid)
{
	unsigned int p;
	unsigned char cnt;
	GT_LPORT port_list[MAX_SWITCH_PORTS];

	MV_IF_NULL_RET_STR(qd_dev, -1, "switch dev qd_dev has not been init!\n");

	printk(KERN_INFO "Switch driver init:\n");
	switch (preset) {
	case MV_PRESET_TRANSPARENT:
		printk(KERN_INFO "    o preset mode = Transparent\n");
		break;
	case MV_PRESET_SINGLE_VLAN:
		printk(KERN_INFO "    o preset mode = Single Vlan\n");
		break;
	case MV_PRESET_PER_PORT_VLAN:
		printk(KERN_INFO "    o preset mode = Vlan Per Port\n");
		break;
	default:
		printk(KERN_INFO "    o preset mode = Unknown\n");
	}

	switch (tag_mode) {
	case MV_TAG_TYPE_MH:
		printk(KERN_INFO "    o tag mode    = Marvell Header\n");
		break;
	case MV_TAG_TYPE_DSA:
		printk(KERN_INFO "    o tag mode    = DSA Tag\n");
		break;
	case MV_TAG_TYPE_NONE:
		printk(KERN_INFO "    o tag mode    = No Tag\n");
		break;
	default:
		printk(KERN_INFO "Unknown\n");

	}

	/* set all ports to work in Normal mode */
	for (p = 0; p < qd_dev->numOfPorts; p++) {
		if (MV_BIT_CHECK(switch_ports_mask, p)) {
			if (gprtSetFrameMode(qd_dev, p, GT_FRAME_MODE_NORMAL) != GT_OK) {
				printk(KERN_ERR "gprtSetFrameMode GT_FRAME_MODE_NORMAL failed\n");
				return -1;
			}
		}
	}

	/* set Header Mode in all ports to False */
	for (p = 0; p < qd_dev->numOfPorts; p++) {
		if (MV_BIT_CHECK(switch_ports_mask, p)) {
			if (gprtSetHeaderMode(qd_dev, p, GT_FALSE) != GT_OK) {
				printk(KERN_ERR "gprtSetHeaderMode GT_FALSE failed\n");
				return -1;
			}
		}
	}

	/* cpu port specific intialization */
	if (preset != MV_PRESET_TRANSPARENT) {
		/* set tag mode for CPU port */
		if ((tag_mode == MV_TAG_TYPE_MH) &&
				(gprtSetHeaderMode(qd_dev, qd_cpu_port, GT_TRUE) != GT_OK)) {
			printk(KERN_ERR "gprtSetHeaderMode GT_TRUE failed\n");
			return -1;
		} else if ((tag_mode == MV_TAG_TYPE_DSA) &&
				(gprtSetFrameMode(qd_dev, qd_cpu_port, GT_FRAME_MODE_DSA) != GT_OK)) {
			printk(KERN_ERR "gprtSetFrameMode GT_TRUE failed\n");
			return -1;
		}

		/* set cpu-port with port-based vlan to all other ports */
		SWITCH_DBG(SWITCH_DBG_LOAD, ("cpu port-based vlan:"));
		for (p = 0, cnt = 0; p < qd_dev->numOfPorts; p++) {
			if (p != qd_cpu_port) {
				SWITCH_DBG(SWITCH_DBG_LOAD, ("%d ", p));
				port_list[cnt] = p;
				cnt++;
			}
		}
		SWITCH_DBG(SWITCH_DBG_LOAD, ("\n"));
		if (gvlnSetPortVlanPorts(qd_dev, qd_cpu_port, port_list, cnt) != GT_OK) {
			printk(KERN_ERR "gvlnSetPortVlanPorts failed\n");
			return -1;
		}
	}

	/* The switch CPU port is not part of the VLAN, but rather connected by tunneling to each */
	/* of the VLAN's ports. Our MAC addr will be added during start operation to the VLAN DB  */
	/* at switch level to forward packets with this DA to CPU port.                           */
	SWITCH_DBG(SWITCH_DBG_LOAD, ("Enabling Tunneling on ports: "));
	for (p = 0; p < qd_dev->numOfPorts; p++) {
		if (MV_BIT_CHECK(switch_ports_mask, p) &&
				((p != qd_cpu_port) || (preset == MV_PRESET_TRANSPARENT))) {
			if (gprtSetVlanTunnel(qd_dev, p, GT_TRUE) != GT_OK) {
				printk(KERN_ERR "gprtSetVlanTunnel failed (port %d)\n", p);
				return -1;
			} else {
				SWITCH_DBG(SWITCH_DBG_LOAD, ("%d ", p));
			}
		}
	}
	SWITCH_DBG(SWITCH_DBG_LOAD, ("\n"));

	/* split ports to vlans according to preset */
	if (preset == MV_PRESET_SINGLE_VLAN) {
		mv_switch_vlan_set(MV_SWITCH_GROUP_VLAN_ID(vid), switch_ports_mask);
	} else if (preset == MV_PRESET_PER_PORT_VLAN) {
		for (p = 0; p < qd_dev->numOfPorts; p++)
			if (MV_BIT_CHECK(switch_ports_mask, p) && (p != qd_cpu_port))
				mv_switch_vlan_set(MV_SWITCH_GROUP_VLAN_ID(vid + p), (1 << p));
	}

	if (preset == MV_PRESET_TRANSPARENT) {
		/* enable all relevant ports (ports connected to the MAC or external ports) */
		for (p = 0; p < qd_dev->numOfPorts; p++) {
			if (!MV_BIT_CHECK(switch_ports_mask, p))
				continue;

			if (gstpSetPortState(qd_dev, p, GT_PORT_FORWARDING) != GT_OK) {
				printk(KERN_ERR "gstpSetPortState failed\n");
				return -1;
			}
		}
	} else {
		/* enable cpu port */
		if (gstpSetPortState(qd_dev, qd_cpu_port, GT_PORT_FORWARDING) != GT_OK) {
			printk(KERN_ERR "gstpSetPortState failed\n");
			return -1;
		}
	}

	return 0;
}

void mv_switch_interrupt_mask(void)
{
#ifdef CONFIG_AVANTA_LP
	MV_U32 reg;

	reg = MV_REG_READ(0x18954/*MV_ETHCOMP_INT_MAIN_MASK_REG*/);

	reg &= ~0x1/*MV_ETHCOMP_SWITCH_INT_MASK*/;

	MV_REG_WRITE(0x18954/*MV_ETHCOMP_INT_MAIN_MASK_REG*/, reg);
#endif
}

void mv_switch_interrupt_unmask(void)
{
#ifdef CONFIG_AVANTA_LP
	MV_U32 reg;

	reg = MV_REG_READ(0x18954/*MV_ETHCOMP_INT_MAIN_MASK_REG*/);

	reg |= 0x1/*MV_ETHCOMP_SWITCH_INT_MASK*/;

	MV_REG_WRITE(0x18954/*MV_ETHCOMP_INT_MAIN_MASK_REG*/, reg);
#endif
}

void mv_switch_interrupt_clear(void)
{
#ifdef CONFIG_AVANTA_LP
	MV_U32 reg;

	reg = MV_REG_READ(0x18950/*MV_ETHCOMP_INT_MAIN_CAUSE_REG*/);

	reg &= ~0x1/*MV_ETHCOMP_SWITCH_INT_MASK*/;

	MV_REG_WRITE(0x18950/*MV_ETHCOMP_INT_MAIN_CAUSE_REG*/, reg);
#endif
}

static unsigned int mv_switch_link_detection_init(struct mv_switch_pdata *plat_data)
{
	unsigned int p;
	static int link_init_done = 0;
	unsigned int connected_phys_mask = 0;

	if (qd_dev == NULL) {
		printk(KERN_ERR "%s: qd_dev not initialized, call mv_switch_load() first\n", __func__);
		return 0;
	}

	switch_irq = plat_data->switch_irq;

	connected_phys_mask = plat_data->connected_port_mask & ~(1 << qd_cpu_port);

	if (!link_init_done) {
		/* Enable Phy Link Status Changed interrupt at Phy level for the all enabled ports */
		for (p = 0; p < qd_dev->numOfPorts; p++) {
			if (MV_BIT_CHECK(connected_phys_mask, p) &&
					(!MV_BIT_CHECK(plat_data->forced_link_port_mask, p))) {
				if (gprtPhyIntEnable(qd_dev, p, (GT_LINK_STATUS_CHANGED)) != GT_OK)
					printk(KERN_ERR "gprtPhyIntEnable failed port %d\n", p);
			}
		}

		if (switch_irq != -1) {
			/* Interrupt supported */

			if ((qd_dev->deviceId == GT_88E6161) || (qd_dev->deviceId == GT_88E6165) ||
			    (qd_dev->deviceId == GT_88E6351) || (qd_dev->deviceId == GT_88E6171) ||
			    (qd_dev->deviceId == GT_88E6352)) {
				GT_DEV_EVENT gt_event = { GT_DEV_INT_PHY, 0, connected_phys_mask };

				if (eventSetDevInt(qd_dev, &gt_event) != GT_OK)
					printk(KERN_ERR "eventSetDevInt failed\n");

				if (eventSetActive(qd_dev, GT_DEVICE_INT) != GT_OK)
					printk(KERN_ERR "eventSetActive failed\n");
			} else {
				if (eventSetActive(qd_dev, GT_PHY_INTERRUPT) != GT_OK)
					printk(KERN_ERR "eventSetActive failed\n");
			}
		}
	}

	if (!link_init_done) {
		/* we want to use a timer for polling link status if no interrupt is available for all or some of the PHYs */
		if (switch_irq == -1) {
			/* Use timer for polling */
			switch_link_poll = 1;
			init_timer(&switch_link_timer);
			switch_link_timer.function = mv_switch_link_timer_function;

			if (switch_irq == -1)
				switch_link_timer.data = connected_phys_mask;

			switch_link_timer.expires = jiffies + (HZ);	/* 1 second */
			add_timer(&switch_link_timer);
		} else {
			/* create tasklet for interrupt handling */
			tasklet_init(&link_tasklet, mv_switch_tasklet, 0);

			/* Interrupt supported */
			if (request_irq(switch_irq, mv_switch_isr, (IRQF_DISABLED | IRQF_SAMPLE_RANDOM), "switch", NULL))
				printk(KERN_ERR "failed to assign irq%d\n", switch_irq);

			/* interrupt unmasking will be done by GW manager */
		}
	}

	link_init_done = 1;

	return connected_phys_mask;

}

int mv_switch_tos_get(unsigned char tos)
{
	unsigned char queue;
	int rc;

	MV_IF_NULL_RET_STR(qd_dev, -1, "switch dev qd_dev has not been init!\n");

	rc = gcosGetDscp2Tc(qd_dev, tos >> 2, &queue);
	if (rc)
		return -1;

	return (int)queue;
}

int mv_switch_tos_set(unsigned char tos, int rxq)
{
	MV_IF_NULL_RET_STR(qd_dev, -1, "switch dev qd_dev has not been init!\n");

	return gcosSetDscp2Tc(qd_dev, tos >> 2, (unsigned char)rxq);
}

int mv_switch_get_free_buffers_num(void)
{
	MV_U16 regVal;

	MV_IF_NULL_RET_STR(qd_dev, -1, "switch dev qd_dev has not been init!\n");

	if (gsysGetFreeQSize(qd_dev, &regVal) != GT_OK) {
		printk(KERN_ERR "gsysGetFreeQSize - FAILED\n");
		return -1;
	}

	return regVal;
}

#define QD_FMT "%10lu %10lu %10lu %10lu %10lu %10lu %10lu\n"
#define QD_CNT(c, f) (GT_U32)(c[0]->f), (GT_U32)(c[1]->f), (GT_U32)(c[2]->f), (GT_U32)(c[3]->f),\
			(GT_U32)(c[4]->f), (GT_U32)(c[5]->f), (GT_U32)(c[6]->f)

#define QD_MAX 7
void mv_switch_stats_print(void)
{
	GT_STATS_COUNTER_SET3 * counters[QD_MAX];
	GT_PORT_STAT2 * port_stats[QD_MAX];

	int p;

	if (qd_dev == NULL) {
		printk(KERN_ERR "Switch is not initialized\n");
		return;
	}

	for (p = 0; p < QD_MAX; p++) {
		counters[p] = (GT_STATS_COUNTER_SET3 *)mvOsMalloc(sizeof(GT_STATS_COUNTER_SET3));
		port_stats[p] = (GT_PORT_STAT2 *)mvOsMalloc(sizeof(GT_PORT_STAT2));
		mvOsMemset(counters[p], 0, sizeof(GT_STATS_COUNTER_SET3));
		mvOsMemset(port_stats[p], 0, sizeof(GT_PORT_STAT2));
	}

	printk(KERN_ERR "Total free buffers:      %u\n\n", mv_switch_get_free_buffers_num());

	for (p = 0; p < QD_MAX; p++) {
		if (gstatsGetPortAllCounters3(qd_dev, p, counters[p]) != GT_OK)
			printk(KERN_ERR "gstatsGetPortAllCounters3 for port #%d - FAILED\n", p);

		if (gprtGetPortCtr2(qd_dev, p, port_stats[p]) != GT_OK)
			printk(KERN_ERR "gprtGetPortCtr2 for port #%d - FAILED\n", p);
	}

	printk(KERN_ERR "PortNum         " QD_FMT, (GT_U32) 0, (GT_U32) 1, (GT_U32) 2, (GT_U32) 3, (GT_U32) 4, (GT_U32) 5,
	       (GT_U32) 6);
	printk(KERN_ERR "-----------------------------------------------------------------------------------------------\n");
	printk(KERN_ERR "InGoodOctetsLo  " QD_FMT, QD_CNT(counters, InGoodOctetsLo));
	printk(KERN_ERR "InGoodOctetsHi  " QD_FMT, QD_CNT(counters, InGoodOctetsHi));
	printk(KERN_ERR "InBadOctets     " QD_FMT, QD_CNT(counters, InBadOctets));
	printk(KERN_ERR "InUnicasts      " QD_FMT, QD_CNT(counters, InUnicasts));
	printk(KERN_ERR "InBroadcasts    " QD_FMT, QD_CNT(counters, InBroadcasts));
	printk(KERN_ERR "InMulticasts    " QD_FMT, QD_CNT(counters, InMulticasts));
	printk(KERN_ERR "inDiscardLo     " QD_FMT, QD_CNT(port_stats, inDiscardLo));
	printk(KERN_ERR "inDiscardHi     " QD_FMT, QD_CNT(port_stats, inDiscardHi));
	printk(KERN_ERR "InFiltered      " QD_FMT, QD_CNT(port_stats, inFiltered));

	printk(KERN_ERR "OutOctetsLo     " QD_FMT, QD_CNT(counters, OutOctetsLo));
	printk(KERN_ERR "OutOctetsHi     " QD_FMT, QD_CNT(counters, OutOctetsHi));
	printk(KERN_ERR "OutUnicasts     " QD_FMT, QD_CNT(counters, OutUnicasts));
	printk(KERN_ERR "OutMulticasts   " QD_FMT, QD_CNT(counters, OutMulticasts));
	printk(KERN_ERR "OutBroadcasts   " QD_FMT, QD_CNT(counters, OutBroadcasts));
	printk(KERN_ERR "OutFiltered     " QD_FMT, QD_CNT(port_stats, outFiltered));

	printk(KERN_ERR "OutPause        " QD_FMT, QD_CNT(counters, OutPause));
	printk(KERN_ERR "InPause         " QD_FMT, QD_CNT(counters, InPause));

	printk(KERN_ERR "Octets64        " QD_FMT, QD_CNT(counters, Octets64));
	printk(KERN_ERR "Octets127       " QD_FMT, QD_CNT(counters, Octets127));
	printk(KERN_ERR "Octets255       " QD_FMT, QD_CNT(counters, Octets255));
	printk(KERN_ERR "Octets511       " QD_FMT, QD_CNT(counters, Octets511));
	printk(KERN_ERR "Octets1023      " QD_FMT, QD_CNT(counters, Octets1023));
	printk(KERN_ERR "OctetsMax       " QD_FMT, QD_CNT(counters, OctetsMax));

	printk(KERN_ERR "Excessive       " QD_FMT, QD_CNT(counters, Excessive));
	printk(KERN_ERR "Single          " QD_FMT, QD_CNT(counters, Single));
	printk(KERN_ERR "Multiple        " QD_FMT, QD_CNT(counters, InPause));
	printk(KERN_ERR "Undersize       " QD_FMT, QD_CNT(counters, Undersize));
	printk(KERN_ERR "Fragments       " QD_FMT, QD_CNT(counters, Fragments));
	printk(KERN_ERR "Oversize        " QD_FMT, QD_CNT(counters, Oversize));
	printk(KERN_ERR "Jabber          " QD_FMT, QD_CNT(counters, Jabber));
	printk(KERN_ERR "InMACRcvErr     " QD_FMT, QD_CNT(counters, InMACRcvErr));
	printk(KERN_ERR "InFCSErr        " QD_FMT, QD_CNT(counters, InFCSErr));
	printk(KERN_ERR "Collisions      " QD_FMT, QD_CNT(counters, Collisions));
	printk(KERN_ERR "Late            " QD_FMT, QD_CNT(counters, Late));
	printk(KERN_ERR "OutFCSErr       " QD_FMT, QD_CNT(counters, OutFCSErr));
	printk(KERN_ERR "Deferred        " QD_FMT, QD_CNT(counters, Deferred));

	gstatsFlushAll(qd_dev);

	for (p = 0; p < QD_MAX; p++) {
		mvOsFree(counters[p]);
		mvOsFree(port_stats[p]);
	}
}

static char *mv_str_port_state(GT_PORT_STP_STATE state)
{
	switch (state) {
	case GT_PORT_DISABLE:
		return "Disable";
	case GT_PORT_BLOCKING:
		return "Blocking";
	case GT_PORT_LEARNING:
		return "Learning";
	case GT_PORT_FORWARDING:
		return "Forwarding";
	default:
		return "Invalid";
	}
}

static char *mv_str_speed_state(int port)
{
	GT_PORT_SPEED_MODE speed;
	char *speed_str;

	if (qd_dev == NULL) {
		pr_err("Switch is not initialized\n");
		return "ERR";
	}
	if (gprtGetSpeedMode(qd_dev, port, &speed) != GT_OK) {
		printk(KERN_ERR "gprtGetSpeedMode failed (port %d)\n", port);
		speed_str = "ERR";
	} else {
		if (speed == PORT_SPEED_1000_MBPS)
			speed_str = "1 Gbps";
		else if (speed == PORT_SPEED_100_MBPS)
			speed_str = "100 Mbps";
		else
			speed_str = "10 Mbps";
	}
	return speed_str;
}

static char *mv_str_duplex_state(int port)
{
	GT_BOOL duplex;

	if (qd_dev == NULL) {
		pr_err("Switch is not initialized\n");
		return "ERR";
	}
	if (gprtGetDuplex(qd_dev, port, &duplex) != GT_OK) {
		printk(KERN_ERR "gprtGetDuplex failed (port %d)\n", port);
		return "ERR";
	} else
		return (duplex) ? "Full" : "Half";
}

static char *mv_str_link_state(int port)
{
	GT_BOOL link;

	if (qd_dev == NULL) {
		pr_err("Switch is not initialized\n");
		return "ERR";
	}
	if (gprtGetLinkState(qd_dev, port, &link) != GT_OK) {
		printk(KERN_ERR "gprtGetLinkState failed (port %d)\n", port);
		return "ERR";
	} else
		return (link) ? "Up" : "Down";
}

static char *mv_str_pause_state(int port)
{
	GT_BOOL force, pause;

	if (qd_dev == NULL) {
		pr_err("Switch is not initialized\n");
		return "ERR";
	}
	if (gpcsGetForcedFC(qd_dev, port, &force) != GT_OK) {
		printk(KERN_ERR "gpcsGetForcedFC failed (port %d)\n", port);
		return "ERR";
	}
	if (force) {
		if (gpcsGetFCValue(qd_dev, port, &pause) != GT_OK) {
			printk(KERN_ERR "gpcsGetFCValue failed (port %d)\n", port);
			return "ERR";
		}
	} else {
		if (gprtGetPauseEn(qd_dev, port, &pause) != GT_OK) {
			printk(KERN_ERR "gprtGetPauseEn failed (port %d)\n", port);
			return "ERR";
		}
	}
	return (pause) ? "Enable" : "Disable";
}

static char *mv_str_egress_mode(GT_EGRESS_MODE mode)
{
	switch (mode) {
	case GT_UNMODIFY_EGRESS:
		return "Unmodify";
	case GT_UNTAGGED_EGRESS:
		return "Untagged";
	case GT_TAGGED_EGRESS:
		return "Tagged";
	case GT_ADD_TAG:
		return "Add Tag";
	default:
		return "Invalid";
	}
}

static char *mv_str_frame_mode(GT_FRAME_MODE mode)
{
	switch (mode) {
	case GT_FRAME_MODE_NORMAL:
		return "Normal";
	case GT_FRAME_MODE_DSA:
		return "DSA";
	case GT_FRAME_MODE_PROVIDER:
		return "Provider";
	case GT_FRAME_MODE_ETHER_TYPE_DSA:
		return "EtherType DSA";
	default:
		return "Invalid";
	}
}

static char *mv_str_header_mode(GT_BOOL mode)
{
	switch (mode) {
	case GT_FALSE:
		return "False";
	case GT_TRUE:
		return "True";
	default:
		return "Invalid";
	}
}

void mv_switch_status_print(void)
{
	int p, i;
	GT_PORT_STP_STATE port_state = -1;
	GT_EGRESS_MODE egress_mode = -1;
	GT_FRAME_MODE frame_mode = -1;
	GT_BOOL header_mode = -1;

	if (qd_dev == NULL) {
		printk(KERN_ERR "Switch is not initialized\n");
		return;
	}
	printk(KERN_ERR "Printing Switch Status:\n");

	for (i = 0; i < MV_SWITCH_DB_NUM; i++)
		if (db_port_mask[i])
			printk(KERN_ERR "%d: %d\n", i, db_port_mask[i]);

	printk(KERN_ERR "Port   State     Link   Duplex   Speed    Pause     Egress     Frame    Header\n");
	for (p = 0; p < qd_dev->numOfPorts; p++) {

		if (gstpGetPortState(qd_dev, p, &port_state) != GT_OK)
			printk(KERN_ERR "gstpGetPortState failed\n");

		if (gprtGetEgressMode(qd_dev, p, &egress_mode) != GT_OK)
			printk(KERN_ERR "gprtGetEgressMode failed\n");

		if (gprtGetFrameMode(qd_dev, p, &frame_mode) != GT_OK)
			printk(KERN_ERR "gprtGetFrameMode failed\n");

		if (gprtGetHeaderMode(qd_dev, p, &header_mode) != GT_OK)
			printk(KERN_ERR "gprtGetHeaderMode failed\n");

		printk(KERN_ERR "%2d, %10s,  %4s,  %4s,  %8s,  %7s,  %s,  %s,  %s\n",
		       p, mv_str_port_state(port_state), mv_str_link_state(p),
		       mv_str_duplex_state(p), mv_str_speed_state(p), mv_str_pause_state(p),
		       mv_str_egress_mode(egress_mode), mv_str_frame_mode(frame_mode), mv_str_header_mode(header_mode));
	}
}

int mv_switch_reg_read(int port, int reg, int type, MV_U16 *value)
{
	GT_STATUS status;

	if (qd_dev == NULL) {
		printk(KERN_ERR "Switch is not initialized\n");
		return 1;
	}

	switch (type) {
	case MV_SWITCH_PHY_ACCESS:
		status = gprtGetPhyReg(qd_dev, port, reg, value);
		break;

	case MV_SWITCH_PORT_ACCESS:
		status = gprtGetSwitchReg(qd_dev, port, reg, value);
		break;

	case MV_SWITCH_GLOBAL_ACCESS:
		status = gprtGetGlobalReg(qd_dev, reg, value);
		break;

	case MV_SWITCH_GLOBAL2_ACCESS:
		status = gprtGetGlobal2Reg(qd_dev, reg, value);
		break;

	case MV_SWITCH_SMI_ACCESS:
		/* port means phyAddr */
		status = miiSmiIfReadRegister(qd_dev, port, reg, value);
		break;

	default:
		printk(KERN_ERR "%s Failed: Unexpected access type %d\n", __func__, type);
		return 1;
	}
	if (status != GT_OK) {
		printk(KERN_ERR "%s Failed: status = %d\n", __func__, status);
		return 2;
	}
	return 0;
}

int mv_switch_reg_write(int port, int reg, int type, MV_U16 value)
{
	GT_STATUS status;

	if (qd_dev == NULL) {
		printk(KERN_ERR "Switch is not initialized\n");
		return 1;
	}

	switch (type) {
	case MV_SWITCH_PHY_ACCESS:
		status = gprtSetPhyReg(qd_dev, port, reg, value);
		break;

	case MV_SWITCH_PORT_ACCESS:
		status = gprtSetSwitchReg(qd_dev, port, reg, value);
		break;

	case MV_SWITCH_GLOBAL_ACCESS:
		status = gprtSetGlobalReg(qd_dev, reg, value);
		break;

	case MV_SWITCH_GLOBAL2_ACCESS:
		status = gprtSetGlobal2Reg(qd_dev, reg, value);
		break;

	case MV_SWITCH_SMI_ACCESS:
		/* port means phyAddr */
		status = miiSmiIfWriteRegister(qd_dev, port, reg, value);
		break;

	default:
		printk(KERN_ERR "%s Failed: Unexpected access type %d\n", __func__, type);
		return 1;
	}
	if (status != GT_OK) {
		printk(KERN_ERR "%s Failed: status = %d\n", __func__, status);
		return 2;
	}
	return 0;
}

int mv_switch_all_multicasts_del(int db_num)
{
	GT_STATUS status = GT_OK;
	GT_ATU_ENTRY atu_entry;
	GT_U8 mc_mac[] = { 0x01, 0x00, 0x00, 0x00, 0x00, 0x00 };
	GT_U8 bc_mac[] = { 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF };

	memcpy(atu_entry.macAddr.arEther, &mc_mac, 6);
	atu_entry.DBNum = db_num;

	MV_IF_NULL_RET_STR(qd_dev, -1, "switch dev qd_dev has not been init!\n");

	while ((status = gfdbGetAtuEntryNext(qd_dev, &atu_entry)) == GT_OK) {

		/* Delete only Mcast addresses */
		if (!is_multicast_ether_addr(atu_entry.macAddr.arEther))
			continue;

		/* we don't want to delete the broadcast entry which is the last one */
		if (memcmp(atu_entry.macAddr.arEther, &bc_mac, 6) == 0)
			break;

		SWITCH_DBG(SWITCH_DBG_MCAST, ("Deleting ATU Entry: db = %d, MAC = %02X:%02X:%02X:%02X:%02X:%02X\n",
					      atu_entry.DBNum, atu_entry.macAddr.arEther[0],
					      atu_entry.macAddr.arEther[1], atu_entry.macAddr.arEther[2],
					      atu_entry.macAddr.arEther[3], atu_entry.macAddr.arEther[4],
					      atu_entry.macAddr.arEther[5]));

		if (gfdbDelAtuEntry(qd_dev, &atu_entry) != GT_OK) {
			printk(KERN_ERR "gfdbDelAtuEntry failed\n");
			return -1;
		}
		memcpy(atu_entry.macAddr.arEther, &mc_mac, 6);
		atu_entry.DBNum = db_num;
	}

	return 0;
}

int mv_switch_port_add(int switch_port, u16 grp_id)
{
	int p;
	u16 port_map, vlan_grp_id;

	MV_IF_NULL_RET_STR(qd_dev, -1, "switch dev qd_dev has not been init!\n");
	if (!MV_BIT_CHECK(switch_ports_mask, switch_port)) {
		printk(KERN_ERR "%s: switch port %d is not connected to PHY/CPU\n", __func__, switch_port);
		return -1;
	}

	if (MV_BIT_CHECK(enabled_ports_mask, switch_port)) {
		printk(KERN_ERR "%s: switch port %d is already enabled\n", __func__, switch_port);
		return -1;
	}

	vlan_grp_id = MV_SWITCH_GROUP_VLAN_ID(grp_id);
	/* Add port to port mask of VLAN group */
	port_map = db_port_mask[grp_id] | (1 << switch_port);

	/* Set default VLAN_ID for port */
	if (gvlnSetPortVid(qd_dev, switch_port, MV_SWITCH_PORT_VLAN_ID(vlan_grp_id, switch_port)) != GT_OK) {
		printk(KERN_ERR "gvlnSetPortVid failed\n");
		return -1;
	}
	/* Map port to VLAN DB */
	if (gvlnSetPortVlanDBNum(qd_dev, switch_port, grp_id) != GT_OK) {
		printk(KERN_ERR "gvlnSetPortVlanDBNum failed\n");
		return -1;
	}

	/* Add port to the VLAN (CPU port is not part of VLAN) */
	if (mv_switch_port_based_vlan_set((port_map & ~(1 << qd_cpu_port)), 0) != 0)
		printk(KERN_ERR "mv_switch_port_based_vlan_set failed\n");

	/* Add port to vtu (used in tx) */
	if (mv_switch_vlan_in_vtu_set(vlan_grp_id, grp_id, (port_map | (1 << qd_cpu_port))))
		printk(KERN_ERR "mv_switch_vlan_in_vtu_set failed\n");

	/* set vtu with each port private vlan id (used in rx) */
	for (p = 0; p < qd_dev->numOfPorts; p++) {
		if (MV_BIT_CHECK(port_map, p) && (p != qd_cpu_port)) {
			if (mv_switch_vlan_in_vtu_set(MV_SWITCH_PORT_VLAN_ID(vlan_grp_id, p),
						      grp_id, port_map & ~(1 << qd_cpu_port)) != 0) {
				printk(KERN_ERR "mv_switch_vlan_in_vtu_set failed\n");
			}
		}
	}

	/* Enable port */
	if (gstpSetPortState(qd_dev, switch_port, GT_PORT_FORWARDING) != GT_OK)
		printk(KERN_ERR "gstpSetPortState failed\n");

	/* Enable Phy Link Status Changed interrupt at Phy level for the port */
	if (gprtPhyIntEnable(qd_dev, switch_port, (GT_LINK_STATUS_CHANGED)) != GT_OK)
		printk(KERN_ERR "gprtPhyIntEnable failed port %d\n", switch_port);

	db_port_mask[grp_id] = port_map;
	enabled_ports_mask |= (1 << switch_port);
	/* TODO if new mux */

	return 0;
}

int mv_switch_port_del(int switch_port)
{
	int p;
	u16 port_map, vlan_grp_id, grp_id;

	MV_IF_NULL_RET_STR(qd_dev, -1, "switch dev qd_dev has not been init!\n");
	if (!MV_BIT_CHECK(switch_ports_mask, switch_port)) {
		printk(KERN_ERR "%s: switch port %d is not connected to PHY/CPU\n", __func__, switch_port);
		return -1;
	}

	if (!MV_BIT_CHECK(enabled_ports_mask, switch_port)) {
		printk(KERN_ERR "%s: switch port %d is already disabled\n", __func__, switch_port);
		return -1;
	}

	/* Search for port's DB number */
	for (grp_id = 0; grp_id < MV_SWITCH_DB_NUM; grp_id++)
		if (db_port_mask[grp_id] & (1 << switch_port))
			break;

	if (grp_id == MV_SWITCH_DB_NUM) {
		printk(KERN_ERR "%s: couldn't find port %d VLAN group\n", __func__, switch_port);
		return -1;
	}

	vlan_grp_id = MV_SWITCH_GROUP_VLAN_ID(grp_id);
	/* Add port to port mask of VLAN group */
	port_map = db_port_mask[grp_id] & ~(1 << switch_port);

	/* Disable link change interrupts on unmapped port */
	if (gprtPhyIntEnable(qd_dev, switch_port, 0) != GT_OK)
		printk(KERN_ERR "gprtPhyIntEnable failed on port #%d\n", switch_port);

	/* Disable unmapped port */
	if (gstpSetPortState(qd_dev, switch_port, GT_PORT_DISABLE) != GT_OK)
		printk(KERN_ERR "gstpSetPortState failed on port #%d\n", switch_port);

	/* Remove port from the VLAN (CPU port is not part of VLAN) */
	if (mv_switch_port_based_vlan_set((port_map & ~(1 << qd_cpu_port)), 0) != 0)
		printk(KERN_ERR "mv_gtw_set_port_based_vlan failed\n");

	/* Remove port from vtu (used in tx) */
	if (mv_switch_vlan_in_vtu_set(vlan_grp_id, MV_SWITCH_VLAN_TO_GROUP(vlan_grp_id),
				      (port_map | (1 << qd_cpu_port))) != 0) {
		printk(KERN_ERR "mv_gtw_set_vlan_in_vtu failed\n");
	}

	/* Remove port from vtu of each port private vlan id (used in rx) */
	for (p = 0; p < qd_dev->numOfPorts; p++) {
		if (MV_BIT_CHECK(port_map, p) && (p != qd_cpu_port)) {
			if (mv_switch_vlan_in_vtu_set(MV_SWITCH_PORT_VLAN_ID(vlan_grp_id, p),
						      MV_SWITCH_VLAN_TO_GROUP(vlan_grp_id),
						      (port_map & ~(1 << qd_cpu_port))) != 0)
				printk(KERN_ERR "mv_gtw_set_vlan_in_vtu failed\n");
		}
	}

	db_port_mask[grp_id] = port_map;
	enabled_ports_mask &= ~(1 << switch_port);

	return 0;
}

/*******************************************************************************
* mv_switch_port_discard_tag_set
*
* DESCRIPTION:
*	The API allows or drops all tagged packets based on logical port.
*
* INPUTS:
*	lport - logical port.
*	mode  - discard tag mode.
*		GT_TRUE = discard tagged packets per lport
*		GT_FALSE = allow tagged packets per lport.
*
* OUTPUTS:
*	None.
*
* RETURNS:
*	On success return MV_OK.
*	On error different types are returned according to the case.
*******************************************************************************/
int mv_switch_port_discard_tag_set(unsigned int lport, GT_BOOL mode)
{
	GT_STATUS rc = GT_OK;

	MV_IF_NULL_RET_STR(qd_dev, MV_FAIL, "switch dev qd_dev has not been init!\n");

	rc = gprtSetDiscardTagged(qd_dev, lport, mode);
	SW_IF_ERROR_STR(rc, "failed to call gprtSetDiscardTagged()\n");

	return MV_OK;
}

/*******************************************************************************
* mv_switch_port_discard_tag_get
*
* DESCRIPTION:
*	This routine gets discard tagged bit for given lport.
*
* INPUTS:
*	lport - logical port.
*
* OUTPUTS:
*	mode  - discard tag mode.
*		GT_TRUE = discard tagged packets per lport
*		GT_FALSE = allow tagged packets per lport.
*
* RETURNS:
*       On success -  TPM_RC_OK.
*       On error different types are returned according to the case - see tpm_error_code_t.
*******************************************************************************/
int mv_switch_port_discard_tag_get(unsigned int lport, GT_BOOL *mode)
{
	GT_STATUS rc = GT_OK;

	MV_IF_NULL_RET_STR(qd_dev, MV_FAIL, "switch dev qd_dev has not been init!\n");

	rc = gprtGetDiscardTagged(qd_dev, lport, mode);
	SW_IF_ERROR_STR(rc, "failed to call gprtGetDiscardTagged()\n");

	return MV_OK;
}

/*******************************************************************************
* mv_switch_port_discard_untag_set
*
* DESCRIPTION:
*	The API allows or drops all untagged packets based on logical port.
*
* INPUTS:
*	lport - logical port.
*	mode  - discard untag mode.
*		GT_TRUE = discard untagged packets per lport
*		GT_FALSE = allow untagged packets per lport.
*
* OUTPUTS:
*	None.
*
* RETURNS:
*	On success return MV_OK.
*	On error different types are returned according to the case.
*******************************************************************************/
int mv_switch_port_discard_untag_set(unsigned int lport, GT_BOOL mode)
{
	GT_STATUS rc = GT_OK;

	MV_IF_NULL_RET_STR(qd_dev, MV_FAIL, "switch dev qd_dev has not been init!\n");

	rc = gprtSetDiscardUntagged(qd_dev, lport, mode);
	SW_IF_ERROR_STR(rc, "failed to call gprtSetDiscardUntagged()\n");

	return MV_OK;
}

/*******************************************************************************
* mv_switch_port_discard_untag_get
*
* DESCRIPTION:
*	This routine gets discard untagged bit for given lport.
*
* INPUTS:
*	lport - logical port.
*
* OUTPUTS:
*	mode  - discard untag mode.
*		GT_TRUE = undiscard tagged packets per lport
*		GT_FALSE = unallow tagged packets per lport.
*
* RETURNS:
*       On success -  TPM_RC_OK.
*       On error different types are returned according to the case - see tpm_error_code_t.
*******************************************************************************/
int mv_switch_port_discard_untag_get(unsigned int lport, GT_BOOL *mode)
{
	GT_STATUS rc = GT_OK;

	MV_IF_NULL_RET_STR(qd_dev, MV_FAIL, "switch dev qd_dev has not been init!\n");

	rc = gprtGetDiscardUntagged(qd_dev, lport, mode);
	SW_IF_ERROR_STR(rc, "failed to call gprtGetDiscardUntagged()\n");

	return MV_OK;
}

/*******************************************************************************
* mv_switch_port_def_vid_set
*
* DESCRIPTION:
*	The API sets port default vlan ID.
*
* INPUTS:
*	lport - logical port.
*	vid   - port default VLAN ID.
*
* OUTPUTS:
*	None.
*
* RETURNS:
*	On success return MV_OK.
*	On error different types are returned according to the case.
*******************************************************************************/
int mv_switch_port_def_vid_set(unsigned int lport, unsigned short vid)
{
	GT_STATUS rc = GT_OK;

	MV_IF_NULL_RET_STR(qd_dev, MV_FAIL, "switch dev qd_dev has not been init!\n");

	rc = gvlnSetPortVid(qd_dev, lport, vid);
	SW_IF_ERROR_STR(rc, "failed to call gvlnSetPortVid()\n");

	return MV_OK;
}

/*******************************************************************************
* mv_switch_port_def_vid_get
*
* DESCRIPTION:
*	The API gets port default vlan ID.
*
* INPUTS:
*	lport - logical port.
*
* OUTPUTS:
*	vid   - port default VLAN ID.
*
* RETURNS:
*	On success return MV_OK.
*	On error different types are returned according to the case.
*******************************************************************************/
int mv_switch_port_def_vid_get(unsigned int lport, unsigned short *vid)
{
	GT_STATUS rc = GT_OK;

	MV_IF_NULL_RET_STR(qd_dev, MV_FAIL, "switch dev qd_dev has not been init!\n");

	rc = gvlnGetPortVid(qd_dev, lport, vid);
	SW_IF_ERROR_STR(rc, "failed to call gvlnGetPortVid()\n");

	return MV_OK;
}

/*******************************************************************************
* mv_switch_port_def_pri_set
*
* DESCRIPTION:
*	The API sets port default priority.
*
* INPUTS:
*	lport - logical port.
*	pri   - the port priority.
*
* OUTPUTS:
*	None.
*
* RETURNS:
*	On success return MV_OK.
*	On error different types are returned according to the case.
*******************************************************************************/
int mv_switch_port_def_pri_set(unsigned int lport, unsigned char pri)
{
	GT_STATUS rc = GT_OK;

	MV_IF_NULL_RET_STR(qd_dev, MV_FAIL, "switch dev qd_dev has not been init!\n");

	rc = gcosSetPortDefaultTc(qd_dev, lport, pri);
	SW_IF_ERROR_STR(rc, "failed to call gcosSetPortDefaultTc()\n");

	return MV_OK;
}

/*******************************************************************************
* mv_switch_port_def_pri_get
*
* DESCRIPTION:
*	The API gets port default priority.
*
* INPUTS:
*	lport - logical port.
*
* OUTPUTS:
*	pri   - the port priority.
*
* RETURNS:
*	On success return MV_OK.
*	On error different types are returned according to the case.
*******************************************************************************/
int mv_switch_port_def_pri_get(unsigned int lport, unsigned char *pri)
{
	GT_STATUS rc = GT_OK;

	MV_IF_NULL_RET_STR(qd_dev, MV_FAIL, "switch dev qd_dev has not been init!\n");

	rc = gcosGetPortDefaultTc(qd_dev, lport, pri);
	SW_IF_ERROR_STR(rc, "failed to call gcosGetPortDefaultTc()\n");

	return MV_OK;
}

/*******************************************************************************
* mv_switch_vtu_entry_find
*
* DESCRIPTION:
*	The API find expected VTU entry in sw_vlan_tbl.
*
* INPUTS:
*	vtu_entry - VTU entry, supply VID.
*
* OUTPUTS:
*	found     - find expected entry or not.
*
* RETURNS:
*	On success return MV_OK.
*	On error different types are returned according to the case.
*******************************************************************************/
int mv_switch_vtu_entry_find(GT_VTU_ENTRY *vtu_entry, GT_BOOL *found)
{
	SW_IF_NULL(vtu_entry);
	SW_IF_NULL(found);

	if (sw_vlan_tbl[vtu_entry->vid].port_bm) {/* if port members is not 0, this vid entry exist in HW andd SW */
		memcpy(vtu_entry, &sw_vlan_tbl[vtu_entry->vid].vtu_entry, sizeof(GT_VTU_ENTRY));
		*found = MV_TRUE;
	} else {
		*found = MV_FALSE;
	}

	return MV_OK;
}

/*******************************************************************************
* mv_switch_vtu_entry_save
*
* DESCRIPTION:
*	The API store expected VTU entry in sw_vlan_tbl.
*
* INPUTS:
*	vtu_entry - VTU entry.
*
* OUTPUTS:
*	None.
*
* RETURNS:
*	On success return MV_OK.
*	On error different types are returned according to the case.
*******************************************************************************/
int mv_switch_vtu_entry_save(GT_VTU_ENTRY *vtu_entry)
{
	SW_IF_NULL(vtu_entry);

	memcpy(&sw_vlan_tbl[vtu_entry->vid].vtu_entry, vtu_entry, sizeof(GT_VTU_ENTRY));

	return MV_OK;
}

/*******************************************************************************
* mv_switch_vid_add
*
* DESCRIPTION:
*	The API adds a VID.
*
* INPUTS:
*	lport     - logical switch port ID.
*	vid       - VLAN ID.
*	egr_mode  - egress mode.
*
* OUTPUTS:
*	None.
*
* RETURNS:
*	On success return MV_OK.
*	On error different types are returned according to the case.
*******************************************************************************/
int mv_switch_vid_add(unsigned int lport, unsigned short vid, unsigned char egr_mode)
{
	GT_VTU_ENTRY vtu_entry;
	unsigned int found = GT_FALSE;
	unsigned int port;
	GT_STATUS rc = GT_OK;

	MV_IF_NULL_RET_STR(qd_dev, MV_FAIL, "switch dev qd_dev has not been init!\n");

	memset(&vtu_entry, 0, sizeof(GT_VTU_ENTRY));

	/* Find existed VTU entry in SW cache */
	vtu_entry.vid = vid;
	rc = mv_switch_vtu_entry_find(&vtu_entry, &found);
	SW_IF_ERROR_STR(rc, "failed to call mv_switch_vtu_entry_find()\n");

	/* Add new VTU entry in case VTU entry does not exist */
	if (found == GT_FALSE) {
		vtu_entry.DBNum				= 0;
		vtu_entry.vid				= vid;
		vtu_entry.vidPriOverride		= GT_FALSE;
		vtu_entry.vidPriority			= 0;
		vtu_entry.vidExInfo.useVIDFPri		= GT_FALSE;
		vtu_entry.vidExInfo.vidFPri		= 0;
		vtu_entry.vidExInfo.useVIDQPri		= GT_FALSE;
		vtu_entry.vidExInfo.vidQPri		= 0;
		vtu_entry.vidExInfo.vidNRateLimit	= GT_FALSE;

	}

	/* Update VTU entry */
	for (port = 0; port < qd_dev->numOfPorts; port++) {
		if (sw_vlan_tbl[vid].port_bm & (1 << port)) {
			if (port == lport)
				vtu_entry.vtuData.memberTagP[port] = egr_mode;/* update egress mode only */
			else
				vtu_entry.vtuData.memberTagP[port] = sw_vlan_tbl[vid].egr_mode[port];
		} else if (port == lport) {
			vtu_entry.vtuData.memberTagP[port] = egr_mode;
		} else if ((sw_port_tbl[port].port_mode == GT_FALLBACK) || (qd_dev->cpuPortNum == port)) {
			/* add cpu_port to VLAN if cpu_port is valid */
			vtu_entry.vtuData.memberTagP[port] = MEMBER_EGRESS_UNMODIFIED;
		} else {
			vtu_entry.vtuData.memberTagP[port] = NOT_A_MEMBER;
		}
	}

	/* Add/Update HW VTU entry */
	rc = gvtuAddEntry(qd_dev, &vtu_entry);
	SW_IF_ERROR_STR(rc, "failed to call gvtuAddEntry()\n");

	/* Record HW VTU entry info to sw_vlan_tbl */
	rc = mv_switch_vtu_entry_save(&vtu_entry);
	SW_IF_ERROR_STR(rc, "failed to call mv_switch_vtu_entry_save()\n");

	return MV_OK;
}

/*******************************************************************************
* mv_switch_port_vid_add
*
* DESCRIPTION:
*	The API adds a VID per lport.
*
* INPUTS:
*	lport     - logical switch port ID.
*	vid       - VLAN ID.
*	egr_mode  - egress mode.
*	belong    - whether this port blong to the VLAN actually
*
* OUTPUTS:
*	None.
*
* RETURNS:
*	On success return MV_OK.
*	On error different types are returned according to the case.
*******************************************************************************/
int mv_switch_port_vid_add(unsigned int lport, unsigned short vid, unsigned char egr_mode, bool belong)
{
	GT_STATUS rc = GT_OK;
	unsigned int  port_bm;
	unsigned int port_idx;

	MV_IF_NULL_RET_STR(qd_dev, MV_FAIL, "switch dev qd_dev has not been init!\n");

	/* Verify whether the port is already a member of this VID */
	port_bm = (unsigned int)(1 << lport);
	if (!(sw_vlan_tbl[vid].port_bm & port_bm) || (sw_vlan_tbl[vid].egr_mode[lport] != egr_mode)) {
		rc = mv_switch_vid_add(lport, vid, egr_mode);
		SW_IF_ERROR_STR(rc, "failed to call mv_switch_vid_add()\n");

		/* add port to vid's member bit map and set port egress mode */
		sw_vlan_tbl[vid].port_bm |= port_bm;
		sw_vlan_tbl[vid].egr_mode[lport] = egr_mode;
	}

	/* add fallback port or CPU port to this VLAN in DB */
	for (port_idx = 0; port_idx < qd_dev->numOfPorts; port_idx++) {
		/* If a VLAN has been defined (there is a member in the VLAN) and
		   the specified port is not a member */
		if (!(sw_vlan_tbl[vid].port_bm & (1 << port_idx)) &&
		     ((sw_port_tbl[port_idx].port_mode == GT_FALLBACK) ||
		      (port_idx == qd_dev->cpuPortNum))) {
			sw_vlan_tbl[vid].port_bm |= (1 << port_idx);
			sw_vlan_tbl[vid].egr_mode[port_idx] = MEMBER_EGRESS_UNMODIFIED;
		}
	}

	/* Add the specified port to the SW Port table */
	if ((true == belong) && (sw_port_tbl[lport].vlan_blong[vid] == MV_SWITCH_PORT_NOT_BELONG))
		sw_port_tbl[lport].vlan_blong[vid] = MV_SWITCH_PORT_BELONG;

	return MV_OK;
}

/*******************************************************************************
* mv_switch_port_vid_del
*
* DESCRIPTION:
*	The API delete existed VID per lport.
*
* INPUTS:
*	lport     - logical switch port ID.
*	vid       - VLAN ID.
*
* OUTPUTS:
*	None.
*
* RETURNS:
*	On success return MV_OK.
*	On error different types are returned according to the case.
*******************************************************************************/
int mv_switch_port_vid_del(unsigned int lport, unsigned short vid)
{
	unsigned int port_bm;
	GT_VTU_ENTRY vtu_entry;
	unsigned int found = GT_TRUE;
	unsigned int port_idx;
	unsigned int is_vlan_member = 0;
	GT_STATUS rc = GT_OK;

	MV_IF_NULL_RET_STR(qd_dev, MV_FAIL, "switch dev qd_dev has not been init!\n");

	/* Do nothing if the port does not in this VLAN */
	port_bm = (unsigned int)(1 << lport);
	if (!(sw_vlan_tbl[vid].port_bm & port_bm)) {
		pr_err("%s(%d) port(%d) is not in VLAN(%d)\n", __func__, __LINE__, lport, vid);
		return MV_OK;
	}

	/* Find VTU entry in SW cache */
	memset(&vtu_entry, 0, sizeof(GT_VTU_ENTRY));
	vtu_entry.vid = vid;
	rc = mv_switch_vtu_entry_find(&vtu_entry, &found);
	SW_IF_ERROR_STR(rc, "failed to call mv_switch_vtu_entry_find()\n");

	/* Step 1. Mark the lport as NOT_A_MEMBER. */
	vtu_entry.vtuData.memberTagP[lport] = NOT_A_MEMBER;

	/* 2. Search whether a secure port is a member of the VLAN */
	for (port_idx = 0; port_idx < qd_dev->numOfPorts; port_idx++) {
		if ((vtu_entry.vtuData.memberTagP[port_idx] != NOT_A_MEMBER) &&
			((sw_port_tbl[port_idx].port_mode == GT_SECURE) ||
			(sw_port_tbl[port_idx].vlan_blong[vid] == MV_SWITCH_PORT_BELONG))) {
			is_vlan_member = 1;
			break;
	    }
	}

	/* Update the VTU entry */
	if (is_vlan_member) {
		/* Add HW VTU entry */
		rc = gvtuAddEntry(qd_dev, &vtu_entry);
		SW_IF_ERROR_STR(rc, "failed to call mv_switch_vtu_entry_find()\n");

		/* Record HW VTU entry info to sw_vlan_tbl */
		rc = mv_switch_vtu_entry_save(&vtu_entry);
		SW_IF_ERROR_STR(rc, "failed to call mv_switch_vtu_entry_save()\n");

		/* Delete port from VID DB */
		sw_vlan_tbl[vid].port_bm &= ~port_bm;
	} else {
		/* Delete the VTU entry */
		rc = gvtuDelEntry(qd_dev, &vtu_entry);
		SW_IF_ERROR_STR(rc, "failed to call gvtuDelEntry()\n");

		sw_vlan_tbl[vid].port_bm = 0;
	}

	if (sw_port_tbl[lport].vlan_blong[vid] == MV_SWITCH_PORT_BELONG) {
		sw_port_tbl[lport].vlan_blong[vid] = MV_SWITCH_PORT_NOT_BELONG;
		sw_vlan_tbl[vid].egr_mode[lport] = NOT_A_MEMBER;
	}

	return MV_OK;
}

/*******************************************************************************
* mv_switch_vid_get
*
* DESCRIPTION:
*	The API get VID information.
*
* INPUTS:
*	vid       - VLAN ID.
*
* OUTPUTS:
*	vtu_entry - VTU entry.
*	found     - MV_TRUE, if the appropriate entry exists.
*
* RETURNS:
*	On success return MV_OK.
*	On error different types are returned according to the case.
*******************************************************************************/
int mv_switch_vid_get(unsigned int vid, GT_VTU_ENTRY *vtu_entry, unsigned int *found)
{
	GT_STATUS rc = GT_OK;

	MV_IF_NULL_RET_STR(qd_dev, MV_FAIL, "switch dev qd_dev has not been init!\n");

	memset(vtu_entry, 0, sizeof(GT_VTU_ENTRY));
	vtu_entry->vid = vid;
	rc = gvtuFindVidEntry(qd_dev, vtu_entry, found);
	SW_IF_ERROR_STR(rc, "failed to call gvtuFindVidEntry()\n");

	return MV_OK;
}

/*******************************************************************************
* mv_switch_port_vid_egress_mode_set
*
* DESCRIPTION:
*	The API sets the egress mode for a member port of a vlan.
*
* INPUTS:
*	lport    - logical switch port ID.
*       vid      - vlan id.
*       egr_mode - egress mode.
*
* OUTPUTS:
*       None.
*
* RETURNS:
*	On success return MV_OK.
*	On error different types are returned according to the case.
*
* COMMENTS:
*       MEMBER_EGRESS_UNMODIFIED - 0
*       NOT_A_MEMBER             - 1
*       MEMBER_EGRESS_UNTAGGED   - 2
*       MEMBER_EGRESS_TAGGED     - 3
*
*******************************************************************************/
int mv_switch_port_vid_egress_mode_set(unsigned int lport, unsigned short vid, unsigned char egr_mode)
{
	GT_STATUS    rc = GT_OK;
	GT_VTU_ENTRY vtu_entry;
	GT_BOOL      found = GT_FALSE;

	MV_IF_NULL_RET_STR(qd_dev, MV_FAIL, "switch dev qd_dev has not been init!\n");

	/* If the port is the member of vlan, set */
	if (sw_vlan_tbl[vid].port_bm & (1 << lport)) {
		sw_vlan_tbl[vid].egr_mode[lport] = egr_mode;

		memset(&vtu_entry, 0, sizeof(GT_VTU_ENTRY));
		vtu_entry.vid = vid;

		rc = mv_switch_vtu_entry_find(&vtu_entry, &found);
		if (rc != GT_OK && rc != GT_NO_SUCH) {
			pr_err("%s(%d) rc(%d) failed to call mv_switch_vtu_entry_find()\n",
			       __func__, __LINE__, rc);

			return MV_FAIL;
		}

		vtu_entry.vtuData.memberTagP[lport] = egr_mode;

		rc = gvtuAddEntry(qd_dev, &vtu_entry);
		SW_IF_ERROR_STR(rc, "failed to call gvtuAddEntry()\n");

		/* Record HW VT entry info to sw_vlan_tbl */
		rc = mv_switch_vtu_entry_save(&vtu_entry);
		SW_IF_ERROR_STR(rc, "failed to call mv_switch_vtu_entry_save()\n");
	} else {
		pr_err("%s(%d) port(%d) is not the member of vlan(%d)\n",
		       __func__, __LINE__, lport, vid);
		return MV_FAIL;
	}

	return MV_OK;
}

/*******************************************************************************
* mv_switch_unknown_unicast_flood_set
*
* DESCRIPTION:
*	This routine enable/disable unknown unicast frame egress on a specific port.
*
* INPUTS:
*	lport   - logical switch port ID.
*	enable  - Enable unknown unicast flooding.
*
* OUTPUTS:
*       None.
*
* RETURNS:
*	On success return MV_OK.
*	On error different types are returned according to the case.
*******************************************************************************/
int mv_switch_unknown_unicast_flood_set(unsigned char lport, GT_BOOL enable)
{
	GT_STATUS rc = GT_OK;

	MV_IF_NULL_RET_STR(qd_dev, MV_FAIL, "switch dev qd_dev has not been init!\n");

	rc = gprtSetForwardUnknown(qd_dev, lport, enable);
	SW_IF_ERROR_STR(rc, "failed to call gprtSetForwardUnknown()\n");

	return MV_OK;
}

/*******************************************************************************
* mv_switch_unknown_unicast_flood_get
*
* DESCRIPTION:
*       This routine gets unknown unicast frame egress mode of a specific port.
*
* INPUTS:
*	lport   - logical switch port ID.
*
* OUTPUTS:
*	enable  - Enable unknown unicast flooding.
*
* RETURNS:
*	On success return MV_OK.
*	On error different types are returned according to the case.
*******************************************************************************/
int mv_switch_unknown_unicast_flood_get(unsigned char lport, GT_BOOL *enable)
{
	GT_STATUS rc = GT_OK;

	MV_IF_NULL_RET_STR(qd_dev, MV_FAIL, "switch dev qd_dev has not been init!\n");

	rc = gprtGetForwardUnknown(qd_dev, lport, enable);
	SW_IF_ERROR_STR(rc, "failed to call gprtGetForwardUnknown()\n");

	return MV_OK;
}

/*******************************************************************************
* mv_switch_unknown_multicast_flood_set
*
* DESCRIPTION:
*	This routine enable/disable unknown multicast frame egress on a specific port.
*
* INPUTS:
*	lport   - logical switch port ID.
*	enable  - Enable unknown multicast flooding.
*
* OUTPUTS:
*	None.
*
* RETURNS:
*	On success return MV_OK.
*	On error different types are returned according to the case.
*******************************************************************************/
int mv_switch_unknown_multicast_flood_set(unsigned char lport, GT_BOOL enable)
{
	GT_STATUS rc = GT_OK;

	MV_IF_NULL_RET_STR(qd_dev, MV_FAIL, "switch dev qd_dev has not been init!\n");

	rc = gprtSetDefaultForward(qd_dev, lport, enable);
	SW_IF_ERROR_STR(rc, "failed to call gprtSetDefaultForward()\n");

	return MV_OK;
}

/*******************************************************************************
* mv_switch_unknown_multicast_flood_get
*
* DESCRIPTION:
*	This routine gets unknown multicast frame egress mode of a specific port.
*
* INPUTS:
*	lport   - logical switch port ID.
*
* OUTPUTS:
*	enable  - Enable unknown multicast flooding.
*
* RETURNS:
*	On success return MV_OK.
*	On error different types are returned according to the case.
*******************************************************************************/
int mv_switch_unknown_multicast_flood_get(unsigned char lport, GT_BOOL *enable)
{
	GT_STATUS rc = GT_OK;

	MV_IF_NULL_RET_STR(qd_dev, MV_FAIL, "switch dev qd_dev has not been init!\n");

	rc = gprtGetDefaultForward(qd_dev, lport, enable);
	SW_IF_ERROR_STR(rc, "failed to call gprtGetDefaultForward()\n");

	return MV_OK;
}

/*******************************************************************************
* mv_switch_broadcast_flood_set
*
* DESCRIPTION:
*	This routine decides whether the switch always floods the broadcast
*	frames to all portsr or uses the multicast egress mode (per port).
*
* INPUTS:
*	enable - enable broadcast flooding regardless the multicast egress mode.
*
* OUTPUTS:
*	None.
*
* RETURNS:
*	On success return MV_OK.
*	On error different types are returned according to the case.
*******************************************************************************/
int mv_switch_broadcast_flood_set(GT_BOOL enable)
{
	GT_STATUS rc = GT_OK;

	MV_IF_NULL_RET_STR(qd_dev, MV_FAIL, "switch dev qd_dev has not been init!\n");

	rc = gsysSetFloodBC(qd_dev, enable);
	SW_IF_ERROR_STR(rc, "failed to call gsysSetFloodBC()\n");

	return MV_OK;
}

/*******************************************************************************
* mv_switch_broadcast_flood_get
*
* DESCRIPTION:
*	This routine gets the global mode of broadcast flood.
*
* INPUTS:
*	None.
*
* OUTPUTS:
*	enable - always floods the broadcast regardless the multicast egress mode.
*
* RETURNS:
*	On success return MV_OK.
*	On error different types are returned according to the case.
*******************************************************************************/
int mv_switch_broadcast_flood_get(GT_BOOL *enable)
{
	GT_STATUS rc = GT_OK;

	MV_IF_NULL_RET_STR(qd_dev, MV_FAIL, "switch dev qd_dev has not been init!\n");

	rc = gsysGetFloodBC(qd_dev, enable);
	SW_IF_ERROR_STR(rc, "failed to call gsysGetFloodBC()\n");

	return MV_OK;
}

/*******************************************************************************
* mv_switch_port_count3_get
*
* DESCRIPTION:
*	This function gets all counter 3 of the given port
*
* INPUTS:
*	lport - logical switch port ID.
*
* OUTPUTS:
*	count - all port counter 3.
*
* RETURNS:
*	On success return MV_OK.
*	On error different types are returned according to the case.
*
* COMMENTS:
*	Clear on read.
*******************************************************************************/
int mv_switch_port_count3_get(unsigned int lport, GT_STATS_COUNTER_SET3 *count)
{
	GT_STATUS rc = GT_OK;

	MV_IF_NULL_RET_STR(qd_dev, MV_FAIL, "switch dev qd_dev has not been init!\n");

	rc = gstatsGetPortAllCounters3(qd_dev, lport, count);
	SW_IF_ERROR_STR(rc, "failed to call gstatsGetPortAllCounters3()\n");

	return MV_OK;
}

/*******************************************************************************
* mv_switch_port_drop_count_get
*
* DESCRIPTION:
*	This function gets the port InDiscards, InFiltered, and OutFiltered counters.
*
* INPUTS:
*	lport - logical switch port ID.
*
* OUTPUTS:
*	count - all port dropped counter.
*
* RETURNS:
*	On success return MV_OK.
*	On error different types are returned according to the case.
*
* COMMENTS:
*	Clear on read.
*******************************************************************************/
int mv_switch_port_drop_count_get(unsigned int lport, GT_PORT_STAT2 *count)
{
	GT_STATUS rc = GT_OK;

	MV_IF_NULL_RET_STR(qd_dev, MV_FAIL, "switch dev qd_dev has not been init!\n");

	rc = gprtGetPortCtr2(qd_dev, lport, count);
	SW_IF_ERROR_STR(rc, "failed to call gprtGetPortCtr2()\n");

	return MV_OK;
}

/*******************************************************************************
* mv_switch_port_count_clear
*
* DESCRIPTION:
*	This function clean all counters of the given port
*
* INPUTS:
*	lport - logical switch port ID.
*
* OUTPUTS:
*       None.
*
* RETURNS:
*	On success return MV_OK.
*	On error different types are returned according to the case.
*******************************************************************************/
int mv_switch_port_count_clear(unsigned int lport)
{
	GT_STATUS rc = GT_OK;

	MV_IF_NULL_RET_STR(qd_dev, MV_FAIL, "switch dev qd_dev has not been init!\n");

	rc = gstatsFlushPort(qd_dev, lport);
	SW_IF_ERROR_STR(rc, "failed to call gstatsFlushPort()\n");

	return MV_OK;
}

/*******************************************************************************
* mv_switch_count_clear
*
* DESCRIPTION:
*	This function gets all counters of the given port
*
* INPUTS:
*       None.
*
* OUTPUTS:
*       None.
*
* RETURNS:
*	On success return MV_OK.
*	On error different types are returned according to the case.
*******************************************************************************/
int mv_switch_count_clear(void)
{
	GT_STATUS rc = GT_OK;

	MV_IF_NULL_RET_STR(qd_dev, MV_FAIL, "switch dev qd_dev has not been init!\n");

	rc = gstatsFlushAll(qd_dev);
	SW_IF_ERROR_STR(rc, "failed to call gstatsFlushAll()\n");

	return MV_OK;
}

/*******************************************************************************
* mv_switch_ingr_limit_mode_set
*
* DESCRIPTION:
*	This routine sets the port's rate control ingress limit mode.
*
* INPUTS:
*	lport - logical switch port ID.
*	mode  - rate control ingress limit mode.
*
* OUTPUTS:
*       None.
*
* RETURNS:
*	On success return MV_OK.
*	On error different types are returned according to the case.
*
* COMMENTS:
*	GT_LIMT_ALL = 0,        limit and count all frames
*	GT_LIMIT_FLOOD,         limit and count Broadcast, Multicast and flooded unicast frames
*	GT_LIMIT_BRDCST_MLTCST, limit and count Broadcast and Multicast frames
*	GT_LIMIT_BRDCST         limit and count Broadcast frames
*
*******************************************************************************/
int mv_switch_ingr_limit_mode_set(unsigned int lport, GT_RATE_LIMIT_MODE mode)
{
	GT_STATUS rc = GT_OK;

	MV_IF_NULL_RET_STR(qd_dev, MV_FAIL, "switch dev qd_dev has not been init!\n");

	rc = grcSetLimitMode(qd_dev, lport, mode);
	SW_IF_ERROR_STR(rc, "failed to call grcSetLimitMode()\n");

	return MV_OK;
}

/*******************************************************************************
* mv_switch_ingr_limit_mode_get
*
* DESCRIPTION:
*	This routine gets the port's rate control ingress limit mode.
*
* INPUTS:
*	lport - logical switch port ID.
*
* OUTPUTS:
*	mode  - rate control ingress limit mode.
*
* RETURNS:
*	On success return MV_OK.
*	On error different types are returned according to the case.
*
* COMMENTS:
*	GT_LIMT_ALL = 0,        limit and count all frames
*	GT_LIMIT_FLOOD,         limit and count Broadcast, Multicast and flooded unicast frames
*	GT_LIMIT_BRDCST_MLTCST, limit and count Broadcast and Multicast frames
*	GT_LIMIT_BRDCST         limit and count Broadcast frames
*
*******************************************************************************/
int mv_switch_ingr_limit_mode_get(unsigned int lport, GT_RATE_LIMIT_MODE *mode)
{
	GT_STATUS rc = GT_OK;

	MV_IF_NULL_RET_STR(qd_dev, MV_FAIL, "switch dev qd_dev has not been init!\n");

	rc = grcGetLimitMode(qd_dev, lport, mode);
	SW_IF_ERROR_STR(rc, "failed to call grcSetLimitMode()\n");

	return MV_OK;
}

/*******************************************************************************
* mv_switch_ingr_police_rate_get
*
* DESCRIPTION:
*	The API gets the ingress policing rate for given switch port.
*
* INPUTS:
*	lport      - logical switch port ID.
*
* OUTPUTS:
*	count_mode - policing rate count mode:
*			GT_PIRL2_COUNT_FRAME = 0
*			GT_PIRL2_COUNT_ALL_LAYER1
*			GT_PIRL2_COUNT_ALL_LAYER2
*			GT_PIRL2_COUNT_ALL_LAYER3
*	cir        - committed infomation rate.
*
* RETURNS:
*	On success return MV_OK.
*	On error different types are returned according to the case.
*******************************************************************************/
int mv_switch_ingr_police_rate_get(unsigned int		lport,
				   GT_PIRL2_COUNT_MODE	*count_mode,
				   unsigned int		*cir)
{
	GT_PIRL2_DATA	pirl_2_Data;
	GT_U32		irl_unit;
	GT_STATUS	rc = GT_OK;

	/* IRL Unit 0 - bucket to be used (0 ~ 4) */
	irl_unit =  0;
	memset(&pirl_2_Data, 0, sizeof(GT_PIRL2_DATA));

	MV_IF_NULL_RET_STR(qd_dev, MV_FAIL, "switch dev qd_dev has not been init!\n");

	rc = gpirl2ReadResource(qd_dev, lport, irl_unit, &pirl_2_Data);
	SW_IF_ERROR_STR(rc, "failed to call gpirl2ReadResource()\n");

	*count_mode	= pirl_2_Data.byteTobeCounted;
	*cir		= pirl_2_Data.ingressRate;

	return MV_OK;
}

/*******************************************************************************
* mv_switch_egr_rate_limit_set
*
* DESCRIPTION:
*	The API Configures the egress frame rate limit of logical port.
* INPUTS:
*	lport - logical switch port ID.
*	mode  - egress rate limit mode.
*       rate  - egress rate limit value.
*
* OUTPUTS:
*       None.
*
* RETURNS:
*	On success return MV_OK.
*	On error different types are returned according to the case.
*
* COMMENTS:
*	GT_ERATE_TYPE used kbRate - frame rate valid values are:
*	7600,..., 9600,
*	10000, 20000, 30000, 40000, ..., 100000,
*	110000, 120000, 130000, ..., 1000000.
*******************************************************************************/
int mv_switch_egr_rate_limit_set(unsigned int lport, GT_PIRL_ELIMIT_MODE mode, unsigned int rate)
{
	GT_ERATE_TYPE	fRate;
	GT_STATUS	rc = GT_OK;

	MV_IF_NULL_RET_STR(qd_dev, MV_FAIL, "switch dev qd_dev has not been init!\n");

	fRate.fRate  = rate;
	fRate.kbRate = rate;

	rc = grcSetELimitMode(qd_dev, lport, mode);
	SW_IF_ERROR_STR(rc, "failed to call grcSetELimitMode()\n");


	rc = grcSetEgressRate(qd_dev, lport, &fRate);
	SW_IF_ERROR_STR(rc, "failed to call grcSetEgressRate()\n");

	return MV_OK;
}

/*******************************************************************************
* mv_switch_egr_rate_limit_get
*
* DESCRIPTION:
*	The API return the egress frame rate limit of logical port
* INPUTS:
*	lport - logical switch port ID.
*
* OUTPUTS:
*	mode  - egress rate limit mode.
*       rate  - egress rate limit value.
*
* RETURNS:
*	On success return MV_OK.
*	On error different types are returned according to the case.
*
* COMMENTS:
*	GT_ERATE_TYPE used kbRate - frame rate valid values are:
*	7600,..., 9600,
*	10000, 20000, 30000, 40000, ..., 100000,
*	110000, 120000, 130000, ..., 1000000.
*******************************************************************************/
int mv_switch_egr_rate_limit_get(unsigned int lport, GT_PIRL_ELIMIT_MODE *mode, unsigned int *rate)
{
	GT_ERATE_TYPE	fRate;
	GT_STATUS		rc = GT_OK;

	MV_IF_NULL_RET_STR(qd_dev, MV_FAIL, "switch dev qd_dev has not been init!\n");

	rc = grcGetELimitMode(qd_dev, lport, mode);
	SW_IF_ERROR_STR(rc, "failed to call grcGetELimitMode()\n");

	rc = grcGetEgressRate(qd_dev, lport, &fRate);
	SW_IF_ERROR_STR(rc, "failed to call grcGetEgressRate()\n");

	if (mode == GT_PIRL_ELIMIT_FRAME) {
		/* frame based limit */
		*rate = fRate.fRate;
	} else {
		/* rate based limit */
		*rate = fRate.kbRate;
	}

	return MV_OK;
}

/*******************************************************************************
* mv_switch_ingr_broadcast_rate_get
*
* DESCRIPTION:
*	The API gets the ingress broacast rate for given switch port.
*
* INPUTS:
*	lport      - logical switch port ID.
*
* OUTPUTS:
*	count_mode - policing rate count mode:
*			GT_PIRL2_COUNT_FRAME = 0
*			GT_PIRL2_COUNT_ALL_LAYER1
*			GT_PIRL2_COUNT_ALL_LAYER2
*			GT_PIRL2_COUNT_ALL_LAYER3
*	cir        - committed infomation rate.
*
* RETURNS:
*	On success return MV_OK.
*	On error different types are returned according to the case.
*******************************************************************************/
int mv_switch_ingr_broadcast_rate_get(unsigned int		lport,
				   GT_PIRL2_COUNT_MODE	*count_mode,
				   unsigned int		*cir)
{
	GT_PIRL2_DATA	pirl_2_Data;
	GT_U32		irl_unit;
	GT_STATUS	rc = GT_OK;

	MV_IF_NULL_RET_STR(qd_dev, MV_FAIL, "switch dev qd_dev has not been init!\n");

	/* IRL Unit 0 - bucket to be used (0 ~ 4) */
	irl_unit =  MV_SWITCH_PIRL_RESOURCE_BROADCAST;
	memset(&pirl_2_Data, 0, sizeof(GT_PIRL2_DATA));

	rc = gpirl2ReadResource(qd_dev, lport, irl_unit, &pirl_2_Data);
	SW_IF_ERROR_STR(rc, "failed to call gpirl2ReadResource()\n");

	*count_mode	= pirl_2_Data.byteTobeCounted;
	*cir		= pirl_2_Data.ingressRate;

	return MV_OK;
}

/*******************************************************************************
* mv_switch_ingr_multicast_rate_get
*
* DESCRIPTION:
*	The API gets the ingress broacast rate for given switch port.
*
* INPUTS:
*	lport      - logical switch port ID.
*
* OUTPUTS:
*	count_mode - policing rate count mode:
*			GT_PIRL2_COUNT_FRAME = 0
*			GT_PIRL2_COUNT_ALL_LAYER1
*			GT_PIRL2_COUNT_ALL_LAYER2
*			GT_PIRL2_COUNT_ALL_LAYER3
*	cir        - committed infomation rate.
*
* RETURNS:
*	On success return MV_OK.
*	On error different types are returned according to the case.
*******************************************************************************/
int mv_switch_ingr_multicast_rate_get(unsigned int		lport,
				   GT_PIRL2_COUNT_MODE	*count_mode,
				   unsigned int		*cir)
{
	GT_PIRL2_DATA	pirl_2_Data;
	GT_U32		irl_unit;
	GT_STATUS	rc = GT_OK;

	MV_IF_NULL_RET_STR(qd_dev, MV_FAIL, "switch dev qd_dev has not been init!\n");

	/* IRL Unit 0 - bucket to be used (0 ~ 4) */
	irl_unit =  MV_SWITCH_PIRL_RESOURCE_MULTICAST;
	memset(&pirl_2_Data, 0, sizeof(GT_PIRL2_DATA));

	rc = gpirl2ReadResource(qd_dev, lport, irl_unit, &pirl_2_Data);
	SW_IF_ERROR_STR(rc, "failed to call gpirl2ReadResource()\n");

	*count_mode	= pirl_2_Data.byteTobeCounted;
	*cir		= pirl_2_Data.ingressRate;

	return MV_OK;
}

/*******************************************************************************
* mv_switch_port_mirror_set
*
* DESCRIPTION:
*	Set port mirror.
*
* INPUTS:
*	sport  - Source port.
*	mode   - mirror mode.
*	enable - enable/disable mirror.
*	dport  - Destination port.
*
* OUTPUTS:
*	None.
*
* RETURNS:
*	On success return MV_OK.
*	On error different types are returned according to the case.
*******************************************************************************/
int mv_switch_port_mirror_set(unsigned int sport, enum sw_mirror_mode_t mode, GT_BOOL enable, unsigned int dport)
{
	GT_STATUS rc = GT_OK;

	MV_IF_NULL_RET_STR(qd_dev, MV_FAIL, "switch dev qd_dev has not been init!\n");

	if (mode == MV_SWITCH_MIRROR_INGRESS) {
		if (enable == GT_TRUE) {
			/* set ingress monitor source */
			rc = gprtSetIngressMonitorSource(qd_dev, sport, GT_TRUE);
			SW_IF_ERROR_STR(rc, "failed to call gprtSetIngressMonitorSource()\n");

			/* set ingress monitor destination */
			rc = gsysSetIngressMonitorDest(qd_dev, dport);
			SW_IF_ERROR_STR(rc, "failed to call gsysSetIngressMonitorDest()\n");
		} else {
			/*disable ingress monitor source */
			rc = gprtSetIngressMonitorSource(qd_dev, sport, GT_FALSE);
			SW_IF_ERROR_STR(rc, "failed to call gprtSetIngressMonitorSource()\n");
		}
	} else if (mode == MV_SWITCH_MIRROR_EGRESS) {
		if (enable == GT_TRUE) {
			/* enable egress monitor source */
			rc = gprtSetEgressMonitorSource(qd_dev, sport, GT_TRUE);
			SW_IF_ERROR_STR(rc, "failed to call gprtSetEgressMonitorSource()\n");

			/* set egress monitor destination */
			rc = gsysSetEgressMonitorDest(qd_dev, dport);
			SW_IF_ERROR_STR(rc, "failed to call gsysSetEgressMonitorDest()\n");
		} else {
			/* disable egress monitor source */
			rc = gprtSetEgressMonitorSource(qd_dev, sport, GT_FALSE);
			SW_IF_ERROR_STR(rc, "failed to call gprtSetEgressMonitorSource()\n");
		}
	} else if (mode ==  MV_SWITCH_MIRROR_BOTH) {
		if (enable == GT_TRUE) {
			/* enable egress monitor source */
			rc = gprtSetIngressMonitorSource(qd_dev, sport, GT_TRUE);
			SW_IF_ERROR_STR(rc, "failed to call gprtSetIngressMonitorSource()\n");

			/* set ingress monitor destination */
			rc = gsysSetIngressMonitorDest(qd_dev, dport);
			SW_IF_ERROR_STR(rc, "failed to call gsysSetIngressMonitorDest()\n");

			/* enable egress monitor source */
			rc = gprtSetEgressMonitorSource(qd_dev, sport, GT_TRUE);
			SW_IF_ERROR_STR(rc, "failed to call gprtSetEgressMonitorSource()\n");

			/* set egress monitor destination */
			rc = gsysSetEgressMonitorDest(qd_dev, dport);
			SW_IF_ERROR_STR(rc, "failed to call gsysSetEgressMonitorDest()\n");
		} else {
			/*disable ingress monitor source */
			rc = gprtSetIngressMonitorSource(qd_dev, sport, GT_FALSE);
			SW_IF_ERROR_STR(rc, "failed to call gprtSetIngressMonitorSource()\n");

			/* disable egress monitor source */
			rc = gprtSetEgressMonitorSource(qd_dev, sport, GT_FALSE);
			SW_IF_ERROR_STR(rc, "failed to call gprtSetEgressMonitorSource()\n");
		}
	} else {
		pr_err("illegal port mirror dir(%d)\n", mode);
		return MV_FAIL;
	}
	return MV_OK;
}

/*******************************************************************************
* mv_switch_port_mirror_get
*
* DESCRIPTION:
*	Get port mirror status.
*
* INPUTS:
*	sport  - Source port.
*	mode   - mirror mode.
*
* OUTPUTS:
*	enable - enable/disable mirror.
*	dport  - Destination port.
*
* RETURNS:
*	On success return MV_OK.
*	On error different types are returned according to the case.
*******************************************************************************/
int mv_switch_port_mirror_get(unsigned int sport, enum sw_mirror_mode_t mode, GT_BOOL *enable, unsigned int *dport)
{
	GT_LPORT port;
	GT_STATUS rc = GT_OK;

	MV_IF_NULL_RET_STR(qd_dev, MV_FAIL, "switch dev qd_dev has not been init!\n");

	if (mode == MV_SWITCH_MIRROR_INGRESS) {
		/* Get ingress monitor source status */
		rc = gprtGetIngressMonitorSource(qd_dev, (GT_LPORT)sport, enable);
		SW_IF_ERROR_STR(rc, "failed to call gprtGetIngressMonitorSource()\n");

		/* Get ingress destination port */
		rc = gsysGetIngressMonitorDest(qd_dev, &port);
		SW_IF_ERROR_STR(rc, "failed to call gsysGetIngressMonitorDest()\n");
		*dport = port;

	} else if (mode == MV_SWITCH_MIRROR_EGRESS) {
		/* Get egress monitor source status */
		rc = gprtGetEgressMonitorSource(qd_dev, (GT_LPORT)sport, enable);
		SW_IF_ERROR_STR(rc, "failed to call gprtGetEgressMonitorSource()\n");

		/* Get egress destination port */
		rc = gsysGetEgressMonitorDest(qd_dev, &port);
		SW_IF_ERROR_STR(rc, "failed to call gsysGetEgressMonitorDest()\n");
		*dport = port;

	} else {
		pr_err("illegal port mirror dir(%d)\n", mode);
		return MV_FAIL;
	}

	return MV_OK;
}

/*******************************************************************************
* mv_switch_age_time_set
*
* DESCRIPTION:
*	This function sets the MAC address aging time.
*
* INPUTS:
*	time - aging time value.
*
* OUTPUTS:
*	None.
*
* RETURNS:
*	On success return MV_OK.
*	On error different types are returned according to the case.
*******************************************************************************/
int mv_switch_age_time_set(unsigned int time)
{
	GT_STATUS rc = GT_OK;

	MV_IF_NULL_RET_STR(qd_dev, MV_FAIL, "switch dev qd_dev has not been init!\n");

	rc = gfdbSetAgingTimeout(qd_dev, time);
	SW_IF_ERROR_STR(rc, "failed to call gfdbSetAgingTimeout()\n");

	return MV_OK;
}

/*******************************************************************************
* mv_switch_age_time_get
*
* DESCRIPTION:
*	This function gets the MAC address aging time.
*
* INPUTS:
*	None.
*
* OUTPUTS:
*	time - MAC aging time.
*
* RETURNS:
*	On success return MV_OK.
*	On error different types are returned according to the case.
*******************************************************************************/
int mv_switch_age_time_get(unsigned int *time)
{
	GT_STATUS rc = GT_OK;

	MV_IF_NULL_RET_STR(qd_dev, MV_FAIL, "switch dev qd_dev has not been init!\n");

	rc = gfdbGetAgingTimeout(qd_dev, (GT_U32 *)time);
	SW_IF_ERROR_STR(rc, "failed to call gfdbGetAgingTimeout()\n");

	return MV_OK;
}

/*******************************************************************************
* mv_switch_mac_learn_disable_set
*
* DESCRIPTION:
*	Enable/disable automatic learning of new source MAC addresses on port
*	ingress direction.
*
* INPUTS:
*	lport  - logical switch port ID.
*	enable - enable/disable MAC learning.
*		GT_TRUE: disable MAC learning
*		GT_FALSE: enable MAC learning
*
* OUTPUTS:
*	None.
*
* RETURNS:
*	On success return MV_OK.
*	On error different types are returned according to the case.
*******************************************************************************/
int mv_switch_mac_learn_disable_set(unsigned int lport, GT_BOOL enable)
{
	GT_STATUS rc = GT_OK;

	MV_IF_NULL_RET_STR(qd_dev, MV_FAIL, "switch dev qd_dev has not been init!\n");

	rc = gprtSetLearnDisable(qd_dev, lport, enable);
	SW_IF_ERROR_STR(rc, "failed to call gprtSetLearnDisable()\n");

	return MV_OK;
}

/*******************************************************************************
* mv_switch_mac_learn_disable_get
*
* DESCRIPTION:
*	Get automatic learning status of new source MAC addresses on port ingress.
*
* INPUTS:
*	lport  - logical switch port ID.
*
* OUTPUTS:
*	enable - enable/disable MAC learning.
*		GT_TRUE: disable MAC learning
*		GT_FALSE: enable MAC learning
*
* RETURNS:
*	On success return MV_OK.
*	On error different types are returned according to the case.
*******************************************************************************/
int mv_switch_mac_learn_disable_get(unsigned int lport, GT_BOOL *enable)
{
	GT_STATUS rc = GT_OK;

	MV_IF_NULL_RET_STR(qd_dev, MV_FAIL, "switch dev qd_dev has not been init!\n");

	rc = gprtGetLearnDisable(qd_dev, lport, enable);
	SW_IF_ERROR_STR(rc, "failed to call gprtGetLearnDisable()\n");

	return MV_OK;
}
#ifdef CONFIG_ARCH_AVANTA_LP

/*******************************************************************************
* mv_switch_ingr_police_rate_set
*
* DESCRIPTION:
*	The API configures the ingress policing rate for given switch port.
*
* INPUTS:
*	lport      - logical switch port ID.
*	count_mode - policing rate count mode:
*			GT_PIRL2_COUNT_FRAME = 0
*			GT_PIRL2_COUNT_ALL_LAYER1
*			GT_PIRL2_COUNT_ALL_LAYER2
*			GT_PIRL2_COUNT_ALL_LAYER3
*	cir        - committed infomation rate.
*	bktTypeMask - ingress packet type mask
*
* OUTPUTS:
*	None.
*
* RETURNS:
*	On success return MV_OK.
*	On error different types are returned according to the case.
*******************************************************************************/
int mv_switch_ingr_police_rate_set(unsigned int	lport,
				   GT_PIRL2_COUNT_MODE	count_mode,
				   unsigned int		cir,
				   GT_U32		bktTypeMask)
{
	GT_U32		irlRes;
	GT_PIRL2_DATA	pirl_2_Data;
	GT_BOOL		pause_state;
	GT_STATUS	rc = GT_OK;

	MV_IF_NULL_RET_STR(qd_dev, MV_FAIL, "switch dev qd_dev has not been init!\n");

	memset(&pirl_2_Data, 0, sizeof(pirl_2_Data));

	irlRes = 0;

	/* configure cir, count_mode */
	pirl_2_Data.ingressRate		= cir;
	pirl_2_Data.customSetup.isValid	= GT_FALSE;
	pirl_2_Data.accountQConf	= GT_FALSE;
	pirl_2_Data.accountFiltered	= GT_TRUE;
	pirl_2_Data.mgmtNrlEn		= GT_TRUE;
	pirl_2_Data.saNrlEn		= GT_FALSE;
	pirl_2_Data.daNrlEn		= GT_FALSE;
	pirl_2_Data.samplingMode	= GT_FALSE;
	pirl_2_Data.actionMode		= PIRL_ACTION_USE_LIMIT_ACTION;

	/* decide which mode to adopt when deal with overload traffic.
	*  If pause state is ON, select FC mode, otherwize select drop mode.
	*/
	rc = mv_phy_port_pause_state_get(lport, &pause_state);
	SW_IF_ERROR_STR(rc, "failed to call mv_phy_port_pause_state_get()\n");
	if (pause_state == GT_TRUE)
		pirl_2_Data.ebsLimitAction = ESB_LIMIT_ACTION_FC;
	else
		pirl_2_Data.ebsLimitAction = ESB_LIMIT_ACTION_DROP;

	pirl_2_Data.fcDeassertMode	= GT_PIRL_FC_DEASSERT_EMPTY;
	pirl_2_Data.bktRateType		= BUCKET_TYPE_TRAFFIC_BASED;
	pirl_2_Data.priORpt		= GT_TRUE;
	pirl_2_Data.priMask		= 0;
	pirl_2_Data.bktTypeMask		= bktTypeMask;
	pirl_2_Data.byteTobeCounted	= count_mode;

	rc = gpirl2WriteResource(qd_dev, lport, irlRes, &pirl_2_Data);
	SW_IF_ERROR_STR(rc, "failed to call gpirl2WriteResource()\n");

	return MV_OK;
}

/*******************************************************************************
* mv_switch_ingr_broadcast_rate_set
*
* DESCRIPTION:
*	The API configures the ingress broadcast rate for given switch port.
*
* INPUTS:
*	lport      - logical switch port ID.
*	count_mode - policing rate count mode:
*			GT_PIRL2_COUNT_FRAME = 0
*			GT_PIRL2_COUNT_ALL_LAYER1
*			GT_PIRL2_COUNT_ALL_LAYER2
*			GT_PIRL2_COUNT_ALL_LAYER3
*	cir        - committed infomation rate.
*
* OUTPUTS:
*	None.
*
* RETURNS:
*	On success return MV_OK.
*	On error different types are returned according to the case.
*******************************************************************************/
int mv_switch_ingr_broadcast_rate_set(unsigned int		lport,
					GT_PIRL2_COUNT_MODE	count_mode,
					unsigned int	cir)
{
	GT_U32		irlRes;
	GT_PIRL2_DATA	pirl_2_Data;
	GT_BOOL		pause_state;
	GT_STATUS	rc = GT_OK;

	MV_IF_NULL_RET_STR(qd_dev, MV_FAIL, "switch dev qd_dev has not been init!\n");

	memset(&pirl_2_Data, 0, sizeof(pirl_2_Data));

	irlRes = MV_SWITCH_PIRL_RESOURCE_BROADCAST;

	/* configure cir, count_mode */
	pirl_2_Data.ingressRate		= cir;
	pirl_2_Data.customSetup.isValid	= GT_FALSE;
	pirl_2_Data.accountQConf	= GT_FALSE;
	pirl_2_Data.accountFiltered	= GT_TRUE;
	pirl_2_Data.mgmtNrlEn		= GT_TRUE;
	pirl_2_Data.saNrlEn		= GT_FALSE;
	pirl_2_Data.daNrlEn		= GT_FALSE;
	pirl_2_Data.samplingMode	= GT_FALSE;
	pirl_2_Data.actionMode		= PIRL_ACTION_USE_LIMIT_ACTION;

	/* decide which mode to adopt when deal with overload traffic.
	*  If pause state is ON, select FC mode, otherwize select drop mode.
	*/
	rc = mv_phy_port_pause_state_get(lport, &pause_state);
	SW_IF_ERROR_STR(rc, "failed to call mv_phy_port_pause_state_get()\n");
	if (pause_state == GT_TRUE)
		pirl_2_Data.ebsLimitAction = ESB_LIMIT_ACTION_FC;
	else
		pirl_2_Data.ebsLimitAction = ESB_LIMIT_ACTION_DROP;

	pirl_2_Data.fcDeassertMode	= GT_PIRL_FC_DEASSERT_EMPTY;
	pirl_2_Data.bktRateType		= BUCKET_TYPE_TRAFFIC_BASED;
	pirl_2_Data.priORpt		= GT_TRUE;
	pirl_2_Data.priMask		= 0;
	pirl_2_Data.bktTypeMask		= (1 << MV_SWITCH_PIRL_BKTTYPR_BROADCAST_BIT);
	pirl_2_Data.byteTobeCounted	= count_mode;

	rc = gpirl2WriteResource(qd_dev, lport, irlRes, &pirl_2_Data);
	SW_IF_ERROR_STR(rc, "failed to call gpirl2WriteResource()\n");

	return MV_OK;
}

/*******************************************************************************
* mv_switch_ingr_multicast_rate_set
*
* DESCRIPTION:
*	The API configures the ingress broadcast rate for given switch port.
*
* INPUTS:
*	lport      - logical switch port ID.
*	count_mode - policing rate count mode:
*			GT_PIRL2_COUNT_FRAME = 0
*			GT_PIRL2_COUNT_ALL_LAYER1
*			GT_PIRL2_COUNT_ALL_LAYER2
*			GT_PIRL2_COUNT_ALL_LAYER3
*	cir        - committed infomation rate.
*
* OUTPUTS:
*	None.
*
* RETURNS:
*	On success return MV_OK.
*	On error different types are returned according to the case.
*******************************************************************************/
int mv_switch_ingr_multicast_rate_set(unsigned int		lport,
				   GT_PIRL2_COUNT_MODE	count_mode,
				   unsigned int cir)
{
	GT_U32		irlRes;
	GT_PIRL2_DATA	pirl_2_Data;
	GT_BOOL		pause_state;
	GT_STATUS	rc = GT_OK;

	MV_IF_NULL_RET_STR(qd_dev, MV_FAIL, "switch dev qd_dev has not been init!\n");

	memset(&pirl_2_Data, 0, sizeof(pirl_2_Data));

	irlRes = MV_SWITCH_PIRL_RESOURCE_MULTICAST;

	/* configure cir, count_mode */
	pirl_2_Data.ingressRate		= cir;
	pirl_2_Data.customSetup.isValid	= GT_FALSE;
	pirl_2_Data.accountQConf	= GT_FALSE;
	pirl_2_Data.accountFiltered	= GT_TRUE;
	pirl_2_Data.mgmtNrlEn		= GT_TRUE;
	pirl_2_Data.saNrlEn		= GT_FALSE;
	pirl_2_Data.daNrlEn		= GT_FALSE;
	pirl_2_Data.samplingMode	= GT_FALSE;
	pirl_2_Data.actionMode		= PIRL_ACTION_USE_LIMIT_ACTION;

	/* decide which mode to adopt when deal with overload traffic.
	*  If pause state is ON, select FC mode, otherwize select drop mode.
	*/
	rc = mv_phy_port_pause_state_get(lport, &pause_state);
	SW_IF_ERROR_STR(rc, "failed to call mv_phy_port_pause_state_get()\n");
	if (pause_state == GT_TRUE)
		pirl_2_Data.ebsLimitAction = ESB_LIMIT_ACTION_FC;
	else
		pirl_2_Data.ebsLimitAction = ESB_LIMIT_ACTION_DROP;

	pirl_2_Data.fcDeassertMode	= GT_PIRL_FC_DEASSERT_EMPTY;
	pirl_2_Data.bktRateType		= BUCKET_TYPE_TRAFFIC_BASED;
	pirl_2_Data.priORpt		= GT_TRUE;
	pirl_2_Data.priMask		= 0;
	pirl_2_Data.bktTypeMask		= ((1 << MV_SWITCH_PIRL_BKTTYPR_MULTICAST_BIT)
		| (1 << MV_SWITCH_PIRL_BKTTYPR_UNKNOWN_MULTICAST_BIT));
	pirl_2_Data.byteTobeCounted	= count_mode;

	rc = gpirl2WriteResource(qd_dev, lport, irlRes, &pirl_2_Data);
	SW_IF_ERROR_STR(rc, "failed to call gpirl2WriteResource()\n");

	return MV_OK;
}

/*******************************************************************************
* mv_switch_queue_weight_set
*
* DESCRIPTION:
*	The API configures the weight of a queues for all
*	Ethernet UNI ports in the integrated switch.
*
* INPUTS:
*	lport  - logical switch port ID.
*	queue  - switch queue, ranging from 0 to 3.
*	weight - weight value per queue (1-8).
*
* OUTPUTS:
*	None.
*
* RETURNS:
*	On success return MV_OK.
*	On error different types are returned according to the case.
*******************************************************************************/
int mv_switch_queue_weight_set(unsigned int lport, unsigned char queue, unsigned char weight)
{
	unsigned int len = 0;
	unsigned int offset = 0;
	unsigned int idx;
	GT_QoS_WEIGHT l_weight;
	GT_STATUS rc = GT_OK;

	MV_IF_NULL_RET_STR(qd_dev, MV_FAIL, "switch dev qd_dev has not been init!\n");

	/* get weight information at first */
	rc = gsysGetQoSWeight(qd_dev, &l_weight);
	SW_IF_ERROR_STR(rc, "failed to call gsysGetQoSWeight()\n");

	offset = MV_SWITCH_MAX_QUEUE_NUM*lport + queue;
	if (offset >= MAX_QOS_WEIGHTS) {
		pr_err("%s offset(%d) is out of range\n", __func__, offset);
		return MV_FAIL;
	}

	/* Update queue weight */
	if ((offset+1) <= l_weight.len) {
		l_weight.queue[offset] = weight;
		len = l_weight.len;
	} else {
		for (idx = l_weight.len; idx < (offset/MV_SWITCH_MAX_QUEUE_NUM+1)*MV_SWITCH_MAX_QUEUE_NUM; idx++) {
			if (idx == offset)
				l_weight.queue[idx] = weight;
			else
				l_weight.queue[idx] = MV_SWITCH_DEFAULT_WEIGHT;
		}

		len = (offset/MV_SWITCH_MAX_QUEUE_NUM+1)*MV_SWITCH_MAX_QUEUE_NUM;
	}

	l_weight.len = len;

	rc = gsysSetQoSWeight(qd_dev, &l_weight);
	SW_IF_ERROR_STR(rc, "failed to call gsysSetQoSWeight()\n");

	return MV_OK;
}

/*******************************************************************************
* mv_switch_queue_weight_get
*
* DESCRIPTION:
*	The API configures the weight of a queues for all
*	Ethernet UNI ports in the integrated switch.
*
* INPUTS:
*	lport  - logical switch port ID.
*	queue  - switch queue, ranging from 0 to 3.
*
* OUTPUTS:
*	weight - weight value per queue (1-8).
*
* RETURNS:
*	On success return MV_OK.
*	On error different types are returned according to the case.
*******************************************************************************/
int mv_switch_queue_weight_get(unsigned int lport, unsigned char queue, unsigned char *weight)
{
	GT_QoS_WEIGHT l_weight;
	unsigned int offset;
	GT_STATUS rc = GT_OK;

	MV_IF_NULL_RET_STR(qd_dev, MV_FAIL, "switch dev qd_dev has not been init!\n");

	/* Get QoS queue information */
	rc = gsysGetQoSWeight(qd_dev, &l_weight);
	SW_IF_ERROR_STR(rc, "failed to call gsysSetQoSWeight()\n");

	offset = MV_SWITCH_MAX_QUEUE_NUM*lport + queue;
	if ((offset + 1) > l_weight.len)
		*weight = 0;
	else
		*weight = l_weight.queue[offset];

	return MV_OK;
}

/*******************************************************************************
* mv_switch_learn2all_enable_set
*
* DESCRIPTION:
*	Enable/disable learn to all devices
*
* INPUTS:
*	enable - enable/disable learn to all devices.
*		GT_TRUE: disable MAC learning
*		GT_FALSE: enable MAC learning
*
* OUTPUTS:
*	None.
*
* RETURNS:
*	On success return MV_OK.
*	On error different types are returned according to the case.
*******************************************************************************/
int mv_switch_learn2all_enable_set(GT_BOOL enable)
{
	GT_STATUS rc = GT_OK;

	MV_IF_NULL_RET_STR(qd_dev, MV_FAIL, "switch dev qd_dev has not been init!\n");

	rc = gsysSetLearn2All(qd_dev, enable);
	SW_IF_ERROR_STR(rc, "failed to call gsysSetLearn2All(%d)\n", enable);

	return MV_OK;
}

/*******************************************************************************
* mv_switch_learn2all_enable_get
*
* DESCRIPTION:
*	returns if the learn2all bit status
*
* INPUTS:
*	None.
*
* OUTPUTS:
*	enabled - learn2all enabled/disabled
*		GT_TRUE: learn2all enabled
*		GT_FALSE: learn2all disabled
*
* RETURNS:
*	On success return MV_OK.
*	On error different types are returned according to the case.
*******************************************************************************/
int mv_switch_learn2all_enable_get(GT_BOOL *enabled)
{
	GT_STATUS rc = GT_OK;

	MV_IF_NULL_RET_STR(qd_dev, MV_FAIL, "switch dev qd_dev has not been init!\n");

	rc = gsysGetLearn2All(qd_dev, enabled);
	SW_IF_ERROR_STR(rc, "failed to call gsysSetLearn2All()\n");

	return MV_OK;
}
#endif
/*******************************************************************************
* mv_switch_mac_limit_set
*
* DESCRIPTION:
*	This function limits the number of MAC addresses per lport.
*
* INPUTS:
*	lport   - logical switch port ID.
*	mac_num - maximum number of MAC addresses per port (0-255).
*
* OUTPUTS:
*	None.
*
* RETURNS:
*	On success return MV_OK.
*	On error different types are returned according to the case.
*
* COMMENTS:
*	The following care is needed when enabling this feature:
*		1) disable learning on the ports
*		2) flush all non-static addresses in the ATU
*		3) define the desired limit for the ports
*		4) re-enable learing on the ports
*******************************************************************************/
int mv_switch_mac_limit_set(unsigned int lport, unsigned int mac_num)
{
	GT_STATUS rc = GT_OK;

	MV_IF_NULL_RET_STR(qd_dev, MV_FAIL, "switch dev qd_dev has not been init!\n");

	/* define the desired limit for the ports */
	rc = gfdbSetPortAtuLearnLimit(qd_dev, lport, mac_num);
	SW_IF_ERROR_STR(rc, "failed to call gfdbSetPortAtuLearnLimit()\n");

	return MV_OK;
}

/*******************************************************************************
* mv_switch_mac_limit_get
*
* DESCRIPTION
*	Port's auto learning limit. When the limit is non-zero value, the number
*	of MAC addresses that can be learned on this lport are limited to the value
*	specified in this API. When the learn limit has been reached any frame
*	that ingresses this lport with a source MAC address not already in the
*	address database that is associated with this lport will be discarded.
*	Normal auto-learning will resume on the lport as soon as the number of
*	active unicast MAC addresses associated to this lport is less than the
*	learn limit.
*	CPU directed ATU Load, Purge, or Move will not have any effect on the
*	learn limit.
*	This feature is disabled when the limit is zero.
*	The following care is needed when enabling this feature:
*		1) dsable learning on the ports
*		2) flush all non-static addresses in the ATU
*		3) define the desired limit for the ports
*		4) re-enable learing on the ports
*
* INPUTS:
*	lport   - logical switch port ID.
*
* OUTPUTS:
*	mac_num - maximum number of MAC addresses per port (0-255).
*
* RETURNS:
*	On success return MV_OK.
*	On error different types are returned according to the case.
*******************************************************************************/
int mv_switch_mac_limit_get(unsigned int lport, unsigned int *mac_num)
{
	GT_STATUS rc = GT_OK;

	MV_IF_NULL_RET_STR(qd_dev, MV_FAIL, "switch dev qd_dev has not been init!\n");

	rc = gfdbGetPortAtuLearnLimit(qd_dev, lport, (GT_U32 *)mac_num);
	SW_IF_ERROR_STR(rc, "failed to call gfdbGetPortAtuLearnLimit()\n");

	return MV_OK;
}

/*******************************************************************************
* mv_switch_mac_addr_add
*
* DESCRIPTION:
*	This function creates a MAC entry in the MAC address table for a
*	specific lport in the integrated switch
*
* INPUTS:
*	port_bm  - logical switch port bitmap, bit0: switch port 0, bit1: port 1.
*	mac_addr - 6byte network order MAC source address.
*	mode     - Static or dynamic mode.
*
* OUTPUTS:
*	None.
*
* RETURNS:
*	On success return MV_OK.
*	On error different types are returned according to the case.
*******************************************************************************/
int mv_switch_mac_addr_add(unsigned int port_bm, unsigned char mac_addr[6], unsigned int mode)
{
	GT_ATU_ENTRY mac_entry;
	unsigned int l_port_bm = 0;
	GT_STATUS    rc = GT_OK;
	enum sw_mac_addr_type_t type = MV_SWITCH_UNICAST_MAC_ADDR;

	MV_IF_NULL_RET_STR(qd_dev, MV_FAIL, "switch dev qd_dev has not been init!\n");

	memset(&mac_entry, 0, sizeof(GT_ATU_ENTRY));

	mac_entry.trunkMember			= GT_FALSE;
	mac_entry.prio				= 0;
	mac_entry.exPrio.useMacFPri		= 0;
	mac_entry.exPrio.macFPri		= 0;
	mac_entry.exPrio.macQPri		= 0;
	mac_entry.DBNum				= 0;
	l_port_bm				= port_bm;
	memcpy(mac_entry.macAddr.arEther, mac_addr, GT_ETHERNET_HEADER_SIZE);

	/* treat broadcast MAC address as multicast one */
	if (((mac_addr[0] & 0x01) == 0x01) ||
		((mac_addr[0] == 0x33) && (mac_addr[1] == 0x33))) {
		type = MV_SWITCH_MULTICAST_MAC_ADDR;
		l_port_bm |= (1 << qd_dev->cpuPortNum);
	}
	mac_entry.portVec = l_port_bm;

	if (type == MV_SWITCH_UNICAST_MAC_ADDR) {
		if (mode == MV_SWITCH_DYNAMIC_MAC_ADDR)
			mac_entry.entryState.ucEntryState = GT_UC_DYNAMIC;
		else
			mac_entry.entryState.ucEntryState = GT_UC_STATIC;
	} else {
		mac_entry.entryState.mcEntryState = GT_MC_STATIC;
	}

	/* add ATU entry */
	rc = gfdbAddMacEntry(qd_dev, &mac_entry);
	SW_IF_ERROR_STR(rc, "failed to call gfdbAddMacEntry()\n");

	return MV_OK;
}

/*******************************************************************************
* mv_switch_mac_addr_del
*
* DESCRIPTION:
*	This function removes an existed MAC entry from the MAC address
*	table in the integrated switch.
*
* INPUTS:
*	lport    - logical switch port ID.
*       mac_addr - MAC address.
*
* OUTPUTS:
*	None.
*
* RETURNS:
*	On success return MV_OK.
*	On error different types are returned according to the case.
*******************************************************************************/
int mv_switch_mac_addr_del(unsigned int lport, unsigned char mac_addr[6])
{
	GT_ATU_ENTRY mac_entry;
	GT_BOOL      found;
	GT_BOOL      mc_addr = GT_FALSE;
	GT_STATUS    rc = GT_OK;

	MV_IF_NULL_RET_STR(qd_dev, MV_FAIL, "switch dev qd_dev has not been init!\n");

	/* try to find VTU entry */
	memset(&mac_entry, 0, sizeof(GT_ATU_ENTRY));
	memcpy(mac_entry.macAddr.arEther, mac_addr, GT_ETHERNET_HEADER_SIZE);
	rc = gfdbFindAtuMacEntry(qd_dev, &mac_entry, &found);
	SW_IF_ERROR_STR(rc, "failed to call gfdbFindAtuMacEntry()\n");

	/* return ok in case no ATU entry is found */
	if (GT_FALSE == found)
		return MV_OK;

	/* delete ATU entry */
	rc = gfdbDelMacEntry(qd_dev, &mac_entry.macAddr);
	SW_IF_ERROR_STR(rc, "failed to call gfdbDelMacEntry()\n");

	/* treat broadcast MAC address as multicast one */
	if (((mac_addr[0] & 0x01) == 0x01) ||
	    ((mac_addr[0] == 0x33) && (mac_addr[1] == 0x33))) {
		mc_addr = GT_TRUE;
	}

	/* add ATU again in case there still have other ports */
	if (((mac_entry.portVec & ~(1 << lport)) && (mc_addr == GT_FALSE)) ||
	    ((mac_entry.portVec & ~((1 << lport) | (1 << qd_dev->cpuPortNum))) && (mc_addr == GT_TRUE))) {
		mac_entry.portVec &= ~(1 << lport);
		rc = gfdbAddMacEntry(qd_dev, &mac_entry);
		SW_IF_ERROR_STR(rc, "failed to call gfdbAddMacEntry()\n");
	}

	return MV_OK;
}

/*******************************************************************************
* mv_switch_port_qos_mode_set()
*
* DESCRIPTION:
*	Configures the scheduling mode per logical port.
*
* INPUTS:
*	lport - logical switch port ID.
*	mode  - scheduler mode.
*
* OUTPUTS:
*	None.
*
* RETURNS:
*	On success return MV_OK.
*	On error different types are returned according to the case.
*******************************************************************************/
int mv_switch_port_qos_mode_set(unsigned int lport, GT_PORT_SCHED_MODE mode)
{
	GT_STATUS rc = GT_OK;

	MV_IF_NULL_RET_STR(qd_dev, MV_FAIL, "switch dev qd_dev has not been init!\n");

	rc = gprtSetPortSched(qd_dev, lport, mode);
	SW_IF_ERROR_STR(rc, "failed to call gprtSetPortSched()\n");

	return MV_OK;
}

/*******************************************************************************
* mv_switch_port_qos_mode_get()
*
* DESCRIPTION:
*	This API gets the scheduling mode per logical port.
*
* INPUTS:
*	lport - logical switch port ID.
*
* OUTPUTS:
*	mode  - scheduler mode.
*
* RETURNS:
*	On success return MV_OK.
*	On error different types are returned according to the case.
*******************************************************************************/
int mv_switch_port_qos_mode_get(unsigned int lport, GT_PORT_SCHED_MODE *mode)
{
	GT_STATUS rc = GT_OK;

	MV_IF_NULL_RET_STR(qd_dev, MV_FAIL, "switch dev qd_dev has not been init!\n");

	rc = gprtGetPortSched(qd_dev, lport, mode);
	SW_IF_ERROR_STR(rc, "failed to call gprtGetPortSched()\n");

	return MV_OK;
}

/*******************************************************************************
* mv_switch_mtu_set
*
* DESCRIPTION:
*	Set switch MTU size.
*
* INPUTS:
*	mtu - MTU size.
*
* OUTPUTS:
*	None
*
* RETURNS:
*	On success return MV_OK.
*	On error different types are returned according to the case.
*******************************************************************************/
int mv_switch_mtu_set(unsigned int mtu)
{
	unsigned int idx;
	GT_JUMBO_MODE jumbo_mode;
	GT_STATUS rc = GT_OK;

	MV_IF_NULL_RET_STR(qd_dev, MV_FAIL, "switch dev qd_dev has not been init!\n");

	/* Set jumbo frames mode */
	if (mtu < 1522) {
		pr_err("MTU(%d) will be adjusted to jumbo mode(1522)\n", mtu);
		jumbo_mode = GT_JUMBO_MODE_1522;
	} else if (mtu == 1522) {
		jumbo_mode = GT_JUMBO_MODE_1522;
	} else if (mtu < 2048) {
		pr_err("MTU(%d) will be adjusted to jumbo mode(2048)\n", mtu);
		jumbo_mode = GT_JUMBO_MODE_2048;
	} else if (mtu == 2048) {
		jumbo_mode = GT_JUMBO_MODE_2048;
	} else if (mtu != 10240) {
		pr_err("MTU(%d) will be adjusted to jumbo mode(10240)\n", mtu);
		jumbo_mode = GT_JUMBO_MODE_10240;
	} else {
		jumbo_mode = GT_JUMBO_MODE_10240;
	}

	for (idx = 0; idx < qd_dev->numOfPorts; idx++) {
		/* Set switch MTU */
		rc = gsysSetJumboMode(qd_dev, idx, jumbo_mode);
		SW_IF_ERROR_STR(rc, "failed to call gsysSetJumboMode()\n");
	}
	return MV_OK;
}

/*******************************************************************************
* mv_switch_mtu_get
*
* DESCRIPTION:
*	Get switch MTU size.
*
* INPUTS:
*	None.
*
* OUTPUTS:
*	mtu - MTU size.
*
* RETURNS:
*	On success return MV_OK.
*	On error different types are returned according to the case.
*******************************************************************************/
int mv_switch_mtu_get(unsigned int *mtu)
{
	GT_JUMBO_MODE jumbo_mode;
	GT_STATUS rc = GT_OK;

	MV_IF_NULL_RET_STR(qd_dev, MV_FAIL, "switch dev qd_dev has not been init!\n");

	/* Get switch MTU */
	rc = gsysGetJumboMode(qd_dev, MV_SWITCH_CPU_PORT_NUM, &jumbo_mode);
	SW_IF_ERROR_STR(rc, "failed to call gsysGetJumboMode()\n");

	/* Convert jumbo frames mode to MTU size */
	if (jumbo_mode == GT_JUMBO_MODE_1522)
		*mtu = 1522;
	else if (jumbo_mode == GT_JUMBO_MODE_2048)
		*mtu = 2048;
	else
		*mtu = 10240;

	return MV_OK;
}

/*******************************************************************************
* mv_switch_link_state_get
*
* DESCRIPTION:
*	The API return realtime port link state of switch logical port.
* INPUTS:
*	lport  - logical switch PHY port ID.
*
* OUTPUTS:
*	state  - realtime port link state.
*			GT_TRUE: link up
*			GT_FALSE: link down down
*
* RETURNS:
*	On success return MV_OK.
*	On error different types are returned according to the case.
*******************************************************************************/
int mv_switch_link_state_get(unsigned int lport, GT_BOOL *state)
{
	GT_STATUS rc = GT_OK;

	MV_IF_NULL_RET_STR(qd_dev, MV_FAIL, "switch dev qd_dev has not been init!\n");

	rc = gprtGetLinkState(qd_dev, lport, state);
	SW_IF_ERROR_STR(rc, "failed to call gprtGetLinkState()\n");

	return MV_OK;
}

/*******************************************************************************
* mv_switch_duplex_state_get
*
* DESCRIPTION:
*	The API return realtime port duplex status of given switch logical port.
* INPUTS:
*	lport - logical switch PHY port ID.
*
* OUTPUTS:
*	state - duplex state.
*		GT_FALSE:half deplex mode
*		GT_TRUE:full deplex mode					.
*
* RETURNS:
*	On success return MV_OK.
*	On error different types are returned according to the case.
*******************************************************************************/
int mv_switch_duplex_state_get(unsigned int lport, GT_BOOL *state)
{
	GT_STATUS rc = GT_OK;

	MV_IF_NULL_RET_STR(qd_dev, MV_FAIL, "switch dev qd_dev has not been init!\n");

	rc = gprtGetDuplex(qd_dev, lport, state);
	SW_IF_ERROR_STR(rc, "failed to call gprtGetDuplex()\n");

	return MV_OK;
}

/*******************************************************************************
* mv_switch_speed_state_get
*
* DESCRIPTION:
*	The API return realtime port speed mode of given switch logical port.
* INPUTS:
*	lport - logical switch PHY port ID.
*
* OUTPUTS:
*	state - speed mode state
*		0:10M
*		1:100M
*		2:1000M
*
* RETURNS:
*	On success return MV_OK.
*	On error different types are returned according to the case.
*******************************************************************************/
int mv_switch_speed_state_get(unsigned int lport, GT_PORT_SPEED_MODE *speed)
{
	GT_STATUS rc = GT_OK;

	MV_IF_NULL_RET_STR(qd_dev, MV_FAIL, "switch dev qd_dev has not been init!\n");

	rc = gprtGetSpeedMode(qd_dev, lport, speed);
	SW_IF_ERROR_STR(rc, "failed to call gprtGetSpeedMode()\n");

	return MV_OK;
}

/*******************************************************************************
* mv_switch_port_secure_mode_set
*
* DESCRIPTION:
*	Change a port mode in the SW data base and remove it from all VLANs
*
* INPUTS:
*	lport - logical switch port ID.
*
* OUTPUTS:
*       None.
*
* RETURNS:
*	On success return MV_OK.
*	On error different types are returned according to the case.
*******************************************************************************/
int mv_switch_port_secure_mode_set(unsigned int lport)
{
	unsigned int port_bm;
	unsigned short vlan_idx;
	GT_STATUS rc = GT_OK;

	sw_port_tbl[lport].port_mode = GT_SECURE;

	port_bm = 1 << lport;

	for (vlan_idx = 0; vlan_idx < MV_SWITCH_MAX_VLAN_NUM; vlan_idx++) {
		if ((sw_vlan_tbl[vlan_idx].port_bm & port_bm) &&
		    (sw_port_tbl[lport].vlan_blong[vlan_idx] == MV_SWITCH_PORT_NOT_BELONG)) {
			rc = mv_switch_port_vid_del(lport, vlan_idx);
			SW_IF_ERROR_STR(rc, "failed to call mv_switch_port_vid_del()\n");

			sw_vlan_tbl[vlan_idx].egr_mode[lport] = NOT_A_MEMBER;
		}
	}

	return MV_OK;
}

/*******************************************************************************
* mv_switch_port_fallback_mode_set
*
* DESCRIPTION:
*	Change a port mode in the SW data base and add it to all VLANs
*
* INPUTS:
*	lport - logical switch port ID.
*
* OUTPUTS:
*       None.
*
* RETURNS:
*	On success return MV_OK.
*	On error different types are returned according to the case.
*******************************************************************************/
int mv_switch_port_fallback_mode_set(unsigned int lport)
{
	unsigned int port_bm;
	unsigned short vlan_idx;
	GT_STATUS rc = GT_OK;

	sw_port_tbl[lport].port_mode = GT_FALLBACK;

	port_bm = 1 << lport;

	for (vlan_idx = 0; vlan_idx < MV_SWITCH_MAX_VLAN_NUM; vlan_idx++) {
		/* If a VLAN has been defined (there is a member in the VLAN) and
		   the specified port is not a member */
		if (sw_vlan_tbl[vlan_idx].port_bm &&
		     !(sw_vlan_tbl[vlan_idx].port_bm & port_bm)) {
			/* Update VTU table */
			rc = mv_switch_port_vid_add(lport, vlan_idx, MEMBER_EGRESS_UNMODIFIED, false);
			SW_IF_ERROR_STR(rc, "failed to call mv_switch_port_vid_add()\n");

			sw_vlan_tbl[vlan_idx].port_bm |= port_bm;
			sw_vlan_tbl[vlan_idx].egr_mode[lport] = MEMBER_EGRESS_UNMODIFIED;
		}
	}

	return MV_OK;
}

/*******************************************************************************
* mv_switch_port_vlan_filter_set
*
* DESCRIPTION:
*	The API sets the filtering mode of a certain lport.
*	If the lport is in filtering mode, only the VIDs added by the
*	tpm_sw_port_add_vid API will be allowed to ingress and egress the lport.
*
* INPUTS:
*	lport  - logical switch port ID.
*       filter - set to 1 means the lport will drop all packets which are NOT in
*		 the allowed VID list (built using API tpm_sw_port_add_vid).
*		 set to 0 - means that the list of VIDs allowed
*		 per lport has no significance (the list is not deleted).
*
* OUTPUTS:
*       None.
*
* RETURNS:
*	On success return MV_OK.
*	On error different types are returned according to the case.
*******************************************************************************/
int mv_switch_port_vlan_filter_set(unsigned int lport, unsigned char filter)
{
	GT_DOT1Q_MODE mode;
	GT_STATUS rc = GT_OK;

	MV_IF_NULL_RET_STR(qd_dev, MV_FAIL, "switch dev qd_dev has not been init!\n");

	/* Move port to secure mode and removed from all VLANs */
	if (filter) {
		/* The port is already in the secure mode - do noting */
		if (sw_port_tbl[lport].port_mode == GT_SECURE)
			return MV_OK;

		rc = mv_switch_port_secure_mode_set(lport);
		SW_IF_ERROR_STR(rc, "failed to call mv_switch_port_secure_mode_set()\n");
		mode = GT_SECURE;
	} else {
		/* Port should be moved to the fallback mode and added to all VLANs */
		if (sw_port_tbl[lport].port_mode == GT_FALLBACK)
			return MV_OK;

		rc = mv_switch_port_fallback_mode_set(lport);
		SW_IF_ERROR_STR(rc, "failed to call mv_switch_port_fallback_mode_set()\n");
		mode = GT_FALLBACK;
	}

	rc = gvlnSetPortVlanDot1qMode(qd_dev, lport, mode);
	SW_IF_ERROR_STR(rc, "failed to call gvlnSetPortVlanDot1qMode()\n");

	return MV_OK;
}

/*******************************************************************************
* mv_switch_port_vlan_filter_get
*
* DESCRIPTION:
*	The API gets the filtering mode of a certain lport.
*
* INPUTS:
*	lport  - logical switch port ID.
*
* OUTPUTS:
*       filter - set to 1 means the lport will drop all packets which are NOT in
*		 the allowed VID list (built using API tpm_sw_port_add_vid).
*		 set to 0 - means that the list of VIDs allowed
*		 per lport has no significance (the list is not deleted).
*
* RETURNS:
*	On success return MV_OK.
*	On error different types are returned according to the case.
*******************************************************************************/
int mv_switch_port_vlan_filter_get(unsigned int lport, unsigned char *filter)
{
	GT_DOT1Q_MODE mode;
	GT_STATUS rc = GT_OK;

	MV_IF_NULL_RET_STR(qd_dev, MV_FAIL, "switch dev qd_dev has not been init!\n");

	rc = gvlnGetPortVlanDot1qMode(qd_dev, lport, &mode);
	SW_IF_ERROR_STR(rc, "failed to call gvlnGetPortVlanDot1qMode()\n");

	if (GT_SECURE == mode)
		*filter = 1;
	else
		*filter = 0;
	return MV_OK;
}

/*******************************************************************************
* mv_switch_port_vlan_mode_set
*
* DESCRIPTION:
*	The API sets the VLAN 802.1q mode of a certain lport.
*
* INPUTS:
*	lport  - logical switch port ID.
*       mode   - VLAN 802.1q mode.
*
* OUTPUTS:
*       None.
*
* RETURNS:
*	On success return MV_OK.
*	On error different types are returned according to the case.
*******************************************************************************/
int mv_switch_port_vlan_mode_set(unsigned int lport, GT_DOT1Q_MODE mode)
{
	GT_STATUS rc = GT_OK;

	MV_IF_NULL_RET_STR(qd_dev, MV_FAIL, "switch dev qd_dev has not been init!\n");

	rc = gvlnSetPortVlanDot1qMode(qd_dev, lport, mode);
	SW_IF_ERROR_STR(rc, "failed to call gvlnSetPortVlanDot1qMode()\n");

	return MV_OK;
}

/*******************************************************************************
* mv_switch_port_vlan_mode_get
*
* DESCRIPTION:
*	The API gets the VLAN 802.1q mode of a certain lport.
*
* INPUTS:
*	lport  - logical switch port ID.
*
* OUTPUTS:
*       mode   - VLAN 802.1q mode.
*
* RETURNS:
*	On success return MV_OK.
*	On error different types are returned according to the case.
*******************************************************************************/
int mv_switch_port_vlan_mode_get(unsigned int lport, GT_DOT1Q_MODE *mode)
{
	GT_STATUS rc = GT_OK;

	MV_IF_NULL_RET_STR(qd_dev, MV_FAIL, "switch dev qd_dev has not been init!\n");

	rc = gvlnGetPortVlanDot1qMode(qd_dev, lport, mode);
	SW_IF_ERROR_STR(rc, "failed to call gvlnGetPortVlanDot1qMode()\n");

	return MV_OK;
}

/*******************************************************************************
* mv_switch_port_mac_filter_mode_set
*
* DESCRIPTION: The routine sets MAC filter mode
*
* INPUTS:
*	lport   - switch port
*	mode   - MAC filter mode
*
* OUTPUTS:
*	None
*
* RETURNS:
*	On success, the function returns MV_OK. On error different types are returned
*	according to the case - see tpm_error_code_t.
*
* COMMENTS:
*	None
*******************************************************************************/
int mv_switch_port_mac_filter_mode_set(unsigned int	lport,
				    GT_SA_FILTERING	mode)

{
	int rc = MV_OK;

	MV_IF_NULL_RET_STR(qd_dev, MV_FAIL, "switch dev qd_dev has not been init!\n");

	/* set filter mode */
	rc = gprtSetSAFiltering(qd_dev, lport, mode);
	SW_IF_ERROR_STR(rc, "fail to set filter port(%d) mode(%d)\n", lport, mode);

	return rc;
}

/*******************************************************************************
* mv_switch_port_mac_filter_mode_get
*
* DESCRIPTION: The routine adds MAC address filter entry
*
* INPUTS:
*	lport   - switch lport
*
* OUTPUTS:
*	mode   - MAC filter mode
*
* RETURNS:
*	On success, the function returns MV_OK. On error different types are returned
*	according to the case - see tpm_error_code_t.
*
* COMMENTS:
*	None
*******************************************************************************/
int mv_switch_port_mac_filter_mode_get(unsigned int	lport,
				    GT_SA_FILTERING	*mode)
{
	int rc = MV_OK;

	MV_IF_NULL_RET_STR(qd_dev, MV_FAIL, "switch dev qd_dev has not been init!\n");

	/* set filter mode */
	rc = gprtGetSAFiltering(qd_dev, lport, mode);
	SW_IF_ERROR_STR(rc, "fail to get filtering mode of port(%d)\n", lport);

	return rc;
}

/*******************************************************************************
* mv_switch_port_mac_filter_entry_add
*
* DESCRIPTION: The routine adds MAC address filter entry
*
* INPUTS:
*	lport    - switch lport
*	mac     - MAC address
*	vlan    - VLAN ID
*	mode    - MAC filter mode
*
* OUTPUTS:
*	None
*
* RETURNS:
*	On success, the function returns MV_OK. On error different types are returned
*	according to the case - see tpm_error_code_t.
*
* COMMENTS:
*	None
*******************************************************************************/
int mv_switch_port_mac_filter_entry_add(unsigned int	lport,
				     unsigned char		*mac,
				     unsigned short		vlan,
				     GT_SA_FILTERING		mode)
{
	int rc = MV_OK;
	unsigned int port_bm;

	if (GT_SA_DROP_ON_LOCK == mode)
		port_bm = (1 << lport);
	else if (GT_SA_DROP_ON_UNLOCK == mode || GT_SA_DROP_TO_CPU == mode)
		port_bm = 0;
	else
		return MV_OK;

	rc = mv_switch_mac_addr_add(port_bm, mac, MV_SWITCH_STATIC_MAC_ADDR);
	SW_IF_ERROR_STR(rc, "fail to add filtering addr of port(%d)\n", lport);

	return rc;
}

/*******************************************************************************
* mv_switch_port_mac_filter_entry_del
*
* DESCRIPTION: The routine deletes MAC address filter entry
*
* INPUTS:
*	lport    - switch port
*	mac     - MAC address
*	vlan    - VLAN ID
*	mode    - MAC filter mode
*
* OUTPUTS:
*	None
*
* RETURNS:
*	On success, the function returns MV_OK. On error different types are returned
*	according to the case - see tpm_error_code_t.
*
* COMMENTS:
*	None
*******************************************************************************/
int mv_switch_port_mac_filter_entry_del(unsigned int	lport,
				     unsigned char		*mac,
				     unsigned short		vlan,
				     GT_SA_FILTERING		mode)
{
	int rc = MV_OK;

	rc = mv_switch_mac_addr_del(lport, mac);
	SW_IF_ERROR_STR(rc, "fail to del filtering addr of port(%d)\n", lport);

	return rc;
}

/*******************************************************************************
* mv_switch_port_vlan_set
*
* DESCRIPTION:
*	This routine sets the port VLAN group port membership list.
*
* INPUTS:
*	lport    - logical switch port ID.
*	mem_port - array of logical ports in the same vlan.
*	mem_num  - number of members in memPorts array
*
* OUTPUTS:
*	None.
*
* RETURNS:
*	On success return MV_OK.
*	On error different types are returned according to the case.
*******************************************************************************/
int mv_switch_port_vlan_set(unsigned int lport, GT_LPORT mem_port[], unsigned int mem_num)
{
	GT_STATUS rc = GT_OK;

	MV_IF_NULL_RET_STR(qd_dev, MV_FAIL, "switch dev qd_dev has not been init!\n");

	rc = gvlnSetPortVlanPorts(qd_dev, (GT_LPORT)lport, mem_port, (unsigned char)mem_num);
	SW_IF_ERROR_STR(rc, "failed to call gvlnSetPortVlanPorts()\n");

	return MV_OK;
}

/*******************************************************************************
* mv_switch_port_vlan_get
*
* DESCRIPTION:
*	This routine gets the port VLAN group port membership list.
*
* INPUTS:
*	lport    - logical switch port ID.
*
* OUTPUTS:
*	mem_port - array of logical ports in the same vlan.
*	mem_num  - number of members in memPorts array
*
* RETURNS:
*	On success return MV_OK.
*	On error different types are returned according to the case.
*******************************************************************************/
int mv_switch_port_vlan_get(unsigned int lport, GT_LPORT mem_port[], unsigned int *mem_num)
{
	GT_U8 num;
	GT_STATUS rc = GT_OK;

	MV_IF_NULL_RET_STR(qd_dev, MV_FAIL, "switch dev qd_dev has not been init!\n");

	rc = gvlnGetPortVlanPorts(qd_dev, lport, mem_port, &num);
	SW_IF_ERROR_STR(rc, "failed to call gvlnGetPortVlanPorts()\n");
	*mem_num = num;

	return MV_OK;
}

/*******************************************************************************
* mv_switch_mh_mode_set
*
* DESCRIPTION:
*	This routine enables/disables ingress and egress header mode of switch port.
*
* INPUTS:
*	lport   - logical switch port ID.
*	enable  - enable/disable marvell header.
*
* OUTPUTS:
*	None.
*
* RETURNS:
*	On success return MV_OK.
*	On error different types are returned according to the case.
*******************************************************************************/
int mv_switch_mh_mode_set(unsigned char lport, GT_BOOL enable)
{
	GT_STATUS rc = GT_OK;

	MV_IF_NULL_RET_STR(qd_dev, MV_FAIL, "switch dev qd_dev has not been init!\n");

	rc = gprtSetHeaderMode(qd_dev, lport, enable);
	SW_IF_ERROR_STR(rc, "failed to call gprtSetHeaderMode()\n");

	return MV_OK;
}

/*******************************************************************************
* mv_switch_mh_mode_get
*
* DESCRIPTION:
*	This routine gets ingress and egress header mode of switch port.
*
* INPUTS:
*	lport   - logical switch port ID.
*
* OUTPUTS:
*	enable  - enable/disable marvell header.
*
* RETURNS:
*	On success return MV_OK.
*	On error different types are returned according to the case.
*******************************************************************************/
int mv_switch_mh_mode_get(unsigned char lport, GT_BOOL *enable)
{
	GT_STATUS rc = GT_OK;

	MV_IF_NULL_RET_STR(qd_dev, MV_FAIL, "switch dev qd_dev has not been init!\n");

	rc = gprtGetHeaderMode(qd_dev, lport, enable);
	SW_IF_ERROR_STR(rc, "failed to call gprtGetHeaderMode()\n");

	return MV_OK;
}

/*******************************************************************************
* mv_switch_frame_mode_set
*
* DESCRIPTION:
*	This routine sets the frame mode.
*
* INPUTS:
*	lport - logical switch port ID.
*	mode  - frame mode.
*
* OUTPUTS:
*	None.
*
* RETURNS:
*	On success return MV_OK.
*	On error different types are returned according to the case.
*******************************************************************************/
int mv_switch_frame_mode_set(unsigned char lport, GT_FRAME_MODE mode)
{
	GT_STATUS rc = GT_OK;

	MV_IF_NULL_RET_STR(qd_dev, MV_FAIL, "switch dev qd_dev has not been init!\n");

	rc = gprtSetFrameMode(qd_dev, lport, mode);
	SW_IF_ERROR_STR(rc, "failed to call gprtSetFrameMode()\n");

	return MV_OK;
}

/*******************************************************************************
* mv_switch_frame_mode_get
*
* DESCRIPTION:
*	This routine gets the frame mode.
*
* INPUTS:
*	lport  - logical switch port ID.
*
* OUTPUTS:
*	mode   - frame mode.
*
* RETURNS:
*	On success return MV_OK.
*	On error different types are returned according to the case.
*******************************************************************************/
int mv_switch_frame_mode_get(unsigned char lport, GT_FRAME_MODE *mode)
{
	GT_STATUS rc = GT_OK;

	MV_IF_NULL_RET_STR(qd_dev, MV_FAIL, "switch dev qd_dev has not been init!\n");

	rc = gprtGetFrameMode(qd_dev, lport, mode);
	SW_IF_ERROR_STR(rc, "failed to call gprtGetFrameMode()\n");

	return MV_OK;
}

/*******************************************************************************
* mv_switch_etype_set
*
* DESCRIPTION:
*	This routine sets ethernet type.
*
* INPUTS:
*	lport - logical switch port ID.
*	etype - Ethernet type.
*
* OUTPUTS:
*	None.
*
* RETURNS:
*	On success return MV_OK.
*	On error different types are returned according to the case.
*******************************************************************************/
int mv_switch_etype_set(unsigned char lport, unsigned short etype)
{
	GT_STATUS rc = GT_OK;

	MV_IF_NULL_RET_STR(qd_dev, MV_FAIL, "switch dev qd_dev has not been init!\n");

	rc = gprtSetPortEType(qd_dev, lport, etype);
	SW_IF_ERROR_STR(rc, "failed to call gprtSetPortEType()\n");

	return MV_OK;
}

/*******************************************************************************
* mv_switch_etype_get
*
* DESCRIPTION:
*	This routine gets the frame mode.
*
* INPUTS:
*	lport  - logical switch port ID.
*
* OUTPUTS:
*	etype - Ethernet type.
*
* RETURNS:
*	On success return MV_OK.
*	On error different types are returned according to the case.
*******************************************************************************/
int mv_switch_etype_get(unsigned char lport, unsigned short *etype)
{
	GT_ETYPE l_etype;
	GT_STATUS rc = GT_OK;

	MV_IF_NULL_RET_STR(qd_dev, MV_FAIL, "switch dev qd_dev has not been init!\n");

	rc = gprtGetPortEType(qd_dev, lport, &l_etype);
	SW_IF_ERROR_STR(rc, "failed to call gprtGetPortEType()\n");
	*etype = (unsigned short)l_etype;

	return MV_OK;
}

/*******************************************************************************
* mv_switch_port_preamble_set
*
* DESCRIPTION:
*	This routine sets preamble of a switch port.
*
* INPUTS:
*	lport   - logical switch port ID.
*	preamble - preamble length.
*
* OUTPUTS:
*	None.
*
* RETURNS:
*	On success return MV_OK.
*	On error different types are returned according to the case.
*******************************************************************************/
int mv_switch_port_preamble_set(unsigned char lport, unsigned short preamble)
{
	unsigned short data;
	GT_STATUS rc = GT_OK;

	MV_IF_NULL_RET_STR(qd_dev, MV_FAIL, "switch dev qd_dev has not been init!\n");

	rc = mv_switch_mii_write(qd_dev, 3, 26, preamble);
	SW_IF_ERROR_STR(rc, "failed to call mv_switch_set_port_reg()\n");

	mvOsDelay(10);

	data = 0xb002 | (lport << 8);
	rc = mv_switch_mii_write(qd_dev, 2, 26, data);
	SW_IF_ERROR_STR(rc, "failed to call mv_switch_set_port_reg()\n");

	return MV_OK;
}

/*******************************************************************************
* mv_switch_port_preamble_get
*
* DESCRIPTION:
*	This routine gets preamble of a switch port.
*
* INPUTS:
*	lport   - logical switch port ID.
*
* OUTPUTS:
*	preamble - preamble length.
*
* RETURNS:
*	On success return MV_OK.
*	On error different types are returned according to the case.
*******************************************************************************/
int mv_switch_port_preamble_get(unsigned char lport, unsigned short *preamble)
{
	unsigned int data;
	GT_STATUS rc = GT_OK;

	MV_IF_NULL_RET_STR(qd_dev, MV_FAIL, "switch dev qd_dev has not been init!\n");

	data = 0xc002 | (lport << 8);
	rc = mv_switch_mii_write(qd_dev, 2, 26, data);
	SW_IF_ERROR_STR(rc, "failed to call mv_switch_mii_read()\n");

	mvOsDelay(10);

	rc = mv_switch_mii_read(qd_dev, 3, 26, &data);
	SW_IF_ERROR_STR(rc, "failed to call mv_switch_mii_read()\n");

	*preamble = data;

	return MV_OK;
}

/*******************************************************************************
* mv_switch_atu_next_entry_get
*
* DESCRIPTION:
*	This function get next FDB entry.
*
* INPUTS:
*	atu_entry - ATU entry
*
* OUTPUTS:
*	atu_entry - ATU entry
*
* RETURNS:
*	On success return MV_OK.
*	On error different types are returned according to the case.
*******************************************************************************/
int mv_switch_atu_next_entry_get(GT_ATU_ENTRY *atu_entry)
{
	GT_STATUS rc = GT_OK;

	MV_IF_NULL_RET_STR(qd_dev, MV_FAIL, "switch dev qd_dev has not been init!\n");

	rc = gfdbGetAtuEntryNext(qd_dev, atu_entry);

	if (rc == GT_OK)
		return MV_OK;
	else
		return MV_FAIL;
}

/*******************************************************************************
* mv_switch_vtu_flush
*
* DESCRIPTION:
*	Flush VTU on the Switch
*
* INPUTS:
*	None.
*
* OUTPUTS:
*	None.
*
* RETURNS:
*	On success return MV_OK.
*	On error different types are returned according to the case.
*******************************************************************************/
int mv_switch_vtu_flush(void)
{
	unsigned int lport;
	GT_STATUS rc = GT_OK;

	MV_IF_NULL_RET_STR(qd_dev, MV_FAIL, "switch dev qd_dev has not been init!\n");

	rc = gvtuFlush(qd_dev);
	SW_IF_ERROR_STR(rc, "failed to call gvtuFlush()\n");

	memset(sw_vlan_tbl, 0, sizeof(sw_vlan_tbl));

	for (lport = 0; lport < qd_dev->numOfPorts; lport++)
		memset(&(sw_port_tbl[lport].vlan_blong), 0, sizeof(sw_port_tbl[lport].vlan_blong));

	return MV_OK;
}

/*******************************************************************************
* mv_switch_atu_flush
*
* DESCRIPTION:
*	Flush ATU on the Switch
*
* INPUTS:
*	flush_cmd - flush command
*	db_num    - ATU DB Num, only 0 should be used, since there is only one ATU DB right now.
*
* OUTPUTS:
*	None.
*
* RETURNS:
*	On success return MV_OK.
*	On error different types are returned according to the case.
*******************************************************************************/
int mv_switch_atu_flush(GT_FLUSH_CMD flush_cmd, unsigned short db_num)
{
	GT_STATUS rc = GT_OK;

	MV_IF_NULL_RET_STR(qd_dev, MV_FAIL, "switch dev qd_dev has not been init!\n");

	rc = gfdbFlushInDB(qd_dev, flush_cmd, db_num);
	SW_IF_ERROR_STR(rc, "failed to call gfdbFlushInDB()\n");
	return MV_OK;
}

/*******************************************************************************
* mv_switch_port_force_link_set
*
* DESCRIPTION:
*       This routine will force given switch port to be linked.
*
* INPUTS:
*	lport  - logical switch PHY port ID.
*	enable - enable/disable port force link.
*	value  - force link up or down
*
* OUTPUTS:
*	None.
*
* RETURNS:
*	On success return MV_OK.
*	On error different types are returned according to the case.
*******************************************************************************/
int mv_switch_port_force_link_set(unsigned int lport, GT_BOOL enable, GT_BOOL value)
{
	GT_STATUS rc = GT_OK;

	MV_IF_NULL_RET_STR(qd_dev, MV_FAIL, "switch dev qd_dev has not been init!\n");

	rc = gpcsSetForcedLink(qd_dev, lport, enable);
	SW_IF_ERROR_STR(rc, "failed to call gpcsSetForcedLink()\n");

	rc = gpcsSetLinkValue(qd_dev, lport, value);
	SW_IF_ERROR_STR(rc, "failed to call gpcsSetLinkValue()\n");

	return MV_OK;
}

/*******************************************************************************
* mv_switch_port_force_link_get
*
* DESCRIPTION:
*       This routine gets the force link state of given switch port.
*
* INPUTS:
*	lport  - logical switch PHY port ID.
*
* OUTPUTS:
*	enable - enable/disable port force link.
*	value  - force link up or down
*
* RETURNS:
*	On success return MV_OK.
*	On error different types are returned according to the case.
*******************************************************************************/
int mv_switch_port_force_link_get(unsigned int lport, GT_BOOL *enable, GT_BOOL *value)
{
	GT_STATUS rc = GT_OK;

	MV_IF_NULL_RET_STR(qd_dev, MV_FAIL, "switch dev qd_dev has not been init!\n");

	rc = gpcsGetForcedLink(qd_dev, lport, enable);
	SW_IF_ERROR_STR(rc, "failed to call gpcsGetForcedLink()\n");

	rc = gpcsGetLinkValue(qd_dev, lport, value);
	SW_IF_ERROR_STR(rc, "failed to call gpcsGetLinkValue()\n");

	return MV_OK;
}

/*******************************************************************************
* mv_switch_port_force_fc_set
*
* DESCRIPTION:
*	This routine will set forced flow control state and value.
*
* INPUTS:
*	lport  - logical switch PHY port ID.
*	enable - enable/disable forced flow control.
*	value  - force flow control value, enable flow control or disable it.
*
* OUTPUTS:
*	None.
*
* RETURNS:
*	On success return MV_OK.
*	On error different types are returned according to the case.
*******************************************************************************/
int mv_switch_port_force_fc_set(unsigned int lport, GT_BOOL enable, GT_BOOL value)
{
	GT_STATUS rc = GT_OK;

	MV_IF_NULL_RET_STR(qd_dev, MV_FAIL, "switch dev qd_dev has not been init!\n");

	rc = gpcsSetForcedFC(qd_dev, lport, enable);
	SW_IF_ERROR_STR(rc, "failed to call gpcsSetForcedFC()\n");

	rc = gpcsSetFCValue(qd_dev, lport, value);
	SW_IF_ERROR_STR(rc, "failed to call gpcsSetFCValue()\n");

	return MV_OK;
}

/*******************************************************************************
* mv_switch_port_force_fc_get
*
* DESCRIPTION:
*	This routine will get forced flow control state and value.
*
* INPUTS:
*	lport  - logical switch PHY port ID.
*
* OUTPUTS:
*	enable - enable/disable forced flow control.
*	value  - force flow control value, enable flow control or disable it.
*
* RETURNS:
*	On success return MV_OK.
*	On error different types are returned according to the case.
*******************************************************************************/
int mv_switch_port_force_fc_get(unsigned int lport, GT_BOOL *enable, GT_BOOL *value)
{
	GT_STATUS rc = GT_OK;

	MV_IF_NULL_RET_STR(qd_dev, MV_FAIL, "switch dev qd_dev has not been init!\n");

	rc = gpcsGetForcedFC(qd_dev, lport, enable);
	SW_IF_ERROR_STR(rc, "failed to call gpcsGetForcedFC()\n");

	rc = gpcsGetFCValue(qd_dev, lport, value);
	SW_IF_ERROR_STR(rc, "failed to call gpcsGetFCValue()\n");

	return MV_OK;
}

/*******************************************************************************
* mv_switch_port_force_speed_set
*
* DESCRIPTION:
*       This routine will force given switch port to work at specific speed.
*
* INPUTS:
*	lport  - logical switch PHY port ID.
*	enable - enable/disable port force speed.
*	mode   - speed mode.
*
* OUTPUTS:
*	None.
*
* RETURNS:
*	On success return MV_OK.
*	On error different types are returned according to the case.
*******************************************************************************/
int mv_switch_port_force_speed_set(unsigned int lport, GT_BOOL enable, unsigned int mode)
{
	GT_PORT_FORCED_SPEED_MODE l_mode;
	GT_STATUS rc = GT_OK;

	MV_IF_NULL_RET_STR(qd_dev, MV_FAIL, "switch dev qd_dev has not been init!\n");

	if (GT_FALSE == enable)
		l_mode = PORT_DO_NOT_FORCE_SPEED;
	else
		l_mode = (GT_PORT_FORCED_SPEED_MODE)mode;

	rc = gpcsSetForceSpeed(qd_dev, lport, l_mode);
	SW_IF_ERROR_STR(rc, "failed to call gpcsSetForceSpeed()\n");

	return MV_OK;
}

/*******************************************************************************
* mv_switch_port_force_speed_get
*
* DESCRIPTION:
*       This routine gets the force speed state of given switch port.
*
* INPUTS:
*	lport  - logical switch PHY port ID.
*
* OUTPUTS:
*	enable - enable/disable port force speed.
*	mode   - speed mode.
*
* RETURNS:
*	On success return MV_OK.
*	On error different types are returned according to the case.
*******************************************************************************/
int mv_switch_port_force_speed_get(unsigned int lport, GT_BOOL *enable, unsigned int *mode)
{
	GT_PORT_FORCED_SPEED_MODE l_mode;
	GT_STATUS rc = GT_OK;

	MV_IF_NULL_RET_STR(qd_dev, MV_FAIL, "switch dev qd_dev has not been init!\n");

	rc = gpcsGetForceSpeed(qd_dev, lport, &l_mode);
	SW_IF_ERROR_STR(rc, "failed to call gpcsGetForceSpeed()\n");

	if (PORT_DO_NOT_FORCE_SPEED == l_mode) {
		*enable = GT_FALSE;
		*mode   = PORT_FORCE_SPEED_1000_MBPS; /* do not have mean in case the force is disabled */
	} else {
		*enable = GT_TRUE;
		*mode   = (unsigned int)l_mode;
	}
	return MV_OK;
}

/*******************************************************************************
* mv_switch_port_force_duplex_set
*
* DESCRIPTION:
*       This routine will force given switch port w/ duplex mode.
*
* INPUTS:
*	lport  - logical switch PHY port ID.
*	enable - enable/disable port force duplex.
*	value  - half or full duplex mode
*
* OUTPUTS:
*	None.
*
* RETURNS:
*	On success return MV_OK.
*	On error different types are returned according to the case.
*******************************************************************************/
int mv_switch_port_force_duplex_set(unsigned int lport, GT_BOOL enable, GT_BOOL value)
{
	GT_STATUS rc = GT_OK;

	MV_IF_NULL_RET_STR(qd_dev, MV_FAIL, "switch dev qd_dev has not been init!\n");

	rc = gpcsSetForcedDpx(qd_dev, lport, enable);
	SW_IF_ERROR_STR(rc, "failed to call gpcsSetForcedDpx()\n");

	rc = gpcsSetDpxValue(qd_dev, lport, value);
	SW_IF_ERROR_STR(rc, "failed to call gpcsSetDpxValue()\n");

	return MV_OK;
}

/*******************************************************************************
* mv_switch_port_force_duplex_get
*
* DESCRIPTION:
*       This routine gets the force duplex state of given switch port.
*
* INPUTS:
*	lport  - logical switch PHY port ID.
*
* OUTPUTS:
*	enable - enable/disable port force duplex.
*	value  - half or full duplex mode
*
* RETURNS:
*	On success return MV_OK.
*	On error different types are returned according to the case.
*******************************************************************************/
int mv_switch_port_force_duplex_get(unsigned int lport, GT_BOOL *enable, GT_BOOL *value)
{
	GT_STATUS rc = GT_OK;

	MV_IF_NULL_RET_STR(qd_dev, MV_FAIL, "switch dev qd_dev has not been init!\n");

	rc = gpcsGetForcedDpx(qd_dev, lport, enable);
	SW_IF_ERROR_STR(rc, "failed to call gpcsGetForcedDpx()\n");

	rc = gpcsGetDpxValue(qd_dev, lport, value);
	SW_IF_ERROR_STR(rc, "failed to call gpcsGetDpxValue()\n");

	return MV_OK;
}

/*******************************************************************************
* mv_switch_port_num_get
*
* DESCRIPTION:
*	This routine will get total switch port number.
*
* INPUTS:
*	None.
*
* OUTPUTS:
*	None.
*
* RETURNS:
*	On success return MV_OK.
*	On error different types are returned according to the case.
*******************************************************************************/
unsigned int mv_switch_port_num_get(void)
{
	MV_IF_NULL_RET_STR(qd_dev, 0, "switch dev qd_dev has not been init!\n");

	return qd_dev->numOfPorts;
}

/*******************************************************************************
* mv_switch_qd_dev_get
*
* DESCRIPTION:
*	This routine gets QA dev.
*
* INPUTS:
*	None.
*
* OUTPUTS:
*	None.
*
* RETURNS:
*	QA dev.
*******************************************************************************/
GT_QD_DEV *mv_switch_qd_dev_get(void)
{
	return qd_dev;
}

/*******************************************************************************
* mv_switch_vtu_shadow_dump
*
* DESCRIPTION:
*	This routine dumps the VTU shadow.
*
* INPUTS:
*	None.
*
* OUTPUTS:
*	None.
*
* RETURNS:
*
*******************************************************************************/
int mv_switch_vtu_shadow_dump(void)
{
	unsigned int vid;
	unsigned int num = 0;
	unsigned int port_idx;
	GT_VTU_ENTRY *vtu_entry;
	pr_err("switch VTU shadow\n\n");

	for (vid = 0; vid < MV_SWITCH_MAX_VLAN_NUM; vid++) {
		if (sw_vlan_tbl[vid].port_bm) {
			vtu_entry = &sw_vlan_tbl[vid].vtu_entry;
			pr_err("DBNum:%i, VID:%i port_bm:0x%02x,\n",
				vtu_entry->DBNum, vtu_entry->vid, sw_vlan_tbl[vid].port_bm);
			pr_err("Tag Mode: ");
			for (port_idx = 0; port_idx < MV_SWITCH_MAX_PORT_NUM; port_idx++)
				pr_err("port(%d):%d; ", port_idx, sw_vlan_tbl[vid].egr_mode[port_idx]);
			pr_err("\n");

			pr_err("vidPriOverride(%d), vidPriority(%d), sid(%d), vidPolicy(%d), useVIDFPri(%d), vidFPri(%d), useVIDQPri(%d), vidQPri(%d), vidNRateLimit(%d)\n",
				vtu_entry->vidPriOverride,
				vtu_entry->vidPriority,
				vtu_entry->sid,
				vtu_entry->vidPolicy,
				vtu_entry->vidExInfo.useVIDFPri,
				vtu_entry->vidExInfo.vidFPri,
				vtu_entry->vidExInfo.useVIDQPri,
				vtu_entry->vidExInfo.vidQPri,
				vtu_entry->vidExInfo.vidNRateLimit);
			num++;
		}
	}
	pr_err("\nTag mode 0: egress unmodified, 1:port not in VLAN, 2:egress untagged, 3:egress tagged\n");
	pr_err("Total switch VLAN number:%d\n", num);

	return MV_OK;
}

/*******************************************************************************
* mv_switch_vlan_tunnel_set
*
* DESCRIPTION:
*	This routine set VLAN tunnel mode of switch port.
*
* INPUTS:
*	lport  - switch port
*       mode   - vlan tunnel mode, enable or disable
*
* OUTPUTS:
*	None.
*
* RETURNS:
*
*******************************************************************************/
int mv_switch_vlan_tunnel_set(unsigned int lport, GT_BOOL mode)
{
	GT_STATUS rc = GT_OK;

	/* check qd_dev init or not */
	if (qd_dev == NULL) {
		rc = MV_ERROR;
		SW_IF_ERROR_STR(rc, "qd_dev not initialized, call mv_switch_load() first\n");
	}

	rc = gprtSetVlanTunnel(qd_dev, lport, mode);
	SW_IF_ERROR_STR(rc, "failed to call gpcsGetForcedDpx()\n");

	return MV_OK;
}

/*******************************************************************************
* mv_switch_cpu_port_get
*
* DESCRIPTION:
*	This routine get cpu port of swicth.
*
* INPUTS:
*	None.
*
* OUTPUTS:
*	cpu_port - swicth CPU port configured.
*
* RETURNS:
*
*******************************************************************************/
int mv_switch_cpu_port_get(unsigned int *cpu_port)
{
	GT_STATUS rc = GT_OK;

	if (cpu_port == NULL)
		return MV_BAD_VALUE;

	/* check qd_dev init or not */
	if (qd_dev == NULL) {
		rc = MV_ERROR;
		SW_IF_ERROR_STR(rc, "qd_dev not initialized, call mv_switch_load() first\n");
	}

	*cpu_port = qd_dev->cpuPortNum;

	return MV_OK;
}

static int mv_switch_probe(struct platform_device *pdev)
{
	struct mv_switch_pdata *plat_data = (struct mv_switch_pdata *)pdev->dev.platform_data;
	/* load switch driver, force link on cpu port */
	mv_switch_load(plat_data);

	/* default switch init, disable all ports */
	mv_switch_init(plat_data);

	return MV_OK;
}

static int mv_switch_remove(struct platform_device *pdev)
{
	printk(KERN_INFO "Removing Marvell Switch Driver\n");
	/* unload */

	return MV_OK;
}

static void mv_switch_shutdown(struct platform_device *pdev)
{
	printk(KERN_INFO "Shutting Down Marvell Switch Driver\n");
}
static const struct mv_mux_switch_ops switch_ops =  {

	/* update ops for mux */
	.promisc_set = mv_switch_promisc_set,
	.jumbo_mode_set = mv_switch_jumbo_mode_set,
	.group_disable = mv_switch_group_disable,
	.group_enable = mv_switch_group_enable,
	.link_status_get = mv_switch_link_status_get,
	.all_mcast_del = mv_switch_all_multicasts_del,
	.mac_addr_set = mv_switch_mac_addr_set,
	.group_cookie_set = mv_switch_group_cookie_set,
	.tag_get = mv_switch_tag_get,
	.preset_init = mv_switch_preset_init,
	.interrupt_unmask = mv_switch_interrupt_unmask,
};

static struct platform_driver mv_switch_driver = {
	.probe = mv_switch_probe,
	.remove = mv_switch_remove,
	.shutdown = mv_switch_shutdown,
#ifdef CONFIG_CPU_IDLE
	/* TBD */
#endif /* CONFIG_CPU_IDLE */
	.driver = {
		.name = MV_SWITCH_SOHO_NAME,
	},
};

static int __init mv_switch_init_module(void)
{
	return platform_driver_register(&mv_switch_driver);
}
module_init(mv_switch_init_module);

static void __exit mv_switch_cleanup_module(void)
{
	platform_driver_unregister(&mv_switch_driver);
}
module_exit(mv_switch_cleanup_module);


MODULE_DESCRIPTION("Marvell Internal Switch Driver - www.marvell.com");
MODULE_AUTHOR("Dmitri Epshtein <dima@marvell.com>");
MODULE_LICENSE("GPL");
