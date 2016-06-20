/*
* ***************************************************************************
* Copyright (C) 2015 Marvell International Ltd.
* ***************************************************************************
* This program is free software: you can redistribute it and/or modify it
* under the terms of the GNU General Public License as published by the Free
* Software Foundation, either version 2 of the License, or any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program.  If not, see <http://www.gnu.org/licenses/>.
* ***************************************************************************
*/

#include <linux/interrupt.h>
#include <linux/kernel.h>
#include <linux/pci.h>
#include <linux/clk.h>
#include <linux/delay.h>
#include <linux/gpio.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/platform_device.h>
#include <linux/of_address.h>
#include <linux/of_gpio.h>
#include <linux/of_pci.h>
#include <linux/of_platform.h>

/* PCIe core controller registers */
#define CTRL_CORE_BASE_ADDR				0x18000
#define CTRL_CONFIG_REG					0x0
#define     CTRL_MODE_SHIFT				0x0
#define     CTRL_MODE_MASK				0x1
#define     PCIE_CORE_MODE_DIRECT			0x0
#define     PCIE_CORE_MODE_COMMAND			0x1

/* PCIe core registers */
#define PCIE_CORE_DEV_CTRL_STATS_REG			0xC8
#define PCIE_CORE_LINK_CTRL_STAT_REG			0xD0
#define     PCIE_CORE_LINK_TRAINING_SHIFT		5
#define     PCIE_CORE_LINK_SPEED_SHIFT			16
#define     PCIE_CORE_LINK_SPEED_MASK			0xF
#define     PCIE_CORE_LINK_WIDTH_SHIFT			20
#define     PCIE_CORE_LINK_WIDTH_MASK			0x3F
#define     PCIE_CORE_LINK_DLL_STATE_SHIFT		29
#define     PCIE_CORE_LINK_DLL_STATE_MASK		0x1
#define     PCIE_DLL_STATE_ACTIVE			1
#define     PCIE_DLL_STATE_INACTIVE			0
#define PCIE_CORE_ERR_CAP_CTRL_REG			0x118

/* PIO registers base address and register offsets */
#define PIO_BASE_ADDR					0x4000
#define PIO_CTRL					0x0
#define     PCIE_PIO_PCIE_TYPE_MASK			0xF
#define     PCIE_PIO_DIS_ADDR_WIN_SHIFT			24
#define PIO_STAT					0x4
#define PIO_ADDR_LS					0x8
#define PIO_ADDR_MS					0xc
#define PIO_WR_DATA					0x10
#define PIO_WR_DATA_STRB				0x14
#define PIO_RD_DATA					0x18
#define PIO_START					0x1c
#define PIO_ISR						0x20
#define PIO_ISRM					0x24

/* Aardvark Control registers */
#define CONTROL_BASE_ADDR				0x4800
#define PCIE_CORE_CTRL0_REG				0x0
#define     PCIE_GEN_SEL_MSK				0x3
#define     PCIE_GEN_SEL_SHIFT				0x0
#define     SPEED_GEN_1					0
#define     SPEED_GEN_2					1
#define     SPEED_GEN_3					2
#define     IS_RC_MSK					1
#define     IS_RC_SHIFT					2
#define     LANE_CNT_MSK				0x18
#define     LANE_CNT_SHIFT				0x3
#define     LANE_COUNT_1				(0 << LANE_CNT_SHIFT)
#define     LANE_COUNT_2				(1 << LANE_CNT_SHIFT)
#define     LANE_COUNT_4				(2 << LANE_CNT_SHIFT)
#define     LANE_COUNT_8				(3 << LANE_CNT_SHIFT)
#define     LINK_TRAINNING_EN				0x40
#define     CORE_RESET_MSK				0x80
#define     MGMT_RESET_MSK				0x100
#define     MGMT_STICKY_RESET_MSK			0x200
#define     APP_RESET_MSK				0x10000
#define     LEGACY_INTA					0x10000000
#define     LEGACY_INTB					0x20000000
#define     LEGACY_INTC					0x40000000
#define     LEGACY_INTD					0x80000000
#define PCIE_CORE_CTRL1_REG				0x4
#define     HOT_RESET_GEN				0x1
#define PCIE_CORE_CTRL2_REG				0x8
#define     STRICT_ORDER_ENABLE				0x20
#define     OB_WIN_ENABLE				0x40
#define     MSI_ENABLE					0x400
#define PCIE_FLUSH_CONTROL_REG				0x0C
#define     PCIE_FLUSH_CONTROL_CLEAR			(1 << 18)
#define PCIE_CORE_PHY_REF_CLK_REG			0x14
#define     PCIE_CORE_EN_TX				(1 << 1)
#define     PCIE_CORE_EN_RX				(1 << 2)
#define     PCIE_CORE_SEL_AMP_MASK			0x7
#define     PCIE_CORE_SEL_AMP_SHIFT			0x8
#define     PCIE_CORE_EN_PU				(1 << 12)
#define PCIE_CORE_MSG_LOG_0_REG				0x30 /*Inbound message log*/

/* PCIe window configuration */
#define OB_WIN_BASE_ADDR				0x4c00
#define OB_WIN_MATCH_LS					0x00
#define OB_WIN_MATCH_MS					0x04
#define OB_WIN_REMAP_LS					0x08
#define OB_WIN_REMAP_MS					0x0c
#define OB_WIN_MASK_LS					0x10
#define OB_WIN_MASK_MS					0x14
#define OB_WIN_ACTIONS					0x18
#define OB_WIN_BLOCK_SIZE				0x20
#define OB_Default_ACTIONS				0xfc
/* PCIe window types */
#define     OB_PCIE_MEM					0x0
#define     OB_PCIE_IO					0x4
#define     OB_PCIE_CONFIG0				0x8
#define     OB_PCIE_CONFIG1				0x9
#define     OB_PCIE_MSG					0xc
#define     OB_PCIE_MSG_VENDOR				0xd

/* BAR registers */
#define PCIE_BAR_BASE					0x5800
#define PCIE_BAR_0_REG					0x0
#define PCIE_BAR_1_REG					0x4
#define PCIE_BAR_2_REG					0x8
#define PCIE_BAR_3_REG					0xc
#define PCIE_BAR_4_REG					0x10
#define PCIE_BAR_5_REG					0x14

/* LMI registers base address and register offsets */
#define LMI_BASE_ADDR					0x6000
#define CFG_REG						0x0
#define     LTSSM_SHIFT					24
#define     LTSSM_MASK					0x3f
#define     LTSSM_L0					0x10
#define     RC_BAR_CONFIG				0x300

/* Transaction types */
#define PCIE_CONFIG_RD_TYPE0				0x8
#define PCIE_CONFIG_RD_TYPE1				0x9
#define PCIE_CONFIG_WR_TYPE0				0xa
#define PCIE_CONFIG_WR_TYPE1				0xb

/* PIO defines */
/* PCI_BDF shifts 8bit, so we need extra 4bit shift */
#define PCIE_BDF(dev)					(dev << 4)
#define PCIE_CONF_BUS(bus)				(((bus) & 0xff) << 20)
#define PCIE_CONF_DEV(dev)				(((dev) & 0x1f) << 15)
#define PCIE_CONF_FUNC(fun)				(((fun) & 0x7)	<< 12)
#define PCIE_CONF_REG(reg)				((reg) & 0xffc)
#define PCIE_CONF_ADDR(bus, devfn, where)	\
	(PCIE_CONF_BUS(bus) | PCIE_CONF_DEV(PCI_SLOT(devfn))	| \
	 PCIE_CONF_FUNC(PCI_FUNC(devfn)) | PCIE_CONF_REG(where))

/* 0x0 */
#define PCIE_CORE_CONFIG_REG_ADDR(offset)	(offset)
/* 0x18000 */
#define PCIE_CORE_REG_ADDR(offset)		\
						(CTRL_CORE_BASE_ADDR + offset)
/*
 * Get the PIO registers addresses of a PCIE device,
 * 0x4000 is the offset of PIO register block.
 */
#define PCIE_PIO_REG_ADDR(offset)	(PIO_BASE_ADDR + offset)
/*
 * Get the LMI register address of a PCIE device,
 * 0x6000 is the offset of LMI register block.
 */
#define PCIE_LMI_REG_ADDR(offset)	(LMI_BASE_ADDR + offset)
/* 0x4c00 OB */
#define PCIE_CORE_OB_REG_ADDR(offset, win)	\
		       (OB_WIN_BASE_ADDR + (win) * OB_WIN_BLOCK_SIZE + (offset))
/* 0x4800 Control */
#define PCIE_CORE_CTRL_REG_ADDR(offset)		(CONTROL_BASE_ADDR + offset)

/* Used in PIO read/write, by default 1ms for PIO opertion */
#define PIO_TIMEOUT_NUM				(1000)
/* 10ms */
#define LINKUP_TIMEOUT				(10)
/* Up to 1.2 seconds */
#define ADVK_PCIE_LINKUP_TIMEOUT		(200)

/*
 * This product ID is registered by Marvell, and used when the Marvell
 * SoC is not the root complex, but an endpoint on the PCIe bus. It is
 * therefore safe to re-use this PCI ID for our emulated PCI-to-PCI
 * bridge.
 */
#define MARVELL_EMULATED_PCI_PCI_BRIDGE_ID	0x7846
#define ARLP_MAX_PCIE_PORTS			0x1

static const char speed_str[4][8] = {"NA", "2.5GHz", "5GHz", "8GHz"};
static const char width_str[9][8] = {
			  "NA", "x1", "x2", "NA", "x4", "NA", "NA", "NA", "x8"};
static const char mode_str[2][16] = {"Endpoint", "Root Complex"};

/* PCI configuration space of a PCI-to-PCI bridge */
struct advk_sw_pci_bridge {
	u16 vendor;
	u16 device;
	u16 command;
	u16 class;
	u8 interface;
	u8 revision;
	u8 bist;
	u8 header_type;
	u8 latency_timer;
	u8 cache_line_size;
	u32 bar[2];
	u8 primary_bus;
	u8 secondary_bus;
	u8 subordinate_bus;
	u8 secondary_latency_timer;
	u8 iobase;
	u8 iolimit;
	u16 secondary_status;
	u16 membase;
	u16 memlimit;
	u16 iobaseupper;
	u16 iolimitupper;
	u8 cappointer;
	u8 reserved1;
	u16 reserved2;
	u32 romaddr;
	u8 intline;
	u8 intpin;
	u16 bridgectrl;
};

struct advk_pcie_port;

/* Structure representing all PCIe interfaces */
struct advk_pcie {
	struct platform_device *pdev;
	struct advk_pcie_port *ports;
	struct resource io;
	struct resource realio;
	struct resource mem;
	struct resource busn;
	int nports;
};

/* Structure representing one PCIe interface */
struct advk_pcie_port {
	char *name;
	void __iomem *base;
	u32 port;
	u32 lane;
	int devfn;
	unsigned int mem_target;
	unsigned int mem_attr;
	unsigned int io_target;
	unsigned int io_attr;
	struct clk *clk;
	int reset_gpio;
	int reset_active_low;
	char *reset_name;
	struct advk_sw_pci_bridge bridge;
	struct device_node *dn;
	struct advk_pcie *pcie;
	phys_addr_t memwin_base;
	size_t memwin_size;
	phys_addr_t iowin_base;
	size_t iowin_size;
	u32 saved_pcie_stat;
};

static inline void advk_writel(struct advk_pcie_port *port, u32 val, u64 reg)
{
	writel(val, port->base + reg);
}

static inline u32 advk_readl(struct advk_pcie_port *port, u64 reg)
{
	return readl(port->base + reg);
}

static inline bool advk_has_ioport(struct advk_pcie_port *port)
{
	return port->io_target != -1 && port->io_attr != -1;
}

static bool advk_pcie_link_up(struct advk_pcie_port *port)
{
	int timeout;
	u32 ltssm_state;
	u32 val;

	timeout = ADVK_PCIE_LINKUP_TIMEOUT;
	do {
		val = advk_readl(port, PCIE_LMI_REG_ADDR(CFG_REG));
		ltssm_state = (val >> LTSSM_SHIFT) & LTSSM_MASK;
		timeout--;
	/* Use ltssm < LTSSM_L0 instead of ltssm != LTSSM_L0 */
	} while (ltssm_state < LTSSM_L0 && timeout > 0);

	if (timeout > 0)
		return 1;
	else
		return 0;
}

static void advk_pcie_set_local_bus_nr(struct advk_pcie_port *port, int nr)
{
}

static void advk_pcie_set_local_dev_nr(struct advk_pcie_port *port, int nr)
{
}

/*
 * Set PCIe address window register which could be used for memory mapping.
 * These address window registers are within PCIe IP internally.
 * It should be called and set correctly if want to access external PCIe device
 * by accessing CPU memory space directly.
 */
static int advk_pcie_set_ob_win(struct advk_pcie_port *port,
			u32 win_num,
			u32 match_ms,
			u32 match_ls,
			u32 mask_ms,
			u32 mask_ls,
			u32 remap_ms,
			u32 remap_ls,
			u32 action)
{
	advk_writel(port, match_ls,
		    PCIE_CORE_OB_REG_ADDR(OB_WIN_MATCH_LS, win_num));
	advk_writel(port, match_ms,
		    PCIE_CORE_OB_REG_ADDR(OB_WIN_MATCH_MS, win_num));
	advk_writel(port, mask_ms,
		    PCIE_CORE_OB_REG_ADDR(OB_WIN_MASK_MS, win_num));
	advk_writel(port, mask_ls,
		    PCIE_CORE_OB_REG_ADDR(OB_WIN_MASK_LS, win_num));
	advk_writel(port, remap_ms,
		    PCIE_CORE_OB_REG_ADDR(OB_WIN_REMAP_MS, win_num));
	advk_writel(port, remap_ls,
		    PCIE_CORE_OB_REG_ADDR(OB_WIN_REMAP_LS, win_num));
	advk_writel(port, action,
		    PCIE_CORE_OB_REG_ADDR(OB_WIN_ACTIONS, win_num));
	advk_writel(port, match_ls | 0x1,
		    PCIE_CORE_OB_REG_ADDR(OB_WIN_MATCH_LS, win_num));

	return 0;
}

static void advk_pcie_setup_wins(struct advk_pcie_port *port)
{
	int i;

	for (i = 0; i < 8; i++)
		advk_pcie_set_ob_win(port, i, 0, 0, 0, 0, 0, 0, 0);
}

static void advk_pcie_setup_hw(struct advk_pcie_port *port)
{
	u32 config, state;

	/* Point PCIe unit MBUS decode windows to DRAM space. */
	advk_pcie_setup_wins(port);

	/* Set to Direct mode. */
	config = advk_readl(port, PCIE_CORE_REG_ADDR(CTRL_CONFIG_REG));
	config &= ~(CTRL_MODE_MASK << CTRL_MODE_SHIFT);
	config |= ((PCIE_CORE_MODE_DIRECT & CTRL_MODE_MASK) << CTRL_MODE_SHIFT);
	advk_writel(port, config, PCIE_CORE_REG_ADDR(CTRL_CONFIG_REG));

	/* Set PCI global control register to RC mode */
	config = advk_readl(port, PCIE_CORE_CTRL_REG_ADDR(PCIE_CORE_CTRL0_REG));
	config |=  (IS_RC_MSK << IS_RC_SHIFT);
	advk_writel(port, config, PCIE_CORE_CTRL_REG_ADDR(PCIE_CORE_CTRL0_REG));

	/*
	 * Set Advanced Error Capabilities and Control PF0 register
	 * ECRC_CHCK_RCV (RD0070118h [8]) = 1h
	 * ECRC_CHCK (RD0070118h [7]) = 1h
	 * ECRC_GEN_TX_EN (RD0070118h [6]) = 1h
	 * ECRC_CHK_TX (RD0070118h [5]) = 1h
	 */
	advk_writel(port, 0x01E0,
		    PCIE_CORE_CONFIG_REG_ADDR(PCIE_CORE_ERR_CAP_CTRL_REG));

	/*
	 * Set PCIe Device Control and Status 1 PF0 register
	 * MAX_RD_REQ_SIZE (RD00700C8h [14:12])/MAX_RD_REQ_SZ (RD00700C8h [14:12]) = 2h (default)
	 * Clear EN_NO_SNOOP (RD00700C8h [11])/EN_NO_SNOOP (RD00700C8h [11]) = 0h (default is 1h)
	 * MAX_PAYLOAD_SIZEW (RD00700C8h [7:5])/MAX_PAYLOAD (RD00700C8h [7:5]) = 7
	 * EN_RELAXED_ORDERING (RD00700C8h [4])/EN_RELAXED_ORDERING (RD00700C8h [4])= 0h (default is 1h)
	 */
	advk_writel(port, 0x20e0,
		    PCIE_CORE_CONFIG_REG_ADDR(PCIE_CORE_DEV_CTRL_STATS_REG));

	/*
	 * Program PCIe Control 2 (RD0074808h) to 0000001Fh
	 * to disable strict ordering by clearing
	 * STRICT_ORDERING_EN (RD0074808h [5]) = 0h (default is 1h).
	 */
	advk_writel(port, 0x001F, PCIE_CORE_CTRL_REG_ADDR(PCIE_CORE_CTRL2_REG));

	/* Set GEN2 */
	state = advk_readl(port, PCIE_CORE_CTRL_REG_ADDR(PCIE_CORE_CTRL0_REG));
	state &= ~PCIE_GEN_SEL_MSK;
	state |= SPEED_GEN_2;
	advk_writel(port, state, PCIE_CORE_CTRL_REG_ADDR(PCIE_CORE_CTRL0_REG));

	/* Set lane X1 */
	state = advk_readl(port, PCIE_CORE_CTRL_REG_ADDR(PCIE_CORE_CTRL0_REG));
	state &= ~LANE_CNT_MSK;
	state |= LANE_COUNT_1;
	advk_writel(port, state, PCIE_CORE_CTRL_REG_ADDR(PCIE_CORE_CTRL0_REG));

	/* Enable link training */
	state = advk_readl(port, PCIE_CORE_CTRL_REG_ADDR(PCIE_CORE_CTRL0_REG));
	state |= LINK_TRAINNING_EN;
	advk_writel(port, state, PCIE_CORE_CTRL_REG_ADDR(PCIE_CORE_CTRL0_REG));

	/* Disable strict ordering */
	state = advk_readl(port, PCIE_CORE_CTRL_REG_ADDR(PCIE_CORE_CTRL2_REG));
	state &= ~STRICT_ORDER_ENABLE;
#ifdef CONFIG_PCI_MSI
	state |= MSI_ENABLE;
#endif
	advk_writel(port, state, PCIE_CORE_CTRL_REG_ADDR(PCIE_CORE_CTRL2_REG));

	/* Enable the AXI address window mapping */
	config = advk_readl(port, PCIE_CORE_CTRL_REG_ADDR(PCIE_CORE_CTRL2_REG));
	config |= OB_WIN_ENABLE;
	advk_writel(port, config, PCIE_CORE_CTRL_REG_ADDR(PCIE_CORE_CTRL2_REG));

	/* Bypass the address window mapping for PIO */
	config = advk_readl(port, PCIE_PIO_REG_ADDR(PIO_CTRL));
	config |= (1 << PCIE_PIO_DIS_ADDR_WIN_SHIFT);
	advk_writel(port, config, PCIE_PIO_REG_ADDR(PIO_CTRL));

	/* Start link training */
	state = advk_readl(port,
		      PCIE_CORE_CONFIG_REG_ADDR(PCIE_CORE_LINK_CTRL_STAT_REG));
	state |= (1 << PCIE_CORE_LINK_TRAINING_SHIFT);
	advk_writel(port, state,
		    PCIE_CORE_CONFIG_REG_ADDR(PCIE_CORE_LINK_CTRL_STAT_REG));

	advk_pcie_link_up(port);

	/* Set PCIe Control 2 register
	 * bit[1:0] ASPM Control, set to 1 to enable L0S entry
	 */
	advk_writel(port, 0x00100001,
		    PCIE_CORE_CONFIG_REG_ADDR(PCIE_CORE_LINK_CTRL_STAT_REG));

	/* Enable BUS, IO, Memory space assess
	 * bit2: Memory IO Request
	 * bit1: Memory Access Enable
	 * bit0: IO Access Enable
	 */
	state = advk_readl(port, PCIE_CORE_CONFIG_REG_ADDR(4));
	state |= 0x7;
	advk_writel(port, state, PCIE_CORE_CONFIG_REG_ADDR(4));
}

/*
 * Check PIO status
 */
static int advk_pcie_check_pio_status(struct advk_pcie_port *port)
{
	unsigned int pio_status;
	unsigned char comp_status;
	char *strcomp_status;

	pio_status = advk_readl(port, PCIE_PIO_REG_ADDR(PIO_STAT));
	comp_status = (pio_status >> 7) & 0x7;

	switch (comp_status) {
	case 0:
		break;
	case 1:
		strcomp_status = "UR";
		break;
	case 2:
		strcomp_status = "CRS";
		break;
	case 4:
		strcomp_status = "CA";
		break;
	default:
		strcomp_status = "Unknown";
		break;
	}
	if (comp_status) {
		if (pio_status & (0x1 << 10))
			pr_err("Non-posted PIO Response Status: %s, %#x @ %#x\n",
			       strcomp_status, pio_status,
			       advk_readl(port, PCIE_PIO_REG_ADDR(PIO_ADDR_LS)));
		else
			pr_err("Posted PIO Response Status: %s, %#x @ %#x\n",
			       strcomp_status, pio_status,
			       advk_readl(port, PCIE_PIO_REG_ADDR(PIO_ADDR_LS)));
	}

	return 0;
}

static int advk_pcie_hw_rd_conf(struct advk_pcie_port *port,
				struct pci_bus *bus, u32 devfn,
				int where, int size, u32 *val)
{
	u32 reg_val, is_done;
	void __iomem *baseaddr;
	int i;
	int ret = PCIBIOS_SUCCESSFUL;

	baseaddr = port->base;

	/* Start PIO */
	advk_writel(port, 0, PCIE_PIO_REG_ADDR(PIO_START));
	advk_writel(port, 1, PCIE_PIO_REG_ADDR(PIO_ISR));

	/* Program the control register */
	reg_val = advk_readl(port, PCIE_PIO_REG_ADDR(PIO_CTRL));
	reg_val &= ~(PCIE_PIO_PCIE_TYPE_MASK);
	if (bus->number ==  1)
		reg_val |= PCIE_CONFIG_RD_TYPE0;
	else
		reg_val |= PCIE_CONFIG_RD_TYPE1;
	advk_writel(port, reg_val, PCIE_PIO_REG_ADDR(PIO_CTRL));

	/* Program the address registers */
	reg_val = PCIE_BDF(devfn)|PCIE_CONF_REG(where);
	advk_writel(port, reg_val, PCIE_PIO_REG_ADDR(PIO_ADDR_LS));
	advk_writel(port, 0, PCIE_PIO_REG_ADDR(PIO_ADDR_MS));

	/* Program the data strobe */
	advk_writel(port, 0xf, PCIE_PIO_REG_ADDR(PIO_WR_DATA_STRB));
	/* Start the transfer */
	advk_writel(port, 1, PCIE_PIO_REG_ADDR(PIO_START));

	/* Polling */
	for (i = 0; i < PIO_TIMEOUT_NUM; i++) {
		reg_val = advk_readl(port, PCIE_PIO_REG_ADDR(PIO_START));
		is_done = advk_readl(port, PCIE_PIO_REG_ADDR(PIO_ISR));
		if ((!reg_val) && is_done)
			break;
	}
	if (i == PIO_TIMEOUT_NUM) {
		pr_err("%s config read failed!\nBus: %d, Dev: %d, Func: %d, Regs: 0x%X, Size: %d\n",
		       __func__, bus->number, PCI_SLOT(devfn), PCI_FUNC(devfn),
		       where, size);
		return PCIBIOS_SET_FAILED;
	}

	/* Get the read result */
	*val = advk_readl(port, PCIE_PIO_REG_ADDR(PIO_RD_DATA));
	if (size == 1)
		*val = (*val >> (8 * (where & 3))) & 0xff;
	else if (size == 2)
		*val = (*val >> (8 * (where & 3))) & 0xffff;

	return ret;
}

static int advk_pcie_hw_wr_conf(struct advk_pcie_port *port,
				 struct pci_bus *bus,
				 u32 devfn, int where, int size, u32 val)
{
	u32 reg_val, is_done;
	void __iomem *baseaddr;
	u32 data_strobe = 0x0;
	int i;
	int ret = PCIBIOS_SUCCESSFUL;

	baseaddr = port->base;

	/* Start PIO */
	advk_writel(port, 0, PCIE_PIO_REG_ADDR(PIO_START));
	advk_writel(port, 1, PCIE_PIO_REG_ADDR(PIO_ISR));

	/* Program the control register */
	reg_val = advk_readl(port, PCIE_PIO_REG_ADDR(PIO_CTRL));
	reg_val &= ~(PCIE_PIO_PCIE_TYPE_MASK);
	if (bus->number == 1)
		reg_val |= PCIE_CONFIG_WR_TYPE0;
	else
		reg_val |= PCIE_CONFIG_WR_TYPE1;
	advk_writel(port, reg_val, PCIE_PIO_REG_ADDR(PIO_CTRL));

	/* Program the address registers */
	reg_val = PCIE_CONF_ADDR(bus->number, devfn, where);
	advk_writel(port, reg_val, PCIE_PIO_REG_ADDR(PIO_ADDR_LS));
	advk_writel(port, 0, PCIE_PIO_REG_ADDR(PIO_ADDR_MS));

	/* Program the write strobe */
	switch (size) {
	case SZ_1:
		switch (where % 4) {
		case 0:
			data_strobe = 0x1;
			reg_val = val;
			break;
		case 1:
			data_strobe = 0x2;
			reg_val = val << 8;
			break;
		case 2:
			data_strobe = 0x4;
			reg_val = val << 16;
			break;
		case 3:
			data_strobe = 0x8;
			reg_val = val << 24;
			break;
		}
		break;
	case SZ_2:
		switch (where % 4) {
		case 0:
			data_strobe = 0x3;
			reg_val = val;
			break;
		case 2:
			data_strobe = 0xc;
			reg_val = val << 16;
			break;
		default:
			return PCIBIOS_SET_FAILED;
		}
		break;
	case SZ_4:
		if (where % 4)
			return PCIBIOS_SET_FAILED;

		data_strobe = 0xf;
		reg_val = val;
		break;
	default:
		return PCIBIOS_SET_FAILED;
	}

	/* Program the data register */
	advk_writel(port, reg_val, PCIE_PIO_REG_ADDR(PIO_WR_DATA));
	/* Pragom the data strobe */
	advk_writel(port, data_strobe, PCIE_PIO_REG_ADDR(PIO_WR_DATA_STRB));
	/* Start the transfer */
	advk_writel(port, 1, PCIE_PIO_REG_ADDR(PIO_START));

	for (i = 0; i < PIO_TIMEOUT_NUM; i++) {
		reg_val = advk_readl(port, PCIE_PIO_REG_ADDR(PIO_START));
		is_done = advk_readl(port, PCIE_PIO_REG_ADDR(PIO_ISR));
		if ((!reg_val) && is_done)
			break;
	}
	if (i == PIO_TIMEOUT_NUM) {
		pr_err("%s config write failed!\nBus: %d, Dev: %d, Func: %d, Regs: 0x%X, Val: 0x%X, Size: %d\n",
		       __func__, bus->number, PCI_SLOT(devfn), PCI_FUNC(devfn),
		       where, val, size);
		return PCIBIOS_SET_FAILED;
	}

	advk_pcie_check_pio_status(port);

	return ret;
}

static void advk_pcie_handle_iobase_change(struct advk_pcie_port *port)
{
	phys_addr_t iobase;

	/* Are the new iobase/iolimit values invalid? */
	if (port->bridge.iolimit < port->bridge.iobase ||
	    port->bridge.iolimitupper < port->bridge.iobaseupper ||
	    !(port->bridge.command & PCI_COMMAND_IO)) {

		/* If a window was configured, remove it */
		if (port->iowin_base) {
			port->iowin_base = 0;
			port->iowin_size = 0;
		}
		return;
	}

	if (!advk_has_ioport(port)) {
		dev_WARN(&port->pcie->pdev->dev,
			 "Attempt to set IO when IO is disabled\n");
		return;
	}

	/*
	 * We read the PCI-to-PCI bridge emulated registers, and
	 * calculate the base address and size of the address decoding
	 * window to setup, according to the PCI-to-PCI bridge
	 * specifications. iobase is the bus address, port->iowin_base
	 * is the CPU address.
	 */
	iobase = ((port->bridge.iobase & 0xF0) << 8) |
		(port->bridge.iobaseupper << 16);
	port->iowin_base = port->pcie->io.start + iobase;
	port->iowin_size = ((0xFFF | ((port->bridge.iolimit & 0xF0) << 8) |
			    (port->bridge.iolimitupper << 16)) -
			    iobase) + 1;

	/* Register outbound window for configuration and set r/w config operations */
	advk_pcie_set_ob_win(port,			/* reg base */
		1,					/* window block*/
		port->iowin_base >> 32,			/* match ms */
		port->iowin_base & 0xFFFFFFFF,		/* match ls */
		0,					/* mask ms */
		0xF8000000,				/* mask ls */
		0,					/* remap ms */
		port->iowin_base & 0xFFFFFFFF,		/* remap ls */
		OB_PCIE_IO);
}

static void advk_pcie_handle_membase_change(struct advk_pcie_port *port)
{
	/* Are the new membase/memlimit values invalid? */
	if (port->bridge.memlimit < port->bridge.membase ||
	    !(port->bridge.command & PCI_COMMAND_MEMORY)) {

		/* If a window was configured, remove it */
		if (port->memwin_base) {
			port->memwin_base = 0;
			port->memwin_size = 0;
		}

		return;
	}

	/*
	 * We read the PCI-to-PCI bridge emulated registers, and
	 * calculate the base address and size of the address decoding
	 * window to setup, according to the PCI-to-PCI bridge
	 * specifications.
	 */
	port->memwin_base  = ((port->bridge.membase & 0xFFF0) << 16);
	port->memwin_size  =
		(((port->bridge.memlimit & 0xFFF0) << 16) | 0xFFFFF) -
		port->memwin_base + 1;

	/*
	 * Register outbound window for configuration and
	 * set r/w config operations.
	 */
	advk_pcie_set_ob_win(port,			/* reg base */
		0,					/* window block*/
		port->memwin_base >> 32,		/* match ms */
		port->memwin_base & 0xFFFFFFFF,		/* match ls */
		0x0,					/* mask ms */
		0xF8000000,				/* mask ls */
		0,					/* remap ms */
		port->memwin_base & 0xFFFFFFFF,		/* remap ls */
		(2 << 20)|OB_PCIE_MEM);
}

/*
 * Initialize the configuration space of the PCI-to-PCI bridge
 * associated with the given PCIe interface.
 */
static void advk_sw_pci_bridge_init(struct advk_pcie_port *port)
{
	struct advk_sw_pci_bridge *bridge = &port->bridge;

	memset(bridge, 0, sizeof(struct advk_sw_pci_bridge));
	/*
	 * Vendor ID 1B4Bh
	 * Device ID 0100h
	 * Revision ID 0h
	 * Class code 1h
	 * Sub-class code 4h
	 */
	bridge->class = PCI_CLASS_BRIDGE_PCI;
	bridge->vendor = PCI_VENDOR_ID_MARVELL_EXT;
	bridge->device = 0x100;
	bridge->revision = 0x0;
	bridge->header_type = PCI_HEADER_TYPE_BRIDGE;
	bridge->cache_line_size = 0x40;

	/* We support 32 bits I/O addressing */
	bridge->iobase = PCI_IO_RANGE_TYPE_32;
	bridge->iolimit = PCI_IO_RANGE_TYPE_32;
}

/*
 * Read the configuration space of the PCI-to-PCI bridge associated to
 * the given PCIe interface.
 */
static int advk_sw_pci_bridge_read(struct advk_pcie_port *port,
				  unsigned int where, int size, u32 *value)
{
	struct advk_sw_pci_bridge *bridge = &port->bridge;

	switch (where & ~3) {
	case PCI_VENDOR_ID:
		*value = bridge->device << 16 | bridge->vendor;
		break;

	case PCI_COMMAND:
		*value = bridge->command;
		break;

	case PCI_CLASS_REVISION:
		*value = bridge->class << 16 | bridge->interface << 8 |
			 bridge->revision;
		break;

	case PCI_CACHE_LINE_SIZE:
		*value = bridge->bist << 24 | bridge->header_type << 16 |
			 bridge->latency_timer << 8 | bridge->cache_line_size;
		break;

	case PCI_BASE_ADDRESS_0 ... PCI_BASE_ADDRESS_1:
		*value = bridge->bar[((where & ~3) - PCI_BASE_ADDRESS_0) / 4];
		break;

	case PCI_PRIMARY_BUS:
		*value = (bridge->secondary_latency_timer << 24 |
			  bridge->subordinate_bus         << 16 |
			  bridge->secondary_bus           <<  8 |
			  bridge->primary_bus);
		break;

	case PCI_IO_BASE:
		if (!advk_has_ioport(port))
			*value = bridge->secondary_status << 16;
		else
			*value = (bridge->secondary_status << 16 |
				  bridge->iolimit          <<  8 |
				  bridge->iobase);
		break;

	case PCI_MEMORY_BASE:
		*value = (bridge->memlimit << 16 | bridge->membase);
		break;

	case PCI_PREF_MEMORY_BASE:
		*value = 0;
		break;

	case PCI_IO_BASE_UPPER16:
		*value = (bridge->iolimitupper << 16 | bridge->iobaseupper);
		break;

	case PCI_ROM_ADDRESS1:
		*value = 0;
		break;

	case PCI_INTERRUPT_LINE:
		/* LINE PIN MIN_GNT MAX_LAT */
		*value = 0;
		break;

	default:
		*value = 0xffffffff;
		return PCIBIOS_BAD_REGISTER_NUMBER;
	}

	if (size == 2)
		*value = (*value >> (8 * (where & 3))) & 0xffff;
	else if (size == 1)
		*value = (*value >> (8 * (where & 3))) & 0xff;

	return PCIBIOS_SUCCESSFUL;
}

/* Write to the PCI-to-PCI bridge configuration space */
static int advk_sw_pci_bridge_write(struct advk_pcie_port *port,
				    unsigned int where, int size, u32 value)
{
	struct advk_sw_pci_bridge *bridge = &port->bridge;
	u32 mask, reg;
	int err;

	if (size == 4)
		mask = 0x0;
	else if (size == 2)
		mask = ~(0xffff << ((where & 3) * 8));
	else if (size == 1)
		mask = ~(0xff << ((where & 3) * 8));
	else
		return PCIBIOS_BAD_REGISTER_NUMBER;

	err = advk_sw_pci_bridge_read(port, where & ~3, 4, &reg);
	if (err)
		return err;

	value = (reg & mask) | value << ((where & 3) * 8);

	switch (where & ~3) {
	case PCI_COMMAND:
	{
		u32 old = bridge->command;

		if (!advk_has_ioport(port))
			value &= ~PCI_COMMAND_IO;

		bridge->command = value & 0xffff;
		if ((old ^ bridge->command) & PCI_COMMAND_IO)
			advk_pcie_handle_iobase_change(port);
		if ((old ^ bridge->command) & PCI_COMMAND_MEMORY)
			advk_pcie_handle_membase_change(port);
		break;
	}

	case PCI_BASE_ADDRESS_0 ... PCI_BASE_ADDRESS_1:
		bridge->bar[((where & ~3) - PCI_BASE_ADDRESS_0) / 4] = value;
		break;

	case PCI_IO_BASE:
		/*
		 * We also keep bit 1 set, it is a read-only bit that
		 * indicates we support 32 bits addressing for the
		 * I/O
		 */
		bridge->iobase = (value & 0xff) | PCI_IO_RANGE_TYPE_32;
		bridge->iolimit = ((value >> 8) & 0xff) | PCI_IO_RANGE_TYPE_32;
		advk_pcie_handle_iobase_change(port);
		break;

	case PCI_MEMORY_BASE:
		bridge->membase = value & 0xffff;
		bridge->memlimit = value >> 16;
		advk_pcie_handle_membase_change(port);
		break;

	case PCI_IO_BASE_UPPER16:
		bridge->iobaseupper = value & 0xffff;
		bridge->iolimitupper = value >> 16;
		advk_pcie_handle_iobase_change(port);
		break;

	case PCI_PRIMARY_BUS:
		bridge->primary_bus             = value & 0xff;
		bridge->secondary_bus           = (value >> 8) & 0xff;
		bridge->subordinate_bus         = (value >> 16) & 0xff;
		bridge->secondary_latency_timer = (value >> 24) & 0xff;
		advk_pcie_set_local_bus_nr(port, bridge->secondary_bus);
		break;

	default:
		break;
	}

	return PCIBIOS_SUCCESSFUL;
}

static struct advk_pcie_port *advk_pcie_find_port(struct advk_pcie *pcie,
						  struct pci_bus *bus,
						  int devfn)
{
	int i;

	for (i = 0; i < pcie->nports; i++) {
		struct advk_pcie_port *port = &pcie->ports[i];

		if (bus->number == 0 && port->devfn == devfn)
			return port;
		if (bus->number != 0 &&
		    bus->number >= port->bridge.secondary_bus &&
		    bus->number <= port->bridge.subordinate_bus)
			return port;
	}

	return NULL;
}


/* PCI configuration space write function */
static int advk_pcie_wr_conf(struct pci_bus *bus, u32 devfn,
			     int where, int size, u32 val)
{
	struct advk_pcie *pcie = bus->sysdata;
	struct advk_pcie_port *port;
	int ret;

	port = advk_pcie_find_port(pcie, bus, devfn);
	if (!port)
		return PCIBIOS_DEVICE_NOT_FOUND;

	/* Access the emulated PCI-to-PCI bridge */
	if (bus->number == 0)
		return advk_sw_pci_bridge_write(port, where, size, val);
	/*
	 * On the secondary bus, we don't want to expose any other
	 * device than the device physically connected in the PCIe
	 * slot, visible in slot 0. In slot 1, there's a special
	 * Marvell device that only makes sense when the Armada is
	 * used as a PCIe endpoint.
	 */
	if (bus->number == port->bridge.secondary_bus &&
	    PCI_SLOT(devfn) != 0)
		return PCIBIOS_DEVICE_NOT_FOUND;

	if (!advk_pcie_link_up(port))
		return PCIBIOS_DEVICE_NOT_FOUND;

	/* Access the real PCIe interface */
	ret = advk_pcie_hw_wr_conf(port, bus, devfn,
				    where, size, val);

	return ret;
}

/* PCI configuration space read function */
static int advk_pcie_rd_conf(struct pci_bus *bus, u32 devfn, int where,
			     int size, u32 *val)
{
	struct advk_pcie *pcie = bus->sysdata;
	struct advk_pcie_port *port;
	int ret;

	port = advk_pcie_find_port(pcie, bus, devfn);
	if (!port) {
		*val = 0xffffffff;
		return PCIBIOS_DEVICE_NOT_FOUND;
	}

	/* Access the emulated PCI-to-PCI bridge */
	if (bus->number == 0)
		return advk_sw_pci_bridge_read(port, where, size, val);

	/*
	 * On the secondary bus, we don't want to expose any other
	 * device than the device physically connected in the PCIe
	 * slot, visible in slot 0. In slot 1, there's a special
	 * Marvell device that only makes sense when the Armada is
	 * used as a PCIe endpoint.
	 */
	if (bus->number == port->bridge.secondary_bus &&
	    PCI_SLOT(devfn) != 0) {
		*val = 0xffffffff;
		return PCIBIOS_DEVICE_NOT_FOUND;
	}

	if (!advk_pcie_link_up(port)) {
		*val = 0xffffffff;
		return PCIBIOS_DEVICE_NOT_FOUND;
	}

	/* Access the real PCIe interface */
	ret = advk_pcie_hw_rd_conf(port, bus, devfn,
				   where, size, val);

	return ret;
}

static struct pci_ops advk_pcie_ops = {
	.read = advk_pcie_rd_conf,
	.write = advk_pcie_wr_conf,
};

/*
 * Looks up the list of register addresses encoded into the reg =
 * <...> property for one that matches the given port/lane. Once
 * found, maps it.
 */
static void __iomem *advk_pcie_map_registers(struct platform_device *pdev,
					     struct device_node *np,
					     struct advk_pcie_port *port)
{
	struct resource regs;
	int ret = 0;

	ret = of_address_to_resource(np, 0, &regs);
	if (ret)
		return ERR_PTR(ret);

	return devm_ioremap_resource(&pdev->dev, &regs);
}

#ifdef CONFIG_PCI_MSI
static struct msi_controller *advk_pcie_msi_init(struct device_node *np)
{
	struct device_node *msi_node;
	struct msi_controller *msi;

	msi_node = of_parse_phandle(np, "msi-parent", 0);
	if (!msi_node)
		return NULL;

	msi = of_pci_find_msi_chip_by_node(msi_node);

	return msi;
}
#endif

static int advk_pcie_probe(struct platform_device *pdev)
{
	struct advk_pcie *pcie;
	struct device_node *np = pdev->dev.of_node;
	struct device_node *child;
	struct pci_bus *bus;
	resource_size_t iobase = 0;
	LIST_HEAD(res);
	int i, ret;

	pcie = devm_kzalloc(&pdev->dev, sizeof(struct advk_pcie),
			    GFP_KERNEL);
	if (!pcie)
		return -ENOMEM;

	pcie->pdev = pdev;
	platform_set_drvdata(pdev, pcie);

	i = 0;
	for_each_child_of_node(pdev->dev.of_node, child) {
		if (!of_device_is_available(child))
			continue;
		i++;
	}

	pcie->ports = devm_kcalloc(&pdev->dev, i,
				   sizeof(struct advk_pcie_port),
				   GFP_KERNEL);
	if (!pcie->ports)
		return -ENOMEM;

	i = 0;
	for_each_available_child_of_node(pdev->dev.of_node, child) {
		struct advk_pcie_port *port = &pcie->ports[i];
		enum of_gpio_flags flags;

		port->pcie = pcie;

		if (of_property_read_u32(child, "marvell,pcie-port",
					 &port->port)) {
			dev_warn(&pdev->dev,
				 "ignoring PCIe DT node, missing pcie-port property\n");
			continue;
		}

		if (of_property_read_u32(child, "marvell,pcie-lane",
					 &port->lane))
			port->lane = 0;

		port->name = kasprintf(GFP_KERNEL, "pcie%d.%d",
				       port->port, port->lane);

		port->devfn = of_pci_get_devfn(child);
		if (port->devfn < 0)
			continue;

		port->reset_gpio = of_get_named_gpio_flags(child,
						   "reset-gpios", 0, &flags);
		if (gpio_is_valid(port->reset_gpio)) {
			u32 reset_udelay = 20000;

			port->reset_active_low = flags & OF_GPIO_ACTIVE_LOW;
			port->reset_name = kasprintf(GFP_KERNEL,
				     "pcie%d.%d-reset", port->port, port->lane);
			of_property_read_u32(child, "reset-delay-us",
					     &reset_udelay);

			ret = devm_gpio_request_one(&pdev->dev,
						    port->reset_gpio,
						    GPIOF_DIR_OUT,
						    port->reset_name);
			if (ret) {
				if (ret == -EPROBE_DEFER)
					return ret;
				continue;
			}

			gpio_set_value(port->reset_gpio,
				       (port->reset_active_low) ? 1 : 0);
			msleep(reset_udelay/1000);
		}

		port->clk = of_clk_get_by_name(child, NULL);
		if (IS_ERR(port->clk)) {
			dev_err(&pdev->dev, "PCIe%d.%d: cannot get clock\n",
				port->port, port->lane);
			continue;
		}

		ret = clk_prepare_enable(port->clk);
		if (ret) {
			dev_err(&pdev->dev, "PCIe%d.%d: cannot enable clock\n",
				port->port, port->lane);
			continue;
		}

		port->base = advk_pcie_map_registers(pdev, child, port);
		if (IS_ERR(port->base)) {
			dev_err(&pdev->dev, "PCIe%d.%d: cannot map registers\n",
				port->port, port->lane);
			port->base = NULL;
			clk_disable_unprepare(port->clk);
			continue;
		}

		advk_pcie_set_local_dev_nr(port, 1);

		advk_pcie_setup_hw(port);

		port->dn = child;

		advk_sw_pci_bridge_init(port);

		i++;
	}
	pcie->nports = i;

	ret = of_pci_get_host_bridge_resources(np, 0, 0xff, &res, &iobase);
	if (ret)
		return ret;

	bus = pci_create_root_bus(&pdev->dev, 0, &advk_pcie_ops, pcie, &res);
	if (!bus)
		return -ENOMEM;

#ifdef CONFIG_PCI_MSI
	bus->msi = advk_pcie_msi_init(np);
#endif

	pci_scan_child_bus(bus);
	pci_assign_unassigned_bus_resources(bus);
	pci_bus_add_devices(bus);

	platform_set_drvdata(pdev, pcie);

	return 0;
}

static const struct of_device_id advk_pcie_of_match_table[] = {
	{ .compatible = "marvell,armada-3700-pcie", },
	{},
};
MODULE_DEVICE_TABLE(of, advk_pcie_of_match_table);

static struct platform_driver advk_pcie_driver = {
	.driver = {
		.name = "advk-pcie",
		.of_match_table = advk_pcie_of_match_table,
		/* Driver unloading/unbinding currently not supported */
		.suppress_bind_attrs = true,
	},
	.probe = advk_pcie_probe,
};
module_platform_driver(advk_pcie_driver);

MODULE_AUTHOR("Hezi Shahmoon <hezi.shahmoon@marvell.com>");
MODULE_DESCRIPTION("Aardvark PCIe driver");
MODULE_LICENSE("GPL v2");
