#ifndef lint
#pragma ident "@(#)disk_profile.c 1.14 95/04/17"
#endif
/*
 * Copyright (c) 1994-1995 Sun Microsystems, Inc.  All Rights Reserved. Sun
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
#include "disk_lib.h"
#include "ibe_lib.h"

/* Public Function Prototypes */

int		configure_dfltmnts(Profile *);
int		configure_sdisk(Profile *);

/* Private Function Prototypes */

static int	_app_config_slice(Label_t, Storage *);
static int	_app_set_slice(char *, int, int, char *, char *, int, int);
static char *	_app_fit_slice(Disk_t *, int, char *);
static int	_app_find_slice(Disk_t *, char *);
static char *	_app_slice_all_disk(char *, char *);
static char *	_app_slice_auto_swap(void);
static int	_app_compare(Storage *, Storage *);
static void	_app_add_comparitor(Storage **, Storage **);

/* ******************************************************************** */
/* 			PUBLIC FUNCTIONS				*/
/* ******************************************************************** */
/*
 * configure_dfltmnts()
 *	Given the filesys record list and the current partitioning, the
 *	default mountlist used in sizing rollup calculations is updated
 *	accordingly.
 * Parameters:
 *	prop	  - pointer to profile structure
 * Return:
 *	D_OK	  - default mounts configured successfully
 *	D_BADARG  - invalid argument
 *	D_FAILED  - internal error
 * Status:
 *	public
 */
int
configure_dfltmnts(Profile *prop)
{
	Defmnt_t	**def_ml;
	Defmnt_t	**dfltp;
	Defmnt_t	*def_me;
	Storage		*fsp;

	/* validate arguments */
	if (prop == NULL) {
		write_notice(ERRMSG,
			"(configure_dfltmnts) %s",
			library_error_msg(D_BADARG));
		return (D_BADARG);
	}

	/* update the default mount list size entries */
	update_dfltmnt_list();

	/* get the list of mount points we care about */
	if ((def_ml = get_dfltmnt_list(NULL)) == NULL) {
		write_notice(ERRMSG, MSG0_INTERNAL_GET_DFLTMNT);
		return (D_FAILED);
	}

	if (get_trace_level() > 2)
		print_dfltmnt_list("Original", def_ml);

	/*
	 * for exisiting or explicit, ignore everything at first
	 */
	if (DISKPARTITIONING(prop) == CFG_NONE ||
			DISKPARTITIONING(prop) == CFG_EXIST) {
		if (get_trace_level() > 1)
			write_status(LOGSCR, LEVEL1|LISTITEM,
				MSG0_DFLTMNT_CLEAR);

		for (dfltp = def_ml; *dfltp; dfltp++) {
			def_me = *dfltp;
			def_me->status = DFLT_IGNORE;
		}
	}

	/*
	 * Now make sure that explicit definitions are marked
	 */
	for (dfltp = def_ml; *dfltp; dfltp++) {
		def_me = *dfltp;
		if (DISKPARTITIONING(prop) == CFG_EXIST) {
			/*
			 * Need to see if we can locate the given mntpt
			 * on some disk, if we can, then we will mark
			 * it as selected so that we can make sure all
			 * is OK later.
			 */
			if (find_mnt_pnt(NULL, NULL,
					def_me->name, NULL, CFG_EXIST)) {
				if (get_trace_level() > 1)
					write_status(LOGSCR, LEVEL1|LISTITEM,
						MSG1_DFLTMNT_FORCE_SELECT,
						def_me->name);
				def_me->status = DFLT_SELECT;
			}
		}

		WALK_LIST(fsp, DISKFILESYS(prop)) {
			if (streq(fsp->name, def_me->name)) {
				if (strcmp(fsp->size, "0") == 0) {
					/*
					 * roll up space for file systems with
					 * explicit '0' sizes
					 */
					if (get_trace_level() > 1)
						write_status(LOGSCR,
						    LEVEL1|LISTITEM,
						    MSG1_DFLTMNT_FORCE_IGNORE,
						    def_me->name);
					def_me->status = DFLT_IGNORE;
				} else {
					if (get_trace_level() > 1)
						write_status(LOGSCR,
						    LEVEL1|LISTITEM,
						    MSG1_DFLTMNT_FORCE_SELECT,
						    def_me->name);
					def_me->status = DFLT_SELECT;
				}
				break;
			} else {
				/*
				 * default file systems which don't have filesys
				 * records and don't require disk space should be
				 * rolled up into their ancestors
				 */
				if (def_me->name[0] == '/' &&
						def_me->size == 0 &&
						def_me->expansion <= 0)
					def_me->status = DFLT_IGNORE;
			}
		}

		/*
		 * if there were no filesys records, all default
		 * file systems which don't accommodate any software
		 * should be rolled up into their ancestors
		 */
		if (DISKFILESYS(prop) == NULL &&
				def_me->name[0] == '/' &&
				def_me->size == 0 &&
				def_me->expansion <= 0)
			def_me->status = DFLT_IGNORE;
	}

	if (get_trace_level() > 2)
		print_dfltmnt_list("Modified (Before Set)", def_ml);

	/* update swap resource for explicit set */
	if (TOTALSWAP(prop) >= 0) {
		for (dfltp = def_ml; *dfltp; dfltp++) {
			def_me = *dfltp;
			if (strcmp(def_me->name, SWAP) == 0) {
				def_me->expansion =
					mb_to_sectors(TOTALSWAP(prop));
				break;
			}
		}
	}

	/* Make sure that the disk library is aware of the new status */
	if (set_dfltmnt_list(def_ml) != D_OK) {
		write_notice(ERRMSG, MSG0_INTERNAL_SET_DFLTMNT);
		return (D_FAILED);
	}

	if (get_trace_level() > 2) {
		if ((def_ml = get_dfltmnt_list(def_ml)) != NULL)
			print_dfltmnt_list("Modified (After Set)", def_ml);
	}

	if ((def_ml = free_dfltmnt_list(def_ml)) != NULL) {
		write_notice(ERRMSG, MSG0_INTERNAL_FREE_DFLTMNT);
		return (D_FAILED);
	}

	return (D_OK);
}

/*
 * configure_sdisk()
 *	Configure the disk slices according to the profile specification.
 * Parameters:
 *	prop	- pointer to profile structure
 * Return:
 *	D_OK	  - configuration successful
 *	D_BADARG  - invalid argument
 *	D_FAILED  - internal failure
 * Status:
 *	public
 */
int
configure_sdisk(Profile *prop)
{
	Storage		*fsf;
	Storage 	*new = NULL;
	Storage		**fsp, **comp, **index;
	Disk_t		*dp;
	int		status = D_OK;
	int		s;

	/* validate arguments */
	if (prop == NULL) {
		write_notice(ERRMSG,
			"(configure_sdisk) %s",
			library_error_msg(D_BADARG));
		return (D_BADARG);
	}

	/*
	 * reset the slice states for explicit and default
	 * partitioning. existing is the current state, so
	 * no reset has to be done in this case
	 */
	WALK_DISK_LIST(dp) {
		if (disk_not_selected(dp))
			continue;

		if (DISKPARTITIONING(prop) != CFG_EXIST) {
			/*
			 * initialize the drive state for non-EXISTING
			 * partitioning
			 */
			if (sdisk_config(dp, NULL, CFG_NONE) != D_OK) {
				write_notice(ERRMSG,
					MSG1_INTERNAL_DISK_RESET,
					disk_name(dp));
				return (D_FAILED);
			}
		} else {
			/*
			 * Check all default mount file system in the existing
			 * configuration for forced start cylinder alignment.
			 * If they have been aligned, and are not default mount
			 * file systems, exit in error; otherwise, print
			 * a warning that they have be forcably aligned. Also
			 * mark all non-zero sized slices as "accessed"
			 */
			WALK_SLICES(s) {
				if (slice_locked(dp, s))
					continue;

				if (slice_aligned(dp, s)) {
					if (get_dfltmnt_ent(NULL,
						slice_mntpnt(dp,
							s)) == D_OK) {
						write_notice(WARNMSG,
							MSG2_SLICE_REALIGNED,
							make_slice_name(
							    disk_name(dp), s),
							slice_mntpnt(dp, s));
					} else {
						write_notice(ERRMSG,
						    MSG2_SLICE_ALIGN_REQUIRED,
						    make_slice_name(
							disk_name(dp), s),
						    slice_mntpnt(dp, s));
						return (D_FAILED);
					}
				}

				/*
				 * for existing partitioning, turn the preserve
				 * on for all slices which are not default mount
				 * points and which are notb "/export" or
				 * "/export/<*>"
				 */
				if (slice_mntpnt_exists(dp, s) &&
						strneq(slice_mntpnt(dp, s),
							OVERLAP) &&
						(strncmp(slice_mntpnt(dp, s),
							"/export/", 8) == 0 ||
						    streq(slice_mntpnt(dp, s),
							"/export") ||
						    get_dfltmnt_ent(NULL,
						    slice_mntpnt(dp, s)) !=
							D_OK)) {
					write_status(LOGSCR, LEVEL1|LISTITEM,
						MSG2_SLICE_PRESERVE,
						make_slice_name(disk_name(dp),
							s),
						slice_mntpnt(dp, s));
					if (set_slice_preserve(dp,
						    s, PRES_YES) != D_OK) {
						write_notice(ERRMSG,
						    MSG1_SLICE_PRESERVE_FAILED,
						    make_slice_name(
							    disk_name(dp), s));
						return (D_FAILED);
					}
				} else {
					write_status(LOGSCR, LEVEL1|LISTITEM,
						MSG2_SLICE_EXISTING,
						make_slice_name(disk_name(dp),
							s),
						slice_mntpnt_exists(dp, s) ?
							slice_mntpnt(dp, s) :
							MSG0_STD_UNNAMED);
					slice_explicit_on(dp, s);
				}

				if (slice_size(dp, s) > 0) {
					(void) slice_access(
						make_slice_name(disk_name(dp),
							s), 1);
				}
			}
		}
	}

	/*
	 * in order to ensure that !any/free filesys records do not have
	 * their slices allocated to other "any" filesys records, make sure
	 * to reserve these records; this is only done for !any/free filesys
	 * records to avoid confusing appearance of the disk state in the
	 * event of a failure
	 */
	WALK_LIST(fsf, DISKFILESYS(prop)) {
		if (is_slice_name(fsf->dev) &&
			    streq(fsf->size, "free") &&
			    (dp = find_disk(fsf->dev)) != NULL) {
			/*
			 * set the slice name to "/reserved", and reset
			 * the start cylinder and size (in case there is
			 * a default geometry, such as slice '2');
			 * remember that "free" is only allowed with
			 * default and explicit partitioning, so we don't
			 * have to worry about resetting the size of an
			 * existing partitioning slice
			 */
			(void) set_slice_mnt(dp,
				atoi(strrchr(fsf->dev, 's') + 1),
				"/reserved", NULL);
			(void) set_slice_geom(dp,
				atoi(strrchr(fsf->dev, 's') + 1),
				disk_geom_firstcyl(dp), 0);
		}
	}

	fsp = &DISKFILESYS(prop);

	/*
	 * sort the filesys chain in order based on _app_compare()
	 */
	while (*fsp != NULL) {
		/* check for last */
		if ((*fsp)->next == NULL) {
			comp = fsp;
		} else {
			/*
			 * there are at least two records left;
			 * find the lowest
			 */
			comp = fsp;
			for (index = &(*fsp)->next;
						*index;
						index = &(*index)->next) {
				if (_app_compare(*comp, *index) < 0)
					comp = index;
			}
		}

		_app_add_comparitor(comp, &new);
	}

	*fsp = new;

	/*
	 * now process the entries in the order they appear in in
	 * the chain
	 */
	WALK_LIST(fsf, *fsp) {
		if ((status = _app_config_slice(DISKPARTITIONING(prop),
				fsf)) != D_OK) {
			return (status);
		}
	}

	/*
	 * finish layout for default partitioning
	 */
	if (DISKPARTITIONING(prop) == CFG_DEFAULT) {
		write_status(LOGSCR, LEVEL1|LISTITEM,
			MSG0_DEFAULT_CONFIGURE_ALL);
		/*
		 * autolayout requires all disks be committed before call
		 */
		if (commit_disks_config() != D_OK)
			return (D_FAILED);

		status = sdisk_default_all();
	}

	return (status);
}

/* ******************************************************************** */
/* 				PRIVATE FUNCTIONS			*/
/* ******************************************************************** */
/*
 * _app_config_slice()
 *	Configure the specified slice (or slices) according to
 *	a specific filesys record.
 * Parameters:
 *	part_opt - partitioning specifier (e.g. "existing", "default", etc)
 *	fsp	 - pointer to filesys record structure
 * Return:
 *	D_OK	 - successful configuration
 *	D_BADARG - invalid argument
 *	D_FAILED - configuration failed
 *	D_NOFIT  - the slice configured does not fit
 *	D_OVER   - the slice configured overlaps
 * Status:
 *	private
 */
static int
_app_config_slice(Label_t part_opt, Storage *fsp)
{
	int 		status;
	int		explicit = 0;
	Disk_t		*dp;
	char		*cp;
	char		*device = NULL;
	int		firstavail;
	int		start = GEOM_IGNORE;
	int		size = GEOM_IGNORE;
	int		n;

	/* validate parameters */
	if (fsp == NULL) {
		write_notice(ERRMSG,
			"(_app_config_slice) %s",
			library_error_msg(D_BADARG));
		return (D_BADARG);
	}

	/* skip entries which were explicitly set to size '0' */
	if (ci_streq(fsp->size, "0"))
		return (D_OK);

	write_status(LOGSCR, LEVEL1|LISTITEM,
		MSG2_SLICE_CONFIGURE,
		fsp->name,
		fsp->dev);

	/* only size of "existing" allowed with existing partitioning */
	if (part_opt == CFG_EXIST && ci_strneq(fsp->size, "existing")) {
		write_notice(ERRMSG, MSG0_EXISTING_FS_SIZE_INVALID);
		return (D_FAILED);
	}

	/*
	 * get the size and, if applicable, the starting cylinder; the
	 * device may be set as a side effect of searching for the size
	 */

	/*
	 * ignored slices (original geometry)
	 */
	if (streq(fsp->name, "ignore")) {
		start = GEOM_ORIG;
		size = GEOM_ORIG;
		explicit = 1;
	}
	/*
	 * explicit size specified in MB
	 */
	else if (is_allnums(fsp->size)) {
		start = GEOM_IGNORE;
		size = mb_to_sectors(atoi(fsp->size));
		explicit = 1;
	}
	/*
	 * "all" size
	 */
	else if (ci_streq(fsp->size, "all")) {
		device = _app_slice_all_disk(fsp->dev, fsp->name);
		if (device == NULL || (dp = find_disk(device)) == NULL) {
			write_notice(ERRMSG,
				MSG1_NO_ALL_FREE_DISK,
				fsp->dev);
			return (D_FAILED);
		} else if (get_trace_level() > 0) {
			write_status(LOGSCR, LEVEL1|LISTITEM,
				"Device for \"all\" for \"%s\" (%s)",
				fsp->name,
				device == NULL ? "NONE" : device);
		}

		start = GEOM_IGNORE;
		size = usable_sdisk_blks(dp);
		explicit = 0;
	}
	/*
	 * explicit size specified in <cyl>:<cyl> format
	 */
	else if ((cp = strchr(fsp->size, ':')) != NULL) {
		*cp++ = '\0';
		if ((dp = find_disk(fsp->dev)) == NULL) {
			write_notice(ERRMSG,
				MSG1_DISK_INVALID,
				fsp->dev);
			return (D_FAILED);
		}

		start = atoi(fsp->size);
		size = cyls_to_blocks(dp, atoi(cp));
		explicit = 1;
		/*
		 * validate the starting cylinder is a legal value
		 * (i.e. is in the data segment of the disk and
		 * doesn't overlap any locked slices); this cannot
		 * be done by the parser because rootdisk is not
		 * expanded at that time so the disk may not be known
		 */
		if (start > sdisk_geom_lcyl(dp)) {
			write_notice(ERRMSG,
				MSG1_START_CYL_EXCEEDS_DISK,
				start);
			return (D_FAILED);
		}

		firstavail = sdisk_geom_firstcyl(dp);
		/*
		 * if this is a system which supports an alternate
		 * sector slice, then advance the allowable
		 * starting cylinder beyond it if it is at the
		 * front of the disk
		 */
		 if (ALT_SLICE < numparts && 
				slice_size(dp, ALT_SLICE) > 0 &&
				slice_start(dp, ALT_SLICE) == 1) {
			firstavail = slice_start(dp, ALT_SLICE) +
				blocks_to_cyls(dp,
					slice_size(dp, ALT_SLICE));
		}

		if (start < firstavail) {
			write_notice(ERRMSG,
				MSG2_START_CYL_INVALID,
				start,
				firstavail);
			return (D_FAILED);
		}
	}
	/*
	 * "free" size
	 */
	else if (ci_streq(fsp->size, "free")) {
		start = GEOM_IGNORE;
		size = GEOM_REST;
		explicit = 0;
	}
	/*
	 * "existing" size
	 */
	else if (ci_streq(fsp->size, "existing")) {
		start = GEOM_ORIG;
		size = GEOM_ORIG;
		explicit = 1;
	}
	/*
	 * "auto" size
	 */
	else if (ci_streq(fsp->size, "auto")) {
		if (streq(fsp->name, SWAP)) {
			if (ci_streq(fsp->dev, "any"))
				device = _app_slice_auto_swap();
			else
				device = fsp->dev;

			dp = find_disk(device);
			size = get_default_fs_size(fsp->name, dp, DONTROLLUP);
		} else {
			/*
			 * auto sized file system should guarantee default fs
			 * overhead, even if we are doing it with explicit
			 * partitioning which, for other file systems, does
			 * not require the default free space
			 */
			if ((n = percent_free_space()) < DEFAULT_FS_FREE)
				sw_lib_init(NULL, NULL, DEFAULT_FS_FREE);

			if (ci_streq(fsp->dev, "any"))
				dp = NULL;
			else
				dp = find_disk(fsp->dev);

			/*
			 * attempt to rollup the size for this file system;
			 * see if it fits as a whole; if not, see if it fits
			 * by itself (DONTROLLUP); for device "any", the
			 * ROLLUP size is attempted across all drives before
			 * we resort back to DONTROLLUP.
			 */
			size = get_default_fs_size(fsp->name, dp, ROLLUP);

			if (_app_fit_slice(dp, size, fsp->name) == NULL)
				size = get_default_fs_size(fsp->name,
					NULL, DONTROLLUP);

			/* reset the default free space */
			if (n == 0)
				sw_lib_init(NULL, NULL, NO_EXTRA_SPACE);
			else
				sw_lib_init(NULL, NULL, n);
		}

		if (get_trace_level() > 1) {
			write_status(LOGSCR, LEVEL1|LISTITEM,
				MSG2_TRACE_AUTO_SIZE,
				size,
				sectors_to_mb(size));
		}

		if (size == 0) {
			write_notice(WARNMSG,
				MSG1_DEFAULT_SIZE_INVALID,
				fsp->name);
			return (D_FAILED);
		}
	}

	/*
	 * '0' size slices are not configured
	 */
	if (size == 0)
		return (D_OK);

	/*
	 * if the device was not already set while determining the size,
	 * make sure it is set
	 */
	if (device == NULL) {
		if (ci_streq(fsp->dev, "any")) {
			/*
			 * try to find a disk that has an open slice which can
			 * hold the required storage
			 */
			if ((device = _app_fit_slice(NULL,
					size, fsp->name)) == NULL) {
				write_notice(ERRMSG,
					MSG2_SLICE_SIZE_NOT_AVAIL,
					sectors_to_mb(size),
					(*fsp->name == '\0' ?
					    MSG0_STD_UNNAMED : fsp->name));
				return (D_FAILED);
			}

			if (get_trace_level() > 1) {
				write_status(LOGSCR, LEVEL1|LISTITEM,
					MSG2_SLICE_ANY_BECOMES,
					fsp->name,
					device);
			}
		} else
			device = fsp->dev;
	}

	/*
	 * try to set the slice according to the size and disk specifications
	 */
	if ((status = _app_set_slice(device, start, size,
			fsp->name, fsp->mntopts, explicit,
			fsp->preserve)) != D_OK)
		return (status);

	return (D_OK);
}

/*
 * _app_set_slice()
 *	Configure the 'start', 'size', 'mnt', and 'opts' of a slice. This
 *	routine handles both explicit and non-explicit specifications.
 *
 *	NOTE:	this routine assumes that if there is a failure, then there
 *		will be an exit imminently
 *
 *	NOTE:	this routine should always report a message if it fails
 *
 * Parameters:
 *	device	- device name (e.g. c0t0d0)
 *	start	- starting cylinder specifier. Only consulted
 *		  when 'size' is an explicit value
 *		  (Valid values: GEOM_IGNORE, or a cyl number)
 *	size	- size of slice in sectors.
 *		  (Valid values: ###	 - size in sectors
 *				 ###:### - explicit start cyl and
 *					   size in sectors
 *	mnt	- pointer to mount point name (NULL if ignored)
 *	opts	- pointer to mount options (NULL if ignored)
 *	explicit - indicate if sizing is explicit (non-modifiable
 *		  by the free space routine)
 *	preserve - indicate if slice should be preserved
 * Return:
 *	D_OK		- set successful
 *	D_NODISK	-
 *	D_FAILED	- set failed
 *	D_NOTSELECT	- disk not selected
 * Status:
 *	private
 */
static int
_app_set_slice(char *device, int start, int size,
			char *mnt, char *opts, int explicit, int preserve)
{
	Disk_t		*dp;
	char		oldname[MAXMNTLEN];
	int		status = D_OK;
	int		slice;
	int		*olp;
	int		n, i;

	if ((dp = find_disk(device)) == NULL) {
		write_notice(ERRMSG,
			"(_app_set_slice) %s",
			library_error_msg(D_NODISK));
		return (D_FAILED);
	}

	if (disk_not_selected(dp)) {
		write_notice(ERRMSG,
			"(_app_set_slice) %s",
			library_error_msg(D_NOTSELECT));
		return (D_NOTSELECT);
	}

	slice = atoi(strrchr(device, 's') + 1);
	if (slice < 0 || slice > LAST_STDSLICE) {
		write_notice(ERRMSG,
			"(_app_set_slice) %s",
			library_error_msg(D_NODISK));
		return (D_NODISK);
	}

	/*
	 * save the current mount point name so you can restore
	 * later if the set is not totally successful
	 */
	(void) strcpy(oldname, slice_mntpnt(dp, slice));

	/*
	 * set the starting cylinder sticky bit if the
	 * starting cylinder is specified
	 */
	if (start != GEOM_IGNORE)
		slice_stuck_on(dp, slice);

	/*
	 * set the mount point name for non-ignored slices
	 */
	if (strneq(mnt, "ignore") &&
			(status = set_slice_mnt(dp,
				slice, mnt, opts)) != D_OK) {
		write_notice(ERRMSG, "%s (%s)",
			library_error_msg(status),
			make_slice_name(disk_name(dp), slice));
	}

	/*
	 * set the geometry
	 */
	if (status == D_OK &&
			(status = set_slice_geom(dp,
				slice, start, size)) != D_OK) {
		write_notice(ERRMSG,
			MSG1_SLICE_GEOM_SET_FAILED,
			make_slice_name(disk_name(dp), slice));

		/*
		 * if this was an explicit set, make sure the failure
		 * wasn't caused by two explicitly set overlapping
		 * named mount points
		 */
		if (start != GEOM_IGNORE) {
			if ((n = slice_overlaps(dp, -1, start,
						size, &olp)) != 0) {
				for (i = 0; i < n; i++) {
					if (slice_stuck(dp, olp[i])) {
						status = D_OVER;
						break;
					}
				}
			}

			write_notice(ERRMSG,
				"%s (%s)",
				library_error_msg(status),
				make_slice_name(disk_name(dp), slice));
		}
	}

	/*
	 * set the ignore status bit for ignored slices now that the
	 * geometry has been set
	 */
	if (status == D_OK && streq(mnt, "ignore")) {
		write_status(LOGSCR, LEVEL1|LISTITEM,
			MSG1_SLICE_IGNORE,
			make_slice_name(disk_name(dp), slice));
		if (set_slice_ignore(dp, slice, IGNORE_YES) != D_OK) {
			write_notice(ERRMSG,
				MSG1_SLICE_IGNORE_FAILED,
				make_slice_name(disk_name(dp), slice));
			status = D_FAILED;
		}
	}

	/*
	 * if there is no preserve for this slice, turn it off (could
	 * be on from partitioning existing logic)
	 */
	if (status == D_OK && preserve == 0 &&
			set_slice_preserve(dp, slice, PRES_NO) != D_OK) {
		write_notice(ERRMSG,
			MSG1_SLICE_PRESERVE_OFF_FAILED,
			make_slice_name(disk_name(dp), slice));
		status = D_FAILED;
	}

	/*
	 * if the slice is to be preserved and it isn't already preserved
	 * set it
	 */
	if (status == D_OK && preserve == 1 &&
			slice_not_preserved(dp, slice)) {
		write_status(LOGSCR, LEVEL1|LISTITEM,
			MSG2_SLICE_PRESERVE,
			(slice_mntpnt_exists(dp, slice) ?
				slice_mntpnt(dp, slice) : MSG0_STD_UNNAMED),
				make_slice_name(disk_name(dp), slice));
		if (set_slice_preserve(dp, slice, PRES_YES) != D_OK) {
			write_notice(ERRMSG,
				MSG1_SLICE_PRESERVE_FAILED,
				make_slice_name(disk_name(dp), slice));
			status = D_FAILED;
		}
	}

	/*
	 * if everything has proceeded without error, add the slice to the
	 * list of accessed slices
	 */
	if (status == D_OK) {
		(void) slice_access(make_slice_name(disk_name(dp), slice), 1);
		if (explicit == 1)
			slice_explicit_on(dp, slice);
	} else {
		/*
		 * restore the original slice name to avoid disk printout
		 * confusion (SHOULD REALLY BE RESTORING THE WHOLE SLICE
		 * STATE)
		 */
		(void) strcpy(slice_mntpnt(dp, slice), oldname);
	}

	return (status);
}

/*
 * _app_slice_auto_swap()
 *	Find a disk which has sufficient contiguous space and available
 *	slices to hold swap.
 * Parameters:
 *	none
 * Return:
 *	char *	- pointer to local string containing either the specified
 *		  device
 *	NULL	- no device found
 * Status:
 *	private
 */
static char *
_app_slice_auto_swap(void)
{
	Disk_t	*dp;
	char	*device = NULL;
	int	slice;

	WALK_DISK_LIST(dp) {
		if (disk_not_selected(dp))
			continue;

		if ((slice = _app_find_slice(dp, SWAP)) < 0)
			continue;

		/*
		 * see if the default swap size fits using the current disk
		 * as a constraint
		 */
		if (sdisk_max_hole_size(dp) >= 
				get_default_fs_size(SWAP, dp, DONTROLLUP)) {
			device = make_slice_name(disk_name(dp), slice);
			break;
		}
	}

	return (device);
}

/*
 * _app_fit_slice()
 *	Find a disk with sufficient contiguous space and an available slice
 *	to hold 'size' sectors of data.
 * Parameters:
 *	disk	- disk structure pointer, if search is to be constrained to
 *		  a particular drive, or NULL if it is to span all selected
 *		  disks
 *	size	- size of data segment in sectors (must be greater than 0)
 *	mnt	- mount point name used to give slice preference to system
 *		  default mount points when possible
 * Return:
 *	char *	- non-NULL pointer to device name
 *	NULL	- no device fits
 * Status:
 *	private
 */
static char *
_app_fit_slice(Disk_t *disk, int size, char *mnt)
{
	Disk_t 	*dp;
	char	*device = NULL;
	int	slice;

	if (disk != NULL) {
		dp = disk;
		if (disk_selected(dp) &&
				(slice = _app_find_slice(dp, mnt)) >= 0 &&
				sdisk_max_hole_size(dp) >= size)
			device = make_slice_name(disk_name(dp), slice);
	} else {
		/*
		 * look for the first available disk with sufficient space
		 * and a free slice
		 */
		WALK_DISK_LIST(dp) {
			if (disk_selected(dp) &&
					(slice = _app_find_slice(dp,
						mnt)) >= 0 &&
					sdisk_max_hole_size(dp) >= size) {
				device = make_slice_name(disk_name(dp), slice);
				break;
			}
		}
	}

	return (device);
}

/*
 * _app_find_slice()
 *	Find an available slice for 'mnt'.
 *
 *	ALGORITHM:
 *	(1) if 'mnt' is a default mount point with a specific
 *	    default slice, then see if that slice is available
 *	(2) try slices which are not default for default mount
 *	    points
 *	(3) try any available slice
 *
 * Parameters:
 *	dp	- non-NULL disk structure pointer
 *	mnt	- mount point name to be placed
 * Return:
 *	# >= 0	 - available slice number
 *	D_BADARG - invalid argument
 *	D_FAILED - configuration failed
 * Status:
 *	private
 */
static int
_app_find_slice(Disk_t *dp, char *mnt)
{
	Defmnt_t	**mpp;
	Defmnt_t	def;
	int		i;
	int		j;

	/* validate parameters */
	if (dp == NULL || mnt == NULL)
		return (D_BADARG);

	/*
	 * for default mount points, if the preferred slice is not "wild"
	 * (-1), and the slice is not in use, then use it
	 */
	if (get_dfltmnt_ent(&def, mnt) == D_OK && def.slice >= 0) {
		if (slice_size(dp, def.slice) == 0 &&
				slice_mntpnt_not_exists(dp, def.slice) &&
				slice_not_ignored(dp, def.slice) &&
				slice_not_stuck(dp, def.slice) &&
				slice_not_locked(dp, def.slice)) {
			return (def.slice);
		}
	}

	/* try slices which are not default for default mount points */
	if ((mpp = get_dfltmnt_list(NULL)) == NULL) {
		write_notice(ERRMSG, MSG0_INTERNAL_GET_DFLTMNT);
		return (D_FAILED);
	}

	WALK_SLICES_STD(i) {
		for (j = 0; mpp[j]; j++) {
			if (mpp[j]->slice == i)
				break;
		}

		if (mpp[j])
			continue;

		if (slice_size(dp, i) == 0 &&
				slice_mntpnt_not_exists(dp, i) &&
				slice_not_ignored(dp, i) &&
				slice_not_stuck(dp, i) &&
				slice_not_locked(dp, i)) {
			(void) free_dfltmnt_list(mpp);
			return (i);
		}
	}

	(void) free_dfltmnt_list(mpp);

	/* try any available slice, starting at the last slice */
	for (i = LAST_STDSLICE; i >= 0; i--) {
		if (slice_size(dp, i) == 0 &&
				slice_mntpnt_not_exists(dp, i) &&
				slice_not_ignored(dp, i) &&
				slice_not_stuck(dp, i) &&
				slice_not_locked(dp, i)) {
			return (i);
		}
	}

	if (get_trace_level() > 2) {
		write_status(SCR, LEVEL0,
			MSG1_SLICE_NOT_AVAIL, mnt);
	}

	return (D_FAILED);
}

/*
 * _app_slice_all_disk()
 *	Verify that a specified disk can be fully allocated. If 'any' is
 *	specified, then see if there is a fully available drive.
 * Parameters:
 *	dev	- device name (e.g. c12t3d2s2)
 *	mnt	- mount point name
 * Return:
 *	char *	- pointer to local string containing either the specified
 *		  device
 *	NULL	- no device found
 * Status:
 *	private
 */
static char *
_app_slice_all_disk(char *dev, char *mnt)
{
	Disk_t		*dp;
	int		slice;

	/* validate parameters */
	if (dev == NULL || mnt == NULL)
		return (NULL);

	if (streq(dev, "any")) {
		WALK_DISK_LIST(dp) {
			if (disk_not_selected(dp))
				continue;

			/*
			 * find a disk with all space free and at least one
			 * slice free
			 */
			if (sdisk_space_avail(dp) == usable_sdisk_blks(dp) &&
					(slice = _app_find_slice(dp, mnt)) >= 0) {
				return (make_slice_name(disk_name(dp), slice));
			}
		}

		write_notice(ERRMSG, MSG0_DISKS_NOT_FREE);
		return (NULL);
	} else {
		if ((dp = find_disk(dev)) == NULL) {
			write_notice(ERRMSG, MSG1_DISK_INVALID, dev);
			return (NULL);
		}

		/*
		 * make sure all space free and at least one slice free
		 */
		if (sdisk_space_avail(dp) == usable_sdisk_blks(dp) &&
				(slice = _app_find_slice(dp, mnt)) >= 0) {
			return (make_slice_name(disk_name(dp), slice));
		}

		write_notice(ERRMSG, MSG1_DISK_NOT_FREE, dev);
		return (NULL);
	}
}

/*
 * _app_compare()
 *	Compare the ordering of the device field between two filesys entries.
 *	Ordinal relationships are:
 *		!any	existing
 *		!any	all
 *		!any	explicit
 *		!any	auto
 *		any	all
 *		any	explicit
 *		any	auto
 *		!any	free
 *		any	free
 * Parameters:
 *	first	- pointer to first filesys entry device
 *		  (Valid values: "all", other)
 *	second	- pointer to second filesys entry device
 *		  (Valid values: "all", other)
 * Return:
 *	 0	- entries are the same
 *	 1	- first field should preceed second entry
 *	-1	- first field should follow second entry
 * Status:
 *	private
 */
static int
_app_compare(Storage *first, Storage *second)
{
	int	f;
	int	s;

	/* !any existing */
	if (strneq(first->dev, "any") && streq(first->size, "existing"))
		f = 0;

	if (strneq(second->dev, "any") && streq(second->size, "existing"))
		s = 0;

	/* !any all */
	if (strneq(first->dev, "any") && streq(first->size, "all"))
		f = 1;

	if (strneq(second->dev, "any") && streq(second->size, "all"))
		s = 1;

	/* !any explicit */
	if (strneq(first->dev, "any") &&
			strneq(first->size, "all") &&
			strneq(first->size, "auto") &&
			strneq(first->size, "free"))
		f = 2;

	if (strneq(second->dev, "any") &&
			strneq(second->size, "all") &&
			strneq(second->size, "auto") &&
			strneq(second->size, "free"))
		s = 2;

	/* !any auto */
	if (strneq(first->dev, "any") && streq(first->size, "auto"))
		f = 3;

	if (strneq(second->dev, "any") && streq(second->size, "auto"))
		s = 3;

	/* any all */
	if (streq(first->dev, "any") && streq(first->size, "all"))
		f = 4;

	if (streq(second->dev, "any") && streq(second->size, "all"))
		s = 4;

	/* any explicit */
	if (streq(first->dev, "any") &&
			strneq(first->size, "all") &&
			strneq(first->size, "auto") &&
			strneq(first->size, "free"))
		f = 5;

	if (streq(second->dev, "any") &&
			strneq(second->size, "all") &&
			strneq(second->size, "auto") &&
			strneq(second->size, "free"))
		s = 5;

	/* any auto */
	if (streq(first->dev, "any") && streq(first->size, "auto"))
		f = 6;

	if (streq(second->dev, "any") && streq(second->size, "auto"))
		s = 6;

	/* !any free */
	if (strneq(first->dev, "any") && streq(first->size, "free"))
		f = 7;

	if (strneq(second->dev, "any") && streq(second->size, "free"))
		s = 7;

	/* any free */
	if (streq(first->dev, "any") && streq(first->size, "free"))
		f = 8;

	if (streq(second->dev, "any") && streq(second->size, "free"))
		s = 8;

	if (f == s)
		return (0);

	if (f > s)
		return (-1);

	return (1);
}

/*
 * _app_add_comparitor()
 *	Move the filesys record **fsp from its current location in source
 *	linked list, to the end of the 'new' linked list.
 * Parameter:
 *	**fsp	- address of pointer to record to be moved
 *	**new	- address of pointer to head of new linked list
 * Return:
 *	none
 * Status:
 *	private
 */
static void
_app_add_comparitor(Storage **fsp, Storage **new)
{
	Storage 	*pp;
	Storage 	*ep;

	ep = *fsp;
	*fsp = (*fsp)->next;
	ep->next = NULL;

	if (*new == NULL)
		*new = ep;
	else {
		for (pp = *new; pp->next; pp = pp->next);
		pp->next = ep;
	}
}
