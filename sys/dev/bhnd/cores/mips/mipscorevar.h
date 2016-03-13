/*
 * mipscorevar.h
 *
 *  Created on: Mar 9, 2016
 *      Author: mizhka
 */

#pragma once

#define MIPSCORE_MAX_RSPEC 2

struct mipscore_softc {
	device_t		 dev;		/**< CPU device */
	uint32_t		 devid;
	struct resource_spec	 rspec[MIPSCORE_MAX_RSPEC];
	struct bhnd_resource	*res[MIPSCORE_MAX_RSPEC];
};

struct mipscore_regs{
        uint32_t        corecontrol;
        uint32_t        exceptionbase;
        uint32_t        PAD1[1];
        uint32_t        biststatus;
        uint32_t        intstatus;
        uint32_t        intmask[6];
        uint32_t        nmimask;
        uint32_t        PAD2[4];
        uint32_t        gpioselect;
        uint32_t        gpiooutput;
        uint32_t        gpioenable;
        uint32_t        PA3[101];
        uint32_t        clkcontrolstatus;
} ;
