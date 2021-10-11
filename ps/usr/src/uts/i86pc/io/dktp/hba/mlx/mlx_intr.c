/*
 * Copyright (c) 1995 by Sun Microsystems, Inc.
 */

#pragma	ident	"@(#)mlx_intr.c	1.1	95/10/16 SMI"

#include <sys/dktp/mlx/mlx.h>

/*ARGSUSED*/
static u_int
dummy_intr(caddr_t arg)
{
	return (DDI_INTR_UNCLAIMED);
}

/*
 * This routine should be part of the DDI/DDK. It works the
 * way ddi_add_intr should've worked.
 */
bool_t
add_intr(dev_info_t		*dip,
		u_int			intr_idx,
		ddi_iblock_cookie_t 	*iblock_cookiep,
		kmutex_t		*mutexp,
		char			*mutexnamep,
		u_int			(*intr_func)(caddr_t),
		caddr_t			intr_arg)
{

/*
 *	Establish initial dummy interrupt handler
 *	get iblock_cookie cookie to initialize mutexes used in the
 *	real interrupt handler
 */
	if (ddi_add_intr(dip, intr_idx, iblock_cookiep, NULL, dummy_intr,
	    NULL)) {
		cmn_err(CE_WARN, "add_intr: cannot add dummy intr");
		return (FALSE);
	}

	/* Make a mutex for our own use and lock it */
	mutex_init(mutexp, mutexnamep, MUTEX_DRIVER, *iblock_cookiep);
	mutex_enter(mutexp);

	ddi_remove_intr(dip, intr_idx, *iblock_cookiep);

	/* Establish real interrupt handler */
	if (ddi_add_intr(dip, intr_idx, iblock_cookiep, NULL, intr_func,
	    intr_arg)) {
		cmn_err(CE_WARN, "add_intr: cannot add intr");
		mutex_exit(mutexp);
		return (FALSE);
	}
	mutex_exit(mutexp);
	return (TRUE);
}


/*
 * scan the regspec from the driver.conf file looking for a
 * match on the irq value. Return the index of the matching tuple.
 */
/*ARGSUSED*/
bool_t
mlx_find_irq(dev_info_t	*dip,
		unchar		 irq,
		int		*intrp,
		int		 len,
		u_int		*intr_idx)
{
	int	nintrs;
	int	indx;

	/*
	 * Check the pairs of interrupt specs (level, irq) for a
	 * match on the irq. The irq value is the second int of
	 * the pair.
	 */
	nintrs = len / sizeof (int);
	intrp++;
	for (indx = 0; indx < nintrs; indx++, intrp += 2) {
		if (*intrp == irq) {
			*intr_idx = indx;
			return (TRUE);
		}
	}

	return (FALSE);
}


/*
 * Convert an IRQ into the index number of the matching tuple
 * in the interrupts property from the mlx.conf file. If the
 * interrupts property isn't in the mlx.conf file create it
 * dynamically using the default priority level of 5.
 *
 * This is needed only for non-self-ID configurations.
 */
bool_t
mlx_xlate_irq_no_sid(mlx_t *mlxp)
{
	dev_info_t	*dip = mlxp->dip;
	int	*intrp;
	int	 len;
	int	 intrspec[3];
	bool_t	 rc;

	/* let mlx.conf file override default interrupt level */
	if (ddi_getlongprop(DDI_DEV_T_NONE, dip, DDI_PROP_DONTPASS,
		"interrupts", (caddr_t)&intrp, &len) == DDI_PROP_SUCCESS) {
		MDBG4(("mlx_xlate_irq_no_sid: interrupts property found\n"));
		rc = mlx_find_irq(dip, mlxp->irq, intrp, len, &mlxp->intr_idx);
		kmem_free(intrp, len);
		return (rc);
	}

	/* create an interrupt spec using default interrupt priority level */
	intrspec[0] = 2;
	intrspec[1] = 5;
	intrspec[2] = mlxp->irq;
	if (ddi_ctlops(dip, dip, DDI_CTLOPS_XLATE_INTRS, (caddr_t)intrspec,
	    ddi_get_parent_data(dip)) != DDI_SUCCESS) {
		MDBG4(("mlx_xlate_irq_no_sid: interrupt create failed\n"));
		return (FALSE);
	}
	mlxp->intr_idx = 0;
	MDBG4(("mlx_xlate_irq_no_sid: okay\n"));
	return (TRUE);
}

/*
 * For self-ID configurations, the framework does all the work.
 */
bool_t
mlx_xlate_irq_sid(mlx_t *mlxp)
{
	mlxp->intr_idx = 0;
	MDBG4(("mlx_xlate_irq_sid: okay\n"));
	return (TRUE);
}

bool_t
mlx_intr_init(dev_info_t	*dip,
		mlx_t 		*mlxp,
		caddr_t		intr_arg)
{

	/* map the irq into the interrupt spec index number */
	if (!MLX_XLATE_IRQ(mlxp)) {
		MDBG4(("mlx_intr_init: xlate failed\n"));
		return (FALSE);
	}

	if (!add_intr(dip, mlxp->intr_idx, &mlxp->iblock_cookie,
	    &mlxp->mutex, "mlx mutex", mlx_intr, intr_arg)) {
		MDBG4(("mlx_intr_init: add_intr failed\n"));
		return (FALSE);
	}
	MDBG4(("mlx_intr_init: add_intr okay\n"));
	return (TRUE);
}
