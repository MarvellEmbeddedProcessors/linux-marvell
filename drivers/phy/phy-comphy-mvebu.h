#ifndef _COMPHY_MVEBU_H
#define _COMPHY_MVEBU_H

#define MVEBU_COMPHY_MAX_CNT	6
#define MVEBU_COMPHY_FUNC_MAX	11

#define to_mvebu_comphy_priv(lane) \
	container_of((lane), struct mvebu_comphy_priv, lanes[(lane)->index])

struct mvebu_comphy_priv {
	struct device *dev;
	void __iomem *comphy_regs;
	void __iomem *comphy_pipe_regs;
	spinlock_t lock;
	const struct mvebu_comphy_soc_info *soc_info;
	struct mvebu_comphy {
		struct phy *phy;
		int mode;
		int index;
	} lanes[MVEBU_COMPHY_MAX_CNT];
};

struct mvebu_comphy_soc_info {
	int num_of_lanes;
	int functions[MVEBU_COMPHY_MAX_CNT][MVEBU_COMPHY_FUNC_MAX];
	struct phy_ops *comphy_ops;
};

static inline void __maybe_unused reg_set(void __iomem *addr, u32 data, u32 mask)
{
	u32 reg_data;

	reg_data = readl(addr);
	reg_data &= ~mask;
	reg_data |= data;
	writel(reg_data, addr);
}

static inline u32 __maybe_unused polling_with_timeout(void __iomem *addr,
						      u32 val,
						      u32 mask,
						      unsigned long usec_timout)
{
	u32 data;

	do {
		udelay(1);
		data = readl(addr) & mask;
	} while (data != val  && --usec_timout > 0);

	if (usec_timout == 0)
		return data;

	return 0;
}

/* Function declaration */
int mvebu_comphy_set_mode(struct phy *phy, enum phy_mode mode);
enum phy_mode mvebu_comphy_get_mode(struct phy *phy);

#endif /* _COMPHY_MVEBU_H */

