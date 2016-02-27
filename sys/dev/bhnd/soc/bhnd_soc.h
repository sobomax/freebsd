/*
 * bhnd_soc.h
 *
 *  Created on: Feb 25, 2016
 *      Author: mizhka
 */

#pragma once

#define SOC_TO_BRIDGE	0
#define SOC_TO_BUS		1

#define BHND_SOC_CCREGS_OFFSET	0x18000000
#define	BHND_SOC_CCREGS_SIZE	0x1000
#define BHND_SOC_MAPPED_OFFSET	0x18000000
#define	BHND_SOC_MAPPED_SIZE	0x01000000

#define BHND_SOC_EROMREGS_OFFSET 0x1810e000
#define BHND_SOC_EROMREGS_SIZE	 0x1000

struct bhnd_soc_softc {
	device_t dev;
	device_t bridge;
	device_t bus;
};
