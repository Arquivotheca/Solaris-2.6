#ifndef lint
#pragma ident   "@(#)disk_debug.c 1.36 95/05/19"
#endif	/* lint */
/*
 * Copyright (c) 1993-1995 Sun Microsystems, Inc.  All Rights Reserved. Sun
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
 * MODULE PURPOSE:
 *	Provide a standard set of procedures for printing out
 *	the contents of various disk library structures. Intended
 *	for debugging convenience
 */

#include "disk_lib.h"

/* Public Function Prototypes */

void	print_disk(Disk_t *, char *);
void	print_orig_sdisk(Disk_t *, char *);
void	print_orig_fdisk(Disk_t *, char *);
void	print_commit_sdisk(Disk_t *, char *);
void	print_commit_fdisk(Disk_t *, char *);
void	print_vtoc(struct vtoc *);
void	print_disk_state(Disk_t *, char *);
void	print_geom(Geom_t *);
void	print_slices(Sdisk_t *);
void	print_parts(Fdisk_t *);
void	print_sdisk_state(u_char);
void	print_fdisk_state(u_char);
void	print_dfltmnt_list(char *, Defmnt_t **);

/* Library Function Prototypes */

/* Local Function Prototypes */

/* ******************************************************************** */
/*			PUBLIC SUPPORT FUNCTIONS			*/
/* ******************************************************************** */
/*
 * print_disk_state()
 *	Print the disk state in human readable format :-)
 * Parameters:
 *	disk	- disk structure pointer (NULL if specifying drive
 *		  by 'drive' - 'disk' has precedence)
 *	drive	- name of drive - e.g. c0t0d0 (NULL if specifying
 *		  drive by 'disk')
 * Return:
 *	none
 * Status:
 *	public
 */
void
print_disk_state(Disk_t *disk, char *drive)
{
	Disk_t	*dp;

	if (disk != NULL)
		dp = disk;
	else if ((dp = find_disk(drive)) == NULL)
		return;

	(void) printf("state:%s%s%s%s%s%s%s%s%s%s\r\n",
		disk_bad_controller(dp) ? " badctrl"	 : "",
		disk_unk_controller(dp) ? " nounknown"	 : "",
		disk_no_pgeom(dp)	? " nopgeom"	 : "",
		disk_cant_format(dp)	? " cantformat"	 : "",
		disk_fdisk_req(dp)	? " fdiskreq"	 : "",
		disk_fdisk_exists(dp)	? " fdiskexists" : "",
		disk_bootdrive(dp)	? " bootdrive"	 : "",
		disk_format_disk(dp)	? " format"	 : "",
		disk_selected(dp)	? " selected"	 : "",
		disk_initialized(dp)	? " initialized" : "" );
}

/*
 * print_disk()
 *	Print the disk configuration information in readable format.
 * Parameters:
 *	disk	- disk structure pointer (NULL if specifying drive
 *		  by 'drive' - 'disk' has precedence)
 *	drive	- name of drive - e.g. c0t0d0 (NULL if specifying
 *		  drive by 'disk')
 * Return:
 *	none
 * Status:
 *	semi-private (internal library use only)
 */
void
print_disk(Disk_t *disk, char *drive)
{
	Disk_t	*dp;

	if (disk != NULL)
		dp = disk;
	else if ((dp = find_disk(drive)) == NULL)
		return;

	(void) printf("\nPHYSICAL DRIVE INFO (%s):\r\n", disk_name(dp));
	print_disk_state(dp, NULL);
	(void) printf("ctype: %-2d  cname: %8s\r\n",
			disk_ctype(dp), disk_cname(dp));
	print_geom(disk_geom_addr(dp));

	if (disk_fdisk_exists(dp)) {
		(void) printf("\r\nCURRENT F-DISK INFO (%s):\r\n",
				disk_name(dp));
		print_fdisk_state(fdisk_state(dp));

		(void) printf("\r\nCURRENT F-DISK PARTITION INFO (%s):\r\n",
				disk_name(dp));
		print_parts(&disk_fdisk(dp));
	}

	(void) printf("\r\nCURRENT S-DISK INFO (%s):\r\n", disk_name(dp));
	print_sdisk_state(sdisk_state(dp));

	(void) printf("\r\nCURRENT S-DISK DISK GEOMETRY (%s):\r\n",
			disk_name(dp));
	print_geom(sdisk_geom(dp));

	(void) printf("\r\nCURRENT S-DISK SLICE INFO (%s):\r\n", disk_name(dp));
	print_slices(&disk_sdisk(dp));
}

/*
 * print_vtoc()
 *	print out the VTOC data in readable format to stdout
 * Parameters:
 *	vtoc	- pointer to VTOC structure to print
 * Return:
 *	none
 * Status:
 *	public
 */
void
print_vtoc(struct vtoc * vtoc)
{
	ushort i;

	if (vtoc == NULL)
		return;

	(void) printf("label: %s\r\n", vtoc->v_asciilabel);
	(void) printf("sanity: 0x%lx version: %ld sectsz: %d nparts: %d\r\n",
		vtoc->v_sanity,
		vtoc->v_version,
		vtoc->v_sectorsz,
		vtoc->v_nparts);

	for (i = 0; i < vtoc->v_nparts; i++) {
		(void) printf("  %d tag: %d flg: %d start: %7ld size %7ld\r\n",
			i,
			vtoc->v_part[i].p_tag,
			vtoc->v_part[i].p_flag,
			vtoc->v_part[i].p_start,
			vtoc->v_part[i].p_size);
	}
}

/*
 * print_orig_sdisk()
 *	Print out the original S-disk configuration.
 * Parameters:
 *	disk	- disk structure pointer (NULL if specifying drive
 *		  by 'drive' - 'disk' has precedence)
 *	drive	- name of drive - e.g. c0t0d0 (NULL if specifying
 *		  drive by 'disk')
 * Return:
 *	none
 * Status:
 *	public
 */
void
print_orig_sdisk(Disk_t *disk, char *drive)
{
	Disk_t	*dp;

	if (disk != NULL)
		dp = disk;
	else if ((dp = find_disk(drive)) == NULL)
		return;

	(void) printf("\r\nEXISTING S-DISK INFO (%s):\r\n", disk_name(dp));
	print_sdisk_state(orig_sdisk_state(dp));

	(void) printf("\r\nEXISTING S-DISK DISK GEOMETRY (%s):\r\n",
			disk_name(dp));
	print_geom(orig_sdisk_geom(dp));

	(void) printf("\r\nEXISTING S-DISK SLICE INFO (%s):\r\n",
			disk_name(dp));
	print_slices(&orig_sdisk(dp));
}

/*
 * print_commit_sdisk()
 *	Print the committed s-disk configuration.
 * Parameters:
 *	disk	- disk structure pointer (NULL if specifying drive
 *		  by 'drive' - 'disk' has precedence)
 *	drive	- name of drive - e.g. c0t0d0 (NULL if specifying
 *		  drive by 'disk')
 * Return:
 *	none
 * Status:
 *	public
 */
void
print_commit_sdisk(Disk_t *disk, char *drive)
{
	Disk_t	*dp;

	if (disk != NULL)
		dp = disk;
	else if ((dp = find_disk(drive)) == NULL)
		return;

	(void) printf("\r\nCOMMITTED S-DISK INFO (%s):\r\n", disk_name(dp));
	print_sdisk_state(comm_sdisk_state(dp));

	(void) printf("\r\nCOMMITTED S-DISK DISK GEOMETRY (%s):\r\n",
			disk_name(dp));
	print_geom(comm_sdisk_geom(dp));

	(void) printf("\r\nCOMMITTED S-DISK SLICE INFO (%s):\r\n",
			disk_name(dp));
	print_slices(&comm_sdisk(dp));
}

/*
 * print_orig_fdisk()
 *	Print the committed f-disk data structure.
 * Parameters:
 *	disk	- disk structure pointer (NULL if specifying drive
 *		  by 'drive' - 'disk' has precedence)
 *	drive	- name of drive - e.g. c0t0d0 (NULL if specifying
 *		  drive by 'disk')
 * Return:
 *	none
 * Status:
 *	public
 */
void
print_orig_fdisk(Disk_t * disk, char * drive)
{
	Disk_t	*dp;

	if (disk != NULL)
		dp = disk;
	else if ((dp = find_disk(drive)) == NULL)
		return;

	(void) printf("\r\nEXISTING F-DISK INFO (%s):\r\n", disk_name(dp));
	print_fdisk_state(orig_fdisk_state(dp));

	(void) printf("\r\nEXISTING F-DISK PARTITION INFO (%s):\r\n",
			disk_name(dp));
	print_parts(&orig_fdisk(dp));
}

/*
 * print_commit_fdisk()
 *	Print the committed f-disk data structure.
 * Parameters:
 *	disk	- disk structure pointer (NULL if specifying drive
 *		  by 'drive' - 'disk' has precedence)
 *	drive	- name of drive - e.g. c0t0d0 (NULL if specifying
 *		  drive by 'disk')
 * Return:
 *	none
 * Status:
 *	public
 */
void
print_commit_fdisk(Disk_t * disk, char * drive)
{
	Disk_t	*dp;

	if (disk != NULL)
		dp = disk;
	else if ((dp = find_disk(drive)) == NULL)
		return;

	(void) printf("\r\nCOMMITTED F-DISK INFO (%s):\r\n", disk_name(dp));
	print_fdisk_state(comm_fdisk_state(dp));

	(void) printf("\r\nCOMMITTED F-DISK PARTITION INFO (%s):\r\n",
		disk_name(dp));
	print_parts(&comm_fdisk(dp));
}

/*
 * print_parts()
 *	Print out the partition data structures in 'fp'.
 * Parameters:
 *	fp	- fdisk structure pointer containing partitions
 * Return:
 *	none
 * Status:
 *	public
 */
void
print_parts(Fdisk_t *fp)
{
	int	i;

	if (fp == NULL)
		return;

	WALK_PARTITIONS_REAL(i) {
		(void) printf(
			"PART %d  id: %3d  orig: %1d  [%-8s %-6s %7s]\r\n",
			i + 1,
			fp->part[i].id,
			fp->part[i].origpart,
			(fp->part[i].active == ACTIVE ? "active" : "inactive"),
			fp->part[i].state & PF_PRESERVE ? "pres" : "unpres",
			fp->part[i].state & PF_STUCK ? "stuck" : "unstuck");
		print_geom(&fp->part[i].geom);
	}
}

/*
 * print_slices()
 *	Print out the slice data in 'sp'.
 * Parameters:
 *	sp	- s-disk structure pointer containing slices
 * Return:
 *	none
 * Status:
 *	public
 */
void
print_slices(Sdisk_t *sp)
{
	int	i;

	if (sp == NULL)
		return;

	WALK_SLICES(i) {
		(void) printf(MSG11_SLICE_TABLE_ENTRY,
		i,
		sp->slice[i].start,
		sp->slice[i].size,
		sp->slice[i].mntpnt,
		sp->slice[i].mntopts		   ? sp->slice[i].mntopts : "",
		sp->slice[i].state & SLF_PRESERVE  ? " preserved"	  : "",
		sp->slice[i].state & SLF_LOCK	   ? " locked"		  : "",
		sp->slice[i].state & SLF_STUCK	   ? " stuck"		  : "",
		sp->slice[i].state & SLF_ALIGNED   ? " aligned"		  : "",
		sp->slice[i].state & SLF_EXPLICIT  ? " explicit"	  : "",
		sp->slice[i].state & SLF_IGNORE	   ? " ignored"		  : "");
	}
}

/*
 * print_geom()
 * 	Print out the geometry structure referenced by 'gp'.
 * Parameters:
 *	gp 	- NULL or valid geometry structure pointer
 * Return:
 *	none
 * Status:
 *	public
 */
void
print_geom(Geom_t *gp)
{
	if (gp == NULL) {
		(void) printf("no disk geometry currently configured\n");
	} else {
		(void) printf(
		"  tcyl: %-7d    lcyl: %-7d   dcyl: %-7d   firstcyl: %-7d\n",
			gp->tcyl,
			gp->lcyl,
			gp->dcyl,
			gp->firstcyl);
		(void) printf(
		" tsect: %-7d   lsect: %-7d  dsect: %-7d      rsect: %-7d\n",
			gp->tsect,
			gp->lsect,
			gp->dsect,
			gp->rsect);
		(void) printf(
		"onecyl: %-7d  hbacyl: %-7d  nhead: %-7d      nsect: %-7d\n",
			gp->onecyl,
			gp->hbacyl,
			gp->nhead,
			gp->nsect);
	}
}

/*
 * print_sdisk_state()
 *	Print out the S-disk state in English.
 * Parameters:
 *	state	- state field from S-disk structure
 * Return:
 *	none
 * Status:
 *	public
 */
void
print_sdisk_state(u_char state)
{
	(void) printf("state:%s%s%s\r\n",
		state & SF_ILLEGAL  ? " illegal"	: "",
		state & SF_S2MOD    ? " slice2modified"	: "",
		state & SF_NOSLABEL ? " noslabel"	: "");
}

/*
 * print_fdisk_state()
 *	Print out the F-disk state in English.
 * Parameters:
 *	state	- state field from F-disk structure
 * Return:
 *	none
 * Status:
 *	public
 */
void
print_fdisk_state(u_char state)
{
	(void) printf("state:%s\r\n",
			state & FF_NOFLABEL ? " noflabel"	: "");
}

/*
 * print_dfltmnt_list()
 *	Print out the default mount list specified by **def_ml.
 * Parameters:
 *	comment	- comment string to be put in message (NULL if none)
 *	def_ml	- pointer to array of default mount list structures.
 * Return:
 *	none
 * Status:
 *	public
 */
void
print_dfltmnt_list(char *comment, Defmnt_t **def_ml)
{
	Defmnt_t	**dfltp;
	Defmnt_t	*def_me;

	(void) printf("%s %s\n", MSG0_TRACE_MOUNT_LIST,
		comment == NULL ? "" : comment);

	for (dfltp = def_ml; *dfltp; dfltp++) {
		def_me = *dfltp;
		(void) printf("\t%-25.25s %10s %7d (%7d)\r\n",
			def_me->name,
			def_me->status == DFLT_SELECT ? "selected" :
			def_me->status == DFLT_DONTCARE ? "dontcare" :
			"ignore",
			def_me->size,
			def_me->expansion);
	}
}
