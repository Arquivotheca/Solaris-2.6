#ifndef lint
#pragma ident "@(#)svc_resource.c 1.31 96/09/26 SMI"
#endif

/*
 * Copyright (c) 1995-1996 by Sun Microsystems, Inc. All rights reserved.
 */
/*
 * Module:	svc_resource.c
 * Group:	libspmisvc
 * Description:	Routines for manipulating and querying sizes of resources.
 */
#include <libgen.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/param.h>
#include <sys/types.h>
#include "spmisvc_lib.h"
#include "spmicommon_lib.h"
#include "spmistore_lib.h"

/* private prototypes */

static int	coreResobjStorageSwap(ResobjHandle, ResSize_t);
static int	coreResobjContentCache(ResobjHandle, ResSize_t);
static int	coreResobjContentDirectory(ResobjHandle, Adopt_t, ResSize_t);
static int	coreSliceobjSumSwap(Disk_t *, u_char);

/* ---------------------- public functions ----------------------- */

/*
 * Function:	ResobjGetSwap
 * Description:	Calculate the swap space required. The default value is
 *		calculated based on physical memory size, where:
 *
 *			  physical		 swap
 *			  --------		 ----
 *			  0 -  64  MB		 32 MB
 *			 64 - 128  MB		 64 MB
 *			128 - 512  MB		128 MB
 *			   > 512   MB		256 MB
 *
 *		The results of This calculation can be modified by explicitly
 *		setting the global swap value (using SYS_SWAPSIZE), or by
 *		setting the SYS_MEMSIZE environment variable, which overrides
 *		the system memory size but uses the default heuristic.
 *
 *		Minimum swap size requires the system have at least 32 MB
 *		of virtual memory (physical + swap).
 * Scope:	public
 * Parameters:	flag	[RO] (ResSize_t)
 *			Valid values are:
 *			    RESSIZE_DEFAULT	default sizing
 *			    RESSIZE_MINIMUM	minimum required sizing
 * Return:	# >= 0	Number of sectors required
 */
int
ResobjGetSwap(ResSize_t flag)
{
	int	swap;
	int	size;
	int	mem = sectors_to_mb(SystemGetMemsize());

	/* validate parameters */
	if (flag != RESSIZE_DEFAULT && flag != RESSIZE_MINIMUM)
		return (0);

	/* see if the user specified an explicit swap size */
	if (GlobalGetAttribute(GLOBALOBJ_SWAP, &swap) == D_OK &&
			swap != VAL_UNSPECIFIED) {
		if ((size = swap) < 0)
			size = 0;
	} else {
		if (flag == RESSIZE_DEFAULT) {
			/* default heuristic */
			if (mem < 64)
				size = 32;
			else if (mem >= 64 && mem < 128)
				size = 64;
			else if (mem >= 128 && mem < 512)
				size = 128;
			else
				size = 256;
		} else {
			size = 32 - mem;
			if (size < 0)
				size = 0;
		}

		size = mb_to_sectors(size);
	}

	return (size);
}

/*
 * Function:	ResobjGetContent
 * Description:	Calculate the content size required for a specified resource.
 *		This function does not take into account minimal file system
 *		sizes; only content requirements. The caller may indicate
 *		if optional child resources should be included in the tabula-
 *		tion, and whether default, minimum, or no expansion overhead
 *		should be included. Minimum sizing adds su-only overhead to
 *		all content fields. Default sizing adds su-only overhead
 *		to all content fields, and also adds the percent free overhead
 *		to the software field.
 * Scope:	public
 * Parameters:	res	[RO] (ResobjHandle)
 *			Resource object handle for resource to be tabulated.
 *		adopt	[RO] (Adopt_t)
 *			Valid values are:
 *			  ADOPT_ALL	account for the content of all children
 *			  ADOPT_NONE	account for own content (no children)
 *		flag	[RO] (ResSize_t)
 *			Valid values are:
 *			  RESSIZE_DEFAULT	default sizing with expansion
 *			  RESSIZE_MINIMUM	required sizing to hold content
 *			  RESSIZE_NONE		no overhead added
 * Return:	0	no space required or bad argument
 *		# > 0	number of sectors required
 */
int
ResobjGetContent(ResobjHandle res, Adopt_t adopt, ResSize_t flag)
{
	int	size;

	/* validate parameters */
	if (!ResobjIsExisting(res) ||
			(adopt != ADOPT_NONE && adopt != ADOPT_ALL) ||
			(flag != RESSIZE_MINIMUM &&
				flag != RESSIZE_DEFAULT &&
				flag != RESSIZE_NONE))
		return (0);

	/* update the resource content data */
	(void) ResobjUpdateContent();

	size = 0;
	if (flag == RESSIZE_DEFAULT) {
		switch (Resobj_Type(res)) {
		    case RESTYPE_UNNAMED:
		    case RESTYPE_DIRECTORY:
			if (streq(Resobj_Name(res), CACHE) &&
					Resobj_Instance(res) == 0) {
				size = coreResobjContentCache(res, flag);
			} else {
				size = coreResobjContentDirectory(res,
						adopt, flag);
			}
			break;
		    case RESTYPE_SWAP:
			size = coreResobjStorageSwap(res, flag);
			break;
		}
	} else {
		switch (Resobj_Type(res)) {
		    case RESTYPE_UNNAMED:
		    case RESTYPE_DIRECTORY:
			if (streq(Resobj_Name(res), CACHE) &&
					Resobj_Instance(res) == 0)
				size = coreResobjContentCache(res, flag);
			else
				size = coreResobjContentDirectory(res,
						adopt, flag);
			break;
		    case RESTYPE_SWAP:
			size = coreResobjStorageSwap(res, flag);
			break;
		}
	}

	return (size);
}

/*
 * Function:	ResobjGetStorage
 * Description:	Calculate the slice size required for a specified resource.
 *		The caller may indicate if optional child resources should be
 *		included in the tabulation, and whether default or minimum
 *		expansion overhead should be included. Minimum sizing adds
 *		su-only overhead to all content fields. Default sizing adds
 *		su-only overhead to all content fields, and also adds the
 *		percent free overhead to the software field, and ensures that
 *		directories are no less that 10 MB.
 * Scope:	public
 * Parameters:	res	[RO] (ResobjHandle)
 *			Resource object handle for resource to be tabulated.
 *		adopt	[RO] (Adopt_t)
 *			Valid values are:
 *			  ADOPT_ALL	account for the content of all children
 *			  ADOPT_NONE	account for own content (no children)
 *		flag	[RO] (ResSize_t)
 *			Flag indicating how much sizing overhead should be added
 *			to the tabulated value. Valid values are:
 *			  RESSIZE_DEFAULT	default sizing with expansion
 *			  RESSIZE_MINIMUM	required sizing to hold content
 * Return:	0	no space required or bad argument
 *		# > 0	number of sectors required
 */
int
ResobjGetStorage(ResobjHandle res, Adopt_t adopt, ResSize_t flag)
{
	int	size;

	/* validate parameters */
	if (!ResobjIsExisting(res) ||
			(adopt != ADOPT_NONE && adopt != ADOPT_ALL) ||
			(flag != RESSIZE_MINIMUM && flag != RESSIZE_DEFAULT))
		return (0);

	/* update the resource content data */
	(void) ResobjUpdateContent();

	/* explicit size has full override */
	if (Resobj_Dev_Explsize(res) >= 0)
		return (Resobj_Dev_Explsize(res));

	/*
	 * if the user specifies an explicit minimum and the content
	 * is less than that value, bump up the value to at least the
	 * minimum
	 */
	size = ResobjGetContent(res, adopt, flag);
	if (Resobj_Dev_Explmin(res) > size)
		size = Resobj_Dev_Explmin(res);

	/*
	 * if this is a directory and we are doing default sizing, the
	 * size must be at least 10 MB; bump up if necessary
	 */
	if (ResobjIsDirectory(res) && flag == RESSIZE_DEFAULT &&
			size < mb_to_sectors(10) && size > 0)
		return (mb_to_sectors(10));

	return (size);
}

/*
 * Function:	get_default_fs_size (OBSOLETE)
 * Description:	Calculate the default slice size for the default resource
 *		specified. This function uses ResobjGetStorage() for its
 *		actual storage calculation. Instance '0' is assumed for
 *		all resources.
 * Scope:	public
 * Parameters:	name	[RO, *RO] (char *)
 *			Name of default resource for tabulation.
 *		dp	[RO, *RO] (Disk_t *)
 *			Unused.
 *		roll	[RO] (int)
 *			Specify if optional child resources should be
 *			included in the tabulation. Valid values are:
 *			  DONTROLLUP	don't include optional children
 *			  ROLLUP	include optional children
 * Return:	0	no sectors required, or bad argument
 *		#	default slice size in sectors
 */
/*ARGSUSED0*/
int
get_default_fs_size(char *name, Disk_t *dp, int roll)
{
	ResobjHandle	res;
	Adopt_t		adopt;
	int		size;

	/* validate parameters */
	if ((res = ResobjFind(name, 0)) == NULL)
		return (0);

	if (roll == ROLLUP)
		adopt = ADOPT_ALL;
	else if (roll == DONTROLLUP)
		adopt = ADOPT_NONE;
	else
		return (0);

	size = ResobjGetStorage(res, adopt, RESSIZE_DEFAULT);
	return (size);
}

/*
 * Function:	get_minimum_fs_size (OBSOLETE)
 * Description:	Calculate the minimum slice size for the default resource
 *		specified. This function uses ResobjGetContent() for its
 *		actual storage calculation. Instance '0' is assumed for
 *		all resources.
 * Scope:	public
 * Parameters:	name	[RO, *RO] (char *)
 *			Name of default resource for tabulation.
 *		dp	[RO, *RO] (Disk_t *)
 *			Unused.
 *		roll	[RO] (int)
 *			Specify if optional child resources should be
 *			included in the tabulation. Valid values are:
 *			  DONTROLLUP	don't include optional children
 *			  ROLLUP	include optional children
 * Return:	0	no sectors required, or bad argument
 *		#	minimum slice size in sectors
 */
/*ARGSUSED0*/
int
get_minimum_fs_size(char *name, Disk_t *dp, int roll)
{
	ResobjHandle	res;
	int		size;
	Adopt_t		adopt;

	/* validate parameters */
	if ((res = ResobjFind(name, 0)) == NULL)
		return (0);

	if (roll == ROLLUP)
		adopt = ADOPT_ALL;
	else if (roll == DONTROLLUP)
		adopt = ADOPT_NONE;
	else
		return (0);

	size = ResobjGetContent(res, adopt, RESSIZE_MINIMUM);
	return (size);
}

/*
 * Function:	set_client_space
 * Description:	Set the client root and swap space expansion requirements for
 *		/export/swap (instance 0) and /export/root (instance 0).
 * Scope:	public
 * Parameters:	num	[RO] (int)
 *			Number of clients (valid: # >= 0).
 *		root	[RO] (int)
 *			Size of a single client root in sectors (valid: # >= 0)
 *		swap	[RO] (int)
 *			Size of a single client swap in sectors (valid: # >= 0)
 * Return:	D_OK	   set successful
 *		D_BADARG   invalid argument
 *		D_FAILED   internal error
 */
int
set_client_space(int num, int root, int swap)
{
	ResobjHandle	res;

	/* validate parameters */
	if (num < 0 || root < 0 || swap < 0)
		return (D_BADARG);

	/*
	 * get the default /export/swap entry and set the services content
	 * value
	 */
	if ((res = ResobjFind(EXPORTSWAP, 0)) == NULL ||
		    ResobjSetAttribute(res,
			RESOBJ_CONTENT_SERVICES,  num * swap,
			NULL) != D_OK) {
		return (D_FAILED);
	}

	/* set the default /export/root instance 0 resource services content */
	if ((res = ResobjFind(EXPORTROOT, 0)) == NULL ||
		    ResobjSetAttribute(res,
			RESOBJ_CONTENT_SERVICES,  num * root,
			NULL) != D_OK) {
		return (D_FAILED);
	}

	return (D_OK);
}

/*
 * Function:	SliceobjSumSwap
 * Description:	Add up the number of sectors allocated to slices with a use
 *		specifier of "swap". Summations can be limited to a single disk,
 *		or can include all selected disks with valid sdisk geometries.
 * Scope:	public
 * Parameters:	dp	[RO, *RO] (Disk_t *) - (OPTIONAL)
 *			Disk object pointer which can be optionally used to
 *			limit the summarization to a single disk. NULL for all
 *			usable (selected, valid sdisk geom pointer) disks to be
 *			processed.
 *		flags	[RO] (u_char)
 *			Search constraint flags used for summarization.  Valid
 *			values are:
 *			    SWAPALLOC_RESONLY	  account only for swap defined
 *						  in resources
 *			    SWAPALLOC_RESEXCLUDE  account only for swap defined
 *						  in disk slices
 *			    SWAPALLOC_ALL	  account for all swap
 * Return:	# > 0	number of sectors allocated to swap according to the
 *			disk configuration and constraint flags provided
 */
int
SliceobjSumSwap(Disk_t *dp, u_char flags)
{
	int	total = 0;

	if (dp == NULL) {
		WALK_DISK_LIST(dp) {
			if (sdisk_is_usable(dp))
				total += coreSliceobjSumSwap(dp, flags);
		}
	} else {
		if (sdisk_is_usable(dp))
			total += coreSliceobjSumSwap(dp, flags);
	}

	return (total);
}

/* ---------------------- internal functions ----------------------- */

/*
 * Function:	_filesys_boot_critical
 * Description:	Look to see if the specified mount point is:
 *		(1) server:	/ /usr /var
 *		(2) autoclient:	/.cache.
 * Scope:	internal
 * Parameters:	name	[RO, *RO] (char *)
 *			Name of directory.
 * Return:	0	mount point is not one of the listed file systems
 *		1	mount point is one of the listed file systems
 */
int
_filesys_boot_critical(char *name)
{
	/* validate parameters */
	if (!NameIsPath(name))
		return (0);

	if (get_machinetype() == MT_CCLIENT) {
		if (streq(name, CACHE))
			return (1);
	} else {
		if (streq(name, ROOT) ||
				streq(name, USR) || streq(name, VAR))
			return (1);
	}

	return (0);
}

/*
 * Function:	ResobjIsGuardian
 * Description:	Determine if the parent resource can be a legitimate
 *		guardian for the child resource. Both parent and child
 *		must have path names for resource names. The parent
 *		resource must be an independent resource. Only child
 *		resources at the same instance level as the parent are
 *		considered "adoptable" by the parent. Viable child
 *		resources cannot be independent.
 * Scope:	internal
 * Parameters:	parent	[RO] (ResobjHandle)
 *			Parent resource object handle.
 *		child	[RO] (ResobjHandle)
 *			Child resource object handle.
 * Return:	0	the parent is not a legitimate guardian
 * 		1	the parent is a legitimate guardian
 */
int
ResobjIsGuardian(ResobjHandle parent, ResobjHandle child)
{
	ResobjHandle	res;
	char *		name;
	int		status;

	/* validate parameters */
	if (!ResobjIsExisting(parent) || !ResobjIsExisting(child))
		return (0);

	/* both resources must have paths for names */
	if (!NameIsPath(Resobj_Name(parent)) ||
			!NameIsPath(Resobj_Name(child)))
		return (0);

	/* only independent resources can be parents */
	if (!ResobjIsIndependent(parent))
		return (0);

	/* only non-independent children can have guardians */
	if (ResobjIsIndependent(child))
		return (0);

	/* the child must be farther down in the file system namespace */
	if ((int) strlen(Resobj_Name(child)) <
			(int) strlen(Resobj_Name(parent)))
		return (0);

	/* the child must be at the same instance level as the parent */
	if (Resobj_Instance(parent) != Resobj_Instance(child))
		return (0);

	name = xstrdup(Resobj_Name(child));
	status = 0;
	do {
		if ((int) strlen(name) < (int) strlen(Resobj_Name(parent)))
			break;

		/*
		 * if we've collapsed all the way to the parent, we've
		 * made a match
		 */
		if ((res = ResobjFind(name, Resobj_Instance(child))) != NULL &&
				ResobjIsIndependent(res)) {
			if (res == parent)
				status = 1;
			break;
		} else {
			if (streq(name, ROOT)) {
				name[0] = '\0';
			} else {
				(void) dirname(name);
				if (name[0] == '\0')
					(void) strcpy(name, ROOT);
			}
		}
	} while (status == 0 && name[0] != '\0');

	free(name);
	return (status);
}

/* ---------------------- private functions ----------------------- */

/*
 * Function:	coreResobjContentCache
 * Description:	Calculate the default cache file system content size.
 *		(1)  Look on the boot disk to see if there is sufficient usable
 *			space to fit a minimum sized /.cache
 *		(2)  If '(1)' fails, look across all disks to find the largest
 *			available segment of disk
 * Scope:	private
 * Parameters:	res	[RO] (ResobjHandle)
 *			Resource object handle to use in tabulation.
 *		flag	[RO] (ResSize_t)
 *			Flag indicating how much sizing overhead should be added
 *			to the tabulated value. Valid values are:
 *			  RESSIZE_DEFAULT	default sizing with expansion
 *			  RESSIZE_MINIMUM	required sizing to hold content
 *			  RESSIZE_NONE		same as RESSIZE_MINIMUM
 * Return:	# >= 0 	size in sectors
 */
static int
coreResobjContentCache(ResobjHandle res, ResSize_t flag)
{
	Disk_t *	dp;
	int		min;
	int		avail;
	int		size = VAL_UNSPECIFIED;
	MachineType	mt = get_machinetype();

	/* default /.cache resource is only supported on AutoClient systems */
	if (mt != MT_CCLIENT)
		return (0);

	if (flag == RESSIZE_DEFAULT) {
		/*
		 * if you're not careful here you'll end up in an
		 * infinite loop
		 */
		min = coreResobjContentCache(res, RESSIZE_MINIMUM);

		if (DiskobjFindBoot(CFG_CURRENT, &dp) != D_OK || dp == NULL)
			(void) DiskobjFindBoot(CFG_EXIST, &dp);

		/*
		 * if there is a disk at this point, check to see if
		 * it has enough contiguous space to meet minimum
		 * requirements; this is the "disk of preference"
		 */
		if (dp != NULL) {
			if (sdisk_is_usable(dp)) {
				avail = sdisk_max_hole_size(dp);
				if (avail >= min)
					return (avail);
			}
		} else {
			/*
			 * if there is not explicitly specified disk and the
			 * boot disk either isn't usable or doesn't have
			 * enough space to meet minimum requirements, find
			 * the disk with the most contiguous space available
			 */
			WALK_DISK_LIST(dp) {
				if (sdisk_is_usable(dp)) {
					avail = sdisk_max_hole_size(dp);
					if (avail > size)
						size = avail;
				}
			}
		}

		/*
		 * make sure you return at least the minimum required space
		 * as a default value, even if there isn't a disk big enough
		 * to hold it
		 */
		if (size < min)
			size = min;
	} else {
		/*
		 * find out the total amount of swap still required
		 */
		if (GlobalGetAttribute(GLOBALOBJ_SWAP, (void *)&size) != D_OK ||
				size == VAL_UNSPECIFIED)
			size = mb_to_sectors(32);

		size -= SliceobjSumSwap(NULL, NULL);
		if (size < 0)
			size = 0;

		/*
		 * add the swap required to the minimum for the floor
		 */
		size += mb_to_sectors(24);
	}

	return (size);
}

/*
 * Function:	coreResobjContentDirectory
 * Description:	Calculate the content size required for a directory resource.
 *		The caller may indicate if optional child resources should be
 *		included in the tabulation, and whether default, minimum, or
 *		no expansion overhead should be included. Minimum sizing adds
 *		su-only overhead to all content fields. Default sizing adds
 *		su-only overhead to all content fields, and also adds the
 *		percent free overhead to the software field.
 * Scope:	private
 * Parameters:	res	[RO] (ResobjHandle)
 *			Resource object handle to use in tabulation.
 *		adopt	[RO] (Adopt_t)
 *			Valid values are:
 *			  ADOPT_ALL	account for the content of all children
 *			  ADOPT_NONE	account for own content (no children)
 *		flag	[RO] (ResSize_t)
 *			Flag indicating how much sizing overhead should be added
 *			to the tabulated value. Valid values are:
 *			  RESSIZE_DEFAULT	default sizing with expansion
 *			  RESSIZE_MINIMUM	required sizing to hold content
 *			  RESSIZE_NONE		same as RESSIZE_MINIMUM
 * Return:	# >= 0	number of sectors required by default for the
 *			resource as specified
 */
static int
coreResobjContentDirectory(ResobjHandle res, Adopt_t adopt, ResSize_t flag)
{
	ResobjHandle	tmp;
	int		software;
	int		extra;
	int		services;
	ulong		total;
	ulong		size;
	int		mfree;
	int		pfree;
	ulong		psize;
	ulong		msize;
	ulong		osize;
	char		objectname[MAXNAMELEN];

	/* validate parameters */
	if (ResobjIsIgnored(res) || ResobjIsDependent(res))
		return (0);

	if (adopt != ADOPT_ALL && adopt != ADOPT_NONE)
		return (0);

	if (flag != RESSIZE_DEFAULT &&
			flag != RESSIZE_MINIMUM && flag != RESSIZE_NONE)
		return (0);

	/* get the current expansion percentage settings */
	if (ResobjGetAttribute(res,
			RESOBJ_CONTENT_SOFTWARE, &software,
			RESOBJ_CONTENT_EXTRA,	 &extra,
			RESOBJ_CONTENT_SERVICES, &services,
			RESOBJ_FS_PERCENTFREE,	 &pfree,
			RESOBJ_FS_MINFREE,	 &mfree,
			RESOBJ_NAME,		 objectname,
			NULL) != D_OK) {
		return (0);
	}

	total = 0;

	/* initialize free percentages */
	switch (flag) {
	    case RESSIZE_MINIMUM:
		mfree = (mfree == VAL_UNSPECIFIED ? 10 : mfree);
		pfree = 0;
		break;
	    case RESSIZE_DEFAULT:
		mfree = (mfree == VAL_UNSPECIFIED ? 10 : mfree);
		pfree = (pfree == VAL_UNSPECIFIED ? 15 : pfree);
		break;
	    case RESSIZE_NONE:
		mfree = 0;
		pfree = 0;
		break;
	}
	if (get_trace_level() > 5)
		write_message(LOG, STATMSG, LEVEL0,
		    "%s: software %ld, services %ld, extra %ld: "
		    "Free %ld%%, Min %ld%%",
		    objectname, sectors_to_mb(software),
		    sectors_to_mb(services), sectors_to_mb(extra),
		    pfree, mfree);

	/* add expansion and su-free for software */
	size = software;
	(void) new_slice_size(sectors_to_kb(size),
			(ulong)pfree, (ulong)mfree,
			&psize, &msize, &osize);
	total += size + kb_to_sectors(psize) + kb_to_sectors(msize);
	if (get_trace_level() > 5)
		write_message(LOG, STATMSG, LEVEL1, "Space for software %ld",
		    sectors_to_mb(size) + kb_to_mb(psize) + kb_to_mb(msize));

	/* add only su-free to services and extra */
	size = services + extra;
	(void) new_slice_size(sectors_to_kb(size),
			0, (ulong)mfree,
			&psize, &msize, &osize);
	total += size + kb_to_sectors(msize);
	if (get_trace_level() > 5)
		write_message(LOG, STATMSG, LEVEL1,
		    "Space for services + extra %ld",
		    sectors_to_mb(size) + kb_to_mb(msize));

	/* optional resources do not process children, so add up and return */
	if (ResobjIsOptional(res)) {
		/*
		 * if adopt/all is specified for an optional resource, return
		 * 0 because it is assumed in adoption, another resource
		 * will account for the space of this resource
		 */
		if (adopt == ADOPT_ALL)
			total = 0;

		/* add UFS overhead when applicable */
		if (total != 0 && flag != RESSIZE_NONE) {
			(void) new_slice_size(sectors_to_kb(total),
				0, 0,
				&psize, &msize, &osize);
			total = total + kb_to_sectors(osize);
			if (get_trace_level() > 5)
				write_message(LOG, STATMSG, LEVEL1,
				    "Space for UFS overhead %ld",
				    kb_to_mb(osize));
		}

		if (get_trace_level() > 5)
			write_message(LOG, STATMSG, LEVEL1,
			    "== Total: %d MB", sectors_to_mb(total));
		return ((int)total);
	}

	/*
	 * the resource is RESSTAT_INDEPENDENT; include the space for
	 * all RESORIGIN_DEFAULT resources for which 'name' is an
	 * immediate ancestor, and which are not themselves independent;
	 * take the adoption parameter into account when making the
	 * determination
	 */
	WALK_DIRECTORY_LIST(tmp) {
		if (Resobj_Origin(tmp) != RESORIGIN_DEFAULT)
			continue;

		/* don't duplicate it's own space */
		if (res == tmp)
			continue;

		/* only count optinal children if adopting */
		if (adopt == ADOPT_NONE && ResobjIsOptional(tmp))
			continue;

		/* if this is a dependent child, add its space */
		if (ResobjIsGuardian(res, tmp)) {
			if (ResobjGetAttribute(tmp,
					RESOBJ_CONTENT_SOFTWARE, &software,
					RESOBJ_CONTENT_EXTRA,	 &extra,
					RESOBJ_CONTENT_SERVICES, &services,
					RESOBJ_FS_PERCENTFREE,	 &pfree,
					RESOBJ_FS_MINFREE,	 &mfree,
					RESOBJ_NAME,		 objectname,
					NULL) == D_OK) {
				switch (flag) {
				    case RESSIZE_MINIMUM:
					mfree = (mfree == VAL_UNSPECIFIED ?
							10 : mfree);
					pfree = 0;
					break;
				    case RESSIZE_DEFAULT:
					mfree = (mfree == VAL_UNSPECIFIED ?
							10 : mfree);
					pfree = (pfree == VAL_UNSPECIFIED ?
							15 : pfree);
					break;
				    case RESSIZE_NONE:
					mfree = 0;
					pfree = 0;
					break;
				}
				if (get_trace_level() > 5)
					write_message(LOG, STATMSG, LEVEL1,
					    "%s: software %ld, services %ld, "
					    "extra %ld: Free %ld%%, Min %ld%%",
					    objectname,
					    sectors_to_mb(software),
					    sectors_to_mb(services),
					    sectors_to_mb(extra), pfree, mfree);
				/* add expansion and su-free for software */
				size = software;
				(void) new_slice_size(sectors_to_kb(size),
						(ulong)pfree, (ulong)mfree,
						&psize, &msize, &osize);
				total += size + kb_to_sectors(psize) +
						kb_to_sectors(msize);
				if (get_trace_level() > 5)
					write_message(LOG, STATMSG, LEVEL2,
					    "Space for software %ld",
					    sectors_to_mb(size)
					    + kb_to_mb(psize)
					    + kb_to_mb(msize));

				/* add only su-free to services and extra */
				size = services + extra;
				(void) new_slice_size(sectors_to_kb(size),
						0, (ulong)mfree,
						&psize, &msize, &osize);
				total += size + kb_to_sectors(msize);
				if (get_trace_level() > 5)
					write_message(LOG, STATMSG, LEVEL2,
					    "Space for services + extra %ld",
					    sectors_to_mb(size)
					    + kb_to_mb(msize));
			}
		}
	}

	/* add UFS overhead when applicable */
	if (total != 0 && flag != RESSIZE_NONE) {
		(void) new_slice_size(sectors_to_kb(total),
			0, 0,
			&psize, &msize, &osize);
		total = total + kb_to_sectors(osize);
		if (get_trace_level() > 5)
			write_message(LOG, STATMSG, LEVEL1,
			    "Space for UFS overhead %d",
			    kb_to_mb(osize));
	}

	if (get_trace_level() > 5)
		write_message(LOG, STATMSG, LEVEL1, "== Total: %d",
		    sectors_to_mb(total));
	return ((int)total);
}

/*
 * Function:	coreResobjStorageSwap
 * Description:	Calculate the swap space required. The default swap
 *		space cannot be less that 16 MB, or more than 32 MB. The
 *		default value is calculated based on physical memory size,
 *		where:
 *
 *			  physical		 swap
 *			  --------		 ----
 *			  0 -  64  MB		 32 MB
 *			 64 - 128  MB		 64 MB
 *			128 - 512  MB		128 MB
 *			   > 512   MB		256 MB
 *
 *		The value is then truncated if it exceeds 20% of the disk
 *		capacity on which the configuration is being made (if the
 *		calculation is being done in the context of a specific drive).
 *		This is for historical reasons (104 MB disks) and should
 *		probably be dropped entirely in future releases. This calc
 *		can be overridden in this routine by explicit value sets.
 *		The size of swap is calculated, in order of precedence, as:
 *		(1) the explicit SYS_SWAPSIZE environment variable
 *		(3) the value based on above calculation
 * Scope:	private
 * Parameters:	res	[RO] (ResobjHandle)
 *			Resource object handle to use in tabulation.
 *		flag	[RO] (ResSize_t)
 *			Flag indicating how much sizing overhead should be added
 *			to the tabulated value. Valid values are:
 *			  RESSIZE_DEFAULT	default sizing with expansion
 *			  RESSIZE_MINIMUM	required sizing to hold content
 * Return:	# >= 0	number of sectors required by default for the
 *			resource as specified
 */
static int
coreResobjStorageSwap(ResobjHandle res, ResSize_t flag)
{
	ResobjHandle tmp;
	int	size = VAL_UNSPECIFIED;
	int	needed;
	int	existing;
	int	tobeallocated;

	/* validate parameters */
	if (ResobjIsIgnored(res) || ResobjIsDependent(res))
		return (0);

	if (flag != RESSIZE_DEFAULT && flag != RESSIZE_MINIMUM)
		return (0);

	/*
	 * As a temporary "feature", swap resources either have an explicit
	 * size, or one (set by a privileged call) has a VAL_FREE explicit
	 * size
	 */
	if (Resobj_Dev_Explsize(res) == VAL_FREE) {
		needed = ResobjGetSwap(flag);

		/* sum the swap allocated on all currently usable disks */
		existing = SliceobjSumSwap(NULL, SWAPALLOC_RESEXCLUDE);

		/* include independent and non-free, non-zero swaps */
		tobeallocated = 0;
		WALK_SWAP_LIST(tmp) {
			if (Resobj_Dev_Explsize(tmp) <= 0)
				continue;

			if (!ResobjIsIndependent(tmp) &&
					!NameIsPath(Resobj_Name(res)))
				continue;

			if (SliceobjFindUse(CFG_CURRENT, NULL, Resobj_Name(res),
					Resobj_Instance(res), TRUE) != NULL)
				continue;

			tobeallocated +=  Resobj_Dev_Explsize(tmp);
		}

		size = needed - existing - tobeallocated;
	} else if (Resobj_Dev_Explsize(res) < 0) {
		size = 0;
	} else {
		size = Resobj_Dev_Explsize(res);
	}

	/* make sure you always return a legal value */
	if (size < 0)
		size = 0;

	return (size);
}

/*
 * Function:	coreSliceobjSumSwap
 * Description:	Total the amount of swap space which is allocated according
 *		to the current disk slice configuration state of a specific
 *		disk. The caller can restrict the summation to include only
 *		space which is accounted for by a resource object, which is
 *		not accounted for by a resource object, or both.
 * Scope:	private
 * Parameters:	dp	[RO, *RO] (Disk_t *) - (OPTIONAL)
 *			Disk object pointer which specifies the disk to be
 *			used for summarization.
 *		flags	[RO] (u_char)
 *			Search constraint flags used for summarization.  Valid
 *			values:
 *			  SWAPALLOC_RESONLY	count only those which are
 *						accounted for by resources
 *			  SWAPALLOC_RESEXCLUDE	count only those which are
 *						not accounted for by resources
 *			  SWAPALLOC_ALL		count all
 * Return:	# > 0	number of sectors allocated to swap according to the
 *			disk configuration and constraint flags
 */
static int
coreSliceobjSumSwap(Disk_t *dp, u_char flags)
{
	int		total;
	int		ival;
	int		i;

	/* validate parameters */
	if (dp == NULL || !sdisk_is_usable(dp))
		return (0);

	/* walk through all slices looking for swap slices */
	total = 0;
	WALK_SLICES_STD(i) {
		/*
		 * only look at slices which are "swap" and are not
		 * ignored
		 */
		if (!streq(Sliceobj_Use(CFG_CURRENT, dp, i), SWAP) ||
				SliceobjIsIgnored(CFG_CURRENT, dp, i))
			continue;

		/*
		 * if the resource-only flag is set, include the slice size
		 * only if there is a resource with the exact instance value
		 */
		if (flags & SWAPALLOC_RESONLY) {
			ival = Sliceobj_Instance(CFG_CURRENT, dp, i);
			if (ival != VAL_UNSPECIFIED &&
				    ResobjFind(SWAP, ival) != NULL)
				total += Sliceobj_Size(CFG_CURRENT, dp, i);
		}

		/*
		 * if the resource-exclude flag is set, include the slice size
		 * only if there is not a resource with the exact instance
		 * value
		 */
		if (flags & SWAPALLOC_RESEXCLUDE) {
			ival = Sliceobj_Instance(CFG_CURRENT, dp, i);
			if (ival == VAL_UNSPECIFIED ||
					ResobjFind(SWAP, ival) == NULL)
				total += Sliceobj_Size(CFG_CURRENT, dp, i);
		}
	}

	return (total);
}
