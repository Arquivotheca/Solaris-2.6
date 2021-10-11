#ifndef lint
#pragma ident "@(#)svc_sdiskfreespace.c 1.6 96/09/26 SMI"
#endif

/*
 * Copyright (c) 1995-1996 by Sun Microsystems, Inc. All rights reserved.
 */
/*
 * Module:	svc_sdiskfreespace.c
 * Group:	libspmisvc
 * Description:	Sdisk free space algorithms.
 */

#include <ctype.h>
#include <stdio.h>
#include <string.h>
#include <sys/param.h>
#include <sys/types.h>
#include "spmisvc_lib.h"
#include "spmistore_lib.h"
#include "spmicommon_api.h"

/* private prototypes */

static void	FilesysFindUnusedName(char *);
static int	FilesysFindUnusedSlice(Disk_t *, int);
static int	SegmentAbsorbPreferred(Disk_t *, int *, int, int, int);
static int	SegmentAbsorbNonpreferred(Disk_t *, int *, int, int,
			ResOrigin_t);
static int	SegmentAbsorbExistingFree(Disk_t *, int *, int);
static int	SegmentCreateFilesys(Disk_t *, int, int);
static int *	SegmentFindSlices(Disk_t *, int, int);
static int	SegmentCountAllocated(Disk_t *, int *);

/* constants */

#define	MINADD	mb_to_sectors(15)
#define	MAXADD	mb_to_sectors(60)

/* ---------------------- public functions ----------------------- */

/*
 * Function:	SdiskobjAllocateUnused
 * Description:	Distribute unallocated sectors on a disk to existing slices,
 *		or for larger segments, create separate /export/home# file
 *		systems. If a disk has immovable slices, this function
 *		will deal with the space in each segment separately. The
 *		algorithm for allocating space is as follows:
 *
 *		For each segment:
 *
 *		(1) If there are unallocated sectors, allocate space to
 *		    preferred slices, up to 30% of their existing size. This
 *		    includes (but is not limited to) '/', '/var', '/export',
 *		    and '/opt'. If the '/' slice is one of the candidates, it
 *		    will receive extra passes for each of '/opt' and '/var'
 *		    which are not themselves separate file systems on one of
 *		    the disks.
 *		(2) If there are still unallocated sectors, allocate space to
 *		    non-preferred default slices, up to 15% of their existing
 *		    size. This includes (but is not limited to) '/usr' and
 *		    '/usr/openwin'.
 *		(3) If there are still unallocated sectors, allocate space to
 *		    non-preferred non-default slices, up to 15% of their
 *		    existing size. This includes slices which are not in the
 *		    default list and are added by applications explicitly (e.g.
 *		    '/foo').
 *		(4) If there are still unallocated sectors, and there are
 *		    already '/export/home*' file systems in the candidate list,
 *		    divide the space amongst them.
 *		(5) If there are still unallocated sectors, if there are at
 *		    least 10 MB of sectors available and there is at least one
 *		    unallocated slice available, and there are either no
 *		    candidates available to receive the space in the segment
 *		    or the number of unallocated sectors remaining are > 250 MB
 *		    and are > half the space originally occupied by all
 *		    candidate slices, then a new /export/home* file system is
 *		    created to hold all all of the free space.
 *		(6) If there are still unallocated sectors and there is at
 *		    least one candidate:
 *
 *		    While there are still sectors to allocate and we are
 *		    making progress allocating them:
 *			Execute steps '1', '2', and '3' respectively
 *
 *		General notes:
 *
 *		(1) Free space will never pump the overall swap slice space
 *		    over 2 x default swap size.
 *		(2) There will never be less than 15 MB allocated to any
 *		    slice on any given pass (except when general note #1
 *		    occurs).
 *		(3) There will never be more than 60 MB allocated to any
 *		    existing slice on any given pass.
 *
 * Scope:	public
 * Parameters:	dp	[RO, *RW] (Disk_t *)
 *			Disk object pointer to be updated with space
 *			allocations.
 * Return:	none
 */
void
SdiskobjAllocateUnused(Disk_t *dp)
{
	int *	np;
	int	front;
	int	end;
	int	sectors;
	int	slicefree;
	int	allocated;
	int	last;
	int	i;

	/* validate parameters */
	if (dp == NULL || !sdisk_is_usable(dp))
		return;

	if (get_trace_level() > 5) {
		write_status(SCR, LEVEL0,
			"Free space allocation for disk %s", disk_name(dp));
	}

	(void) adjust_slice_starts(dp);
	front = sdisk_geom_firstcyl(dp);
	end = front;

	/*
	 * for each segment of unused space on the disk, attempt
	 * to allocate space in order of precedence
	 */
	while ((end = SegmentFindEnd(dp, front)) >= 0) {
		/*
		 * define the attributes of the segment: free sectors
		 * and free slices
		 */
		sectors = SegmentFindFreeSectors(dp, front, end);
		np = SegmentFindSlices(dp, front, end);

		/*
		 * find out how many sectors are allocated initially
		 * to potential candidates
		 */
		allocated = SegmentCountAllocated(dp, np);

		if (get_trace_level() > 5) {
			write_status(SCR, LEVEL1|LISTITEM,
				"Segment %d/%d: %d free, %d allocated sectors",
				front, end, sectors, allocated);
		}

		/*
		 * pump up existing slices slightly before any spin-off
		 * attempts are made
		 */
		if (sectors > 0) {
			sectors = SegmentAbsorbPreferred(dp, np,
					sectors, 30, ROLLUP);
		}

		if (sectors > 0) {
			sectors = SegmentAbsorbNonpreferred(dp,
					np, sectors, 15, RESORIGIN_DEFAULT);
		}

		if (sectors > 0) {
			sectors = SegmentAbsorbNonpreferred(dp,
					np, sectors, 15, RESORIGIN_APPEXT);
		}

		/*
		 * if there are already /export/home* file systems in
		 * the candidate list, divide the space amongst them
		 */
		if (sectors > 0) {
			sectors = SegmentAbsorbExistingFree(dp, np, sectors);
		}

		/*
		 * see if there are unallocated slices available with which to
		 * create a file system
		 */
		slicefree = 0;
		WALK_SLICES(i) {
			if (Sliceobj_Size(CFG_CURRENT, dp, i) == 0)
				slicefree++;
		}
		
		/*
		 * see if it's even feasible to spin off a separate
		 * file system
		 */
		if (sectors > MINFSSIZE && slicefree > 0) {
			/*
			 * see if the threshhold for reasonableness
			 * exists to support a separate file system
			 */
			if ((np == NULL) || (sectors > mb_to_sectors(250) &&
					sectors > (allocated / 2))) {
				sectors = SegmentCreateFilesys(dp,
						end, sectors);
			}
		}

		/*
		 * if there is still unallocated space and there is at
		 * least one candidate to receive space, allocate the
		 * space a piece at a time giving preference to preferred,
		 * non-preferred default, and non-preferred non-default
		 * slices respectively
		 */
		if (np != NULL && sectors > 0) {
			last = sectors + 1;
			while (last > sectors) { 
				last = sectors;
				if (sectors > 0) {
					sectors = SegmentAbsorbPreferred(dp,
						np, sectors, 30,
						DONTROLLUP);
				}

				if (sectors > 0) {
					sectors = SegmentAbsorbNonpreferred(dp,
						np, sectors, 15,
						RESORIGIN_DEFAULT);
				}

				if (sectors > 0) {
					sectors = SegmentAbsorbNonpreferred(dp,
						np, sectors, 15,
						RESORIGIN_APPEXT);
				}
			}
		}

		front = end;

		if (get_trace_level() > 5) {
			write_status(SCR, LEVEL1|LISTITEM,
				"%d remaining sectors in segment",
				sectors);
		}
	}
}

/*
 * Function:	SegmentFindEnd
 * Description:	Find the next cylinder following 'cyl' which is the starting
 *		cylinder of a fixed slice (i.e. locked/preserved/stuck/ignored)
 *		which would potentially block preceding slices from sliding
 *		forward down the disk. Slices of size '0' and of type 'overlap'
 *		should be ignored, since they don't block anything.
 * Scope:	internal
 * Parameters:	dp	[RO, *RO] (Disk_t *)
 *			Disk object pointer with valid sdisk geometry pointer.
 *		cyl	[RO] (int)
 *			Cylinder which is the lower end of search.
 * Return:	# >= 0	starting cylinder number of nearest (lowest start)
 *			fixed slice which starts after 'cyl'
 *		-1	cyl == last cylinder on disk
 */
int
SegmentFindEnd(Disk_t *dp, int cyl)
{
	int	i;
	int	last;

	/* validate parameters */
	if (dp == NULL || sdisk_geom_null(dp))
		return (0);

	last = sdisk_geom_lcyl(dp);
	if (cyl == last)
		return (-1);

	WALK_SLICES(i) {
		if (slice_is_overlap(dp, i) ||
				Sliceobj_Size(CFG_CURRENT, dp, i) == 0)
			continue;

		if (Sliceobj_Start(CFG_CURRENT, dp, i) <= cyl ||
				Sliceobj_Start(CFG_CURRENT, dp, i) >= last)
			continue;

		if (SliceobjIsPreserved(CFG_CURRENT, dp, i) ||
				SliceobjIsLocked(CFG_CURRENT, dp, i) ||
				SliceobjIsIgnored(CFG_CURRENT, dp, i) ||
				SliceobjIsStuck(CFG_CURRENT, dp, i))
			last = Sliceobj_Start(CFG_CURRENT, dp, i);
	}

	return (last);
}

/*
 * Function:	SegmentFindFreeSectors
 * Description:	Determine the number of sectors which are unallocated within
 *		a specified region of the disk. Sectors contained within
 *		overlap slices are not considered to be allocated.
 * Scope:	internal
 * Parameters:	dp	[RO, *RO] (Disk_t *)
 *			Disk object pointer with valid sdisk geometry pointer.
 *		lowcyl	[RO] (int)
 *			Lower cylinder boundary of search area on disk.
 *		highcyl [RO] (int)
 *			Upper cylinder boundary of search area on disk.
 * Return:	# >= 0	Number of sectors which are unallocated within the
 *			specfied region of the disk
 */
int
SegmentFindFreeSectors(Disk_t *dp, int lowcyl, int highcyl)
{
	int	count;
	int	begin;
	int	end;
	int	send;
	int	i;

	/* validate parameters */
	if (dp == NULL || sdisk_geom_null(dp))
		return (0);

	count = 0;
	begin = lowcyl;

	while (begin < highcyl) {
		WALK_SLICES(i) {
			if (slice_is_overlap(dp, i) ||
					Sliceobj_Size(CFG_CURRENT, dp, i) == 0)
				continue;

			send = Sliceobj_Start(CFG_CURRENT, dp, i) +
					blocks_to_cyls(dp, Sliceobj_Size(
						CFG_CURRENT, dp, i));

			if (Sliceobj_Start(CFG_CURRENT, dp, i) == begin)
				begin = (send > highcyl ? highcyl : send);
		}

		end = highcyl;
		WALK_SLICES(i) {
			if (slice_is_overlap(dp, i) ||
					Sliceobj_Size(CFG_CURRENT, dp, i) == 0)
				continue;

			if (Sliceobj_Start(CFG_CURRENT, dp, i) < end &&
					Sliceobj_Start(CFG_CURRENT,
						dp, i) >= begin)
				end = Sliceobj_Start(CFG_CURRENT, dp, i);
		}

		count += cyls_to_blocks(dp, end - begin);
		begin = end;
	}

	return (count);
}

/* ---------------------- private functions ----------------------- */

/*
 * Function:	SegmentAbsorbPreferred
 * Description:	Distribute a given number of sectors amongst a list of slices,
 *		selecting only those which support a resource origin default
 *		resources in either the repository or dynamic resource class.
 *		If one of the candidates is "/", then it will receive repeated
 *		allocations for each of "/var" and "/opt" which don't have
 *		allocates slices (optional).
 * Scope:	private
 * Parameters:	dp	    [RO, *RO] (Disk_t *)
 *			    Disk object pointer with a valid sdisk config.
 *		np	    [RO, *RO] (int[])
 *			    Array of integers representing slice numbers for
 *			    recipient candidates.
 *		free	    [RO] (int)
 *			    Number of sectors to allocate.
 *		constraint  [RO] (int)
 *			    Value indicating what percentage of the current
 *			    slice size should be used as a maximum for
 *			    expansion. If VAL_UNSPECIFIED, then 100% is
 *			    assumed.
 *		roll	    [RO] (int)
 *			    Specifier indicating whether or not the "/" file
 *			    system should receive extra helpings of space for
 *			    having to handle "/var" and/or "/opt" when one or
 *			    both of these file systems do not have their own
 *			    slice.  Valid values are:
 *			    ROLLUP	"/" gets extra helpings for supporting
 *					children
 *			    DONTROLLUP  "/" doesn't get extra helpings for
 *					supporting children
 * Return:	# >= 0	    number of free sectors remaining
 */
static int
SegmentAbsorbPreferred(Disk_t *dp, int np[], int free, int constraint, int roll)
{
	ResobjHandle	res;
	ResClass_t	class;
	ResOrigin_t	origin;
	int	sectors;
	int	add;
	int	i;
	int	candidate;
	int	ep[NUMPARTS + 1];
	int	maxadd = MAXADD;

	/* validate parameters */
	if (dp == NULL || sdisk_geom_null(dp) || np == NULL)
		return (free);

	if (constraint < 0 || constraint > 100)
		return (free);

	sectors = free;

	/*
	 * all slices which have resources in the dynamic or respository 
	 * class, with a default origin, are considered "preferred";
	 * if "/" is one of the candidates, the for each of '/var' and
	 * '/opt' which do not have their own file systems configured
	 * on any disk, '/' receives an extra accounting
	 */
	candidate = 0;
	if (sectors > 0) {
		for (i = 0; valid_sdisk_slice(np[i]); i++) {
			/*
			 * NOTE that the analysis of the slice instance
			 * is strictly because at this time, initial
			 * install doesn't set instance values, so
			 * we should assume instance '0' in this case
			 */
			if (Sliceobj_Instance(CFG_CURRENT, dp, np[i])
					!= VAL_UNSPECIFIED) {
				res = ResobjFind(Sliceobj_Use(CFG_CURRENT,
						dp, np[i]),
					Sliceobj_Instance(CFG_CURRENT,
						dp, np[i]));
			} else {
				res = ResobjFind(Sliceobj_Use(CFG_CURRENT,
						dp, np[i]), 0);
			}

			if (res == NULL || ResobjGetAttribute(res,
					    RESOBJ_CONTENT_CLASS, &class,
					    RESOBJ_ORIGIN,	  &origin,
					    NULL) != D_OK)
				continue;

			if (origin != RESORIGIN_DEFAULT ||
					(class != RESCLASS_REPOSITORY &&
					 class != RESCLASS_DYNAMIC))
				continue;

			ep[candidate++] = np[i];

			if (streq(Sliceobj_Use(CFG_CURRENT, dp,
					np[i]), ROOT)) {
				if (roll == ROLLUP) {
					if (SliceobjFindUse(CFG_CURRENT, NULL,
							VAR, 0, 0) == NULL)
						ep[candidate++] = np[i];
					if (SliceobjFindUse(CFG_CURRENT, NULL,
							OPT, 0, 0) == NULL)
						ep[candidate++] = np[i];
				}
			}
		}
	}

	if (get_trace_level() > 5) {
		write_status(SCR, LEVEL2|LISTITEM,
			"%d preferred candidates provided %d sectors",
			 candidate, sectors);
	}

	/*
	 * if there is at least one candidate to receive space, do
	 * the allocation
	 */
	if (candidate > 0) {
		for (i = 0; i < candidate; i++) {
			if (NameIsSwap(Sliceobj_Use(CFG_CURRENT, dp, ep[i]))) {
				maxadd = ResobjGetSwap(RESSIZE_DEFAULT) * 2;
				if ((maxadd - SliceobjSumSwap(NULL,
						SWAPALLOC_ALL)) <= 0)
					continue;
			}

			if (constraint == 0)
				constraint = 100;

			add = (constraint * Sliceobj_Size(CFG_CURRENT,
					dp, ep[i])) / 100;

			/* never add less than the specified minimum */
			if (add < MINADD)
				add = MINADD;

			/* never add more than the specified maximum */
			if (add > maxadd)
				add = maxadd;

			/* never add more sectors than you have */
			if (add > sectors)
				add = sectors;

			/* round the value to cylinder boundaries */
			add = blocks_to_blocks(dp, add);

			if (get_trace_level() > 5) {
				write_status(SCR, LEVEL2|LISTITEM|CONTINUE,
				"adding %d MB (%2d%%) to %s (%s)",
				    sectors_to_mb(add),
				    constraint,
				    make_slice_name(disk_name(dp), ep[i]),
				    Sliceobj_Use(CFG_CURRENT, dp, ep[i]));
			}

			if (SliceobjSetAttribute(dp, ep[i],
					SLICEOBJ_SIZE,
					Sliceobj_Size(CFG_CURRENT,
						dp, ep[i]) + add,
					NULL) == D_OK) {
				sectors -= add;
			}
		}
	}

	/* return sectors that are still unallocated */
	return (sectors);
}

/*
 * Function:	SegmentAbsorbExistingFree
 * Description:	Distributed the specified number of sectors amongst any
 *		candidate slices which are /export/home, or /export/home<#>.
 * Scope:	private
 * Parameters:	dp	    [RO, *RO] (Disk_t *)
 *			    Disk object pointer with a valid sdisk config.
 *		np	    [RO, *RO] (int[])
 *			    Array of integers representing slice numbers for
 *			    recipient candidates.
 *		free	    [RO] (int)
 *			    Number of sectors to allocate.
 * Return:	# >= 0	    number of free sectors remaining
 */
static int
SegmentAbsorbExistingFree(Disk_t *dp, int np[], int free)
{
	int	sectors;
	int	candidate;
	int	i;
	int	ep[NUMPARTS + 1];
	char *	cp;
	int	add;

	/* validate parameters */
	if (dp == NULL || sdisk_geom_null(dp) || np == NULL)
		return (free);

	sectors = free;

	/*
	 * see if there are other existing free space directories
	 * which are good potential candidates
	 */
	candidate = 0;
	if (sectors > 0) {
		for (i = 0; valid_sdisk_slice(np[i]); i++) {
			if (strneq(Sliceobj_Use(CFG_CURRENT, dp, np[i]),
					EXPORTHOME,
					strlen(EXPORTHOME))) {
				cp = Sliceobj_Use(CFG_CURRENT, dp, np[i]) +
						(int) strlen(EXPORTHOME);
				if (*cp == '\0' || isdigit(*cp))
					ep[candidate++] = np[i];
			}
		}
	}

	/*
	 * if there is at least one candidate to receive space, do the
	 * allocation
	 */
	if (candidate > 0) {
		for (i = 0; i < candidate; i++) {
			add = blocks_to_blocks(dp, sectors / candidate);

			if (add > sectors)
				add = sectors;

			if (get_trace_level() > 5) {
				write_status(SCR, LEVEL1|LISTITEM,
				    "Adding %d MB to existing free %s (%s)",
				    sectors_to_mb(add),
				    make_slice_name(disk_name(dp), ep[i]),
				    Sliceobj_Use(CFG_CURRENT, dp, ep[i]));
			}

			if (SliceobjSetAttribute(dp, ep[i],
					SLICEOBJ_SIZE,
					Sliceobj_Size(CFG_CURRENT,
						dp, ep[i]) + add,
					NULL) == D_OK) {
				sectors -= add;
			}
		}
	}

	/* return sectors that are still unallocated */
	return (sectors);
}

/*
 * Function:	SegmentAbsorbNonpreferred
 * Description:	Distribute a given number of sectors amongst a list of slices,
 *		selecting only those which support the resource origin specified
 *		by the user, and are neither in the repository nor the dynamic
 *		resource class.
 * Description:	Distribute a given number of sectors among a list of slices,
 *		selecting only those which either (1) have resource objects with
 *		a default origin (origin == RESORIGIN_DEFAULT) and are not a
 *		preferred file system, or (2) either do not have an associated
 *		resource, or have an associated resource which does not have a
 *		default origin (origin != RESORIGIN_DEFAULT). The user must
 *		specify the overall size growth in which the slices are allowed
 *		to expand.
 * Scope:	private
 * Parameters:	dp	    [RO, *RO] (Disk_t *)
 *			    Disk object pointer with a valid sdisk config.
 *		np	    [RO, *RO] (int[])
 *			    Array of integers representing slice numbers for
 *			    recipient candidates.
 *		free	    [RO] (int)
 *			    Number of sectors to allocate.
 *		constraint  [RO] (int)
 *			    Value indicating what percentage of the current
 *			    slice size should be used as a maximum for
 *			    expansion. If VAL_UNSPECIFIED, then a 2 GB max is
 *			    assumed.
 *		origin	    [RO] (ResOrigin_t)
 *			    Specify the origin constraint for the allocation.
 *			    Valid values are:
 *				RESORIGIN_DEFAULT
 *				RESORIGIN_APPEXT | RESORIGIN_UNDEFINED
 * Return:	# >= 0	    number of free sectors remaining
 */
static int
SegmentAbsorbNonpreferred(Disk_t *dp, int np[], int free,
				int constraint, ResOrigin_t origin)
{
	ResobjHandle	res;
	ResClass_t	class;
	ResOrigin_t	resorigin;
	int	sectors;
	int	add;
	int	i;
	int	candidate;
	int	ep[NUMPARTS + 1];
	int	maxadd = MAXADD;

	/* validate parameters */
	if (dp == NULL || sdisk_geom_null(dp) || np == NULL)
		return (free);

	if (constraint < 0 || constraint > 100)
		return (free);

	sectors = free;

	/*
	 * create a list of slices from the list provided, which are not
	 * preferred and which meet the origin constraint specified
	 */
	candidate = 0;
	if (sectors > 0) {
		for (i = 0; valid_sdisk_slice(np[i]); i++) {
			/*
			 * NOTE that the analysis of the slice instance
			 * is strictly because at this time, initial
			 * install doesn't set instance values, so
			 * we should assume instance '0' in this case
			 */
			if (Sliceobj_Instance(CFG_CURRENT, dp, np[i])
					!= VAL_UNSPECIFIED) {
				res = ResobjFind(Sliceobj_Use(CFG_CURRENT,
						dp, np[i]),
					Sliceobj_Instance(CFG_CURRENT,
						dp, np[i]));
			} else {
				res = ResobjFind(Sliceobj_Use(CFG_CURRENT,
						dp, np[i]), 0);
			}

			if (res == NULL || ResobjGetAttribute(res,
					    RESOBJ_CONTENT_CLASS, &class,
					    RESOBJ_ORIGIN,	  &resorigin,
					    NULL) != D_OK)
				continue;

			if ((origin != RESORIGIN_UNDEFINED &&
					resorigin != origin) ||
					class == RESCLASS_REPOSITORY ||
					class == RESCLASS_DYNAMIC)
				continue;

			ep[candidate++] = np[i];
		}
	}

	if (get_trace_level() > 5) {
		write_status(SCR, LEVEL2|LISTITEM,
		    "%d non-preferred %s candidates provided %d sectors",
		    candidate,
		    origin == RESORIGIN_DEFAULT ? "default" : 
			    origin == RESORIGIN_APPEXT ? "appext" : "unknown",
		    sectors);
	}

	/*
	 * if there is at least one candidate to receive space, do the
	 * allocation
	 */
	if (candidate > 0) {
		for (i = 0; i < candidate; i++) {
			if (NameIsSwap(Sliceobj_Use(CFG_CURRENT, dp, ep[i]))) {
				maxadd = ResobjGetSwap(RESSIZE_DEFAULT) * 2;
				if ((maxadd - SliceobjSumSwap(NULL,
						SWAPALLOC_ALL)) <= 0)
					continue;
			}

			if (constraint == 0)
				constraint = 100;

			add = (constraint * Sliceobj_Size(CFG_CURRENT,
					dp, ep[i])) / 100;

			/* never add less than the specified minimum */
			if (add < MINADD)
				add = MINADD;

			/* never add more than the specified maximum */
			if (add > maxadd)
				add = maxadd;

			/* never add more sectors than you have */
			if (add > sectors)
				add = sectors;

			/* round the value to cylinder boundaries */
			add = blocks_to_blocks(dp, add);

			if (get_trace_level() > 5) {
				write_status(SCR, LEVEL2|LISTITEM|CONTINUE,
				    "adding %d MB (%2d%%) to %s (%s)",
				    sectors_to_mb(add),
				    constraint,
				    make_slice_name(disk_name(dp), ep[i]),
				    Sliceobj_Use(CFG_CURRENT, dp, ep[i]));
			}

			if (SliceobjSetAttribute(dp, ep[i],
					SLICEOBJ_SIZE,
					Sliceobj_Size(CFG_CURRENT,
						dp, ep[i]) + add,
					NULL) == D_OK) {
				sectors -= add;
			}
		}
	}

	/* return sectors that can't be allocated */
	return (sectors);
}

/*
 * Function:	FilesysFindUnusedName
 * Description: Generate a file system name of the form:
 *
 *			/export/home    or
 *			/export/home<#>
 *
 *		which is unique across all slices on all selected drives within
 *		the current slice configuration. The number appended onto the
 *		file system name is iteratively incremented by '1' (starting at
 *		0) until a unique name is found.
 * Scope:	private
 * Parameters:	name	- pointer to character array defined in calling process;
 *			  used to retrieve the file system name. Should be a
 *			  minimum of 16 characters long.
 * Return:	none
 */
static void
FilesysFindUnusedName(char *name)
{
	int	i;

	(void) strcpy(name, "/export/home");
	for (i = 0; /* ever */; i++) {
		if (SliceobjFindUse(CFG_CURRENT, NULL, name, 0, 0) == NULL)
			break;
		(void) sprintf(name, "/export/home%d", i);
	}
}

/*
 * Function:	FilesysFindUnusedSlice
 * Description: Search all the slices on a disk looking for an zero sized,
 *		unlocked, unstuck, unpreserved, unignored, slice with no
 *		defined mount point. The slice should preferably come after
 *		other slices which start at a parameter specified cylinder.
 * Scope:	private
 * Parameters:	dp	- non-NULL disk structure pointer
 *		cyl	- user specific cylinder after which
 *			  (0 <= # < total usable cylinders on the drive)
 * Return:	 #	- number of unused slice
 *		-1	- no unused slices found (or 'dp' is NULL or the
 *			  Sdisk geometry pointer is null)
 */
static int
FilesysFindUnusedSlice(Disk_t *dp, int cyl)
{
	int	mark = 16;
	int	slice = -1;
	int	i;

	if (dp == NULL || sdisk_geom_null(dp))
		return (-1);

	WALK_SLICES(i) {
		if (i >= mark)
			break;

		if (Sliceobj_Size(CFG_CURRENT, dp, i) > 0 ||
				SliceobjIsPreserved(CFG_CURRENT, dp, i) ||
				SliceobjIsIgnored(CFG_CURRENT, dp, i) ||
				SliceobjIsStuck(CFG_CURRENT, dp, i) ||
				SliceobjIsLocked(CFG_CURRENT, dp, i)) {
			if (Sliceobj_Start(CFG_CURRENT, dp, i) == cyl)
				mark = i;
		} else {
			if (Sliceobj_Size(CFG_CURRENT, dp, i) == 0 &&
				    !slice_mntpnt_exists(dp, i) &&
				    slice < mark)
				slice = i;
		}
	}

	if (slice > 0)
		return (slice);

	/*
	 * if there was no luck in the preferred location on the disk,
	 * look anywhere (first come, first served)
	 */
	WALK_SLICES(i) {
		if (Sliceobj_Size(CFG_CURRENT, dp, i) == 0 &&
				!slice_mntpnt_exists(dp, i) &&
				!SliceobjIsPreserved(CFG_CURRENT, dp, i) &&
				!SliceobjIsIgnored(CFG_CURRENT, dp, i) &&
				!SliceobjIsStuck(CFG_CURRENT, dp, i) &&
				!SliceobjIsLocked(CFG_CURRENT, dp, i)) {
			return (i);
		}
	}
	/* no luck finding anything */
	return (-1);
}

/*
 * Function:	SegmentCreateFilesys
 * Description:	Find an unused slice (preferably located on a slice following
 *		the user specified cylinder). Find an unused mount point name
 *		(looking across all selected drives) and assign it the mount
 *		point name and the user supplied size. The starting cylinder
 *		is set to '1' less than the user specified cylinder in order
 *		to insure that future calls to adjust_slice_starts() moves the
 *		slice into the correct location on the drive. This routine is
 *		used as part of the procedure of placing unused disk space into
 *		named file systems.  The cylinder locating is necessary to
 *		ensure that disks with fixed slices (e.g. preserved slices)
 *		have file systems made in the correct location on the drive.
 *		The 'cyl' parameter is actually the number of the starting
 *		cylinder of the slice which terminates (blocks) the free
 *		space area.
 *
 *		NOTE:	this routine does not leave the S-disk in a valid state.
 *			An adjust_slice_start() call must be made in order to
 *			moved the slice into an acceptable position.
 *
 * Scope:	private
 * Parameters:	dp	non-NULL disk structure pointer with a valid S-disk
 *			geometry pointer
 *		cyl	cylinder before which the slice should be placed
 *		sectors	number of sectors for file system
 * Return:	0	space allocated successfully
 *		size 	failure to make a separate file system
 */
static int
SegmentCreateFilesys(Disk_t *dp, int cyl, int sectors)
{
	int	slice;
	char	name[MAXNAMELEN];
	Slice_t	saved;

	if (dp == NULL || sdisk_geom_null(dp))
		return (sectors);

	if ((slice = FilesysFindUnusedSlice(dp, cyl)) >= 0) {
		(void) memcpy(&saved, Sliceobj_Addr(CFG_CURRENT, dp, slice),
				sizeof (Slice_t));
		FilesysFindUnusedName(name);
		if (get_trace_level() > 5) {
			write_status(SCR, LEVEL1|LISTITEM,
				"Creating %d MB %s file system on slice %d",
				sectors_to_mb(sectors), name, slice);
		}

		if (SliceobjSetAttribute(dp, slice,
				SLICEOBJ_START,		cyl - 1,
				SLICEOBJ_SIZE,		sectors,
				SLICEOBJ_USE,		name,
				SLICEOBJ_INSTANCE,	0,
				SLICEOBJ_MOUNTOPTS,	"-",
				NULL) != D_OK) {
			(void) memcpy(Sliceobj_Addr(CFG_CURRENT, dp, slice),
					&saved, sizeof (Slice_t));
			return (sectors);
		}
	} else
		return (sectors);

	return (0);
}

/*
 * Function:	SegmentFindSlices
 * Description: Create an array of slice indexes which represent those slices
 *		which are not fixed (i.e. stuck, preserved, locked, explicit,
 *		or ignored), which are not overlaps, and which have a starting
 *		cylinder greater than 'lowcyl' and less than 'highcyl'. This
 *		array comprises a list of slices which are viable recipients of
 *		a block of unallocated sectors located on the disk between
 *		'lowcyl' and 'highcyl'.
 * Scope:	private
 * Parameters:	dp	[RO, *RO] (Disk_t *)
 *			Disk object pointer with a valid sdisk geometry
 *		lowcyl	[RO] (int)
 *			Cylinder specifying the lower boundary of the area
 *		highcyl	[RO] (int)
 *			Cylinder specifying the upper boundary of the area
 * Return:	int *	pointer to array containing slice indexes
 *		NULL	no free slices
 */
static int *
SegmentFindSlices(Disk_t *dp, int lowcyl, int highcyl)
{
	static int	np[NUMPARTS + 1];
	int		count;
	int		i;

	/* validate parameters */
	if (dp == NULL || sdisk_geom_null(dp))
		return (NULL);

	count = 0;
	WALK_SLICES(i) {
		if (Sliceobj_Size(CFG_CURRENT, dp, i) == 0 ||
				slice_is_overlap(dp, i) ||
				Sliceobj_Start(CFG_CURRENT, dp, i) < lowcyl ||
				Sliceobj_Start(CFG_CURRENT, dp, i) > highcyl ||
				SliceobjIsStuck(CFG_CURRENT, dp, i) ||
				SliceobjIsExplicit(CFG_CURRENT, dp, i) ||
				SliceobjIsPreserved(CFG_CURRENT, dp, i) ||
				SliceobjIsIgnored(CFG_CURRENT, dp, i) ||
				SliceobjIsLocked(CFG_CURRENT, dp, i)) {
			continue;
		}

		np[count++] = i;
	}

	np[count] = -1;
	if (count == 0)
		return (NULL);
	else
		return (np);
}

/*
 * Function:	SegmentCountAllocated
 * Description:	Total up the number of sectors allocated to slices in the
 *		provided array, excluding overlap and swap slices.
 * Scope:	private
 * Parameters:	dp	[RO, *RO] (Disk_t *)
 *			Disk object pointer with an existing sdisk config.
 *		np	[RO, *RO] (int [])
 *			Array of slice indexes, terminated with a -1 slice
 *			index.
 * Return:	# >= 0	Number of sectors allocated to the slices listed.
 */
static int
SegmentCountAllocated(Disk_t *dp, int np[])
{
	int	i;
	int	used;

	/* validate parameters */
	if (dp == NULL || np == NULL)
		return (0);

	used = 0;
	for (i = 0; valid_sdisk_slice(np[i]); i++) {
		if (!streq(Sliceobj_Use(CFG_CURRENT, dp, np[i]), OVERLAP) &&
				!streq(Sliceobj_Use(CFG_CURRENT,
					dp, np[i]), SWAP) &&
				Sliceobj_Size(CFG_CURRENT, dp, np[i]) > 0)
			used += Sliceobj_Size(CFG_CURRENT, dp, np[i]);
	}

	return (used);
}
