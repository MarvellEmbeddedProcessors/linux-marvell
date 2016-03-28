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

/***************************************************************
 * This file contains:  UP-level utilities used by sysfs
 ***************************************************************
 */

/* includes */
#include <linux/kernel.h>
#include "gop/mv_gop_if.h"
#include "gop/mv_ptp_regs.h"
#include "gop/mv_tai_regs.h"
#include "gop/mv_ptp_if.h"
#include "net_dev/mv_ptp_service.h"

static void mv_pp3_tai_tod_from_kernel(int prt_off_on_extend);
static void mv_pp3_tai_tod_and_kernel_print(int prt_off_on_extend);

int mv_pp3_tai_tod_load_set(u32 sec_h, u32 sec_l, u32 nano, u32 op)
{
	/* Generic TAI TOD operationS */
	struct mv_pp3_tai_tod ts;
	u32 value;

	ts.sec_msb_16b = sec_h;
	ts.sec_lsb_32b = sec_l;
	ts.nsec = nano;
	ts.nfrac = 0;

	switch (op) {
	/* LOAD-SET ToD with sec_high, sec_low, nsec */
	case 0:
		mv_pp3_tai_tod_op(MV_TAI_SET_UPDATE, &ts, 0);
		break;

	/* SYNC ToD time from/to linux or Sys/kernel */
	case 0x21:
		mv_pp3_tai_tod_op(MV_TAI_TO_LINUX, &ts, 0);
		break;
	case 0x41:
		mv_pp3_tai_tod_op(MV_TAI_FROM_LINUX, &ts, 0);
		break;
	case 0x45:
		/* Do twice. First is for cache loading */
		mv_pp3_tai_tod_from_kernel(0);
		mv_pp3_tai_tod_from_kernel(2);
		break;
	case 0x46:
		mv_pp3_tai_tod_and_kernel_print(0);
		mv_pp3_tai_tod_and_kernel_print(1);
		break;
	case 0x47:
		mv_pp3_tai_tod_and_kernel_print(0);
		mv_pp3_tai_tod_and_kernel_print(2);
		break;

	/* ToD time Increment/Decrement */
	case 0x1c:
		mv_pp3_tai_tod_op(MV_TAI_INCREMENT, &ts, 0);
		break;
	case 0xdc:
		mv_pp3_tai_tod_op(MV_TAI_DECREMENT, &ts, 0);
		break;
	case 0x1c0:
		mv_pp3_tai_tod_op(MV_TAI_INCREMENT_GRACEFUL, &ts, 0);
		break;
	case 0xdc0:
		mv_pp3_tai_tod_op(MV_TAI_DECREMENT_GRACEFUL, &ts, 0);
		break;

	/* FREQ update */
	case 0xf1c:
		ts.sec_msb_16b = 0;
		ts.sec_lsb_32b = 0;
		ts.nsec = 0;
		ts.nfrac = sec_h;
		mv_pp3_tai_tod_op(MV_TAI_FREQ_UPDATE, &ts, 0);
		break;
	case 0xfdc:
		ts.sec_msb_16b = 0;
		ts.sec_lsb_32b = 0;
		ts.nsec = 0;
		ts.nfrac = -sec_h;
		mv_pp3_tai_tod_op(MV_TAI_FREQ_UPDATE, &ts, 0);
		break;

	/* TAI/ToD Clock configuration */
	case 0xce1: /* External, Increment [h] seconds */
	case 0xced: /* External, Decrement [h] seconds */
		if (sec_h) { /* ToD Inc/Dec required */
			sec_h = (op == 0xce1) ? sec_h : -sec_h;
			sec_l = 0;
		} else { /* sync but keep current ToD */
			sec_h = 1;
			sec_l = 1;
		}
		mv_pp3_tai_clock_from_external_sync(1, sec_l, sec_h);
		break;
	case 0xcea: /* External, Absolute set [h] seconds */
		mv_pp3_tai_clock_from_external_sync(1, sec_h, 0);
		break;
	case 0xcec: /* Check stability status */
		mv_pp3_tai_clock_in_cntr_get(&value);
		pr_info("TAI/ToD clock: stability=%d, counter=%u\n",
			mv_pp3_tai_clock_stable_status_get(), value);
		break;
	case 0xc1: /* Internal */
		mv_pp3_tai_clock_from_external_sync(0, 0, 0);
		mv_pp3_tai_clock_cfg_external(false);
		break;
	case 0xc0: /* Off */
		mv_pp3_tai_clock_from_external_sync(0, 0, 0);
		mv_pp3_tai_clock_disable();
		break;
	case 0xceb11: /* Blink led on gpio=11 */
		mv_pp3_ptp_event_led_sysfs(11);
		break;

	/* DEBUG with 3 "just generic" param-values passed transparently */
	case 0xdeb:
		mv_ptp_hook_extra_op(sec_h, sec_l, nano);
		break;

	default:
		pr_info("tai_op: Wrong parameter\n"
			"Supported HEX: 0; 21,41,45,46,47; 1c,dc,1c0,dc0; f1c,fdc;"
			" ce1,ceD,ceA,ceC,c1,c0; deb\n");
		return -EINVAL;
	}
	return 0;
}

void mv_pp3_tai_tod_to_linux(struct mv_pp3_tai_tod *ts)
{
	struct timespec tv;
	unsigned long flag;
	int rc;
	rc = mv_pp3_tai_tod_op(MV_TAI_GET_CAPTURE, ts, 0);
	if (rc)
		return;
	tv.tv_sec = ts->sec_lsb_32b;
	tv.tv_nsec = ts->nsec;
	flag = irqs_disabled();
	if (flag) {
		/* write over sysfs runs under IRQ-DISABLE (versus regular sys-call)
		 * To avoid deadlock in settimeofday() between cpuA and cpuB waiting
		 * each to other (refer smp.c:WARN_ON_ONCE) we must enable irq here
		 */
		local_irq_enable();
	}
	do_settimeofday(&tv);
	if (flag)
		local_irq_disable();
}

void mv_pp3_tai_tod_from_linux(struct mv_pp3_tai_tod *ts)
{
	struct timeval tv;
	do_gettimeofday(&tv);
	ts->sec_msb_16b = 0;
	ts->sec_lsb_32b = tv.tv_sec;
	ts->nsec = tv.tv_usec * 1000;
	mv_pp3_tai_tod_op(MV_TAI_SET_UPDATE, ts, 0);
}

static void mv_pp3_tai_tod_from_kernel(int prt_off_on_extend)
{
	struct mv_pp3_tai_tod ts;
	u64 cl;
	u32 sec, nsec;

	ts.sec_msb_16b = 0;
	ts.nfrac = 0;

	cl = local_clock();
	sec = (u32)div_u64_rem(cl, 1000000000, &nsec);

	ts.sec_lsb_32b = sec;
	ts.nsec = nsec;
	mv_pp3_tai_tod_op(MV_TAI_SET_UPDATE, &ts, 0);
	if (prt_off_on_extend) {
		/* Adjust delta between K-get and ToD real set
		 * With double exec silent=1/0 (for caching) and
		 * adjust value 0x200 the +/- 0x14 deviation found
		 * "Cached" ToD read/write takes >4200nsec
		 */
		ts.nsec = 0x200;
		ts.sec_lsb_32b = 0;
		mv_pp3_tai_tod_op(MV_TAI_INCREMENT, &ts, 0);
	}
	mv_pp3_tai_tod_and_kernel_print(prt_off_on_extend);
}

static void mv_pp3_tai_tod_and_kernel_print(int prt_off_on_extend)
{
	struct mv_pp3_tai_tod ts;
	u64 cl[2], tod64;
	u32 sec[2], nsec[2], d;
	char sign;

	cl[0] = local_clock();
	mv_pp3_tai_tod_op(MV_TAI_GET_CAPTURE, &ts, 0);
	cl[1] = local_clock();

	if (!prt_off_on_extend) /* only cache adjust requested */
		return;

	tod64 = ts.sec_lsb_32b * 1000000000 + ts.nsec;
	pr_info("  Kclock - ToD = %d\n", (int)(cl[0] - tod64));

	if (prt_off_on_extend == 1)
		return;

	/* Account and print the buffered results */
	sec[0] = (u32)div_u64_rem(cl[0], 1000000000, &nsec[0]);
	sec[1] = (u32)div_u64_rem(cl[1], 1000000000, &nsec[1]);
	pr_info("Kclock=%u.%09u  ToD=%u.%09u  Kclock=%u.%09u\n",
		sec[0], nsec[0], ts.sec_lsb_32b, ts.nsec, sec[1], nsec[1]);

	if (nsec[0] > ts.nsec) {
		d = nsec[0] - ts.nsec;
		sign = '+';
	} else {
		d = ts.nsec - nsec[0];
		sign = '-';
	}
	pr_info("  hex-delta nsec: (Kclock=%08x) - (ToD=%08x) = %c%04x\n\n",
		nsec[0], ts.nsec, sign, d);
}


/*****************************************************************************/
/*****************************************************************************/
static inline void mv_pp3_ptp_reg_print(char *reg_name, u32 reg)
{
	pr_info("  %-45s: 0x%x = 0x%08x\n", reg_name, MV_PP3_PTP_UNIT_OFFSET + reg, mv_pp3_ptp_reg_read(reg));
}


/* Time Counter Function Configuration 0 Register 0x03180A10
 The field bits[4:2] defines "Time Counter Function" to be performed
  when CPU sets bit_0 "Time Counter Function Trigger"
	0 = Update    : Copies the value FROM the SHADOW timer TO the TIMER register
	1 = FreqUpdate: Copies the value FROM the SHADOW timer TO the fractional nanosecond drift register.
	2= Increment  : Adds the value of the shadow timer to the timer register.
	3= Decrement  : Subtracts the value of the shadow timer from the timer register.
	4= Capture    : Copies the value of the TIMER TO the SHADOW timer register.
	5= GracefulInc: Gracefully increment the value of the shadow timer to the timer.
	6= GracefulDec: Gracefully decrement the value of the shadow timer from the timer.
	7= NOP        : No operation is performed

  There are 2 sets of CAPTURE registers: VALUE_0_xx and VALUE_1_xx, and
  "Capture Status Register" 0x03180AA4 bit_0 and bit_1 reflecting which one (or both)
  is ready and waiting for read. The Status-bit is auto-cleared after VALUE_n_xx reading.
  The Capture and Status behavior also depends upon 0x03180A10 bit6 "Capture Overwrite":
  With 0x03180A10[6]=0 the Status=[1][1] would go tp [0][0] upon "new capture before reading".
*/
void mv_pp3_tai_tod_dump_util(struct mv_pp3_tai_tod *ts)
{
	u32 high, low, med;

	if (!ts->sec_msb_16b)
		pr_info("  %-45s: 0x%08x (%u)\n", "TIME_CAPTURE_VALUE_0_SEC", ts->sec_lsb_32b, ts->sec_lsb_32b);
	else
		pr_info("  %-45s: 0x%04x%08x\n", "TIME_CAPTURE_VALUE_0_SEC", ts->sec_msb_16b, ts->sec_lsb_32b);

	pr_info("  %-45s: 0x%08x (%u)\n", "TIME_CAPTURE_VALUE_0_NANO", ts->nsec, ts->nsec);

	high = mv_pp3_ptp_reg_read(MV_TAI_TIME_CAPTURE_VALUE_0_FRAC_HIGH_REG);
	low = mv_pp3_ptp_reg_read(MV_TAI_TIME_CAPTURE_VALUE_0_FRAC_LOW_REG);
	pr_info("  %-45s: 0x%08x\n", "TIME_CAPTURE_VALUE_0_FRAC", ((high << 16) + low));

	high = mv_pp3_ptp_reg_read(MV_TAI_TIME_CAPTURE_VALUE_1_SEC_HIGH_REG);
	med = mv_pp3_ptp_reg_read(MV_TAI_TIME_CAPTURE_VALUE_1_SEC_MED_REG);
	low = mv_pp3_ptp_reg_read(MV_TAI_TIME_CAPTURE_VALUE_1_SEC_LOW_REG);
	if (!high)
		pr_info("  %-45s: 0x%08x\n", "TIME_CAPTURE_VALUE_1_SEC", ((med << 16) + low));
	else
		pr_info("  %-45s: 0x%08x%08x\n", "TIME_CAPTURE_VALUE_1_SEC", high, ((med << 16) + low));

	high = mv_pp3_ptp_reg_read(MV_TAI_TIME_CAPTURE_VALUE_1_NANO_HIGH_REG);
	low = mv_pp3_ptp_reg_read(MV_TAI_TIME_CAPTURE_VALUE_1_NANO_LOW_REG);
	pr_info("  %-45s: 0x%08x\n", "TIME_CAPTURE_VALUE_1_NANO", ((high << 16) + low));

	high = mv_pp3_ptp_reg_read(MV_TAI_TIME_CAPTURE_VALUE_1_FRAC_HIGH_REG);
	low = mv_pp3_ptp_reg_read(MV_TAI_TIME_CAPTURE_VALUE_1_FRAC_LOW_REG);
	pr_info("  %-45s: 0x%08x\n", "TIME_CAPTURE_VALUE_1_FRAC", ((high << 16) + low));
}

void mv_pp3_tai_tod_dump(void)
{
	struct mv_pp3_tai_tod ts;
	mv_pp3_tai_tod_op(MV_TAI_GET_CAPTURE, &ts, 0);
	mv_pp3_tai_tod_dump_util(&ts);
}

void mv_pp3_ptp_reg_dump(int port)
{
	/* Read interrupt-status clears it. Disable this print if IRQ-handler to be used */
	mv_pp3_ptp_reg_print("PTP_INT_STATUS_TS", MV_PTP_INT_STATUS_TS_REG(port));

	mv_pp3_ptp_reg_print("PTP_GENERAL_CTRL", MV_PTP_GENERAL_CTRL_REG(port));
	mv_pp3_ptp_reg_print("TOTAL_PTP_PCKTS_CNTR", MV_PTP_TOTAL_PTP_PCKTS_CNTR_REG(port));
	mv_pp3_ptp_reg_print("PTPV1_PCKT_CNTR", MV_PTP_PTPV1_PCKT_CNTR_REG(port));
	mv_pp3_ptp_reg_print("PTPV2_PCKT_CNTR", MV_PTP_PTPV2_PCKT_CNTR_REG(port));
	mv_pp3_ptp_reg_print("Y1731_PCKT_CNTR", MV_PTP_Y1731_PCKT_CNTR_REG(port));
	mv_pp3_ptp_reg_print("NTPTS_PCKT_CNTR", MV_PTP_NTPTS_PCKT_CNTR_REG(port));
	mv_pp3_ptp_reg_print("NTPRECEIVE_PCKT_CNTR", MV_PTP_NTPRECEIVE_PCKT_CNTR_REG(port));
	mv_pp3_ptp_reg_print("NTPTRANSMIT_PCKT_CNTR", MV_PTP_NTPTRANSMIT_PCKT_CNTR_REG(port));
	mv_pp3_ptp_reg_print("WAMP_PCKT_CNTR", MV_PTP_WAMP_PCKT_CNTR_REG(port));
	mv_pp3_ptp_reg_print("NONE_ACTION_PCKT_CNTR", MV_PTP_NONE_ACTION_PCKT_CNTR_REG(port));
	mv_pp3_ptp_reg_print("FORWARD_ACTION_PCKT_CNTR", MV_PTP_FORWARD_ACTION_PCKT_CNTR_REG(port));
	mv_pp3_ptp_reg_print("DROP_ACTION_PCKT_CNTR", MV_PTP_DROP_ACTION_PCKT_CNTR_REG(port));
	mv_pp3_ptp_reg_print("CAPTURE_ACTION_PCKT_CNTR", MV_PTP_CAPTURE_ACTION_PCKT_CNTR_REG(port));
	mv_pp3_ptp_reg_print("ADD_TIME_ACTION_PCKT_CNTR", MV_PTP_ADDTIME_ACTION_PCKT_CNTR_REG(port));
	mv_pp3_ptp_reg_print("ADD_CORRECT_TIME_ACTION_PCKT_CNTR", MV_PTP_ADDCORRECTEDTIME_ACTION_PCKT_CNTR_REG(port));
	mv_pp3_ptp_reg_print("CAPTURE_TIME_ACTION_PCKT_CNTR", MV_PTP_CAPTUREADDTIME_ACTION_PCKT_CNTR_REG(port));
	mv_pp3_ptp_reg_print("CAPTURE_CORRECT_TIME_ACTION_PCKT_CNTR",
		MV_PTP_CAPTUREADDCORRECTEDTIME_ACTION_PCKT_CNTR_REG(port));
	mv_pp3_ptp_reg_print("ADD_INGRESS_TIME_ACTION_PCKT_CNTR", MV_PTP_ADDINGRESSTIME_ACTION_PCKT_CNTR_REG(port));
	mv_pp3_ptp_reg_print("CAPTURE_ADD_INGRESS_TIME_ACTION_PCKT_CNTR",
		MV_PTP_CAPTUREADDINGRESSTIME_ACTION_PCKT_CNTR_REG(port));
	mv_pp3_ptp_reg_print("CAPTURE_INGRESS_TIME_ACTION_PCKT_CNTR",
		MV_PTP_CAPTUREINGRESSTIME_ACTION_PCKT_CNTR_REG(port));
	mv_pp3_ptp_reg_print("NTP_PTP_OFFSET_HIGH", MV_PTP_NTP_PTP_OFFSET_HIGH_REG(port));
	mv_pp3_ptp_reg_print("NTP_PTP_OFFSET_LOW", MV_PTP_NTP_PTP_OFFSET_LOW_REG(port));
}

void mv_pp3_tai_reg_dump(void)
{
	mv_pp3_ptp_reg_print("TAI_CTRL_REG0", MV_TAI_CTRL_REG0_REG);
	mv_pp3_ptp_reg_print("TAI_CTRL_REG1", MV_TAI_CTRL_REG1_REG);
	mv_pp3_ptp_reg_print("TIME_CNTR_FUNC_CFG_0", MV_TAI_TIME_CNTR_FUNC_CFG_0_REG);
	mv_pp3_ptp_reg_print("TIME_CNTR_FUNC_CFG_1", MV_TAI_TIME_CNTR_FUNC_CFG_1_REG);
	mv_pp3_ptp_reg_print("TIME_CNTR_FUNC_CFG_2", MV_TAI_TIME_CNTR_FUNC_CFG_2_REG);
	mv_pp3_ptp_reg_print("FREQUENCY_ADJUST_TIME_WINDOW", MV_TAI_FREQUENCY_ADJUST_TIME_WINDOW_REG);
	mv_pp3_ptp_reg_print("TOD_STEP_NANO_CFG", MV_TAI_TOD_STEP_NANO_CFG_REG);
	mv_pp3_ptp_reg_print("TOD_STEP_FRAC_CFG_HIGH", MV_TAI_TOD_STEP_FRAC_CFG_HIGH_REG);
	mv_pp3_ptp_reg_print("TOD_STEP_FRAC_CFG_LOW", MV_TAI_TOD_STEP_FRAC_CFG_LOW_REG);
	mv_pp3_ptp_reg_print("TIME_ADJUSTMENT_PROPAGATION_DELAY_CFG_HIGH",
		MV_TAI_TIME_ADJUSTMENT_PROPAGATION_DELAY_CFG_HIGH_REG);
	mv_pp3_ptp_reg_print("TIME_ADJUSTMENT_PROPAGATION_DELAY_CFG_LOW",
		MV_TAI_TIME_ADJUSTMENT_PROPAGATION_DELAY_CFG_LOW_REG);
	mv_pp3_ptp_reg_print("TRIGGER_GENERATION_TOD_SEC_HIGH", MV_TAI_TRIGGER_GENERATION_TOD_SEC_HIGH_REG);
	mv_pp3_ptp_reg_print("TRIGGER_GENERATION_TOD_SEC_MED", MV_TAI_TRIGGER_GENERATION_TOD_SEC_MED_REG);
	mv_pp3_ptp_reg_print("TRIGGER_GENERATION_TOD_SEC_LOW", MV_TAI_TRIGGER_GENERATION_TOD_SEC_LOW_REG);
	mv_pp3_ptp_reg_print("TRIGGER_GENERATION_TOD_NANO_HIGH", MV_TAI_TRIGGER_GENERATION_TOD_NANO_HIGH_REG);
	mv_pp3_ptp_reg_print("TRIGGER_GENERATION_TOD_NANO_LOW", MV_TAI_TRIGGER_GENERATION_TOD_NANO_LOW_REG);
	mv_pp3_ptp_reg_print("TRIGGER_GENERATION_TOD_FRAC_HIGH", MV_TAI_TRIGGER_GENERATION_TOD_FRAC_HIGH_REG);
	mv_pp3_ptp_reg_print("TRIGGER_GENERATION_TOD_FRAC_LOW", MV_TAI_TRIGGER_GENERATION_TOD_FRAC_LOW_REG);
	mv_pp3_ptp_reg_print("TIME_LOAD_VALUE_SEC_HIGH", MV_TAI_TIME_LOAD_VALUE_SEC_HIGH_REG);
	mv_pp3_ptp_reg_print("TIME_LOAD_VALUE_SEC_MED", MV_TAI_TIME_LOAD_VALUE_SEC_MED_REG);
	mv_pp3_ptp_reg_print("TIME_LOAD_VALUE_SEC_LOW", MV_TAI_TIME_LOAD_VALUE_SEC_LOW_REG);
	mv_pp3_ptp_reg_print("TIME_LOAD_VALUE_NANO_HIGH", MV_TAI_TIME_LOAD_VALUE_NANO_HIGH_REG);
	mv_pp3_ptp_reg_print("TIME_LOAD_VALUE_NANO_LOW", MV_TAI_TIME_LOAD_VALUE_NANO_LOW_REG);
	mv_pp3_ptp_reg_print("TIME_LOAD_VALUE_FRAC_HIGH", MV_TAI_TIME_LOAD_VALUE_FRAC_HIGH_REG);
	mv_pp3_ptp_reg_print("TIME_LOAD_VALUE_FRAC_LOW", MV_TAI_TIME_LOAD_VALUE_FRAC_LOW_REG);

	/* Read STATUS first since it is cleared after VALUExx reading */
	mv_pp3_ptp_reg_print("TIME_CAPTURE_STATUS", MV_TAI_TIME_CAPTURE_STATUS_REG);
	mv_pp3_ptp_reg_print("TIME_CAPTURE_VALUE_0_SEC_HIGH", MV_TAI_TIME_CAPTURE_VALUE_0_SEC_HIGH_REG);
	mv_pp3_ptp_reg_print("TIME_CAPTURE_VALUE_0_SEC_MED", MV_TAI_TIME_CAPTURE_VALUE_0_SEC_MED_REG);
	mv_pp3_ptp_reg_print("TIME_CAPTURE_VALUE_0_SEC_LOW", MV_TAI_TIME_CAPTURE_VALUE_0_SEC_LOW_REG);
	mv_pp3_ptp_reg_print("TIME_CAPTURE_VALUE_0_NANO_HIGH", MV_TAI_TIME_CAPTURE_VALUE_0_NANO_HIGH_REG);
	mv_pp3_ptp_reg_print("TIME_CAPTURE_VALUE_0_NANO_LOW", MV_TAI_TIME_CAPTURE_VALUE_0_NANO_LOW_REG);
	mv_pp3_ptp_reg_print("TIME_CAPTURE_VALUE_0_FRAC_HIGH", MV_TAI_TIME_CAPTURE_VALUE_0_FRAC_HIGH_REG);
	mv_pp3_ptp_reg_print("TIME_CAPTURE_VALUE_0_FRAC_LOW", MV_TAI_TIME_CAPTURE_VALUE_0_FRAC_LOW_REG);
	mv_pp3_ptp_reg_print("TIME_CAPTURE_VALUE_1_SEC_HIGH", MV_TAI_TIME_CAPTURE_VALUE_1_SEC_HIGH_REG);
	mv_pp3_ptp_reg_print("TIME_CAPTURE_VALUE_1_SEC_MED", MV_TAI_TIME_CAPTURE_VALUE_1_SEC_MED_REG);
	mv_pp3_ptp_reg_print("TIME_CAPTURE_VALUE_1_SEC_LOW", MV_TAI_TIME_CAPTURE_VALUE_1_SEC_LOW_REG);
	mv_pp3_ptp_reg_print("TIME_CAPTURE_VALUE_1_NANO_HIGH", MV_TAI_TIME_CAPTURE_VALUE_1_NANO_HIGH_REG);
	mv_pp3_ptp_reg_print("TIME_CAPTURE_VALUE_1_NANO_LOW", MV_TAI_TIME_CAPTURE_VALUE_1_NANO_LOW_REG);
	mv_pp3_ptp_reg_print("TIME_CAPTURE_VALUE_1_FRAC_HIGH", MV_TAI_TIME_CAPTURE_VALUE_1_FRAC_HIGH_REG);
	mv_pp3_ptp_reg_print("TIME_CAPTURE_VALUE_1_FRAC_LOW", MV_TAI_TIME_CAPTURE_VALUE_1_FRAC_LOW_REG);

	mv_pp3_ptp_reg_print("TIME_UPDATE_CNTR_LSB", MV_TAI_TIME_UPDATE_CNTR_LSB_REG);
	mv_pp3_ptp_reg_print("GENERATE_FUNCTION_MASK_SEC_HIGH", MV_TAI_GENERATE_FUNCTION_MASK_SEC_HIGH_REG);
	mv_pp3_ptp_reg_print("GENERATE_FUNCTION_MASK_SEC_MED", MV_TAI_GENERATE_FUNCTION_MASK_SEC_MED_REG);
	mv_pp3_ptp_reg_print("GENERATE_FUNCTION_MASK_SEC_LOW", MV_TAI_GENERATE_FUNCTION_MASK_SEC_LOW_REG);
	mv_pp3_ptp_reg_print("GENERATE_FUNCTION_MASK_NANO_HIGH", MV_TAI_GENERATE_FUNCTION_MASK_NANO_HIGH_REG);
	mv_pp3_ptp_reg_print("GENERATE_FUNCTION_MASK_NANO_LOW", MV_TAI_GENERATE_FUNCTION_MASK_NANO_LOW_REG);
	mv_pp3_ptp_reg_print("GENERATE_FUNCTION_MASK_FRAC_HIGH", MV_TAI_GENERATE_FUNCTION_MASK_FRAC_HIGH_REG);
	mv_pp3_ptp_reg_print("GENERATE_FUNCTION_MASK_FRAC_LOW", MV_TAI_GENERATE_FUNCTION_MASK_FRAC_LOW_REG);
	mv_pp3_ptp_reg_print("DRIFT_ADJUSTMENT_CFG_HIGH", MV_TAI_DRIFT_ADJUSTMENT_CFG_HIGH_REG);
	mv_pp3_ptp_reg_print("DRIFT_ADJUSTMENT_CFG_LOW", MV_TAI_DRIFT_ADJUSTMENT_CFG_LOW_REG);
	mv_pp3_ptp_reg_print("CAPTURE_TRIGGER_CNTR", MV_TAI_CAPTURE_TRIGGER_CNTR_REG);
	mv_pp3_ptp_reg_print("PCLK_CLOCK_CYCLE_CFG_HIGH", MV_TAI_PCLK_CLOCK_CYCLE_CFG_HIGH_REG);
	mv_pp3_ptp_reg_print("PCLK_CLOCK_CYCLE_CFG_LOW", MV_TAI_PCLK_CLOCK_CYCLE_CFG_LOW_REG);
	mv_pp3_ptp_reg_print("DRIFT_THR_CFG_HIGH", MV_TAI_DRIFT_THR_CFG_HIGH_REG);
	mv_pp3_ptp_reg_print("DRIFT_THR_CFG_LOW", MV_TAI_DRIFT_THR_CFG_LOW_REG);
	mv_pp3_ptp_reg_print("CLOCK_CYCLE_CFG_HIGH", MV_TAI_CLOCK_CYCLE_CFG_HIGH_REG);
	mv_pp3_ptp_reg_print("CLOCK_CYCLE_CFG_LOW", MV_TAI_CLOCK_CYCLE_CFG_LOW_REG);
	mv_pp3_ptp_reg_print("EXT_CLOCK_PROPAGATION_DELAY_CFG_HIGH",
		MV_TAI_EXT_CLOCK_PROPAGATION_DELAY_CFG_HIGH_REG);
	mv_pp3_ptp_reg_print("EXT_CLOCK_PROPAGATION_DELAY_CFG_LOW",
		MV_TAI_EXT_CLOCK_PROPAGATION_DELAY_CFG_LOW_REG);
	mv_pp3_ptp_reg_print("INCOMING_CLOCKIN_CNTING_EN", MV_TAI_INCOMING_CLOCKIN_CNTING_EN_REG);
	mv_pp3_ptp_reg_print("INCOMING_CLOCKIN_CNTING_CFG_LOW", MV_TAI_INCOMING_CLOCKIN_CNTING_CFG_LOW_REG);
	mv_pp3_ptp_reg_print("TIME_UPDATE_CNTR_MSB", MV_TAI_TIME_UPDATE_CNTR_MSB_REG);
	mv_pp3_ptp_reg_print("INCOMING_CLOCKIN_CNTING_CFG_HIGH", MV_TAI_INCOMING_CLOCKIN_CNTING_CFG_HIGH_REG);
}
