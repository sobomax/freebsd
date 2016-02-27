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

//Prototypes
static int bcma_attach(device_t);
static int bcma_probe(device_t);
static device_t bcma_add_child(device_t bus, u_int order, const char *name, int unit);
static struct resource_list* bcma_get_resource_list(device_t dev, device_t child);

//Implementations
static device_t bcma_add_child(device_t bus, u_int order, const char *name, int unit){
	struct bcma_eeprom_coreinfo *core;
	if((core = malloc(sizeof(struct bcma_eeprom_coreinfo), M_BCMA, M_WAITOK))==NULL){
		return NULL;
	}
	LIST_INIT(&(core->addresses));
	resource_list_init(&core->resources);

	device_t child = device_add_child_ordered(bus, order, name, unit);
	if(child != NULL){
		device_set_ivars(child,core);
	}else{
		free(core, M_BCMA);
	}
	return(child);
}

static struct resource_list* bcma_get_resource_list(device_t dev, device_t child){
	struct bcma_eeprom_coreinfo* core = (struct bcma_eeprom_coreinfo*)device_get_ivars(child);
	return &(core->resources);
}

static int bcma_attach(device_t dev){
	BCMA_DEBUG(("Attaching BCMA bus..."))
	struct bcma_softc* sc = device_get_softc(dev);

	sc->bcma_dev = dev;
	int mem_rid = BCMA_MEM_RID;
	sc->mem = bus_alloc_resource_any(dev, SYS_RES_MEMORY, &mem_rid, RF_ACTIVE);
	sc->mem_rid = mem_rid;
	if (sc->mem == NULL) {
		device_printf(dev, "unable to allocate probe aperture\n");
		return (ENXIO);
	}

	// Get EEPROM MMIO area from ChipCommon registers
	int res;
	if((res = bcma_init_scan_eeprom(sc, BCMA_EROM_OFFSET, BCMA_CORE_SIZE, BCMA_EROM_RID)) < 0){
		bcma_release_resources(sc);
		return res;
	}

	// Let's scan for devices now
	bcma_scan_eeprom(sc);

	// Release EROM & ChipCommon memory
	bcma_release_resources(sc);

	// Create found devices
	struct bcma_eeprom_coreinfo *core;
	LIST_FOREACH(core, &(sc->cores), next_core){
		device_t child = device_add_child(dev,NULL,-1);
		device_set_ivars(child, core);
	}

	BCMA_DEBUG(("Attaching BCMA devices..."))
	res = bus_generic_attach(dev);

#if !defined(NO_STOP_IN_BCMA)
	kdb_enter("STOP", "stop in bcma");
#endif
	return res;
}

static int bcma_probe(device_t dev){
	BCMA_DEBUG(("Probe BCMA bus..."))
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
	bus_space_tag_t main_mem_tag = rman_get_bustag(sc->mem);
	bus_space_handle_t main_mem_hndl = rman_get_bushandle(sc->mem);

	/*
	 * Read chip information
	 */
	u_int32_t chipinfo = bus_space_read_4(main_mem_tag, main_mem_hndl, BCMA_CHIPINFO_OFFSET);
	memcpy(&(sc->chip),&chipinfo, sizeof(u_int32_t)); // = (struct bcma_chipinfo)chipinfo;

	device_printf(dev, "Found chip with id 0x%04X, rev 0x%02X and package 0x%02X\n",
				(sc->chip).id, (sc->chip).revision, (sc->chip).package);

	bcma_release_resources(sc);

	return (BUS_PROBE_DEFAULT);
}

static device_method_t bcma_methods[] = {
	/* Device interface */
	DEVMETHOD(device_attach,	bcma_attach),
	DEVMETHOD(device_detach,	bus_generic_detach),
	DEVMETHOD(device_probe,		bcma_probe),
	DEVMETHOD(device_resume,	bus_generic_resume),
	DEVMETHOD(device_shutdown,	bus_generic_shutdown),
	DEVMETHOD(device_suspend,	bus_generic_suspend),

	/* Bus interface */
	DEVMETHOD(bus_add_child,		bcma_add_child),
	DEVMETHOD(bus_print_child,		bus_generic_print_child),
	// DEVMETHOD(bus_probe_nomatch,	bus_generic_probe),
	DEVMETHOD(bus_read_ivar,		bus_generic_read_ivar),
	DEVMETHOD(bus_write_ivar,		bus_generic_write_ivar),
	DEVMETHOD(bus_setup_intr,		bus_generic_setup_intr),
	DEVMETHOD(bus_teardown_intr,	bus_generic_teardown_intr),

	//RESOURCE CUSTOM
	DEVMETHOD(bus_get_resource_list,	bcma_get_resource_list),
	//RESOURCE GENERIC
	DEVMETHOD(bus_activate_resource, 	bus_generic_activate_resource),
	DEVMETHOD(bus_deactivate_resource, 	bus_generic_deactivate_resource),
	DEVMETHOD(bus_adjust_resource,		bus_generic_adjust_resource),
	DEVMETHOD(bus_alloc_resource,		bus_generic_rl_alloc_resource),
	DEVMETHOD(bus_get_resource,			bus_generic_rl_get_resource),
	DEVMETHOD(bus_release_resource, 	bus_generic_rl_release_resource),
	DEVMETHOD(bus_set_resource,			bus_generic_rl_set_resource),

	DEVMETHOD_END
};

static driver_t bcma_driver = {
	"bcma",
	bcma_methods,
	sizeof(struct bcma_softc),
};
static devclass_t bcma_devclass;

EARLY_DRIVER_MODULE(bcma, nexus, bcma_driver, bcma_devclass, 0, 0,
    BUS_PASS_BUS);
