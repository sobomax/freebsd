/*
 * bcma_pcie.c
 *
 *  Created on: Feb 5, 2016
 *      Author: mizhka
 */

#include <sys/kernel.h>
#include <sys/module.h>
#include <sys/errno.h>
#include <sys/bus.h>

#include <dev/pci/pcivar.h>
#include <dev/pci/pcireg.h>

#include <dev/bcma/bcma_ids.h>
#include <dev/bcma/bcma_var.h>
#include <dev/bcma/bcma_debug.h>
#include <dev/bcma/bcma_resources.h>

static int bcma_pcie_probe(device_t dev){
	struct bcma_eeprom_coreinfo *ptr = device_get_ivars(dev);
	if(ptr->id == BCMA_CORE_PCIE){
		BCMA_DEBUG(("PCI-e found with rev 0x%02x", ptr->revision));
		return BUS_PROBE_GENERIC;
	}

	return ENXIO;
}

static int bcma_pcie_attach(device_t dev){
	//enable core
	//
	//tries to read core memory
	struct bcma_eeprom_coreinfo* coreinfo = device_get_ivars(coreinfo);
	struct bcma_pcie_softc* sc = device_get_softc(dev);

	sc->mem_rid = BCMA_RES_MEM_RID;
	sc->mem = bus_alloc_resource_any(dev, SYS_RES_MEMORY, &(sc->mem_rid), RF_ACTIVE);

	if(sc->mem == NULL){
		BCMA_WARN(("can't allocate core memory area for PCI-e"));
		return ENXIO;
	}

	/* Disable the RETRY_TIMEOUT register (0x41) to keep
	//* PCI Tx retries from interfering with C3 CPU state */
	//188	pci_read_config_dword(dev, 0x40, &val);


	return 0;
}

static struct device_method_t bcma_pcie_methods[] =
{
		DEVMETHOD(device_probe, bcma_pcie_probe),
		DEVMETHOD(device_attach,bcma_pcie_attach)
};

static driver_t bcma_pcie_driver = {
		"bcma_pcie",
		bcma_pcie_methods,
		sizeof(struct bcma_pcie_softc)
};

static devclass_t bcma_pcie_class;

DRIVER_MODULE(bcma_pcie, bcma, bcma_pcie_driver, bcma_pcie_class, 0, 0);
