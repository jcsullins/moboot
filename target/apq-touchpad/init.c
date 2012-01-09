/*
 * Copyright (c) 2011, Code Aurora Forum. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *  * Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *  * Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 *  * Neither the name of Google, Inc. nor the names of its contributors
 *    may be used to endorse or promote products derived from this
 *    software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
 * COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS
 * OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
 * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT
 * OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <reg.h>
#include <debug.h>
#include <smem.h>
#include <uart_dm.h>
#include <gsbi.h>
#include <baseband.h>
#include <dev/keys.h>
#include <dev/pm8921.h>
#include <dev/ssbi.h>
#include <platform/iomap.h>
#include <platform/pmic.h>
#include <lib/ptable.h>
#include <string.h>
#include <lib/atags.h>

#define LINUX_MACHTYPE_APQ8064_SIM     3572

extern unsigned int mmc_boot_main(unsigned char slot, unsigned int base);
extern void mdelay(unsigned msecs);
extern void keypad_init(void);
extern void display_init(void);

static unsigned mmc_sdc_base[] =
{
	MSM_SDC1_BASE,
	MSM_SDC2_BASE,
	MSM_SDC3_BASE,
	MSM_SDC4_BASE
};

static pm8921_dev_t pmic;
static const uint8_t uart_gsbi_id  = GSBI_ID_3;

enum topaz_board_types {
	TOPAZ_PROTO = 0,
	TOPAZ_PROTO2,
	TOPAZ_EVT1,
	TOPAZ_EVT2,
	TOPAZ_EVT3,
	TOPAZ_DVT,
	TOPAZ_PVT,
	TOPAZ_3G_PROTO,
	TOPAZ_3G_PROTO2,
	TOPAZ_3G_EVT1,
	TOPAZ_3G_EVT2,
	TOPAZ_3G_EVT3,
	TOPAZ_3G_EVT4,
	TOPAZ_3G_DVT,
	TOPAZ_3G_PVT,
	TOPAZ_END
};

static unsigned board_type = TOPAZ_EVT1;

unsigned board_type_is_3g = 0;

static struct {
	enum topaz_board_types type;
	const char *str;
} boardtype_tbl[] = {
	/* WiFi */
	{TOPAZ_PROTO,    "topaz-Wifi-proto"},
	{TOPAZ_PROTO2,   "topaz-Wifi-proto2"},
	{TOPAZ_EVT1,     "topaz-Wifi-evt1"},
	{TOPAZ_EVT2,     "topaz-Wifi-evt2"},
	{TOPAZ_EVT3,     "topaz-Wifi-evt3"},
	{TOPAZ_DVT,      "topaz-Wifi-dvt"},
	{TOPAZ_PVT,      "topaz-Wifi-pvt"},

	/* 3G */
	{TOPAZ_3G_PROTO,    "topaz-3G-proto"},
	{TOPAZ_3G_PROTO2,   "topaz-3G-proto2"},
	{TOPAZ_3G_EVT1,     "topaz-3G-evt1"},
	{TOPAZ_3G_EVT2,     "topaz-3G-evt2"},
	{TOPAZ_3G_EVT3,     "topaz-3G-evt3"},
	{TOPAZ_3G_EVT4,     "topaz-3G-evt4"},
	{TOPAZ_3G_DVT,      "topaz-3G-dvt"},
	{TOPAZ_3G_PVT,      "topaz-3G-pvt"},

	/* TODO: Non-standard board strings, to be removed once all copies of
	 * bootie in the wild are updated to use the above format */

	/* WiFi */
	{TOPAZ_PROTO,    "topaz-1stbuild-Wifi"},
	{TOPAZ_PROTO2,   "topaz-2ndbuild-Wifi"},
	{TOPAZ_EVT1,     "topaz-3rdbuild-Wifi"},
	{TOPAZ_EVT2,     "topaz-4thbuild-Wifi"},
	{TOPAZ_EVT3,     "topaz-5thbuild-Wifi"},
	{TOPAZ_DVT,      "topaz-6thbuild-Wifi"},
	{TOPAZ_PVT,      "topaz-7thbuild-Wifi"},
	{TOPAZ_PVT,      "topaz-pvt-Wifi"},

	/* 3G */
	{TOPAZ_3G_PROTO,    "topaz-1stbuild-3G"},
	{TOPAZ_3G_PROTO2,   "topaz-2ndbuild-3G"},
	{TOPAZ_3G_EVT1,     "topaz-3rdbuild-3G"},
	{TOPAZ_3G_EVT2,     "topaz-4thbuild-3G"},
	{TOPAZ_3G_EVT3,     "topaz-5thbuild-3G"},
	{TOPAZ_3G_DVT,      "topaz-6thbuild-3G"},
	{TOPAZ_3G_PVT,      "topaz-7thbuild-3G"},
	{TOPAZ_3G_PVT,      "topaz-pvt-3G"},
	{TOPAZ_END,			"invalid"}
};


void boardtype_init()
{
	char *boardtype_str;
	int i;

	boardtype_str = atags_get_cmdline_arg(NULL, "boardtype");

	if (strlen(boardtype_str) < 12) {
		free(boardtype_str);
		return;
	}

	boardtype_str += 11;

	for (i = 0; boardtype_tbl[i].type != TOPAZ_END; i++)
		if (!strcmp(boardtype_str, boardtype_tbl[i].str))
			board_type = boardtype_tbl[i].type;

	if (board_type >= TOPAZ_3G_PROTO) {
		board_type_is_3g = 1;
	}

	if (!strcmp(boardtype_str, "opal-3G-evt3")) {
		board_type_is_3g = 1;
	}

	boardtype_str -= 11;
	free(boardtype_str);
}

void target_init(void)
{
	uint32_t base_addr;
	uint8_t slot;
	dprintf(SPEW, "target_init()\n");

	/* Initialize PMIC driver */
	pmic.read  = pa1_ssbi2_read_bytes;
	pmic.write = pa1_ssbi2_write_bytes;

	boardtype_init();

	pm8921_init(&pmic);

	/* Keypad init */
	// keys_init(); // -JCS conflict w/ pm8048 stuff
	// keypad_init(); // -JCS  conflict as above
	//
	struct pm8058_gpio usbhost_gpio_cfg = {
		.direction = PM_GPIO_DIR_IN,
		.pull = PM_GPIO_PULL_NO,
		.vin_sel = PM_GPIO_VIN_S3,	
		.out_strength = PM_GPIO_STRENGTH_NO,
		.function = PM_GPIO_FUNC_NORMAL,
		.inv_int_pol = 0,
	};


	display_init();

	if (board_type_is_3g) {
		// 3G VolUp/VolDn
		pm8058_gpio_config(5, &usbhost_gpio_cfg);
		pm8058_gpio_config(6, &usbhost_gpio_cfg);
	}

	/* Trying Slot 1 first */
	slot = 1;
	base_addr = mmc_sdc_base[slot-1];
	if(mmc_boot_main(slot, base_addr))
	{
		/* Trying Slot 3 next */
		slot = 3;
		base_addr = mmc_sdc_base[slot-1];
		if(mmc_boot_main(slot, base_addr))
		{
			dprintf(CRITICAL, "mmc init failed!");
			ASSERT(0);
		}
	}
}

uint32_t board_machtype(void)
{
	struct smem_board_info_v6 board_info_v6;
	uint32_t board_info_len = 0;
	uint32_t smem_status = 0;
	uint32_t format = 0;
	uint32_t id = HW_PLATFORM_UNKNOWN;
	uint32_t mach_id;


	smem_status = smem_read_alloc_entry_offset(SMEM_BOARD_INFO_LOCATION,
											   &format, sizeof(format), 0);
	if(!smem_status)
	{
		if (format == 6)
		{
			board_info_len = sizeof(board_info_v6);

			smem_status = smem_read_alloc_entry(SMEM_BOARD_INFO_LOCATION,
												&board_info_v6, board_info_len);
			if(!smem_status)
			{
				id = board_info_v6.board_info_v3.hw_platform;
			}
		}
	}

	/* Detect the board we are running on */
	switch(id)
	{
		/* APQ8064 machine id not yet defined for CDP etc.
		 * default to simulator.
		 */
		case HW_PLATFORM_SURF:
		case HW_PLATFORM_FFA:
		case HW_PLATFORM_FLUID:
		default:
			mach_id = LINUX_MACHTYPE_APQ8064_SIM;
	};

	return mach_id;
}

static void set_dload_mode(int on)
{
	unsigned dload_mode_addr = 0x2A05F000;

	if (dload_mode_addr) {
		writel(on ? 0xE47B337D : 0, dload_mode_addr);
		writel(on ? 0xCE14091A : 0,
		       dload_mode_addr + sizeof(unsigned int));
		dmb();
	}
}

void reboot_device(uint32_t reboot_reason)
{

	/* TBD - set download mode? */
	set_dload_mode(0);
	
	pm8058_reset_pwr_off(1);

	writel(reboot_reason, RESTART_REASON_ADDR);
	dmb();

	writel(0, MSM_WDT0_EN);
	writel(0, PSHOLD_CTL_SU);
	mdelay(5000);

	writel(0x31F3, MSM_WDT0_BARK_TIME);
	writel(0x31F3, MSM_WDT0_BITE_TIME);
	writel(3, MSM_WDT0_EN);
	dmb();

	secure_writel(3, MSM_TCSR_WDOG_CFG);
	mdelay(10000);

	dprintf(CRITICAL, "Shutdown failed\n");
}

uint32_t check_reboot_mode(void)
{
	uint32_t restart_reason = 0;

	/* Read reboot reason and scrub it */
	restart_reason = readl(RESTART_REASON_ADDR);
	writel(0x00, RESTART_REASON_ADDR);

	return restart_reason;
}

void target_serialno(unsigned char *buf)
{
	uint32_t serialno;
	if(target_is_emmc_boot())
	{
		serialno =  mmc_get_psn();
		sprintf(buf,"%x",serialno);
	}
}

/* Do any target specific intialization needed before entering fastboot mode */
void target_fastboot_init(void)
{
	/* Set the BOOT_DONE flag in PM8921 */
	pm8921_boot_done();
}

/* GSBI to be used for UART */
uint8_t target_uart_gsbi(void)
{
	return uart_gsbi_id;
}

/* Returns type of baseband. APQ for APPs only proc. */
unsigned target_baseband()
{
	return BASEBAND_APQ;
}
