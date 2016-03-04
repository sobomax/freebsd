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

struct bhndb_nexus_softc {
	struct bhndb_softc	bhndb;
};

static int bhndb_nexus_attach(device_t dev){
	int error;
	if ((error = bhndb_attach(dev, BHND_DEVCLASS_SOC_BRIDGE)))
		return (error);

	/* Success */
	return (0);
}

static device_method_t bhndb_nexus_methods[] = {
	DEVMETHOD(device_attach, bhndb_nexus_attach),
	DEVMETHOD_END
};

DEFINE_CLASS_1(bhndb_nexus, bhndb_nexus_driver, bhndb_nexus_methods,
		sizeof(struct bhndb_nexus_softc), bhndb_driver );

DRIVER_MODULE(bhndb_nexus, bhnd_soc, bhndb_nexus_driver, bhndb_devclass, NULL, NULL);

MODULE_VERSION(bhnd_nexus, 1);
MODULE_DEPEND(bhnd_nexus, bcma, 1, 1, 1);
