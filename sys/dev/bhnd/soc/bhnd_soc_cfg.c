/*
 * bnhd_soc_cfg.c
 *
 *  Created on: Feb 25, 2016
 *      Author: mizhka
 */

#include <sys/cdefs.h>
__FBSDID("$FreeBSD$");

#include <sys/param.h>

#include <dev/bhnd/bhnd_types.h>
#include <dev/bhnd/bhnd_ids.h>
#include <dev/bhnd/bhndb/bhndb.h>
#include <dev/bhnd/soc/bhnd_soc.h>

const struct bhndb_hwcfg bhnd_soc_bcma_generic_hwcfg = {
	.is_hostb_required  = false,
	.resource_specs		= (const struct resource_spec[]) {
		{ SYS_RES_MEMORY,	0,	RF_ACTIVE },
		{ -1,			0,		0 }
	},

	.register_windows	= (const struct bhndb_regwin[]) {
		/* 0x18000000: chipc core registers */
		{
			.win_type	= BHNDB_REGWIN_T_CORE,
			.win_offset	= BHND_SOC_CCREGS_OFFSET,
			.win_size	= BHND_SOC_CCREGS_SIZE,
			.res		= {
					.type = SYS_RES_MEMORY,
					.rid  = 0
			},
			.win_spec.core 		= {
					.class	= BHND_DEVCLASS_CC,
					.unit	= 0,
					.port	= 0,
					.region	= 0,
					.port_type = BHND_PORT_DEVICE
			}
		},
		{
			.win_type	= BHNDB_REGWIN_T_FIXED,
			.win_offset	= BHND_SOC_MAPPED_OFFSET,
			.win_size	= BHND_SOC_MAPPED_SIZE,
			.res		= {
					.type = SYS_RES_MEMORY,
					.rid  = 0
			}
		},
		BHNDB_REGWIN_TABLE_END
	},
};

const struct bhndb_hw bhnd_soc_generic_hw_table[] = {
	/* SoC WLAN */
	{
		.name = "Asus RT-N16 SoC WLAN",
		.hw_reqs = (const struct bhnd_core_match[])
		{
			/* PCIe Core */
			{
				.vendor	= BHND_MFGID_BCM,
				.device	= BHND_COREID_PCIE,
				.hwrev	= {
					.start	= 14,
					.end	= 14
				},
				.class	= BHND_DEVCLASS_PCIE,
				.unit	= 0
			},

			/* 802.11 Core */
			{
				.vendor	= BHND_MFGID_BCM,
				.device	= BHND_COREID_D11,
				.hwrev	= {
					.start	= 0,
					.end	= BHND_HWREV_INVALID
				},
				.class	= BHND_DEVCLASS_WLAN,
				.unit	= 0
			},

			/* Gigabit Core */
			{
				.vendor = BHND_MFGID_BCM,
				.device = BHND_COREID_GMAC,
				.hwrev  = {
						.start = 0,
						.end   = BHND_HWREV_INVALID
				},
				.class  = BHND_DEVCLASS_ENET,
				.unit	= 0
			}
		},
		.num_hw_reqs = 3,
		.cfg = &bhnd_soc_bcma_generic_hwcfg
	},
	{ NULL, NULL, 0, NULL }
};
