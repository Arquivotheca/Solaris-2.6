/*
 * Copyright (c) 1995 by Sun Microsystems, Inc.
 */

#pragma	ident	"@(#)mlx_conf.c	1.1	95/10/16 SMI"

/*
 * MLX-specific configuration routines
 */

#include <sys/dktp/mlx/mlx.h>

nops_t	*mlx_conf[] = {
	&dac960p_nops,

#if defined(i386)
	&dac960_nops,
	&dmc960_nops,
#endif	/* defined(i386) */

	NULL
};

nops_t *
mlx_hbatype(dev_info_t	 *dip,
		int		**rpp,
		int		 *lp,
		bool_t		  probing)
{
	bus_t	  bus_type = 0;
	int	  *regp = NULL;
	int	  reglen;
	int	  *pidp = NULL;		/* ptr to the product id property */
	int	  pidlen = 0;		/* length of the array */
	char	  *parent_type = NULL;
	int	  parentlen = 0;
	nops_t	  **nopspp = NULL;


	/*
	 * The parent-type property is a hack for 2.4 compatibility, since
	 * if you're on an EISA+PCI machine under Solaris 2.4 the PCI devices
	 * end up under EISA and there's no way to tell which one you're
	 * probing.  Getting the parent's device_type is the real right way.
	 */
	if ((ddi_getlongprop(DDI_DEV_T_ANY, dip,
	    DDI_PROP_DONTPASS, "parent-type", (caddr_t)&parent_type,
	    &parentlen) != DDI_PROP_SUCCESS)) {

		if (ddi_getlongprop(DDI_DEV_T_ANY, dip,
		    0, "device_type", (caddr_t)&parent_type,
		    &parentlen) != DDI_PROP_SUCCESS) {

			if (ddi_getlongprop(DDI_DEV_T_ANY, dip,
			    0, "bus-type", (caddr_t)&parent_type,
			    &parentlen) != DDI_PROP_SUCCESS) {
				parent_type = NULL;	/* Don't know. */
			}
		}
	}


	/* If we don't know, it's PCI */
	if (parent_type && strcmp(parent_type, "eisa") == 0)
		bus_type = BUS_TYPE_EISA;
	else if (parent_type && strcmp(parent_type, "mc") == 0)
		bus_type = BUS_TYPE_MC;
	else
		bus_type = BUS_TYPE_PCI;


	/* get the hba's address */
	if (ddi_getlongprop(DDI_DEV_T_NONE, dip, DDI_PROP_DONTPASS,
	    "reg", (caddr_t)&regp, &reglen) != DDI_PROP_SUCCESS) {
		if (parent_type != NULL)
			kmem_free(parent_type, parentlen);
		MDBG4(("mlx_hbatype: reg property not found\n"));
		return (NULL);
	}

	/* pass the reg property back to mlx_cfg_init() */
	if (rpp != NULL) {
		*rpp = regp;
		*lp = reglen;
	}

	/* the product id property is optional, if it's not specified */
	/* then the chip specific modules will use default values */
	if (ddi_getlongprop(DDI_DEV_T_NONE, dip, DDI_PROP_DONTPASS,
	    "product-id", (caddr_t)&pidp, &pidlen) != DDI_PROP_SUCCESS) {
		MDBG4(("mlx_hbatype: product-id property not found\n"));
		pidp = NULL;
		pidlen = 0;
	}

	/* determine which mlxops vector table to use */
	for (nopspp = mlx_conf; *nopspp != NULL; nopspp++) {
		if ((*nopspp)->mlx_probe(dip, regp, reglen, pidp, pidlen,
		    bus_type, probing)) {
			break;
		}
	}

	if (parent_type != NULL)
		kmem_free(parent_type, parentlen);
	if (pidp != NULL)
		kmem_free(pidp, pidlen);
	if (!nopspp || !rpp)
		kmem_free(regp, reglen);

	return (*nopspp);
}


/*
 * Determine the interrupt request line for this HBA
 */
/*ARGSUSED*/
bool_t
mlx_get_irq_pci(mlx_t		*mlxp,
			int		*regp,
			int		 reglen)
{
	ddi_acc_handle_t handle;

	if (pci_config_setup(mlxp->dip, &handle) != DDI_SUCCESS)
		return (FALSE);
	mlxp->irq = pci_config_getb(handle, PCI_CONF_ILINE);
	pci_config_teardown(&handle);

	cmn_err(CE_CONT, "?mlx: pci slot=%d,%d reg=0x%x\n",
		*regp, *(regp + 1), mlxp->reg);
	return (TRUE);
}

bool_t
mlx_cfg_init(dev_info_t	*dip,
		mlx_t		*mlxp)
{
	int	*regp;		/* ptr to the reg property */
	int	reglen; 	/* length of the array */

	regp = mlxp->regp;
	reglen = mlxp->reglen;

	if ((mlxp->rnum = MLX_RNUMBER(mlxp, regp, reglen)) < 0) {
		MDBG4(("mlx_cfg_init: no reg\n"));
		return (FALSE);
	}

	if (!MLX_INIT(mlxp, dip)) {
		MDBG4(("mlx_cfg_init: init failed\n"));
		return (FALSE);
	}

	if (!MLX_GET_IRQ(mlxp, regp, reglen)) {
		MDBG4(("mlx_cfg_init: no irq\n"));
		return (FALSE);
	}

	return (TRUE);
}

/*
 * Determine the i/o register base address for this HBA
 * PCI, read reg from the config register
 */
/*ARGSUSED*/
int
mlx_get_reg_pci(mlx_t	*mlxp,
			int	*regp,
			int	 reglen)
{
	ddi_acc_handle_t handle;

	if (pci_config_setup(mlxp->dip, &handle) != DDI_SUCCESS)
		return (-1);
	mlxp->reg =
	    pci_config_getl(handle, PCI_CONF_BASE0) & PCI_BASE_IO_ADDR_M;
	pci_config_teardown(&handle);

	return (MLX_PCI_RNUMBER);
}

#if defined(i386)
/*ARGSUSED*/
bool_t
mlx_get_irq_eisa(mlx_t		*mlxp,
			int		*regp,
			int		 reglen)
{
	if (eisa_get_irq(*regp, &mlxp->irq)) {
		cmn_err(CE_CONT, "?mlx: eisa slot=0x%x reg=0x%x\n",
			mlxp->reg, mlxp->reg);
		return (TRUE);
	}

	cmn_err(CE_WARN, "mlx: mlx_get_irq(eisa) failed reg=0x%x,0x%x,0x%x\n",
		*regp, *(regp +1), *(regp +2));
	return (FALSE);
}

/*ARGSUSED*/
bool_t
mlx_get_irq_mc(mlx_t		*mlxp,
			int		*regp,
			int		 reglen)
{
	if (mc_get_irq(*regp, &mlxp->irq)) {
		cmn_err(CE_CONT, "?mlx: mc slot=0x%x reg=0x%x\n",
			MLX_SLOT(*regp), mlxp->reg);
		return (TRUE);
	}

	cmn_err(CE_WARN, "mlx: mlx_get_irq(eisa) failed reg=0x%x,0x%x,0x%x\n",
		*regp, *(regp +1), *(regp +2));
	return (FALSE);
}
#endif	/* defined(i386) */
