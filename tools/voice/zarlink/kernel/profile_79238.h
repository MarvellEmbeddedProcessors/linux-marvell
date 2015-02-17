/*
 * profile_79238.h --
 *
 * This header file exports the Profile data types
 *
 * Project Info --
 *   File:  C:\Documents and Settings\benavi\Desktop\profile_792\792.vpw
 *   Type:  VCP2-792 Project (Line Module Le51HR0128)
 *   Date:  Thursday, June 03, 2010 13:40:38
 *
 *   This file was generated with Profile Wizard Version: P1.14.1
 */

#ifndef PROFILE_79238_H
#define PROFILE_79238_H

#ifdef VP_API_TYPES_H
#include "vp_api_types.h"
#else
typedef unsigned char VpProfileDataType;
#endif

extern int dev_profile_size;
extern int dc_profile_size;
extern int ac_profile_size;
extern int ring_profile_size;

/************** Device_Parameters **************/
/* Device Profile */
extern const VpProfileDataType VE792_DEV_PROFILE[];

/************** AC_Coefficients **************/
extern const VpProfileDataType VE792_AC_COEFF_600[]; /* AC Parameters (600) */
extern const VpProfileDataType AC_COEFF_900[];       /* AC Parameters (900) */

/************** DC_Parameters **************/
extern const VpProfileDataType VE792_DC_COEFF[];     /* DC Parameters */

/************** Ring_Parameters **************/
extern const VpProfileDataType RING_20HZ_SINE[];     /* Ringing 20Hz, Sine Wave, 40Vrms */
extern const VpProfileDataType RING_25HZ_SINE[];     /* Ringing 25Hz, Sine Wave, 40Vrms */

/************** Call_Progress_Tones **************/
extern const VpProfileDataType TONE_DIAL[];          /* US Dial Tone */
extern const VpProfileDataType TONE_RINGBACK[];      /* US Ringback Tone */
extern const VpProfileDataType TONE_BUSY[];          /* US Busy Tone */
extern const VpProfileDataType TONE_REORDER[];       /* US Reorder Tone */
extern const VpProfileDataType TONE_US_HOWLER[];     /* US Howler Tone (ROH) */
extern const VpProfileDataType TONE_UK_HOWLER[];     /* UK Howler Tone */
extern const VpProfileDataType TONE_AUS_HOWLER[];    /* Australia Howler Tone */
extern const VpProfileDataType TONE_NTT_HOWLER[];    /* Japan Howler Tone */
extern const VpProfileDataType TONE_SIT[];           /* Special Information Tone (Called Number Not Connected) */
extern const VpProfileDataType TONE_ONEKHZ_L[];      /* A 1kHz tone at -10dBm0 */
extern const VpProfileDataType TONE_ONEKHZ_H[];      /* A 1kHz tone at 0dBm0 */
extern const VpProfileDataType TONE_CALLWAIT[];      /* Call Waiting Beep */
extern const VpProfileDataType TONE_CLI[];           /* Caller ID Alert Tone */

/************** Cadence_Definitions **************/
extern const VpProfileDataType TONE_CAD_DIAL[];      /* US Dial Tone Cadence */
extern const VpProfileDataType TONE_CAD_STUTTER[];   /* Stutter Dial Tone Cadence */
extern const VpProfileDataType TONE_CAD_RINGBACK[];  /* US Ringback Tone Cadence */
extern const VpProfileDataType TONE_CAD_BUSY[];      /* US Busy Tone Cadence */
extern const VpProfileDataType TONE_CAD_REORDER[];   /* US Reorder Tone Cadence */
extern const VpProfileDataType TONE_CAD_US_HOWLER[]; /* US Howler Tone Cadence (ROH) */
extern const VpProfileDataType TONE_CAD_UK_HOWLER[]; /* UK Howler Tone Cadence */
extern const VpProfileDataType TONE_CAD_AUS_HOWLER[];/* Australia Howler Tone Cadence */
extern const VpProfileDataType TONE_CAD_NTT_HOWLER[];/* Japan Howler Tone Cadence */
extern const VpProfileDataType TONE_CAD_SIT[];       /* Special Information Tone Cadence */
extern const VpProfileDataType RING_CAD_STD[];       /* Standard Ringing Cadence */
extern const VpProfileDataType RING_CAD_CID[];       /* Ringing Cadence with CallerID */
extern const VpProfileDataType RING_CAD_ON[];        /* Ringing Always On */

/************** Caller_ID **************/
extern const VpProfileDataType CID_TYPE2_US[];       /* US Caller ID Type II */
extern const VpProfileDataType CID_TYPE1_UK[];       /* UK Caller ID Type I */
extern const VpProfileDataType CLI_TYPE1_US[];       /* US Caller ID (Type 1 - On-Hook) */

/************** Metering_Profile **************/
extern const VpProfileDataType METER_12KHZ[];        /* Metering Profile 12 kHz Tone */
extern const VpProfileDataType METER_POLREV[];       /* Metering Profile Polarity Reversal */

#endif /* PROFILE_79238_H */
