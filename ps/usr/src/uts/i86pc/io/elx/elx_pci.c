/*
 * Copyright (c) 1996 by Sun Microsystems, Inc.
 * All Rights Reserved.
 */
#ident "@(#)elx_pci.c	1.7	96/05/09 SMI"

#include <sys/types.h>
#if	defined PCI_DDI_EMULATION || COMMON_IO_EMULATION
#include <sys/xpci/sunddi_2.5.h>
#else	/* PCI_DDI_EMULATION */
#include <sys/sunddi.h>
#endif	/* PCI_DDI_EMULATION */
#include <sys/pci.h>

#define	ELX_PCI_VENID	0x10b7

int elx_verify_id(ushort);

int
elx_pci_probe(dev_info_t *devinfo)
{
	ushort vendorid;
	ushort deviceid;
	int found;
	ddi_acc_handle_t handle;

	if (pci_config_setup(devinfo, &handle) != DDI_SUCCESS)
		return (0);

	vendorid = pci_config_getw(handle, PCI_CONF_VENID);
	deviceid = pci_config_getw(handle, PCI_CONF_DEVID);
	found = 0;

	if (vendorid == ELX_PCI_VENID && elx_verify_id(deviceid)) {
		ushort cmdreg;
		unchar iline;

		cmdreg = pci_config_getw(handle, PCI_CONF_COMM);
		iline = pci_config_getb(handle, PCI_CONF_ILINE);

		if (((iline <= 15) && (iline != 0)) && ((cmdreg & PCI_COMM_IO)))
			found = 1;
	}

	pci_config_teardown(&handle);

	return (found);
}

/*
 *	elx_pci_get_irq(devinfo)
 *		return interrupt (sanity sanity checked in probe above)
 */
/*ARGSUSED*/
int
elx_pci_get_irq(dev_info_t *devinfo)
{
	int iline = 0;
#ifdef	PCI_DDI_EMULATION
	ddi_acc_handle_t handle;

	if (pci_config_setup(devinfo, &handle) != DDI_SUCCESS)
		return (-1);

	iline = pci_config_getb(handle, PCI_CONF_ILINE);

	pci_config_teardown(&handle);
#endif	/* PCI_DDI_EMULATION */

	return (iline);
}

void
elx_pci_enable(dev_info_t *devinfo, int on_off)
{
	ushort tmp;
	ddi_acc_handle_t handle;

	if (pci_config_setup(devinfo, &handle) != DDI_SUCCESS)  {
		cmn_err(CE_WARN, "elx: PCI error enabling adapter");
		return;
	}

	tmp = pci_config_getw(handle, PCI_CONF_COMM);

	if (on_off)
		tmp |= PCI_COMM_ME;
	else
		tmp &= ~PCI_COMM_ME;

	/*
	 * as per the errata list in "PCI/EISA Bus-Master
	 * Adapter Driver Technical Reference", May 1995
	 */
	tmp &= ~PCI_COMM_PARITY_DETECT;

	pci_config_putw(handle, PCI_CONF_COMM, tmp);
#ifndef	PCI_DDI_EMULATION
	pci_config_putw(handle, PCI_CONF_LATENCY_TIMER, 0xff);
#endif
	pci_config_teardown(&handle);
}
