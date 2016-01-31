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

#define BCMA_LOGGING	BCMA_TRACE_LEVEL
#include <dev/bcma/bcma_debug.h>
#include <dev/bcma/bcma_var.h>
#include <dev/bcma/bcma_ids.h>

static int bcma_chipcommon_probe(device_t dev);
static int bcma_chipcommon_attach(device_t dev);

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

static int bcma_chipcommon_attach(device_t dev){
	//struct bcma_eeprom_coreinfo *coreinfo = device_get_ivars(dev);
	//struct bcma_chipcommon_softc *sc = device_get_softc(dev);



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
