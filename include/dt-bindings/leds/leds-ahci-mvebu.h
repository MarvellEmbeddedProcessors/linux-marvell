/*
 * This header provides constants for AHCI MVEBU LED bindings.
 *
 * Copyright (C) 2017 Marvell
 *
 * Ken Ma <make@marvell.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef _DT_BINDINGS_LEDS_AHCI_MVEBU_H
#define _DT_BINDINGS_LEDS_AHCI_MVEBU_H

/*
 * 0    LEDS_AHCI_MVEBU_OFF - the LED is not activated on activity
 * 1    LEDS_AHCI_MVEBU_BLINK_ON - the LED blinks on every 10ms when activity
 *                                 is detected.
 * 2    LEDS_AHCI_MVEBU_BLINK_OFF - the LED is on when idle, and blinks off
 *                                  every 10ms when activity is detected.
 */
#define LEDS_AHCI_MVEBU_OFF			0
#define LEDS_AHCI_MVEBU_BLINK_ON		1
#define LEDS_AHCI_MVEBU_BLINK_OFF		2

#endif /* _DT_BINDINGS_LEDS_AHCI_MVEBU_H */
