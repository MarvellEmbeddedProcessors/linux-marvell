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
#include "mv_switch.h"

#define MV_SWITCH_DEF_INDEX     0
#define MV_ETH_PORT_0           0
#define MV_ETH_PORT_1           1

static u16	db_port_mask[MV_SWITCH_DB_NUM];
static u16	db_link_mask[MV_SWITCH_DB_NUM];
static void	*db_cookies[MV_SWITCH_DB_NUM];

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


int mv_switch_mac_addr_set(int db, unsigned char *mac_addr, unsigned char op)
{
	GT_ATU_ENTRY mac_entry;
	unsigned int ports_mask = db_port_mask[db] | (1 << qd_cpu_port);

	memset(&mac_entry, 0, sizeof(GT_ATU_ENTRY));

	mac_entry.trunkMember = GT_FALSE;
	mac_entry.prio = 0;
	mac_entry.exPrio.useMacFPri = GT_FALSE;
	mac_entry.exPrio.macFPri = 0;
	mac_entry.exPrio.macQPri = 0;
	mac_entry.DBNum = db;
	mac_entry.portVec = ports_mask;
	memcpy(mac_entry.macAddr.arEther, mac_addr, 6);

	if (is_multicast_ether_addr(mac_addr))
		mac_entry.entryState.mcEntryState = GT_MC_STATIC;
	else
		mac_entry.entryState.ucEntryState = GT_UC_NO_PRI_STATIC;

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

	rc = gcosGetDscp2Tc(qd_dev, tos >> 2, &queue);
	if (rc)
		return -1;

	return (int)queue;
}

int mv_switch_tos_set(unsigned char tos, int rxq)
{
	return gcosSetDscp2Tc(qd_dev, tos >> 2, (unsigned char)rxq);
}

int mv_switch_get_free_buffers_num(void)
{
	MV_U16 regVal;

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

	if (gprtGetDuplex(qd_dev, port, &duplex) != GT_OK) {
		printk(KERN_ERR "gprtGetDuplex failed (port %d)\n", port);
		return "ERR";
	} else
		return (duplex) ? "Full" : "Half";
}

static char *mv_str_link_state(int port)
{
	GT_BOOL link;

	if (gprtGetLinkState(qd_dev, port, &link) != GT_OK) {
		printk(KERN_ERR "gprtGetLinkState failed (port %d)\n", port);
		return "ERR";
	} else
		return (link) ? "Up" : "Down";
}

static char *mv_str_pause_state(int port)
{
	GT_BOOL force, pause;

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

	while ((status = gfdbGetAtuEntryNext(qd_dev, &atu_entry)) == GT_OK) {

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

	/* Add port to the VLAN */
	if (mv_switch_port_based_vlan_set(port_map, 0) != 0)
		printk(KERN_ERR "mv_switch_port_based_vlan_set failed\n");

	/* Add port to vtu (used in tx) */
	if (mv_switch_vlan_in_vtu_set(vlan_grp_id, grp_id, (port_map | (1 << qd_cpu_port))))
		printk(KERN_ERR "mv_switch_vlan_in_vtu_set failed\n");

	/* set vtu with each port private vlan id (used in rx) */
	for (p = 0; p < qd_dev->numOfPorts; p++) {
		if (MV_BIT_CHECK(port_map, p) && (p != qd_cpu_port)) {
			if (mv_switch_vlan_in_vtu_set(MV_SWITCH_PORT_VLAN_ID(vlan_grp_id, p),
						      grp_id, port_map) != 0) {
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

	/* Remove port from the VLAN */
	if (mv_switch_port_based_vlan_set(port_map, 0) != 0)
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
