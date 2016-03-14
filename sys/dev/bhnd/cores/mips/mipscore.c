/*
 * mipscore.c
 *
 *  Created on: Mar 9, 2016
 *      Author: mizhka
 */

#include <sys/cdefs.h>
#include <sys/param.h>
#include <sys/kernel.h>
#include <sys/bus.h>
#include <sys/module.h>
#include <sys/systm.h>
#include <sys/errno.h>
#include <sys/rman.h>
#include <sys/stddef.h>

#include <machine/bus.h>
#include <machine/resource.h>

#include <dev/bhnd/bhnd.h>
#include <dev/bhnd/bhndvar.h>
#include <dev/bhnd/bhnd_ids.h>

#include "mipscorevar.h"

static const struct resource_spec mipscore_rspec[MIPSCORE_MAX_RSPEC] = {
	{ SYS_RES_MEMORY,	0,	RF_ACTIVE },
	{ -1, -1, 0 }
};

struct bhnd_core_id mipscore_ids[] = {
		{BHND_MFGID_MIPS, BHND_COREID_MIPS},
		{BHND_MFGID_MIPS, BHND_COREID_MIPS33},
		{BHND_MFGID_MIPS, BHND_COREID_MIPS74K},
		{-1,-1}
};


static int mipscore_probe(device_t dev){
	uint32_t mfg = bhnd_get_vendor(dev);
	uint32_t devid = bhnd_get_device(dev);

	for( struct bhnd_core_id* testid = mipscore_ids; testid->mfg != -1; testid++ ){
		if(mfg == testid->mfg && devid == testid->devid){
			bhnd_set_generic_core_desc(dev);
			return (BUS_PROBE_DEFAULT);
		}
	}
	return (ENXIO);
}

static int mipscore_attach(device_t dev){

	struct mipscore_softc* sc = device_get_softc(dev);
	uint32_t devid = bhnd_get_device(dev);

	sc->devid = devid;
	sc->dev = dev;

	/* Allocate bus resources */
	memcpy(sc->rspec, mipscore_rspec, sizeof(sc->rspec));
	int error = bhnd_alloc_resources(dev, sc->rspec, sc->res);
	if(error)
		return (error);

	struct resource* res = sc->res[0]->res;

	if(!res){
		return (ENXIO);
	}

	bus_space_handle_t hdl = rman_get_bushandle(res);
	bus_space_tag_t tag = rman_get_bustag(res);

	struct mipscore_regs* regs = malloc(sizeof(struct mipscore_regs), M_BHND, M_NOWAIT);
	if(!regs){
		error = ENXIO;
		goto cleanup;
	}

	if(devid == BHND_COREID_MIPS74K){
		bus_space_read_multi_4(tag, hdl,0,(uint32_t*)regs, sizeof(struct mipscore_regs)/ sizeof(uint32_t));

		regs->intmask[5] = (1 << 31);
		/* Use intmask5 register to route the timer interrupt */
		bus_space_write_4(tag,hdl,offsetof(struct mipscore_regs, intmask[5]),regs->intmask[5]);
	}
cleanup:
	if(!regs)
		free(regs, M_BHND);
	return error;
}

static device_method_t mipscore_methods[] = {
		DEVMETHOD(device_probe, 	mipscore_probe),
		DEVMETHOD(device_attach,	mipscore_attach),
		DEVMETHOD_END
};

devclass_t bhnd_mipscore_devclass;

DEFINE_CLASS_0(bhnd_mipscore, mipscore_driver, mipscore_methods, sizeof(struct mipscore_softc));
DRIVER_MODULE(bhnd_mipscore, bhnd, mipscore_driver, bhnd_mipscore_devclass, 0, 0);
MODULE_VERSION(bhnd_mipscore, 1);
