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
#include "common/mv_sw_if.h"
#include "common/mv_hw_if.h"
#include "gop/a390_mg_if.h"
#include "gop/mv_gop_if.h"
#include "gop/mac/mv_gmac_if.h"
#include "gop/mac/mv_xlg_mac_if.h"
#include "gop/pcs/mv_gpcs_if.h"
#include "gop/pcs/mv_xpcs_if.h"
#include "gop/serdes/mv_serdes_if.h"


struct mv_io_addr *gop_regs_addrs_size;
static void __iomem		*gop_vbase_address;
static enum mv_gop_access_mode	gop_access_mode;
static struct gop_port_ctrl	*gop_ports;
static int			gop_ports_num;

void mv_gop_init(struct mv_io_addr *gop_regs, int ports_num, enum mv_gop_access_mode mode)
{
	int i;

	gop_ports = kzalloc(sizeof(struct gop_port_ctrl) * ports_num, GFP_KERNEL);
	if (gop_ports == NULL) {
		pr_err("%s: Can't allocated %d bytes of memory\n",
				__func__, sizeof(struct gop_port_ctrl) * ports_num);
		return;
	}
	gop_regs_addrs_size = gop_regs;
	gop_ports_num = ports_num;
	gop_access_mode = mode;
	gop_vbase_address = gop_regs->vaddr;
	if (mode == INDIRECT_MG_ACCESS)
		a390_addr_completion_init(gop_vbase_address);

	for (i = 0; i < ports_num; i++)
		gop_ports[i].flags = (1 << NOT_CREATED);
}

int mv_gop_addrs_size_get(u32 *va_base, u32 *pa_base, u32 *size)
{
	*va_base = (u32)gop_regs_addrs_size->vaddr;
	*pa_base = (u32)gop_regs_addrs_size->paddr;
	*size = (u32)gop_regs_addrs_size->size;
	if (gop_access_mode != INDIRECT_MG_ACCESS)
		return -1; /* gop-base is valid for indirect only */
	return 0;
}

/* return read register value */
u32 mv_gop_reg_read(u32 reg_addr)
{
	u32 access_addr;

	if (gop_access_mode == INDIRECT_MG_ACCESS)
		access_addr = a390_addr_completion_cfg(reg_addr);
	else
		access_addr = reg_addr;

	return readl((void *)(access_addr + gop_vbase_address));
}

/* write data to register */
void mv_gop_reg_write(u32 reg_addr, u32 data)
{
	u32 access_addr;

	if (gop_access_mode == INDIRECT_MG_ACCESS)
		access_addr = a390_addr_completion_cfg(reg_addr);
	else
		access_addr = reg_addr;
#ifdef PP3_DEBUG
	pr_info("\nwrite reg 0x%x, data 0x%x", reg_addr, data);
#endif
	writel(data, (void *)(access_addr + gop_vbase_address));
}

void mv_gop_reg_print(char *reg_name, u32 reg)
{
	pr_info("  %-32s: 0x%x = 0x%08x\n", reg_name, reg, mv_gop_reg_read(reg));
}

/*******************************************************************************
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
*******************************************************************************/
int mv_pp3_gop_port_init(int port_num, enum mv_port_mode port_mode)
{
	int num_of_act_lanes;
	int mac_num;

	if (port_num >= gop_ports_num) {
		pr_err("%s: illegal port number %d", __func__, port_num);
		return -1;
	}

	switch (port_mode) {
	case MV_PORT_RGMII:
		mac_num = port_num;
		mv_gmac_reset(mac_num, RESET);
		/* configure PCS */
		mv_gpcs_mode_cfg(mac_num, false);

		/* configure MAC */
		mv_gmac_mode_cfg(mac_num, port_mode);
		/* pcs unreset */
		mv_gpcs_reset(port_num, UNRESET);
		/* mac unreset */
		mv_gmac_reset(mac_num, UNRESET);
	break;
	case MV_PORT_SGMII:
	case MV_PORT_SGMII2_5:
	case MV_PORT_QSGMII:
		num_of_act_lanes = 1;
		mac_num = port_num;
		/* configure PCS */
		mv_gpcs_mode_cfg(mac_num, true);

		/* configure MAC */
		mv_gmac_mode_cfg(mac_num, port_mode);
		/* select proper Mac mode */
		mv_xlg_2_gig_mac_cfg(mac_num);

		/* pcs unreset */
		mv_gpcs_reset(port_num, UNRESET);
		/* mac unreset */
		mv_gmac_reset(mac_num, UNRESET);
	break;
	case MV_PORT_XAUI:
		num_of_act_lanes = 4;
		mac_num = 0;
		/* configure PCS */
		mv_xpcs_mode(num_of_act_lanes);
		/* configure MAC */
		mv_xlg_mac_mode_cfg(mac_num, num_of_act_lanes);

		/* pcs unreset */
		mv_xpcs_reset(UNRESET);
		/* mac unreset */
		mv_xlg_mac_reset(mac_num, UNRESET);
	break;
	case MV_PORT_RXAUI:
		num_of_act_lanes = 2;
		mv_serdes_init(0, MV_RXAUI); /* mapped to serdes 6 */
		mv_serdes_init(1, MV_RXAUI); /* mapped to serdes 5 */

		mac_num = 0;
		/* configure PCS */
		mv_xpcs_mode(num_of_act_lanes);
		/* configure MAC */
		mv_xlg_mac_mode_cfg(mac_num, num_of_act_lanes);

		/* pcs unreset */
		mv_xpcs_reset(UNRESET);

		/* mac unreset */;
		mv_xlg_mac_reset(mac_num, UNRESET);
	break;
	default:
		pr_err("%s: Requested port mode (%d) not supported", __func__, port_mode);
		return -1;
	}

	gop_ports[port_num].port_mode = port_mode;
	gop_ports[port_num].flags = (1 << CREATED);

	return 0;
}

static inline int mv_pp3_check_gop_port_num(const char *msg, int port_num)
{
	int rc = 0;

	if (port_num >= gop_ports_num) {
		pr_err("%s: illegal port #%d", msg, port_num);
		rc = -1;
	}

	if (gop_ports[port_num].flags & (1 << NOT_CREATED)) {
		pr_err("%s: no port mode defined on port #%d\n", msg, port_num);
		rc = -1;
	}

	return rc;
}

/*******************************************************************************
* mv_port_reset
*
* DESCRIPTION:
*       Clears the port mode and release all its resources according to selected.
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
*******************************************************************************/
int mv_pp3_gop_port_reset(int port_num)
{
	int mac_num;

	if (mv_pp3_check_gop_port_num(__func__, port_num))
		return -1;

	switch (gop_ports[port_num].port_mode) {
	case MV_PORT_RGMII:
	case MV_PORT_SGMII:
	case MV_PORT_SGMII2_5:
	case MV_PORT_QSGMII:
		mac_num = port_num;
		/* pcs unreset */
		mv_gpcs_reset(port_num, RESET);
		/* mac unreset */
		mv_gmac_reset(mac_num, RESET);
	break;
	case MV_PORT_XAUI:
		mac_num = 0;
		/* pcs unreset */
		mv_xpcs_reset(RESET);
		/* mac unreset */
		mv_xlg_mac_reset(mac_num, RESET);
	break;
	case MV_PORT_RXAUI:
		mac_num = 0;
		/* pcs unreset */
		mv_xpcs_reset(RESET);
		/* mac unreset */
		mv_xlg_mac_reset(mac_num, RESET);
	break;
	default:
		pr_err("%s: Wrong port mode (%d)", __func__, gop_ports[port_num].port_mode);
		return -1;
	}

	gop_ports[port_num].flags |= (1 << UNDER_RESET);

	/* TBD:serdes reset or power down if needed*/

	return 0;
}

/*-------------------------------------------------------------------*/
void mv_pp3_gop_port_enable(int port_num)
{
	if (mv_pp3_check_gop_port_num(__func__, port_num))
		return;

	switch (gop_ports[port_num].port_mode) {
	case MV_PORT_RGMII:
	case MV_PORT_SGMII:
	case MV_PORT_SGMII2_5:
	case MV_PORT_QSGMII:
		mv_gmac_port_enable(port_num);
	break;
	case MV_PORT_XAUI:
	case MV_PORT_RXAUI:
		/* run digital reset / unreset */
		mv_serdes_reset(0, false, false, true);
		mv_serdes_reset(1, false, false, true);
		mv_serdes_reset(0, false, false, false);
		mv_serdes_reset(1, false, false, false);
		mv_xlg_mac_port_enable(port_num);
	break;
	default:
		pr_err("%s: Wrong port mode (%d)", __func__, gop_ports[port_num].port_mode);
		return;
	}

	gop_ports[port_num].flags |= (1 << ENABLED);
}

void mv_pp3_gop_port_disable(int port_num)
{
	if (mv_pp3_check_gop_port_num(__func__, port_num))
		return;

	switch (gop_ports[port_num].port_mode) {
	case MV_PORT_RGMII:
	case MV_PORT_SGMII:
	case MV_PORT_SGMII2_5:
	case MV_PORT_QSGMII:
		mv_gmac_port_disable(port_num);
	break;
	case MV_PORT_XAUI:
	case MV_PORT_RXAUI:
		mv_xlg_mac_port_disable(0);
	break;
	default:
		pr_err("%s: Wrong port mode (%d)", __func__, gop_ports[port_num].port_mode);
		return;
	}

	gop_ports[port_num].flags &= ~(1 << ENABLED);
}

void mv_pp3_gop_port_periodic_xon_set(int port_num, int enable)
{
	if (mv_pp3_check_gop_port_num(__func__, port_num))
		return;

	switch (gop_ports[port_num].port_mode) {
	case MV_PORT_SGMII:
	case MV_PORT_SGMII2_5:
	case MV_PORT_QSGMII:
		mv_gmac_port_periodic_xon_set(port_num, enable);
	break;
	case MV_PORT_XAUI:
	case MV_PORT_RXAUI:
		mv_xlg_mac_port_periodic_xon_set(port_num, enable);
	break;
	default:
		pr_err("%s: Wrong port mode (%d)", __func__, gop_ports[port_num].port_mode);
		return;
	}
}

bool mv_pp3_gop_port_is_link_up(int port_num)
{
	if (mv_pp3_check_gop_port_num(__func__, port_num))
		return false;

	switch (gop_ports[port_num].port_mode) {
	case MV_PORT_RGMII:
	case MV_PORT_SGMII:
	case MV_PORT_SGMII2_5:
	case MV_PORT_QSGMII:
		return mv_gmac_link_status_get(port_num);
	break;
	case MV_PORT_XAUI:
	case MV_PORT_RXAUI:
		udelay(1000);
		return mv_xlg_mac_link_status_get(port_num);
	break;
	default:
		pr_err("%s: Wrong port mode (%d)", __func__, gop_ports[port_num].port_mode);
		return false;
	}
}

int mv_pp3_gop_port_link_status(int port_num, struct mv_port_link_status *pstatus)
{
	if (mv_pp3_check_gop_port_num(__func__, port_num))
		return -1;

	switch (gop_ports[port_num].port_mode) {
	case MV_PORT_RGMII:
	case MV_PORT_SGMII:
	case MV_PORT_QSGMII:
		mv_gmac_link_status(port_num, pstatus);
	break;
	case MV_PORT_SGMII2_5:
		mv_gmac_link_status(port_num, pstatus);
		if (pstatus->speed == MV_PORT_SPEED_1000)
			pstatus->speed = MV_PORT_SPEED_2000;
	break;
	case MV_PORT_XAUI:
	case MV_PORT_RXAUI:
		mv_xlg_mac_link_status(port_num, pstatus);
	break;
	default:
		pr_err("%s: Wrong port mode (%d)", __func__, gop_ports[port_num].port_mode);
		return -1;
	}
	return 0;
}

int mv_pp3_gop_port_regs(int port_num)
{
	if (mv_pp3_check_gop_port_num(__func__, port_num))
		return -1;

	switch (gop_ports[port_num].port_mode) {
	case MV_PORT_RGMII:
	case MV_PORT_SGMII:
	case MV_PORT_SGMII2_5:
	case MV_PORT_QSGMII:
		pr_info("\n[gop GMAC #%d registers]\n", port_num);
		mv_gmac_regs_dump(port_num);
	break;
	case MV_PORT_XAUI:
	case MV_PORT_RXAUI:
		pr_info("\n[gop XLG MAC #%d registers]\n", port_num);
		mv_xlg_mac_regs_dump(port_num);
	break;
	default:
		pr_err("%s: Wrong port mode (%d)", __func__, gop_ports[port_num].port_mode);
		return -1;
	}
	return 0;
}

int mv_pp3_gop_port_events_mask(int port_num)
{
	if (mv_pp3_check_gop_port_num(__func__, port_num))
		return -1;

	switch (gop_ports[port_num].port_mode) {
	case MV_PORT_RGMII:
	case MV_PORT_SGMII:
	case MV_PORT_SGMII2_5:
	case MV_PORT_QSGMII:
		mv_gmac_port_link_event_mask(port_num);
	break;
	case MV_PORT_XAUI:
	case MV_PORT_RXAUI:
		mv_xlg_port_link_event_mask(port_num);
	break;
	default:
		pr_err("%s: Wrong port mode (%d)", __func__, gop_ports[port_num].port_mode);
		return -1;
	}
	return 0;
}

int mv_pp3_gop_port_events_unmask(int port_num)
{
	if (mv_pp3_check_gop_port_num(__func__, port_num))
		return -1;

	switch (gop_ports[port_num].port_mode) {
	case MV_PORT_RGMII:
	case MV_PORT_SGMII:
	case MV_PORT_SGMII2_5:
	case MV_PORT_QSGMII:
		mv_gmac_port_link_event_unmask(port_num);
		/* gige interrupt cause connected to CPU via XLG external interrupt cause */
		mv_xlg_port_external_event_unmask(0, 2);
	break;
	case MV_PORT_XAUI:
	case MV_PORT_RXAUI:
		mv_xlg_port_external_event_unmask(port_num, 1);
	break;
	default:
		pr_err("%s: Wrong port mode (%d)", __func__, gop_ports[port_num].port_mode);
		return -1;
	}
	return 0;
}

int mv_pp3_gop_port_events_clear(int port_num)
{
	if (mv_pp3_check_gop_port_num(__func__, port_num))
		return -1;

	switch (gop_ports[port_num].port_mode) {
	case MV_PORT_RGMII:
	case MV_PORT_SGMII:
	case MV_PORT_SGMII2_5:
	case MV_PORT_QSGMII:
		mv_gmac_port_link_event_clear(port_num);
	break;
	case MV_PORT_XAUI:
	case MV_PORT_RXAUI:
		mv_xlg_port_link_event_clear(port_num);
	break;
	default:
		pr_err("%s: Wrong port mode (%d)", __func__, gop_ports[port_num].port_mode);
		return -1;
	}
	return 0;
}

int mv_pp3_gop_status_show(int port_num)
{
	struct mv_port_link_status port_status;
	bool port_en;

	if (mv_pp3_check_gop_port_num(__func__, port_num))
		return -1;

	mv_pp3_gop_port_link_status(port_num, &port_status);
	port_en = (gop_ports[port_num].flags & (1 << ENABLED)) ? true : false;

	pr_info("-------------- Port %d configuration ----------------", port_num);

	switch (gop_ports[port_num].port_mode) {
	case MV_PORT_RGMII:
		pr_info("Port mode               : RGMII");
	break;
	case MV_PORT_SGMII:
		pr_info("Port mode               : SGMII");
	break;
	case MV_PORT_SGMII2_5:
		pr_info("Port mode               : SGMII2_5");
	break;
	case MV_PORT_QSGMII:
		pr_info("Port mode               : QSGMII");
	break;
	case MV_PORT_XAUI:
		pr_info("Port mode               : XAUI");
	break;
	case MV_PORT_RXAUI:
		pr_info("Port mode               : RXAUI");
	break;
	default:
		pr_err("%s: Wrong port mode (%d)", __func__, gop_ports[port_num].port_mode);
		return -1;
	}
	pr_info("\nMAC status              : %s", (port_en) ? "enable" : "disable");
	pr_info("\nLink status             : %s", (port_status.linkup) ? "link up" : "link down");
	pr_info("\n");

	if ((gop_ports[port_num].port_mode == MV_PORT_SGMII2_5) && (port_status.speed == MV_PORT_SPEED_1000))
		port_status.speed = MV_PORT_SPEED_2000;

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
	case MV_PORT_SPEED_2000:
		pr_info("Port speed              : 2.5G");
	break;
	case MV_PORT_SPEED_10000:
		pr_info("Port speed              : 10G");
	break;
	default:
		pr_err("%s: Wrong port speed (%d)\n", __func__, port_status.speed);
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
		pr_err("%s: Wrong port duplex (%d)", __func__, port_status.duplex);
		return -1;
	}
	pr_info("\n");

	return 0;
}

/* get port speed and duplex */
int mv_pp3_gop_speed_duplex_get(int port_num, enum mv_port_speed *speed, enum mv_port_duplex *duplex)
{
	if (mv_pp3_check_gop_port_num(__func__, port_num))
		return -1;

	switch (gop_ports[port_num].port_mode) {
	case MV_PORT_RGMII:
	case MV_PORT_SGMII:
	case MV_PORT_SGMII2_5:
	case MV_PORT_QSGMII:
		mv_gmac_speed_duplex_get(port_num, speed, duplex);
	break;
	case MV_PORT_XAUI:
	case MV_PORT_RXAUI:
		mv_xlg_mac_speed_duplex_get(port_num, speed, duplex);
	break;
	default:
		pr_err("%s: Wrong port mode (%d)", __func__, gop_ports[port_num].port_mode);
		return -1;
	}
	return 0;
}

/* set port speed and duplex */
int mv_pp3_gop_speed_duplex_set(int port_num, enum mv_port_speed speed, enum mv_port_duplex duplex)
{
	if (mv_pp3_check_gop_port_num(__func__, port_num))
		return -1;

	switch (gop_ports[port_num].port_mode) {
	case MV_PORT_RGMII:
	case MV_PORT_SGMII:
	case MV_PORT_SGMII2_5:
	case MV_PORT_QSGMII:
		mv_gmac_speed_duplex_set(port_num, speed, duplex);
	break;
	case MV_PORT_XAUI:
	case MV_PORT_RXAUI:
		mv_xlg_mac_speed_duplex_set(port_num, speed, duplex);
	break;
	default:
		pr_err("%s: Wrong port mode (%d)", __func__, gop_ports[port_num].port_mode);
		return -1;
	}
	return 0;
}

int mv_pp3_gop_autoneg_restart(int port_num)
{
	if (mv_pp3_check_gop_port_num(__func__, port_num))
		return -1;

	switch (gop_ports[port_num].port_mode) {
	case MV_PORT_RGMII:
	break;
	case MV_PORT_SGMII:
	case MV_PORT_SGMII2_5:
	case MV_PORT_QSGMII:
		mv_gmac_port_autoneg_restart(port_num);
	break;
	case MV_PORT_XAUI:
	case MV_PORT_RXAUI:
		pr_err("gop port %d: autoneg-restart not supported for mode=%d (XAUI/RXAUI)\n",
		       port_num, gop_ports[port_num].port_mode);
		return -1;
	default:
		pr_err("gop port %d: autoneg-restart wrong mode=%d\n",
		       port_num, gop_ports[port_num].port_mode);
		return -1;
	}
	return 0;
}

int mv_pp3_gop_fl_cfg(int port_num)
{
	if (mv_pp3_check_gop_port_num(__func__, port_num))
		return -1;

	switch (gop_ports[port_num].port_mode) {
	case MV_PORT_RGMII:
	case MV_PORT_SGMII:
	case MV_PORT_QSGMII:
		/* disable AN */
		mv_pp3_gop_speed_duplex_set(port_num, MV_PORT_SPEED_1000, MV_PORT_DUPLEX_FULL);
		/* force link */
		mv_gmac_force_link_mode_set(port_num, true, false);
	break;
	case MV_PORT_SGMII2_5:
		/* disable AN */
		mv_pp3_gop_speed_duplex_set(port_num, MV_PORT_SPEED_2000, MV_PORT_DUPLEX_FULL);
		/* force link */
		mv_gmac_force_link_mode_set(port_num, true, false);
	break;
	case MV_PORT_XAUI:
	case MV_PORT_RXAUI:
		return 0;
	default:
		pr_err("%s: Wrong port mode (%d)", __func__, gop_ports[port_num].port_mode);
		return -1;
	}
	return 0;
}

