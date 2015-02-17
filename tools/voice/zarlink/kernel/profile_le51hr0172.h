/*
 * Le51HR0172.h --
 *
 * This header file exports the Profile data types
 *
 * Project Info --
 *   File:   Y:\TestData\Profiles\Le51HR0172_Rev1\Le51HR0172.vpw
 *   Type:   Le51HR0172
 *   Date:   Tuesday, January 17, 2012 12:29:48
 *   Device: NGCC Le79238
 *
 *   This file was generated with Profile Wizard Version: P2.1.2
 */

#ifndef LE51HR0172_H
#define LE51HR0172_H

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
extern const VpProfileDataType VE792_DEV_PROFILE[];        /* Device Configuration Data */

/************** AC_Coefficients **************/
extern const VpProfileDataType VE792_AC_COEFF[];           /* (NEW) Add AC Coefficients */

/************** DC_Parameters **************/
extern const VpProfileDataType VE792_DC_COEFF[];           /* DC Parameters */

/************** Ring_Parameters **************/
extern const VpProfileDataType VE792_RING_20HZ_SINE[];     /* Ringing 20Hz, Sine Wave, 40Vrms */

/************** Call_Progress_Tones **************/

/************** Cadence_Definitions **************/
extern const VpProfileDataType RINGING_STD[];        /* Standard 2/4 Ringing Cadence */
extern const VpProfileDataType C_CONT[];             /* Continuous ON Cadence */

/************** Caller_ID **************/
extern const VpProfileDataType CID_TYPE2_US[];       /* US Caller ID Type II */
extern const VpProfileDataType CLI_TYPE1_US[];       /* US Caller ID (Type 1 - On-Hook) */

/************** Metering_Profile **************/

#endif /* LE51HR0172_H */
