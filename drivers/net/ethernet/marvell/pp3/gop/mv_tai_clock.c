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

/**************************************************************************
**
**  This file implements HW->SW workaround
**  needed for sync from External 1PPS signal coming from GPS
**  (e.g. for "Sync TAI clock from GPS" is on PTP GRAND-MASTER system).
**  If PTP GRAND-MASTER is not required, the code has no any impact
**
***************************************************************************
*/

/* includes */
#include <linux/kernel.h>
#include <linux/timer.h>
#include <linux/gpio.h>

#include "gop/mv_ptp_if.h"
#include "net_dev/mv_ptp_service.h"

#include <linux/err.h>
#include <linux/pinctrl/consumer.h>

#define DFLT_CLOCK_IS_EXTERNAL	true /* true or false */

#define MV_PTP_EVENT_MPP_DTB_STRING	"ptp-event-pin"
#define MV_PTP_EVENT_DEV_DTB_STRING	"ptp-event"
#define MV_PTP_1PPS_LED_DTB_STRING	"ptp-1pps-led"

#define MV_PTP_GPS_DOWN2UP_PULSES	2
#define MSEC_1PPS_D(DELTA)	(jiffies + msecs_to_jiffies(1000 + (DELTA)))
#define MSEC_DELAY(MSEC)	(jiffies + msecs_to_jiffies(MSEC))
/* MSEC_BEFORE_1PPS should be shortest but enough for finishing all
 * contexts' jobs (even under high-loaded CPU) before 1PPS occurred
 */
#define MSEC_BEFORE_1PPS	20

enum mv_ptp_event_clock_state {
	PTP_CLK_FREE_RUN = 0,
	PTP_CLK_SYNC_START,
	PTP_CLK_SYNC_START_TIMER,
	PTP_CLK_HW_SYNC,
	PTP_CLK_GPS_UP,
	PTP_CLK_GPS_DOWN,
};

struct mv_ptp_event {
	const char *name;

	struct pinctrl *pinctrl;
	struct pinctrl_state *pin_gpio_state;
	struct pinctrl_state *pin_ptp_state;

	unsigned gpio;
	int gpio2irq;
	bool gpio_irq_engaged;

	enum mv_ptp_event_clock_state state;
	int state_cntr;
	bool gps_up_already_printed;
	struct work_struct work_q;
	struct timer_list timer;

	u32 sec;   /* Absolute set seconds to TAI/ToD */
	int d_sec; /* Relative Inc/Dec delta to TAI/ToD */
	/* if both are 0 - only sync without set requested */
	/* if both are 1 - sync but try to keep current ToD */

	unsigned led;
	int led_val;
};

struct mv_ptp_event mv_ptp_event;


/***************************************************************************/
int mv_pp3_ptp_event_led_sysfs(unsigned led_gpio)
{
	struct mv_ptp_event *ev = &mv_ptp_event;
	if (ev->led) {
		pr_info("%s: GPIO-1PPS led is already on pin %d\n", PTP_TAI_PRT_STR, ev->led);
		return -EINVAL;
	}
	if (!gpio_request_one(led_gpio, GPIOF_DIR_OUT, ev->name)) {
		gpio_direction_output(led_gpio, ev->led_val);
		ev->led = led_gpio; /* this must be last */
	} else {
		pr_info("%s: GPIO-1PPS led config failed\n", PTP_TAI_PRT_STR);
		return -EINVAL;
	}
	return 0;
}

static inline void ptp_event_led_init(struct platform_device *pdev)
{
	struct mv_ptp_event *ev = &mv_ptp_event;
	struct device_node *np = pdev->dev.of_node;
	if (of_property_read_u32(np, MV_PTP_1PPS_LED_DTB_STRING, &ev->led))
		return;
	if (!ev->led)
		return; /* Zero is invalid parameter */
	if (!gpio_request_one(ev->led, GPIOF_DIR_OUT, ev->name))
		gpio_direction_output(ev->led, ev->led_val);
	else
		ev->led = 0;
}

static inline void ptp_event_led_blink(struct mv_ptp_event *ev, int req)
{
	int val;
	if (ev->led) {
		val = (req < 0) ? ~ev->led_val : !!(req);
		ev->led_val = val;
		gpio_set_value(ev->led, val);
	}
}

/***************************************************************************/
static irqreturn_t ptp_gpio_isr(int irq, void *data)
{
	struct mv_ptp_event *ev = data;
	if (!ev->gpio_irq_engaged)
		return IRQ_HANDLED;

	switch (ev->state) {
	case PTP_CLK_SYNC_START:
		ev->state = PTP_CLK_SYNC_START_TIMER;
		mod_timer(&ev->timer, MSEC_1PPS_D(-MSEC_BEFORE_1PPS));
		break;
	case PTP_CLK_GPS_UP:
		/* Restart GPS-Up watchdog  */
		mod_timer(&ev->timer, MSEC_1PPS_D(30));
		ptp_event_led_blink(ev, -1); /*toggle*/
		break;
	case PTP_CLK_GPS_DOWN:
		/* GPS Up-and-Down timer */
		mod_timer(&ev->timer, MSEC_1PPS_D(20));
		schedule_work(&ev->work_q);
		break;
	default: /* ignore _FREE_RUN, _SYNC_START_TIMER, _HW_SYNC */
		break;
	}
	return IRQ_HANDLED;
}

static int ptp_event_ptp_gpioirq(int ptp_hw, int gpio_irq)
{
	struct mv_ptp_event *ev = &mv_ptp_event;
	int ret = 0;

	if (ptp_hw && gpio_irq)
		ptp_hw = gpio_irq = 0;

	/* Clean pending irq by pair free_irq/request_irq */
	if (ev->gpio_irq_engaged) {
		ev->gpio_irq_engaged = false;
		free_irq(ev->gpio2irq, &mv_ptp_event);
	}
	if (ptp_hw)
		pinctrl_select_state(ev->pinctrl, ev->pin_ptp_state);
	else
		pinctrl_select_state(ev->pinctrl, ev->pin_gpio_state);

	if (!ptp_hw && gpio_irq) {
		ev->gpio_irq_engaged = false;
		ret = request_irq(ev->gpio2irq,
					ptp_gpio_isr,
					IRQF_TRIGGER_RISING,
					ev->name,
					ev);
		ev->gpio_irq_engaged = !ret;
		if (ret)
			pr_err("%s: event request_irq failed\n", PTP_TAI_PRT_STR);
	}
	return ret;
}

/***************************************************************************/
static void ptp_tai_tod_set_synchronous(u32 sec, int d_sec)
{
	/* This one is called only in very specific time -
	 * just before expected (next) 1PPS signal obtaining
	 */
	struct mv_pp3_tai_tod ts;
	int clock_restart;

	memset(&ts, 0, sizeof(ts));

	if (!sec && !d_sec)
		sec = 1;
	clock_restart = (d_sec <= 0) || (d_sec > 2);

	if (sec && !d_sec) {
		/* Absolute set required */
		ts.sec_lsb_32b = sec;
	} else {
		/* Get current time, reset-sync and set current+delta */
		mv_pp3_tai_tod_op_read_captured(&ts, NULL); /*cleanup*/
		mv_pp3_tai_tod_op(MV_TAI_GET_CAPTURE, &ts, 0);
		if (sec && d_sec)
			d_sec = 0; /* keep current requested */
		ts.sec_lsb_32b += d_sec;
	}
	ts.nsec = 1000000000 - 1000;
	if (clock_restart)
		mv_pp3_tai_clock_cfg_external(true);
	mv_pp3_tai_tod_op(MV_TAI_SET_UPDATE, &ts, 1);
}

/***************************************************************************/
static void ptp_event_work_cb(struct work_struct *work)
{
	/* EVENT threaded state machine driven by timer */
	struct mv_ptp_event *ev = &mv_ptp_event;
	enum mv_ptp_event_clock_state curr = ev->state;
	struct mv_pp3_tai_tod ts;
	u16 in_cntr_1pps;

	switch (curr) {
	case PTP_CLK_SYNC_START_TIMER:
		ptp_tai_tod_set_synchronous(ev->sec, ev->d_sec);
		/* hw-sync/stabilization takes 3*1PPS cycles */
		if (ev->sec || ev->d_sec) {
			ev->state = PTP_CLK_HW_SYNC;
			mv_pp3_tai_clock_in_cntr_get(NULL); /*cleanup*/
			ptp_event_ptp_gpioirq(1, 0);
			/* Pin-function "ptp" but not "gpio" now.
			 * TAI controlled by HW which could make collision with SW,
			 * Make this period as short as possible.
			 * No gpio-irq available, poll clock_in_cntr with 1msec
			 */
			ev->state_cntr = MSEC_BEFORE_1PPS * 4;
			mod_timer(&ev->timer, MSEC_DELAY(1));
			ev->sec = ev->d_sec = 0;
			break;
		} else {
			ev->state_cntr = -1;
		}
		/* fallthrough */
	case PTP_CLK_HW_SYNC:
		if (ev->state_cntr > 0) {
			in_cntr_1pps = mv_pp3_tai_clock_in_cntr_get(NULL);
			if (!in_cntr_1pps) {
				mod_timer(&ev->timer, MSEC_DELAY(1));
				break; /* keep state, continue polling */
			}
			/* in_cntr_1pps incremented. Stop polling */
		} else if (!ev->state_cntr) {
			pr_warn("%s: guard timer expired\n", PTP_TAI_PRT_STR);
		}
		/* NEXT STATE */
		ev->state = PTP_CLK_GPS_UP;
		mv_pp3_tai_set_nop();
		ptp_event_ptp_gpioirq(0, 1);
		mv_pp3_tai_tod_op_read_captured(&ts, NULL); /*cleanup*/
		mv_pp3_ptp_reset_all_ptp_ports();
		mv_pp3_tai_clock_stable_status_set(1); /* Now stable */
		/* GPS-Up pulse-presence watchdog start */
		mod_timer(&ev->timer, MSEC_1PPS_D(30));
		if (!ev->gps_up_already_printed) {
			ev->gps_up_already_printed = true;
			pr_info("GPS 1PPS is Up\n");
			ptp_event_led_blink(ev, 1);
		}
		break;
	case PTP_CLK_GPS_UP:
		ev->state = PTP_CLK_GPS_DOWN;
		/* clock is free-running but stable */
		pr_info("GPS 1PPS is Down\n");
		ptp_event_led_blink(ev, 0);
		ev->gps_up_already_printed = false;
		ev->state_cntr = 0;
		ptp_event_ptp_gpioirq(0, 1);
		break;
	case PTP_CLK_GPS_DOWN:
		/* Skip some pulses till stable */
		if (++ev->state_cntr >= MV_PTP_GPS_DOWN2UP_PULSES) {
			del_timer(&ev->timer);
			/* 1PPS present again. Restart whole state machine */
			mv_pp3_tai_clock_stable_status_set(0); /* Now unstable */
			ev->state = PTP_CLK_SYNC_START;
			ev->sec = ev->d_sec = 1;
			ptp_event_ptp_gpioirq(0, 1);
		}
		break;

	case PTP_CLK_FREE_RUN:
	case PTP_CLK_SYNC_START:
	default:
		pr_err("%s: event work called on wrong state %d\n",
			PTP_TAI_PRT_STR, curr);
		return;
		break;
	}
}

static void ptp_event_timer_cb(unsigned long data)
{
	/* EVENT timer-swirq state machine driven by timer and HW isr */
	struct mv_ptp_event *ev = (struct mv_ptp_event *)data;
	enum mv_ptp_event_clock_state state = ev->state;

	switch (state) {
	case PTP_CLK_HW_SYNC:
		ev->state_cntr--; /*threshold*/
		schedule_work(&ev->work_q);
		break;

	case PTP_CLK_SYNC_START_TIMER:
	case PTP_CLK_GPS_UP:
		schedule_work(&ev->work_q);
		break;

	case PTP_CLK_GPS_DOWN:
		ev->state_cntr = 0; /*failed again */
		break;

	case PTP_CLK_FREE_RUN:
		/* Could be upon frequent asynchronous sysfs
		 * mv_pp3_tai_clock_from_external_sync()
		 */
	case PTP_CLK_SYNC_START:
	default:
		pr_warn("%s: event timer called on wrong state %d\n",
			PTP_TAI_PRT_STR, state);
		break;
	}
}

/***************************************************************************/
static int ptp_event_monitor_start(void)
{
	struct mv_ptp_event *ev = &mv_ptp_event;
	int rc;

	if (ev->state != PTP_CLK_FREE_RUN)
		return 0; /* already done */

	ev->gpio2irq = gpio_to_irq(ev->gpio);

	INIT_WORK(&ev->work_q, ptp_event_work_cb);
	init_timer(&ev->timer);
	ev->timer.data = (unsigned long)ev;
	ev->timer.function = ptp_event_timer_cb;

	/* To disable MPP HW "ptp-event" set MPP to GPIO */
	rc = gpio_request_one(ev->gpio, GPIOF_DIR_IN, ev->name);
	if (rc) {
		pr_err("%s: gpio_request failed\n", PTP_TAI_PRT_STR);
		return -2;
	}
	ptp_event_ptp_gpioirq(0, 0);
	return 0;
}

static void ptp_event_monitor_stop(void) /* == deinit */
{
	struct mv_ptp_event *ev = &mv_ptp_event;

	if (ev->state != PTP_CLK_FREE_RUN) {
		del_timer(&ev->timer);
		pinctrl_select_state(ev->pinctrl, ev->pin_gpio_state);
		if (ev->gpio_irq_engaged)
			free_irq(ev->gpio2irq, &mv_ptp_event);
		gpio_free(ev->gpio);
		ev->gpio_irq_engaged = false;
		ev->state = PTP_CLK_FREE_RUN;
		ev->gps_up_already_printed = false;
	}
}


void mv_pp3_tai_clock_from_external_sync(u32 start, u32 sec, int d_sec)
{
	struct mv_ptp_event *ev = &mv_ptp_event;

	if (!ev->pinctrl || !ev->gpio) {
		if (start)
			pr_info("TAI/ToD: clock sync from external GPS not configured\n");
		return;
	}
	if (start) {
		/* if both sec/d_sec == 0 - sync without set ToD requested
		 * if both sec/d_sec == 1 - sync with keep current ToD
		 */
		if (d_sec && !mv_pp3_tai_clock_enable_get())
			d_sec = 0;
		ev->sec = sec;
		ev->d_sec = d_sec;
		ptp_event_monitor_start();
		ev->state = PTP_CLK_SYNC_START;
		ptp_event_ptp_gpioirq(0, 1);
	} else {
		ptp_event_monitor_stop();
	}
}

bool mv_pp3_tai_clock_external_init(struct platform_device *pdev)
{
	struct mv_ptp_event *ev = &mv_ptp_event;
	struct device_node *np = pdev->dev.of_node;
	u32 gpio;
	bool is_external = DFLT_CLOCK_IS_EXTERNAL;
	int rc;

	ev->name = MV_PTP_EVENT_DEV_DTB_STRING;

	if (of_property_read_u32(np, MV_PTP_EVENT_MPP_DTB_STRING, &gpio)) {
		pr_info("%s: \"%s\" name not found in .dtb. Sync from GPS is disabled\n",
			PTP_TAI_PRT_STR, MV_PTP_EVENT_MPP_DTB_STRING);
		return false;
	}
	ev->gpio = gpio;
	/* Request (and free) gpio to avoid MPP10 impact */
	rc = gpio_request_one(ev->gpio, GPIOF_DIR_IN, ev->name);
	gpio_free(ev->gpio);
	if (rc) {
		is_external = false;
		pr_err("%s: gpio_request failed on init\n", PTP_TAI_PRT_STR);
	}
	ptp_event_led_init(pdev);
	return is_external;
}


void mv_pp3_tai_clock_external_init2(bool from_external)
{
	/* Start TAI-ToD with it Linux ToD */
	static int tod_linux_is_set;
	struct mv_ptp_event *ev = &mv_ptp_event;
	struct mv_pp3_tai_tod ts;

	if (!tod_linux_is_set) {
		tod_linux_is_set = 1;
		mv_pp3_tai_tod_from_linux(&ts);
	}
	if (!ev->pinctrl || !ev->gpio)
		return;

	if (from_external)
		mv_pp3_tai_clock_from_external_sync(1, 1, 1);
}


/******************************************************************************
* PTP-Event is a physical pulse obtained over MV_PTP_EVENT_MPP_DTB_STRING pin.
*  Whilst the TAI-Clock-External is working it needs MPP function alternating
*  between "gpio" and "ptp".
*  This func-alternating is quite non-trivial requiring special Device Tree.
*  1). It requires 2 sets of pin/function declaration:
*	pinctrl {
*		ptp_event_pin_gpio: ptp_event_pin_gpio {
*			marvell,pins = "mpp10";
*			marvell,function = "gpio";
*		};
*		ptp_event_pin_ptp: ptp_event_pin_ptp {
*			marvell,pins = "mpp10";
*			marvell,function = "ptp";
*		};
*	};
*
*  2). Special pincontrol device to alternate the above sets:
*	pp3_ptp_event: ptp-event {
*		compatible = "marvell,ptp-event";
*		status = "okay";
*		pinctrl-0 = <&ptp_event_pin_gpio>;
*		pinctrl-1 = <&ptp_event_pin_ptp>;
*		pinctrl-names = "gpio", "ptp";
*	};
*
*  3). Device probe making:
*	pinctrl = devm_pinctrl_get(&pdev->dev);
*	pin_gpio_state = pinctrl_lookup_state(pinctrl, "gpio");
*	pin_ptp_state  = pinctrl_lookup_state(pinctrl, "ptp");
*
*  4). So finalyy run-time code could alternate by:
*	pinctrl_select_state(pinctrl, pin_gpio_state);
*                     or
*	pinctrl_select_state(pinctrl, pin_ptp_state);
*
*  NOTE: if MV_PTP_EVENT_DEV_DTB_STRING="ptp-event" or
*           MV_PTP_EVENT_MPP_DTB_STRING="ptp-event-pin"
*  not present in the DTB file, the TAI-Clock-External is disabled
*  and only TAI-Clock-Internal is available.
******************************************************************************
*/
static int ptp_event_dev(struct platform_device *pdev)
{
	struct mv_ptp_event *ev = &mv_ptp_event;

	if (ev->pinctrl)
		goto err; /* wrong init ordering */

	ev->pinctrl = devm_pinctrl_get(&pdev->dev);

	if (PTR_ERR_OR_ZERO(ev->pinctrl))
		goto err;

	ev->pin_gpio_state = pinctrl_lookup_state(ev->pinctrl, "gpio");
	ev->pin_ptp_state  = pinctrl_lookup_state(ev->pinctrl, "ptp");
	if (PTR_ERR_OR_ZERO(ev->pin_gpio_state) || PTR_ERR_OR_ZERO(ev->pin_ptp_state))
		goto err;

	pinctrl_select_state(ev->pinctrl, ev->pin_gpio_state);
	if (DFLT_CLOCK_IS_EXTERNAL)
		mv_pp3_tai_clock_external_init2(true);
	return 0;

err:
	pr_info("%s: cannot init \"%s\". Sync from GPS is disabled\n",
		PTP_TAI_PRT_STR, MV_PTP_EVENT_DEV_DTB_STRING);
	ev->pinctrl = NULL;
	return 0; /* This service is optional => say OK */
}


static const struct of_device_id ptp_event_dev_match[] = {
	{ .compatible = "marvell," MV_PTP_EVENT_DEV_DTB_STRING },
	{}
};
MODULE_DEVICE_TABLE(of, mv_pp3_shared_match);

static struct platform_driver ptp_event_driver = {
	.probe		= ptp_event_dev,
	.driver = {
		.name	= MV_PTP_EVENT_DEV_DTB_STRING,
		.owner	= THIS_MODULE,
		.of_match_table = ptp_event_dev_match,
	},
};

static int __init ptp_event_init_module(void)
{
	if (platform_driver_register(&ptp_event_driver))
		pr_err("%s: Can't register %s\n", PTP_TAI_PRT_STR, MV_PTP_EVENT_DEV_DTB_STRING);
	return 0;
}
module_init(ptp_event_init_module);

MODULE_DESCRIPTION("Marvell PPv3 PTP Pin Event driver - www.marvell.com");
MODULE_AUTHOR("Yan Markman <ymarkman@marvell.com>");
MODULE_LICENSE("GPL");
