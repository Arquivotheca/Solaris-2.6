#ifndef lint
#pragma ident   "@(#)disk_find.c 1.106 95/06/16 SMI"
#endif
/*
 * Copyright (c) 1991-1995 Sun Microsystems, Inc.  All Rights Reserved. Sun
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
 * MODULE PURPOSE:	This module contains code necessary for building the
 *			original disk chain from physical attributes of the
 *			system.
 */
#include "disk_lib.h"
#include "ibe_api.h"

#include <signal.h>
#include <dirent.h>

/*
 * INTEL SPECIFIC IOCTL FOR HBA DISK GEOMETRY (not in merged <sys/dkio.h>)
 */
#ifndef	DKIOCG_VIRTGEOM
#define	DKIOCG_VIRTGEOM	(DKIOC|33)
#endif

/*
 * INTEL SPECIFIC IOCTL FOR PHYSICAL DISK GEOMETRY (not in merged <sys/dkio.h>)
 */
#ifndef	DKIOCG_PHYGEOM
#define	DKIOCG_PHYGEOM	(DKIOC|32)
#endif

/* Public Function Prototypes */

int		build_disk_list(void);

/* Library Function Prototypes */

Disk_t *	_alloc_disk(char *);
void		_add_disk_to_list(Disk_t *);
void		_lock_unusable_slices(Disk_t *);
void		_mark_overlap_slices(Disk_t *);
void		_dealloc_disk(Disk_t *);

/* Local Function Prototypes */

static int 	_init_disk(char *);
static int	_sdisk_info_init(Disk_t *);
static int 	_get_sdisk_label(int, Disk_t *);
static int	_controller_info_init(Disk_t *);
static int	_disk_ctype_info(Disk_t *, int);
static char * 	_get_fsname_from_sb(Disk_t *, int);
static void	_setup_sdisk_mntpnts(Disk_t *);
static int	_fdisk_info_init(Disk_t *);
static int	_install_default_slabel(Disk_t *);

/* ******************************************************************** */
/*			PUBLIC SUPPORT FUNCTIONS			*/
/* ******************************************************************** */
/*
 * build_disk_list()
 *	Create the disk chain (accessed by first_disk()) based on
 *	physical disks attached to the system. Disk structures are
 *	initialize with the current state info for each drive, and
 *	their current, committed, and original F-disk and S-disk
 *	sets are all identical. All drives are unselected, and
 *	the bootdrive is marked and at the head of the chain. Due
 *	to dynamics in the ordering of the chain, the first disk
 *	in the chain should never be stored locally, but should
 *	always be accessed by using the first_disk() function.
 *
 *	NOTE:	Sun CD-ROM drives are not ever included in the
 *		disk chain
 *	NOTE:	This routine should be used exclusively from the
 *		load_disks() routine
 * Parameters:
 *	none
 * Return:
 *	# >= 0	- number of disks in disk list
 * Status:
 *	public
 */
int
build_disk_list(void)
{
	Disk_t		*dp;
	struct dirent	*dent;
	DIR		*dir;
	int		count = 0;
	char		drive[MAXNAMELEN];
	char		oldpath[MAXNAMELEN + 1];

	if (getcwd(oldpath, MAXNAMELEN) == NULL ||
			first_disk() != NULL ||
			chdir("/dev/rdsk") < 0 ||
			(dir = opendir(".")) == NULL)
		return (0);
	/*
	 * Find all s2 and p0 entries in the raw devices directory and
	 * if the drive does not already exist in the disk list, intialize
	 * it and add it.
	 */
	while ((dent = readdir(dir)) != NULL) {
		if (dent->d_name[0] == '.')
			continue;

		if (_whole_disk_name(drive, dent->d_name) == 0)
			if (_init_disk(drive) == 1)
				count++;
	}

	(void) closedir(dir);
	(void) chdir(oldpath);

	/* mark the boot disk and recommit (CFG_EXIST must be loaded) */

	if ((dp = _init_bootdisk()) != NULL)
		_init_commit_orig(dp);

	return (count);
}

/* ******************************************************************** */
/*			LIBRARY SUPPORT FUNCTIONS			*/
/* ******************************************************************** */
/*
 * _mark_overlap_slices()
 *	Scan through the slices and mark those which are unlabelled
 *	and which overlap other sized slices as OVERLAP. First mark
 *	unnamed slices which overlap named slices. Then mark slices
 *	which span the whole drive and are unnamed. Then, if two
 *	unlabelled slices occur which overlap, the latter of the
 *	two is marked.
 *
 *	NOTE:	Used at disk structure initialization time.
 * Parameters:
 *	dp	- non-NULL disk structure pointer
 * Return:
 *	none
 * Status:
 *	semi-private (internal library use only)
 */
void
_mark_overlap_slices(Disk_t *dp)
{
	int	i;
	int	j;
	int	count;
	int	*sp;

	/* mark unnamed slices which overlap named slices */

	for (i = numparts - 1; i >= 0; i--) {
		if (slice_mntpnt_exists(dp, i) ||
				slice_size(dp, i) == 0 ||
				(slice_locked(dp, i) &&
				    disk_initialized(dp)))
			continue;

		count = slice_overlaps(dp, i, slice_start(dp, i),
				slice_size(dp, i), &sp);

		if (count != 0) {
			for (j = 0; j < count; j++) {
				if (slice_mntpnt_is_fs(dp, sp[j])) {
					slice_mntpnt_set(dp, i, OVERLAP);
					break;
				}
			}
		}
	}

	/* mark unnamed slices which span the drive */

	WALK_SLICES(i) {
		if ((slice_locked(dp, i) && disk_initialized(dp)) ||
				slice_mntpnt_exists(dp, i) ||
				slice_size(dp, i) != accessible_sdisk_blks(dp))
			continue;

		slice_mntpnt_set(dp, i, OVERLAP);
	}

	/* mark unnamed slices which overlap non-overlap slices */

	for (i = numparts - 1; i >= 0; i--) {
		if (slice_mntpnt_exists(dp, i) ||
				slice_size(dp, i) == 0 ||
				(slice_locked(dp, i) &&
				    disk_initialized(dp)))
			continue;

		if (slice_overlaps(dp, i, slice_start(dp, i),
				slice_size(dp, i), (int **)0) != 0)
			slice_mntpnt_set(dp, i, OVERLAP);
	}
}

/*
 * _add_disk_to_list()
 *	Add the disk structure 'disk' to the end of the disk list.
 * Parameters:
 *	disk	- non-NULL disk structure pointer to add to disk list
 * Return:
 *	none
 * Status:
 *	semi-private (internal library use only)
 */
void
_add_disk_to_list(Disk_t *disk)
{
	Disk_t	*dp;

	if (disk == NULL)
		return;

	_set_next_disk(disk, NULL);
	if (first_disk() == NULL)
		_set_first_disk(disk);
	else {
		for (dp = first_disk(); next_disk(dp); dp = next_disk(dp))
			;
		_set_next_disk(dp, disk);
	}
}

/*
 * _alloc_disk()
 *	Allocate space for the disk structure, initialize the drive
 *	name, state, and S-disk geometry pointer.
 * Parameters:
 *	drive	 - unique drive name (e.g. c0t0d0)
 * Return:
 *	Disk_t * - pointer to newly allocated disk structure
 * Status:
 *	semi-private (internal library use only)
 * Algorithm:
 *	The S-disk geometry pointer is set to point at the disk geometry
 *	structure (the default SPARC case).
 */
Disk_t *
_alloc_disk(char *drive)
{
	Disk_t	*dp;

	if ((dp = (Disk_t *) xcalloc(sizeof (Disk_t))) != NULL) {
		(void) strcpy(disk_name(dp), drive);
		disk_state_set(dp, DF_INITIAL);
		sdisk_geom_set(dp, disk_geom_addr(dp));
		_set_next_disk(dp, NULL);
	}

	return (dp);
}

/*
 * _lock_unusable_slices()
 *	Set the lock bit on all slices in the upper VTOC (i.e. 8-15) and
 *	slice 2 on fdisks systems. This routine should be used during disk
 *	initialization after slice configuration has completed.
 * Parameters:
 *	dp	- non-NULL disk structure pointer
 *		  (state: okay)
 * Return:
 *	none
 * Status:
 *	semi-private (internal library use only)
 */
void
_lock_unusable_slices(Disk_t *dp)
{
	int	i;

	if (dp == NULL || disk_not_okay(dp))
		return;

	if (disk_fdisk_req(dp))
		slice_lock_on(dp, ALL_SLICE);

	for (i = LAST_STDSLICE + 1; i < numparts; i++)
		slice_lock_on(dp, i);
}

/*
 * _dealloc_disk()
 *	Free the space used by the disk structure referenced by 'dp'.
 * Parameters:
 *	dp	- non-NULL valid disk structure pointer
 * Return:
 *	none
 * Status:
 *	semi-private (internal library use only)
 */
void
_dealloc_disk(Disk_t *dp)
{
	int	i;

	WALK_SLICES(i) {
		if (slice_mntopts(dp, i))
			slice_mntopts_clear(dp, i);
	}

	free(dp);
}

/* ******************************************************************** */
/*			INTERNAL SUPPORT FUNCTIONS			*/
/* ******************************************************************** */

/*
 * _init_disk()
 *	Initialize the disk data structure and, if everything is
 *	successful, add it to the disk list.
 * Parameters:
 *	drive	- disk name (e.g. c0t0d0)
 * Return:
 *	0	- no disk added to disk list
 *	1	- disk added to disk list
 * Status:
 *	private
 * Algorithm:
 *	- query for controller and geometry information
 *	- query for F-disk data
 *	- query for S-disk data
 *	- do cleanup and finalization work
 *	- add the disk to the chain
 */
static int
_init_disk(char *drive)
{
	Disk_t	*dp;
	int	status;

	/* disk has already been loaded, so ignore this entry */
	if (find_disk(drive) != NULL)
		return (0);

	if ((dp = _alloc_disk(drive)) == NULL)
		return (0);

	/* get the controller and disk geometry information */
	if ((status = _controller_info_init(dp)) == -1) {
		_dealloc_disk(dp);
		return (0);
	} else if (status == 1) {
		disk_initialized_on(dp);
		_init_commit_orig(dp);
		_add_disk_to_list(dp);
		return (1);
	}

	/*
	 * check to see if there is a p0 device for the drive and if so
	 * initialize the data fdisk data for the drive; validate that
	 * the data returned is reasonable
	 */
	if ((status = _fdisk_info_init(dp)) == -1) {
		_dealloc_disk(dp);
		return (0);
	} else if (status == 1) {
		disk_initialized_on(dp);
		_init_commit_orig(dp);
		_add_disk_to_list(dp);
		return (1);
	}

	/*
	 * initialize the data S-disk current drive state
	 */
	if (_sdisk_info_init(dp) < 0) {
		_dealloc_disk(dp);
		return (0);
	}

	/*
	 * copy the current F-Disk and S-Disk configuration into
	 * the committed and original stores, and add the drive to
	 * the list of drives on the system
	 */
	disk_initialized_on(dp);
	_init_commit_orig(dp);
	_add_disk_to_list(dp);

	return (1);
}

/*
 * _sdisk_info_init()
 *	Initialize the sdisk current, committed, and orig structure and
 *	the flag field according to what is found out on the disk. Note
 *	that the "geom" pointer is already set to either the disk geom
 *	(SPARC machines), the Solaris F-disk geometry structure (if one
 *	was found), or is NULL (F-disk required, but no Solaris partition
 *	found).
 * Parameters:
 *	dp	- disk structure pointer
 * Return:
 *	-1	- ignore disk
 *	 0	- continue processing disk
 * Status:
 *	private
 * Algorithm:
 * 	- ignore devices that can't be accessed
 *	- ignore non-character devices
 * 	- log drives with bad controllers, but don't ignore them
 * 	- ignore CDROM disks
 * 	- mark drives with unknown disk types, but don't ignore them
 * 	- get the S-Disk concept of the disk geometry from the drive
 * 	- get the S-Disk VTOC label information
 *	- adjust the S-disk geometry pointer
 */
static int
_sdisk_info_init(Disk_t *dp)
{
	char	device[MAXNAMELEN];
	int	fd;

	if (sdisk_geom(dp) == NULL)
		return (0);

	/*
	 * reset the S-disk right away (don't null it or the locked
	 * slices won't get setup correctly), since there may be an
	 * F-disk partition, but no slice 2, and you want to make
	 * sure the proper slices are locked
	 */
	(void) _reset_sdisk(dp);

	/* this assumes that the existing system has a slice 2 */
	(void) sprintf(device, "/dev/rdsk/%ss2", disk_name(dp));

	if ((fd = open(device, O_RDWR | O_NDELAY)) < 0)
		return (-1);

	if (_get_sdisk_label(fd, dp) < 0)
		sdisk_state_set(dp, SF_NOSLABEL);

	(void) close(fd);
	return (0);
}

/*
 * _get_sdisk_label()
 *	Retrieve the disk label VTOC info from the drive and
 *	load the "start" and "size" information found there
 *	into the current slice table. Then scan the superblock
 *	area for each slice and, store the mount name for the
 *	slice. Overlapping slices will result in a "best fit"
 *	selection with the other slice marked "overlap".
 * Parameters:
 *	fd	- disk device open file descriptor to use in
 *		  retrieving the VTOC
 *	dp	- non-NULL disk structure pointer
 * Return:
 *	-1	- attempt to read label information failed
 *	 0	- success
 * Status:
 *	private
 * Algorithm:
 *	Special case: if the VTOC read fails, it's possible it happened
 *	because someone made a filesystem on Slice 2 on an INTEL machine.
 *	We want to report this, so if the read_vtoc() fails, peek into
 *	slice 2 and see if there is a valid superblock. If so, mark the
 *	slice flags as ILLEGAL after loading the fs name into slice 2.
 */
static int
_get_sdisk_label(int fd, Disk_t *dp)
{
	struct vtoc	vtoc;
	char		*fsp;
	int		i;
	int		over;
	int		size;

	/*
	 * read the VTOC; on i386 systems, if the read fails, look for an
	 * illegal slice 2 condition. In any case, if read_vtoc()
	 * fails there is no further searching done.
	 */
	if (read_vtoc(fd, &vtoc) < 0) {
		if (streq(get_default_inst(), "i386")) {
			if ((fsp = _get_fsname_from_sb(dp, ALL_SLICE)) !=
					NULL) {
				slice_mntpnt_set(dp, ALL_SLICE, fsp);
				sdisk_set_touch2(dp);
				sdisk_set_illegal(dp);
			}
		}

		return (-1);
	}

	/*
	 * set up start and size for slices based on VTOC. If it succeeds,
	 * load the mount point fields in the disk structure based on the tag
	 * information as much as possible (i.e. ROOT and SWAP)
	 */
	WALK_SLICES(i) {
		/*
		 * we allow the alternate sector slice lock on fdisk systems to
		 * be overridden if it's existing size is non-zero in order to
		 * preserve existing labels which will have the slice situated
		 * in a different configuration from the current default
		 * (preserve data)
		 */
		if (i != ALT_SLICE && slice_locked(dp, i))
			continue;

		if (i == ALT_SLICE && vtoc.v_part[i].p_size == 0)
			continue;

		/* force the vtag field for slice 9 - fdisk systems only */
		if (i == ALT_SLICE)
			vtoc.v_part[i].p_tag = V_ALTSCTR;
		else {
			/* clear the mntpnt name in prep for loading */
			slice_mntpnt_clear(dp, i);
		}

		/*
		 * since we are immediately rounding the starting cylinder up
		 * to the nearest cylinder boundary, we may need to adjust the
		 * size accordingly, and mark the slice as SLF_ALIGNED so that
		 * it cannot be preserved. If the size is not cylinder aligned,
		 * an adjustment is also made, but the slice is not marked.
		 */
		size = vtoc.v_part[i].p_size;
		slice_start_set(dp, i,
			blocks_to_cyls(dp, vtoc.v_part[i].p_start));
		over = vtoc.v_part[i].p_start % one_cyl(dp);
		if (over > 0) {
			slice_aligned_on(dp, i);
			size -= (one_cyl(dp) - over);
		}

		slice_size_set(dp, i, blocks_to_blocks(dp, size));

		/* set the mount point name on non-zero sized slices */
		if (slice_size(dp, i) > 0) {
			switch (vtoc.v_part[i].p_tag) {
			    case V_ROOT:
				(void) set_slice_mnt(dp, i, ROOT, NULL);
				break;

			    case V_SWAP:
				(void) set_slice_mnt(dp, i, SWAP, NULL);
				break;

			    case V_BACKUP:
				(void) set_slice_mnt(dp, i, OVERLAP,
						NULL);
				break;

			    case V_CACHE:
				/*
				 * NOTE: the assumption that V_CACHE == /.cache
				 * is true as of Solaris 2.5, but may not always
				 * be true.
				 */
				(void) set_slice_mnt(dp, i, CACHE, NULL);
				break;

			    default:
				break;
			}

			/* get remaining mount points from super-blocks */
			_setup_sdisk_mntpnts(dp);
		}
	}

	return (0);
}

/*
 * _controller_info_init()
 *	Find out the controller type and make sure the controller is
 *	actual responding. This routine should only "fail" if the info
 *	it finds indicates the drive should be ignored.
 *
 *	NOTE:	i386 returns the HBA geometry for ioctl(DKIOCGGEOM) if
 *		there is no Solaris partition
 * Parameters:
 *	dp	- non-NULL disk structure pointer
 * Return:
 *	 1	- drive controller status failed. Further probing
 *		  efforts are pointless, but the drive should not
 *		  be ignored.
 *	 0	- drive valid to continue probing efforts
 *	-1	- the calling routine should ignore this drive
 * Status:
 *	private
 */
static int
_controller_info_init(Disk_t *dp)
{
	struct dk_geom	dkg;
	char		device[MAXNAMELEN];
	int		fd;
	int		status;
	int		p = 0;

	if (dp == NULL)
		return (-1);

	/* must use p0 device on fdisk supporting systems */
	(void) sprintf(device, "/dev/rdsk/%sp0", disk_name(dp));
	if (access(device, F_OK) == 0) {
		p++;
		/*
		 * all fdisk supporting systems have the DF_FDISKEXISTS bit
		 * set and have 16 slices
		 */
		numparts = 16;
		disk_state_set(dp, DF_FDISKEXISTS);

		/* fdisk exposure is only allowed for i386 */
		if (streq(get_default_inst(), "i386"))
			disk_state_set(dp, DF_FDISKREQ);
	} else
		(void) sprintf(device, "/dev/rdsk/%ss2", disk_name(dp));

	if ((fd = open(device, O_RDWR|O_NDELAY)) < 0)
		return (-1);

	if ((status = _disk_ctype_info(dp, fd)) != 0) {
		(void) close(fd);
		return (status);
	}

	if (ioctl(fd, (p ? DKIOCG_PHYGEOM : DKIOCGGEOM), &dkg) < 0) {
		(void) close(fd);
		return (1);
	}

	/* put a default label on unlabelled non-Fdisk disks */
	if (p == 0 && dkg.dkg_pcyl == 0) {
		if (_install_default_slabel(dp) < 0 ||
				ioctl(fd,
				(p ? DKIOCG_PHYGEOM : DKIOCGGEOM),
					&dkg) < 0) {
			(void) close(fd);
			return (1);
		}
	}

	/* if still unlabelled, the drive is messed up */
	if (dkg.dkg_pcyl == 0) {
		/* IDE drives can't handle reformatting */
		if (disk_ctype(dp) != DKC_DIRECT)
			disk_state_unset(dp, DF_CANTFORMAT);

		(void) close(fd);
		return (1);
	}

	/*
	 * variable sector drive drivers sometimes get confused
	 * and return a NULL nsect value. Install will not deal
	 * with these drives.
	 */
	if (dkg.dkg_nsect == (u_short)0) {
		(void) close(fd);
		return (1);
	}

	/* fdisk supporting systems don't use the first cylinder */
	disk_geom_firstcyl(dp) = (p ? 1 : 0);
	disk_geom_nhead(dp) = dkg.dkg_nhead;
	disk_geom_nsect(dp) = dkg.dkg_nsect;
	disk_geom_onecyl(dp) = dkg.dkg_nhead * dkg.dkg_nsect;
	disk_geom_hbacyl(dp) = disk_geom_onecyl(dp);
	disk_geom_tcyl(dp) = dkg.dkg_pcyl;
	disk_geom_tsect(dp) = cyls_to_blocks(dp, disk_geom_tcyl(dp));
	disk_geom_lcyl(dp) = dkg.dkg_ncyl;
	disk_geom_lsect(dp) = cyls_to_blocks(dp, disk_geom_lcyl(dp));
	disk_geom_dcyl(dp) = disk_geom_lcyl(dp) - disk_geom_firstcyl(dp);
	disk_geom_dsect(dp) = cyls_to_blocks(dp, disk_geom_dcyl(dp));
	disk_geom_rsect(dp) = 0;

	/*
	 * update the HBA sector/cyl value for i386 BIOS constraint
	 * calculations
	 */
	if (streq(get_default_inst(), "i386") &&
			ioctl(fd, DKIOCG_VIRTGEOM, &dkg) == 0)
		disk_geom_hbacyl(dp) = dkg.dkg_nhead * dkg.dkg_nsect;

	disk_state_unset(dp, DF_NOPGEOM);
	disk_state_unset(dp, DF_CANTFORMAT);

	(void) close(fd);
	return (0);
}

/*
 * _disk_ctype_info()
 *	Get the controller type information (type and name) and load
 *	it into the disk structure.
 * Parameters:
 *	dp	- non-NULL disk structure pointer
 *	fd	- open file descriptor for "whole disk" device
 * Return:
 *	 1	- drive controller status failed. Further probing
 *		  efforts are pointless, but the drive should not
 *		  be ignored.
 *	 0	- drive valid to continue probing efforts
 *	-1	- the calling routine should ignore this drive
 * Status:
 *	private
 */
static int
_disk_ctype_info(Disk_t *dp, int  fd)
{
	struct dk_cinfo	dkc;

	if (ioctl(fd, DKIOCINFO, &dkc) < 0)
		return (1);

	disk_state_unset(dp, DF_BADCTRL);
	if (dkc.dki_ctype == DKC_CDROM)
		return (-1);

	disk_ctype_set(dp, dkc.dki_ctype);
	disk_cname_set(dp, dkc.dki_cname);
	if (dkc.dki_ctype == DKC_UNKNOWN)
		return (1);

	disk_state_unset(dp, DF_UNKNOWN);
	return (0);
}

/*
 * _setup_sdisk_mntpnts()
 *	Load the current filesystem names from existing superblocks for
 *	slices which do not already have mount point names and which have
 *	a non-zero size. This routine should only be used during disk
 *	initialization. Load the file system information, accounting for
 *	swap (which won't have a superblock, but should have a mntpnt label),
 *	and the alternate sector slice.
 *
 *	NOTE:	slice size and start are assumed to already be set at this
 *		point
 *
 *	NOTE:	overlapping UFS fs will cause duplicate mountpoints at this
 *		point. This will be resolved further down in this routine
 *		If slice 2 has a superblock at this point, it is a legal
 *		SPARC configuration only.
 *
 * Parameters:
 *	dp	  - non-NULL disk structure pointer
 * Return:
 *	none
 * Status:
 *	private
 * Algorithm:
 *	Clear the current slice configuration, set it up according to
 *	the way the drive is configured, and then copy the final setup
 *	to the committed.
 */
static void
_setup_sdisk_mntpnts(Disk_t *dp)
{
	int	i, j;
	char	*fsp;

	if (dp == NULL)
		return;

	WALK_SLICES(i) {
		if (slice_mntpnt_exists(dp, i) ||
				slice_size(dp, i) == 0)
			continue;

		if (fsp = _get_fsname_from_sb(dp, i)) {
			slice_mntpnt_set(dp, i, fsp);
			if (i == ALL_SLICE) {
				if (streq(get_default_inst(), "i386"))
					sdisk_set_illegal(dp);

				sdisk_set_touch2(dp);
			}
		}

		/*
		 * determine what to do with the alternate sector slice
		 * on fdisk supporting systems
		 */
		if (i == ALT_SLICE) {
			if (_disk_is_scsi(dp)) {
				(void) _set_slice_size(dp, i, 0);
			} else {
				if (slice_size(dp, i) > 0)
					(void) slice_mntpnt_set(dp, i,
						ALTSECTOR);
			}
		}
	}

	/*
	 * get rid of overlapping duplicate fs mountpoints on the drive due
	 * to concurrent superblock areas (give fs credit to the smaller of
	 * the two file systems - best fit)
	 */
	WALK_SLICES(i) {
		if (slice_mntpnt_isnt_fs(dp, i))
			continue;

		for (j = i + 1; j < numparts; j++) {
			if (slice_mntpnt_isnt_fs(dp, j))
				continue;
			/*
			 * check for same mount points with matching
			 * superblock areas
			 */
			if (strcmp(slice_mntpnt(dp, i),
						slice_mntpnt(dp, j)) == 0 &&
					slice_start(dp, i) ==
						slice_start(dp, j)) {
				slice_mntpnt_clear(dp,
					slice_size(dp, i) <= slice_size(dp, j)
					? j : i);
			}
		}
	}

	/*
	 * clear Slice 2 if it got accidentally touched in this sorting
	 * process
	 */
	if ((slice_size(dp, ALL_SLICE) == sdisk_geom_tcyl(dp) * one_cyl(dp)) &&
			slice_start(dp, ALL_SLICE) == 0 &&
			slice_mntpnt_not_exists(dp, ALL_SLICE))
		sdisk_unset_touch2(dp);

	_mark_overlap_slices(dp);
}

/*
 * _get_fsname_from_sb()
 *	Get the superblock from a slice (if available).
 * Parameters:
 *	dp	- non-NULL disk structure pointer
 *	slice	- slice index number
 * Return:
 *	NULL	- no valid FS name found
 *	char *	- pointer to temporary structure containing FS name
 * Status:
 *	private
 * Algorithm:
 *	Open the raw device, scan to the superblock offset, and read
 *	what should be the first superblock (assuming there was one -
 *	check the "magic" field to see). If the name given is "/a...",
 *	then strip off the leading "/a" to get the name of the real file
 *	system, otherwise, just copy the name.
 */
static char *
_get_fsname_from_sb(Disk_t *dp, int slice)
{
	static int		sblock[SBSIZE/sizeof (int)];
	static struct fs 	*fsp = (struct fs *) sblock;
	char			devpath[MAXNAMELEN];
	int			fd;

	(void) memset(fsp, 0, (SBSIZE/sizeof (int)) * sizeof (int));
	(void) sprintf(devpath, "/dev/rdsk/%ss%d", disk_name(dp), slice);

	/* attempt to open the disk; if it fails, skip it */
	if ((fd = open(devpath, O_RDONLY | O_NDELAY)) < 0)
		return (NULL);

	if (lseek(fd, SBOFF, SEEK_SET) == -1) {
		(void) close(fd);
		return (NULL);
	}

	if (read(fd, fsp, sizeof (sblock)) != sizeof (sblock)) {
		(void) close(fd);
		return (NULL);
	}

	(void) close(fd);

	/* make sure you aren't going to load bogus data */
	if (fsp->fs_magic != FS_MAGIC ||
			fsp->fs_fsmnt[0] != '/' ||
			strlen(fsp->fs_fsmnt) > (size_t)(MAXMNTLEN - 1))
		return (NULL);

	/*
	 * make sure the suprblock does not represent a file system bigger
	 * than the slice
	 */
	if (fsp->fs_ncyl > blocks_to_cyls(dp, slice_size(dp, slice)))
		return (NULL);

	if (strcmp(fsp->fs_fsmnt, "/a") == 0)
		return (ROOT);

	if (strncmp(fsp->fs_fsmnt, "/a/", 3) == 0)
		return (&fsp->fs_fsmnt[2]);

	return (fsp->fs_fsmnt);
}

/*
 * _fdisk_info_init()
 *	Initialize the F-disk data structure for 'dp' according to what is
 *	found on the disk. If there is no Solaris partition at this point, be
 *	sure to NULL out the sdisk geometry pointer or it will incorrectly
 *	reference the main disk geometry.
 *
 *	The following sanity checks are performed:
 *
 *	(1) make sure that the numsect field is the same as the label
 *		geometry's concept of the nsect/cyl * total cyl
 *	(2) make sure no partition projects off the end of the disk
 *
 *	If either of these tests fails, the NOPGEOM error flag which
 *	was previously cleared in _controller_info_init(), is reset.
 * Parameters:
 *	dp	- disk structure pointer (assumed to be non-NULL)
 * Return:
 *	 1	- fdisk input is inconsistent. Further probing
 *		  efforts are pointless, but the drive should
 *		  not be ignored
 *	-1	- ignore the drive altogether
 *	 0	- drive okay
 * Status:
 *	private
 */
static int
_fdisk_info_init(Disk_t *dp)
{
	char		device[MAXNAMELEN];
	struct dk_geom	dkg;
	char		cmd[64];
	char		buf[128];
	FILE		*pp;
	int		i, id, act, rsect, tsect, d;
	int		numacyl;
	int		fd;
	int		pid;
	int		origpart = 1;

	if (dp == NULL)
		return (-1);

	/* this routine is unneccessary unless the system supports an fdisk */
	if (disk_no_fdisk_exists(dp))
		return (0);

	/*
	 * clear the S-disk geometry pointer (reset later if there turns out
	 * to be a Solaris partition) and reset all fdisk partition entries
	 * and status fields
	 */
	sdisk_geom_clear(dp);
	(void) _reset_fdisk(dp);

	/*
	 * execute the appropriate command to obtain the current fdisk
	 * configuration information
	 */
	(void) sprintf(device, "/dev/rdsk/%sp0", disk_name(dp));
	if (streq(get_default_inst(), "ppc")) {
		/* use the fdisk simulator for the PowerPC in read-only mode */
		(void) sprintf(cmd,
			"/usr/sbin/install.d/prep_partition %s 2>&1", device);
	} else {
		/* the /dev/fd file system must be mounted for this call */
		(void) sprintf(cmd,
			"/sbin/fdisk -n -R -W /dev/fd/2 %s 2>&1 >/dev/null",
			device);
	}

	if (get_trace_level() > 4) {
		write_status(LOGSCR, LEVEL0,
			"Loading fdisk info using \"%s\"", cmd);
	}
		
	/*
	 * read in fdisk table definition data printed in "fdisk -W" format
	 */
	if ((pp = (FILE *)popen(cmd, "r")) != NULL) {
		/* skip the comment lines */
		while ((feof(pp) == 0) &&
				fgets(buf, sizeof (buf), pp) != NULL &&
				strncmp(buf, "* Id", 4) != 0) {
			if (get_trace_level() > 4) {
				write_status(LOGSCR, LEVEL1|LISTITEM|CONTINUE|PARTIAL,
					"%s", buf);
			}
		}

		if (get_trace_level() > 4) {
			write_status(LOGSCR, LEVEL1|LISTITEM|CONTINUE|PARTIAL,
				"%s", buf);
		}

		for (i = 1; i <= FD_NUMPART && feof(pp) == 0 &&
				fgets(buf, sizeof (buf), pp) != NULL; i++) {

			if (get_trace_level() > 4) {
				write_status(LOGSCR, LEVEL1|LISTITEM|CONTINUE|PARTIAL,
					"%s", buf);
			}

			/*
			 * set nsect, nhead, and onecyl from the main disk
			 * geometry structure (nsect/nhead/onecyl/hbacyl)
			 */
			part_geom_nsect(dp, i) = disk_geom_nsect(dp);
			part_geom_nhead(dp, i) = disk_geom_nhead(dp);
			part_geom_onecyl(dp, i) = disk_geom_onecyl(dp);
			part_geom_hbacyl(dp, i) = disk_geom_hbacyl(dp);

			if (sscanf(buf, "%d %d %d %d %d %d %d %d %d %d",
					&id, &act, &d, &d, &d, &d, &d, &d,
					&rsect, &tsect) != 10) {
				(void) pclose(pp);
				return (-1);
			}

			/*
			 * set the partition type and active state (id/active)
			 * and initialize the original partition number
			 */
			part_id(dp, i) = id;
			part_active_set(dp, i, act);
			part_orig_partnum(dp, i) = origpart++;

			/*
			 * set the relative offset sector and total cyl/sector
			 * (rsect/tsect/tcyl) on used partitions only
			 */
			if (part_id(dp, i) != UNUSED) {
				part_geom_rsect(dp, i) = rsect;
				part_geom_tsect(dp, i) = tsect;
				part_geom_tcyl(dp, i) =
						blocks_to_cyls(dp, tsect);
			}

			/*
			 * set the first data cylinder, last data cylinder/sect
			 * (firstcyl/lcyl/lsect)
			 */
			if (id == SUNIXOS) {
				/* setup the default alternate cylinder value */
				numacyl = NUMALTCYL;

				/*
				 * on i386 systems only, check the alternate
				 * cylinder value on the drive We are using the
				 * DKIOCGGEOM call here, because we are
				 * specifically looking for Solaris partitions
				 * with labels. This is critical because 2.1
				 * labels have an acyl value of '0', while 2.4
				 * and later have an acyl value of '2' (or more)
				 * Note that the dkg_acyl value cannot be relied
				 * upon for 2.1 labels, so the acyl value is
				 * calculated from pcyl - ncyl, which is
				 * considered to be reliable.
				 */
				if (streq(get_default_inst(), "i386")
						&& (fd = open(device,
							O_RDWR | O_NDELAY))
							>= 0) {
					if (ioctl(fd, DKIOCGGEOM, &dkg) == 0) {
						numacyl = dkg.dkg_pcyl -
								dkg.dkg_ncyl;
					} else {
						sdisk_state_set(dp,
							SF_NOSLABEL);
					}

					(void) close(fd);
				}

				part_geom_firstcyl(dp, i) = 1;
				part_geom_lcyl(dp, i) =
					part_geom_tcyl(dp, i) - numacyl;
				part_geom_lsect(dp, i) =
					cyls_to_blocks(dp,
						part_geom_lcyl(dp, i));

				if (part_geom_lcyl(dp, i) >
						part_geom_firstcyl(dp, i))
					sdisk_geom_set(dp,
							part_geom_addr(dp, i));
			} else {
				part_geom_firstcyl(dp, i) = 0;
				part_geom_lcyl(dp, i) = part_geom_tcyl(dp, i);
				part_geom_lsect(dp, i) = part_geom_tsect(dp, i);
			}

			part_geom_dcyl(dp, i) = part_geom_lcyl(dp, i) -
					part_geom_firstcyl(dp, i);
			part_geom_dsect(dp, i) = cyls_to_blocks(dp,
					part_geom_dcyl(dp, i));

			if (part_geom_dcyl(dp, i) < 0)
				part_geom_dcyl(dp, i) = 0;

			if (part_geom_dsect(dp, i) < 0)
				part_geom_dsect(dp, i) = 0;

			if (part_geom_lcyl(dp, i) < 0)
				part_geom_lcyl(dp, i) = 0;

			if (part_geom_lsect(dp, i) < 0)
				part_geom_lsect(dp, i) = 0;

			if (part_geom_firstcyl(dp, i) > part_geom_tcyl(dp, i))
				part_geom_firstcyl(dp, i) = 0;
		}

		(void) pclose(pp);

		/*
		 * finish initializing the orig_partnum and geometry
		 * structure defaults for partitions which where not
		 * loaded (remember that the -W output only prints as
		 * many entries as there are defined partitions)
		 */
		for (; i <= FD_NUMPART; i++) {
			part_orig_partnum(dp, i) = origpart++;
			part_geom_nsect(dp, i) = disk_geom_nsect(dp);
			part_geom_nhead(dp, i) = disk_geom_nhead(dp);
			part_geom_onecyl(dp, i) = disk_geom_onecyl(dp);
			part_geom_hbacyl(dp, i) = disk_geom_hbacyl(dp);
		}
	}

	_sort_fdisk_input(dp);

	/*
	 * after sorting the partition table, initialize the sdisk
	 * geometry pointer if there is a Solaris partition. On PowerPC,
	 * if there is no Solaris partition then mark the disk as
	 * having no physical geometry and return it as unusable
	 */
	if ((pid = get_solaris_part(dp, CFG_CURRENT)) > 0)
		sdisk_geom_set(dp, part_geom_addr(dp, pid));
	else if (streq(get_default_inst(), "ppc")) {
		disk_state_set(dp, DF_NOPGEOM);
		return (1);
	}

	return (0);
}

/*
 * _install_default_slabel()
 *	Call format on the drive and slap whatever format thinks is the
 *	default label for the drive onto the drive. This will initialize
 *	the controller to the correct geometry.
 * Parameters:
 *	dp	- non-NULL disk structure pointer with valid name field
 * Return:
 *	 0	- default label installed successfully
 *	-1	- default label installation failed
 * Status:
 *	private
 */
static int
_install_default_slabel(Disk_t * dp)
{
	static char	tmpfile[MAXNAMELEN] = "";
	FILE		*fp;
	char		cmd[126];

	if (get_install_debug() > 0) {
		write_status(LOGSCR, LEVEL0,
			INSTALL_DEFAULT_VTOC, disk_name(dp));
		return (0);
	}

	if (access("/usr/sbin/format", X_OK) != 0)
		return (-1);

	if (tmpfile[0] == '\0') {
		(void) tmpnam(tmpfile);
		if ((fp = fopen(tmpfile, "w")) == NULL) {
			tmpfile[0] = '\0';
			return (-1);
		}

		(void) fprintf(fp, "l\n");
		(void) fprintf(fp, "quit\n");
		(void) fclose(fp);
	}

	if (disk_fdisk_exists(dp)) {
		(void) sprintf(cmd, "/usr/sbin/format -s -f %s \
			/dev/rdsk/%sp0 >/dev/null 2>&1",
			tmpfile, disk_name(dp));
	} else {
		(void) sprintf(cmd, "/usr/sbin/format -s -f %s \
			/dev/rdsk/%ss2 >/dev/null 2>&1",
			tmpfile, disk_name(dp));
	}

	if (system(cmd) != 0)
		return (-1);

	return (0);
}
