/*
 * bcma_chipcommon.c
 *
 *  Created on: Jan 29, 2016
 *      Author: mizhka
 */

#include <sys/cdefs.h>
__FBSDID("$FreeBSD$");
#include <sys/types.h>
#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/bus.h>
#include <sys/errno.h>
#include <sys/module.h>

#include <machine/bus.h>

#define BCMA_LOGGING	BCMA_TRACE_LEVEL
#include <dev/bcma/bcma_debug.h>
#include <dev/bcma/bcma_var.h>
#include <dev/bcma/bcma_ids.h>
#include <dev/bcma/bcma_chipcommon.h>

static int bcma_chipcommon_probe(device_t dev);
static int bcma_chipcommon_attach(device_t dev);

// Internal API
void bcma_chipcommon_parse_capabilities(struct bcma_chipcommon_capabilities* capabilities, u_int32_t current);

static int bcma_chipcommon_probe(device_t dev){
	struct bcma_eeprom_coreinfo *ptr = device_get_ivars(dev);
	BCMA_TRACE(("Starting probing..."));
	BCMA_TRACE(("Probe for device: %s", bcma_ids_lookup(ptr->id, bcma_core_ids)));
	if(ptr->id == BCMA_CORE_CHIPCOMMON){
		BCMA_DEBUG(("ChipCommon found with rev 0x%02x", ptr->revision));
		return BUS_PROBE_GENERIC;
	}
	return ENXIO;
}

void bcma_chipcommon_parse_capabilities(struct bcma_chipcommon_capabilities* capabilities, u_int32_t current){
	capabilities->num_uarts = BCMA_BITS(current, BCMA_CC_CAP_NUM_UART);
	capabilities->is_bigend = BCMA_BITS(current, BCMA_CC_CAP_BIG_ENDIAN);
	capabilities->uart_gpio = BCMA_BITS(current, BCMA_CC_CAP_UART_GPIO);
	capabilities->uart_clock= BCMA_BITS(current, BCMA_CC_CAP_UART_CLOCK);
	capabilities->flash_type= BCMA_BITS(current, BCMA_CC_CAP_FLASH_TYPE);

	capabilities->external_buses = BCMA_BITS(current, BCMA_CC_CAP_EXTERNAL_BUSES);
	capabilities->power_control = BCMA_BITS(current, BCMA_CC_CAP_POWER_CONTROL);
	capabilities->jtag_master = BCMA_BITS(current, BCMA_CC_CAP_JTAG_MASTER);

	capabilities->pll_type = BCMA_BITS(current, BCMA_CC_CAP_PLL_TYPE);
	capabilities->otp_size = BCMA_BITS(current, BCMA_CC_CAP_OTP_SIZE);
	capabilities->is_64bit = BCMA_BITS(current, BCMA_CC_CAP_64BIT);
	capabilities->boot_rom = BCMA_BITS(current, BCMA_CC_CAP_BOOT_ROM);
	capabilities->pmu = BCMA_BITS(current, BCMA_CC_CAP_PMU);
	capabilities->eci = BCMA_BITS(current, BCMA_CC_CAP_ECI);
	capabilities->sprom = BCMA_BITS(current, BCMA_CC_CAP_SPROM);

	BCMA_DEBUG(("UARTs: 0x%01x", capabilities->num_uarts))
	BCMA_DEBUG(("BigEngian: 0x%01x", capabilities->is_bigend))
	BCMA_DEBUG(("UART-GPIO: 0x%01x", capabilities->uart_gpio))
	BCMA_DEBUG(("UART Clock: 0x%01x", capabilities->uart_clock))
	BCMA_DEBUG(("Flash type: %s", bcma_ids_lookup(capabilities->flash_type, bcma_chipcommon_flash_types)));

	BCMA_DEBUG(("External buses: %s",  bcma_ids_lookup(capabilities->external_buses, bcma_chipcommon_external_buses)));
	BCMA_DEBUG(("Power control: 0x%01x", capabilities->power_control))
	BCMA_DEBUG(("JTAG master: 0x%01x", capabilities->jtag_master))

	BCMA_DEBUG(("PLL Type: %s", bcma_ids_lookup(capabilities->pll_type, bcma_chipcommon_pll_types)));
	BCMA_DEBUG(("OTP size: 0x%01x", capabilities->otp_size))
	BCMA_DEBUG(("Is 64bit? 0x%01x", capabilities->is_64bit))
	BCMA_DEBUG(("Boot ROM: 0x%01x", capabilities->boot_rom))
	BCMA_DEBUG(("PMU: 0x%01x", capabilities->pmu))
	BCMA_DEBUG(("ECI: 0x%01x", capabilities->eci))
	BCMA_DEBUG(("SPROM: 0x%01x", capabilities->sprom))
}

static int bcma_chipcommon_attach(device_t dev){
	struct bcma_eeprom_coreinfo *coreinfo = device_get_ivars(dev);
	struct bcma_chipcommon_softc *sc = device_get_softc(dev);

	sc->mem_rid = BCMA_CHIPCOMMON_MEM_RID;
	sc->mem = bus_alloc_resource_any(dev, SYS_RES_MEMORY, &(sc->mem_rid) , RF_ACTIVE);
	if(sc->mem == NULL){
		BCMA_WARN(("can't allocate main memory for ChipCommon"));
		return ENXIO;
	}

	//Just copy revision from core info structure
	sc->revision = coreinfo->revision;

	bus_space_tag_t mem_tag = rman_get_bustag(sc->mem);
	bus_space_handle_t mem_handle = rman_get_bushandle(sc->mem);

	u_int32_t current = bus_space_read_4(mem_tag, mem_handle, BCMA_CC_CAP);
	bcma_chipcommon_parse_capabilities( &(sc->capabilities),current);

	//PMU - TODO

	if(sc->capabilities.flash_type == BCMA_CC_CAP_FLASH_TYPE_PARALLEL){
		sc->flash_mem_rid = BCMA_CHIPCOMMON_FLASH;
		sc->flash_mem = bus_alloc_resource_any(dev, SYS_RES_MEMORY, &(sc->flash_mem_rid), RF_ACTIVE);
		if(sc->flash_mem == NULL){
			BCMA_WARN(("can't allocate flash memory on ChipCommon"))
			return ENXIO;
		}
	}
	//lookup for Capabilities, Flash, Power management unit (aka PMU),
	return 0;
}

static device_method_t bcma_chipcommon_methods[] = {
	/* Device interface */
	DEVMETHOD(device_attach,	bcma_chipcommon_attach),
	DEVMETHOD(device_detach,	bus_generic_detach),
	DEVMETHOD(device_probe,		bcma_chipcommon_probe),

	DEVMETHOD_END
};

static driver_t bcma_chipcommon_driver = {
	"bcma_chipcommon",
	bcma_chipcommon_methods,
	sizeof(struct bcma_chipcommon_softc),
};

static devclass_t bcma_chipcommon_devclass;

DRIVER_MODULE(bcma_chipcommon, bcma, bcma_chipcommon_driver, bcma_chipcommon_devclass, 0, 0);
