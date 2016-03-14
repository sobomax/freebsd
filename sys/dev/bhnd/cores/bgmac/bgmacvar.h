/*
 * bgmacvar.h
 *
 *  Created on: Mar 13, 2016
 *      Author: mizhka
 */

#pragma once

#define BGMAC_MAX_RSPEC	2

struct bgmac_softc{
	device_t dev;
	struct resource_spec	 rspec[BGMAC_MAX_RSPEC];
	struct bhnd_resource	*res[BGMAC_MAX_RSPEC];
	bus_space_handle_t 	hdl; /** main resource handle */
	bus_space_tag_t 	tag; /** main resource tag */
};
