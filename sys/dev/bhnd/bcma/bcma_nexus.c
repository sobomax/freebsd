/*
 * bcma_nexus.c
 *
 *  Created on: Feb 25, 2016
 *      Author: mizhka
 */

#include <sys/cdefs.h>
__FBSDID("$FreeBSD$");

#include <sys/param.h>
#include <sys/kernel.h>
#include <sys/bus.h>
#include <sys/module.h>

static int bcma_nexus_probe(device_t dev){
	return BUS_PROBE_DEFAULT;
}

static int bcma_nexus_attach(device_t dev){
	int	error, rid = 0;
 	struct resource* mem = bus_alloc_resource_any(dev, SYS_RES_MEMORY, &rid, RF_ACTIVE);

 	if(mem == NULL){
 		device_printf(dev, "failed to allocate mem resource\n");
 		return (error);
 	}

 	bus_space_tag_t tag = rman_get_bustag(mem);
 	bus_space_handle_t hndl = rman_get_bushandle(mem);
 	u_int32_t offset = bus_space_read_4(tag, hndl, 0xFC);
  	bus_release_resource(dev, SYS_RES_MEMORY, rid, mem);

 	int erom_rid = 10;
 	error = bus_set_resource(dev, SYS_RES_MEMORY, erom_rid, offset, BCMA_EROM_TABLE_SIZE);

 	if (error != 0){
 		//perror("[bcma] bus_set_resource returned:");
 		device_printf(dev, "failed to set EROM resource\n");
 		return (error);
 	}
 	/* Map the EROM resource and enumerate our children. */

 	struct resource* erom_res = bus_alloc_resource_any(dev, SYS_RES_MEMORY, &erom_rid, RF_ACTIVE);
 	if (erom_res == NULL) {
 		device_printf(dev, "failed to allocate EROM resource\n");
 		return (ENXIO);
 	}

 	error = bcma_add_children(dev, erom_res, BCMA_EROM_TABLE_START);

 	/* Clean up */
 	bus_release_resource(dev, SYS_RES_MEMORY, erom_rid, erom_res);
 	if (error)
 		return (error);

 	/* Call our superclass' implementation */
 	return (bcma_attach(dev));
}

static device_method_t bcma_nexus_methods[] = {
	/* Device interface */
	DEVMETHOD(device_probe,			bcma_nexus_probe),
	DEVMETHOD(device_attach,		bcma_nexus_attach),

	/* Bus interface */
//	DEVMETHOD(bus_suspend_child,	bcma_nexus_suspend_child),
//	DEVMETHOD(bus_resume_child,		bcma_nexus_resume_child),

	DEVMETHOD_END
};

DEFINE_CLASS_1(bhnd, bcma_nexus_driver, bcma_nexus_methods,
    sizeof(struct bcma_softc), bcma_driver);

DRIVER_MODULE(bcma_nexus, nexus, bcma_nexus_driver, bhnd_devclass, NULL, NULL);

MODULE_VERSION(bcma_nexus, 1);
MODULE_DEPEND(bcma_nexus, bcma, 1, 1, 1);
MODULE_DEPEND(bcma_nexus, bhnd_soc, 1, 1, 1);

