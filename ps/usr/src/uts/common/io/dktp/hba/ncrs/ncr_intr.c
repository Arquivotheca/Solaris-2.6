/*
 * Copyright (c) 1995 by Sun Microsystems, Inc.
 */

#pragma	ident	"@(#)ncr_intr.c	1.6	96/02/02 SMI"

#include <sys/dktp/ncrs/ncr.h>


static u_int
dummy_intr(caddr_t arg)
{
	return (DDI_INTR_UNCLAIMED);
}

/*
 * This routine should be part of the DDI/DDK. It works the
 * way ddi_add_intr should've worked.
 */
static bool_t
add_intr(	dev_info_t		*dip,
		u_int			inumber,
		ddi_iblock_cookie_t 	*iblockp,
		kmutex_t		*mutexp,
		char			*mutexnamep,
		u_int			(*intr_func)(caddr_t),
		caddr_t			intr_arg )
{

/*
 *	Establish initial dummy interrupt handler 
 *	get iblock cookie to initialize mutexes used in the 
 *	real interrupt handler
 */
	if (ddi_add_intr(dip, inumber, iblockp, NULL, dummy_intr, NULL)){
		cmn_err(CE_WARN, "add_intr: cannot add dummy intr");
		return (FALSE);
	}

	/* Make a mutex for our own use and lock it */
	mutex_init(mutexp, mutexnamep, MUTEX_DRIVER, *iblockp);
	mutex_enter(mutexp);

	ddi_remove_intr(dip, inumber, *iblockp);

	/* Establish real interrupt handler */
	if (ddi_add_intr(dip, inumber, iblockp, NULL, intr_func, intr_arg)) {
		cmn_err(CE_WARN, "add_intr: cannot add intr");
		mutex_exit(mutexp);
		return (FALSE);
	}
	return (TRUE);
}


/*
 * scan the regspec from the driver.conf file looking for a 
 * match on the irq value. Return the index of the matching tuple.
 */

bool_t
ncr_find_irq(	dev_info_t	*dip,
		unchar		 irq,
		int		*intrp,
		int		 len,
		u_int		*inumber )
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
	for (indx = 0; indx < nintrs; indx++, intrp += 2 ) {
		if (*intrp == irq) {
			*inumber = indx;
			return (TRUE);
		}
	}

	return (FALSE);
}


/*
 * Convert an IRQ into the index number of the matching tuple
 * in the interrupts property from the ncrs.conf file. If the
 * interrupts property isn't in the ncrs.conf file create it
 * dynamically using the default priority level of 5.
 *
 * This is needed only for non-self-ID configurations.
 */
bool_t
ncr_xlate_irq_no_sid( ncr_t *ncrp )
{
	dev_info_t	*dip = ncrp->n_dip;
	int	*intrp;
	int	 len;
	int	 intrspec[3];
	bool_t	 rc;

	/* let ncrs.conf file override default interrupt level */
	if (ddi_getlongprop(DDI_DEV_T_NONE, dip, DDI_PROP_DONTPASS
		, "interrupts", (caddr_t)&intrp, &len) == DDI_PROP_SUCCESS) {
		NDBG4(("ncr_xlate_irq_no_sid: interrupts property found\n"));
		rc = ncr_find_irq(dip, ncrp->n_irq, intrp, len
			, &ncrp->n_inumber);
		kmem_free(intrp, len);
		return (rc);
	}

	/* create an interrupt spec using default interrupt priority level */
	intrspec[0] = 2;
	intrspec[1] = 5;
	intrspec[2] = ncrp->n_irq;
	if (ddi_ctlops(dip, dip, DDI_CTLOPS_XLATE_INTRS, (caddr_t)intrspec
			  , ddi_get_parent_data(dip)) != DDI_SUCCESS) {
		NDBG4(("ncr_xlate_irq_no_sid: interrupt create failed\n"));
		return (FALSE);
	}
	ncrp->n_inumber = 0;
	NDBG4(("ncr_xlate_irq_no_sid: okay\n"));
	return (TRUE);
}

/*
 * For self-ID configurations, the framework does all the work.
 */
bool_t
ncr_xlate_irq_sid( ncr_t *ncrp )
{
	ncrp->n_inumber = 0;
	NDBG4(("ncr_xlate_irq_sid: okay\n"));
	return (TRUE);
}

bool_t
ncr_intr_init(	dev_info_t	*dip,
		ncr_t 		*ncrp,
		caddr_t		intr_arg )
{

	/* map the irq into the interrupt spec index number */
	if (!NCR_XLATE_IRQ(ncrp)) {
		NDBG4(("ncr_intr_init: xlate failed\n"));
		return (FALSE);
	}

	if (!add_intr(dip, ncrp->n_inumber, &ncrp->n_iblock
		       , &ncrp->n_mutex, "ncrs mutex", ncr_intr
		       , intr_arg)) {
		NDBG4(("ncr_intr_init: add_intr failed\n"));
		return (FALSE);
	}
	NDBG4(("ncr_intr_init: add_intr okay\n"));
	return (TRUE);

}
