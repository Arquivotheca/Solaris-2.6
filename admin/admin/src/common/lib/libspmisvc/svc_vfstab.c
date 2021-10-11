#ifndef lint
#pragma ident "@(#)svc_vfstab.c 1.12 96/07/30 SMI"
#endif

/*
 * Copyright (c) 1995-1996 by Sun Microsystems, Inc. All rights reserved.
 */
/*
 * Module:	svc_vfstab.c
 * Group:	libspmisvc
 * Description	Routines for manipulating mount list objects.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/mntent.h>
#include <sys/param.h>
#include <sys/types.h>
#include "spmisvc_lib.h"
#include "spmicommon_api.h"
#include "svc_strings.h"

/* constants */

#ifndef	MNTTYPE_PROC
#define	MNTTYPE_PROC	"proc"
#endif

#ifndef	MNTTYPE_FD
#define	MNTTYPE_FD	"fd"
#endif

/* internal prototypes */

int		_create_mount_list(Disk_t *, Dfs *, Vfsent **);
void		_free_mount_list(Vfsent **);
int		_merge_mount_entry(struct vfstab *, Vfsent **);
int		_mount_add_local_entry(Vfsent **, Disk_t *, int);
int		_mount_add_remote_entry(Dfs *, Vfsent **);
int 		_mount_filesys_all(OpType, Vfsent *);
int		_mount_remaining(Vfsent *vlist);
int		_mount_filesys_specific(char *, struct vfstab *);
void		_mount_list_print(Vfsent **);
void		_mount_list_sort(Vfsent **);
int		_merge_mount_list(OpType, Vfsent **);
int		_mount_synchronous_fs(struct vfstab *, Vfsent *);
void		_vfstab_free_entry(struct vfstab *);

/* private prototypes */

static int	_mount_order_compare(Vfsent *, Vfsent *);

/* ---------------------- internal functions ----------------------- */

/*
 * Function:	_create_mount_list
 * Description:	Create a linked list of vfstab entries for all local and remote
 *		file system, and set '*vlist' to point to the head of the list.
 *		The initial mount list entries are loaded from /etc/vfstab. As
 *		entries are added from the remote mount list or local disk list,
 *		entries in the vfstab list are either added, or replaced. The
 *		final mount point list is sorted in order of ancestory within
 *		the file system tree, ensuring that /, /usr, /usr/kvm, and /var
 *		are in the front of the file if specified.
 *
 *		NOTE:	COC interface may need to support an environment
 *			variable to specify the source vfstab file to
 *			be used only for debugging purposes
 * Scope:	internal
 * Parameters:	dlist	- pointer to head of disk structure list
 *		rlist 	- pointer to head of NFS mount structure list
 *		vlist	- address of pointer to head of mount point list
 * Return:	NOERR	- success
 *		ERROR	- error occurred
 */
int
_create_mount_list(Disk_t *dlist, Dfs *rlist, Vfsent **vlist)
{
	Dfs 	*rfs;
	Disk_t	*dp;
	int	slice;

	/*
	 * add local slice supported entries
	 */
	for (dp = dlist; dp; dp = next_disk(dp)) {
		if (sdisk_not_usable(dp))
			continue;

		WALK_SLICES(slice) {
			if (slice_ignored(dp, slice))
				continue;

			if (_mount_add_local_entry(vlist, dp, slice) == ERROR)
				return (ERROR);
		}
	}

	/*
	 * walk the list of remote file systems and add an entry
	 * for each
	 */
	for (rfs = rlist; rfs; rfs = rfs->c_next) {
		if (_mount_add_remote_entry(rfs, vlist) == ERROR)
			return (ERROR);
	}

	/*
	 * sort the mount list
	 */
	_mount_list_sort(vlist);

	return (NOERR);
}

/*
 * Function:	_free_mount_list
 * Description:	Free all entries in the mount list.
 * Scope:	internal
 * Parameters:	Vfsent **  - pointer to head of vfsent linked list
 * Return:	none
 */
void
_free_mount_list(Vfsent **vlist)
{
	Vfsent	*tmp;
	Vfsent	*vp;

	/* validat parameters */
	if (vlist == NULL)
		return;

	/*
	 * Loop throught the list and free up the data
	 */

	for (vp = *vlist; vp; vp = tmp) {
		_vfstab_free_entry(vp->entry);
		tmp = vp->next;
		free(vp);
	}
}

/*
 * Function:	_merge_mount_entry
 * Description:	Merge a vfstab entry into the vfstab linked list; check entry
 *		mount point and device against those already in the list. If
 *		one of these matches, the existing entry is preserved; a new
 * 		entry is made.
 *
 *		NOTE:	the calling routine should not reuse the entry
 *			structure because the structure is being
 *			used in the linked list
 * Scope:	internal
 * Parameters:	vent    - pointer to structure contining the new vfstab entry
 *		vlist   - a linked list of vfstab struct
 * Return:	NOERR	- entry successfully added
 *		ERROR	- error occurred
 */
int
_merge_mount_entry(struct vfstab *vent, Vfsent **vlist)
{
	Vfsent	**vp;

	/* NULL entries are considered to be already merged */
	if (vent == NULL)
		return (NOERR);

	/*
	 * check to see if the new entry conflicts with an existing entry,
	 * and if it does, preserve the existing entry; a conflict is considered
	 * to be matching special devices or matching mount point fields
	 */
	for (vp = vlist; *vp; vp = &(*vp)->next) {
		if ((strcmp(vent->vfs_special,
				(*vp)->entry->vfs_special) == 0) &&
				strncmp(vent->vfs_special, "/dev/", 5) == 0)
			return (NOERR);

		if (vent->vfs_mountp && (*vp)->entry->vfs_mountp &&
				strcmp(vent->vfs_mountp,
				(*vp)->entry->vfs_mountp) == 0)
			return (NOERR);

		/*
		 * check to make sure you didn't just delete the last entry
		 * in the list to prevent an invalid dereference in the loop
		 */
		if (*vp == NULL)
			break;
	}

	/*
	 * create a new entry and add it to the end of the list
	 */
	if ((*vp = (Vfsent *)xcalloc(sizeof (Vfsent))) == NULL)
		return (ERROR);

	(*vp)->next = (Vfsent *)NULL;
	(*vp)->entry = vent;

	/*
	 * re-sort the mount list to ensure that order is maintained
	 */
	_mount_list_sort(vlist);

	return (NOERR);
}

/*
 * Function:	_mount_add_local_entry
 * Description:	Create a mount list structure for a file system associated with
 *		a disk, populate it with information from the disk structure,
 *		and add it to the end of the 'vlist' linked list.
 *
 *		NOTE:	this does not include 'swapfile' for swapping
 *		NOTE:	system critical file systems are not automounted
 *
 * Scope:	internal
 * Parameters:	vlist	- address of pointer to head of mount point list
 *		dp	- pointer to disk structure
 *		slice	- slice index number for device
 * Return:	ERROR	- failure to allocate mount list structure
 *		NOERR	- add succeeded
 */
int
_mount_add_local_entry(Vfsent **vlist, Disk_t *dp, int slice)
{
	struct vfstab	*vp;

	/* validate parameters */
	if (dp == NULL)
		return (NOERR);

	if (vlist == NULL)
		return (ERROR);

	/* ignore vfstab-irrelevant slice types */
	if ((slice_mntpnt_isnt_fs(dp, slice) &&
			!slice_is_swap(dp, slice)) ||
			slice_size(dp, slice) == 0)
		return (NOERR);

	/*
	 * allocate a new vfstab structure and null out all fields
	 */
	if ((vp = (struct vfstab *)xcalloc(sizeof (struct vfstab))) == NULL)
		return (ERROR);

	vfsnull(vp);

	vp->vfs_special = xstrdup(make_block_device(disk_name(dp), slice));
	if (slice_is_swap(dp, slice)) {
		vp->vfs_fstype = xstrdup(MNTTYPE_SWAP);
		vp->vfs_automnt = xstrdup("no");
	} else {
		vp->vfs_fsckdev = xstrdup(
			make_char_device(disk_name(dp), slice));
		vp->vfs_mountp = xstrdup(slice_mntpnt(dp, slice));
		vp->vfs_fstype = xstrdup(MNTTYPE_UFS);
		if (slice_mntopts(dp, slice) &&
		    strlen(slice_mntopts(dp, slice)))
			vp->vfs_mntopts = xstrdup(slice_mntopts(dp, slice));

		if (_filesys_boot_critical(vp->vfs_mountp) == 1) {
			vp->vfs_fsckpass = xstrdup("1");
			vp->vfs_automnt = xstrdup("no");
		} else {
			vp->vfs_fsckpass = xstrdup("2");
			vp->vfs_automnt = xstrdup("yes");
		}
	}

	/* merge this entry into the existing mount list */
	if (_merge_mount_entry(vp, vlist) == ERROR) {
		_vfstab_free_entry(vp);
		return (ERROR);
	}

	return (NOERR);
}

/*
 * Function:	_mount_add_remote_entry
 * Description: Create a mount list structure for a DFS filesystem (only NFS).
 *		Populate it with information from the 'rfs' remote filesystem
 *		structure, and add it to the end of the 'vlist' linked list.
 *		fsckdev and fsckpass are left NULL.
 *
 *		NOTE:	/usr and /usr/openwin are mounted "ro" unless
 *			explicitly a non-default value in the remote mount
 *			record
 *		NOTE:	system critical file systems are not automounted
 *
 * Scope:	internal
 * Parameters:	vlist	- address of pointer to head of mount point list
 *		rent	- pointer to remote filesystem data structure
 * Return:	NOERR	- add succeeded
 *		ERROR	- failure to allocate mount list structure
 */
int
_mount_add_remote_entry(Dfs *rent, Vfsent **vlist)
{
	struct vfstab	*vp;
	char		buf[MAXNAMELEN];

	/* validate parameters */
	if (vlist == NULL)
		return (ERROR);

	/* a NULL entry is considered to be already added */
	if (rent == NULL)
		return (NOERR);

	/*
	 * allocate a new vfstab structure and null out all fields
	 */
	if ((vp = (struct vfstab *)xcalloc(sizeof (struct vfstab))) == NULL)
		return (ERROR);

	vfsnull(vp);

	vp->vfs_fstype = xstrdup(MNTTYPE_NFS);
	(void) sprintf(buf, "%s:%s", rent->c_hostname, rent->c_export_path);
	vp->vfs_special = xstrdup(buf);

	vp->vfs_mountp = xstrdup(rent->c_mnt_pt);

	/* system file systems are never automounted */
	if (_filesys_boot_critical(vp->vfs_mountp) == 1)
		vp->vfs_automnt = xstrdup("no");
	else
		vp->vfs_automnt = xstrdup("yes");

	/* remote /usr file systems should be mounted "ro" */
	if ((streq(vp->vfs_mountp, USR) || streq(vp->vfs_mountp, USROWN)) &&
			(rent->c_mount_opts == NULL ||
				streq(rent->c_mount_opts, "-"))) {
		vp->vfs_mntopts = xstrdup("ro");
	} else {
		vp->vfs_mntopts = (rent->c_mount_opts ?
				rent->c_mount_opts : "-");
	}

	/* merge this entry into the existing mount list */
	if (_merge_mount_entry(vp, vlist) == ERROR)
		_vfstab_free_entry(vp);

	return (NOERR);
}

/*
 * Function:	_mount_filesys_all
 * Description:	Mount file systems listed in 'vlist' relative to
 *		the install base directory, creating mount points as
 *		necessary. The 'vlist' must be sorted in file system
 *		hierarchical order to ensure mounting is successful
 *		(i.e. "/usr" precedes "/usr/openwin", etc.). Async
 *		metadata is disabled for file systems that are successfully
 *		mounted.
 *
 *		NOTE:	the 'vlist' contains both NFS and UFS mounts;
 *			NFS file systems are not mounted
 *
 * Scope:	internal
 * Parameters:	vlist	- pointer to head of mount structure linked list
 * Return:	NOERR	- no error
 *		ERROR	- error occurred
 */
int
_mount_filesys_all(OpType Operation, Vfsent *vlist)
{
	Vfsent		*vp;
	char		buf[MAXPATHLEN];
	struct vfstab	*vfsp;

	/* no mounting if either the disks, or execution are being simulated */
	if (GetSimulation(SIM_EXECUTE) || get_trace_level() > 1)
		write_status(SCR, LEVEL0, MOUNTING_TARGET);

	/*
	 * create mount points for vfstab entries (if necessary)
	 * an mount local file systems which might be required
	 * by the installation process
	 */
	WALK_LIST(vp, vlist) {
		vfsp = vp->entry;
		buf[0] = '\0';
		/*
		 * only look at entries with directory mount_p names
		 */
		if (vfsp->vfs_mountp == NULL ||
				vfsp->vfs_mountp[0] != '/')
			continue;

		/*
		 * If the operation is set to ADAPTIVE then we need to weed
		 * out all file systems with types other than ufs or s5
		 */

		if (Operation == SI_ADAPTIVE) {

			/*
			 * In the case of doing an Adaptive upgrade the mount
			 * list can contain swap.
			 */

			if (strcmp(vfsp->vfs_fstype, "swap") == 0) {
				continue;
			}

			/*
			 * If the file system is not a ufs or System V
			 * file system then we don't try and mount it.
			 */

			else if (strcmp(vfsp->vfs_fstype, "ufs") != 0 &&
			    strcmp(vfsp->vfs_fstype, "s5") != 0) {
				continue;
			}
		}

		/*
		 * create the mount point offset by the base directory
		 */
		(void) sprintf(buf, "%s%s",
			get_rootdir(),
			strcmp(vfsp->vfs_mountp, ROOT) == 0 ? "" :
				vfsp->vfs_mountp);

		if (access(buf, X_OK) != 0) {
			if (_create_dir(buf) != NOERR)
				return (ERROR);
		}

		/*
		 * mount system software file systems which are not
		 * potentially running newfs or fsck in the background
		 */
		if (_mount_synchronous_fs(vfsp, vlist) == 0)
			continue;

		if (_mount_filesys_specific(buf, vfsp) != NOERR) {
			write_notice(ERRMSG,
				MSG2_FILESYS_MOUNT_FAILED,
				vfsp->vfs_mountp,
				vfsp->vfs_special);
			return (ERROR);
		}
	}

	return (NOERR);
}

/*
 * Function:	_mount_filesys_specific
 * Description:	Mount a file system specified by 'vp'. If the file system type
 *		is NFS, specify a retry of '0' so that only one NFS mount is
 *		attempted.
 * Scope:	internal
 * Parameters:	name	- mount point offset by base directory
 *		vp	- pointer to vfstab structure
 * Return:	NOERR	- successful
 *		ERROR	- error on mount
 */
int
_mount_filesys_specific(char *name, struct vfstab *vp)
{
	char		cmd[MAXNAMELEN] = "";
	char		options[32] = "";

	/* validate parameters */
	if (name == NULL || vp == NULL)
		return (ERROR);

	if (GetSimulation(SIM_EXECUTE) || get_trace_level() > 0) {
		write_status(SCR, LEVEL1|LISTITEM,
			MSG2_FILESYS_MOUNT,
			name,
			vp->vfs_special);
	}

	if (strcmp(vp->vfs_fstype, MNTTYPE_NFS) == 0)
		(void) strcpy(options, "-o retry=0");

	(void) sprintf(cmd,
		"/sbin/mount %s -F %s %s %s >/dev/null 2>&1",
		options, vp->vfs_fstype, vp->vfs_special, name);

	/* only execute the mount command if running live */
	if (!GetSimulation(SIM_EXECUTE)) {
		if (system(cmd) != 0)
			return (ERROR);

		_filesys_fiodio(name, 1);
	}

	return (NOERR);
}

/*
 * Function:	_mount_list_print
 * Description:	Print the mount list for tracing purposes.
 * Scope:	internal
 * Parameters:	vlist	- pointer to head of list to be printed
 * Return:	none
 */
void
_mount_list_print(Vfsent **vlist)
{
	Vfsent	*vp;

	/* null lists have nothing to print */
	if (vlist == NULL)
		return;

	WALK_LIST(vp, *vlist) {
		write_status(LOGSCR, LEVEL1,
		    "%s\t%s\t%s\t%s\t%s\t%s\t%s",
		    vp->entry->vfs_special ? vp->entry->vfs_special : "-",
		    vp->entry->vfs_fsckdev ? vp->entry->vfs_fsckdev : "-",
		    vp->entry->vfs_mountp ? vp->entry->vfs_mountp : "-",
		    vp->entry->vfs_fstype ? vp->entry->vfs_fstype : "-",
		    vp->entry->vfs_fsckpass ? vp->entry->vfs_fsckpass : "-",
		    vp->entry->vfs_automnt ? vp->entry->vfs_automnt : "-",
		    vp->entry->vfs_mntopts ? vp->entry->vfs_mntopts : "-");
	}
}

/*
 * Function:	_mount_list_sort
 * Description:	Bubble sort the mount list based on the alphabetic ordering of
 *		the mount point names. This ensures that the list is ordered in
 *		order of hierarchy in the file system (needed when mounting
 *		file systems later on). Make sure "/", "/usr", "/usr/platform",
 *		"/var", and "/tmp" are at the head of the list.
 * Scope:	internal
 * Parameters:	vlist	 - address of pointer to head of mount list
 * Return:	none
 */
void
_mount_list_sort(Vfsent **vlist)
{
	Vfsent	**lp;
	Vfsent	**rp;
	Vfsent	*tmp;

	/* NULL list is considered sorted */
	if (vlist == NULL)
		return;

	for (lp = vlist; *lp; lp = &(*lp)->next) {
		rp = lp;
		while (*rp && (*rp)->next) {
			rp = &(*rp)->next;
			if (_mount_order_compare(*lp, *rp) != 0) {
				if ((*lp)->next == *rp) {
					tmp = *rp;
					*rp = tmp->next;
					tmp->next = *lp;
					*lp = tmp;
				} else {
					tmp = *rp;
					*rp = *lp;
					*lp = tmp;
					tmp = (*rp)->next;
					(*rp)->next = (*lp)->next;
					(*lp)->next = tmp;
				}
			}
		}
	}
}

/*
 * Function:	_merge_mount_list
 * Description:	Merge vfstab entries from the existing vfstab with the
 *		explicitly specified mount entries. Exclude all existing
 *		entries which:
 *
 *		(1)  have duplicate special device entries with existing
 *			entries
 *		(2)  have duplicate mount point names
 *		(3)  are swap files
 *
 * Scope:	internal
 * Parameters:	vlist	- address of pointer to head of mount list
 * Return:	NOERR	- merge successful
 *		ERROR	- merge failed
 */
int
_merge_mount_list(OpType Operation, Vfsent **vlist)
{
	struct vfstab   *vp;
	struct vfstab   vfstab;
	Disk_t		*dp;
	FILE		*fp;
	int		status = NOERR;
	char		vfile[32] = "";
	char		*v = NULL;
	char		buf[MAXPATHLEN];

	if (vlist == NULL)
		return (ERROR);

	/*
	 * open the appropriate vfstab file for reading
	 */
	if (((v = getenv("SYS_VFSTAB")) != NULL) && *v)
		(void) strcpy(vfile, v);
	else
		(void) sprintf(vfile, "%s/etc/vfstab", get_rootdir());

	if (get_trace_level() > 1) {
		write_status(LOGSCR, LEVEL1|LISTITEM,
			MSG1_VFSTAB_ORIG_OPEN,
			vfile);
	}

	if ((fp = fopen(vfile, "r")) == NULL) {
		/*
		 * failure to open the original vfstab file is only an
		 * error if you are running live; otherwise it is just
		 * a warning only if an explicit file was specified
		 */
		if (GetSimulation(SIM_EXECUTE)) {
			if (v != NULL)
				write_notice(WARNMSG, MSG_OPEN_FAILED, vfile);
		} else {
			write_notice(ERRMSG, MSG_OPEN_FAILED, vfile);
			return (ERROR);
		}
	} else {
		/*
		 * scan through the existing vfstab file and add
		 * in any entries which are not swap, or on selected
		 * disks
		 */
		for (vfsnull(&vfstab); getvfsent(fp, &vfstab) == 0;
				vfsnull(&vfstab)) {

			if (Operation != SI_ADAPTIVE) {
				/*
				 * strip out swap files
				 */
				if (strcmp(vfstab.vfs_fstype,
				    MNTTYPE_SWAP) == 0 &&
				    vfstab.vfs_special[0] == '/')
					continue;
			}

			/*
			 * strip out existing entries on selected disks
			 */
			if ((dp = find_disk(vfstab.vfs_special)) != NULL &&
					disk_selected(dp))
				continue;

			/*
			 * If this is an adaptive upgrade then we have to weed
			 * out all of the currently mountable ufs or s5 (the
			 * only ones that we currently allow) file systems.
			 * This is done since the vfstab list passed into this
			 * function contains all of the mount points required to
			 * mount the new file system layout.
			 */

			if (Operation == SI_ADAPTIVE) {

				/*
				 * So, if the file system being checked is
				 * a swap or either a ufs or s5 file system
				 * and we can map the device name for the
				 * mount point and the fsck to it's real
				 * device entry in the /devices tree, then
				 * this file system entry will be replaced
				 * below if necessary.
				 */

				if (streq(vfstab.vfs_fstype, "swap") ||
				    ((streq(vfstab.vfs_fstype, "ufs") ||
				    streq(vfstab.vfs_fstype, "s5")) &&
				    _map_to_effective_dev(vfstab.vfs_special,
					buf) == 0 &&
				    _map_to_effective_dev(vfstab.vfs_fsckdev,
					buf) == 0)) {
					continue;
				}
			}

			if ((vp = (struct vfstab *)
				    xcalloc(sizeof (struct vfstab))) == NULL) {
				status = ERROR;
				break;
			}

			vp->vfs_special = xstrdup(vfstab.vfs_special);
			vp->vfs_fsckdev = xstrdup(vfstab.vfs_fsckdev);
			vp->vfs_mountp = xstrdup(vfstab.vfs_mountp);
			vp->vfs_fstype = xstrdup(vfstab.vfs_fstype);
			vp->vfs_fsckpass = xstrdup(vfstab.vfs_fsckpass);
			vp->vfs_automnt = xstrdup(vfstab.vfs_automnt);
			vp->vfs_mntopts = xstrdup(vfstab.vfs_mntopts);

			/* merge the new entry into the mount list */
			if (_merge_mount_entry(vp, vlist) == ERROR) {
				status = ERROR;
				_vfstab_free_entry(vp);
				break;
			}
		}

		(void) fclose(fp);
	}

	return (status);
}

/*
 * Function:	_mount_synchronous_fs
 * Description:	Determine if a specific file system in the mount list
 *		should be created and mounted synchronously, or in the
 *		background
 * Scope:	internal
 * Parameters:	vfsp	- pointer to the specific vfstab entry being
 *			  considered
 *		vlist	- pointer to the head of the list for all vfstab
 *			  entries associated with the system
 * Return:	0	- the file system should be handled asynchronously
 *		1	- the file system should be handled synchronously
 */
int
_mount_synchronous_fs(struct vfstab *vfsp, Vfsent *vlist)
{
	char	buf[MAXPATHLEN] = "";
	int	length;
	Vfsent	*vp;

	/* validate parameters */
	if (vfsp == NULL || vlist == NULL)
		return (0);

	/* non-UFS file systems are not processed synchronously */
	if (strcmp(vfsp->vfs_fstype, MNTTYPE_UFS) != 0)
		return (0);

	/* this should never happen because of the UFS test, but... */
	if (vfsp->vfs_mountp == NULL)
		return (0);

	/* if the entry is a system ancestor, it is syncronous */
	if (_system_fs_ancestor(vfsp->vfs_mountp) == 1)
		return (1);

	/*
	 * if the entry has other mount entries which are directory
	 * children (farther down in the file system namespace), then
	 * it is synchronous
	 */
	(void) strcpy(buf, vfsp->vfs_mountp);
	(void) strcat(buf, "/");
	length = (int) strlen(buf);
	WALK_LIST(vp, vlist) {
		if (vp->entry && vp->entry->vfs_mountp &&
				strncmp(vp->entry->vfs_mountp,
					buf, length) == 0)
			return (1);
	}

	return (0);
}

/*
 * Function:	_vfstab_free_entry
 * Description:	Deallocate allocated space from vfstab structure.
 * Scope:	internal
 * Parameters:	vp	- pointer to vfstab structure to be deallocated
 * Return:	none
 */
void
_vfstab_free_entry(struct vfstab *vp)
{
	/* a null structure pointer is considere freed */
	if (vp == NULL)
		return;

	free(vp->vfs_special);
	free(vp->vfs_fsckdev);
	free(vp->vfs_mountp);
	free(vp->vfs_fstype);
	free(vp->vfs_fsckpass);
	free(vp->vfs_automnt);
	free(vp->vfs_mntopts);
	free(vp);
}

/*
 * Function:	_mount_remaining
 * Description:	Mount all file systems that are in the vlist that
 *		have not been previously mounted.  This usually are the
 *		file systems that have been newfs'd in the background.
 *
 *		NOTE:	the 'vlist' contains both NFS and UFS mounts;
 *			NFS file systems are not mounted
 *
 * Scope:	internal
 * Parameters:	vlist	- pointer to head of mount structure linked list
 * Return:	NOERR	- no error
 *		ERROR	- error occurred
 */
int
_mount_remaining(Vfsent *vlist)
{
	Vfsent		*vp;
	struct vfstab	*vfsp;
	char		buf[MAXPATHLEN];

	WALK_LIST(vp, vlist) {
		vfsp = vp->entry;

		/*
		 * We are only mounting local file systems
		 */

		if (strcmp(vfsp->vfs_fstype, MNTTYPE_UFS) != 0)
			continue;

		if (vfsp->vfs_mountp == NULL)
			continue;

		/*
		 * mount all unmounted file systems which were
		 * not previously mounted
		 */

		if (_mount_synchronous_fs(vfsp, vlist) == 1)
			continue;

		(void) sprintf(buf, "%s%s",
		    get_rootdir(),
		    strcmp(vfsp->vfs_mountp, ROOT) == 0 ? "" :
		    vfsp->vfs_mountp);

		if (_mount_filesys_specific(buf, vfsp) !=
		    NOERR) {
			write_notice(WARNMSG,
			    MSG2_FILESYS_MOUNT_FAILED,
			    vfsp->vfs_mountp,
			    vfsp->vfs_special);
		}
	}
	return (NOERR);
}

/* ---------------------- private functions ----------------------- */

/*
 * Function:	_mount_order_compare
 * Description:	Mount list comparitor function. Rankings are:
 *
 *		(1) MNTTYPE_PROC and MNTTYPE_FD (relatively strcoll)
 *		(2) MNTTYPE_SWAP
 *		(3) system critical file systems (e.g. / and /usr)
 *		(4) special devices of /dev/<*> and a pathname mount point
 *		(5) non-MNTTYPE_TMPFS (relatively strcoll)
 *		(6) MNTTYPE_TMPFS (relatively strcoll)
 *
 * Scope:	private
 * Parameters:	lp	- pointer to the left vfstab entry
 *		rp	- pointer to the right vfstab entry
 * Return:	0	- the strings are in the correct order
 *		1	- the strings are in reverse order
 */
static int
_mount_order_compare(Vfsent *lp, Vfsent *rp)
{
	char	*lmp;
	char	*rmp;
	char	*lsp;
	char	*rsp;
	char	*lfs;
	char	*rfs;
	int	left = -1;
	int	right = -1;

	/*
	 * if any of the parameters are invalid, we will always
	 * return with a "correct order" status
	 */
	if (lp == NULL ||
			lp->entry == NULL ||
			rp == NULL ||
			rp->entry == NULL)
		return (0);

	lmp = lp->entry->vfs_mountp;
	rmp = rp->entry->vfs_mountp;
	lsp = lp->entry->vfs_special;
	rsp = rp->entry->vfs_special;
	lfs = lp->entry->vfs_fstype;
	rfs = rp->entry->vfs_fstype;

	/*
	 * establish ranking for left entry
	 */
	if (strcmp(lfs, MNTTYPE_PROC) == 0)
		left = 0;
	else if (strcmp(lfs, MNTTYPE_FD) == 0)
		left = 0;
	else if (strcmp(lfs, MNTTYPE_SWAP) == 0)
		left = 1;
	else if (lmp != NULL && _filesys_boot_critical(lmp) == 1)
		left = 2;
	else if (strncmp(lsp, "/dev/", 5) == 0 &&
			lmp != NULL && is_pathname(lmp))
		left = 3;
	else if (strcmp(lfs, MNTTYPE_TMPFS) != 0)
		left = 4;
	else
		left = 5;

	/*
	 * establish ranking for right entry
	 */
	if (strcmp(rfs, MNTTYPE_PROC) == 0)
		right = 0;
	else if (strcmp(rfs, MNTTYPE_FD) == 0)
		right = 0;
	else if (strcmp(rfs, MNTTYPE_SWAP) == 0)
		right = 1;
	else if (rmp != NULL && _filesys_boot_critical(rmp) == 1)
		right = 2;
	else if (strncmp(rsp, "/dev/", 5) == 0 &&
			rmp != NULL && is_pathname(rmp))
		right = 3;
	else if (strcmp(rfs, MNTTYPE_TMPFS) != 0)
		right = 4;
	else
		right = 5;

	/*
	 * determine if the two entries are in the correct or
	 * reversed order
	 */
	if (left < right)
		return (0);
	else if (left > right)
		return (1);
	else if (lmp == NULL)
		return (0);
	else if (rmp == NULL)
		return (1);
	else
		return (strcoll(lmp, rmp) <= 0 ? 0 : 1);
}
