#ifndef _CPRI_COMMON_H_
#define _CPRI_COMMON_H_

#define BPHY_CPRI_PKT_BUF_SIZE           1664    /* wqe 128 bytes + 1536 bytes */
#define RX_BUF_POISON_PILL 0xdeadbeef

/* cpri dl cbuf cfg */
struct cpri_dl_cbuf_cfg {
        int                             num_entries;
        u64                             cbuf_iova_addr;
        void __iomem                    *cbuf_virt_addr;
        /* sw */
        u64                             sw_wr_ptr;
        /* dl lock */
        spinlock_t                      lock;
};

/* cpri ul cbuf cfg */
struct cpri_ul_cbuf_cfg {
        int                             num_entries;
        u64                             cbuf_iova_addr;
        void __iomem                    *cbuf_virt_addr;
        /* sw */
        int                             sw_rd_ptr;
        /* ul lock */
        spinlock_t                      lock;
};

struct cpri_common_cfg {
        struct cpri_dl_cbuf_cfg		dl_cfg;
        struct cpri_ul_cbuf_cfg         ul_cfg;
        u8				refcnt;
};

static inline void cpri_rx_buf_poison(int index,
				      struct cpri_ul_cbuf_cfg *ul_cfg)
{
        u8 *buf = NULL;

        buf = (u8 __force *)ul_cfg->cbuf_virt_addr +
                (BPHY_CPRI_PKT_BUF_SIZE * index);
        *((u32*)(buf)) = RX_BUF_POISON_PILL;
        wmb();
}

static inline bool cpri_rx_buf_is_poisoned(int index,
					   struct cpri_ul_cbuf_cfg *ul_cfg)
{
        u8 *buf = NULL;
        bool ret = false;

        buf = (u8 __force *)ul_cfg->cbuf_virt_addr +
                (BPHY_CPRI_PKT_BUF_SIZE * index);
        if (*((u32*)(buf)) == RX_BUF_POISON_PILL) {
                 ret = true;
        }

        return ret;
}

#endif
