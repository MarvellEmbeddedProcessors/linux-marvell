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

#ifndef _MV_GOP_HW_H_
#define _MV_GOP_HW_H_

/* Sets the field located at the specified in data.     */
#define U32_SET_FIELD(data, mask, val)	((data) = (((data) & ~(mask)) | (val)))

/* port related */
enum mv_reset {RESET, UNRESET};

enum mv_port_speed {
	MV_PORT_SPEED_AN,
	MV_PORT_SPEED_10,
	MV_PORT_SPEED_100,
	MV_PORT_SPEED_1000,
	MV_PORT_SPEED_2500,
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

/* different loopback types can be configure on different levels:
 * MAC, PCS, SERDES
 */
enum mv_lb_type {
	MV_DISABLE_LB,
	MV_RX_2_TX_LB,
	MV_TX_2_RX_LB,         /* on SERDES level - analog loopback */
	MV_TX_2_RX_DIGITAL_LB  /* on SERDES level - digital loopback */
};

enum sd_media_mode {MV_RXAUI, MV_XAUI};

/* Net Complex */
enum mv_netc_topology {
	MV_NETC_GE_MAC0_RXAUI_L23	=	BIT(0),
	MV_NETC_GE_MAC0_RXAUI_L45	=	BIT(1),
	MV_NETC_GE_MAC0_XAUI		=	BIT(2),
	MV_NETC_GE_MAC2_SGMII		=	BIT(3),
	MV_NETC_GE_MAC3_SGMII		=	BIT(4),
	MV_NETC_GE_MAC3_RGMII		=	BIT(5),
};

enum mv_netc_phase {
	MV_NETC_FIRST_PHASE,
	MV_NETC_SECOND_PHASE,
};

enum mv_netc_sgmii_xmi_mode {
	MV_NETC_GBE_SGMII,
	MV_NETC_GBE_XMII,
};

enum mv_netc_mii_mode {
	MV_NETC_GBE_RGMII,
	MV_NETC_GBE_MII,
};

enum mv_netc_lanes {
	MV_NETC_LANE_23,
	MV_NETC_LANE_45,
};

/* pp3_gop_ctrl flags */
#define mv_gop_F_DEBUG_BIT		0
#define mv_gop_F_ATTACH_BIT		1

#define mv_gop_F_DEBUG		(1 << mv_gop_F_DEBUG_BIT)
#define mv_gop_F_ATTACH		(1 << mv_gop_F_ATTACH_BIT)

enum gop_port_flags {NOT_CREATED, CREATED, UNDER_RESET, ENABLED};

struct gop_port_ctrl {
	u32  flags;
};

#define MV_RGMII_TX_FIFO_MIN_TH		(0x41)
#define MV_SGMII_TX_FIFO_MIN_TH		(0x5)
#define MV_SGMII2_5_TX_FIFO_MIN_TH	(0xB)

static inline u32 mv_gop_gen_read(void __iomem *base, u32 offset)
{
	void *reg_ptr = base + offset;
	u32 val;

	val = readl(reg_ptr);
	return val;
}

static inline void mv_gop_gen_write(void __iomem *base, u32 offset, u32 data)
{
	void *reg_ptr = base + offset;

	writel(data, reg_ptr);
}

/* GOP port configuration functions */
int mv_gop110_port_init(struct gop_hw *gop, struct mv_mac_data *mac);
int mv_gop110_port_reset(struct gop_hw *gop, struct mv_mac_data *mac);
void mv_gop110_port_enable(struct gop_hw *gop, struct mv_mac_data *mac);
void mv_gop110_port_disable(struct gop_hw *gop, struct mv_mac_data *mac);
void mv_gop110_port_periodic_xon_set(struct gop_hw *gop,
				     struct mv_mac_data *mac,
				     int enable);
bool mv_gop110_port_is_link_up(struct gop_hw *gop, struct mv_mac_data *mac);
int mv_gop110_port_link_status(struct gop_hw *gop, struct mv_mac_data *mac,
			       struct mv_port_link_status *pstatus);
int mv_gop110_port_regs(struct gop_hw *gop, struct mv_mac_data *mac);
int mv_gop110_port_events_mask(struct gop_hw *gop, struct mv_mac_data *mac);
int mv_gop110_port_events_unmask(struct gop_hw *gop, struct mv_mac_data *mac);
int mv_gop110_port_events_clear(struct gop_hw *gop, struct mv_mac_data *mac);
int mv_gop110_status_show(struct gop_hw *gop, struct mv_mac_data *mac);
int mv_gop110_speed_duplex_get(struct gop_hw *gop, struct mv_mac_data *mac,
			       enum mv_port_speed *speed,
			       enum mv_port_duplex *duplex);
int mv_gop110_speed_duplex_set(struct gop_hw *gop, struct mv_mac_data *mac,
			       enum mv_port_speed speed,
			       enum mv_port_duplex duplex);
int mv_gop110_autoneg_restart(struct gop_hw *gop, struct mv_mac_data *mac);
int mv_gop110_fl_cfg(struct gop_hw *gop, struct mv_mac_data *mac);
int mv_gop110_force_link_mode_set(struct gop_hw *gop, struct mv_mac_data *mac,
				  bool force_link_up,
				  bool force_link_down);
int mv_gop110_force_link_mode_get(struct gop_hw *gop, struct mv_mac_data *mac,
				  bool *force_link_up,
				  bool *force_link_down);
int mv_gop110_loopback_set(struct gop_hw *gop, struct mv_mac_data *mac,
			   bool lb);
void mv_gop_reg_print(char *reg_name, u32 reg);

/* Gig PCS Functions */
int mv_gop110_gpcs_mode_cfg(struct gop_hw *gop, int pcs_num, bool en);
int mv_gop110_gpcs_reset(struct gop_hw *gop, int pcs_num, enum mv_reset act);

/* Serdes Functions */
static inline u32 mv_gop110_serdes_read(struct gop_hw *gop, int lane_num,
					u32 offset)
{
	return(mv_gop_gen_read(gop->gop_110.serdes.base,
		lane_num * gop->gop_110.serdes.obj_size + offset));
}

static inline void mv_gop110_serdes_write(struct gop_hw *gop, int lane_num,
					  u32 offset, u32 data)
{
	mv_gop_gen_write(gop->gop_110.serdes.base,
			 lane_num * gop->gop_110.serdes.obj_size +
			 offset, data);
}

static inline void mv_gop110_serdes_print(struct gop_hw *gop, char *reg_name,
					  int lane_num, u32 reg)
{
	pr_info("  %-32s: 0x%x = 0x%08x\n", reg_name, reg,
		mv_gop110_serdes_read(gop, lane_num, reg));
}

void mv_gop110_serdes_lane_regs_dump(struct gop_hw *gop, int lane);
void mv_gop110_serdes_init(struct gop_hw *gop, int lane,
			   enum sd_media_mode mode);
void mv_gop110_serdes_reset(struct gop_hw *gop, int lane, bool analog_reset,
			    bool core_reset, bool digital_reset);

/* MPCS Functions */

static inline u32 mv_gop110_mpcs_global_read(struct gop_hw *gop, u32 offset)
{
	return mv_gop_gen_read(gop->gop_110.mspg_base, offset);
}

static inline void mv_gop110_mpcs_global_write(struct gop_hw *gop, u32 offset,
					       u32 data)
{
	mv_gop_gen_write(gop->gop_110.mspg_base, offset, data);
}

static inline void mv_gop110_mpcs_global_print(struct gop_hw *gop,
					       char *reg_name, u32 reg)
{
	pr_info("  %-32s: 0x%x = 0x%08x\n", reg_name, reg,
		mv_gop110_mpcs_global_read(gop, reg));
}

/* XPCS Functions */

static inline u32 mv_gop110_xpcs_global_read(struct gop_hw *gop, u32 offset)
{
	return mv_gop_gen_read(gop->gop_110.xpcs_base, offset);
}

static inline void mv_gop110_xpcs_global_write(struct gop_hw *gop, u32 offset,
					       u32 data)
{
	mv_gop_gen_write(gop->gop_110.xpcs_base, offset, data);
}

static inline void mv_gop110_xpcs_global_print(struct gop_hw *gop,
					       char *reg_name, u32 reg)
{
	pr_info("  %-32s: 0x%x = 0x%08x\n", reg_name, reg,
		mv_gop110_xpcs_global_read(gop, reg));
}

static inline u32 mv_gop110_xpcs_lane_read(struct gop_hw *gop, int lane_num,
					   u32 offset)
{
	return mv_gop_gen_read(gop->gop_110.xpcs_base, offset);
}

static inline void mv_gop110_xpcs_lane_write(struct gop_hw *gop, int lane_num,
					     u32 offset, u32 data)
{
	mv_gop_gen_write(gop->gop_110.xpcs_base, offset, data);
}

static inline void mv_gop110_xpcs_lane_print(struct gop_hw *gop,
					     char *reg_name,
					     int lane_num, u32 reg)
{
	pr_info("  %-32s: 0x%x = 0x%08x\n", reg_name, reg,
		mv_gop110_xpcs_lane_read(gop, lane_num, reg));
}

void mv_gop110_xpcs_gl_regs_dump(struct gop_hw *gop);
void mv_gop110_xpcs_lane_regs_dump(struct gop_hw *gop, int lane);
int mv_gop110_xpcs_reset(struct gop_hw *gop, enum mv_reset reset);
int mv_gop110_xpcs_mode(struct gop_hw *gop, int num_of_lanes);
int mv_gop110_mpcs_mode(struct gop_hw *gop);

/* XLG MAC Functions */
static inline u32 mv_gop110_xlg_mac_read(struct gop_hw *gop, int mac_num,
					 u32 offset)
{
	return(mv_gop_gen_read(gop->gop_110.xlg_mac.base,
	       mac_num * gop->gop_110.xlg_mac.obj_size + offset));
}

static inline void mv_gop110_xlg_mac_write(struct gop_hw *gop, int mac_num,
					   u32 offset, u32 data)
{
	mv_gop_gen_write(gop->gop_110.xlg_mac.base,
		mac_num * gop->gop_110.xlg_mac.obj_size + offset, data);
}

static inline void mv_gop110_xlg_mac_print(struct gop_hw *gop, char *reg_name,
					   int mac_num, u32 reg)
{
	pr_info("  %-32s: 0x%x = 0x%08x\n", reg_name, reg,
		mv_gop110_xlg_mac_read(gop, mac_num, reg));
}

/* MIB MAC Functions */
static inline u32 mv_gop110_xmib_mac_read(struct gop_hw *gop, int mac_num,
					  u32 offset)
{
	return(mv_gop_gen_read(gop->gop_110.xmib.base,
	       mac_num * gop->gop_110.xmib.obj_size + offset));
}

static inline void mv_gop110_xmib_mac_write(struct gop_hw *gop, int mac_num,
					    u32 offset, u32 data)
{
	mv_gop_gen_write(gop->gop_110.xmib.base,
			 mac_num * gop->gop_110.xmib.obj_size + offset, data);
}

static inline void mv_gop110_xmib_mac_print(struct gop_hw *gop, char *reg_name,
					    int mac_num, u32 reg)
{
	pr_info("  %-32s: 0x%x = 0x%08x\n", reg_name, reg,
		mv_gop110_xmib_mac_read(gop, mac_num, reg));
}

void mv_gop110_xlg_mac_regs_dump(struct gop_hw *gop, int port);
int mv_gop110_xlg_mac_reset(struct gop_hw *gop, int mac_num,
			    enum mv_reset reset);
int mv_gop110_xlg_mac_mode_cfg(struct gop_hw *gop, int mac_num,
			       int num_of_act_lanes);
int mv_gop110_xlg_mac_loopback_cfg(struct gop_hw *gop, int mac_num,
				   enum mv_lb_type type);

bool mv_gop110_xlg_mac_link_status_get(struct gop_hw *gop, int mac_num);
void mv_gop110_xlg_mac_port_enable(struct gop_hw *gop, int mac_num);
void mv_gop110_xlg_mac_port_disable(struct gop_hw *gop, int mac_num);
void mv_gop110_xlg_mac_port_periodic_xon_set(struct gop_hw *gop,
					     int mac_num,
					     int enable);
int mv_gop110_xlg_mac_link_status(struct gop_hw *gop, int mac_num,
				  struct mv_port_link_status *pstatus);
int mv_gop110_xlg_mac_max_rx_size_set(struct gop_hw *gop, int mac_num,
				      int max_rx_size);
int mv_gop110_xlg_mac_force_link_mode_set(struct gop_hw *gop, int mac_num,
					  bool force_link_up,
					  bool force_link_down);
int mv_gop110_xlg_mac_speed_duplex_set(struct gop_hw *gop, int mac_num,
				       enum mv_port_speed speed,
				       enum mv_port_duplex duplex);
int mv_gop110_xlg_mac_speed_duplex_get(struct gop_hw *gop, int mac_num,
				       enum mv_port_speed *speed,
				       enum mv_port_duplex *duplex);
int mv_gop110_xlg_mac_fc_set(struct gop_hw *gop, int mac_num,
			     enum mv_port_fc fc);
void mv_gop110_xlg_mac_fc_get(struct gop_hw *gop, int mac_num,
			      enum mv_port_fc *fc);
int mv_gop110_xlg_mac_port_link_speed_fc(struct gop_hw *gop, int mac_num,
					 enum mv_port_speed speed,
					 int force_link_up);
void mv_gop110_xlg_port_link_event_mask(struct gop_hw *gop, int mac_num);
void mv_gop110_xlg_port_external_event_unmask(struct gop_hw *gop,
					      int mac_num,
					      int bit_2_open);
void mv_gop110_xlg_port_link_event_clear(struct gop_hw *gop, int mac_num);
void mv_gop110_xlg_2_gig_mac_cfg(struct gop_hw *gop, int mac_num);

/* GMAC Functions  */
static inline u32 mv_gop110_gmac_read(struct gop_hw *gop, int mac_num,
				      u32 offset)
{
	return(mv_gop_gen_read(gop->gop_110.gmac.base,
	       mac_num * gop->gop_110.gmac.obj_size + offset));
}

static inline void mv_gop110_gmac_write(struct gop_hw *gop, int mac_num,
					u32 offset, u32 data)
{
	mv_gop_gen_write(gop->gop_110.gmac.base,
			 mac_num * gop->gop_110.gmac.obj_size + offset, data);
}

static inline void mv_gop110_gmac_print(struct gop_hw *gop, char *reg_name,
					int mac_num, u32 reg)
{
	pr_info("  %-32s: 0x%x = 0x%08x\n", reg_name, reg,
		mv_gop110_gmac_read(gop, mac_num, reg));
}

void mv_gop110_register_bases_dump(struct gop_hw *gop);
void mv_gop110_gmac_regs_dump(struct gop_hw *gop, int port);
int mv_gop110_gmac_reset(struct gop_hw *gop, int mac_num,
			 enum mv_reset reset);
int mv_gop110_gmac_mode_cfg(struct gop_hw *gop, struct mv_mac_data *mac);
int mv_gop110_gmac_loopback_cfg(struct gop_hw *gop, int mac_num,
				enum mv_lb_type type);
bool mv_gop110_gmac_link_status_get(struct gop_hw *gop, int mac_num);
void mv_gop110_gmac_port_enable(struct gop_hw *gop, int mac_num);
void mv_gop110_gmac_port_disable(struct gop_hw *gop, int mac_num);
void mv_gop110_gmac_port_periodic_xon_set(struct gop_hw *gop, int mac_num,
					  int enable);
int mv_gop110_gmac_link_status(struct gop_hw *gop, int mac_num,
			       struct mv_port_link_status *pstatus);
int mv_gop110_gmac_max_rx_size_set(struct gop_hw *gop, int mac_num,
				   int max_rx_size);
int mv_gop110_gmac_force_link_mode_set(struct gop_hw *gop, int mac_num,
				       bool force_link_up,
				       bool force_link_down);
int mv_gop110_gmac_force_link_mode_get(struct gop_hw *gop, int mac_num,
				       bool *force_link_up,
				       bool *force_link_down);
int mv_gop110_gmac_speed_duplex_set(struct gop_hw *gop, int mac_num,
				    enum mv_port_speed speed,
				    enum mv_port_duplex duplex);
int mv_gop110_gmac_speed_duplex_get(struct gop_hw *gop, int mac_num,
				    enum mv_port_speed *speed,
				    enum mv_port_duplex *duplex);
int mv_gop110_gmac_fc_set(struct gop_hw *gop, int mac_num,
			  enum mv_port_fc fc);
void mv_gop110_gmac_fc_get(struct gop_hw *gop, int mac_num,
			   enum mv_port_fc *fc);
int mv_gop110_gmac_port_link_speed_fc(struct gop_hw *gop, int mac_num,
				      enum mv_port_speed speed,
				      int force_link_up);
void mv_gop110_gmac_port_link_event_mask(struct gop_hw *gop, int mac_num);
void mv_gop110_gmac_port_link_event_unmask(struct gop_hw *gop, int mac_num);
void mv_gop110_gmac_port_link_event_clear(struct gop_hw *gop, int mac_num);
int mv_gop110_gmac_port_autoneg_restart(struct gop_hw *gop, int mac_num);

/* SMI Functions  */
static inline u32 mv_gop110_smi_read(struct gop_hw *gop, u32 offset)
{
	return mv_gop_gen_read(gop->gop_110.smi_base, offset);
}

static inline void mv_gop110_smi_write(struct gop_hw *gop, u32 offset,
				       u32 data)
{
	mv_gop_gen_write(gop->gop_110.smi_base, offset, data);
}

static inline void mv_gop110_smi_print(struct gop_hw *gop, char *reg_name,
				       u32 reg)
{
	pr_info("  %-32s: 0x%x = 0x%08x\n", reg_name, reg,
		mv_gop110_smi_read(gop, reg));
}

/* RFU1 Functions  */
static inline u32 mv_gop110_rfu1_read(struct gop_hw *gop, u32 offset)
{
	return mv_gop_gen_read(gop->gop_110.rfu1_base, offset);
}
static inline void mv_gop110_rfu1_write(struct gop_hw *gop, u32 offset,
		u32 data)
{
	mv_gop_gen_write(gop->gop_110.rfu1_base, offset, data);
}
static inline void mv_gop110_rfu1_print(struct gop_hw *gop, char *reg_name,
		u32 reg)
{
	pr_info("  %-32s: 0x%x = 0x%08x\n", reg_name, reg,
		mv_gop110_rfu1_read(gop, reg));
}

/* PTP Functions  */
static inline u32 mv_gop110_ptp_read(struct gop_hw *gop, int mac_num,
				     u32 offset)
{
	return mv_gop_gen_read(gop->gop_110.ptp.base,
			       mac_num * gop->gop_110.ptp.obj_size + offset);
}

static inline void mv_gop110_ptp_write(struct gop_hw *gop, int mac_num,
				       u32 offset, u32 data)
{
	mv_gop_gen_write(gop->gop_110.ptp.base,
			 mac_num * gop->gop_110.ptp.obj_size + offset, data);
}

static inline void mv_gop110_ptp_print(struct gop_hw *gop, char *reg_name,
				       int mac_num, u32 reg)
{
	pr_info("  %-32s: 0x%x = 0x%08x\n", reg_name, reg,
		mv_gop110_ptp_read(gop, mac_num, reg));
}

int mv_gop110_smi_init(struct gop_hw *gop);
int mv_gop110_smi_phy_addr_cfg(struct gop_hw *gop, int port, int addr);

/* MIB Functions  */
u64 mv_gop110_mib_read64(struct gop_hw *gop, int port, unsigned int offset);
void mv_gop110_mib_counters_show(struct gop_hw *gop, int port);

/* PTP Functions */
void mv_gop110_ptp_enable(struct gop_hw *gop, int port, bool state);

/*RFU Functions */
int mv_gop110_netc_init(struct gop_hw *gop,
			u32 net_comp_config, enum mv_netc_phase phase);
void mv_gop110_netc_active_port(struct gop_hw *gop, u32 port, u32 val);

#endif /* _MV_GOP_HW_H_ */
