#ifndef lint
#pragma ident "@(#)svc_sdiskconfig.c 1.34 96/10/10 SMI"
#endif

/*
 * Copyright (c) 1995-1996 by Sun Microsystems, Inc. All rights reserved.
 */
/*
 * Module:	svc_sdiskconfig.c
 * Group:	libspmisvc
 * Description:	Slice configuration and sdisk autolayout routines.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/param.h>
#include <sys/types.h>
#include "spmisvc_lib.h"
#include "spmistore_lib.h"
#include "spmicommon_api.h"

/* private prototypes */

static int		AutolayoutOptional(void);
static int		AutolayoutDisks(void);
static int		ResobjConfigExplicit(u_char);
static int		SliceobjConfigResobj(ResobjHandle, Disk_t *);
static void		ResobjAdoptDependents(ResobjHandle, Disk_t *, int);
static ResobjHandle *	ResobjFindBestFit(ResobjHandle *, int);
static void		ResobjRecurseBestFit(int, int, int, ResobjHandle *,
				ResobjHandle *, ResobjHandle *);
static ResobjHandle *	ResobjGetCandidates(u_char, Disk_t *);
static void		CheckpointRestoreExplRoot(CheckHandle handle);
static int		LayoutAlgorithm1(void);
static void		LayoutAlgorithm1Disk(Disk_t *);
static int		LayoutAlgorithm2(void);
static void		LayoutAlgorithm2Disk(Disk_t *);
static Disk_t *		InitBootobjDisk(void);
static int		AdvanceOptional(void);
static int		AdvanceBootobj(int);
static int		SyncConfig(void);

/* comparison constraint bits for checkpoint */
#define	CHECKPOINT_DISKS	0x01
#define	CHECKPOINT_RESOURCES	0x02
#define	CHECKPOINT_ALL		(CHECKPOINT_DISKS | CHECKPOINT_RESOURCES)

/* explicit constraint flag bits for resource autolayout */
#define	EXPLICIT_SLICE		0x01
#define	EXPLICIT_SIZE		0x02
#define	EXPLICIT_START		0x04
#define	EXPLICIT_NOSIZE		0x08
#define	PREFER_NODISK		0x10

/* globals */
static int		_BestTotal;

/* autolayout functions */
typedef int	(*FunctionPtr)(void);

FunctionPtr	_LayoutAlgorithm[] = {
			LayoutAlgorithm1,
			LayoutAlgorithm2,
			NULL };

/* ---------------------- public functions ----------------------- */

/*
 * Function:	SdiskobjConfig
 * Description:	Set the sdisk structure state according to 'label'. This
 *		routine returns in error if requested to restore the existing
 *		or committed states and there was an sdisk geometry change
 *		(e.g. SOLARIS partition changed) between the current and
 *		original (or committed) configuration.
 * Scope:	public
 * Parameter:	layout	[RO] (Layout_t)
 *			Specify the how the slice configuration should be
 *			layed out. Valid values are:
 *			    LAYOUT_RESET	configure empty
 *			    LAYOUT_COMMIT	restore the committed config
 *			    LAYOUT_EXIST	restore the existing config
 *		disk	[RO, *RO] (Disk_t *)
 *			Disk structure pointer; NULL if specifying drive
 *			by 'drive'. 'dp' has precedence in the drive order
 *			(state:  okay, selected)
 *		drive	[RO, *RO] (char *)
 *			Name of drive (e.g. c0t0d0)(NULL if specifying drive
 *			by 'disk') (state:  selected)
 * Return:	D_OK	    disk state set successfully
 *		D_BADARG    invalid argument
 *		D_NODISK    neither argument was specified
 *		D_NOTSELECT disk state was not selected
 *		D_GEOMCHNG  disk geom change prohibits restore
 *		D_FAILED    layout failed
 */
int
SdiskobjConfig(Layout_t layout, Disk_t *disk, char *drive)
{
	Disk_t	*dp;

	if (disk != NULL)
		dp = disk;
	else if ((dp = find_disk(drive)) == NULL)
		return (D_NODISK);

	/* only selected drives can be configured */
	if (!disk_selected(dp))
		return (D_NOTSELECT);

	if (layout != LAYOUT_RESET &&
			layout != LAYOUT_COMMIT &&
			layout != LAYOUT_EXIST)
		return (D_BADARG);
	/*
	 * just ignore disks which don't have an sdisk geometry (i.e. no
	 * Solaris partition or not fdisk configuration not validated)
	 */
	if (sdisk_geom_null(dp))
		return (D_OK);

	switch (layout) {
	    case LAYOUT_RESET:
		return (_reset_sdisk(dp));
	    case LAYOUT_EXIST:
		return (SdiskobjRestore(CFG_EXIST, dp));
	    case LAYOUT_COMMIT:
		return (SdiskobjRestore(CFG_COMMIT, dp));
	}

	return (D_BADARG);
}

/*
 * Function:	SdiskobjAutolayout
 * Description:	Autolayout the slices to try and satisfy the resource
 *		space requirements.
 * Scope:	public
 * Parameters:	none
 * Return:	D_OK		layout successful
 *		D_NODISK	no disks to configure
 *		D_FAILED	could not allocate storage for all resources
 */
int
SdiskobjAutolayout(void)
{
	CheckHandle  	chkpta;
	int		status = D_FAILED;
	Disk_t *	dp;

	/* if there are no disks, we're done */
	if (first_disk() == NULL)
		return (D_NODISK);

	if (get_trace_level() > 5) {
		write_status(SCR, LEVEL0,
			"Executing sdisk autolayout algorithm");
	}

	/*
	 * create checkpoint 'A' before any resource or disk changes can
	 * occur
	 */
	if (get_trace_level() > 5) {
		write_status(SCR, LEVEL1,
			"Creating checkpoint A");
	}

	if ((chkpta = CheckpointCreate()) == NULL)
		return (D_FAILED);

	/* initialize the boot object (may update the "/" resource) */
	if (AdvanceBootobj(TRUE) != D_OK)
		return (D_FAILED);

	/*
	 * make sure all resources which already have space allocated are
	 * marked as ResobjIndependent
	 */
	if (SyncConfig() != D_OK)
		return (D_FAILED);

	/*
	 * try to layout the file systems under different boot object
	 * configurations
	 */
	do {
		if ((status = AutolayoutOptional()) != D_OK) {
			if (get_trace_level() > 5) {
				write_status(SCR, LEVEL1,
					"Restoring all of checkpoint A");
			}

			(void) CheckpointRestore(chkpta, CHECKPOINT_ALL);
		}
	} while (status != D_OK && AdvanceBootobj(FALSE) == D_OK);

	/*
	 * if we had a successful layout, allocate free space on disks which
	 * autolayout modified
	 */
	if (status == D_OK) {
		WALK_DISK_LIST(dp) {
			if (sdisk_is_usable(dp) && CheckpointCompare(
					CFG_CURRENT, dp, chkpta,
					CHECKPOINT_DISKS) != 0)
				(void) SdiskobjAllocateUnused(dp);
		}
	}

	/*
	 * restore the explicit disk and device values which were there before
	 * the autolayout
	 */
	CheckpointRestoreExplRoot(chkpta);

	/* destroy checkpoint 'A' */
	(void) CheckpointDestroy(chkpta);
	return (status);
}

/* ---------------------- private functions ----------------------- */

/*
 * Function:	CheckpointRestoreExplRoot
 * Description:	Restore the "/" (0) explicit disk and device values to
 *		the current resource object from the checkpoint save.
 *		This is because autolayout plays with these values
 *		and we don't want unexpected side effects
 * Scope:	private
 * Parameters:	handle	[RO] (CheckHandle)
 *			Checkpoint handle from which to extract the
 *			original values.
 * Return:	none
 */
static void
CheckpointRestoreExplRoot(CheckHandle handle)
{
	Checkpoint *	chkpt;
	ResStatEntry *	res;
	ResobjHandle	rres;

	/* validate parameters */
	if (handle == NULL)
		return;

	chkpt = (Checkpoint *)handle;
	for (res = chkpt->resources;
			res != NULL;
			res = res->next) {
		if (streq(Resobj_Name(res->resource), ROOT) &&
				Resobj_Instance(res->resource) == 0) {
			if ((rres = ResobjFind(ROOT, 0)) != NULL) {
				(void) ResobjSetAttribute(rres,
					RESOBJ_DEV_EXPLDISK,
					    Resobj_Dev_Expldisk(res->resource),
					RESOBJ_DEV_EXPLDISK,
					    Resobj_Dev_Expldisk(res->resource),
					NULL);
			}
			break;
		}
	}
}

/*
 * Function:	AutolayoutOptional
 * Description:	For a given boot object configuration, rotate through varying
 *		optional resource configurations until one is found that works.
 * Scope:	private
 * Parameters:	none
 * Return:	D_OK		configuration successful
 *		D_FAILED	configuration incomplete
 */
static int
AutolayoutOptional(void)
{
	CheckHandle	chkptb;
	int		status;

	/*
	 * layout the explicit disk resources first
	 */
	if (get_trace_level() > 5) {
		write_status(SCR, LEVEL1,
			"Configuring resources with explicit size");
	}

	status = ResobjConfigExplicit(EXPLICIT_SIZE);
	if (status != D_OK)
		return (status);

	/* see if there is anymore work to do */
	if (ResobjIsComplete(RESSIZE_DEFAULT) == NULL)
		return (D_OK);

	/* create checkpoint 'B' of the current resource/disk configuration */
	if (get_trace_level() > 5) {
		write_status(SCR, LEVEL1,
			"Creating checkpoint B");
	}

	if ((chkptb = CheckpointCreate()) == NULL)
		return (D_FAILED);

	status = D_OK;
	do {
		if ((status = AutolayoutDisks()) != D_OK) {
			if (get_trace_level() > 5) {
				write_status(SCR, LEVEL1,
					"Restoring disks of checkpoint B");
			}

			(void) CheckpointRestore(chkptb, CHECKPOINT_DISKS);
		}
	} while (status != D_OK && AdvanceOptional() == D_OK);

	/* destroy checkpoint 'B' */
	(void) CheckpointDestroy(chkptb);
	return (status);
}

/*
 * Function:	AdvanceOptional
 * Description:	Go through the optional resources and find the next one that
 *		is a viable candidate for spinning off. The order of spin-off
 *		precedence is:
 *		    Process resource object classes:
 *			RESCLASS_DYNAMIC
 *			RESCLASS_REPOSITORY
 *			RESCLASS_STATIC
 *		    and within each class, find the largest resource using
 *		    minimum sizing which does size optional resources on
 *		    an individual basis (NOTE: default sizing would always
 *		    give a '0' size for optional resources - this may be
 *		    something we want to change)
 * Scope:	private
 * Parameters:	none
 * Return:	D_OK	  an optional resource has been configured as
 *			  independent
 *		D_FAILED  no more optional resources to spin off
 */
static int
AdvanceOptional(void)
{
	static ResClass_t   _Class = RESCLASS_DYNAMIC;
	ResobjHandle *	    resource_list;
	ResobjHandle	    res;
	int	count;
	int	found;
	int	size;
	int	csize;
	int	index;
	int	i;

	if (get_trace_level() > 5) {
		write_status(SCR, LEVEL1,
			"Updating optional resource configuration");
	}

	count = 0;
	WALK_RESOURCE_LIST(res, RESTYPE_UNDEFINED) {
		if (ResobjIsOptional(res))
			count++;
	}

	if (count == 0) {
		if (get_trace_level() > 5) {
			write_status(SCR, LEVEL1|LISTITEM,
				"No optional resources to update");
		}
		return (D_FAILED);
	}

	if ((resource_list = (ResobjHandle *)xcalloc(
			sizeof (ResobjHandle) * count)) == NULL) {
		return (D_FAILED);
	}

	/*
	 * while you haven't got a candidate
	 *   (1) collect all optional resources in the current class
	 *   (2) if there are none, advance the class and start over
	 *   (3) if there are no more classes, we're done (D_FAILED)
	 *   (4) of the viable resources, find the one with the
	 *	 largest default size and mark it
	 */
	for (found = 0; !found && _Class != RESCLASS_UNDEFINED; /* none */) {
		count = 0;
		WALK_RESOURCE_LIST(res, RESTYPE_UNDEFINED) {
			/*
			 * do not consider resources with no minimum size
			 * as candidates for standing on their own
			 */
			if (ResobjIsOptional(res) &&
					Resobj_Content_Class(res) == _Class &&
					ResobjGetContent(res, ADOPT_NONE,
						RESSIZE_MINIMUM) > 0)
				resource_list[count++] = res;
		}

		if (count == 0) {
			/*
			 * if no resource found in this class, advance the
			 * class
			 */
			switch (_Class) {
			    case RESCLASS_DYNAMIC:
				_Class = RESCLASS_REPOSITORY;
				break;
			    case RESCLASS_REPOSITORY:
				_Class = RESCLASS_STATIC;
				break;
			    case RESCLASS_STATIC:
				_Class = RESCLASS_UNDEFINED;
			}
		} else {
			index = 0;
			size = ResobjGetContent(resource_list[index],
					ADOPT_ALL, RESSIZE_DEFAULT);
			for (i = index + 1; i < count; i++) {
				csize = ResobjGetContent(resource_list[i],
						ADOPT_ALL, RESSIZE_DEFAULT);
				if (csize > size) {
					index = i;
					size = csize;
				}
			}

			/* pick the largest of the resources */
			(void) ResobjSetAttribute(resource_list[index],
					RESOBJ_STATUS,  RESSTAT_INDEPENDENT,
					NULL);
			found++;
			if (get_trace_level() > 5) {
				write_status(SCR, LEVEL1|LISTITEM,
					"Spinning off optional resource %s",
					Resobj_Name(resource_list[index]));
			}
		}
	}

	free(resource_list);
	return (found > 0 ? D_OK : D_FAILED);
}

/*
 * Function:	AutolayoutDisks
 * Description:	Apply various combinations of autolayout algorithms to the
 *		disks, processing preferred disk constraints first, and
 *		then general autolayout configurations.
 * Scope:	private
 * Parameters:	none
 * Return:	D_OK
 *		D_FAILED
 */
static int
AutolayoutDisks(void)
{
	CheckHandle	chkptc;
	int		status;
	int		i;

	/* layout the explicit disk resources first */
	if (get_trace_level() > 5) {
		write_status(SCR, LEVEL1,
			"Configuring resources with no explicit size");
	}

	status = ResobjConfigExplicit(EXPLICIT_NOSIZE);
	if (status != D_OK)
		return (status);

	/* if there is no more work to do, we're done */
	if (ResobjIsComplete(RESSIZE_DEFAULT) == NULL)
		return (D_OK);

	/* create checkpoint 'C' */
	if (get_trace_level() > 5) {
		write_status(SCR, LEVEL1,
			"Creating checkpoint C");
	}

	if ((chkptc = CheckpointCreate()) == NULL)
		return (D_FAILED);

	/*
	 * try layout algorithms until one results in complete storage
	 * allocation for all independent resources
	 */
	for (status = D_FAILED, i = 0;
			status != D_OK && _LayoutAlgorithm[i] != NULL; i++) {
		if ((status = (*_LayoutAlgorithm[i])()) != D_OK) {
			if (get_trace_level() > 5) {
				write_status(SCR, LEVEL1,
					"Restoring all of checkpoint C");
			}
			(void) CheckpointRestore(chkptc, CHECKPOINT_ALL);
		}
	}

	/* destroy checkpoint 'C' */
	(void) CheckpointDestroy(chkptc);
	return (status);
}

/*
 * Function:	ResobjGetCandidate
 * Description:
 * Scope:	private
 * Parameters:	constraints [RO] (u_char)
 *			    Constraint flag use to specify additional expliict
 *			    constraints which must be honored for configuration
 *			    selection. Valid bit fields are:
 *				EXPLICIT_SLICE
 *				EXPLICIT_SIZE
 *				EXPLICIT_NOSIZE
 *				EXPLICIT_START
 *		dp	    [RO, *RO] (Disk_t *)
 * Return:	NULL
 *		ResobjHandle *	pointer to head of resource handle array
 */
static ResobjHandle *
ResobjGetCandidates(u_char constraints, Disk_t *dp)
{
	static ResobjHandle *	list = NULL;
	ResobjHandle	res;
	int		count;
	Disk_t *	tdp;
	char		pdisk[MAXNAMELEN];
	char		edisk[MAXNAMELEN];
	int		edevice;
	int		estart;
	int		esize;

	/*
	 * free the old list (if there is one) and allocate a new one which is
	 * large enough to hold the maximum number of independent resources
	 */
	if (list != NULL)
		free(list);
	count = 0;
	WALK_RESOURCE_LIST(res, RESTYPE_UNDEFINED) {
		if (ResobjIsIndependent(res))
			count++;
	}

	list = (ResobjHandle *)xcalloc(sizeof (ResobjHandle) * (count + 1));

	/*
	 * create the resource list based on the constraints provided
	 */
	count = 0;
	WALK_RESOURCE_LIST(res, RESTYPE_UNDEFINED) {
		/* only consider independent resources */
		if (!ResobjIsIndependent(res))
			continue;

		if (ResobjGetAttribute(res,
				RESOBJ_DEV_EXPLDISK,	 edisk,
				RESOBJ_DEV_PREFDISK,	 pdisk,
				RESOBJ_DEV_EXPLDEVICE,	 &edevice,
				RESOBJ_DEV_EXPLSTART,	 &estart,
				RESOBJ_DEV_EXPLSIZE,	 &esize,
				NULL) != D_OK)
			continue;

		/*
		 * don't consider resources which are already configured
		 * somewhere in the disk list
		 */
		if (SliceobjFindUse(CFG_CURRENT, NULL, Resobj_Name(res),
				Resobj_Instance(res), FALSE) != NULL)
			continue;

		/*
		 * do not configure explicit candidates with this routine
		 */
		if (edisk[0] != '\0')
			continue;
		/*
		 * if the use wants only resources that have no preferential
		 * disk and this resource prefers a disk, ignore it
		 */
		if ((constraints & PREFER_NODISK) && (pdisk[0] != '\0'))
			continue;

		/*
		 * if the caller provides a disk pointer, ignore resources
		 * which have specified preferred disks which don't match the
		 * resource preference
		 */
		if (dp != NULL) {
			if (pdisk[0] == '\0' ||
				(tdp = find_disk(pdisk)) == NULL || tdp != dp)
			continue;
		}

		/*
		 * if the explicit size constraint is specified and the
		 * resource has no explicit size, skip it
		 */
		if ((constraints & EXPLICIT_SIZE) && esize < 0)
			continue;

		/*
		 * if the no-explicit size constraint is specified and the
		 * resource has an explicit size, skip it
		 */
		if ((constraints & EXPLICIT_NOSIZE) && esize >= 0)
			continue;

		/*
		 * if the explicit start constraint is specified and the
		 * resource has no explicit start, skip it
		 */
		if ((constraints & EXPLICIT_START) && estart <= 0)
			continue;

		/*
		 * if the explicit slice constraint is specified and the
		 * resource has no explicit slice, skip it
		 */
		if ((constraints & EXPLICIT_SLICE) &&
				edevice == VAL_UNSPECIFIED)
			continue;

		list[count++] = res;
	}

	list[count] = NULL;
	return (list);
}

/*
 * Function:	ResobjConfigExplicit
 * Description:	Configure all independent resources which have an explicit
 *		disk specified, and, based on the constraint flags, also
 *		have an:
 *		    Explicit size
 *		    Explicit slice
 *		    Explicit starting cylinder
 * Scope:	private
 * Parameters:	constraints [RO] (u_char)
 *			    Constraint flag use to specify additional expliict
 *			    constraints which must be honored for configuration
 *			    selection. Valid bit fields are:
 *				EXPLICIT_SLICE
 *				EXPLICIT_SIZE
 *				EXPLICIT_NOSIZE
 *				EXPLICIT_START
 * Return:	D_OK	   configuration of explicit resources successful
 *		D_FAILED   could not configure the disk
 *		D_NODISK   the resource requires an explicit disk which
 *			   doesn't exist on the system
 */
static int
ResobjConfigExplicit(u_char constraints)
{
	ResobjHandle	res;
	Disk_t *	dp;
	char		edisk[MAXNAMELEN];
	int		edevice;
	int		estart;
	int		esize;
	int		status = D_OK;

	/*
	 * configure all resources which are RESOBJ_INDEPENDENT and have
	 * explicitly specified disks; if the user specified explicit
	 * sizes only, then only configure those slices which have
	 * an explicit device size constraint
	 */
	WALK_RESOURCE_LIST(res, RESTYPE_UNDEFINED) {
		if (ResobjGetAttribute(res,
				RESOBJ_DEV_EXPLDISK,	 edisk,
				RESOBJ_DEV_EXPLDEVICE,	 &edevice,
				RESOBJ_DEV_EXPLSTART,	 &estart,
				RESOBJ_DEV_EXPLSIZE,	 &esize,
				NULL) != D_OK)
			continue;

		/*
		 * the resource must be independent, and must have
		 * an explicit disk
		 */
		if (!ResobjIsIndependent(res) || streq(edisk, ""))
			continue;

		/*
		 * when an explicit disk is specified and the disk does
		 * not exist on the system, this is a fatal error
		 */
		if ((dp = find_disk(edisk)) == NULL) {
			status = D_NODISK;
			break;
		}

		/*
		 * if the explicit size constraint is specified and the
		 * resource has no explicit size, skip it
		 */
		if ((constraints & EXPLICIT_SIZE) && esize < 0)
			continue;

		/*
		 * if the no-explicit size constraint is specified and the
		 * resource has an explicit size, skip it
		 */
		if ((constraints & EXPLICIT_NOSIZE) && esize >= 0)
			continue;

		/*
		 * if the explicit start constraint is specified and the
		 * resource has no explicit start, skip it
		 */
		if ((constraints & EXPLICIT_START) && estart <= 0)
			continue;

		/*
		 * if the explicit slice constraint is specified and the
		 * resource has no explicit slice, skip it
		 */
		if ((constraints & EXPLICIT_SLICE) &&
				edevice == VAL_UNSPECIFIED)
			continue;

		if (get_trace_level() > 5) {
			write_status(SCR, LEVEL1|LISTITEM,
				"%s on disk %s",
				Resobj_Name(res), disk_name(dp));
		}

		/* configure the resource */
		if ((status = SliceobjConfigResobj(res, dp)) != D_OK) {
			if (get_trace_level() > 5) {
				write_status(SCR, LEVEL2|LISTITEM,
					"Configuration failed");
			}
			break;
		}
	}

	return (status);
}

/*
 * Function:	SliceobjConfigResobj
 * Description:	Get a slice for the given resource on the disk specified, and
 *		configure the slice to hold the default sizing for the
 *		resource.
 * Scope:	private
 * Parameters:	res	[RO] (ResobjHandle)
 *			Resource object handle.
 *		dp	[RO, *RW] (Disk_t *)
 *			Disk object handle to use for updating.
 * Return: D_OK		the resource had storage successfully configured
 *	   D_BADARG	invalid argument
 *	   D_FAILED	internal failure while attempting configuration
 *	   D_NOSPACE	there is insufficient space on the disk to provide
 *			storage for the resource as specified
 *	   D_DUPMNT	the required slice is already in use, or there are no
 *			more slices available
 */
/*ARGSUSED0*/
static int
SliceobjConfigResobj(ResobjHandle res, Disk_t *dp)
{
	ResobjHandle  child;
	SliceKey *  key;
	Disk_t *    tdp;
	char	    edisk[MAXNAMELEN];
	int	    edevice;
	int	    pdevice;
	int	    ddevice;
	int	    slice;
	int	    estart;
	int	    esize;
	FsAction_t  fsaction;
	int	    size;

	/* validate parameters */
	if (!ResobjIsExisting(res) || !sdisk_is_usable(dp))
		return (D_BADARG);

	/* there is only work to do for independent resources */
	if (!ResobjIsIndependent(res))
		return (D_OK);

	if (ResobjGetAttribute(res,
			RESOBJ_DEV_EXPLDISK,	 edisk,
			RESOBJ_DEV_EXPLDEVICE,	 &edevice,
			RESOBJ_DEV_PREFDEVICE,	 &pdevice,
			RESOBJ_DEV_DFLTDEVICE,	 &ddevice,
			RESOBJ_DEV_EXPLSTART,	 &estart,
			RESOBJ_DEV_EXPLSIZE,	 &esize,
			RESOBJ_FS_ACTION,	 &fsaction,
			NULL) != D_OK)
		return (D_FAILED);

	/*
	 * if an explicit disk was specified and it isn't the one
	 * passed in, flag an invalid argument
	 */
	if (is_disk_name(edisk)) {
		if ((tdp = find_disk(edisk)) == NULL || tdp != dp)
			return (D_BADARG);
	}

	/*
	 * find out if the resource already has space allocated
	 */
	if ((key = SliceobjFindUse(CFG_CURRENT, dp, Resobj_Name(res),
			Resobj_Instance(res), FALSE)) != NULL) {
		if (edevice != VAL_UNSPECIFIED && edevice != key->slice)
			return (D_FAILED);

		return (D_OK);
	}

	/*
	 * there isn't any storage allocated for this resource; find a slice
	 * to use for the layout, and determine the default storage required
	 */
	slice = VAL_UNSPECIFIED;
	if (edevice != VAL_UNSPECIFIED) {
		if (!SliceobjIsAllocated(CFG_CURRENT, dp, edevice))
			slice = edevice;
		else
			return (D_DUPMNT);
	} else if (pdevice != VAL_UNSPECIFIED &&
			!SliceobjIsAllocated(CFG_CURRENT, dp, pdevice)) {
		slice = pdevice;
	} else if (ddevice != VAL_UNSPECIFIED &&
			!SliceobjIsAllocated(CFG_CURRENT, dp, ddevice)) {
		slice = ddevice;
	} else  {
		WALK_SLICES(slice) {
			if (!SliceobjIsAllocated(CFG_CURRENT, dp, slice))
				break;
		}
	}

	if (!valid_sdisk_slice(slice))
		return (D_NOSPACE);

	/*
	 * make sure the resource has a chance of fitting on the disk,
	 * and then try to allocate the slice for the resource
	 */
	size = ResobjGetStorage(res, ADOPT_ALL, RESSIZE_DEFAULT);
	if (size > sdisk_max_hole_size(dp))
		return (D_NOSPACE);

	if (size == 0)
		return (D_FAILED);

	/*
	 * update the use, instance, start, size, and flag statuses
	 * for: explicit, stuck, preserved
	 */
	if (SliceobjSetAttribute(dp, slice,
		    SLICEOBJ_USE,	Resobj_Name(res),
		    SLICEOBJ_INSTANCE,  Resobj_Instance(res),
		    SLICEOBJ_START,
				estart >= 0 ? estart : sdisk_geom_firstcyl(dp),
		    SLICEOBJ_SIZE,	size,
		    SLICEOBJ_EXPLICIT,	esize >= 0 ? TRUE : FALSE,
		    SLICEOBJ_STUCK,
				estart >= sdisk_geom_firstcyl(dp) ?
					TRUE : FALSE,
		    SLICEOBJ_PRESERVED,	fsaction == FSACT_CHECK ? TRUE : FALSE,
		    NULL) != D_OK)
		return (D_NOSPACE);

	/* if you were successful, adopt all of your optional children */
	WALK_RESOURCE_LIST(child, RESTYPE_UNDEFINED) {
		if (ResobjIsOptional(child) && ResobjIsGuardian(res, child)) {
			if (get_trace_level() > 5) {
				write_status(SCR, LEVEL2|LISTITEM,
					"Adopting optional resource %s",
					Resobj_Name(child));
			}
			(void) ResobjSetAttribute(child,
				RESOBJ_STATUS,  RESSTAT_DEPENDENT,
				NULL);
		}
	}

	return (D_OK);
}

/*
 * Function:	ResobjAdoptDependents
 * Description:	All optional dependent resources of the specified resource
 *		should be examined to see how many of them can be adopted
 *		given the size of the slice configured for the parent.
 *		The algorithm used for fitting should guarantee that the
 *		maximum use of the slice space is made (i.e. best fit
 *		the optional resources).
 * Parameters:	res	[RO] (ResobjHandle)
 *			Resource object handle.
 *		dp	[RO, *RW] (Disk_t *)
 *			Disk object handle of the parent resource storage.
 *		slice	[RO] (int)
 *			Slice index of the parent resource storage.
 * Return:	none
 */
static void
ResobjAdoptDependents(ResobjHandle res, Disk_t *dp, int slice)
{
	ResobjHandle	children[30];
	ResobjHandle	child;
	ResobjHandle *	best;
	int		remain;
	int		size;
	int		count;
	int		index;
	int		i;

	/*
	 * first see if you can adopt all of the children
	 */
	size = ResobjGetContent(res, ADOPT_ALL, RESSIZE_DEFAULT);
	if (size <= Sliceobj_Size(CFG_CURRENT, dp, slice)) {
		WALK_RESOURCE_LIST(child, RESTYPE_UNDEFINED) {
			if (!ResobjIsOptional(child) ||
					!ResobjIsGuardian(res, child))
				continue;

			(void) ResobjSetAttribute(child,
				RESOBJ_STATUS,  RESSTAT_DEPENDENT,
				NULL);
		}

		return;
	}

	/*
	 * absorb as many children as possible using an exhaustive
	 * best-fit algorithm
	 */
	size = ResobjGetContent(res, ADOPT_NONE, RESSIZE_DEFAULT);
	remain = Sliceobj_Size(CFG_CURRENT, dp, slice) - size;
	if (remain < one_cyl(dp))
		return;

	/*
	 * collect a list of all candidate children and their default
	 * sizes (we should not be rounding up to MINFSSIZE here)
	 */
	count = 0;
	WALK_RESOURCE_LIST(child, RESTYPE_UNDEFINED) {
		/* we are only interested in unadopted children */
		if (!ResobjIsOptional(child))
			continue;

		/* make sure we are qualified to adopt this child */
		if (!ResobjIsGuardian(res, child))
			continue;

		children[count] = child;
		count++;
	}

	/*
	 * terminate the list, get the best fit, and absorb the resources
	 * listed in the best fit sequence list; spin off any resources
	 * which were orphaned since they have no hope of being made
	 * independent
	 */
	children[count] = NULL;
	best = ResobjFindBestFit(children, remain);
	for (i = 0; children[i] != NULL; i++) {
		for (index = 0; best[index] != NULL; index++) {
			if (children[i] == best[index]) {
				(void) ResobjSetAttribute(best[index],
					RESOBJ_STATUS,  RESSTAT_DEPENDENT,
					NULL);
				break;
			} else {
				(void) ResobjSetAttribute(children[i],
					RESOBJ_STATUS,  RESSTAT_INDEPENDENT,
					NULL);
			}
		}
	}
}

/*
 * Function:	ResobjFindBestFit
 * Description:	Given a list of possible child resources, and a fixed
 *		size of disk in sectors, find the best combination
 *		of children (using default sizing), to fit in the
 *		size given.
 * Scope:	private
 * Parameters:	children  [RO, *RO] (ResobjHandle *)
 *			  NULL terminated array of ResobjHandles,
 *			  each representing a child resource.
 *		remain	  [RO] (int)
 *			  Size of space remaining into which a fit can
 *			  be made
 * Return:	NULL		Bad argument
 *		ResobjHandle *
 */
static ResobjHandle *
ResobjFindBestFit(ResobjHandle *children, int remain)
{
	static ResobjHandle *	best = NULL;
	ResobjHandle *		current;
	int			index;

	/* validate parameters */
	if (children == 0 || remain < 0)
		return (NULL);

	/* free up any existing dynamically allocated data */
	if (best != NULL)
		free(best);

	/*
	 * allocate a new array to hold the maximum number of resources
	 * possible for both current and best-fit sequence tracking;
	 * make sure you include the NULL terminator entry
	 */
	for (index = 0; children[index] != NULL; index++);
	index++;
	current = (ResobjHandle *)xcalloc(sizeof (ResobjHandle) * index);
	best = (ResobjHandle *)xcalloc(sizeof (ResobjHandle) * index);

	_BestTotal = remain;
	for (index = 0; _BestTotal != 0; index++) {
		ResobjRecurseBestFit(index, remain, 0, children,
			current, best);
		if (children[index] == NULL)
			break;
	}

	if (get_trace_level() > 5) {
		for (index = 0; best[index] != NULL; index++) {
			write_status(SCR, LEVEL1|LISTITEM,
				"Best fit includes %s",
				Resobj_Name(best[index]));
		}

		if (index > 0) {
			write_status(SCR, LEVEL2|LISTITEM,
				"%d sectors remain after best fit",
				_BestTotal);
		}
	}

	free(current);
	return (best);
}

/*
 * Function:	ResobjRecurseBestFit
 * Descropton:	Recursive function which determines if the current node
 *		of the search can fit in the remaining space, and if so
 *		searches all its child paths to see if any of them are
 *		the optimal fit.
 * Scope:	private
 * Parameters;	index	   [RO] (int)
 *			   Index into children array for the current resource.
 *		available  [RO] (int)
 *			   Number of sectors available for allocation.
 *		sequence   [RO] (int)
 *			   Current seqence in the sequence status array to
 *			   record this resource
 *		children   [RO, *RO] (ResobjHandle *)
 *			   Array of resources to be considered in the fit
 *			   process
 *		current	   [RO, *RO] (ResobjHandle *)
 *			   Current resource sequence array.
 *		best	   [RO, *RO] (ResobjHandle *)
 *			   Best resource sequence array.
 * Return:	none
 */
static void
ResobjRecurseBestFit(int index, int available, int sequence,
			ResobjHandle *children, ResobjHandle *current,
			ResobjHandle *best)
{
	int	i;

	/*
	 * once you hit a terminating condition, you've finished a
	 * leaf sequence in the tree; see if this is the best
	 * fit and if so, update the best sequence list and best
	 * total value
	 */
	if (children[index] == NULL) {
		current[sequence] = NULL;
		if (available < _BestTotal && available >= 0) {
			_BestTotal = available;
			for (i = 0; current[i] != NULL; i++)
				best[i] = current[i];
			best[i] = NULL;
		}

		return;
	}

	/* see how much is left over after accounting for this node */
	available -= ResobjGetContent(children[index], ADOPT_NONE,
				RESSIZE_DEFAULT);

	/*
	 * if we're already too big at this point we might as well
	 * not bother checking the children
	 */
	if (available < 0)
		return;

	/*
	 * update the current seqence counter with the current resource
	 */
	current[sequence++] = children[index];

	for (index++; _BestTotal != 0; index++) {
		ResobjRecurseBestFit(index, available,
			sequence, children, current, best);
		if (children[index] == NULL)
			break;
	}
}

/*
 * Function:	LayoutAlgorithm1
 * Description:	Layout algorithm #1. For every configurable disk on the system
 *		(starting with the boot disk):
 *		  (1) configure all resources which are:
 *			- unconfigured
 *			- independent
 *			- non-explicit
 *			- prefer this disk
 *		  (2) then configure all resources which are:
 *			- unconfigured
 *			- independent
 *			- non-explicit
 *			- prefer no disk
 *		  (3) then configure all resources which are:
 *			- unconfigured
 *			- independent
 *			- non-explicit
 *			- prefer another disk
 * Scope:	private
 * Parameters:	none
 * Return:	D_OK
 *		D_FAILED
 */
static int
LayoutAlgorithm1(void)
{
	Space *		sp;
	ResobjHandle	res;
	Disk_t *	dp;
	Disk_t *	bdp;
	int		count;
	int		i;

	if (ResobjIsComplete(RESSIZE_DEFAULT) == NULL)
		return (D_OK);

	/* configure the boot disk first */
	if (DiskobjFindBoot(CFG_CURRENT, &bdp) != D_OK ||
			bdp == NULL || !sdisk_is_usable(bdp))
		return (D_FAILED);

	if (get_trace_level() > 5) {
		write_status(SCR, LEVEL1,
			"Executing layout algorithm #1");
	}

	LayoutAlgorithm1Disk(bdp);

	/* configure disks with unconfigured preferred resources next */
	WALK_DISK_LIST(dp) {
		if (ResobjIsComplete(RESSIZE_DEFAULT) == NULL)
			return (D_OK);

		if (dp != bdp || !sdisk_is_usable(dp))
			continue;

		count = 0;
		WALK_RESOURCE_LIST(res, RESTYPE_UNDEFINED) {
			if (ResobjIsIndependent(res) &&
					Resobj_Dev_Prefdisk(res) != NULL &&
					streq(Resobj_Dev_Prefdisk(res),
						disk_name(dp)) &&
					SliceobjFindUse(CFG_CURRENT,
						NULL, Resobj_Name(res),
						Resobj_Instance(res),
						FALSE) == NULL)
				count++;
		}

		if (count > 0)
			LayoutAlgorithm1Disk(dp);
	}

	/* configure disks in order next */
	WALK_DISK_LIST(dp) {
		if (ResobjIsComplete(RESSIZE_DEFAULT) == NULL)
			return (D_OK);

		LayoutAlgorithm1Disk(dp);
	}

	if ((sp = ResobjIsComplete(RESSIZE_DEFAULT)) != NULL) {
		if (get_trace_level() > 5) {
			write_status(SCR, LEVEL2,
				"Cannot find disk space for:");
			for (i = 0; sp[i].name[0] != '\0'; i++) {
				write_status(SCR, LEVEL2|LISTITEM,
					"%s (%d MB)",
					sp[i].name,
					sectors_to_mb(sp[i].required));
			}
		}

		return (D_FAILED);
	}

	return (D_OK);
}

/*
 * Function:	LayoutAlgorithm1Disk
 * Description:	First layout algorithm per disk.
 * Scope:	private
 * Parameters:	dp	[RO, *RO] (Disk_t *)
 *			Disk object handle.
 * Return:	D_OK
 *		D_FAILED
 */
static void
LayoutAlgorithm1Disk(Disk_t *dp)
{
	int		count;
	ResobjHandle *	resource_list;
	ResobjHandle *	best_list;
	int		front;
	int		end;
	int		size;

	/* validate parameters */
	if (!sdisk_is_usable(dp))
		return;

	if (get_trace_level() > 5) {
		write_status(SCR, LEVEL1|LISTITEM,
			"Executing disk layout algorithm #1 for disk %s",
			disk_name(dp));
	}

	/*
	 * configure all resources with fixed starting cylinders which
	 * prefer this disk since they will affect the layout segmentation
	 * processing which follows
	 */
	resource_list = ResobjGetCandidates(EXPLICIT_START, dp);
	for (count = 0; resource_list[count] != NULL; count++) {
		if (get_trace_level() > 5) {
			write_status(SCR, LEVEL1|LISTITEM,
				"Configure resource %s with explicit starting cyl",
				Resobj_Name(resource_list[count]));
		}
		(void) SliceobjConfigResobj(resource_list[count], dp);
	}

	if (ResobjIsComplete(RESSIZE_DEFAULT) == NULL)
		return;

	/* pack the slices as much as possible before starting segment logic */
	(void) adjust_slice_starts(dp);

	front = sdisk_geom_firstcyl(dp);
	while ((end = SegmentFindEnd(dp, front)) >= 0) {
		/* find out how many free sectors there are in this segment */
		size = SegmentFindFreeSectors(dp, front, end);

		/*
		 * configure all unconfigured resources which prefer this
		 * disk
		 */
		resource_list = ResobjGetCandidates(0, dp);
		best_list = ResobjFindBestFit(resource_list, size);
		if (get_trace_level() > 5) {
			write_status(SCR, LEVEL1|LISTITEM,
				"Configuring resources preferring disk %s",
				disk_name(dp));
		}

		for (count = 0; best_list[count] != NULL; count++) {
			if (get_trace_level() > 5) {
				write_status(SCR, LEVEL1|LISTITEM|CONTINUE,
					Resobj_Name(best_list[count]));
			}

			(void) SliceobjConfigResobj(best_list[count], dp);
		}

		/* configure all unconfigured resources which prefer no disk */
		resource_list = ResobjGetCandidates(PREFER_NODISK, NULL);
		best_list = ResobjFindBestFit(resource_list, size);
		if (get_trace_level() > 5) {
			write_status(SCR, LEVEL1|LISTITEM,
			    "Configuring resources with no disk preference");
		}

		for (count = 0; best_list[count] != NULL; count++) {
			if (get_trace_level() > 5) {
				write_status(SCR, LEVEL1|LISTITEM|CONTINUE,
					Resobj_Name(best_list[count]));
			}

			(void) SliceobjConfigResobj(best_list[count], dp);
		}

		front = end;
	}
}

/*
 * Function:	LayoutAlgorithm2
 * Description:	Layout algorithm #2.
 * Scope:	private
 * Parameters:	none
 * Return:	D_OK		layout successful
 *		D_FAILED	layout failure
 */
static int
LayoutAlgorithm2(void)
{
	Space *		sp;
	ResobjHandle	res;
	Disk_t *	dp;
	Disk_t *	bdp;
	int		count;
	int		i;

	if (ResobjIsComplete(RESSIZE_DEFAULT) == NULL)
		return (D_OK);

	/* configure the boot disk first */
	if (DiskobjFindBoot(CFG_CURRENT, &bdp) != D_OK ||
			bdp == NULL || !sdisk_is_usable(bdp))
		return (D_FAILED);

	if (get_trace_level() > 5) {
		write_status(SCR, LEVEL1,
			"Executing layout algorithm #2");
	}

	LayoutAlgorithm2Disk(bdp);

	/* configure disks with unconfigured preferred resources next */
	WALK_DISK_LIST(dp) {
		if (ResobjIsComplete(RESSIZE_DEFAULT) == NULL)
			return (D_OK);

		if (dp != bdp || !sdisk_is_usable(dp))
			continue;

		count = 0;
		WALK_RESOURCE_LIST(res, RESTYPE_UNDEFINED) {
			if (ResobjIsIndependent(res) &&
					Resobj_Dev_Prefdisk(res) != NULL &&
					streq(Resobj_Dev_Prefdisk(res),
						disk_name(dp)) &&
					SliceobjFindUse(CFG_CURRENT,
						NULL, Resobj_Name(res),
						Resobj_Instance(res),
						FALSE) == NULL)
				count++;
		}

		if (count > 0)
			LayoutAlgorithm2Disk(dp);
	}

	/* configure disks in order next */
	WALK_DISK_LIST(dp) {
		if (ResobjIsComplete(RESSIZE_DEFAULT) == NULL)
			return (D_OK);

		LayoutAlgorithm2Disk(dp);
	}

	if ((sp = ResobjIsComplete(RESSIZE_DEFAULT)) != NULL) {
		if (get_trace_level() > 5) {
			write_status(SCR, LEVEL2,
				"Cannot find disk space for:");
			for (i = 0; sp[i].name[0] != '\0'; i++) {
				write_status(SCR, LEVEL2|LISTITEM,
					"%s (%d MB)",
					sp[i].name,
					sectors_to_mb(sp[i].required));
			}
		}

		return (D_FAILED);
	}

	return (D_OK);
}

/*
 * Function:	LayoutAlgorithm2Disk
 * Description:	Second layout per disk algorithm.
 * Scope:	private
 * Parameters:	dp	[RO, *RO] (Disk_t *)
 *			Disk object handle.
 * Return:	D_OK		layout successful
 *		D_FAILED	layout failure
 */
static void
LayoutAlgorithm2Disk(Disk_t *dp)
{
	int		count;
	ResobjHandle *	resource_list;
	ResobjHandle *	best_list;
	int		front;
	int		end;
	int		size;

	/* validate parameters */
	if (!sdisk_is_usable(dp))
		return;

	if (get_trace_level() > 5) {
		write_status(SCR, LEVEL1|LISTITEM,
			"Executing disk layout algorithm #2 for %s",
			disk_name(dp));
	}

	/*
	 * configure all resources with fixed starting cylinders which
	 * prefer this disk since they will affect the layout segmentation
	 * processing which follows
	 */
	resource_list = ResobjGetCandidates(EXPLICIT_START, dp);
	for (count = 0; resource_list[count] != NULL; count++) {
		if (get_trace_level() > 5) {
			write_status(SCR, LEVEL1|LISTITEM,
				"Resource %s with explicit starting cyl",
				Resobj_Name(resource_list[count]));
		}

		(void) SliceobjConfigResobj(resource_list[count], dp);
	}

	/* if all resources are already satisfied, we're done */
	if (ResobjIsComplete(RESSIZE_DEFAULT) == NULL)
		return;

	/* pack the slices as much as possible before starting segment logic */
	(void) adjust_slice_starts(dp);

	front = sdisk_geom_firstcyl(dp);
	while ((end = SegmentFindEnd(dp, front)) >= 0) {
		/* find out how many free sectors there are in this segment */
		size = SegmentFindFreeSectors(dp, front, end);

		/*
		 * configure all unconfigured resources which prefer this
		 * disk
		 */
		resource_list = ResobjGetCandidates(0, dp);
		best_list = ResobjFindBestFit(resource_list, size);
		if (get_trace_level() > 5) {
			write_status(SCR, LEVEL1|LISTITEM,
				"Configuring resources preferring disk %s",
				disk_name(dp));
		}

		for (count = 0; best_list[count] != NULL; count++) {
			if (get_trace_level() > 5) {
				write_status(SCR, LEVEL1|LISTITEM|CONTINUE,
					Resobj_Name(best_list[count]));
			}

			(void) SliceobjConfigResobj(best_list[count], dp);
		}

		/* configure all unconfigured resources which prefer no disk */
		resource_list = ResobjGetCandidates(PREFER_NODISK, NULL);
		best_list = ResobjFindBestFit(resource_list, size);
		if (get_trace_level() > 5) {
			write_status(SCR, LEVEL1|LISTITEM,
				"Configuring resources preferring no disk");
		}

		for (count = 0; best_list[count] != NULL; count++) {
			if (get_trace_level() > 5) {
				write_status(SCR, LEVEL1|LISTITEM|CONTINUE,
					Resobj_Name(best_list[count]));
			}

			(void) SliceobjConfigResobj(best_list[count], dp);
		}

		/*
		 * configure all unconfigured resources which prefer any
		 * disks
		 */
		resource_list = ResobjGetCandidates(0, NULL);
		best_list = ResobjFindBestFit(resource_list, size);
		if (get_trace_level() > 5) {
			write_status(SCR, LEVEL1|LISTITEM,
				"Configuring resources preferring any disk");
		}

		for (count = 0; best_list[count] != NULL; count++) {
			if (get_trace_level() > 5) {
				write_status(SCR, LEVEL1|LISTITEM|CONTINUE,
					Resobj_Name(best_list[count]));
			}

			(void) SliceobjConfigResobj(best_list[count], dp);
		}

		front = end;
	}
}

/*
 * Function:	SyncConfig
 * Description:	Make sure that any resource which has allocated space on
 *		the disks is marked as an independent resource, and that
 *		optimal adoption is made.
 * Scope:	private
 * Parameters:	none
 * Return:	D_OK		layout successful
 *		D_FAILED	layout failure
 */
static int
SyncConfig(void)
{
	ResobjHandle	res;
	SliceKey *	key;
	Disk_t *	dp;
	int		slice;
	int		size;
	int		swapcnt = 0;
	int		unnamedcnt = 0;
	int		Preserved;

	if (get_trace_level() > 0) {
		write_status(SCR, LEVEL1,
		    "Syncing resource objects with disk configuration");
	}

	/*
	 * THIS IS A BRIDGE UNTIL INITIAL INSTALL CAN FULL UTILIZE RESOURCES:
	 * We need to make sure that all slices have an instance number
	 * assigned which isn't VAL_UNSPECIFIED
	 */
	WALK_SWAP_LIST(res)
		swapcnt++;

	WALK_DISK_LIST(dp) {
		if (!sdisk_is_usable(dp))
			continue;

		WALK_SLICES(slice) {
			if (Sliceobj_Instance(CFG_CURRENT, dp, slice) !=
					VAL_UNSPECIFIED)
				continue;

			/* set file systems to instance '0' */
			if (NameIsPath(Sliceobj_Use(CFG_CURRENT, dp, slice))) {
				Sliceobj_Instance(CFG_CURRENT, dp, slice) = 0;
			} else if (NameIsSwap(Sliceobj_Use(
					CFG_CURRENT, dp, slice))) {
				while (SliceobjFindUse(CFG_CURRENT, NULL,
						SWAP, swapcnt, TRUE) != NULL)
					swapcnt++;
				Sliceobj_Instance(CFG_CURRENT, dp, slice) =
					swapcnt++;
			} else if (Sliceobj_Size(CFG_CURRENT, dp, slice) > 0 &&
					NameIsNull(Sliceobj_Use(
						CFG_CURRENT, dp, slice))) {
				while (SliceobjFindUse(CFG_CURRENT, NULL,
						SWAP, unnamedcnt,
						TRUE) != NULL)
					unnamedcnt++;
				Sliceobj_Instance(CFG_CURRENT, dp, slice) =
					unnamedcnt++;
			}
		}
	}

	WALK_RESOURCE_LIST(res, RESTYPE_UNDEFINED) {
		if ((key = SliceobjFindUse(CFG_CURRENT, NULL, Resobj_Name(res),
				Resobj_Instance(res), TRUE)) != NULL) {

			if (SliceobjGetAttribute(CFG_CURRENT,
			    key->dp,
			    key->slice,
			    SLICEOBJ_PRESERVED, &Preserved,
			    NULL)) {
				return (D_FAILED);
			}

			if (!Preserved) {
				if (!ResobjIsIndependent(res)) {
					if (ResobjSetAttribute(res,
					    RESOBJ_STATUS, RESSTAT_INDEPENDENT,
					    NULL) != D_OK)
						return (D_FAILED);
				}

				/*
				 * make sure you can at least fit the
				 * default size of the basic resource
				 * with no adoption; must use default
				 * because this is in Autolayout and
				 * that is always default
				 */

				size = ResobjGetContent(res, ADOPT_NONE,
				    RESSIZE_DEFAULT);
				if (size > Sliceobj_Size(CFG_CURRENT,
				    key->dp, key->slice))
					return (D_NOSPACE);

				if (size < Sliceobj_Size(CFG_CURRENT,
				    key->dp, key->slice)) {
					ResobjAdoptDependents(res, key->dp,
					    key->slice);
				}
			}
			if (get_trace_level() > 0) {
				write_status(SCR, LEVEL1|LISTITEM,
				    "Resource %s is already configured",
				    Resobj_Name(res));
			}
		}
	}

	return (D_OK);
}

/*
 * Function:	AdvanceBootobj
 * Description:	Advance the boot object disk to the next available disk,
 *		and reset the boot device, all in the current configuration.
 *		If the disk is explicit, fail. If reset is specified, then
 *		restart the advance by reinitializing the default boot
 *		object.
 * Scope:	private
 * Parameters:	reset	[RO] (int)
 *			Specify if the walking mechnanism should be
 *			resarted. Valid values are: TRUE, FALSE
 * Return:	D_OK		layout successful
 *		D_FAILED	layout failure
 */
static int
AdvanceBootobj(int reset)
{
	ResobjHandle	   res;
	static Disk_t *	   curdp = NULL;
	static Disk_t *	   initdp = NULL;
	Disk_t *	   cdp = NULL;
	int		   dev;

	if (get_trace_level() > 5) {
		write_status(SCR, LEVEL1,
			"Advancing the boot object");
	}

	/*
	 * if you have a default "/" record (of "/.cache" on AutoClients),
	 * make sure you keep it's explicit device fields in sync with
	 * the boot object
	 */
	if (get_machinetype() == MT_CCLIENT)
		res = ResobjFind(CACHE, 0);
	else
		res = ResobjFind(ROOT, 0);


	/*
	 * initialize the boot object if undefined or a reset is
	 * explicitly requested
	 */
	(void) DiskobjFindBoot(CFG_CURRENT, &cdp);
	if (cdp == NULL || reset == 1) {
		if ((initdp = InitBootobjDisk()) == NULL) {
			if (get_trace_level() > 5) {
				write_status(SCR, LEVEL1|LISTITEM,
					"Could not initialize boot object");
			}
			return (D_FAILED);
		}

		curdp = initdp;
		if (get_trace_level() > 5) {
			write_status(SCR, LEVEL1|LISTITEM,
				"Initializing the  boot disk to %s",
				disk_name(curdp));
		}
	} else {
		/*
		 * we are no in advance code; if the disk is explicitly set, it
		 * can't be advanced
		 */
		if (BootobjIsExplicit(CFG_CURRENT, BOOTOBJ_DISK_EXPLICIT)) {
			if (get_trace_level() > 5) {
				write_status(SCR, LEVEL1|LISTITEM,
				    "Could not advance explicit boot object");
			}
			return (D_FAILED);
		}

		/*
		 * if the "/" resource has an explicit disk, you can't advance
		 */

		/*
		 * if the current disk pointer isn't set, start walking the
		 * disk list from the start; look for a disk with a usable
		 * sdisk
		 */
		curdp = (curdp == initdp ? first_disk() : next_disk(curdp));
		for (; curdp != NULL; curdp = next_disk(curdp)) {
			if (curdp != initdp && sdisk_is_usable(curdp))
				break;
		}

		/*
		 * if there are no more disks, it can't be advanced, otherwise,
		 * update the current boot disk value
		 */
		if (curdp == NULL) {
			if (get_trace_level() > 5) {
				write_status(SCR, LEVEL1|LISTITEM,
			    "Could not advance the boot disk - no more disks");
			}
			return (D_FAILED);
		}

		if (get_trace_level() > 5) {
			write_status(SCR, LEVEL1|LISTITEM,
				"Setting the  boot disk to %s",
				disk_name(curdp));
		}
	}

	if (BootobjSetAttribute(CFG_CURRENT,
			BOOTOBJ_DISK, disk_name(curdp),
			NULL) != D_OK) {
		if (get_trace_level() > 5)
			write_status(SCR, LEVEL1|LISTITEM,
				"Failed to update boot disk");
		return (D_FAILED);
	}

	if (ResobjSetAttribute(res,
			RESOBJ_DEV_EXPLDISK, disk_name(curdp),
			NULL) != D_OK) {
		if (get_trace_level() > 5)
			write_status(SCR, LEVEL1|LISTITEM,
			    "Failed to update root resource explicit disk");
		return (D_FAILED);
	}

	/*
	 * Set the boot device. If the boot device is not explicitly
	 * set, update the current boot device based on system type:
	 *   (1) On SPARC, set to '0'
	 *   (2) On Intel, set to the partition index of the Solaris
	 *	 partition
	 *   (3) On PowerPC, set to the partition index of the DOS
	 *	 partition
	 */
	if (!BootobjIsExplicit(CFG_CURRENT, BOOTOBJ_DEVICE_EXPLICIT)) {
		if (IsIsa("sparc")) {
			dev = 0;
		} else if (IsIsa("i386")) {
			dev = get_solaris_part(curdp, CFG_CURRENT);
			dev = (valid_fdisk_part(dev) ? dev : -1);
		} else if (IsIsa("ppc")) {
			(void) ResobjSetAttribute(res,
				RESOBJ_DEV_EXPLDEVICE,  0,
				NULL);
			WALK_PARTITIONS(dev) {
				if (part_id(curdp, dev) == DOSOS12 ||
					part_id(curdp, dev) == DOSOS16)
					break;
			}
			dev = (valid_fdisk_part(dev) ? dev : -1);
		}
		(void) BootobjSetAttribute(CFG_CURRENT,
			BOOTOBJ_DEVICE,  dev,
			NULL);
	} else {
		if (IsIsa("sparc")) {
			if (BootobjGetAttribute(CFG_CURRENT,
					BOOTOBJ_DEVICE,	&dev,
					NULL) == D_OK) {
				(void) ResobjSetAttribute(res,
					RESOBJ_DEV_EXPLDEVICE,  dev,
					NULL);
			}
		} else if (IsIsa("ppc")) {
			(void) ResobjSetAttribute(res,
				RESOBJ_DEV_EXPLDEVICE,  0,
				NULL);
		}
	}

	return (D_OK);
}

/*
 * Function:	InitBootobjDisk
 * Description:	Set the current boot object disk to a well-known default
 *		in preparation for sdisk autolayout. If the boot device
 *		is a partition, only the disk need be set. Order of precedence
 *		in selection is:
 *		  (1) explicit boot disk
 *		  (2) explicit "/" disk resource
 *		  (3) existing boot disk
 * Scope:	private
 * Parameters:	none
 * Return:	NULL	No more disks available for the boot object
 *		!NULL	Pointer to disk object associated with boot disk
 *			initially selected
 */
static Disk_t *
InitBootobjDisk(void)
{
	ResobjHandle	res;
	Disk_t *	dp;
	char		disk[32] = "";
	char		cdisk[32];
	char		edisk[32];
	char		etype;
	int		cdev;
	int		edev;

	/*
	 * get the current and original boot object disk and device
	 * configurations
	 */
	if (BootobjGetAttribute(CFG_CURRENT,
			BOOTOBJ_DISK,	cdisk,
			BOOTOBJ_DEVICE,	&cdev,
			NULL) != D_OK ||
		BootobjGetAttribute(CFG_EXIST,
			BOOTOBJ_DISK,	edisk,
			BOOTOBJ_DEVICE,	&edev,
			BOOTOBJ_DEVICE_TYPE, &etype,
			NULL) != D_OK) {
		return (NULL);
	}

	if (BootobjIsExplicit(CFG_CURRENT, BOOTOBJ_DISK_EXPLICIT)) {
		if ((dp = find_disk(cdisk)) == NULL || !sdisk_is_usable(dp))
			return (NULL);

		return (dp);
	}

	if ((res = ResobjFind(ROOT, 0)) != NULL &&
			ResobjGetAttribute(res,
				RESOBJ_DEV_EXPLDISK, disk,
				NULL) == D_OK &&
			disk[0] != '\0') {
		if ((dp = find_disk(disk)) == NULL || !sdisk_is_usable(dp))
			return (NULL);

		(void) BootobjSetAttribute(CFG_CURRENT,
			BOOTOBJ_DISK, disk,
			NULL);
		return (dp);
	}

	if ((dp = find_disk(edisk)) != NULL && sdisk_is_usable(dp)) {
		(void) BootobjSetAttribute(CFG_CURRENT,
			BOOTOBJ_DISK, edisk,
			NULL);
		return (dp);
	}

	WALK_DISK_LIST(dp) {
		if (sdisk_is_usable(dp)) {
			(void) BootobjSetAttribute(CFG_CURRENT,
				BOOTOBJ_DISK, disk_name(dp),
				NULL);
			return (dp);
		}
	}

	return (NULL);
}
