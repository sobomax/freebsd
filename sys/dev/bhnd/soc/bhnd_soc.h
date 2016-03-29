/*
 * bhnd_soc.h
 *
 *  Created on: Feb 25, 2016
 *      Author: mizhka
 */

#pragma once

#define SOC_TO_BUS		1

#define BHND_SOC_MAXNUM_CORES	0x20
#define BHND_SOC_RAM_OFFSET		0x0
#define BHND_SOC_RAM_SIZE		0x20000000

struct bhnd_soc_softc {
	device_t dev;
	device_t bridge;
	device_t bus;
	struct bhnd_chipid		 chipid;	/**< chip identification */
};

struct bhnd_soc_devinfo{
	struct resource_list resources;
};

//TOREMOVE:
#define SOC_TO_BRIDGE	0

#define BHND_SOC_CCREGS_OFFSET	0x18000000
#define	BHND_SOC_CCREGS_SIZE	0x1000
#define BHND_SOC_MAPPED_OFFSET	0x18000000
#define	BHND_SOC_MAPPED_SIZE	0x08000000
