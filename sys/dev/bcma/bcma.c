/*
 * bcma.c
 *
 *  Created on: Jan 24, 2016
 *      Author: mizhka
 */

#include "opt_ddb.h"

#include <sys/cdefs.h>
__FBSDID("$FreeBSD$");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/module.h>
#include <sys/types.h>
#include <sys/malloc.h>
#include <sys/rman.h>
#include <sys/bus.h>
#include <sys/errno.h>
#include <sys/kdb.h>

#include <machine/bus.h>
#include <machine/resource.h>

#include <dev/bcma/bcma_debug.h>
#include <dev/bcma/bcma_reg.h>
#include <dev/bcma/bcma_var.h>

MALLOC_DEFINE(M_BCMA, "BCMA_cores", "Broadcom bus cores");

static int bcma_attach(device_t);
static int bcma_probe(device_t);

static device_method_t bcma_methods[] = {
	/* Device interface */
	DEVMETHOD(device_attach,	bcma_attach),
	DEVMETHOD(device_detach,	bus_generic_detach),
	DEVMETHOD(device_probe,		bcma_probe),
	DEVMETHOD(device_resume,	bus_generic_resume),
	DEVMETHOD(device_shutdown,	bus_generic_shutdown),
	DEVMETHOD(device_suspend,	bus_generic_suspend),

	/* Bus interface */
	DEVMETHOD(bus_activate_resource,bus_generic_activate_resource),
	DEVMETHOD(bus_add_child,		bus_generic_add_child),
	DEVMETHOD(bus_alloc_resource,	bus_generic_alloc_resource),
	DEVMETHOD(bus_get_resource_list,bus_generic_get_resource_list),
	DEVMETHOD(bus_print_child,		bus_generic_print_child),
	// DEVMETHOD(bus_probe_nomatch,	bus_generic_probe),
	DEVMETHOD(bus_read_ivar,		bus_generic_read_ivar),
	DEVMETHOD(bus_setup_intr,		bus_generic_setup_intr),
	DEVMETHOD(bus_teardown_intr,	bus_generic_teardown_intr),
	DEVMETHOD(bus_write_ivar,		bus_generic_write_ivar),

	DEVMETHOD_END
};

static int bcma_attach(device_t dev){
	bus_generic_probe(dev);
	return (bus_generic_attach(dev));
}

static int bcma_probe(device_t dev){
	struct bcma_softc* sc = device_get_softc(dev);
	sc->bcma_dev = dev;
	int mem_rid = BCMA_MEM_RID;
	struct resource* main_mem = bus_alloc_resource_any(dev, SYS_RES_MEMORY, &mem_rid, RF_ACTIVE);
	sc->mem_rid = mem_rid;

	if (main_mem == NULL) {
		device_printf(dev, "unable to allocate probe aperture\n");
		return (ENXIO);
	}

	sc->bcma_dev = dev;
	sc->mem = main_mem;
	bus_space_tag_t main_mem_tag = rman_get_bustag(main_mem);
	bus_space_handle_t main_mem_hndl = rman_get_bushandle(main_mem);

	/*
	 * Read chip information
	 */
	u_int32_t chipinfo = bus_space_read_4(main_mem_tag, main_mem_hndl, BCMA_CHIPINFO_OFFSET);
	memcpy(&(sc->chip),&chipinfo, sizeof(u_int32_t)); // = (struct bcma_chipinfo)chipinfo;

	device_printf(dev, "Found chip with id 0x%04X, rev 0x%02X and package 0x%02X\n",
				(sc->chip).id, (sc->chip).revision, (sc->chip).package);

	/*
	 * Let's scan for devices now
	 */
	//u_int32_t eromptr = bus_space_read_4(rman_get_bustag(sc->mem), rman_get_bushandle(sc->mem), BCMA_EROM_OFFSET); //0x00FC

	u_int32_t eromptr = bus_space_read_4(main_mem_tag, main_mem_hndl, BCMA_EROM_OFFSET);
	BCMA_TRACE(("EROMPRT = 0x%04X", eromptr))
	int res = bus_set_resource(dev,SYS_RES_MEMORY, BCMA_EROM_RID, eromptr, BCMA_CORE_SIZE);

	if(res != 0){
		BCMA_WARN(("can't set up resource for EROM memory"))
		bcma_release_resources(sc);
		return (ENXIO);
	}

	int erom_rid = BCMA_EROM_RID;
	struct resource* erom = bus_alloc_resource_any(dev, SYS_RES_MEMORY, &erom_rid, RF_ACTIVE);

	if(erom == NULL){
		BCMA_WARN(("can't allocate EROM resource"));
		bcma_release_resources(sc);
		return (ENXIO);
	}
	sc->erom_mem = erom;
	sc->erom_rid = erom_rid;

	bcma_scan_eeprom(sc);

	kdb_enter("STOP", "stop in bcma");

	return 0;
}

static driver_t bcma_driver = {
	"bcma",
	bcma_methods,
	sizeof(struct bcma_softc),
};
static devclass_t bcma_devclass;

DRIVER_MODULE(bcma, nexus, bcma_driver, bcma_devclass, 0, 0);
