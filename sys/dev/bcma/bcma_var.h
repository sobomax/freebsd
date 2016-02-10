/*
 * bcma_var.h
 *
 *  Created on: Jan 24, 2016
 *      Author: mizhka
 */
#pragma once

#include <sys/types.h>
#include <sys/malloc.h>
#include <sys/rman.h>
#include <sys/systm.h>
#include <sys/queue.h>

#include <machine/resource.h>
#include <dev/bcma/bcma_chipcommon.h>

MALLOC_DECLARE(M_BCMA);

//Masks
#define BCMA_BITS(value,MASK)		(value & MASK) >> MASK##_SHIFT;
#define BCMA_EROM_BITS(value,MASK)	(value & MASK) >> MASK##_SHIFT;

struct bcma_chipinfo {
	u_int16_t id;
	u_int8_t tbd :4;
	u_int8_t package :4;
	u_int8_t total_cores :4;
	u_int8_t revision :4;
};

struct bcma_eeprom_coreaddress {
	u_int8_t  type;
	u_int8_t  port;
	u_int32_t address;
	u_int32_t size;
	LIST_ENTRY(bcma_eeprom_coreaddress) next_address;
};

struct bcma_eeprom_coreinfo {
	LIST_ENTRY(bcma_eeprom_coreinfo) next_core;
	u_int16_t manuf;
	u_int16_t id;
	u_int8_t class;
	u_int8_t revision;
	u_int8_t num_mports; //master
	u_int8_t num_sports; //slave
	u_int8_t num_mwraps; //master
	u_int8_t num_swraps; //slave
	LIST_HEAD(bcma_eeprom_address_head, bcma_eeprom_coreaddress) addresses;
	struct resource_list resources;
};

struct bcma_softc {
	device_t bcma_dev;
	struct resource* mem;
	struct resource* erom_mem;
	int mem_rid;
	int erom_rid;
	struct bcma_chipinfo chip;
	LIST_HEAD(bcma_eeprom_info_head, bcma_eeprom_coreinfo) cores;
};

struct bcma_chipcommon_capabilities{
	u_int8_t num_uarts:BCMA_CC_CAP_NUM_UART_BASE;
	u_int8_t is_bigend:BCMA_CC_CAP_BIG_ENDIAN_BASE;
	u_int8_t uart_clock:BCMA_CC_CAP_UART_CLOCK_BASE;
	u_int8_t uart_gpio:BCMA_CC_CAP_UART_GPIO_BASE;
	u_int8_t external_buses:BCMA_CC_CAP_EXTERNAL_BUSES_BASE;
	u_int8_t flash_type:BCMA_CC_CAP_FLASH_TYPE_BASE;
	u_int8_t pll_type:BCMA_CC_CAP_PLL_TYPE_BASE;
	u_int8_t power_control:BCMA_CC_CAP_POWER_CONTROL_BASE;
	u_int8_t otp_size:BCMA_CC_CAP_OTP_SIZE_BASE;
	u_int8_t jtag_master:BCMA_CC_CAP_JTAG_MASTER_BASE;
	u_int8_t boot_rom:BCMA_CC_CAP_BOOT_ROM_BASE;
	u_int8_t is_64bit:BCMA_CC_CAP_64BIT_BASE;
	u_int8_t pmu:BCMA_CC_CAP_PMU_BASE;
	u_int8_t eci:BCMA_CC_CAP_ECI_BASE;
	u_int8_t sprom:BCMA_CC_CAP_SPROM_BASE;
};

struct bcma_chipcommon_softc {
	u_int8_t revision;
	struct resource* mem, *flash_mem;
	int mem_rid, flash_mem_rid;
	struct bcma_chipcommon_capabilities capabilities;
};

struct bcma_pcie_softc {
	struct resource* mem;
	int mem_rid;
};
/**
 * Private methods of BCMA
 */
void bcma_release_resources(struct bcma_softc* sc);
void bcma_scan_eeprom(struct bcma_softc* sc);
int bcma_init_scan_eeprom(struct bcma_softc* sc, u_int32_t offset, u_int32_t size, int resource_id);
