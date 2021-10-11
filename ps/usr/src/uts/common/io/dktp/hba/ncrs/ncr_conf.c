/*
 * Copyright (c) 1995 by Sun Microsystems, Inc.
 */

#pragma	ident	"@(#)ncr_conf.c	1.9	96/02/02 SMI"

/*
 * NCR-specific configuration routines
 */

#include <sys/dktp/ncrs/ncr.h>
#include <sys/pci.h>

#if defined(i386)
extern	nops_t	ncr53c710_nops;
extern	nops_t	ncr53c710_nops_SNI;
extern	nops_t	ncr53c825_nops_compaq;
#endif
extern	nops_t	ncr53c810_nops;

nops_t	*ncr_conf[] = {
	&ncr53c810_nops,

#if defined(i386)
	&ncr53c710_nops,

/* Special hack for the Siemens/Nixdorf Inc PCE-5S, it must */
/* appear after the ncr53c710_nops entry. Don't move it. */
	&ncr53c710_nops_SNI,

/* Special hack for the Compaq 32-Bit Fast-Wide SCSI-2 as used on */
/* the Compaq Proliant 4500 */
	&ncr53c825_nops_compaq,

#endif	/* defined(i386) */

	NULL
};


nops_t *
ncr_hbatype(	dev_info_t	 *dip,
		int		**rpp,
		int		 *lp,
		bus_t		 *bp,
		bool_t		  probing )
{
	nops_t	**nopspp;
	bus_t	  bus_type;
	int	 *regp;
	int	  reglen;
	int	 *pidp;		/* ptr to the product id property */
	int	  pidlen;	/* length of the array */
	char	  parent_type[16];
	int	  parentlen;
	

	/*
	 * The parent-type property is a hack for 2.4 compatibility, since
	 * if you're on an EISA+PCI machine under Solaris 2.4 the PCI devices
	 * end up under EISA and there's no way to tell which one you're
	 * probing.  Getting the parent's device_type is the real right way.
	 */
	parentlen = sizeof (parent_type);
	if (ddi_getlongprop_buf(DDI_DEV_T_NONE, dip,
	    DDI_PROP_DONTPASS, "parent-type", (caddr_t)parent_type,
	    &parentlen) != DDI_PROP_SUCCESS) {

		parentlen = sizeof (parent_type);
		if (ddi_getlongprop_buf(DDI_DEV_T_NONE, ddi_get_parent(dip),
		    DDI_PROP_DONTPASS, "device_type", (caddr_t)parent_type,
		    &parentlen) != DDI_PROP_SUCCESS) {
			parent_type[0] = '\0';	/* Don't know. */
		}
	}

	/* If we don't know, it's EISA */
	if (strcmp(parent_type, "pci") == 0)
		bus_type = BUS_TYPE_PCI;
	else
		bus_type = BUS_TYPE_EISA;


	/* get the hba's address */
	if (ddi_getlongprop(DDI_DEV_T_NONE, dip, DDI_PROP_DONTPASS,
	    "reg", (caddr_t)&regp, &reglen) != DDI_PROP_SUCCESS) {
		NDBG4(("ncr_hbatype: reg property not found\n"));
		return (NULL);
	}


	/* pass the reg property back to ncr_cfg_init() */
	if (rpp != NULL) {
		*rpp = regp;
		*lp = reglen;
		*bp = bus_type;
	}


	/* the product id property is optional, if it's not specified */
	/* then the chip specific modules will use default values */
	if (ddi_getlongprop(DDI_DEV_T_NONE, dip, DDI_PROP_DONTPASS
		, "product-id", (caddr_t)&pidp, &pidlen) != DDI_PROP_SUCCESS) {
		NDBG4(("ncr_hbatype: product-id property not found\n"));
		pidp = NULL;
		pidlen = 0;
	}

	/* determine which ncrops vector table to use */
	for (nopspp = ncr_conf; *nopspp != NULL; nopspp++) {
		if ((*nopspp)->ncr_probe(dip, regp, reglen, pidp, pidlen
					     , bus_type, probing)) {
			if (rpp == NULL)
				kmem_free(regp, reglen);
			return (*nopspp);
		}
	}

	if (pidp != NULL)
		kmem_free(pidp, pidlen);
	kmem_free(regp, reglen);
	return (NULL);
}


/*
 * Determine the interrupt request line for this HBA
 */
bool_t
ncr_get_irq_pci(	ncr_t		*ncrp,
			int		*regp,
			int		 reglen )
{
	ddi_acc_handle_t handle;

	if (pci_config_setup(ncrp->n_dip, &handle) != DDI_SUCCESS)
		return (FALSE);
	ncrp->n_irq = pci_config_getb(handle, PCI_CONF_ILINE);
	pci_config_teardown(&handle);

	cmn_err(CE_CONT, "?ncrs: instance=%d: pci slot=%x\n",
		ddi_get_instance(ncrp->n_dip), *regp);
	return (TRUE);
}

#if defined(i386)
bool_t
ncr_get_irq_eisa(	ncr_t		*ncrp,
			int		*regp,
			int		 reglen )
{
	if (eisa_get_irq(*regp, &ncrp->n_irq)) {
		cmn_err(CE_CONT, "?ncrs: instance=%d: eisa slot=0x%x\n",
			ddi_get_instance(ncrp->n_dip), ncrp->n_reg);
		return (TRUE);
	}

	cmn_err(CE_WARN, "ncrs: ncr_get_irq(eisa) failed reg=0x%x,0x%x,0x%x\n"
		, *regp, *(regp +1), *(regp +2));
	return (FALSE);
}

#ifdef	PCI_DDI_EMULATION
/*
 * Determine the i/o register base address for this HBA
 * PCI, read ioaddr from the config register
 */
bool_t
ncr_get_ioaddr_pci(	ncr_t	*ncrp,
			int	*regp,
			int	 reglen )
{
	ddi_acc_handle_t handle;
	int iobase;

	if (pci_config_setup(ncrp->n_dip, &handle) != DDI_SUCCESS)
		return (FALSE);
	ncrp->n_reg =
	    pci_config_getl(handle, PCI_CONF_BASE0) & PCI_BASE_IO_ADDR_M;
	pci_config_teardown(&handle);

	return (TRUE);

}

bool_t
ncr_get_ioaddr_eisa(	ncr_t	*ncrp,
			int	*regp,
			int	 reglen )
{
	/* EISA, ioaddr is the same as the slot address */
	ncrp->n_reg = *regp;
	return (TRUE);
}
#endif	/* PCI_DDI_EMULATION */
#endif	/* defined(i386) */

bool_t
ncr_cfg_init(	dev_info_t	*dip,
		ncr_t		*ncrp )
{
	int	*regp;		/* ptr to the reg property */
	int	 reglen;	/* length of the array */
	int	 val;
	int	 len;
	bus_t	 btype;
#ifndef PCI_DDI_EMULATION
	static ddi_device_acc_attr_t attr = {
		DDI_DEVICE_ATTR_V0,
		DDI_NEVERSWAP_ACC,	/* not portable */
		DDI_STRICTORDER_ACC,
	};
#endif

	ncrp->n_ops = ncr_hbatype(dip, &regp, &reglen, &btype, FALSE);

	if (ncrp->n_ops == NULL) {
		NDBG4(("ncr_cfg_init: no hbatype match\n"));
		return (FALSE);
	}

	if (ddi_regs_map_setup(dip, NCR_RNUMBER(ncrp), (caddr_t *)&ncrp->n_reg,
		(offset_t)0, (offset_t)0, &attr, &ncrp->n_handle)
		!= DDI_SUCCESS) {
		NDBG4(("ncr_cfg_init: no ioaddr\n"));
		goto err_exit;
	}
	ncrp->n_bustype = btype;
	ncrp->n_regp = regp;
	ncrp->n_reglen = reglen;

#ifdef	PCI_DDI_EMULATION
	if (!NCR_GET_IOADDR(ncrp, regp, reglen)) {
		NDBG4(("ncr_cfg_init: no ioaddr\n"));
		goto err_exit;
	}
#endif

	if (!NCR_GET_IRQ(ncrp, regp, reglen)) {
		NDBG4(("ncr_cfg_init: no irq\n"));
		ddi_regs_map_free(&ncrp->n_handle);
		goto err_exit;
	}
	return (TRUE);

    err_exit:
	kmem_free(regp, reglen);
	return (FALSE);
}
