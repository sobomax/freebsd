/*
 * soc.c
 *
 *  Created on: Feb 16, 2016
 *      Author: mizhka
 */

#include<sys/param.h>
#include<sys/kernel.h>
#include<sys/bus.h>
#include<sys/module.h>

#include<errno.h>
#include<dev/bhnd/bhndb/bhndb.h>

static struct bhnd_soc_softc{
	device_t dev;
	device_t bridge;
	int bustype;
};

static int bhnd_soc_probe(device_t dev){
	return BUS_PROBE_GENERIC;
}

static int bhnd_soc_attach(device_t dev){
	struct bhnd_soc_softc* sc = device_get_softc(dev);
	sc->dev = dev;
	device_printf(dev, "devclass = 0x%x", (void*)bhndb_devclass);
	int err = bhndb_attach_bridge(dev, &sc->bridge, 0);
	if(err != 0){
		perror("can't attach bridge");
		return err;
	}

	return 0;
}

static struct method_t bhnd_soc_methods[] = {
		DEVMETHOD(device_probe,		bhnd_soc_probe),
		DEVMETHOD(device_attach,	bhnd_soc_attach),
		DEVMETHOD_END
};

devclass_t bhnd_soc_devclass;

DEFINE_CLASS_0(bhnd_soc_driver, bhnd_soc_driver, bhnd_soc_methods, sizeof(struct bhnd_soc_softc));
DRIVER_MODULE(bhnd_soc, nexus, bhnd_soc_driver, bhnd_soc_devclass, NULL, NULL);
