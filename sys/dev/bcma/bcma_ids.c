/*
 * bcma_ids.c
 *
 *  Created on: Jan 29, 2016
 *      Author: mizhka
 */

#include <dev/bcma/bcma_ids.h>

const char* bcma_ids_lookup(u_int16_t id, struct bcma_id* root){
	struct bcma_id* ptr = root;

	while(1){
		if((ptr->id == id) || (ptr->id == 0))
			return ptr->name;
		ptr++;
	}
}

//IDs
struct bcma_id bcma_vendors[] = {{ BCMA_MANUF_ARM, "ARM" },
								 { BCMA_MANUF_BCM, "Broadcom"},
								 { BCMA_MANUF_MIPS,"MIPS"},
								 { 0, "UNKNOWN"} };

struct bcma_id bcma_core_ids[] = {
		{ BCMA_CORE_OOB_ROUTER, "OOB Router" },
		{ BCMA_CORE_INVALID, "Invalid" },
		{ BCMA_CORE_CHIPCOMMON, "ChipCommon" },
		{ BCMA_CORE_ILINE20, "ILine 20" },
		{ BCMA_CORE_SRAM, "SRAM" },
		{ BCMA_CORE_SDRAM, "SDRAM" },
		{ BCMA_CORE_PCI, "PCI" },
		{ BCMA_CORE_MIPS, "MIPS" },
		{ BCMA_CORE_ETHERNET, "Fast Ethernet" },
		{ BCMA_CORE_V90, "V90" },
		{ BCMA_CORE_USB11_HOSTDEV, "USB 1.1 Hostdev" },
		{ BCMA_CORE_ADSL, "ADSL" },
		{ BCMA_CORE_ILINE100, "ILine 100" },
		{ BCMA_CORE_IPSEC, "IPSEC" },
		{ BCMA_CORE_UTOPIA, "UTOPIA" },
		{ BCMA_CORE_PCMCIA, "PCMCIA" },
		{ BCMA_CORE_INTERNAL_MEM, "Internal Memory" },
		{ BCMA_CORE_MEMC_SDRAM, "MEMC SDRAM" },
		{ BCMA_CORE_OFDM, "OFDM" },
		{ BCMA_CORE_EXTIF, "EXTIF" },
		{ BCMA_CORE_80211, "IEEE 802.11" },
		{ BCMA_CORE_PHY_A, "PHY A" },
		{ BCMA_CORE_PHY_B, "PHY B" },
		{ BCMA_CORE_PHY_G, "PHY G" },
		{ BCMA_CORE_MIPS_3302, "MIPS 3302" },
		{ BCMA_CORE_USB11_HOST, "USB 1.1 Host" },
		{ BCMA_CORE_USB11_DEV, "USB 1.1 Device" },
		{ BCMA_CORE_USB20_HOST, "USB 2.0 Host" },
		{ BCMA_CORE_USB20_DEV, "USB 2.0 Device" },
		{ BCMA_CORE_SDIO_HOST, "SDIO Host" },
		{ BCMA_CORE_ROBOSWITCH, "Roboswitch" },
		{ BCMA_CORE_PARA_ATA, "PATA" },
		{ BCMA_CORE_SATA_XORDMA, "SATA XOR-DMA" },
		{ BCMA_CORE_ETHERNET_GBIT, "GBit Ethernet" },
		{ BCMA_CORE_PCIE, "PCIe" },
		{ BCMA_CORE_PHY_N, "PHY N" },
		{ BCMA_CORE_SRAM_CTL, "SRAM Controller" },
		{ BCMA_CORE_MINI_MACPHY, "Mini MACPHY" },
		{ BCMA_CORE_ARM_1176, "ARM 1176" },
		{ BCMA_CORE_ARM_7TDMI, "ARM 7TDMI" },
		{ BCMA_CORE_PHY_LP, "PHY LP" },
		{ BCMA_CORE_PMU, "PMU" },
		{ BCMA_CORE_PHY_SSN, "PHY SSN" },
		{ BCMA_CORE_SDIO_DEV, "SDIO Device" },
		{ BCMA_CORE_ARM_CM3, "ARM CM3" },
		{ BCMA_CORE_PHY_HT, "PHY HT" },
		{ BCMA_CORE_MIPS_74K, "MIPS 74K" },
		{ BCMA_CORE_MAC_GBIT, "GBit MAC" },
		{ BCMA_CORE_DDR12_MEM_CTL, "DDR1/DDR2 Memory Controller" },
		{ BCMA_CORE_PCIE_RC, "PCIe Root Complex" },
		{ BCMA_CORE_OCP_OCP_BRIDGE, "OCP to OCP Bridge" },
		{ BCMA_CORE_SHARED_COMMON, "Common Shared" },
		{ BCMA_CORE_OCP_AHB_BRIDGE, "OCP to AHB Bridge" },
		{ BCMA_CORE_SPI_HOST, "SPI Host" },
		{ BCMA_CORE_I2S, "I2S" },
		{ BCMA_CORE_SDR_DDR1_MEM_CTL, "SDR/DDR1 Memory Controller" },
		{ BCMA_CORE_SHIM, "SHIM" },
		{ BCMA_CORE_DEFAULT, "Default" },
		{ APB_BRIDGE_CORE_ID, "APB bridge" },
		{ AXI_CORE_ID, "AXI/GPV" },
		{ EROM_CORE_ID, "EROM" },
		{ 0, "UNKNOWN"}
	};

struct bcma_id bcma_chipcommon_pll_types[] = {
		{ 0x02, "48Mhz base, 3 dividers" },
		{ 0x04, "48Mhz, 4 dividers" },
		{ 0x06, "25Mhz, 2 dividers" },
		{ 0x01, "48Mhz, 4 dividers" },
		{ 0x03, "25Mhz, 4 dividers" },
		{ 0x05, "100/200 or 120/240 only" },
		{ 0x07, "25Mhz, 4 dividers" },
		{ 0, "UNKNOWN"}
};

struct bcma_id bcma_chipcommon_external_buses[] = {
		{ 0x02, "ExtBus: ProgIf only" },
		{ 0x01, "ExtBus: PCMCIA, IDE and Prog" },
		{ 0x00, "No ExtBus present" }
};

struct bcma_id bcma_chipcommon_flash_types[] = {
		{ 0x01, "ST" },
		{ 0x02, "Atmel" },
		{ 0x07, "parallel" },
		{ 0x00, "none" }
};

struct bcma_id bcma_erom_addr_types[] = {
		{ 0x01, "        bridge" },
		{ 0x02, " slave wrapper" },
		{ 0x03, "master wrapper" },
		{ 0x00, "    slave port" }
};
