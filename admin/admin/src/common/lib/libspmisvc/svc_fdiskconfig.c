#ifndef lint
#pragma ident "@(#)svc_fdiskconfig.c 1.11 96/06/19 SMI"
#endif

/*
 * Copyright (c) 1995-1996 by Sun Microsystems, Inc. All rights reserved.
 */
/*
 * Module:	svc_fdiskconfig.c
 * Group:	libspmisvc
 * Description: This module contains fdisk object layout manipulation
 *		routines.
 */
#include <stdio.h>
#include <sys/param.h>
#include <sys/types.h>
#include "spmisvc_lib.h"
#include "spmistore_lib.h"

/* public prototypes */

int		FdiskobjConfig(Layout_t, Disk_t *, char *);

/* private prototypes */

static int	FdiskobjAutolayoutDisk(Disk_t *);
static int	PartobjFindMaxfree(Disk_t *, int *, int *);

/* ---------------------- public functions ----------------------- */

/*
 * Function:	FdiskobjConfig
 * Description:	Routine to configure the entire current fdisk structure
 *		The action taken is determed by the parameter 'label' (see
 *		below).
 *
 *		NOTE:	The S-disk geometry pointer is cleared by called
 *			routines whenever appropriate. Need to call
 *			check_fdisk() to reinstate that pointer before
 *			accessing the S-disk in subsequent calls.
 * Scope:	public
 * Parameter:	layout	[RO] (Layout_t)
 *			Specify the how the fdisk configuration should be
 *			layed out. Valid values are:
 *			    LAYOUT_RESET	configure empty
 *			    LAYOUT_DEFAULT	implement autolayout
 *			    LAYOUT_COMMIT	restore the committed config
 *			    LAYOUT_EXIST	restore the existing config
 *		disk	[RO, *RO] (Disk_t *)
 *			Disk structure pointer; NULL if specifying drive
 *			by 'drive'. 'disk' has precedence in the drive order
 *			(state:  okay, selected)
 *		drive	[RO, *RO] (char *)
 *			Name of drive (e.g. c0t0d0) - NULL if specifying drive
 *			by 'disk'.
 * Return:	D_OK	    - disk state set successfully
 *		D_NODISK    - neither argument was specified
 *		D_BADDISK   - disk state not valid for requested operation
 *		D_BADARG    - illegal 'label' value
 *		D_NOTSELECT - disk state was not selected
 */
int
FdiskobjConfig(Layout_t layout, Disk_t *disk, char *drive)
{
	Disk_t	*dp;

	if (disk != NULL)
		dp = disk;
	else if ((dp = find_disk(drive)) == NULL)
		return (D_NODISK);

	if (!disk_fdisk_req(dp))
		return (D_OK);

	if (!disk_selected(dp))
		return (D_NOTSELECT);

	switch (layout) {
	    case LAYOUT_RESET:
		FdiskobjReset(dp);
		return (D_OK);
	    case LAYOUT_DEFAULT:
		return (FdiskobjAutolayoutDisk(dp));
	    case LAYOUT_COMMIT:
		return (FdiskobjRestore(CFG_COMMIT, dp));
	    case LAYOUT_EXIST:
		return (FdiskobjRestore(CFG_EXIST, dp));
	}

	return (D_BADARG);
}

/* ---------------------- private functions ----------------------- */

/*
 * Function:	FdiskobjAutolayoutDisk
 * Description:	Configure the fdisk partitions with a default configuration.
 *		If there is an existing Solaris partition, we're done (unless
 *		it is configured to conflict with an explicit boot device).
 *		If there is no Solaris partition, try to make one that's
 *		the biggest possible given the availability of contiguous
 *		disk space.
 * Scope:	private
 * Parameters:	dp	[RO, *RO] (Disk_t *)
 *			Non-NULL disk object pointer.
 * Return:	D_OK	    - disk setup successfully, or no setup required
 *		D_NODISK    - 'dp' is NULL
 *		D_BADDISK   - sdisk geometry pointer NULL or state not "okay"
 *		D_BOOTFIXED - boot device explicitly specified and is
 *			      not available as a Solaris partition
 */
static int
FdiskobjAutolayoutDisk(Disk_t *dp)
{
	int		device;
	int		size;
	int		part;
	char		diskname[32];
	Disk_t *	bdp;
	Errmsg_t *	elp;

	/* validate parameters */
	if (dp == NULL)
		return (D_NODISK);

	(void) BootobjGetAttribute(CFG_CURRENT,
		    BOOTOBJ_DISK,   diskname,
		    BOOTOBJ_DEVICE, &device,
		    NULL);

	/*
	 * If there is a Solaris partition on this disk:
	 * (1) for PPC, we're done (no boot object interaction)
	 * (2) for Intel, if this is the boot disk and this is not the
	 *	boot partition, we fail; otherwise, make sure the boot
	 *	device reflects the correct partition and we're done
	 */
	if ((part = get_solaris_part(dp, CFG_CURRENT)) > 0) {
		if (IsIsa("i386")) {
			if (DiskobjFindBoot(CFG_CURRENT, &bdp) == D_OK &&
					dp == bdp) {
				if (!BootobjIsExplicit(CFG_CURRENT,
						BOOTOBJ_DEVICE_EXPLICIT)) {
					(void) BootobjSetAttribute(CFG_CURRENT,
						BOOTOBJ_DEVICE, part,
						NULL);
				} else if (device != part){
					return (D_BOOTFIXED);
				}
			}
		}

		return (D_OK);
	}

	/*
	 * we know we don't have a Solaris partition at this point; find the
	 * "best" partition to use for one and create the space
	 */
	if (PartobjFindMaxfree(dp, &part, &size) != D_OK)
		return (D_FAILED);

	if (set_part_attr(dp, part, SUNIXOS, GEOM_IGNORE) != D_OK ||
			set_part_geom(dp, part, GEOM_IGNORE, size) != D_OK ||
			adjust_part_starts(dp) != D_OK)
		return (D_FAILED);
	
	/*
	 * On Intel, if this is the current boot disk and the boot device
	 * isn't explicit, update the boot device
	 */
	if (IsIsa("i386") &&
		    DiskobjFindBoot(CFG_CURRENT, &bdp) == D_OK &&
		    dp == bdp &&
		    !BootobjIsExplicit(CFG_CURRENT, BOOTOBJ_DEVICE_EXPLICIT)) {
		(void) BootobjSetAttribute(CFG_CURRENT,
			BOOTOBJ_DEVICE, part,
			NULL);
	}

	/* make sure there are no errors; update the sdisk geometry pointer */
	if (check_fdisk(dp) > 0 && (elp = worst_error()) != NULL)
		return (elp->code);

	return (D_OK);
}

/*
 * Function:	PartobjFindMaxfree
 * Description: Find a partition which meets boot object constraints where
 *		applicable, and returns the partition index and size for
 *		the largest continugous available disk space.
 * Scope:	private
 * Parameters:	dp	[RO, *RO] (Disk_t *)
 *			Non-NULL pointer to disk object.
 *		partp	[RO, *WO] (&int)
 *			Variable used to retrieve partition index (0 if unset).
 *		sizep	[RO, *WO] (&int)
 *			Variable used to retrieve partition size in sectors
 *			(0 if unset).
 * Return:	D_OK
 *		D_FAILED
 */
static int
PartobjFindMaxfree(Disk_t *dp, int *partp, int *sizep)
{
	int	    p;
	int	    m;
	int	    size = 0;
	int	    part;
	int	    unused = 0;
	Disk_t *    bdp;

	(void) DiskobjFindBoot(CFG_CURRENT, &bdp);
	/*
	 * On Intel systems, if this is the boot disk and the boot device
	 * is explicitly specified, that's the only partition we can use;
	 * if it isn't the boot disk or isn't explicitly specified, all
	 * partitions are fair game.
	 */
	if (IsIsa("i386")) {
		if (dp == bdp && BootobjIsExplicit(CFG_CURRENT,
				BOOTOBJ_DEVICE_EXPLICIT)) {
			(void) BootobjGetAttribute(CFG_CURRENT,
				    BOOTOBJ_DEVICE, &part,
				    NULL);
		}
	} else {
		part = 0;
	}

	/*
	 * if the partition is explicitly known, then use the maximum
	 * size available to that partition; otherwise, go find the
	 * partition with the most space available
	 */
	if (valid_fdisk_part(part)) {
		size = max_size_part_hole(dp, part);
	} else {
		WALK_PARTITIONS(p) {
			if (part_id(dp, p) == UNUSED) {
				unused++;
				m = max_size_part_hole(dp, p);
				if (m > size) {
					size = m;
					part = p;
				}
			}
                }
        }

	if (!valid_fdisk_part(part))
		return (D_FAILED);

	*partp = part;
	*sizep = size;
	return (D_OK);
}
