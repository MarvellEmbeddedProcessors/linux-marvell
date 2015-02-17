/*
 * profile_88801.c --
 *
 * This file contains profile data in byte format
 *
 * Project Info --
 *   File:   C:\Zarlink\profiles\zl88801_shared_tracker\NewProject.vpw
 *   Type:   Custom Design Using Microsemi ZL880 - ZL88801
 *   Date:   Tuesday, November 12, 2013 09:27:50
 *   Device: ZL880 ZL88801_T
 *
 *   This file was generated with Profile Wizard Version: P2.5.0
 */

#include "profile_88801.h"


/************** Device Profile **************/

/* Device Configuration Data */
const VpProfileDataType DEV_PROFILE[] = {
/* Profile Header ---------------------------------------------------------- */
0x0D, 0xFF, 0x00,       /* Device Profile for ZL88801_T device */
0x28,                   /* Profile length = 40 + 4 = 44 bytes */
0x04,                   /* Profile format version = 4 */
0x14,                   /* MPI section length = 20 bytes */
/* Raw MPI Section --------------------------------------------------------- */
0x46, 0x0A,             /* PCLK = 8.192 MHz; INTM = CMOS-compatible */
0x44, 0x46,             /* PCM Clock Slot = 6 TX / 0 RX; XE = Pos. */
0x5E, 0x14, 0x04,       /* Device Mode Register */
0xF6, 0x54, 0x40, 0x54, /* Switching Regulator Timing Params */
0x40, 0xA8, 0x40,
0xE4, 0x04, 0x92, 0x0A, /* Switching Regulator Params */
0xE6, 0x60,             /* Switching Regulator Control */
/* Formatted Parameters Section -------------------------------------------- */
0x00,                   /* IO21 Mode: Digital */
			/* IO22 Mode: Digital */
0x70,                   /* Dial pulse correction: 7 ms */
			/* Switcher Configuration =
				Shared BuckBoost (12 V in, 100 V out) */
0x00, 0xFF,             /* Over-current shutdown count = 255 */
0x01,                   /* Leading edge blanking window = 81 ns */
0x1C, 0x12, 0x1C, 0x12, /* FreeRun Timing Parameters */
0x1C, 0x12,
0xFF,
0x54, 0x40,             /* Low-Power Timing Parameters */
0x62, 0x62,             /* Switcher Limit Voltages 98 V, 98 V */
0x80,                   /* Charge pump disabled */
			/* Charge Pump Overload Protection: Under-Voltage Shutdown */
0x3C                    /* Switcher Input 12 V */
};

/************** DC Profile **************/

/* DC Parameters */
const VpProfileDataType DC_PROFILE[] = {
/* Profile Header ---------------------------------------------------------- */
0x0D, 0x01, 0x00,       /* DC Profile for ZL88801_T device */
0x0C,                   /* Profile length = 12 + 4 = 16 bytes */
0x02,                   /* Profile format version = 2 */
0x03,                   /* MPI section length = 3 bytes */
/* Raw MPI Section --------------------------------------------------------- */
0xC6, 0x92, 0x28,       /* DC Feed Parameters: ir_overhead = 100 ohm; */
			/* VOC = 48 V; LI = 50 ohm; VAS = 8.78 V; ILA = 26 mA */
			/* Maximum Voice Amplitude = 2.93 V */
/* Formatted Parameters Section -------------------------------------------- */
0x1B,                   /* Switch Hook Threshold = 10 mA */
			/* Ground-Key Threshold = 18 mA */
0x84,                   /* Switch Hook Debounce = 8 ms */
			/* Ground-Key Debounce = 16 ms */
0x50,                   /* Low-Power Switch Hook Hysteresis = 2 V */
			/* Ground-Key Hysteresis = 6 mA */
			/* Switch Hook Hysteresis = 0 mA */
0x80,                   /* Low-Power Switch Hook Threshold = 22 V */
0x04,                   /* Floor Voltage = -25 V */
0x00,                   /* R_OSP = 0 ohms */
0x07                    /* R_ISP = 7 ohms */
};

/************** AC Profile **************/
/* AC FXS RF100 600R Normal Coefficients (Default)  */
const VpProfileDataType AC_FXS_RF100_600R_DEF[] = {
  /* AC Profile */
0xAD, 0x00, 0xAC, 0x4C, 0x01, 0x49, 0xCA, 0xEA, 0x98, 0xBA, 0xEB, 0x2A,
0x2C, 0xB5, 0x25, 0xAA, 0x24, 0x2C, 0x3D, 0x9A, 0xAA, 0xBA, 0x27, 0x9F,
0x01, 0x8A, 0x2D, 0x01, 0x2B, 0xB0, 0x5A, 0x33, 0x24, 0x5C, 0x35, 0xA4,
0x5A, 0x3D, 0x33, 0xB6, 0x88, 0x3A, 0x10, 0x3D, 0x3D, 0xB2, 0xA7, 0x6B,
0xA5, 0x2A, 0xCE, 0x2A, 0x8F, 0x82, 0xA8, 0x71, 0x80, 0xA9, 0xF0, 0x50,
0x00, 0x86, 0x2A, 0x42, 0x22, 0x4B, 0x1C, 0xA3, 0xA8, 0xFF, 0x8F, 0xAA,
0xF5, 0x9F, 0xBA, 0xF0, 0x96, 0x2E, 0x01, 0x00
};

/************** Ringing Profile **************/

/* Ringing 20Hz, Sine Wave */
const VpProfileDataType RING_20HZ_SINE[] = {
/* Profile Header ---------------------------------------------------------- */
0x0D, 0x04, 0x00,       /* Ringing Profile for ZL88801_T device */
0x12,                   /* Profile length = 18 + 4 = 22 bytes */
0x01,                   /* Profile format version = 1 */
0x0C,                   /* MPI section length = 12 bytes */
/* Raw MPI Section --------------------------------------------------------- */
0xC0, 0x10, 0x00, 0x00, /* Ringing Generator Parameters: */
0x00, 0x37, 0x3A, /* 20.1 Hz Sine; 1.41 CF; 70.00 Vpk; 0.00 V bias */
0x08, 0x00, 0x00, /* Ring trip cycles = 2; RTHALFCYC = 1 */
0x00, 0x00,
/* Formatted Parameters Section -------------------------------------------- */
0xB3,                   /* Ring Trip Method = AC; Threshold = 25.5 mA */
0x0E,                   /* Ringing Current Limit = 78 mA */
0x4D,                   /* Fixed; Max Ringing Supply = 70 V */
0x00                    /* Balanced; Ring Cadence Control Disabled */
};

/************** Tone Profile **************/

/************** Cadence Profile **************/

/************** Caller ID Profile **************/

/* US Caller ID Type II */
const VpProfileDataType CID_TYPE2_US[] = {
/* Space=2100Hz, Mark=1300Hz, Amp=-13.50dBm */
/* Caller ID Profile */
0x00, 0x05, 0x00, 0x4C, 0x00, 0x00,
0x09, 0xD4, /* MPI Length and Command */
0x16, 0x66, 0x12, 0xD8, 0x0D, 0xDD, 0x12, 0xD8, /* MPI Data */
0x00, 0x01, /* Checksum Computed by device/API */
0x00, 0x3C, /* Length of Elements Data */
0x00, 0x02, /* Mute the far end */
0x00, 0x04, /* Alert Tone */
/* Call Waiting Tone for Type II Caller ID */
/* 439.82 Hz, -13.00 dBm0, 0.00 Hz, -100.00 dBm0 */
0x09, 0xD4, 0x04, 0xB1, 0x13, 0xF7, 0x00, 0x00, 0x00, 0x00,
0x00, 0x05, 0x01, 0x2C, /* Alert Tone Part 2 - Tone+Cadence = 300ms */
0x00, 0x06, 0x00, 0x0A, /* Silence Interval for 10ms */
0x00, 0x04, /* Alert Tone */
/* Caller ID Alert Tone */
/* 2129.88 Hz, -13.00 dBm0, 2749.88 Hz, -13.00 dBm0 */
0x09, 0xD4, 0x16, 0xB8, 0x13, 0xF7, 0x1D, 0x55, 0x13, 0xF7,
0x00, 0x05, 0x00, 0x50, /* Alert Tone Part 2 - Tone+Cadence = 80ms */
0x00, 0x08, 0x00, 0xA0, 0x00, 0xD0, 0x00, 0x00, /* Detect Tone A | D, Timeout = 160ms */
0x00, 0x06, 0x00, 0x64, /* Silence Interval for 100ms */
0x00, 0x0A, 0x00, 0x46, /* Mark Signal for 70ms */
0x00, 0x0B, /* Message Data (FSK Format) */
0x00, 0x03, /* Unmute the far end */
0x00, 0x0D  /* End of Transmission */
};

/* US Caller ID (Type 1 - On-Hook) */
const VpProfileDataType CLI_TYPE1_US[] = {
/* Space=2200Hz, Mark=1200Hz, Amp=-7.00dBm */
/* Caller ID Profile */
0x00, 0x05, 0x00, 0x20, 0x00, 0x00,
0x09, 0xD4, /* MPI Length and Command */
0x17, 0x77, 0x27, 0xD4, 0x0C, 0xCC, 0x27, 0xD4, /* MPI Data */
0x00, 0x01, /* Checksum Computed by device/API */
0x00, 0x10, /* Length of Elements Data */
0x00, 0x06, 0x02, 0x08, /* Silence Interval for 520ms */
0x00, 0x09, 0x00, 0xFA, /* Channel Seizure for 250ms */
0x00, 0x0A, 0x00, 0x96, /* Mark Signal for 150ms */
0x00, 0x0B, /* Message Data (FSK Format) */
0x00, 0x0D  /* End of Transmission */
};

/************** Metering_Profile **************/
int dev_profile_size = sizeof(DEV_PROFILE);
int dc_profile_size = sizeof(DC_PROFILE);
int ac_profile_size = sizeof(AC_FXS_RF100_600R_DEF);
int ring_profile_size = sizeof(RING_20HZ_SINE);

/* end of file profile_88801.c */
