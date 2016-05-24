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

/*****************************************************************
 * This file contains
 *    PTP (timestamp) hw config and access utilities
 *    TAI-Clock       hw config and access utilities
 *    TAI/ToD         up-level operations
 * Refer also
 *    mv_pp3_ptp_reg_read      - inline in .H
 *    mv_pp3_ptp_reg_write     - inline in .H
 ***************************************************************
 */

/* includes */
#include <linux/kernel.h>
#include "gop/a390_mg_if.h"
#include "gop/mv_gop_if.h"
#include "gop/mv_ptp_regs.h"
#include "gop/mv_tai_regs.h"
#include "gop/mv_ptp_if.h"
#include "net_dev/mv_ptp_service.h"
#include "platform/mv_pp3_defs.h"

static u32 ptp_tclk_hz;
static u32 ptp_ports_enabled; /* used to reset ports on tai_clock */
static u32 ptp_tai_clock_in_cntr;
static bool ptp_tai_clock_enabled;
static bool ptp_tai_clock_external_cfg;

static DEFINE_MUTEX(ptp_op_mutex);
static DEFINE_MUTEX(ptp_clock_in_cntr_mutex);

/************************************************************************/
/*********    HW-level utilities     ************************************/
/************************************************************************/

/* ------------------------------------------------------------*/
/* -------   TAI Clock  (hw-level)   --------------------------*/
/* ------------------------------------------------------------*/

/* Set Tclock value [Hz] from dtb "clock-frequency" */
int mv_pp3_ptp_tclk_hz_set(u32 tclk_hz)
{
	ptp_tclk_hz = tclk_hz;
	return 0;
}

void mv_pp3_tai_clock_stable_status_set(bool on)
{
	/* Save to "free" REG, PTP-Application could poll it
	 * like the mv_pp3_tai_clock_stable_status_get() does
	 * DRIFT_THR_CFG_HIGH is chosen as free
	 * Reg's Init-value=0xFFFF, keep it when stable otherwise 0.
	 */
	const u32 reg_init_val = 0x0000ffff;
	u32 regv = (on) ? reg_init_val : 0;
	mv_pp3_ptp_reg_write(MV_TAI_DRIFT_THR_CFG_HIGH_REG, regv);
}

bool mv_pp3_tai_clock_stable_status_get(void)
{
	u32 regv = mv_pp3_ptp_reg_read(MV_TAI_DRIFT_THR_CFG_HIGH_REG);
	return (bool)regv & ptp_tai_clock_enabled;
}

bool mv_pp3_tai_clock_enable_get(void)
{
	return ptp_tai_clock_enabled;
}

u16 mv_pp3_tai_clock_in_cntr_get(u32 *accumulated)
{
	/* The HW 1PPS-in-Counter is reset upon reading
	 * If some contexts are using this the "accumulated" is prefered
	 */
	u32 regv;
	int mutex_req;

	mutex_req = !in_interrupt();
	if (mutex_req)
		mutex_lock(&ptp_clock_in_cntr_mutex);

	regv = mv_pp3_ptp_reg_read(MV_TAI_INCOMING_CLOCKIN_CNTING_CFG_LOW_REG);
	regv &= MV_TAI_INCOMING_CLOCKIN_CNTING_CFG_LOW_CLOCK_CNTR_BITS_0_15_MASK;
	ptp_tai_clock_in_cntr += regv;
	if (accumulated)
		*accumulated = ptp_tai_clock_in_cntr;

	if (mutex_req)
		mutex_unlock(&ptp_clock_in_cntr_mutex);
	return (u16)regv;
}

void mv_pp3_tai_set_nop(void)
{
	u32 regv;
	regv = mv_pp3_ptp_reg_read(MV_TAI_TIME_CNTR_FUNC_CFG_0_REG);
	regv = MV_TAI_CNTR_TIME_FUNC_BITSET(MV_TAI_NOP, regv);
	mv_pp3_ptp_reg_write(MV_TAI_TIME_CNTR_FUNC_CFG_0_REG, regv);
}

void mv_pp3_tai_clock_cfg_external(bool from_external)
{
	/* "external" is from GPS vs free-running/Generate */
	u32 regv, mghz_tclk, tclk_step_nsec, clock_mode;

	mv_pp3_tai_clock_stable_status_set(0);

#ifdef MV_PP3_DEDICATED_MG_REGION
	/* Configure Register access (multiple call is safe) */
	a390_addr_completion_fixed_init(MV_PP3_DEDICATED_MG_REGION, MV_PP3_PTP_TAI_UNIT_OFFSET);
#endif
	/* Init, Activate TAI Clock RESET (set bit=0) */
	regv = mv_pp3_ptp_reg_read(MV_TAI_CTRL_REG0_REG);
	regv = MV_SET_BIT(regv, MV_TAI_CTRL_REG0_SW_RESET_OFFS, 0);
	mv_pp3_ptp_reg_write(MV_TAI_CTRL_REG0_REG, regv);

	/* Set clock step (e.g. 4nsec for frequency 250MHz) */
	if (ptp_tclk_hz) {
		mghz_tclk  = ptp_tclk_hz / 1000000;
		tclk_step_nsec = 1000 / mghz_tclk;
	} else {
		/* Set value good for succesfull bootup but definitelly invalid for TAI */
		tclk_step_nsec = 200;
		WARN_ONCE(1, "%s: Tclock is not initialized by .DTB\n", PTP_TAI_PRT_STR);
	}
	mv_pp3_ptp_reg_write(MV_TAI_TOD_STEP_NANO_CFG_REG, tclk_step_nsec);

	/* Generate 1PPS-Out cycle = 1 sec */
	mv_pp3_ptp_reg_write(MV_TAI_CLOCK_CYCLE_CFG_HIGH_REG, 0x4000);
	mv_pp3_ptp_reg_write(MV_TAI_CLOCK_CYCLE_CFG_LOW_REG, 0);

	/* Configure TAI clock: */
	regv = 0;
	regv = MV_TAI_CNTR_TIME_FUNC_BITSET(MV_TAI_NOP, regv);
	/* MV_TAI_TIME_CNTR_FUNC_CFG_0_INT_CLOCK_GENERATOR_EN keep disable */
	clock_mode = (from_external) ? 2 : 1; /* 2=Reception : 1=Generate */
	MV_U32_SET_FIELD(regv, MV_TAI_TIME_CNTR_FUNC_CFG_0_CLOCK_MODE_MASK,
		clock_mode << MV_TAI_TIME_CNTR_FUNC_CFG_0_CLOCK_MODE_OFFS);
	regv = MV_SET_BIT(regv, MV_TAI_TIME_CNTR_FUNC_CFG_0_PCLK_COUNTER_START_OFFS, 1);
	regv = MV_SET_BIT(regv, MV_TAI_TIME_CNTR_FUNC_CFG_0_CAPTURE_OVERWRITE_EN_OFFS, 1);
	mv_pp3_ptp_reg_write(MV_TAI_TIME_CNTR_FUNC_CFG_0_REG, regv);

	/* start clock - Deactivate TAI Clock RESET (by set bit=1) */
	regv = mv_pp3_ptp_reg_read(MV_TAI_CTRL_REG0_REG);
	regv = MV_SET_BIT(regv, MV_TAI_CTRL_REG0_SW_RESET_OFFS, 1);
	mv_pp3_ptp_reg_write(MV_TAI_CTRL_REG0_REG, regv);
	ptp_tai_clock_enabled = true;
	ptp_tai_clock_external_cfg = from_external;
	if (!from_external)
		mv_pp3_tai_clock_stable_status_set(1);
}

void mv_pp3_tai_clock_disable(void)
{
	u32 regv;
	mv_pp3_tai_clock_stable_status_set(0);
	/* stop clock by placing TAI into RESET state */
	regv = mv_pp3_ptp_reg_read(MV_TAI_CTRL_REG0_REG);
	regv = MV_SET_BIT(regv, MV_TAI_CTRL_REG0_SW_RESET_OFFS, 0);
	mv_pp3_ptp_reg_write(MV_TAI_CTRL_REG0_REG, regv);
	ptp_tai_clock_enabled = false;
	mv_pp3_tai_clock_in_cntr_get(NULL); /*reset the HW counter */
	ptp_tai_clock_in_cntr = 0; /* reset the SW accumulated */
}

void mv_pp3_tai_clock_init(struct platform_device *pdev)
{
	/* Enable counting ClockIn - once forever (for Internal&External) */
	mv_pp3_ptp_reg_write(MV_TAI_INCOMING_CLOCKIN_CNTING_EN_REG, 1);

	ptp_tai_clock_external_cfg = mv_pp3_tai_clock_external_init(pdev);
	mv_pp3_tai_clock_cfg_external(ptp_tai_clock_external_cfg); /*hw here*/
	mv_pp3_tai_clock_external_init2(ptp_tai_clock_external_cfg);
}


/* ------------------------------------------------------------*/
/* -------   PTP timestamp  (hw-level)   ----------------------*/
/* ------------------------------------------------------------*/
void mv_pp3_ptp_reset(int port)
{
	u32 reg_data;
	if (port >= MV_PP3_EMAC_NUM) /* Could be from sysfs command */
		return;
	if (!(ptp_ports_enabled & (1 << port)))
		return;

	reg_data = mv_pp3_ptp_reg_read(MV_PTP_GENERAL_CTRL_REG(port));
		/* activate reset */
	reg_data &= ~MV_PTP_GENERAL_CTRL_PTP_RESET_MASK;
	mv_pp3_ptp_reg_write(MV_PTP_GENERAL_CTRL_REG(port), reg_data);
	udelay(1);
		/* unreset unit */
	reg_data |= MV_PTP_GENERAL_CTRL_PTP_RESET_MASK;
	mv_pp3_ptp_reg_write(MV_PTP_GENERAL_CTRL_REG(port), reg_data);
}

void mv_pp3_ptp_reset_all_ptp_ports(void)
{
	/* Required for TAI-Clock-External sync down-up recovery */
	int port;
	for (port = 0; port < MV_PP3_EMAC_NUM; port++)
		mv_pp3_ptp_reset(port); /* only enabled would be reset */
}

int mv_pp3_ptp_enable(int port, bool enable)
{
	u32 reg_data;

	if (port >= MV_PP3_EMAC_NUM) {
		/* Could be from sysfs command */
		pr_info("%s: wrong port number %d\n", PTP_TAI_PRT_STR, port);
		return -1;
	}
	if (enable) {
		ptp_ports_enabled |= 1 << port;
		reg_data = mv_pp3_ptp_reg_read(MV_PTP_GENERAL_CTRL_REG(port));
		reg_data |= MV_PTP_GENERAL_CTRL_PTP_UNIT_ENABLE_MASK;
		reg_data |= MV_PTP_GENERAL_CTRL_TS_QUEUE_OVER_WRITE_ENABLE_MASK;
		mv_pp3_ptp_reg_write(MV_PTP_GENERAL_CTRL_REG(port), reg_data);

		/* unreset PTP unit */
		reg_data |= MV_PTP_GENERAL_CTRL_PTP_RESET_MASK;
		mv_pp3_ptp_reg_write(MV_PTP_GENERAL_CTRL_REG(port), reg_data);

		mv_pp3_ptp_reset(port);
		mv_ptp_hook_enable(port, true);
	} else {
		ptp_ports_enabled &= ~(1 << port);
		mv_ptp_hook_enable(port, false);
		reg_data = mv_pp3_ptp_reg_read(MV_PTP_GENERAL_CTRL_REG(port));
		reg_data &= ~MV_PTP_GENERAL_CTRL_PTP_UNIT_ENABLE_MASK;
		/* disable PTP */
		mv_pp3_ptp_reg_write(MV_PTP_GENERAL_CTRL_REG(port), reg_data);
	}
	return 0;
}

/*****************************************************************************/
/*********    UP-level utilities     *****************************************/
/*****************************************************************************/
int mv_pp3_tai_tod_op_read_captured(struct mv_pp3_tai_tod *ts, u32 *p_status)
{
	/* Called as Global after synched GET_CAPTURE request */
	const u32 wait_max = 8;
	u32 wait = wait_max;
	u32 status, tmp;

	if (p_status) {
		/* Check/wait for ready */
		do {
			if (!wait--) {
				pr_err("%s: cannot read tod timestamp after %d retries\n",
					PTP_TAI_PRT_STR, wait_max);
				return -1;
			}
			status = mv_pp3_ptp_reg_read(MV_TAI_TIME_CAPTURE_STATUS_REG);
		} while (!(status & MV_TAI_TIME_CAPTURE_STATUS_CAPTURE_0_VALID_MASK));
	} else {
		status = mv_pp3_ptp_reg_read(MV_TAI_TIME_CAPTURE_STATUS_REG);
		/* Go ahead even with status=0 */
	}

	/* SEC_HIGH and FRAC are always zero but read needed to unlock the hw-queue */
	ts->sec_msb_16b = mv_pp3_ptp_reg_read(MV_TAI_TIME_CAPTURE_VALUE_0_SEC_HIGH_REG)/*& 0xFFFF*/;
	ts->sec_lsb_32b = (mv_pp3_ptp_reg_read(MV_TAI_TIME_CAPTURE_VALUE_0_SEC_MED_REG) << 16)
			| (mv_pp3_ptp_reg_read(MV_TAI_TIME_CAPTURE_VALUE_0_SEC_LOW_REG) & 0xFFFF);
	ts->nsec = (mv_pp3_ptp_reg_read(MV_TAI_TIME_CAPTURE_VALUE_0_NANO_HIGH_REG) << 16)
		| (mv_pp3_ptp_reg_read(MV_TAI_TIME_CAPTURE_VALUE_0_NANO_LOW_REG) & 0xFFFF);
	ts->nfrac = (mv_pp3_ptp_reg_read(MV_TAI_TIME_CAPTURE_VALUE_0_FRAC_HIGH_REG) << 16)
		| (mv_pp3_ptp_reg_read(MV_TAI_TIME_CAPTURE_VALUE_0_FRAC_LOW_REG) & 0xFFFF);

	if (status > 1) {
		/* flush capture-fifo */
		tmp = mv_pp3_ptp_reg_read(MV_TAI_TIME_CAPTURE_VALUE_1_SEC_HIGH_REG)/*& 0xFFFF*/;
		tmp = (mv_pp3_ptp_reg_read(MV_TAI_TIME_CAPTURE_VALUE_1_SEC_MED_REG) << 16)
				| (mv_pp3_ptp_reg_read(MV_TAI_TIME_CAPTURE_VALUE_1_SEC_LOW_REG) & 0xFFFF);
		tmp = (mv_pp3_ptp_reg_read(MV_TAI_TIME_CAPTURE_VALUE_1_NANO_HIGH_REG) << 16)
			| (mv_pp3_ptp_reg_read(MV_TAI_TIME_CAPTURE_VALUE_1_NANO_LOW_REG) & 0xFFFF);
		tmp = (mv_pp3_ptp_reg_read(MV_TAI_TIME_CAPTURE_VALUE_1_FRAC_HIGH_REG) << 16)
			| (mv_pp3_ptp_reg_read(MV_TAI_TIME_CAPTURE_VALUE_1_FRAC_LOW_REG) & 0xFFFF);
		if (p_status)
			pr_info("ptp/tai: capture-0 loss and taken to capture-1\n");
	}
	return 0;
}

int mv_pp3_tai_tod_op(enum mv_pp3_tai_ptp_op op, struct mv_pp3_tai_tod *ts,
			int synced_op)
{
	u32 ctrl, ctrl_new, status;
	int rc = 0, mutex_req;
	bool keep_last_op = (bool)synced_op;
	u32 trigger_bit = synced_op ? 0 : 1;

	if (unlikely((u32)op >= MV_TAI_PTP_SW_OP)) {
		if (op == MV_TAI_TO_LINUX) {
			mv_pp3_tai_tod_to_linux(ts);
			return 0;
		} else if (op == MV_TAI_FROM_LINUX) {
			mv_pp3_tai_tod_from_linux(ts);
			return 0;
		} else {
			pr_err("%s: wrong operation=%d requested\n", PTP_TAI_PRT_STR, op);
			return -1;
		}
	}

	mutex_req = !in_interrupt();
	if (mutex_req)
		mutex_lock(&ptp_op_mutex);

	if (unlikely(op == MV_TAI_NOP)) {
		ctrl = mv_pp3_ptp_reg_read(MV_TAI_TIME_CNTR_FUNC_CFG_0_REG);
		ctrl_new = MV_TAI_CNTR_TIME_FUNC_BITSET(op, ctrl);
		goto exit;
	}

	if (op == MV_TAI_GET_CAPTURE) {
		/* GET-exec first, then Read from CAPTURE */
		ctrl = mv_pp3_ptp_reg_read(MV_TAI_TIME_CNTR_FUNC_CFG_0_REG);
		ctrl_new = MV_TAI_CNTR_TIME_FUNC_BITSET(op, ctrl);
		mv_pp3_ptp_reg_write(MV_TAI_TIME_CNTR_FUNC_CFG_0_REG, ctrl_new | trigger_bit);
		/* Check "ready" and read captured into ts */
		if (!synced_op)
			rc  = mv_pp3_tai_tod_op_read_captured(ts, &status);
	} else {
		/* All operations are "setting": load SHADOW first */
		mv_pp3_ptp_reg_write(MV_TAI_TIME_LOAD_VALUE_SEC_MED_REG, ts->sec_lsb_32b >> 16);
		mv_pp3_ptp_reg_write(MV_TAI_TIME_LOAD_VALUE_SEC_LOW_REG, ts->sec_lsb_32b & 0xFFFF);
		mv_pp3_ptp_reg_write(MV_TAI_TIME_LOAD_VALUE_NANO_HIGH_REG, ts->nsec >> 16);
		mv_pp3_ptp_reg_write(MV_TAI_TIME_LOAD_VALUE_NANO_LOW_REG, ts->nsec & 0xFFFF);
		mv_pp3_ptp_reg_write(MV_TAI_TIME_LOAD_VALUE_SEC_HIGH_REG, ts->sec_msb_16b & 0xFFFF);
		mv_pp3_ptp_reg_write(MV_TAI_TIME_LOAD_VALUE_FRAC_HIGH_REG, ts->nfrac >> 16);
		mv_pp3_ptp_reg_write(MV_TAI_TIME_LOAD_VALUE_FRAC_LOW_REG, ts->nfrac & 0xFFFF);

		/* Lock-exec new SHADOW/LOAD into tai-tod */
		ctrl = mv_pp3_ptp_reg_read(MV_TAI_TIME_CNTR_FUNC_CFG_0_REG);
		ctrl_new = MV_TAI_CNTR_TIME_FUNC_BITSET(op, ctrl);
		mv_pp3_ptp_reg_write(MV_TAI_TIME_CNTR_FUNC_CFG_0_REG, ctrl_new | trigger_bit);
	}

exit:
	if ((!keep_last_op) && (ctrl_new != ctrl)) /* restore original */
		mv_pp3_ptp_reg_write(MV_TAI_TIME_CNTR_FUNC_CFG_0_REG, ctrl);

	if (mutex_req)
		mutex_unlock(&ptp_op_mutex);
	return rc;
}
