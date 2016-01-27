/*
 * bcma_subr.c
 *
 *  Created on: Jan 27, 2016
 *      Author: mizhka
 */

#include <sys/param.h>
#include <sys/bus.h>

#include <machine/bus.h>
#include <sys/rman.h>
#include <machine/resource.h>

#include <dev/bcma/bcma_var.h>
#include <dev/bcma/bcma_reg.h>
#include <dev/bcma/bcma_debug.h>
#include <dev/bcma/bcma_ids.h>

void bcma_release_resources(struct bcma_softc* sc){
	device_t dev;
	dev = sc->bcma_dev;

	if(sc->mem != NULL)
		bus_release_resource(dev,SYS_RES_MEMORY, sc->mem_rid, sc->mem);

	if(sc->erom_mem != NULL)
		bus_release_resource(dev,SYS_RES_MEMORY, sc->erom_rid ,sc->erom_mem);

	return;
}

void bcma_scan_eeprom(struct bcma_softc* sc){
	BCMA_INFO(("Start scanning of EEPROM..."));
	struct resource* erom = sc->erom_mem;
	bus_space_tag_t erom_mem_tag = rman_get_bustag(erom);
	bus_space_handle_t erom_mem_hndl = rman_get_bushandle(erom);

	int32_t state = 0, counter = 0, core_id = -1, index = -sizeof(u_int32_t);
	u_int8_t skip_next_read = 0;
	u_int32_t current = 0;
	struct bcma_eeprom_coreinfo* core;

	u_int32_t tag;
	u_int32_t tagx;

	//TODO: store core to software context
	core = malloc(sizeof(struct bcma_eeprom_coreinfo), M_BCMA, M_WAITOK);

	while(1){
		if(skip_next_read){
			skip_next_read = 0;
		}else{
			/* read current word */
			index+= sizeof(u_int32_t);
			current = bus_space_read_4(erom_mem_tag, erom_mem_hndl, index);
			BCMA_TRACE(("eeprom[%02x] = 0x%04x", index, current));
		}
		/* is word valid? */
		if(!(current & BCMA_EROM_VALID)){
			BCMA_WARN(("Invalid record? [%02x] = 0x%04x", index, current));
			continue; //??? reset / exit?
		}
		tagx = (current & BCMA_EROM_TAGX);
		tag  = (current & BCMA_EROM_TAG);

		if(tag == BCMA_EROM_TAG_CI && state > 1)
			state = 0;

		if(current == (BCMA_EROM_VALID | BCMA_EROM_TAG_END)){
			BCMA_INFO(("Scanning has finished successfully"));
			return;
		}

		switch(state){
		case 0://First byte of core
			core_id++;
			core->class = BCMA_EROM_BITS(current, BCMA_EROM_CLASS);
			core->id 	= BCMA_EROM_BITS(current, BCMA_EROM_ID);
			core->manuf = BCMA_EROM_BITS(current, BCMA_EROM_MANUF);
			BCMA_DEBUG(("<====================================================>"));
			BCMA_DEBUG(("[core %d] class: 0x%02x; id: 0x%02x; manuf: 0x%02x",core_id, core->class, core->id,core-> manuf));
			BCMA_INFO(("[core %d] %s (%s)", core_id, bcma_ids_lookup(core->id, bcma_core_ids), bcma_ids_lookup(core->manuf, bcma_vendors)))
			state++;
			break;
		case 1://Second byte of core
			core->revision = BCMA_EROM_BITS(current, BCMA_EROM_REV);
			core->num_mports = BCMA_EROM_BITS(current, BCMA_EROM_NMP);
			core->num_sports = BCMA_EROM_BITS(current, BCMA_EROM_NSP);
			core->num_mwraps = BCMA_EROM_BITS(current, BCMA_EROM_NMW);
			core->num_swraps = BCMA_EROM_BITS(current, BCMA_EROM_NSW);
			BCMA_DEBUG(("revision: 0x%02x; mports: 0x%02x; sports: 0x%02x; mwraps: 0x%02x; swraps: 0x%02x",
					core->revision, core->num_mports,core-> num_sports, core->num_mwraps,core-> num_swraps));

			//if(core->manuf == 0x43B && core->id == 0xFFF){
			//	BCMA_WARN(("TBD: ARM stuff?"));
			//	state = 40;
			//	break;
			//}

			if(core->num_sports == 0){
				BCMA_WARN(("there is no S port"));
				state = 40;
				break;
			}

			/* check if component is a core at all */
			if(core->num_mwraps == 0 && core->num_swraps == 0){
				BCMA_DEBUG(("Component doesn't have wrappers..."));
				//state = 40;
				//break;
			}

			state++;
			counter = core->num_mports;
			if(counter == 0)
				state++; //skip reading master port information if there is no ports :)
			break;
		case 2://read M ports
			if((current & BCMA_EROM_TAG) != BCMA_EROM_TAG_MP){
				BCMA_WARN(("unexpected non-MP record"));
				state = 40;
				skip_next_read = 1;
				break;
			}

			// Is any parsing required? :?
			// On Asus RT-N16 there is only one port and value is 0x00000003 (3 is tag means valid + master port)

			counter--;
			if(counter == 0){
				state++;
			}
			break;
		case 3:// read slave ports - There are 2 ports on Asus RT-N16
		case 4:// read master wrappers - 1 main wrapper for Asus RT-N16
		case 5:// read slave wrappers - 4 slave wrappers
			;
			u_int32_t addr_aggr = (current & BCMA_EROM_ADDR_AG32);
			u_int32_t addr_size = BCMA_EROM_BITS(current, BCMA_EROM_ADDR_SZ);
			u_int32_t addr_type = (current & BCMA_EROM_ADDR_TYPE);
			u_int32_t addr_port = BCMA_EROM_BITS(current, BCMA_EROM_ADDR_PORT);
			u_int32_t address = (current & BCMA_EROM_ADDR_ADDR);

			BCMA_TRACE(("tagx: 0x%01x; aggregation: 0x%01x; size: 0x%01x; addrType: 0x%01x; addrPort: 0x%01x; address: 0x%04x",
					tagx, addr_aggr, addr_size, addr_type, addr_port, address));

			//check if it's address or not
			if(tagx != BCMA_EROM_TAG_ADDR){
				BCMA_WARN(("unexpected non-ADDRESS record: 0x%04x",tag));
				state=40;
				skip_next_read=1;
				break;
			}

			struct bcma_eeprom_coreaddress* coreaddress = malloc(sizeof(struct bcma_eeprom_coreaddress),M_BCMA, M_WAITOK);

			coreaddress->type = addr_type;
			coreaddress->port = addr_port;
			coreaddress->address = address;

			//if aggregation -> read high byte 		//state 10
			if(addr_aggr > 0){
				index+= sizeof(u_int32_t);
				current = bus_space_read_4(erom_mem_tag, erom_mem_hndl, index);
				BCMA_TRACE(("ADDR32: eeprom[%02x] = 0x%04x", index, current));
			}

			//if size undef -> read next byte 		//state 11
			// move to state
			if(addr_size == BCMA_EROM_ADDR_SZ_SZD){
				index+= sizeof(u_int32_t);
				current = bus_space_read_4(erom_mem_tag, erom_mem_hndl, index);
				BCMA_TRACE(("CUSTOM SIZE: eeprom[%02x] = 0x%04x", index, current));
				addr_aggr = (current & BCMA_EROM_ADDR_AG32);
				//if size aggregation -> read high byte //state 12
				if(addr_aggr > 0){
					index+= sizeof(u_int32_t);
					current = bus_space_read_4(erom_mem_tag, erom_mem_hndl, index);
					BCMA_TRACE(("CUSTOM SIZE32: eeprom[%02x] = 0x%04x", index, current));
				}
			}else{
				coreaddress->size = BCMA_EROM_ADDR_SZ_BASE << addr_size;
			}

			BCMA_DEBUG(("Assigned address for 0x%01x/0x%01x: 0x%04x - 0x%04x (0x%04x)",
					coreaddress->type, coreaddress->port, coreaddress->address, coreaddress->address + coreaddress->size - 1, coreaddress->size))

			// check port ... TODO: if(addr_port =)
			break;
		case 40://error... please lookup next
			break;
		default:
			BCMA_WARN(("UNKNOWN STATE: exit"))
			return;
		}
	}
}

