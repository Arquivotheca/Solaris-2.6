#ifndef lint
#pragma ident "@(#)store_bootobj.c 1.13 96/08/08 SMI"
#endif

/*
 * Copyright (c) 1996 by Sun Microsystems, Inc. All rights reserved.
 */
/*
 * Module:	store_bootobj.c
 * Group:	libspmistore
 * Description: This module contains all interfaces for accessing the boot
 *		object
 */

#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <device_info.h>
#include "spmistore_lib.h"
#include "spmicommon_api.h"

/* local data structures */

typedef struct {
	char *	disk;
	int	device;
	char	devtype;
	u_int	flags;
} BootobjState;

/* public prototypes */

int		BootobjCommit(void);
int		BootobjRestore(Label_t);
int		BootobjGetAttribute(Label_t, ...);
int		BootobjSetAttribute(Label_t, ...);
int		BootobjCompare(Label_t, Label_t, int);
int		BootobjConflicts(Label_t, char *, int);
int		BootobjIsExplicit(Label_t, BootobjAttrType);

/* internal prototypes */

int		BootobjSetAttributePriv(Label_t, ...);
int		BootobjInit(void);

/* private prototypes */

static void		BootobjCopy(Label_t, Label_t);
static void		BootobjReset(Label_t);
static BootobjState *	BootobjGetRef(Label_t);
static void		BootobjstateDuplicate(BootobjState *, BootobjState *);
static void		BootobjstateFree(BootobjState *);
static int		coreBootobjGetAttribute(Label_t, int, va_list);
static int		coreBootobjSetAttribute(Label_t, int, va_list);

/* local constants and macros */

#define	BootobjGetFlag(x, y)	((x)->flags & (y) ? 1 : 0)
#define	BootobjSetFlag(x, y)	((x)->flags |= (y))
#define	BootobjClearFlag(x, y)	((x)->flags &= ~(y))

/* bit values for boot object flag field */

#define	BF_DISK_EXPLICIT	0x0001
#define	BF_DEVICE_EXPLICIT	0x0002
#define	BF_PROM_UPDATE		0x0004
#define	BF_PROM_UPDATEABLE	0x0008

/* ---------------------- public functions ----------------------- */

/*
 * Function:	BootobjCompare
 * Description:	Compare each of the data elements for two states of the boot
 *		object in search of differences. The user may specify if the
 *		matches must be exact, or if wildcard (undefined) values in
 *		one state are considered to match any value for the same
 *		data element in the comparitor state.
 * Scope:	public
 * Parameters:	first	  [RO] (Label_t)
 *			  Boot object configuration state used for comparison.
 *			  Valid values are:
 *
 *			  CFG_CURRENT	current boot object state
 *			  CFG_COMMIT	committed boot object state
 *			  CFG_EXISING	existing boot object state
 *
 *		second	  [RO] (Label_t)
 *			  Boot object configuration state used for comparison.
 *			  Valid values are:
 *
 *			  CFG_CURRENT	current boot object state
 *			  CFG_COMMIT		committed boot object state
 *			  CFG_EXISING	existing boot object state
 *
 *		explicit  [RO] (int)
 *			  State whether or not the comparison should allow
 *			  for wildcarding (0), or should be explicit (1).
 * Return:	D_OK	  The object states are considered the same.
 *		D_FAILED  At least one data element in the two states differs.
 */
/*ARGSUSED0*/
int
BootobjCompare(Label_t first, Label_t second, int explicit)
{
	char	fdisk[32];
	char	sdisk[32];
	int	fdevice;
	int	sdevice;

	if (first != CFG_CURRENT && first != CFG_COMMIT && first != CFG_EXIST)
		return (D_FAILED);

	if (second != CFG_CURRENT &&
			second != CFG_COMMIT && second != CFG_EXIST)
		return (D_FAILED);

	if (first == second)
		return (D_OK);

	(void) BootobjGetAttribute(first,
		BOOTOBJ_DISK, &fdisk,
		BOOTOBJ_DEVICE, &fdevice,
		NULL);
	(void) BootobjGetAttribute(second,
		BOOTOBJ_DISK, &sdisk,
		BOOTOBJ_DEVICE, &sdevice,
		NULL);

	if (explicit) {

		if (!streq(fdisk, sdisk))
			return (D_FAILED);

		if (fdevice != sdevice)
			return (D_FAILED);
	} else {
		if (!streq(fdisk, sdisk) &&
				fdisk[0] != '\0' && sdisk[0] != '\0')
			return (D_FAILED);

		if (fdevice != sdevice && fdevice != -1 && sdevice != -1)
			return (D_FAILED);
	}

	return (D_OK);
}

/*
 * Function:	BootobjCommit
 * Description: Update the committed state of the boot object with the
 *		configuration of the current state. No boot object state
 *		consistency checks are made.
 * Scope:	public
 * Parameters:	none
 * Return:	D_OK	boot object configuration committed successfully
 */
int
BootobjCommit(void)
{
	BootobjCopy(CFG_COMMIT, CFG_CURRENT);
	return (D_OK);
}

/*
 * Function:	BootobjRestore
 * Description:	Restore either the committed or existing state fo the boot
 *		object to the current state. No consistency checks relative to
 *		the disk object list are made.
 * Scope:	public
 * Parameters:	state	[RO] (Label_t)
 *			Identify boot object state from which to copy
 *			configuration information into the current state. Valid
 *			values are:
 *			    CFG_COMMIT	restore from committed state
 *			    CFG_EXIST	restore from existing state
 * Return:	D_OK	current boot object configuration restored from the
 *			committed or existing state successfully
 *		D_BADARG    invalid source state specifier
 */
int
BootobjRestore(Label_t state)
{
	/* validate parameters */
	if (state != CFG_COMMIT && state != CFG_EXIST)
		return (D_BADARG);

	/* copy from the source state */
	BootobjCopy(CFG_CURRENT, state);
	return (D_OK);
}

/*
 * Function:	BootobjGetAttribute
 * Description:	Retrieve the boot object attribute information associated
 *		with a boot object state. Attribute/value pairs are processed
 *		left-to-right in as they appear in the parameter list.
 * Scope:	public
 * Parameters:	state	[RO] (Label_t)
 *			Identify boot object state from which to retrieve
 *			attribute information. Valid values are:
 *			  CFG_CURRENT	current state info
 *			  CFG_COMMIT	committed state info
 *			  CFG_EXIST	existing state info
 *		...	Null terminated list of attribute value pairs.
 *			Attributes and values are:
 *			    BOOTOBJ_DISK		char *	(char[32])
 *			    BOOTOBJ_DISK_EXPLICIT	int *	(&int)
 *			    BOOTOBJ_DEVICE		int *	(&int)
 *			    BOOTOBJ_DEVICE_TYPE		char *	(&char)
 *			    BOOTOBJ_DEVICE_EXPLICIT	int *	(&int)
 *			    BOOTOBJ_PROM_UPDATE		int *	(&int)
 *			    BOOTOBJ_PROM_UPDATEABLE	int *	(&int)
 * Return:	D_OK	 Attributes retrieved successfully.
 *		D_BADARG Invalid state, attribute, or value specifier.
 */
int
BootobjGetAttribute(Label_t state, ...)
{
	va_list		ap;
	int		status;

	/* validate parameters */
	if (state != CFG_CURRENT && state != CFG_COMMIT && state != CFG_EXIST)
		return (D_BADARG);

	va_start(ap, state);
	status = coreBootobjGetAttribute(state, 0, ap);
	va_end(ap);
	return (status);
}

/*
 * Function:	BootobjSetAttribute
 * Description:	Update current state of boot object application modifiable
 *		attributes. Attribute/value pairs are processed left-to-right
 *		as they appear in the parameter list. Some attributes, if
 *		modified, have the side effect of updating associated
 *		attributes. All constraints and side-effects are noted with
 *		each attribute. If an update fails while processing an
 *		attribute the state of the current boot object prior to the
 *		call is restored.
 * Scope:	public
 * Parameters:	state	[RO] (Label_t)
 *			Identify boot object state from which to retrieve
 *			configuration information. Valid values are:
 *				CFG_CURRENT	current state info
 *			(NOTE: required for varargs list initialization)
 *		...	Null terminated list of attribute value pairs.
 *			Attributes and values are:
 *
 *			BOOTOBJ_DISK		 char[32] (""|NULL|<disk>)
 *			BOOTOBJ_DISK_EXPLICIT	 int (0|1)
 *			BOOTOBJ_DEVICE		 int (device index)
 *			BOOTOBJ_DEVICE_EXPLICIT  int (0|1)
 *			BOOTOBJ_PROM_UPDATE	 int (0|1)
 *
 * Return:	D_OK	 Attributes updated successfully.
 *		D_BADARG Invalid state, attribute, or value specifier.
 */
int
BootobjSetAttribute(Label_t state, ...)
{
	BootobjState	save;
	va_list		ap;
	int		status;

	/* validate parameters */
	if (state != CFG_CURRENT)
		return (D_BADARG);

	/* save the state for possible restoration */
	BootobjstateDuplicate(&save, BootobjGetRef(state));
	va_start(ap, state);
	status = coreBootobjSetAttribute(state, 0, ap);
	va_end(ap);
	/* restore the original state if there was an update failure */
	if (status != D_OK)
		BootobjstateDuplicate(BootobjGetRef(state), &save);
	BootobjstateFree(&save);
	return (status);
}

/*
 * Function:	BootobjConflicts
 * Description:	Assess whether the disk and device specified conflicts with
 *		the configuration requested.
 * Scope:	public
 * Parameters:	state	[RO] (Label_t)
 *			Identify the boot object state with which to compare the
 *			disk/device configuration information. Valid values are:
 *			    CFG_CURRENT	    current state info
 *			    CFG_COMMIT	    committed state info
 *			    CFG_EXIST	    existing state info
 *		disk	[RO] (char[32])
 *			Name of disk being compared against the specified state
 *			(e.g. c0t0d0). NULL means wildcard.
 *		device	[RO] (int)
 *			Device index being compared against the specified state.
 *			-1 means wildcard.
 * Return:	0	There are no conflicts with the specified disk/device
 *			relative to the specified boot object state.
 *		1	There are conflicts with the specified disk/device
 *			wrt the specified boot object state.
 */
int
BootobjConflicts(Label_t state, char *disk, int device)
{
	char 	dname[32];
	int	dev;
	int	edisk;
	int	edev;

	/* validate parameters */
	if (state != CFG_CURRENT && state != CFG_COMMIT && state != CFG_EXIST)
		return (0);

	/*
	 * check to see if there are any conflicts with the request and
	 * explicit boot object configurations
	 */
	if (BootobjGetAttribute(state,
			BOOTOBJ_DISK, dname,
			BOOTOBJ_DISK_EXPLICIT, &edisk,
			BOOTOBJ_DEVICE, &dev,
			BOOTOBJ_DEVICE_EXPLICIT, &edev,
			NULL) == D_OK) {
		if (edisk == 1 && disk != NULL && !streq(dname, disk))
			return (1);

		if (edev == 1 && device != -1 && dev != device)
			return (1);
	}

	return (0);
}

/*
 * Function:	BootobjIsExplicit
 * Description:	Boolean function to quickly assess if the disk or device of
 *		a given state of the boot object are explicitly set.
 * Scope:	public
 * Parameters:	state	[RO] (Label_t)
 *			Identify boot object state from which to retrieve
 *			configuration information. Valid values are:
 *			    CFG_CURRENT	    current state info
 *			    CFG_COMMIT	    committed state info
 *			    CFG_EXIST	    existing state info
 *		attr	[RO] (BootobjAttrType)
 *			Boot object attribute type for explicit attributes.
 *			Valid values are:
 *			    BOOTOBJ_DISK_EXPLICIT
 *			    BOOTOBJ_DEVICE_EXPLICIT
 * Return:	1	The explicit attribute is set.
 *		0	The explicit attribute is unset.
 */
int
BootobjIsExplicit(Label_t state, BootobjAttrType attr)
{
	int	val;

	if (state != CFG_CURRENT && state != CFG_COMMIT && state != CFG_EXIST)
		return (0);

	if (attr != BOOTOBJ_DISK_EXPLICIT && attr != BOOTOBJ_DEVICE_EXPLICIT)
		return (0);

	if (BootobjGetAttribute(state, attr, &val, NULL) == D_OK && val == 1)
		return (1);

	return (0);
}

/* ---------------------- internal functions ---------------------- */

/*
 * Function:	BootobjInit
 * Description:	Initialize the boot object structure. The disk object
 *		list must be created before calling this function.
 * Scope:	internal
 * Parameters:	none
 * Return:	D_OK	  Boot object state initialized successfully.
 *		D_FAILED  Boot object state initialization failed.
 */
/* 
 * To get around a problem with dbx and libthread, define NODEVINFO
 * to 'comment out' code references to functions in libdevinfo,
 * which is threaded.
 */
int
BootobjInit(void)
{
	char *	  disk;
	int	  device;
	int	  prom_updateable;

	/* get the default boot disk and device */
	BootDefault(&disk, &device);

	/*
	 * if we are running a live run, we can use the DDI interfaces
	 * to se if the firmware is updateable; if we are doing a
	 * disk simulation, however, we can't count on the DDI interfaces
	 * because this may be a cross platform run and the return codes
	 * my be incorrect for the targetted architecture
	 */
	prom_updateable = 0;
	if (GetSimulation(SIM_SYSDISK) != 0) {
		if (IsIsa("sparc"))
			prom_updateable = 1;
#ifndef NODEVINFO
	} else if (devfs_bootdev_modifiable() == 0) {
		prom_updateable = 1;
#endif
	}

	/* initialize the existing object structure */
	if (BootobjSetAttributePriv(CFG_EXIST,
			BOOTOBJ_DISK,		 disk,
			BOOTOBJ_DISK_EXPLICIT,	 0,
			BOOTOBJ_DEVICE_TYPE,	 IsIsa("sparc") ? 's' : 'p',
			BOOTOBJ_DEVICE,		 device,
			BOOTOBJ_DEVICE_EXPLICIT, 0,
			BOOTOBJ_PROM_UPDATE,	 IsIsa("sparc") ? 1 : 0,
			BOOTOBJ_PROM_UPDATEABLE, prom_updateable,
			NULL) != D_OK) {
		BootobjReset(CFG_EXIST);
		return (D_FAILED);
	}

	/* initialize the current and committed states to match the existing */
	BootobjCopy(CFG_COMMIT, CFG_EXIST);
	BootobjCopy(CFG_CURRENT, CFG_EXIST);
	return (D_OK);
}

/*
 * Function:	BootobjSetAttributePriv
 * Description:	Update the current boot object attributes. Attribute/value pairs
 *		are processed from left-to-right as they appear in the parameter
 *		list. All constraints and side-effects noted with each
 *		attribute. If an update fails while processing an attribute
 *		the state of the current boot object prior to the call is
 *		restored. This function is allowed full attribute access.
 * Scope:	internal
 * Parameters:	state	[RO] (Label_t)
 *			Identify boot object state from which to retrieve
 *			configuration information. Valid values are:
 *				CFG_CURRENT	current state info
 *				CFG_COMMIT	committed state info
 *				CFG_EXIST	existing state info
 *		...	Null terminated list of attribute value pairs.
 *			Attributes and values are:
 *
 *			BOOTOBJ_DISK		 char[] (disk name)
 *			BOOTOBJ_DISK_EXPLICIT    int (0|1)
 *			BOOTOBJ_DEVICE		 int (device index)
 *			BOOTOBJ_DEVICE_TYPE	 char ('p'|'s') RESTRICTED
 *			BOOTOBJ_DEVICE_EXPLICIT	 int (0|1)
 *			BOOTOBJ_PROM_UPDATE	 int (0|1)
 *			BOOTOBJ_PROM_UPDATEABLE  int (0|1) RESTRICTED
 *
 * Return:	D_OK	  All attributes set successfully.
 *		D_BADARG  One of the arguments is invalid.
 */
int
BootobjSetAttributePriv(Label_t state, ...)
{
	BootobjState	save;
	va_list		ap;
	int		status;

	/* validate parameters */
	if (state != CFG_CURRENT && state != CFG_COMMIT && state != CFG_EXIST)
		return (D_BADARG);

	BootobjstateDuplicate(&save, BootobjGetRef(state));
	va_start(ap, state);
	status = coreBootobjSetAttribute(state, 1, ap);
	va_end(ap);
	if (status != D_OK)
		BootobjstateDuplicate(BootobjGetRef(state), &save);
	BootobjstateFree(&save);
	return (status);
}

/* ---------------------- private functions ----------------------- */

/*
 * Function:	BootobjCopy
 * Description:	Copy one boot object state to another.
 * Scope:	private
 * Parameters:	dst	[RO] (Label_t)
 *			Boot object configuration state for copy destination.
 *			Valid values are:
 *			    CFG_CURRENT		current state
 *			    CFG_COMMIT		committed state
 *			    CFG_EXIST		existing state
 *		src	[RO] (Label_t)
 *			Boot object configuration state for copy source.
 *			Valid values are:
 *			    CFG_CURRENT		current state
 *			    CFG_COMMIT		committed state
 *			    CFG_EXIST		existing state
 * Return:	none
 */
static void
BootobjCopy(Label_t dst, Label_t src)
{
	/* validate parameters */
	if (dst != CFG_CURRENT && dst != CFG_COMMIT && dst != CFG_EXIST)
		return;

	if (src != CFG_CURRENT && src != CFG_COMMIT && src != CFG_EXIST)
		return;

	BootobjReset(dst);
	BootobjstateDuplicate(BootobjGetRef(dst), BootobjGetRef(src));
}

/*
 * Function:	BootobjstateDuplicate
 * Description:	Duplicate one boot object state structure to the other.
 * Scope:	private
 * Parameters:	dst	[RO, *RO] (BootobjState *)
 *			Pointer to boot object state structure to receive
 *			copy.
 * 		src	[RO, *RO] (BootobjState *)
 *			Pointer to boot object state structure to be used
 *			as the source of a copy.
 * Return:	none
 */
static void
BootobjstateDuplicate(BootobjState *dst, BootobjState *src)
{
	/* validate parameters */
	if (dst == NULL || src ==  NULL)
		return;

	if (src->disk != NULL && src->disk[0] != '\0')
		dst->disk = xstrdup(src->disk);
	else
		dst->disk = NULL;

	dst->device = src->device;
	dst->devtype = src->devtype;
	dst->flags = src->flags;
}

/*
 * Function:	BootobjstateFree
 * Description:	Free any dynamically allocate components of a boot object
 *		state. The state structure itself is NOT freed by this routine.
 * Scope:	private
 * Parameters:	bosp	[RO, *RW] (BootobjState *)
 *			Pointer to a boot object state structure.
 * Return:	none
 */
static void
BootobjstateFree(BootobjState *bosp)
{
	/* validate parameters */
	if (bosp == NULL)
		return;

	if (bosp->disk != NULL) {
		(void) free(bosp->disk);
		bosp->disk = NULL;
	}
}

/*
 * Function:	BootobjReset
 * Description:	Reset a given state of the boot object to a restarting
 *		condition. Reset will deallocate disk string data and set the
 *		pointer to NULL, set the device to -1, and clear all flag bits.
 * Scope:	private
 * Parameters:	state	[RO] (Label_t)
 *			Boot object configuration state for which to
 *			return a pointer. Valid values are:
 *			    CFG_CURRENT    current configuration
 *			    CFG_COMMIT	   committed configuration
 *			    CFG_EXIST	   existing configuration
 * Return:	none
 */
static void
BootobjReset(Label_t state)
{
	BootobjState *	bop;

	/* validate parameters */
	if (state != CFG_CURRENT && state != CFG_COMMIT && state != CFG_EXIST)
		return;

	if ((bop = BootobjGetRef(state)) == NULL)
		return;

	/* free dynamic components and reset static components */
	BootobjstateFree(bop);
	bop->flags = (u_int)0;
	bop->devtype = '\0';
	bop->device = -1;
}

/*
 * Function:	coreBootobjGetAttribute
 * Description:	Internal privileged program used to retrieve all of the
 *		attributes associated with the boot object. The privilege bit
 *		is set if the a restricted piece of object data exists
 *		(currently not needed) which should not be returned for the
 *		public interface.
 * Scope:	private
 * Parameters:	state	[RO] (Label_t)
 *			Boot object configuration state for which to
 *			return a pointer. Valid values are:
 *			    CFG_CURRENT	    current state
 *			    CFG_COMMIT	    committed state
 *			    CFG_EXIST	    existing state
 *		privilege  [RO] (int)
 *			Privilege bit indicating whether or not privileged data
 *			should be returned upon request. Valid values are:
 *			    '1' - privilege on
 *			    '0' - no privilege
 *		ap	[RW] (va_list)
 *			Varargs list of attribute specifiers and their
 *			corresponding value identifiers used to retrieve the
 *			information.
 * Return:	D_OK	  Data retrieved successfully.
 *		D_BADARG  Invalid argument.
 */
/*ARGSUSED1*/
static int
coreBootobjGetAttribute(Label_t state, int privilege, va_list ap)
{
	BootobjAttrType	 keyword;
	BootobjState *   bop;
	int *		 vip;
	char *		 vcp;

	/* validate parameters */
	if (state != CFG_CURRENT && state != CFG_COMMIT && state != CFG_EXIST)
		return (D_BADARG);

	/* initialize boot object pointer */
	bop = BootobjGetRef(state);

	/*
	 * extract attribute/value pairs and return data
	 */
	while ((keyword = va_arg(ap, BootobjAttrType)) != NULL) {
		switch (keyword) {
		    case BOOTOBJ_DISK:
			vcp = va_arg(ap, char *);
			if (vcp == NULL)
				return (D_BADARG);
			if (bop->disk == NULL)
				vcp[0] = '\0';
			else
				(void) strcpy(vcp, bop->disk);
			break;

		    case BOOTOBJ_DISK_EXPLICIT:
			vip = va_arg(ap, int *);
			if (vip == NULL)
				return (D_BADARG);
			(*vip) = BootobjGetFlag(bop, BF_DISK_EXPLICIT);
			break;

		    case BOOTOBJ_DEVICE:
			vip = va_arg(ap, int *);
			if (vip == NULL)
				return (D_BADARG);
			(*vip) = bop->device;
			break;

		    case BOOTOBJ_DEVICE_TYPE:
			vcp = va_arg(ap, char *);
			if (vcp == NULL)
				return (D_BADARG);
			(*vcp) = bop->devtype;
			break;

		    case BOOTOBJ_DEVICE_EXPLICIT:
			vip = va_arg(ap, int *);
			if (vip == NULL)
				return (D_BADARG);
			(*vip) = BootobjGetFlag(bop, BF_DEVICE_EXPLICIT);
			break;

		    case BOOTOBJ_PROM_UPDATE:
			vip = va_arg(ap, int *);
			if (vip == NULL)
				return (D_BADARG);
			(*vip) = BootobjGetFlag(bop, BF_PROM_UPDATE);
			break;

		    case BOOTOBJ_PROM_UPDATEABLE:
			vip = va_arg(ap, int *);
			if (vip == NULL)
				return (D_BADARG);
			(*vip) = BootobjGetFlag(bop, BF_PROM_UPDATEABLE);
			break;

		    default:
			return (D_BADARG);
		}
	}

	return (D_OK);
}

/*
 * Function:	coreBootobjSetAttribute
 * Description:	Internal privileged program used to set all of the attributes
 *		associated with the boot object. The privilege bit is set if the
 *		a restricted piece of object data exists which should not be
 *		updated using public interfaces.
 * Scope:	private
 * Parameters:	state	[RO] (Label_t)
 *			Boot object configuration state for which to return a
 *			pointer. Valid values are:
 *			    CFG_CURRENT		current boot object state
 *			    CFG_COMMIT		committed boot object state
 *			    CFG_EXIST		existing boot object state
 *		privilege  [RO] (int)
 *			Privilege bit indicating whether or not privileged data
 *			should be set. Valid values are:
 *			    '1' - privilege on
 *			    '0' - no privilege
 *		ap	[RW] (va_list)
 *			Varargs list of attribute specifiers and their
 *			corresponding value identifiers used to provide
 *			information.
 * Return:	D_OK	  Configuration update successful.
 *		D_BADARG  Invalid argument specified.
 */
static int
coreBootobjSetAttribute(Label_t state, int privilege, va_list ap)
{
	BootobjAttrType	keyword;
	BootobjState *  bop;
	char *		vcp;
	char 		vc;
	int 		vi;

	/* initialize boot object pointer */
	switch (state) {
	    case CFG_CURRENT:
		break;
	    case CFG_COMMIT:	/* set only supported with privilege */
	    case CFG_EXIST:
		if (privilege == 0)
			return (D_BADARG);
		break;
	    default:
		return (D_BADARG);
	}

	bop = BootobjGetRef(state);

	/*
	 * process each attribute/value pair
	 */
	while ((keyword = va_arg(ap, BootobjAttrType)) != NULL) {
		switch (keyword) {
		    case BOOTOBJ_DISK:
			vcp = va_arg(ap, char *);
			if (bop->disk != NULL) {
				free(bop->disk);
				bop->disk = NULL;
			}
			/* reset if NULL or "" disk name were specified */
			if (vcp == NULL || vcp[0] == '\0') {
				BootobjClearFlag(bop, BF_DISK_EXPLICIT);
				BootobjClearFlag(bop, BF_DEVICE_EXPLICIT);
				bop->device = -1;
			} else {
				bop->disk = xstrdup(vcp);
			}
			break;

		    case BOOTOBJ_DISK_EXPLICIT:
			vi = va_arg(ap, int);
			switch (vi) {
			    case 0:	/* disable explicit specification */
				BootobjClearFlag(bop, BF_DISK_EXPLICIT);
				BootobjClearFlag(bop, BF_DEVICE_EXPLICIT);
				break;
			    case 1:
				if (bop->disk == NULL)
					return (D_BADARG);
				BootobjSetFlag(bop, BF_DISK_EXPLICIT);
				break;
			    default:
				return (D_BADARG);
			}
			break;

		    case BOOTOBJ_DEVICE:	/* boot device index */
			vi = va_arg(ap, int);
			if (vi == -1) {
				BootobjClearFlag(bop, BF_DEVICE_EXPLICIT);
			} else if (vi >= 0) {
				if (bop->devtype == 'p' &&
						invalid_fdisk_part(vi))
					return (D_BADARG);
				if (bop->devtype == 's' &&
						invalid_sdisk_slice(vi))
					return (D_BADARG);
			} else {
				return (D_BADARG);
			}

			bop->device = vi;
			break;

		    case BOOTOBJ_DEVICE_TYPE:	/* privileged */
			if (privilege == 0)
				return (D_BADARG);
			vc = va_arg(ap, char);
			if (vc != 'p' && vc != 's')
				return (D_BADARG);
			bop->devtype = vc;
			break;

		    case BOOTOBJ_DEVICE_EXPLICIT:
			vi = va_arg(ap, int);
			switch (vi) {
			    case 0:
				BootobjClearFlag(bop, BF_DEVICE_EXPLICIT);
				break;
			    case 1:
				if (bop->device == -1)
					return (D_BADARG);
				BootobjSetFlag(bop, BF_DEVICE_EXPLICIT);
				break;
			    default:
				return (D_BADARG);
			}
			break;

		    case BOOTOBJ_PROM_UPDATE:
			vi = va_arg(ap, int);
			switch (vi) {
			    case 0:
				BootobjClearFlag(bop, BF_PROM_UPDATE);
				break;
			    case 1:
				BootobjSetFlag(bop, BF_PROM_UPDATE);
				break;
			    default:
				return (D_BADARG);
			}
			break;

		    case BOOTOBJ_PROM_UPDATEABLE:	/* privileged */
			if (privilege == 0)
				return (D_BADARG);
			vi = va_arg(ap, int);
			switch (vi) {
			    case 0:
				BootobjClearFlag(bop, BF_PROM_UPDATEABLE);
				break;
			    case 1:
				BootobjSetFlag(bop, BF_PROM_UPDATEABLE);
				break;
			    default:
				return (D_BADARG);
			}
			break;

		    default:
			return (D_BADARG);
		}
	}

	return (D_OK);
}

/*
 * Function:	BootobjGetRef
 * Description:	Retrieve a pointer to the boot object to be modified. This
 *		allows for data encapsulation within this function.
 * Scope:	private
 * Parameters:	state	[RO] (Label_t)
 *			Boot object configuration state for which to
 *			return a pointer. Valid values are:
 *			    CFG_CURRENT	    current state
 *			    CFG_COMMIT	    committed state
 *			    CFG_EXIST	    existing state
 * Return:	NULL	Invalid argument.
 *		!NULL	Pointer to boot object associated with given state.
 */
static BootobjState *
BootobjGetRef(Label_t state)
{
	static BootobjState	Boot_object[] = {{ NULL, -1, '\0', 0 },
						 { NULL, -1, '\0', 0 },
						 { NULL, -1, '\0', 0 }};

	switch (state) {
	    case CFG_CURRENT:
		return (&Boot_object[0]);
	    case CFG_COMMIT:
		return (&Boot_object[1]);
	    case CFG_EXIST:
		return (&Boot_object[2]);
	    default:
		return (NULL);
	}
}
