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

#ifdef ARMADA_390
#include <linux/phy.h>
#endif
#include <linux/skbuff.h>
#include <linux/netdevice.h>
#include <linux/etherdevice.h>
#include <linux/platform_device.h>
#include <linux/inetdevice.h>
#include <uapi/linux/ppp_defs.h>

#include <net/ip.h>
#include <net/ipv6.h>

#include "mv_pp2x.h"
#include "mv_gop110_hw.h"

void mv_gop110_register_bases_dump(struct gop_hw *gop)
{
	pr_info("  %-32s: 0x%p\n", "GMAC", gop->gop_110.gmac.base);
	pr_info("  %-32s: 0x%p\n", "XLG_MAC", gop->gop_110.xlg_mac.base);
	pr_info("  %-32s: 0x%p\n", "SERDES", gop->gop_110.serdes.base);
	pr_info("  %-32s: 0x%p\n", "XMIB", gop->gop_110.xmib.base);
	pr_info("  %-32s: 0x%p\n", "SMI", gop->gop_110.smi_base);
	pr_info("  %-32s: 0x%p\n", "XSMI", gop->gop_110.xsmi_base);
	pr_info("  %-32s: 0x%p\n", "MSPG", gop->gop_110.mspg_base);
	pr_info("  %-32s: 0x%p\n", "XPCS", gop->gop_110.xpcs_base);
	pr_info("  %-32s: 0x%p\n", "PTP", gop->gop_110.ptp.base);
	pr_info("  %-32s: 0x%p\n", "RFU1", gop->gop_110.rfu1_base);

}
EXPORT_SYMBOL(mv_gop110_register_bases_dump);

/* print value of unit registers */
void mv_gop110_gmac_regs_dump(struct gop_hw *gop, int port)
{
	int ind;
	char reg_name[32];

	mv_gop110_gmac_print(gop, "PORT_MAC_CTRL0", port,
			     MV_GMAC_PORT_CTRL0_REG);
	mv_gop110_gmac_print(gop, "PORT_MAC_CTRL1", port,
			     MV_GMAC_PORT_CTRL1_REG);
	mv_gop110_gmac_print(gop, "PORT_MAC_CTRL2", port,
			     MV_GMAC_PORT_CTRL2_REG);
	mv_gop110_gmac_print(gop, "PORT_AUTO_NEG_CFG", port,
			     MV_GMAC_PORT_AUTO_NEG_CFG_REG);
	mv_gop110_gmac_print(gop, "PORT_STATUS0", port,
			     MV_GMAC_PORT_STATUS0_REG);
	mv_gop110_gmac_print(gop, "PORT_SERIAL_PARAM_CFG", port,
			     MV_GMAC_PORT_SERIAL_PARAM_CFG_REG);
	mv_gop110_gmac_print(gop, "PORT_FIFO_CFG_0", port,
			     MV_GMAC_PORT_FIFO_CFG_0_REG);
	mv_gop110_gmac_print(gop, "PORT_FIFO_CFG_1", port,
			     MV_GMAC_PORT_FIFO_CFG_1_REG);
	mv_gop110_gmac_print(gop, "PORT_SERDES_CFG0", port,
			     MV_GMAC_PORT_SERDES_CFG0_REG);
	mv_gop110_gmac_print(gop, "PORT_SERDES_CFG1", port,
			     MV_GMAC_PORT_SERDES_CFG1_REG);
	mv_gop110_gmac_print(gop, "PORT_SERDES_CFG2", port,
			     MV_GMAC_PORT_SERDES_CFG2_REG);
	mv_gop110_gmac_print(gop, "PORT_SERDES_CFG3", port,
			     MV_GMAC_PORT_SERDES_CFG3_REG);
	mv_gop110_gmac_print(gop, "PORT_PRBS_STATUS", port,
			     MV_GMAC_PORT_PRBS_STATUS_REG);
	mv_gop110_gmac_print(gop, "PORT_PRBS_ERR_CNTR", port,
			     MV_GMAC_PORT_PRBS_ERR_CNTR_REG);
	mv_gop110_gmac_print(gop, "PORT_STATUS1", port,
			     MV_GMAC_PORT_STATUS1_REG);
	mv_gop110_gmac_print(gop, "PORT_MIB_CNTRS_CTRL", port,
			     MV_GMAC_PORT_MIB_CNTRS_CTRL_REG);
	mv_gop110_gmac_print(gop, "PORT_MAC_CTRL3", port,
			     MV_GMAC_PORT_CTRL3_REG);
	mv_gop110_gmac_print(gop, "QSGMII", port,
			     MV_GMAC_QSGMII_REG);
	mv_gop110_gmac_print(gop, "QSGMII_STATUS", port,
			     MV_GMAC_QSGMII_STATUS_REG);
	mv_gop110_gmac_print(gop, "QSGMII_PRBS_CNTR", port,
			     MV_GMAC_QSGMII_PRBS_CNTR_REG);
	for (ind = 0; ind < 8; ind++) {
		sprintf(reg_name, "CCFC_PORT_SPEED_TIMER%d", ind);
		mv_gop110_gmac_print(gop, reg_name, port,
				     MV_GMAC_CCFC_PORT_SPEED_TIMER_REG(ind));
	}
	for (ind = 0; ind < 4; ind++) {
		sprintf(reg_name, "FC_DSA_TAG%d", ind);
		mv_gop110_gmac_print(gop, reg_name, port,
				     MV_GMAC_FC_DSA_TAG_REG(ind));
	}
	mv_gop110_gmac_print(gop, "LINK_LEVEL_FLOW_CTRL_WIN_REG_0", port,
			     MV_GMAC_LINK_LEVEL_FLOW_CTRL_WINDOW_REG_0);
	mv_gop110_gmac_print(gop, "LINK_LEVEL_FLOW_CTRL_WIN_REG_1", port,
			     MV_GMAC_LINK_LEVEL_FLOW_CTRL_WINDOW_REG_1);
	mv_gop110_gmac_print(gop, "PORT_MAC_CTRL4", port,
			     MV_GMAC_PORT_CTRL4_REG);
	mv_gop110_gmac_print(gop, "PORT_SERIAL_PARAM_1_CFG", port,
			     MV_GMAC_PORT_SERIAL_PARAM_1_CFG_REG);
	mv_gop110_gmac_print(gop, "LPI_CTRL_0", port,
			     MV_GMAC_LPI_CTRL_0_REG);
	mv_gop110_gmac_print(gop, "LPI_CTRL_1", port,
			     MV_GMAC_LPI_CTRL_1_REG);
	mv_gop110_gmac_print(gop, "LPI_CTRL_2", port,
			     MV_GMAC_LPI_CTRL_2_REG);
	mv_gop110_gmac_print(gop, "LPI_STATUS", port,
			     MV_GMAC_LPI_STATUS_REG);
	mv_gop110_gmac_print(gop, "LPI_CNTR", port,
			     MV_GMAC_LPI_CNTR_REG);
	mv_gop110_gmac_print(gop, "PULSE_1_MS_LOW", port,
			     MV_GMAC_PULSE_1_MS_LOW_REG);
	mv_gop110_gmac_print(gop, "PULSE_1_MS_HIGH", port,
			     MV_GMAC_PULSE_1_MS_HIGH_REG);
	mv_gop110_gmac_print(gop, "PORT_INT_MASK", port,
			     MV_GMAC_INTERRUPT_MASK_REG);
	mv_gop110_gmac_print(gop, "INT_SUM_MASK", port,
			     MV_GMAC_INTERRUPT_SUM_MASK_REG);
}
EXPORT_SYMBOL(mv_gop110_gmac_regs_dump);

/* Set the MAC to reset or exit from reset */
int mv_gop110_gmac_reset(struct gop_hw *gop, int mac_num, enum mv_reset reset)
{
	u32 reg_addr;
	u32 val;

	reg_addr = MV_GMAC_PORT_CTRL2_REG;

	/* read - modify - write */
	val = mv_gop110_gmac_read(gop, mac_num, reg_addr);
	if (reset == RESET)
		val |= MV_GMAC_PORT_CTRL2_PORTMACRESET_MASK;
	else
		val &= ~MV_GMAC_PORT_CTRL2_PORTMACRESET_MASK;
	mv_gop110_gmac_write(gop, mac_num, reg_addr, val);

	return 0;
}

static void mv_gop110_gmac_rgmii_cfg(struct gop_hw *gop, int mac_num)
{
	u32 val, thresh, an;

	/* configure minimal level of the Tx FIFO before the lower
	 * part starts to read a packet
	 */
	thresh = MV_RGMII_TX_FIFO_MIN_TH;
	val = mv_gop110_gmac_read(gop, mac_num, MV_GMAC_PORT_FIFO_CFG_1_REG);
	U32_SET_FIELD(val, MV_GMAC_PORT_FIFO_CFG_1_TX_FIFO_MIN_TH_MASK,
		      (thresh << MV_GMAC_PORT_FIFO_CFG_1_TX_FIFO_MIN_TH_OFFS));
	mv_gop110_gmac_write(gop, mac_num, MV_GMAC_PORT_FIFO_CFG_1_REG, val);

	/* Disable bypass of sync module */
	val = mv_gop110_gmac_read(gop, mac_num, MV_GMAC_PORT_CTRL4_REG);
	val |= MV_GMAC_PORT_CTRL4_SYNC_BYPASS_MASK;
	/* configure DP clock select according to mode */
	val &= ~MV_GMAC_PORT_CTRL4_DP_CLK_SEL_MASK;
	val |= MV_GMAC_PORT_CTRL4_QSGMII_BYPASS_ACTIVE_MASK;
	val |= MV_GMAC_PORT_CTRL4_EXT_PIN_GMII_SEL_MASK;
	mv_gop110_gmac_write(gop, mac_num, MV_GMAC_PORT_CTRL4_REG, val);

	val = mv_gop110_gmac_read(gop, mac_num, MV_GMAC_PORT_CTRL2_REG);
	val |= MV_GMAC_PORT_CTRL2_CLK_125_BYPS_EN_MASK;
	val &= ~MV_GMAC_PORT_CTRL2_DIS_PADING_OFFS;
	mv_gop110_gmac_write(gop, mac_num, MV_GMAC_PORT_CTRL2_REG, val);

	val = mv_gop110_gmac_read(gop, mac_num, MV_GMAC_PORT_CTRL0_REG);
	/* configure GIG MAC to SGMII mode */
	val &= ~MV_GMAC_PORT_CTRL0_PORTTYPE_MASK;
	mv_gop110_gmac_write(gop, mac_num, MV_GMAC_PORT_CTRL0_REG, val);

	/* configure AN 0xb8e8 */
	an = MV_GMAC_PORT_AUTO_NEG_CFG_AN_BYPASS_EN_MASK |
		MV_GMAC_PORT_AUTO_NEG_CFG_EN_AN_SPEED_MASK   |
		MV_GMAC_PORT_AUTO_NEG_CFG_EN_FC_AN_MASK      |
		MV_GMAC_PORT_AUTO_NEG_CFG_EN_FDX_AN_MASK     |
		MV_GMAC_PORT_AUTO_NEG_CFG_CHOOSE_SAMPLE_TX_CONFIG_MASK;
	mv_gop110_gmac_write(gop, mac_num, MV_GMAC_PORT_AUTO_NEG_CFG_REG, an);
}

static void mv_gop110_gmac_qsgmii_cfg(struct gop_hw *gop, int mac_num)
{
	u32 val, thresh, an;

	/* configure minimal level of the Tx FIFO before the lower
	 * part starts to read a packet
	 */
	thresh = MV_SGMII_TX_FIFO_MIN_TH;
	val = mv_gop110_gmac_read(gop, mac_num, MV_GMAC_PORT_FIFO_CFG_1_REG);
	U32_SET_FIELD(val, MV_GMAC_PORT_FIFO_CFG_1_TX_FIFO_MIN_TH_MASK,
		      (thresh << MV_GMAC_PORT_FIFO_CFG_1_TX_FIFO_MIN_TH_OFFS));
	mv_gop110_gmac_write(gop, mac_num, MV_GMAC_PORT_FIFO_CFG_1_REG, val);

	/* Disable bypass of sync module */
	val = mv_gop110_gmac_read(gop, mac_num, MV_GMAC_PORT_CTRL4_REG);
	val |= MV_GMAC_PORT_CTRL4_SYNC_BYPASS_MASK;
	/* configure DP clock select according to mode */
	val &= ~MV_GMAC_PORT_CTRL4_DP_CLK_SEL_MASK;
	val &= ~MV_GMAC_PORT_CTRL4_EXT_PIN_GMII_SEL_MASK;
	/* configure QSGMII bypass according to mode */
	val &= ~MV_GMAC_PORT_CTRL4_QSGMII_BYPASS_ACTIVE_MASK;
	mv_gop110_gmac_write(gop, mac_num, MV_GMAC_PORT_CTRL4_REG, val);

	val = mv_gop110_gmac_read(gop, mac_num, MV_GMAC_PORT_CTRL2_REG);
	val &= ~MV_GMAC_PORT_CTRL2_DIS_PADING_OFFS;
	mv_gop110_gmac_write(gop, mac_num, MV_GMAC_PORT_CTRL2_REG, val);

	val = mv_gop110_gmac_read(gop, mac_num, MV_GMAC_PORT_CTRL0_REG);
	/* configure GIG MAC to SGMII mode */
	val &= ~MV_GMAC_PORT_CTRL0_PORTTYPE_MASK;
	mv_gop110_gmac_write(gop, mac_num, MV_GMAC_PORT_CTRL0_REG, val);

	/* configure AN 0xB8EC */
	an = MV_GMAC_PORT_AUTO_NEG_CFG_EN_PCS_AN_MASK |
		MV_GMAC_PORT_AUTO_NEG_CFG_AN_BYPASS_EN_MASK |
		MV_GMAC_PORT_AUTO_NEG_CFG_EN_AN_SPEED_MASK  |
		MV_GMAC_PORT_AUTO_NEG_CFG_EN_FC_AN_MASK     |
		MV_GMAC_PORT_AUTO_NEG_CFG_EN_FDX_AN_MASK    |
		MV_GMAC_PORT_AUTO_NEG_CFG_CHOOSE_SAMPLE_TX_CONFIG_MASK;
	mv_gop110_gmac_write(gop, mac_num, MV_GMAC_PORT_AUTO_NEG_CFG_REG, an);
}

static void mv_gop110_gmac_sgmii_cfg(struct gop_hw *gop, int mac_num)
{
	u32 val, thresh, an;

	/* configure minimal level of the Tx FIFO before the lower
	 * part starts to read a packet
	 */
	thresh = MV_SGMII_TX_FIFO_MIN_TH;
	val = mv_gop110_gmac_read(gop, mac_num, MV_GMAC_PORT_FIFO_CFG_1_REG);
	U32_SET_FIELD(val, MV_GMAC_PORT_FIFO_CFG_1_TX_FIFO_MIN_TH_MASK,
		      (thresh << MV_GMAC_PORT_FIFO_CFG_1_TX_FIFO_MIN_TH_OFFS));
	mv_gop110_gmac_write(gop, mac_num, MV_GMAC_PORT_FIFO_CFG_1_REG, val);

	/* Disable bypass of sync module */
	val = mv_gop110_gmac_read(gop, mac_num, MV_GMAC_PORT_CTRL4_REG);
	val |= MV_GMAC_PORT_CTRL4_SYNC_BYPASS_MASK;
	/* configure DP clock select according to mode */
	val &= ~MV_GMAC_PORT_CTRL4_DP_CLK_SEL_MASK;
	/* configure QSGMII bypass according to mode */
	val |= MV_GMAC_PORT_CTRL4_QSGMII_BYPASS_ACTIVE_MASK;
	mv_gop110_gmac_write(gop, mac_num, MV_GMAC_PORT_CTRL4_REG, val);

	val = mv_gop110_gmac_read(gop, mac_num, MV_GMAC_PORT_CTRL2_REG);
	val |= MV_GMAC_PORT_CTRL2_DIS_PADING_OFFS;
	mv_gop110_gmac_write(gop, mac_num, MV_GMAC_PORT_CTRL2_REG, val);

	val = mv_gop110_gmac_read(gop, mac_num, MV_GMAC_PORT_CTRL0_REG);
	/* configure GIG MAC to SGMII mode */
	val &= ~MV_GMAC_PORT_CTRL0_PORTTYPE_MASK;
	mv_gop110_gmac_write(gop, mac_num, MV_GMAC_PORT_CTRL0_REG, val);

	/* configure AN */
	an = MV_GMAC_PORT_AUTO_NEG_CFG_EN_PCS_AN_MASK |
		MV_GMAC_PORT_AUTO_NEG_CFG_AN_BYPASS_EN_MASK |
		MV_GMAC_PORT_AUTO_NEG_CFG_EN_AN_SPEED_MASK  |
		MV_GMAC_PORT_AUTO_NEG_CFG_EN_FC_AN_MASK     |
		MV_GMAC_PORT_AUTO_NEG_CFG_EN_FDX_AN_MASK    |
		MV_GMAC_PORT_AUTO_NEG_CFG_CHOOSE_SAMPLE_TX_CONFIG_MASK;
	mv_gop110_gmac_write(gop, mac_num, MV_GMAC_PORT_AUTO_NEG_CFG_REG, an);
}

static void mv_gop110_gmac_sgmii2_5_cfg(struct gop_hw *gop, int mac_num)
{
	u32 val, thresh, an;

	/* configure minimal level of the Tx FIFO before the lower
	 * part starts to read a packet
	 */
	thresh = MV_SGMII2_5_TX_FIFO_MIN_TH;
	val = mv_gop110_gmac_read(gop, mac_num, MV_GMAC_PORT_FIFO_CFG_1_REG);
	U32_SET_FIELD(val, MV_GMAC_PORT_FIFO_CFG_1_TX_FIFO_MIN_TH_MASK,
		      (thresh << MV_GMAC_PORT_FIFO_CFG_1_TX_FIFO_MIN_TH_OFFS));
	mv_gop110_gmac_write(gop, mac_num, MV_GMAC_PORT_FIFO_CFG_1_REG, val);

	/* Disable bypass of sync module */
	val = mv_gop110_gmac_read(gop, mac_num, MV_GMAC_PORT_CTRL4_REG);
	val |= MV_GMAC_PORT_CTRL4_SYNC_BYPASS_MASK;
	/* configure DP clock select according to mode */
	val |= MV_GMAC_PORT_CTRL4_DP_CLK_SEL_MASK;
	/* configure QSGMII bypass according to mode */
	val |= MV_GMAC_PORT_CTRL4_QSGMII_BYPASS_ACTIVE_MASK;
	mv_gop110_gmac_write(gop, mac_num, MV_GMAC_PORT_CTRL4_REG, val);

	val = mv_gop110_gmac_read(gop, mac_num, MV_GMAC_PORT_CTRL2_REG);
	val |= MV_GMAC_PORT_CTRL2_DIS_PADING_OFFS;
	mv_gop110_gmac_write(gop, mac_num, MV_GMAC_PORT_CTRL2_REG, val);

	val = mv_gop110_gmac_read(gop, mac_num, MV_GMAC_PORT_CTRL0_REG);
	/* configure GIG MAC to 1000Base-X mode connected to a
	 * fiber transceiver
	 */
	val |= MV_GMAC_PORT_CTRL0_PORTTYPE_MASK;
	mv_gop110_gmac_write(gop, mac_num, MV_GMAC_PORT_CTRL0_REG, val);

	/* configure AN 0x9260 */
	an = MV_GMAC_PORT_AUTO_NEG_CFG_SET_MII_SPEED_MASK  |
		MV_GMAC_PORT_AUTO_NEG_CFG_SET_GMII_SPEED_MASK |
		MV_GMAC_PORT_AUTO_NEG_CFG_ADV_PAUSE_MASK    |
		MV_GMAC_PORT_AUTO_NEG_CFG_SET_FULL_DX_MASK  |
		MV_GMAC_PORT_AUTO_NEG_CFG_CHOOSE_SAMPLE_TX_CONFIG_MASK;
	mv_gop110_gmac_write(gop, mac_num, MV_GMAC_PORT_AUTO_NEG_CFG_REG, an);
}

/* Set the internal mux's to the required MAC in the GOP */
int mv_gop110_gmac_mode_cfg(struct gop_hw *gop, struct mv_mac_data *mac)
{
	u32 reg_addr;
	u32 val;

	int mac_num = mac->gop_index;

	/* Set TX FIFO thresholds */
	switch (mac->phy_mode) {
	case PHY_INTERFACE_MODE_SGMII:
		if (mac->speed == 2500)
			mv_gop110_gmac_sgmii2_5_cfg(gop, mac_num);
		else
			mv_gop110_gmac_sgmii_cfg(gop, mac_num);
	break;
	case PHY_INTERFACE_MODE_RGMII:
		mv_gop110_gmac_rgmii_cfg(gop, mac_num);
	break;
	case PHY_INTERFACE_MODE_QSGMII:
		mv_gop110_gmac_qsgmii_cfg(gop, mac_num);
	break;
	default:
		return -1;
	}

	/* Jumbo frame support - 0x1400*2= 0x2800 bytes */
	val = mv_gop110_gmac_read(gop, mac_num, MV_GMAC_PORT_CTRL0_REG);
	U32_SET_FIELD(val, MV_GMAC_PORT_CTRL0_FRAMESIZELIMIT_MASK,
		      (0x1400 << MV_GMAC_PORT_CTRL0_FRAMESIZELIMIT_OFFS));
	mv_gop110_gmac_write(gop, mac_num, MV_GMAC_PORT_CTRL0_REG, val);

	/* PeriodicXonEn disable */
	reg_addr = MV_GMAC_PORT_CTRL1_REG;
	val = mv_gop110_gmac_read(gop, mac_num, reg_addr);
	val &= ~MV_GMAC_PORT_CTRL1_EN_PERIODIC_FC_XON_MASK;
	mv_gop110_gmac_write(gop, mac_num, reg_addr, val);

	/* mask all ports interrupts */
	mv_gop110_gmac_port_link_event_mask(gop, mac_num);

	/* unmask link change interrupt */
	val = mv_gop110_gmac_read(gop, mac_num, MV_GMAC_INTERRUPT_MASK_REG);
	val |= MV_GMAC_INTERRUPT_CAUSE_LINK_CHANGE_MASK;
	val |= 1; /* unmask summary bit */
	mv_gop110_gmac_write(gop, mac_num, MV_GMAC_INTERRUPT_MASK_REG, val);

	return 0;
}

/* Configure MAC loopback */
int mv_gop110_gmac_loopback_cfg(struct gop_hw *gop, int mac_num,
				enum mv_lb_type type)
{
	u32 reg_addr;
	u32 val;

	reg_addr = MV_GMAC_PORT_CTRL1_REG;
	val = mv_gop110_gmac_read(gop, mac_num, reg_addr);
	switch (type) {
	case MV_DISABLE_LB:
		val &= ~MV_GMAC_PORT_CTRL1_GMII_LOOPBACK_MASK;
		break;
	case MV_TX_2_RX_LB:
		val |= MV_GMAC_PORT_CTRL1_GMII_LOOPBACK_MASK;
		break;
	case MV_RX_2_TX_LB:
	default:
		return -1;
	}
	mv_gop110_gmac_write(gop, mac_num, reg_addr, val);

	return 0;
}

/* Get MAC link status */
bool mv_gop110_gmac_link_status_get(struct gop_hw *gop, int mac_num)
{
	u32 reg_addr;
	u32 val;

	reg_addr = MV_GMAC_PORT_STATUS0_REG;

	val = mv_gop110_gmac_read(gop, mac_num, reg_addr);
	return (val & 1) ? true : false;
}

/* Enable port and MIB counters */
void mv_gop110_gmac_port_enable(struct gop_hw *gop, int mac_num)
{
	u32 reg_val;

	reg_val = mv_gop110_gmac_read(gop, mac_num, MV_GMAC_PORT_CTRL0_REG);
	reg_val |= MV_GMAC_PORT_CTRL0_PORTEN_MASK;
	reg_val |= MV_GMAC_PORT_CTRL0_COUNT_EN_MASK;

	mv_gop110_gmac_write(gop, mac_num, MV_GMAC_PORT_CTRL0_REG, reg_val);
}

/* Disable port */
void mv_gop110_gmac_port_disable(struct gop_hw *gop, int mac_num)
{
	u32 reg_val;

	/* mask all ports interrupts */
	mv_gop110_gmac_port_link_event_mask(gop, mac_num);

	reg_val = mv_gop110_gmac_read(gop, mac_num, MV_GMAC_PORT_CTRL0_REG);
	reg_val &= ~MV_GMAC_PORT_CTRL0_PORTEN_MASK;

	mv_gop110_gmac_write(gop, mac_num, MV_GMAC_PORT_CTRL0_REG, reg_val);
}

void mv_gop110_gmac_port_periodic_xon_set(struct gop_hw *gop,
					  int mac_num, int enable)
{
	u32 reg_val;

	reg_val =  mv_gop110_gmac_read(gop, mac_num, MV_GMAC_PORT_CTRL1_REG);

	if (enable)
		reg_val |= MV_GMAC_PORT_CTRL1_EN_PERIODIC_FC_XON_MASK;
	else
		reg_val &= ~MV_GMAC_PORT_CTRL1_EN_PERIODIC_FC_XON_MASK;

	mv_gop110_gmac_write(gop, mac_num, MV_GMAC_PORT_CTRL1_REG, reg_val);
}

int mv_gop110_gmac_link_status(struct gop_hw *gop, int mac_num,
			       struct mv_port_link_status *pstatus)
{
	u32 reg_val;

	reg_val = mv_gop110_gmac_read(gop, mac_num, MV_GMAC_PORT_STATUS0_REG);

	if (reg_val & MV_GMAC_PORT_STATUS0_GMIISPEED_MASK)
		pstatus->speed = MV_PORT_SPEED_1000;
	else if (reg_val & MV_GMAC_PORT_STATUS0_MIISPEED_MASK)
		pstatus->speed = MV_PORT_SPEED_100;
	else
		pstatus->speed = MV_PORT_SPEED_10;

	if (reg_val & MV_GMAC_PORT_STATUS0_LINKUP_MASK)
		pstatus->linkup = 1 /*TRUE*/;
	else
		pstatus->linkup = 0 /*FALSE*/;

	if (reg_val & MV_GMAC_PORT_STATUS0_FULLDX_MASK)
		pstatus->duplex = MV_PORT_DUPLEX_FULL;
	else
		pstatus->duplex = MV_PORT_DUPLEX_HALF;

	if (reg_val & MV_GMAC_PORT_STATUS0_PORTTXPAUSE_MASK)
		pstatus->tx_fc = MV_PORT_FC_ACTIVE;
	else if (reg_val & MV_GMAC_PORT_STATUS0_TXFCEN_MASK)
		pstatus->tx_fc = MV_PORT_FC_ENABLE;
	else
		pstatus->tx_fc = MV_PORT_FC_DISABLE;

	if (reg_val & MV_GMAC_PORT_STATUS0_PORTRXPAUSE_MASK)
		pstatus->rx_fc = MV_PORT_FC_ACTIVE;
	else if (reg_val & MV_GMAC_PORT_STATUS0_RXFCEN_MASK)
		pstatus->rx_fc = MV_PORT_FC_ENABLE;
	else
		pstatus->rx_fc = MV_PORT_FC_DISABLE;

	return 0;
}

/* Change maximum receive size of the port */
int mv_gop110_gmac_max_rx_size_set(struct gop_hw *gop,
				   int mac_num, int max_rx_size)
{
	u32	reg_val;

	reg_val =  mv_gop110_gmac_read(gop, mac_num, MV_GMAC_PORT_CTRL0_REG);
	reg_val &= ~MV_GMAC_PORT_CTRL0_FRAMESIZELIMIT_MASK;
	reg_val |= (((max_rx_size - MVPP2_MH_SIZE) / 2) <<
			MV_GMAC_PORT_CTRL0_FRAMESIZELIMIT_OFFS);
	mv_gop110_gmac_write(gop, mac_num, MV_GMAC_PORT_CTRL0_REG, reg_val);

	return 0;
}

/* Sets "Force Link Pass" and "Do Not Force Link Fail" bits.
*  This function should only be called when the port is disabled.
* INPUT:
*	int  port		- port number
*	bool force_link_pass	- Force Link Pass
*	bool force_link_fail - Force Link Failure
*		0, 0 - normal state: detect link via PHY and connector
*		1, 1 - prohibited state.
*/
int mv_gop110_gmac_force_link_mode_set(struct gop_hw *gop, int mac_num,
				       bool force_link_up,
				       bool force_link_down)
{
	u32 reg_val;

	/* Can't force link pass and link fail at the same time */
	if ((force_link_up) && (force_link_down))
		return -EINVAL;

	reg_val = mv_gop110_gmac_read(gop, mac_num,
				      MV_GMAC_PORT_AUTO_NEG_CFG_REG);

	if (force_link_up)
		reg_val |= MV_GMAC_PORT_AUTO_NEG_CFG_FORCE_LINK_UP_MASK;
	else
		reg_val &= ~MV_GMAC_PORT_AUTO_NEG_CFG_FORCE_LINK_UP_MASK;

	if (force_link_down)
		reg_val |= MV_GMAC_PORT_AUTO_NEG_CFG_FORCE_LINK_DOWN_MASK;
	else
		reg_val &= ~MV_GMAC_PORT_AUTO_NEG_CFG_FORCE_LINK_DOWN_MASK;

	mv_gop110_gmac_write(gop, mac_num,
			     MV_GMAC_PORT_AUTO_NEG_CFG_REG, reg_val);

	return 0;
}

/* Get "Force Link Pass" and "Do Not Force Link Fail" bits.
* INPUT:
*	int  port		- port number
* OUTPUT:
*	bool *force_link_pass	- Force Link Pass
*	bool *force_link_fail	- Force Link Failure
*/
int mv_gop110_gmac_force_link_mode_get(struct gop_hw *gop, int mac_num,
				       bool *force_link_up,
				       bool *force_link_down)
{
	u32 reg_val;

	/* Can't force link pass and link fail at the same time */
	if ((!force_link_up) || (!force_link_down))
		return -EINVAL;

	reg_val = mv_gop110_gmac_read(gop, mac_num,
				      MV_GMAC_PORT_AUTO_NEG_CFG_REG);

	if (reg_val & MV_GMAC_PORT_AUTO_NEG_CFG_FORCE_LINK_UP_MASK)
		*force_link_up = true;
	else
		*force_link_up = false;

	if (reg_val & MV_GMAC_PORT_AUTO_NEG_CFG_FORCE_LINK_DOWN_MASK)
		*force_link_down = true;
	else
		*force_link_down = false;

	return 0;
}

/* Sets port speed to Auto Negotiation / 1000 / 100 / 10 Mbps.
*  Sets port duplex to Auto Negotiation / Full / Half Duplex.
*/
int mv_gop110_gmac_speed_duplex_set(struct gop_hw *gop, int mac_num,
				    enum mv_port_speed speed,
				    enum mv_port_duplex duplex)
{
	u32 reg_val;

	/* Check validity */
	if ((speed == MV_PORT_SPEED_1000) && (duplex == MV_PORT_DUPLEX_HALF))
		return -EINVAL;

	reg_val = mv_gop110_gmac_read(gop, mac_num,
				      MV_GMAC_PORT_AUTO_NEG_CFG_REG);

	switch (speed) {
	case MV_PORT_SPEED_AN:
		reg_val |= MV_GMAC_PORT_AUTO_NEG_CFG_EN_AN_SPEED_MASK;
		/* the other bits don't matter in this case */
		break;
	case MV_PORT_SPEED_2500:
	case MV_PORT_SPEED_1000:
		reg_val &= ~MV_GMAC_PORT_AUTO_NEG_CFG_EN_AN_SPEED_MASK;
		reg_val |= MV_GMAC_PORT_AUTO_NEG_CFG_SET_GMII_SPEED_MASK;
		/* the 100/10 bit doesn't matter in this case */
		break;
	case MV_PORT_SPEED_100:
		reg_val &= ~MV_GMAC_PORT_AUTO_NEG_CFG_EN_AN_SPEED_MASK;
		reg_val &= ~MV_GMAC_PORT_AUTO_NEG_CFG_SET_GMII_SPEED_MASK;
		reg_val |= MV_GMAC_PORT_AUTO_NEG_CFG_SET_MII_SPEED_MASK;
		break;
	case MV_PORT_SPEED_10:
		reg_val &= ~MV_GMAC_PORT_AUTO_NEG_CFG_EN_AN_SPEED_MASK;
		reg_val &= ~MV_GMAC_PORT_AUTO_NEG_CFG_SET_GMII_SPEED_MASK;
		reg_val &= ~MV_GMAC_PORT_AUTO_NEG_CFG_SET_MII_SPEED_MASK;
		break;
	default:
		pr_info("GMAC: Unexpected Speed value %d\n", speed);
		return -EINVAL;
	}

	switch (duplex) {
	case MV_PORT_DUPLEX_AN:
		reg_val  |= MV_GMAC_PORT_AUTO_NEG_CFG_EN_FDX_AN_MASK;
		/* the other bits don't matter in this case */
		break;
	case MV_PORT_DUPLEX_HALF:
		reg_val &= ~MV_GMAC_PORT_AUTO_NEG_CFG_EN_FDX_AN_MASK;
		reg_val &= ~MV_GMAC_PORT_AUTO_NEG_CFG_SET_FULL_DX_MASK;
		break;
	case MV_PORT_DUPLEX_FULL:
		reg_val &= ~MV_GMAC_PORT_AUTO_NEG_CFG_EN_FDX_AN_MASK;
		reg_val |= MV_GMAC_PORT_AUTO_NEG_CFG_SET_FULL_DX_MASK;
		break;
	default:
		pr_err("GMAC: Unexpected Duplex value %d\n", duplex);
		return -EINVAL;
	}

	mv_gop110_gmac_write(gop, mac_num,
			     MV_GMAC_PORT_AUTO_NEG_CFG_REG, reg_val);
	return 0;
}

/* Gets port speed and duplex */
int mv_gop110_gmac_speed_duplex_get(struct gop_hw *gop, int mac_num,
				    enum mv_port_speed *speed,
				    enum mv_port_duplex *duplex)
{
	u32 reg_val;

	/* Check validity */
	if (!speed || !duplex)
		return -EINVAL;

	reg_val = mv_gop110_gmac_read(gop, mac_num,
				      MV_GMAC_PORT_AUTO_NEG_CFG_REG);

	if (reg_val & MV_GMAC_PORT_AUTO_NEG_CFG_EN_AN_SPEED_MASK)
		*speed = MV_PORT_SPEED_AN;
	else if (reg_val & MV_GMAC_PORT_AUTO_NEG_CFG_SET_GMII_SPEED_MASK)
		*speed = MV_PORT_SPEED_1000;
	else if (reg_val & MV_GMAC_PORT_AUTO_NEG_CFG_SET_MII_SPEED_MASK)
		*speed = MV_PORT_SPEED_100;
	else
		*speed = MV_PORT_SPEED_10;

	if (reg_val & MV_GMAC_PORT_AUTO_NEG_CFG_EN_FDX_AN_MASK)
		*duplex = MV_PORT_DUPLEX_AN;
	else if (reg_val & MV_GMAC_PORT_AUTO_NEG_CFG_SET_FULL_DX_MASK)
		*duplex = MV_PORT_DUPLEX_FULL;
	else
		*duplex = MV_PORT_DUPLEX_HALF;

	return 0;
}

/* Configure the port's Flow Control properties */
int mv_gop110_gmac_fc_set(struct gop_hw *gop, int mac_num, enum mv_port_fc fc)
{
	u32 reg_val;
	u32 fc_en;

	reg_val = mv_gop110_gmac_read(gop, mac_num,
				      MV_GMAC_PORT_AUTO_NEG_CFG_REG);

	switch (fc) {
	case MV_PORT_FC_AN_NO:
		reg_val &= ~MV_GMAC_PORT_AUTO_NEG_CFG_EN_FC_AN_MASK;
		reg_val &= ~MV_GMAC_PORT_AUTO_NEG_CFG_ADV_PAUSE_MASK;
		reg_val &= ~MV_GMAC_PORT_AUTO_NEG_CFG_ADV_ASM_PAUSE_MASK;
		break;

	case MV_PORT_FC_AN_SYM:
		reg_val |= MV_GMAC_PORT_AUTO_NEG_CFG_EN_FC_AN_MASK;
		reg_val |= MV_GMAC_PORT_AUTO_NEG_CFG_ADV_PAUSE_MASK;
		reg_val &= ~MV_GMAC_PORT_AUTO_NEG_CFG_ADV_ASM_PAUSE_MASK;
		break;

	case MV_PORT_FC_AN_ASYM:
		reg_val |= MV_GMAC_PORT_AUTO_NEG_CFG_EN_FC_AN_MASK;
		reg_val |= MV_GMAC_PORT_AUTO_NEG_CFG_ADV_PAUSE_MASK;
		reg_val |= MV_GMAC_PORT_AUTO_NEG_CFG_ADV_ASM_PAUSE_MASK;
		break;

	case MV_PORT_FC_DISABLE:
		reg_val &= ~MV_GMAC_PORT_AUTO_NEG_CFG_EN_FC_AN_MASK;
		reg_val &= ~MV_GMAC_PORT_AUTO_NEG_CFG_ADV_ASM_PAUSE_MASK;
		fc_en = mv_gop110_gmac_read(gop, mac_num,
					    MV_GMAC_PORT_CTRL4_REG);
		fc_en &= ~MV_GMAC_PORT_CTRL4_FC_EN_RX_MASK;
		fc_en &= ~MV_GMAC_PORT_CTRL4_FC_EN_TX_MASK;
		mv_gop110_gmac_write(gop, mac_num,
				     MV_GMAC_PORT_CTRL4_REG, fc_en);
		break;

	case MV_PORT_FC_ENABLE:
		reg_val &= ~MV_GMAC_PORT_AUTO_NEG_CFG_EN_FC_AN_MASK;
		fc_en = mv_gop110_gmac_read(gop, mac_num,
					    MV_GMAC_PORT_CTRL4_REG);
		fc_en |= MV_GMAC_PORT_CTRL4_FC_EN_RX_MASK;
		fc_en |= MV_GMAC_PORT_CTRL4_FC_EN_TX_MASK;
		mv_gop110_gmac_write(gop, mac_num,
				     MV_GMAC_PORT_CTRL4_REG, fc_en);
		break;

	default:
		pr_err("GMAC: Unexpected FlowControl value %d\n", fc);
		return -EINVAL;
	}

	mv_gop110_gmac_write(gop, mac_num,
			     MV_GMAC_PORT_AUTO_NEG_CFG_REG, reg_val);
	return 0;
}

/* Get Flow Control configuration of the port */
void mv_gop110_gmac_fc_get(struct gop_hw *gop, int mac_num,
			   enum mv_port_fc *fc)
{
	u32 reg_val;

	reg_val = mv_gop110_gmac_read(gop, mac_num,
				      MV_GMAC_PORT_AUTO_NEG_CFG_REG);

	if (reg_val & MV_GMAC_PORT_AUTO_NEG_CFG_EN_FC_AN_MASK) {
		/* Auto negotiation is enabled */
		if (reg_val & MV_GMAC_PORT_AUTO_NEG_CFG_ADV_PAUSE_MASK) {
			if (reg_val &
			    MV_GMAC_PORT_AUTO_NEG_CFG_ADV_ASM_PAUSE_MASK)
				*fc = MV_PORT_FC_AN_ASYM;
			else
				*fc = MV_PORT_FC_AN_SYM;
		} else {
			*fc = MV_PORT_FC_AN_NO;
		}
	} else {
		/* Auto negotiation is disabled */
		reg_val = mv_gop110_gmac_read(gop, mac_num,
					      MV_GMAC_PORT_CTRL4_REG);
		if ((reg_val & MV_GMAC_PORT_CTRL4_FC_EN_RX_MASK) &&
		    (reg_val & MV_GMAC_PORT_CTRL4_FC_EN_TX_MASK))
			*fc = MV_PORT_FC_ENABLE;
		else
			*fc = MV_PORT_FC_DISABLE;
	}
}

int mv_gop110_gmac_port_link_speed_fc(struct gop_hw *gop, int mac_num,
				      enum mv_port_speed speed,
				      int force_link_up)
{
	if (force_link_up) {
		if (mv_gop110_gmac_speed_duplex_set(gop, mac_num, speed,
						    MV_PORT_DUPLEX_FULL)) {
			pr_err("mv_gop110_gmac_speed_duplex_set failed\n");
			return -EPERM;
		}
		if (mv_gop110_gmac_fc_set(gop, mac_num, MV_PORT_FC_ENABLE)) {
			pr_err("mv_gop110_gmac_fc_set failed\n");
			return -EPERM;
		}
		if (mv_gop110_gmac_force_link_mode_set(gop, mac_num, 1, 0)) {
			pr_err("mv_gop110_gmac_force_link_mode_set failed\n");
			return -EPERM;
		}
	} else {
		if (mv_gop110_gmac_force_link_mode_set(gop, mac_num, 0, 0)) {
			pr_err("mv_gop110_gmac_force_link_mode_set failed\n");
			return -EPERM;
		}
		if (mv_gop110_gmac_speed_duplex_set(gop, mac_num,
						    MV_PORT_SPEED_AN,
						    MV_PORT_DUPLEX_AN)) {
			pr_err("mv_gop110_gmac_speed_duplex_set failed\n");
			return -EPERM;
		}
		if (mv_gop110_gmac_fc_set(gop, mac_num, MV_PORT_FC_AN_SYM)) {
			pr_err("mv_gop110_gmac_fc_set failed\n");
			return -EPERM;
		}
	}

	return 0;
}

void mv_gop110_gmac_port_link_event_mask(struct gop_hw *gop, int mac_num)
{
	u32 reg_val;

	reg_val = mv_gop110_gmac_read(gop, mac_num,
				      MV_GMAC_INTERRUPT_SUM_MASK_REG);
	reg_val &= ~MV_GMAC_INTERRUPT_SUM_CAUSE_LINK_CHANGE_MASK;
	mv_gop110_gmac_write(gop, mac_num,
			     MV_GMAC_INTERRUPT_SUM_MASK_REG, reg_val);
}

void mv_gop110_gmac_port_link_event_unmask(struct gop_hw *gop, int mac_num)
{
	u32 reg_val;

	reg_val = mv_gop110_gmac_read(gop, mac_num,
				      MV_GMAC_INTERRUPT_SUM_MASK_REG);
	reg_val |= MV_GMAC_INTERRUPT_SUM_CAUSE_LINK_CHANGE_MASK;
	reg_val |= 1; /* unmask summary bit */
	mv_gop110_gmac_write(gop, mac_num,
			     MV_GMAC_INTERRUPT_SUM_MASK_REG, reg_val);
}

void mv_gop110_gmac_port_link_event_clear(struct gop_hw *gop, int mac_num)
{
	u32 reg_val;

	reg_val = mv_gop110_gmac_read(gop, mac_num,
				      MV_GMAC_INTERRUPT_CAUSE_REG);
}

int mv_gop110_gmac_port_autoneg_restart(struct gop_hw *gop, int mac_num)
{
	u32 reg_val;

	reg_val = mv_gop110_gmac_read(gop, mac_num,
				      MV_GMAC_PORT_AUTO_NEG_CFG_REG);
	/* enable AN and restart it */
	reg_val |= MV_GMAC_PORT_AUTO_NEG_CFG_EN_PCS_AN_MASK;
	reg_val |= MV_GMAC_PORT_AUTO_NEG_CFG_INBAND_RESTARTAN_MASK;
	mv_gop110_gmac_write(gop, mac_num,
			     MV_GMAC_PORT_AUTO_NEG_CFG_REG, reg_val);
	return 0;
}

/*************************************************************************
* mv_port_init
*
* DESCRIPTION:
*       Init physical port. Configures the port mode and all it's elements
*       accordingly.
*       Does not verify that the selected mode/port number is valid at the
*       core level.
*
* INPUTS:
*       port_num    - physical port number
*       port_mode   - port standard metric
*
* OUTPUTS:
*       None.
*
* RETURNS:
*       0  - on success
*       1  - on error
*
*************************************************************************/
int mv_gop110_port_init(struct gop_hw *gop, struct mv_mac_data *mac)
{
	int num_of_act_lanes;
	int mac_num = mac->gop_index;

	if (mac_num >= MVCPN110_GOP_MAC_NUM) {
		pr_err("%s: illegal port number %d", __func__, mac_num);
		return -1;
	}
	MVPP2_PRINT_VAR(mac_num);
	MVPP2_PRINT_VAR(mac->phy_mode);

	switch (mac->phy_mode) {
	case PHY_INTERFACE_MODE_RGMII:
		mv_gop110_force_link_mode_set(gop, mac, false, true);
		mv_gop110_gmac_reset(gop, mac_num, RESET);
		/* configure PCS */
		mv_gop110_gpcs_mode_cfg(gop, mac_num, false);

		/* configure MAC */
		mv_gop110_gmac_mode_cfg(gop, mac);

		/* select proper Mac mode */
		mv_gop110_xlg_2_gig_mac_cfg(gop, mac_num);

		/* pcs unreset */
		mv_gop110_gpcs_reset(gop, mac_num, UNRESET);
		/* mac unreset */
		mv_gop110_gmac_reset(gop, mac_num, UNRESET);
		mv_gop110_force_link_mode_set(gop, mac, false, false);
	break;
	case PHY_INTERFACE_MODE_SGMII:
	case PHY_INTERFACE_MODE_QSGMII:
		num_of_act_lanes = 1;
		mv_gop110_force_link_mode_set(gop, mac, false, true);
		mv_gop110_gmac_reset(gop, mac_num, RESET);
		/* configure PCS */
		mv_gop110_gpcs_mode_cfg(gop, mac_num, true);

		/* configure MAC */
		mv_gop110_gmac_mode_cfg(gop, mac);
		/* select proper Mac mode */
		mv_gop110_xlg_2_gig_mac_cfg(gop, mac_num);

		/* pcs unreset */
		mv_gop110_gpcs_reset(gop, mac_num, UNRESET);
		/* mac unreset */
		mv_gop110_gmac_reset(gop, mac_num, UNRESET);
		mv_gop110_force_link_mode_set(gop, mac, false, false);
	break;
	case PHY_INTERFACE_MODE_XAUI:
		num_of_act_lanes = 4;
		mac_num = 0;
		/* configure PCS */
		mv_gop110_xpcs_mode(gop, num_of_act_lanes);
		/* configure MAC */
		mv_gop110_xlg_mac_mode_cfg(gop, mac_num, num_of_act_lanes);

		/* pcs unreset */
		mv_gop110_xpcs_reset(gop, UNRESET);
		/* mac unreset */
		mv_gop110_xlg_mac_reset(gop, mac_num, UNRESET);
	break;
	case PHY_INTERFACE_MODE_RXAUI:
		num_of_act_lanes = 2;
		/* mapped to serdes 6 */
		mv_gop110_serdes_init(gop, 0, MV_RXAUI);
		/* mapped to serdes 5 */
		mv_gop110_serdes_init(gop, 1, MV_RXAUI);

		mac_num = 0;
		/* configure PCS */
		mv_gop110_xpcs_mode(gop, num_of_act_lanes);
		/* configure MAC */
		mv_gop110_xlg_mac_mode_cfg(gop, mac_num, num_of_act_lanes);

		/* pcs unreset */
		mv_gop110_xpcs_reset(gop, UNRESET);

		/* mac unreset */
		mv_gop110_xlg_mac_reset(gop, mac_num, UNRESET);

		/* run digital reset / unreset */
		mv_gop110_serdes_reset(gop, 0, false, false, true);
		mv_gop110_serdes_reset(gop, 1, false, false, true);
		mv_gop110_serdes_reset(gop, 0, false, false, false);
		mv_gop110_serdes_reset(gop, 1, false, false, false);
	break;
	case PHY_INTERFACE_MODE_KR:

		num_of_act_lanes = 2;
		mac_num = 0;
		/* configure PCS */
		mv_gop110_xpcs_mode(gop, num_of_act_lanes);
		mv_gop110_mpcs_mode(gop);
		/* configure MAC */
		mv_gop110_xlg_mac_mode_cfg(gop, mac_num, num_of_act_lanes);

		/* pcs unreset */
		mv_gop110_xpcs_reset(gop, UNRESET);

		/* mac unreset */
		mv_gop110_xlg_mac_reset(gop, mac_num, UNRESET);
	break;
	default:
		pr_err("%s: Requested port mode (%d) not supported",
		       __func__, mac->phy_mode);
		return -1;
	}

	return 0;
}

/**************************************************************************
* mv_port_reset
*
* DESCRIPTION:
*       Clears the port mode and release all its resources
*       according to selected.
*       Does not verify that the selected mode/port number is valid at the core
*       level and actual terminated mode.
*
* INPUTS:
*       port_num   - physical port number
*       port_mode  - port standard metric
*       action     - Power down or reset
*
* OUTPUTS:
*       None.
*
* RETURNS:
*       0  - on success
*       1  - on error
*
**************************************************************************/
int mv_gop110_port_reset(struct gop_hw *gop, struct mv_mac_data *mac)
{
	int mac_num = mac->gop_index;

	switch (mac->phy_mode) {
	case PHY_INTERFACE_MODE_RGMII:
	case PHY_INTERFACE_MODE_SGMII:
	case PHY_INTERFACE_MODE_QSGMII:
		/* pcs unreset */
		mv_gop110_gpcs_reset(gop, mac_num, RESET);
		/* mac unreset */
		mv_gop110_gmac_reset(gop, mac_num, RESET);
	break;
	case PHY_INTERFACE_MODE_XAUI:
		/* pcs unreset */
		mv_gop110_xpcs_reset(gop, RESET);
		/* mac unreset */
		mv_gop110_xlg_mac_reset(gop, mac_num, RESET);
	break;
	case PHY_INTERFACE_MODE_RXAUI:
		/* pcs unreset */
		mv_gop110_xpcs_reset(gop, RESET);
		/* mac unreset */
		mv_gop110_xlg_mac_reset(gop, mac_num, RESET);
	break;
	/* Stefan: need to check KR case */
	case PHY_INTERFACE_MODE_KR:
		/* pcs unreset */
		mv_gop110_xpcs_reset(gop, RESET);
		/* mac unreset */
		mv_gop110_xlg_mac_reset(gop, mac_num, RESET);
	break;
	default:
		pr_err("%s: Wrong port mode (%d)", __func__, mac->phy_mode);
		return -1;
	}

	/* TBD:serdes reset or power down if needed*/

	return 0;
}

/*-------------------------------------------------------------------*/
void mv_gop110_port_enable(struct gop_hw *gop, struct mv_mac_data *mac)
{
	int port_num = mac->gop_index;

	switch (mac->phy_mode) {
	case PHY_INTERFACE_MODE_RGMII:
	case PHY_INTERFACE_MODE_SGMII:
	case PHY_INTERFACE_MODE_QSGMII:
		mv_gop110_gmac_port_enable(gop, port_num);
		mv_gop110_force_link_mode_set(gop, mac, false, false);
		mv_gop110_gmac_reset(gop, port_num, UNRESET);
	break;
	case PHY_INTERFACE_MODE_XAUI:
	case PHY_INTERFACE_MODE_RXAUI:
	case PHY_INTERFACE_MODE_KR:
		mv_gop110_xlg_mac_port_enable(gop, port_num);
	break;
	default:
		pr_err("%s: Wrong port mode (%d)", __func__, mac->phy_mode);
		return;
	}
}

void mv_gop110_port_disable(struct gop_hw *gop, struct mv_mac_data *mac)
{
	int port_num = mac->gop_index;

	switch (mac->phy_mode) {
	case PHY_INTERFACE_MODE_RGMII:
	case PHY_INTERFACE_MODE_SGMII:
	case PHY_INTERFACE_MODE_QSGMII:
		mv_gop110_gmac_port_disable(gop, port_num);
		mv_gop110_force_link_mode_set(gop, mac, false, true);
		mv_gop110_gmac_reset(gop, port_num, RESET);
	break;
	case PHY_INTERFACE_MODE_XAUI:
	case PHY_INTERFACE_MODE_RXAUI:
	case PHY_INTERFACE_MODE_KR:
		mv_gop110_xlg_mac_port_disable(gop, port_num);
	break;
	default:
		pr_err("%s: Wrong port mode (%d)", __func__, mac->phy_mode);
		return;
	}
}

void mv_gop110_port_periodic_xon_set(struct gop_hw *gop,
				     struct mv_mac_data *mac,
				     int enable)
{
	int port_num = mac->gop_index;

	switch (mac->phy_mode) {
	case PHY_INTERFACE_MODE_SGMII:
	case PHY_INTERFACE_MODE_QSGMII:
		mv_gop110_gmac_port_periodic_xon_set(gop, port_num, enable);
	break;
	case PHY_INTERFACE_MODE_XAUI:
	case PHY_INTERFACE_MODE_RXAUI:
	case PHY_INTERFACE_MODE_KR:
		mv_gop110_xlg_mac_port_periodic_xon_set(gop, port_num, enable);
	break;
	default:
		pr_err("%s: Wrong port mode (%d)", __func__, mac->phy_mode);
		return;
	}
}

bool mv_gop110_port_is_link_up(struct gop_hw *gop, struct mv_mac_data *mac)
{
	int port_num = mac->gop_index;

	switch (mac->phy_mode) {
	case PHY_INTERFACE_MODE_RGMII:
	case PHY_INTERFACE_MODE_SGMII:
	case PHY_INTERFACE_MODE_QSGMII:
		return mv_gop110_gmac_link_status_get(gop, port_num);
	break;
	case PHY_INTERFACE_MODE_XAUI:
	case PHY_INTERFACE_MODE_RXAUI:
	case PHY_INTERFACE_MODE_KR:
		udelay(1000);
		return mv_gop110_xlg_mac_link_status_get(gop, port_num);
	break;
	default:
		pr_err("%s: Wrong port mode gop_port(%d), phy_mode(%d)",
		       __func__, port_num, mac->phy_mode);
		return false;
	}
}

int mv_gop110_port_link_status(struct gop_hw *gop, struct mv_mac_data *mac,
			       struct mv_port_link_status *pstatus)
{
	int port_num = mac->gop_index;

	switch (mac->phy_mode) {
	case PHY_INTERFACE_MODE_RGMII:
	case PHY_INTERFACE_MODE_SGMII:
	case PHY_INTERFACE_MODE_QSGMII:
		mv_gop110_gmac_link_status(gop, port_num, pstatus);
	break;
	case PHY_INTERFACE_MODE_XAUI:
	case PHY_INTERFACE_MODE_RXAUI:
	case PHY_INTERFACE_MODE_KR:
		mv_gop110_xlg_mac_link_status(gop, port_num, pstatus);
	break;
	default:
		pr_err("%s: Wrong port mode (%d)", __func__, mac->phy_mode);
		return -1;
	}
	return 0;
}

bool mv_gop110_port_autoneg_status(struct gop_hw *gop, struct mv_mac_data *mac)
{
	u32 reg_val;

		reg_val = mv_gop110_gmac_read(gop, mac->gop_index, MV_GMAC_PORT_AUTO_NEG_CFG_REG);

		if (reg_val & MV_GMAC_PORT_AUTO_NEG_CFG_EN_FDX_AN_OFFS &&
			reg_val & MV_GMAC_PORT_AUTO_NEG_CFG_EN_AN_SPEED_MASK)
			return true;
		else
			return false;
}

int mv_gop110_check_port_type(struct gop_hw *gop, int port_num)
{
	u32 reg_val;

	reg_val = mv_gop110_gmac_read(gop, port_num, MV_GMAC_PORT_CTRL0_REG);
	return (reg_val & MV_GMAC_PORT_CTRL0_PORTTYPE_MASK) >>
			MV_GMAC_PORT_CTRL0_PORTTYPE_OFFS;
}

void mv_gop110_gmac_set_autoneg(struct gop_hw *gop, struct mv_mac_data *mac, bool auto_neg)
{
	u32 reg_val;
	int mac_num = mac->gop_index;

	reg_val = mv_gop110_gmac_read(gop, mac_num,
				MV_GMAC_PORT_AUTO_NEG_CFG_REG);

	if (auto_neg) {
		reg_val |= MV_GMAC_PORT_AUTO_NEG_CFG_EN_AN_SPEED_MASK;
		reg_val |= MV_GMAC_PORT_AUTO_NEG_CFG_EN_FDX_AN_MASK;
		}

	else {
		reg_val &= ~MV_GMAC_PORT_AUTO_NEG_CFG_EN_AN_SPEED_MASK;
		reg_val &= ~MV_GMAC_PORT_AUTO_NEG_CFG_EN_FDX_AN_MASK;
		}

	mv_gop110_gmac_write(gop, mac_num,
				MV_GMAC_PORT_AUTO_NEG_CFG_REG, reg_val);
}


int mv_gop110_port_regs(struct gop_hw *gop, struct mv_mac_data *mac)
{
	int port_num = mac->gop_index;

	switch (mac->phy_mode) {
	case PHY_INTERFACE_MODE_RGMII:
	case PHY_INTERFACE_MODE_SGMII:
	case PHY_INTERFACE_MODE_QSGMII:
		pr_info("\n[gop GMAC #%d registers]\n", port_num);
		mv_gop110_gmac_regs_dump(gop, port_num);
	break;
	case PHY_INTERFACE_MODE_XAUI:
	case PHY_INTERFACE_MODE_RXAUI:
	case PHY_INTERFACE_MODE_KR:
		pr_info("\n[gop XLG MAC #%d registers]\n", port_num);
		mv_gop110_xlg_mac_regs_dump(gop, port_num);
	break;
	default:
		pr_err("%s: Wrong port mode (%d)", __func__, mac->phy_mode);
		return -1;
	}
	return 0;
}

int mv_gop110_port_events_mask(struct gop_hw *gop, struct mv_mac_data *mac)
{
	int port_num = mac->gop_index;

	switch (mac->phy_mode) {
	case PHY_INTERFACE_MODE_RGMII:
	case PHY_INTERFACE_MODE_SGMII:
	case PHY_INTERFACE_MODE_QSGMII:
		mv_gop110_gmac_port_link_event_mask(gop, port_num);
	break;
	case PHY_INTERFACE_MODE_XAUI:
	case PHY_INTERFACE_MODE_RXAUI:
	case PHY_INTERFACE_MODE_KR:
		mv_gop110_xlg_port_link_event_mask(gop, port_num);
	break;
	default:
		pr_err("%s: Wrong port mode (%d)", __func__, mac->phy_mode);
		return -1;
	}
	return 0;
}

int mv_gop110_port_events_unmask(struct gop_hw *gop, struct mv_mac_data *mac)
{
	int port_num = mac->gop_index;

	switch (mac->phy_mode) {
	case PHY_INTERFACE_MODE_RGMII:
	case PHY_INTERFACE_MODE_SGMII:
	case PHY_INTERFACE_MODE_QSGMII:
		mv_gop110_gmac_port_link_event_unmask(gop, port_num);
		/* gige interrupt cause connected to CPU via XLG
		 * external interrupt cause
		 */
		mv_gop110_xlg_port_external_event_unmask(gop, 0, 2);
	break;
	case PHY_INTERFACE_MODE_XAUI:
	case PHY_INTERFACE_MODE_RXAUI:
	case PHY_INTERFACE_MODE_KR:
		mv_gop110_xlg_port_external_event_unmask(gop, port_num, 1);
	break;
	default:
		pr_err("%s: Wrong port mode (%d)", __func__, mac->phy_mode);
		return -1;
	}
	return 0;
}

int mv_gop110_port_events_clear(struct gop_hw *gop, struct mv_mac_data *mac)
{
	int port_num = mac->gop_index;

	switch (mac->phy_mode) {
	case PHY_INTERFACE_MODE_RGMII:
	case PHY_INTERFACE_MODE_SGMII:
	case PHY_INTERFACE_MODE_QSGMII:
		mv_gop110_gmac_port_link_event_clear(gop, port_num);
	break;
	case PHY_INTERFACE_MODE_XAUI:
	case PHY_INTERFACE_MODE_RXAUI:
	case PHY_INTERFACE_MODE_KR:
		mv_gop110_xlg_port_link_event_clear(gop, port_num);
	break;
	default:
		pr_err("%s: Wrong port mode (%d)", __func__, mac->phy_mode);
		return -1;
	}
	return 0;
}

int mv_gop110_status_show(struct gop_hw *gop, struct mv_pp2x *pp2, int port_num)
{
	struct mv_port_link_status port_status;
	struct mv_pp2x_port *pp_port;
	struct mv_mac_data *mac;

	pp_port = mv_pp2x_port_struct_get(pp2, port_num);
	mac = &pp_port->mac_data;

	mv_gop110_port_link_status(gop, mac, &port_status);

	pr_info("-------------- Port %d configuration ----------------",
		port_num);

	switch (mac->phy_mode) {
	case PHY_INTERFACE_MODE_RGMII:
		pr_info("Port mode               : RGMII");
	break;
	case PHY_INTERFACE_MODE_SGMII:
		pr_info("Port mode               : SGMII");
	break;
	case PHY_INTERFACE_MODE_QSGMII:
		pr_info("Port mode               : QSGMII");
	break;
	case PHY_INTERFACE_MODE_XAUI:
		pr_info("Port mode               : XAUI");
	break;
	case PHY_INTERFACE_MODE_RXAUI:
		pr_info("Port mode               : RXAUI");
	break;
	case PHY_INTERFACE_MODE_KR:
		pr_info("Port mode               : KR");
	break;
	default:
		pr_err("%s: Wrong port mode (%d)", __func__, mac->phy_mode);
		return -1;
	}

	pr_info("\nLink status             : %s",
		(port_status.linkup) ? "link up" : "link down");
	pr_info("\n");

	if ((mac->phy_mode == PHY_INTERFACE_MODE_SGMII) &&
	    (mac->speed == 2500) &&
	    (port_status.speed == MV_PORT_SPEED_1000))
		port_status.speed = MV_PORT_SPEED_2500;

	switch (port_status.speed) {
	case MV_PORT_SPEED_AN:
		pr_info("Port speed              : AutoNeg");
	break;
	case MV_PORT_SPEED_10:
		pr_info("Port speed              : 10M");
	break;
	case MV_PORT_SPEED_100:
		pr_info("Port speed              : 100M");
	break;
	case MV_PORT_SPEED_1000:
		pr_info("Port speed              : 1G");
	break;
	case MV_PORT_SPEED_2500:
		pr_info("Port speed              : 2.5G");
	break;
	case MV_PORT_SPEED_10000:
		pr_info("Port speed              : 10G");
	break;
	default:
		pr_err("%s: Wrong port speed (%d)\n", __func__,
		       port_status.speed);
		return -1;
	}
	pr_info("\n");
	switch (port_status.duplex) {
	case MV_PORT_DUPLEX_AN:
		pr_info("Port duplex             : AutoNeg");
	break;
	case MV_PORT_DUPLEX_HALF:
		pr_info("Port duplex             : half");
	break;
	case MV_PORT_DUPLEX_FULL:
		pr_info("Port duplex             : full");
	break;
	default:
		pr_err("%s: Wrong port duplex (%d)", __func__,
		       port_status.duplex);
		return -1;
	}
	pr_info("\n");

	return 0;
}
EXPORT_SYMBOL(mv_gop110_status_show);

/* get port speed and duplex */
int mv_gop110_speed_duplex_get(struct gop_hw *gop, struct mv_mac_data *mac,
			       enum mv_port_speed *speed,
			       enum mv_port_duplex *duplex)
{
	int port_num = mac->gop_index;

	switch (mac->phy_mode) {
	case PHY_INTERFACE_MODE_RGMII:
	case PHY_INTERFACE_MODE_SGMII:
	case PHY_INTERFACE_MODE_QSGMII:
		mv_gop110_gmac_speed_duplex_get(gop, port_num, speed,
						duplex);
	break;
	case PHY_INTERFACE_MODE_XAUI:
	case PHY_INTERFACE_MODE_RXAUI:
	case PHY_INTERFACE_MODE_KR:
		mv_gop110_xlg_mac_speed_duplex_get(gop, port_num, speed,
						   duplex);
	break;
	default:
		pr_err("%s: Wrong port mode (%d)", __func__, mac->phy_mode);
		return -1;
	}
	return 0;
}

/* set port speed and duplex */
int mv_gop110_speed_duplex_set(struct gop_hw *gop, struct mv_mac_data *mac,
			       enum mv_port_speed speed,
			       enum mv_port_duplex duplex)
{
	int port_num = mac->gop_index;

	switch (mac->phy_mode) {
	case PHY_INTERFACE_MODE_RGMII:
	case PHY_INTERFACE_MODE_SGMII:
	case PHY_INTERFACE_MODE_QSGMII:
		mv_gop110_gmac_speed_duplex_set(gop, port_num, speed, duplex);
	break;
	case PHY_INTERFACE_MODE_XAUI:
	case PHY_INTERFACE_MODE_RXAUI:
	case PHY_INTERFACE_MODE_KR:
		mv_gop110_xlg_mac_speed_duplex_set(gop, port_num, speed,
						   duplex);
	break;
	default:
		pr_err("%s: Wrong port mode (%d)", __func__, mac->phy_mode);
		return -1;
	}
	return 0;
}

int mv_gop110_autoneg_restart(struct gop_hw *gop, struct mv_mac_data *mac)
{
	int port_num = mac->gop_index;

	switch (mac->phy_mode) {
	case PHY_INTERFACE_MODE_RGMII:
	break;
	case PHY_INTERFACE_MODE_SGMII:
	case PHY_INTERFACE_MODE_QSGMII:
		mv_gop110_gmac_port_autoneg_restart(gop, port_num);
	break;
	case PHY_INTERFACE_MODE_XAUI:
	case PHY_INTERFACE_MODE_RXAUI:
	case PHY_INTERFACE_MODE_KR:
		pr_err("%s: on supported for port mode (%d)", __func__,
		       mac->phy_mode);
		return -1;
	default:
		pr_err("%s: Wrong port mode (%d)", __func__, mac->phy_mode);
		return -1;
	}
	return 0;
}

int mv_gop110_fl_cfg(struct gop_hw *gop, struct mv_mac_data *mac)
{
	int port_num = mac->gop_index;

	switch (mac->phy_mode) {
	case PHY_INTERFACE_MODE_RGMII:
	case PHY_INTERFACE_MODE_SGMII:
	case PHY_INTERFACE_MODE_QSGMII:
		/* disable AN */
		if (mac->speed == 2500)
			mv_gop110_speed_duplex_set(gop, mac,
						   MV_PORT_SPEED_2500,
						   MV_PORT_DUPLEX_FULL);
		else
			mv_gop110_speed_duplex_set(gop, mac,
						   MV_PORT_SPEED_1000,
						   MV_PORT_DUPLEX_FULL);
		/* force link */
		mv_gop110_gmac_force_link_mode_set(gop, port_num, true, false);
	break;
	case PHY_INTERFACE_MODE_XAUI:
	case PHY_INTERFACE_MODE_RXAUI:
	case PHY_INTERFACE_MODE_KR:
		return 0;
	default:
		pr_err("%s: Wrong port mode (%d)", __func__, mac->phy_mode);
		return -1;
	}
	return 0;
}

/* set port ForceLinkUp and ForceLinkDown*/
int mv_gop110_force_link_mode_set(struct gop_hw *gop, struct mv_mac_data *mac,
				  bool force_link_up,
				  bool force_link_down)
{
	int port_num = mac->gop_index;

	switch (mac->phy_mode) {
	case PHY_INTERFACE_MODE_RGMII:
	case PHY_INTERFACE_MODE_SGMII:
	case PHY_INTERFACE_MODE_QSGMII:
		/* force link */
		mv_gop110_gmac_force_link_mode_set(gop, port_num,
						   force_link_up,
						   force_link_down);
	break;
	case PHY_INTERFACE_MODE_XAUI:
	case PHY_INTERFACE_MODE_RXAUI:
	case PHY_INTERFACE_MODE_KR:
		return 0;
	default:
		pr_err("%s: Wrong port mode (%d)", __func__, mac->phy_mode);
		return -1;
	}
	return 0;
}

/* get port ForceLinkUp and ForceLinkDown*/
int mv_gop110_force_link_mode_get(struct gop_hw *gop, struct mv_mac_data *mac,
				  bool *force_link_up,
				  bool *force_link_down)
{
	int port_num = mac->gop_index;

	switch (mac->phy_mode) {
	case PHY_INTERFACE_MODE_RGMII:
	case PHY_INTERFACE_MODE_SGMII:
	case PHY_INTERFACE_MODE_QSGMII:
		return mv_gop110_gmac_force_link_mode_get(gop, port_num,
							  force_link_up,
							  force_link_down);
	break;
	case PHY_INTERFACE_MODE_XAUI:
	case PHY_INTERFACE_MODE_RXAUI:
	case PHY_INTERFACE_MODE_KR:
		return 0;
	default:
		pr_err("%s: Wrong port mode (%d)", __func__, mac->phy_mode);
		return -1;
	}
	return 0;
}

/* set port internal loopback*/
int mv_gop110_loopback_set(struct gop_hw *gop, struct mv_mac_data *mac,
			   bool lb)
{
	int port_num = mac->gop_index;
	enum mv_lb_type type;

	switch (mac->phy_mode) {
	case PHY_INTERFACE_MODE_RGMII:
	case PHY_INTERFACE_MODE_SGMII:
	case PHY_INTERFACE_MODE_QSGMII:
		/* set loopback */
		if (lb)
			type = MV_TX_2_RX_LB;
		else
			type = MV_DISABLE_LB;

		mv_gop110_gmac_loopback_cfg(gop, port_num, type);
	break;
	case PHY_INTERFACE_MODE_XAUI:
	case PHY_INTERFACE_MODE_RXAUI:
	case PHY_INTERFACE_MODE_KR:
		return 0;
	default:
		pr_err("%s: Wrong port mode (%d)", __func__, mac->phy_mode);
		return -1;
	}
	return 0;
}

/**************************************************************************
* mv_gop110_gpcs_mode_cfg
*
* DESCRIPTION:
	Configure port to working with Gig PCS or don't.
*
* INPUTS:
*       pcs_num   - physical PCS number
*       en        - true to enable PCS
*
* OUTPUTS:
*       None.
*
* RETURNS:
*       0  - on success
*       1  - on error
*
**************************************************************************/
int mv_gop110_gpcs_mode_cfg(struct gop_hw *gop, int pcs_num, bool en)
{
	u32 val;

	val = mv_gop110_gmac_read(gop, pcs_num, MV_GMAC_PORT_CTRL2_REG);

	if (en)
		val |= MV_GMAC_PORT_CTRL2_PCS_EN_MASK;
	else
		val &= ~MV_GMAC_PORT_CTRL2_PCS_EN_MASK;

	/* enable / disable PCS on this port */
	mv_gop110_gmac_write(gop, pcs_num, MV_GMAC_PORT_CTRL2_REG, val);

	return 0;
}

/**************************************************************************
* mv_gop110_gpcs_reset
*
* DESCRIPTION:
*       Set the selected PCS number to reset or exit from reset.
*
* INPUTS:
*       pcs_num    - physical PCS number
*       action    - reset / unreset
*
* OUTPUTS:
*       None.
*
* RETURNS:
*       0  - on success
*       1  - on error
*
*************************************************************************/
int  mv_gop110_gpcs_reset(struct gop_hw *gop, int pcs_num, enum mv_reset act)
{
	u32 reg_data;

	reg_data = mv_gop110_gmac_read(gop, pcs_num, MV_GMAC_PORT_CTRL2_REG);
	if (act == RESET)
		U32_SET_FIELD(reg_data, MV_GMAC_PORT_CTRL2_SGMII_MODE_MASK, 0);
	else
		U32_SET_FIELD(reg_data, MV_GMAC_PORT_CTRL2_SGMII_MODE_MASK,
			      1 << MV_GMAC_PORT_CTRL2_SGMII_MODE_OFFS);

	mv_gop110_gmac_write(gop, pcs_num, MV_GMAC_PORT_CTRL2_REG, reg_data);
	return 0;
}

/* print value of unit registers */
void mv_gop110_serdes_lane_regs_dump(struct gop_hw *gop, int lane)
{
	pr_info("\nSerdes Lane #%d registers]\n", lane);
	mv_gop110_serdes_print(gop, "MV_SERDES_CFG_0_REG", lane,
			       MV_SERDES_CFG_0_REG);
	mv_gop110_serdes_print(gop, "MV_SERDES_CFG_1_REG", lane,
			       MV_SERDES_CFG_1_REG);
	mv_gop110_serdes_print(gop, "MV_SERDES_CFG_2_REG", lane,
			       MV_SERDES_CFG_2_REG);
	mv_gop110_serdes_print(gop, "MV_SERDES_CFG_3_REG", lane,
			       MV_SERDES_CFG_3_REG);
	mv_gop110_serdes_print(gop, "MV_SERDES_MISC_REG", lane,
			       MV_SERDES_MISC_REG);
}
EXPORT_SYMBOL(mv_gop110_serdes_lane_regs_dump);

void mv_gop110_serdes_init(struct gop_hw *gop, int lane,
			   enum sd_media_mode mode)
{
	u32 reg_val;

	/* Media Interface Mode */
	reg_val = mv_gop110_serdes_read(gop, lane, MV_SERDES_CFG_0_REG);
	if (mode == MV_RXAUI)
		reg_val |= MV_SERDES_CFG_0_MEDIA_MODE_MASK;
	else
		reg_val &= ~MV_SERDES_CFG_0_MEDIA_MODE_MASK;

	/* Pull-Up PLL to StandAlone mode */
	reg_val |= MV_SERDES_CFG_0_PU_PLL_MASK;
	/* powers up the SD Rx/Tx PLL */
	reg_val |= MV_SERDES_CFG_0_RX_PLL_MASK;
	reg_val |= MV_SERDES_CFG_0_TX_PLL_MASK;
	mv_gop110_serdes_write(gop, lane, MV_SERDES_CFG_0_REG, reg_val);

	mv_gop110_serdes_reset(gop, lane, false, false, false);

	reg_val = 0x17f;
	mv_gop110_serdes_write(gop, lane, MV_SERDES_MISC_REG, reg_val);
}

void mv_gop110_serdes_reset(struct gop_hw *gop, int lane, bool analog_reset,
			    bool core_reset, bool digital_reset)
{
	u32 reg_val;

	reg_val = mv_gop110_serdes_read(gop, lane, MV_SERDES_CFG_1_REG);
	if (analog_reset)
		reg_val &= ~MV_SERDES_CFG_1_ANALOG_RESET_MASK;
	else
		reg_val |= MV_SERDES_CFG_1_ANALOG_RESET_MASK;

	if (core_reset)
		reg_val &= ~MV_SERDES_CFG_1_CORE_RESET_MASK;
	else
		reg_val |= MV_SERDES_CFG_1_CORE_RESET_MASK;

	if (digital_reset)
		reg_val &= ~MV_SERDES_CFG_1_DIGITAL_RESET_MASK;
	else
		reg_val |= MV_SERDES_CFG_1_DIGITAL_RESET_MASK;

	mv_gop110_serdes_write(gop, lane, MV_SERDES_CFG_1_REG, reg_val);
}

/**************************************************************************
* mv_gop110_smi_init
**************************************************************************/
int mv_gop110_smi_init(struct gop_hw *gop)
{
	u32 val;

	/* not invert MDC */
	val = mv_gop110_smi_read(gop, MV_SMI_MISC_CFG_REG);
	val &= ~MV_SMI_MISC_CFG_INVERT_MDC_MASK;
	mv_gop110_smi_write(gop, MV_SMI_MISC_CFG_REG, val);

	return 0;
}

/**************************************************************************
* mv_gop_phy_addr_cfg
**************************************************************************/
int mv_gop110_smi_phy_addr_cfg(struct gop_hw *gop, int port, int addr)
{
	mv_gop110_smi_write(gop, MV_SMI_PHY_ADDRESS_REG(port), addr);

	return 0;
}

/* print value of unit registers */
void mv_gop110_xlg_mac_regs_dump(struct gop_hw *gop, int port)
{
	int timer;
	char reg_name[16];

	mv_gop110_xlg_mac_print(gop, "PORT_MAC_CTRL0", port,
				MV_XLG_PORT_MAC_CTRL0_REG);
	mv_gop110_xlg_mac_print(gop, "PORT_MAC_CTRL1", port,
				MV_XLG_PORT_MAC_CTRL1_REG);
	mv_gop110_xlg_mac_print(gop, "PORT_MAC_CTRL2", port,
				MV_XLG_PORT_MAC_CTRL2_REG);
	mv_gop110_xlg_mac_print(gop, "PORT_STATUS", port,
				MV_XLG_MAC_PORT_STATUS_REG);
	mv_gop110_xlg_mac_print(gop, "PORT_FIFOS_THRS_CFG", port,
				MV_XLG_PORT_FIFOS_THRS_CFG_REG);
	mv_gop110_xlg_mac_print(gop, "PORT_MAC_CTRL3", port,
				MV_XLG_PORT_MAC_CTRL3_REG);
	mv_gop110_xlg_mac_print(gop, "PORT_PER_PRIO_FLOW_CTRL_STATUS", port,
				MV_XLG_PORT_PER_PRIO_FLOW_CTRL_STATUS_REG);
	mv_gop110_xlg_mac_print(gop, "DEBUG_BUS_STATUS", port,
				MV_XLG_DEBUG_BUS_STATUS_REG);
	mv_gop110_xlg_mac_print(gop, "PORT_METAL_FIX", port,
				MV_XLG_PORT_METAL_FIX_REG);
	mv_gop110_xlg_mac_print(gop, "XG_MIB_CNTRS_CTRL", port,
				MV_XLG_MIB_CNTRS_CTRL_REG);
	for (timer = 0; timer < 8; timer++) {
		sprintf(reg_name, "CNCCFC_TIMER%d", timer);
		mv_gop110_xlg_mac_print(gop, reg_name, port,
					MV_XLG_CNCCFC_TIMERI_REG(timer));
	}
	mv_gop110_xlg_mac_print(gop, "PPFC_CTRL", port,
				MV_XLG_MAC_PPFC_CTRL_REG);
	mv_gop110_xlg_mac_print(gop, "FC_DSA_TAG_0", port,
				MV_XLG_MAC_FC_DSA_TAG_0_REG);
	mv_gop110_xlg_mac_print(gop, "FC_DSA_TAG_1", port,
				MV_XLG_MAC_FC_DSA_TAG_1_REG);
	mv_gop110_xlg_mac_print(gop, "FC_DSA_TAG_2", port,
				MV_XLG_MAC_FC_DSA_TAG_2_REG);
	mv_gop110_xlg_mac_print(gop, "FC_DSA_TAG_3", port,
				MV_XLG_MAC_FC_DSA_TAG_3_REG);
	mv_gop110_xlg_mac_print(gop, "DIC_BUDGET_COMPENSATION", port,
				MV_XLG_MAC_DIC_BUDGET_COMPENSATION_REG);
	mv_gop110_xlg_mac_print(gop, "PORT_MAC_CTRL4", port,
				MV_XLG_PORT_MAC_CTRL4_REG);
	mv_gop110_xlg_mac_print(gop, "PORT_MAC_CTRL5", port,
				MV_XLG_PORT_MAC_CTRL5_REG);
	mv_gop110_xlg_mac_print(gop, "EXT_CTRL", port,
				MV_XLG_MAC_EXT_CTRL_REG);
	mv_gop110_xlg_mac_print(gop, "MACRO_CTRL", port,
				MV_XLG_MAC_MACRO_CTRL_REG);
	mv_gop110_xlg_mac_print(gop, "MACRO_CTRL", port,
				MV_XLG_MAC_MACRO_CTRL_REG);
	mv_gop110_xlg_mac_print(gop, "PORT_INT_MASK", port,
				MV_XLG_INTERRUPT_MASK_REG);
	mv_gop110_xlg_mac_print(gop, "EXTERNAL_INT_MASK", port,
				MV_XLG_EXTERNAL_INTERRUPT_MASK_REG);
}
EXPORT_SYMBOL(mv_gop110_xlg_mac_regs_dump);

/* Set the MAC to reset or exit from reset */
int mv_gop110_xlg_mac_reset(struct gop_hw *gop, int mac_num,
			    enum mv_reset reset)
{
	u32 reg_addr;
	u32 val;

	reg_addr = MV_XLG_PORT_MAC_CTRL0_REG;

	/* read - modify - write */
	val = mv_gop110_xlg_mac_read(gop, mac_num, reg_addr);
	if (reset == RESET)
		val &= ~MV_XLG_MAC_CTRL0_MACRESETN_MASK;
	else
		val |= MV_XLG_MAC_CTRL0_MACRESETN_MASK;
	mv_gop110_xlg_mac_write(gop, mac_num, reg_addr, val);

	return 0;
}

/* Set the internal mux's to the required MAC in the GOP */
int mv_gop110_xlg_mac_mode_cfg(struct gop_hw *gop, int mac_num,
			       int num_of_act_lanes)
{
	u32 reg_addr;
	u32 val;

	/* configure 10G MAC mode */
	reg_addr = MV_XLG_PORT_MAC_CTRL0_REG;
	val = mv_gop110_xlg_mac_read(gop, mac_num, reg_addr);
	U32_SET_FIELD(val, MV_XLG_MAC_CTRL0_RXFCEN_MASK,
		      (1 << MV_XLG_MAC_CTRL0_RXFCEN_OFFS));
	mv_gop110_xlg_mac_write(gop, mac_num, reg_addr, val);

	reg_addr = MV_XLG_PORT_MAC_CTRL3_REG;
	val = mv_gop110_xlg_mac_read(gop, mac_num, reg_addr);
	U32_SET_FIELD(val, MV_XLG_MAC_CTRL3_MACMODESELECT_MASK,
		      (1 << MV_XLG_MAC_CTRL3_MACMODESELECT_OFFS));
	mv_gop110_xlg_mac_write(gop, mac_num, reg_addr, val);

	reg_addr = MV_XLG_PORT_MAC_CTRL4_REG;

	/* read - modify - write */
	val = mv_gop110_xlg_mac_read(gop, mac_num, reg_addr);
	U32_SET_FIELD(val, MV_XLG_MAC_CTRL4_MAC_MODE_DMA_1G_MASK, 0 <<
					MV_XLG_MAC_CTRL4_MAC_MODE_DMA_1G_OFFS);
	U32_SET_FIELD(val, MV_XLG_MAC_CTRL4_FORWARD_PFC_EN_MASK, 1 <<
					MV_XLG_MAC_CTRL4_FORWARD_PFC_EN_OFFS);
	U32_SET_FIELD(val, MV_XLG_MAC_CTRL4_FORWARD_802_3X_FC_EN_MASK, 1 <<
				MV_XLG_MAC_CTRL4_FORWARD_802_3X_FC_EN_OFFS);
	mv_gop110_xlg_mac_write(gop, mac_num, reg_addr, val);

	/* Jumbo frame support - 0x1400*2= 0x2800 bytes */
	val = mv_gop110_xlg_mac_read(gop, mac_num, MV_XLG_PORT_MAC_CTRL1_REG);
	U32_SET_FIELD(val, MV_XLG_MAC_CTRL1_FRAMESIZELIMIT_MASK, 0x1400);
	mv_gop110_xlg_mac_write(gop, mac_num, MV_XLG_PORT_MAC_CTRL1_REG, val);

	/* mask all port interrupts */
	mv_gop110_xlg_port_link_event_mask(gop, mac_num);

	/* unmask link change interrupt */
	val = mv_gop110_xlg_mac_read(gop, mac_num, MV_XLG_INTERRUPT_MASK_REG);
	val |= MV_XLG_INTERRUPT_LINK_CHANGE_MASK;
	val |= 1; /* unmask summary bit */
	mv_gop110_xlg_mac_write(gop, mac_num, MV_XLG_INTERRUPT_MASK_REG, val);

	return 0;
}

/* Configure MAC loopback */
int mv_gop110_xlg_mac_loopback_cfg(struct gop_hw *gop, int mac_num,
				   enum mv_lb_type type)
{
	u32 reg_addr;
	u32 val;

	reg_addr = MV_XLG_PORT_MAC_CTRL1_REG;
	val = mv_gop110_xlg_mac_read(gop, mac_num, reg_addr);
	switch (type) {
	case MV_DISABLE_LB:
		val &= ~MV_XLG_MAC_CTRL1_MACLOOPBACKEN_MASK;
		val &= ~MV_XLG_MAC_CTRL1_XGMIILOOPBACKEN_MASK;
		break;
	case MV_RX_2_TX_LB:
		val &= ~MV_XLG_MAC_CTRL1_MACLOOPBACKEN_MASK;
		val |= MV_XLG_MAC_CTRL1_XGMIILOOPBACKEN_MASK;
		break;
	case MV_TX_2_RX_LB:
		val |= MV_XLG_MAC_CTRL1_MACLOOPBACKEN_MASK;
		val |= MV_XLG_MAC_CTRL1_XGMIILOOPBACKEN_MASK;
		break;
	default:
		return -1;
	}
	mv_gop110_xlg_mac_write(gop, mac_num, reg_addr, val);
	return 0;
}

/* Get MAC link status */
bool mv_gop110_xlg_mac_link_status_get(struct gop_hw *gop, int mac_num)
{
	if (mv_gop110_xlg_mac_read(gop, mac_num,
				   MV_XLG_MAC_PORT_STATUS_REG) & 1)
		return true;

	return false;
}

/* Enable port and MIB counters update */
void mv_gop110_xlg_mac_port_enable(struct gop_hw *gop, int mac_num)
{
	u32 reg_val;

	reg_val = mv_gop110_xlg_mac_read(gop, mac_num,
					 MV_XLG_PORT_MAC_CTRL0_REG);
	reg_val |= MV_XLG_MAC_CTRL0_PORTEN_MASK;
	reg_val &= ~MV_XLG_MAC_CTRL0_MIBCNTDIS_MASK;

	mv_gop110_xlg_mac_write(gop, mac_num,
				MV_XLG_PORT_MAC_CTRL0_REG, reg_val);
}

/* Disable port */
void mv_gop110_xlg_mac_port_disable(struct gop_hw *gop, int mac_num)
{
	u32 reg_val;

	/* mask all port interrupts */
	mv_gop110_xlg_port_link_event_mask(gop, mac_num);

	reg_val = mv_gop110_xlg_mac_read(gop, mac_num,
					 MV_XLG_PORT_MAC_CTRL0_REG);
	reg_val &= ~MV_XLG_MAC_CTRL0_PORTEN_MASK;

	mv_gop110_xlg_mac_write(gop, mac_num,
				MV_XLG_PORT_MAC_CTRL0_REG, reg_val);
}

void mv_gop110_xlg_mac_port_periodic_xon_set(struct gop_hw *gop,
					     int mac_num,
					     int enable)
{
	u32 reg_val;

	reg_val =  mv_gop110_xlg_mac_read(gop, mac_num,
					  MV_XLG_PORT_MAC_CTRL0_REG);

	if (enable)
		reg_val |= MV_XLG_MAC_CTRL0_PERIODICXONEN_MASK;
	else
		reg_val &= ~MV_XLG_MAC_CTRL0_PERIODICXONEN_MASK;

	mv_gop110_xlg_mac_write(gop, mac_num,
				MV_XLG_PORT_MAC_CTRL0_REG, reg_val);
}

int mv_gop110_xlg_mac_link_status(struct gop_hw *gop,
				  int mac_num,
				  struct mv_port_link_status *pstatus)
{
	u32 reg_val;
	u32 mac_mode;
	u32 fc_en;

	reg_val = mv_gop110_xlg_mac_read(gop, mac_num,
					 MV_XLG_PORT_MAC_CTRL3_REG);
	mac_mode = (reg_val & MV_XLG_MAC_CTRL3_MACMODESELECT_MASK) >>
		    MV_XLG_MAC_CTRL3_MACMODESELECT_OFFS;

	/* speed  and duplex */
	switch (mac_mode) {
	case 0:
		pstatus->speed = MV_PORT_SPEED_1000;
		pstatus->duplex = MV_PORT_DUPLEX_AN;
		break;
	case 1:
		pstatus->speed = MV_PORT_SPEED_10000;
		pstatus->duplex = MV_PORT_DUPLEX_FULL;
		break;
	default:
		return -1;
	}

	/* link status */
	reg_val = mv_gop110_xlg_mac_read(gop, mac_num,
					 MV_XLG_MAC_PORT_STATUS_REG);
	if (reg_val & MV_XLG_MAC_PORT_STATUS_LINKSTATUS_MASK)
		pstatus->linkup = 1 /*TRUE*/;
	else
		pstatus->linkup = 0 /*FALSE*/;

	/* flow control status */
	fc_en = mv_gop110_xlg_mac_read(gop, mac_num,
				       MV_XLG_PORT_MAC_CTRL0_REG);
	if (reg_val & MV_XLG_MAC_PORT_STATUS_PORTTXPAUSE_MASK)
		pstatus->tx_fc = MV_PORT_FC_ACTIVE;
	else if (fc_en & MV_XLG_MAC_CTRL0_TXFCEN_MASK)
		pstatus->tx_fc = MV_PORT_FC_ENABLE;
	else
		pstatus->tx_fc = MV_PORT_FC_DISABLE;

	if (reg_val & MV_XLG_MAC_PORT_STATUS_PORTRXPAUSE_MASK)
		pstatus->rx_fc = MV_PORT_FC_ACTIVE;
	else if (fc_en & MV_XLG_MAC_CTRL0_RXFCEN_MASK)
		pstatus->rx_fc = MV_PORT_FC_ENABLE;
	else
		pstatus->rx_fc = MV_PORT_FC_DISABLE;

	return 0;
}

/* Change maximum receive size of the port */
int mv_gop110_xlg_mac_max_rx_size_set(struct gop_hw *gop, int mac_num,
				      int max_rx_size)
{
	u32	reg_val;

	reg_val =  mv_gop110_xlg_mac_read(gop, mac_num,
					  MV_XLG_PORT_MAC_CTRL1_REG);
	reg_val &= ~MV_XLG_MAC_CTRL1_FRAMESIZELIMIT_MASK;
	reg_val |= (((max_rx_size - MVPP2_MH_SIZE) / 2) <<
		    MV_XLG_MAC_CTRL1_FRAMESIZELIMIT_OFFS);
	mv_gop110_xlg_mac_write(gop, mac_num,
				MV_XLG_PORT_MAC_CTRL1_REG, reg_val);

	return 0;
}

/* Sets "Force Link Pass" and "Do Not Force Link Fail" bits.
*  This function should only be called when the port is disabled.
* INPUT:
*	int  port		- port number
*	bool force_link_pass	- Force Link Pass
*	bool force_link_fail - Force Link Failure
*		0, 0 - normal state: detect link via PHY and connector
*		1, 1 - prohibited state.
*/
int mv_gop110_xlg_mac_force_link_mode_set(struct gop_hw *gop, int mac_num,
					  bool force_link_up,
					  bool force_link_down)
{
	u32 reg_val;

	/* Can't force link pass and link fail at the same time */
	if ((force_link_up) && (force_link_down))
		return -EINVAL;

	reg_val = mv_gop110_xlg_mac_read(gop, mac_num,
					 MV_XLG_PORT_MAC_CTRL0_REG);

	if (force_link_up)
		reg_val |= MV_XLG_MAC_CTRL0_FORCELINKPASS_MASK;
	else
		reg_val &= ~MV_XLG_MAC_CTRL0_FORCELINKPASS_MASK;

	if (force_link_down)
		reg_val |= MV_XLG_MAC_CTRL0_FORCELINKDOWN_MASK;
	else
		reg_val &= ~MV_XLG_MAC_CTRL0_FORCELINKDOWN_MASK;

	mv_gop110_xlg_mac_write(gop, mac_num,
				MV_XLG_PORT_MAC_CTRL0_REG, reg_val);

	return 0;
}

/* Sets port speed to Auto Negotiation / 1000 / 100 / 10 Mbps.
*  Sets port duplex to Auto Negotiation / Full / Half Duplex.
*/
int mv_gop110_xlg_mac_speed_duplex_set(struct gop_hw *gop, int mac_num,
				       enum mv_port_speed speed,
				       enum mv_port_duplex duplex)
{
	/* not supported */
	return -1;
}

/* Gets port speed and duplex */
int mv_gop110_xlg_mac_speed_duplex_get(struct gop_hw *gop, int mac_num,
				       enum mv_port_speed *speed,
				       enum mv_port_duplex *duplex)
{
	/* not supported */
	return -1;
}

/* Configure the port's Flow Control properties */
int mv_gop110_xlg_mac_fc_set(struct gop_hw *gop, int mac_num,
			     enum mv_port_fc fc)
{
	u32 reg_val;

	reg_val = mv_gop110_xlg_mac_read(gop, mac_num,
					 MV_XLG_PORT_MAC_CTRL0_REG);

	switch (fc) {
	case MV_PORT_FC_DISABLE:
		reg_val &= ~MV_XLG_MAC_CTRL0_RXFCEN_MASK;
		reg_val &= ~MV_XLG_MAC_CTRL0_TXFCEN_MASK;
		break;

	case MV_PORT_FC_ENABLE:
		reg_val |= MV_XLG_MAC_CTRL0_RXFCEN_MASK;
		reg_val |= MV_XLG_MAC_CTRL0_TXFCEN_MASK;
		break;

	case MV_PORT_FC_AN_NO:
	case MV_PORT_FC_AN_SYM:
	case MV_PORT_FC_AN_ASYM:
	default:
		pr_err("XLG MAC: Unexpected FlowControl value %d\n", fc);
		return -EINVAL;
	}

	mv_gop110_xlg_mac_write(gop, mac_num,
				MV_XLG_PORT_MAC_CTRL0_REG, reg_val);
	return 0;
}

/* Get Flow Control configuration of the port */
void mv_gop110_xlg_mac_fc_get(struct gop_hw *gop, int mac_num,
			      enum mv_port_fc *fc)
{
	u32 reg_val;

	/* No auto negotiation for flow control */
	reg_val = mv_gop110_xlg_mac_read(gop, mac_num,
					 MV_XLG_PORT_MAC_CTRL0_REG);

	if ((reg_val & MV_XLG_MAC_CTRL0_RXFCEN_MASK) &&
	    (reg_val & MV_XLG_MAC_CTRL0_TXFCEN_MASK))
		*fc = MV_PORT_FC_ENABLE;
	else
		*fc = MV_PORT_FC_DISABLE;
}

int mv_gop110_xlg_mac_port_link_speed_fc(struct gop_hw *gop, int mac_num,
					 enum mv_port_speed speed,
					 int force_link_up)
{
	if (force_link_up) {
		if (mv_gop110_xlg_mac_fc_set(gop, mac_num,
					     MV_PORT_FC_ENABLE)) {
			pr_err("mv_gop110_xlg_mac_fc_set failed\n");
			return -EPERM;
		}
		if (mv_gop110_xlg_mac_force_link_mode_set(gop, mac_num,
							  1, 0)) {
			pr_err(
				"mv_gop110_xlg_mac_force_link_mode_set failed\n");
			return -EPERM;
		}
	} else {
		if (mv_gop110_xlg_mac_force_link_mode_set(gop, mac_num,
							  0, 0)) {
			pr_err(
				"mv_gop110_xlg_mac_force_link_mode_set failed\n");
			return -EPERM;
		}
		if (mv_gop110_xlg_mac_fc_set(gop, mac_num,
					     MV_PORT_FC_AN_SYM)) {
			pr_err("mv_gop110_xlg_mac_fc_set failed\n");
			return -EPERM;
		}
	}

	return 0;
}

void mv_gop110_xlg_port_link_event_mask(struct gop_hw *gop, int mac_num)
{
	u32 reg_val;

	reg_val = mv_gop110_xlg_mac_read(gop, mac_num,
					 MV_XLG_EXTERNAL_INTERRUPT_MASK_REG);
	reg_val &= ~(1 << 1);
	mv_gop110_xlg_mac_write(gop, mac_num,
				MV_XLG_EXTERNAL_INTERRUPT_MASK_REG, reg_val);
}

void mv_gop110_xlg_port_external_event_unmask(struct gop_hw *gop, int mac_num,
					      int bit_2_open)
{
	u32 reg_val;

	reg_val = mv_gop110_xlg_mac_read(gop, mac_num,
					 MV_XLG_EXTERNAL_INTERRUPT_MASK_REG);
	reg_val |= (1 << bit_2_open);
	reg_val |= 1; /* unmask summary bit */
	mv_gop110_xlg_mac_write(gop, mac_num,
				MV_XLG_EXTERNAL_INTERRUPT_MASK_REG, reg_val);
}

void mv_gop110_xlg_port_link_event_clear(struct gop_hw *gop, int mac_num)
{
	u32 reg_val;

	reg_val = mv_gop110_xlg_mac_read(gop, mac_num,
					 MV_XLG_INTERRUPT_CAUSE_REG);
}

void mv_gop110_xlg_2_gig_mac_cfg(struct gop_hw *gop, int mac_num)
{
	u32 reg_val;

	/* relevant only for MAC0 (XLG0 and GMAC0) */
	if (mac_num > 0)
		return;

	/* configure 1Gig MAC mode */
	reg_val = mv_gop110_xlg_mac_read(gop, mac_num,
					 MV_XLG_PORT_MAC_CTRL3_REG);
	U32_SET_FIELD(reg_val, MV_XLG_MAC_CTRL3_MACMODESELECT_MASK,
		      (0 << MV_XLG_MAC_CTRL3_MACMODESELECT_OFFS));
	mv_gop110_xlg_mac_write(gop, mac_num,
				MV_XLG_PORT_MAC_CTRL3_REG, reg_val);
}

/* print value of unit registers */
void mv_gop110_xpcs_gl_regs_dump(struct gop_hw *gop)
{
	pr_info("\nXPCS Global registers]\n");
	mv_gop110_xpcs_global_print(gop, "GLOBAL_CFG_0",
				    MV_XPCS_GLOBAL_CFG_0_REG);
	mv_gop110_xpcs_global_print(gop, "GLOBAL_CFG_1",
				    MV_XPCS_GLOBAL_CFG_1_REG);
	mv_gop110_xpcs_global_print(gop, "GLOBAL_FIFO_THR_CFG",
				    MV_XPCS_GLOBAL_FIFO_THR_CFG_REG);
	mv_gop110_xpcs_global_print(gop, "GLOBAL_MAX_IDLE_CNTR",
				    MV_XPCS_GLOBAL_MAX_IDLE_CNTR_REG);
	mv_gop110_xpcs_global_print(gop, "GLOBAL_STATUS",
				    MV_XPCS_GLOBAL_STATUS_REG);
	mv_gop110_xpcs_global_print(gop, "GLOBAL_DESKEW_ERR_CNTR",
				    MV_XPCS_GLOBAL_DESKEW_ERR_CNTR_REG);
	mv_gop110_xpcs_global_print(gop, "TX_PCKTS_CNTR_LSB",
				    MV_XPCS_TX_PCKTS_CNTR_LSB_REG);
	mv_gop110_xpcs_global_print(gop, "TX_PCKTS_CNTR_MSB",
				    MV_XPCS_TX_PCKTS_CNTR_MSB_REG);
}
EXPORT_SYMBOL(mv_gop110_xpcs_gl_regs_dump);

/* print value of unit registers */
void mv_gop110_xpcs_lane_regs_dump(struct gop_hw *gop, int lane)
{
	pr_info("\nXPCS Lane #%d registers]\n", lane);
	mv_gop110_xpcs_lane_print(gop, "LANE_CFG_0", lane,
				  MV_XPCS_LANE_CFG_0_REG);
	mv_gop110_xpcs_lane_print(gop, "LANE_CFG_1", lane,
				  MV_XPCS_LANE_CFG_1_REG);
	mv_gop110_xpcs_lane_print(gop, "LANE_STATUS", lane,
				  MV_XPCS_LANE_STATUS_REG);
	mv_gop110_xpcs_lane_print(gop, "SYMBOL_ERR_CNTR", lane,
				  MV_XPCS_SYMBOL_ERR_CNTR_REG);
	mv_gop110_xpcs_lane_print(gop, "DISPARITY_ERR_CNTR", lane,
				  MV_XPCS_DISPARITY_ERR_CNTR_REG);
	mv_gop110_xpcs_lane_print(gop, "PRBS_ERR_CNTR", lane,
				  MV_XPCS_PRBS_ERR_CNTR_REG);
	mv_gop110_xpcs_lane_print(gop, "RX_PCKTS_CNTR_LSB", lane,
				  MV_XPCS_RX_PCKTS_CNTR_LSB_REG);
	mv_gop110_xpcs_lane_print(gop, "RX_PCKTS_CNTR_MSB", lane,
				  MV_XPCS_RX_PCKTS_CNTR_MSB_REG);
	mv_gop110_xpcs_lane_print(gop, "RX_BAD_PCKTS_CNTR_LSB", lane,
				  MV_XPCS_RX_BAD_PCKTS_CNTR_LSB_REG);
	mv_gop110_xpcs_lane_print(gop, "RX_BAD_PCKTS_CNTR_MSB", lane,
				  MV_XPCS_RX_BAD_PCKTS_CNTR_MSB_REG);
	mv_gop110_xpcs_lane_print(gop, "CYCLIC_DATA_0", lane,
				  MV_XPCS_CYCLIC_DATA_0_REG);
	mv_gop110_xpcs_lane_print(gop, "CYCLIC_DATA_1", lane,
				  MV_XPCS_CYCLIC_DATA_1_REG);
	mv_gop110_xpcs_lane_print(gop, "CYCLIC_DATA_2", lane,
				  MV_XPCS_CYCLIC_DATA_2_REG);
	mv_gop110_xpcs_lane_print(gop, "CYCLIC_DATA_3", lane,
				  MV_XPCS_CYCLIC_DATA_3_REG);
}
EXPORT_SYMBOL(mv_gop110_xpcs_lane_regs_dump);

/* Set PCS to reset or exit from reset */
int mv_gop110_xpcs_reset(struct gop_hw *gop, enum mv_reset reset)
{
	u32 reg_addr;
	u32 val;

	reg_addr = MV_XPCS_GLOBAL_CFG_0_REG;

	/* read - modify - write */
	val = mv_gop110_xpcs_global_read(gop, reg_addr);
	if (reset == RESET)
		val &= ~MV_XPCS_GLOBAL_CFG_0_PCSRESET_MASK;
	else
		val |= MV_XPCS_GLOBAL_CFG_0_PCSRESET_MASK;
	mv_gop110_xpcs_global_write(gop, reg_addr, val);

	return 0;
}

/* Set the internal mux's to the required PCS in the PI */
int mv_gop110_xpcs_mode(struct gop_hw *gop, int num_of_lanes)
{
	u32 reg_addr;
	u32 val;
	int lane;

	switch (num_of_lanes) {
	case 1:
		lane = 0;
	break;
	case 2:
		lane = 1;
	break;
	case 4:
		lane = 2;
	break;
	default:
		return -1;
	}

	/* configure XG MAC mode */
	reg_addr = MV_XPCS_GLOBAL_CFG_0_REG;
	val = mv_gop110_xpcs_global_read(gop, reg_addr);
	val &= ~MV_XPCS_GLOBAL_CFG_0_PCSMODE_MASK;
	U32_SET_FIELD(val, MV_XPCS_GLOBAL_CFG_0_PCSMODE_MASK, 0);
	U32_SET_FIELD(val, MV_XPCS_GLOBAL_CFG_0_LANEACTIVE_MASK, (2 * lane) <<
			MV_XPCS_GLOBAL_CFG_0_LANEACTIVE_OFFS);
	mv_gop110_xpcs_global_write(gop, reg_addr, val);

	return 0;
}

int mv_gop110_mpcs_mode(struct gop_hw *gop)
{
	u32 reg_addr;
	u32 val;

	/* configure PCS40G COMMON CONTROL */
	reg_addr = PCS40G_COMMON_CONTROL;
	val = mv_gop110_mpcs_global_read(gop, reg_addr);
	U32_SET_FIELD(val, FORWARD_ERROR_CORRECTION_MASK,
		      0 << FORWARD_ERROR_CORRECTION_OFFSET);

	mv_gop110_mpcs_global_write(gop, reg_addr, val);

	/* configure PCS CLOCK RESET */
	reg_addr = PCS_CLOCK_RESET;
	val = mv_gop110_mpcs_global_read(gop, reg_addr);
	U32_SET_FIELD(val, CLK_DIVISION_RATIO_MASK,
			1 << CLK_DIVISION_RATIO_OFFSET);

	mv_gop110_mpcs_global_write(gop, reg_addr, val);

	U32_SET_FIELD(val, CLK_DIV_PHASE_SET_MASK,
			0 << CLK_DIV_PHASE_SET_OFFSET);
	U32_SET_FIELD(val, MAC_CLK_RESET_MASK, 1 << MAC_CLK_RESET_OFFSET);
	U32_SET_FIELD(val, RX_SD_CLK_RESET_MASK, 1 << RX_SD_CLK_RESET_OFFSET);
	U32_SET_FIELD(val, TX_SD_CLK_RESET_MASK, 1 << TX_SD_CLK_RESET_OFFSET);

	mv_gop110_mpcs_global_write(gop, reg_addr, val);

	return 0;
}

u64 mv_gop110_mib_read64(struct gop_hw *gop, int port, unsigned int offset)
{
	u64 val, val2;

	val = mv_gop110_xmib_mac_read(gop, port, offset);
	if (offset == MV_MIB_GOOD_OCTETS_RECEIVED_LOW ||
	    offset == MV_MIB_GOOD_OCTETS_SENT_LOW) {
		val2 = mv_gop110_xmib_mac_read(gop, port, offset + 4);
		val += (val2 << 32);
	}

	return val;
}

static void mv_gop110_mib_print(struct gop_hw *gop, int port, u32 offset,
				char *mib_name)
{
	u64 val;

	val = mv_gop110_mib_read64(gop, port, offset);
	pr_info("  %-32s: 0x%02x = %lld\n", mib_name, offset, val);
}

void mv_gop110_mib_counters_show(struct gop_hw *gop, int port)
{
	pr_info("\n[Rx]\n");
	mv_gop110_mib_print(gop, port, MV_MIB_GOOD_OCTETS_RECEIVED_LOW,
			    "GOOD_OCTETS_RECEIVED");
	mv_gop110_mib_print(gop, port, MV_MIB_BAD_OCTETS_RECEIVED,
			    "BAD_OCTETS_RECEIVED");

	mv_gop110_mib_print(gop, port, MV_MIB_UNICAST_FRAMES_RECEIVED,
			    "UNCAST_FRAMES_RECEIVED");
	mv_gop110_mib_print(gop, port, MV_MIB_BROADCAST_FRAMES_RECEIVED,
			    "BROADCAST_FRAMES_RECEIVED");
	mv_gop110_mib_print(gop, port, MV_MIB_MULTICAST_FRAMES_RECEIVED,
			    "MULTICAST_FRAMES_RECEIVED");

	pr_info("\n[RMON]\n");
	mv_gop110_mib_print(gop, port, MV_MIB_FRAMES_64_OCTETS,
			    "FRAMES_64_OCTETS");
	mv_gop110_mib_print(gop, port, MV_MIB_FRAMES_65_TO_127_OCTETS,
			    "FRAMES_65_TO_127_OCTETS");
	mv_gop110_mib_print(gop, port, MV_MIB_FRAMES_128_TO_255_OCTETS,
			    "FRAMES_128_TO_255_OCTETS");
	mv_gop110_mib_print(gop, port, MV_MIB_FRAMES_256_TO_511_OCTETS,
			    "FRAMES_256_TO_511_OCTETS");
	mv_gop110_mib_print(gop, port, MV_MIB_FRAMES_512_TO_1023_OCTETS,
			    "FRAMES_512_TO_1023_OCTETS");
	mv_gop110_mib_print(gop, port, MV_MIB_FRAMES_1024_TO_MAX_OCTETS,
			    "FRAMES_1024_TO_MAX_OCTETS");

	pr_info("\n[Tx]\n");
	mv_gop110_mib_print(gop, port, MV_MIB_GOOD_OCTETS_SENT_LOW,
			    "GOOD_OCTETS_SENT");
	mv_gop110_mib_print(gop, port, MV_MIB_UNICAST_FRAMES_SENT,
			    "UNICAST_FRAMES_SENT");
	mv_gop110_mib_print(gop, port, MV_MIB_MULTICAST_FRAMES_SENT,
			    "MULTICAST_FRAMES_SENT");
	mv_gop110_mib_print(gop, port, MV_MIB_BROADCAST_FRAMES_SENT,
			    "BROADCAST_FRAMES_SENT");
	mv_gop110_mib_print(gop, port, MV_MIB_CRC_ERRORS_SENT,
			    "CRC_ERRORS_SENT");

	pr_info("\n[FC control]\n");
	mv_gop110_mib_print(gop, port, MV_MIB_FC_RECEIVED,
			    "FC_RECEIVED");
	mv_gop110_mib_print(gop, port, MV_MIB_FC_SENT,
			    "FC_SENT");

	pr_info("\n[Errors]\n");
	mv_gop110_mib_print(gop, port, MV_MIB_RX_FIFO_OVERRUN,
			    "RX_FIFO_OVERRUN");
	mv_gop110_mib_print(gop, port, MV_MIB_UNDERSIZE_RECEIVED,
			    "UNDERSIZE_RECEIVED");
	mv_gop110_mib_print(gop, port, MV_MIB_FRAGMENTS_RECEIVED,
			    "FRAGMENTS_RECEIVED");
	mv_gop110_mib_print(gop, port, MV_MIB_OVERSIZE_RECEIVED,
			    "OVERSIZE_RECEIVED");
	mv_gop110_mib_print(gop, port, MV_MIB_JABBER_RECEIVED,
			    "JABBER_RECEIVED");
	mv_gop110_mib_print(gop, port, MV_MIB_MAC_RECEIVE_ERROR,
			    "MAC_RECEIVE_ERROR");
	mv_gop110_mib_print(gop, port, MV_MIB_BAD_CRC_EVENT,
			    "BAD_CRC_EVENT");
	mv_gop110_mib_print(gop, port, MV_MIB_COLLISION,
			    "COLLISION");
	/* This counter must be read last. Read it clear all the counters */
	mv_gop110_mib_print(gop, port, MV_MIB_LATE_COLLISION,
			    "LATE_COLLISION");
}
EXPORT_SYMBOL(mv_gop110_mib_counters_show);

void mv_gop110_ptp_enable(struct gop_hw *gop, int port, bool state)
{
	u32 reg_data;

	if (state) {
		/* PTP enable */
		reg_data = mv_gop110_ptp_read(gop, port,
					      MV_PTP_GENERAL_CTRL_REG);
		reg_data |= MV_PTP_GENERAL_CTRL_PTP_UNIT_ENABLE_MASK;
		/* enable PTP */
		mv_gop110_ptp_write(gop, port, MV_PTP_GENERAL_CTRL_REG,
				    reg_data);
		/* unreset unit */
		reg_data |= MV_PTP_GENERAL_CTRL_PTP_RESET_MASK;
		mv_gop110_ptp_write(gop, port, MV_PTP_GENERAL_CTRL_REG,
				    reg_data);
	} else {
		reg_data = mv_gop110_ptp_read(gop, port,
					      MV_PTP_GENERAL_CTRL_REG);
		reg_data &= ~MV_PTP_GENERAL_CTRL_PTP_UNIT_ENABLE_MASK;
		/* disable PTP */
		mv_gop110_ptp_write(gop, port, MV_PTP_GENERAL_CTRL_REG,
				    reg_data);
	}
}

void mv_gop110_netc_active_port(struct gop_hw *gop, u32 port, u32 val)
{
	u32 reg;

	reg = mv_gop110_rfu1_read(gop, MV_NETCOMP_PORTS_CONTROL_1);
	reg &= ~(NETC_PORTS_ACTIVE_MASK(port));

	val <<= NETC_PORTS_ACTIVE_OFFSET(port);
	val &= NETC_PORTS_ACTIVE_MASK(port);

	reg |= val;

	mv_gop110_rfu1_write(gop, MV_NETCOMP_PORTS_CONTROL_1, reg);
}

static void mv_gop110_netc_xaui_enable(struct gop_hw *gop, u32 port, u32 val)
{
	u32 reg;

	reg = mv_gop110_rfu1_read(gop, SD1_CONTROL_1_REG);
	reg &= ~SD1_CONTROL_XAUI_EN_MASK;

	val <<= SD1_CONTROL_XAUI_EN_OFFSET;
	val &= SD1_CONTROL_XAUI_EN_MASK;

	reg |= val;

	mv_gop110_rfu1_write(gop, SD1_CONTROL_1_REG, reg);
}

static void mv_gop110_netc_rxaui0_enable(struct gop_hw *gop, u32 port, u32 val)
{
	u32 reg;

	reg = mv_gop110_rfu1_read(gop, SD1_CONTROL_1_REG);
	reg &= ~SD1_CONTROL_RXAUI0_L23_EN_MASK;

	val <<= SD1_CONTROL_RXAUI0_L23_EN_OFFSET;
	val &= SD1_CONTROL_RXAUI0_L23_EN_MASK;

	reg |= val;

	mv_gop110_rfu1_write(gop, SD1_CONTROL_1_REG, reg);
}

static void mv_gop110_netc_rxaui1_enable(struct gop_hw *gop, u32 port, u32 val)
{
	u32 reg;

	reg = mv_gop110_rfu1_read(gop, SD1_CONTROL_1_REG);
	reg &= ~SD1_CONTROL_RXAUI1_L45_EN_MASK;

	val <<= SD1_CONTROL_RXAUI1_L45_EN_OFFSET;
	val &= SD1_CONTROL_RXAUI1_L45_EN_MASK;

	reg |= val;

	mv_gop110_rfu1_write(gop, SD1_CONTROL_1_REG, reg);
}



static void mv_gop110_netc_mii_mode(struct gop_hw *gop, u32 port, u32 val)
{
	u32 reg;

	reg = mv_gop110_rfu1_read(gop, MV_NETCOMP_CONTROL_0);
	reg &= ~NETC_GBE_PORT1_MII_MODE_MASK;

	val <<= NETC_GBE_PORT1_MII_MODE_OFFSET;
	val &= NETC_GBE_PORT1_MII_MODE_MASK;

	reg |= val;

	mv_gop110_rfu1_write(gop, MV_NETCOMP_CONTROL_0, reg);
}



static void mv_gop110_netc_gop_reset(struct gop_hw *gop, u32 val)
{
	u32 reg;

	reg = mv_gop110_rfu1_read(gop, MV_GOP_SOFT_RESET_1_REG);
	reg &= ~NETC_GOP_SOFT_RESET_MASK;

	val <<= NETC_GOP_SOFT_RESET_OFFSET;
	val &= NETC_GOP_SOFT_RESET_MASK;

	reg |= val;

	mv_gop110_rfu1_write(gop, MV_GOP_SOFT_RESET_1_REG, reg);
}

static void mv_gop110_netc_gop_clock_logic_set(struct gop_hw *gop, u32 val)
{
	u32 reg;

	reg = mv_gop110_rfu1_read(gop, MV_NETCOMP_PORTS_CONTROL_0);
	reg &= ~NETC_CLK_DIV_PHASE_MASK;

	val <<= NETC_CLK_DIV_PHASE_OFFSET;
	val &= NETC_CLK_DIV_PHASE_MASK;

	reg |= val;

	mv_gop110_rfu1_write(gop, MV_NETCOMP_PORTS_CONTROL_0, reg);
}

static void mv_gop110_netc_port_rf_reset(struct gop_hw *gop, u32 port, u32 val)
{
	u32 reg;

	reg = mv_gop110_rfu1_read(gop, MV_NETCOMP_PORTS_CONTROL_1);
	reg &= ~(NETC_PORT_GIG_RF_RESET_MASK(port));

	val <<= NETC_PORT_GIG_RF_RESET_OFFSET(port);
	val &= NETC_PORT_GIG_RF_RESET_MASK(port);

	reg |= val;

	mv_gop110_rfu1_write(gop, MV_NETCOMP_PORTS_CONTROL_1, reg);
}

static void mv_gop110_netc_gbe_sgmii_mode_select(struct gop_hw *gop, u32 port,
						u32 val)
{
	u32 reg, mask, offset;

	if (port == 2) {
		mask = NETC_GBE_PORT0_SGMII_MODE_MASK;
		offset = NETC_GBE_PORT0_SGMII_MODE_OFFSET;
	} else {
		mask = NETC_GBE_PORT1_SGMII_MODE_MASK;
		offset = NETC_GBE_PORT1_SGMII_MODE_OFFSET;
	}
	reg = mv_gop110_rfu1_read(gop, MV_NETCOMP_CONTROL_0);
	reg &= ~mask;

	val <<= offset;
	val &= mask;

	reg |= val;

	mv_gop110_rfu1_write(gop, MV_NETCOMP_CONTROL_0, reg);
}

static void mv_gop110_netc_bus_width_select(struct gop_hw *gop, u32 val)
{
	u32 reg;

	reg = mv_gop110_rfu1_read(gop, MV_NETCOMP_PORTS_CONTROL_0);
	reg &= ~NETC_BUS_WIDTH_SELECT_MASK;

	val <<= NETC_BUS_WIDTH_SELECT_OFFSET;
	val &= NETC_BUS_WIDTH_SELECT_MASK;

	reg |= val;

	mv_gop110_rfu1_write(gop, MV_NETCOMP_PORTS_CONTROL_0, reg);
}

static void mv_gop110_netc_sample_stages_timing(struct gop_hw *gop, u32 val)
{
	u32 reg;

	reg = mv_gop110_rfu1_read(gop, MV_NETCOMP_PORTS_CONTROL_0);
	reg &= ~NETC_GIG_RX_DATA_SAMPLE_MASK;

	val <<= NETC_GIG_RX_DATA_SAMPLE_OFFSET;
	val &= NETC_GIG_RX_DATA_SAMPLE_MASK;

	reg |= val;

	mv_gop110_rfu1_write(gop, MV_NETCOMP_PORTS_CONTROL_0, reg);
}

static void mv_gop110_netc_mac_to_xgmii(struct gop_hw *gop, u32 port,
					enum mv_netc_phase phase)
{
	switch (phase) {
	case MV_NETC_FIRST_PHASE:
		/* Set Bus Width to HB mode = 1 */
		mv_gop110_netc_bus_width_select(gop, 1);
		/* Select RGMII mode */
		mv_gop110_netc_gbe_sgmii_mode_select(gop, port,
							MV_NETC_GBE_XMII);
		break;
	case MV_NETC_SECOND_PHASE:
		/* De-assert the relevant port HB reset */
		mv_gop110_netc_port_rf_reset(gop, port, 1);
		break;
	}
}

static void mv_gop110_netc_mac_to_sgmii(struct gop_hw *gop, u32 port,
					enum mv_netc_phase phase)
{
	switch (phase) {
	case MV_NETC_FIRST_PHASE:
		/* Set Bus Width to HB mode = 1 */
		mv_gop110_netc_bus_width_select(gop, 1);
		/* Select SGMII mode */
		if (port >= 1)
			mv_gop110_netc_gbe_sgmii_mode_select(gop, port,
			MV_NETC_GBE_SGMII);

		/* Configure the sample stages */
		mv_gop110_netc_sample_stages_timing(gop, 0);
		/* Configure the ComPhy Selector */
		/* mv_gop110_netc_com_phy_selector_config(netComplex); */
		break;
	case MV_NETC_SECOND_PHASE:
		/* De-assert the relevant port HB reset */
		mv_gop110_netc_port_rf_reset(gop, port, 1);
		break;
	}
}

static void mv_gop110_netc_mac_to_rxaui(struct gop_hw *gop, u32 port,
					enum mv_netc_phase phase,
					enum mv_netc_lanes lanes)
{
	/* Currently only RXAUI0 supported */
	if (port != 0)
		return;

	switch (phase) {
	case MV_NETC_FIRST_PHASE:
		/* RXAUI Serdes/s Clock alignment */
		if (lanes == MV_NETC_LANE_23)
			mv_gop110_netc_rxaui0_enable(gop, port, 1);
		else
			mv_gop110_netc_rxaui1_enable(gop, port, 1);
		break;
	case MV_NETC_SECOND_PHASE:
		/* De-assert the relevant port HB reset */
		mv_gop110_netc_port_rf_reset(gop, port, 1);
		break;
	}
}

static void mv_gop110_netc_mac_to_xaui(struct gop_hw *gop, u32 port,
					enum mv_netc_phase phase)
{
	switch (phase) {
	case MV_NETC_FIRST_PHASE:
		/* RXAUI Serdes/s Clock alignment */
		mv_gop110_netc_xaui_enable(gop, port, 1);
		break;
	case MV_NETC_SECOND_PHASE:
		/* De-assert the relevant port HB reset */
		mv_gop110_netc_port_rf_reset(gop, port, 1);
		break;
	}
}


int mv_gop110_netc_init(struct gop_hw *gop,
			u32 net_comp_config, enum mv_netc_phase phase)
{
	u32 c = net_comp_config;

	MVPP2_PRINT_VAR(net_comp_config);

	if (c & MV_NETC_GE_MAC0_RXAUI_L23)
		mv_gop110_netc_mac_to_rxaui(gop, 0, phase, MV_NETC_LANE_23);

	if (c & MV_NETC_GE_MAC0_RXAUI_L45)
		mv_gop110_netc_mac_to_rxaui(gop, 0, phase, MV_NETC_LANE_45);

	if (c & MV_NETC_GE_MAC0_XAUI)
		mv_gop110_netc_mac_to_xaui(gop, 0, phase);

	if (c & MV_NETC_GE_MAC2_SGMII)
		mv_gop110_netc_mac_to_sgmii(gop, 2, phase);
	else
		mv_gop110_netc_mac_to_xgmii(gop, 2, phase);
	if (c & MV_NETC_GE_MAC3_SGMII)
		mv_gop110_netc_mac_to_sgmii(gop, 3, phase);
	else {
		mv_gop110_netc_mac_to_xgmii(gop, 3, phase);
		if (c & MV_NETC_GE_MAC3_RGMII)
			mv_gop110_netc_mii_mode(gop, 3, MV_NETC_GBE_RGMII);
		else
			mv_gop110_netc_mii_mode(gop, 3, MV_NETC_GBE_MII);
	}

	/* Activate gop ports 0, 2, 3 */
	mv_gop110_netc_active_port(gop, 0, 1);
	mv_gop110_netc_active_port(gop, 2, 1);
	mv_gop110_netc_active_port(gop, 3, 1);

	if (phase == MV_NETC_SECOND_PHASE) {
		/* Enable the GOP internal clock logic */
		mv_gop110_netc_gop_clock_logic_set(gop, 1);
		/* De-assert GOP unit reset */
		mv_gop110_netc_gop_reset(gop, 1);
	}
	return 0;
}


