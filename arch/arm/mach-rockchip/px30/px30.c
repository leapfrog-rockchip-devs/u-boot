/*
 * Copyright (c) 2017 Rockchip Electronics Co., Ltd
 *
 * SPDX-License-Identifier:     GPL-2.0+
 */
#include <common.h>
#include <clk.h>
#include <dm.h>
#include <fdt_support.h>
#include <asm/io.h>
#include <asm/arch/cru_px30.h>
#include <asm/arch/grf_px30.h>
#include <asm/arch/hardware.h>
#include <asm/arch/uart.h>
#include <asm/arch/clock.h>
#include <asm/arch/cpu.h>
#include <asm/arch/cru_px30.h>
#include <dt-bindings/clock/px30-cru.h>

#define PMU_PWRDN_CON			0xff000018
#define GRF_CPU_CON1			0xff140504

#define VIDEO_PHY_BASE			0xff2e0000
#define FW_DDR_CON_REG			0xff534040
#define SERVICE_CORE_ADDR		0xff508000
#define QOS_PRIORITY			0x08

#define QOS_PRIORITY_LEVEL(h, l)	((((h) & 3) << 8) | ((l) & 3))

#ifdef CONFIG_ARM64
#include <asm/armv8/mmu.h>

static struct mm_region px30_mem_map[] = {
	{
		.virt = 0x0UL,
		.phys = 0x0UL,
		.size = 0xff000000UL,
		.attrs = PTE_BLOCK_MEMTYPE(MT_NORMAL) |
			 PTE_BLOCK_INNER_SHARE
	}, {
		.virt = 0xff000000UL,
		.phys = 0xff000000UL,
		.size = 0x01000000UL,
		.attrs = PTE_BLOCK_MEMTYPE(MT_DEVICE_NGNRNE) |
			 PTE_BLOCK_NON_SHARE |
			 PTE_BLOCK_PXN | PTE_BLOCK_UXN
	}, {
		/* List terminator */
		0,
	}
};

struct mm_region *mem_map = px30_mem_map;
#endif

#define PMU_PWRDN_CON			0xff000018
#define GRF_BASE			0xff140000
#define CRU_BASE			0xff2b0000
#define VIDEO_PHY_BASE			0xff2e0000
#define SERVICE_CORE_ADDR		0xff508000
#define DDR_FW_BASE			0xff534000

#define FW_DDR_CON			0x40

#define QOS_PRIORITY			0x08

#define QOS_PRIORITY_LEVEL(h, l)	((((h) & 3) << 8) | ((l) & 3))

#define GRF_GPIO1A_DS2			0x0090
#define GRF_GPIO1B_DS2			0x0094
#define GRF_GPIO1A_E			0x00F0
#define GRF_GPIO1B_E			0x00F4

/* GRF_GPIO1CL_IOMUX */
enum {
	GPIO1C1_SHIFT		= 4,
	GPIO1C1_MASK		= 0xf << GPIO1C1_SHIFT,
	GPIO1C1_GPIO		= 0,
	GPIO1C1_UART1_TX,

	GPIO1C0_SHIFT		= 0,
	GPIO1C0_MASK		= 0xf << GPIO1C0_SHIFT,
	GPIO1C0_GPIO		= 0,
	GPIO1C0_UART1_RX,
};

/* GRF_GPIO1DL_IOMUX */
enum {
	GPIO1D3_SHIFT		= 12,
	GPIO1D3_MASK		= 0xf << GPIO1D3_SHIFT,
	GPIO1D3_GPIO		= 0,
	GPIO1D3_SDMMC_D1,
	GPIO1D3_UART2_RXM0,

	GPIO1D2_SHIFT		= 8,
	GPIO1D2_MASK		= 0xf << GPIO1D2_SHIFT,
	GPIO1D2_GPIO		= 0,
	GPIO1D2_SDMMC_D0,
	GPIO1D2_UART2_TXM0,
};

/* GRF_GPIO1DH_IOMUX */
enum {
	GPIO1D7_SHIFT		= 12,
	GPIO1D7_MASK		= 0xf << GPIO1D7_SHIFT,
	GPIO1D7_GPIO		= 0,
	GPIO1D7_SDMMC_CMD,

	GPIO1D6_SHIFT		= 8,
	GPIO1D6_MASK		= 0xf << GPIO1D6_SHIFT,
	GPIO1D6_GPIO		= 0,
	GPIO1D6_SDMMC_CLK,

	GPIO1D5_SHIFT		= 4,
	GPIO1D5_MASK		= 0xf << GPIO1D5_SHIFT,
	GPIO1D5_GPIO		= 0,
	GPIO1D5_SDMMC_D3,

	GPIO1D4_SHIFT		= 0,
	GPIO1D4_MASK		= 0xf << GPIO1D4_SHIFT,
	GPIO1D4_GPIO		= 0,
	GPIO1D4_SDMMC_D2,
};

/* GRF_GPIO2BH_IOMUX */
enum {
	GPIO2B6_SHIFT		= 8,
	GPIO2B6_MASK		= 0xf << GPIO2B6_SHIFT,
	GPIO2B6_GPIO		= 0,
	GPIO2B6_CIF_D1M0,
	GPIO2B6_UART2_RXM1,

	GPIO2B4_SHIFT		= 0,
	GPIO2B4_MASK		= 0xf << GPIO2B4_SHIFT,
	GPIO2B4_GPIO		= 0,
	GPIO2B4_CIF_D0M0,
	GPIO2B4_UART2_TXM1,
};

/* GRF_GPIO3AL_IOMUX */
enum {
	GPIO3A2_SHIFT		= 8,
	GPIO3A2_MASK		= 0xf << GPIO3A2_SHIFT,
	GPIO3A2_GPIO		= 0,
	GPIO3A2_UART5_TX	= 4,

	GPIO3A1_SHIFT		= 4,
	GPIO3A1_MASK		= 0xf << GPIO3A1_SHIFT,
	GPIO3A1_GPIO		= 0,
	GPIO3A1_UART5_RX	= 4,
};

enum {
	IOVSEL6_CTRL_SHIFT	= 0,
	IOVSEL6_CTRL_MASK	= BIT(0),
	VCCIO6_SEL_BY_GPIO	= 0,
	VCCIO6_SEL_BY_IOVSEL6,

	IOVSEL6_SHIFT		= 1,
	IOVSEL6_MASK		= BIT(1),
	VCCIO6_3V3		= 0,
	VCCIO6_1V8,
};

/*
 * The voltage of VCCIO6(which is the voltage domain of emmc/flash/sfc
 * interface) can indicated by GPIO0_B6 or io_vsel6. The SOC defaults
 * use GPIO0_B6 to indicate power supply voltage for VCCIO6 by hardware,
 * then we can switch to io_vsel6 after system power on, and release GPIO0_B6
 * for other usage.
 */

#define GPIO0_B6		14
#define GPIO0_BASE		0xff040000
#define GPIO_SWPORTA_DDR	0x4
#define GPIO_EXT_PORTA		0x50

static int grf_vccio6_vsel_init(void)
{
	static struct px30_grf * const grf = (void *)GRF_BASE;
	u32 val;

	val = readl(GPIO0_BASE + GPIO_SWPORTA_DDR);
	val &= ~BIT(GPIO0_B6);
	writel(val, GPIO0_BASE + GPIO_SWPORTA_DDR);

	if (readl(GPIO0_BASE + GPIO_EXT_PORTA) & BIT(GPIO0_B6))
		val = VCCIO6_SEL_BY_IOVSEL6 << IOVSEL6_CTRL_SHIFT |
		      VCCIO6_1V8 << IOVSEL6_SHIFT;
	else
		val = VCCIO6_SEL_BY_IOVSEL6 << IOVSEL6_CTRL_SHIFT |
		      VCCIO6_3V3 << IOVSEL6_SHIFT;
	rk_clrsetreg(&grf->io_vsel, IOVSEL6_CTRL_MASK | IOVSEL6_MASK, val);

	return 0;
}

int arch_cpu_init(void)
{
#ifdef CONFIG_SPL_BUILD
	/* We do some SoC one time setting here. */
	/* Disable the ddr secure region setting to make it non-secure */
	writel(0x0, FW_DDR_CON_REG);

	if (soc_is_px30s()) {
		/* set the emmc data(GPIO1A0-A7) drive strength to 14.2ma */
		writel(0xFFFF0000, GRF_BASE + GRF_GPIO1A_DS2);
		writel(0xFFFFFFFF, GRF_BASE + GRF_GPIO1A_E);
		/* set the emmc clock(GPIO1B1) drive strength to 23.7ma */
		/* set the emmc cmd(GPIO1B2) drive strength to 14.2ma */
		writel(0x00060002, GRF_BASE + GRF_GPIO1B_DS2);
		writel(0x003C0038, GRF_BASE + GRF_GPIO1B_E);
	}
#endif
	/* Enable PD_VO (default disable at reset) */
	rk_clrreg(PMU_PWRDN_CON, 1 << 13);

#ifdef CONFIG_SPL_BUILD
	/* Set cpu qos priority */
	writel(QOS_PRIORITY_LEVEL(1, 1), SERVICE_CORE_ADDR + QOS_PRIORITY);

#if !defined(CONFIG_DEBUG_UART_BOARD_INIT) || \
    (CONFIG_DEBUG_UART_BASE != 0xff160000) || \
    (CONFIG_DEBUG_UART_CHANNEL != 0)
	static struct px30_grf * const grf = (void *)GRF_BASE;
	/* fix sdmmc pinmux if not using uart2-channel0 as debug uart */
	rk_clrsetreg(&grf->gpio1dl_iomux,
		     GPIO1D3_MASK | GPIO1D2_MASK,
		     GPIO1D3_SDMMC_D1 << GPIO1D3_SHIFT |
		     GPIO1D2_SDMMC_D0 << GPIO1D2_SHIFT);
	rk_clrsetreg(&grf->gpio1dh_iomux,
		     GPIO1D7_MASK | GPIO1D6_MASK | GPIO1D5_MASK | GPIO1D4_MASK,
		     GPIO1D7_SDMMC_CMD << GPIO1D7_SHIFT |
		     GPIO1D6_SDMMC_CLK << GPIO1D6_SHIFT |
		     GPIO1D5_SDMMC_D3 << GPIO1D5_SHIFT |
		     GPIO1D4_SDMMC_D2 << GPIO1D4_SHIFT);
#endif

#endif

	/* Enable PD_VO (default disable at reset) */
	rk_clrreg(PMU_PWRDN_CON, 1 << 13);

	/* Disable video phy bandgap by default */
	writel(0x82, VIDEO_PHY_BASE + 0x0000);
	writel(0x05, VIDEO_PHY_BASE + 0x03ac);

	/* Clear the force_jtag */
	rk_clrreg(GRF_CPU_CON1, 1 << 7);

	grf_vccio6_vsel_init();

	return 0;
}

#define GRF_BASE		0xff140000
#define UART2_BASE		0xff160000
#define CRU_BASE		0xff2b0000
void board_debug_uart_init(void)
{
	static struct px30_grf * const grf = (void *)GRF_BASE;
	static struct px30_cru * const cru = (void *)CRU_BASE;

#if defined(CONFIG_DEBUG_UART_BASE) && (CONFIG_DEBUG_UART_BASE == 0xff158000)
	/* uart_sel_clk default select 24MHz */
	rk_clrsetreg(&cru->clksel_con[34],
		     UART1_PLL_SEL_MASK | UART1_DIV_CON_MASK,
		     UART1_PLL_SEL_24M << UART1_PLL_SEL_SHIFT | 0);
	rk_clrsetreg(&cru->clksel_con[35],
		     UART1_CLK_SEL_MASK,
		     UART1_CLK_SEL_UART1 << UART1_CLK_SEL_SHIFT);

	rk_clrsetreg(&grf->gpio1cl_iomux,
		     GPIO1C1_MASK | GPIO1C0_MASK,
		     GPIO1C1_UART1_TX << GPIO1C1_SHIFT |
		     GPIO1C0_UART1_RX << GPIO1C0_SHIFT);
#elif defined(CONFIG_DEBUG_UART_BASE) && (CONFIG_DEBUG_UART_BASE == 0xff178000)
	/* uart_sel_clk default select 24MHz */
	rk_clrsetreg(&cru->clksel_con[46],
		     UART5_PLL_SEL_MASK | UART5_DIV_CON_MASK,
		     UART5_PLL_SEL_24M << UART5_PLL_SEL_SHIFT | 0);
	rk_clrsetreg(&cru->clksel_con[47],
		     UART5_CLK_SEL_MASK,
		     UART5_CLK_SEL_UART5 << UART5_CLK_SEL_SHIFT);

	rk_clrsetreg(&grf->gpio3al_iomux,
		     GPIO3A2_MASK | GPIO3A1_MASK,
		     GPIO3A2_UART5_TX << GPIO3A2_SHIFT |
		     GPIO3A1_UART5_RX << GPIO3A1_SHIFT);
#else
	/* GRF_IOFUNC_CON0 */
	enum {
		CON_IOMUX_UART2SEL_SHIFT	= 10,
		CON_IOMUX_UART2SEL_MASK = 3 << CON_IOMUX_UART2SEL_SHIFT,
		CON_IOMUX_UART2SEL_M0	= 0,
		CON_IOMUX_UART2SEL_M1,
		CON_IOMUX_UART2SEL_USBPHY,
	};

	/* uart_sel_clk default select 24MHz */
	rk_clrsetreg(&cru->clksel_con[37],
		     UART2_PLL_SEL_MASK | UART2_DIV_CON_MASK,
		     UART2_PLL_SEL_24M << UART2_PLL_SEL_SHIFT | 0);
	rk_clrsetreg(&cru->clksel_con[38],
		     UART2_CLK_SEL_MASK,
		     UART2_CLK_SEL_UART2 << UART2_CLK_SEL_SHIFT);

#if (CONFIG_DEBUG_UART2_CHANNEL == 1)
	/* Enable early UART2 */
	rk_clrsetreg(&grf->iofunc_con0,
		     CON_IOMUX_UART2SEL_MASK,
		     CON_IOMUX_UART2SEL_M1 << CON_IOMUX_UART2SEL_SHIFT);

	/*
	 * Set iomux to UART2_M0 and UART2_M1.
	 * Because uart2_rxm0 and uart2_txm0 are default reset value,
	 * so only need set uart2_rxm1 and uart2_txm1 here.
	 */
	rk_clrsetreg(&grf->gpio2bh_iomux,
		     GPIO2B6_MASK | GPIO2B4_MASK,
		     GPIO2B6_UART2_RXM1 << GPIO2B6_SHIFT |
		     GPIO2B4_UART2_TXM1 << GPIO2B4_SHIFT);
#else
	rk_clrsetreg(&grf->iofunc_con0,
		     CON_IOMUX_UART2SEL_MASK,
		     CON_IOMUX_UART2SEL_M0 << CON_IOMUX_UART2SEL_SHIFT);

	rk_clrsetreg(&grf->gpio1dl_iomux,
		     GPIO1D3_MASK | GPIO1D2_MASK,
		     GPIO1D3_UART2_RXM0 << GPIO1D3_SHIFT |
		     GPIO1D2_UART2_TXM0 << GPIO1D2_SHIFT);
#endif /* CONFIG_DEBUG_UART2_CHANNEL == 1 */

#endif /* CONFIG_DEBUG_UART_BASE && CONFIG_DEBUG_UART_BASE == ... */
}

int set_armclk_rate(void)
{
	struct px30_clk_priv *priv;
	struct clk clk;
	int ret;

	ret = rockchip_get_clk(&clk.dev);
	if (ret) {
		printf("Failed to get clk dev\n");
		return ret;
	}
	clk.id = ARMCLK;
	priv = dev_get_priv(clk.dev);
	ret = clk_set_rate(&clk, priv->armclk_hz);
	if (ret < 0) {
		printf("Failed to set armclk %lu\n", priv->armclk_hz);
		return ret;
	}
	priv->set_armclk_rate = true;

	return 0;
}

static int fdt_fixup_cpu_opp_table(const void *blob)
{
	int opp_node, cpu_node, sub_node;
	int len;
	u32 phandle;
	u32 *pp;

	/* Replace opp table */
	opp_node = fdt_path_offset(blob, "/px30s-cpu0-opp-table");
	if (opp_node < 0) {
		printf("Failed to get px30s-cpu0-opp-table node\n");
		return -EINVAL;
	}

	phandle = fdt_get_phandle(blob, opp_node);
	if (!phandle) {
		printf("Failed to get cpu opp table phandle\n");
		return -EINVAL;
	}

	cpu_node = fdt_path_offset(blob, "/cpus");
	if (cpu_node < 0) {
		printf("Failed to get cpus node\n");
		return -EINVAL;
	}

	fdt_for_each_subnode(sub_node, blob, cpu_node) {
		pp = (u32 *)fdt_getprop(blob, sub_node, "operating-points-v2",
					&len);
		if (!pp)
			continue;
		pp[0] = cpu_to_fdt32(phandle);
	}

	return 0;
}

static int fdt_fixup_dmc_opp_table(const void *blob)
{
	int opp_node, dmc_node;
	int len;
	u32 phandle;
	u32 *pp;

	opp_node = fdt_path_offset(blob, "/px30s-dmc-opp-table");
	if (opp_node < 0) {
		printf("Failed to get px30s-dmc-opp-table node\n");
		return -EINVAL;
	}

	phandle = fdt_get_phandle(blob, opp_node);
	if (!phandle) {
		printf("Failed to get dmc opp table phandle\n");
		return -EINVAL;
	}

	dmc_node = fdt_path_offset(blob, "/dmc");
	if (dmc_node < 0) {
		printf("Failed to get dmc node\n");
		return -EINVAL;
	}

	pp = (u32 *)fdt_getprop(blob, dmc_node, "operating-points-v2", &len);
	if (!pp)
		return 0;
	pp[0] = cpu_to_fdt32(phandle);

	return 0;
}

static int fdt_fixup_gpu_opp_table(const void *blob)
{
	int opp_node, gpu_node;
	int len;
	u32 phandle;
	u32 *pp;

	opp_node = fdt_path_offset(blob, "/px30s-gpu-opp-table");
	if (opp_node < 0) {
		printf("Failed to get px30s-gpu-opp-table node\n");
		return -EINVAL;
	}

	phandle = fdt_get_phandle(blob, opp_node);
	if (!phandle) {
		printf("Failed to get gpu opp table phandle\n");
		return -EINVAL;
	}

	gpu_node = fdt_path_offset(blob, "/gpu@ff400000");
	if (gpu_node < 0) {
		printf("Failed to get gpu node\n");
		return -EINVAL;
	}

	pp = (u32 *)fdt_getprop(blob, gpu_node, "operating-points-v2", &len);
	if (!pp)
		return 0;
	pp[0] = cpu_to_fdt32(phandle);

	return 0;
}

static int fdt_fixup_bus_apll(const void *blob)
{
	char path[] = "/bus-apll";

	do_fixup_by_path((void *)blob, path, "status", "disabled", sizeof("disabled"), 0);

	return 0;
}

static int fdt_fixup_cpu_gpu_clk(const void *blob)
{
	int cpu_node, gpu_node, scmi_clk_node;
	int len;
	u32 phandle;
	u32 *pp;

	scmi_clk_node = fdt_path_offset(blob, "/firmware/scmi/protocol@14");
	if (scmi_clk_node < 0) {
		printf("Failed to get px30s scmi clk node\n");
		return -EINVAL;
	}

	phandle = fdt_get_phandle(blob, scmi_clk_node);
	if (!phandle)
		return 0;

	cpu_node = fdt_path_offset(blob, "/cpus/cpu@0");
	if (cpu_node < 0) {
		printf("Failed to get px30s cpu node\n");
		return -EINVAL;
	}
	/*
	 * before fixed:
	 *	clocks = <&cru ARMCLK>;
	 * after fixed:
	 *	clocks = <&scmi_clk 0>;
	 */
	pp = (u32 *)fdt_getprop(blob, cpu_node,
				"clocks",
				&len);
	if (!pp)
		return 0;
	if ((len / 8) >= 1) {
		pp[0] = cpu_to_fdt32(phandle);
		pp[1] = cpu_to_fdt32(0);
	}

	gpu_node = fdt_path_offset(blob, "/gpu@ff400000");
	if (gpu_node < 0) {
		printf("Failed to get px30s gpu node\n");
		return -EINVAL;
	}
	/*
	 * before fixed:
	 *	clocks = <&cru SCLK_GPU>;
	 * after fixed:
	 *	clocks = <&scmi_clk 1>;
	 */
	pp = (u32 *)fdt_getprop(blob, gpu_node,
				"clocks",
				&len);
	if (!pp)
		return 0;
	if ((len / 8) >= 1) {
		pp[0] = cpu_to_fdt32(phandle);
		pp[1] = cpu_to_fdt32(1);
	}
	return 0;
}

static int fdt_fixup_i2s_soft_reset(const void *blob)
{
	int node;
	int len;
	u32 *pp;

	node = fdt_path_offset(blob, "/i2s@ff060000");
	if (node < 0) {
		printf("Failed to get px30s i2s node\n");
		return -EINVAL;
	}

	/*
	 * before fixed:
	 *	resets = <&cru SRST_I2S0_TX>, <&cru SRST_I2S0_RX>;
	 * after fixed:
	 *	resets = <&cru SRST_I2S0_TX>, <&cru 128>;
	 */
	pp = (u32 *)fdt_getprop(blob, node,
				"resets",
				&len);
	if (!pp)
		return 0;
	if ((len / 8) >= 2)
		pp[3] = cpu_to_fdt32(128);

	return 0;
}

int rk_board_fdt_fixup(const void *blob)
{
	if (soc_is_px30s()) {
		fdt_increase_size((void *)blob, SZ_8K);
		fdt_fixup_cpu_opp_table(blob);
		fdt_fixup_dmc_opp_table(blob);
		fdt_fixup_gpu_opp_table(blob);
		fdt_fixup_bus_apll(blob);
		fdt_fixup_cpu_gpu_clk(blob);
		fdt_fixup_i2s_soft_reset(blob);
	}

	return 0;
}

int rk_board_early_fdt_fixup(const void *blob)
{
	rk_board_fdt_fixup(blob);

	return 0;
}

#if !defined(CONFIG_SPL_BUILD) && defined(CONFIG_ROCKCHIP_DMC_FSP)
int rk_board_init(void)
{
	struct udevice *dev;
	u32 ret = 0;

	ret = uclass_get_device_by_driver(UCLASS_DMC, DM_GET_DRIVER(dmc_fsp), &dev);
	if (ret) {
		printf("dmc_fsp failed, ret=%d\n", ret);
		return 0;
	}

	return 0;
}
#endif
