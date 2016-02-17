/*
 * bcma_nexus.c
 *
 *  Created on: Feb 11, 2016
 *      Author: mizhka
 */

/**
 * This driver is supposed to attach directly to nexus / SoC
 */

#include <sys/param.h>
#include <sys/kernel.h>
#include <sys/kobj.h>
#include <sys/bus.h>
#include <sys/errno.h>
#include <sys/rman.h>
#include <sys/module.h>

#include <machine/bus.h>
#include <machine/resource.h>

#include <dev/bhnd/bhnd_ids.h>

#include <dev/bhnd/bhndb/bhndb.h>
#include <dev/bhnd/bhndb/bhndbvar.h>

static int bhndb_nexus_attach(device_t dev){

	struct bhndb_nexus_softc		*sc;
	int				 error;

	sc = device_get_softc(dev);
	sc->dev = dev;

//	/* Find our hardware config */
//	if (bwn_pci_find_devcfg(dev, &sc->devcfg, &ident))
//		return (ENXIO);

//	/* Save quirk flags */
//	sc->quirks = ident->quirks;

	/* Attach bridge device */
	if ((error = bhndb_attach_bridge(dev, &sc->bhndb_dev, -1)))
		return (ENXIO);

	/* Success */
	return (0);

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

static device_method_t bhndb_nexus_methods[] = {
	DEVMETHOD(device_attach, bhndb_nexus_attach),
	DEVMETHOD_END
};

DEFINE_CLASS_1(bhndb, bhndb_nexus_driver, bhndb_nexus_methods,
		sizeof(struct bhndb), bhndb_driver );

DRIVER_MODULE(bhndb_nexus, nexus, bhndb_nexus_driver, bhndb_devclass, NULL, NULL);

MODULE_VERSION(bhnd_nexus, 1);
MODULE_DEPEND(bhnd_nexus, bcma, 1, 1, 1);
