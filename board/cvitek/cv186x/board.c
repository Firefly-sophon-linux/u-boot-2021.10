// SPDX-License-Identifier: GPL-2.0+
/*
 * (C) Copyright 2013
 * David Feng <fenghua@phytium.com.cn>
 * Sharma Bhupesh <bhupesh.sharma@freescale.com>
 *
 */
#include <common.h>
#include <dm.h>
#include <malloc.h>
#include <errno.h>
#include <asm/io.h>
#include <console.h>
#include <linux/compiler.h>
#if defined(__aarch64__)
#include <asm/armv8/mmu.h>
#endif
#include <usb/dwc2_udc.h>
#include <usb.h>
#include "cv186x_reg.h"
#include "mmio.h"
#include "pinmux/cv186x_pinmux.h"
#include <linux/delay.h>
#include <bootstage.h>
#include <version.h>
#include <command.h>
#include <mmc.h>
#include <net.h>

#ifdef CONFIG_VIDEO_SOPH
#include <video_soph.h>
#endif

#include <linux/stringify.h>

#if defined(__riscv)
#include <asm/csr.h>
#endif

DECLARE_GLOBAL_DATA_PTR;
#define SD1_SDIO_PAD

#if defined(__aarch64__)
static struct mm_region cv186x_mem_map[] = {
	{
		.virt = 0x0UL,
		.phys = 0x0UL,
		.size = 0x80000000UL,
		.attrs = PTE_BLOCK_MEMTYPE(MT_DEVICE_NGNRNE) |
			 PTE_BLOCK_NON_SHARE |
			 PTE_BLOCK_PXN | PTE_BLOCK_UXN
	}, {
		.virt = PHYS_SDRAM_1,
		.phys = PHYS_SDRAM_1,
		.size = PHYS_SDRAM_1_SIZE,
		.attrs = PTE_BLOCK_MEMTYPE(MT_NORMAL) |
			 PTE_BLOCK_INNER_SHARE
	}, {
		/* List terminator */
		0,
	}
};

struct mm_region *mem_map = cv186x_mem_map;
#endif

void pinmux_config(int io_type)
{
		switch (io_type) {
		case PINMUX_UART0:
			PINMUX_CONFIG(UART0_TX, UART0_TX, G11);
			PINMUX_CONFIG(UART0_RX, UART0_RX, G11);
		break;
		case PINMUX_SDIO0:
			/* set pwr switch pin for gpio */
			PINMUX_CONFIG(PAD_VIVO0_D15, GPIO133, G5);
			PINMUX_CONFIG(SD0_CLK, SD0_CLK, G8);
			PINMUX_CONFIG(SD0_CMD, SD0_CMD, G8);
			PINMUX_CONFIG(SD0_D0, SD0_D0, G8);
			PINMUX_CONFIG(SD0_D1, SD0_D1, G8);
			PINMUX_CONFIG(SD0_D2, SD0_D2, G8);
			PINMUX_CONFIG(SD0_D3, SD0_D3, G8);
			//PINMUX_CONFIG(SD0_CD_X, GPIO53, G9);
			PINMUX_CONFIG(SD0_CD_X, SD0_CD_X, G9);
			PINMUX_CONFIG(SD0_PWR_EN, SD0_PWR_EN, G9);
			break;
		case PINMUX_SDIO1:
			/* set pwr switch pin for gpio */
			PINMUX_CONFIG(PAD_VIVO0_D16, GPIO134, G5);
			PINMUX_CONFIG(SD1_CLK, SD1_CLK, G11);
			PINMUX_CONFIG(SD1_CMD, SD1_CMD, G11);
			PINMUX_CONFIG(SD1_D0, SD1_D0, G11);
			PINMUX_CONFIG(SD1_D1, SD1_D1, G11);
			PINMUX_CONFIG(SD1_D2, SD1_D2, G11);
			PINMUX_CONFIG(SD1_D3, SD1_D3, G11);
			//PINMUX_CONFIG(SD1_CD_X, GPIO61, G12);
			PINMUX_CONFIG(SD1_CD_X, SD1_CD_X, G12);
			PINMUX_CONFIG(SD1_PWR_EN, SD1_PWR_EN, G12);
			break;
		case PINMUX_SDIO2:
#ifdef	SD2_SDIO_PWM
			/* 0:sd2 ctrl conncect to PWM and FAN IOs */
			mmio_clrsetbits_32(TOP_BASE + 0x294, 0x1 << 26, 0);
			PINMUX_CONFIG(PWM0, SD2_0_CLK, G9);
			PINMUX_CONFIG(PWM1, SD2_0_CMD, G9);
			PINMUX_CONFIG(PWM2, SD2_0_D0, G9);
			PINMUX_CONFIG(PWM3, SD2_0_D1, G9);
			PINMUX_CONFIG(FAN0, SD2_0_D2, G9);
			PINMUX_CONFIG(FAN1, SD2_0_D3, G9);
#else
			mmio_clrsetbits_32(TOP_BASE + 0x294, 0x1 << 26, 0x1 << 26);
			mmio_clrsetbits_32(0x28104c00, 0xf0, 0x2 << 4);
			mmio_clrsetbits_32(0x28104c04, 0xf0, 0x2 << 4);
			mmio_clrsetbits_32(0x28104c08, 0xf0, 0x2 << 4);
			mmio_clrsetbits_32(0x28104c10, 0xf0, 0x2 << 4);
			mmio_clrsetbits_32(0x28104c14, 0xf0, 0x2 << 4);
			mmio_clrsetbits_32(0x28104c18, 0xf0, 0x2 << 4);
#endif
			break;
		case PINMUX_EMMC:
			PINMUX_CONFIG(EMMC_CLK, EMMC_CLK, G11);
			PINMUX_CONFIG(EMMC_RST_X, EMMC_RST_X, G11);
			PINMUX_CONFIG(EMMC_CMD, EMMC_CMD, G11);
			PINMUX_CONFIG(EMMC_D0, EMMC_D0, G11);
			PINMUX_CONFIG(EMMC_D1, EMMC_D1, G11);
			PINMUX_CONFIG(EMMC_D2, EMMC_D2, G11);
			PINMUX_CONFIG(EMMC_D3, EMMC_D3, G11);
			break;
		case PINMUX_SPI_NAND:
			PINMUX_CONFIG(EMMC_CLK, SPINAND_SCK, G11);
			PINMUX_CONFIG(EMMC_CMD, SPINAND_SDI, G11);
			PINMUX_CONFIG(EMMC_D1, SPINAND_CS_X, G11);
			PINMUX_CONFIG(EMMC_D0, SPINAND_SDO, G11);
			PINMUX_CONFIG(EMMC_D2, SPINAND_HOLD_X, G11);
			PINMUX_CONFIG(EMMC_D3, SPINAND_WP_X, G11);
			// set pin pull, make sure to follow the rom
			PIN_PULL_CONFIG_2(EMMC_CLK, G11, 0);
			PIN_PULL_CONFIG_2(EMMC_CMD, G11, 1);
			PIN_PULL_CONFIG_2(EMMC_D0, G11, 1);
			PIN_PULL_CONFIG_2(EMMC_D1, G11, 1);
			PIN_PULL_CONFIG_2(EMMC_D2, G11, 1);
			PIN_PULL_CONFIG_2(EMMC_D3, G11, 1);
			break;
		case PINMUX_SPI_NOR:
			PINMUX_CONFIG(EMMC_CLK, SPINOR_SCK, G11);
			PINMUX_CONFIG(EMMC_CMD, SPINOR_SDI, G11);
			PINMUX_CONFIG(EMMC_D1, SPINOR_CS_X, G11);
			PINMUX_CONFIG(EMMC_D0, SPINOR_SDO, G11);
			PINMUX_CONFIG(EMMC_D2, SPINOR_HOLD_X, G11);
			PINMUX_CONFIG(EMMC_D3, SPINOR_WP_X, G11);
		break;
		case PINMUX_SPI0:
			/* set for gpio for spix_cs */
			PINMUX_CONFIG(SPI0_CS_X, GPIO107, G9);
			PINMUX_CONFIG(SPI0_SDI, SPI0_SDI, G9);
			PINMUX_CONFIG(SPI0_SDO, SPI0_SDO, G9);
			PINMUX_CONFIG(SPI0_SCK, SPI0_SCK, G9);
		break;
		case PINMUX_DSI:
			// PINMUX_CONFIG(PAD_MIPI_TXM0, XGPIOC_12);
			// PINMUX_CONFIG(PAD_MIPI_TXP0, XGPIOC_13);
			// PINMUX_CONFIG(PAD_MIPI_TXM1, XGPIOC_14);
			// PINMUX_CONFIG(PAD_MIPI_TXP1, XGPIOC_15);
			// PINMUX_CONFIG(PAD_MIPI_TXM2, XGPIOC_16);
			// PINMUX_CONFIG(PAD_MIPI_TXP2, XGPIOC_17);
			// PINMUX_CONFIG(PAD_MIPI_TXM3, XGPIOC_20);
			// PINMUX_CONFIG(PAD_MIPI_TXP3, XGPIOC_21);
			// PINMUX_CONFIG(PAD_MIPI_TXM4, XGPIOC_18);
			// PINMUX_CONFIG(PAD_MIPI_TXP4, XGPIOC_19);
		break;
		case PINMUX_LVDS:
			// PINMUX_CONFIG(PAD_MIPI_TXM0, XGPIOC_12);
			// PINMUX_CONFIG(PAD_MIPI_TXP0, XGPIOC_13);
			// PINMUX_CONFIG(PAD_MIPI_TXM1, XGPIOC_14);
			// PINMUX_CONFIG(PAD_MIPI_TXP1, XGPIOC_15);
			// PINMUX_CONFIG(PAD_MIPI_TXM2, XGPIOC_16);
			// PINMUX_CONFIG(PAD_MIPI_TXP2, XGPIOC_17);
			// PINMUX_CONFIG(PAD_MIPI_TXM3, XGPIOC_20);
			// PINMUX_CONFIG(PAD_MIPI_TXP3, XGPIOC_21);
			// PINMUX_CONFIG(PAD_MIPI_TXM4, XGPIOC_18);
			// PINMUX_CONFIG(PAD_MIPI_TXP4, XGPIOC_19);
		break;
		case PINMUX_I2C0:
			PINMUX_CONFIG(IIC0_SCL, IIC0_SCL, G12);
			PINMUX_CONFIG(IIC0_SDA, IIC0_SDA, G12);
		break;
		case PINMUX_I2S0:
			PINMUX_CONFIG(I2S0_SCLK, I2S0_SCLK, G6);
			PINMUX_CONFIG(I2S0_WSI, I2S0_WSI, G6);
			PINMUX_CONFIG(I2S0_SDI0, I2S0_SDI0, G6);
			PINMUX_CONFIG(I2S0_SDI1, I2S0_SDI1, G6);
			PINMUX_CONFIG(I2S0_SDO, I2S0_SDO, G6);
			PINMUX_CONFIG(I2S0_MCLK, I2S0_MCLK, G6);
		break;
		case PINMUX_I2S1:
			PINMUX_CONFIG(PAD_AOUTL0, I2S1_SCLK, PHY);
			PINMUX_CONFIG(PAD_AOUTR0, I2S1_WSI, PHY);
			PINMUX_CONFIG(PAD_AINL0_MIC, I2S1_SDI, PHY);
			PINMUX_CONFIG(PAD_AINR0_MIC, I2S1_SDO, PHY);
			PINMUX_CONFIG(PWR_GPIO1, I2S1_MCLK, G7);
		break;
		case PINMUX_I2S2:
			PINMUX_CONFIG(PWR_GPIO0, I2S2_MCLK, G7);
			PINMUX_CONFIG(PAD_AOUTL1, I2S2_SCLK, PHY);
			PINMUX_CONFIG(PAD_AOUTR1, I2S2_WSI, PHY);
			PINMUX_CONFIG(PAD_AINL1_MIC, I2S2_SDI, PHY);
			PINMUX_CONFIG(PAD_AINR1_MIC, I2S2_SDO, PHY);
		break;
		case PINMUX_WGN0:
			PINMUX_CONFIG(CAN0_TX, WG0_D0, G11);
			PINMUX_CONFIG(CAN0_RX, WG0_D1, G11);
		break;
		default:
			break;
	}

	// Pins of mipi used for other function
	mmio_write_32(0x6700b0c0, 0x3fffffff);
	mmio_write_32(0x6700b160, 0x0);
}

#include "../cvi_board_init.c"

void cpu_pwr_ctrl(void)
{
#if defined(CONFIG_RISCV)
	mmio_write_32(0x01901008, 0x30001);// cortexa53_pwr_iso_en
#elif defined(CONFIG_ARM)
	//mmio_write_32(0x01901004, 0x30001);// c906_top_pwr_iso_en   (pld mark)
#endif
}

void bm_storage_boot_loader_version_uboot(void)
{
	int size = 0;

	size = strlen(version_string);
	for (int i = 0; i < size; i++)
		mmio_write_8(UBOOT_VERSION_BASE + i, version_string[i]);

	mmio_write_8(UBOOT_VERSION_BASE + size, '\0');
}

#ifdef CONFIG_VIDEO_SOPH
void board_show_logo(void)
{
	struct udevice *dev;

	uclass_get_device_by_driver(UCLASS_VIDEO,
			DM_DRIVER_GET(soph_display), &dev);
	soph_show_logo();
}
#endif

int board_init(void)
{
#if (!defined CONFIG_TARGET_CVITEK_CV181X_FPGA) && (!defined CONFIG_TARGET_CVITEK_CV186X_FPGA) && \
	(!defined CV186X_FPGA_PALLDIUM_ENV)
	// extern uint32_t BOOT0_START_TIME;
	//uint16_t start_time = DIV_ROUND_UP(BOOT0_START_TIME, SYS_COUNTER_FREQ_IN_SECOND / 1000);

	// Save uboot start time. time is from boot0.h
	//mmio_write_16(TIME_RECORDS_FIELD_UBOOT_START, start_time);
#endif

	cpu_pwr_ctrl();

	pinmux_config(PINMUX_SDIO0);
#if defined(CONFIG_NAND_SUPPORT)
	pinmux_config(PINMUX_SPI_NAND);
#elif defined(CONFIG_SPI_FLASH)
	pinmux_config(PINMUX_SPI_NOR);
#elif defined(CONFIG_EMMC_SUPPORT)
	pinmux_config(PINMUX_EMMC);
#endif
#ifdef CONFIG_DISPLAY_CVITEK_MIPI
	//pinmux_config(PINMUX_DSI);
#elif defined(CONFIG_DISPLAY_CVITEK_LVDS)
	//pinmux_config(PINMUX_LVDS);
#endif

	//pinmux_config(PINMUX_SDIO1);

	pinmux_config(PINMUX_I2C0);
	pinmux_config(PINMUX_SPI0);
	PINMUX_CONFIG(AUX0, GPIO7, G9);//unmute dac1

	PINMUX_CONFIG(UART4_TX, GPIO91, G12);
	PINMUX_CONFIG(UART4_RX, GPIO92, G12);
	//pinmux_config(PINMUX_I2S1);
	//pinmux_config(PINMUX_I2S2);
	// pinmux_config(PINMUX_WGN0);

	cvi_board_init();
	PINMUX_CONFIG(PWR_GPIO3, PWR_GPIO3, G7);
	PINMUX_CONFIG(PWR_GPIO4, PWR_GPIO4, G7);
	PINMUX_CONFIG(PWR_GPIO5, PWR_GPIO5, G7);
	PINMUX_CONFIG(PWR_GPIO6, PWR_GPIO6, G7);
	bm_storage_boot_loader_version_uboot();
	return 0;
}

#define DTSNAME_MAX_LEN 32
#define DEFAULT_DTSNAME "config-" __stringify(CVICHIP) "_" __stringify(CVIBOARD)

void get_dts_type_from_oem(unsigned char *dtsname)
{
	if (!dtsname) {
		printf("get dts type from oem failed!\n");
		return;
	}
#if 0
#ifdef CONFIG_EMMC_SUPPORT
	run_command("mmc dev 0 2\0", 0);
	run_command("mmc read 0x120000000 0 0xc0", 0);
	memcpy(dtsname, (void *)(0x120000000 + 0xa0), DTSNAME_MAX_LEN);
	dtsname[DTSNAME_MAX_LEN - 1] = '\0';
#endif
	// use default dts
	if (strlen(dtsname) == 0)
#endif
		memcpy(dtsname, DEFAULT_DTSNAME, sizeof(DEFAULT_DTSNAME));
#ifdef CONFIG_EMMC_SUPPORT
        run_command("mmc dev 0 2\0", 0);
        run_command("mmc read 0x120000000 0 1\0", 0); 
        unsigned char result = 0;
        memcpy(&result, (void *)(0x120000000 + 0xF0), 1);  // 读取 0x1200000F0 的数据
        
        unsigned char high_nibble = (result >> 4) & 0xF;  // 高 4 位
        unsigned char low_nibble = result & 0xF;          // 低 4 位
        
        printf("INFO: Read result from 0x1200000F0: high nibble = 0x%X, low nibble = 0x%X\n", high_nibble, low_nibble);
        if (result != 8) {
            char suffix[8]; 
            snprintf(suffix, sizeof(suffix), "_%uG", result);  
            strncat(dtsname, suffix, DTSNAME_MAX_LEN - strlen(dtsname) - 1);
        }
#endif
	printf("FORCE INFO: DTS_TYPE[%s]\n", dtsname);
}

int setup_sophgo_dts(void)
{
	char configName[DTSNAME_MAX_LEN * 2] = {0};
	char dtsType[DTSNAME_MAX_LEN] = {0};
	char *ptr = NULL;

	get_dts_type_from_oem(dtsType);

	ptr = strstr(dtsType, "config-");
	//If the string does not start with "config-"
	if (!ptr && ptr != dtsType) {
		sprintf(configName, "config-%s", dtsType);
		env_set("DTS_TYPE", configName);
		return 0;
	}

	env_set("DTS_TYPE", dtsType);
	return 0;
}
#if defined(CONFIG_ROOTFS_UBUNTU) || defined(CONFIG_ROOTFS_DEBIAN)
#ifdef CONFIG_BOARD_LATE_INIT
static void get_ether_addr_from_emmc(unsigned char *mac, int i)
{
	run_command("mmc dev 0 2\0", 0);
	run_command("mmc read 0x120000000 0 0x60", 0);
	memcpy(mac, (void *)(0x120000000 + 0x40 + i*0x10), 6);
}
#define ETHER_NUM 2
#define MAC_SIZE 6
static int setup_mac(void)
{
	int i;
	uint8_t mac[ETHER_NUM][MAC_SIZE];
	char eth[16];

	// get ethaddr from emmc boot1 part
	for (i = 0; i < ETHER_NUM; i++) {
		get_ether_addr_from_emmc(mac[i], i);
		if (i == 0)
			snprintf(eth, sizeof(eth), "ethaddr");
		else
			snprintf(eth, sizeof(eth), "eth%uaddr", i);

		if (is_zero_ethaddr(mac[i])) {
			debug("%s mac address not specified, use default\n",
						eth);
			continue;
		}

		if (!is_valid_ethaddr(mac[i])) {
			printf("Invalid mac address %pM\n", mac[i]);
			continue;
		}

		if (eth_env_set_enetaddr(eth, mac[i]) == -EEXIST)
			printf("mac%d address has benn set before\n", i);
	}

	return 0;
}

int board_late_init(void)
{
	console_record_reset_enable();
	setup_mac();
	setup_sophgo_dts();
#ifdef CONFIG_VIDEO_SOPH
	board_show_logo();
#endif
	cvi_board_late_init();

	return 0;
}
#endif
#else
int board_late_init(void)
{
	setup_sophgo_dts();
#ifdef CONFIG_VIDEO_SOPH
	board_show_logo();
#endif
	return 0;
}
#endif

#if defined(__aarch64__)
int dram_init(void)
{
	gd->ram_size = PHYS_SDRAM_1_SIZE;
	return 0;
}

int dram_init_banksize(void)
{
	gd->bd->bi_dram[0].start = PHYS_SDRAM_1;
	gd->bd->bi_dram[0].size = PHYS_SDRAM_1_SIZE;

	return 0;
}
#endif

#ifdef CV_SYS_OFF
static void cv_system_off(void)
{
	mmio_write_32(REG_RTC_BASE + RTC_EN_SHDN_REQ, 0x01);
	while (mmio_read_32(REG_RTC_BASE + RTC_EN_SHDN_REQ) != 0x01)
		;
	mmio_write_32(REG_RTC_CTRL_BASE + RTC_CTRL0_UNLOCKKEY, 0xAB18);
	mmio_setbits_32(REG_RTC_CTRL_BASE + RTC_CTRL0, 0xFFFF0800 | (0x1 << 0));

	while (1)
		;
}
#endif

void cv_system_reset(void)
{
	mmio_write_32(REG_RTC_BASE + RTC_EN_PWR_CYC_REQ, 0x01);
	while (mmio_read_32(REG_RTC_BASE + RTC_EN_PWR_CYC_REQ) != 0x01)
		;
	mmio_write_32(REG_RTC_CTRL_BASE + RTC_CTRL0_UNLOCKKEY, 0xAB18);
	mmio_setbits_32(REG_RTC_CTRL_BASE + RTC_CTRL0, 0xFFFF0800 | (0x1 << 3));

	while (1)
		;
}

/*
 * Board specific reset that is system reset.
 */
void reset_cpu(void)
{
	cv_system_reset();
}

#ifdef CONFIG_USB_GADGET_DWC2_OTG
struct dwc2_plat_otg_data cv182x_otg_data = {
	.regs_otg = USB_BASE,
	.usb_gusbcfg	= 0x40081400,
	.rx_fifo_sz	 = 512,
	.np_tx_fifo_sz  = 512,
	.tx_fifo_sz	 = 512,
};

int board_usb_init(int index, enum usb_init_type init)
{
	uint32_t value;

	value = mmio_read_32(TOP_BASE + REG_TOP_SOFT_RST) & (~BIT_TOP_SOFT_RST_USB);
	mmio_write_32(TOP_BASE + REG_TOP_SOFT_RST, value);
	udelay(50);
	value = mmio_read_32(TOP_BASE + REG_TOP_SOFT_RST) | BIT_TOP_SOFT_RST_USB;
	mmio_write_32(TOP_BASE + REG_TOP_SOFT_RST, value);

	/* Set USB phy configuration */
	value = mmio_read_32(REG_TOP_USB_PHY_CTRL);
	mmio_write_32(REG_TOP_USB_PHY_CTRL, value | BIT_TOP_USB_PHY_CTRL_EXTVBUS
					| USB_PHY_ID_OVERRIDE_ENABLE
					| USB_PHY_ID_VALUE);

	/* Enable ECO RXF */
	mmio_write_32(REG_TOP_USB_ECO, mmio_read_32(REG_TOP_USB_ECO) | BIT_TOP_USB_ECO_RX_FLUSH);

	printf("cvi_usb_hw_init done\n");

	return dwc2_udc_probe(&cv182x_otg_data);
}
#endif

void board_save_time_record(uintptr_t saveaddr)
{
	uint64_t boot_us = 0;
#if defined(__aarch64__)
	boot_us = timer_get_boot_us();
#elif defined(__riscv)
	// Read from CSR_TIME directly. RISC-V timers is initialized later.
	boot_us = csr_read(CSR_TIME) / (SYS_COUNTER_FREQ_IN_SECOND / 1000000);
#else
#error "Unknown ARCH"
#endif

	mmio_write_16(saveaddr, DIV_ROUND_UP(boot_us, 1000));
}

