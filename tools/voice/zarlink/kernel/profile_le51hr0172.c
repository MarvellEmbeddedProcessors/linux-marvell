/*
 * Le51HR0172.c --
 *
 * This file contains profile data in byte format
 *
 * Project Info --
 *   File:   Y:\TestData\Profiles\Le51HR0172_Rev1\Le51HR0172.vpw
 *   Type:   Le51HR0172
 *   Date:   Tuesday, January 17, 2012 12:29:48
 *   Device: NGCC Le79238
 *
 *   This file was generated with Profile Wizard Version: P2.1.2
 */

#include "profile_le51hr0172.h"


/************** Device_Parameters **************/

/* Device Configuration Data */
const VpProfileDataType VE792_DEV_PROFILE[] =
{
    /* Device Profile for VE792 Device Family */
    /* Profile header ----------------------------------------------------- */
        /* version */               0x00,
        /* type */                  0xFF,       /* 0xFF = device profile */
        /* number of sections */    0x02,
        /* content length */        0x29,        /* (2 + 5) + (2 + 32)      */
    /* Section 0 ----------------------------------------------(PRE-BOOT)-- */
        /* section type */          0x00,       /* 0x00 = register list */
        /* content length */        0x05,
        /* Access 0 */
            /* access type */       0x00,       /* 0x00 = direct page write */
            /* page offset */       0x0B,       /* CLK_CFG register */
            /* length */            0x01,
            /* data */              0x00, 0x0A, /* PCLK = 8192 KHz */
    /* Section 1 ---------------------------------------------(POST-BOOT)-- */
        /* section type */          0x00,       /* 0x00 = register list */
        /* content length */        0x20,       /* 5 + 11 + 11 + 5 */
        /* Access 0 */
            /* access type */       0x00,       /* 0x00 = direct page write */
            /* page offset */       0x09,       /* PCM_CFG register */
            /* length */            0x01,
            /* data */              0x00, 0x40, /* Pos. edge, RCS 0, TCS 0 */
        /* Access 1 */
            /* access type */       0x00,       /* 0x00 = direct page write */
            /* page offset */       0x32,       /* HBAT_ADJ + TBAT register */
            /* length */            0x04,
            /* data */              0x00, 0x00, /* HBAT_ADJ = 0 V */
            /* data */              0xEC, 0xCD, /* THBAT = -30 V */
            /* data */              0xF6, 0x66, /* TLBAT = -15 V */
            /* data */              0x06, 0x66, /* TPBAT = 10 V */
        /* Access 2 */
            /* access type */       0x00,       /* 0x00 = direct page write */
            /* page offset */       0x38,       /* POWER register */
            /* length */            0x04,
            /* data */              0x7F, 0xFF, /* Ringing Power Algorithm: */
            /* data */              0x7F, 0xFF, /*     Normal               */
            /* data */              0x7F, 0xFF,
            /* data */              0x00, 0x00,
        /* Access 3 */
            /* access type */       0x00,       /* 0x00 = direct page write */
            /* page offset */       0x3F,       /* SYS_CFG register */
            /* length */            0x01,
            /* data */              0x00, 0x00  /* DSPWait On; ADCBias On */
    /* Unstructured data -------------------------------------------------- */
        /* none */
};

/************** AC_Coefficients **************/

/* (NEW) Add AC Coefficients */
const VpProfileDataType VE792_AC_COEFF[] =
{
    /* AC Profile */
    /* Profile header ----------------------------------------------------- */
        /* version */               0x01,
        /* type */                  0x00,       /* 0x00 = AC profile */
        /* number of sections */    0x01,
        /* content length */        0x74,     /* (2 + 81) + 1 + 32 */
    /* Section 0 ---------------------------------------------------------- */
        /* section type */          0x01,       /* 0x01 = mailbox command */
        /* content length */        0x51,       /* 81 */
        /* command ID */            0x2A,       /* 0x2A = WR_AC_PARAM */
        /* data */
            /* AC_MASK */           0x01, 0xFF,
            /* Z FIR */             0x00, 0xB4, 0xBA, 0xAC, 0x35, 0xA3, 0x33,
                                    0x5A, 0x24, 0xDC, 0x4B,
            /* Z IIR */             0x22, 0x22, 0x97, 0x9F, 0x01,
            /* GR */                0xA2, 0xA0,
            /* R IIR */             0xDC, 0x01,
            /* R FIR */             0x2A, 0x10, 0xAA, 0xC9, 0x42, 0x37, 0x3A,
                                    0x42, 0xBD, 0xBA, 0xC3, 0xB4,
            /* B FIR */             0x00, 0x2A, 0x7A, 0x4B, 0x9F, 0x9A, 0xA9,
                                    0xAD, 0xBD, 0xC6, 0x2A, 0x52, 0x97, 0xAA,
                                    0x60,
            /* B IIR */             0x2E, 0x01,
            /* GX */                0x24, 0x40,
            /* X FIR */             0xBA, 0x10, 0x22, 0xA8, 0xEA, 0xA1, 0x2A,
                                    0x2B, 0xA6, 0x27, 0x34, 0xAF, 0x00,
            /* AISN */              0x00, 0x05,
            /* DISN */              0x00, 0x01, 0x00, 0x07, 0x00, 0x6F, 0x00,
                                    0x1C, 0x00, 0x80,
            /* MTR_CFG */           0x00, 0x00,
    /* Unstructured data -------------------------------------------------- */
        /* data */                  0x00,         /* VP_CFG2 bit LRG = 0 */
            /* Meter LUT */         0x11, 0x5F,   /* Shadow Register 1 12KHz 2/8 */
                                    0x06, 0x09,   /* Shadow Register 2 */
                                    0x11, 0x2F,   /* Shadow Register 1 12KHz 4/8 */
                                    0x06, 0x09,   /* Shadow Register 2 */
                                    0x11, 0x77,   /* Shadow Register 1 12KHz 6/8 */
                                    0x08, 0x0C,   /* Shadow Register 2 */
                                    0x11, 0x77,   /* Shadow Register 1 12KHz 8/8 */
                                    0x0C, 0x12,   /* Shadow Register 2 */
                                    0x01, 0x9F,   /* Shadow Register 1 16KHz 2/8 */
                                    0x06, 0x09,   /* Shadow Register 2 */
                                    0x01, 0x67,   /* Shadow Register 1 16KHz 4/8 */
                                    0x06, 0x09,   /* Shadow Register 2 */
                                    0x01, 0x2F,   /* Shadow Register 1 16KHz 6/8 */
                                    0x08, 0x0C,   /* Shadow Register 2 */
                                    0x01, 0x2F,   /* Shadow Register 1 16KHz 8/8 */
                                    0x0C, 0x12    /* Shadow Register 2 */
};

/************** DC_Parameters **************/

/* DC Parameters */
const VpProfileDataType VE792_DC_COEFF[] =
{
    /* DC Profile */
    /* Profile header ----------------------------------------------------- */
        /* version */               0x00,
        /* type */                  0x01,       /* 0x01 = DC profile */
        /* number of sections */    0x02,
        /* content length */        0x25,       /* (2 + 13) + (2 + 20) */
    /* Section 0 ---------------------------------------------------------- */
        /* section type */          0x01,       /* 0x01 = mailbox command */
        /* content length */        0x0D,       /* 13 */
        /* command ID */            0x2C,       /* 0x2C = WR_DC_PARAM */
        /* data */                  0x44, 0x44, /* V1 */
        /* data */                  0x0D, 0xA7, /* VAS */
        /* data */                  0x01, 0x73, /* VAS_OFFSET */
        /* data */                  0x2E, 0xEF, /* RFD */
        /* data */                  0x80, 0x00, /* RPTC */
        /* data */                  0x50, 0x75, /* ILA */
    /* Section 1 ---------------------------------------------------------- */
        /* section type */          0x00,       /* 0x00 = register list */
        /* content length */        0x14,       /* 20 */
        /* Access 0 */
            /* access type */       0x01,       /* 0x01 = chan. page write */
            /* page offset */       0x33,       /* LOOP_SUP register */
            /* length */            0x06,
            /* data */              0xF3, 0x33, /* TGK */
            /* data */              0x00, 0xC8, /* PGK */
            /* data */              0x0B, 0xB8, /* TSH */
            /* data */              0x00, 0x1E, /* DSH */
            /* data */              0x40, 0x00, /* IFTA */
            /* data */              0x0C, 0xCD, /* IFTD */
        /* Access 1 */
            /* access type */       0x01,       /* 0x01 = chan. page write */
            /* page offset */       0x3C,       /* LOOP_SUP register, HSH */
            /* length */            0x01,
            /* data */              0x03, 0xE8  /* HSH */
    /* Unstructured data -------------------------------------------------- */
        /* none */
};

/************** Ring_Parameters **************/

/* Ringing 20Hz, Sine Wave, 40Vrms */
const VpProfileDataType VE792_RING_20HZ_SINE[] =
{
    /* Ring Profile (Internal Ringing) */
    /* Balanced, Sinusoidal, Crest Factor 1.4142 */
    /* 20 Hz, 84.72 Vpk, -47.17 V Tip Bias, -62.83 V Ring Bias */
    /* Ring Trip: Short Loop 27.2 mA, Long Loop 8 mA */
    /*            Current Spike Threshold 100 mA */
    /*            Averaging Period 50 ms */
    /* Ring Exit Hook Switch Debounce: Threshold 0 ms, Duration 0 ms */
    /* Profile header ----------------------------------------------------- */
        /* version */               0x00,
        /* type */                  0x04,       /* 0x04 = ringing profile */
        /* number of sections */    0x01,
        /* content length */        0x22,       /* (2 + 32) */
    /* Section 0 ---------------------------------------------------------- */
        /* section type */          0x00,       /* 0x00 = register list */
        /* content length */        0x20,       /* 32 */
        /* Access 0 */
            /* access type */       0x01,       /* 0x01 = chan. page write */
            /* page offset */       0x22,       /* RING register */
            /* length */            0x0B,       /* 11 */
            /* data */              0x30, 0x00, /* R control / FRQR[23:16] */
            /* data */              0xA3, 0xD7, /* FRQR[15:0] */
            /* data */              0x48, 0x4A, /* AMPR */
            /* data */              0xD7, 0xC0, /* RBA */
            /* data */              0xCA, 0x63, /* RBB */
            /* data */              0x04, 0x05, /* EBR, EXR */
            /* data */              0x22, 0xDB, /* RTSL */
            /* data */              0x0A, 0x3D, /* RTLL */
            /* data */              0x00, 0x19, /* RTAP */
            /* data */              0x00, 0x00, /* RSVD */
            /* data */              0x7F, 0xFF, /* IST */
        /* Access 1 */
            /* access type */       0x01,       /* 0x01 = chan. page write */
            /* page offset */       0x3A,       /* REDSH, REDD */
            /* length */            0x02,
            /* data */              0x00, 0x00, /* REDSH */
            /* data */              0x00, 0x00  /* REDD */
    /* Unstructured data -------------------------------------------------- */
        /* none */
};

/************** Call_Progress_Tones **************/

/************** Cadence_Definitions **************/

/* Standard 2/4 Ringing Cadence */
const VpProfileDataType RINGING_STD[] =
{
    /* Cadence Profile */
    /* Profile header ----------------------------------------------------- */
        /* version */               0x00,
        /* type */                  0x08,       /* 0x08 = ring cadence */
        /* number of sections */    0x01,
        /* content length */        0x12,       /* (2 + 16) */
    /* Section 0 ---------------------------------------------------------- */
        /* section type */          0x02,       /* 0x02 = sequence */
        /* content length */        0x10,       /* 16 */
        /* data */                  0x00, 0x06, /* sequence length = 7 */
        /* data */                  0x02, 0x86, /* 00 - Line State */
        /* data */                  0x01, 0x3B,
        /* data */                  0x47, 0xD0, /* 01 - Sequential Delay */
        /* data */                  0x02, 0x83, /* 02 - Line State */
        /* data */                  0x01, 0x3B,
        /* data */                  0x4F, 0xA0, /* 03 - Sequential Delay */
        /* data */                  0x10, 0x00, /* 04 - Branch to 00 */
    /* Unstructured data -------------------------------------------------- */
        /* none */
};

/* Continuous ON Cadence */
const VpProfileDataType C_CONT[] =
{
    /* Cadence Profile */
    /* Profile header ----------------------------------------------------- */
        /* version */               0x00,
        /* type */                  0x03,       /* 0x03 = tone cadence */
        /* number of sections */    0x01,
        /* content length */        0x06,       /* (2 + 4) */
    /* Section 0 ---------------------------------------------------------- */
        /* section type */          0x02,       /* 0x02 = sequence */
        /* content length */        0x04,       /* 4 */
        /* data */                  0x00, 0x00, /* sequence length = 1 */
        /* data */                  0x05, 0x0F, /* 00 - Generator Ctrl */
    /* Unstructured data -------------------------------------------------- */
        /* none */
};

/************** Caller_ID **************/

/* US Caller ID Type II */
const VpProfileDataType CID_TYPE2_US[] =
{
    /* Profile header ----------------------------------------------------- */
        /* version */               0x01,
        /* type */                  0x05,       /* 0x05 = caller ID profile */
        /* number of sections */    0x04,
        /* content length */        0x70,       /* 112 */
    /* Section 0 ---------------------------------------------------------- */
        /* section type */          0x02,       /* 0x02 = sequencer program */
        /* content length */        0x24,       /* 36 */
        /* data */                  0x00, 0x10, /* sequence length = 17 */
        /* data */                  0x01, 0x3B, /* Mute on */
        /* data */                  0x01, 0x3D, /* Alert tone 300ms */
        /* data */                  0x41, 0x2C, /* - TONE_CALLWAIT */
        /* data */                  0x05, 0x00,
        /* data */                  0x40, 0x0A, /* Silence 10ms */
        /* data */                  0x01, 0x3E, /* Alert tone 80ms */
        /* data */                  0x40, 0x50, /* - TONE_CLI */
        /* data */                  0x05, 0x00,
        /* data */                  0x01, 0x32, /* Detect interval 160ms */
        /* data */                  0x40, 0xA0, /* - Tones: A, D */
        /* data */                  0x09, 0xB2,
        /* data */                  0x40, 0x64, /* Silence 100ms */
        /* data */                  0x01, 0x26, /* Send FSK */
        /* data */                  0x0B, 0x01,
        /* data */                  0x09, 0x00,
        /* data */                  0x01, 0x27,
        /* data */                  0x01, 0x3A, /* Mute off */
    /* Section 1 ---------------------------------------------------------- */
        /* section type */          0x00,       /* 0x00 = register list */
        /* content length */        0x0E,       /* 14 */
        /* Access 0 */
            /* access type */       0x01,       /* 0x01 = chan. page write */
            /* page offset */       0x1F,       /* FSK_GEN register */
            /* length */            0x01,
            /* data */              0x00, 0x04, /* stop = 1, start = 0 */
        /* Access 1 */
            /* access type */       0x01,       /* 0x01 = chan. page write */
            /* page offset */       0x10,       /* FSK_GEN register */
            /* length */            0x03,
            /* data */              0x29, 0x9A, /* mark freq. 1300Hz  */
                                    0x43, 0x33, /* space freq. 2100Hz */
                                    0x12, 0xC6, /* amplitude = -13.5dBm0 */
    /* Section 2 ---------------------------------------------------------- */
        /* section type */          0x00,       /* 0x00 = register list */
        /* content length */        0x17,       /* 23 */
        /* Access 0 */
            /* access type */       0x01,       /* 0x01 = chan. page write */
            /* page offset */       0x03,       /* SIG_GEN register */
            /* length */            0x0A,       /* 10 */
            /* data */              0x30, 0x0E,
            /* data */              0x14, 0x7B, /* FreqA = 440Hz */
            /* data */              0x13, 0xE3, /* AmpA  = -13dBm0 */
            /* data */              0x00, 0x00, /* FreqB = 0Hz */
            /* data */              0x58, 0xD6, /* AmpB  = 0dBm0 */
            /* data */              0x00, 0x00, /* FreqC = 0Hz */
            /* data */              0x58, 0xD6, /* AmpC  = 0dBm0 */
            /* data */              0x00, 0x00, /* FreqD = 0Hz */
            /* data */              0x58, 0xD6, /* AmpD  = 0dBm0 */
            /* data */              0x00, 0x01, /* Generator control = A */
    /* Section 3 ---------------------------------------------------------- */
        /* section type */          0x00,       /* 0x00 = register list */
        /* content length */        0x17,       /* 23 */
        /* Access 0 */
            /* access type */       0x01,       /* 0x01 = chan. page write */
            /* page offset */       0x03,       /* SIG_GEN register */
            /* length */            0x0A,       /* 10 */
            /* data */              0x30, 0x44,
            /* data */              0x28, 0xF6, /* FreqA = 2130Hz */
            /* data */              0x13, 0xE3, /* AmpA  = -13dBm0 */
            /* data */              0x58, 0x00, /* FreqB = 2750Hz */
            /* data */              0x13, 0xE3, /* AmpB  = -13dBm0 */
            /* data */              0x00, 0x00, /* FreqC = 0Hz */
            /* data */              0x58, 0xD6, /* AmpC  = 0dBm0 */
            /* data */              0x00, 0x00, /* FreqD = 0Hz */
            /* data */              0x58, 0xD6, /* AmpD  = 0dBm0 */
            /* data */              0x00, 0x03, /* Generator control = AB */
    /* Unstructured data -------------------------------------------------- */
                                    0x00, 0x00, /* 0-bit channel seizure */
                                    0x00, 0x58, /* 88-bit mark period */
                                    0x00, 0x0D, /* Detect Tones A, D */
                                    0x01,       /* API checksum */
                                    0x00        /* FSK Message Data */
};

/* US Caller ID (Type 1 - On-Hook) */
const VpProfileDataType CLI_TYPE1_US[] =
{
    /* Profile header ----------------------------------------------------- */
        /* version */               0x01,
        /* type */                  0x05,       /* 0x05 = caller ID profile */
        /* number of sections */    0x02,
        /* content length */        0x26,       /* 38 */
    /* Section 0 ---------------------------------------------------------- */
        /* section type */          0x02,       /* 0x02 = sequencer program */
        /* content length */        0x0C,       /* 12 */
        /* data */                  0x00, 0x04, /* sequence length = 5 */
        /* data */                  0x42, 0x08, /* Silence 520ms */
        /* data */                  0x01, 0x26, /* Send FSK */
        /* data */                  0x0B, 0x01,
        /* data */                  0x09, 0x00,
        /* data */                  0x01, 0x27,
    /* Section 1 ---------------------------------------------------------- */
        /* section type */          0x00,       /* 0x00 = register list */
        /* content length */        0x0E,       /* 14 */
        /* Access 0 */
            /* access type */       0x01,       /* 0x01 = chan. page write */
            /* page offset */       0x1F,       /* FSK_GEN register */
            /* length */            0x01,
            /* data */              0x00, 0x04, /* stop = 1, start = 0 */
        /* Access 1 */
            /* access type */       0x01,       /* 0x01 = chan. page write */
            /* page offset */       0x10,       /* FSK_GEN register */
            /* length */            0x03,
            /* data */              0x26, 0x66, /* mark freq. 1200Hz  */
                                    0x46, 0x66, /* space freq. 2200Hz */
                                    0x27, 0xAE, /* amplitude = -7dBm0 */
    /* Unstructured data -------------------------------------------------- */
                                    0x01, 0x30, /* 304-bit channel seizure */
                                    0x00, 0xB8, /* 184-bit mark period */
                                    0x00, 0x00,
                                    0x01,       /* API checksum */
                                    0x00        /* FSK Message Data */
};

/************** Metering_Profile **************/
int dev_profile_size = sizeof(VE792_DEV_PROFILE);
int ac_profile_size = sizeof(VE792_AC_COEFF);
int dc_profile_size = sizeof(VE792_DC_COEFF);
int ring_profile_size = sizeof(VE792_RING_20HZ_SINE);

/* end of file Le51HR0172.c */
