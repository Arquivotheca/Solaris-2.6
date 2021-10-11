#ifndef lint
#pragma ident   "@(#)svc_fs_space.c 1.13 96/10/10 SMI"
#endif
/*
 * Copyright (c) 1991-1996 Sun Microsystems, Inc.  All Rights Reserved.
 */
#include <ftw.h>
#include <stdlib.h>
#include "spmicommon_api.h"
#include "spmistore_api.h"
#include "spmisoft_lib.h"
#include "spmisvc_lib.h"

/* internal prototypes */

ulong	new_slice_size(ulong, ulong, int, ulong *, ulong *, ulong *);

/* private prototyptes */

static void	add_device_space(FSspace **);
static void	add_upg_fs_overhead(FSspace **);
static FSspace	**sort_space_fs(FSspace **, char **);
static int	upg_percent_free_space(char *);
static ulong	roundup(float);
static int	calc_devfs_overhead(void);
static int	calc_devfs_filesize(const char *, const struct stat *, int,
			struct FTW *);

/* local constants */

static int	dflt_percent_free = 5;
static int	total;

int	upg_fs_freespace[N_LOCAL_FS] = {
	/* root */		 5,
	/* usr */		 0,
	/* usr_own_fs */	 -1,
	/* opt */		 0,
	/* swap */		 0, /* not applicable */
	/* var */		 5,
	/* export/exec */	 5,
	/* export/swap */	 5,
	/* export/root */	 5,
	/* export/home */	 5,
	/* export */		 5
};

extern char *Pkgs_dir;

/* ******************************************************************** */
/*			PUBLIC SUPPORT FUNCTIONS			*/
/* ******************************************************************** */

/*
 * verify_fs_layout()
 *	Verify that the proposed software layout is sufficient to
 *	hold the selected software.
 * Parameters:
 * Return:
 * Status:
 *	public
 */
int
verify_fs_layout(FSspace **fs_list, int (*callback_proc)(void *, void *),
    void *callback_arg)
{
	int status, i;
	int sp_fail = 0;

	if ((status = calc_partition_size(fs_list, callback_proc,
	    callback_arg)) != SUCCESS)
		return (status);

	for (i = 0; fs_list[i]; i++) {
		if (fs_list[i]->fsp_flags & FS_IGNORE_ENTRY)
			continue;

		if ((fs_list[i]->fsp_flags & FS_USE_PROPOSED_SIZE &&
		    fs_list[i]->fsp_reqd_slice_size >
		    fs_list[i]->fsp_proposed_slice_size) ||
		    (!(fs_list[i]->fsp_flags & FS_USE_PROPOSED_SIZE) &&
		    fs_list[i]->fsp_reqd_slice_size >
		    fs_list[i]->fsp_cur_slice_size)) {
			fs_list[i]->fsp_flags |= FS_INSUFFICIENT_SPACE;
			sp_fail = 1;
		} else
			fs_list[i]->fsp_flags &= ~FS_INSUFFICIENT_SPACE;
	}

	if (sp_fail)
		return (SP_ERR_NOT_ENOUGH_SPACE);
	else
		return (SUCCESS);
}

/*
 * calc_partition_size()
 *	Calculate the partition sizes required to hold the selected
 *	software, given the proposed file system layout.
 * Parameters:
 * Return:
 * Status:
 *	public
 */
int
calc_partition_size(FSspace **sp, int (*callback_proc)(void *, void *),
    void *callback_arg)
{
	int	status, i, slice;
	Disk_t	*dp;
	char	*device;
	ulong	new_size;
	int	su;
	ulong	reqfree, su_blks, ufsoh_blks;

	if ((status = calc_sw_fs_usage(sp, callback_proc,
	    callback_arg)) != SUCCESS)
		return (status);

	if (is_upgrade()) {
		add_upg_fs_overhead(sp);

		set_units(D_KBYTE);
		for (i = 0; sp[i]; i++) {
			if (sp[i]->fsp_flags & FS_IGNORE_ENTRY)
				continue;

			if (sp[i]->fsp_fsi) {
				WALK_DISK_LIST(dp) {
					if (disk_not_okay(dp))
						continue;

					device =
					    sp[i]->fsp_fsi->fsi_device;

					if (strstr(device, disk_name(dp)) == 0)
						continue;

					slice = (int) atoi(device +
					    (strlen(device) - 1));
					sp[i]->fsp_cur_slice_size =
					    blocks2size(dp,
					    orig_slice_size(dp, slice), 1);
					break;
				}
				su = sp[i]->fsp_fsi->su_only;
			} else  {
				dp = NULL;
				su = 10;
			}

			if (dp == NULL)
				sp[i]->fsp_cur_slice_size = 0;

			new_size = new_slice_size(
			    sp[i]->fsp_reqd_contents_space +
			    sp[i]->fsp_reqd_free, 0, su, &reqfree,
			    &su_blks, &ufsoh_blks);
			fsp_set_field(sp[i], FSP_CONTENTS_SU_ONLY, su_blks);
			fsp_set_field(sp[i], FSP_CONTENTS_UFS_OVHD, ufsoh_blks);

			/*
			 *  If this is a file system which will not be
			 *  touched by the upgrade, set the required slice
			 *  size to the current slice size (unless it's
			 *  already smaller than the current slice size).
			 *  By doing that, we make sure that the file
			 *  system will not be flagged as having insufficient
			 *  space, even if it's full.
			 */
			if (!(sp[i]->fsp_flags & FS_HAS_PACKAGED_DATA) &&
			    sp[i]->fsp_reqd_slice_size >
			    sp[i]->fsp_cur_slice_size) {
				sp[i]->fsp_reqd_slice_size =
				    sp[i]->fsp_cur_slice_size;
			}

			/*
			 *  If this is a file system which corresponds to an
			 *  existing slice, and if the amount of space for
			 *  contents plus required-free is less than the
			 *  amount of available space on the slice, make sure
			 *  that the fsp_reqd_slice_size is greater than the
			 *  fsp_cur_slice_size.  Due to errors in the UFS
			 *  overhead calculation, this is not always
			 *  automatically the case, so increase the required
			 *  size until it is greater than the current size,
			 *  and make sure to round up the size to the next
			 *  physical boundary (such as a cylinder).
			 */
			if (sp[i]->fsp_cur_slice_size != 0) {
				if (sp[i]->fsp_reqd_contents_space +
				    sp[i]->fsp_reqd_free >
				    (ulong)(((100 - (float)su)/100) *
				    (float)(sp[i]->fsp_fsi->f_blocks))) {
					new_size = sp[i]->fsp_reqd_slice_size;
					while (sp[i]->fsp_cur_slice_size >=
					    new_size) {
						new_size++;
						new_size = size2blocks(dp,
						    new_size);
						new_size = blocks2size(dp,
						    new_size, 1);
					}
					fsp_set_field(sp[i],
					    FSP_CONTENTS_ERR_EXTRA,
					    new_size -
					    sp[i]->fsp_reqd_slice_size);
				}
			}
		}
	} else {
		return (FAILURE);	/* not supported for init install */
	}
	return (SUCCESS);
}

/*
 * percent_free_space()
 *	Retrieve the value of the dflt_percent_free global variable,
 *	which is a # >= 0, and represents the amount of space the
 *	software library was initialized to use as "desireable extra
 *	free space on any given file system".
 * Parameters:
 *	none
 * Return:
 *	# >= 0	- current value of dflt_percent_free global
 * Status:
 *	public
 */
int
percent_free_space(void)
{
	return (dflt_percent_free);
}

/*
 * set_percent_free_space()
 *	Set the value of the dflt_percent_free global.
 * Parameters:
 *	disk_space	- specifies the percent of free space which should be
 *			  used in calculating space requirements. To set this
 *			  to "0%" the argument "NO_EXTRA_SPACE" should be used.
 *			  Default is 5% free space.
 * Return:
 *	none
 * Status:
 *	public
 */
void
set_percent_free_space(int disk_space)
{
	if (disk_space != 0) {
		if (disk_space <= NO_EXTRA_SPACE)
			dflt_percent_free = 0;
		else
			dflt_percent_free = disk_space;
	}
}

/*
 * space_meter()
 *	Allocate a space table based on either the default mount points
 *	or the ones listed in in 'mplist'. Run the software tree and
 *	populate the table.
 * Parameters:
 *	mplist	 - array of mount points for which space is to be metered.
 *		   If this is NULL, the default mount point list will be used
 * Return:
 * 	NULL	 - invalid mount point list
 *	Space ** - pointer to allocated and initialized array of space
 *		   structures
 *		   *NOTE* Don't free this, because it gets reused.
 * Status:
 *	public
 */
FSspace **
space_meter(char **mplist)
{
	Module	*mod, *prodmod;
	Product	*prod;
	static	FSspace **new_sp = NULL;
	static	prev_null = 0;
	int	i, j;
	FSspace	**mtab;

	if (mplist != (char **)NULL && mplist[0] == (char *)NULL)
		mplist = NULL;

	if (!valid_mountp_list(mplist)) {
#ifdef DEBUG
		(void) printf(
			"DEBUG: space_meter(): Invalid mount point passed\n");
#endif
		return (NULL);
	}

	if ((mod = get_media_head()) == (Module *)NULL) {
#ifdef DEBUG
		(void) printf("DEBUG: space_meter(): media head NULL\n");
#endif
		return (NULL);
	}

	if (get_trace_level() > 5) {
		write_message(LOG, STATMSG, LEVEL0, "mplist in space_meter()");
		for (i = 0; mplist != NULL && mplist[i] != NULL; i++) {
			write_message(LOG, STATMSG, LEVEL1, "%s", mplist[i]);
		}
	}
	prodmod = mod->sub;
	prod = prodmod->info.prod;
	Pkgs_dir = prod->p_pkgdir;

	/* set up the space table */
	if (mplist == NULL || mplist == def_mnt_pnt) {
		if (prev_null == 1) {
			/* Reuse table. */
			sort_spacetab(new_sp);
			reset_stab(new_sp);
		} else {
			free_space_tab(new_sp);
			new_sp = load_def_spacetab(NULL);
		}
		prev_null = 1;
	} else {
		free_space_tab(new_sp);
		new_sp = load_defined_spacetab(mplist);
		prev_null = 0;
	}

	if (new_sp == NULL)
		return (NULL);

	if (is_upgrade()) {
		/*
		 * Only process 'real' filesystems
		 * Ignore all mountpoints except ones in the master spacetab.
		 */
		mtab = get_master_spacetab();
		for (i = 0; new_sp[i] != NULL; i++) {
			new_sp[i]->fsp_flags = FS_IGNORE_ENTRY;
			for (j = 0; mtab[j] != NULL; j++) {
				if (!strcmp(mtab[j]->fsp_mntpnt,
					    new_sp[i]->fsp_mntpnt)) {
					new_sp[i]->fsp_flags = 0;
					break;
				}
			}
		}
	}

	if (calc_sw_fs_usage(new_sp, NULL, NULL) != SUCCESS)
		return (NULL);

	/*
	 * add any necessary overhead to '/' to account for the devfs
	 * plumbing of the devices tree
	 */
	add_device_space(new_sp);

	if (is_upgrade()) {
		/*
		 * Clean up the FS_IGNORE_ENTRY flags we set earlier
		 */
		for (i = 0; new_sp[i] != NULL; i++) {
			new_sp[i]->fsp_flags &= ~FS_IGNORE_ENTRY;
		}
	}

	if (mplist != NULL)
		return (sort_space_fs(new_sp, mplist));

	return (sort_space_fs(new_sp, def_mnt_pnt));
}

/*
 * Function:	add_device_space
 * Description: If this is a disk simulation run, then add 1 MB
 *		for an average size requirement, and print a message
 *		notifying the user that in the real run, '/' may be
 *		larger depending on device requirements found. If
 *		this is an upgrade, then do not add anything extra
 *		because the space used has already been accounted for
 *		in the existing size data. If this is an initial
 *		install using live disks (even if it is an execution
 *		simulation), we will assume that the current size of
 *		/dev and /devices is an accurate estimate of what will
 *		actually be required during the install (i.e. if you
 *		are simulating execution on an earlier release of the
 *		OS than you will be booting for the real install, we
 *		assume there will be no significant size changes required
 *		of /dev and /devfs between the releases). The minimum
 *		amount added will always be 1 MB, thereby ensuring a
 *		larger number of systems will get conformity between
 *		their simulations and their live runs.
 * Scope:	private
 * Parameters:	sp	[**RO, *RO] (FSspace **)
 *			Space table used in the calculation.
 * Return:	none
 */
static void
add_device_space(FSspace **sp)
{
	int 	i;
	ulong	extra;

	/* validate parameter and make sure this isn't an upgrade */
	if (sp == (FSspace **)NULL || is_upgrade())
		return;

	for (i = 0; sp[i]; i++) {
		/* find the '/' entry in the space table */
		if (streq("/", sp[i]->fsp_mntpnt) &&
				!(sp[i]->fsp_flags & FS_IGNORE_ENTRY) &&
				sp[i]->fsp_reqd_contents_space > 0)
			break;
	}

	if (sp[i] != NULL) {
		if (GetSimulation(SIM_SYSDISK))
			extra = 0;
		else
			extra = calc_devfs_overhead();

		/* always guarantee a minimum of 1 MB */
		if (extra < mb_to_kb(1))
			extra = mb_to_kb(1);

		/* add the resulting space to the '/' space table entry */
		fsp_add_to_field(sp[i], FSP_CONTENTS_DEVFS, extra);
	}
}

/*
 * Function:	calc_devfs_overhead
 * Description:	Calculate the file system space required to hold the /dev
 *		and /devices directories. Return the size in kB.
 * Scope:	private
 * Parameters:	none
 * Return:	# >= 0	number of kB required
 */
static int
calc_devfs_overhead(void)
{
	total = 0;
	(void) nftw("/dev", calc_devfs_filesize, 64,
			FTW_MOUNT | FTW_PHYS | FTW_CHDIR);
	(void) nftw("/devices", calc_devfs_filesize, 64,
			FTW_MOUNT | FTW_PHYS | FTW_CHDIR);
	return (total);
}

/*
 * Function:	calc_devfs_filesize
 * Description:	Calculate the number of kB used by directory, symbolic link,
 *		or "normal file" and increment the global total count.
 * Scope:	private
 * Parameters:	path	[RO, *RO] (const char *)
 *		statp	[RO, *RO] (const struct stat *)
 *		ftype	[RO] (int)
 *		ftwp	[RO, *RO] (struct FTW)
 * Return:	always returns '0'
 * Globals:	total	augmented with current size value
 */
/*ARGSUSED0*/
static int
calc_devfs_filesize(const char *path, const struct stat *statp, int ftype,
		struct FTW *ftwp)
{
	/*
	 * we're only concerned with directory, symlinks, and files that are
	 * real; round byte size up to nearest 1024 byte frag boundary
	 */
	if (ftype == FTW_D || ftype == FTW_SL ||
			(ftype == FTW_F && statp->st_blocks > 0)) {
		total += (statp->st_size + 1023) / 1024;
	}

	return (0);
}

/*
 * add_upg_fs_overhead()
 * Calculate the upgrade overhead for each filesystem in the 'sp'
 * space table, and factor each of the 'fsp_reqd_contents_space'
 * and 'fsp_cts.contents_inodes_used' fields.
 *
 * Parameters:	sp	- space table to use in calculation
 * Return:	none
 */
static void
add_upg_fs_overhead(FSspace **sp)
{
	int 	i;
	int	oh;
	ulong	extra;

	/* parameter check */
	if (sp == (FSspace **)NULL)
		return;

	for (i = 0; sp[i]; i++) {
		if (sp[i]->fsp_flags & FS_IGNORE_ENTRY)
			continue;

		if (sp[i]->fsp_reqd_contents_space == 0)
			continue;

		oh = upg_percent_free_space(sp[i]->fsp_mntpnt);
		if (oh == 0)
			continue;
		oh += 100;
		extra = (sp[i]->fsp_reqd_contents_space * oh) / 100;
		fsp_add_to_field(sp[i], FSP_CONTENTS_REQD_FREE,
		    extra - sp[i]->fsp_reqd_contents_space);

		if (sp[i]->fsp_cts.contents_inodes_used == 0)
			continue;
		sp[i]->fsp_cts.contents_inodes_used =
		    (sp[i]->fsp_cts.contents_inodes_used * oh) / 100;
	}
}

/*
 * new_slice_size()
 *	Calculates the total size of a physical slice in 1024-blocks
 *	required to support a defined amount of "needed" usable space.
 *
 * Parameters:
 *	size    - number of blocks required to be available in file system
 *		  for actual contents, plus required free space.
 *	su_only	- % of blocks reserved for su-only use
 *
 * Algorithm:
 *
 * Here's the derivation of the computation below:
 *
 *    Assume:
 *	fsfree = required amount of not-root-only space that must be left
 *		 free on a particular file system, expressed as a fraction
 *		 (i.e., 10% free space means fsfree = .1)
 *	su	= percent of file system reserved for root-only access,
 *		 expressed as a fraction (10% free means su_only = .1).
 *	ufs_oh_var = variable part  of the ufs overhead (proportional to
 *		 size of slice).  Empirical analysis shows this to be
 *		 approximately 6.8%.  It is expressed as a fraction
 *		 (i.e., 10% free means ufs_oh_var = .10).
 *	ufs_oh_fixed = fixed part of the UFS overhead.  This has been
 *		 found to be about 700 1K blocks.
 *	nblks =  The total number of 1024 blocks in a disk slice.
 *	avail =  space required for actual contents
 *
 *	Here is the equation that calculates the amount of amount of
 *	available space:
 *
 *  avail = ((nblks *(1 - ufs_oh_var)) - ufs_oh_fixed)))(1 - su)(1 - fsfree)
 *
 *	Solving for nblks, we get:
 *
 *  nblks = ((avail/((1 - su)(1 - fsfree))) + ufs_fixed)/(1 - ufs_var)
 *
 * Return:
 *	# >= 0  - # of blocks generally available in new size
 * Status:
 *	public
 */
ulong
new_slice_size(ulong avail, ulong fsfree, int su_only, ulong *reqfree_blks,
	ulong *su_blks, ulong *ufsoh_blks)
{
	ulong accum, new;

	/* parameter check */
	if (su_only > 99)
		su_only = 0;
	if (fsfree > 99)
		fsfree = 0;

	accum = avail;

	/* add reqd_Free */
	new = roundup((float)accum / ((100 - (float)fsfree)/100));
	*reqfree_blks = new - accum;
	accum = new;

	/* add su_only */
	new = roundup((float)accum / ((100 - (float)su_only)/100));
	*su_blks = new - accum;
	accum = new;

	/* add ufs overhead */
	new = roundup(((float)accum + 700) / (1 - 0.068));
	*ufsoh_blks = new - accum;
	accum = new;

	return (accum);
}

/*
 * sort_space_fs()
 *	Sort the space table into the same order as 'mntlist'.
 * Parameters:
 *	space	- pointer to Space table to be sorted
 *	mntlist	- array of strings containing mountpoints
 * Return:
 *	FSspace **	- value of 'space' parameter
 * Status:
 *	public
 */
static FSspace **
sort_space_fs(FSspace ** space, char ** mntlist)
{
	int	i, j, k;
	FSspace 	*sp;

	for (i = 0, k = 0; mntlist[i] != NULL; i++) {
		for (j = k; space[j] != NULL; j++) {
			if (strcmp(mntlist[i], space[j]->fsp_mntpnt) == 0)
				break;
		}
		if (space[j] != NULL) {
			sp = space[k];
			space[k] = space[j];
			space[j] = sp;
			k++;
		}
	}
	return (space);
}

/*
 * upg_percent_free_space()
 * Calculate the upgrade overhead # of blocks required for the 'mountp' mount
 * point.
 * Parameters:	mountp	- mount point to calculate
 * Return:	# >= 0	- # of overhead blocks
 */
static int
upg_percent_free_space(char * mountp)
{
		int   oh;

		if (strcmp(mountp, "swap") == 0)
			oh = 0;
		else if (strcmp(mountp, "/") == 0)
			oh = upg_fs_freespace[ROOT_FS];
		else if (strcmp(mountp, "/usr") == 0)
			oh = upg_fs_freespace[USR_FS];
		else if (strcmp(mountp, "/usr/openwin") == 0)
			oh = upg_fs_freespace[USR_OWN_FS];
		else if (strcmp(mountp, "/opt") == 0)
			oh = upg_fs_freespace[OPT_FS];
		else if (strcmp(mountp, "/var") == 0)
			oh = upg_fs_freespace[VAR_FS];
		else if (strcmp(mountp, "/export/exec") == 0)
			oh = upg_fs_freespace[EXP_EXEC_FS];
		else if (strcmp(mountp, "/export/root") == 0)
			oh = upg_fs_freespace[EXP_ROOT_FS];
		else if (strcmp(mountp, "/export/swap") == 0)
			oh = upg_fs_freespace[EXP_SWAP_FS];
		else if (strcmp(mountp, "/export/home") == 0)
			oh = upg_fs_freespace[EXP_HOME_FS];
		else if (strcmp(mountp, "/export") == 0)
			oh = upg_fs_freespace[EXPORT_FS];
		else
			oh = 0;

		return (oh);
}

static ulong
roundup(float f)
{
	ulong u;

	u = f;
	/* round up if fractional part was truncated. */
	if (u != f)
		u++;
	return (u);
}
