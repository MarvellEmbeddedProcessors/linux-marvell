/*
 * All the Stream id calculation based on ATF configuration.
 * Reference for this calculation can be found under
 * <atf root dir>/drivers/marvell/mochi/<required setup>.h
 */

/* AP XOR stream IDs: 0x100-0xFFF */
#define MVEBU_STREAM_ID_AP0_XOR0		0x100
/* max stream IDs per AP */
#define MVEBU_STREAM_ID_AP_MAX			0x200
#define MVEBU_STREAM_ID_APx_XORx(x, ap)		(MVEBU_STREAM_ID_AP0_XOR0 + \
						(ap * MVEBU_STREAM_ID_AP_MAX) + x)

/* CP stream ID base */
#define MVEBU_STREAM_ID_BASE			0x40

/* CP max stream IDs */
#define MVEBU_MAX_STREAM_ID_PER_CP		0x10

/* CP XOR0 stream ID offset */
#define MVEBU_STREAM_ID_CP_XOR0_OFFSET		0x2

/* CP XOR0 stream ID: 0x42 */
#define MVEBU_STREAM_ID_CP0_XOR0		(MVEBU_STREAM_ID_BASE + \
						MVEBU_STREAM_ID_CP_XOR0_OFFSET)

#define MVEBU_STREAM_ID_CPx_XORx(cp_num, xor_num)	\
	(MVEBU_STREAM_ID_CP0_XOR0 + \
	((cp_num  % 4) * MVEBU_MAX_STREAM_ID_PER_CP) + xor_num)


