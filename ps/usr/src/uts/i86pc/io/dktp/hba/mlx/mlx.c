/*
 * Copyright (c) 1996, Sun Microsystems, Inc.
 * All Rights Reserved.
 */

#pragma	ident	"@(#)mlx.c	1.41	96/05/27 SMI"

/*
 * This file is a product of Sun Microsystems, Inc. and is provided for
 * unrestricted use provided that this legend is included on all tape
 * media and as a part of the software program in whole or part.  Users
 * may copy or modify this file without charge, but are not authorized to
 * license or distribute it to anyone else except as part of a product
 * or program developed by the user.
 *
 * THIS FILE IS PROVIDED AS IS WITH NO WARRANTIES OF ANY KIND INCLUDING THE
 * WARRANTIES OF DESIGN, MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE, OR ARISING FROM A COURSE OF DEALING, USAGE OR TRADE PRACTICE.
 *
 * This file is provided with no support and without any obligation on the
 * part of Sun Microsystems, Inc. to assist in its use, correction,
 * modification or enhancement.
 *
 * SUN MICROSYSTEMS, INC. SHALL HAVE NO LIABILITY WITH RESPECT TO THE
 * INFRINGEMENT OF COPYRIGHTS, TRADE SECRETS OR ANY PATENTS BY THIS FILE
 * OR ANY PART THEREOF.
 *
 * In no event will Sun Microsystems, Inc. be liable for any lost revenue
 * or profits or other special, indirect and consequential damages, even
 * if Sun has been advised of the possibility of such damages.
 *
 * Sun Microsystems, Inc.
 * 2550 Garcia Avenue
 * Mountain View, California  94043
 */


#include <sys/dktp/mlx/mlx.h>

ulong	mlx_debug_flags = 0;
int	mlx_forceload = 0;

struct dev_ops	mlx_ops = {
	DEVO_REV,		/* devo_rev, */
	0,			/* refcnt  */
	ddi_no_info,		/* info */
	nulldev,		/* identify */
	mlx_probe,		/* probe */
	mlx_attach,		/* attach */
	mlx_detach,		/* detach */
	mlx_flush_cache,	/* reset */
	(struct cb_ops *)0,	/* driver operations */
	NULL,			/* bus operations */
	NULL			/* power mgmt */
};

/*
 * Make the system load these modules whenever this driver loads.  This
 * is required for constructing the set of modules needed for boot; they
 * must all be loaded before anything initializes.  Just use this
 * line as-is in your driver.
 */
#ifdef	PCI_DDI_EMULATION
char _depends_on[] = "misc/xpci misc/scsi";
#else
char _depends_on[] = "misc/scsi";
#endif

/*
 * This is the loadable module wrapper.
 */

extern struct mod_ops mod_driverops;

static struct modldrv modldrv = {
	&mod_driverops,	/* Type of module. This one is a driver */
	"Mylex DXC960 SCSI Host Adapter Driver",	/* module's name */
	&mlx_ops,	/* driver ops */
};

static struct modlinkage modlinkage = {
	MODREV_1, (void *)&modldrv, NULL
};

/*
 * Local static data
 */
kmutex_t mlx_global_mutex;

int mlx_pgsz = 0;			/* global size variables */
int mlx_pgmsk;
int mlx_pgshf;
mlx_t *mlx_cards = NULL;		/* head of the global mlx structures */

#ifdef MSCSI_FEATURE
/*
 * The mscsi_bus routines are used when the mscsi_bus nexus
 * driver is incorporated to create instances for each scsi bus.
 * Don't use these on a single scsi-bus controller.
 */

/* Wrapper for scsi_hba_ctlops to allow creation of mscsi_bus children */

/*ARGSUSED*/
static int
mscsi_hba_ctlops(dev_info_t *dip, dev_info_t *rdip,
	ddi_ctl_enum_t ctlop, void *arg, void *result)
{
	struct bus_ops	*hba_bus_ops;
	struct mbus_ops	*hba_mbus_ops;
	dev_info_t	*child_dip;
	int 		mscsi_bus;
	int 		len;

	scsi_hba_tran_t *hba_tran;		/* mlx-specific variables */
	mlx_hba_t	*hba;

	/*
	 * Intercept bus operations for special mscsi_bus nodes
	 */
	if ((ctlop == DDI_CTLOPS_REPORTDEV || ctlop == DDI_CTLOPS_INITCHILD ||
	    ctlop == DDI_CTLOPS_UNINITCHILD)) {

		/* determine child_dip */
		if (ctlop == DDI_CTLOPS_REPORTDEV)
			child_dip = rdip;
		else
			child_dip = (dev_info_t *)arg;
		if (!dip)
			return (DDI_FAILURE);

		/* determine mscsi_bus number */
		len = sizeof (mscsi_bus);
		if (ddi_getlongprop_buf(DDI_DEV_T_ANY,
		    child_dip, DDI_PROP_DONTPASS, MSCSI_BUSPROP,
		    (caddr_t)&mscsi_bus, &len) != DDI_PROP_SUCCESS)
			mscsi_bus = -1;

		/* set mlx-specific initialization variables */
		hba_tran = (scsi_hba_tran_t *)ddi_get_driver_private(dip);
		hba = MLX_SCSI_TRAN2HBA(hba_tran);

		if (strcmp(MSCSI_NAME, ddi_get_name(child_dip)) == 0) {
			switch (ctlop) {
			case DDI_CTLOPS_REPORTDEV:

				cmn_err(CE_CONT, "?mscsi-device: %s%d\n",
					ddi_get_name(rdip),
					ddi_get_instance(rdip));
				return (DDI_SUCCESS);

			case DDI_CTLOPS_INITCHILD:

				/*
				 * Creating a mscsi_bus child node representing
				 * a scsi-bus. This is where to confirm the
				 * bus exists. scsi-bus initialization can
				 * also occur here. Alternatively, the hba
				 * driver can arrange for the mscsi_bus
				 * child to callback the parent's devops
				 * entries (via the MSCSI_CALLPROP
				 *  property) to perform initialization.
				 */
				if (!hba->mlx || mscsi_bus < 0 ||
				    hba->mlx->nchn <= mscsi_bus)
					return (DDI_FAILURE);

				/* perform a generic initchild */
				return (impl_ddi_sunbus_initchild(child_dip));

			case DDI_CTLOPS_UNINITCHILD:

				/*
				 * Creating a mscsi_bus child node representing
				 * a scsi-bus. This is where to confirm the
				 * bus exists. scsi-bus initialization can
				 * also occur here. Alternatively, the hba
				 * driver can arrange for the mscsi_bus
				 * child to callback the parent's devops
				 * entries (via the MSCSI_CALLPROP
				 *  property) to perform initialization.
				 */

				/* perform generic removechild */
				impl_ddi_sunbus_removechild(child_dip);

				return (DDI_SUCCESS);
			}
		}

#ifndef	PCI_DDI_EMULATION
		/*
		 * mlx-specific: raid system drive devices are not
		 * associated with a specfic channel, and will
		 * attach directly to the hba parent node.
		 * Other scsi devices will attach via mscsi.
		 */
		if (ctlop == DDI_CTLOPS_INITCHILD &&
		    strcmp("cmdk", ddi_get_name(child_dip)))
			return (DDI_FAILURE);
#endif
	}

	/*
	 * Otherwise pass-thru request
	 */
	hba_mbus_ops = (struct mbus_ops *)DEVI(dip)->devi_ops->devo_bus_ops;
	if (hba_mbus_ops) {
		hba_bus_ops = hba_mbus_ops->m_bops;
		if (hba_bus_ops)
			return ((hba_bus_ops->bus_ctl)
				(dip, rdip, ctlop, arg, result));
	}
	return (DDI_FAILURE);
}

/* Installation of bus_ops wrapper, called by HBA from _init() */

static int
mscsi_hba_init(struct modlinkage *modlp)
{
	struct dev_ops *hba_dev_ops;
	struct mbus_ops *hba_mbus_ops;

	/*
	 * Get the devops structure of the hba,
	 * and put our busops vector and bus_ctl routine in its place.
	 */
	hba_dev_ops = ((struct modldrv *)
		(modlp->ml_linkage[0]))->drv_dev_ops;
	hba_mbus_ops = (struct mbus_ops *)kmem_zalloc(sizeof (struct mbus_ops),
		KM_SLEEP);
	if (!hba_mbus_ops)
		return (1);

	/*
	 * copy private bus_ops, saving original, and wrap bus_ctl function
	 */
	bcopy((caddr_t)hba_dev_ops->devo_bus_ops, (caddr_t)hba_mbus_ops,
		sizeof (struct bus_ops));
	hba_mbus_ops->m_bops = hba_dev_ops->devo_bus_ops;
	hba_mbus_ops->m_ops.bus_ctl = mscsi_hba_ctlops;
	hba_dev_ops->devo_bus_ops = (struct bus_ops *)hba_mbus_ops;

	return (0);
}

/* Removal of bus_ops wrapper, called by HBA from _fini() */

static void
mscsi_hba_fini(struct modlinkage *modlp)
{
	struct dev_ops *hba_dev_ops;
	struct bus_ops *hba_bus_ops;
	struct mbus_ops *hba_mbus_ops;

	/*
	 * Release private copied busops structure
	 */
	hba_dev_ops = ((struct modldrv *)
		(modlp->ml_linkage[0]))->drv_dev_ops;
	hba_mbus_ops = (struct mbus_ops *)hba_dev_ops->devo_bus_ops;
	if (hba_mbus_ops) {
		hba_bus_ops = hba_mbus_ops->m_bops;
		kmem_free(hba_mbus_ops, sizeof (struct mbus_ops));
		hba_dev_ops->devo_bus_ops = hba_bus_ops;
	}
}
#endif	/* MSCSI_FEATURE */

/* The loadable-module _init(9E) entry point */

int
_init(void)
{
	int	status;

#if defined(MLX_DEBUG)
if (mlx_debug_flags & 0x1000)
	debug_enter("\n\n\nMLX HBA INIT\n\n");
#endif
	if ((status = scsi_hba_init(&modlinkage)) != 0) {
		return (status);
	}

#ifdef	MSCSI_FEATURE
	/* mscsi_bus drivers only */
	if ((status = mscsi_hba_init(&modlinkage)) != 0) {
		return (status);
	}
#endif

	mutex_init(&mlx_global_mutex, "MLX Global Mutex",
		MUTEX_DRIVER, (void *)NULL);

	if (!mlx_pgsz) {
		register int i;
		int len;

		/*
		 * Set page size at init
		 */
		mlx_pgsz = ptob(1L);
		mlx_pgmsk = mlx_pgsz - 1;
		for (i = mlx_pgsz, len = 0; i > 1; len++)
			i >>= 1;
		mlx_pgshf = len;
	}

	if ((status = mod_install(&modlinkage)) != 0) {
		mutex_destroy(&mlx_global_mutex);
		scsi_hba_fini(&modlinkage);
	}
	return (status);
}

/* The loadable-module _fini(9E) entry point */

int
_fini(void)
{
	int	  status;

	MDBG1(("mlx_fini\n"));
	/* XXX KLUDGE do not unload when forceloaded from DU distribution */
	if (mlx_forceload > 1)
		return (1);

	if ((status = mod_remove(&modlinkage)) == 0) {
		mutex_destroy(&mlx_global_mutex);
#ifdef	MSCSI_FEATURE
		mscsi_hba_fini(&modlinkage);	/* mscsi_bus drivers only */
#endif
		scsi_hba_fini(&modlinkage);
	}
	return (status);
}

/* The loadable-module _info(9E) entry point */

int
_info(struct modinfo *modinfop)
{
#if defined(MLX_DEBUG)
if (mlx_debug_flags & 0x1000)
	debug_enter("\n\n\nMLX HBA INFO\n\n");
#endif
	MDBG0(("mlx_info\n"));

	return (mod_info(&modlinkage, modinfop));
}
