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
#define MVEBU_STREAM_ID_BASE			0x80

/* CP XOR0 stream ID offset */
#define MVEBU_STREAM_ID_CP_XOR0_OFFSET		0x9

/* CP XOR0 stream ID: 0x89 */
#define MVEBU_STREAM_ID_CP_XOR0			(MVEBU_STREAM_ID_BASE + MVEBU_STREAM_ID_CP_XOR0_OFFSET)

#define MVEBU_STREAM_ID_CP_XORx(xor_num)	(MVEBU_STREAM_ID_CP_XOR0 + xor_num)
