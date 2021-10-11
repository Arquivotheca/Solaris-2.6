/*
 * Copyright (c) 1995 by Sun Microsystems, Inc.
 */

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

#pragma	ident	"@(#)ncr.c	1.13	96/02/02 SMI"

#include <sys/dktp/ncrs/ncr.h>

/**/
/**						| (1<<15) | (1<<16));
/** ulong	ncr_debug_flags = 0x000fffff & ~((1<<8) | (1<<13) | (1<<14) 
/**						| (1<<15) | (1<<16));
/**/
ulong	ncr_debug_flags = 0x0;
int	ncr_forceload = 0;

struct dev_ops	ncr_ops = {
	DEVO_REV,		/* devo_rev, */
	0,			/* refcnt  */
	ddi_no_info,		/* info */
	ncr_identify,		/* identify */
	ncr_probe,		/* probe */
	ncr_attach,		/* attach */
	ncr_detach,		/* detach */
	nodev,			/* reset */
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
	"NCR 53Cx10 SCSI Host Bus Adapter Driver",   /* Name of the module. */
	&ncr_ops,	/* driver ops */
};

static struct modlinkage modlinkage = {
	MODREV_1, (void *)&modldrv, NULL
};

/*
 * Local static data
 */
kmutex_t ncr_global_mutex;

/* The loadable-module _init(9E) entry point */

int
_init(void)
{
	int	status;

#if defined(NCR_DEBUG)
debug_enter("\n\n\nNCR HBA INIT\n\n");
#endif
	if ((status = scsi_hba_init(&modlinkage)) != 0) {
		return (status);
	}

	mutex_init(&ncr_global_mutex, "NCR Global Mutex",
		MUTEX_DRIVER, (void *)NULL);

	if ((status = mod_install(&modlinkage)) != 0) {
		mutex_destroy(&ncr_global_mutex);
		scsi_hba_fini(&modlinkage);
	}
	return (status);
}

/* The loadable-module _fini(9E) entry point */

int
_fini(void)
{
	int	  status;

	NDBG1(("ncr_fini\n"));
	/* XXX KLUDGE do not unload when forceloaded from DU distribution */
	if (ncr_forceload > 1)
		return (1);

	if ((status = mod_remove(&modlinkage)) == 0) {
		mutex_destroy(&ncr_global_mutex);
		scsi_hba_fini(&modlinkage);
	}
	return (status);
}

/* The loadable-module _info(9E) entry point */

int
_info(struct modinfo *modinfop)
{
#if defined(NCR_DEBUG)
debug_enter("\n\n\nNCR HBA INFO\n\n");
#endif
	NDBG0(("ncr_info\n"));

	return (mod_info(&modlinkage, modinfop));
}
