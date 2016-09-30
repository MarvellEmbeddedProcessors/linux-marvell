#ifndef __LINUX_MTD_SPI_GD5F_SPINAND_H_
#define __LINUX_MTD_SPI_GD5F_SPINAND_H_

/* SPI NAND Command Set Definitions */
enum {
	SPI_NAND_WRITE_ENABLE			= 0x06,
	SPI_NAND_WRITE_DISABLE			= 0x04,
	SPI_NAND_GET_FEATURE_INS		= 0x0F,
	SPI_NAND_SET_FEATURE			= 0x1F,
	SPI_NAND_PAGE_READ_INS			= 0x13,
	SPI_NAND_READ_CACHE_INS			= 0x03,
	SPI_NAND_FAST_READ_CACHE_INS		= 0x0B,
	SPI_NAND_READ_CACHE_X2_INS		= 0x3B,
	SPI_NAND_READ_CACHE_X4_INS		= 0x6B,
	SPI_NAND_READ_CACHE_DUAL_IO_INS		= 0xBB,
	SPI_NAND_READ_CACHE_QUAD_IO_INS		= 0xEB,
	SPI_NAND_READ_ID			= 0x9F,
	SPI_NAND_PROGRAM_LOAD_INS		= 0x02,
	SPI_NAND_PROGRAM_LOAD4_INS		= 0x32,
	SPI_NAND_PROGRAM_EXEC_INS		= 0x10,
	SPI_NAND_PROGRAM_LOAD_RANDOM_INS	= 0x84,
	SPI_NAND_PROGRAM_LOAD_RANDOM4_INS	= 0xC4,
	SPI_NAND_BLOCK_ERASE_INS		= 0xD8,
	SPI_NAND_RESET				= 0xFF
};

/* Feature registers */
enum feature_register {
	SPI_NAND_PROTECTION_REG_ADDR	= 0xA0,
	SPI_NAND_FEATURE_EN_REG_ADDR	= 0xB0,
	SPI_NAND_STATUS_REG_ADDR	= 0xC0,
	SPI_NAND_DS_REG_ADDR		= 0xD0,
};

/*
 * Status register description: SPI_NAND_STATUS_REG_ADDR
 *
 * SR7 - reserved
 * SR6 - ECC status 2
 * SR5 - ECC status 1
 * SR4 - ECC status 0
 *
 * ECCS provides ECC status as follows:
 * 000b = No bit errors were detected during the previous read algorithm.
 * 001b = bit error was detected and corrected, error bit number < 3.
 * 010b = bit error was detected and corrected, error bit number = 4.
 * 011b = bit error was detected and corrected, error bit number = 5.
 * 100b = bit error was detected and corrected, error bit number = 6.
 * 101b = bit error was detected and corrected, error bit number = 7.
 * 110b = bit error was detected and corrected, error bit number = 8.
 * 111b = bit error was detected and not corrected.
 *
 * SR3 - P_Fail Program fail
 * SR2 - E_Fail Erase fail
 * SR1 - WEL - Write enable latch
 * SR0 - OIP - Operation in progress
 */
enum {
	SPI_NAND_ECCS2	= 0x40,
	SPI_NAND_ECCS1	= 0x20,
	SPI_NAND_ECCS0	= 0x10,
	SPI_NAND_PF	= 0x08,
	SPI_NAND_EF	= 0x04,
	SPI_NAND_WEL	= 0x02,
	SPI_NAND_OIP	= 0x01
};

enum {
	SPI_NAND_ECC_NO_ERRORS = 0x00,
	SPI_NAND_ECC_UNABLE_TO_CORRECT =
		SPI_NAND_ECCS0 | SPI_NAND_ECCS1 | SPI_NAND_ECCS2
};

/*
 * Feature enable register description: SPI_NAND_FEATURE_EN_REG_ADDR
 *
 * FR7 - OTP protect
 * FR6 - OTP enable
 * FR5 - reserved
 * FR4 - ECC enable
 * FR3 - reserved
 * FR2 - reserved
 * FR1 - reserved
 * FR0 - Quad operation enable
 */
enum {
	SPI_NAND_QUAD_EN	= 0x01,
	SPI_NAND_ECC_EN		= 0x10,
	SPI_NAND_OTP_EN		= 0x40,
	SPI_NAND_OTP_PRT	= 0x80
};

/*
 * Protection register description: SPI_NAND_PROTECTION_REG_ADDR
 *
 * BL7 - BRWD
 * BL6 - reserved
 * BL5 - Block protect 2
 * BL4 - Block protect 1
 * BL3 - Block protect 0
 * BL2 - INV
 * BL1 - CMP
 * BL0 - reserved
 */
enum {
	SPI_NAND_BRWD	= 0x80,
	SPI_NAND_BP2	= 0x20,
	SPI_NAND_BP1	= 0x10,
	SPI_NAND_BP0	= 0x08,
	SPI_NAND_INV	= 0x04,
	SPI_NAND_CMP	= 0x02
};

enum protected_rows {
	/* All unlocked : 000xx */
	SPI_NAND_PROTECTED_ALL_UNLOCKED	= 0x00,
	SPI_NAND_UPPER_1_64_LOCKED	= 0x04,
	SPI_NAND_LOWER_63_64_LOCKED	= 0x05,
	SPI_NAND_LOWER_1_64_LOCKED	= 0x06,
	SPI_NAND_UPPER_63_64_LOCKED	= 0x07,
	SPI_NAND_UPPER_1_32_LOCKED	= 0x08,
	SPI_NAND_LOWER_31_32_LOCKED	= 0x09,
	SPI_NAND_LOWER_1_32_LOCKED	= 0x0A,
	SPI_NAND_UPPER_31_32_LOCKED	= 0x0B,
	SPI_NAND_UPPER_1_16_LOCKED	= 0x0C,
	SPI_NAND_LOWER_15_16_LOCKED	= 0x0D,
	SPI_NAND_LOWER_1_16_LOCKED	= 0x0E,
	SPI_NAND_UPPER_15_16_LOCKED	= 0x0F,
	SPI_NAND_UPPER_1_8_LOCKED	= 0x10,
	SPI_NAND_LOWER_7_8_LOCKED	= 0x11,
	SPI_NAND_LOWER_1_8_LOCKED	= 0x12,
	SPI_NAND_UPPER_7_8_LOCKED	= 0x13,
	SPI_NAND_UPPER_1_4_LOCKED	= 0x14,
	SPI_NAND_LOWER_3_4_LOCKED	= 0x15,
	SPI_NAND_LOWER_1_4_LOCKED	= 0x16,
	SPI_NAND_UPPER_3_4_LOCKED	= 0x17,
	SPI_NAND_UPPER_1_2_LOCKED	= 0x18,
	SPI_NAND_BLOCK_0_LOCKED		= 0x19,
	SPI_NAND_LOWER_1_2_LOCKED	= 0x1A,
	SPI_NAND_BLOCK_0_LOCKED1	= 0x1B,
	/* All locked (default) : 111xx */
	SPI_NAND_PROTECTED_ALL_LOCKED	= 0x1C,

};

struct spinand_info {
	struct nand_ecclayout *ecclayout;
	struct spi_device *spi;
	void *priv;
};

struct spinand_state {
	/* Offset in page */
	u32 col;
	/* Page number */
	u32 row;
	/* TODO */
	int buf_ptr;
	/* Data buffer */
	u8 *buf;
};

struct spinand_cmd {
	/* Command byte */
	u8 cmd;
	/* Number of address bytes */
	u32 n_addr;
	/* Address bytes */
	u8 addr[3];
	/* Number of dummy bytes */
	u32 n_dummy;
	/* Number of TX bytes */
	u32 n_tx;
	/* Bytes to be written */
	u8 *tx_buf;
	/* Number of RX bytes */
	u32 n_rx;
	/* Received bytes */
	u8 *rx_buf;
};

#endif /* __LINUX_MTD_SPI_GD5F_SPINAND_H_ */
