/*
 * chipc_spi.c
 *
 *  Created on: Mar 22, 2016
 *      Author: mizhka
 */

#include <sys/cdefs.h>
__FBSDID("$FreeBSD$");
#include <sys/param.h>
#include <sys/kernel.h>
#include <sys/module.h>
#include <sys/errno.h>
#include <sys/rman.h>
#include <sys/bus.h>
#include <sys/systm.h>

#include <machine/bus.h>

/*
 * Flash slicer
 */
#include <sys/slicer.h>

/*
 * SPI BUS interface
 */
#include <dev/spibus/spi.h>
#include "spibus_if.h"
#include "flash_if.h"

#include <dev/bhnd/bhndvar.h>
#include "chipcreg.h"
#include "chipcvar.h"
#include "chipc_spi.h"

/*
 * **************************** VARIABLES ****************************
 */

//static struct resource_spec chipc_spi_resource_specs[] = {
//		{SYS_RES_MEMORY, 0, RF_ACTIVE},
//		{-1, -1, -1}
//};

/*
 * **************************** PROTOTYPES ****************************
 */

static int chipc_spi_probe(device_t dev);
static int chipc_spi_attach(device_t dev);
static int chipc_spi_transfer(device_t dev, device_t child, struct spi_command *cmd);
static int chipc_spi_txrx(struct chipc_spi_softc *sc, uint8_t in, uint8_t* out);
static int chipc_spi_wait(struct chipc_spi_softc *sc);
static int
chipc_spi_trx_slicer(device_t dev, struct flash_slice *slices, int *nslices);


/*
 * **************************** IMPLEMENTATION ************************
 */

#define TRX_MAGIC 	0x30524448
#define CFE_MAGIC 	0x43464531
#define NVRAM_MAGIC	0x48534C46

/* Slicer operates on the TRX-based FW */
static int chipc_spi_trx_slicer(device_t dev, struct flash_slice *slices, int *nslices)
{
	BHND_INFO_DEV(dev, ("chipc_spi_trx_slicer!"));

	//flash(mx25l) <- spibus <- chipc_spi
	int flash_size = FLASH_GET_SIZE(dev);
	device_t spibus = device_get_parent(dev);
	device_t chipc_spi = device_get_parent(spibus);
	struct chipc_spi_softc *sc = device_get_softc(chipc_spi);
	*nslices = 0;

	device_printf(dev,"slicer: scanning memory for headers...\n");
	for(uint32_t ofs = 0; ofs < flash_size; ofs+= 0x1000){
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

static int chipc_spi_probe(device_t dev){
	device_set_desc(dev, "ChipCommon SPI");
	return (BUS_PROBE_DEFAULT); //??? BUS_PROBE_NOWILDCARD
}

static int chipc_spi_attach(device_t dev){
	device_t chipcommon = device_get_parent(dev);
	struct chipc_spi_softc* sc = device_get_softc(dev);
	struct chipc_softc* parent_sc = device_get_softc(chipcommon);

	if (!parent_sc) {
		BHND_ERROR_DEV(dev, ("no found chipcommon's softc"));
		return (ENXIO);
	}

	if(!(parent_sc->core)) {
		BHND_ERROR_DEV(dev, ("no found main BHND resource of chipcommon's softc"));
		return (ENXIO);
	}

	if(!(parent_sc->core->res)) {
		BHND_ERROR_DEV(dev, ("no found main resource of chipcommon's softc"));
		return (ENXIO);
	}

	sc->sc_mem_res = parent_sc->core->res;

	sc->sc_rid = 0;
	sc->sc_res = bus_alloc_resource_any(dev, SYS_RES_MEMORY, &sc->sc_rid,
	    RF_ACTIVE);
	if (sc->sc_res == NULL)
		return (ENXIO);

	sc->sc_tag = rman_get_bustag(sc->sc_res);
	sc->sc_handle = rman_get_bushandle(sc->sc_res);

	flash_register_slicer(chipc_spi_trx_slicer);

	device_add_child(dev, "spibus", 0);
	return (bus_generic_attach(dev));
}

static int chipc_spi_wait(struct chipc_spi_softc *sc) {
	int i = CHIPC_SPI_MAXTRIES;

	while (i--)
		if (!(SPI_READ(sc, CHIPC_FLASHCTL) & CHIPC_FLASHCTL_START))
			break;

	if (i != 0)
		return (0);

	BHND_DEBUG_DEV(sc->dev, ("busy"));
	return (-1);
}

static int chipc_spi_txrx(struct chipc_spi_softc *sc, uint8_t out, uint8_t* in) {
	uint32_t ctl = CHIPC_FLASHCTL_START | CHIPC_FLASHCTL_CSACTIVE | out;
	SPI_BARRIER_WRITE(sc);
	SPI_WRITE(sc, CHIPC_FLASHCTL, ctl);
	SPI_BARRIER_WRITE(sc);

	if (chipc_spi_wait(sc))
		return (-1);

	*in = SPI_READ(sc, CHIPC_FLASHDATA) & 0xff;
	return (0);
}

static int chipc_spi_transfer(device_t dev, device_t child, struct spi_command *cmd) {
	struct chipc_spi_softc *sc = device_get_softc(dev);
	uint8_t *buf_in, *buf_out;
	int i;

	//ar71xx_spi_chip_activate(sc, devi->cs);

	KASSERT(cmd->tx_cmd_sz == cmd->rx_cmd_sz,
	    ("TX/RX command sizes should be equal"));
	KASSERT(cmd->tx_data_sz == cmd->rx_data_sz,
	    ("TX/RX data sizes should be equal"));

	if(cmd->tx_cmd_sz == 0){
		BHND_DEBUG_DEV(child, ("size of command is ZERO"));
		return (EIO);
	}

	SPI_BARRIER_WRITE(sc);
	SPI_WRITE(sc, CHIPC_FLASHADDR, 0);
	SPI_BARRIER_WRITE(sc);

	/*
	 * Transfer command
	 */

	buf_out = (uint8_t *)cmd->tx_cmd;
	buf_in = (uint8_t *)cmd->rx_cmd;
	for (i = 0; i < cmd->tx_cmd_sz; i++)
		 if (chipc_spi_txrx(sc, buf_out[i], &(buf_in[i])))
			 return (EIO);

	/*
	 * Receive/transmit data
	 */
	buf_out = (uint8_t *)cmd->tx_data;
	buf_in = (uint8_t *)cmd->rx_data;
	for (i = 0; i < cmd->tx_data_sz; i++)
		if (chipc_spi_txrx(sc, buf_out[i], &(buf_in[i])))
			return (EIO);

	/*
	 * Clear CS bit and whole control register
	 */
	SPI_BARRIER_WRITE(sc);
	SPI_WRITE(sc, CHIPC_FLASHCTL, 0);
	SPI_BARRIER_WRITE(sc);

	return (0);
}

/*
 * **************************** METADATA ************************
 */

static device_method_t chipc_spi_methods[] = {
		DEVMETHOD(device_probe,		chipc_spi_probe),
		DEVMETHOD(device_attach,    chipc_spi_attach),
		/* SPI */
		DEVMETHOD(spibus_transfer,  		chipc_spi_transfer),
		DEVMETHOD_END
};


static driver_t chipc_spi_driver = {
	"spi",
	chipc_spi_methods,
	sizeof(struct chipc_spi_softc),
};

static devclass_t chipc_spi_devclass;


DRIVER_MODULE(chipc_spi, bhnd_chipc, chipc_spi_driver, chipc_spi_devclass, 0, 0);
