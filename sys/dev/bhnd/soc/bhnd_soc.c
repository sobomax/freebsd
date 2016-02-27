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

#include<sys/errno.h>

#include<machine/resource.h>
#include<dev/bhnd/bhndb/bhndb.h>
#include<dev/bhnd/bhnd_types.h>

struct bhnd_soc_softc {
	device_t dev;
	device_t bridge;
};

static const struct bhndb_hwcfg * bhnd_soc_get_generic_hwcfg(device_t dev, device_t child);
//static struct bhndb_hw * bhnd_soc_get_bhndb_hwtable(device_t dev, device_t child);
//static bool bhnd_soc_is_core_disabled(device_t dev, device_t child, struct bhnd_core_info *core);

static int bhnd_soc_probe(device_t dev){
	device_set_desc(dev, "Broadcom SOC for HND");
	return BUS_PROBE_GENERIC;
}

static int bhnd_soc_attach(device_t dev){
	struct bhnd_soc_softc* sc = device_get_softc(dev);
	sc->dev = dev;
	BUS_PRINT_CHILD(device_get_parent(dev), dev);

	int err = bhndb_attach_bridge(dev, &sc->bridge, 0);
	if(err != 0){
		device_printf(dev,"can't attach bridge: %d\n", err);
		return err;
	}

	return 0;
}

//us
#define BHND_SOC_CCREGS_OFFSET	0x18000000
#define	BHND_SOC_CCREGS_SIZE	0x1000

#define BHND_SOC_EROMREGS_OFFSET 0x1810e000
#define BHND_SOC_EROMREGS_SIZE	 0x1000

const struct bhndb_hwcfg bhnd_soc_bcma_generic_hwcfg = {
	.resource_specs		= (const struct resource_spec[]) {
		{ SYS_RES_MEMORY,	0,	RF_ACTIVE },
		{ -1,			0,		0 }
	},

	.register_windows	= (const struct bhndb_regwin[]) {
		/* 0x18000000: chipc core registers */
		{
			.win_type	= BHNDB_REGWIN_T_CORE,
			.win_offset	= BHND_SOC_CCREGS_OFFSET,
			.win_size	= BHND_SOC_CCREGS_SIZE,
			.res		= {
					.type = SYS_RES_MEMORY,
					.rid  = 0
			},
			.win_spec.core 		= {
					.class	= BHND_DEVCLASS_CC,
					.unit	= 0,
					.port	= 0,
					.region	= 0,
					.port_type = BHND_PORT_DEVICE
			}
		},
		BHNDB_REGWIN_TABLE_END
	},
};

static const struct bhndb_hwcfg * bhnd_soc_get_generic_hwcfg(device_t dev, device_t child){
	return &bhnd_soc_bcma_generic_hwcfg;
}

static device_method_t bhnd_soc_methods[] = {
		DEVMETHOD(device_probe,		bhnd_soc_probe),
		DEVMETHOD(device_attach,	bhnd_soc_attach),
		DEVMETHOD(bhndb_bus_get_generic_hwcfg,bhnd_soc_get_generic_hwcfg	),
		DEVMETHOD_END
};

devclass_t bhnd_soc_devclass;

DEFINE_CLASS_0(bhnd_soc, bhnd_soc_driver, bhnd_soc_methods, sizeof(struct bhnd_soc_softc));
DRIVER_MODULE(bhnd_soc, nexus, bhnd_soc_driver, bhnd_soc_devclass, NULL, NULL);
