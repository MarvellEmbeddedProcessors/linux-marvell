/*
 * profile_79238.c --
 *
 * This file contains profile data in byte format
 *
 * Project Info --
 *   File:  C:\Documents and Settings\benavi\Desktop\profile_792\792.vpw
 *   Type:  VCP2-792 Project (Line Module Le51HR0128)
 *   Date:  Wednesday, June 30, 2010 14:29:56
 *
 *   This file was generated with Profile Wizard Version: P1.14.1
 */

#include "profile_79238.h"

/*** Device_Parameters for VE792 Device Family ***/
const VpProfileDataType VE792_DEV_PROFILE[] =
{
    /* Device Profile */
    /* Profile header ----------------------------------------------------- */
        /* version */               0x00,
        /* type */                  0xFF,       /* 0xFF = device profile */
        /* number of sections */    0x02,
        /* content length */        0x24,        /* (2 + 5) + (2 + 27)      */
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
        /* content length */        0x1B,       /* 5 + 11 + 11 */
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
            /* data */              0x13, 0x33, /* TPBAT = 30 V */
        /* Access 2 */
            /* access type */       0x00,       /* 0x00 = direct page write */
            /* page offset */       0x38,
            /* length */            0x04,
            /* data */              0x7F, 0xFF,
            /* data */              0x7F, 0xFF,
            /* data */              0x7F, 0xFF,
            /* data */              0x00, 0x00
    /* Unstructured data -------------------------------------------------- */
        /* none */
};

/************** AC_Coefficients **************/

/* AC Parameters (600) */
const VpProfileDataType VE792_AC_COEFF_600[] =
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
            /* Z FIR */             0x00, 0x22, 0xBA, 0x74, 0x45, 0x3E, 0x33,
                                    0x5A, 0x24, 0xDC, 0x4B,
            /* Z IIR */             0x22, 0x22, 0x97, 0x9F, 0x01,
            /* GR */                0xA2, 0xA0,
            /* R IIR */             0xDC, 0x01,
            /* R FIR */             0x2A, 0x10, 0xAD, 0xC9, 0x22, 0x27, 0x22,
                                    0x52, 0x3F, 0xBA, 0xC3, 0xB4,
            /* B FIR */             0x00, 0x3C, 0x7A, 0x4B, 0xAF, 0x9A, 0xA9,
                                    0xC6, 0xBA, 0x37, 0x24, 0x6B, 0x8F, 0x3B,
                                    0x70,
            /* B IIR */             0x2E, 0x01,
            /* GX */                0x3A, 0x30,
            /* X FIR */             0x3D, 0x20, 0x9F, 0x2A, 0xA3, 0x4E, 0x4B,
                                    0x23, 0x52, 0xAB, 0xBB, 0x97, 0x00,
            /* AISN */              0x00, 0x05,
            /* DISN */              0x00, 0x01, 0x00, 0x07, 0x00, 0x6F, 0x00,
                                    0x1C, 0x00, 0x80,
            /* MTR_CFG */           0x00, 0x00,
    /* Unstructured data -------------------------------------------------- */
        /* data */                  0x00,         /* VP_CFG2 bit LRG = 0 */
        /* Meter LUT */             0x11, 0x5F,   /* Shadow Register 1 12KHz 2/8 */
                                    0x06, 0x09,   /* Shadow Register 2 */
        /* Meter LUT */             0x11, 0x2F,   /* Shadow Register 1 12KHz 4/8 */
                                    0x06, 0x09,   /* Shadow Register 2 */
        /* Meter LUT */             0x11, 0x77,   /* Shadow Register 1 12KHz 6/8 */
                                    0x08, 0x0C,   /* Shadow Register 2 */
        /* Meter LUT */             0x11, 0x77,   /* Shadow Register 1 12KHz 8/8 */
                                    0x0C, 0x12,   /* Shadow Register 2 */
        /* Meter LUT */             0x01, 0x9F,   /* Shadow Register 1 16KHz 2/8 */
                                    0x06, 0x09,   /* Shadow Register 2 */
        /* Meter LUT */             0x01, 0x67,   /* Shadow Register 1 16KHz 4/8 */
                                    0x06, 0x09,   /* Shadow Register 2 */
        /* Meter LUT */             0x01, 0x2F,   /* Shadow Register 1 16KHz 6/8 */
                                    0x08, 0x0C,   /* Shadow Register 2 */
        /* Meter LUT */             0x01, 0x2F,   /* Shadow Register 1 16KHz 8/8 */
                                    0x0C, 0x12    /* Shadow Register 2 */

};

/* AC Parameters (900) */
const VpProfileDataType AC_COEFF_900[] =
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
            /* Z FIR */             0x00, 0x4A, 0xCA, 0xBD, 0xA5, 0x43, 0x23,
                                    0x2B, 0xA3, 0xC2, 0x3B,
            /* Z IIR */             0xA3, 0xA1, 0x97, 0x9F, 0x01,
            /* GR */                0xA2, 0xB0,
            /* R IIR */             0xDC, 0x01,
            /* R FIR */             0x3A, 0x10, 0xAD, 0xD9, 0xC3, 0xA7, 0x42,
                                    0x42, 0xA3, 0xBA, 0x2B, 0xA4,
            /* B FIR */             0x00, 0xAA, 0x73, 0x4B, 0x2F, 0x9B, 0xA9,
                                    0xA5, 0xB3, 0xA6, 0x2A, 0x5C, 0x87, 0xCA,
                                    0x70,
            /* B IIR */             0x2E, 0x01,
            /* GX */                0xA3, 0xA0,
            /* X FIR */             0x27, 0x20, 0xC2, 0x2A, 0x4A, 0x2E, 0x23,
                                    0x23, 0x22, 0x2C, 0x2A, 0x87, 0x00,
            /* AISN */              0x00, 0x0A,
            /* DISN */              0x00, 0x01, 0x00, 0x07, 0x00, 0x72, 0x00,
                                    0x35, 0x00, 0x80,
            /* MTR_CFG */           0x00, 0x00,
    /* Unstructured data -------------------------------------------------- */
        /* data */                  0x00,         /* VP_CFG2 bit LRG = 0 */
        /* Meter LUT */             0x11, 0x5F,   /* Shadow Register 1 12KHz 2/8 */
                                    0x06, 0x09,   /* Shadow Register 2 */
        /* Meter LUT */             0x11, 0x2F,   /* Shadow Register 1 12KHz 4/8 */
                                    0x06, 0x09,   /* Shadow Register 2 */
        /* Meter LUT */             0x11, 0x77,   /* Shadow Register 1 12KHz 6/8 */
                                    0x08, 0x0C,   /* Shadow Register 2 */
        /* Meter LUT */             0x11, 0x77,   /* Shadow Register 1 12KHz 8/8 */
                                    0x0C, 0x12,   /* Shadow Register 2 */
        /* Meter LUT */             0x01, 0x9F,   /* Shadow Register 1 16KHz 2/8 */
                                    0x06, 0x09,   /* Shadow Register 2 */
        /* Meter LUT */             0x01, 0x67,   /* Shadow Register 1 16KHz 4/8 */
                                    0x06, 0x09,   /* Shadow Register 2 */
        /* Meter LUT */             0x01, 0x2F,   /* Shadow Register 1 16KHz 6/8 */
                                    0x08, 0x0C,   /* Shadow Register 2 */
        /* Meter LUT */             0x01, 0x2F,   /* Shadow Register 1 16KHz 8/8 */
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
        /* data */                  0x11, 0x11, /* V1 */
        /* data */                  0x11, 0xEC, /* VAS */
        /* data */                  0x01, 0x1C, /* VAS_OFFSET */
        /* data */                  0x55, 0x55, /* RFD */
        /* data */                  0x80, 0x00, /* RPTC */
        /* data */                  0x49, 0x25, /* ILA */
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
const VpProfileDataType RING_20HZ_SINE[] =
{
    /* Ring Profile */
    /* Balanced, Sinusoidal, Crest Factor 1.4142 */
    /* 20 Hz, 82.86 Vpk, 15.33 V Tip Bias, 4.67 V Ring Bias */
    /* Ring Trip: Short Loop 41.9 mA, Long Loop 8 mA */
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
            /* data */              0x46, 0xB6, /* AMPR */
            /* data */              0x0D, 0x15, /* RBA */
            /* data */              0x03, 0xFC, /* RBB */
            /* data */              0x04, 0x05, /* EBR */
            /* data */              0x35, 0xAB, /* RTSL */
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

/* Ringing 25Hz, Sine Wave, 40Vrms */
const VpProfileDataType RING_25HZ_SINE[] =
{
    /* Ring Profile */
    /* Balanced, Sinusoidal, Crest Factor 1.4142 */
    /* 25 Hz, 66.31 Vpk, 15.33 V Tip Bias, 4.67 V Ring Bias */
    /* Ring Trip: Short Loop 19 mA, Long Loop 8 mA */
    /*            Current Spike Threshold 100 mA */
    /*            Averaging Period 40 ms */
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
            /* data */              0xCC, 0xCD, /* FRQR[15:0] */
            /* data */              0x38, 0x97, /* AMPR */
            /* data */              0x0D, 0x15, /* RBA */
            /* data */              0x03, 0xFC, /* RBB */
            /* data */              0x04, 0x05, /* EBR */
            /* data */              0x18, 0x49, /* RTSL */
            /* data */              0x0A, 0x3D, /* RTLL */
            /* data */              0x00, 0x14, /* RTAP */
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

/* US Dial Tone */
const VpProfileDataType TONE_DIAL[] =
{
    /* Tone Profile */
    /* Profile header ----------------------------------------------------- */
        /* version */               0x00,
        /* type */                  0x02,       /* 0x02 = tone prof */
        /* number of sections */    0x01,
        /* content length */        0x19,       /* (2 + 21) + 2 */
    /* Section 0 ---------------------------------------------------------- */
        /* section type */          0x00,       /* 0x00 = register list */
        /* content length */        0x15,       /* 21 */
        /* Access 0 */
            /* access type */       0x01,       /* 0x01 = chan. page write */
            /* page offset */       0x03,       /* SIG_GEN register */
            /* length */            0x09,
            /* data */              0x30, 0x0B,
            /* data */              0x33, 0x33, /* FreqA = 350Hz */
            /* data */              0x13, 0xE3, /* AmpA  = -13dBm0 */
            /* data */              0x0E, 0x14, /* FreqB = 440Hz */
            /* data */              0x13, 0xE3, /* AmpB  = -13dBm0 */
            /* data */              0x00, 0x00, /* FreqC = 0Hz */
            /* data */              0x58, 0xD6, /* AmpC  = 0dBm0 */
            /* data */              0x00, 0x00, /* FreqD = 0Hz */
            /* data */              0x58, 0xD6, /* AmpD  = 0dBm0 */
    /* Unstructured data -------------------------------------------------- */
        /* data */                  0x00, 0x03  /* Generator control = AB */
};

/* US Ringback Tone */
const VpProfileDataType TONE_RINGBACK[] =
{
    /* Tone Profile */
    /* Profile header ----------------------------------------------------- */
        /* version */               0x00,
        /* type */                  0x02,       /* 0x02 = tone prof */
        /* number of sections */    0x01,
        /* content length */        0x19,       /* (2 + 21) + 2 */
    /* Section 0 ---------------------------------------------------------- */
        /* section type */          0x00,       /* 0x00 = register list */
        /* content length */        0x15,       /* 21 */
        /* Access 0 */
            /* access type */       0x01,       /* 0x01 = chan. page write */
            /* page offset */       0x03,       /* SIG_GEN register */
            /* length */            0x09,
            /* data */              0x30, 0x0E,
            /* data */              0x14, 0x7B, /* FreqA = 440Hz */
            /* data */              0x13, 0xE3, /* AmpA  = -13dBm0 */
            /* data */              0x0F, 0x5C, /* FreqB = 480Hz */
            /* data */              0x13, 0xE3, /* AmpB  = -13dBm0 */
            /* data */              0x00, 0x00, /* FreqC = 0Hz */
            /* data */              0x58, 0xD6, /* AmpC  = 0dBm0 */
            /* data */              0x00, 0x00, /* FreqD = 0Hz */
            /* data */              0x58, 0xD6, /* AmpD  = 0dBm0 */
    /* Unstructured data -------------------------------------------------- */
        /* data */                  0x00, 0x03  /* Generator control = AB */
};

/* US Busy Tone */
const VpProfileDataType TONE_BUSY[] =
{
    /* Tone Profile */
    /* Profile header ----------------------------------------------------- */
        /* version */               0x00,
        /* type */                  0x02,       /* 0x02 = tone prof */
        /* number of sections */    0x01,
        /* content length */        0x19,       /* (2 + 21) + 2 */
    /* Section 0 ---------------------------------------------------------- */
        /* section type */          0x00,       /* 0x00 = register list */
        /* content length */        0x15,       /* 21 */
        /* Access 0 */
            /* access type */       0x01,       /* 0x01 = chan. page write */
            /* page offset */       0x03,       /* SIG_GEN register */
            /* length */            0x09,
            /* data */              0x30, 0x0F,
            /* data */              0x5C, 0x29, /* FreqA = 480Hz */
            /* data */              0x13, 0xE3, /* AmpA  = -13dBm0 */
            /* data */              0x13, 0xD7, /* FreqB = 620Hz */
            /* data */              0x13, 0xE3, /* AmpB  = -13dBm0 */
            /* data */              0x00, 0x00, /* FreqC = 0Hz */
            /* data */              0x58, 0xD6, /* AmpC  = 0dBm0 */
            /* data */              0x00, 0x00, /* FreqD = 0Hz */
            /* data */              0x58, 0xD6, /* AmpD  = 0dBm0 */
    /* Unstructured data -------------------------------------------------- */
        /* data */                  0x00, 0x03  /* Generator control = AB */
};

/* US Reorder Tone */
const VpProfileDataType TONE_REORDER[] =
{
    /* Tone Profile */
    /* Profile header ----------------------------------------------------- */
        /* version */               0x00,
        /* type */                  0x02,       /* 0x02 = tone prof */
        /* number of sections */    0x01,
        /* content length */        0x19,       /* (2 + 21) + 2 */
    /* Section 0 ---------------------------------------------------------- */
        /* section type */          0x00,       /* 0x00 = register list */
        /* content length */        0x15,       /* 21 */
        /* Access 0 */
            /* access type */       0x01,       /* 0x01 = chan. page write */
            /* page offset */       0x03,       /* SIG_GEN register */
            /* length */            0x09,
            /* data */              0x30, 0x0F,
            /* data */              0x5C, 0x29, /* FreqA = 480Hz */
            /* data */              0x13, 0xE3, /* AmpA  = -13dBm0 */
            /* data */              0x13, 0xD7, /* FreqB = 620Hz */
            /* data */              0x13, 0xE3, /* AmpB  = -13dBm0 */
            /* data */              0x00, 0x00, /* FreqC = 0Hz */
            /* data */              0x58, 0xD6, /* AmpC  = 0dBm0 */
            /* data */              0x00, 0x00, /* FreqD = 0Hz */
            /* data */              0x58, 0xD6, /* AmpD  = 0dBm0 */
    /* Unstructured data -------------------------------------------------- */
        /* data */                  0x00, 0x03  /* Generator control = AB */
};

/* US Howler Tone (ROH) */
const VpProfileDataType TONE_US_HOWLER[] =
{
    /* Tone Profile */
    /* Profile header ----------------------------------------------------- */
        /* version */               0x00,
        /* type */                  0x02,       /* 0x02 = tone prof */
        /* number of sections */    0x01,
        /* content length */        0x19,       /* (2 + 21) + 2 */
    /* Section 0 ---------------------------------------------------------- */
        /* section type */          0x00,       /* 0x00 = register list */
        /* content length */        0x15,       /* 21 */
        /* Access 0 */
            /* access type */       0x01,       /* 0x01 = chan. page write */
            /* page offset */       0x03,       /* SIG_GEN register */
            /* length */            0x09,
            /* data */              0x30, 0x2C,
            /* data */              0xCC, 0xCD, /* FreqA = 1400Hz */
            /* data */              0x58, 0xD6, /* AmpA  = 0dBm0 */
            /* data */              0x41, 0xEC, /* FreqB = 2060Hz */
            /* data */              0x58, 0xD6, /* AmpB  = 0dBm0 */
            /* data */              0x4E, 0x66, /* FreqC = 2450Hz */
            /* data */              0x58, 0xD6, /* AmpC  = 0dBm0 */
            /* data */              0x53, 0x33, /* FreqD = 2600Hz */
            /* data */              0x58, 0xD6, /* AmpD  = 0dBm0 */
    /* Unstructured data -------------------------------------------------- */
        /* data */                  0x00, 0x0F  /* Generator control = ABCD */
};

/* UK Howler Tone */
const VpProfileDataType TONE_UK_HOWLER[] =
{
    /* Tone Profile */
    /* Profile header ----------------------------------------------------- */
        /* version */               0x00,
        /* type */                  0x02,       /* 0x02 = tone prof */
        /* number of sections */    0x01,
        /* content length */        0x13,       /* (2 + 15) + 2 */
    /* Section 0 ---------------------------------------------------------- */
        /* section type */          0x00,       /* 0x00 = register list */
        /* content length */        0x0F,       /* 15 */
        /* Access 0 */
            /* access type */       0x01,       /* 0x01 = chan. page write */
            /* page offset */       0x02,       /* SIG_GEN register */
            /* length */            0x06,
            /* data */              0x34, 0xCD, /* 1650Hz Offset */
            /* data */              0x20, 0x00, /* Periodic, Linear */
            /* data */              0x08, 0x31, /* FreqA = 1Hz */
            /* data */              0x1B, 0x33, /* AmpA = 850Hz */
            /* data */              0x80, 0x00, /* FreqB = Freq Modulated */
            /* data */              0x00, 0x00, /* AmpB = Sequence control */
    /* Unstructured data -------------------------------------------------- */
        /* data */                  0x00, 0x13  /* Generators = Bias+AB */
};

/* Australia Howler Tone */
const VpProfileDataType TONE_AUS_HOWLER[] =
{
    /* Tone Profile */
    /* Profile header ----------------------------------------------------- */
        /* version */               0x00,
        /* type */                  0x02,       /* 0x02 = tone prof */
        /* number of sections */    0x01,
        /* content length */        0x13,       /* (2 + 15) + 2 */
    /* Section 0 ---------------------------------------------------------- */
        /* section type */          0x00,       /* 0x00 = register list */
        /* content length */        0x0F,       /* 15 */
        /* Access 0 */
            /* access type */       0x01,       /* 0x01 = chan. page write */
            /* page offset */       0x02,       /* SIG_GEN register */
            /* length */            0x06,
            /* data */              0x4B, 0x33, /* 2350Hz Offset */
            /* data */              0x20, 0x00, /* Periodic, Linear */
            /* data */              0x08, 0x31, /* FreqA = 1Hz */
            /* data */              0x1B, 0x33, /* AmpA = 850Hz */
            /* data */              0x80, 0x00, /* FreqB = Freq Modulated */
            /* data */              0x00, 0x00, /* AmpB = Sequence control */
    /* Unstructured data -------------------------------------------------- */
        /* data */                  0x00, 0x13  /* Generators = Bias+AB */
};

/* Japan Howler Tone */
const VpProfileDataType TONE_NTT_HOWLER[] =
{
    /* Tone Profile */
    /* Profile header ----------------------------------------------------- */
        /* version */               0x00,
        /* type */                  0x02,       /* 0x02 = tone prof */
        /* number of sections */    0x01,
        /* content length */        0x11,       /* (2 + 13) + 2 */
    /* Section 0 ---------------------------------------------------------- */
        /* section type */          0x00,       /* 0x00 = register list */
        /* content length */        0x0D,       /* 13 */
        /* Access 0 */
            /* access type */       0x01,       /* 0x01 = chan. page write */
            /* page offset */       0x03,       /* SIG_GEN register */
            /* length */            0x05,
            /* data */              0x00, 0x00, /* One-shot, Linear */
            /* data */              0x00, 0x23, /* FreqA = 15s Ramp */
            /* data */              0x7F, 0xFF, /* AmpA = Maximum */
            /* data */              0x0C, 0xCD, /* FreqB = 400Hz */
            /* data */              0x80, 0x00, /* AmpB = Ampl Modulated */
    /* Unstructured data -------------------------------------------------- */
        /* data */                  0x00, 0x03  /* Generator control = AB */
};

/* Special Information Tone (Called Number Not Connected) */
const VpProfileDataType TONE_SIT[] =
{
    /* Tone Profile */
    /* Profile header ----------------------------------------------------- */
        /* version */               0x00,
        /* type */                  0x02,       /* 0x02 = tone prof */
        /* number of sections */    0x01,
        /* content length */        0x19,       /* (2 + 21) + 2 */
    /* Section 0 ---------------------------------------------------------- */
        /* section type */          0x00,       /* 0x00 = register list */
        /* content length */        0x15,       /* 21 */
        /* Access 0 */
            /* access type */       0x01,       /* 0x01 = chan. page write */
            /* page offset */       0x03,       /* SIG_GEN register */
            /* length */            0x09,
            /* data */              0x30, 0x1E,
            /* data */              0x66, 0x66, /* FreqA = 950Hz */
            /* data */              0x23, 0x5D, /* AmpA  = -8dBm0 */
            /* data */              0x2C, 0xCD, /* FreqB = 1400Hz */
            /* data */              0x23, 0x5D, /* AmpB  = -8dBm0 */
            /* data */              0x39, 0x9A, /* FreqC = 1800Hz */
            /* data */              0x23, 0x5D, /* AmpC  = -8dBm0 */
            /* data */              0x00, 0x00, /* FreqD = 0Hz */
            /* data */              0x58, 0xD6, /* AmpD  = 0dBm0 */
    /* Unstructured data -------------------------------------------------- */
        /* data */                  0x00, 0x07  /* Generator control = ABC */
};

/* A 1kHz tone at -10dBm0 */
const VpProfileDataType TONE_ONEKHZ_L[] =
{
    /* Tone Profile */
    /* Profile header ----------------------------------------------------- */
        /* version */               0x00,
        /* type */                  0x02,       /* 0x02 = tone prof */
        /* number of sections */    0x01,
        /* content length */        0x19,       /* (2 + 21) + 2 */
    /* Section 0 ---------------------------------------------------------- */
        /* section type */          0x00,       /* 0x00 = register list */
        /* content length */        0x15,       /* 21 */
        /* Access 0 */
            /* access type */       0x01,       /* 0x01 = chan. page write */
            /* page offset */       0x03,       /* SIG_GEN register */
            /* length */            0x09,
            /* data */              0x30, 0x20,
            /* data */              0x00, 0x00, /* FreqA = 1000Hz */
            /* data */              0x1C, 0x17, /* AmpA  = -10dBm0 */
            /* data */              0x00, 0x00, /* FreqB = 0Hz */
            /* data */              0x58, 0xD6, /* AmpB  = 0dBm0 */
            /* data */              0x00, 0x00, /* FreqC = 0Hz */
            /* data */              0x58, 0xD6, /* AmpC  = 0dBm0 */
            /* data */              0x00, 0x00, /* FreqD = 0Hz */
            /* data */              0x58, 0xD6, /* AmpD  = 0dBm0 */
    /* Unstructured data -------------------------------------------------- */
        /* data */                  0x00, 0x01  /* Generator control = A */
};

/* A 1kHz tone at 0dBm0 */
const VpProfileDataType TONE_ONEKHZ_H[] =
{
    /* Tone Profile */
    /* Profile header ----------------------------------------------------- */
        /* version */               0x00,
        /* type */                  0x02,       /* 0x02 = tone prof */
        /* number of sections */    0x01,
        /* content length */        0x19,       /* (2 + 21) + 2 */
    /* Section 0 ---------------------------------------------------------- */
        /* section type */          0x00,       /* 0x00 = register list */
        /* content length */        0x15,       /* 21 */
        /* Access 0 */
            /* access type */       0x01,       /* 0x01 = chan. page write */
            /* page offset */       0x03,       /* SIG_GEN register */
            /* length */            0x09,
            /* data */              0x30, 0x20,
            /* data */              0x00, 0x00, /* FreqA = 1000Hz */
            /* data */              0x58, 0xD6, /* AmpA  = 0dBm0 */
            /* data */              0x00, 0x00, /* FreqB = 0Hz */
            /* data */              0x58, 0xD6, /* AmpB  = 0dBm0 */
            /* data */              0x00, 0x00, /* FreqC = 0Hz */
            /* data */              0x58, 0xD6, /* AmpC  = 0dBm0 */
            /* data */              0x00, 0x00, /* FreqD = 0Hz */
            /* data */              0x58, 0xD6, /* AmpD  = 0dBm0 */
    /* Unstructured data -------------------------------------------------- */
        /* data */                  0x00, 0x01  /* Generator control = A */
};

/* Call Waiting Beep */
const VpProfileDataType TONE_CALLWAIT[] =
{
    /* Tone Profile */
    /* Profile header ----------------------------------------------------- */
        /* version */               0x00,
        /* type */                  0x02,       /* 0x02 = tone prof */
        /* number of sections */    0x01,
        /* content length */        0x19,       /* (2 + 21) + 2 */
    /* Section 0 ---------------------------------------------------------- */
        /* section type */          0x00,       /* 0x00 = register list */
        /* content length */        0x15,       /* 21 */
        /* Access 0 */
            /* access type */       0x01,       /* 0x01 = chan. page write */
            /* page offset */       0x03,       /* SIG_GEN register */
            /* length */            0x09,
            /* data */              0x30, 0x0E,
            /* data */              0x14, 0x7B, /* FreqA = 440Hz */
            /* data */              0x13, 0xE3, /* AmpA  = -13dBm0 */
            /* data */              0x00, 0x00, /* FreqB = 0Hz */
            /* data */              0x58, 0xD6, /* AmpB  = 0dBm0 */
            /* data */              0x00, 0x00, /* FreqC = 0Hz */
            /* data */              0x58, 0xD6, /* AmpC  = 0dBm0 */
            /* data */              0x00, 0x00, /* FreqD = 0Hz */
            /* data */              0x58, 0xD6, /* AmpD  = 0dBm0 */
    /* Unstructured data -------------------------------------------------- */
        /* data */                  0x00, 0x01  /* Generator control = A */
};

/* Caller ID Alert Tone */
const VpProfileDataType TONE_CLI[] =
{
    /* Tone Profile */
    /* Profile header ----------------------------------------------------- */
        /* version */               0x00,
        /* type */                  0x02,       /* 0x02 = tone prof */
        /* number of sections */    0x01,
        /* content length */        0x19,       /* (2 + 21) + 2 */
    /* Section 0 ---------------------------------------------------------- */
        /* section type */          0x00,       /* 0x00 = register list */
        /* content length */        0x15,       /* 21 */
        /* Access 0 */
            /* access type */       0x01,       /* 0x01 = chan. page write */
            /* page offset */       0x03,       /* SIG_GEN register */
            /* length */            0x09,
            /* data */              0x30, 0x44,
            /* data */              0x28, 0xF6, /* FreqA = 2130Hz */
            /* data */              0x13, 0xE3, /* AmpA  = -13dBm0 */
            /* data */              0x58, 0x00, /* FreqB = 2750Hz */
            /* data */              0x13, 0xE3, /* AmpB  = -13dBm0 */
            /* data */              0x00, 0x00, /* FreqC = 0Hz */
            /* data */              0x58, 0xD6, /* AmpC  = 0dBm0 */
            /* data */              0x00, 0x00, /* FreqD = 0Hz */
            /* data */              0x58, 0xD6, /* AmpD  = 0dBm0 */
    /* Unstructured data -------------------------------------------------- */
        /* data */                  0x00, 0x03  /* Generator control = AB */
};

/************** Cadence_Definitions **************/

/* US Dial Tone Cadence */
const VpProfileDataType TONE_CAD_DIAL[] =
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
        /* data */                  0x05, 0x03, /* 00 - Generator Ctrl */
    /* Unstructured data -------------------------------------------------- */
        /* none */
};

/* Stutter Dial Tone Cadence */
const VpProfileDataType TONE_CAD_STUTTER[] =
{
    /* Cadence Profile */
    /* Profile header ----------------------------------------------------- */
        /* version */               0x00,
        /* type */                  0x03,       /* 0x03 = tone cadence */
        /* number of sections */    0x01,
        /* content length */        0x10,       /* (2 + 14) */
    /* Section 0 ---------------------------------------------------------- */
        /* section type */          0x02,       /* 0x02 = sequence */
        /* content length */        0x0E,       /* 14 */
        /* data */                  0x00, 0x05, /* sequence length = 6 */
        /* data */                  0x05, 0x03, /* 00 - Generator Ctrl */
        /* data */                  0x40, 0x64, /* 01 - Sequential Delay */
        /* data */                  0x05, 0x00, /* 02 - Generator Ctrl */
        /* data */                  0x40, 0x64, /* 03 - Sequential Delay */
        /* data */                  0x10, 0x60, /* 04 - Branch to 00 */
        /* data */                  0x05, 0x03, /* 05 - Generator Ctrl */
    /* Unstructured data -------------------------------------------------- */
        /* none */
};

/* US Ringback Tone Cadence */
const VpProfileDataType TONE_CAD_RINGBACK[] =
{
    /* Cadence Profile */
    /* Profile header ----------------------------------------------------- */
        /* version */               0x00,
        /* type */                  0x03,       /* 0x03 = tone cadence */
        /* number of sections */    0x01,
        /* content length */        0x0E,       /* (2 + 12) */
    /* Section 0 ---------------------------------------------------------- */
        /* section type */          0x02,       /* 0x02 = sequence */
        /* content length */        0x0C,       /* 12 */
        /* data */                  0x00, 0x04, /* sequence length = 5 */
        /* data */                  0x05, 0x03, /* 00 - Generator Ctrl */
        /* data */                  0x47, 0xD0, /* 01 - Sequential Delay */
        /* data */                  0x05, 0x00, /* 02 - Generator Ctrl */
        /* data */                  0x4F, 0xA0, /* 03 - Sequential Delay */
        /* data */                  0x10, 0x00, /* 04 - Branch to 00 */
    /* Unstructured data -------------------------------------------------- */
        /* none */
};

/* US Busy Tone Cadence */
const VpProfileDataType TONE_CAD_BUSY[] =
{
    /* Cadence Profile */
    /* Profile header ----------------------------------------------------- */
        /* version */               0x00,
        /* type */                  0x03,       /* 0x03 = tone cadence */
        /* number of sections */    0x01,
        /* content length */        0x0E,       /* (2 + 12) */
    /* Section 0 ---------------------------------------------------------- */
        /* section type */          0x02,       /* 0x02 = sequence */
        /* content length */        0x0C,       /* 12 */
        /* data */                  0x00, 0x04, /* sequence length = 5 */
        /* data */                  0x05, 0x03, /* 00 - Generator Ctrl */
        /* data */                  0x41, 0xF4, /* 01 - Sequential Delay */
        /* data */                  0x05, 0x00, /* 02 - Generator Ctrl */
        /* data */                  0x41, 0xF4, /* 03 - Sequential Delay */
        /* data */                  0x10, 0x00, /* 04 - Branch to 00 */
    /* Unstructured data -------------------------------------------------- */
        /* none */
};

/* US Reorder Tone Cadence */
const VpProfileDataType TONE_CAD_REORDER[] =
{
    /* Cadence Profile */
    /* Profile header ----------------------------------------------------- */
        /* version */               0x00,
        /* type */                  0x03,       /* 0x03 = tone cadence */
        /* number of sections */    0x01,
        /* content length */        0x0E,       /* (2 + 12) */
    /* Section 0 ---------------------------------------------------------- */
        /* section type */          0x02,       /* 0x02 = sequence */
        /* content length */        0x0C,       /* 12 */
        /* data */                  0x00, 0x04, /* sequence length = 5 */
        /* data */                  0x05, 0x03, /* 00 - Generator Ctrl */
        /* data */                  0x40, 0xFA, /* 01 - Sequential Delay */
        /* data */                  0x05, 0x00, /* 02 - Generator Ctrl */
        /* data */                  0x40, 0xFA, /* 03 - Sequential Delay */
        /* data */                  0x10, 0x00, /* 04 - Branch to 00 */
    /* Unstructured data -------------------------------------------------- */
        /* none */
};

/* US Howler Tone Cadence (ROH) */
const VpProfileDataType TONE_CAD_US_HOWLER[] =
{
    /* Cadence Profile */
    /* Profile header ----------------------------------------------------- */
        /* version */               0x00,
        /* type */                  0x03,       /* 0x03 = tone cadence */
        /* number of sections */    0x01,
        /* content length */        0x0E,       /* (2 + 12) */
    /* Section 0 ---------------------------------------------------------- */
        /* section type */          0x02,       /* 0x02 = sequence */
        /* content length */        0x0C,       /* 12 */
        /* data */                  0x00, 0x04, /* sequence length = 5 */
        /* data */                  0x05, 0x0F, /* 00 - Generator Ctrl */
        /* data */                  0x40, 0x64, /* 01 - Sequential Delay */
        /* data */                  0x05, 0x00, /* 02 - Generator Ctrl */
        /* data */                  0x40, 0x64, /* 03 - Sequential Delay */
        /* data */                  0x10, 0x00, /* 04 - Branch to 00 */
    /* Unstructured data -------------------------------------------------- */
        /* none */
};

/* UK Howler Tone Cadence */
const VpProfileDataType TONE_CAD_UK_HOWLER[] =
{
    /* Cadence Profile */
    /* Profile header ----------------------------------------------------- */
        /* version */               0x00,
        /* type */                  0x03,       /* 0x03 = Tone cadence prof */
        /* number of sections */    0x01,
        /* content length */        0x38,       /* (2 + 54) */
    /* Section 0 ---------------------------------------------------------- */
        /* section type */          0x02,       /* 0x02 = sequence */
        /* content length */        0x36,       /* 54 */
        /* data */                  0x00, 0x19, /* sequence length = 26 */
        /* data */                  0x05, 0x13,
        /* data */                  0x82, 0x04,
        /* data */                  0x43, 0xE8,
        /* data */                  0x82, 0x8A,
        /* data */                  0x43, 0xE8,
        /* data */                  0x83, 0x33,
        /* data */                  0x43, 0xE8,
        /* data */                  0x84, 0x07,
        /* data */                  0x43, 0xE8,
        /* data */                  0x85, 0x12,
        /* data */                  0x43, 0xE8,
        /* data */                  0x86, 0x62,
        /* data */                  0x43, 0xE8,
        /* data */                  0x88, 0x09,
        /* data */                  0x43, 0xE8,
        /* data */                  0x8A, 0x1E,
        /* data */                  0x43, 0xE8,
        /* data */                  0x8C, 0xBD,
        /* data */                  0x43, 0xE8,
        /* data */                  0x90, 0x09,
        /* data */                  0x43, 0xE8,
        /* data */                  0x94, 0x30,
        /* data */                  0x43, 0xE8,
        /* data */                  0x99, 0x6A,
        /* data */                  0x43, 0xE8,
        /* data */                  0x9F, 0xFF
    /* Unstructured data -------------------------------------------------- */
        /* none */
};

/* Australia Howler Tone Cadence */
const VpProfileDataType TONE_CAD_AUS_HOWLER[] =
{
    /* Cadence Profile */
    /* Profile header ----------------------------------------------------- */
        /* version */               0x00,
        /* type */                  0x03,       /* 0x03 = Tone cadence prof */
        /* number of sections */    0x01,
        /* content length */        0x44,       /* (2 + 66) */
    /* Section 0 ---------------------------------------------------------- */
        /* section type */          0x02,       /* 0x02 = sequence */
        /* content length */        0x42,       /* 66 */
        /* data */                  0x00, 0x1F, /* sequence length = 32 */
        /* data */                  0x05, 0x13,
        /* data */                  0x81, 0x03,
        /* data */                  0x43, 0xE8,
        /* data */                  0x81, 0x46,
        /* data */                  0x43, 0xE8,
        /* data */                  0x81, 0x9B,
        /* data */                  0x43, 0xE8,
        /* data */                  0x82, 0x05,
        /* data */                  0x43, 0xE8,
        /* data */                  0x82, 0x8B,
        /* data */                  0x43, 0xE8,
        /* data */                  0x83, 0x33,
        /* data */                  0x43, 0xE8,
        /* data */                  0x84, 0x07,
        /* data */                  0x43, 0xE8,
        /* data */                  0x85, 0x12,
        /* data */                  0x43, 0xE8,
        /* data */                  0x86, 0x62,
        /* data */                  0x43, 0xE8,
        /* data */                  0x88, 0x0A,
        /* data */                  0x43, 0xE8,
        /* data */                  0x8A, 0x1E,
        /* data */                  0x43, 0xE8,
        /* data */                  0x8C, 0xBD,
        /* data */                  0x43, 0xE8,
        /* data */                  0x90, 0x0A,
        /* data */                  0x43, 0xE8,
        /* data */                  0x94, 0x31,
        /* data */                  0x43, 0xE8,
        /* data */                  0x99, 0x6B,
        /* data */                  0x43, 0xE8,
        /* data */                  0x9F, 0xFF
    /* Unstructured data -------------------------------------------------- */
        /* none */
};

/* Japan Howler Tone Cadence */
const VpProfileDataType TONE_CAD_NTT_HOWLER[] =
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
        /* data */                  0x05, 0x03, /* 00 - Generator Ctrl */
    /* Unstructured data -------------------------------------------------- */
        /* none */
};

/* Special Information Tone Cadence */
const VpProfileDataType TONE_CAD_SIT[] =
{
    /* Cadence Profile */
    /* Profile header ----------------------------------------------------- */
        /* version */               0x00,
        /* type */                  0x03,       /* 0x03 = tone cadence */
        /* number of sections */    0x01,
        /* content length */        0x12,       /* (2 + 16) */
    /* Section 0 ---------------------------------------------------------- */
        /* section type */          0x02,       /* 0x02 = sequence */
        /* content length */        0x10,       /* 16 */
        /* data */                  0x00, 0x06, /* sequence length = 7 */
        /* data */                  0x05, 0x01, /* 00 - Generator Ctrl */
        /* data */                  0x41, 0x2C, /* 01 - Sequential Delay */
        /* data */                  0x05, 0x02, /* 02 - Generator Ctrl */
        /* data */                  0x41, 0x2C, /* 03 - Sequential Delay */
        /* data */                  0x05, 0x04, /* 04 - Generator Ctrl */
        /* data */                  0x41, 0x2C, /* 05 - Sequential Delay */
        /* data */                  0x05, 0x00, /* 06 - Generator Ctrl */
    /* Unstructured data -------------------------------------------------- */
        /* none */
};

/* Standard Ringing Cadence */
const VpProfileDataType RING_CAD_STD[] =
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

/* Ringing Cadence with CallerID */
const VpProfileDataType RING_CAD_CID[] =
{
    /* Cadence Profile */
    /* Profile header ----------------------------------------------------- */
        /* version */               0x00,
        /* type */                  0x08,       /* 0x08 = ring cadence */
        /* number of sections */    0x01,
        /* content length */        0x28,       /* (2 + 38) */
    /* Section 0 ---------------------------------------------------------- */
        /* section type */          0x02,       /* 0x02 = sequence */
        /* content length */        0x26,       /* 38 */
        /* data */                  0x00, 0x11, /* sequence length = 18 */
        /* data */                  0x02, 0x86, /* 00 - Line State */
        /* data */                  0x01, 0x3B,
        /* data */                  0x47, 0xD0, /* 01 - Sequential Delay */
        /* data */                  0x02, 0x83, /* 02 - Line State */
        /* data */                  0x01, 0x3B,
        /* data */                  0x0A, 0x00, /* 03 - Relative Time Marker */
        /* data */                  0x40, 0xC8, /* 04 - Sequential Delay */
        /* data */                  0x02, 0x85, /* 05 - Line State */
        /* data */                  0x01, 0x3A,
        /* data */                  0x01, 0x22, /* 06 - Start CallerID */
        /* data */                  0x6F, 0xA0, /* 07 - Relative Delay */
        /* data */                  0x02, 0x86, /* 08 - Line State */
        /* data */                  0x01, 0x3B,
        /* data */                  0x47, 0xD0, /* 09 - Sequential Delay */
        /* data */                  0x02, 0x83, /* 10 - Line State */
        /* data */                  0x01, 0x3B,
        /* data */                  0x4F, 0xA0, /* 11 - Sequential Delay */
        /* data */                  0x10, 0x0B, /* 12 - Branch to 08 */
    /* Unstructured data -------------------------------------------------- */
        /* none */
};

/* Ringing Always On */
const VpProfileDataType RING_CAD_ON[] =
{
    /* Cadence Profile */
    /* Profile header ----------------------------------------------------- */
        /* version */               0x00,
        /* type */                  0x08,       /* 0x08 = ring cadence */
        /* number of sections */    0x01,
        /* content length */        0x08,       /* (2 + 6) */
    /* Section 0 ---------------------------------------------------------- */
        /* section type */          0x02,       /* 0x02 = sequence */
        /* content length */        0x06,       /* 6 */
        /* data */                  0x00, 0x01, /* sequence length = 2 */
        /* data */                  0x02, 0x86, /* 00 - Line State */
        /* data */                  0x01, 0x3B,
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

/* UK Caller ID Type I */
const VpProfileDataType CID_TYPE1_UK[] =
{
    /* Profile header ----------------------------------------------------- */
        /* version */               0x01,
        /* type */                  0x05,       /* 0x05 = caller ID profile */
        /* number of sections */    0x03,
        /* content length */        0x55,       /* 85 */
    /* Section 0 ---------------------------------------------------------- */
        /* section type */          0x02,       /* 0x02 = sequencer program */
        /* content length */        0x22,       /* 34 */
        /* data */                  0x00, 0x0F, /* sequence length = 16 */
        /* data */                  0x40, 0x32, /* Silence 50ms */
        /* data */                  0x03, 0x00, /* Line Reversal */
        /* data */                  0x40, 0x64, /* Silence 100ms */
        /* data */                  0x01, 0x3D, /* Alert tone 100ms */
        /* data */                  0x40, 0x64, /* - TONE_CLI */
        /* data */                  0x05, 0x00,
        /* data */                  0x01, 0x2F, /* Mask hooks 20ms, */
        /* data */                  0x40, 0x14, /*    Silence 100ms */
        /* data */                  0x01, 0x2F,
        /* data */                  0x40, 0x50,
        /* data */                  0x01, 0x26, /* Send FSK */
        /* data */                  0x0B, 0x01,
        /* data */                  0x09, 0x00,
        /* data */                  0x01, 0x27,
        /* data */                  0x40, 0xC8, /* Silence 200ms */
        /* data */                  0x03, 0x00, /* Line Reversal */
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
                                    0x27, 0xAE, /* amplitude = -7dBm0 */
    /* Section 2 ---------------------------------------------------------- */
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
                                    0x00, 0x78, /* 120-bit channel seizure */
                                    0x00, 0x40, /* 64-bit mark period */
                                    0x00, 0x00,
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

/* Metering Profile 12 kHz Tone */
const VpProfileDataType METER_12KHZ[] =
{
    /* Metering Profile */
    /* 12KHz Tone Metering */
    /* Ramp Time: 10ms, Amplitude: 2000mVrms (Normal Gain) */
    /* Profile header ----------------------------------------------------- */
        /* version */               0x00,
        /* type */                  0x07,       /* 0x07 = Metering profile */
        /* number of sections */    0x00,
        /* content length */        0x06,
    /* Unstructured data -------------------------------------------------- */
        /* data */                  0x00, 0x02, /* MTR_TYPE */
        /* data */                  0x00, 0x0A, /* RAMP_TIME */
        /* data */                  0x09, 0x94  /* RAMP_STEP */
};

/* Metering Profile Polarity Reversal */
const VpProfileDataType METER_POLREV[] =
{
    /* Metering Profile */
    /* Polrev Pulse Metering */
    /* Profile header ----------------------------------------------------- */
        /* version */               0x00,
        /* type */                  0x07,       /* 0x07 = Metering profile */
        /* number of sections */    0x00,
        /* content length */        0x02,
    /* Unstructured data -------------------------------------------------- */
        /* data */                  0x00, 0x00  /* MTR_TYPE */
};

int dev_profile_size = sizeof(VE792_DEV_PROFILE);
int dc_profile_size = sizeof(VE792_DC_COEFF);
int ac_profile_size = sizeof(VE792_AC_COEFF_600);
int ring_profile_size = sizeof(RING_20HZ_SINE);

/* end of file profile_79238.c */
