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
#include <sys/errno.h>

#include <machine/bus.h>
#include <machine/resource.h>

#include <dev/bhnd/bhnd.h>

#include "bhndb_if.h"
#include "bcmavar.h"
#include "bcma_eromreg.h"

#define BCMA_NEXUS_EROM_RID	10

static int bcma_nexus_probe(device_t dev){
	const struct bhnd_chipid *cid = BHNDB_GET_CHIPID(device_get_parent(dev), dev);

	/* Check bus type */
	if (cid->chip_type != BHND_CHIPTYPE_BCMA)
		return (ENXIO);

	/* Delegate to default probe implementation */
	return (bcma_probe(dev));
}

static int bcma_nexus_attach(device_t dev){
	const struct bhnd_chipid *cid = BHNDB_GET_CHIPID(device_get_parent(dev), dev);

  	int erom_rid = BCMA_NEXUS_EROM_RID;
 	int	error = bus_set_resource(dev, SYS_RES_MEMORY, erom_rid, cid->enum_addr, BCMA_EROM_TABLE_SIZE);
 	if (error != 0){
 		BHND_ERROR_DEV(dev, ("failed to set EROM resource"));
 		return (error);
 	}

 	/* Map the EROM resource and enumerate our children. */
 	BHND_DEBUG_DEV(dev, ("erom enum address: %x", (uint32_t)cid->enum_addr));
 	struct resource* erom_res = bus_alloc_resource_any(dev, SYS_RES_MEMORY, &erom_rid, RF_ACTIVE);
 	if (erom_res == NULL) {
 		device_printf(dev, "failed to allocate EROM resource\n");
 		return (ENXIO);
 	}

 	BHND_DEBUG_DEV(dev, ("erom scanning start address: %p", rman_get_virtual(erom_res)));
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
	DEVMETHOD_END
};

DEFINE_CLASS_1(bhnd, bcma_nexus_driver, bcma_nexus_methods, sizeof(struct bcma_softc), bcma_driver);
DRIVER_MODULE(bcma_nexus, bhnd_soc, bcma_nexus_driver, bhnd_devclass, NULL, NULL);

MODULE_VERSION(bcma_nexus, 1);
MODULE_DEPEND(bcma_nexus, bcma, 1, 1, 1);
MODULE_DEPEND(bcma_nexus, bhnd_soc, 1, 1, 1);
