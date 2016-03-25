/*
 * chipc_spi.h
 *
 *  Created on: Mar 24, 2016
 *      Author: mizhka
 */

#pragma once

#define CHIPC_SPI_MAXRES 		2
#define CHIPC_SPI_MAXTRIES 		1000

#define CHIPC_SPI_ACTION_INPUT	1
#define CHIPC_SPI_ACTION_OUTPUT	2

struct chipc_spi_softc{
	device_t dev;
	struct bhnd_resource* bhnd_res[CHIPC_SPI_MAXRES];
	struct resource* sc_mem_res;
};

/*
 * register space access macros
 */

#define	SPI_BARRIER_WRITE(sc)		bus_barrier((sc)->sc_mem_res, 0, 0, 	\
					    BUS_SPACE_BARRIER_WRITE)
#define	SPI_BARRIER_READ(sc)	bus_barrier((sc)->sc_mem_res, 0, 0, 	\
					    BUS_SPACE_BARRIER_READ)
#define	SPI_BARRIER_RW(sc)		bus_barrier((sc)->sc_mem_res, 0, 0, 	\
					    BUS_SPACE_BARRIER_READ | BUS_SPACE_BARRIER_WRITE)

#define SPI_WRITE(sc, reg, val)	do {	\
		bus_write_4(sc->sc_mem_res, (reg), (val)); \
	} while (0)

#define SPI_READ(sc, reg)	 bus_read_4(sc->sc_mem_res, (reg))

#define SPI_SET_BITS(sc, reg, bits)	\
	SPI_WRITE(sc, reg, SPI_READ(sc, (reg)) | (bits))

#define SPI_CLEAR_BITS(sc, reg, bits)	\
	SPI_WRITE(sc, reg, SPI_READ(sc, (reg)) & ~(bits))


