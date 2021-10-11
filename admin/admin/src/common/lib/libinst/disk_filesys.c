#ifndef lint
#pragma ident "@(#)disk_filesys.c 1.50 95/01/13"
#endif
/*
 * Copyright (c) 1991 Sun Microsystems, Inc.  All Rights Reserved. Sun
 * considers its source code as an unpublished, proprietary trade secret, and
 * it is available only under strict license provisions.  This copyright
 * notice is placed here only to protect Sun in the event the source is
 * deemed a published work.  Dissassembly, decompilation, or other means of
 * reducing the object code to human readable form is prohibited by the
 * license agreement under which this code is provided to the user or company
 * in possession of this copy.
 *
 * RESTRICTED RIGHTS LEGEND: Use, duplication, or disclosure by the Government
 * is subject to restrictions as set forth in subparagraph (c)(1)(ii) of the
 * Rights in Technical Data and Computer Software clause at DFARS 52.227-7013
 * and in similar clauses in the FAR and NASA FAR Supplement.
 */
/*
 * This module contains routines which deal with file system size
 * determinations, requirements, and recommendations
 */
#include "disk_lib.h"

/* Local Statics */

static Space	*error[NUMDEFMNT + 1];
static Space	 tmperr[NUMDEFMNT + 1];

/* Public Function Prototypes */

Space		**filesys_ok(void);

/* Library Function Prototypes */

/* Local Function Prototypes */

/* ******************************************************************** */
/*			PUBLIC SUPPORT FUNCTIONS			*/
/* ******************************************************************** */

/*
 * filesys_ok()
 *	Determine if all file systems as configured in the disk
 *	list have sufficient space (>= min) to hold the currently
 *	configured software. Only mountpoints with valid filesystem
 *	pathnames are reviewed. Default file system expansion values
 *	are not taken into consideration during this check. There
 *	are two separate checks that are made:
 *
 *	(1)	every file system type which has been SELECTED
 *		must have at least one slice with that name (may
 *		be more than one for SWAP)
 *	(2)	every SELECTED mount point must fit all the
 *		software intended for it
 * Parameters:
 *	none
 * Return:
 *	NULL	- all file systems have more than the minimum amount
 *		  of space
 *	Space**	- array of space information for all file system which
 *		  do not have a minimum amount of space configured; the
 *		  last entry is a NULL pointer.
 * Status:
 *	public
 */
Space **
filesys_ok(void)
{
	static Defmnt_t	**mpp = (Defmnt_t **)NULL;
	struct mnt_pnt	p;
	int		i, j;
	int		size;
	int		allocsize;

	/* get the updated mount point mask */
	mpp = get_dfltmnt_list(mpp);

	for (j = 0, i = 0; mpp[i]; i++) {
		/* process only DFLT_SELECT slice names */
		if (mpp[i]->status != DFLT_SELECT)
			continue;

		size = get_minimum_fs_size(mpp[i]->name, NULL, ROLLUP);

		/* set the allocated size variable for the current slice name */
		if (strcmp(mpp[i]->name, SWAP) == 0) {
			/*
			 * swap is assessed as an entire resource, and not as
			 * an individual slice
			 */
			allocsize = swap_size_allocated(NULL, NULL);
		} else if (find_mnt_pnt(NULL, NULL, mpp[i]->name,
				&p, CFG_CURRENT)) {
			allocsize = slice_size(p.dp, p.slice);
		} else
			allocsize = 0;

		/*
		 * Check the required size against the allocated size;
		 * At this point in the routine, we know that the slice
		 * must have been required (SELECTED); if it was
		 * required, but doesn't exist, this is considered
		 * to be an error. In this case the size would be "too
		 * small", but would have a bused (required size) of "0".
		 */
		if ((size > allocsize) || (allocsize == 0)) {
			/* too small */
			tmperr[j].mountp = mpp[i]->name;
			tmperr[j].bused = size;
			tmperr[j].bavail = allocsize;
			error[j] = &tmperr[j];
			j++;
		}
	}

	error[j] = (Space *)NULL;

	if (j != 0)
		return ((Space **)error);
	else
		return ((Space **)NULL);
}
