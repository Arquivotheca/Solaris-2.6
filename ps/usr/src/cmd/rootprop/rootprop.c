/*
 * Root Properties access driver
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

#ifndef lint
static char     sccsid[] = "@(#)rootprop.c 1.2 93/11/18 Copyright 1993 Sun Microsystems";
#endif

/*
 * This device allows applications to get the size and value of any 
 * property of the root. It has no read or write functionality.
 */

#include <sys/types.h>
#include <sys/param.h>
#include <sys/file.h>
#include <sys/conf.h>
#include <sys/uio.h>
#include <sys/map.h>
#include <sys/debug.h>
#include <sys/modctl.h>
#include <sys/kmem.h>
#include <sys/cmn_err.h>
#include <sys/open.h>
#include <sys/stat.h>
#include <sys/ddi.h>
#include <sys/sunddi.h>

#include "rootprop_io.h"
#include "rootprop_reg.h"

static	void *state_head;	/* opaque handle top of state structs */

/*
 * These are the entry points into our driver that are called when the
 * driver is loaded, during a system call, or in response to an interrupt.
 */
static	int	rootprop_getinfo(dev_info_t *dip, ddi_info_cmd_t infocmd, void *arg,
		    void **result);
static	int	rootprop_identify(dev_info_t *dip);
static	int	rootprop_probe(dev_info_t *dip);
static	int	rootprop_attach(dev_info_t *dip, ddi_attach_cmd_t cmd);
static	int	rootprop_open(dev_t *dev, int openflags, int otyp, cred_t *credp);
static	int	rootprop_ioctl(dev_t dev, int cmd, int arg, int flag, cred_t *credp,
		    int *rvalp);
static	int	rootprop_detach(dev_info_t *dip, ddi_detach_cmd_t cmd);

/*
 * When our driver is loaded or unloaded, the system calls our _init or
 * _fini routine with a modlinkage structure.  The modlinkage structure
 * contains:
 *
 *	modlinkage->
 *		modldrv->
 *			dev_ops->
 * 				cb_ops
 *
 * cb_ops contains the normal driver entry points and replaces
 * the cdevsw & bdevsw structures in previous releases.
 *
 * dev_ops contains, in addition to the pointer to cb_ops, the routines
 * that support loading and unloading our driver.
 */
static struct cb_ops	rootprop_cb_ops = {
	rootprop_open,
	nulldev,
	nodev,			/* not a block driver	*/
	nodev,			/* no print routine	*/
	nodev,			/* no dump routine	*/
	nodev,			/* no read routine; ioctls only. */
	nodev,			/* no write routine; ioctls only. */
	rootprop_ioctl,
	nodev,			/* no devmap routine	*/
	nodev,			/* no mmap routine	*/
	nodev,			/* no segmap routine	*/
	nochpoll,		/* no chpoll routine	*/
	ddi_prop_op,
	0,			/* not a STREAMS driver	*/
	D_NEW | D_MP,		/* safe for multi-thread/multi-processor */
};

static struct dev_ops rootprop_ops = {
	DEVO_REV,		/* DEVO_REV indicated by manual	*/
	0,			/* device reference count	*/
	rootprop_getinfo,
	rootprop_identify,
	rootprop_probe,
	rootprop_attach,
	rootprop_detach,
	nodev,			/* device reset routine		*/
	&rootprop_cb_ops,
	(struct bus_ops *)0,	/* bus operations		*/
};

extern	struct	mod_ops mod_driverops;
static	struct modldrv modldrv = {
	&mod_driverops,
	"Root Properties access driver, version @(#) rootprop.c",
	&rootprop_ops,
};

static	struct modlinkage modlinkage = {
	MODREV_1,		/* MODREV_1 indicated by manual */
	(void *)&modldrv,
	NULL,			/* termination of list of linkage structures */
};

/*
 * _init, _info, and _fini support loading and unloading the driver.
 */
int
_init(void)
{
	register int	error;

	if ((error = ddi_soft_state_init(&state_head, sizeof (Rootprop), 1)) != 0)
		return (error);

	if ((error = mod_install(&modlinkage)) != 0)
		ddi_soft_state_fini(&state_head);

	return (error);
}

int
_info(struct modinfo *modinfop)
{
	return (mod_info(&modlinkage, modinfop));
}

int
_fini(void)
{
	int status;

	if ((status = mod_remove(&modlinkage)) != 0)
		return (status);

	ddi_soft_state_fini(&state_head);

	return (status);
}

/*
 * When our driver is loaded, rootprop_identify() is called with a dev_info_t
 * full of information from the driver.conf(4) file.
 */

static int
rootprop_identify(dev_info_t *dip)
{
	char *dev_name;
	dev_name = ddi_get_name(dip);

	if (strcmp(dev_name, ROOTPROP_NAME) == 0) {
		return (DDI_IDENTIFIED);
	} else {
		return (DDI_NOT_IDENTIFIED);
	}
}

static int 
rootprop_probe(dev_info_t *dip)
{
	/* 
 	 * We have no way to check if the rootprop device actually exists,
	 * so return DONTCARE, which will cause the system to proceed
	 * as if the device *does* exist.
	 */
	return (DDI_PROBE_DONTCARE);
}

/*
 * rootprop_attach gets called if rootprop_identify and rootprop_probe succeed.
 *
 * This is a very paranoid attach routine.  We take all the knowledge we
 * have about our board and check it against what has been filled in for
 * us from our driver.conf(4) file.
 */
static int
rootprop_attach(dev_info_t *dip, ddi_attach_cmd_t cmd)
{
	int			instance;
	register Rootprop		*rootprop_p;
	int			result;
	int			error;

	if (cmd != DDI_ATTACH)
		return (DDI_FAILURE);

	instance = ddi_get_instance(dip);

	if (ddi_soft_state_zalloc(state_head, instance) != 0)
		return (DDI_FAILURE);

	rootprop_p = (Rootprop *)ddi_get_soft_state(state_head, instance);
	ddi_set_driver_private(dip, (caddr_t)rootprop_p);
	rootprop_p->dip = dip;

	/*
	 * ddi_create_minor_node creates an entry in an internal kernel
	 * table; the actual entry in the file system is created by
	 * drvconfig(1) when you run add_drv(1);
	 */
	if (ddi_create_minor_node(dip, ddi_get_name(dip), S_IFCHR, instance,
	    NULL, NULL) == DDI_FAILURE) {
		ddi_soft_state_free(state_head, instance);
		return (DDI_FAILURE);
	}

	ddi_report_dev(dip);

	return (DDI_SUCCESS);
}

/*
 * This is a pretty generic getinfo routine as described in the manual.
 */
/*ARGSUSED*/
static int
rootprop_getinfo(dev_info_t *dip, ddi_info_cmd_t infocmd, void *arg, void **result)
{
	register int 	error;
	register Rootprop	*rootprop_p;

	switch (infocmd) {
	case DDI_INFO_DEVT2DEVINFO:
		rootprop_p = (Rootprop *)ddi_get_soft_state(state_head,
			    getminor((dev_t)arg));
		if (rootprop_p == NULL) {
			error = DDI_FAILURE;
		} else {
			*result = rootprop_p->dip;
			error = DDI_SUCCESS;
		}
		break;
	case DDI_INFO_DEVT2INSTANCE:
		*result = (void *)getminor((dev_t)arg);
		error = DDI_SUCCESS;
		break;
	default:
		error = DDI_FAILURE;
	}

	return (error);
}

/*
 * When our driver is unloaded, rootprop_detach cleans up and frees the
 * resources we allocated in rootprop_attach.
 */
static int
rootprop_detach(dev_info_t *dip, ddi_detach_cmd_t cmd)
{
	register Rootprop	*rootprop_p;	/* will point to this */
	int		instance;

	if (cmd != DDI_DETACH)
		return (DDI_FAILURE);

	instance = ddi_get_instance(dip);
	rootprop_p = (Rootprop *)ddi_get_soft_state(state_head, instance);

	/*
	 * Remove the minor node created in attach
	 */
	ddi_remove_minor_node(dip, NULL);

	ddi_soft_state_free(state_head, instance);

	return (DDI_SUCCESS);
}

/*
 * rootprop_open is called in response to the open(2) system call
 */
/*ARGSUSED*/
static	int
rootprop_open(dev_t *dev, int openflags, int otyp, cred_t *credp)
{
	int		retval = 0;
	register Rootprop	*rootprop_p;

	rootprop_p = (Rootprop *)ddi_get_soft_state(state_head, getminor(*dev));

	/*
	 * Verify instance structure
	 */
	if (rootprop_p == NULL)
		return (ENXIO);

	/*
	 * Verify we are being opened as a character device
	 */
	if (otyp != OTYP_CHR)
		return (EINVAL);

	return (retval);
}

/*ARGSUSED*/
static	int
rootprop_ioctl(dev_t dev, int cmd, int arg, int flag, cred_t *credp, int *rvalp)
{
	register Rootprop	*rootprop_p;
	rootprop_arg_t callargs;
	int		retval = 0;
	int	len;
	caddr_t u_pname;
	caddr_t u_pbuf;

	rootprop_p = (Rootprop *)ddi_get_soft_state(state_head, getminor(dev));

	/*
	 * Get the argument list.
	 */
	if (copyin( (caddr_t)arg, (caddr_t)&callargs, sizeof(rootprop_arg_t))
		== -1) {
		return(EFAULT);
	}
	/*
	 * The property should always be there, and the len should always be
	 * positive.
	 */
	u_pname = (caddr_t)callargs.pname;
	if (u_pname && (callargs.pnamelen > 0) ) {
		if (!(callargs.pname = kmem_zalloc( callargs.pnamelen , (int)NULL ))) {
			return(ENOMEM);
		}
		if (copyin( u_pname, (caddr_t)callargs.pname, callargs.pnamelen )
		   == -1) {
			return(EFAULT);
		}
	} else {
		return(EINVAL);
	}

	switch (cmd) {
	    case ROOTPROP_LEN:
			retval = ddi_getproplen( DDI_DEV_T_ANY, ddi_root_node(), (int)NULL, 
		         callargs.pname, &callargs.pbuflen);
			switch(retval) {
			case DDI_PROP_SUCCESS:
				retval = 0;
				break;
			case DDI_PROP_NOT_FOUND:
				retval = ENODEV;
				break;
			default:
				retval =  EIO;
				break;
			}
			callargs.pname = u_pname;
			if (copyout( (caddr_t)&callargs, (caddr_t)arg, 
			    sizeof(rootprop_arg_t))
				== -1) {
				retval = EFAULT;
			}
			break;
	    case ROOTPROP_PROP:
			/*
		 	* We will be passing out a buffer. Set it up here. No need to
		 	* copyin the current contents.
		 	*/
			u_pbuf = (caddr_t)callargs.pbuf;
			if (u_pbuf && (callargs.pbuflen > 0) ) {
				if (!(callargs.pbuf = 
			     	kmem_zalloc( callargs.pbuflen , (int)NULL ))) {
					kmem_free(callargs.pname, callargs.pnamelen );
					return(ENOMEM);
				}
			} else {
				kmem_free(callargs.pname, callargs.pnamelen );
				return(EINVAL);
			}
			retval = ddi_getlongprop_buf( DDI_DEV_T_ANY, ddi_root_node(), 
		             	(int)NULL,
		             	(char *)callargs.pname, 
				     	(caddr_t)callargs.pbuf,
					 	(int *)&callargs.pbuflen);
			switch(retval) {
			case DDI_PROP_SUCCESS:
				retval = 0;
				if (copyout( (caddr_t)callargs.pbuf, u_pbuf, callargs.pbuflen)
			   	== -1 ) {
					retval = EFAULT;
				}
				break;
			case DDI_PROP_NOT_FOUND:
				retval = ENODEV;
				break;
			default:
				retval =  EIO;
				break;
			}
			kmem_free(callargs.pbuf, callargs.pbuflen );
			kmem_free(callargs.pname, callargs.pnamelen );
			break;
		default:
			retval = EINVAL;
			break;
	}

	return (retval);
}

