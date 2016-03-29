/*
 * chipc_cfi.c
 *
 *  Created on: Feb 29, 2016
 *      Author: mizhka
 */


#include <sys/cdefs.h>
__FBSDID("$FreeBSD$");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/bus.h>
#include <sys/conf.h>
#include <sys/kernel.h>
#include <sys/malloc.h>
#include <sys/module.h>
#include <sys/rman.h>
#include <sys/sysctl.h>
#include <sys/slicer.h>

#include <machine/bus.h>

#include <dev/cfi/cfi_var.h>

#define TRX_MAGIC 	0x30524448
#define CFE_MAGIC 	0x43464531
#define NVRAM_MAGIC	0x48534C46

static int chipc_cfi_trx_slicer(device_t dev, struct flash_slice *slices, int *nslices);

static int
chipc_cfi_probe(device_t dev)
{
	struct cfi_softc *sc = device_get_softc(dev);
	sc->sc_width = 2;
	int error = cfi_probe(dev);
	return (error);
}

static int
chipc_cfi_attach(device_t dev)
{
	int error = cfi_attach(dev);
	if(error)
		return error;

	flash_register_slicer(chipc_cfi_trx_slicer);
	return 0;
}

/* Slicer operates on the TRX-based FW */
static int
chipc_cfi_trx_slicer(device_t dev, struct flash_slice *slices, int *nslices)
{
	/* Find TRX header in flash memory with step = 0x1000 (or block size (128Kb)? */

	struct cfi_softc *sc = device_get_softc(dev);
	*nslices = 0;

	device_printf(dev,"slicer: scanning memory for headers...\n");
	for(uint32_t ofs = 0; ofs < sc->sc_size; ofs+= 0x1000){
		uint32_t val = bus_space_read_4(sc->sc_tag, sc->sc_handle, ofs);
		switch(val){
		case TRX_MAGIC:
			device_printf(dev, "TRX found: %x\n", ofs);
			//read last offset of TRX header
			uint32_t fs_ofs = bus_space_read_4(sc->sc_tag, sc->sc_handle, ofs + 24);
			device_printf(dev, "FS offset: %x\n", fs_ofs);

			slices[*nslices].base = ofs + fs_ofs;
			//XXX: fully sized? any other partition?
			uint32_t fw_len = bus_space_read_4(sc->sc_tag, sc->sc_handle, ofs + 4);
			slices[*nslices].size = fw_len - fs_ofs;
			slices[*nslices].label = "rootfs";
			*nslices += 1;
			break;
		case CFE_MAGIC:
			device_printf(dev, "CFE found: %x\n", ofs);
			break;
		case NVRAM_MAGIC:
			device_printf(dev, "NVRAM found: %x\n", ofs);
			break;
		default:
			break;
		}
	}
	device_printf(dev,"slicer: done\n");

	return (0);
}

static device_method_t chipc_cfi_methods[] = {
	/* device interface */
	DEVMETHOD(device_probe,		chipc_cfi_probe),
	DEVMETHOD(device_attach,	chipc_cfi_attach),
	DEVMETHOD(device_detach,	cfi_detach),

	{0, 0}
};

static driver_t chipc_cfi_driver = {
	cfi_driver_name,
	chipc_cfi_methods,
	sizeof(struct cfi_softc),
};

DRIVER_MODULE(cfi, bhnd_chipc, chipc_cfi_driver, cfi_devclass, 0, 0);
