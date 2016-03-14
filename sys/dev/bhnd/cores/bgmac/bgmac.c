/*
 * bgmac.c
 *
 * Broadcom Gigabit MAC
 *
 * On Asus RT-N16 GMAC core attaches BCM53115 chip to SoC via GMII. So this driver is
 * MII-based. Information about registers are taken from:
 *      http://bcm-v4.sipsolutions.net/mac-gbit/Registers
 *
 *  Created on: Mar 13, 2016
 *      Author: mizhka
 */

#include <sys/cdefs.h>
#include <sys/types.h>
#include <sys/param.h>
#include <sys/kernel.h>
#include <sys/module.h>
#include <sys/bus.h>
#include <sys/rman.h>

#include <dev/mii/mii.h>
#include <dev/mii/miivar.h>
#include "miidevs.h"
#include <dev/mii/brgphyreg.h>

#include <machine/bus.h>
#include <machine/resource.h>

#include <dev/bhnd/bhnd.h>
#include <dev/bhnd/bhnd_ids.h>

#include "bgmacvar.h"
#include "bgmacreg.h"

static const struct resource_spec bgmac_rspec[BGMAC_MAX_RSPEC] = {
	{ SYS_RES_MEMORY,	0,	RF_ACTIVE },
	{ -1, -1, 0 }
};

static struct bhnd_core_id bgmac_ids[] = {
		{BHND_MFGID_BCM, BHND_COREID_GMAC},
		{-1,-1}
};

/**
 * Prototypes
 */

static int bgmac_probe(device_t dev);
static int bgmac_attach(device_t dev);

static int bgmac_readreg(device_t dev,int phy,int reg);
static int bgmac_writereg(device_t dev,int phy,int reg, int val);

/**
 * Implementation
 */

static int bgmac_probe(device_t dev){
	uint32_t mfg = bhnd_get_vendor(dev);
	uint32_t devid = bhnd_get_device(dev);

	for( struct bhnd_core_id* testid = bgmac_ids; testid->mfg != -1; testid++ ){
		if(mfg == testid->mfg && devid == testid->devid){
			bhnd_set_generic_core_desc(dev);
			return (BUS_PROBE_DEFAULT);
		}
	}
	return (ENXIO);
}

static int bgmac_attach(device_t dev){

	struct bgmac_softc* sc = device_get_softc(dev);
	sc->dev = dev;

	/* Allocate bus resources */
	memcpy(sc->rspec, bgmac_rspec, sizeof(sc->rspec));
	int error = bhnd_alloc_resources(dev, sc->rspec, sc->res);
	if(error)
		return (error);

	struct resource* res = sc->res[0]->res;

	if(!res){
		return (ENXIO);
	}

	sc->hdl = rman_get_bushandle(res);
	sc->tag = rman_get_bustag(res);

	return 0;
}

enum{
	PHY_READ,
	PHY_WRITE
} phymode;

#define BGMAC_TIMEOUT 1000

int bgmac_poll_reg(device_t dev,uint8_t reg, uint32_t mask){
	struct bgmac_softc* sc = device_get_softc(dev);
	/* Poll for the register to complete. */
	for (int i = 0; i < BGMAC_TIMEOUT; i++) {
		DELAY(10);
		uint32_t val = bus_space_read_4(sc->hdl, sc->tag, reg);
		if ((val & mask) == 0) {
			DELAY(5);
			return 0;
		}
	}
	return -1;
}

int bgmac_phyreg(device_t dev, phymode mode, int phy,int reg,int* val){
	struct bgmac_softc* sc = device_get_softc(dev);

	//Set address on PHY control register
	uint32_t tmp = bus_space_read_4(sc->hdl,sc->tag,BGMAC_REG_PHY_CONTROL);
	tmp = (tmp & (~BGMAC_REG_PHY_ACCESS_ADDR)) | phy;
	bus_space_write_4(sc->hdl,sc->tag,BGMAC_REG_PHY_CONTROL);

	//Send header (first 16 bytes) over MII
	tmp = BGMAC_REG_PHY_ACCESS_START;
	if(mode == PHY_WRITE){
		tmp |= BGMAC_REG_PHY_ACCESS_WRITE;
		tmp |= (*val & BGMAC_REG_PHY_ACCESS_DATA);
	}
	tmp |= phy << BGMAC_REG_PHY_ACCESS_ADDR_SHIFT;
	tmp |= reg << BGMAC_REG_PHY_ACCESS_REG_SHIFT;

	bus_space_write_4(sc->hdl,sc->tag,BGMAC_REG_PHY_ACCESS);

	//Wait while operation is finished
	if(bgmac_poll_reg(dev, BGMAC_REG_PHY_ACCESS, BGMAC_REG_PHY_ACCESS_START)){
		return -1;
	}

	if(mode == PHY_READ){
		//Read rest of 16 bytes back
		tmp = bus_space_read_4(sc->hdl,sc->tag,BGMAC_REG_PHY_ACCESS);
		tmp &= BGMAC_REG_PHY_ACCESS_DATA;
		*val = tmp;
	}
	return 0;
}

static int bgmac_readreg(device_t dev,int phy,int reg){
	int tmp;
	if(bgmac_phyreg(dev, PHY_READ,phy,reg,&tmp)){
		return -1;
	}
	return tmp;
}

static int bgmac_writereg(device_t dev,int phy,int reg, int val){
	int tmp = val;
	if(bgmac_phyreg(dev, PHY_WRITE,phy,reg,&tmp)){
		return -1;
	}
	return 0;
}


/**
 * Driver metadata
 */

static device_method_t bgmac_methods[] = {
		DEVMETHOD(device_probe,	 bgmac_probe),
		DEVMETHOD(device_attach, bgmac_attach),
		/** miibus interface **/
		DEVMETHOD(miibus_readreg, 	bgmac_readreg),
		DEVMETHOD(miibus_writereg, 	bgmac_writereg),

		DEVMETHOD_END
};

devclass_t bhnd_bgmac_devclass;

DEFINE_CLASS_0(bhnd_bgmac, bgmac_driver, bgmac_methods, sizeof(struct bgmac_softc));
DRIVER_MODULE(bhnd_bgmac, bhnd, bgmac_driver, bhnd_bgmac_devclass, 0, 0);
MODULE_VERSION(bhnd_bgmac, 1);
