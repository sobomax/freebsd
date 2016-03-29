/*
 * chipc_subr.c
 *
 *  Created on: Feb 29, 2016
 *      Author: mizhka
 */

#include <sys/param.h>
#include <sys/errno.h>
#include <sys/bus.h>
#include <sys/malloc.h>

#include <sys/rman.h>

#include <machine/resource.h>
#include <dev/bhnd/bhndvar.h>

#include "bus_if.h"
#include "chipc.h"
#include "chipcvar.h"
#include "chipcreg.h"

int chipc_init_pflash(device_t dev, uint32_t flash_config){
	/*TODO:
	 * 1) read manufacture & device ID
	 * 2)
	 */
	//XXX: region 1 is taken from actual HW configuration...
	int width = (flash_config & CHIPC_CF_DS) ? 2 : 1;
	int enabled = (flash_config & CHIPC_CF_EN);
	int byteswap = (flash_config & CHIPC_CF_BS);

	device_printf(dev,"trying attach flash width=%d enabled=%d swapbytes=%d\n", width, enabled, byteswap);
	int chipc_rid = bhnd_get_port_rid(dev, BHND_PORT_DEVICE, 1, 1);
	struct bhnd_resource* chipc_res = bhnd_alloc_resource_any(dev, SYS_RES_MEMORY, &chipc_rid, 0);
	if(chipc_res == NULL){
		device_printf(dev, "can't allocate dev1.1 port for ChipCommon Parallel Flash\n");
		return ENXIO;
	}

	struct chipc_devinfo* devinfo = malloc(sizeof(struct chipc_devinfo*), M_BHND, M_NOWAIT);
	if (devinfo == NULL){
		device_printf(dev, "can't allocate memory for chipc_devinfo\n");
		return ENOMEM;
	}

	resource_list_init(&(devinfo->resources));

	device_t flashdev = device_add_child(dev, "cfi", -1);
	if(flashdev == NULL){
		device_printf(dev, "can't add ChipCommon Parallel Flash to bus\n");
		return ENXIO;
	}

	device_set_ivars(flashdev,devinfo);
	resource_list_add(&devinfo->resources, SYS_RES_MEMORY, 0, rman_get_start(chipc_res->res),
			rman_get_end(chipc_res->res), 1);

	bhnd_release_resource(dev, SYS_RES_MEMORY, chipc_rid, chipc_res);
	return 0;
}

int chipc_init_sflash(device_t dev, char* flash_name){
	device_t chipc_spi = device_add_child(dev, "spi", -1);
	if(chipc_spi == NULL){
		BHND_ERROR_DEV(dev, ("can't add chipc_spi to ChipCommon"));
		return ENXIO;
	}

	int err = device_probe_and_attach(chipc_spi);
	if(err){
		BHND_ERROR_DEV(dev, ("failed attach chipc_spi: %d", err));
		return err;
	}

	device_t* children;
	int cnt_children;
	err = device_get_children(chipc_spi, &children, &cnt_children);

	if(err){
		BHND_ERROR_DEV(chipc_spi, ("can't get list of children: %d", err));
		return err;
	}

	if(cnt_children == 0){
		BHND_ERROR_DEV(chipc_spi, ("no found children"));
		return ENXIO;
	}

	device_t flash = BUS_ADD_CHILD(children[0], 0, flash_name, -1);

	if(children)
		free(children, M_TEMP);

	if (!flash){
		BHND_ERROR_DEV(chipc_spi, ("can't add %s to spibus", flash_name));
	}

	err = device_probe_and_attach(flash);
	if(err){
		BHND_ERROR_DEV(dev, ("failed attach flash %s: %d", flash_name, err));
		return err;
	}

	return 0;
}
