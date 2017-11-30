/*
 * All the Stream id calculation based on ATF configuration.
 * Reference for this calculation can be found under
 * <atf root dir>/drivers/marvell/mochi/<required setup>.h
 */

/* AP XOR stream IDs: 0xa0-0xaf */
#define MVEBU_STREAM_ID_XOR0		0xa0
#define MVEBU_STREAM_ID_XORx(x, ap)	(MVEBU_STREAM_ID_XOR0 + ap * 4 + x)

/* CP(n) XOR0 stream ID: 0x89 + 11*n */
#define MVEBU_STREAM_ID_CP0_XOR0	0x89
/*
 * There are 12 required stream ids per CP,
 * but only 11 of them are unique (two SATA ports get the same stream id).
 */
#define STREAM_PER_CP_COUNT		11
#define MVEBU_STREAM_ID_CPx_XORx(xor_num, cp110_num) \
		(MVEBU_STREAM_ID_CP0_XOR0 + (cp110_num * STREAM_PER_CP_COUNT) + xor_num)
