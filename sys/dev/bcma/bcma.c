/*
 * bcma.c
 *
 *  Created on: Jan 24, 2016
 *      Author: mizhka
 */

#include "opt_ddb.h"

#include <sys/cdefs.h>
__FBSDID("$FreeBSD$");

#include <dev/bcma/bcma_reg.h>
#include <dev/bcma/bcma_var.h>

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
	DEVMETHOD(bus_probe_nomatch,	bus_generic_probe),
	DEVMETHOD(bus_read_ivar,		bus_generic_read_ivar),
	DEVMETHOD(bus_setup_intr,		bus_generic_setup_intr),
	DEVMETHOD(bus_teardown_intr,	bus_generic_teardown_intr),
	DEVMETHOD(bus_write_ivar,		bus_generic_write_ivar),

	DEVMETHOD_END
};

static driver_t bcma_driver = {
	"bcma",
	bcma_methods,
	sizeof(struct bcma_softc),
};
static devclass_t bcma_devclass;

DRIVER_MODULE(bcma, nexus, bcma_driver, bcma_devclass, 0, 0);
