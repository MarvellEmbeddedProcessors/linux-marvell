// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (C) 2017 Marvell
 *
 * Antoine Tenart <antoine.tenart@free-electrons.com>
 */

#include <linux/arm-smccc.h>
#include <linux/io.h>
#include <linux/iopoll.h>
#include <linux/mfd/syscon.h>
#include <linux/module.h>
#include <linux/of_device.h>
#include <linux/phy/phy.h>
#include <linux/platform_device.h>

struct mvebu_comhy_conf {
	enum phy_mode mode;
	unsigned lane;
	unsigned port;
};

#define MVEBU_COMPHY_CONF(_lane, _port, _mode)	\
	{					\
		.lane = _lane,			\
		.port = _port,			\
		.mode = _mode,			\
	}

/* FW related definitions */

#define MV_SIP_COMPHY_POWER_ON	0x82000001
#define MV_SIP_COMPHY_POWER_OFF	0x82000002
#define MV_SIP_COMPHY_PLL_LOCK	0x82000003

#define COMPHY_FW_MODE_FORMAT(mode)		((mode) << 12)
#define COMPHY_FW_NET_FORMAT(mode, idx, speeds)	\
		(COMPHY_FW_MODE_FORMAT(mode) | ((idx) << 8) | ((speeds) << 2))

#define COMPHY_FW_PCIE_FORMAT(pcie_width, mode, idx, speeds)	\
		(((pcie_width) << 18) | COMPHY_FW_NET_FORMAT(mode, idx, speeds))

#define COMPHY_SATA_MODE	0x1
#define COMPHY_SGMII_MODE	0x2	/* SGMII 1G */
#define COMPHY_HS_SGMII_MODE	0x3	/* SGMII 2.5G */
#define COMPHY_USB3H_MODE	0x4
#define COMPHY_USB3D_MODE	0x5
#define COMPHY_PCIE_MODE	0x6
#define COMPHY_RXAUI_MODE	0x7
#define COMPHY_XFI_MODE		0x8
#define COMPHY_SFI_MODE		0x9
#define COMPHY_USB3_MODE	0xa

/* COMPHY speed macro */
#define COMPHY_SPEED_1_25G		0 /* SGMII 1G */
#define COMPHY_SPEED_2_5G		1
#define COMPHY_SPEED_3_125G		2 /* SGMII 2.5G */
#define COMPHY_SPEED_5G			3
#define COMPHY_SPEED_5_15625G		4 /* XFI 5G */
#define COMPHY_SPEED_6G			5
#define COMPHY_SPEED_10_3125G		6 /* XFI 10G */
#define COMPHY_SPEED_MAX		0x3F

#define COMPHY_FW_NOT_SUPPORTED		(-1)

typedef unsigned long (comphy_fn)(unsigned long, phys_addr_t,
				  unsigned long, unsigned long);

static unsigned long cp110_comphy_smc(unsigned long function_id,
				      phys_addr_t comphy_phys_addr,
				      unsigned long lane, unsigned long mode)
{
	struct arm_smccc_res res;

	arm_smccc_smc(function_id, comphy_phys_addr,
		      lane, mode, 0, 0, 0, 0, &res);
	return res.a0;
}

static const struct mvebu_comhy_conf mvebu_comphy_cp110_modes[] = {
	/* lane 0 */
	MVEBU_COMPHY_CONF(0, 1, PHY_MODE_SGMII),
	MVEBU_COMPHY_CONF(0, 1, PHY_MODE_2500SGMII),
	MVEBU_COMPHY_CONF(0, 0, PHY_MODE_PCIE),
	MVEBU_COMPHY_CONF(0, 1, PHY_MODE_SATA),
	/* lane 1 */
	MVEBU_COMPHY_CONF(1, 2, PHY_MODE_SGMII),
	MVEBU_COMPHY_CONF(1, 2, PHY_MODE_2500SGMII),
	MVEBU_COMPHY_CONF(1, 0, PHY_MODE_PCIE),
	MVEBU_COMPHY_CONF(1, 0, PHY_MODE_SATA),
	MVEBU_COMPHY_CONF(1, 0, PHY_MODE_USB_HOST),
	/* lane 2 */
	MVEBU_COMPHY_CONF(2, 0, PHY_MODE_SGMII),
	MVEBU_COMPHY_CONF(2, 0, PHY_MODE_2500SGMII),
	MVEBU_COMPHY_CONF(2, 0, PHY_MODE_10GKR),
	MVEBU_COMPHY_CONF(2, 0, PHY_MODE_PCIE),
	MVEBU_COMPHY_CONF(2, 0, PHY_MODE_USB_HOST),
	MVEBU_COMPHY_CONF(2, 0, PHY_MODE_SATA),
	/* lane 3 */
	MVEBU_COMPHY_CONF(3, 1, PHY_MODE_SGMII),
	MVEBU_COMPHY_CONF(3, 1, PHY_MODE_2500SGMII),
	MVEBU_COMPHY_CONF(3, 0, PHY_MODE_PCIE),
	MVEBU_COMPHY_CONF(3, 1, PHY_MODE_SATA),
	MVEBU_COMPHY_CONF(3, 1, PHY_MODE_USB_HOST),
	/* lane 4 */
	MVEBU_COMPHY_CONF(4, 0, PHY_MODE_SGMII),
	MVEBU_COMPHY_CONF(4, 0, PHY_MODE_2500SGMII),
	MVEBU_COMPHY_CONF(4, 0, PHY_MODE_10GKR),
	MVEBU_COMPHY_CONF(4, 1, PHY_MODE_SGMII),
	MVEBU_COMPHY_CONF(4, 1, PHY_MODE_PCIE),
	MVEBU_COMPHY_CONF(4, 1, PHY_MODE_USB_HOST),
	/* lane 5 */
	MVEBU_COMPHY_CONF(5, 2, PHY_MODE_SGMII),
	MVEBU_COMPHY_CONF(5, 2, PHY_MODE_2500SGMII),
	MVEBU_COMPHY_CONF(5, 2, PHY_MODE_PCIE),
	MVEBU_COMPHY_CONF(5, 1, PHY_MODE_SATA),
};

struct mvebu_comphy_data {
	comphy_fn *comphy_smc;
	const struct mvebu_comhy_conf *modes;
	size_t modes_size;
	u8 lanes;
	u8 ports;
};

struct mvebu_comphy_priv {
	phys_addr_t phys;
	struct device *dev;
	const struct mvebu_comphy_data *data;
};

struct mvebu_comphy_lane {
	struct mvebu_comphy_priv *priv;
	unsigned id;
	enum phy_mode mode;
	int port;
};

static const struct mvebu_comphy_data cp110_data = {
	.comphy_smc = cp110_comphy_smc,
	.modes = mvebu_comphy_cp110_modes,
	.modes_size = ARRAY_SIZE(mvebu_comphy_cp110_modes),
	.lanes = 6,
	.ports = 3,
};

static int mvebu_is_comphy_mode_valid(struct mvebu_comphy_lane *lane,
				      enum phy_mode mode)
{
	const struct mvebu_comphy_data *data = lane->priv->data;
	const struct mvebu_comhy_conf *modes = data->modes;
	int i;

	for (i = 0; i < data->modes_size; i++) {
		if (modes[i].lane == lane->id &&
		    modes[i].port == lane->port &&
		    modes[i].mode == mode)
			break;
	}

	if (i == data->modes_size)
		return -EINVAL;

	return 0;
}

static int mvebu_comphy_power_on(struct phy *phy)
{
	struct mvebu_comphy_lane *lane = phy_get_drvdata(phy);
	struct mvebu_comphy_priv *priv = lane->priv;
	const struct mvebu_comphy_data *data = priv->data;
	int ret;

	switch (lane->mode) {
	case PHY_MODE_SGMII:
		ret = data->comphy_smc(MV_SIP_COMPHY_POWER_ON, priv->phys,
				 lane->id,
				 COMPHY_FW_NET_FORMAT(COMPHY_SGMII_MODE,
						      lane->port,
						      COMPHY_SPEED_1_25G));

		break;
	case PHY_MODE_2500SGMII:
		ret = data->comphy_smc(MV_SIP_COMPHY_POWER_ON, priv->phys,
				 lane->id,
				 COMPHY_FW_NET_FORMAT(COMPHY_HS_SGMII_MODE,
						      lane->port,
						      COMPHY_SPEED_3_125G));

		break;
	case PHY_MODE_10GKR:
		ret = data->comphy_smc(MV_SIP_COMPHY_POWER_ON, priv->phys,
				 lane->id,
				 COMPHY_FW_NET_FORMAT(COMPHY_XFI_MODE,
						      lane->port,
						      COMPHY_SPEED_10_3125G));
		break;
	case PHY_MODE_PCIE:
		ret = data->comphy_smc(MV_SIP_COMPHY_POWER_ON, priv->phys,
				 lane->id,
				 COMPHY_FW_PCIE_FORMAT(phy->attrs.bus_width,
						       COMPHY_PCIE_MODE,
						       lane->port,
						       COMPHY_SPEED_5G));
		break;
	case PHY_MODE_SATA:
		ret = data->comphy_smc(MV_SIP_COMPHY_POWER_ON, priv->phys,
				 lane->id,
				 COMPHY_FW_MODE_FORMAT(COMPHY_SATA_MODE));
		break;
	case PHY_MODE_USB_HOST:
		ret = data->comphy_smc(MV_SIP_COMPHY_POWER_ON, priv->phys,
				 lane->id,
				 COMPHY_FW_MODE_FORMAT(COMPHY_USB3H_MODE));
		break;
	default:
		return -ENOTSUPP;
	}

	dev_dbg(priv->dev, "%s: smc returned %d\n", __func__, ret);

	return ret;
}

static int mvebu_comphy_set_mode(struct phy *phy, enum phy_mode mode)
{
	struct mvebu_comphy_lane *lane = phy_get_drvdata(phy);

	if (mvebu_is_comphy_mode_valid(lane, mode) < 0)
		return -EINVAL;

	lane->mode = mode;
	return 0;
}

static int mvebu_comphy_power_off(struct phy *phy)
{
	struct mvebu_comphy_lane *lane = phy_get_drvdata(phy);
	struct mvebu_comphy_priv *priv = lane->priv;
	const struct mvebu_comphy_data *data = priv->data;

	return data->comphy_smc(MV_SIP_COMPHY_POWER_OFF, priv->phys,
				lane->id, 0);
}

static const struct phy_ops mvebu_comphy_ops = {
	.power_on	= mvebu_comphy_power_on,
	.power_off	= mvebu_comphy_power_off,
	.set_mode	= mvebu_comphy_set_mode,
	.owner		= THIS_MODULE,
};

static const struct phy_ops mvebu_comphy_ops_deprecated = {
	.owner		= THIS_MODULE,
};

static struct phy *mvebu_comphy_xlate(struct device *dev,
				      struct of_phandle_args *args)
{
	struct mvebu_comphy_lane *lane;
	struct mvebu_comphy_priv *priv;
	struct phy *phy;

	priv = dev_get_drvdata(dev);

	if (WARN_ON(args->args[0] >= priv->data->ports))
		return ERR_PTR(-EINVAL);

	phy = of_phy_simple_xlate(dev, args);
	if (IS_ERR(phy))
		return phy;

	lane = phy_get_drvdata(phy);
	if (lane->port >= 0)
		return ERR_PTR(-EBUSY);
	lane->port = args->args[0];

	return phy;
}

static int mvebu_comphy_probe(struct platform_device *pdev)
{
	struct mvebu_comphy_priv *priv;
	const struct mvebu_comphy_data *data;
	struct phy_provider *provider;
	struct device_node *child;
	struct resource *res;
	int i;

	priv = devm_kzalloc(&pdev->dev, sizeof(*priv), GFP_KERNEL);
	if (!priv)
		return -ENOMEM;

	data = of_device_get_match_data(&pdev->dev);
	priv->data = data;

	priv->dev = &pdev->dev;

	/*
	 * Request all resources declared in dts for this driver, even if they
	 * are not used explicit by this driver. This will prevent other Linux
	 * drivers from accessing comphy register range, and therefore prevent
	 * concurrent access with FW, which handles comphy initialization via RT
	 * services.
	 */
	for (i = 0; i < pdev->num_resources; i++) {
		res = platform_get_resource(pdev, IORESOURCE_MEM, i);

		if (i == 0)
			priv->phys = res->start;

		if (!devm_request_mem_region(&pdev->dev, res->start,
					     resource_size(res), res->name)) {
			dev_err(&pdev->dev, "resource %s busy\n", res->name);
			return -EBUSY;
		}
	}

	for_each_available_child_of_node(pdev->dev.of_node, child) {
		struct mvebu_comphy_lane *lane;
		struct phy *phy;
		int ret;
		u32 val;

		ret = of_property_read_u32(child, "reg", &val);
		if (ret < 0) {
			dev_err(&pdev->dev, "missing 'reg' property (%d)\n",
				ret);
			continue;
		}

		if (val >= data->lanes) {
			dev_err(&pdev->dev, "invalid 'reg' property\n");
			continue;
		}

		lane = devm_kzalloc(&pdev->dev, sizeof(*lane), GFP_KERNEL);
		if (!lane)
			return -ENOMEM;

		phy = devm_phy_create(&pdev->dev, child, &mvebu_comphy_ops);
		if (IS_ERR(phy))
			return PTR_ERR(phy);

		lane->priv = priv;
		lane->mode = PHY_MODE_INVALID;
		lane->id = val;
		lane->port = -1;
		phy_set_drvdata(phy, lane);

		/*
		 * To avoid relying on the bootloader/firmware configuration,
		 * power off all comphys.
		 */
		ret = mvebu_comphy_power_off(phy);
		if (ret == COMPHY_FW_NOT_SUPPORTED) {
			dev_warn(&pdev->dev, "RELYING ON BOTLOADER SETTINGS\n");
			dev_WARN(&pdev->dev, "firmware updated needed\n");

			/*
			 * If comphy power off fails it means that the
			 * deprecated firmware is used and we should rely on
			 * bootloader settings, therefore we are switching to
			 * empty ops.
			 */
			phy_destroy(phy);
			phy = devm_phy_create(&pdev->dev, child,
					      &mvebu_comphy_ops_deprecated);
			phy_set_drvdata(phy, lane);
		}
	}

	dev_set_drvdata(&pdev->dev, priv);
	provider = devm_of_phy_provider_register(&pdev->dev,
						 mvebu_comphy_xlate);
	return PTR_ERR_OR_ZERO(provider);
}

static const struct of_device_id mvebu_comphy_of_match_table[] = {
	{
		.compatible = "marvell,comphy-cp110",
		.data = &cp110_data,
	},
	{ },
};
MODULE_DEVICE_TABLE(of, mvebu_comphy_of_match_table);

static struct platform_driver mvebu_comphy_driver = {
	.probe	= mvebu_comphy_probe,
	.driver	= {
		.name = "mvebu-comphy",
		.of_match_table = mvebu_comphy_of_match_table,
	},
};
module_platform_driver(mvebu_comphy_driver);

MODULE_AUTHOR("Antoine Tenart <antoine.tenart@free-electrons.com>, Grzegorz Jaszczyk <jaz@semihalf.com>");
MODULE_DESCRIPTION("Common PHY driver for mvebu SoCs");
MODULE_LICENSE("GPL v2");
