/**
 * core.h - Marvell Central IP usb3 core header
 *
 * Copyright (C) 2013 Marvell Inc.
 *
 * Authors: Lei Wen <leiwen@marvell.com>
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions, and the following disclaimer,
 *    without modification.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. The names of the above-listed copyright holders may not be used
 *    to endorse or promote products derived from this software without
 *    specific prior written permission.
 *
 * ALTERNATIVELY, this software may be distributed under the terms of the
 * GNU General Public License ("GPL") version 2, as published by the Free
 * Software Foundation.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS
 * IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
 * LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */
#ifndef __DRIVERS_USB_MVC2_H
#define __DRIVERS_USB_MVC2_H

#define USB3_IP_VER_Z2	0x215
#define USB3_IP_VER_Z3	0x220
#define USB3_IP_VER_A0	0x221

#define MVCP_DEV_INFO   0x0

#define MVCP_EP_COUNT	2

#define MVCP_LFPS_SIGNAL(n)     (0x8 + ((n-1) << 2))
#define MVCP_COUNTER_PULSE      0x20
#define MVCP_REF_INT            0x24
#define MVCP_REF_INTEN          0x28
	#define MVCP_REF_INTEN_USB2_CNT     (1 << 31)
	#define MVCP_REF_INTEN_USB2_DISCNT  (1 << 30)
	#define MVCP_REF_INTEN_RESUME       (1 << 29)
	#define MVCP_REF_INTEN_SUSPEND      (1 << 28)
	#define MVCP_REF_INTEN_RESET        (1 << 27)
	#define MVCP_REF_INTEN_POWERON      (1 << 26)
	#define MVCP_REF_INTEN_POWEROFF     (1 << 25)

#define MVCP_GLOBAL_CONTROL     0x2C
	#define MVCP_GLOBAL_CONTROL_SOFT_CONNECT    (1 << 31)
	#define MVCP_GLOBAL_CONTROL_SOFT_RESET      (1 << 30)
	#define MVCP_GLOBAL_CONTROL_SAFE            (1 << 29)
	#define MVCP_GLOBAL_CONTROL_PHYRESET        (1 << 28)
	#define MVCP_GLOBAL_CONTROL_SOCACCESS       (1 << 27)
	#define MVCP_GLOBAL_CONTROL_SS_VBUS         (1 << 3)
	#define MVCP_GLOBAL_CONTROL_POWERPRESENT    (1 << 2)
	#define MVCP_GLOBAL_CONTROL_USB2_BUS_RESET  (1 << 0)

#define MVCP_SYSTEM_DEBUG       0x30

#define MVCP_POWER_MANAGEMENT_DEVICE    0xC8
#define MVCP_POWER_MANAGEMENT_SOC       0xCC
#define MVCP_LOW_POWER_STATUS           0xD0
#define MVCP_SOFTWARE_RESET             0xD4

#define MVCP_TOP_INT_STATUS		0xD8
	#define MVCP_TOP_INT_SS_EP		(0x1<<6)
	#define MVCP_TOP_INT_VBUS		(0x1<<5)
	#define MVCP_TOP_INT_PME		(0x1<<4)
	#define MVCP_TOP_INT_REF		(0x1<<3)
	#define MVCP_TOP_INT_SS_CORE		(0x1<<2)
	#define MVCP_TOP_INT_SS_SYS		(0x1<<1)
	#define MVCP_TOP_INT_SS_AXI		(0x1<<1)
	#define MVCP_TOP_INT_USB2		(0x1<<0)

#define MVCP_TOP_INT_EN                 0xDC

#define MVCP_TIMER_TIMEOUT(n)           (0x194 + ((n-1) << 2))
#define MVCP_LFPS_TX_CONFIG             0x19C
#define MVCP_LFPS_RX_CONFIG             0x1A0
#define MVCP_LFPS_WR_TRESET             0x1A4
#define MVCP_COUNTER_DELAY_TX           0x1A8
#define MVCP_DEV_USB_ADDRESS            0x320
#define MVCP_FUNCTION_WAKEUP            0x324

#define MVCP_ENDPOINT_0_CONFIG          0x328
	#define MVCP_ENDPOINT_0_CONFIG_CHG_STATE    (1 << 7)

#define MVCP_OUT_ENDPOINT_CONFIG_BASE   0x32C

#define MVCP_IN_ENDPOINT_CONFIG_BASE    0x368
	#define MVCP_EP_BURST(x)            ((x + 1) << 24)
	#define MVCP_EP_MAX_PKT(x)          ((((x >> 8) & 0x7)  \
											<< 8) | ((x & 0xff) \
											<< 16))
	#define MVCP_EP_RESETSEQ            (1 << 14)
	#define MVCP_EP_STALL               (1 << 11)
	#define MVCP_EP_BULK_STREAM_EN      (1 << 7)
	#define MVCP_EP_ENABLE              (1 << 6)
	#define MVCP_EP_TYPE_INT            (0x3 << 4)
	#define MVCP_EP_TYPE_BLK            (0x2 << 4)
	#define MVCP_EP_TYPE_ISO            (0x1 << 4)
	#define MVCP_EP_TYPE_CTL            (0x0 << 4)
	#define MVCP_EP_TYPE(x)             ((x & 0x3) << 4)
	#define MVCP_EP_NUM(x)              (x & 0xf)

static inline unsigned int epcon(int n, int in)
{
	if (n == 0)
		return MVCP_ENDPOINT_0_CONFIG;

	if (in)
		return MVCP_IN_ENDPOINT_CONFIG_BASE + ((n - 1) << 2);
	else
		return MVCP_OUT_ENDPOINT_CONFIG_BASE + ((n - 1) << 2);
}

#define MVCP_PHY        0x3A4
	#define MVCP_PHY_LTSSM_MASK 0x1f
		#define LTSSM_DISABLED  0x1
		#define LTSSM_U0        0xc
		#define LTSSM_U1        0xd
		#define LTSSM_U2        0xe
		#define LTSSM_U3        0xf

#define MVCP_SS_CORE_INT        0x3B8
	#define MVCP_SS_CORE_INT_SETUP      (1 << 14)
	#define MVCP_SS_CORE_INT_HOT_RESET  (1 << 11)
	#define MVCP_SS_CORE_INT_LTSSM_CHG  (1 << 8)

#define MVCP_SS_CORE_INTEN      0x3BC
	#define MVCP_SS_CORE_INTEN_SETUP        (1 << 14)
	#define MVCP_SS_CORE_INTEN_HOT_RESET    (1 << 11)
	#define MVCP_SS_CORE_INTEN_LTSSM_CHG    (1 << 8)

/* IP_VERSION <= USB3_IP_VER_Z2 */
#define EP_IN_BINTERVAL_REG_1_2_3         0x03C0
#define EP_IN_BINTERVAL_REG_4_5_6_7       0x03C4
#define EP_IN_BINTERVAL_REG_8_9_10_11     0x03C8
#define EP_IN_BINTERVAL_REG_12_13_14_15   0x03CC
#define EP_OUT_BINTERVAL_REG_1_2_3        0x03D0
#define EP_OUT_BINTERVAL_REG_4_5_6_7      0x03D4
#define EP_OUT_BINTERVAL_REG_8_9_10_11    0x03D8
#define EP_OUT_BINTERVAL_REG_12_13_14_15  0x03DC

#define MVCP_TX_TSI_NUM         0x3EC
#define MVCP_START_STATE_DELAY  0x3F0

#define MVCP_LOWPOWER           0x3F4
	#define MVCP_LOWPOWER_U2_EN     (1 << 3)
	#define MVCP_LOWPOWER_U1_EN     (1 << 2)
	#define MVCP_LOWPOWER_U2_REJ    (1 << 1)
	#define MVCP_LOWPOWER_U1_REJ    (1 << 0)

#define MVCP_SETUP_DP_LOW   0x3F8
#define MVCP_SETUP_DP_HIGH  0x3FC

#define MVCP_SETUP_CONTROL  0x400
	#define MVCP_SETUP_CONTROL_FETCHED  (1 << 0)

#define MVCP_DMA_GLOBAL_CONFIG  0x7D0
	#define MVCP_DMA_GLOBAL_CONFIG_INTCLR       (1 << 3)
	#define MVCP_DMA_GLOBAL_CONFIG_RESETDONE    (1 << 2)
	#define MVCP_DMA_GLOBAL_CONFIG_RUN          (1 << 1)
	#define MVCP_DMA_GLOBAL_CONFIG_RESET        (1 << 0)

#define MVCP_BULK_STREAMING_ENABLE      0x7D4
#define MVCP_EP_OUT_REC_STREAM_ID_BASE  0x7D8
#define MVCP_EP_IN_REC_STREAM_ID_BASE   0x814
static inline int streamid(int n, int i)
{
	if (n)
		return MVCP_EP_IN_REC_STREAM_ID_BASE + ((n - 1) << 2);
	else
		return MVCP_EP_OUT_REC_STREAM_ID_BASE + ((n - 1) << 2);
}

#define MVCP_DMA_COMPLETE_SUCCESS       0x850
#define MVCP_DMA_COMPLETE_ERROR         0x854
#define MVCP_DMA_BD_FETCH_ERROR         0x858
#define MVCP_DMA_BD_FETCH_ERROR_EN      0x85C
#define MVCP_DMA_DATA_ERROR             0x860
#define MVCP_DMA_DATA_ERROR_EN          0x864
#define MVCP_DMA_ERROR_HANDLING         0x868
#define MVCP_EP_OUT_RX_DMA_CONFIG_BASE  0x86C

#define MVCP_EP_IN_TX_DMA_CONFIG_BASE   0x8AC
	#define MVCP_EPDMA_START            (1 << 6)

static inline int ep_dma_config(int num, int dir)
{
	if (dir)
		return MVCP_EP_IN_TX_DMA_CONFIG_BASE + (num << 2);
	else
		return MVCP_EP_OUT_RX_DMA_CONFIG_BASE + (num << 2);
}

#define MVCP_EP_OUT_RX_DMA_START_BASE   0x8EC
#define MVCP_EP_IN_TX_DMA_START_BASE    0x92C

static inline int ep_dma_addr(int num, int dir)
{
	if (dir)
		return MVCP_EP_IN_TX_DMA_START_BASE + (num << 2);
	else
		return MVCP_EP_OUT_RX_DMA_START_BASE + (num << 2);
}

#define MVCP_DMA_SUSPEND        0x9EC
#define MVCP_DMA_SUSPEND_DONE   0x9F0
#define MVCP_DMA_HALT           0x9F4
#define MVCP_DMA_HALT_DONE      0x9F8

#define MVCP_SS_SYS_INT         0xA0C
	#define MVCP_SS_AXI_DATA_ERR    (1 << 29)
	#define MVCP_SS_AXI_BDF_ERR     (1 << 28)
	#define MVCP_SS_DONEQ_FULL_ERR  (1 << 27)
	#define MVCP_SS_SYS_INT_DMA     (1 << 25)

#define MVCP_SS_SYS_INTEN       0xA10
	#define MVCP_SS_SYS_INTEN_DMA   (1 << 25)

#define MVCP_DMA_STATE(n)       (0xA24 + ((n-1) << 2))
	#define MVCP_DMA_STATE_DBG_CACHE(x) ((x & 0x3) << 16)

#define MVCP_SEGMENT_COUNTER(n) (0xA38 + ((n-1) << 2))

#define MVCP_EP_IN_DONEQ_START_BASE     0xA78
#define MVCP_EP_IN_DONEQ_END_BASE       0xAB8
#define MVCP_EP_OUT_DONEQ_START_BASE    0xAF8
#define MVCP_EP_OUT_DONEQ_END_BASE      0xB38
#define MVCP_EP_IN_DONEQ_WRITE_BASE     0xB78
#define MVCP_EP_IN_DONEQ_READ_BASE      0xBB8
#define MVCP_EP_OUT_DONEQ_WRITE_BASE    0xBF8
#define MVCP_EP_OUT_DONEQ_READ_BASE     0xC38
#define MVCP_DONEQ_FULL_STATUS          0xC78

static inline int ep_doneq_start(int num, int dir)
{
	if (dir)
		return MVCP_EP_IN_DONEQ_START_BASE + (num << 2);
	else
		return MVCP_EP_OUT_DONEQ_START_BASE + (num << 2);
}

static inline int ep_doneq_end(int num, int dir)
{
	if (dir)
		return MVCP_EP_IN_DONEQ_END_BASE + (num << 2);
	else
		return MVCP_EP_OUT_DONEQ_END_BASE + (num << 2);
}

static inline int ep_doneq_write(int num, int dir)
{
	if (dir)
		return MVCP_EP_IN_DONEQ_WRITE_BASE + (num << 2);
	else
		return MVCP_EP_OUT_DONEQ_WRITE_BASE + (num << 2);
}

static inline int ep_doneq_read(int num, int dir)
{
	if (dir)
		return MVCP_EP_IN_DONEQ_READ_BASE + (num << 2);
	else
		return MVCP_EP_OUT_DONEQ_READ_BASE + (num << 2);
}

#define MVCP_BD_MEM_IN      0x123C
#define MVCP_PL_DEBUG(n)    (0x1264 + ((n-1) << 2))
#define MVCP_DMA_DBG_IN(n)  (0x1274 + ((n-1) << 2))
#define MVCP_DMA_DBG_OUT(n) (0x127C + ((n-1) << 2))

#define MVCP_DMA_ENABLE     0x1284

/* IP_VERSION >= 0X218 */
#define SS_IN_DMA_CONTROL_REG(x)  (0x1300 + 4 * (x))
#define SS_OUT_DMA_CONTROL_REG(x) (0x1340 + 4 * (x))
#define DMA_START     (1 << 0)
#define DMA_HALT      (1 << 1)
#define DMA_SUSPEND   (1 << 2)
#define DONEQ_CONFIG  (1 << 3)
#define ABORT_REQ     (1 << 4)
#define ABORT_DONE    (1 << 5)

#define SS_IN_EP_INT_STATUS_REG(x)  (0x1380 + 4 * (x))
#define SS_OUT_EP_INT_STATUS_REG(x) (0x13C0 + 4 * (x))
#define SS_IN_EP_INT_ENABLE_REG(x)  (0x1400 + 4 * (x))
#define SS_OUT_EP_INT_ENABLE_REG(x) (0x1440 + 4 * (x))

#define COMPLETION_SUCCESS   (1 << 0)
#define COMPLETION_WITH_ERR  (1 << 1)
#define BD_FETCH_ERROR       (1 << 2)
#define DMA_DATA_ERROR       (1 << 3)
#define DONEQ_FULL           (1 << 4)
#define DMA_SUSPEND_DONE     (1 << 5)
#define DMA_HALT_DONE        (1 << 6)
#define PRIME_REC            (1 << 16)
#define HIMD_REC             (1 << 17)
#define STREAM_REJ           (1 << 18)
#define HOST_FLOW_CTRL       (1 << 19)

#define SS_EP_TOP_INT_STATUS_REG 0x1480
#define SS_EP_TOP_INT_ENABLE_REG 0x1484
#define SS_AXI_INT_STATUS_REG 0x1488
#define SS_AXI_INT_ENABLE_REG 0x148C

#define EP_IN_BINTERVAL_REG(x)  (0x1490 + 4 * ((x)-1))
#define EP_OUT_BINTERVAL_REG(x) (0x14CC + 4 * ((x)-1))
/* END IP_VERSION >= 0X218 */

struct mvc2_ep;

struct mvc2_req {
	struct usb_request  req;
	int bd_total;
	struct bd   *bd;
	struct list_head    queue;
};

enum mvc2_dev_state {
	MVCP_DEFAULT_STATE,
	MVCP_ADDRESS_STATE,
	MVCP_CONFIGURED_STATE,
};

struct mvc2_register {
	unsigned int lfps_signal;
	unsigned int counter_pulse;
	unsigned int ref_int;
	unsigned int ref_inten;
	unsigned int global_control;
};

struct mvc2 {
	struct usb_gadget   gadget;
	struct usb_gadget_driver    *driver;
	struct device   *dev;
	struct clk  *clk;
	struct usb_phy *phy;
	struct phy *comphy;
	int irq;
	void    __iomem *base;
	void    __iomem *win_base;
	void    __iomem *phy_base;
	#define MVCP_STATUS_USB2        (1 << 10)
	#define MVCP_STATUS_CONNECTED   (1 << 9)
	#define MVCP_STATUS_TEST_MASK   (0x7 << 5)
	#define MVCP_STATUS_TEST(x)     (((x) & 0x7) << 5)
	#define MVCP_STATUS_U3          (1 << 4)
	#define MVCP_STATUS_U2          (1 << 3)
	#define MVCP_STATUS_U1          (1 << 2)
	#define MVCP_STATUS_U0          (1 << 1)
	#define MVCP_STATUS_POWER_MASK  (0xf << 1)
	#define MVCP_STATUS_SELF_POWERED    (1 << 0)
	unsigned int    status;
	enum mvc2_dev_state dev_state;
	spinlock_t  lock;

	struct mvc2_req ep0_req;
	int ep0_dir;
	void    *setup_buf;

	struct dma_pool *bd_pool;
	struct mvc2_ep  *eps;

	unsigned int    epnum;
	unsigned int    dma_status;

	struct work_struct  *work;
	struct pm_qos_request   qos_idle;
	s32                     lpm_qos;

	unsigned int    isoch_delay;
	unsigned int    u1sel;
	unsigned int    u1pel;
	unsigned int    u2sel;
	unsigned int    u2pel;
	struct mvc2_register *reg;
	unsigned int	mvc2_version;
	int vbus_pin;
	int prev_vbus;
	struct work_struct	vbus_work;
	struct workqueue_struct *qwork;
	/* Flags for HW reset. false: no need reset; true: need reset */
	bool phy_hw_reset;
};

extern void mvc2_usb2_connect(void);
extern void mvc2_usb2_disconnect(void);
extern int eps_init(struct mvc2 *cp);
extern void reset_seqencenum(struct mvc2_ep *ep, int num, int in);
extern void mvc2_config_mac(struct mvc2 *cp);
extern void mvc2_hw_reset(struct mvc2 *cp);
extern int mvc2_std_request(struct mvc2 *cp, struct usb_ctrlrequest *r,
		bool *delegate);
extern int mvc2_gadget_init(struct mvc2 *cp);
extern void mvc2_usb2_operation(struct mvc2 *cp, int op);
extern void mvc2_connect(struct mvc2 *cp, int is_on);
extern unsigned int u1u2_enabled(void);

#define BD_DMA_BOUNDARY 4096
#define BD_ADDR_ALIGN   4
/*
 * Although one BD could transfer 64k-1 bytes data,
 * for calculation efficiency, we short it for 32k
 */
#define BD_SEGMENT_SHIFT    (15)
#define BD_MAX_SIZE         (1 << BD_SEGMENT_SHIFT)
struct bd {
	#define BD_BUF_RDY          (1 << 31)
	#define BD_INT_EN           (1 << 30)
	#define BD_NXT_RDY          (1 << 29)
	#define BD_NXT_PTR_JUMP     (1 << 28)
	#define BD_FLUSH_BIT        (1 << 27)
	#define BD_ABORT_BIT        (1 << 26)
	#define BD_ZLP              (1 << 25)
	#define BD_CHAIN_BIT        (1 << 24)
	#define BD_ENCODED_STREAM_ID(x) ((x & 0xff) << 16)
	#define BD_BUF_SZ(x)        (x & 0xffff)
	unsigned int    cmd;
	unsigned int    buf;

	/* This field should be next bd's physical addr */
	unsigned int    phys_next;
#define BD_STREAM_ID(x)     ((x & 0xffff) << 16)
#define BD_STREAM_LEN(x)    (x & 0xffff)
	unsigned int    stream;
};

/*
 * Since each BD would transfer BD_MAX_SIZE, so for each endpoint, it allow to
 * hold (MAX_QUEUE_SLOT-1)*BD_MAX_SIZE in pending status to be transferred
 */
#define MAX_QUEUE_SLOT  256
struct doneq {
	unsigned int    addr;
	#define DONE_LEN(x)     ((x >> 16) & 0xffff)
	#define DONE_AXI_ERROR  (1 << 4)
	#define DONE_SHORT_PKT  (1 << 3)
	#define DONE_FLUSH      (1 << 2)
	#define DONE_ABORT      (1 << 1)
	#define DONE_CYCLE  (1 << 0)
	unsigned int    status;
};

#define MAXNAME 14
/*
 * For one ep, it would pending for several req,
 * while hardware would handle only one req for one time.
 * Other req is pending over queue list.
 *
 * For the transferring req, we would separate it into several bd to info
 * hw to transfer. And dma engine would auto feature those chained bd, and
 * send them out. When each bd finish, its BD_BUF_RDY flag would be cleared.
 * If BD_INT_EN flag is set, interrupt would be raised correspondingly.
 * ep --
 *      \--req->req->
 *          \     \--bd->bd->null
 *           \--bd->bd->null
 */
struct mvc2_ep {
	struct usb_ep   ep;
	struct mvc2 *cp;

	#define MV_CP_EP_TRANSERING     (1 << 8)
	#define MV_CP_EP_STALL          (1 << 7)
	#define MV_CP_EP_BULK_STREAM    (1 << 6)
	#define MV_CP_EP_WEDGE          (1 << 5)
	#define MV_CP_EP_DIRIN          (1 << 4)
	#define MV_CP_EP_NUM_MASK       (0xf)
	#define MV_CP_EP_NUM(x)         (x & MV_CP_EP_NUM_MASK)
	unsigned int    state;

	/*
	 * Actually in current hw solution,
	 * TransferQ and DoneQ size should be equal
	 */
	/*
	 * TransferQ:
	 *        doneq_cur    bd_cur
	 *        |            |
	 *        v            v
	 * |-------0============------|
	 *         ^
	 *         |
	 *         not ready bd
	 *
	 * In above diagram, "-" shows current available bd could be allocated,
	 * while "=" shows bd cannot be touched by the sw.
	 * When we need to do enqueue operation, we need to allocate bd
	 * from "-" pool.
	 *
	 * Note: we need ensure at least one bd in the ring as not ready
	 */
	struct bd   *bd_ring;
	dma_addr_t  bd_ring_phys;
	unsigned int    bd_cur;
	unsigned int    bd_sz;

	/* DoneQ */
	struct doneq    *doneq_start;
	dma_addr_t  doneq_start_phys;
	unsigned int    doneq_cur;

	char    name[MAXNAME];
	struct list_head    queue;
	struct list_head    wait, tmp;
	unsigned int dir;
	unsigned    stopped:1,
				wedge:1,
				ep_type:2,
				ep_num:8;
	unsigned int left_bds;
	/* Lock to keep queue is safe operated */
	spinlock_t  lock;
};

#define EPBIT(epnum, dir)    ({unsigned int tmp;        \
		tmp = (dir) ? (0x10000 << epnum) : (1 << epnum); tmp; })

#define MV_CP_READ(reg)        ({unsigned int val;        \
			val = ioread32(cp->base + reg); val; })
#define MV_CP_WRITE(val, reg)    ({iowrite32(val, cp->base + reg); })

struct mvc2;

static inline unsigned int ip_ver(struct mvc2 *cp)
{
	return cp->mvc2_version;
}

static inline unsigned int lfps_signal(struct mvc2 *cp, unsigned int n)
{
	return cp->reg->lfps_signal + ((n - 1) << 2);
}

/*
 * struct mvc2_glue - glue structure to combine 2.0/3.0 udc together
 * @u20: 2.0 driver udc
 * @u30: 3.0 driver udc
 * @usb2_connect: whether usb2.0 is in connection
 * @connect_num: how many usb3 has been tried
 */
struct mvc2_glue {
	struct usb_udc  *u20;
	struct usb_udc  *u30;

	int usb2_connect;
	unsigned int status;
};

extern struct mvc2_glue glue;
extern bool usb3_disconnect;

int mvc2_checkvbus(struct mvc2 *cp);
void mvc2_handle_setup(struct mvc2 *cp);
int mv_udc_register_status_notify(struct notifier_block *nb);

#endif
