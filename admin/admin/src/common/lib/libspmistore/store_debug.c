#ifndef lint
#pragma ident "@(#)store_debug.c 1.10 96/07/10 SMI"
#endif

/*
 * Copyright (c) 1995-1996 by Sun Microsystems, Inc. All rights reserved.
 */
/*
 * Module:	store_debug.c
 * Group:	libspmistore
 * Description: 
 */
#include <stdio.h>
#include <sys/param.h>
#include <sys/types.h>
#include "spmistore_lib.h"
#include "spmicommon_api.h"
#include "store_strings.h"

/* private prototypes */

static void	BootobjPrintState(char *, Label_t);
static void	DiskobjPrintState(Disk_t *);
static void	DiskobjPrintSpecific(Label_t, Disk_t *);
static void	SdiskobjPrintGeom(Label_t, Disk_t *);
static void	SdiskobjPrintState(Label_t, Disk_t *);
static void	SdiskobjPrintSlices(Label_t, Disk_t *);
static void	FdiskobjPrintPartGeom(Label_t, Disk_t *, int);
static void	FdiskobjPrintState(Label_t, Disk_t *);
static void	FdiskobjPrintParts(Label_t, Disk_t *);

/* ---------------------- public functions ----------------------- */

/*
 * Function:	print_disk
 * Description: Print the disk configuration information in readable format.
 * Scope:	public
 * Parameters:	disk	- disk structure pointer (NULL if specifying drive
 *		  	  by 'drive' - 'disk' has precedence)
 *		drive	- name of drive - e.g. c0t0d0 (NULL if specifying
 *		  	  drive by 'disk')
 * Return:	none
 */
void
print_disk(Disk_t *disk, char *drive)
{
	Disk_t	*dp;

	if (disk != NULL)
		dp = disk;
	else if ((dp = find_disk(drive)) == NULL)
		return;

	DiskobjPrint(CFG_CURRENT, dp);
}

/*
 * Function:	DiskobjPrint
 * Description: Print the disk configuration information in readable format.
 * Scope:	public
 * Parameters:	state	[RO] (Label_t)
 *			State of the partition to print. Valid values are:
 *			    CFG_CURRENT
 *			    CFG_COMMIT
 *			    CFG_EXIST
 *		dp	[RO, *RO] (Disk_t *)
 *			Disk object handle.
 * Return:	none
 */
void
DiskobjPrint(Label_t state, Disk_t *dp)
{
	/* validate parameters */
	if (dp == NULL)
		return;

	write_status(SCR, LEVEL0,
		"PHYSICAL DISK (%s):", disk_name(dp));

	DiskobjPrintState(dp);
	DiskobjPrintSpecific(state, dp);
	write_status(SCR, LEVEL0|CONTINUE, "");
}

/*
 * Function:    BootobjPrint
 * Description:	Print the current, committed, and existing state of the boot
 *		object.
 * Scope:       internal
 * Parameters:  none
 * Return:      none
 */
void
BootobjPrint(void)
{
	write_status(SCR, LEVEL0|CONTINUE, "Boot Object Status:");
	BootobjPrintState("Current", CFG_CURRENT);
	BootobjPrintState("Committed", CFG_COMMIT);
	BootobjPrintState("Existing", CFG_EXIST);

}

/* ---------------------- private functions ----------------------- */

/*
 * Function:	DiskobjPrintSpecific
 * Description:	Print a specific state of the disk object.
 * Scope:	private
 * Parameters:	state	[RO] (Label_t)
 *			State of the partition to print. Valid values are:
 *			    CFG_CURRENT
 *			    CFG_COMMIT
 *			    CFG_EXIST
 *		dp	[RO, *RO] (Disk_t *)
 *			Disk object handle.
 * Return:	none
 */
static void
DiskobjPrintSpecific(Label_t state, Disk_t *dp)
{
	char *	config;

	/* validate parameters */
	if (dp == NULL)
		return;

	if (state != CFG_CURRENT && state != CFG_COMMIT && state != CFG_EXIST)
		return;

	switch (state) {
	    case CFG_CURRENT:	config = "CURRENT";
				break;
	    case CFG_COMMIT:	config = "COMMITTED";
				break;
	    case CFG_EXIST:	config = "EXISTING";
				break;
	    default:		config = "";
				break;
	}

	write_status(SCR, LEVEL0|LISTITEM|CONTINUE,
		"%s CONFIGURATION:", config);

	if (disk_fdisk_exists(dp)) {
		write_status(SCR, LEVEL1, "FDISK:");
		FdiskobjPrintState(state, dp);
		write_status(SCR, LEVEL1, "FDISK PARTITIONS:");
		FdiskobjPrintParts(state, dp);
	}

	write_status(SCR, LEVEL1, "SDISK:");
	SdiskobjPrintState(state, dp);
	write_status(SCR, LEVEL1, "SDISK GEOMETRY:");
	SdiskobjPrintGeom(state, dp);
	write_status(SCR, LEVEL1, "S-DISK SLICES:");
	SdiskobjPrintSlices(state, dp);
}

/*
 * Function:	BootobjPrintState
 * Description:	Print the specified state of the boot object with the title
 *		provided.
 * Scope:	private
 * Parameters:	title	[RO, *RO] (char *)
 *			Title label to use when printing output.
 *		state	[RO] (Label_t)
 *			State of boot object to be printed. Valid values are:
 *			    CFG_CURRENT,
 *			    CFG_COMMIT,
 *			    CFG_EXIST
 * Return:	none
 */
static void
BootobjPrintState(char *title, Label_t state)
{
	char	disk[32] = "";
	int	device = -1;
	char	dtype = '\0';
	int	diskexpl = 0;
	int	devexpl = 0;
	int	pupdate = 0;
	int	pwriteable = 0;

	(void) BootobjGetAttribute(state,
			BOOTOBJ_DISK, disk,
			BOOTOBJ_DEVICE, &device,
			BOOTOBJ_DEVICE_TYPE, &dtype,
			BOOTOBJ_DISK_EXPLICIT, &diskexpl,
			BOOTOBJ_DEVICE_EXPLICIT, &devexpl,
			BOOTOBJ_PROM_UPDATE, &pupdate,
			BOOTOBJ_PROM_UPDATEABLE, &pwriteable,
			NULL);

	write_status(SCR, LEVEL1, "%s:\n", title);
	write_status(SCR, LEVEL2, "           Disk: %s", *disk == '\0' ? "NULL" : disk);
	write_status(SCR, LEVEL2, "         Device: %d", device);
	write_status(SCR, LEVEL2, "    Device Type: %c", dtype);
	write_status(SCR, LEVEL2, "  Disk Explicit: %d", diskexpl);
	write_status(SCR, LEVEL2, "Device Explicit: %d", devexpl);
	write_status(SCR, LEVEL2, "Prom Updateable: %d", pupdate);
	write_status(SCR, LEVEL2, "    Prom Update: %d", pwriteable);
}

/*
 * Function:	SdiskobjPrintSlices
 * Description:	Print out the slice configuration associated with the given state
 *		of the disk.
 * Scope:	private
 * Parameters:	state	[RO] (Label_t)
 *			State of the partition to print. Valid values are:
 *			    CFG_CURRENT
 *			    CFG_COMMIT
 *			    CFG_EXIST
 *		dp	[RO, *RO] (Disk_t *)
 *			Disk object handle.
 * Return:	none
 */
static void
SdiskobjPrintSlices(Label_t state, Disk_t *dp)
{
	int	slice;

	/* validate parameters */
	if (dp == NULL)
		return;

	if (state != CFG_CURRENT && state != CFG_COMMIT && state != CFG_EXIST)
		return;

	write_status(SCR, LEVEL1|LISTITEM|CONTINUE, 
		"Slice       Geometry            Name           Options     Flags");
	write_status(SCR, LEVEL1|LISTITEM|CONTINUE, 
		"-----   --------------  ---------------------  ---------  -----------");
		
	WALK_SLICES(slice) {
		write_status(SCR, LEVEL1|LISTITEM|CONTINUE, 
		"  %-2d    %4d [%7d]  [%2d] %-18s %-8s  [%s%s%s%s%s%s ]",
			slice,
			Sliceobj_Start(state, dp, slice),
			Sliceobj_Size(state, dp, slice),
			Sliceobj_Instance(state, dp, slice),
			Sliceobj_Use(state, dp, slice),
			Sliceobj_Mountopts(state, dp, slice),
			SliceobjIsPreserved(state, dp, slice)	? " preserved"	  : "",
			SliceobjIsLocked(state, dp, slice)	? " locked"	  : "",
			SliceobjIsStuck(state, dp, slice)	? " stuck"	  : "",
			SliceobjIsRealigned(state, dp, slice)	? " realigned"	  : "",
			SliceobjIsExplicit(state, dp, slice)	? " explicit"	  : "",
			SliceobjIsIgnored(state, dp, slice)	? " ignored"	  : "");
	}
}

/*
 * Function:	SdiskobjPrintGeom
 * Description: Print out the geometry structure associated with the given state
 *		of the sdisk.
 * Scope:	private
 * Parameters:	state	[RO] (Label_t)
 *			State of the partition to print. Valid values are:
 *			    CFG_CURRENT
 *			    CFG_COMMIT
 *			    CFG_EXIST
 *		dp	[RO, *RO] (Disk_t *)
 *			Disk object handle.
 * Return:	none
 */
static void
SdiskobjPrintGeom(Label_t state, Disk_t *dp)
{
	/* validate parameters */
	if (dp == NULL)
		return;

	if (state != CFG_CURRENT && state != CFG_COMMIT && state != CFG_EXIST)
		return;

	if (Sdiskobj_Geom(state, dp) == NULL) {
		write_status(SCR, LEVEL1|LISTITEM|CONTINUE,
			"no disk geometry currently configured");
	} else {
		write_status(SCR, LEVEL1|LISTITEM|CONTINUE,
			"  tcyl: %-7d    lcyl: %-7d   dcyl: %-7d   firstcyl: %-7d",
			Sdiskobj_Geom(state, dp)->tcyl,
			Sdiskobj_Geom(state, dp)->lcyl,
			Sdiskobj_Geom(state, dp)->dcyl,
			Sdiskobj_Geom(state, dp)->firstcyl);
		write_status(SCR, LEVEL1|LISTITEM|CONTINUE,
		    " tsect: %-7d   lsect: %-7d  dsect: %-7d      rsect: %-7d",
		    Sdiskobj_Geom(state, dp)->tsect,
		    Sdiskobj_Geom(state, dp)->lsect,
		    Sdiskobj_Geom(state, dp)->dsect,
		    Sdiskobj_Geom(state, dp)->rsect);
		write_status(SCR, LEVEL1|LISTITEM|CONTINUE,
		    "onecyl: %-7d  hbacyl: %-7d  nhead: %-7d      nsect: %-7d",
		    Sdiskobj_Geom(state, dp)->onecyl,
		    Sdiskobj_Geom(state, dp)->hbacyl,
		    Sdiskobj_Geom(state, dp)->nhead,
		    Sdiskobj_Geom(state, dp)->nsect);
		write_status(SCR, LEVEL1|LISTITEM|CONTINUE,
		    "  free: %-7d",
		    sdisk_space_avail(dp));
	}
}

/*
 * Function:	SdiskobjPrintState
 * Description:	Print the sdisk state in English.
 * Scope:	private
 * Parameters:	state	[RO] (Label_t)
 *			State of the partition to print. Valid values are:
 *			    CFG_CURRENT
 *			    CFG_COMMIT
 *			    CFG_EXIST
 *		dp	[RO, *RO] (Disk_t *)
 *			Disk object handle.
 * Return:	none
 */
static void
SdiskobjPrintState(Label_t state, Disk_t *dp)
{
	write_status(SCR, LEVEL1|LISTITEM|CONTINUE,
		"state:%s%s",
		Sdiskobj_Flag(state, dp, SF_ILLEGAL)	? " illegal"	: "",
		Sdiskobj_Flag(state, dp, SF_NOSLABEL)	? " noslabel"	: "");
}

/*
 * Function:	DiskobjPrintState
 * Description: Print the disk state in English.
 * Scope:	private
 * Parameters:	dp	[RO, *RO] (Disk_t *)
 *			Disk object handle.
 * Return:	none
 */
static void
DiskobjPrintState(Disk_t *dp)
{
	/* validate parameters */
	if (dp == NULL)
		return;

	write_status(SCR, LEVEL1|LISTITEM|CONTINUE,
		"state:%s%s%s%s%s%s%s%s%s",
		disk_bad_controller(dp) ? " badctrl"	 : "",
		disk_unk_controller(dp) ? " unknown"	 : "",
		disk_no_pgeom(dp)	? " nogeom"	 : "",
		disk_cant_format(dp)	? " noformat"	 : "",
		disk_fdisk_req(dp)	? " fdiskreq"	 : "",
		disk_fdisk_exists(dp)	? " fdiskexists" : "",
		disk_format_disk(dp)	? " format"	 : "",
		disk_selected(dp)	? " selected"	 : "",
		disk_initialized(dp)	? " init"	 : "" );
	write_status(SCR, LEVEL1|LISTITEM|CONTINUE,
		"ctype: %-2d",
		disk_ctype(dp));
}

/*
 * Function:	FdiskobjPrintState
 * Description: Print out the fdisk state in English.
 * Scope:	private
 * Parameters:	state	[RO] (Label_t)
 *			State of the partition to print. Valid values are:
 *			    CFG_CURRENT
 *			    CFG_COMMIT
 *			    CFG_EXIST
 *		dp	[RO, *RO] (Disk_t *)
 *			Disk object handle.
 * Return:	none
 */
static void
FdiskobjPrintState(Label_t state, Disk_t *dp)
{
	/* validate parameters */
	if (dp == NULL)
		return;

	if (state != CFG_CURRENT && state != CFG_COMMIT && state != CFG_EXIST)
		return;

	write_status(SCR, LEVEL1|LISTITEM|CONTINUE,
		"state:%s",
		Fdiskobj_Flag(state, dp, FF_NOFLABEL) ? " noflabel"	: "");
}

/*
 * Function:	FdiskobjPrintParts
 * Description:	Print out the partition data structures.
 * Scope:	private
 * Parameters:	state	[RO] (Label_t)
 *			State of the partition to print. Valid values are:
 *			    CFG_CURRENT
 *			    CFG_COMMIT
 *			    CFG_EXIST
 *		dp	[RO, *RO] (Disk_t *)
 *			Disk object handle.
 * Return:	none
 */
static void
FdiskobjPrintParts(Label_t state, Disk_t *dp)
{
	int	part;

	/* validate parameters */
	if (dp == NULL)
		return;

	if (state != CFG_CURRENT && state != CFG_COMMIT && state != CFG_EXIST)
		return;

	WALK_PARTITIONS(part) {
		write_status(SCR, LEVEL1|LISTITEM|CONTINUE, 
			"PART %d  id: %3d  orig: %1d  [%s%s%s]",
			part,
			Partobj_Id(state, dp, part),
			Partobj_Origpart(state, dp, part),
			Partobj_Active(state, dp, part) == ACTIVE   ?
					" active" : " inactive",
			Partobj_Flag(state, dp, part, PF_PRESERVED) ?
					" preserved" : " unpreserved",
			Partobj_Flag(state, dp, part, PF_STUCK)	    ? " stuck" :
					" unstuck");
		FdiskobjPrintPartGeom(state, dp, part);
	}
}

/*
 * Function:	FdiskobjPrintPartGeom
 * Description: Print the geometry associated with the partition.
 * Scope:	private
 * Parameters:	state	[RO] (Label_t)
 *			State of the partition to print. Valid values are:
 *			    CFG_CURRENT
 *			    CFG_COMMIT
 *			    CFG_EXIST
 *		dp	[RO, *RO] (Disk_t *)
 *			Disk object handle.
 *		part	[RO] (int)
 *			Partition index.
 * Return:	none
 */
static void
FdiskobjPrintPartGeom(Label_t state, Disk_t *dp, int part)
{
	/* validate parameters */
	if (dp == NULL || !valid_fdisk_part(part))
		return;

	if (state != CFG_CURRENT && state != CFG_COMMIT && state != CFG_EXIST)
		return;

	write_status(SCR, LEVEL2,
		"  tcyl: %-7d    lcyl: %-7d   dcyl: %-7d   firstcyl: %-7d",
		Partobj_Geom(state, dp, part).tcyl,
		Partobj_Geom(state, dp, part).lcyl,
		Partobj_Geom(state, dp, part).dcyl,
		Partobj_Geom(state, dp, part).firstcyl);
	write_status(SCR, LEVEL2,
		" tsect: %-7d   lsect: %-7d  dsect: %-7d      rsect: %-7d",
		Partobj_Geom(state, dp, part).tsect,
		Partobj_Geom(state, dp, part).lsect,
		Partobj_Geom(state, dp, part).dsect,
		Partobj_Geom(state, dp, part).rsect);
	write_status(SCR, LEVEL2,
		"onecyl: %-7d  hbacyl: %-7d  nhead: %-7d      nsect: %-7d",
		Partobj_Geom(state, dp, part).onecyl,
		Partobj_Geom(state, dp, part).hbacyl,
		Partobj_Geom(state, dp, part).nhead,
		Partobj_Geom(state, dp, part).nsect);
}
