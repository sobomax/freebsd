/*
 * soc.c
 *
 *  Created on: Feb 16, 2016
 *      Author: mizhka
 */

#include <sys/param.h>
#include <sys/kernel.h>
#include <sys/bus.h>
#include <sys/module.h>

#include <sys/errno.h>

#include <machine/resource.h>

#include <dev/bhnd/bhnd_types.h>
#include <dev/bhnd/bhndb/bhndb.h>
#include <dev/bhnd/soc/bhnd_soc.h>

/*
 * **************************** VARIABLES ****************************
 */

extern const struct bhndb_hw bhnd_soc_generic_hw_table[];
extern const struct bhndb_hwcfg bhnd_soc_bcma_generic_hwcfg;

int attachment_mode = SOC_TO_BRIDGE;

/*
 * **************************** PROTOTYPES ****************************
 */
//internal methods
static int 	bhnd_soc_attach_bridge(device_t dev, struct bhnd_soc_softc* sc);
static int 	bhnd_soc_attach_bus(device_t dev, struct bhnd_soc_softc* sc);
//device methods
static int 	bhnd_soc_probe(device_t dev);
static int 	bhnd_soc_attach(device_t dev);
//bhndb methods
static const struct bhndb_hwcfg * 	bhnd_soc_get_generic_hwcfg(device_t dev, device_t child);
static const struct bhndb_hw * 			bhnd_soc_get_bhndb_hwtable(device_t dev, device_t child);
static bool bhnd_soc_is_core_disabled(device_t dev, device_t child, struct bhnd_core_info *core);
//TODO:


/*
 * **************************** IMPLEMENTATION ****************************
 */

static int bhnd_soc_attach_bridge(device_t dev, struct bhnd_soc_softc* sc){
	int err = bhndb_attach_bridge(dev, &sc->bridge, 0);
	if(err != 0){
		device_printf(dev,"can't attach bridge: %d\n", err);
		return err;
	}
	return 0;
}

static int bhnd_soc_attach_bus(device_t dev, struct bhnd_soc_softc* sc){
	return bhndb_attach_by_class(dev, &(sc->bus), -1, bhnd_devclass);
}

static int bhnd_soc_probe(device_t dev){
	device_set_desc(dev, "Broadcom SOC for HND");
	return BUS_PROBE_GENERIC;
}

static int bhnd_soc_attach(device_t dev){
	struct bhnd_soc_softc* sc = device_get_softc(dev);
	sc->dev = dev;
	BUS_PRINT_CHILD(device_get_parent(dev), dev);

	return (attachment_mode == SOC_TO_BRIDGE) ?
		bhnd_soc_attach_bridge(dev,sc) :
		bhnd_soc_attach_bus(dev,sc);
}

static const struct bhndb_hw *		bhnd_soc_get_bhndb_hwtable(device_t dev, device_t child){
	return bhnd_soc_generic_hw_table;
}

static const struct bhndb_hwcfg *	bhnd_soc_get_generic_hwcfg(device_t dev, device_t child){
	return &bhnd_soc_bcma_generic_hwcfg;
}

static bool bhnd_soc_is_core_disabled(device_t dev, device_t child, struct bhnd_core_info *core){
	return false;
}

/*
 * **************************** DRIVER METADATA ****************************
 */

static device_method_t bhnd_soc_methods[] = {
		//device interface
		DEVMETHOD(device_probe,		bhnd_soc_probe),
		DEVMETHOD(device_attach,	bhnd_soc_attach),
		//bhndb_bus interface
		DEVMETHOD(bhndb_bus_get_generic_hwcfg , bhnd_soc_get_generic_hwcfg),
		DEVMETHOD(bhndb_bus_get_hardware_table, bhnd_soc_get_bhndb_hwtable),
		DEVMETHOD(bhndb_bus_is_core_disabled,	bhnd_soc_is_core_disabled),
		DEVMETHOD_END
};

devclass_t bhnd_soc_devclass;

DEFINE_CLASS_0(bhnd_soc, bhnd_soc_driver, bhnd_soc_methods, sizeof(struct bhnd_soc_softc));
DRIVER_MODULE(bhnd_soc, nexus, bhnd_soc_driver, bhnd_soc_devclass, NULL, NULL);
