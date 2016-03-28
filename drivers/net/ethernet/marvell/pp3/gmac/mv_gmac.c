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
#include "net_dev/mv_netdev.h"
#include "mv_gmac_regs.h"
#include "mv_gmac.h"

static struct pp3_gmac_ctrl pp3_gmac[PP3_GMAC_MAX];
static unsigned int gop_base_addr;

/*-------------------------------------------------------------------*/
/*		Gmac regs API					     */
/*-------------------------------------------------------------------*/
void pp3_gmac_base_addr_set(unsigned int addr)
{
	int gmac;

	gop_base_addr = addr;
	for (gmac = 0; gmac < PP3_GMAC_MAX; gmac++)
		pp3_gmac_unit_base(gmac, gop_base_addr + GMAC_REG_BASE(gmac));
}

u32 pp3_gmac_base_addr_get(void)
{
	return gop_base_addr;
}

u32 pp3_gmac_reg_read(int port, u32 reg)
{
	u32 reg_data;

	reg_data = mv_pp3_hw_reg_read(reg + pp3_gmac[port].base);

	if (pp3_gmac[port].flags & MV_PP3_GMAC_F_DEBUG)
		pr_info("read     : 0x%x = 0x%08x\n", reg, reg_data);

	return reg_data;
}

void pp3_gmac_reg_write(int port, u32 reg, u32 data)
{
	mv_pp3_hw_reg_write(reg + pp3_gmac[port].base, data);

	if (pp3_gmac[port].flags & MV_PP3_GMAC_F_DEBUG) {
		u32 reg_data;
		pr_info("write    : 0x%x = 0x%08x\n", reg, data);
		reg_data = mv_pp3_hw_reg_read(reg + pp3_gmac[port].base);
		pr_info("read back: 0x%x = 0x%08x\n", reg, reg_data);
	}
}

static void pp3_gmac_reg_print(int port, char *reg_name, u32 reg)
{
	pr_info("  %-32s: 0x%x = 0x%08x\n", reg_name, reg, pp3_gmac_reg_read(port, reg));
}

void pp3_gmac_unit_base(int port, u32 base)
{
	pr_info("\tgop port %d silicon base address is 0x%x\n", port, base);
	pp3_gmac[port].base = base;
	pp3_gmac[port].flags |= MV_PP3_GMAC_F_ATTACH;
}

/*-------------------------------------------------------------------*/
void pp3_gmac_port_def_init(int port)
{
	/* NSS FPGA def config */
	pp3_gmac_reg_write(port, 0x00000000 , 0x0000d001);/* mac_ctrl0 */
	pp3_gmac_reg_write(port, 0x00000014 , 0x000008c4);/* ser_par */
	pp3_gmac_reg_write(port, 0x00000094 , 0x00000005);/* ser_par_1 */
	pp3_gmac_reg_write(port, 0x00000090 , 0x0000001a);/* mac_ctrl4 */
	pp3_gmac_reg_write(port, 0x00000048 , 0x00000300);/* mac_ctrl3 */
	pp3_gmac_reg_write(port, 0x0000000c , 0x00009222);/* an_ctrl_reg */
	pp3_gmac_reg_write(port, 0x00000008 , 0x00000100);/* mac_ctrl2 */
}

/*-------------------------------------------------------------------*/
void pp3_gmac_port_enable(int port)
{
	u32 rev_val;

	rev_val = pp3_gmac_reg_read(port, GMAC_CTRL_0_REG);
	rev_val |= GMAC_PORT_EN_MASK;
	rev_val |= GMAC_GMAC_MIB_CNTR_EN_MASK;

	pp3_gmac_reg_write(port, GMAC_CTRL_0_REG, rev_val);
}

void pp3_gmac_port_diable(int port)
{
	u32 rev_val;

	rev_val = pp3_gmac_reg_read(port, GMAC_CTRL_0_REG);
	rev_val &= ~(GMAC_PORT_EN_MASK);
	pp3_gmac_reg_write(port, GMAC_CTRL_0_REG, rev_val);
}

static void pp3_gmac_port_rgmii_set(int port, int enable)
{
	u32  rev_val;

	rev_val = pp3_gmac_reg_read(port, GMAC_CTRL_2_REG);

	if (enable)
		rev_val |= GMAC_PORT_RGMII_MASK;
	else
		rev_val &= ~GMAC_PORT_RGMII_MASK;

	pp3_gmac_reg_write(port, GMAC_CTRL_2_REG, rev_val);
}

static void pp3_gmac_port_sgmii_set(int port, int enable)
{
	u32 rev_val;

	rev_val = pp3_gmac_reg_read(port, GMAC_CTRL_2_REG);

	if (enable)
		rev_val |= (GMAC_PCS_ENABLE_MASK | GMAC_INBAND_AN_MASK);
	else
		rev_val &= ~(GMAC_PCS_ENABLE_MASK | GMAC_INBAND_AN_MASK);

	pp3_gmac_reg_write(port, GMAC_CTRL_2_REG, rev_val);
}

void pp3_gmac_port_periodic_xon_set(int port, int enable)
{
	u32 rev_val;

	rev_val = pp3_gmac_reg_read(port, GMAC_CTRL_1_REG);

	if (enable)
		rev_val |= GMAC_PERIODIC_XON_EN_MASK;
	else
		rev_val &= ~GMAC_PERIODIC_XON_EN_MASK;

	pp3_gmac_reg_write(port, GMAC_CTRL_1_REG, rev_val);
}

void pp3_gmac_port_lb_set(int port, int is_gmii, int is_pcs_en)
{
	u32 rev_val;

	rev_val = pp3_gmac_reg_read(port, GMAC_CTRL_1_REG);

	if (is_gmii)
		rev_val |= GMAC_GMII_LB_EN_MASK;
	else
		rev_val &= ~GMAC_GMII_LB_EN_MASK;

	if (is_pcs_en)
		rev_val |= GMAC_PCS_LB_EN_MASK;
	else
		rev_val &= ~GMAC_PCS_LB_EN_MASK;

	pp3_gmac_reg_write(port, GMAC_CTRL_1_REG, rev_val);
}

void pp3_gmac_port_reset_set(int port, bool setReset)
{
	u32 rev_val;

	rev_val = pp3_gmac_reg_read(port, GMAC_CTRL_2_REG);
	rev_val &= ~GMAC_PORT_RESET_MASK;

	if (setReset == 1 /*TRUE*/)
		rev_val |= GMAC_PORT_RESET_MASK;
	else
		rev_val &= ~GMAC_PORT_RESET_MASK;

	pp3_gmac_reg_write(port, GMAC_CTRL_2_REG, rev_val);

	if (setReset == 0 /*FALSE*/) {
		u32 reset = 1;
		while (reset)
			reset = pp3_gmac_reg_read(port, GMAC_CTRL_2_REG) & GMAC_PORT_RESET_MASK;
	}
}

void pp3_gmac_port_power_up(int port, bool is_sgmii, bool is_rgmii)
{
	pp3_gmac_port_sgmii_set(port, is_sgmii);
	pp3_gmac_port_rgmii_set(port, is_rgmii);
	pp3_gmac_port_periodic_xon_set(port, 0 /*FALSE*/);
	pp3_gmac_port_reset_set(port, 0 /*FALSE*/);
}

void pp3_gmac_def_set(int port)
{
	u32 rev_val;

	/* Update TX FIFO MIN Threshold */
	rev_val = pp3_gmac_reg_read(port, GMAC_PORT_FIFO_CFG_1_REG);
	rev_val &= ~GMAC_TX_FIFO_MIN_TH_ALL_MASK;
	/* Minimal TX threshold must be less than minimal packet length */
	rev_val |= GMAC_TX_FIFO_MIN_TH_MASK(64 - 4 - 2);
	pp3_gmac_reg_write(port, GMAC_PORT_FIFO_CFG_1_REG, rev_val);
}

void pp3_gmac_port_power_down(int port)
{
	pp3_gmac_port_diable(port);
	pp3_gmac_mib_counters_clear(port);
	pp3_gmac_port_reset_set(port, 1 /*TRUE*/);
}

bool pp3_gmac_port_is_link_up(int port)
{
	return pp3_gmac_reg_read(port, GMAC_STATUS_REG) & GMAC_LINK_UP_MASK;
}

int pp3_gmac_link_status(int port, struct mv_port_link_status *pstatus)
{
	u32 rev_val;

	rev_val = pp3_gmac_reg_read(port, GMAC_STATUS_REG);

	if (rev_val & GMAC_SPEED_1000_MASK)
		pstatus->speed = MV_PORT_SPEED_1000;
	else if (rev_val & GMAC_SPEED_100_MASK)
		pstatus->speed = MV_PORT_SPEED_100;
	else
		pstatus->speed = MV_PORT_SPEED_10;

	if (rev_val & GMAC_LINK_UP_MASK)
		pstatus->linkup = 1 /*TRUE*/;
	else
		pstatus->linkup = 0 /*FALSE*/;

	if (rev_val & GMAC_FULL_DUPLEX_MASK)
		pstatus->duplex = MV_PORT_DUPLEX_FULL;
	else
		pstatus->duplex = MV_PORT_DUPLEX_HALF;

	if (rev_val & GMAC_TX_FLOW_CTRL_ACTIVE_MASK)
		pstatus->tx_fc = MV_PORT_FC_ACTIVE;
	else if (rev_val & GMAC_TX_FLOW_CTRL_ENABLE_MASK)
		pstatus->tx_fc = MV_PORT_FC_ENABLE;
	else
		pstatus->tx_fc = MV_PORT_FC_DISABLE;

	if (rev_val & GMAC_RX_FLOW_CTRL_ACTIVE_MASK)
		pstatus->rx_fc = MV_PORT_FC_ACTIVE;
	else if (rev_val & GMAC_RX_FLOW_CTRL_ENABLE_MASK)
		pstatus->rx_fc = MV_PORT_FC_ENABLE;
	else
		pstatus->rx_fc = MV_PORT_FC_DISABLE;

	return 0;
}

char *pp3_gmac_speed_str_get(enum mv_port_speed speed)
{
	char *str;

	switch (speed) {
	case MV_PORT_SPEED_10:
		str = "10 Mbps";
		break;
	case MV_PORT_SPEED_100:
		str = "100 Mbps";
		break;
	case MV_PORT_SPEED_1000:
		str = "1 Gbps";
		break;
	case MV_PORT_SPEED_2000:
		str = "2 Gbps";
		break;
	case MV_PORT_SPEED_AN:
		str = "AutoNeg";
		break;
	default:
		str = "Unknown";
	}
	return str;
}

/******************************************************************************/
/*                          Port Configuration functions                      */
/******************************************************************************/

/*******************************************************************************
* mvNetaMaxRxSizeSet -
*
* DESCRIPTION:
*       Change maximum receive size of the port. This configuration will take place
*       imidiately.
*
* INPUT:
*
* RETURN:
*******************************************************************************/
int pp3_gmac_max_rx_size_set(int port, int max_rx_size)
{
	u32		rev_val;

	rev_val =  pp3_gmac_reg_read(port, GMAC_CTRL_0_REG);
	rev_val &= ~GMAC_MAX_RX_SIZE_MASK;
	rev_val |= (((max_rx_size - MV_MH_SIZE) / 2) << GMAC_MAX_RX_SIZE_OFFS);
	pp3_gmac_reg_write(port, GMAC_CTRL_0_REG, rev_val);
	return 0;
}

/*******************************************************************************
* pp3_gmac_force_link_mode_set -
*
* DESCRIPTION:
*       Sets "Force Link Pass" and "Do Not Force Link Fail" bits.
*	Note: This function should only be called when the port is disabled.
*
* INPUT:
*	int  port		- port number
*	bool force_link_pass	- Force Link Pass
*	bool force_link_fail - Force Link Failure
*		0, 0 - normal state: detect link via PHY and connector
*		1, 1 - prohibited state.
*
* RETURN:
*******************************************************************************/
int pp3_gmac_force_link_mode_set(int port, bool force_link_up, bool force_link_down)
{
	u32 rev_val;

	/* Can't force link pass and link fail at the same time */
	if ((force_link_up) && (force_link_down))
		return -EINVAL;

	rev_val = pp3_gmac_reg_read(port, GMAC_AN_CTRL_REG);

	if (force_link_up)
		rev_val |= GMAC_FORCE_LINK_PASS_MASK;
	else
		rev_val &= ~GMAC_FORCE_LINK_PASS_MASK;

	if (force_link_down)
		rev_val |= GMAC_FORCE_LINK_FAIL_MASK;
	else
		rev_val &= ~GMAC_FORCE_LINK_FAIL_MASK;

	pp3_gmac_reg_write(port, GMAC_AN_CTRL_REG, rev_val);

	return 0;
}

/*******************************************************************************
* pp3_gmac_speed_duplex_set -
*
* DESCRIPTION:
*       Sets port speed to Auto Negotiation / 1000 / 100 / 10 Mbps.
*	Sets port duplex to Auto Negotiation / Full / Half Duplex.
*
* INPUT:
*	int port - port number
*	enum mv_port_speed speed - port speed
*	enum mv_port_duplex duplex - port duplex mode
*
* RETURN:
*******************************************************************************/
int pp3_gmac_speed_duplex_set(int port, enum mv_port_speed speed, enum mv_port_duplex duplex)
{
	u32 rev_val;

	/* Check validity */
	if ((speed == MV_PORT_SPEED_1000) && (duplex == MV_PORT_DUPLEX_HALF))
		return -EINVAL;

	rev_val = pp3_gmac_reg_read(port, GMAC_AN_CTRL_REG);

	switch (speed) {
	case MV_PORT_SPEED_AN:
		rev_val |= GMAC_ENABLE_SPEED_AUTO_NEG_MASK;
		/* the other bits don't matter in this case */
		break;
	case MV_PORT_SPEED_1000:
		rev_val &= ~GMAC_ENABLE_SPEED_AUTO_NEG_MASK;
		rev_val |= GMAC_SET_GMII_SPEED_1000_MASK;
		/* the 100/10 bit doesn't matter in this case */
		break;
	case MV_PORT_SPEED_100:
		rev_val &= ~GMAC_ENABLE_SPEED_AUTO_NEG_MASK;
		rev_val &= ~GMAC_SET_GMII_SPEED_1000_MASK;
		rev_val |= GMAC_SET_MII_SPEED_100_MASK;
		break;
	case MV_PORT_SPEED_10:
		rev_val &= ~GMAC_ENABLE_SPEED_AUTO_NEG_MASK;
		rev_val &= ~GMAC_SET_GMII_SPEED_1000_MASK;
		rev_val &= ~GMAC_SET_MII_SPEED_100_MASK;
		break;
	default:
		pr_info("Unexpected Speed value %d\n", speed);
		return -EINVAL;
	}

	switch (duplex) {
	case MV_PORT_DUPLEX_AN:
		rev_val  |= GMAC_ENABLE_DUPLEX_AUTO_NEG_MASK;
		/* the other bits don't matter in this case */
		break;
	case MV_PORT_DUPLEX_HALF:
		rev_val &= ~GMAC_ENABLE_DUPLEX_AUTO_NEG_MASK;
		rev_val &= ~GMAC_SET_FULL_DUPLEX_MASK;
		break;
	case MV_PORT_DUPLEX_FULL:
		rev_val &= ~GMAC_ENABLE_DUPLEX_AUTO_NEG_MASK;
		rev_val |= GMAC_SET_FULL_DUPLEX_MASK;
		break;
	default:
		pr_err("Unexpected Duplex value %d\n", duplex);
		return -EINVAL;
	}

	pp3_gmac_reg_write(port, GMAC_AN_CTRL_REG, rev_val);
	return 0;
}

/*******************************************************************************
* pp3_gmac_speed_duplex_get -
*
* DESCRIPTION:
*       Gets port speed
*	Gets port duplex
*
* INPUT:
*	int port - port number
* OUTPUT:
*	enum mv_port_speed *speed - port speed
*	enum mv_port_duplex *duplex - port duplex mode
*
* RETURN:
*******************************************************************************/
int pp3_gmac_speed_duplex_get(int port, enum mv_port_speed *speed, enum mv_port_duplex *duplex)
{
	u32 rev_val;

	/* Check validity */
	if (!speed || !duplex)
		return -EINVAL;

	rev_val = pp3_gmac_reg_read(port, GMAC_AN_CTRL_REG);
	if (rev_val & GMAC_ENABLE_SPEED_AUTO_NEG_MASK)
		*speed = MV_PORT_SPEED_AN;
	else if (rev_val & GMAC_SET_GMII_SPEED_1000_MASK)
		*speed = MV_PORT_SPEED_1000;
	else if (rev_val & GMAC_SET_MII_SPEED_100_MASK)
		*speed = MV_PORT_SPEED_100;
	else
		*speed = MV_PORT_SPEED_10;

	if (rev_val & GMAC_ENABLE_DUPLEX_AUTO_NEG_MASK)
		*duplex = MV_PORT_DUPLEX_AN;
	else if (rev_val & GMAC_SET_FULL_DUPLEX_MASK)
		*duplex = MV_PORT_DUPLEX_FULL;
	else
		*duplex = MV_PORT_DUPLEX_HALF;

	return 0;
}

/*******************************************************************************
* pp3_gmac_fc_set - Set Flow Control of the port.
*
* DESCRIPTION:
*       This function configures the port's Flow Control properties.
*
* INPUT:
*       int				port		- Port number
*       enum mv_port_fc  fc - Flow control of the port.
*
* RETURN:   int
*       0           - Success
*       MV_OUT_OF_RANGE - Failed. Port is out of valid range
*       MV_BAD_VALUE    - Value fc parameters is not valid
*
*******************************************************************************/
int pp3_gmac_fc_set(int port, enum mv_port_fc fc)
{
	u32 rev_val;

	rev_val = pp3_gmac_reg_read(port, GMAC_AN_CTRL_REG);

	switch (fc) {
	case MV_PORT_FC_AN_NO:
		rev_val |= GMAC_ENABLE_FLOW_CONTROL_AUTO_NEG_MASK;
		rev_val &= ~GMAC_FLOW_CONTROL_ADVERTISE_MASK;
		rev_val &= ~GMAC_FLOW_CONTROL_ASYMETRIC_MASK;
		break;

	case MV_PORT_FC_AN_SYM:
		rev_val |= GMAC_ENABLE_FLOW_CONTROL_AUTO_NEG_MASK;
		rev_val |= GMAC_FLOW_CONTROL_ADVERTISE_MASK;
		rev_val &= ~GMAC_FLOW_CONTROL_ASYMETRIC_MASK;
		break;

	case MV_PORT_FC_AN_ASYM:
		rev_val |= GMAC_ENABLE_FLOW_CONTROL_AUTO_NEG_MASK;
		rev_val |= GMAC_FLOW_CONTROL_ADVERTISE_MASK;
		rev_val |= GMAC_FLOW_CONTROL_ASYMETRIC_MASK;
		break;

	case MV_PORT_FC_DISABLE:
		rev_val &= ~GMAC_ENABLE_FLOW_CONTROL_AUTO_NEG_MASK;
		rev_val &= ~GMAC_SET_FLOW_CONTROL_MASK;
		break;

	case MV_PORT_FC_ENABLE:
		rev_val &= ~GMAC_ENABLE_FLOW_CONTROL_AUTO_NEG_MASK;
		rev_val |= GMAC_SET_FLOW_CONTROL_MASK;
		break;

	default:
		pr_err("ethDrv: Unexpected FlowControl value %d\n", fc);
		return -EINVAL;
	}

	pp3_gmac_reg_write(port, GMAC_AN_CTRL_REG, rev_val);

	return 0;
}

/*******************************************************************************
* pp3_gmac_fc_get - Get Flow Control configuration of the port.
*
* DESCRIPTION:
*       This function returns the port's Flow Control properties.
*
* INPUT:
*       int port			- Port number
*
* OUTPUT:
*       enum mv_port_fc  *fc		- Flow control of the port.
*
*
*******************************************************************************/
void pp3_gmac_fc_get(int port, enum mv_port_fc *fc)
{
	u32 rev_val;

	rev_val = pp3_gmac_reg_read(port, GMAC_AN_CTRL_REG);

	if (rev_val & GMAC_ENABLE_FLOW_CONTROL_AUTO_NEG_MASK) {
		/* Auto negotiation is enabled */
		if (rev_val & GMAC_FLOW_CONTROL_ADVERTISE_MASK) {
			if (rev_val & GMAC_FLOW_CONTROL_ASYMETRIC_MASK)
				*fc = MV_PORT_FC_AN_ASYM;
			else
				*fc = MV_PORT_FC_AN_SYM;
		} else
			*fc = MV_PORT_FC_AN_NO;
	} else {
		/* Auto negotiation is disabled */
		if (rev_val & GMAC_SET_FLOW_CONTROL_MASK)
			*fc = MV_PORT_FC_ENABLE;
		else
			*fc = MV_PORT_FC_DISABLE;
	}
}

int pp3_gmac_port_link_speed_fc(int port, enum mv_port_speed speed,
				     int force_link_up)
{
	if (force_link_up) {
		if (pp3_gmac_speed_duplex_set(port, speed, MV_PORT_DUPLEX_FULL)) {
			pr_err("pp3_gmac_speed_duplex_set failed\n");
			return -EPERM;
		}
		if (pp3_gmac_fc_set(port, MV_PORT_FC_ENABLE)) {
			pr_err("pp3_gmac_fc_set failed\n");
			return -EPERM;
		}
		if (pp3_gmac_force_link_mode_set(port, 1, 0)) {
			pr_err("pp3_gmac_force_link_mode_set failed\n");
			return -EPERM;
		}
	} else {
		if (pp3_gmac_force_link_mode_set(port, 0, 0)) {
			pr_err("pp3_gmac_force_link_mode_set failed\n");
			return -EPERM;
		}
		if (pp3_gmac_speed_duplex_set(port, MV_PORT_SPEED_AN, MV_PORT_DUPLEX_AN)) {
			pr_err("pp3_gmac_speed_duplex_set failed\n");
			return -EPERM;
		}
		if (pp3_gmac_fc_set(port, MV_PORT_FC_AN_SYM)) {
			pr_err("pp3_gmac_fc_set failed\n");
			return -EPERM;
		}
	}

	return 0;
}

/******************************************************************************/
/*                         PHY Control Functions                              */
/******************************************************************************/
void pp3_gmac_phy_addr_set(int port, int phy_addr)
{
	unsigned int rev_val;

	rev_val = mv_pp3_hw_reg_read(PHY_ADDR_REG + pp3_gmac_base_addr_get());

	rev_val &= ~PHY_ADDR_MASK(port);
	rev_val |= ((phy_addr << PHY_ADDR_OFFS(port)) & PHY_ADDR_MASK(port));

	 mv_pp3_hw_reg_write((PHY_ADDR_REG + pp3_gmac_base_addr_get()), rev_val);

	return;
}

int pp3_gmac_phy_addr_get(int port)
{
	unsigned int rev_val;

	rev_val = mv_pp3_hw_reg_read(PHY_ADDR_REG + pp3_gmac_base_addr_get());

	return (rev_val & PHY_ADDR_MASK(port)) >> PHY_ADDR_OFFS(port);
}

/*
void pp3_gmac_lms_regs(void)
{
	pr_info("\n[GoP LMS registers]\n");

	mvGmacPrintReg(PHY_ADDR_REG, "MV_GOP_LMS_PHY_ADDR_REG");
	mvGmacPrintReg(GMAC_PHY_AN_CFG0_REG, "MV_GOP_LMS_PHY_AN_CFG0_REG");
}
*/

void pp3_gmac_port_regs(int port)
{
	/*TODO: port val validation */

	pr_info("\n[gop MAC #%d registers]\n", port);

	pp3_gmac_reg_print(port, "GMAC_CTRL_0_REG", GMAC_CTRL_0_REG);
	pp3_gmac_reg_print(port, "GMAC_CTRL_1_REG", GMAC_CTRL_1_REG);
	pp3_gmac_reg_print(port, "GMAC_CTRL_2_REG", GMAC_CTRL_2_REG);

	pp3_gmac_reg_print(port, "GMAC_AN_CTRL_REG", GMAC_AN_CTRL_REG);
	pp3_gmac_reg_print(port, "GMAC_STATUS_REG", GMAC_STATUS_REG);

	pp3_gmac_reg_print(port, "GMAC_PORT_FIFO_CFG_0_REG", GMAC_PORT_FIFO_CFG_0_REG);
	pp3_gmac_reg_print(port, "GMAC_PORT_FIFO_CFG_1_REG", GMAC_PORT_FIFO_CFG_1_REG);

	pp3_gmac_reg_print(port, "GMAC_PORT_ISR_CAUSE_REG", GMAC_PORT_ISR_CAUSE_REG);
	pp3_gmac_reg_print(port, "GMAC_PORT_ISR_MASK_REG", GMAC_PORT_ISR_CAUSE_REG);
	pp3_gmac_reg_print(port, "GMAC_PORT_ISR_SUM_CAUSE_REG", GMAC_PORT_ISR_SUM_CAUSE_REG);
	pp3_gmac_reg_print(port, "GMAC_PORT_ISR_SUM_MASK_REG", GMAC_PORT_ISR_SUM_MASK_REG);
}

/******************************************************************************/
/*                      MIB Counters functions                                */
/******************************************************************************/

/*******************************************************************************
* pp3_gmac_mib_counter_read - Read a MIB counter
*
* DESCRIPTION:
*       This function reads a MIB counter of a specific ethernet port.
*       NOTE - Read from GMAC_MIB_GOOD_OCTETS_RECEIVED_LOW or
*              GMAC_MIB_GOOD_OCTETS_SENT_LOW counters will return 64 bits value,
*              so p_high_32 pointer should not be NULL in this case.
*
* INPUT:
*       port        - Ethernet Port number.
*       offset   - MIB counter offset.
*
* OUTPUT:
*       u32*       p_high_32 - pointer to place where 32 most significant bits
*                             of the counter will be stored.
*
* RETURN:
*       32 low sgnificant bits of MIB counter value.
*
*******************************************************************************/
u32 pp3_gmac_mib_counter_read(int port, unsigned int offset, u32 *p_high_32)
{
	u32 val_low_32, val_high_32;
	int abs_offset;

	val_low_32 = mv_pp3_hw_reg_read(pp3_gmac_base_addr_get() + GMAC_MIB_COUNTERS_BASE(port) + offset);

	/* Implement FEr ETH. Erroneous Value when Reading the Upper 32-bits    */
	/* of a 64-bit MIB Counter.                                             */
	if ((offset == GMAC_MIB_GOOD_OCTETS_RECEIVED_LOW) || (offset == GMAC_MIB_GOOD_OCTETS_SENT_LOW)) {
		abs_offset = pp3_gmac_base_addr_get() + GMAC_MIB_COUNTERS_BASE(port) + offset + 4;

		val_high_32 = mv_pp3_hw_reg_read(abs_offset);

		if (p_high_32 != NULL)
			*p_high_32 = val_high_32;
	}
	return val_low_32;
}

/*******************************************************************************
* pp3_gmac_mib_counters_clear - Clear all MIB counters
*
* DESCRIPTION:
*       This function clears all MIB counters
*
* INPUT:
*       port      - Ethernet Port number.
*
* RETURN:   void
*
*******************************************************************************/
void pp3_gmac_mib_counters_clear(int port)
{
	int i, abs_offset;

	/* Perform dummy reads from MIB counters */
	/* Read of last counter clear all counter were read before */
	for (i = GMAC_MIB_GOOD_OCTETS_RECEIVED_LOW; i <= GMAC_MIB_LATE_COLLISION; i += 4) {
		abs_offset = pp3_gmac_base_addr_get() + GMAC_MIB_COUNTERS_BASE(port) + i;
		mv_pp3_hw_reg_read(abs_offset);
	}
}

static void pp3_gmac_mib_print(int port, u32 offset, char *mib_name)
{
	u32 reg_low, reg_high = 0;

	reg_low = pp3_gmac_mib_counter_read(port, offset, &reg_high);

	if (!reg_high)
		pr_info("  %-32s: 0x%02x = %u\n", mib_name, offset, reg_low);
	else
		pr_info("  %-32s: 0x%02x = 0x%08x%08x\n", mib_name, offset, reg_high, reg_low);

}

/* Print MIB counters of the Ethernet port */
void pp3_gmac_mib_counters_show(int port)
{
	/* TODO: check port value */

	pr_info("\nMIBs: port=%d, base=0x%x\n", port, GMAC_MIB_COUNTERS_BASE(port));

	pr_info("\n[Rx]\n");
	pp3_gmac_mib_print(port, GMAC_MIB_GOOD_OCTETS_RECEIVED_LOW, "GOOD_OCTETS_RECEIVED");
	pp3_gmac_mib_print(port, GMAC_MIB_BAD_OCTETS_RECEIVED, "BAD_OCTETS_RECEIVED");
	pp3_gmac_mib_print(port, GMAC_MIB_UNICAST_FRAMES_RECEIVED, "UNCAST_FRAMES_RECEIVED");
	pp3_gmac_mib_print(port, GMAC_MIB_BROADCAST_FRAMES_RECEIVED, "BROADCAST_FRAMES_RECEIVED");
	pp3_gmac_mib_print(port, GMAC_MIB_MULTICAST_FRAMES_RECEIVED, "MULTICAST_FRAMES_RECEIVED");

	pr_info("\n[RMON]\n");
	pp3_gmac_mib_print(port, GMAC_MIB_FRAMES_64_OCTETS, "FRAMES_64_OCTETS");
	pp3_gmac_mib_print(port, GMAC_MIB_FRAMES_65_TO_127_OCTETS, "FRAMES_65_TO_127_OCTETS");
	pp3_gmac_mib_print(port, GMAC_MIB_FRAMES_128_TO_255_OCTETS, "FRAMES_128_TO_255_OCTETS");
	pp3_gmac_mib_print(port, GMAC_MIB_FRAMES_256_TO_511_OCTETS, "FRAMES_256_TO_511_OCTETS");
	pp3_gmac_mib_print(port, GMAC_MIB_FRAMES_512_TO_1023_OCTETS, "FRAMES_512_TO_1023_OCTETS");
	pp3_gmac_mib_print(port, GMAC_MIB_FRAMES_1024_TO_MAX_OCTETS, "FRAMES_1024_TO_MAX_OCTETS");

	pr_info("\n[Tx]\n");
	pp3_gmac_mib_print(port, GMAC_MIB_GOOD_OCTETS_SENT_LOW, "GOOD_OCTETS_SENT");
	pp3_gmac_mib_print(port, GMAC_MIB_UNICAST_FRAMES_SENT, "UNICAST_FRAMES_SENT");
	pp3_gmac_mib_print(port, GMAC_MIB_MULTICAST_FRAMES_SENT, "MULTICAST_FRAMES_SENT");
	pp3_gmac_mib_print(port, GMAC_MIB_BROADCAST_FRAMES_SENT, "BROADCAST_FRAMES_SENT");
	pp3_gmac_mib_print(port, GMAC_MIB_CRC_ERRORS_SENT, "CRC_ERRORS_SENT");

	pr_info("\n[FC control]\n");
	pp3_gmac_mib_print(port, GMAC_MIB_FC_RECEIVED, "FC_RECEIVED");
	pp3_gmac_mib_print(port, GMAC_MIB_FC_SENT, "FC_SENT");

	pr_info("\n[Errors]\n");
	pp3_gmac_mib_print(port, GMAC_MIB_RX_FIFO_OVERRUN, "GMAC_MIB_RX_FIFO_OVERRUN");
	pp3_gmac_mib_print(port, GMAC_MIB_UNDERSIZE_RECEIVED, "UNDERSIZE_RECEIVED");
	pp3_gmac_mib_print(port, GMAC_MIB_FRAGMENTS_RECEIVED, "FRAGMENTS_RECEIVED");
	pp3_gmac_mib_print(port, GMAC_MIB_OVERSIZE_RECEIVED, "OVERSIZE_RECEIVED");
	pp3_gmac_mib_print(port, GMAC_MIB_JABBER_RECEIVED, "JABBER_RECEIVED");
	pp3_gmac_mib_print(port, GMAC_MIB_MAC_RECEIVE_ERROR, "MAC_RECEIVE_ERROR");
	pp3_gmac_mib_print(port, GMAC_MIB_BAD_CRC_EVENT, "BAD_CRC_EVENT");
	pp3_gmac_mib_print(port, GMAC_MIB_COLLISION, "COLLISION");
	/* This counter must be read last. Read it clear all the counters */
	pp3_gmac_mib_print(port, GMAC_MIB_LATE_COLLISION, "LATE_COLLISION");
}
